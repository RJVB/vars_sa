/* vars.h: headerfile for vars.c
 * variable and parameter handling routines
 * (C) R. J. Bertin 1990,1991
 * :ts=5
 * :sw=5
 */

#ifndef _VARSEDIT_H
#define _VARSEDIT_H

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef DEFUN
#	ifndef VARS_STANDALONE
#		include <local/defun.h>
#	else
#		include "defun.h"
#	endif
#endif

#ifndef _MXT_H
#	ifndef VARS_STANDALONE
#		include <local/mxt.h>
#	else
#		include "mxt.h"
#	endif
#endif

	/* parameter file reading functions:	*/
DEFUN( AllocLineBuffer, (char ***ptr, int linelength, int numlines), int);
DEFUN( Init_Parbuffer, (), int);
DEFUN( Free_Parbuffer, (), int);
DEFUN( Clear_Parbuffer, (), int);
DEFUN( Read_parfile, ( int x ), int); /* read x lines from parfile */

DEFUN( d2long, (double x), long);
DEFUN( d2int, (double x), int);
DEFUN( d2short, (double x), short);

DEFUN( Double_par_default, ( char *name , double def), double);
#define Double_par(name)	Double_par_default((name),0.0)

DEFUN( Doubles_par_default, ( char *name, double *array, int elems, int strict , double def), int);
#define Doubles_par(name,array,elems,strict)	Doubles_par_default((name),(array),(elems),(strict),0.0)

DEFUN( Int_par_default, ( char *name, int def), int);
#define Int_par(name)	Int_par_default((name),0)

DEFUN( Ints_par_default, ( char *name, int *array, int elems, int strict, int def), int);
#define Ints_par(name,array,elems,strict)	Ints_par_default((name),(array),(elems),(strict),0)

DEFUN( Bool_par_default, ( char *name, int def), int);
#define Bool_par(name)	Bool_par_default((name),False)

DEFUN( Str_par_default, ( char *name, char *target, char *def), void);
#define Str_par(name,target)	Str_par_default((name),(target),NULL)

/* these are the variables that the above routines work on/with
 * You can create different contexts by saving and changing them.
 * Free_Parbuffer de-allocates all memory associated with parbufferkey.
 */

#define MaxParLines 20

typedef struct parfile_context{
	FILE *_Parfile;
	int _Parlinenumber, _Parlinelength, _Parlinesread;
	char **_Parfilecopy, *_Parfiledummy, *_Parfiledummy2;
	struct Remember *_Parbufferkey;
	char *_Parfilename;
	long _Parposition, _Parlineposition;
} Parfile_Context;

#define parfile		parfile_context._Parfile
#define parfilename		parfile_context._Parfilename
#define parlinenumber	parfile_context._Parlinenumber
#define parlinelength	parfile_context._Parlinelength
#define parlinesread	parfile_context._Parlinesread
#define parfilecopy		parfile_context._Parfilecopy
#define parfiledummy	parfile_context._Parfiledummy
#define parfiledummy2	parfile_context._Parfiledummy2
#define parbufferkey	parfile_context._Parbufferkey
#define parposition		parfile_context._Parposition
#define parlineposition		parfile_context._Parlineposition

#define Parfile		_Parfile
#define Parfilename		_Parfilename
#define Parlinenumber	_Parlinenumber
#define Parlinelength	_Parlinelength
#define Parlinesread	_Parlinesread
#define Parfilecopy		_Parfilecopy
#define Parfiledummy	_Parfiledummy
#define Parfiledummy2	_Parfiledummy2
#define Parbufferkey	_Parbufferkey
#define Parposition		_Parposition
#define Parlineposition		_Parlineposition

extern Parfile_Context parfile_context;
  /* the total number of lines read from parfile (excl. includes!) by the last
   \ call to Read_parfile() (which returns the total number of lines copied into the
   \ buffer). Note that this number includes lines that were wrapped and/or commented
   \ out! So even if there were include-files, this number can be larger than the
   \ number of copied lines!
   */
extern int parlinesread_top;

DEFUN( *dup_Parfile_Context, ( Parfile_Context *pc, FILE *fp, char *fname), Parfile_Context);

/* the following read an array of values
 * from s in  array of length *elems. Format: a,b,c,..
 * return EOF if less than *elems elements in s.
 * *elems is updated to reflect the number of items read.
 */

#define ASCANF_FUNCTION_BUF	512
#define ASCANF_MAX_ARGS		128

