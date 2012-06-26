#if 0
/* Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc. */
/* This file is part of GNU Modula-2.

GNU Modula-2 is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GNU Modula-2 is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.   See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with gm2; see the file COPYING.   If not, write to the Free Software
Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

This file was originally part of the University of Ulm library
*/



/* Ulm's Modula-2 Library
    Copyright (C) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991,
    1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
    2002, 2003, 2004, 2005
    by University of Ulm, SAI, D-89069 Ulm, Germany
*/
#endif

#include <stdio.h>
#ifdef VARS_STANDALONE
#	include "cxerrno.h"
#	include "Macros.h"
#else
#	include "local/cxerrno.h"
#	include <local/Macros.h>
#endif
#include "sincos.h"
#ifndef __ppc__
#	define USE_SSE_AUTO
#	define SSE_MATHFUN_WITH_CODE
#	include "sse_mathfun/sse_mathfun.h"
#endif

#if 0
    PROCEDURE CopySign(x, y: REAL) : REAL;
	 /* return x with its sign changed to that of y */
    BEGIN
	 x := ABS(x);
	 IF y < 0.0 THEN
	RETURN -x
	 ELSE
	RETURN x
	 END;
    END CopySign;
#endif

static
#ifdef __GNUC__
inline
#endif
double CopySign( double x, double y )
{
	if( x< 0 ){
		x= -x;
	}
	return( (y<0)? -x : x );
}

#if 0
    PROCEDURE Finite(x: REAL) : BOOLEAN;
    BEGIN
	 RETURN ~IsNaN(x) & ~IsInf(x)
    END Finite;
#endif

static
#ifdef __GNUC__
inline
#endif
int Finite( double x )
{
	return( !NaNorInf(x) );
}

#define half	0.5

#define	thresh	2.6117239648121182150E-1
#define	PIo4	7.8539816339744827900E-1
#define	PIo2	1.5707963267948965580E0
#define	PI3o4	2.3561944901923448370E0
#define	PI	3.1415926535897931160E0
#define	PI2	6.2831853071795862320E0

#define	S0	-1.6666666666666463126E-1
#define	S1	8.3333333332992771264E-3
#define	S2	-1.9841269816180999116E-4
#define	S3	2.7557309793219876880E-6
#define	S4	-2.5050225177523807003E-8
#define	S5	1.5868926979889205164E-10

#define	C0	4.1666666666666504759E-2
#define	C1	-1.3888888888865301516E-3
#define	C2	2.4801587269650015769E-5
#define	C3	-2.7557304623183959811E-7
#define	C4	2.0873958177697780076E-9
#define	C5	-1.1250289076471311557E-11

#define	small	1.2E-8
#define	big	1.0E20

static
#ifdef __GNUC__
inline
#endif
double SinS( double x )
{
#if 0
	/* STATIC KERNEL FUNCTION OF SIN(X), COS(X), AND TAN(X)
	 * CODED IN C BY K.C. NG, 1/21/85;
	 * REVISED BY K.C. NG on 8/13/85.
	 *
	 *   		sin(x*k) - x
	 * RETURN	--------------- on [-PI/4,PI/4] ,
	 *   				x
	 *
	 * where k=pi/PI, PI is the rounded
	 * value of pi in machine precision:
	 *
	 *   	Decimal:
	 *   			pi = 3.141592653589793 23846264338327 .....
	 *      53 bits	PI = 3.141592653589793 115997963 ..... ,
	 *      56 bits	PI = 3.141592653589793 227020265 ..... ,
	 *
	 *   	Hexadecimal:
	 *   			pi = 3.243F6A8885A308D313198A2E....
	 *      53 bits	PI = 3.243F6A8885A30   =   2 * 1.921FB54442D18
	 *      56 bits	PI = 3.243F6A8885A308 =   4 * .C90FDAA22168C2
	 *
	 * Method:
	 *   	1. Let z=x*x. Create a polynomial approximation to
	 *   		(sin(k*x)-x)/x   =	z*(S0 + S1*z^1 + ... + S5*z^5).
	 *   	Then
	 *   	sin__S(x*x) = z*(S0 + S1*z^1 + ... + S5*z^5)
	 *
	 *   	The coefficient S's are obtained by a special Remez algorithm.
	 *
	 * Accuracy:
	 *   	In the absence of rounding error, the approximation has absolute error
	 *   	less than 2**(-61.11) for VAX D FORMAT, 2**(-57.45) for IEEE DOUBLE.
		 */
	 BEGIN
	RETURN x*(S0+x*(S1+x*(S2+x*(S3+x*(S4+x*S5)))))
	 END SinS;
#endif
	return( x*(S0+x*(S1+x*(S2+x*(S3+x*(S4+x*S5))))) );
}

