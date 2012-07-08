#ifndef VARS_STANDALONE
#	include <stdio.h>
#	include "local/cx.h"
#	include <local/varsedit.h>
#	include "varsintr.h"
#	include <local/mxt.h>
#	include <local/sincos.h>
#	include "local/Macros.h"
#else
#	include "vars-standalone.h"
#	include "varsedit.h"
#	include "varsintr.h"
#	include "mxt.h"
#	include "sincos.h"
#	include "ALLOCA.h"
#endif

#ifndef __ppc__
// #	define USE_SSE_AUTO
#	define USE_SSE3	// AMD CPUs don't have SSE4 and there is no way to detect them reliably at compile time except when using gcc?!
#	include "sse_mathfun/sse_mathfun.h"
#endif

#if defined(_HPUX_SOURCE) || defined(_AUX_SOURCE) || defined(_APOLLO_SOURCE)
#	define VARS1  11L
#else
#	define VARS1 10l
#endif
#ifndef VARS_STANDALONE
#	define VARLIST  6L
#else
#	define VARLIST  5L
#endif
#define VARS2  19L

extern Variable_t piets[VARS1], varlist[VARLIST], other_piets[VARS2];

extern int ascanf_arguments;

IDENTIFY("cx/vars/mxt test module");

int blabla( v, n, I, K)
Variable_t *v;
long n, I, K;
{  FILE *fp= v[I].Stdout;
   int i, k;
   char *fname, *basename;
	fprintf( fp, "Executing command #%d (%s[%d,%d]) for the %dth time with arguments '%s' - FILE \"%s\":\n",
		I, v[I].name, v[I].first, v[I].last,
		line_invocation,
		(v[I].args)?v[I].args : "<none>",
		Find_Symbol(fp)
	);
	for( k= i= 0; i< n; i++){
		print_var( fp, &v[i], 0);
		print_varMean( fp, &v[i]);
		k+= v[i].count;
	}
	fprintf( fp, "\tSum of all valid items is: %ld\n", k);
	i= 0;
#ifndef VARS_STANDALONE
	while( (fname= CX_nextfile( "./", &basename )) ){
		i+= 1;
	}
	fprintf( fp, "%d files in ./\n", i);
#endif
	return( (int) k);
}

int to_downcase( Variable_t *v, long n, long I, long K)
{  FILE *fp= v[I].Stdout;
	if( v[I].args ){
	  char *c= strdup(v[I].args);
		if( c ){
#ifndef VARS_STANDALONE
			downcase(c);
#else
			{ char *d= c;
				while(*d){
					*d= tolower(*d);
					d++;
				}
			}
#endif
			fprintf( fp, "%s\n", c );
			lib_free(c);
		}
	}
	else{
		fprintf( fp, "\tJust what do you think the downcase of nothing is???!\n" );
	}
	return( 1 );
}