typedef enum ascanf_Function_type { NOT_EOF , NOT_EOF_OR_RETURN } ascanf_Function_type;

typedef int (*ascanf_Function_method)(double args[ASCANF_MAX_ARGS],double *result);

typedef struct ascanf_Function{
	char *name;
/* 	int (*function)(double args[ASCANF_MAX_ARGS],double *result);	*/
	ascanf_Function_method function;
	int args;
	ascanf_Function_type type;
	char *usage;
	int name_length;
	long hash;
	struct ascanf_Function *cdr;
} ascanf_Function;
extern ascanf_Function *ascanf_FunctionList;
extern int ascanf_Functions;

typedef int (*__ascanf_Function_method)(double args[ASCANF_MAX_ARGS],double *result, ascanf_Function *AF);

DEFUN( show_ascanf_functions, (FILE *fp, char *prefix), int);
DEFUN( add_ascanf_functions, (ascanf_Function *array, int n), int);

DEFUN( cascanf, ( int *elems, char *s, char *array, char *change ), int);
	/* 8 bits variables	*/
DEFUN( dascanf, ( int *elems, char *s, short *array, char *change ), int);
	/* 16 bit variables, can handle hex vars as 0x<val>	*/
DEFUN( lascanf, ( int *elems, char *s, long *array, char *change ), int);
DEFUN( xascanf, ( int *elems, char *s, long *array, char *change ), int);
	/* 32 bits variables, can handle hex vars as 0x<val>	*/
DEFUN( hfascanf, ( int *elems, char *s, float *array, char *change ), int);
	/* floating point variables	*/
DEFUN( fascanf, ( int *elems, char *s, double *array, char *change ), int);
	/* floating point variables	*/

/* variable types:	*/
typedef enum TypeOfVariable {
	CHAR_VAR=	1, UCHAR_VAR, SHORT_VAR, USHORT_VAR, INT_VAR,
	UINT_VAR, LONG_VAR, ULONG_VAR, HEX_VAR, FLOAT_VAR, DOUBLE_VAR,
	COMMAND_VAR, VARIABLE_VAR,
	CHAR_PVAR, SHORT_PVAR, INT_PVAR, LONG_PVAR, FLOAT_PVAR, DOUBLE_PVAR, FUNCTION_PVAR,
	VARTYPES
} TypeOfVariable;

#define MAX_VARTYPE	VARTYPES-1

struct _Variable;
struct Variable_t;

typedef DEFMETHOD(VariableChange_method,(struct Variable_t*, long , long, long),int);

typedef union variabletype{
	void *ptr;
	char *c, **cp;			/* a byte	*/
	unsigned char *uc;
	short *s, **sp;		/* 16 bits	*/
	unsigned short *us;
	int *i, **ip;			/* 16 OR 32 bits!	*/
	unsigned int *ui;
	long *l, **lp;			/* 32 bits; also used for pointers	*/
	unsigned long *ul;		/* also HEX_VARS	*/
	float *f, **fp;
	double *d, **dp;		/* floating point	*/
	VariableChange_method command;
} VariableType;

typedef union variableset{
	void *ptr;
	char c, *cp;			/* a byte	*/
	unsigned char uc;
	short s, *sp;			/* 16 bits	*/
	unsigned short us;
	int i, *ip;			/* 16 OR 32 bits!	*/
	unsigned int ui;
	long l, *lp;			/* 32 bits; also used for pointers	*/
	unsigned long ul;
	float f, *fp;
	double d, *dp;			/* floating point	*/
	VariableChange_method command;
} VariableSet;

/* 920717: count and maxcount changed to unsigned short	*/
typedef struct var_changed_field{
	unsigned short N, N_copy;
	char *flags;
} Var_Changed_Field;

/* 921203: use_changed changed to access:
	access=-1 : read-only Variable
	access=0  : no changed flags
	access=1  : changed flags
 */
 /* 20040903: despite the 920717 comment above, count and maxcount were *signed* shorts.
  \ Whether intentional or not, they're back to being unsigned.
  \ Idem for first, last.
  */
#define VARIABLE_DEFINITION_USER(typeof_varfield)	\
	int id;\
	TypeOfVariable type;\
	unsigned short count, maxcount;\
	typeof_varfield var;\
	char *name;\
	VariableChange_method change_handler;\
	int access;\
	char *description, *alias;\
	int bounds_set;\
	double min_val, max_val

