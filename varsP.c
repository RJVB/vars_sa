/* vars.c: package for handling
 * variables and parameters in arelatively simple manner
 * (C) R. J. Bertin 1990,1991
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

#include "varsedit.h"

/* #include <values.h>	*/
#include <signal.h>

IDENTIFY( "Parameter handling");

#ifdef _MAC_MPWC_
/* a kludge that remedies the complete-line-oriented input behaviour of
 * the MPW shell. The current cursor position is marked with a NUL byte,
 * after reading the line, everything up to this first NUL byte is
 * discarded.
 */
extern char _chk_abort(char c);

char *_fgets( char *buf, int len, FILE *fp)
{	char *s= buf, *d= buf;

	if( !buf)
		return( NULL);
	if( fp== stdin){
		fflush( stdin);
		fputc( '\0', stdout);		/* mark the current position with a NUL	*/
		fflush( stdout);
		if( !fgets( s, len, stdin))
			return( NULL);
		d= &s[strlen(s)+1];			/* first char AFTER my NUL...	*/
		while( *d){				/* discard upto first NUL	*/
			*s++= _chk_abort(*d++);
		}
		*s= '\0';
		return( buf);
	}
	else
		return( fgets( buf, len, fp) );
}

#endif /* _MAC_MPWC_	*/
		

Parfile_Context parfile_context= {
	NULL,
	MaxParLines, 256, 0,
	NULL, NULL, NULL,
	NULL
};

/* 
 \ int parlinelength= 256,  parlinenumber = MaxParLines;
 \ char **parfilecopy= NULL, *parfiledummy= NULL, *parfiledummy2;
 \ FILE *parfile;
 \ int parlinesread= 0, parlinesread_top= 0;
 \ struct Remember *parbufferkey= NULL;
 */

int parlinesread_top= 0;

int AllocLineBuffer( ptr, linelength, numlines)
char ***ptr;
int linelength, numlines;
{	int i, length;
	char **lb= *ptr;
#ifdef MCH_AMIGA
	unsigned long flags= (unsigned long)(MEMF_PUBLIC|MEMF_CLEAR);
#else
	unsigned long flags= 0L;
#endif
		/* get an array of pointers:	*/
		if( !(lb= CX_AllocRemember( &parbufferkey, 
						(unsigned long)numlines*sizeof(char*), flags
					)
			)
		){
			return( 0);
		}
		length= linelength+ 1;
		/* now setup the pointers in (*ptr):	*/
		for( i= 0; i< numlines; i++){
			if( !(lb[i]= CX_AllocRemember( &parbufferkey,
							(unsigned long)length, flags
						)
				)
			){
				return( 0);
			}
		}
		*ptr= lb;
		return(numlines);
}

/* initialise the buffers used by the Read_parfile functions.	*/
int Init_Parbuffer()
{  static char called = 0;
#ifdef MCH_AMIGA
	unsigned long flags= (unsigned long)(MEMF_PUBLIC|MEMF_CLEAR);
#else
	unsigned long flags= 0L;
#endif
	
	if( !called){
	/* install Free_Parbuffer as exithandler	*/
#ifdef _MAC_THINKC_
		CX_onexit( Free_Parbuffer, "Free_Parbuffer", 0, 1);
#else
		onexit( Free_Parbuffer, "Free_Parbuffer", 0, 1);
#endif
#ifndef VARS_STANDALONE
		register_A_fnc();
		register_P_fnc();
#else
#if defined(SYMBOLTABLE)
		register_A_fnc();
		register_P_fnc();
#endif
		Init_Trace_Stack(64);
#endif
		called++;
	}
		/* allocate the scratch strings:	*/
	if( !parfiledummy){
		errno= 0;
		if( !(parfiledummy= CX_AllocRemember( &parbufferkey, 
							(unsigned long)( parlinelength + 1), flags
						)
			)
		){
			if( !errno)
				errno= EALLOCSCRATCH;
			return( 0);
		}
	}
	if( !parfiledummy2){
		if( !(parfiledummy2= CX_AllocRemember( &parbufferkey, 
							(unsigned long)( parlinelength + 1), flags
						)
			)
		){
			if( !errno)
				errno= EALLOCSCRATCH;
			return( 0);
		}
	}
		/* and the real buffer for the parfile copy	*/
	if( !parfilecopy){
		errno= 0;
		if(  parlinenumber <= 0)
			 parlinenumber = MaxParLines;
		if( !AllocLineBuffer( &parfilecopy,  parlinelength ,  parlinenumber ) ){
			if( !errno)
				errno= EALLOCMEMORY;
			return( 0);
		}
	}
	return(  parlinenumber );
}

