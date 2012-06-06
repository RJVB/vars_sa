/* varsA.c: package for handling
 * variables and parameters in arelatively simple manner
 \ simple scanner for arrays including a function interpreter.
 * (C) R. J. Bertin 1990,1991,..,1994
 * :ts=5
 * :sw=5
 */

#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#include "varsintr.h"

#ifndef VARS_STANDALONE
#	include "cx.h"
#	include "mxt.h"
#else
#	include "vars-standalone.h"
#endif
#include "CX_writable_strings.h"
#include "varsedit.h"

#if !defined(WIN32) && !defined(__MACH__) && !defined(__APPLE_CC__) && !defined(__CYGWIN__)
#	include <values.h>
#endif
#include <signal.h>

IDENTIFY( "ascanf routines");

#if defined(linux) || defined(__MACH__) || defined(__APPLE_CC__) || defined(_MSC_VER) || defined(__CYGWIN__)
FILE *vars_file= NULL;
#	ifndef VARS_STANDALONE
	FILE *vars_errfile= NULL;
#	endif
#else
FILE *vars_file= stdin;
#	ifndef VARS_STANDALONE
	FILE *vars_errfile= stderr;
#	endif
#endif
#define StdErr	vars_errfile

extern char *vars_Find_Symbol(pointer x);

long d2long( x)
double x;
{  double X= (double) MAXLONG;
	if( x <= X && x>= -X -1L ){
		return( (long) x);
	}
	else if( x< 0)
		return( LONG_MIN);
	else if( x> 0)
		return( MAXLONG);
}

short d2short(x)
double x;
{  double X= (double)MAXSHORT;
	if( x <= X && x>= -X -1L ){
		return( (short) x);
	}
	else if( x< 0)
		return( SHRT_MIN);
	else if( x> 0)
		return( MAXSHORT);
}

int d2int( x)
double x;
{  double X= (double)MAXLONG;
	if( x <= X && x>= -X -1L ){
		return( (long) x);
	}
	else if( x< 0)
		return( LONG_MIN);
	else if( x> 0)
		return( MAXLONG);
}

/* find the first occurence of the character 'match' in the
 \ string arg_buf, skipping over balanced pairs of <brace-left>
 \ and <brace_right>. The escaped flag must be initialised!
 */
char *find_balanced( char *arg_buf, const char match, const char brace_left, const char brace_right, int *escaped )
{ int brace_level= 0;
  char *d= arg_buf;
	while( d && *d && !(*d== match && brace_level== 0) ){
		if( *d== brace_left && !*escaped ){
			brace_level++;
		}
		else if( *d== brace_right && !*escaped ){
			brace_level--;
			if( !brace_level ){
				d++;
				  /* Ok... I know this is bad practice.. that I should've written
				   \ a break statement here... But I just prefer to easily see (without
				   \ counting braces etc) where we'll continue...
				   */
				goto balanced;
			}
		}
		if( *d== '\\' ){
		  /* if the previous character is a '\\', we can turn
		   \ the escaped flag off; else we must turn it on
		   */
			*escaped= !*escaped;
		}
		else{
		  /* a non-'\\'. Effectively turns off the escaped flag.	*/
			*escaped= False;
		}
		d++;
	}
balanced:;
	return( (d && *d== match)? d : NULL );
}

/* this index() function skips over a (nested) matched
 * set of square braces.
 */
char *ascanf_index( char *s, char c)
{ int escaped= False;
	if( !( s= find_balanced( s, c, '[', ']', &escaped)) ){
		return(NULL);
	}
	if( *s== c)
		return( s);
	else
		return( NULL);
}

/* routine that parses input in the form "[ arg ]" .
 * arg may be of the form "arg1,arg2", or of the form
 * "fun[ arg ]" or "fun". fascanf() is used to parse arg.
 * After parsing, (*function)() is called with the arguments
 * in an array, or NULL if there were no arguments. Its second argument
 * is the return-value. The return-code can be 1 (success), 0 (syntax error)
 * or EOF to skip the remainder of the line.
 */
int reset_ascanf_index_value= True, reset_ascanf_self_value= True;
double ascanf_self_value, ascanf_current_value, ascanf_index_value, ascanf_memory[ASCANF_MAX_ARGS], ascanf_progn_return;
int ascanf_arguments, ascanf_arg_error, ascanf_verbose= 1;

#ifndef VARS_STANDALONE
extern int matherr_verbose;
#endif

int ascanf_escape= False;
static int ascanf_loop= 0, *ascanf_loop_ptr= &ascanf_loop;

DEFUN(ascanf_if, ( double args[ASCANF_MAX_ARGS], double *result), int);
DEFUN(ascanf_progn, ( double args[ASCANF_MAX_ARGS], double *result), int);
DEFUN(ascanf_verbose_fnc, ( double args[ASCANF_MAX_ARGS], double *result), int);
#ifndef VARS_STANDALONE
DEFUN(ascanf_matherr_fnc, ( double args[ASCANF_MAX_ARGS], double *result), int);
#endif
DEFUN( ascanf_whiledo, ( double args[ASCANF_MAX_ARGS], double *result), int );
DEFUN( ascanf_for_to, ( double args[ASCANF_MAX_ARGS], double *result), int );
DEFUN( ascanf_dowhile, ( double args[ASCANF_MAX_ARGS], double *result), int );
DEFUN( ascanf_print, ( double args[ASCANF_MAX_ARGS], double *result), int );
DEFUN( ascanf_Eprint, ( double args[ASCANF_MAX_ARGS], double *result), int );

char ascanf_separator= ',';

int ascanf_function( ascanf_Function *Function, int Index, char **s, int ind, double *A, char *caller)
/* ascanf_Function *Function;	*/
/* int Index;	*/
/* char **s, *caller;	*/
/* int ind;	*/
/* double *A;	*/
{ char arg_buf[ASCANF_FUNCTION_BUF], *c= arg_buf, *d, *args= arg_buf;
  double arg[ASCANF_MAX_ARGS];
  int i= 0, n= Function->args, brace_level= 0, ok= 0, ok2= 0;
  int _ascanf_loop= *ascanf_loop_ptr, *awlp= ascanf_loop_ptr;

  ascanf_Function_method function= Function->function;
    /* Actually, we pass a third argument to the function: Function !
     \ The stuff with the double underscore is to prevent compiler blabla
	*/
  __ascanf_Function_method __function= (__ascanf_Function_method) Function->function;

  char *name= Function->name;
  static int level= 0;
  int verb= ascanf_verbose;
#ifndef VARS_STANDALONE
  int mverb= matherr_verbose;
#endif
/* 930903: These are now set in externally - i.e. at the top
   of the possibly recursive tree.
	if( !level ){
		ascanf_self_value= *A;
		ascanf_index_value++;
	}
 */
	level++;
	ascanf_arg_error= 0;
#ifndef VARS_STANDALONE
	if( function== ascanf_matherr_fnc ){
		matherr_verbose= 1;
	}
#endif
	if( function== ascanf_verbose_fnc ){
		ascanf_verbose= 1;
#ifndef VARS_STANDALONE
		matherr_verbose= -1;
#endif
		fprintf( vars_errfile, "#%d:\t%s\n", level, *s );
	}
	if( *(d= &((*s)[ind]) )== '[' ){
		d++;
		while( *d && !(*d== ']' && brace_level== 0) ){
		  /* find the end of this input pattern	*/
			if( i< ASCANF_FUNCTION_BUF-1 )
				*c++ = *d;
			if( *d== '[')
				brace_level++;
			else if( *d== ']')
				brace_level--;
			d++;
			i++;
		}
		*c= '\0';
		if( *d== ']' ){
		  /* there was an argument list: parse it	*/
			while( isspace((unsigned char)*args) )
				args++;
			if( !strlen(args) ){
				ascanf_arguments= 0;
				if( ascanf_verbose ){
					fprintf( vars_errfile, "#%d\t%s== ", level, name );
					Flush_File( vars_errfile );
				}
				ok= (*__function)( NULL, A, Function);
				ascanf_current_value= *A;
				ok2= 1;
			}
			else{
				if( i<= ASCANF_FUNCTION_BUF-1 ){
				  int j;
				  char *larg[3]= { NULL, NULL, NULL};
					for( arg[0]= 0.0, j= 1; j< Function->args; j++){
						arg[j]= 1.0;
					}
					if( function== ascanf_if ){
					  int N= 0;
						larg[0]= arg_buf;
						larg[1]= ascanf_index( larg[0], ascanf_separator);
						larg[2]= ascanf_index( &(larg[1][1]), ascanf_separator);
						if( larg[0] ){
							N= 1;
							n= 1;
							fascanf( &n, larg[0], &arg[0], NULL );
							if( arg[0] ){
							  /* try to evaluate a second argument	*/
								if( larg[1] ){
									n= 1;
									fascanf( &n, &(larg[1][1]), &arg[1], NULL);
									if( n ){
										N= 2;
									}
								}
							}
							else{
								set_NaN( arg[1] );
								if( larg[2] ){
								  /* try to evaluate the third argument.	*/
									n= 1;
									fascanf( &n, &(larg[2][1]), &arg[2], NULL);
									if( n ){
										N= 3;
									}
								}
								else{
								  /* default third argument	*/
									N= 3;
									arg[2]= 0.0;
								}
							}
							n= N;
						}
					}
/* 
					else if( function== ascanf_progn || function== ascanf_dowhile ||
							function== ascanf_print || function== ascanf_Eprint || function== ascanf_verbose_fnc
					){
						ascanf_progn_return= 0.0;
					}
 */
					ascanf_loop_ptr= &_ascanf_loop;
					do{
						if( *ascanf_loop_ptr> 0 ){
						  /* We can come back here while evaluating the arguments of
						   \ a while function. We don't want to loop those forever...
						   */
							if( ascanf_verbose ){
								fputs( " (loop)\n", StdErr );
							}
							*ascanf_loop_ptr= -1;
						}
						  /* ascanf_whiledo is the C while() construct. If the first
						   \ element, the test, evals to false, the rest of the arguments
						   \ are not evaluated. ascanf_dowhile tests the last argument,
						   \ which is much easier to implement.
						   */
						if( function== ascanf_whiledo ){
						  int N= n-1;
/* 							ascanf_progn_return= 0.0;	*/
							larg[0]= arg_buf;
							larg[1]= ascanf_index( larg[0], ascanf_separator );
							n= 1;
							fascanf( &n, larg[0], &arg[0], NULL );
							if( arg[0] ){
							  /* test argument evaluated true, now evaluate rest of the args	*/
								n= N;
								fascanf( &n, &larg[1][1], &arg[1], NULL );
								 /* the actual number of arguments:	*/
								n+= 1;
							}
							else{
							  /* test false: we skip the rest	*/
								n= 1;
							}
						}
						  /* the C for() construct. The first argument is an initialiser, which is
						   \ initialised only the first time around. The second is the continuation
						   \ test. The rest of the arguments is evaluated only when the second is true.
						   */
						else if( function== ascanf_for_to ){
						  int N= n-2;
						  int first_ok= 1;
							larg[0]= arg_buf;
							larg[1]= ascanf_index( larg[0], ascanf_separator);
							larg[2]= ascanf_index( &(larg[1][1]), ascanf_separator);
							n= 1;
							if( !*ascanf_loop_ptr ){
								fascanf( &n, larg[0], &arg[0], NULL );
								first_ok= n;
								if( ascanf_verbose ){
									fprintf( StdErr, "#%s %d\n",
										(first_ok)? " for_to(init)" : " for_to(invalid init)", level
									);
								}
							}
							if( first_ok ){
								n= 1;
								fascanf( &n, &larg[1][1], &arg[1], NULL );
								if( ascanf_verbose ){
									fprintf( StdErr, "#%s %d\n",
										(arg[1])? " for_to(test,true)" : " for_to(test,false)", level
									);
								}
								if( arg[1] ){
								  /* test argument evaluated true, now evaluate rest of the args	*/
									n= N;
									fascanf( &n, &larg[2][1], &arg[2], NULL );
									 /* the actual number of arguments:	*/
									n+= 2;
								}
								else{
								  /* test false: we skip the rest	*/
									n= 1;
								}
							}
							else{
							  /* test false: we skip the rest	*/
								n= 1;
							}
						}
						else{
							if( !(larg[0] || larg[1] || larg[2]) ){
								fascanf( &n, arg_buf, arg, NULL );
							}
						}
						ascanf_arguments= n;
						if( ascanf_verbose ){
							fprintf( vars_errfile, "#%d\t%s[%s", level, name, d2str( arg[0], "%g", NULL) );
							for( j= 1; j< n; j++){
								fprintf( vars_errfile, ",%s", d2str( arg[j], "%g", NULL) );
							}
							fprintf( vars_errfile, "]== ");
							Flush_File( vars_errfile );
						}
						ok= (*__function)( arg, A, Function);
						ascanf_current_value= *A;
					} while( *ascanf_loop_ptr> 0 && !ascanf_escape );
					ascanf_loop_ptr= awlp;
					ok2= 1;
				}
				else{
					fprintf( cx_stderr, "%s(\"%s\"): arguments list too long (%s)\n",
						caller, *s, arg_buf
					);
				}
			}
			*s= d;
		}
		else{
			fprintf( cx_stderr, "%s(\"%s\"): missing ']'\n", caller, *s);
		}
	}
	else{
	  /* there was no argument list	*/
		*args= '\0';
		ascanf_arguments= 0;
		if( ascanf_verbose ){
			fprintf( vars_errfile, "#%d\t%s== ", level, name );
			Flush_File( vars_errfile );
		}
		ok= (*__function)(NULL,A, Function);
		ascanf_current_value= *A;
		ok2= 1;
	}
	if( ok2){
		if( ascanf_verbose ){
			fprintf( vars_errfile, "%s%s",
				d2str( *A, "%g", NULL),
				(level== 1)? "\t  ," : "\t->"
			);
			if( ascanf_arg_error ){
				fprintf( vars_errfile, " (needs %d arguments)", Function->args );
			}
			fputc( '\n', vars_errfile );
			Flush_File( vars_errfile);
		}
		else if( ascanf_arg_error ){
			fprintf( vars_errfile, "%s== %s: needs %d arguments\n", name, d2str( *A, "%g", NULL), Function->args );
			Flush_File( vars_errfile );
		}
	}
	if( function== ascanf_verbose_fnc
#ifndef VARS_STANDALONE
		|| function== ascanf_matherr_fnc
#endif
	){
		ascanf_verbose= verb;
#ifndef VARS_STANDALONE
		matherr_verbose= mverb;
#endif
	}
	level--;
	return( ok);
}

