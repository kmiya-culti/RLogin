/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/README */

/*
This is a reference implementation of Streamlined NTRU Prime 4591^761.
This implementation is designed primarily for clarity, subject to the
following constraints:

   * The implementation is written in C. The Sage implementation in the
     NTRU Prime paper is considerably more concise (and compatible).

   * The implementation avoids data-dependent branches and array
     indices. For example, conditional swaps are computed by arithmetic
     rather than by branches.

   * The implementation avoids other C operations that often take
     variable time. For example, divisions by 3 are computed via
     multiplications and shifts.
     
This implementation does _not_ sacrifice clarity for speed.

This implementation has not yet been reviewed for correctness or for
constant-time behavior. It does pass various tests and has no known
bugs, but there are at least some platforms where multiplications take
variable time, and fixing this requires platform-specific effort; see
https://www.bearssl.org/ctmul.html and http://repository.tue.nl/800603.

This implementation allows "benign malleability" of ciphertexts, as
defined in http://www.shoup.net/papers/iso-2_1.pdf. Specifically, each
32-bit ciphertext word encodes three integers between 0 and 1530; if
larger integers appear then they are silently reduced modulo 1531.
Similar comments apply to public keys.

There is a separate "avx" implementation where similar comments apply,
except that "avx" _does_ sacrifice clarity for speed on CPUs with AVX2
instructions.
*/

/* See https://ntruprime.cr.yp.to/software.html for detailed documentation. */

#include "stdafx.h"
#include "RLogin.h"

#include <stdint.h>
#include <stdlib.h>

typedef	int8_t		crypto_int8;
typedef	uint8_t		crypto_uint8;
typedef	int16_t		crypto_int16;
typedef	uint16_t	crypto_uint16;
typedef	int32_t		crypto_int32;
typedef	uint32_t	crypto_uint32;

#define sntrup4591761_PUBLICKEYBYTES	1218
#define sntrup4591761_SECRETKEYBYTES	1600
#define sntrup4591761_CIPHERTEXTBYTES	1047
#define sntrup4591761_BYTES				32

int	sntrup4591761_keypair(unsigned char *pk, unsigned char *sk);
int	sntrup4591761_enc(unsigned char *cstr, unsigned char *k, const unsigned char *pk);
int	sntrup4591761_dec(unsigned char *k, const unsigned char *cstr, const unsigned char *sk);

extern void rand_buf(void *buf, int len);
extern int crypto_hash_sha512(unsigned char *out,const unsigned char *in,unsigned long long inlen);
extern int crypto_verify_32(const unsigned char *x,const unsigned char *y);

#define randombytes(buf, buf_len) rand_buf((buf), (buf_len))

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/int32_sort.c */

static void minmax(crypto_int32 *x,crypto_int32 *y)
{
  crypto_uint32 xi = *x;
  crypto_uint32 yi = *y;
  crypto_uint32 xy = xi ^ yi;
  crypto_uint32 c = yi - xi;
  c ^= xy & (c ^ yi);
  c >>= 31;
  c = 0 - c;
  c &= xy;
  *x = xi ^ c;
  *y = yi ^ c;
}