int oops( v, n, i, k)
Variable_t *v;
long n, i, k;
{ static char buf[2048], level= 0;
	level+= 1;
	fprintf( v[i].Stdout, "\aOops! %s[%d,%d]%c{%s} => %ld items changed in variable #%d (linecall %d)!\n",
		v[i].name, v[i].first, v[i].last, v[i].Operator, v[i].args,
		k, i, v[i].serial
	);
	if( v[i].bounds_set ){
	  int j;
	  int hit= False;
		for( j= 0; j< v[i].maxcount; j++ ){
			if( var_Value( &v[i], j)< v[i].min_val ||
				var_Value( &v[i], j)> v[i].max_val
			){
				hit= True;
			}
		}
		if( hit ){
			fprintf( v[i].Stdout, "\aOops! Values should remain within the interval [%g,%g]\n",
				v[i].min_val, v[i].max_val
			);
		}
	}
	print_var( v[i].Stdout, (Variable_t*) &v[i], 2);
	if( streq( v[i].name, "0xPietje") ){
		print_var( v[i].Stdout, find_named_var( "Pietje", True, v, n, &k, stderr), 0);
	}
	else if( streq( v[i].name, "Pietje") ){
		print_var( v[i].Stdout, find_named_var( "0xPietje", True, v, n, &k, stderr), 0);
	}
	else if( streq( v[i].name, "0xfloat") ){
		print_var( v[i].Stdout, find_named_var( "float", True, v, n, &k, stderr), 0);
	}
	else if( streq( v[i].name, "float") ){
		print_var( v[i].Stdout, find_named_var( "0xfloat", True, v, n, &k, stderr), 0);
	}
	else if( v[i].args && streq( v[i].name, "double_p") && v[i].parent ){
		sprintf( buf, "%s[%d,%d]%c{%s}", v[i].name, v[i].first, v[i].last, v[i].Operator, v[i].args );
	}
	else if( v[i].var== varlist && buf[0] ){
		if( level== 1 ){
		  long id= 0;
		  Variable_t *V= find_named_var( "double_p", True, varlist, VARLIST, &id, stderr);
		  long changes= 0;
			if( V ){
				fprintf( v[i].Stdout, "--- Doing \"%s\" bypassing parent \"%s\"\n", buf, v[i].name );
				fflush( v[i].Stdout );
				{ FILE *stde= stderr;
					_parse_varline( buf, V, -1, &changes, False, &v[i].Stdout, &stde );
				}
				fprintf( v[i].Stdout, "--- Done\n" );
				fflush( v[i].Stdout );
				buf[0]= '\0';
			}
		}
		else{
			fprintf( v[i].Stdout, "=== but parent \"%s\" handler is called anyway!\n", v[i].name );
		}
	}
	v[i].count= 0;
	level-= 1;
	return( 0);
}

int ctype_fun( v, n, i, k)
Variable_t *v;
long n, i, k;
{  char *a, c;
   int r1= 0, r2= 0;
	if( v[i].args ){
		a= v[i].args;
		if( !strncasecmp( a, "0x", 2) ){
		  int C;
			sscanf( &a[2], "%x", &C);
			c= (char) C;
		}
		else{
			c= *a;
		}
		switch( v[i].id ){
			case 5:
				r1= isalpha(*a);
				r2= isalpha(c);
				break;
			case 6:
				r1= isupper(*a);
				r2= isupper(c);
				break;
			case 7:
				r1= islower(*a);
				r2= islower(c);
				break;
			case 8:
				r1= isdigit(*a);
				r2= isdigit(c);
				break;
			case 9:
				r1= isxdigit(*a);
				r2= isxdigit(c);
				break;
			case 10:
				r1= isalnum(*a);
				r2= isalnum(c);
				break;
			case 11:
				r1= isspace((unsigned char)*a);
				r2= isspace((unsigned char)c);
				break;
			case 12:
				r1= ispunct(*a);
				r2= ispunct(c);
				break;
			case 13:
				r1= isprint(*a);
				r2= isprint(c);
				break;
			case 14:
				r1= isgraph(*a);
				r2= isgraph(c);
				break;
			case 15:
				r1= iscntrl(*a);
				r2= iscntrl(c);
				break;
			case 16:
				r1= isascii(*a);
				r2= isascii(c);
				break;
		}
		fprintf( v[i].Stdout, "%s(*\"%s\")= %d(0x%x) ; %s(%c{0x%x})= %d(0x%x)\n",
			v[i].name, a, r1, r1, v[i].name, c, (int)c, r2, r2
		);
#if defined(_HPUX_SOURCE) || defined(_APOLLO_SOURCE)
		fprintf( v[i].Stdout, "ctype[%u]=%d (0x%x)\n",
			(unsigned char) c, __ctype[c], __ctype[c]
		);
#elif _AUX_SOURCE
		fprintf( v[i].Stdout, "ctype[%u]=%d (0x%x)\n",
			(unsigned char) c, (_ctype+1)[(unsigned)c], (_ctype+1)[(unsigned)c]
		);
#endif
		return(r1);
	}
	return(0);
}

char stdinput[128];

int do_input( v, n, i, k)
Variable_t *v;
long n, i, k;
{  int N= fwrite( stdinput, 1, strlen(stdinput), stdin);
	if( !N)
		fprintf( v[i].Stderr, "Error: %s\n", serror() );
	else
		fprintf( v[i].Stderr, "%d bytes written to stdin\n", N);
	return(0);
}