int set_ascanf_memory( double d)
{  int i;
	for( i= 0; i< ASCANF_MAX_ARGS; i++ ){
		ascanf_memory[i]= d;
	}
	return( i );
}

int ascanf_mem( double args[ASCANF_MAX_ARGS], double *result)
{
	if( !args || ascanf_arguments== 0 ){
		ascanf_arg_error= 0;
		*result= (double) ASCANF_MAX_ARGS;
		if( ascanf_verbose ){
			fprintf( vars_errfile, " (slots) " );
		}
		return(1);
	}
	if( (args[0]= floor( args[0] ))>= -1 && args[0]< ASCANF_MAX_ARGS ){
	  int i= (int) args[0], I= i+1;
		if( ascanf_arguments>= 2 ){
			if( i== -1 ){
				I= ASCANF_MAX_ARGS;
				i= 0;
			}
			for( ; i< I; i++ ){
				ascanf_memory[i]= args[1];
			}
			*result= args[1];
		}
		else{
			if( i== -1 ){
				ascanf_arg_error= 1;
				*result= 0;
			}
			else{
				*result= ascanf_memory[i];
			}
		}
	}
	else{
		ascanf_arg_error= 1;
		*result= 0;
	}
	return( 1 );
}

double **ascanf_mxy_buf;
int ascanf_mxy_X= 0, ascanf_mxy_Y= 0;

int free_ascanf_mxy_buf()
{ int i;
	if( ascanf_mxy_X>= 0 && ascanf_mxy_Y>= 0 && ascanf_mxy_buf ){
		for( i= 0; i< ascanf_mxy_X; i++ ){
			if( ascanf_mxy_buf[i] ){
				free( ascanf_mxy_buf[i] );
			}
		}
		free( ascanf_mxy_buf );
		ascanf_mxy_buf= NULL;
		ascanf_mxy_X= 0;
		ascanf_mxy_Y= 0;
	}
}

int ascanf_setmxy( double args[ASCANF_MAX_ARGS], double *result)
{
	if( !args || ascanf_arguments< 2 ){
		ascanf_arg_error= 1;
		return(0);
	}
	if( ((args[0]= floor( args[0] ))>= 0 ) &&
		((args[1]= floor( args[1] ))>= 0 )
	){
	  int i, ok;
		free_ascanf_mxy_buf();
		ascanf_mxy_X= (int) args[0];
		ascanf_mxy_Y= (int) args[1];
		if( (ascanf_mxy_buf= (double**) calloc( ascanf_mxy_X, sizeof(double*))) ){
			for( ok= 1, i= 0; i< ascanf_mxy_X; i++ ){
				if( !(ascanf_mxy_buf[i]= (double*) calloc( ascanf_mxy_Y, sizeof(double))) ){
					ok= 0;
				}
			}
			if( !ok ){
				ascanf_arg_error= 1;
				free_ascanf_mxy_buf();
				*result= 0;
				return( 0 );
			}
			*result= (double) ascanf_mxy_X * ascanf_mxy_Y * sizeof(double);
		}
	}
	return( 1 );
}

int ascanf_mxy( double args[ASCANF_MAX_ARGS], double *result)
{
	if( !args || ascanf_arguments< 1 ){
		ascanf_arg_error= 1;
		return(0);
	}
	if( ((args[0]= floor( args[0] ))>= -1 && args[0]< ascanf_mxy_X) &&
		((args[1]= floor( args[1] ))>= -1 && args[1]< ascanf_mxy_Y) &&
		ascanf_mxy_buf
	){
	 int x= (int) args[0], y= (int) args[1], X= x+1, Y= y+1, y0;
		if( ascanf_arguments>= 3 ){
			if( x== -1 ){
			  /* set x-range	*/
				X= ascanf_mxy_X;
				x= 0;
			}
			if( y== -1 ){
			  /* set y-range	*/
				Y= ascanf_mxy_Y;
				y= 0;
			}
			y0= y;
			for( ; x< X; x++ ){
				for( y= y0; y< Y; y++ ){
					ascanf_mxy_buf[x][y]= args[2];
				}
			}
			*result= args[2];
		}
		else{
			if( x== -1 || y== -1 ){
				ascanf_arg_error= 1;
				*result= 0;
			}
			else{
				*result= ascanf_mxy_buf[x][y];
			}
		}
	}
	else{
		ascanf_arg_error= 1;
		*result= 0;
	}
	return( 1 );
}

int ascanf_Index( double args[ASCANF_MAX_ARGS], double *result)
{
	*result= ascanf_index_value;
	return( 1);
}

int ascanf_self( double args[ASCANF_MAX_ARGS], double *result )
{
	*result= ascanf_self_value;
	return( 1 );
}

int ascanf_current( double args[ASCANF_MAX_ARGS], double *result )
{
	*result= ascanf_current_value;
	return( 1 );
}

/* routine for the "ran[low,high]" ascanf syntax	*/
int ascanf_random( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= drand();
		return( 1 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= drand() * (args[1]- args[0]) + args[0];
		return( 1 );
	}
}

/* routine for the "ranselect[n0,n1,...nn]" ascanf syntax	*/
int ascanf_random_select( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int n;
		CLIP_EXPR( n, (int) rand_in( 0, ascanf_arguments), 0, ascanf_arguments);
		*result= args[n];
		return( 1 );
	}
}

/* routine for the "add[x,y]" ascanf syntax	*/
int ascanf_add( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i;
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= args[0];
		for( i= 1; i< ascanf_arguments; i++ ){
			*result+= args[i];
		}
		return( 1 );
	}
}

/* routine for the "sub[x,y]" ascanf syntax	*/
int ascanf_sub( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i;
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= args[0];
		for( i= 1; i< ascanf_arguments; i++ ){
			*result-= args[i];
		}
		return( 1 );
	}
}

/* routine for the "mul[x,y]" ascanf syntax	*/
int ascanf_mul( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i;
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= args[0];
		for( i= 1; i< ascanf_arguments; i++ ){
			*result*= args[i];
		}
		return( 1 );
	}
}

/* routine for the "div[x,y]" ascanf syntax	*/
int ascanf_div( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i;
		ascanf_arg_error= (ascanf_arguments< 2 );
		if( !args[1]){
			set_Inf( *result, args[0] );
		}
		else{
			*result= args[0];
			for( i= 1; i< ascanf_arguments; i++ ){
				if( !args[i]){
					set_Inf( *result, *result );
				}
				else{
					*result/= args[i];
				}
			}
		}
		return( 1 );
	}
}

/* routine for the "fac[x,y]" ascanf syntax	*/
int ascanf_fac( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= dfac( args[0], args[1]);
		return( 1 );
	}
}

/* routine for the "atan2[x,y]" ascanf syntax	*/
int ascanf_atan2( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 2 );
		if( ascanf_arguments< 3 || args[2]== 0.0 ){
			args[2]= M_2PI;
		}
		*result= args[2]* ( atan2( args[0], args[1] ) / M_2PI );
		return( 1 );
	}
}

/* return arg(x,y) in radians [0,2PI]	*/
double arg( x, y)
double x, y;
{ 
	if( x> 0.0){
		if( y>= 0.0)
			return( (atan(y/x)));
		else
			return( M_2PI+ (atan(y/x)));
	}
	else if( x< 0.0){
		return( M_PI+ (atan(y/x)));
	}
	else{
		if( y> 0.0){
		  /* 90 degrees	*/
			return( M_PI_2 );
		}
		else if( y< 0.0){
		  /* 270 degrees	*/
			return( M_PI_2 * 3 );
		}
		else
			return( 0.0);
	}
}

/* routine for the "arg[x,y]" ascanf syntax	*/
int ascanf_arg( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 2 );
		if( ascanf_arguments< 3 || args[2]== 0.0 ){
			args[2]= M_2PI;
		}
		*result= args[2] * (arg( args[0], args[1] )/ M_2PI);
		return( 1 );
	}
}

