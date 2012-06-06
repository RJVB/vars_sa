#define MXT_SOURCE

#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#ifdef VARS_STANDALONE
#	include "vars-standalone.h"
#endif

#include <sys/types.h>
#include "mxt.h"
#define PI M_PI

#undef MAXDOUBLE

#ifndef VARS_STANDALONE
#	include "cx.h"
#endif

#include <float.h>

IDENTIFY( "Extended math functions - RJB");

long _lround(x)
double x;
{
	return( (x>= 0)? ( (long)(x+ 0.5) )  : ( (long)(x- 0.5) ) );
}

int _round(x)
double x;
{
	return( (x>= 0)? ( (int)(x+ 0.5) )  : ( (int)(x- 0.5) ) );
}


int _dsign(x)
double x;
{
	return( ( x< 0 )? -1: 1);
}


int _sign(x)
int x;
{
	return( ( x< 0)? -1: 1);
}


double fraxion( x)
double x;
{	 double y;
	return( modf( x, &y));
}	


static double *sintab;
int sintab_set= 0, sintab_res= 360;		
char belll[]= { 0x07, 0x00};
#define PI_180	1.745329251994329577e-02

int set_sintab()
{	 

	register int i;
	register double x, *st;

	if( !sintab_set){
		  if( calloc_error( sintab, double, 360))
		       return( 0);
		  sintab_set= 1;
	}
	if( sintab_set){
		  for(i= 0, st= sintab; i< 360; i++){
		       x= (double) ( i* PI_180 );
		       *st++ = sin(x);
		       if( !( i% 180)) {
				fputs( "%s", stderr);
				fflush( stderr);
			  }
		  }
	}	  
	return( sintab_set);
}
  

double sinus(x, l)
double x, l;
{
	int xx;
	
	xx= abs( round( x* (360.0/ l) ) % 360 );
	return( (double) dsign(x) * sintab[xx] );
}


double cosinus(x, l)
double x, l;
{
	int xx;
	
	xx= abs( round( x* (360/ l)+ 90 ) % 360);
	return( (double) sintab[ xx] );
}


void free_sintab()
{
	if( sintab_set){
		  free( sintab);
		  sintab_set= 0;
	}
}


int blockwave( x, l)
double x, l;
{
		return( (int)( ( sinus( x, l)>= 0)? 1 : -1 ) );
}


int impulse( x, l, tol)
double x, l, tol;
{	  double y, z, n;
		y= fabs( modf( x/ l, &n) );
		z= fabs( (tol)/ (2* l) );
		return( ( ( y)<= z || ( 1- y)<= z ) );
}

#ifndef _UNIX_C_
double fmod( x, y)
double x, y;
{	double z;
	return( y* modf( x/ y, &z));	
}
#endif

unsigned short random_seed[3];

/* RJVB 20030824: make sure to pass correct argument to seed48()... */
int randomise()				/* randomise the random generators */
{	unsigned short _seed[3];
	long *seed= (long*) _seed;
	int i, n;
#ifndef VARS_STANDALONE
	extern DEFMETHOD(seed48_method, (unsigned short seed[3]), unsigned short*);
#endif

	*seed= (long)time(NULL);
	if( (i= 3- (sizeof(_seed)- sizeof(*seed))/sizeof(unsigned short))< 3 ){
		_seed[i]= ~ *seed;
	}
	srand( (unsigned int) *seed);
#ifndef VARS_STANDALONE
	memcpy( random_seed, (*seed48_method)(_seed), 3* sizeof(unsigned short));
#else
	memcpy( random_seed, seed48(_seed), 3* sizeof(unsigned short));
#endif
	n= abs(rand()) % 500;
	for( i= 0; i< n; i++){
		drand();
		rand();
	}
	return( i);
}

int Permute( a, n, t)			/* permute array a of length n and type t */
void *a;
int n, t;
{	double *d; double *dd, ddd;
	int *i, j; int *ii, iii;
	long *l;	long *ll, lll;
	int y;
	
	switch( t){
		case DOUBLES:
			dd= d= (double *) a;
			for( j= 0; j< n; j++){
				y= (int)( n* drand());
				ddd= *d;
				*d++= dd[ y];
				dd[ y]= ddd;
			}
			return( j);
			break;
		case INTS:
			ii= i= (int *) a;
			for( j= 0; j< n; j++){
				y= (int)(n* drand());
#ifdef DEBUG
	if( y>= n){
		fputs( "File ", stderr);
		fputs( __FILE__, stderr);
		fputs( " # y>=n in Permute()\n", stderr);
	}
#endif
				iii= *i;
				*i++= ii[ y];
				ii[ y]= iii;
			}
			return( j);
			break;
		case LONGS:
			ll= l= (long *) a;
			for( j= 0; j< n; j++){
				y= (int)( n* drand());
				lll= *l;
				*l++= ll[ y];
				ll[ y]= lll;
			}
			return( j);
			break;
		default:
			return( 0);
			break;
	}
}

/* calculate atan( dy/ dx) with the result in [0,2PI]	*/

double atan3( dy, dx)
double dy, dx;
{	double tan;
	tan= atan2( dy, dx);
	return( (tan< 0.0)? tan + M_2PI : tan);
}

/* calculate sine and cosine at once	*/

double FFPsincos( co, arg)
double *co, arg;
{
	*co= cos( arg);
	return( sin( arg));
}


/* add p% noise to an array of bytes of size x, y. returns the	*/
/* actual noise percentage added.								*/

double noise_add( p, pic, x, y)
double p;
unsigned char *pic;
int x, y;
{
	register int ix, iy;
	register long Index, t;

	p/= 100.0;
	for( iy= 0, Index= 0L, t= 0L; iy< y; iy++)
		for( ix= 0; ix< x; ix++, Index++)
			if( drand()< p){
				pic[ Index]^= (unsigned char) ( 255.* drand());
				t+= 1L;
			}
	p= t* 100./ ( x* y);
	printf( "Noise added: %g%%\n", p);
	return( p);
}

double SIN(x)						/* linear approx. of a sine	*/
double x;
{	double xx;
	xx= fabs( fmod( x- M_PI_2, M_2PI));		/* sin(x)== cos(x-0.5PI)	*/
	if( xx< M_PI)
		return( 1.- xx* M_2_PI);
	else
		return( -3.+ xx* M_2_PI);
}

double COS(x)						/* linear approx. of a cosine	*/
double x;
{	double xx;
	xx= fabs( fmod( x, M_2PI));
	if( xx< M_PI)
		return( 1.- xx* M_2_PI);
	else
		return( -3.+ xx* M_2_PI);
}

