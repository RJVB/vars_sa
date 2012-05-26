#include <stdio.h>
#include "lowlevel_timer.h"

#if defined(__GNUC__) && defined(__i386__)
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

IDENTIFY("CPU-based lowlevel timer interface");

/* See the comments in lowlevel_timer.h */

double lowlevel_clock_calibrator= 1, lowlevel_clock_ticks= 0;

int init_lowlevel_time()
{
#if defined(linux)
  FILE *fp= fopen("/proc/cpuinfo", "r");
  int ok= 0;
	if( fp ){
	  char buf[256];
		while( !ok && fgets( buf, sizeof(buf)-1, fp ) ){
			if( strncasecmp(buf, "cpu mhz", 7)== 0 ){
			  char *c= buf;
				while( *c && !isdigit(*c) ){
					c++;
				}
				if( sscanf(c, "%lf", &lowlevel_clock_calibrator) == 1 && lowlevel_clock_calibrator> 0 ){
/* 					fprintf( stderr, "lowlevel_clock_calibrator=1/%g\n", lowlevel_clock_calibrator*1e6 );	*/
					lowlevel_clock_calibrator= 1.0 / (lowlevel_clock_calibrator * 1e6);
					ok= 1;
				}
			}
		}
		fclose(fp);
	}
	if( !ok )
#endif
#if defined(__GNUC__) && defined(__i386__)

	{ double cpu_mhz, CPU_mhz= 0;
	  lowlevel_clock_tick tsc_start, tsc_end,
		cycles= 0, delays= 0;
	  struct timeval tv_start, tv_end;
	  long usec_delay;
	  int i;

		tsc_start= read_tsc();
		gettimeofday(&tv_start, NULL);
		for( i= 0; i< 5; i++ ){
			do{
				gettimeofday(&tv_end, NULL);
				usec_delay = 1000000L * (tv_end.tv_sec - tv_start.tv_sec) +
								    (tv_end.tv_usec - tv_start.tv_usec);
			} while( usec_delay< 100000L/5 );
			tsc_end= read_tsc();
			cycles += (tsc_end-tsc_start);
			delays += usec_delay;
		}
		lowlevel_clock_calibrator = (double) delays / (double) cycles * 1e-6;
	}

#elif defined(__GNUC__) && (defined(__MACH__) || defined(__APPLE_CC__))

	{ struct mach_timebase_info timebase;

		mach_timebase_info(&timebase);
		lowlevel_clock_calibrator= (double)timebase.numer / (double)timebase.denom * 1e-9;
	}

#else

	lowlevel_clock_calibrator= 1e-6;

#endif
}
