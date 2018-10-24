#!/usr/bin/tclsh

#export TCLLIBPATH=./
#package require Tclbmath

source tclbmath.tcl

set size 50

set pgm [::bmath::compile \
  { Ba } { {0 n 0} {1 n 1} {4 n 4} {i1 n 1} {i2 n $size} l ba } \
  {
      PADD $ba $Ba $0  #increments pointers
      LTI  $l  $i2 $i1
      CJMP $l $L_Done
    %L_Again
      MOV  $ba $i1 0
      IADD $i1 $i1 $1
      PADD $ba $ba $4
      LEI  $l  $i1 $i2
      CJMP $l $L_Again
    %L_Done
  }]

set barray [binary format x[expr 4*$size] 0]
puts [time {
  ::bmath::run $pgm barray
} 100]

binary scan $barray n* vlst
puts $vlst

puts [time {
  set vlst {}
  for {set i 1} {$i <= $size} {incr i} {
    lappend vlst $i
  }
} 100]

puts $vlst
