// ssh.cpp: Cssh クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

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

#include <openssl/aes.h>

static const EVP_CIPHER *evp_aes_128_ctr(void);
static void ssh_aes_ctr_iv(EVP_CIPHER_CTX *, int, u_char *, u_int);

struct ssh_aes_ctr_ctx
{
	AES_KEY 	aes_ctx;
	u_char  	aes_counter[AES_BLOCK_SIZE];
};

/*
 * increment counter 'ctr',
 * the counter is of size 'len' bytes and stored in network-byte-order.
 * (LSB at ctr[len-1], MSB at ctr[0])
 */
static void ssh_ctr_inc(u_char *ctr, u_int len)
{
	int i;

	for (i = len - 1; i >= 0; i--)
		if (++ctr[i])   /* continue on overflow */
			return;
}

static int ssh_aes_ctr(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src, size_t len)
{
	struct ssh_aes_ctr_ctx *c;
	u_int n = 0;
	u_char buf[AES_BLOCK_SIZE];

	if (len == 0)
		return (1);
	if ((c = (struct ssh_aes_ctr_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	while ((len--) > 0) {
		if (n == 0) {
			AES_encrypt(c->aes_counter, buf, &c->aes_ctx);
			ssh_ctr_inc(c->aes_counter, AES_BLOCK_SIZE);
		}
		*(dest++) = *(src++) ^ buf[n];
		n = (n + 1) % AES_BLOCK_SIZE;
	}
	return (1);
}

static int ssh_aes_ctr_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh_aes_ctr_ctx *c;

	if ((c = (struct ssh_aes_ctr_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh_aes_ctr_ctx *)malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key != NULL)
		AES_set_encrypt_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8,
		    &c->aes_ctx);
	if (iv != NULL)
		memcpy(c->aes_counter, iv, AES_BLOCK_SIZE);
	return (1);
}

static int ssh_aes_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_aes_ctr_ctx *c;

	if ((c = (struct ssh_aes_ctr_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

static void ssh_aes_ctr_iv(EVP_CIPHER_CTX *evp, int doset, u_char * iv, size_t len)
{
	struct ssh_aes_ctr_ctx *c;

	if ((c = (struct ssh_aes_ctr_ctx *)EVP_CIPHER_CTX_get_app_data(evp)) == NULL)
		return; //fatal("ssh_aes_ctr_iv: no context");
	if (doset)
		memcpy(c->aes_counter, iv, len);
	else
		memcpy(iv, c->aes_counter, len);
}

static const EVP_CIPHER *evp_aes_128_ctr(void)
{
	static EVP_CIPHER aes_ctr;

	memset(&aes_ctr, 0, sizeof(EVP_CIPHER));
	aes_ctr.nid = NID_undef;
	aes_ctr.block_size = AES_BLOCK_SIZE;
	aes_ctr.iv_len = AES_BLOCK_SIZE;
	aes_ctr.key_len = 16;
	aes_ctr.init = ssh_aes_ctr_init;
	aes_ctr.cleanup = ssh_aes_ctr_cleanup;
	aes_ctr.do_cipher = ssh_aes_ctr;
#ifndef SSH_OLD_EVP
	aes_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH |
	    EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&aes_ctr);
}

//////////////////////////////////////////////////////////////////////

struct ssh1_3des_ctx
{
	EVP_CIPHER_CTX	k1, k2, k3;
};

static const EVP_CIPHER * evp_ssh1_3des(void);
static void ssh1_3des_iv(EVP_CIPHER_CTX *, int, u_char *, int);

static int ssh1_3des_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv, int enc)
{
	struct ssh1_3des_ctx *c;
	u_char *k1, *k2, *k3;

	if ((c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = (struct ssh1_3des_ctx *)malloc(sizeof(*c));
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}
	if (key == NULL)
		return (1);
	if (enc == -1)
		enc = ctx->encrypt;
	k1 = k2 = k3 = (u_char *) key;
	k2 += 8;
	if (EVP_CIPHER_CTX_key_length(ctx) >= 16+8) {
		if (enc)
			k3 += 16;
		else
			k1 += 16;
	}
	EVP_CIPHER_CTX_init(&c->k1);
	EVP_CIPHER_CTX_init(&c->k2);
	EVP_CIPHER_CTX_init(&c->k3);
	if (EVP_CipherInit(&c->k1, EVP_des_cbc(), k1, NULL, enc) == 0 ||
	    EVP_CipherInit(&c->k2, EVP_des_cbc(), k2, NULL, !enc) == 0 ||
	    EVP_CipherInit(&c->k3, EVP_des_cbc(), k3, NULL, enc) == 0) {
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

	if ((c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		return (0);
	}
	if (EVP_Cipher(&c->k1, dest, (u_char *)src, len) == 0 ||
	    EVP_Cipher(&c->k2, dest, dest, len) == 0 ||
	    EVP_Cipher(&c->k3, dest, dest, len) == 0)
		return (0);
	return (1);
}

static int ssh1_3des_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh1_3des_ctx *c;

	if ((c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
		EVP_CIPHER_CTX_cleanup(&c->k1);
		EVP_CIPHER_CTX_cleanup(&c->k2);
		EVP_CIPHER_CTX_cleanup(&c->k3);
		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

static void ssh1_3des_iv(EVP_CIPHER_CTX *evp, int doset, u_char *iv, int len)
{
	struct ssh1_3des_ctx *c;

	if (len != 24)
		return;
	if ((c = (struct ssh1_3des_ctx *)EVP_CIPHER_CTX_get_app_data(evp)) == NULL)
		return;
	if (doset) {
		memcpy(c->k1.iv, iv, 8);
		memcpy(c->k2.iv, iv + 8, 8);
		memcpy(c->k3.iv, iv + 16, 8);
	} else {
		memcpy(iv, c->k1.iv, 8);
		memcpy(iv + 8, c->k2.iv, 8);
		memcpy(iv + 16, c->k3.iv, 8);
	}
}

static const EVP_CIPHER *evp_ssh1_3des(void)
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

/*
 * SSH1 uses a variation on Blowfish, all bytes must be swapped before
 * and after encryption/decryption. Thus the swap_bytes stuff (yuk).
 */
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

	swap_bytes(in, out, len);
	ret = (*orig_bf)(ctx, out, out, len);
	swap_bytes(out, out, len);
	return (ret);
}

static const EVP_CIPHER *evp_ssh1_bf(void)
{
	static EVP_CIPHER ssh1_bf;

	memcpy(&ssh1_bf, EVP_bf_cbc(), sizeof(EVP_CIPHER));
	orig_bf = ssh1_bf.do_cipher;
	ssh1_bf.nid = NID_undef;
	ssh1_bf.do_cipher = bf_ssh1_cipher;
	ssh1_bf.key_len = 32;
	return (&ssh1_bf);
}

static void rsa_public_encrypt(BIGNUM *out, BIGNUM *in, RSA *key)
{
    BYTE *inbuf, *outbuf;
    int len, ilen, olen;

    olen = BN_num_bytes(key->n);
    outbuf = (BYTE *)malloc(olen);

    ilen = BN_num_bytes(in);
    inbuf = (BYTE *)malloc(ilen);
    BN_bn2bin(in, inbuf);

    len = RSA_public_encrypt(ilen, inbuf, outbuf, key, RSA_PKCS1_PADDING);

    BN_bin2bn(outbuf, len, out);

    free(outbuf);
    free(inbuf);
}

void *mm_zalloc(void *mm, unsigned int ncount, unsigned int size)
{
	unsigned int len = size * ncount;
	return malloc(len);
}

void mm_zfree(void *mm, void *address)
{
	free(address);
}

static int	ssh_crc32(LPBYTE buf, int len)
{
static const unsigned long crc32tab[] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
	0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
	0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
	0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
	0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
	0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
	0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
	0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
	0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
	0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
	0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
	0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
	0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
	0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
	0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
	0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
	0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
	0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
	0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
	0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
	0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
	0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
	0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
	0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
	0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
	0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
	0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
	0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
	0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
	0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
	0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
	0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
	0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
	0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
	0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
	0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
	0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
	0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
	0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
	0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
	0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
	0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
	0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
	0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
	0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
	0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
	0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
	0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
	0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
	0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
	0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
	0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};
	int i;
	unsigned long crc;

	crc = 0;
	for ( i = 0 ;  i < len ;  i++ )
		crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
	return (int)crc;
}

static int dh_pub_is_valid(DH *dh, BIGNUM *dh_pub)
{
	int i;
	int n = BN_num_bits(dh_pub);
	int bits_set = 0;

	if (dh_pub->neg)
		return 0;
	for (i = 0; i <= n; i++)
		if (BN_is_bit_set(dh_pub, i))
			bits_set++;
	/* if g==2 and bits_set==1 then computing log_g(dh_pub) is trivial */
	if (bits_set > 1 && (BN_cmp(dh_pub, dh->p) == -1))
		return 1;
	return 0;
}
static int dh_gen_key(DH *dh, int need)
{
	int i, bits_set, tries = 0;

	if (dh->p == NULL)
		return 1;
	if (need > INT_MAX / 2 || 2 * need >= BN_num_bits(dh->p))
		return 1;
	do {
		if (dh->priv_key != NULL)
			BN_clear_free(dh->priv_key);
		if ((dh->priv_key = BN_new()) == NULL)
			return 1;
		/* generate a 2*need bits random private exponent */
		if (!BN_rand(dh->priv_key, 2*need, 0, 0))
			return 1;
		if (DH_generate_key(dh) == 0)
			return 1;
		for (i = 0, bits_set = 0; i <= BN_num_bits(dh->priv_key); i++)
			if (BN_is_bit_set(dh->priv_key, i))
				bits_set++;
		if (tries++ > 10)
			return 1;
	} while (!dh_pub_is_valid(dh, dh->pub_key));
	return 0;
}
static DH *dh_new_group_asc(const char *gen, const char *modulus)
{
	DH *dh;

    if ((dh = DH_new()) == NULL)
		return NULL;
	if (BN_hex2bn(&dh->p, modulus) == 0)
		return NULL;
    if (BN_hex2bn(&dh->g, gen) == 0)
		return NULL;
    return (dh);
}
static DH *dh_new_group1(void)
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
static DH *dh_new_group14(void)
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
static u_char *kex_dh_hash(
    LPCSTR client_version_string, LPCSTR server_version_string,
    LPBYTE ckexinit, int ckexinitlen,
    LPBYTE skexinit, int skexinitlen,
    LPBYTE serverhostkeyblob, int sbloblen,
    BIGNUM *client_dh_pub,
    BIGNUM *server_dh_pub,
    BIGNUM *shared_secret)
{
	CBuffer b;
	static u_char digest[EVP_MAX_MD_SIZE];
	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX md;

	b.PutStr(client_version_string);
	b.PutStr(server_version_string);

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	b.Put32Bit(ckexinitlen+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)ckexinit, ckexinitlen);

	b.Put32Bit(skexinitlen+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)skexinit, skexinitlen);

	b.PutBuf((LPBYTE)serverhostkeyblob, sbloblen);

	b.PutBIGNUM2(client_dh_pub);
	b.PutBIGNUM2(server_dh_pub);
	b.PutBIGNUM2(shared_secret);

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, b.GetPtr(), b.GetSize());
	EVP_DigestFinal(&md, digest, NULL);

	return digest;
}
static int	dh_estimate(int bits)
{
	if ( bits <= 128 )
		return (1024);		/* O(2**86) */
	if (bits <= 192)
		return (2048);		/* O(2**116) */
	return (4096);			/* O(2**156) */
}
u_char *kex_gex_hash(
    LPCSTR client_version_string, LPCSTR server_version_string,
    LPBYTE ckexinit, int ckexinitlen,
    LPBYTE skexinit, int skexinitlen,
    LPBYTE serverhostkeyblob, int sbloblen,
	int min, int wantbits, int max, BIGNUM *prime, BIGNUM *gen,
    BIGNUM *client_dh_pub,
    BIGNUM *server_dh_pub,
    BIGNUM *shared_secret,
	int *hashlen,
	const EVP_MD *evp_md)
{
	CBuffer b;
	static u_char digest[EVP_MAX_MD_SIZE];
//	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX md;

	b.PutStr(client_version_string);
	b.PutStr(server_version_string);

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	b.Put32Bit(ckexinitlen+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)ckexinit, ckexinitlen);

	b.Put32Bit(skexinitlen+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)skexinit, skexinitlen);

	b.PutBuf((LPBYTE)serverhostkeyblob, sbloblen);
	if ( min == -1 || max == -1 )
		b.Put32Bit(wantbits);
	else {
		b.Put32Bit(min);
		b.Put32Bit(wantbits);
		b.Put32Bit(max);
	}

	b.PutBIGNUM2(prime);
	b.PutBIGNUM2(gen);
	b.PutBIGNUM2(client_dh_pub);
	b.PutBIGNUM2(server_dh_pub);
	b.PutBIGNUM2(shared_secret);

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, b.GetPtr(), b.GetSize());
	EVP_DigestFinal(&md, digest, NULL);

	*hashlen = EVP_MD_size(evp_md);

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
static u_char *derive_key(int id, int need, u_char *hash, int hashlen, BIGNUM *shared_secret, u_char *session_id, int sesslen, const EVP_MD *evp_md)
{
	CBuffer b;
//	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX md;
	char c = id;
	int have;
	int mdsz = EVP_MD_size(evp_md);
	u_char *digest = (u_char *)malloc(need + (mdsz - (need % mdsz)));

	b.PutBIGNUM2(shared_secret);

	/* K1 = HASH(K || H || "A" || session_id) */
	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, b.GetPtr(), b.GetSize());
	EVP_DigestUpdate(&md, hash, hashlen);
	EVP_DigestUpdate(&md, &c, 1);
	EVP_DigestUpdate(&md, session_id, sesslen);
	EVP_DigestFinal(&md, digest, NULL);

	/*
     * expand key:
     * Kn = HASH(K || H || K1 || K2 || ... || Kn-1)
     * Key = K1 || K2 || ... || Kn
     */
	for ( have = mdsz ; need > have ; have += mdsz ) {
		EVP_DigestInit(&md, evp_md);
		EVP_DigestUpdate(&md, b.GetPtr(), b.GetSize());
        EVP_DigestUpdate(&md, hash, hashlen);
		EVP_DigestUpdate(&md, digest, have);
		EVP_DigestFinal(&md, digest + have, NULL);
	}

//#ifdef	_DEBUG
//	TRACE("key %c\n", c);
//	dump_digest(digest, need);
//#endif

	return digest;
}

//////////////////////////////////////////////////////////////////////
// CCipher
//////////////////////////////////////////////////////////////////////

static const struct _CipherTab {
	int		num;
	char	*name;
	char	*sname;
	const EVP_CIPHER * (*func)(void);
	int		block_size;
	int		key_len;
	int		discard;
} CipherTab[] = {
	{ SSH2_CIPHER_3DES,			"3des-cbc",		"3des",		(const EVP_CIPHER *(*)())EVP_des_ede3_cbc,	8,	24,	0	},
	{ SSH2_CIPHER_BLOWFISH,		"blowfish-cbc",	"blf",		(const EVP_CIPHER *(*)())EVP_bf_cbc,		8,	16,	0	},
	{ SSH2_CIPHER_CAST128,		"cast128-cbc",	"cast",		(const EVP_CIPHER *(*)())EVP_cast5_cbc,		8,	16,	0	},
	{ SSH2_CIPHER_ARCFOUR,		"arcfour",		"arc4",		(const EVP_CIPHER *(*)())EVP_rc4,			8,	16,	0	},
	{ SSH2_CIPHER_ARC128,		"arcfour128",	"arc128",	(const EVP_CIPHER *(*)())EVP_rc4,			8,	16,	1536},
	{ SSH2_CIPHER_ARC256,		"arcfour256",	"arc256",	(const EVP_CIPHER *(*)())EVP_rc4,			8,	32,	1536},
	{ SSH2_CIPHER_AES128,		"aes128-cbc",	"aes128",	(const EVP_CIPHER *(*)())EVP_aes_128_cbc,	16,	16,	0	},
	{ SSH2_CIPHER_AES192,		"aes192-cbc",	"aes192",	(const EVP_CIPHER *(*)())EVP_aes_192_cbc,	16,	24,	0	},
	{ SSH2_CIPHER_AES256,		"aes256-cbc",	"aes256",	(const EVP_CIPHER *(*)())EVP_aes_256_cbc,	16,	32,	0	},
	{ SSH2_CIPHER_AES128R,		"aes128-ctr",	"a128ctr",	(const EVP_CIPHER *(*)())evp_aes_128_ctr,	16,	16,	0	},
	{ SSH2_CIPHER_AES192R,		"aes192-ctr",	"a192ctr",	(const EVP_CIPHER *(*)())evp_aes_128_ctr,	16,	24,	0	},
	{ SSH2_CIPHER_AES256R,		"aes256-ctr",	"a256ctr",	(const EVP_CIPHER *(*)())evp_aes_128_ctr,	16,	32,	0	},
	{ SSH_CIPHER_BLOWFISH,		"blowfish",		"blf",		(const EVP_CIPHER *(*)())evp_ssh1_bf,		8,	32,	0	},
	{ SSH_CIPHER_3DES,			"3des",			"3des",		(const EVP_CIPHER *(*)())evp_ssh1_3des,		8,	16,	0	},
	{ SSH_CIPHER_DES,			"des",			"des",		(const EVP_CIPHER *(*)())EVP_des_cbc,		8,	8,	0	},
	{ SSH_CIPHER_NONE,			"none",			"none",		(const EVP_CIPHER *(*)())EVP_enc_null,		8,	0,	0	},
	{ 0, NULL, NULL }
};

CCipher::CCipher()
{
	m_Index = (-1);
	m_KeyBuf = NULL;
	m_IvBuf = NULL;
}
CCipher::~CCipher()
{
	if ( m_Index != (-1) )
		EVP_CIPHER_CTX_cleanup(&m_Evp);
	if ( m_KeyBuf != NULL )
		delete m_KeyBuf;
	if ( m_IvBuf != NULL)
		delete m_IvBuf;
}
int CCipher::Init(LPCSTR name, int mode, LPBYTE key, int len, LPBYTE iv)
{
	int n, i;
	const EVP_CIPHER *type = NULL;

	if ( m_Index != (-1) )
		EVP_CIPHER_CTX_cleanup(&m_Evp);
	if ( m_KeyBuf != NULL )
		delete m_KeyBuf;
	if ( m_IvBuf != NULL)
		delete m_IvBuf;

	m_Index = (-1);
	m_KeyBuf = NULL;
	m_IvBuf = NULL;

	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		if ( strcmp(name, CipherTab[n].name) == 0 ) {
			type = (*(CipherTab[n].func))();
			break;
		}
	}
	if ( type == NULL )
		return TRUE;
	m_Index = n;

	if ( iv != NULL ) {
		m_IvBuf = new BYTE[CipherTab[n].key_len];
		memcpy(m_IvBuf, iv, CipherTab[n].key_len);
	}

	if ( len < 0 )
		len = CipherTab[n].key_len;
	if ( key != NULL ) {
		m_KeyBuf = new BYTE[len];
		memcpy(m_KeyBuf, key, len);
	}

	EVP_CIPHER_CTX_init(&m_Evp);

	if ( EVP_CipherInit(&m_Evp, type, NULL, m_IvBuf, (mode == MODE_ENC)) == 0 )
		return TRUE;

	if ( (i = EVP_CIPHER_CTX_key_length(&m_Evp)) > 0 && i != len ) {
		if ( EVP_CIPHER_CTX_set_key_length(&m_Evp, len) == 0 )
			return TRUE;
	}

	if ( EVP_CipherInit(&m_Evp, NULL, m_KeyBuf, NULL,  -1) == 0 )
		return TRUE;

	if ( (i = CipherTab[n].discard) > 0) {
		LPBYTE junk = new BYTE[i];
		LPBYTE discard = new BYTE[i];
		if ( EVP_Cipher(&m_Evp, discard, junk, i) == 0)
			return TRUE;
		delete junk;
		delete discard;
	}

	return FALSE;
}
void CCipher::Cipher(LPBYTE inbuf, int len, CBuffer *out)
{
	if ( m_Index == (-1) )
		out->Apend(inbuf, len);
	else
		EVP_Cipher(&m_Evp, out->PutSpc(len), inbuf, len);
}
int CCipher::GetIndex(LPCSTR name)
{
	int n;
	if ( name == NULL )
		return m_Index;
	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		if ( strcmp(name, CipherTab[n].name) == 0 )
			return n;
	}
	return (-1);
}
int CCipher::GetKeyLen(LPCSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return 8;
	else
		return CipherTab[n].key_len;
}
int CCipher::GetBlockSize(LPCSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return 8;
	else
		return CipherTab[n].block_size;
}
LPCSTR CCipher::GetName(int num)
{
	int n;
	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		if ( num == CipherTab[n].num )
			return (LPCSTR)(CipherTab[n].name);
	}
	return NULL;
}
int CCipher::GetNum(LPCSTR str)
{
	int n;
	if ( (n = GetIndex(str)) < 0 )
		return 0;
	else
		return CipherTab[n].num;
}
LPCSTR CCipher::GetMatchList(LPCSTR str)
{
	int n, i;
	CString tmp;
	CStringArray arry;

	arry.RemoveAll();
	while ( *str != '\0' ) {
		while ( *str != '\0' && strchr(", \t", *str) != NULL )
			str++;
		tmp = "";
		while ( *str != '\0' && strchr(", \t", *str) == NULL )
			tmp += *(str++);
		if ( !tmp.IsEmpty() )
			arry.Add(tmp);
	}
	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		for ( i = 0 ; i < arry.GetSize() ; i++ ) {
			if ( arry[i].Compare(CipherTab[n].name) == 0 )
				return CipherTab[n].name;
		}
	}
	return NULL;
}
LPCSTR CCipher::GetTitle()
{
	if ( m_Index == (-1) )
		return "";
	return CipherTab[m_Index].sname;
}
void CCipher::MakeKey(CBuffer *bp, LPCSTR pass)
{
	MD5_CTX md;
	u_char digest[16];

	MD5_Init(&md);
	MD5_Update(&md, pass, (unsigned long)strlen(pass));
	MD5_Final(digest, &md);

	bp->Clear();
	bp->Apend(digest, 16);
}

