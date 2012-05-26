/* Extended trigonometric functions, independent from the rest of cx */

#include <math.h>

#if defined(VARS_STANDALONE) && !defined(VARS_STANDALONE_INTERNAL)
#	define VARS_STANDALONE_INTERNAL
#endif

#include <local/mathdef.h>
#include <local/Macros.h>

IDENTIFY("Additional efficient trigonometric functions");

#include "cxtrigo.h"


#ifdef NEEDS_SINCOS

#if 0

/* From http://www.stereopsis.com/computermath101/ */

static char sincosinited = 0;
 
static float sinbuf_[257];
static double dsinbuf_[1025];
 
#define _2pi (2.0 * 3.1415926535897932384626434)
#define _2pif float(_2pi)
 
#if defined(__i386__) || defined(i386)
	  /* little endian: who else but the Intel x86 family? */
	enum { iexp_ = 1, iman_ = 0 };
#else
	enum { iexp_ = 0, iman_ = 1 };
#endif
 
// =====================================================================
// init sincos buffers, single and double precision
// =====================================================================

void initsincos()
{ double angle = _2pi / (4 * 256);
  unsigned int ang;

	if( sincosinited ){
		return;
	}

	for( ang = 0; ang <= 128; ang++ ){
	  double a= angle * ang;
		sinbuf_[ ang] = sin(a) * 2;
		sinbuf_[256 - ang] = cos(a) * 2;
	}
	angle = _2pi / (4 * 1024);
	for( ang = 0; ang <= 512; ang++ ){
	  double a= angle * ang;
		dsinbuf_[ ang] = sin(a);
		dsinbuf_[1024 - ang] = cos(a);
	}
	sincosinited = 1;
}
 
// =====================================================================
// sincos, single-precision (initsincos_() must be called in advance)
// =====================================================================
void sincosf(float theta, float *s, float *c)
{ const float _1p5m       = 1572864.0;
  const double _2piinv     = 1.0 / _2pi;
  const float _pi512inv   = _2pi / 1024.0;
  // ____ double-precision fmadd
  double thb = theta * _2piinv + _1p5m;
  // ____ read 32 bit integer angle
  unsigned int thi = ((unsigned int*)(&thb))[iman_];
  // ____ first 8 bits of argument, in [0..¹/2]
  unsigned int th = (thi >> 20) & 0x03FC;
  void* sbuf = sinbuf_;
  // ____ read approximate sincos * 2
  float st = *(float*)(address(sbuf) + (  th));
  float ct = *(float*)(address(sbuf) + (1024 - th));
  unsigned int tf = (thi & 0x003FFFFF) | 0x3F800000;
  float sd = (*(float*)(&tf)) * _pi512inv - _pi512inv;
  float cd = 0.5f - sd * sd;
  float sv = sd * ct + cd * st;
  float cv = cd * ct - sd * st;

	if( (int)(thi) >= 0 ){
		if( thi & 0x40000000 ){
			*s = cv, *c = -sv;
		}
		else{
			*s = sv, *c = cv;
		}
	}
	else{
		if( thi & 0x40000000 ){
			*s = -cv, *c = sv;
		}
		else{
			*s = -sv, *c = -cv;
		}
	}
}

// =====================================================================
// sincos, double-precision (initsincos_() must be called in advance)
// =====================================================================
void sincos( double theta, double *s, double *c )
{ const float _2e52p5 = 1048576.0 * 1048576.0 * 4096.0 * 1.5;
  const double _2piinv = 1.0 / _2pi;
  const double thb = (_2e52p5 + _2piinv * theta) - _2e52p5;

	theta = (theta * _2piinv - thb);
    
	if( theta > 0 ){
		theta += 1;  // 1..2
	}
	else{
		theta += 2;  // 1..2
	}
	{ double buf[1];  
		*buf = theta;
		unsigned int thi = ((unsigned int*)(&buf))[iexp_];
		unsigned int th8 = thi >> 5 & 0x1FF8;
		((unsigned int*)(&buf))[iexp_] = thi & 0xFFF000FF;
		    
		theta = *buf * _2pi - _2pi;  // [0 .. 2¹ / 4096]
		{ // ____ read approximate sincos
		  double st = *(double*)(address(dsinbuf_) + ( th8));
		  double ct = *(double*)(address(dsinbuf_) + (8192 - th8));
		  // ____ calculate delta sincos explicitly
		  double theta2 = theta * theta;
		  double theta3 = theta2 * theta;
		  double theta4 = theta2 * theta2;
		  double sd = theta - theta3 * (double)(1.0 / 6.0);
		  double cd = theta2 * 0.5f - theta4 * (double)(1.0 / 24.0);
		  // ____ multiply to obtain final sincos
		  double sv = (st + sd * ct) - cd * st;
		  double cv = (ct - cd * ct) - sd * st;
			// ____ expand to [0..2¹]
			if( thi & 0x00080000 ){
				if( thi & 0x00040000 ){
					*s = -cv, *c = sv;
				}
				else{
					*s = -sv, *c = -cv;
				}
			}
			else{
				if( thi & 0x00040000 ){
					*s = cv, *c = -sv;
				}
				else{
					*s = sv, *c = cv;
				}
			}
		}
	}
}

