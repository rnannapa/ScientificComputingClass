#!/usr/bin/tclsh

package require Tclbmath

# source tclbmath.tcl


if {$argc != 1} {
  puts stderr "Usage: $argv0 size"
  exit 1
}

set size [lindex $argv 0]

set pgm [::bmath::compile \
  { Bs Ba } { {0 n 0} {1 n 1} {2 n 2} {4 n 4} \
  {pi f 3.14159265} {i1 n 0} {i3 f 1.0} i i2 th dth l ba } \
  {
      MOV  $i2  $Bs  0
      LTI  $l   $i2  $0
      CJMP $l   $L_Done

      PADD $ba  $Ba  $0
      MFFI $dth $pi  $2
      EQI  $l   $i2  $0
      CJMP $l   $L_Again
      
      DFFI $dth $dth $i2
    
    %L_Again
      MFFI $th  $dth $i1
      COS  $ba  $th  $i3
      PADD $ba  $ba  $4
      SIN  $ba  $th  $i3
      PADD $ba  $ba  $4
      IADD $i1  $i1  $1
      LEI  $l   $i1  $i2
      CJMP $l $L_Again
    %L_Done
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