int _addlog( v, n, i, k)
Variable_t *v;
long n, i, k;
{
#ifndef VARS_STANDALONE
	Addlog( (char*) v[i].var );
#else
	fprintf( v[i].Stderr, "Sorry, no logfile support compiled in.\n" );
#endif
	return(0);
}

int time_print( v, n, i, k)
_Variable *v;
long n, i, k;
{	char buf[80];
  double *Pietje= v[i].var.d;
	fprintf( v[i].Stdout, "CLIP_EXPR( Pietje[0], Pietje[1], Pietje[2], Pietje[3])\n");
	CLIP_EXPR( Pietje[0], Pietje[1], Pietje[2], Pietje[3]);
	print_var( v[i].Stdout, (Variable_t*) &v[i], 1);
#ifndef VARS_STANDALONE
	fprintf( v[i].Stdout, "\tTime version: %s ; %s\n",
		parse_seconds( Pietje[0], buf), parse_seconds( Pietje[1], NULL)
	);
#endif
	fflush( v[i].Stdout);
	return(0);
}

int include_file( char *name )
{ FILE *vf= vars_file, *fp;
  Parfile_Context pc= parfile_context, lpc;
  extern int edit_variables_maxlines;
  int changes= 0, dum= -1;
  int ispipe= False;
  static int level= 0;
  char *prompt;

	while( name && isspace((unsigned char)*name) )
		name++;
	if( !name || !*name )
		return(0);
	edit_variables_interim= NULL;
	if( name[0]== '|' ){
		fp= Open_Pipe_From( &name[1], NULL );
		ispipe= 1;
		prompt= "<| ";
	}
	else if( sscanf( name, "lines=%d", &dum )== 1 && dum> 0 ){
		edit_variables_maxlines= dum;
		fp= NULL;
	}
	else if( !strcmp( &name[ strlen(name)-3 ], ".gz" ) ){
	  /* This should actually be done by looking _in_ the file. But
	   \ then this is easier (we don't have to unget the magic number)
	   */
#ifdef _GCC_
	  char command[strlen(name)+ 15];
#else
	  char command[1024];
#endif
		sprintf( command, "|gunzip -v < \"%s\"", name );
		fp= Open_Pipe_From( &command[1], NULL );
		name= strdup(command);
		ispipe= 2;
		prompt= "<| ";
	}
	else{
		fp= fopen( name, "r");
		prompt= "<< ";
	}
	if( fp ){
	  int depth;
	  FILE *rfp;
#ifdef VARS_STANDALONE
	  ALLOCA( logmsg, char, strlen(name)*2, lmlen);
#endif
		level++;
		vars_file= fp;
		parfile_context= *dup_Parfile_Context( &lpc, fp, name);
		parposition= 0;
		parlineposition= 0;
		sprintf( logmsg, "include_file(%s|%d)", name, level );
		PUSH_TRACE( logmsg, depth);
		if( (changes= edit_variables_list( prompt, VARS1, (Variable_t*) piets, VARS2, (Variable_t*)other_piets, 0)) ){
			fprintf( stderr,
				"include_file(%s|%d); (%d) Variables changed:\n",
				name, level, changes
			);
		}
		else{
			fputs( "Nothing changed...", stderr);
			if( errno){
				fprintf( stderr, "(%s)", serror() );
			}
			fputc( '\n', stderr);
			fflush( stderr);
		}
		rfp= parfile;
		parfile_context= pc;
		if( rfp== fp ){
			if( ispipe ){
				Close_Pipe(fp);
				if( ispipe== 2 ){
					free( name );
				}
				ispipe= False;
			}
			else{
				fclose(fp);
			}
		}
		vars_file= vf;
		level--;
		edit_variables_maxlines= -1;
		POP_TRACE(depth);
	}
	else if( dum<= 0 ){
		fprintf( stderr, "include_file(%s|%d): %s\n",
				name, level, serror()
		);
	}
	return(changes);
}

int do_include_file( v, n, id, C)
Variable_t *v;
long n, id, C;
{ char *name= v[id].args;
  static int level= 0;
  int ret;

	if( name ){
		level++;
		ret= include_file(name);
		level--;
	}
	else{
		fprintf( v[id].Stdout, "usage: \"%s\"\n", v[id].description );
	}
	return( ret );
}

