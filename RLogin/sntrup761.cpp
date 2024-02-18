/*  $OpenBSD: sntrup761.c,v 1.5 2021/01/08 02:33:13 dtucker Exp $ */

/*
 * Public Domain, Authors:
 * - Daniel J. Bernstein
 * - Chitchanok Chuengsatiansup
 * - Tanja Lange
 * - Christine van Vredendaal
 */

#include "stdafx.h"
#include "RLogin.h"

#include <stdint.h>
#include <stdlib.h>

#include <string.h>

#include <openssl/sha.h>

#define	crypto_hash_sha512(out, in, inlen)	SHA512(in, inlen, out)

extern void rand_buf(void *buf, int len);
extern int crypto_verify_32(const unsigned char *x,const unsigned char *y);

#define randombytes(buf, buf_len) rand_buf((buf), (buf_len))

#define int8		int8_t
#define uint8		uint8_t
#define int16		int16_t
#define uint16		uint16_t
#define int32		int32_t
#define uint32		uint32_t
#define int64		int64_t
#define uint64		uint64_t

/* from supercop-20201130/crypto_sort/int32/portable4/int32_minmax.inc */
#define int32_MINMAX(a,b) \
do { \
  int64_t ab = (int64_t)b ^ (int64_t)a; \
  int64_t c = (int64_t)b - (int64_t)a; \
  c ^= ab & (c ^ b); \
  c >>= 31; \
  c &= ab; \
  a ^= c; \
  b ^= c; \
} while(0)

/* from supercop-20201130/crypto_sort/int32/portable4/sort.c */


static void crypto_sort_int32(void *ap,int64_t n)
{
  int64_t top,p,q,r,i,j;
  int32 *x = (int32 *)ap;

  if (n < 2) return;
  top = 1;
  while (top < n - top) top += top;

  for (p = top;p >= 1;p >>= 1) {
    i = 0;
    while (i + 2 * p <= n) {
      for (j = i;j < i + p;++j)
        int32_MINMAX(x[j],x[j+p]);
      i += 2 * p;
    }
    for (j = i;j < n - p;++j)
      int32_MINMAX(x[j],x[j+p]);

    i = 0;
    j = 0;
    for (q = top;q > p;q >>= 1) {
      if (j != i) for (;;) {
        if (j == n - q) goto done;
        int32 a = x[j + p];
        for (r = q;r > p;r >>= 1)
          int32_MINMAX(a,x[j + r]);
        x[j + p] = a;
        ++j;
        if (j == i + p) {
          i += 2 * p;
          break;
        }
      }
      while (i + p <= n - q) {
        for (j = i;j < i + p;++j) {
          int32 a = x[j + p];
          for (r = q;r > p;r >>= 1)
            int32_MINMAX(a,x[j+r]);
          x[j + p] = a;
        }
        i += 2 * p;
      }
      /* now i + p > n - q */
      j = i;
      while (j < n - q) {
        int32 a = x[j + p];
        for (r = q;r > p;r >>= 1)
          int32_MINMAX(a,x[j+r]);
        x[j + p] = a;
        ++j;
      }

      done: ;
    }
  }
}

/* from supercop-20201130/crypto_sort/uint32/useint32/sort.c */

/* can save time by vectorizing xor loops */
/* can save time by integrating xor loops with int32_sort */

static void crypto_sort_uint32(void *ap,int64_t n)
{
  uint32 *x = (uint32 *)ap;
  int64_t j;
  for (j = 0;j < n;++j) x[j] ^= 0x80000000;
  crypto_sort_int32(ap,n);
  for (j = 0;j < n;++j) x[j] ^= 0x80000000;
}

/* from supercop-20201130/crypto_kem/sntrup761/ref/uint32.c */

/*
CPU division instruction typically takes time depending on x.
This software is designed to take time independent of x.
Time still varies depending on m; user must ensure that m is constant.
Time also varies on CPUs where multiplication is variable-time.
There could be more CPU issues.
There could also be compiler issues. 
*/