#define VARIABLE_DEFINITION_SYSTEM(typeof_varfield)	\
	Var_Changed_Field *changed;\
	int hit;\
	FILE *Stdout, *Stderr;\
	char *args;\
	int Operator;\
	struct Variable_t *parent;\
	unsigned short first, last;\
	int serial;\
	int old_id;\
	struct Variable_t *old_var;\
	unsigned long change_label

/* make a structure definition Variable_t that contains
 * VariableType var in vars.c, and
 * void *var in user source. The two different types (having the same size!!)
 * can be accessed anywhere as _Variable and Variable respectively.
 */
#ifdef _VARS_SOURCE		/* _VARS_SOURCE	*/
	  /* Structure definition for Variable_t as used internally:	*/
	typedef struct Variable_t{
		VARIABLE_DEFINITION_USER(VariableType);
		VARIABLE_DEFINITION_SYSTEM(VariableType);
	} Variable_t;
	  /* The same definition, global	*/
	typedef struct _Variable{
		VARIABLE_DEFINITION_USER(VariableType);
		VARIABLE_DEFINITION_SYSTEM(VariableType);
	} _Variable;

	  /* The user definable version:	*/
	typedef struct Variable{
		VARIABLE_DEFINITION_USER(pointer);
		VARIABLE_DEFINITION_SYSTEM(pointer);
	} Variable;
#else				/* !_VARS_SOURCE	*/
	  /* The global internal version:	*/
	typedef struct _Variable{
		VARIABLE_DEFINITION_USER(VariableType);
		VARIABLE_DEFINITION_SYSTEM(VariableType);
	} _Variable;

	  /* Version for user-initialisation:	*/
	typedef struct Variable{
		VARIABLE_DEFINITION_USER(pointer);
		VARIABLE_DEFINITION_SYSTEM(pointer);
	} Variable;
	  /* Version of Variable_t for the user:	*/
	typedef struct Variable_t{
		VARIABLE_DEFINITION_USER(pointer);
		VARIABLE_DEFINITION_SYSTEM(pointer);
	} Variable_t;
#endif				/* _VARS_SOURCE	*/

/* Some easier definitions:
 * these types have their ->var field with the correct
 * type. Use'em if you are sure of the type of a Variable.
 */
typedef struct char_Variable{
	VARIABLE_DEFINITION_USER(char*);
	VARIABLE_DEFINITION_SYSTEM(char*);
} char_Variable;

typedef struct uchar_Variable{
	VARIABLE_DEFINITION_USER(unsigned char*);
	VARIABLE_DEFINITION_SYSTEM(unsigned char*);
} uchar_Variable;

typedef struct short_Variable{
	VARIABLE_DEFINITION_USER(short*);
	VARIABLE_DEFINITION_SYSTEM(short*);
} short_Variable;

typedef struct ushort_Variable{
	VARIABLE_DEFINITION_USER(unsigned short*);
	VARIABLE_DEFINITION_SYSTEM(unsigned short*);
} ushort_Variable;

typedef struct int_Variable{
	VARIABLE_DEFINITION_USER(int*);
	VARIABLE_DEFINITION_SYSTEM(int*);
} int_Variable;

typedef struct uint_Variable{
	VARIABLE_DEFINITION_USER(unsigned int*);
	VARIABLE_DEFINITION_SYSTEM(unsigned int*);
} uint_Variable;

typedef struct long_Variable{
	VARIABLE_DEFINITION_USER(long*);
	VARIABLE_DEFINITION_SYSTEM(long*);
} long_Variable;

typedef struct ulong_Variable{
	VARIABLE_DEFINITION_USER(unsigned long*);
	VARIABLE_DEFINITION_SYSTEM(unsigned long*);
} ulong_Variable;

typedef struct double_Variable{
	VARIABLE_DEFINITION_USER(double*);
	VARIABLE_DEFINITION_SYSTEM(double*);
} double_Variable;

typedef struct charp_Variable{
	VARIABLE_DEFINITION_USER(char**);
	VARIABLE_DEFINITION_SYSTEM(char**);
} charp_Variable;

typedef struct shortp_Variable{
	VARIABLE_DEFINITION_USER(short**);
	VARIABLE_DEFINITION_SYSTEM(short**);
} shortp_Variable;

typedef struct intp_Variable{
	VARIABLE_DEFINITION_USER(int**);
	VARIABLE_DEFINITION_SYSTEM(int**);
} intp_Variable;

typedef struct longp_Variable{
	VARIABLE_DEFINITION_USER(long**);
	VARIABLE_DEFINITION_SYSTEM(long**);
} longp_Variable;

