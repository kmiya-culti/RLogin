/***********
## XMSS reference code [![Build Status]
(https://travis-ci.org/joostrijneveld/xmss-reference.svg?branch=master)]
(https://travis-ci.org/joostrijneveld/xmss-reference)

This repository contains the reference implementation that accompanies the Internet Draft 
_"XMSS: Extended Hash-Based Signatures"_, [`draft-irtf-cfrg-xmss-hash-based-signatures`]
(https://datatracker.ietf.org/doc/draft-irtf-cfrg-xmss-hash-based-signatures/).

This reference implementation supports all parameter sets as defined in the Draft at run-time
(specified by prefixing the public and private keys with a 32-bit `oid`). Implementations that
want to use compile-time parameter sets can remove the `struct xmss_params` function parameter.

_When using the current code base, please be careful, expect changes and watch this document
for further documentation. In particular, `xmss_core_fast.c` is long due for a serious clean-up.
While this will not change its public API or output, it may affect the storage format of the BDS
state (i.e. part of the secret key)._

### Dependencies

For the SHA-2 hash functions (i.e. SHA-256 and SHA-512), we rely on OpenSSL. Make sure to install
the OpenSSL development headers. On Debian-based systems, this is achieved by installing the OpenSSL
development package `libssl-dev`.

### License

This reference implementation was written by Andreas Hulsing and Joost Rijneveld. All included code
is available under the CC0 1.0 Universal Public Domain Dedication.
**************/

#include "stdafx.h"
#include "RLogin.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#define	MAX_N		64
#define	MAX_D		12

//////////////////////////////////////////////////////////////////////
// params.h

/* These are merely internal identifiers for the supported hash functions. */
#define XMSS_SHA2 0
#define XMSS_SHAKE 1

/* This is a result of the OID definitions in the draft; needed for parsing. */
#define XMSS_OID_LEN 4

/* This structure will be populated when calling xmss[mt]_parse_oid. */
typedef struct _xmss_params {
    unsigned int func;
    unsigned int n;
    unsigned int wots_w;
    unsigned int wots_log_w;
    unsigned int wots_len1;
    unsigned int wots_len2;
    unsigned int wots_len;
    unsigned int wots_sig_bytes;
    unsigned int full_height;
    unsigned int tree_height;
    unsigned int d;
    unsigned int index_bytes;
    unsigned int sig_bytes;
    unsigned int pk_bytes;
    unsigned long long sk_bytes;
    unsigned int bds_k;
} xmss_params;

/**
 * Accepts strings such as "XMSS-SHA2_10_256"
 *  and outputs OIDs such as 0x01000001.
 * Returns -1 when the parameter set is not found, 0 otherwise
 */
int xmss_str_to_oid(uint32_t *oid, const char *s);

/**
 * Accepts takes strings such as "XMSSMT-SHA2_20/2_256"
 *  and outputs OIDs such as 0x01000001.
 * Returns -1 when the parameter set is not found, 0 otherwise
 */
int xmssmt_str_to_oid(uint32_t *oid, const char *s);

/**
 * Accepts OIDs such as 0x01000001, and configures params accordingly.
 * Returns -1 when the OID is not found, 0 otherwise.
 */
int xmss_parse_oid(xmss_params *params, const uint32_t oid);

/**
 * Accepts OIDs such as 0x01000001, and configures params accordingly.
 * Returns -1 when the OID is not found, 0 otherwise.
 */
int xmssmt_parse_oid(xmss_params *params, const uint32_t oid);

//////////////////////////////////////////////////////////////////////
// xmss.h

/**
 * Generates a XMSS key pair for a given parameter set.
 * Format sk: [OID || (32bit) idx || SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [OID || root || PUB_SEED]
 */
int xmss_keypair(unsigned char *pk, unsigned char *sk, const uint32_t oid);

/**
 * Signs a message using an XMSS secret key.
 * Returns
 * 1. an array containing the signature followed by the message AND
 * 2. an updated secret key!
 */
int xmss_sign(unsigned char *sk,
              unsigned char *sm, unsigned long long *smlen,
              const unsigned char *m, unsigned long long mlen);

/**
 * Verifies a given message signature pair using a given public key.
 *
 * Note: m and mlen are pure outputs which carry the message in case
 * verification succeeds. The (input) message is assumed to be contained in sm
 * which has the form [signature || message].
 */
int xmss_sign_open(unsigned char *m, unsigned long long *mlen,
                   const unsigned char *sm, unsigned long long smlen,
                   const unsigned char *pk);

/*
 * Generates a XMSSMT key pair for a given parameter set.
 * Format sk: [OID || (ceil(h/8) bit) idx || SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [OID || root || PUB_SEED]
 */
int xmssmt_keypair(unsigned char *pk, unsigned char *sk, const uint32_t oid);

/**
 * Signs a message using an XMSSMT secret key.
 * Returns
 * 1. an array containing the signature followed by the message AND
 * 2. an updated secret key!
 */
int xmssmt_sign(unsigned char *sk,
                unsigned char *sm, unsigned long long *smlen,
                const unsigned char *m, unsigned long long mlen);

/**
 * Verifies a given message signature pair using a given public key.
 *
 * Note: m and mlen are pure outputs which carry the message in case
 * verification succeeds. The (input) message is assumed to be contained in sm
 * which has the form [signature || message].
 */
int xmssmt_sign_open(unsigned char *m, unsigned long long *mlen,
                     const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk);

//////////////////////////////////////////////////////////////////////
// randombytes.h

/**
 * Tries to read xlen bytes from a source of randomness, and writes them to x.
 */
void rand_buf(void *buf, int len);
#define	randombytes(b, n)		rand_buf(b, (int)(n))

//////////////////////////////////////////////////////////////////////
// hash_address.h

#define XMSS_ADDR_TYPE_OTS 0
#define XMSS_ADDR_TYPE_LTREE 1
#define XMSS_ADDR_TYPE_HASHTREE 2

//////////////////////////////////////////////////////////////////////
// utils.c

/**
 * Converts the value of 'in' to 'outlen' bytes in big-endian byte order.
 */
static void ull_to_bytes(unsigned char *out, unsigned int outlen,
                  unsigned long long in)
{
    //int i;
    ///* Iterate over out in decreasing order, for big-endianness. */
    //for (i = outlen - 1; i >= 0; i--) {
    //    out[i] = in & 0xff;
    //    in = in >> 8;
    //}

	ASSERT(sizeof(unsigned long long) <= 8);

	if ( outlen > sizeof(unsigned long long) ) {
		ZeroMemory(out, outlen - sizeof(unsigned long long));
		out += (outlen - sizeof(unsigned long long));
		outlen = sizeof(unsigned long long);
	}

	switch(outlen) {
	case 8: out[7] = (unsigned char)in; in >>= 8;
	case 7: out[6] = (unsigned char)in; in >>= 8;
	case 6: out[5] = (unsigned char)in; in >>= 8;
	case 5: out[4] = (unsigned char)in; in >>= 8;
	case 4: out[3] = (unsigned char)in; in >>= 8;
	case 3: out[2] = (unsigned char)in; in >>= 8;
	case 2: out[1] = (unsigned char)in; in >>= 8;
	case 1: out[0] = (unsigned char)in; in >>= 8;
	case 0: break;
	}
}

/**
 * Converts the inlen bytes in 'in' from big-endian byte order to an integer.
 */
static unsigned long long bytes_to_ull(const unsigned char *in, unsigned int inlen)
{
    //unsigned long long retval = 0;
    //unsigned int i;
    //for (i = 0; i < inlen; i++) {
    //    retval |= ((unsigned long long)in[i]) << (8*(inlen - 1 - i));
    //}
    //return retval;

	ASSERT(sizeof(unsigned long long) <= 8);

	if ( inlen > sizeof(unsigned long long) ) {
		in += (inlen - sizeof(unsigned long long));
		inlen = sizeof(unsigned long long);
	}

	unsigned long long val = 0;
	switch(inlen) {
	case 8: val |= (((unsigned long long)*(in++)) << 56);
	case 7: val |= (((unsigned long long)*(in++)) << 48);
	case 6: val |= (((unsigned long long)*(in++)) << 40);
	case 5: val |= (((unsigned long long)*(in++)) << 32);
	case 4: val |= (((unsigned long long)*(in++)) << 24);
	case 3: val |= (((unsigned long long)*(in++)) << 16);
	case 2: val |= (((unsigned long long)*(in++)) << 8);
	case 1: val |= ((unsigned long long)*in);
	}
	return val;
}

//////////////////////////////////////////////////////////////////////
// fips202.c in shake128/256 -> openssl SHAKE128/256 

void SHAKE128(const unsigned char *in, size_t inlen, unsigned char *md, size_t mdlen)
{
	EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
	if ( md_ctx == NULL ||
	     EVP_DigestInit(md_ctx, EVP_shake128()) == 0 ||
		 EVP_MD_CTX_ctrl(md_ctx, EVP_MD_CTRL_XOF_LEN, (int)mdlen, NULL) == 0 ||
	     EVP_DigestUpdate(md_ctx, in, inlen) == 0 ||
		 EVP_DigestFinal(md_ctx, md, NULL) )
		throw _T("SHAKE128 Error");
	EVP_MD_CTX_free(md_ctx);
}
void SHAKE256(const unsigned char *in, size_t inlen, unsigned char *md, size_t mdlen)
{
	EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
	if ( md_ctx == NULL ||
		 EVP_DigestInit(md_ctx, EVP_shake256()) == 0 ||
		 EVP_MD_CTX_ctrl(md_ctx, EVP_MD_CTRL_XOF_LEN, (int)mdlen, NULL) == 0 ||
		 EVP_DigestUpdate(md_ctx, in, inlen) == 0 ||
		 EVP_DigestFinal(md_ctx, md, NULL) == 0 )
		throw _T("SHAKE256 Error");
	EVP_MD_CTX_free(md_ctx);
}

//////////////////////////////////////////////////////////////////////
// hash_address.c

static void set_layer_addr(uint32_t addr[8], uint32_t layer)
{
    addr[0] = layer;
}

static void set_tree_addr(uint32_t addr[8], uint64_t tree)
{
    addr[1] = (uint32_t) (tree >> 32);
    addr[2] = (uint32_t) tree;
}

static void set_type(uint32_t addr[8], uint32_t type)
{
    addr[3] = type;
}

static void set_key_and_mask(uint32_t addr[8], uint32_t key_and_mask)
{
    addr[7] = key_and_mask;
}

static void copy_subtree_addr(uint32_t out[8], const uint32_t in[8])
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

/* These functions are used for OTS addresses. */

static void set_ots_addr(uint32_t addr[8], uint32_t ots)
{
    addr[4] = ots;
}

static void set_chain_addr(uint32_t addr[8], uint32_t chain)
{
    addr[5] = chain;
}

static void set_hash_addr(uint32_t addr[8], uint32_t hash)
{
    addr[6] = hash;
}

/* This function is used for L-tree addresses. */

static void set_ltree_addr(uint32_t addr[8], uint32_t ltree)
{
    addr[4] = ltree;
}

/* These functions are used for hash tree addresses. */

static void set_tree_height(uint32_t addr[8], uint32_t tree_height)
{
    addr[5] = tree_height;
}

static void set_tree_index(uint32_t addr[8], uint32_t tree_index)
{
    addr[6] = tree_index;
}

//////////////////////////////////////////////////////////////////////
// hash.c

#define XMSS_HASH_PADDING_F 0
#define XMSS_HASH_PADDING_H 1
#define XMSS_HASH_PADDING_HASH 2
#define XMSS_HASH_PADDING_PRF 3

