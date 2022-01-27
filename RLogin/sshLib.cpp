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

enum LEGACYNID {
	Nids_seed_ctr,		Nids_bf_ctr,		Nids_cast5_ctr,		Nids_idea_ctr,		Nids_des3_ctr,
	Nids_ssh1_3des,		Nids_ssh1_bf,		Nids_chachapoly_256,
#ifdef	USE_NETTLE
	Nids_twofish_ctr,	Nids_twofish_cbc,	Nids_serpent_ctr,	Nids_serpent_cbc,
#endif
#ifdef	USE_CLEFIA
	Nids_clefia_ctr,	Nids_clefia_cbc,	
#endif
	LEGACYCIPHERMAX
};
static int LegacyEngineNids[LEGACYCIPHERMAX] = {
	NID_undef,			NID_undef,			NID_undef,			NID_undef,			NID_undef,
	NID_undef,			NID_undef,			NID_undef,			
#ifdef	USE_NETTLE
	NID_undef,			NID_undef,			NID_undef,			NID_undef,
#endif
#ifdef	USE_CLEFIA
	NID_undef,			NID_undef,
#endif
};
static EVP_CIPHER *LegacyEngineCipherMeth[LEGACYCIPHERMAX] = {
	NULL,				NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,
#ifdef	USE_NETTLE
	NULL,				NULL,				NULL,				NULL,
#endif
#ifdef	USE_CLEFIA
	NULL,				NULL,
#endif
};
static const EVP_CIPHER * (*LegacyEngineCipherTab[LEGACYCIPHERMAX])(void) = {
	evp_seed_ctr,		evp_bf_ctr,			evp_cast5_ctr,		evp_idea_ctr,		evp_des3_ctr,
	evp_ssh1_3des,		evp_ssh1_bf,		evp_chachapoly_256,	
#ifdef	USE_NETTLE
	evp_twofish_ctr,	evp_twofish_cbc,	evp_serpent_ctr,	evp_serpent_cbc,	
#endif
#ifdef	USE_CLEFIA
	evp_clefia_ctr,		evp_clefia_cbc,	
#endif
};
static const char *LegacyEngineObj[LEGACYCIPHERMAX][3] = {
	{ "0.21.1.1",	"SEED-CTR",		"seed-ctr"		},	{ "0.21.2.1",	"BLOWFISH-CTR",	"blowfish-ctr"	},
	{ "0.21.3.1",	"CAST5-CTR",	"cast5-ctr"		},	{ "0.21.4.1",	"IDEA-CTR",		"idea-ctr"		},
	{ "0.21.5.1",	"3DES-CTR",		"3des-ctr"		},	{ "0.21.6.1",	"SSH1-3DES",	"ssh1-3des"		},
	{ "0.21.6.2",	"SSH1-BLOWFISH","ssh1-blowfish"	},	{ "0.21.7.1",	"CHACHAPOLYSSH","chachapolyssh"	},
#ifdef	USE_NETTLE
	{ "0.21.8.1",	"TWOFISH-CTR",	"twofish-ctr"	},	{ "0.21.8.2",	"TWOFISH-CBC",	"twofish-cbc"	},
	{ "0.21.9.1",	"SERPENT-CTR",	"serpent-ctr"	},	{ "0.21.9.2",	"SERPENT-CBC",	"serpent-cbc"	},
#endif
#ifdef	USE_CLEFIA
	{ "0.21.10.1",	"CLEFIA-CTR",	"clefia-ctr"	},	{ "0.21.10.2",	"CLEFIA-CBC",	"clefia-cbc"	},
#endif
};
static ENGINE *LegacyEngineCtx = NULL;
static int LegacyEngineMin = 0;

