#include "m6378.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>       /* sigaction() */
#include <sys/mman.h>     /* shm_open(), mmap() */
#include <fcntl.h>        /* For O_* constants  */
#include <sys/stat.h>     /* For mode constants */
#include <semaphore.h>    /* sem_*()  */
#include <unistd.h>       /* sleep()  */
#include <string.h>       /* memcpy() */
#include <time.h>         /* clock_gettime() */

//#define TELL_REMAP
//#define TELL_RESIZE
//#define TELL_DEFRAG

/******************************************************************************/

#ifndef USE_UNNAMED_SEM
# if defined(__linux__)
#   define USE_UNNAMED_SEM 1 /* posix */
# elif defined(__CYGWIN__) && !defined(_WIN32)
#   define USE_UNNAMED_SEM 2 /* named posix */
# elif defined(__APPLE__) && defined(__MACH__)
#   define USE_UNNAMED_SEM 3 /* sys5 */
# else
#   warning "No unnamed semaphore type specified. Will try named posix."
#   define USE_UNNAMED_SEM 1 /* posix */
# endif
#endif

#if USE_UNNAMED_SEM == 1
  /* Posix unnamed semaphores work OK on linux. */
# warning "Using posix unnamed sems."
# define SEM_T sem_t
# define SEM_UNNAMED_OPEN(k,m)   (1)
# define SEM_UNNAMED_ATTACH(k,m) (1)
# define SEM_INIT(sp,i,ui)       sem_init(sp,i,ui)
# define SEM_UNNAMED_CLOSE()     (1)
# define SEM_UNNAMED_UNLINK()    (1)
# define SEM_DESTROY(sp)         sem_destroy(sp)
# define SEM_POST(sp)            sem_post(sp)
# define SEM_WAIT(sp)            sem_wait(sp)
//# define SEM_GETVALUE(sp,ip)     sem_getvalue(sp,ip)
#elif USE_UNNAMED_SEM == 2
  /* Cygwin's unnamed posix semaphores are hopelessly broken.
   * They do support sys5 semaphores (must have cygserver running),
   * but are painfully slow. Here posix unnamed sems are simulated 
   * with posix named semaphores and seem to work OK on acer-ax3400. */
# warning "Using named sems for posix unnamed sems."
# include "posn-unnamed-sem.h"
#elif USE_UNNAMED_SEM == 3
  /* OSX does not support unnamed posix semaphores. sys5 semaphores
   * here seem to work OK in their place. */
# warning "Using sys5 sems for posix unnamed sems."
# include "sys5-unnamed-sem.h"
#else
# error "Unknown directive USE_UNNAMED_SEM=?"
#endif

/*
 * sshm -- static shared memory. It does not change while running.
 * dshm -- dynamic shared memory. It may expand + its pointer may change.
 * This defines the initial size of the dshm buffer.
 */
#define INITIAL_DSHMSIZE (4096*info.wordsize)

/* 
 * See pg 283, line 23 of the MPI 2.2 standard.
 * "The error codes satisfy,
 * 0 = MPI_SUCCESS < MPI_ERR_... < MPI_ERR_LASTCODE
 * All errors here so far are handled by a panic abort.
 */
#define M6378_SUCCESS 0

/* 
 * Bash's wait command returns the exit status of the pid when the
 * process exits normally, or if terminated by a signal n (error),
 * wait returns 128+n. wait also returns status 127 to indicate no 
 * running job with pid found.
 * This is what macro EXIT_SIGERROR below is used for.
 * In normal use, never use exit(status) with status >= 127.
 */
#define EXIT_SIGERROR 128

#ifndef MAX
# define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef MIN
# define MIN(a,b) ((a)<(b)?(a):(b))
#endif

/******************************************************************************/

static
char sshm_name[256], dshm_name[256]; /* *_open() NAME_MAX = 255 */
static
char *ssem_name; /* OK to use sshm_name. */

static
struct _Info {
  sem_t  *mutex0; /* Named semaphore used in init and finalize only. */
  int    sshm_fd, dshm_fd;
  char   *sshm, *dshm;  
  size_t sshm_size, dshm_size; /* Store dshm_size for in-process use. */
  struct _Sshm_HInfo *sshm_hinfo;
  struct _Sshm_PInfo *sshm_pinfo;
  struct _Sshm_TInfo *sshm_tinfo;
  int    rel_number; /* This id's "release number". Only used in finalize. */
  pid_t  pid, gid;
  int    np, id;
  size_t wordsize, pagesize;
  struct timespec start_time;
  int    clock_resolution; /* (in nanosecs) Most systems should be 1. */
} info = { SEM_FAILED, -1, -1, MAP_FAILED, MAP_FAILED,
            0, 0, NULL, NULL, NULL, 0 };

