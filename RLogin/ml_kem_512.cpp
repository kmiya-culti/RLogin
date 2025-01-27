#include "stdafx.h"

#define	ML_KEM_512
#include "ml_kem.cpp"

#if 0
#define mlkem512_PUBLICKEYBYTES			800
#define mlkem512_SECRETKEYBYTES			1632
#define mlkem512_CIPHERTEXTBYTES		768
#define mlkem512_BYTES					32

void mlkem_test()
{
	uint8_t pk[KYBER_PUBLICKEYBYTES];
	uint8_t sk[KYBER_SECRETKEYBYTES];

	crypto_kem_keypair(pk, sk);

	uint8_t ct[KYBER_CIPHERTEXTBYTES];
	uint8_t ss[KYBER_SSBYTES];
	uint8_t os[KYBER_SSBYTES];

	crypto_kem_enc(ct, ss, pk);
	crypto_kem_dec(os, ct, sk);

	if ( memcmp(ss, os, KYBER_SSBYTES) == 0 &&
			mlkem512_PUBLICKEYBYTES == KYBER_PUBLICKEYBYTES &&
			mlkem512_SECRETKEYBYTES == KYBER_SECRETKEYBYTES &&
			mlkem512_CIPHERTEXTBYTES == KYBER_CIPHERTEXTBYTES &&
			mlkem512_BYTES == KYBER_SSBYTES )
		TRACE("OK\n");
}
#endif