static void addr_to_bytes(unsigned char *bytes, const uint32_t addr[8])
{
    int i;
    for (i = 0; i < 8; i++) {
        ull_to_bytes(bytes + i*4, 4, addr[i]);
    }
}

static int core_hash(const xmss_params *params,
                     unsigned char *out,
                     const unsigned char *in, unsigned long long inlen)
{
    if (params->n == 32 && params->func == XMSS_SHA2) {
        SHA256(in, (size_t)inlen, out);
    }
    else if (params->n == 32 && params->func == XMSS_SHAKE) {
        //shake128(out, 32, in, inlen);
        SHAKE128(in, (size_t)inlen, out, 32);
    }
    else if (params->n == 64 && params->func == XMSS_SHA2) {
        SHA512(in, (size_t)inlen, out);
    }
    else if (params->n == 64 && params->func == XMSS_SHAKE) {
        //shake256(out, 64, in, inlen);
        SHAKE256(in, (size_t)inlen, out, 64);
    }
    else {
        return -1;
    }
    return 0;
}

/*
 * Computes PRF(key, in), for a key of params->n bytes, and a 32-byte input.
 */
static int prf(const xmss_params *params,
        unsigned char *out, const unsigned char in[32],
        const unsigned char *key)
{
	int rt;
    unsigned char buf[2 * MAX_N + 32];	// [2*params->n + 32];

    ull_to_bytes(buf, params->n, XMSS_HASH_PADDING_PRF);
    memcpy(buf + params->n, key, params->n);
    memcpy(buf + 2*params->n, in, 32);

    rt = core_hash(params, out, buf, 2*params->n + 32);

	return rt;
}

/*
 * Computes the message hash using R, the public root, the index of the leaf
 * node, and the message. Notably, it requires m_with_prefix to have 4*n bytes
 * of space before the message, to use for the prefix. This is necessary to
 * prevent having to move the message around (and thus allocate memory for it).
 */
static int hash_message(const xmss_params *params, unsigned char *out,
                 const unsigned char *R, const unsigned char *root,
                 unsigned long long idx,
                 unsigned char *m_with_prefix, unsigned long long mlen)
{
    /* We're creating a hash using input of the form:
       toByte(X, 32) || R || root || index || M */
    ull_to_bytes(m_with_prefix, params->n, XMSS_HASH_PADDING_HASH);
    memcpy(m_with_prefix + params->n, R, params->n);
    memcpy(m_with_prefix + 2*params->n, root, params->n);
    ull_to_bytes(m_with_prefix + 3*params->n, params->n, idx);

    return core_hash(params, out, m_with_prefix, mlen + 4*params->n);
}

/**
 * We assume the left half is in in[0]...in[n-1]
 */
static int thash_h(const xmss_params *params,
            unsigned char *out, const unsigned char *in,
            const unsigned char *pub_seed, uint32_t addr[8])
{
	int rt;
    unsigned char buf[4 * MAX_N];		// [4 * params->n];
    unsigned char bitmask[2 * MAX_N];	// [2 * params->n];
    unsigned char addr_as_bytes[32];
    unsigned int i;

    /* Set the function padding. */
    ull_to_bytes(buf, params->n, XMSS_HASH_PADDING_H);

    /* Generate the n-byte key. */
    set_key_and_mask(addr, 0);
    addr_to_bytes(addr_as_bytes, addr);
    prf(params, buf + params->n, addr_as_bytes, pub_seed);

    /* Generate the 2n-byte mask. */
    set_key_and_mask(addr, 1);
    addr_to_bytes(addr_as_bytes, addr);
    prf(params, bitmask, addr_as_bytes, pub_seed);

    set_key_and_mask(addr, 2);
    addr_to_bytes(addr_as_bytes, addr);
    prf(params, bitmask + params->n, addr_as_bytes, pub_seed);

    for (i = 0; i < 2 * params->n; i++) {
        buf[2*params->n + i] = in[i] ^ bitmask[i];
    }
    rt = core_hash(params, out, buf, 4 * params->n);

	return rt;
}

static int thash_f(const xmss_params *params,
            unsigned char *out, const unsigned char *in,
            const unsigned char *pub_seed, uint32_t addr[8])
{
	int rt;
    unsigned char buf[3 * MAX_N];		// [3 * params->n];
    unsigned char bitmask[MAX_N];		// [params->n];
    unsigned char addr_as_bytes[32];
    unsigned int i;

    /* Set the function padding. */
    ull_to_bytes(buf, params->n, XMSS_HASH_PADDING_F);

    /* Generate the n-byte key. */
    set_key_and_mask(addr, 0);
    addr_to_bytes(addr_as_bytes, addr);
    prf(params, buf + params->n, addr_as_bytes, pub_seed);

    /* Generate the n-byte mask. */
    set_key_and_mask(addr, 1);
    addr_to_bytes(addr_as_bytes, addr);
    prf(params, bitmask, addr_as_bytes, pub_seed);

    for (i = 0; i < params->n; i++) {
        buf[2*params->n + i] = in[i] ^ bitmask[i];
    }
    rt = core_hash(params, out, buf, 3 * params->n);

	return rt;
}

//////////////////////////////////////////////////////////////////////
// wots.c

/**
 * Helper method for pseudorandom key generation.
 * Expands an n-byte array into a len*n byte array using the `prf` function.
 */
static void expand_seed(const xmss_params *params,
                        unsigned char *outseeds, const unsigned char *inseed)
{
    uint32_t i;
    unsigned char ctr[32];

    for (i = 0; i < params->wots_len; i++) {
        ull_to_bytes(ctr, 32, i);
        prf(params, outseeds + i*params->n, ctr, inseed);
    }
}

/**
 * Computes the chaining function.
 * out and in have to be n-byte arrays.
 *
 * Interprets in as start-th value of the chain.
 * addr has to contain the address of the chain.
 */
static void gen_chain(const xmss_params *params,
                      unsigned char *out, const unsigned char *in,
                      unsigned int start, unsigned int steps,
                      const unsigned char *pub_seed, uint32_t addr[8])
{
    uint32_t i;

    /* Initialize out with the value at position 'start'. */
    memcpy(out, in, params->n);

    /* Iterate 'steps' calls to the hash function. */
    for (i = start; i < (start+steps) && i < params->wots_w; i++) {
        set_hash_addr(addr, i);
        thash_f(params, out, out, pub_seed, addr);
    }
}

/**
 * base_w algorithm as described in draft.
 * Interprets an array of bytes as integers in base w.
 * This only works when log_w is a divisor of 8.
 */
static void base_w(const xmss_params *params,
                   int *output, const int out_len, const unsigned char *input)
{
    int in = 0;
    int out = 0;
    unsigned char total;
    int bits = 0;
    int consumed;

    for (consumed = 0; consumed < out_len; consumed++) {
        if (bits == 0) {
            total = input[in];
            in++;
            bits += 8;
        }
        bits -= params->wots_log_w;
        output[out] = (total >> bits) & (params->wots_w - 1);
        out++;
    }
}

/* Computes the WOTS+ checksum over a message (in base_w). */
static void wots_checksum(const xmss_params *params,
                          int *csum_base_w, const int *msg_base_w)
{
	/*
	wots_len2 = 3; wots_log_w = 4;	 (3 * 4 + 7) / 8 = 2 ?
	*/
    int csum = 0;
	int csum_len = (params->wots_len2 * params->wots_log_w + 7) / 8;
    unsigned char csum_bytes[4];	// [(params->wots_len2 * params->wots_log_w + 7) / 8];
    unsigned int i;

	ASSERT(csum_len <= 4);

    /* Compute checksum. */
    for (i = 0; i < params->wots_len1; i++) {
        csum += params->wots_w - 1 - msg_base_w[i];
    }

    /* Convert checksum to base_w. */
    /* Make sure expected empty zero bits are the least significant bits. */
    csum = csum << (8 - ((params->wots_len2 * params->wots_log_w) % 8));
    ull_to_bytes(csum_bytes, csum_len, csum);
    base_w(params, csum_base_w, params->wots_len2, csum_bytes);
}

/* Takes a message and derives the matching chain lengths. */
static void chain_lengths(const xmss_params *params,
                          int *lengths, const unsigned char *msg)
{
    base_w(params, lengths, params->wots_len1, msg);
    wots_checksum(params, lengths + params->wots_len1, lengths);
}

/**
 * WOTS key generation. Takes a 32 byte seed for the private key, expands it to
 * a full WOTS private key and computes the corresponding public key.
 * It requires the seed pub_seed (used to generate bitmasks and hash keys)
 * and the address of this WOTS key pair.
 *
 * Writes the computed public key to 'pk'.
 */
static void wots_pkgen(const xmss_params *params,
                unsigned char *pk, const unsigned char *seed,
                const unsigned char *pub_seed, uint32_t addr[8])
{
    uint32_t i;

    /* The WOTS+ private key is derived from the seed. */
    expand_seed(params, pk, seed);

    for (i = 0; i < params->wots_len; i++) {
        set_chain_addr(addr, i);
        gen_chain(params, pk + i*params->n, pk + i*params->n,
                  0, params->wots_w - 1, pub_seed, addr);
    }
}

/**
 * Takes a n-byte message and the 32-byte seed for the private key to compute a
 * signature that is placed at 'sig'.
 */
static void wots_sign(const xmss_params *params,
               unsigned char *sig, const unsigned char *msg,
               const unsigned char *seed, const unsigned char *pub_seed,
               uint32_t addr[8])
{
    int *lengths = new int [params->wots_len];
    uint32_t i;

    chain_lengths(params, lengths, msg);

    /* The WOTS+ private key is derived from the seed. */
    expand_seed(params, sig, seed);

    for (i = 0; i < params->wots_len; i++) {
        set_chain_addr(addr, i);
        gen_chain(params, sig + i*params->n, sig + i*params->n,
                  0, lengths[i], pub_seed, addr);
    }

	delete [] lengths;
}

/**
 * Takes a WOTS signature and an n-byte message, computes a WOTS public key.
 *
 * Writes the computed public key to 'pk'.
 */
static void wots_pk_from_sig(const xmss_params *params, unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const unsigned char *pub_seed, uint32_t addr[8])
{
    int *lengths = new int [params->wots_len];
    uint32_t i;

    chain_lengths(params, lengths, msg);

    for (i = 0; i < params->wots_len; i++) {
        set_chain_addr(addr, i);
        gen_chain(params, pk + i*params->n, sig + i*params->n,
                  lengths[i], params->wots_w - 1 - lengths[i], pub_seed, addr);
    }

	delete [] lengths;
}

//////////////////////////////////////////////////////////////////////
// xmss_commons.c

/**
 * Computes a leaf node from a WOTS public key using an L-tree.
 * Note that this destroys the used WOTS public key.
 */
static void l_tree(const xmss_params *params,
                   unsigned char *leaf, unsigned char *wots_pk,
                   const unsigned char *pub_seed, uint32_t addr[8])
{
    unsigned int l = params->wots_len;
    unsigned int parent_nodes;
    uint32_t i;
    uint32_t height = 0;

    set_tree_height(addr, height);

    while (l > 1) {
        parent_nodes = l >> 1;
        for (i = 0; i < parent_nodes; i++) {
            set_tree_index(addr, i);
            /* Hashes the nodes at (i*2)*params->n and (i*2)*params->n + 1 */
            thash_h(params, wots_pk + i*params->n,
                           wots_pk + (i*2)*params->n, pub_seed, addr);
        }
        /* If the row contained an odd number of nodes, the last node was not
           hashed. Instead, we pull it up to the next layer. */
        if (l & 1) {
            memcpy(wots_pk + (l >> 1)*params->n,
                   wots_pk + (l - 1)*params->n, params->n);
            l = (l >> 1) + 1;
        }
        else {
            l = l >> 1;
        }
        height++;
        set_tree_height(addr, height);
    }
    memcpy(leaf, wots_pk, params->n);
}