//////////////////////////////////////////////////////////////////////
// CMacomp
//////////////////////////////////////////////////////////////////////

static const struct _MacTab {
        char            *name;
		char			*sname;
        const EVP_MD *  (*func)(void);
        int             trunbits;
} MacsTab[] = {
    { "hmac-md5",			"md5",		(const EVP_MD *((*)()))EVP_md5,			 0 },
    { "hmac-sha1",			"sha1",		(const EVP_MD *((*)()))EVP_sha1,	  	 0 },
    { "hmac-ripemd160",     "ripe",		(const EVP_MD *((*)()))EVP_ripemd160,	 0 },
    { "hmac-md5-96",        "md5-96",	(const EVP_MD *((*)()))EVP_md5,			96 },
    { "hmac-sha1-96",       "sha1-96",	(const EVP_MD *((*)()))EVP_sha1,		96 },
	{ "none",				"none",		(const EVP_MD *((*)()))NULL,			 0 },
    { NULL, NULL, 0 }
};

CMacomp::CMacomp()
{
	m_Index = (-1);
	m_Md = NULL;
	m_KeyLen = 0;
	m_KeyBuf = NULL;
	m_BlockSize = 0;
}
CMacomp::~CMacomp()
{
	if ( m_KeyBuf != NULL )
		delete m_KeyBuf;
}
int CMacomp::Init(LPCSTR name, LPBYTE key, int len)
{
	int n;

	if ( m_KeyBuf != NULL )
		delete m_KeyBuf;

	m_KeyBuf = NULL;
	m_Index = (-1);
	m_Md = NULL;
	m_KeyLen = m_BlockSize = 0;

	for ( n = 0 ; MacsTab[n].name != NULL ; n++ ) {
		if ( strcmp(MacsTab[n].name, name) == 0 ) {
			if ( MacsTab[n].func != NULL ) {
				m_Md = (*(MacsTab[n].func))();
				m_KeyLen = m_BlockSize = EVP_MD_size(m_Md);
				if ( MacsTab[n].trunbits != 0 )
					m_BlockSize = MacsTab[n].trunbits / 8;
			}
			break;
		}
	}

	if ( MacsTab[n].name == NULL )
		return TRUE;

	m_Index = n;

	if ( len < 0 )
		len = m_KeyLen;
	else
		m_KeyLen = len;

	if ( key != NULL ) {
		m_KeyBuf = new BYTE[len];
		memcpy(m_KeyBuf, key, len);
	}

	return FALSE;
}
void CMacomp::Compute(int seq, LPBYTE inbuf, int len, CBuffer *out)
{
	HMAC_CTX c;
	u_char tmp[EVP_MAX_MD_SIZE];
	u_char b[4];

	if ( m_KeyBuf == NULL || m_Md == NULL || m_BlockSize > EVP_MAX_MD_SIZE )
		return;

	HMAC_Init(&c, m_KeyBuf, m_KeyLen, m_Md);
 	b[0] = (BYTE)(seq >> 24);
	b[1] = (BYTE)(seq >> 16);
	b[2] = (BYTE)(seq >> 8);
	b[3] = (BYTE)seq;
	HMAC_Update(&c, b, sizeof(b));
	HMAC_Update(&c, inbuf, len);
	HMAC_Final(&c, tmp, NULL);
	HMAC_cleanup(&c);

	out->Apend(tmp, m_BlockSize);
}
int CMacomp::GetIndex(LPCSTR name)
{
	int n;
	if ( name == NULL )
		return m_Index;
	for ( n = 0 ; MacsTab[n].name != NULL ; n++ ) {
		if ( strcmp(MacsTab[n].name, name) == 0 )
			return n;
	}
	return (-1);
}
int CMacomp::GetKeyLen(LPCSTR name)
{
	int n;
	const EVP_MD *md;

	if ( (n = GetIndex(name)) < 0 )
		return 0;
	else if ( n == m_Index )
		return m_KeyLen;

	if ( MacsTab[n].func == NULL )
		return 0;

	if ( (md = (*(MacsTab[n].func))()) == NULL )
		return 0;

	return EVP_MD_size(md);
}
int CMacomp::GetBlockSize(LPCSTR name)
{
	int n;
	const EVP_MD *md;

	if ( (n = GetIndex(name)) < 0 )
		return 0;
	else if ( n == m_Index )
		return m_BlockSize;

	if ( MacsTab[n].func == NULL )
		return 0;

	if ( MacsTab[n].trunbits != 0 )
		return (MacsTab[n].trunbits / 8);

	if ( (md = (*(MacsTab[n].func))()) == NULL )
		return 0;

	return EVP_MD_size(md);
}
LPCSTR CMacomp::GetMatchList(LPCSTR str)
{
	int n, i;
	CString tmp;
	CStringArray arry;

	arry.RemoveAll();
	while ( *str != '\0' ) {
		while ( *str != '\0' && strchr(", \t", *str) != NULL )
			str++;
		tmp = "";
		while ( *str != '\0' && strchr(", \t", *str) == NULL )
			tmp += *(str++);
		if ( !tmp.IsEmpty() )
			arry.Add(tmp);
	}
	for ( n = 0 ; MacsTab[n].name != NULL ; n++ ) {
		for ( i = 0 ; i < arry.GetSize() ; i++ ) {
			if ( arry[i].Compare(MacsTab[n].name) == 0 )
				return MacsTab[n].name;
		}
	}
	return NULL;
}
LPCSTR CMacomp::GetTitle()
{
	if ( m_Index < 0 )
		return "";
	return MacsTab[m_Index].sname;
}

//////////////////////////////////////////////////////////////////////
// CCompress
//////////////////////////////////////////////////////////////////////

static const struct _CompTab {
        char            *name;
		char			*sname;
} CompTab[] = { 
	{ "zlib",				"cmp"	},
	{ "none",				"non"	},
	{ "zlib@openssh.com",	"dly"	},
	{ NULL,					NULL	}
};

CCompress::CCompress()
{
	m_Mode = 0;
}
CCompress::~CCompress()
{
	if ( (m_Mode & 1) != 0 )
		inflateEnd(&m_InCompStr);

	if ( (m_Mode & 2) != 0 )
		deflateEnd(&m_OutCompStr);
}
int CCompress::Init(LPCSTR name, int mode, int level)
{
	if ( (m_Mode & 1) != 0 )
		inflateEnd(&m_InCompStr);

	if ( (m_Mode & 2) != 0 )
		deflateEnd(&m_OutCompStr);

	m_Mode = 0;

	if ( name != NULL ) {
		if ( strcmp(name, "zlib@openssh.com") == 0 ) {
		    m_Mode = 4;
			return FALSE;
		} else if ( strcmp(name, "zlib") != 0 )
			return FALSE;
	}

	if ( mode == MODE_ENC ) {	// Compress
		m_OutCompStr.zalloc = (alloc_func)mm_zalloc;
		m_OutCompStr.zfree  = (free_func)mm_zfree;
		m_OutCompStr.opaque = NULL;
		deflateInit(&m_OutCompStr, level);
		m_Mode |= 2;
	} else {			// DeCompress
		m_InCompStr.zalloc = (alloc_func)mm_zalloc;
		m_InCompStr.zfree  = (free_func)mm_zfree;
		m_InCompStr.opaque = NULL;
		inflateInit(&m_InCompStr);
		m_Mode |= 1;
	}

	return FALSE;
}
void CCompress::Compress(LPBYTE inbuf, int len, CBuffer *out)
{
	BYTE buf[4096];

	if ( len == 0 )
		return;

	if ( (m_Mode & 2) == 0 ) {
		out->Apend(inbuf, len);
		return;
	}

	m_OutCompStr.next_in  = inbuf;
	m_OutCompStr.avail_in = len;

	do {
		m_OutCompStr.next_out = buf;
		m_OutCompStr.avail_out = sizeof(buf);
		if ( deflate(&m_OutCompStr, Z_PARTIAL_FLUSH) != Z_OK )
			AfxThrowMemoryException();
		out->Apend(buf, sizeof(buf) - m_OutCompStr.avail_out);
	} while ( m_OutCompStr.avail_out == 0 );
}
void CCompress::UnCompress(LPBYTE inbuf, int len, CBuffer *out)
{
	BYTE buf[4096];
	int status;

	if ( len == 0 )
		return;

	if ( (m_Mode & 1) == 0 ) {
		out->Apend(inbuf, len);
		return;
	}

	m_InCompStr.next_in  = inbuf;
	m_InCompStr.avail_in = len;

	for (;;) {
		m_InCompStr.next_out = buf;
		m_InCompStr.avail_out = sizeof(buf);
		status = inflate(&m_InCompStr, Z_PARTIAL_FLUSH);
		if ( status == Z_BUF_ERROR )
			break;
		else if ( status != Z_OK )
			AfxThrowMemoryException();
		out->Apend(buf, sizeof(buf) - m_InCompStr.avail_out);
	}
}
LPCSTR CCompress::GetMatchList(LPCSTR str)
{
	int n, i;
	CString tmp;
	CStringArray arry;

	arry.RemoveAll();
	while ( *str != '\0' ) {
		while ( *str != '\0' && strchr(", \t", *str) != NULL )
			str++;
		tmp = "";
		while ( *str != '\0' && strchr(", \t", *str) == NULL )
			tmp += *(str++);
		if ( !tmp.IsEmpty() )
			arry.Add(tmp);
	}
	for ( n = 0 ; CompTab[n].name != NULL ; n++ ) {
		for ( i = 0 ; i < arry.GetSize() ; i++ ) {
			if ( arry[i].Compare(CompTab[n].name) == 0 )
				return CompTab[n].name;
		}
	}
	return NULL;
}
LPCSTR CCompress::GetTitle()
{
	return CompTab[m_Mode == 0 ? 1 : ((m_Mode & 4) ? 2 : 0)].sname;
}

//////////////////////////////////////////////////////////////////////
// CIdKey
//////////////////////////////////////////////////////////////////////