static void uint32_divmod_uint14(uint32 *q,uint16 *r,uint32 x,uint16 m)
{
  uint32 v = 0x80000000;
  uint32 qpart;
  uint32 mask;

  v /= m;

  /* caller guarantees m > 0 */
  /* caller guarantees m < 16384 */
  /* vm <= 2^31 <= vm+m-1 */
  /* xvm <= 2^31 x <= xvm+x(m-1) */

  *q = 0;

  qpart = (x*(uint64)v)>>31;
  /* 2^31 qpart <= xv <= 2^31 qpart + 2^31-1 */
  /* 2^31 qpart m <= xvm <= 2^31 qpart m + (2^31-1)m */
  /* 2^31 qpart m <= 2^31 x <= 2^31 qpart m + (2^31-1)m + x(m-1) */
  /* 0 <= 2^31 newx <= (2^31-1)m + x(m-1) */
  /* 0 <= newx <= (1-1/2^31)m + x(m-1)/2^31 */
  /* 0 <= newx <= (1-1/2^31)(2^14-1) + (2^32-1)((2^14-1)-1)/2^31 */

  x -= qpart*m; *q += qpart;
  /* x <= 49146 */

  qpart = (x*(uint64)v)>>31;
  /* 0 <= newx <= (1-1/2^31)m + x(m-1)/2^31 */
  /* 0 <= newx <= m + 49146(2^14-1)/2^31 */
  /* 0 <= newx <= m + 0.4 */
  /* 0 <= newx <= m */

  x -= qpart*m; *q += qpart;
  /* x <= m */

  x -= m; *q += 1;
  mask = -(int32)(x>>31);
  x += mask&(uint32)m; *q += mask;
  /* x < m */

  *r = x;
}


static uint16 uint32_mod_uint14(uint32 x,uint16 m)
{
  uint32 q;
  uint16 r;
  uint32_divmod_uint14(&q,&r,x,m);
  return r;
}

/* from supercop-20201130/crypto_kem/sntrup761/ref/int32.c */

static void int32_divmod_uint14(int32 *q,uint16 *r,int32 x,uint16 m)
{
  uint32 uq,uq2;
  uint16 ur,ur2;
  uint32 mask;

  uint32_divmod_uint14(&uq,&ur,0x80000000+(uint32)x,m);
  uint32_divmod_uint14(&uq2,&ur2,0x80000000,m);
  ur -= ur2; uq -= uq2;
  mask = -(int32)(ur>>15);
  ur += mask&m; uq += mask;
  *r = ur; *q = uq;
}


static uint16 int32_mod_uint14(int32 x,uint16 m)
{
  int32 q;
  uint16 r;
  int32_divmod_uint14(&q,&r,x,m);
  return r;
}

/* from supercop-20201130/crypto_kem/sntrup761/ref/paramsmenu.h */
/* pick one of these three: */
#define SIZE761
#undef SIZE653
#undef SIZE857

/* pick one of these two: */
#define SNTRUP /* Streamlined NTRU Prime */
#undef LPR /* NTRU LPRime */

/* from supercop-20201130/crypto_kem/sntrup761/ref/params.h */
/* menu of parameter choices: */
/* what the menu means: */

#if defined(SIZE761)
#define sntrup_p 761
#define sntrup_q 4591
#define Rounded_bytes 1007
#define Rq_bytes 1158
#define sntrup_w 286

#elif defined(SIZE653)
#define sntrup_p 653
#define sntrup_q 4621
#define Rounded_bytes 865
#define Rq_bytes 994
#define sntrup_w 288

#elif defined(SIZE857)
#define sntrup_p 857
#define sntrup_q 5167
#define Rounded_bytes 1152
#define Rq_bytes 1322
#define sntrup_w 322

#else
#error "no parameter set defined"
#endif

/* from supercop-20201130/crypto_kem/sntrup761/ref/Decode.h */
#ifndef Decode_H
#define Decode_H


/* Decode(R,s,M,len) */
/* assumes 0 < M[i] < 16384 */
/* produces 0 <= R[i] < M[i] */

#endif

/* from supercop-20201130/crypto_kem/sntrup761/ref/Decode.c */