struct _Sshm_HInfo {
  SEM_T  bar_gate1, bar_gate2, bar_mutex;
  int    bar_count;
  SEM_T  dshm_make, dshm_mutx;
  int    dshm_users;
  size_t dshm_size; /* Store dshm_size for interprocess use. */
  size_t dshm_next;
  size_t dshm_frag; /* Keeps track of fragmented dshm. */
  int    rel_count; /* Shared "release count". Only used in finalize. */
};

struct _Sshm_PInfo {
  pid_t pid;
};

struct _Sshm_TInfo {
  SEM_T  wok, rok;
  size_t dbuf_size, dbuf_loc;
};

struct _MsgTemplate {
  size_t ndata;
  double data;
};

/******************************************************************************/

//static /* Remove this??? */
//int semValue( SEM_T *sem ) {
//  int ival;
//  SEM_GETVALUE( sem, &ival );
//  return(ival);
//}

static 
int determineWordAlignment() {
  /* Let the compiler figure out word alignment size. */
  struct { char c; double d; } align;
  return( (int)( (char*)&align.d - (char*)&align.c ) );
}

static
size_t wordAlignedSizeof( size_t nbytes ) {
  /* Assuming wordsize is a power of 2. */
  return( ((nbytes-1)&(~(info.wordsize-1)))+info.wordsize );
}

static
size_t pageAlignedSizeof( size_t nbytes ) {
  /* Assuming pagesize is a power of 2. */
  return( ((nbytes-1)&(~(info.pagesize-1)))+info.pagesize );
}

static 
size_t fd2size( int fd ) {
  struct stat sbuf;
  fstat( fd, &sbuf );
  return( (size_t)sbuf.st_size );
}

static void release( int );

static 
void default_handler( int sig, siginfo_t *siginfo, void *context ) {
  switch( sig ) {
    case(SIGTERM):
      release(1);
      exit(EXIT_SIGERROR+SIGTERM);
    case(SIGINT):
      break;
  }
}

static
void runtime_handler( int sig, siginfo_t *siginfo, void *context ) {
  if( info.sshm && info.sshm_pinfo ) {
    int id;
    for( id = info.id + 1; id < info.np; id++ ) {
      pid_t topid = info.sshm_pinfo[id].pid;
      if( topid > 0 ) kill( topid, SIGTERM );
    }
  } 
  release(1);
  fprintf( stderr, "*** Fatal *** Process %d caught runtime error: ", info.id );
  /* According to posix, these dump core by default. */
  switch(sig) {
    case SIGABRT:
      fprintf( stderr, "SIGABRT" );
      break;
    case SIGBUS:
      fprintf( stderr, "SIGBUS" );
      break;
    case SIGFPE:
      fprintf( stderr, "SIGFPE" );
      break;
    case SIGILL:
      fprintf( stderr, "SIGILL" );
      break;
    case SIGQUIT:
      fprintf( stderr, "SIGQUIT" );
      break;
    case SIGSEGV:
      fprintf( stderr, "SIGSEGV" );
      break;
    case SIGSYS:
      fprintf( stderr, "SIGSYS" );
      break;
    case SIGTRAP:
      fprintf( stderr, "SIGTRAP" );
      break;
    case SIGXCPU:
      fprintf( stderr, "SIGXCPU" );
      break;
    case SIGXFSZ:
      fprintf( stderr, "SIGXFSZ" );
      break;
  }
#ifdef SAVE_COREDUMP
  fprintf( stderr, ".\nAll processes will terminate with core dump.\n" );
  fflush( stderr );
  signal( sig, SIG_DFL );
  raise( sig ); /* Signal the default handler. */
#else
  fprintf( stderr, ".\nAll processes will terminate with no core dump.\n" );
  fflush( stderr );
  exit(EXIT_SIGERROR+sig);
#endif
}

static
void abort_Init() {
  /* Usual exit signal with error code SIGINT signals parent shell that 
   * this initization failed. */
  fflush( stderr );
  release(1);
  exit(EXIT_SIGERROR+SIGINT);
}

