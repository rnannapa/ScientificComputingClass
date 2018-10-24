#!/usr/bin/tclsh

package require Tclbmath

if {$argc != 1} {
  puts stderr "Usage: $argv0 size"
  exit 1
}
set size [lindex $argv 0]

set pgm [::bmath::compile \
  { global vars, I used: Bs, Ba } \
  { constants and local vars, I used: 0, 1, 2, 4, 3.14159265, 1.0 and
    i, tmp, th, dth, ba } \
  {
    bmath pseudo asm code.
    Instructions I used: MFFI, EQI, CJMP, DFFI,
                         MOV,  LTI, PADD, COS, SIN, IADD, LEI
    Labels I used: %L_Zero, %L_Again, %L_Done
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