static void Decode(uint16 *out,const unsigned char *S,const uint16 *M,int64 len)
{
  if (len == 1) {
    if (M[0] == 1)
      *out = 0;
    else if (M[0] <= 256)
      *out = uint32_mod_uint14(S[0],M[0]);
    else
      *out = uint32_mod_uint14(S[0]+(((uint16)S[1])<<8),M[0]);
  }
  if (len > 1) {
    uint16 *R2 = new uint16[((int)len+1)/2];
    uint16 *M2 = new uint16[(int)(len+1)/2];
    uint16 *bottomr = new uint16[(int)len/2];
    uint32 *bottomt = new uint32[(int)len/2];
    int64_t i;
    for (i = 0;i < len-1;i += 2) {
      uint32 m = M[i]*(uint32) M[i+1];
      if (m > 256*16383) {
        bottomt[i/2] = 256*256;
        bottomr[i/2] = S[0]+256*S[1];
        S += 2;
        M2[i/2] = (((m+255)>>8)+255)>>8;
      } else if (m >= 16384) {
        bottomt[i/2] = 256;
        bottomr[i/2] = S[0];
        S += 1;
        M2[i/2] = (m+255)>>8;
      } else {
        bottomt[i/2] = 1;
        bottomr[i/2] = 0;
        M2[i/2] = m;
      }
    }
    if (i < len)
      M2[i/2] = M[i];
    Decode(R2,S,M2,(len+1)/2);
    for (i = 0;i < len-1;i += 2) {
      uint32 r = bottomr[i/2];
      uint32 r1;
      uint16 r0;
      r += bottomt[i/2]*R2[i/2];
      uint32_divmod_uint14(&r1,&r0,r,M[i]);
      r1 = uint32_mod_uint14(r1,M[i+1]); /* only needed for invalid inputs */
      *out++ = r0;
      *out++ = r1;
    }
    if (i < len)
      *out++ = R2[i/2];

	delete [] R2;
	delete [] M2;
	delete [] bottomr;
	delete [] bottomt;
  }
}

/* from supercop-20201130/crypto_kem/sntrup761/ref/Encode.h */
#ifndef Encode_H
#define Encode_H


/* Encode(s,R,M,len) */
/* assumes 0 <= R[i] < M[i] < 16384 */

#endif

/* from supercop-20201130/crypto_kem/sntrup761/ref/Encode.c */

/* 0 <= R[i] < M[i] < 16384 */
static void Encode(unsigned char *out,const uint16 *R,const uint16 *M,int64 len)
{
  if (len == 1) {
    uint16 r = R[0];
    uint16 m = M[0];
    while (m > 1) {
      *out++ = (unsigned char)r;
      r >>= 8;
      m = (m+255)>>8;
    }
  }
  if (len > 1) {
	uint16 *R2 = new uint16[((int)len+1)/2];
    uint16 *M2 = new uint16[((int)len+1)/2];
    int64_t i;
    for (i = 0;i < len-1;i += 2) {
      uint32 m0 = M[i];
      uint32 r = R[i]+R[i+1]*m0;
      uint32 m = M[i+1]*m0;
      while (m >= 16384) {
        *out++ = r;
        r >>= 8;
        m = (m+255)>>8;
      }
      R2[i/2] = r;
      M2[i/2] = m;
    }
    if (i < len) {
      R2[i/2] = R[i];
      M2[i/2] = M[i];
    }
    Encode(out,R2,M2,(len+1)/2);
	delete [] R2;
	delete [] M2;
  }
}

/* from supercop-20201130/crypto_kem/sntrup761/ref/kem.c */

/* ----- masks */

#ifndef LPR

/* return -1 if x!=0; else return 0 */
static int int16_nonzero_mask(int16 x)
{
  uint16 u = x; /* 0, else 1...65535 */
  uint32 v = u; /* 0, else 1...65535 */
  v = -(int32)v; /* 0, else 2^32-65535...2^32-1 */
  v >>= 31; /* 0, else 1 */
  return -(int)v; /* 0, else -1 */
}

#endif

/* return -1 if x<0; otherwise return 0 */
static int int16_negative_mask(int16 x)
{
  uint16 u = x;
  u >>= 15;
  return -(int) u;
  /* alternative with gcc -fwrapv: */
  /* x>>15 compiles to CPU's arithmetic right shift */
}