/**
 * Computes a root node given a leaf and an auth path
 */
static void compute_root(const xmss_params *params, unsigned char *root,
                         const unsigned char *leaf, unsigned long leafidx,
                         const unsigned char *auth_path,
                         const unsigned char *pub_seed, uint32_t addr[8])
{
    uint32_t i;
    unsigned char buffer[2 * MAX_N];	// [2*params->n];

    /* If leafidx is odd (last bit = 1), current path element is a right child
       and auth_path has to go left. Otherwise it is the other way around. */
    if (leafidx & 1) {
        memcpy(buffer + params->n, leaf, params->n);
        memcpy(buffer, auth_path, params->n);
    }
    else {
        memcpy(buffer, leaf, params->n);
        memcpy(buffer + params->n, auth_path, params->n);
    }
    auth_path += params->n;

    for (i = 0; i < params->tree_height - 1; i++) {
        set_tree_height(addr, i);
        leafidx >>= 1;
        set_tree_index(addr, leafidx);

        /* Pick the right or left neighbor, depending on parity of the node. */
        if (leafidx & 1) {
            thash_h(params, buffer + params->n, buffer, pub_seed, addr);
            memcpy(buffer, auth_path, params->n);
        }
        else {
            thash_h(params, buffer, buffer, pub_seed, addr);
            memcpy(buffer + params->n, auth_path, params->n);
        }
        auth_path += params->n;
    }

    /* The last iteration is exceptional; we do not copy an auth_path node. */
    set_tree_height(addr, params->tree_height - 1);
    leafidx >>= 1;
    set_tree_index(addr, leafidx);
    thash_h(params, root, buffer, pub_seed, addr);
}

/**
 * Used for pseudo-random key generation.
 * Generates the seed for the WOTS key pair at address 'addr'.
 *
 * Takes n-byte sk_seed and returns n-byte seed using 32 byte address 'addr'.
 */
static void get_seed(const xmss_params *params, unsigned char *seed,
              const unsigned char *sk_seed, uint32_t addr[8])
{
    unsigned char bytes[32];

    /* Make sure that chain addr, hash addr, and key bit are zeroed. */
    set_chain_addr(addr, 0);
    set_hash_addr(addr, 0);
    set_key_and_mask(addr, 0);

    /* Generate seed. */
    addr_to_bytes(bytes, addr);
    prf(params, seed, bytes, sk_seed);
}

/**
 * Computes the leaf at a given address. First generates the WOTS key pair,
 * then computes leaf using l_tree. As this happens position independent, we
 * only require that addr encodes the right ltree-address.
 */
static void gen_leaf_wots(const xmss_params *params, unsigned char *leaf,
                   const unsigned char *sk_seed, const unsigned char *pub_seed,
                   uint32_t ltree_addr[8], uint32_t ots_addr[8])
{
    unsigned char seed[MAX_N];		// [params->n];
    unsigned char *pk = new unsigned char [params->wots_sig_bytes];

    get_seed(params, seed, sk_seed, ots_addr);
    wots_pkgen(params, pk, seed, pub_seed, ots_addr);

    l_tree(params, leaf, pk, pub_seed, ltree_addr);

	delete [] pk;
}

/**
 * Verifies a given message signature pair under a given public key.
 * Note that this assumes a pk without an OID, i.e. [root || PUB_SEED]
 */
static int xmssmt_core_sign_open(const xmss_params *params,
                          unsigned char *m, unsigned long long *mlen,
                          const unsigned char *sm, unsigned long long smlen,
                          const unsigned char *pk)
{
    const unsigned char *pub_root = pk;
    const unsigned char *pub_seed = pk + params->n;
    unsigned char *wots_pk = new unsigned char [params->wots_sig_bytes];
    unsigned char leaf[MAX_N];		// [params->n];
    unsigned char root[MAX_N];		// [params->n];
    unsigned char *mhash = root;
    unsigned long long idx = 0;
    unsigned int i;
    uint32_t idx_leaf;

	uint32_t ots_addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t ltree_addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t node_addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    set_type(ots_addr, XMSS_ADDR_TYPE_OTS);
    set_type(ltree_addr, XMSS_ADDR_TYPE_LTREE);
    set_type(node_addr, XMSS_ADDR_TYPE_HASHTREE);

    *mlen = smlen - params->sig_bytes;

    /* Convert the index bytes from the signature to an integer. */
    idx = bytes_to_ull(sm, params->index_bytes);

    /* Put the message all the way at the end of the m buffer, so that we can
     * prepend the required other inputs for the hash function. */
    memcpy(m + params->sig_bytes, sm + params->sig_bytes, (size_t)*mlen);

    /* Compute the message hash. */
    hash_message(params, mhash, sm + params->index_bytes, pk, idx,
                 m + params->sig_bytes - 4*params->n, *mlen);
    sm += params->index_bytes + params->n;

    /* For each subtree.. */
    for (i = 0; i < params->d; i++) {
        idx_leaf = (idx & ((1 << params->tree_height)-1));
        idx = idx >> params->tree_height;

        set_layer_addr(ots_addr, i);
        set_layer_addr(ltree_addr, i);
        set_layer_addr(node_addr, i);

        set_tree_addr(ltree_addr, idx);
        set_tree_addr(ots_addr, idx);
        set_tree_addr(node_addr, idx);

        /* The WOTS public key is only correct if the signature was correct. */
        set_ots_addr(ots_addr, idx_leaf);
        /* Initially, root = mhash, but on subsequent iterations it is the root
           of the subtree below the currently processed subtree. */
        wots_pk_from_sig(params, wots_pk, sm, root, pub_seed, ots_addr);
        sm += params->wots_sig_bytes;

        /* Compute the leaf node using the WOTS public key. */
        set_ltree_addr(ltree_addr, idx_leaf);
        l_tree(params, leaf, wots_pk, pub_seed, ltree_addr);

        /* Compute the root node of this subtree. */
        compute_root(params, root, leaf, idx_leaf, sm, pub_seed, node_addr);
        sm += params->tree_height*params->n;
    }

    /* Check if the root node equals the root node in the public key. */
    if (memcmp(root, pub_root, params->n)) {
        /* If not, zero the message */
        memset(m, 0, (size_t)*mlen);
        *mlen = 0;
        return -1;
    }

    /* If verification was successful, copy the message from the signature. */
    memcpy(m, sm, (size_t)*mlen);

	delete [] wots_pk;

    return 0;
}

/**
 * Verifies a given message signature pair under a given public key.
 * Note that this assumes a pk without an OID, i.e. [root || PUB_SEED]
 */
static int xmss_core_sign_open(const xmss_params *params,
                        unsigned char *m, unsigned long long *mlen,
                        const unsigned char *sm, unsigned long long smlen,
                        const unsigned char *pk)
{
    /* XMSS signatures are fundamentally an instance of XMSSMT signatures.
       For d=1, as is the case with XMSS, some of the calls in the XMSSMT
       routine become vacuous (i.e. the loop only iterates once, and address
       management can be simplified a bit).*/
    return xmssmt_core_sign_open(params, m, mlen, sm, smlen, pk);
}

//////////////////////////////////////////////////////////////////////
// xmss_core_fast.c

typedef struct _treehash_inst {
    unsigned char h;
    unsigned long next_idx;
    unsigned char stackusage;
    unsigned char completed;
    unsigned char *node;
} treehash_inst;

typedef struct _bds_state {
    unsigned char *stack;
    unsigned int stackoffset;
    unsigned char *stacklevels;
    unsigned char *auth;
    unsigned char *keep;
    treehash_inst *treehash;
    unsigned char *retain;
    unsigned int next_leaf;
} bds_state;

/* These serialization functions provide a transition between the current
   way of storing the state in an exposed struct, and storing it as part of the
   byte array that is the secret key.
   They will probably be refactored in a non-backwards-compatible way, soon. */

static void xmssmt_serialize_state(const xmss_params *params,
                                   unsigned char *sk, bds_state *states)
{
    unsigned int i, j;

    /* Skip past the 'regular' sk */
    sk += params->index_bytes + 4*params->n;

    for (i = 0; i < 2*params->d - 1; i++) {
        sk += (params->tree_height + 1) * params->n; /* stack */

        ull_to_bytes(sk, 4, states[i].stackoffset);
        sk += 4;

        sk += params->tree_height + 1; /* stacklevels */
        sk += params->tree_height * params->n; /* auth */
        sk += (params->tree_height >> 1) * params->n; /* keep */

        for (j = 0; j < params->tree_height - params->bds_k; j++) {
            ull_to_bytes(sk, 1, states[i].treehash[j].h);
            sk += 1;

            ull_to_bytes(sk, 4, states[i].treehash[j].next_idx);
            sk += 4;

            ull_to_bytes(sk, 1, states[i].treehash[j].stackusage);
            sk += 1;

            ull_to_bytes(sk, 1, states[i].treehash[j].completed);
            sk += 1;

            sk += params->n; /* node */
        }

        /* retain */
        sk += ((1 << params->bds_k) - params->bds_k - 1) * params->n;

        ull_to_bytes(sk, 4, states[i].next_leaf);
        sk += 4;
    }
}

static void xmssmt_deserialize_state(const xmss_params *params,
                                     bds_state *states,
                                     unsigned char **wots_sigs,
                                     unsigned char *sk)
{
    unsigned int i, j;

    /* Skip past the 'regular' sk */
    sk += params->index_bytes + 4*params->n;

    // TODO These data sizes follow from the (former) test xmss_core_fast.c
    // TODO They should be reconsidered / motivated more explicitly

    for (i = 0; i < 2*params->d - 1; i++) {
        states[i].stack = sk;
        sk += (params->tree_height + 1) * params->n;

        states[i].stackoffset = (unsigned int)bytes_to_ull(sk, 4);
        sk += 4;

        states[i].stacklevels = sk;
        sk += params->tree_height + 1;

        states[i].auth = sk;
        sk += params->tree_height * params->n;

        states[i].keep = sk;
        sk += (params->tree_height >> 1) * params->n;

        for (j = 0; j < params->tree_height - params->bds_k; j++) {
            states[i].treehash[j].h = (unsigned char)bytes_to_ull(sk, 1);
            sk += 1;

            states[i].treehash[j].next_idx = (unsigned long)bytes_to_ull(sk, 4);
            sk += 4;

            states[i].treehash[j].stackusage = (unsigned char)bytes_to_ull(sk, 1);
            sk += 1;

            states[i].treehash[j].completed = (unsigned char)bytes_to_ull(sk, 1);
            sk += 1;

            states[i].treehash[j].node = sk;
            sk += params->n;
        }

        states[i].retain = sk;
        sk += ((1 << params->bds_k) - params->bds_k - 1) * params->n;

        states[i].next_leaf = (unsigned int)bytes_to_ull(sk, 4);
        sk += 4;
    }

    if (params->d > 1) {
        *wots_sigs = sk;
    }
}

static void xmss_serialize_state(const xmss_params *params,
                                 unsigned char *sk, bds_state *state)
{
    xmssmt_serialize_state(params, sk, state);
}

static void xmss_deserialize_state(const xmss_params *params,
                                   bds_state *state, unsigned char *sk)
{
    xmssmt_deserialize_state(params, state, NULL, sk);
}