static int LegacyEngineCiphers(ENGINE *e, const EVP_CIPHER **cipher, const int **nids, int nid)
{
	// ENGINE_register_ciphers		int num_nids = e->ciphers(e, NULL, &nids, 0);
	// ENGINE_set_default_ciphers	int num_nids = e->ciphers(e, NULL, &nids, 0);
	// ENGINE_get_cipher			!e->ciphers(e, &ret, NULL, nid)

	if ( nids != NULL ) {
		*nids = LegacyEngineNids;
		return LEGACYCIPHERMAX;
	}

	if ( cipher == NULL )
		return 0;

	int idx = LEGACYCIPHERMAX;

	if ( LegacyEngineMin != 0 && nid >= LegacyEngineMin && nid < (LegacyEngineMin + LEGACYCIPHERMAX) )
		idx = nid - LegacyEngineMin;
	else {
		for ( idx = 0 ; idx <  LEGACYCIPHERMAX ; idx++ ) {
			if ( LegacyEngineNids[idx] == nid )
				break;
		}
	}

	if ( idx >= LEGACYCIPHERMAX || (*cipher = LegacyEngineCipherTab[idx]()) == NULL )
		return 0;

	return 1;
}
int LegacyEngineInit()
{
	int n , min;
	BOOL bSeq = TRUE;

	if ( (LegacyEngineCtx = ENGINE_new()) == NULL )
		return 0;

	for ( n = 0 ; n < LEGACYCIPHERMAX ; n++ ) {
		if ( LegacyEngineNids[n] == NID_undef )
			LegacyEngineNids[n] = OBJ_create(LegacyEngineObj[n][0], LegacyEngineObj[n][1], LegacyEngineObj[n][2]);

		ASSERT(LegacyEngineNids[n] != 0);

		if ( n == 0 )
			min = LegacyEngineNids[n];

		if ( LegacyEngineNids[n] != (min + n) )
			bSeq = FALSE;
	}

	if ( bSeq )
		LegacyEngineMin = min;

	ENGINE_set_id(LegacyEngineCtx, "RLG");
	ENGINE_set_name(LegacyEngineCtx, "RLOGIN");
	ENGINE_set_ciphers(LegacyEngineCtx, LegacyEngineCiphers);

	ENGINE_register_ciphers(LegacyEngineCtx);
	ENGINE_add(LegacyEngineCtx);

	return 1;
}
void LegacyEngineFree()
{
	for ( int n = 0 ; n < LEGACYCIPHERMAX ; n++ ) {
		if ( LegacyEngineCipherMeth[n] != NULL ) {
			EVP_CIPHER_meth_free(LegacyEngineCipherMeth[n]);
			LegacyEngineCipherMeth[n] = NULL;
		}
	}

	if ( LegacyEngineCtx != NULL ) {
		ENGINE_remove(LegacyEngineCtx);
		ENGINE_unregister_ciphers(LegacyEngineCtx);
		ENGINE_free(LegacyEngineCtx);
		LegacyEngineCtx = NULL;
	}
}

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
	if ( LegacyEngineCipherMeth[Nids_seed_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_seed_ctr], SEED_BLOCK_SIZE, 16);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, SEED_BLOCK_SIZE);
			EVP_CIPHER_meth_set_init(cip, ssh_seed_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_seed_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_seed_ctr]; 
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
	if ( LegacyEngineCipherMeth[Nids_bf_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_bf_ctr], BF_BLOCK, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, BF_BLOCK);
			EVP_CIPHER_meth_set_init(cip, ssh_blowfish_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_bf_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_bf_ctr];
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
	if ( LegacyEngineCipherMeth[Nids_cast5_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_cast5_ctr], CAST_BLOCK, CAST_KEY_LENGTH);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, CAST_BLOCK);
			EVP_CIPHER_meth_set_init(cip, ssh_cast_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_cast5_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_cast5_ctr];
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
	if ( LegacyEngineCipherMeth[Nids_idea_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_idea_ctr], IDEA_BLOCK, IDEA_KEY_LENGTH);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, IDEA_BLOCK);
			EVP_CIPHER_meth_set_init(cip, ssh_idea_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_idea_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_idea_ctr];
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
	if ( LegacyEngineCipherMeth[Nids_des3_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_des3_ctr], DES_BLOCK, 24);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, DES_BLOCK);
			EVP_CIPHER_meth_set_init(cip, ssh_des3_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_des3_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_des3_ctr];
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
		if ( EVP_CIPHER_meth_get_do_cipher(EVP_CIPHER_CTX_cipher(ctx)) == ssh_cipher_ctr )
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
	if ( LegacyEngineCipherMeth[Nids_twofish_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_twofish_ctr], TWOFISH_BLOCK_SIZE, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, TWOFISH_BLOCK_SIZE);
			EVP_CIPHER_meth_set_init(cip, ssh_twofish_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_twofish_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_twofish_ctr];
}
const EVP_CIPHER *evp_twofish_cbc(void)
{
	if ( LegacyEngineCipherMeth[Nids_twofish_cbc] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_twofish_cbc], TWOFISH_BLOCK_SIZE, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, TWOFISH_BLOCK_SIZE);
			EVP_CIPHER_meth_set_init(cip, ssh_twofish_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_twofish_cbc);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_twofish_cbc] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_twofish_cbc]; 
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
		if ( EVP_CIPHER_meth_get_do_cipher(EVP_CIPHER_CTX_cipher(ctx)) == ssh_cipher_ctr )
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
	if ( LegacyEngineCipherMeth[Nids_serpent_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_serpent_ctr], SERPENT_BLOCK_SIZE, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, SERPENT_BLOCK_SIZE);
			EVP_CIPHER_meth_set_init(cip, ssh_serpent_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_serpent_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_serpent_ctr]; 
}
const EVP_CIPHER *evp_serpent_cbc(void)
{
	if ( LegacyEngineCipherMeth[Nids_serpent_cbc] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_serpent_cbc], SERPENT_BLOCK_SIZE, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, SERPENT_BLOCK_SIZE);
			EVP_CIPHER_meth_set_init(cip, ssh_serpent_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_serpent_cbc);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_serpent_cbc] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_serpent_cbc]; 
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
		if ( EVP_CIPHER_meth_get_do_cipher(EVP_CIPHER_CTX_cipher(ctx)) == ssh_cipher_ctr )
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
	if ( LegacyEngineCipherMeth[Nids_clefia_ctr] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_clefia_ctr], CLEFIA_BLOCK_SIZE, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, CLEFIA_BLOCK_SIZE);
			EVP_CIPHER_meth_set_init(cip, ssh_clefia_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_cipher_ctr);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_clefia_ctr] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_clefia_ctr];
}
const EVP_CIPHER *evp_clefia_cbc(void)
{
	if ( LegacyEngineCipherMeth[Nids_clefia_cbc] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_clefia_cbc], CLEFIA_BLOCK_SIZE, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_iv_length(cip, CLEFIA_BLOCK_SIZE);
			EVP_CIPHER_meth_set_init(cip, ssh_clefia_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_cipher_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_clefia_cbc);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_clefia_cbc] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_clefia_cbc];
}
#endif	// USE_CLEFIA