#else

/* From the included doubledouble.cc */

// sin and cos.   Faster than separate calls of sin and cos

/* 20040820 RJVB: not so... on a G4 1.5Ghz, this is 2x slower than doing the separate calls.
 \ Of course, I am talking normal doubles here, not doubledoubles. Even on an old MIPS R5000,
 \ a sin and a cos is faster....
 */
void sincos(const double x, double *sinx, double *cosx)
{ 
  static const double tab[9]={ /* tab[b] := sin(b*Pi/16)... */
    0.0,
    0.1950903220161282678482848684770222409277,
    0.3826834323650897717284599840303988667613,
    0.5555702330196022247428308139485328743749,
    0.7071067811865475244008443621048490392850,
    0.8314696123025452370787883776179057567386,
    0.9238795325112867561281831893967882868225,
    0.9807852804032304491261822361342390369739,
    1.0
  };
  static const double sinsTab[7] = { /* Chebyshev coefficients */
    0.9999999999999999999999999999993767021096,
    -0.1666666666666666666666666602899977158461,
    8333333333333333333322459353395394180616.0e-42,
    -1984126984126984056685882073709830240680.0e-43,
    2755731922396443936999523827282063607870.0e-45,
    -2505210805220830174499424295197047025509.0e-47,
    1605649194713006247696761143618673476113.0e-49
  };
  int a,b;
  double sins, coss, k1, k3, t2, s, s2, sinb, cosb;

	if( fabs(x)<1.0e-11 ){
		*sinx=x, *cosx= 1.0-0.5* x* x;
		return;
	}
	k1= x/M_2PI;
	k3=k1-rint(k1);
	t2=4*k3;
	a= (int)(rint(t2));
	b= (int)(rint((8*(t2-a))));
	s= M_PI*(k3+k3-(8*a+b)/16.0);
	s2= s * s;
	sins=s*(sinsTab[0]+(sinsTab[1]+(sinsTab[2]+(sinsTab[3]+(sinsTab[4]+
	    (sinsTab[5]+sinsTab[6]*s2)*s2)*s2)*s2)*s2)*s2);
	coss=sqrt(1.0-(sins * sins)); /* ok, sins is small */
/* 	sinb= ( b>= 0 )? tab[b] : -tab[-b];	*/
/* 	cosb= tab[8-ABS(b)];	*/
	if( b>= 0 ){
		sinb= tab[b];
		cosb= tab[8-b];
	}
	else{
		sinb= -tab[-b];
		cosb= tab[8+b];
	}
	// sin(x)=
	// sin(s)(cos(1/2 a Pi) cos(1/16 b Pi) - sin(1/2 a Pi) sin(1/16 b Pi))
	// cos(s)(sin(1/2 a Pi) cos(1/16 b Pi) + cos(1/2 a Pi) sin(1/16 b Pi))
	// cos(x)=
	// cos(s)(cos(1/2 a Pi) cos(1/16 b Pi) - sin(1/2 a Pi) sin(1/16 b Pi))
	//-sin(s)(sin(1/2 a Pi) cos(1/16 b Pi) + cos(1/2 a Pi) sin(1/16 b Pi))
	if( 0==a ){
		*sinx= sins*cosb+coss*sinb;
		*cosx= coss*cosb-sins*sinb;
	}
	else if( 1==a ){
		*sinx=-sins*sinb+coss*cosb;
		*cosx=-coss*sinb-sins*cosb;
	}
	else if( -1==a ){
		*sinx= sins*sinb-coss*cosb;
		*cosx= coss*sinb+sins*cosb;
	}
	else{ /* |a|=2 */
		*sinx=-sins*cosb-coss*sinb;
		*cosx=-coss*cosb+sins*sinb;
	}
	return;
}

