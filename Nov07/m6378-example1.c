/* Compile:
 * > gcc m6378-example1.c -lm6378
 * Run: (any number of processes > 1 will be ok)
 * > m6378run 4 a.out
 */

#include <m6378.h>
#include <stdio.h>

int modulo (int i, int n) {
  /* In c, % is actually the remainder operator: i%n := i-(i/n)n.
   * This is not the mathematical modulo when i < 0. The code below
   * however returns the correct modulo for all i assuming n > 0. */
  int mod = i % n;
  if( mod < 0 )
    mod += n;
  return(mod);
}


int main( int argc, char *argv[] ) {
  int np, myid, id;
  int imsg;

  M6378_Init( &argc, &argv ); /*mpi init> initilization*/
  M6378_Comm_size( &np ); /*# of processors */
  M6378_Comm_rank( &myid ); /* which processor am I, processor ID*/

  if( np < 2 ) {
    if( myid == 0 )
      fprintf( stderr, "Number of processes must be larger than 1.\n" );
    M6378_Abort(1); /*like mpi_abort */
  }

  /* Master, i.e. myid == 0, broadcasts to all slaves, i.e. myid > 0. */
  if( myid == 0 ) {
    for( id = 1; id < np; id++ ) {
      imsg = 0;
      M6378_Send( id, &imsg, sizeof(int) );
    }
  } else {
    imsg = 1111; /* Just to make sure we've read imsg. */
    M6378_Recv( 0, &imsg, NULL );
    printf( "Process %d recv imsg=%d from master.\n", myid, imsg );
  }

  /* Master gathers from all slaves. */
  if( myid == 0 ) {
    for( id = 1; id < np; id++ ) {
      M6378_Recv( id, &imsg, NULL );
      printf( "Master recv imsg=%d from process %d.\n", imsg, id );
    }
  } else {
    imsg = myid;
    M6378_Send( 0, &imsg, sizeof(int) );
  }

  /* Each process will wait here until all have reached the barrier. */
  M6378_Barrier();

  /* Below, think of all processes as equals, no master, no slaves.
   * Each sends an int message to the process on its right modulo np, 
   * i.e. to_id = (myid+1) mod np, and each receives the message from
   * the process on its left modulo np, i.e. from_id = (myid-1) mod np.
   */
  id = modulo( myid+1, np );
  imsg = myid;
  M6378_Send( id, &imsg, sizeof(int) );
  printf( "%d sends imsg=%d to %d\n",   myid, imsg, id );  

  id = modulo( myid-1, np );
  M6378_Recv( id, &imsg, NULL );
  printf( "%d recvs imsg=%d from %d\n", myid, imsg, id );  

  M6378_Finalize();
  return(0);
}