#define PITEMS	4
short pietje[PITEMS]= {1, 2, 3, 4};
double Pietje[PITEMS]= { 1024.0, 1.345e-12, 0.0 };
float Fietje[PITEMS]= { 1024.0, 1.345e-12, 0.0 };
char prompt[65]= ">> ";
extern Variable_t other_piets[VARS2];

char *char_p[65];
short *short_p[PITEMS];
int *int_p[PITEMS], int_[PITEMS];
long *long_p[PITEMS], long_[PITEMS];
double *double_p[PITEMS];
float fl[PITEMS];

long cycles= 2, steps= 3, clr= -1;
long *loop_counters[]= { &cycles, &steps, &clr };

int i1= 10, i2= 20;

extern VariableSelection *Reference_Variable;
extern int Reference_Variables;

Variable_t piets[VARS1]= {
	{ 0, SHORT_VAR, PITEMS,PITEMS, VARIABLE(pietje), "pietje", oops, 1, "some shorts", "jantje",
		True, -100, 100
	},
	{ 1, DOUBLE_VAR, PITEMS,PITEMS, (void*) Pietje, "Pietje", time_print, 1, "some doubles", "Jantje" },
	{ 2, HEX_VAR, PITEMS*sizeof(double)/sizeof(long), PITEMS*sizeof(double)/sizeof(long), Pietje, "0xPietje", oops, 1,
		"hex representation of Pietje"
	},
	{ 3, CHAR_VAR, 0, 0, NULL, "argv", oops, -1, "this program as started" },
	{ 4, CHAR_VAR, 0, 128, stdinput, "stdin", do_input, 1, "writes to stdin!!!"},
	{ 5, FLOAT_VAR, PITEMS,PITEMS, (void*) fl, "float", oops, 1, "some floats" },
	{ 6, HEX_VAR, PITEMS*sizeof(float)/sizeof(long), PITEMS*sizeof(float)/sizeof(long), fl, "0xfloat", oops, 1,
		"hex representation of float"
	},
	{ 7,	COMMAND_VAR,	0,	0,	(pointer) do_include_file,	"try's_include_file",	oops,	0,
		"try's_include_file[{lines=<n>}]{filename} or try's_include_file{[lines=<n>}]{|command}\n"
		"files are closed after each full line read to enable changes to be made *after* the current position!\n"
		"This is a version local to the try/test programme, without recursion protection!\n"
	},
	{ 8, FLOAT_VAR, PITEMS,PITEMS, (void*) Fietje, "Fietje", time_print, 1, "some floats" },
	{ 9, HEX_VAR, PITEMS*sizeof(float)/sizeof(long), PITEMS*sizeof(float)/sizeof(long), Fietje, "0xFietje", oops, 1,
		"hex representation of Fietje"
	},
#if defined(_HPUX_SOURCE) || defined(_AUX_SOURCE) || defined(_APOLLO_SOURCE)
	{ 10, UCHAR_VAR, 256, 256, NULL, "ctype", oops, 1, "Internal ctype buffer"},
#endif
  },
  varlist[VARLIST]= {
	{ 0, CHAR_PVAR, 64, 64, char_p, "char_p", oops, 1, "some character pointers" },
	{ 1, SHORT_PVAR, PITEMS, PITEMS, short_p, "short_p", oops, 1, "some short pointers\n\tto the elements of pietje" },
	{ 2, INT_PVAR, 0, PITEMS, int_p, "int_p", oops, 1, "some int pointers" },
	{ 3, LONG_PVAR, 0, PITEMS, long_p, "long_p", oops, 1, "some long pointers" },
	{ 4, DOUBLE_PVAR, PITEMS, PITEMS, double_p, "double_p", oops, 1, "some double pointers\n\tto the elements of Pietje" },
#ifndef VARS_STANDALONE
	{ 5, CHAR_VAR, 0, sizeof(logmsg), logmsg, "logmsg", _addlog, 1, "interface to the logging facilities" },
#endif
  },
  other_piets[VARS2]= {
	{ 0, CHAR_VAR, PITEMS*sizeof(double), PITEMS*sizeof(double), Pietje, "StrPietje", oops, 1, "char representation of Pietje" },
	{ 1, UCHAR_VAR, PITEMS*sizeof(double), PITEMS*sizeof(double), Pietje, "BPietje", oops, 1, "byte representation of Pietje" },
	{ 2, CHAR_VAR, 64, 64, prompt, "prompt", oops, 1, "the prompt as used" },
	{ 3, COMMAND_VAR, 0, 0, NULL, "PIETJE!", blabla, 1, "a sample command that \"does nothing\" ungracefully" },
	{ 4, VARIABLE_VAR, 0, VARLIST, varlist, "varlist", oops, 1, "a sample Variable list" },
	{ 5, COMMAND_VAR, 0, 0, NULL, "isalpha", ctype_fun, 1 },
	{ 6, COMMAND_VAR, 0, 0, NULL, "isupper", ctype_fun, 1 },
	{ 7, COMMAND_VAR, 0, 0, NULL, "islower", ctype_fun, 1 },
	{ 8, COMMAND_VAR, 0, 0, NULL, "isdigit", ctype_fun, 1 },
	{ 9, COMMAND_VAR, 0, 0, NULL, "isxdigit", ctype_fun, 1 },
	{ 10, COMMAND_VAR, 0, 0, NULL, "isalnum", ctype_fun, 1 },
	{ 11, COMMAND_VAR, 0, 0, NULL, "isspace", ctype_fun, 1 },
	{ 12, COMMAND_VAR, 0, 0, NULL, "ispunct", ctype_fun, 1 },
	{ 13, COMMAND_VAR, 0, 0, NULL, "isprint", ctype_fun, 1 },
	{ 14, COMMAND_VAR, 0, 0, NULL, "isgraph", ctype_fun, 1 },
	{ 15, COMMAND_VAR, 0, 0, NULL, "iscntrl", ctype_fun, 1 },
	{ 16, COMMAND_VAR, 0, 0, NULL, "isascii", ctype_fun, 1 },
	{ 17, COMMAND_VAR, 0, 0, NULL, "downcase", to_downcase, 1, "make a downcase version of the argument(s)" },
	{ 18,	LONG_PVAR,	3,	3,	(pointer) loop_counters,	"cycles",	oops,	1},
};

