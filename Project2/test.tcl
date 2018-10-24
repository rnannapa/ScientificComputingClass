#!/usr/bin/tclsh

#export TCLLIBPATH=./
#package require Tclbmath

source tclbmath.tcl


# if {$argc != 1} {
#   puts stderr "Usage: $argv0 size"
#   exit 1
# }
# set size [lindex $argv 0]

set size 10

set pgm [::bmath::compile \
  { Ba } { {0 n 0} {1 n 1} {2 n 2} {4 n 4} \
  {pi f 3.14159265} {i1 n 0} {i2 n $size} {i3 f 1.0} i tmp th dth l ba } \
  {
     PADD $ba  $Ba  $0
     LEI  $l   $i2  $0
     CJMP $l   $L_Zero
     MFFI $dth $pi $2
     DFFI $dth $dth $i2
     %L_Again
     	MFFI $th $dth $i1
     	COS  $tmp $th $i3
     	MOV  $ba $tmp  3
     	PADD $ba $ba $4
     	IADD $i1 $i1 $1
     	LEI $l $i1 $i2
     	CJMP $l $L_Again
     %L_Done
   %L_Zero
  }]



set barray [binary format x[expr 4*($size+1)] 0]
#puts [time {
  ::bmath::run $pgm barray
#} 100]

binary scan $barray f* vlst
set i 0
foreach {x} $vlst {
  puts [format "xy\[%d\] = (%f)" $i $x]
  incr i
}
unset barray
exit 0
