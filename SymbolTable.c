/* Symbol Table facilities:	*/

#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#include <stdio.h>
#include "SymbolTable.h"
#include "vars-standalone.h"
#include "varsedit.h"

typedef struct SymbolTableList{
	int Index;
	SymbolTable *Table, *Tail, *Current;
	long elements;
	int result;
} SymbolTableList;
#define SYMBOLTABLE_LISTSIZE	64
#define TableIndex(ptr)	((int)((unsigned long)(ptr)%SYMBOLTABLE_LISTSIZE))

long symbol_tab= 0L;
SymbolTableList CXX_Symbol_Table[SYMBOLTABLE_LISTSIZE];
int Symbol_LabelSize= LABELSIZE;
long CXX_Symbol_Table_mem= 0L;
double symbol_times= 0.0;
double symbol_lookup= 0.0, symbol_hit= 0.0;
int symbol_printing= 1;


#define TAIL_SYMBOL	0x80
char name_of_symbol_null[]= "NULL",
	unknown[16];
int symbol_level= 0;

int _TableIndex(x)
pointer x;
{
	return( TableIndex(x) );
}
	
DEFMETHOD( discard_symbol, (SymbolTable *sym), int);

#	ifdef __STDC__
int Remove_SymbolTable_Entry( SymbolTable *tab)
#	else
int Remove_SymbolTable_Entry( tab)
   SymbolTable *tab;
#	endif
{	SymbolTable *x, *head, *cdr;
	SymbolTableList *listhead;
	_Variable *value;
	int r= 0;
	long m;
	
	if( !tab)
		return(0);
	if( tab->item< 0 || tab->Index< 0){
		return(0);
	}
	Symbol_Timing();
	symbol_level++;

	listhead= &CXX_Symbol_Table[ tab->Index ];
	if( listhead->elements> 0 ){
		  /* Dispose of a SymbolTable entry. Code also in Dispose_Symbols !!	*/
		if( (value= tab->value) ){
			if( value->var.ptr && value->var.ptr!= tab->adr )
				free( value->var.ptr);
			free( value);
		}

		cdr= tab->cdr;
		if( listhead->Current== tab)
			listhead->Current= cdr;
		if( tab== listhead->Table){
			listhead->Table= cdr;
			listhead->Tail->cdr= listhead->Table;
			tab->cdr= NULL;
			if( discard_symbol)
				(*discard_symbol)( tab);
			tab->Index= -1;
			tab->item= -1;
			tab->symbol_len= 0;
			tab->cdr= NULL;
			if( tab->description ){
				lib_free( tab->description );
			}
			if( (m= (long) free( (char*) tab)) ){
				CXX_Symbol_Table_mem-= m;
			}
			listhead->elements--;
			symbol_tab--;
			r= 1;
		}
		else{
			head= listhead->Table;
			x= head->cdr;
			while( x!= tab && x!= listhead->Table ){
				head= x;
				x= x->cdr;
			}
			if( x== tab){
				head->cdr= x->cdr;
				if( x== listhead->Tail )
					listhead->Tail= head;
				x->cdr= NULL;
				if( discard_symbol)
					(*discard_symbol)( x);
				x->Index= -1;
				x->item= -1;
				x->symbol_len= 0;
				x->cdr= NULL;
				if( tab->description ){
					lib_free( tab->description );
				}
				if( (m= (long) free( (char*) x)) ){
					CXX_Symbol_Table_mem-= m;
				}
				listhead->elements--;
				symbol_tab--;
				r= 1;
			}
			else{
				fprintf( cx_stderr, "CXX:Remove_SymbolTable_Entry(): warning: entry 0x%lx (%s) not found!\n",
					tab, tab->symbol
				);
				fflush( cx_stderr );
			}
		}
	}
	if( listhead->elements<= 0){
		listhead->elements= 0;
		listhead->Table= NULL;
		listhead->Current= NULL;
		listhead->Tail= NULL;
	}
	Symbol_Timing();
	symbol_level--;
	return( r);
}

long symbol_hash( name)
char *name;
{  long hash= 0L;
	/* a hashcode based on COMPLETE name	*/
	while( *name){
		hash+= (hash << 1L) ^ *name++;
	}
	return( hash);
}

#define STARTPOINT	listhead->Current

static int _Get_Symbol_regexp= False;
static int _Get_Symbol_list_Index= 0, _Get_Symbol_current_list_Index= 0;
static long _Get_Symbol_item= 0, _Get_Symbol_current_item= 0;

#	ifdef __STDC__
SymbolTable *_Get_Symbol( pointer x, int tag, ObjectTypes type, int remove_emptySyn)
#	else
SymbolTable *_Get_Symbol( x, tag, type, remove_emptySyn)
  pointer x;
  int tag, remove_emptySyn;
  ObjectTypes type;
