#!/usr/bin/tclsh

# export TCLLIBPATH=./
#package require Tclbmath

source tclbmath.tcl

set size 10

set pgm [::bmath::compile \
  { Bp Ba } { {0 n 0} {1 n 1} {4 n 4} {i1 n 1} {pi f [expr 4*atan(1)]} i2 lg ba } \
  {
      PADD $ba $Ba $0 # please set the address regester of "ba" to address regester of "Ba"
      MOV  $i2 $Bp 0 # 0 changes int to int
      LTI  $lg $i2 $i1
      CJMP $lg $L_Done
    %L_Again
#      MOV  $ba $i1 0
      MOV  $ba $pi 3 # 3 FLOAT TO FLOAT 
      IADD $i1 $i1 $1
      PADD $ba $ba $4 # increament ba
      LEI  $lg $i1 $i2 #LEI lessthan or equal (I integer) 
      CJMP $lg $L_Again # L_Again can be changed to anything you need
    %L_Done
  }]



set barray [binary format x[expr 4*$size] 0]
set bpar   [binary format n $size]




puts [time {
  ::bmath::run $pgm bpar barray
} 100]




binary scan $barray f* vlst
puts $vlst
















exit

puts [time {
  set vlst {}
  for {set i 1} {$i <= $size} {incr i} {
    lappend vlst $i
  }
} 100]

puts $vlst