void SINCOS(x, s, c)				/* linear approx. of a sine	& cosine	*/
double x, *s, *c;
{	double xx, yy;
	yy= fmod( x, M_2PI);
	xx= fabs( yy);
	if( xx< M_PI)
		*c= ( 1.- xx* M_2_PI);
	else
		*c= ( -3.+ xx* M_2_PI);
	xx= fabs( ((yy< 0.0)? M_2PI+ yy : yy)- M_PI_2);
	if( xx< M_PI)
		*s= ( 1.- xx* M_2_PI);
	else
		*s= ( -3.+ xx* M_2_PI);
}

static double Gonio_Base_Value= 360.0, Gonio_Base_Value_2= 180.0, Gonio_Base_Value_4= 90.0;
double Units_per_Radian= 57.2957795130823;

double Gonio_Base( base)
double base;
{
	if( base){
		Units_per_Radian= (Gonio_Base_Value= base)/ M_2PI;
		Gonio_Base_Value_4= (Gonio_Base_Value_2= base/ 2.0)/ 2.0;
	}
	return( Gonio_Base_Value);
}

double _Sin( x)
double x;
{
	return( Gonio( sin, x) );
}

double _Cos( x)
double x;
{
	return( Gonio( cos, x) );
}

double _Tan(x)
double x;
{
	return( Gonio( tan, x) );
}

double _ASin(x)
double x;
{
	return( InvGonio( asin, x) );
}

double _ACos(x)
double x;
{
	return( InvGonio( acos, x) );
}

double _ATan(x)
double x;
{
	return( InvGonio( atan, x) );
}

double _ATan2(x, y)
double x, y;
{
	return( atan2(x, y)* Units_per_Radian);
}

double _Arg(x, y)
double x, y;
{
	return( atan3(x, y)* Units_per_Radian);
}

/* An empty SimpleStats structure:	*/
SimpleStats EmptySimpleStats= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
SimpleAngleStats EmptySimpleAngleStats= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Add data to a SimpleStats structure	*/
SimpleStats *SS_Add_Data( a, count, sum, weight)
SimpleStats *a;
long count;
double sum, weight;
{
	SS_Add_Data_( *a, count, sum, weight);
	return( a);
}

/* Add squared data to a SimpleStats structure	*/
SimpleStats *SS_Add_Squared_Data( a, count, sumsq, weight)
SimpleStats *a;
long count;
double sumsq, weight;
{
	SS_Add_Squared_Data_( *a, count, sumsq, weight);
	return( a);
}

/* Add SimpleStats b to SimpleStats a, returning a	*/
SimpleStats *SS_Add( a,b)
SimpleStats *a, *b;
{
	SS_Add_( *a, *b);
	return( a);
}

/* Sum SimpleStats a and SimpleStats b, returning a
 * pointer to a static SimpleStats
 */
SimpleStats *SS_Sum( a,b)
SimpleStats *a, *b;
{  static SimpleStats sum;
	sum= *a;
	SS_Add_( sum, *b);
	return( &sum);
}

/* return the mean of a dataset as described by the
 * SimpleStats structure *SS. Note that the SS_ routines
 * expect that both the count and the weight_sum fields are
 * set.
 */
double SS_Mean( SS)
SimpleStats *SS;
{  double weight_sum;
	if( SS->count && (weight_sum= SS->weight_sum) ){
		return( SS->mean= SS->sum/ weight_sum );
	}
	else
		return( SS->mean= 0.0);
}

double SS_Mean_Div( SSa, SSb)
SimpleStats *SSa, *SSb;
{  double weight_suma, weight_sumb;
	if( SSa->sum && SSa->count && (weight_suma= SSa->weight_sum) &&
		SSb->sum && SSb->count && (weight_sumb= SSb->weight_sum)
	){
		return( (SSa->sum/ weight_suma) / (SSb->sum/ weight_sumb) );
	}
	else
		return( 0.0);
}

double SS_weight_scaler= 1;

/* return the standard deviation of a dataset as described by the
 * SimpleStats structure *SS. The same considerations as given under
 * st_dev and SS_Mean hold.
 */
double SS_St_Dev( SS)
SimpleStats *SS;
{	double sqrt(), stdv;
   long count= SS->count;
   double weight_sum= SS->weight_sum,
	sum= SS->sum,
	sum_sqr= SS->sum_sqr, f= 1;

	if( count== 1L )
		return( SS->stdv= 0.0);
	if( count <= 0.0 || weight_sum<= 0 )
		return( SS->stdv= -1.0);
	  /* If the sum of the weight is smaller than one, we multiply all weights with
	   \ a common 10-fold, such that their sum becomes larger than 1. This influences the
	   \ result (somewhat), so a) the situation should be avoided and b) the factor used
	   \ should maybe have a different value.
	   */
	if( weight_sum<= 1.0 ){
		while( f* weight_sum<= 1.0 ){
			f*= 10;
		}
		sum_sqr*= f;
		sum*= f;
		weight_sum*= f;
	}
	SS_weight_scaler= f;
	if( (stdv= (sum_sqr- ((sum* sum)/ weight_sum) )/ (weight_sum - 1.0) )>= 0.0 )
		return( SS->stdv= sqrt( stdv) ); 
	else
		return( SS->stdv= -1.0);
}

double SS_Skew( SS)
SimpleStats *SS;
{ long count= SS->count;
  double weight_sum= SS->weight_sum,
	sum= SS->sum,
	sum_sqr= SS->sum_sqr, sum_cub= SS->sum_cub;

	set_NaN(SS->skew);
	if( count== 1L ){
		SS->skew= 0.0;
		return( 0.0);
	}
	if( count <= 0.0 || weight_sum<= 0 )
		return( 0.0);
	if( SS_St_Dev(SS)<= 0 ){
		if( SS->stdv== 0 ){
			SS->skew= 0;
		}
		return(0);
	}
	  /* If the sum of the weight is smaller than one, we multiply all weights with
	   \ a common 10-fold, such that their sum becomes larger than 1. This influences the
	   \ result (somewhat), so a) the situation should be avoided and b) the factor used
	   \ should maybe have a different value.
	   */
	if( SS_weight_scaler!= 1.0 ){
		sum_sqr*= SS_weight_scaler;
		sum_cub*= SS_weight_scaler;
		sum*= SS_weight_scaler;
		weight_sum*= SS_weight_scaler;
	}
	SS->mean= sum/ weight_sum;
	SS->skew= (3* SS->mean* sum_sqr- sum_cub- 2* (sum*sum*sum)/(count*count))/ (SS->stdv*count);
	return( SS->skew );
}

