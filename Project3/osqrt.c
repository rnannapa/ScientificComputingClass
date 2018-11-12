#define OSQRT osqrt_

#define EZERO 0x3f800000 /* 127<<23 */
#define EMASK 0x7f800000 /* 255<<23 */
/* #define SMASK 0x80000000 /* 1<<31 */

float OSQRT( float *fp ) {
  float a = *fp, scale, x;
  
  if(a<=0.0){
    if(a == 0.0){
      return( INFINITY );
      else
        return( NAN );
    }
  }
  
  unsigned int *ia     = (unsigned int*)&a,
               *iscale = (unsigned int*)&scale;
  *iscale = (*ia & 0x80000000) | 2*EZERO - (*ia & EMASK);
  a = a*scale*scale;
  x = 0.5;
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  x = 0.5*x*(3.0-a*x*x);
  return( x*scale*scale);
}