typedef struct doublep_Variable{
	VARIABLE_DEFINITION_USER(double**);
	VARIABLE_DEFINITION_SYSTEM(double**);
} doublep_Variable;

#define VARIABLECHANGE_METHOD(name)	DEFUN(name,(Variable_t*, long , long, long),int)

/* with this you can make a selection-list of items of
 * several Variable arrays.
 */
typedef struct variableselection{
	Variable_t *Vars;
	int id;
	int Index, subIndex;
} VariableSelection; 

/* And this is to assign an extra description. The description-field
 \ of the Variable_t itself was conceived to give some explanation of
 \ what it does. This one is conceived for the graphical interface
 \ in graftool::grafvarsV.
 */
typedef struct DescribedVariable{
	Variable_t *var;
	char *description;
	double minval, maxval;
	char *default_args;
} DescribedVariable;

typedef struct DescribedVariable2{
	Variable_t *var;
	int id;
	char *description;
	double minval, maxval;
	char *default_args;
} DescribedVariable2;

#define VARIABLE(x)		(((pointer)&(x)))

#define OPERATORS		6

/* values in the changed field of a Value:
 * Note that this field is installed by check_vars!
 */
#define VAR_CHANGED	'N'
#define VAR_REASSIGN	'R'
#define VAR_UNCHANGED	'o'

#define RET_SHORT	int

extern FILE *vars_file;
#ifndef VARS_STANDALONE
extern FILE *vars_errfile;
#endif
extern int vars_compress_controlchars;

extern char Variable_Operators[OPERATORS+1];
extern int sizeofVariable[MAX_VARTYPE];
extern char *VarType[MAX_VARTYPE+1];
extern ObjectTypes ObjectTypeOfVar[MAX_VARTYPE+1];

DEFUN( check_vars, ( Variable_t *vars, long n, FILE *errfp), RET_SHORT );

typedef enum SincType { SINC_STRING=0xABCDEF09, SINC_FILE=0x4AF3BD8C } SincType;
/* #define SINC_STRING	0xABCDEF09	*/
/* #define SINC_FILE	0x4AF3BD8C	*/

/* NB: dynamical allocation/expansion of Sinc strings uses lib_calloc() and needs lib_free() afterwards!	*/ 
typedef enum SincString{ SString_Fixed, SString_Dynamic, SString_Global } SincString;
extern int SincString_Behaviour;

typedef struct string{
	union file_or_string{
		char *string;
		FILE *file;
	} sinc;
	SincType type;
	long _cnt, _base, _tlen, alloc_len;
	SincString behaviour;
} Sinc;

DEFUN( var_changed_flag, (Variable_t *var, int Index), int);
DEFUN( var_accessed, (Variable_t *var, int Index), int);
DEFUN( *var_ChangedString, (Variable_t *var), char );

DEFUN( var_set_Variable, (Variable_t *var, TypeOfVariable type, pointer val,\
	unsigned int count, unsigned int maxcount), int);
DEFUN( var_Value, (Variable_t *var, int Index), double);
DEFUN( var_Mean, (Variable_t *var, Variable_t *mean, SimpleStats *ss), Variable_t *);
/* printing routines:	*/
/* Sprint_var: the basic routine: prints to a Sinc.
 * Is called by fprint_var, sprint_var and print_var.
 */
DEFUN( Sprint_var, ( Sinc *fp, Variable_t *var, int print_addr), RET_SHORT );
DEFUN( fprint_var, ( FILE *fp, Variable_t *var, int print_addr), RET_SHORT );
DEFUN( sprint_var, ( char *fp, Variable_t *var, int print_addr, long stringlength), RET_SHORT );

DEFUN( Sprint_varMean, ( Sinc *fp, Variable_t *var), RET_SHORT );
DEFUN( fprint_varMean, ( FILE *fp, Variable_t *var), RET_SHORT );
DEFUN( sprint_varMean, ( char *fp, Variable_t *var, long stringlength), RET_SHORT );

/* print_var appends a NEWLINE to the file	*/
DEFUN( print_var, ( FILE *fp, Variable_t *var, int print_addr), RET_SHORT );
DEFUN( print_varMean, ( FILE *fp, Variable_t *var), RET_SHORT );

/* lowlevel printing routines	*/
DEFUN( print_charvar, ( Sinc *fp, Variable_t *var), RET_SHORT );
DEFUN( print_shortvar, ( Sinc *fp, Variable_t *var), RET_SHORT );
DEFUN( print_intvar, ( Sinc *fp, Variable_t *var), RET_SHORT );
DEFUN( print_longvar, ( Sinc *fp, Variable_t *var), RET_SHORT );
DEFUN( print_hexvar, ( Sinc *fp, Variable_t *var), RET_SHORT );
DEFUN( print_doublevar, ( Sinc *fp, Variable_t *var), RET_SHORT );

