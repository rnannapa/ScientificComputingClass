#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef MAX
# define MAX(a,b) ( (a)>(b) ? (a) : (b) )
#endif

void pgm( int *size, float *array ) {
  float pi=3.14159265;
  float dtheta, theta;
  int i;
  float *ptr = array;
  /* Don't divide by 0. */
  dtheta = 2*pi;
  if( *size != 0 ) dtheta /= *size;
  for( i = 0; i <= *size; i++ ) {
    theta = i*dtheta;
    *ptr++ = cos(theta);
    *ptr++ = sin(theta);
  } 
}

int main( int argc, char *argv[] ) {
  float *array;
  int size, i;
  if( argc != 2 ) {
    fprintf( stderr, "Usage: %s size\n", argv[0] );
    return(1);
  }
  size = atoi( argv[1] );
  array = malloc( MAX( (size+1)*2*4, 0) );
  if( !array ) {
    fprintf( stderr, "Memory allocation error.\n" );
    return(2);
  }

  pgm( &size, array );

  for( i = 0; i <= size; i++ ) {
    printf( "xy[%d] = (%f,%f)\n", i, array[2*i], array[2*i+1] );
  }
  free( array );
  return(0);
}