VariableSelection Sel[]= {
	{piets, -VARS1}, {varlist, -VARLIST}, {other_piets, -VARS2}
};

//#ifndef VARS_STANDALONE
SymbolTable_List try_list[]= {
	FUNCTION_SYMBOL_ENTRY( oops),
	FUNCTION_SYMBOL_ENTRY( time_print),
	FUNCTION_SYMBOL_ENTRY( do_input),
	FUNCTION_SYMBOL_ENTRY( _addlog),
	FUNCTION_SYMBOL_ENTRY( blabla),
	VARIABLEs_SYMBOL_ENTRY( piets, _variable, VARS1 ),
	VARIABLEs_SYMBOL_ENTRY( varlist, _variable, VARLIST ),
	VARIABLEs_SYMBOL_ENTRY( other_piets, _variable, VARS2 ),
};
//#endif

int try_lapsed( double args[ASCANF_MAX_ARGS], double *result )
{
#ifndef VARS_STANDALONE
	*result= Elapsed_Time();
#else
	*result= -1;
#endif
	return( 1 );
}

int try_exp( double args[ASCANF_MAX_ARGS], double *result )
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

int try_peit( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= Pietje[0];
		return( 1 );
	}
	else{
		  /* we don't need args[1], so we set it to 0
		   * (which prints nicer)
		   */
		args[1]= 0.0;
		*result= Pietje[0] * args[0];
		return( 1 );
	}
}

int try_sin( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= sin(0.0);
		return( 1 );
	}
	else{
		  /* we don't need args[1], so we set it to 0
		   * (which prints nicer)
		   */
		args[1]= 0.0;
		*result= sin( args[0]);
		return( 1 );
	}
}

int try_cxsin( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= cxsin(0.0, 0.0);
		return( 1 );
	}
	else{
		if( ascanf_arguments< 2 ){
			args[1]= 0;
		}
		*result= cxsin( args[0], args[1]);
		return( 1 );
	}
}

