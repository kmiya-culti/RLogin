/* $OpenBSD: cipher-ctr.c,v 1.11 2010/10/01 23:05:32 djm Exp $ */
/*
 * Copyright (c) 2003 Markus Friedl <markus@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "includes.h"

#include <sys/types.h>

#include <stdarg.h>
#include <string.h>

#include <openssl/evp.h>

#include "xmalloc.h"
#include "log.h"

/* compatibility with old or broken OpenSSL versions */
#include "openbsd-compat/openssl-compat.h"

#ifndef USE_BUILTIN_RIJNDAEL
#include <openssl/aes.h>
#endif

#include "openssl/camellia.h"
#include "openssl/blowfish.h"
#include "openssl/des.h"

#include <pthread.h>

#define	MT_BUF_SIZE	16
#define	MT_BUF_MAX	(8 * 1024)
#define	MT_BUF_MIN	(MT_BUF_MAX - 1024)

struct mt_ctr_ctx {
	int		top;
	int		pos;
	int		len;
	void		*ctx;
	void		(*enc)(void *ctx, u_char *buf);

	int		endof;
	int		usereq;
	pthread_t	tid;
	pthread_mutex_t	mutex;
	pthread_cond_t	req_cond;
	pthread_mutex_t	req_mutex;
	pthread_cond_t	ans_cond;
	pthread_mutex_t	ans_mutex;

	u_char		th_ctr[MT_BUF_SIZE];
	u_char		rv_ctr[MT_BUF_SIZE];
	u_char		buf[MT_BUF_MAX][MT_BUF_SIZE];
};

static void *mt_proc(void *x)
{
	struct mt_ctr_ctx *c = (struct mt_ctr_ctx *)x;

	while ( c->endof == 0 ) {
		pthread_mutex_lock(&c->mutex);
		pthread_mutex_lock(&c->req_mutex);
		while ( c->len < MT_BUF_MAX ) {
			pthread_mutex_unlock(&c->mutex);
			(*(c->enc))(c->ctx, c->buf[c->top]);
			pthread_mutex_lock(&c->mutex);
			c->top = (c->top + 1) % MT_BUF_MAX;
			if ( c->len++ < 512 )
			    pthread_cond_signal(&c->ans_cond);
		}
		c->usereq = 0;
		pthread_mutex_unlock(&c->mutex);
		pthread_cond_wait(&c->req_cond, &c->req_mutex);
	}
	return NULL;
}
static int mt_init(struct mt_ctr_ctx *c, void *ctx, void (*enc)(void *ctx, u_char *buf))
{
	c->top = c->pos = c->len = 0;
	c->ctx = ctx;
	c->enc = enc;

	c->endof = 1;
	c->usereq = 1;
	pthread_mutex_init(&c->mutex, NULL);

	pthread_cond_init(&c->req_cond, NULL);
	pthread_mutex_init(&c->req_mutex, NULL);

	pthread_cond_init(&c->ans_cond, NULL);
	pthread_mutex_init(&c->ans_mutex, NULL);

	return 0;
}
static void mt_finis(struct mt_ctr_ctx *c)
{
	if ( c->endof == 0 ) {
		c->endof = 1;
		pthread_cond_signal(&c->req_cond);
		pthread_join(c->tid, NULL);
	}
	c->top = c->pos = c->len = 0;
}
static u_char *mt_get_buf(struct mt_ctr_ctx *c)
{
	if ( c->endof != 0 ) {
		c->endof = 0;
		memcpy(c->th_ctr, c->rv_ctr, MT_BUF_SIZE);
		pthread_create(&c->tid, NULL, mt_proc, (void *)c);
	}

	while ( c->len == 0 ) {
		pthread_mutex_lock(&c->ans_mutex);
		pthread_cond_wait(&c->ans_cond, &c->ans_mutex);
	}

	return c->buf[c->pos];
}
static void mt_inc_buf(struct mt_ctr_ctx *c)
{
	pthread_mutex_lock(&c->mutex);
	c->pos = (c->pos + 1) % MT_BUF_MAX;
	if ( --c->len < MT_BUF_MIN && c->usereq == 0 ) {
		pthread_cond_signal(&c->req_cond);
		c->usereq = 1;
	}
	pthread_mutex_unlock(&c->mutex);
}

