#include "stdafx.h"

#define	ML_KEM_1024
#include "ml_kem.cpp"

#if 0
#define mlkem1024_PUBLICKEYBYTES		1568
#define mlkem1024_SECRETKEYBYTES		3168
#define mlkem1024_CIPHERTEXTBYTES		1568
#define mlkem1024_BYTES					32

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
			mlkem1024_PUBLICKEYBYTES == KYBER_PUBLICKEYBYTES &&
			mlkem1024_SECRETKEYBYTES == KYBER_SECRETKEYBYTES &&
			mlkem1024_CIPHERTEXTBYTES == KYBER_CIPHERTEXTBYTES &&
			mlkem1024_BYTES == KYBER_SSBYTES )
		TRACE("OK\n");
}
#endif