/* ----- arithmetic mod 3 */

#ifdef small
#undef small
#endif

typedef int8 small;

/* F3 is always represented as -1,0,1 */
/* so ZZ_fromF3 is a no-op */

/* x must not be close to top int16 */
static small F3_freeze(int16 x)
{
  return int32_mod_uint14(x+1,3)-1;
}

/* ----- arithmetic mod q */

#define q12 ((sntrup_q-1)/2)
typedef int16 Fq;
/* always represented as -q12...q12 */
/* so ZZ_fromFq is a no-op */

/* x must not be close to top int32 */
static Fq Fq_freeze(int32 x)
{
  return int32_mod_uint14(x+q12,sntrup_q)-q12;
}

#ifndef LPR

static Fq Fq_recip(Fq a1)
{ 
  int i = 1;
  Fq ai = a1;

  while (i < sntrup_q-2) {
    ai = Fq_freeze(a1*(int32)ai);
    i += 1;
  }
  return ai;
} 

#endif

/* ----- Top and Right */

/* ----- small polynomials */

#ifndef LPR

/* 0 if Weightw_is(r), else -1 */
static int Weightw_mask(small *r)
{
  int weight = 0;
  int i;

  for (i = 0;i < sntrup_p;++i) weight += r[i]&1;
  return int16_nonzero_mask(weight-sntrup_w);
}

/* R3_fromR(R_fromRq(r)) */
static void R3_fromRq(small *out,const Fq *r)
{
  int i;
  for (i = 0;i < sntrup_p;++i) out[i] = F3_freeze(r[i]);
}

/* h = f*g in the ring R3 */
static void R3_mult(small *h,const small *f,const small *g)
{
  small fg[sntrup_p+sntrup_p-1];
  small result;
  int i,j;

  for (i = 0;i < sntrup_p;++i) {
    result = 0;
    for (j = 0;j <= i;++j) result = F3_freeze(result+f[j]*g[i-j]);
    fg[i] = result;
  }
  for (i = sntrup_p;i < sntrup_p+sntrup_p-1;++i) {
    result = 0;
    for (j = i-sntrup_p+1;j < sntrup_p;++j) result = F3_freeze(result+f[j]*g[i-j]);
    fg[i] = result;
  }

  for (i = sntrup_p+sntrup_p-2;i >= sntrup_p;--i) {
    fg[i-sntrup_p] = F3_freeze(fg[i-sntrup_p]+fg[i]);
    fg[i-sntrup_p+1] = F3_freeze(fg[i-sntrup_p+1]+fg[i]);
  }

  for (i = 0;i < sntrup_p;++i) h[i] = fg[i];
}

/* returns 0 if recip succeeded; else -1 */
static int R3_recip(small *out,const small *in)
{ 
  small f[sntrup_p+1],g[sntrup_p+1],v[sntrup_p+1],r[sntrup_p+1];
  int i,loop,delta;
  int sign,swap,t;
  
  for (i = 0;i < sntrup_p+1;++i) v[i] = 0;
  for (i = 0;i < sntrup_p+1;++i) r[i] = 0;
  r[0] = 1;
  for (i = 0;i < sntrup_p;++i) f[i] = 0;
  f[0] = 1; f[sntrup_p-1] = f[sntrup_p] = -1;
  for (i = 0;i < sntrup_p;++i) g[sntrup_p-1-i] = in[i];
  g[sntrup_p] = 0;
    
  delta = 1; 

  for (loop = 0;loop < 2*sntrup_p-1;++loop) {
    for (i = sntrup_p;i > 0;--i) v[i] = v[i-1];
    v[0] = 0;
    
    sign = -g[0]*f[0];
    swap = int16_negative_mask(-delta) & int16_nonzero_mask(g[0]);
    delta ^= swap&(delta^-delta);
    delta += 1;
    
    for (i = 0;i < sntrup_p+1;++i) {
      t = swap&(f[i]^g[i]); f[i] ^= t; g[i] ^= t;
      t = swap&(v[i]^r[i]); v[i] ^= t; r[i] ^= t;
    }
  
    for (i = 0;i < sntrup_p+1;++i) g[i] = F3_freeze(g[i]+sign*f[i]);
    for (i = 0;i < sntrup_p+1;++i) r[i] = F3_freeze(r[i]+sign*v[i]);

    for (i = 0;i < sntrup_p;++i) g[i] = g[i+1];
    g[sntrup_p] = 0;
  }
  
  sign = f[0];
  for (i = 0;i < sntrup_p;++i) out[i] = sign*v[sntrup_p-1-i];
  
  return int16_nonzero_mask(delta);
} 

