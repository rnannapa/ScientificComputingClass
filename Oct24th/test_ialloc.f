       real*8 a(0:1)
       integer*8 ioff, ialloc

       ioff = ialloc( a, 100*8 )/8
       if( ioff .eq. 0 ) then
         print*,'Memory allocation failed.'
         stop
       endif

c       a(ioff+0) = 0.0
c       a(ioff+1) = 1.0
c       a(ioff+8) = 9.0

c       print*, ioff, a(ioff+0), a(ioff+1), a(ioff+9)
      call set(3,3,a)

      do i=0,8
        print*,i,a(i)
      enddo


       end
      
      subroutine set (m,n,a)
      integer m n
      real*8 a(m,1)


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




