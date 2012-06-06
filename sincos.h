#ifndef _SINCOS_H

#include <math.h>
#ifdef VARS_STANDALONE
#	include "mathdef.h"
#	include "Macros.h"
#	include "NaN.h"
#else
#	include "local/mathdef.h"
#	include "local/Macros.h"
#	include "local/NaN.h"
#endif

extern double cxsin( double x, double base );
extern double cxcos( double x, double base );
extern void cxsincos( double x, double base, double *sr, double *cr );

#endif
