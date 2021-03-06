/* ****************
 * Macros.h: some useful macros
 * (C)(R) R. J. Bertin
 * :ts=4
 * ****************
 */

#ifndef _MACRO_H
#define _MACRO_H

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef _CPU_H
#	include "cpu.h"
#endif

#include <stddef.h>
#include <stdlib.h>
#include <limits.h>

/* Original SUN macro.h file included for convenience: */

#ifndef __cplusplus
#	ifdef NULL
#		undef NULL
#	endif
#	define NULL    ((void*)0)
#endif

#define reg     register

#define MAXUSHORT	USHRT_MAX
#ifndef MAXSHORT
#define MAXSHORT	SHRT_MAX
#endif
#ifndef MAXINT
#define MAXINT		INT_MAX		/* works with 16 and 32 bit ints	*/
#define MAXUINT	UINT_MAX
#define MAXLONG	LONG_MAX
#endif
#define MAXULONG	ULONG_MAX

#define SETBIT(s,n)     (s[(n)/16] |=  (1 << ((n) % 16)))
#define XORBIT(s,n)     (s[(n)/16] ^=  (1 << ((n) % 16)))
#define CLRBIT(s,n)     (s[(n)/16] &= ~(1 << ((n) % 16)))
#define TSTBIT(s,n)     (s[(n)/16] &   (1 << ((n) % 16)))

#ifndef LOOPDN
#define LOOPDN(v, n)    for ((v) = (n); --(v) != -1; )
#endif

#define DIM(a)          (sizeof(a) / sizeof(a[0]))

#define MOD(v,d)  ( (v) >= 0  ? (v) % (d) : (d) - ( (-(v))%(d) ) )
#define ABS(v)    ((v)<0?-(v):(v))
#define FABS(v)   (((v)<0.0)?-(v):(v))
#ifndef SIGN
#	define SIGN(x)		(((x)<0)?-1:1)
#endif
#define ODD(x)		((x) & 1)

#define BITWISE_AND(a,b)	((a)&(b))
#define BITWISE_OR(a,b)	((a)|(b))
#define BITWISE_XOR(a,b)	((a)^(b))
#define BITWISE_NOT(a)	(~(a))

#define LOGIC_AND(a,b)	((a)&&(b))
#define LOGIC_OR(a,b)	((a)||(b))
#define LOGIC_XOR(a,b)	(((int)(a)!=0)^((int)(b)!=0))
#define LOGIG_NOT(a)	(!(a))

#ifdef DEBUG
#define ASSERTS(cond) \
   {if (!(cond)) \
       printf("Assertion failure: line %d in %s\n", __LINE, __FILE);   }
#else
#define ASSERTS(cond)
#endif


/********************* Macros to access linear arrays as MultiDimensional ****/
/*                     Usage: map?d( x1,..,xn, Size(x1),..,Size(xn) )        */
/*                     !!! map?d() return a long argument !!!                */

#define map2d(x,y,X,Y)          ((long)y*X+(long)x)
#define map3d(x,y,z,X,Y,Z)      (((long)z*Y+(long)y)*X+(long)x)
#define map4d(a,b,c,d,A,B,C,D)  ((((long)d*C+(long)c)*B+(long)b)*A+(long)a)

/*                      Macros to allocate (multidim) arrays                */
/*          Usage: if( calloc??_error(pointer,type,size1,..,sizen)) ERROR   */

#ifdef _MAC_THINKC_
#define lcalloc clalloc
	char *lcalloc();
#endif

#ifndef lfree
/* cx.h has not yet been included	*/
#define lcalloc calloc
#endif


#define calloc_error(p,t,n)  ((p=(t*)lcalloc((unsigned long)(n),(unsigned long)sizeof(t)))==(t*)0)
#define calloc2d_error(p,t,x,y)  ((p=(t*)lcalloc((unsigned long)(x*y),(unsigned long)sizeof(t)))==(t*)0)
#define calloc3d_error(p,t,x,y,z)  ((p=(t*)lcalloc((unsigned long)(x*y*z),(unsigned long)sizeof(t)))==(t*)0)
#define calloc4d_error(p,t,a,b,c,d)  ((p=(t*)lcalloc((unsigned long)(a*b*c*d),(unsigned long)sizeof(t)))==(t*)0)

