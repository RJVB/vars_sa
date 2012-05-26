#ifndef _CXERRNO_H
#define _CXERRNO_H

#include <errno.h>

#ifdef __cplusplus
	extern "C" {
#endif

#define	EOPENLIBRARY	sys_nerr
#define	EOPENSCREEN	sys_nerr+1
#define	EOPENWINDOW	sys_nerr+2
#define	EALLOCMEMORY	sys_nerr+3
#define	EINITTMPRAS	sys_nerr+4
#define	EALLOCSCRATCH	EINITTMPRAS
#define	ELFREEALIEN	sys_nerr+5
#define	EUNAVAILABLE	sys_nerr+6
#define	ESINCISNULL	sys_nerr+7
#define	EILLSINCTYPE	sys_nerr+8
#define	ENOXTERM		sys_nerr+9
#define	EALIENERRNO	sys_nerr+10
#define	ENOSUCHPAR	sys_nerr+11
#define	EVARILLID		sys_nerr+12
#define	EVARNONAME	sys_nerr+13
#define	EVARILLTYPE	sys_nerr+14
#define	EVARNOCOMMAND	sys_nerr+15
#define	EVARNULL		sys_nerr+16
#define	ENOSUCHVAR	sys_nerr+17
#define	EVARILLSYNTAX	sys_nerr+18
#define	EVARRDONLY	sys_nerr+19
#define	EDIVZERO		sys_nerr+20
#define	EINDEXRANGE	sys_nerr+21
#define	ELFREETWICE	sys_nerr+22

extern char *cx_sys_errlist[];
extern int cx_sys_nerr;
extern int sys_errno;

#ifdef __cplusplus
	}
#endif
#endif	/* _CXERRNO_H	*/