static void int32_sort(crypto_int32 *x,int n)
{
  int top,p,q,i;

  if (n < 2) return;
  top = 1;
  while (top < n - top) top += top;

  for (p = top;p > 0;p >>= 1) {
    for (i = 0;i < n - p;++i)
      if (!(i & p))
        minmax(x + i,x + i + p);
    for (q = top;q > p;q >>= 1)
      for (i = 0;i < n - q;++i)
        if (!(i & p))
          minmax(x + i + p,x + i + q);
  }
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/swap.c */

static void swap(void *x,void *y,int bytes,int mask)
{
  int i;
  char xi, yi, c, t;

  c = mask;
  
  for (i = 0;i < bytes;++i) {
    xi = i[(char *) x];
    yi = i[(char *) y];
    t = c & (xi ^ yi);
    xi ^= t;
    yi ^= t;
    i[(char *) x] = xi;
    i[(char *) y] = yi;
  }
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/params.h */

#define sntrup4591761_q			4591
#define sntrup4591761_qshift	2295
#define sntrup4591761_p			761
#define sntrup4591761_w			286

#define rq_encode_len			1218
#define small_encode_len		191

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/small.c */

#define	sntrup4591761_small		crypto_int8
#define	sntrup4591761_modq		crypto_int16

/* XXX: these functions rely on p mod 4 = 1 */

/* all coefficients in -1, 0, 1 */
static void small_encode(unsigned char *c,const sntrup4591761_small *f)
{
  sntrup4591761_small c0;
  int i;

  for (i = 0;i < sntrup4591761_p/4;++i) {
    c0 = *f++ + 1;
    c0 += (*f++ + 1) << 2;
    c0 += (*f++ + 1) << 4;
    c0 += (*f++ + 1) << 6;
    *c++ = c0;
  }
  c0 = *f++ + 1;
  *c++ = c0;
}

static void small_decode(sntrup4591761_small *f,const unsigned char *c)
{
  unsigned char c0;
  int i;

  for (i = 0;i < sntrup4591761_p/4;++i) {
    c0 = *c++;
    *f++ = ((sntrup4591761_small) (c0 & 3)) - 1; c0 >>= 2;
    *f++ = ((sntrup4591761_small) (c0 & 3)) - 1; c0 >>= 2;
    *f++ = ((sntrup4591761_small) (c0 & 3)) - 1; c0 >>= 2;
    *f++ = ((sntrup4591761_small) (c0 & 3)) - 1;
  }
  c0 = *c++;
  *f++ = ((sntrup4591761_small) (c0 & 3)) - 1;
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/random32.c */

static crypto_int32 small_random32(void)
{
  unsigned char x[4];
  randombytes(x,4);
  return x[0] + (x[1] << 8) + (x[2] << 16) + (x[3] << 24);
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/randomsmall.c */

static void small_random(sntrup4591761_small *g)
{
  int i;

  for (i = 0;i < sntrup4591761_p;++i) {
    crypto_uint32 r = small_random32();
    g[i] = (sntrup4591761_small) (((1073741823 & r) * 3) >> 30) - 1;
  }
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/randomweightw.c */

static void small_random_weightw(sntrup4591761_small *f)
{
  crypto_int32 r[sntrup4591761_p];
  int i;

  for (i = 0;i < sntrup4591761_p;++i) r[i] = small_random32();
  for (i = 0;i < sntrup4591761_w;++i) r[i] &= -2;
  for (i = sntrup4591761_w;i < sntrup4591761_p;++i) r[i] = (r[i] & -3) | 1;
  int32_sort(r,sntrup4591761_p);
  for (i = 0;i < sntrup4591761_p;++i) f[i] = ((sntrup4591761_small) (r[i] & 3)) - 1;
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/mod3.h */

/* -1 if x is nonzero, 0 otherwise */
static inline int mod3_nonzero_mask(sntrup4591761_small x)
{
  return -x*x;
}

/* input between -100000 and 100000 */
/* output between -1 and 1 */
static inline sntrup4591761_small mod3_freeze(crypto_int32 a)
{
  a -= 3 * ((10923 * a) >> 15);
  a -= 3 * ((89478485 * a + 134217728) >> 28);
  return a;
}

static inline sntrup4591761_small mod3_minusproduct(sntrup4591761_small a,sntrup4591761_small b,sntrup4591761_small c)
{
  crypto_int32 A = a;
  crypto_int32 B = b;
  crypto_int32 C = c;
  return mod3_freeze(A - B * C);
}

static inline sntrup4591761_small mod3_plusproduct(sntrup4591761_small a,sntrup4591761_small b,sntrup4591761_small c)
{
  crypto_int32 A = a;
  crypto_int32 B = b;
  crypto_int32 C = c;
  return mod3_freeze(A + B * C);
}

static inline sntrup4591761_small mod3_product(sntrup4591761_small a,sntrup4591761_small b)
{
  return a * b;
}

static inline sntrup4591761_small mod3_sum(sntrup4591761_small a,sntrup4591761_small b)
{
  crypto_int32 A = a;
  crypto_int32 B = b;
  return mod3_freeze(A + B);
}

static inline sntrup4591761_small mod3_reciprocal(sntrup4591761_small a1)
{
  return a1;
}

static inline sntrup4591761_small mod3_quotient(sntrup4591761_small num,sntrup4591761_small den)
{
  return mod3_product(num,mod3_reciprocal(den));
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/r3_mult.c */

static void r3_mult(sntrup4591761_small *h,const sntrup4591761_small *f,const sntrup4591761_small *g)
{
  sntrup4591761_small fg[sntrup4591761_p + sntrup4591761_p - 1];
  sntrup4591761_small result;
  int i, j;

  for (i = 0;i < sntrup4591761_p;++i) {
    result = 0;
    for (j = 0;j <= i;++j)
      result = mod3_plusproduct(result,f[j],g[i - j]);
    fg[i] = result;
  }
  for (i = sntrup4591761_p;i < sntrup4591761_p + sntrup4591761_p - 1;++i) {
    result = 0;
    for (j = i - sntrup4591761_p + 1;j < sntrup4591761_p;++j)
      result = mod3_plusproduct(result,f[j],g[i - j]);
    fg[i] = result;
  }

  for (i = sntrup4591761_p + sntrup4591761_p - 2;i >= sntrup4591761_p;--i) {
    fg[i - sntrup4591761_p] = mod3_sum(fg[i - sntrup4591761_p],fg[i]);
    fg[i - sntrup4591761_p + 1] = mod3_sum(fg[i - sntrup4591761_p + 1],fg[i]);
  }

  for (i = 0;i < sntrup4591761_p;++i)
    h[i] = fg[i];
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/r3_recip.c */

/* caller must ensure that x-y does not overflow */
static int smaller_mask(int x,int y)
{
  return (x - y) >> 31;
}

static void vectormod3_product(sntrup4591761_small *z,int len,const sntrup4591761_small *x,const sntrup4591761_small c)
{
  int i;
  for (i = 0;i < len;++i) z[i] = mod3_product(x[i],c);
}

static void vectormod3_minusproduct(sntrup4591761_small *z,int len,const sntrup4591761_small *x,const sntrup4591761_small *y,const sntrup4591761_small c)
{
  int i;
  for (i = 0;i < len;++i) z[i] = mod3_minusproduct(x[i],y[i],c);
}

static void vectormod3_shift(sntrup4591761_small *z,int len)
{
  int i;
  for (i = len - 1;i > 0;--i) z[i] = z[i - 1];
  z[0] = 0;
}

/*
r = s^(-1) mod m, returning 0, if s is invertible mod m
or returning -1 if s is not invertible mod m
r,s are polys of degree <p
m is x^p-x-1
*/
int r3_recip(sntrup4591761_small *r,const sntrup4591761_small *s)
{
  const int loops = 2*sntrup4591761_p + 1;
  int loop;
  sntrup4591761_small f[sntrup4591761_p + 1]; 
  sntrup4591761_small g[sntrup4591761_p + 1]; 
  sntrup4591761_small u[loops + 1];
  sntrup4591761_small v[loops + 1];
  sntrup4591761_small c;
  int i;
  int d = sntrup4591761_p;
  int e = sntrup4591761_p;
  int swapmask;

  for (i = 2;i < sntrup4591761_p;++i) f[i] = 0;
  f[0] = -1;
  f[1] = -1;
  f[sntrup4591761_p] = 1;
  /* generalization: can initialize f to any polynomial m */
  /* requirements: m has degree exactly p, nonzero constant coefficient */

  for (i = 0;i < sntrup4591761_p;++i) g[i] = s[i];
  g[sntrup4591761_p] = 0;

  for (i = 0;i <= loops;++i) u[i] = 0;

  v[0] = 1;
  for (i = 1;i <= loops;++i) v[i] = 0;

  loop = 0;
  for (;;) {
    /* e == -1 or d + e + loop <= 2*p */

    /* f has degree p: i.e., f[p]!=0 */
    /* f[i]==0 for i < p-d */

    /* g has degree <=p (so it fits in p+1 coefficients) */
    /* g[i]==0 for i < p-e */

    /* u has degree <=loop (so it fits in loop+1 coefficients) */
    /* u[i]==0 for i < p-d */
    /* if invertible: u[i]==0 for i < loop-p (so can look at just p+1 coefficients) */

    /* v has degree <=loop (so it fits in loop+1 coefficients) */
    /* v[i]==0 for i < p-e */
    /* v[i]==0 for i < loop-p (so can look at just p+1 coefficients) */

    if (loop >= loops) break;

    c = mod3_quotient(g[sntrup4591761_p],f[sntrup4591761_p]);

    vectormod3_minusproduct(g,sntrup4591761_p + 1,g,f,c);
    vectormod3_shift(g,sntrup4591761_p + 1);

#ifdef SIMPLER
    vectormod3_minusproduct(v,loops + 1,v,u,c);
    vectormod3_shift(v,loops + 1);
#else
    if (loop < sntrup4591761_p) {
      vectormod3_minusproduct(v,loop + 1,v,u,c);
      vectormod3_shift(v,loop + 2);
    } else {
      vectormod3_minusproduct(v + loop - sntrup4591761_p,sntrup4591761_p + 1,v + loop - sntrup4591761_p,u + loop - sntrup4591761_p,c);
      vectormod3_shift(v + loop - sntrup4591761_p,sntrup4591761_p + 2);
    }
#endif

    e -= 1;

    ++loop;

    swapmask = smaller_mask(e,d) & mod3_nonzero_mask(g[sntrup4591761_p]);
    swap(&e,&d,sizeof e,swapmask);
    swap(f,g,(sntrup4591761_p + 1) * sizeof(sntrup4591761_small),swapmask);

#ifdef SIMPLER
    swap(u,v,(loops + 1) * sizeof(sntrup4591761_small),swapmask);
#else
    if (loop < sntrup4591761_p) {
      swap(u,v,(loop + 1) * sizeof(sntrup4591761_small),swapmask);
    } else {
      swap(u + loop - sntrup4591761_p,v + loop - sntrup4591761_p,(sntrup4591761_p + 1) * sizeof(sntrup4591761_small),swapmask);
    }
#endif
  }

  c = mod3_reciprocal(f[sntrup4591761_p]);
  vectormod3_product(r,sntrup4591761_p,u + sntrup4591761_p,c);
  return smaller_mask(0,d);
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/modq.h */

/* -1 if x is nonzero, 0 otherwise */
static inline int modq_nonzero_mask(sntrup4591761_modq x)
{
  crypto_int32 r = (crypto_uint16) x;
  r = -r;
  r >>= 30;
  return r;
}

/* input between -9000000 and 9000000 */
/* output between -2295 and 2295 */
static inline sntrup4591761_modq modq_freeze(crypto_int32 a)
{
  a -= 4591 * ((228 * a) >> 20);
  a -= 4591 * ((58470 * a + 134217728) >> 28);
  return a;
}

static inline sntrup4591761_modq modq_minusproduct(sntrup4591761_modq a,sntrup4591761_modq b,sntrup4591761_modq c)
{
  crypto_int32 A = a;
  crypto_int32 B = b;
  crypto_int32 C = c;
  return modq_freeze(A - B * C);
}

static inline sntrup4591761_modq modq_plusproduct(sntrup4591761_modq a,sntrup4591761_modq b,sntrup4591761_modq c)
{
  crypto_int32 A = a;
  crypto_int32 B = b;
  crypto_int32 C = c;
  return modq_freeze(A + B * C);
}

static inline sntrup4591761_modq modq_product(sntrup4591761_modq a,sntrup4591761_modq b)
{
  crypto_int32 A = a;
  crypto_int32 B = b;
  return modq_freeze(A * B);
}

static inline sntrup4591761_modq modq_square(sntrup4591761_modq a)
{
  crypto_int32 A = a;
  return modq_freeze(A * A);
}

static inline sntrup4591761_modq modq_sum(sntrup4591761_modq a,sntrup4591761_modq b)
{
  crypto_int32 A = a;
  crypto_int32 B = b;
  return modq_freeze(A + B);
}

static inline sntrup4591761_modq modq_reciprocal(sntrup4591761_modq a1)
{
  sntrup4591761_modq a2 = modq_square(a1);
  sntrup4591761_modq a3 = modq_product(a2,a1);
  sntrup4591761_modq a4 = modq_square(a2);
  sntrup4591761_modq a8 = modq_square(a4);
  sntrup4591761_modq a16 = modq_square(a8);
  sntrup4591761_modq a32 = modq_square(a16);
  sntrup4591761_modq a35 = modq_product(a32,a3);
  sntrup4591761_modq a70 = modq_square(a35);
  sntrup4591761_modq a140 = modq_square(a70);
  sntrup4591761_modq a143 = modq_product(a140,a3);
  sntrup4591761_modq a286 = modq_square(a143);
  sntrup4591761_modq a572 = modq_square(a286);
  sntrup4591761_modq a1144 = modq_square(a572);
  sntrup4591761_modq a1147 = modq_product(a1144,a3);
  sntrup4591761_modq a2294 = modq_square(a1147);
  sntrup4591761_modq a4588 = modq_square(a2294);
  sntrup4591761_modq a4589 = modq_product(a4588,a1);
  return a4589;
}

static inline sntrup4591761_modq modq_quotient(sntrup4591761_modq num,sntrup4591761_modq den)
{
  return modq_product(num,modq_reciprocal(den));
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/rq.c */

static void rq_encode(unsigned char *c,const sntrup4591761_modq *f)
{
  crypto_int32 f0, f1, f2, f3, f4;
  int i;

  for (i = 0;i < sntrup4591761_p/5;++i) {
    f0 = *f++ + sntrup4591761_qshift;
    f1 = *f++ + sntrup4591761_qshift;
    f2 = *f++ + sntrup4591761_qshift;
    f3 = *f++ + sntrup4591761_qshift;
    f4 = *f++ + sntrup4591761_qshift;
    /* now want f0 + 6144*f1 + ... as a 64-bit integer */
    f1 *= 3;
    f2 *= 9;
    f3 *= 27;
    f4 *= 81;
    /* now want f0 + f1<<11 + f2<<22 + f3<<33 + f4<<44 */
    f0 += f1 << 11;
    *c++ = f0; f0 >>= 8;
    *c++ = f0; f0 >>= 8;
    f0 += f2 << 6;
    *c++ = f0; f0 >>= 8;
    *c++ = f0; f0 >>= 8;
    f0 += f3 << 1;
    *c++ = f0; f0 >>= 8;
    f0 += f4 << 4;
    *c++ = f0; f0 >>= 8;
    *c++ = f0; f0 >>= 8;
    *c++ = f0;
  }
  /* XXX: using p mod 5 = 1 */
  f0 = *f++ + sntrup4591761_qshift;
  *c++ = f0; f0 >>= 8;
  *c++ = f0;
}

static void rq_decode(sntrup4591761_modq *f,const unsigned char *c)
{
  crypto_uint32 c0, c1, c2, c3, c4, c5, c6, c7;
  crypto_uint32 f0, f1, f2, f3, f4;
  int i;

  for (i = 0;i < sntrup4591761_p/5;++i) {
    c0 = *c++;
    c1 = *c++;
    c2 = *c++;
    c3 = *c++;
    c4 = *c++;
    c5 = *c++;
    c6 = *c++;
    c7 = *c++;

    /* f0 + f1*6144 + f2*6144^2 + f3*6144^3 + f4*6144^4 */
    /* = c0 + c1*256 + ... + c6*256^6 + c7*256^7 */
    /* with each f between 0 and 4590 */

    c6 += c7 << 8;
    /* c6 <= 23241 = floor(4591*6144^4/2^48) */
    /* f4 = (16/81)c6 + (1/1296)(c5+[0,1]) - [0,0.75] */
    /* claim: 2^19 f4 < x < 2^19(f4+1) */
    /* where x = 103564 c6 + 405(c5+1) */
    /* proof: x - 2^19 f4 = (76/81)c6 + (37/81)c5 + 405 - (32768/81)[0,1] + 2^19[0,0.75] */
    /* at least 405 - 32768/81 > 0 */
    /* at most (76/81)23241 + (37/81)255 + 405 + 2^19 0.75 < 2^19 */
    f4 = (103564*c6 + 405*(c5+1)) >> 19;

    c5 += c6 << 8;
    c5 -= (f4 * 81) << 4;
    c4 += c5 << 8;

    /* f0 + f1*6144 + f2*6144^2 + f3*6144^3 */
    /* = c0 + c1*256 + c2*256^2 + c3*256^3 + c4*256^4 */
    /* c4 <= 247914 = floor(4591*6144^3/2^32) */
    /* f3 = (1/54)(c4+[0,1]) - [0,0.75] */
    /* claim: 2^19 f3 < x < 2^19(f3+1) */
    /* where x = 9709(c4+2) */
    /* proof: x - 2^19 f3 = 19418 - (1/27)c4 - (262144/27)[0,1] + 2^19[0,0.75] */
    /* at least 19418 - 247914/27 - 262144/27 > 0 */
    /* at most 19418 + 2^19 0.75 < 2^19 */
    f3 = (9709*(c4+2)) >> 19;

    c4 -= (f3 * 27) << 1;
    c3 += c4 << 8;
    /* f0 + f1*6144 + f2*6144^2 */
    /* = c0 + c1*256 + c2*256^2 + c3*256^3 */
    /* c3 <= 10329 = floor(4591*6144^2/2^24) */
    /* f2 = (4/9)c3 + (1/576)c2 + (1/147456)c1 + (1/37748736)c0 - [0,0.75] */
    /* claim: 2^19 f2 < x < 2^19(f2+1) */
    /* where x = 233017 c3 + 910(c2+2) */
    /* proof: x - 2^19 f2 = 1820 + (1/9)c3 - (2/9)c2 - (32/9)c1 - (1/72)c0 + 2^19[0,0.75] */
    /* at least 1820 - (2/9)255 - (32/9)255 - (1/72)255 > 0 */
    /* at most 1820 + (1/9)10329 + 2^19 0.75 < 2^19 */
    f2 = (233017*c3 + 910*(c2+2)) >> 19;

    c2 += c3 << 8;
    c2 -= (f2 * 9) << 6;
    c1 += c2 << 8;
    /* f0 + f1*6144 */
    /* = c0 + c1*256 */
    /* c1 <= 110184 = floor(4591*6144/2^8) */
    /* f1 = (1/24)c1 + (1/6144)c0 - (1/6144)f0 */
    /* claim: 2^19 f1 < x < 2^19(f1+1) */
    /* where x = 21845(c1+2) + 85 c0 */
    /* proof: x - 2^19 f1 = 43690 - (1/3)c1 - (1/3)c0 + 2^19 [0,0.75] */
    /* at least 43690 - (1/3)110184 - (1/3)255 > 0 */
    /* at most 43690 + 2^19 0.75 < 2^19 */
    f1 = (21845*(c1+2) + 85*c0) >> 19;

    c1 -= (f1 * 3) << 3;
    c0 += c1 << 8;
    f0 = c0;

    *f++ = modq_freeze(f0 + sntrup4591761_q - sntrup4591761_qshift);
    *f++ = modq_freeze(f1 + sntrup4591761_q - sntrup4591761_qshift);
    *f++ = modq_freeze(f2 + sntrup4591761_q - sntrup4591761_qshift);
    *f++ = modq_freeze(f3 + sntrup4591761_q - sntrup4591761_qshift);
    *f++ = modq_freeze(f4 + sntrup4591761_q - sntrup4591761_qshift);
  }

  c0 = *c++;
  c1 = *c++;
  c0 += c1 << 8;
  *f++ = modq_freeze(c0 + sntrup4591761_q - sntrup4591761_qshift);
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/rq_mult.c */

static void rq_mult(sntrup4591761_modq *h,const sntrup4591761_modq *f,const sntrup4591761_small *g)
{
  sntrup4591761_modq fg[sntrup4591761_p + sntrup4591761_p - 1];
  sntrup4591761_modq result;
  int i, j;

  for (i = 0;i < sntrup4591761_p;++i) {
    result = 0;
    for (j = 0;j <= i;++j)
      result = modq_plusproduct(result,f[j],g[i - j]);
    fg[i] = result;
  }
  for (i = sntrup4591761_p;i < sntrup4591761_p + sntrup4591761_p - 1;++i) {
    result = 0;
    for (j = i - sntrup4591761_p + 1;j < sntrup4591761_p;++j)
      result = modq_plusproduct(result,f[j],g[i - j]);
    fg[i] = result;
  }

  for (i = sntrup4591761_p + sntrup4591761_p - 2;i >= sntrup4591761_p;--i) {
    fg[i - sntrup4591761_p] = modq_sum(fg[i - sntrup4591761_p],fg[i]);
    fg[i - sntrup4591761_p + 1] = modq_sum(fg[i - sntrup4591761_p + 1],fg[i]);
  }

  for (i = 0;i < sntrup4591761_p;++i)
    h[i] = fg[i];
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/rq_recip3.c */

/* caller must ensure that x-y does not overflow */
static int smaller_maskq(int x,int y)
{
  return (x - y) >> 31;
}

static void vectormodq_product(sntrup4591761_modq *z,int len,const sntrup4591761_modq *x,const sntrup4591761_modq c)
{
  int i;
  for (i = 0;i < len;++i) z[i] = modq_product(x[i],c);
}

static void vectormodq_minusproduct(sntrup4591761_modq *z,int len,const sntrup4591761_modq *x,const sntrup4591761_modq *y,const sntrup4591761_modq c)
{
  int i;
  for (i = 0;i < len;++i) z[i] = modq_minusproduct(x[i],y[i],c);
}

static void vectormodq_shift(sntrup4591761_modq *z,int len)
{
  int i;
  for (i = len - 1;i > 0;--i) z[i] = z[i - 1];
  z[0] = 0;
}

/*
r = (3s)^(-1) mod m, returning 0, if s is invertible mod m
or returning -1 if s is not invertible mod m
r,s are polys of degree <p
m is x^p-x-1
*/
static int rq_recip3(sntrup4591761_modq *r,const sntrup4591761_small *s)
{
  const int loops = 2*sntrup4591761_p + 1;
  int loop;
  sntrup4591761_modq f[sntrup4591761_p + 1]; 
  sntrup4591761_modq g[sntrup4591761_p + 1]; 
  sntrup4591761_modq u[loops + 1];
  sntrup4591761_modq v[loops + 1];
  sntrup4591761_modq c;
  int i;
  int d = sntrup4591761_p;
  int e = sntrup4591761_p;
  int swapmask;

  for (i = 2;i < sntrup4591761_p;++i) f[i] = 0;
  f[0] = -1;
  f[1] = -1;
  f[sntrup4591761_p] = 1;
  /* generalization: can initialize f to any polynomial m */
  /* requirements: m has degree exactly p, nonzero constant coefficient */

  for (i = 0;i < sntrup4591761_p;++i) g[i] = 3 * s[i];
  g[sntrup4591761_p] = 0;

  for (i = 0;i <= loops;++i) u[i] = 0;

  v[0] = 1;
  for (i = 1;i <= loops;++i) v[i] = 0;

  loop = 0;
  for (;;) {
    /* e == -1 or d + e + loop <= 2*p */

    /* f has degree p: i.e., f[p]!=0 */
    /* f[i]==0 for i < p-d */

    /* g has degree <=p (so it fits in p+1 coefficients) */
    /* g[i]==0 for i < p-e */

    /* u has degree <=loop (so it fits in loop+1 coefficients) */
    /* u[i]==0 for i < p-d */
    /* if invertible: u[i]==0 for i < loop-p (so can look at just p+1 coefficients) */

    /* v has degree <=loop (so it fits in loop+1 coefficients) */
    /* v[i]==0 for i < p-e */
    /* v[i]==0 for i < loop-p (so can look at just p+1 coefficients) */

    if (loop >= loops) break;

    c = modq_quotient(g[sntrup4591761_p],f[sntrup4591761_p]);

    vectormodq_minusproduct(g,sntrup4591761_p + 1,g,f,c);
    vectormodq_shift(g,sntrup4591761_p + 1);

#ifdef SIMPLER
    vectormodq_minusproduct(v,loops + 1,v,u,c);
    vectormodq_shift(v,loops + 1);
#else
    if (loop < sntrup4591761_p) {
      vectormodq_minusproduct(v,loop + 1,v,u,c);
      vectormodq_shift(v,loop + 2);
    } else {
      vectormodq_minusproduct(v + loop - sntrup4591761_p,sntrup4591761_p + 1,v + loop - sntrup4591761_p,u + loop - sntrup4591761_p,c);
      vectormodq_shift(v + loop - sntrup4591761_p,sntrup4591761_p + 2);
    }
#endif

    e -= 1;

    ++loop;

    swapmask = smaller_maskq(e,d) & modq_nonzero_mask(g[sntrup4591761_p]);
    swap(&e,&d,sizeof e,swapmask);
    swap(f,g,(sntrup4591761_p + 1) * sizeof(sntrup4591761_modq),swapmask);

#ifdef SIMPLER
    swap(u,v,(loops + 1) * sizeof(sntrup4591761_modq),swapmask);
#else
    if (loop < sntrup4591761_p) {
      swap(u,v,(loop + 1) * sizeof(sntrup4591761_modq),swapmask);
    } else {
      swap(u + loop - sntrup4591761_p,v + loop - sntrup4591761_p,(sntrup4591761_p + 1) * sizeof(sntrup4591761_modq),swapmask);
    }
#endif
  }

  c = modq_reciprocal(f[sntrup4591761_p]);
  vectormodq_product(r,sntrup4591761_p,u + sntrup4591761_p,c);
  return smaller_maskq(0,d);
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/rq_round3.c */

static void rq_round3(sntrup4591761_modq *h,const sntrup4591761_modq *f)
{
  int i;

  for (i = 0;i < sntrup4591761_p;++i)
    h[i] = ((21846 * (f[i] + 2295) + 32768) >> 16) * 3 - 2295;
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/rq_rounded.c */

static void rq_encoderounded(unsigned char *c,const sntrup4591761_modq *f)
{
  crypto_int32 f0, f1, f2;
  int i;

  for (i = 0;i < sntrup4591761_p/3;++i) {
    f0 = *f++ + sntrup4591761_qshift;
    f1 = *f++ + sntrup4591761_qshift;
    f2 = *f++ + sntrup4591761_qshift;
    f0 = (21846 * f0) >> 16;
    f1 = (21846 * f1) >> 16;
    f2 = (21846 * f2) >> 16;
    /* now want f0 + f1*1536 + f2*1536^2 as a 32-bit integer */
    f2 *= 3;
    f1 += f2 << 9;
    f1 *= 3;
    f0 += f1 << 9;
    *c++ = f0; f0 >>= 8;
    *c++ = f0; f0 >>= 8;
    *c++ = f0; f0 >>= 8;
    *c++ = f0;
  }
  /* XXX: using p mod 3 = 2 */
  f0 = *f++ + sntrup4591761_qshift;
  f1 = *f++ + sntrup4591761_qshift;
  f0 = (21846 * f0) >> 16;
  f1 = (21846 * f1) >> 16;
  f1 *= 3;
  f0 += f1 << 9;
  *c++ = f0; f0 >>= 8;
  *c++ = f0; f0 >>= 8;
  *c++ = f0;
}

static void rq_decoderounded(sntrup4591761_modq *f,const unsigned char *c)
{
  crypto_uint32 c0, c1, c2, c3;
  crypto_uint32 f0, f1, f2;
  int i;

  for (i = 0;i < sntrup4591761_p/3;++i) {
    c0 = *c++;
    c1 = *c++;
    c2 = *c++;
    c3 = *c++;

    /* f0 + f1*1536 + f2*1536^2 */
    /* = c0 + c1*256 + c2*256^2 + c3*256^3 */
    /* with each f between 0 and 1530 */

    /* f2 = (64/9)c3 + (1/36)c2 + (1/9216)c1 + (1/2359296)c0 - [0,0.99675] */
    /* claim: 2^21 f2 < x < 2^21(f2+1) */
    /* where x = 14913081*c3 + 58254*c2 + 228*(c1+2) */
    /* proof: x - 2^21 f2 = 456 - (8/9)c0 + (4/9)c1 - (2/9)c2 + (1/9)c3 + 2^21 [0,0.99675] */
    /* at least 456 - (8/9)255 - (2/9)255 > 0 */
    /* at most 456 + (4/9)255 + (1/9)255 + 2^21 0.99675 < 2^21 */
    f2 = (14913081*c3 + 58254*c2 + 228*(c1+2)) >> 21;

    c2 += c3 << 8;
    c2 -= (f2 * 9) << 2;
    /* f0 + f1*1536 */
    /* = c0 + c1*256 + c2*256^2 */
    /* c2 <= 35 = floor((1530+1530*1536)/256^2) */
    /* f1 = (128/3)c2 + (1/6)c1 + (1/1536)c0 - (1/1536)f0 */
    /* claim: 2^21 f1 < x < 2^21(f1+1) */
    /* where x = 89478485*c2 + 349525*c1 + 1365*(c0+1) */
    /* proof: x - 2^21 f1 = 1365 - (1/3)c2 - (1/3)c1 - (1/3)c0 + (4096/3)f0 */
    /* at least 1365 - (1/3)35 - (1/3)255 - (1/3)255 > 0 */
    /* at most 1365 + (4096/3)1530 < 2^21 */
    f1 = (89478485*c2 + 349525*c1 + 1365*(c0+1)) >> 21;

    c1 += c2 << 8;
    c1 -= (f1 * 3) << 1;

    c0 += c1 << 8;
    f0 = c0;

    *f++ = modq_freeze(f0 * 3 + sntrup4591761_q - sntrup4591761_qshift);
    *f++ = modq_freeze(f1 * 3 + sntrup4591761_q - sntrup4591761_qshift);
    *f++ = modq_freeze(f2 * 3 + sntrup4591761_q - sntrup4591761_qshift);
  }

  c0 = *c++;
  c1 = *c++;
  c2 = *c++;

  f1 = (89478485*c2 + 349525*c1 + 1365*(c0+1)) >> 21;

  c1 += c2 << 8;
  c1 -= (f1 * 3) << 1;

  c0 += c1 << 8;
  f0 = c0;

  *f++ = modq_freeze(f0 * 3 + sntrup4591761_q - sntrup4591761_qshift);
  *f++ = modq_freeze(f1 * 3 + sntrup4591761_q - sntrup4591761_qshift);
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/keypair.c */

int	sntrup4591761_keypair(unsigned char *pk, unsigned char *sk)
{
  sntrup4591761_small g[sntrup4591761_p];
  sntrup4591761_small grecip[sntrup4591761_p];
  sntrup4591761_small f[sntrup4591761_p];
  sntrup4591761_modq f3recip[sntrup4591761_p];
  sntrup4591761_modq h[sntrup4591761_p];

  do
    small_random(g);
  while (r3_recip(grecip,g) != 0);

  small_random_weightw(f);
  rq_recip3(f3recip,f);

  rq_mult(h,f3recip,g);

  rq_encode(pk,h);
  small_encode(sk,f);
  small_encode(sk + small_encode_len,grecip);
  memcpy(sk + 2 * small_encode_len,pk,rq_encode_len);

  return 0;
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/enc.c */

int	sntrup4591761_enc(unsigned char *cstr, unsigned char *k, const unsigned char *pk)
{
  sntrup4591761_small r[sntrup4591761_p];
  sntrup4591761_modq h[sntrup4591761_p];
  sntrup4591761_modq c[sntrup4591761_p];
  unsigned char rstr[small_encode_len];
  unsigned char hash[64];

  small_random_weightw(r);

  small_encode(rstr,r);
  crypto_hash_sha512(hash,rstr,sizeof rstr);

  rq_decode(h,pk);
  rq_mult(c,h,r);
  rq_round3(c,c);

  memcpy(k,hash + 32,32);
  memcpy(cstr,hash,32);
  rq_encoderounded(cstr + 32,c);

  return 0;
}

/* libpqcrypto-20180314/crypto_kem/sntrup4591761/ref/dec.c */

int	sntrup4591761_dec(unsigned char *k, const unsigned char *cstr, const unsigned char *sk)
{
  sntrup4591761_small f[sntrup4591761_p];
  sntrup4591761_modq h[sntrup4591761_p];
  sntrup4591761_small grecip[sntrup4591761_p];
  sntrup4591761_modq c[sntrup4591761_p];
  sntrup4591761_modq t[sntrup4591761_p];
  sntrup4591761_small t3[sntrup4591761_p];
  sntrup4591761_small r[sntrup4591761_p];
  sntrup4591761_modq hr[sntrup4591761_p];
  unsigned char rstr[small_encode_len];
  unsigned char hash[64];
  int i;
  int result = 0;
  int weight;

  small_decode(f,sk);
  small_decode(grecip,sk + small_encode_len);
  rq_decode(h,sk + 2 * small_encode_len);

  rq_decoderounded(c,cstr + 32);

  rq_mult(t,c,f);
  for (i = 0;i < sntrup4591761_p;++i) t3[i] = mod3_freeze(modq_freeze(3*t[i]));

  r3_mult(r,t3,grecip);

  weight = 0;
  for (i = 0;i < sntrup4591761_p;++i) weight += (1 & r[i]);
  weight -= sntrup4591761_w;
  result |= modq_nonzero_mask(weight); /* XXX: puts limit on p */

  rq_mult(hr,h,r);
  rq_round3(hr,hr);
  for (i = 0;i < sntrup4591761_p;++i) result |= modq_nonzero_mask(hr[i] - c[i]);

  small_encode(rstr,r);

  crypto_hash_sha512(hash,rstr,sizeof rstr);
  result |= crypto_verify_32(hash,cstr);

  for (i = 0;i < 32;++i) k[i] = (hash[32 + i] & ~result);
  return result;
}

