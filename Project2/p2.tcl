# !/usr/bin/tclsh
# export TCLLIBPATH=./
# package require Tclbmath

# source tclbmath.tcl

# set size 10

# set pgm [::bmath::compile\
#   { Ba } { {0 n 0} {1 n 1} {2 n 2} {4 n 4} {pi f 3.14159265} {One f 1.0} {i1 n 1} {i2 n $size} l tmp th dth ba } \
#   {
#   	  PADD $ba $Ba $0 
#       LTI  $l  $i2 $i1
#       CJMP $l $L_Done 
#     %L_Again
#       MOV  $ba $i1 0
#       IADD $i1 $i1 $1
#       PADD $ba $ba $4
#       LEI  $l  $i1 $i2
#       CJMP $l $L_Again
#     %L_Done
#   	}]

# # set bsize  [binary format n [expr $size]]
# # set barray [binary format x[expr max(($size+1)*4)] 0]
# set barray [binary format x[expr 4*$size] 0]
# ::bmath::run $pgm barray

# binary scan $barray n* vlst
# # set i 0
# # foreach {x y} $vlst {
# #  puts [format "xy\[%d\] = (%f,%f)" $i $x $y]
# #  incr i
# # }
# # unset bsize barray
# # exit 0
# puts $vlst


#!/usr/bin/tclsh

#package require Tclbmath
#export TCLLIBPATH=./
#package require Tclbmath
source tclbmath.tcl
# if {$argc != 1} {
#   puts stderr "Usage: $argv0 size"
#   exit 1
# }
# set size [lindex $argv 0]

set size 5

set pgm [::bmath::compile \
  { Bs, Ba } \
  { {0 n 0} {1 n 1} {2 n 2} {4 n 4} {pi f 3.14159265} \
  {One f 1.0} {i1 n 1} {i2 n $size}\
  i tmp th dth ba } \
  {




  	# Instructions I used: MFFI, EQI, CJMP, DFFI,
    #                     MOV,  LTI, PADD, COS, SIN, IADD, LEI
  
    
  }]

set bsize  [binary format n [expr $size]]
set barray [binary format x[expr max(($size+1)*2*4,0)] 0]

::bmath::run $pgm bsize barray

binary scan $barray f* vlst
set i 0
foreach {x y} $vlst {
 puts [format "xy\[%d\] = (%f,%f)" $i $x $y]
 incr i
}
unset bsize barray
exit 0