/* routines that print arrays of length <len> and Variable type <type> to
 \ string or FILE
 */
DEFUN( Sprint_array, ( Sinc *buf, TypeOfVariable type, int len, pointer data), RET_SHORT);
DEFUN( fprint_array, ( FILE *buf, TypeOfVariable type, int len, pointer data), RET_SHORT);
DEFUN( *sprint_array, ( char *buf, int buflen, TypeOfVariable type, int len, pointer data), char);

DEFUN( cook_string, (char *dest, char *source, int *count, char *do_comments ), long);

DEFUN( find_named_var, ( char *buf, int exact, Variable_t *vars, long n, long *I, FILE *errfp), Variable_t*);
DEFUN( find_var, ( pointer ptr, Variable_t *vars, long n, long *I, FILE *errfp), Variable_t*);
DEFUN( find_named_VariableSelection, ( char *name, VariableSelection *XVariables, int MaxXVariables,\
	int *I, int *J, VariableSelection **vs_return), Variable_t*);

extern VariableSelection *Reference_Variable;
extern int Reference_Variables;
extern int var_label_change;
extern unsigned long var_change_label;
extern char var_change_label_string[5];

DEFUN( var_update_change_label, (), unsigned long );
DEFUN( var_update_change_label_string, (unsigned long change_label, char label_string[5]), char*);
  /* clear the labels for the <n> length variable array <v>. If mask==0, all labels are
   \ cleared, otherwise only those that are set to <mask>. The function returns the
   \ number of cleared variables.
   */
DEFUN( vars_clear_change_labels, ( Variable_t *v, long n, unsigned long mask ), int );
DEFUN( vars_update_change_label, (), unsigned long );
DEFUN( vars_update_change_label_string, (), char* );

extern int line_invocation;

#ifdef _AUX_SOURCE
#	define CX_VA_DCL	int __builtin_va_alist,
#endif
#ifdef _APOLLO_SOURCE
#	define CX_VA_DCL	int va_alist,
#endif
#if defined(_IRIX_SOURCE) || defined(linux) || defined(_AIX)
#	ifdef __GNUC___
#		define CX_VA_DCL	int __builtin_va_alist,
#	else
#		define CX_VA_DCL	long va_alist,
#endif
#endif
#ifndef CX_VA_DCL
#	ifdef __GNUC___
		  /* educated guess... */
#		define CX_VA_DCL	int __builtin_va_alist,
#	elif WIN32
#		define CX_VA_DCL	void *va_first,
#	else
#		define CX_VA_DCL	/* */
#	endif
#endif

DEFUN( parse_varline, ( char *buf, Variable_t *vars, long n, long *changes, int cook_it, FILE **outfp, FILE **errf), long);
/* A version to which one can pass a static string as an argument: */
DEFUN( parse_varline_safe, ( char *buf, Variable_t *vars, long n, long *changes, int cook_it, FILE **outfp, FILE **errf), long);
DEFUN( edit_vars_arglist, ( char *prompt), RET_SHORT );

#ifndef VARARGS_OLD
	DEFUN( sort_variables_list_alpha, ( long n, Variable_t *vars, ...), int);
	DEFUN( set_vars_arglist, ( long n, Variable_t *vars, ...), int );
	DEFUN( concat_Variables, ( long *length, long n, ...), Variable_t* );
	DEFUN( concat_Variables_with_description, ( long *length, Variable_t *var, ...), DescribedVariable* );
	DEFUN( concat_Variables_with_description2, ( long *length, DescribedVariable2 *var, ...), DescribedVariable* );
	DEFUN( parse_varlist_line, ( char *buf, long *changes, int cook_it, FILE **outfp, FILE **errf, long n, Variable_t *vars, ...), long);
	DEFUN( edit_variables_list, ( char *prompt, long n, ...), RET_SHORT );
