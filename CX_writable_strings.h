#ifndef _CX_WRITABLE_STRINGS_H
#define _CX_WRITABLE_STRINGS_H

/* This code stems from times where all compilers thought it perfectly legal to modify 'static' strings,
 \ allowing code like sprintf( "blablabla", "boobooboo" ) to have the expected result. Various routines
 \ in the CX libraries do string parsing, and modify elements in the given string expression (usually
 \ restoring it to its original value). Currently, gcc is in the process of removing its option -fwritable-strings
 \ which allows such old code to run without crashing.
 \ The macros below emulate that functionality by making local copies. They are for 'internal' use only.
 \ Invoke CX_MAKE_WRITABLE(function-argument, local-copy) for each string expression that needs caching.
 \ Activate the emulation by setting CX_writable_strings to True.
 \ This uses xgalloca(), an emulation of alloca().
 */

#	define CX_MAKE_WRITABLE(arg,locvar)	if( CX_writable_strings ){ \
	if( ((locvar)= (char*) xgalloca( (strlen((arg))+1)*sizeof(char), __FILE__, __LINE__ )) ){ \
		strcpy((locvar),(arg)); \
		(arg)=(locvar); \
	} \
	else{ \
		fprintf( cx_stderr, "CX_MAKE_WRITABLE(\"%s\"): allocation failure (%s); proceeding without emulation!!\n", \
			(arg), serror() \
		); \
	} \
}

/* Temporarily switch off the emulation. This allows avoiding that a function B called by a function A will
 \ allocate its own copies of the string expression(s) if called through A, while still making it copy them
 \ when called directly. You have to specifiy the local (!) cache variable for the global switch.
 */
#	define CX_DEFER_WRITABLE(locvar)	{ (locvar)=CX_writable_strings ; CX_writable_strings=0; }

/* Cleans up. Does the xgalloca() 'garbage collection', and resets CX_writable_strings. It is OK to pass
 \ CX_writable_strings itself, if you didn't defer.
 */
#	define CX_END_WRITABLE(locvar)	if((locvar)){ xgalloca(0, __FILE__, __LINE__) ; CX_writable_strings=(locvar) ; }

#endif