static void memswap(void *a, void *b, void *t, unsigned long long len)
{
    memcpy(t, a, (size_t)len);
    memcpy(a, b, (size_t)len);
    memcpy(b, t, (size_t)len);
}

/**
 * Swaps the content of two bds_state objects, swapping actual memory rather
 * than pointers.
 * As we're mapping memory chunks in the secret key to bds state objects,
 * it is now necessary to make swaps 'real swaps'. This could be done in the
 * serialization function as well, but that causes more overhead
 */
// TODO this should not be necessary if we keep better track of the states
static void deep_state_swap(const xmss_params *params,
                            bds_state *a, bds_state *b)
{
    // TODO this is extremely ugly and should be refactored
    // TODO right now, this ensures that both 'stack' and 'retain' fit
    unsigned char *t = new unsigned char [
        ((params->tree_height + 1) > ((1 << params->bds_k) - params->bds_k - 1)
         ? (params->tree_height + 1)
         : ((1 << params->bds_k) - params->bds_k - 1))
        * params->n];
    unsigned int i;

    memswap(a->stack, b->stack, t, (params->tree_height + 1) * params->n);
    memswap(&a->stackoffset, &b->stackoffset, t, sizeof(a->stackoffset));
    memswap(a->stacklevels, b->stacklevels, t, params->tree_height + 1);
    memswap(a->auth, b->auth, t, params->tree_height * params->n);
    memswap(a->keep, b->keep, t, (params->tree_height >> 1) * params->n);

    for (i = 0; i < params->tree_height - params->bds_k; i++) {
        memswap(&a->treehash[i].h, &b->treehash[i].h, t, sizeof(a->treehash[i].h));
        memswap(&a->treehash[i].next_idx, &b->treehash[i].next_idx, t, sizeof(a->treehash[i].next_idx));
        memswap(&a->treehash[i].stackusage, &b->treehash[i].stackusage, t, sizeof(a->treehash[i].stackusage));
        memswap(&a->treehash[i].completed, &b->treehash[i].completed, t, sizeof(a->treehash[i].completed));
        memswap(a->treehash[i].node, b->treehash[i].node, t, params->n);
    }

    memswap(a->retain, b->retain, t, ((1 << params->bds_k) - params->bds_k - 1) * params->n);
    memswap(&a->next_leaf, &b->next_leaf, t, sizeof(a->next_leaf));

	delete [] t;
}

static int treehash_minheight_on_stack(const xmss_params *params,
                                       bds_state *state,
                                       const treehash_inst *treehash)
{
    unsigned int r = params->tree_height, i;

    for (i = 0; i < treehash->stackusage; i++) {
        if (state->stacklevels[state->stackoffset - i - 1] < r) {
            r = state->stacklevels[state->stackoffset - i - 1];
        }
    }
    return r;
}

/**
 * Merkle's TreeHash algorithm. The address only needs to initialize the first 78 bits of addr. Everything else will be set by treehash.
 * Currently only used for key generation.
 *
 */
static void treehash_init(const xmss_params *params,
                          unsigned char *node, int height, int index,
                          bds_state *state, const unsigned char *sk_seed,
                          const unsigned char *pub_seed, const uint32_t addr[8])
{
    unsigned int idx = index;
    // use three different addresses because at this point we use all three formats in parallel
    uint32_t ots_addr[8] = {0};
    uint32_t ltree_addr[8] = {0};
    uint32_t node_addr[8] = {0};
    // only copy layer and tree address parts
    copy_subtree_addr(ots_addr, addr);
    // type = ots
    set_type(ots_addr, 0);
    copy_subtree_addr(ltree_addr, addr);
    set_type(ltree_addr, 1);
    copy_subtree_addr(node_addr, addr);
    set_type(node_addr, 2);

    uint32_t lastnode, i;
    unsigned char *stack = new unsigned char [(height+1)*params->n];
    unsigned int *stacklevels = new unsigned int [height+1];
    unsigned int stackoffset=0;
    unsigned int nodeh;

    lastnode = idx+(1<<height);

    for (i = 0; i < params->tree_height-params->bds_k; i++) {
        state->treehash[i].h = (unsigned char)i;
        state->treehash[i].completed = 1;
        state->treehash[i].stackusage = 0;
    }

    i = 0;
    for (; idx < lastnode; idx++) {
        set_ltree_addr(ltree_addr, idx);
        set_ots_addr(ots_addr, idx);
        gen_leaf_wots(params, stack+stackoffset*params->n, sk_seed, pub_seed, ltree_addr, ots_addr);
        stacklevels[stackoffset] = 0;
        stackoffset++;
        if (params->tree_height - params->bds_k > 0 && i == 3) {
            memcpy(state->treehash[0].node, stack+stackoffset*params->n, params->n);
        }
        while (stackoffset>1 && stacklevels[stackoffset-1] == stacklevels[stackoffset-2]) {
            nodeh = stacklevels[stackoffset-1];
            if (i >> nodeh == 1) {
                memcpy(state->auth + nodeh*params->n, stack+(stackoffset-1)*params->n, params->n);
            }
            else {
                if (nodeh < params->tree_height - params->bds_k && i >> nodeh == 3) {
                    memcpy(state->treehash[nodeh].node, stack+(stackoffset-1)*params->n, params->n);
                }
                else if (nodeh >= params->tree_height - params->bds_k) {
                    memcpy(state->retain + ((1 << (params->tree_height - 1 - nodeh)) + nodeh - params->tree_height + (((i >> nodeh) - 3) >> 1)) * params->n, stack+(stackoffset-1)*params->n, params->n);
                }
            }
            set_tree_height(node_addr, stacklevels[stackoffset-1]);
            set_tree_index(node_addr, (idx >> (stacklevels[stackoffset-1]+1)));
            thash_h(params, stack+(stackoffset-2)*params->n, stack+(stackoffset-2)*params->n, pub_seed, node_addr);
            stacklevels[stackoffset-2]++;
            stackoffset--;
        }
        i++;
    }

    for (i = 0; i < params->n; i++) {
        node[i] = stack[i];
    }

	delete [] stack;
	delete [] stacklevels;
}

static void treehash_update(const xmss_params *params,
                            treehash_inst *treehash, bds_state *state,
                            const unsigned char *sk_seed,
                            const unsigned char *pub_seed,
                            const uint32_t addr[8])
{
    uint32_t ots_addr[8] = {0};
    uint32_t ltree_addr[8] = {0};
    uint32_t node_addr[8] = {0};
    // only copy layer and tree address parts
    copy_subtree_addr(ots_addr, addr);
    // type = ots
    set_type(ots_addr, 0);
    copy_subtree_addr(ltree_addr, addr);
    set_type(ltree_addr, 1);
    copy_subtree_addr(node_addr, addr);
    set_type(node_addr, 2);

    set_ltree_addr(ltree_addr, treehash->next_idx);
    set_ots_addr(ots_addr, treehash->next_idx);

    unsigned char nodebuffer[2 * MAX_N];	// [2 * params->n];
    unsigned int nodeheight = 0;

    gen_leaf_wots(params, nodebuffer, sk_seed, pub_seed, ltree_addr, ots_addr);
    while (treehash->stackusage > 0 && state->stacklevels[state->stackoffset-1] == nodeheight) {
        memcpy(nodebuffer + params->n, nodebuffer, params->n);
        memcpy(nodebuffer, state->stack + (state->stackoffset-1)*params->n, params->n);
        set_tree_height(node_addr, nodeheight);
        set_tree_index(node_addr, (treehash->next_idx >> (nodeheight+1)));
        thash_h(params, nodebuffer, nodebuffer, pub_seed, node_addr);
        nodeheight++;
        treehash->stackusage--;
        state->stackoffset--;
    }
    if (nodeheight == treehash->h) { // this also implies stackusage == 0
        memcpy(treehash->node, nodebuffer, params->n);
        treehash->completed = 1;
    }
    else {
        memcpy(state->stack + state->stackoffset*params->n, nodebuffer, params->n);
        treehash->stackusage++;
        state->stacklevels[state->stackoffset] = nodeheight;
        state->stackoffset++;
        treehash->next_idx++;
    }
}

/**
 * Performs treehash updates on the instance that needs it the most.
 * Returns the updated number of available updates.
 **/
static char bds_treehash_update(const xmss_params *params,
                                bds_state *state, unsigned int updates,
                                const unsigned char *sk_seed,
                                unsigned char *pub_seed,
                                const uint32_t addr[8])
{
    uint32_t i, j;
    unsigned int level, l_min, low;
    unsigned int used = 0;

    for (j = 0; j < updates; j++) {
        l_min = params->tree_height;
        level = params->tree_height - params->bds_k;
        for (i = 0; i < params->tree_height - params->bds_k; i++) {
            if (state->treehash[i].completed) {
                low = params->tree_height;
            }
            else if (state->treehash[i].stackusage == 0) {
                low = i;
            }
            else {
                low = treehash_minheight_on_stack(params, state, &(state->treehash[i]));
            }
            if (low < l_min) {
                level = i;
                l_min = low;
            }
        }
        if (level == params->tree_height - params->bds_k) {
            break;
        }
        treehash_update(params, &(state->treehash[level]), state, sk_seed, pub_seed, addr);
        used++;
    }
    return updates - used;
}

/**
 * Updates the state (typically NEXT_i) by adding a leaf and updating the stack
 * Returns -1 if all leaf nodes have already been processed
 **/
static char bds_state_update(const xmss_params *params,
                             bds_state *state, const unsigned char *sk_seed,
                             const unsigned char *pub_seed,
                             const uint32_t addr[8])
{
    uint32_t ltree_addr[8] = {0};
    uint32_t node_addr[8] = {0};
    uint32_t ots_addr[8] = {0};

    unsigned int nodeh;
    int idx = state->next_leaf;
    if (idx == 1 << params->tree_height) {
        return -1;
    }

    // only copy layer and tree address parts
    copy_subtree_addr(ots_addr, addr);
    // type = ots
    set_type(ots_addr, 0);
    copy_subtree_addr(ltree_addr, addr);
    set_type(ltree_addr, 1);
    copy_subtree_addr(node_addr, addr);
    set_type(node_addr, 2);

    set_ots_addr(ots_addr, idx);
    set_ltree_addr(ltree_addr, idx);

    gen_leaf_wots(params, state->stack+state->stackoffset*params->n, sk_seed, pub_seed, ltree_addr, ots_addr);

    state->stacklevels[state->stackoffset] = 0;
    state->stackoffset++;
    if (params->tree_height - params->bds_k > 0 && idx == 3) {
        memcpy(state->treehash[0].node, state->stack+state->stackoffset*params->n, params->n);
    }
    while (state->stackoffset>1 && state->stacklevels[state->stackoffset-1] == state->stacklevels[state->stackoffset-2]) {
        nodeh = state->stacklevels[state->stackoffset-1];
        if (idx >> nodeh == 1) {
            memcpy(state->auth + nodeh*params->n, state->stack+(state->stackoffset-1)*params->n, params->n);
        }
        else {
            if (nodeh < params->tree_height - params->bds_k && idx >> nodeh == 3) {
                memcpy(state->treehash[nodeh].node, state->stack+(state->stackoffset-1)*params->n, params->n);
            }
            else if (nodeh >= params->tree_height - params->bds_k) {
                memcpy(state->retain + ((1 << (params->tree_height - 1 - nodeh)) + nodeh - params->tree_height + (((idx >> nodeh) - 3) >> 1)) * params->n, state->stack+(state->stackoffset-1)*params->n, params->n);
            }
        }
        set_tree_height(node_addr, state->stacklevels[state->stackoffset-1]);
        set_tree_index(node_addr, (idx >> (state->stacklevels[state->stackoffset-1]+1)));
        thash_h(params, state->stack+(state->stackoffset-2)*params->n, state->stack+(state->stackoffset-2)*params->n, pub_seed, node_addr);

        state->stacklevels[state->stackoffset-2]++;
        state->stackoffset--;
    }
    state->next_leaf++;
    return 0;
}