int Free_Parbuffer()
{  int freed= 0;
	if( parbufferkey ){
		/* free-up all memory associated with parbufferkey
		 * and the parbufferkey structure:
		 */
		freed= (int) CX_FreeRemember( &parbufferkey, TRUE);
		parfilecopy= NULL;
		parfiledummy= NULL;
		parfiledummy2= NULL;
		parlinenumber = 0;
	}
	return( freed);
}

/* clear the buffers used by the Read_parfile() functions	*/
Clear_Parbuffer()
{	int i, j;
	char *c;
	if( !parfilecopy){
		if( !Init_Parbuffer() ){
			return(0);
		}
	}
	for( i= 0; i<  parlinenumber ; i++){
		for( j= 0, c= parfilecopy[i]; j<  parlinelength ; j++){
			*c++= '\0';
		}
	}
	return( i);
}

/* ========================= Read_parfile functions ===========================	*/

/* copy string s to string d over count-1 bytes.
 * Escape and control sequences are cooked. Copying
 * stops after encountering a comment-token.
 */
long cook_string(d, s, count, do_comments )
char *d, *s;
int *count;
char *do_comments;
{	long K= 0;
	char *c= &s[1];
	
	/* return if NULL pointers, or strlen(s)== 0	*/
	if( !d || !s || !count || !*s)
		return( EOF);
	if( do_comments && index( do_comments, *s) ){
		*s= *d= '\0';
	}
	while( *s && K< *count- 1){
		if( *s== '\\'){
			switch( *c){
				case 't':
					*d++= '\t';
					s++;
					c++;
					break;
				case 'f':
					*d++= '\f';
					s++;
					c++;
					break;
				case 'n':
					*d++= '\n';
					s++;
					c++;
					break;
				case 'r':
					*d++= '\r';
					s++;
					c++;
					break;
				case 'b':
					*d++= '\b';
					s++;
					c++;
					break;
				case '0':
					*d++= '\0';
					s++;
					c++;
					break;
				default:
				  /* also covers \\ , \^ , and \~	*/
					*d++= *c;
					s++;
					c++;
					break;
			}
		}
		else if( *s== '^' && (*c>= '@' && *c<= '_') ){
			*d++= *c- '@';
			s++; c++;
		}
/* 
		else if( *s== '~' && (*c>= '@' && *c<= '_') ){
			*d++= *c- '@' + 128;
			s++; c++;
		}
 */
		else if( do_comments && index( do_comments, *c) ){
		  /* *c (the next character) is a comment token NOT preceded by an escape	*/
			*c= '\0';
			*d++= *s;
		}
		else
			*d++= *s;
		s++; c++; K++;
	}
	*d= '\0';
	*count= (int) K;
	return( K);
}

/* duplicate the current parfile_context, and install a new
 * filepointer and filename in it
 */
Parfile_Context *dup_Parfile_Context( pc, fp, fname)
Parfile_Context *pc;
FILE *fp;
char *fname;
{
	*pc= parfile_context;
	pc->_Parlinesread= 0;
	if( fp){
		pc->_Parfile= fp;
	}
	if( fname ){
		pc->_Parfilename= fname;
	}
	return(pc);
}

DEFUN( find_balanced, (char *string, const char match, const char left_brace, const char right_brace, int *esc), char*);

