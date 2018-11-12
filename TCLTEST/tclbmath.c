/* #define PRINT_INFO */

#if (defined(WIN32))
#  include <windows.h>
#endif

#include <tcl.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* memcpy() */

typedef char* PTR1;

static char buf1024[1024];
static Tcl_ObjType *stringTypePtr, 
                   *bytearrayTypePtr,
                   *listTypePtr;

static int bmath_run
( ClientData, Tcl_Interp*, int, Tcl_Obj*[] );

static char *cur_ns = NULL;
static int   cur_ns_len =  0;

int Tclbmath_Init( Tcl_Interp *interp ) {
# ifdef USE_TCL_STUBS
  if(Tcl_InitStubs(interp, "8.5", 0) == NULL)
    return TCL_ERROR;
# endif

  cur_ns = Tcl_GetCurrentNamespace(interp)->fullName;
  cur_ns_len = strlen( cur_ns );

  sprintf( buf1024, "%s%s", cur_ns, "::run" );
  Tcl_CreateObjCommand( interp,
                        buf1024,
                        (Tcl_ObjCmdProc*)bmath_run,
                        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL );

  stringTypePtr    = (Tcl_ObjType*)Tcl_GetObjType( "string" );
  bytearrayTypePtr = (Tcl_ObjType*)Tcl_GetObjType( "bytearray" );
  listTypePtr      = (Tcl_ObjType*)Tcl_GetObjType( "list" );
  return TCL_OK;
}

PTR1 getAddressFromBytearrayName
( Tcl_Interp *interp, Tcl_Obj *objName ) {
  Tcl_Obj *obj;

  obj = Tcl_ObjGetVar2( interp, objName, NULL, TCL_LEAVE_ERR_MSG );
  if( !obj ) 
    return (PTR1)0;

#ifdef PRINT_INFO
  printf( "Get address for %s's value\n", objName->bytes );
  if( objName->typePtr ) {
  printf( "      name type %s\n", objName->typePtr->name );
  } else {
  printf( "      name type %s\n", "null typePtr" );
  }
  printf( "     value type %s\n", obj->typePtr->name );
  printf( "       refCount %d\n", obj->refCount );
  printf( "\n" );
  fflush(stdout);
#endif

  if( Tcl_IsShared(obj) ) {
    Tcl_Obj *new = Tcl_DuplicateObj( obj );
    if( !Tcl_ObjSetVar2( interp, objName, NULL, new, TCL_LEAVE_ERR_MSG ) ) {
      Tcl_DecrRefCount( new );
      return (PTR1)0;
    }
    /* Tcl_ObjSetVar2() will decrement the old obj and increment new. */
    obj = new;
  }
  if( !obj->typePtr || obj->typePtr == stringTypePtr ) {
    if( Tcl_ConvertToType(interp,obj,bytearrayTypePtr) != TCL_OK )
      return (PTR1)0;
  }
  else
  if( obj->typePtr != bytearrayTypePtr ) {
    sprintf( buf1024, "Will not attempt to convert type \"%s\" to type \"bytearray\"",
      obj->typePtr->name );
    Tcl_AppendResult( interp, buf1024, (char*)NULL );
    return (PTR1)0;
  }
  Tcl_InvalidateStringRep(obj);
  return (PTR1)Tcl_GetByteArrayFromObj(obj,NULL); 
}