int try_cos( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= cos(0.0);
		return( 1 );
	}
	else{
		  /* we don't need args[1], so we set it to 0
		   * (which prints nicer)
		   */
		args[1]= 0.0;
		*result= cos( args[0]);
		return( 1 );
	}
}

int try_cxcos( double args[ASCANF_MAX_ARGS], double *result )
{
	if( !args){
		*result= cxcos(0.0, 0.0);
		return( 1 );
	}
	else{
		if( ascanf_arguments< 2 ){
			args[1]= 0;
		}
		*result= cxcos( args[0], args[1]);
		return( 1 );
	}
}

int try_cxsin2( double args[ASCANF_MAX_ARGS], double *result )
{ double s, c;
	if( !args){
		cxsincos( 0, 0, &s, &c );
		*result= s;
		return( 1 );
	}
	else{
		if( ascanf_arguments< 2 ){
			args[1]= 0;
		}
		cxsincos( args[0], args[1], &s, &c);
		*result= s;
		return( 1 );
	}
}

int try_cxcos2( double args[ASCANF_MAX_ARGS], double *result )
{ double s, c;
	if( !args){
		cxsincos( 0, 0, &s, &c );
		*result= c;
		return( 1 );
	}
	else{
		if( ascanf_arguments< 2 ){
			args[1]= 0;
		}
		cxsincos( args[0], args[1], &s, &c);
		*result= c;
		return( 1 );
	}
}

ascanf_Function try_ascanf_Function[]= {
	{ "exp", try_exp, 1, NOT_EOF, "exp[x]" },
	{ "lapsed", try_lapsed, 0, NOT_EOF, "lapsed" },
	{ "peit", try_peit, 1, NOT_EOF, "peit or peit[mulfac]" },
	{ "sin", try_sin, 1, NOT_EOF, "sin[x]" },
	{ "cxsin", try_cxsin, 2, NOT_EOF_OR_RETURN, "cxsin[x[,base]]" },
	{ "cxsin2", try_cxsin2, 2, NOT_EOF_OR_RETURN, "cxsin2[x[,base]] (based on cxsincos)" },
	{ "cos", try_cos, 1, NOT_EOF, "cos[x]" },
	{ "cxcos", try_cxcos, 2, NOT_EOF_OR_RETURN, "cxcos[x[,base]]" },
	{ "cxcos2", try_cxcos2, 2, NOT_EOF_OR_RETURN, "cxcos2[x[,base]] (based on cxsincos)" },
};
int try_ascanf_Functions= sizeof(try_ascanf_Function)/sizeof(ascanf_Function);

int nag()
{
	fprintf( stderr, "Hah -- you can't just quit like that (this)!\n" );
	return(0);
}

