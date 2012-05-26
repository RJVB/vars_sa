#ifndef _VARS_STANDALONE_H

/* ??!! */
#ifndef VARS_STANDALONE
#	define VARS_STANDALONE
#endif

#ifdef VARS_STANDALONE

#ifndef _STDIO_H
#	include <stdio.h>
#	ifndef _STDIO_H
#		define _STDIO_H
#	endif
#endif
#ifndef _TIME_H
#	include <sys/types.h>
#	ifndef WIN32
#		include <sys/times.h>
#	endif
#	include <time.h>
#	ifndef _TIME_H
#		define _TIME_H
#	endif
#endif

#include <ctype.h>

#ifndef WIN32
#	if !defined(__APPLE_CC__) && !defined(__MACH__)
#		include <termio.h>
#		include <values.h>
#	endif
#	include <sys/ioctl.h>
#	include <unistd.h>
#	include <dlfcn.h>
#endif

/* #include <stdarg.h>	*/
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
/* #include <curses.h>	*/

#include <fcntl.h>
#include <sys/stat.h>

#ifndef __sys_types_h
#	include <sys/types.h>
#endif

#include <signal.h>
#include <setjmp.h>

#include "cpu.h"

#include "cxerrno.h"


DEFUN( *CX_Time, (), char );

#define Flush_File(fp)	fflush(fp)
static int Delay_Flush;

#define cx_stderr	stderr
#define vars_errfile	stderr

#define DEFUN(Func,ArgList,Type)	extern Type Func ArgList
#define DEFMETHOD(Func,ArgList,Type)	Type (*Func)ArgList

/* memory management routines:	*/

DEFUN( *lib_calloc, (size_t n, size_t s), void);
DEFUN( lib_free, (void *p), void);

#ifdef VARS_STANDALONE_INTERNAL
#	define free(p)	lfree((p))
#	define calloc(n,s)	lcalloc((n),(s))
#endif

/* Disposable facilities:	*/
typedef struct disposable{	
	pointer car;
	long id;
	unsigned long size;
	struct disposable *head, *cdr;
} Disposable;				

	/* functions for managing user-allocated memory:
	 * all Disposable memory freed by Dispose_All_Disposables()
	 * which is installed as a onexit() handler.
	 */
DEFUN( *Find_Disposable,	(pointer x),	Disposable );
DEFUN( Enter_Disposable,	(pointer x, unsigned long size),	int );
DEFUN( Remove_Disposable,	(pointer x),	pointer );
DEFUN( Dispose_Disposable,	(pointer x),	pointer );
DEFUN( Dispose_All_Disposables,	(),	int );
DEFUN( List_Disposables,	(FILE *fp),	int );

typedef struct RootPointer{
	int MaxItemSize, MaxItems;
	int ItemSize, Items;
	struct RootPointer *RootPointer;
	unsigned char *UserMem;
} RootPointer;

typedef struct Remember{
	struct Remember *NextRemember;
	unsigned long RememberSize;
	pointer Memory, UserMemory;
	unsigned long item_len, item_nr;
	long serial;
	int items;
	struct rememberlist *RememberList;
} RememberKey;

typedef struct rememberlist{
	struct rememberlist *prev, *next;
	long serial;
	struct Remember *RmKey;
} RememberList;

/* #define RootPointer_Length(rp)	((rp)?((*)rp)[-1]:0)	*/
/* #define RootPointer_ISize(rp)	((rp)?((int*)rp)[-2]:0)	*/
/* #define RootPointer_Extra(rp)	(2*sizeof(unsigned long)+4*sizeof(int))	*/
/* #define RootPointer_Base(rp)	((pointer) &(((int*)rp)[-4]))	*/

#define RootPointer__Base(rp)	((RootPointer*)(((RootPointer**)rp)[-2]))
#define RootPointer_Base(rp)	((rp)?RootPointer__Base(rp):NULL)
#define RootPointer_Length(rp)	((rp)?RootPointer__Base(rp)->Items:0)
#define RootPointer_MaxLength(rp)	((rp)?RootPointer__Base(rp)->MaxItems:0)
#define RootPointer_ISize(rp)	((rp)?RootPointer__Base(rp)->ItemSize:0)
#define RootPointer_MaxISize(rp)	((rp)?RootPointer__Base(rp)->MaxItemSize:0)
#define RootPointer_Extra(rp)	(sizeof(RootPointer)+2*sizeof(unsigned long))
#define RootPointer_Size(rp)	((unsigned long)\
	(RootPointer_Extra(rp)+RootPointer_Length(rp)*RootPointer_ISize(rp)))