static
void acknowledge_Init() {
  /* SIGTERM signals parent shell that this process initialized. */
  if( kill( getppid(), SIGTERM ) < 0 ) {
    perror( "acknowledge_Init kill error" );
    abort_Init();
  }
}

int M6378_Init( int *argc, char **argv[] ) {
  int i;
  struct sigaction sigact;

  if( *argc < 4 ) {
    fprintf( stderr, "%s must be started from the m6378run script.\n", (*argv)[0] );
    abort_Init();
  }
  info.gid = atoi((*argv)[1]);
  info.np  = atoi((*argv)[2]);
  if( info.gid  != getppid()
   || getppid() != getpgrp()
   || info.np < 1 ) {
    fprintf( stderr, "%s must be started from the m6378run script.\n", (*argv)[0] );
    abort_Init();
  }
  /* To explain the second condition above:
   * In shells with job control (interactive shells) each job is put in its
   * own process group, see getpgrp().
   * In shells without job control (non interactive shells) commands are run 
   * in the shell's process group which is normally its pid. 
   * Thanks Barmar @ unix stack exchange.
   */

  sprintf( sshm_name, "/S-%d", info.gid );
  sprintf( dshm_name, "/D-%d", info.gid );
  ssem_name = sshm_name;

//  /* Case when program is only doing clean-up duties. */
//  if( strcmp((*argv)[3],"M6378 Clean-up") == 0 ) {
//    SEM_UNNAMED_ATTACH(info.gid,5+2*info.np*(info.np-1));
//    SEM_UNNAMED_UNLINK();
//    shm_unlink( sshm_name );
//    shm_unlink( dshm_name );
//    sem_unlink( ssem_name );
//    exit(0);
//  }

  info.id  = atoi((*argv)[3]);
  if( info.id < 0 || info.id >= info.np ) {
    fprintf( stderr, "%s must be started from the m6378run script.\n", (*argv)[0] );
    abort_Init();
  }
  info.pid = getpid();

  *argc -= 3;
  for( i = 1; i < *argc; i++ )
    (*argv)[i] = (*argv)[i+3];

  info.wordsize = determineWordAlignment();
  info.pagesize = sysconf(_SC_PAGE_SIZE);

  info.mutex0  = sem_open( ssem_name, O_CREAT, S_IRUSR|S_IWUSR, 1 );
  if( info.mutex0 == SEM_FAILED ) {
    fprintf(stderr, "M6378_Init failed to open named semaphore.\n" );
    abort_Init();
  }

  sem_wait(info.mutex0);

    info.sshm_fd = shm_open( sshm_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR );
    info.dshm_fd = shm_open( dshm_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR );
    if( info.sshm_fd == -1 || info.dshm_fd == -1 ) {
      fprintf(stderr, "M6378_Init opening shared memory error.\n" );
      abort_Init();
    }
    info.sshm_size = pageAlignedSizeof(
                       sizeof( struct _Sshm_HInfo )
                     + sizeof( struct _Sshm_PInfo ) * info.np
                     + sizeof( struct _Sshm_TInfo ) * info.np*info.np );
    info.dshm_size = pageAlignedSizeof(INITIAL_DSHMSIZE);

    if( fd2size(info.sshm_fd) == 0 ) {
      struct _Sshm_HInfo *hinfo;
      struct _Sshm_TInfo *tinfo;
      int i, j;
      /* This sets shm size and also initializes it to \0. */
      if( 0 > ftruncate( info.sshm_fd, info.sshm_size ) ) {
        fprintf(stderr, "M6378_Init ftruncate error.\n" );
        abort_Init();
      }
      info.sshm = mmap( NULL, info.sshm_size,
                        PROT_READ|PROT_WRITE, MAP_SHARED, info.sshm_fd, 0 );
      if( info.sshm == MAP_FAILED ) {
        fprintf(stderr, "M6378_Init mmap error.\n" );
        abort_Init();
      }
      if( 0 > ftruncate( info.dshm_fd, info.dshm_size ) ) {
        fprintf(stderr, "M6378_Init ftruncate error.\n" );
        abort_Init();
      }
      info.dshm = mmap( NULL, info.dshm_size,
                        PROT_READ|PROT_WRITE, MAP_SHARED, info.dshm_fd, 0 );
      if( info.dshm == MAP_FAILED ) {
        fprintf(stderr, "M6378_Init mmap error.\n" );
        abort_Init();
      }
      hinfo = (struct _Sshm_HInfo*)(info.sshm);

      if( 0 > SEM_UNNAMED_OPEN(info.gid,5+2*info.np*(info.np-1)) ) {
        fprintf(stderr, "M6378_Init SEM_UNNAMED_OPEN error.\n" );
        abort_Init();
      }
      if( 0 > SEM_INIT(&(hinfo->dshm_make), 1, 1) ) {
        fprintf(stderr, "M6378_Init SEM_INIT error.\n");
        abort_Init();
      }
      if( 0 > SEM_INIT(&(hinfo->dshm_mutx), 1, 1) ) {
        fprintf(stderr, "M6378_Init SEM_INIT error.\n");
        abort_Init();
      }
      hinfo->dshm_users = 0;

      if( 0 > SEM_INIT(&(hinfo->bar_gate1), 1, 0) ) {
        fprintf(stderr, "M6378_Init SEM_INIT error.\n");
        abort_Init();
      }
      if( 0 > SEM_INIT(&(hinfo->bar_gate2), 1, 0) ) {
        fprintf(stderr, "M6378_Init SEM_INIT error.\n");
        abort_Init();
      }
      if( 0 > SEM_INIT(&(hinfo->bar_mutex), 1, 1) ) {
        fprintf(stderr, "M6378_Init SEM_INIT error.\n");
        abort_Init();
      }
      hinfo->bar_count = 0;
      hinfo->dshm_size = info.dshm_size; /* Copy to shared memory. */
      hinfo->dshm_next = 0;
      hinfo->dshm_frag = 0;
      hinfo->rel_count = 0;

      tinfo = (struct _Sshm_TInfo*)(
         info.sshm + sizeof(struct _Sshm_HInfo)
                   + sizeof(struct _Sshm_PInfo)*info.np );
      for( j = 0; j < info.np; j++ )
      for( i = 0; i < info.np; i++ ) {
        if( i == j ) continue;
        if( 0 > SEM_INIT(&(tinfo[i+j*info.np].wok), 1, 1) ) {
          fprintf(stderr, "M6378_Init SEM_INIT(%d,%d).wok error.\n", i, j);
          abort_Init();
        }
        if( 0 > SEM_INIT(&(tinfo[i+j*info.np].rok), 1, 0) ) {
          fprintf(stderr, "M6378_Init SEM_INIT(%d,%d).rok error.\n", i, j);
          abort_Init();
        }
      }
    } 
    else {
      info.sshm = mmap( NULL, info.sshm_size,
                        PROT_READ|PROT_WRITE, MAP_SHARED, info.sshm_fd, 0 );
      if( info.sshm == MAP_FAILED ) {
        fprintf(stderr, "M6378_Init mmap error.\n" );
        abort_Init();
      }
      info.dshm = mmap( NULL, info.dshm_size,
                        PROT_READ|PROT_WRITE, MAP_SHARED, info.dshm_fd, 0 );
      if( info.dshm == MAP_FAILED ) {
        fprintf(stderr, "M6378_Init mmap error.\n" );
        abort_Init();
      }
      if( 0 > SEM_UNNAMED_ATTACH(info.gid,5+2*info.np*(info.np-1)) ) {
        fprintf(stderr, "M6378_Init SEM_UNNAMED_ATTACH error.\n" );
        abort_Init();
      }
    }

  sem_post(info.mutex0);

  info.sshm_hinfo =  (struct _Sshm_HInfo*)(info.sshm);
  info.sshm_pinfo =  (struct _Sshm_PInfo*)
         ((char*)info.sshm_hinfo + sizeof(struct _Sshm_HInfo));
  info.sshm_tinfo =  (struct _Sshm_TInfo*)
         ((char*)info.sshm_pinfo + sizeof(struct _Sshm_PInfo)*info.np);
  info.sshm_pinfo[info.id].pid = info.pid;

  /* Borrowing start_time struct to get timer nano-resolution. */
  clock_getres( CLOCK_MONOTONIC, &info.start_time );
  info.clock_resolution = (int)info.start_time.tv_nsec; 
  /* OK. This is permanent. */
  clock_gettime( CLOCK_MONOTONIC, &info.start_time );

  /* Setup default signal handler. */
  memset( &sigact, '\0', sizeof(sigact) );
  sigact.sa_sigaction = &default_handler;
  sigact.sa_flags = SA_SIGINFO;
  if( sigaction( SIGTERM, &sigact, NULL ) < 0 
   || sigaction( SIGINT,  &sigact, NULL ) < 0 ) {
    perror( "M6378_Init sigaction error" );
    abort_Init();
  }

  /* Setup runtime signal handler. */
  sigact.sa_sigaction = &runtime_handler;
  sigact.sa_flags = SA_SIGINFO|SA_RESETHAND|SA_NODEFER;
  if( sigaction( SIGABRT, &sigact, NULL ) < 0
   || sigaction( SIGBUS,  &sigact, NULL ) < 0
   || sigaction( SIGFPE,  &sigact, NULL ) < 0
   || sigaction( SIGILL,  &sigact, NULL ) < 0
   || sigaction( SIGQUIT, &sigact, NULL ) < 0
   || sigaction( SIGSEGV, &sigact, NULL ) < 0
   || sigaction( SIGSYS,  &sigact, NULL ) < 0
   || sigaction( SIGTRAP, &sigact, NULL ) < 0
   || sigaction( SIGXCPU, &sigact, NULL ) < 0
   || sigaction( SIGXFSZ, &sigact, NULL ) < 0 ) {
    perror( "M6378_Init sigaction error" );
    abort_Init();
  }

  /* Acknowledge to parent shell that we correctly initialized. */
  acknowledge_Init();

  M6378_Barrier();
  return( M6378_SUCCESS );
}

