#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "PassDlg.h"
#include "IdKeyFileDlg.h"
#include "ssh.h"

#include "ssh1.h"
#include "ssh2.h"

#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// OpenSSH C lib
//////////////////////////////////////////////////////////////////////

#include "openssl/aes.h"
#include "openssl/camellia.h"
#include "openssl/seed.h"
#include "openssl/blowfish.h"
#include "openssl/cast.h"
#include "openssl/idea.h"
#include "openssl/des.h"
#include "openssl/rand.h"

#if	OPENSSL_VERSION_NUMBER >= 0x10100000L
extern "C" {
	#include "internal/evp_int.h"
	#include "internal/chacha.h"
	#include "internal/poly1305.h"
}
#endif

#ifndef _STDINT
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned long    uint32_t;
typedef unsigned __int64 uint64_t;
typedef          char    int8_t;
typedef          short   int16_t;
typedef          long    int32_t;
typedef          __int64 int64_t;
#endif

#ifdef	USE_NETTLE
	#include "nettle-2.0/twofish.cpp"
	#include "nettle-2.0/serpent.cpp"
#endif

#ifdef	USE_CLEFIA
	#include "nettle-2.0/clefia.cpp"
#endif

#if 0
extern "C" {
	#include "internal/cryptlib.h"

	void *DEBUG_malloc(size_t size, const char *file, int line)
	{
		void *p = malloc(size);

		if ( size == 40 )
			TRACE("%08X %s(%d)\n", p, file, line);

		return p;
	}
	void *DEBUG_realloc(void *pMem, size_t size, const char *file, int line)
	{
		return realloc(pMem, size);
	}
	void DEBUG_free(void *pMem, const char *file, int line)
	{
		free(pMem);
	}
}
void DEBUG_init()
{
	CRYPTO_set_mem_functions(DEBUG_malloc, DEBUG_realloc, DEBUG_free);
}
#endif

//////////////////////////////////////////////////////////////////////

#if	OPENSSL_VERSION_NUMBER < 0x10100000L

EVP_MD_CTX *EVP_MD_CTX_new(void)
{
	EVP_MD_CTX *pCtx;

	if ( (pCtx = (EVP_MD_CTX *)malloc(sizeof(EVP_MD_CTX))) == NULL )
		return NULL;

	ZeroMemory(pCtx, sizeof(EVP_MD_CTX));
	return pCtx;
}
void EVP_MD_CTX_free(EVP_MD_CTX *pCtx)
{
	if ( pCtx == NULL )
		return;

	ZeroMemory(pCtx, sizeof(EVP_MD_CTX));
	free(pCtx);
}
HMAC_CTX *HMAC_CTX_new(void)
{
	HMAC_CTX *pHmac;

	if ( (pHmac = (HMAC_CTX *)malloc(sizeof(HMAC_CTX))) == NULL )
		return NULL;

	ZeroMemory(pHmac, sizeof(HMAC_CTX));
	return pHmac;
}
void HMAC_CTX_free(HMAC_CTX *pHmac)
{
	if ( pHmac == NULL )
		return;

	HMAC_CTX_cleanup(pHmac);
	free(pHmac);
}

const EVP_CIPHER *EVP_CIPHER_CTX_cipher(const EVP_CIPHER_CTX *ctx)
{
    return ctx->cipher;
}
int EVP_CIPHER_CTX_block_size(const EVP_CIPHER_CTX *ctx)
{
    return ctx->cipher->block_size;
}
int EVP_CIPHER_CTX_encrypting(const EVP_CIPHER_CTX *ctx)
{
    return ctx->encrypt;
}
int EVP_PKEY_id(const EVP_PKEY *pkey)
{
    return pkey->type;
}

int RSA_bits(const RSA *r)
{
    return (BN_num_bits(r->n));
}
int RSA_size(const RSA *r)
{
    return (BN_num_bytes(r->n));
}
void RSA_get0_key(const RSA *r, const BIGNUM **n, const BIGNUM **e, const BIGNUM **d)
{
    if ( n != NULL ) *n = r->n;
    if ( e != NULL ) *e = r->e;
    if ( d != NULL ) *d = r->d;
}
void RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q)
{
    if ( p != NULL ) *p = r->p;
    if ( q != NULL ) *q = r->q;
}
void RSA_get0_crt_params(const RSA *r, const BIGNUM **dmp1, const BIGNUM **dmq1, const BIGNUM **iqmp)
{
    if ( dmp1 != NULL ) *dmp1 = r->dmp1;
    if ( dmq1 != NULL ) *dmq1 = r->dmq1;
    if ( iqmp != NULL ) *iqmp = r->iqmp;
}
int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d)
{
    if ( (r->n == NULL && n == NULL) || (r->e == NULL && e == NULL) )
		return 0;

    if ( n != NULL ) { BN_free(r->n); r->n = n; }
    if ( e != NULL ) { BN_free(r->e); r->e = e; }
    if ( d != NULL ) { BN_free(r->d); r->d = d; }

    return 1;
}
int RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q)
{
    if ( (r->p == NULL && p == NULL) || (r->q == NULL && q == NULL) )
		return 0;

    if ( p != NULL ) { BN_free(r->p); r->p = p; }
    if ( q != NULL ) { BN_free(r->q); r->q = q; }

    return 1;
}
int RSA_set0_crt_params(RSA *r, BIGNUM *dmp1, BIGNUM *dmq1, BIGNUM *iqmp)
{
    if ( (r->dmp1 == NULL && dmp1 == NULL) || (r->dmq1 == NULL && dmq1 == NULL) || (r->iqmp == NULL && iqmp == NULL) )
		return 0;

    if ( dmp1 != NULL ) { BN_free(r->dmp1); r->dmp1 = dmp1; }
    if ( dmq1 != NULL ) { BN_free(r->dmq1); r->dmq1 = dmq1; }
    if ( iqmp != NULL ) { BN_free(r->iqmp); r->iqmp = iqmp; }

    return 1;
}

int DSA_bits(const DSA *dsa)
{
    return BN_num_bits(dsa->p);
}
void DSA_get0_pqg(const DSA *d, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g)
{
    if ( p != NULL ) *p = d->p;
    if ( q != NULL ) *q = d->q;
    if ( g != NULL ) *g = d->g;
}
int DSA_set0_pqg(DSA *d, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
    if ( (d->p == NULL && p == NULL) || (d->q == NULL && q == NULL) || (d->g == NULL && g == NULL) )
		return 0;

    if ( p != NULL ) { BN_free(d->p); d->p = p; }
    if ( q != NULL ) { BN_free(d->q); d->q = q; }
    if ( g != NULL ) { BN_free(d->g); d->g = g; }

    return 1;
}
void DSA_get0_key(const DSA *d, const BIGNUM **pub_key, const BIGNUM **priv_key)
{
    if ( pub_key != NULL ) *pub_key = d->pub_key;
    if ( priv_key != NULL ) *priv_key = d->priv_key;
}
int DSA_set0_key(DSA *d, BIGNUM *pub_key, BIGNUM *priv_key)
{
    if ( d->pub_key == NULL && pub_key == NULL )
		return 0;

    if ( pub_key != NULL ) { BN_free(d->pub_key); d->pub_key = pub_key; }
    if ( priv_key != NULL ) { BN_free(d->priv_key); d->priv_key = priv_key; }

    return 1;
}

void DSA_SIG_get0(const DSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps)
{
    if ( pr != NULL ) *pr = sig->r;
    if ( ps != NULL ) *ps = sig->s;
}
int DSA_SIG_set0(DSA_SIG *sig, BIGNUM *r, BIGNUM *s)
{
    if ( r == NULL || s == NULL )
		return 0;

	BN_clear_free(sig->r);
    BN_clear_free(sig->s);
    sig->r = r;
    sig->s = s;

	return 1;
}

void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps)
{
    if ( pr != NULL ) *pr = sig->r;
    if ( ps != NULL ) *ps = sig->s;
}

int ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s)
{
    if ( r == NULL || s == NULL )
		return 0;

    BN_clear_free(sig->r);
    BN_clear_free(sig->s);
    sig->r = r;
    sig->s = s;

    return 1;
}

int DH_bits(const DH *dh)
{
    return BN_num_bits(dh->p);
}
void DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g)
{
    if ( p != NULL ) *p = dh->p;
    if ( q != NULL ) *q = dh->q;
    if ( g != NULL ) *g = dh->g;
}
int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
    if ( (dh->p == NULL && p == NULL) || (dh->g == NULL && g == NULL) )
		return 0;

    if ( p != NULL ) { BN_free(dh->p); dh->p = p; }
    if ( q != NULL ) { BN_free(dh->q); dh->q = q; dh->length = BN_num_bits(q); }
    if ( g != NULL ) { BN_free(dh->g); dh->g = g; }

    return 1;
}
void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key)
{
    if ( pub_key != NULL ) *pub_key = dh->pub_key;
    if ( priv_key != NULL ) *priv_key = dh->priv_key;
}
int DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key)
{
    if ( dh->pub_key == NULL && pub_key == NULL )
		return 0;

    if ( pub_key != NULL ) { BN_free(dh->pub_key); dh->pub_key = pub_key; }
    if ( priv_key != NULL ) { BN_free(dh->priv_key); dh->priv_key = priv_key; }

    return 1;
}
int DH_set_length(DH *dh, long length)
{
    dh->length = length;
    return 1;
}
#endif

//////////////////////////////////////////////////////////////////////

#define	MT_BUF_SIZE	16
#define	MT_BUF_MAX	(8 * 1024)
#define	MT_BUF_MIN	(MT_BUF_MAX - 2048)

struct	mt_ctr_ctx {
	int			top;
	int			pos;
	volatile int len;
	void		*ctx;
	void		(*enc)(void *ctx, u_char *buf);
	u_char		buf[MT_BUF_MAX][MT_BUF_SIZE];
};

BOOL mt_proc(LPVOID pParam)
{
	BOOL ret = FALSE;
	struct mt_ctr_ctx *c = (struct mt_ctr_ctx *)pParam;

	for ( int i = 0 ; i < 256 ; i++ ) {
		if ( c->len >= MT_BUF_MAX )
			break;
		(*(c->enc))(c->ctx, c->buf[c->top]);
		c->top = (c->top + 1) % MT_BUF_MAX;
		c->len++;
		ret = TRUE;
	}

	return ret;
}

static struct mt_ctr_ctx *mt_init(void *ctx, void (*enc)(void *ctx, u_char *buf))
{
	struct mt_ctr_ctx *c = new struct mt_ctr_ctx;

	c->top = c->pos = 0;
	c->len = 0;
	c->ctx = ctx;
	c->enc = enc;

	((CRLoginApp *)AfxGetApp())->AddIdleProc(IDLEPROC_ENCRYPT, c);

	return c;
}
static void mt_finis(struct mt_ctr_ctx *c)
{
	if ( c == NULL )
		return;

	((CRLoginApp *)AfxGetApp())->DelIdleProc(IDLEPROC_ENCRYPT, c);

	delete c;
}
static u_char *mt_get_buf(struct mt_ctr_ctx *c)
{
	if ( c->len == 0 ) {
		(*(c->enc))(c->ctx, c->buf[c->top]);
		c->top = (c->top + 1) % MT_BUF_MAX;
		c->len++;
	}
	return c->buf[c->pos];
}
static void mt_inc_buf(struct mt_ctr_ctx *c)
{
	c->pos = (c->pos + 1) % MT_BUF_MAX;
	c->len--;
}

//////////////////////////////////////////////////////////////////////

struct ssh_cipher_ctx
{
	struct mt_ctr_ctx *mt_ctx;
};

/*
 * increment counter 'ctr',
 * the counter is of size 'len' bytes and stored in network-byte-order.
 * (LSB at ctr[len-1], MSB at ctr[0])
 */
static void ssh_ctr_inc(u_char *ctr, int len)
{
	ctr += len;
	while ( --len > 0 )
		if ( ++*(--ctr) )   /* continue on overflow */
			return;
}
static int ssh_cipher_ctr(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	struct ssh_cipher_ctx *c;
	register int i;
	register size_t *spdst, *spsrc, *spwrk;
	register u_char *wrk;

	if ((c = (struct ssh_cipher_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	int block_size = EVP_CIPHER_CTX_block_size(ctx);

	ASSERT(block_size == 8 || block_size == 16);
	ASSERT(sizeof(size_t) == 4 || sizeof(size_t) == 8);

	spdst = (size_t *)dest;
	spsrc = (size_t *)src;

	while ( len >= (size_t)block_size ) {
		spwrk = (size_t *)mt_get_buf(c->mt_ctx);

		//for ( i = ctx->cipher->block_size / sizeof(size_t) ; i > 0 ; i-- )
		//	*(spdst++) = *(spsrc++) ^ *(spwrk++);

		switch(block_size / sizeof(size_t)) {
		case 4:
			*(spdst++) = *(spsrc++) ^ *(spwrk++);
		case 3:
			*(spdst++) = *(spsrc++) ^ *(spwrk++);
		case 2:
			*(spdst++) = *(spsrc++) ^ *(spwrk++);
		case 1:
			*(spdst++) = *(spsrc++) ^ *(spwrk++);
		case 0:
			break;
		}

		//if ( (i = ctx->cipher->block_size % sizeof(size_t)) != 0 ) {
		//	dest = (u_char *)spdst;
		//	src  = (u_char *)spsrc;
		//	wrk  = (u_char *)spwrk;

		//	for ( ; i > 0 ; i-- )
		//		*(dest++) = *(src++) ^ *(wrk++);

		//	spdst = (size_t *)dest;
		//	spsrc = (size_t *)src;
		//}

		len -= block_size;
		mt_inc_buf(c->mt_ctx);
	}

	if ( len > 0 ) {
		spwrk = (size_t *)mt_get_buf(c->mt_ctx);

		for ( i = (int)(len / sizeof(size_t)) ; i > 0 ; i-- )
			*(spdst++) = *(spsrc++) ^ *(spwrk++);

		dest = (u_char *)spdst;
		src  = (u_char *)spsrc;
		wrk  = (u_char *)spwrk;

		for ( i = len % sizeof(size_t) ; i > 0 ; i-- )
			*(dest++) = *(src++) ^ *(wrk++);

		mt_inc_buf(c->mt_ctx);
	}

	return (1);
}
static int ssh_cipher_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_cipher_ctx *c;

	if ((c = (struct ssh_cipher_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		mt_finis(c->mt_ctx);
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

//////////////////////////////////////////////////////////////////////

struct ssh_aes_ctx
{
	struct mt_ctr_ctx	*mt_ctx;	// dont move !!
	AES_KEY 			aes_ctx;
	u_char  			aes_counter[AES_BLOCK_SIZE];
};

static void ssh_aes_enc(void *ctx, u_char *buf)
{
	struct ssh_aes_ctx *c = (struct ssh_aes_ctx *)ctx;

	AES_encrypt(c->aes_counter, buf, &c->aes_ctx);
	ssh_ctr_inc(c->aes_counter, AES_BLOCK_SIZE);
}
static int ssh_aes_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_aes_ctx *c;

	if ((c = (struct ssh_aes_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_aes_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_aes_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		AES_set_encrypt_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8, &c->aes_ctx);
	if (iv != NULL)
		memcpy(c->aes_counter, iv, AES_BLOCK_SIZE);
	return (1);
}
const EVP_CIPHER *evp_aes_128_ctr(void)
{
	static EVP_CIPHER aes_ctr;

	memset(&aes_ctr, 0, sizeof(EVP_CIPHER));
	aes_ctr.nid = NID_undef;
	aes_ctr.block_size = AES_BLOCK_SIZE;
	aes_ctr.iv_len = AES_BLOCK_SIZE;
	aes_ctr.key_len = 16;
	aes_ctr.init = ssh_aes_init;
	aes_ctr.cleanup = ssh_cipher_cleanup;
	aes_ctr.do_cipher = ssh_cipher_ctr;
	aes_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&aes_ctr);
}

//////////////////////////////////////////////////////////////////////

struct ssh_camellia_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	CAMELLIA_KEY		camellia_ctx;
	u_char				camellia_counter[CAMELLIA_BLOCK_SIZE];
};

static void ssh_camellia_enc(void *ctx, u_char *buf)
{
	struct ssh_camellia_ctx *c = (struct ssh_camellia_ctx *)ctx;

	Camellia_encrypt(c->camellia_counter, buf, &c->camellia_ctx);
	ssh_ctr_inc(c->camellia_counter, CAMELLIA_BLOCK_SIZE);
}
static int ssh_camellia_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_camellia_ctx *c;

	if ((c = (struct ssh_camellia_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_camellia_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_camellia_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		Camellia_set_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8, &c->camellia_ctx);
	if (iv != NULL)
		memcpy(c->camellia_counter, iv, CAMELLIA_BLOCK_SIZE);
	return (1);
}
const EVP_CIPHER *evp_camellia_128_ctr(void)
{
	static EVP_CIPHER camellia_ctr;

	memset(&camellia_ctr, 0, sizeof(EVP_CIPHER));
	camellia_ctr.nid = NID_undef;
	camellia_ctr.block_size = CAMELLIA_BLOCK_SIZE;
	camellia_ctr.iv_len = CAMELLIA_BLOCK_SIZE;
	camellia_ctr.key_len = 16;
	camellia_ctr.init = ssh_camellia_init;
	camellia_ctr.cleanup = ssh_cipher_cleanup;
	camellia_ctr.do_cipher = ssh_cipher_ctr;
	camellia_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&camellia_ctr);
}

//////////////////////////////////////////////////////////////////////

struct ssh_seed_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	SEED_KEY_SCHEDULE	seed_ctx;
	u_char				seed_counter[SEED_BLOCK_SIZE];
};

static void ssh_seed_enc(void *ctx, u_char *buf)
{
	struct ssh_seed_ctx *c = (struct ssh_seed_ctx *)ctx;

	SEED_encrypt(c->seed_counter, buf, &c->seed_ctx);
	ssh_ctr_inc(c->seed_counter, SEED_BLOCK_SIZE);
}
static int ssh_seed_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_seed_ctx *c;

	if ((c = (struct ssh_seed_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_seed_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_seed_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		SEED_set_key(key, &c->seed_ctx);
	if (iv != NULL)
		memcpy(c->seed_counter, iv, SEED_BLOCK_SIZE);
	return (1);
}
const EVP_CIPHER *evp_seed_ctr(void)
{
	static EVP_CIPHER seed_ctr;

	memset(&seed_ctr, 0, sizeof(EVP_CIPHER));
	seed_ctr.nid = NID_undef;
	seed_ctr.block_size = SEED_BLOCK_SIZE;
	seed_ctr.iv_len = SEED_BLOCK_SIZE;
	seed_ctr.key_len = 16;
	seed_ctr.init = ssh_seed_init;
	seed_ctr.cleanup = ssh_cipher_cleanup;
	seed_ctr.do_cipher = ssh_cipher_ctr;
	seed_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&seed_ctr);
}

//////////////////////////////////////////////////////////////////////

struct ssh_blowfish_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	BF_KEY				blowfish_ctx;
	u_char				blowfish_counter[BF_BLOCK];
};

static void ssh_blowfish_enc(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	BF_LONG data[BF_BLOCK / 4 + 1];
	struct ssh_blowfish_ctx *c = (struct ssh_blowfish_ctx *)ctx;

	p = c->blowfish_counter;
	for ( i = a = 0 ; i < BF_BLOCK ; i += 4, a++ ) {
		data[a]  = ((BF_LONG)(*(p++)) << 24);
		data[a] |= ((BF_LONG)(*(p++)) << 16);
		data[a] |= ((BF_LONG)(*(p++)) <<  8);
		data[a] |= ((BF_LONG)(*(p++)) <<  0);
	}

	BF_encrypt(data, &c->blowfish_ctx);

	p = buf;
	for ( i = a = 0 ; i < BF_BLOCK ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >> 24);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >>  0);
	}

	ssh_ctr_inc(c->blowfish_counter, BF_BLOCK);
}
static int ssh_blowfish_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_blowfish_ctx *c;

	if ((c = (struct ssh_blowfish_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_blowfish_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_blowfish_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		BF_set_key(&c->blowfish_ctx, EVP_CIPHER_CTX_key_length(ctx), key);
	if (iv != NULL)
		memcpy(c->blowfish_counter, iv, BF_BLOCK);
	return (1);
}
const EVP_CIPHER *evp_bf_ctr(void)
{
	static EVP_CIPHER blowfish_ctr;

	memset(&blowfish_ctr, 0, sizeof(EVP_CIPHER));
	blowfish_ctr.nid = NID_undef;
	blowfish_ctr.block_size = BF_BLOCK;
	blowfish_ctr.iv_len = BF_BLOCK;
	blowfish_ctr.key_len = 32;
	blowfish_ctr.init = ssh_blowfish_init;
	blowfish_ctr.cleanup = ssh_cipher_cleanup;
	blowfish_ctr.do_cipher = ssh_cipher_ctr;
	blowfish_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&blowfish_ctr);
}

//////////////////////////////////////////////////////////////////////

struct ssh_cast_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	CAST_KEY			cast_ctx;
	u_char				cast_counter[CAST_BLOCK];
};

static void ssh_cast_enc(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	CAST_LONG data[CAST_BLOCK / 4 + 1];
	struct ssh_cast_ctx *c = (struct ssh_cast_ctx *)ctx;

	p = c->cast_counter;
	for ( i = a = 0 ; i < CAST_BLOCK ; i += 4, a++ ) {
		data[a]  = ((CAST_LONG)(*(p++)) << 24);
		data[a] |= ((CAST_LONG)(*(p++)) << 16);
		data[a] |= ((CAST_LONG)(*(p++)) <<  8);
		data[a] |= ((CAST_LONG)(*(p++)) <<  0);
	}

	CAST_encrypt(data, &c->cast_ctx);

	p = buf;
	for ( i = a = 0 ; i < CAST_BLOCK ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >> 24);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >>  0);
	}

	ssh_ctr_inc(c->cast_counter, CAST_BLOCK);
}
static int ssh_cast_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_cast_ctx *c;

	if ((c = (struct ssh_cast_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_cast_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_cast_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		CAST_set_key(&c->cast_ctx, EVP_CIPHER_CTX_key_length(ctx), key);
	if (iv != NULL)
		memcpy(c->cast_counter, iv, CAST_BLOCK);
	return (1);
}
const EVP_CIPHER *evp_cast5_ctr(void)
{
	static EVP_CIPHER cast_ctr;

	memset(&cast_ctr, 0, sizeof(EVP_CIPHER));
	cast_ctr.nid = NID_undef;
	cast_ctr.block_size = CAST_BLOCK;
	cast_ctr.iv_len = CAST_BLOCK;
	cast_ctr.key_len = CAST_KEY_LENGTH;
	cast_ctr.init = ssh_cast_init;
	cast_ctr.cleanup = ssh_cipher_cleanup;
	cast_ctr.do_cipher = ssh_cipher_ctr;
	cast_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&cast_ctr);
}

//////////////////////////////////////////////////////////////////////

struct ssh_idea_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	IDEA_KEY_SCHEDULE	idea_ctx;
	u_char				idea_counter[IDEA_BLOCK];
};

static void	ssh_idea_enc(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	IDEA_INT data[IDEA_BLOCK / 4 + 1];
	struct ssh_idea_ctx *c = (struct ssh_idea_ctx *)ctx;

	p = c->idea_counter;
	for ( i = a = 0 ; i < IDEA_BLOCK ; i += 4, a++ ) {
		data[a]  = ((IDEA_INT)(*(p++)) << 24);
		data[a] |= ((IDEA_INT)(*(p++)) << 16);
		data[a] |= ((IDEA_INT)(*(p++)) <<  8);
		data[a] |= ((IDEA_INT)(*(p++)) <<  0);
	}

	idea_encrypt((unsigned long *)data, &c->idea_ctx);

	p = buf;
	for ( i = a = 0 ; i < IDEA_BLOCK ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >> 24);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >>  0);
	}

	ssh_ctr_inc(c->idea_counter, IDEA_BLOCK);
}
static int ssh_idea_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_idea_ctx *c;

	if ((c = (struct ssh_idea_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_idea_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_idea_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		idea_set_encrypt_key(key, &c->idea_ctx);
	if (iv != NULL)
		memcpy(c->idea_counter, iv, IDEA_BLOCK);
	return (1);
}
const EVP_CIPHER *evp_idea_ctr(void)
{
	static EVP_CIPHER idea_ctr;

	memset(&idea_ctr, 0, sizeof(EVP_CIPHER));
	idea_ctr.nid = NID_undef;
	idea_ctr.block_size = IDEA_BLOCK;
	idea_ctr.iv_len = IDEA_BLOCK;
	idea_ctr.key_len = IDEA_KEY_LENGTH;
	idea_ctr.init = ssh_idea_init;
	idea_ctr.cleanup = ssh_cipher_cleanup;
	idea_ctr.do_cipher = ssh_cipher_ctr;
	idea_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&idea_ctr);
}

//////////////////////////////////////////////////////////////////////

struct ssh_des3_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	DES_key_schedule	des3_ctx[3];
	DES_cblock			des3_counter;
};

#define	DES_BLOCK	sizeof(DES_cblock)

static void ssh_des3_enc(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	DES_LONG data[DES_BLOCK / 4 + 1];
	struct ssh_des3_ctx *c = (struct ssh_des3_ctx *)ctx;

	p = c->des3_counter;
	for ( i = a = 0 ; i < DES_BLOCK ; i += 4, a++ ) {
		data[a]  = ((DES_LONG)(*(p++)) <<  0);
		data[a] |= ((DES_LONG)(*(p++)) <<  8);
		data[a] |= ((DES_LONG)(*(p++)) << 16);
		data[a] |= ((DES_LONG)(*(p++)) << 24);
	}

	DES_encrypt3(data, &c->des3_ctx[0], &c->des3_ctx[1], &c->des3_ctx[2]);

	p = buf;
	for ( i = a = 0 ; i < DES_BLOCK ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >>  0);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >> 24);
	}

	ssh_ctr_inc(c->des3_counter, DES_BLOCK);
}
static int ssh_des3_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_des3_ctx *c;

	if ((c = (struct ssh_des3_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_des3_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_des3_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL) {
		DES_set_key((const_DES_cblock *)(key +  0), &c->des3_ctx[0]);
		DES_set_key((const_DES_cblock *)(key +  8), &c->des3_ctx[1]);
		DES_set_key((const_DES_cblock *)(key + 16), &c->des3_ctx[2]);
	}
	if (iv != NULL)
		memcpy(c->des3_counter, iv, DES_BLOCK);
	return (1);
}
const EVP_CIPHER *evp_des3_ctr(void)
{
	static EVP_CIPHER des3_ctr;

	memset(&des3_ctr, 0, sizeof(EVP_CIPHER));
	des3_ctr.nid = NID_undef;
	des3_ctr.block_size = DES_BLOCK;
	des3_ctr.iv_len = DES_BLOCK;
	des3_ctr.key_len = 24;
	des3_ctr.init = ssh_des3_init;
	des3_ctr.cleanup = ssh_cipher_cleanup;
	des3_ctr.do_cipher = ssh_cipher_ctr;
	des3_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&des3_ctr);
}

