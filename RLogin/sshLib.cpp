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
#include "openssl/sha.h"

extern "C" {
	#include "crypto/chacha.h"
	#include "crypto/poly1305.h"
}

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

//////////////////////////////////////////////////////////////////////

struct prov_ctx_st {
	const OSSL_CORE_HANDLE	*handle;
	const OSSL_DISPATCH		*in;
	unsigned int status;
};

enum NIDS_cipher {
	Nids_aes_ctr,
	Nids_seed_ctr,
	Nids_bf_ctr,
	Nids_cast5_ctr,
	Nids_idea_ctr,
	Nids_des3_ctr,
#ifdef	USE_NETTLE
	Nids_twofish_ctr,
	Nids_twofish_cbc,
	Nids_serpent_ctr,
	Nids_serpent_cbc,
#endif
#ifdef	USE_CLEFIA
	Nids_clefia_ctr,
	Nids_clefia_cbc,	
#endif
	Nids_ssh1_3des,
	Nids_ssh1_bf,
	Nids_chachapoly_256,

	NIDSCIPHERMAX
};

static const EVP_CIPHER *rlg_evp_cipver_fetch(int nids);

//////////////////////////////////////////////////////////////////////

#define	MT_BUFSIZE		(64 * 1024)

struct	mt_ctr_ctx {
	int			top;
	int			pos;
	int			max;
	volatile int len;
	void		*ctx;
	void		(*enc)(void *ctx, u_char *buf);
	int			blk;
	u_char		buf[MT_BUFSIZE];
};

BOOL mt_proc(LPVOID pParam)
{
	BOOL ret = FALSE;
	struct mt_ctr_ctx *c = (struct mt_ctr_ctx *)pParam;

	for ( int i = 0 ; i < 256 ; i++ ) {
		if ( c->len >= c->max )
			break;
		(*(c->enc))(c->ctx, c->buf + c->blk * 2 * c->top);
		c->top = (c->top + 1) % c->max;
		c->len++;
		ret = TRUE;
	}

	return ret;
}

static struct mt_ctr_ctx *mt_init(void *ctx, void (*enc)(void *ctx, u_char *buf), int blk)
{
	struct mt_ctr_ctx *c = new struct mt_ctr_ctx;

	c->top = c->pos = 0;
	c->len = 0;
	c->max = MT_BUFSIZE / (blk * 2);
	c->ctx = ctx;
	c->enc = enc;
	c->blk = blk;

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
		(*(c->enc))(c->ctx, c->buf + c->blk * 2 * c->top);
		c->top = (c->top + 1) % c->max;
		c->len++;
	}
	return (c->buf + c->blk * 2 * c->pos);
}
static u_char *mt_get_iv(struct mt_ctr_ctx *c)
{
	if ( c->len == 0 )
		return NULL;

	return (c->buf + c->blk * 2 * c->pos + c->blk);
}
static void mt_inc_buf(struct mt_ctr_ctx *c)
{
	c->pos = (c->pos + 1) % c->max;
	c->len--;
}
static void mt_reset(struct mt_ctr_ctx *c)
{
	c->top = c->pos = 0;
	c->len = 0;
}

//////////////////////////////////////////////////////////////////////

struct ssh_cipher_ctx
{
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |
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
static int rlg_cipher_ctr(struct mt_ctr_ctx *mt_ctx, u_char *dest, const u_char *src, size_t len)
{
	register int i;
	register size_t *spdst, *spsrc, *spwrk;
	register u_char *wrk;

	if ( mt_ctx == NULL )
		return 0;

	ASSERT(mt_ctx->blk == 8 || mt_ctx->blk == 16);
	ASSERT(sizeof(size_t) == 4 || sizeof(size_t) == 8);

	spdst = (size_t *)dest;
	spsrc = (size_t *)src;

	while ( len >= (size_t)mt_ctx->blk ) {
		spwrk = (size_t *)mt_get_buf(mt_ctx);

		//for ( i = ctx->cipher->block_size / sizeof(size_t) ; i > 0 ; i-- )
		//	*(spdst++) = *(spsrc++) ^ *(spwrk++);

		switch(mt_ctx->blk / sizeof(size_t)) {
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

		len -= mt_ctx->blk;
		mt_inc_buf(mt_ctx);
	}

	if ( len > 0 ) {
		spwrk = (size_t *)mt_get_buf(mt_ctx);

		for ( i = (int)(len / sizeof(size_t)) ; i > 0 ; i-- )
			*(spdst++) = *(spsrc++) ^ *(spwrk++);

		dest = (u_char *)spdst;
		src  = (u_char *)spsrc;
		wrk  = (u_char *)spwrk;

		for ( i = len % sizeof(size_t) ; i > 0 ; i-- )
			*(dest++) = *(src++) ^ *(wrk++);

		mt_inc_buf(mt_ctx);
	}

	return (1);
}

//////////////////////////////////////////////////////////////////////

#define	PARAM_AEAD_FLAG				0001
#define	PARAM_CUSTOM_IV_FLAG		0002
#define	PARAM_CTS_FLAG				0004
#define	PARAM_TLS1_MULTIBLOCK_FLAG	0010
#define	PARAM_HAS_RAND_KEY_FLAG		0020

static int ssh_ctr_cipher(struct ssh_cipher_ctx *ssh_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	if ( ssh_ctx == NULL || out == NULL || outl == NULL || in == NULL )
		return 0;

	if ( rlg_cipher_ctr(ssh_ctx->mt_ctx, out, in, inl) == 0 )
		return 0;

	*outl = inl;

	return 1;
}
static int ssh_ctr_update(struct ssh_cipher_ctx *ssh_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	return ssh_ctr_cipher(ssh_ctx, out, outl, outsize, in, inl);
}
static int ssh_ctr_final(void *cctx, unsigned char *out, size_t *outl, size_t outsize)
{
	return 1;
}
static int rlg_get_params(OSSL_PARAM params[], unsigned int mode, size_t blklen, size_t keylen, size_t ivlen, int flag)
{
	OSSL_PARAM *p;
	int rc = 1;

	for ( p = params ; p->data_type != 0 ; p++ ) {
		switch((p->data_type << 6) | p->key[0]) {
		case (OSSL_PARAM_INTEGER << 6) | 'a':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_AEAD) == 0 )
				OSSL_PARAM_set_int(p, (flag & PARAM_AEAD_FLAG) != 0);
			else
				rc = 0;
			break;
		case (OSSL_PARAM_INTEGER << 6) | 'c':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_CUSTOM_IV) == 0 )
				OSSL_PARAM_set_int(p, (flag & PARAM_CUSTOM_IV_FLAG) != 0);
			else if ( strcmp(p->key, OSSL_CIPHER_PARAM_CTS) == 0 )
				OSSL_PARAM_set_int(p, (flag & PARAM_CTS_FLAG) != 0);
			else
				rc = 0;
			break;
		case (OSSL_PARAM_INTEGER << 6) | 'h':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_HAS_RAND_KEY) == 0 )
				OSSL_PARAM_set_int(p, (flag & PARAM_HAS_RAND_KEY_FLAG) != 0);
			else
				rc = 0;
			break;
		case (OSSL_PARAM_INTEGER << 6) | 't':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_TLS1_MULTIBLOCK) == 0 )
				OSSL_PARAM_set_int(p, (flag & PARAM_TLS1_MULTIBLOCK_FLAG) != 0);
			else
				rc = 0;
			break;

		case (OSSL_PARAM_UNSIGNED_INTEGER << 6) | 'b':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_BLOCK_SIZE) == 0 )
				OSSL_PARAM_set_size_t(p, blklen);
			else
				rc = 0;
			break;
		case (OSSL_PARAM_UNSIGNED_INTEGER << 6) | 'i':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_IVLEN) == 0 )
				OSSL_PARAM_set_size_t(p, ivlen);
			else
				rc = 0;
			break;
		case (OSSL_PARAM_UNSIGNED_INTEGER << 6) | 'k':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_KEYLEN) == 0 )
				OSSL_PARAM_set_size_t(p, keylen);
			else
				rc = 0;
			break;
		case (OSSL_PARAM_UNSIGNED_INTEGER << 6) | 'm':
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_MODE) == 0 )
				OSSL_PARAM_set_uint(p, mode);
			else
				rc = 0;
			break;
		default:
			rc = 0;
			break;
		}
	}

	ASSERT(rc == 1);
	return rc;
}
static const OSSL_PARAM *ssh_gettable_params(struct prov_ctx_st *prov_ctx)
{
	static OSSL_PARAM params[] = {
		OSSL_PARAM_int(OSSL_CIPHER_PARAM_AEAD, 0),
		OSSL_PARAM_int(OSSL_CIPHER_PARAM_CUSTOM_IV, 0),
		OSSL_PARAM_int(OSSL_CIPHER_PARAM_CTS, 0),
		OSSL_PARAM_int(OSSL_CIPHER_PARAM_HAS_RAND_KEY, 0),
		OSSL_PARAM_int(OSSL_CIPHER_PARAM_TLS1_MULTIBLOCK, 0),
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_BLOCK_SIZE, 0),
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_IVLEN, 0),
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_KEYLEN, 0),
		OSSL_PARAM_uint(OSSL_CIPHER_PARAM_MODE, 0),
		OSSL_PARAM_END,
	};

	return params;
}
static int ssh_get_ctx_params(struct ssh_cipher_ctx *ssh_ctx, OSSL_PARAM params[])
{
	OSSL_PARAM *p;
	int rc = 1;

	for ( p = params ; p->data_type != 0 ; p++ ) {
		switch(p->data_type) {
		case OSSL_PARAM_UNSIGNED_INTEGER:
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_KEYLEN) == 0 )
				OSSL_PARAM_set_size_t(p, ssh_ctx->keylen);
			else if ( strcmp(p->key, OSSL_CIPHER_PARAM_IVLEN) == 0 )
				OSSL_PARAM_set_size_t(p, ssh_ctx->ivlen);
			else
				rc = 0;
			break;

		case OSSL_PARAM_OCTET_STRING:
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_IV) == 0 ) {
				u_char *pIv;
				if ( ssh_ctx->mt_ctx == NULL || (pIv = mt_get_iv(ssh_ctx->mt_ctx)) == NULL )
					pIv = ssh_ctx->iv;
				OSSL_PARAM_set_octet_string(p, pIv, ssh_ctx->ivlen);
			} else
				rc = 0;
			break;
		default:
			rc = 0;
			break;
		}
	}

	ASSERT(rc == 1);
	return rc;
}
static const OSSL_PARAM *ssh_gettable_ctx_params(struct ssh_cipher_ctx *ssh_ctx, struct prov_ctx_st *prov_ctx)
{
	static OSSL_PARAM params[] = {
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_KEYLEN, 0),
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_IVLEN, 0),
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_IV, NULL, 0),
		OSSL_PARAM_END,
	};

	return params;
}
static int ssh_set_ctx_params(struct ssh_cipher_ctx *ssh_ctx, const OSSL_PARAM params[])
{
	const OSSL_PARAM *p;
	int rc = 1;

	for ( p = params ; p->data_type != 0 ; p++ ) {
		switch(p->data_type) {
		case OSSL_PARAM_UNSIGNED_INTEGER:
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_KEYLEN) == 0 )
				OSSL_PARAM_get_size_t(p, &ssh_ctx->keylen);
			else if ( strcmp(p->key, OSSL_CIPHER_PARAM_IVLEN) == 0 )
				OSSL_PARAM_get_size_t(p, &ssh_ctx->ivlen);
			else
				rc = 0;
			break;
		default:
			rc = 0;
			break;
		}
	}

	ASSERT(rc == 1);
	return rc;
}
static const OSSL_PARAM *ssh_settable_ctx_params(struct ssh_cipher_ctx *ssh_ctx, struct prov_ctx_st *prov_ctx)
{
	static OSSL_PARAM params[] = {
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_KEYLEN, 0),
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_IVLEN, 0),
		OSSL_PARAM_END,
	};

	return params;
}

//////////////////////////////////////////////////////////////////////
// aes-ctr mt_ctx test code

#define	AES_BLKSIZE		AES_BLOCK_SIZE
#define	AES_KEYSIZE		32
#define	AES_IVSIZE		AES_BLOCK_SIZE

struct aes_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	AES_KEY				aes_key;
};
static void aes_encode(void *ctx, u_char *buf)
{
	struct aes_ctx_st *aes_ctx = (struct aes_ctx_st *)ctx;

	memcpy(buf + AES_BLKSIZE, aes_ctx->iv, AES_BLKSIZE);

	AES_encrypt(aes_ctx->iv, buf, &aes_ctx->aes_key);
	ssh_ctr_inc(aes_ctx->iv, AES_IVSIZE);
}
static void *aes_newctx(void *prov_ctx)
{
	struct aes_ctx_st *aes_ctx;
		
	if ( (aes_ctx = (struct aes_ctx_st *)malloc(sizeof(struct aes_ctx_st))) == NULL )
		return NULL;

	memset(aes_ctx, 0, sizeof(struct aes_ctx_st));

	aes_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	aes_ctx->keylen = AES_KEYSIZE;
	aes_ctx->ivlen  = AES_IVSIZE;

	return aes_ctx;
}
static void aes_freectx(struct aes_ctx_st *aes_ctx)
{
	mt_finis(aes_ctx->mt_ctx);
	memset(aes_ctx, 0, sizeof(struct aes_ctx_st));
	free(aes_ctx);
}
struct aes_ctx_st *aes_dupctx(struct aes_ctx_st *aes_ctx)
{
	struct aes_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct aes_ctx_st *)malloc(sizeof(struct aes_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, aes_ctx, sizeof(struct aes_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int aes_init(struct aes_ctx_st *aes_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen > AES_KEYSIZE )
			keylen = AES_KEYSIZE;
		AES_set_encrypt_key(key, (int)(keylen * 8), &aes_ctx->aes_key);
	}

	if ( iv == NULL && aes_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(aes_ctx->mt_ctx);
		ivlen = AES_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= AES_IVSIZE);

		if ( ivlen > AES_IVSIZE )
			ivlen = AES_IVSIZE;
		memcpy(aes_ctx->iv, iv, ivlen);
		if ( ivlen < AES_IVSIZE )
			memset(aes_ctx->iv + ivlen, 0, AES_IVSIZE - ivlen);
	}

	if ( aes_ctx->mt_ctx == NULL )
		aes_ctx->mt_ctx = mt_init((void *)aes_ctx, aes_encode, AES_BLKSIZE);
	else if ( key != NULL || iv != NULL )
		mt_reset(aes_ctx->mt_ctx);

	return 1;
}
static int aes_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, AES_BLKSIZE, AES_KEYSIZE, AES_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH aes_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))aes_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))aes_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))aes_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))aes_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))aes_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))aes_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_aes_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_aes_ctr);
}

