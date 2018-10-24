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
  { Ba } \
  { {0 n 0} {1 n 1} {2 n 2} {4 n 4} {pi f 3.14159265} \
  {One f 1.0} {i1 n 1} {i2 n $size}\
  i tmp th dth ba } \
  {
  #   MFFI $dth $pi   $2
  #   LEI  $i   $0    $size
  #   CJMP $i %L_Done
  #   DFFI $dth $dth $size
  #   set i 0
  # %L_Again
  #   MFFI $th $dth $i
  #   PADD $ba $Ba $4
  #   COS  $ba $dth $One
  #   PADD $ba $Ba $4
  #   SIN  $ba $dth $One
  #   IADD $i $i $1
  #   MOV  $tmp $i 0
  #   LEI  $tmp $tmp $size
  #   CJMP $tmp %L_Again
  # %L_Done
    #bmath pseudo asm code.
    #Instructions I used: MFFI, EQI, CJMP, DFFI,
    #                     MOV,  LTI, PADD, COS, SIN, IADD, LEI
    #Labels I used: %L_Zero, %L_Again, %L_Done
    # PADD $ba $Ba $0
    # MFFI $dth $pi $2
    # LEI  $i $0 $size
    # CJMP $i %L_Zero
    # LEI $i $i1 $i2
    # CJMP $l $L_Done
    # MOV $i $0 0
  %L_Again
    MFFI $th $dth $i
    COS  $tmp $th $One
    MOV $ba $tmp 3
    PADD $ba $ba $4
    SIN $tmp $th $One
    MOV $ba $tmp 3
    IADD $i $i $1
  %L_Done
  %L_Zero

    
  }]

# set bsize  [binary format n [expr $size]]
# set barray [binary format x[expr max(($size+1)*2*4,0)] 0]

# ::bmath::run $pgm bsize barray

# binary scan $barray f* vlst
# set i 0
# foreach {x y} $vlst {
#  puts [format "xy\[%d\] = (%f,%f)" $i $x $y]
#  incr i
# }
# unset bsize barray
# exit 0

# set barray [binary format x[expr 4*$size] 0]
set barray [binary format x[expr max(($size+1)*4)] 0]
#puts [time {
  ::bmath::run $pgm barray
#} 100]

binary scan $barray n* vlst
puts $vlst