/* 	(RootPointer_Extra(rp)+2*sizeof(int)+RootPointer_Length(rp)*RootPointer_ISize(rp)))	???	*/

#define ROOTPOINTER_FAILURES 5
extern char *RootPointer_Failure[ROOTPOINTER_FAILURES];

DEFUN( init_RootPointer,	(char *ptr, int n, int itemsize, unsigned long size, int EnterDisposable),	pointer );
DEFUN( new_RootPointer_Key,	(RememberKey **rmkey, int n, int itemsize),	pointer );
DEFUN( new_RootPointer,	(int n, int itemsize),	pointer );
DEFUN( renew_RootPointer, (pointer *rp, int n, int itemsize, DEFMETHOD( dispose_method, (pointer rp), int)),	pointer );

DEFUN( RootPointer_Check,	(pointer p),	int );
DEFUN( RootPointer_CheckLength,	(pointer p, int n, char *mesg),	int );
DEFUN( _RootPointer_Length,	(pointer x),	int );
DEFUN( _RootPointer_ISize,	(pointer x),	int );
DEFUN( long _RootPointer_Size,	(pointer x),	unsigned );
DEFUN( *_RootPointer_Base,	(pointer x),	RootPointer );
DEFUN( print_RootPointer, ( FILE *fp, pointer p, int IsARootPointer ), int);

#	define LCALLOC 0x9abcdef0L
#	define LFREED  0xcba90fedL
	extern RememberList *CX_callocList, *CX_callocListTail;
	extern unsigned long CX_calloced, CX_callocItems;
	extern short lfree_alien;

	extern char Lfree_Alien;

#	define FREE(x)	{char la= Lfree_Alien;Lfree_Alien=1;free(x);Lfree_Alien=la;}

	DEFUN( __CX_FreeRemember, ( struct Remember **rmkey, long de_allocmem, int (*pre_fun)(), char *msg), unsigned long);
void* CX_AllocRemember( struct Remember **rmkey, unsigned long n, unsigned long dum);
unsigned long CX_FreeRemember( struct Remember **rmkey, long de_allocmem);

DEFUN( lcalloc, (unsigned long n, unsigned long s), pointer );
DEFUN( lfree, (pointer ptr), unsigned long );

#define TRACENAMELENGTH	64
#define TRACENAMELENGTH_1	TRACENAMELENGTH-1

#ifndef __FUNC__
#	define __FUNC__ "{unknown function}"
#endif
#ifndef __TIME__
#	define __TIME__ "(GOD knows when)"
#endif

typedef struct cx_trace_stack_item{
	struct cx_trace_stack_item *next;
	int depth, line;
	char file[TRACENAMELENGTH], cctime[TRACENAMELENGTH], func[TRACENAMELENGTH];
} trace_stack_item;

extern trace_stack_item *CX_Trace_StackBase, *CX_Trace_StackTop;
extern int Max_Trace_StackDepth;
DEFUN( Init_Trace_Stack, ( int max_depth ), int);
DEFUN( push_trace_stack, ( char *file, char *cctime, char* func, int line ), int);
	/* returns stacklevel of new item, -1 on error	*/
DEFUN( swap_trace_stack, ( char *file, char *cctime, char* func, int line ), int);
	/* returns stacklevel of refreshed top-item, -1 on error	*/
DEFUN( pop_trace_stack, ( int n_times ), int);
	/* returns new depth of stacktop	*/
DEFUN( pop_trace_stackto, ( int inclusive_level ), int);
	/* returns new depth of stacktop	*/
DEFUN( print_trace_stack, ( FILE *pf, unsigned int depth, char *msg ), int);

#ifdef VARS_DEBUG
#	define POP_TRACE(depth)	fprintf( stderr, "popto(%s,%d)=%d\n",\
		CX_Trace_StackTop->func, depth-1, pop_trace_stackto(depth-1))
#	define PUSH_TRACE(fun,newdepth)	fprintf( stderr, "push(%s)=%d\n",\
		fun, newdepth=push_trace_stack(__FILE__,CX_Time(),fun,__LINE__))