/**
 * Returns the auth path for node leaf_idx and computes the auth path for the
 * next leaf node, using the algorithm described by Buchmann, Dahmen and Szydlo
 * in "Post Quantum Cryptography", Springer 2009.
 */
static void bds_round(const xmss_params *params,
                      bds_state *state, const unsigned long leaf_idx,
                      const unsigned char *sk_seed,
                      const unsigned char *pub_seed, uint32_t addr[8])
{
    unsigned int i;
    unsigned int tau = params->tree_height;
    unsigned int startidx;
    unsigned int offset, rowidx;
    unsigned char buf[2 * MAX_N];	// [2 * params->n];

    uint32_t ots_addr[8] = {0};
    uint32_t ltree_addr[8] = {0};
    uint32_t node_addr[8] = {0};

    // only copy layer and tree address parts
    copy_subtree_addr(ots_addr, addr);
    // type = ots
    set_type(ots_addr, 0);
    copy_subtree_addr(ltree_addr, addr);
    set_type(ltree_addr, 1);
    copy_subtree_addr(node_addr, addr);
    set_type(node_addr, 2);

    for (i = 0; i < params->tree_height; i++) {
        if (! ((leaf_idx >> i) & 1)) {
            tau = i;
            break;
        }
    }

    if (tau > 0) {
        memcpy(buf, state->auth + (tau-1) * params->n, params->n);
        // we need to do this before refreshing state->keep to prevent overwriting
        memcpy(buf + params->n, state->keep + ((tau-1) >> 1) * params->n, params->n);
    }
    if (!((leaf_idx >> (tau + 1)) & 1) && (tau < params->tree_height - 1)) {
        memcpy(state->keep + (tau >> 1)*params->n, state->auth + tau*params->n, params->n);
    }
    if (tau == 0) {
        set_ltree_addr(ltree_addr, leaf_idx);
        set_ots_addr(ots_addr, leaf_idx);
        gen_leaf_wots(params, state->auth, sk_seed, pub_seed, ltree_addr, ots_addr);
    }
    else {
        set_tree_height(node_addr, (tau-1));
        set_tree_index(node_addr, leaf_idx >> tau);
        thash_h(params, state->auth + tau * params->n, buf, pub_seed, node_addr);
        for (i = 0; i < tau; i++) {
            if (i < params->tree_height - params->bds_k) {
                memcpy(state->auth + i * params->n, state->treehash[i].node, params->n);
            }
            else {
                offset = (1 << (params->tree_height - 1 - i)) + i - params->tree_height;
                rowidx = ((leaf_idx >> i) - 1) >> 1;
                memcpy(state->auth + i * params->n, state->retain + (offset + rowidx) * params->n, params->n);
            }
        }

        for (i = 0; i < ((tau < params->tree_height - params->bds_k) ? tau : (params->tree_height - params->bds_k)); i++) {
            startidx = leaf_idx + 1 + 3 * (1 << i);
            if (startidx < 1U << params->tree_height) {
                state->treehash[i].h = i;
                state->treehash[i].next_idx = startidx;
                state->treehash[i].completed = 0;
                state->treehash[i].stackusage = 0;
            }
        }
    }
}

/*
 * Generates a XMSS key pair for a given parameter set.
 * Format sk: [(32bit) idx || SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [root || PUB_SEED] omitting algo oid.
 */
static int xmss_core_keypair(const xmss_params *params,
                      unsigned char *pk, unsigned char *sk)
{
    uint32_t addr[8] = {0};

    // TODO refactor BDS state not to need separate treehash instances
    bds_state state;
    treehash_inst *treehash = new treehash_inst [params->tree_height - params->bds_k];
    state.treehash = treehash;

    xmss_deserialize_state(params, &state, sk);

    state.stackoffset = 0;
    state.next_leaf = 0;

    // Set idx = 0
    sk[0] = 0;
    sk[1] = 0;
    sk[2] = 0;
    sk[3] = 0;
    // Init SK_SEED (n byte), SK_PRF (n byte), and PUB_SEED (n byte)
    randombytes(sk + params->index_bytes, 3*params->n);
    // Copy PUB_SEED to public key
    memcpy(pk + params->n, sk + params->index_bytes + 2*params->n, params->n);

    // Compute root
    treehash_init(params, pk, params->tree_height, 0, &state, sk + params->index_bytes, sk + params->index_bytes + 2*params->n, addr);
    // copy root o sk
    memcpy(sk + params->index_bytes + 3*params->n, pk, params->n);

    /* Write the BDS state into sk. */
    xmss_serialize_state(params, sk, &state);

	delete [] treehash;

    return 0;
}

/**
 * Signs a message.
 * Returns
 * 1. an array containing the signature followed by the message AND
 * 2. an updated secret key!
 *
 */
static int xmss_core_sign(const xmss_params *params,
                   unsigned char *sk,
                   unsigned char *sm, unsigned long long *smlen,
                   const unsigned char *m, unsigned long long mlen)
{
    const unsigned char *pub_root = sk + params->index_bytes + 3*params->n;

    uint16_t i = 0;

    // TODO refactor BDS state not to need separate treehash instances
    bds_state state;
    treehash_inst *treehash = new treehash_inst [params->tree_height - params->bds_k];
    state.treehash = treehash;

    /* Load the BDS state from sk. */
    xmss_deserialize_state(params, &state, sk);

    // Extract SK
    unsigned long idx = ((unsigned long)sk[0] << 24) | ((unsigned long)sk[1] << 16) | ((unsigned long)sk[2] << 8) | sk[3];
    unsigned char sk_seed[MAX_N];		// [params->n];
    memcpy(sk_seed, sk + params->index_bytes, params->n);
    unsigned char sk_prf[MAX_N];		// [params->n];
    memcpy(sk_prf, sk + params->index_bytes + params->n, params->n);
    unsigned char pub_seed[MAX_N];		// [params->n];
    memcpy(pub_seed, sk + params->index_bytes + 2*params->n, params->n);

    // index as 32 bytes string
    unsigned char idx_bytes_32[32];
    ull_to_bytes(idx_bytes_32, 32, idx);

    // Update SK
    sk[0] = ((idx + 1) >> 24) & 255;
    sk[1] = ((idx + 1) >> 16) & 255;
    sk[2] = ((idx + 1) >> 8) & 255;
    sk[3] = (idx + 1) & 255;
    // Secret key for this non-forward-secure version is now updated.
    // A production implementation should consider using a file handle instead,
    //  and write the updated secret key at this point!

    // Init working params
    unsigned char R[MAX_N];			// [params->n];
    unsigned char msg_h[MAX_N];		// [params->n];
    unsigned char ots_seed[MAX_N];	// [params->n];
    uint32_t ots_addr[8] = {0};

    // ---------------------------------
    // Message Hashing
    // ---------------------------------

    // Message Hash:
    // First compute pseudorandom value
    prf(params, R, idx_bytes_32, sk_prf);

    /* Already put the message in the right place, to make it easier to prepend
     * things when computing the hash over the message. */
    memcpy(sm + params->sig_bytes, m, (size_t)mlen);

    /* Compute the message hash. */
    hash_message(params, msg_h, R, pub_root, idx,
                 sm + params->sig_bytes - 4*params->n, mlen);

    // Start collecting signature
    *smlen = 0;

    // Copy index to signature
    sm[0] = (idx >> 24) & 255;
    sm[1] = (idx >> 16) & 255;
    sm[2] = (idx >> 8) & 255;
    sm[3] = idx & 255;

    sm += 4;
    *smlen += 4;

    // Copy R to signature
    for (i = 0; i < params->n; i++) {
        sm[i] = R[i];
    }

    sm += params->n;
    *smlen += params->n;

    // ----------------------------------
    // Now we start to "really sign"
    // ----------------------------------

    // Prepare Address
    set_type(ots_addr, 0);
    set_ots_addr(ots_addr, idx);

    // Compute seed for OTS key pair
    get_seed(params, ots_seed, sk_seed, ots_addr);

    // Compute WOTS signature
    wots_sign(params, sm, msg_h, ots_seed, pub_seed, ots_addr);

    sm += params->wots_sig_bytes;
    *smlen += params->wots_sig_bytes;

    // the auth path was already computed during the previous round
    memcpy(sm, state.auth, params->tree_height*params->n);

    if (idx < (1U << params->tree_height) - 1) {
        bds_round(params, &state, idx, sk_seed, pub_seed, ots_addr);
        bds_treehash_update(params, &state, (params->tree_height - params->bds_k) >> 1, sk_seed, pub_seed, ots_addr);
    }

    sm += params->tree_height*params->n;
    *smlen += params->tree_height*params->n;

    memcpy(sm, m, (size_t)mlen);
    *smlen += mlen;

    /* Write the updated BDS state back into sk. */
    xmss_serialize_state(params, sk, &state);

	delete [] treehash;

	return 0;
}

/**
 * Given a set of parameters, this function returns the size of the secret key.
 * This is implementation specific, as varying choices in tree traversal will
 * result in varying requirements for state storage.
 */
static unsigned long long xmssmt_core_sk_bytes(const xmss_params *params)
{
    return params->index_bytes + 4 * params->n
            + (2 * params->d - 1) * (
                (params->tree_height + 1) * params->n
                + 4
                + params->tree_height + 1
                + params->tree_height * params->n
                + (params->tree_height >> 1) * params->n
                + (params->tree_height - params->bds_k) * (7 + params->n)
                + ((1 << params->bds_k) - params->bds_k - 1) * params->n
                + 4
             )
            + (params->d - 1) * params->wots_sig_bytes;
}

/**
 * Given a set of parameters, this function returns the size of the secret key.
 * This is implementation specific, as varying choices in tree traversal will
 * result in varying requirements for state storage.
 */
static unsigned long long xmss_core_sk_bytes(const xmss_params *params)
{
    return xmssmt_core_sk_bytes(params);
}

/*
 * Generates a XMSSMT key pair for a given parameter set.
 * Format sk: [(ceil(h/8) bit) idx || SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [root || PUB_SEED] omitting algo oid.
 */