#else
	DEFUN( sort_variables_list_alpha, (), int);
	DEFUN( set_vars_arglist, (), int );
	/* concatenate a number of Variable-arrays. The first argument returns the number of
	 \ items in the resulting array. The other arguments are pairs
	 \ <count>, <pointer to Variable(s)>
	 \ Last argument must be NULL.
	 \ This routine makes a new array that contains copies of the original Variables! It
	 \ can thus be used to make an independant copy of a list:
	 \ concat_Variables( &length, n, list, 0);
	 \ where (afterwards, upon success) length=n.
	 */
	DEFUN( concat_Variables, ( long *length, CX_VA_DCL ...), Variable_t* );

	/* concatenate a number of Variables, with extra description. The first argument returns the number of
	 \ items in the resulting array. The other arguments are quartets
	 \ <pointer to Variable>, <pointer to string>, minimal value, maximal value
	 \ Last argument must be NULL.
	 \ This routine makes an array of structures combining a pointer to the original Variable (no copy!!),
	 \ with a pointer to the corresponding description string.
	 */
	DEFUN( concat_Variables_with_description, ( long *length, CX_VA_DCL ...), DescribedVariable* );

	/* also concatenate a number of Variables, with extra description. The first argument returns the number of
	 \ items in the resulting array. The other arguments are pairs
	 \ <pointer to VariableDescription2 array>, <lenght of array>
	 \ Last argument must be NULL.
	 \ This routine makes an array of structures combining a pointer to the original Variable (no copy!!),
	 \ with a pointer to the corresponding description string. If the description string is zero, either the
	 \ Variable's internal description is taken, or else its name.
	 */
	DEFUN( concat_Variables_with_description2, ( long *length, CX_VA_DCL ...), DescribedVariable* );
	DEFUN( parse_varlist_line, ( char *buf, long *changes, int cook_it, FILE **outfp, FILE **errf, CX_VA_DCL ...), long);
	DEFUN( edit_variables_list, ( char *prompt, CX_VA_DCL ...), RET_SHORT );
#endif

DEFUN( vars_include_file, (char *filename), int );
DEFUN( vars_parse_line, (char *buffer, long *changes), long );
DEFUN( vars_init_readline, (), int );

 /* an "interim" command	*/
extern char *edit_variables_list_interim;
#define edit_variables_interim	edit_variables_list_interim

#define edit_variables(vars,n,prompt)	edit_variables_list(prompt,n,vars,0)

DEFUN( Sinc_string, ( Sinc *sinc, char *string, long _cnt, long base), Sinc* );
DEFUN( Sinc_string_behaviour, ( Sinc *sinc, char *string, long _cnt, long base, SincString behaviour), Sinc* );
extern int SincString_Behaviour;
DEFUN( SincAllowExpansion, (Sinc *sinc), int );
DEFUN( Sinc_file, ( Sinc *sinc, FILE *file, long cnt, long base), Sinc* );
DEFUN( Sinc_base, ( Sinc *sinc, long base), Sinc* );
DEFUN( Sputs, ( char *text, Sinc *sinc), int );
DEFUN( SSputs, ( char *text, Sinc *sinc), Sinc* );
DEFUN( Sputc, ( int a_char, Sinc *sinc), int );
DEFUN( SSputc, ( int a_char, Sinc *sinc), Sinc* );
DEFUN( Sflush, (Sinc *sinc), int );

#define Seof(Sinc)	(Sinc->_cnt>0 && Sinc->_base>= Sinc->_cnt)
/* 20001116: the Serror() macro had CINC... instead of SINC... ; was that a desactivation?	*/
#define Serror(Sinc)	(!Sinc || !Sinc->sinc.string || (Sinc->type!=SINC_STRING && Sinc->type!=SINC_FILE))
#define _Sflush(Sinc)	{if(Sinc->type==SINC_FILE)Flush_File(Sinc->sinc.file);}


#ifndef VARS_STANDALONE
#	ifndef _CX_H
#		include "cx.h"
#	endif
#endif

extern VariableChange_method SymbolValue_change_handler;

DEFUN( *Set_SymbolValue,	(pointer address, int type, unsigned long n_items, char *items),	Variable_t ); 
		/* set the value field for the SymbolTable entry for <address> to <value>.
		 * returns <address> on success, 0 on failure
		 */

DEFUN( *Add_VariableSymbolValue, \
	(char *name, pointer variablepointer, int size, ObjectTypes obtype, int log_it, int Valtype, unsigned long Valitems, char *val),\
	struct SymbolTable );

DEFUN( *Get_SymbolValue,	(pointer address, int tag),	Variable_t ); 
		/* Get the value Variable_t associated with <address>, or NULL
		 * if <address> is not in the address table, or doesn't have a
		 * value
		 * See Get_Symbol for the interpretation of <address>
		 */

#ifdef __cplusplus
	}
#endif
#endif