double M6378_Dtime( int *resolution ) {
  struct timespec t;
  if( resolution ) 
    *resolution = info.clock_resolution;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return( (double)(t.tv_sec - info.start_time.tv_sec)
        + (t.tv_nsec - info.start_time.tv_nsec)*1.0e-9 );
}

static
void abort_PostInit( int status ) {
  /* Terminate jobs possibly waiting before us in bash's wait queue. */
  if( info.sshm && info.sshm_pinfo ) {
    int id;
    for( id = info.id + 1; id < info.np; id++ ) {
      pid_t topid = info.sshm_pinfo[id].pid;
      if( topid > 0 ) kill( topid, SIGTERM );
    }
  } 
  else {
    fprintf( stderr, "abort_PostInit called when in an unexpected state.\n" );
  }
  fflush( stderr );
  release(1);
  exit( EXIT_SIGERROR + status );
}

int M6378_Abort( int status ) {
  /* NSIG below seems to be generally defined in sys/signal.h. 
   * Its value must agree with NSIG computed in m6378run. */
  abort_PostInit( NSIG+status );
  return( M6378_SUCCESS );
}

int M6378_Comm_rank( int *rank ) {
  *rank = info.id;
  return( M6378_SUCCESS );
}

int M6378_Comm_size ( int *nproc ) {
  *nproc = info.np;
  return( M6378_SUCCESS );
}