/* copy x lines from parfile to the parfilecopy buffer */
int Read_parfile( x)
int x;
{
	int i, N, prompt, l, EOI= 0, verbose= 0;
	char *pc;
	int dumlen, depth;
	char *filename;
	static int level= 0;
	
	  /* i is a local var counting the number of lines read by the
	   \ current call and its possible children.
	   */
	i= N= 0;
	parlinesread_top= 0;

	if( !parfile ){
		return(0);
	}
	if( !parfilecopy){
		if( !Init_Parbuffer() ){
			return(0);
		}
	}
	PUSH_TRACE( "Read_parfile()", depth);
	if( x>  parlinenumber ){
		fprintf( cx_stderr, "vars::Read_parfile(): too many lines (%d) requested (max. %d) - excess ignored\n",
			x, parlinenumber
		);
		Flush_File( cx_stderr);
		x=  parlinenumber ;
	}
	fillstr( parfiledummy, '\n');
	if( level== 0 ){
	  /* If this is a toplevel call, we start storing read lines
	   \ in the 0th buffer; else we append.
	   */
		parlinesread= 0;
	}
	if ( ( prompt= (parfile== stdin)) ){
	  extern char *vars_address_symbol();
		printf( "Reading %d lines for parameter input from \"%s\" (%s,%s)\n",
			x, parfilename, vars_address_symbol(parfilename), vars_address_symbol(parfile)
		);
		if( !parfilename ){
			parfilename= "stdin";
		}
		puts( "Terminate input with a '/'.");
	}
	if( ferror(parfile) ){
		perror( "vars::Read_parfile(): Error on parfile");
		POP_TRACE(depth); return( EOF);
	}
	if( feof(parfile) ){
		perror( "vars::Read_parfile(): EOF on parfile");
		POP_TRACE(depth); return( EOF);
	}
	level+= 1;
	while( i< x && !feof( parfile) && !EOI ){
	  char *X;
		if( prompt){
			printf( "%d> ", i); 
			fflush( stdout);
		}
		X= fgets( parfiledummy,  parlinelength -1, parfile);
		N+= 1;
		if( X && parfiledummy[0]!= '%' ){
			while( X && parfiledummy[strlen(parfiledummy)-2] == '\\' ){
			  char *c;
				parfiledummy[strlen(parfiledummy)-2]= '\0';
				X= fgets( parfiledummy2, parlinelength- 1, parfile);
				for( c= parfiledummy2; isspace((unsigned char)*c); c++)
					;
				if( strlen(c)> parlinelength- strlen(parfiledummy)-1 )
					X= NULL;
				strncat( parfiledummy, c, parlinelength- strlen(parfiledummy)-1 );
				parfiledummy[parlinelength-1]= '\0';
				N+= 1;
			}
			if( (parfiledummy[strlen(parfiledummy)-1] != '\n' && X) || (strlen(parfiledummy) && !X) ){
				fprintf( cx_stderr, "vars::Read_parfile(): possibly incomplete line (last %s):\n\t'%s'\n",
					(X)? X : "NULL", parfiledummy
				);
				fflush( stderr);
			}
			if( X== NULL && ferror( parfile) ){
				perror( "vars::Read_parfile(): Error reading parfile");
				break;
			}
			if( strstr( parfiledummy, "VERBOSE")!=NULL){
				verbose= !verbose;
			}
			if( verbose){
				fprintf( cx_stderr, "\tvars::Read_parfile(): raw input = \n%s\n", parfiledummy);
				Flush_File( cx_stderr);
			}
			  /* check for presence of the EndOfInput character
			   \ a '/' not part of "\/", "/=", "^/" or "a/b" (a fraction, with a and b digits).
			   \ Using find_balanced(), the EndOfInput should not occur
			   \ within the matching braces of a Variable argument
			   */
			{ int escaped= False;
				pc= find_balanced( parfiledummy, '/', LEFT_BRACE, RIGHT_BRACE, &escaped );
			}
			EOI= (pc &&
					pc[-1]!= '\\' && pc[1]!= '=' && pc[-1]!= '^' &&
					!(isdigit((unsigned char)pc[-1]) && isdigit((unsigned char)pc[1]))
				)? 1 : 0;
			if( EOI ){
			  /* end of input. Check if it is not otherwise empty	*/
			  char *c= parfiledummy;
				while( isspace((unsigned char)*c) && *c )
					c++;
				if( *c== '/'){
					strcpy( parfiledummy, c);
				}
			}
			strccpy( parfiledummy2, parfiledummy, "\n");
			  /* discard after comment	('#', ';')	*/
			dumlen= strlen( parfiledummy2)+ 1;
			cook_string( parfiledummy, parfiledummy2, &dumlen, "#;" );
			if( verbose){
				fprintf( cx_stderr, "\tstripped result = \n%s\n",
					parfiledummy);
				Flush_File( cx_stderr);
			}
			if( (filename= strstr( parfiledummy, "INCLUDE")) ){
			  FILE *pfp= parfile;
			  char *pfn= parfilename;
				filename+= 7;
				while( isspace((unsigned char)*filename) && *filename ){
					filename++;
				}
				if( *filename ){
				  int len= strlen(filename);
				  char fn[257];
					  /* strip the (possibly) trailing newline	*/
					if( filename[len-1]== '\n' ){
						filename[len-1]= '\0';
					}
					if( (parfile= fopen( filename, "r")) ){
					  int j;
						parfilename= filename;
						if( verbose ){
							strncpy( fn, filename, 256 );
							fprintf( cx_stderr, "\treading %d lines from include-file \"%s\"\n",
								x- i, fn
							);
						}
						  /* just call ourselves with the number of lines remaining to be
						   \ read. This will handle recursive includes.
						   */
						j= Read_parfile( x- i);
						if( verbose ){
							fprintf( cx_stderr, "\tread %d lines from include-file \"%s\"\n",
								j, fn
							);
						}
						  /* parlinesread is already updated (global variable through a macro),
						   \ now update our local copy of the number of lines read.
						   */
						i+= j;
						fclose( parfile );
					}
					else{
						fprintf( cx_stderr, "\tvars::Read_parfile(): can't open include \"%s\"\n",
							filename
						);
						fflush( cx_stderr );
					}
					parfile= pfp;
					parfilename= pfn;
				}
			}
			else{
				if( ( l= strlen( parfiledummy))> 1 && *parfiledummy!= '/' ){
					  /* overwrite the contents in the corresponding buffer
					   * which is only altered for the length of
					   * parfiledummy; the rest is left intact!.
					   */
					stricpy( parfilecopy[parlinesread], parfiledummy);
					if( verbose){
						fprintf( cx_stderr, "\tcooked result[%d,%d] = \n%s\n",
							parlinesread, i,
							parfilecopy[parlinesread]
						);
						Flush_File( cx_stderr);
					}
					i+= 1;
					parlinesread+= 1;
				}
			}
#ifndef VARS_STANDALONE
			Chk_Abort();
#endif
		}
	}
	level-= 1;
	parlinesread_top= N;
	if( x== 0){
		if( ferror( parfile) || feof( parfile)){
			perror( "vars::Read_parfile(): Error: nothing read from parfile");
			POP_TRACE(depth); return( EOF);
		}
	}
	POP_TRACE(depth); return( i);
}