//////////////////////////////////////////////////////////////////////

#ifdef	USE_NETTLE
struct ssh_twofish_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	struct twofish_ctx	twofish_ctx;
	u_char				twofish_counter[TWOFISH_BLOCK_SIZE];
};

static void ssh_twofish_enc(void *ctx, u_char *buf)
{
	struct ssh_twofish_ctx *c = (struct ssh_twofish_ctx *)ctx;

	twofish_encrypt(&c->twofish_ctx, TWOFISH_BLOCK_SIZE, buf, c->twofish_counter);
	ssh_ctr_inc(c->twofish_counter, TWOFISH_BLOCK_SIZE);
}
static int	ssh_twofish_cbc(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	struct ssh_twofish_ctx *c;
	u_char buf[TWOFISH_BLOCK_SIZE];
	u_char tmp[TWOFISH_BLOCK_SIZE];
	register int i;
	register union _BpIp {
		u_char		*bp;
		size_t		*ip;
	} a, b, e;

	if ((c = (struct ssh_twofish_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	if ( EVP_CIPHER_CTX_encrypting(ctx) ) {
		while ( len >= TWOFISH_BLOCK_SIZE ) {
			a.bp = buf;
			b.bp = (u_char *)src;
			e.bp = c->twofish_counter;

			for ( i = TWOFISH_BLOCK_SIZE / sizeof(size_t) ; i > 0 ; i-- )
				*(a.ip++) = *(b.ip++) ^ *(e.ip++);

			// TWOFISH_BLOCK_SIZE % sizeof(size_t) == 0 ?
			//for ( i = TWOFISH_BLOCK_SIZE % sizeof(size_t) ; i > 0 ; i-- )
			//	*(a.bp++) = *(b.bp++) ^ *(e.bp++);

			twofish_encrypt(&c->twofish_ctx, TWOFISH_BLOCK_SIZE, c->twofish_counter, buf);

			memcpy(dest, c->twofish_counter, TWOFISH_BLOCK_SIZE);

			src  += TWOFISH_BLOCK_SIZE;
			dest += TWOFISH_BLOCK_SIZE;
			len  -= TWOFISH_BLOCK_SIZE;
		}

		if ( len > 0 ) {
			for ( i = 0 ; i < (int)len ; i++ )
				buf[i] = *(src++) ^ c->twofish_counter[i];

			for ( ; i < TWOFISH_BLOCK_SIZE ; i++ )
				buf[i] = c->twofish_counter[i];

			twofish_encrypt(&c->twofish_ctx, TWOFISH_BLOCK_SIZE, c->twofish_counter, buf);

			for ( i = 0 ; i < (int)len ; i++ )
				*(dest++) = c->twofish_counter[i];
		}

	} else {
		while ( len >= TWOFISH_BLOCK_SIZE ) {
			twofish_decrypt(&c->twofish_ctx, TWOFISH_BLOCK_SIZE, buf, src);

			a.bp = dest;
			b.bp = buf;
			e.bp = c->twofish_counter;

			for ( i = TWOFISH_BLOCK_SIZE / sizeof(size_t) ; i > 0 ; i-- )
				*(a.ip++) = *(b.ip++) ^ *(e.ip++);

			// TWOFISH_BLOCK_SIZE % sizeof(size_t) == 0 ?
			//for ( i = TWOFISH_BLOCK_SIZE % sizeof(size_t) ; i > 0 ; i-- )
			//	*(a.bp++) = *(b.bp++) ^ *(e.bp++);

			memcpy(c->twofish_counter, src, TWOFISH_BLOCK_SIZE);

			src  += TWOFISH_BLOCK_SIZE;
			dest += TWOFISH_BLOCK_SIZE;
			len  -= TWOFISH_BLOCK_SIZE;
		}

		if ( len > 0 ) {
			for ( i = 0 ; i < (int)len ; i++ )
				tmp[i] = *(src++);

			for ( ; i < TWOFISH_BLOCK_SIZE ; i++ )
				tmp[i] = c->twofish_counter[i];

			twofish_decrypt(&c->twofish_ctx, TWOFISH_BLOCK_SIZE, buf, tmp);

			for ( i = 0 ; i < (int)len ; i++ ) {
				*(dest++) = buf[i] ^ c->twofish_counter[i];
				c->twofish_counter[i] = tmp[i];
			}

			for ( ; i < TWOFISH_BLOCK_SIZE ; i++ )
				c->twofish_counter[i] = buf[i];
		}
	}

	return (1);
}
static int ssh_twofish_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_twofish_ctx *c;

	if ((c = (struct ssh_twofish_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_twofish_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_twofish_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		twofish_set_key(&c->twofish_ctx, EVP_CIPHER_CTX_key_length(ctx), key);
	if (iv != NULL)
		memcpy(c->twofish_counter, iv, TWOFISH_BLOCK_SIZE);
	return (1);
}
const EVP_CIPHER *evp_twofish_ctr(void)
{
	static EVP_CIPHER twofish_ctr;

	memset(&twofish_ctr, 0, sizeof(EVP_CIPHER));
	twofish_ctr.nid = NID_undef;
	twofish_ctr.block_size = TWOFISH_BLOCK_SIZE;
	twofish_ctr.iv_len = TWOFISH_BLOCK_SIZE;
	twofish_ctr.key_len = 32;
	twofish_ctr.init = ssh_twofish_init;
	twofish_ctr.cleanup = ssh_cipher_cleanup;
	twofish_ctr.do_cipher = ssh_cipher_ctr;
	twofish_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&twofish_ctr);
}
const EVP_CIPHER *evp_twofish_cbc(void)
{
	static EVP_CIPHER twofish_cbc;

	memset(&twofish_cbc, 0, sizeof(EVP_CIPHER));
	twofish_cbc.nid = NID_undef;
	twofish_cbc.block_size = TWOFISH_BLOCK_SIZE;
	twofish_cbc.iv_len = TWOFISH_BLOCK_SIZE;
	twofish_cbc.key_len = 32;
	twofish_cbc.init = ssh_twofish_init;
	twofish_cbc.cleanup = ssh_cipher_cleanup;
	twofish_cbc.do_cipher = ssh_twofish_cbc;
	twofish_cbc.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&twofish_cbc);
}

//////////////////////////////////////////////////////////////////////

struct ssh_serpent_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	struct serpent_ctx	serpent_ctx;
	u_char				serpent_counter[SERPENT_BLOCK_SIZE];
};

static void ssh_serpent_enc(void *ctx, u_char *buf)
{
	struct ssh_serpent_ctx *c = (struct ssh_serpent_ctx *)ctx;

	serpent_encrypt(&c->serpent_ctx, SERPENT_BLOCK_SIZE, buf, c->serpent_counter);
	ssh_ctr_inc(c->serpent_counter, SERPENT_BLOCK_SIZE);
}
static int	ssh_serpent_cbc(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	u_int n, i;
	struct ssh_serpent_ctx *c;
	u_char buf[SERPENT_BLOCK_SIZE];
	u_char tmp[SERPENT_BLOCK_SIZE];

	if (len == 0)
		return (1);
	if ((c = (struct ssh_serpent_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	if ( EVP_CIPHER_CTX_encrypting(ctx) ) {
		while ( len > 0 ) {
			n = (int)(len > SERPENT_BLOCK_SIZE ? SERPENT_BLOCK_SIZE : len);

			for ( i = 0 ; i < n ; i++ )
				buf[i] = *(src++) ^ c->serpent_counter[i];

			for ( ; i < SERPENT_BLOCK_SIZE ; i++ )
				buf[i] = c->serpent_counter[i];

			serpent_encrypt(&c->serpent_ctx, SERPENT_BLOCK_SIZE, c->serpent_counter, buf);

			for ( i = 0 ; i < n ; i++ )
				*(dest++) = c->serpent_counter[i];

			len -= n;
		}

	} else {
		while ( len > 0 ) {
			n = (int)(len > SERPENT_BLOCK_SIZE ? SERPENT_BLOCK_SIZE : len);

			for ( i = 0 ; i < n ; i++ )
				tmp[i] = *(src++);

			for ( ; i < SERPENT_BLOCK_SIZE ; i++ )
				tmp[i] = c->serpent_counter[i];

			serpent_decrypt(&c->serpent_ctx, SERPENT_BLOCK_SIZE, buf, tmp);

			for ( i = 0 ; i < n ; i++ ) {
				*(dest++) = buf[i] ^ c->serpent_counter[i];
				c->serpent_counter[i] = tmp[i];
			}

			for ( ; i < SERPENT_BLOCK_SIZE ; i++ )
				c->serpent_counter[i] = buf[i];

			len -= n;
		}
	}

	return (1);
}
static int ssh_serpent_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_serpent_ctx *c;

	if ((c = (struct ssh_serpent_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_serpent_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_serpent_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		serpent_set_key(&c->serpent_ctx, EVP_CIPHER_CTX_key_length(ctx), key);
	if (iv != NULL)
		memcpy(c->serpent_counter, iv, SERPENT_BLOCK_SIZE);
	return (1);
}
const EVP_CIPHER *evp_serpent_ctr(void)
{
	static EVP_CIPHER serpent_ctr;

	memset(&serpent_ctr, 0, sizeof(EVP_CIPHER));
	serpent_ctr.nid = NID_undef;
	serpent_ctr.block_size = SERPENT_BLOCK_SIZE;
	serpent_ctr.iv_len = SERPENT_BLOCK_SIZE;
	serpent_ctr.key_len = 32;
	serpent_ctr.init = ssh_serpent_init;
	serpent_ctr.cleanup = ssh_cipher_cleanup;
	serpent_ctr.do_cipher = ssh_cipher_ctr;
	serpent_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&serpent_ctr);
}
const EVP_CIPHER *evp_serpent_cbc(void)
{
	static EVP_CIPHER serpent_cbc;

	memset(&serpent_cbc, 0, sizeof(EVP_CIPHER));
	serpent_cbc.nid = NID_undef;
	serpent_cbc.block_size = SERPENT_BLOCK_SIZE;
	serpent_cbc.iv_len = SERPENT_BLOCK_SIZE;
	serpent_cbc.key_len = 32;
	serpent_cbc.init = ssh_serpent_init;
	serpent_cbc.cleanup = ssh_cipher_cleanup;
	serpent_cbc.do_cipher = ssh_serpent_cbc;
	serpent_cbc.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&serpent_cbc);
}

#endif	// USE_NETTLE

//////////////////////////////////////////////////////////////////////

#ifdef	USE_CLEFIA

struct ssh_clefia_ctx
{
	struct mt_ctr_ctx	*mt_ctx;		// dont move !!
	struct clefia_ctx	clefia_ctx;
	u_char				clefia_counter[CLEFIA_BLOCK_SIZE];
};

static void ssh_clefia_enc(void *ctx, u_char *buf)
{
	struct ssh_clefia_ctx *c = (struct ssh_clefia_ctx *)ctx;

	ClefiaEncrypt(&c->clefia_ctx, buf, c->clefia_counter);
	ssh_ctr_inc(c->clefia_counter, CLEFIA_BLOCK_SIZE);
}
static int	ssh_clefia_cbc(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	u_int n, i;
	struct ssh_clefia_ctx *c;
	u_char buf[CLEFIA_BLOCK_SIZE];
	u_char tmp[CLEFIA_BLOCK_SIZE];

	if (len == 0)
		return (1);
	if ((c = (struct ssh_clefia_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	if ( ctx->encrypt ) {
		while ( len > 0 ) {
			n = (len > CLEFIA_BLOCK_SIZE ? CLEFIA_BLOCK_SIZE : len);

			for ( i = 0 ; i < n ; i++ )
				buf[i] = *(src++) ^ c->clefia_counter[i];

			for ( ; i < CLEFIA_BLOCK_SIZE ; i++ )
				buf[i] = c->clefia_counter[i];

			ClefiaEncrypt(&c->clefia_ctx, c->clefia_counter, buf);

			for ( i = 0 ; i < n ; i++ )
				*(dest++) = c->clefia_counter[i];

			len -= n;
		}

	} else {
		while ( len > 0 ) {
			n = (len > CLEFIA_BLOCK_SIZE ? CLEFIA_BLOCK_SIZE : len);

			for ( i = 0 ; i < n ; i++ )
				tmp[i] = *(src++);

			for ( ; i < CLEFIA_BLOCK_SIZE ; i++ )
				tmp[i] = c->clefia_counter[i];

			ClefiaDecrypt(&c->clefia_ctx, buf, tmp);

			for ( i = 0 ; i < n ; i++ ) {
				*(dest++) = buf[i] ^ c->clefia_counter[i];
				c->clefia_counter[i] = tmp[i];
			}

			for ( ; i < CLEFIA_BLOCK_SIZE ; i++ )
				c->clefia_counter[i] = buf[i];

			len -= n;
		}
	}

	return (1);
}
static int ssh_clefia_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_clefia_ctx *c;

	if ((c = (struct ssh_clefia_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_clefia_ctx *)malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		if ( EVP_CIPHER_CTX_cipher(ctx)->do_cipher == ssh_cipher_ctr )
			c->mt_ctx = mt_init((void *)c, ssh_clefia_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		ClefiaKeySet(&c->clefia_ctx, key, EVP_CIPHER_CTX_key_length(ctx) * 8);
	if (iv != NULL)
		memcpy(c->clefia_counter, iv, CLEFIA_BLOCK_SIZE);
	return (1);
}
const EVP_CIPHER *evp_clefia_ctr(void)
{
	static EVP_CIPHER clefia_ctr;

	memset(&clefia_ctr, 0, sizeof(EVP_CIPHER));
	clefia_ctr.nid = NID_undef;
	clefia_ctr.block_size = CLEFIA_BLOCK_SIZE;
	clefia_ctr.iv_len = CLEFIA_BLOCK_SIZE;
	clefia_ctr.key_len = 32;
	clefia_ctr.init = ssh_clefia_init;
	clefia_ctr.cleanup = ssh_cipher_cleanup;
	clefia_ctr.do_cipher = ssh_cipher_ctr;
	clefia_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&clefia_ctr);
}
const EVP_CIPHER *evp_clefia_cbc(void)
{
	static EVP_CIPHER clefia_cbc;

	memset(&clefia_cbc, 0, sizeof(EVP_CIPHER));
	clefia_cbc.nid = NID_undef;
	clefia_cbc.block_size = CLEFIA_BLOCK_SIZE;
	clefia_cbc.iv_len = CLEFIA_BLOCK_SIZE;
	clefia_cbc.key_len = 32;
	clefia_cbc.init = ssh_clefia_init;
	clefia_cbc.cleanup = ssh_cipher_cleanup;
	clefia_cbc.do_cipher = ssh_clefia_cbc;
	clefia_cbc.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&clefia_cbc);
}
#endif	// USE_CLEFIA

//////////////////////////////////////////////////////////////////////

struct ssh1_3des_ctx
{
	EVP_CIPHER_CTX	*k1, *k2, *k3;
};

static int ssh1_3des_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh1_3des_ctx *c;
	u_char *k1, *k2, *k3;

	if ( (c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		if ( (c = (struct ssh1_3des_ctx *)malloc(sizeof(*c))) == NULL )
			return (0);
		memset(c, 0, sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}

	if ( key == NULL )
		return (1);

	if ( enc == (-1) )
		enc = EVP_CIPHER_CTX_encrypting(ctx);

	k1 = k2 = k3 = (u_char *)key;
	k2 += 8;

	if ( EVP_CIPHER_CTX_key_length(ctx) >= (16 + 8) ) {
		if ( enc )
			k3 += 16;
		else
			k1 += 16;
	}

	if ( c->k1 == NULL ) c->k1 = EVP_CIPHER_CTX_new();
	if ( c->k2 == NULL ) c->k2 = EVP_CIPHER_CTX_new();
	if ( c->k3 == NULL ) c->k3 = EVP_CIPHER_CTX_new();

	EVP_CIPHER_CTX_init(c->k1);
	EVP_CIPHER_CTX_init(c->k2);
	EVP_CIPHER_CTX_init(c->k3);

	if ( EVP_CipherInit(c->k1, EVP_des_cbc(), k1, NULL, enc) == 0 || EVP_CipherInit(c->k2, EVP_des_cbc(), k2, NULL, !enc) == 0 || EVP_CipherInit(c->k3, EVP_des_cbc(), k3, NULL, enc) == 0 ) {
		EVP_CIPHER_CTX_cleanup(c->k1);
		EVP_CIPHER_CTX_cleanup(c->k2);
		EVP_CIPHER_CTX_cleanup(c->k3);
		EVP_CIPHER_CTX_free(c->k1);
		EVP_CIPHER_CTX_free(c->k2);
		EVP_CIPHER_CTX_free(c->k3);
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
		return (0);
	}

	return (1);
}
static int ssh1_3des_cbc(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	struct ssh1_3des_ctx *c;

	if ( (c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL )
		return (0);

	if ( EVP_Cipher(c->k1, dest, (u_char *)src, (unsigned int)len) == 0 || EVP_Cipher(c->k2, dest, dest, (unsigned int)len) == 0 || EVP_Cipher(c->k3, dest, dest, (unsigned int)len) == 0 )
		return (0);

	return (1);
}
static int ssh1_3des_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh1_3des_ctx *c;

	if ( (c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL ) {
		EVP_CIPHER_CTX_cleanup(c->k1);
		EVP_CIPHER_CTX_cleanup(c->k2);
		EVP_CIPHER_CTX_cleanup(c->k3);
		EVP_CIPHER_CTX_free(c->k1);
		EVP_CIPHER_CTX_free(c->k2);
		EVP_CIPHER_CTX_free(c->k3);
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}
const EVP_CIPHER *evp_ssh1_3des(void)
{
	static EVP_CIPHER ssh1_3des;

	memset(&ssh1_3des, 0, sizeof(EVP_CIPHER));
	ssh1_3des.nid = NID_undef;
	ssh1_3des.block_size = 8;
	ssh1_3des.iv_len = 0;
	ssh1_3des.key_len = 16;
	ssh1_3des.init = ssh1_3des_init;
	ssh1_3des.cleanup = ssh1_3des_cleanup;
	ssh1_3des.do_cipher = ssh1_3des_cbc;
	ssh1_3des.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH;
	return (&ssh1_3des);
}

//////////////////////////////////////////////////////////////////////

static void swap_bytes(const u_char *src, u_char *dst, int n)
{
	u_char c[4];

	/* Process 4 bytes every lap. */
	for (n = n / 4; n > 0; n--) {
		c[3] = *src++;
		c[2] = *src++;
		c[1] = *src++;
		c[0] = *src++;

		*dst++ = c[0];
		*dst++ = c[1];
		*dst++ = c[2];
		*dst++ = c[3];
	}
}

static int (*orig_bf)(EVP_CIPHER_CTX *, u_char *, const u_char *, size_t) = NULL;

static int bf_ssh1_cipher(EVP_CIPHER_CTX *ctx, u_char *out, const u_char *in, size_t len)
{
	int ret;

	swap_bytes(in, out, (int)len);
	ret = (*orig_bf)(ctx, out, out, len);
	swap_bytes(out, out, (int)len);
	return (ret);
}

const EVP_CIPHER *evp_ssh1_bf(void)
{
	static EVP_CIPHER ssh1_bf;

	memcpy(&ssh1_bf, EVP_bf_cbc(), sizeof(EVP_CIPHER));
	orig_bf = ssh1_bf.do_cipher;
	ssh1_bf.nid = NID_undef;
	ssh1_bf.do_cipher = bf_ssh1_cipher;
	ssh1_bf.key_len = 32;
	return (&ssh1_bf);
}

//////////////////////////////////////////////////////////////////////

void rsa_public_encrypt(BIGNUM *out, BIGNUM *in, RSA *key)
{
    BYTE *inbuf, *outbuf;
    int len, ilen, olen;

    olen = RSA_size(key);
    outbuf = (BYTE *)malloc(olen);

    ilen = BN_num_bytes(in);
    inbuf = (BYTE *)malloc(ilen);
    BN_bn2bin(in, inbuf);

    len = RSA_public_encrypt(ilen, inbuf, outbuf, key, RSA_PKCS1_PADDING);

    BN_bin2bn(outbuf, len, out);

    free(outbuf);
    free(inbuf);
}

int	ssh_crc32(LPBYTE buf, int len)
{
	int i;
	unsigned long crc;

	crc = 0;
	for ( i = 0 ;  i < len ;  i++ )
		crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
	return (int)crc;
}

int dh_pub_is_valid(DH *dh, const BIGNUM *dh_pub)
{
	int n;
	int bits_len = BN_num_bits(dh_pub);
	int bits_set = 0;
	BIGNUM const *p = NULL;

	if ( BN_is_negative(dh_pub) )	// dh_pub->neg )
		return 0;

	for ( n = 0 ; n <= bits_len ; n++ ) {
		if ( BN_is_bit_set(dh_pub, n) )
			bits_set++;
	}

	DH_get0_pqg(dh, &p, NULL, NULL);

	return (bits_set > 1 && p != NULL && BN_cmp(dh_pub, p) == -1 ? 1 : 0);
}
int dh_gen_key(DH *dh, int need)
{
	int pbits;
	BIGNUM const *pub_key = NULL;

	if ( need < 0 || (pbits = DH_bits(dh)) <= 0 || need > INT_MAX / 2 || 2 * need > pbits )
		return 1;

	if ( need < 256 )
		need = 256;

	if ( (need * 2) < (pbits - 1) )
		DH_set_length(dh, need * 2);
	else
		DH_set_length(dh, pbits - 1);

	if ( DH_generate_key(dh) == 0 )
		return 1;

	DH_get0_key(dh, &pub_key, NULL);

	if ( !dh_pub_is_valid(dh, pub_key) )
		return 1;

	return 0;
}
DH *dh_new_group_asc(const char *gen, const char *modulus)
{
	DH *dh;
	BIGNUM *p, *g;

    if ((dh = DH_new()) == NULL)
		return NULL;

	if ( (p = BN_new()) == NULL || BN_hex2bn(&p, modulus) == 0)
		return NULL;

	if ( (g = BN_new()) == NULL || BN_hex2bn(&g, gen) == 0 )
		return NULL;

	DH_set0_pqg(dh, p, NULL, g);

	return (dh);
}
DH *dh_new_group1(void)
{
    static const char *gen = "2", *group1 =
        "FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
        "29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
        "EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
        "E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
        "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE65381"
        "FFFFFFFF" "FFFFFFFF";

    return (dh_new_group_asc(gen, group1));
}
DH *dh_new_group14(void)
{
	static const char *gen = "2", *group14 =
	    "FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
	    "29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
	    "EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
	    "E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
	    "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
	    "C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
	    "83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
	    "670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
	    "E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
	    "DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
	    "15728E5A" "8AACAA68" "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group14));
}
DH *dh_new_group15(void)
{
	static const char *gen = "2", *group15 =
		"FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
		"29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
		"EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
		"E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
		"EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
		"C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
		"83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
		"670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
		"E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
		"DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
		"15728E5A" "8AAAC42D" "AD33170D" "04507A33" "A85521AB" "DF1CBA64"
		"ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D" "B3970F85" "A6E1E4C7"
		"ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226" "1AD2EE6B"
		"F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
		"BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31"
		"43DB5BFC" "E0FD108E" "4B82D120" "A93AD2CA" "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group15));
}
DH *dh_new_group16(void)
{
	static const char *gen = "2", *group16 =
	    "FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
	    "29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
	    "EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
	    "E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
	    "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
	    "C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
	    "83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
	    "670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
	    "E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
	    "DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
	    "15728E5A" "8AAAC42D" "AD33170D" "04507A33" "A85521AB" "DF1CBA64"
	    "ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D" "B3970F85" "A6E1E4C7"
	    "ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226" "1AD2EE6B"
	    "F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
	    "BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31"
	    "43DB5BFC" "E0FD108E" "4B82D120" "A9210801" "1A723C12" "A787E6D7"
	    "88719A10" "BDBA5B26" "99C32718" "6AF4E23C" "1A946834" "B6150BDA"
	    "2583E9CA" "2AD44CE8" "DBBBC2DB" "04DE8EF9" "2E8EFC14" "1FBECAA6"
	    "287C5947" "4E6BC05D" "99B2964F" "A090C3A2" "233BA186" "515BE7ED"
	    "1F612970" "CEE2D7AF" "B81BDD76" "2170481C" "D0069127" "D5B05AA9"
	    "93B4EA98" "8D8FDDC1" "86FFB7DC" "90A6C08F" "4DF435C9" "34063199"
	    "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group16));
}
DH *dh_new_group17(void)
{
	static const char *gen = "2", *group17 =
		"FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
		"29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
		"EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
		"E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
		"EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
		"C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
		"83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
		"670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
		"E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
		"DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
		"15728E5A" "8AAAC42D" "AD33170D" "04507A33" "A85521AB" "DF1CBA64"
		"ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D" "B3970F85" "A6E1E4C7"
		"ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226" "1AD2EE6B"
		"F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
		"BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31"
		"43DB5BFC" "E0FD108E" "4B82D120" "A9210801" "1A723C12" "A787E6D7"
		"88719A10" "BDBA5B26" "99C32718" "6AF4E23C" "1A946834" "B6150BDA"
		"2583E9CA" "2AD44CE8" "DBBBC2DB" "04DE8EF9" "2E8EFC14" "1FBECAA6"
		"287C5947" "4E6BC05D" "99B2964F" "A090C3A2" "233BA186" "515BE7ED"
		"1F612970" "CEE2D7AF" "B81BDD76" "2170481C" "D0069127" "D5B05AA9"
		"93B4EA98" "8D8FDDC1" "86FFB7DC" "90A6C08F" "4DF435C9" "34028492"
		"36C3FAB4" "D27C7026" "C1D4DCB2" "602646DE" "C9751E76" "3DBA37BD"
		"F8FF9406" "AD9E530E" "E5DB382F" "413001AE" "B06A53ED" "9027D831"
		"179727B0" "865A8918" "DA3EDBEB" "CF9B14ED" "44CE6CBA" "CED4BB1B"
		"DB7F1447" "E6CC254B" "33205151" "2BD7AF42" "6FB8F401" "378CD2BF"
		"5983CA01" "C64B92EC" "F032EA15" "D1721D03" "F482D7CE" "6E74FEF6"
		"D55E702F" "46980C82" "B5A84031" "900B1C9E" "59E7C97F" "BEC7E8F3"
		"23A97A7E" "36CC88BE" "0F1D45B7" "FF585AC5" "4BD407B2" "2B4154AA"
		"CC8F6D7E" "BF48E1D8" "14CC5ED2" "0F8037E0" "A79715EE" "F29BE328"
		"06A1D58B" "B7C5DA76" "F550AA3D" "8A1FBFF0" "EB19CCB1" "A313D55C"
		"DA56C9EC" "2EF29632" "387FE8D7" "6E3C0468" "043E8F66" "3F4860EE"
		"12BF2D5B" "0B7474D6" "E694F91E" "6DCC4024" "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group17));
}
DH *dh_new_group18(void)
{
	static const char *gen = "2", *group18 =
		"FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
		"29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
		"EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
		"E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
		"EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
		"C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
		"83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
		"670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
		"E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
		"DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
		"15728E5A" "8AAAC42D" "AD33170D" "04507A33" "A85521AB" "DF1CBA64"
		"ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D" "B3970F85" "A6E1E4C7"
		"ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226" "1AD2EE6B"
		"F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
		"BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31"
		"43DB5BFC" "E0FD108E" "4B82D120" "A9210801" "1A723C12" "A787E6D7"
		"88719A10" "BDBA5B26" "99C32718" "6AF4E23C" "1A946834" "B6150BDA"
		"2583E9CA" "2AD44CE8" "DBBBC2DB" "04DE8EF9" "2E8EFC14" "1FBECAA6"
		"287C5947" "4E6BC05D" "99B2964F" "A090C3A2" "233BA186" "515BE7ED"
		"1F612970" "CEE2D7AF" "B81BDD76" "2170481C" "D0069127" "D5B05AA9"
		"93B4EA98" "8D8FDDC1" "86FFB7DC" "90A6C08F" "4DF435C9" "34028492"
		"36C3FAB4" "D27C7026" "C1D4DCB2" "602646DE" "C9751E76" "3DBA37BD"
		"F8FF9406" "AD9E530E" "E5DB382F" "413001AE" "B06A53ED" "9027D831"
		"179727B0" "865A8918" "DA3EDBEB" "CF9B14ED" "44CE6CBA" "CED4BB1B"
		"DB7F1447" "E6CC254B" "33205151" "2BD7AF42" "6FB8F401" "378CD2BF"
		"5983CA01" "C64B92EC" "F032EA15" "D1721D03" "F482D7CE" "6E74FEF6"
		"D55E702F" "46980C82" "B5A84031" "900B1C9E" "59E7C97F" "BEC7E8F3"
		"23A97A7E" "36CC88BE" "0F1D45B7" "FF585AC5" "4BD407B2" "2B4154AA"
		"CC8F6D7E" "BF48E1D8" "14CC5ED2" "0F8037E0" "A79715EE" "F29BE328"
		"06A1D58B" "B7C5DA76" "F550AA3D" "8A1FBFF0" "EB19CCB1" "A313D55C"
		"DA56C9EC" "2EF29632" "387FE8D7" "6E3C0468" "043E8F66" "3F4860EE"
		"12BF2D5B" "0B7474D6" "E694F91E" "6DBE1159" "74A3926F" "12FEE5E4"
		"38777CB6" "A932DF8C" "D8BEC4D0" "73B931BA" "3BC832B6" "8D9DD300"
		"741FA7BF" "8AFC47ED" "2576F693" "6BA42466" "3AAB639C" "5AE4F568"
		"3423B474" "2BF1C978" "238F16CB" "E39D652D" "E3FDB8BE" "FC848AD9"
		"22222E04" "A4037C07" "13EB57A8" "1A23F0C7" "3473FC64" "6CEA306B"
		"4BCBC886" "2F8385DD" "FA9D4B7F" "A2C087E8" "79683303" "ED5BDD3A"
		"062B3CF5" "B3A278A6" "6D2A13F8" "3F44F82D" "DF310EE0" "74AB6A36"
		"4597E899" "A0255DC1" "64F31CC5" "0846851D" "F9AB4819" "5DED7EA1"
		"B1D510BD" "7EE74D73" "FAF36BC3" "1ECFA268" "359046F4" "EB879F92"
		"4009438B" "481C6CD7" "889A002E" "D5EE382B" "C9190DA6" "FC026E47"
		"9558E447" "5677E9AA" "9E3050E2" "765694DF" "C81F56E8" "80B96E71"
		"60C980DD" "98EDD3DF" "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group18));
}
int	dh_estimate(int bits)
{
	if ( bits <= 112 )
		return 2048;
	else if ( bits <= 128 )
		return 3072;
	else if ( bits <= 192 )
		return 7680;
	else
		return 8192;
}
int key_ec_validate_public(const EC_GROUP *group, const EC_POINT *pub)
{
	BN_CTX *bnctx = NULL;
	EC_POINT *nq = NULL;
	BIGNUM *order, *x, *y, *tmp;
	int ret = -1;

	if ( (bnctx = BN_CTX_new()) == NULL )
		return (-1);
	BN_CTX_start(bnctx);

	/*
	 * We shouldn't ever hit this case because bignum_get_ecpoint()
	 * refuses to load GF2m points.
	 */
	if ( EC_METHOD_get_field_type(EC_GROUP_method_of(group)) != NID_X9_62_prime_field )
		goto out;

	/* Q != infinity */
	if ( EC_POINT_is_at_infinity(group, pub) )
		goto out;

	if ( (x = BN_CTX_get(bnctx)) == NULL ||
	     (y = BN_CTX_get(bnctx)) == NULL ||
	     (order = BN_CTX_get(bnctx)) == NULL ||
	     (tmp = BN_CTX_get(bnctx)) == NULL)
		goto out;

	/* log2(x) > log2(order)/2, log2(y) > log2(order)/2 */
	if ( EC_GROUP_get_order(group, order, bnctx) != 1 )
		goto out;
	if ( EC_POINT_get_affine_coordinates_GFp(group, pub, x, y, bnctx) != 1 )
		goto out;
	if ( BN_num_bits(x) <= BN_num_bits(order) / 2 )
		goto out;
	if ( BN_num_bits(y) <= BN_num_bits(order) / 2 )
		goto out;

	/* nQ == infinity (n == order of subgroup) */
	if ( (nq = EC_POINT_new(group)) == NULL )
		goto out;
	if ( EC_POINT_mul(group, nq, NULL, pub, order, bnctx) != 1 )
		goto out;
	if ( EC_POINT_is_at_infinity(group, nq) != 1 )
		goto out;

	/* x < order - 1, y < order - 1 */
	if ( !BN_sub(tmp, order, BN_value_one()) )
		goto out;
	if ( BN_cmp(x, tmp) >= 0 )
		goto out;
	if ( BN_cmp(y, tmp) >= 0 )
		goto out;

	ret = 0;
out:
	if ( bnctx != NULL )
		BN_CTX_free(bnctx);
	if ( nq != NULL )
		EC_POINT_free(nq);
	return ret;
}
int kex_gen_hash(
	BYTE *digest,
    LPCSTR client_version_string,
	LPCSTR server_version_string,
    LPBYTE ckexinit, int ckexinitlen,
    LPBYTE skexinit, int skexinitlen,
    LPBYTE sblob, int sbloblen,
	LPBYTE addb, int addblen,
	const EVP_MD *evp_md)
{
	CBuffer b;
	EVP_MD_CTX *md_ctx;

	b.PutStr(client_version_string);
	b.PutStr(server_version_string);

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	b.Put32Bit(ckexinitlen+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)ckexinit, ckexinitlen);

	b.Put32Bit(skexinitlen+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)skexinit, skexinitlen);

	b.PutBuf((LPBYTE)sblob, sbloblen);

	b.Apend(addb, addblen);

	md_ctx = EVP_MD_CTX_new();
	EVP_DigestInit(md_ctx, evp_md);
	EVP_DigestUpdate(md_ctx, b.GetPtr(), b.GetSize());
	EVP_DigestFinal(md_ctx, digest, NULL);
	EVP_MD_CTX_free(md_ctx);

	return EVP_MD_size(evp_md);
}

u_char *derive_key(int id, int need, u_char *hash, int hashlen, u_char *ssec, int sseclen, u_char *session_id, int sesslen, const EVP_MD *evp_md)
{
	EVP_MD_CTX *md_ctx;
	char c = id;
	int have;
	int mdsz = EVP_MD_size(evp_md);
	u_char *digest = (u_char *)malloc(need + (mdsz - (need % mdsz)));

	/* K1 = HASH(K || H || "A" || session_id) */
	md_ctx = EVP_MD_CTX_new();
	EVP_DigestInit(md_ctx, evp_md);
	EVP_DigestUpdate(md_ctx, ssec, sseclen);
	EVP_DigestUpdate(md_ctx, hash, hashlen);
	EVP_DigestUpdate(md_ctx, &c, 1);
	EVP_DigestUpdate(md_ctx, session_id, sesslen);
	EVP_DigestFinal(md_ctx, digest, NULL);
	EVP_MD_CTX_free(md_ctx);

	/*
     * expand key:
     * Kn = HASH(K || H || K1 || K2 || ... || Kn-1)
     * Key = K1 || K2 || ... || Kn
     */
	for ( have = mdsz ; need > have ; have += mdsz ) {
		md_ctx = EVP_MD_CTX_new();
		EVP_DigestInit(md_ctx, evp_md);
		EVP_DigestUpdate(md_ctx, ssec, sseclen);
        EVP_DigestUpdate(md_ctx, hash, hashlen);
		EVP_DigestUpdate(md_ctx, digest, have);
		EVP_DigestFinal(md_ctx, digest + have, NULL);
		EVP_MD_CTX_free(md_ctx);
	}

//#ifdef	_DEBUG
//	TRACE("key %c\n", c);
//	dump_digest(digest, need);
//#endif

	return digest;
}
#ifdef	_DEBUG
static void dump_digest(u_char *digest, int len)
{
	int i;

    for (i = 0; i< len; i++) {
		TRACE("%02x", digest[i]);
		if (i%32 == 31)
			TRACE("\n");
		else if (i%8 == 7)
			TRACE(" ");
	}
	TRACE("\n");
}
#endif

//////////////////////////////////////////////////////////////////////

void *mm_zalloc(void *mm, unsigned int ncount, unsigned int size)
{
	unsigned int len = size * ncount;
	return malloc(len);
}

void mm_zfree(void *mm, void *address)
{
	free(address);
}

//////////////////////////////////////////////////////////////////////

void rand_buf(void *buf, int len)
{
#ifdef	CryptAcquireContext
	// CryptGenRandom cryptographically random
	static BOOL ProvInit = FALSE;
	static HCRYPTPROV hProv = NULL;
#endif

	// RtlGenRandom pseudo-random
	static BOOL Rtlinit = FALSE;
    static HMODULE hAdvApi = NULL;
	static BOOLEAN (__stdcall *RtlGenRandom)(PVOID RandomBuffer, ULONG RandomBufferLength) = NULL;

	static BOOL BCryptInit = FALSE;
    static HMODULE hBCrypt = NULL;
	static NTSTATUS (WINAPI *pBCryptGenRandom)(BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags) = NULL;

	// Free Handle & Library
	if ( buf == NULL && len <= 0 ) {
#ifdef	CryptAcquireContext
		if ( ProvInit && hProv != NULL ) {
			CryptReleaseContext(hProv, 0);
			hProv = NULL;
			ProvInit = FALSE;
		}
#endif

		if ( Rtlinit && hAdvApi != NULL ) {
			FreeLibrary(hAdvApi);
			hAdvApi = NULL;
			RtlGenRandom = NULL;
			Rtlinit = FALSE;
		}

		if ( BCryptInit && hBCrypt != NULL ) {
			FreeLibrary(hBCrypt);
			hBCrypt = NULL;
			pBCryptGenRandom = NULL;
			hBCrypt = FALSE;
		}

		return;
	}

	// BCryptGenRandom(NIST SP800-90 standard) Windows Vista SP1 later
	if ( !BCryptInit ) {
		if ( (hBCrypt = LoadLibrary(_T("Bcrypt.dll"))) != NULL )
			pBCryptGenRandom = (NTSTATUS (WINAPI *)(BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags))GetProcAddress(hBCrypt, "BCryptGenRandom");
		BCryptInit = TRUE;
	}

	if ( pBCryptGenRandom != NULL && pBCryptGenRandom(NULL, (PUCHAR)buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == ((NTSTATUS)0x00000000L) ) // STATUS_SUCCESS
		return;

#ifdef	CryptAcquireContext
	// cryptographically random (This API is deprecated)
	if ( !ProvInit ) {
		if ( !CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0) )
			hProv = NULL;
		ProvInit = TRUE;
	}

	if ( hProv != NULL && CryptGenRandom(hProv, len, (BYTE *)buf) )
		return;
#endif

	// rand_s pseudo-random
	if ( !Rtlinit ) {
		if ( (hAdvApi = LoadLibrary(_T("Advapi32.dll"))) != NULL )
			RtlGenRandom = (BOOLEAN (__stdcall *)(PVOID RandomBuffer, ULONG RandomBufferLength))GetProcAddress(hAdvApi, "SystemFunction036");
		Rtlinit = TRUE;
	}

	if ( RtlGenRandom != NULL && RtlGenRandom(buf, len) )
		return;

	// openssl cryptographically strong pseudo-random
	if ( RAND_bytes((unsigned char *)buf, len) > 0 )
		return;

	// poor random
	unsigned char *p = (unsigned char *)buf;
	for ( int n = 0 ; n < len ; n++ )
		*(p++) = (unsigned char)rand();
}
int rand_int(int n)
{
	// 0 - <n	(max 0x7FFF 32767)
    int r;
    int adjusted_max = (RAND_MAX + 1) - (RAND_MAX + 1) % n;

	do { r = rand(); } while (r >= adjusted_max);

    return (int)(((double)r / adjusted_max) * n);
}
long rand_long(long n)
{
	// 0 - <n	(max 0x7FFFFFFF 2147483647)
    long r;
    long adjusted_max = (long)(0x80000000UL - 0x80000000UL % (unsigned long)n);

	do {
		rand_buf(&r, sizeof(r));
		r &= 0x7FFFFFFFL;
	} while (r >= adjusted_max);

    return (long)(((double)r / adjusted_max) * n);
}
double rand_double()
{
	// 0 - <1.0	(res 1/4503599627370495 2e-16)
    long long r;

	rand_buf(&r, sizeof(r));
	// 52bit	000F FFFF FFFF FFFF = 4503599627370495
	r &= 0x000FFFFFFFFFFFFFLL;

	return (double)r / 4503599627370496.0;
}

//////////////////////////////////////////////////////////////////////
//	umac
//////////////////////////////////////////////////////////////////////

/* -----------------------------------------------------------------------
 * 
 * umac.h -- C Implementation UMAC Message Authentication
 *
 * Version 0.93a of rfc4418.txt -- 2006 July 14
 *
 * For a full description of UMAC message authentication see the UMAC
 * world-wide-web page at http://www.cs.ucdavis.edu/~rogaway/umac
 * Please report bugs and suggestions to the UMAC webpage.
 *
 * Copyright (c) 1999-2004 Ted Krovetz
 *                                                                 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and with or without fee, is hereby
 * granted provided that the above copyright notice appears in all copies
 * and in supporting documentation, and that the name of the copyright
 * holder not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 *
 * Comments should be directed to Ted Krovetz (tdk@acm.org)                                        
 *                                                                   
 * ---------------------------------------------------------------------- */
 
 /* ////////////////////// IMPORTANT NOTES /////////////////////////////////
  *
  * 1) This version does not work properly on messages larger than 16MB
  *
  * 2) If you set the switch to use SSE2, then all data must be 16-byte
  *    aligned
  *
  * 3) When calling the function umac(), it is assumed that msg is in
  * a writable buffer of length divisible by 32 bytes. The message itself
  * does not have to fill the entire buffer, but bytes beyond msg may be
  * zeroed.
  *
  * 4) Two free AES implementations are supported by this implementation of
  * UMAC. Paulo Barreto's version is in the public domain and can be found
  * at http://www.esat.kuleuven.ac.be/~rijmen/rijndael/ (search for
  * "Barreto"). The only two files needed are rijndael-alg-fst.c and
  * rijndael-alg-fst.h.
  * Brian Gladman's version is distributed with GNU Public lisence
  * and can be found at http://fp.gladman.plus.com/AES/index.htm. It
  * includes a fast IA-32 assembly version.
  *
  /////////////////////////////////////////////////////////////////////// */

#define	UMAC_OUTPUT_LEN		16	   /* Alowable: 4, 8, 12, 16              */
#define UMAC_KEY_LEN		16	   /* UMAC takes 16 bytes of external key */

#define STREAMS (UMAC_OUTPUT_LEN/4)	/* Number of times hash is applied    */
#define L1_KEY_LEN         1024     /* Internal key bytes                 */
#define L1_KEY_SHIFT         16     /* Toeplitz key shift between streams */
#define L1_PAD_BOUNDARY      32     /* pad message to boundary multiple   */
#define ALLOC_BOUNDARY       16     /* Keep buffers aligned to this       */
#define HASH_BUF_BYTES       64     /* nh_aux_hb buffer multiple          */

#define p36    ((UINT64)0x0000000FFFFFFFFBull)               /* 2^36 -  5 */
#define p64    ((UINT64)0xFFFFFFFFFFFFFFC5ull)               /* 2^64 - 59 */
#define m36    ((UINT64)0x0000000FFFFFFFFFull)   /* The low 36 of 64 bits */

struct nh_ctx {
	UINT8  nh_key [L1_KEY_LEN + L1_KEY_SHIFT * (STREAMS - 1)]; /* NH Key */
	UINT8  data   [HASH_BUF_BYTES];    /* Incomming data buffer           */
	int next_data_empty;    /* Bookeeping variable for data buffer.       */
	int bytes_hashed;        /* Bytes (out of L1_KEY_LEN) incorperated.   */
	UINT64 state[STREAMS];               /* on-line state     */
	int block_size;
};

struct uhash_ctx {
	struct nh_ctx hash;			          /* Hash context for L1 NH hash  */
	UINT64 poly_key_8[STREAMS];           /* p64 poly keys                */
	UINT64 poly_accum[STREAMS];           /* poly hash result             */
	UINT64 ip_keys[STREAMS * 4];          /* Inner-product keys           */
	UINT32 ip_trans[STREAMS];             /* Inner-product translation    */
	UINT32 msg_len;                       /* Total length of data passed  */
	int streams;						  /* block_size / 4               */
};

struct umac_ctx {
	struct uhash_ctx hash;		/* Hash function for message compression  */
    UINT8 cache[AES_BLOCK_SIZE];	/* Previous AES output is saved       */
    UINT8 nonce[AES_BLOCK_SIZE];	/* The AES input making above cache   */
    AES_KEY	prf_key;				/* Expanded AES key for PDF           */
};

#define	endian_big_64(c)	((((UINT64)(((UINT8 *)(c))[0])) << 56) | \
							 (((UINT64)(((UINT8 *)(c))[1])) << 48) | \
							 (((UINT64)(((UINT8 *)(c))[2])) << 40) | \
							 (((UINT64)(((UINT8 *)(c))[3])) << 32) | \
							 (((UINT64)(((UINT8 *)(c))[4])) << 24) | \
							 (((UINT64)(((UINT8 *)(c))[5])) << 16) | \
							 (((UINT64)(((UINT8 *)(c))[6])) <<  8) | \
							 (((UINT64)(((UINT8 *)(c))[7])) <<  0))

#define	endian_big_32(c)	((((UINT32)(((UINT8 *)(c))[0])) << 24) | \
							 (((UINT32)(((UINT8 *)(c))[1])) << 16) | \
							 (((UINT32)(((UINT8 *)(c))[2])) <<  8) | \
							 (((UINT32)(((UINT8 *)(c))[3])) <<  0))

#define	store_big_32(p,i)	((UINT8 *)(p))[0] = (UINT)((i) >> 24), \
						    ((UINT8 *)(p))[1] = (UINT)((i) >> 16), \
						    ((UINT8 *)(p))[2] = (UINT)((i) >>  8), \
						    ((UINT8 *)(p))[3] = (UINT)((i) >>  0)

#define MUL64(a,b)					((UINT64)((UINT64)(UINT32)(a) * (UINT64)(UINT32)(b)))
#define LOAD_UINT32_LITTLE(ptr)     (*(UINT32 *)(ptr))

//////////////////////////////////////////////////////////////////////

static void kdf(void *bufp, AES_KEY *key, UINT8 ndx, int nbytes)
{
    int i;
    UINT8 in_buf[AES_BLOCK_SIZE];
    UINT8 out_buf[AES_BLOCK_SIZE];
    UINT8 *dst_buf = (UINT8 *)bufp;
    
    /* Setup the initial value */
	memset(in_buf, 0, sizeof(in_buf));
    in_buf[AES_BLOCK_SIZE - 9] = ndx;
    in_buf[AES_BLOCK_SIZE - 1] = i = 1;
        
    while ( nbytes >= AES_BLOCK_SIZE ) {
		AES_encrypt(in_buf, out_buf, key);
        memcpy(dst_buf, out_buf, AES_BLOCK_SIZE);
        in_buf[AES_BLOCK_SIZE - 1] = ++i;
        nbytes  -= AES_BLOCK_SIZE;
        dst_buf += AES_BLOCK_SIZE;
    }
    if ( nbytes ) {
		AES_encrypt(in_buf, out_buf, key);
        memcpy(dst_buf, out_buf, nbytes);
    }
}

//////////////////////////////////////////////////////////////////////

static void nh_aux4(void *kp, void *dp, void *hp, UINT32 dlen)
{
    UINT64 h;
    UINT c = dlen / 32;
    UINT32 *k = (UINT32 *)kp;
    UINT32 *d = (UINT32 *)dp;
    UINT32 d0,d1,d2,d3,d4,d5,d6,d7;
    UINT32 k0,k1,k2,k3,k4,k5,k6,k7;
    
    h = *((UINT64 *)hp);
    do {
        d0 = LOAD_UINT32_LITTLE(d+0); d1 = LOAD_UINT32_LITTLE(d+1);
        d2 = LOAD_UINT32_LITTLE(d+2); d3 = LOAD_UINT32_LITTLE(d+3);
        d4 = LOAD_UINT32_LITTLE(d+4); d5 = LOAD_UINT32_LITTLE(d+5);
        d6 = LOAD_UINT32_LITTLE(d+6); d7 = LOAD_UINT32_LITTLE(d+7);
        k0 = *(k+0); k1 = *(k+1); k2 = *(k+2); k3 = *(k+3);
        k4 = *(k+4); k5 = *(k+5); k6 = *(k+6); k7 = *(k+7);
        h += MUL64((k0 + d0), (k4 + d4));
        h += MUL64((k1 + d1), (k5 + d5));
        h += MUL64((k2 + d2), (k6 + d6));
        h += MUL64((k3 + d3), (k7 + d7));
        
        d += 8;
        k += 8;
    } while (--c);
  *((UINT64 *)hp) = h;
}
static void nh_aux8(void *kp, void *dp, void *hp, UINT32 dlen)
{
  UINT64 h1,h2;
  UINT c = dlen / 32;
  UINT32 *k = (UINT32 *)kp;
  UINT32 *d = (UINT32 *)dp;
  UINT32 d0,d1,d2,d3,d4,d5,d6,d7;
  UINT32 k0,k1,k2,k3,k4,k5,k6,k7,
        k8,k9,k10,k11;

  h1 = *((UINT64 *)hp);
  h2 = *((UINT64 *)hp + 1);
  k0 = *(k+0); k1 = *(k+1); k2 = *(k+2); k3 = *(k+3);
  do {
    d0 = LOAD_UINT32_LITTLE(d+0); d1 = LOAD_UINT32_LITTLE(d+1);
    d2 = LOAD_UINT32_LITTLE(d+2); d3 = LOAD_UINT32_LITTLE(d+3);
    d4 = LOAD_UINT32_LITTLE(d+4); d5 = LOAD_UINT32_LITTLE(d+5);
    d6 = LOAD_UINT32_LITTLE(d+6); d7 = LOAD_UINT32_LITTLE(d+7);
    k4 = *(k+4); k5 = *(k+5); k6 = *(k+6); k7 = *(k+7);
    k8 = *(k+8); k9 = *(k+9); k10 = *(k+10); k11 = *(k+11);

    h1 += MUL64((k0 + d0), (k4 + d4));
    h2 += MUL64((k4 + d0), (k8 + d4));

    h1 += MUL64((k1 + d1), (k5 + d5));
    h2 += MUL64((k5 + d1), (k9 + d5));

    h1 += MUL64((k2 + d2), (k6 + d6));
    h2 += MUL64((k6 + d2), (k10 + d6));

    h1 += MUL64((k3 + d3), (k7 + d7));
    h2 += MUL64((k7 + d3), (k11 + d7));

    k0 = k8; k1 = k9; k2 = k10; k3 = k11;

    d += 8;
    k += 8;
  } while (--c);
  ((UINT64 *)hp)[0] = h1;
  ((UINT64 *)hp)[1] = h2;
}
static void nh_aux12(void *kp, void *dp, void *hp, UINT32 dlen)
{
    UINT64 h1,h2,h3;
    UINT c = dlen / 32;
    UINT32 *k = (UINT32 *)kp;
    UINT32 *d = (UINT32 *)dp;
    UINT32 d0,d1,d2,d3,d4,d5,d6,d7;
    UINT32 k0,k1,k2,k3,k4,k5,k6,k7,
        k8,k9,k10,k11,k12,k13,k14,k15;
    
    h1 = *((UINT64 *)hp);
    h2 = *((UINT64 *)hp + 1);
    h3 = *((UINT64 *)hp + 2);
    k0 = *(k+0); k1 = *(k+1); k2 = *(k+2); k3 = *(k+3);
    k4 = *(k+4); k5 = *(k+5); k6 = *(k+6); k7 = *(k+7);
    do {
        d0 = LOAD_UINT32_LITTLE(d+0); d1 = LOAD_UINT32_LITTLE(d+1);
        d2 = LOAD_UINT32_LITTLE(d+2); d3 = LOAD_UINT32_LITTLE(d+3);
        d4 = LOAD_UINT32_LITTLE(d+4); d5 = LOAD_UINT32_LITTLE(d+5);
        d6 = LOAD_UINT32_LITTLE(d+6); d7 = LOAD_UINT32_LITTLE(d+7);
        k8 = *(k+8); k9 = *(k+9); k10 = *(k+10); k11 = *(k+11);
        k12 = *(k+12); k13 = *(k+13); k14 = *(k+14); k15 = *(k+15);
        
        h1 += MUL64((k0 + d0), (k4 + d4));
        h2 += MUL64((k4 + d0), (k8 + d4));
        h3 += MUL64((k8 + d0), (k12 + d4));
        
        h1 += MUL64((k1 + d1), (k5 + d5));
        h2 += MUL64((k5 + d1), (k9 + d5));
        h3 += MUL64((k9 + d1), (k13 + d5));
        
        h1 += MUL64((k2 + d2), (k6 + d6));
        h2 += MUL64((k6 + d2), (k10 + d6));
        h3 += MUL64((k10 + d2), (k14 + d6));
        
        h1 += MUL64((k3 + d3), (k7 + d7));
        h2 += MUL64((k7 + d3), (k11 + d7));
        h3 += MUL64((k11 + d3), (k15 + d7));
        
        k0 = k8; k1 = k9; k2 = k10; k3 = k11;
        k4 = k12; k5 = k13; k6 = k14; k7 = k15;
        
        d += 8;
        k += 8;
    } while (--c);
    ((UINT64 *)hp)[0] = h1;
    ((UINT64 *)hp)[1] = h2;
    ((UINT64 *)hp)[2] = h3;
}
static void nh_aux16(void *kp, void *dp, void *hp, UINT32 dlen)
{
    UINT64 h1,h2,h3,h4;
    UINT c = dlen / 32;
    UINT32 *k = (UINT32 *)kp;
    UINT32 *d = (UINT32 *)dp;
    UINT32 d0,d1,d2,d3,d4,d5,d6,d7;
    UINT32 k0,k1,k2,k3,k4,k5,k6,k7,
        k8,k9,k10,k11,k12,k13,k14,k15,
        k16,k17,k18,k19;
    
    h1 = *((UINT64 *)hp);
    h2 = *((UINT64 *)hp + 1);
    h3 = *((UINT64 *)hp + 2);
    h4 = *((UINT64 *)hp + 3);
    k0 = *(k+0); k1 = *(k+1); k2 = *(k+2); k3 = *(k+3);
    k4 = *(k+4); k5 = *(k+5); k6 = *(k+6); k7 = *(k+7);
    do {
        d0 = LOAD_UINT32_LITTLE(d+0); d1 = LOAD_UINT32_LITTLE(d+1);
        d2 = LOAD_UINT32_LITTLE(d+2); d3 = LOAD_UINT32_LITTLE(d+3);
        d4 = LOAD_UINT32_LITTLE(d+4); d5 = LOAD_UINT32_LITTLE(d+5);
        d6 = LOAD_UINT32_LITTLE(d+6); d7 = LOAD_UINT32_LITTLE(d+7);
        k8 = *(k+8); k9 = *(k+9); k10 = *(k+10); k11 = *(k+11);
        k12 = *(k+12); k13 = *(k+13); k14 = *(k+14); k15 = *(k+15);
        k16 = *(k+16); k17 = *(k+17); k18 = *(k+18); k19 = *(k+19);
        
        h1 += MUL64((k0 + d0), (k4 + d4));
        h2 += MUL64((k4 + d0), (k8 + d4));
        h3 += MUL64((k8 + d0), (k12 + d4));
        h4 += MUL64((k12 + d0), (k16 + d4));
        
        h1 += MUL64((k1 + d1), (k5 + d5));
        h2 += MUL64((k5 + d1), (k9 + d5));
        h3 += MUL64((k9 + d1), (k13 + d5));
        h4 += MUL64((k13 + d1), (k17 + d5));
        
        h1 += MUL64((k2 + d2), (k6 + d6));
        h2 += MUL64((k6 + d2), (k10 + d6));
        h3 += MUL64((k10 + d2), (k14 + d6));
        h4 += MUL64((k14 + d2), (k18 + d6));
        
        h1 += MUL64((k3 + d3), (k7 + d7));
        h2 += MUL64((k7 + d3), (k11 + d7));
        h3 += MUL64((k11 + d3), (k15 + d7));
        h4 += MUL64((k15 + d3), (k19 + d7));
        
        k0 = k8; k1 = k9; k2 = k10; k3 = k11;
        k4 = k12; k5 = k13; k6 = k14; k7 = k15;
        k8 = k16; k9 = k17; k10 = k18; k11 = k19;
        
        d += 8;
        k += 8;
    } while (--c);
    ((UINT64 *)hp)[0] = h1;
    ((UINT64 *)hp)[1] = h2;
    ((UINT64 *)hp)[2] = h3;
    ((UINT64 *)hp)[3] = h4;
}

static void nh_transform(struct nh_ctx *hc, UINT8 *buf, UINT32 nbytes)
{
    UINT8 *key;
  
    key = hc->nh_key + hc->bytes_hashed;

	switch(hc->block_size) {
	case 4:
		nh_aux4(key, buf, hc->state, nbytes);
		break;
	case 8:
		nh_aux8(key, buf, hc->state, nbytes);
		break;
	case 12:
		nh_aux12(key, buf, hc->state, nbytes);
		break;
	case 16:
		nh_aux16(key, buf, hc->state, nbytes);
		break;
	}
}
static void nh_reset(struct nh_ctx *hc)
{
    hc->bytes_hashed = 0;
    hc->next_data_empty = 0;
    hc->state[0] = 0;
    hc->state[1] = 0;
    hc->state[2] = 0;
    hc->state[3] = 0;
}
static void nh_init(struct nh_ctx *hc, AES_KEY *prf_key)
{
	int i;
	int n = L1_KEY_LEN + L1_KEY_SHIFT * (hc->block_size / 4 - 1);
	UINT8 buf[L1_KEY_LEN + L1_KEY_SHIFT * (STREAMS - 1)];

    kdf(buf, prf_key, 1, n);

    for ( i = 0 ; i < n ; i += 4 )
		*((UINT32 *)(&(hc->nh_key[i]))) = endian_big_32(&(buf[i]));

    nh_reset(hc);
}
static void nh_update(struct nh_ctx *hc, UINT8 *buf, UINT32 nbytes)
{
    UINT32 i,j;
    
    j = hc->next_data_empty;
    if ((j + nbytes) >= HASH_BUF_BYTES) {
        if (j) {
            i = HASH_BUF_BYTES - j;
            memcpy(hc->data+j, buf, i);
            nh_transform(hc,hc->data,HASH_BUF_BYTES);
            nbytes -= i;
            buf += i;
            hc->bytes_hashed += HASH_BUF_BYTES;
        }
        if (nbytes >= HASH_BUF_BYTES) {
            i = nbytes & ~(HASH_BUF_BYTES - 1);
            nh_transform(hc, buf, i);
            nbytes -= i;
            buf += i;
            hc->bytes_hashed += i;
        }
        j = 0;
    }
    memcpy(hc->data + j, buf, nbytes);
    hc->next_data_empty = j + nbytes;
}
static void zero_pad(UINT8 *p, int nbytes)
{
    if ( nbytes >= (int)sizeof(UINT) ) {
        while ((ptrdiff_t)p % sizeof(UINT)) {
            *p = 0;
            nbytes--;
            p++;
        }
        while (nbytes >= (int)sizeof(UINT)) {
            *(UINT *)p = 0;
            nbytes -= sizeof(UINT);
            p += sizeof(UINT);
        }
    }
    while (nbytes) {
        *p = 0;
        nbytes--;
        p++;
    }
}
static void nh_final(struct nh_ctx *hc, UINT8 *result)
{
    int nh_len, nbits;

    if (hc->next_data_empty != 0) {
        nh_len = ((hc->next_data_empty + (L1_PAD_BOUNDARY - 1)) & ~(L1_PAD_BOUNDARY - 1));
        zero_pad(hc->data + hc->next_data_empty, nh_len - hc->next_data_empty);
        nh_transform(hc, hc->data, nh_len);
        hc->bytes_hashed += hc->next_data_empty;
    } else if (hc->bytes_hashed == 0) {
    	nh_len = L1_PAD_BOUNDARY;
        zero_pad(hc->data, L1_PAD_BOUNDARY);
        nh_transform(hc, hc->data, nh_len);
    }

    nbits = (hc->bytes_hashed << 3);

    ((UINT64 *)result)[0] = ((UINT64 *)hc->state)[0] + nbits;

	if ( hc->block_size >= 8 )
		((UINT64 *)result)[1] = ((UINT64 *)hc->state)[1] + nbits;
	if ( hc->block_size >= 12 )
		((UINT64 *)result)[2] = ((UINT64 *)hc->state)[2] + nbits;
	if ( hc->block_size >= 16 )
		((UINT64 *)result)[3] = ((UINT64 *)hc->state)[3] + nbits;

    nh_reset(hc);
}
static void nh(struct nh_ctx *hc, UINT8 *buf, UINT32 padded_len, UINT32 unpadded_len, UINT8 *result)
{
    UINT32 nbits;
    
    /* Initialize the hash state */
    nbits = (unpadded_len << 3);
    
	switch(hc->block_size) {
	case 4:
	    ((UINT64 *)result)[0] = nbits;
		nh_aux4(hc->nh_key, buf, result, padded_len);
		break;
	case 8:
	    ((UINT64 *)result)[0] = nbits;
		((UINT64 *)result)[1] = nbits;
		nh_aux8(hc->nh_key, buf, result, padded_len);
		break;
	case 12:
	    ((UINT64 *)result)[0] = nbits;
		((UINT64 *)result)[1] = nbits;
		((UINT64 *)result)[2] = nbits;
		nh_aux12(hc->nh_key, buf, result, padded_len);
		break;
	case 16:
	    ((UINT64 *)result)[0] = nbits;
		((UINT64 *)result)[1] = nbits;
		((UINT64 *)result)[2] = nbits;
		((UINT64 *)result)[3] = nbits;
		nh_aux16(hc->nh_key, buf, result, padded_len);
		break;
	}

}

//////////////////////////////////////////////////////////////////////

static UINT64 poly64(UINT64 cur, UINT64 key, UINT64 data)
{
    UINT32 key_hi = (UINT32)(key >> 32),
           key_lo = (UINT32)key,
           cur_hi = (UINT32)(cur >> 32),
           cur_lo = (UINT32)cur,
           x_lo,
           x_hi;
    UINT64 X,T,res;
    
    X =  MUL64(key_hi, cur_lo) + MUL64(cur_hi, key_lo);
    x_lo = (UINT32)X;
    x_hi = (UINT32)(X >> 32);
    
    res = (MUL64(key_hi, cur_hi) + x_hi) * 59 + MUL64(key_lo, cur_lo);
     
    T = ((UINT64)x_lo << 32);
    res += T;
    if (res < T)
        res += 59;

    res += data;
    if (res < data)
        res += 59;

    return res;
}

static void poly_hash(struct uhash_ctx *hc, UINT32 data_in[])
{
    int i;
    UINT64 *data=(UINT64*)data_in;
    
	for ( i = 0 ; i < hc->streams ; i++ ) {
        if ((UINT32)(data[i] >> 32) == 0xfffffffful) {
            hc->poly_accum[i] = poly64(hc->poly_accum[i], hc->poly_key_8[i], p64 - 1);
            hc->poly_accum[i] = poly64(hc->poly_accum[i], hc->poly_key_8[i], (data[i] - 59));
        } else {
            hc->poly_accum[i] = poly64(hc->poly_accum[i], hc->poly_key_8[i], data[i]);
        }
    }
}

//////////////////////////////////////////////////////////////////////

static UINT64 ip_aux(UINT64 t, UINT64 *ipkp, UINT64 data)
{
    t = t + ipkp[0] * (UINT64)(UINT16)(data >> 48);
    t = t + ipkp[1] * (UINT64)(UINT16)(data >> 32);
    t = t + ipkp[2] * (UINT64)(UINT16)(data >> 16);
    t = t + ipkp[3] * (UINT64)(UINT16)(data);
    
    return t;
}
static UINT32 ip_reduce_p36(UINT64 t)
{
    UINT64 ret;
    
    ret = (t & m36) + 5 * (t >> 36);

	if ( ret >= p36 )
        ret -= p36;

    return (UINT32)(ret);
}
static void ip_short(struct uhash_ctx *pc, u_char *nh_res, u_char *res)
{
	int i;
    UINT64 t;
    
	for ( i = 0 ; i < pc->streams ; i++ ) {
		t = ip_aux(0, pc->ip_keys + i * 4, ((UINT64 *)nh_res)[i]);
		store_big_32(res + i * 4, ip_reduce_p36(t) ^ pc->ip_trans[i]);
	}
}
static void ip_long(struct uhash_ctx *pc, u_char *res)
{
    int i;
    UINT64 t;

    for ( i = 0 ; i < pc->streams ; i++ ) {
        if ( pc->poly_accum[i] >= p64 )
			pc->poly_accum[i] -= p64;
		t  = ip_aux(0, pc->ip_keys + (i * 4), pc->poly_accum[i]);
		store_big_32(res + i * 4, ip_reduce_p36(t) ^ pc->ip_trans[i]);
    }
}

//////////////////////////////////////////////////////////////////////

static void uhash_reset(struct uhash_ctx *pc)
{
	int n;

	nh_reset(&pc->hash);
    pc->msg_len = 0;
	for ( n = 0 ; n < pc->streams ; n++ )
	    pc->poly_accum[n] = 1;
}
static void uhash_init(struct uhash_ctx *pc, AES_KEY *prf_key)
{
    int i;
    UINT8 buf[(STREAMS * 8 + 4) * sizeof(UINT64)];
    
    /* Initialize the L1 hash */
	nh_init(&pc->hash, prf_key);
    
    /* Setup L2 hash variables */
	kdf(buf, prf_key, 2, (pc->streams * 8 + 4) * sizeof(UINT64));    /* Fill buffer with index 1 key */

    for ( i = 0 ; i < pc->streams ; i++ ) {
		pc->poly_key_8[i] = endian_big_64(&(buf[24 * i]));
        /* Mask the 64-bit keys to their special domain */
        pc->poly_key_8[i] &= ((UINT64)0x01ffffffu << 32) + 0x01ffffffu;
        pc->poly_accum[i] = 1;  /* Our polyhash prepends a non-zero word */
    }
    
    /* Setup L3-1 hash variables */
	kdf(buf, prf_key, 3,  (pc->streams * 8 + 4) * sizeof(UINT64)); /* Fill buffer with index 2 key */

	for ( i = 0 ; i < pc->streams ; i++ ) {
		pc->ip_keys[4 * i + 0] = endian_big_64(&(buf[(8 * i + 4 + 0) * sizeof(UINT64)]));
		pc->ip_keys[4 * i + 1] = endian_big_64(&(buf[(8 * i + 4 + 1) * sizeof(UINT64)]));
		pc->ip_keys[4 * i + 2] = endian_big_64(&(buf[(8 * i + 4 + 2) * sizeof(UINT64)]));
		pc->ip_keys[4 * i + 3] = endian_big_64(&(buf[(8 * i + 4 + 3) * sizeof(UINT64)]));
	}

    for ( i = 0 ; i < (pc->streams * 4) ; i++ )
		pc->ip_keys[i] %= p36;  /* Bring into Z_p36 */
    
    /* Setup L3-2 hash variables    */
    /* Fill buffer with index 4 key */
	kdf(buf, prf_key, 4, pc->streams * sizeof(UINT32));

    for ( i = 0 ; i < pc->streams ; i++ )
		pc->ip_trans[i] = endian_big_32(&(buf[i * sizeof(UINT32)]));
}
static void uhash_update(struct uhash_ctx *pc, u_char *input, int len)
{
    int bytes_hashed, bytes_remaining;
    UINT64 result_buf[STREAMS];
    
    if ( (pc->msg_len + len) <= L1_KEY_LEN ) {
        nh_update(&pc->hash, input, len);
        pc->msg_len += len;

    } else {
         bytes_hashed = pc->msg_len % L1_KEY_LEN;
         if ( pc->msg_len == L1_KEY_LEN )
             bytes_hashed = L1_KEY_LEN;

         if ( (bytes_hashed + len) >= L1_KEY_LEN ) {

             if ( bytes_hashed ) {
                 bytes_remaining = (L1_KEY_LEN - bytes_hashed);
                 nh_update(&pc->hash, input, bytes_remaining);
                 nh_final(&pc->hash, (UINT8 *)result_buf);
                 pc->msg_len += bytes_remaining;
                 poly_hash(pc, (UINT32 *)result_buf);
                 len -= bytes_remaining;
                 input += bytes_remaining;
             }

             while ( len >= L1_KEY_LEN ) {
                 nh(&pc->hash, input, L1_KEY_LEN, L1_KEY_LEN, (UINT8 *)result_buf);
                 pc->msg_len += L1_KEY_LEN;
                 len -= L1_KEY_LEN;
                 input += L1_KEY_LEN;
                 poly_hash(pc, (UINT32 *)result_buf);
             }
         }

         if ( len ) {
             nh_update(&pc->hash, input, len);
             pc->msg_len += len;
         }
     }
}
static void uhash_final(struct uhash_ctx *pc, u_char *res)
{
    UINT64 result_buf[STREAMS];

    if ( pc->msg_len > L1_KEY_LEN ) {
        if ( pc->msg_len % L1_KEY_LEN ) {
            nh_final(&pc->hash, (UINT8 *)result_buf);
            poly_hash(pc, (UINT32 *)result_buf);
        }
        ip_long(pc, res);
    } else {
        nh_final(&pc->hash, (UINT8 *)result_buf);
        ip_short(pc, (UINT8 *)result_buf, res);
    }
    uhash_reset(pc);
}

//////////////////////////////////////////////////////////////////////

struct umac_ctx *UMAC_open(int len)
{
	struct umac_ctx *ctx;
	
	if ( (ctx = (struct umac_ctx *)malloc(sizeof(struct umac_ctx))) == NULL )
		return NULL;

	if      ( len >= 16 )	len = 16;
	else if ( len >= 12 )	len = 12;
	else if ( len >=  8 )	len =  8;
	else					len =  4;

	memset(ctx, 0, sizeof(struct umac_ctx));
	ctx->hash.streams = len / 4;
	ctx->hash.hash.block_size = len;

	return ctx;
}
void UMAC_init(struct umac_ctx *ctx, u_char *key, int len)
{
	AES_KEY prf_key;
    UINT8 buf[UMAC_KEY_LEN];

	AES_set_encrypt_key(key, len * 8, &prf_key);
	uhash_init(&ctx->hash, &prf_key);

    kdf(buf, &prf_key, 0, UMAC_KEY_LEN);
	AES_set_encrypt_key(buf, UMAC_KEY_LEN * 8, &(ctx->prf_key));
    
    memset(ctx->nonce, 0, sizeof(ctx->nonce));
    AES_encrypt(ctx->nonce, ctx->cache, &(ctx->prf_key));
}
void UMAC_update(struct umac_ctx *ctx, const u_char *input, size_t len)
{
	uhash_update(&ctx->hash, (u_char *)input, (int)len);
}
void UMAC_final(struct umac_ctx *ctx, u_char *tag, u_char *nonce)
{
	int mask = 0, ndx = 0;
    UINT8 tmp_nonce_lo[4];

	uhash_final(&ctx->hash, tag);

	switch(ctx->hash.hash.block_size) {
	case 4:
		mask = 3;
		ndx = nonce[7] & mask;
		break;
	case 8:
		mask = 1;
		ndx = nonce[7] & mask;
		break;
	}

	*(UINT32 *)tmp_nonce_lo = ((UINT32 *)nonce)[1];
    tmp_nonce_lo[3] &= ~mask; /* zero last bit */
    
    if ( (((UINT32 *)tmp_nonce_lo)[0] != ((UINT32 *)ctx->nonce)[1]) || (((UINT32 *)nonce)[0] != ((UINT32 *)ctx->nonce)[0]) ) {
        ((UINT32 *)ctx->nonce)[0] = ((UINT32 *)nonce)[0];
        ((UINT32 *)ctx->nonce)[1] = ((UINT32 *)tmp_nonce_lo)[0];
        AES_encrypt(ctx->nonce, ctx->cache, &(ctx->prf_key));
    }
    
	switch(ctx->hash.hash.block_size) {
	case 4:
	    *((UINT32 *)tag) ^= ((UINT32 *)ctx->cache)[ndx];
		break;
	case 8:
	    *((UINT64 *)tag) ^= ((UINT64 *)ctx->cache)[ndx];
		break;
	case 12:
		((UINT64 *)tag)[0] ^= ((UINT64 *)ctx->cache)[0];
		((UINT32 *)tag)[2] ^= ((UINT32 *)ctx->cache)[2];
		break;
	case 16:
		((UINT64 *)tag)[0] ^= ((UINT64 *)ctx->cache)[0];
		((UINT64 *)tag)[1] ^= ((UINT64 *)ctx->cache)[1];
		break;
	}
}
void UMAC_close(struct umac_ctx *ctx)
{
	free(ctx);
}

//////////////////////////////////////////////////////////////////////
//	chacha
//////////////////////////////////////////////////////////////////////

/*
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

struct chacha_ctx {
	u_int input[16];
};

#define CHACHA_MINKEYLEN	16
#define CHACHA_NONCELEN 	8
#define CHACHA_CTRLEN		8
#define CHACHA_STATELEN 	(CHACHA_NONCELEN+CHACHA_CTRLEN)
#define CHACHA_BLOCKLEN 	64

typedef unsigned char u8;
typedef unsigned int u32;

typedef struct chacha_ctx chacha_ctx;

#define U8C(v) (v##U)
#define U32C(v) (v##U)

#define U8V(v) ((u8)(v) & U8C(0xFF))
#define U32V(v) ((u32)(v) & U32C(0xFFFFFFFF))

#define ROTL32(v, n) \
  (U32V((v) << (n)) | ((v) >> (32 - (n))))

#define U8TO32_LITTLE(p) \
  (((u32)((p)[0])      ) | \
   ((u32)((p)[1]) <<  8) | \
   ((u32)((p)[2]) << 16) | \
   ((u32)((p)[3]) << 24))

#define U32TO8_LITTLE(p, v) \
  do { \
    (p)[0] = U8V((v)	  ); \
    (p)[1] = U8V((v) >>  8); \
    (p)[2] = U8V((v) >> 16); \
    (p)[3] = U8V((v) >> 24); \
  } while (0)

#define ROTATE(v,c) (ROTL32(v,c))
#define XOR(v,w) ((v) ^ (w))
#define PLUS(v,w) (U32V((v) + (w)))
#define PLUSONE(v) (PLUS((v),1))

#define QUARTERROUND(a,b,c,d) \
  a = PLUS(a,b); d = ROTATE(XOR(d,a),16); \
  c = PLUS(c,d); b = ROTATE(XOR(b,c),12); \
  a = PLUS(a,b); d = ROTATE(XOR(d,a), 8); \
  c = PLUS(c,d); b = ROTATE(XOR(b,c), 7);

static const char sigma[] = "expand 32-byte k";
static const char tau[]   = "expand 16-byte k";

static void chacha_keysetup(chacha_ctx *x,const u8 *k,u32 kbits)
{
  const char *constants;

  x->input[4] = U8TO32_LITTLE(k + 0);
  x->input[5] = U8TO32_LITTLE(k + 4);
  x->input[6] = U8TO32_LITTLE(k + 8);
  x->input[7] = U8TO32_LITTLE(k + 12);
  if (kbits == 256) { /* recommended */
    k += 16;
    constants = sigma;
  } else { /* kbits == 128 */
    constants = tau;
  }
  x->input[8] = U8TO32_LITTLE(k + 0);
  x->input[9] = U8TO32_LITTLE(k + 4);
  x->input[10] = U8TO32_LITTLE(k + 8);
  x->input[11] = U8TO32_LITTLE(k + 12);
  x->input[0] = U8TO32_LITTLE(constants + 0);
  x->input[1] = U8TO32_LITTLE(constants + 4);
  x->input[2] = U8TO32_LITTLE(constants + 8);
  x->input[3] = U8TO32_LITTLE(constants + 12);
}

static void chacha_ivsetup(chacha_ctx *x, const u8 *iv, const u8 *counter)
{
  x->input[12] = counter == NULL ? 0 : U8TO32_LITTLE(counter + 0);
  x->input[13] = counter == NULL ? 0 : U8TO32_LITTLE(counter + 4);
  x->input[14] = U8TO32_LITTLE(iv + 0);
  x->input[15] = U8TO32_LITTLE(iv + 4);
}

#if	OPENSSL_VERSION_NUMBER >= 0x10100000L

static void chacha_encrypt_bytes(chacha_ctx *x,const u8 *m,u8 *c,u32 bytes)
{
	ChaCha20_ctr32(c, m, bytes, x->input + 4, x->input + 12);
}

#else	// OPENSSL_VERSION_NUMBER >= 0x10100000L

static void chacha_encrypt_bytes(chacha_ctx *x,const u8 *m,u8 *c,u32 bytes)
{
  u32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
  u32 j0, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;
  u8 *ctarget = NULL;
  u8 tmp[64];
  u_int i;

  if (!bytes) return;

  j0 = x->input[0];
  j1 = x->input[1];
  j2 = x->input[2];
  j3 = x->input[3];
  j4 = x->input[4];
  j5 = x->input[5];
  j6 = x->input[6];
  j7 = x->input[7];
  j8 = x->input[8];
  j9 = x->input[9];
  j10 = x->input[10];
  j11 = x->input[11];
  j12 = x->input[12];
  j13 = x->input[13];
  j14 = x->input[14];
  j15 = x->input[15];

  for (;;) {
    if (bytes < 64) {
      for (i = 0;i < bytes;++i) tmp[i] = m[i];
      m = tmp;
      ctarget = c;
      c = tmp;
    }
    x0 = j0;
    x1 = j1;
    x2 = j2;
    x3 = j3;
    x4 = j4;
    x5 = j5;
    x6 = j6;
    x7 = j7;
    x8 = j8;
    x9 = j9;
    x10 = j10;
    x11 = j11;
    x12 = j12;
    x13 = j13;
    x14 = j14;
    x15 = j15;
    for (i = 20;i > 0;i -= 2) {
      QUARTERROUND( x0, x4, x8,x12)
      QUARTERROUND( x1, x5, x9,x13)
      QUARTERROUND( x2, x6,x10,x14)
      QUARTERROUND( x3, x7,x11,x15)
      QUARTERROUND( x0, x5,x10,x15)
      QUARTERROUND( x1, x6,x11,x12)
      QUARTERROUND( x2, x7, x8,x13)
      QUARTERROUND( x3, x4, x9,x14)
    }
    x0 = PLUS(x0,j0);
    x1 = PLUS(x1,j1);
    x2 = PLUS(x2,j2);
    x3 = PLUS(x3,j3);
    x4 = PLUS(x4,j4);
    x5 = PLUS(x5,j5);
    x6 = PLUS(x6,j6);
    x7 = PLUS(x7,j7);
    x8 = PLUS(x8,j8);
    x9 = PLUS(x9,j9);
    x10 = PLUS(x10,j10);
    x11 = PLUS(x11,j11);
    x12 = PLUS(x12,j12);
    x13 = PLUS(x13,j13);
    x14 = PLUS(x14,j14);
    x15 = PLUS(x15,j15);

    x0 = XOR(x0,U8TO32_LITTLE(m + 0));
    x1 = XOR(x1,U8TO32_LITTLE(m + 4));
    x2 = XOR(x2,U8TO32_LITTLE(m + 8));
    x3 = XOR(x3,U8TO32_LITTLE(m + 12));
    x4 = XOR(x4,U8TO32_LITTLE(m + 16));
    x5 = XOR(x5,U8TO32_LITTLE(m + 20));
    x6 = XOR(x6,U8TO32_LITTLE(m + 24));
    x7 = XOR(x7,U8TO32_LITTLE(m + 28));
    x8 = XOR(x8,U8TO32_LITTLE(m + 32));
    x9 = XOR(x9,U8TO32_LITTLE(m + 36));
    x10 = XOR(x10,U8TO32_LITTLE(m + 40));
    x11 = XOR(x11,U8TO32_LITTLE(m + 44));
    x12 = XOR(x12,U8TO32_LITTLE(m + 48));
    x13 = XOR(x13,U8TO32_LITTLE(m + 52));
    x14 = XOR(x14,U8TO32_LITTLE(m + 56));
    x15 = XOR(x15,U8TO32_LITTLE(m + 60));

    j12 = PLUSONE(j12);
    if (!j12) {
      j13 = PLUSONE(j13);
      /* stopping at 2^70 bytes per nonce is user's responsibility */
    }

    U32TO8_LITTLE(c + 0,x0);
    U32TO8_LITTLE(c + 4,x1);
    U32TO8_LITTLE(c + 8,x2);
    U32TO8_LITTLE(c + 12,x3);
    U32TO8_LITTLE(c + 16,x4);
    U32TO8_LITTLE(c + 20,x5);
    U32TO8_LITTLE(c + 24,x6);
    U32TO8_LITTLE(c + 28,x7);
    U32TO8_LITTLE(c + 32,x8);
    U32TO8_LITTLE(c + 36,x9);
    U32TO8_LITTLE(c + 40,x10);
    U32TO8_LITTLE(c + 44,x11);
    U32TO8_LITTLE(c + 48,x12);
    U32TO8_LITTLE(c + 52,x13);
    U32TO8_LITTLE(c + 56,x14);
    U32TO8_LITTLE(c + 60,x15);

    if (bytes <= 64) {
      if (bytes < 64) {
	    for (i = 0;i < bytes;++i) ctarget[i] = c[i];
      }
      x->input[12] = j12;
      x->input[13] = j13;
      return;
    }
    bytes -= 64;
    c += 64;
    m += 64;
  }
}
#endif	//	OPENSSL_VERSION_NUMBER >= 0x10100000L

//////////////////////////////////////////////////////////////////////
//	poly1305
//////////////////////////////////////////////////////////////////////

/* 
 * Public Domain poly1305 from Andrew Moon
 * poly1305-donna-unrolled.c from https://github.com/floodyberry/poly1305-donna
 */

#define POLY1305_KEYLEN 	32
#define POLY1305_TAGLEN 	16

#if	OPENSSL_VERSION_NUMBER < 0x10100000L

#define mul32x32_64(a,b) ((uint64_t)(a) * (b))

#define U8TO32_LE(p) \
	(((uint32_t)((p)[0])) | \
	 ((uint32_t)((p)[1]) <<  8) | \
	 ((uint32_t)((p)[2]) << 16) | \
	 ((uint32_t)((p)[3]) << 24))

#define U32TO8_LE(p, v) \
	do { \
		(p)[0] = (uint8_t)((v)); \
		(p)[1] = (uint8_t)((v) >>  8); \
		(p)[2] = (uint8_t)((v) >> 16); \
		(p)[3] = (uint8_t)((v) >> 24); \
	} while (0)

static void poly1305_auth(unsigned char out[POLY1305_TAGLEN], const unsigned char *m, size_t inlen, const unsigned char key[POLY1305_KEYLEN])
{
	uint32_t t0,t1,t2,t3;
	uint32_t h0,h1,h2,h3,h4;
	uint32_t r0,r1,r2,r3,r4;
	uint32_t s1,s2,s3,s4;
	uint32_t b, nb;
	size_t j;
	uint64_t t[5];
	uint64_t f0,f1,f2,f3;
	uint32_t g0,g1,g2,g3,g4;
	uint64_t c;
	unsigned char mp[16];

	/* clamp key */
	t0 = U8TO32_LE(key+0);
	t1 = U8TO32_LE(key+4);
	t2 = U8TO32_LE(key+8);
	t3 = U8TO32_LE(key+12);

	/* precompute multipliers */
	r0 = t0 & 0x3ffffff; t0 >>= 26; t0 |= t1 << 6;
	r1 = t0 & 0x3ffff03; t1 >>= 20; t1 |= t2 << 12;
	r2 = t1 & 0x3ffc0ff; t2 >>= 14; t2 |= t3 << 18;
	r3 = t2 & 0x3f03fff; t3 >>= 8;
	r4 = t3 & 0x00fffff;

	s1 = r1 * 5;
	s2 = r2 * 5;
	s3 = r3 * 5;
	s4 = r4 * 5;

	/* init state */
	h0 = 0;
	h1 = 0;
	h2 = 0;
	h3 = 0;
	h4 = 0;

	/* full blocks */
	if (inlen < 16) goto poly1305_donna_atmost15bytes;
poly1305_donna_16bytes:
	m += 16;
	inlen -= 16;

	t0 = U8TO32_LE(m-16);
	t1 = U8TO32_LE(m-12);
	t2 = U8TO32_LE(m-8);
	t3 = U8TO32_LE(m-4);

	h0 += t0 & 0x3ffffff;
	h1 += ((((uint64_t)t1 << 32) | t0) >> 26) & 0x3ffffff;
	h2 += ((((uint64_t)t2 << 32) | t1) >> 20) & 0x3ffffff;
	h3 += ((((uint64_t)t3 << 32) | t2) >> 14) & 0x3ffffff;
	h4 += (t3 >> 8) | (1 << 24);


poly1305_donna_mul:
	t[0]  = mul32x32_64(h0,r0) + mul32x32_64(h1,s4) + mul32x32_64(h2,s3) + mul32x32_64(h3,s2) + mul32x32_64(h4,s1);
	t[1]  = mul32x32_64(h0,r1) + mul32x32_64(h1,r0) + mul32x32_64(h2,s4) + mul32x32_64(h3,s3) + mul32x32_64(h4,s2);
	t[2]  = mul32x32_64(h0,r2) + mul32x32_64(h1,r1) + mul32x32_64(h2,r0) + mul32x32_64(h3,s4) + mul32x32_64(h4,s3);
	t[3]  = mul32x32_64(h0,r3) + mul32x32_64(h1,r2) + mul32x32_64(h2,r1) + mul32x32_64(h3,r0) + mul32x32_64(h4,s4);
	t[4]  = mul32x32_64(h0,r4) + mul32x32_64(h1,r3) + mul32x32_64(h2,r2) + mul32x32_64(h3,r1) + mul32x32_64(h4,r0);

	                h0 = (uint32_t)t[0] & 0x3ffffff; c =           (t[0] >> 26);
	t[1] += c;      h1 = (uint32_t)t[1] & 0x3ffffff; b = (uint32_t)(t[1] >> 26);
	t[2] += b;      h2 = (uint32_t)t[2] & 0x3ffffff; b = (uint32_t)(t[2] >> 26);
	t[3] += b;      h3 = (uint32_t)t[3] & 0x3ffffff; b = (uint32_t)(t[3] >> 26);
	t[4] += b;      h4 = (uint32_t)t[4] & 0x3ffffff; b = (uint32_t)(t[4] >> 26);
	h0 += b * 5;

	if (inlen >= 16) goto poly1305_donna_16bytes;

	/* final bytes */
poly1305_donna_atmost15bytes:
	if (!inlen) goto poly1305_donna_finish;

	for (j = 0; j < inlen; j++) mp[j] = m[j];
	mp[j++] = 1;
	for (; j < 16; j++)	mp[j] = 0;
	inlen = 0;

	t0 = U8TO32_LE(mp+0);
	t1 = U8TO32_LE(mp+4);
	t2 = U8TO32_LE(mp+8);
	t3 = U8TO32_LE(mp+12);

	h0 += t0 & 0x3ffffff;
	h1 += ((((uint64_t)t1 << 32) | t0) >> 26) & 0x3ffffff;
	h2 += ((((uint64_t)t2 << 32) | t1) >> 20) & 0x3ffffff;
	h3 += ((((uint64_t)t3 << 32) | t2) >> 14) & 0x3ffffff;
	h4 += (t3 >> 8);

	goto poly1305_donna_mul;

poly1305_donna_finish:
	             b = h0 >> 26; h0 = h0 & 0x3ffffff;
	h1 +=     b; b = h1 >> 26; h1 = h1 & 0x3ffffff;
	h2 +=     b; b = h2 >> 26; h2 = h2 & 0x3ffffff;
	h3 +=     b; b = h3 >> 26; h3 = h3 & 0x3ffffff;
	h4 +=     b; b = h4 >> 26; h4 = h4 & 0x3ffffff;
	h0 += b * 5; b = h0 >> 26; h0 = h0 & 0x3ffffff;
	h1 +=     b;

	g0 = h0 + 5; b = g0 >> 26; g0 &= 0x3ffffff;
	g1 = h1 + b; b = g1 >> 26; g1 &= 0x3ffffff;
	g2 = h2 + b; b = g2 >> 26; g2 &= 0x3ffffff;
	g3 = h3 + b; b = g3 >> 26; g3 &= 0x3ffffff;
	g4 = h4 + b - (1 << 26);

	b = (g4 >> 31) - 1;
	nb = ~b;
	h0 = (h0 & nb) | (g0 & b);
	h1 = (h1 & nb) | (g1 & b);
	h2 = (h2 & nb) | (g2 & b);
	h3 = (h3 & nb) | (g3 & b);
	h4 = (h4 & nb) | (g4 & b);

	f0 = ((h0      ) | (h1 << 26)) + (uint64_t)U8TO32_LE(&key[16]);
	f1 = ((h1 >>  6) | (h2 << 20)) + (uint64_t)U8TO32_LE(&key[20]);
	f2 = ((h2 >> 12) | (h3 << 14)) + (uint64_t)U8TO32_LE(&key[24]);
	f3 = ((h3 >> 18) | (h4 <<  8)) + (uint64_t)U8TO32_LE(&key[28]);

	U32TO8_LE(&out[ 0], f0); f1 += (f0 >> 32);
	U32TO8_LE(&out[ 4], f1); f2 += (f1 >> 32);
	U32TO8_LE(&out[ 8], f2); f3 += (f2 >> 32);
	U32TO8_LE(&out[12], f3);
}
#endif	//	OPENSSL_VERSION_NUMBER < 0x10100000L

//////////////////////////////////////////////////////////////////////
//	chachapoly
//////////////////////////////////////////////////////////////////////

#define CHACHA_KEYLEN	32 /* Only 256 bit keys used here */

struct ssh_chachapoly_ctx
{
	struct chacha_ctx main_ctx, header_ctx;
	u_char seqbuf[8];
	u_char expected_tag[POLY1305_TAGLEN];
	u_char poly_key[POLY1305_KEYLEN];

#if	OPENSSL_VERSION_NUMBER >= 0x10100000L
	POLY1305 *poly_ctx;
#endif
};

static int ssh_chachapoly_cipher(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	size_t aadlen = 4;
	struct ssh_chachapoly_ctx *c;
	const u_char one[8] = { 1, 0, 0, 0, 0, 0, 0, 0 }; /* NB. little-endian */

	if ((c = (struct ssh_chachapoly_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	chacha_ivsetup(&c->header_ctx, c->seqbuf, NULL);
	chacha_encrypt_bytes(&c->header_ctx, src, dest, (u32)aadlen);

	if ( len > aadlen ) {
		chacha_ivsetup(&c->main_ctx, c->seqbuf, one);
		chacha_encrypt_bytes(&c->main_ctx, src + aadlen, dest + aadlen, (u32)(len - aadlen));
	}

	return (1);
}

static int ssh_chachapoly_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr)
{
	struct ssh_chachapoly_ctx *c;

	if ((c = (struct ssh_chachapoly_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	switch(type) {
	case EVP_CTRL_POLY_IV_GEN:
		// Run ChaCha20 once to generate the Poly1305 key. The IV is the packet sequence number.
		memset(c->poly_key, 0, POLY1305_KEYLEN);
		if ( ptr != NULL && arg == 8 )
			memcpy(c->seqbuf, ptr, arg);
		chacha_ivsetup(&c->main_ctx, c->seqbuf, NULL);
		chacha_encrypt_bytes(&c->main_ctx, c->poly_key, c->poly_key, POLY1305_KEYLEN);
		break;

	case EVP_CTRL_POLY_GET_TAG:
		if ( ptr != NULL && arg >= POLY1305_TAGLEN )
			memcpy(ptr, c->expected_tag, arg);
		break;

	case EVP_CTRL_POLY_SET_TAG:
		if ( ptr != NULL && arg > 0 ) {
#if	OPENSSL_VERSION_NUMBER >= 0x10100000L
			Poly1305_Init(c->poly_ctx, c->poly_key);
			Poly1305_Update(c->poly_ctx, (const unsigned char *)ptr, arg);
			Poly1305_Final(c->poly_ctx, c->expected_tag);
#else
			poly1305_auth(c->expected_tag, (u_char *)ptr, arg, c->poly_key);
#endif
		}
		break;
	}

	return (1);
}
static int ssh_chachapoly_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_chachapoly_ctx *c;

	if ((c = (struct ssh_chachapoly_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		if ( (c = (struct ssh_chachapoly_ctx *)malloc(sizeof(*c))) == NULL )
			return 0;
		memset(c, 0, sizeof(*c));

#if	OPENSSL_VERSION_NUMBER >= 0x10100000L
		if ( (c->poly_ctx = (POLY1305 *)malloc(Poly1305_ctx_size())) == NULL )
			return 0;
		memset(c->poly_ctx, 0, Poly1305_ctx_size());
#endif

		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}

	if (key != NULL) {
		if ( EVP_CIPHER_CTX_key_length(ctx) != (32 + 32) )
			return 0;
		chacha_keysetup(&c->main_ctx, key, 256);
		chacha_keysetup(&c->header_ctx, key + 32, 256);
	}

	return (1);
}
static int ssh_chachapoly_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_chachapoly_ctx *c;

	if ((c = (struct ssh_chachapoly_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
#if	OPENSSL_VERSION_NUMBER >= 0x10100000L
		free(c->poly_ctx);
#endif
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}
const EVP_CIPHER *evp_chachapoly_256(void)
{
	static EVP_CIPHER chachapoly_256;

	memset(&chachapoly_256, 0, sizeof(EVP_CIPHER));
	chachapoly_256.nid = NID_undef;
	chachapoly_256.block_size = 8;
	chachapoly_256.iv_len = 0;
	chachapoly_256.key_len = CHACHA_KEYLEN * 2;
	chachapoly_256.init = ssh_chachapoly_init;
	chachapoly_256.cleanup = ssh_chachapoly_cleanup;
	chachapoly_256.do_cipher = ssh_chachapoly_cipher;
	chachapoly_256.ctrl = ssh_chachapoly_ctrl;
	chachapoly_256.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&chachapoly_256);
}


//////////////////////////////////////////////////////////////////////
//	bcrypt_pbkdf
//////////////////////////////////////////////////////////////////////

// openbsd-compat/blowfish.c

/* $OpenBSD: blowfish.c,v 1.18 2004/11/02 17:23:26 hshoexer Exp $ */
/*
 * Blowfish block cipher for OpenBSD
 * Copyright 1997 Niels Provos <provos@physnet.uni-hamburg.de>
 * All rights reserved.
 *
 * Implementation advice by David Mazieres <dm@lcs.mit.edu>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This code is derived from section 14.3 and the given source
 * in section V of Applied Cryptography, second edition.
 * Blowfish is an unpatented fast block cipher designed by
 * Bruce Schneier.
 */

#define BLF_N	16			/* Number of Subkeys */
#define BLF_MAXKEYLEN ((BLF_N-2)*4)	/* 448 bits */
#define BLF_MAXUTILIZED ((BLF_N+2)*4)	/* 576 bits */

#define BCRYPT_BLOCKS 8
#define BCRYPT_HASHSIZE (BCRYPT_BLOCKS * 4)

typedef	unsigned char		u_int8_t;
typedef	unsigned short		u_int16_t;
typedef	unsigned long		u_int32_t;
typedef	unsigned long long	u_int64_t;

typedef struct BlowfishContext {
	u_int32_t S[4][256];	/* S-Boxes */
	u_int32_t P[BLF_N + 2]; /* Subkeys */
} blf_ctx;

#define F(s, x) ((((s)[        (((x)>>24)&0xFF)]  \
		 + (s)[0x100 + (((x)>>16)&0xFF)]) \
		 ^ (s)[0x200 + (((x)>> 8)&0xFF)]) \
		 + (s)[0x300 + ( (x)	 &0xFF)])

#define BLFRND(s,p,i,j,n) (i ^= F(s,j) ^ (p)[n])

static void Blowfish_initstate(blf_ctx *c)
{
	/* P-box and S-box tables initialized with digits of Pi */

	static const blf_ctx initstate =
	{ {
		{
			0xd1310ba6, 0x98dfb5ac, 0x2ffd72db, 0xd01adfb7,
			0xb8e1afed, 0x6a267e96, 0xba7c9045, 0xf12c7f99,
			0x24a19947, 0xb3916cf7, 0x0801f2e2, 0x858efc16,
			0x636920d8, 0x71574e69, 0xa458fea3, 0xf4933d7e,
			0x0d95748f, 0x728eb658, 0x718bcd58, 0x82154aee,
			0x7b54a41d, 0xc25a59b5, 0x9c30d539, 0x2af26013,
			0xc5d1b023, 0x286085f0, 0xca417918, 0xb8db38ef,
			0x8e79dcb0, 0x603a180e, 0x6c9e0e8b, 0xb01e8a3e,
			0xd71577c1, 0xbd314b27, 0x78af2fda, 0x55605c60,
			0xe65525f3, 0xaa55ab94, 0x57489862, 0x63e81440,
			0x55ca396a, 0x2aab10b6, 0xb4cc5c34, 0x1141e8ce,
			0xa15486af, 0x7c72e993, 0xb3ee1411, 0x636fbc2a,
			0x2ba9c55d, 0x741831f6, 0xce5c3e16, 0x9b87931e,
			0xafd6ba33, 0x6c24cf5c, 0x7a325381, 0x28958677,
			0x3b8f4898, 0x6b4bb9af, 0xc4bfe81b, 0x66282193,
			0x61d809cc, 0xfb21a991, 0x487cac60, 0x5dec8032,
			0xef845d5d, 0xe98575b1, 0xdc262302, 0xeb651b88,
			0x23893e81, 0xd396acc5, 0x0f6d6ff3, 0x83f44239,
			0x2e0b4482, 0xa4842004, 0x69c8f04a, 0x9e1f9b5e,
			0x21c66842, 0xf6e96c9a, 0x670c9c61, 0xabd388f0,
			0x6a51a0d2, 0xd8542f68, 0x960fa728, 0xab5133a3,
			0x6eef0b6c, 0x137a3be4, 0xba3bf050, 0x7efb2a98,
			0xa1f1651d, 0x39af0176, 0x66ca593e, 0x82430e88,
			0x8cee8619, 0x456f9fb4, 0x7d84a5c3, 0x3b8b5ebe,
			0xe06f75d8, 0x85c12073, 0x401a449f, 0x56c16aa6,
			0x4ed3aa62, 0x363f7706, 0x1bfedf72, 0x429b023d,
			0x37d0d724, 0xd00a1248, 0xdb0fead3, 0x49f1c09b,
			0x075372c9, 0x80991b7b, 0x25d479d8, 0xf6e8def7,
			0xe3fe501a, 0xb6794c3b, 0x976ce0bd, 0x04c006ba,
			0xc1a94fb6, 0x409f60c4, 0x5e5c9ec2, 0x196a2463,
			0x68fb6faf, 0x3e6c53b5, 0x1339b2eb, 0x3b52ec6f,
			0x6dfc511f, 0x9b30952c, 0xcc814544, 0xaf5ebd09,
			0xbee3d004, 0xde334afd, 0x660f2807, 0x192e4bb3,
			0xc0cba857, 0x45c8740f, 0xd20b5f39, 0xb9d3fbdb,
			0x5579c0bd, 0x1a60320a, 0xd6a100c6, 0x402c7279,
			0x679f25fe, 0xfb1fa3cc, 0x8ea5e9f8, 0xdb3222f8,
			0x3c7516df, 0xfd616b15, 0x2f501ec8, 0xad0552ab,
			0x323db5fa, 0xfd238760, 0x53317b48, 0x3e00df82,
			0x9e5c57bb, 0xca6f8ca0, 0x1a87562e, 0xdf1769db,
			0xd542a8f6, 0x287effc3, 0xac6732c6, 0x8c4f5573,
			0x695b27b0, 0xbbca58c8, 0xe1ffa35d, 0xb8f011a0,
			0x10fa3d98, 0xfd2183b8, 0x4afcb56c, 0x2dd1d35b,
			0x9a53e479, 0xb6f84565, 0xd28e49bc, 0x4bfb9790,
			0xe1ddf2da, 0xa4cb7e33, 0x62fb1341, 0xcee4c6e8,
			0xef20cada, 0x36774c01, 0xd07e9efe, 0x2bf11fb4,
			0x95dbda4d, 0xae909198, 0xeaad8e71, 0x6b93d5a0,
			0xd08ed1d0, 0xafc725e0, 0x8e3c5b2f, 0x8e7594b7,
			0x8ff6e2fb, 0xf2122b64, 0x8888b812, 0x900df01c,
			0x4fad5ea0, 0x688fc31c, 0xd1cff191, 0xb3a8c1ad,
			0x2f2f2218, 0xbe0e1777, 0xea752dfe, 0x8b021fa1,
			0xe5a0cc0f, 0xb56f74e8, 0x18acf3d6, 0xce89e299,
			0xb4a84fe0, 0xfd13e0b7, 0x7cc43b81, 0xd2ada8d9,
			0x165fa266, 0x80957705, 0x93cc7314, 0x211a1477,
			0xe6ad2065, 0x77b5fa86, 0xc75442f5, 0xfb9d35cf,
			0xebcdaf0c, 0x7b3e89a0, 0xd6411bd3, 0xae1e7e49,
			0x00250e2d, 0x2071b35e, 0x226800bb, 0x57b8e0af,
			0x2464369b, 0xf009b91e, 0x5563911d, 0x59dfa6aa,
			0x78c14389, 0xd95a537f, 0x207d5ba2, 0x02e5b9c5,
			0x83260376, 0x6295cfa9, 0x11c81968, 0x4e734a41,
			0xb3472dca, 0x7b14a94a, 0x1b510052, 0x9a532915,
			0xd60f573f, 0xbc9bc6e4, 0x2b60a476, 0x81e67400,
			0x08ba6fb5, 0x571be91f, 0xf296ec6b, 0x2a0dd915,
			0xb6636521, 0xe7b9f9b6, 0xff34052e, 0xc5855664,
		0x53b02d5d, 0xa99f8fa1, 0x08ba4799, 0x6e85076a},
		{
			0x4b7a70e9, 0xb5b32944, 0xdb75092e, 0xc4192623,
			0xad6ea6b0, 0x49a7df7d, 0x9cee60b8, 0x8fedb266,
			0xecaa8c71, 0x699a17ff, 0x5664526c, 0xc2b19ee1,
			0x193602a5, 0x75094c29, 0xa0591340, 0xe4183a3e,
			0x3f54989a, 0x5b429d65, 0x6b8fe4d6, 0x99f73fd6,
			0xa1d29c07, 0xefe830f5, 0x4d2d38e6, 0xf0255dc1,
			0x4cdd2086, 0x8470eb26, 0x6382e9c6, 0x021ecc5e,
			0x09686b3f, 0x3ebaefc9, 0x3c971814, 0x6b6a70a1,
			0x687f3584, 0x52a0e286, 0xb79c5305, 0xaa500737,
			0x3e07841c, 0x7fdeae5c, 0x8e7d44ec, 0x5716f2b8,
			0xb03ada37, 0xf0500c0d, 0xf01c1f04, 0x0200b3ff,
			0xae0cf51a, 0x3cb574b2, 0x25837a58, 0xdc0921bd,
			0xd19113f9, 0x7ca92ff6, 0x94324773, 0x22f54701,
			0x3ae5e581, 0x37c2dadc, 0xc8b57634, 0x9af3dda7,
			0xa9446146, 0x0fd0030e, 0xecc8c73e, 0xa4751e41,
			0xe238cd99, 0x3bea0e2f, 0x3280bba1, 0x183eb331,
			0x4e548b38, 0x4f6db908, 0x6f420d03, 0xf60a04bf,
			0x2cb81290, 0x24977c79, 0x5679b072, 0xbcaf89af,
			0xde9a771f, 0xd9930810, 0xb38bae12, 0xdccf3f2e,
			0x5512721f, 0x2e6b7124, 0x501adde6, 0x9f84cd87,
			0x7a584718, 0x7408da17, 0xbc9f9abc, 0xe94b7d8c,
			0xec7aec3a, 0xdb851dfa, 0x63094366, 0xc464c3d2,
			0xef1c1847, 0x3215d908, 0xdd433b37, 0x24c2ba16,
			0x12a14d43, 0x2a65c451, 0x50940002, 0x133ae4dd,
			0x71dff89e, 0x10314e55, 0x81ac77d6, 0x5f11199b,
			0x043556f1, 0xd7a3c76b, 0x3c11183b, 0x5924a509,
			0xf28fe6ed, 0x97f1fbfa, 0x9ebabf2c, 0x1e153c6e,
			0x86e34570, 0xeae96fb1, 0x860e5e0a, 0x5a3e2ab3,
			0x771fe71c, 0x4e3d06fa, 0x2965dcb9, 0x99e71d0f,
			0x803e89d6, 0x5266c825, 0x2e4cc978, 0x9c10b36a,
			0xc6150eba, 0x94e2ea78, 0xa5fc3c53, 0x1e0a2df4,
			0xf2f74ea7, 0x361d2b3d, 0x1939260f, 0x19c27960,
			0x5223a708, 0xf71312b6, 0xebadfe6e, 0xeac31f66,
			0xe3bc4595, 0xa67bc883, 0xb17f37d1, 0x018cff28,
			0xc332ddef, 0xbe6c5aa5, 0x65582185, 0x68ab9802,
			0xeecea50f, 0xdb2f953b, 0x2aef7dad, 0x5b6e2f84,
			0x1521b628, 0x29076170, 0xecdd4775, 0x619f1510,
			0x13cca830, 0xeb61bd96, 0x0334fe1e, 0xaa0363cf,
			0xb5735c90, 0x4c70a239, 0xd59e9e0b, 0xcbaade14,
			0xeecc86bc, 0x60622ca7, 0x9cab5cab, 0xb2f3846e,
			0x648b1eaf, 0x19bdf0ca, 0xa02369b9, 0x655abb50,
			0x40685a32, 0x3c2ab4b3, 0x319ee9d5, 0xc021b8f7,
			0x9b540b19, 0x875fa099, 0x95f7997e, 0x623d7da8,
			0xf837889a, 0x97e32d77, 0x11ed935f, 0x16681281,
			0x0e358829, 0xc7e61fd6, 0x96dedfa1, 0x7858ba99,
			0x57f584a5, 0x1b227263, 0x9b83c3ff, 0x1ac24696,
			0xcdb30aeb, 0x532e3054, 0x8fd948e4, 0x6dbc3128,
			0x58ebf2ef, 0x34c6ffea, 0xfe28ed61, 0xee7c3c73,
			0x5d4a14d9, 0xe864b7e3, 0x42105d14, 0x203e13e0,
			0x45eee2b6, 0xa3aaabea, 0xdb6c4f15, 0xfacb4fd0,
			0xc742f442, 0xef6abbb5, 0x654f3b1d, 0x41cd2105,
			0xd81e799e, 0x86854dc7, 0xe44b476a, 0x3d816250,
			0xcf62a1f2, 0x5b8d2646, 0xfc8883a0, 0xc1c7b6a3,
			0x7f1524c3, 0x69cb7492, 0x47848a0b, 0x5692b285,
			0x095bbf00, 0xad19489d, 0x1462b174, 0x23820e00,
			0x58428d2a, 0x0c55f5ea, 0x1dadf43e, 0x233f7061,
			0x3372f092, 0x8d937e41, 0xd65fecf1, 0x6c223bdb,
			0x7cde3759, 0xcbee7460, 0x4085f2a7, 0xce77326e,
			0xa6078084, 0x19f8509e, 0xe8efd855, 0x61d99735,
			0xa969a7aa, 0xc50c06c2, 0x5a04abfc, 0x800bcadc,
			0x9e447a2e, 0xc3453484, 0xfdd56705, 0x0e1e9ec9,
			0xdb73dbd3, 0x105588cd, 0x675fda79, 0xe3674340,
			0xc5c43465, 0x713e38d8, 0x3d28f89e, 0xf16dff20,
		0x153e21e7, 0x8fb03d4a, 0xe6e39f2b, 0xdb83adf7},
		{
			0xe93d5a68, 0x948140f7, 0xf64c261c, 0x94692934,
			0x411520f7, 0x7602d4f7, 0xbcf46b2e, 0xd4a20068,
			0xd4082471, 0x3320f46a, 0x43b7d4b7, 0x500061af,
			0x1e39f62e, 0x97244546, 0x14214f74, 0xbf8b8840,
			0x4d95fc1d, 0x96b591af, 0x70f4ddd3, 0x66a02f45,
			0xbfbc09ec, 0x03bd9785, 0x7fac6dd0, 0x31cb8504,
			0x96eb27b3, 0x55fd3941, 0xda2547e6, 0xabca0a9a,
			0x28507825, 0x530429f4, 0x0a2c86da, 0xe9b66dfb,
			0x68dc1462, 0xd7486900, 0x680ec0a4, 0x27a18dee,
			0x4f3ffea2, 0xe887ad8c, 0xb58ce006, 0x7af4d6b6,
			0xaace1e7c, 0xd3375fec, 0xce78a399, 0x406b2a42,
			0x20fe9e35, 0xd9f385b9, 0xee39d7ab, 0x3b124e8b,
			0x1dc9faf7, 0x4b6d1856, 0x26a36631, 0xeae397b2,
			0x3a6efa74, 0xdd5b4332, 0x6841e7f7, 0xca7820fb,
			0xfb0af54e, 0xd8feb397, 0x454056ac, 0xba489527,
			0x55533a3a, 0x20838d87, 0xfe6ba9b7, 0xd096954b,
			0x55a867bc, 0xa1159a58, 0xcca92963, 0x99e1db33,
			0xa62a4a56, 0x3f3125f9, 0x5ef47e1c, 0x9029317c,
			0xfdf8e802, 0x04272f70, 0x80bb155c, 0x05282ce3,
			0x95c11548, 0xe4c66d22, 0x48c1133f, 0xc70f86dc,
			0x07f9c9ee, 0x41041f0f, 0x404779a4, 0x5d886e17,
			0x325f51eb, 0xd59bc0d1, 0xf2bcc18f, 0x41113564,
			0x257b7834, 0x602a9c60, 0xdff8e8a3, 0x1f636c1b,
			0x0e12b4c2, 0x02e1329e, 0xaf664fd1, 0xcad18115,
			0x6b2395e0, 0x333e92e1, 0x3b240b62, 0xeebeb922,
			0x85b2a20e, 0xe6ba0d99, 0xde720c8c, 0x2da2f728,
			0xd0127845, 0x95b794fd, 0x647d0862, 0xe7ccf5f0,
			0x5449a36f, 0x877d48fa, 0xc39dfd27, 0xf33e8d1e,
			0x0a476341, 0x992eff74, 0x3a6f6eab, 0xf4f8fd37,
			0xa812dc60, 0xa1ebddf8, 0x991be14c, 0xdb6e6b0d,
			0xc67b5510, 0x6d672c37, 0x2765d43b, 0xdcd0e804,
			0xf1290dc7, 0xcc00ffa3, 0xb5390f92, 0x690fed0b,
			0x667b9ffb, 0xcedb7d9c, 0xa091cf0b, 0xd9155ea3,
			0xbb132f88, 0x515bad24, 0x7b9479bf, 0x763bd6eb,
			0x37392eb3, 0xcc115979, 0x8026e297, 0xf42e312d,
			0x6842ada7, 0xc66a2b3b, 0x12754ccc, 0x782ef11c,
			0x6a124237, 0xb79251e7, 0x06a1bbe6, 0x4bfb6350,
			0x1a6b1018, 0x11caedfa, 0x3d25bdd8, 0xe2e1c3c9,
			0x44421659, 0x0a121386, 0xd90cec6e, 0xd5abea2a,
			0x64af674e, 0xda86a85f, 0xbebfe988, 0x64e4c3fe,
			0x9dbc8057, 0xf0f7c086, 0x60787bf8, 0x6003604d,
			0xd1fd8346, 0xf6381fb0, 0x7745ae04, 0xd736fccc,
			0x83426b33, 0xf01eab71, 0xb0804187, 0x3c005e5f,
			0x77a057be, 0xbde8ae24, 0x55464299, 0xbf582e61,
			0x4e58f48f, 0xf2ddfda2, 0xf474ef38, 0x8789bdc2,
			0x5366f9c3, 0xc8b38e74, 0xb475f255, 0x46fcd9b9,
			0x7aeb2661, 0x8b1ddf84, 0x846a0e79, 0x915f95e2,
			0x466e598e, 0x20b45770, 0x8cd55591, 0xc902de4c,
			0xb90bace1, 0xbb8205d0, 0x11a86248, 0x7574a99e,
			0xb77f19b6, 0xe0a9dc09, 0x662d09a1, 0xc4324633,
			0xe85a1f02, 0x09f0be8c, 0x4a99a025, 0x1d6efe10,
			0x1ab93d1d, 0x0ba5a4df, 0xa186f20f, 0x2868f169,
			0xdcb7da83, 0x573906fe, 0xa1e2ce9b, 0x4fcd7f52,
			0x50115e01, 0xa70683fa, 0xa002b5c4, 0x0de6d027,
			0x9af88c27, 0x773f8641, 0xc3604c06, 0x61a806b5,
			0xf0177a28, 0xc0f586e0, 0x006058aa, 0x30dc7d62,
			0x11e69ed7, 0x2338ea63, 0x53c2dd94, 0xc2c21634,
			0xbbcbee56, 0x90bcb6de, 0xebfc7da1, 0xce591d76,
			0x6f05e409, 0x4b7c0188, 0x39720a3d, 0x7c927c24,
			0x86e3725f, 0x724d9db9, 0x1ac15bb4, 0xd39eb8fc,
			0xed545578, 0x08fca5b5, 0xd83d7cd3, 0x4dad0fc4,
			0x1e50ef5e, 0xb161e6f8, 0xa28514d9, 0x6c51133c,
			0x6fd5c7e7, 0x56e14ec4, 0x362abfce, 0xddc6c837,
		0xd79a3234, 0x92638212, 0x670efa8e, 0x406000e0},
		{
			0x3a39ce37, 0xd3faf5cf, 0xabc27737, 0x5ac52d1b,
			0x5cb0679e, 0x4fa33742, 0xd3822740, 0x99bc9bbe,
			0xd5118e9d, 0xbf0f7315, 0xd62d1c7e, 0xc700c47b,
			0xb78c1b6b, 0x21a19045, 0xb26eb1be, 0x6a366eb4,
			0x5748ab2f, 0xbc946e79, 0xc6a376d2, 0x6549c2c8,
			0x530ff8ee, 0x468dde7d, 0xd5730a1d, 0x4cd04dc6,
			0x2939bbdb, 0xa9ba4650, 0xac9526e8, 0xbe5ee304,
			0xa1fad5f0, 0x6a2d519a, 0x63ef8ce2, 0x9a86ee22,
			0xc089c2b8, 0x43242ef6, 0xa51e03aa, 0x9cf2d0a4,
			0x83c061ba, 0x9be96a4d, 0x8fe51550, 0xba645bd6,
			0x2826a2f9, 0xa73a3ae1, 0x4ba99586, 0xef5562e9,
			0xc72fefd3, 0xf752f7da, 0x3f046f69, 0x77fa0a59,
			0x80e4a915, 0x87b08601, 0x9b09e6ad, 0x3b3ee593,
			0xe990fd5a, 0x9e34d797, 0x2cf0b7d9, 0x022b8b51,
			0x96d5ac3a, 0x017da67d, 0xd1cf3ed6, 0x7c7d2d28,
			0x1f9f25cf, 0xadf2b89b, 0x5ad6b472, 0x5a88f54c,
			0xe029ac71, 0xe019a5e6, 0x47b0acfd, 0xed93fa9b,
			0xe8d3c48d, 0x283b57cc, 0xf8d56629, 0x79132e28,
			0x785f0191, 0xed756055, 0xf7960e44, 0xe3d35e8c,
			0x15056dd4, 0x88f46dba, 0x03a16125, 0x0564f0bd,
			0xc3eb9e15, 0x3c9057a2, 0x97271aec, 0xa93a072a,
			0x1b3f6d9b, 0x1e6321f5, 0xf59c66fb, 0x26dcf319,
			0x7533d928, 0xb155fdf5, 0x03563482, 0x8aba3cbb,
			0x28517711, 0xc20ad9f8, 0xabcc5167, 0xccad925f,
			0x4de81751, 0x3830dc8e, 0x379d5862, 0x9320f991,
			0xea7a90c2, 0xfb3e7bce, 0x5121ce64, 0x774fbe32,
			0xa8b6e37e, 0xc3293d46, 0x48de5369, 0x6413e680,
			0xa2ae0810, 0xdd6db224, 0x69852dfd, 0x09072166,
			0xb39a460a, 0x6445c0dd, 0x586cdecf, 0x1c20c8ae,
			0x5bbef7dd, 0x1b588d40, 0xccd2017f, 0x6bb4e3bb,
			0xdda26a7e, 0x3a59ff45, 0x3e350a44, 0xbcb4cdd5,
			0x72eacea8, 0xfa6484bb, 0x8d6612ae, 0xbf3c6f47,
			0xd29be463, 0x542f5d9e, 0xaec2771b, 0xf64e6370,
			0x740e0d8d, 0xe75b1357, 0xf8721671, 0xaf537d5d,
			0x4040cb08, 0x4eb4e2cc, 0x34d2466a, 0x0115af84,
			0xe1b00428, 0x95983a1d, 0x06b89fb4, 0xce6ea048,
			0x6f3f3b82, 0x3520ab82, 0x011a1d4b, 0x277227f8,
			0x611560b1, 0xe7933fdc, 0xbb3a792b, 0x344525bd,
			0xa08839e1, 0x51ce794b, 0x2f32c9b7, 0xa01fbac9,
			0xe01cc87e, 0xbcc7d1f6, 0xcf0111c3, 0xa1e8aac7,
			0x1a908749, 0xd44fbd9a, 0xd0dadecb, 0xd50ada38,
			0x0339c32a, 0xc6913667, 0x8df9317c, 0xe0b12b4f,
			0xf79e59b7, 0x43f5bb3a, 0xf2d519ff, 0x27d9459c,
			0xbf97222c, 0x15e6fc2a, 0x0f91fc71, 0x9b941525,
			0xfae59361, 0xceb69ceb, 0xc2a86459, 0x12baa8d1,
			0xb6c1075e, 0xe3056a0c, 0x10d25065, 0xcb03a442,
			0xe0ec6e0e, 0x1698db3b, 0x4c98a0be, 0x3278e964,
			0x9f1f9532, 0xe0d392df, 0xd3a0342b, 0x8971f21e,
			0x1b0a7441, 0x4ba3348c, 0xc5be7120, 0xc37632d8,
			0xdf359f8d, 0x9b992f2e, 0xe60b6f47, 0x0fe3f11d,
			0xe54cda54, 0x1edad891, 0xce6279cf, 0xcd3e7e6f,
			0x1618b166, 0xfd2c1d05, 0x848fd2c5, 0xf6fb2299,
			0xf523f357, 0xa6327623, 0x93a83531, 0x56cccd02,
			0xacf08162, 0x5a75ebb5, 0x6e163697, 0x88d273cc,
			0xde966292, 0x81b949d0, 0x4c50901b, 0x71c65614,
			0xe6c6c7bd, 0x327a140a, 0x45e1d006, 0xc3f27b9a,
			0xc9aa53fd, 0x62a80f00, 0xbb25bfe2, 0x35bdd2f6,
			0x71126905, 0xb2040222, 0xb6cbcf7c, 0xcd769c2b,
			0x53113ec0, 0x1640e3d3, 0x38abbd60, 0x2547adf0,
			0xba38209c, 0xf746ce76, 0x77afa1c5, 0x20756060,
			0x85cbfe4e, 0x8ae88dd8, 0x7aaaf9b0, 0x4cf9aa7e,
			0x1948c25c, 0x02fb8a8c, 0x01c36ae4, 0xd6ebe1f9,
			0x90d4f869, 0xa65cdea0, 0x3f09252d, 0xc208e69f,
		0xb74e6132, 0xce77e25b, 0x578fdfe3, 0x3ac372e6}
	},
	{
		0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344,
		0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89,
		0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
		0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917,
		0x9216d5d9, 0x8979fb1b
	} };

	*c = initstate;
}
static void Blowfish_encipher(blf_ctx *c, u_int32_t *xl, u_int32_t *xr)
{
	u_int32_t Xl;
	u_int32_t Xr;
	u_int32_t *s = c->S[0];
	u_int32_t *p = c->P;

	Xl = *xl;
	Xr = *xr;

	Xl ^= p[0];
	BLFRND(s, p, Xr, Xl, 1); BLFRND(s, p, Xl, Xr, 2);
	BLFRND(s, p, Xr, Xl, 3); BLFRND(s, p, Xl, Xr, 4);
	BLFRND(s, p, Xr, Xl, 5); BLFRND(s, p, Xl, Xr, 6);
	BLFRND(s, p, Xr, Xl, 7); BLFRND(s, p, Xl, Xr, 8);
	BLFRND(s, p, Xr, Xl, 9); BLFRND(s, p, Xl, Xr, 10);
	BLFRND(s, p, Xr, Xl, 11); BLFRND(s, p, Xl, Xr, 12);
	BLFRND(s, p, Xr, Xl, 13); BLFRND(s, p, Xl, Xr, 14);
	BLFRND(s, p, Xr, Xl, 15); BLFRND(s, p, Xl, Xr, 16);

	*xl = Xr ^ p[17];
	*xr = Xl;
}
static u_int32_t Blowfish_stream2word(const u_int8_t *data, u_int16_t databytes, u_int16_t *current)
{
	u_int8_t i;
	u_int16_t j;
	u_int32_t temp;

	temp = 0x00000000;
	j = *current;

	for (i = 0; i < 4; i++, j++) {
		if (j >= databytes)
			j = 0;
		temp = (temp << 8) | data[j];
	}

	*current = j;
	return temp;
}
static void Blowfish_expand0state(blf_ctx *c, const u_int8_t *key, u_int16_t keybytes)
{
	u_int16_t i;
	u_int16_t j;
	u_int16_t k;
	u_int32_t temp;
	u_int32_t datal;
	u_int32_t datar;

	j = 0;
	for (i = 0; i < BLF_N + 2; i++) {
		/* Extract 4 int8 to 1 int32 from keystream */
		temp = Blowfish_stream2word(key, keybytes, &j);
		c->P[i] = c->P[i] ^ temp;
	}

	j = 0;
	datal = 0x00000000;
	datar = 0x00000000;
	for (i = 0; i < BLF_N + 2; i += 2) {
		Blowfish_encipher(c, &datal, &datar);

		c->P[i] = datal;
		c->P[i + 1] = datar;
	}

	for (i = 0; i < 4; i++) {
		for (k = 0; k < 256; k += 2) {
			Blowfish_encipher(c, &datal, &datar);

			c->S[i][k] = datal;
			c->S[i][k + 1] = datar;
		}
	}
}
static void Blowfish_expandstate(blf_ctx *c, const u_int8_t *data, u_int16_t databytes, const u_int8_t *key, u_int16_t keybytes)
{
	u_int16_t i;
	u_int16_t j;
	u_int16_t k;
	u_int32_t temp;
	u_int32_t datal;
	u_int32_t datar;

	j = 0;
	for (i = 0; i < BLF_N + 2; i++) {
		/* Extract 4 int8 to 1 int32 from keystream */
		temp = Blowfish_stream2word(key, keybytes, &j);
		c->P[i] = c->P[i] ^ temp;
	}

	j = 0;
	datal = 0x00000000;
	datar = 0x00000000;
	for (i = 0; i < BLF_N + 2; i += 2) {
		datal ^= Blowfish_stream2word(data, databytes, &j);
		datar ^= Blowfish_stream2word(data, databytes, &j);
		Blowfish_encipher(c, &datal, &datar);

		c->P[i] = datal;
		c->P[i + 1] = datar;
	}

	for (i = 0; i < 4; i++) {
		for (k = 0; k < 256; k += 2) {
			datal ^= Blowfish_stream2word(data, databytes, &j);
			datar ^= Blowfish_stream2word(data, databytes, &j);
			Blowfish_encipher(c, &datal, &datar);

			c->S[i][k] = datal;
			c->S[i][k + 1] = datar;
		}
	}

}
static void blf_enc(blf_ctx *c, u_int32_t *data, u_int16_t blocks)
{
	u_int32_t *d;
	u_int16_t i;

	d = data;
	for (i = 0; i < blocks; i++) {
		Blowfish_encipher(c, d, d + 1);
		d += 2;
	}
}

// openbsd-compat/bcrypt_pbkdf.c

/*
 * pkcs #5 pbkdf2 implementation using the "bcrypt" hash
 *
 * The bcrypt hash function is derived from the bcrypt password hashing
 * function with the following modifications:
 * 1. The input password and salt are preprocessed with SHA512.
 * 2. The output length is expanded to 256 bits.
 * 3. Subsequently the magic string to be encrypted is lengthened and modifed
 *    to "OxychromaticBlowfishSwatDynamite"
 * 4. The hash function is defined to perform 64 rounds of initial state
 *    expansion. (More rounds are performed by iterating the hash.)
 *
 * Note that this implementation pulls the SHA512 operations into the caller
 * as a performance optimization.
 *
 * One modification from official pbkdf2. Instead of outputting key material
 * linearly, we mix it. pbkdf2 has a known weakness where if one uses it to
 * generate (i.e.) 512 bits of key material for use as two 256 bit keys, an
 * attacker can merely run once through the outer loop below, but the user
 * always runs it twice. Shuffling output bytes requires computing the
 * entirety of the key material to assemble any subkey. This is something a
 * wise caller could do; we just do it for you.
 */

static u_int64_t load_bigendian(const unsigned char *x)
{
  return
      (u_int64_t) (x[7]) \
  | (((u_int64_t) (x[6])) << 8) \
  | (((u_int64_t) (x[5])) << 16) \
  | (((u_int64_t) (x[4])) << 24) \
  | (((u_int64_t) (x[3])) << 32) \
  | (((u_int64_t) (x[2])) << 40) \
  | (((u_int64_t) (x[1])) << 48) \
  | (((u_int64_t) (x[0])) << 56)
  ;
}

static void store_bigendian(unsigned char *x, u_int64_t u)
{
  x[7] = (unsigned char)(u); u >>= 8;
  x[6] = (unsigned char)(u); u >>= 8;
  x[5] = (unsigned char)(u); u >>= 8;
  x[4] = (unsigned char)(u); u >>= 8;
  x[3] = (unsigned char)(u); u >>= 8;
  x[2] = (unsigned char)(u); u >>= 8;
  x[1] = (unsigned char)(u); u >>= 8;
  x[0] = (unsigned char)(u);
}

#define SHR(x,c) ((x) >> (c))
#define ROTR(x,c) (((x) >> (c)) | ((x) << (64 - (c))))

#define Ch(x,y,z) ((x & y) ^ (~x & z))
#define Maj(x,y,z) ((x & y) ^ (x & z) ^ (y & z))
#define Sigma0(x) (ROTR(x,28) ^ ROTR(x,34) ^ ROTR(x,39))
#define Sigma1(x) (ROTR(x,14) ^ ROTR(x,18) ^ ROTR(x,41))
#define sigma0(x) (ROTR(x, 1) ^ ROTR(x, 8) ^ SHR(x,7))
#define sigma1(x) (ROTR(x,19) ^ ROTR(x,61) ^ SHR(x,6))

#define M(w0,w14,w9,w1) w0 = sigma1(w14) + w9 + sigma0(w1) + w0;

#define EXPAND \
  M(w0 ,w14,w9 ,w1 ) \
  M(w1 ,w15,w10,w2 ) \
  M(w2 ,w0 ,w11,w3 ) \
  M(w3 ,w1 ,w12,w4 ) \
  M(w4 ,w2 ,w13,w5 ) \
  M(w5 ,w3 ,w14,w6 ) \
  M(w6 ,w4 ,w15,w7 ) \
  M(w7 ,w5 ,w0 ,w8 ) \
  M(w8 ,w6 ,w1 ,w9 ) \
  M(w9 ,w7 ,w2 ,w10) \
  M(w10,w8 ,w3 ,w11) \
  M(w11,w9 ,w4 ,w12) \
  M(w12,w10,w5 ,w13) \
  M(w13,w11,w6 ,w14) \
  M(w14,w12,w7 ,w15) \
  M(w15,w13,w8 ,w0 )

#undef	F
#define F(w,k) \
  T1 = h + Sigma1(e) + Ch(e,f,g) + k + w; \
  T2 = Sigma0(a) + Maj(a,b,c); \
  h = g; \
  g = f; \
  f = e; \
  e = d + T1; \
  d = c; \
  c = b; \
  b = a; \
  a = T1 + T2;

static u_int64_t crypto_hashblocks_sha512(unsigned char *statebytes,const unsigned char *in,unsigned long long inlen)
{
  u_int64_t state[8];
  u_int64_t a;
  u_int64_t b;
  u_int64_t c;
  u_int64_t d;
  u_int64_t e;
  u_int64_t f;
  u_int64_t g;
  u_int64_t h;
  u_int64_t T1;
  u_int64_t T2;

  a = load_bigendian(statebytes +  0); state[0] = a;
  b = load_bigendian(statebytes +  8); state[1] = b;
  c = load_bigendian(statebytes + 16); state[2] = c;
  d = load_bigendian(statebytes + 24); state[3] = d;
  e = load_bigendian(statebytes + 32); state[4] = e;
  f = load_bigendian(statebytes + 40); state[5] = f;
  g = load_bigendian(statebytes + 48); state[6] = g;
  h = load_bigendian(statebytes + 56); state[7] = h;

  while (inlen >= 128) {
    u_int64_t w0  = load_bigendian(in +   0);
    u_int64_t w1  = load_bigendian(in +   8);
    u_int64_t w2  = load_bigendian(in +  16);
    u_int64_t w3  = load_bigendian(in +  24);
    u_int64_t w4  = load_bigendian(in +  32);
    u_int64_t w5  = load_bigendian(in +  40);
    u_int64_t w6  = load_bigendian(in +  48);
    u_int64_t w7  = load_bigendian(in +  56);
    u_int64_t w8  = load_bigendian(in +  64);
    u_int64_t w9  = load_bigendian(in +  72);
    u_int64_t w10 = load_bigendian(in +  80);
    u_int64_t w11 = load_bigendian(in +  88);
    u_int64_t w12 = load_bigendian(in +  96);
    u_int64_t w13 = load_bigendian(in + 104);
    u_int64_t w14 = load_bigendian(in + 112);
    u_int64_t w15 = load_bigendian(in + 120);

    F(w0 ,0x428a2f98d728ae22ULL)
    F(w1 ,0x7137449123ef65cdULL)
    F(w2 ,0xb5c0fbcfec4d3b2fULL)
    F(w3 ,0xe9b5dba58189dbbcULL)
    F(w4 ,0x3956c25bf348b538ULL)
    F(w5 ,0x59f111f1b605d019ULL)
    F(w6 ,0x923f82a4af194f9bULL)
    F(w7 ,0xab1c5ed5da6d8118ULL)
    F(w8 ,0xd807aa98a3030242ULL)
    F(w9 ,0x12835b0145706fbeULL)
    F(w10,0x243185be4ee4b28cULL)
    F(w11,0x550c7dc3d5ffb4e2ULL)
    F(w12,0x72be5d74f27b896fULL)
    F(w13,0x80deb1fe3b1696b1ULL)
    F(w14,0x9bdc06a725c71235ULL)
    F(w15,0xc19bf174cf692694ULL)

    EXPAND

    F(w0 ,0xe49b69c19ef14ad2ULL)
    F(w1 ,0xefbe4786384f25e3ULL)
    F(w2 ,0x0fc19dc68b8cd5b5ULL)
    F(w3 ,0x240ca1cc77ac9c65ULL)
    F(w4 ,0x2de92c6f592b0275ULL)
    F(w5 ,0x4a7484aa6ea6e483ULL)
    F(w6 ,0x5cb0a9dcbd41fbd4ULL)
    F(w7 ,0x76f988da831153b5ULL)
    F(w8 ,0x983e5152ee66dfabULL)
    F(w9 ,0xa831c66d2db43210ULL)
    F(w10,0xb00327c898fb213fULL)
    F(w11,0xbf597fc7beef0ee4ULL)
    F(w12,0xc6e00bf33da88fc2ULL)
    F(w13,0xd5a79147930aa725ULL)
    F(w14,0x06ca6351e003826fULL)
    F(w15,0x142929670a0e6e70ULL)

    EXPAND

    F(w0 ,0x27b70a8546d22ffcULL)
    F(w1 ,0x2e1b21385c26c926ULL)
    F(w2 ,0x4d2c6dfc5ac42aedULL)
    F(w3 ,0x53380d139d95b3dfULL)
    F(w4 ,0x650a73548baf63deULL)
    F(w5 ,0x766a0abb3c77b2a8ULL)
    F(w6 ,0x81c2c92e47edaee6ULL)
    F(w7 ,0x92722c851482353bULL)
    F(w8 ,0xa2bfe8a14cf10364ULL)
    F(w9 ,0xa81a664bbc423001ULL)
    F(w10,0xc24b8b70d0f89791ULL)
    F(w11,0xc76c51a30654be30ULL)
    F(w12,0xd192e819d6ef5218ULL)
    F(w13,0xd69906245565a910ULL)
    F(w14,0xf40e35855771202aULL)
    F(w15,0x106aa07032bbd1b8ULL)

    EXPAND

    F(w0 ,0x19a4c116b8d2d0c8ULL)
    F(w1 ,0x1e376c085141ab53ULL)
    F(w2 ,0x2748774cdf8eeb99ULL)
    F(w3 ,0x34b0bcb5e19b48a8ULL)
    F(w4 ,0x391c0cb3c5c95a63ULL)
    F(w5 ,0x4ed8aa4ae3418acbULL)
    F(w6 ,0x5b9cca4f7763e373ULL)
    F(w7 ,0x682e6ff3d6b2b8a3ULL)
    F(w8 ,0x748f82ee5defb2fcULL)
    F(w9 ,0x78a5636f43172f60ULL)
    F(w10,0x84c87814a1f0ab72ULL)
    F(w11,0x8cc702081a6439ecULL)
    F(w12,0x90befffa23631e28ULL)
    F(w13,0xa4506cebde82bde9ULL)
    F(w14,0xbef9a3f7b2c67915ULL)
    F(w15,0xc67178f2e372532bULL)

    EXPAND

    F(w0 ,0xca273eceea26619cULL)
    F(w1 ,0xd186b8c721c0c207ULL)
    F(w2 ,0xeada7dd6cde0eb1eULL)
    F(w3 ,0xf57d4f7fee6ed178ULL)
    F(w4 ,0x06f067aa72176fbaULL)
    F(w5 ,0x0a637dc5a2c898a6ULL)
    F(w6 ,0x113f9804bef90daeULL)
    F(w7 ,0x1b710b35131c471bULL)
    F(w8 ,0x28db77f523047d84ULL)
    F(w9 ,0x32caab7b40c72493ULL)
    F(w10,0x3c9ebe0a15c9bebcULL)
    F(w11,0x431d67c49c100d4cULL)
    F(w12,0x4cc5d4becb3e42b6ULL)
    F(w13,0x597f299cfc657e2aULL)
    F(w14,0x5fcb6fab3ad6faecULL)
    F(w15,0x6c44198c4a475817ULL)

    a += state[0];
    b += state[1];
    c += state[2];
    d += state[3];
    e += state[4];
    f += state[5];
    g += state[6];
    h += state[7];
  
    state[0] = a;
    state[1] = b;
    state[2] = c;
    state[3] = d;
    state[4] = e;
    state[5] = f;
    state[6] = g;
    state[7] = h;

    in += 128;
    inlen -= 128;
  }

  store_bigendian(statebytes +  0,state[0]);
  store_bigendian(statebytes +  8,state[1]);
  store_bigendian(statebytes + 16,state[2]);
  store_bigendian(statebytes + 24,state[3]);
  store_bigendian(statebytes + 32,state[4]);
  store_bigendian(statebytes + 40,state[5]);
  store_bigendian(statebytes + 48,state[6]);
  store_bigendian(statebytes + 56,state[7]);

  return inlen;
}
int crypto_hash_sha512(unsigned char *out,const unsigned char *in,unsigned long long inlen)
{
  unsigned char h[64];
  unsigned char padded[256];
  unsigned int i;
  unsigned long long bytes = inlen;
  static const unsigned char iv[64] = {
	  0x6a,0x09,0xe6,0x67,0xf3,0xbc,0xc9,0x08,
	  0xbb,0x67,0xae,0x85,0x84,0xca,0xa7,0x3b,
	  0x3c,0x6e,0xf3,0x72,0xfe,0x94,0xf8,0x2b,
	  0xa5,0x4f,0xf5,0x3a,0x5f,0x1d,0x36,0xf1,
	  0x51,0x0e,0x52,0x7f,0xad,0xe6,0x82,0xd1,
	  0x9b,0x05,0x68,0x8c,0x2b,0x3e,0x6c,0x1f,
	  0x1f,0x83,0xd9,0xab,0xfb,0x41,0xbd,0x6b,
	  0x5b,0xe0,0xcd,0x19,0x13,0x7e,0x21,0x79
	} ;


  for (i = 0;i < 64;++i) h[i] = iv[i];

  crypto_hashblocks_sha512(h,in,inlen);
  in += inlen;
  inlen &= 127;
  in -= inlen;

  for (i = 0;i < inlen;++i) padded[i] = in[i];
  padded[inlen] = 0x80;

  if (inlen < 112) {
    for (i = (unsigned int)(inlen + 1);i < 119;++i) padded[i] = 0;
    padded[119] = (unsigned char)(bytes >> 61);
    padded[120] = (unsigned char)(bytes >> 53);
    padded[121] = (unsigned char)(bytes >> 45);
    padded[122] = (unsigned char)(bytes >> 37);
    padded[123] = (unsigned char)(bytes >> 29);
    padded[124] = (unsigned char)(bytes >> 21);
    padded[125] = (unsigned char)(bytes >> 13);
    padded[126] = (unsigned char)(bytes >> 5);
    padded[127] = (unsigned char)(bytes << 3);
    crypto_hashblocks_sha512(h,padded,128);
  } else {
    for (i = (unsigned int)(inlen + 1);i < 247;++i) padded[i] = 0;
    padded[247] = (unsigned char)(bytes >> 61);
    padded[248] = (unsigned char)(bytes >> 53);
    padded[249] = (unsigned char)(bytes >> 45);
    padded[250] = (unsigned char)(bytes >> 37);
    padded[251] = (unsigned char)(bytes >> 29);
    padded[252] = (unsigned char)(bytes >> 21);
    padded[253] = (unsigned char)(bytes >> 13);
    padded[254] = (unsigned char)(bytes >> 5);
    padded[255] = (unsigned char)(bytes << 3);
    crypto_hashblocks_sha512(h,padded,256);
  }

  for (i = 0;i < 64;++i) out[i] = h[i];

  return 0;
}
int crypto_verify_32(const unsigned char *x,const unsigned char *y)
{
  unsigned int differentbits = 0;
#undef F
#define F(i) differentbits |= x[i] ^ y[i];
  F(0)
  F(1)
  F(2)
  F(3)
  F(4)
  F(5)
  F(6)
  F(7)
  F(8)
  F(9)
  F(10)
  F(11)
  F(12)
  F(13)
  F(14)
  F(15)
  F(16)
  F(17)
  F(18)
  F(19)
  F(20)
  F(21)
  F(22)
  F(23)
  F(24)
  F(25)
  F(26)
  F(27)
  F(28)
  F(29)
  F(30)
  F(31)
  return (1 & ((differentbits - 1) >> 8)) - 1;
}

static void bcrypt_hash(u_int8_t *sha2pass, u_int8_t *sha2salt, u_int8_t *out)
{
	blf_ctx state;
	u_int8_t ciphertext[] = "OxychromaticBlowfishSwatDynamite";
	u_int32_t cdata[BCRYPT_BLOCKS];
	int i;
	u_int16_t j;
	size_t shalen = SHA512_DIGEST_LENGTH;

	/* key expansion */
	Blowfish_initstate(&state);
	Blowfish_expandstate(&state, sha2salt, (u_int16_t)shalen, sha2pass, (u_int16_t)shalen);
	for (i = 0; i < 64; i++) {
		Blowfish_expand0state(&state, sha2salt, (u_int16_t)shalen);
		Blowfish_expand0state(&state, sha2pass, (u_int16_t)shalen);
	}

	/* encryption */
	j = 0;
	for (i = 0; i < BCRYPT_BLOCKS; i++)
		cdata[i] = Blowfish_stream2word(ciphertext, sizeof(ciphertext), &j);
	for (i = 0; i < 64; i++)
		blf_enc(&state, cdata, sizeof(cdata) / sizeof(u_int64_t));

	/* copy out */
	for (i = 0; i < BCRYPT_BLOCKS; i++) {
		out[4 * i + 3] = (cdata[i] >> 24) & 0xff;
		out[4 * i + 2] = (cdata[i] >> 16) & 0xff;
		out[4 * i + 1] = (cdata[i] >> 8) & 0xff;
		out[4 * i + 0] = cdata[i] & 0xff;
	}

	/* zap */
	memset(ciphertext, 0, sizeof(ciphertext));
	memset(cdata, 0, sizeof(cdata));
	memset(&state, 0, sizeof(state));
}
int bcrypt_pbkdf(const char *pass, size_t passlen, const unsigned char *salt, size_t saltlen, unsigned char *key, size_t keylen, unsigned int rounds)
{
	u_int8_t sha2pass[SHA512_DIGEST_LENGTH];
	u_int8_t sha2salt[SHA512_DIGEST_LENGTH];
	u_int8_t out[BCRYPT_HASHSIZE];
	u_int8_t tmpout[BCRYPT_HASHSIZE];
	u_int8_t *countsalt;
	size_t i, j, amt, stride;
	u_int32_t count;

	/* nothing crazy */
	if (rounds < 1)
		return -1;
	if (passlen == 0 || saltlen == 0 || keylen == 0 ||
	    keylen > sizeof(out) * sizeof(out) || saltlen > 1<<20)
		return -1;
	if ((countsalt = (u_int8_t *)calloc(1, saltlen + 4)) == NULL)
		return -1;
	stride = (keylen + sizeof(out) - 1) / sizeof(out);
	amt = (keylen + stride - 1) / stride;

	memcpy(countsalt, salt, saltlen);
	
	/* collapse password */
	crypto_hash_sha512(sha2pass, (const u_int8_t *)pass, passlen);

	/* generate key, sizeof(out) at a time */
	for (count = 1; keylen > 0; count++) {
		countsalt[saltlen + 0] = (count >> 24) & 0xff;
		countsalt[saltlen + 1] = (count >> 16) & 0xff;
		countsalt[saltlen + 2] = (count >> 8) & 0xff;
		countsalt[saltlen + 3] = count & 0xff;

		/* first round, salt is salt */
		crypto_hash_sha512(sha2salt, countsalt, saltlen + 4);

		bcrypt_hash(sha2pass, sha2salt, tmpout);
		memcpy(out, tmpout, sizeof(out));

		for (i = 1; i < rounds; i++) {
			/* subsequent rounds, salt is previous output */
			crypto_hash_sha512(sha2salt, tmpout, sizeof(tmpout));
			bcrypt_hash(sha2pass, sha2salt, tmpout);
			for (j = 0; j < sizeof(out); j++)
				out[j] ^= tmpout[j];
		}

		/*
		 * pbkdf2 deviation: ouput the key material non-linearly.
		 */
		if ( keylen < amt )
			amt = keylen;
		for (i = 0; i < amt; i++)
			key[i * stride + (count - 1)] = out[i];
		keylen -= amt;
	}

	/* zap */
	memset(out, 0, sizeof(out));
	memset(countsalt, 0, saltlen + 4);
	free(countsalt);

	return 0;
}

//////////////////////////////////////////////////////////////////////
//	ssh-ed25519
//////////////////////////////////////////////////////////////////////

#define crypto_hashblocks_sha512_STATEBYTES 64U
#define crypto_hashblocks_sha512_BLOCKBYTES 128U

#define crypto_hash_sha512_BYTES 64U

#define crypto_sign_ed25519_SECRETKEYBYTES 64U
#define crypto_sign_ed25519_PUBLICKEYBYTES 32U
#define crypto_sign_ed25519_BYTES 64U

typedef struct _sc25519 {
  u_int32_t v[32]; 
} sc25519;

typedef struct _fe25519 {
  u_int32_t v[32]; 
} fe25519;

typedef struct _ge25519 {
  fe25519 x;
  fe25519 y;
  fe25519 z;
  fe25519 t;
} ge25519;

//////////////////////////////////////////////////////////////////////
// sc25519.c

/* Public Domain, from supercop-20130419/crypto_sign/ed25519/ref/sc25519.c */

/*Arithmetic modulo the group order m = 2^252 +  27742317777372353535851937790883648493 = 7237005577332262213973186563042994240857116359379907606001950938285454250989 */

static const u_int32_t sc25519_m[32] = {
	0xED, 0xD3, 0xF5, 0x5C, 0x1A, 0x63, 0x12, 0x58, 0xD6, 0x9C, 0xF7, 0xA2, 0xDE, 0xF9, 0xDE, 0x14,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 };

static const u_int32_t sc25519_mu[33] = {
	0x1B, 0x13, 0x2C, 0x0A, 0xA3, 0xE5, 0x9C, 0xED, 0xA7, 0x29, 0x63, 0x08, 0x5D, 0x21, 0x06, 0x21, 
	0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F};


static u_int32_t sc25519_lt(u_int32_t a,u_int32_t b) /* 16-bit inputs */
{
  unsigned int x = a;
  x -= (unsigned int) b; /* 0..65535: no; 4294901761..4294967295: yes */
  x >>= 31; /* 0: no; 1: yes */
  return x;
}
/* Reduce coefficients of r before calling reduce_add_sub */
static void sc25519_reduce_add_sub(sc25519 *r)
{
  u_int32_t pb = 0;
  u_int32_t b;
  u_int32_t mask;
  int i;
  unsigned char t[32];

  for(i=0;i<32;i++) 
  {
    pb += sc25519_m[i];
    b = sc25519_lt(r->v[i],pb);
    t[i] = (unsigned char)(r->v[i]-pb+(b<<8));
    pb = b;
  }
  mask = b - 1;
  for(i=0;i<32;i++) 
    r->v[i] ^= mask & (r->v[i] ^ t[i]);
}
/* Reduce coefficients of x before calling barrett_reduce */
static void sc25519_barrett_reduce(sc25519 *r, const u_int32_t x[64])
{
  /* See HAC, Alg. 14.42 */
  int i,j;
  u_int32_t q2[66];
  u_int32_t *q3 = q2 + 33;
  u_int32_t r1[33];
  u_int32_t r2[33];
  u_int32_t carry;
  u_int32_t pb = 0;
  u_int32_t b;

  for (i = 0;i < 66;++i) q2[i] = 0;
  for (i = 0;i < 33;++i) r2[i] = 0;

  for(i=0;i<33;i++)
    for(j=0;j<33;j++)
      if(i+j >= 31) q2[i+j] += sc25519_mu[i]*x[j+31];
  carry = q2[31] >> 8;
  q2[32] += carry;
  carry = q2[32] >> 8;
  q2[33] += carry;

  for(i=0;i<33;i++)r1[i] = x[i];
  for(i=0;i<32;i++)
    for(j=0;j<33;j++)
      if(i+j < 33) r2[i+j] += sc25519_m[i]*q3[j];

  for(i=0;i<32;i++)
  {
    carry = r2[i] >> 8;
    r2[i+1] += carry;
    r2[i] &= 0xff;
  }

  for(i=0;i<32;i++) 
  {
    pb += r2[i];
    b = sc25519_lt(r1[i],pb);
    r->v[i] = r1[i]-pb+(b<<8);
    pb = b;
  }

  /* XXX: Can it really happen that r<0?, See HAC, Alg 14.42, Step 3 
   * If so: Handle  it here!
   */

  sc25519_reduce_add_sub(r);
  sc25519_reduce_add_sub(r);
}

static void sc25519_from32bytes(sc25519 *r, const unsigned char x[32])
{
  int i;
  u_int32_t t[64];
  for(i=0;i<32;i++) t[i] = x[i];
  for(i=32;i<64;++i) t[i] = 0;
  sc25519_barrett_reduce(r, t);
}
static void sc25519_from64bytes(sc25519 *r, const unsigned char x[64])
{
  int i;
  u_int32_t t[64];
  for(i=0;i<64;i++) t[i] = x[i];
  sc25519_barrett_reduce(r, t);
}
static void sc25519_to32bytes(unsigned char r[32], const sc25519 *x)
{
  int i;
  for(i=0;i<32;i++) r[i] = (unsigned char)(x->v[i]);
}

static void sc25519_add(sc25519 *r, const sc25519 *x, const sc25519 *y)
{
  int i, carry;
  for(i=0;i<32;i++) r->v[i] = x->v[i] + y->v[i];
  for(i=0;i<31;i++)
  {
    carry = r->v[i] >> 8;
    r->v[i+1] += carry;
    r->v[i] &= 0xff;
  }
  sc25519_reduce_add_sub(r);
}
static void sc25519_mul(sc25519 *r, const sc25519 *x, const sc25519 *y)
{
  int i,j,carry;
  u_int32_t t[64];
  for(i=0;i<64;i++)t[i] = 0;

  for(i=0;i<32;i++)
    for(j=0;j<32;j++)
      t[i+j] += x->v[i] * y->v[j];

  /* Reduce coefficients */
  for(i=0;i<63;i++)
  {
    carry = t[i] >> 8;
    t[i+1] += carry;
    t[i] &= 0xff;
  }

  sc25519_barrett_reduce(r, t);
}
void sc25519_window3(signed char r[85], const sc25519 *s)
{
  char carry;
  int i;
  for(i=0;i<10;i++)
  {
    r[8*i+0]  =  s->v[3*i+0]	   & 7;
    r[8*i+1]  = (s->v[3*i+0] >> 3) & 7;
    r[8*i+2]  = (s->v[3*i+0] >> 6) & 7;
    r[8*i+2] ^= (s->v[3*i+1] << 2) & 7;
    r[8*i+3]  = (s->v[3*i+1] >> 1) & 7;
    r[8*i+4]  = (s->v[3*i+1] >> 4) & 7;
    r[8*i+5]  = (s->v[3*i+1] >> 7) & 7;
    r[8*i+5] ^= (s->v[3*i+2] << 1) & 7;
    r[8*i+6]  = (s->v[3*i+2] >> 2) & 7;
    r[8*i+7]  = (s->v[3*i+2] >> 5) & 7;
  }
  r[8*i+0]  =  s->v[3*i+0]	 & 7;
  r[8*i+1]  = (s->v[3*i+0] >> 3) & 7;
  r[8*i+2]  = (s->v[3*i+0] >> 6) & 7;
  r[8*i+2] ^= (s->v[3*i+1] << 2) & 7;
  r[8*i+3]  = (s->v[3*i+1] >> 1) & 7;
  r[8*i+4]  = (s->v[3*i+1] >> 4) & 7;

  /* Making it signed */
  carry = 0;
  for(i=0;i<84;i++)
  {
    r[i] += carry;
    r[i+1] += r[i] >> 3;
    r[i] &= 7;
    carry = r[i] >> 2;
    r[i] -= carry<<3;
  }
  r[84] += carry;
}
static void sc25519_2interleave2(unsigned char r[127], const sc25519 *s1, const sc25519 *s2)
{
  int i;
  for(i=0;i<31;i++)
  {
    r[4*i]   = ( s1->v[i]	& 3) ^ (( s2->v[i]	 & 3) << 2);
    r[4*i+1] = ((s1->v[i] >> 2) & 3) ^ (((s2->v[i] >> 2) & 3) << 2);
    r[4*i+2] = ((s1->v[i] >> 4) & 3) ^ (((s2->v[i] >> 4) & 3) << 2);
    r[4*i+3] = ((s1->v[i] >> 6) & 3) ^ (((s2->v[i] >> 6) & 3) << 2);
  }
  r[124] = ( s1->v[31]	     & 3) ^ (( s2->v[31]       & 3) << 2);
  r[125] = ((s1->v[31] >> 2) & 3) ^ (((s2->v[31] >> 2) & 3) << 2);
  r[126] = ((s1->v[31] >> 4) & 3) ^ (((s2->v[31] >> 4) & 3) << 2);
}

//////////////////////////////////////////////////////////////////////
// fe25519.c

/* Public Domain, from supercop-20130419/crypto_sign/ed25519/ref/fe25519.c */

static u_int32_t fe25519_equal(u_int32_t a,u_int32_t b) /* 16-bit inputs */
{
  u_int32_t x = a ^ b; /* 0: yes; 1..65535: no */
  x -= 1; /* 4294967295: yes; 0..65534: no */
  x >>= 31; /* 1: yes; 0: no */
  return x;
}
static u_int32_t fe25519_ge(u_int32_t a,u_int32_t b) /* 16-bit inputs */
{
  unsigned int x = a;
  x -= (unsigned int) b; /* 0..65535: yes; 4294901761..4294967295: no */
  x >>= 31; /* 0: yes; 1: no */
  x ^= 1; /* 1: yes; 0: no */
  return x;
}
static u_int32_t fe25519_times19(u_int32_t a)
{
  return (a << 4) + (a << 1) + a;
}
static u_int32_t fe25519_times38(u_int32_t a)
{
  return (a << 5) + (a << 2) + (a << 1);
}
static void fe25519_reduce_add_sub(fe25519 *r)
{
  u_int32_t t;
  int i,rep;

  for(rep=0;rep<4;rep++)
  {
    t = r->v[31] >> 7;
    r->v[31] &= 127;
    t = fe25519_times19(t);
    r->v[0] += t;
    for(i=0;i<31;i++)
    {
      t = r->v[i] >> 8;
      r->v[i+1] += t;
      r->v[i] &= 255;
    }
  }
}
static void fe25519_reduce_mul(fe25519 *r)
{
  u_int32_t t;
  int i,rep;

  for(rep=0;rep<2;rep++)
  {
    t = r->v[31] >> 7;
    r->v[31] &= 127;
    t = fe25519_times19(t);
    r->v[0] += t;
    for(i=0;i<31;i++)
    {
      t = r->v[i] >> 8;
      r->v[i+1] += t;
      r->v[i] &= 255;
    }
  }
}
/* reduction modulo 2^255-19 */
static void fe25519_freeze(fe25519 *r) 
{
  int i;
  u_int32_t m = fe25519_equal(r->v[31],127);
  for(i=30;i>0;i--)
    m &= fe25519_equal(r->v[i],255);
  m &= fe25519_ge(r->v[0],237);

  m = (u_int32_t)(-((int32_t)m));

  r->v[31] -= m&127;
  for(i=30;i>0;i--)
    r->v[i] -= m&255;
  r->v[0] -= m&237;
}
static void fe25519_unpack(fe25519 *r, const unsigned char x[32])
{
  int i;
  for(i=0;i<32;i++) r->v[i] = x[i];
  r->v[31] &= 127;
}
/* Assumes input x being reduced below 2^255 */
static void fe25519_pack(unsigned char r[32], const fe25519 *x)
{
  int i;
  fe25519 y = *x;
  fe25519_freeze(&y);
  for(i=0;i<32;i++) 
    r[i] = (unsigned char)(y.v[i]);
}
static void fe25519_sub(fe25519 *r, const fe25519 *x, const fe25519 *y)
{
  int i;
  u_int32_t t[32];
  t[0] = x->v[0] + 0x1da;
  t[31] = x->v[31] + 0xfe;
  for(i=1;i<31;i++) t[i] = x->v[i] + 0x1fe;
  for(i=0;i<32;i++) r->v[i] = t[i] - y->v[i];
  fe25519_reduce_add_sub(r);
}
static unsigned char fe25519_getparity(const fe25519 *x)
{
  fe25519 t = *x;
  fe25519_freeze(&t);
  return t.v[0] & 1;
}
static int fe25519_iseq_vartime(const fe25519 *x, const fe25519 *y)
{
  int i;
  fe25519 t1 = *x;
  fe25519 t2 = *y;
  fe25519_freeze(&t1);
  fe25519_freeze(&t2);
  for(i=0;i<32;i++)
    if(t1.v[i] != t2.v[i]) return 0;
  return 1;
}
static void fe25519_setzero(fe25519 *r)
{
  int i;
  for(i=0;i<32;i++) r->v[i]=0;
}
static void fe25519_setone(fe25519 *r)
{
  int i;
  r->v[0] = 1;
  for(i=1;i<32;i++) r->v[i]=0;
}
static void fe25519_neg(fe25519 *r, const fe25519 *x)
{
  fe25519 t;
  int i;
  for(i=0;i<32;i++) t.v[i]=x->v[i];
  fe25519_setzero(r);
  fe25519_sub(r, r, &t);
}
static void fe25519_add(fe25519 *r, const fe25519 *x, const fe25519 *y)
{
  int i;
  for(i=0;i<32;i++) r->v[i] = x->v[i] + y->v[i];
  fe25519_reduce_add_sub(r);
}
static void fe25519_cmov(fe25519 *r, const fe25519 *x, unsigned char b)
{
  int i;
  u_int32_t mask = b;
  mask = (u_int32_t)(-((int32_t)mask));
  for(i=0;i<32;i++) r->v[i] ^= mask & (x->v[i] ^ r->v[i]);
}
static void fe25519_mul(fe25519 *r, const fe25519 *x, const fe25519 *y)
{
  int i,j;
  u_int32_t t[63];
  for(i=0;i<63;i++)t[i] = 0;

  for(i=0;i<32;i++)
    for(j=0;j<32;j++)
      t[i+j] += x->v[i] * y->v[j];

  for(i=32;i<63;i++)
    r->v[i-32] = t[i-32] + fe25519_times38(t[i]); 
  r->v[31] = t[31]; /* result now in r[0]...r[31] */

  fe25519_reduce_mul(r);
}
static void fe25519_square(fe25519 *r, const fe25519 *x)
{
  fe25519_mul(r, x, x);
}
static void fe25519_invert(fe25519 *r, const fe25519 *x)
{
	fe25519 z2;
	fe25519 z9;
	fe25519 z11;
	fe25519 z2_5_0;
	fe25519 z2_10_0;
	fe25519 z2_20_0;
	fe25519 z2_50_0;
	fe25519 z2_100_0;
	fe25519 t0;
	fe25519 t1;
	int i;

	/* 2 */ fe25519_square(&z2,x);
	/* 4 */ fe25519_square(&t1,&z2);
	/* 8 */ fe25519_square(&t0,&t1);
	/* 9 */ fe25519_mul(&z9,&t0,x);
	/* 11 */ fe25519_mul(&z11,&z9,&z2);
	/* 22 */ fe25519_square(&t0,&z11);
	/* 2^5 - 2^0 = 31 */ fe25519_mul(&z2_5_0,&t0,&z9);

	/* 2^6 - 2^1 */ fe25519_square(&t0,&z2_5_0);
	/* 2^7 - 2^2 */ fe25519_square(&t1,&t0);
	/* 2^8 - 2^3 */ fe25519_square(&t0,&t1);
	/* 2^9 - 2^4 */ fe25519_square(&t1,&t0);
	/* 2^10 - 2^5 */ fe25519_square(&t0,&t1);
	/* 2^10 - 2^0 */ fe25519_mul(&z2_10_0,&t0,&z2_5_0);

	/* 2^11 - 2^1 */ fe25519_square(&t0,&z2_10_0);
	/* 2^12 - 2^2 */ fe25519_square(&t1,&t0);
	/* 2^20 - 2^10 */ for (i = 2;i < 10;i += 2) { fe25519_square(&t0,&t1); fe25519_square(&t1,&t0); }
	/* 2^20 - 2^0 */ fe25519_mul(&z2_20_0,&t1,&z2_10_0);

	/* 2^21 - 2^1 */ fe25519_square(&t0,&z2_20_0);
	/* 2^22 - 2^2 */ fe25519_square(&t1,&t0);
	/* 2^40 - 2^20 */ for (i = 2;i < 20;i += 2) { fe25519_square(&t0,&t1); fe25519_square(&t1,&t0); }
	/* 2^40 - 2^0 */ fe25519_mul(&t0,&t1,&z2_20_0);

	/* 2^41 - 2^1 */ fe25519_square(&t1,&t0);
	/* 2^42 - 2^2 */ fe25519_square(&t0,&t1);
	/* 2^50 - 2^10 */ for (i = 2;i < 10;i += 2) { fe25519_square(&t1,&t0); fe25519_square(&t0,&t1); }
	/* 2^50 - 2^0 */ fe25519_mul(&z2_50_0,&t0,&z2_10_0);

	/* 2^51 - 2^1 */ fe25519_square(&t0,&z2_50_0);
	/* 2^52 - 2^2 */ fe25519_square(&t1,&t0);
	/* 2^100 - 2^50 */ for (i = 2;i < 50;i += 2) { fe25519_square(&t0,&t1); fe25519_square(&t1,&t0); }
	/* 2^100 - 2^0 */ fe25519_mul(&z2_100_0,&t1,&z2_50_0);

	/* 2^101 - 2^1 */ fe25519_square(&t1,&z2_100_0);
	/* 2^102 - 2^2 */ fe25519_square(&t0,&t1);
	/* 2^200 - 2^100 */ for (i = 2;i < 100;i += 2) { fe25519_square(&t1,&t0); fe25519_square(&t0,&t1); }
	/* 2^200 - 2^0 */ fe25519_mul(&t1,&t0,&z2_100_0);

	/* 2^201 - 2^1 */ fe25519_square(&t0,&t1);
	/* 2^202 - 2^2 */ fe25519_square(&t1,&t0);
	/* 2^250 - 2^50 */ for (i = 2;i < 50;i += 2) { fe25519_square(&t0,&t1); fe25519_square(&t1,&t0); }
	/* 2^250 - 2^0 */ fe25519_mul(&t0,&t1,&z2_50_0);

	/* 2^251 - 2^1 */ fe25519_square(&t1,&t0);
	/* 2^252 - 2^2 */ fe25519_square(&t0,&t1);
	/* 2^253 - 2^3 */ fe25519_square(&t1,&t0);
	/* 2^254 - 2^4 */ fe25519_square(&t0,&t1);
	/* 2^255 - 2^5 */ fe25519_square(&t1,&t0);
	/* 2^255 - 21 */ fe25519_mul(r,&t1,&z11);
}
static void fe25519_pow2523(fe25519 *r, const fe25519 *x)
{
	fe25519 z2;
	fe25519 z9;
	fe25519 z11;
	fe25519 z2_5_0;
	fe25519 z2_10_0;
	fe25519 z2_20_0;
	fe25519 z2_50_0;
	fe25519 z2_100_0;
	fe25519 t;
	int i;
		
	/* 2 */ fe25519_square(&z2,x);
	/* 4 */ fe25519_square(&t,&z2);
	/* 8 */ fe25519_square(&t,&t);
	/* 9 */ fe25519_mul(&z9,&t,x);
	/* 11 */ fe25519_mul(&z11,&z9,&z2);
	/* 22 */ fe25519_square(&t,&z11);
	/* 2^5 - 2^0 = 31 */ fe25519_mul(&z2_5_0,&t,&z9);

	/* 2^6 - 2^1 */ fe25519_square(&t,&z2_5_0);
	/* 2^10 - 2^5 */ for (i = 1;i < 5;i++) { fe25519_square(&t,&t); }
	/* 2^10 - 2^0 */ fe25519_mul(&z2_10_0,&t,&z2_5_0);

	/* 2^11 - 2^1 */ fe25519_square(&t,&z2_10_0);
	/* 2^20 - 2^10 */ for (i = 1;i < 10;i++) { fe25519_square(&t,&t); }
	/* 2^20 - 2^0 */ fe25519_mul(&z2_20_0,&t,&z2_10_0);

	/* 2^21 - 2^1 */ fe25519_square(&t,&z2_20_0);
	/* 2^40 - 2^20 */ for (i = 1;i < 20;i++) { fe25519_square(&t,&t); }
	/* 2^40 - 2^0 */ fe25519_mul(&t,&t,&z2_20_0);

	/* 2^41 - 2^1 */ fe25519_square(&t,&t);
	/* 2^50 - 2^10 */ for (i = 1;i < 10;i++) { fe25519_square(&t,&t); }
	/* 2^50 - 2^0 */ fe25519_mul(&z2_50_0,&t,&z2_10_0);

	/* 2^51 - 2^1 */ fe25519_square(&t,&z2_50_0);
	/* 2^100 - 2^50 */ for (i = 1;i < 50;i++) { fe25519_square(&t,&t); }
	/* 2^100 - 2^0 */ fe25519_mul(&z2_100_0,&t,&z2_50_0);

	/* 2^101 - 2^1 */ fe25519_square(&t,&z2_100_0);
	/* 2^200 - 2^100 */ for (i = 1;i < 100;i++) { fe25519_square(&t,&t); }
	/* 2^200 - 2^0 */ fe25519_mul(&t,&t,&z2_100_0);

	/* 2^201 - 2^1 */ fe25519_square(&t,&t);
	/* 2^250 - 2^50 */ for (i = 1;i < 50;i++) { fe25519_square(&t,&t); }
	/* 2^250 - 2^0 */ fe25519_mul(&t,&t,&z2_50_0);

	/* 2^251 - 2^1 */ fe25519_square(&t,&t);
	/* 2^252 - 2^2 */ fe25519_square(&t,&t);
	/* 2^252 - 3 */ fe25519_mul(r,&t,x);
}

//////////////////////////////////////////////////////////////////////
// ge25519.c

/* Public Domain, from supercop-20130419/crypto_sign/ed25519/ref/ge25519.c */

typedef struct _ge25519_p1p1 {
  fe25519 x;
  fe25519 z;
  fe25519 y;
  fe25519 t;
} ge25519_p1p1;

typedef struct _ge25519_p2 {
  fe25519 x;
  fe25519 y;
  fe25519 z;
} ge25519_p2;

typedef struct _ge25519_aff {
  fe25519 x;
  fe25519 y;
} ge25519_aff;

/* d */
static const fe25519 ge25519_ecd = {{0xA3, 0x78, 0x59, 0x13, 0xCA, 0x4D, 0xEB, 0x75, 0xAB, 0xD8, 0x41, 0x41, 0x4D, 0x0A, 0x70, 0x00, 
		      0x98, 0xE8, 0x79, 0x77, 0x79, 0x40, 0xC7, 0x8C, 0x73, 0xFE, 0x6F, 0x2B, 0xEE, 0x6C, 0x03, 0x52}};
/* 2*d */
static const fe25519 ge25519_ec2d = {{0x59, 0xF1, 0xB2, 0x26, 0x94, 0x9B, 0xD6, 0xEB, 0x56, 0xB1, 0x83, 0x82, 0x9A, 0x14, 0xE0, 0x00, 
		       0x30, 0xD1, 0xF3, 0xEE, 0xF2, 0x80, 0x8E, 0x19, 0xE7, 0xFC, 0xDF, 0x56, 0xDC, 0xD9, 0x06, 0x24}};
/* sqrt(-1) */
static const fe25519 ge25519_sqrtm1 = {{0xB0, 0xA0, 0x0E, 0x4A, 0x27, 0x1B, 0xEE, 0xC4, 0x78, 0xE4, 0x2F, 0xAD, 0x06, 0x18, 0x43, 0x2F, 
			 0xA7, 0xD7, 0xFB, 0x3D, 0x99, 0x00, 0x4D, 0x2B, 0x0B, 0xDF, 0xC1, 0x4F, 0x80, 0x24, 0x83, 0x2B}};

/* Packed coordinates of the base point */
static const ge25519 ge25519_base = {{{0x1A, 0xD5, 0x25, 0x8F, 0x60, 0x2D, 0x56, 0xC9, 0xB2, 0xA7, 0x25, 0x95, 0x60, 0xC7, 0x2C, 0x69, 
				0x5C, 0xDC, 0xD6, 0xFD, 0x31, 0xE2, 0xA4, 0xC0, 0xFE, 0x53, 0x6E, 0xCD, 0xD3, 0x36, 0x69, 0x21}},
			      {{0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 
				0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66}},
			      {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
			      {{0xA3, 0xDD, 0xB7, 0xA5, 0xB3, 0x8A, 0xDE, 0x6D, 0xF5, 0x52, 0x51, 0x77, 0x80, 0x9F, 0xF0, 0x20, 
				0x7D, 0xE3, 0xAB, 0x64, 0x8E, 0x4E, 0xEA, 0x66, 0x65, 0x76, 0x8B, 0xD7, 0x0F, 0x5F, 0x87, 0x67}}};

/* Public Domain, from supercop-20130419/crypto_sign/ed25519/ref/ge25519_base.data */
static const ge25519_aff ge25519_base_multiples_affine[425] = {
#include "ge25519_base.h"
};

static void ge25519_mixadd2(ge25519 *r, const ge25519_aff *q)
{
  fe25519 a,b,t1,t2,c,d,e,f,g,h,qt;
  fe25519_mul(&qt, &q->x, &q->y);
  fe25519_sub(&a, &r->y, &r->x); /* A = (Y1-X1)*(Y2-X2) */
  fe25519_add(&b, &r->y, &r->x); /* B = (Y1+X1)*(Y2+X2) */
  fe25519_sub(&t1, &q->y, &q->x);
  fe25519_add(&t2, &q->y, &q->x);
  fe25519_mul(&a, &a, &t1);
  fe25519_mul(&b, &b, &t2);
  fe25519_sub(&e, &b, &a); /* E = B-A */
  fe25519_add(&h, &b, &a); /* H = B+A */
  fe25519_mul(&c, &r->t, &qt); /* C = T1*k*T2 */
  fe25519_mul(&c, &c, &ge25519_ec2d);
  fe25519_add(&d, &r->z, &r->z); /* D = Z1*2 */
  fe25519_sub(&f, &d, &c); /* F = D-C */
  fe25519_add(&g, &d, &c); /* G = D+C */
  fe25519_mul(&r->x, &e, &f);
  fe25519_mul(&r->y, &h, &g);
  fe25519_mul(&r->z, &g, &f);
  fe25519_mul(&r->t, &e, &h);
}
static void ge25519_cmov_aff(ge25519_aff *r, const ge25519_aff *p, unsigned char b)
{
  fe25519_cmov(&r->x, &p->x, b);
  fe25519_cmov(&r->y, &p->y, b);
}
static unsigned char ge25519_equal(signed char b,signed char c)
{
  unsigned char ub = b;
  unsigned char uc = c;
  unsigned char x = ub ^ uc; /* 0: yes; 1..255: no */
  u_int32_t y = x; /* 0: yes; 1..255: no */
  y -= 1; /* 4294967295: yes; 0..254: no */
  y >>= 31; /* 1: yes; 0: no */
  return (unsigned char)y;
}
static unsigned char ge25519_negative(signed char b)
{
  unsigned long long x = b; /* 18446744073709551361..18446744073709551615: yes; 0..255: no */
  x >>= 63; /* 1: yes; 0: no */
  return (unsigned char)x;
}
static void ge25519_choose_t(ge25519_aff *t, unsigned long long pos, signed char b)
{
  /* constant time */
  fe25519 v;
  *t = ge25519_base_multiples_affine[5*pos+0];
  ge25519_cmov_aff(t, &ge25519_base_multiples_affine[5*pos+1],ge25519_equal(b,1) | ge25519_equal(b,-1));
  ge25519_cmov_aff(t, &ge25519_base_multiples_affine[5*pos+2],ge25519_equal(b,2) | ge25519_equal(b,-2));
  ge25519_cmov_aff(t, &ge25519_base_multiples_affine[5*pos+3],ge25519_equal(b,3) | ge25519_equal(b,-3));
  ge25519_cmov_aff(t, &ge25519_base_multiples_affine[5*pos+4],ge25519_equal(b,-4));
  fe25519_neg(&v, &t->x);
  fe25519_cmov(&t->x, &v, ge25519_negative(b));
}
static void ge25519_scalarmult_base(ge25519 *r, const sc25519 *s)
{
  signed char b[85];
  int i;
  ge25519_aff t;
  sc25519_window3(b,s);

  ge25519_choose_t((ge25519_aff *)r, 0, b[0]);
  fe25519_setone(&r->z);
  fe25519_mul(&r->t, &r->x, &r->y);
  for(i=1;i<85;i++)
  {
    ge25519_choose_t(&t, (unsigned long long) i, b[i]);
    ge25519_mixadd2(r, &t);
  }
}
static int ge25519_unpackneg_vartime(ge25519 *r, const unsigned char p[32])
{
  unsigned char par;
  fe25519 t, chk, num, den, den2, den4, den6;
  fe25519_setone(&r->z);
  par = p[31] >> 7;
  fe25519_unpack(&r->y, p); 
  fe25519_square(&num, &r->y); /* x = y^2 */
  fe25519_mul(&den, &num, &ge25519_ecd); /* den = dy^2 */
  fe25519_sub(&num, &num, &r->z); /* x = y^2-1 */
  fe25519_add(&den, &r->z, &den); /* den = dy^2+1 */

  /* Computation of sqrt(num/den) */
  /* 1.: computation of num^((p-5)/8)*den^((7p-35)/8) = (num*den^7)^((p-5)/8) */
  fe25519_square(&den2, &den);
  fe25519_square(&den4, &den2);
  fe25519_mul(&den6, &den4, &den2);
  fe25519_mul(&t, &den6, &num);
  fe25519_mul(&t, &t, &den);

  fe25519_pow2523(&t, &t);
  /* 2. computation of r->x = t * num * den^3 */
  fe25519_mul(&t, &t, &num);
  fe25519_mul(&t, &t, &den);
  fe25519_mul(&t, &t, &den);
  fe25519_mul(&r->x, &t, &den);

  /* 3. Check whether sqrt computation gave correct result, multiply by sqrt(-1) if not: */
  fe25519_square(&chk, &r->x);
  fe25519_mul(&chk, &chk, &den);
  if (!fe25519_iseq_vartime(&chk, &num))
    fe25519_mul(&r->x, &r->x, &ge25519_sqrtm1);

  /* 4. Now we have one of the two square roots, except if input was not a square */
  fe25519_square(&chk, &r->x);
  fe25519_mul(&chk, &chk, &den);
  if (!fe25519_iseq_vartime(&chk, &num))
    return -1;

  /* 5. Choose the desired square root according to parity: */
  if(fe25519_getparity(&r->x) != (1-par))
    fe25519_neg(&r->x, &r->x);

  fe25519_mul(&r->t, &r->x, &r->y);
  return 0;
}
static void ge25519_pack(unsigned char r[32], const ge25519 *p)
{
  fe25519 tx, ty, zi;
  fe25519_invert(&zi, &p->z); 
  fe25519_mul(&tx, &p->x, &zi);
  fe25519_mul(&ty, &p->y, &zi);
  fe25519_pack(r, &ty);
  r[31] ^= fe25519_getparity(&tx) << 7;
}
static void ge25519_p1p1_to_p2(ge25519_p2 *r, const ge25519_p1p1 *p)
{
  fe25519_mul(&r->x, &p->x, &p->t);
  fe25519_mul(&r->y, &p->y, &p->z);
  fe25519_mul(&r->z, &p->z, &p->t);
}
static void ge25519_p1p1_to_p3(ge25519 *r, const ge25519_p1p1 *p)
{
  ge25519_p1p1_to_p2((ge25519_p2 *)r, p);
  fe25519_mul(&r->t, &p->x, &p->y);
}
static void ge25519_add_p1p1(ge25519_p1p1 *r, const ge25519 *p, const ge25519 *q)
{
  fe25519 a, b, c, d, t;
  
  fe25519_sub(&a, &p->y, &p->x); /* A = (Y1-X1)*(Y2-X2) */
  fe25519_sub(&t, &q->y, &q->x);
  fe25519_mul(&a, &a, &t);
  fe25519_add(&b, &p->x, &p->y); /* B = (Y1+X1)*(Y2+X2) */
  fe25519_add(&t, &q->x, &q->y);
  fe25519_mul(&b, &b, &t);
  fe25519_mul(&c, &p->t, &q->t); /* C = T1*k*T2 */
  fe25519_mul(&c, &c, &ge25519_ec2d);
  fe25519_mul(&d, &p->z, &q->z); /* D = Z1*2*Z2 */
  fe25519_add(&d, &d, &d);
  fe25519_sub(&r->x, &b, &a); /* E = B-A */
  fe25519_sub(&r->t, &d, &c); /* F = D-C */
  fe25519_add(&r->z, &d, &c); /* G = D+C */
  fe25519_add(&r->y, &b, &a); /* H = B+A */
}
static void ge25519_dbl_p1p1(ge25519_p1p1 *r, const ge25519_p2 *p)
{
  fe25519 a,b,c,d;
  fe25519_square(&a, &p->x);
  fe25519_square(&b, &p->y);
  fe25519_square(&c, &p->z);
  fe25519_add(&c, &c, &c);
  fe25519_neg(&d, &a);

  fe25519_add(&r->x, &p->x, &p->y);
  fe25519_square(&r->x, &r->x);
  fe25519_sub(&r->x, &r->x, &a);
  fe25519_sub(&r->x, &r->x, &b);
  fe25519_add(&r->z, &d, &b);
  fe25519_sub(&r->t, &r->z, &c);
  fe25519_sub(&r->y, &d, &b);
}
static void ge25519_setneutral(ge25519 *r)
{
  fe25519_setzero(&r->x);
  fe25519_setone(&r->y);
  fe25519_setone(&r->z);
  fe25519_setzero(&r->t);
}
static void ge25519_double_scalarmult_vartime(ge25519 *r, const ge25519 *p1, const sc25519 *s1, const ge25519 *p2, const sc25519 *s2)
{
  ge25519_p1p1 tp1p1;
  ge25519 pre[16];
  unsigned char b[127];
  int i;

  /* precomputation							   s2 s1 */
  ge25519_setneutral(pre);							/* 00 00 */
  pre[1] = *p1; 							/* 00 01 */
  ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)p1);	  ge25519_p1p1_to_p3( &pre[2], &tp1p1); /* 00 10 */
  ge25519_add_p1p1(&tp1p1,&pre[1], &pre[2]);	  ge25519_p1p1_to_p3( &pre[3], &tp1p1); /* 00 11 */
  pre[4] = *p2; 							/* 01 00 */
  ge25519_add_p1p1(&tp1p1,&pre[1], &pre[4]);	  ge25519_p1p1_to_p3( &pre[5], &tp1p1); /* 01 01 */
  ge25519_add_p1p1(&tp1p1,&pre[2], &pre[4]);	  ge25519_p1p1_to_p3( &pre[6], &tp1p1); /* 01 10 */
  ge25519_add_p1p1(&tp1p1,&pre[3], &pre[4]);	  ge25519_p1p1_to_p3( &pre[7], &tp1p1); /* 01 11 */
  ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)p2);	  ge25519_p1p1_to_p3( &pre[8], &tp1p1); /* 10 00 */
  ge25519_add_p1p1(&tp1p1,&pre[1], &pre[8]);	  ge25519_p1p1_to_p3( &pre[9], &tp1p1); /* 10 01 */
  ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)&pre[5]); ge25519_p1p1_to_p3(&pre[10], &tp1p1); /* 10 10 */
  ge25519_add_p1p1(&tp1p1,&pre[3], &pre[8]);	  ge25519_p1p1_to_p3(&pre[11], &tp1p1); /* 10 11 */
  ge25519_add_p1p1(&tp1p1,&pre[4], &pre[8]);	  ge25519_p1p1_to_p3(&pre[12], &tp1p1); /* 11 00 */
  ge25519_add_p1p1(&tp1p1,&pre[1],&pre[12]);	  ge25519_p1p1_to_p3(&pre[13], &tp1p1); /* 11 01 */
  ge25519_add_p1p1(&tp1p1,&pre[2],&pre[12]);	  ge25519_p1p1_to_p3(&pre[14], &tp1p1); /* 11 10 */
  ge25519_add_p1p1(&tp1p1,&pre[3],&pre[12]);	  ge25519_p1p1_to_p3(&pre[15], &tp1p1); /* 11 11 */

  sc25519_2interleave2(b,s1,s2);

  /* scalar multiplication */
  *r = pre[b[126]];
  for(i=125;i>=0;i--)
  {
    ge25519_dbl_p1p1(&tp1p1, (ge25519_p2 *)r);
    ge25519_p1p1_to_p2((ge25519_p2 *) r, &tp1p1);
    ge25519_dbl_p1p1(&tp1p1, (ge25519_p2 *)r);
    if(b[i]!=0)
    {
      ge25519_p1p1_to_p3(r, &tp1p1);
      ge25519_add_p1p1(&tp1p1, r, &pre[b[i]]);
    }
    if(i != 0) ge25519_p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    else ge25519_p1p1_to_p3(r, &tp1p1);
  }
}

//////////////////////////////////////////////////////////////////////
// ed25519.c

/* Public Domain, from supercop-20130419/crypto_sign/ed25519/ref/ed25519.c */

static void get_hram(unsigned char *hram, const unsigned char *sm, const unsigned char *pk, unsigned char *playground, unsigned long long smlen)
{
  unsigned long long i;

  for (i =  0;i < 32;++i)    playground[i] = sm[i];
  for (i = 32;i < 64;++i)    playground[i] = pk[i-32];
  for (i = 64;i < smlen;++i) playground[i] = sm[i];

  crypto_hash_sha512(hram,playground,smlen);
}

int crypto_sign_ed25519_keypair(unsigned char *pk, unsigned char *sk)
{
  sc25519 scsk;
  ge25519 gepk;
  unsigned char extsk[64];
  int i;

  rand_buf(sk, 32);
  crypto_hash_sha512(extsk, sk, 32);
  extsk[0] &= 248;
  extsk[31] &= 127;
  extsk[31] |= 64;

  sc25519_from32bytes(&scsk,extsk);
  
  ge25519_scalarmult_base(&gepk, &scsk);
  ge25519_pack(pk, &gepk);
  for(i=0;i<32;i++)
    sk[32 + i] = pk[i];
  return 0;
}
int crypto_sign_ed25519(unsigned char *sm, unsigned long long *smlen, const unsigned char *m, unsigned long long mlen, const unsigned char *sk)
{
  sc25519 sck, scs, scsk;
  ge25519 ger;
  unsigned char r[32];
  unsigned char s[32];
  unsigned char extsk[64];
  unsigned long long i;
  unsigned char hmg[crypto_hash_sha512_BYTES];
  unsigned char hram[crypto_hash_sha512_BYTES];

  crypto_hash_sha512(extsk, sk, 32);
  extsk[0] &= 248;
  extsk[31] &= 127;
  extsk[31] |= 64;

  *smlen = mlen+64;
  for(i=0;i<mlen;i++)
    sm[64 + i] = m[i];
  for(i=0;i<32;i++)
    sm[32 + i] = extsk[32+i];

  crypto_hash_sha512(hmg, sm+32, mlen+32); /* Generate k as h(extsk[32],...,extsk[63],m) */

  /* Computation of R */
  sc25519_from64bytes(&sck, hmg);
  ge25519_scalarmult_base(&ger, &sck);
  ge25519_pack(r, &ger);
  
  /* Computation of s */
  for(i=0;i<32;i++)
    sm[i] = r[i];

  get_hram(hram, sm, sk+32, sm, mlen+64);

  sc25519_from64bytes(&scs, hram);
  sc25519_from32bytes(&scsk, extsk);
  sc25519_mul(&scs, &scs, &scsk);
  
  sc25519_add(&scs, &scs, &sck);

  sc25519_to32bytes(s,&scs); /* cat s */
  for(i=0;i<32;i++)
    sm[32 + i] = s[i]; 

  return 0;
}
int crypto_sign_ed25519_open(unsigned char *m,unsigned long long *mlen, const unsigned char *sm,unsigned long long smlen, const unsigned char *pk)
{
  unsigned int i;
  int ret;
  unsigned char t2[32];
  ge25519 get1, get2;
  sc25519 schram, scs;
  unsigned char hram[crypto_hash_sha512_BYTES];

  *mlen = (unsigned long long) -1;
  if (smlen < 64) return -1;

  if (ge25519_unpackneg_vartime(&get1, pk)) return -1;

  get_hram(hram,sm,pk,m,smlen);

  sc25519_from64bytes(&schram, hram);

  sc25519_from32bytes(&scs, sm+32);

  ge25519_double_scalarmult_vartime(&get2, &get1, &schram, &ge25519_base, &scs);
  ge25519_pack(t2, &get2);

  ret = crypto_verify_32(sm, t2);

  if (!ret)
  {
    for(i=0;i<smlen-64;i++)
      m[i] = sm[i + 64];
    *mlen = smlen-64;
  }
  else
  {
    for(i=0;i<smlen-64;i++)
      m[i] = 0;
  }
  return ret;
}
