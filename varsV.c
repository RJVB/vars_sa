/* vars.c: package for handling
 * variables and parameters in a relatively simple manner
 * (C) R. J. Bertin 1990,1991
 * :ts=5
 * :sw=5
 */

#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#include "varsintr.h"
#define VARS_SOURCE

int VarsEdit_Length= 1024, VEL_changed= 0;
#define VARSEDIT_LENGTH	VarsEdit_Length
#include "ALLOCA.h"

#ifndef VARS_STANDALONE
#	include "cx.h"
#	include "mxt.h"
#else
#	include "vars-standalone.h"
#endif

#include "varsedit.h"

/* #include <values.h>	*/
#include <signal.h>

IDENTIFY( "Variable_t handling");

/* int *varsV_D_errno= &errno;	*/

/* Variable functions: based on an array of Variable_t structures,
 \ allow handling of the variables described by this array.
 \ The Variable_t structure contains
 \	an id number
 \	a type indicator (see varsedit.h)
 \	the maxcount of elements in the described variable
 \	the number of actual elements in this variable (typically initialised
 \		to maxcount). This value reflects the number of values most
 \		recently changed.
 \	a union containing pointers of the various types that point
 \		to the described variable.
 \	a pointer to the namestring associated with the variable.
 \	a functionpointer specifying a function that is called when
 \		changes are made to the variable. Arguments passed are:
 \			vars	: the array of _Variables
 \			n	: the number of elements of vars
 \			i	: the index of the _Variable in question
 \			k	: the number of changes made to vars[i] (!= 0)
 \	These structures can be initialised using an identical (Variable)
 \	structure, that has a (void) pointer instead of the union.
 \	Define CURLY_RESTRICTED to restrict the '{' '}' delimiters to
 \	COMMAND_VAR's, CHAR_VAR's and VARIABLE_VAR's only. Backw. Comp.
 */

char Variable_Operators[OPERATORS+1]= "^*+/-:|";
int sizeofVariable[MAX_VARTYPE]= { -1, sizeof(char), sizeof(unsigned char),
	sizeof(short), sizeof(unsigned short),
	sizeof(int), sizeof(unsigned int),
	sizeof(long), sizeof(unsigned long), sizeof(unsigned long),
	sizeof(float), sizeof(double), sizeof(Variable_t*), sizeof(char*), sizeof(short*),
	sizeof(int*), sizeof(long*), sizeof(float*), sizeof(double*)
};

char *VarType[MAX_VARTYPE+1]= {
	 "no type"
	,"char"
	,"uchar"
	,"short"
	,"ushort"
	,"int"
	,"uint"
	,"long"
	,"ulong"
	,"hex"
	,"float"
	,"double"
	,"command"
	,"Variable"
	,"char ptr"
	,"short ptr"
	,"int ptr"
	,"long ptr"
	,"float ptr"
	,"double ptr"
};

ObjectTypes ObjectTypeOfVar[MAX_VARTYPE+1]= {
	_nothing, _string, _char, _short, _short, _int, _int, _long, _long,
	_long, _float, _double, _function, _variable, _charP, _shortP, _intP, _longP, _floatP, _doubleP, _functionP
};

typedef struct Var_list{
	long n;
	Variable_t *vars;
	int user;
} Var_list;
#define MAX_VAR_LISTS	32

char VarsSeparator[2]= "|";

static long vars_arg_items= 0;
static Var_list vars_arglist[MAX_VAR_LISTS];

typedef struct Watch_Variable{
	TypeOfVariable type;
	Variable_t *var;
	char *buf;
	struct Watch_Variable *tail;
} Watch_Variable;

static Watch_Variable *Watch_Variables;
static int Watch= True, *WWatch= NULL;

static FUNCTION( parse_varline_with_list, \
	( char *buffer, long items, Var_list *arglist, long *changes, int cook_it, FILE **outfp, FILE **errfp, char *caller), \
	long );

/* check_vars() modulating switches	*/
int c_v_on_NULLptr= 1, c_v_on_PARENT_notify= 1, c_v_PARENT_check= 1, c_v_correct_ID= 0, c_v_on_NULLelement= 0;
int c_v_on_NULLptr_default= 1, c_v_on_PARENT_notify_default= 1, c_v_PARENT_check_default= 1, c_v_correct_ID_default= 0, c_v_on_NULLelement_default= 0;
char *check_vars_caller= NULL;

int execute_command_question= 0;

DEFUN( find_balanced, (char *string, const char match, const char left_brace, const char right_brace, int *esc), char*);

/* check the changed field of a Variable. Presently, this consists
 * of a Pascal-like string with its length stored twice as a ushort at offset 0,
 * followed by the actual changed-flags in a C-string with its address at offset 2*sizeof(ushort).
 * The length should be maxcount+1 (to store the NULL byte).
 */
static int _check_changed_field( FILE *fp, Variable_t *vars)
{  int N;
   char *cvc= (check_vars_caller)? check_vars_caller : "";
#ifdef USE_ROOTPOINTER_CHANGED
	if( !(N= RootPointer_CheckLength( vars->changed, -1, "check_vars()" )) || N< vars->maxcount+1 ){
		fprintf( fp,
			"%s::_check_changed_field(): Disposing changed field for Variable \"%s\" (size %d)\n",
			cvc, vars->name, N
		);
		Dispose_Disposable( vars->changed);
		vars->changed= NULL;
		return( 1);
	}
#else
  Var_Changed_Field *len= vars->changed;
	if( len && ((N= len->N)< vars->maxcount+ 1 || len->N!= len->N_copy) ){
		  /* dispose a too short changed field only if Disposable;
		   * else enter it, (hopefully) postponing bad-pointer-freeing
		   * related crashes to a later stage.
		   */
		if( Find_Disposable( vars->changed) ){
			Dispose_Disposable( vars->changed);
			fprintf( fp,
				"%s::_check_changed_field(): Disposed changed field for Variable \"%s\" (size %d)\n",
				cvc, vars->name, N
			);
		}
/* 		else
			Enter_Disposable( vars->changed, N+2*sizeof(unsigned short) );	*/
		vars->changed= NULL;
		return( 1);
	}
#endif
	return( 0);
}

#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
DEFUN( *_Get_Symbol, ( pointer x, int tag, ObjectTypes obtype, int remove_emptySyn), SymbolTable);
extern char name_of_symbol_null[];
#endif

double *parse_operator_buffer= NULL;
#define PARSE_OPERATOR_BUFFER "parse_operator() buffer"

char *vars_Find_Symbol( x)
pointer x;
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
{	SymbolTable *t;

	if( x== NULL){
		return( name_of_symbol_null);
	}
	else if( x== parse_operator_buffer ){
		return( PARSE_OPERATOR_BUFFER );
	}
	if( ( t= _Get_Symbol( x,  0, _nothing, 0))== NULL){
		return( "" );
	}
	else
		return( t->symbol);
}
#else
{
	return("<no symbol table>");
}
#endif

char *vars_address_symbol( x)
pointer x;
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
{ char *ret= vars_Find_Symbol(x);

	if( *ret== '\0' ){
	  static char buf[32];
		sprintf( buf, "[0x%lx]", x );
		return( buf );
	}
	else
		return( ret );
}
#else
{
	return("<no symbol table>");
}
#endif

check_vars_low_errormsg( FILE *fp, char *cvc, int line, Variable_t *Vars, Variable_t *vars)
{
	fprintf( fp, "%s::check_vars(%d): Variable %s[%d] \"%s\" (%s->%s)",
		cvc, line, vars_address_symbol(Vars), vars->id,
		vars->name? vars->name : "",
		vars_Find_Symbol(vars), vars_Find_Symbol(vars->var.ptr)
	);
	if( vars->description ){
		fprintf( fp, " \"%s\"", vars->description );
	}
	fflush( fp );
}

#ifdef _MSC_VER
	extern Variable vars_internals[];
#else
	static Variable vars_internals[];
#endif

/* check the array of Variable_ts for inconstistencies;
 * notify errors on stderr, return 0 if all OK.
 */
RET_SHORT check_vars( vars, n, fp)
Variable_t *vars;
long n;
FILE *fp;
{	long i, k, iderr= 0;
	short j= 0;
     static char VarVarActive= 0, called= 0;
	char *cvc= (check_vars_caller)? check_vars_caller : "";
	pointer *ptr;
	Variable_t *varlist= vars;

	if( !fp)
		fp= vars_errfile;
	if( !called){
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
	  extern int Program_Inited;
	  int cvID= c_v_correct_ID;
	  extern int VARS_INTERNALS;
		called= 1;
		c_v_correct_ID= 1;
		check_vars_reset( vars_internals, VARS_INTERNALS, fp);
		c_v_correct_ID= cvID;
		register_S_fnc();
		register_V_fnc();
#ifndef VARS_STANDALONE
		if( !Program_Inited ){
			fprintf( vars_errfile, "%s::check_vars(): you haven't called InitProgram() from main()!\n",
				cvc
			);
		}
#endif
#endif
	}
	if( !vars){
		fprintf( fp, "%s::check_vars(%d): NULL pointer passed.\n", cvc, __LINE__);
		errno= EINVAL;
		Flush_File( fp);
		return(1);
	}
	if( n== -1 ){
	  /* just check one Variable.	*/
		if( vars->id>= 0 ){
			n= 1+ vars->id;
		}
		else{
			n= 1;
		}
	}
	  /* 20001023: vars->id can be <0. This is of course either pathological, or because of a lazy
	   \ user who initialises all IDs with <0, and then lets check_vars() do the job. Therefore,
	   \ i should not be allowed to become <0...
	   */
	for( k= 0L, i= MAX( vars->id,0);
		k< n && (!iderr && i< n);
		k++, i++, vars++
	){
		if( vars->id< 0 || i!= vars->id){
			  /* 20041115: */
			if( vars->id< 0 ){
				if( vars->name ){
					vars->name= strdup(vars->name);
				}
				if( vars->description ){
					vars->description= strdup(vars->description);
				}
			}
			if( !c_v_correct_ID){
				check_vars_low_errormsg( fp, cvc, __LINE__, varlist, vars);
				fprintf( fp, " has illegal id %d (should be %d)\n", vars->id, i);
				errno= EVARILLID;
				j++;
				iderr++;
			}
			else{
				if( vars->id>= 0 ){
					check_vars_low_errormsg( fp, cvc, __LINE__, varlist, vars);
					fprintf( fp, " illegal id %d corrected to %d\n", vars->id, i);
				}
				vars->id= i;
			}
		}
		if( vars->first > vars->last && vars->last> 0 ){
		  short c= vars->last;
			vars->last= vars->first;
			vars->first= c;
		}
		if( vars->first>= vars->maxcount && vars->maxcount== 0 ){
		  /* this is always silently corrected	*/
			vars->first= vars->maxcount;
		}
		else if( vars->type!= COMMAND_VAR && (vars->first< 0 || vars->first>= vars->maxcount) ){
			check_vars_low_errormsg( fp, cvc, __LINE__, varlist, vars);
			if( !c_v_correct_ID ){
				fprintf( fp, " has illegal range start %d\n", (int) vars->first);
				errno= EVARILLID;
				j++;
				iderr++;
			}
			else{
			  int first= vars->first;
				vars->first= (vars->first>= vars->maxcount)? vars->maxcount-1 : 0;
				fprintf( fp, " illegal range-start %d corrected to %d\n",
					first, vars->first
				);
			}
		}
		if( vars->last== 0 ){
		  /* vars->last can become -1	*/
			vars->last= vars->maxcount- 1;
		}
		else if( vars->type!= COMMAND_VAR && (vars->last>= vars->maxcount || vars->last< -1) ){
			check_vars_low_errormsg( fp, cvc, __LINE__, varlist, vars);
			if( !c_v_correct_ID){
				fprintf( fp, " has illegal range end %d\n", (int) vars->last);
				errno= EVARILLID;
				j++;
				iderr++;
			}
			else{
			  int last= vars->last;
				vars->last= (vars->last>= vars->maxcount)? vars->maxcount-1 : 0;
				fprintf( fp, " illegal range-end %d corrected to %d\n",
					last, (int) vars->last
				);
				vars->id= i;
			}
		}
		if( !vars->name){
			check_vars_low_errormsg( fp, cvc, __LINE__, varlist, vars);
			fprintf( fp, " has NULL name\n");
			errno= EVARNONAME;
			j++;
		}
		else{
			if( vars->type<= 0 || vars->type> MAX_VARTYPE){
				fprintf( fp, "%s::check_vars(%d): Variable \"%s\" has illegal type %d\n",
					cvc, __LINE__, vars->name, vars->type
				);
				errno= EVARILLTYPE;
				j++;
			}
			if( vars->type== COMMAND_VAR){
				if( !vars->var.command )
					vars->var.command= vars->change_handler;
				if( !vars->var.command ){
					fprintf( fp,
						"%s::check_vars(%d): Command Variable \"%s\" has no command method nor a Change_Handler\n",
						cvc, __LINE__, vars->name
					);
					errno= EVARNOCOMMAND;
					j++;
				}
			}
			else switch( vars->type ){
				case VARIABLE_VAR:{
				  int cv;
				  long e;
				  Variable_t *syn= vars->var.ptr;
					if( vars->var.ptr== vars){
						vars->maxcount= 0L;
						fprintf( fp, "%s::check_vars(%d): Variable Variable \"%s\" points to itself (maxcount set to 0)\n",
							cvc, __LINE__, vars->name
						);
					}
					else if( !VarVarActive){
						VarVarActive= 1;
						if( !(cv= check_vars( vars->var.ptr, vars->maxcount, fp)) ){
							for( e= 0; e< vars->maxcount; e++, syn++){
								if( syn->type== VARIABLE_VAR && syn->var.ptr== vars){
									syn->maxcount= 0L;
									fprintf( fp,
										"%s::check_vars(%d): Element %s points to parent Variable \"%s\" (maxcount set to 0)\n",
										cvc, __LINE__, syn->name, vars->name
									);
								}
								else{
									syn->parent= vars;
								}
							}
						}
						else
							j+= cv;
						VarVarActive= 0;
					}
/* 
					else
						fprintf( fp, "%s::check_vars(%d): warning: more than one level of indirection in %s -> %s\n",
							cvc, __LINE__, vars->name, syn->name
						);
 */
					break;
				}
				case CHAR_PVAR:
					ptr= (pointer*) vars->var.cp;
				case SHORT_PVAR:
					ptr= (pointer*) vars->var.sp;
				case INT_PVAR:
					ptr= (pointer*) vars->var.ip;
				case LONG_PVAR:
					ptr= (pointer*) vars->var.lp;
				case FLOAT_PVAR:
					ptr= (pointer*) vars->var.fp;
				case DOUBLE_PVAR:{
				  int i;
				  int hit= 0;
					ptr= (pointer*) vars->var.dp;
					for( i= 0; i< vars->maxcount; i++ ){
						if( ! ptr[i]  && c_v_on_NULLelement ){
							if( !hit ){
								fprintf( fp, "%s::check_vars(%d): warning: NULL %s(s) in Variable \"%s\":",
									cvc, __LINE__,
									VarType[vars->type],
									vars->name
								);
								hit= 1;
							}
							fprintf( fp, "[%d]", i);
							Flush_File(fp);
						}
					}
					if( hit ){
						fputc( '\n', fp);
						Flush_File( fp);
					}
					break;
				}
				default:{
					  /* 990923: var.ptr==NULL only error when maxcount> 0	*/
					if( vars->var.ptr== NULL && c_v_on_NULLptr && vars->maxcount ){
						fprintf( fp, "%s::check_vars(%d): Variable \"%s\" points to NULL\n",
							cvc, __LINE__, vars->name
						);
						errno= EVARNULL;
						j++;
					}
					if( vars->count> vars->maxcount){
						fprintf( fp,
							"%s::check_vars(%d): Variable \"%s\" count > maxcount (corrected)\n",
							cvc, __LINE__, vars->name
						);
						vars->count= vars->maxcount;
					}
					if( vars->access>= 1){
						if( vars->changed){
						  /* already there; check it and dispose if faulty	*/
							_check_changed_field( fp, vars);
						}
						if( vars->type!= COMMAND_VAR && !vars->changed){
						  /* should try to construct a new changed field	*/
							if( !(vars->changed=
									(Var_Changed_Field*) lcalloc( 1, sizeof(Var_Changed_Field)+ vars->maxcount+1)
								)
							){
								fprintf( fp,
									"%s::check_vars(%d): can't attach changed field to Variable \"%s\" (%s)\n",
										cvc, __LINE__, vars->name, serror()
								);
							}
							else{	/*  if( !_check_changed_field( fp, vars))	*/
							  unsigned short I;
							  Var_Changed_Field *vcf= vars->changed;
							  unsigned long c= (unsigned long) &vcf->flags;
								Enter_Disposable( vars->changed, calloc_size);
								Init_Parbuffer();
								sprintf( parfiledummy, "\"%s\"->changed", vars->name );
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
								Add_VariableSymbol( parfiledummy, vars->changed, 1, _char, 0);
#endif
								  /* initialise it correctly	*/
								vcf->N= (unsigned short)vars->maxcount+1;
								vcf->N_copy= (unsigned short)vars->maxcount+1;
								  /* flags will point to the extra space immediately after
								   * it.
								   */
								vcf->flags= (char*)( c+ sizeof( char*) );
								for( I= 0; I< vars->maxcount; I++){
									RESET_CHANGED_FIELD( vars, I);
								}
							}
						}
						SET_CHANGED_FIELD(vars, vars->maxcount, '\0');
					}
					if( vars->access>= 1 && !vars->changed ){
					  /* allocation failed: switch of access	*/
						vars->access= 0;
					}
					break;
				}
			}
		}
		if( c_v_PARENT_check ){
			if( vars->parent ){
				if( &(((Variable_t*)vars->parent->var.ptr)[vars->id])!= vars ){
					if( c_v_on_PARENT_notify ){
						fprintf( fp, "%s::check_vars(%d): Variable \"%s\"->parent == \"%s\", but id doesn't match - corrected\n",
							cvc, __LINE__, vars->name, vars->parent->name
						);
					}
					vars->parent= NULL;
				}
			}
		}
	}
	return( j);
}

RET_SHORT check_vars_reset( Variable_t *vars, long n, FILE *fp)
{  short ret;
   char *cvc= check_vars_caller;
	check_vars_caller= "check_vars_reset";
	ret= check_vars( vars, n, fp);
   /* reset the check_vars() switches	*/
	c_v_on_NULLptr= c_v_on_NULLptr_default;
	c_v_on_PARENT_notify= c_v_on_PARENT_notify_default;
	c_v_PARENT_check= c_v_PARENT_check_default;
	c_v_correct_ID= c_v_correct_ID_default;
	check_vars_caller= cvc;
	return(ret);
}

int alpha_var_sort( a, b)
Variable_t *a, *b;
{
	return( strcasecmp( a->name, b->name) );
}

int vars_sort_follow_tree= 1;

#ifndef VARARGS_OLD
int sort_variables_list_alpha( long n, Variable_t *vars, ... )
#else
int sort_variables_list_alpha( va_alist )
va_dcl
#endif
{  va_list ap;
#ifdef VARARGS_OLD
   long n;
   Variable_t *vars;
#endif
   long i, vsn, j;
   VariableSelection *vs, *_vs= NULL;
   char cvc[256], _cvc_ = check_vars_caller;
   int N= 0;

#ifndef VARARGS_OLD
	va_start(ap, vars);
#else
	va_start(ap);
	n= va_arg(ap, long);
#endif
	while( !n ){
	  int sorted= 0;
	  int OK;
#ifdef VARARGS_OLD
		vars= va_arg( ap, Variable_t*);
#endif
		vsn= va_arg( ap, long );
		vs= va_arg( ap, VariableSelection* );
		sprintf( cvc, "sort_variables_list_alpha(%s)\\\n", vars_Find_Symbol(vars) );
		check_vars_caller= cvc;
		  /* Check for a second sorting which would destroy the info on the original order	*/
		if( (OK= !check_vars_reset( vars, n, vars_errfile)) ){
			for( i= 0; i< n; i++ ){
				if( vars[i].old_var && vars[i].old_var->old_id== i ){
					sorted+= 1;
				}
			}
		}
		  /* If not sorted before, and otherwise also correct; sort it!	*/
		if( sorted!= n && OK ){
			if( vsn && vs && calloc_error( _vs, VariableSelection, vsn) ){
				fprintf( vars_errfile,
					"sort_variables_list_alpha(): skipping list #%d; can't allocate %d size VariableSelection\n",
					N, vsn
				);
			}
			  /* store the old, presorting, ids	*/
			for( i= 0; i< n; i++ ){
				vars[i].old_id= vars[i].id;
			}
			if( ((!vsn && !vs) || _vs) ){
				  /* sort the Variables	*/
				qsort( vars, (unsigned) n, sizeof(Variable_t), alpha_var_sort);
				if( vs && _vs ){
					  /* make a copy of the original VariableSelection array	*/
					for( j= 0; j< vsn; j++){
						_vs[j]= vs[j];
					}
				}
				for( i= 0; i< n; i++){
				  Variable_t *ovar;
					if( vs && _vs ){
						for( j= 0; j< vsn; j++){
						  /* first check if there is a reference to vars[i] in
						   * the Selection list vs: update its reference id.
						   */
							if( _vs[j].Vars== vars && _vs[j].id== vars[i].id)
								vs[j].id= i;
						}
					}
					  /* set the reference to the Variable which used to have this id	*/
					for( ovar= NULL, j= 0; !ovar && j< n; j++ ){
					  /* this finds the old Variable that used to be in position i.	*/
						if( vars[j].old_id== i ){
							ovar= &vars[j];
						}
					}
					vars[i].old_var= ovar;
					  /* now update vars[i].id	*/
					vars[i].id= i;
				}
				  /* now check to see if there are VARIABLE_VARS
				   \ For those, we call ourselves again.
				   */
				if( vars_sort_follow_tree ){
					for( i= 0; i< n; i++){
						if( vars[i].type== VARIABLE_VAR ){
							c_v_on_PARENT_notify= 0;
							N+= sort_variables_list_alpha(
								vars[i].maxcount, vars[i].var.ptr, vsn, vs, 0
							);
						}
					}
				}
			}
			if( _vs){
				free( _vs);
				_vs= NULL;
			}
			c_v_on_PARENT_notify= 0;
			sprintf( cvc, "sort_variables_list_alpha(%s:done)\\\n", vars_Find_Symbol(vars) );
			check_vars_caller= cvc;
			check_vars_reset( vars, n, vars_errfile);
			N++;
		}
		n= va_arg(ap, long);
	}
	va_end(ap);
	check_vars_caller = _cvc_;
	return(N);
}

char ItemSeparator[32]= " \\\n\t";

int vars_print_varvarcommands= 1,
#ifdef CURLY_RESTRICTED
	vars_print_all_curly= 0;
#else
	vars_print_all_curly= 1;
#endif

int VarsPagerActive= False, VarsMeanPrinting= False;

/* int Variable_StartItem= 0;	*/

/* Prints the value (list of ~) on the specified Sinc.
 \ Only for numeric/text Variables; not for COMMAND/VARIABLE vars.
 */
int Sprint_var_value( Sinc *fp, Variable_t *var,
	char *underline, char* nounderline,
	char *bold, char *nobold,
	int *additional
)
{ char dum[128];
  int r;
	switch( var->type){
		case CHAR_VAR:
			Sputs( underline, fp );
			r= print_charvar( fp, var);
			if( !vars_print_all_curly ){
				Sputs( bold, fp);
				Sputc( RIGHT_BRACE, fp);
				Sputs( nobold, fp);
			}
			break;
		case UCHAR_VAR:
		case CHAR_PVAR:
			if( !vars_print_all_curly ){
				Sputc( ' ', fp);
			}
			Sputs( underline, fp );
			r= print_charvar( fp, var);
			break;
		case SHORT_VAR:
		case USHORT_VAR:
		case SHORT_PVAR:
			if( !vars_print_all_curly ){
				Sputc( ' ', fp);
			}
			Sputs( underline, fp );
			r= print_shortvar( fp, var);
			break;
		case INT_VAR:
		case UINT_VAR:
		case INT_PVAR:
			if( !vars_print_all_curly ){
				Sputc( ' ', fp);
			}
			Sputs( underline, fp );
			r= print_intvar( fp, var);
			break;
		case LONG_VAR:
		case ULONG_VAR:
		case LONG_PVAR:
			if( !vars_print_all_curly ){
				Sputc( ' ', fp);
			}
			Sputs( underline, fp );
			r= print_longvar( fp, var);
			break;
		case HEX_VAR:
			if( !vars_print_all_curly ){
				Sputc( ' ', fp);
			}
			Sputs( underline, fp );
			r= print_hexvar( fp, var);
			break;
		case FLOAT_VAR:
		case FLOAT_PVAR:
			if( !vars_print_all_curly ){
				Sputc( ' ', fp);
			}
			Sputs( underline, fp );
			r= print_floatvar( fp, var);
			break;
		case DOUBLE_VAR:
		case DOUBLE_PVAR:
			if( !vars_print_all_curly ){
				Sputc( ' ', fp);
			}
			Sputs( underline, fp );
			r= print_doublevar( fp, var);
			break;
		case COMMAND_VAR:
			Sputs( VarsSeparator, fp);
			sprintf( dum, "# %s%s{%s%s%s%s%s}%s %hu calls; last returned= %hu",
				bold,
				var->name,
				nobold, underline,
				(var->args)? var->args : "",
				nounderline, bold, nobold,
				var->maxcount, var->count
			);
			Sputs( dum, fp);
			break;
		default:
			Sputs( nounderline, fp);
			sprintf( dum, "## illegal type: '%s%s%s' %d",
				bold, var->name, nobold, var->id
			);
			Sputs( dum, fp);
			*additional= r = 0;
			break;
	}
	return( r );
}

/* print a Variable_t on stream <fp>.
 * format: <name>= <val1>[,<val2>,...<val'maxcount'>] [# valid <count>]
 * <count> is only printed if different from <maxcount>
 */
RET_SHORT Sprint_var( Sinc *fp, Variable_t *var, int verbose_print)
{	short r, len, Additional= 1, brace= 0, end_brace= 0;
	int additional= 1;
	char dum[128], *caller= "Sprint_var()";
     static char VarVarActive= 0;
	int depth;
     FILE *fpp= NULL;
	char *bold= "", *nobold= "", *underline= "", *nounderline= "";
	char *TERM= getenv( "TERM");
	int df= Delay_Flush;

	  /* check this Variable_t	*/
	if( fp->type== SINC_FILE){
/* 		fpp= (FILE*) fp;	*/
		fpp= fp->sinc.file;
	}

	if( VarsMeanPrinting ){
		c_v_PARENT_check= False;
	}

	check_vars_caller= caller;
	if( check_vars_reset( var, -1L, fpp) ){
		VarsMeanPrinting= False;
		return( 0);
	}
	if( fpp && fpp== stderr ){
		Delay_Flush= 2;
	}
	Sflush( fp);

	strcpy( dum, caller);
	strncat( dum, var->name, 127- strlen(dum) );
	PUSH_TRACE( dum, depth);
	if( verbose_print< 0){
		if( verbose_print== -1)
			verbose_print= additional= 0;
		else
			verbose_print= -verbose_print - 1;
		if( verbose_print>= 2){
			brace++;
		}
	}
	check_vars_caller= "Sprint_var()";
#ifdef _UNIX_C_
	if( TERM && fpp && (VarsPagerActive || isatty(fileno(fpp)) ) ){
		if( strncaseeq( TERM, "xterm", 5) || strncaseeq( TERM, "vt1", 3) ||
			strncaseeq( TERM, "vt2", 3)
		){
			bold= "\033[1m";
			nobold= "\033[m";
			underline= "\033[4m";
			nounderline= nobold;
		}
		else if( strncaseeq( TERM, "hp", 2) ){
			bold= "\033&dB";
			nobold= "\033&d@";
			underline= "\033&dD";
			nounderline= nobold;
		}
	}
#endif
	if( !VarVarActive && var->parent && !check_vars( var->parent, -1L, fpp) ){
		Sputs( var->parent->name, fp);
		Sputs( bold, fp);
		Sputs( "{ ", fp);
		Sputs( nobold, fp);
		brace++;
	}
	if( var->type!= COMMAND_VAR && var->type!= VARIABLE_VAR ){
		Sputs( bold, fp);
		if( VarsMeanPrinting ){
			Sputc( '[', fp);
		}
		Sputs( var->name, fp);
		if( VarsMeanPrinting ){
			Sputc( ']', fp);
		}
		if( var->first || var->last!= var->maxcount-1 ){
		  char dum[32];
			sprintf( dum, "[%d,%d]", var->first, var->last );
			Sputs( dum, fp );
		}
		if( !vars_print_all_curly ){
			if( var->type== CHAR_VAR){
				Sputc( LEFT_BRACE, fp);
				Sputs( nobold, fp);
			}
			else{
				Sputs( nobold, fp);
				Sputc( '=', fp);
			}
		}
		else{
			Sputc( LEFT_BRACE, fp);
			Sputs( nobold, fp);
			end_brace+= 1;
		}
	}
	Sputs( nobold, fp);
	switch( var->type){
		case COMMAND_VAR:
			Additional= 0;
			if( VarVarActive && !vars_print_varvarcommands ){
				break;
			}
			Sputs( nounderline, fp);
			Sputs( "### Command ", fp);
			sprintf( dum, "%s%s{%s%s%s%s%s}%s %hu calls; last returned= %hu",
				bold,
				var->name,
				nobold, underline,
				(var->args)? var->args : "",
				nounderline, bold, nobold,
				var->maxcount, var->count
			);
			Sputs( dum, fp);
			break;
		case VARIABLE_VAR:{
		  long i;
		  Variable_t *syn= var->var.ptr;
			if( !VarVarActive){
				VarVarActive+= 1;
				Sputs( nounderline, fp);
				Sputs( bold, fp);
				if( VarsMeanPrinting ){
					Sputc( '[', fp);
				}
				Sputs( var->name, fp);
				if( VarsMeanPrinting ){
					Sputc( ']', fp);
				}
				if( var->first || var->last!= var->maxcount-1 ){
				  char dum[32];
					sprintf( dum, "[%d,%d]", var->first, var->last );
					Sputs( dum, fp );
				}
				Sputs( "{ ", fp);
				Sputs( nobold, fp);
				for( i= var->first; i<= var->last; i++ ){
					Sprint_var( fp, &syn[i], -1);
					if( i!= var->last){
						Sputs( ItemSeparator, fp);
					}
				}
				Sputs( bold, fp);
				Sputs( " }", fp);
				Sputs( nobold, fp);
				VarVarActive= 0;
			}
			else{
				Sputs( "## ", fp);
				Sputs( var->name, fp);
				Sputs( "{ ", fp);
				Sputs( syn->name, fp);
				Sputs( " }", fp);
				Sputs( ": too many indirections", fp);
				Additional= 0;
			}
			break;
		}
		default:
			r= Sprint_var_value( fp, var, underline, nounderline, bold, nobold, &additional);
			break;
	}
	Sputs( nounderline, fp);
	if( brace ){
		Sputs( " ", fp);
		Sputs( bold, fp);
		while( brace> 1 ){
			Sputs( "} ", fp);
			brace--;
		}
		Sputs( "}", fp);
		brace--;
		Sputs( nobold, fp);
	}
	Sputs( bold, fp);
	while( end_brace ){
		Sputc( RIGHT_BRACE, fp);
		end_brace-= 1;
	}
	Sputs( nobold, fp);
	if( additional){
	  int sep= 0;
		if( Additional && var->alias ){
			if( !sep){
				Sputs( "\t#", fp);
				sep= 1;
			}
			else{
				Sputs( ";", fp );
			}
			sprintf( dum, " (%s)", var->alias );
			Sputs( dum, fp);
		}
		if( Additional ){
			if( !sep){
				Sputs( "\t#", fp);
				sep= 1;
			}
			else{
				Sputs( ";", fp );
			}
			sprintf( dum, " %lu of %lu", var->count, var->maxcount );
			Sputs( dum, fp);
		}
		if( Additional && var_label_change && var->change_label ){
			if( !sep){
				Sputs( "\t#", fp);
				sep= 1;
			}
			else{
				Sputs( ";", fp );
			}
			sprintf( dum, " access-label 0x%lx (%s)",
				var->change_label, vars_update_change_label_string( var->change_label, NULL)
			);
			Sputs( dum, fp);
		}
		if( verbose_print){
			if( !sep){
				Sputs( "\t#", fp);
				sep= 1;
			}
			else{
				Sputs( ";", fp );
			}
			sprintf( dum, " (%s)[#%d:0x%lx]", VarType[var->type], var->id, var->var.ptr);
			Sputs( dum, fp);
		}
		if( Additional && verbose_print>= 2){
			if( var->access>= 1){
				if( var->changed){
					Sputs( "\n ## ChangedString: '", fp);
					Sputs( CHANGED_FIELD(var), fp );
					Sputs( "'", fp);
				}
			}
			else if( var->access== -1){
				Sputs( "\n## Read Only", fp);
			}
		}
		if( verbose_print>= 3 ){
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
			if( var->change_handler){
			  char *ch= vars_Find_Symbol(var->change_handler);
				if( ch && ch[0]!= '\0' ){
					Sputs( "\n #C# ", fp);
					Sputs( ch, fp);
				}
			}
#endif
			if( var->description){
			  char *c= var->description;
				Sputs( "\n #D# '", fp);
				while( *c){
					if( *c== '\n'){
						Sputs( "\n #D#  ", fp);
					}
					else{
						Sputc( *c, fp);
					}
					c++;
				}
				Sputs( "'", fp);
			}
			if( var->parent){
				Sputs( "\n #P# '", fp);
				Sputs( var->parent->name, fp);
				Sputs( "'", fp);
				Sputs( "(", fp);
				Sputs( VarType[var->parent->type], fp);
				Sputs( ")", fp);
			}
		}
	}
	Delay_Flush= df;
	Sflush( fp);
	len= fp->_tlen;
	POP_TRACE(depth);
	VarsMeanPrinting= False;
	return( len );
}