char *SS_sprint( char *buffer, char *format, char *sep, double min_err, SimpleStats *a)
{  static char _buffer[100][256];
   static int bnr= 0;
   double stdv;
   int local= False;

	if( !a ){
		return( "<NULL pointer>" );
	}
	if( !buffer ){
		buffer= _buffer[bnr];
		bnr= (bnr+1) % 100;
		local= True;
	}
	d2str( SS_Mean(a), format, buffer);
	if( (stdv= SS_St_Dev(a))> min_err ){
	  char Format[128];
// 		sprintf( Format, "%%s%s%%s", sep );
		sprintf( Format, "%s%%s", sep );
		if( strlen(Format)> sizeof(Format)/sizeof(char) ){
			fprintf( stderr, "SS_sprint(): format string \"%s\" is %d bytes too long (poss. fatal)\n",
				Format, strlen(Format)- sizeof(Format)/sizeof(char)
			);
			fflush(stderr);
		}
// 		sprintf( buffer, Format, buffer, d2str( stdv, format, NULL) );
		sprintf( &buffer[strlen(buffer)-1], Format, d2str( stdv, format, NULL) );
	}
	if( local && strlen(buffer)> sizeof(_buffer[0])/sizeof(char) ){
		fprintf( stderr, "SS_sprint(): result is %d bytes too long (poss. fatal)\n",
			strlen(buffer)- sizeof(_buffer[0])/sizeof(char)
		);
		fflush(stderr);
	}
	return( buffer );
}

char *SS_sprint_full( char *buffer, char *format, char *sep, double min_err, SimpleStats *a)
{  static char _buffer[100][256];
   static int bnr= 0;
   char Format[256];
   int local= False;
	if( !a ){
		return( "<NULL pointer>" );
	}
	if( !buffer ){
		buffer= _buffer[bnr];
		bnr= (bnr+1) % 100;
		local= True;
	}
	SS_sprint( buffer, format, sep, min_err, a);
	if( a->takes ){
		sprintf( Format, " c=%lu(%%s*%%s) [%%s,%%s] t=%lu", (long) a->count, (long) a->takes );
	}
	else{
		sprintf( Format, " c=%lu(%%s*%%s) [%%s,%%s]", (long) a->count );
	}
#ifdef SS_SKEW
	strcat( Format, " skew=%s");
	SS_Skew( a );
#endif
	if( strlen(Format)> sizeof(Format)/sizeof(char) ){
		fprintf( stderr, "SS_sprint_full(): format string \"%s\" is %d bytes too long (poss. fatal)\n",
			Format, strlen(Format)- sizeof(Format)/sizeof(char)
		);
		fflush(stderr);
	}
	sprintf( &buffer[strlen(buffer)-1], Format,
		d2str( a->weight_sum, format, NULL),
		d2str( SS_weight_scaler, format, NULL),
		d2str( a->min, format, NULL),
		d2str( a->max, format, NULL)
#ifdef SS_SKEW
		, d2str( a->skew, format, NULL)
#endif
	);
	if( local && strlen(buffer)> sizeof(_buffer[0])/sizeof(char) ){
		fprintf( stderr, "SS_sprint_full(): result is %d bytes too long (poss. fatal)\n",
			strlen(buffer)- sizeof(_buffer[0])/sizeof(char)
		);
		fflush(stderr);
	}
	return( buffer );
}

/* Add data to a SimpleAngleStats structure	*/
SimpleAngleStats *SAS_Add_Data( a, count, sum, weight)
SimpleAngleStats *a;
long count;
double sum, weight;
{
	SAS_Add_Data_( *a, count, sum, weight);
	return(a);
}

/* Add squared data to a SimpleAngleStats structure.
 * The square-root of the data must be provided to allow for
 * a negative root.
 */
SimpleAngleStats *SAS_Add_Squared_Data( a, count, sum, sumsq, weight)
SimpleAngleStats *a;
long count;
double sum, sumsq, weight;
{
	SAS_Add_Squared_Data_( *a, count, sum, sumsq, weight);
	return(a);
}

/* Add SimpleAngleStats b to SimpleAngleStats a, returning a	*/
SimpleAngleStats *SAS_Add( a,b)
SimpleAngleStats *a, *b;
{
	SAS_Add_( *a, *b);
	return( a);
}

/* Sum SimpleAngleStats a and SimpleAngleStats b, returning a
 * pointer to a static SimpleAngleStats
 */
SimpleAngleStats *SAS_Sum( a,b)
SimpleAngleStats *a, *b;
{  static SimpleAngleStats sum;
	sum= *a;
	SAS_Add_( sum, *b);
	return( &sum);
}

/* return arg(x,y) in degrees [-PI,PI]	*/
double _atan2( x, y)
double x, y;
{ 
	if( x> 0.0)
		return( atan(y/x) );
	else if( x< 0.0){
		if( y>= 0.0)
			return( M_PI+ (atan(y/x)));
		else
			return( (atan(y/x))- M_PI);
	}
	else{
		if( y> 0.0)
			return( hPI );
		else if( y< 0.0)
			return( -hPI );
		else
			return( 0.0);
	}
}

/* return arg(x,y) in degrees [0,twoPI]	*/
double _atan3( x, y)
double x, y;
{ 
	if( x> 0.0){
		if( y>= 0.0)
			return( (atan(y/x)));
		else
			return( twoPI+ (atan(y/x)));
	}
	else if( x< 0.0){
		return( M_PI+ (atan(y/x)));
	}
	else{
		if( y> 0.0)
			return( hPI );
		else if( y< 0.0)
			return( M_PI + hPI );
		else
			return( 0.0);
	}
}

/* return the mean of a dataset as described by the
 * SimpleAngleStats structure *SAS. Note that the SAS_ routines
 * expect that both the count and the weight_sum fields are
 * set.
 */
double SAS_Mean( SAS)
SimpleAngleStats *SAS;
{  double gb= Gonio_Base_Value, pav, nav, av= 0.0;
   int ok= 0;
	if( SAS->Gonio_Base!= gb)
		Gonio_Base( SAS->Gonio_Base);
	if( SAS->pos_count && SAS->pos_weight_sum){
		pav= SAS->pos_sum/ SAS->pos_weight_sum;
		ok+= 1;
	}
	if( SAS->neg_count && SAS->neg_weight_sum){
		nav= SAS->neg_sum/ SAS->neg_weight_sum;
		ok+= 2;
	}
	switch( ok){
		case 1:
			av= pav;
			break;
		case 2:
			av= nav;
			break;
		case 3:{
/* 			av= ATan2( Sin(pav)+Sin(nav) , Cos(pav)+Cos(nav) );	*/
			av= Units_per_Radian * _atan3( Cos(pav)+Cos(nav) , Sin(pav)+Sin(nav) );
			break;
		}
	}
	Gonio_Base( gb);
	return( av);
}

