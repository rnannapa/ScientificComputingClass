c Compile only
c > cc -c ialloc.c 
C Produces ialloc.o.
c Let gfortran compile this and link to ialloc.o
c > gfortran test_ialloc.f ialloc.o
c Produces a.out.

c Fixed two bugs (see bug below) from class here.


       real*8 a(0:0)
       integer*8 ioff, ialloc

       ioff = ialloc( a, 9*8 )/8
       if( ioff .eq. 0 ) then
         print*,'Memory allocation failed.'
         stop
       endif

c       a(ioff+0) = 0.0
c       a(ioff+1) = 1.0
c       a(ioff+8) = 9.0

c       print*, ioff, a(ioff+0), a(ioff+1), a(ioff+9)

c had bug here earlier.
       call set(3,3,a(ioff))

       do i=0,8
c had bug here earlier.
         print*,i,a(ioff+i)
       enddo

       end



       subroutine set(m,n,a)
       integer m,n
       real*8 a(m,n)

       a(1,1) = 1
       a(2,1) = 2
       a(3,1) = 3

       a(1,2) = 11
       a(2,2) = 12
       a(3,2) = 13

       a(1,3) = 21
       a(2,3) = 22
       a(3,3) = 23
 
       return
       end