//////////////////////////////////////////////////////////////////////

#define	SEED_BLKSIZE	SEED_BLOCK_SIZE
#define	SEED_KEYSIZE	SEED_KEY_LENGTH
#define	SEED_IVSIZE		SEED_BLOCK_SIZE

struct seed_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	SEED_KEY_SCHEDULE	seed_key;
};
static void seed_encode(void *ctx, u_char *buf)
{
	struct seed_ctx_st *seed_ctx = (struct seed_ctx_st *)ctx;

	memcpy(buf + SEED_BLKSIZE, seed_ctx->iv, SEED_BLKSIZE);

	SEED_encrypt(seed_ctx->iv, buf, &seed_ctx->seed_key);
	ssh_ctr_inc(seed_ctx->iv, SEED_IVSIZE);
}
static void *seed_newctx(void *prov_ctx)
{
	struct seed_ctx_st *seed_ctx;
		
	if ( (seed_ctx = (struct seed_ctx_st *)malloc(sizeof(struct seed_ctx_st))) == NULL )
		return NULL;

	memset(seed_ctx, 0, sizeof(struct seed_ctx_st));

	seed_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	seed_ctx->keylen = SEED_KEYSIZE;
	seed_ctx->ivlen  = SEED_IVSIZE;

	return seed_ctx;
}
static void seed_freectx(struct seed_ctx_st *seed_ctx)
{
	mt_finis(seed_ctx->mt_ctx);
	memset(seed_ctx, 0, sizeof(struct seed_ctx_st));
	free(seed_ctx);
}
struct seed_ctx_st *seed_dupctx(struct seed_ctx_st *seed_ctx)
{
	struct seed_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct seed_ctx_st *)malloc(sizeof(struct seed_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, seed_ctx, sizeof(struct seed_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int seed_init(struct seed_ctx_st *seed_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		unsigned char tmp[SEED_KEYSIZE];

		if ( keylen > SEED_KEYSIZE )
			keylen = SEED_KEYSIZE;
		memcpy(tmp, key, keylen);
		if ( keylen < SEED_KEYSIZE )
			memset(tmp + keylen, 0, SEED_KEYSIZE - keylen);

		SEED_set_key(tmp, &(seed_ctx->seed_key));
		memset(tmp, 0, sizeof(tmp));
	}

	if ( iv == NULL && seed_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(seed_ctx->mt_ctx);
		ivlen = SEED_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= SEED_IVSIZE);

		if ( ivlen > SEED_IVSIZE )
			ivlen = SEED_IVSIZE;
		memcpy(seed_ctx->iv, iv, ivlen);
		if ( ivlen < SEED_IVSIZE )
			memset(seed_ctx->iv + ivlen, 0, SEED_IVSIZE - ivlen);
	}

	if ( seed_ctx->mt_ctx == NULL )
		seed_ctx->mt_ctx = mt_init((void *)seed_ctx, seed_encode, SEED_BLKSIZE);
	else if ( key != NULL || iv != NULL )
		mt_reset(seed_ctx->mt_ctx);

	return 1;
}
static int seed_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, SEED_BLKSIZE, SEED_KEYSIZE, SEED_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH seed_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))seed_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))seed_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))seed_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))seed_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))seed_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))seed_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_seed_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_seed_ctr);
}

//////////////////////////////////////////////////////////////////////

#define	BLOWFISH_BLKSIZE	BF_BLOCK
#define	BLOWFISH_KEYSIZE	32
#define	BLOWFISH_IVSIZE		BF_BLOCK

struct blowfish_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	BF_KEY				blowfish_key;
};
static void blowfish_encode(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	BF_LONG data[BF_BLOCK / sizeof(BF_LONG) + 1];
	struct blowfish_ctx_st *blowfish_ctx = (struct blowfish_ctx_st *)ctx;

	memcpy(buf + BLOWFISH_BLKSIZE, blowfish_ctx->iv, BLOWFISH_BLKSIZE);

	p = blowfish_ctx->iv;
	for ( i = a = 0 ; i < BF_BLOCK ; i += 4, a++ ) {
		data[a]  = ((BF_LONG)(*(p++)) << 24);
		data[a] |= ((BF_LONG)(*(p++)) << 16);
		data[a] |= ((BF_LONG)(*(p++)) <<  8);
		data[a] |= ((BF_LONG)(*(p++)) <<  0);
	}

	BF_encrypt(data, &blowfish_ctx->blowfish_key);

	p = buf;
	for ( i = a = 0 ; i < BF_BLOCK ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >> 24);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >>  0);
	}

	ssh_ctr_inc(blowfish_ctx->iv, BF_BLOCK);
}
static void *blowfish_newctx(void *prov_ctx)
{
	struct blowfish_ctx_st *blowfish_ctx;
		
	if ( (blowfish_ctx = (struct blowfish_ctx_st *)malloc(sizeof(struct blowfish_ctx_st))) == NULL )
		return NULL;

	memset(blowfish_ctx, 0, sizeof(struct blowfish_ctx_st));

	blowfish_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	blowfish_ctx->keylen = BLOWFISH_KEYSIZE;
	blowfish_ctx->ivlen  = BLOWFISH_IVSIZE;

	return blowfish_ctx;
}
static void blowfish_freectx(struct blowfish_ctx_st *blowfish_ctx)
{
	mt_finis(blowfish_ctx->mt_ctx);
	memset(blowfish_ctx, 0, sizeof(struct blowfish_ctx_st));
	free(blowfish_ctx);
}
struct blowfish_ctx_st *blowfish_dupctx(struct blowfish_ctx_st *blowfish_ctx)
{
	struct blowfish_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct blowfish_ctx_st *)malloc(sizeof(struct blowfish_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, blowfish_ctx, sizeof(struct blowfish_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int blowfish_init(struct blowfish_ctx_st *blowfish_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen > BLOWFISH_KEYSIZE )
			keylen = BLOWFISH_KEYSIZE;

		BF_set_key(&blowfish_ctx->blowfish_key, (int)keylen, key);
	}
	
	if ( iv == NULL && blowfish_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(blowfish_ctx->mt_ctx);
		ivlen = BLOWFISH_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= BLOWFISH_IVSIZE);

		if ( ivlen > BLOWFISH_IVSIZE )
			ivlen = BLOWFISH_IVSIZE;
		memcpy(blowfish_ctx->iv, iv, ivlen);
		if ( ivlen < BLOWFISH_IVSIZE )
			memset(blowfish_ctx->iv + ivlen, 0, BLOWFISH_IVSIZE - ivlen);
	}

	if ( blowfish_ctx->mt_ctx == NULL )
		blowfish_ctx->mt_ctx = mt_init((void *)blowfish_ctx, blowfish_encode, BLOWFISH_BLKSIZE);
	else if ( key != NULL || iv != NULL )
		mt_reset(blowfish_ctx->mt_ctx);

	return 1;
}
static int blowfish_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, BLOWFISH_BLKSIZE, BLOWFISH_KEYSIZE, BLOWFISH_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH blowfish_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))blowfish_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))blowfish_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))blowfish_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))blowfish_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))blowfish_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))blowfish_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_bf_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_bf_ctr);
}

//////////////////////////////////////////////////////////////////////

#define	CAST_BLKSIZE	CAST_BLOCK
#define	CAST_KEYSIZE	CAST_KEY_LENGTH
#define	CAST_IVSIZE		CAST_BLOCK

struct cast_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	CAST_KEY			cast_key;
};
static void cast_encode(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	CAST_LONG data[CAST_BLOCK / sizeof(CAST_LONG) + 1];
	struct cast_ctx_st *c = (struct cast_ctx_st *)ctx;

	memcpy(buf + CAST_BLKSIZE, c->iv, CAST_BLKSIZE);

	p = c->iv;
	for ( i = a = 0 ; i < CAST_BLOCK ; i += 4, a++ ) {
		data[a]  = ((CAST_LONG)(*(p++)) << 24);
		data[a] |= ((CAST_LONG)(*(p++)) << 16);
		data[a] |= ((CAST_LONG)(*(p++)) <<  8);
		data[a] |= ((CAST_LONG)(*(p++)) <<  0);
	}

	CAST_encrypt(data, &c->cast_key);

	p = buf;
	for ( i = a = 0 ; i < CAST_BLOCK ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >> 24);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >>  0);
	}

	ssh_ctr_inc(c->iv, CAST_BLOCK);
}
static void *cast_newctx(void *prov_ctx)
{
	struct cast_ctx_st *cast_ctx;
		
	if ( (cast_ctx = (struct cast_ctx_st *)malloc(sizeof(struct cast_ctx_st))) == NULL )
		return NULL;

	memset(cast_ctx, 0, sizeof(struct cast_ctx_st));

	cast_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	cast_ctx->keylen = CAST_KEYSIZE;
	cast_ctx->ivlen  = CAST_IVSIZE;

	return cast_ctx;
}
static void cast_freectx(struct cast_ctx_st *cast_ctx)
{
	mt_finis(cast_ctx->mt_ctx);
	memset(cast_ctx, 0, sizeof(struct cast_ctx_st));
	free(cast_ctx);
}
struct cast_ctx_st *cast_dupctx(struct cast_ctx_st *cast_ctx)
{
	struct cast_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct cast_ctx_st *)malloc(sizeof(struct cast_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, cast_ctx, sizeof(struct cast_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int cast_init(struct cast_ctx_st *cast_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen > CAST_KEYSIZE )
			keylen = CAST_KEYSIZE;

		CAST_set_key(&cast_ctx->cast_key, (int)keylen, key);
	}

	if ( iv == NULL && cast_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(cast_ctx->mt_ctx);
		ivlen = CAST_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= CAST_IVSIZE);

		if ( ivlen > CAST_IVSIZE )
			ivlen = CAST_IVSIZE;
		memcpy(cast_ctx->iv, iv, ivlen);
		if ( ivlen < CAST_IVSIZE )
			memset(cast_ctx->iv + ivlen, 0, CAST_IVSIZE - ivlen);
	}

	if ( cast_ctx->mt_ctx == NULL )
		cast_ctx->mt_ctx = mt_init((void *)cast_ctx, cast_encode, CAST_BLKSIZE);
	else if ( key != NULL || iv != NULL )
		mt_reset(cast_ctx->mt_ctx);

	return 1;
}
static int cast_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, CAST_BLKSIZE, CAST_KEYSIZE, CAST_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH cast_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))cast_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))cast_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))cast_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))cast_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))cast_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))cast_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_cast5_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_cast5_ctr);
}

//////////////////////////////////////////////////////////////////////

#define	IDEA_BLKSIZE	IDEA_BLOCK
#define	IDEA_KEYSIZE	IDEA_KEY_LENGTH
#define	IDEA_IVSIZE		IDEA_BLOCK

struct idea_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	IDEA_KEY_SCHEDULE	idea_key;
};
static void idea_encode(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	IDEA_INT data[IDEA_BLOCK / sizeof(IDEA_INT) + 1];
	struct idea_ctx_st *c = (struct idea_ctx_st *)ctx;

	memcpy(buf + IDEA_BLKSIZE, c->iv, IDEA_BLKSIZE);

	p = c->iv;
	for ( i = a = 0 ; i < IDEA_BLOCK ; i += 4, a++ ) {
		data[a]  = ((IDEA_INT)(*(p++)) << 24);
		data[a] |= ((IDEA_INT)(*(p++)) << 16);
		data[a] |= ((IDEA_INT)(*(p++)) <<  8);
		data[a] |= ((IDEA_INT)(*(p++)) <<  0);
	}

	idea_encrypt((unsigned long *)data, &c->idea_key);

	p = buf;
	for ( i = a = 0 ; i < IDEA_BLOCK ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >> 24);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >>  0);
	}

	ssh_ctr_inc(c->iv, IDEA_BLOCK);
}
static void *idea_newctx(void *prov_ctx)
{
	struct idea_ctx_st *idea_ctx;
		
	if ( (idea_ctx = (struct idea_ctx_st *)malloc(sizeof(struct idea_ctx_st))) == NULL )
		return NULL;

	memset(idea_ctx, 0, sizeof(struct idea_ctx_st));

	idea_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	idea_ctx->keylen = IDEA_KEYSIZE;
	idea_ctx->ivlen  = IDEA_IVSIZE;

	return idea_ctx;
}
static void idea_freectx(struct idea_ctx_st *idea_ctx)
{
	mt_finis(idea_ctx->mt_ctx);
	memset(idea_ctx, 0, sizeof(struct idea_ctx_st));
	free(idea_ctx);
}
struct idea_ctx_st *idea_dupctx(struct idea_ctx_st *idea_ctx)
{
	struct idea_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct idea_ctx_st *)malloc(sizeof(struct idea_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, idea_ctx, sizeof(struct idea_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int idea_init(struct idea_ctx_st *idea_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		unsigned char tmp[IDEA_KEYSIZE];

		if ( keylen > IDEA_KEYSIZE )
			keylen = IDEA_KEYSIZE;
		memcpy(tmp, key, keylen);
		if ( keylen < IDEA_KEYSIZE )
			memset(tmp + keylen, 0, IDEA_KEYSIZE - keylen);

		IDEA_set_encrypt_key(tmp, &idea_ctx->idea_key);
		memset(tmp, 0, sizeof(tmp));
	}

	if ( iv == NULL && idea_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(idea_ctx->mt_ctx);
		ivlen = IDEA_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= IDEA_IVSIZE);

		if ( ivlen > IDEA_IVSIZE )
			ivlen = IDEA_IVSIZE;
		memcpy(idea_ctx->iv, iv, ivlen);
		if ( ivlen < IDEA_IVSIZE )
			memset(idea_ctx->iv + ivlen, 0, IDEA_IVSIZE - ivlen);
	}

	if ( idea_ctx->mt_ctx == NULL )
		idea_ctx->mt_ctx = mt_init((void *)idea_ctx, idea_encode, IDEA_BLKSIZE);
	else if ( key != NULL || iv != NULL )
		mt_reset(idea_ctx->mt_ctx);

	return 1;
}
static int idea_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, IDEA_BLKSIZE, IDEA_KEYSIZE, IDEA_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH idea_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))idea_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))idea_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))idea_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))idea_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))idea_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))idea_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_idea_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_idea_ctr);
}

