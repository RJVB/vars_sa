#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#include "vars-standalone.h"

IDENTIFY( "standalone helper functions" );

#ifndef SYMBOLTABLE
#	define Find_Symbol(x)	"<no symboltable>"
#endif

#define StdErr	stderr

int CX_writable_strings= 0;

char *cx_sys_errlist[]= {					/* sys_nerr+ ...	*/
	 "can't open library"					/* 0	*/
	,"can't connect to Display"				/* 1	*/
	,"can't open window"					/* 2	*/
	,"can't allocate memory"					/* 3	*/
	,"can't allocate scratch memory"			/* 4	*/
	,"attempt to lfree() an alien pointer"		/* 5	*/
	,"function not (yet) available"			/* 6	*/
	,"NULL Sinc pointer or Sinc->sinc field"	/* 7	*/
	,"illegal Sinc type"					/* 8	*/
	,"xterm won't open"						/* 9	*/
	,"alien system error number"				/* 10	*/
	,"no such parameter in parfilecopy"		/* 11	*/
	,"Variable with illegal id found"
	,"Variable with NULL name found"
	,"Variable with illegal type found"
	,"COMMAND Variable without method found"
	,"Variable with NULL variable found"
	,"no accessible Variable with such name"
	,"illegal Variable syntax"
	,"Variable is read-only"
	,"divide by zero"
	,"index out of range"
	,"attempt to free again"
};
int cx_sys_nerr= sizeof(cx_sys_errlist)/sizeof(char*);
int sys_errno= 0;

#ifndef SYMBOLTABLE
char *address_symbol(pointer x)
{ static char hex[32];
	sprintf( hex, "0x%lx", x );
	return(hex);
}
#endif

/* Disposable list facilities:	*/

Disposable *LaundryBasket= NULL;
long DisposableVolume= 0L;

char *RootPointer_Failure[ROOTPOINTER_FAILURES]= {
	 "OK"
	,"NULL pointer or (Max)Size<=0"
	,"no terminator after user space"
	,"address of user space not after terminator or invalid UserMem"
	,"(Max)ItemSize<=0"
};


Disposable *Find_Disposable( pointer x)
{	register Disposable *debris= LaundryBasket;

	if( debris== NULL || x== NULL)
		return( NULL);
	if( debris->car== x)
		return( debris);
	while( debris){
		if( debris->car== x)
			return( debris);
		debris= debris->cdr;
	}
	return( NULL);
}

int Enter_Disposable( pointer x, unsigned long size)
{
	Disposable *cons;
	
	if( x== NULL)
		return( 1);
	/* don't allow duplicate entries to prevent
	 * freeing twice
	 */
	if( Find_Disposable( x) )
		return( 1);
	if( calloc_error( cons, Disposable, 1))
		return( -1);
	cons->car= x;
	cons->size= size;
	cons->cdr= LaundryBasket;
	if( LaundryBasket){
		cons->head= LaundryBasket->head;
		LaundryBasket->head= cons;
	}
	else{
		cons->head= cons;
		onexit( Dispose_All_Disposables, "Dispose_All_Disposables(CXX)", 0, 1);
	}
	LaundryBasket= cons;
	DisposableVolume+= 1L;
	LaundryBasket->id= DisposableVolume;
	return( 1);	
}

/* Remove_Disposable(x): remove an entry from the Disposable list.
 * x itself is unchanged. Returns x upon sucessful action (this includes
 * the case x==NULL !)
 */
pointer Remove_Disposable( pointer x)
{	Disposable *debris= Find_Disposable(x);

	  /* if we found something, remove it from the list	*/
	if( debris){
		if( debris== LaundryBasket){
		  /* update the ListHead	*/
			LaundryBasket= debris->cdr;
			if( LaundryBasket ){
				LaundryBasket->head= NULL;
			}
		}
		else{
			if( debris->head && debris->head->cdr){
				debris->head->cdr= debris->cdr;
			}
			else{
				debris->head= NULL;
			}
			if( debris->cdr && debris->cdr->head){
				debris->cdr->head= debris->head;
			}
			else{
				debris->cdr= NULL;
			}
		}
		  /* deallocate the listnode	*/
		free( debris);
		DisposableVolume-= 1L;
		return( x);
	}
	return( 0L);
}

/* Dispose_Disposable(x): remove an entry from the Disposable list,
 * and deallocate x. Returns NULL upon sucessful action (this includes
 * the case x==NULL !), else x
 */
pointer Dispose_Disposable( pointer x)
{  pointer y= Remove_Disposable(x);
	if( x && y== x){
	  /* the listnode associated with x was removed
	   * from the list: now deallocate x itself
	   */
#ifdef SYMBOLTABLE
		Remove_VariableSymbol(x);
#endif
		free(x);
		return(NULL);
	}
	else
		return(x);
}

int Dispose_All_Disposables()
{	register Disposable *next;
	int i= 0;
	unsigned long size= 0L;

	while( LaundryBasket){
		i+= 1;
		size+= LaundryBasket->size;
		next= LaundryBasket->cdr;
		free( LaundryBasket->car);
		free( LaundryBasket);
		DisposableVolume-= 1L;
		LaundryBasket= next;
	}
	return( i);
}

int List_Disposables( FILE *fp)
{	register Disposable *debris= LaundryBasket;
	register long i= 0L;
	register unsigned long size= 0L;
	char form1[40], form2[40];
	
	fprintf( fp, "\t** List_Disposables **\n");
/* 	sprintf( form1, "%%-4c|%%-10s|%%-%ds|%%s\n", LABELSIZE);	*/
	sprintf( form1, "%%-4c|%%-10s|%%-10s|%%s\n");
/* 	sprintf( form2, "%%-4ld|0x%%-08lx|%%-%ds|%%-8lu\n", LABELSIZE);	*/
	sprintf( form2, "%%-4ld|0x%%-08lx|%%-10lu|%%s\n");
	fprintf( fp, form1,
		'#', "Address", "Size", "Name"
	);
	while( debris){
		fprintf( fp, form2,
			i++, debris->car, debris->size, Find_Symbol(debris->car)
		);
		size+= debris->size;
		debris= debris->cdr;
	}
	fprintf( fp, "Total: %lu bytes\n", size);
	fprintf( fp, "\t*****************\n");
	Flush_File( fp);
	return( (int)i);
}
#ifdef DEBUG
#	define REGISTER	static
#else
#	define REGISTER	register
#endif

/* initialise an array as a RootPointer data structure: this is a pointer-array containing
 * * the max itemsize (int) and max number (int) at offset (int*)-4
 *   (as stored when the RootPointer was first allocated)
 * * the size (int) and the number (int) of elements before offset 0
 *   (indicate the actual sizes (updated by renew_RootPointer))
 * * a terminator at offset n
 * * the startaddress of the user array (offset 0) at offset n+1*unsigned long
 * all elements are set to NULL 
 * the whole thing is entered in the Disposable-list if requested.
 */