#	endif
{
	long j= 0L, hash;
	SymbolTableList *listhead;
	SymbolTable *startpoint, *tab= NULL, *res= NULL;
	static pointer prev= NULL;
	static pointer pprev= NULL;
	static SymbolTable *first_sym= NULL;
	int list_Index, x_ok, hit= 0;
	int type_check= False, new_Index= False;
	
	  /* do a double check against an infinite recursion	*/
	if( prev== x || pprev== x){
		return( NULL);
	}
	else{
		pprev= prev;
		prev= x;
	}
	Symbol_Timing();
	symbol_level++;
	_Get_Symbol_regexp= False;
	if( tag== SYMBOL_LOOKUP){
	  char *c= (char *) x;
	  static char *last_name= NULL;
	  static int last_n_len= 0;
		listhead= &CXX_Symbol_Table[_Get_Symbol_list_Index];
		_Get_Symbol_list_Index= 0;
		listhead->result= 0;
		x_ok= (c)? strlen(c) : 0;
		if( x_ok ){
			if( strncmp( c, "RE^", 3)== 0 && c[x_ok-1]== '$' ){
				if( strcmp( c, "RE^$")== 0 ){
					first_sym= NULL;
					Symbol_Timing();
					symbol_level--;
					listhead->Current= NULL;
					return( NULL );
				}
				if( !first_sym || !last_name || strcmp( last_name, &c[2] ) ){
					if( re_comp(&c[2]) ){
						Symbol_Timing();
						symbol_level--;
						fprintf( stderr, "cxx:_Get_Symbol(): can't compile regexp for \"%s\"\n", &c[2]);
						return(NULL);
					}
/* 					if( strlen(&c[2])> last_n_len || !last_name ){	*/
					if( !last_name || x_ok-2> last_n_len ){
						if( last_name ){
							lib_free(last_name);
						}
						last_name= strdup(&c[2]);
						last_n_len= strlen(last_name);
					}
					else{
						strcpy( last_name, &c[2]);
					}
					first_sym= NULL;
					listhead= &CXX_Symbol_Table[0];
					listhead->Current= NULL;
				}
				else{
					j= _Get_Symbol_item;
				}
				_Get_Symbol_item= 0;
				_Get_Symbol_regexp= True;
			}
			else{
				hash= symbol_hash( c);
				  /* non-regexp: resets the memory for the last regexp done.	*/
				if( last_name ){
					last_name[0]= '\0';
				}
			}
		}
		if( type!= _nothing){
			type_check= True;
		}
	}
	else{
		x_ok= (x!=NULL);
		listhead= &CXX_Symbol_Table[ TableIndex(x) ];
		listhead->result= 0;
	}
	list_Index= listhead->Index;
	if( !listhead->Current){
		listhead->Current= listhead->Table;
	}
	else if( listhead->Current->Index!= list_Index){
		listhead->Current= listhead->Table;
	}
	startpoint= STARTPOINT;
	if( x_ok && symbol_tab && startpoint ){
		tab= startpoint;
		do{
		  SymbolTable *cdr;
			if( tag== SYMBOL_LOOKUP){
				if( _Get_Symbol_regexp ){
					if( re_exec(tab->symbol) ){
						if( !type_check || (tab->obtype== type) ){
							symbol_lookup++;
							symbol_hit++;
							hit= 1;
						}
						if( !first_sym ){
							first_sym= tab;
						}
					}
					else{
						hit= 0;
					}
				}
				else if( tab->hash== hash){
					if( !type_check || (tab->obtype== type) ){
						symbol_lookup++;
						if( (hit= (strncmp( tab->symbol, (char*)x, Symbol_LabelSize )== 0) ) ){
							symbol_hit++;
						}
					}
				}
				else{
					hit= 0;
				}
			}
			else{
				hit= (tab->adr == x);
			}
			  /* Take the cdr here, since we may remove <tab> (via stab...) before tab is set to its cdr...
			   \ Systems that don't like access to freed memory will crash on that!
			   */
			cdr= tab->cdr;
			if( hit){
				if( tag== SYMBOL_LOOKUP ){
					res= tab;
				}
				else if( !(tab->flag & SYNONYM_SYMBOL) ){
					res= tab;
				}
				else{
				  SymbolSynonym *stab= (SymbolSynonym*) tab;
					  /* do the best to get a synonym for x	*/
					res= stab->synonym;
					if( remove_emptySyn || !res || res->adr!= stab->syno_adr ){
					  /* no (correct) synonym structure: find the synonym address	*/
						res= _Get_Symbol( stab->syno_adr, 0, _nothing, 0);
					}
					if( res){
					  /* found something: store it so we needn't do it again	*/
						stab->synonym= res;
					}
					else if( remove_emptySyn){
					  /* remove void	*/
#if DEBUG==2
						fprintf( stderr, "cxx::_Get_Symbol(): removing target empty synonym %ld.%ld %s[0x%lx]\n",
							tab->Index, tab->item, tab->symbol, tab->adr
						);
#endif
						Remove_SymbolTable_Entry( (SymbolTable*) stab);
					}
				}
			}		 
			else if( tab && (tab->flag & SYNONYM_SYMBOL) && remove_emptySyn){
			    /* this is a synonym that should be checked to be
			     * valid. Use _Get_Symbol to see if syno_adr is still present;
			     * if not, remove entry.
			     */
			  SymbolSynonym *stab= (SymbolSynonym*) tab;
			  SymbolTable *synonym= _Get_Symbol( stab->syno_adr, 0, _nothing, 0);
				if( !synonym ){
#if DEBUG==2
					fprintf( stderr, "cxx::_Get_Symbol(): removing other empty synonym %ld.%ld %s[0x%lx]\n",
						tab->Index, tab->item, tab->symbol, tab->adr
					);
#endif
					Remove_SymbolTable_Entry( (SymbolTable*) stab);
				}
				else if( stab->synonym!= synonym ){
					stab->synonym= synonym;
				}
			}
			if( tab ){
				tab= cdr;
			}
			j+= 1L;
			if( tag== SYMBOL_LOOKUP){
				if( j>= listhead->elements || tab== startpoint ){
					new_Index= True;
					if( list_Index< SYMBOLTABLE_LISTSIZE-1 ){
						do{
							list_Index++;
							listhead= &CXX_Symbol_Table[list_Index];
							if( !listhead->Current){
								listhead->Current= listhead->Table;
							}
							else if( listhead->Current->Index!= list_Index){
								listhead->Current= listhead->Table;
							}
							startpoint= STARTPOINT;
						} while( !(startpoint || listhead->elements>0) && list_Index< SYMBOLTABLE_LISTSIZE-1 );
						j= 0L;
						tab= startpoint;
						listhead->result= 0;
					}
					else{
						startpoint= NULL;
						tab= NULL;
					}
				}
			}
		}
		while( j< listhead->elements && startpoint && tab && res== NULL && !(j && tab== startpoint) );
	}
	prev= pprev;
	pprev= NULL;
	if( !res){
		if( symbol_tab){
			strcpy( unknown, UNKNOWN);
		}
		else{
			strcpy( unknown, "<no symbols>");
		}
	}
	else{
		CXX_Symbol_Table[list_Index].result= 1;
		  /* In regexp mode, the next call should return the next matching item.
		   \ Else the next call should return the same item (performance consideration)
		   */
		CXX_Symbol_Table[list_Index].Current= (_Get_Symbol_regexp)? tab : res;
		_Get_Symbol_current_list_Index= list_Index;
		_Get_Symbol_current_item= j;
	}
	Symbol_Timing();
	symbol_level--;
	return( res);
}