/* routine for the "conv_angle[x,base]" ascanf syntax	*/
int ascanf_conv_angle( double args[ASCANF_MAX_ARGS], double *result )
{ extern double conv_angle_( double, double);
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 1 );
		if( ascanf_arguments< 2 || args[1]== 0.0 ){
			args[1]= M_2PI;
		}
		*result= conv_angle_( args[0], args[1] );
		return( 1 );
	}
}

/* routine for the "mod_angle[x,base]" ascanf syntax	*/
int ascanf_mod_angle( double args[ASCANF_MAX_ARGS], double *result )
{ extern double mod_angle_( double, double);
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 1 );
		if( ascanf_arguments< 2 || args[1]== 0.0 ){
			args[1]= M_2PI;
		}
		*result= mod_angle_( args[0], args[1] );
		return( 1 );
	}
}

/* routine for the "len[x,y]" ascanf syntax	*/
int ascanf_len( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= sqrt( args[0]*args[0] + args[1]*args[1] );
		return( 1 );
	}
}

/* routine for the "log[x,y]" ascanf syntax	*/
int ascanf_log( double args[ASCANF_MAX_ARGS], double *result )
{ double largs1;
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		if( ascanf_arguments== 1 || args[1]== 1 ){
			largs1= 1.0;
		}
		else{
			largs1= log( args[1] );
		}
		*result= log( args[0]) / largs1;
		return( 1 );
	}
}

/* routine for the "pow[x,y]" ascanf syntax	*/
int ascanf_pow( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= pow( args[0], args[1] );
		return( 1 );
	}
}

/* routine for the "pi[mul_fact]" ascanf syntax	*/
int ascanf_pi( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= M_PI;
		return( 1 );
	}
	else{
		  /* we don't need args[1], so we set it to 0
		   * (which prints nicer)
		   */
		args[1]= 0.0;
		*result= M_PI * args[0];
		return( 1 );
	}
}

#ifndef VARS_STANDALONE
int ascanf_elapsed( double args[ASCANF_MAX_ARGS], double *result )
{  static Time_Struct timer;
   static char called= 0;
	Elapsed_Since( &timer);
	if( !called){
		called= 1;
		*result= 0.0;
		return(1);
	}
	if( !args){
		*result= timer.Tot_Time;
	}
	else{
		*result= timer.Time;
	}
	return( 1 );
}

int ascanf_time( double args[ASCANF_MAX_ARGS], double *result )
{  static Time_Struct timer_0;
   Time_Struct timer;
   static char called= 0;
	if( !called){
	  /* initialise the real timer	*/
		called= 1;
		Set_Timer();
		Elapsed_Since( &timer_0 );
		*result= 0.0;
		return(1);
	}
	else{
	  /* make a copy of the timer, and determine the time
	   \ since initialisation
	   */
		timer= timer_0;
		Elapsed_Since( &timer );
	}
	if( !ascanf_arguments ){
		*result= timer.Tot_Time;
	}
	else{
		*result= timer.Time;
	}
	return( 1 );
}
#endif

static int ascanf_continue= 0;
void ascanf_continue_( int sig )
{
	ascanf_continue= 1;
#ifndef WIN32
	signal( SIGALRM, ascanf_continue_ );
#endif
}

#ifndef WIN32
#	include <sys/time.h>
#endif

int ascanf_sleep_once( double args[ASCANF_MAX_ARGS], double *result )
{
#ifndef VARS_STANDALONE
  Time_Struct timer;
#endif
	if( ascanf_arguments>= 1 && args[0]> 0 ){
#ifndef WIN32
	  struct itimerval rtt, ortt;
	  pointer old_alarm;
		rtt.it_value.tv_sec= (unsigned long) floor(args[0]);
		rtt.it_value.tv_usec= (unsigned long) ( (args[0]- rtt.it_value.tv_sec)* 1000000 );
		rtt.it_interval.tv_sec= 0;
		rtt.it_interval.tv_usec= 0;
		old_alarm= signal( SIGALRM, ascanf_continue_ );
		ascanf_continue= 0;
#ifndef VARS_STANDALONE
		Elapsed_Since( &timer );
#endif
		setitimer( ITIMER_REAL, &rtt, &ortt );
		pause();
#ifndef VARS_STANDALONE
		Elapsed_Since( &timer );
#endif
		  /* restore the previous setting of the timer.	*/
		setitimer( ITIMER_REAL, &ortt, &rtt );
		  /* and restore the previous SIGALRM handler. ascanf_set_interval is not so nice...	*/
		signal( SIGALRM, old_alarm );
#endif
	}
	else{
		ascanf_arg_error= 1;
		return( 0 );
	}
	ascanf_continue= 0;
#ifndef VARS_STANDALONE
	*result= timer.Tot_Time;
#else
	*result= args[0];
#endif
	return( 1 );
}

int ascanf_set_interval( double args[ASCANF_MAX_ARGS], double *result )
{ static double initial= -1, interval= -1;
#ifndef VARS_STANDALONE
  static Time_Struct timer;
#endif
	if( ascanf_arguments>= 1 && args[0]> 0 ){
#ifndef WIN32
	  struct itimerval rtt, ortt;
		if( ascanf_arguments< 2 || args[1]<= 0 ){
			args[1]= args[0];
		}
		if( args[0]!= initial || args[1]!= interval ){
			rtt.it_value.tv_sec= (unsigned long) floor(args[0]);
			rtt.it_value.tv_usec= (unsigned long) ( (args[0]- rtt.it_value.tv_sec)* 1000000 );
			rtt.it_interval.tv_sec= (unsigned long) floor(args[1]);
			rtt.it_interval.tv_usec= (unsigned long) ( (args[1]- rtt.it_interval.tv_sec)* 1000000 );
			signal( SIGALRM, ascanf_continue_ );
			ascanf_continue= 0;
			initial= args[0];
			interval= args[1];
#ifndef VARS_STANDALONE
			Elapsed_Since( &timer );
#endif
			setitimer( ITIMER_REAL, &rtt, &ortt );
		}
		if( !ascanf_continue ){
		  /* wait for the signal	*/
			pause();
			  /* Make sure that we will pause if no interval has expired since the last call!	*/
			ascanf_continue= 0;
#ifndef VARS_STANDALONE
			Elapsed_Since( &timer );
#endif
		}
#endif
	}
	else{
		ascanf_arg_error= 1;
		return( 0 );
	}
#ifndef VARS_STANDALONE
	*result= timer.Tot_Time;
#else
	*result= args[0];
#endif
	return( 1 );
}

#ifndef degrees
#	define degrees(a)			((a)*57.295779512)
#endif
#ifndef radians
#	define radians(a)			((a)*(1.0/57.295779512))
#endif

int ascanf_degrees( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= degrees(M_2PI);
		return( 1 );
	}
	else{
		  /* we don't need args[1], so we set it to 0
		   * (which prints nicer)
		   */
		args[1]= 0.0;
		*result= degrees( args[0]);
		return( 1 );
	}
}

int ascanf_radians( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= radians(360.0);
		return( 1 );
	}
	else{
		  /* we don't need args[1], so we set it to 0
		   * (which prints nicer)
		   */
		args[1]= 0.0;
		*result= radians( args[0]);
		return( 1 );
	}
}

int ascanf_sin( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		if( ascanf_arguments== 1 || args[1]== 0.0 ){
			args[1]= M_2PI;
		}
		*result= sin( M_2PI * args[0]/ args[1] );
		return( 1 );
	}
}

int ascanf_cos( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		if( ascanf_arguments== 1 || args[1]== 0.0 ){
			args[1]= M_2PI;
		}
		*result= cos( M_2PI * args[0]/ args[1] );
		return( 1 );
	}
}

int ascanf_tan( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		if( ascanf_arguments== 1 || args[1]== 0.0 ){
			args[1]= M_2PI;
		}
		*result= tan( M_2PI * args[0]/ args[1] );
		return( 1 );
	}
}

int ascanf_exp( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= exp(1.0);
		return( 1 );
	}
	else{
		  /* we don't need args[1], so we set it to 0
		   * (which prints nicer)
		   */
		args[1]= 0.0;
		*result= exp( args[0]);
		return( 1 );
	}
}

int ascanf_dcmp( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args || ascanf_arguments< 2 ){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		if( ascanf_arguments== 2 ){
			args[2]= -1.0;
		}
		*result= dcmp( args[0], args[1], args[2] );
		return( 1 );
	}
}

int ascanf_misc_fun( double args[ASCANF_MAX_ARGS], double *result, int code, int argc )
{
	if( !args || ascanf_arguments< argc ){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int a0= (args[0]!= 0.0), a1= (args[1]!= 0.0);
		if( ascanf_arguments== 2 ){
			args[2]= 0.0;
		}
		switch( code ){
			case 0:
				*result= (args[0])? args[1] : args[2];
				break;
			case -1:
				*result= (double) ( args[0] == args[1] );
				break;
			case 1:
				*result= (double) ( args[0] > args[1] );
				break;
			case 2:
				*result= (double) ( args[0] < args[1] );
				break;
			case 3:
				*result= (double) ( args[0] >= args[1] );
				break;
			case 4:
				*result= (double) ( args[0] <= args[1] );
				break;
			case 5:
				*result= (double) ( a0 && a1 );
				break;
			case 6:
				*result= (double) ( a0 || a1 );
				break;
			case 7:
				  /* Boolean XOR doesn't exist (not even as ^^)
				   \ However, if a0 and a1 are Booleans, and
				   \ either 0 or 1, than the bitwise operation
				   \ does exactly the same thing
				   */
				*result= (double) ( a0 ^ a1 );
				break;
			case 8:
				*result= (double) ! a0;
				break;
			case 9:
				*result= fabs( args[0] );
				break;
			case 10:
				*result= (ascanf_progn_return= args[0]);
				break;
			case 11:
				*result= MIN( args[0], args[1] );
				break;
			case 12:
				*result= MAX( args[0], args[1] );
				break;
			case 13:
				*result= floor( args[0] );
				break;
			case 14:
				*result= ceil( args[0] );
				break;
			case 15:
				*result= uniform_rand( args[0], args[1] );
				break;
			case 16:
				*result= abnormal_rand( args[0], args[1] );
				break;
#ifndef WIN32
			case 17:
				*result= erf( args[0] );
				break;
			case 18:
				*result= erfc( args[0] );
				break;
#endif
	    		case 19:
	    			*result= normal_rand( args[0], args[1] );
	    			break;
	    		case 20:
				if( args[0]< args[1] ){
					*result=  args[1];
				}
				else if( args[0]> args[2] ){
					*result= args[2];
				}
				else{
					*result= args[0];
				}
	    			break;
		}
		return( 1 );
	}
}

int ascanf_if( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 0, 2) );
}

int ascanf_if2( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 0, 2) );
}

int ascanf_gt( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 1, 2) );
}

int ascanf_lt( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 2, 2) );
}

int ascanf_eq( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, -1, 2) );
}

int ascanf_ge( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 3, 2) );
}

int ascanf_le( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 4, 2) );
}

int ascanf_and( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 5, 2) );
}

int ascanf_or( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 6, 2) );
}

int ascanf_xor( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 7, 2) );
}

int ascanf_not( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 8, 1) );
}

int ascanf_abs( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 9, 1) );
}

int ascanf_return( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 10, 1) );
}

int ascanf_min( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 11, 2) );
}