/* return the standard deviation of a dataset as described by the
 * SimpleAngleStats structure *SAS. The same considerations as given under
 * st_dev and SAS_Mean hold.
 */
double SAS_St_Dev( SAS)
SimpleAngleStats *SAS;
{  double sqrt(), stdv,
	  weight_sum= SAS->pos_weight_sum+ SAS->neg_weight_sum,
	  sum= SAS->pos_sum + SAS->neg_sum,	/* SAS_Mean( SAS),	*/
	  sum_sqr= SAS->sum_sqr, f= 1.0;
   long count= SAS->pos_count+ SAS->neg_count;

	if( count== 1L )
		return( SAS->stdv= 0.0);
	if( count <= 0 || weight_sum<= 0)
		return( SAS->stdv= -1.0);
	  /* If the sum of the weight is smaller than one, we multiply all weights with
	   \ a common 10-fold, such that their sum becomes larger than 1. This influences the
	   \ result (somewhat), so a) the situation should be avoided and b) the factor used
	   \ should maybe have a different value.
	   */
	if( weight_sum<= 1.0 ){
		while( f* weight_sum<= 1.0 ){
			f*= 10;
		}
		sum_sqr*= f;
		sum*= f;
		weight_sum*= f;
	}
	if( (stdv= (sum_sqr- (sum* sum)/ count)/ (count - 1.0) )>= 0.0 )
		return( SAS->stdv= sqrt( stdv) ); 
	else
		return( SAS->stdv= -1.0);
}

char *SAS_sprint( char *buffer, char *format, char *sep, double min_err, SimpleAngleStats *a)
{  static char _buffer[256];
   double stdv;

	if( !a ){
		return( "" );
	}
	if( !buffer ){
		buffer= _buffer;
	}
/* 	sprintf( buffer, format, SAS_Mean(a) );	*/
	d2str( SAS_Mean(a), format, buffer);
	if( (stdv= SAS_St_Dev(a))> min_err ){
/* 	  char Format[128];	*/
		strcat( buffer, sep);
/* 
		sprintf( Format, "%%s%s", format );
		sprintf( buffer, Format, stdv );
 */
		sprintf( buffer, "%s%s", buffer, d2str( stdv, format, NULL) );
	}
	return( buffer );
}

char *SAS_sprint_full( char *buffer, char *format, char *sep, double min_err, SimpleAngleStats *a)
{  static char _buffer[512];
   char Format[128];
	if( !a ){
		return( "" );
	}
	if( !buffer ){
		buffer= _buffer;
	}
	SAS_sprint( buffer, format, sep, min_err, a);
/* 
	sprintf( Format, "%%s c=%ld,%ld [%s,%s]", (long) a->pos_count, (long) a->neg_count, format, format );
	sprintf( buffer, Format, buffer, a->min, a->max );
 */
	sprintf( Format, "%%s c=%ld,%ld [%%s,%%s]", (long) a->pos_count, (long) a->neg_count );
	sprintf( buffer, Format, buffer, d2str(a->min, "%g", NULL), d2str(a->max, "%g", NULL) );
	return( buffer );
}

/* return arg(x,y) in degrees [0,360]	*/
double phival( x, y)
double x, y;
{ 
	if( x> 0.0){
		if( y>= 0.0)
			return( degrees(atan(y/x)));
		else
			return( 360.0+ degrees(atan(y/x)));
	}
	else if( x< 0.0){
		return( 180.0+ degrees(atan(y/x)));
	}
	else{
		if( y> 0.0)
			return( 90.0);
		else if( y< 0.0)
			return( 270.0);
		else
			return( 0.0);
	}
}

/* return arg(x,y) in degrees [-180,180]	*/
double phival2( x, y)
double x, y;
{ 
	if( x> 0.0)
		return( degrees(atan(y/x)));
	else if( x< 0.0){
		if( y>= 0.0)
			return( 180.0+ degrees(atan(y/x)));
		else
			return( degrees(atan(y/x))- 180.0);
	}
	else{
		if( y> 0.0)
			return( 90.0);
		else if( y< 0.0)
			return( -90.0);
		else
			return( 0.0);
	}
}

/* return phi in <-180,180]	*/
double conv_angle( phi)
double phi;
{
	double x;
#ifndef VARS_STANDALONE
	struct exception exc;

	if( NaNorInf(phi) ){
		exc.name= "fmod";
		exc.type= DOMAIN;
		exc.arg1= phi;
		exc.arg2= 0.0;
		exc.retval= 0.0;
		if( !matherr( &exc) )
			errno= EDOM;
		return(exc.retval);
	}
#endif
#ifdef MCH_AMIGA
	/* phi mod 360 : AZTEC MFFP inverts sign!!
	 * phi mod 360 : AZTEC IEEE conserves sign!!
	 */
	Sign= ( phi< 0)? -1 : 1;
	x= Sign* fabs( 360.0* modf( phi/ 360.0, &y));
#endif
#ifdef _MAC_FINDER_
	Sign= 1;
	x= 360.0* modf( phi/ 360.0, &y); 		/* phi mod 360 */
#endif
#ifdef _UNIX_C_
	x= fmod( phi, 360.0);
#endif
	if( x> -180.0)
		return( (x<= 180.0)? x : x- 360.0);
	else
		return( 360.0+ x);
}

/* return phi in <-base/2,base/2]	*/
double conv_angle_( phi, base)
double phi, base;
{
	double x, hbase= base/2;
#ifndef VARS_STANDALONE
	struct exception exc;
	
	if( NaNorInf(phi) ){
		exc.name= "fmod";
		exc.type= DOMAIN;
		exc.arg1= phi;
		exc.arg2= 0.0;
		exc.retval= 0.0;
		if( !matherr( &exc) )
			errno= EDOM;
		return(exc.retval);
	}
#endif
#ifdef MCH_AMIGA
	/* phi mod 360 : AZTEC MFFP inverts sign!!
	 * phi mod 360 : AZTEC IEEE conserves sign!!
	 */
	Sign= ( phi< 0)? -1 : 1;
	x= Sign* fabs( base* modf( phi/ base, &y));
#endif
#ifdef _MAC_FINDER_
	Sign= 1;
	x= base* modf( phi/ base, &y); 		/* phi mod 360 */
#endif
#ifdef _UNIX_C_
	x= fmod( phi, base);
#endif
	if( x> -hbase)
		return( (x<= hbase)? x : x- base);
	else
		return( base+ x);
}

