#ifndef _WIN32_H

#include <process.h>

#include "defun.h"

DEFUN( *re_comp, (char *p), char);
DEFUN( re_exec, (char *p), int);
DEFUN( *re_exec_len, (char *p), char);

extern int strcasecmp(const char *a, const char *b );
extern int strncasecmp(const char *a, const char *b, size_t n );
extern char *ttyname(int fd);

extern double drand48();
extern unsigned short * seed48(unsigned short seed16v[3]);

#define	index(s,c)	strchr((s),(c))
#define	rindex(s,c)	strrchr((s),(c))
#define popen(n,t)	_popen((n),(t))
#define pclose(f)	_pclose((f))

#ifndef i386
#	define i386
#endif

#define _WIN32_h
#endif
