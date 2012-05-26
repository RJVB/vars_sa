#ifndef _LOWLEVEL_TIMER_H

/****************************************************************************/
/*                                  Timers                                  */
/*  From fftw 2.1.5	*/
/* Adapted by RJVB */
/*
 \ 20040817
 \ Unified interface to access lowlevel (CPU) timers. Under linux-x86 and
 \ under Mac OS X, we know how to handle these fast, precise CPU timers. On
 \ other platforms, we fall back to using gettimeofday().
 \ In any case:
 \ ** init_lowlevel_time() must be called before these routines return meaningful results
 \ ** these routines can only be used to determine intervals: there beginpoint is
 \	not defined in any (cross-platform) consistent manner.
 */
/****************************************************************************/

 
extern double lowlevel_clock_calibrator, lowlevel_clock_ticks;
extern int init_lowlevel_time();

/*
 \ get_lowlevel_time(): return a time value, measured in seconds.
 \ get_lowlevel_time2(): idem, but also sets lowlevel_clock_ticks to the corresponding clocktick value.
 \ 	Just the setting of that extra variable can cause a 3x slowdown of the call!
 */

#if defined(__GNUC__) && defined(__i386__)

/*
 * Use internal Pentium register (time stamp counter). Resolution
 * is 1/CLOCK_FREQUENCY seconds (e.g. 5 ns for Pentium 200 MHz).
 * (This code was contributed by Wolfgang Reimer)
 * In this implementation, almost 15x faster than gettimeofday() (which has similar resolution).
 */

	typedef unsigned long long lowlevel_clock_tick;

	static __inline__ lowlevel_clock_tick read_tsc()
	{ lowlevel_clock_tick ret;

		__asm__ __volatile__("rdtsc": "=A" (ret)); 
		/* no input, nothing else clobbered */
		return ret;
	}

#	define get_lowlevel_time()	(read_tsc() * lowlevel_clock_calibrator)
#	define get_lowlevel_time2()	((lowlevel_clock_ticks= read_tsc()) * lowlevel_clock_calibrator)

#elif defined(__GNUC__) && (defined(__MACH__) || defined(__APPLE_CC__))

#	include <mach/mach_time.h>

	typedef uint64_t lowlevel_clock_tick;

#	define get_lowlevel_time()	(mach_absolute_time() * lowlevel_clock_calibrator)
#	define get_lowlevel_time2()	((lowlevel_clock_ticks= mach_absolute_time()) * lowlevel_clock_calibrator)

#else

#	include <sys/time.h>
#	include <unistd.h>

	typedef unsigned long long lowlevel_clock_tick;

	static __inline__ lowlevel_clock_tick read_timeofday()
	{ struct timeval tv; struct timezone tz;
		gettimeofday(&tv, &tz);
		return( tv.tv_sec * 1000000 + tv.tv_usec );
	}

#	define get_lowlevel_time()	(read_timeofday() * lowlevel_clock_calibrator)
#	define get_lowlevel_time2()	((lowlevel_clock_ticks= read_timeofday()) * lowlevel_clock_calibrator)

#endif

#define _LOWLEVEL_TIMER_H
#endif