RET_SHORT Sprint_array( Sinc *fp, TypeOfVariable type, int len, pointer data)
{ Variable_t Var, *var= &Var;

	memset( var, 0, sizeof(Variable_t) );
	Var.id= 0;
	Var.name="";
	Var.type= type;
	Var.var.ptr= data;
	Var.maxcount= len;

	return( Sprint_var( fp, var, 0) );
}

RET_SHORT fprint_array( FILE *fp, TypeOfVariable type, int len, pointer data)
{ Sinc sinc;
	return( Sprint_array( Sinc_file( &sinc, fp, 0, 0), type, len, data) );
}

char *sprint_array( char *buf, int buflen, TypeOfVariable type, int len, pointer data)
{ Sinc sinc;
	Sprint_array( Sinc_string( &sinc, buf, buflen, 0), type, len, data);
	return( buf );
}

int var_changed_flag( var, Index)
Variable_t *var;
int Index;
{
	check_vars_caller= "var_changed_flag()";
	if( check_vars_reset( (Variable_t*) var, -1L, NULL) )
		return(0);
	if( Index< 0 || Index>= var->maxcount){
		errno= EINDEXRANGE;
		return(0);
	}
	if( var->access>= 1 && var->changed ){
		return( (int) CHANGED_FIELD_INDEX(var,Index) );
	}
	else
		return(0);
}

int var_accessed( var, Index)
Variable_t *var;
int Index;
{  int flag= var_changed_flag( var, Index);
	return( (flag)? flag!= VAR_UNCHANGED : True );
}

char *var_ChangedString( var)
Variable_t *var;
{
	if( var->changed)
		return( CHANGED_FIELD(var) );
	else
		return( "");
}

int var_set_Variable( Variable_t *var, TypeOfVariable type, pointer val, unsigned int count, unsigned int maxcount)
/* Variable_t *var;	*/
/* TypeOfVariable type;	*/
/* pointer val;	*/
/* unsigned int count, maxcount;	*/
{
	if( var){
		var->type= type;
		var->count= (unsigned short) count;
		var->first= 0;
		var->last= (var->maxcount= (unsigned short) maxcount)- 1;
		var->var.ptr= val;
		check_vars_caller= "var_set_Variable()";
		return( !check_vars_reset( var, -1, NULL) );
	}
	else{
		return(0);
	}
}

double var_Value( var, Index)
Variable_t *var;
int Index;
{  double r= 0.0;
   static Variable_t *Var= NULL;
	if( !Var || Var!= var){
		check_vars_caller= "var_Value()";
		if( check_vars_reset( var, -1L, NULL) )	
			return( 0.0);
	}
	if( Index< 0 || Index>= var->maxcount){
		errno= EINDEXRANGE;
		return(0.0);
	}
	switch( var->type){
		case CHAR_VAR:
			r= (double) var->var.c[Index];
			break;
		case CHAR_PVAR:
			if( var->var.cp[Index])
				r= (double) *(var->var.cp[Index]);
			break;
		case UCHAR_VAR:
			r= (double) var->var.uc[Index];
			break;
		case SHORT_VAR:
			r= (double) var->var.s[Index];
			break;
		case SHORT_PVAR:
			if( var->var.sp[Index])
				r= (double) *(var->var.sp[Index]);
			break;
		case USHORT_VAR:
			r= (double) var->var.us[Index];
			break;
		case INT_VAR:
			r= (double) var->var.i[Index];
			break;
		case INT_PVAR:
			if( var->var.ip[Index])
				r= (double) *(var->var.ip[Index]);
			break;
		case UINT_VAR:
			r= (double) var->var.ui[Index];
			break;
		case LONG_VAR:
			r= (double) var->var.l[Index];
			break;
		case LONG_PVAR:
			if( var->var.lp[Index])
				r= (double) *(var->var.lp[Index]);
			break;
		case ULONG_VAR:
		case HEX_VAR:
			r= (double) var->var.ul[Index];
			break;
		case FLOAT_VAR:
			r= (double) var->var.f[Index];
			break;
		case FLOAT_PVAR:
			if( var->var.fp[Index])
				r= (double) *(var->var.fp[Index]);
			break;
		case DOUBLE_VAR:
			r= (double) var->var.d[Index];
			break;
		case DOUBLE_PVAR:
			if( var->var.dp[Index])
				r= (double) *(var->var.dp[Index]);
			break;
		case COMMAND_VAR:
			errno= EVARILLTYPE;
			set_NaN(r);
			break;
		case VARIABLE_VAR:{
			r= var_Value( &(((Variable_t*) var->var.ptr)[Index]), 0);
			break;
		}
		default:
			errno= EVARILLTYPE;
			set_NaN(r);
			break;
	}
	return(r);
}

double stdv_zero_thres= 0.0;

Variable_t *var_Mean( var, mean, ss)
Variable_t *var;
Variable_t *mean;
SimpleStats *ss;
{  unsigned long i= 0L;
   VariableType _var= var->var;
   VariableSet *set= mean->var.ptr;
   static char VarVarActive= 0;
   double val, stdv = -1;

	check_vars_caller= "var_Mean()";
	if( !ss || !mean || check_vars_reset( var, -1L, NULL) )	
		return( NULL);

	  /* Set the whole recipient Variable_t to zero	*/
	memset( mean, 0, sizeof(*mean) );
	  /* (re)initialise appropriate fields. <set> was
	   \ initialised in its declaration.
	   */
	mean->var.ptr= set;
	mean->type= var->type;
	mean->name= var->name;
	mean->id= var->id;
	  /* Used by Sprint_var to print this Variable_t as the
	   \ child of its parent.
	   */
	mean->parent= var->parent;
	SS_Reset( ss);

	if( var->type== COMMAND_VAR)
		return( NULL);
	if( var->type== VARIABLE_VAR ){
	  long I;
	  Variable_t *syn= var->var.ptr, _mean;
	  VariableSet _set[2];
	  SimpleStats _ss;
		if( !VarVarActive){
			VarVarActive++;
			memset( _set, 0, sizeof(_set) );
			_mean.var.ptr= &_set[0];
			for( I= 0; I< var->maxcount; I++, syn++ ){
				var_Mean( syn, &_mean, &_ss);
				SS_Add_( *ss, _ss);
			}
			mean->type= DOUBLE_VAR;
			if( ss->count){
				if( (stdv= SS_St_Dev(ss)) && stdv> stdv_zero_thres )
					mean->count= mean->maxcount= 2L;
				else
					mean->count= mean->maxcount= 1L;
			}
			mean->var.d[0]= SS_Mean(ss);
			mean->var.d[1]= stdv;
			mean->last= mean->maxcount- 1;
			VarVarActive--;
		}
	}
	else{
	  double count;
		  /* calculate the statistics	*/
		for( i= 0; i< var->maxcount; i++){
			count= 0.0;
			val= 0.0;
			switch( var->type){
				case CHAR_PVAR:
				case CHAR_VAR:
				case UCHAR_VAR:
					if( !(var->type== CHAR_PVAR && !_var.cp[i]) ){
						val= (double)( (var->type==CHAR_PVAR)? *(_var.cp[i]) : _var.c[i] );
						count= 1.0;
					}
					break;
				case SHORT_PVAR:
				case SHORT_VAR:
				case USHORT_VAR:
					if( !(var->type== SHORT_PVAR && !_var.sp[i]) ){
						val= (double)( (var->type==SHORT_PVAR)? *(_var.sp[i]) : _var.s[i] );
						count= 1.0;
					}
					break;
				case INT_PVAR:
				case INT_VAR:
				case UINT_VAR:
					if( !(var->type== INT_PVAR && !_var.ip[i]) ){
						val= (double)( (var->type==INT_PVAR)? *(_var.ip[i]) : _var.i[i] );
						count= 1.0;
					}
					break;
				case LONG_PVAR:
				case LONG_VAR:
				case ULONG_VAR:
				case HEX_VAR:
					if( !(var->type== LONG_PVAR && !_var.lp[i]) ){
						val= (double)( (var->type==LONG_PVAR)? *(_var.lp[i]) : _var.l[i] );
						count= 1.0;
					}
					break;
				case FLOAT_PVAR:
				case FLOAT_VAR:
					if( !(var->type== FLOAT_PVAR && !_var.fp[i]) ){
						val= (double)( (var->type==FLOAT_PVAR)? *(_var.fp[i]) : _var.f[i] );
						count= 1.0;
					}
					break;
				case DOUBLE_PVAR:
				case DOUBLE_VAR:
					if( !(var->type== DOUBLE_PVAR && !_var.dp[i]) ){
						val= (double)( (var->type==DOUBLE_PVAR)? *(_var.dp[i]) : _var.d[i] );
						count= 1.0;
					}
					break;
				default:
					errno= EVARILLTYPE;
					return(NULL);
					break;
			}
			SS_Add_Data_( *ss, (int)count, val, count );
		}
		if( ss->count){
			if( (stdv= SS_St_Dev(ss)) && stdv> stdv_zero_thres )
				mean->count= mean->maxcount= 2L;
			else
				mean->count= mean->maxcount= 1L;
			mean->last= mean->maxcount- 1;
		}
		  /* fill in the correct fields of the mean return Variable	*/
		switch( var->type){
			case CHAR_PVAR:
				mean->type= CHAR_VAR;
			case CHAR_VAR:
			case UCHAR_VAR:
				mean->var.c[0]= (char) SS_Mean(ss);
				mean->var.c[1]= (char) stdv;
				break;
			case SHORT_PVAR:
				mean->type= SHORT_VAR;
			case SHORT_VAR:
			case USHORT_VAR:
				mean->var.s[0]= d2short( SS_Mean(ss));
				mean->var.s[1]= d2short( stdv);
				break;
			case INT_PVAR:
				mean->type= INT_VAR;
			case INT_VAR:
			case UINT_VAR:
				mean->var.i[0]= d2int( SS_Mean(ss));
				mean->var.i[1]= d2int( stdv);
				break;
			case LONG_PVAR:
				mean->type= LONG_VAR;
			case LONG_VAR:
			case ULONG_VAR:
			case HEX_VAR:
				mean->var.l[0]= d2long( SS_Mean(ss));
				mean->var.l[1]= d2long( stdv);
				break;
			case FLOAT_PVAR:
				mean->type= FLOAT_VAR;
			case FLOAT_VAR:
				mean->var.f[0]= (float) SS_Mean(ss);
				mean->var.f[1]= (float) stdv;
				break;
			case DOUBLE_PVAR:
				mean->type= DOUBLE_VAR;
			case DOUBLE_VAR:
				mean->var.d[0]= SS_Mean(ss);
				mean->var.d[1]= stdv;
				break;
			default:
				errno= EVARILLTYPE;
				return(NULL);
				break;
		}
	}
	return( mean);
}

/* print the mean of a Variable_t on stream <fp>.
 * format: [name]= <val>
 * this cannot be read back!
 */
RET_SHORT Sprint_varMean( fp, var)
Sinc *fp;
Variable_t *var;
{ SimpleStats ss;
  Variable_t mean;
  VariableSet set[2];

	/* check this Variable_t	*/
	check_vars_caller= "Sprint_varMean()";
	if( fp->type== SINC_FILE){
		if( check_vars_reset( var, -1L, (FILE*) fp) )	
			return( 0);
	}
	else{
		if( check_vars_reset( var, -1L, NULL) )	
			return( 0);
	}

	memset( set, 0, sizeof(set) );
	mean.var.ptr= &set[0];

	if( !var_Mean( var, &mean, &ss) ){
		return(0);
	}

	VarsMeanPrinting= True;
	c_v_PARENT_check= False;
	return( Sprint_var( fp, &mean, -1) );
}

RET_SHORT sprint_var( fp, var, addr, len)
char *fp;
Variable_t *var;
int addr;
long len;
{	Sinc sinc;
	return( Sprint_var( Sinc_string( &sinc, fp, len, 0L), var, addr) );
}

RET_SHORT fprint_var( fp, var, addr)
FILE *fp;
Variable_t *var;
int addr;
{	Sinc sinc;
	return( Sprint_var( Sinc_file( &sinc, fp, 0L, 0L), var, addr) );
}

RET_SHORT print_var( FILE *fp, Variable_t *var, int addr)
{	Sinc sinc;
	short ret;
	int df= Delay_Flush;

	Delay_Flush= 2;
	ret= Sprint_var( Sinc_file( &sinc, fp, 0L, 0L), var, addr);
	ret+= fputc( '\n', fp);
	Delay_Flush= df;
	Flush_File(fp);
	return( ret);
}

RET_SHORT sprint_varMean( fp, var, len)
char *fp;
Variable_t *var;
long len;
{	Sinc sinc;
	return( Sprint_varMean( Sinc_string( &sinc, fp, len, 0L), var) );
}

RET_SHORT fprint_varMean( fp, var)
FILE *fp;
Variable_t *var;
{	Sinc sinc;
	return( Sprint_varMean( Sinc_file( &sinc, fp, 0L, 0L), var) );
}

RET_SHORT print_varMean( fp, var)
FILE *fp;
Variable_t *var;
{	Sinc sinc;
	short ret;
	int df= Delay_Flush;

	Delay_Flush= 2;
	ret= Sprint_varMean( Sinc_file( &sinc, fp, 0L, 0L), var);
	fputc( '\n', fp);
	Delay_Flush= df;
	Flush_File(fp);
	return( ret);
}

/* print_????var() functions: print the value(s) of var on file fp.	*/

int vars_compress_controlchars= 1, vars_print_as_string=1, list_level= 0;
int vars_echo= 0;

RET_SHORT print_charvar( fp, var)
Variable_t *var;
Sinc *fp;
{	long i;
	VariableType _var= var->var;
	char dum[32], *c;
	unsigned u;

	if( !var || !var->var.ptr)
		return( 0);
	switch( var->type){
		case UCHAR_VAR:
		case CHAR_PVAR:
		/* output a (range) of bytes	*/
			for( i= var->first; i<= var->last; i++){
				if( var->type== CHAR_PVAR){
					u= (_var.cp[i])? (unsigned)*(_var.cp[i]) : 0;
					sprintf( dum, "%u", u );
					Sputs( dum, fp);
				}
				else{
					u= (unsigned) _var.uc[i];
					sprintf( dum, "%u", u );
					Sputs( dum, fp);
				}
				if( i<= var->last- 1){
					Sputs( ", ", fp);
				}
			}
			break;
		case CHAR_VAR:
		  /* output a character or string	*/
			c= &(var->var.c[var->first]);
			for( i= var->first; i<= var->last && !(!*c && vars_print_as_string) ; i++, c++){
				if( *c< ' '){
				  int repcount= 0;
					while( vars_compress_controlchars && *c== c[repcount])
						repcount++;
					if( repcount> 1)
						Sputc( '[', fp);
					switch( *c){
						case '\\':
							Sputs( "\\\\", fp);
							break;
						case '\t':
							Sputs( "\\t", fp);
							break;
						case '\f':
							Sputs( "\\f", fp);
							break;
						case '\n':
							Sputs( "\\n", fp);
							break;
						case '\r':
							Sputs( "\\r", fp);
							break;
						case '\b':
							Sputs( "\\b", fp);
							break;
						case '\0':
							Sputs( "\\0", fp);
							break;
						case RIGHT_BRACE:
							Sputs( "\\}", fp);
							break;
						default:
							if( ((unsigned)*c) < '@'){
								Sputc( '^', fp);
								Sputc( *c+ '@', fp);
							}
							break;
					}
					if( repcount> 1){
						sprintf( dum, "*%d]", repcount);
						Sputs( dum, fp);
						c+= repcount- 1;
						i+= repcount- 1;
					}
				}
				else
					switch( *c){
						case '\\':
							Sputs( "\\\\", fp);
							break;

						case '^':
						case '~':
							Sputc( '\\', fp);
						default:
							Sputc( *c, fp);
							break;
					}
			}
			break;
	}
	return( 1);
}

RET_SHORT print_shortvar( fp, var)
Variable_t *var;
Sinc *fp;
{	unsigned short *us;
	long i;
	char dum[32];

	if( !var || !var->var.ptr)
		return( 0);
	switch( var->type){
		case SHORT_VAR:
		case SHORT_PVAR:
			for( i= (long) var->first; i<= var->last; i++){
				if( var->type== SHORT_PVAR){
					sprintf( dum, "%d", (var->var.sp[i])? *(var->var.sp[i]) : 0 );
					Sputs( dum, fp);
				}
				else{
					sprintf( dum, "%d", var->var.s[i] );
					Sputs( dum, fp);
				}
				if( i<= var->last- 1)
					Sputs( ", ", fp);
			}
			break;
		case USHORT_VAR:
			us= &(var->var.us[var->first]);
			for( i= (long) var->first; i<= var->last; i++){
				sprintf( dum, "%u", (unsigned) *us++);
				Sputs( dum, fp);
				if( i<= var->last- 1)
					Sputs( ", ", fp);
			}
			break;
	}
	return( 1);
}

RET_SHORT print_intvar( fp, var)
Variable_t *var;
Sinc *fp;
{	long i;
	unsigned int *uI;
	char dum[64];

	if( !var || !var->var.ptr)
		return( 0);
	switch( var->type){
		case INT_VAR:
		case INT_PVAR:
			for( i= (long) var->first; i<= var->last; i++){
				if( var->type== INT_PVAR){
					sprintf( dum, "%d", (var->var.ip[i])? *(var->var.ip[i]) : 0 );
					Sputs( dum, fp);
				}
				else{
					sprintf( dum, "%d", var->var.i[i] );
					Sputs( dum, fp);
				}
				if( i<= var->last- 1)
					Sputs( ", ", fp);
			}
			break;
		case UINT_VAR:
			uI= &(var->var.ui[var->first]);
			for( i= (long) var->first + 1L /*?*/; i<= var->last+1; i++){
				sprintf( dum, "%u", (unsigned) *uI++);
				Sputs( dum, fp);
				if( i<= var->last)
					Sputs( ", ", fp);
			}
			break;
	}
	return( 1);
}

RET_SHORT print_longvar( fp, var)
Variable_t *var;
Sinc *fp;
{	long i;
	unsigned long *ul;
	char dum[64];

	if( !var || !var->var.ptr)
		return( 0);
	switch( var->type){
		case LONG_VAR:
		case LONG_PVAR:
			for( i= (long) var->first; i<= var->last; i++){
				if( var->type== LONG_PVAR){
					sprintf( dum, "%ld", (var->var.lp[i])? *(var->var.lp[i]) : 0 );
					Sputs( dum, fp);
				}
				else{
					sprintf( dum, "%ld", var->var.l[i] );
					Sputs( dum, fp);
				}
				if( i<= var->last- 1)
					Sputs( ", ", fp);
			}
			break;
		case ULONG_VAR:
			ul= &(var->var.ul[var->first]);
			for( i= var->first + 1L; i<= var->last+1; i++){
				sprintf( dum, "%lu", *ul++);
				Sputs( dum, fp);
				if( i<= var->last)
					Sputs( ", ", fp);
			}
			break;
	}
	return( 1);
}

RET_SHORT print_hexvar( fp, var)
Variable_t *var;
Sinc *fp;
{	long i;
	long *l;
	char dum[32];

	if( !var || !var->var.ptr)
		return( 0);
	if( var->type== HEX_VAR){
		l= &(var->var.l[var->first]);
		for( i= var->first + 1L; i<= var->last+1; i++){
			sprintf( dum, "0x%lx", *l++);
			Sputs( dum, fp);
			if( i<= var->last)
				Sputs( ", ", fp);
		}
	}
	return( 1);
}

char double_print_format[16]= "%g";
int try_reverse_double= 1;
extern int Allow_Fractions;

RET_SHORT print_floatvar( fp, var)
Variable_t *var;
Sinc *fp;
{	long i;
	float d;
	char format[18];
	int trd= try_reverse_double, AF= Allow_Fractions;

	if( !var || !var->var.ptr)
		return( 0);
	try_reverse_double= 0;
	if( trd ){
		Allow_Fractions= 1;
	}
	else{
		Allow_Fractions= 0;
	}
	if( var->type== FLOAT_VAR || var->type== FLOAT_PVAR ){
		for( i= (long) var->first; i<= var->last; i++ ){
			if( var->type== FLOAT_PVAR ){
				d= (var->var.fp[i])? *(var->var.fp[i]) : 0.0;
			}
			else{
				d= var->var.f[i];
			}
			if( fabs(d)< 1.0 && try_reverse_double &&
/* 				!strneq( double_print_format, "1/", 2) && double_print_format[0]!= '/'	*/
				!index( double_print_format, '/')
			){
			  char buf[256], *c;
			  int neg;
				if( d< 0 ){
					d*= -1;
					neg= 1;
					Sputs( "-", fp);
				}
				else{
					neg= 0;
				}
				sprintf( format, "1/%s", double_print_format);
				d2str( d, format, buf);
				c= d2str( d, double_print_format, NULL);
				if( strlen( buf) <= strlen( c ) ){
					Sputs( buf, fp);
				}
				else{
					Sputs( c, fp);
				}
			}
			else{
				Sputs( d2str( d, double_print_format, NULL ), fp);
			}
			if( i<= var->last- 1){
				Sputs( ", ", fp);
			}
			Sflush( fp );
		}
	}
	try_reverse_double= trd;
	Allow_Fractions= AF;
	return( 1);
}

RET_SHORT print_doublevar( fp, var)
Sinc *fp;
Variable_t *var;
{	long i;
	double d;
	char format[18];
	int trd= try_reverse_double, AF= Allow_Fractions;

	if( !var || !var->var.ptr)
		return( 0);
	try_reverse_double= 0;
	if( trd ){
		Allow_Fractions= 1;
	}
	else{
		Allow_Fractions= 0;
	}
	if( var->type== DOUBLE_VAR || var->type== DOUBLE_PVAR ){
		for( i= (long) var->first; i<= var->last; i++ ){
			if( var->type== DOUBLE_PVAR ){
				d= (var->var.dp[i])? *(var->var.dp[i]) : 0.0;
			}
			else{
				d= var->var.d[i];
			}
			if( fabs(d)< 1.0 && try_reverse_double &&
/* 				!strneq( double_print_format, "1/", 2) && double_print_format[0]!= '/'	*/
				!index( double_print_format, '/')
			){
			  char buf[256], *c;
			  int neg;
				if( d< 0 ){
					d*= -1;
					neg= 1;
					Sputs( "-", fp);
				}
				else{
					neg= 0;
				}
				sprintf( format, "1/%s", double_print_format);
				d2str( d, format, buf);
				c= d2str( d, double_print_format, NULL);
				if( strlen( buf) <= strlen( c ) ){
					Sputs( buf, fp);
				}
				else{
					Sputs( c, fp);
				}
			}
			else{
				Sputs( d2str( d, double_print_format, NULL ), fp);
			}
			if( i<= var->last- 1){
				Sputs( ", ", fp);
			}
			Sflush( fp );
		}
	}
	try_reverse_double= trd;
	Allow_Fractions= AF;
	return( 1);
}

/* search for a variable in the set 'vars[n]', starting from
 * vars[0]. If a Variable with name matching (a part of) 'buf'
 * is found, a pointer to it is returned. 'I' is updated to
 * the position relative to vars[0]. 'plac_in_buf' points to
 * the first character of the part of 'buf' matching the name.
 * Any error logging is done on FILE 'errfp'.
 * To search for multiple occurences, repeated calls of type
 *	var= &the_Variables[0]; I= var->id;
 * 	while( (var= find_next_named_var( "...foo= bar", var, 10, &I, points_to_foo, stderr) ){
 * 		if( var!= the_Variables[I])
 * 			fprintf( stderr, "Something is wrong...\n");
 * 		var++; I++;
 * 	}
 * can be made. 
 */

static int var_syntax_ok( name, buf, var)
char *name, *buf;
Variable_t *var;
{
	if( !var->hit ){
		if( buf && name> buf){
			if( (index( " \t,\n{}[]", name[-1]) || name[-1]== '\0') ){
				var->hit= 1;
			}
		}
		else{
			var->hit= 1;
		}
		if( var->hit){
			return( 1);
		}
	}
	else{
		var->hit= 0;
	}
	return( 0);
}

static Variable_t *find_next_named_var( buf, vars, n, I, place_in_buf, match, option, errfp)
char *buf;
Variable_t *vars;
long n, *I;
char **match, **place_in_buf, **option;
FILE *errfp;
{	static Variable_t *var= NULL;

	if( !(vars== var && var) ){
		check_vars_caller= "find_next_named_var()";
		if( check_vars_reset( vars, n, errfp))
			return( NULL);
	}
	var= vars;
	for( ; var->id< n-1; var++, *I+=1L ){
		if( (*place_in_buf= strstr( buf, var->name)) ){
			if( var_syntax_ok( *place_in_buf, buf, var) ){
				*match= var->name;
				  /* find the first character following the matched pattern	*/
				*option= &((*place_in_buf)[strlen(*match)]);
				return( var);
			}
		}
		else if( var->alias && (*place_in_buf= strstr( buf, var->alias)) ){
			if( var_syntax_ok( *place_in_buf, buf, var) ){
				*match= var->alias;
				*option= &((*place_in_buf)[strlen(*match)]);
				return( var);
			}
		}
	}
	if( var->id== n- 1){
		if( (*place_in_buf= strstr( buf, var->name)) ){
			if( var_syntax_ok( *place_in_buf, buf, var) ){
				*match= var->name;
				*option= &((*place_in_buf)[strlen(*match)]);
				return( var);
			}
		}
		else if( var->alias && (*place_in_buf= strstr( buf, var->alias )) ){
			if( var_syntax_ok( *place_in_buf, buf, var) ){
				*match= var->alias;
				*option= &((*place_in_buf)[strlen(*match)]);
				return( var);
			}
		}
	}
	return( NULL);
}

static Variable_t *find_next_regex_var( buf, vars, n, I, place_in_buf, match, option, errfp)
char *buf;
Variable_t *vars;
long n, *I;
char **match, **place_in_buf, **option;
FILE *errfp;
{	static Variable_t *var= NULL;
 	char O;

	if( !(vars== var && var) ){
		check_vars_caller= "find_next_named_var()";
		if( check_vars_reset( vars, n, errfp))
			return( NULL);
	}
	var= vars;
	if( !(*option= rindex(&buf[1], '$')) ){
		fprintf( errfp, "find_next_regex_var(%s): no $ to terminate regex\n", buf);
		return(NULL);
	}
	(*option)++;
	O= **option;
	**option= '\0';
	if( re_comp(buf) ){
		fprintf( errfp, "find_next_regex_var(%s): can't compile regex\n", buf);
		return(NULL);
	}
	**option= O;
	*place_in_buf= buf;
#define re_exec_len	re_exec
	for( ; var->id< n-1; var++, *I+=1L ){
		if( (re_exec_len( var->name)) ){
			if( var_syntax_ok( var->name, NULL, var) ){
				*match= var->name;
				return( var);
			}
		}
		else if( var->alias && (re_exec_len( var->alias)) ){
			if( var_syntax_ok( var->name, NULL, var) ){
				*match= var->alias;
				return( var);
			}
		}
	}
	if( var->id== n- 1){
		if( (re_exec_len( var->name)) ){
			if( var_syntax_ok( var->name, NULL, var) ){
				*match= var->name;
				return( var);
			}
		}
		else if( var->alias && (re_exec_len( var->alias )) ){
			if( var_syntax_ok( var->name, NULL, var) ){
				*match= var->alias;
				return( var);
			}
		}
	}
	return( NULL);
#undef re_exec_len
}

