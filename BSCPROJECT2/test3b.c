#include <stdio.h>

#define FACTOR factor_
float FACTOR( float*, int*, int* );

#define SOLVE solve_
float SOLVE( float *x, float *matrix, int*, float*, int* );



int main() {
  float matrix[4] = {1.0, 2.0, 2.0, 1.0}, inverse[4], b[2] = {0.0, 0.0};
  int   pivot[2] = {0,0};  
  int   n = 2;
  float fac;

  fac = FACTOR( matrix, pivot, &n );

  printf( "fac = %f\n", fac );

  printf( "pivot = %d %d\n", pivot[0], pivot[1] );

  // printf( "row 1, %f\t %f\n", matrix[0], matrix[2] );
  // printf( "row 2, %f\t %f\n", matrix[1], matrix[3] );

  b[0] = 1.0; b[1] = 0.0;
  SOLVE( &inverse[0], matrix, pivot, b, &n );
  printf("B:%f\n", *b);

  b[0] = 0.0; b[1] = 1.0;
  SOLVE( &inverse[2], matrix, pivot, b, &n );
  printf("B:%f\n", *b);
  
  printf( "row 1, %f\t %f\n", inverse[0], inverse[2] );
  printf( "row 2, %f\t %f\n", inverse[1], inverse[3] );

}


// for( j = 0; j < n; j++ ) {
//     for( i = 0; i < n; i++ ) {
//       printf( "\t%f", inverse[i+j*n] );
//     }
//     printf( "\n" );
//   }
// }

// for (i=1;i<=n;i++){
//     for (j=1;j<=n;j++){
//       if (i == j){
//         b[i+j*n] = 1.0;
//       }
//       if (i != j){
//         b[i+j*n] = 0.0;
//       }
//       // printf("\t%f", b[i+j*n]);
//     }
//     printf("\n b for inverse calculation %f\n", *b);
//     SOLVE( &inverse[i*n], matrix, pivot, b, &n );
//     // printf("Inverse %f\n",inverse[i*n]);
// }


  // for( i = 0; i < n; i++ ) {
  //   for( j = 0; j < n; j++ ) {
  //     printf( "\t%f", inverse[i+j*n] );
  //   }
  //   printf( "\n" );
  // }