//////////////////////////////////////////////////////////////////////

struct ssh1_3des_ctx
{
	DES_key_schedule ks1, ks2, ks3;
	DES_cblock iv;
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

	DES_set_key_unchecked((const_DES_cblock *)k1, &(c->ks1));
	DES_set_key_unchecked((const_DES_cblock *)k2, &(c->ks2));
	DES_set_key_unchecked((const_DES_cblock *)k3, &(c->ks3));
	memset(&(c->iv), 0, sizeof(c->iv));

	return (1);
}
static int ssh1_3des_cbc(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	struct ssh1_3des_ctx *c;

	if ( (c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL )
		return (0);

	int enc = EVP_CIPHER_CTX_encrypting(ctx) ? DES_ENCRYPT : DES_DECRYPT;
	DES_cbc_encrypt(src,  dest, (long)len, &(c->ks1), &(c->iv), enc);
	DES_cbc_encrypt(dest, dest, (long)len, &(c->ks2), &(c->iv), !enc);
	DES_cbc_encrypt(dest, dest, (long)len, &(c->ks3), &(c->iv), enc);

	return (1);
}
static int ssh1_3des_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh1_3des_ctx *c;

	if ( (c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL ) {
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}
const EVP_CIPHER *evp_ssh1_3des(void)
{
	if ( LegacyEngineCipherMeth[Nids_ssh1_3des] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_ssh1_3des], 8, 16);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_init(cip, ssh1_3des_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh1_3des_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh1_3des_cbc);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT);
			LegacyEngineCipherMeth[Nids_ssh1_3des] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_ssh1_3des];
}