/* VarType_par(s) functions: search in parfilecopy for an occurence
 * of the pattern 's= <val>'. When found, an attempt is made to read
 * <val>, and return it; if unsuccesful, or s is not found, 0 is
 * returned. In both cases, feedback is offered on cx_stderr, in a
 * format that is readable by Read_parfile() (error messages are preceded
 * by a comment token).
 */
double Double_par_default( s, def)
char *s;
double def;
{ 
	double x=  def ;
	int i= 0, len, hit1= 1, hit2= 0, m= 1;
	char mask[ 80], *p;
	
	if( !parfilecopy)
		if( !Init_Parbuffer() )
			return( def );
	/* prepare mask : "<s>="	*/
	strcpy( mask, s);
	strcat( mask, "=");
	len= strlen( mask);
	while( i< parlinesread && hit1!= 0){
		/* check buffer #i	*/
		p= parfilecopy[ i];
		/* search for an occurence of 'mask' with something behind it	*/
		while( *(p+ len) && ( hit1= strncmp( p, mask, len))!= 0)
			p++;
		if( *p && *(p+ len) )
			/* found, now parse the contents immediately behind it
			 * only one double is read (m== 1)
			 */
			hit2= fascanf( &m, p+len, &x, NULL);
		i+= 1;
	}
#ifdef __GNUC__
	{ char format[parlinelength+1];
		sprintf( format, "%s= %%g", s );
		d2str( x, format, parfiledummy2 );
	}
#else
	if( NaN(x) ){
	  unsigned long nan= NaN(x);
		sprintf( parfiledummy2, "%s= NaN%lu", s, (nan & 0x000ff000) >> 12);
	}
	else if( Inf(x)> 0 )
		sprintf( parfiledummy2, "%s= Inf", s);
	else if( Inf(x)< 0 )
		sprintf( parfiledummy2, "%s= -Inf", s);
	else
		sprintf( parfiledummy2, "%s= %g", s, x);
#endif
	fputs( parfiledummy2, cx_stderr);
	if ( hit2== EOF || hit1){
		fprintf( cx_stderr, "\t# default value since not found or invalid format\n");
		errno= ENOSUCHPAR;
	}
	else
	    fputc( '\n', cx_stderr);
	return( x);
}