#define lcalloc_error calloc_error
#define lcalloc2d_error calloc2d_error
#define lcalloc3d_error calloc3d_error
#define lcalloc4d_error calloc4d_error

/*                  Miscellaneous Macros                                    */

#ifndef MAX
#define MAX(a,b)                (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)                (((a)<(b))?(a):(b))
#endif
#define MAXp(a,b)				(((a)&&(a)>(b))?(a):(b))	/* maximal non-zero	*/
#define MINp(a,b)				(((a)&&(a)<(b))?(a):(b))	/* minimal non-zero	*/

#define ProgName				(argv[0])

#if defined(i386) || defined(__i386__)
#	define __ARCHITECTURE__	"i386"
#elif defined(__x86_64__) || defined(x86_64) || defined(_LP64)
#	define __ARCHITECTURE__	"x86_64"
#elif defined(__ppc__)
#	define __ARCHITECTURE__	"ppc"
#else
#	define __ARCHITECTURE__	""
#endif

#ifndef SWITCHES
#	ifdef DEBUG
#		define _IDENTIFY(s,i)	"$Id: @(#) '" __FILE__ "'-[" __DATE__ "," __TIME__ "]-(\015\013\t\t" s "\015\013\t) DEBUG version" __ARCHITECTURE__" " i " $"
#	else
#		define _IDENTIFY(s,i)	"$Id: @(#) '" __FILE__ "'-[" __DATE__ "," __TIME__ "]-(\015\013\t\t" s "\015\013\t)" __ARCHITECTURE__" " i " $"
#	endif
#else
  /* SWITCHES contains the compiler name and the switches given to the compiler.	*/
#	define _IDENTIFY(s,i)	"$Id: @(#) '" __FILE__ "'-[" __DATE__ "," __TIME__ "]-(\015\013\t\t" s "\015\013\t)["__ARCHITECTURE__" "SWITCHES"] $"
#endif

#ifdef PROFILING
#	define __IDENTIFY(s,i)\
	static char *ident= _IDENTIFY(s,i);
#else
#	define __IDENTIFY(s,i)\
	static char *ident_stub(){ static char *ident= _IDENTIFY(s,i);\
		static char called=0;\
		if( !called){\
			called=1;\
			return(ident_stub());\
		}\
		else{\
			called= 0;\
			return(ident);\
		}}
#endif

#ifdef __GNUC__
#	define IDENTIFY(s)	__attribute__((used)) __IDENTIFY(s," (gcc)")
#else
#	define IDENTIFY(s)	__IDENTIFY(s," (cc)")
#endif

#ifndef CLIP
#	define CLIP(var,low,high)	if((var)<(low)){\
		(var)=(low);\
	}else if((var)>(high)){\
		(var)=(high);}
#endif
#define CLIP_EXPR(var,expr,low,high)	{ double l, h; if(((var)=(expr))<(l=(low))){\
	(var)=l;\
}else if((var)>(h=(high))){\
	(var)=h;}}

#define CLIP_IEXPR(var,expr,low,high)	{ double e= (expr), l= (low), h= (high);\
	if( e< l ){ \
		e= l; \
	} \
	else if( e> h ){ \
		e= h; \
	} \
	(var)= (int) e; \
}

#ifndef CLIP_EXPR_CAST
/* A safe casting/clipping macro.	*/
#	define CLIP_EXPR_CAST(ttype,var,stype,expr,low,high)	{stype clip_expr_cast_lvalue=(expr); if(clip_expr_cast_lvalue<(low)){\
		(var)=(ttype)(low);\
	}else if(clip_expr_cast_lvalue>(high)){\
		(var)=(ttype)(high);\
	}else{\
		(var)=(ttype)clip_expr_cast_lvalue;}}
#endif