//////////////////////////////////////////////////////////////////////

#define	DES3_BLKSIZE	sizeof(DES_cblock)
#define	DES3_KEYSIZE	24
#define	DES3_IVSIZE		sizeof(DES_cblock)

struct des3_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	DES_key_schedule	des3_key[3];
};
static void des3_encode(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	DES_LONG data[sizeof(DES_cblock) / sizeof(DES_LONG) + 1];
	struct des3_ctx_st *c = (struct des3_ctx_st *)ctx;

	memcpy(buf + DES3_BLKSIZE, c->iv, DES3_BLKSIZE);

	p = c->iv;
	for ( i = a = 0 ; i < DES3_BLKSIZE ; i += 4, a++ ) {
		data[a]  = ((DES_LONG)(*(p++)) <<  0);
		data[a] |= ((DES_LONG)(*(p++)) <<  8);
		data[a] |= ((DES_LONG)(*(p++)) << 16);
		data[a] |= ((DES_LONG)(*(p++)) << 24);
	}

	DES_encrypt3(data, &c->des3_key[0], &c->des3_key[1], &c->des3_key[2]);

	p = buf;
	for ( i = a = 0 ; i < DES3_BLKSIZE ; i += 4, a++ ) {
		*(p++) = (u_char)(data[a] >>  0);
		*(p++) = (u_char)(data[a] >>  8);
		*(p++) = (u_char)(data[a] >> 16);
		*(p++) = (u_char)(data[a] >> 24);
	}

	ssh_ctr_inc(c->iv, DES3_IVSIZE);
}
static void *des3_newctx(void *prov_ctx)
{
	struct des3_ctx_st *des3_ctx;
		
	if ( (des3_ctx = (struct des3_ctx_st *)malloc(sizeof(struct des3_ctx_st))) == NULL )
		return NULL;

	memset(des3_ctx, 0, sizeof(struct des3_ctx_st));

	des3_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	des3_ctx->keylen = DES3_KEYSIZE;
	des3_ctx->ivlen  = DES3_IVSIZE;

	return des3_ctx;
}
static void des3_freectx(struct des3_ctx_st *des3_ctx)
{
	mt_finis(des3_ctx->mt_ctx);
	memset(des3_ctx, 0, sizeof(struct des3_ctx_st));
	free(des3_ctx);
}
struct des3_ctx_st *des3_dupctx(struct des3_ctx_st *des3_ctx)
{
	struct des3_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct des3_ctx_st *)malloc(sizeof(struct des3_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, des3_ctx, sizeof(struct des3_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int des3_init(struct des3_ctx_st *des3_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		unsigned char tmp[DES3_KEYSIZE];

		if ( keylen > DES3_KEYSIZE )
			keylen = DES3_KEYSIZE;
		memcpy(tmp, key, keylen);
		if ( keylen < DES3_KEYSIZE )
			memset(tmp + keylen, 0, DES3_KEYSIZE - keylen);

		DES_set_key((const_DES_cblock *)(tmp +  0), &des3_ctx->des3_key[0]);
		DES_set_key((const_DES_cblock *)(tmp +  8), &des3_ctx->des3_key[1]);
		DES_set_key((const_DES_cblock *)(tmp + 16), &des3_ctx->des3_key[2]);
		memset(tmp, 0, sizeof(tmp));
	}

	if ( iv == NULL && des3_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(des3_ctx->mt_ctx);
		ivlen = DES3_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= DES3_IVSIZE);

		if ( ivlen > DES3_IVSIZE )
			ivlen = DES3_IVSIZE;
		memcpy(des3_ctx->iv, iv, ivlen);
		if ( ivlen < DES3_IVSIZE )
			memset(des3_ctx->iv + ivlen, 0, DES3_IVSIZE - ivlen);
	}

	if ( des3_ctx->mt_ctx == NULL )
		des3_ctx->mt_ctx = mt_init((void *)des3_ctx, des3_encode, DES3_BLKSIZE);
	else if ( key != NULL || iv != NULL )
		mt_reset(des3_ctx->mt_ctx);

	return 1;
}
static int des3_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, DES3_BLKSIZE, DES3_KEYSIZE, DES3_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH des3_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))des3_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))des3_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))des3_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))des3_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))des3_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))des3_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_des3_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_des3_ctr);
}

//////////////////////////////////////////////////////////////////////

#ifdef	USE_NETTLE

#define	TWOFISH_BLKSIZE		TWOFISH_BLOCK_SIZE
#define	TWOFISH_KEYSIZE		TWOFISH_KEY_SIZE
#define	TWOFISH_IVSIZE		TWOFISH_BLOCK_SIZE

struct twofish_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	struct twofish_ctx	twofish_key;
	BOOL				encrypting;
	BOOL				cbc_mode;
};
static void twofish_encode(void *ctx, u_char *buf)
{
	struct twofish_ctx_st *c = (struct twofish_ctx_st *)ctx;
		
	memcpy(buf + TWOFISH_BLKSIZE, c->iv, TWOFISH_BLKSIZE);

	twofish_encrypt(&c->twofish_key, TWOFISH_BLOCK_SIZE, buf, c->iv);
	ssh_ctr_inc(c->iv, TWOFISH_IVSIZE);
}
static int	twofish_cbc_cipher(struct twofish_ctx_st *twofish_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	u_char buf[TWOFISH_BLOCK_SIZE];
	u_char tmp[TWOFISH_BLOCK_SIZE];
	register int i;
	register union _BpIp {
		u_char		*bp;
		size_t		*ip;
	} a, b, e;

	if ( out == NULL || outl == NULL || in == NULL )
		return 0;

	*outl = 0;

	if ( twofish_ctx->encrypting ) {
		while ( inl >= TWOFISH_BLOCK_SIZE ) {
			a.bp = buf;
			b.bp = (u_char *)in;
			e.bp = twofish_ctx->iv;

			for ( i = TWOFISH_BLOCK_SIZE / sizeof(size_t) ; i > 0 ; i-- )
				*(a.ip++) = *(b.ip++) ^ *(e.ip++);

			// TWOFISH_BLOCK_SIZE % sizeof(size_t) == 0 ?
			//for ( i = TWOFISH_BLOCK_SIZE % sizeof(size_t) ; i > 0 ; i-- )
			//	*(a.bp++) = *(b.bp++) ^ *(e.bp++);

			twofish_encrypt(&twofish_ctx->twofish_key, TWOFISH_BLOCK_SIZE, twofish_ctx->iv, buf);

			memcpy(out, twofish_ctx->iv, TWOFISH_BLOCK_SIZE);

			in    += TWOFISH_BLOCK_SIZE;
			out   += TWOFISH_BLOCK_SIZE;
			inl   -= TWOFISH_BLOCK_SIZE;
			*outl += TWOFISH_BLOCK_SIZE;
		}

		if ( inl > 0 ) {
			for ( i = 0 ; i < (int)inl ; i++ )
				buf[i] = *(in++) ^ twofish_ctx->iv[i];

			for ( ; i < TWOFISH_BLOCK_SIZE ; i++ )
				buf[i] = twofish_ctx->iv[i];

			twofish_encrypt(&twofish_ctx->twofish_key, TWOFISH_BLOCK_SIZE, twofish_ctx->iv, buf);

			for ( i = 0 ; i < (int)inl ; i++ )
				*(out++) = twofish_ctx->iv[i];

			*outl += inl;
		}

	} else {
		while ( inl >= TWOFISH_BLOCK_SIZE ) {
			twofish_decrypt(&twofish_ctx->twofish_key, TWOFISH_BLOCK_SIZE, buf, in);

			a.bp = out;
			b.bp = buf;
			e.bp = twofish_ctx->iv;

			for ( i = TWOFISH_BLOCK_SIZE / sizeof(size_t) ; i > 0 ; i-- )
				*(a.ip++) = *(b.ip++) ^ *(e.ip++);

			// TWOFISH_BLOCK_SIZE % sizeof(size_t) == 0 ?
			//for ( i = TWOFISH_BLOCK_SIZE % sizeof(size_t) ; i > 0 ; i-- )
			//	*(a.bp++) = *(b.bp++) ^ *(e.bp++);

			memcpy(twofish_ctx->iv, in, TWOFISH_BLOCK_SIZE);

			in    += TWOFISH_BLOCK_SIZE;
			out   += TWOFISH_BLOCK_SIZE;
			inl   -= TWOFISH_BLOCK_SIZE;
			*outl += TWOFISH_BLOCK_SIZE;
		}

		if ( inl > 0 ) {
			for ( i = 0 ; i < (int)inl ; i++ )
				tmp[i] = *(in++);

			for ( ; i < TWOFISH_BLOCK_SIZE ; i++ )
				tmp[i] = twofish_ctx->iv[i];

			twofish_decrypt(&twofish_ctx->twofish_key, TWOFISH_BLOCK_SIZE, buf, tmp);

			for ( i = 0 ; i < (int)inl ; i++ ) {
				*(out++) = buf[i] ^ twofish_ctx->iv[i];
				twofish_ctx->iv[i] = tmp[i];
			}
			*outl += inl;

			for ( ; i < TWOFISH_BLOCK_SIZE ; i++ )
				twofish_ctx->iv[i] = buf[i];
		}
	}

	return (1);
}
static void *twofish_newctx(void *prov_ctx)
{
	struct twofish_ctx_st *twofish_ctx;
		
	if ( (twofish_ctx = (struct twofish_ctx_st *)malloc(sizeof(struct twofish_ctx_st))) == NULL )
		return NULL;

	memset(twofish_ctx, 0, sizeof(struct twofish_ctx_st));

	twofish_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	twofish_ctx->keylen = TWOFISH_KEYSIZE;
	twofish_ctx->ivlen  = TWOFISH_IVSIZE;

	return twofish_ctx;
}
static void twofish_freectx(struct twofish_ctx_st *twofish_ctx)
{
	mt_finis(twofish_ctx->mt_ctx);
	memset(twofish_ctx, 0, sizeof(struct twofish_ctx_st));
	free(twofish_ctx);
}
struct twofish_ctx_st *twofish_dupctx(struct twofish_ctx_st *twofish_ctx)
{
	struct twofish_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct twofish_ctx_st *)malloc(sizeof(struct twofish_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, twofish_ctx, sizeof(struct twofish_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int twofish_init(struct twofish_ctx_st *twofish_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen > TWOFISH_KEYSIZE )
			keylen = TWOFISH_KEYSIZE;

		twofish_set_key(&twofish_ctx->twofish_key, (int)keylen, key);
	}

	if ( iv == NULL && twofish_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(twofish_ctx->mt_ctx);
		ivlen = TWOFISH_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= TWOFISH_BLKSIZE);

		if ( ivlen > TWOFISH_IVSIZE )
			ivlen = TWOFISH_IVSIZE;
		memcpy(twofish_ctx->iv, iv, ivlen);
		if ( ivlen < TWOFISH_IVSIZE )
			memset(twofish_ctx->iv + ivlen, 0, TWOFISH_IVSIZE - ivlen);
	}

	if ( !twofish_ctx->cbc_mode ) {
		if ( twofish_ctx->mt_ctx == NULL )
			twofish_ctx->mt_ctx = mt_init((void *)twofish_ctx, twofish_encode, TWOFISH_BLKSIZE);
		else if ( key != NULL || iv != NULL )
			mt_reset(twofish_ctx->mt_ctx);
	}

	return 1;
}
static int twofish_einit(struct twofish_ctx_st *twofish_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	twofish_ctx->cbc_mode = TRUE;
	twofish_ctx->encrypting = TRUE;
	return twofish_init(twofish_ctx, key, keylen, iv, ivlen, params);
}
static int twofish_dinit(struct twofish_ctx_st *twofish_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	twofish_ctx->cbc_mode = TRUE;
	twofish_ctx->encrypting = FALSE;
	return twofish_init(twofish_ctx, key, keylen, iv, ivlen, params);
}
static int twofish_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, TWOFISH_BLKSIZE, TWOFISH_KEYSIZE, TWOFISH_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH twofish_ctr_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))twofish_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))twofish_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))twofish_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))twofish_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))twofish_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))twofish_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_twofish_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_twofish_ctr);
}
static const OSSL_DISPATCH twofish_cbc_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))twofish_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))twofish_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))twofish_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))twofish_einit },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))twofish_dinit },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))twofish_cbc_cipher },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))twofish_cbc_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))twofish_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_twofish_cbc(void)
{
	return EVP_CIPHER_fetch(NULL, "twofish-cbc", "provider=rlogin");
}

//////////////////////////////////////////////////////////////////////

#define	SERPENT_BLKSIZE		SERPENT_BLOCK_SIZE
#define	SERPENT_KEYSIZE		SERPENT_MAX_KEY_SIZE
#define	SERPENT_IVSIZE		SERPENT_BLOCK_SIZE