SymbolTable *Cons_SymbolTable( SymbolTable *entry)
{	SymbolTableList *listhead;
	int list_Index= TableIndex(entry->adr);
	static char called= 0;

	if( !called){
		memset( CXX_Symbol_Table, 0, sizeof( CXX_Symbol_Table) );
	}

	listhead= &CXX_Symbol_Table[list_Index];
	listhead->Index= list_Index;
	entry->Index= list_Index;
	if( listhead->Table ){
		entry->cdr= listhead->Table;
		entry->item= entry->cdr->item+ 1;
	}
	else{
		entry->cdr= entry;
		listhead->Tail= entry;
		entry->item= 0;
		entry->flag|= TAIL_SYMBOL;
	}
	if( !called){
		onexit( Dispose_Symbols, "Dispose_Symbols(CXX)", 0, 1);
		called= 1;
#	define LCALLOC 0x9abcdef0L
#	define LFREED  0xcba90fedL
/* 		Add_VariableSymbol( "LCALLOC magic number", (pointer) LCALLOC, 1, _long, 0);	*/
/* 		Add_VariableSymbol( "LFREED magic number", (pointer) LFREED, 1, _long, 0);	*/
	}
	listhead->Table= entry;
	listhead->Current= entry;
	listhead->Tail->cdr= listhead->Table;
	listhead->elements++;
	symbol_tab+= 1L;
#ifdef __ppc__
	fprintf( cx_stderr, "Adding \"%s\" [0x%p] to table %d (head @%p; %ld elements)\n",
		   entry->symbol, entry->adr, list_Index, &CXX_Symbol_Table[list_Index], listhead->elements
	);
#endif
	return( entry );
}

DEFMETHOD( init_new_symbol, (SymbolTable *sym), int);

/*! cons an item to the address table */
SymbolTable *Add_Symbol( char *name, size_t ptr, int size, ObjectTypes obtype, int log, char * caller)
{
	SymbolTable *dum;
	int i;
	int len;
	unsigned long mlen;
	int ok= True;
	
	if( !name)
		return(0);
	if( !symbol_tab){
		for( i= 0; i< SYMBOLTABLE_LISTSIZE; i++){
			CXX_Symbol_Table[i].Index= i;
		}
	}
#ifdef UNIQUE_TABLE
	if( dum= _Get_Symbol( ptr, 0, _nothing, 0) ){
		if( !strncmp( name, dum->symbol, Symbol_LabelSize) ){
		  /* attempt to add an identical entry: ignore	*/
			return( dum);
		}
	}
#endif
	Symbol_Timing();
	symbol_level++;
	len= MIN( Symbol_LabelSize, strlen(name))+1;
	if( len== 1)
		fprintf( stderr, "Add_Symbol(): adding an empty symbol %s\n", address((pointer)ptr) );
	mlen= (unsigned long) (sizeof(SymbolTable)+len-1);
	if( !(dum= (SymbolTable*) calloc( (unsigned long) 1, mlen)) ){
		fprintf( stderr, "%s(): no room (%lu bytes) for symbol '%s' (%s)\n", 
			caller, mlen, name, serror());
		Symbol_Timing();
		symbol_level--;
		return( NULL );
	}
	CXX_Symbol_Table_mem+= (long) mlen;
	dum->hash= symbol_hash(name);
	strncpy( dum->symbol, name, len-1);
	dum->symbol[len-1]= '\0';
	dum->symbol_len= len-1;
	dum->obtype= obtype;
	dum->adr= (pointer) ptr;
	dum->log_it= (char) log;
	dum->value= NULL;
	dum->description= NULL;
	dum->size= (size> 0)? size : 1;

	if( init_new_symbol)
		ok= (*init_new_symbol)( dum);

	Cons_SymbolTable( dum);

	if( !ok)
		Remove_SymbolTable_Entry( dum );

	Symbol_Timing();
	symbol_level--;
	return( dum );
}

/* cons a synonym to the address table */
SymbolSynonym *Add_Synonym( synonym, ptr, size, obtype, log)
pointer ptr;
pointer synonym;
ObjectTypes obtype;
int size, log;
{
	SymbolSynonym *dum;
	SymbolTable *syno;
	
	/* first check if the synonym exists	*/
	syno= _Get_Symbol( synonym,  0, _nothing, 0);
	while( syno && (syno->flag & SYNONYM_SYMBOL) )
		syno= _Get_Symbol( synonym,  0, _nothing, 0);
	Symbol_Timing();
	symbol_level++;
	if( !syno){
		fprintf( stderr, "Add_Synonym(): can't find the synonym %s for %s\n",
				address_symbol(synonym), address_symbol(ptr)
		);
		return( NULL );
	}
	if( calloc_error( dum, SymbolSynonym, 1)){
			fprintf( stderr, "Add_Synonym(): no room for synonym for '%s' (%s)\n", 
				syno->symbol, serror());
			Symbol_Timing();
			symbol_level--;
			return( NULL );
	}
	CXX_Symbol_Table_mem+= (long) calloc_size;

	dum->flag|= SYNONYM_SYMBOL;
	  /* syno contains all data for ptr/synonym	*/
	  /* syno_adr will contain the address of the final
	   * synonym (no synonyms for synonyms are stored)
	   */
	dum->syno_adr= syno->adr;
	dum->synonym= syno;
	dum->obtype= obtype;
	dum->adr= ptr;
	dum->log_it= (char) log;
	dum->value= NULL;
	dum->description= NULL;
	dum->size= (size> 0)? size : 1;

	  /* now cons the new node to the list (aliasing as a SymbolTable)	*/
	Cons_SymbolTable( (SymbolTable*) dum);
	Symbol_Timing();
	symbol_level--;
	return( dum );
}