/* This routine does more or less the same thing. It just doesn't
 * interfere with find_next_named_var, which is meant to be used by
 * parse_varline. Use of find_next_named_var inside a VarChangeHandler
 * routine can result in a closed loop situation.
 * This routine compares over the length of var->name!
 */
#ifdef OLD_FIND_NAMED_VAR
Variable_t *old_find_named_var( name, exact, vars, n, I, errfp)
char *name;
int exact;
Variable_t *vars;
long n, *I;
FILE *errfp;
{	static Variable_t *var= NULL;
	long II= 0;

	if( !(vars== var && var) ){
		c_v_on_NULLptr= 0;
		check_vars_caller= "find_named_var()";
		if( check_vars_reset( vars, n, errfp) )
			return( NULL);
	}
	if( !I)
		I= &II;
	var= vars;
	for( ; var->id< n-1; var++, *I+=1L ){
		if( exact){
			if( !strcmp( name, var->name )  || (var->alias && streq(var->alias, name)) ){
				return( var);
			}
		}
		else{
			if( !strncmp( name, var->name, strlen(var->name) )  ||
				(var->alias && strneq(var->alias, name, strlen(var->alias) ))
			){
				return( var);
			}
		}
	}
	if( var->id== n- 1){
		if( exact){
			if( !strcmp( name, var->name )  || (var->alias && streq(var->alias, name)) ){
				return( var);
			}
		}
		else{
			if( !strncmp( name, var->name, strlen(var->name) ) ||
				(var->alias && strneq(var->alias, name, strlen(var->alias) ))
			){
				return( var);
			}
		}
	}
	return( NULL);
}
#else

Variable_t *find_named_var( _name, exact, vars, n, I, errfp)
char *_name;
int exact;
Variable_t *vars;
long n, *I;
FILE *errfp;
{	static Variable_t *var= NULL;
	long II= 0;
	int len;
	char *c;
	int C= False;
	static char *name= NULL;
	static int name_len= 0;

	if( !_name ){
		return(NULL);
	}
	if( !(vars== var && var) ){
		c_v_on_NULLptr= 0;
		check_vars_caller= "find_named_var()";
		if( check_vars_reset( vars, n, errfp) )
			return( NULL);
	}
	if( !I){
		I= &II;
	}
	var= vars;
	if( strlen(_name)> name_len || !name ){
		if( name ){
			lib_free(name);
		}
		name= strdup(_name);
		name_len= strlen(name);
	}
	else{
		strcpy( name, _name );
	}
	if( !exact ){
	  /* comparison up to the first whitespace or ",;"	*/
		c= name;
		while( *c && !isspace((unsigned char)*c) && !index( ",;", *c) ){
			c++;
		}
		  /* Found a match: set it to zero	*/
		if( c && *c && c!= name ){
			C= True;
			*c= '\0';
		}
	}
	len= strlen(name);
	for( ; var->id< n-1; var++, *I+=1L ){
		if( exact){
			if( !strcmp( name, var->name )  || (var->alias && streq(var->alias, name)) ){
				return( var);
			}
		}
		else{
 /* If we found a whitespace or ";,", the comparison takes place over the length
			   \ of the string to be matched; else over the length of the name or alias string.
			if( !strncmp( name, var->name, (C)? len : strlen(var->name) )  ||
				(var->alias && strneq(var->alias, name, (C)? len : strlen(var->alias) ))
			){
				return( var);
			}
 */
			  /* if a marker was found, we do an exact compare.	*/
			if( C ){
				if( !strcmp( name, var->name )  || (var->alias && streq(var->alias, name)) ){
					return( var);
				}
			}
			else{
				if( !strncmp( name, var->name, strlen(var->name) )  ||
					(var->alias && strneq(var->alias, name, strlen(var->alias) ))
				){
					return( var);
				}
			}
		}
	}
	if( var->id== n- 1){
		if( exact){
			if( !strcmp( name, var->name )  || (var->alias && streq(var->alias, name)) ){
				return( var);
			}
		}
		else{
/* 
			if( !strncmp( name, var->name, (C)? len : strlen(var->name) ) ||
				(var->alias && strneq(var->alias, name, (C)? len : strlen(var->alias) ))
			){
				return( var);
			}
 */
			if( C ){
				if( !strcmp( name, var->name )  || (var->alias && streq(var->alias, name)) ){
					return( var);
				}
			}
			else{
				if( !strncmp( name, var->name, strlen(var->name) )  ||
					(var->alias && strneq(var->alias, name, strlen(var->alias) ))
				){
					return( var);
				}
			}
		}
	}
	return( NULL);
}

#endif

Variable_t *find_var( ptr, vars, n, I, errfp)
pointer ptr;
Variable_t *vars;
long n, *I;
FILE *errfp;
{	static Variable *var= NULL;

	if( !(vars== (Variable_t*)var && var) ){
		check_vars_caller= "find_var()";
		if( check_vars_reset( vars, n, errfp)){
			return( NULL);
		}
	}
	var= (Variable*)vars;
	for( ; var->id< n-1; var++, *I+=1L ){
		if( var->var == ptr ){
			return( (Variable_t*) var);
		}
	}
	if( var->id== n- 1){
		if( var->var == ptr ){
			return( (Variable_t*) var);
		}
	}
	return( NULL);
}

Variable_t *find_named_VariableSelection( name, XVariables, MaxXVariables, I, J, vsret)
char *name;
VariableSelection *XVariables, **vsret;
int MaxXVariables, *I, *J;
{  int hit= 0;
   Variable_t *x_Var= NULL;
	while( *I< MaxXVariables && !hit){
	  int N;
		*vsret= &XVariables[*I];
		if( (N= XVariables[*I].id)< 0 ){
			N*= -1;
			while( *J< N && !hit){
				x_Var= &XVariables[*I].Vars[*J];
				(*J)++;
				if( !name || strcmp( name, x_Var->name)== 0 || (x_Var->alias && streq(x_Var->alias, name)) ){
					hit= 1;
					(*vsret)->Index= *I;
					(*vsret)->subIndex= *J- 1;
				}
			}
			if( *J>= N){
				*J= 0;
				(*I)++;
			}
		}
		else{
			x_Var= &XVariables[*I].Vars[N];
			(*I)++;
			if( !name || strcmp( name, x_Var->name)== 0 || (x_Var->alias && streq(x_Var->alias, name)) ){
				hit= 1;
				(*vsret)->Index= *I- 1;
				(*vsret)->subIndex= 0;
			}
		}
	}
	if( hit)
		return( x_Var);
	else
		return( NULL);
}

/* functions operating on the contents of a variable.
 * the double-array operands contains the rvalues
 * , the variable var the lvalues of the operator.
 * If operand contains {a,b,c}, then a will work
 * on var[0], b on var[1], and c on the rest of var.
 */
long power_var( var, operand, operands, count)
Variable_t *var;
double *operand;
int operands, *count;
{  int i;
   VariableType *lvalue= &var->var;
	operand= &operand[var->first];
	for( i= var->first; i<= var->last; i++){
		switch( var->type){
			case CHAR_VAR:
				lvalue->c[i] = (char)( pow((double) lvalue->c[i], *operand) );
				break;
			case CHAR_PVAR:
				if( lvalue->cp[i])
					*(lvalue->cp[i]) = (char)( pow((double) *(lvalue->cp[i]), *operand) );
				break;
			case UCHAR_VAR:
				lvalue->uc[i] = (unsigned char)( pow((double) lvalue->uc[i], *operand) );
				break;
			case SHORT_VAR:
				lvalue->s[i] =  d2short( pow((double) lvalue->s[i], *operand) );
				break;
			case SHORT_PVAR:
				if( lvalue->sp[i])
					*(lvalue->sp[i]) =  d2short( pow((double) *(lvalue->sp[i]), *operand) );
				break;
			case USHORT_VAR:
				lvalue->us[i] *= (unsigned short) d2short(*operand);
				break;
			case INT_VAR:
				lvalue->i[i] = d2int( pow((double) lvalue->i[i], *operand) );
				break;
			case INT_PVAR:
				if( lvalue->ip[i])
					*(lvalue->ip[i]) = d2int( pow((double) *(lvalue->ip[i]), *operand) );
				break;
			case UINT_VAR:
				lvalue->ui[i] = (unsigned int)d2int( pow((double) lvalue->ui[i], *operand) );
				break;
			case LONG_VAR:
				lvalue->l[i] = d2long( pow((double) lvalue->l[i], *operand) );
				break;
			case LONG_PVAR:
				if( lvalue->lp[i])
					*(lvalue->lp[i]) = d2long( pow((double) *(lvalue->lp[i]), *operand) );
				break;
			case ULONG_VAR:
			case HEX_VAR:
				lvalue->ul[i] = (unsigned long)d2long( pow((double) lvalue->ul[i], *operand) );
				break;
			case FLOAT_VAR:
				lvalue->f[i] = (float) pow( lvalue->f[i], *operand);
				break;
			case FLOAT_PVAR:
				if( lvalue->fp[i])
					(*lvalue->fp[i]) = (float) pow( (*lvalue->fp[i]), *operand);
				break;
			case DOUBLE_VAR:
				lvalue->d[i] = pow( lvalue->d[i], *operand);
				break;
			case DOUBLE_PVAR:
				if( lvalue->dp[i])
					(*lvalue->dp[i]) = pow( (*lvalue->dp[i]), *operand);
				break;
		}
		if( *operand!= 1.0){
			(*count)++;
			SET_CHANGED_FIELD( var, i, VAR_CHANGED);
		}
		else
			SET_CHANGED_FIELD( var, i, VAR_REASSIGN);
		if( i+ 1< operands)
			operand++;
	}
	return( (long) i );
}

long multiply_var( var, operand, operands, count)
Variable_t *var;
double *operand;
int operands, *count;
{  int i;
   VariableType *lvalue= &var->var;
	operand= &operand[var->first];
	for( i= var->first; i<= var->last; i++){
		switch( var->type){
			case CHAR_VAR:
				lvalue->c[i] = (char)( lvalue->c[i] * *operand);
				break;
			case CHAR_PVAR:
				if( lvalue->cp[i])
					*(lvalue->cp[i]) = (char)( *(lvalue->cp[i]) * *operand);
				break;
			case UCHAR_VAR:
				lvalue->uc[i] = (unsigned char)( lvalue->uc[i] * *operand);
				break;
			case SHORT_VAR:
				lvalue->s[i] =  d2short( lvalue->s[i] * *operand);
				break;
			case SHORT_PVAR:
				if( lvalue->sp[i])
					*(lvalue->sp[i]) =  d2short( *(lvalue->sp[i]) * *operand);
				break;
			case USHORT_VAR:
				lvalue->us[i] *= (unsigned short)d2short(*operand);
				break;
			case INT_VAR:
				lvalue->i[i] = d2int( lvalue->i[i] * *operand);
				break;
			case INT_PVAR:
				if( lvalue->ip[i])
					*(lvalue->ip[i]) = d2int( *(lvalue->ip[i]) * *operand);
				break;
			case UINT_VAR:
				lvalue->ui[i] = (unsigned int)d2int( lvalue->ui[i] * *operand);
				break;
			case LONG_VAR:
				lvalue->l[i] = d2long( lvalue->l[i] * *operand);
				break;
			case LONG_PVAR:
				if( lvalue->lp[i])
					*(lvalue->lp[i]) = d2long( *(lvalue->lp[i]) * *operand);
				break;
			case ULONG_VAR:
			case HEX_VAR:
				lvalue->ul[i] = (unsigned long)d2long( lvalue->ul[i] * *operand);
				break;
			case FLOAT_VAR:
				lvalue->f[i] = (float) (lvalue->f[i] * *operand);
				break;
			case FLOAT_PVAR:
				if( lvalue->fp[i])
					(*lvalue->fp[i]) = (float) ( (*lvalue->fp[i]) * *operand);
				break;
			case DOUBLE_VAR:
				lvalue->d[i] *= *operand;
				break;
			case DOUBLE_PVAR:
				if( lvalue->dp[i])
					*(lvalue->dp[i]) *= *operand;
				break;
		}
		if( *operand!= 1.0){
			(*count)++;
			SET_CHANGED_FIELD( var, i, VAR_CHANGED);
		}
		else
			SET_CHANGED_FIELD( var, i, VAR_REASSIGN);
		if( i+ 1< operands)
			operand++;
	}
	return( (long) i );
}

long divide_var( var, operand, operands, count)
Variable_t *var;
double *operand;
int operands, *count;
{  int i;
   VariableType *lvalue= &var->var;
	operand= &operand[var->first];
	for( i= var->first; i<= var->last; i++){
		if( *operand== 0){
			errno= EDIVZERO;
			if( var->type== DOUBLE_VAR){
				set_NaN( lvalue->d[i]);
			}
			else if( var->type== FLOAT_VAR ){
			  /* hope this works...	*/
			  double d;
				set_NaN( d );
				lvalue->f[i]= (float) d;
			}
			return( EOF);
		}
		switch( var->type){
			case CHAR_VAR:
				lvalue->c[i] = (char)( lvalue->c[i] / *operand);
				break;
			case CHAR_PVAR:
				if( lvalue->cp[i])
					*(lvalue->cp[i]) = (char)( *(lvalue->cp[i]) / *operand);
				break;
			case UCHAR_VAR:
				lvalue->uc[i] = (unsigned char)( lvalue->uc[i] / *operand);
				break;
			case SHORT_VAR:
				lvalue->s[i] =  d2short( lvalue->s[i] / *operand);
				break;
			case SHORT_PVAR:
				if( lvalue->sp[i])
					*(lvalue->sp[i]) =  d2short( *(lvalue->sp[i]) / *operand);
				break;
			case USHORT_VAR:
				lvalue->us[i] = (unsigned short)d2short( lvalue->us[i] / *operand);
				break;
			case INT_VAR:
				lvalue->i[i] = d2int( lvalue->i[i] / *operand);
				break;
			case INT_PVAR:
				if( lvalue->ip[i])
					*(lvalue->ip[i]) = d2int( *(lvalue->ip[i]) / *operand);
				break;
			case UINT_VAR:
				lvalue->ui[i] = (unsigned int)d2int( lvalue->ui[i] / *operand);
				break;
			case LONG_VAR:
				lvalue->l[i] = d2long( lvalue->l[i] / *operand);
				break;
			case LONG_PVAR:
				if( lvalue->lp[i])
					*(lvalue->lp[i]) = d2long( *(lvalue->lp[i]) / *operand);
				break;
			case ULONG_VAR:
			case HEX_VAR:
				lvalue->ul[i] = (unsigned long)d2long( lvalue->ul[i] / *operand);
				break;
			case FLOAT_VAR:
				lvalue->f[i] = (float) ( lvalue->f[i] / *operand);
				break;
			case FLOAT_PVAR:
				if( lvalue->fp[i])
					(*lvalue->fp[i]) = (float) ( (*lvalue->fp[i]) / *operand);
				break;
			case DOUBLE_VAR:
				lvalue->d[i] /= *operand;
				break;
			case DOUBLE_PVAR:
				if( lvalue->dp[i])
					*(lvalue->dp[i]) /= *operand;
				break;
		}
		if( *operand!= 1.0){
			(*count)++;
			SET_CHANGED_FIELD( var, i, VAR_CHANGED);
		}
		else
			SET_CHANGED_FIELD( var, i, VAR_REASSIGN);
		if( i+1< operands)
			operand++;
	}
	return( (long) i );
}

long add_var( var, operand, operands, count)
Variable_t *var;
double *operand;
int operands, *count;
{  int i;
   VariableType *lvalue= &var->var;
	operand= &operand[var->first];
	for( i= var->first; i<= var->last; i++){
		switch( var->type){
			case CHAR_VAR:
				lvalue->c[i] = (char)( lvalue->c[i] + *operand);
				break;
			case CHAR_PVAR:
				if( lvalue->cp[i])
					*(lvalue->cp[i]) = (char)( *(lvalue->cp[i]) + *operand);
				break;
			case UCHAR_VAR:
				lvalue->uc[i] = (unsigned char)( lvalue->uc[i] + *operand);
				break;
			case SHORT_VAR:
				lvalue->s[i] =  d2short( lvalue->s[i] + *operand);
				break;
			case SHORT_PVAR:
				if( lvalue->sp[i])
					*(lvalue->sp[i]) =  d2short( *(lvalue->sp[i]) + *operand);
				break;
			case USHORT_VAR:
				lvalue->us[i] = (unsigned short)d2short( lvalue->us[i] + *operand);
				break;
			case INT_VAR:
				lvalue->i[i] = d2int( lvalue->i[i] + *operand);
				break;
			case INT_PVAR:
				if( lvalue->ip[i])
					*(lvalue->ip[i]) = d2int( *(lvalue->ip[i]) + *operand);
				break;
			case UINT_VAR:
				lvalue->ui[i] = (unsigned int)d2int( lvalue->ui[i] + *operand);
				break;
			case LONG_VAR:
				lvalue->l[i] = d2long( lvalue->l[i] + *operand);
				break;
			case LONG_PVAR:
				if( lvalue->lp[i])
					*(lvalue->lp[i]) = d2long( *(lvalue->lp[i]) + *operand);
				break;
			case ULONG_VAR:
			case HEX_VAR:
				lvalue->ul[i] = (unsigned long)d2long( lvalue->ul[i] + *operand);
				break;
			case FLOAT_VAR:
				lvalue->f[i] = (float) ( lvalue->f[i] + *operand);
				break;
			case FLOAT_PVAR:
				if( lvalue->fp[i])
					(*lvalue->fp[i]) = (float) ( (*lvalue->fp[i]) + *operand);
				break;
			case DOUBLE_VAR:
				lvalue->d[i] += *operand;
				break;
			case DOUBLE_PVAR:
				if( lvalue->dp[i])
					*(lvalue->dp[i]) += *operand;
				break;
		}
		if( *operand!= 0.0){
			(*count)++;
			SET_CHANGED_FIELD( var, i, VAR_CHANGED);
		}
		else
			SET_CHANGED_FIELD( var, i, VAR_REASSIGN);
		if( i+1< operands)
			operand++;
	}
	return( (long) i );
}

long subtract_var( var, operand, operands, count)
Variable_t *var;
double *operand;
int operands, *count;
{  int i;
   VariableType *lvalue= &var->var;
	operand= &operand[var->first];
	for( i= var->first; i<= var->last; i++){
		switch( var->type){
			case CHAR_VAR:
				lvalue->c[i] = (char)( lvalue->c[i] - *operand);
				break;
			case CHAR_PVAR:
				if( lvalue->cp[i])
					*(lvalue->cp[i]) = (char)( *(lvalue->cp[i]) - *operand);
				break;
			case UCHAR_VAR:
				lvalue->uc[i] = (unsigned char)( lvalue->uc[i] - *operand);
				break;
			case SHORT_VAR:
				lvalue->s[i] =  d2short( lvalue->s[i] - *operand);
				break;
			case SHORT_PVAR:
				if( lvalue->sp[i])
					*(lvalue->sp[i]) =  d2short( *(lvalue->sp[i]) - *operand);
				break;
			case USHORT_VAR:
				lvalue->us[i] = (unsigned short)d2short( lvalue->us[i] - *operand);
				break;
			case INT_VAR:
				lvalue->i[i] = d2int( lvalue->i[i] - *operand);
				break;
			case INT_PVAR:
				if( lvalue->ip[i])
					*(lvalue->ip[i]) = d2int( *(lvalue->ip[i]) - *operand);
				break;
			case UINT_VAR:
				lvalue->ui[i] = (unsigned int)d2int( lvalue->ui[i] - *operand);
				break;
			case LONG_VAR:
				lvalue->l[i] = d2long( lvalue->l[i] - *operand);
				break;
			case LONG_PVAR:
				if( lvalue->lp[i])
					*(lvalue->lp[i]) = d2long( *(lvalue->lp[i]) - *operand);
				break;
			case ULONG_VAR:
			case HEX_VAR:
				lvalue->ul[i] = (unsigned long)d2long( lvalue->ul[i] - *operand);
				break;
			case FLOAT_VAR:
				lvalue->f[i] = (float) ( lvalue->f[i] - *operand);
				break;
			case FLOAT_PVAR:
				if( lvalue->fp[i])
					(*lvalue->fp[i]) = (float) ( (*lvalue->fp[i]) - *operand);
				break;
			case DOUBLE_VAR:
				lvalue->d[i] -= *operand;
				break;
			case DOUBLE_PVAR:
				if( lvalue->dp[i])
					*(lvalue->dp[i]) -= *operand;
				break;
		}
		if( *operand!= 0.0 ){
			(*count)++;
			SET_CHANGED_FIELD( var, i, VAR_CHANGED);
		}
		else
			SET_CHANGED_FIELD( var, i, VAR_REASSIGN);
		if( i+1< operands)
			operand++;
	}
	return( (long) i );
}

long set_var( var, operand, operands, count)
Variable_t *var;
double *operand;
int operands, *count;
{  int i;
   VariableType *lvalue= &var->var;
   double value;
	operand= &operand[var->first];
	value = *operand;
	for( i= var->first; i<= var->last; i++){
		switch( var->type){
			case CHAR_VAR:
				value= (double) lvalue->c[i];
				lvalue->c[i] = (char)( *operand);
				break;
			case CHAR_PVAR:
				if( lvalue->cp[i]){
					value= (double) *(lvalue->cp[i]);
					*(lvalue->cp[i]) = (char)( *operand);
				}
				break;
			case UCHAR_VAR:
				value= (double) lvalue->uc[i];
				lvalue->uc[i] = (unsigned char)( *operand);
				break;
			case SHORT_VAR:
				value= (double) lvalue->s[i];
				lvalue->s[i] =  d2short( *operand);
				break;
			case SHORT_PVAR:
				if( lvalue->sp[i]){
					value= (double) *(lvalue->sp[i]);
					*(lvalue->sp[i]) =  d2short( *operand);
				}
				break;
			case USHORT_VAR:
				value= (double) lvalue->us[i];
				lvalue->us[i] = (unsigned short)d2short( *operand);
				break;
			case INT_VAR:
				value= (double) lvalue->i[i];
				lvalue->i[i] = d2int( *operand);
				break;
			case INT_PVAR:
				if( lvalue->ip[i]){
					value= (double) *(lvalue->ip[i]);
					*(lvalue->ip[i]) = d2int( *operand);
				}
				break;
			case UINT_VAR:
				value= (double) lvalue->ui[i];
				lvalue->ui[i] = (unsigned int)d2int( *operand);
				break;
			case LONG_VAR:
				value= (double) lvalue->l[i];
				lvalue->l[i] = d2long( *operand);
				break;
			case LONG_PVAR:
				if( lvalue->lp[i]){
					value= (double) *(lvalue->lp[i]);
					*(lvalue->lp[i]) = d2long( *operand);
				}
				break;
			case ULONG_VAR:
			case HEX_VAR:
				value= (double) lvalue->ul[i];
				lvalue->ul[i] = (unsigned long)d2long( *operand);
				break;
			case FLOAT_VAR:
				value= (double) lvalue->f[i];
				lvalue->f[i] = (float) *operand;
				break;
			case FLOAT_PVAR:
				if( lvalue->fp[i]){
					value= (double) *lvalue->fp[i];
					(*lvalue->fp[i]) = (float) *operand;
				}
				break;
			case DOUBLE_VAR:
				value= lvalue->d[i];
				lvalue->d[i] = *operand;
				break;
			case DOUBLE_PVAR:
				if( lvalue->dp[i]){
					value= *(lvalue->dp[i]);
					*(lvalue->dp[i]) = *operand;
				}
				break;
		}
		if( *operand!= value ){
			(*count)++;
			SET_CHANGED_FIELD( var, i, VAR_CHANGED);
		}
		else
			SET_CHANGED_FIELD( var, i, VAR_REASSIGN);
		if( i+1< operands)
			operand++;
	}
	return( (long) i );
}

extern int reset_ascanf_index_value, reset_ascanf_self_value;
extern double ascanf_self_value, ascanf_current_value, ascanf_index_value;

long set_var2( var, p, operand, operands, count, errfp)
Variable_t *var;
char *p;
double *operand;
int operands, *count;
FILE *errfp;
{  int i, operno= var->first;
   VariableType *lvalue= &var->var;
   double value;
   int raiv= reset_ascanf_index_value,
		rasv= reset_ascanf_self_value;
	for( i= var->first; i<= var->last; i++){
		reset_ascanf_index_value= False;
		reset_ascanf_self_value= False;
		ascanf_self_value= operand[i];
		ascanf_current_value= operand[i];
		if( var->type!= CHAR_VAR && var->type!= CHAR_PVAR && var->type!= UCHAR_VAR ){
			fascanf( &operands, p, &operand[var->first], NULL );
		}
		else{
		  char *c= p;
		  double *d= &operand[var->first];
		  int max_op= operands;
			operands= 0;
			while( *c && operands< max_op ){
				*d++= (double) *c++;
				operands+= 1;
			}
		}
		if( !operands){
			if( errno){
				fprintf( errfp, "vars::set_var2(%s,|,\"%s\",%d): %s\n",
					var->name, p, *count, serror()
				);
			}
			return(EOF);
		}
		value = operand[operno];
		switch( var->type){
			case CHAR_VAR:
				value= (double) lvalue->c[i];
				lvalue->c[i] = (char)( operand[operno]);
				break;
			case CHAR_PVAR:
				if( lvalue->cp[i]){
					value= (double) *(lvalue->cp[i]);
					*(lvalue->cp[i]) = (char)( operand[operno]);
				}
				break;
			case UCHAR_VAR:
				value= (double) lvalue->uc[i];
				lvalue->uc[i] = (unsigned char)( operand[operno]);
				break;
			case SHORT_VAR:
				value= (double) lvalue->s[i];
				lvalue->s[i] =  d2short( operand[operno]);
				break;
			case SHORT_PVAR:
				if( lvalue->sp[i]){
					value= (double) *(lvalue->sp[i]);
					*(lvalue->sp[i]) =  d2short( operand[operno]);
				}
				break;
			case USHORT_VAR:
				value= (double) lvalue->us[i];
				lvalue->us[i] = (unsigned short)d2short( operand[operno]);
				break;
			case INT_VAR:
				value= (double) lvalue->i[i];
				lvalue->i[i] = d2int( operand[operno]);
				break;
			case INT_PVAR:
				if( lvalue->ip[i]){
					value= (double) *(lvalue->ip[i]);
					*(lvalue->ip[i]) = d2int( operand[operno]);
				}
				break;
			case UINT_VAR:
				value= (double) lvalue->ui[i];
				lvalue->ui[i] = (unsigned int)d2int( operand[operno]);
				break;
			case LONG_VAR:
				value= (double) lvalue->l[i];
				lvalue->l[i] = d2long( operand[operno]);
				break;
			case LONG_PVAR:
				if( lvalue->lp[i]){
					value= (double) *(lvalue->lp[i]);
					*(lvalue->lp[i]) = d2long( operand[operno]);
				}
				break;
			case ULONG_VAR:
			case HEX_VAR:
				value= (double) lvalue->ul[i];
				lvalue->ul[i] = (unsigned long)d2long( operand[operno]);
				break;
			case FLOAT_VAR:
				value= (double) lvalue->f[i];
				lvalue->f[i] = (float) operand[operno];
				break;
			case FLOAT_PVAR:
				if( lvalue->fp[i]){
					value= (double) *(lvalue->fp[i]);
					*(lvalue->fp[i]) = (float) operand[operno];
				}
				break;
			case DOUBLE_VAR:
				value= lvalue->d[i];
				lvalue->d[i] = operand[operno];
				break;
			case DOUBLE_PVAR:
				if( lvalue->dp[i]){
					value= *(lvalue->dp[i]);
					*(lvalue->dp[i]) = operand[operno];
				}
				break;
		}
		if( operand[operno]!= value ){
			(*count)++;
			SET_CHANGED_FIELD( var, i, VAR_CHANGED);
		}
		else
			SET_CHANGED_FIELD( var, i, VAR_REASSIGN);
		if( i+1< operands)
			operno++;
	}
	reset_ascanf_index_value= raiv;
	reset_ascanf_self_value= rasv;
	return( (long) i );
}

long parse_operator( var, Operator, p, count, errfp)
Variable_t *var;
int Operator;
char *p;
int *count;
FILE *errfp;
{ 
#ifdef __GNUC__
   int max_operands= (var)?var->maxcount : 1;
   double operand[max_operands], *_pob_ = parse_operator_buffer;
   int operand_size= sizeof(operand);
#else
   static double *operand= NULL;
   static int max_operands= 0, operand_size= 0;
#endif
   int help= 0, i, operands= 0, _eno= errno;
   long K = -1;
   int Cws;
   char *laa;

	CX_MAKE_WRITABLE(p,laa);
	CX_DEFER_WRITABLE(Cws);

	if( var && var->maxcount ){
		operands= var->last-var->first+ 1;
		*count= 0;
		errno= 0;
		if( var->type== COMMAND_VAR || var->type== VARIABLE_VAR){
			errno= EVARILLSYNTAX;
			CX_END_WRITABLE(Cws);
			return(EOF);
		}
#ifndef __GNUC__
		if( !operand || operands> max_operands){
			Dispose_Disposable( operand);
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
			Remove_VariableSymbol( operand);
#endif
			if( calloc_error( operand, double, operands) ){
				errno= EALLOCSCRATCH;
				CX_END_WRITABLE(Cws);
				return(EOF);
			}
			Enter_Disposable( operand, calloc_size);
			operand_size= (int) calloc_size;
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
			Add_VariableSymbol( PARSE_OPERATOR_BUFFER, operand, operands, _double, 0);
#endif
			max_operands= operands;
		}
		else
#else
		parse_operator_buffer= operand;
#endif
		{
			memset( operand, 0, operand_size);
		}
		for( i= 0; i< var->maxcount; i++){
			operand[i]= var_Value( var, i);
		}
		if( Operator!= '|' ){
		  int raiv= reset_ascanf_index_value;
			reset_ascanf_index_value= False;
			ascanf_index_value= var->first;
			if( var->type!= CHAR_VAR && var->type!= CHAR_PVAR && var->type!= UCHAR_VAR ){
				fascanf( &operands, p, &operand[var->first], NULL );
			}
			else{
			  char *c= p;
			  double *d= &operand[var->first];
			  int max_op= operands;
				operands= 0;
				while( *c && operands< max_op ){
					*d++= (double) *c++;
					operands+= 1;
				}
			}
			if( !operands){
				fprintf( errfp, "vars::parse_operator(%s,%c,\"%s\",%d): no operands found %s\n",
					var->name, Operator, p, *count, (errno)? serror() : ""
				);
				CX_END_WRITABLE(Cws);
				return(EOF);
			}
			reset_ascanf_index_value= raiv;
		}
#ifdef __GNUC__
		parse_operator_buffer = _pob_;
#endif
	}
	else{
		if( !strcmp( p, "help" ) ){
			help= 1;
		}
		else{
			fprintf( errfp, "vars::parse_operator(NULL,%c,\"%s\",%d): NULL variable\n",
				Operator, p, *count
			);
		}
	}
	errno= 0;
	switch( Operator){
		case '^':
			if( help){
				CX_END_WRITABLE(Cws);
				return( (long) "calculate power : var^arg" );
			}
			else
				K= power_var( var, operand, operands, count);
			break;
		case '*':
			if( help ){
				CX_END_WRITABLE(Cws);
				return( (long) "calculate product var * arg" );
			}
			else
				K= multiply_var( var, operand, operands, count);
			break;
		case '/':
			if( help ){
				CX_END_WRITABLE(Cws);
				return( (long) "calculate fraction var / arg" );
			}
			else
				K= divide_var( var, operand, operands, count);
			break;
		case '+':
			if( help ){
				CX_END_WRITABLE(Cws);
				return( (long) "calculate sum var + arg" );
			}
			else
				K= add_var( var, operand, operands, count);
			break;
		case '-':
			if( help ){
				CX_END_WRITABLE(Cws);
				return( (long) "calculate difference var - arg" );
			}
			else
				K= subtract_var( var, operand, operands, count);
			break;
		case ':':
			if( help ){
				CX_END_WRITABLE(Cws);
				return( (long) "set var array to args (evaluated once)" );
			}
			else
				K= set_var( var, operand, operands, count);
			break;
		case '|':
			if( help ){
				CX_END_WRITABLE(Cws);
				return( (long) "set var array to args (evaluated for each var in the array)" );
			}
			else
				K= set_var2( var, p, operand, operands, count, errfp);
			break;
		default:
			if( help ){
				CX_END_WRITABLE(Cws);
				return( (long) "unknown Operator" );
			}
			break;
	}
	if( errno){
		fprintf( errfp, "vars::parse_operator(%s,%c,\"%s\",%d): %s\n",
			var->name, Operator, p, *count, serror()
		);
	}
	errno= _eno;
	CX_END_WRITABLE(Cws);
	return( K);
}

