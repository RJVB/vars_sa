#ifndef _NAN_H
#define _NAN_H
/* RJVB: header file with the definitions of NaN and Inf for IEEE double floating point variables.
 */

/* since math.h contains a MAXDOUBLE definition, undef any
 * made in limits.h
 */
#ifdef MAXDOUBLE
#	undef MAXDOUBLE
#endif

  /* conditional inclusion of header files would be so great... Now you will have
   \ to add the symbol defining your environment when necessary....
   */
#if defined (linux) || defined(sgi) || defined(__GNUC__) || defined(_AIX)
#	include "math.h"
#endif
#include "mathdef.h"

/* Handling of Not_a_Number's (only in IEEE floating-point standard) */

#ifdef linux
#	include <bits/nan.h>
/* #elif _AIX	*/
  /* Doesn't have a nan.h - we don't really need it neither.	*/
/* #else	*/
/* #	include <nan.h>	*/
#endif

#ifdef VARS_STANDALONE
#	include "64typedefs.h"
#else
#	include "local/64typedefs.h"
#endif

#define NAN_VAL	0x7ff
#define POS_INF	0x7ff00000
#define NEG_INF	0xfff00000

#if defined(i386) || defined(x86_64) || defined(__LITTLE_ENDIAN__) || (defined(BYTE_ORDER) && BYTE_ORDER==LITTLE_ENDIAN)
/* definitions for byte-reversed-ordered machines:	*/
typedef union IEEEsfp {
	struct {
		int16 low, high;
	} l;
	struct {
		uint32 f:23;
		uint32 e:8;
		uint32 s:1;
	} s;
	float f;
} IEEEsfp;

typedef union IEEEfp {
	struct {
		int32 low, high;
	} l;
	struct {
/* 		uint32 f1:32,f2:20;	*/
		uint32 f2:32,f1:20;
		uint32 e:11;
		uint32 s:1;
	} s;
	double d;
} IEEEfp;

#	define posInf	*((double*)&((IEEEfp){0,POS_INF}))

#else
typedef union IEEEsfp {
	struct {
		int16 high, low;
	} l;
	struct {
		uint32 s:1;
		uint32 e:8;
		uint32 f:23;
	} s;
	float f;
} IEEEsfp;

typedef union IEEEfp {
	struct {
		int32 high, low;
	} l;
	struct {
		uint32 s:1;
		uint32 e:11;
		uint32 f1:20, f2:32;
	} s;
	double d;
} IEEEfp;

#	define posInf	*((double*)&((IEEEfp){POS_INF,0}))

#endif

#if __GNUC__==4 && __GNUC_MINOR__==0 && (defined(__APPLE_CC__) || defined(__MACH__))
/* 20050808: on this host/compiler combination, it appears to be necessary to use the
 \ system calls isnan() and/or isinf() for reliable detection. This reeks of a compiler error.
 */
#	define USE_HOST_NANFS
#endif

#undef NaN

#define I3Ed(X)	((IEEEfp*)&(X))
#define I3Edf(X)	((IEEEsfp*)&(X))

/* NaN or Inf: (s.e==NAN_VAL and ((s.f1!=0 and s.f2!=0) or (s.f1==0 and s.f2==0)))
 * i.e. s.e==NAN_VAL and (!(s.f1 xor s.f2))
 */
#	define NaNorInf(X)	(((IEEEfp *)&(X))->s.e==NAN_VAL)

#define fpNaNorInf(X)	((sizeof(X)==sizeof(float))? \
		(((IEEEsfp *)&(X))->s.e==0xff) :\
		(((IEEEfp *)&(X))->s.e==NAN_VAL) \
	)


#	define NaN(X)	( (I3Ed(X)->s.e==NAN_VAL)? \
	((I3Ed(X)->s.f1!= 0 )? I3Ed(X)->s.f1 : \
		((I3Ed(X)->s.f2!= 0)? I3Ed(X)->s.f2 : 0)) : 0)

#define fpNaN(X)	( (sizeof(X)==sizeof(float))? \
		((I3Edf(X)->s.e==0xff)?  I3Edf(X)->s.f : 0 ) : \
		((I3Ed(X)->s.e==NAN_VAL)? \
		((I3Ed(X)->s.f1!= 0 )? I3Ed(X)->s.f1 : \
			((I3Ed(X)->s.f2!= 0)? I3Ed(X)->s.f2 : 0)) : 0) \
	)