main( int argc, char **argv)
{
#ifndef VARS_STANDALONE
	Time_Struct timer;
#endif
	long i;
	IEEEfp *ie= (IEEEfp*) Pietje;
	int len;
	long id= 0;
	Variable_t *Argv, *ct;
/* 	extern int c_v_on_NULLptr, c_v_on_NULLptr_default;	*/

/* 	c_v_on_NULLptr= 0;	*/
/* 	c_v_on_NULLptr_default= 0;	*/

#ifndef VARS_STANDALONE
	AbortCode= nag;

	InitProgram();
	Chk_Abort();
	atexit( atexit_log_sysinfo );
/* 	Install_Traps( NULL );	*/
#endif

	randomise();

	Add_SymbolTable_List( try_list, SymbolTable_ListLength(try_list) );

#ifndef VARS_STANDALONE
	Elapsed_Since( &timer);
#endif
	lfree_alien= 0;

	{ float zero = 0.0;
		fl[0]= 1.0f / zero;
		fl[1]= -1.0f / zero;
		fl[2]= 0.0f * fl[0];
		fl[3]= 0.0f * fl[1];
	}

	id= 0;
	if( (Argv= find_named_var( "argv", True, piets, VARS1, &id, stderr)) ){
		fprint_var( stdout, Argv, 2);
		fprintf( stdout, " id=%d (%d)\n", id, Argv->id);
		for( i= 0; i< argc- 1; i++){
			fputs( argv[i], stderr);
			fputc( ' ', stderr);
			fflush( stderr);
			argv[i][strlen(argv[i])]= ',';
		}
		len= strlen( argv[0])+ 1;
		var_set_Variable( Argv, CHAR_VAR, argv[0], len, len);
	}
	{ Variable_t *ppp = find_named_var( "PIETJE!", True, other_piets, VARS2, &id, stderr );
		if( ppp ){
			fprint_var( stderr, ppp, 2 );
		}
	}

	id= 0;
	ct= find_named_var( "ctype", True, piets, VARS1, &id, stderr);
#if defined(_HPUX_SOURCE) || defined(_APOLLO_SOURCE)
	var_set_Variable( ct, UCHAR_VAR, (pointer) __ctype, 256, 256);
#elif _AUX_SOURCE
	var_set_Variable( ct, UCHAR_VAR, (pointer) _ctype, 257, 257);
#endif

	for( i= 0; i< 65; i++)
		char_p[i]= &prompt[i];
	for( i= 0; i< PITEMS; i++){
		short_p[i]= &pietje[i];
		int_p[i]= &int_[i];
		long_p[i]= &long_[i];
		double_p[i]= &Pietje[i];
	}

	Reference_Variable= Sel;
	Reference_Variables= sizeof(Sel)/ sizeof(VariableSelection);
	add_ascanf_functions( try_ascanf_Function, try_ascanf_Functions);
	if( argc> 1 ){
	  FILE *stdo= stdout, *stde= stderr;
		parse_varlist_line( "show internal", &id, False, &stdo, &stde,
			VARS1, (Variable_t*) piets, VARS2, (Variable_t*) other_piets, 0
		);
	}

	sort_variables_list_alpha( VARS1, piets, 0, NULL, VARLIST, varlist, 0, NULL, 0);

//#ifndef VARS_STANDALONE
	Add_VariableSymbol( "stdout", stdout, 1, _file, 0);
	Add_VariableSymbol( "stderr", stderr, 1, _file, 0);
	Add_VariableSymbol( "cx_stderr", cx_stderr, 1, _file, 0);
	Add_VariableSymbol( "vars_errfile", vars_errfile, 1, _file, 0);

	for( i= 0; i< VARS1; i++ ){
		Add_VariableSymbol( piets[i].name, piets[i].var, piets[i].maxcount, ObjectTypeOfVar[piets[i].type], 0);
	}
	for( i= 0; i< VARLIST; i++ ){
		Add_VariableSymbol( varlist[i].name, varlist[i].var, varlist[i].maxcount, ObjectTypeOfVar[varlist[i].type], 0);
	}
	for( i= 0; i< VARS2; i++ ){
		Add_VariableSymbol( other_piets[i].name, other_piets[i].var, other_piets[i].maxcount, ObjectTypeOfVar[other_piets[i].type], 0);
	}
//#endif

	{ long length;
	  Variable_t *list= concat_Variables( &length, VARS1, (Variable_t*) piets, VARS2, (Variable_t*) other_piets, 0);
	  long changes= 0;
		if( list ){
		  FILE *stdo= stdout, *stde= stderr;
			parse_varline( "list", list, length, &changes, False, &stdo, &stde );
			dispose_Concatenated_Variables( list, length, VARS1, (Variable_t*) piets, VARS2, (Variable_t*) other_piets, 0 );
		}
	}

	if( (parfile= fopen( "TryPars", "r" )) ){
	  int n;
		Read_parfile(10);
		n= PITEMS;
		Doubles_par_default( "Pietje", Pietje, n, 0, 1.0);
		n= Ints_par_default( "pietje", int_, n, 0, 2 );
		for( i= 0; i< n; i++ ){
			pietje[i]= (short) int_[i];
		}
		fclose( parfile );
	}

	set_fpNaN(fl[0]);
	set_fpInf(fl[1],1);
	if( fpINF(fl[1]) ){
		set_fpInf(fl[2],-1);
		set_fpINF(fl[3]);
	}
#ifdef USE_SSE4
	{ v2df a, b, c;
	  v2si ia;
	  double aa;
	  v4sf fa, fb;
		a = _MM_SETR_PD( -1.5, 2.5 );
		ia = _mm_cvttpd_pi32(a);
		fa = _MM_SETR_PS( -1.5, 2.5, -345, 678 );
		c = _mm_floor_pd(a);
		b = _mm_abs_pd(a);
		fprintf( cx_stderr, "_mm_floor_pd({%g,%g}) = {%g,%g}; ssfloor(%g)=%g\n",
			   VELEM(double, a, 0), VELEM(double, a, 1),
			   VELEM(double, c, 0), VELEM(double, c, 1),
			   VELEM(double, a, 0), ssfloor(VELEM(double, a, 0))
		);
#ifdef i386
		fprintf( cx_stderr, "Calling _mm_empty()\n" );
		_mm_empty();
#endif
		fprintf( cx_stderr, "_mm_floor_pd({%g,%g}) = {%g,%g}\n",
			   VELEM(double, a, 0), VELEM(double, a, 1),
			   VELEM(double, c, 0), VELEM(double, c, 1)
		);
		fprintf( cx_stderr, "_mm_abs_pd({%g,%g}) = {%g,%g}\n",
			   VELEM(double, a, 0), VELEM(double, a, 1),
			   VELEM(double, b, 0), VELEM(double, b, 1)
		);
		aa = _mm_abs_sd( VELEM(double, a, 0) );
#ifdef i386
		_mm_empty();
#endif
		fprintf( cx_stderr, "_mm_abs_sd(%g) = %g\n",
			   VELEM(double, a, 0), aa
		);
		fprintf( cx_stderr, "_mm_cvttpd_pi32({%g,%g}) = {%d,%d}\n",
			   VELEM(double, a, 0), VELEM(double, a, 1),
			   VELEM(int, ia, 0), VELEM(int, ia, 1)
		);
		fb = _mm_abs_ps(fa);
#ifdef i386
		_mm_empty();
#endif
		fprintf( cx_stderr, "_mm_abs_ps({%g,%g,%g,%g}) = {%g,%g,%g,%g}\n",
			   VELEM(float, fa, 0), VELEM(float, fa, 1), VELEM(float, fa, 2), VELEM(float, fa, 3),
			   VELEM(float, fb, 0), VELEM(float, fb, 1), VELEM(float, fb, 2), VELEM(float, fb, 3)
		);
	}
#endif

	{ Parfile_Context pc= parfile_context, lpc;
		parfile_context= *dup_Parfile_Context( &lpc, vars_file, argv[1] );
		vars_clear_change_labels( piets, VARS1, 0 );
		vars_clear_change_labels( other_piets, VARS2, 0 );
		var_change_label= 'TRY!';
		vars_update_change_label_string( var_change_label, var_change_label_string );
		var_label_change= True;
		edit_variables_list( prompt, VARS1, (Variable_t*) piets, VARS2, (Variable_t*)other_piets, 0);
		pc= parfile_context;
	}
#ifndef VARS_STANDALONE
	Elapsed_Since( &timer);
	printf( "Time spent: %s\n", time_string( timer.Tot_Time) );
	printf( "Sleeping 0.001234s..." ); fflush(stdout);
	printf( "%lfs\n", CXSleep(0.001234) );
#endif
	{ int i;
	  int n1= vars_clear_change_labels( piets, VARS1, 'TRY!' );
	  int n2= vars_clear_change_labels( other_piets, VARS2, 'TRY!' );
		fprintf( stdout, "You accessed %d variables in list1, %d in list2",
			n1, n2
		);
		if( var_change_label!= 'TRY!' ){
			fprintf( stdout, " - before changing the label mask from 'TRY!' to '%s'...!",
				var_change_label_string
			);
		}
		fputc( '\n', stdout );
	}
	{ Sinc dump;
		Sinc_string_behaviour( &dump, NULL, 0, 0, SString_Dynamic );
		for( i= 0; i< VARS1; i++ ){
			Sprint_var( &dump, &piets[i], 1 );
			Sputc( '\n', &dump );
		}
		for( i= 0; i< VARS2; i++ ){
			Sprint_var( &dump, &other_piets[i], 1 );
			Sputc( '\n', &dump );
		}
		if( dump.sinc.string ){
			fprintf( stdout, "Printing all variables in a Sinc string yields a string of %d (%d allocated) chars long\n",
				strlen(dump.sinc.string), dump._cnt
			);
			lib_free( dump.sinc.string );
		}
	}
	exit( 0);
}