PTR1 getAddrFromObj
( Tcl_Interp *interp, Tcl_Obj *obj ) {
  PTR1 retaddr;

  if( obj->typePtr == listTypePtr ) {
    int llen, offset;
    Tcl_Obj **lobjv, *resultObj;

    if( Tcl_ListObjGetElements(interp,obj,&llen,&lobjv) != TCL_OK )
      return (PTR1)0;
    offset = 0;
    switch( llen ) {
      case(2):
        if( Tcl_ExprObj(interp,lobjv[1],&resultObj) != TCL_OK )
          return (PTR1)0;
        if( Tcl_GetIntFromObj(interp,resultObj,&offset) != TCL_OK )
          return (PTR1)0;
      case(1):
#ifdef PRINT_INFO
        printf( "name in list\n" );
        if( lobjv[0]->typePtr )
          printf( "name %s, name's type %s\n", lobjv[0]->bytes, lobjv[0]->typePtr->name );
        else
          printf( "name %s, name's type %s\n", lobjv[0]->bytes, "(null)" );
        fflush(stdout);
#endif
        if( !(retaddr = getAddressFromBytearrayName(interp,lobjv[0])) )
          return (PTR1)0;
        return retaddr+offset;
      default:
        sprintf( buf1024, "Argument must be a list with one or two elements." );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return (PTR1)0;
    }
  }
  else {
#ifdef PRINT_INFO
        printf( "name not in list\n" );
        if( obj->typePtr )
          printf( "name %s, name's type %s\n", obj->bytes, obj->typePtr->name );
        else
          printf( "name %s, name's type %s\n", obj->bytes, "(null)" );
        fflush(stdout);
#endif
    if( !(retaddr = getAddressFromBytearrayName(interp,obj)) )
      return (PTR1)0;
    return retaddr;
  }
}