static int xmssmt_core_keypair(const xmss_params *params,
                        unsigned char *pk, unsigned char *sk)
{
    unsigned char ots_seed[MAX_N];	// [params->n];
    uint32_t addr[8] = {0};
    unsigned int i;
    unsigned char *wots_sigs;

    // TODO refactor BDS state not to need separate treehash instances
    bds_state states[2 * MAX_D - 1];	// [2*params->d - 1];
    treehash_inst *treehash = new treehash_inst [(2*params->d - 1) * (params->tree_height - params->bds_k)];
    for (i = 0; i < 2*params->d - 1; i++) {
        states[i].treehash = treehash + i * (params->tree_height - params->bds_k);
    }

    xmssmt_deserialize_state(params, states, &wots_sigs, sk);

    for (i = 0; i < 2 * params->d - 1; i++) {
        states[i].stackoffset = 0;
        states[i].next_leaf = 0;
    }

    // Set idx = 0
    for (i = 0; i < params->index_bytes; i++) {
        sk[i] = 0;
    }
    // Init SK_SEED (params->n byte), SK_PRF (params->n byte), and PUB_SEED (params->n byte)
    randombytes(sk+params->index_bytes, 3*params->n);
    // Copy PUB_SEED to public key
    memcpy(pk+params->n, sk+params->index_bytes+2*params->n, params->n);

    // Start with the bottom-most layer
    set_layer_addr(addr, 0);
    // Set up state and compute wots signatures for all but topmost tree root
    for (i = 0; i < params->d - 1; i++) {
        // Compute seed for OTS key pair
        treehash_init(params, pk, params->tree_height, 0, states + i, sk+params->index_bytes, pk+params->n, addr);
        set_layer_addr(addr, (i+1));
        get_seed(params, ots_seed, sk + params->index_bytes, addr);
        wots_sign(params, wots_sigs + i*params->wots_sig_bytes, pk, ots_seed, pk+params->n, addr);
    }
    // Address now points to the single tree on layer d-1
    treehash_init(params, pk, params->tree_height, 0, states + i, sk+params->index_bytes, pk+params->n, addr);
    memcpy(sk + params->index_bytes + 3*params->n, pk, params->n);

    xmssmt_serialize_state(params, sk, states);

	delete [] treehash;

    return 0;
}

/**
 * Signs a message.
 * Returns
 * 1. an array containing the signature followed by the message AND
 * 2. an updated secret key!
 *
 */
static int xmssmt_core_sign(const xmss_params *params,
                     unsigned char *sk,
                     unsigned char *sm, unsigned long long *smlen,
                     const unsigned char *m, unsigned long long mlen)
{
    const unsigned char *pub_root = sk + params->index_bytes + 3*params->n;

    uint64_t idx_tree;
    uint32_t idx_leaf;
    uint64_t i, j;
    int needswap_upto = -1;
    unsigned int updates;

    unsigned char sk_seed[MAX_N];	// [params->n];
    unsigned char sk_prf[MAX_N];	// [params->n];
    unsigned char pub_seed[MAX_N];	// [params->n];
    // Init working params
    unsigned char R[MAX_N];			// [params->n];
    unsigned char msg_h[MAX_N];		// [params->n];
    unsigned char ots_seed[MAX_N];	// [params->n];
    uint32_t addr[8] = {0};
    uint32_t ots_addr[8] = {0};
    unsigned char idx_bytes_32[32];

    unsigned char *wots_sigs;

    // TODO refactor BDS state not to need separate treehash instances
    bds_state states[2 * MAX_D - 1];	// [2*params->d - 1];
    treehash_inst *treehash = new treehash_inst [(2*params->d - 1) * (params->tree_height - params->bds_k)];
    for (i = 0; i < 2*params->d - 1; i++) {
        states[i].treehash = treehash + i * (params->tree_height - params->bds_k);
    }

    xmssmt_deserialize_state(params, states, &wots_sigs, sk);

    // Extract SK
    unsigned long long idx = 0;
    for (i = 0; i < params->index_bytes; i++) {
        idx |= ((unsigned long long)sk[i]) << 8*(params->index_bytes - 1 - i);
    }

    memcpy(sk_seed, sk+params->index_bytes, params->n);
    memcpy(sk_prf, sk+params->index_bytes+params->n, params->n);
    memcpy(pub_seed, sk+params->index_bytes+2*params->n, params->n);

    // Update SK
    for (i = 0; i < params->index_bytes; i++) {
        sk[i] = ((idx + 1) >> 8*(params->index_bytes - 1 - i)) & 255;
    }
    // Secret key for this non-forward-secure version is now updated.
    // A production implementation should consider using a file handle instead,
    //  and write the updated secret key at this point!

    // ---------------------------------
    // Message Hashing
    // ---------------------------------

    // Message Hash:
    // First compute pseudorandom value
    ull_to_bytes(idx_bytes_32, 32, idx);
    prf(params, R, idx_bytes_32, sk_prf);

    /* Already put the message in the right place, to make it easier to prepend
     * things when computing the hash over the message. */
    memcpy(sm + params->sig_bytes, m, (size_t)mlen);

    /* Compute the message hash. */
    hash_message(params, msg_h, R, pub_root, idx,
                 sm + params->sig_bytes - 4*params->n, mlen);

    // Start collecting signature
    *smlen = 0;

    // Copy index to signature
    for (i = 0; i < params->index_bytes; i++) {
        sm[i] = (idx >> 8*(params->index_bytes - 1 - i)) & 255;
    }

    sm += params->index_bytes;
    *smlen += params->index_bytes;

    // Copy R to signature
    for (i = 0; i < params->n; i++) {
        sm[i] = R[i];
    }

    sm += params->n;
    *smlen += params->n;

    // ----------------------------------
    // Now we start to "really sign"
    // ----------------------------------

    // Handle lowest layer separately as it is slightly different...

    // Prepare Address
    set_type(ots_addr, 0);
    idx_tree = idx >> params->tree_height;
    idx_leaf = (idx & ((1 << params->tree_height)-1));
    set_layer_addr(ots_addr, 0);
    set_tree_addr(ots_addr, idx_tree);
    set_ots_addr(ots_addr, idx_leaf);

    // Compute seed for OTS key pair
    get_seed(params, ots_seed, sk_seed, ots_addr);

    // Compute WOTS signature
    wots_sign(params, sm, msg_h, ots_seed, pub_seed, ots_addr);

    sm += params->wots_sig_bytes;
    *smlen += params->wots_sig_bytes;

    memcpy(sm, states[0].auth, params->tree_height*params->n);
    sm += params->tree_height*params->n;
    *smlen += params->tree_height*params->n;

    // prepare signature of remaining layers
    for (i = 1; i < params->d; i++) {
        // put WOTS signature in place
        memcpy(sm, wots_sigs + (i-1)*params->wots_sig_bytes, params->wots_sig_bytes);

        sm += params->wots_sig_bytes;
        *smlen += params->wots_sig_bytes;

        // put AUTH nodes in place
        memcpy(sm, states[i].auth, params->tree_height*params->n);
        sm += params->tree_height*params->n;
        *smlen += params->tree_height*params->n;
    }

    updates = (params->tree_height - params->bds_k) >> 1;

    set_tree_addr(addr, (idx_tree + 1));
    // mandatory update for NEXT_0 (does not count towards h-k/2) if NEXT_0 exists
    if ((1 + idx_tree) * (1ULL << params->tree_height) + idx_leaf < (1ULL << params->full_height)) {
        bds_state_update(params, &states[params->d], sk_seed, pub_seed, addr);
    }

    for (i = 0; i < params->d; i++) {
        // check if we're not at the end of a tree
        if (! (((idx + 1) & ((1ULL << ((i+1)*params->tree_height)) - 1)) == 0)) {
            idx_leaf = (idx >> (params->tree_height * i)) & ((1 << params->tree_height)-1);
            idx_tree = (idx >> (params->tree_height * (i+1)));
            set_layer_addr(addr, (uint32_t)i);
            set_tree_addr(addr, idx_tree);
            if (i == (unsigned int) (needswap_upto + 1)) {
                bds_round(params, &states[i], idx_leaf, sk_seed, pub_seed, addr);
            }
            updates = bds_treehash_update(params, &states[i], updates, sk_seed, pub_seed, addr);
            set_tree_addr(addr, (idx_tree + 1));
            // if a NEXT-tree exists for this level;
            if ((1 + idx_tree) * (1ULL << params->tree_height) + idx_leaf < (1ULL << (params->full_height - params->tree_height * i))) {
                if (i > 0 && updates > 0 && states[params->d + i].next_leaf < (1ULL << params->full_height)) {
                    bds_state_update(params, &states[params->d + i], sk_seed, pub_seed, addr);
                    updates--;
                }
            }
        }
        else if (idx < (1ULL << params->full_height) - 1) {
            deep_state_swap(params, states+params->d + i, states + i);

            set_layer_addr(ots_addr, (uint32_t)(i+1));
            set_tree_addr(ots_addr, ((idx + 1) >> ((i+2) * params->tree_height)));
            set_ots_addr(ots_addr, (((idx >> ((i+1) * params->tree_height)) + 1) & ((1 << params->tree_height)-1)));

            get_seed(params, ots_seed, sk+params->index_bytes, ots_addr);
            wots_sign(params, wots_sigs + i*params->wots_sig_bytes, states[i].stack, ots_seed, pub_seed, ots_addr);

            states[params->d + i].stackoffset = 0;
            states[params->d + i].next_leaf = 0;

            updates--; // WOTS-signing counts as one update
            needswap_upto = (int)i;
            for (j = 0; j < params->tree_height-params->bds_k; j++) {
                states[i].treehash[j].completed = 1;
            }
        }
    }

    memcpy(sm, m, (size_t)mlen);
    *smlen += mlen;

    xmssmt_serialize_state(params, sk, states);
		
	delete [] treehash;

    return 0;
}

//////////////////////////////////////////////////////////////////////
// params.c

static struct _xmss_name_tab {
	char *name;
	uint32_t oid;
} xmss_name_tab[] = {
	{	"XMSS_SHA2-256_W16_H10",	0x00000001	},	// openssh/sshkey-xmss.h
	{	"XMSS_SHA2-256_W16_H16",	0x00000002	},
	{	"XMSS_SHA2-256_W16_H20",	0x00000003	},
	
    {	"XMSS-SHA2_10_256",			0x00000001	},
    {	"XMSS-SHA2_16_256",			0x00000002	},
    {	"XMSS-SHA2_20_256",			0x00000003	},
    {	"XMSS-SHA2_10_512",			0x00000004	},
    {	"XMSS-SHA2_16_512",			0x00000005	},
    {	"XMSS-SHA2_20_512",			0x00000006	},
    {	"XMSS-SHAKE_10_256",		0x00000007	},
    {	"XMSS-SHAKE_16_256",		0x00000008	},
    {	"XMSS-SHAKE_20_256",		0x00000009	},
    {	"XMSS-SHAKE_10_512",		0x0000000a	},
    {	"XMSS-SHAKE_16_512",		0x0000000b	},
    {	"XMSS-SHAKE_20_512",		0x0000000c	},
	{	NULL,						0x00000000	},
}, xmssmt_name_tab[] = {
	{	"XMSSMT-SHA2_20/2_256",		0x00000001	},
    {	"XMSSMT-SHA2_20/4_256",		0x00000002	},
    {	"XMSSMT-SHA2_40/2_256",		0x00000003	},
    {	"XMSSMT-SHA2_40/4_256",		0x00000004	},
    {	"XMSSMT-SHA2_40/8_256",		0x00000005	},
    {	"XMSSMT-SHA2_60/3_256",		0x00000006	},
    {	"XMSSMT-SHA2_60/6_256",		0x00000007	},
    {	"XMSSMT-SHA2_60/12_256",	0x00000008	},
    {	"XMSSMT-SHA2_20/2_512",		0x00000009	},
    {	"XMSSMT-SHA2_20/4_512",		0x0000000a	},
    {	"XMSSMT-SHA2_40/2_512",		0x0000000b	},
    {	"XMSSMT-SHA2_40/4_512",		0x0000000c	},
    {	"XMSSMT-SHA2_40/8_512",		0x0000000d	},
    {	"XMSSMT-SHA2_60/3_512",		0x0000000e	},
    {	"XMSSMT-SHA2_60/6_512",		0x0000000f	},
    {	"XMSSMT-SHA2_60/12_512",	0x00000010	},
    {	"XMSSMT-SHAKE_20/2_256",	0x00000011	},
    {	"XMSSMT-SHAKE_20/4_256",	0x00000012	},
    {	"XMSSMT-SHAKE_40/2_256",	0x00000013	},
    {	"XMSSMT-SHAKE_40/4_256",	0x00000014	},
    {	"XMSSMT-SHAKE_40/8_256",	0x00000015	},
    {	"XMSSMT-SHAKE_60/3_256",	0x00000016	},
    {	"XMSSMT-SHAKE_60/6_256",	0x00000017	},
    {	"XMSSMT-SHAKE_60/12_256",	0x00000018	},
    {	"XMSSMT-SHAKE_20/2_512",	0x00000019	},
    {	"XMSSMT-SHAKE_20/4_512",	0x0000001a	},
    {	"XMSSMT-SHAKE_40/2_512",	0x0000001b	},
    {	"XMSSMT-SHAKE_40/4_512",	0x0000001c	},
    {	"XMSSMT-SHAKE_40/8_512",	0x0000001d	},
    {	"XMSSMT-SHAKE_60/3_512",	0x0000001e	},
    {	"XMSSMT-SHAKE_60/6_512",	0x0000001f	},
    {	"XMSSMT-SHAKE_60/12_512",	0x00000020	},
	{	NULL,						0x00000000	},
};

