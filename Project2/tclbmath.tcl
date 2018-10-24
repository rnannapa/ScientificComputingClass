
if { ![info exists ::_TCLBMATH_] } {
## UNC paths (//Host/share/file) breaks cygwin's [file dirname].
## Here's a workaround:
  if { ![regsub {/[^/]*$} [info script] "" ::_TCLBMATH_] } {
    set ::_TCLBMATH_ ./
  }

# set ::_TCLBMATH_ [file normalize [file dirname [info script]]]

# ::_TCLBMATH_ = path to this script defined in the global scope.
### When !exists ::_TCLBMATH_ ##################################################

package provide Tclbmath 1.0

namespace eval ::bmath  {

  set bits [expr 8*$tcl_platform(pointerSize)]
  switch -glob -- $::tcl_platform(os) {
    "CYGWIN_NT-*" -
    "Windows NT*" {
      load $::_TCLBMATH_/tclbmath-w${bits}.dll
    }
    "Linux" -
    default {
      load $::_TCLBMATH_/tclbmath-l${bits}.so
    }
  }
  unset bits

  set enumCMDMAP { PADD PSUB
                   IADD ISUB IMUL IDIV IMIN IMAX
                   FADD FSUB FMUL FDIV FMIN FMAX
                   MFFI DFFI MBBF MSSF MOV  COPY
                   LTP  LEP  EQP  NEP
                   LTI  LEI  EQI  NEI
                   LTF  LEF  EQF  NEF
                   LAND LOR  AND  OR   SHFT 
                   CJMP
                   IMOD IABS FMOD FABS
                   SQRT OSQR EXP  LOG  POW
                   SIN  COS  ATAN
                   FLR  CEIL ROUN
                   GLYAX4 GLYAX3 GLYAX2 GLYAX1
                   GLPA4  GLPA3  GLPA2
                   GLAITS GLNR2  GLNR3 }
  for {set n 0} {$n < [llength $enumCMDMAP]} {incr n} {
    set CMDMAP([lindex $enumCMDMAP $n]) $n
  }
  unset enumCMDMAP


  proc compileSet {args} {
    foreach {n v} $args {
      uplevel "set $n \"$v\""
    }
  }


  proc compileSubst {varlst pgmstr} {
    set { hide varlst } $varlst
    set { hide pgmstr } $pgmstr
    unset varlst pgmstr
    compileSet {*}${ hide varlst }
    if {[catch {set pgmstr [string trim [subst -nocommands ${ hide pgmstr }]]}]} {
      return -code error -errorinfo "Probable undefined varname."
    } else {
      return -code ok $pgmstr
    }
  }


  proc compile {glst llst pgm} {
    variable CMDMAP
    regsub -all {#[^\n]*} $pgm "" pgm

    set pgmstr ""
    set varlst {}
    set np 0
    foreach l [split $pgm \n] {
      set tmplst [list {*}$l]
      set tmp0 [lindex $tmplst 0]
      switch -glob -- $tmp0 {
        "" continue
        "%*" {
          lappend varlst [string range $tmp0 1 end] [list [expr (($np)>>8)&255] [expr ($np)&255]]
        }
        default {
          if {![info exists CMDMAP($tmp0)]} {
            return -code error -errorinfo "bmath opname $tmp0 is unknown."
          }
          append pgmstr "$CMDMAP($tmp0) [lindex $tmplst 1] [lindex $tmplst 2] [lindex $tmplst 3]\n"
          incr np
        }
      }
    }

    set glst [uplevel [list subst $glst]]
    set ng 0
    foreach g $glst {
      lappend varlst $g $ng
      incr ng
    }

    set llst [uplevel [list subst $llst]]
    set nl 0
    set b_ldata {}
    foreach l $llst {
      lappend varlst [lindex $l 0] [expr $ng+$nl]
      if { [llength $l] == 1 } {
        append b_ldata [binary format x4]
      } else {
        if { [llength $l] == 3 && ([lindex $l 1] eq "n" || [lindex $l 1] eq "f") } {
          append b_ldata [binary format [lindex $l 1] [lindex $l 2]]
        } else {
          return -code error -errorinfo "invalid local variable initialization"
        }
      }
      incr nl
    }

    set b_pgm ""
    append b_pgm [binary format aaaann B P G M $ng $nl] \
                 $b_ldata \
                 [binary format c* [list {*}[compileSubst $varlst $pgmstr]]]
    if {[string length $b_pgm] & 0x3} {
      return -code error -errorinfo "Compiled bytecode is not a multiple of 4."
    }
    return $b_pgm
  }

}; # end namespace eval bmath

################################################################################

}; # end info exists _TCLBMATH_