CIdKey::CIdKey()
{
	m_Uid  = 0;
	m_Name = "";
	m_Pass = "";
	m_Type = IDKEY_NONE;
	m_Rsa  = NULL;
	m_Dsa  = NULL;
	m_Flag = FALSE;
}
CIdKey::~CIdKey()
{
	if ( m_Rsa != NULL )
		RSA_free(m_Rsa);
	if ( m_Dsa != NULL )
		DSA_free(m_Dsa);
}
void CIdKey::SetString(CString &str)
{
	CString tmp;
	CStringArrayExt array;

	array.AddVal(m_Uid);
	array.Add(m_Name);
	EncryptStr(tmp, m_Pass);
	array.Add(tmp);
	WritePublicKey(tmp);
	array.Add(tmp);
	WritePrivateKey(tmp, m_Pass);
	array.Add(tmp);
	array.SetString(str);
}
int CIdKey::GetString(LPCSTR str)
{
	CStringArrayExt array;

	array.GetString(str);
	if ( array.GetSize() < 5 )
		return FALSE;
	m_Uid  = array.GetVal(0);
	m_Name = array.GetAt(1);
	DecryptStr(m_Pass, array.GetAt(2));
	ReadPublicKey(array.GetAt(3));
	ReadPrivateKey(array.GetAt(4), m_Pass);
	return TRUE;
}
int CIdKey::GetProfile(LPCSTR pSection, int Uid)
{
	CString ent, str;
	ent.Format("List%02d", Uid);
	str = AfxGetApp()->GetProfileString(pSection, ent, "");
	return GetString(str);
}
void CIdKey::SetProfile(LPCSTR pSection)
{
	CString ent, str;
	ent.Format("List%02d", m_Uid);
	SetString(str);
	AfxGetApp()->WriteProfileString(pSection, ent, str);
}
void CIdKey::DelProfile(LPCSTR pSection)
{
	CString ent;
	ent.Format("List%02d", m_Uid);
	((CRLoginApp *)AfxGetApp())->DelProfileEntry(pSection, ent);
}
const CIdKey & CIdKey::operator = (CIdKey &data)
{
	m_Uid  = data.m_Uid;
	m_Name = data.m_Name;
	m_Pass = data.m_Pass;
	m_Flag = data.m_Flag;
	Close();

	switch(data.m_Type) {
	case IDKEY_NONE:
		m_Type = IDKEY_NONE;
		break;
	case IDKEY_RSA1:
	case IDKEY_RSA2:
	case IDKEY_RSA_CERT:
		Create(data.m_Type);
		BN_copy(m_Rsa->n, data.m_Rsa->n);
		BN_copy(m_Rsa->e, data.m_Rsa->e);
		BN_copy(m_Rsa->d, data.m_Rsa->d);
		BN_copy(m_Rsa->p, data.m_Rsa->p);
		BN_copy(m_Rsa->q, data.m_Rsa->q);
		BN_copy(m_Rsa->iqmp, data.m_Rsa->iqmp);
		break;
	case IDKEY_DSA2:
	case IDKEY_DSA_CERT:
		Create(data.m_Type);
		BN_copy(m_Dsa->p, data.m_Dsa->p);
		BN_copy(m_Dsa->q, data.m_Dsa->q);
		BN_copy(m_Dsa->g, data.m_Dsa->g);
		BN_copy(m_Dsa->pub_key, data.m_Dsa->pub_key);
		BN_copy(m_Dsa->priv_key, data.m_Dsa->priv_key);
		break;
	}

	return *this;
}
int CIdKey::Create(int type)
{
	m_Type = IDKEY_NONE;
	switch(type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
	case IDKEY_RSA_CERT:
		m_Type = type;
		if ( m_Rsa != NULL )
			break;
		if ( (m_Rsa = RSA_new()) == NULL )
			return FALSE;
		if ( (m_Rsa->n = BN_new()) == NULL || (m_Rsa->e = BN_new()) == NULL )
			return Close();
		if ( (m_Rsa->d = BN_new()) == NULL || (m_Rsa->iqmp = BN_new()) == NULL )
			return Close();
		if ( (m_Rsa->p = BN_new()) == NULL || (m_Rsa->q = BN_new()) == NULL )
			return Close();
		break;
	case IDKEY_DSA2:
	case IDKEY_DSA_CERT:
		m_Type = type;
		if ( m_Dsa != NULL )
			break;
		if ( (m_Dsa = DSA_new()) == NULL )
			break;
		if ( (m_Dsa->p = BN_new()) == NULL || (m_Dsa->q = BN_new()) == NULL || (m_Dsa->g = BN_new()) == NULL )
			return Close();
		if ( (m_Dsa->pub_key = BN_new()) == NULL || (m_Dsa->priv_key = BN_new()) == NULL )
			return Close();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
int CIdKey::Generate(int type, int bits)
{
	m_Type = IDKEY_NONE;
	switch(type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		if ( m_Rsa != NULL )
			RSA_free(m_Rsa);
		if ( (m_Rsa = RSA_generate_key(bits, 35, NULL, NULL)) == NULL )
			return FALSE;
		m_Type = type;
		break;
	case IDKEY_DSA2:
		if ( m_Dsa != NULL )
			DSA_free(m_Dsa);
		if ( (m_Dsa = DSA_generate_parameters(bits, NULL, 0, NULL, NULL, NULL, NULL)) == NULL )
			return FALSE;
		if ( !DSA_generate_key(m_Dsa) )
			return Close();
		m_Type = IDKEY_DSA2;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
int CIdKey::Close()
{
	if ( m_Rsa != NULL )
		RSA_free(m_Rsa);
	if ( m_Dsa != NULL )
		DSA_free(m_Dsa);
	m_Rsa = NULL;
	m_Dsa = NULL;
	m_Type = IDKEY_NONE;
	return FALSE;
}
int CIdKey::Compere(CIdKey *pKey)
{
	switch(m_Type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
	case IDKEY_RSA_CERT:
		if ( m_Rsa == NULL || pKey->m_Rsa == NULL )
			break;
		if ( BN_cmp(m_Rsa->e, pKey->m_Rsa->e) != 0 )
			break;
		if ( BN_cmp(m_Rsa->n, pKey->m_Rsa->n) != 0 )
			break;
		return 0;
	case IDKEY_DSA2:
	case IDKEY_DSA_CERT:
		if ( m_Dsa == NULL || pKey->m_Dsa == NULL )
			break;
		if ( BN_cmp(m_Dsa->p, pKey->m_Dsa->p) != 0 )
			break;
		if ( BN_cmp(m_Dsa->q, pKey->m_Dsa->q) != 0 )
			break;
		if ( BN_cmp(m_Dsa->g, pKey->m_Dsa->g) != 0 )
			break;
		if ( BN_cmp(m_Dsa->pub_key, pKey->m_Dsa->pub_key) != 0 )
			break;
		return 0;
	}
	return (-1);
}
LPCSTR CIdKey::GetName()
{
	switch(m_Type) {
	case IDKEY_RSA1: return "rsa1";
	case IDKEY_RSA2: return "ssh-rsa";
	case IDKEY_DSA2: return "ssh-dss";
	case IDKEY_RSA_CERT: return "ssh-rsa-cert-v00@openssh.com";
	case IDKEY_DSA_CERT: return "ssh-dss-cert-v00@openssh.com";
	}
	return "";
}
int CIdKey::GetTypeFromName(LPCSTR name)
{
	if ( strcmp(name, "rsa1") == 0 )
		return IDKEY_RSA1;
	else if ( strcmp(name, "rsa") == 0 || strcmp(name, "ssh-rsa") == 0 )
		return IDKEY_RSA2;
	else if ( strcmp(name, "dsa") == 0 || strcmp(name, "ssh-dss") == 0 )
		return IDKEY_DSA2;
	//else if ( strcmp(name, "ssh-rsa-cert-v00@openssh.com") == 0 )
	//	return IDKEY_RSA_CERT;
	//else if ( strcmp(name, "ssh-dss-cert-v00@openssh.com") == 0 )
	//	return IDKEY_DSA_CERT;
	else
		return IDKEY_NONE;
}
int CIdKey::HostVerify(LPCSTR host)
{
	CString str, ses, known;

	ses.Format("%s-%s", host, GetName());
	known = AfxGetApp()->GetProfileString("KnownHosts", ses, "");
	WritePublicKey(str);

	if ( !known.IsEmpty() && known.Compare(str) != 0 ) {
		if ( MessageBox(NULL, "Host key verification failed", "Warning",
					MB_ICONWARNING | MB_RETRYCANCEL) == IDCANCEL )
			return FALSE;
		known = "";
	}

	if ( known.IsEmpty() )
		AfxGetApp()->WriteProfileString("KnownHosts", ses, str);

	return TRUE;
}
int CIdKey::RsaSign(CBuffer *bp, LPBYTE buf, int len)
{
	int n;
	const EVP_MD *evp_md =  EVP_sha1();
	EVP_MD_CTX md;
	unsigned int dlen, slen;
    BYTE digest[EVP_MAX_MD_SIZE];
	CSpace sig;

	if ( m_Rsa == NULL )
		return FALSE;

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, buf, len);
	EVP_DigestFinal(&md, digest, &dlen);

	sig.SetSize(RSA_size(m_Rsa));
	if ( RSA_sign(NID_sha1, digest, dlen, sig.GetPtr(), &slen, m_Rsa) != 1 )
		return FALSE;

	if ( (n = sig.GetSize() - slen) > 0 ) {
		memcpy(sig.GetPtr() + n, sig.GetPtr(), slen);
		memset(sig.GetPtr(), 0, n);
	} else if ( n < 0 )
		return FALSE;

	bp->Clear();
	bp->PutStr("ssh-rsa");
	bp->PutBuf(sig.GetPtr(), sig.GetSize());

	return TRUE;
}
int CIdKey::DssSign(CBuffer *bp, LPBYTE buf, int len)
{
	const EVP_MD *evp_md =  EVP_sha1();
	EVP_MD_CTX md;
	unsigned int dlen, slen, rlen;
    BYTE digest[EVP_MAX_MD_SIZE];
	BYTE sigblob[SIGBLOB_LEN];
	DSA_SIG *sig;

	if ( m_Dsa == NULL )
		return FALSE;

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, buf, len);
	EVP_DigestFinal(&md, digest, &dlen);

	if ( (sig = DSA_do_sign(digest, dlen, m_Dsa)) == NULL )
		return FALSE;

	rlen = BN_num_bytes(sig->r);
	slen = BN_num_bytes(sig->s);
	if (rlen > INTBLOB_LEN || slen > INTBLOB_LEN) {
		DSA_SIG_free(sig);
		return FALSE;
	}
	memset(sigblob, 0, SIGBLOB_LEN);
	BN_bn2bin(sig->r, sigblob + SIGBLOB_LEN - INTBLOB_LEN - rlen);
	BN_bn2bin(sig->s, sigblob + SIGBLOB_LEN - slen);
	DSA_SIG_free(sig);

	bp->Clear();
	bp->PutStr("ssh-dss");
	bp->PutBuf(sigblob, SIGBLOB_LEN);

	return TRUE;
}
int CIdKey::Sign(CBuffer *bp, LPBYTE buf, int len)
{
	switch(m_Type) {
	case IDKEY_RSA2: return RsaSign(bp, buf, len);
	case IDKEY_DSA2: return DssSign(bp, buf, len);
	}
	return FALSE;
}
int CIdKey::RsaVerify(CBuffer *bp, LPBYTE data, int datalen)
{
	int len, n;
	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX md;
	u_char digest[EVP_MAX_MD_SIZE];
	u_int dlen;
	CString keytype;
	CBuffer sigblob;
	CSpace spc;
	static const u_char id_sha1[] = {
        0x30, 0x21, /* type Sequence, length 0x21 (33) */
        0x30, 0x09, /* type Sequence, length 0x09 */
        0x06, 0x05, /* type OID, length 0x05 */
        0x2b, 0x0e, 0x03, 0x02, 0x1a, /* id-sha1 OID */
        0x05, 0x00, /* NULL */
        0x04, 0x14  /* Octet string, length 0x14 (20), followed by sha1 hash */
	};

	bp->GetStr(keytype);
	if ( keytype.Compare("ssh-rsa") != 0 )
		return FALSE;

	bp->GetBuf(&sigblob);
	if ( (n = RSA_size(m_Rsa) - sigblob.GetSize()) > 0 ) {
		sigblob.PutSpc(n);
		memcpy(sigblob.GetPtr() + n, sigblob.GetPtr(), sigblob.GetSize() - n);
		memset(sigblob.GetPtr(), 0, n);
	} else if ( n < 0 )
		return FALSE;

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, data, datalen);
	EVP_DigestFinal(&md, digest, &dlen);

	spc.SetSize(RSA_size(m_Rsa));
	if ( (len = RSA_public_decrypt(sigblob.GetSize(), sigblob.GetPtr(), spc.GetPtr(), m_Rsa, RSA_PKCS1_PADDING)) < 0 )
		return FALSE;

	if ( len != ((int)dlen + 15) )
		return FALSE;

	if ( memcmp(spc.GetPtr(), id_sha1, 15) != 0 )
		return FALSE;

	if ( memcmp(spc.GetPtr() + 15, digest, dlen) != 0 )
		return FALSE;

	return TRUE;
}
int CIdKey::DssVerify(CBuffer *bp, LPBYTE data, int datalen)
{
	int ret;
	DSA_SIG *dsig;
	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX md;
	u_char digest[EVP_MAX_MD_SIZE];
	u_int dlen;
	CString keytype;
	CBuffer sigblob;

	bp->GetStr(keytype);
	if ( keytype.Compare("ssh-dss") != 0 )
		return FALSE;

	bp->GetBuf(&sigblob);
	if ( sigblob.GetSize() != SIGBLOB_LEN )
		return FALSE;

	if ( (dsig = DSA_SIG_new()) == NULL ||
		 (dsig->r = BN_new()) == NULL || (dsig->s = BN_new()) == NULL )
		return FALSE;

	BN_bin2bn(sigblob.GetPtr(), INTBLOB_LEN, dsig->r);
	BN_bin2bn(sigblob.GetPtr() + INTBLOB_LEN, INTBLOB_LEN, dsig->s);

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, data, datalen);
	EVP_DigestFinal(&md, digest, &dlen);

	ret = DSA_do_verify(digest, dlen, dsig, m_Dsa);

	DSA_SIG_free(dsig);

	return (ret == 1 ? TRUE : FALSE);
}
int CIdKey::Verify(CBuffer *bp, LPBYTE data, int datalen)
{
	switch(m_Type) {
	case IDKEY_RSA2: return RsaVerify(bp, data, datalen);
	case IDKEY_DSA2: return DssVerify(bp, data, datalen);
	}
	return FALSE;
}
int CIdKey::GetBlob(CBuffer *bp)
{
	CString str;

	m_Type = IDKEY_NONE;
	bp->GetStr(str);

	if ( str.Compare("rsa") == 0 || str.Compare("ssh-rsa") == 0 ) {
		if ( !Create(IDKEY_RSA2) )
			return FALSE;
		bp->GetBIGNUM2(m_Rsa->e);
		bp->GetBIGNUM2(m_Rsa->n);

	} else if ( str.Compare("dsa") == 0 || str.Compare("ssh-dss") == 0 ) {
		if ( !Create(IDKEY_DSA2) )
			return FALSE;
		bp->GetBIGNUM2(m_Dsa->p);
		bp->GetBIGNUM2(m_Dsa->q);
		bp->GetBIGNUM2(m_Dsa->g);
		bp->GetBIGNUM2(m_Dsa->pub_key);

	} else
		return FALSE;

	return TRUE;
}
int CIdKey::SetBlob(CBuffer *bp)
{
	switch(m_Type) {
	case IDKEY_RSA2:
		bp->PutStr("ssh-rsa");
		bp->PutBIGNUM2(m_Rsa->e);
		bp->PutBIGNUM2(m_Rsa->n);
		break;
	case IDKEY_DSA2:
		bp->PutStr("ssh-dss");
		bp->PutBIGNUM2(m_Dsa->p);
		bp->PutBIGNUM2(m_Dsa->q);
		bp->PutBIGNUM2(m_Dsa->g);
		bp->PutBIGNUM2(m_Dsa->pub_key);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
void CIdKey::GetUserHostName(CString &str)
{
	DWORD size;
	TCHAR buf[MAX_COMPUTERNAME_LENGTH + 2];

	str = "";
	size = MAX_COMPUTERNAME_LENGTH;
	if ( GetUserName(buf, &size) )
		str += buf;
	str += "@";
	size = MAX_COMPUTERNAME_LENGTH;
	if ( GetComputerName(buf, &size) )
		str += buf;
}
void CIdKey::MakeKey(CBuffer *bp, LPCSTR pass)
{
	CString tmp;
	unsigned int dlen;
	u_char digest[EVP_MAX_MD_SIZE];
	const EVP_MD *evp_md = EVP_sha1();
	EVP_MD_CTX md;

	GetUserHostName(tmp);

	if ( pass != NULL )
		tmp += pass;

	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, (LPBYTE)(LPCSTR)tmp, tmp.GetLength());
	EVP_DigestFinal(&md, digest, &dlen);

	bp->Clear();
	bp->Apend(digest, dlen);
}
void CIdKey::Decrypt(CBuffer *bp, LPCSTR str, LPCSTR pass)
{
	CBuffer key;
	CBuffer tmp;
	CCipher cip;

	tmp.Base64Decode(str);
	MakeKey(&key, pass);
	cip.Init("3des", MODE_DEC, key.GetPtr(), key.GetSize());
	cip.Cipher(tmp.GetPtr(), tmp.GetSize(), bp);
}
void CIdKey::Encrypt(CString &str, LPBYTE buf, int len, LPCSTR pass)
{
	CBuffer key;
	CBuffer tmp, out, sub;
	CCipher cip;

	MakeKey(&key, pass);
	cip.Init("3des", MODE_ENC, key.GetPtr(), key.GetSize());
	sub.Apend(buf, len);
	sub.RoundUp(8);
	cip.Cipher(sub.GetPtr(), sub.GetSize(), &tmp);
	out.Base64Encode(tmp.GetPtr(), tmp.GetSize());
	str = out.GetPtr();
}
void CIdKey::DecryptStr(CString &out, LPCSTR str)
{
	CBuffer tmp;
	Decrypt(&tmp, str);
	tmp.Put8Bit(0);
	out = tmp.GetPtr(); 
}
void CIdKey::EncryptStr(CString &out, LPCSTR str)
{
	Encrypt(out, (LPBYTE)str, (int)strlen(str) + 1);
}
int CIdKey::ReadPublicKey(LPCSTR str)
{
  TRY {
	int n;
	CString tmp;
	CBuffer buf;

	while ( *str == ' ' || *str == '\t' )
		str++;

	if ( isdigit(*str) ) {	// Try SSH1 RSA
		if ( !Create(IDKEY_RSA1) )
			return FALSE;

		for ( tmp = "" ; isdigit(*str) ; )
			tmp += *(str++);
		if ( (n = atoi(tmp)) == 0 )
			return Close();

		while ( *str == ' ' || *str == '\t' )
			str++;
		for ( tmp = "" ; isdigit(*str) ; )
			tmp += *(str++);
		if ( BN_dec2bn(&(m_Rsa->e), (LPSTR)(LPCSTR)tmp) == 0 )
			return Close();

		while ( *str == ' ' || *str == '\t' )
			str++;
		for ( tmp = "" ; isdigit(*str) ; )
			tmp += *(str++);
		if ( BN_dec2bn(&(m_Rsa->n), (LPSTR)(LPCSTR)tmp) == 0 )
			return Close();

	} else if ( strncmp(str, "ssh-rsa", 7) == 0 || strncmp(str, "rsa", 3) == 0 ||
			    strncmp(str, "ssh-dss", 7) == 0 || strncmp(str, "dsa", 3) == 0 ) {
		if ( (str = strchr(str, ' ')) == NULL )
			return FALSE;
		buf.Base64Decode(str + 1);

		if ( !GetBlob(&buf) )
			return FALSE;

	} else
		return FALSE;

	return TRUE;

  } CATCH(CUserException, e) {
	return FALSE;
  } END_CATCH
}
int CIdKey::WritePublicKey(CString &str)
{
	char *p;
	CBuffer tmp, buf;
	CString user;

	GetUserHostName(user);

	switch(m_Type) {
	case IDKEY_RSA1:
		str.Format("%u", BN_num_bits(m_Rsa->n));
		if ( (p = BN_bn2dec(m_Rsa->e)) == NULL )
			return FALSE;
		str += " ";
		str += p;
		free(p);
		if ( (p = BN_bn2dec(m_Rsa->n)) == NULL )
			return FALSE;
		str += " ";
		str += p;
		free(p);
		break;
	case IDKEY_RSA2:
	case IDKEY_DSA2:
		if ( !SetBlob(&tmp) )
			return FALSE;
		buf.Base64Encode(tmp.GetPtr(), tmp.GetSize());
		str = GetName();
		str += " ";
		str += (LPCSTR)(buf.GetPtr());
		str += " ";
		str += user;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
int CIdKey::ReadPrivateKey(LPCSTR str, LPCSTR pass)
{
  TRY {
	CBuffer tmp;

	Decrypt(&tmp, str, pass);

	if ( tmp.GetSize() < 1 || !Create(tmp.Get8Bit()) )
		return FALSE;

	switch(m_Type) {
	case IDKEY_RSA1:
		tmp.GetBIGNUM(m_Rsa->d);
		tmp.GetBIGNUM(m_Rsa->iqmp);
		tmp.GetBIGNUM(m_Rsa->q);
		tmp.GetBIGNUM(m_Rsa->p);
		break;
	case IDKEY_RSA2:
		tmp.GetBIGNUM(m_Rsa->d);
		tmp.GetBIGNUM(m_Rsa->iqmp);
		tmp.GetBIGNUM(m_Rsa->q);
		tmp.GetBIGNUM(m_Rsa->p);
		break;
	case IDKEY_DSA2:
		tmp.GetBIGNUM(m_Dsa->priv_key);
		break;
	default:
		return FALSE;
	}
	return TRUE;
  } CATCH(CUserException, e) {
	return FALSE;
  } END_CATCH
}
int CIdKey::WritePrivateKey(CString &str, LPCSTR pass)
{
	CBuffer tmp;

	switch(m_Type) {
	case IDKEY_RSA1:
		tmp.Put8Bit(m_Type);
		tmp.PutBIGNUM(m_Rsa->d);
		tmp.PutBIGNUM(m_Rsa->iqmp);
		tmp.PutBIGNUM(m_Rsa->q);
		tmp.PutBIGNUM(m_Rsa->p);
		goto ENCODE;
	case IDKEY_RSA2:
		tmp.Put8Bit(m_Type);
		tmp.PutBIGNUM(m_Rsa->d);
		tmp.PutBIGNUM(m_Rsa->iqmp);
		tmp.PutBIGNUM(m_Rsa->q);
		tmp.PutBIGNUM(m_Rsa->p);
		goto ENCODE;
	case IDKEY_DSA2:
		tmp.Put8Bit(m_Type);
		tmp.PutBIGNUM(m_Dsa->priv_key);
	ENCODE:
		Encrypt(str, tmp.GetPtr(), tmp.GetSize(), pass);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static const char authfile_id_string[] = "SSH PRIVATE KEY FILE FORMAT 1.1\n";

int CIdKey::SaveRsa1Key(FILE *fp, LPCSTR pass)
{
	CBuffer tmp;

	tmp.Apend((LPBYTE)authfile_id_string, sizeof(authfile_id_string));
	tmp.Put8Bit(SSH_CIPHER_3DES);
	tmp.Put32Bit(0);
	tmp.Put32Bit(0);

	tmp.PutBIGNUM(m_Rsa->n);
	tmp.PutBIGNUM(m_Rsa->e);

	CString com;
	GetUserHostName(com);
	tmp.PutStr(com);

	int ck = rand();
	CBuffer out;
	out.Put16Bit(ck);
	out.Put16Bit(ck);

	out.PutBIGNUM(m_Rsa->d);
	out.PutBIGNUM(m_Rsa->iqmp);
	out.PutBIGNUM(m_Rsa->q);
	out.PutBIGNUM(m_Rsa->p);

	CCipher cip;
	CBuffer key;
	cip.MakeKey(&key, pass);
	cip.Init(cip.GetName(SSH_CIPHER_3DES), MODE_ENC, key.GetPtr(), key.GetSize());
	cip.Cipher(out.GetPtr(), out.GetSize(), &tmp);

	if ( fwrite(tmp.GetPtr(), tmp.GetSize(), 1, fp) != 1 )
		return FALSE;

	return TRUE;
}
int CIdKey::LoadRsa1Key(FILE *fp, LPCSTR pass)
{
	TRY {
		fseek(fp, 0L, SEEK_END);
		long len = ftell(fp);
		CBuffer buf;
		LPBYTE p = buf.PutSpc(len);
		fseek(fp, 0L, SEEK_SET);
		fread(p, 1, len, fp);

		if ( memcmp(p, authfile_id_string, sizeof(authfile_id_string)) != 0 )
			AfxThrowUserException();

		buf.Consume(sizeof(authfile_id_string));
		int ctype = buf.Get8Bit();		// cipher type
		buf.Get32Bit();					// reserved 
		buf.Get32Bit();					// reserved

		Create(IDKEY_RSA1);
		buf.GetBIGNUM(m_Rsa->n);
		buf.GetBIGNUM(m_Rsa->e);

		CString com;
		buf.GetStr(com);

		CCipher cip;
		CBuffer out, key;
		cip.MakeKey(&key, pass);
		cip.Init(cip.GetName(ctype), MODE_DEC, key.GetPtr(), key.GetSize());
		cip.Cipher(buf.GetPtr(), buf.GetSize(), &out);

		int ck1 = out.Get8Bit();
		int ck2 = out.Get8Bit();
		if ( ck1 != out.Get8Bit() || ck2 != out.Get8Bit() )
			AfxThrowUserException();

		out.GetBIGNUM(m_Rsa->d);
		out.GetBIGNUM(m_Rsa->iqmp);
		out.GetBIGNUM(m_Rsa->q);
		out.GetBIGNUM(m_Rsa->p);

		if ( RSA_blinding_on(m_Rsa, NULL) != 1 )
			AfxThrowUserException();

		m_Type = IDKEY_RSA1;
		fclose(fp);
		return TRUE;
	} CATCH(CUserException, e) {
		fclose(fp);
		return FALSE;
	} END_CATCH
}

int	CIdKey::LoadPrivateKey(LPCSTR file, LPCSTR pass)
{
	FILE *fp;
	EVP_PKEY *pk;

	((CRLoginApp *)AfxGetApp())->SSL_Init();

	if ( (fp = fopen(file, "r")) == NULL )
		return FALSE;

	pk = PEM_read_PrivateKey(fp, NULL, NULL, (void *)pass);

	if ( pk == NULL )
		return LoadRsa1Key(fp, pass);

	m_Type = IDKEY_NONE;
	switch(pk->type) {
	case EVP_PKEY_RSA:
		if ( m_Rsa != NULL )
			RSA_free(m_Rsa);
		m_Rsa = EVP_PKEY_get1_RSA(pk);
		m_Type = IDKEY_RSA2;
		break;
	case EVP_PKEY_DSA:
		if ( m_Dsa != NULL )
			DSA_free(m_Dsa);
		m_Dsa = EVP_PKEY_get1_DSA(pk);
		m_Type = IDKEY_DSA2;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}
int CIdKey::SavePrivateKey(int type, LPCSTR file, LPCSTR pass)
{
	FILE *fp;
	int rt = FALSE;
	int len = (int)strlen(pass);
//	const EVP_CIPHER *cipher = EVP_des_ede3_cbc();
	const EVP_CIPHER *cipher = EVP_aes_128_cbc();

	if ( (fp = fopen(file, "w")) == NULL )
		return FALSE;

	switch(type) {
	case IDKEY_RSA1:
		if ( m_Rsa != NULL )
			rt = SaveRsa1Key(fp, pass);
		break;
	case IDKEY_RSA2:
		if ( m_Rsa != NULL )
			rt = PEM_write_RSAPrivateKey(fp, m_Rsa, cipher, (unsigned char *)pass, len, NULL, NULL);
		break;
	case IDKEY_DSA2:
		if ( m_Dsa != NULL )
			rt = PEM_write_DSAPrivateKey(fp, m_Dsa, cipher, (unsigned char *)pass, len, NULL, NULL);
		break;
	}
	
	fclose(fp);
	return rt;
}
int CIdKey::GetSize()
{
	switch (m_Type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		return BN_num_bits(m_Rsa->n);
	case IDKEY_DSA2:
		return BN_num_bits(m_Dsa->p);
	}
	return 0;
}
void CIdKey::FingerPrint(CString &str)
{
#define	FLDSIZE_X		16
#define	FLDSIZE_Y		8

	int n, b, c;
	int x, y;
	const EVP_MD *md = EVP_md5();
	EVP_MD_CTX ctx;
	CBuffer blob;
	unsigned int len;
	u_char tmp[EVP_MAX_MD_SIZE];
	u_char map[FLDSIZE_X][FLDSIZE_Y];
	static const char *augmentation_string = " .o+=*BOX@%&#/^SE";

	str.Empty();
	if ( !SetBlob(&blob) ) {
		if ( m_Type != IDKEY_RSA1 )
			return;
		blob.PutBIGNUM2(m_Rsa->n);
		blob.PutBIGNUM2(m_Rsa->e);
	}

	EVP_DigestInit(&ctx, md);
	EVP_DigestUpdate(&ctx, blob.GetPtr(), blob.GetSize());
	EVP_DigestFinal(&ctx, tmp, &len);

	memset(map, 0, sizeof(map));
	x = FLDSIZE_X / 2;
	y = FLDSIZE_Y / 2;

	for ( n = 0 ; n < len ; n++ ) {
		c = tmp[n];
		for ( b = 0 ; b < 4 ; b++ ) {
			if ( (x += (c & 1) ? 1 : -1) < 0 )
				x = 0;
			else if ( x >= FLDSIZE_X )
				x = FLDSIZE_X - 1;

			if ( (y += (c & 2) ? 1 : -1) < 0 )
				y = 0;
			else if ( y >= FLDSIZE_Y )
				y = FLDSIZE_X - 1;

			map[x][y]++;
			c >>= 2;
		}
	}

	n = strlen(augmentation_string) - 1;
	map[FLDSIZE_X / 2][FLDSIZE_Y / 2] = n - 1;
	map[x][y] = n;

	switch (m_Type) {
	case IDKEY_RSA1:
		str.Format("+--[RSA1 %4d]--+\r\n", GetSize());
		break;
	case IDKEY_RSA2:
		str.Format("+--[RSA2 %4d]--+\r\n", GetSize());
		break;
	case IDKEY_DSA2:
		str.Format("+--[DSA2 %4d]--+\r\n", GetSize());
		break;
	}

	for ( y = 0 ; y < FLDSIZE_Y ; y++ ) {
		str += "|";
		for ( x = 0 ; x < FLDSIZE_X ; x++ )
			str += (map[x][y] >= n ? 'E' : augmentation_string[map[x][y]]);
		str += "|\n";
	}

	str += "+--------------+\r\n";
}

//////////////////////////////////////////////////////////////////////
// CIdKeyTab

CIdKeyTab::CIdKeyTab()
{
	m_pSection = "IdKeyTab";
	Serialize(FALSE);
}
CIdKeyTab::~CIdKeyTab()
{
}
void CIdKeyTab::Init()
{
	m_Data.RemoveAll();
}
void CIdKeyTab::SetArray(CStringArrayExt &array)
{
	CString str;
	array.RemoveAll();
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_Data[n].SetString(str);
		array.Add(str);
	}
}
void CIdKeyTab::GetArray(CStringArrayExt &array)
{
	CIdKey tmp;
	m_Data.RemoveAll();
	for ( int n = 0 ; n < array.GetSize() ; n++ ) {
		if ( tmp.GetString(array[n]) )
			m_Data.Add(tmp);
	}
}
void CIdKeyTab::Serialize(int mode)
{
	int n;

	if ( mode ) {		// Write
		for ( n = 0 ; n < m_Data.GetSize() ; n++ )
			m_Data[n].SetProfile(m_pSection);
	} else {			// Read
		CIdKey tmp;
		CStringArrayExt entry;
		CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
		pApp->GetProfileKeys(m_pSection, entry);
		m_Data.RemoveAll();
		for ( n = 0 ; n < entry.GetSize() ; n++ ) {
			if ( entry[n].Left(4).Compare("List") != 0 || strchr("0123456789", entry[n][4]) == NULL )
				continue;
			if ( tmp.GetString(pApp->GetProfileString(m_pSection, entry[n], "")) )
				m_Data.Add(tmp);
		}
	}
}
const CIdKeyTab & CIdKeyTab::operator = (CIdKeyTab &data)
{
	m_Data.RemoveAll();
	for ( int n = 0 ; n < data.m_Data.GetSize() ; n++ )
		m_Data.Add(data.m_Data[n]);
	return *this;
}
int CIdKeyTab::AddEntry(CIdKey &key)
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].Compere(&key) == 0 )
			return FALSE;
	}
	key.m_Uid = ((CRLoginApp *)AfxGetApp())->GetProfileSeqNum(m_pSection, "ListMax");
	m_Data.Add(key);
	key.SetProfile(m_pSection);
	return TRUE;
}
CIdKey *CIdKeyTab::GetUid(int uid)
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].m_Uid == uid )
			return &(m_Data[n]);
	}
	CIdKey tmp;
	if ( tmp.GetProfile(m_pSection, uid) )
		return &(m_Data[m_Data.Add(tmp)]);
	return NULL;
}
void CIdKeyTab::UpdateUid(int uid)
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].m_Uid == uid ) {
			m_Data[n].SetProfile(m_pSection);
			return;
		}
	}
}
void CIdKeyTab::RemoveUid(int uid)
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].m_Uid == uid ) {
			m_Data[n].DelProfile(m_pSection);
			m_Data.RemoveAt(n);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CPermit
//////////////////////////////////////////////////////////////////////

CPermit::CPermit()
{
	m_lHost = _T("");
	m_lPort = 0;
	m_rHost = _T("");
	m_rPort = 0;
}
CPermit::~CPermit()
{
}
const CPermit & CPermit::operator = (CPermit &data)
{
	m_lHost	= data.m_lHost;
	m_lPort	= data.m_lPort;
	m_rHost	= data.m_rHost;
	m_rPort	= data.m_rPort;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CFilter
//////////////////////////////////////////////////////////////////////

CFilter::CFilter()
{
	m_Type = 0;
	m_pChan = NULL;
	m_pNext = NULL;
}
CFilter::~CFilter()
{
}
void CFilter::Destroy()
{
	delete this;
}
void CFilter::Close()
{
	m_pChan->DoClose();
}
void CFilter::OnConnect()
{
}
void CFilter::Send(LPBYTE buf, int len)
{
	m_pChan->m_Output.Apend(buf, len);
	m_pChan->m_pSsh->SendMsgChannelData(m_pChan->m_LocalID);
}
void CFilter::OnRecive(const void *lpBuf, int nBufLen)
{
}
void CFilter::OnSendEmpty()
{
}
int CFilter::GetSendSize()
{
	return 0;
}
int CFilter::GetRecvSize()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CChannel
//////////////////////////////////////////////////////////////////////

CChannel::CChannel():CExtSocket(NULL)
{
	m_pSsh = NULL;
	m_Status = 0;
	m_Next = (-1);
	m_pFilter = NULL;
}
CChannel::~CChannel()
{
	Close();
}
const CChannel & CChannel::operator = (CChannel &data)
{
	m_Fd			= data.m_Fd;
	m_pDocument     = data.m_pDocument;

	m_Status		= data.m_Status;
	m_RemoteID		= data.m_RemoteID;
	m_LocalID		= data.m_LocalID;
	m_LocalComs		= data.m_LocalComs;
	m_LocalWind		= data.m_LocalWind;
	m_RemoteWind	= data.m_RemoteWind;
	m_RemoteMax		= data.m_RemoteMax;
	m_Input			= data.m_Input;
	m_Output		= data.m_Output;
	m_TypeName		= data.m_TypeName;
	m_Eof			= data.m_Eof;
	m_Next			= data.m_Next;
	m_lHost			= data.m_lHost;
	m_lPort			= data.m_lPort;
	m_rHost			= data.m_rHost;
	m_rPort			= data.m_rPort;
	m_ReadByte		= data.m_ReadByte;
	m_WriteByte		= data.m_WriteByte;
	m_ConnectTime	= data.m_ConnectTime;

	return *this;
}
int CChannel::Create(LPCTSTR lpszHostAddress, UINT nHostPort, LPCSTR lpszRemoteAddress, UINT nRemotePort)
{
	m_lHost	= (lpszHostAddress != NULL ? lpszHostAddress : "");
	m_lPort	= nHostPort;
	m_rHost	= (lpszRemoteAddress != NULL ? lpszRemoteAddress : "");
	m_rPort	= nRemotePort;

	return CExtSocket::Create(lpszHostAddress, nHostPort, lpszRemoteAddress);
}
void CChannel::Close()
{
	m_Status = 0;
	m_Input.Clear();
	m_Output.Clear();
	CExtSocket::Close();

	if ( m_pFilter != NULL ) {
		m_pFilter->m_pChan = NULL;
		m_pFilter->Destroy();
		m_pFilter = NULL;
	}

	((CRLoginApp *)AfxGetApp())->DelSocketIdle(this);
}
int CChannel::Send(const void *lpBuf, int nBufLen, int nFlags)
{
	m_WriteByte += nBufLen;

	if ( m_pFilter != NULL )
		m_pFilter->OnRecive(lpBuf, nBufLen);
	else
		CExtSocket::Send(lpBuf, nBufLen, nFlags);

	return nBufLen;
}
void CChannel::DoClose()
{
	switch(m_Status & (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE)) {
	case CHAN_OPEN_REMOTE:
		m_pSsh->SendMsgChannelOpenFailure(m_LocalID);
		break;
	case CHAN_OPEN_LOCAL:
		m_Eof |= CEOF_DEAD;
		break;
	case CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE:
		if ( (m_Eof & CEOF_DEAD) == 0 ) {
			m_Eof |= CEOF_DEAD;
			m_pSsh->ChannelCheck(m_LocalID);
		}
		break;
	}
}
void CChannel::OnError(int err)
{
	TRACE("Cannel OnError %d\n", err);
	DoClose();
}
void CChannel::OnClose()
{
	TRACE("Cannel OnClose\n");
	DoClose();
}
void CChannel::OnConnect()
{
	TRACE("Cannel OnConnect\n");
	if ( m_pFilter != NULL )
		m_pFilter->OnConnect();
	else
		m_pSsh->SendMsgChannelOpenConfirmation(m_LocalID);
}
void CChannel::OnAccept(SOCKET hand)
{
	m_pSsh->ChannelAccept(m_LocalID, hand);
}
void CChannel::OnReciveCallBack(void *lpBuf, int nBufLen, int nFlags)
{
	if ( nFlags != 0 )
		return;

	m_ReadByte += nBufLen;

	if ( (m_Status & CHAN_PROXY_SOCKS) != 0 ) {
		m_Output.Apend((LPBYTE)lpBuf, nBufLen);
		m_pSsh->DecodeProxySocks(m_LocalID);
	} else
		CExtSocket::OnReciveCallBack(lpBuf, nBufLen, nFlags);
}
int CChannel::OnReciveProcBack(void *lpBuf, int nBufLen, int nFlags)
{
	int n;

	if ( m_Status != (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE) )
		return 0;

	if ( (m_Eof & (CEOF_ICLOSE | CEOF_SEOF | CEOF_SCLOSE | CEOF_DEAD)) != 0 )
		return nBufLen;

	if ( (n = m_RemoteWind - m_Output.GetSize()) <= 0 ) {
		m_pSsh->SendMsgChannelData(m_LocalID);
		return 0;
	} else if ( nBufLen > n )
		nBufLen = n;

//	TRACE("Chanel #%d ProcBack %d\n", m_LocalID, nBufLen);

	m_Output.Apend((LPBYTE)lpBuf, nBufLen);
	m_pSsh->SendMsgChannelData(m_LocalID);

	return nBufLen;
}
int CChannel::GetSendSize()
{
	if ( m_pFilter != NULL )
		return m_pFilter->GetSendSize();
	else
		return (CExtSocket::GetSendSize() + m_Input.GetSize());
}
int CChannel::GetRecvSize()
{
	if ( m_pFilter != NULL )
		return m_pFilter->GetRecvSize();
	else
		return (CExtSocket::GetRecvSize() + m_Output.GetSize());
}
void CChannel::OnRecvEmpty()
{
}
void CChannel::OnSendEmpty()
{
	m_pSsh->ChannelPolling(m_LocalID);
}

//////////////////////////////////////////////////////////////////////
// CStdIoFilter
//////////////////////////////////////////////////////////////////////

CStdIoFilter::CStdIoFilter()
{
	m_Type = 1;
}
void CStdIoFilter::OnRecive(const void *lpBuf, int nBufLen)
{
	m_pChan->m_pSsh->CExtSocket::OnReciveCallBack((void *)lpBuf, nBufLen, 0);
}
int CStdIoFilter::GetSendSize()
{
	return m_pChan->m_pSsh->GetRecvSize();
}

//////////////////////////////////////////////////////////////////////
// CSFtpFilter
//////////////////////////////////////////////////////////////////////

CSFtpFilter::CSFtpFilter()
{
	m_Type = 2;
	m_pSFtp = NULL;
}
void CSFtpFilter::Destroy()
{
	if ( m_pSFtp != NULL ) {
		if ( m_pSFtp->m_hWnd != NULL )
			m_pSFtp->DestroyWindow();
		else
			delete m_pSFtp;
	}
	CFilter::Destroy();
}
void CSFtpFilter::OnConnect()
{
	if ( m_pSFtp != NULL )
		m_pSFtp->OnConnect();
}
void CSFtpFilter::OnRecive(const void *lpBuf, int nBufLen)
{
	if ( m_pSFtp != NULL )
		m_pSFtp->OnRecive(lpBuf, nBufLen);
}

//////////////////////////////////////////////////////////////////////
// CAgent
//////////////////////////////////////////////////////////////////////

CAgent::CAgent()
{
	m_Type = 3;
}
void CAgent::OnRecive(const void *lpBuf, int nBufLen)
{
	int n;
	CBuffer tmp;

	m_RecvBuf.Apend((LPBYTE)lpBuf, nBufLen);

	while ( m_RecvBuf.GetSize() >= 4 ) {
		n = m_RecvBuf.PTR32BIT(m_RecvBuf.GetPtr());
		if ( n > (256 * 1024) || n < 0 )
			AfxThrowUserException();
		if ( m_RecvBuf.GetSize() < (n + 4) )
			break;
		tmp.Clear();
		tmp.Apend(m_RecvBuf.GetPtr() + 4, n);
		m_RecvBuf.Consume(n + 4);
		ReciveBuffer(&tmp);
	}
}
CIdKey *CAgent::GetIdKey(CIdKey *key)
{
	int n;
	CIdKey *pKey;
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	for ( n = 0 ; n < m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.GetSize() ; n++ ) {
		if ( (pKey = pMain->m_IdKeyTab.GetUid(m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.GetVal(n))) != NULL && pKey->m_Type != IDKEY_NONE ) {
			if ( pKey->Compere(key) == 0 )
				return pKey;
		}
	}
	return NULL;
}
void CAgent::ReciveBuffer(CBuffer *bp)
{
	CBuffer tmp;
	int type = bp->Get8Bit();

	switch(type) {
	case SSH_AGENTC_LOCK:
	case SSH_AGENTC_UNLOCK:
		tmp.Put32Bit(1);
		tmp.Put8Bit(SSH_AGENT_SUCCESS);
		Send(tmp.GetPtr(), tmp.GetSize());
		break;

	case SSH2_AGENTC_SIGN_REQUEST:
	{
		int flag;
		CBuffer blob, data, sig;
		CIdKey key, *pkey;

		bp->GetBuf(&blob);
		bp->GetBuf(&data);
		flag = bp->Get32Bit();
		key.GetBlob(&blob);
		if ( (pkey = GetIdKey(&key)) != NULL && pkey->Sign(&sig, data.GetPtr(), data.GetSize()) ) {
			data.Clear();
			data.Put8Bit(SSH2_AGENT_SIGN_RESPONSE);
			data.PutBuf(sig.GetPtr(), sig.GetSize());
			tmp.Put32Bit(data.GetSize());
			tmp.Apend(data.GetPtr(), data.GetSize());
			Send(tmp.GetPtr(), tmp.GetSize());
		} else {
			tmp.Put32Bit(1);
			tmp.Put8Bit(SSH_AGENT_FAILURE);
			Send(tmp.GetPtr(), tmp.GetSize());
		}
		break;
	}

	case SSH2_AGENTC_REQUEST_IDENTITIES:
	{
		int n, i = 0;
		CIdKey *pKey;
		CBuffer data, work;
		CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
		for ( n = 0 ; n < m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.GetSize() ; n++ ) {
			if ( (pKey = pMain->m_IdKeyTab.GetUid(m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.GetVal(n))) != NULL && pKey->m_Type != IDKEY_NONE ) {
				switch(pKey->m_Type) {
				case IDKEY_RSA1:
				case IDKEY_RSA2:
					data.Put32Bit(BN_num_bits(pKey->m_Rsa->n));
					data.PutBIGNUM(pKey->m_Rsa->e);
					data.PutBIGNUM(pKey->m_Rsa->n);
					break;
				case IDKEY_DSA2:
					work.Clear();
					pKey->SetBlob(&work);
					data.PutBuf(work.GetPtr(), work.GetSize());
					break;
				}
				data.PutStr(pKey->m_Name);
				i++;
			}
		}
		work.Clear();
		work.Put8Bit(SSH2_AGENT_IDENTITIES_ANSWER);
		work.Put32Bit(i);
		work.Apend(data.GetPtr(), data.GetSize());

		tmp.Put32Bit(work.GetSize());
		tmp.Apend(work.GetPtr(), work.GetSize());
		Send(tmp.GetPtr(), tmp.GetSize());
		break;
	}

	case SSH2_AGENTC_ADD_IDENTITY:
	case SSH2_AGENTC_ADD_ID_CONSTRAINED:
	{
		int type;
		CString name;
		CIdKey key;

		bp->GetStr(name);
		type = key.GetTypeFromName(name);
		if ( type == IDKEY_RSA2 && key.Create(IDKEY_RSA2) ) {
			key.Create(IDKEY_RSA2);
			bp->GetBIGNUM2(key.m_Rsa->n);
			bp->GetBIGNUM2(key.m_Rsa->e);
			bp->GetBIGNUM2(key.m_Rsa->d);
			bp->GetBIGNUM2(key.m_Rsa->iqmp);
			bp->GetBIGNUM2(key.m_Rsa->p);
			bp->GetBIGNUM2(key.m_Rsa->q);
		} else if ( type == IDKEY_DSA2 && key.Create(IDKEY_DSA2) ) {
			bp->GetBIGNUM2(key.m_Dsa->p);
			bp->GetBIGNUM2(key.m_Dsa->q);
			bp->GetBIGNUM2(key.m_Dsa->g);
			bp->GetBIGNUM2(key.m_Dsa->pub_key);
			bp->GetBIGNUM2(key.m_Dsa->priv_key);
		} else {
			tmp.Put32Bit(1);
			tmp.Put8Bit(SSH_AGENT_FAILURE);
			Send(tmp.GetPtr(), tmp.GetSize());
			break;
		}
		bp->GetStr(key.m_Name);
		if ( ((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab.AddEntry(key) )
			m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.AddVal(key.m_Uid);
		tmp.Put32Bit(1);
		tmp.Put8Bit(SSH_AGENT_SUCCESS);
		Send(tmp.GetPtr(), tmp.GetSize());
		break;
	}

	case SSH2_AGENTC_REMOVE_IDENTITY:
	{
		CBuffer blob;
		CIdKey key, *pKey;

		bp->GetBuf(&blob);
		key.GetBlob(&blob);
		if ( (pKey = GetIdKey(&key)) != NULL ) {
			((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab.RemoveUid(pKey->m_Uid);
			tmp.Put32Bit(1);
			tmp.Put8Bit(SSH_AGENT_SUCCESS);
			Send(tmp.GetPtr(), tmp.GetSize());
		} else {
			tmp.Put32Bit(1);
			tmp.Put8Bit(SSH_AGENT_FAILURE);
			Send(tmp.GetPtr(), tmp.GetSize());
		}
		break;
	}

	case SSH2_AGENTC_REMOVE_ALL_IDENTITIES:
	case SSH_AGENTC_REQUEST_RSA_IDENTITIES:
	default:
		tmp.Put32Bit(1);
		tmp.Put8Bit(SSH_AGENT_FAILURE);
		Send(tmp.GetPtr(), tmp.GetSize());
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// CRcpUpload
//////////////////////////////////////////////////////////////////////

CRcpUpload::CRcpUpload()
{
	m_Type = 4;
	m_Fd   = (-1);
	m_Mode = 0;
}
CRcpUpload::~CRcpUpload()
{
}
void CRcpUpload::Close()
{
	if ( m_ProgDlg.m_hWnd != NULL )
		m_ProgDlg.DestroyWindow();

	if ( m_Fd != (-1) )
		_close(m_Fd);
	m_Fd = (-1);

	CFilter *fp;
	if ( (fp = m_pSsh->m_pListFilter) == this )
		m_pSsh->m_pListFilter = m_pNext;
	else {
		if ( fp->m_pNext == this )
			fp->m_pNext = m_pNext;
	}
	m_pNext = NULL;

	CFilter::Close();
}
void CRcpUpload::HostKanjiStr(LPCSTR str, CString &tmp)
{
	m_pSsh->m_pDocument->m_TextRam.m_IConv.IConvStr("CP932", m_pSsh->m_pDocument->m_TextRam.m_SendCharSet[m_pSsh->m_pDocument->m_TextRam.m_KanjiMode], str, tmp);
}
void CRcpUpload::OnSendEmpty()
{
	int n = CHAN_SES_PACKET_DEFAULT * 2;
	BYTE tmp[CHAN_SES_PACKET_DEFAULT * 2];

	if ( m_Mode != 3 || m_pChan == NULL || m_pChan->m_Output.GetSize() > 0 )
		return;

	if ( (m_Size + n) > m_Stat.st_size )
		n = (int)(m_Stat.st_size - m_Size);

	if ( n <= 0 ) {
		m_Mode = 5;
		tmp[0] = 0;
		Send(tmp, 1);
		return;
	}

	if ( m_ProgDlg.m_AbortFlag || _read(m_Fd, tmp, n) != n ) {
		Close();
		return;
	}

	Send(tmp, n);
	m_Size += n;
	m_ProgDlg.SetPos(m_Size);
}
void CRcpUpload::OnRecive(const void *lpBuf, int nBufLen)
{
	int c;
	LPBYTE p = (LPBYTE)lpBuf;
	LPBYTE e = (LPBYTE)lpBuf + nBufLen;
	CString str, tmp;

	while ( p < e ) {
		c = *(p++);
		switch(m_Mode) {
		case 0:
			if ( c != '\0' ) {
				Close();
				return;
			}
			str.Format("T%I64u 0 %I64u 0\n", m_Stat.st_mtime, m_Stat.st_atime);
			Send((LPBYTE)(LPCSTR)str, str.GetLength());
			m_Mode = 1;
			break;
		case 1:
			if ( c != '\0' ) {
				Close();
				return;
			}
			str.Format("C%04o %I64d %s\n", (m_Stat.st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)), m_Stat.st_size, m_File);
			HostKanjiStr(str, tmp);
			Send((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
			m_Size = 0;
			m_Mode = 2;
			break;
		case 2:
			if ( c != '\0' ) {
				Close();
				return;
			}
			m_ProgDlg.Create(IDD_PROGDLG, AfxGetMainWnd());
			m_ProgDlg.ShowWindow(SW_SHOW);
			m_ProgDlg.SetFileName(m_Path);
			m_ProgDlg.SetRange(m_Stat.st_size, 0);
			m_Mode = 3;
			OnSendEmpty();
			break;
		case 5:
			m_ProgDlg.SetPos(m_Size);
			Close();
			return;
		}
	}
}
void CRcpUpload::Destroy()
{
	if ( m_FileList.IsEmpty() || m_pChan != NULL )
		return;

	char *p;
	CKeyNode fmt;
	CStringW cmd;

	m_Mode = 0;
	m_Path = m_FileList.RemoveHead();
	if ( (p = strrchr((char *)(LPCSTR)m_Path, '\\')) != NULL || (p = strrchr((char *)(LPCSTR)m_Path, ':')) != NULL )
		m_File = p + 1;
	else
		m_File = m_Path;

	if ( _stati64(m_Path, &m_Stat) || (m_Stat.st_mode & _S_IFMT) == _S_IFDIR )
		return;

	if ( (m_Fd = _open(m_Path, _O_BINARY | _O_RDONLY)) == (-1) )
		return;

	fmt.SetMaps(m_pSsh->m_pDocument->m_TextRam.m_DropFileCmd[5]);
	fmt.CommandLine(CStringW(m_File), cmd);
	HostKanjiStr(CString(cmd), m_Cmd);
	m_pSsh->OpenRcpUpload(NULL);
}

//////////////////////////////////////////////////////////////////////
// Cssh 構築/消滅
//////////////////////////////////////////////////////////////////////

Cssh::Cssh(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = 3;
	m_SaveDh = NULL;
	m_SSHVer = 0;
	m_InPackStat = 0;
	m_pListFilter = NULL;

	for ( int n = 0 ; n < 6 ; n++ )
		m_VKey[n] = NULL;
}
Cssh::~Cssh()
{
	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);

	for ( int n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
	}

	((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);
}
int Cssh::SetIdKeyList()
{
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CIdKey *pKey;

	m_IdKey.Close();
	while ( m_IdKeyPos < m_pDocument->m_ParamTab.m_IdKeyList.GetSize() ) {
		if ( (pKey = pMain->m_IdKeyTab.GetUid(m_pDocument->m_ParamTab.m_IdKeyList.GetVal(m_IdKeyPos++))) != NULL ) {
			m_IdKey = *pKey;
			if ( m_IdKey.m_Type == IDKEY_NONE )
				continue;
			return TRUE;
		}
	}
	return FALSE;
}
int Cssh::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
  TRY {
	m_SSHVer = 0;
	m_ServerVerStr = "";
	m_ClientVerStr = "";
	m_InPackStat = 0;
	m_PacketStat = 0;
	m_SSH2Status = 0;
	m_SendPackSeq = 0;
	m_RecvPackSeq = 0;
	m_CipherMode = SSH_CIPHER_NONE;
	m_SessionIdLen = 0;
	m_StdChan = (-1);
	m_AuthStat = 0;
	m_HostName = m_pDocument->m_ServerEntry.m_HostName;
	m_DhMode = DHMODE_GROUP_1;
	m_GlbReqMap.RemoveAll();
	m_ChnReqMap.RemoveAll();
	m_Chan.RemoveAll();
	m_ChanNext = m_ChanFree = m_ChanFree2 = (-1);
	m_OpenChanCount = 0;
	m_pListFilter = NULL;
	m_Permit.RemoveAll();
	m_MyPeer.Clear();
	m_HisPeer.Clear();
	m_Incom.Clear();
	m_InPackBuf.Clear();
	m_IdKey.Close();
	m_IdKeyPos = 0;

	srand((UINT)time(NULL));

	if ( !m_pDocument->m_ServerEntry.m_IdkeyName.IsEmpty() ) {
		if ( !m_IdKey.LoadPrivateKey(m_pDocument->m_ServerEntry.m_IdkeyName, m_pDocument->m_ServerEntry.m_PassName) ) {
			CIdKeyFileDlg dlg;
			dlg.m_OpenMode  = 1;
			dlg.m_Title = "SSH鍵ファイルの読み込み";
			dlg.m_Message = "作成時に設定したパスフレーズを入力してください";
			dlg.m_IdkeyFile = m_pDocument->m_ServerEntry.m_IdkeyName;
			if ( dlg.DoModal() != IDOK ) {
				m_pDocument->m_ServerEntry.m_IdkeyName.Empty();
				return FALSE;
			}
			if ( !m_IdKey.LoadPrivateKey(dlg.m_IdkeyFile, dlg.m_PassName) ) {
				AfxMessageBox("鍵ファイルを読み込めませんでした");
				return FALSE;
			}
		}
		m_IdKey.m_Pass = m_pDocument->m_ServerEntry.m_PassName;
	} else
		SetIdKeyList();

	m_EncCip.Init("none", MODE_ENC);
	m_DecCip.Init("none", MODE_DEC);
	m_EncMac.Init("none");
	m_DecMac.Init("none");
	m_EncCmp.Init("none", MODE_ENC);
	m_DecCmp.Init("none", MODE_DEC);

	int n;
	CString str;

	m_CipTabMax = 0;
	for ( n = 0 ; n < 8 && m_pDocument->m_ParamTab.GetPropNode(0, n, str) ; n++ )
		m_CipTab[m_CipTabMax++] = m_EncCip.GetNum(str);

	n = m_pDocument->m_ParamTab.GetPropNode(2, 0, str);
	m_CompMode = (n == FALSE || str.Compare("none") == 0 ? FALSE : TRUE);

	if ( !CExtSocket::Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType) )
		return FALSE;

	SetRecvSize(CHAN_SES_WINDOW_DEFAULT);

	return TRUE;
  } CATCH(CUserException, e) {
	AfxMessageBox("SSH Packet Buffer Error In Open");
	return FALSE;
  } END_CATCH
}
void Cssh::Close()
{
	m_Chan.RemoveAll();
	CExtSocket::Close();
}
int Cssh::Send(const void* lpBuf, int nBufLen, int nFlags)
{
  TRY {
	CBuffer tmp;
	switch(m_SSHVer) {
	case 1:
		tmp.Put8Bit(SSH_CMSG_STDIN_DATA);
		tmp.PutBuf((LPBYTE)lpBuf, nBufLen);
		SendPacket(&tmp);
		break;
	case 2:
		if ( m_StdChan == (-1) || m_StdInRes.GetSize() > 0 || !CHAN_OK(m_StdChan) )
			m_StdInRes.Apend((LPBYTE)lpBuf, nBufLen);
		else
			m_Chan[m_StdChan].m_pFilter->Send((LPBYTE)lpBuf, nBufLen);
		break;
	}
	return nBufLen;
  } CATCH(CUserException, e) {
	AfxMessageBox("SSH Packet Buffer Error In Send");
	return 0;
  } END_CATCH
}
void Cssh::OnReciveCallBack(void* lpBuf, int nBufLen, int nFlags)
{
  TRY {
	int crc, ch;
	CBuffer *bp, tmp, cmp, dec;
	CString str;

	if ( nFlags != 0 )
		return;

	m_Incom.Apend((LPBYTE)lpBuf, nBufLen);

	for ( ; ; ) {
		switch(m_InPackStat) {
		case 0:		// Fast Verstion String
			while ( m_Incom.GetSize() > 0 ) {
				ch = m_Incom.Get8Bit();
				if ( ch == '\n' ) {
					if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSH1MODE) &&
						   (m_ServerVerStr.Mid(0, 5).Compare("SSH-2") == 0 ||
							m_ServerVerStr.Mid(0, 8).Compare("SSH-1.99") == 0) ) {
						m_ClientVerStr = "SSH-2.0-OpenSSH-4.0 RLogin-1.10";
						m_InPackStat = 3;
						m_SSHVer = 2;
					} else {
						m_ClientVerStr = "SSH-1.5-OpenSSH-4.0 RLogin-1.10";
						m_InPackStat = 1;
						m_SSHVer = 1;
					}
					str.Format("%s\n", m_ClientVerStr);
					CExtSocket::Send((LPCSTR)str, str.GetLength());
					break;
				} else if ( ch != '\r' )
					m_ServerVerStr += (char)ch;
			}
			break;

		case 1:		// SSH1 Packet Length
			if ( m_Incom.GetSize() < 4 )
				goto BREAKOUT;
			m_InPackLen = m_Incom.Get32Bit();
			m_InPackPad = 8 - (m_InPackLen % 8);
			if ( m_InPackLen < 4 || m_InPackLen > (256 * 1024) ) {	// Error Packet Length
				SendDisconnect("Packet length Error");
				AfxThrowUserException();
			}
			m_InPackStat = 2;
			// break; Not use
		case 2:		// SSH1 Packet
			if ( m_Incom.GetSize() < (m_InPackLen + m_InPackPad) )
				goto BREAKOUT;
			tmp.Clear();
			tmp.Apend(m_Incom.GetPtr(), m_InPackLen + m_InPackPad);
			m_Incom.Consume(m_InPackLen + m_InPackPad);
			m_InPackStat = 1;
			bp = &tmp;
			dec.Clear();
			m_DecCip.Cipher(bp->GetPtr(), bp->GetSize(), &dec);
			bp = &dec;
			crc = bp->PTR32BIT(bp->GetPos(bp->GetSize() - 4));
			if ( crc != ssh_crc32(bp->GetPtr(), bp->GetSize() - 4) ) {
				SendDisconnect("Packet length Error");
				AfxThrowUserException();
			}
			bp->Consume(m_InPackPad);
			bp->ConsumeEnd(4);
			cmp.Clear();
			m_DecCmp.UnCompress(bp->GetPtr(), bp->GetSize(), &cmp);
			bp = &cmp;
			RecivePacket(bp);
			break;

		case 3:		// SSH2 Packet Lenght
			if ( m_Incom.GetSize() < m_DecCip.GetBlockSize() )
				goto BREAKOUT;
			m_InPackBuf.Clear();
			m_DecCip.Cipher(m_Incom.GetPtr(), m_DecCip.GetBlockSize(), &m_InPackBuf);
			m_Incom.Consume(m_DecCip.GetBlockSize());
			m_InPackLen = m_InPackBuf.PTR32BIT(m_InPackBuf.GetPtr());
			if ( m_InPackLen < 5 || m_InPackLen > (256 * 1024) ) {
				SendDisconnect2(1, "Packet Len Error");
				AfxThrowUserException();
			}
			m_InPackStat = 4;
			// break; Not use
		case 4:		// SSH2 Packet
			if ( m_Incom.GetSize() < (m_InPackLen - m_DecCip.GetBlockSize() + 4 + m_DecMac.GetBlockSize()) )
				goto BREAKOUT;
			m_InPackStat = 3;
			m_DecCip.Cipher(m_Incom.GetPtr(), m_InPackLen - m_DecCip.GetBlockSize() + 4, &m_InPackBuf);
			m_Incom.Consume(m_InPackLen - m_DecCip.GetBlockSize() + 4);
			bp = &m_InPackBuf;
			tmp.Clear();
			m_DecMac.Compute(m_RecvPackSeq, bp->GetPtr(), bp->GetSize(), &tmp);
			m_InPackLen = bp->Get32Bit();
			m_InPackPad = bp->Get8Bit();
			bp->ConsumeEnd(m_InPackPad);
			cmp.Clear();
			m_DecCmp.UnCompress(bp->GetPtr(), bp->GetSize(), &cmp);
			bp = &cmp;
			if ( m_DecMac.GetBlockSize() > 0 ) {
				if ( memcmp(tmp.GetPtr(), m_Incom.GetPtr(), m_DecMac.GetBlockSize()) != 0 ) {
					SendDisconnect2(1, "MAC Error");
					AfxThrowUserException();
				}
				m_Incom.Consume(m_DecMac.GetBlockSize());
			}
			m_RecvPackSeq++;
			RecivePacket2(bp);
			break;
		}
	}
BREAKOUT:
	return;

 } CATCH(CUserException, e) {
	AfxMessageBox("SSH Packet Buffer Error In Recive");
 } END_CATCH
}
void Cssh::SendWindSize(int x, int y)
{
  TRY {
	CBuffer tmp;
	switch(m_SSHVer) {
	case 1:
		tmp.Put8Bit(SSH_CMSG_WINDOW_SIZE);
		tmp.Put32Bit(y);
		tmp.Put32Bit(x);
		tmp.Put32Bit(0);
		tmp.Put32Bit(0);
		SendPacket(&tmp);
		break;
	case 2:
		if ( m_StdChan == (-1) )
			break;
		tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
		tmp.Put32Bit(m_Chan[m_StdChan].m_RemoteID);
		tmp.PutStr("window-change");
		tmp.Put8Bit(0);
		tmp.Put32Bit(x);
		tmp.Put32Bit(y);
		tmp.Put32Bit(0);
		tmp.Put32Bit(0);
		SendPacket2(&tmp);
		break;
	}
 } CATCH(CUserException, e) {
	AfxMessageBox("SSH Packet Buffer Error In Recive");
 } END_CATCH
}
void Cssh::OnTimer(UINT_PTR nIDEvent)
{
	SendMsgKeepAlive();
}
void Cssh::OnRecvEmpty()
{
	if ( m_StdChan != (-1) )
		ChannelPolling(m_StdChan);
}
void Cssh::OnSendEmpty()
{
	for ( CFilter *fp = m_pListFilter ; fp != NULL ; fp = fp->m_pNext )
		fp->OnSendEmpty();
}

//////////////////////////////////////////////////////////////////////
// SSH1 Function
//////////////////////////////////////////////////////////////////////

void Cssh::SendDisconnect(LPCSTR str)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH_MSG_DISCONNECT);
	tmp.PutStr(str);
	SendPacket(&tmp);
}
void Cssh::SendPacket(CBuffer *bp)
{
	int len, pad, crc;
	CBuffer tmp, ext, dum;

	m_EncCmp.Compress(bp->GetPtr(), bp->GetSize(), &dum);
	bp = &dum;

	len = bp->GetSize() + 4;		// with CRC 4 Byte
	pad = 8 - (len % 8);

	tmp.Apend((LPBYTE)"\0\0\0\0\0\0\0\0", pad);
	tmp.Apend(bp->GetPtr(), bp->GetSize());
	crc = ssh_crc32(tmp.GetPtr(), tmp.GetSize());
	tmp.Put32Bit(crc);
	bp = &tmp;

	dum.Clear();
	m_EncCip.Cipher(bp->GetPtr(), bp->GetSize(), &dum);
	bp = &dum;

	ext.Put32Bit(len);
	ext.Apend(bp->GetPtr(), bp->GetSize());

	CExtSocket::Send(ext.GetPtr(), ext.GetSize());
}

int Cssh::SMsgPublicKey(CBuffer *bp)
{
	int n, i;
	RSA *host_key;
	RSA *server_key;
	CBuffer tmp;
	CIdKey skey;

	for ( n = 0 ; n < 8 ; n++ )
		m_Cookie[n] = bp->Get8Bit();

	server_key = RSA_new();
	server_key->n = BN_new();
	server_key->e = BN_new();
	n = bp->Get32Bit();
	bp->GetBIGNUM(server_key->e);
	bp->GetBIGNUM(server_key->n);

	host_key = RSA_new();
	host_key->n = BN_new();
	host_key->e = BN_new();
	n = bp->Get32Bit();
	bp->GetBIGNUM(host_key->e);
	bp->GetBIGNUM(host_key->n);

	skey.Create(IDKEY_RSA1);
	BN_copy(skey.m_Rsa->e, host_key->e);
	BN_copy(skey.m_Rsa->n, host_key->n);
	if ( !skey.HostVerify(m_HostName) )
		return FALSE;

	m_ServerFlag  = bp->Get32Bit();
	m_SupportCipher = bp->Get32Bit();
	m_SupportAuth = bp->Get32Bit();

	EVP_MD *evp_md = (EVP_MD *)EVP_md5();
	EVP_MD_CTX md;
    BYTE nbuf[2048];
	BYTE obuf[EVP_MAX_MD_SIZE];
	int len;
	BIGNUM *key;

    EVP_DigestInit(&md, evp_md);

	len = BN_num_bytes(host_key->n);
    BN_bn2bin(host_key->n, nbuf);
    EVP_DigestUpdate(&md, nbuf, len);

	len = BN_num_bytes(server_key->n);
    BN_bn2bin(server_key->n, nbuf);
    EVP_DigestUpdate(&md, nbuf, len);
    EVP_DigestUpdate(&md, m_Cookie, 8);
    EVP_DigestFinal(&md, obuf, NULL);
	memcpy(m_SessionId, obuf, 16);

	for ( n = 0 ; n < 32 ; n++ ) {
		if ( (n % 4) == 0 )
			i = rand();
		m_SessionKey[n] = (BYTE)(i);
		i >>= 8;
	}

	key = BN_new();
	BN_set_word(key, 0);
	for ( n  = 0; n < 32 ; n++ ) {
		BN_lshift(key, key, 8);
		if ( n < 16 )
			BN_add_word(key, m_SessionKey[n] ^ m_SessionId[n]);
		else
			BN_add_word(key, m_SessionKey[n]);
    }

	if ( BN_cmp(server_key->n, host_key->n) < 0 ) {
		rsa_public_encrypt(key, key, server_key);
		rsa_public_encrypt(key, key, host_key);
	} else {
		rsa_public_encrypt(key, key, host_key);
		rsa_public_encrypt(key, key, server_key);
	}

	RSA_free(server_key);
	RSA_free(host_key);

	m_CipherMode = 0;
	for ( n = 0 ; n < m_CipTabMax ; n++ ) {
		if ( (m_SupportCipher & (1 << m_CipTab[n])) != 0 ) {
			m_CipherMode = m_CipTab[n];
			break;
		}
	}
	if ( m_CipherMode == 0 )
		return FALSE;

	tmp.Put8Bit(SSH_CMSG_SESSION_KEY);
	tmp.Put8Bit(m_CipherMode);
    for ( n = 0 ; n < 8 ; n++ )
		tmp.Put8Bit(m_Cookie[n]);
	tmp.PutBIGNUM(key);
	tmp.Put32Bit(SSH_PROTOFLAG_SCREEN_NUMBER | SSH_PROTOFLAG_HOST_IN_FWD_OPEN);
	SendPacket(&tmp);

	BN_clear_free(key);

	m_EncCip.Init(m_EncCip.GetName(m_CipherMode), MODE_ENC, m_SessionKey, 32);
	m_DecCip.Init(m_DecCip.GetName(m_CipherMode), MODE_DEC, m_SessionKey, 32);

	return TRUE;
}
int Cssh::SMsgAuthRsaChallenge(CBuffer *bp)
{
	int n;
	int len;
	CBuffer tmp;
	BIGNUM *challenge;
	MD5_CTX md;
	BYTE buf[32], res[16];
	CSpace inbuf, otbuf;

	while ( m_IdKey.m_Type == IDKEY_DSA2 ) {
		if ( !SetIdKeyList() )
			return FALSE;
	}

	if ( m_IdKey.m_Pass.Compare(m_pDocument->m_ServerEntry.m_PassName) != 0 )
		return FALSE;

	if ( (challenge = BN_new()) == NULL )
		return FALSE;

	bp->GetBIGNUM(challenge);
	inbuf.SetSize(BN_num_bytes(challenge));
	BN_bn2bin(challenge, inbuf.GetPtr());

	otbuf.SetSize(BN_num_bytes(m_IdKey.m_Rsa->n));

	if ( (len = RSA_private_decrypt(inbuf.GetSize(), inbuf.GetPtr(), otbuf.GetPtr(), m_IdKey.m_Rsa, RSA_PKCS1_PADDING)) <= 0 ) {
		BN_free(challenge);
		return FALSE;
	}

	BN_bin2bn(otbuf.GetPtr(), len, challenge);

	if ( (len = BN_num_bytes(challenge)) <= 0 || len > 32 ) {
		BN_free(challenge);
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	BN_bn2bin(challenge, buf + sizeof(buf) - len);
	MD5_Init(&md);
	MD5_Update(&md, buf, 32);
	MD5_Update(&md, m_SessionId, 16);
	MD5_Final(res, &md);	

	tmp.Put8Bit(SSH_CMSG_AUTH_RSA_RESPONSE);
	for ( n = 0 ; n < 16 ; n++ )
		tmp.Put8Bit(res[n]);
	SendPacket(&tmp);

	BN_free(challenge);
	return TRUE;
}
void Cssh::RecivePacket(CBuffer *bp)
{
	int n;
	CString str;
	CBuffer tmp;
	int type = bp->Get8Bit();

	if ( type == SSH_MSG_DISCONNECT ) {
		bp->GetStr(str);
		CExtSocket::OnReciveCallBack((void *)((LPCSTR)str), str.GetLength(), 0);
		Close();
		return;
	} else if ( type == SSH_MSG_DEBUG ) {
		bp->GetStr(str);
		CExtSocket::OnReciveCallBack((void *)((LPCSTR)str), str.GetLength(), 0);
		return;
	}

	switch(m_PacketStat) {
	case 0:		// Session Key Excheng
		if ( type != SSH_SMSG_PUBLIC_KEY )
			goto DISCONNECT;
		if ( !SMsgPublicKey(bp) )
			goto DISCONNECT;
		m_PacketStat = 1;
		break;
	case 1:		// Session Key Exc Success & Send Login User
		if ( type != SSH_SMSG_SUCCESS )
			goto DISCONNECT;
		tmp.Put8Bit(SSH_CMSG_USER);
		tmp.PutStr(m_pDocument->m_ServerEntry.m_UserName);
		SendPacket(&tmp);
		m_PacketStat = 2;
		break;
	case 2:		// Login User Status
		m_PacketStat = 5;
		if ( type == SSH_SMSG_SUCCESS )
			break;
		if ( type != SSH_SMSG_FAILURE )
			goto DISCONNECT;
		if ( (m_SupportAuth & (1 << SSH_AUTH_RSA)) != 0 && m_IdKey.m_Rsa != NULL ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_RSA);
			tmp.PutBIGNUM(m_IdKey.m_Rsa->n);
			SendPacket(&tmp);
			m_PacketStat = 10;
			break;
		}
	GO_AUTH_TIS:
		if ( (m_SupportAuth & (1 << SSH_AUTH_TIS)) != 0 ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_TIS);
			SendPacket(&tmp);
			m_PacketStat = 3;
			break;
		}
	GO_AUTH_PASS:
		if ( (m_SupportAuth & (1 << SSH_AUTH_PASSWORD)) != 0 ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_PASSWORD);
			tmp.PutStr(m_pDocument->m_ServerEntry.m_PassName);
			SendPacket(&tmp);
			m_PacketStat = 4;
			break;
		}
		goto DISCONNECT;
	case 10:
		if ( type != SSH_SMSG_AUTH_RSA_CHALLENGE )
			goto GO_AUTH_TIS;
		if ( !SMsgAuthRsaChallenge(bp) )
			goto DISCONNECT;
		m_PacketStat = 11;
		break;
	case 3:		// TIS Auth
		if ( type != SSH_SMSG_AUTH_TIS_CHALLENGE )
			goto GO_AUTH_PASS;
		tmp.Put8Bit(SSH_CMSG_AUTH_TIS_RESPONSE);
		tmp.PutStr(m_pDocument->m_ServerEntry.m_PassName);
		SendPacket(&tmp);
		m_PacketStat = 4;
		break;
	case 11:
		if ( type != SSH_SMSG_SUCCESS )
			goto GO_AUTH_TIS;
	case 4:		// Password Auth Success
		if ( type != SSH_SMSG_SUCCESS )
			goto DISCONNECT;
		if ( m_CompMode == FALSE )
			goto NOCOMPRESS;
		tmp.Put8Bit(SSH_CMSG_REQUEST_COMPRESSION);
		tmp.Put32Bit(COMPLEVEL);
		SendPacket(&tmp);
		m_PacketStat = 5;
		break;
	case 5:		// Compression ?
		if ( type == SSH_SMSG_SUCCESS ) {
			m_EncCmp.Init("zlib", MODE_ENC, COMPLEVEL);
			m_DecCmp.Init("zlib", MODE_DEC, COMPLEVEL);
		} else if ( type == SSH_SMSG_FAILURE ) {
			m_EncCmp.Init("node", MODE_ENC, COMPLEVEL);
			m_DecCmp.Init("none", MODE_DEC, COMPLEVEL);
		} else
			goto DISCONNECT;
	NOCOMPRESS:
		tmp.Put8Bit(SSH_CMSG_REQUEST_PTY);
		tmp.PutStr(m_pDocument->m_ServerEntry.m_TermName);
		tmp.Put32Bit(m_pDocument->m_TextRam.m_Lines);
		tmp.Put32Bit(m_pDocument->m_TextRam.m_Cols);
		tmp.Put32Bit(0);
		tmp.Put32Bit(0);
		tmp.Put8Bit(0);		// TTY_OP_END
		SendPacket(&tmp);
		m_PacketStat = 6;
		break;
	case 6:		// TERM name & WinSz Success
		if ( type != SSH_SMSG_SUCCESS )
			goto DISCONNECT;
		tmp.Put8Bit(SSH_CMSG_EXEC_SHELL);
		SendPacket(&tmp);
		m_PacketStat = 7;

		str.Format("%s+%s", m_EncCmp.GetTitle(), m_EncCip.GetTitle());
		m_pDocument->SetStatus(str);
		break;

	case 7:		// Client loop
		switch(type) {
		case SSH_MSG_CHANNEL_DATA:
		case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
		case SSH_MSG_PORT_OPEN:
		case SSH_SMSG_AGENT_OPEN:
		case SSH_SMSG_X11_OPEN:
		case SSH_MSG_CHANNEL_CLOSE:
		case SSH_MSG_CHANNEL_CLOSE_CONFIRMATION:
			tmp.Put8Bit(SSH_SMSG_FAILURE);
			SendPacket(&tmp);
			break;

		case SSH_MSG_IGNORE:
			break;
		case SSH_SMSG_EXITSTATUS:
			SendDisconnect("Recv Exit Status");
			break;
		case SSH_SMSG_STDERR_DATA:
		case SSH_SMSG_STDOUT_DATA:
			n = bp->Get32Bit();
			CExtSocket::OnReciveCallBack(bp->GetPtr(), n, 0);
			break;

		default:
			SendDisconnect("Unkonw Type");
			break;
		}
		break;

	DISCONNECT:
		m_PacketStat = 99;
		SendDisconnect("What Type ?");
	case 99:
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// SSH2 Function
//////////////////////////////////////////////////////////////////////

void Cssh::LogIt(LPCSTR format, ...)
{
    va_list args;
	CString str;

	va_start(args, format);
	str.FormatV(format, args);
	va_end(args);

	if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) ) {
		GetMainWnd()->SetMessageText(str);
		return;
	}

	CString tmp;
	CTime now = CTime::GetCurrentTime();
	DWORD size;
	TCHAR buf[MAX_COMPUTERNAME_LENGTH + 2];

	tmp = now.Format("%b %d %H:%M:%S ");
	size = MAX_COMPUTERNAME_LENGTH;
	if ( GetComputerName(buf, &size) )
		tmp += buf;
	tmp += " : ";
	tmp += str;
	tmp += "\r\n";

	CExtSocket::OnReciveCallBack((void *)(LPCSTR)tmp, tmp.GetLength(), 0);
}
int Cssh::ChannelOpen()
{
	int n, mx, id;

	if ( (id = m_ChanFree) == (-1) ) {
		for ( mx = 0 ; (n = m_ChanFree2) != (-1) ; mx++ ) {
			m_ChanFree2 = m_Chan[n].m_Next;
			m_Chan[n].m_Next = m_ChanFree;
			m_ChanFree = n;
		}
		if ( mx < 5 ) {
			n = (int)m_Chan.GetSize();
			mx = n + (10 - mx);
			m_Chan.SetSize(mx);
			for ( ; n < mx ; n++ ) {
				m_Chan[n].m_Next = m_ChanFree;
				m_ChanFree = n;
			}
		}
		id = m_ChanFree;
	}

	m_ChanFree = m_Chan[id].m_Next;
	m_Chan[id].m_Next = m_ChanNext;
	m_ChanNext = id;

	m_Chan[id].m_pDocument  = m_pDocument;
	m_Chan[id].m_pSsh       = this;

	m_Chan[id].m_Status     = 0;
	m_Chan[id].m_TypeName   = "";
	m_Chan[id].m_RemoteID   = (-1);
	m_Chan[id].m_LocalID    = id;
	m_Chan[id].m_RemoteWind = 0;
	m_Chan[id].m_RemoteMax  = 0;
	m_Chan[id].m_LocalComs  = 0;
	m_Chan[id].m_LocalWind  = CHAN_SES_WINDOW_DEFAULT;
	m_Chan[id].m_LocalPacks = CHAN_SES_PACKET_DEFAULT;
	m_Chan[id].m_Eof        = 0;
	m_Chan[id].m_Input.Clear();
	m_Chan[id].m_Output.Clear();

	m_Chan[id].m_lHost      = "";
	m_Chan[id].m_lPort      = 0;
	m_Chan[id].m_rHost      = "";
	m_Chan[id].m_rPort      = 0;
	m_Chan[id].m_ReadByte   = 0;
	m_Chan[id].m_WriteByte  = 0;
	m_Chan[id].m_ConnectTime= 0;

	((CRLoginApp *)AfxGetApp())->SetSocketIdle(&(m_Chan[id]));

	return id;
}
void Cssh::ChannelClose(int id)
{
	int n, nx;

	for ( n = nx = m_ChanNext ; n != (-1) ; ) {
		if ( n == id ) {
			if ( n == nx )
				m_ChanNext = m_Chan[n].m_Next;
			else
				m_Chan[nx].m_Next = m_Chan[n].m_Next;
			m_Chan[n].m_Next = m_ChanFree2;
			m_ChanFree2 = n;
			m_Chan[id].Close();
			break;
		}
		nx = n;
		n = m_Chan[n].m_Next;
	}

	if ( id == m_StdChan ) {
		m_StdChan = (-1);
		m_SSH2Status &= ~SSH2_STAT_HAVESTDIO;
	}

	if ( m_ChanNext == (-1) )
		SendDisconnect2(SSH2_DISCONNECT_CONNECTION_LOST, "End");
}
void Cssh::ChannelCheck(int n)
{
	CBuffer tmp;

	if ( !CHAN_OK(n) ) {
		if ( (m_Chan[n].m_Status & CHAN_LISTEN) && (m_Chan[n].m_Eof & CEOF_DEAD) ) {
			ChannelClose(n);
			LogIt("Listen Closed #%d %s:%d -> %s:%d", n, m_Chan[n].m_lHost, m_Chan[n].m_lPort, m_Chan[n].m_rHost, m_Chan[n].m_rPort);
		}
		return;
	}

	m_Chan[n].m_Eof &= ~(CEOF_IEMPY | CEOF_OEMPY);
	if ( m_Chan[n].GetSendSize() == 0 )
		m_Chan[n].m_Eof |= CEOF_IEMPY;
	if ( m_Chan[n].GetRecvSize() == 0 )
		m_Chan[n].m_Eof |= CEOF_OEMPY;

	if ( !(m_Chan[n].m_Eof & CEOF_SEOF) && !(m_Chan[n].m_Eof & CEOF_SCLOSE) ) {
		if ( (m_Chan[n].m_Eof & CEOF_ICLOSE) || ((m_Chan[n].m_Eof & CEOF_DEAD) && (m_Chan[n].m_Eof & CEOF_OEMPY)) ) {
			m_Chan[n].m_Output.Clear();
			m_Chan[n].m_Eof |= (CEOF_SEOF | CEOF_OEMPY);
			tmp.Clear();
			tmp.Put8Bit(SSH2_MSG_CHANNEL_EOF);
			tmp.Put32Bit(m_Chan[n].m_RemoteID);
			SendPacket2(&tmp);
			TRACE("Cannel #%d Send Eof\n", m_Chan[n].m_LocalID);
		}
	}

	if ( !(m_Chan[n].m_Eof & CEOF_SCLOSE) ) {
		if ( (m_Chan[n].m_Eof & CEOF_DEAD) || ((m_Chan[n].m_Eof & CEOF_IEOF) && (m_Chan[n].m_Eof & CEOF_IEMPY)) ) {
			m_Chan[n].m_Input.Clear();
			m_Chan[n].m_Eof |= (CEOF_SCLOSE | CEOF_IEMPY);
			tmp.Clear();
			tmp.Put8Bit(SSH2_MSG_CHANNEL_CLOSE);
			tmp.Put32Bit(m_Chan[n].m_RemoteID);
			SendPacket2(&tmp);
			TRACE("Cannel #%d Send Close\n", m_Chan[n].m_LocalID);
		}
	}

	if ( !(m_Chan[n].m_Eof & CEOF_ICLOSE) && !(m_Chan[n].m_Eof & CEOF_OEMPY) && m_Chan[n].m_Output.GetSize() > 0 )
		SendMsgChannelData(n);

	if ( (m_Chan[n].m_Eof & CEOF_ICLOSE) && (m_Chan[n].m_Eof & CEOF_SCLOSE) ) {
		if ( m_Chan[n].m_pFilter != NULL )
			LogIt("Closed #%d Filter", n);
		else
			LogIt("Closed #%d %s:%d -> %s:%d", n, m_Chan[n].m_lHost, m_Chan[n].m_lPort, m_Chan[n].m_rHost, m_Chan[n].m_rPort);
		m_OpenChanCount--;
		ChannelClose(n);
		TRACE("Cannel #%d Close\n", m_Chan[n].m_LocalID);
	}
}
void Cssh::ChannelAccept(int id, SOCKET hand)
{
	int i = ChannelOpen();
	CString host;
	int port;

	if ( !m_Chan[id].Accept(&(m_Chan[i]), hand) ) {
		ChannelClose(i);
		return;
	}

	CExtSocket::GetPeerName((int)m_Chan[i].m_Fd, host, &port);

	if ( (m_Chan[id].m_Status & CHAN_PROXY_SOCKS) != 0 ) {
		m_Chan[i].m_Status = CHAN_PROXY_SOCKS;
		m_Chan[i].m_lHost = host;
		m_Chan[i].m_lPort = port;
	} else {
		SendMsgChannelOpen(i, "direct-tcpip", m_Chan[id].m_rHost, m_Chan[id].m_rPort, host, port);
	}
}
void Cssh::ChannelPolling(int id)
{
	int n;
	CBuffer tmp;

	if ( (n = m_Chan[id].m_LocalComs - m_Chan[id].GetSendSize()) >= (m_Chan[id].m_LocalWind / 2) ) {
		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_WINDOW_ADJUST);
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.Put32Bit(n);
		SendPacket2(&tmp);
		m_Chan[id].m_LocalComs -= n;
		TRACE("Send Window Adjust %d\n", n);
	}

	if ( m_Chan[id].GetSendSize() <= 0 )
		ChannelCheck(id);
}
void Cssh::DecodeProxySocks(int id)
{
	int n;
	int len;
	DWORD dw;
	LPBYTE p;
	CString host[2];
	int port[2];
	struct {
		BYTE version;
		BYTE command;
		WORD dest_port;
		union {
			struct in_addr	in_addr;
			DWORD			dw_addr;
		} dest;
	} s4_req;
	BYTE tmp[257];

	if ( (len = m_Chan[id].m_Output.GetSize()) < 2 )
		return;

	p = m_Chan[id].m_Output.GetPtr();
	switch(p[0]) {
	case 4:
		if ( len < sizeof(s4_req) )
			break;
		for ( n = sizeof(s4_req) ; n < len ; n++ ) {
			if ( p[n] == '\0' )
				break;
		}
		if ( n++ >= len )
			break;
		s4_req.version   = m_Chan[id].m_Output.Get8Bit();
		s4_req.command   = m_Chan[id].m_Output.Get8Bit();
		s4_req.dest_port = *((WORD *)(m_Chan[id].m_Output.GetPtr())); m_Chan[id].m_Output.Consume(2);
		s4_req.dest.dw_addr = *((DWORD *)(m_Chan[id].m_Output.GetPtr())); m_Chan[id].m_Output.Consume(4);
		m_Chan[id].m_Output.Consume( n - sizeof(s4_req));

		m_Chan[id].m_Input.Put8Bit(0);
		m_Chan[id].m_Input.Put8Bit(90);
		m_Chan[id].m_Input.Put16Bit(0);
		m_Chan[id].m_Input.Put32Bit(INADDR_ANY);
		m_Chan[id].Send(m_Chan[id].m_Input.GetPtr(), m_Chan[id].m_Input.GetSize());
		m_Chan[id].m_Input.Clear();

		host[0] = inet_ntoa(s4_req.dest.in_addr);
		port[0] = ntohs(s4_req.dest_port);
		host[1] = m_Chan[id].m_lHost;
		port[1] = m_Chan[id].m_lPort;

		SendMsgChannelOpen(id, "direct-tcpip", host[0], port[0], host[1], port[1]);
		break;

	case 5:
		if ( (m_Chan[id].m_Status & CHAN_SOCKS5_AUTH) == 0 ) {
			if ( len < (p[1] + 2) )
				break;
			for ( n = 2 ; n < (p[1] + 2) ; n++ ) {
				if ( p[n] == '\0' )		// SOCKS5_NOAUTH
					break;
			}
			if ( n >= (p[1] + 2) ) {
				ChannelClose(id);
				break;
			}
			m_Chan[id].m_Output.Consume(p[1] + 2);
			len = m_Chan[id].m_Output.GetSize();

			m_Chan[id].m_Input.Put8Bit(5);
			m_Chan[id].m_Input.Put8Bit(0);
			m_Chan[id].Send(m_Chan[id].m_Input.GetPtr(), m_Chan[id].m_Input.GetSize());
			m_Chan[id].m_Input.Clear();

			m_Chan[id].m_Status |= CHAN_SOCKS5_AUTH;
		}

		if ( len < 4 )
			break;
		if ( p[1] != '\x01' || p[2] != '\0' ) {	// SOCKS5_CONNECT  ?
			ChannelClose(id);
			break;
		}
		if ( p[3] == '\x01' ) {			// SOCKS5_IPV4
			if ( len < (4 + 4 + 2) )
				break;
			memcpy(tmp, p + 4, 4);
			m_Chan[id].m_Output.Consume(4 + 4);
			m_Chan[id].GetHostName(AF_INET, tmp, host[0]);
		} else if ( p[3] == '\x03' ) {	// SOCKS5_DOMAIN
			if ( len < (4 + p[4] + 2) )
				break;
			memcpy(tmp, p + 5, p[4]);
			m_Chan[id].m_Output.Consume(5 + p[4]);
			tmp[p[4]] = '\0';
			host[0] = tmp;
#ifndef	NOIPV6
		} else if ( p[3] == '\x04' ) {	// SOCKS5_IPV6
			if ( len < (4 + 16 + 2) )
				break;
			memcpy(tmp, p + 4, 16);
			m_Chan[id].m_Output.Consume(4 + 16);
			m_Chan[id].GetHostName(AF_INET6, tmp, host[0]);
#endif
		} else {
			ChannelClose(id);
			break;
		}

		dw = *((WORD *)(m_Chan[id].m_Output.GetPtr()));
		m_Chan[id].m_Output.Consume(2);

		m_Chan[id].m_Input.Put8Bit(0x05);
		m_Chan[id].m_Input.Put8Bit(0x00);	// SOCKS5_SUCCESS
		m_Chan[id].m_Input.Put8Bit(0x00);
		m_Chan[id].m_Input.Put8Bit(0x01);	// SOCKS5_IPV4
		m_Chan[id].m_Input.Put32Bit(INADDR_ANY);
		m_Chan[id].m_Input.Put16Bit(dw);
		m_Chan[id].Send(m_Chan[id].m_Input.GetPtr(), m_Chan[id].m_Input.GetSize());
		m_Chan[id].m_Input.Clear();

		port[0] = ntohs((WORD)dw);
		host[1] = m_Chan[id].m_lHost;
		port[1] = m_Chan[id].m_lPort;

		SendMsgChannelOpen(id, "direct-tcpip", host[0], port[0], host[1], port[1]);
		break;

	default:
		ChannelClose(id);
		break;
	}
}
int Cssh::MatchList(LPCSTR client, LPCSTR server, CString &str)
{
	int n;
	CString tmp;
	CStringArray array;

	array.RemoveAll();
	while ( *server != '\0' ) {
		while ( *server != '\0' && strchr(" ,", *server) != NULL )
			server++;
		tmp = "";
		while ( *server != '\0' && strchr(" ,", *server) == NULL )
			tmp += *(server++);
		if ( !tmp.IsEmpty() )
			array.Add(tmp);
	}

	while ( *client != '\0' ) {
		while ( *client != '\0' && strchr(" ,", *client) != NULL )
			client++;
		tmp = "";
		while ( *client != '\0' && strchr(" ,", *client) == NULL )
			tmp += *(client++);
		if ( !tmp.IsEmpty() ) {
			for ( n = 0 ; n < array.GetSize() ; n++ ) {
				if ( tmp.Compare(array[n]) == 0 ) {
					str = tmp;
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}
void Cssh::PortForward()
{
	int n, i, a = 0;
	CString str;
	CStringArrayExt tmp;

	for ( i = 0 ; i < m_pDocument->m_ParamTab.m_PortFwd.GetSize() ; i++ ) {
		m_pDocument->m_ParamTab.m_PortFwd.GetArray(i, tmp);
		if ( tmp.GetSize() < 4 )
			continue;

		if ( tmp[0].Compare("localhost") == 0 ) { //&& tmp[2].Compare(m_HostName) == 0 ) {
			n = ChannelOpen();
			m_Chan[n].m_Status = CHAN_LISTEN;
			if ( !m_Chan[n].Create(tmp[0], GetPortNum(tmp[1]), tmp[2], GetPortNum(tmp[3])) ) {
				str.Format("Port Forward Error %s:%s->%s:%s", tmp[0], tmp[1], tmp[2], tmp[3]);
				AfxMessageBox(str);
				ChannelClose(n);
			} else {
				LogIt("Local Listen %s:%s", tmp[0], tmp[1]);
				a++;
			}
		} else if ( tmp[0].Compare("socks") == 0 && tmp[2].Compare(m_HostName) == 0 ) {
			n = ChannelOpen();
			m_Chan[n].m_Status = CHAN_LISTEN | CHAN_PROXY_SOCKS;
			tmp[0] = "localhost";
			if ( !m_Chan[n].Create(tmp[0], GetPortNum(tmp[1]), tmp[2], GetPortNum(tmp[3])) ) {
				str.Format("Socks Listen Error %s:%s->%s:%s", tmp[0], tmp[1], tmp[2], tmp[3]);
				AfxMessageBox(str);
				ChannelClose(n);
			} else {
				LogIt("Local Socks %s:%s", tmp[0], tmp[1]);
				a++;
			}
		} else if ( tmp[0].Compare(m_HostName) == 0 ) { // && tmp[2].Compare("localhost") == 0 ) {
			n = (int)m_Permit.GetSize();
			m_Permit.SetSize(n + 1);
			m_Permit[n].m_lHost = tmp[2];
			m_Permit[n].m_lPort = GetPortNum(tmp[3]);
			m_Permit[n].m_rHost = tmp[0];
			m_Permit[n].m_rPort = GetPortNum(tmp[1]);
			SendMsgGlobalRequest(n, "tcpip-forward", "localhost", GetPortNum(tmp[1]));
			LogIt("Remote Listen %s:%s", tmp[0], tmp[1]);
			a++;
		}
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) && a == 0 )
		AfxMessageBox(_T("ポートフォワードの設定が無効です"));
}

void Cssh::OpenSFtpChannel()
{
	int id = SendMsgChannelOpen((-1), "session");
	CSFtpFilter *pFilter = new CSFtpFilter;
	m_Chan[id].m_pFilter = pFilter;
	pFilter->m_pSFtp = new CSFtp(NULL);
	pFilter->m_pSFtp->m_pSSh = this;
	pFilter->m_pSFtp->m_pChan = &(m_Chan[id]);
	pFilter->m_pSFtp->m_HostKanjiSet = m_pDocument->m_TextRam.m_SendCharSet[m_pDocument->m_TextRam.m_KanjiMode];
	pFilter->m_pSFtp->Create(IDD_SFTPDLG, CWnd::GetDesktopWindow());
	pFilter->m_pSFtp->ShowWindow(SW_SHOW);
}
void Cssh::OpenRcpUpload(LPCSTR file)
{
	if ( file != NULL ) {
		m_RcpCmd.m_pSsh = this;
		m_RcpCmd.m_FileList.AddTail(file);
		m_RcpCmd.Destroy();
	} else {
		m_RcpCmd.m_pSsh = this;
		int id = SendMsgChannelOpen((-1), "session");
		m_Chan[id].m_pFilter = &m_RcpCmd;
		m_RcpCmd.m_pChan = &(m_Chan[id]);
		m_RcpCmd.m_pNext = m_pListFilter;
		m_pListFilter = &m_RcpCmd;
	}
}

void Cssh::SendMsgKexDhInit()
{
	CBuffer tmp;

	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);
	m_SaveDh = (m_DhMode == DHMODE_GROUP_1 ? dh_new_group1() : dh_new_group14());
	dh_gen_key(m_SaveDh, m_NeedKeyLen * 8);

	tmp.Put8Bit(SSH2_MSG_KEXDH_INIT);
	tmp.PutBIGNUM2(m_SaveDh->pub_key);
	SendPacket2(&tmp);
}
void Cssh::SendMsgKexDhGexRequest()
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_KEX_DH_GEX_REQUEST);
	tmp.Put32Bit(1024);
	tmp.Put32Bit(dh_estimate(m_NeedKeyLen * 8));
	tmp.Put32Bit(8192);
	SendPacket2(&tmp);
}
void Cssh::SendMsgNewKeys()
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_NEWKEYS);
	SendPacket2(&tmp);

	if ( m_EncCip.Init(m_VProp[0], MODE_ENC, m_VKey[2], (-1), m_VKey[0]) ||
		 m_EncMac.Init(m_VProp[2], m_VKey[4], (-1)) ||
		 m_EncCmp.Init(m_VProp[4], MODE_ENC, 6) )
		Close();
}
void Cssh::SendMsgServiceRequest(LPCSTR str)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_SERVICE_REQUEST);
	tmp.PutStr(str);
	SendPacket2(&tmp);
}
int Cssh::SendMsgUserAuthRequest(LPCSTR str)
{
	int skip;
	CBuffer tmp, blob, sig;

	tmp.PutBuf(m_SessionId, m_SessionIdLen);
	skip = tmp.GetSize();
	tmp.Put8Bit(SSH2_MSG_USERAUTH_REQUEST);
	tmp.PutStr(m_pDocument->m_ServerEntry.m_UserName);
	tmp.PutStr("ssh-connection");

	if ( str == NULL )
		m_AuthStat = 0;

	switch(m_AuthStat) {
	case 0:
		tmp.PutStr("none");
		tmp.Consume(skip);
		SendPacket2(&tmp);
		m_AuthStat = 2;
		return TRUE;
	case 1:
	RETRY:
		if ( !SetIdKeyList() )
			m_AuthStat = 3;
	case 2:
		if ( m_IdKey.m_Type != IDKEY_NONE && str != NULL && strstr(str, "publickey") != NULL ) {
			if ( m_IdKey.m_Pass.Compare(m_pDocument->m_ServerEntry.m_PassName) != 0 )
				goto RETRY;
			if ( m_IdKey.m_Type == IDKEY_RSA1 )
				m_IdKey.m_Type = IDKEY_RSA2;
			m_IdKey.SetBlob(&blob);
			m_AuthStat = 1;
			break;
		}
	case 3:
		if ( !m_pDocument->m_ServerEntry.m_PassName.IsEmpty() && str != NULL && strstr(str, "password") != NULL ) {
			tmp.PutStr("password");
			tmp.Put8Bit(0);
			tmp.PutStr(m_pDocument->m_ServerEntry.m_PassName);
			tmp.Consume(skip);
			SendPacket2(&tmp);
			m_AuthStat = 4;
			return TRUE;
		}
	case 4:
		if ( str != NULL && strstr(str, "keyboard-interactive") != NULL ) {
			tmp.PutStr("keyboard-interactive");
			tmp.PutStr("");		// LANG
			tmp.PutStr("");		// DEV
			tmp.Consume(skip);
			SendPacket2(&tmp);
			m_AuthStat = 5;
			return TRUE;
		}
	default:
		m_AuthStat = 5;
		return FALSE;
	}

	tmp.PutStr("publickey");
	tmp.Put8Bit(1);
	tmp.PutStr(m_IdKey.GetName());
	tmp.PutBuf(blob.GetPtr(), blob.GetSize());

	if ( !m_IdKey.Sign(&sig, tmp.GetPtr(), tmp.GetSize()) )
		return FALSE;

	tmp.PutBuf(sig.GetPtr(), sig.GetSize());
	tmp.Consume(skip);
	SendPacket2(&tmp);

	return TRUE;
}
int Cssh::SendMsgChannelOpen(int n, LPCSTR type, LPCSTR lhost, int lport, LPCSTR rhost, int rport)
{
	CBuffer tmp;

	if ( n < 0 )
		n = ChannelOpen();

	m_Chan[n].m_Status     = CHAN_OPEN_LOCAL;
	m_Chan[n].m_TypeName   = type;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN);
	tmp.PutStr(type);
	tmp.Put32Bit(n);
	tmp.Put32Bit(m_Chan[n].m_LocalWind);
	tmp.Put32Bit(m_Chan[n].m_LocalPacks);
	if ( lhost != NULL ) {
		tmp.PutStr(lhost);
		tmp.Put32Bit(lport);
		tmp.PutStr(rhost);
		tmp.Put32Bit(rport);
		m_Chan[n].m_lHost = lhost;
		m_Chan[n].m_lPort = lport;
		m_Chan[n].m_rHost = rhost;
		m_Chan[n].m_rPort = rport;
	}
	SendPacket2(&tmp);

	TRACE("Channel Open %d(%d)\n", m_Chan[n].m_LocalWind, m_Chan[n].m_LocalPacks);

	return n;
}
void Cssh::SendMsgChannelOpenConfirmation(int id)
{
	CBuffer tmp;

	m_Chan[id].m_Status |= CHAN_OPEN_LOCAL;
	m_Chan[id].m_ConnectTime = CTime::GetCurrentTime();
	m_OpenChanCount++;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_CONFIRMATION);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.Put32Bit(m_Chan[id].m_LocalID);
	tmp.Put32Bit(m_Chan[id].m_LocalWind);
	tmp.Put32Bit(m_Chan[id].m_LocalPacks);
	SendPacket2(&tmp);

	TRACE("Channel OpenConf %d(%d)\n", m_Chan[id].m_LocalWind, m_Chan[id].m_LocalPacks);

	if ( m_Chan[id].m_pFilter != NULL )
		LogIt("Open Filter #%d", id);
	else
		LogIt("Open Connect #%d %s:%d -> %s:%d", id, m_Chan[id].m_lHost, m_Chan[id].m_lPort, m_Chan[id].m_rHost, m_Chan[id].m_rPort);
}
void Cssh::SendMsgChannelOpenFailure(int id)
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_FAILURE);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.Put32Bit(SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED);
	tmp.PutStr("open failed");
	tmp.PutStr("");
	SendPacket2(&tmp);

	ChannelClose(id);
}
void Cssh::SendMsgChannelData(int id)
{
	int len;
	CBuffer tmp(CHAN_SES_PACKET_DEFAULT + 16);

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return;

	while ( (len = m_Chan[id].m_Output.GetSize()) > 0 ) {
		if ( len > m_Chan[id].m_RemoteWind )
			len = m_Chan[id].m_RemoteWind;

		if ( len > m_Chan[id].m_RemoteMax )
			len = m_Chan[id].m_RemoteMax;

		if ( len <= 0 )
			return;

		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_DATA);
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.PutBuf(m_Chan[id].m_Output.GetPtr(), len);
		m_Chan[id].m_Output.Consume(len);
		m_Chan[id].m_RemoteWind -= len;
		SendPacket2(&tmp);
	}

	ChannelCheck(id);
}
void Cssh::SendMsgChannelRequesstShell(int id)
{
	int n;
	char *p;
	CBuffer tmp;
	CString str;
	CStringEnv env;

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHAGENT) ) {
		tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.PutStr("auth-agent-req@openssh.com");
		tmp.Put8Bit(0);
		SendPacket2(&tmp);
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHX11PF) ) {
		if ( (p = getenv("DISPLAY")) == NULL ) {
			if ( m_pDocument->m_ParamTab.m_XDisplay.IsEmpty() )
				p = ":0";
			else
				p = (char *)(LPCSTR)m_pDocument->m_ParamTab.m_XDisplay;
		}

		while ( *p != '\0' && *p != ':' )
			str += *(p++);

		if ( str.IsEmpty() || str.CompareNoCase("unix") == 0 )
			str = "127.0.0.1";

		if ( *(p++) == ':' && CExtSocket::SokcetCheck(str, 6000 + atoi(p)) ) {
			str.Format("%04x%04x%04x%04x", rand(), rand(), rand(), rand());
			tmp.Clear();
			tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
			tmp.Put32Bit(m_Chan[id].m_RemoteID);
			tmp.PutStr("x11-req");
			tmp.Put8Bit(0);
			tmp.Put8Bit(0);		// XXX bool single connection
			tmp.PutStr("MIT-MAGIC-COOKIE-1");
			tmp.PutStr(str);
			tmp.Put32Bit(atoi(p));
			SendPacket2(&tmp);
		}
	}

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("pty-req");
	tmp.Put8Bit(1);
	tmp.PutStr(m_pDocument->m_ServerEntry.m_TermName);
	tmp.Put32Bit(m_pDocument->m_TextRam.m_Cols);
	tmp.Put32Bit(m_pDocument->m_TextRam.m_Lines);
	tmp.Put32Bit(0);
	tmp.Put32Bit(0);
	tmp.Put32Bit(0);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(0);

	env.GetString(m_pDocument->m_ParamTab.m_ExtEnvStr);
	for ( n = 0 ; n < env.GetSize() ; n++ ) {
		if ( env[n].m_Value == 0 || env[n].m_nIndex.IsEmpty() || env[n].m_String.IsEmpty() )
			continue;
		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.PutStr("env");
		tmp.Put8Bit(0);
		tmp.PutStr(env[n].m_nIndex);
		tmp.PutStr(env[n].m_String);
		SendPacket2(&tmp);
	}

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("shell");
	tmp.Put8Bit(1);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(1);
}
void Cssh::SendMsgChannelRequesstSubSystem(int id)
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("subsystem");
	tmp.Put8Bit(1);
	tmp.PutStr("sftp");
	SendPacket2(&tmp);
	m_ChnReqMap.Add(2);
}
void Cssh::SendMsgChannelRequesstExec(int id)
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("exec");
	tmp.Put8Bit(1);
	tmp.PutStr(m_Chan[id].m_pFilter->m_Cmd);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(3);
}
void Cssh::SendMsgGlobalRequest(int num, LPCSTR str, LPCSTR rhost, int rport)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_GLOBAL_REQUEST);
	tmp.PutStr(str);
	tmp.Put8Bit(1);
	if ( rhost != NULL ) {
		tmp.PutStr(rhost);
		tmp.Put32Bit(rport);
	}
	SendPacket2(&tmp);
	m_GlbReqMap.Add((WORD)num);
}
void Cssh::SendMsgKeepAlive()
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_GLOBAL_REQUEST);
	tmp.PutStr("keepalive@openssh.com");
	tmp.Put8Bit(1);
	SendPacket2(&tmp);
	m_GlbReqMap.Add(0xFFFF);
}
void Cssh::SendMsgUnimplemented()
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_UNIMPLEMENTED);
	tmp.Put32Bit(m_RecvPackSeq);
	SendPacket2(&tmp);
}
void Cssh::SendDisconnect2(int st, LPCSTR str)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_DISCONNECT);
	tmp.Put32Bit(st);
	tmp.PutStr(str);
	SendPacket2(&tmp);
}
void Cssh::SendPacket2(CBuffer *bp)
{
	int n = bp->GetSize() + 128;
	int pad;
	CBuffer tmp(n);
	CBuffer enc(n);

	tmp.PutSpc(4 + 1);		// Size + Pad Era
	m_EncCmp.Compress(bp->GetPtr(), bp->GetSize(), &tmp);

	if ( (pad = m_EncCip.GetBlockSize() - (tmp.GetSize() % m_EncCip.GetBlockSize())) < 4 )
		pad += m_EncCip.GetBlockSize();

	tmp.SET32BIT(tmp.GetPos(0), tmp.GetSize() - 4 + pad);
	tmp.SET8BIT(tmp.GetPos(4), pad);
	for ( n = 0 ; n < pad ; n += 16 )
		tmp.Apend((LPBYTE)"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", ((pad - n) >= 16 ? 16 : (pad - n)));

	m_EncCip.Cipher(tmp.GetPtr(), tmp.GetSize(), &enc);

	if ( m_EncMac.GetBlockSize() > 0 )
		m_EncMac.Compute(m_SendPackSeq, tmp.GetPtr(), tmp.GetSize(), &enc);

	CExtSocket::Send(enc.GetPtr(), enc.GetSize());

	m_SendPackSeq++;
}