int M6378_Barrier() {
  struct _Sshm_HInfo *hinfo = info.sshm_hinfo;
  SEM_WAIT(&(hinfo->bar_mutex));
    hinfo->bar_count++;
    if( hinfo->bar_count == info.np ) {
      int n;
      for( n = 0; n < info.np; n++ )
        SEM_POST(&(hinfo->bar_gate1));
    }
  SEM_POST(&(hinfo->bar_mutex));
  SEM_WAIT(&(hinfo->bar_gate1));

  SEM_WAIT(&(hinfo->bar_mutex));
    hinfo->bar_count--;
    if( hinfo->bar_count == 0 ) {
      int n;
      for( n = 0; n < info.np; n++ )
        SEM_POST(&(hinfo->bar_gate2));
    }
  SEM_POST(&(hinfo->bar_mutex));
  SEM_WAIT(&(hinfo->bar_gate2));
  return( M6378_SUCCESS );
}

static
void release( int unlink ) {
  if( unlink ) {
    struct _Sshm_HInfo *hinfo = info.sshm_hinfo;
    struct _Sshm_TInfo *tinfo = info.sshm_tinfo;
    if( hinfo ) {
      SEM_DESTROY(&hinfo->dshm_make);
      SEM_DESTROY(&hinfo->dshm_mutx);
      SEM_DESTROY(&hinfo->bar_gate1);
      SEM_DESTROY(&hinfo->bar_gate2);
      SEM_DESTROY(&hinfo->bar_mutex);
    }
    if( tinfo ) {
      int i, j;
      for( j = 0; j < info.np; j++ )
      for( i = 0; i < info.np; i++ ) {
        if( i == j ) continue;
        SEM_DESTROY(&(tinfo[i+j*info.np].wok));
        SEM_DESTROY(&(tinfo[i+j*info.np].rok));
      }
    }
    SEM_UNNAMED_UNLINK();
    if( info.sshm_fd != -1 ) shm_unlink( sshm_name );
    if( info.dshm_fd != -1 ) shm_unlink( dshm_name );
  }
  SEM_UNNAMED_CLOSE();

  if( info.mutex0 != SEM_FAILED ) {
    /* The semaphore name is removed immediately. The semaphore is destroyed 
     * once all other processes that have the semaphore open close it. */
    sem_unlink( ssem_name );
    sem_close(info.mutex0);
    info.mutex0 = SEM_FAILED;
  }
  if( info.sshm != MAP_FAILED ) {
    munmap( info.sshm, info.sshm_size );
    info.sshm = MAP_FAILED;
    info.sshm_size = 0;
  }
  info.sshm_hinfo = (struct _Sshm_HInfo*)0;
  info.sshm_pinfo = (struct _Sshm_PInfo*)0;
  info.sshm_tinfo = (struct _Sshm_TInfo*)0;
  if( info.sshm_fd != -1 ) {
    close( info.sshm_fd );
    info.sshm_fd = -1;
  }
  if( info.dshm != MAP_FAILED ) {
    munmap( info.dshm, info.dshm_size );
    info.dshm = MAP_FAILED;
    info.dshm_size = 0;
  }
  if( info.dshm_fd != -1 ) {
    close( info.dshm_fd );
    info.dshm_fd = -1;
  }
}