/* isNaN() only tests for NaN-ness, without returning the relevant bits. It should be marginally faster */
#define isNaN(X)	( (I3Ed(X)->s.e==NAN_VAL) && ((I3Ed(X)->s.f1) || (I3Ed(X)->s.f2)) )
#define isfpNaN(X)	( (sizeof(X)==sizeof(float))? \
		((I3Edf(X)->s.e==0xff) && (I3Edf(X)->s.f) ) :\
		((I3Ed(X)->s.e==NAN_VAL) && ((I3Ed(X)->s.f1) || (I3Ed(X)->s.f2)) ) \
	)

#	define INF(X)	(((IEEEfp *)&(X))->s.e==NAN_VAL &&\
	((IEEEfp *)&(X))->s.f1== 0 &&\
	((IEEEfp *)&(X))->s.f2== 0)

#define fpINF(X)	( (sizeof(X)==sizeof(float))? \
		(((IEEEsfp *)&(X))->s.e==0xff &&\
			((IEEEsfp *)&(X))->s.f== 0 ) : \
		(((IEEEfp *)&(X))->s.e==NAN_VAL &&\
			((IEEEfp *)&(X))->s.f1== 0 &&\
			((IEEEfp *)&(X))->s.f2== 0) \
	)

#define set_NaN(X)	{IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
	local_IEEEfp->s.s= 0; local_IEEEfp->s.e=NAN_VAL ;\
	local_IEEEfp->s.f1= 0xfffff ;\
	local_IEEEfp->s.f2= 0xffffffff;}

#define set_fpNaN(X)	if( sizeof(X) == sizeof(float) ){ \
		IEEEsfp *local_IEEEsfp=(IEEEsfp*)(&(X));\
		local_IEEEsfp->s.s= 0; local_IEEEsfp->s.e=0xff ;\
		local_IEEEsfp->s.f= 0xffffff ;\
	} \
	else{ \
		IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
		local_IEEEfp->s.s= 0; local_IEEEfp->s.e=NAN_VAL ;\
		local_IEEEfp->s.f1= 0xfffff ;\
		local_IEEEfp->s.f2= 0xffffffff; \
	}

#define Inf(X)	((INF(X))?((((IEEEfp *)&(X))->l.high==POS_INF)?1:-1):0)

#define fpInf(X)	( (sizeof(X)==sizeof(float))? \
		(fpINF(X))?((((IEEEsfp *)&(X))->s.s)?-1:1):0 :\
		(INF(X))?((((IEEEfp *)&(X))->l.high==POS_INF)?1:-1):0 \
	)

#define set_Inf(X,S)	{IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
	local_IEEEfp->l.high=(S>0)?POS_INF:NEG_INF ;\
	local_IEEEfp->l.low= 0x00000000 ;}

#define set_fpInf(X,S)	if( sizeof(X)==sizeof(float) ){ \
		IEEEsfp *local_IEEEsfp=(IEEEsfp*)(&(X));\
		local_IEEEsfp->s.e=0xff ;\
		local_IEEEsfp->s.s= (S>0)? 0 : 1 ;\
		local_IEEEsfp->s.f= 0 ;\
	} \
	else{ \
		IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
		local_IEEEfp->l.high=(S>0)?POS_INF:NEG_INF ;\
		local_IEEEfp->l.low= 0x00000000 ;\
	}

#define set_INF(X)	{IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
	local_IEEEfp->s.e=NAN_VAL ;\
	local_IEEEfp->s.f1= 0x00000 ;\
	local_IEEEfp->s.f2= 0x00000000 ;}

#define set_fpINF(X)	if( sizeof(X) == sizeof(float) ){ \
		IEEEsfp *local_IEEEsfp=(IEEEsfp*)(&(X));\
		local_IEEEsfp->s.e=0xff ;\
		local_IEEEsfp->s.s= 0 ;\
		local_IEEEsfp->s.f= 0x000000 ;\
	} \
	else{ \
		IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
		local_IEEEfp->s.e=NAN_VAL ;\
		local_IEEEfp->s.f1= 0x00000 ;\
		local_IEEEfp->s.f2= 0x00000000 ;\
	}

#endif	/* !_NAN_H	*/
