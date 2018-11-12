/*
 * acer-ax3400> ipcs -a
 * acer-ax3400> ipcrm -S 5932
 */

#include <sys/sem.h>
#include <fcntl.h>

#define SEM_T ushort

static key_t sys5_key;
static int sys5_semid = -1;
static ushort sys5_semcnt = 0;
static struct sembuf sys5_sempost = {0, +1, 0};
static struct sembuf sys5_semwait = {0, -1, 0};

/* TODO check that SEM_NUMBER_MAX does not exceed system limit.*/
#define SEM_UNNAMED_OPEN(key,maxnum)  \
  ( sys5_key = key, sys5_semid = semget( sys5_key, maxnum, IPC_CREAT|S_IRUSR|S_IWUSR ) )

/* TODO check that SEM_NUMBER_MAX does not exceed system limit.*/
#define SEM_UNNAMED_ATTACH(key,maxnum) \
  ( sys5_key = key, sys5_semid = semget( sys5_key, maxnum, S_IRUSR|S_IWUSR ) )

#define SEM_UNNAMED_CLOSE()  (1)

#define SEM_UNNAMED_UNLINK() semctl( sys5_semid, 0, IPC_RMID )

#define SEM_INIT(Sp,i,ui)  \
  ( *(Sp) = sys5_semcnt+1, semctl( sys5_semid, sys5_semcnt++, SETVAL, ui ) )

#define SEM_POST(Sp) \
  ( sys5_sempost.sem_num = *(Sp)-1, semop( sys5_semid, &sys5_sempost, 1 ) )

#define SEM_WAIT(Sp) \
  ( sys5_semwait.sem_num = *(Sp)-1, semop( sys5_semid, &sys5_semwait, 1 ) )

#define SEM_DESTROY(Sp)      (1)

//#define SEM_GETVALUE(Sp,ip)  \
//    semctl( sys5_semid, *(Sp)-1, GETVAL, ip )
