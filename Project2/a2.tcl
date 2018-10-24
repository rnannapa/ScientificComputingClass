#!/usr/bin/tclsh

#export TCLLIBPATH=./
#package require Tclbmath

source tclbmath.tcl


if {$argc != 1} {
  puts stderr "Usage: $argv0 size"
  exit 1
}
set size [lindex $argv 0]

set pgm [::bmath::compile \
  { Bs Ba } { {0 n 0} {1 n 1} {2 n 2} {4 n 4} \
  {pi f 3.14159265} {i1 n 0} i2 {i3 f 1.0} i tmp th dth l ba } \
  {
  	  MOV  $i2  $Bs 0 
      PADD $ba  $Ba  $0
      LTI  $l   $i2  $0
      CJMP $l   $L_Zero
      MFFI $dth $pi $2
      EQI  $l $i2 $0
      CJMP $l $L_Again
      DFFI $dth $dth $i2
    %L_Again
      MFFI $th $dth $i1
      COS  $tmp $th $i3
      MOV  $ba $tmp  3
      PADD $ba $ba $4
      SIN  $tmp $th $i3
      MOV  $ba $tmp  3
      PADD $ba $ba $4
      IADD $i1 $i1 $1
      LEI $l $i1 $i2
      CJMP $l $L_Again
    %L_Done
   %L_Zero
  }]

set barray [binary format x[expr max(4*2*($size+1),0)] 0]

set bsize  [binary format n $size]

#puts [time {
  ::bmath::run $pgm bsize barray
#} 100]

binary scan $barray f* vlst
set i 0
foreach {x y} $vlst {
  puts [format "xy\[%d\] = (%f,%f)" $i $x $y]
  incr i
}
unset barray
exit 0