#	ifdef __STDC__
SymbolTable *Add_FunctionSymb( char *name, pointer ptr, int size, int log)
#	else
SymbolTable *Add_FunctionSymb( name, ptr, size, log)
  char *name;
  pointer ptr;
  int size, log;
#	endif
{  SymbolTable *new;
	if( (new= Add_Symbol( name, (size_t)ptr, size, _function, log, "Add_FunctionSymbol")) ){
		new->flag|= FUNCTION_SYMBOL;
		return( new);
	}
	else
		return( NULL );
}

#	ifdef __STDC__
SymbolTable *Add_VariableSymbol( char *name, pointer ptr, int size, ObjectTypes obtype, int log)
#	else
SymbolTable *Add_VariableSymbol( name, ptr, size, obtype, log)
  char *name;
  pointer ptr;
  ObjectTypes obtype;
  int size, log;
#	endif
{  SymbolTable *new;
	if( (new= Add_Symbol( name, (size_t)ptr, size, obtype, log, "Add_VariableSymbol")) ){
		new->flag|= VARIABLE_SYMBOL;
		return( new);
	}
	else
		return( NULL );
}

int Add_SymbolTable_List( SymbolTable_List *list, int len)
{  int i;
   SymbolTable *new;
	for( i= 0; i< len; i++, list++){
		if( !(new= Add_Symbol( list->name, (size_t)list->address, list->size, list->obtype, list->log_it,
				"Add_SymbolTable_List"
			))
		){
			return(i);
		}
		else{
			new->flag|= list->flag;
			if( list->description ){
				Add_SymbolDescription( new, list->description );
			}
		}
	}
	return(i);
}

int Substitute_SymbolTable_List_Verbose= False;

int Substitute_SymbolTable_List( SymbolTable_List *list, int len)
{  int i, n;
   SymbolTable *new;
	for( i= 0; i< len; i++, list++){
		n= Remove_VariableSymbol( list->address);
		if( n && Substitute_SymbolTable_List_Verbose ){
			fprintf( stderr,
				"Substitute_SymbolTable_List(): removed %d items for new item #%d \"%s\" [0x%lx] (%s)\n",
					n, i, list->name, list->address, ObjectTypeNames[list->obtype]
			);
		}
		if( !(new= Add_Symbol( list->name, (size_t)list->address, list->size, list->obtype, list->log_it,
				"Substitute_SymbolTable_List"
			))
		){
			return(i);
		}
		else{
			new->flag|= list->flag;
			if( list->description ){
				Add_SymbolDescription( new, list->description );
			}
		}
	}
	return(i);
}

SymbolTable *Add_SymbolDescription(SymbolTable *entry, char *description)
{
	if( entry ){
		if( description ){
			if( (entry->description= strdup(description)) ){
				entry->log_it+= 1;
				CXX_Symbol_Table_mem+= strlen(entry->description);
				return( entry );
			}
		}
		else{
			if( entry->description ){
				if( entry->log_it ){
					entry->log_it-= 1;
				}
				CXX_Symbol_Table_mem-= strlen(entry->description);
				lib_free( entry->description );
			}
			entry->description= NULL;
			return( entry );
		}
	}
	return( NULL );
}

SymbolSynonym *Add_SynonymDescription(SymbolSynonym *entry, char *description)
{
	return( (SymbolSynonym*) Add_SymbolDescription( (SymbolTable*) entry, description) );
}

int Remove_VariableSymbol( x)
pointer x;
{  SymbolTable *at= _Get_Symbol( x, 0, _nothing, 1), *pat= NULL;
   int RA= 1, ret= 0, n= 0;
   char lastname[128];
	
	if( !x){
		return(0);
	}
	lastname[127]= '\0';
	while( at /* && pat!= at */ && RA ){
	  /* remove the found SymbolTable structure	*/
		strncpy( lastname, at->symbol, 126);
		RA= Remove_SymbolTable_Entry( at );
		ret+= RA;
		pat= at;
		at= _Get_Symbol( x, 0, _nothing, 1);
		n++;
	}
	if( n> 2 || (at && pat==at) ){
		fprintf( stderr,
			"Remove_VariableSymbol(): removed %d items for pointer [0x%lx](%s)\n", n, x, lastname
		);
	}
	return( ret);
}