int Cssh::SSH2MsgKexInit(CBuffer *bp)
{
	int n;
	CBuffer tmp;
	static const struct {
		int	mode;
		char *name;
	} kextab[] = {
		{ DHMODE_GROUP_GEX256,	"diffie-hellman-group-exchange-sha256",	},
		{ DHMODE_GROUP_GEX,		"diffie-hellman-group-exchange-sha1",	},
		{ DHMODE_GROUP_14,		"diffie-hellman-group14-sha1",			},
		{ DHMODE_GROUP_1,		"diffie-hellman-group1-sha1",			},
		{ 0,					NULL									},
	};

	m_HisPeer = *bp;
	for ( n = 0 ; n < 16 ; n++ )
		m_Cookie[n] = bp->Get8Bit();
	for ( n = 0 ; n < 10 ; n++ )
		bp->GetStr(m_SProp[n]);
	bp->Get8Bit();
	bp->Get32Bit();

	for ( n = 0 ; kextab[n].name != NULL ; n++ ) {
		m_DhMode   = kextab[n].mode;
		m_CProp[0] = kextab[n].name;	// PROPOSAL_KEX_ALGS
		if ( m_SProp[0].Find(m_CProp[0]) >= 0 )
			break;
	}
	if ( kextab[n].name == NULL )
		return TRUE;

	m_CProp[1] = "ssh-dss";						// PROPOSAL_SERVER_HOST_KEY_ALGS
	if ( m_SProp[1].Find(m_CProp[1]) < 0 ) {
		m_CProp[1] = "ssh-rsa";
		if ( m_SProp[1].Find(m_CProp[1]) < 0 )
			return TRUE;
	}
	m_pDocument->m_ParamTab.GetProp(6, m_CProp[2], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFENC));			// PROPOSAL_ENC_ALGS_CTOS,
	m_pDocument->m_ParamTab.GetProp(3, m_CProp[3], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFENC));			// PROPOSAL_ENC_ALGS_STOC,
	m_pDocument->m_ParamTab.GetProp(7, m_CProp[4], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFMAC));			// PROPOSAL_MAC_ALGS_CTOS,
	m_pDocument->m_ParamTab.GetProp(4, m_CProp[5], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFMAC));			// PROPOSAL_MAC_ALGS_STOC,
	m_pDocument->m_ParamTab.GetProp(8, m_CProp[6]);				// PROPOSAL_COMP_ALGS_CTOS,
	m_pDocument->m_ParamTab.GetProp(5, m_CProp[7]);				// PROPOSAL_COMP_ALGS_STOC,
	m_CProp[8] = "";											// PROPOSAL_LANG_CTOS,
	m_CProp[9] = "";											// PROPOSAL_LANG_STOC,

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( !MatchList(m_CProp[n + 2], m_SProp[n + 2], m_VProp[n]) )
			return TRUE;
	}

	tmp.Put8Bit(SSH2_MSG_KEXINIT);
	for ( n = 0 ; n < 16 ; n++ )
		tmp.Put8Bit(m_Cookie[n]);
	for ( n = 0 ; n < 10 ; n++ )
		tmp.PutStr(m_CProp[n]);
	tmp.Put8Bit(0);
	tmp.Put32Bit(0);
	m_MyPeer.Clear();
	m_MyPeer.Apend(tmp.GetPtr() + 1, tmp.GetSize() - 1);
	SendPacket2(&tmp);

	m_NeedKeyLen = 0;
	if ( m_NeedKeyLen < m_EncCip.GetKeyLen(m_VProp[0]) )    m_NeedKeyLen = m_EncCip.GetKeyLen(m_VProp[0]);
	if ( m_NeedKeyLen < m_DecCip.GetKeyLen(m_VProp[1]) )    m_NeedKeyLen = m_DecCip.GetKeyLen(m_VProp[1]);
	if ( m_NeedKeyLen < m_EncCip.GetBlockSize(m_VProp[0]) ) m_NeedKeyLen = m_EncCip.GetBlockSize(m_VProp[0]);
	if ( m_NeedKeyLen < m_DecCip.GetBlockSize(m_VProp[1]) ) m_NeedKeyLen = m_DecCip.GetBlockSize(m_VProp[1]);
	if ( m_NeedKeyLen < m_EncMac.GetKeyLen(m_VProp[2]) )    m_NeedKeyLen = m_EncMac.GetKeyLen(m_VProp[2]);
	if ( m_NeedKeyLen < m_DecMac.GetKeyLen(m_VProp[3]) )    m_NeedKeyLen = m_DecMac.GetKeyLen(m_VProp[3]);

	return FALSE;
}
int Cssh::SSH2MsgKexDhReply(CBuffer *bp)
{
	int n;
	CBuffer tmp, blob, sig;
	CIdKey key;
	BIGNUM *spub, *ssec;
	LPBYTE p, hash;

	bp->GetBuf(&tmp);
	blob = tmp;

	if ( !key.GetBlob(&tmp) )
		return TRUE;

	if ( !key.HostVerify(m_HostName) )
		return TRUE;

	if ( (spub = BN_new()) == NULL || (ssec = BN_new()) == NULL )
		return TRUE;

	bp->GetBIGNUM2(spub);
	bp->GetBuf(&sig);

	if ( !dh_pub_is_valid(m_SaveDh, spub) )
		return TRUE;

	tmp.Clear();
	p = tmp.PutSpc(DH_size(m_SaveDh));
	n = DH_compute_key(p, spub, m_SaveDh);
	BN_bin2bn(p, n, ssec);

	hash = kex_dh_hash(
		m_ClientVerStr, m_ServerVerStr,
		m_MyPeer.GetPtr(), m_MyPeer.GetSize(),
		m_HisPeer.GetPtr(), m_HisPeer.GetSize(),
		blob.GetPtr(), blob.GetSize(),
		m_SaveDh->pub_key, spub, ssec);

	if ( m_SessionIdLen == 0 ) {
		m_SessionIdLen = 20;
		memcpy(m_SessionId, hash, 20);
	}

	if ( !key.Verify(&sig, hash, 20) )
		return TRUE;

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
		m_VKey[n] = derive_key('A' + n, m_NeedKeyLen, hash, 20, ssec,
			m_SessionId, m_SessionIdLen, EVP_sha1());
	}

	BN_clear_free(spub);
	BN_clear_free(ssec);

	return FALSE;
}
int Cssh::SSH2MsgKexDhGexGroup(CBuffer *bp)
{
	CBuffer tmp;

	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);

	if ( (m_SaveDh = DH_new()) == NULL )
		return TRUE;
	if ( (m_SaveDh->p = BN_new()) == NULL )
		return TRUE;
	if ( (m_SaveDh->g = BN_new()) == NULL )
		return TRUE;

	if ( !bp->GetBIGNUM2(m_SaveDh->p) )
		return TRUE;
	if ( !bp->GetBIGNUM2(m_SaveDh->g) )
		return TRUE;

	dh_gen_key(m_SaveDh, m_NeedKeyLen * 8);

	tmp.Put8Bit(SSH2_MSG_KEX_DH_GEX_INIT);
	tmp.PutBIGNUM2(m_SaveDh->pub_key);
	SendPacket2(&tmp);

	return FALSE;
}
int Cssh::SSH2MsgKexDhGexReply(CBuffer *bp)
{
	int n;
	CBuffer tmp, blob, sig;
	CIdKey key;
	BIGNUM *spub, *ssec;
	LPBYTE p, hash;
	int hashlen;

	bp->GetBuf(&tmp);
	blob = tmp;

	if ( !key.GetBlob(&tmp) )
		return TRUE;

	if ( !key.HostVerify(m_HostName) )
		return TRUE;

	if ( (spub = BN_new()) == NULL || (ssec = BN_new()) == NULL )
		return TRUE;

	bp->GetBIGNUM2(spub);
	bp->GetBuf(&sig);

	if ( !dh_pub_is_valid(m_SaveDh, spub) )
		return TRUE;

	tmp.Clear();
	p = tmp.PutSpc(DH_size(m_SaveDh));
	n = DH_compute_key(p, spub, m_SaveDh);
	BN_bin2bn(p, n, ssec);

	hash = kex_gex_hash(
		m_ClientVerStr, m_ServerVerStr,
		m_MyPeer.GetPtr(), m_MyPeer.GetSize(),
		m_HisPeer.GetPtr(), m_HisPeer.GetSize(),
		blob.GetPtr(), blob.GetSize(),
		1024, dh_estimate(m_NeedKeyLen * 8), 8192,
		m_SaveDh->p, m_SaveDh->g, m_SaveDh->pub_key,
		spub, ssec,
		&hashlen,
		m_DhMode == DHMODE_GROUP_GEX ? EVP_sha1() : EVP_sha256());

	if ( m_SessionIdLen == 0 ) {
		m_SessionIdLen = hashlen;
		memcpy(m_SessionId, hash, hashlen);
	}

	if ( !key.Verify(&sig, hash, hashlen) )
		return TRUE;

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
		m_VKey[n] = derive_key('A' + n, m_NeedKeyLen, hash, hashlen, ssec,
			m_SessionId, m_SessionIdLen,
			m_DhMode == DHMODE_GROUP_GEX ? EVP_sha1() : EVP_sha256());
	}

	BN_clear_free(spub);
	BN_clear_free(ssec);

	return FALSE;
}
int Cssh::SSH2MsgNewKeys(CBuffer *bp)
{
	if ( m_DecCip.Init(m_VProp[1], MODE_DEC, m_VKey[3], (-1), m_VKey[1]) ||
		 m_DecMac.Init(m_VProp[3], m_VKey[5], (-1)) ||
		 m_DecCmp.Init(m_VProp[5], MODE_DEC, 6) )
		return TRUE;

	CString str, ats[3];

	ats[0] = m_EncCmp.GetTitle();
	if ( ats[0].Compare(m_DecCmp.GetTitle()) != 0 ) {
		ats[0] += _T("/");
		ats[0] += m_DecCmp.GetTitle();
	}
	ats[1] = m_EncCip.GetTitle();
	if ( ats[1].Compare(m_DecCip.GetTitle()) != 0 ) {
		ats[1] += _T("/");
		ats[1] += m_DecCip.GetTitle();
	}
	ats[2] = m_EncMac.GetTitle();
	if ( ats[2].Compare(m_DecMac.GetTitle()) != 0 ) {
		ats[2] += _T("/");
		ats[2] += m_DecMac.GetTitle();
	}
	str.Format(_T("%s+%s+%s"), ats[0], ats[1], ats[2]);
	m_pDocument->SetStatus(str);

	return FALSE;
}
int Cssh::SSH2MsgUserAuthInfoRequest(CBuffer *bp)
{
	int n, e, max;
	CBuffer tmp;
	CString name, inst, lang, prom;
	CPassDlg dlg;

	bp->GetStr(name);
	bp->GetStr(inst);
	bp->GetStr(lang);
	max = bp->Get32Bit();

	tmp.Put8Bit(SSH2_MSG_USERAUTH_INFO_RESPONSE);
	tmp.Put32Bit(max);

	for ( n = 0 ; n < max ; n++ ) {
		bp->GetStr(prom);
		e = bp->Get8Bit();
		dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
		dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
		dlg.m_Prompt   = prom;
		dlg.m_PassName = "";
		if ( dlg.DoModal() != IDOK )
			return FALSE;
		tmp.PutStr(dlg.m_PassName);
	}

	SendPacket2(&tmp);
	return TRUE;
}
int Cssh::SSH2MsgChannelOpen(CBuffer *bp)
{
	int n, i, id;
	CBuffer tmp;
	CString type;
	CString host[2];
	int port[2];
	CString str;
	char *p;

	bp->GetStr(type);
	id = bp->Get32Bit();

	n = ChannelOpen();
	m_Chan[n].m_TypeName   = type;
	m_Chan[n].m_RemoteID   = id;
	m_Chan[n].m_RemoteWind = bp->Get32Bit();
	m_Chan[n].m_RemoteMax  = bp->Get32Bit();
	m_Chan[n].m_LocalComs  = 0;

	TRACE("Channel Open Recive %d(%d)\n", m_Chan[n].m_RemoteWind, m_Chan[n].m_RemoteMax);

	if ( type.CompareNoCase("auth-agent@openssh.com") == 0 ) {
		m_Chan[n].m_pFilter = new CAgent;
		m_Chan[n].m_pFilter->m_pChan = &(m_Chan[n]);
		m_Chan[n].m_Status |= CHAN_OPEN_REMOTE;
		SendMsgChannelOpenConfirmation(n);
		SendMsgChannelData(n);
		return FALSE;

	} else if ( type.CompareNoCase("x11") == 0 ) {
		bp->GetStr(host[0]);
		port[0] = bp->Get32Bit();

		if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHX11PF) )
			goto FAILURE;

		if ( (p = getenv("DISPLAY")) == NULL ) {
			if ( m_pDocument->m_ParamTab.m_XDisplay.IsEmpty() )
				p = ":0";
			else
				p = (char *)(LPCSTR)m_pDocument->m_ParamTab.m_XDisplay;
		}

		host[1].Empty();
		while ( *p != '\0' && *p != ':' )
			host[1] += *(p++);
		if ( *(p++) != ':' )
			goto FAILURE;
		port[1] = atoi(p);

		if ( host[1].IsEmpty() || host[1].CompareNoCase("unix") == 0 )
			host[1] = "127.0.0.1";

		m_Chan[n].m_Status |= CHAN_OPEN_REMOTE;
		m_Chan[n].m_lHost = host[1];
		m_Chan[n].m_lPort = 6000 + port[1];
		m_Chan[n].m_rHost = host[0];
		m_Chan[n].m_rPort = port[0];

		if ( (i = m_Chan[n].Open(host[1], 6000 + port[1])) < 0 ) {
			str.Format("Channel Open Failure %s:%d->%s:%d", host[0], port[0], host[1], 6000 + port[1]);
			AfxMessageBox(str);
			goto FAILURE;
		}

		return FALSE;

	} else if ( type.CompareNoCase("forwarded-tcpip") != 0 )
		goto FAILURE;

	bp->GetStr(host[0]);
	port[0] = bp->Get32Bit();
	bp->GetStr(host[1]);
	port[1] = bp->Get32Bit();

	for ( i = 0 ; i < m_Permit.GetSize() ; i++ ) {
		if ( port[0] == m_Permit[i].m_rPort )
			break;
	}

	if ( i >=  m_Permit.GetSize() ) {
		str.Format("Channel Open Permit Failure %s:%d->%s:%d", host[0], port[0], host[1], port[1]);
		AfxMessageBox(str);
		goto FAILURE;
	}

	m_Chan[n].m_Status |= CHAN_OPEN_REMOTE;
	m_Chan[n].m_lHost = host[1];
	m_Chan[n].m_lPort = port[1];
	m_Chan[n].m_rHost = host[0];
	m_Chan[n].m_rPort = port[0];

	if ( (i = m_Chan[n].Open(m_Permit[i].m_lHost, m_Permit[i].m_lPort)) < 0 ) {
		str.Format("Channel Open Failure %s:%d->%s:%d", host[0], port[0], host[1], port[1]);
		AfxMessageBox(str);
		goto FAILURE;
	}

	return FALSE;

