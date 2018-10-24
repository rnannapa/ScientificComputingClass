#define RECIP recip_

#define EZERO 0x3f800000 /* 127<<23 */
#define EMASK 0x7f800000 /* 255<<23 */

float RECIP( float *fp ) {
  float a = *fp, scale, x;
  unsigned int *ia     = (unsigned int*)fp,
               *iscale = (unsigned int*)&scale;

  *iscale = 2*EZERO - (*ia & EMASK);
  a = a*scale;
  x = 0.5;
  x = x*(2.0-a*x);
  x = x*(2.0-a*x);
  x = x*(2.0-a*x);
  x = x*(2.0-a*x);
  x = x*(2.0-a*x);
  return( x*scale );
}