struct serpent_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	struct serpent_ctx	serpent_key;
	BOOL				encrypting;
	BOOL				cbc_mode;
};
static void serpent_encode(void *ctx, u_char *buf)
{
	struct serpent_ctx_st *c = (struct serpent_ctx_st *)ctx;

	memcpy(buf + SERPENT_BLKSIZE, c->iv, SERPENT_BLKSIZE);

	serpent_encrypt(&c->serpent_key, SERPENT_BLOCK_SIZE, buf, c->iv);
	ssh_ctr_inc(c->iv, SERPENT_IVSIZE);
}
static int	serpent_cbc_cipher(struct serpent_ctx_st *serpent_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	u_int n, i;
	u_char buf[SERPENT_BLOCK_SIZE];
	u_char tmp[SERPENT_BLOCK_SIZE];

	if ( out == NULL || outl == NULL || in == NULL )
		return 0;

	*outl = 0;

	if ( serpent_ctx->encrypting  ) {
		while ( inl > 0 ) {
			n = (int)(inl > SERPENT_BLOCK_SIZE ? SERPENT_BLOCK_SIZE : inl);

			for ( i = 0 ; i < n ; i++ )
				buf[i] = *(in++) ^ serpent_ctx->iv[i];

			for ( ; i < SERPENT_BLOCK_SIZE ; i++ )
				buf[i] = serpent_ctx->iv[i];

			serpent_encrypt(&serpent_ctx->serpent_key, SERPENT_BLOCK_SIZE, serpent_ctx->iv, buf);

			for ( i = 0 ; i < n ; i++ )
				*(out++) = serpent_ctx->iv[i];

			inl -= n;
			*outl += n;
		}

	} else {
		while ( inl > 0 ) {
			n = (int)(inl > SERPENT_BLOCK_SIZE ? SERPENT_BLOCK_SIZE : inl);

			for ( i = 0 ; i < n ; i++ )
				tmp[i] = *(in++);

			for ( ; i < SERPENT_BLOCK_SIZE ; i++ )
				tmp[i] = serpent_ctx->iv[i];

			serpent_decrypt(&serpent_ctx->serpent_key, SERPENT_BLOCK_SIZE, buf, tmp);

			for ( i = 0 ; i < n ; i++ ) {
				*(out++) = buf[i] ^ serpent_ctx->iv[i];
				serpent_ctx->iv[i] = tmp[i];
			}

			for ( ; i < SERPENT_BLOCK_SIZE ; i++ )
				serpent_ctx->iv[i] = buf[i];

			inl -= n;
			*outl += n;
		}
	}

	return (1);
}
static void *serpent_newctx(void *prov_ctx)
{
	struct serpent_ctx_st *serpent_ctx;
		
	if ( (serpent_ctx = (struct serpent_ctx_st *)malloc(sizeof(struct serpent_ctx_st))) == NULL )
		return NULL;

	memset(serpent_ctx, 0, sizeof(struct serpent_ctx_st));

	serpent_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	serpent_ctx->keylen = SERPENT_KEYSIZE;
	serpent_ctx->ivlen  = SERPENT_IVSIZE;

	return serpent_ctx;
}
static void serpent_freectx(struct serpent_ctx_st *serpent_ctx)
{
	mt_finis(serpent_ctx->mt_ctx);
	memset(serpent_ctx, 0, sizeof(struct serpent_ctx_st));
	free(serpent_ctx);
}
struct serpent_ctx_st *serpent_dupctx(struct serpent_ctx_st *serpent_ctx)
{
	struct serpent_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct serpent_ctx_st *)malloc(sizeof(struct serpent_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, serpent_ctx, sizeof(struct serpent_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int serpent_init(struct serpent_ctx_st *serpent_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen > SERPENT_KEYSIZE )
			keylen = SERPENT_KEYSIZE;

		serpent_set_key(&serpent_ctx->serpent_key, (int)keylen, key);
	}

	if ( iv == NULL && serpent_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(serpent_ctx->mt_ctx);
		ivlen = SERPENT_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= SERPENT_IVSIZE);

		if ( ivlen > SERPENT_IVSIZE )
			ivlen = SERPENT_IVSIZE;
		memcpy(serpent_ctx->iv, iv, ivlen);
		if ( ivlen < SERPENT_IVSIZE )
			memset(serpent_ctx->iv + ivlen, 0, SERPENT_IVSIZE - ivlen);
	}

	if ( !serpent_ctx->cbc_mode ) {
		if ( serpent_ctx->mt_ctx == NULL )
			serpent_ctx->mt_ctx = mt_init((void *)serpent_ctx, serpent_encode, SERPENT_BLKSIZE);
		else if ( key != NULL || iv != NULL )
			mt_reset(serpent_ctx->mt_ctx);
	}

	return 1;
}
static int serpent_einit(struct serpent_ctx_st *serpent_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	serpent_ctx->cbc_mode = TRUE;
	serpent_ctx->encrypting = TRUE;
	return serpent_init(serpent_ctx, key, keylen, iv, ivlen, params);
}
static int serpent_dinit(struct serpent_ctx_st *serpent_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	serpent_ctx->cbc_mode = TRUE;
	serpent_ctx->encrypting = FALSE;
	return serpent_init(serpent_ctx, key, keylen, iv, ivlen, params);
}
static int serpent_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, SERPENT_BLKSIZE, SERPENT_KEYSIZE, SERPENT_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH serpent_ctr_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))serpent_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))serpent_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))serpent_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))serpent_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))serpent_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))serpent_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_serpent_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_twofish_ctr);
}
static const OSSL_DISPATCH serpent_cbc_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))serpent_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))serpent_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))serpent_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))serpent_einit },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))serpent_dinit },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))serpent_cbc_cipher },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))serpent_cbc_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))serpent_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_serpent_cbc(void)
{
	return rlg_evp_cipver_fetch(Nids_twofish_cbc);
}

#endif	//	USE_NETTLE

//////////////////////////////////////////////////////////////////////

#ifdef	USE_CLEFIA

#define	CLEFIA_BLKSIZE		CLEFIA_BLOCK_SIZE
#define	CLEFIA_KEYSIZE		CLEFIA_KEY_SIZE
#define	CLEFIA_IVSIZE		CLEFIA_BLOCK_SIZE

struct clefia_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	struct clefia_ctx	clefia_key;
	BOOL				encrypting;
	BOOL				cbc_mode;
};
static void clefia_encode(void *ctx, u_char *buf)
{
	struct clefia_ctx_st *c = (struct clefia_ctx_st *)ctx;

	memcpy(buf + CLEFIA_BLKSIZE, c->iv, CLEFIA_BLKSIZE);

	ClefiaEncrypt(&c->clefia_key, buf, c->iv);
	ssh_ctr_inc(c->iv, CLEFIA_IVSIZE);
}
static int	clefia_cbc_cipher(struct clefia_ctx_st *clefia_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	u_int n, i;
	u_char buf[CLEFIA_BLOCK_SIZE];
	u_char tmp[CLEFIA_BLOCK_SIZE];

	if ( out == NULL || outl == NULL || in == NULL )
		return 0;

	*outl = 0;

	if ( clefia_ctx->encrypting  ) {
		while ( inl > 0 ) {
			n = (int)(inl > CLEFIA_BLOCK_SIZE ? CLEFIA_BLOCK_SIZE : inl);

			for ( i = 0 ; i < n ; i++ )
				buf[i] = *(in++) ^ clefia_ctx->iv[i];

			for ( ; i < CLEFIA_BLOCK_SIZE ; i++ )
				buf[i] = clefia_ctx->iv[i];

			ClefiaEncrypt(&clefia_ctx->clefia_key, clefia_ctx->iv, buf);

			for ( i = 0 ; i < n ; i++ )
				*(out++) = clefia_ctx->iv[i];

			inl -= n;
			*outl += n;
		}

	} else {
		while ( inl > 0 ) {
			n = (int)(inl > CLEFIA_BLOCK_SIZE ? CLEFIA_BLOCK_SIZE : inl);

			for ( i = 0 ; i < n ; i++ )
				tmp[i] = *(in++);

			for ( ; i < CLEFIA_BLOCK_SIZE ; i++ )
				tmp[i] = clefia_ctx->iv[i];

			ClefiaDecrypt(&clefia_ctx->clefia_key, buf, tmp);

			for ( i = 0 ; i < n ; i++ ) {
				*(out++) = buf[i] ^ clefia_ctx->iv[i];
				clefia_ctx->iv[i] = tmp[i];
			}

			for ( ; i < CLEFIA_BLOCK_SIZE ; i++ )
				clefia_ctx->iv[i] = buf[i];

			inl -= n;
			*outl += n;
		}
	}

	return (1);
}
static void *clefia_newctx(void *prov_ctx)
{
	struct clefia_ctx_st *clefia_ctx;
		
	if ( (clefia_ctx = (struct clefia_ctx_st *)malloc(sizeof(struct clefia_ctx_st))) == NULL )
		return NULL;

	memset(clefia_ctx, 0, sizeof(struct clefia_ctx_st));

	clefia_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	clefia_ctx->keylen = CLEFIA_KEYSIZE;
	clefia_ctx->ivlen  = CLEFIA_IVSIZE;

	return clefia_ctx;
}
static void clefia_freectx(struct clefia_ctx_st *clefia_ctx)
{
	mt_finis(clefia_ctx->mt_ctx);
	memset(clefia_ctx, 0, sizeof(struct clefia_ctx_st));
	free(clefia_ctx);
}
struct clefia_ctx_st *clefia_dupctx(struct clefia_ctx_st *clefia_ctx)
{
	struct clefia_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct clefia_ctx_st *)malloc(sizeof(struct clefia_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, clefia_ctx, sizeof(struct clefia_ctx_st));
	dup_ctx->mt_ctx = NULL;

	return dup_ctx;
}
static int clefia_init(struct clefia_ctx_st *clefia_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen > CLEFIA_KEYSIZE )
			keylen = CLEFIA_KEYSIZE;

		ClefiaKeySet(&clefia_ctx->clefia_key, key, (int)keylen * 8);
	}

	if ( iv == NULL && clefia_ctx->mt_ctx != NULL ) {
		iv = mt_get_iv(clefia_ctx->mt_ctx);
		ivlen = CLEFIA_IVSIZE;
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= CLEFIA_IVSIZE);

		if ( ivlen > CLEFIA_IVSIZE )
			ivlen = CLEFIA_IVSIZE;
		memcpy(clefia_ctx->iv, iv, ivlen);
		if ( ivlen < CLEFIA_IVSIZE )
			memset(clefia_ctx->iv + ivlen, 0, CLEFIA_IVSIZE - ivlen);
	}

	if ( !clefia_ctx->cbc_mode ) {
		if ( clefia_ctx->mt_ctx == NULL )
			clefia_ctx->mt_ctx = mt_init((void *)clefia_ctx, clefia_encode, CLEFIA_BLKSIZE);
		else if ( key != NULL || iv != NULL )
			mt_reset(clefia_ctx->mt_ctx);
	}

	return 1;
}
static int clefia_einit(struct clefia_ctx_st *clefia_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	clefia_ctx->cbc_mode = TRUE;
	clefia_ctx->encrypting = TRUE;
	return clefia_init(clefia_ctx, key, keylen, iv, ivlen, params);
}
static int clefia_dinit(struct clefia_ctx_st *clefia_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	clefia_ctx->cbc_mode = TRUE;
	clefia_ctx->encrypting = FALSE;
	return clefia_init(clefia_ctx, key, keylen, iv, ivlen, params);
}
static int clefia_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, CLEFIA_BLKSIZE, CLEFIA_KEYSIZE, CLEFIA_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH clefia_ctr_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))clefia_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))clefia_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))clefia_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))clefia_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))clefia_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh_ctr_update },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh_ctr_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))clefia_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_clefia_ctr(void)
{
	return rlg_evp_cipver_fetch(Nids_clefia_ctr);
}
static const OSSL_DISPATCH clefia_cbc_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))clefia_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))clefia_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))clefia_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))clefia_einit },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))clefia_dinit },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))clefia_cbc_cipher },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))clefia_cbc_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))clefia_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_clefia_cbc(void)
{
	return rlg_evp_cipver_fetch(Nids_clefia_cbc);
}

#endif

//////////////////////////////////////////////////////////////////////

#define	SSH1_3DES_BLKSIZE		sizeof(DES_cblock)
#define	SSH1_3DES_KEYSIZE		16
#define	SSH1_3DES_IVSIZE		sizeof(DES_cblock)

