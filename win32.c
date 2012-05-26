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

char *ttyname(int fd)
{ static buf[64];
	sprintf( buf, "tty%d:", fd );
	return(buf);
}

#include "regex.c"
#include "drand48.c"