/* 
 * The MPI standard requires that MPI_Finalize be called only by the same 
 * thread that initialized MPI with MPI_Init. The number of processes 
 * running after this routine is called is undefined; it is best not to 
 * perform much more than a return rc after calling MPI_Finalize.
 *
 * Here only the master process, info.id = 0, survives finalize.
 */

int M6378_Finalize() {
  sem_wait(info.mutex0);
    if( info.rel_number == 0 )
      info.rel_number = ++(info.sshm_hinfo->rel_count);
  sem_post(info.mutex0);
  release( info.rel_number == info.np );
  if( info.id != 0 ) 
    exit(0);
  return( M6378_SUCCESS );
}

static
int remapWhenNecessary( struct _Sshm_HInfo *hinfo ) {
  if( info.dshm_size == hinfo->dshm_size ) 
    return(0);

#ifdef TELL_REMAP
fprintf( stderr, "remap (%d)\n", info.id );
fflush(stderr);
#endif

  /* Wish I could use mremap. Not supported by OSX though. */
  if( 0 > munmap( info.dshm, info.dshm_size ) ) {
    fprintf( stderr, "remapWhenNecessary munmap error." );
    fflush(stderr);
    return(1);
  }
  close( info.dshm_fd );

  info.dshm_fd = shm_open( dshm_name, O_RDWR, S_IRUSR|S_IWUSR );
  info.dshm_size = hinfo->dshm_size;
  info.dshm = mmap( NULL, info.dshm_size,
                    PROT_READ|PROT_WRITE, MAP_SHARED, info.dshm_fd, 0 );
  if( info.dshm == MAP_FAILED ) {
    fprintf( stderr, "remapWhenNecessary mmap error." );
    fflush(stderr);
    return(2);
  }

#ifdef TELL_REMAP
fprintf( stderr, "remap (%d) done\n", info.id );
fflush(stderr);
#endif

  return(0);
}