struct ssh1_3des_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	DES_key_schedule	des_key[3];
	BOOL				encrypting;
};
static int	ssh1_3des_cipher(struct ssh1_3des_ctx_st *ssh1_3des_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	int enc = ssh1_3des_ctx->encrypting ? DES_ENCRYPT : DES_DECRYPT;

	if ( out == NULL || outl == NULL || in == NULL )
		return 0;

	*outl = 0;

	DES_cbc_encrypt(in,  out, (long)inl, &(ssh1_3des_ctx->des_key[0]), (DES_cblock *)(ssh1_3des_ctx->iv), enc);
	DES_cbc_encrypt(out, out, (long)inl, &(ssh1_3des_ctx->des_key[1]), (DES_cblock *)(ssh1_3des_ctx->iv), !enc);
	DES_cbc_encrypt(out, out, (long)inl, &(ssh1_3des_ctx->des_key[2]), (DES_cblock *)(ssh1_3des_ctx->iv), enc);

	*outl += inl;

	return (1);
}
static void *ssh1_3des_newctx(void *prov_ctx)
{
	struct ssh1_3des_ctx_st *ssh1_3des_ctx;
		
	if ( (ssh1_3des_ctx = (struct ssh1_3des_ctx_st *)malloc(sizeof(struct ssh1_3des_ctx_st))) == NULL )
		return NULL;

	memset(ssh1_3des_ctx, 0, sizeof(struct ssh1_3des_ctx_st));

	ssh1_3des_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	ssh1_3des_ctx->keylen = SSH1_3DES_KEYSIZE;
	ssh1_3des_ctx->ivlen  = SSH1_3DES_IVSIZE;

	return ssh1_3des_ctx;
}
static void ssh1_3des_freectx(struct ssh1_3des_ctx_st *ssh1_3des_ctx)
{
	memset(ssh1_3des_ctx, 0, sizeof(struct ssh1_3des_ctx_st));
	free(ssh1_3des_ctx);
}
struct ssh1_3des_ctx_st *ssh1_3des_dupctx(struct ssh1_3des_ctx_st *ssh1_3des_ctx)
{
	struct ssh1_3des_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct ssh1_3des_ctx_st *)malloc(sizeof(struct ssh1_3des_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, ssh1_3des_ctx, sizeof(struct ssh1_3des_ctx_st));

	return dup_ctx;
}
static int ssh1_3des_init(struct ssh1_3des_ctx_st *ssh1_3des_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		u_char tmp[SSH1_3DES_KEYSIZE + 8];
		u_char *k1, *k2, *k3;

		if ( keylen > (SSH1_3DES_KEYSIZE + 8) )
			keylen = SSH1_3DES_KEYSIZE + 8;
		memcpy(tmp, key, keylen);
		if ( keylen < SSH1_3DES_KEYSIZE )
			memset(tmp + keylen, 0, SSH1_3DES_KEYSIZE - keylen);

		k1 = k2 = k3 = tmp;
		k2 += 8;

		if ( keylen >= (SSH1_3DES_KEYSIZE + 8) ) {
			if ( ssh1_3des_ctx->encrypting )
				k3 += 16;
			else
				k1 += 16;
		}

		DES_set_key_unchecked((const_DES_cblock *)k1, &(ssh1_3des_ctx->des_key[0]));
		DES_set_key_unchecked((const_DES_cblock *)k2, &(ssh1_3des_ctx->des_key[1]));
		DES_set_key_unchecked((const_DES_cblock *)k3, &(ssh1_3des_ctx->des_key[2]));
		memset(tmp, 0, sizeof(tmp));
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= SSH1_3DES_IVSIZE);

		if ( ivlen > SSH1_3DES_IVSIZE )
			ivlen = SSH1_3DES_IVSIZE;
		memcpy(ssh1_3des_ctx->iv, iv, ivlen);
		if ( ivlen < SSH1_3DES_IVSIZE )
			memset(ssh1_3des_ctx->iv + ivlen, 0, SSH1_3DES_IVSIZE - ivlen);
	}

	return 1;
}
static int ssh1_3des_einit(struct ssh1_3des_ctx_st *ssh1_3des_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	ssh1_3des_ctx->encrypting = TRUE;
	return ssh1_3des_init(ssh1_3des_ctx, key, keylen, iv, ivlen, params);
}
static int ssh1_3des_dinit(struct ssh1_3des_ctx_st *ssh1_3des_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	ssh1_3des_ctx->encrypting = FALSE;
	return ssh1_3des_init(ssh1_3des_ctx, key, keylen, iv, ivlen, params);
}
static int ssh1_3des_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, SSH1_3DES_BLKSIZE, SSH1_3DES_KEYSIZE, SSH1_3DES_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH ssh1_3des_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))ssh1_3des_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))ssh1_3des_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))ssh1_3des_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))ssh1_3des_einit },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))ssh1_3des_dinit },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh1_3des_cipher },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh1_3des_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))ssh1_3des_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_ssh1_3des(void)
{
	return rlg_evp_cipver_fetch(Nids_ssh1_3des);
}

//////////////////////////////////////////////////////////////////////

#define	SSH1_BF_BLKSIZE		BF_BLOCK
#define	SSH1_BF_KEYSIZE		32
#define	SSH1_BF_IVSIZE		BF_BLOCK

struct ssh1_bf_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	BF_KEY				bf_key;
	BOOL				encrypting;
};
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
static int	ssh1_bf_cipher(struct ssh1_bf_ctx_st *ssh1_bf_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	if ( out == NULL || outl == NULL || in == NULL )
		return 0;

	*outl = 0;

	swap_bytes(in, out, (int)inl);
	BF_cbc_encrypt(out, out, (long)inl, &(ssh1_bf_ctx->bf_key), ssh1_bf_ctx->iv, ssh1_bf_ctx->encrypting ? BF_ENCRYPT : BF_DECRYPT);
	swap_bytes(out, out, (int)inl);

	*outl += inl;

	return (1);
}
static void *ssh1_bf_newctx(void *prov_ctx)
{
	struct ssh1_bf_ctx_st *ssh1_bf_ctx;
		
	if ( (ssh1_bf_ctx = (struct ssh1_bf_ctx_st *)malloc(sizeof(struct ssh1_bf_ctx_st))) == NULL )
		return NULL;

	memset(ssh1_bf_ctx, 0, sizeof(struct ssh1_bf_ctx_st));

	ssh1_bf_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	ssh1_bf_ctx->keylen = SSH1_BF_KEYSIZE;
	ssh1_bf_ctx->ivlen  = SSH1_BF_IVSIZE;

	return ssh1_bf_ctx;
}
static void ssh1_bf_freectx(struct ssh1_bf_ctx_st *ssh1_bf_ctx)
{
	memset(ssh1_bf_ctx, 0, sizeof(struct ssh1_bf_ctx_st));
	free(ssh1_bf_ctx);
}
struct ssh1_bf_ctx_st *ssh1_bf_dupctx(struct ssh1_bf_ctx_st *ssh1_bf_ctx)
{
	struct ssh1_bf_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct ssh1_bf_ctx_st *)malloc(sizeof(struct ssh1_bf_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, ssh1_bf_ctx, sizeof(struct ssh1_bf_ctx_st));

	return dup_ctx;
}
static int ssh1_bf_init(struct ssh1_bf_ctx_st *ssh1_bf_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen > SSH1_BF_KEYSIZE )
			keylen = SSH1_BF_KEYSIZE;

		BF_set_key(&(ssh1_bf_ctx->bf_key), (int)keylen, key);
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= SSH1_BF_IVSIZE);

		if ( ivlen > SSH1_BF_IVSIZE )
			ivlen = SSH1_BF_IVSIZE;
		memcpy(ssh1_bf_ctx->iv, iv, ivlen);
		if ( ivlen < SSH1_BF_IVSIZE )
			memset(ssh1_bf_ctx->iv + ivlen, 0, SSH1_BF_IVSIZE - ivlen);
	}

	return 1;
}
static int ssh1_bf_einit(struct ssh1_bf_ctx_st *ssh1_bf_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	ssh1_bf_ctx->encrypting = TRUE;
	return ssh1_bf_init(ssh1_bf_ctx, key, keylen, iv, ivlen, params);
}
static int ssh1_bf_dinit(struct ssh1_bf_ctx_st *ssh1_bf_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	ssh1_bf_ctx->encrypting = FALSE;
	return ssh1_bf_init(ssh1_bf_ctx, key, keylen, iv, ivlen, params);
}
static int ssh1_bf_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, SSH1_BF_BLKSIZE, SSH1_BF_KEYSIZE, SSH1_BF_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH ssh1_bf_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))ssh1_bf_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))ssh1_bf_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))ssh1_bf_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))ssh1_bf_einit },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))ssh1_bf_dinit },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))ssh1_bf_cipher },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))ssh1_bf_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))ssh_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))ssh_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))ssh_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))ssh_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))ssh1_bf_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_ssh1_bf(void)
{
	return rlg_evp_cipver_fetch(Nids_ssh1_bf);
}

//////////////////////////////////////////////////////////////////////

#define	CHACHAPOLY_BLKSIZE		POLY1305_BLOCK_SIZE
#define	CHACHAPOLY_KEYSIZE		(CHACHA_KEY_SIZE * 2)
#define	CHACHAPOLY_IVSIZE		8

typedef struct _chacha_ctx {
	u_int	key[CHACHA_KEY_SIZE / sizeof(u_int)];
	u_int	conter[CHACHA_CTR_SIZE / sizeof(u_int)];
} chacha_ctx;

struct chachapoly_ctx_st {
	struct mt_ctr_ctx	*mt_ctx;					// dont move !!
	struct prov_ctx_st	*prov_ctx;					//	|
	size_t				keylen;						//	|
	size_t				ivlen;						//	|
	unsigned char		iv[SSH2_CIPHER_IVSIZEMAX];	//  |

	chacha_ctx			main_ctx, header_ctx;
	u_char				expected_tag[POLY1305_DIGEST_SIZE];
	u_char				poly_key[POLY1305_KEY_SIZE];
	POLY1305			*poly_ctx;
};

static void chacha_counter(chacha_ctx *ctx, const u_char *iv, const u_char *counter)
{
	if ( counter != NULL ) {
		ctx->conter[0] = *((u_int *)(counter + 0));
		ctx->conter[1] = *((u_int *)(counter + 4));
	} else {
		ctx->conter[0] = 0;
		ctx->conter[1] = 0;
	}
	ctx->conter[2] = *((u_int *)(iv + 0));
	ctx->conter[3] = *((u_int *)(iv + 4));
}
static int	chachapoly_cipher(struct chachapoly_ctx_st *chachapoly_ctx, unsigned char *out, size_t *outl, size_t outsize, const unsigned char *in, size_t inl)
{
	size_t aadlen = 4;
	const u_char one[8] = { 1, 0, 0, 0, 0, 0, 0, 0 }; /* NB. little-endian */

	if ( out == NULL || outl == NULL || in == NULL )
		return 0;

	*outl = 0;

	chacha_counter(&chachapoly_ctx->header_ctx, chachapoly_ctx->iv, NULL);
	ChaCha20_ctr32(out, in, aadlen, chachapoly_ctx->header_ctx.key, chachapoly_ctx->header_ctx.conter);

	if ( inl > aadlen ) {
		chacha_counter(&chachapoly_ctx->main_ctx, chachapoly_ctx->iv, one);
		ChaCha20_ctr32(out + aadlen, in + aadlen, inl - aadlen, chachapoly_ctx->main_ctx.key, chachapoly_ctx->main_ctx.conter);
	}

	*outl += inl;

	return (1);
}
static void *chachapoly_newctx(void *prov_ctx)
{
	struct chachapoly_ctx_st *chachapoly_ctx;
		
	if ( (chachapoly_ctx = (struct chachapoly_ctx_st *)malloc(sizeof(struct chachapoly_ctx_st))) == NULL )
		return NULL;

	memset(chachapoly_ctx, 0, sizeof(struct chachapoly_ctx_st));

	chachapoly_ctx->prov_ctx = (struct prov_ctx_st*)prov_ctx;
	chachapoly_ctx->keylen = CHACHAPOLY_KEYSIZE;
	chachapoly_ctx->ivlen  = CHACHAPOLY_IVSIZE;

	if ( (chachapoly_ctx->poly_ctx = (POLY1305 *)malloc(Poly1305_ctx_size())) == NULL ) {
		free(chachapoly_ctx);
		return NULL;
	}

	memset(chachapoly_ctx->poly_ctx, 0, Poly1305_ctx_size());

	return chachapoly_ctx;
}
static void chachapoly_freectx(struct chachapoly_ctx_st *chachapoly_ctx)
{
	free(chachapoly_ctx->poly_ctx);

	memset(chachapoly_ctx, 0, sizeof(struct chachapoly_ctx_st));
	free(chachapoly_ctx);
}
struct chachapoly_ctx_st *chachapoly_dupctx(struct chachapoly_ctx_st *chachapoly_ctx)
{
	struct chachapoly_ctx_st *dup_ctx;
		
	if ( (dup_ctx = (struct chachapoly_ctx_st *)malloc(sizeof(struct chachapoly_ctx_st))) == NULL )
		return NULL;

	memcpy(dup_ctx, chachapoly_ctx, sizeof(struct chachapoly_ctx_st));

	if ( (dup_ctx->poly_ctx = (POLY1305 *)malloc(Poly1305_ctx_size())) == NULL ) {
		free(dup_ctx);
		return NULL;
	}

	memset(dup_ctx->poly_ctx, 0, Poly1305_ctx_size());