int print_parse_errors= 0, print_timing= 0;
#ifndef VARS_STANDALONE
typedef struct vars_Time_Struct{
	int level;
	Time_Struct timer;
} vars_Time_Struct;

vars_Time_Struct _varsV_timer, *varsV_timer= &_varsV_timer;
#endif

int remove_leadspace_command= False;

#ifdef _MSC_VER
static int vars_do_include_file(struct Variable_t*, long , long, long);
#endif

/* 20020122: NB: no checking on errfp! */
static int do_command_var( var, vars, n, p, J, errfp)
Variable_t *var, *vars;
long n, *J;
char *p;
FILE **errfp;
{ int depth;
  char trace_line[128], *caller= "do_command_var()";
  int Cws;
  char *laa;

	CX_MAKE_WRITABLE(p,laa);
	CX_DEFER_WRITABLE(Cws);

	check_vars_caller= caller;
	if( !check_vars_reset( var, -1L, *errfp) && var->type== COMMAND_VAR){
		  ALLOCA(args, char, VARSEDIT_LENGTH+2,xlen);
		  char *a;
		  int len;
		  double d_arglist[85];
		  char c_argbuf[1024];
			strcpy( trace_line, caller);
			strncat( trace_line, var->name, 127- strlen(trace_line) );
			PUSH_TRACE( trace_line, depth);
			  /* parse the remainder of this line to pass
			   * to the command function as arguments.
			   */
			a= p;
			if( *a){
				len= strlen(a)+ 1;
				  /* if the first character of the argument
				   \ is a ':' then parse the argumentstring
				   \ through cook_string() AND fascanf() AND
				   \ print_doublevar() (through sprint_array).
				   \ This enables ascanf expressions as arguments
				   \ to COMMAND_VARs.
				   */
				if( a[0]== ':' ){
				  int n= sizeof(d_arglist)/sizeof(double);
					cook_string( args, &a[1], &len, NULL );
					memset( d_arglist, 0, sizeof(d_arglist) );
					fascanf( &n, args, d_arglist, NULL );
					sprint_array( c_argbuf, sizeof(c_argbuf), DOUBLE_VAR, n, (pointer) d_arglist);
					a= c_argbuf;
				}
				else{
					cook_string( args, a, &len, NULL );
					a= args;
				}
				if( strcmp(p,a) && print_parse_errors ){
					fprintf( *errfp, "# arg \"%s\" -> \"%s\"\n",
						p, a
					);
					Flush_File(*errfp);
				}
			}
			else{
				args[0]= '\0';
				a= args;
			}
			var->args= a;
			  /* 950915: don't remove leading space from the argument buffer, unless remove_leadspace_command.	*/
			while( *var->args && (index( "?]=", *var->args) || 
					(remove_leadspace_command && isspace((unsigned char)*var->args)	) )
			){
				var->args++;
			}
			if( !*var->args)
				var->args= NULL;

			var->count= 0;
			if( var->var.command && var->var.command!= var->change_handler ){
			  /* first call var.command if it is not NULL	*/
				if( print_parse_errors ){
					fprintf( *errfp, "#-> command %s(\"%s\",%d,%ld,0L)\"%s\"\n",
						address_symbol(var->var.command), vars_Find_Symbol(vars),
						n, (long) var->id, (var->args)? var->args : ""
					);
					Flush_File(*errfp);
				}
#ifdef _MSC_VER
				  /* RJVB 20030825: workaround a weirdness that occurs under MVC++ 6, at least under Windows 95.
				   \ Possibly related to a memory overwrite or something else otherwise benign enough not to
				   \ cause problems when vars_do_include_file() is called directly.
				   */
				if( var->var.command== vars_do_include_file ){
					var->count= (unsigned short) vars_do_include_file( 
							(Variable_t*) vars, (long) n, (n==0)? 0L : (long) (var->id), 0L
					);
				}
				else
#endif
				var->count= (unsigned short) (*var->var.command)(
					(Variable_t*) vars, (long) n, (n==0)? 0L : (long) var->id, 0L
				);
#ifndef VARS_STANDALONE
				Elapsed_Since( &varsV_timer->timer );
#endif
				if( print_timing ){
					fprintf( *errfp, "#T# %s parsing + command %s(\"%s\",%d,%ld,0L)\"%s\": %g (%g sys) seconds (%d)\n",
						var->name,
						address_symbol(var->var.command), vars_Find_Symbol(vars),
						n, (long) var->id, (var->args)? var->args : "",
#ifndef VARS_STANDALONE
						varsV_timer->timer.Tot_Time, varsV_timer->timer.Time,
						varsV_timer->level
#else
						-1, -1, -1
#endif
					);
					Flush_File(*errfp);
				}
			}
			  /* Now call the change_handler	*/
			if( var->change_handler){
				if( print_parse_errors ){
					fprintf( *errfp, "#-> change_handler %s(\"%s\",%d,%ld,0L)\"%s\"\n",
						address_symbol(var->change_handler), vars_Find_Symbol(vars),
						n, (long) var->id, (var->args)? var->args : ""
					);
					Flush_File(*errfp);
				}
				var->count+= (unsigned short) (*var->change_handler)(
					vars, n, (n==0)? 0L : (long) var->id, var->count
				);
#ifndef VARS_STANDALONE
				Elapsed_Since( &varsV_timer->timer );
#endif
				if( print_timing ){
					fprintf( *errfp, "#T# %s->change_handler %s(\"%s\",%d,%ld,0L)\"%s\": %g (%g sys) seconds (%d)\n",
						var->name,
						address_symbol(var->change_handler), vars_Find_Symbol(vars),
						n, (long) var->id, (var->args)? var->args : "",
#ifndef VARS_STANDALONE
						varsV_timer->timer.Tot_Time, varsV_timer->timer.Time,
						varsV_timer->level
#else
						-1, -1, -1
#endif
					);
					Flush_File(*errfp);
				}
			}
			if( var_label_change ){
				var->change_label= var_change_label;
			}
			var->maxcount++;
			(*J)++;
			var->args= NULL;
		Flush_File( *errfp);
		GCA();
		CX_END_WRITABLE(Cws);
		POP_TRACE(depth); return( *J);
	}
	else{
		CX_END_WRITABLE(Cws);
		return(0);
	}
}

Variable_t *Last_ReferenceVariable= NULL,
	*Last_ParsedVariable= NULL, *Last_ParsedVVariable= NULL,
	*Last_ParsedIVariable= NULL, *Last_ParsedIVVariable= NULL,
	*Last_ErrorVariable= NULL;
char Processed_Buffer[64];

char Save_VarsHistFileName[256];

Save_VarsHistLine(char *line )
{  FILE *fp;
   static double count= 0;
   int close_fp= False;
	if( strlen( Save_VarsHistFileName )){
		if( !strcmp( Save_VarsHistFileName, "stdout") ){
			fp= stdout;
		}
		else if( !strcmp( Save_VarsHistFileName, "stderr") ){
			fp= stderr;
		}
		else{
			fp= fopen( Save_VarsHistFileName, "a+");
			close_fp= True;
		}
		if( fp ){
			calendar( fp, d2str( count++, "# #%g\t", NULL) );
			fputs( line, fp);
			if( !rindex( line, '\n' ) ){
				fputc( '\n', fp);
			}
			if( close_fp ){
				fclose( fp );
			}
		}
		else{
			fprintf( vars_errfile, "Save_VarsHistLine(): can't open \"%s\" (%s)\n",
				Save_VarsHistFileName, serror()
			);
		}
	}
}

_vcc()
{
	if( vars_compress_controlchars){
		fputs( "compressing control characters (can't be read back in)\n", vars_errfile);
	}
	else{
		fputs( "printing all characters (input can be read back in)\n", vars_errfile);
	}
}

_vps()
{
	if( vars_print_as_string ){
		fputs( "printing as a string (stops at first 0)\n", vars_errfile);
	}
	else if( vars_compress_controlchars){
		fputs( "printing all characters (with compress of control charactars)\n", vars_errfile);
	}
	else{
		fputs( "printing all characters (without compress of control charactars)\n", vars_errfile);
	}
}

_perror()
{
	fprintf( vars_errfile, "\t# %s\n", serror());
}

_suspend_ourselves()
{ int ret;
  char stopped[64];
	strncpy( stopped, CX_Time(), 63);
	fprintf( vars_errfile, "Suspending at %s\n", stopped);
#ifndef WIN32
	ret= raise( SIGSTOP);
#else
	ret= 0;
#endif
	if( ret){
		fprintf( vars_errfile, "Error sending SIGSTOP: %s", serror() );
		return( 0);
	}
	else{
		fprintf( vars_errfile, "Continuing from suspend (%s) at %s\n",
			stopped, CX_Time()
		);
		return( 1);
	}
}

int nice_incr= 0;
_nicer( v, n, i, C)
Variable_t *v;
long n, i, C;
{ int x;
	errno= 0;
#ifndef WIN32
	x= nice( nice_incr);
	if( x== -1 && errno){
		fprintf( vars_errfile, "nicer error: %s\n", serror() );
		x= nice(0);
	}
#else
	x= -20;
#endif
	sprintf( v[i].description, "nice level is %d", x+20);
	fprintf( vars_errfile, "%s\n", v[i].description );
	return(0);
}

int _show_ref_vars(), _show_ascanf_functions(), _print_trace_stack();

char PagerName[128]= "$VARSPAGER";
char StreamName[128]= "nl | tee -a /dev/tty | wc";
char Stream2Name[128]= "nl | tee -a /dev/tty | wc";
char SafePager[128]= "$VARSPAGER";

int VarsExitLines[5]= {0, 64, 0, 0, 0};
char **VarsExitLine;

int alloc_exit_lines( v, n, I, C)
Variable_t *v;
long n, I, C;
{ Variable_t *V= &v[I];
  int i;
#ifdef MCH_AMIGA
	unsigned long flags= (unsigned long)(MEMF_PUBLIC|MEMF_CLEAR);
#else
	unsigned long flags= 0L;
#endif
	extern struct Remember *CX_key;

	if( VarsExitLine ){
		fprintf( V->Stdout, "Exit buffer already allocated\n");
		VarsExitLines[0]= VarsExitLines[3];
		VarsExitLines[1]= VarsExitLines[4];
		if( VarsExitLines[2] ){
			fprintf( V->Stdout, "%d out of %d lines of length %d used:\n",
				VarsExitLines[2], VarsExitLines[0], VarsExitLines[1]
			);
		}
		for( i= 0; i< VarsExitLines[2]; i++ ){
			fprintf( V->Stdout, "{%s}\n", VarsExitLine[i] );
		}
		Flush_File( V->Stdout);
		return(0);
	}
	if( ( VarsExitLine= CX_AllocRemember( &CX_key,
			(unsigned long) VarsExitLines[0]* sizeof(char*), flags )
		)
	){
		for( i= 0; i< VarsExitLines[0]; i++ ){
			if( !( VarsExitLine[i]= CX_AllocRemember( &CX_key,
					(unsigned long) VarsExitLines[1]* sizeof(char*), flags )
				)
			){
				VarsExitLines[0]= i;
			}
		}
		VarsExitLines[3]= VarsExitLines[0];
		VarsExitLines[4]= VarsExitLines[1];
	}
	else{
		VarsExitLines[0]= 0;
	}
	fprintf( V->Stdout, "Exit buffer of %d lines of length %d\n",
		VarsExitLines[0], VarsExitLines[1]
	);
	Flush_File( V->Stdout);
	return( 1 );
}

DEFUN( do_vars_exit_lines, (int val), int);

int add_exit_line( v, N, i, C)
Variable_t *v;
long N, i, C;
{ Variable_t *V= &v[i];
  int *n= &VarsExitLines[2], len= VarsExitLines[1];
  static char called= 0;
	if( VarsExitLine && *n< VarsExitLines[0] ){
		if( V->args ){
			if( !called ){
				called= 1;
				onexit( do_vars_exit_lines, "do_vars_exit_lines", 0, 1);
			}
			strncpy( VarsExitLine[*n], V->args, len );
			VarsExitLine[*n][len-1]= '\0';
			(*n)+= 1;
		}
		else{
			return( alloc_exit_lines( v, n, i, C) );
		}
	}
	else{
		fprintf( V->Stdout, "Call exit_lines first, or limit of %d buffers reached\n",
			VarsExitLines[0]
		);
	}
	return( *n );
}

int restart_the_program( v, N, i, C)
Variable_t *v;
long N, i, C;
{ Variable_t *V= &v[i];
  int val= 0, ret;
#ifndef VARS_STANDALONE
  FILE *cst= cx_stderr;
	Writelog( (logmsg, "restart of program at %s\n", CX_Time() ), logmsg);
	if( V->args ){
		val= atoi( V->args );
		Appendlog( (logmsg, "argument '%s' ignored\n", V->args ), logmsg);
	}
	Close_LogWindow();
	ret= RestartProgram();
	fprintf( V->Stdout, "restart_the_program(%d): RestartProgram() returns %d",
					val, ret
	);
	cx_stderr= cst;
#else
	fprintf( V->Stderr, "restart_the_program(%d): not (yet) supported", val );
	ret= -1;
#endif
	return(ret);
}

static int _log_symboltable( v, N, i, C)
Variable_t *v;
long N, i, C;
{ Variable_t *V= &v[i];
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
  extern int log_all_Symbols;
  int las= log_all_Symbols;
#ifndef VARS_STANDALONE
  FILE *lp= logfp;
#endif
	log_all_Symbols= (V->args)? 1 : 0;
#ifndef VARS_STANDALONE
	logfp= V->Stdout;
	log_Symbols();
#else
	log_Symbols(V->Stdout);
#endif
	log_all_Symbols= las;
#ifndef VARS_STANDALONE
	logfp= lp;
#endif
#endif
	return(1);
}

static int _do_Find_NamedSymbol( v, n, I, c)
Variable_t *v;
long n, I, c;
{ char *a= v[I].args;
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
  int duplicates= 0, hits= 0;
  SymbolTable * (*lookup)();
  char *name;
	if( !strcmp(v[I].name, "Find_NamedSymbol") ){
		lookup= Get_NamedSymbols;
		name= a;
	}
	else{
		lookup= Get_Symbols;
		if( a && *a ){
		  unsigned long adr;
		  static char Name[32];
			if( !sscanf( a, "%lx", &adr ) ){
				if( !sscanf( a, "0x%lx", &adr ) ){
					a= NULL;
				}
				else{
					a= (char*) adr;
				}
			}
			else{
				a= (char*) adr;
			}
			sprintf( Name, "0x%lx", a );
			name= Name;
		}
	}
	if( a){
	  SymbolTable *sym, *nsym= NULL;
	  Disposable *disp;
	  FILE *fp;
		fp= v[I].Stdout;
		(*lookup)(NULL);
		if( (sym= (*lookup)( a)) ){
		 do{
		  if( sym!= nsym || !nsym ){
			hits+= 1;
			if( nsym ){
				sym= nsym;
			}
			fprintf( fp,
				"%s -> %s[%d]\t(%s) [#%ld.%ld hash=0x%lx]\n", name, address_symbol(sym->adr),
				sym->size, ObjectTypeNames[sym->obtype],
				sym->Index, sym->item, sym->hash
			);
			if( sym->value){
				fputs( "\t\t", fp);
				print_var( fp, (Variable_t*) sym->value, 1);
			}
			if( sym->description ){
				fprintf( fp, "\t\t\"%s\"\n", sym->description );
			}
			Print_SymbolValues( fp, "\t ", sym, "\n");
			if( (disp= Find_Disposable(sym->adr) ) ){
				fprintf( fp, "\t\tEntered as Disposable #%d with size=%lu\n",
					disp->id, disp->size
				);
			}
			fflush( fp );
		  }
		  else{
			duplicates++;
		  }
		 } while( (nsym= (*lookup)( a)) );
		}
		else{
			fprintf( fp, "\"%s\" not in SymbolTable\n", name );
			fflush( fp );
		}
		if( duplicates && lookup== Get_NamedSymbols ){
			fprintf( fp, "%d duplicate Symbols: checking table\n", duplicates );
			fflush( fp );
			Check_SymbolTable( cx_stderr);
		}
		return(hits);
	}
#endif
	return( 0);
}

int add_watch_variable( v, N, n, C)
Variable_t *v;
long N, n, C;
{ Variable_t *V= &v[n];
  Watch_Variable *wv;
  Variable_t *var;
  int i, j= 0;
  long I= 0;
  char *name, *match, *option;
	if( V->args ){
		for( i= 0; i< vars_arg_items; i++ ){
			name= V->args;
			match= V->args;
			option= V->args;
			while( (var= find_next_named_var( V->args, vars_arglist[i].vars, vars_arglist[i].n,
								&I, &name, &match, &option, vars_errfile
							)
				)
			){
				if( !calloc_error( wv, Watch_Variable, 1) ){
					wv->type= VARIABLE_VAR;
					wv->var= var;
					wv->tail= Watch_Variables;
					Watch_Variables= wv;
					fprintf( V->Stdout, "Added Watch Variable: ");
					print_var( V->Stdout, var, 0);
					j+= 1;
				}
				else{
					fprintf( vars_errfile, "varsV::add_watch_variable(\"%s\"): can't get memory (%s)\n",
						V->args, serror()
					);
					return(0);
				}
			}
		}
	}
	return(j);
}

int add_watch_command( Variable_t *v, long N, long n, long C)
/* Variable_t *v;	*/
/* long N, n, C;	*/
{ Variable_t *V= &v[n];
  Watch_Variable *wv;
  int j= 0;
	if( V->args ){
		if( !calloc_error( wv, Watch_Variable, 1) ){
			wv->type= CHAR_VAR;
			wv->buf= strdup(V->args);
			wv->tail= Watch_Variables;
			Watch_Variables= wv;
			fprintf( V->Stdout, "Added Watch Command: \"%s\"\n", wv->buf );
			Flush_File( V->Stdout);
			j+= 1;
		}
		else{
			fprintf( vars_errfile, "varsV::add_watch_variable(\"%s\"): can't get memory (%s)\n",
				V->args, serror()
			);
			return(0);
		}
	}
	return(j);
}

int delete_watch_variable( v, N, i, C)
Variable_t *v;
long N, i, C;
{ Variable_t *V= &v[i];
  Watch_Variable *wv= Watch_Variables, *pwv= wv;
  int hit;
  int j= 0;
	if( V->args ){
		while( wv ){
			hit= False;
			if( wv->type== VARIABLE_VAR ){
				hit= streq( wv->var->name, V->args);
			}
			else if( wv->type== CHAR_VAR ){
				if( (hit= streq( wv->buf, V->args)) ){
					lib_free( wv->buf );
				}
			}
			if( !hit ){
				pwv= wv;
				wv= wv->tail;
			}
			else{
				if( wv== Watch_Variables ){
					if( wv->tail== NULL ){
						Watch_Variables= NULL;
						free( wv );
						wv= NULL;
					}
					else{
						Watch_Variables= wv->tail;
						free( wv );
						pwv= wv= Watch_Variables;
					}
				}
				else{
					pwv->tail= wv->tail;
					free( wv );
					wv= pwv->tail;
				}
				j+= 1;
			}
		}
	}
	return( j );
}

int show_watch_variables( v, N, i, C)
Variable_t *v;
long N, i, C;
{ Variable_t *V= &v[i];
  Watch_Variable *wv= Watch_Variables;
  int j= 0;
	while( wv ){
		if( wv->type== VARIABLE_VAR ){
			print_var( V->Stdout, wv->var, 0);
		}
		else if( wv->type== CHAR_VAR ){
			fprintf( V->Stdout, "\"%s\"\n", wv->buf );
		}
		wv= wv->tail;
		j+= 1;
	}
	if( *WWatch ){
		fprintf( V->Stdout, "Watching is on\n");
	}
	else{
		fprintf( V->Stdout, "Watching is off\n");
	}
	Flush_File( V->Stdout );
	return( j );
}

extern unsigned short random_seed[5];
static int _seed48( Variable_t *v, long N, long i, long C)
{
	if( v[i].args && strlen(v[i].args) ){
		seed48( random_seed);
		srand( (unsigned int) random_seed[3]);
		for( i= 0; i< random_seed[4]; i++){
			drand();
			rand();
		}
	}
	else{
		fprintf( v[i].Stdout, "%s: random generators polled %d times\n",
			v[i].name, randomise()
		);
		print_var( v[i].Stdout, &v[i], 0);
		Flush_File( v[i].Stdout );
	}
	return(0);
}

char StdInput[256], StdOutput[256];

static int IO( Variable_t *v, long N, long i, long C)
{  char *newfile= v[i].args;
   int stdinput= False, stdoutput= False;
	if( newfile ){
		while( isspace((unsigned char)*newfile) ){
			newfile++;
		}
	}
	else{
		return 0;
	}
	if( !strncmp( v[i].name, "stdinput", 8) ){
		stdinput= True;
		strncpy( StdInput, newfile, 255 );
	}
	else if( !strncmp( v[i].name, "stdoutput", 9) ){
		stdoutput= True;
		strncpy( StdOutput, newfile, 255 );
	}
	else if( !strncmp( v[i].name, "terminal", 8) ){
		stdoutput= stdinput= True;
		strncpy( StdInput, newfile, 255 );
		strncpy( StdOutput, newfile, 255 );
	}
	if( stdinput){
		change_stdin( newfile, vars_errfile);
	}
	if( stdoutput){
		change_stdout_stderr( newfile, vars_errfile);
	}
	return(1);
}

static int vars_fflush( Variable_t *v, long N, long i, long C)
{  char *file= v[i].args;
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
   SymbolTable *sym;
	if( file ){
		if( (sym= _Get_Symbol( file,  SYMBOL_LOOKUP, _file, 0)) ){
			Flush_File( (FILE*) sym->adr );
			fprintf( v[i].Stdout, "# Flushed %s\n", sym->symbol );
		}
		else{
			fprintf( v[i].Stdout, "# %s is unknown or is not a file\n", file );
		}
	}
#endif
	return(1);
}

static int VarChanges= 0, no_recurse= False;
static int to_toplevel= False, to_nextlevel= False;

static int Check_Recurse( char *name )
{ int r;
	if( no_recurse && parfilename && name && !strcmp( parfilename, name) ){
		r= 1;
	}
	else{
		r= 0;
	}
	return(r);
}

int vars_include_file( char *name )
{ FILE *vf= vars_file, *fp= NULL;
  Parfile_Context pc= parfile_context, lpc;
  extern int edit_variables_maxlines;
  int changes= 0, dum= -1, recerr= False, evm= edit_variables_maxlines;
  static int _evm= -1;
  int ispipe= False;
  static int level= 0;
  char *prompt;

	while( name && isspace((unsigned char)*name) ){
		name++;
	}
	if( !name || !*name ){
		return(0);
	}

	if( level && (to_toplevel || to_nextlevel) ){
		fprintf( stderr, "include_file(%s|%d): skipping to %s\n",
				name, level, (to_toplevel)? "toplevel" : "nextlevel"
		);
		edit_variables_interim= "/";
		to_nextlevel= False;
		return( 0 );
	}
	else{
		edit_variables_interim= NULL;
		to_toplevel= False;
	}
	if( name[0]== '|' ){
		if( !(recerr= Check_Recurse( name )) ){
			fp= Open_Pipe_From( &name[1], NULL );
			ispipe= 1;
			prompt= "<| ";
		}
	}
	else if( sscanf( name, "lines=%d", &dum )== 1 && dum> 0 ){
		_evm= dum;
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
		if( !(recerr= Check_Recurse( command )) ){
			fp= Open_Pipe_From( &command[1], NULL );
			name= strdup(command);
			ispipe= 2;
			prompt= "<| ";
		}
	}
	else if( !(recerr= Check_Recurse(name)) ){
		fp= fopen( name, "r");
		prompt= "<< ";
	}
	if( fp ){
	  int depth;
	  FILE *rfp;
#ifdef VARS_STANDALONE
	  ALLOCA( logmsg, char, strlen(name)*2+ 64, lmlen);
#endif
		level++;
		vars_file= fp;
		parfile_context= *dup_Parfile_Context( &lpc, fp, name);
		parposition= 0;
		parlineposition= 0;
		edit_variables_maxlines= _evm;
		_evm= -1;
		sprintf( logmsg, "include_file(%s|%d|l=%d)", name, level, edit_variables_maxlines );
		PUSH_TRACE( logmsg, depth);
		if( (changes= edit_vars_arglist( prompt )) ){
#ifndef VARS_STANDALONE
			sprintf( logmsg,
				"include_file(%s|%d|l=%ld); (%d) Variables changed:\n",
				name, level, parlineposition, changes
			);
			fputs( logmsg, stderr );
#else
			fprintf( stderr,
				"include_file(%s|%d|l=%ld); (%d) Variables changed:\n",
				name, level, parlineposition, changes
			);
#endif
		}
		else{
			fprintf( stderr,
				"include_file(%s|%d|l=%ld); nothing changed... ",
				name, level, parlineposition
			);
			if( errno){
				fprintf( stderr, "(%s)", serror() );
			}
			fputc( '\n', stderr);
			fflush( stderr);
		}
		VarChanges+= changes;
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
		edit_variables_maxlines= evm;
		POP_TRACE(depth);
	}
	else if( dum<= 0 ){
		fprintf( stderr, "include_file(%s|%d): %s\n",
				name, level, ((recerr)? " recursion (self-inclusion) detected" : serror())
		);
	}
	return(changes);
}

static int vars_do_include_file( Variable_t *v, long n, long id, long C)
{ char *name= v[id].args;
  static int level= 0;
  int ret;

	if( name ){
		level++;
		ret= vars_include_file(name);
		level--;
	}
	else{
		fprintf( v[id].Stdout, "usage: \"%s\"\n", v[id].description );
		ret = 0;
	}
	return( ret );
}

static int vars_ch_parlinelength( v, n, id, C)
Variable_t *v;
long n, id, C;
{ int len= v[id].var.i[0];
  static int level= 0;
  int ret= 0;

	if( v[id].var.i== &parlinelength ){
		if( len> 0 && !level ){
			level++;
			Free_Parbuffer();
			parlinelength= len;
			ret= Init_Parbuffer();
			level--;
		}
	}
	else if( v[id].var.i== &VarsEdit_Length ){
		VarsEdit_Length= ABS( VarsEdit_Length);
		VEL_changed= 1;
	}
	return( ret );
}

unsigned long vars_update_change_label()
{ char *c= var_change_label_string;
  int ss= (sizeof(unsigned long)- 1)* 8;
	var_change_label= 0;
	while( *c ){
		var_change_label|= (*c) << ss;
		c++;
		ss-= 8;
	}
	return( var_change_label );
}

char *vars_update_change_label_string( unsigned long change_label, char label_string[5] )
{ static char buf[5];
	if( !label_string ){
		label_string= buf;
	}
	label_string[0]= (char) ((change_label & 0xFF000000) >> ((sizeof(unsigned long)- 1)* 8));
	label_string[1]= (char) ((change_label & 0x00FF0000) >> ((sizeof(unsigned long)- 2)* 8));
	label_string[2]= (char) ((change_label & 0x0000FF00) >> ((sizeof(unsigned long)- 3)* 8));
	label_string[3]= (char) ((change_label & 0x000000FF));
	label_string[4]= '\0';
	return( label_string );
}

int vars_clear_change_labels( Variable_t *v, long n, unsigned long mask )
{ long i;
  int N= 0;
	for( i= 0; i< n; i++ ){
		if( !mask || v[i].change_label== mask ){
			v[i].change_label= 0;
			N+= 1;
		}
	}
	return(N);
}

static int vars_label_changes( Variable_t *v, long N, long i, long C)
{
	if( v[i].var.c== var_change_label_string ){
		vars_update_change_label();
	}
	else if( v[i].var.ul== &var_change_label ){
		vars_update_change_label_string( var_change_label, var_change_label_string );
	}
	return( 1 );
}

static Variable w_v_Pars[]={
	{0, COMMAND_VAR, 0, 0, NULL, "add_var", add_watch_variable, 0,
		"add variables with name matching argument to watch list"
	},
	{1, COMMAND_VAR, 0, 0, NULL, "add_cmd", add_watch_command, 0,
		"add argument as Vars command to watch list"
	},
	{2, COMMAND_VAR, 0, 0, NULL, "delete", delete_watch_variable, 0,
		"delete Watch variables matching argument from watch list"
	},
	{3, COMMAND_VAR, 0, 0, NULL, "show", show_watch_variables, 0 },
	{4, INT_PVAR, 1, 1, (pointer) &WWatch, "active", NULL, 1 },
};
#define WVP	sizeof(w_v_Pars)/sizeof(Variable)