int xmss_str_to_oid(uint32_t *oid, const char *s)
{
	int n;

	for ( n = 0 ; xmss_name_tab[n].name != NULL ; n++ ) {
		if ( !strcmp(s, xmss_name_tab[n].name) ) {
			*oid = xmss_name_tab[n].oid;
			return 0;
		}
	}
    return -1;
}
const char *xmss_oid_to_str(uint32_t oid)
{
	int n;

	oid &= 0x000000FF;

	for ( n = 0 ; xmss_name_tab[n].name != NULL ; n++ ) {
		if ( oid == xmss_name_tab[n].oid )
			return xmss_name_tab[n].name;
	}
    return "";
}

int xmssmt_str_to_oid(uint32_t *oid, const char *s)
{
	int n;

	for ( n = 0 ; xmssmt_name_tab[n].name != NULL ; n++ ) {
		if ( !strcmp(s, xmssmt_name_tab[n].name) ) {
			*oid = xmssmt_name_tab[n].oid;
			return 0;
		}
	}
    return -1;
}
const char *xmssmt_oid_to_str(uint32_t oid)
{
	int n;

	oid &= 0x000000FF;

	for ( n = 0 ; xmssmt_name_tab[n].name != NULL ; n++ ) {
		if ( oid == xmssmt_name_tab[n].oid )
			return xmssmt_name_tab[n].name;
	}
    return "";
}

int xmss_parse_oid(xmss_params *params, const uint32_t oid)
{
    switch (oid) {
        case 0x00000001:
        case 0x00000002:
        case 0x00000003:
        case 0x00000004:
        case 0x00000005:
        case 0x00000006:
            params->func = XMSS_SHA2;
            break;

        case 0x00000007:
        case 0x00000008:
        case 0x00000009:
        case 0x0000000a:
        case 0x0000000b:
        case 0x0000000c:
            params->func = XMSS_SHAKE;
            break;

        default:
            return -1;
    }
    switch (oid) {
        case 0x00000001:
        case 0x00000002:
        case 0x00000003:

        case 0x00000007:
        case 0x00000008:
        case 0x00000009:
            params->n = 32;
            break;

        case 0x00000004:
        case 0x00000005:
        case 0x00000006:

        case 0x0000000a:
        case 0x0000000b:
        case 0x0000000c:
            params->n = 64;
            break;

        default:
            return -1;
    }
    switch (oid) {
        case 0x00000001:
        case 0x00000004:
        case 0x00000007:
        case 0x0000000a:
            params->full_height = 10;
            break;

        case 0x00000002:
        case 0x00000005:
        case 0x00000008:
        case 0x0000000b:
            params->full_height = 16;
            break;

        case 0x00000003:
        case 0x00000006:
        case 0x00000009:
        case 0x0000000c:
            params->full_height = 20;

            break;
        default:
            return -1;
    }
    params->d = 1;
    params->tree_height = params->full_height  / params->d;
    params->wots_w = 16;
    params->wots_log_w = 4;
    params->wots_len1 = 8 * params->n / params->wots_log_w;
    /* len_2 = floor(log(len_1 * (w - 1)) / log(w)) + 1 */
    params->wots_len2 = 3;
    params->wots_len = params->wots_len1 + params->wots_len2;
    params->wots_sig_bytes = params->wots_len * params->n;
    params->index_bytes = 4;
    params->sig_bytes = (params->index_bytes + params->n
                         + params->d * params->wots_sig_bytes
                         + params->full_height * params->n);

    // TODO figure out sensible and legal values for this based on the above
    //params->bds_k = 0;

	// openssh k=2
    params->bds_k = 2;

    params->pk_bytes = 2 * params->n;
    params->sk_bytes = xmss_core_sk_bytes(params);
    return 0;
}

int xmssmt_parse_oid(xmss_params *params, const uint32_t oid)
{
    switch (oid) {
        case 0x00000001:
        case 0x00000002:
        case 0x00000003:
        case 0x00000004:
        case 0x00000005:
        case 0x00000006:
        case 0x00000007:
        case 0x00000008:
        case 0x00000009:
        case 0x0000000a:
        case 0x0000000b:
        case 0x0000000c:
        case 0x0000000d:
        case 0x0000000e:
        case 0x0000000f:
        case 0x00000010:
            params->func = XMSS_SHA2;
            break;

        case 0x00000011:
        case 0x00000012:
        case 0x00000013:
        case 0x00000014:
        case 0x00000015:
        case 0x00000016:
        case 0x00000017:
        case 0x00000018:
        case 0x00000019:
        case 0x0000001a:
        case 0x0000001b:
        case 0x0000001c:
        case 0x0000001e:
        case 0x0000001d:
        case 0x0000001f:
        case 0x00000020:
            params->func = XMSS_SHAKE;
            break;

        default:
            return -1;
    }
    switch (oid) {
        case 0x00000001:
        case 0x00000002:
        case 0x00000003:
        case 0x00000004:
        case 0x00000005:
        case 0x00000006:
        case 0x00000007:
        case 0x00000008:

        case 0x00000011:
        case 0x00000012:
        case 0x00000013:
        case 0x00000014:
        case 0x00000015:
        case 0x00000016:
        case 0x00000017:
        case 0x00000018:
            params->n = 32;
            break;

        case 0x00000009:
        case 0x0000000a:
        case 0x0000000b:
        case 0x0000000c:
        case 0x0000000d:
        case 0x0000000e:
        case 0x0000000f:
        case 0x00000010:

        case 0x00000019:
        case 0x0000001a:
        case 0x0000001b:
        case 0x0000001c:
        case 0x0000001d:
        case 0x0000001e:
        case 0x0000001f:
        case 0x00000020:
            params->n = 64;
            break;

        default:
            return -1;
    }
    switch (oid) {
        case 0x00000001:
        case 0x00000002:

        case 0x00000009:
        case 0x0000000a:

        case 0x00000011:
        case 0x00000012:

        case 0x00000019:
        case 0x0000001a:
            params->full_height = 20;
            break;

        case 0x00000003:
        case 0x00000004:
        case 0x00000005:

        case 0x0000000b:
        case 0x0000000c:
        case 0x0000000d:

        case 0x00000013:
        case 0x00000014:
        case 0x00000015:

        case 0x0000001b:
        case 0x0000001c:
        case 0x0000001d:
            params->full_height = 40;
            break;

        case 0x00000006:
        case 0x00000007:
        case 0x00000008:

        case 0x0000000e:
        case 0x0000000f:
        case 0x00000010:

        case 0x00000016:
        case 0x00000017:
        case 0x00000018:

        case 0x0000001e:
        case 0x0000001f:
        case 0x00000020:
            params->full_height = 60;
            break;

        default:
            return -1;
    }
    switch (oid) {
        case 0x00000001:
        case 0x00000003:
        case 0x00000009:
        case 0x0000000b:
        case 0x00000011:
        case 0x00000013:
        case 0x00000019:
        case 0x0000001b:
            params->d = 2;
            break;

        case 0x00000002:
        case 0x00000004:
        case 0x0000000a:
        case 0x0000000c:
        case 0x00000012:
        case 0x00000014:
        case 0x0000001a:
        case 0x0000001c:
            params->d = 4;
            break;

        case 0x00000005:
        case 0x0000000d:
        case 0x00000015:
        case 0x0000001d:
            params->d = 8;
            break;

        case 0x00000006:
        case 0x0000000e:
        case 0x00000016:
        case 0x0000001e:
            params->d = 3;
            break;

        case 0x00000007:
        case 0x0000000f:
        case 0x00000017:
        case 0x0000001f:
            params->d = 6;
            break;

        case 0x00000008:
        case 0x00000010:
        case 0x00000018:
        case 0x00000020:
            params->d = 12;
            break;

        default:
            return -1;
    }

    params->tree_height = params->full_height  / params->d;
    params->wots_w = 16;
    params->wots_log_w = 4;
    params->wots_len1 = 8 * params->n / params->wots_log_w;
    /* len_2 = floor(log(len_1 * (w - 1)) / log(w)) + 1 */
    params->wots_len2 = 3;
    params->wots_len = params->wots_len1 + params->wots_len2;
    params->wots_sig_bytes = params->wots_len * params->n;
    /* Round index_bytes up to nearest byte. */
    params->index_bytes = (params->full_height + 7) / 8;
    params->sig_bytes = (params->index_bytes + params->n
                         + params->d * params->wots_sig_bytes
                         + params->full_height * params->n);

    // TODO figure out sensible and legal values for this based on the above
    params->bds_k = 0;

    params->pk_bytes = 2 * params->n;
    params->sk_bytes = xmssmt_core_sk_bytes(params);
    return 0;
}

//////////////////////////////////////////////////////////////////////
// xms.c

/* This file provides wrapper functions that take keys that include OIDs to
identify the parameter set to be used. After setting the parameters accordingly
it falls back to the regular XMSS core functions. */

int xmss_keypair(unsigned char *pk, unsigned char *sk, const uint32_t oid)
{
    xmss_params params;
    unsigned int i;

    if (xmss_parse_oid(&params, oid)) {
        return -1;
    }
    for (i = 0; i < XMSS_OID_LEN; i++) {
        pk[XMSS_OID_LEN - i - 1] = (oid >> (8 * i)) & 0xFF;
        /* For an implementation that uses runtime parameters, it is crucial
        that the OID is part of the secret key as well;
        i.e. not just for interoperability, but also for internal use. */
        sk[XMSS_OID_LEN - i - 1] = (oid >> (8 * i)) & 0xFF;
    }
    return xmss_core_keypair(&params, pk + XMSS_OID_LEN, sk + XMSS_OID_LEN);
}

int xmss_sign(unsigned char *sk,
              unsigned char *sm, unsigned long long *smlen,
              const unsigned char *m, unsigned long long mlen)
{
    xmss_params params;
    uint32_t oid = 0;
    unsigned int i;

    for (i = 0; i < XMSS_OID_LEN; i++) {
        oid |= sk[XMSS_OID_LEN - i - 1] << (i * 8);
    }
    if (xmss_parse_oid(&params, oid)) {
        return -1;
    }
    return xmss_core_sign(&params, sk + XMSS_OID_LEN, sm, smlen, m, mlen);
}