int Dispose_Symbols()
{	SymbolTable *car, *cdr;
	long n= 0L, nn= 0L, N= symbol_tab;
	long mem= CXX_Symbol_Table_mem, m; 
	_Variable *value;
	int Index;
	SimpleStats SS;

	Symbol_Timing();
	symbol_level++;
	SS_Reset_(SS);
	for( Index= 0; Index< SYMBOLTABLE_LISTSIZE; Index++ ){
		for( car= CXX_Symbol_Table[Index].Table, cdr= NULL, n= 0;
			car && CXX_Symbol_Table[Index].elements>= 0 && cdr!= CXX_Symbol_Table[Index].Table;
		){
			cdr= car->cdr;
	
			SS_Add_Data_( SS, 1, car->linkcount, 1.0);
			  /* Dispose of a SymbolTable entry. Code also in Remove_SymbolTable_Entry !!	*/
			if( (value= car->value) ){
				if( value->var.ptr && value->var.ptr!= car->adr ){
					free( value->var.ptr);
				}
				free( value);
			}
			CXX_Symbol_Table[Index].elements--;
			symbol_tab--;
			n++;
			if( discard_symbol){
				(*discard_symbol)( car );
			}
			if( (m= (long) free( car)) ){
				CXX_Symbol_Table_mem-= m;
				nn++;
			}
			else{
				fprintf( stderr,
					"Dispose_Symbols(): corrupt item Index=%d.%ld, 0x%lx ; adr=[0x%lx] symbol=(%s)\n",
						Index, n, car, car->adr, car->symbol
				);
			}
	
			car= cdr;
		}
		CXX_Symbol_Table[Index].Table= NULL;
		CXX_Symbol_Table[Index].Tail= NULL;
		CXX_Symbol_Table[Index].Current= NULL;
		CXX_Symbol_Table[Index].elements= 0;
	}
	Symbol_Timing();
	symbol_level--;
	fprintf( stderr,
		"Dispose_Symbols(): symbol table freed (%ld (%ld)items of %ld items = %ld of %ld bytes left)\n"
		"\tSymbol overhead: %g accesses\n",
		nn, n, N, CXX_Symbol_Table_mem, mem+ sizeof(CXX_Symbol_Table), symbol_times
	);
	if( symbol_lookup){
		fprintf( stderr, "\thash %g out of %g (%g%%)\n",
			symbol_hit, symbol_lookup, symbol_hit/ symbol_lookup* 100.0
		);
	}
	if( SS.sum ){
		fprintf( stderr, "\n\taverage linkcount: %s", SS_sprint_full_(NULL, "%g", " +- ", -2, SS) );
	}
	fputs( "\n", stderr );
	symbol_tab= 0L;
	symbol_times= symbol_hit= symbol_lookup= 0.0;
	memset( CXX_Symbol_Table, 0, sizeof( CXX_Symbol_Table) );
	return( (int) n );
}

void log_SymbolTable_Stats()
{  int Index, used= 0;
	for( Index= 0; Index< SYMBOLTABLE_LISTSIZE; Index++){
		if( CXX_Symbol_Table[Index].elements> 0)
			used++;
	}
	fprintf( stderr,
		"SymbolTable: %ld items = %ld bytes (%d of %d lists used)\n\tSymbol overhead: %g accesses",
		symbol_tab, CXX_Symbol_Table_mem+ sizeof(CXX_Symbol_Table),
		used, Index,
		symbol_times
	);
	if( symbol_lookup){
		fprintf( stderr, " ; hash %g out of %g (%g%%)",
			symbol_hit, symbol_lookup, symbol_hit/ symbol_lookup* 100.0
		);
	}
	fputs( "\n", stderr );
}

int log_all_Symbols= 0;
DEFMETHOD( Symbol_PrintValue, (FILE *fp, struct _Variable *var, int extra), short );

void log_Symbols(FILE *fp)
{
	SymbolTable *cus;
	char template[64];
	int Index;

	fprintf( fp, "Dump of Symbol Table:\n");
	Symbol_Timing();
	symbol_level++;
	for( Index= 0; Index< SYMBOLTABLE_LISTSIZE; Index++){
		cus= CXX_Symbol_Table[Index].Table;
		fprintf( fp, "\n**\tTableIndex %d : %ld items", Index, CXX_Symbol_Table[Index].elements);
		sprintf( template, "\n%%ld\t[0x%%08lx/%%08lx]:\t(%%s)[%%d]\t{%%s}");
		if( symbol_tab && cus){
			do{
				if( log_all_Symbols || cus->log_it){
					if( !(cus->flag & SYNONYM_SYMBOL) ){
						fprintf( fp, template, cus->item, cus->adr, cus->hash, cus->symbol, cus->size,
							ObjectTypeNames[cus->obtype]
						);
						if( cus->linkcount ){
							fprintf( fp, " link=%lu ", cus->linkcount );
						}
						if( Symbol_PrintValue ) if( cus->value){
							fputs( "\n\t\t", fp);
fprintf( fp, "[0x%lx] ", cus->value );
							Flush_File( fp);
							(*Symbol_PrintValue)( fp, cus->value, 1);
						}
					}
					else{
					  SymbolTable *syno;
						if( (syno= ((SymbolSynonym*)cus)->synonym) ){
							fprintf( fp, template,
								cus->item, cus->adr,
								cus->hash, syno->symbol, syno->size, ObjectTypeNames[cus->obtype]
							);
							fprintf( fp, ": synonym for %s {%s}", address_symbol(syno->adr),
								Symbol_ObjectTypeName(syno->adr)
							);
						}
						else{
							fprintf( fp, template, cus->item, cus->adr, cus->symbol, cus->size,
								ObjectTypeNames[cus->obtype]
							);
							fprintf( fp, "::: empty synonym");
						}
					}
					if( cus->description ){
						fprintf( fp, "\n\t\t\"%s\"", cus->description );
					}
				}
				cus= cus->cdr;
			}
			while( cus && cus!= CXX_Symbol_Table[Index].Table);
		}
	}
	fprintf( fp, "\ntotal: %ld entries, %ld bytes\n",
		symbol_tab, CXX_Symbol_Table_mem
	);
	Flush_File( fp);

	  /* reset this flag:	*/
	log_all_Symbols= 0;

	Symbol_Timing();
	symbol_level--;
}

#define STPR(s)	(s->symbol_len>0)?s->symbol:"<empty string>",address(s->adr)

