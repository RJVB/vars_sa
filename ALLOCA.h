#ifndef ALLOCA

#ifdef __cplusplus
	extern "C" {
#endif

/* NB: some gcc versions (linux 2.9.?) don't like a declaration of the type 
 \ type x[len+1]
 \ claiming that the size of x is too large. Bug or feature, it can be remedied
 \ by using x[len+2] instead of x[len+1], *or* by x[LEN], where LEN=len+1.
 */

/* Set this to some value (in Mb) if you want to perform size-checking on the size argument passed to
 \ the ALLOCA() macro (defined in ALLOCA.h; expands to a system-dependent interface to the alloca()
 \ memory allocator). The need for this will depend on how your system handles alloca allocation failures.
 \ The IRIX alloca() routine (and gcc's too) is guaranteed not to fail -- i.e. it always returns a non-null
 \ pointer. If however you ask for some huge amount of memory (e.g. -1 :)), the process will be killed by
 \ the system, without leaving any leeway to the programme to handle the situation gracefully. Also,
 \ utilities like Electric Fence do not trace allocations using alloca-like allocators.
 \ The size checking offered by xgraph does not alter this, but it will print an informative message whenever
 \ more memory is requested, which allows to trace the location where the allocation was made.
 \ It will also likely slow down the programme.
 \ To activate, set CHKLCA_MAX to something (e.g. 128 to warn about allocations larger than 128Mb). Don't
 \ remove the #ifndef pair around the definition; this makes it possible to do file-specific activation.
 */
#ifndef CHKLCA_MAX
#	define CHKLCA_MAX	0
#endif

#if CHKLCA_MAX > 0
#	ifdef __GNUC__
inline
#	endif
  /* 20020501 RJVB */
static size_t check_alloca_size(size_t n, size_t s, size_t max, char *file, int lineno )
{
	if( n*s > (max << 20) ){
		fprintf( StdErr, "\ncheck_alloca_size(%lu,%lu): request for %g>%luMb in %s, line %d!\n\n",
			n, s, ( (n*s) / (double)( 1 << 20) ), max,
			file, lineno
		);
		fflush( StdErr );
	}
	return(n);
}

#	if defined(__GNUC__) && !defined(DEBUG)
#		define CHKLCA(len,type)	check_alloca_size((size_t)(len),sizeof(type),CHKLCA_MAX,__FILE__,__LINE__)
#	else
#		define CHKLCA(len,type)	check_alloca_size((size_t)(len),1,CHKLCA_MAX,__FILE__,__LINE__)
#	endif
#else
#	define CHKLCA(len,type)	len
#endif

extern void *xgalloca(unsigned int n, char *file, int linenr);

#define HAS_alloca

#ifdef WIN32
#	define alloca(n)	_alloca((n))
#endif

#	if defined(__GNUC__) && !defined(DEBUG)
#		define XGALLOCA(x,type,len,xlen,dum)	type x[CHKLCA(len,type)]; int xlen= sizeof(x)
#		define ALLOCA(x,type,len,xlen)	type x[CHKLCA(len,type)]
#		define _ALLOCA(x,type,len,xlen)	type x[CHKLCA(len,type)]; int xlen= sizeof(x)
#		define GCA()	/* */
#	else
	/* #	define LMAXBUFSIZE	MAXBUFSIZE*3	*/
#		ifndef STR
#		define STR(name)	# name
#		endif
		extern void *XGalloca(void **ptr, int items, int *alloced_items, int size, char *name);
#		define XGALLOCA(x,type,len,xlen,dum)	static int xlen= -1; static type *x=NULL; type *dum=(type*) XGalloca((void**)&x,CHKLCA(len,type),&xlen,sizeof(type),STR(x))

#		if defined(HAS_alloca) && !defined(DEBUG)
#			ifndef WIN32
#				include <alloca.h>
#			endif
#			define ALLOCA(x,type,len,xlen)	int xlen=(CHKLCA(len,type))* sizeof(type); type *x= (type*) alloca(xlen)
#			define _ALLOCA(x,type,len,xlen)	ALLOCA(x,type,CHKLCA(len,type),xlen)
#			define GCA()	/* */
#		else
#			define ALLOCA(x,type,len,xlen)	int xlen=(CHKLCA(len,type))* sizeof(type); type *x= (type*) xgalloca(xlen,__FILE__,__LINE__)
#			define _ALLOCA(x,type,len,xlen)	ALLOCA(x,type,CHKLCA(len,type),xlen)
#			define GCA()	xgalloca(0,__FILE__,__LINE__)
#		endif

#	endif

#if !defined(sgiKK) && !defined(alloca)
#	define _REALLOCA(x,type,len,xlen)	xlen=(CHKLCA(len,type))* sizeof(type); x= (type*) xgalloca(xlen,__FILE__,__LINE__)
#else
#	define _REALLOCA(x,type,len,xlen)	xlen=(CHKLCA(len,type))* sizeof(type); x= (type*) xgalloca(xlen,__FILE__,__LINE__)
#endif

#if !defined(__GNUC__) && !defined(HAS_alloca)
#	undef alloca
#	define alloca(n) xgalloca(CHKLCA((n),char),__FILE__,__LINE__)
#endif

#ifndef STRDUPA
#	define STRDUPA(target,tcopy,src,tlen)	ALLOCA(target,char,((src)?strlen(src):0)+1,tlen); char* tcopy=strcpy(target,src)
#endif

#ifdef __cplusplus
	}
#endif

#endif