pointer init_RootPointer( char *q, int n, int itemsize, unsigned long size, int EnterDisposable )
/* char *q;	*/
/* int n, itemsize, EnterDisposable ;	*/
/* unsigned long size;	*/
{  int i;
   REGISTER RootPointer *P;
   unsigned long *p;

	P= (RootPointer*) &q[0];

	if( size ){
		P->MaxItemSize= itemsize;
		P->MaxItems= n;
	}
	P->ItemSize= itemsize;
	P->Items= n;
	P->RootPointer= P;
	P->UserMem= (unsigned char*) &q[sizeof(RootPointer)];

	p= (unsigned long*) &q[sizeof(RootPointer)+ n* itemsize];

	  /* the two unsigned longs following the max. user space contain
	   * a terminator, and the address of the user space resp.
	   */
	if( size){
		p[0]= 0xFFFFFFFF;
		p[1]= (unsigned long) P->UserMem;
	}

	if( EnterDisposable)
		Enter_Disposable( q, size);

	q= (char*) P->UserMem;
	if( size){
		  /* the user space is filled with zeros	*/
		for( i= 0; i< n* itemsize ; i++)
			q[i]= 0;
	}
	else{
	  /* This should actually do a clear of all the items in
	   * the array.
	   */
	}
#ifdef DEBUG
  /* print the contents, using the user pointer	*/
	print_RootPointer( cx_stderr, (pointer) q, False );
#endif
	if( !RootPointer_Check( q) )
		return( q);
	else
		return( NULL);
}

/* make a RootPointer data structure, linked to rmkey:
 * This pointer is NOT entered in the Disposable list!!
 */
pointer new_RootPointer_Key( RememberKey **rmkey, int n, int itemsize)
{	unsigned long size;
	char *q;
	
	  /* ensure multiple of 4 bytes:	*/
	while( (size= n* itemsize+ RootPointer_Extra(NULL)) % 4 )
		n++;

	  /* get the memory	*/
	if( !( q= CX_AllocRemember( rmkey, size, 0L)) )
		return( NULL);
	return( init_RootPointer( q, n, itemsize, size, False) );
}

/* make a RootPointer data structure: */
pointer new_RootPointer( int n, int itemsize)
{	unsigned long size;
	char *q;
	
	  /* ensure multiple of 4 bytes:	*/
	while( (size= n* itemsize+ RootPointer_Extra(NULL)) % 4 )
		n++;

	  /* get the memory; n*itemsize bytes, plus space for 3 extra
	   * unsigned longs, and an extra int
	   */
	if( calloc_error( q, char, size) )
		return( NULL);
	return( init_RootPointer( q, n, itemsize, size, True) );
}

pointer renew_RootPointer( pointer *rp, int n, int itemsize,
	DEFMETHOD(dispose_method, (pointer p), int)
)
/* pointer *rp;	*/
/* int n, itemsize;	*/
/* DEFMETHOD( dispose_method, (pointer rp), int);	*/
{  int alloc= 1;
   char **rptr= ((char**)rp);
   REGISTER RootPointer *P;
	if( !RootPointer_Check(*rp) ){
	  /* it's a valid RootPointer; check its sizes:	*/
		P= (RootPointer*) RootPointer_Base(*rp);
		if( P->MaxItemSize>= itemsize && P->MaxItems>= n)
			alloc= 0;
	}
	if( alloc){
	  /* must make a new one..	*/
		if( *rp && dispose_method)
			(*dispose_method)( *rp);
		return( (*rptr= (char*) new_RootPointer( n, itemsize)) );
	}
	else{
	  /* re-initialise : pass (mem) size==0 to indicate that only
	   * the actual length and itemsize fields should be updated.
	   */
		return( (*rptr= init_RootPointer( (pointer)P, n, itemsize, 0, False)) ); 
	}
}

int RootPointer_Check( pointer p)
{	int count, isize;
	REGISTER RootPointer *P;
	unsigned long *q;
	unsigned char *c= p;
	
	if( !p)
		return(1);
	P= (RootPointer*) RootPointer_Base(p);
	if( P->UserMem!= c)
		return( 3);
	if( (count= P->MaxItems)<= 0 || RootPointer_Length(p)<= 0 )
		return(1);
	if( (isize= P->MaxItemSize)<= 0 || P->ItemSize<= 0 )
		return(4);
	q= (unsigned long*) &c[count* isize];
	if( q[0]!= 0xFFFFFFFF)
		return( 2);
	if( q[1]!= (unsigned long) p  )
		return( 3);
	return( 0);
}

/* This function checks if RootPointer p has at least n
 * items. Returns 0 for a bad RootPointer, n if p has at
 * least n items, or the number of items in p if n is larger
 * or <= zero.
 */
int RootPointer_CheckLength( pointer p, int n, char *mesg)
{  int j, check;
	if( !(check= RootPointer_Check( p) ) ){
		j= RootPointer_Length( p);
		if( n< 0)
			n= j;
		else if( j< n){
			fprintf( stderr, "%s::RootPointer_CheckLength(): only %d items in 0x%lx, not %d\n", mesg, j, p, n);
			n= j;
		}
		return(n);
	}
	else{
		fprintf( stderr, "%s::RootPointer_CheckLength(): %s 0x%lx\n",
				mesg, RootPointer_Failure[check], p);
		return(0);
	}
}

int _RootPointer_Length(pointer x)
{
	return( (int) RootPointer_Length(x) );
}

int _RootPointer_ISize(pointer x)
{
	return( (int) RootPointer_ISize(x) );
}

unsigned long _RootPointer_Size(pointer x)
{
	return( (unsigned long) RootPointer_Size(x) );
}

RootPointer *_RootPointer_Base(pointer x)
{
	return( RootPointer_Base(x) );
}

int print_RootPointer( FILE *fp, pointer p, int IsARootPointer )
{  REGISTER RootPointer *rp;
	if( IsARootPointer){
		if( !p)
			return(0);
		rp= (RootPointer*) p;
		p= (pointer) rp->UserMem;
	}
	else{
		if( RootPointer_Check(p))
			return(0);
		else
			rp= (RootPointer*) RootPointer_Base(p);
	}
	fprintf( fp, "RootPointer data (0x%lx) for (%s)\n", rp, address_symbol(p));
	fprintf( fp, "\tMaxItemSize= %d, MaxItems= %d\n", rp->MaxItemSize, rp->MaxItems);
	fprintf( fp, "\tItemSize= %d, Items= %d\n", rp->ItemSize, rp->Items);
	fprintf( fp, "\tRootPointer= 0x%lx\n", rp->RootPointer);
	fprintf( fp, "\tUserMem= 0x%lx\n", rp->UserMem);
	return(1);
}

RememberList *CX_callocList= NULL, *CX_callocListTail= NULL;
static long CX_callocSerial= 0L;
unsigned long CX_calloced= 0, CX_callocItems= 0;

short lfree_alien= 1, PrintRememberList_on_Exit= 0;

char Lfree_Alien= 0;
struct Remember *CX_key= NULL;

#undef free
#undef calloc

void *lib_calloc( size_t n, size_t s)
{
	if( n && s ){
		return( calloc( n, s) );
	}
	else{
		return( NULL );
	}
}

void lib_free( void *p)
{
	free(p);
}