#endif

/* ----- polynomials mod q */

/* h = f*g in the ring Rq */
static void Rq_mult_small(Fq *h,const Fq *f,const small *g)
{
  Fq fg[sntrup_p+sntrup_p-1];
  Fq result;
  int i,j;

  for (i = 0;i < sntrup_p;++i) {
    result = 0;
    for (j = 0;j <= i;++j) result = Fq_freeze(result+f[j]*(int32)g[i-j]);
    fg[i] = result;
  }
  for (i = sntrup_p;i < sntrup_p+sntrup_p-1;++i) {
    result = 0;
    for (j = i-sntrup_p+1;j < sntrup_p;++j) result = Fq_freeze(result+f[j]*(int32)g[i-j]);
    fg[i] = result;
  }

  for (i = sntrup_p+sntrup_p-2;i >= sntrup_p;--i) {
    fg[i-sntrup_p] = Fq_freeze(fg[i-sntrup_p]+fg[i]);
    fg[i-sntrup_p+1] = Fq_freeze(fg[i-sntrup_p+1]+fg[i]);
  }

  for (i = 0;i < sntrup_p;++i) h[i] = fg[i];
}

#ifndef LPR

/* h = 3f in Rq */
static void Rq_mult3(Fq *h,const Fq *f)
{
  int i;
  
  for (i = 0;i < sntrup_p;++i) h[i] = Fq_freeze(3*f[i]);
}

/* out = 1/(3*in) in Rq */
/* returns 0 if recip succeeded; else -1 */
static int Rq_recip3(Fq *out,const small *in)
{ 
  Fq f[sntrup_p+1],g[sntrup_p+1],v[sntrup_p+1],r[sntrup_p+1];
  int i,loop,delta;
  int swap,t;
  int32 f0,g0;
  Fq scale;

  for (i = 0;i < sntrup_p+1;++i) v[i] = 0;
  for (i = 0;i < sntrup_p+1;++i) r[i] = 0;
  r[0] = Fq_recip(3);
  for (i = 0;i < sntrup_p;++i) f[i] = 0;
  f[0] = 1; f[sntrup_p-1] = f[sntrup_p] = -1;
  for (i = 0;i < sntrup_p;++i) g[sntrup_p-1-i] = in[i];
  g[sntrup_p] = 0;

  delta = 1;

  for (loop = 0;loop < 2*sntrup_p-1;++loop) {
    for (i = sntrup_p;i > 0;--i) v[i] = v[i-1];
    v[0] = 0;

    swap = int16_negative_mask(-delta) & int16_nonzero_mask(g[0]);
    delta ^= swap&(delta^-delta);
    delta += 1;

    for (i = 0;i < sntrup_p+1;++i) {
      t = swap&(f[i]^g[i]); f[i] ^= t; g[i] ^= t;
      t = swap&(v[i]^r[i]); v[i] ^= t; r[i] ^= t;
    }

    f0 = f[0];
    g0 = g[0];
    for (i = 0;i < sntrup_p+1;++i) g[i] = Fq_freeze(f0*g[i]-g0*f[i]);
    for (i = 0;i < sntrup_p+1;++i) r[i] = Fq_freeze(f0*r[i]-g0*v[i]);

    for (i = 0;i < sntrup_p;++i) g[i] = g[i+1];
    g[sntrup_p] = 0;
  }

  scale = Fq_recip(f[0]);
  for (i = 0;i < sntrup_p;++i) out[i] = Fq_freeze(scale*(int32)v[sntrup_p-1-i]);

  return int16_nonzero_mask(delta);
}

#endif

/* ----- rounded polynomials mod q */