static
int resizeWhenNecessary( struct _Sshm_HInfo *hinfo,
                         struct _Sshm_TInfo *tinfo,
                         size_t newsize ) {
                         /* newsize must be mult of wordsize. */
  size_t newnext, save_size;
  char *save;

  if( newsize <= tinfo->dbuf_size )
    return(0);

  save = (char*)0; save_size = 0;

  /* 20% defrag threshold. */
  if( hinfo->dshm_frag > 0.20*hinfo->dshm_size ) {
    int i, j, next = 0;

#ifdef TELL_DEFRAG
fprintf( stderr, "Defragging suggested (frag/size=%d/%d=%g%%)\n",
  hinfo->dshm_frag, hinfo->dshm_size, 
  100*((float)hinfo->dshm_frag)/hinfo->dshm_size );
fflush(stderr);
#endif

    /* Don't need to save contents of tinfo->dbuf... */
    save_size = hinfo->dshm_next - hinfo->dshm_frag - tinfo->dbuf_size;
    tinfo->dbuf_size = tinfo->dbuf_loc = (size_t)0;

    /* info.dshm and info.dshm_size may be stale. */
    hinfo->dshm_size = MAX( hinfo->dshm_size, save_size + newsize );
    remapWhenNecessary( hinfo );
    save = (char*)malloc( save_size );
    if( !save ) {
      fprintf( stderr, "resizeWhenNecessary malloc error." );
      fflush(stderr);
      return(1);
    }
    for( j = 0; j < info.np; j++ )
    for( i = 0; i < info.np; i++ ) {
      struct _Sshm_TInfo *ti = &info.sshm_tinfo[i+j*info.np];
      if( i == j ) continue;
      if( ti->dbuf_size > 0 ) {
        memcpy( save+next, info.dshm+ti->dbuf_loc, ti->dbuf_size );
        ti->dbuf_loc = next;
        next += ti->dbuf_size;
      }
    }    
    hinfo->dshm_next = next; /* Should equal save_size. */
    hinfo->dshm_frag = 0;

#ifdef TELL_DEFRAG
fprintf( stderr, "Defragging completed.\n" );
fflush(stderr);
#endif

  }

  newnext = hinfo->dshm_next + newsize;

  if( newnext > hinfo->dshm_size ) {
    int newdshm_size = pageAlignedSizeof(2*newnext);

    /* Need all these extra steps to overcome OSX ftruncate issues. */
    if( !save ) {
      remapWhenNecessary( hinfo );
      save_size = hinfo->dshm_next;
      save = malloc( save_size );
      if( !save ) {
        fprintf( stderr, "resizeWhenNecessary malloc error." );
        fflush(stderr);
        return(2);
      }
      memcpy( save, info.dshm, save_size );
    }

#ifdef TELL_RESIZE
fprintf( stderr, "resizing (%d) from %d to %d\n", info.id, info.dshm_size, newdshm_size );
fflush( stderr );
#endif

    if( 0 > munmap( info.dshm, info.dshm_size ) ) {
      fprintf( stderr, "resizeWhenNecessary munmap error." );
      fflush(stderr);
      return(3);
    }
    close( info.dshm_fd );
    shm_unlink( dshm_name );

    info.dshm_fd = shm_open( dshm_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR );
    if( info.dshm_fd == -1 ) {
      fprintf(stderr, "resizeWhenNecessary shm_open error.\n" );
      fflush(stderr);
      return(4);
    }

    info.dshm_size = hinfo->dshm_size = newdshm_size;
    if( 0 > ftruncate( info.dshm_fd, info.dshm_size ) ) {
      fprintf(stderr, "resizeWhenNecessary ftruncate error.\n" );
      fflush(stderr);
      return(5);
    }

    info.dshm = mmap( NULL, info.dshm_size,
                      PROT_READ|PROT_WRITE, MAP_SHARED, info.dshm_fd, 0 );
    if( info.dshm == MAP_FAILED ) {
      fprintf( stderr, "resizeWhenNecessary mmap error." );
      fflush(stderr);
      return(6);
    }

#ifdef TELL_RESIZE
fprintf( stderr, "resizing (%d) done\n", info.id );
fflush(stderr);
#endif

  }

  if( save ) {
    memcpy( info.dshm, save, save_size );
    free(save); save = (char*)0; save_size = 0;
  }

  hinfo->dshm_frag += tinfo->dbuf_size;
  tinfo->dbuf_size = newsize;
  tinfo->dbuf_loc  = hinfo->dshm_next;
  hinfo->dshm_next = newnext;

  return(0);
}