#endif

#endif

#ifdef NEEDS_ATAN2R
static flag ataninited = false;
static float atanbuf_[257 * 2];
static double datanbuf_[513 * 2];
 
// ====================================================================
// arctan initialization
// =====================================================================
static void initatan_() {
if (ataninited)                   return;
unsigned int ind;
for (ind = 0; ind <= 256; ind++) {
     double v = ind / 256.0;
     double asinv = ::asin(v);
     atanbuf_[ind * 2    ] = ::cos(asinv);
     atanbuf_[ind * 2 + 1] = asinv;
     }
for (ind = 0; ind <= 512; ind++) {
     double v = ind / 512.0;
     double asinv = ::asin(v);
     datanbuf_[ind * 2    ] = ::cos(asinv);
     datanbuf_[ind * 2 + 1] = asinv;
     }
ataninited = 1;
}
 
// =====================================================================
// arctan, single-precision; returns theta and r
// =====================================================================
float atan2r_(float y_, float x_, float& r_) {
Assert(ataninited);
float mag2 = x_ * x_ + y_ * y_;
if(!(mag2 > 0))  { goto zero; }   // degenerate case
float rinv = sqrtinv_(mag2);
unsigned int flags = 0;
float x, y;
float ypbuf[1];
float yp = 32768;
if (y_ < 0 ) { flags |= 4; y_ = -y_; }
if (x_ < 0 ) { flags |= 2; x_ = -x_; }
if (y_ > x_) {
flags |= 1;
yp += x_ * rinv; x = rinv * y_; y = rinv * x_;
ypbuf[0] = yp;
}
else {
yp += y_ * rinv; x = rinv * x_; y = rinv * y_;
ypbuf[0] = yp;
}
r_ = rinv * mag2;
int ind = (((int32*)(ypbuf))[0] & 0x01FF) * 2 * sizeof(float);
    
float* asbuf = (float*)(address(atanbuf_) + ind);
float sv = yp - 32768;
float cv = asbuf[0];
float asv = asbuf[1];
sv = y * cv - x * sv;    // delta sin value
// ____ compute arcsin directly
float asvd = 6 + sv * sv;   sv *= float(1.0 / 6.0);
float th = asv + asvd * sv;
if (flags & 1) { th = _2pif / 4 - th; }
if (flags & 2) { th = _2pif / 2 - th; }
if (flags & 4) { return -th; }
return th;
zero:
r_ = 0; return 0;
}
 
// =====================================================================
// arctan, double-precision; returns theta and r
// =====================================================================
double atan2r_(double y_, double x_, double& r_) {
Assert(ataninited);
const float _0 = 0.0;
double mag2 = x_ * x_ + y_ * y_;
if(!(mag2 > _0)) { goto zero; }   // degenerate case
double rinv = sqrtinv_(mag2);
unsigned int flags = 0;
double x, y;
double ypbuf[1];
double _2p43 = 65536.0 * 65536.0 * 2048.0;
double yp = _2p43;
if (y_ < _0) { flags |= 4; y_ = -y_; }
if (x_ < _0) { flags |= 2; x_ = -x_; }
if (y_ > x_) {
flags |= 1;
yp += x_ * rinv; x = rinv * y_; y = rinv * x_;
ypbuf[0] = yp;
}
else {
yp += y_ * rinv; x = rinv * x_; y = rinv * y_;
ypbuf[0] = yp;
}
r_ = rinv * mag2;
int ind = (((int32*)(ypbuf))[iman_] & 0x03FF) * 16;
double* dasbuf = (double*)(address(datanbuf_) + ind);
double sv = yp - _2p43; // index fraction
double cv = dasbuf[0];
double asv = dasbuf[1];
sv = y * cv - x * sv;    // delta sin value
// ____ compute arcsin directly
double asvd = 6 + sv * sv;   sv *= double(1.0 / 6.0);
double th = asv + asvd * sv;
if (flags & 1) { th = _2pi / 4 - th; }
if (flags & 2) { th = _2pi / 2 - th; }
if (flags & 4) { th = -th; }
return th;
zero:
r_ = _0; return _0;
}
#endif
