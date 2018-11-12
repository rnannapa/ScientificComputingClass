c Looks like g77 and gcc no longer use the same linker (my guess).
c Use gfortran instead.
c Compile
c > gcc -c recip.c
c Produces recip.o
c Let gfortran compile this and link to recip.o
c > gfortran test_recip.f recip.o
c Produces a.out.

       implicit none
       real a,b,c,d,e, osqrt

       a = 1023
       b = -1
       c = 1.45
       d = 0.2445
       e = -0
c       print*, 1.0/sqrt(a), ' =? ', osqrt(a)
       print*, sqrt(a), ' =? ', 1.0/osqrt(a)
c       print*, 1.0/sqrt(b), ' =? ', osqrt(b)
       print*, sqrt(b), ' =? ', 1.0/osqrt(b)
c       print*, 1.0/sqrt(c), ' =? ', osqrt(c)
       print*, sqrt(c), ' =? ', 1.0/osqrt(c)
c       print*, 1.0/sqrt(d), ' =? ', osqrt(d)
       print*, sqrt(d), ' =? ', 1.0/osqrt(d)
c       print*, 1.0/sqrt(e), ' =? ', osqrt(e)
       print*, sqrt(e), ' =? ', 1.0/osqrt(e)
       end