static Variable c_v_Pars[]={
	{0, INT_VAR, 1, 1, (pointer) &c_v_on_NULLptr, "NULLptr", NULL, 0,
		"don't accept NULL Variables [1]"
	}
	,{1, INT_VAR, 1, 1, (pointer) &c_v_on_PARENT_notify, "PARENT_notify", NULL, 0,
		"notify if Parent field of a VarVariable was corrected [1]"
	}
	,{2, INT_VAR, 1, 1, (pointer) &c_v_correct_ID, "correct_ID", NULL, 0,
		"correct the IDs of Variable list items [0]"
	}
	,{3, INT_VAR, 1, 1, (pointer) &c_v_on_NULLelement, "NULLelement", NULL, 0,
		"List the NULL pointers found in a pointer Variable [0]"
	}
};
#define CVP	sizeof(c_v_Pars)/sizeof(Variable)

#ifndef VARS_STANDALONE
extern int resize();
#else
	int resize()
	{
		return(0);
	}
#endif
extern int ascanf_verbose;
extern double ascanf_delta_t;
int vars_check_internals= True;
int var_label_change= False;
unsigned long var_change_label= 'VARS';
char var_change_label_string[]= "VARS";

static Variable vars_internals[]={
	{-1, CHAR_VAR, 0, 16, (pointer) double_print_format, "double printf", NULL, 0,
	 	"printf format string for doubles"
	}
	,{-1, DOUBLE_VAR, 1, 1, (pointer) &stdv_zero_thres, "stdv zero", NULL, 0,
		"zero threshold for standard deviation of var_Mean"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &vars_compress_controlchars, "compress", _vcc, 0,
		"controls the runlength compression of controlcharacters"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &vars_print_as_string, "string print", _vps, 0,
		"controls the printing of CHAR_VAR's"
	}
	,{-1, CHAR_VAR, 0, 128, (pointer) PagerName, "pager", NULL, 1,
	 	"command to pipe help (?) and list output to"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &nice_incr, "nicer", _nicer, 0,
		"nice level is at program startup value"
	}
	,{-1, COMMAND_VAR, 1, 1, (pointer) NULL, "errno", _perror, 0,
		"system error code"
	}
	,{-1, COMMAND_VAR, 0, 0, NULL, "ref_vars", _show_ref_vars, 0,
		"show the Variables in the ReferenceVariable Selection list"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &list_level, "list_level", NULL, 0,
		"list information level"
	}
	,{-1, COMMAND_VAR, 0, 0, NULL, "ascanf functions", _show_ascanf_functions, 0,
		"show the functions recognised by the ascanf routines"
	}
	,{-1, COMMAND_VAR, 0, 0, NULL, "CX trace", _print_trace_stack, 0,
		"print the CX stack trace"
	}
	,{-1, COMMAND_VAR, 0, 0, NULL, "suspend program", _suspend_ourselves, 0,
		"suspend the running program (needs a SIGCONT afterwards)"
	}
	,{-1, CHAR_VAR, 0, 128, (pointer) StreamName, "Stream", NULL, 1,
	 	"Used by `Stream` option"
	}
	,{-1, CHAR_VAR, 0, 32, (pointer) ItemSeparator, "ItemSeparator", NULL, 1,
	 	"string that separates items of a Variable Variable"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &print_parse_errors, "print parse errors", NULL, 1,
		"print all parse errors and be verbose in parsing"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &vars_print_varvarcommands, "print varvarcommands", NULL, 0,
		"print Variable_Var command items"
	}
	,{-1, CHAR_VAR, 2, 2, (pointer) VarsSeparator, "VarsSeparator", NULL, 0}
	,{-1, INT_VAR, 1, 1, (pointer) &print_timing, "print timing", NULL, 1,
		"print timing"
	}
	,{-1, CHAR_VAR, 1, 256, (pointer) Save_VarsHistFileName, "HistFileName", NULL, 0}
	,{-1, CHAR_VAR, 0, 128, (pointer) Stream2Name, "Stream2", NULL, 1,
	 	"Used by `Stream2` option"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &vars_print_all_curly, "print curly", NULL, 0,
		"print all (i.e. also numerical) Variables as 'name{1, 2, 3}' or else as 'name= 1, 2, 3'"
	}
	,{-1, INT_VAR, 0, 1, (pointer) &try_reverse_double, "try_reverse_double", NULL, 0,
	 	"print doubles as 1/%x if that is shorter than %x (see double printf)"
	}
	,{-1, INT_VAR, 2, 2, (pointer) VarsExitLines, "exit_lines", alloc_exit_lines, 1,
		"allocate a r*c buffer for exit commands. Can be called only once."
	}
	,{-1, COMMAND_VAR, 0, 0, add_exit_line, "add_exit_line", NULL, 0}
	,{-1, VARIABLE_VAR, CVP, CVP, (pointer) c_v_Pars, "check_vars", NULL, 0 }
	,{-1, COMMAND_VAR, 0, 0, restart_the_program, "restart the program", NULL, 0}
	,{-1, COMMAND_VAR, 0, 0, resize, "resize", NULL, 0, "X11 resize function"}
	,{-1, VARIABLE_VAR, WVP, WVP, (pointer) w_v_Pars, "Watch", NULL, 0 }
	,{-1, INT_VAR, 1, 1, (pointer) &execute_command_question, "execute command question", NULL, 1,
		"execute <command>? and <command>??"
	}
	,{-1, COMMAND_VAR, 0, 0, _log_symboltable, "SymbolTable", NULL, 0}
	,{-1, INT_VAR, 1, 1, (pointer) &ascanf_verbose, "ascanf verbose", NULL, 0,
		"verbose ascanf function parsing progress indication"
	}
	,{-1,	COMMAND_VAR, 0, 0, _do_Find_NamedSymbol, "Find_NamedSymbol",	NULL, 1,
		"Look up SymbolTable entries by name", "FNS"
	}
	,{-1, DOUBLE_VAR, 0, ASCANF_MAX_ARGS, (pointer) ascanf_memory, "ascanf memory", NULL, 0,
		"ascanf internal memory buffer for the MEM function"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &remove_leadspace_command, "no leadspaces, command", NULL, 0,
		"Remove leading spaces from argument to a COMMAND_VAR function"
	}
	,{-1,	USHORT_VAR, 5, 5,	random_seed,	"random_seed",	_seed48, 0,
		"seed for the double random generator\n"
		"With empty argument, the generator is randomised with a date-dependent seed\n"
		"elements: #1-3 for seed48() routine, #4 for srand(), #5 nr. of times to poll generators after seeding\n"
	}
	,{-1, DOUBLE_VAR, 1, 1, (pointer) &ascanf_delta_t, "delta-t", NULL, 1,
		"time resolution for ascanf functions using differential equations"
	}
	,{-1, COMMAND_VAR, 0, 0, IO, "stdinput", NULL, 1, StdInput }
	,{-1, COMMAND_VAR, 0, 0, IO, "stdoutput", NULL, 1, StdOutput }
	,{-1, COMMAND_VAR, 0, 0, IO, "terminal", NULL, 1, StdInput }
	,{-1, INT_VAR, 0, 1, (pointer) &Allow_Fractions, "AllowFractionPrinting", NULL, 0,
	 	"Allow to print doubles as %g/%g or %g'%g/%g if that is not longer than %x (see double printf)"
	}
	,{-1, COMMAND_VAR, 0, 0, vars_fflush, "fflush", NULL, 1 }
	,{-1, INT_VAR, 0, 1, (pointer) &vars_check_internals, "Check_Internals", NULL, 0,
	 	"Check the internal Variables for a match when none has been found in the user-passed lists"
	}
	,{-1,	COMMAND_VAR,	0,	0,	(pointer) vars_do_include_file,	"include_file",	NULL,	0,
		"include_file[{lines=<n>}]{filename} or include_file{[lines=<n>}]{|command}\n"
		"files are closed after each full line read to enable changes to be made *after* the current position!\n"
		"Lists scanned are those last passed to edit_variables_list(), parse_varlist_line(), or set_vars_arglist()"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &vars_echo, "echo_level", NULL, 0,
		"level of echoing of (results of) commands"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &parlinelength, "max-line-length", vars_ch_parlinelength, 0,
		"maximum line-length for Read_parfile()"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &VarsEdit_Length, "edit-line-length", vars_ch_parlinelength, 0,
		"maximum line-length for edit_variables()"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &var_label_change, "label-changes", vars_label_changes, 0,
		"label each variable that is accessed from now on with the label specified by 'change-label' or\n"
		" 'change-label-string'.\n"
	}
	,{-1, ULONG_VAR, 1, 1, (pointer) &var_change_label, "change-label", vars_label_changes, 0 }
	,{-1, CHAR_VAR, 5, 5, var_change_label_string, "change-label-string", vars_label_changes, 0,
		"A string representation of a 4-byte mask to be assigned to accessed variables"
	}
	,{-1, INT_VAR, 1, 1, (pointer) &no_recurse, "no-recursive-include", NULL, 0,
		"Protection against recursive (\"self\") inclusion of files\n"
		" Works for one level only!\n"
	}
};

int VARS_INTERNALS= sizeof(vars_internals)/sizeof(Variable);

/* 
#define PagerResult CHANGED_FIELD(&vars_internals[4])
#define StreamResult CHANGED_FIELD(&vars_internals[12])
#define Stream2Result CHANGED_FIELD(&vars_internals[19])
 */
char *PagerResult, *StreamResult, *Stream2Result;

int PagerExit= 0;
static int PagerLevel= 0;

char *_PagerResult()
{
	return( PagerResult);
}

static FILE *PIPE_file= NULL, **PIPE_fileptr= NULL;
static int which_PIPE, PIPE_nr, PIPE_depth;
static Variable_t *PIPE_var= NULL;

#ifndef WIN32
void PIPE_handler(sig)
int sig;
{  FILE *fp= (PIPE_fileptr)? *PIPE_fileptr : PIPE_file;
	fprintf( stderr, "Page: Broken Pipe (%s -> %s) exit=%d\n",
		vars_Find_Symbol(fp), vars_Find_Symbol( stdout),
		Close_Pipe(fp)
	);
	fflush( stderr );
	if( fp){
		  /* assumes this will cause printing routines to
		   * not block:
		   */
#if !defined(linux)
		memset( fp, 0, sizeof(FILE) );
#endif
		if( PIPE_file ){
			PIPE_file= stdout;
		}
		if( PIPE_fileptr ){
			*PIPE_fileptr= stdout;
		}
	}
	if( PIPE_var){
		  /* any further reference to this file will use
		   * stdout instead
		   */
		PIPE_var->Stdout= stdout;
	}
	signal( SIGPIPE, PIPE_handler);
}
#endif

FILE *Open_Stream( int which, FILE *alternative)
{
#ifndef WIN32
	if( ++PagerLevel )
		signal( SIGPIPE, PIPE_handler);
#else
	PagerLevel++;
#endif
	PIPE_nr= 2;
	if( which == 1 ){
		which_PIPE= 1;
		return( Open_Pipe_To( StreamName, alternative) );
	}
	else{
		which_PIPE= 2;
		return( Open_Pipe_To( Stream2Name, alternative) );
	}
}

FILE *Open_Pager( FILE *alternative)
{
	if( PagerExit)
		strcpy( PagerName, SafePager);
	if( isatty(fileno(stdout)) ){
	  FILE *ret;
#ifndef WIN32
		if( ++PagerLevel )
			signal( SIGPIPE, PIPE_handler);
#else
		PagerLevel++;
#endif
		VarsPagerActive= True;
		ret= Open_Pipe_To( PagerName, alternative);
		if( ret== alternative && PagerLevel== 1 ){
			VarsPagerActive= False;
		}
		PIPE_nr= 1;
		return( ret );
	}
	else{
		return( alternative);
	}
}

int Close_Stream( int which, FILE *output)
{  int StreamExit;
	fflush(output);
	StreamExit= Close_Pipe( output);
#ifndef WIN32
	if( ! --PagerLevel )
		signal( SIGPIPE, SIG_DFL);
#else
	PagerLevel--;
#endif
	if( which== 1 ){
		if( StreamResult){
			sprintf( StreamResult, "Last Exit Code: %d", PagerExit );
		}
	}
	else{
		if( Stream2Result){
			sprintf( Stream2Result, "Last Exit Code: %d", PagerExit );
		}
	}
	return( StreamExit);
}

int Close_Pager( FILE *output)
{
	Flush_File(output);
	PagerExit= Close_Pipe( output);
	if( ! --PagerLevel ){
#ifndef WIN32
		signal( SIGPIPE, SIG_DFL);
#endif
		VarsPagerActive= False;
	}
	if( !PagerExit)
		strcpy( SafePager, PagerName);
	if( PagerResult)
		sprintf( PagerResult, "Last Exit Code: %d", PagerExit );
	return( PagerExit);
}

VariableSelection *Reference_Variable= NULL;
int Reference_Variables= 0;

int _show_ref_vars()
{  int I= 0, J= 0;
   FILE *fp;
   Variable_t *var;
   VariableSelection *vs;
	if( Reference_Variable && Reference_Variables){
		fp= Open_Pager( vars_errfile);
		var= find_named_VariableSelection( NULL, Reference_Variable, Reference_Variables, &I, &J, &vs);
		while( var){
			fprintf( fp, "<%03d,%03d> ", vs->Index, vs->subIndex);
			print_var( fp, var, list_level);
			var= find_named_VariableSelection( NULL, Reference_Variable, Reference_Variables, &I, &J, &vs);
		}
		Close_Pager( fp);
		return(I);
	}
	return(0);
}

int _show_ascanf_functions()
{ FILE *fp;
  int I;
	fp= Open_Pager( vars_errfile);
	PIPE_fileptr= &fp;
	I= show_ascanf_functions( fp, "\t");
	Close_Pager( fp);
	PIPE_fileptr= NULL;
	return(I);
}

int _print_trace_stack()
{ FILE *fp;
  int I,depth;
	fp= Open_Pager( vars_errfile);
	PIPE_fileptr= &fp;
	PUSH_TRACE( "_print_trace_stack()", depth);
	I= print_trace_stack( fp, Max_Trace_StackDepth, CX_Time() );
	Close_Pager( fp);
	PIPE_fileptr= NULL;
	POP_TRACE(depth); return(I);
}

/* parse buffer for the occurence of all _Variable names
 * as defined in 'vars'. If a name is succeeded by '?',
 * print the value(s). If succeeded by '=', alter the value(s).
 * Otherwise return an error (0). The 'count' of the altered
 * variables is updated to reflect the number of values changed
 * of this variable. The number of legal actions is returned;
 * <changes> holds the number of actual changes.
 */
#ifdef _PARSE_VARLINE_CLOSES_PIPES
#	define _PARSE_VARLINE_RETURN(x)	POP_TRACE(depth);\
	if( pipe && PIPE_file ){\
		if(pipe==1){\
			Close_Pipe(PIPE_file);\
			PIPE_file= 0;\
		}\
		else if (pipe==2){\
			Close_Stream(which_PIPE,PIPE_file);\
			PIPE_file= 0;\
		}\
	}\
	varsV_timer= old_timer;\
	GCA();\
	CX_END_WRITABLE(Cws);\
	return((x))
#else
#	define _PARSE_VARLINE_RETURN(x)	POP_TRACE(depth);\
	varsV_timer= old_timer;\
	CX_END_WRITABLE(Cws);\
	return((x))
#endif

Variable_t *in_var_var( Variable_t *var_var, Variable_t *var)
{  int n;
   Variable_t *list= (Variable_t*) var_var->var.ptr;
	if( !var_var || !var)
		return( NULL );
	for( n= 0; n<= var_var->last && list!= var; list++, n++ );
	if( list== var){
		if( var->parent!= var_var ){
			var->parent= var_var;
			fprintf( var->Stderr, "\"%s\"->parent != \"%s\" : corrected\n",
				var->name, var_var->name
			);
			Flush_File( var->Stderr);
		}
		return( var);
	}
	else{
		if( var->parent== var_var ){
			var->parent= NULL;
			fprintf( var->Stderr, "\"%s\"->parent == \"%s\" : corrected\n",
				var->name, var_var->name
			);
			Flush_File( var->Stderr);
		}
		return( NULL);
	}
}

int line_invocation= 0;

/* 20020122: rewrite of the file handling outfp/errfp. Since the files referenced by these pointers might be
 \ changed by handlers/methods invoked by ourselves, they need to be handlers in the Mac's sense of the term:
 \ pointers to a FILE* variable. This guarantees (?!) that, as long as the user continues to reference a valid
 \ open file through the filepointer to which he/she passed a pointer to us, output to this file will not
 \ cause any catastrophic events (like flushing a closed file under Linux...).
 */