	return dup_ctx;
}
static int chachapoly_init(struct chachapoly_ctx_st *chachapoly_ctx, const unsigned char *key, size_t keylen, const unsigned char *iv, size_t ivlen, const OSSL_PARAM params[])
{
	if ( key != NULL ) {
		if ( keylen < CHACHAPOLY_KEYSIZE )
			return 0;

		memcpy(chachapoly_ctx->main_ctx.key, key, CHACHA_KEY_SIZE);
		memcpy(chachapoly_ctx->header_ctx.key, key + CHACHA_KEY_SIZE, CHACHA_KEY_SIZE);
	}

	if ( iv != NULL ) {
		ASSERT(SSH2_CIPHER_IVSIZEMAX >= CHACHAPOLY_IVSIZE);

		// iv update chachapoly_set_ctx_params(OSSL_CIPHER_PARAM_AEAD_TLS1_IV_FIXED)
	}

	return 1;
}
static int chachapoly_get_ctx_params(struct chachapoly_ctx_st *chachapoly_ctx, OSSL_PARAM params[])
{
	OSSL_PARAM *p;
	int rc = 1;

	for ( p = params ; p->data_type != 0 ; p++ ) {
		switch(p->data_type) {
		case OSSL_PARAM_UNSIGNED_INTEGER:
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_KEYLEN) == 0 )
				OSSL_PARAM_set_size_t(p, chachapoly_ctx->keylen);
			else if ( strcmp(p->key, OSSL_CIPHER_PARAM_IVLEN) == 0 )
				OSSL_PARAM_set_size_t(p, chachapoly_ctx->ivlen);
			else
				rc = 0;
			break;

		case OSSL_PARAM_OCTET_STRING:
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_AEAD_TAG) == 0 )
				OSSL_PARAM_set_octet_string(p, chachapoly_ctx->expected_tag, POLY1305_DIGEST_SIZE);
			else if ( strcmp(p->key, OSSL_CIPHER_PARAM_IV) == 0 )
				OSSL_PARAM_set_octet_string(p, chachapoly_ctx->iv, CHACHAPOLY_IVSIZE);
			else
				rc = 0;
			break;
		default:
			rc = 0;
			break;
		}
	}

	ASSERT(rc == 1);
	return rc;
}
static const OSSL_PARAM *chachapol_gettable_ctx_params(struct ssh_cipher_ctx *ssh_ctx, struct prov_ctx_st *prov_ctx)
{
	static OSSL_PARAM params[] = {
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_KEYLEN, 0),
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_IVLEN, 0),
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_IV, NULL, 0),
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, NULL, 0),
		OSSL_PARAM_END,
	};

	return params;
}
static int chachapoly_set_ctx_params(struct chachapoly_ctx_st *chachapoly_ctx, const OSSL_PARAM params[])
{
	const OSSL_PARAM *p;
	const void *ptr = NULL;
	size_t len = 0;
	int rc = 1;

	for ( p = params ; p->data_type != 0 ; p++ ) {
		switch(p->data_type) {
		case OSSL_PARAM_UNSIGNED_INTEGER:
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_KEYLEN) == 0 )
				OSSL_PARAM_get_size_t(p, &chachapoly_ctx->keylen);
			else if ( strcmp(p->key, OSSL_CIPHER_PARAM_IVLEN) == 0 )
				OSSL_PARAM_get_size_t(p, &chachapoly_ctx->ivlen);
			else
				rc = 0;
			break;

		case OSSL_PARAM_OCTET_STRING:
			if ( strcmp(p->key, OSSL_CIPHER_PARAM_AEAD_TLS1_IV_FIXED) == 0 ) {
				OSSL_PARAM_get_octet_string_ptr(p, &ptr, &len);
				memset(chachapoly_ctx->poly_key, 0, POLY1305_KEY_SIZE);
				if ( ptr != NULL && len == CHACHAPOLY_IVSIZE )
					memcpy(chachapoly_ctx->iv, ptr, len);
				chacha_counter(&chachapoly_ctx->main_ctx, chachapoly_ctx->iv, NULL);
				ChaCha20_ctr32(chachapoly_ctx->poly_key, chachapoly_ctx->poly_key, POLY1305_KEY_SIZE, chachapoly_ctx->main_ctx.key, chachapoly_ctx->main_ctx.conter);

			} else if ( strcmp(p->key, OSSL_CIPHER_PARAM_AEAD_TAG) == 0 ) {
				OSSL_PARAM_get_octet_string_ptr(p, &ptr, &len);
				if ( ptr != NULL && len > 0 ) {
					Poly1305_Init(chachapoly_ctx->poly_ctx, chachapoly_ctx->poly_key);
					Poly1305_Update(chachapoly_ctx->poly_ctx, (const unsigned char *)ptr, len);
					Poly1305_Final(chachapoly_ctx->poly_ctx, chachapoly_ctx->expected_tag);
				}
			} else
				rc = 0;
			break;
		default:
			rc = 0;
			break;
		}
	}

	ASSERT(rc == 1);
	return rc;
}
static const OSSL_PARAM *chachapol_settable_ctx_params(struct ssh_cipher_ctx *ssh_ctx, struct prov_ctx_st *prov_ctx)
{
	static OSSL_PARAM params[] = {
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_KEYLEN, 0),
		OSSL_PARAM_size_t(OSSL_CIPHER_PARAM_IVLEN, 0),
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_AEAD_TLS1_IV_FIXED, NULL, 0),
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, NULL, 0),
		OSSL_PARAM_END,
	};

	return params;
}
static int chachapoly_get_params(OSSL_PARAM params[])
{
	return rlg_get_params(params, 0, CHACHAPOLY_BLKSIZE, CHACHAPOLY_KEYSIZE, CHACHAPOLY_IVSIZE, PARAM_CUSTOM_IV_FLAG);
}
static const OSSL_DISPATCH chachapoly_functions[] = {
	{ OSSL_FUNC_CIPHER_NEWCTX,				(void (*)(void))chachapoly_newctx },
	{ OSSL_FUNC_CIPHER_FREECTX,				(void (*)(void))chachapoly_freectx },
	{ OSSL_FUNC_CIPHER_DUPCTX,				(void (*)(void))chachapoly_dupctx },
	{ OSSL_FUNC_CIPHER_ENCRYPT_INIT,		(void (*)(void))chachapoly_init },
	{ OSSL_FUNC_CIPHER_DECRYPT_INIT,		(void (*)(void))chachapoly_init },
	{ OSSL_FUNC_CIPHER_UPDATE,				(void (*)(void))chachapoly_cipher },
	{ OSSL_FUNC_CIPHER_FINAL,				(void (*)(void))ssh_ctr_final },
	{ OSSL_FUNC_CIPHER_CIPHER,				(void (*)(void))chachapoly_cipher },
	{ OSSL_FUNC_CIPHER_GET_CTX_PARAMS,		(void (*)(void))chachapoly_get_ctx_params },
	{ OSSL_FUNC_CIPHER_SET_CTX_PARAMS,		(void (*)(void))chachapoly_set_ctx_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_PARAMS,		(void (*)(void))ssh_gettable_params },
	{ OSSL_FUNC_CIPHER_GETTABLE_CTX_PARAMS,	(void (*)(void))chachapol_gettable_ctx_params },
	{ OSSL_FUNC_CIPHER_SETTABLE_CTX_PARAMS,	(void (*)(void))chachapol_settable_ctx_params },
	{ OSSL_FUNC_CIPHER_GET_PARAMS,			(void (*)(void))chachapoly_get_params },
	{ 0, NULL }
};
const EVP_CIPHER *evp_chachapoly_256(void)
{
	return rlg_evp_cipver_fetch(Nids_chachapoly_256);
}

//////////////////////////////////////////////////////////////////////

static const OSSL_ALGORITHM rlg_cipher_algorithm[] = {
	{ "aes-rlg-ctr",	"provider=rlogin", aes_functions },
	{ "seed-ctr",		"provider=rlogin", seed_functions },
	{ "blowfish-ctr",	"provider=rlogin", blowfish_functions },
	{ "cast5-ctr",		"provider=rlogin", cast_functions },
	{ "idea-ctr",		"provider=rlogin", idea_functions },
	{ "3des-ctr",		"provider=rlogin", des3_functions },
#ifdef	USE_NETTLE
	{ "twofish-ctr",	"provider=rlogin", twofish_ctr_functions },
	{ "twofish-cbc",	"provider=rlogin", twofish_cbc_functions },
	{ "serpent-ctr",	"provider=rlogin", serpent_ctr_functions },
	{ "serpent-cbc",	"provider=rlogin", serpent_cbc_functions },
#endif
#ifdef	USE_CLEFIA
	{ "clefia-ctr",		"provider=rlogin", clefia_ctr_functions },
	{ "clefia-cbc",		"provider=rlogin", clefia_cbc_functions },
#endif
	{ "ssh1-3des",		"provider=rlogin", ssh1_3des_functions },
	{ "ssh1-bf",		"provider=rlogin", ssh1_bf_functions },
	{ "chachapolyssh",	"provider=rlogin", chachapoly_functions },
	{ NULL, NULL, NULL }
};
static EVP_CIPHER *rlg_evp_cipher_tab[] = {
	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
#ifdef	USE_NETTLE
	NULL,	NULL,	NULL,	NULL,
#endif
#ifdef	USE_CLEFIA
	NULL,	NULL,
#endif
	NULL,	NULL,	NULL,
};
static const EVP_CIPHER *rlg_evp_cipver_fetch(int nids)
{
	ASSERT(nids >= 0 && nids < NIDSCIPHERMAX);

	if ( rlg_evp_cipher_tab[nids] == NULL )
		rlg_evp_cipher_tab[nids] = EVP_CIPHER_fetch(NULL, rlg_cipher_algorithm[nids].algorithm_names, rlg_cipher_algorithm[nids].property_definition);

	return rlg_evp_cipher_tab[nids];
}
static void rlg_provider_teardown(struct prov_ctx_st *prov_ctx)
{
     free(prov_ctx);
}
static const OSSL_PARAM *rlg_provider_gettable_params(struct prov_ctx_st *prov_ctx)
{
	static OSSL_PARAM params[] = {
		OSSL_PARAM_utf8_string(OSSL_PROV_PARAM_NAME, NULL, 0),
		OSSL_PARAM_utf8_string(OSSL_PROV_PARAM_VERSION, NULL, 0),
		OSSL_PARAM_utf8_string(OSSL_PROV_PARAM_BUILDINFO, NULL, 0),
		OSSL_PARAM_uint(OSSL_PROV_PARAM_STATUS, 0),
		OSSL_PARAM_END,
	};

	return params;
}
static int rlg_provider_get_params(struct prov_ctx_st *prov_ctx, OSSL_PARAM params[])
{
	OSSL_PARAM *p;
	CString version;

	if ( (p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_NAME)) != NULL )
		OSSL_PARAM_set_utf8_string(p, "rlogin");

	if ( (p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_VERSION)) != NULL ||
		 (p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_BUILDINFO)) != NULL ) {
		((CRLoginApp *)AfxGetApp())->GetVersion(version);
		OSSL_PARAM_set_utf8_string(p, TstrToMbs(version));
	}

	if ( (p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_STATUS)) != NULL )
		OSSL_PARAM_set_uint(p, prov_ctx->status);

	return 0;
}
static const OSSL_ALGORITHM *rlg_provider_query(struct prov_ctx_st *prov_ctx, int operation_id, int *no_store)
{
	switch (operation_id) {
	case OSSL_OP_CIPHER:
		return rlg_cipher_algorithm;
	}

	return NULL;
}
//static void rlg_provider_unquery_operation(struct prov_ctx_st *prov_ctx, int operation_id, const OSSL_ALGORITHM *algorithm)
//{
//}
//static const OSSL_ITEM *rlg_provider_get_reason_strings(struct prov_ctx_st *prov_ctx)
//{
//	return NULL;
//}
//static int rlg_provider_get_capabilities(struct prov_ctx_st *prov_ctx, const char *capability, OSSL_CALLBACK *cb, void *arg)
//{
//	return 0;
//}
//static int rlg_provider_self_test(struct prov_ctx_st *prov_ctx)
//{
//	return 0;
//}
static const OSSL_DISPATCH rlg_prov_functions[] = {
	{ OSSL_FUNC_PROVIDER_TEARDOWN,				(void (*)(void))rlg_provider_teardown },
	{ OSSL_FUNC_PROVIDER_GETTABLE_PARAMS,		(void (*)(void))rlg_provider_gettable_params },
	{ OSSL_FUNC_PROVIDER_GET_PARAMS,			(void (*)(void))rlg_provider_get_params },
	{ OSSL_FUNC_PROVIDER_QUERY_OPERATION,		(void (*)(void))rlg_provider_query },
//	{ OSSL_FUNC_PROVIDER_UNQUERY_OPERATION,		(void (*)(void))rlg_provider_unquery_operation },
//	{ OSSL_FUNC_PROVIDER_GET_REASON_STRINGS,	(void (*)(void))rlg_provider_get_reason_strings },
//	{ OSSL_FUNC_PROVIDER_GET_CAPABILITIES,		(void (*)(void))rlg_provider_get_capabilities },
//	{ OSSL_FUNC_PROVIDER_SELF_TEST,				(void (*)(void))rlg_provider_self_test },
	{ 0, NULL }
};
int RLOGIN_provider_init(const OSSL_CORE_HANDLE *handle, const OSSL_DISPATCH *in, const OSSL_DISPATCH **out, void **provctx)
{
	struct prov_ctx_st *prov_ctx;
	
	if ( (prov_ctx = (struct prov_ctx_st *)malloc(sizeof(struct prov_ctx_st))) == NULL )
		return 0;

	prov_ctx->handle = handle;
	prov_ctx->in = in;
	prov_ctx->status = 1;

	*provctx = prov_ctx;
	*out = rlg_prov_functions;

	return 1;
 }
void RLOGIN_provider_finish()
{
	for ( int n = 0 ; n < NIDSCIPHERMAX ; n++ ) {
		if ( rlg_evp_cipher_tab[n] != NULL ) {
			EVP_CIPHER_free(rlg_evp_cipher_tab[n]);
			rlg_evp_cipher_tab[n] = NULL;
		}
	}
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

#ifdef	USE_NOENDIAN
#define LOAD_UINT32_LITTLE(ptr)		((((UINT32)(((UINT8 *)(ptr))[0])) <<  0) | \
									 (((UINT32)(((UINT8 *)(ptr))[1])) <<  8) | \
									 (((UINT32)(((UINT8 *)(ptr))[2])) << 16) | \
									 (((UINT32)(((UINT8 *)(ptr))[3])) << 24))
#else
#define LOAD_UINT32_LITTLE(ptr)     (*(UINT32 *)(ptr))
#endif

//////////////////////////////////////////////////////////////////////

static void aes_kdf(void *bufp, AES_KEY *key, UINT8 ndx, int nbytes)
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

    aes_kdf(buf, prf_key, 1, n);

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
	aes_kdf(buf, prf_key, 2, (pc->streams * 8 + 4) * sizeof(UINT64));    /* Fill buffer with index 1 key */

    for ( i = 0 ; i < pc->streams ; i++ ) {
		pc->poly_key_8[i] = endian_big_64(&(buf[24 * i]));
        /* Mask the 64-bit keys to their special domain */
        pc->poly_key_8[i] &= ((UINT64)0x01ffffffu << 32) + 0x01ffffffu;
        pc->poly_accum[i] = 1;  /* Our polyhash prepends a non-zero word */
    }
    
    /* Setup L3-1 hash variables */
	aes_kdf(buf, prf_key, 3,  (pc->streams * 8 + 4) * sizeof(UINT64)); /* Fill buffer with index 2 key */

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
	aes_kdf(buf, prf_key, 4, pc->streams * sizeof(UINT32));

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

    aes_kdf(buf, &prf_key, 0, UMAC_KEY_LEN);
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