static
#ifdef __GNUC__
inline
#endif
double CosC( double x )
{
#if 0
	/*
	 * STATIC KERNEL FUNCTION OF SIN(X), COS(X), AND TAN(X)
	 * CODED IN C BY K.C. NG, 1/21/85;
	 * REVISED BY K.C. NG on 8/13/85.
	 *
	 *   						x*x
	 * RETURN	cos(k*x) - 1 + ----- on [-PI/4,PI/4],	where k = pi/PI,
	 *   						 2
	 * PI is the rounded value of pi in machine precision :
	 *
	 *   	Decimal:
	 *   			pi = 3.141592653589793 23846264338327 .....
	 *      53 bits	PI = 3.141592653589793 115997963 ..... ,
	 *      56 bits	PI = 3.141592653589793 227020265 ..... ,
	 *
	 *   	Hexadecimal:
	 *   			pi = 3.243F6A8885A308D313198A2E....
	 *      53 bits	PI = 3.243F6A8885A30   =   2 * 1.921FB54442D18
	 *      56 bits	PI = 3.243F6A8885A308 =   4 * .C90FDAA22168C2
	 *
	 *
	 * Method:
	 *   	1. Let z=x*x. Create a polynomial approximation to
	 *   		cos(k*x)-1+z/2   =	z*z*(C0 + C1*z^1 + ... + C5*z^5)
	 *   	then
	 *   	cos__C(z) =   z*z*(C0 + C1*z^1 + ... + C5*z^5)
	 *
	 *   	The coefficient C's are obtained by a special Remez algorithm.
	 *
	 * Accuracy:
	 *   	In the absence of rounding error, the approximation has absolute error
	 *   	less than 2**(-64) for VAX D FORMAT, 2**(-58.3) for IEEE DOUBLE.
	 */
	 BEGIN
	RETURN x*x*(C0+x*(C1+x*(C2+x*(C3+x*(C4+x*C5)))))
	 END CosC;
#endif
	return( x*x*(C0+x*(C1+x*(C2+x*(C3+x*(C4+x*C5))))) );
}

#ifdef __GNUC__
inline
#endif
double cxsin( double x, double base )
{ double a, c, z;
	if( !Finite(x) ){
		return( x - x );
	}
	if( base== 0 ){
		base= PI2;
	}

	x= fmod(x * PI2/base, base); /* reduce x into [-base/2,base/2] */
	a = CopySign(x, 1.0);
	if( a >= PIo4 ){
		if( a >= PI3o4 ){ /* ... in [3PI/4,PI] */
		   a = PI - a;
		   x = CopySign(a, x);
		}
		else{ /* ... in [PI/4,3PI/4] */
		   a = PIo2 - a; /* rtn. sign(x)*C(PI/2-|x|) */
		   z = a * a;
		   c = CosC(z);
		   z *= half;
		   if( z >= thresh ){
		   	a = half - ((z - half) - c);
		   }
		   else{
		   	a = 1.0 - (z - c);
		   }
		   return CopySign(a, x);
		}
	}

	if( a < small ){ /* return S(x) */
		// tmp = big + a;
		return x;
	}

	return( x + x * SinS(x * x) );
}

#ifdef __GNUC__
inline
#endif
double cxcos( double x, double base )
{ double a, c, z, s= 1.0;
	if( !Finite(x) ){
		return( x - x );
	}
	if( base== 0 ){
		base= PI2;
	}
	
	x= fmod(x * PI2/base, base); /* reduce x into [-base/2,base/2] */
	a = CopySign(x, 1.0);
	if( a >= PIo4 ){
		if( a >= PI3o4 ){ /* ... in [3PI/4,PI] */
		   a = PI - a;
		   s = - 1.0;
		}
		else{			   /* ... in [PI/4,3PI/4] */
		   a = PIo2 - a;
		   return( a + a * SinS(a * a) ); /* rtn. S(PI/2-|x|) */
		}
	}
	if( a < small ){
		// tmp = big + a;
		return( s );/* rtn. s*C(a) */
	}

	z = a * a;
	c = CosC(z);
	z *= half;
	if( z >= thresh ){
		a = half - ((z - half) - c);
	}
	else{
		a = 1.0 - (z - c);
	}
	return( CopySign(a, s) );
}

#ifdef __GNUC__
inline
#endif
void cxsincos( double x, double base, double *sr, double *cr )
{ double a, c, z, s= 1, sx;
	if( !Finite(x) ){
		*sr = *cr= ( x - x );
		return;
	}
	if( base== 0 ){
		base= PI2;
	}
	
	x= sx= fmod(x * PI2/base, base); /* reduce x into [-base/2,base/2] */
#ifdef USE_SSE2
	{ v2df x2, s, c;
		x2 = _MM_SET1_PD(x);
		sincos_pd( x2, &s, &c );
		*sr = ((double*)&s)[0];
		*cr = ((double*)&c)[0];
	}
#else
	a = CopySign(x, 1.0);
	if( a >= PIo4 ){
		if( a >= PI3o4 ){ /* ... in [3PI/4,PI] */
			a = PI - a;
			sx = CopySign(a, x);
			s = -1;
		}
		else{ /* ... in [PI/4,3PI/4] */
			a = PIo2 - a; /* rtn. sign(x)*C(PI/2-|x|) */
			*cr = a + a * SinS(a * a);
			z = a * a;
			c = CosC(z);
			z *= half;
			if( z >= thresh ){
				a = half - ((z - half) - c);
			}
			else{
				a = 1.0 - (z - c);
			}
			*sr= CopySign(a, sx);
			return;
		}
	}

	if( a < small ){ /* return S(x) */
		// tmp = big + a;
		*sr=  x;
		*cr= 1;
		return;
	}

	*sr= ( sx + sx * SinS(sx * sx) );
	z = a * a;
	c = CosC(z);
	z *= half;
	if( z >= thresh ){
		a = half - ((z - half) - c);
	}
	else{
		a = 1.0 - (z - c);
	}
	*cr= CopySign(a, s);
#endif
	return;
}

