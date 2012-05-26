/* ********************* The Symbol Table facilities ************************* */

#define LABELSIZE 128
#define FUNCTION_SYMBOL	0x01
#define VARIABLE_SYMBOL	0x02
#define SYNONYM_SYMBOL	0x04
#define UNKNOWN			"* ??? *"

/* Definition for an entry in the Symbol Table. The obtype field
 * specifies the data type the adr field points to. No distinction
 * is made between an instance of a type , and a pointer to an instance
 * of a type (e.g. an array of Paddlers is a _paddler just like a Paddler
 * is a _paddler). [Always leave some responsibility to the user...]
 */
 /* 950425: added a size field, which is meant to specify the number of
  \ elements pointed to by the adr field.
  */
#ifdef REUSE_FOR_SYNO
typedef struct SymbolTable{
	struct SymbolTable *cdr;
	char log_it, flag;
	int size;
	ObjectTypes obtype;
	pointer adr;
	long hash;
	long Index, item;
	unsigned long linkcount;	/* for xlisp	*/
	struct _Variable *value;
	char *description;
	int symbol_len;
	char symbol[1];
} SymbolTable;
	#define AddrTable SymbolTable

typedef struct syno{
	struct SymbolTable *cdr;
	char log_it, flag;
	int size;
	ObjectTypes obtype;
	pointer adr;			/* the synonym-pointer	*/
	pointer syno_adr;		/* the pointer that was already in the list (a copy of synonym->adr)	*/
	unsigned Index, item;
	unsigned long linkcount;
	struct _Variable *value;
	char *description;
	int symbol_len;
	SymbolTable *synonym;		/* a pointer to the entry containing the parent-synonym	*/
} SymbolSynonym;

#else

#	define SYMBOSYNOTAB struct SymbolTable *cdr;\
	char log_it, flag;\
	int size;\
	ObjectTypes obtype;\
	pointer adr, syno_adr;\
	long hash;\
	long Index, item;\
	unsigned long linkcount;	/* for xlisp	*/\
	struct _Variable *value;\
	char *description;\
	struct SymbolTable *synonym;\
	int symbol_len;\
	char symbol[1];\

typedef struct SymbolTable{
	SYMBOSYNOTAB
} SymbolTable;
	#define AddrTable SymbolTable

typedef struct syno{
	SYMBOSYNOTAB
} SymbolSynonym;

#endif
	#define AddrSynonym SymbolSynonym

typedef struct SymbolTable_List{
	char *name;
	pointer address;
	ObjectTypes obtype;
	int size, flag;
	int log_it;
	char *description;
} SymbolTable_List;
#define SymbolTable_ListLength(l)	(sizeof(l)/sizeof(SymbolTable_List))

#ifndef STRING
#	define STRING(name)	# name
#endif
#define FUNCTION_SYMBOL_ENTRY(x)	{ # x"()", (pointer) x, _function, 1, FUNCTION_SYMBOL, 0, NULL}
#define P_TO_VAR_SYMBOL_ENTRY(x,type)	{ # x , (pointer) x, type, 1, VARIABLE_SYMBOL, 0, NULL}
#define VARIABLE_SYMBOL_ENTRY(x,type)	{ # x , (pointer) &x, type, 1, VARIABLE_SYMBOL, 0, NULL}
#define FUNCTIONs_SYMBOL_ENTRY(x,n)	{ # x"()", (pointer) x, _function,(int)n, FUNCTION_SYMBOL, 0, NULL}
#define P_TO_VARs_SYMBOL_ENTRY(x,type,n)	{ # x , (pointer) x, type,(int)n, VARIABLE_SYMBOL, 0, NULL}
#define VARIABLEs_SYMBOL_ENTRY(x,type,n)	{ # x , (pointer) &x, type, (int)n, VARIABLE_SYMBOL, 0, NULL}
#define FUNCTION_SYMBOL_ENTRY_DESCR(x,d)	{ # x"()", (pointer) x, _function, 1, FUNCTION_SYMBOL, 0, d}
#define P_TO_VAR_SYMBOL_ENTRY_DESCR(x,type,d)	{ # x , (pointer) x, type, 1, VARIABLE_SYMBOL, 0, d}
#define VARIABLE_SYMBOL_ENTRY_DESCR(x,type,d)	{ # x , (pointer) &x, type, 1, VARIABLE_SYMBOL, 0, d}
#define FUNCTIONs_SYMBOL_ENTRY_DESCR(x,d)	{ # x"()", (pointer) x, _function, (int)n, FUNCTION_SYMBOL, 0, d}
#define P_TO_VARs_SYMBOL_ENTRY_DESCR(x,type,d)	{ # x , (pointer) x, type, (int)n, VARIABLE_SYMBOL, 0, d}
#define VARIABLEs_SYMBOL_ENTRY_DESCR(x,type,d)	{ # x , (pointer) &x, type, (int)n, VARIABLE_SYMBOL, 0, d}