/* search for s= a,b,c,...,n in parfilecopy
 * if strict== 1, <n> values are requiered,
 * else as many doubles as are present are
 * read. The number read is returned.
 */
int Doubles_par_default( s, a, n, strict, def)		
char *s;
double *a;
int n, strict;
double def;
{
	int i= 0, len, hit1= 1, hit2= 0, m= n;
	char mask[ 80], *p;
	
	if( !parfilecopy)
		if( !Init_Parbuffer() )
			return(def);
	*a= def;
	strcpy( mask, s);
	strcat( mask, "=");
	len= strlen( mask);
	while( i< parlinesread && hit1!= 0){
		p= parfilecopy[ i];
		while( *(p+ len) && ( hit1= strncmp( p, mask, len))!= 0)
			p++;
		if( *p && *(p+ len) )
			hit2= fascanf( &m, p+len, a, NULL);
		i+= 1;
	}
	if ( ( hit2== EOF && m== 0) || hit1){
		fprintf( cx_stderr, "# Parameter '%s' not found or invalid format.\n", s);
		errno= ENOSUCHPAR;
		for( i= 0; i< n; i++ ){
			a[i]= def;
		}
		return( -1);
	}
	else 
		if( strict && hit2== EOF && m< n){
			fprintf( cx_stderr, "# Parameter '%s' has too few elements (%d,%d)\n", s, m, n);
			return( -1);
		}
		else{
			fprintf( cx_stderr, "%s= ", s);
			for( i= 0; i< m-1; i++)
				fprintf( cx_stderr, "%g,", a[i]);
			fprintf( cx_stderr, "%g\n", a[i]);
		}
	return( m);
}

int Int_par_default( s, def) 			/* idem for s= x, x an int */
char *s;
int def;
{
	int x= def;
	int i= 0, len, hit1= 1, hit2= 0, m= 1;
	char mask[ 80], *p;
	
	if( !parfilecopy)
		if( !Init_Parbuffer() )
			return(def);
	strcpy( mask, s);
	strcat( mask, "=");
	len= strlen( mask);
	while( i< parlinesread && hit1!= 0){
		p= parfilecopy[ i];
		while( *(p+ len) && ( hit1= strncmp( p, mask, len))!= 0)
			p++;
		if( *p && *(p+ len) ){
			if( sizeof(int)== 2){
			  /* 16 bits integers	*/
				hit2= dascanf( &m, p+len, (short*) &x, NULL);
			}
			else if( sizeof(int)== 4){
			  /* 32 bits integers	*/
				hit2= lascanf( &m, p+len, (long*) &x, NULL);
			}
		}
		i+= 1;
	}
	sprintf( parfiledummy2, "%s= %d", s, x);
	fputs( parfiledummy2, cx_stderr);
	if ( hit2== EOF || hit1){
		fprintf( cx_stderr, "\t# default value since not found or invalid format\n" );
		errno= ENOSUCHPAR;
	}
	else
		fputc( '\n', cx_stderr);

	return( x);
}

int Ints_par_default( s, a, n, strict, def)		
char *s;
int *a;
int n, strict;
int def;
{
	int i= 0, len, hit1= 1, hit2= 0, m= n;
	char mask[ 80], *p;
	
	if( !parfilecopy)
		if( !Init_Parbuffer() )
			return(def);
	strcpy( mask, s);	
	strcat( mask, "=");
	len= strlen( mask);
	while( i< parlinesread && hit1!= 0){
		p= parfilecopy[ i];
		while( *(p+ len) && ( hit1= strncmp( p, mask, len))!= 0)
			p++;
		if( *p && *(p+ len) ){
			if( sizeof(int)== 2){
				hit2= dascanf( &m, p+len, (short*) a, NULL);
			}
			else if( sizeof(int)== 4){
				hit2= lascanf( &m, p+len, (long*) a, NULL);
			}
		}
		i+= 1;
	}
	if ( ( hit2== EOF && m== 0) || hit1){
		fprintf( cx_stderr, "# Parameter '%s' not found or invalid format.\n", s);
		errno= ENOSUCHPAR;
		return( -1);
	}
	else 
		if( strict && hit2== EOF && m< n){
			fprintf( cx_stderr, "# Parameter '%s' has too few elements (%d,%d)\n", s, m, n);
			return( -1);
		}
		else{
			fprintf( cx_stderr, "%s= ", s);
			for( i= 0; i< m-1; i++)
				fprintf( cx_stderr, "%d,", a[i]);
			fprintf( cx_stderr, "%d\n", a[i]);
		}
	return( m);
}

