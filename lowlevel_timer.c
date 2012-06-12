#include <stdio.h>
#include "lowlevel_timer.h"

#if defined(__GNUC__) && (defined(__i386__) || defined(i386) || defined(x86_64) || defined(__x86_64__))
#	include <sys/time.h>
#	include <unistd.h>
#endif

#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#ifdef VARS_STANDALONE
#	include "cxerrno.h"
#	include "Macros.h"
#else
#	include "local/cxerrno.h"
#	include <local/Macros.h>
#endif

#if (defined(__MACH__) || defined(__APPLE_CC__))
#	include <mach/mach_time.h>
#endif

IDENTIFY("CPU-based lowlevel timer interface");

/* See the comments in lowlevel_timer.h */

double lowlevel_clock_calibrator= 1, lowlevel_clock_ticks= 0;
extern FILE *cx_stderr;

#ifdef __CYGWIN__
#	include <windows.h>

	double fget_lowlevel_time()
	{ LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		return count.QuadPart * lowlevel_clock_calibrator;
	}
	double fget_lowlevel_time2()
	{ LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		return (lowlevel_clock_ticks = (double) count.QuadPart) * lowlevel_clock_calibrator;
	}
#endif

int init_lowlevel_time()
{ int ok= 0;
#if (defined(__MACH__) || defined(__APPLE_CC__))

	{ struct mach_timebase_info timebase;

		mach_timebase_info(&timebase);
		lowlevel_clock_calibrator= ((double)timebase.numer / (double)timebase.denom) * 1e-9;
		ok = 1;
	}

#elif defined(_MSC_VER) || defined(__WATCOMC__) || defined(WIN32) || defined(__CYGWIN__)
	{ LARGE_INTEGER lpFrequency;
		if( !QueryPerformanceFrequency(&lpFrequency) ){
			lowlevel_clock_calibrator = 0;
		}
		else{
			lowlevel_clock_calibrator = 1.0 / ((double) lpFrequency.QuadPart);
		}
	}
#elif defined(linux)

	lowlevel_clock_calibrator = 1e-9;
	ok = 1;
#	if 0
	{ struct timespec hrt;
	  clockid_t cid;
	  int rcid = clock_getcpuclockid( 0, &cid );
#		ifdef CLOCK_MONOTONIC
		clock_getres( CLOCK_MONOTONIC, &hrt );
#		elif defined(CLOCK_REALTIME)
		clock_getres( CLOCK_REALTIME, &hrt );
#		endif
 		fprintf( cx_stderr, "lowlevel_clock_calibrator=%g clockid=%lu (%d), clock_gettime resolution = %gs\n",
 			lowlevel_clock_calibrator, cid, rcid, (hrt.tv_sec + hrt.tv_nsec * 1e-9) );
	}
#	endif
#else

	lowlevel_clock_calibrator= 1e-6;
	ok = -1;

#endif
	return ok;
}