int Check_SymbolTable(fp)
FILE *fp;
{	SymbolTableList *listhead;
	SymbolTable *cus, *sym;
	int Index, errors0= 0, errors1= 0, errors2= 0, errors3= 0,
		_errors0[SYMBOLTABLE_LISTSIZE],
		_errors1[SYMBOLTABLE_LISTSIZE],
		_errors2[SYMBOLTABLE_LISTSIZE],
		_errors3[SYMBOLTABLE_LISTSIZE];

	if( !fp )
		fp= cx_stderr;
	for( Index= 0; Index< SYMBOLTABLE_LISTSIZE; Index++){
		listhead= &CXX_Symbol_Table[Index];
		cus= listhead->Table;
		if( !listhead->Current)
			listhead->Current= listhead->Table;
		_errors0[Index]= 0;
		_errors1[Index]= 0;
		_errors2[Index]= 0;
		_errors3[Index]= 0;
		if( symbol_tab && cus){
			do{
				if( cus->Index!= Index){
					fprintf( fp, "Check_SymbolTable(): Invalid Index %d for item %ld.%ld (%s%s)\n",
						cus->Index, Index, cus->item, STPR(cus)
					);
					errors0++;
					_errors0[Index]++;
				}
				if( cus->adr && !_Get_Symbol( cus->adr, ADDRESS_LOOKUP, cus->obtype, 0) ){
					fprintf( fp, "Check_SymbolTable(): Address LU for item %ld.%ld (%s%s) fails\n",
						cus->Index, cus->item, STPR(cus)
					);
					errors1++;
					_errors1[Index]++;
				}
				if( cus->symbol && cus->symbol_len> 0){
					if( !(sym= _Get_Symbol( cus->symbol, SYMBOL_LOOKUP, cus->obtype, 0)) ){
						fprintf( fp, "Check_SymbolTable(): Symbol LU for item %ld.%ld (%s%s) fails\n",
							cus->Index, cus->item, STPR(cus)
						);
						errors2++;
						_errors2[Index]++;
					}
					else if( strcmp( cus->symbol, sym->symbol) ){
						fprintf( fp, "Check_SymbolTable(): Symbol LU for item %ld.%ld (%s%s) yields\n\titem %ld.%ld (%s%s)\n",
							cus->Index, cus->item, STPR(cus),
							sym->Index, sym->item, STPR(sym)
						);
						errors3++;
						_errors3[Index]++;
					}
				}
				cus= cus->cdr;
			}
			while( cus && cus!= listhead->Table);
		}
	}
	if( errors0 || errors1 || errors2 || errors3){
		fprintf( fp, "Check_SymbolTable(): %d Index errors, %d ALU fails, %d SLU fails, %d SLU errors of %ld items\n",
			errors0, errors1, errors2, errors3, symbol_tab
		);
		for( Index= 0; Index< SYMBOLTABLE_LISTSIZE; Index++){
			if( _errors0[Index] || _errors1[Index] || _errors2[Index] || _errors3[Index] ){
				fprintf( fp, "\tIndex %d[%ld]:  %d Index errors, %d ALU fails, %d SLU fails, %d SLU errors\n",
					Index, CXX_Symbol_Table[Index].elements,
					_errors0[Index], _errors1[Index], _errors2[Index], _errors3[Index]
				);				
			}
		}
	}
	Flush_File( fp);

	return( errors0+ errors1+ errors2+ errors3);
}

int Symbol_Known( x)
pointer x;
{
	if( x== NULL)
		return( 1);				/* NULL we know anyway...	*/
	return( _Get_Symbol(x, 0, _nothing, 0)!=NULL );
}

#	ifdef __STDC__
SymbolTable *Get_Symbol( pointer x, int tag)
#	else
SymbolTable *Get_Symbol( x, tag)
  pointer x;
  int tag;
#	endif
{
	return( _Get_Symbol( x,  tag, _nothing, 0) );
}

pointer Find_NamedSymbol( x, obtype)
char *x;
ObjectTypes obtype;
{	SymbolTable *t;
/* 	static char remark[40];	*/

	if( x== NULL || strlen(x)== 0 || CXX_Symbol_Table== NULL)
		return( NULL);
	if( ( t= _Get_Symbol( x,  SYMBOL_LOOKUP, obtype, 0))== NULL)
		return( NULL);
	else if( obtype== _nothing || t->obtype== obtype)
		return( t->adr);
	else
		return( NULL);
}

SymbolTable *Get_NamedSymbols( x)
char *x;
{	SymbolTable *t;
	static SymbolTable *current= NULL;
	static ObjectTypes obtype= _nothing;
	static char *name= NULL;
	static int cur_index= 0;
	static int regex= False;
	static long item= 0L;

	if( x== NULL || x[0]== '\0' || CXX_Symbol_Table== NULL){
		name= NULL;
		obtype= _nothing+ 1;
		return( NULL);
	}
	if( x!= name){
		name= x;
		obtype= _nothing+ 1;
	}
	  /* regexp searches are done type-unspecific.	*/
	if( regex || (strncmp( x, "RE^", 3)==0 && x[strlen(x)-1]== '$') ){
		if( regex ){
		  /* These values must be restored since they were maybe
		   \ altered by an intervening call to _Get_Symbol()
		   */
			_Get_Symbol_list_Index= cur_index;
			_Get_Symbol_item= item;
			CXX_Symbol_Table[cur_index].Current= current;
		}
		else{
			_Get_Symbol_list_Index= 0;
			_Get_Symbol_item= 0;
		}
		if( ( t= _Get_Symbol( x,  SYMBOL_LOOKUP, _nothing, 0)) ){
			if( !_Get_Symbol_regexp ){
				regex= False;
			}
			else{
				regex= True;
				cur_index= _Get_Symbol_current_list_Index;
				item= _Get_Symbol_current_item;
				current= CXX_Symbol_Table[cur_index].Current;
			}
			return( t);
		}
		else{
			if( _Get_Symbol_regexp ){
			  /* reset regexp mechanism	*/
				_Get_Symbol( "RE^$", SYMBOL_LOOKUP, obtype, 0);
				regex= False;
				cur_index= 0;
				current= NULL;
				item= 0L;
			}
		}
	}
	else{
	  /* non-regexp searches are done type-specifically, yielding all
	   \ symbols that match <x> by doing a lookup with all object-types.
	   */
		while( obtype!= MaxObjectType ){
			_Get_Symbol_list_Index= 0;
			_Get_Symbol_item= 0;
			if( ( t= _Get_Symbol( x,  SYMBOL_LOOKUP, obtype++, 0)) ){
				return( t);
			}
		}
	}
	regex= False;
	cur_index= 0;
	current= NULL;
	item= 0L;
	return( NULL);
}