int bmath_run
( ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *objv[] ) {
  char *b_pgm;
  int  npgm, ngbl, nlcl, n, npgm_cmds, cntr;
  int  data_buf[256];
  PTR1 data_ptrs[256];
  struct CmdLine {
    unsigned char cmd, idx1, idx2, idx3;
  } *pgm_cmds;
  enum CMDMAP { PADD, PSUB,
                IADD, ISUB, IMUL, IDIV, IMIN, IMAX, 
                FADD, FSUB, FMUL, FDIV, FMIN, FMAX,
                MFFI, DFFI, MBBF, MSSF, MOV,  COPY,
                LTP,  LEP,  EQP,  NEP,
                LTI,  LEI,  EQI,  NEI,
                LTF,  LEF,  EQF,  NEF,
                LAND, LOR,  AND,  OR,   SHFT,
                CJMP, 
                IMOD, IABS, FMOD, FABS, 
                SQRT, OSQR, EXP,  LOG,  POW,
                SIN,  COS,  ATAN,
                FLR,  CEIL, ROUN,
                GLYAX4, GLYAX3, GLYAX2, GLYAX1,
                GLPA4,  GLPA3,  GLPA2,
                GLAITS, GLNR2,  GLNR3 };
 
  if( objc < 2 ) {
    Tcl_WrongNumArgs( interp, 1, objv, "b_program ?b_arg1? ..." );
    return TCL_ERROR;
  }

  if( objv[1]->typePtr != bytearrayTypePtr ) {
    /* tcl 8.5 will often convert byte arrays to strings for no reason. */
    if( !objv[1]->typePtr || objv[1]->typePtr == stringTypePtr ) {
#ifdef PRINT_INFO
      printf( "Will try to convert program from string to bytearray type..." );
      fflush(stdout);
#endif
      if( Tcl_ConvertToType(interp,objv[1],bytearrayTypePtr) != TCL_OK ) {
        sprintf( buf1024, "Program argument can not be converted to bytearray type." );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return TCL_ERROR;
      }
#ifdef PRINT_INFO
      printf( " OK\n" );
      fflush(stdout);
#endif
    }
    else {
      sprintf( buf1024, "Program arg (typed %s) must be bytearray or string type.", objv[1]->typePtr->name );
      Tcl_AppendResult( interp, buf1024, (char*)NULL );
      return TCL_ERROR;
    }
  }

  b_pgm = (char*)Tcl_GetByteArrayFromObj( objv[1], &npgm );
  if( npgm & 0x3 || npgm < 12 ) {
    sprintf( buf1024, "Program's byte length is impossible." );
    Tcl_AppendResult( interp, buf1024, (char*)NULL );
    return TCL_ERROR;
  }
  if( b_pgm[0] != 'B' || b_pgm[1] != 'P'
   || b_pgm[2] != 'G' || b_pgm[3] != 'M' ) {
    sprintf( buf1024, "Incorrect program magic number." );
    Tcl_AppendResult( interp, buf1024, (char*)NULL );
    return TCL_ERROR;
  }
  ngbl = ((int*)b_pgm)[1];
  if( objc-2 != ngbl ) {
    sprintf( buf1024, "The given program requires exactly %d args.", ngbl );
    Tcl_AppendResult( interp, buf1024, (char*)NULL );
    return TCL_ERROR;
  }
  for( n = 0; n < ngbl; n++ ) {
    if( !(data_ptrs[n] = getAddrFromObj(interp,objv[2+n])) )
      return TCL_ERROR;
  }
  nlcl = ((int*)b_pgm)[2];
  if( npgm < (3+nlcl)*4 ) {
    sprintf( buf1024, "The program's byte array is unexpectedly truncated." );
    Tcl_AppendResult( interp, buf1024, (char*)NULL ); 
    return TCL_ERROR;
  }
  memcpy( data_buf, &((int*)b_pgm)[3], nlcl*4 );
  for( n = 0; n < nlcl; n++ ) {
    data_ptrs[ngbl+n] = (PTR1)&data_buf[n];
  }
  pgm_cmds  = (struct CmdLine*)&((int*)b_pgm)[3+nlcl];
  npgm_cmds = npgm/4 - (3+nlcl);

  for( cntr = 0; cntr < npgm_cmds;) {
    register struct CmdLine *p = &pgm_cmds[cntr];
    cntr++;
    switch( p->cmd ) {
    case(PADD):
      data_ptrs[p->idx1]  
      =
      data_ptrs[p->idx2] + *((int*)data_ptrs[p->idx3]);
      break;
    case(PSUB):
      *((int*)data_ptrs[p->idx1])
      =
      data_ptrs[p->idx2] - data_ptrs[p->idx3];
      break;

    case(IADD):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) + *((int*)data_ptrs[p->idx3]);
      break;
    case(ISUB):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) - *((int*)data_ptrs[p->idx3]);
      break;
    case(IMUL):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) * *((int*)data_ptrs[p->idx3]);
      break;
    case(IDIV):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) / *((int*)data_ptrs[p->idx3]);
      break;
    case(IMIN):
      if( *((int*)data_ptrs[p->idx2]) < *((int*)data_ptrs[p->idx3]) )
        *((int*)data_ptrs[p->idx1]) = *((int*)data_ptrs[p->idx2]);
      else
        *((int*)data_ptrs[p->idx1]) = *((int*)data_ptrs[p->idx3]);
      break;
    case(IMAX):
      if( *((int*)data_ptrs[p->idx2]) > *((int*)data_ptrs[p->idx3]) )
        *((int*)data_ptrs[p->idx1]) = *((int*)data_ptrs[p->idx2]);
      else
        *((int*)data_ptrs[p->idx1]) = *((int*)data_ptrs[p->idx3]);
      break;

    case(FADD):
      *((float*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) + *((float*)data_ptrs[p->idx3]);
      break;
    case(FSUB):
      *((float*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) - *((float*)data_ptrs[p->idx3]);
      break;
    case(FMUL):
      *((float*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) * *((float*)data_ptrs[p->idx3]);
      break;
    case(FDIV):
      *((float*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) / *((float*)data_ptrs[p->idx3]);
      break;
    case(FMIN):
      if( *((float*)data_ptrs[p->idx2]) < *((float*)data_ptrs[p->idx3]) )
        *((float*)data_ptrs[p->idx1]) = *((float*)data_ptrs[p->idx2]);
      else
        *((float*)data_ptrs[p->idx1]) = *((float*)data_ptrs[p->idx3]);
      break;
    case(FMAX):
      if( *((float*)data_ptrs[p->idx2]) > *((float*)data_ptrs[p->idx3]) )
        *((float*)data_ptrs[p->idx1]) = *((float*)data_ptrs[p->idx2]);
      else
        *((float*)data_ptrs[p->idx1]) = *((float*)data_ptrs[p->idx3]);
      break;

    case(MFFI):
      *((float*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) * *((int*)data_ptrs[p->idx3]);
      break;
    case(DFFI):
      *((float*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) / *((int*)data_ptrs[p->idx3]);
      break;
    case(MBBF): {
      float tmp;
      tmp = *((unsigned char*)data_ptrs[p->idx2]) 
          * *((float*)data_ptrs[p->idx3]);
      if( tmp < 0 )
        tmp = 0.0;
      else
      if( tmp > 255.0 )
        tmp = 255.0;
      *((unsigned char*)data_ptrs[p->idx1]) = (unsigned char)tmp;
      break;
    }
    case(MSSF): {
      float tmp;
      tmp = *((unsigned short*)data_ptrs[p->idx2]) 
          * *((float*)data_ptrs[p->idx3]);
      if( tmp < 0 )
        tmp = 0.0;
      else
      if( tmp > 65535.0 )
        tmp = 65535.0;
      *((unsigned short*)data_ptrs[p->idx1]) = (unsigned short)tmp;
      break;
    }
    case(MOV):
      switch( p->idx3 ) {
        case(0):
          *((int*)data_ptrs[p->idx1])
          =
          *((int*)data_ptrs[p->idx2]);
          break;
        case(1):
          *(  (int*)data_ptrs[p->idx1])
          =
          *((float*)data_ptrs[p->idx2]);
          break;
        case(2):
          *((float*)data_ptrs[p->idx1])
          =
          *(  (int*)data_ptrs[p->idx2]);
          break;
        case(3):
          *((float*)data_ptrs[p->idx1])
          =
          *((float*)data_ptrs[p->idx2]);
          break;
        case(4):
          *((int*)data_ptrs[p->idx1])
          = -
          *((int*)data_ptrs[p->idx2]);
          break;
        case(5):
          *(  (int*)data_ptrs[p->idx1])
          = -
          *((float*)data_ptrs[p->idx2]);
          break;
        case(6):
          *((float*)data_ptrs[p->idx1])
          = -
          *(  (int*)data_ptrs[p->idx2]);
          break;
        case(7):
          *((float*)data_ptrs[p->idx1])
          = -
          *((float*)data_ptrs[p->idx2]);
          break;
        default:
          sprintf( buf1024, "%s: MOV arg3 must be from {0,...,7}.", objv[0]->bytes );
          Tcl_AppendResult( interp, buf1024, (char*)NULL );
          return TCL_ERROR;
      }
      break;
    case(COPY):
      memcpy( data_ptrs[p->idx1], data_ptrs[p->idx2], 
              *((int*)data_ptrs[p->idx3]) );
      break;

    case(LTP):
      *((int*)data_ptrs[p->idx1])  
      =
      data_ptrs[p->idx2] < data_ptrs[p->idx3];
      break;
    case(LEP):
      *((int*)data_ptrs[p->idx1])  
      =
      data_ptrs[p->idx2] <= data_ptrs[p->idx3];
      break;
    case(EQP):
      *((int*)data_ptrs[p->idx1])  
      =
      data_ptrs[p->idx2] == data_ptrs[p->idx3];
      break;
    case(NEP):
      *((int*)data_ptrs[p->idx1])  
      =
      data_ptrs[p->idx2] != data_ptrs[p->idx3];
      break;

    case(LTI):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) < *((int*)data_ptrs[p->idx3]);
      break;
    case(LEI):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) <= *((int*)data_ptrs[p->idx3]);
      break;
    case(EQI):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) == *((int*)data_ptrs[p->idx3]);
      break;
    case(NEI):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) != *((int*)data_ptrs[p->idx3]);
      break;

    case(LTF):
      *((int*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) < *((float*)data_ptrs[p->idx3]);
      break;
    case(LEF):
      *((int*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) <= *((float*)data_ptrs[p->idx3]);
      break;
    case(EQF):
      *((int*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) == *((float*)data_ptrs[p->idx3]);
      break;
    case(NEF):
      *((int*)data_ptrs[p->idx1])  
      =
      *((float*)data_ptrs[p->idx2]) != *((float*)data_ptrs[p->idx3]);
      break;

    case(LAND):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) && *((int*)data_ptrs[p->idx3]);
      break;
    case(LOR):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) || *((int*)data_ptrs[p->idx3]);
      break;
    case(AND):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) & *((int*)data_ptrs[p->idx3]);
      break;
    case(OR):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) | *((int*)data_ptrs[p->idx3]);
      break;
    case(SHFT):
      *((unsigned int*)data_ptrs[p->idx1])  
      =
      *((unsigned int*)data_ptrs[p->idx2]) << *((int*)data_ptrs[p->idx3]);
      break;

    case(CJMP):
      if( *((int*)data_ptrs[p->idx1]) )
        cntr = (p->idx2<<8)|(p->idx3);
      break;

    case(IMOD):
      *((int*)data_ptrs[p->idx1])  
      =
      *((int*)data_ptrs[p->idx2]) % *((int*)data_ptrs[p->idx3]);
      break;
    case(IABS):
      *((int*)data_ptrs[p->idx1])  
      =
      abs( *((int*)data_ptrs[p->idx2]) ) * *((int*)data_ptrs[p->idx3]);
      break;
    case(FMOD):
      *((float*)data_ptrs[p->idx1])
      =
      fmod( *((float*)data_ptrs[p->idx2]), *((float*)data_ptrs[p->idx3]) );
      break;
    case(FABS):
      *((float*)data_ptrs[p->idx1])
      =
      fabs( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;

    case(SQRT):
      if( *((float*)data_ptrs[p->idx2]) < 0.0 ) {
        sprintf( buf1024, "%s: SQRT domain error.", objv[0]->bytes );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return TCL_ERROR;
      }
      *((float*)data_ptrs[p->idx1])
      =
      sqrt( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;
    case(OSQR):
      if( *((float*)data_ptrs[p->idx2]) <= 0.0 ) {
        sprintf( buf1024, "%s: OSQR domain error.", objv[0]->bytes );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return TCL_ERROR;
      }
      *((float*)data_ptrs[p->idx1])
      = *((float*)data_ptrs[p->idx3]) / sqrt( *((float*)data_ptrs[p->idx2]) );
      break;
    case(EXP):
      *((float*)data_ptrs[p->idx1])
      =
      exp( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;
    case(LOG):
      if( *((float*)data_ptrs[p->idx2]) <= 0.0 ) {
        sprintf( buf1024, "%s: LOG domain error.", objv[0]->bytes );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return TCL_ERROR;
      }
      *((float*)data_ptrs[p->idx1])
      = log( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;
    case(POW):
      *((float*)data_ptrs[p->idx1])
      =
      pow( *((float*)data_ptrs[p->idx2]), *((float*)data_ptrs[p->idx3]) );
      break;

    case(SIN):
      *((float*)data_ptrs[p->idx1])
      =
      sin( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;
    case(COS):
      *((float*)data_ptrs[p->idx1])
      =
      cos( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;
    case(ATAN):
      *((float*)data_ptrs[p->idx1])
      =
      atan2( *((float*)data_ptrs[p->idx2]), *((float*)data_ptrs[p->idx3]) );
      break;

    case(FLR):
      *((float*)data_ptrs[p->idx1])
      =
      floor( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;
    case(CEIL):
      *((float*)data_ptrs[p->idx1])
      =
      ceil( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;
    case(ROUN):
      *((float*)data_ptrs[p->idx1])
      =
      round( *((float*)data_ptrs[p->idx2]) ) * *((float*)data_ptrs[p->idx3]);
      break;

    case(GLYAX4): {
      float *A=(float*)data_ptrs[p->idx2],
            *x=(float*)data_ptrs[p->idx3+0],*y=(float*)data_ptrs[p->idx3+1],
            *z=(float*)data_ptrs[p->idx3+2],*w=(float*)data_ptrs[p->idx3+3],
            sv_x, sv_y, sv_z, sv_w;
      sv_x = A[0]*(*x) + A[4]*(*y) + A[ 8]*(*z);
      sv_y = A[1]*(*x) + A[5]*(*y) + A[ 9]*(*z);
      sv_z = A[2]*(*x) + A[6]*(*y) + A[10]*(*z);
      sv_w = A[3]*(*x) + A[7]*(*y) + A[11]*(*z);
      *((float*)data_ptrs[p->idx1+0]) = sv_x + A[12]*(*w);
      *((float*)data_ptrs[p->idx1+1]) = sv_y + A[13]*(*w);
      *((float*)data_ptrs[p->idx1+2]) = sv_z + A[14]*(*w);
      *((float*)data_ptrs[p->idx1+3]) = sv_w + A[15]*(*w);
      break;
    }
    case(GLYAX3): {
      float *A=(float*)data_ptrs[p->idx2],
            *x=(float*)data_ptrs[p->idx3+0],*y=(float*)data_ptrs[p->idx3+1],
            *z=(float*)data_ptrs[p->idx3+2],
            sv_x, sv_y, sv_z;
      sv_x = A[0]*(*x) + A[4]*(*y) + A[ 8]*(*z);
      sv_y = A[1]*(*x) + A[5]*(*y) + A[ 9]*(*z);
      sv_z = A[2]*(*x) + A[6]*(*y) + A[10]*(*z);
      *((float*)data_ptrs[p->idx1+0]) = sv_x + A[12];
      *((float*)data_ptrs[p->idx1+1]) = sv_y + A[13];
      *((float*)data_ptrs[p->idx1+2]) = sv_z + A[14];
      break;
    }
    case(GLYAX2): {
      float *A=(float*)data_ptrs[p->idx2],
            *x=(float*)data_ptrs[p->idx3+0],*y=(float*)data_ptrs[p->idx3+1],
            sv_x, sv_y;
      sv_x = A[0]*(*x) + A[4]*(*y);
      sv_y = A[1]*(*x) + A[5]*(*y);
      *((float*)data_ptrs[p->idx1+0]) = sv_x + A[12];
      *((float*)data_ptrs[p->idx1+1]) = sv_y + A[13];
      break;
    }
    case(GLYAX1): {
      float *A=(float*)data_ptrs[p->idx2],
            *x=(float*)data_ptrs[p->idx3+0];
      *((float*)data_ptrs[p->idx1+0]) = A[0]*(*x) + A[12];
      break;
    }
    case(GLPA4): {
      int stride=*((int*)data_ptrs[p->idx3]);
      data_ptrs[p->idx1+0] = data_ptrs[p->idx2+0]+stride;
      data_ptrs[p->idx1+1] = data_ptrs[p->idx2+1]+stride;
      data_ptrs[p->idx1+2] = data_ptrs[p->idx2+2]+stride;
      data_ptrs[p->idx1+3] = data_ptrs[p->idx2+3]+stride;
      break;
    }
    case(GLPA3): {
      int stride=*((int*)data_ptrs[p->idx3]);
      data_ptrs[p->idx1+0] = data_ptrs[p->idx2+0]+stride;
      data_ptrs[p->idx1+1] = data_ptrs[p->idx2+1]+stride;
      data_ptrs[p->idx1+2] = data_ptrs[p->idx2+2]+stride;
      break;
    }
    case(GLPA2): {
      int stride=*((int*)data_ptrs[p->idx3]);
      data_ptrs[p->idx1+0] = data_ptrs[p->idx2+0]+stride;
      data_ptrs[p->idx1+1] = data_ptrs[p->idx2+1]+stride;
      break;
    }

    case(GLAITS): {
      float *R=(float*)data_ptrs[p->idx1],
            *A=(float*)data_ptrs[p->idx2], tmp1[4*4], tmp2[4*4];
      if( p->idx3 > 7 ) { 
        sprintf( buf1024, "%s: GLAITS arg3 must be from {0,...,7}.", objv[0]->bytes );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return TCL_ERROR;
      } 
      /* Inverse */
      if( 1 & p->idx3 ) {
        float dlo01, dlo02, dlo03, dlo12, dlo13, dlo23, 
              dhi01, dhi02, dhi03, dhi12, dhi13, dhi23, det, odet;
        dlo01 = A[0+0*4]*A[1+1*4]-A[0+1*4]*A[1+0*4]; dhi01 = A[2+0*4]*A[3+1*4]-A[2+1*4]*A[3+0*4];
        dlo02 = A[0+0*4]*A[1+2*4]-A[0+2*4]*A[1+0*4]; dhi02 = A[2+0*4]*A[3+2*4]-A[2+2*4]*A[3+0*4];
        dlo03 = A[0+0*4]*A[1+3*4]-A[0+3*4]*A[1+0*4]; dhi03 = A[2+0*4]*A[3+3*4]-A[2+3*4]*A[3+0*4];
        dlo12 = A[0+1*4]*A[1+2*4]-A[0+2*4]*A[1+1*4]; dhi12 = A[2+1*4]*A[3+2*4]-A[2+2*4]*A[3+1*4];
        dlo13 = A[0+1*4]*A[1+3*4]-A[0+3*4]*A[1+1*4]; dhi13 = A[2+1*4]*A[3+3*4]-A[2+3*4]*A[3+1*4];
        dlo23 = A[0+2*4]*A[1+3*4]-A[0+3*4]*A[1+2*4]; dhi23 = A[2+2*4]*A[3+3*4]-A[2+3*4]*A[3+2*4];
        det = dlo01*dhi23 - dlo02*dhi13 + dlo03*dhi12 + dlo12*dhi03 - dlo13*dhi02 + dlo23*dhi01;
        if( det == 0.0 ) {
          sprintf( buf1024, "%s: GLAIT 1, noninvertible error.", objv[0]->bytes );
          Tcl_AppendResult( interp, buf1024, (char*)NULL );
          return TCL_ERROR;
        }
        odet = 1.0/det;
        /* (adj row/transpose col 0) */
        tmp1[0+0*4] = +odet*(A[1+1*4]*dhi23 - A[1+2*4]*dhi13 + A[1+3*4]*dhi12);
        tmp1[1+0*4] = -odet*(A[1+0*4]*dhi23 - A[1+2*4]*dhi03 + A[1+3*4]*dhi02);
        tmp1[2+0*4] = +odet*(A[1+0*4]*dhi13 - A[1+1*4]*dhi03 + A[1+3*4]*dhi01);
        tmp1[3+0*4] = -odet*(A[1+0*4]*dhi12 - A[1+1*4]*dhi02 + A[1+2*4]*dhi01);
        /* (adj row/transpose col 1) */
        tmp1[0+1*4] = -odet*(A[0+1*4]*dhi23 - A[0+2*4]*dhi13 + A[0+3*4]*dhi12);
        tmp1[1+1*4] = +odet*(A[0+0*4]*dhi23 - A[0+2*4]*dhi03 + A[0+3*4]*dhi02);
        tmp1[2+1*4] = -odet*(A[0+0*4]*dhi13 - A[0+1*4]*dhi03 + A[0+3*4]*dhi01);
        tmp1[3+1*4] = +odet*(A[0+0*4]*dhi12 - A[0+1*4]*dhi02 + A[0+2*4]*dhi01);
        /* (adj row/transpose col 2) */
        tmp1[0+2*4] = +odet*(A[3+1*4]*dlo23 - A[3+2*4]*dlo13 + A[3+3*4]*dlo12);
        tmp1[1+2*4] = -odet*(A[3+0*4]*dlo23 - A[3+2*4]*dlo03 + A[3+3*4]*dlo02);
        tmp1[2+2*4] = +odet*(A[3+0*4]*dlo13 - A[3+1*4]*dlo03 + A[3+3*4]*dlo01);
        tmp1[3+2*4] = -odet*(A[3+0*4]*dlo12 - A[3+1*4]*dlo02 + A[3+2*4]*dlo01);
        /* (adj row/transpose col 3) */
        tmp1[0+3*4] = -odet*(A[2+1*4]*dlo23 - A[2+2*4]*dlo13 + A[2+3*4]*dlo12);
        tmp1[1+3*4] = +odet*(A[2+0*4]*dlo23 - A[2+2*4]*dlo03 + A[2+3*4]*dlo02);
        tmp1[2+3*4] = -odet*(A[2+0*4]*dlo13 - A[2+1*4]*dlo03 + A[2+3*4]*dlo01);
        tmp1[3+3*4] = +odet*(A[2+0*4]*dlo12 - A[2+1*4]*dlo02 + A[2+2*4]*dlo01);
        A = tmp1;
      }
      /* Transpose */
      if( 2 & p->idx3 ) {
        tmp2[0]  = A[0]; tmp2[1]  = A[4]; tmp2[2]  = A[8];  tmp2[3]  = A[12]; 
        tmp2[4]  = A[1]; tmp2[5]  = A[5]; tmp2[6]  = A[9];  tmp2[7]  = A[13]; 
        tmp2[8]  = A[2]; tmp2[9]  = A[6]; tmp2[10] = A[10]; tmp2[11] = A[14]; 
        tmp2[12] = A[3]; tmp2[13] = A[7]; tmp2[14] = A[11]; tmp2[15] = A[15]; 
        A = tmp2;
      }
      /* 4x4 to 3x3 submatrix */ 
      if( 4 & p->idx3 ) {
        memcpy( &R[0], &A[0], 3*sizeof(float) );
        memcpy( &R[3], &A[4], 3*sizeof(float) );
        memcpy( &R[6], &A[8], 3*sizeof(float) );
      } else {
        memcpy( R, A, 16*sizeof(float) );
      }
      break;
    }
    case(GLNR2): {
      float x=*((float*)data_ptrs[p->idx2+0]),
            y=*((float*)data_ptrs[p->idx2+1]),
            osr;
      osr = x*x+y*y;
      if( osr == 0.0 ) {
        sprintf( buf1024, "%s: GLNR2, zero length vector error.", objv[0]->bytes );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return TCL_ERROR;
      }
      osr = *((float*)data_ptrs[p->idx3])/sqrt(osr);
      *((float*)data_ptrs[p->idx1+0]) = x*osr;
      *((float*)data_ptrs[p->idx1+1]) = y*osr;
      break;
    }
    case(GLNR3): {
      float x=*((float*)data_ptrs[p->idx2+0]),
            y=*((float*)data_ptrs[p->idx2+1]),
            z=*((float*)data_ptrs[p->idx2+2]),
            osr;
      osr = x*x+y*y+z*z;
      if( osr == 0.0 ) {
        sprintf( buf1024, "%s: GLNR3, zero length vector error.", objv[0]->bytes );
        Tcl_AppendResult( interp, buf1024, (char*)NULL );
        return TCL_ERROR;
      }
      osr = *((float*)data_ptrs[p->idx3])/sqrt(osr);
      *((float*)data_ptrs[p->idx1+0]) = x*osr;
      *((float*)data_ptrs[p->idx1+1]) = y*osr;
      *((float*)data_ptrs[p->idx1+2]) = z*osr;
      break;
    }

    default:  
      sprintf( buf1024, "%s: Unknown command %d", objv[0]->bytes, p->cmd );
      Tcl_AppendResult( interp, buf1024, (char*)NULL );
      return TCL_ERROR;

    }
  }
  return TCL_OK;
}