int ascanf_max( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 12, 2) );
}

int ascanf_floor( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 13, 1) );
}

int ascanf_ceil( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 14, 1) );
}

int ascanf_uniform( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 15, 2) );
}

int ascanf_abnormal( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 16, 2) );
}

#ifndef WIN32
int ascanf_erf( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 17, 1) );
}

int ascanf_erfc( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 18, 1) );
}
#endif

int ascanf_normal( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 19, 2) );
}

int ascanf_clip( double args[ASCANF_MAX_ARGS], double *result)
{
	return( ascanf_misc_fun( args, result, 20, 3) );
}

int ascanf_progn( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		*result= ascanf_progn_return;
		return( 1 );
	}
}

int ascanf_whiledo( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args || ascanf_arguments< 1 || (args[0] && ascanf_arguments< 2) ){
	  /* if !args[0], this routine is called with ascanf_arguments==1,
	   \ in which case it should set *ascanf_loop_ptr to true
	   \ (this is to avoid jumping or deep nesting in ascanf_function
	   */
		ascanf_arg_error= 1;
		*ascanf_loop_ptr= 0;
		return( 0 );
	}
	else{
		*result= ascanf_progn_return;
		*ascanf_loop_ptr= (args[0]!= 0);
		return( 1 );
	}
}

int ascanf_for_to( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args || ascanf_arguments< 1 ){
		ascanf_arg_error= 1;
		*ascanf_loop_ptr= 0;
		return( 0 );
	}
	else{
		*result= ascanf_progn_return;
		*ascanf_loop_ptr= (args[1]!= 0);
		return( 1 );
	}
}

int ascanf_dowhile( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args || ascanf_arguments< 2 ){
		ascanf_arg_error= 1;
		*ascanf_loop_ptr= 0;
		return( 0 );
	}
	else{
		*result= ascanf_progn_return;
		*ascanf_loop_ptr= (args[ascanf_arguments-1]!= 0);
		return( 1 );
	}
}

/* routine for the "print[x,y,...]" ascanf syntax	*/
int ascanf_print( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i;
		if( ascanf_arguments< 2 ){
			*result= args[0];
		}
		else{
			*result= ascanf_progn_return;
		}
		fprintf( stdout, "print[%s", d2str( args[0], "%g", NULL) );
		for( i= 1; i< ascanf_arguments; i++ ){
			fprintf( stdout, ",%s", d2str( args[i], "%g", NULL) );
		}
		fprintf( stdout, "]== %s\n", d2str( *result, "%g", NULL) );
		return( 1 );
	}
}

/* routine for the "Eprint[x,y,...]" ascanf syntax	*/
int ascanf_Eprint( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i;
		if( ascanf_arguments< 2 ){
			*result= args[0];
		}
		else{
			*result= ascanf_progn_return;
		}
		fprintf( vars_errfile, "print[%s", d2str( args[0], "%g", NULL) );
		for( i= 1; i< ascanf_arguments; i++ ){
			fprintf( vars_errfile, ",%s", d2str( args[i], "%g", NULL) );
		}
		fprintf( vars_errfile, "]== %s\n", d2str( *result, "%g", NULL) );
		return( 1 );
	}
}

#ifndef VARS_STANDALONE
/* routine for the "matherr[x,y,...]" ascanf syntax	*/
int ascanf_matherr_fnc( double args[ASCANF_MAX_ARGS], double *result )
{
	if( ascanf_arguments== 1 ){
		*result= args[0];
	}
	else{
		*result= ascanf_progn_return;
	}
	return( 1 );
}
#endif

/* routine for the "verbose[x,y,...]" ascanf syntax	*/
int ascanf_verbose_fnc( double args[ASCANF_MAX_ARGS], double *result )
{
	if( ascanf_arguments== 1 ){
	  /* We just return the 1st argument	*/
		*result= args[0];
	}
	else{
	  /* We return the argument/value selected return[] (even if not called in our scope)	*/
		*result= ascanf_progn_return;
	}
	return( 1 );
}

/* routine for the "fmod[x,y]" ascanf syntax	*/
int ascanf_fmod( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		ascanf_arg_error= (ascanf_arguments< 2 );
		*result= fmod( args[0], args[1] );
		return( 1 );
	}
}

SimpleStats ascanf_SS[ASCANF_MAX_ARGS];

int ascanf_SS_set( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args || ascanf_arguments== 0 ){
		ascanf_arg_error= 0;
		if( ascanf_verbose ){
			fprintf( StdErr, " (slots) " );
		}
		*result= (double) sizeof(ascanf_SS)/sizeof(SimpleStats);
		return(1);
	}
	if( (args[0]= floor( args[0] ))>= -1 && args[0]< ASCANF_MAX_ARGS ){
	  int i= (int) args[0], I= i+1;
		if( ascanf_arguments>= 2 ){
			if( i== -1 ){
				I= ASCANF_MAX_ARGS;
				i= 0;
			}
			if( ascanf_arguments== 2 ){
				args[2]= 1.0;
			}
			for( ; i< I; i++ ){
				SS_Add_Data_(ascanf_SS[i], 1, args[1], args[2]);
			}
			*result= args[1];
		}
		else{
			if( i== -1 ){
				I= ASCANF_MAX_ARGS;
				i= 0;
			}
			if( ascanf_verbose ){
				fprintf( StdErr, " reset slot %d through %d\n", i, I-1 );
			}
			for( ; i< I; i++ ){
				SS_Reset_(ascanf_SS[i]);
			}
			*result= 0.0;
		}
	}
	else{
		ascanf_arg_error= 1;
		*result= 0;
	}
	return( 1 );
}

int ascanf_SS_get( double args[ASCANF_MAX_ARGS], double *result, int what )
{
	if( !args || ascanf_arguments== 0 ){
		ascanf_arg_error= 0;
		if( ascanf_verbose ){
			fprintf( StdErr, " (slots) " );
		}
		*result= (double) sizeof(ascanf_SS)/sizeof(SimpleStats);
		return(1);
	}
	if( (args[0]= floor( args[0] ))>= -1 && args[0]< ASCANF_MAX_ARGS ){
	  int i= (int) args[0];
		if( ascanf_arguments>= 1 ){
			if( i== -1 ){
				ascanf_arg_error= 1;
				*result= 0.0;
			}
			else{
				if( ascanf_verbose ){
				  char buf[256];
					SS_sprint_full( buf, "%g", " +- ", 0, &ascanf_SS[i] );
					fprintf( StdErr, "%s\n", buf );
				}
				switch( what ){
					case 0:
						*result= SS_Mean_( ascanf_SS[i]);
						break;
					case 1:
						*result= SS_St_Dev_( ascanf_SS[i]);
						break;
					case 2:
						*result= (double) ascanf_SS[i].count;
						break;
					case 3:
						*result= (double) ascanf_SS[i].weight_sum;
						break;
					case 4:
						*result= (double) ascanf_SS[i].min;
						break;
					case 5:
/* 						*result= (double) ascanf_SS[i].pos_min;	*/
						break;
					case 6:
						*result= (double) ascanf_SS[i].max;
						break;
				}
			}
		}
		else{
			ascanf_arg_error= 1;
			*result= 0.0;
		}
	}
	else{
		ascanf_arg_error= 1;
		*result= 0;
	}
	return( 1 );
}

int ascanf_SS_Mean( double args[ASCANF_MAX_ARGS], double *result )
{
	return( ascanf_SS_get( args, result, 0) );
}

int ascanf_SS_St_Dev( double args[ASCANF_MAX_ARGS], double *result )
{
	return( ascanf_SS_get( args, result, 1) );
}

int ascanf_SS_Count( double args[ASCANF_MAX_ARGS], double *result )
{
	return( ascanf_SS_get( args, result, 2) );
}

int ascanf_SS_WeightSum( double args[ASCANF_MAX_ARGS], double *result )
{
	return( ascanf_SS_get( args, result, 3) );
}

int ascanf_SS_min( double args[ASCANF_MAX_ARGS], double *result )
{
	return( ascanf_SS_get( args, result, 4) );
}

int ascanf_SS_pos_min( double args[ASCANF_MAX_ARGS], double *result )
{
	return( ascanf_SS_get( args, result, 5) );
}

int ascanf_SS_max( double args[ASCANF_MAX_ARGS], double *result )
{
	return( ascanf_SS_get( args, result, 6) );
}

/* routine for the "compress[x,C[,F]]" ascanf syntax	*/
int ascanf_compress( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args || ascanf_arguments< 2){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
		if( ascanf_arguments< 3 ){
			*result= args[0]/ ( ABS(args[0]) + args[1] );
		}
		else{
		  double F= args[2];
			*result= pow(args[0],F)/ ( pow( ABS(args[0]),F) + pow( args[1],F) );
		}
		return( 1 );
	}
}

double ascanf_delta_t= 0.01;
/* routine for the "lowpass[x,tau,mem_index]" ascanf syntax	*/
int ascanf_lowpass( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args || ascanf_arguments< 3){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i= (int) args[2];
	  double mem= (args[1])? exp( - ascanf_delta_t / args[1] ) : 0.0;
		ascanf_memory[i]= *result= mem* ascanf_memory[i]+ args[0];
		return( 1 );
	}
}

/* routine for the "nlowpass[x,tau,mem_index]" ascanf syntax	*/
int ascanf_nlowpass( double args[ASCANF_MAX_ARGS], double *result )
{ extern double ascanf_delta_t;
	if( !args || ascanf_arguments< 3){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i= (int) args[2];
	  double mem= (args[1])? exp( - ascanf_delta_t / args[1] ) : 0.0;
		ascanf_memory[i]= *result= mem* ascanf_memory[i]+ (1-mem)* args[0];
		return( 1 );
	}
}

/* routine for the "shunt[x,y,C,tau,mem_index]" ascanf syntax
 */
int ascanf_shunt( double args[ASCANF_MAX_ARGS], double *result )
{ extern double ascanf_delta_t;
	if( !args || ascanf_arguments< 5){
		ascanf_arg_error= 1;
		return( 0 );
	}
	else{
	  int i= (int) args[4];
	  double mem= exp( - ascanf_delta_t / args[3] );
		ascanf_memory[i]= *result=
			ascanf_memory[i]*( 1+ (mem- 1)* (args[2]+ args[1]))+ (1- mem)* args[0];
		return( 1 );
	}
}

