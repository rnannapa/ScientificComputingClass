#!/usr/bin/tclsh

#export TCLLIBPATH=./
#package require Tclbmath

source tclbmath.tcl

set size 0


set pgm [::bmath::compile \
  { Ba } { { 0 n 0 } { 1 n 1 } { 4 n 4 } { i1 n 1 } { i2 n $size } \
  l la ba } \
  {
      PADD $ba $Ba $0  #increments pointers
      LEI  $l  $i2 0 # l = i2 < i1
      CJMP $l $L_Zero
      # LTI  $l $i1 $i2
      # CJMP $l $L_Done  # if i2 < i1 go to end
    %L_Again
      MOV  $ba $i1 0   # int (ba) = int(i1) without change in sign
      IADD $i1 $i1 $1  # integer addition
      PADD $ba $ba $4  # add 4 to pointer (pointer arth)
      LEI  $l  $i1 $i2  # $l = i1 <=i2
      CJMP $l $L_Again  # if above is true go back to start of loop
    %L_Done
    %L_Zero
  }]

set barray [binary format x[expr 4*$size] 0]
#puts [time {
  ::bmath::run $pgm barray
#} 100]

binary scan $barray n* vlst
puts $vlst