void *M6378_Sendstart( int to, int ndata ) {
  struct _Sshm_HInfo *hinfo =  info.sshm_hinfo;
  struct _Sshm_TInfo *tinfo = &info.sshm_tinfo[info.id+to*info.np];
  struct _MsgTemplate *mtemp;

  if( to < 0 || to >= info.np || to == info.id ) {
    fprintf( stderr, "M6378_Send* called with id (%d) out of range.\n", to );
    abort_PostInit(SIGINT);
  }

  SEM_WAIT(&(tinfo->wok));

    SEM_WAIT(&(hinfo->dshm_make));
      if( 0 != resizeWhenNecessary( hinfo, tinfo, 
                             wordAlignedSizeof(info.wordsize+ndata) ) ) {
        fprintf( stderr, "M6378_Send* fatal error.\n" );
        abort_PostInit(SIGINT);
      }        
    SEM_POST(&(hinfo->dshm_make));

    SEM_WAIT(&(hinfo->dshm_mutx));
      hinfo->dshm_users++;
      if( hinfo->dshm_users == 1 )
        SEM_WAIT(&(hinfo->dshm_make));
    SEM_POST(&(hinfo->dshm_mutx));

    if( 0 != remapWhenNecessary( hinfo ) ) {
      fprintf( stderr, "M6378_Send* fatal error.\n" );
      abort_PostInit(SIGINT);
    }        

    mtemp = (struct _MsgTemplate*)(info.dshm + tinfo->dbuf_loc); 
    mtemp->ndata = ndata;
    /* Continue msg sending below... */

  return( (void*)&mtemp->data );
}

int M6378_Sendfinish( int to ) {
  struct _Sshm_HInfo *hinfo =  info.sshm_hinfo;
  struct _Sshm_TInfo *tinfo = &info.sshm_tinfo[info.id+to*info.np];

  if( to < 0 || to >= info.np || to == info.id ) {
    fprintf( stderr, "M6378_Send* called with id (%d) out of range.\n", to );
    abort_PostInit(SIGINT);
  }

    /*... complete msg sending above. */
    SEM_WAIT(&(hinfo->dshm_mutx));
      hinfo->dshm_users--;
      if( hinfo->dshm_users == 0 )
        SEM_POST(&(hinfo->dshm_make));
    SEM_POST(&(hinfo->dshm_mutx));

  SEM_POST(&(tinfo->rok));

  return( M6378_SUCCESS );
}

int M6378_Send( int to, void *data, int ndata ) {
  void *bufdata = M6378_Sendstart( to, ndata );
  memcpy( bufdata, data, ndata );
  M6378_Sendfinish( to );
  return( M6378_SUCCESS );
}

void *M6378_Recvstart( int fr, int *ndata ) {
  struct _Sshm_HInfo *hinfo =  info.sshm_hinfo;
  struct _Sshm_TInfo *tinfo = &info.sshm_tinfo[fr+info.id*info.np];
  struct _MsgTemplate *mtemp;

  if( fr < 0 || fr >= info.np || fr == info.id ) {
    fprintf( stderr, "M6378_Recv* called with id (%d) out of range.\n", fr );
    abort_PostInit(SIGINT);
  }

  SEM_WAIT(&(tinfo->rok));

    SEM_WAIT(&(hinfo->dshm_mutx));
      hinfo->dshm_users++;
      if( hinfo->dshm_users == 1 )
        SEM_WAIT(&(hinfo->dshm_make));
    SEM_POST(&(hinfo->dshm_mutx));

    if( 0 != remapWhenNecessary( hinfo ) ) {
      fprintf( stderr, "M6378_Recv* fatal error.\n" );
      abort_PostInit(SIGINT);
    }        
    mtemp = (struct _MsgTemplate*)(info.dshm + tinfo->dbuf_loc); 

    if( ndata )
      *ndata = mtemp->ndata;
    /* Continue msg receiving below... */

  return( (void*)&mtemp->data );
}

int M6378_Recvfinish( int fr ) {
  struct _Sshm_HInfo *hinfo =  info.sshm_hinfo;
  struct _Sshm_TInfo *tinfo = &info.sshm_tinfo[fr+info.id*info.np];

  if( fr < 0 || fr >= info.np || fr == info.id ) {
    fprintf( stderr, "M6378_Recv* called with id (%d) out of range.\n", fr );
    abort_PostInit(SIGINT);
  }

    /*... complete msg receiving above. */
    SEM_WAIT(&(hinfo->dshm_mutx));
      hinfo->dshm_users--;
      if( hinfo->dshm_users == 0 )
        SEM_POST(&(hinfo->dshm_make));
    SEM_POST(&(hinfo->dshm_mutx));

  SEM_POST(&(tinfo->wok));

  return( M6378_SUCCESS );
}

int M6378_Recv( int fr, void *data, int *ndata ) {
  int newndata;
  void *bufdata = M6378_Recvstart( fr, &newndata );
  memcpy( data, bufdata, newndata );
  M6378_Recvfinish( fr );
  if( ndata )
    *ndata = newndata;
  return( M6378_SUCCESS );
}