/* return phi in [-180,180]	*/
double conv_angle2( phi)
double phi;
{
	double x;
#ifndef VARS_STANDALONE
	struct exception exc;
	
	if( NaNorInf(phi) ){
		exc.name= "fmod";
		exc.type= DOMAIN;
		exc.arg1= phi;
		exc.arg2= 0.0;
		exc.retval= 0.0;
		if( !matherr( &exc) )
			errno= EDOM;
		return(exc.retval);
	}
#endif
#ifdef MCH_AMIGA
	Sign= ( phi< 0)? -1 : 1;
	x= Sign* fabs( 360.0* modf( phi/ 360.0, &y));
#endif
#ifdef _MAC_FINDER_
	Sign= 1;
	x= 360.0* modf( phi/ 360.0, &y); 		/* phi mod 360 */
#endif
#ifdef _UNIX_C_
	x= fmod( phi, 360.0);
#endif
	if( x>= -180.0)					/* conv_angle(-180) returns -180	*/
		return( (x<= 180.0)? x : x- 360.0);
	else
		return( 360.0+ x);
}

/* return an angle between [0,360> */
double mod_angle( phi)
double phi;
{
	double x;
#ifndef VARS_STANDALONE
	struct exception exc;

	if( NaNorInf(phi) ){
		exc.name= "fmod";
		exc.type= DOMAIN;
		exc.arg1= phi;
		exc.arg2= 0.0;
		exc.retval= 0.0;
		if( !matherr( &exc) )
			errno= EDOM;
		return(exc.retval);
	}
#endif
#ifdef _MAC_FINDER_
	Sign= 1;
	x= 360.0* modf( phi/ 360.0, &y);		/* phi mod 360 */
#endif
#ifdef _UNIX_C_
	x= fmod( phi, 360.0);
#endif
#ifdef MCH_AMIGA
	Sign= ( phi< 0)? -1 : 1;				/* phi mod 360; Aztec modf returns -frac.. */
	x= Sign* fabs( 360.0* modf( phi/ 360.0, &y));
#endif
	return( (x>= 0.0)? x : 360.0+ x);
}

/* return an angle between [0,base> */
double mod_angle_( phi, base)
double phi, base;
{
	double x;
#ifndef VARS_STANDALONE
	struct exception exc;

	if( NaNorInf(phi) ){
		exc.name= "fmod";
		exc.type= DOMAIN;
		exc.arg1= phi;
		exc.arg2= 0.0;
		exc.retval= 0.0;
		if( !matherr( &exc) )
			errno= EDOM;
		return(exc.retval);
	}
#endif
#ifdef _MAC_FINDER_
	Sign= 1;
	x= base* modf( phi/ base, &y);		/* phi mod 360 */
#endif
#ifdef _UNIX_C_
	x= fmod( phi, base);
#endif
#ifdef MCH_AMIGA
	Sign= ( phi< 0)? -1 : 1;				/* phi mod 360; Aztec modf returns -frac.. */
	x= Sign* fabs( base* modf( phi/ base, &y));
#endif
	return( (x>= 0.0)? x : base+ x);
}

double _Euclidian_SQDist(dx,dy,dz)
double dx, dy, dz;
{
	return( Euclidian_SQDist(dx,dy,dz) );
}

double _Euclidian_Dist(dx,dy,dz)
double dx, dy, dz;
{
	return( Euclidian_Dist(dx,dy,dz) );
}

/* Return the visual angle in degrees spanned by a circular object of
 \ radius r at distance R. This is approximated by arcsin(r/R),
 \ which is approximated by r/R if r/R< 0.1 (then the difference
 \ is smaller than 0.01 degrees). If R=0 or R<r then 360 degrees is returned.
 */
double SubVisAngle_threshold= 0.1;
double SubVisAngle( double R, double r )
{ double ret;
	if( R && R>= r ){
		if( (ret= fabs(r/ R))>= SubVisAngle_threshold ){
			ret= (asin(ret));
		}
	}
	else{
		ret= M_2PI;
	}
	return( degrees(ret) );
}

double curt(x)
double x;
{  static double one_third= 1.0/ 3.0;
	if( x== 0.0)
		return(0.0);
	else if( x> 0)
		return( pow( x, one_third) );
	else
		return( - pow( -x, one_third) );
}

Leaky_Int_Par *Init_Leaky_Int_Par( lip)
Leaky_Int_Par *lip;
{ double *tau= lip->tau, *delta_t= lip->delta_t;
	if( !tau || !delta_t)
		return(NULL);
	if( *tau> 0)
		lip->memory= exp( - *delta_t / *tau);
	else if( *tau< 0){
		lip->memory= - *tau;
		*tau= (- *delta_t) / log( lip->memory);
	}
	else
		lip->memory= 0.0;
	lip->DeltaT= *delta_t;
	return( lip);
}

double Leaky_Int_Response( Leaky_Int_Par *lip, double *state, double input, double gain)
{
	if( lip->DeltaT != *lip->delta_t ){
		Init_Leaky_Int_Par( lip );
	}
	*state= lip->memory* *state + gain * input;
	if( lip->state ){
		*lip->state= *state;
	}
	return(*state);
}

double Normalised_Leaky_Int_Response( Leaky_Int_Par *lip, double *state, double input)
{
	if( lip->DeltaT != *lip->delta_t ){
		Init_Leaky_Int_Par( lip );
	}
	*state= lip->memory* *state + (1.0- lip->memory) * input;
	if( lip->state ){
		*lip->state= *state;
	}
	return(*state);
}

int _NaNorInf( x)
double *x;
{
	return( NaNorInf( *x ) );
}

int _NaN( x)
double *x;
{
	return( (int) NaN( *x ) );
}

int _INF( x)
double *x;
{
	return( INF( *x ) );
}

DEFUN( _set_NaN, (double *x), IEEEfp* );
IEEEfp *_set_NaN( x)
double *x;
{
	set_NaN( *x);
	return( (IEEEfp*) x);
}

int _Inf( x)
double *x;
{
	return( Inf( *x) );
}

IEEEfp *_set_Inf( x, sign)
double *x;
int sign;
{
	set_Inf( *x, sign);
	return( (IEEEfp*) x);
}

IEEEfp *_set_INF( x)
double *x;
{
	set_INF( *x);
	return( (IEEEfp*) x);
}

double Entier(double x)
{
	return( (x>0)? floor(x) : ceil(x) );
}

#define StdErr cx_stderr

/* Searching for possible (shorter printing) fractions does take some time, so some of
 \ the less common denominators are commented out. A negative value is a factor to apply
 \ to the other, positive denominators to be tested.
 */
int _d2str_factors_ext[]=
			{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
				-3, -6, -10, -11, -33, -66, -100, -111, -333, -666, -1000, -1111, -3333, -6666, 0 },
	_d2str_factors[]=
			{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 100, 200, 1000, 2000, 0 },
	*d2str_factors= _d2str_factors;