void CX_PrintRememberList(FILE *fp)
{	RememberList *l;
	struct Remember *r;
	unsigned long memtot= 0L, i;
	int print_it;

	if( !fp)
		fp= cx_stderr;
	fprintf( fp, "Total Memory allocated: %lu bytes\n", CX_calloced);

	for( l= CX_callocList, i= 0L; i< CX_callocItems && l && !(i && l== CX_callocList); i++, l= l->prev)
		;
	print_it= PrintRememberList_on_Exit || i< 10;

	for( l= CX_callocList, i= 0L; i< CX_callocItems && l && !(i && l== CX_callocList); i++, l= l->prev){
		if( l->serial< 0 || l->serial> CX_callocSerial){
			fprintf( fp, ">%lu:Invalid Serial #%ld RmKey=0x%lx\n",
				i, l->serial, l->RmKey
			);
		}
		else{
			if( print_it ){
				fprintf( fp, ">%lu:\tSerial #%ld RmKey=0x%lx:\n",
					i, l->serial, l->RmKey
				);
			}
			if( (r= l->RmKey) ){
			  unsigned long tot= 0L;
				while( r){
					if( print_it ){
						fprintf( fp, "\t: KeySerial #%ld:\tMem@0x%lx,Size=%lu (#%lu*%lu) More=%d\n",
							r->serial, r->Memory, r->RememberSize, r->item_nr, r->item_len,
							r->items
						);
					}
					tot+= r->RememberSize;
					r= r->NextRemember;
				}
				if( print_it )
					fprintf( fp, "\t\tTotal Memory: %lu bytes\n", tot);
				memtot+= tot;
			}
		}
	}
	fprintf( fp, "Total User Memory: %lu bytes, %lu RememberList entries\n", memtot, i);
}

#	ifdef __STDC__
pointer CX_AllocRemember( struct Remember **rmkey, unsigned long n, unsigned long dum)
#	else
pointer CX_AllocRemember( rmkey, n, dum)
  struct Remember **rmkey;
  unsigned long n, dum;
#	endif
{	struct Remember *r;
	char *p;
	RememberList *l=NULL;
	unsigned long alloced;
	long _serial;
	static unsigned long N;

	if( !(N= n)){
		return( NULL);
	}
	if( (p= calloc( N, 1))== NULL ){
		fprintf( cx_stderr, "CX_AllocRemember(): can't allocate %lu bytes: %s\n",
			N, serror()
		);
		return( NULL);
	}
	if( !(r= (struct Remember*)calloc( 1L, (unsigned long) sizeof( struct Remember)))){
		fprintf( cx_stderr, "CX_AllocRemember(): can't allocate %d bytes for Remember key: %s\n",
			sizeof(struct Remember), serror()
		);
		errno= EALLOCSCRATCH;
		free( p);
		return( NULL);
	}

	r->RememberSize= N;
	r->Memory= p;
	r->UserMemory= p;
	r->item_len= N;
	r->item_nr= 1;
	if( (r->NextRemember= *rmkey) )
		_serial= r->serial= (*rmkey)->serial+ 1L;
	else
		_serial= r->serial= CX_callocSerial;
	switch( _serial){
		case -1:
			fprintf( stderr, "Allocating key #%d, key=0x%lx, mem=0x%lx\n", _serial, r, p);
	}

	alloced= N+ (unsigned long)sizeof(struct Remember);

	if( !*rmkey ){
#ifdef USE_REMEMBERLIST
		if( !(l= (RememberList*) calloc( 1L, (unsigned long) sizeof(RememberList)) ) ){
			fprintf( cx_stderr, "CX_AllocRemember(): can't allocate %d bytes for RememberList entry: %s\n",
				sizeof(RememberList), serror()
			);
			errno= EALLOCSCRATCH;
			free(p);
			free(r);
			return(NULL);
		}
		l->RmKey= r;
		l->serial= r->serial;
		if( !CX_callocList)
			CX_callocListTail= CX_callocList= l;
		else{
			l->next= CX_callocListTail;
			CX_callocListTail->prev= l;
		}
		l->prev= CX_callocList;
		CX_callocList->next= l;
		CX_callocList= l;
		alloced+= (unsigned long)sizeof(RememberList);
#endif
		CX_callocItems++;
		r->RememberList= l;
	}
	else{
		r->items= (*rmkey)->items+ 1;
		r->RememberList= (*rmkey)->RememberList;
	}

	CX_callocSerial++;
	*rmkey= r;
	CX_calloced+= alloced;
	return( p);
}

#	ifdef __STDC__
unsigned long __CX_FreeRemember( struct Remember **rmkey, long de_allocmem, int (*pre_fun)(), char *msg )
#	else
unsigned long __CX_FreeRemember( rmkey, de_allocmem, pre_fun, msg)
  struct Remember **rmkey;
  long de_allocmem;
  int (*pre_fun)();
  char *msg;
#	endif
{	struct Remember *r;
	unsigned long i, freed= 0L;
/*
	RememberList *l, *pl;
	static long prev_serial= -
	static RememberList *prev_l_prev= NULL;
 */
	long serial, id;
	int ok= 1;

	i= 0L;
	if( !rmkey || !(r= *rmkey) )
		return(0);
#ifdef USE_REMEMBERLIST
	l= CX_callocList;
	for( ;i< CX_callocItems && !(i && l== CX_callocList) && r->RememberList!= l ; l= l->prev, i++)
		pl= l;
	if( r->RememberList!= l){
		fprintf( cx_stderr, "%s: ignoring bad RememberKey 0x%lx@0x%lx #%ld (0x%lx@0x%lx #%ld), size=%lu\n",
			msg, r, r->RememberList, r->serial, l->RmKey, l, l->serial, r->RememberSize
		);
		return;
	}
#endif

	serial= r->serial;
	id= 0;
switch( serial){
	case -1:
		fprintf( stderr, "Freeing key #%d\n", serial);
}
	*rmkey= NULL;
	while( r && serial>= 0 && ok ){
	  struct Remember *nextr;
#ifdef USE_REMEMBERLIST
		if( (nextr= r->NextRemember)== NULL){
			if( l->RmKey!= r || l->serial!= r->serial){
				fprintf( cx_stderr,
					"%s: fatal: bad first RememberKey 0x%lx@0x%lx #%ld (0x%lx@0x%lx #%ld), size=%lu\n",
					msg, r, r->RememberList, r->serial, l->RmKey, l, l->serial, r->RememberSize
				);
				ok= 0;
			}
		}
#else
		nextr= r->NextRemember;
#endif
		if( ok){
#ifdef USE_REMEMBERLIST
			if( r->serial!= serial || l!= r->RememberList){
				fprintf( cx_stderr,
					"%s: fatal: bad RememberKey %d 0x%lx@0x%lx #%ld!=%ld (0x%lx@0x%lx #%ld), size=%lu\n",
					msg, id, r, r->RememberList, r->serial, serial, l->RmKey, l, l->serial, r->RememberSize
				);
				ok= 0;
			}
#else
			if( r->serial!= serial ){
				fprintf( cx_stderr,
					"%s: fatal: bad RememberKey %d 0x%lx@0x%lx #%ld!=%ld, size=%lu\n",
					msg, id, r, r->RememberList, r->serial, serial, r->RememberSize
				);
				ok= 0;
			}
#endif
			else{
				if( de_allocmem){
					if( pre_fun){
					  /* Pass the actual userpointer to a function that can do something
					   * with it. Typically, pre_fun will be Remove_VariableSymbol().
					   */
						(*pre_fun)( r->UserMemory);
					}
					free( r->Memory);
					freed+= r->RememberSize;
				}
				freed+= (unsigned long) sizeof(struct Remember);
				r->RememberSize= 0;
				r->Memory= NULL;
				free( r);
			}
		}
		serial-= 1L;
		id++;
		r= nextr;
	}

#ifdef USE_REMEMBERLIST
	if( !r){
		/* free the entry in the RememberList	*/
		l->prev->next= l->next;
		l->next->prev= l->prev;
		prev_serial= l->serial;
		prev_l_prev= l->prev;
		if( l== CX_callocList){
			if( CX_callocList== CX_callocListTail)
				CX_callocList= CX_callocListTail= NULL;
			else
				CX_callocList= l->prev;
		}
		else if( l== CX_callocListTail)
			CX_callocListTail= l->next;
		l->RmKey= r;
		free(l);
		freed+= (unsigned long) sizeof(RememberList);
	}
#endif

	CX_calloced-= freed;
	CX_callocItems--;
	return( freed);
}