int xmss_sign_open(unsigned char *m, unsigned long long *mlen,
                   const unsigned char *sm, unsigned long long smlen,
                   const unsigned char *pk)
{
    xmss_params params;
    uint32_t oid = 0;
    unsigned int i;

    for (i = 0; i < XMSS_OID_LEN; i++) {
        oid |= pk[XMSS_OID_LEN - i - 1] << (i * 8);
    }
    if (xmss_parse_oid(&params, oid)) {
        return -1;
    }
    return xmss_core_sign_open(&params, m, mlen, sm, smlen, pk + XMSS_OID_LEN);
}

int xmss_sign_bytes(uint32_t oid)
{
    xmss_params params;

	if (xmss_parse_oid(&params, oid))
        return -1;

	return params.sig_bytes;
}
int xmss_bits(uint32_t oid)
{
    xmss_params params;

	if (xmss_parse_oid(&params, oid))
        return 0;

	return params.n * 8;
}
int xmss_height(uint32_t oid)
{
    xmss_params params;

	if (xmss_parse_oid(&params, oid))
        return 0;

	return params.full_height;
}
int xmss_key_bytes(uint32_t oid, int *plen, int *slen)
{
    xmss_params params;

	if (xmss_parse_oid(&params, oid))
        return -1;

	*plen = (int)params.pk_bytes + XMSS_OID_LEN;
	*slen = (int)params.sk_bytes + XMSS_OID_LEN;

	return 0;
}
int xmss_sk_bytes(uint32_t oid, int *ilen, int *slen)
{
    xmss_params params;

	if (xmss_parse_oid(&params, oid))
        return -1;

	*ilen = params.index_bytes;
	*slen = params.n * 4;

	return 0;
}
int xmss_load_openssh_sk(uint32_t oid, unsigned char *sk, CBuffer *bp, CStringA *encname, CBuffer *ivbuf)
{
	int n, i, ts;
    xmss_params params;
	int ret = -1;
	CBuffer tmp;
	CStringA str;

	if (xmss_parse_oid(&params, oid))
        return -1;

	for (i = 0; i < XMSS_OID_LEN; i++)
        sk[XMSS_OID_LEN - i - 1] = (oid >> (8 * i)) & 0xFF;
	sk += XMSS_OID_LEN;

	bds_state state;
	int treesize = params.tree_height - params.bds_k;
    treehash_inst *treehash = new treehash_inst [treesize];
	unsigned char *th_node = new unsigned char [treesize * params.n];

	state.treehash = treehash;
	xmss_deserialize_state(&params, &state, sk);

    state.stackoffset = 0;
    state.next_leaf = 0;

	// seckeys
	tmp.Clear(); bp->GetBuf(&tmp);
	if ( tmp.GetSize() != (params.index_bytes + params.n * 4) )
		goto ERRRET;
	memcpy(sk, tmp.GetPtr(), tmp.GetSize());

	//SSHKEY_SERIALIZE_DEFAULT = 0;
	//SSHKEY_SERIALIZE_STATE = 1;
	//SSHKEY_SERIALIZE_FULL = 2;
	//SSHKEY_SERIALIZE_INFO = 254;
	i = bp->Get8Bit();
	if ( i == 2 ) {
		bp->GetStr(str);
		if ( encname != NULL )
			*encname = str;
		tmp.Clear(); bp->GetBuf(&tmp);
		if ( ivbuf != NULL ) {
			ivbuf->Clear();
			ivbuf->Apend(tmp.GetPtr(), tmp.GetSize());
		}
	} else if ( i != 1 )
		goto ERRRET;

	// SSH_XMSS_K2_MAGIC
	bp->GetStr(str);
	if ( str.Compare("k=2") != 0 )
		goto ERRRET;

	// index
	i = bp->Get32Bit();
	sk[0] = (unsigned char)(i >> 24);
    sk[1] = (unsigned char)(i >> 16);
    sk[2] = (unsigned char)(i >>  8);
    sk[3] = (unsigned char)(i      );

	// stack
	tmp.Clear(); bp->GetBuf(&tmp);
	if ( tmp.GetSize() != ((params.tree_height + 1) * params.n) )
		goto ERRRET;
	memcpy(state.stack, tmp.GetPtr(), tmp.GetSize());

	// stackoffset
	state.stackoffset = bp->Get32Bit();

	// stacklevels
	tmp.Clear(); bp->GetBuf(&tmp);
	if ( tmp.GetSize() != (params.tree_height + 1) )
		goto ERRRET;
	memcpy(state.stacklevels, tmp.GetPtr(), tmp.GetSize());

	// auth
	tmp.Clear(); bp->GetBuf(&tmp);
	if ( tmp.GetSize() != (params.tree_height * params.n) )
		goto ERRRET;
	memcpy(state.auth, tmp.GetPtr(), tmp.GetSize());

	// keep
	tmp.Clear(); bp->GetBuf(&tmp);
	if ( tmp.GetSize() != ((params.tree_height >> 1) * params.n) )
		goto ERRRET;
	memcpy(state.keep, tmp.GetPtr(), tmp.GetSize());

	// th_nodes
	tmp.Clear(); bp->GetBuf(&tmp);
	if ( tmp.GetSize() != (treesize * params.n) )
		goto ERRRET;
	memcpy(th_node, tmp.GetPtr(), tmp.GetSize());

	// retain
	tmp.Clear(); bp->GetBuf(&tmp);
	if ( tmp.GetSize() != (((1ULL << params.bds_k) - params.bds_k - 1) * params.n) )
		goto ERRRET;
	memcpy(state.retain, tmp.GetPtr(), tmp.GetSize());

	// sizeof treehash
	ts = bp->Get32Bit();
	if ( ts > treesize )
		goto ERRRET;

	for ( i = 0 ; i < ts ; i++ ) {
		treehash[i].h          = (unsigned char)bp->Get32Bit();
		treehash[i].next_idx   = (unsigned long)bp->Get32Bit();
		treehash[i].stackusage = (unsigned char)bp->Get32Bit();
		treehash[i].completed  = (unsigned char)bp->Get8Bit();
		if ( (n = bp->Get32Bit()) < 0 || n > (int)(treesize * params.n) )
			goto ERRRET;
		memcpy(treehash[i].node, th_node + n, params.n);
	}

	xmss_serialize_state(&params, sk, &state);
	ret = 0;

ERRRET:
	delete [] treehash;
	delete [] th_node;
	return ret;
}
int xmss_save_openssh_sk(uint32_t oid, unsigned char *sk, CBuffer *bp, const char *encname, unsigned char *ivbuf, int ivlen)
{
	int i;
    xmss_params params;
	CBuffer tmp;

	if (xmss_parse_oid(&params, oid))
        return -1;
	sk += XMSS_OID_LEN;

	bds_state state;
	int treesize = params.tree_height - params.bds_k;
    treehash_inst *treehash = new treehash_inst [treesize];

    state.treehash = treehash;
    xmss_deserialize_state(&params, &state, sk);

	// seckeys
	bp->PutBuf((LPBYTE)sk, params.index_bytes + params.n * 4);

	//SSHKEY_SERIALIZE_DEFAULT = 0;
	//SSHKEY_SERIALIZE_STATE = 1;
	//SSHKEY_SERIALIZE_FULL = 2;
	//SSHKEY_SERIALIZE_INFO = 254;
	if ( encname != NULL && *encname != '\0' && ivbuf != NULL && ivlen > 0 ) {
		bp->Put8Bit(2);
		bp->PutStr(encname);
		bp->PutBuf(ivbuf, ivlen);
	} else
		bp->Put8Bit(1);

	// SSH_XMSS_K2_MAGIC
	bp->PutBuf((LPBYTE)"k=2", 3);

	// index
	i = ((unsigned long)sk[0] << 24) | ((unsigned long)sk[1] << 16) | ((unsigned long)sk[2] << 8) | sk[3];
	bp->Put32Bit(i);

	// stack
	bp->PutBuf((LPBYTE)state.stack, (params.tree_height + 1) * params.n);

	// stackoffset
	bp->Put32Bit(state.stackoffset);

	// stacklevels
	bp->PutBuf((LPBYTE)state.stacklevels, params.tree_height + 1);

	// auth
	bp->PutBuf((LPBYTE)state.auth, params.tree_height * params.n);

	// keep
	bp->PutBuf((LPBYTE)state.keep, (params.tree_height >> 1) * params.n);

	// th_nodes
	tmp.Clear();
	for ( i = 0 ; i < treesize ; i++ )
		tmp.Apend(treehash[i].node, params.n);
	bp->PutBuf(tmp.GetPtr(), tmp.GetSize());

	// retain
	bp->PutBuf((LPBYTE)state.retain, ((1ULL << params.bds_k) - params.bds_k - 1) * params.n);

	// sizeof treehash
	bp->Put32Bit(treesize);

	for ( i = 0 ; i < treesize ; i++ ) {
		bp->Put32Bit(treehash[i].h);
		bp->Put32Bit(treehash[i].next_idx);
		bp->Put32Bit(treehash[i].stackusage);
		bp->Put8Bit(treehash[i].completed);
		bp->Put32Bit(i * params.n);
	}

	delete [] treehash;

	return 0;
}

//////////////////////////////////////////////////////////////////////

int xmssmt_keypair(unsigned char *pk, unsigned char *sk, const uint32_t oid)
{
    xmss_params params;
    unsigned int i;

    if (xmssmt_parse_oid(&params, oid)) {
        return -1;
    }
    for (i = 0; i < XMSS_OID_LEN; i++) {
        pk[XMSS_OID_LEN - i - 1] = (oid >> (8 * i)) & 0xFF;
        sk[XMSS_OID_LEN - i - 1] = (oid >> (8 * i)) & 0xFF;
    }
    return xmssmt_core_keypair(&params, pk + XMSS_OID_LEN, sk + XMSS_OID_LEN);
}

int xmssmt_sign(unsigned char *sk,
                unsigned char *sm, unsigned long long *smlen,
                const unsigned char *m, unsigned long long mlen)
{
    xmss_params params;
    uint32_t oid = 0;
    unsigned int i;

    for (i = 0; i < XMSS_OID_LEN; i++) {
        oid |= sk[XMSS_OID_LEN - i - 1] << (i * 8);
    }
    if (xmssmt_parse_oid(&params, oid)) {
        return -1;
    }
    return xmssmt_core_sign(&params, sk + XMSS_OID_LEN, sm, smlen, m, mlen);
}

int xmssmt_sign_open(unsigned char *m, unsigned long long *mlen,
                     const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk)
{
    xmss_params params;
    uint32_t oid = 0;
    unsigned int i;

    for (i = 0; i < XMSS_OID_LEN; i++) {
        oid |= pk[XMSS_OID_LEN - i - 1] << (i * 8);
    }
    if (xmssmt_parse_oid(&params, oid)) {
        return -1;
    }
    return xmssmt_core_sign_open(&params, m, mlen, sm, smlen, pk + XMSS_OID_LEN);
}

int xmssmt_sign_bytes(uint32_t oid)
{
    xmss_params params;

	if (xmssmt_parse_oid(&params, oid))
        return -1;

	return params.sig_bytes;
}
int xmssmt_bits(uint32_t oid)
{
    xmss_params params;

	if (xmssmt_parse_oid(&params, oid))
        return 0;

	return params.n * 8;
}
int xmssmt_height(uint32_t oid)
{
    xmss_params params;

	if (xmssmt_parse_oid(&params, oid))
        return 0;

	return params.full_height;
}
int xmssmt_key_bytes(uint32_t oid, int *plen, int *slen)
{
    xmss_params params;

	if (xmssmt_parse_oid(&params, oid))
        return -1;

	*plen = (int)params.pk_bytes + XMSS_OID_LEN;
	*slen = (int)params.sk_bytes + XMSS_OID_LEN;

	return 0;
}