long _parse_varline( char *buffer, Variable_t *vars, long n, long *changes, int cook_it, FILE **outfp, FILE **errfp)
/* char *buffer;	*/
/* Variable_t *vars;	*/
/* long *changes, n;	*/
/* int cook_it;	*/
/* FILE **outfp, **errfp;	*/
{   char *p, *option, *match, *name, *left_brace= NULL, *right_brace= NULL;
    ALLOCA( ref_val, char, VARSEDIT_LENGTH,xlen);
    static char *_buffer= NULL;
    static int _buflen= 0;
    long I, J= 0, K;
    static Variable_t *Var= NULL;
    Variable_t *var;
    int count, i, depth;
    static char VarVarActive= 0, parse_vars_internal= 0;
    static Variable_t *var_var= NULL;
    char *assignOp, orig_sep, *separator;
#ifndef VARS_STANDALONE
    vars_Time_Struct parse_varline_timer, *old_timer= varsV_timer;
#else
	char varsV_timer = 0, old_timer = 0;
#endif
    DEFMETHOD( find_next_method, (char*, Variable_t*, long,long*, char**,char**,char**,FILE*),Variable_t*)= find_next_named_var;
    int buf_offset= 0;
    int watch= True, remember_var;
    FILE *ierrfp, *ioutfp;
    int Cws;
    char *lbuffer=NULL;

	CX_MAKE_WRITABLE(buffer, lbuffer);
	CX_DEFER_WRITABLE(Cws);
	
	errno= 0;

#ifndef VARS_STANDALONE
	Elapsed_Since( &parse_varline_timer.timer );
#endif

	p= buffer;
	while( isspace((unsigned char)*p) )
		p++;
	if( index( "#;", *p) ){
	  /* a simple initial test to not do anything	*/
		GCA();
		CX_END_WRITABLE(Cws);
		return(-1);
	}

	if( !(vars== Var && Var) ){
		check_vars_caller= "parse_varline()";
		if( check_vars_reset( vars, (n< 0)? -1 : n, (errfp)? *errfp : NULL )){
			GCA();
			CX_END_WRITABLE(Cws);
			return( 0L);
		}
	}
	Var= var= vars;

#ifndef VARS_STANDALONE
	varsV_timer= &parse_varline_timer;
	varsV_timer->level= old_timer->level+ 1;
#endif

	if( !outfp ){
		ioutfp= stdout;
		outfp= &ioutfp;
	}
	if( !*outfp ){
		outfp= &ioutfp;
		*outfp= stdout;
	}
	if( !errfp ){
#ifndef VARS_STANDALONE
		errfp= &vars_errfile;
#else
		errfp= &ierrfp;
		*errfp= NULL;
#endif
	}
	if( !*errfp ){
		errfp= &ierrfp;
		*errfp= vars_errfile;
	}

	if( strlen(buffer)>= _buflen){
		Dispose_Disposable( _buffer);
		_buffer= NULL; _buflen= 0;
		if( !calloc_error( _buffer, char, strlen(buffer)+2) ){
			Enter_Disposable( _buffer, calloc_size);
			_buflen= strlen(buffer)+ 2;
		}
	}
	if( _buffer ){
	  int count= strlen(buffer)+ 1;
		strcpy( _buffer, buffer);
		if( cook_it){
			cook_string( buffer, _buffer, &count, NULL);
			if( strcmp( buffer, _buffer) ){
				fprintf( *errfp, "%s\n", buffer);
			}
		}
	}

	PUSH_TRACE( "_parse_varline()", depth);
	if( !parse_vars_internal){
	  int r;
	  char *internal;
	  int w= Watch, *_ww_ = WWatch;
	  static char called= 0;
		WWatch= &w;
		if( !called ){
		  Variable_t *vn;
		  long I;
			vars_internals[6].var= &errno;
			sort_variables_list_alpha( VARS_INTERNALS, (Variable_t*) vars_internals, 0, NULL, 0);
			check_vars_caller= "parse_varline()";
			if( check_vars_reset( vars_internals, VARS_INTERNALS, *errfp)){
				GCA();
				CX_END_WRITABLE(Cws);
				return(0);
			}
			I= 0;
			if( (vn= find_named_var( "Stream", False, (Variable_t*) vars_internals, VARS_INTERNALS, &I, *errfp)) ){
				StreamResult= CHANGED_FIELD(vn);
			}
			else{
				StreamResult= NULL;
			}
			I= 0;
			if( (vn= find_named_var( "Stream2", False, (Variable_t*) vars_internals, VARS_INTERNALS, &I, *errfp)) ){
				Stream2Result= CHANGED_FIELD(vn);
			}
			else{
				Stream2Result= NULL;
			}
			I= 0;
			if( (vn= find_named_var( "pager", False, (Variable_t*) vars_internals, VARS_INTERNALS, &I, *errfp)) ){
				PagerResult= CHANGED_FIELD(vn);
			}
			else{
				PagerResult= NULL;
			}
			called= 1;
		}
	  	if( !strncmp( buffer, SET_INTERNAL, strlen(SET_INTERNAL)) ){
		  	parse_vars_internal= 1;
		  	internal= &buffer[strlen(SET_INTERNAL)];
			Watch= False;
			r= _parse_varline( internal, (Variable_t*) vars_internals, VARS_INTERNALS, changes, False, outfp, errfp);
			parse_vars_internal= 0;
			Watch= w;
			if( r ){
/* 				buffer[0]= '\0';	*/
				_PARSE_VARLINE_RETURN(-1);
			}
		}
		else if( !strcmp( buffer, SHOW_INTERNAL) ){
		  int i;
			fprintf( *outfp, "### vars internal Variables in Variable output format:\n");
			for( i= 0; i< VARS_INTERNALS; i++){
				fprintf( *outfp, "%s ", SET_INTERNAL);
				print_var( *outfp, (Variable_t*) &vars_internals[i], 3);
			}
/* 			buffer[0]= '\0';	*/
			_PARSE_VARLINE_RETURN(-1);
		}
		WWatch = _ww_;
	}

	  /* first check to see whether buffer contains a commando to
	   * access the environmental variables. If so, process buffer
	   * as such, and return; else process buffer in relation to
	   * the Variables in vars. Note that you can have a Variable
	   * named 'env', but to reference it, it should be preceded
	   * by some whitespace.
	   */
	if( strcmp( buffer, "/")== 0 ){
/* 		buffer[0]= '\0';	*/
		_PARSE_VARLINE_RETURN(-1);
	}
	else if( strncmp( buffer, "env", 3)== 0){
	  char *buf= &buffer[3], *var= NULL, *val= NULL;
		while( *buf && !isspace((unsigned char) *buf))
			buf++;
		while( *buf && isspace((unsigned char) *buf))
			buf++;
		var= buf;
		while( *buf && *buf!= '=' && *buf!= '?' )
			buf++;
		if( *buf)
			*buf++= '\0';
		if( *buf!= '?')
			val= buf;
		if( *var && *val ){
			buf= setenv( var, val);
			if( !buf){
				fprintf( *errfp, "Error: '%s' != '%s' (%s)\n",
					var, val, serror()
				);
				Flush_File( *errfp);
			}
		}
		else if( *var){
			if( (val= getenv(var)) ){
				fprintf( *outfp, "env %s=%s\n", var, val );
				Flush_File( *outfp);
			}
		}
		else{
#if defined(_UNIX_C_) && !(defined(__APPLE_CC__) || defined(__MACH__))
		  extern char **environ;
		  char **env= environ;
			while( *env){
				fprintf( *outfp, "env %s\n", *env);
				env++;
			}
#endif
		}
/* 		buffer[0]= '\0';	*/
		_PARSE_VARLINE_RETURN(-1);
	}
	else if( strncmp( buffer, "shell", 5)== 0 || buffer[0]== '!' ){
	  char *command= &buffer[5];
	  int ret;
	  void_method p;
#ifndef VARS_STANDALONE
	  extern int Check_PipeProcess();
#ifndef WIN32
	  extern void call_alarm_call();
#endif
#endif
		if( buffer[0]== '!'){
			command= &buffer[1];
		}
#ifndef WIN32
		p= (void_method) signal( SIGALRM, SIG_IGN );
#endif
		ret= system(command);
		fprintf( *outfp, "shell %s\t# %d\n", command, ret >> 8 );
#ifndef WIN32
#if !defined(VARS_STANDALONE)
		if( p== (void_method) call_alarm_call ){
			set_alarm_call( 1800, Check_PipeProcess );
		}
		else
#endif
		{
			signal( SIGALRM, p);
		}
#endif
		_PARSE_VARLINE_RETURN(-1);
	}
	else if( strcmp( buffer, "$$") == 0){
#if !defined(WIN32) && !defined(_UNISTD_H) && !defined(__APPLE_CC__) && !defined(__MACH__)
	  extern long getpid(), getpgrp(), getpgrp2(), getppid();
#endif
	  extern char *ttyname();
	  char *tty;
	  int e= errno;
#ifndef _HPUX_SOURCE
#	ifdef WIN32
		fprintf( *outfp, "#\tpid=%ld tty=%s\n",
			getpid(), ( (tty= ttyname(fileno(stdin))) )? tty : serror()
		);
#	else
		fprintf( *outfp, "#\tpid=%ld pgrp=%ld ppid=%ld tty=%s\n",
			getpid(), getpgrp(), getppid(),
			( (tty= ttyname(fileno(stdin))) )? tty : serror()
		);
#	endif
#else
		  /* HP-UX also knows the pgroup id of a process	*/
		fprintf( *outfp, "#\tpid=%ld pgrp=%ld ppid=%ld ppgrp=%ld tty=%s\n",
			getpid(), getpgrp(), getppid(), getpgrp2( getppid() ),
			( (tty= ttyname(fileno(stdin))) )? tty : serror()
		);
#endif
/* 		buffer[0]= '\0';	*/
		errno= e;
		_PARSE_VARLINE_RETURN(-1);
	}
	else if( strneq( buffer, "RE^", 3) ){
		find_next_method= find_next_regex_var;
/* 		strcpy( buffer, &buffer[2] );	*/
		buf_offset= 2;
	}

	  /* check for an empty line: nothing should be done then,
	   * and no errors reported
	   */
	for( name= &buffer[buf_offset]; *name && isspace((unsigned char)*name); name++)
		;
	if( !*name){
/* 		buffer[buf_offset]= '\0';	*/
		errno= 0;
		_PARSE_VARLINE_RETURN(-1);
	}

	*changes= 0L;
	strncpy( Processed_Buffer, buffer, 63);
	if( n< 0 ){
		var->hit= 0;
		I= var->id;
		n= 0;
		if( (var= (*find_next_method)( &buffer[buf_offset], var, var->id+1, &I, &name, &match, &option, *errfp)) ){
			goto one_variable_case;
		}
		else{
			fprintf( *errfp, "vars::_parse_varline(): single Variable \"%s\" doesn't match buffer \"%s\"\n",
				vars->name, buffer
			);
		}
	}
	else{
		for( I= vars->id; I< n; var++, I++)
			var->hit= 0;
		var= vars;
		I= var->id;
	}
	errno= ENOSUCHVAR;
	while( (var= (*find_next_method)( &buffer[buf_offset], var, n, &I, &name, &match, &option, *errfp)) ){
	  char *char_var_buffer;
	  int Operator= 0, Index= 0, allow_echo= 0;
one_variable_case:;
		Operator= 0;
		Index= 0;
		if( name &&
			(name== &buffer[buf_offset] || !(name[-2]== REFERENCE_OP && name[-1]==REFERENCE_OP2) )
		){
			line_invocation= 0;
			assignOp= NULL;
			if( print_parse_errors ){
				fprintf( *errfp, "# \"%s\" = Variable \"%s\"\n", match, var->name );
				Flush_File( *errfp);
			}
			Last_ErrorVariable= NULL;
			Processed_Buffer[0]= 0;
			errno= 0;
			var->args= NULL;
			var->Stdout= *outfp;
			var->Stderr= *errfp;
			var->serial= 0;
			PIPE_var= (PIPE_file)? var : NULL;
  /* Here we can jump back if a '{' immediately follows a '}'	*/
doit_once_more:;
			  /* increment the serial counter telling how many times the Variable
			   \ is accessed on one line.
			   */
			var->serial+= 1;
			if( *option== '[' ){
			  char *c;
				{ int escaped= False;
					c= find_balanced( ++option, ']', '[', ']', &escaped );
				}
				if( c ){
				  short range[2];
				  int n= 2;
					*c= '\0';
					range[0]= 0;
					range[1]= var->maxcount;
					dascanf( &n, option, range, NULL );
					if( var->type!= COMMAND_VAR ){
						CLIP( range[0], 0, (short) (var->maxcount-1) );
						CLIP( range[1], 0, (short) (var->maxcount-1) );
					}
					var->first= (short) range[0];
					var->last= (short) (range[1]);
					*c= ']';
					option= &c[1];
				}
			}
			else if( var->type!= COMMAND_VAR ){
				var->first= 0;
				var->last= var->maxcount- 1;
			}
			else{
				var->first= 0;
				var->last= 0;
			}
			if( index( Variable_Operators, *option) ){
				Operator= *option;
				option++;
			}
			var->Operator= Operator;
			left_brace= NULL;
			right_brace= NULL;
			p= (*option== VarsSeparator[0])? &option[1] : option;
			  /* using find_balanced(), separation should not occur
			   \ within the matching braces of a Variable argument
			   */
			{ int escaped= False;
				if( (separator= find_balanced( p, VarsSeparator[0], LEFT_BRACE, RIGHT_BRACE, &escaped ) ) ){
					if( !escaped ){
						orig_sep= *separator;
						*separator= '\0';
					}
				}
			}
			p= &option[1];
			remember_var= True;
			switch( *option ){
				case '?':
					if( execute_command_question && var->type== COMMAND_VAR){
						K= (long) do_command_var( var, vars, n, p, &J, errfp );
						count= var->count;
						Flush_File( *outfp );
					}
					  /* print all matching items.	*/
					if( parse_vars_internal)
						fprintf( *outfp, "%s ", SET_INTERNAL);
					if( !var->parent && VarVarActive && var_var && in_var_var( var_var, var) ){
						fprintf( *outfp, "%s%c ", LEFT_BRACE, var_var->name);
						if( option[1]== '?'){
							J+= (long) print_var( *outfp, var, -4);
						}
						else{
							J+= (long) print_var( *outfp, var, -3);
						}
					}
					else{
						if( option[1]== '?'){
							J+= (long) print_var( *outfp, var, 3);
						}
						else{
							J+= (long) print_var( *outfp, var, 2);
						}
					}
					watch= False;
					break;
				case ']':
					if( name!= &buffer[buf_offset]){
						if( name[-1]== '[' ){
							  /* check if this was a mean_of_reference_variable
							   \ WHY IS THIS NECESSARY??
							   */
							if( !( name[-3]== REFERENCE_OP && name[-2]== LEFT_BRACE && option[1]== RIGHT_BRACE) ){
								if( !var->parent && VarVarActive && var_var)
									fprintf( *outfp, "%s%c ", LEFT_BRACE, var_var->name);
								if( parse_vars_internal)
									fprintf( *outfp, "%s ", SET_INTERNAL);
								J+= fprint_varMean( *outfp, var);
								if( !var->parent && VarVarActive && var_var)
									fprintf( *outfp, " %c\n", RIGHT_BRACE);
								else
									fputc( '\n', *outfp);
							}
							else{
								remember_var= False;
							}
						}
					}
					watch= False;
					break;
				case LEFT_BRACE:
					watch= True;
#ifdef CURLY_RESTRICTED
					if( var->type!= VARIABLE_VAR && var->type!= COMMAND_VAR && var->type!= CHAR_VAR ){
						Last_ErrorVariable= var;
						errno= EVARILLSYNTAX;
					}
					else
#endif
					{
					  char *b= &option[1];
					  int brace_level= 1;
						left_brace= option;
						right_brace= b;
						  /*  This loop looks for the first non-escaped RIGHT_BRACE */
						if( *right_brace!= RIGHT_BRACE){
						  int escaped= (*right_brace== '\\');
							while( *right_brace && brace_level ){
								right_brace++;
								if( *right_brace== LEFT_BRACE && !escaped ){
									brace_level++;
								}
								else if( *right_brace== RIGHT_BRACE && !escaped ){
									brace_level--;
								}
								if( *right_brace== '\\' ){
									escaped= !escaped;
								}
								else{
									escaped= False;
								}
							}
						}
						if( !right_brace || *right_brace!= RIGHT_BRACE ){
							fprintf( *errfp, "vars::_parse_varline(): missing last '%c'\n", RIGHT_BRACE);
							Flush_File( *errfp);
							Last_ErrorVariable= var;
							errno= EVARILLSYNTAX;
						}
						else{
							*right_brace= 0;
							  /* in this case we should not throw away
							   \ leading whitespaces for the CHAR_VAR's
							   */
							p= option+ 1;
							char_var_buffer= p;
							while( isspace((unsigned char)*p) )
								p++;
							goto assign_case;
						}
					}
					break;
				case '=':
					watch= True;
					  /* discard leading whitespaces of option part of buffer	*/
					p= option+1;
					char_var_buffer= p;
					while( isspace((unsigned char)*p) )
						p++;

assign_case:;
					line_invocation+= 1;
					allow_echo= 1;
					if( var->access== -1){
						Last_ErrorVariable= var;
						errno= EVARRDONLY;
						break;
					}
					
					  /* check for Variable references
					   \ the present format is ${name} or ${[name]} for mean,stderr.
					   \ Optionally, a [<index] can be appended directly after the
					   \ closing brace of the pattern. The whole input line is scanned
					   \ for these kind of patterns, which are parsed and replaced by
					   \ the result of this parsing. The resulting new inputline is build
					   \ in the internal buffer 'ref_val'. The original inputline is touched,
					   \ but restored.
					   \ 20030307: check the environment for name if no reference variable of
					   \ that name exists.
					   */
					if( Reference_Variable && Reference_Variables ){
					  char *rv= ref_val, *p_mem= p, *p_mem2= p;
					  int subst= 0, skip_rest= 0, prune= True;
						do{
							if( *p== REFERENCE_OP && p[1]== LEFT_BRACE ){
							  int aver, J= 0, I= 0, Index= -1;
							  char *cIndex= NULL, *end= &p[2], pend, *c= NULL, pc= 0, *ev;
							  Variable_t *rvar;
							  VariableSelection *vs;
							  int escaped= False;
								p+= 2;
								  /* find the end of the sub-expression	*/
								end= find_balanced( end, RIGHT_BRACE, LEFT_BRACE, RIGHT_BRACE, &escaped);
								if( *end!= RIGHT_BRACE ){
									fprintf( vars_errfile, "##\"%s\": missing '%c'\n",
										p_mem, RIGHT_BRACE
									);
									Flush_File( vars_errfile );
									errno= EVARILLSYNTAX;
									skip_rest= 1;
								}
								if( (pend= *end) ){
									cIndex= end+ 1;
								}
								*end= '\0';
								if( (aver= (*p== '[')) ){
								  /* average value requested	*/
									p++;
									if( (c= index( p, ']')) ){
										pc= *c;
										*c= '\0';
									}
									else{
										fprintf( vars_errfile, "##\"%s\": missing right ']' of mean\n",
											p_mem
										);
										Flush_File( vars_errfile );
										errno= EVARILLSYNTAX;
										skip_rest= 1;
									}
								}
								if( cIndex  && *cIndex== '[' ){
								  /* an index was requested	*/
									cIndex++;
									while( *cIndex && !isdigit((unsigned char)*cIndex) && *cIndex!= ']' ){
									  /* skip whitespace	*/
										cIndex++;
									}
									if( isdigit((unsigned char)*cIndex) ){
										Index= atoi( cIndex);
									}
									else{
										fprintf( vars_errfile, "##\"%s\": ignoring illegal index '%s'\n",
											p_mem, cIndex
										);
										Flush_File( vars_errfile );
										errno= EVARILLSYNTAX;
									}
									while( *cIndex && *cIndex!= ']' ){
										cIndex++;
									}
									if( !*cIndex ){
										fprintf( vars_errfile, "##\"%s\": missing right ']' of index\n",
											p_mem
										);
										Flush_File( vars_errfile );
										errno= EVARILLSYNTAX;
										skip_rest= 1;
									}
								}
								else{
								  /* false alarm	*/
									cIndex= NULL;
								}

								  /* check if we are referencing one of the following:	*/
								if( !strcmp( p, "*") )
									rvar= Last_ParsedVariable;
								else if( !strcmp( p, "*>") )
									rvar= Last_ParsedVVariable;
								else if( !strcmp( p, "I*") )
									rvar= Last_ParsedIVariable;
								else if( !strcmp( p, "I*>") )
									rvar= Last_ParsedIVVariable;
								else if( !strcmp( p, "&") )
									rvar= Last_ReferenceVariable;
								else{
								  /* no. Search in the Reference_Variable	*/
									rvar= find_named_VariableSelection( p,
										Reference_Variable, Reference_Variables, &I, &J, &vs
									);
								}
								if( rvar){
								  /* found something; make a new input	*/
									if( aver){
										sprint_varMean( rv, rvar, VARSEDIT_LENGTH- strlen(ref_val));
										fprintf( vars_errfile, "# ${[%s]}", rvar->name);
										if( Index>= 2)
											Index= 1;
									}
									else{
										sprint_var( rv, rvar, 0, VARSEDIT_LENGTH- strlen(ref_val));
										fprintf( vars_errfile, "# ${%s}", rvar->name);
									}
									if( Index>= 0){
										if( Index>= rvar->maxcount)
											Index= rvar->maxcount- 1;
										fprintf( vars_errfile, "[%d]", Index);
									}
									fputs( "==", vars_errfile);
									Last_ReferenceVariable= rvar;
								}
								else if( ( ev = getenv(p) ) ){
									strncpy( rv, ev, VARSEDIT_LENGTH-strlen(ref_val) );
									fprintf( vars_errfile, "# env.var ${%s}==", p );
									rvar= NULL;
									prune= False;
								}
								else{
									strncpy( Processed_Buffer, p, 63);
									errno= ENOSUCHVAR;
									rv[0]= '\0';
									fprintf( vars_errfile, "# $not found: \"%s\"", p);
								}
								  /* restore the input-buffer	*/
								if(c){
									*c= pc;
								}
								*end= pend;
								  /* later on, we continue from here
								   \ i.e. either from (after) the index spec,
								   \ or after the closing brace
								   */
								p_mem= (cIndex)? cIndex : end;

								if( prune ){
									 /* Here <p> is pointed to the (local pointer to the) ref_val buffer	*/
									p= index( rv, '=');
									subst+= 1;

									if( !p){
									  /* no '=' in rv. Maybe a brace? If not, just
									   \ use the start of rv, else remove the (matching) right brace.
									   \ Since this buffer is written by us, we can be reasonably sure
									   \ that there is a matching brace (if the buffer was long enough)
									   */
										p= index( rv, LEFT_BRACE );
										if( !p){
											p= rv;
										}
										else{
											c= (++p);
											escaped= False;
											if( (c= find_balanced( c, RIGHT_BRACE, LEFT_BRACE, RIGHT_BRACE, &escaped)) &&
												*c== RIGHT_BRACE
											){
												*c= '\0';
											}
										}
									}
									else{
									  /* skip the '='	*/
										p++;
									}
								}
								else{
									p= rv;
									subst= -1;
								}
								  /* See if we have to give a certain element. We do
								   \ that by counting separators (,). This ought to be
								   \ cleaned up - s-printing only that element in the
								   \ ref_val buffer!
								   */
								if( Index>= 0){
								  char *f, *l;
								  int i;
									if( (f= index( p, ',')) ){
										if( Index){
											l= index( f+ 1, ',');
											for( i= 0; i< Index- 1 && f && l; i++){
												f= l;
												l= index( f+ 1, ',');
											}
											if( l)
												*l= '\0';
											if( f)
												p= f+1;
										}
										else{
											*f= '\0';
										}
									}
								}
								  /* 20020212: */
								if( rvar && rvar->type!= CHAR_VAR ){
									  /* Remove any leading whitespace:	*/
									while( *p && isspace((unsigned char)*p) ){
										p++;
									}
									if( p> rv ){
									  /* discard unwanted information BEFORE the desired info
									   \ Unwanted info AFTER the desired info has already been
									   \ discarded by setting a '\0' at the appropriate place.
									   \ 20000207: not necessarily. Discard anything leftover.
									   */
									   char *e= &p[1];
										while( *e && !isspace((unsigned char)*e) && !index( "#}", *e ) ){
											e++;
										}
										if( *e ){
											*e= '\0';
										}
										strcpy( rv, p);
									}
								}
								else{
								  /* 20020212: strings should be copied completely, including spaces etc. It
								   \ has to be assumed that the earlier operations already removed all that had
								   \ to be removed from the sprint_var() output of rvar (in rv).
								   */
									strcpy( rv, p );
								}
								fprintf( vars_errfile, "%s\n", rv);
								Flush_File( vars_errfile);
								  /* increment the ref_val pointer to the end	*/
								while( *rv ){
									rv++;
								}
							}
							else{
							  /* p== p_mem; copy the first character	*/
								*rv++= *p;
								*rv= '\0';
							}
							if( !skip_rest && *p_mem && *(p= &p_mem[1]) ){
							  /* check starting from p_mem[1];
							   \ non-interesting characters are copied into ref_val
							   */
								while( *p && !(*p== REFERENCE_OP && p[1]== LEFT_BRACE) && rv< ref_val+ VARSEDIT_LENGTH-1 ){
									*rv++= *p;
									p++;
								}
								*rv= '\0';
								p_mem= p;
							}
							else{
								p= NULL;
							}
						} while( p && *p && rv< ref_val+ VARSEDIT_LENGTH-1 );
						if( subst ){
						  /* ref_val contains a different line than p_mem2 (the original)	*/
							p= ref_val;
							char_var_buffer= p;
							fprintf( vars_errfile, "# new input: \"%s\" (%d changes)\n", p, subst);
						}
						else{
						  /* nothing. use the original	*/
							p= p_mem2;
						}
					}
			/* OOf. That's 241 lines (as of 941125) of code for such a seemingly simple functionality...	*/

/* 950915 : deleted the following here; I want to be able to pass a leading whitespace
 \ to a CHAR_VAR or COMMAND_VAR or ...
 */
/* 					char_var_buffer= p;	*/

					  /* to be sure...	*/
					Index= var->first;

					count= (int) var->last - var->first+ 1;
					  /* tell the ascanf() routines where we will be starting	*/
					ascanf_index_value= var->first;

					K= 0;
					  /* since we are about to make an (attempt to an) assignment,
					   * reset the changed field to VAR_UNCHANGED
					   */
					for( i= var->first; i<= var->last; i++){
						RESET_CHANGED_FIELD( var, i);
					}
					if( Operator){
						if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
							*assignOp= '\0';
						}
						K= parse_operator( var, Operator, p, &count, *errfp);
					}
					else switch( var->type){
						case CHAR_VAR:
						  /* This doesn't yet set the CHANGED_FIELD!!	*/
							if( char_var_buffer[0] ){
								K= cook_string( &(var->var.c[Index]), char_var_buffer, &count, NULL);
							}
							else{
							  /* We want to be able to pass/set an empty string...	*/
								K= (var->var.c[Index])? 1 : 0;
								var->var.c[Index]= '\0';
							}
							break;
						case UCHAR_VAR:
							if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
								*assignOp= '\0';
							}
							K= (long) cascanf( &count, p, &(var->var.c[Index]), CHANGED_FIELD(var) );
							break;
						case SHORT_VAR:
						case USHORT_VAR:
							if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
								*assignOp= '\0';
							}
							K= (long) dascanf( &count, p, &(var->var.s[Index]), CHANGED_FIELD(var) );
							break;
						case INT_VAR:
						case UINT_VAR:
							if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
								*assignOp= '\0';
							}
							if( sizeof(int)== 2)
								K= (long) dascanf( &count, p, &(var->var.s[Index]), CHANGED_FIELD(var) );
							else
								if( sizeof(int)== 4)
									K= (long) lascanf( &count, p, &(var->var.l[Index]), CHANGED_FIELD(var) );
							break;
						case LONG_VAR:
						case ULONG_VAR:
							if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
								*assignOp= '\0';
							}
							K= (long) lascanf( &count, p, &(var->var.l[Index]), CHANGED_FIELD(var) );
							break;
						case HEX_VAR:
							if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
								*assignOp= '\0';
							}
							K= (long) xascanf( &count, p, &(var->var.l[Index]), CHANGED_FIELD(var) );
							break;
						case FLOAT_VAR:
							if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
								*assignOp= '\0';
							}
							K= (long) hfascanf( &count, p, &(var->var.f[Index]), CHANGED_FIELD(var) );
							break;
						case DOUBLE_VAR:
							if( (assignOp= index( p, '=' )) && assignOp[1]!= '=' ){
								*assignOp= '\0';
							}
							K= (long) fascanf( &count, p, &(var->var.d[Index]), CHANGED_FIELD(var) );
							break;
						case COMMAND_VAR:
							K= (long) do_command_var( var, vars, n, char_var_buffer, &J, errfp );
							count= var->count;
							Flush_File( *outfp );
							break;
						case VARIABLE_VAR:{
						  Variable_t *syn= var->var.ptr;
							if( !VarVarActive){
								VarVarActive++;
								var_var= var;
								var->count= 0;
								J+= _parse_varline( p, syn, var->maxcount, changes, True, outfp, errfp);
								K= count= (int) var->count;
								VarVarActive--;
								var_var= NULL;
							}
							break;
						}
						case CHAR_PVAR:
						case SHORT_PVAR:
						case INT_PVAR:
						case LONG_PVAR:
						case FLOAT_PVAR:
						case DOUBLE_PVAR:{
						  void **ptr= (void**) var->var.ptr;
							K= (long) ascanf( ObjectTypeOfVar[var->type],
									&count, p, &(ptr[Index]), CHANGED_FIELD(var)
								);
							if( K== 0 && count== 0 ){
								fprintf( vars_errfile, "vars::_parse_varline(%s): Var %s,%s: potential error in ascanf()\n",
									buffer, var->name, ObjectTypeOfVar[var->type]
								);
								fflush( vars_errfile);
							}
							break; 
						}
					}
					var->count= count;
					if( K== (long) EOF){
					  char *pp= p;
						while( isspace(*pp) ){
							pp++;
						}
						if( *pp ){
						  /* read-error on a non-empty buffer. This is to exclude error-messages
						   \ on an "empty assignment", which is reserved for special uses by the
						   \ user (cf. _seed48() )
						   */
							K= (long) count;
							errno= EVARILLSYNTAX;
						}
					}
					if( K ){
						if( var->type!= COMMAND_VAR ){
							var->args= p;
							if( var->change_handler){
								if( print_parse_errors ){
									fprintf( *errfp, "#-> change_handler %s(\"%s\",%d,%ld,%ld)\"%s\"\n",
										address_symbol(var->change_handler), vars_Find_Symbol(vars),
										n, (long) var->id, K,
										(var->args)? var->args : ""
									);
									Flush_File(*errfp);
								}
								if( n== 0 ){
									(*var->change_handler)( vars, 1, 0L, K);
								}
								else{
									(*var->change_handler)( vars, n, (long) var->id, K);
								}
								Flush_File( *outfp );
#ifndef VARS_STANDALONE
								Elapsed_Since( &varsV_timer->timer );
#endif
								if( print_timing ){
									fprintf( *errfp,
										"#T# %s->change_handler %s(\"%s\",%d,%ld,0L)\"%s\": %g (%g sys) seconds\n",
										var->name,
										address_symbol(var->change_handler), vars_Find_Symbol(vars),
										n, (long) var->id, (var->args)? var->args : "",
#ifndef VARS_STANDALONE
										varsV_timer->timer.Tot_Time, varsV_timer->timer.Time
#else
										-1, -1
#endif
									);
									Flush_File(*errfp);
								}
							}
							var->args= NULL;
						}
						if( var_label_change ){
							var->change_label= var_change_label;
						}
						if( VarVarActive && var_var){
							var_var->count++;
							SET_CHANGED_FIELD(var_var, var->id, VAR_CHANGED);
						}
					}
					*changes+= K;
					break;
				default:
					allow_echo= 1;
					if( var->type== COMMAND_VAR){
					  /* execute the command, with the buffer starting at the
					   * not-recognised character
					   */
						watch= True;
						if( !p){
							p= option;
						}
						K= do_command_var( var, vars, n, &p[-1], &J, errfp);
						Flush_File( *outfp );
						var->count= count;
						if( K== (long) EOF){
							K= (long) count;
						}
						*changes+= K;
					}
					else{
						strncpy( Processed_Buffer, p, 63);
						Last_ErrorVariable= var;
						errno= EVARILLSYNTAX;
					}
					break;
			}
			if( var->parent && !(VarVarActive && var_var) ){
			  /* an item of a VARIABLE_VAR, somehow bypassing its
			   \ parent. We call the parent's change_handler, just in case
			   \ it does some definitive action initiated by the child's handler
			   \ (I do that sometimes: all childs share the parent's change_handler,
			   \ which does the actual work after the child's have set certain switches)
			   */
				var->parent->args= _buffer;
				if( var->parent->change_handler){
					if( print_parse_errors ){
						fprintf( *errfp, "#-> change_handler %s(\"%s\",%d,%ld,%ld)\"%s\"\n",
							address_symbol(var->parent->change_handler), vars_Find_Symbol(vars),
							n, (long) var->id, K,
							(var->parent->args)? var->parent->args : ""
						);
						Flush_File(*errfp);
					}
					(*var->parent->change_handler)( var->parent, 1, 0, K);
					Flush_File( *outfp );
#ifndef VARS_STANDALONE
					Elapsed_Since( &varsV_timer->timer );
#endif
					if( print_timing ){
						fprintf( *errfp,
							"#T# %s->change_handler %s(\"%s\",%d,%ld,0L)\"%s\": %g (%g sys) seconds\n",
							var->parent->name,
							address_symbol(var->parent->change_handler), vars_Find_Symbol(vars),
							n, (long) var->id, (var->parent->args)? var->parent->args : "",
#ifndef VARS_STANDALONE
							varsV_timer->timer.Tot_Time, varsV_timer->timer.Time
#else
							-1, -1
#endif
						);
						Flush_File(*errfp);
					}
				}
				var->parent->args= NULL;
				if( var_label_change ){
					var->parent->change_label= var_change_label;
				}
			}
			if( remember_var ){
				if( !parse_vars_internal){
					if( VarVarActive)
						Last_ParsedVVariable= var;
					else
						Last_ParsedVariable= var;
				}
				else{
					if( VarVarActive)
						Last_ParsedIVVariable= var;
					else
						Last_ParsedIVariable= var;
				}
			}

			if( Watch_Variables ){
			  Watch_Variable *wv= Watch_Variables;
			  int w= Watch, *_ww_ = WWatch;
				WWatch= &w;
				if( Watch && watch ){
					Watch= False;
					fprintf( stderr, "#W \"%s\"\n", buffer );
					while( wv ){
						if( wv->type== VARIABLE_VAR ){
							fputs( " # ", stderr );
							print_var( stderr, wv->var, 0);
						}
						else if( wv->type== CHAR_VAR ){
						  long changes= 0;
						  FILE *stde= stderr;
							fprintf( stderr, " # ");
							parse_varline_with_list( wv->buf, vars_arg_items, vars_arglist, &changes, False, outfp, &stde,
								"_parse_varline(Watch)"
							);
						}
						wv= wv->tail;
					}
					fputs( " W#\n", stderr);
					fflush( stderr );
					Watch= w;
				}
				else{
					if( !Watch ){
						fprintf( stderr, "#W# Watch skipped (off) \"%s\"\n", buffer );
					}
					else{
						fprintf( stderr, "#W# Watch skipped (no changes made) \"%s\"\n", buffer);
					}
					fflush( stderr );
				}
				WWatch = _ww_;
			}

			if( assignOp ){
				*assignOp= '=';
			}
			if( separator ){
				*separator= orig_sep;
			}

			if( left_brace && right_brace){
			  char *c= &left_brace[1];
			  /* erase the contents of this block in the input buffer
			   * and put back the RIGHT_BRACE in right_brace instead of the NULL
			   * used to demark the end of the block.
			   */
				while( c!= right_brace){
					*c++= ' ';
				}
				*right_brace= RIGHT_BRACE;
				if( right_brace[1]== '[' || right_brace[1]== LEFT_BRACE ){
				  /* This provides some hacked-in support for multiple
				   \ command calls in a row ( name[range]{arg1}[range]{arg2}[range]{arg3})
				   \ a la TeX.
				   */
					option= &right_brace[1];
					goto doit_once_more;
				}
			}
			left_brace= NULL;
			right_brace= NULL;

			  /* make sure that we don't meet this Variable again,
			   * not even as behind our back it was unmarked because
			   * e.g. the change_handler called us. We even mark the
			   * Variable as "used" (hit=2, even though this feature is not used..)
			   */
			var->hit= 2;
			var->first= 0;
			var->last= var->maxcount- 1;
			if( vars_echo && allow_echo ){
				print_var( *outfp, var, vars_echo);
			}
		}
	}
	  /* if zero (error) nothing happened	*/
	_PARSE_VARLINE_RETURN( J+ *changes);
}

/* _parse_varline has the habit of setting buffer[0] to \0 in some
 * cases, to prevent from evaluating the buffer again (a.o. the "env"
 * option) when called repeatedly with the same buffer and different
 * Variable lists. This routine restores the first character, so
 *   parse_varline( "env", .....);
 * will dump the environment every time (and not just the first time)
 * 940516: _parse_varline() now returns -1 in those cases, and no
 * longer sets buffer[0] to '\0'.
 */
long parse_varline( char *buffer, Variable_t *vars, long n, long *changes, int cook_it, FILE **outfp, FILE **errfp)
/* char *buffer;	*/
/* Variable_t *vars;	*/
/* long *changes, n;	*/
/* int cook_it;	*/
/* FILE **outfp, **errfp;	*/
{  char c1;
   long j;
   int depth;
   int pipe= 0;
   int done= False;
   FILE *ierrfp, *ioutfp;
   int Cws;
   char *lbuffer= NULL;

   CX_MAKE_WRITABLE(buffer, lbuffer);
   CX_DEFER_WRITABLE(Cws);

	PUSH_TRACE("parse_varline()", depth);
	if( !strncmp( buffer, "Page ", 5) ){
		buffer+= 5;
		if( strlen(buffer) && !PIPE_file ){
			PIPE_file= Open_Pager( stdout);
			outfp= &PIPE_file;
			pipe= PIPE_nr;
			PIPE_depth= depth;
		}
	}
	if( !strncmp( buffer, "Stream2 ", 8) ){
		buffer+= 8;
		if( strlen(buffer) && !PIPE_file ){
			PIPE_file= Open_Stream( 2, stdout);
			outfp= &PIPE_file;
			pipe= PIPE_nr;
			PIPE_depth= depth;
		}
	}
	if( !strncmp( buffer, "Stream ", 7) ){
		buffer+= 7;
		if( strlen(buffer) && !PIPE_file ){
			PIPE_file= Open_Stream( 1, stdout);
			outfp= &PIPE_file;
			pipe= PIPE_nr;
			PIPE_depth= depth;
		}
	}
	if( !outfp ){
		ioutfp= stdout;
		outfp= &ioutfp;
	}
	if( !*outfp ){
		outfp= &ioutfp;
		*outfp= stdout;
	}
	if( !errfp ){
#ifndef VARS_STANDALONE
		errfp= &vars_errfile;
#else
		errfp= &ierrfp;
		*errfp= NULL;
#endif
	}
	if( !*errfp ){
		errfp= &ierrfp;
		*errfp= vars_errfile;
	}

	if( !strcmp( buffer, "list") ){
	  /* a list of all variables was asked for	*/
	  int i;
		if( !PIPE_file ){
			PIPE_file= Open_Pager( stdout);
			outfp= &PIPE_file;
			pipe= PIPE_nr;
			PIPE_depth= depth;
		}
		  /* no valid trailing arguments: list all	*/
		fprintf( *outfp, "#\t*** List #0 (%s)[%ld] ***\n\n",
			vars_address_symbol(vars), n
		);
		for( i= 0; i< n; i++){
			print_var( *outfp, &vars[i], list_level);
		}
		Flush_File( *outfp);
		done= True;
	}
	if( pipe && !PIPE_file){
		pipe= False;
	}
	if( done ){
		j= 1;
	}
	else{
		c1= buffer[0];
		errno = 0;
		j= _parse_varline( buffer, vars, n, changes, cook_it, outfp, errfp);
		buffer[0]= c1;
	}
	if( !j){
		fprintf( vars_errfile, "vars::parse_varline(\"%s\",%s): error",
			buffer, address_symbol( vars)
		);
		if( errno){
		  char *vS = Find_Symbol(vars);
#ifdef VARS_STANDALONE
			fprintf( vars_errfile, ": %s (%d)", serror(), errno );
#else
			fprintf( vars_errfile, ": %s", serror() );
#endif
			if( errno == ENOSUCHVAR && vS ){
				fprintf( cx_stderr, " varlist=\"%s\"", vS );
			}
		}
		if( Last_ErrorVariable)
			fprintf( vars_errfile, " var=\"%s\"", Last_ErrorVariable->name);
		if( strlen(Processed_Buffer) ){
			Processed_Buffer[sizeof(Processed_Buffer)-1]= '\0';
			fprintf( vars_errfile, " buf=\"%s\"", Processed_Buffer);
		}
		fputc( '\n', vars_errfile);
		Flush_File( vars_errfile);
	}
	if( pipe && PIPE_file && PIPE_depth== depth ){
		if( pipe== 1 )
			Close_Pager( PIPE_file);
		else if( pipe== 2)
			Close_Stream( which_PIPE, PIPE_file);
		PIPE_file= NULL;
		PIPE_depth= 0;
		pipe= False;
	}
	CX_END_WRITABLE(Cws);
	POP_TRACE(depth); return(j);
}

long parse_varline_safe( char *buffer, Variable_t *vars, long n, long *changes, int cook_it, FILE **outfp, FILE **errfp)
{ ALLOCA( copybuf, char, ((buffer)? strlen(buffer) : 1) + 1,xlen2);
	strcpy( copybuf, buffer );
	return( parse_varline( copybuf, vars, n, changes, cook_it, outfp, errfp ) );
}
	
static char last_prompt[8]= ">> ";

static long parse_varline_with_list( char *buffer, long items, Var_list *arglist,
	long *changes, int cook_it, FILE **outfp, FILE **errfp, char *caller )