#	ifdef __STDC__
unsigned long CX_FreeRemember( struct Remember **rmkey, long de_allocmem)
#	else
unsigned long CX_FreeRemember( rmkey, de_allocmem)
  struct Remember **rmkey;
  long de_allocmem;
#	endif
{
	return( __CX_FreeRemember( rmkey, de_allocmem, NULL, "CX_FreeRemember()") );
}

#include "alloca.c"

unsigned long calloc_size= 0L;

pointer lcalloc( unsigned long n, unsigned long s)
{	unsigned long N;
	long *p, *q;
	struct Remember *rmkey= NULL;

	if( ( N= n* s)){							/* get N, plus 2 longs for data	*/
		if( ( q= p= (long*)CX_AllocRemember( &rmkey, N+2*sizeof(pointer), 0L))== NULL )
			return( NULL);
		*p++= LCALLOC;							/* mark it as gotten by lcalloc	*/
		*p++= (unsigned long)rmkey;				/* store the Remember key	*/
		rmkey->UserMemory= p;					/* The actual user pointer	*/
		rmkey->item_len= s;
		rmkey->item_nr= n;
		calloc_size= N;
		return( (pointer)p);					/* return pointer to user area	*/
	}
	else{
		calloc_size= 0;
		return( NULL);
	}
}

unsigned long lfree( pointer ptr)
{	struct Remember *rmkey;
	unsigned long n, *p= ptr;

	if( p){
		errno= 0;
		if( p[-2]== LCALLOC){				/* lcalloc() pointer	*/
			rmkey= (struct Remember*) p[-1];	/* the Remember key	*/
			n= rmkey->RememberSize;			/* the whole allocated size	*/
			p[-2]= LFREED;
			CX_FreeRemember( &rmkey, TRUE);	/* CX_FreeRemember() the whole thing	*/
			return( n- 2*sizeof(pointer));				/* return size freed	*/
		}
		else{
			if( lfree_alien || Lfree_Alien){
				free( p);					/* try to free() it	*/
				return( -1);
			}
			else{
				if( p[-2]== LFREED ){
					errno= ELFREETWICE;
					fprintf( cx_stderr, "lfree(): attempt to free 0x%lx again\n", p);
				}
				else{
					errno= ELFREEALIEN;
					fprintf( cx_stderr, "lfree(): bad pointer 0x%lx ((ULONG)p[-2]=0x%lx p[-1]=0x%lx)\n", p, p[-2], p[-1] );
				}
				fflush( cx_stderr);
				return( 0);
			}
		}
	}
	else{
		errno= ELFREEALIEN;
/* 		fprintf( cx_stderr, "lfree(): bad pointer 0x%lx\n", p);	*/
		fputs( "lfree(): null pointer\n", cx_stderr);
		fflush( cx_stderr);
		return( 0);
	}
}

#define free lfree
#define calloc lcalloc


/* a simple TraceStack frame:
 * call Init_Trace_Stack with the maximum depth desired;
 * call Push_Trace_Stack() (macro) to push a new trace
 * to the stack; Pop_Trace_Stack(n) (macro) to pop n
 * traces off the stack. Print_Trace_Stack prints to
 * its FILE argument.
 */

trace_stack_item *CX_Trace_StackBase= NULL, *CX_Trace_StackTop= NULL;
int Max_Trace_StackDepth= 0;

Dispose_Trace_Stack()
{
	if( CX_Trace_StackBase)
		free( CX_Trace_StackBase);
	CX_Trace_StackBase= NULL;
	Max_Trace_StackDepth= 0;
	CX_Trace_StackTop= NULL;
	return(0);
}

int Init_Trace_Stack(int n)
{	int i;
	if( n> Max_Trace_StackDepth+1 ){
		Dispose_Trace_Stack();
		if( !(CX_Trace_StackBase= (trace_stack_item*)calloc(n,sizeof(trace_stack_item))) )
			return( 0);
		else{
			onexit( Dispose_Trace_Stack, "Dispose_Trace_Stack()", 0, 0);
			CX_Trace_StackTop= CX_Trace_StackBase;
			for( i= 0; i< n; i++)
				CX_Trace_StackBase[i].depth= i+ 1;
			Max_Trace_StackDepth= n- 1;
			return( n);
		}
	}
	else if( n> 0){
		CX_Trace_StackTop= CX_Trace_StackBase;
		for( i= 0; i< n; i++)
			CX_Trace_StackBase[i].depth= i+ 1;
		Max_Trace_StackDepth= n;
	}
}

/* #define next_item(x)	(*((struct cx_trace_stack_item**)(x)))	*/
#define next_item(x)	x->next

int push_trace_stack( char *file, char *cctime, char *func, int line)
{	trace_stack_item *t;
	if( CX_Trace_StackTop && CX_Trace_StackTop->depth< Max_Trace_StackDepth){
		if( !next_item(CX_Trace_StackTop)){	/* really CX_Trace_StackTop->next	*/
			t= CX_Trace_StackTop;			/* the place to push to	*/
			next_item(t)= &CX_Trace_StackTop[1];	/* the next place to push to	*/
		}
		else{
			t= next_item( CX_Trace_StackTop);	/* push to next item	*/
			next_item(t)= &t[1];				/* update & increment top	*/
		}
		t->line= line;
		strncpy( t->file, file, TRACENAMELENGTH_1);
		strncpy( t->cctime, cctime, TRACENAMELENGTH_1);
		strncpy( t->func, func, TRACENAMELENGTH_1);
		CX_Trace_StackTop= t;
		return( t->depth);
	}
	else
		return( -1);
}

int swap_trace_stack( char *file, char *cctime, char *func, int line)
{	trace_stack_item *t= CX_Trace_StackTop;
	int depth;
	if( t && t->depth-1< Max_Trace_StackDepth){
		if( !next_item(t)){
			next_item(t)= &CX_Trace_StackTop[1];
			depth= 0;
		}
		else
			depth= t->depth;
		t->line= line;
		strncpy( t->file, file, TRACENAMELENGTH_1);
		strncpy( t->cctime, cctime, TRACENAMELENGTH_1);
		strncpy( t->func, func, TRACENAMELENGTH_1);
		return( depth);
	}
	else
		return( -1);
}