ascanf_Function vars_ascanf_Functions[]= {
	{ "index", ascanf_Index, 0, NOT_EOF, "index (of element)" },
	{ "self", ascanf_self, 0, NOT_EOF, "self (starting value)" },
	{ "current", ascanf_current, 0, NOT_EOF, "current value" },
	{ "MEM", ascanf_mem, 2, NOT_EOF_OR_RETURN,
		"MEM[n[,expr]]: set MEM[n] to expr, or get MEM[n]: " STRING(ASCANF_MAX_ARGS) " locations"\
		"\n\tSpecify n=-1 to set whole range"
	},
	{ "SETMXY", ascanf_setmxy, 2, NOT_EOF_OR_RETURN, "SETMXY[I,J]: set new dimensions of MXY buffer" },
	{ "MXY", ascanf_mxy, 3, NOT_EOF_OR_RETURN,
		"MXY[i,j[,expr]]: set MXY[i,j] to expr, or get MXY[i,j]"
	},
#ifndef VARS_STANDALONE
	{ "elapsed", ascanf_elapsed, 1, NOT_EOF, "elapsed since last call (real time) or elapsed[x] (system time)"},
	{ "time", ascanf_time, 1, NOT_EOF, "time since first call (real time) or time[x] (system time)"},
#endif
	{ "sleep", ascanf_sleep_once, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "sleep[sec[,exprs]]: wait for <sec> seconds" },
	{ "intertimer", ascanf_set_interval, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN,
		"intertimer[init,interv[,exprs]]: set an interval timer: init. delay <init>, interval <interv>" },
	{ "add", ascanf_add, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "add[x,y]" },
	{ "sub", ascanf_sub, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "sub[x,y]" },
	{ "mul", ascanf_mul, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "mul[x,y]" },
	{ "div", ascanf_div, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "div[x,y]" },
	{ "ran", ascanf_random, 2, NOT_EOF, "ran or ran[low,high]" },
	{ "ransel", ascanf_random_select, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "ransel[n1[,n2,...,nn]]: randomly select an argument" },
	{ "uniform", ascanf_uniform, 2, NOT_EOF_OR_RETURN, "uniform[av,stdv]: random number in a uniform distribution" },
	{ "abnormal", ascanf_abnormal, 2, NOT_EOF_OR_RETURN, "abnormal[av,stdv]: random number in an abnormal distribution" },
	{ "normal", ascanf_normal, 2, NOT_EOF_OR_RETURN, "normal[av,stdv]: random number in a normal distribution" },
	{ "pi", ascanf_pi, 1, NOT_EOF, "pi or pi[mulfac]" },
	{ "pow", ascanf_pow, 2, NOT_EOF_OR_RETURN, "pow[x,y]" },
	{ "fac", ascanf_fac, 2, NOT_EOF_OR_RETURN, "fac[x,dx]" },
	{ "radians", ascanf_radians, 1, NOT_EOF, "radians[x]" },
	{ "degrees", ascanf_degrees, 1, NOT_EOF, "degrees[x]" },
	{ "sin", ascanf_sin, 2, NOT_EOF_OR_RETURN, "sin[x[,base]]" },
	{ "cos", ascanf_cos, 2, NOT_EOF_OR_RETURN, "cos[x[,base]]" },
	{ "tan", ascanf_tan, 2, NOT_EOF_OR_RETURN, "tan[x[,base]]" },
	{ "atan2", ascanf_atan2, 3, NOT_EOF_OR_RETURN, "atan2[Y,X[,base]] atan(y/x) (not C-lib conforming reversal of x,y!)" },
	{ "arg", ascanf_arg, 3, NOT_EOF_OR_RETURN, "arg[x,y[,base]] angle to (x,y) (in 0..2PI)" },
	{ "conv_angle", ascanf_conv_angle, 2, NOT_EOF_OR_RETURN, "conv_angle[x[,base]] angle to <-base/2,base/2] (in 0..2PI)" },
	{ "mod_angle", ascanf_mod_angle, 2, NOT_EOF_OR_RETURN, "mod_angle[x[,base]] angle to <-base,base] (in 0..2PI)" },
	{ "len", ascanf_len, 2, NOT_EOF_OR_RETURN, "len[x,y] distance to (x,y)" },
	{ "exp", ascanf_exp, 1, NOT_EOF, "exp[x]" },
#ifndef WIN32
	{ "erf", ascanf_erf, 1, NOT_EOF_OR_RETURN, "erf[x]" },
	{ "1-erf", ascanf_erfc, 1, NOT_EOF_OR_RETURN, "1-erf[x]" },
#endif
	{ "log", ascanf_log, 2, NOT_EOF_OR_RETURN, "log[x,y]" },
	{ "cmp", ascanf_dcmp, 3, NOT_EOF_OR_RETURN, "cmp[x,y,[precision]]" },
	{ "abs", ascanf_abs, 1, NOT_EOF_OR_RETURN, "abs[x]" },
	{ "floor", ascanf_floor, 1, NOT_EOF_OR_RETURN, "floor[x]" },
	{ "ceil", ascanf_ceil, 1, NOT_EOF_OR_RETURN, "ceil[x]" },
	{ "fmod", ascanf_fmod, 2, NOT_EOF_OR_RETURN, "fmod[x,y]" },
	{ "ifelse2", ascanf_if2, 3, NOT_EOF_OR_RETURN, "ifelse2[expr,val1,[else-val:0]] - all arguments are evaluated" },
	{ "ifelse", ascanf_if, 3, NOT_EOF_OR_RETURN, "ifelse[expr,val1,[else-val:0]] - lazy evaluation" },
	  /* These comparative routines have a double '=='. This is because varsV.c recognises a single '=' as
	   \ an opcode, but not a double '=='...
	   */
	{ "==", ascanf_eq, 2, NOT_EOF_OR_RETURN, "==[x,y]" },
	{ ">==", ascanf_ge, 2, NOT_EOF_OR_RETURN, ">==[x,y]" },
	{ "<==", ascanf_le, 2, NOT_EOF_OR_RETURN, "<==[x,y]" },
	{ ">", ascanf_gt, 2, NOT_EOF_OR_RETURN, ">[x,y]" },
	{ "<", ascanf_lt, 2, NOT_EOF_OR_RETURN, "<[x,y]" },
	{ "AND", ascanf_and, 2, NOT_EOF_OR_RETURN, "AND[x,y] (boolean)" },
	{ "OR", ascanf_or, 2, NOT_EOF_OR_RETURN, "OR[x,y] (boolean)" },
	{ "XOR", ascanf_xor, 2, NOT_EOF_OR_RETURN, "XOR[x,y] (boolean)" },
	{ "NOT", ascanf_not, 1, NOT_EOF_OR_RETURN, "NOT[x] (boolean)" },
	{ "MIN", ascanf_min, 2, NOT_EOF_OR_RETURN, "MIN[x,y]" },
	{ "MAX", ascanf_max, 2, NOT_EOF_OR_RETURN, "MAX[x,y]" },
	{ "clip", ascanf_clip, 3, NOT_EOF_OR_RETURN, "clip[expr,min,max]" },
	{ "return", ascanf_return, 1, NOT_EOF_OR_RETURN, "return[x]" },
	{ "progn", ascanf_progn, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "progn[expr1[,..,expN]]: value set by return[x]" },
	{ "for-to", ascanf_for_to, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "for-to[init_expr,test_expr,expr1[,..,expN]]: value set by return[x]" },
	{ "whiledo", ascanf_whiledo, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "whiledo[test_expr,expr1[,..,expN]]: value set by return[x]" },
	{ "dowhile", ascanf_dowhile, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "dowhile[expr1[,..,expN],test_expr]: value set by return[x]" },
	{ "verbose", ascanf_verbose_fnc, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "verbose[[x,..]]: turns on verbosity for its scope" },
#ifndef VARS_STANDALONE
	{ "matherr", ascanf_matherr_fnc, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "matherr[[x,..]]: turns on matherr verbosity for its scope" },
#endif
	{ "print", ascanf_print, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "print[x[,..]]: returns first (and only) arg,\n\
\t\tvalue set by return[y]" },
	{ "Eprint", ascanf_Eprint, ASCANF_MAX_ARGS, NOT_EOF_OR_RETURN, "Eprint[x[,..]]: prints on stderr" },
	{ "SS_Add", ascanf_SS_set, 3, NOT_EOF_OR_RETURN,
		"SS_Add[n[,expr[,weight]]]: store expr in statistics bin n, with weigth (def.1): " STRING(ASCANF_MAX_ARGS) " locations"\
		"\n\tSpecify n=-1 to store in every bin; missing expr resets bin(s)"
	},
	{ "SS_Mean", ascanf_SS_Mean, 1, NOT_EOF_OR_RETURN, "SS_Mean[n]: get mean for stats bin n" },
	{ "SS_Stdev", ascanf_SS_St_Dev, 1, NOT_EOF_OR_RETURN, "SS_Stdev[n]: get standard deviation for stats bin n" },
	{ "SS_Count", ascanf_SS_Count, 1, NOT_EOF_OR_RETURN, "SS_Count[n]: get nr. of elements in stats bin n" },
	{ "SS_WeightSum", ascanf_SS_WeightSum, 1, NOT_EOF_OR_RETURN, "SS_WeightSum[n]: get sum of weights in stats bin n" },
	{ "SS_Min", ascanf_SS_min, 1, NOT_EOF_OR_RETURN, "SS_Min[n]: get minimum in stats bin n" },
/* 	{ "SS_PosMin", ascanf_SS_pos_min, 1, NOT_EOF_OR_RETURN, "SS_PosMin[n]: get minimal positive value in stats bin n" },	*/
	{ "SS_Max", ascanf_SS_max, 1, NOT_EOF_OR_RETURN, "SS_Max[n]: get maximum in stats bin n" },
	{ "compress", ascanf_compress, 3, NOT_EOF_OR_RETURN, "compress[x,C[,F]]: x^F/(|x^f|+C^f) - default F=1" },
	{ "lowpass", ascanf_lowpass, 3, NOT_EOF_OR_RETURN, "lowpass[x,tau,mem_index] - mem_index is index into MEM array" },
	{ "nlowpass", ascanf_nlowpass, 3, NOT_EOF_OR_RETURN, "nlowpass[x,tau,mem_index]: normalised - mem_index is index into MEM array" },
	{ "shunt", ascanf_shunt, 5, NOT_EOF_OR_RETURN, "shunt[x,y,C,tau,mem_index]: normalised shunting inhibition (x/(C+y)) - mem_index is index into MEM array" },
}, *ascanf_FunctionList= vars_ascanf_Functions;

int ascanf_Functions= sizeof(vars_ascanf_Functions)/sizeof(ascanf_Function);

long ascanf_hash( name)
char *name;
{  long hash= 0L;
#ifdef DEBUG
   int len= 0;
#endif
	while( *name && *name!= '[' && *name!= ascanf_separator && !isspace((unsigned char)*name) && *name!= '{' && *name!= '}' ){
		hash+= hash<<3L ^ *name++;
#ifdef DEBUG
		len+= 1;
#endif
	}
	return( hash);
}

int check_for_ascanf_function( int Index, char **s, double *result, int *ok, char *caller)
{  int i, len;
   ascanf_Function *af;
   long hash= ascanf_hash( *s );
   static unsigned int level= 0;
	if( reset_ascanf_index_value ){
		ascanf_index_value= (double) Index;
	}
	if( reset_ascanf_self_value ){
		ascanf_self_value= *result;
		ascanf_current_value= *result;
	}
	level++;
	for( i= 0; i< ascanf_Functions && ascanf_FunctionList; i++){
		af= &ascanf_FunctionList[i];
		while( af && af->name && af->function ){
			if( af->name_length ){
				len= af->name_length;
			}
			else{
				len= af->name_length= strlen( af->name );
			}
			if( !af->hash ){
				af->hash= ascanf_hash( af->name );
			}
			if( hash== af->hash ){
				if( !strncmp( *s, af->name, len) ){
					*ok= ascanf_function( af, Index, s, len, result, caller);
					level--;
					if( !reset_ascanf_index_value && !level ){
						ascanf_index_value+= 1;
					}
					switch(af->type){
						case NOT_EOF:
							return( (*ok!= EOF) );
							break;
						case NOT_EOF_OR_RETURN:
							return( (*ok== EOF)? 0 : *ok );
							break;
						default:
							return( *ok);
							break;
					}
				}
			}
#ifdef DEBUG_EXTRA
			else{
				if( ascanf_verbose ){
					fprintf( vars_errfile, "!%s(%lx,%lx)", af->name, hash, af->name );
					Flush_File( vars_errfile );
				}
			}
#endif
			af= af->cdr;
		}
	}
	level--;
	if( !reset_ascanf_index_value && !level ){
		ascanf_index_value+= 1;
	}
	return(0);
}