/* char *buffer;	*/
/* long items;	*/
/* Var_list *arglist;	*/
/* long *changes;	*/
/* int cook_it;	*/
/* FILE **outfp, **errfp;	*/
/* char *caller;	*/
{ int N, errors= 0, depth;
  long ch= 0, n= 0, r= 0, j= 0;
  long rr;
  Variable_t *vars;
  int pipe= 0;
  char c1;
  FILE *ierrfp, *ioutfp;
  int Cws;
  char *lbuffer= NULL;

	CX_MAKE_WRITABLE(buffer, lbuffer);
	CX_DEFER_WRITABLE(Cws);

	r= 0;
	{  char buf[TRACENAMELENGTH];
	   Sinc sinc;
		Sinc_string( &sinc, buf, TRACENAMELENGTH-1, 0L);
		Sputs( "parse_varline_with_list()<-", &sinc);
		Sputs( caller, &sinc);
		PUSH_TRACE(buf, depth);
	}

	if( !strncmp( buffer, "Page ", 5) ){
		buffer+= 5;
		if( strlen(buffer) && !PIPE_file ){
			PIPE_file= Open_Pager( stdout);
			outfp= &PIPE_file;
			pipe= PIPE_nr;
			PIPE_depth= depth;
		}
	}
	if( !strncmp( buffer, "Stream2 ", 8) ){
		buffer+= 8;
		if( strlen(buffer) && !PIPE_file ){
			PIPE_file= Open_Stream( 2, stdout);
			outfp= &PIPE_file;
			pipe= PIPE_nr;
			PIPE_depth= depth;
		}
	}
	if( !strncmp( buffer, "Stream ", 7) ){
		buffer+= 7;
		if( strlen(buffer) && !PIPE_file ){
			PIPE_file= Open_Stream( 1, stdout);
			outfp= &PIPE_file;
			pipe= PIPE_nr;
			PIPE_depth= depth;
		}
	}
	if( pipe && !PIPE_file){
		pipe= False;
	}

	if( !outfp ){
		ioutfp= stdout;
		outfp= &ioutfp;
	}
	if( !*outfp ){
		outfp= &ioutfp;
		*outfp= stdout;
	}
	if( !errfp ){
#ifndef VARS_STANDALONE
		errfp= &vars_errfile;
#else
		errfp= &ierrfp;
		*errfp= NULL;
#endif
	}
	if( !*errfp ){
		errfp= &ierrfp;
		*errfp= vars_errfile;
	}

	rr= 0;
	c1= buffer[0];
	for( N= 0; N< items && rr!= -1 ; N++ ){
		n= arglist[N].n;
		vars= arglist[N].vars;
		if( vars== (Variable_t*) vars_internals ){
			if( vars_check_internals && !arglist[N].user ){
				if( rr> 0 ){
				  /* This is a situation where we don't want to check the internal
				   \ Variable list - at least 1 match has already been found. Even though
				   \ at the moment of this writing, this automated check of the internal list
				   \ is performed after all other lists have been processed, we use a goto
				   \ statement (instead of a break) to check for possible future "extensions".
				   \ Note that a continue statement would not do in this place...
				   */
					goto next_arglist;
				}
				else{
					fprintf( *errfp, "; Checking internal variables\n" );
				}
			}
			else{
			  /* auto-check should not be carried out	*/
				goto next_arglist;
			}
		}
		  /* 20030704: changed from _parse_varline to parse_varline: */
		if( (rr= parse_varline( buffer, vars, n, changes, cook_it, outfp, errfp))> 0 ){
			r+= rr;
			if( vars_echo ){
				fprintf( *errfp, "; Hit in Variable list #%d (%s)\n",
					N, vars_address_symbol(vars)
				);
			}
			ch+= *changes;
			if( *changes> 0)
				j++;
		}
		else{
			if( !rr)
				errors++;
			else
				r++;
			if( errno){
			  /* don't yell about NOSUCHVAR in list y if found in list x
			   * or to be found in list z
			   */
				if( errno!= ENOSUCHVAR || print_parse_errors ){
					fprintf( *errfp, "vars::%s: error in list #%d: %s", caller, N, serror());
					if( Last_ErrorVariable){
						if( Last_ErrorVariable->parent ){
							fprintf( *errfp, " item=\"%s\" in var=\"%s\"",
								Last_ErrorVariable->name, Last_ErrorVariable->parent->name
							);
						}
						else{
							fprintf( *errfp, " var=\"%s\"", Last_ErrorVariable->name);
						}
					}
					if( strlen(Processed_Buffer) ){
						Processed_Buffer[sizeof(Processed_Buffer)-1]= '\0';
						fprintf( *errfp, " buf=\"%s\"", Processed_Buffer);
					}
					fprintf( *errfp, " r=%ld c=%ld", rr, *changes);
					fputc( '\n', *errfp);
					errno= 0;
					errors--;
				}
			}
		}
next_arglist:;
	}
	buffer[0]= c1;
	if( !r && errors ){
		if( errno ){
			fprintf( *errfp, "vars::%s: error: %s", caller, serror());
			errno= 0;
		}
		else{
			fprintf( *errfp, "vars::%s: error", caller);
		}
		if( Last_ErrorVariable)
			fprintf( *errfp, " var=\"%s\"", Last_ErrorVariable->name);
		if( strlen(Processed_Buffer) ){
			Processed_Buffer[sizeof(Processed_Buffer)-1]= '\0';
			fprintf( *errfp, " buf=\"%s\"", Processed_Buffer);
		}
		fprintf( *errfp, " r=%ld c=%ld errors=%d", r, *changes, errors);
		fputc( '\n', *errfp);
	}
	*changes= ch;
	if( pipe && PIPE_file && PIPE_depth== depth ){
		if( pipe== 1 )
			Close_Pager( PIPE_file);
		else if( pipe== 2)
			Close_Stream( which_PIPE, PIPE_file);
		PIPE_file= NULL;
		PIPE_depth= 0;
		pipe= False;
	}
	CX_END_WRITABLE(Cws);
	POP_TRACE(depth); return(j);
}

#ifndef VARARGS_OLD
long parse_varlist_line( char *buffer, long *changes, int cook_it, FILE **outfp, FILE **errfp, long n, Variable_t *vars, ... )
#else
long parse_varlist_line( char *buffer, long *changes, int cook_it, FILE **outfp, FILE **errfp, CX_VA_DCL ... )
#endif
{  va_list ap;
#ifdef VARARGS_OLD
   long n;
#endif
   long i= 0;
   FILE *ierrfp, *ioutfp;
	if( !outfp ){
		ioutfp= stdout;
		outfp= &ioutfp;
	}
	if( !*outfp ){
		outfp= &ioutfp;
		*outfp= stdout;
	}
	if( !errfp ){
#ifndef VARS_STANDALONE
		errfp= &vars_errfile;
#else
		errfp= &ierrfp;
		*errfp= NULL;
#endif
	}
	if( !*errfp ){
		errfp= &ierrfp;
		*errfp= vars_errfile;
	}

#ifndef VARARGS_OLD
	va_start(ap, vars);
#else
	va_start(ap);
	n= va_arg(ap,long);
#endif
	vars_arg_items= 0;
	while( vars_arg_items< MAX_VAR_LISTS && n!= 0 ){
		vars_arglist[vars_arg_items].n= n;
#ifdef VARARGS_OLD
		vars_arglist[vars_arg_items].vars= va_arg( ap, Variable_t*);
#else
		vars_arglist[vars_arg_items].vars= vars;
#endif
		vars_arglist[vars_arg_items].user= True;
		vars_arg_items++;
		i++;
		n= va_arg(ap,long);
	}
	if( vars_arg_items< MAX_VAR_LISTS- 1 && vars_check_internals ){
		vars_arglist[vars_arg_items].n= VARS_INTERNALS;
		vars_arglist[vars_arg_items].vars= (Variable_t*) vars_internals;
		vars_arglist[vars_arg_items].user= False;
		vars_arg_items++;
	}
	if( vars_arg_items>= MAX_VAR_LISTS && n!= 0 ){
	  Variable_t *dum;
		while( (n= va_arg(ap, long)) ){
			dum= va_arg( ap, Variable_t*);
			i++;
		}
		fprintf( *errfp, "vars::parse_varlist_line(): %ld lists passed: %ld max\n", i, vars_arg_items);
		Flush_File( *errfp);
	}
	va_end(ap);
	return( parse_varline_with_list( buffer, vars_arg_items, vars_arglist, changes, cook_it, outfp, errfp, "parse_varlist_line()" ) );
}

give_evl_help( prompt, efp)
char *prompt;
FILE *efp;
{  int i;
	efp= Open_Pager( efp);
	PIPE_fileptr= &efp;
	fputs( "Commands: (lines marked with '> ' are accessible only in interactive mode)\n", efp);
	fputs( "\t> help\t: show this message\n", efp);
	fputs( "\t> exit\t: exit from this prompt (", efp);
		fputs( prompt, efp); fputs( ")\n", efp);
	fputs( "\t> Page <string>\t: parse <string> with vars->Stdout connected to the pager\n", efp );
	fputs( "\tRE^<regex>$<string>\t: apply <string> to all <Name>s matching ^<regex>$\n", efp );
	fputs( "\t\t\tOften the '^' needs to be escaped: compare ^G (bell) to \\^G !\n", efp );
	fputs( "\t\tAppend '[s,e]' to '<Name>' (as below) to START at item index 's' and END at item 'e'\n", efp );
	fputs( "\t<Name>?\t: show the value of <Name>\n", efp);
	fputs( "\t<Name>??\t: show the value of <Name> and the associated description\n", efp);
	fputs( "\t[<Name>]\t: show the average (and standard deviation if not 0) of <Name>\n", efp);
	fputs( "\t<Name>= {val1,val2,..}\t: set <Name> to {Val1,Val2,..}\n", efp);
	fprintf( efp, "\t<Name>\"%s\"= {val1,val2,valN}\t: apply operator from list to <Name> using {Val1,Val2,..} :\n",
		Variable_Operators
	);
	fprintf( efp, "\t\t<Name.1,Name.2>\"%s\"= {val1,val2} ; <Name.3,..>\"%s\"= {valN}\n",
		Variable_Operators, Variable_Operators
	);
	for( i= 0; i< strlen(Variable_Operators) ; i++ ){
		fprintf( efp, "\t\t\t%c: %s\n",
				Variable_Operators[i],
				(char*) parse_operator( NULL, Variable_Operators[i], "help", NULL, efp )
		);
	}
	fprintf( efp, "\t\t%c{ref_name} indicates substitute value of Reference_Variable ref_name, or else env.var $ref_name\n", REFERENCE_OP);
	fprintf( efp, "\t\t%c{[ref_name]} indicates substitute mean and StDev. of Reference_Variable ref_name\n",
		REFERENCE_OP
	);
	fprintf( efp, "\t\t\t%c{rn}[index] or %c{[rn]}[index] means substitute value at index\n",
		REFERENCE_OP, REFERENCE_OP
	);
	fputs( "\t<CommandName>\t: call method associated with <CommandName>\n", efp);
	fputs( "\n\t\t> Use * as first character on line to indicate last accessed Variable\n", efp);
	fputs( "\t\tArguments to char, command or varlist Variable must be enclosed in {}: others need not;\n", efp);
	fputs( "\t\tmultiple arguments can be concatenated: <name>[range]{arg1}[range]{arg2}[range]{..}[range]{argN}\n", efp);
	fputs( "\t\t\twhere 'arg?' includes everything after the first '{' up to the first non-escaped '}' in input\n",
		efp
	);
	fputs( "\t\tand the '[range]' construct is optional\n", efp );
	fputs( "\n\t> list [#,..]\t: list all current variables with their values (optional list specification)\n",
		efp
	);
	fprintf( efp, "\t%s\t: show the settings of all internal Variables\n", SHOW_INTERNAL);
	fprintf( efp ,"\t%s |Variable syntax|\t: change/show an internal Variable\n", SET_INTERNAL); 
	fputs( "\t> stdinput <filename>\t: redirect file <filename> to stdin\n", efp);
	fputs( "\t> stdoutput <filename>\t: append stdout and stderr to file <filename>\n", efp);
	fputs( "\t> terminal <filename>\t: make <filename> the new terminal\n", efp);
#ifdef _UNIX_C_
	fputs( "\tenv <var>\t: show the value of the environment variable <var> or all\n", efp);
#else
	fputs( "\tenv <var>\t: show the value of the environment variable <var>\n", efp);
#endif
	fputs( "\tenv <var>=<val>\t: set the value of the environment variable <var> to <val>\n", efp);
	fputs( "\tshell <string>\t: execute <string>\n", efp);
	fputs( "\t!<string>\t: execute <string>\n", efp);
	fputs( "\t$$\t: print the process id, process group id, the parent process id and tty name\n", efp);
	if( Last_ParsedVariable){
		fputs( "\t> Last Accessed   (*): ", efp);
		print_var( efp, Last_ParsedVariable, list_level);
	}
	if( Last_ParsedVVariable){
		fputs( "\t> Last Accessed  (*>): ", efp);
		print_var( efp, Last_ParsedVVariable, list_level);
	}
	if( Last_ReferenceVariable){
		fputs( "\t> Last Referenced (&): ", efp);
		print_var( efp, Last_ReferenceVariable, list_level);
	}
	if( Last_ParsedIVariable){
		fputs( "\t> Last Accessed Internal  (I*): ", efp);
		print_var( efp, Last_ParsedIVariable, list_level);
	}
	if( Last_ParsedIVVariable){
		fputs( "\t> Last Accessed Internal (I*>): ", efp);
		print_var( efp, Last_ParsedIVVariable, list_level);
	}
	fprintf( efp, "\t> Maximum line length in interactive mode and include_file{}: %d bytes\n", VARSEDIT_LENGTH );
	Flush_File( efp);
	Close_Pager( efp);
	PIPE_fileptr= NULL;
}

char *edit_variables_list_interim= NULL;

int vars_isatty( FILE *fp )
{
	if( fp ){
		return( isatty(fileno(fp)) );
	}
	else{
		errno= EBADF;
		return(0);
	}
}

int edit_variables_maxlines= -1, *edit_variables_linesread= NULL;

void *lib_termcap= NULL, *lib_readline= NULL;
int readline_error= False;
DEFMETHOD( gnu_readline, (char *prompt), char* )= NULL;
DEFMETHOD( gnu_add_history, (char *line), void )= NULL;

#if defined(unix) || defined(linux) || defined(__MACH__) || defined(__APPLE_CC__)
int vars_try_readline= True;
#include <dlfcn.h>

int vars_init_readline()
{
	if( !lib_readline && !readline_error && vars_try_readline ){
	  const char *err;
#if defined(__MACH__) || defined(__APPLE_CC__)
		char tlib[]= "libtermcap.dylib";
		char rlib[]= "libreadline.dylib";
#else
		char tlib[]= "libtermcap.so";
		char rlib[]= "libreadline.so";
#endif
		lib_termcap= CX_dlopen( tlib, RTLD_NOW|RTLD_GLOBAL);
		lib_readline= CX_dlopen( rlib, RTLD_NOW|RTLD_GLOBAL);
		if( lib_termcap && lib_readline ){
			gnu_readline= dlsym( lib_readline, "readline");
			gnu_add_history= dlsym( lib_readline, "add_history" );
		}
#ifndef WIN32
		err= dlerror();
#endif
		if( !lib_readline || !gnu_readline || err ){
			fprintf( cx_stderr, "ReadLine: can't load/initialise %s (%s): GNU readline not available.\n",
				rlib, (err)? err : "unknown error"
			);
			gnu_readline= NULL;
			gnu_add_history= NULL;
			if( lib_readline ){
				dlclose( lib_readline );
				lib_readline= NULL;
			}
			if( lib_termcap ){
				dlclose( lib_termcap );
				lib_termcap= NULL;
			}
			readline_error= True;
			return(-1);
		}
		else{
			return(1);
		}
	}
	else{
		return(0);
	}
}
#else
int vars_try_readline= 0;
#endif

char *edit_input( int level, char *prompt, char *buffer, int len, int *nlines, FILE **Fp, FILE *verrfile)
{  char *c, *d, *term= NULL, *_buffer= buffer;
   int loop= 0, ln= 0;
   long oldparposition= parposition, oldparlineposition= parlineposition, size= -1;
   int open_close;
   FILE *fp= *Fp;

	buffer[0]= '\0';
	strncpy( last_prompt, prompt, 7);
	last_prompt[7]= '\0';
#ifdef WIN32
	  /* ftell() seems to be buggy on MVC? */
	open_close= 0;
#else
	open_close= (fp== parfile && parfilename && parfilename[0]!= '|' && !vars_isatty(parfile) &&
		strcmp(parfilename, "stdin") && fp!= stdin )?  True : False;
#endif
	if( open_close && fp== NULL ){
		if( !strcmp( parfilename, "stdin") ){
			fp= fopen( "/dev/tty", "r");
			parfile= fp;
		}
		else if( (fp= fopen(parfilename, "r")) ){
			parfile= fp;
			fseek( parfile, 0, SEEK_END);
			size= ftell( parfile );
			fseek( parfile, parposition, SEEK_SET);
		}
		if( !fp ){
			fprintf( verrfile, "edit_input(\"%s\"): can't reopen input \"%s\" (%s,%s)\n",
				prompt, parfilename, vars_address_symbol(parfilename), vars_address_symbol(parfile)
			);
			return(NULL);
		}
	}
#ifdef VARS_STANDALONE
	else if( !fp ){
	  static char *N="stdin";
		if( parfile ){
			fp= parfile;
		}
		else if( !parfilename || streq(parfilename, "stdin") ){
			fp= stdin;
			parfile= fp;
			if( !parfilename ){
				parfilename= N;
			}
		}
		if( !vars_file ){
			vars_file= fp;
		}
		if( Fp && !*Fp ){
			*Fp= fp;
		}
		open_close= False;
	}
#endif
	if( !fp ){
		fprintf( verrfile, "edit_input(\"%s\"): NULL filePointer passed, and cannot decide to reopen \"%s\" (%s,%s)\n",
			prompt, (parfilename)? parfilename : "<<NULL>>", vars_address_symbol(parfilename), vars_address_symbol(parfile)
		);
		return(NULL);
	}
	do{
		if( edit_variables_list_interim ){
			d= buffer;
			strncpy( _buffer, edit_variables_list_interim, len- strlen(buffer)- 1 );
			edit_variables_list_interim= NULL;
			strcat( _buffer, "\n");
		}
#if defined(unix) || defined(linux) || defined(__MACH__) || defined(__APPLE_CC__)
		if( fp== stdin ){
			vars_init_readline();
		}
		if( gnu_readline && fp== stdin ){
		  _ALLOCA( Prompt, char, strlen(prompt)+ 16, plen );
		  char *rlbuf;
		  static char called= 0;
			if( !called ){
				fprintf( cx_stderr, "## Using GNU ReadLine ##\n" );
				fflush( cx_stderr );
				called+= 1;
			}
			if( level> 0 ){
				sprintf( Prompt, "%d", level );
				strncat( Prompt, prompt, plen- strlen(Prompt) );
			}
			else{
				strcpy( Prompt, prompt );
			}
			if( (rlbuf= (*gnu_readline)(Prompt) ) ){
				strncpy( _buffer, rlbuf, len- strlen(buffer)- 1 );
				  /* gnu readline removes the final newline: put it back */
				strcat( _buffer, "\n" );
				d= _buffer;
			}
			else{
				d= NULL;
			}
			GCA();
		}
		else
#endif
		{
			d= fgets( _buffer, len- strlen(buffer), fp );
		}
		if( !d && errno== EINTR){
		  /* if we receive a signal, don't fall back
		   * to caller
		   */
			d= buffer;
			fprintf( verrfile, "vars::edit_input(): %s (%s)\n",
				serror(), buffer
			);
			Flush_File( verrfile );
			if( level> 0)
				fprintf( stderr, "%d%s%s", level, prompt, buffer );
			else
				fprintf( stderr, "%s%s", prompt, buffer );
			fflush( stderr);
/* 			*d= '\0';	*/
			errno= 0;
		}
		else if( d ){
		  int dlen= strlen(d);
			if( dlen> 1 && *(term= &d[dlen-1])== '\n' && *(c=&d[dlen-2]) == '\\' ){
				*c= '\0';
				term= NULL;
				_buffer= c;
				if( level ){
					fprintf( verrfile, "%d:%d%s", level, ++loop, prompt );
				}
				else{
					fprintf( verrfile, "%d%s", ++loop, prompt );
				}
				Flush_File( verrfile);
			}
			else{
				c= NULL;
			}
		}
		parlineposition+= 1;
		ln+= 1;
	} while( d && c && len- strlen(buffer)> 0 );
	if( term && *term== '\n' ){
		  /* remove the trailing newline for feedback	*/
		*term= '\0';
	}
	else{
		term= NULL;
	}
	*nlines= ln;
	{ int nl= False, echo= vars_isatty(verrfile);
		if( echo && d && !vars_isatty(parfile) ){
			fprintf( verrfile, "%s", buffer );
			nl= True;
		}
		if( open_close ){
			parposition= ftell(parfile);
			if( echo ){
				fprintf( verrfile, " # [\"%s\"@%ld:%ld - %ld:%ld",
					parfilename,
					oldparposition+ 1, oldparlineposition+ 1,
					parposition, parlineposition
				);
				if( size> 0 ){
					fprintf( verrfile, " (%s%%)", d2str(parposition* 100.0/ size, "%g", NULL) );
				}
				fprintf( verrfile, " (lr=%d",
					*nlines
				);
				if( edit_variables_linesread && edit_variables_maxlines> 0 ){
					fprintf( verrfile, "=%d:%d)]\n", *edit_variables_linesread+1, edit_variables_maxlines );
				}
				else{
					fputs( ")]\n", verrfile );
				}
			}
			fclose(parfile);
			parfile= NULL;
			*Fp= NULL;
		}
		else if( nl ){
			fputs( "\n", verrfile );
		}
	}
	if( term ){
		  /* restore the trailing newline	*/
		*term= '\n';
	}
	if( d ){
		Save_VarsHistLine( buffer );
		return( buffer );
	}
	else{
		return( NULL );
	}
}

/* Check buffer from its tail for the existance of
 \ one of the characters in discard. Any occurance
 \ is set to '\0', thus truncating buffer at that
 \ position.
 */
char *discard_after( char *buffer, char *discard)
{  char *c;
	while( *discard ){
		while( (c= rindex( buffer, *discard)) ){
			*c= '\0';
		}
		discard++;
	}
	return( buffer);
}

DEFMETHOD( correct_input, (FILE *input_file, char *input_buffer, FILE *out_file), int)= NULL;

static RET_SHORT _edit_vars_arglist( char *prompt )
{ Variable_t *vars;
  ALLOCA( _buffer, char, VARSEDIT_LENGTH+2,xlen);
  ALLOCA( buffer, char, VARSEDIT_LENGTH+2,xlen2);
  char *bufptr= buffer;
  char *c= (char*)1, *newfile;
  int i, j= 0, check_slash= 1, depth, exit_command= 0, nlines= 0, tlines= 0, *_evlr_ = edit_variables_linesread;
  long changes;
  int stdinput= 0, stdoutput= 0;
  FILE *out= stdout, *vfp= vars_file, *errfp= vars_errfile;
  static int level= -1;
  long n, item= 0;


	if( edit_variables_maxlines > 0 ){
		fprintf( vars_errfile, "%s [Reading %d lines]\n", prompt, edit_variables_maxlines );
	}
	edit_variables_linesread= &tlines;

	for( item= 0; item< vars_arg_items; item++){
		n= vars_arglist[item].n;
		vars= vars_arglist[item].vars;
		check_vars_caller= "edit_vars_arglist()";
		if( check_vars_reset( vars, n, vars_errfile)){
			Flush_File( vars_errfile );
			GCA();
			return( 0);
		}
	}
	fflush( stdout);

	level++;
	PUSH_TRACE("edit_vars_arglist()", depth);

	if( vars_file== stdin){
		if( !gnu_readline ){
			if( level> 0)
				fprintf( stderr, "%d%s", level, prompt);
			else
				fputs( prompt, stderr);
		}
		fflush( stderr);
		fflush( stdout);
	}
	memset( Processed_Buffer, 0, sizeof(Processed_Buffer) );
	  /* Pass a local copy of vars_file to edit_input, since it may close the file, and
	   \ set the filepointer to NULL
	   */
	c= edit_input( level, prompt, _buffer, VARSEDIT_LENGTH, &nlines, &vfp, vars_errfile);
	while( c && strcmp( _buffer, "exit")){
	  char *asterisc;

		discard_after( _buffer, "\n" );
/* discard everything after a NEWLINE, ';', or '#'
		discard_after( _buffer, ";#" );
*/
		while( isspace((unsigned char)*c) ){
			c++;
		}
		if( *c== '#' ){
			*c= '\0';
		}

		if( *c && gnu_add_history && vfp== stdin ){
		  char *cc= &c[ strlen(c)-1 ], C= *cc;
			if( *cc== '\n' ){
				*cc= '\0';
			}
			(*gnu_add_history)( c );
			*cc= C;
		}

		if( !strncmp( _buffer, "!!", 2) ){
		  /* recall previous input-buffer	*/
			if( strlen(&_buffer[2]) ){
			  /* For some reason on HPUX8, the following command appends "!!" to
			   \ buffer if _buffer=="!!" (and hence _buffer[2]=='\0').
			   */
				strncat( buffer, &_buffer[2], VARSEDIT_LENGTH- strlen(buffer) );
			}
			strcpy( _buffer, buffer);
			fprintf( vars_errfile, "%s\n", _buffer);
			Flush_File( vars_errfile);
		}
		else{
			if( (Last_ParsedVariable && Last_ParsedVariable->name) ||
				(Last_ParsedVVariable && Last_ParsedVVariable->name && Last_ParsedVVariable->parent)
			){
				if( !strncmp(_buffer, "*>", 2) || !strncmp( _buffer, "[*>]", 4) ){
					if( strlen(_buffer)-3 + strlen(Last_ParsedVVariable->parent->name) + 2 +
						strlen(Last_ParsedVVariable->name) < VARSEDIT_LENGTH
					){
						asterisc= index( _buffer, '*');
						sprintf( buffer, "%s{", Last_ParsedVVariable->parent->name );
						if( _buffer[0]== '[' ){
							strcat( buffer, "[" );
						}
						strcat( buffer, Last_ParsedVVariable->name);
						if( asterisc[2] )
							strcat( buffer, &asterisc[2]);
						strcat( buffer, "}");
						strcpy( _buffer, buffer);
						if( vars_echo ){
							fprintf( vars_errfile, "%s\n", _buffer);
						}
					}
					else{
						fprintf( vars_errfile, "# Line too long for '%s{%s%s}'\n",
							Last_ParsedVVariable->parent->name,
							Last_ParsedVVariable->name, &_buffer[1]
						);
					}
				}
				else if( _buffer[0]== '*' || !strncmp( _buffer, "[*]", 3) ){
					if( strlen(_buffer)-3 + strlen(Last_ParsedVariable->name) < VARSEDIT_LENGTH){
						asterisc= index( _buffer, '*');
						if( _buffer[0]== '[' ){
							sprintf( buffer, "[" );
						}
						else{
							buffer[0]= '\0';
						}
						strcat( buffer, Last_ParsedVariable->name);
						if( asterisc[1] )
							strcat( buffer, &asterisc[1]);
						strcpy( _buffer, buffer);
						if( vars_echo ){
							fprintf( vars_errfile, "%s\n", _buffer);
						}
					}
					else{
						fprintf( vars_errfile, "# Line too long for '%s%s'\n",
							Last_ParsedVariable->name, &_buffer[1]
						);
					}
				}
				Flush_File( vars_errfile);
			}
			if( (Last_ParsedIVariable && Last_ParsedIVariable->name) ||
				(Last_ParsedIVVariable && Last_ParsedIVVariable->name && Last_ParsedIVVariable->parent)
			){
				if( !strncmp(_buffer, "I*>", 3) ){
					if( strlen(_buffer)-3 + strlen(SET_INTERNAL) + strlen(Last_ParsedIVVariable->parent->name) + 2 +
						strlen(Last_ParsedIVVariable->name) < VARSEDIT_LENGTH
					){
						asterisc= index( _buffer, '*');
						sprintf( buffer, "%s %s{", SET_INTERNAL, Last_ParsedIVVariable->parent->name );
						strcat( buffer, Last_ParsedIVVariable->name);
						if( asterisc[2] )
							strcat( buffer, &asterisc[2]);
						strcat( buffer, "}");
						strcpy( _buffer, buffer);
						if( vars_echo ){
							fprintf( vars_errfile, "%s\n", _buffer);
						}
					}
					else{
						fprintf( vars_errfile, "# Line too long for '%s{%s%s}'\n",
							Last_ParsedIVVariable->parent->name,
							Last_ParsedIVVariable->name, &_buffer[1]
						);
					}
				}
				else if( !strncmp( _buffer, "I*", 2) ){
					if( strlen(_buffer)-3 + strlen(SET_INTERNAL) + strlen(Last_ParsedIVariable->name) < VARSEDIT_LENGTH){
						asterisc= index( _buffer, '*');
						sprintf( buffer, "%s %s", SET_INTERNAL, Last_ParsedIVariable->name);
						if( asterisc[1] )
							strcat( buffer, &asterisc[1]);
						strcpy( _buffer, buffer);
						if( vars_echo ){
							fprintf( vars_errfile, "%s\n", _buffer);
						}
					}
					else{
						fprintf( vars_errfile, "# Line too long for '%s%s'\n",
							Last_ParsedIVariable->name, &_buffer[1]
						);
					}
				}
				Flush_File( vars_errfile);
			}
		}

		buffer[VARSEDIT_LENGTH]= '\0';
		fillstr( buffer, 0);

#ifdef COOKIT
		changes= strlen(_buffer)+ 1;
		cook_string( buffer, _buffer, &changes, NULL);
		if( strcmp( buffer, _buffer) && vars_echo ){
		  /* show cooked result	*/
			fprintf( vars_errfile, "# \"%s\" -> \"%s\"\n",
				_buffer, buffer
			);
			Flush_File( vars_errfile);
		}
#else
		strcpy( buffer, _buffer);
#endif

		if( correct_input 
#ifdef _UNIX_C_
			&& vfp && isatty( fileno(vfp) )
#endif
		){
		  int r;
			while( (r= (*correct_input)( vfp, buffer, vars_errfile )) ){
				if( r== -1 ){
					fprintf( vars_errfile,
						"# input buffer correction (%s) failed - skipping line\n#  (%s)\n",
						address_symbol( correct_input), buffer
					);
					buffer[0]= '\0';
				}
			}
		}

		changes= 0;

		stdinput= stdoutput= 0;
		check_slash= 1;

		  /* remove trailing whitespace	*/
		bufptr= &buffer[ strlen(buffer)-1 ];
		while( isspace((unsigned char) *bufptr) ){
			*bufptr-- = '\0';
		}
		bufptr= buffer;
/* 
		if( !strncmp( bufptr, "Page ", 5) ){
			bufptr+= 5;
			if( strlen(bufptr) && level<= 0 ){
				PIPE_file= Open_Pager( stdout);
				out= PIPE_file;
			}
		}
 */
		if( strlen( bufptr) ){
			/* now take a look at the buffer	*/
	
			if( !strncmp( bufptr, "list", 4)){
			  /* a list of all variables was asked for	*/
			  int N;
			  FILE *output;
				c= &bufptr[4];
				while( *c && !isdigit((unsigned char)*c))
					c++;
				output= Open_Pager( stdout);
				PIPE_fileptr= &output;
				if( strlen(bufptr)== 4){
				  /* no valid trailing arguments: list all	*/
					for( N= 0; N< vars_arg_items; N++ ){
						n= vars_arglist[N].n;
						vars= vars_arglist[N].vars;
						Flush_File( output);
						if( vars_arglist[N].user ){
							if( N){
								fputc( '\n', output);
							}
							fprintf( output, "#\t*** List #%d (%s)[%ld] ***\n\n",
								N, vars_address_symbol(vars), n
							);
							Flush_File( output);
							for( i= 0; i< n; i++){
								print_var( output, &vars[i], list_level);
							}
						}
					}
				}
				else do{
				  int i;
					while( isspace(*c) ){
						c++;
					}
					if( sscanf( c, "%d", &N)== 1 ){
						if( N>= 0 && N< vars_arg_items ){
							n= vars_arglist[N].n;
							vars= vars_arglist[N].vars;
							Flush_File( output);
							fprintf( output, "#\t*** List #%d (%s)[%ld] ***\n",
								N, vars_address_symbol(vars), n
							);
							Flush_File( output);
							for( i= 0; i< n; i++){
								print_var( output, &vars[i], list_level);
							}
						}
					}
					else{
						Flush_File( output);
						fprintf( output, "#\tVariable list #%d doesn't exist\n", N);
						Flush_File( output);
					}
					/* read next number	*/
					while( *c && isdigit((unsigned char)*c) )
						c++;
					while( *c && !isdigit((unsigned char)*c) )
						c++;
				} while( *c);	/* end do	*/
				if( output!= stdout){
					Close_Pager( output);
					PIPE_fileptr= NULL;
				}
			}
			else if( !strcmp( bufptr, "help") || bufptr[0]== '?' ){
				give_evl_help( prompt, vars_errfile);
			}
			else if( !strncmp( bufptr, "stdinput", 8) ){
				if( strlen( bufptr)> 8){
					newfile= &bufptr[8];
					while( isspace((unsigned char)*newfile) ){
						newfile++;
					}
				}
				else
					newfile= NULL;
				stdinput= 1;
				check_slash= 0;
			}
			else if( !strncmp( bufptr, "stdoutput", 9) ){
				if( strlen( bufptr)> 9){
					newfile= &bufptr[9];
					while( isspace((unsigned char)*newfile) ){
						newfile++;
					}
				}
				else
					newfile= NULL;
				stdoutput= 1;
				check_slash= 0;
			}
			else if( !strncmp( bufptr, "terminal", 8) ){
				if( strlen( bufptr)> 8){
					newfile= &bufptr[8];
					while( isspace((unsigned char)*newfile) ){
						newfile++;
					}
				}
				else
					newfile= NULL;
				stdoutput= stdinput= 1;
			}
			else if( !strncmp( bufptr, "evaluate", 4) ){
			  char *c= bufptr;
			  double d[ASCANF_MAX_ARGS];
			  int n= ASCANF_MAX_ARGS;
			  Sinc sinc;
				while( c && *c && !isspace((unsigned char)*c) ){
					c++;
				}
				if( c && *c ){
					fascanf( &n, c, d, NULL );
					fprintf( vars_errfile, "# evaluated %d expressions\n", n );
					if( !ascanf_verbose && n> 0 ){
/* 					  Variable_t V[]= {0, DOUBLE_VAR, 1, ASCANF_MAX_ARGS, (pointer) d, "val", NULL};	*/
					  Variable_t V[]= {0, DOUBLE_VAR, 1, ASCANF_MAX_ARGS, NULL, "val", NULL};
						V[0].maxcount= n;
						V[0].var.d= d;
						fprintf( vars_errfile, "# val= ");
						print_doublevar( Sinc_file( &sinc, vars_errfile, 0L, 0L), V );
						fputc( '\n', vars_errfile);
					}
				}
			}
			else if( bufptr[0]=='/' ){
				fputs( "[Exit (/)]\n", vars_errfile);
				Flush_File( vars_errfile );
				GCA();
				edit_variables_linesread = _evlr_;
				POP_TRACE(depth); level--; return( j);
			}
			else if( bufptr[0]== 0x1a ){
				fputs( "[Exit (^D)]\n", vars_errfile);
				Flush_File( vars_errfile );
				GCA();
				edit_variables_linesread = _evlr_;
				POP_TRACE(depth); level--; return( j);
			}
			else if( !strcmp(bufptr, "exit") ){
				fputs( "[Exit (exit)]\n", vars_errfile);
				Flush_File( vars_errfile );
				GCA();
				edit_variables_linesread = _evlr_;
				POP_TRACE(depth); level--; return( j);
			}
			else if( !stdinput && !stdoutput){
	/* 			if( ((c= index( bufptr, '/')) && c[1]!= '=' && c[-1]!= '\\' && c[-1]!= '^' && check_slash ) )	*/
				if( bufptr[strlen(bufptr)-1] == '/' )
				{
					bufptr[strlen(bufptr)-1]= '\0';
					exit_command= 1;
				}
				j+= parse_varline_with_list( bufptr, vars_arg_items, vars_arglist, &changes, False, &out, &errfp, "edit_vars_arglist()" );
			}
			if( stdinput){
				if( newfile && *newfile ){
					change_stdin( newfile, vars_errfile);
				}
			}
			if( stdoutput){
				if( newfile && *newfile ){
					change_stdout_stderr( newfile, vars_errfile);
				}
			}
	
			if( exit_command== 1 ){
				fputs( "[Exit (/)].\n", vars_errfile);
				Flush_File( vars_errfile );
				GCA();
				edit_variables_linesread = _evlr_;
				POP_TRACE(depth); level--; return( j);
			}
/* 			if( PIPE_file ){	*/
/* 				Close_Pager( out);	*/
/* 				PIPE_file= NULL;	*/
/* 			}	*/
/* 			out= stdout;	*/
		}
#ifndef VARS_STANDALONE
		Chk_Abort();
#endif
		if( vars_file== stdin){
			if( !gnu_readline ){
				if( level> 0)
					fprintf( stderr, "%d%s", level, prompt);
				else
					fputs( prompt, stderr);
			}
			fflush( stderr);
			fflush( stdout);
		}

		  /* restore the input-buffer	*/
		strcpy( buffer, _buffer);

		*edit_variables_linesread+= nlines;
		if( edit_variables_maxlines> 0 && *edit_variables_linesread>= edit_variables_maxlines ){
		  /* We have reached our quota	*/
			c= NULL;
			VEL_changed= False;
		}
		else{
		  /* get a new input-buffer	*/

			if( VEL_changed> 0 ){
				VEL_changed= -1;
				  /* We can't continue reading in this invocation, since a change in buffer length
				   \ was requested. Therefore, do some minimal re-initialisations, continue reading
				   \ in a recursive call, and then exit via the normal cleaning-up-after-you're-done
				   \ lines.
				   */
				vars_file= vfp;
				fprintf( vars_errfile,
					"vars::edit_vars_arglist(): buffer length was changed to %d bytes: taking this into account...\n",
					VARSEDIT_LENGTH
				);
				  /* 20030424: a more elegant solution that may work better for large line-lengths
				   \ is to get called via a wrapper function, that will just call us again when
				   \ the line-length has to change...
				   */
/* 				level-= 1;	*/
/* 				j+= edit_vars_arglist(prompt);	*/
/* 				level+= 1;	*/
				c= NULL;
			}
			else{
				c= edit_input( level, prompt, _buffer, VARSEDIT_LENGTH, &nlines, &vfp, vars_errfile);
			}
		}
	}
	fprintf( vars_errfile, "[Exit(%s)]\n", _buffer);
	Flush_File( vars_errfile );
	GCA();
	edit_variables_linesread = _evlr_;
	POP_TRACE(depth); level--; return( j);
}

