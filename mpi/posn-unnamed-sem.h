
#define SEM_T ushort

static int posn_key = -1;
static ushort posn_maxnum = 0;
static ushort posn_semcnt = 0;
static sem_t **posn_local_addrs = (sem_t**)0;
static char posn_semname[256];

#define SEM_UNNAMED_OPEN(key,maxnum) \
  ( ( posn_key = key,                \
      posn_maxnum = maxnum,          \
      posn_local_addrs = (sem_t**)calloc( maxnum, sizeof(sem_t*) ) \
     ) != (sem_t**)0 ? 1 : -1 )

#define SEM_UNNAMED_ATTACH(key,maxnum) SEM_UNNAMED_OPEN(key,maxnum) 

static
void SEM_UNNAMED_CLOSE() {
  if( posn_local_addrs ) {
    int i;
    for( i = 0; i < posn_maxnum; i++ ) {
      if( posn_local_addrs[i] )
        sem_close(posn_local_addrs[i]);
    }
    free(posn_local_addrs);
    posn_local_addrs = (sem_t**)0;
  }  
  posn_key = -1;
  posn_maxnum = 0;
  posn_semcnt = 0;
}

static
void SEM_UNNAMED_UNLINK() {
  int i;
  for( i = 1; i <= posn_maxnum; i++ ) {
    sprintf( posn_semname, "/posn_unnamed-sem-%d-%05d", posn_key, i );
    sem_unlink( posn_semname );
  }
}

#define SEM_INIT(Sp,i,ui)    \
  ( *(Sp) = 1+posn_semcnt++, \
    ( posn_SEM2sem(*(Sp),ui) ? 1 : -1 ) )

#define SEM_POST(Sp) \
  sem_post( posn_SEM2sem(*(Sp),-1) )

#define SEM_WAIT(Sp) \
  sem_wait( posn_SEM2sem(*(Sp),-1) )

#define SEM_DESTROY(Sp) (1)

//#define SEM_GETVALUE(Sp,ip) \
//  sem_getvalue( posn_SEM2sem(*(Sp),-1), ip )

/*****************************************************************************/

static
sem_t *posn_SEM2sem( SEM_T id, int ival ) {
  sem_t *sp;          /* id >= 1, ival = -1 do not initialize. */
  if( !posn_local_addrs || id < 1 || id > posn_maxnum )
    return( (sem_t*)0 );
  sp = posn_local_addrs[id-1];
  if( !sp ) {
    sprintf( posn_semname, "/posn_unnamed-sem-%d-%05d", posn_key, id );
    if( ival == -1 )
      sp = sem_open( posn_semname, O_CREAT, S_IRUSR|S_IWUSR );
    else
      sp = sem_open( posn_semname, O_CREAT, S_IRUSR|S_IWUSR, ival );
    if( !sp ) return( (sem_t*)0 );
    posn_local_addrs[id-1] = sp;
  }
  return( sp );
}