int pop_trace_stack(int n)
{	int i;
	trace_stack_item *t= CX_Trace_StackTop;
	if( !t ){
		return(-1);
	}
	for( i= 0; i< n && t->depth-1> 0; i++)
		t--;
	CX_Trace_StackTop= t;
	if( i< n && t== CX_Trace_StackBase)
		next_item(t)= NULL;
	return( t->depth);
}

int pop_trace_stackto(int n)
{	trace_stack_item *t= CX_Trace_StackTop;
	int m= n;
	if( !t ){
		return(-1);
	}
	if( n< 0)
		return( t->depth);
	if( m== 0)
		m++;
	while( t->depth> m)
		t--;
	CX_Trace_StackTop= t;
	if( n== 0 && t== CX_Trace_StackBase)
		next_item(t)= NULL;
	return( t->depth);
}

int print_trace_stack(FILE *fp, unsigned int n, char *msg)
{	unsigned int i;
	int j;
	if( !CX_Trace_StackTop ){
		return(-1);
	}
	i= MIN( n, (unsigned int) CX_Trace_StackTop->depth);
	i= MIN( i, (unsigned int) Max_Trace_StackDepth);
	if( msg)
		fprintf( fp, "# Trace Stack %s:\n", msg);
	else
		fprintf( fp, "# Trace Stack:\n");
	if( (j= i) ){
		for( --j ; j>= 0 && fp; j--){
			CX_Trace_StackBase[j].file[TRACENAMELENGTH_1]= '\0';
			CX_Trace_StackBase[j].func[TRACENAMELENGTH_1]= '\0';
			fprintf( fp, "file %s ; line %d #%d: \"%s\" (%s)\n",
				CX_Trace_StackBase[j].file, CX_Trace_StackBase[j].line,
				j, CX_Trace_StackBase[j].func, CX_Trace_StackBase[j].cctime
			);
		}
	}
	fflush( fp);
	return( (int)i);
}

void stimeprint(char *str, char *str2)
{
	   struct tm *tm;
	   time_t Time;
	   
	   Time= time( NULL);
	   tm= localtime( &Time);
	   sprintf(str, "%s%02d:%02d:%02d", str2, tm->tm_hour, tm->tm_min, tm->tm_sec);
}


void sdateprint(char *str, char *str2)
{
	   struct tm *tm;
	   time_t Time;
	   
	   Time= time( NULL);
	   tm= localtime( &Time);
	   sprintf(str, "%s%02d/%02d/%d", str2, tm->tm_mday, tm->tm_mon+ 1, tm->tm_year +1900);
}
 
/* print strcat( s, "date ( time)") to strm */ 
void calendar(FILE *strm, char *s)
{
	char tim[20], dat[20];
	
	fprintf(strm, s);
	stimeprint(tim, "");
	sdateprint(dat, "");
	fprintf(strm, "%s (%s)\n", dat, tim);
}

int change_stdin( char *newfile, FILE *errfile)
{
#ifdef MCH_AMIGA
  char *_dev_tty="*";
#endif
#ifdef _UNIX_C_
  int stdin_fd= fileno(stdin);
  char *_dev_tty=ttyname(stdin_fd);
#endif
 FILE *fp;
	if( newfile && strlen(newfile) ){
	 char *c= newfile;
		if( !_dev_tty ){
			_dev_tty= "/dev/tty";
		}
		if( (fp= fopen( c, "r+")) && !feof(fp) && !ferror(fp) ){
			fclose(fp);
			if( !(fp= freopen( c, "r+", stdin)) && !feof(fp) && !ferror(fp) ){
				fprintf( errfile, "cxx:change_stdin(\"%s\"): error: can't redirect stdin (%s) - reopening %s\n",
					c, serror(), _dev_tty
				);
				fp= freopen( _dev_tty, "r", stdin);
				c= _dev_tty;
			}
		}
		if( !fp){
			fprintf( errfile, "cxx:change_stdin(\"%s\"): error: can't redirect stdin (%s)\n",
				c, serror()
			);
			Flush_File( errfile);
			return( False);
		}
		else
			return( True);
	}
	else{
/* 
		fputs( "cxx::change_stdin(): error: need a filename to redirect from\n", errfile);
		Flush_File( errfile);
		return( False);
 */
		fputs( "cxx::change_stdin(): warning: closing stdin!\n", errfile );
		Flush_File( errfile);
		fclose( stdin );
	}
}

int change_stdout_stderr( char *newfile, FILE *errfile)
{
#ifdef MCH_AMIGA
  char *_dev_tty="*";
#endif
#ifdef _UNIX_C_
  int stdin_fd= fileno(stdin);
  char *_dev_tty=ttyname(stdin_fd);
#endif
 FILE *fp, *fpo, *fpe;
	if( newfile && strlen(newfile) ){
	 char *c= newfile;
		if( !_dev_tty ){
			_dev_tty= "/dev/tty";
		}
		if( (fp= fopen( c, "a")) ){
			fclose(fp);
			if( !(fpo= freopen( c, "a", stdout)) ){
				fprintf( errfile, "cxx:change_stdout_stderr(\"%s\"): error: can't change stdout (%s) - reopening %s\n",
					c, serror(), _dev_tty
				);
				fpo= freopen( _dev_tty, "a", stdout);
				c= _dev_tty;
			}
			if( !(fpe= freopen( c, "a", stderr)) ){
				fprintf( errfile, "cxx:change_stdout_stderr(\"%s\"): error: can't change stderr (%s) - reopening %s\n",
					c, serror(), _dev_tty
				);
				fpe= freopen( _dev_tty, "a", stderr);
				c= _dev_tty;
			}
		}
		if( !fpo || !fpe ){
			fprintf( errfile, "cxx:change_stdout_stderr(\"%s\"): error: can't change stdout & stderr (%s)\n",
				c, serror()
			);
			Flush_File( errfile);
			return( False);
		}
		if( c== _dev_tty){
			return( False);
		}
		else{
			return( True);
		}
	}
	else{
/* 
		fputs( "cxx:change_stdout_stderr(): error: need a filename to append to\n", errfile);
		Flush_File( errfile);
		return( False);
 */
		fputs( "cxx:change_stdout_stderr(): warning: closing output streams!\n", errfile );
		Flush_File( errfile);
		fclose( stdout );
		fclose( stderr );
	}
}

int Close_File( FILE *fp, int final)
{	long size;
	char *c;
	int ret;

	if( !fp)
		return(0);
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
	if( !strcmp( (c= Find_Symbol(fp)), UNKNOWN) ){
		fprintf( stderr, "Close_File(): Unknown filepointer [0x%lx]: not closed!\n", fp);
		return( 0);
	}
#ifndef VARS_STANDALONE
	if( cx_stderr== fp){
		cx_stderr= stderr;
	}
#endif
#endif
	Flush_File( fp);
	fseek( fp, 0L, 2); size= ftell( fp);
	ret= fclose( fp);
	fprintf( stderr, "Close_File(): file '%s' closed, %ld bytes", c, size);
	if( size== 0L){
		if( unlink( c))
			fprintf( stderr, "; couldn't remove: %s", serror());
		else
			fputs( "; empty, removed", stderr);
	}
	calendar( stderr, " - ");
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
	Remove_VariableSymbol( fp);
#endif
	Flush_File(stderr);
	return( ret);
}