FAILURE:
	ChannelClose(n);
	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_FAILURE);
	tmp.Put32Bit(id);
	tmp.Put32Bit(SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED);
	tmp.PutStr("open failed");
	tmp.PutStr("");
	SendPacket2(&tmp);
	return FALSE;
}
int Cssh::SSH2MsgChannelOpenReply(CBuffer *bp, int type)
{
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || m_Chan[id].m_Status != CHAN_OPEN_LOCAL )
		return TRUE;

	if ( type == SSH2_MSG_CHANNEL_OPEN_FAILURE ) {
		if ( m_Chan[id].m_pFilter != NULL )
			LogIt("Open Failure #%d Filter", id);
		else
			LogIt("Open Failure #%d %s:%d -> %s:%d", id, m_Chan[id].m_lHost, m_Chan[id].m_lPort, m_Chan[id].m_rHost, m_Chan[id].m_rPort);
		ChannelClose(id);
		return (id == m_StdChan ? TRUE : FALSE);
	}

	m_Chan[id].m_Status     |= CHAN_OPEN_REMOTE;
	m_Chan[id].m_RemoteID    = bp->Get32Bit();
	m_Chan[id].m_RemoteWind  = bp->Get32Bit();
	m_Chan[id].m_RemoteMax   = bp->Get32Bit();
	m_Chan[id].m_ConnectTime = CTime::GetCurrentTime();
	m_OpenChanCount++;

	TRACE("Channel OpenReply Recive %d(%d)\n", m_Chan[id].m_RemoteWind, m_Chan[id].m_RemoteMax);

	if ( m_Chan[id].m_pFilter != NULL ) {
		switch(m_Chan[id].m_pFilter->m_Type) {
		case 1:
			if ( (m_SSH2Status & SSH2_STAT_HAVESHELL) == 0 )
				SendMsgChannelRequesstShell(id);
			break;
		case 2:
			SendMsgChannelRequesstSubSystem(id);
			break;
		case 4:
			SendMsgChannelRequesstExec(id);
			break;
		}
	} else
		LogIt("Open Confirmation #%d %s:%d -> %s:%d", id, m_Chan[id].m_lHost, m_Chan[id].m_lPort, m_Chan[id].m_rHost, m_Chan[id].m_rPort);

	SendMsgChannelData(id);
	return FALSE;
}
int Cssh::SSH2MsgChannelClose(CBuffer *bp)
{
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() )
		return TRUE;

	m_Chan[id].m_Eof |= CEOF_ICLOSE;
	ChannelCheck(id);

	return FALSE;
}
int Cssh::SSH2MsgChannelData(CBuffer *bp, int type)
{
	CBuffer tmp;
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;

	bp->GetBuf(&tmp);
	m_Chan[id].m_LocalComs += tmp.GetSize();

	if ( type == SSH2_MSG_CHANNEL_DATA )
		m_Chan[id].Send(tmp.GetPtr(), tmp.GetSize());

	ChannelPolling(id);
	return FALSE;
}
int Cssh::SSH2MsgChannelEof(CBuffer *bp)
{
	int id = bp->Get32Bit();
	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;
	m_Chan[id].m_Eof |= CEOF_IEOF;
	ChannelCheck(id);
	return FALSE;
}
int Cssh::SSH2MsgChannelRequesst(CBuffer *bp)
{
	int reply;
	CString str;
	CBuffer tmp;
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;

	bp->GetStr(str);
	reply = bp->Get8Bit();

	if ( str.Compare("exit-status") == 0 ) {
		bp->Get32Bit();
		if ( id == m_StdChan )
			m_SSH2Status &= ~SSH2_STAT_HAVESHELL;
	}

	if ( reply ) {
		tmp.Put8Bit(SSH2_MSG_CHANNEL_SUCCESS);
		tmp.Put32Bit(id);
		SendPacket2(&tmp);
	}
	return FALSE;
}
int Cssh::SSH2MsgChannelRequestReply(CBuffer *bp, int type)
{
	int num;
	int id = bp->Get32Bit();

	if ( m_ChnReqMap.GetSize() <= 0 )
		return FALSE;
	num = m_ChnReqMap.GetAt(0);
	m_ChnReqMap.RemoveAt(0);

	switch(num) {
	case 0:		// pty-req
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			return TRUE;
		m_SSH2Status |= SSH2_STAT_HAVESTDIO;
		break;
	case 1:		// shell
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			return TRUE;
		m_SSH2Status |= SSH2_STAT_HAVESHELL;
		if ( m_StdInRes.GetSize() > 0 && CHAN_OK(m_StdChan) ) {
			m_Chan[m_StdChan].m_pFilter->Send(m_StdInRes.GetPtr(), m_StdInRes.GetSize());
			m_StdInRes.Clear();
		}
		break;
	case 2:		// subsystem
	case 3:		// exec
		if ( type == SSH2_MSG_CHANNEL_FAILURE ) {
			m_Chan[id].m_Eof |= CEOF_DEAD;
			ChannelCheck(id);
		} else if ( m_Chan[id].m_pFilter != NULL )
			m_Chan[id].m_pFilter->OnConnect();
		break;
	case 4:		// x11-req
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			AfxMessageBox("X11-req Failure");
		break;
	case 5:		// env
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			AfxMessageBox("env Failure");
		break;
	}
	return FALSE;
}
int Cssh::SSH2MsgChannelAdjust(CBuffer *bp)
{
	int len;
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;

	if ( (len = bp->Get32Bit()) < 0 )
		return TRUE;

	m_Chan[id].m_RemoteWind += len;

	TRACE("Recive Window Adjust %d\n", len);

	if ( m_Chan[id].m_Output.GetSize() > 0 )
		SendMsgChannelData(id);

	return FALSE;
}
int Cssh::SSH2MsgGlobalRequest(CBuffer *bp)
{
	CBuffer tmp;
	CString str, msg;
	BOOL success = FALSE;

	bp->GetStr(str);
	if ( !str.IsEmpty() ) {
		msg.Format("Get Msg Global Request '%s'", str);
//		AfxMessageBox(msg);
	}
	if ( bp->Get8Bit() > 0 ) {
		tmp.Put8Bit(success ? SSH2_MSG_REQUEST_SUCCESS : SSH2_MSG_REQUEST_FAILURE);
		SendPacket2(&tmp);
	}
	return FALSE;
}
int Cssh::SSH2MsgGlobalRequestReply(CBuffer *bp, int type)
{
	int num;
	CString str;

	if ( m_GlbReqMap.GetSize() <= 0 ) {
		AfxMessageBox("Get Msg Global Request Underflow ?");
		return FALSE;
	}
	num = m_GlbReqMap.GetAt(0);
	m_GlbReqMap.RemoveAt(0);

	if ( (WORD)num == 0xFFFF )	// KeepAlive Reply
		return FALSE;

	if ( type == SSH2_MSG_REQUEST_FAILURE && num < m_Permit.GetSize() ) {
		str.Format("Global Request Failure %s:%d->%s:%d",
			m_Permit[num].m_lHost, m_Permit[num].m_lPort, m_Permit[num].m_rHost, m_Permit[num].m_rPort);
		AfxMessageBox(str);
		m_Permit.RemoveAt(num);
	}

	return FALSE;
}
void Cssh::RecivePacket2(CBuffer *bp)
{
	CString str;
	int type = bp->Get8Bit();

//	TRACE("SSH2 Type %d\n", type);

	switch(type) {
	case SSH2_MSG_KEXINIT:
		if ( SSH2MsgKexInit(bp) )
			goto DISCONNECT;
		m_SSH2Status |= SSH2_STAT_HAVEPROP;
		switch(m_DhMode) {
		case DHMODE_GROUP_1:
		case DHMODE_GROUP_14:
			SendMsgKexDhInit();
			break;
		case DHMODE_GROUP_GEX:
		case DHMODE_GROUP_GEX256:
			SendMsgKexDhGexRequest();
			break;
		}
		break;
	case SSH2_MSG_KEXDH_REPLY:	// SSH2_MSG_KEX_DH_GEX_GROUP:
		if ( (m_SSH2Status & SSH2_STAT_HAVEPROP) == 0 )
			goto DISCONNECT;
		switch(m_DhMode) {
		case DHMODE_GROUP_1:
		case DHMODE_GROUP_14:
			if ( SSH2MsgKexDhReply(bp) )
				goto DISCONNECT;
			m_SSH2Status &= ~SSH2_STAT_HAVEPROP;
			m_SSH2Status |= SSH2_STAT_HAVEKEYS;
			SendMsgNewKeys();
			break;
		case DHMODE_GROUP_GEX:
		case DHMODE_GROUP_GEX256:
			if ( SSH2MsgKexDhGexGroup(bp) )
				goto DISCONNECT;
			break;
		}
		break;
	case SSH2_MSG_KEX_DH_GEX_REPLY:
		if ( (m_SSH2Status & SSH2_STAT_HAVEPROP) == 0 )
			goto DISCONNECT;
		if ( SSH2MsgKexDhGexReply(bp) )
			goto DISCONNECT;
		m_SSH2Status &= ~SSH2_STAT_HAVEPROP;
		m_SSH2Status |= SSH2_STAT_HAVEKEYS;
		SendMsgNewKeys();
		break;
	case SSH2_MSG_NEWKEYS:
		if ( (m_SSH2Status & SSH2_STAT_HAVEKEYS) == 0 )
			goto DISCONNECT;
		if ( SSH2MsgNewKeys(bp) )
			goto DISCONNECT;
		m_SSH2Status &= ~SSH2_STAT_HAVEKEYS;
		m_SSH2Status |= SSH2_STAT_HAVESESS;
		if ( (m_SSH2Status & SSH2_STAT_HAVELOGIN) == 0 ) {
			m_AuthStat = 0;
			SendMsgServiceRequest("ssh-userauth");
		}
		break;
	case SSH2_MSG_SERVICE_ACCEPT:
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		bp->GetStr(str);
		if ( str.Compare("ssh-userauth") == 0 ) {
			if ( !SendMsgUserAuthRequest(NULL) )
				goto DISCONNECT;
		} else
			goto DISCONNECT;
		break;

	case SSH2_MSG_USERAUTH_SUCCESS:
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		if ( m_EncCmp.m_Mode == 4 )
			m_EncCmp.Init(NULL, MODE_ENC, 6);
		if ( m_DecCmp.m_Mode == 4 )
			m_DecCmp.Init(NULL, MODE_DEC, 6);
		m_SSH2Status |= SSH2_STAT_HAVELOGIN;
		if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) && (m_SSH2Status & SSH2_STAT_HAVESTDIO) == 0 ) {
			m_StdChan = ChannelOpen();
			m_Chan[m_StdChan].m_pFilter = new CStdIoFilter;
			m_Chan[m_StdChan].m_pFilter->m_pChan = &(m_Chan[m_StdChan]);
			m_Chan[m_StdChan].m_LocalPacks = 2 * 1024;
			m_Chan[m_StdChan].m_LocalWind = 16 * m_Chan[m_StdChan].m_LocalPacks;
			SendMsgChannelOpen(m_StdChan, "session");
		}
		if ( (m_SSH2Status & SSH2_STAT_HAVEPFWD) == 0 ) {
			m_SSH2Status |= SSH2_STAT_HAVEPFWD;
			PortForward();
		}
		if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHKEEPAL) && m_pDocument->m_TextRam.m_KeepAliveSec > 0 )
			((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(m_pDocument->m_TextRam.m_KeepAliveSec * 1000, TIMEREVENT_SOCK | TIMEREVENT_INTERVAL, this);
		break;
	case SSH2_MSG_USERAUTH_FAILURE:
		bp->GetStr(str);
		if ( !SendMsgUserAuthRequest(str) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_USERAUTH_BANNER:
		bp->GetStr(str);
		CExtSocket::OnReciveCallBack((void *)(LPCSTR)str, str.GetLength(), 0);
		break;
	case SSH2_MSG_USERAUTH_INFO_REQUEST:
		if ( !SSH2MsgUserAuthInfoRequest(bp) )
			goto DISCONNECT;
		break;

	case SSH2_MSG_CHANNEL_OPEN:
		if ( SSH2MsgChannelOpen(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_OPEN_FAILURE:
	case SSH2_MSG_CHANNEL_OPEN_CONFIRMATION:
		if ( SSH2MsgChannelOpenReply(bp, type) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_CLOSE:
		if ( SSH2MsgChannelClose(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_DATA:
	case SSH2_MSG_CHANNEL_EXTENDED_DATA:
		if ( SSH2MsgChannelData(bp, type) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_EOF:
		if ( SSH2MsgChannelEof(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_REQUEST:
		if ( SSH2MsgChannelRequesst(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_WINDOW_ADJUST:
		if ( SSH2MsgChannelAdjust(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_SUCCESS:
	case SSH2_MSG_CHANNEL_FAILURE:
		if ( SSH2MsgChannelRequestReply(bp, type) )
			goto DISCONNECT;
		break;

	case SSH2_MSG_GLOBAL_REQUEST:
		if ( SSH2MsgGlobalRequest(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_REQUEST_SUCCESS:
	case SSH2_MSG_REQUEST_FAILURE:
		if ( SSH2MsgGlobalRequestReply(bp, type) )
			goto DISCONNECT;
		break;

	case SSH2_MSG_DISCONNECT:
		bp->Get32Bit();
		bp->GetStr(str);
//		CExtSocket::OnReciveCallBack((void *)((LPCSTR)str), str.GetLength(), 0);
//		Close();
		AfxMessageBox(str);
		break;
	case SSH2_MSG_IGNORE:
	case SSH2_MSG_DEBUG:
	case SSH2_MSG_UNIMPLEMENTED:
		break;

	default:
		SendMsgUnimplemented();
		break;
	DISCONNECT:
		SendDisconnect2(SSH2_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT, "Packet Type Error");
		break;
	}
}
