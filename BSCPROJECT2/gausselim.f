cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

        real function factor( a, pivot, n )
        integer n, pivot(n)
        real    a(n,n)
	integer pr, pc, row, col
	real    maxpv, val
	integer maxpr, tmp
	real    multp
	integer k
        real    p, pmin, pmax
	do row = 1, n
	  pivot(row) = row
	enddo
	do col = 1, n-1
	  maxpr = col
	  maxpv = abs( a(pivot(col),col) )
	  do row = col+1, n
	    val = abs( a(pivot(row),col) )
	    if( val .gt. maxpv ) then
	      maxpr = row
	      maxpv = val
	    endif
	  enddo
	  if( maxpv .eq. 0.0 ) then
	    factor = 0.0
	    return
	  endif
	  tmp = pivot(col)
	  pivot(col) = pivot(maxpr)
	  pivot(maxpr) = tmp
	  pc = pivot(col)
	  do row = col+1, n
	    pr = pivot(row)
	    multp = a(pr,col)/a(pc,col) 
	    a(pr,col) = multp
	    do k = col+1, n
	      a(pr,k) = a(pr,k)-multp*a(pc,k)
            enddo
	  enddo
	enddo 
	if( a(pivot(n),n) .eq. 0.0 ) then
	  factor = 0.0
	else
          pmin = abs(a(pivot(1),1))
          pmax = pmin
          do k = 2, n
            p = abs(a(pivot(k),k))
            if( p .lt. pmin ) pmin = p
            if( p .gt. pmax ) pmax = p
          enddo
	  factor = pmin/pmax
	endif
	return
	end

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

	subroutine solve( x, a, pivot, b, n )
	integer n, pivot(n)
	real    x(n), a(n,n), b(n)
	integer pr, pc, row, col
	do col = 1, n-1
	  pc = pivot(col)
	  do row = col+1, n
	    pr = pivot(row)
	    b(pr) = b(pr)-a(pr,col)*b(pc)
	  enddo
	enddo
	do row = n, 1, -1
	  x(row) = 0.0
	  pr = pivot(row)
	  do col = row+1, n
	    x(row) = x(row)+a(pr,col)*x(col)
 	  enddo
	  x(row) = (b(pr)-x(row))/a(pr,row)
	enddo
	end