int	HMAC_digest(const EVP_MD *md, BYTE *key, int keylen, BYTE *in, int inlen, BYTE *out, int outlen)
{
#ifdef	USE_MACCTX
	EVP_MAC *mac = NULL;
	EVP_MAC_CTX *ctx = NULL;
	const char *digest;
	OSSL_PARAM params[3];
	size_t dlen = 0;

	if ( md == NULL )
		goto ENDOF;

	if ( (digest = EVP_MD_get0_name(md)) == NULL )
		goto ENDOF;

	//if ( EVP_Q_mac(NULL, "hmac", NULL, digest, NULL, key, keylen, in, inlen, out, outlen, &dlen) == NULL )
	//	return 0;

	if ( (mac = EVP_MAC_fetch(NULL, "hmac", NULL)) == NULL )
		goto ENDOF;

	params[0] = OSSL_PARAM_construct_utf8_string("digest", (char *)digest, 0);
	params[1] = OSSL_PARAM_construct_end();

	if ( (ctx = EVP_MAC_CTX_new(mac)) == NULL )
		goto ENDOF;

	if ( EVP_MAC_init(ctx, (const unsigned char *)key, (size_t)keylen, params) <= 0 )
		goto ENDOF;

	if ( EVP_MAC_update(ctx, (const unsigned char *)in, (size_t)inlen) <= 0 )
		goto ENDOF;

	if ( EVP_MAC_final(ctx, out, &dlen, outlen) <= 0 )
		goto ENDOF;

ENDOF:
	if ( ctx != NULL )
		EVP_MAC_CTX_free(ctx);

	if ( mac != NULL )
		EVP_MAC_free(mac);

#else
	HMAC_CTX *ctx = NULL;
	unsigned int dlen = 0;

	if ( md == NULL )
		goto ENDOF;

	if ( (ctx = HMAC_CTX_new()) == NULL )
		goto ENDOF;

	if ( HMAC_Init(ctx, key, keylen, md) <= 0 )
		goto ENDOF;

	if ( HMAC_Update(ctx, in, inlen) <= 0 )
		goto ENDOF;

	if ( outlen < EVP_MD_size(md) )
		goto ENDOF;

	if ( HMAC_Final(ctx, out, &dlen) <= 0 )
		goto ENDOF;

ENDOF:
	if ( ctx != NULL )
		HMAC_CTX_free(ctx);
#endif

	return (int)dlen;
}
int	MD_digest(const EVP_MD *md, BYTE *in, int inlen, BYTE *out, int outlen)
{
	EVP_MD_CTX *ctx = NULL;
	unsigned dlen = 0;

	if ( md == NULL )
		goto ENDOF;

	if ( (ctx = EVP_MD_CTX_new()) == NULL )
		goto ENDOF;

	if ( EVP_DigestInit(ctx, md) <= 0 )
		goto ENDOF;

	if ( EVP_DigestUpdate(ctx, in, inlen) <= 0 )
		goto ENDOF;

	if ( outlen < EVP_MD_size(md) )
		goto ENDOF;

	if ( EVP_DigestFinal(ctx, out, &dlen) <= 0 )
		goto ENDOF;

ENDOF:
	if ( ctx != NULL )
		EVP_MD_CTX_free(ctx);

	return dlen;
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

#define	crypto_hash_sha512(out, in, inlen)	SHA512(in, inlen, out)

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

	// opensslのblowfishとは、初期化方法が違う(salt)ので利用できない

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
	if (passlen == 0 || saltlen == 0 || keylen == 0 || keylen > 1024 || saltlen > (1 << 20) )
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
// PuTTY-0.75-pre in sshblake2.c and sshargon2.c

/*
 * Implementation of the Argon2 password hash function.
 *
 * My sources for the algorithm description and test vectors (the latter in
 * test/cryptsuite.py) were the reference implementation on Github, and also
 * the Internet-Draft description:
 *
 *   https://github.com/P-H-C/phc-winner-argon2
 *   https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-argon2-13
 */

static inline uint32_t GET_32BIT_LSB_FIRST(const void *vp)
{
#ifdef	USE_NOENDIAN
    const uint8_t *p = (const uint8_t *)vp;
    return (((uint64_t)p[0]	 ) |
			((uint64_t)p[1] <<  8) |
		    ((uint64_t)p[2] << 16) |
			((uint64_t)p[3] << 24));
#else
	return *((uint32_t *)vp);
#endif
}

static inline void PUT_32BIT_LSB_FIRST(void *vp, uint32_t value)
{
#ifdef	USE_NOENDIAN
    uint8_t *p = (uint8_t *)vp;
    p[0] = (uint8_t)(value);
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
#else
	*((uint32_t *)vp) = value;
#endif
}

static inline uint64_t GET_64BIT_LSB_FIRST(const void *vp)
{
#ifdef	USE_NOENDIAN
    const uint8_t *p = (const uint8_t *)vp;
    return (((uint64_t)p[0]	 ) |
			((uint64_t)p[1] <<  8) |
		    ((uint64_t)p[2] << 16) |
			((uint64_t)p[3] << 24) |
			((uint64_t)p[4] << 32) |
			((uint64_t)p[5] << 40) |
			((uint64_t)p[6] << 48) |
			((uint64_t)p[7] << 56));
#else
	return *((uint64_t *)vp);
#endif
}

static inline void PUT_64BIT_LSB_FIRST(void *vp, uint64_t value)
{
#ifdef	USE_NOENDIAN
    uint8_t *p = (uint8_t *)vp;
    p[0] = (uint8_t)(value);
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
    p[4] = (uint8_t)(value >> 32);
    p[5] = (uint8_t)(value >> 40);
    p[6] = (uint8_t)(value >> 48);
    p[7] = (uint8_t)(value >> 56);

#else
	*((uint64_t *)vp) = value;
#endif
}

static inline uint64_t ror64(uint64_t x, unsigned rotation)
{
    return (x << (64 - (rotation & 63))) | (x >> (rotation & 63));
}

static inline uint64_t trunc32(uint64_t x)
{
    return x & 0xFFFFFFFF;
}

static void memxor(uint8_t *out, const uint8_t *in1, const uint8_t *in2, size_t size)
{
	register size_t n = size / sizeof(size_t);
	register size_t *pout = (size_t *)out;
	register size_t *pin1 = (size_t *)in1;
	register size_t *pin2 = (size_t *)in2;

	while ( n-- > 0 )
		*(pout++) = *(pin1++) ^ *(pin2++);

	if ( (n = size % sizeof(size_t)) > 0 ) {
		out = (uint8_t *)pout;
		in1 = (uint8_t *)pin1;
		in2 = (uint8_t *)pin2;
#if 0
		while ( n-- > 0 )
		   *(out++) = *(in1++) ^ *(in2++);
#else
		ASSERT(sizeof(size_t) <= 8);
		switch(n) {
		case 7:
		   *(out++) = *(in1++) ^ *(in2++);
		case 6:
		   *(out++) = *(in1++) ^ *(in2++);
		case 5:
		   *(out++) = *(in1++) ^ *(in2++);
		case 4:
		   *(out++) = *(in1++) ^ *(in2++);
		case 3:
		   *(out++) = *(in1++) ^ *(in2++);
		case 2:
		   *(out++) = *(in1++) ^ *(in2++);
		case 1:
		   *(out++) = *(in1++) ^ *(in2++);
		}
#endif
	}
}

///////////////////////////////////////////////////////////////////////////

static void put_uint32_le(CBuffer *buf, uint32_t val)
{
    unsigned char data[4];
    PUT_32BIT_LSB_FIRST(data, val);
	buf->Apend(data, 4);
}
static void put_stringpl_le(CBuffer *buf, CBuffer *data)
{
	put_uint32_le(buf, data->GetSize());
	buf->Apend(data->GetPtr(), data->GetSize());
}
static void inline put_data_ptr(CBuffer *buf, void *data, size_t len)
{
	buf->Apend((LPBYTE)data, (int)len);
}
static void inline put_buffer_clear(CBuffer *buf)
{
	buf->Clear();
}

///////////////////////////////////////////////////////////////////////////
/*
 * BLAKE2 (RFC 7693) implementation for PuTTY.
 *
 * The BLAKE2 hash family includes BLAKE2s, in which the hash state is
 * operated on as a collection of 32-bit integers, and BLAKE2b, based
 * on 64-bit integers. At present this code implements BLAKE2b only.
 */

/* RFC 7963 section 2.1 */
enum { R1 = 32, R2 = 24, R3 = 16, R4 = 63 };

/* RFC 7693 section 2.6 */
static const uint64_t blake2_iv[] = {
    0x6a09e667f3bcc908,                /* floor(2^64 * frac(sqrt(2)))  */
    0xbb67ae8584caa73b,                /* floor(2^64 * frac(sqrt(3)))  */
    0x3c6ef372fe94f82b,                /* floor(2^64 * frac(sqrt(5)))  */
    0xa54ff53a5f1d36f1,                /* floor(2^64 * frac(sqrt(7)))  */
    0x510e527fade682d1,                /* floor(2^64 * frac(sqrt(11))) */
    0x9b05688c2b3e6c1f,                /* floor(2^64 * frac(sqrt(13))) */
    0x1f83d9abfb41bd6b,                /* floor(2^64 * frac(sqrt(17))) */
    0x5be0cd19137e2179,                /* floor(2^64 * frac(sqrt(19))) */
};

/* RFC 7693 section 2.7 */
static const unsigned char blake2_sigma[][16] = {
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
    {14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3},
    {11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4},
    { 7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8},
    { 9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13},
    { 2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9},
    {12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11},
    {13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10},
    { 6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5},
    {10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0},
    /* This array recycles if you have more than 10 rounds. BLAKE2b
     * has 12, so we repeat the first two rows again. */
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
    {14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3},
};

static inline void blake2_g_half(uint64_t v[16], unsigned a, unsigned b, unsigned c,
                          unsigned d, uint64_t x, unsigned r1, unsigned r2)
{
    v[a] += v[b] + x;
    v[d] ^= v[a];
    v[d] = ror64(v[d], r1);
    v[c] += v[d];
    v[b] ^= v[c];
    v[b] = ror64(v[b], r2);
}

static inline void blake2_g(uint64_t v[16], unsigned a, unsigned b, unsigned c,
                     unsigned d, uint64_t x, uint64_t y)
{
    blake2_g_half(v, a, b, c, d, x, R1, R2);
    blake2_g_half(v, a, b, c, d, y, R3, R4);
}

static inline void blake2_f(uint64_t h[8], uint64_t m[16], uint64_t offset_hi,
                     uint64_t offset_lo, unsigned final)
{
    uint64_t v[16];
    memcpy(v, h, 8 * sizeof(*v));
    memcpy(v + 8, blake2_iv, 8 * sizeof(*v));
    v[12] ^= offset_lo;
    v[13] ^= offset_hi;
    v[14] ^= (uint64_t)(-(int64_t)final);
    for (unsigned round = 0; round < 12; round++) {
        const unsigned char *s = blake2_sigma[round];
        blake2_g(v,  0,  4,  8, 12, m[s[ 0]], m[s[ 1]]);
        blake2_g(v,  1,  5,  9, 13, m[s[ 2]], m[s[ 3]]);
        blake2_g(v,  2,  6, 10, 14, m[s[ 4]], m[s[ 5]]);
        blake2_g(v,  3,  7, 11, 15, m[s[ 6]], m[s[ 7]]);
        blake2_g(v,  0,  5, 10, 15, m[s[ 8]], m[s[ 9]]);
        blake2_g(v,  1,  6, 11, 12, m[s[10]], m[s[11]]);
        blake2_g(v,  2,  7,  8, 13, m[s[12]], m[s[13]]);
        blake2_g(v,  3,  4,  9, 14, m[s[14]], m[s[15]]);
    }
    for (unsigned i = 0; i < 8; i++)
        h[i] ^= v[i] ^ v[i+8];
    SecureZeroMemory(v, sizeof(v));
}

static inline void blake2_f_outer(uint64_t h[8], uint8_t blk[128], uint64_t offset_hi,
                           uint64_t offset_lo, unsigned final)
{
    uint64_t m[16];
    for (unsigned i = 0; i < 16; i++)
        m[i] = GET_64BIT_LSB_FIRST(blk + 8*i);
    blake2_f(h, m, offset_hi, offset_lo, final);
    SecureZeroMemory(m, sizeof(m));
}

typedef struct blake2b {
    uint64_t h[8];
    unsigned hashlen;

    uint8_t block[128];
    size_t used;
    uint64_t lenhi, lenlo;
} blake2b;

static void blake2b_reset(blake2b *s, unsigned hashlen)
{
	s->hashlen = hashlen;

    /* Initialise the hash to the standard IV */
    memcpy(s->h, blake2_iv, sizeof(s->h));

    /* XOR in the parameters: secret key length (here always 0) in
     * byte 1, and hash length in byte 0. */
    s->h[0] ^= 0x01010000 ^ s->hashlen;

    s->used = 0;
    s->lenhi = s->lenlo = 0;
}

static void blake2b_write(blake2b *s, const void *vp, size_t len)
{
    const uint8_t *p = (const uint8_t *)vp;

    while (len > 0) {
        if (s->used == sizeof(s->block)) {
            blake2_f_outer(s->h, s->block, s->lenhi, s->lenlo, 0);
            s->used = 0;
        }

        size_t chunk = sizeof(s->block) - s->used;
        if (chunk > len)
            chunk = len;

        memcpy(s->block + s->used, p, chunk);
        s->used += chunk;
        p += chunk;
        len -= chunk;

        s->lenlo += chunk;
        s->lenhi += (s->lenlo < chunk);
    }
}

static void blake2b_digest(blake2b *s, uint8_t *digest)
{
    memset(s->block + s->used, 0, sizeof(s->block) - s->used);
    blake2_f_outer(s->h, s->block, s->lenhi, s->lenlo, 1);

    uint8_t hash_pre[128];

    for (unsigned i = 0; i < 8; i++)
        PUT_64BIT_LSB_FIRST(hash_pre + 8*i, s->h[i]);

	memcpy(digest, hash_pre, s->hashlen);
	SecureZeroMemory(hash_pre, sizeof(hash_pre));
}

///////////////////////////////////////////////////////////////////////////

static void hprime_hash(CBuffer *data, uint8_t *out, int len)
{
	blake2b s;
	uint8_t hash[64];

	// opensslのEVP_blake2b512とs->hのイニシャル方法が違うので利用できない

	blake2b_reset(&s, len > 64 ? 64 : len);
	blake2b_write(&s, data->GetPtr(), data->GetSize());

	while ( len > 64 ) {
		blake2b_digest(&s, hash);

		memcpy(out, hash, 32);
		out += 32;
		len -= 32;

		blake2b_reset(&s, len > 64 ? 64 : len);
		blake2b_write(&s, hash, 64);

		SecureZeroMemory(hash, sizeof(hash));
	}

	blake2b_digest(&s, out);
}

///////////////////////////////////////////////////////////////////////////

static inline void argon2_GB(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d)
{
    *a += *b + 2 * trunc32(*a) * trunc32(*b);
    *d = ror64(*d ^ *a, 32);
    *c += *d + 2 * trunc32(*c) * trunc32(*d);
    *b = ror64(*b ^ *c, 24);
    *a += *b + 2 * trunc32(*a) * trunc32(*b);
    *d = ror64(*d ^ *a, 16);
    *c += *d + 2 * trunc32(*c) * trunc32(*d);
    *b = ror64(*b ^ *c, 63);
}

static inline void argon2_P(uint64_t *out, unsigned outstep,
                     uint64_t *in, unsigned instep)
{
    for ( unsigned i = 0 ; i < 8 ; i++ ) {
        out[i*outstep] = in[i*instep];
        out[i*outstep+1] = in[i*instep+1];
    }

    argon2_GB(out+0*outstep+0, out+2*outstep+0, out+4*outstep+0, out+6*outstep+0);
    argon2_GB(out+0*outstep+1, out+2*outstep+1, out+4*outstep+1, out+6*outstep+1);
    argon2_GB(out+1*outstep+0, out+3*outstep+0, out+5*outstep+0, out+7*outstep+0);
    argon2_GB(out+1*outstep+1, out+3*outstep+1, out+5*outstep+1, out+7*outstep+1);

    argon2_GB(out+0*outstep+0, out+2*outstep+1, out+5*outstep+0, out+7*outstep+1);
    argon2_GB(out+0*outstep+1, out+3*outstep+0, out+5*outstep+1, out+6*outstep+0);
    argon2_GB(out+1*outstep+0, out+3*outstep+1, out+4*outstep+0, out+6*outstep+1);
    argon2_GB(out+1*outstep+1, out+2*outstep+0, out+4*outstep+1, out+7*outstep+0);
}

static void argon2_G_xor(uint8_t *out, const uint8_t *X, const uint8_t *Y)
{
    uint64_t R[128], Q[128], Z[128];

    for (unsigned i = 0; i < 128; i++)
        R[i] = GET_64BIT_LSB_FIRST(X + 8*i) ^ GET_64BIT_LSB_FIRST(Y + 8*i);

    for (unsigned i = 0; i < 8; i++)
        argon2_P(Q+16*i, 2, R+16*i, 2);

    for (unsigned i = 0; i < 8; i++)
        argon2_P(Z+2*i, 16, Q+2*i, 16);

    for (unsigned i = 0; i < 128; i++)
        PUT_64BIT_LSB_FIRST(out + 8*i, GET_64BIT_LSB_FIRST(out + 8*i) ^ R[i] ^ Z[i]);

    SecureZeroMemory(R, sizeof(R));
    SecureZeroMemory(Q, sizeof(Q));
    SecureZeroMemory(Z, sizeof(Z));
}

//void argon2(uint32_t flavour, uint32_t mem, uint32_t passes, uint32_t parallel, 
//	CBuffer *P, CBuffer *S, CBuffer *K, CBuffer *X, 
//	u_char *out, uint32_t taglen);

void argon2(uint32_t y, uint32_t m, uint32_t t, uint32_t p,
	CBuffer *P, CBuffer *S, CBuffer *K, CBuffer *X,
	u_char *out, uint32_t T)
{
	CBuffer tmp(-1);
	uint8_t h0[64];
	struct blk { uint8_t data[1024]; };

	put_buffer_clear(&tmp);
	put_uint32_le(&tmp, p);
	put_uint32_le(&tmp, T);
	put_uint32_le(&tmp, m);
	put_uint32_le(&tmp, t);
	put_uint32_le(&tmp, 0x13);		/* hash function version number */
	put_uint32_le(&tmp, y);
	put_stringpl_le(&tmp, P);
	put_stringpl_le(&tmp, S);
	put_stringpl_le(&tmp, K);
	put_stringpl_le(&tmp, X);

	hprime_hash(&tmp, h0, 64);

    size_t SL = m / (4 * p);		/* segment length: # of 1Kb blocks in a segment */
    size_t q = 4 * SL;				/* width of the array: 4 segments times SL */
    size_t mprime = q * p;			/* total size of the array, approximately m */

    struct blk *B = new struct blk[mprime];
    memset(B, 0, mprime * sizeof(struct blk));

    for ( size_t i = 0 ; i < p ; i++ ) {
		put_buffer_clear(&tmp);
		put_uint32_le(&tmp, 1024);
		put_data_ptr(&tmp, h0, 64);
		put_uint32_le(&tmp, 0);
		put_uint32_le(&tmp, (uint32_t)i);

		hprime_hash(&tmp, B[i].data, 1024);
	}

	for ( size_t i = 0 ; i < p ; i++ ) {
		put_buffer_clear(&tmp);
		put_uint32_le(&tmp, 1024);
		put_data_ptr(&tmp, h0, 64);
		put_uint32_le(&tmp, 1);
		put_uint32_le(&tmp, (uint32_t)i);

		hprime_hash(&tmp, B[i + p].data, 1024);
	}

	size_t jstart = 2;
    bool d_mode = (y == 0);
    struct blk out2i, tmp2i, in2i;

    for (size_t pass = 0; pass < t; pass++) {
        for (unsigned slice = 0; slice < 4; slice++) {
            if (pass == 0 && slice == 2 && y == 2)
                d_mode = true;

            for (size_t i = 0; i < p; i++) {
                for (size_t jpre = jstart; jpre < SL; jpre++) {
                    size_t j = slice * SL + jpre;
                    uint32_t jm1 = (uint32_t)(j == 0 ? q-1 : j-1);
                    uint32_t J1, J2;

                    if (d_mode) {
                        J1 = GET_32BIT_LSB_FIRST(B[i + p * jm1].data);
                        J2 = GET_32BIT_LSB_FIRST(B[i + p * jm1].data + 4);
                    } else {
                        if (jpre == jstart || jpre % 128 == 0) {
                            memset(in2i.data, 0, sizeof(in2i.data));
                            PUT_64BIT_LSB_FIRST(in2i.data +  0, pass);
                            PUT_64BIT_LSB_FIRST(in2i.data +  8, i);
                            PUT_64BIT_LSB_FIRST(in2i.data + 16, slice);
                            PUT_64BIT_LSB_FIRST(in2i.data + 24, mprime);
                            PUT_64BIT_LSB_FIRST(in2i.data + 32, t);
                            PUT_64BIT_LSB_FIRST(in2i.data + 40, y);
                            PUT_64BIT_LSB_FIRST(in2i.data + 48, jpre / 128 + 1);

                            memset(tmp2i.data, 0, sizeof(tmp2i.data));
                            argon2_G_xor(tmp2i.data, tmp2i.data, in2i.data);
                            memset(out2i.data, 0, sizeof(out2i.data));
                            argon2_G_xor(out2i.data, out2i.data, tmp2i.data);
                        }
                        J1 = GET_32BIT_LSB_FIRST(
                            out2i.data + 8 * (jpre % 128));
                        J2 = GET_32BIT_LSB_FIRST(
                            out2i.data + 8 * (jpre % 128) + 4);
                    }

                    uint32_t index_l = (uint32_t)((pass == 0 && slice == 0) ? i : J2 % p);
                    uint32_t Wstart = (uint32_t)(pass == 0 ? 0 : (slice + 1) % 4 * SL);
                    uint32_t Wend;

                    if (index_l == i) {
                        Wend = jm1;
                    } else {
                        Wend = (uint32_t)(SL * slice);
                        if (jpre == 0)
                            Wend = (uint32_t)((Wend + q-1) % q);
                    }

                    uint32_t Wsize = (uint32_t)((Wend + q - Wstart) % q);

					uint32_t x = ((uint64_t)J1 * J1) >> 32;
                    uint32_t y = ((uint64_t)Wsize * x) >> 32;
                    uint32_t zz = Wsize - 1 - y;

                    uint32_t index_z = (Wstart + zz) % q;

                    argon2_G_xor(B[i + p * j].data, B[i + p * jm1].data,
                          B[index_l + p * index_z].data);
                }
            }

			jstart = 0;
        }
    }

    struct blk C = B[p * (q-1)];

    for ( size_t i = 1 ; i < p ; i++ )
        memxor(C.data, C.data, B[i + p * (q-1)].data, 1024);

	put_buffer_clear(&tmp);
	put_uint32_le(&tmp, T);
	put_data_ptr(&tmp, C.data, 1024);
	hprime_hash(&tmp, out, T);

    SecureZeroMemory(out2i.data, sizeof(out2i.data));
    SecureZeroMemory(tmp2i.data, sizeof(tmp2i.data));
    SecureZeroMemory(in2i.data, sizeof(in2i.data));
    SecureZeroMemory(C.data, sizeof(C.data));
    SecureZeroMemory(B, mprime * sizeof(struct blk));

	delete [] B;
}

//////////////////////////////////////////////////////////////////////
// OpenSSL ml_kem lib

int ossl_mlkem_keypair(uint8_t *pk, uint8_t *sk, int type)
{
	int rt = (-1);
	ML_KEM_KEY *private_key = NULL;
	ML_KEM_KEY *public_key = NULL;
	const ML_KEM_VINFO *v;

	if ( (public_key = ossl_ml_kem_key_new(NULL, NULL, type)) == NULL )
		goto ENDOF;

	if ( (private_key = ossl_ml_kem_key_new(NULL, NULL, type)) == NULL )
		goto ENDOF;

	if ( (v = ossl_ml_kem_key_vinfo(public_key)) == NULL )
		goto ENDOF;

#ifdef	_DEBUG
	switch(type) {
	case EVP_PKEY_ML_KEM_512:
		ASSERT(	v->pubkey_bytes == mlkem512_PUBLICKEYBYTES ||
				v->prvkey_bytes == mlkem512_SECRETKEYBYTES ||
				v->ctext_bytes  == mlkem512_CIPHERTEXTBYTES ||
				ML_KEM_SHARED_SECRET_BYTES == mlkem512_BYTES );
		break;
	case EVP_PKEY_ML_KEM_768:
		ASSERT(	v->pubkey_bytes == mlkem768_PUBLICKEYBYTES ||
				v->prvkey_bytes == mlkem768_SECRETKEYBYTES ||
				v->ctext_bytes  == mlkem768_CIPHERTEXTBYTES ||
				ML_KEM_SHARED_SECRET_BYTES == mlkem768_BYTES );
		break;
	case EVP_PKEY_ML_KEM_1024:
		ASSERT(	v->pubkey_bytes == mlkem1024_PUBLICKEYBYTES ||
				v->prvkey_bytes == mlkem1024_SECRETKEYBYTES ||
				v->ctext_bytes  == mlkem1024_CIPHERTEXTBYTES ||
				ML_KEM_SHARED_SECRET_BYTES == mlkem1024_BYTES );
		break;
	}
#endif

	if ( !ossl_ml_kem_genkey(pk, v->pubkey_bytes, private_key) )
		goto ENDOF;

	if ( !ossl_ml_kem_encode_private_key(sk, v->prvkey_bytes, private_key) )
		goto ENDOF;

	rt = 0;

ENDOF:
	if ( public_key != NULL )
		ossl_ml_kem_key_free(public_key);

	if ( private_key != NULL )
		ossl_ml_kem_key_free(private_key);

	return rt;
}
int ossl_mlkem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk, int type)
{
	int rt = (-1);
	ML_KEM_KEY *public_key = NULL;
	const ML_KEM_VINFO *v;

	if ( (public_key = ossl_ml_kem_key_new(NULL, NULL, type)) == NULL )
		goto ENDOF;

	if ( (v = ossl_ml_kem_key_vinfo(public_key)) == NULL )
		goto ENDOF;

	if ( !ossl_ml_kem_parse_public_key(pk, v->pubkey_bytes, public_key) )
		goto ENDOF;

	if ( !ossl_ml_kem_encap_rand(ct, v->ctext_bytes, ss, ML_KEM_SHARED_SECRET_BYTES, public_key) )
		goto ENDOF;

	rt = 0;

ENDOF:
	if ( public_key != NULL )
		ossl_ml_kem_key_free(public_key);

	return rt;
}
int ossl_mlkem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk, int type)
{
	int rt = (-1);
	ML_KEM_KEY *private_key = NULL;
	const ML_KEM_VINFO *v;

	if ( (private_key = ossl_ml_kem_key_new(NULL, NULL, type)) == NULL )
		goto ENDOF;

	if ( (v = ossl_ml_kem_key_vinfo(private_key)) == NULL )
		goto ENDOF;

	if ( !ossl_ml_kem_parse_private_key(sk, v->prvkey_bytes, private_key) )
		goto ENDOF;

	if ( !ossl_ml_kem_decap(ss, ML_KEM_SHARED_SECRET_BYTES, ct, v->ctext_bytes, private_key) )
		goto ENDOF;

	rt = 0;

ENDOF:
	if ( private_key != NULL )
		ossl_ml_kem_key_free(private_key);

	return rt;
}
