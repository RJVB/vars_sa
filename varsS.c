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

IDENTIFY( "Sinc routines");

Sinc *Sinc_string_behaviour( Sinc *sinc, char *string, long cnt, long base, SincString behaviour )
{
	if( !sinc ||
		(!string && behaviour!= SString_Dynamic && !(behaviour== SString_Global && SincString_Behaviour== SString_Dynamic))
	){
		return( NULL);
	}
	  /* 20020313 */
	if( cnt> 0 && !string ){
		string= calloc( cnt, sizeof(char) );
		string[0]= '\0';
	}
	if( (sinc->sinc.string= string) ){
		sinc->_tlen= strlen(string);
		if( cnt<= 0 ){
			cnt= strlen(string);
		}
	}
	else{
		sinc->_tlen= 0;
	}
	sinc->type= SINC_STRING;
	sinc->alloc_len= sinc->_cnt= ( cnt > 0L)? cnt : 0L;
	sinc->_base=  (base > 0L)? base : 0L;
	sinc->behaviour= behaviour;
	return( sinc);
}

Sinc *Sinc_string( Sinc *sinc, char *string, long cnt, long base )
{
	return( Sinc_string_behaviour( sinc, string, cnt, base, SString_Fixed ) );
}

Sinc *Sinc_file( Sinc *sinc, FILE *file, long cnt, long base )
{
	if( !sinc || !file )
		return( NULL);
	sinc->sinc.file= file;
	sinc->type= SINC_FILE;
	sinc->_cnt= sinc->_base=  0L;
	sinc->_tlen= 0;
	return( sinc);
}

Sinc *Sinc_base( Sinc *sinc, long base )
{
	sinc->_base= base;
	return( sinc);
}

int Sflush( Sinc *sinc )
{
	if( !sinc || !sinc->sinc.string ){
		errno= ESINCISNULL;
		return( EOF);
	}
	switch( sinc->type){
		case SINC_STRING:
			sinc->sinc.string[ sinc->_cnt- 1]= '\0';
			sinc->_tlen= strlen(sinc->sinc.string);
			return( 0);
			break;
		case SINC_FILE:
			sinc->_tlen+= sinc->_cnt;
			sinc->_cnt= 0;
			return( Flush_File( sinc->sinc.file) );
			break;
		default:
			errno= EILLSINCTYPE;
			return( EOF);
	}
}

int Srewind( Sinc *sinc )
{
	if( !sinc || !sinc->sinc.string ){
		errno= ESINCISNULL;
		return( EOF);
	}
	switch( sinc->type){
		case SINC_STRING:
			sinc->_cnt= 0;
			sinc->sinc.string[ sinc->_cnt]= '\0';
			sinc->_tlen= strlen(sinc->sinc.string);
			return( 0);
			break;
		case SINC_FILE:
			sinc->_tlen= sinc->_cnt= 0;
			rewind( sinc->sinc.file );
			return( 0 );
			break;
		default:
			errno= EINVAL;
			return( EOF);
	}
}

int SincString_Behaviour= SString_Fixed;

int SincAllowExpansion( Sinc *sinc )
{
	if( !sinc ){
		return(0);
	}
	if( sinc->behaviour== SString_Dynamic || (sinc->behaviour== SString_Global && SincString_Behaviour!= SString_Fixed) ){
		return(1);
	}
	else{
		return(0);
	}
}

int Sputs( char *text, Sinc *sinc )
{  int i= 0, sia= SincAllowExpansion(sinc);
	if( !text)
		return( 0);
	if( !sinc || (!sinc->sinc.string && !sia) ){
		errno= ESINCISNULL;
		return( EOF);
	}
	switch( sinc->type){
		case SINC_STRING:
			if( sinc->_base< 0)
				sinc->_base= 0;
			{ int tlen= strlen(text)+1;
				if( (!sinc->sinc.string || tlen+ sinc->_base> sinc->alloc_len) && sia ){
					tlen+= sinc->_base;
					errno= 0;
					if( sinc->sinc.string && sinc->alloc_len> 1 ){
						sinc->sinc.string= (char*) realloc( sinc->sinc.string, tlen* sizeof(char));
					}
					else{
						if( sinc->sinc.string ){
							lib_free( sinc->sinc.string );
						}
						sinc->sinc.string= (char*) lib_calloc( tlen, sizeof(char));
						sinc->_base= 0;
					}
					sinc->alloc_len= tlen;
				}
				else{
					tlen= sinc->_cnt;
				}
				if( !sinc->sinc.string ){
					if( !errno ){
						errno= ESINCISNULL;
					}
					return(EOF);
				}
				sinc->_cnt= tlen;
			}
			if( sinc->_cnt> 0 ){
			  /* suppose the length indication makes sense: use it.
			   * make sure the string is always terminated
			   */
				sinc->sinc.string[sinc->_cnt-1]= '\0';
				if( sinc->_base < sinc->_cnt- 1 || (sinc->_base== sinc->_cnt-1 && sinc->alloc_len> 1 && sia) ){
				    /* 20020313 */
				  int cnt= (sia)? sinc->alloc_len : sinc->_cnt;
					for( i= 0; sinc->_base < cnt-1 && *text ; sinc->_base++, i++ )
						sinc->sinc.string[sinc->_base]= *text++ ;
					sinc->sinc.string[sinc->_base]= '\0';
					if( sia ){
						sinc->_cnt= strlen( sinc->sinc.string )+ 1;
					}
					return(i);
				}
				else{
					if( sia ){
						sinc->_base= strlen( sinc->sinc.string );
					}
					else{
						sinc->_base= sinc->_cnt;
					}
				}
			}
			else{
			  /* okay, just hope for the best...	*/
				for( i= 0; *text ; sinc->_base++, i++ )
					sinc->sinc.string[sinc->_base]= *text++ ;
				sinc->sinc.string[sinc->_base]= '\0';
				if( sia ){
					sinc->_cnt= strlen( sinc->sinc.string )+ 1;
				}
				return(i);
			}
			return( EOF);
			break;
		case SINC_FILE:{
		  int n= fputs( text, sinc->sinc.file );
			if( n!= EOF ){
				sinc->_cnt+= n;
			}
			return( n );
			break;
		}
		default:
			errno= EILLSINCTYPE;
			return( EOF);
	}
}