/* Bool_par(s): check if <s> occurs somewhere in parfilecopy	*/
int Bool_par_default( s, def)
char *s;
int def;
{
	int x= def;
	int i= 0, len, hit1= 1, hit2= 0;
	char mask[ 80], *p;
	
	if( !parfilecopy){
		if( !Init_Parbuffer() ){
			return(0);
		}
	}
	strcpy( mask, s);
	len= strlen( mask);
	while( i< parlinesread && hit1!= 0){
		p= parfilecopy[ i];
		while( *(p+ len- 1) && ( hit1= strncmp( p, mask, len))!= 0){
			p++;
		}
		if( !hit1){
			hit2= 1;
			x= True;
		}
		i+= 1;
	}
	sprintf( parfiledummy2, "%s= %s ", s, (x)? "True" : "False");
	fputs( parfiledummy2, cx_stderr);
	if( hit2== EOF || hit1 ){
		errno= ENOSUCHPAR;
		fprintf( cx_stderr, "(default since parameter not found)\n");
	}
	else{
		fputc( '\n', cx_stderr);
	}

	return( x);
}

/* Str_par(s,t): find <s> in parfilecopy;
 * if found, put the rest of the buffer in 't'
 * else fill t with '\0'
 */
void Str_par_default( s, t, def)
char *s, *t, *def;
{	int i= 0, len, hit1= 1, hit2= 0;
	char mask[ 80], mask2[ 80], *p;
	
	if( !parfilecopy)
		if( !Init_Parbuffer() )
			return;
	strcpy( mask, s);
	strcat( mask, "=");
	strcpy( mask2, mask);
	strcat( mask2, "%s");
	len= strlen( mask);
	while( i< parlinesread && hit1!= 0){
		p= parfilecopy[ i];
		while( *(p+ len) && ( hit1= strncmp( p, mask, len))!= 0)
			p++;
		hit2= sscanf( p, mask2, t);
		i+= 1;
	}
	if ( hit2== EOF || hit1){
		len= strlen(t);
		if( def){
			strncpy( t, def, len );
			t[len]= '\0';
		}
		else
			fillstr( t, '\0');
		sprintf( parfiledummy2, "%s= %s", s, t);
		fprintf( cx_stderr, "%s\t# default string since not found or invalid format\n", parfiledummy2);
		errno= ENOSUCHPAR;
	}
	else{
		sprintf( parfiledummy2, "%s= %s", s, t);
		fprintf( cx_stderr, "%s\n", parfiledummy2);
	}
}

#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
SymbolTable_List VarsP_List[]= {
	FUNCTION_SYMBOL_ENTRY( AllocLineBuffer),
	FUNCTION_SYMBOL_ENTRY( Bool_par_default),
	FUNCTION_SYMBOL_ENTRY( Clear_Parbuffer),
	FUNCTION_SYMBOL_ENTRY( Double_par_default),
	FUNCTION_SYMBOL_ENTRY( Doubles_par_default),
	FUNCTION_SYMBOL_ENTRY( Free_Parbuffer),
	FUNCTION_SYMBOL_ENTRY( Init_Parbuffer),
	FUNCTION_SYMBOL_ENTRY( Int_par_default),
	FUNCTION_SYMBOL_ENTRY( Ints_par_default),
	FUNCTION_SYMBOL_ENTRY( Read_parfile),
	FUNCTION_SYMBOL_ENTRY( Str_par_default),
	FUNCTION_SYMBOL_ENTRY( dup_Parfile_Context),
};
int VarsP_List_Length= SymbolTable_ListLength(VarsP_List);

register_P_fnc()
{
	Add_SymbolTable_List( VarsP_List, VarsP_List_Length );
}
#endif