int show_ascanf_functions( FILE *fp, char *prefix)
{  int i;
   ascanf_Function *af;
	if( !fp)
		return(0);
	fprintf( fp, "%s*** ascanf functions:\n", prefix);
	for( i= 0; i< ascanf_Functions && ascanf_FunctionList; i++){
		af= &ascanf_FunctionList[i];
		while( af){
			fprintf( fp, "%s%s\t(%s)\n", prefix, (af->usage)? af->usage : af->name, vars_Find_Symbol(af->function) );
			af= af->cdr;
		}
	}
	return(1);
}

int add_ascanf_functions( ascanf_Function *array, int n)
{  int i;
   ascanf_Function *af= &vars_ascanf_Functions[ascanf_Functions-1];
	for( i= n-2; i>= 0; i--){
		array[i].cdr= &array[i+1];
	}
	array[n-1].cdr= af->cdr;
	af->cdr= array;
	return( n);
}

/* ascanf(n, s, a) (ArrayScanf) functions; read a maximum of <n>
 * values from buffer 's' into array 'a'. Multiple values must be
 * separated by a comma; whitespace is ignored. <n> is updated to
 * contain the number of values actually read; this is also returned
 * unless an error occurs, in which case EOF is returned.
 * NOTE: cascanf(), dascanf() and lascanf() correctly handle mixed decimal and
 * hex values ; hex values must be preceded by '0x'
 \ 941129: replaced all "*s== ' ' || *s== '\t'" by "isspace(*s)" for skipping
 \ whitespace.
 */

/* read multiple characters (bytes)	*/
int cascanf( int *n, char *s, char *a, char *ch)
//int *n;
//char *s;
//char *a, *ch;
{	int r= 0, i= 0, j= 1;
	char *q;
	int A;
	static int level= 0;
	int Cws;
	char *lss;

	if( !a || !s || !*s){
		*n= 0;
		return( EOF);
	}
	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	level++;
	for( i= 0, q= ascanf_index( s, ascanf_separator); i< *n && j!= EOF && q && *s; a++, i++){
		/* temporarily discard the rest of the buffer	*/
		*q= '\0';
		/* skip whitespace	*/
		for( ; isspace((unsigned char)*s) && *s; s++);
		RESET_CHANGED_FLAG( ch, i);
		if( *s ){
		  double B= (double) *a;
			A= (int) *a;
			if( reset_ascanf_self_value ){
				ascanf_self_value= B;
			}
			if( reset_ascanf_index_value ){
				ascanf_index_value= i;
			}
			if( !strncmp( s, "0x", 2) ){
				s+= 2;
				j= sscanf( s, "%x", &A);
			}
			else if( check_for_ascanf_function( i, &s, &B, &j, "cascanf") ){
				r+= 1;
				A= d2int(B);
			}
			else
				j= sscanf( s, "%d", &A);
			if( j!= EOF)
				r+= j;
			SET_CHANGED_FLAG( ch, i, A, *a, j);
			*a= (char) A;
		}
		*q= ascanf_separator;					/* restore	*/
		s= q+ 1;					/* next number ?	*/
		q= ascanf_index( s, ascanf_separator);			/* end of next number	*/
	}
	for( ; isspace((unsigned char)*s) && *s; s++);
	RESET_CHANGED_FLAG( ch, i);
	if( !q && i< *n && *s){
	  double B= (double) *a;
		A= (int) *a;
		if( reset_ascanf_self_value ){
			ascanf_self_value= B;
		}
		if( reset_ascanf_index_value ){
			ascanf_index_value= i;
		}
		if( !strncmp( s, "0x", 2) ){
			s+= 2;
			j= sscanf( s, "%x", &A);
		}
		else if( check_for_ascanf_function( i, &s, &B, &j, "cascanf") ){
			r+= 1;
			A= d2int(B);
		}
		else
			j= sscanf( s, "%d", &A);
		SET_CHANGED_FLAG( ch, i, A, *a, j);
		*a++= A;
		if( j!= EOF)
			r+= j;
	}
	if( !level){
		reset_ascanf_index_value= True;
		reset_ascanf_self_value= True;
	}
	level--;
	CX_END_WRITABLE(Cws);
	if( r< *n){
		*n= r;
		return( EOF);
	}
	return( r);
}

#ifndef MCH_AMIGA
#	define SHORT_DECI	"%hd"
#	define SHORT_HEXA	"%hx"
#else
#	define SHORT_DECI	"%d"
#	define SHORT_HEXA	"%x"
#endif

/* read multiple 16 bits integers (dec or hex)	*/
int dascanf( n, s, a, ch)
int *n;
char *s;
short *a;
char *ch;
{	int r= 0, i= 0, j= 1;
	char *q;
	short A;
	static int level= 0;
	int Cws;
	char *lss;

	if( !a || !s || !*s){
		*n= 0;
		return( EOF);
	}
	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	level++;
	for( i= 0, q= ascanf_index( s, ascanf_separator); i< *n && j!= EOF && q && *s; a++, i++){
		*q= '\0';
		/* skip whitespace	*/
		for( ; isspace((unsigned char)*s) && *s; s++);
		RESET_CHANGED_FLAG( ch, i);
		if( *s ){
		  double B= (double) *a;
			A= (short) *a;
			if( reset_ascanf_self_value ){
				ascanf_self_value= B;
			}
			if( reset_ascanf_index_value ){
				ascanf_index_value= i;
			}
			if( !strncmp( s, "0x", 2) ){
				s+= 2;
				j= sscanf( s, SHORT_HEXA, &A);
			}
			else if( check_for_ascanf_function( i, &s, &B, &j, "dascanf") ){
				r+= 1;
				A= d2short( B);
			}
			else
				j= sscanf( s, SHORT_DECI, &A);
			if( j!= EOF)
				r+= j;
			SET_CHANGED_FLAG( ch, i, A, *a, j);
			*a= A;
		}
		*q= ascanf_separator;
		s= q+ 1;
		q= ascanf_index( s, ascanf_separator);
	}
	for( ; isspace((unsigned char)*s) && *s; s++);
	RESET_CHANGED_FLAG( ch, i);
	if( !q && i< *n && *s){
	  double B= (double) *a;
		A= (short) *a;
		if( reset_ascanf_self_value ){
			ascanf_self_value= B;
		}
		if( reset_ascanf_index_value ){
			ascanf_index_value= i;
		}
		if( !strncmp( s, "0x", 2) ){
			s+= 2;
			j= sscanf( s, SHORT_HEXA, &A );
		}
		else if( check_for_ascanf_function( i, &s, &B, &j, "dascanf") ){
			r+= 1;
			A= d2short( B);
		}
		else
			j= sscanf( s, SHORT_DECI, &A );
		if( j!= EOF)
			r+= j;
		SET_CHANGED_FLAG( ch, i, A, *a, j);
		*a++= A;
	}
	if( !level){
		reset_ascanf_index_value= True;
		reset_ascanf_self_value= True;
	}
	level--;
	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	if( r< *n){
		*n= r;
		return( EOF);
	}
	return( r);
}

/* read multiple 32 bits long integers (dec or hex)	*/
int lascanf( n, s, a, ch)
int *n;
char *s;
long *a;
char *ch;
{	int r= 0, i= 0, j= 1;
	char *q;
	long A;
	static int level= 0;
	int Cws;
	char *lss;

	if( !a || !s || !*s){
		*n= 0;
		return( EOF);
	}
	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	level++;
	for( i= 0, q= ascanf_index( s, ascanf_separator); i< *n && j!= EOF && q && *s; a++, i++){
		*q= '\0';
		for( ; isspace((unsigned char)*s) && *s; s++);
		RESET_CHANGED_FLAG( ch, i);
		if( *s ){
		  double B= (double) *a;
			A= (long) *a;
			if( reset_ascanf_self_value ){
				ascanf_self_value= B;
			}
			if( reset_ascanf_index_value ){
				ascanf_index_value= i;
			}
			if( !strncmp( s, "0x", 2) ){
				s+= 2;
				j= sscanf( s, "%lx", &A );
			}
			else if( check_for_ascanf_function( i, &s, &B, &j, "lascanf") ){
				r+= 1;
				A= (long) B;
			}
			else
				j= sscanf( s, "%ld", &A );
			if( j!= EOF)
				r+= j;
			SET_CHANGED_FLAG( ch, i, A, *a, j);
			*a= A;
		}
		*q= ascanf_separator;
		s= q+ 1;
		q= ascanf_index( s, ascanf_separator);
	}
	for( ; isspace((unsigned char)*s) && *s; s++);
	RESET_CHANGED_FLAG( ch, i);
	if( !q && i< *n && *s){
	  double B= (double) *a;
		A= (long) *a;
		if( reset_ascanf_self_value ){
			ascanf_self_value= B;
		}
		if( reset_ascanf_index_value ){
			ascanf_index_value= i;
		}
		if( !strncmp( s, "0x", 2) ){
			s+= 2;
			j= sscanf( s, "%lx", &A );
		}
		else if( check_for_ascanf_function( i, &s, &B, &j, "lascanf") ){
			r+= 1;
			A= (long) B;
		}
		else
			j= sscanf( s, "%ld", &A );
		if( j!= EOF)
			r+= j;
		SET_CHANGED_FLAG( ch, i, A, *a, j);
		*a++= A;
	}
	if( !level){
		reset_ascanf_index_value= True;
		reset_ascanf_self_value= True;
	}
	level--;
	CX_END_WRITABLE(Cws);
	if( r< *n){
		*n= r;
		return( EOF);
	}
	return( r);
}

/* read multiple 32 bits long hexadecimal integers */
int xascanf( n, s, a, ch)
int *n;
char *s;
long *a;
char *ch;
{	int r= 0, i= 0, j= 1;
	char *q;
	long A;
	static int level= 0;
	int Cws;
	char *lss;

	if( !a || !s || !*s){
		*n= 0;
		return( EOF);
	}
	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	level++;
	for( i= 0, q= ascanf_index( s, ascanf_separator); i< *n && j!= EOF && q && *s; a++, i++){
		*q= '\0';
		for( ; isspace((unsigned char)*s) && *s; s++);
		RESET_CHANGED_FLAG( ch, i);
		if( *s ){
		  double B= (double) *a;
			A= (long) *a;
			if( reset_ascanf_self_value ){
				ascanf_self_value= B;
			}
			if( reset_ascanf_index_value ){
				ascanf_index_value= i;
			}
			if( check_for_ascanf_function( i, &s, &B, &j, "xascanf") ){
				r+= 1;
				A= (long) B;
			}
			else if( (j= sscanf( s, "0x%lx", &A ))!= EOF)
				r+= j;
			SET_CHANGED_FLAG( ch, i, A, *a, j);
			*a= A;
		}
		*q= ascanf_separator;
		s= q+ 1;
		q= ascanf_index( s, ascanf_separator);
	}
	for( ; isspace((unsigned char)*s) && *s; s++);
	RESET_CHANGED_FLAG( ch, i);
	if( !q && i< *n && *s){
	  double B= (double) *a;
		A= (long) *a;
		if( reset_ascanf_self_value ){
			ascanf_self_value= B;
		}
		if( reset_ascanf_index_value ){
			ascanf_index_value= i;
		}
		if( check_for_ascanf_function( i, &s, &B, &j, "xascanf") ){
			r+= 1;
			A= (long) B;
		}
		else if( ( j= sscanf( s, "0x%lx", &A ))!= EOF)
			r+= j;
		SET_CHANGED_FLAG( ch, i, A, *a, j);
		*a++= A;
	}
	if( !level){
		reset_ascanf_index_value= True;
		reset_ascanf_self_value= True;
	}
	level--;
	CX_END_WRITABLE(Cws);
	if( r< *n){
		*n= r;
		return( EOF);
	}
	return( r);
}

