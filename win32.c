#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#ifdef VARS_STANDALONE
#	include "vars-standalone.h"
#endif

int strcasecmp(const char *a, const char *b)
{
/*   int ret= 0; 	*/
/* 	while( *a && *b ){	*/
/* 		ret+= tolower(*a) - tolower(*b);	*/
/* 		a++, b++;	*/
/* 	}	*/
/* 	return(ret);	*/
	return( _stricmp(a,b) );
}

int strncasecmp(const char *a, const char *b, size_t n)
{
	return( _strnicmp(a,b,n) );
}

// MSCV's snprintf does not append a nullbyte if it has to truncate the string to add to the destination.
// Crashes can result, so we proxy the function and add the nullbyte ourselves if the return value indicates
// truncation. NB: POSIX snprintf does not return -1 when it truncates (at least not the version on Mac OS X).
int snprintf( char *buffer, size_t count, const char *format, ... )
{ int n;
	va_list ap;
	va_start( ap, format );
	n = _vsnprintf( buffer, count, format, ap );
	if( n < 0 ){
		buffer[count-1] = '\0';
	}
	va_end(ap);
	return n;
}

char *ttyname(int fd)
{ static buf[64];
	sprintf( buf, "tty%d:", fd );
	return(buf);
}

#include "regex.c"
#include "drand48.c"