extern long symbol_tab;
extern int Symbol_LabelSize;
extern long CXX_Symbol_Table_mem;
extern double symbol_time, symbol_times;
extern double symbol_lookup, symbol_hit;

extern int symbol_printing;
	#define addr_printing	symbol_printing

#define Symbol_ObjectTypeName(x)	(ObjectTypeNames[Symbol_ObjectType((pointer)(x))])

extern DEFMETHOD( init_new_symbol, (SymbolTable *sym), int);

typedef unsigned long ULONG;

DEFUN( *Cons_SymbolTable, (SymbolTable *entry), SymbolTable );
DEFUN( *Add_Synonym,	(pointer synonym, pointer variable, int size, ObjectTypes obtype, int log),	SymbolSynonym );
DEFUN( *Add_Symbol, ( char *name, size_t ptr, int size, ObjectTypes obtype, int log, char *caller), SymbolTable);
DEFUN( *Add_FunctionSymb,	(char *name, pointer functionpointer, int size, int log_it),	SymbolTable );
DEFUN( *Add_VariableSymbol,	(char *name, pointer variablepointer, int size, ObjectTypes obtype, int log_it),	SymbolTable );
DEFUN( Add_SymbolTable_List,	(SymbolTable_List *list, int len),	int);
DEFUN( Subsitute_SymbolTable_List,	(SymbolTable_List *list, int len),	int);
DEFUN( *Add_SymbolDescription,	(SymbolTable *entry, char *description), SymbolTable);
DEFUN( *Add_SynonymDescription,	(SymbolSynonym *entry, char *description), SymbolSynonym);

		/* add item to symbol table; returns new length */

extern DEFMETHOD( discard_symbol, (SymbolTable *sym), int);
DEFUN( Remove_VariableSymbol,	(pointer address),	int );
DEFUN( Dispose_Symbols,	(),	int );
	#define Dispose_Addresses	Dispose_Symbols
DEFUN( Symbol_Known,	(pointer address),	int );
	#define Address_Known	Symbol_Known
		/* returns 1 if address is in symbol table	*/

  /* @@@@@@@@@@@@@@@@@@@@@@@@@@@ NOT for your uses! @@@@@@@@@@@@@@@@@@@@@@@@@@@@	*/
extern double symbol_times;
extern double symbol_lookup, symbol_hit;
extern int symbol_level;
extern DEFMETHOD( Symbol_PrintValue, (FILE *fp, struct _Variable *var, int extra), short );
typedef DEFMETHOD( Symbol_PrintValue_method, (FILE *fp, struct _Variable *var, int extra), short );

/* hardly timing, just counting: */
#define Symbol_Timing()\
		if(!symbol_level){\
			symbol_times++;\
		}\

  /* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	*/

#define SYMBOL_LOOKUP	1
#define ADDRESS_LOOKUP	0
DEFUN( *Get_Symbol,	(pointer address, int tag),	SymbolTable ); 
		/* find item in address table; return entry associated with it
		 * tag==0 means address is a variable address,
		 * tag==SYM means address points to a string naming a variable
		 */

DEFUN( Find_NamedSymbol, (char *name, ObjectTypes obtype),	pointer);
		/* Find the symbol with name name and type obtype. NULL is
		 * returned when unsuccessfull (or if "NULL" was requested).
		 * The obtype _nothing acts as a wildcard
		 */
DEFUN( *Get_NamedSymbols, (char *name), SymbolTable);
		/* return an entry for all different types with a given name	*/
DEFUN( *Get_Symbols, (pointer adr), SymbolTable);
		/* return an entry for all different types with a given address	*/
DEFUN( Find_NamedSymbols, (char *name), pointer);
		/* returns Get_NamedSymbols(name)->adr	*/
DEFUN( *Find_Symbol,	(pointer address),	char ); 
		/* find item in address table; return label associated with it */
DEFUN( Symbol_ObjectType, (pointer address), ObjectTypes );
		/* return the type of an entry, or _nothing	*/
DEFUN( NamedSymbol_ObjectType, (char *name), ObjectTypes );
DEFUN( *Print_SymbolValues, (FILE *fp, char *prefix, SymbolTable *symbol, char *suffix), SymbolTable);
		
/* return a static string containing the address of p (as [0x????????])
 * if symbol_printing, or else empty
 */
DEFUN( address, (pointer p), char *);
/* return a static string containing the address and symbol name of
 * p (if symbol_printing), or just the symbol name otherwise.
 * Format: [0x????????](???...)
 */
DEFUN( address_symbol, (pointer p), char*);

/* ******************************************************************** */