static void Round(Fq *out,const Fq *a)
{
  int i;
  for (i = 0;i < sntrup_p;++i) out[i] = a[i]-F3_freeze(a[i]);
}

/* ----- sorting to generate short polynomial */

static void Short_fromlist(small *out,const uint32 *in)
{
  uint32 L[sntrup_p];
  int i;

  for (i = 0;i < sntrup_w;++i) L[i] = in[i]&(uint32)-2;
  for (i = sntrup_w;i < sntrup_p;++i) L[i] = (in[i]&(uint32)-3)|1;
  crypto_sort_uint32(L,sntrup_p);
  for (i = 0;i < sntrup_p;++i) out[i] = (L[i]&3)-1;
}

/* ----- underlying hash function */

#define Hash_bytes 32

/* e.g., b = 0 means out = Hash0(in) */
static void Hash_prefix(unsigned char *out,int b,const unsigned char *in,int inlen)
{
  unsigned char *x = new unsigned char[inlen+1];
  unsigned char h[64];
  int i;

  x[0] = b;
  for (i = 0;i < inlen;++i) x[i+1] = in[i];
  crypto_hash_sha512(h,x,inlen+1);
  for (i = 0;i < 32;++i) out[i] = h[i];

  delete [] x;
}

/* ----- higher-level randomness */

static uint32 urandom32(void)
{
  unsigned char c[4];
  uint32 out[4];

  randombytes(c,4);
  out[0] = (uint32)c[0];
  out[1] = ((uint32)c[1])<<8;
  out[2] = ((uint32)c[2])<<16;
  out[3] = ((uint32)c[3])<<24;
  return out[0]+out[1]+out[2]+out[3];
}

static void Short_random(small *out)
{
  uint32 L[sntrup_p];
  int i;

  for (i = 0;i < sntrup_p;++i) L[i] = urandom32();
  Short_fromlist(out,L);
}

#ifndef LPR

static void Small_random(small *out)
{
  int i;

  for (i = 0;i < sntrup_p;++i) out[i] = (((urandom32()&0x3fffffff)*3)>>30)-1;
}

#endif

/* ----- Streamlined NTRU Prime Core */

#ifndef LPR

/* h,(f,ginv) = KeyGen() */
static void KeyGen(Fq *h,small *f,small *ginv)
{
  small g[sntrup_p];
  Fq finv[sntrup_p];
  
  for (;;) {
    Small_random(g);
    if (R3_recip(ginv,g) == 0) break;
  }
  Short_random(f);
  Rq_recip3(finv,f); /* always works */
  Rq_mult_small(h,finv,g);
}

/* c = Encrypt(r,h) */
static void Encrypt(Fq *c,const small *r,const Fq *h)
{
  Fq hr[sntrup_p];

  Rq_mult_small(hr,h,r);
  Round(c,hr);
}

/* r = Decrypt(c,(f,ginv)) */
static void Decrypt(small *r,const Fq *c,const small *f,const small *ginv)
{
  Fq cf[sntrup_p];
  Fq cf3[sntrup_p];
  small e[sntrup_p];
  small ev[sntrup_p];
  int mask;
  int i;

  Rq_mult_small(cf,c,f);
  Rq_mult3(cf3,cf);
  R3_fromRq(e,cf3);
  R3_mult(ev,e,ginv);

  mask = Weightw_mask(ev); /* 0 if weight w, else -1 */
  for (i = 0;i < sntrup_w;++i) r[i] = ((ev[i]^1)&~mask)^1;
  for (i = sntrup_w;i < sntrup_p;++i) r[i] = ev[i]&~mask;
}
  
#endif

/* ----- NTRU LPRime Core */

/* ----- encoding I-bit inputs */

/* ----- Expand */

/* ----- Seeds */

/* ----- Generator, HashShort */

/* ----- NTRU LPRime Expand */

/* ----- encoding small polynomials (including short polynomials) */

#define Small_bytes ((sntrup_p+3)/4)

/* these are the only functions that rely on p mod 4 = 1 */