#if 0
#ifdef True
#	undef True
#endif
#ifdef False
#	undef False
#endif

#ifndef NO_ENUM_BOOLEAN
#	ifdef Boolean
#		undef Boolean
#	endif
	typedef enum Boolean { False, True} Boolean;
#else
/* 	enum Boolean { False, True};	*/
/* #	define Boolean	enum Boolean	*/
#	ifdef Boolean
#		undef Boolean
#	endif

#	ifndef _XtIntrinsic_h
		typedef enum Boolean { False, True} Boolean;
#	else
#		define True 1
#		define False 0
#	endif
#endif
#else
	
#ifndef True
#	define True 1
#	define False 0
#endif

#endif

#ifdef MCH_AMIGA
#	define sleep(x)	Delay(60*x)
#endif

#include <errno.h>

#if !defined(linux) && !defined(__APPLE_CC__) && !defined(__MACH__) && !defined(__CYGWIN__) && !defined(_MSC_VER)
extern int sys_nerr;
extern char *sys_errlist[];
#endif
#ifdef _MSC_VER
#	define sys_nerr	_sys_nerr
#endif

#define SYS_ERRLIST	_sys_errlist
#define SYS_NERR	_sys_nerr

#ifndef _CX_H
// #	define serror() ((errno>0&&errno<sys_nerr)?sys_errlist[errno]:"invalid errno")
	static
#	if !defined(DEBUG) && !defined(_MSC_VER)
	inline
#	endif
	char *serror()
	{ extern char* strerror(int);
		if( errno>0&&errno<sys_nerr ){
			return strerror(errno);
		}
		else if( errno < sys_nerr + cx_sys_nerr ){
			return cx_sys_errlist[errno - sys_nerr];
		}
		else{
			return "invalid errno";
		}
	}
#endif

#define SinCos(a,s,c)	{(s)=sin((a));(c)=cos((a));}

#define degrees(a)		((a)*57.295779512)
#define radians(a)		((a)*(1.0/57.295779512))


/* some convenient typedef's:	*/

/* functionpointer (method) typedef's	*/

typedef char (*char_method)();

#ifndef _TRAPS_H
	typedef int (*int_method)();
#endif

typedef unsigned int (*uint_method)();
typedef short (*short_method)();
typedef unsigned short (*ushort_method)();
typedef long (*long_method)();
typedef unsigned long (*ulong_method)();
typedef void (*void_method)();
typedef void* (*pointer_method)();
typedef pointer_method (*method_method)();
typedef double (*double_method)();

/* typedef unsigned char pixel;	*/


#ifndef CheckMask
#	define	CheckMask(var,mask)	(((var)&(mask))==(mask))
#endif
#ifndef CheckExclMask
#	define	CheckExclMask(var,mask)	(((var)^(mask))==0)
#endif

#if __GNUC__ >= 3
// #	undef __pure
// #	define __pure         __attribute__ ((pure))
// #	undef __const
// #	define __const        __attribute__ ((const))
// #	define __noreturn     __attribute__ ((noreturn))
// #	define __malloc       __attribute__ ((malloc))
// #	define __must_check   __attribute__ ((warn_unused_result))
// #	ifndef __deprecated
// #		define __deprecated   __attribute__ ((deprecated))
// #	endif
// #	define __used         __attribute__ ((used))
// #	undef __unused
// #	define __unused       __attribute__ ((unused))
// #	define __packed       __attribute__ ((packed))
#	define likely(x)      __builtin_expect (!!(x), 1)
#	define unlikely(x)    __builtin_expect (!!(x), 0)
#else
// #	define __pure         /* no pure */
// #	define __const        /* no const */
// #	define __noreturn     /* no noreturn */
// #	define __malloc       /* no malloc */
// #	define __must_check   /* no warn_unused_result */
// #	define __deprecated   /* no deprecated */
// #	define __used         /* no used */
// #	define __unused       /* no unused */
// #	define __packed       /* no packed */
#	define likely(x)      (x)
#	define unlikely(x)    (x)
#endif

#ifdef __cplusplus
	}
#endif
#endif