SymbolTable *Get_Symbols( x)
pointer x;
{	SymbolTable *t;
	static SymbolTable *current= NULL;
	static ObjectTypes obtype= _nothing;
	static char *ptr= NULL;
	static int cur_index= 0;
	static long item= 0L;

	if( x== NULL || CXX_Symbol_Table== NULL){
		ptr= NULL;
		obtype= _nothing+ 1;
		return( NULL);
	}
	if( x!= ptr){
		ptr= x;
		obtype= _nothing+ 1;
	}
	  /* non-regexp searches are done type-specifically, yielding all
	   \ symbols that match <x> by doing a lookup with all object-types.
	   */
	while( obtype!= MaxObjectType ){
		_Get_Symbol_list_Index= 0;
		_Get_Symbol_item= 0;
		if( ( t= _Get_Symbol( x,  ADDRESS_LOOKUP, obtype++, 0)) ){
			return( t);
		}
	}
	cur_index= 0;
	current= NULL;
	item= 0L;
	return( NULL);
}

pointer Find_NamedSymbols( char *x)
{  SymbolTable *tab= Get_NamedSymbols( x);
	if( tab)
		return( tab->adr);
	else
		return( NULL);
}

char *Find_Symbol( pointer x)
{	SymbolTable *t;
	static char remark[50];

	if( x== NULL)
		return( name_of_symbol_null);
	if( CXX_Symbol_Table[ TableIndex(x) ].Table== NULL){
		snprintf( remark, sizeof(remark)-1, "* [0x%08lx] (no symboltable %d) *", x, TableIndex(x) );
		return( remark);
	}
	if( ( t= _Get_Symbol( x,  0, _nothing, 0))== NULL){
		return( unknown);
	}
	else
		return( t->symbol);
}

ObjectTypes Symbol_ObjectType( pointer x)
{	SymbolTable *t;

	if( x== NULL)
		return( _nothing );
	if( CXX_Symbol_Table[ TableIndex(x) ].Table== NULL){
		return( _nothing);
	}
	if( ( t= _Get_Symbol( x,  0, _nothing, 0))== NULL ){
		return( _nothing );
	}
	else{
		if( t->obtype< 0 || t->obtype>= MaxObjectType)
			t->obtype= _nothing;
		return( t->obtype );
	}
}

ObjectTypes NamedSymbol_ObjectType( x)
char *x;
{	SymbolTable *t;

	if( x== NULL || strlen(x)== 0 || CXX_Symbol_Table== NULL )
		return( _nothing );
	if( ( t= _Get_Symbol( x,  SYMBOL_LOOKUP, _nothing, 0))== NULL ){
		return( _nothing );
	}
	else{
		if( t->obtype< 0 || t->obtype>= MaxObjectType)
			t->obtype= _nothing;
		return( t->obtype );
	}
}

char *address( p)
pointer p;
{  static char a[16];
	if( symbol_printing)
		sprintf( a, "[0x%lx]", p);
	else
		a[0]= '\0';
	return(a);
}

char *address_symbol( p)
pointer p;
{  char *symbol= Find_Symbol(p);
   static char *str= NULL;
   static int ls;
	if( !str || ls!= Symbol_LabelSize){
		if( str){
			Remove_VariableSymbol( str);
			Dispose_Disposable( str);
			fprintf( stderr,
				"address_symbol(): Symbol_LabelSize changed from %d to %d: making new buffer\n",
					ls, Symbol_LabelSize
			);
		}
		ls= Symbol_LabelSize;
		if( calloc_error( str, char, 24+ ls) )
			return( symbol );
		Enter_Disposable( str, calloc_size );
		Add_VariableSymbol( "address_symbol() buffer", str, 1, _string, 0);
	}
	switch( symbol_printing){
		case 0:
			sprintf( str, "(%s)", symbol);
			break;
		case 1:
			if( symbol== unknown)
				sprintf( str, "[0x%lx]", p);
			else
				sprintf( str, "(%s)", symbol);
			break;
		case 2:
			sprintf( str, "(%s)[0x%lx]", symbol, p);
			break;
		default:
			break;
	}
	return( str );
}

#ifndef WIN32
#	include <pwd.h>
#	include <grp.h>
#endif