Sinc *SSputs( char *text, Sinc *sinc )
{
	if( Sputs( text, sinc)!= EOF )
		return( sinc);
	else
		return( NULL);
}

int Sputc( int c, Sinc *sinc )
{ int sia= SincAllowExpansion(sinc);
	if( !sinc || (!sinc->sinc.string && !sia) ){
		errno= ESINCISNULL;
		return( EOF);
	}
	switch( sinc->type){
		case SINC_STRING:
			if( sinc->_base< 0)
				sinc->_base= 0;
			{ int tlen= 2;
				if( (!sinc->sinc.string || tlen+ sinc->_base> sinc->alloc_len) && sia ){
					tlen+= sinc->_base;
					errno= 0;
					if( sinc->sinc.string && sinc->_cnt> 1 ){
						sinc->sinc.string= realloc( sinc->sinc.string, tlen* sizeof(char));
					}
					else{
						if( sinc->sinc.string ){
							lib_free( sinc->sinc.string );
						}
						sinc->sinc.string= lib_calloc( tlen, sizeof(char));
						sinc->_base= 0;
					}
					sinc->alloc_len= tlen;
				}
				else{
					tlen= sinc->_cnt;
				}
				if( !sinc->sinc.string ){
					if( !errno ){
						errno= ESINCISNULL;
					}
					return(EOF);
				}
				sinc->_cnt= tlen;
			}
			if( sinc->_cnt> 0 ){
			/* suppose the length indication makes sense: use it.
			 * make sure the string is always terminated	*/
				sinc->sinc.string[sinc->_cnt-1]= '\0';
				if( sinc->_base < sinc->_cnt- 1 || (sinc->_base== sinc->_cnt-1 && sinc->alloc_len> 1 && sia) ){
					sinc->sinc.string[ sinc->_base++ ]= c ;
					sinc->sinc.string[sinc->_base]= '\0';
					if( sia ){
						sinc->_cnt= strlen( sinc->sinc.string )+ 1;
					}
					return(c);
				}
				else{
					if( sia ){
						sinc->_base= strlen( sinc->sinc.string );
					}
					else{
						sinc->_base= sinc->_cnt;
					}
				}
			}
			else{
			  /* okay, just hope for the best...	*/
				sinc->sinc.string[ sinc->_base++ ]= c ;
				sinc->sinc.string[sinc->_base]= '\0';
				if( sia ){
					sinc->_cnt= strlen( sinc->sinc.string )+ 1;
				}
				return( c);
			}
			return( EOF );
			break;
		case SINC_FILE:{
		  int n= fputc( c, sinc->sinc.file );
			if( n!= EOF ){
				sinc->_cnt+= 1;
			}
			return( n );
			break;
		}
		default:
			errno= EILLSINCTYPE;
			return( EOF);
	}
}

Sinc *SSputc( int c, Sinc *sinc )
{
	if( Sputc( c, sinc)!= EOF)
		return( sinc);
	else	
		return( NULL);
}

#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
SymbolTable_List VarsS_List[]= {
	FUNCTION_SYMBOL_ENTRY( SSputc),
	FUNCTION_SYMBOL_ENTRY( SSputs),
	FUNCTION_SYMBOL_ENTRY( Sinc_base),
	FUNCTION_SYMBOL_ENTRY( Sinc_file),
	FUNCTION_SYMBOL_ENTRY( Sinc_string),
	FUNCTION_SYMBOL_ENTRY( Sinc_string_behaviour),
	FUNCTION_SYMBOL_ENTRY( SincAllowExpansion),
	FUNCTION_SYMBOL_ENTRY( Sputc),
	FUNCTION_SYMBOL_ENTRY( Sputs),
	VARIABLE_SYMBOL_ENTRY_DESCR( SincString_Behaviour,_int, "Global behaviour switch for the (non)expansion of Sinc strings"),
};
int VarsS_List_Length= SymbolTable_ListLength(VarsS_List);

register_S_fnc()
{
	Add_SymbolTable_List( VarsS_List, VarsS_List_Length );
}
#endif