static int read_fraction(char *s, double *A, int *r, char *caller, int *n, pointer array)
{ double B, P;
  int j= 0;
	if( (j= sscanf( s, FLOFMT"'"FLOFMT"/"FLOFMT, &P, A, &B ))!= 3){
		j= sscanf( s, FLOFMT"/"FLOFMT, A, &B );
	}
	if( j== 2 || j== 3 ){
		*r+= 1;
		if( B ){
			*A= *A/ B;
			if( j== 3 ){
				if( P< 0 ){
					*A= P- *A;
				}
				else{
					*A+= P;
				}
			}
		}
		else{
			set_Inf( *A, (*A)? *A : 1);
		}
	}
	else{
		fprintf( vars_errfile, "%s(%d,\"%s\",%s)[%d]: incomplete fraction %s/%s returns %s\n",
			caller,
			*n, s, vars_Find_Symbol( array), *r,
			d2str( *A, "%g", NULL),
			d2str( B, "%g", NULL),
			d2str( *A, "%g", NULL)
		);
	}
	return(j);
}

/* read multiple floating point values: floats	*/
/* since this function is younger than its brother that
 \ reads doubles, and since that brother was named
 \ fascanf(), this one had to be called hfascanf().
 \ After all, if a double(float) is called a float (f),
 \ than a float - being half a double - is a halffloat (hf)
 */
int hfascanf( n, s, a, ch)
int *n;							/* max # of elements in a	*/
char *s;					/* input string	*/
float *a;					/* target array	*/
char *ch;
{	int i= 0, r= 0, j= 1, sign= 1;
	char *q;
	double A;
	pointer aa= (pointer) a;
	static int level= 0;
	int Cws;
	char *lss;

	if( !a || !s || !*s){
		*n= 0;
		return( EOF);
	}
	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	level++;
	for( i= 0, q= ascanf_index( s, ascanf_separator); i< *n && j!= EOF && q && *s; a++, i++ ){
		*q= '\0';
		for( ; isspace((unsigned char)*s) && *s; s++);
		RESET_CHANGED_FLAG( ch, i);
		if( *s ){
		  char *cs= s, *ss= s;
			sign= 1;
			switch( *s ){
				case '+':
					sign= 1;
					ss++;
					break;
				case '-':
					sign= -1;
					ss++;
					break;
			}
			A= (double) *a;
			if( reset_ascanf_self_value ){
				ascanf_self_value= A;
			}
			if( reset_ascanf_index_value ){
				ascanf_index_value= i;
			}
			if( !strncmp( ss, "NaN", 3) ){
				set_NaN( A );
				if( sign== -1 ){
					I3Ed(A)->s.s= 1;
				}
				r+= (j= 1);
			}
			else if( !strncmp( ss, "Inf", 3) ){
				set_Inf( A, sign);
				r+= (j= 1);
			}
			else if( check_for_ascanf_function( i, &cs, &A, &j, "fascanf") ){
				r+= 1;
			}
			else if( !strncmp( ss, "0x", 2) ){
			  long l;
				s+= 2;
				if( !(j= sscanf( s, "%lx", &l))!= EOF){
					r+= j;
					A= (double) sign* l;
				}
			}
			else if( index( s, '/') ){
				j= read_fraction( s, &A, &r, "hfascanf", n, aa);
			}
			else{
				if( (j= sscanf( s, FLOFMT, &A ))!= EOF)
					r+= j;
			}
			SET_CHANGED_FLAG( ch, i, A, (double) *a, j);
			*a= (float) A;
		}
		*q= ascanf_separator;
		s= q+ 1;
		q= ascanf_index( s, ascanf_separator);
	}
	for( ; isspace((unsigned char)*s) && *s; s++);
	RESET_CHANGED_FLAG( ch, i);
	if( !q && i< *n && *s){
	  char *cs= s, *ss= s;
		sign= 1;
		switch( *s ){
			case '+':
				sign= 1;
				ss++;
				break;
			case '-':
				sign= -1;
				ss++;
				break;
		}
		A= (double) *a;
		if( reset_ascanf_self_value ){
			ascanf_self_value= A;
		}
		if( reset_ascanf_index_value ){
			ascanf_index_value= i;
		}
		if( !strncmp( ss, "NaN", 3) ){
			set_NaN( A);
			if( sign== -1 ){
				I3Ed(A)->s.s= 1;
			}
			r+= 1;
		}
		else if( !strncmp( ss, "Inf", 3) ){
			set_Inf( A, sign);
			r+= 1;
		}
		else if( check_for_ascanf_function( i, &cs, &A, &j, "fascanf") ){
			r+= 1;
		}
		else if( !strncmp( ss, "0x", 2) ){
		  long l;
			s+= 2;
			if( !(j= sscanf( s, "%lx", &l))!= EOF){
				r+= j;
				A= (double) sign* l;
			}
		}
		else if( index( s, '/') ){
			j= read_fraction( s, &A, &r, "hfascanf", n, aa);
		}
		else{
			if( (j= sscanf( s, FLOFMT, &A ))!= EOF)
				r+= j;
		}
		SET_CHANGED_FLAG( ch, i, A, (double) *a, j);
		*a++= (float) A;
	}
	if( !level){
		reset_ascanf_index_value= True;
		reset_ascanf_self_value= True;
	}
	level--;
	CX_END_WRITABLE(Cws);
	if( r< *n){
		*n= r;					/* not enough read	*/
		return( EOF);				/* so return EOF	*/
	}
	return( r);
}

/* read multiple floating point values	*/
int fascanf( n, s, a, ch)
int *n;							/* max # of elements in a	*/
char *s;					/* input string	*/
double *a;					/* target array	*/
char *ch;
{	int i= 0, r= 0, j= 1, sign= 1;
	char *q;
	double A;
	pointer aa= (pointer) a;
	static int level= 0;
	int Cws;
	char *lss;

	if( !a || !s || !*s){
		*n= 0;
		return( EOF);
	}

	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	level++;
	for( i= 0, q= ascanf_index( s, ascanf_separator); i< *n && j!= EOF && q && *s; a++, i++ ){
		*q= '\0';
		for( ; isspace((unsigned char)*s) && *s; s++);
		RESET_CHANGED_FLAG( ch, i);
		if( *s ){
		  char *cs= s, *ss= s;
			sign= 1;
			switch( *s ){
				case '+':
					sign= 1;
					ss++;
					break;
				case '-':
					sign= -1;
					ss++;
					break;
			}
			A= *a;
			if( reset_ascanf_self_value ){
				ascanf_self_value= A;
			}
			if( reset_ascanf_index_value ){
				ascanf_index_value= i;
			}
			if( !strncmp( ss, "NaN", 3) ){
				set_NaN( A );
				if( sign== -1 ){
					I3Ed(A)->s.s= 1;
				}
				r+= (j= 1);
			}
			else if( !strncmp( ss, "Inf", 3) ){
				set_Inf( A, sign);
				r+= (j= 1);
			}
			else if( check_for_ascanf_function( i, &cs, &A, &j, "fascanf") ){
				r+= 1;
			}
			else if( !strncmp( ss, "0x", 2) ){
			  long l;
				s+= 2;
				if( !(j= sscanf( s, "%lx", &l))!= EOF){
					r+= j;
					A= (double) sign* l;
				}
			}
			else if( index( s, '/') ){
				j= read_fraction( s, &A, &r, "hfascanf", n, aa);
			}
			else{
				if( (j= sscanf( s, FLOFMT, &A ))!= EOF)
					r+= j;
			}
			SET_CHANGED_FLAG( ch, i, A, *a, j);
			*a= A;
		}
		*q= ascanf_separator;
		s= q+ 1;
		q= ascanf_index( s, ascanf_separator);
	}
	for( ; isspace((unsigned char)*s) && *s; s++);
	RESET_CHANGED_FLAG( ch, i);
	if( !q && i< *n && *s){
	  char *cs= s, *ss= s;
		sign= 1;
		switch( *s ){
			case '+':
				sign= 1;
				ss++;
				break;
			case '-':
				sign= -1;
				ss++;
				break;
		}
		A= *a;
		if( reset_ascanf_self_value ){
			ascanf_self_value= A;
		}
		if( reset_ascanf_index_value ){
			ascanf_index_value= i;
		}
		if( !strncmp( ss, "NaN", 3) ){
			set_NaN( A);
			if( sign== -1 ){
				I3Ed(A)->s.s= 1;
			}
			r+= 1;
		}
		else if( !strncmp( ss, "Inf", 3) ){
			set_Inf( A, sign);
			r+= 1;
		}
		else if( check_for_ascanf_function( i, &cs, &A, &j, "fascanf") ){
			r+= 1;
		}
		else if( !strncmp( ss, "0x", 2) ){
		  long l;
			s+= 2;
			if( !(j= sscanf( s, "%lx", &l))!= EOF){
				r+= j;
				A= (double) sign* l;
			}
		}
		else if( index( s, '/') ){
			j= read_fraction( s, &A, &r, "hfascanf", n, aa);
		}
		else{
			if( (j= sscanf( s, FLOFMT, &A ))!= EOF)
				r+= j;
		}
		SET_CHANGED_FLAG( ch, i, A, *a, j);
		*a++= A;
	}
	if( !level){
		reset_ascanf_index_value= True;
		reset_ascanf_self_value= True;
	}
	level--;
	CX_END_WRITABLE(Cws);
	if( r< *n){
		*n= r;					/* not enough read	*/
		return( EOF);				/* so return EOF	*/
	}
	return( r);
}