FILE *Open_File( char *name, char *mode)
{  FILE *fp;
	if( !name || !name[0] ){
		fprintf( stderr, "Open_File(): empty filename or nullpointer\n" );
		return( NULL );
	}
	if( !mode || !mode[0] ){
	  struct stat Stat;
		if( stat( name, &Stat) ){
		  /* file doesn't exist, assume "w"	*/
			mode= "w";
			fprintf( stderr, "Open_File(\"%s\",NULL): file non-existent, creating\n", name );
		}
		else{
			mode= "r";
			fprintf( stderr, "Open_File(\"%s\",NULL): file exists, opening for reading\n", name );
		}
	}
	if( (fp= fopen( name, mode))){
		fprintf( stderr, "Open_File(): '%s' (re)opened in mode '%s'",
			name, mode
		);
		calendar( stderr, " - ");
	}
	else{
		fprintf( stderr, "Open_File(): can't open '%s' (%s)\n",
			name, serror()
		);
	}
	Flush_File( stderr);
	return( fp);
}

#define PPROCESS_TAG	"Pipe to "
#define APPROCESS_TAG	"Alternative to "

/* Open a pipe to command. If command starts with
 * a '$', then the rest of the string is interpreted
 * as an env variable. If the pipe-open doesn't succeed,
 * the alternative FILE pointer is returned.
 * Otherwise, the pointer is entered in the SymbolTable;
 * this should not be interfered with!
 */
FILE *Open_Pipe_To( char *command, FILE *alternative)
{ FILE *fp= NULL;
  char *pager= command, tag[512];
	if( command && command[0]== '$')
		pager= GetEnv( &command[1]);
	if( pager){
		  /* A small line to try to make sh start faster.
		   \ (on some implementations) popen returns a valid
		   \ FILE pointer while somewhat later sh will complain
		   \ that it couldn't execute the command
		system( "sh < /dev/null > /dev/null 2>&1");
		   */
		fp= popen( pager, "w");
	}
	if( fp){
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
		sprintf( tag, "%s%s", PPROCESS_TAG, pager);
		Add_VariableSymbol( tag, fp, 1, _file, 0);
#endif
	}
	else{
		fp= alternative;
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
		sprintf( tag, "%sfd=%d", APPROCESS_TAG, fileno(fp));
		Add_VariableSymbol( tag, fp, 1, _file, 0);
#endif
		if( pager ){
			fprintf( cx_stderr, "Open_Pipe_To(%s,%s): popen(%s,w) failed (%s)\n",
				(command)? command : "<<Null Pointer>>", address_symbol(alternative),
				(pager)? pager : "<<Null Pointer>>", serror()
			);
		}
	}
	return(fp);
}

FILE *Open_Pipe_From( char *command, FILE *alternative)
{ FILE *fp= NULL;
  char *pager= command, tag[512];
	if( command && command[0]== '$')
		pager= GetEnv( &command[1]);
	if( pager){
		  /* A small line to try to make sh start faster.
		   \ (on some implementations) popen returns a valid
		   \ FILE pointer while somewhat later sh will complain
		   \ that it couldn't execute the command
		   */
		system( "sh < /dev/null > /dev/null 2>&1");
		fp= popen( pager, "r");
	}
	if( fp){
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
		sprintf( tag, "%s%s", PPROCESS_TAG, pager);
		Add_VariableSymbol( tag, fp, 1, _file, 0);
#endif
	}
	else{
		fp= alternative;
#if !defined(VARS_STANDALONE) || defined(SYMBOLTABLE)
		sprintf( tag, "%sfd=%d", APPROCESS_TAG, fileno(fp));
		Add_VariableSymbol( tag, fp, 1, _file, 0);
#endif
		if( pager ){
			fprintf( cx_stderr, "Open_Pipe_From(%s,%s): popen(%s,w) failed (%s)\n",
				(command)? command : "<<Null Pointer>>", address_symbol(alternative),
				(pager)? pager : "<<Null Pointer>>", serror()
			);
		}
	}
	return(fp);
}

/* Close a pipe to a process. The SymbolTable entry is used to
 * determine if fp is a pipe.
 */
#ifndef SYMBOLTABLE
Close_Pipe( FILE *fp)
{ int r;
	errno= 0;
	r= pclose(fp);
	if( errno ){
		fprintf( cx_stderr, "Close_Pipe(0x%lx): pclose() returns %d (%s)\n",
			fp, r, serror()
		);
		fflush( cx_stderr );
	}
	return(r);
}
#else
Close_Pipe( FILE *fp)
{  char *process= Find_Symbol( fp);
   int r;
	if( !process || !strncmp( process, PPROCESS_TAG, strlen(PPROCESS_TAG)) ){
		Remove_VariableSymbol( fp);
		errno= 0;
		r= pclose(fp);
		if( errno ){
			fprintf( cx_stderr, "Close_Pipe(%s): pclose() returns %d (%s)\n",
				process, r, serror()
			);
			fflush( cx_stderr );
		}
		return(r);
	}
	else{
		if( fp!= stdout && fp!= stderr && !(process && !strncmp(process, APPROCESS_TAG, strlen(APPROCESS_TAG))) ){
		  /* barf warning when passed an fp that is not an obvious alternative to Open_Pipe_To/From */
			fprintf( cx_stderr, "Close_Pipe(%s): called with FILEpointer not opened by Open_Pipe_To/From()\n",
				(process)? process : "NULL pointer"
			);
		}
		fflush( cx_stderr );
		return( 0);
	}
}
#endif

char *ObjectTypeNames[MaxObjectType]= {
	"_nothing",
	/* basic types:	*/
	"_file", "_function", "_char", "_string", "_short", "_int", "_long", "_float", "_double", "_variable",
	"_charP", "_shortP", "_intP", "_longP", "_floatP", "_doubleP", "_functionP", "_rootpointer",

	/* PatchWorks Types:	*/
	"_something", "_patch", "_neuron", "_eye", "_paddler",
	"_glowball", "_target", "_worldLayer", "_paddlerbeak", "_paddlerretina",
	"_paddlerbrain", "_paddlermechrec", "_paddlepos", "_paddleosc",
	"_simplestats", "_simpleanglestats", "_leaky_int_par", "_xlispnode"
};

char *CX_Time()
{ time_t Timer;
  char *ts, *c;
	Timer= time( NULL);
	ts= ctime(&Timer);
	if( (c= index( ts, '\n')) )
		*c= '\0';
	return( ts);
}

char Dir[256]= "./.env", *EnvDir= Dir;
static char CX_env[256];

#undef getenv
#undef setenv

/* check if a variable of name <pref><n> exists
 * in the environment or on disk.
 */
