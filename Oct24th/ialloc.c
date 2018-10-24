#include <stdlib.h>
#include <stdint.h>


#define IALLOC ialloc_  /*_ is appended by the fortran compiler*/

int64_t IALLOC( char *base, int *size ) {
  char *ptr;
  ptr = malloc( *size );
  if( !ptr ) return(0);
  return(ptr-base);
}
