#ifndef _CXTRIGO_H

#if defined(__MACH__) || defined(__APPLE_CC__)

#	define NEEDS_SINCOS	1

	extern void initsincos();

	extern void sincosf(float angle, float *sinus, float *cosinus );
	extern void sincos(double angle, double *sinus, double *cosinus );

#endif

#define sincosdeg(a,s,c)	sincos(radians(a),(s),(c))

#define _CXTRIGO_H
#endif