/* read multiple values in an array of pointers to a basic type	*/
int ascanf( type, n, s, a, ch)
ObjectTypes type;
int *n;
char *s;
char **a;
char *ch;
{	int N, i= 0, r= 0, j= 1;
	char *q, *end;
	VariableType A;
	VariableSet B;
	int Cws;
	char *lss;

	if( !a || !s || !*s){
		*n= 0;
		return( EOF);
	}
	CX_MAKE_WRITABLE(s, lss);
	CX_DEFER_WRITABLE(Cws);

	end= &s[ strlen(s) ];
	for( i= 0, q= ascanf_index( s, ascanf_separator); i< *n && j!= EOF && q && *q && *s && q< end; a++, i++ ){
		*q= '\0';
		for( ; isspace((unsigned char)*s) && *s; s++);
		RESET_CHANGED_FLAG( ch, i);
		if( *s && *a ){
			N= 1;
			A.ptr= (void*) *a;
			memcpy( &B, *a, sizeof(VariableSet) );
			switch( type){
				case _charP:
					r+= (j= cascanf( &N, s, A.c, (ch)? &ch[i] : NULL ));
					SET_CHANGED_FLAG( ch, i, *A.c, B.c, j);
					break;
				case _shortP:
					r+= (j= dascanf( &N, s, A.s, (ch)? &ch[i] : NULL ));
					SET_CHANGED_FLAG( ch, i, *A.s, B.s, j);
					break;
				case _intP:
					if( sizeof(int)== 2)
						r+= (j= dascanf( &N, s, (short*) A.i, (ch)? &ch[i] : NULL ));
					else
						r+= (j= lascanf( &N, s, (long*) A.i, (ch)? &ch[i] : NULL ));
					SET_CHANGED_FLAG( ch, i, *A.i, B.i, j);
					break;
				case _longP:
					r+= (j= lascanf( &N, s, A.l, (ch)? &ch[i] : NULL ));
					SET_CHANGED_FLAG( ch, i, *A.l, B.l, j);
					break;
				case _floatP:
					r+= (j= hfascanf( &N, s, A.f, (ch)? &ch[i] : NULL ));
					SET_CHANGED_FLAG( ch, i, *A.f, B.f, j);
					break;
				case _doubleP:
					r+= (j= fascanf( &N, s, A.d, (ch)? &ch[i] : NULL ));
					SET_CHANGED_FLAG( ch, i, *A.d, B.d, j);
					break;
				default:
					if( type< MaxObjectType){
						fprintf( cx_stderr, "vars::ascanf(): illegal type %s\n",
							ObjectTypeNames[type]
						);
					}
					else{
						fprintf( cx_stderr, "vars::ascanf(): illegal type %d\n",
							type 
						);
					}
					*n= 0;
					reset_ascanf_index_value= True;
					CX_END_WRITABLE(Cws);
					return(0);
						
			}
		}
		*q= ascanf_separator;
		s= q+ 1;
		q= ascanf_index( s, ascanf_separator);
	}
	for( ; isspace((unsigned char)*s) && *s; s++);
	RESET_CHANGED_FLAG( ch, i);
	if( !q && i< *n && *s && *a){
		N= 1;
		A.ptr= (void*) *a;
		memcpy( &B, *a, sizeof(VariableSet) );
		switch( type){
			case _charP:
				r+= (j= cascanf( &N, s, A.c, (ch)? &ch[i] : NULL ));
				SET_CHANGED_FLAG( ch, i, *A.c, B.c, j);
				break;
			case _shortP:
				r+= (j= dascanf( &N, s, A.s, (ch)? &ch[i] : NULL ));
				SET_CHANGED_FLAG( ch, i, *A.s, B.s, j);
				break;
			case _intP:
				if( sizeof(int)== 2)
					r+= (j= dascanf( &N, s, (short*) A.i, (ch)? &ch[i] : NULL ));
				else
					r+= (j= lascanf( &N, s, (long*) A.i, (ch)? &ch[i] : NULL ));
				SET_CHANGED_FLAG( ch, i, *A.i, B.i, j);
				break;
			case _longP:
				r+= (j= lascanf( &N, s, A.l, (ch)? &ch[i] : NULL ));
				SET_CHANGED_FLAG( ch, i, *A.l, B.l, j);
				break;
			case _floatP:
				r+= (j= hfascanf( &N, s, A.f, (ch)? &ch[i] : NULL ));
				SET_CHANGED_FLAG( ch, i, *A.f, B.f, j);
				break;
			case _doubleP:
				r+= (j= fascanf( &N, s, A.d, (ch)? &ch[i] : NULL ));
				SET_CHANGED_FLAG( ch, i, *A.d, B.d, j);
				break;
			default:
				if( type< MaxObjectType){
					fprintf( cx_stderr, "vars::ascanf(): illegal type %s\n",
						ObjectTypeNames[type]
					);
				}
				else{
					fprintf( cx_stderr, "vars::ascanf(): illegal type %d\n",
						type 
					);
				}
				*n= 0;
				reset_ascanf_index_value= True;
				CX_END_WRITABLE(Cws);
				return(0);
					
		}
	}
	CX_END_WRITABLE(Cws);
	if( r< *n){
		*n= r;					/* not enough read	*/
		return( EOF);				/* so return EOF	*/
	}
	return( r);
}

#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
SymbolTable_List VarsA_List[]= {
	FUNCTION_SYMBOL_ENTRY( ascanf),
	FUNCTION_SYMBOL_ENTRY( ascanf_add),
	FUNCTION_SYMBOL_ENTRY( ascanf_div),
	FUNCTION_SYMBOL_ENTRY( ascanf_fac),
	FUNCTION_SYMBOL_ENTRY( ascanf_function),
	FUNCTION_SYMBOL_ENTRY( ascanf_index),
	FUNCTION_SYMBOL_ENTRY( ascanf_mul),
	FUNCTION_SYMBOL_ENTRY( ascanf_pi),
	FUNCTION_SYMBOL_ENTRY( ascanf_pow),
	FUNCTION_SYMBOL_ENTRY( ascanf_random),
	FUNCTION_SYMBOL_ENTRY( ascanf_random_select),
	FUNCTION_SYMBOL_ENTRY( ascanf_uniform),
	FUNCTION_SYMBOL_ENTRY( ascanf_abnormal),
	FUNCTION_SYMBOL_ENTRY( ascanf_Index),
	FUNCTION_SYMBOL_ENTRY( ascanf_self),
	FUNCTION_SYMBOL_ENTRY( ascanf_current),
	FUNCTION_SYMBOL_ENTRY( ascanf_sub),
#ifndef VARS_STANDALONE
	FUNCTION_SYMBOL_ENTRY( ascanf_time),
	FUNCTION_SYMBOL_ENTRY( ascanf_elapsed),
#endif
	FUNCTION_SYMBOL_ENTRY( ascanf_radians),
	FUNCTION_SYMBOL_ENTRY( ascanf_degrees),
	FUNCTION_SYMBOL_ENTRY( ascanf_sin),
	FUNCTION_SYMBOL_ENTRY( ascanf_cos),
	FUNCTION_SYMBOL_ENTRY( ascanf_tan),
	FUNCTION_SYMBOL_ENTRY( ascanf_atan2),
	FUNCTION_SYMBOL_ENTRY( ascanf_len),
	FUNCTION_SYMBOL_ENTRY( ascanf_arg),
	FUNCTION_SYMBOL_ENTRY( ascanf_exp),
#ifndef WIN32
	FUNCTION_SYMBOL_ENTRY( ascanf_erf),
	FUNCTION_SYMBOL_ENTRY( ascanf_erfc),
#endif
	FUNCTION_SYMBOL_ENTRY( ascanf_log),
	FUNCTION_SYMBOL_ENTRY( ascanf_floor),
	FUNCTION_SYMBOL_ENTRY( ascanf_ceil),
	FUNCTION_SYMBOL_ENTRY( ascanf_min),
	FUNCTION_SYMBOL_ENTRY( ascanf_max),
	FUNCTION_SYMBOL_ENTRY( ascanf_dcmp),
	FUNCTION_SYMBOL_ENTRY( ascanf_fmod),
	FUNCTION_SYMBOL_ENTRY( ascanf_misc_fun),
	FUNCTION_SYMBOL_ENTRY( ascanf_if),
	FUNCTION_SYMBOL_ENTRY( ascanf_if2),
	FUNCTION_SYMBOL_ENTRY( ascanf_gt),
	FUNCTION_SYMBOL_ENTRY( ascanf_lt),
	FUNCTION_SYMBOL_ENTRY( ascanf_eq),
	FUNCTION_SYMBOL_ENTRY( ascanf_ge),
	FUNCTION_SYMBOL_ENTRY( ascanf_le),
	FUNCTION_SYMBOL_ENTRY( ascanf_and),
	FUNCTION_SYMBOL_ENTRY( ascanf_or),
	FUNCTION_SYMBOL_ENTRY( ascanf_xor),
	FUNCTION_SYMBOL_ENTRY( ascanf_not),
	FUNCTION_SYMBOL_ENTRY( ascanf_abs),
	FUNCTION_SYMBOL_ENTRY( ascanf_return),
	FUNCTION_SYMBOL_ENTRY( ascanf_progn),
	FUNCTION_SYMBOL_ENTRY( ascanf_whiledo),
	FUNCTION_SYMBOL_ENTRY( ascanf_dowhile),
	FUNCTION_SYMBOL_ENTRY( ascanf_print),
	FUNCTION_SYMBOL_ENTRY( ascanf_Eprint),
	FUNCTION_SYMBOL_ENTRY( ascanf_verbose_fnc),
#ifndef VARS_STANDALONE
	FUNCTION_SYMBOL_ENTRY( ascanf_matherr_fnc),
#endif
	FUNCTION_SYMBOL_ENTRY( ascanf_mem),
	FUNCTION_SYMBOL_ENTRY( ascanf_mxy),
	FUNCTION_SYMBOL_ENTRY( ascanf_setmxy),
	FUNCTION_SYMBOL_ENTRY( cascanf),
	FUNCTION_SYMBOL_ENTRY( check_for_ascanf_function),
	FUNCTION_SYMBOL_ENTRY( d2int),
	FUNCTION_SYMBOL_ENTRY( d2long),
	FUNCTION_SYMBOL_ENTRY( d2short),
	FUNCTION_SYMBOL_ENTRY( dascanf),
	FUNCTION_SYMBOL_ENTRY( fascanf),
	FUNCTION_SYMBOL_ENTRY( lascanf),
	FUNCTION_SYMBOL_ENTRY( show_ascanf_functions),
	FUNCTION_SYMBOL_ENTRY( xascanf),
	FUNCTION_SYMBOL_ENTRY( ascanf_sleep_once ),
	FUNCTION_SYMBOL_ENTRY( ascanf_set_interval ),
	FUNCTION_SYMBOL_ENTRY( ascanf_for_to ),
	FUNCTION_SYMBOL_ENTRY( ascanf_SS_get ),
	FUNCTION_SYMBOL_ENTRY( ascanf_SS_Mean ),
	FUNCTION_SYMBOL_ENTRY( ascanf_SS_Count ),
	FUNCTION_SYMBOL_ENTRY( ascanf_SS_min ),
/* 	FUNCTION_SYMBOL_ENTRY( ascanf_SS_pos_min ),	*/
	FUNCTION_SYMBOL_ENTRY( ascanf_SS_max ),
	FUNCTION_SYMBOL_ENTRY( ascanf_compress ),
	FUNCTION_SYMBOL_ENTRY( ascanf_lowpass ),
	FUNCTION_SYMBOL_ENTRY( ascanf_nlowpass ),
	FUNCTION_SYMBOL_ENTRY( ascanf_shunt ),
	P_TO_VARs_SYMBOL_ENTRY( ascanf_memory, _double, ASCANF_MAX_ARGS),
};
int VarsA_List_Length= SymbolTable_ListLength(VarsA_List);

register_A_fnc()
{
	Add_SymbolTable_List( VarsA_List, VarsA_List_Length );
}
#endif
