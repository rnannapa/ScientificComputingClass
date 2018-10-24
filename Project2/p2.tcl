#!/usr/bin/tclsh

#export TCLLIBPATH=./
#package require Tclbmath

source tclbmath.tcl

set size 1


set pgm [::bmath::compile \
  { Ba } { {0 n 0} {1 n 1} {4 n 4} \
  {2 n 2} {pi f 3.14159265} {L f 1.0} {i1 n 1} {i2 n $size} i tmp th dth ba } \
  {
      PADD $ba $Ba $0 
      LEI  $i  $i2 0
      CJMP $i $L_Zero
      MFFI $dth $pi $2
      DFFI $dth $dth $i2
    %L_Again
      MFFI $th $dth $i1 
      COS  $tmp $th $L
      MOV  $ba $tmp 3   
      IADD $i1 $i1 $1  
      PADD $ba $ba $4  
      LEI  $i  $i1 $i2 
      CJMP $i $L_Again  # if above is true go back to start of loop
    %L_Done
    %L_Zero
  }]

set barray [binary format x[expr 4*$size] 0]
#puts [time {
  ::bmath::run $pgm barray
#} 100]

binary scan $barray n* vlst
puts $vlst

