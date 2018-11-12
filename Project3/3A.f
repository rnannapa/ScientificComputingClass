c Looks like g77 and gcc no longer use the same linker (my guess).
c Use gfortran instead.
c Compile
c > gcc -c recip.c
c Produces recip.o
c Let gfortran compile this and link to recip.o
c > gfortran test_recip.f recip.o
c Produces a.out.

       implicit none
       real a, osqrt

       a = +4
c       a = +4

       print*, sqrt(a), ' =? ', osqrt(a)
       end