#else
#	define POP_TRACE(depth)	pop_trace_stackto(depth-1)
#	define PUSH_TRACE(fun,newdepth)	newdepth=push_trace_stack(__FILE__,CX_Time(),fun,__LINE__)
#endif

#define streq(a,b)	!strcmp(a,b)
#define strneq(a,b,n)	!strncmp(a,b,n)
#define strcaseeq(a,b)	!strcasecmp(a,b)
#define strncaseeq(a,b,n)	!strncasecmp(a,b,n)

DEFUN( *Open_NullFile, (), FILE);
DEFUN( Close_File, (FILE *fp, int final), int);
DEFUN( *Open_File, (char *name, char *mode), FILE);
DEFUN( *Open_Pipe_To, (char *command, FILE *alternative ), FILE);
DEFUN( *Open_Pipe_From, (char *command, FILE *alternative ), FILE);
DEFUN( Close_Pipe, (FILE *pipe), int);

DEFUN( *GetEnv, ( char *name), char);
DEFUN( *SetEnv, ( char *name, char *value), char);
#define getenv(n) GetEnv(n)
#define setenv(n,v) SetEnv(n,v)
extern char *EnvDir;						/* ="tmp:"; variables here	*/

#include "Macros.h"
#undef lfree
#undef lcalloc

#if !defined(IDENTIFY)

#ifndef SWITCHES
#	ifdef DEBUG
#		define _IDENTIFY(s,i)	static char *id_string= "$Id: @(#) " __FILE__ "'-[" __DATE__ "," __TIME__ "]-(\015\013\t\t" s "\015\013\t) DEBUG version" i " $"
#	else
#		define _IDENTIFY(s,i)	static char *id_string= "$Id: @(#) " __FILE__ "'-[" __DATE__ "," __TIME__ "]-(\015\013\t\t" s "\015\013\t)" i " $"
#	endif
#else
  /* SWITCHES contains the compiler name and the switches given to the compiler.	*/
#	define _IDENTIFY(s,i)	static char *id_string= "$Id: @(#) " __FILE__ "'-[" __DATE__ "," __TIME__ "]-(\015\013\t\t" s "\015\013\t)["SWITCHES"]" " $"
#endif

#define __IDENTIFY(s,i)\
static char *id_string_stub(){ _IDENTIFY(s,i);\
	static char called=0;\
	if( !called){\
		called=1;\
		return(id_string_stub());\
	}\
	else{\
		called= 0;\
		return(id_string);\
	}}

#ifdef __GNUC__
#	define IDENTIFY(s)	__attribute__((used)) __IDENTIFY(s," (gcc)")
#else
#	define IDENTIFY(s)	__IDENTIFY(s," (cc)")
#endif

#endif

typedef enum objecttypes {
	_nothing= 0,
	/* basic types:	*/
	_file, _function, _char, _string, _short, _int, _long, _float, _double, _variable,
	_charP, _shortP, _intP, _longP, _floatP, _doubleP, _functionP, _rootpointer,

	/* PatchWorks Types:	*/
	_something, _patch, _neuron, _eye, _paddler,
	_glowball, _target, _worldLayer, _paddlersnout, _paddlerretina,
	_paddlerbrain, _paddlermechrec, _paddlepos, _paddleosc,

	_simplestats, _simpleanglestats,
	_leaky_int_par,

	/* xlisp NODE:	*/
	_xlisp_node,

	/* Maximum value:	*/
	MaxObjectType
} ObjectTypes;
extern char *ObjectTypeNames[MaxObjectType];

#ifndef STRING
#	define STRING(name)	# name
#endif

#define TRUE	1
#define FALSE	0

extern unsigned long calloc_size;

#define Writelog(expr,string)	{sprintf expr; fputs(string,stderr);}
#define Appendlog(expr,string)	{sprintf expr; fputs(string,stderr);}
#define logfp	stderr

#ifdef SYMBOLTABLE
#	include "SymbolTable.h"
#endif

#ifdef WIN32
#	define onexit	cx_onexit

#	include "win32.h"

#endif

#if defined(CX_SOURCE) || defined(VARS_SOURCE)
#	include "CX_writable_strings.h"
#endif

extern int CX_writable_strings;

extern int EndianType;
extern int CheckEndianness();

extern char *concat2( char *first_target, ...);
extern char *concat( char *first, ...);

extern void *CX_dlopen(const char *libname, int flags);

#define _VARS_STANDALONE_H
#endif
#endif