static void Small_encode(unsigned char *s,const small *f)
{
  small x;
  int i;

  for (i = 0;i < sntrup_p/4;++i) {
    x = *f++ + 1;
    x += (*f++ + 1)<<2;
    x += (*f++ + 1)<<4;
    x += (*f++ + 1)<<6;
    *s++ = x;
  }
  x = *f++ + 1;
  *s++ = x;
}

static void Small_decode(small *f,const unsigned char *s)
{
  unsigned char x;
  int i;

  for (i = 0;i < sntrup_p/4;++i) {
    x = *s++;
    *f++ = ((small)(x&3))-1; x >>= 2;
    *f++ = ((small)(x&3))-1; x >>= 2;
    *f++ = ((small)(x&3))-1; x >>= 2;
    *f++ = ((small)(x&3))-1;
  }
  x = *s++;
  *f++ = ((small)(x&3))-1;
}

/* ----- encoding general polynomials */

#ifndef LPR

static void Rq_encode(unsigned char *s,const Fq *r)
{
  uint16 R[sntrup_p],M[sntrup_p];
  int i;
  
  for (i = 0;i < sntrup_p;++i) R[i] = r[i]+q12;
  for (i = 0;i < sntrup_p;++i) M[i] = sntrup_q;
  Encode(s,R,M,sntrup_p);
}

static void Rq_decode(Fq *r,const unsigned char *s)
{
  uint16 R[sntrup_p],M[sntrup_p];
  int i;

  for (i = 0;i < sntrup_p;++i) M[i] = sntrup_q;
  Decode(R,s,M,sntrup_p);
  for (i = 0;i < sntrup_p;++i) r[i] = ((Fq)R[i])-q12;
}
  
#endif

/* ----- encoding rounded polynomials */

static void Rounded_encode(unsigned char *s,const Fq *r)
{
  uint16 R[sntrup_p],M[sntrup_p];
  int i;

  for (i = 0;i < sntrup_p;++i) R[i] = ((r[i]+q12)*10923)>>15;
  for (i = 0;i < sntrup_p;++i) M[i] = (sntrup_q+2)/3;
  Encode(s,R,M,sntrup_p);
}

static void Rounded_decode(Fq *r,const unsigned char *s)
{
  uint16 R[sntrup_p],M[sntrup_p];
  int i;

  for (i = 0;i < sntrup_p;++i) M[i] = (sntrup_q+2)/3;
  Decode(R,s,M,sntrup_p);
  for (i = 0;i < sntrup_p;++i) r[i] = R[i]*3-q12;
}

/* ----- encoding top polynomials */

/* ----- Streamlined NTRU Prime Core plus encoding */

#ifndef LPR

typedef small Inputs[sntrup_p]; /* passed by reference */
#define Inputs_random Short_random
#define Inputs_encode Small_encode
#define Inputs_bytes Small_bytes

#define Ciphertexts_bytes Rounded_bytes
#define SecretKeys_bytes (2*Small_bytes)
#define PublicKeys_bytes Rq_bytes

/* pk,sk = ZKeyGen() */
static void ZKeyGen(unsigned char *pk,unsigned char *sk)
{
  Fq h[sntrup_p];
  small f[sntrup_p],v[sntrup_p];

  KeyGen(h,f,v);
  Rq_encode(pk,h);
  Small_encode(sk,f); sk += Small_bytes;
  Small_encode(sk,v);
}

/* C = ZEncrypt(r,pk) */
static void ZEncrypt(unsigned char *C,const Inputs r,const unsigned char *pk)
{
  Fq h[sntrup_p];
  Fq c[sntrup_p];
  Rq_decode(h,pk);
  Encrypt(c,r,h);
  Rounded_encode(C,c);
}

/* r = ZDecrypt(C,sk) */
static void ZDecrypt(Inputs r,const unsigned char *C,const unsigned char *sk)
{
  small f[sntrup_p],v[sntrup_p];
  Fq c[sntrup_p];

  Small_decode(f,sk); sk += Small_bytes;
  Small_decode(v,sk);
  Rounded_decode(c,C);
  Decrypt(r,c,f,v);
}

#endif

/* ----- NTRU LPRime Expand plus encoding */

/* ----- confirmation hash */

#define Confirm_bytes 32

