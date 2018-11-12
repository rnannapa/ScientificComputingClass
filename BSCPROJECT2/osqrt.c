#include<math.h>
#include<stdlib.h>
#include<stdio.h>

#define OSQRT osqrt_

#define EZERO 0x3f800000 /* 127<<23 */
#define EMASK 0x7f800000 /* 255<<23 */


float OSQRT( float *fp ) {
  float a = *fp, scale, x;
  unsigned int *ia     = (unsigned int*)&a,
               *iscale = (unsigned int*)&scale;
  
  // *iscale = 2*EZERO - ((((*ia)/2) + EZERO/2)&EMASK);
  
  *iscale = 2*EZERO - (((*ia + EZERO)>>1) & EMASK);
  
  if (a<=0.0) {
    if (a==0.0){
      return(INFINITY);
    }else{
      return(NAN);
    }
  }

  a = a*scale*scale;
  
  x = 0.5;
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  return( x*scale );
}