/* Given a real 0<=d<1, find in the array DF an element df > 0 such that 
 \ d * df * scale is integer.
 */
static int find_d2str_factor( double d, int *DF, int scale)
{  int *df= DF;
   double t;
   int n;
	while( d>= 0 && d< 1 && df && *df ){
		if( *df> 0 ){
			n= scale* *df;
			t= d* n;
/* 			if( floor(t) == t ){	*/
			if( fabs(floor(t) - t)<= DBL_EPSILON ){
				return( n );
			}
		}
		df++;
	}
	return( 0 );
}

#define D2STR_BUFS	100

int Allow_Fractions= 1;

/* The internal buffers should be long enough to print the longest doubles.
 \ Since %lf prints 1e308 as a 1 with 308 0s and then some, we must take
 \ this possibility into account... DBL_MAX_10_EXP+12=320 on most platforms.
 */
char *d2str( double d, char *Format , char *buf )
#ifndef OLD_D2STR
{  static char internal_buf[D2STR_BUFS][DBL_MAX_10_EXP+12], *template= NULL, nan_buf[9*sizeof(long)];
   static int buf_nr= 0, temp_len= 0;
   int Sign, plen= 0, mlen, frac_len= 0;
   int nan= False, ext_frac= False;
   char *format= Format, *buf_arg= buf, frac_buf[DBL_MAX_10_EXP+12], *slash;
	if( !buf ){
	 /* use one of the internal buffers, cycling through them.	*/
		buf= internal_buf[buf_nr];
		buf_nr= (buf_nr+1) % D2STR_BUFS;
		mlen= DBL_MAX_10_EXP+12;
	}
	else{
		mlen= -1;
	}
	nan_buf[0]= '\0';
	if( NaN(d) ){
	  unsigned long nan_val= NaN(d);
		sprintf( nan_buf, "%sNaN%lu",
			(I3Ed(d)->s.s)? "-" : "",
			(nan_val & 0x000ff000) >> 12
		);
		nan= True;
	}
	else if( (Sign=Inf(d)) ){
		if( Sign> 0 ){
			strcpy( nan_buf, "Inf");
		}
		else if( Sign< 0){
			strcpy( nan_buf, "-Inf");
		}
		nan= True;
	}
	/* else	*/{
	  char *c;
	  int len;
		if( !format ){
			format= "%g";
		}
		else if( format[0]== '%' && format[1]== '!' ){
		  char *c= &format[2], *sform;
		  int pedantic, signif, fallback= False;
			if( *c== '!' ){
				c++;
				pedantic= True;
			}
			else{
				pedantic= False;
			}
			if( nan ){
				format= "%g";
			}
			else if( sscanf( c, "%d", &signif) && signif> 0 ){
			  double pd= fabs(d);
			  double order;
			  int iorder;
				if( pd> 0 || pedantic ){
					if( pd ){
						order= log10(pd);
/* 						iorder= (int) ((order< 0)? ceil(order) : floor(order));	*/
						iorder= (int) floor(order);
					}
					else{
						iorder= 0;
					}
					if( iorder< -3 || iorder>= signif ){
						sform= "%.*e";
						iorder= 0;
					}
					else{
						if( !pedantic && fabs(floor(d)- d)<= DBL_EPSILON ){
						  /* Ignore the precision argument:	*/
/* 							sform= "%2$g";	*/
							sform= "%g";
							fallback= True;
						}
						else{
						  /* make sure we print all the requested digits
						   \ (%g won't print trailing zeroes).
						   \ Also add a small, significance-dependent value
						   \ to ensure that e.g. with '%!4g', 10.555 prints
						   \ as 10.56 (i.e. that the last 5 rounds up the
						   \ preceding digit.
						   */
							sform= "%.*f";
							if( d> 0 ){
								d+= pow( 10, iorder- signif- 0.0);
							}
							else{
								d-= pow( 10, iorder- signif- 0.0);
							}
						}
					}
				}
				else{
					iorder= 0;
					  /* sform="%2$g" should cause the 2nd argument after the format string 
					   \ to be printed with "%g", but not all machines appear to support this format.
					   */
					sform= "%g";
					fallback= True;
				}
				if( fallback ){
					sprintf( buf, sform, d );
				}
				else{
					sprintf( buf, sform, signif- iorder- 1, d );
				}
				return( buf );
			}
		}
		if( (len= strlen(format)+1) > temp_len ){
		  char *c= template;
			if( !(template= (char*) calloc( len, 1)) ){
				fprintf( stderr, "matherr::d2str(%g,\"%s\"): can't allocate buffer (%s)\n",
					d, format, serror()
				);
				fflush( stderr );
			}
			else{
				if( c ){ free( c );}
				temp_len= len;
				strcpy( template, format);
				format= template;
			}
		}
		else if( template ){
			strcpy( template, format);
			format= template;
		}
		if( format== template ){
		  /* in this case we may alter the format-string
		   \ this is done if we are printing a nan. In that
		   \ case, the floating printing specification (fps) is
		   \ substituted for a similar '%s'
		   \ a fps has the form "%[<length>.<res>][l]{fFeEgG}";
		   \ the result the form "%[<length>.<res>]s".
		   */
			do{
				c= index( format, '%');
				if( c && c[1]== '%' ){
					c+= 2;
				}
			} while( c && *c && *c!= '%' );
			if( nan && c && *c== '%' ){
			  char *d= &c[1];
				strcpy( buf, nan_buf );
				while( *d && !( index( "gGfFeE", *d) || (index( "lL", *d) && index( "gGfFeE", d[1]))) ){
					d++;
				}
				if( *d ){
					c= d;
					if( *d== 'l' || *d== 'L' ){
						d+= 2;
					}
					else{
						d++;
					}
					*c++= 's';
					while( *d ){
						*c++= *d++;
					}
					*c= '\0';
				}
				else{
					return( buf );
				}
			}
			else if( nan ){
			  /* screwed up	*/
				return( buf );
			}
		}
		else{
			nan= False;
		}
		slash= index( format, '/');
		if( !strcmp( format, "%g'%g/%g") || !slash){
		  /* we handle fractions as a/b (when -1< d <1), or
		   \ else as p'a/b, with |p|> 1, when either it has been
		   \ specifically requested, or when the format does not specify
		   \ a fraction, in which case we only print this way when it does
		   \ not take more space, and Allow_Fractions is true.
		   */
			ext_frac= True;
		}
		if( !nan ){
		  double D= ABS(d), p= (ext_frac && D> 1)? Entier(d) : 0, P= ABS(p), dd= D-P;
			  /* dd is the non-integer part of d	*/
			if( dd && (Allow_Fractions || slash) ){
			  /* Find a denominator f such that d=n/f with n integer and f>0
			   \ d2str_factors is an integer array containing the
			   \ denominators f (positive elements) and scale-factors
			   \ (negative elements) to apply to the denominators.
			   */
			  int *df, f;
			  int scale= 1;
			      /* We need the absolute value of the printable, and possibly its
				  \ integer part (when we are allowed to print as p'a/b)
				  */
				if( d2str_factors== NULL || d2str_factors== _d2str_factors || d2str_factors== _d2str_factors_ext ){
				  /* if we're using 1 of the 2 default factor-lists, select the extended one
				   \ if a fraction was explicitely requested, and the shorter one otherwise.
				   */
					d2str_factors= (slash)? _d2str_factors_ext : _d2str_factors;
				}
				df= d2str_factors;
				if( !(f= find_d2str_factor( dd, d2str_factors, scale )) ){
					  /* Nothing found, check for scale-factors. An array {1,2,3,-10}
					   \ with thus check {1,2,3,10,20,30} as possible denominators
					   */
					  /* try it with a simple factor	*/
					while( df && *df && !f ){
						if( *df< 0 ){
							scale= - *df;
							f= find_d2str_factor( dd, d2str_factors, scale);
						}
						df++;
					}
					df= d2str_factors;
					while( df && *df && !f ){
					  int *df2= d2str_factors;
						if( *df> 1 ){
							while( !f && df2 && *df2 ){
								if( *df2< 0 ){
								  double sc= pow( - *df2, *df );
									if( sc< MAXINT ){
										  /* try it with power *df of factor *df2 ...	*/
										f= find_d2str_factor( dd, d2str_factors, (int) sc );
									}
								}
								df2++;
							}
						}
						df++;
					}
				}
				if( f && f!= 1 ){
				  /* print as (f*d)/f	*/
					if( slash ){
						if( ext_frac ){
							if( p ){
								frac_len= sprintf( frac_buf, format, p, f* dd, (double) f );
							}
							else{
							  /* ext_frac is only true when format=="%g'%g/%g", so we can safely
							   \ make the following simplification:
							   */
								frac_len= sprintf( frac_buf, "%g/%g", f* d, (double) f );
							}
						}
						else{
							frac_len= sprintf( frac_buf, format, f* d, (double) f );
						}
					}
					else{
						if( p ){
							frac_len= sprintf( frac_buf, "%g'%g/%g", p, f* dd, (double) f );
						}
						else{
							frac_len= sprintf( frac_buf, "%g/%g", f* d, (double) f );
						}
					}
				}
			}
		}
		  /* Now make the output	*/
		if( !nan && strneq( format, "1/", 2) ){
		  /* Maybe one day this will be changed to
		   \ handle different cases (e.g. x/%g) where
		   \ x any number
		   */
		  /* Just see the following else if..!	*/
			if( d!= 0 ){
				plen= sprintf( buf, format, 1.0/d);
			}
			else{
				plen= sprintf( buf, &format[2], d);
			}
		}
		else if( (c= slash) ){
			if( !nan ){
				if( frac_len ){
					plen= frac_len;
					strcpy( buf, frac_buf );
				}
				else{
					if( ext_frac ){
					  /* asked for %g'%g/%g, but we didn't find a fraction. So:	*/
						plen= sprintf( buf, "%g", d);
					}
					else{
						*c= '\0';
						plen= sprintf( buf, format, d);
						*c= '/';
					}
				}
			}
			else{
				*c= '\0';
				plen= sprintf( buf, format, nan_buf, 1);
				*c= '/';
			}
		}
		else{
			if( !nan ){
				plen= sprintf( buf, format, d);
			}
			else if( format== template ){
			  /* print the nan_buf according to format. Just in case
			   \ the originale Format was something like '%g/%g', an
			   \ extra argument of 1.0 is added. Now Inf will print like
			   \ Inf/1 .
			   \ 960325: allow printing of <nan> as (possible fraction) to
			   \ prevent Inf/1.
			   */
				plen= sprintf( buf, format, nan_buf, 1.0 );
			}
			if( Allow_Fractions && (frac_len> 0 && frac_len<= plen) ){
				strcpy( buf, frac_buf );
				plen= frac_len;
			}
		}
	}
	if( plen<= 0 && (Format && *Format) ){
		fprintf( cx_stderr, "cx::d2str(%g,\"%s\",0x%lx)=> \"%s\": no output generated: report to sysop (will return %g)\n",
			d, (Format)? Format : "<NULL>", buf_arg, buf, d
		);
		plen= sprintf( buf, "%g", d );
		fflush( cx_stderr );
	}
	if( mlen> 0 && plen> mlen ){
		fprintf( cx_stderr, "cx::d2str(%g): wrote %d bytes in internal buffer of %d\n",
			d, plen, mlen
		);
		fflush( cx_stderr );
	}
	return( buf );
}
#else
{  static char internal_buf[D2STR_BUFS][DBL_MAX_10_EXP+12], *template= NULL, nan_buf[9*sizeof(long)];
   static int buf_nr= 0, temp_len= 0;
   int Sign, plen= -1, mlen;
   int nan= False;
   char *format= Format;
	if( !buf ){
	 /* use one of the internal buffers, cycling through them.	*/
		buf= internal_buf[buf_nr];
		buf_nr= (buf_nr+1) % D2STR_BUFS;
		mlen= DBL_MAX_10_EXP+12;
	}
	else{
		mlen= -1;
	}
	nan_buf[0]= '\0';
	if( NaN(d) ){
	  unsigned long nan_val= NaN(d);
		sprintf( nan_buf, "NaN%lu", (nan_val & 0x000ff000) >> 12);
		nan= True;
	}
	else if( (Sign=Inf(d)) ){
		if( Sign> 0 ){
			strcpy( nan_buf, (use_greek_inf)? GREEK_INF : "Inf");
		}
		else if( Sign< 0){
			strcpy( nan_buf, (use_greek_inf)? GREEK_MIN_INF : "-Inf");
		}
		nan= True;
	}
	/* else	*/{
	  char *c;
	  int len;
		if( !format ){
			format= "%g";
		}
		if( (len= strlen(format)+1) > temp_len ){
		  char *c= template;
			if( !(template= (char*) calloc( len, 1)) ){
				fprintf( stderr, "matherr::d2str(%g,\"%s\"): can't allocate buffer (%s)\n",
					d, format, serror()
				);
				fflush( stderr );
			}
			else{
				if( c ){ free( c );}
				temp_len= len;
				strcpy( template, format);
				format= template;
			}
		}
		else{
			strcpy( template, format);
			format= template;
		}
		if( format== template ){
		  /* in this case we may alter the format-string
		   \ this is done if we are printing a nan. In that
		   \ case, the floating printing specification (fps) is
		   \ substituted for a similar '%s'
		   \ a fps has the form "%[<length>.<res>][l]{fFeEgG}";
		   \ the result the form "%[<length>.<res>]s".
		   */
			do{
				c= index( format, '%');
				if( c[1]== '%' ){
					c+= 2;
				}
			} while( *c && *c!= '%' );
			if( nan && *c== '%' ){
			  char *d= &c[1];
				strcpy( buf, nan_buf );
				while( *d && !index( "gGfFeE", *d) ){
					d++;
					if( *d== 'l' || *d== 'L' && index( "gGfFeE", d[1] ) ){
						d++;
					}
				}
				if( *d ){
					c= d++;
					*c++= 's';
					while( *d ){
						*c++= *d++;
					}
					*c= '\0';
				}
				else{
					return( buf );
				}
			}
			else if( nan ){
			  /* screwed up	*/
				return( buf );
			}
			plen= strlen(buf);
		}
		else{
			nan= False;
		}
		if( !nan && strneq( format, "1/", 2) ){
		  /* Maybe one day this will be changed to
		   \ handle different cases (e.g. x/%g) where
		   \ x any number
		   */
			if( d!= 0 ){
				plen= sprintf( buf, format, 1.0/d);
			}
			else{
				plen= sprintf( buf, &format[2], d);
			}
		}
		else if( !nan && (c= index( format, '/')) ){
		  int *df= d2str_factors, f;
		  int scale= 1;
			if( !(f= find_d2str_factor( d, d2str_factors, scale )) ){
				while( df && *df && !f ){
					if( *df< 0 ){
						scale= - *df;
						f= find_d2str_factor( d, d2str_factors, scale);
					}
					df++;
				}
			}
			if( f && f!= 1 ){
			  /* print as (f*d)/f	*/
				plen= sprintf( buf, format, f* d, (double) f );
			}
			else{
				*c= '\0';
				plen= sprintf( buf, format, d);
				*c= '/';
			}
		}
		else{
			if( !nan ){
				plen= sprintf( buf, format, d);
			}
			else if( format== template ){
			  /* print the nan_buf according to format. Just in case
			   \ the originale Format was something like '%g/%g', an
			   \ extra argument of 1.0 is added. Now Inf will print like
			   \ Inf/1 .
			   */
				plen= sprintf( buf, format, nan_buf, 1.0 );
			}
		}
	}
	if( mlen> 0 ){
	  /* means we can do a check for overwriting the (internal) buffer	*/
#ifdef _APOLLO_SOURCE
		plen= strlen(buf);
#else
		if( plen== -1 || plen> mlen ){
		  /* plen not determined, or maybe wrong... (double check)	*/
			plen= strlen(buf);
		}
#endif
		if( plen> mlen ){
			fprintf( StdErr, "d2str(%g)=\"%s\": wrote %d bytes in internal buffer of %d\n",
				d, buf, plen, mlen
			);
			fflush( StdErr );
		}
	}
	return( buf );
}
#endif

