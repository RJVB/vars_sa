#define _VARS_SOURCE

#ifdef VARARGS_OLD
#	ifdef __GNUC__
#		include <sys/types.h>
#		include <varargs.h>
#		define _STDARG_H
#	else
#		ifdef WIN32
#			include <stdarg.h>
#		else
#			include <varargs.h>
#		endif
#	endif
#else
#	include <sys/types.h>
#	include <stdarg.h>
#	define _STDARG_H
#endif

#ifndef _UNIX_C_
extern char *index();
#endif

#ifdef _MAC_MPWC_
#	define fgets _fgets
#endif

#define CHANGED_FIELD_OFFSET	(2*sizeof(unsigned short))
#define CHANGED_FIELD(vars)	(((vars)->changed)?(vars)->changed->flags:NULL)
#define CHANGED_FIELD_INDEX(vars,i)	(((vars)->changed)?(vars)->changed->flags[(i)]:0)

#define RESET_CHANGED_FLAG(ch,i) if(ch){\
		ch[i]= VAR_UNCHANGED;\
	}

#define SET_CHANGED_FLAG(ch,i,a,b,scanf_ret) if(ch){\
		if( scanf_ret== EOF)\
			ch[i]= VAR_UNCHANGED;\
		else if( a== b)\
			ch[i]= VAR_REASSIGN;\
		else if( a!= b)\
			ch[i]= VAR_CHANGED;\
	}

#define SET_CHANGED_FIELD(vars,i,val) {\
	(((vars)->changed)?(vars)->changed->flags[i]=(val):0); \
	if( var_label_change && ((val)== VAR_REASSIGN || (val)== VAR_CHANGED) ){ \
		(vars)->change_label= var_change_label; \
	} \
}

#define RESET_CHANGED_FIELD(v,i) SET_CHANGED_FIELD(v,i,VAR_UNCHANGED)

#define LEFT_BRACE	'{'
#define RIGHT_BRACE '}'
#define REFERENCE_OP	'$'
#define REFERENCE_OP2	'{'

#define SET_INTERNAL	"set internal"
#define SHOW_INTERNAL	"show internal"

#include <time.h>

#ifdef VARS_DEBUG
#	define POP_TRACE(depth)	fprintf( stderr, "popto(%s,%d)=%d\n",\
		CX_Trace_StackTop->func, depth-1, pop_trace_stackto(depth-1))
#	define PUSH_TRACE(fun,newdepth)	fprintf( stderr, "push(%s)=%d\n",\
		fun, newdepth=push_trace_stack(__FILE__,CX_Time(),fun,__LINE__))
#else
#	define POP_TRACE(depth)	pop_trace_stackto(depth-1)
#	define PUSH_TRACE(fun,newdepth)	newdepth=push_trace_stack(__FILE__,CX_Time(),fun,__LINE__)
#endif

extern double ascanf_memory[];