RET_SHORT edit_vars_arglist( char *prompt )
{ RET_SHORT r= 0;
	do{
		VEL_changed= 0;
		r+= _edit_vars_arglist(prompt);
	} while( VEL_changed== -1 );
	return(r);
}

/* edit_variables_list(): handle the Variable_t array with <n> elements.
 * Using 'prompt', a buffer is read from stdin, which is then processed.
 * Special:
 *	list: show all variables
 *	help, ?: show help
 *	exit, /, ^Z: exit
 * In all other cases, parse_varline is called with the inputbuffer;
 * Ok is echoed upon a legal input, else Error. The number of legal
 * actions (involving changes) is returned.
 */
#ifndef VARARGS_OLD
RET_SHORT edit_variables_list( char *prompt, long n, ... )
#else
RET_SHORT edit_variables_list( char *prompt, CX_VA_DCL ... )
#endif
{ va_list ap;
#ifdef VARARGS_OLD
  long n;
#endif
  Variable_t *vars;
  RET_SHORT ret;
  static int level= -1;
  int depth;
  long item= 0;


#ifndef VARARGS_OLD
	va_start(ap, n);
#else
	va_start(ap);
	n= va_arg(ap, long);
#endif
	vars_arg_items= 0;
	while( vars_arg_items< MAX_VAR_LISTS && n!= 0 ){
		vars_arglist[vars_arg_items].n= n;
		vars_arglist[vars_arg_items].vars= va_arg( ap, Variable_t*);
		vars_arglist[vars_arg_items].user= True;
		vars_arg_items++;
		item++;
		n= va_arg(ap, long);
	}
	if( vars_arg_items< MAX_VAR_LISTS- 1 && vars_check_internals ){
		vars_arglist[vars_arg_items].n= VARS_INTERNALS;
		vars_arglist[vars_arg_items].vars= (Variable_t*) vars_internals;
		vars_arglist[vars_arg_items].user= False;
		vars_arg_items++;
	}
	if( vars_arg_items>= MAX_VAR_LISTS && n!= 0 ){
	  Variable_t *dum;
		while( (n= va_arg(ap, long)) ){
			dum= va_arg( ap, Variable_t*);
			item++;
		}
		fprintf( vars_errfile, "vars::edit_variables_list(): %ld lists passed: %ld max\n", item, vars_arg_items);
		Flush_File( vars_errfile);
	}
	va_end(ap);

	for( item= 0; item< vars_arg_items; item++){
		n= vars_arglist[item].n;
		vars= vars_arglist[item].vars;
		check_vars_caller= "edit_variables_list()";
		if( check_vars_reset( vars, n, vars_errfile)){
			Flush_File( vars_errfile );
			return( 0);
		}
	}
	fflush( stdout);

	level++;
	PUSH_TRACE("edit_variables_list()", depth);
	ret= edit_vars_arglist( prompt);
	POP_TRACE(depth); level--; return( ret);
}

long vars_parse_line( char *bufptr, long *changes )
{ long Changes= 0;
  FILE *out= stdout, *errfp= vars_errfile;
	if( !changes ){
		changes= &Changes;
	}
	return(
		parse_varline_with_list( bufptr, vars_arg_items, vars_arglist, changes, False, &out, &errfp, "vars_parse_line()" )
	);
}

#ifndef VARARGS_OLD
int set_vars_arglist( long n, Variable_t *vars, ... )
#else
int set_vars_arglist( va_alist )
va_dcl
#endif
{ va_list ap;
#ifdef VARARGS_OLD
  long n;
  Variable_t *vars;
#endif
  long item= 0;


#ifndef VARARGS_OLD
	va_start(ap, vars);
#else
	va_start(ap);
	n= va_arg(ap, long);
#endif
	vars_arg_items= 0;
	while( vars_arg_items< MAX_VAR_LISTS && n!= 0 ){
		vars_arglist[vars_arg_items].n= n;
#ifdef VARARGS_OLD
		vars_arglist[vars_arg_items].vars= va_arg( ap, Variable_t*);
#else
		vars_arglist[vars_arg_items].vars= vars;
#endif
		vars_arglist[vars_arg_items].user= True;
		vars_arg_items++;
		item++;
		n= va_arg(ap, long);
	}
	if( vars_arg_items< MAX_VAR_LISTS- 1 && vars_check_internals ){
		vars_arglist[vars_arg_items].n= VARS_INTERNALS;
		vars_arglist[vars_arg_items].vars= (Variable_t*) vars_internals;
		vars_arglist[vars_arg_items].user= False;
		vars_arg_items++;
	}
	if( vars_arg_items>= MAX_VAR_LISTS && n!= 0 ){
	  Variable_t *dum;
		while( (n= va_arg(ap, long)) ){
			dum= va_arg( ap, Variable_t*);
			item++;
		}
		fprintf( vars_errfile, "vars::set_vars_arglist(): %ld lists passed: %ld max\n", item, vars_arg_items);
		Flush_File( vars_errfile);
	}
	va_end(ap);

	for( item= 0; item< vars_arg_items; item++){
		n= vars_arglist[item].n;
		vars= vars_arglist[item].vars;
		check_vars_caller= "set_vars_arglist()";
		if( check_vars_reset( vars, n, vars_errfile)){
			Flush_File( vars_errfile );
			return( 0);
		}
	}
	fflush( stdout);
	return( vars_arg_items );
}

#ifndef VARARGS_OLD
Variable_t *concat_Variables( long *length, long N, ... )
#else
Variable_t *concat_Variables( long *length, CX_VA_DCL ... )
#endif
{ va_list ap;
  long n;
  long Tot= 0;
  int i, j, items= 0;
  Variable_t *vars, *var;

#ifndef VARARGS_OLD
	n= N;
	va_start(ap, N);
#else
	va_start(ap);
	n= va_arg(ap, long);
#endif
	while( n!= 0 ){
		Tot+= n;
		items+= 1;
		var= va_arg( ap, Variable_t*);
		n= va_arg(ap, long);
	}
	va_end(ap);

	if( calloc_error( vars, Variable_t, Tot) ){
		fprintf( cx_stderr, "vars::concat_Variables(): can't allocate memory for %ld items (%s)\n",
			Tot, serror()
		);
		fflush( cx_stderr );
		*length= 0;
		return( NULL );
	}
#ifndef VARARGS_OLD
	n= N;
	va_start(ap, N);
#else
	va_start(ap);
	n= va_arg(ap, long);
#endif
	for( i= 0; i< Tot && n; ){
		var= va_arg( ap, Variable_t*);
		for( j= 0; j< n; j++ ){
			  /* A copy of the original Variable is made here!	*/
			vars[i]= var[j];
			vars[i].id= i;
			i+= 1;
		}
		n= va_arg( ap, long);
	}
	va_end(ap);
	*length= (long) Tot;
	return(vars);
}

/*!
	Dispose a concatenated list of Variables created by concat_Variables .
	Before disposal, catch the most evident pointers to the memory we'll be freeing.
 */
unsigned long dispose_Concatenated_Variables( Variable_t *list, long length, ... )
{ long i, n;
  Variable_t *vars, *var, *endList;
  unsigned long ret = 0;
  va_list ap;
	if( list && length > 0 ){
		endList = &list[length-1];
		va_start(ap, length);
		while( (n = va_arg(ap, long)) && (vars = va_arg(ap, Variable_t*)) ){
			check_vars_reset( vars, n, vars_errfile);
			for( i = 0, var = vars ; i < n ; i++, var++ ){
				if( var->parent >= list && var->parent < endList ){
					var->parent = NULL;
				}
				if( var->old_var >= list && var->old_var < endList ){
					// this is more delicate: we don't actually know what to replace the
					// invalidated old_var with!
					var->old_var = NULL;
				}
			}
		}
		ret = lfree(list);
	}
	return ret;
}

#ifndef VARARGS_OLD
DescribedVariable *concat_Variables_with_description( long *length, Variable_t *Var, ... )
#else
DescribedVariable *concat_Variables_with_description( long *length, CX_VA_DCL ... )
#endif
{ va_list ap;
  long Tot= 0;
  int i;
  Variable_t *var;
  DescribedVariable *vars;
  char *descr;
  double mi, ma;

#ifndef VARARGS_OLD
	var= Var;
	va_start(ap, Var);
#else
	va_start(ap);
	var= va_arg(ap, Variable_t*);
#endif
	while( var!= NULL ){
		Tot+= 1;
		descr= va_arg( ap, char*);
		mi= va_arg( ap, double);
		ma= va_arg( ap, double);
		var= va_arg(ap, Variable_t*);
	}
	va_end(ap);

	if( calloc_error( vars, DescribedVariable, Tot) ){
		fprintf( cx_stderr, "vars::concat_Variables_with_description(): can't allocate memory for %ld items (%s)\n",
			Tot, serror()
		);
		fflush( cx_stderr );
		*length= 0;
		return( NULL );
	}
#ifndef VARARGS_OLD
	var= Var;
	va_start(ap, Var);
#else
	va_start(ap);
	var= va_arg(ap, Variable_t*);
#endif
	for( i= 0; i< Tot && var; i++ ){
		descr= va_arg( ap, char*);
		mi= va_arg( ap, double);
		ma= va_arg( ap, double);
		  /* This doesn't make a copy of the original Variable!!	*/
		vars[i].var= var;
		vars[i].description= (descr)? descr :
			( (var->description)? var->description : var->name);
		vars[i].minval= mi;
		vars[i].maxval= ma;
		var= va_arg(ap, Variable_t*);
	}
	va_end(ap);
	*length= (long) Tot;
	return(vars);
}

#ifndef VARARGS_OLD
DescribedVariable *concat_Variables_with_description2( long *length, DescribedVariable2 *Var, ... )
#else
DescribedVariable *concat_Variables_with_description2( long *length, CX_VA_DCL ... )
#endif
{ va_list ap;
  long n, N= 0;
  int i, j;
  DescribedVariable2 *var;
  DescribedVariable *vars;
  char *descr;

#ifndef VARARGS_OLD
	var= Var;
	va_start(ap, Var);
#else
	va_start(ap);
	var= va_arg( ap, DescribedVariable2*);
#endif
	while( var!= NULL ){
		N+= va_arg( ap, int);
		var= va_arg( ap, DescribedVariable2*);
	}
	va_end(ap);

	if( calloc_error( vars, DescribedVariable, N) ){
		fprintf( cx_stderr, "vars::concat_Variables_with_description(): can't allocate memory for %ld items (%s)\n",
			N, serror()
		);
		fflush( cx_stderr );
		*length= 0;
		return( NULL );
	}
#ifndef VARARGS_OLD
	var= Var;
	va_start(ap, Var);
#else
	va_start(ap);
	var= va_arg( ap, DescribedVariable2*);
#endif
	for( i= 0; i< N && var; ){
		n= va_arg( ap, int);
		  /* This doesn't make copies of the original Variable!!	*/
		for( j= 0; j< n; j++ ){
		  Variable_t *v;
			v= &var[j].var[var[j].id];
			if( v->old_var && v->old_var->old_id== var[j].id ){
			  /* This means that the list (var[j].var) has been sorted,
			   \ and the original order was preserved.
			   */
				v= v->old_var;
			}
			descr= var[j].description;
			vars[i].var= v;
			vars[i].description= (descr)? descr :
				( (v->description)? v->description : v->name);
			vars[i].default_args= var[j].default_args;
			vars[i].minval= var[j].minval;
			vars[i].maxval= var[j].maxval;
			i++;
		}
		var= va_arg( ap, DescribedVariable2*);
	}
	va_end(ap);
	*length= (long) N;
	return(vars);
}

#if defined(VARIABLE_SYMBOL) && (!defined(VARS_STANDALONE) || defined(SYMBOLTABLE))

VariableChange_method SymbolValue_change_handler= NULL;
int Set_SymbolValue_Verbose= False;

#	ifdef __STDC__
Variable_t *Set_SymbolValue( pointer x, int type, unsigned long n_items, char *items)
#	else
Variable_t *Set_SymbolValue( x, type, n_items, items)
  pointer x;
  int type;
  unsigned long n_items;
  char *items;
#	endif
{  SymbolTable *symbol;
   Variable_t *var= NULL;
   char *vars= NULL;
   int ok;
#ifdef VARS_STANDALONE
   char logmsg[1024];
#endif

	if( type< 0 || type>= COMMAND_VAR ){
		Writelog(
			(logmsg, "Set_SymbolValue(): Illegal type for value of symbol (%s)\n",
				vars_Find_Symbol(x)
			), logmsg
		);
		Symbol_Timing();
		symbol_level--;
		return(NULL);
	}
	if( !Symbol_PrintValue){
		Symbol_PrintValue= (Symbol_PrintValue_method) fprint_var;
	}
	if( (symbol= _Get_Symbol(x,  0, _nothing, 0)) ){
	   int len= (int)(n_items* sizeofVariable[type]);
		Symbol_Timing();
		symbol_level++;
		vars= NULL;
		if( (var= (Variable_t*) symbol->value) ){
			if( check_vars( var, -1L, logfp) ){
				symbol->value= NULL;
				var= NULL;
			}
		}
		if( var){
		  /* dispose of the old variable contents;
		   * keep the variable itself!
		   */
			if( n_items> ((int)(var->maxcount* sizeofVariable[var->type])) ){
			  /* the present arena is too small	*/
				if( var->var.ptr && var->var.ptr!= symbol->adr )
					free( var->var.ptr);
				var->var.ptr= NULL;
			}
			else if( var->var.ptr){
				if( var->var.ptr!= symbol->adr){
				  /* re-use this arena - re-initialise to 0	*/
					memset( var->var.ptr, 0, (var->maxcount* sizeofVariable[var->type]) );
					vars= (char*)var->var.ptr;
				}
			}
		}
		else{
		  /* allocate new variable	*/
			  var= (Variable_t*) calloc( 1L, (unsigned long)sizeof(Variable_t) );
		}

		if( !items || var->var.ptr== symbol->adr ){
		  /* user wants to be able to display the current value of the variable.  */
			len= 0;
		}

		  /* now check if we have a variable, and if we
		   * can get space for the contents of it. If empty
		   * contents (len==0), don't yell, but just create empty
		   * variable
		   */
		if( len> 0 && !vars ){
			ok= !calloc_error( vars, char, len);
		}
		else{
			ok= 1;
		}
		if( !var || !ok
/* 			((!vars && len>0)?calloc_error( vars, char, len):0)	*/
		){
			Writelog(
				(logmsg, "Set_SymbolValue(): no memory (%d bytes) for value of symbol (%s)\n",
					len, symbol->symbol
				), logmsg
			);
			Symbol_Timing();
			symbol_level--;
			return(NULL);
		}

		  /* now initialise the Variable	*/
		var->id= 0;
		var->name= symbol->symbol;
		var->change_handler= SymbolValue_change_handler;
		var->access= 0;
		if( vars){
			if( items){
				memcpy( vars, items, len);
			}
			var_set_Variable( var, type, (pointer) vars, n_items, n_items);
			if( Set_SymbolValue_Verbose ){
				Writelog(
					(logmsg, "Set_SymbolValue(): \"%s\" set to value of %s\n",
						symbol->symbol, address_symbol(items)
					), logmsg
				);
			}
		}
		else if( len== 0 && !items ){
			vars= items= (char*) x;
			var_set_Variable( var, type, (pointer) vars, n_items, n_items);
			if( Set_SymbolValue_Verbose ){
				Writelog(
					(logmsg, "Set_SymbolValue(): \"%s\" set to value of %s (itself)\n",
						symbol->symbol, address_symbol(x)
					), logmsg
				);
			}
		}

		Symbol_Timing();
		symbol_level--;
		if( logfp){
			check_vars_caller= "Set_SymbolValue()";
			if( !check_vars_reset( var, -1L, logfp) )
				return( (Variable_t*) (symbol->value= (_Variable*) var) );
			else
				return( NULL);
		}
		  /* hmmm. Let's hope for the best for now	*/
		return( (Variable_t*) (symbol->value= (_Variable*) var) );
	}
	else
		return( (Variable_t*)NULL );
}

#	ifdef __STDC__
SymbolTable *Add_VariableSymbolValue( char *name, pointer ptr, int size, ObjectTypes obtype, int log,\
	int type, unsigned long items, char *value )
#	else
SymbolTable *Add_VariableSymbolValue( name, ptr, size, obtype, log, type, items, value)
  char *name;
  pointer ptr;
  ObjectTypes obtype;
  int size, log, type;
  unsigned long items;
  char *value;
#	endif
{  SymbolTable *new;
	if( (new= Add_Symbol( name, (ULONG)ptr, size, obtype, log, "Add_VariableSymbolValue")) ){
		new->flag|= VARIABLE_SYMBOL;
		Set_SymbolValue( ptr, type, items, value);
		return( new);
	}
	return( NULL);
}

#	ifdef __STDC__
Variable_t *Get_SymbolValue(pointer x, int tag)
#	else
Variable_t *Get_SymbolValue(x, tag)
  pointer x;
  int tag;
#	endif
{  SymbolTable *symbol= Get_Symbol(x, tag);
	if( symbol)
		return( (Variable_t*) symbol->value);
	else
		return( NULL);
}

SymbolTable_List VarsV_List[]= {
	FUNCTION_SYMBOL_ENTRY( Add_VariableSymbolValue),
	FUNCTION_SYMBOL_ENTRY( Close_Pager),
	FUNCTION_SYMBOL_ENTRY( Get_SymbolValue),
	FUNCTION_SYMBOL_ENTRY( Open_Pager),
#ifndef WIN32
	FUNCTION_SYMBOL_ENTRY( PIPE_handler),
#endif
	FUNCTION_SYMBOL_ENTRY( Set_SymbolValue),
	FUNCTION_SYMBOL_ENTRY( Sprint_var),
	FUNCTION_SYMBOL_ENTRY( Sprint_varMean),
	FUNCTION_SYMBOL_ENTRY( _PagerResult),
	FUNCTION_SYMBOL_ENTRY( _check_changed_field),
	FUNCTION_SYMBOL_ENTRY( _nicer),
	FUNCTION_SYMBOL_ENTRY( _parse_varline),
	FUNCTION_SYMBOL_ENTRY( _perror),
	FUNCTION_SYMBOL_ENTRY( _show_ascanf_functions),
	FUNCTION_SYMBOL_ENTRY( _show_ref_vars),
	FUNCTION_SYMBOL_ENTRY( _suspend_ourselves),
	FUNCTION_SYMBOL_ENTRY( _vcc),
	FUNCTION_SYMBOL_ENTRY( _vps),
	FUNCTION_SYMBOL_ENTRY( add_var),
	FUNCTION_SYMBOL_ENTRY( alpha_var_sort),
	FUNCTION_SYMBOL_ENTRY( check_vars),
	FUNCTION_SYMBOL_ENTRY( check_vars_reset),
	FUNCTION_SYMBOL_ENTRY( cook_string),
	FUNCTION_SYMBOL_ENTRY( divide_var),
	FUNCTION_SYMBOL_ENTRY( do_command_var),
	FUNCTION_SYMBOL_ENTRY( edit_input),
	FUNCTION_SYMBOL_ENTRY( edit_vars_arglist),
	FUNCTION_SYMBOL_ENTRY( edit_variables_list),
	FUNCTION_SYMBOL_ENTRY( set_vars_arglist),
	FUNCTION_SYMBOL_ENTRY( find_named_VariableSelection),
	FUNCTION_SYMBOL_ENTRY( find_named_var),
	FUNCTION_SYMBOL_ENTRY( find_next_named_var),
	FUNCTION_SYMBOL_ENTRY( find_var),
	FUNCTION_SYMBOL_ENTRY( fprint_var),
	FUNCTION_SYMBOL_ENTRY( fprint_varMean),
	FUNCTION_SYMBOL_ENTRY( give_evl_help),
	FUNCTION_SYMBOL_ENTRY( multiply_var),
	FUNCTION_SYMBOL_ENTRY( parse_operator),
	FUNCTION_SYMBOL_ENTRY( parse_varline),
	FUNCTION_SYMBOL_ENTRY( parse_varline_with_list),
	FUNCTION_SYMBOL_ENTRY( parse_varlist_line),
	FUNCTION_SYMBOL_ENTRY( power_var),
	FUNCTION_SYMBOL_ENTRY( print_charvar),
	FUNCTION_SYMBOL_ENTRY( print_doublevar),
	FUNCTION_SYMBOL_ENTRY( print_hexvar),
	FUNCTION_SYMBOL_ENTRY( print_intvar),
	FUNCTION_SYMBOL_ENTRY( print_longvar),
	FUNCTION_SYMBOL_ENTRY( print_shortvar),
	FUNCTION_SYMBOL_ENTRY( print_var),
	FUNCTION_SYMBOL_ENTRY( print_varMean),
	FUNCTION_SYMBOL_ENTRY( Sprint_array),
	FUNCTION_SYMBOL_ENTRY( fprint_array),
	FUNCTION_SYMBOL_ENTRY( sprint_array),
	FUNCTION_SYMBOL_ENTRY( set_var),
	FUNCTION_SYMBOL_ENTRY( sort_variables_list_alpha),
	FUNCTION_SYMBOL_ENTRY( sprint_var),
	FUNCTION_SYMBOL_ENTRY( sprint_varMean),
	FUNCTION_SYMBOL_ENTRY( subtract_var),
	FUNCTION_SYMBOL_ENTRY( var_ChangedString),
	FUNCTION_SYMBOL_ENTRY( var_Mean),
	FUNCTION_SYMBOL_ENTRY( var_Value),
	FUNCTION_SYMBOL_ENTRY( var_accessed),
	FUNCTION_SYMBOL_ENTRY( var_changed_flag),
	FUNCTION_SYMBOL_ENTRY( var_set_Variable),
	FUNCTION_SYMBOL_ENTRY( var_syntax_ok),
	FUNCTION_SYMBOL_ENTRY( vars_Find_Symbol),
	FUNCTION_SYMBOL_ENTRY( vars_address_symbol),
	FUNCTION_SYMBOL_ENTRY( vars_include_file),
	FUNCTION_SYMBOL_ENTRY( vars_do_include_file),
	FUNCTION_SYMBOL_ENTRY( alloc_exit_lines),
	FUNCTION_SYMBOL_ENTRY( add_exit_line),
	FUNCTION_SYMBOL_ENTRY( do_vars_exit_lines),
	FUNCTION_SYMBOL_ENTRY( add_watch_variable),
	FUNCTION_SYMBOL_ENTRY( add_watch_command),
	FUNCTION_SYMBOL_ENTRY( delete_watch_variable),
	FUNCTION_SYMBOL_ENTRY( show_watch_variables),
	FUNCTION_SYMBOL_ENTRY( _log_symboltable),
	FUNCTION_SYMBOL_ENTRY( _do_Find_NamedSymbol),
	FUNCTION_SYMBOL_ENTRY( _seed48),
	FUNCTION_SYMBOL_ENTRY( IO),
	P_TO_VARs_SYMBOL_ENTRY( vars_internals, _variable, sizeof(vars_internals)/sizeof(Variable)),
};
int VarsV_List_Length= SymbolTable_ListLength(VarsV_List);

register_V_fnc()
{
	Add_SymbolTable_List( VarsV_List, VarsV_List_Length );
}

#endif /* VARIABLE_SYMBOL	*/

int do_vars_exit_lines(int val)
{  int i;
   long changes= 0;
	if( VarsExitLines[2] ){
		fprintf( vars_errfile, "vars::do_vars_exit_lines(%d): using the %d following Variable_t lists:\n",
			val, vars_arg_items
		);
		for( i= 0; i< vars_arg_items; i++ ){
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
			fprintf( vars_errfile, "\"%s\"[%d] ",
				Find_Symbol( vars_arglist[i].vars ), 
				vars_arglist[i].n
			);
#else
			fprintf( vars_errfile, "0x%ld[%d] ",
				vars_arglist[i].vars, vars_arglist[i].n
			);
#endif
		}
		fputc( '\n', vars_errfile);
	}
	for( i= 0; i< VarsExitLines[2]; i++ ){
	  FILE *stdo= stdout, *errfp= vars_errfile;
		fprintf( vars_errfile, "%s%s\n", last_prompt, VarsExitLine[i]);
		Flush_File( vars_errfile);
		parse_varline_with_list( VarsExitLine[i],
			vars_arg_items, vars_arglist, &changes, 0,
			&stdo, &errfp, "vars::do_vars_exit_lines()"
		);
	}
	return( (int) changes );
}