char *__GetEnv( char *n, char *pref, int add_prefix )
{	char *env= CX_env;
	char *set, *getenv(), name[256], fname[512];
	char *home= getenv("HOME");
	FILE *EnvFp;
	int nn= EOF;
	char *where;

	errno= 0;
	if( !home)
		home= ".";
	if( !add_prefix ){
	  /* first time: try without prefix	*/
		strncpy( name, n, 255);
	}
	else if( strlen(pref) + strlen(n) < 255 ){
		sprintf( name, "%s%s", pref, n);
	}
	else{
		if( getenv("GETENV_DEBUG") )
			fprintf( stderr, "cx::__GetEnv(\"%s\",\"%s\",%d) = NULL\n", n, pref, add_prefix );
		return( NULL );
	}
	if( !(set= getenv(name)) ){
	  int en;
	  /* name not set in the environment; try to find it on disk	*/
		if( EnvDir && strlen( EnvDir)){
			sprintf( fname, "%s/%s", EnvDir, name);
		}
		else{
			sprintf( fname, "%s/.env/%s", home, name);
		}
		en= errno;
		if( ( EnvFp= fopen( fname, "r"))){
			nn= fread( env, 1, sizeof(CX_env), EnvFp);
			fclose( EnvFp);
			if( !set && nn>= 0 && nn!= EOF ){
				env[nn]= '\0';
				if( env[0]== '\\' ){
					set= ( &env[1]);
				}
				else{
					set= ( env);
				}
			}
			where= "DSK";
		}
		else{
			errno= en;
		}
	}
	else{
		where= "ENV";
	}
	if( set ){
	 char *next_set;
	 /* variable was found: see if a prefixed one exists to
	  \ override it.
	  */
		if( getenv("GETENV_DEBUG") ){
			fprintf( stderr, "cx::__GetEnv(\"%s\";\"%s\",\"%s\") = \"%s\" (%s)\n",
				name, n, pref, set, where
			);
		}
		if( nn>= 0 && nn!= EOF){
			if( env[0]=='$' ){
				if( /* strcmp( n, &env[1]) && */ strcmp( name, &env[1]) ){
				  /* disk env-variable refers to another	*/
					  /* recursiveness	*/
					strncpy( fname, &env[1], 255 );
					set= ( __GetEnv( fname, pref, False ) );
				}
				else{
					fprintf( stderr, "cx::__GetEnv(): recursive variable '%s' (%s) == '%s'\n",
						n, name, &env[1]
					);
					fflush( stderr);
				}
			}
			if( !set ){
				if( env[0]== '\\' ){
					set= ( &env[1]);
				}
				else{
					set= ( env);
				}
			}
		}
		  /* recursiveness	*/
		next_set= __GetEnv( name, pref, True );
		return( (next_set)? next_set : set );
	}
	if( getenv("GETENV_DEBUG") ){
		fprintf( stderr, "cx::__GetEnv(\"%s\";\"%s\",\"%s\") = NULL\n", name, n, pref );
	}
	return( NULL);
}

/* return the value of variable
 * 1) (env) _<n>
 * 2) (disk) _<n>
 * 3) (env) <n>
 * 4) (disk) <n>
 */
char *_GetEnv( char *n)		/* find an env. var in EnvDir:<n>	*/
{  char *getenv();

	errno= 0;
/* by having __GetEnv() check prefix "" first, and then,
 \ if set, calling __GetEnv(n, prefix) recursively, one
 \ can get a real hierarchy!
 */
	return( __GetEnv( n, "_", False) );
}

int GetEnvDir()
{	char *ed;
	
	ed= EnvDir;					/* save EnvDir path	*/
	EnvDir= "";
	if( ( EnvDir= _GetEnv( "ENVDIR"))){
		strcpy( Dir, EnvDir);		/* save the path we	found	*/
		EnvDir= Dir;				/* set EnvDir to the saved path	*/
	}
	else
		EnvDir= ed;				/* restore default path	*/
	return( 1);
}

char *GetEnv( char *n)
{
	GetEnvDir();
	return( _GetEnv( n));
}

char *_SetEnv( char *n, char *v)			/* find an env. var in memory (set)	*/
{	static char *fail_env= "_SetEnv=failed (no memory)";
	char *nv;

	if( !n ){
		return(NULL);
	}
	if( !v ){
		v= "";
	}
	if( !(nv= lib_calloc( strlen(n)+strlen(v)+2, 1L)) ){
		putenv( fail_env);
		return( NULL);
	}
	sprintf( nv, "%s=%s", n, v);
	putenv( nv);
	return( _GetEnv(n) );
}

char *SetEnv( char *n, char *v)
{

	GetEnvDir();
	return( _SetEnv( n, v));
}

typedef struct exit_code{
	char *name;				/* pointer to a namestring	*/
	int (*code)();				/* exit method, called if result== 0	*/
	int verbose,				/* notify if called?	*/
		condition,			/* exit code for calling this code	*/
		result,				/* result from code call	*/
		name_length;
	struct exit_code *next;		/* next in chain (FILO)	*/
} Exit_Code;

static Exit_Code *ExitCodeChain= NULL;
static struct Remember *ExitCode_key= NULL;
static CX_Exit();

static void atexit_handler()
{
	CX_Exit( 0, -1);
}

int onexit( int (*code)(), char *name, int condition, int verbose)
{	Exit_Code *ec= ExitCodeChain;
	static int nr= 0;

	if( !code || !name)
		return( 0);
	if( !nr ){
		atexit( atexit_handler );
	}
	while( ec){
		if( ec->code== code)
			return( 0);
		ec= ec->next;
	}
	if( !(ec= (Exit_Code*)CX_AllocRemember( &ExitCode_key,
			(long)sizeof( Exit_Code), 0L)))
		return( 0);
	ec->name= name;
	ec->code= code;
	ec->verbose= verbose;
	ec->condition= condition;
	ec->name_length= strlen( name);
	ec->next= ExitCodeChain;
	ExitCodeChain= ec;
	return( ++nr);
}

static CX_Exit(int x, int flag)
{	/* extern int _exit();	*/
  static char called= 0;

	if( called)
		return(0);
	called= 1;

	while( ExitCodeChain){
		if( x>= ExitCodeChain->condition){
			if( ExitCodeChain->code && !ExitCodeChain->result){
				if( ExitCodeChain->verbose){
					fputs( "Calling exit handler '", stderr);
					if( ExitCodeChain->name){
					int i;
						for( i= 0; i< ExitCodeChain->name_length; i++)
							fputc( ExitCodeChain->name[i], stderr);
					}
				}
				ExitCodeChain->result= (*ExitCodeChain->code)( x);
				if( ExitCodeChain->verbose){
					fprintf( stderr, " [returned %d]\n", ExitCodeChain->result);
					fflush( stderr);
				}
			}
			else{
				if( ExitCodeChain->verbose){
					fputs( "Error: skipping exit handler '", stderr);
					if( ExitCodeChain->name){
					int i;
						for( i= 0; i< ExitCodeChain->name_length; i++)
							fputc( ExitCodeChain->name[i], stderr);
					}
					fputs( "' without address\n", stderr);
					fflush( stderr);
				}
			}
		}
		ExitCodeChain= ExitCodeChain->next;
	}
	if( ExitCode_key)
		CX_FreeRemember( &ExitCode_key, TRUE);
	if( CX_key)
		CX_FreeRemember( &CX_key, TRUE);
	if( CX_calloced && CX_callocList){
		fprintf( stderr, "CX warning: %lu bytes not freed! Unfree memory list:\n", CX_calloced);
		CX_PrintRememberList(stderr);
	}
	else if( CX_callocList || CX_calloced )
		fprintf( stderr, "CX_calloced==%lu, and CX_callocList==0x%lx: somebody messed around!\n",
			CX_calloced, CX_callocList
		);
	fflush( stderr);
	if( flag){
		if( flag< 0){
			fprintf( stderr, "CX_Exit called from atexit\n");
			return(0);
		}
		else
			exit( x);
	}
	else
		_exit( x);
}

