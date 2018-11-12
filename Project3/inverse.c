#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#define FACTOR factor_
float FACTOR( float*, int*, int* );

#define SOLVE solve_
float SOLVE( float *x, float *matrix, int*, float*, int* );



int main(int argc, char *argv[]) {
  
  int n, i, j, k;
  float *matrix, *inverse;
  float fac;
  n = round(sqrt(argc-1));
  float* b = calloc(n, sizeof(float));
  int* pivot = calloc(n, sizeof(int));

  // printf("Size of matrix:%dx%d\n", n,n );

  if( n == 0 || argc-1 != n*n ) {
    fprintf( stderr, "Usage: %s square-matrix\n", argv[0] );
    return(1);
  }

  matrix = malloc( n*n*sizeof(float) );
  inverse = malloc( n*n*sizeof(float) );
  
  if( !matrix ) {
    fprintf( stderr, "Memory allocation error.\n" );
    return(2);
  }
  
  /* Read Matrix from command prompt*/
  k = 1;
  for( i = 0; i < n; i++ ) {
    for( j = 0; j < n; j++ ) {
      matrix[i+j*n] = atof( argv[k++] );
    }
  }
  
  /* Display Matrix */

  for( i = 0; i < n; i++ ) {
    for( j = 0; j < n; j++ ) {
      printf( "\t%f", matrix[i+j*n] );
    }
    printf( "\n" );
  }

  fac = FACTOR( matrix, pivot, &n );
  // printf( "fac = %f\n", fac );

  for (i = 0; i<n; i++){  
    for(j = 0; j<n; j++){
      if(i==j){
        b[j] = 1.0;
      }
      if(i!=j){
        b[j] = 0.0;
      }
    }
    SOLVE( &inverse[i*n], matrix, pivot, b, &n );
  }

  printf("Inverse of matrix:\n");

  for (i=0; i < n; i++){
    for (j=0; j < n; j++){
      printf("\t %f\t",inverse[i+j*n]);
    }
    printf("\n");
  }
  free(matrix); free(inverse); free(b); free(pivot);
}