/* h = HashConfirm(r,pk,cache); cache is Hash4(pk) */
static void HashConfirm(unsigned char *h,const unsigned char *r,const unsigned char *pk,const unsigned char *cache)
{
  unsigned char x[Hash_bytes*2];
  int i;

  Hash_prefix(x,3,r,Inputs_bytes);
  for (i = 0;i < Hash_bytes;++i) x[Hash_bytes+i] = cache[i];
  Hash_prefix(h,2,x,sizeof x);
}

/* ----- session-key hash */

/* k = HashSession(b,y,z) */
static void HashSession(unsigned char *k,int b,const unsigned char *y,const unsigned char *z)
{
  unsigned char x[Hash_bytes+Ciphertexts_bytes+Confirm_bytes];
  int i;

  Hash_prefix(x,3,y,Inputs_bytes);
  for (i = 0;i < Ciphertexts_bytes+Confirm_bytes;++i) x[Hash_bytes+i] = z[i];
  Hash_prefix(k,b,x,sizeof x);
}

/* ----- Streamlined NTRU Prime and NTRU LPRime */

/* pk,sk = KEM_KeyGen() */
static void KEM_KeyGen(unsigned char *pk,unsigned char *sk)
{
  int i;

  ZKeyGen(pk,sk); sk += SecretKeys_bytes;
  for (i = 0;i < PublicKeys_bytes;++i) *sk++ = pk[i];
  randombytes(sk,Inputs_bytes); sk += Inputs_bytes;
  Hash_prefix(sk,4,pk,PublicKeys_bytes);
}

/* c,r_enc = Hide(r,pk,cache); cache is Hash4(pk) */
static void Hide(unsigned char *c,unsigned char *r_enc,const Inputs r,const unsigned char *pk,const unsigned char *cache)
{
  Inputs_encode(r_enc,r);
  ZEncrypt(c,r,pk); c += Ciphertexts_bytes;
  HashConfirm(c,r_enc,pk,cache);
}

/* c,k = Encap(pk) */
static void Encap(unsigned char *c,unsigned char *k,const unsigned char *pk)
{
  Inputs r;
  unsigned char r_enc[Inputs_bytes];
  unsigned char cache[Hash_bytes];

  Hash_prefix(cache,4,pk,PublicKeys_bytes);
  Inputs_random(r);
  Hide(c,r_enc,r,pk,cache);
  HashSession(k,1,r_enc,c);
}

/* 0 if matching ciphertext+confirm, else -1 */
static int Ciphertexts_diff_mask(const unsigned char *c,const unsigned char *c2)
{
  uint16 differentbits = 0;
  int len = Ciphertexts_bytes+Confirm_bytes;

  while (len-- > 0) differentbits |= (*c++)^(*c2++);
  return (1&((differentbits-1)>>8))-1;
}

/* k = Decap(c,sk) */
static void Decap(unsigned char *k,const unsigned char *c,const unsigned char *sk)
{
  const unsigned char *pk = sk + SecretKeys_bytes;
  const unsigned char *rho = pk + PublicKeys_bytes;
  const unsigned char *cache = rho + Inputs_bytes;
  Inputs r;
  unsigned char r_enc[Inputs_bytes];
  unsigned char cnew[Ciphertexts_bytes+Confirm_bytes];
  int mask;
  int i;

  ZDecrypt(r,c,sk);
  Hide(cnew,r_enc,r,pk,cache);
  mask = Ciphertexts_diff_mask(c,cnew);
  for (i = 0;i < Inputs_bytes;++i) r_enc[i] ^= mask&(r_enc[i]^rho[i]);
  HashSession(k,1+mask,r_enc,c);
}

/* ----- crypto_kem API */

int sntrup761_keypair(unsigned char *pk,unsigned char *sk)
{
  KEM_KeyGen(pk,sk);
  return 0;
}

int sntrup761_enc(unsigned char *c,unsigned char *k,const unsigned char *pk)
{
  Encap(c,k,pk);
  return 0;
}

int sntrup761_dec(unsigned char *k,const unsigned char *c,const unsigned char *sk)
{
  Decap(k,c,sk);
  return 0;
}