int CX_exit(int x)
{
	return( CX_Exit( x, 1));
}


void fillstr(char *a, int b)		/* fill a with b */
{
	while(*a)
		*a++= b;
}

void stricpy( char *a, char *b)					/* copy b in a if b< a */
{
	   if( strlen( b)> strlen( a))	/* b bigger: a becomes b	*/
			 strcpy( a, b);
	   else
			 while( *b)				/* a bigger: copy b into a	*/
					*a++= *b++;
}

/* copy b to a with exclusion of chars specified
 * in c, unless they are preceded by a '\'
 */
void strccpy(char *a, char *b, char *c)
{ int wrong_b;
  char *d;
	
	d= c;
	while(*b){
		while( !(wrong_b= (*b== *c)) && (*c++) )
			;
		if( !(wrong_b) )
			*a++= *b++;
		else if( b[-1]== '\\' ){
		  /* that b[-1]=='\\' was meant to escape b[0]. Backup a (to remove \)
		   * and put in *b
		   */
			*(--a)= *b++;
			a++;
		}
		else 
			b++;
		c= d;
	}
	*a= '\0';
}

#ifdef SYMBOLTABLE
#	include "SymbolTable.c"
#endif

#ifndef WIN32
static int_method alarm_method= NULL;

void call_alarm_call( int action )
{
	if( action== SIGALRM ){
		signal( SIGALRM, SIG_IGN );
		if( alarm_method ){
			(*alarm_method)( action );
		}
		signal( SIGALRM, call_alarm_call );
	}
}

unsigned int set_alarm_call( unsigned int interval, int_method fun)
{
	alarm_method= fun;
	signal( SIGALRM, call_alarm_call );
	return( alarm( interval ) );
}

#endif

int EndianType;

int CheckEndianness()
{ union{
	char t[2];
	short s;
  } e;
	e.t[0]= 'a';
	e.t[1]= 'b';
	if( e.s== 0x6162 ){
		EndianType= 0;
	}
	else if( e.s== 0x6261 ){
		EndianType= 1;
	}
	else{
		fprintf( stderr, "Found weird endian type: *((short*)\"ab\")!= 0x6162 nor 0x6261 but 0x%hx\n",
			e.s
		);
		EndianType= 2;
	}
	return( EndianType );
}

#include <stdarg.h>

/* concat2(): concats a number of strings; reallocates the first, returning a pointer to the
 \ new area. Caution: a NULL-pointer is accepted as a first argument: passing a single NULL-pointer
 \ as the (1-item) arglist is therefore valid, but possibly lethal!
 */
#ifdef CONCATBLABLA
char *concat2( char *a, char *b, ...)
{}
#endif
char *concat2(char *string, ...)
{ va_list ap;
  int n= 0, tlen= 0;
  char *c= string, *buf= NULL, *first= NULL;
	va_start(ap, string);
	do{
		if( !n ){
			first= c;
		}
		if( c ){
			tlen+= strlen(c);
		}
		n+= 1;
	}
	while( (c= va_arg(ap, char*)) != NULL || n== 0 );
	va_end(ap);
	if( n== 0 || tlen== 0 ){
		return( NULL );
	}
	tlen+= 4;
	if( first ){
		buf= realloc( first, tlen* sizeof(char) );
	}
	else{
		buf= first= calloc( tlen, sizeof(char) );
	}
	if( buf ){
		va_start(ap, string);
		  /* realloc() leaves the contents of the old array intact,
		   \ but it may no longer be found at the same location. Therefore
		   \ need not make a copy of it; the first argument can be discarded.
		   \ strcat'ing is done from the 2nd argument.
		   */
		while( (c= va_arg(ap, char*)) != NULL ){
			strcat( buf, c);
		}
		va_end(ap);
		buf[tlen-1]= '\0';
		fprintf( cx_stderr, "%d bytes (%d strings) concatenated in buffer of length %d (%s)\n",
			strlen(buf), n, tlen, buf
		);
	}
	else{
		fprintf( cx_stderr, "can't (re)allocate %d bytes to concat %d strings\n",
			tlen, n
		);
	}
	return( buf );
}

/* concat(): concats a number of strings	*/
#ifdef CONCATBLABLA
char *concat( char *a, char *b, ...)
{}
#endif
char *concat(char *first, ...)
{ va_list ap;
  int n= 0, tlen= 0;
  char *c= first, *buf= NULL;
	va_start(ap, first);
	do{
		tlen+= strlen(c)+ 1;
		n+= 1;
	}
	while( (c= va_arg(ap, char*)) != NULL );
	va_end(ap);
	if( n== 0 || tlen== 0 ){
		fprintf( cx_stderr, "concat(): %d strings, %d bytes: no action\n",
			n, tlen
		);
		return( NULL );
	}
	if( (buf= calloc( tlen, sizeof(char) )) ){
		c= first;
		va_start(ap, first);
		do{
			strcat( buf, c);
		}
		while( (c= va_arg(ap, char*)) != NULL );
		va_end(ap);
		fprintf( cx_stderr, "concat(): %d bytes to concat %d strings\n",
			tlen, n
		);
	}
	else{
		fprintf( cx_stderr, "concat(): can't get %d bytes to concat %d strings\n",
			tlen, n
		);
	}
	return( buf );
}

/* 20040316: a dlopen() wrapper. If loading a library fails, check to see if it doesn't live in $HOME/lib .
 \ This allows users to put softlinks to system libraries that have different names, for instance.
 */
void *CX_dlopen( const char *libname, int flags )
#ifdef WIN32
{ return( NULL ); }
#else
{ void *handle= dlopen( libname, flags);
  char *e;
	e= dlerror();
	if( !handle || e ){
	  char *c;
		if( (c= concat( getenv("HOME"), "/lib/", libname, 0 )) ){
			fprintf( cx_stderr, "CX_dlopen(\"%s\"): error loading (%s); trying \"%s\"",
				libname, e, c
			);
			handle= dlopen(c, flags);
			e= dlerror();
			lib_free(c);
			if( !handle || e ){
				fprintf( cx_stderr, "; failed too (%s)\n", e );
			}
			else{
				fputs( " (OK)\n", cx_stderr );
			}
		}
		else{
			fprintf( cx_stderr, "CX_dlopen(\"%s\"): failure loading (%s), no alternative location (%s).\n",
				libname, e, serror()
			);
		}
	}
	return(handle);
}
#endif