/******************************************************
	cipher-ctr
*******************************************************/

struct ssh_cipher_ctx {
	struct mt_ctr_ctx	mt_ctx;
};

/*
 * increment counter 'ctr',
 * the counter is of size 'len' bytes and stored in network-byte-order.
 * (LSB at ctr[len-1], MSB at ctr[0])
 */
static void ssh_ctr_inc(u_char *ctr, size_t len)
{
	ctr += len;
	while ( --len > 0 )
		if ( ++*(--ctr) )   /* continue on overflow */
			return;
}
static int ssh_cipher_ctr(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, u_int len)
{
	struct ssh_cipher_ctx *c;
	register int i;
	register size_t *spdst, *spsrc, *spwrk;
	register u_char *wrk;

	if ((c = (struct ssh_cipher_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	//ASSERT(ctx->cipher->block_size == 8 || ctx->cipher->block_size == 16);
	//ASSERT(sizeof(size_t) == 4 || sizeof(size_t) == 8);

	spdst = (size_t *)dest;
	spsrc = (size_t *)src;

	while ( len >= (u_int)ctx->cipher->block_size ) {
		spwrk = (size_t *)mt_get_buf(&c->mt_ctx);
		ssh_ctr_inc(c->mt_ctx.rv_ctr, ctx->cipher->block_size);

		//for ( i = ctx->cipher->block_size / sizeof(size_t) ; i > 0 ; i-- )
		//	*(spdst++) = *(spsrc++) ^ *(spwrk++);

		switch(ctx->cipher->block_size / sizeof(size_t)) {
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

		len -= ctx->cipher->block_size;
		mt_inc_buf(&c->mt_ctx);
	}

	if ( len > 0 ) {
		spwrk = (size_t *)mt_get_buf(&c->mt_ctx);
		ssh_ctr_inc(c->mt_ctx.rv_ctr, ctx->cipher->block_size);

		for ( i = len / sizeof(size_t) ; i > 0 ; i-- )
			*(spdst++) = *(spsrc++) ^ *(spwrk++);

		dest = (u_char *)spdst;
		src  = (u_char *)spsrc;
		wrk  = (u_char *)spwrk;

		for ( i = len % sizeof(size_t) ; i > 0 ; i-- )
			*(dest++) = *(src++) ^ *(wrk++);

		mt_inc_buf(&c->mt_ctx);
	}

	return (1);
}

/******************************************************
	aes-ctr
*******************************************************/

struct ssh_aes_ctr_ctx
{
	struct mt_ctr_ctx	mt_ctx;
	AES_KEY			aes_ctx;
};

static void ssh_aes_enc(void *ctx, u_char *buf)
{
	struct ssh_aes_ctr_ctx *c = (struct ssh_aes_ctr_ctx *)ctx;

	AES_encrypt(c->mt_ctx.th_ctr, buf, &c->aes_ctx);
	ssh_ctr_inc(c->mt_ctx.th_ctr, AES_BLOCK_SIZE);
}
static int ssh_aes_ctr_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_aes_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = xmalloc(sizeof(*c));
		mt_init(&c->mt_ctx, (void *)c, ssh_aes_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	mt_finis(&c->mt_ctx);

	if (key != NULL)
		AES_set_encrypt_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8, &c->aes_ctx);
	if (iv != NULL)
		memcpy(c->mt_ctx.rv_ctr, iv, AES_BLOCK_SIZE);

	return (1);
}

static int
ssh_aes_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_aes_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		mt_finis(&c->mt_ctx);
		memset(c, 0, sizeof(*c));
		xfree(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

void
ssh_aes_ctr_iv(EVP_CIPHER_CTX *evp, int doset, u_char * iv, size_t len)
{
	struct ssh_aes_ctr_ctx *c;

	if ((c = EVP_CIPHER_CTX_get_app_data(evp)) == NULL)
		fatal("ssh_aes_ctr_iv: no context");
	if (doset)
		memcpy(c->mt_ctx.rv_ctr, iv, len);
	else
		memcpy(iv, c->mt_ctx.rv_ctr, len);
}

const EVP_CIPHER *
evp_aes_128_ctr(void)
{
	static EVP_CIPHER aes_ctr;

	memset(&aes_ctr, 0, sizeof(EVP_CIPHER));
	aes_ctr.nid = NID_undef;
	aes_ctr.block_size = AES_BLOCK_SIZE;
	aes_ctr.iv_len = AES_BLOCK_SIZE;
	aes_ctr.key_len = 16;
	aes_ctr.init = ssh_aes_ctr_init;
	aes_ctr.cleanup = ssh_aes_ctr_cleanup;
	aes_ctr.do_cipher = ssh_cipher_ctr;
#ifndef SSH_OLD_EVP
	aes_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH |
	    EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&aes_ctr);
}

/******************************************************
	camellia-ctr
*******************************************************/

struct ssh_camellia_ctx
{
	struct mt_ctr_ctx	mt_ctx;
	CAMELLIA_KEY		camellia_ctx;
};

static void ssh_camellia_enc(void *ctx, u_char *buf)
{
	struct ssh_camellia_ctx *c = (struct ssh_camellia_ctx *)ctx;

	Camellia_encrypt(c->mt_ctx.th_ctr, buf, &c->camellia_ctx);
	ssh_ctr_inc(c->mt_ctx.th_ctr, CAMELLIA_BLOCK_SIZE);
}
static int ssh_camellia_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_camellia_ctx *c;

	if ((c = (struct ssh_camellia_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_camellia_ctx *)malloc(sizeof(*c));
		mt_init(&c->mt_ctx, (void *)c, ssh_camellia_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	mt_finis(&c->mt_ctx);

	if (key != NULL)
		Camellia_set_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8, &c->camellia_ctx);
	if (iv != NULL)
		memcpy(c->mt_ctx.rv_ctr, iv, CAMELLIA_BLOCK_SIZE);
	return (1);
}

static int ssh_camellia_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_camellia_ctx *c;

	if ((c = (struct ssh_camellia_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		mt_finis(&c->mt_ctx);
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

void ssh_camellia_iv(EVP_CIPHER_CTX *evp, int doset, u_char * iv, u_int len)
{
	struct ssh_camellia_ctx *c;

	if ((c = (struct ssh_camellia_ctx *)EVP_CIPHER_CTX_get_app_data(evp)) == NULL)
		return;	// fatal("ssh_camellia_ctr_iv: no context");
	if (doset)
		memcpy(c->mt_ctx.rv_ctr, iv, len);
	else
		memcpy(iv, c->mt_ctx.rv_ctr, len);
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
	camellia_ctr.cleanup = ssh_camellia_cleanup;
	camellia_ctr.do_cipher = ssh_cipher_ctr;
	camellia_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&camellia_ctr);
}

/******************************************************
	blowfish-ctr
*******************************************************/

struct ssh_blowfish_ctx
{
	struct mt_ctr_ctx	mt_ctx;
	BF_KEY			blowfish_ctx;
};

static void ssh_blowfish_enc(void *ctx, u_char *buf)
{
	int i, a;
	u_char *p;
	BF_LONG data[BF_BLOCK / 4 + 1];
	struct ssh_blowfish_ctx *c = (struct ssh_blowfish_ctx *)ctx;

	p = c->mt_ctx.th_ctr;
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

	ssh_ctr_inc(c->mt_ctx.th_ctr, BF_BLOCK);
}

static int ssh_blowfish_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_blowfish_ctx *c;

	if ((c = (struct ssh_blowfish_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_blowfish_ctx *)malloc(sizeof(*c));
		mt_init(&c->mt_ctx, (void *)c, ssh_blowfish_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	mt_finis(&c->mt_ctx);

	if (key != NULL)
		BF_set_key(&c->blowfish_ctx, EVP_CIPHER_CTX_key_length(ctx), key);
	if (iv != NULL)
		memcpy(c->mt_ctx.rv_ctr, iv, BF_BLOCK);
	return (1);
}

static int ssh_blowfish_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_blowfish_ctx *c;

	if ((c = (struct ssh_blowfish_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		mt_finis(&c->mt_ctx);
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

void ssh_blowfish_iv(EVP_CIPHER_CTX *evp, int doset, u_char * iv, u_int len)
{
	struct ssh_blowfish_ctx *c;

	if ((c = (struct ssh_blowfish_ctx *)EVP_CIPHER_CTX_get_app_data(evp)) == NULL)
		return;	// fatal("ssh_blowfish_ctr_iv: no context");
	if (doset)
		memcpy(c->mt_ctx.rv_ctr, iv, len);
	else
		memcpy(iv, c->mt_ctx.rv_ctr, len);
}

const EVP_CIPHER *evp_bf_ctr(void)
{
	static EVP_CIPHER blowfish_ctr;

	memset(&blowfish_ctr, 0, sizeof(EVP_CIPHER));
	blowfish_ctr.nid = NID_undef;
	blowfish_ctr.block_size = BF_BLOCK;
	blowfish_ctr.iv_len = BF_BLOCK;
	blowfish_ctr.key_len = 16;
	blowfish_ctr.init = ssh_blowfish_init;
	blowfish_ctr.cleanup = ssh_blowfish_cleanup;
	blowfish_ctr.do_cipher = ssh_cipher_ctr;
	blowfish_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&blowfish_ctr);
}

/******************************************************
	des3-ctr
*******************************************************/

struct ssh_des3_ctx
{
	struct mt_ctr_ctx	mt_ctx;
	DES_key_schedule	des3_ctx[3];
};

#define	DES_BLOCK	sizeof(DES_cblock)

static void ssh_des3_enc(void *ctx, u_char *buf)
{
	u_int i, a;
	u_char *p;
	DES_LONG data[DES_BLOCK / 4 + 1];
	struct ssh_des3_ctx *c = (struct ssh_des3_ctx *)ctx;

	p = c->mt_ctx.th_ctr;
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

	ssh_ctr_inc(c->mt_ctx.th_ctr, DES_BLOCK);
}

static int ssh_des3_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_des3_ctx *c;

	if ((c = (struct ssh_des3_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_des3_ctx *)malloc(sizeof(*c));
		mt_init(&c->mt_ctx, (void *)c, ssh_des3_enc);
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	mt_finis(&c->mt_ctx);

	if (key != NULL) {
		DES_set_key((const_DES_cblock *)(key +  0), &c->des3_ctx[0]);
		DES_set_key((const_DES_cblock *)(key +  8), &c->des3_ctx[1]);
		DES_set_key((const_DES_cblock *)(key + 16), &c->des3_ctx[2]);
	}
	if (iv != NULL)
		memcpy(c->mt_ctx.rv_ctr, iv, DES_BLOCK);
	return (1);
}

static int ssh_des3_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_des3_ctx *c;

	if ((c = (struct ssh_des3_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		mt_finis(&c->mt_ctx);
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

void ssh_des3_iv(EVP_CIPHER_CTX *evp, int doset, u_char * iv, u_int len)
{
	struct ssh_des3_ctx *c;

	if ((c = (struct ssh_des3_ctx *)EVP_CIPHER_CTX_get_app_data(evp)) == NULL)
		return;	// fatal("ssh_des3_ctr_iv: no context");
	if (doset)
		memcpy(c->mt_ctx.rv_ctr, iv, len);
	else
		memcpy(iv, c->mt_ctx.rv_ctr, len);
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
	des3_ctr.cleanup = ssh_des3_cleanup;
	des3_ctr.do_cipher = ssh_cipher_ctr;
	des3_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH | EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;

	return (&des3_ctr);
}