double dfac( x, dx)
double x, dx;
{  double X= x-dx, result= x;
	if( dx<= 0.0){
		set_Inf( result, x);
	}
	else{
		while( X> 0){
			result*= X;
			X-= dx;
		}
	}
	return(result);
}

/* rand_choice(): return a random choice of min or max.  */
double rand_choice( double min, double max)
{
	if( min!= max ){
		if( drand()< 0.5 ){
			return( min);
		}
		else{
			return( max);
		}
	}
	else{
		return( min );
	}
}

/* rand_in(): return a random value in the range [min,max].
 * The average value is (max+min)/2.0 .
 */
double rand_in( min, max)
double min;
double max;
{	double rng= (max-min);

	if( rng ){
		return( drand()* rng+ min );
	}
	else{
		return( min );
	}
}

/* uniform_rand(av,stdv): return a random value taken
 * from a uniform distribution. This is calculated as
 \ av + rand_in[-1.75,1.75]*stdv
 \ which gives approx. the right results.
 */
double uniform_rand( double av, double stdv )
{
	if( stdv ){
		return( av + stdv* (drand()* 3.5- 1.75) );
	}
	else{
		return( av );
	}
}

/* normal_rand(av,stdv): return a random value taken from
 \ a normal distribution. Modified from the Num.Rec gasdev()
 \ function.
 */