//////////////////////////////////////////////////////////////////////

struct bf_ssh1_ctx
{
	BF_KEY key;
	u_char iv[BF_BLOCK];
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
static int bf_ssh1_cipher(EVP_CIPHER_CTX *ctx, u_char *out, const u_char *in, size_t len)
{
	struct bf_ssh1_ctx *c;

	if ( (c = (struct bf_ssh1_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return 0;

	swap_bytes(in, out, (int)len);
	BF_cbc_encrypt(out, out, (long)len, &(c->key), c->iv, EVP_CIPHER_CTX_encrypting(ctx) ? BF_ENCRYPT : BF_DECRYPT);
	swap_bytes(out, out, (int)len);
	return 1;
}
static int bf_ssh1_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct bf_ssh1_ctx *c;

	if ( (c = (struct bf_ssh1_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		if ( (c = (struct bf_ssh1_ctx *)malloc(sizeof(*c))) == NULL )
			return (0);
		memset(c, 0, sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}

	if ( key != NULL )
		BF_set_key(&(c->key), EVP_CIPHER_CTX_key_length(ctx), key);

	return (1);
}
static int bf_ssh1_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh1_3des_ctx *c;

	if ( (c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL ) {
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

const EVP_CIPHER *evp_ssh1_bf(void)
{
	if ( LegacyEngineCipherMeth[Nids_ssh1_bf] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_ssh1_bf], BF_BLOCK, 32);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_init(cip, bf_ssh1_init);
			EVP_CIPHER_meth_set_cleanup(cip, bf_ssh1_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, bf_ssh1_cipher);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT);
			LegacyEngineCipherMeth[Nids_ssh1_bf] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_ssh1_bf];
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

	if ( EVP_MAC_init(ctx, (const unsigned char *)key, (size_t)keylen, params) == 0 )
		goto ENDOF;

	if ( EVP_MAC_update(ctx, (const unsigned char *)in, (size_t)inlen) == 0 )
		goto ENDOF;

	if ( EVP_MAC_final(ctx, out, &dlen, outlen) == 0 )
		goto ENDOF;

ENDOF:
	if ( ctx != NULL )
		EVP_MAC_CTX_free(ctx);

	if ( mac != NULL )
		EVP_MAC_free(mac);

#else
	HMAC_CTX *ctx = NULL;
	unsigned int dlen;

	if ( md == NULL )
		goto ENDOF;

	if ( (ctx = HMAC_CTX_new()) == NULL )
		goto ENDOF;

	if ( HMAC_Init(ctx, key, keylen, md) == 0 )
		goto ENDOF;

	if ( HMAC_Update(ctx, in, inlen) == 0 )
		goto ENDOF;

	if ( outlen < EVP_MD_size(md) )
		goto ENDOF;

	if ( HMAC_Final(ctx, out, &dlen) == 0 )
		goto ENDOF;

ENDOF:
	if ( ctx != NULL )
		HMAC_CTX_free(ctx);
#endif

	return (int)dlen;
}
int	EVP_MD_digest(const EVP_MD *md, BYTE *in, int inlen, BYTE *out, int outlen)
{
	EVP_MD_CTX *ctx = NULL;
	unsigned dlen = 0;

	if ( md == NULL )
		goto ENDOF;

	if ( (ctx = EVP_MD_CTX_new()) == NULL )
		goto ENDOF;

	if ( EVP_DigestInit(ctx, md) == 0 )
		goto ENDOF;

	if ( EVP_DigestUpdate(ctx, in, inlen) == 0 )
		goto ENDOF;

	if ( outlen < EVP_MD_size(md) )
		goto ENDOF;

	if ( EVP_DigestFinal(ctx, out, &dlen) == 0 )
		goto ENDOF;

ENDOF:
	if ( ctx != NULL )
		EVP_MD_CTX_free(ctx);

	return dlen;
}

//////////////////////////////////////////////////////////////////////
//	chacha
//////////////////////////////////////////////////////////////////////

/*
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

typedef struct _chacha_ctx {
	u_int input[16];
} chacha_ctx;

typedef unsigned char u8;
typedef unsigned int u32;

#define U8C(v) (v##U)
#define U32C(v) (v##U)

#define U8V(v) ((u8)(v) & U8C(0xFF))
#define U32V(v) ((u32)(v) & U32C(0xFFFFFFFF))

#define ROTL32(v, n) (U32V((v) << (n)) | ((v) >> (32 - (n))))

#if 0
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
#else
#define U8TO32_LITTLE(p) *((u32 *)(p))
#define U32TO8_LITTLE(p, v) *((u32 *)(p))=(v)
#endif

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

static void chacha_encrypt_bytes(chacha_ctx *x,const u8 *m,u8 *c,u32 bytes)
{
	ChaCha20_ctr32(c, m, bytes, x->input + 4, x->input + 12);
}

//////////////////////////////////////////////////////////////////////
//	chachapoly
//////////////////////////////////////////////////////////////////////

struct ssh_chachapoly_ctx
{
	chacha_ctx main_ctx, header_ctx;
	u_char seqbuf[8];
	u_char expected_tag[POLY1305_DIGEST_SIZE];
	u_char poly_key[POLY1305_KEY_SIZE];
	POLY1305 *poly_ctx;
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
		memset(c->poly_key, 0, POLY1305_KEY_SIZE);
		if ( ptr != NULL && arg == 8 )
			memcpy(c->seqbuf, ptr, arg);
		chacha_ivsetup(&c->main_ctx, c->seqbuf, NULL);
		chacha_encrypt_bytes(&c->main_ctx, c->poly_key, c->poly_key, POLY1305_KEY_SIZE);
		break;

	case EVP_CTRL_POLY_GET_TAG:
		if ( ptr != NULL && arg >= POLY1305_DIGEST_SIZE )
			memcpy(ptr, c->expected_tag, arg);
		break;

	case EVP_CTRL_POLY_SET_TAG:
		if ( ptr != NULL && arg > 0 ) {
			Poly1305_Init(c->poly_ctx, c->poly_key);
			Poly1305_Update(c->poly_ctx, (const unsigned char *)ptr, arg);
			Poly1305_Final(c->poly_ctx, c->expected_tag);
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

		if ( (c->poly_ctx = (POLY1305 *)malloc(Poly1305_ctx_size())) == NULL )
			return 0;
		memset(c->poly_ctx, 0, Poly1305_ctx_size());

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
		free(c->poly_ctx);
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}
const EVP_CIPHER *evp_chachapoly_256(void)
{
	if ( LegacyEngineCipherMeth[Nids_chachapoly_256] == NULL ) {
		EVP_CIPHER *cip = EVP_CIPHER_meth_new(LegacyEngineNids[Nids_chachapoly_256], 8, CHACHA_KEY_SIZE * 2);
		if ( cip != NULL ) {
			EVP_CIPHER_meth_set_init(cip, ssh_chachapoly_init);
			EVP_CIPHER_meth_set_cleanup(cip, ssh_chachapoly_cleanup);
			EVP_CIPHER_meth_set_do_cipher(cip, ssh_chachapoly_cipher);
			EVP_CIPHER_meth_set_ctrl(cip, ssh_chachapoly_ctrl);
			EVP_CIPHER_meth_set_flags(cip, EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV);
			LegacyEngineCipherMeth[Nids_chachapoly_256] = cip;
		}
	}
	return LegacyEngineCipherMeth[Nids_chachapoly_256];
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
	return *((uint32_t *)vp);
}

static inline void PUT_32BIT_LSB_FIRST(void *vp, uint32_t value)
{
	*((uint32_t *)vp) = value;
}

static inline uint64_t GET_64BIT_LSB_FIRST(const void *vp)
{
	return *((uint64_t *)vp);
}

static inline void PUT_64BIT_LSB_FIRST(void *vp, uint64_t value)
{
	*((uint64_t *)vp) = value;
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

	// opensslのEVP_blake2b512は、ハッシュサイズ固定(64)なので使用できない

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