SymbolTable *Print_SymbolValues( FILE *fp, char *prefix, SymbolTable *sym, char *suffix)
{
	if( sym ){
		switch( sym->obtype ){
			case _short:{
			  int i;
			  short *d= (short *)sym->adr;
				fprintf( fp, "%s%s= %d", prefix, (char*) sym->symbol, (int) d[0] );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %d", (int) d[i] );
				}
				fputs( suffix, fp);
				break;
			}
			case _shortP:{
			  int i;
			  short **d= (short **)sym->adr;
				fprintf( fp, "%s%s= %d", prefix, (char*) sym->symbol, (int) *d[0] );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %d", (int) *d[i] );
				}
				fputs( suffix, fp);
				break;
			}
			case _int:{
			  int i;
			  int *d= (int *)sym->adr;
				fprintf( fp, "%s%s= %d", prefix, (char*) sym->symbol, d[0] );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %d", d[i] );
				}
				fputs( suffix, fp);
				break;
			}
			case _intP:{
			  int i;
			  int **d= (int **)sym->adr;
				fprintf( fp, "%s%s= %d", prefix, (char*) sym->symbol, *d[0] );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %d", *d[i] );
				}
				fputs( suffix, fp);
				break;
			}
			case _long:{
			  int i;
			  long *d= (long *)sym->adr;
				fprintf( fp, "%s%s= %ld", prefix, (char*) sym->symbol, d[0] );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %ld", d[i] );
				}
				fputs( suffix, fp);
				break;
			}
			case _longP:{
			  int i;
			  long **d= (long **)sym->adr;
				fprintf( fp, "%s%s= %ld", prefix, (char*) sym->symbol, *d[0] );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %ld", *d[i] );
				}
				fputs( suffix, fp);
				break;
			}
			case _double:{
			  int i;
			  double *d= (double *)sym->adr;
				fprintf( fp, "%s%s= %s", prefix, (char*) sym->symbol, d2str( d[0], "%g", NULL) );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %s", d2str( d[i], "%g", NULL) );
				}
				fputs( suffix, fp);
				break;
			}
			case _doubleP:{
			  int i;
			  double **d= (double **)sym->adr;
				fprintf( fp, "%s%s= %s", prefix, (char*) sym->symbol, d2str( *d[0], "%g", NULL) );
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", %s", d2str( *d[i], "%g", NULL) );
				}
				fputs( suffix, fp);
				break;
			}
			case _function:{
			  pointer d= (pointer)sym->adr;
			  SymbolTable  *t;
				fprintf( fp, "%s%s= 0x%lx", prefix, (char*) sym->symbol, d );
				if( (t= _Get_Symbol( d,  0, _nothing, 0)) ){
					fprintf( fp, " (%s)", t->symbol );
				}
				fputs( suffix, fp);
				break;
			}
			case _functionP:{
			  int i;
			  pointer *d= (pointer)sym->adr;
			  SymbolTable  *t;
				fprintf( fp, "%s%s= 0x%lx", prefix, (char*) sym->symbol, d[0] );
				if( (t= _Get_Symbol( d[0],  0, _nothing, 0)) ){
					fprintf( fp, " (%s)", t->symbol );
				}
				for( i= 1; i< sym->size; i++ ){
					fprintf( fp, ", 0x%lx", d[i] );
					if( (t= _Get_Symbol( d[i],  0, _nothing, 0)) ){
						fprintf( fp, " (%s)", t->symbol );
					}
				}
				fputs( suffix, fp);
				break;
			}
			case _string:
				fprintf( fp, "%s%s= \"%s\"\n", prefix, (char*) sym->symbol, (char*) sym->adr );
				break;
			case _file:{
			  FILE *_fp= (FILE*) sym->adr;
			  struct stat stats;
				Flush_File(_fp);
				if( fstat( fileno(_fp), &stats) ){
					fprintf( fp, "%s%s[fp=0x%lx,fd=%d]: \"%s\"\n", prefix,
						(char*) sym->symbol, _fp, fileno(_fp), serror()
					);
				}
				else{
#ifndef WIN32
				  struct passwd *ui;
				  struct group *gi;
#endif
				  char *size, *sizet;
				  struct tm *tm;
					fprintf( fp, "%s%s[fp=0x%lx,fd=%d]: ", prefix, (char*) sym->symbol, _fp, fileno(_fp) );
					if( stats.st_size> 1024 * 1024 ){
						size= d2str( stats.st_size/(1024.0*1024.0), "%g", NULL);
						sizet= "Mb";
					}
					else if( stats.st_size> 32* 1024 ){
						size= d2str( stats.st_size/1024.0, "%g", NULL);
						sizet= "Kb";
					}
					else{
						size= d2str( (double) stats.st_size, "%g", NULL);
						sizet= "";
					}
					tm= localtime( &stats.st_atime);
					fprintf( fp, "size=%s%s last_access=%02d%02d%02d-%02d:%02d:%02d ",
						size, sizet, tm->tm_year, tm->tm_mon+ 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec
					);
					fflush(fp);
#ifndef WIN32
					setpwent();
					ui= getpwuid( stats.st_uid );
					endpwent();
					setgrent();
					gi= getgrgid( stats.st_gid );
					endgrent();
#endif
					tm= localtime( &stats.st_mtime);
#ifndef WIN32
					if( gi && ui ){
						fprintf( fp, "last_mod=%02d%02d%02d-%02d:%02d:%02d mode=0x%x own=%s,%s attr=0x%x dev_id=%d\n",
							tm->tm_year, tm->tm_mon+ 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
#ifdef hpux
							stats.st_basemode,
#else
							0,
#endif
							ui->pw_name, gi->gr_name,
							stats.st_mode, stats.st_dev
						);
					}
					else
#endif
					{
						fprintf( fp, "last_mod=%02d%02d%02d-%02d:%02d:%02d mode=0x%x own=%d,%d attr=0x%x dev_id=%d\n",
							tm->tm_year, tm->tm_mon+ 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
#ifdef hpux
							stats.st_basemode,
#else
							0,
#endif
							stats.st_uid, stats.st_gid,
							stats.st_mode, stats.st_dev
						);
					}
				}
				fputs( suffix, fp);
				fflush(fp);
				break;
			}
			case _simplestats:{
			  int i;
			  SimpleStats *ss= (SimpleStats*) sym->adr;
			  char buf[512], *nl= "\n";
			    /* _simplestats variable-arrays are preceded by their real name for each item,
			     \ or by the parent symbol's name.
				*/
			  SymbolTable *nsym;
				for( i= 0; i< sym->size; i++ ){
					if( i== sym->size- 1 ){
						nl= "";
					}
					fprintf( fp, "%s%s= %s%s", prefix, (char*) (nsym=Get_Symbol(&ss[i],0))? nsym->symbol : sym->symbol,
						SS_sprint_full( buf, "%g", " +- ", -2.0, &ss[i] ), nl
					);
				}
				fputs( suffix, fp);
				break;
			}
			case _simpleanglestats:{
			  int i;
			  SimpleAngleStats *ss= (SimpleAngleStats*) sym->adr;
			  char buf[512], *nl= "\n";
			  SymbolTable *nsym;
				for( i= 0; i< sym->size; i++ ){
					if( i== sym->size- 1 ){
						nl= "";
					}
					fprintf( fp, "%s%s= %s%s", prefix, (char*) (nsym=Get_Symbol(&ss[i],0))? nsym->symbol : sym->symbol,
						SAS_sprint_full( buf, "%g", " +- ", -2.0, &ss[i] ), nl
					);
				}
				fputs( suffix, fp);
				break;
			}
		}
	}
	return( sym );
}