double normal_rand( double av, double stdv )
{
	static int iset=0;
	static double gset;
	double fac,r,v1,v2;

	if( !stdv ){
		return( av );
	}
	if  (iset == 0) {
		do {
			v1=2.0* drand()-1.0;
			v2=2.0* drand()-1.0;
			r=v1*v1+v2*v2;
		} while (r >= 1.0);
		fac= stdv * sqrt(-2.0*log(r)/r);
		gset= v1*fac;
		iset=1;
		return( av + v2*fac );
	} else {
		iset=0;
		return av + gset;
	}
}

/* normal_in(): return a random value from a normal distribution in the range [min,max].
 * The average value is (max+min)/2.0 .
 */
double normal_in( double min, double max)
{	double av= (min+max)/2, stdv= (max-min)/6, r;

	if( stdv ){
		CLIP_EXPR( r, normal_rand( av, stdv ), min, max);
		return( r );
	}
	else{
		return( av );
	}
}

/* abnormal_rand(av,stdv): return a random value taken from
 \ an abnormal distribution
 */
double abnormal_rand( double av, double stdv )
{  double r;
	if( !stdv ){
		return( av );
	}
	r= drand()- 0.5;
	return( av + stdv * ((r< 0)? -17.87 : 17.87) *
				( exp( 0.5* r* r) - 1.0 )
	);
}

double dcmp( double b, double a, double prec)
{ double a_low, a_high;

	if( prec>= 0.0 ){
		a_low= (1.0-prec)* a;
		a_high= (1.0+prec)* a;
		if( b< a_low ){
			return( b- a_low );
		}
		else if( b> a_high ){
			return( b- a_high );
		}
		else{
			return( 0.0 );
		}
	}
	else{
	  /* prec< 0 is meant to check if b equals
	   \ a allowing for -prec times the machine imprecision
	   \ DBL_EPSILON. Let's hope that a_low and
	   \ a_high are calculated correctly!
	   */
		prec*= -1.0;
		b-= a;
		if( b< - prec * DBL_EPSILON || b> prec * DBL_EPSILON ){
			return( b - prec * DBL_EPSILON );
		}
		else{
			return( 0.0 );
		}
	}
}

