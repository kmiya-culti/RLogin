// ssh.cpp: Cssh クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <windns.h>
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "EditDlg.h"
#include "IdKeyFileDlg.h"
#include "CertKeyDlg.h"
#include "ssh.h"

#include "ssh1.h"
#include "ssh2.h"

#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <sddl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CCipher
//////////////////////////////////////////////////////////////////////

static const struct _CipherTab {
	int		num;
	LPCTSTR	name;
	const EVP_CIPHER * (*func)(void);
	int		block_size;
	int		key_len;
	int     iv_len;
	int     tag_len;
	int		discard;
} CipherTab[] = {
	{ SSH_CIPHER_NONE,			_T("none"),								(const EVP_CIPHER *(*)())EVP_enc_null,			8,	0,	0, 0, 0	},	// RFC 4253
	{ SSH_CIPHER_DES,			_T("des"),								(const EVP_CIPHER *(*)())EVP_des_cbc,			8,	8,	0, 0, 0	},
	{ SSH_CIPHER_3DES,			_T("3des"),								(const EVP_CIPHER *(*)())evp_ssh1_3des,			8,	16,	0, 0, 0	},
	{ SSH_CIPHER_BLOWFISH,		_T("blowfish"),							(const EVP_CIPHER *(*)())evp_ssh1_bf,			8,	32,	0, 0, 0	},

	{ SSH2_CIPHER_3DES_CBC,		_T("3des-cbc"),							(const EVP_CIPHER *(*)())EVP_des_ede3_cbc,		8,	24,	0, 0, 0	},	// RFC 4253	REQUIRED
	{ SSH2_CIPHER_3DES_CTR,		_T("3des-ctr"),							(const EVP_CIPHER *(*)())evp_des3_ctr,			8,	24,	0, 0, 0	},	// RFC 4344	RECOMMENDED
	{ SSH2_CIPHER_DES_CBC,		_T("des-cbc"),							(const EVP_CIPHER *(*)())EVP_des_cbc,			8,	8,	0, 0, 0	},	// FIPS-46-3

	{ SSH2_CIPHER_BLF_CBC,		_T("blowfish-cbc"),						(const EVP_CIPHER *(*)())EVP_bf_cbc,			8,	16,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_BLF_CTR,		_T("blowfish-ctr"),						(const EVP_CIPHER *(*)())evp_bf_ctr,			8,	32,	0, 0, 0	},	// RFC 4344

	{ SSH2_CIPHER_IDEA_CBC,		_T("idea-cbc"),							(const EVP_CIPHER *(*)())EVP_idea_cbc,			8,	16,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_IDEA_CTR,		_T("idea-ctr"),							(const EVP_CIPHER *(*)())evp_idea_ctr,			8,	16,	0, 0, 0	},	// RFC 4344

	{ SSH2_CIPHER_CAST_CBC,		_T("cast128-cbc"),						(const EVP_CIPHER *(*)())EVP_cast5_cbc,			8,	16,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_CAST_CTR,		_T("cast128-ctr"),						(const EVP_CIPHER *(*)())evp_cast5_ctr,			8,	16,	0, 0, 0	},	// RFC 4344

	{ SSH2_CIPHER_ARCFOUR,		_T("arcfour"),							(const EVP_CIPHER *(*)())EVP_rc4,				8,	16,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_ARC128,		_T("arcfour128"),						(const EVP_CIPHER *(*)())EVP_rc4,				8,	16,	0, 0, 1536	},	// RFC 4345
	{ SSH2_CIPHER_ARC256,		_T("arcfour256"),						(const EVP_CIPHER *(*)())EVP_rc4,				8,	32,	0, 0, 1536	},	// RFC 4345

	{ SSH2_CIPHER_AES128,		_T("aes128-cbc"),						(const EVP_CIPHER *(*)())EVP_aes_128_cbc,		16,	16,	0, 0, 0	},	// RFC 4253	RECOMMENDED
	{ SSH2_CIPHER_AES192,		_T("aes192-cbc"),						(const EVP_CIPHER *(*)())EVP_aes_192_cbc,		16,	24,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_AES256,		_T("aes256-cbc"),						(const EVP_CIPHER *(*)())EVP_aes_256_cbc,		16,	32,	0, 0, 0	},	// RFC 4253

	{ SSH2_CIPHER_AES128R,		_T("aes128-ctr"),						(const EVP_CIPHER *(*)())EVP_aes_128_ctr,		16,	16,	0, 0, 0	},	// RFC 4344	RECOMMENDED
	{ SSH2_CIPHER_AES192R,		_T("aes192-ctr"),						(const EVP_CIPHER *(*)())EVP_aes_192_ctr,		16,	24,	0, 0, 0	},	// RFC 4344	RECOMMENDED
	{ SSH2_CIPHER_AES256R,		_T("aes256-ctr"),						(const EVP_CIPHER *(*)())EVP_aes_256_ctr,		16,	32,	0, 0, 0	},	// RFC 4344	RECOMMENDED

#ifdef	USE_NETTLE
	{ SSH2_CIPHER_TWF256,		_T("twofish-cbc"),						(const EVP_CIPHER *(*)())evp_twofish_cbc,		16,	32,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_TWF256R,		_T("twofish-ctr"),						(const EVP_CIPHER *(*)())evp_twofish_ctr,		16,	32,	0, 0, 0	},

	{ SSH2_CIPHER_TWF128,		_T("twofish128-cbc"),					(const EVP_CIPHER *(*)())evp_twofish_cbc,		16,	16,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_TWF192,		_T("twofish192-cbc"),					(const EVP_CIPHER *(*)())evp_twofish_cbc,		16,	24,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_TWF256,		_T("twofish256-cbc"),					(const EVP_CIPHER *(*)())evp_twofish_cbc,		16,	32,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_TWF128R,		_T("twofish128-ctr"),					(const EVP_CIPHER *(*)())evp_twofish_ctr,		16,	16,	0, 0, 0	},	// RFC 4344
	{ SSH2_CIPHER_TWF192R,		_T("twofish192-ctr"),					(const EVP_CIPHER *(*)())evp_twofish_ctr,		16,	24,	0, 0, 0	},	// RFC 4344
	{ SSH2_CIPHER_TWF256R,		_T("twofish256-ctr"),					(const EVP_CIPHER *(*)())evp_twofish_ctr,		16,	32,	0, 0, 0	},	// RFC 4344

	{ SSH2_CIPHER_SEP128,		_T("serpent128-cbc"),					(const EVP_CIPHER *(*)())evp_serpent_cbc,		16,	16,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_SEP192,		_T("serpent192-cbc"),					(const EVP_CIPHER *(*)())evp_serpent_cbc,		16,	24,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_SEP256,		_T("serpent256-cbc"),					(const EVP_CIPHER *(*)())evp_serpent_cbc,		16,	32,	0, 0, 0	},	// RFC 4253
	{ SSH2_CIPHER_SEP128R,		_T("serpent128-ctr"),					(const EVP_CIPHER *(*)())evp_serpent_ctr,		16,	16,	0, 0, 0	},	// RFC 4344
	{ SSH2_CIPHER_SEP192R,		_T("serpent192-ctr"),					(const EVP_CIPHER *(*)())evp_serpent_ctr,		16,	24,	0, 0, 0	},	// RFC 4344
	{ SSH2_CIPHER_SEP256R,		_T("serpent256-ctr"),					(const EVP_CIPHER *(*)())evp_serpent_ctr,		16,	32,	0, 0, 0	},	// RFC 4344
#endif

	{ SSH2_CIPHER_CAM128,		_T("camellia128-cbc"),					(const EVP_CIPHER *(*)())EVP_camellia_128_cbc,	16,	16,	0, 0, 0	},
	{ SSH2_CIPHER_CAM192,		_T("camellia192-cbc"),					(const EVP_CIPHER *(*)())EVP_camellia_192_cbc,	16,	24,	0, 0, 0	},
	{ SSH2_CIPHER_CAM256,		_T("camellia256-cbc"),					(const EVP_CIPHER *(*)())EVP_camellia_256_cbc,	16,	32,	0, 0, 0	},
	{ SSH2_CIPHER_CAM128R,		_T("camellia128-ctr"),					(const EVP_CIPHER *(*)())EVP_camellia_128_ctr,	16,	16,	0, 0, 0	},
	{ SSH2_CIPHER_CAM192R,		_T("camellia192-ctr"),					(const EVP_CIPHER *(*)())EVP_camellia_192_ctr,	16,	24,	0, 0, 0	},
	{ SSH2_CIPHER_CAM256R,		_T("camellia256-ctr"),					(const EVP_CIPHER *(*)())EVP_camellia_256_ctr,	16,	32,	0, 0, 0	},

#ifdef	USE_CLEFIA
	{ SSH2_CIPHER_CLE128,		_T("clefia128-cbc"),					(const EVP_CIPHER *(*)())evp_clefia_cbc,		16,	16,	0, 0, 0	},
	{ SSH2_CIPHER_CLE192,		_T("clefia192-cbc"),					(const EVP_CIPHER *(*)())evp_clefia_cbc,		16,	24,	0, 0, 0	},
	{ SSH2_CIPHER_CLE256,		_T("clefia256-cbc"),					(const EVP_CIPHER *(*)())evp_clefia_cbc,		16,	32,	0, 0, 0	},
	{ SSH2_CIPHER_CLE128R,		_T("clefia128-ctr"),					(const EVP_CIPHER *(*)())evp_clefia_ctr,		16,	16,	0, 0, 0	},
	{ SSH2_CIPHER_CLE192R,		_T("clefia192-ctr"),					(const EVP_CIPHER *(*)())evp_clefia_ctr,		16,	24,	0, 0, 0	},
	{ SSH2_CIPHER_CLE256R,		_T("clefia256-ctr"),					(const EVP_CIPHER *(*)())evp_clefia_ctr,		16,	32,	0, 0, 0	},
#endif

	{ SSH2_CIPHER_SEED_CBC,		_T("seed-cbc@ssh.com"),					(const EVP_CIPHER *(*)())EVP_seed_cbc,			16,	16,	0, 0, 0	},
	{ SSH2_CIPHER_SEED_CTR,		_T("seed-ctr@ssh.com"),					(const EVP_CIPHER *(*)())evp_seed_ctr,			16,	16,	0, 0, 0	},

	{ SSH2_AEAD_AES128GCM,		_T("aes128-gcm@openssh.com"),			(const EVP_CIPHER *(*)())EVP_aes_128_gcm,		16,	16,	12, 16, 0	},
	{ SSH2_AEAD_AES256GCM,		_T("aes256-gcm@openssh.com"),			(const EVP_CIPHER *(*)())EVP_aes_256_gcm,		16,	32,	12, 16, 0	},
	{ SSH2_AEAD_AES128GCM,		_T("aes128-gcm"),						(const EVP_CIPHER *(*)())EVP_aes_128_gcm,		16,	16,	12, 16, 0	},	// draft-miller-sshm-aes-gcm-00
	{ SSH2_AEAD_AES256GCM,		_T("aes256-gcm"),						(const EVP_CIPHER *(*)())EVP_aes_256_gcm,		16,	32,	12, 16, 0	},	// draft-miller-sshm-aes-gcm-00

	{ SSH2_AEAD_AES128GCM,		_T("AEAD_AES_128_GCM"),					(const EVP_CIPHER *(*)())EVP_aes_128_gcm,		16,	16,	12, 16, 0	},	// RFC 5647
	{ SSH2_AEAD_AES256GCM,		_T("AEAD_AES_256_GCM"),					(const EVP_CIPHER *(*)())EVP_aes_256_gcm,		16,	32,	12, 16, 0	},	// RFC 5647

	{ SSH2_AEAD_AES128CCM,		_T("AEAD_AES_128_CCM"),					(const EVP_CIPHER *(*)())EVP_aes_128_ccm,		16,	16,	12, 16, 0	},	// RFC 5116
	{ SSH2_AEAD_AES256CCM,		_T("AEAD_AES_256_CCM"),					(const EVP_CIPHER *(*)())EVP_aes_256_ccm,		16,	32,	12, 16, 0	},	// RFC 5116

	{ SSH2_AEAD_CHACHAPOLY,		_T("AEAD_CHACHA20_POLY1305"),			(const EVP_CIPHER *(*)())EVP_chacha20_poly1305,	16,	32,	12, 16, 0	},	// RFC 8103

	{ SSH2_CHACHA20_POLY1305,	_T("chacha20-poly1305@openssh.com"),	(const EVP_CIPHER *(*)())evp_chachapoly_256,	16,	64,	 0, 16, 0	},
	{ SSH2_CHACHA20_POLY1305,	_T("chacha20-poly1305"),				(const EVP_CIPHER *(*)())evp_chachapoly_256,	16,	64,	 0, 16, 0	},	// draft-ietf-sshm-chacha20-poly1305-00

	{ 0, NULL, NULL }
};

CCipher::CCipher()
{
	m_Index = (-1);
	m_KeyBuf = NULL;
	m_IvBuf = NULL;
	m_pEvp = NULL;
	m_Mode = (-1);
}
CCipher::~CCipher()
{
	Close();
}
void CCipher::Close()
{
	if ( m_pEvp != NULL ) {
		EVP_CIPHER_CTX_cleanup(m_pEvp);
		EVP_CIPHER_CTX_free(m_pEvp);
		m_pEvp = NULL;
	}

	if ( m_KeyBuf != NULL ) {
		delete [] m_KeyBuf;
		m_KeyBuf = NULL;
	}

	if ( m_IvBuf != NULL) {
		delete [] m_IvBuf;
		m_IvBuf = NULL;
	}

	m_Index = (-1);
	m_Mode = (-1);
}
int CCipher::Init(LPCTSTR name, int mode, LPBYTE key, int len, LPBYTE iv)
{
	int n, i;
	const EVP_CIPHER *cipher = NULL;
	OSSL_PARAM params[3];

	Close();

	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		if ( _tcscmp(name, CipherTab[n].name) == 0 ) {
			cipher = (*(CipherTab[n].func))();
			break;
		}
	}
	if ( cipher == NULL )
		return FALSE;

	if ( iv != NULL ) {
		i = (CipherTab[n].iv_len > 0 ? CipherTab[n].iv_len : CipherTab[n].block_size);
		m_IvBuf = new BYTE[i];
		memcpy(m_IvBuf, iv, i);
	}

	if ( len < 0 )
		len = CipherTab[n].key_len;
	if ( key != NULL ) {
		m_KeyBuf = new BYTE[len];
		memcpy(m_KeyBuf, key, len);
	}

	if ( (m_pEvp = EVP_CIPHER_CTX_new()) == NULL )
		goto ERRENDOF;

	if ( EVP_CipherInit(m_pEvp, cipher, m_KeyBuf, m_IvBuf, (mode == MODE_ENC ? 1 : 0)) == 0 )
		goto ERRENDOF;

	if ( (i = EVP_CIPHER_CTX_key_length(m_pEvp)) > 0 && i != len ) {
		if ( EVP_CIPHER_CTX_set_key_length(m_pEvp, len) == 0 )
			goto ERRENDOF;
		EVP_CipherInit_ex(m_pEvp, NULL, NULL, m_KeyBuf, NULL, -1);
	}

	if ( SSH2_CIPHER_GCM(CipherTab[n].num) && m_IvBuf != NULL ) {
		params[0] = OSSL_PARAM_construct_octet_string(OSSL_CIPHER_PARAM_AEAD_TLS1_IV_FIXED, m_IvBuf, (-1));
		params[1] = OSSL_PARAM_construct_end();
		if ( EVP_CIPHER_CTX_set_params(m_pEvp, params) == 0 )
			goto ERRENDOF;
	}

	if ( SSH2_CIPHER_CCM(CipherTab[n].num) ) {
		//	RFC 5116
		//	5.3. AEAD_AES_128_CCM
		//	the nonce length n is 12,
		//	the tag length t is 16

		size_t sz = CipherTab[n].iv_len;
		params[0] = OSSL_PARAM_construct_size_t(OSSL_CIPHER_PARAM_IVLEN, &sz);
		params[1] = OSSL_PARAM_construct_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, NULL, CipherTab[n].tag_len);
		params[2] = OSSL_PARAM_construct_end();
		if ( EVP_CIPHER_CTX_set_params(m_pEvp, params) == 0 )
			goto ERRENDOF;

		EVP_CipherInit_ex(m_pEvp, NULL, NULL, m_KeyBuf, m_IvBuf, -1);
	}

	if ( (i = CipherTab[n].discard) > 0) {
		LPBYTE junk = new BYTE[i];
		LPBYTE discard = new BYTE[i];
		if ( EVP_Cipher(m_pEvp, discard, junk, i) == 0)
			return TRUE;
		delete [] junk;
		delete [] discard;
	}

	m_Index = n;
	m_Mode = mode;

	return TRUE;

ERRENDOF:
	if ( m_pEvp != NULL ) {
		EVP_CIPHER_CTX_free(m_pEvp);
		m_pEvp = NULL;
	}

	return FALSE;
}
int CCipher::SetIvCounter(DWORD seq)
{
	BYTE iv[SSH2_CIPHER_IVSIZEMAX], *p;
	OSSL_PARAM params[2] = {
		OSSL_PARAM_octet_string(NULL, iv, 0),
		OSSL_PARAM_END
	};

	if ( m_Index == (-1) )
		return FALSE;

	ASSERT(CipherTab[m_Index].iv_len <= SSH2_CIPHER_IVSIZEMAX);

	switch(CipherTab[m_Index].num / 100) {
	case 2:	// GCM
		params[0].key = OSSL_CIPHER_PARAM_AEAD_TLS1_GET_IV_GEN;
		params[0].data_size = CipherTab[m_Index].iv_len;
		if ( EVP_CIPHER_CTX_get_params(m_pEvp, params) == 0 )
			return FALSE;
		break;

	case 3:	// CCM
	case 4:	// AEAD
		// 要確認
		p = iv + CipherTab[m_Index].iv_len;
		for ( int i = 0 ; i < sizeof(seq) && p > iv ; i++ ) {
			*(--p) = (BYTE)(seq);
			seq >>= 8;
		}

		if ( p > iv ) {
			if ( m_IvBuf != NULL )
				memcpy(iv, m_IvBuf, (p - iv));
			else
				memset(iv, 0, (p - iv));
		}

		if ( EVP_CipherInit_ex(m_pEvp, NULL, NULL, NULL, iv, -1) == 0 )
			return FALSE;
		break;

	case 5:	// SSH ChachaPoly
		iv[0] = iv[1] = iv[2] = iv[3] = 0;
		iv[4] = (BYTE)(seq >> 24);
		iv[5] = (BYTE)(seq >> 16);
		iv[6] = (BYTE)(seq >> 8);
		iv[7] = (BYTE)(seq);

		params[0].key = OSSL_CIPHER_PARAM_AEAD_TLS1_IV_FIXED;
		params[0].data_size = 8;
		if ( EVP_CIPHER_CTX_set_params(m_pEvp, params) == 0 )
			return FALSE;
		break;
	}

	return TRUE;
}
int CCipher::GeHeadData(LPBYTE inbuf)
{
	BYTE outbuf[4];

	if ( m_Index == (-1) )
		return 0;
	else if ( SSH2_CIPHER_POLY(CipherTab[m_Index].num) ) {
		EVP_Cipher(m_pEvp, outbuf, inbuf, 4);
		inbuf = outbuf;
	}

	return ((int)inbuf[0] << 24) | ((int)inbuf[1] << 16) | ((int)inbuf[2] << 8) | ((int)inbuf[3]);
}
int CCipher::CipherSshChahaPoly(LPBYTE inbuf, int len, CBuffer *out)
{
	u_char tag[SSH2_CIPHER_TAGSIZEMAX];			
	OSSL_PARAM params[2] = {
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, NULL, 0),
		OSSL_PARAM_END
	};

	ASSERT(CipherTab[m_Index].tag_len <= SSH2_CIPHER_TAGSIZEMAX);

	if ( m_Mode == MODE_ENC ) {
		if ( len < 4 )
			return FALSE;

		if ( EVP_Cipher(m_pEvp, out->PutSpc(len), inbuf, len) == 0 )
			return FALSE;

		params[0].data = out->GetPtr();
		params[0].data_size = out->GetSize();
		if ( EVP_CIPHER_CTX_set_params(m_pEvp, params) == 0 )
			return FALSE;

		params[0].data = out->PutSpc(CipherTab[m_Index].tag_len);
		params[0].data_size = CipherTab[m_Index].tag_len;
		if ( EVP_CIPHER_CTX_get_params(m_pEvp, params) == 0 )
			return FALSE;

	} else if ( m_Mode == MODE_DEC ) {
		if ( (len -= CipherTab[m_Index].tag_len) < 4 )
			return FALSE;

		params[0].data = inbuf;
		params[0].data_size = len;
		if ( EVP_CIPHER_CTX_set_params(m_pEvp, params) == 0 )
			return FALSE;

		params[0].data = tag;
		params[0].data_size = CipherTab[m_Index].tag_len;
		if ( EVP_CIPHER_CTX_get_params(m_pEvp, params) == 0 )
			return FALSE;

		if ( memcmp(tag, inbuf + len, CipherTab[m_Index].tag_len) != 0 )
			return FALSE;

		if ( !EVP_Cipher(m_pEvp, out->PutSpc(len), inbuf, len) )
			return FALSE;

	} else
		return FALSE;

	return TRUE;
}
int CCipher::CipherAead(LPBYTE inbuf, int len, CBuffer *out)
{
	OSSL_PARAM params[2] = {
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, NULL, 0),
		OSSL_PARAM_END
	};

	if ( m_Mode == MODE_ENC ) {	// m_pEvp->encrypt ) {
		if ( len < 4 )
			return FALSE;

		// AAD(Additional Authenticated Data) Packet Length not encrypt !!
		if ( EVP_Cipher(m_pEvp, NULL, inbuf, 4) < 0 )
			return FALSE;
		out->Apend(inbuf, 4);
		inbuf += 4;
		len -= 4;

		if ( EVP_Cipher(m_pEvp, out->PutSpc(len), inbuf, len) != len )
			return FALSE;

		len += 4;
		if ( EVP_Cipher(m_pEvp, NULL, NULL, len) != len )
			return FALSE;

		params[0].data = out->PutSpc(CipherTab[m_Index].tag_len);
		params[0].data_size = CipherTab[m_Index].tag_len;
		if ( EVP_CIPHER_CTX_get_params(m_pEvp, params) == 0 )
			return FALSE;

	} else if ( m_Mode == MODE_DEC ) {
		if ( (len -= CipherTab[m_Index].block_size) < 4 )
			return FALSE;

		params[0].data = inbuf + len;
		params[0].data_size = CipherTab[m_Index].tag_len;
		if ( EVP_CIPHER_CTX_set_params(m_pEvp, params) == 0 )
			return FALSE;

		// AAD(Additional Authenticated Data) Packet Length not deccrypt !!
		if ( EVP_Cipher(m_pEvp, NULL, inbuf, 4) < 0 )	
			return FALSE;
		out->Apend(inbuf, 4);
		inbuf += 4;
		len -= 4;

		if ( EVP_Cipher(m_pEvp, out->PutSpc(len), inbuf, len) != len )
			return FALSE;

		len += 4;
		if ( EVP_Cipher(m_pEvp, NULL, NULL, len) != len )
			return FALSE;

	} else
		return FALSE;

	return TRUE;
}
int CCipher::CipherAeadCCM(LPBYTE inbuf, int len, CBuffer *out)
{
	OSSL_PARAM params[2] = {
		OSSL_PARAM_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, NULL, 0),
		OSSL_PARAM_END
	};

	if ( m_Mode == MODE_ENC ) {
		if ( len < 4 )
			return FALSE;

		if ( EVP_Cipher(m_pEvp, NULL, NULL, len - 4) < 0 )
			return FALSE;

		// AAD Additional Authenticated Data uint32 Packet Length not encrypt !!
		if ( EVP_Cipher(m_pEvp, NULL, inbuf, 4) < 0 )
			return FALSE;
		out->Apend(inbuf, 4);
		inbuf += 4;
		len   -= 4;

		if ( EVP_Cipher(m_pEvp, out->PutSpc(len), inbuf, len) != len )
			return FALSE;

		params[0].data = out->PutSpc(CipherTab[m_Index].tag_len);
		params[0].data_size = CipherTab[m_Index].tag_len;
		if ( EVP_CIPHER_CTX_get_params(m_pEvp, params) == 0 )
			return FALSE;

	} else if ( m_Mode == MODE_DEC ) {

		if ( (len -= CipherTab[m_Index].tag_len) < 4 )
			return FALSE;

		params[0].data = inbuf + len;
		params[0].data_size = CipherTab[m_Index].tag_len;
		if ( EVP_CIPHER_CTX_set_params(m_pEvp, params) == 0 )
			return FALSE;

		if ( EVP_Cipher(m_pEvp, NULL, NULL, len - 4) < 0 )
			return FALSE;

		// AAD Additional Authenticated Data uint32 Packet Length not deccrypt !!
		if ( EVP_Cipher(m_pEvp, NULL, inbuf, 4) < 0 )
			return FALSE;
		out->Apend(inbuf, 4);
		inbuf += 4;
		len   -= 4;

		if ( EVP_Cipher(m_pEvp, out->PutSpc(len), inbuf, len) != len )
			return FALSE;

	} else
		return FALSE;

	return TRUE;
}
int CCipher::Cipher(LPBYTE inbuf, int len, CBuffer *out)
{
	if ( m_Index == (-1) )
		return FALSE;

	switch(CipherTab[m_Index].num / 100) {
	case 0:
	case 1:
		if ( len > 0 && EVP_Cipher(m_pEvp, out->PutSpc(len), inbuf, len) == len )
			return TRUE;
		break;
	case 2:	// GCM
	case 4:	// AEAD
		return CipherAead(inbuf, len, out);
	case 3:	// CCM
		return CipherAeadCCM(inbuf, len, out);
	case 5:	// SSH CHACHAPOLY
		return CipherSshChahaPoly(inbuf, len, out);
	}

	return FALSE;
}
int CCipher::GetIndex(LPCTSTR name)
{
	int n;
	if ( name == NULL )
		return m_Index;
	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		if ( _tcscmp(name, CipherTab[n].name) == 0 )
			return n;
	}
	return (-1);
}
int CCipher::GetKeyLen(LPCTSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return 8;
	else
		return CipherTab[n].key_len;
}
int CCipher::GetBlockSize(LPCTSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return 8;
	else
		return CipherTab[n].block_size;
}
int CCipher::GetIvSize(LPCTSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return 8;
	else
		return (CipherTab[n].iv_len == 0 ? CipherTab[n].block_size : CipherTab[n].iv_len);
}
int CCipher::GetTagSize(LPCTSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return 0;
	else
		return CipherTab[n].tag_len;
}
BOOL CCipher::IsAEAD(LPCTSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return FALSE;
	else
		return (SSH2_CIPHER_GCM(CipherTab[n].num) || SSH2_CIPHER_CCM(CipherTab[n].num) || SSH2_CIPHER_AEAD(CipherTab[n].num) ? TRUE : FALSE);
}
BOOL CCipher::IsPOLY(LPCTSTR name)
{
	int n;
	if ( (n = GetIndex(name)) < 0 )
		return FALSE;
	else
		return (SSH2_CIPHER_POLY(CipherTab[n].num) ? TRUE : FALSE);
}
LPCTSTR CCipher::GetName(int num)
{
	int n;
	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		if ( num == CipherTab[n].num )
			return CipherTab[n].name;
	}
	return NULL;
}
int CCipher::GetNum(LPCTSTR str)
{
	int n;
	if ( (n = GetIndex(str)) < 0 )
		return 0;
	else
		return CipherTab[n].num;
}
LPCTSTR CCipher::GetTitle()
{
	if ( m_Index == (-1) )
		return _T("");
	return CipherTab[m_Index].name;
}
void CCipher::MakeKey(CBuffer *bp, LPCTSTR pass)
{
	int dlen;
	u_char digest[EVP_MAX_MD_SIZE];
	CStringA mbs = TstrToMbs(pass);

	dlen = MD_digest(EVP_md5(), (BYTE *)(LPCSTR)mbs, mbs.GetLength(), digest, sizeof(digest));

	bp->Clear();
	bp->Apend(digest, dlen);
}

void CCipher::BenchMark(CBuffer *out)
{
#if 0
	int n, i, c, e;
	CCipher enc, dec;
	clock_t st, ed;
	BYTE key[64], iv[64];
	BYTE tmp[1024];
	CBuffer enc_buf, dec_buf;
	double d;
	CStringA str;

	CString msg;
	clock_t total = clock();

	rand_buf(key, 64);
	rand_buf(iv, 64);
	rand_buf(tmp, 1024);

	for ( n = 0 ; CipherTab[n].name != NULL ; n++ ) {
		e = 0;
		if ( !enc.Init(CipherTab[n].name, MODE_ENC, key, (-1), iv) )
			e++;
		if ( !dec.Init(CipherTab[n].name, MODE_DEC, key, (-1), iv) )
			e++;
		st = ed = clock();
		if ( e == 0 ) {
			for ( c = 0 ; (ed = clock()) < (st + 1 * CLOCKS_PER_SEC) ; c++ ) {
				for ( i = 0 ; i < 100 ; i++ ) {
					enc_buf.Clear();
					enc.SetIvCounter(i);
					enc.Cipher(tmp, 1024, &enc_buf);
					dec_buf.Clear();
					dec.SetIvCounter(i);
					dec.Cipher(enc_buf.GetPtr(), enc_buf.GetSize(), &dec_buf);
					if ( dec_buf.GetSize() != 1024 || memcmp(dec_buf.GetPtr(), tmp, 1024) != 0 )
						e++;
				}
			}
			enc.Close();
			dec.Close();
		}

		d = (ed > st ? (double(c * 100) * CLOCKS_PER_SEC / (ed - st)) : 0);
		str.Format("%s\t\t%d.%03d(%d)\r\n", TstrToMbs(CipherTab[n].name), (int)d / 1000, (int)d % 1000, e);
		TRACE("%s", str);

		if ( out != NULL )
			*out += (LPCSTR)str;
	}

	total = clock() - total;
	msg.Format(_T("Total %d.%03d sec"), (int)(total / CLOCKS_PER_SEC), (int)(total % CLOCKS_PER_SEC));
	::AfxMessageBox(msg, MB_ICONINFORMATION);
#endif
}

//////////////////////////////////////////////////////////////////////
// CMacomp
//////////////////////////////////////////////////////////////////////

static const struct _MacTab {
        LPCTSTR			name;
        const EVP_MD *  (*func)(void);
        int             trunbits;
		BOOL			umac;
		int				key_len;
		int				block_len;
		BOOL			etm;
		BOOL			aead;
} MacsTab[] = {
	{ _T("hmac-md5"),						(const EVP_MD *((*)()))EVP_md5,			 0,	FALSE, -1, -1, FALSE, FALSE },	// RFC 4253
	{ _T("hmac-md5-96"),					(const EVP_MD *((*)()))EVP_md5,			96,	FALSE, -1, -1, FALSE, FALSE },	// RFC 4253

	{ _T("hmac-sha1"),						(const EVP_MD *((*)()))EVP_sha1,	  	 0,	FALSE, -1, -1, FALSE, FALSE },	// RFC 4253	REQUIRED
	{ _T("hmac-sha1-96"),					(const EVP_MD *((*)()))EVP_sha1,		96,	FALSE, -1, -1, FALSE, FALSE },	// RFC 4253

	{ _T("hmac-sha2-256"),					(const EVP_MD *((*)()))EVP_sha256,	  	 0,	FALSE, -1, -1, FALSE, FALSE },	// RFC 6668
	{ _T("hmac-sha2-256-96"),				(const EVP_MD *((*)()))EVP_sha256,		96,	FALSE, -1, -1, FALSE, FALSE },
	{ _T("hmac-sha2-512"),					(const EVP_MD *((*)()))EVP_sha512,	  	 0,	FALSE, -1, -1, FALSE, FALSE },	// RFC 6668
	{ _T("hmac-sha2-512-96"),				(const EVP_MD *((*)()))EVP_sha512,		96,	FALSE, -1, -1, FALSE, FALSE },

	{ _T("hmac-ripemd160"),					(const EVP_MD *((*)()))EVP_ripemd160,	 0,	FALSE, -1, -1, FALSE, FALSE },
	{ _T("hmac-whirlpool"),					(const EVP_MD *((*)()))EVP_whirlpool,	 0,	FALSE, -1, -1, FALSE, FALSE },

	{ _T("umac-32"),						(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16,  4, FALSE, FALSE },
	{ _T("umac-64"),						(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16,  8, FALSE, FALSE },
	{ _T("umac-96"),						(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16, 12, FALSE, FALSE },
	{ _T("umac-128"),						(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16, 16, FALSE, FALSE },

	{ _T("umac-64@openssh.com"),			(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16,  8, FALSE, FALSE },
	{ _T("umac-128@openssh.com"),			(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16, 16, FALSE, FALSE },

	{ _T("hmac-md5-etm@openssh.com"),		(const EVP_MD *((*)()))EVP_md5,			 0,	FALSE, -1, -1, TRUE, FALSE },
	{ _T("hmac-sha1-etm@openssh.com"),		(const EVP_MD *((*)()))EVP_sha1,	  	 0,	FALSE, -1, -1, TRUE, FALSE },
	{ _T("hmac-sha2-256-etm@openssh.com"),	(const EVP_MD *((*)()))EVP_sha256,	  	 0,	FALSE, -1, -1, TRUE, FALSE },
	{ _T("hmac-sha2-512-etm@openssh.com"),	(const EVP_MD *((*)()))EVP_sha512,	  	 0,	FALSE, -1, -1, TRUE, FALSE },
	{ _T("umac-64-etm@openssh.com"),		(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16,  8, TRUE, FALSE },
	{ _T("umac-128-etm@openssh.com"),		(const EVP_MD *((*)()))NULL,			 0,	 TRUE, 16, 16, TRUE, FALSE },

	{ _T("aes128-gcm@openssh.com"),			(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// RFC 5647
	{ _T("aes256-gcm@openssh.com"),			(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// RFC 5647
	{ _T("aes128-gcm"),						(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// draft-miller-sshm-aes-gcm-00
	{ _T("aes256-gcm"),						(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// draft-miller-sshm-aes-gcm-00

	{ _T("AEAD_AES_128_GCM"),				(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// RFC 5647
	{ _T("AEAD_AES_256_GCM"),				(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// RFC 5647

	{ _T("AEAD_AES_128_CCM"),				(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// RFC 5647
	{ _T("AEAD_AES_256_CCM"),				(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// RFC 5647

	{ _T("chacha20-poly1305@openssh.com"),	(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },
	{ _T("chacha20-poly1305"),				(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },	// draft-ietf-sshm-chacha20-poly1305-00
	{ _T("AEAD_CHACHA20_POLY1305"),			(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, TRUE },

	{ _T("none"),							(const EVP_MD *((*)()))NULL,			 0,	FALSE, -1, -1, FALSE, FALSE },	// RFC 4253
    { NULL, NULL, 0 }
};

CMacomp::CMacomp()
{
	m_Index = (-1);
	m_Md = NULL;
	m_KeyLen = 0;
	m_KeyBuf = NULL;
	m_BlockSize = 0;
	m_UmacCtx = NULL;
	m_UmacMode = FALSE;
	m_EtmMode  = FALSE;
#ifdef	USE_MACCTX
	m_pMacCtx = NULL;
	m_pMac = NULL;
#else
	m_pHmac = NULL;
#endif
}
CMacomp::~CMacomp()
{
	Close();
}
void CMacomp::Close()
{
	if ( m_KeyBuf != NULL ) {
		delete [] m_KeyBuf;
		m_KeyBuf = NULL;
	}

#ifdef	USE_MACCTX
	if ( m_pMacCtx != NULL ) {
		EVP_MAC_CTX_free(m_pMacCtx);
		m_pMacCtx = NULL;
	}
	if ( m_pMac != NULL ) {
		EVP_MAC_free(m_pMac);
		m_pMac = NULL;
	}
#else
	if ( m_pHmac != NULL ) {
		HMAC_CTX_free(m_pHmac);
		m_pHmac = NULL;
	}
#endif

	if ( m_UmacCtx != NULL ) {
		UMAC_close((struct umac_ctx *)m_UmacCtx);
		m_UmacCtx = FALSE;
	}

	m_Index = (-1);
	m_Md = NULL;
	m_KeyLen = m_BlockSize = 0;
	m_UmacCtx = NULL;
	m_UmacMode = FALSE;
	m_EtmMode  = FALSE;
	m_BlockSize = 0;
}
int CMacomp::Init(LPCTSTR name, LPBYTE key, int len)
{
	int n;

	Close();

	for ( n = 0 ; MacsTab[n].name != NULL ; n++ ) {
		if ( _tcscmp(MacsTab[n].name, name) == 0 ) {
			if ( MacsTab[n].func != NULL ) {
				if ( (m_Md = (*(MacsTab[n].func))()) == NULL )
					return FALSE;
				m_KeyLen = m_BlockSize = EVP_MD_size(m_Md);
				if ( MacsTab[n].trunbits != 0 )
					m_BlockSize = MacsTab[n].trunbits / 8;
				m_UmacMode = FALSE;

			} else if ( MacsTab[n].umac ) {
				m_KeyLen    = MacsTab[n].key_len;
				m_BlockSize = MacsTab[n].block_len;
				m_UmacCtx = UMAC_open(m_BlockSize);
				if ( MacsTab[n].trunbits != 0 )
					m_BlockSize = MacsTab[n].trunbits / 8;
				m_UmacMode = TRUE;
			}

			if ( MacsTab[n].etm )
				m_EtmMode = TRUE;
			break;
		}
	}

	if ( MacsTab[n].name == NULL )
		return FALSE;

	if ( len < 0 )
		len = m_KeyLen;
	else
		m_KeyLen = len;

#ifdef	USE_MACCTX
	const char *mdname;
	OSSL_PARAM params[2];

	if ( m_Md != NULL ) {
		if ( (m_pMac = EVP_MAC_fetch(NULL, "hmac", NULL)) == NULL )
			goto ERRENDOF;

		if ( (mdname = EVP_MD_get0_name(m_Md)) == NULL )
			goto ERRENDOF;

		params[0] = OSSL_PARAM_construct_utf8_string(OSSL_ALG_PARAM_DIGEST, (char *)mdname, 0);
		params[1] = OSSL_PARAM_construct_end();
	}

	if ( key != NULL ) {
		m_KeyBuf = new BYTE[len];
		memcpy(m_KeyBuf, key, len);

		if ( m_UmacMode )
			UMAC_init((struct umac_ctx *)m_UmacCtx, m_KeyBuf, m_KeyLen);

		else if ( m_Md != NULL ) {
			if ( (m_pMacCtx = EVP_MAC_CTX_new(m_pMac)) == NULL )
				goto ERRENDOF;

			if ( EVP_MAC_init(m_pMacCtx, (const unsigned char *)m_KeyBuf, (size_t)m_KeyLen, params) == 0 )
				goto ERRENDOF;
		}

	} else if ( m_Md != NULL ) {
		if ( (m_pMacCtx = EVP_MAC_CTX_new(m_pMac)) == NULL )
			goto ERRENDOF;

		if ( EVP_MAC_init(m_pMacCtx, NULL, 0, params) == 0 )
			goto ERRENDOF;

	} else
		goto ERRENDOF;

	m_Index = n;
	return TRUE;

ERRENDOF:
	if ( m_pMacCtx != NULL ) {
		EVP_MAC_CTX_free(m_pMacCtx);
		m_pMacCtx = NULL;
	}
	if ( m_pMac != NULL ) {
		EVP_MAC_free(m_pMac);
		m_pMac = NULL;
	}
	return FALSE;

#else
	if ( key != NULL ) {
		m_KeyBuf = new BYTE[len];
		memcpy(m_KeyBuf, key, len);

		if ( m_UmacMode )
			UMAC_init((struct umac_ctx *)m_UmacCtx, m_KeyBuf, m_KeyLen);

		else if ( m_Md != NULL ) {
			if ( (m_pHmac = HMAC_CTX_new()) == NULL )
				goto ERRENDOF;

			if ( HMAC_Init(m_pHmac, m_KeyBuf, m_KeyLen, m_Md) == NULL )
				goto ERRENDOF;
		}

	} else if ( m_Md != NULL ) {
		if ( (m_pHmac = HMAC_CTX_new()) == NULL )
			goto ERRENDOF;

		if ( HMAC_Init(m_pHmac, NULL, 0, m_Md) == NULL )
			goto ERRENDOF;

	} else
		goto ERRENDOF;

	m_Index = n;
	return TRUE;

ERRENDOF:
	if ( m_pHmac != NULL ) {
		HMAC_CTX_free(m_pHmac);
		m_pHmac = NULL;
	}
	return FALSE;
#endif
}
void CMacomp::Compute(DWORD seq, LPBYTE inbuf, int len, CBuffer *out)
{
	u_char b[8];
	u_char tmp[EVP_MAX_MD_SIZE];

	if ( m_Index == (-1) )
		return;

	else if ( m_UmacMode ) {
		if ( m_KeyBuf == NULL || m_UmacCtx == NULL || m_BlockSize<= 0 || m_BlockSize > EVP_MAX_MD_SIZE )
			return;

	 	b[0] = (BYTE)0;
		b[1] = (BYTE)0;
		b[2] = (BYTE)0;
		b[3] = (BYTE)0;
	 	b[4] = (BYTE)(seq >> 24);
		b[5] = (BYTE)(seq >> 16);
		b[6] = (BYTE)(seq >> 8);
		b[7] = (BYTE)seq;

		UMAC_update((struct umac_ctx *)m_UmacCtx, inbuf, len);
		UMAC_final((struct umac_ctx *)m_UmacCtx, tmp, b);

		out->Apend(tmp, m_BlockSize);

	} else {
#ifdef	USE_MACCTX
		if ( m_pMacCtx == NULL || m_BlockSize<= 0 || m_BlockSize > EVP_MAX_MD_SIZE )
			return;

		if ( EVP_MAC_init(m_pMacCtx, (const unsigned char *)m_KeyBuf, (size_t)m_KeyLen, NULL) == 0 )
			return;

		b[0] = (BYTE)(seq >> 24);
		b[1] = (BYTE)(seq >> 16);
		b[2] = (BYTE)(seq >> 8);
		b[3] = (BYTE)seq;

		if ( EVP_MAC_update(m_pMacCtx, b, 4) == 0 || EVP_MAC_update(m_pMacCtx, inbuf, len) == 0 || EVP_MAC_final(m_pMacCtx, tmp, NULL, sizeof(tmp)) == 0 )
			return;

		out->Apend(tmp, m_BlockSize);

#else
		if ( m_pHmac == NULL || m_BlockSize<= 0 || m_BlockSize > EVP_MAX_MD_SIZE )
			return;

		if ( HMAC_Init(m_pHmac, NULL, 0, NULL) == 0 )
			return;

		b[0] = (BYTE)(seq >> 24);
		b[1] = (BYTE)(seq >> 16);
		b[2] = (BYTE)(seq >> 8);
		b[3] = (BYTE)seq;

		if ( HMAC_Update(m_pHmac, b, 4) == 0 || HMAC_Update(m_pHmac, inbuf, len) == 0 || HMAC_Final(m_pHmac, tmp, NULL) == 0 )
			return;

		out->Apend(tmp, m_BlockSize);
#endif	
	}
}
int CMacomp::GetIndex(LPCTSTR name)
{
	int n;
	if ( name == NULL )
		return m_Index;
	for ( n = 0 ; MacsTab[n].name != NULL ; n++ ) {
		if ( _tcscmp(MacsTab[n].name, name) == 0 )
			return n;
	}
	return (-1);
}
BOOL CMacomp::IsAEAD(LPCTSTR name)
{
	int n;

	if ( (n = GetIndex(name)) < 0 )
		return FALSE;

	return MacsTab[n].aead;
}
int CMacomp::GetKeyLen(LPCTSTR name)
{
	int n;
	const EVP_MD *md;

	if ( (n = GetIndex(name)) < 0 )
		return 0;
	else if ( n == m_Index )
		return m_KeyLen;

	if ( MacsTab[n].umac )
		return MacsTab[n].key_len;

	if ( MacsTab[n].func == NULL )
		return 0;

	if ( (md = (*(MacsTab[n].func))()) == NULL )
		return 0;

	return EVP_MD_size(md);
}
int CMacomp::GetBlockSize(LPCTSTR name)
{
	int n;
	const EVP_MD *md;

	if ( (n = GetIndex(name)) < 0 )
		return 0;
	else if ( n == m_Index )
		return m_BlockSize;

	if ( MacsTab[n].umac )
		return MacsTab[n].block_len;

	if ( MacsTab[n].func == NULL )
		return 0;

	if ( MacsTab[n].trunbits != 0 )
		return (MacsTab[n].trunbits / 8);

	if ( (md = (*(MacsTab[n].func))()) == NULL )
		return 0;

	return EVP_MD_size(md);
}
LPCTSTR CMacomp::GetTitle()
{
	if ( m_Index < 0 )
		return _T("");
	return MacsTab[m_Index].name;
}
int CMacomp::SpeedCheck()
{
	int n;
	CMacomp mac;
	clock_t st;
	BYTE tmp[1024];
	CBuffer mac_buf;

	st = clock();

	mac.Init(_T("hmac-sha2-512"), tmp);

	for ( n = 0 ; n < 70000 ; n++ ) {
		mac_buf.Clear();
		mac.Compute(n, tmp, 1024, &mac_buf);
	}

	return (clock() - st);
}
void CMacomp::BenchMark(CBuffer *out)
{
#if 0
	int n, i, c;
	CMacomp mac;
	clock_t st, ed;
	BYTE iv[64];
	BYTE tmp[1024];
	CBuffer mac_buf;
	double d;
	CStringA str;

	for ( n = 0 ; n < 64 ; n += 2 ) {
		i = rand();
		iv[n + 0] = (BYTE)(i >>  8);
		iv[n + 1] = (BYTE)(i >>  0);
	}

	for ( n = 0 ; n < 1024 ; n += 2 ) {
		i = rand();
		tmp[n + 0] = (BYTE)(i >>  8);
		tmp[n + 1] = (BYTE)(i >>  0);
	}

	for ( n = 0 ; MacsTab[n].name != NULL ; n++ ) {
		if ( MacsTab[n].aead )
			continue;
		if ( !mac.Init(MacsTab[n].name, iv) && mac.GetBlockSize(NULL) > 0 ) {
			st = ed = clock();
			for ( c = 0 ; (ed = clock()) < (st + 1 * CLOCKS_PER_SEC) ; c++ ) {
				for ( i = 0 ; i < 100 ; i++ ) {
					mac_buf.Clear();
					mac.Compute(i, tmp, 1024, &mac_buf);
					if ( mac_buf.GetSize() == 0 )
						break;
				}
			}

			d = (ed <= st ? 0 : double(c * 100) * CLOCKS_PER_SEC / (ed - st));
			str.Format("%s\t\t%d.%03d\r\n", TstrToMbs(MacsTab[n].name), (int)d / 1000, (int)d % 1000);
		} else
			str.Format("%s\t\t0.0\r\n", TstrToMbs(MacsTab[n].name));

		TRACE(str);

		if ( out != NULL )
			*out += (LPCSTR)str;
	}
#endif
}

//////////////////////////////////////////////////////////////////////
// CCompress
//////////////////////////////////////////////////////////////////////

static const struct _CompTab {
        LPCTSTR		name;
} CompTab[] = { 
	{ _T("zlib")				},
	{ _T("none")				},
	{ _T("zlib@openssh.com")	},
	{ NULL						}
};

CCompress::CCompress()
{
	m_Mode = 0;
	ZeroMemory(&m_InCompStr, sizeof(m_InCompStr));
	ZeroMemory(&m_OutCompStr, sizeof(m_OutCompStr));
}
CCompress::~CCompress()
{
	if ( (m_Mode & 1) != 0 )
		inflateEnd(&m_InCompStr);

	if ( (m_Mode & 2) != 0 )
		deflateEnd(&m_OutCompStr);
}
int CCompress::Init(LPCTSTR name, int mode, int level)
{
	if ( (m_Mode & 1) != 0 )
		inflateEnd(&m_InCompStr);

	if ( (m_Mode & 2) != 0 )
		deflateEnd(&m_OutCompStr);

	m_Mode = 0;

	if ( name != NULL ) {
		if ( _tcscmp(name, _T("zlib@openssh.com")) == 0 ) {
		    m_Mode = 4;
			return TRUE;
		} else if ( _tcscmp(name, _T("zlib")) != 0 )
			return TRUE;
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

	return TRUE;
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
			throw _T("compress error");
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
			throw _T("uncompress error");
		out->Apend(buf, sizeof(buf) - m_InCompStr.avail_out);
	}
}
LPCTSTR CCompress::GetTitle()
{
	return CompTab[m_Mode == 0 ? 1 : ((m_Mode & 4) ? 2 : 0)].name;
}

//////////////////////////////////////////////////////////////////////
// CXmssKey

CXmssKey::CXmssKey()
{
	m_oId = 0;
	m_PubLen = m_SecLen = 0;
	m_pPubBuf = m_pSecBuf = NULL;
	m_PassLen = 0;
	m_pPassBuf = NULL;
}
CXmssKey::~CXmssKey()
{
	RemoveAll();
}
void CXmssKey::RemoveAll()
{
	if ( m_pPubBuf != NULL )
		delete [] m_pPubBuf;

	if ( m_pSecBuf != NULL ) {
		SecureZeroMemory(m_pSecBuf, m_SecLen);
		delete [] m_pSecBuf;
	}

	if ( m_pPassBuf != NULL ) {
		SecureZeroMemory(m_pPassBuf, m_PassLen * sizeof(TCHAR));
		delete [] m_pPassBuf;
	}

	m_pPubBuf = m_pSecBuf = NULL;
	m_pPassBuf = NULL;

	m_EncName.Empty();

	if ( m_EncIvBuf.GetSize() > 0 )
		SecureZeroMemory(m_EncIvBuf.GetPtr(), m_EncIvBuf.GetSize());
	m_EncIvBuf.Clear();
}
const CXmssKey & CXmssKey::operator = (CXmssKey &data)
{
	RemoveAll();

	m_oId = data.m_oId;

	m_EncName  = data.m_EncName;
	m_EncIvBuf = data.m_EncIvBuf;

	if ( data.m_pPubBuf != NULL ) {
		m_PubLen = data.m_PubLen;
		m_pPubBuf = new BYTE[m_PubLen];
		memcpy(m_pPubBuf, data.m_pPubBuf, m_PubLen);
	}

	if ( data.m_pSecBuf != NULL ) {
		m_SecLen = data.m_SecLen;
		m_pSecBuf = new BYTE[m_SecLen];
		memcpy(m_pSecBuf, data.m_pSecBuf, m_SecLen);
	}

	if ( data.m_pPassBuf != NULL ) {
		m_PassLen = data.m_PassLen;
		m_pPassBuf = new TCHAR[m_PassLen];
		memcpy(m_pPassBuf, data.m_pPassBuf, m_PassLen * sizeof(TCHAR));
	}

	return *this;
}
BOOL CXmssKey::SetBufSize(uint32_t oId)
{
	RemoveAll();

	if ( oId != 0 )
		m_oId = oId;

	if ( xmss_key_bytes(m_oId, &m_PubLen, &m_SecLen) != 0 )
		return FALSE;

	m_pPubBuf = new BYTE[m_PubLen];
	m_pSecBuf = new BYTE[m_SecLen];

	ZeroMemory(m_pPubBuf, m_PubLen);
	ZeroMemory(m_pSecBuf, m_SecLen);

    for ( int i = 0 ; i < XMSS_OID_LEN ; i++) {
		if ( m_PubLen > (XMSS_OID_LEN - i - 1) )
			m_pPubBuf[XMSS_OID_LEN - i - 1] = (BYTE)(m_oId >> (8 * i));
		if ( m_SecLen > (XMSS_OID_LEN - i - 1) )
			m_pSecBuf[XMSS_OID_LEN - i - 1] = (BYTE)(m_oId >> (8 * i));
	}

	return TRUE;
}
BOOL CXmssKey::SetBufSize(LPCSTR name)
{
	uint32_t oId;

	if ( xmss_str_to_oid(&oId, name) != 0 )
		return FALSE;

	return SetBufSize(oId);
}
void CXmssKey::SetPassBuf(LPCTSTR pass)
{
	if ( m_pPassBuf != NULL ) {
		SecureZeroMemory(m_pPassBuf, m_PassLen * sizeof(TCHAR));
		delete [] m_pPassBuf;
	}
	m_pPassBuf = NULL;

	if ( pass != NULL ) {
		m_PassLen = (int)_tcslen(pass) + 1;
		m_pPassBuf = new TCHAR[m_PassLen];
		_tcscpy(m_pPassBuf, pass);
	}
}
BOOL CXmssKey::MakeEncIvBuf()
{
	if ( m_EncName.IsEmpty() || m_EncIvBuf.GetSize() <= 0 ) {
		CCipher cip;
		m_EncName = "aes256-gcm@openssh.com";
		m_EncIvBuf.Clear();
		m_EncIvBuf.PutSpc(cip.GetKeyLen(MbsToTstr(m_EncName)) + cip.GetIvSize(MbsToTstr(m_EncName)));
		rand_buf(m_EncIvBuf.GetPtr(), m_EncIvBuf.GetSize());
		return TRUE;
	}

	return FALSE;
}
BOOL CXmssKey::LoadOpenSshSec(CBuffer *bp)
{
	if ( xmss_load_openssh_sk(m_oId, m_pSecBuf, bp, &m_EncName, &m_EncIvBuf) != 0 )
		return FALSE;

	return TRUE;
}
BOOL CXmssKey::SaveOpenSshSec(CBuffer *bp)
{
	MakeEncIvBuf();

	if ( xmss_save_openssh_sk(m_oId, m_pSecBuf, bp, m_EncName, m_EncIvBuf.GetPtr(), m_EncIvBuf.GetSize()) != 0 )
		return FALSE;

	return TRUE;
}
BOOL CXmssKey::LoadStateFile(LPCTSTR fileName)
{
	int n, i, a;
	CCipher cip;
	int keylen, ivlen;
	CBuffer tmp, dec;
	CBuffer index, state;
	CString baseName;
	CString statFile;
	CFile file;

	// Not File Entry !?
	if ( m_EncName.IsEmpty() || m_EncIvBuf.GetSize() <= 0 )
		return FALSE;

	if ( m_pSecBuf == NULL || m_SecLen <= 0 )
		return FALSE;

	keylen = cip.GetKeyLen(MbsToTstr(m_EncName));
	ivlen  = cip.GetIvSize(MbsToTstr(m_EncName));
	
	if ( m_EncIvBuf.GetSize() < (keylen + ivlen) )
		return FALSE;

	if ( !cip.Init(MbsToTstr(m_EncName), MODE_DEC, m_EncIvBuf.GetPtr(), keylen, m_EncIvBuf.GetPtr() + keylen) )
		return FALSE;

	baseName = fileName;
	if ( (n = baseName.ReverseFind(_T('.'))) >= 0 )
		baseName.Delete(n, baseName.GetLength() - n);

	statFile.Format(_T("%s.xmss"), (LPCTSTR)baseName);

	try {
		if ( !file.Open(statFile, CFile::modeRead | CFile::shareExclusive) )
			return FALSE;

		n = (int)file.GetLength();
		file.Read(tmp.PutSpc(n), n);
		file.Close();

		if ( !cip.Cipher(tmp.GetPtr(), tmp.GetSize(), &dec) )
			return FALSE;

		tmp.Clear(); dec.GetBuf(&tmp);
		if ( strcmp((LPCSTR)tmp, "xmss-state-v1") != 0 )
			return FALSE;

		tmp.Clear(); dec.GetBuf(&tmp);
		if ( m_PubLen != tmp.GetSize() || memcmp(m_pPubBuf, tmp.GetPtr(), m_PubLen) != 0 )
			return FALSE;

		if ( xmss_sk_bytes(m_oId, &i, &n) != 0 )
			return FALSE;
		a = m_SecLen - (XMSS_OID_LEN + i + n);

		dec.GetBuf(&index);		// index
		if ( index.GetSize() != i )
			return FALSE;

		dec.GetBuf(&state);		// state
		if ( state.GetSize() != a )
			return FALSE;

		memcpy(m_pSecBuf + XMSS_OID_LEN, index.GetPtr(), index.GetSize()); 
		memcpy(m_pSecBuf + XMSS_OID_LEN + i + n, state.GetPtr(), state.GetSize()); 

		return TRUE;

	} catch(...) {
		::AfxMessageBox(_T("xmss state file read error"), MB_ICONERROR);
		return FALSE;
	}
}
BOOL CXmssKey::SaveStateFile(LPCTSTR fileName)
{
	int n, i;
	CCipher cip;
	int keylen, ivlen;
	CBuffer tmp, enc;
	CString baseName;
	CString statFile, oldFile;
	CFile file;

	// Not File Entry !?
	if ( m_EncName.IsEmpty() || m_EncIvBuf.GetSize() <= 0 )
		return FALSE;

	if ( m_pSecBuf == NULL || m_SecLen <= 0 )
		return FALSE;

	keylen = cip.GetKeyLen(MbsToTstr(m_EncName));
	ivlen  = cip.GetIvSize(MbsToTstr(m_EncName));
	
	if ( m_EncIvBuf.GetSize() < (keylen + ivlen) )
		return FALSE;

	if ( !cip.Init(MbsToTstr(m_EncName), MODE_ENC, m_EncIvBuf.GetPtr(), keylen, m_EncIvBuf.GetPtr() + keylen) )
		return FALSE;

	tmp.PutStr("xmss-state-v1");
	tmp.PutBuf(m_pPubBuf, m_PubLen);

	if ( xmss_sk_bytes(m_oId, &i, &n) != 0 )
		return FALSE;

	tmp.PutBuf(m_pSecBuf + XMSS_OID_LEN, i);											// index
	tmp.PutBuf(m_pSecBuf + XMSS_OID_LEN + i + n, m_SecLen - (XMSS_OID_LEN + i + n));	// state

	if ( !cip.Cipher(tmp.GetPtr(), tmp.GetSize(), &enc) )
		return FALSE;

	baseName = fileName;
	if ( (n = baseName.ReverseFind(_T('.'))) >= 0 )
		baseName.Delete(n, baseName.GetLength() - n);

	statFile.Format(_T("%s.xmss"), (LPCTSTR)baseName);
	oldFile.Format(_T("%s.omss"), (LPCTSTR)baseName);

	// no error check...
	DeleteFile(oldFile);
	_trename(statFile, oldFile);

	try {
		if ( !file.Open(statFile, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive) )
			return FALSE;

		file.Write(enc.GetPtr(), enc.GetSize());
		file.Close();

		DeleteFile(oldFile);

		return TRUE;

	} catch(...) {
		return FALSE;
	}
}
const char *CXmssKey::GetName()
{
	return xmss_oid_to_str(m_oId);
}
int CXmssKey::GetBits()
{
	return xmss_bits(m_oId);
}
int CXmssKey::GetHeight()
{ 
	return xmss_height(m_oId);
}
int CXmssKey::KeyPair(GENSTATUS *gs)
{
	return xmss_keypair(m_pPubBuf, m_pSecBuf, m_oId, gs);
}
int CXmssKey::GetSignByte()
{ 
	return xmss_sign_bytes(m_oId);
}
int CXmssKey::Sign(unsigned char *sm, unsigned long long *smlen, 
		const unsigned char *m, unsigned long long mlen)
{
	return xmss_sign(m_pSecBuf, sm, smlen, m, mlen); 
}
int CXmssKey::Verify(unsigned char *m, unsigned long long *mlen, 
		const unsigned char *sm, unsigned long long smlen)
{
	return xmss_sign_open(m, mlen, sm, smlen, m_pPubBuf);
}

//////////////////////////////////////////////////////////////////////
// CIdKey

CIdKey::CIdKey()
{
	m_Uid  = (-1);
	m_Name = _T("");
	m_Hash = _T("");
	m_Type = IDKEY_NONE;
	m_Rsa  = NULL;
	m_Dsa  = NULL;
	m_Flag = FALSE;
	m_EcDsa = NULL;
	m_Cert = 0;
	m_bSecInit = FALSE;
	m_bHostPass = TRUE;
	m_AgeantType = IDKEY_AGEANT_NONE;
	m_PublicKey.m_bZero = TRUE;
	m_PrivateKey.m_bZero = TRUE;
	m_Nid = NID_undef;
}
CIdKey::~CIdKey()
{
	if ( m_Rsa != NULL )
		RSA_free(m_Rsa);

	if ( m_Dsa != NULL )
		DSA_free(m_Dsa);

	if ( m_EcDsa != NULL )
		EC_KEY_free(m_EcDsa);
}
int CIdKey::Init(LPCTSTR pass)
{
	if ( m_Type == IDKEY_NONE )
		return FALSE;

	if ( m_bSecInit || m_AgeantType != IDKEY_AGEANT_NONE )
		return TRUE;

	if ( CompPass(pass) != 0 )
		return FALSE;

	if ( !ReadPrivateKey(m_SecBlob, pass, m_bHostPass) )
		return FALSE;

	if ( m_bHostPass )
		WritePrivateKey(m_SecBlob, pass);

	if ( m_Type == IDKEY_XMSS )
		m_XmssKey.SetPassBuf(pass);

	m_bSecInit = TRUE;

	return TRUE;
}
void CIdKey::SetString(CString &str)
{
	CString tmp;
	CStringArrayExt stra;

	stra.AddVal(m_Uid);
	stra.Add(m_Name);
	stra.Add(_T("=remove"));	//	EncryptStr(tmp, m_Pass);
	WritePublicKey(tmp);
	stra.Add(tmp);
	stra.Add(m_SecBlob);
	stra.Add(m_Hash);
	stra.AddVal(m_bHostPass);
	stra.AddVal(m_AgeantType);

	stra.SetString(str);
}
int CIdKey::GetString(LPCTSTR str)
{
	CString tmp;
	CStringArrayExt stra;

	stra.GetString(str);

	if ( stra.GetSize() < 5 )
		return FALSE;

	m_Uid  = stra.GetVal(0);
	m_Name = stra.GetAt(1);

	if ( !ReadPublicKey(stra.GetAt(3)) )
		return FALSE;

	m_SecBlob = stra.GetAt(4);
	m_bSecInit = FALSE;

	m_bHostPass  = (stra.GetSize() > 6 ? stra.GetVal(6) : TRUE);
	m_AgeantType = (stra.GetSize() > 7 ? stra.GetVal(7) : IDKEY_AGEANT_NONE);

	if ( stra.GetAt(2).Compare(_T("=remove")) != 0 ) {
		DecryptStr(tmp, stra.GetAt(2), FALSE);
		SetPass(tmp);
		if ( m_bHostPass ) {
			if ( !ReadPrivateKey(m_SecBlob, tmp, m_bHostPass) )
				return FALSE;
			WritePrivateKey(m_SecBlob, tmp);
		}
	} else if ( stra.GetSize() > 5 )
		m_Hash = stra.GetAt(5);
	else
		return FALSE;

	return TRUE;
}
int CIdKey::GetProfile(LPCTSTR pSection, int Uid)
{
	CString ent, str;
	ent.Format(_T("List%02d"), Uid);
	str = AfxGetApp()->GetProfileString(pSection, ent, _T(""));
	return GetString(str);
}
void CIdKey::SetProfile(LPCTSTR pSection)
{
	CString ent, str;
	ASSERT(m_Uid >= 0);
	ent.Format(_T("List%02d"), m_Uid);
	SetString(str);
	AfxGetApp()->WriteProfileString(pSection, ent, str);
}
void CIdKey::DelProfile(LPCTSTR pSection)
{
	CString ent;
	ASSERT(m_Uid >= 0);
	ent.Format(_T("List%02d"), m_Uid);
	((CRLoginApp *)AfxGetApp())->DelProfileEntry(pSection, ent);
}
const CIdKey & CIdKey::operator = (CIdKey &data)
{
	Close();

	m_Uid  = data.m_Uid;
	m_Name = data.m_Name;
	m_Hash = data.m_Hash;
	m_Flag = data.m_Flag;
	m_Nid  = data.m_Nid;

	m_bSecInit   = data.m_bSecInit;
	m_SecBlob    = data.m_SecBlob;
	m_bHostPass  = data.m_bHostPass;
	m_AgeantType = data.m_AgeantType;

	m_Cert     = data.m_Cert;
	m_CertBlob = data.m_CertBlob;

	m_FilePath = data.m_FilePath;

	switch(data.m_Type) {
	case IDKEY_NONE:
		m_Type = IDKEY_NONE;
		break;
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		if ( data.m_Rsa == NULL )
			break;
		if ( !Create(data.m_Type) )
			break;
		{
			BIGNUM const *n = NULL, *e = NULL, *d = NULL, *p = NULL, *q = NULL;
			BIGNUM const *dmp1 = NULL, *dmq1 = NULL, *iqmp = NULL;

			RSA_get0_key(data.m_Rsa, &n, &e, &d);
			RSA_get0_factors(data.m_Rsa, &p, &q);
			RSA_get0_crt_params(data.m_Rsa, &dmp1, &dmq1, &iqmp);

			if ( n != NULL ) n = BN_dup(n);
			if ( e != NULL ) e = BN_dup(e);
			if ( d != NULL ) d = BN_dup(d);
			if ( p != NULL ) p = BN_dup(p);
			if ( q != NULL ) q = BN_dup(q);
			if ( dmp1 != NULL ) dmp1 = BN_dup(dmp1);
			if ( dmq1 != NULL ) dmq1 = BN_dup(dmq1);
			if ( iqmp != NULL ) iqmp = BN_dup(iqmp);

			RSA_set0_key(m_Rsa, (BIGNUM *)n, (BIGNUM *)e, (BIGNUM *)d);
			RSA_set0_factors(m_Rsa, (BIGNUM *)p, (BIGNUM *)q);
			RSA_set0_crt_params(m_Rsa, (BIGNUM *)dmp1, (BIGNUM *)dmq1, (BIGNUM *)iqmp);
		}
		break;
	case IDKEY_DSA2:
		if ( data.m_Dsa == NULL )
			break;
		if ( !Create(data.m_Type) )
			break;
		{
			BIGNUM const *p = NULL, *q = NULL, *g = NULL;
			BIGNUM const *pub_key = NULL, *priv_key = NULL;

			DSA_get0_pqg(data.m_Dsa, &p, &q, &g);
			DSA_get0_key(data.m_Dsa, &pub_key, &priv_key);

			if ( p != NULL ) p = BN_dup(p);
			if ( q != NULL ) q = BN_dup(q);
			if ( g != NULL ) g = BN_dup(g);
			if ( pub_key != NULL ) pub_key = BN_dup(pub_key);
			if ( priv_key != NULL ) priv_key = BN_dup(priv_key);

			DSA_set0_pqg(m_Dsa, (BIGNUM *)p, (BIGNUM *)q, (BIGNUM *)g);
			DSA_set0_key(m_Dsa, (BIGNUM *)pub_key, (BIGNUM *)priv_key);
		}
		break;
	case IDKEY_ECDSA:
		if ( data.m_EcDsa == NULL )
			break;
		if ( m_EcDsa != NULL )
			EC_KEY_free(m_EcDsa);
		m_EcDsa = EC_KEY_dup(data.m_EcDsa);
		m_Type  = IDKEY_ECDSA;
		break;
	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		m_PublicKey  = data.m_PublicKey;
		m_PrivateKey = data.m_PrivateKey;
		m_Type = data.m_Type;
		break;
	case IDKEY_XMSS:
		m_XmssKey = data.m_XmssKey;
		m_Type  = IDKEY_XMSS;
		break;
	case IDKEY_UNKNOWN:
		m_TypeName = data.m_TypeName;
		m_TypeBlob = data.m_TypeBlob;
		m_Type  = IDKEY_UNKNOWN;
		break;
	}

	return *this;
}

static const struct _NIDListTab {
	int			nid;
	LPCTSTR		name;
	int			bits;
} NidListTab[] = {
	{	NID_sect163k1,			_T("nistk163"),		163	},	// RECOMMENDED
	{	NID_X9_62_prime192v1,	_T("nistp192"),		192	},	// RECOMMENDED
	{	NID_secp224r1,			_T("nistp224"),		224	},	// RECOMMENDED
	{	NID_sect233k1,			_T("nistk233"),		233	},	// RECOMMENDED
	{	NID_sect233r1,			_T("nistb233"),		233	},	// RECOMMENDED
	{	NID_X9_62_prime256v1,	_T("nistp256"),		256	},
	{	NID_sect283k1,			_T("nistk283"),		283	},	// RECOMMENDED
	{	NID_secp384r1,			_T("nistp384"),		384	},
	{	NID_sect409k1,			_T("nistk409"),		409	},	// RECOMMENDED
	{	NID_sect409r1,			_T("nistb409"),		409	},	// RECOMMENDED
	{	NID_secp521r1,			_T("nistp521"),		521	},
	{	NID_sect571k1,			_T("nistt571"),		571	},	// RECOMMENDED
	{	0,						NULL,				0	},
}, DsaNidTab[] = {
	{	NID_ML_DSA_44,			_T("44"),			44	},
	{	NID_ML_DSA_65,			_T("65"),			65	},
	{	NID_ML_DSA_87,			_T("87"),			87	},

	{	NID_SLH_DSA_SHA2_128s,	_T("sha2-128s"),	128	| 0x10000 },
	{	NID_SLH_DSA_SHA2_128f,	_T("sha2-128f"),	128 },
	{	NID_SLH_DSA_SHA2_192s,	_T("sha2-192s"),	192 | 0x10000 },
	{	NID_SLH_DSA_SHA2_192f,	_T("sha2-192f"),	192 },
	{	NID_SLH_DSA_SHA2_256s,	_T("sha2-256s"),	256 | 0x10000 },
	{	NID_SLH_DSA_SHA2_256f,	_T("sha2-256f"),	256 },

	{	NID_SLH_DSA_SHAKE_128s,	_T("shake-128s"),	128	| 0x30000 },
	{	NID_SLH_DSA_SHAKE_128f,	_T("shake-128f"),	128	| 0x20000 },
	{	NID_SLH_DSA_SHAKE_192s,	_T("shake-192s"),	192	| 0x30000 },
	{	NID_SLH_DSA_SHAKE_192f,	_T("shake-192f"),	192	| 0x20000 },
	{	NID_SLH_DSA_SHAKE_256s,	_T("shake-256s"),	256	| 0x30000 },
	{	NID_SLH_DSA_SHAKE_256f,	_T("shake-256f"),	256	| 0x20000 },

	{	NID_undef,				_T("undef"),		0	},
};

int CIdKey::GetIndexNid(int nid)
{
	for ( int n = 0 ; NidListTab[n].nid != 0 ; n++ ) {
		if ( NidListTab[n].nid == nid )
			return n;
	}
	return (-1);
}
int CIdKey::GetIndexName(LPCTSTR name)
{
	for ( int n = 0 ; NidListTab[n].nid != 0 ; n++ ) {
		if ( _tcscmp(NidListTab[n].name, name) == 0 )
			return n;
	}
	return (-1);
}
const EVP_MD *CIdKey::GetEvpMdIdx(int idx)
{
	if ( NidListTab[idx].bits <= 256 )
		return EVP_sha256();
	else if ( NidListTab[idx].bits <= 384 )
		return EVP_sha384();
	else
		return EVP_sha512();
}

int CIdKey::GetEcNidFromName(LPCTSTR name)
{
	for ( int n = 0 ; NidListTab[n].nid != 0 ; n++ ) {
		if ( _tcscmp(NidListTab[n].name, name) == 0 )
			return NidListTab[n].nid;
	}
	return (-1);
}
LPCTSTR CIdKey::GetEcNameFromNid(int nid)
{
	for ( int n = 0 ; NidListTab[n].nid != 0 ; n++ ) {
		if ( NidListTab[n].nid == nid )
			return NidListTab[n].name;
	}
	return NULL;
}
const EVP_MD *CIdKey::GetEcEvpMdFromNid(int nid)
{
	for ( int n = 0 ; NidListTab[n].nid != 0 ; n++ ) {
		if ( NidListTab[n].nid == nid ) {
			if ( NidListTab[n].bits <= 256 )
				return EVP_sha256();
			else if ( NidListTab[n].bits <= 384 )
				return EVP_sha384();
			else
				return EVP_sha512();
		}
	}
	return NULL;
}
int CIdKey::GetEcNidFromKey(EC_KEY *k)
{
	int n;
	int nid;
	EC_GROUP *eg;
	BN_CTX *bnctx;
	const EC_GROUP *g = EC_KEY_get0_group(k);

	if ( (nid = EC_GROUP_get_curve_name(g)) > 0 )
		return nid;

	if ( (bnctx = BN_CTX_new()) == NULL )
		return 0;

	for ( n = 0 ; NidListTab[n].nid != 0 ; n++ ) {
		if ( (eg = EC_GROUP_new_by_curve_name(NidListTab[n].nid)) == NULL )
			return 0;
		if ( EC_GROUP_cmp(g, eg, bnctx) == 0 )
			break;
		EC_GROUP_free(eg);
	}
	BN_CTX_free(bnctx);

	if ( NidListTab[n].nid != 0 ) {
		EC_GROUP_set_asn1_flag(eg, OPENSSL_EC_NAMED_CURVE);
		if ( EC_KEY_set_group(k, eg) != 1 )
			return 0;
	}
	return NidListTab[n].nid;
}
void CIdKey::RsaGenAddPara(BIGNUM *iqmp)
{
	BN_CTX *ctx;
	BIGNUM *aux;
	BIGNUM const *d = NULL, *q = NULL, *p = NULL;
	BIGNUM *dmq1 = NULL, *dmp1 = NULL;

	if ( m_Rsa == NULL )
		return;

	if ( (aux = BN_new()) == NULL )
		return;

	if ( (ctx = BN_CTX_new()) == NULL )
		return;

	RSA_get0_key(m_Rsa, NULL, NULL, &d);
	RSA_get0_factors(m_Rsa, &p, &q);

	if ( q != NULL && d != NULL && (dmq1 = BN_new()) != NULL ) {
		if ( (BN_sub(aux, q, BN_value_one()) == 0) || (BN_mod(dmq1, d, aux, ctx) == 0) ) {
			BN_clear_free(dmq1);
			dmq1 = NULL;
		}
	}

	if ( p != NULL && d != NULL && (dmp1 = BN_new()) != NULL ) {
		if ( (BN_sub(aux, p, BN_value_one()) == 0) || (BN_mod(dmp1, d, aux, ctx) == 0) ) {
			BN_clear_free(dmp1);
			dmp1 = NULL;
		}
	}

	RSA_set0_crt_params(m_Rsa, dmp1, dmq1, iqmp);

	BN_clear_free(aux);
	BN_CTX_free(ctx);
}
int CIdKey::EdFromPkeyRaw(const EVP_PKEY *pkey)
{
	size_t len = 0;

	m_PublicKey.Clear();
	m_PrivateKey.Clear();

	if ( EVP_PKEY_get_raw_public_key(pkey, NULL, &len) <= 0 )
		return FALSE;

	if ( len <= 0 )
		return FALSE;

	m_PublicKey.PutSpc((int)len);

	if ( EVP_PKEY_get_raw_public_key(pkey, m_PublicKey.GetPtr(), &len) <= 0 )
		return FALSE;

	len = 0;

	if ( EVP_PKEY_get_raw_private_key(pkey, NULL, &len) <= 0 )
		return FALSE;

	if ( len <= 0 )
		return FALSE;

	m_PrivateKey.PutSpc((int)len);

	if ( EVP_PKEY_get_raw_private_key(pkey, m_PrivateKey.GetPtr(), &len) <= 0 )
		return FALSE;

	return TRUE;
}
LPCTSTR CIdKey::GetDsaName(int nid)
{
	int n;
	for ( n = 0 ; DsaNidTab[n].nid != NID_undef ; n++ ) {
		if ( DsaNidTab[n].nid == nid )
			break;
	}
	return DsaNidTab[n].name;
}
int CIdKey::GetDsaNid(LPCTSTR name, int type)
{
	int n;
	for ( n = 0 ; DsaNidTab[n].nid != NID_undef ; n++ ) {
		if ( _tcscmp(name, DsaNidTab[n].name) == 0 )
			break;
	}
	switch(type) {
	case IDKEY_ML_DSA:
		if ( n < 0 && n > 2 )
			return NID_undef;
		break;
	case IDKEY_SLH_DSA:
		if ( n < 3 && n > 14 )
			return NID_undef;
		break;
	}
	return DsaNidTab[n].nid;
}
int CIdKey::GetDsaSize(int nid)
{
	int n;
	for ( n = 0 ; DsaNidTab[n].nid != NID_undef ; n++ ) {
		if ( DsaNidTab[n].nid == nid )
			break;
	}
	return (DsaNidTab[n].bits & 0xFFFF);
}
int CIdKey::GetDsaPkey(int bits)
{
	int n;
	for ( n = 0 ; DsaNidTab[n].nid != NID_undef ; n++ ) {
		if ( DsaNidTab[n].bits == bits )
			break;
	}
	return DsaNidTab[n].nid;
}
BOOL CIdKey::IsSlhDsaStype()
{
	int n;
	for ( n = 0 ; DsaNidTab[n].nid != NID_undef ; n++ ) {
		if ( DsaNidTab[n].nid == m_Nid )
			break;
	}
	return ((DsaNidTab[n].bits & 0x10000) != 0 ? TRUE : FALSE);
}
BOOL CIdKey::IsSlhDsaShake()
{
	int n;
	for ( n = 0 ; DsaNidTab[n].nid != NID_undef ; n++ ) {
		if ( DsaNidTab[n].nid == m_Nid )
			break;
	}
	return ((DsaNidTab[n].bits & 0x20000) != 0 ? TRUE : FALSE);
}
int CIdKey::Create(int type)
{
	m_Type = IDKEY_NONE;
	switch(type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		m_Type = type;
		if ( m_Rsa != NULL )
			break;
		if ( (m_Rsa = RSA_new()) == NULL )
			return FALSE;
		//RSA_set0_key(m_Rsa, BN_new(), BN_new(), NULL);
		//RSA_set0_factors(m_Rsa, BN_new(), BN_new());
		//RSA_set0_crt_params(m_Rsa, BN_new(), BN_new(), BN_new());
		break;

	case IDKEY_DSA2:
		m_Type = type;
		if ( m_Dsa != NULL )
			break;
		if ( (m_Dsa = DSA_new()) == NULL )
			return FALSE;
		//DSA_set0_pqg(m_Dsa, BN_new(), BN_new(), BN_new());
		//DSA_set0_key(m_Dsa, BN_new(), NULL);
		break;

	case IDKEY_ECDSA:
		m_Type = type;
		if ( m_EcDsa != NULL )
			break;
		if ( GetIndexNid(m_Nid) >= 0 && (m_EcDsa = EC_KEY_new_by_curve_name(m_Nid)) == NULL )
			return FALSE;
		break;
	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		m_Type = type;
		//m_PublicKey.Clear();
		//m_PrivateKey.Clear();
		break;
	case IDKEY_XMSS:
		m_Type = type;
		m_XmssKey.RemoveAll();
		break;
	case IDKEY_UNKNOWN:
		m_Type = type;
		m_TypeName.Empty();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
static int GenCallBack(EVP_PKEY_CTX *ctx)
{
	GENSTATUS *gs = (GENSTATUS *)EVP_PKEY_CTX_get_app_data(ctx);

	// RSA
	//	0...1...20
	//	0...1...2
	//	0...1......3
	//	0...1...2
	//	0...1...2
	//	0...1......3
	if ( gs != NULL && (gs->type == IDKEY_RSA1 || gs->type == IDKEY_RSA2) ) {
		int c;
		int n = EVP_PKEY_CTX_get_keygen_info(ctx, 0);
		static const int StageSize[] = { 25, 25, 450, 25, 25, 450 };
		static const int StageMax[]  = { 0, 25, 50, 500, 525, 550, 1000 };

		if ( gs->max == 0 ) {
			gs->stat = 0;
			gs->zero = gs->count = 0;
			gs->max = 1000;
		}

		switch(n) {
		case 0:
			gs->zero++;
			break;

		case 1:
			gs->count++;
			// RSAの鍵作成で0と1の個数に相関関係がありそう・・・y = 0.12x + 130
			c = gs->zero * 12 / 100 + 130;
			if ( gs->count > c )
				c = gs->count * 105 / 100;
			gs->pos = StageSize[gs->stat] * gs->count / c + StageMax[gs->stat];
			break;

		case 2:
		case 3:
			if ( ++gs->stat > 6 )
				gs->stat = 6;
			gs->pos = StageMax[gs->stat];
			gs->zero = gs->count = 0;
			break;
		}
	}
	return (gs != NULL && gs->abort != 0 ? 0 : 1);
}
int CIdKey::Generate(int type, int bits, LPCTSTR pass, GENSTATUS *gs)
{
	int n;
	char *name;
	int rt = FALSE;
	int id = EVP_PKEY_NONE;
	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *ctx_params = NULL;
	EVP_PKEY* pkey_params = NULL;
	BOOL bParams = FALSE;
	CIdKey secKey;

	switch(type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		id = EVP_PKEY_RSA;
		break;
	case IDKEY_DSA2:
		id = EVP_PKEY_DSA;
		break;
	case IDKEY_ECDSA:
		id = EVP_PKEY_EC;
		break;
	case IDKEY_ED25519:
		id = EVP_PKEY_ED25519;
		break;
	case IDKEY_ED448:
		id = EVP_PKEY_ED448;
		break;
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		if ( (id = GetDsaPkey(bits)) == NID_undef )
			return FALSE;
		break;
	case IDKEY_XMSS:
		if ( bits <= 10 )
			name = "XMSS-SHA2_10_256";
		else if ( bits <= 16 )
			name = "XMSS-SHA2_16_256";
		else // if ( bits <= 20 )
			name = "XMSS-SHA2_20_256";

		if ( !m_XmssKey.SetBufSize(name) )
			goto ERRRET;

		if ( m_XmssKey.KeyPair(gs) != 0 )
			goto ERRRET;

		m_Type = IDKEY_XMSS;
		goto NEWRET;
	default:
		return FALSE;
	}

	if ( (ctx = EVP_PKEY_CTX_new_id(id, NULL)) == NULL )
		goto ERRRET;

	switch(id) {
	case EVP_PKEY_RSA:
		if ( EVP_PKEY_keygen_init(ctx) <= 0 )
			goto ERRRET;

		if ( EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0 )
			goto ERRRET;
		break;

	case EVP_PKEY_DSA:
		if ( EVP_PKEY_paramgen_init(ctx) <= 0 )
			goto ERRRET;

		if ( EVP_PKEY_CTX_set_dsa_paramgen_bits(ctx, bits) <= 0 )
			goto ERRRET;

		bParams = TRUE;
		break;

	case EVP_PKEY_EC:
		for ( n = 0 ; NidListTab[n].nid != 0 ; n++ ) {
			if ( NidListTab[n].bits >= bits )
				break;
		}
		if ( NidListTab[n].nid == 0 )
			goto ERRRET;

		if ( EVP_PKEY_paramgen_init(ctx) <= 0 )
			goto ERRRET;

		if ( EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NidListTab[n].nid) <= 0 )
			goto ERRRET;

		bParams = TRUE;
		break;

	case EVP_PKEY_ED25519:
	case EVP_PKEY_ED448:
	case EVP_PKEY_ML_DSA_44:
	case EVP_PKEY_ML_DSA_65:
	case EVP_PKEY_ML_DSA_87:
	case EVP_PKEY_SLH_DSA_SHA2_128S:
	case EVP_PKEY_SLH_DSA_SHA2_128F:
	case EVP_PKEY_SLH_DSA_SHA2_192S:
	case EVP_PKEY_SLH_DSA_SHA2_192F:
	case EVP_PKEY_SLH_DSA_SHA2_256S:
	case EVP_PKEY_SLH_DSA_SHA2_256F:
	case EVP_PKEY_SLH_DSA_SHAKE_128S:
	case EVP_PKEY_SLH_DSA_SHAKE_128F:
	case EVP_PKEY_SLH_DSA_SHAKE_192S:
	case EVP_PKEY_SLH_DSA_SHAKE_192F:
	case EVP_PKEY_SLH_DSA_SHAKE_256S:
	case EVP_PKEY_SLH_DSA_SHAKE_256F:
		if ( EVP_PKEY_keygen_init(ctx) <= 0 )
			goto ERRRET;
		break;
	}

	if ( bParams ) {
		if ( EVP_PKEY_paramgen(ctx, &pkey_params) <= 0 )
			goto ERRRET;

		if ( pkey_params == NULL )
			goto ERRRET;

		ctx_params = ctx;

		if ( (ctx = EVP_PKEY_CTX_new(pkey_params, NULL)) == NULL )
			goto ERRRET;

		if ( EVP_PKEY_keygen_init(ctx) <= 0 )
			goto ERRRET;
	}

	if ( gs != NULL ) {
		EVP_PKEY_CTX_set_cb(ctx, GenCallBack);
		EVP_PKEY_CTX_set_app_data(ctx, gs);
	}

	if ( EVP_PKEY_keygen(ctx, &pkey) <= 0 )
		goto ERRRET;

	if ( pkey == NULL )
		goto ERRRET;

	if ( !SetEvpPkey(pkey) )
		goto ERRRET;

	if ( type == IDKEY_RSA1 )
		m_Type = type;

NEWRET:
	if ( type == IDKEY_ML_DSA_ES ) {
		switch(m_Nid) {
		case NID_ML_DSA_44:
		case NID_ML_DSA_65:
			if ( !secKey.Generate(IDKEY_ECDSA, 256, pass, gs) )
				goto ERRRET;
			break;
		case NID_ML_DSA_87:
			if ( !secKey.Generate(IDKEY_ECDSA, 384, pass, gs) )
				goto ERRRET;
			break;
		}

		BOOL bErr = TRUE;
		BN_CTX *bnctx;
		const BIGNUM *b = EC_KEY_get0_private_key(secKey.m_EcDsa);
		int len;

		if ( b != NULL && (bnctx = BN_CTX_new()) != NULL ) {
			if ( (len = (int)EC_POINT_point2oct(EC_KEY_get0_group(secKey.m_EcDsa), EC_KEY_get0_public_key(secKey.m_EcDsa), POINT_CONVERSION_COMPRESSED, NULL, 0, bnctx)) > 0 ) {
				if ( EC_POINT_point2oct(EC_KEY_get0_group(secKey.m_EcDsa), EC_KEY_get0_public_key(secKey.m_EcDsa), POINT_CONVERSION_COMPRESSED, m_PublicKey.PutSpc(len), len, bnctx) == len )
					bErr = FALSE;
			}
			BN_CTX_free(bnctx);
		}
		if ( bErr )
			goto ERRRET;
		if ( (len = BN_num_bytes(b)) <= 2 )
			goto ERRRET;
		if ( BN_bn2bin(b, m_PrivateKey.PutSpc(len)) != len )
			goto ERRRET;

		m_Type = type;

	} else if ( type == IDKEY_ML_DSA_ED ) {
		switch(m_Nid) {
		case NID_ML_DSA_44:
		case NID_ML_DSA_65:
			if ( !secKey.Generate(IDKEY_ED25519, 255, pass, gs) )
				goto ERRRET;
			break;
		case NID_ML_DSA_87:
			if ( !secKey.Generate(IDKEY_ED448, 448, pass, gs) )
				goto ERRRET;
			break;
		}

		m_PublicKey.Apend(secKey.m_PublicKey.GetPtr(), secKey.m_PublicKey.GetSize());
		m_PrivateKey.Apend(secKey.m_PrivateKey.GetPtr(), secKey.m_PrivateKey.GetSize());
		m_Type = type;
	}

	if ( !WritePrivateKey(m_SecBlob, pass) )
		goto ERRRET;

	m_bSecInit = FALSE;
	rt = TRUE;

ERRRET:
	if ( pkey != NULL )
		EVP_PKEY_free(pkey);

	if ( ctx != NULL )
		EVP_PKEY_CTX_free(ctx);

	if ( pkey_params != NULL )
		EVP_PKEY_free(pkey_params);

	if ( ctx_params != NULL )
		EVP_PKEY_CTX_free(ctx_params);

	return rt;
}
int CIdKey::Close()
{
	if ( m_Rsa != NULL )
		RSA_free(m_Rsa);

	if ( m_Dsa != NULL )
		DSA_free(m_Dsa);

	if ( m_EcDsa != NULL )
		EC_KEY_free(m_EcDsa);

	m_Rsa = NULL;
	m_Dsa = NULL;
	m_EcDsa = NULL;
	m_XmssKey.RemoveAll();
	m_Type = IDKEY_NONE;
	m_bSecInit = FALSE;
	m_SecBlob.Empty();
	m_Cert = 0;
	m_CertBlob.Clear();
	m_PublicKey.Clear();
	m_PrivateKey.Clear();
	m_Nid = NID_undef;

	return FALSE;
}
int CIdKey::ComperePublic(CIdKey *pKey)
{
	int ret = (-1);
	BN_CTX *bnctx = NULL;

	if ( (m_Type & IDKEY_TYPE_MASK) != (pKey->m_Type & IDKEY_TYPE_MASK) )
		return (-1);

	switch(m_Type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		if ( m_Rsa == NULL || pKey->m_Rsa == NULL )
			break;
		{
			BIGNUM const *sn = NULL, *dn = NULL;
			BIGNUM const *se = NULL, *de = NULL;

			RSA_get0_key(m_Rsa, &sn, &se, NULL);
			RSA_get0_key(pKey->m_Rsa, &dn, &de, NULL);

			if ( sn == NULL || dn == NULL || BN_cmp(sn, dn) != 0 )
				break;
			if ( se == NULL || de == NULL || BN_cmp(se, de) != 0 )
				break;
		}
		ret = 0;
		break;
	case IDKEY_DSA2:
		if ( m_Dsa == NULL || pKey->m_Dsa == NULL )
			break;
		{
			BIGNUM const *sp = NULL, *dp = NULL;
			BIGNUM const *sq = NULL, *dq = NULL;
			BIGNUM const *sg = NULL, *dg = NULL;
			BIGNUM const *spub_key = NULL, *dpub_key = NULL;

			DSA_get0_pqg(m_Dsa, &sp, &sq, &sg);
			DSA_get0_pqg(pKey->m_Dsa, &dp, &dq, &dg);

			DSA_get0_key(m_Dsa, &spub_key, NULL);
			DSA_get0_key(pKey->m_Dsa, &dpub_key, NULL);

			if ( sp == NULL || dp == NULL || BN_cmp(sp, dp) != 0 )
				break;
			if ( sq == NULL || dq == NULL || BN_cmp(sq, dq) != 0 )
				break;
			if ( sg == NULL || dg == NULL || BN_cmp(sg, dg) != 0 )
				break;
			if ( spub_key == NULL || dpub_key == NULL || BN_cmp(spub_key, dpub_key) != 0 )
				break;
		}
		ret = 0;
		break;
	case IDKEY_ECDSA:
		if ( m_EcDsa == NULL || pKey->m_EcDsa == NULL )
			break;
		if ( m_Nid != pKey->m_Nid )
			break;
		if ( EC_KEY_get0_public_key(m_EcDsa) == NULL || EC_KEY_get0_public_key(pKey->m_EcDsa) == NULL )
			break;
		if ( (bnctx = BN_CTX_new()) == NULL )
			break;
		if ( EC_GROUP_cmp(EC_KEY_get0_group(m_EcDsa), EC_KEY_get0_group(pKey->m_EcDsa), bnctx) != 0 )
			break;
		if ( EC_POINT_cmp(EC_KEY_get0_group(m_EcDsa), EC_KEY_get0_public_key(m_EcDsa), EC_KEY_get0_public_key(pKey->m_EcDsa), bnctx) != 0 )
			break;
		ret = 0;
		break;
	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		if ( m_PublicKey.GetSize() == 0 || m_PublicKey.GetSize() != pKey->m_PublicKey.GetSize() )
			break;
		if ( memcmp(m_PublicKey.GetPtr(), pKey->m_PublicKey.GetPtr(), m_PublicKey.GetSize()) != 0 )
			break;
		ret = 0;
		break;
	case IDKEY_XMSS:
		if ( m_XmssKey.m_pPubBuf == NULL || pKey->m_XmssKey.m_pPubBuf == NULL || m_XmssKey.m_PubLen != pKey->m_XmssKey.m_PubLen )
			break;
		if ( memcmp(m_XmssKey.m_pPubBuf, pKey->m_XmssKey.m_pPubBuf, m_XmssKey.m_PubLen) != 0 )
			break;
		ret = 0;
		break;
	case IDKEY_UNKNOWN:
		if ( m_TypeName.Compare(pKey->m_TypeName) != 0 )
			break;
		if ( m_TypeBlob.GetSize() == 0 || m_TypeBlob.GetSize() != pKey->m_TypeBlob.GetSize() )
			break;
		if ( memcmp(m_TypeBlob.GetPtr(), pKey->m_TypeBlob.GetPtr(), m_TypeBlob.GetSize()) != 0 )
			break;
		ret = 0;
		break;
	}

	if ( bnctx != NULL )
		BN_CTX_free(bnctx);

	return ret;
}
int CIdKey::Compere(CIdKey *pKey)
{
	if ( m_Type != pKey->m_Type || m_Cert != pKey->m_Cert )
		return (-1);

	if ( m_Cert != 0 ) {
		if ( m_CertBlob.GetSize() != pKey->m_CertBlob.GetSize() )
			return (-1);
		return memcmp(m_CertBlob.GetPtr(), pKey->m_CertBlob.GetPtr(), m_CertBlob.GetSize());
	}

	return ComperePublic(pKey);
}
LPCTSTR CIdKey::GetName(int nFlag)
{
	int n;

	m_Work.Empty();

	if ( (nFlag & IDKEY_NAME_AGEANT) != 0 ) {
		if ( m_AgeantType == IDKEY_AGEANT_PUTTY || m_AgeantType == IDKEY_AGEANT_PUTTYPIPE )
			m_Work += _T("Pageant:");
		else if ( m_AgeantType == IDKEY_AGEANT_WINSSH )
			m_Work += _T("Wageant:");
	}

#ifdef	USE_X509
	if ( bCert ) {
		switch(m_Cert) {
		case IDKEY_CERTX509:
			m_Work += _T("x509v3-");
			break;
		}
	}
#endif

	switch(m_Type) {
	case IDKEY_RSA1:
		m_Work += _T("rsa1");
		break;
	case IDKEY_RSA2:
		if ( (nFlag & IDKEY_NAME_SIGN) != 0 ) {
			switch(m_Nid) {
			case NID_sha1:	 m_Work += _T("ssh-rsa"); break;
			case NID_sha256: m_Work += _T("rsa-sha2-256"); break;
			case NID_sha512: m_Work += _T("rsa-sha2-512"); break;
			}
		} else
			m_Work += _T("ssh-rsa");
		break;
	case IDKEY_DSA2:
		m_Work += _T("ssh-dss");
		break;
	case IDKEY_ECDSA:
		m_Work += _T("ecdsa-sha2-");
		if ( (n = GetIndexNid(m_Nid)) >= 0 )
			m_Work += NidListTab[n].name;
		else
			m_Work += _T("unknown");
		break;
	case IDKEY_ED25519:
		m_Work += _T("ssh-ed25519");
		break;
	case IDKEY_ED448:
		m_Work += _T("ssh-ed448");
		break;
	case IDKEY_ML_DSA:
		m_Work += _T("ssh-mldsa-");			// draft-sfluhrer-ssh-mldsa-04
		m_Work += GetDsaName(m_Nid);
		break;
	case IDKEY_ML_DSA_ES:
		m_Work += _T("ssh-mldsa");			// draft-sun-ssh-composite-sigs-01
		switch(m_Nid) {
		case NID_ML_DSA_44:
			m_Work += _T("44-es256");
			break;
		case NID_ML_DSA_65:
			m_Work += _T("65-es256");
			break;
		case NID_ML_DSA_87:
			m_Work += _T("87-es384");
			break;
		}
		break;
	case IDKEY_ML_DSA_ED:
		m_Work += _T("ssh-mldsa");			// draft-sun-ssh-composite-sigs-01
		switch(m_Nid) {
		case NID_ML_DSA_44:
			m_Work += _T("44-ed25519");
			break;
		case NID_ML_DSA_65:
			m_Work += _T("65-ed25519");
			break;
		case NID_ML_DSA_87:
			m_Work += _T("87-ed448");
			break;
		}
		break;
	case IDKEY_SLH_DSA:
		m_Work += _T("ssh-slh-dsa-");		// draft-josefsson-ssh-sphincs-00
		m_Work += GetDsaName(m_Nid);
		break;
	case IDKEY_XMSS:
		if ( (nFlag & IDKEY_NAME_CERT) != 0 && m_Cert != 0 )
			m_Work += _T("ssh-xmss");
		else
			m_Work += _T("ssh-xmss@openssh.com");
		break;
	case IDKEY_UNKNOWN:
		m_Work += m_TypeName;
		break;
	}

	if ( (nFlag & IDKEY_NAME_CERT) != 0 ) {
		switch(m_Cert) {
		case IDKEY_CERTV00:
			m_Work += _T("-cert-v00@openssh.com");
			break;
		case IDKEY_CERTV01:
			m_Work += _T("-cert-v01@openssh.com");
			break;
		}
	}

	return m_Work;
}
typedef struct _TypeNameTab {
	LPCTSTR		name;
	int			type;
	int			nid;
} TYPENAMETAB;

static int TypeNameCmp(const void *src, const void *dis)
{
	return _tcscmp((LPCTSTR)src, ((TYPENAMETAB *)dis)->name);
}

BOOL CIdKey::GetTypeFromName(LPCTSTR name, int &type, int &nid)
{
	int n;
	int cert = 0;
	int len = (int)_tcslen(name);
	LPCTSTR last;
	CString tmp;

#define	TYPENAMETABMAX		44
	static const TYPENAMETAB TypeNameTab[TYPENAMETABMAX] = {
		{ _T("dsa"),						IDKEY_DSA2,			NID_undef				},
		{ _T("ecdsa-sha2-nistb233"),		IDKEY_ECDSA,		NID_sect233r1			},
		{ _T("ecdsa-sha2-nistb409"),		IDKEY_ECDSA,		NID_sect409r1			},
		{ _T("ecdsa-sha2-nistk163"),		IDKEY_ECDSA,		NID_sect163k1			},
		{ _T("ecdsa-sha2-nistk233"),		IDKEY_ECDSA,		NID_sect233k1			},
		{ _T("ecdsa-sha2-nistk283"),		IDKEY_ECDSA,		NID_sect283k1			},
		{ _T("ecdsa-sha2-nistk409"),		IDKEY_ECDSA,		NID_sect409k1			},
		{ _T("ecdsa-sha2-nistp192"),		IDKEY_ECDSA,		NID_X9_62_prime192v1	},
		{ _T("ecdsa-sha2-nistp224"),		IDKEY_ECDSA,		NID_secp224r1			},
		{ _T("ecdsa-sha2-nistp256"),		IDKEY_ECDSA,		NID_X9_62_prime256v1	},
		{ _T("ecdsa-sha2-nistp384"),		IDKEY_ECDSA,		NID_secp384r1			},
		{ _T("ecdsa-sha2-nistp521"),		IDKEY_ECDSA,		NID_secp521r1			},
		{ _T("ecdsa-sha2-nistt571"),		IDKEY_ECDSA,		NID_sect571k1			},
		{ _T("rsa"),						IDKEY_RSA2,			NID_sha1				},
		{ _T("rsa-sha2-256"),				IDKEY_RSA2,			NID_sha256				},
		{ _T("rsa-sha2-512"),				IDKEY_RSA2,			NID_sha512				},
		{ _T("rsa1"),						IDKEY_RSA1,			NID_sha1				},
		{ _T("ssh-dss"),					IDKEY_DSA2,			NID_undef				},
		{ _T("ssh-ed25519"),				IDKEY_ED25519,		NID_ED25519				},
		{ _T("ssh-ed448"),					IDKEY_ED448,		NID_ED448				},
		{ _T("ssh-mldsa-44"),				IDKEY_ML_DSA,		NID_ML_DSA_44			},	// draft-sfluhrer-ssh-mldsa-04
		{ _T("ssh-mldsa-65"),				IDKEY_ML_DSA,		NID_ML_DSA_65			},	// draft-sfluhrer-ssh-mldsa-04
		{ _T("ssh-mldsa-87"),				IDKEY_ML_DSA,		NID_ML_DSA_87			},	// draft-sfluhrer-ssh-mldsa-04
		{ _T("ssh-mldsa44-ed25519"),		IDKEY_ML_DSA_ED,	NID_ML_DSA_44			},	// draft-sun-ssh-composite-sigs-01	NID_ED25519
		{ _T("ssh-mldsa44-es256"),			IDKEY_ML_DSA_ES,	NID_ML_DSA_44			},	// draft-sun-ssh-composite-sigs-01	NID_X9_62_prime256v1
		{ _T("ssh-mldsa65-ed25519"),		IDKEY_ML_DSA_ED,	NID_ML_DSA_65			},	// draft-sun-ssh-composite-sigs-01	NID_ED25519
		{ _T("ssh-mldsa65-es256"),			IDKEY_ML_DSA_ES,	NID_ML_DSA_65			},	// draft-sun-ssh-composite-sigs-01	NID_X9_62_prime256v1
		{ _T("ssh-mldsa87-ed448"),			IDKEY_ML_DSA_ED,	NID_ML_DSA_87			},	// draft-sun-ssh-composite-sigs-01	NID_ED448
		{ _T("ssh-mldsa87-es384"),			IDKEY_ML_DSA_ES,	NID_ML_DSA_87			},	// draft-sun-ssh-composite-sigs-01	NID_secp384r1
		{ _T("ssh-rsa"),					IDKEY_RSA2,			NID_sha1				},
		{ _T("ssh-slh-dsa-sha2-128f"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHA2_128f	},
		{ _T("ssh-slh-dsa-sha2-128s"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHA2_128s	},
		{ _T("ssh-slh-dsa-sha2-192f"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHA2_192f	},
		{ _T("ssh-slh-dsa-sha2-192s"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHA2_192s	},
		{ _T("ssh-slh-dsa-sha2-256f"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHA2_256f	},
		{ _T("ssh-slh-dsa-sha2-256s"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHA2_256s	},
		{ _T("ssh-slh-dsa-shake-128f"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHAKE_128f	},
		{ _T("ssh-slh-dsa-shake-128s"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHAKE_128s	},
		{ _T("ssh-slh-dsa-shake-192f"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHAKE_192f	},
		{ _T("ssh-slh-dsa-shake-192s"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHAKE_192s	},
		{ _T("ssh-slh-dsa-shake-256f"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHAKE_256f	},
		{ _T("ssh-slh-dsa-shake-256s"),		IDKEY_SLH_DSA,		NID_SLH_DSA_SHAKE_256s	},
		{ _T("ssh-xmss"),					IDKEY_XMSS,			NID_undef				},
		{ _T("ssh-xmss@openssh.com"),		IDKEY_XMSS,			NID_undef				},
	};

	//  123456789012345678901
	//  -cert-v0?@openssh.com
	if ( (last = name + len - 21) >= name ) {
		if ( _tcscmp(last, _T("-cert-v00@openssh.com")) == 0 )
			cert = IDKEY_CERTV00;
		else if ( _tcscmp(last, _T("-cert-v01@openssh.com")) == 0 )
			cert = IDKEY_CERTV01;
	}

	//  12345
	//  -cert			draft-miller-ssh-cert-00
	if ( cert == 0 && (last = name + len - 5) >= name ) {
		if ( _tcscmp(last, _T("-cert")) == 0 )
			cert = IDKEY_CERTV01;
	}

	if ( cert != 0 ) {
		while ( name < last )
			tmp += *(name++);
		name = tmp;
#ifdef	USE_X509
	} else if ( _tcsncmp(name, _T("x509v3-"), 7) == 0 ) {
		cert = IDKEY_CERTX509;
		name += 7;
#endif
	}

	if ( BinaryFind((void *)name, (void *)TypeNameTab, sizeof(TYPENAMETAB), TYPENAMETABMAX, TypeNameCmp, &n) ) {
		type = TypeNameTab[n].type | cert;
		nid = TypeNameTab[n].nid;
	} else if ( *name != _T('\0') ) {
		type = IDKEY_UNKNOWN;
		nid = NID_undef;
		m_TypeName = name;
	} else {
		type = IDKEY_NONE;
		nid = NID_undef;
		return FALSE;
	}

	return TRUE;
}
int CIdKey::ChkOldCertHosts(LPCTSTR host)
{
	int n;
	CString dig, work;
	CStringArrayExt entry;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();

	WritePublicKey(dig);

	pApp->GetProfileKeys(_T("CertHosts"), entry);
	for ( n = 0 ; n < entry.GetSize() ; n++ ) {
		if ( !IsListEntry(entry[n]) )
			continue;
		work = pApp->GetProfileString(_T("CertHosts"), entry[n], _T(""));
		if ( work.Compare(dig) == 0 ) {
			pApp->DelProfileEntry(_T("CertHosts"), entry[n]);
			return TRUE;
		}
	}

	return FALSE;
}
BOOL CIdKey::KnownHostsCheck(LPCTSTR dig)
{
	int n, i, a;
	LPCTSTR p;
	CStringArrayExt list, entry;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
	CIdKey key;
	CString digest;

	pApp->GetProfileKeys(_T("KnownHosts"), list);
	for ( n = 0 ; n < list.GetSize() ; n++ ) {
		// 新しい形式(KnownHosts\host:nnn)
		if ( (p = _tcsrchr(list[n], _T(':'))) != NULL && IsDigits(p + 1) ) {
			pApp->GetProfileStringArray(_T("KnownHosts"), list[n], entry);
			for ( i = 0 ; i < entry.GetSize() ; i++ ) {
				if ( entry[i].Compare(dig) == 0 )
					return TRUE;
			}

		// 古い形式(KnownHosts\host-xxx)
		} else if ( (p = _tcsrchr(list[n], _T('-'))) != NULL && key.GetTypeFromName(p + 1, i, a) && i != IDKEY_UNKNOWN ) {
			digest = pApp->GetProfileString(_T("KnownHosts"), list[n], _T(""));
			if ( digest.Compare(dig) == 0 )
				return TRUE;


		// 古い形式(KnownHosts\host)
		} else {
			pApp->GetProfileStringArray(_T("KnownHosts"), list[n], entry);
			for ( i = 0 ; i < entry.GetSize() ; i++ ) {
				if ( entry[i].Compare(dig) == 0 )
					return TRUE;
			}
		}
	}

	return FALSE;
}
int CIdKey::HostVerify(LPCTSTR host, UINT port, class Cssh *pSsh)
{
	int n, i, found;
	CString dig, ses, known, work, kname;
	CStringArrayExt entry;
	CStringBinary cert;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
	CCertKeyDlg dlg;

	int type = SSHFP_KEY_RESERVED;
	PDNS_RECORD rec, p;
	CBuffer digest;
	struct _DNS_SSHFP_DATA {
		BYTE type;
		BYTE hash;
		BYTE digest[1];
	} *t;
	ADDRINFOT hints, *ai;
	CString PunyName;

	if ( pSsh != NULL )
		pSsh->m_bKnownHostUpdate = TRUE;

	if (  pSsh != NULL && pSsh->m_pDocument != NULL && pSsh->m_pDocument->m_TextRam.IsOptEnable(TO_DNSSSSHFP) ) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_NUMERICHOST;

		pSsh->PunyCodeAdress(host, PunyName);
		host = PunyName;

		if ( GetAddrInfo(host, NULL, &hints, &ai) == 0 )
			FreeAddrInfo(ai);
		else {
			switch(m_Type) {
			case IDKEY_RSA1:
			case IDKEY_RSA2:
				type = SSHFP_KEY_RSA;
				break;
			case IDKEY_DSA2:
				type = SSHFP_KEY_DSA;
				break;
			case IDKEY_ECDSA:
				type = SSHFP_KEY_ECDSA;
				break;
			case IDKEY_ED25519:
				type = SSHFP_KEY_ED25519;
				break;
			case IDKEY_ED448:
				type = SSHFP_KEY_ED448;
				break;
			case IDKEY_XMSS:
				type = SSHFP_KEY_XMSS;
				break;
			}

			if ( type != SSHFP_KEY_RESERVED && (n = DnsQuery(host, DNS_TYPE_SSHFP, DNS_QUERY_STANDARD, NULL, &rec, NULL)) == 0 ) {
				found = 0;
				for ( p = rec ; p != NULL ; p = p->pNext ) {
					TRACE("rec %d\n", p->wType);
					if ( p->wType != DNS_TYPE_SSHFP )
						continue;
					t = (struct _DNS_SSHFP_DATA *)&(p->Data.Null);
					if ( t->type == SSHFP_KEY_RSA || t->type == SSHFP_KEY_DSA || t->type == SSHFP_KEY_ECDSA || t->type == SSHFP_KEY_ED25519 || t->type == SSHFP_KEY_ED448 || t->type == SSHFP_KEY_XMSS ) {
						found |= 001;
						if ( t->type == type && DnsDigest(t->hash, digest) && p->wDataLength == (digest.GetSize() + 2) && memcmp(t->digest, digest.GetPtr(), digest.GetSize()) == 0 )
							found |= 002;
					}
				}
				DnsRecordListFree(rec, DnsFreeRecordList);

				if ( found == 001 ) {
					if ( ::AfxMessageBox(CStringLoad(IDE_DNSDIGESTNOTMATCH), MB_ICONWARNING | MB_YESNO) != IDYES )
						return FALSE;
				}
			}
		}
	}

	found = FALSE;
	WritePublicKey(dig, FALSE);

	kname.Format(_T("%s:%d"), (LPCTSTR)host, port);
	pApp->GetProfileStringArray(_T("KnownHosts"), kname, entry);

	if ( entry.GetSize() == 0 ) {
		CStringArrayExt oldEntry;

		// 古い形式(KnownHosts\host-xxx)を救済
		pApp->GetProfileKeys(_T("KnownHosts"), oldEntry);
		i = (int)_tcslen(host);
		for ( n = 0 ; n < oldEntry.GetSize() ; n++ ) {
			if ( _tcsncmp(host, oldEntry[n], i) == 0 && oldEntry[n][i] == _T('-') ) {
				known = pApp->GetProfileString(_T("KnownHosts"), oldEntry[n], _T(""));
				if ( !known.IsEmpty() )
					entry.Add(known);
				pApp->DelProfileEntry(_T("KnownHosts"), oldEntry[n]);
			}
		}
		if ( entry.GetSize() > 0 )
			pApp->WriteProfileStringArray(_T("KnownHosts"), kname, entry);

		// 古い形式(KnownHosts\host)を救済
		pApp->GetProfileStringArray(_T("KnownHosts"), host, oldEntry);
		if ( oldEntry.GetSize() != 0 ) {
			for ( n = 0 ; n < oldEntry.GetSize() ; n++ )
				entry.Add(oldEntry[n]);
			pApp->DelProfileEntry(_T("KnownHosts"), host);
			pApp->WriteProfileStringArray(_T("KnownHosts"), kname, entry);
		}
	}

	for ( n = 0 ; n < entry.GetSize() ; n++ ) {
		CIdKey work;
		work.ReadPublicKey(entry[n]);
		if ( ComperePublic(&work) == 0 ) {
			found = TRUE;
			break;
		}
	}

	if ( !found ) {
		// すでに別ホスト：別ポートで登録済みのホスト鍵なら許可するが"hostkeys-00@openssh.com"の処理はしない
		//if ( KnownHostsCheck(dig) ) {
		//	pSsh->m_bKnownHostUpdate = FALSE;
		//	return TRUE;
		//}

		dlg.m_HostName = host;
		FingerPrint(dlg.m_Digest);
		if ( dlg.DoModal() != IDOK )
			return FALSE;

		if ( dlg.m_SaveKeyFlag ) {
			entry.Add(dig);
			pApp->WriteProfileStringArray(_T("KnownHosts"), kname, entry);

		// 保存しないならSSH2_MSG_GLOBAL_REQUESTの"hostkeys-00@openssh.com"を処理しない
		} else 	if ( pSsh != NULL )
			pSsh->m_bKnownHostUpdate = FALSE;
	}

	return TRUE;
}

int CIdKey::XmssSign(CBuffer *bp, LPBYTE buf, int len)
{
	int reqlen;
	unsigned long long siglen;
	CBuffer tmp(-1);

	if ( (reqlen = m_XmssKey.GetSignByte()) <= 0 )
		return FALSE;

	tmp.PutSpc(reqlen + len);
	siglen = tmp.GetSize();

	if ( m_Uid >= 0 && m_XmssKey.m_pPassBuf != NULL ) {
		// レジストリベースのXMSS

		// XMSSのロック
		CIdKey *pKey;
		CString lockName;
		CString passBuf(m_XmssKey.m_pPassBuf);
		lockName.Format(_T("RLogin_XMSS_%d"), m_Uid);
		CMutexLock mutex(lockName);

		// XMSSの更新をチェック
		pKey = ((CMainFrame *)theApp.m_pMainWnd)->m_IdKeyTab.ReloadUid(m_Uid);
		if ( pKey != NULL && pKey != this && pKey->m_Type == m_Type ) {
			if ( Compere(pKey) == 0 && m_SecBlob.Compare(pKey->m_SecBlob) != 0 ) {
				m_SecBlob = pKey->m_SecBlob;
				if ( !ReadPrivateKey(m_SecBlob, passBuf, m_bHostPass) )
					return FALSE;
			}
		} else if ( pKey == this && !Init(passBuf) )
			return FALSE;

		if ( m_XmssKey.Sign(tmp.GetPtr(), &siglen, buf, len) != 0 )
			return FALSE;

		// 鍵を更新
		WritePrivateKey(m_SecBlob, passBuf);
		m_XmssKey.SetPassBuf(NULL);

		// レジストリの鍵を更新
		pKey = ((CMainFrame *)theApp.m_pMainWnd)->m_IdKeyTab.GetUid(m_Uid);
		if ( pKey != NULL && pKey != this && pKey->m_Type == m_Type )
			pKey->m_SecBlob = m_SecBlob;
		((CMainFrame *)theApp.m_pMainWnd)->m_IdKeyTab.UpdateUid(m_Uid);

	} else if ( !m_FilePath.IsEmpty() ) {
		// ファイルベースのXMSS
		int n;
		CString lockName(_T("RLogin_XMSS_"));

		if ( (n = m_FilePath.ReverseFind('\\')) >= 0 || (n = m_FilePath.ReverseFind(':')) >= 0 )
			lockName += m_FilePath.Mid(n + 1);
		else
			lockName += m_FilePath;

		lockName += _T(".lock");
		CMutexLock mutex(lockName);

		m_XmssKey.LoadStateFile(m_FilePath);

		if ( m_XmssKey.Sign(tmp.GetPtr(), &siglen, buf, len) != 0 )
			return FALSE;

		m_XmssKey.SaveStateFile(m_FilePath);

	} else if ( m_XmssKey.Sign(tmp.GetPtr(), &siglen, buf, len) != 0 )
		return FALSE;

	bp->Clear();
	bp->PutStr(TstrToMbs(GetName()));
	bp->PutBuf(tmp.GetPtr(), (int)(siglen - len));

	return TRUE;
}
int CIdKey::CompositeHash(CBuffer *bp, LPBYTE buf, int len, LPBYTE rad)
{
	// draft-sun-ssh-composite-sigs-01
	// 3.2.  Composite Sign
	//	M' <- Prefix || Domain || 0x00 || r || PH(M)

	BYTE *prefix = (BYTE *)"CompositeAlgorithmSignatures2025";	// 32 byte
	BYTE *domain;
	const EVP_MD *md = EVP_sha512();
	int dlen = EVP_MAX_MD_SIZE;
	u_char digest[EVP_MAX_MD_SIZE];

	switch(m_Type) {
	case IDKEY_ML_DSA_ES:
		switch(m_Nid) {
		case NID_ML_DSA_44:
			md = EVP_sha256();
			domain = (BYTE *)"\x06\x0B\x60\x86\x48\x01\x86\xFA\x6B\x50\x09\x01\x03";
			break;
		case NID_ML_DSA_65:
			domain = (BYTE *)"\x06\x0B\x60\x86\x48\x01\x86\xFA\x6B\x50\x09\x01\x08";
			break;
		case NID_ML_DSA_87:
			domain = (BYTE *)"\x06\x0B\x60\x86\x48\x01\x86\xFA\x6B\x50\x09\x01\x0C";
			break;
		default:
			return FALSE;
		}
		break;
	case IDKEY_ML_DSA_ED:
		switch(m_Nid) {
		case NID_ML_DSA_44:
			domain = (BYTE *)"\x06\x0B\x60\x86\x48\x01\x86\xFA\x6B\x50\x09\x01\x02";
			break;
		case NID_ML_DSA_65:
			domain = (BYTE *)"\x06\x0B\x60\x86\x48\x01\x86\xFA\x6B\x50\x09\x01\x0B";
			break;
		case NID_ML_DSA_87:
			domain = (BYTE *)"\x06\x0B\x60\x86\x48\x01\x86\xFA\x6B\x50\x09\x01\x0E";
			break;
		default:
			return FALSE;
		}
		break;
	default:
		return FALSE;
	}

	if ( (dlen = MD_digest(md, buf, len, digest, sizeof(digest))) <= 0 )
		return FALSE;

	bp->Apend(prefix, 32);
	bp->Apend(domain, 13);
	bp->Apend((BYTE *)"\x00", 1);
	bp->Apend(rad, 32);
	bp->Apend(digest, dlen);

	return TRUE;
}

/* See FIPS 204 Section 4 Table 1 & Table 2 */
# define ML_DSA_44_PRIV_LEN 2560
# define ML_DSA_44_PUB_LEN 1312
# define ML_DSA_44_SIG_LEN 2420

/* See FIPS 204 Section 4 Table 1 & Table 2 */
# define ML_DSA_65_PRIV_LEN 4032
# define ML_DSA_65_PUB_LEN 1952
# define ML_DSA_65_SIG_LEN 3309

/* See FIPS 204 Section 4 Table 1 & Table 2 */
# define ML_DSA_87_PRIV_LEN 4896
# define ML_DSA_87_PUB_LEN 2592
# define ML_DSA_87_SIG_LEN 4627

int CIdKey::CompositeSign(CBuffer *bp, LPBYTE buf, int len)
{
	int n;
	CIdKey firKey, secKey;
	BYTE ra[32];
	CBuffer hash, sig, tmp;
	int pubLen = 0;
	int priLen = 0;
	CBuffer pub(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
	CBuffer pri(m_PrivateKey.GetPtr(), m_PrivateKey.GetSize());
	CStringA name = TstrToMbs(GetName());
	BN_CTX *bnctx;
	EC_POINT *q;
	BIGNUM *b;

	firKey.m_Nid = m_Nid;
	if ( !firKey.Create(IDKEY_ML_DSA) )
		return FALSE;

	switch(m_Nid) {
	case NID_ML_DSA_44:
		secKey.m_Nid = (m_Type == IDKEY_ML_DSA_ES ? NID_X9_62_prime256v1 : NID_ED25519);
		pubLen = ML_DSA_44_PUB_LEN;
		priLen = ML_DSA_44_PRIV_LEN;
		break;
	case NID_ML_DSA_65:
		secKey.m_Nid = (m_Type == IDKEY_ML_DSA_ES ? NID_X9_62_prime256v1 : NID_ED25519);
		pubLen = ML_DSA_65_PUB_LEN;
		priLen = ML_DSA_65_PRIV_LEN;
		break;
	case NID_ML_DSA_87:
		secKey.m_Nid = (m_Type == IDKEY_ML_DSA_ES ? NID_secp384r1 : NID_ED448);
		pubLen = ML_DSA_87_PUB_LEN;
		priLen = ML_DSA_87_PRIV_LEN;
		break;
	default:
		return FALSE;
	}

	if ( !secKey.Create((secKey.m_Nid == NID_ED25519 ? IDKEY_ED25519 : secKey.m_Nid == NID_ED448 ? IDKEY_ED448 : IDKEY_ECDSA)) )
		return FALSE;

	if ( pub.GetSize() <= pubLen )
		return FALSE;
	firKey.m_PublicKey.Apend(pub.GetPtr(), pubLen);
	pub.Consume(pubLen);

	if ( secKey.m_Type == IDKEY_ECDSA ) {
		BOOL bErr = TRUE;
		if ( (bnctx = BN_CTX_new()) != NULL ) {
			if ( (q = EC_POINT_new(EC_KEY_get0_group(secKey.m_EcDsa))) != NULL ) {
				if ( EC_POINT_oct2point(EC_KEY_get0_group(secKey.m_EcDsa), q, pub.GetPtr(), pub.GetSize(), bnctx) == 1 ) {
					if ( EC_KEY_set_public_key(secKey.m_EcDsa, q) == 1 )
						bErr = FALSE;
				}
				EC_POINT_free(q);
			}
			BN_CTX_free(bnctx);
		}
		if ( bErr )
			return FALSE;
	} else {	// IDKEY_ED25519 or IDKEY_ED448
		secKey.m_PublicKey.Apend(pub.GetPtr(), pub.GetSize());
	}

	if ( pri.GetSize() <= priLen )
		return FALSE;
	firKey.m_PrivateKey.Apend(pri.GetPtr(), priLen);
	firKey.m_bSecInit = TRUE;
	pri.Consume(priLen);

	if ( secKey.m_Type == IDKEY_ECDSA ) {
		BOOL bErr = TRUE;
		if ( (b = BN_new()) != NULL ) {
			if ( BN_bin2bn(pri.GetPtr(), pri.GetSize(), b) != NULL ) {
				if ( EC_KEY_set_private_key(secKey.m_EcDsa, b) == 1 )
					bErr = FALSE;
			}
			BN_free(b);
		}
		if ( bErr )
			return FALSE;
		secKey.m_bSecInit = TRUE;

	} else {	// IDKEY_ED25519 or IDKEY_ED448
		secKey.m_PrivateKey.Apend(pri.GetPtr(), pri.GetSize());
		secKey.m_bSecInit = TRUE;
	}

	rand_buf(ra, 32);

	if ( !CompositeHash(&hash, buf, len, ra) )
		return FALSE;

	tmp.Apend(ra, 32);

	if ( !firKey.Sign(&sig, hash.GetPtr(), hash.GetSize()) )
		return FALSE;

	n = sig.Get32Bit();	// name
	sig.Consume(n);
	n = sig.Get32Bit();	// sig
	tmp.Apend(sig.GetPtr(), n);

	if ( !secKey.Sign(&sig, hash.GetPtr(), hash.GetSize()) )
		return FALSE;

	n = sig.Get32Bit();	// name
	sig.Consume(n);
	n = sig.Get32Bit();	// sig
	tmp.Apend(sig.GetPtr(), n);

	bp->Clear();
	bp->PutStr(name);
	bp->PutBuf(tmp.GetPtr(), tmp.GetSize());

	return TRUE;
}
BOOL CIdKey::SignAlgCheck(LPCTSTR alg)
{
	if ( alg == NULL || *alg == _T('\0') )
		return TRUE;

	if ( InStrStr(alg, GetName(IDKEY_NAME_SIGN)) )
		return TRUE;

	if ( (m_Type & IDKEY_TYPE_MASK) != IDKEY_RSA1 && (m_Type & IDKEY_TYPE_MASK) != IDKEY_RSA2 )
		return FALSE;

	if ( InStrStr(alg, _T("rsa-sha2-512")) ) {
		m_Nid = NID_sha512;
		return TRUE;
	} else if ( InStrStr(alg, _T("rsa-sha2-256")) ) {
		m_Nid = NID_sha256;
		return TRUE;
	} else if ( InStrStr(alg, _T("ssh-rsa")) ) {
		m_Nid = NID_sha1;
		return TRUE;
	}

	return FALSE;
}
int CIdKey::Sign(CBuffer *bp, LPBYTE buf, int len)
{
	if ( m_AgeantType != IDKEY_AGEANT_NONE ) {
		int flag = 0;
		CBuffer blob;
		if ( (m_Type & IDKEY_TYPE_MASK) == IDKEY_RSA1 || (m_Type & IDKEY_TYPE_MASK) == IDKEY_RSA2 ) {
			switch(m_Nid) {
			case NID_sha256: flag = SSH_AGENT_RSA_SHA2_256; break;
			case NID_sha512: flag = SSH_AGENT_RSA_SHA2_512; break;
			}
		}
		SetBlob(&blob, FALSE);
		return ((CMainFrame *)theApp.m_pMainWnd)->AgeantSign(m_AgeantType, &blob, bp, buf, len, flag);
	}

	if ( !m_bSecInit )
		return FALSE;

	int type;
	CStringA name = TstrToMbs(GetName());
	const EVP_MD *evp_md = NULL;

	switch(m_Type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		switch(m_Nid) {
		case NID_sha1:   evp_md = EVP_sha1();   name = "ssh-rsa"; break;
		case NID_sha256: evp_md = EVP_sha256(); name = "rsa-sha2-256"; break;
		case NID_sha512: evp_md = EVP_sha512(); name = "rsa-sha2-512"; break;
		}
		break;

	case IDKEY_DSA2:
		evp_md = EVP_sha1();
		break;

	case IDKEY_ECDSA:
		if ( (type = GetIndexNid(m_Nid)) < 0 )
			return FALSE;
		evp_md = GetEvpMdIdx(type);
		break;

	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
		return CompositeSign(bp, buf, len);

	case IDKEY_XMSS:
		return XmssSign(bp, buf, len);

	case IDKEY_UNKNOWN:
		return FALSE;
	}

	int rt = FALSE;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *md_ctx = NULL;
	size_t siglen;
	CBuffer tmp(-1);
	DSA_SIG *dsig = NULL;
	const BIGNUM *r = NULL;
	const BIGNUM *s = NULL;
	const unsigned char *sigbuf;
	BYTE sigblob[SIGBLOB_LEN];
	unsigned int slen, rlen;
	ECDSA_SIG *edsig = NULL;

	if ( (pkey = GetEvpPkey(GETPKEY_PRIVATEKEY)) == NULL )
		goto ERRENDOF;

	if ( (md_ctx = EVP_MD_CTX_new()) == NULL )
		goto ERRENDOF;

	if ( EVP_DigestSignInit(md_ctx, NULL, evp_md, NULL, pkey) <= 0 )
		goto ERRENDOF;

	if ( EVP_DigestSign(md_ctx, NULL, &siglen, buf, len) <= 0 )
		goto ERRENDOF;

	tmp.PutSpc((int)siglen);

	if ( EVP_DigestSign(md_ctx, tmp.GetPtr(), &siglen, buf, len) <= 0 )
		goto ERRENDOF;

	switch(m_Type & IDKEY_TYPE_MASK) {
	case IDKEY_DSA2:
		sigbuf = tmp.GetPtr();
		d2i_DSA_SIG(&dsig, &sigbuf, tmp.GetSize());

		if ( dsig == NULL )
			goto ERRENDOF;

		DSA_SIG_get0(dsig, &r, &s);

		if ( r == NULL || s == NULL )
			goto ERRENDOF;

		rlen = BN_num_bytes(r);
		slen = BN_num_bytes(s);

		if (rlen > INTBLOB_LEN || slen > INTBLOB_LEN )
			goto ERRENDOF;

		memset(sigblob, 0, SIGBLOB_LEN);
		BN_bn2bin(r, sigblob + SIGBLOB_LEN - INTBLOB_LEN - rlen);
		BN_bn2bin(s, sigblob + SIGBLOB_LEN - slen);

		tmp.Clear();
		tmp.Apend(sigblob, SIGBLOB_LEN);
		break;

	case IDKEY_ECDSA:
		sigbuf = tmp.GetPtr();
		d2i_ECDSA_SIG(&edsig, &sigbuf, tmp.GetSize());

		if ( edsig == NULL )
			goto ERRENDOF;

		ECDSA_SIG_get0(edsig, &r, &s);

		if ( r == NULL || s == NULL )
			goto ERRENDOF;
	
		tmp.Clear();
		tmp.PutBIGNUM2(r);
		tmp.PutBIGNUM2(s);
		break;
	}

	bp->Clear();
	bp->PutStr(name);
	bp->PutBuf(tmp.GetPtr(), tmp.GetSize());
	rt = TRUE;

ERRENDOF:
	if ( md_ctx != NULL )
		EVP_MD_CTX_free(md_ctx);

	if ( pkey != NULL )
		EVP_PKEY_free(pkey);

	if ( dsig != NULL )
		DSA_SIG_free(dsig);

	if ( edsig != NULL )
		ECDSA_SIG_free(edsig);

	return rt;
}

int CIdKey::XmssVerify(CBuffer *sig, LPBYTE data, int datalen)
{
	unsigned long long mlen;
	CBuffer tmp(-1);

	sig->Apend(data, datalen);
	tmp.PutSpc(sig->GetSize());
	mlen = sig->GetSize();

	if ( m_XmssKey.Verify(tmp.GetPtr(), &mlen, sig->GetPtr(), sig->GetSize()) != 0 )
		return FALSE;

	if ( mlen != datalen )
		return FALSE;

	if ( memcmp(tmp.GetPtr(), data, datalen) != 0 )
		return FALSE;

	return TRUE;
}
int CIdKey::CompositeVerify(CBuffer *sig, LPBYTE data, int datalen)
{
	CIdKey firKey, secKey;
	BYTE ra[32];
	CBuffer hash, tmp;
	int pubLen = 0;
	int sigLen = 0;
	CBuffer pub(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
	BN_CTX *bnctx;
	EC_POINT *q;

	firKey.m_Nid = m_Nid;
	if ( !firKey.Create(IDKEY_ML_DSA) )
		return FALSE;

	switch(m_Nid) {
	case NID_ML_DSA_44:
		secKey.m_Nid = (m_Type == IDKEY_ML_DSA_ES ? NID_X9_62_prime256v1 : NID_ED25519);
		pubLen = ML_DSA_44_PUB_LEN;
		sigLen = ML_DSA_44_SIG_LEN;
		break;
	case NID_ML_DSA_65:
		secKey.m_Nid = (m_Type == IDKEY_ML_DSA_ES ? NID_X9_62_prime256v1 : NID_ED25519);
		pubLen = ML_DSA_65_PUB_LEN;
		sigLen = ML_DSA_65_SIG_LEN;
		break;
	case NID_ML_DSA_87:
		secKey.m_Nid = (m_Type == IDKEY_ML_DSA_ES ? NID_secp384r1 : NID_ED448);
		pubLen = ML_DSA_87_PUB_LEN;
		sigLen = ML_DSA_87_SIG_LEN;
		break;
	default:
		return FALSE;
	}

	if ( !secKey.Create((secKey.m_Nid == NID_ED25519 ? IDKEY_ED25519 : secKey.m_Nid == NID_ED448 ? IDKEY_ED448 : IDKEY_ECDSA)) )
		return FALSE;

	if ( pub.GetSize() <= pubLen )
		return FALSE;
	firKey.m_PublicKey.Apend(pub.GetPtr(), pubLen);
	pub.Consume(pubLen);

	if ( secKey.m_Type == IDKEY_ECDSA ) {
		BOOL bErr = TRUE;
		if ( (bnctx = BN_CTX_new()) != NULL ) {
			if ( (q = EC_POINT_new(EC_KEY_get0_group(secKey.m_EcDsa))) != NULL ) {
				if ( EC_POINT_oct2point(EC_KEY_get0_group(secKey.m_EcDsa), q, pub.GetPtr(), pub.GetSize(), bnctx) == 1 ) {
					if ( EC_KEY_set_public_key(secKey.m_EcDsa, q) == 1 )
						bErr = FALSE;
				}
				EC_POINT_free(q);
			}
			BN_CTX_free(bnctx);
		}
		if ( bErr )
			return FALSE;
	} else {	// IDKEY_ED25519 or IDKEY_ED448
		secKey.m_PublicKey.Apend(pub.GetPtr(), pub.GetSize());
	}

	memcpy(ra, sig->GetPtr(), 32);
	sig->Consume(32);

	if ( !CompositeHash(&hash, data, datalen, ra) )
		return FALSE;

	tmp.Clear();
	tmp.PutStr(TstrToMbs(firKey.GetName()));
	tmp.PutBuf(sig->GetPtr(), sigLen);
	sig->Consume(sigLen);

	if ( !firKey.Verify(&tmp, hash.GetPtr(), hash.GetSize()) )
		return FALSE;

	tmp.Clear();
	tmp.PutStr(TstrToMbs(secKey.GetName()));
	tmp.PutBuf(sig->GetPtr(), sig->GetSize());

	if ( !secKey.Verify(&tmp, hash.GetPtr(), hash.GetSize()) )
		return FALSE;

	return TRUE;

}
int CIdKey::Verify(CBuffer *bp, LPBYTE data, int datalen)
{
	int n;
	int type, nid;
	int rt = FALSE;
	CStringA keytype;
	CBuffer sig(-1);
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *md_ctx = NULL;
	const EVP_MD *evp_md = NULL;
	DSA_SIG *dsig = NULL;
	ECDSA_SIG *ecsig = NULL;
	BIGNUM *r = NULL;
	BIGNUM *s = NULL;
	u_char *sigbuf = NULL;
	CBuffer tmp(bp->GetPtr(), bp->GetSize());

	if ( tmp.GetSize() < 4 )
		return FALSE;

	bp = &tmp;
	bp->GetStr(keytype);

	if ( !GetTypeFromName(MbsToTstr(keytype), type, nid) || (type & IDKEY_TYPE_MASK) != (m_Type & IDKEY_TYPE_MASK) )
		goto ERRENDOF;

	bp->GetBuf(&sig);

	switch(m_Type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA2:
		switch(nid) {
		case NID_sha1:
			evp_md = EVP_sha1();
			break;
		case NID_sha256:
			evp_md = EVP_sha256();
			break;
		case NID_sha512:
			evp_md = EVP_sha512();
			break;
		}
		break;

	case IDKEY_DSA2:
		evp_md = EVP_sha1();

		if ( sig.GetSize() < (INTBLOB_LEN * 2) )
			goto ERRENDOF;

		dsig = DSA_SIG_new();
		r = BN_new();
		s = BN_new();

		if ( dsig == NULL || r == NULL || s == NULL )
			goto ERRENDOF;

		BN_bin2bn(sig.GetPtr(), INTBLOB_LEN, r);
		BN_bin2bn(sig.GetPtr() + INTBLOB_LEN, INTBLOB_LEN, s);

		if ( DSA_SIG_set0(dsig, r, s) == 0 )
			goto ERRENDOF;

		n = i2d_DSA_SIG(dsig, &sigbuf);

		if ( sigbuf == NULL )
			goto ERRENDOF;

		sig.Clear();
		sig.Apend(sigbuf, n);
		break;

	case IDKEY_ECDSA:
		if ( (n = GetIndexNid(nid)) < 0 )
			goto ERRENDOF;

		evp_md = GetEvpMdIdx(n);

		ecsig = ECDSA_SIG_new();
		r = sig.GetBIGNUM2(NULL);
		s = sig.GetBIGNUM2(NULL);

		if ( ecsig == NULL || r == NULL || s == NULL )
			goto ERRENDOF;

		if ( ECDSA_SIG_set0(ecsig, r, s) == 0 )
			goto ERRENDOF;

		n = i2d_ECDSA_SIG(ecsig, &sigbuf);

		if ( sigbuf == NULL )
			goto ERRENDOF;

		sig.Clear();
		sig.Apend(sigbuf, n);
		break;

	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_SLH_DSA:
		evp_md = NULL;
		break;

	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
		return CompositeVerify(&sig, data, datalen);

	case IDKEY_XMSS:
		return XmssVerify(&sig, data, datalen);

	case IDKEY_UNKNOWN:
		return FALSE;
	}

	if ( (pkey = GetEvpPkey(GETPKEY_PUBLICKEY)) == NULL )
		goto ERRENDOF;

	if ( (md_ctx = EVP_MD_CTX_new()) == NULL )
		goto ERRENDOF;

	if ( EVP_DigestVerifyInit(md_ctx, NULL, evp_md, NULL, pkey) <= 0 )
		goto ERRENDOF;

	if ( EVP_DigestVerify(md_ctx, sig.GetPtr(), sig.GetSize(), data, datalen) > 0 )
		rt = TRUE;

ERRENDOF:
	if ( md_ctx != NULL )
		EVP_MD_CTX_free(md_ctx);

	if ( pkey != NULL )
		EVP_PKEY_free(pkey);

	if ( dsig != NULL )
		DSA_SIG_free(dsig);		// include BN_free(s/r)

	if ( ecsig != NULL )
		ECDSA_SIG_free(ecsig);	// include BN_free(s/r)

	if ( sigbuf != NULL )
		OPENSSL_free(sigbuf);

	return rt;
}

#ifdef	USE_X509
int CIdKey::GetBlob_x509(CBuffer *bp, LPCSTR name)
{
	int rt = FALSE;
	int n;
	BIO *mbio;
	X509 *pX509;
	EVP_PKEY *pk;
	CIdKey key;
	CBuffer tmp(-1);

	if ( strncmp(name, "x509v3-sign-", 12) != 0 ) {	// ???
		// RFC 6187
		n = bp->Get32Bit();
		if ( n < 1 || n > 1024 )	// Cert count ?
			return FALSE;
	}

	bp->GetBuf(&tmp);
	if ( (mbio = BIO_new_mem_buf(tmp.GetPtr(), tmp.GetSize())) == NULL )
		return FALSE;

	pX509 = d2i_X509_bio(mbio, NULL);
	BIO_free_all(mbio);

	if ( pX509 == NULL )
		return FALSE;

	if ( (pk = X509_get_pubkey(pX509)) != NULL ) {
		if ( (rt = key.SetEvpPkey(pk)) != FALSE )
			*this = key;
		EVP_PKEY_free(pk);
	}

	if ( rt )
		m_Cert = IDKEY_CERTX509;

	X509_free(pX509);
	return rt;
}
#endif
int CIdKey::GetBlob(CBuffer *bp)
{
	CStringA str;
	int type, idx;
	EC_POINT *q;
	CBuffer work(-1);
	int signed_len = 0;
	CIdKey skey;

	m_Type = IDKEY_NONE;
	m_Cert = 0;
	m_CertBlob = *bp;

	bp->GetStr(str);

	if ( !GetTypeFromName(MbsToTstr(str), type, m_Nid) )
		return FALSE;

#ifdef	USE_X509
	if ( (type & IDKEY_CERTX509) != 0 )
		return GetBlob_x509(bp, str);
#endif

	if ( (type & IDKEY_CERTV01) != 0 )
		bp->GetBuf(&work);					// Skip nonce

	switch(type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA2:
		if ( !Create(IDKEY_RSA2) )
			return FALSE;
		{
			BIGNUM *e = bp->GetBIGNUM2(NULL);
			BIGNUM *n = bp->GetBIGNUM2(NULL);

			RSA_set0_key(m_Rsa, n, e, NULL);
		}
		break;

	case IDKEY_DSA2:
		if ( !Create(IDKEY_DSA2) )
			return FALSE;
		{
			BIGNUM *p = bp->GetBIGNUM2(NULL);
			BIGNUM *q = bp->GetBIGNUM2(NULL);
			BIGNUM *g = bp->GetBIGNUM2(NULL);
			BIGNUM *pub_key = bp->GetBIGNUM2(NULL);

			DSA_set0_pqg(m_Dsa, p, q, g);
			DSA_set0_key(m_Dsa, pub_key, NULL);
		}
		break;

	case IDKEY_ECDSA:
		if ( !Create(IDKEY_ECDSA) )
			return FALSE;

		bp->GetStr(str);
		if ( (idx = GetIndexName(MbsToTstr(str))) < 0 )
			return FALSE;
		m_Nid = NidListTab[idx].nid;
		if ( m_EcDsa != NULL )
			EC_KEY_free(m_EcDsa);
		if ( (m_EcDsa = EC_KEY_new_by_curve_name(m_Nid)) == NULL )
			return FALSE;
		if ( (q = EC_POINT_new(EC_KEY_get0_group(m_EcDsa))) == NULL )
			return FALSE;
		if ( !bp->GetEcPoint(EC_KEY_get0_group(m_EcDsa), q) ) {
			EC_POINT_free(q);
			return FALSE;
		}
		if ( EC_METHOD_get_field_type(EC_GROUP_method_of(EC_KEY_get0_group(m_EcDsa))) == NID_X9_62_prime_field && key_ec_validate_public(EC_KEY_get0_group(m_EcDsa), q) != 0 ) {
			EC_POINT_free(q);
			return FALSE;
		}
		if ( EC_KEY_set_public_key(m_EcDsa, q) != 1 ) {
			EC_POINT_free(q);
			return FALSE;
		}

		EC_POINT_free(q);
		break;

	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		if ( !Create(type & IDKEY_TYPE_MASK) )
			return FALSE;
		m_PublicKey.Clear();
		bp->GetBuf(&m_PublicKey);
		break;

	case IDKEY_XMSS:
		bp->GetStr(str);
		if ( !m_XmssKey.SetBufSize(str) )
			return FALSE;

		work.Clear(); bp->GetBuf(&work);				// public key
		if ( (work.GetSize() + XMSS_OID_LEN) != m_XmssKey.m_PubLen )
			return FALSE;

		memcpy(m_XmssKey.m_pPubBuf + XMSS_OID_LEN, work.GetPtr(), work.GetSize());

		m_Type = IDKEY_XMSS;
		break;

	case IDKEY_UNKNOWN:
		if ( m_TypeName.IsEmpty() || bp->GetSize() == 0 )
			return FALSE;
		m_Type = IDKEY_UNKNOWN;
		m_TypeBlob = *bp;
		break;

	default:
		return FALSE;
	}

	if ( (type & (IDKEY_CERTV00 | IDKEY_CERTV01)) != 0 ) {
		if ( (type & IDKEY_CERTV01) != 0 )
			bp->Get64Bit();								// serial
		bp->Get32Bit();									// type
		work.Clear(); bp->GetBuf(&work);				// key_id
		work.Clear(); bp->GetBuf(&work);				// principals
		bp->Get64Bit();									// valid_after
		bp->Get64Bit();									// valid_before
		work.Clear(); bp->GetBuf(&work);				// critical
		if ( (type & IDKEY_CERTV01) != 0 )
			bp->GetBuf(&work);							// exts
		if ( (type & IDKEY_CERTV00) != 0 )
			bp->GetBuf(&work);							// 
		work.Clear(); bp->GetBuf(&work);				// resv
		work.Clear(); bp->GetBuf(&work);				// sig_key
		if ( !skey.GetBlob(&work) )
			return FALSE;

		signed_len = m_CertBlob.GetSize() - bp->GetSize();
		work.Clear(); bp->GetBuf(&work);				// sig

		if ( !skey.Verify(&work, m_CertBlob.GetPtr(), signed_len) )
			return FALSE;

		m_Cert = type & IDKEY_CERT_MASK;
	}

	return TRUE;
}
int CIdKey::SetBlob(CBuffer *bp, BOOL bCert)
{
	if ( bCert && m_Cert != 0 && m_CertBlob.GetSize() > 0 ) {
		bp->Apend(m_CertBlob.GetPtr(), m_CertBlob.GetSize());
		return TRUE;
	}

	bp->PutStr(TstrToMbs(GetName()));		// Key Name ssh-rsa....

	switch(m_Type) {
	case IDKEY_RSA2:
		{
			BIGNUM const *n = NULL, *e = NULL;

			RSA_get0_key(m_Rsa, &n, &e, NULL);

			bp->PutBIGNUM2(e);
			bp->PutBIGNUM2(n);
		}
		break;
	case IDKEY_DSA2:
		{
			BIGNUM const *p = NULL, *q = NULL, *g = NULL, *pub_key = NULL;

			DSA_get0_pqg(m_Dsa, &p, &q, &g);
			DSA_get0_key(m_Dsa, &pub_key, NULL);

			bp->PutBIGNUM2(p);
			bp->PutBIGNUM2(q);
			bp->PutBIGNUM2(g);
			bp->PutBIGNUM2(pub_key);
		}
		break;
	case IDKEY_ECDSA:
		bp->PutStr(TstrToMbs(GetEcNameFromNid(m_Nid)));
		bp->PutEcPoint(EC_KEY_get0_group(m_EcDsa), EC_KEY_get0_public_key(m_EcDsa));
		break;
	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		bp->PutBuf(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
		break;
	case IDKEY_XMSS:
		bp->PutStr(m_XmssKey.GetName());
		bp->PutBuf(m_XmssKey.m_pPubBuf + XMSS_OID_LEN, m_XmssKey.m_PubLen - XMSS_OID_LEN);
		break;
	case IDKEY_UNKNOWN:
		if ( m_TypeName.IsEmpty() || m_TypeBlob.GetSize() == 0 )
			return FALSE;
		bp->Apend(m_TypeBlob.GetPtr(), m_TypeBlob.GetSize());
		break;
	default:
		return FALSE;
	}

	return TRUE;
}
int CIdKey::GetPrivateBlob(CBuffer *bp)
{
	CStringA str;
	int type, idx;
	EC_POINT *q = NULL;
	BIGNUM *p = NULL;
	CBuffer tmp(-1);
	int ret = FALSE;

	m_Type = IDKEY_NONE;

	bp->GetStr(str);

	if ( !GetTypeFromName(MbsToTstr(str), type, m_Nid) )
		return FALSE;

	if ( (type & IDKEY_CERT_MASK) != 0 ) {
		bp->GetBuf(&tmp);
		return GetBlob(&tmp);
	}

	switch(type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA2:
		if ( !Create(IDKEY_RSA2) )
			return FALSE;
		{
			BIGNUM *n = bp->GetBIGNUM2(NULL);
			BIGNUM *e = bp->GetBIGNUM2(NULL);
			BIGNUM *d = bp->GetBIGNUM2(NULL);
			BIGNUM *iqmp = bp->GetBIGNUM2(NULL);
			BIGNUM *p = bp->GetBIGNUM2(NULL);
			BIGNUM *q = bp->GetBIGNUM2(NULL);

			RSA_set0_key(m_Rsa, n, e, d);
			RSA_set0_factors(m_Rsa, p, q);
			RsaGenAddPara(iqmp);
		}
		break;

	case IDKEY_DSA2:
		if ( !Create(IDKEY_DSA2) )
			return FALSE;
		{
			BIGNUM *p = bp->GetBIGNUM2(NULL);
			BIGNUM *q = bp->GetBIGNUM2(NULL);
			BIGNUM *g = bp->GetBIGNUM2(NULL);
			BIGNUM *pub_key = bp->GetBIGNUM2(NULL);
			BIGNUM *priv_key = bp->GetBIGNUM2(NULL);

			DSA_set0_pqg(m_Dsa, p, q, g);
			DSA_set0_key(m_Dsa, pub_key, priv_key);
		}
		break;

	case IDKEY_ECDSA:
		if ( !Create(IDKEY_ECDSA) )
			return FALSE;

		bp->GetStr(str);
		if ( (idx = GetIndexName(MbsToTstr(str))) < 0 )
			return FALSE;
		m_Nid = NidListTab[idx].nid;
		if ( m_EcDsa != NULL )
			EC_KEY_free(m_EcDsa);
		if ( (m_EcDsa = EC_KEY_new_by_curve_name(m_Nid)) == NULL )
			return FALSE;
		if ( (q = EC_POINT_new(EC_KEY_get0_group(m_EcDsa))) == NULL )
			goto ENDRET;
		if ( !bp->GetEcPoint(EC_KEY_get0_group(m_EcDsa), q) )
			goto ENDRET;
		if ( EC_METHOD_get_field_type(EC_GROUP_method_of(EC_KEY_get0_group(m_EcDsa))) == NID_X9_62_prime_field &&	// XXXXXXXXX nistp ???
				key_ec_validate_public(EC_KEY_get0_group(m_EcDsa), q) != 0 )
			goto ENDRET;
		if ( EC_KEY_set_public_key(m_EcDsa, q) != 1 )
			goto ENDRET;
		if ( (p = bp->GetBIGNUM2(NULL)) == NULL )
			goto ENDRET;
		if ( EC_KEY_set_private_key(m_EcDsa, p) != 1 )
			goto ENDRET;
		break;

	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		if ( !Create(type & IDKEY_TYPE_MASK) )
			return FALSE;

		m_PublicKey.Clear();
		bp->GetBuf(&m_PublicKey);
		tmp.Clear();
		bp->GetBuf(&tmp);

		m_PrivateKey.Clear();
		m_PrivateKey.Apend(tmp.GetPtr(), tmp.GetSize() - m_PublicKey.GetSize());

		if ( memcmp(m_PublicKey.GetPtr(), tmp.GetPtr() + m_PrivateKey.GetSize(), m_PublicKey.GetSize()) != 0 )
			return FALSE;
		break;

	case IDKEY_XMSS:
		bp->GetStr(str);
		if ( !m_XmssKey.SetBufSize(str) )
			return FALSE;

		tmp.Clear(); bp->GetBuf(&tmp);
		if ( (tmp.GetSize() + XMSS_OID_LEN) != m_XmssKey.m_PubLen )
			return FALSE;
		memcpy(m_XmssKey.m_pPubBuf + XMSS_OID_LEN, tmp.GetPtr(), tmp.GetSize());

		if ( !m_XmssKey.LoadOpenSshSec(bp) )
			return FALSE;

		m_Type = IDKEY_XMSS;
		break;

	default:
		return FALSE;
	}

	ret = TRUE;

ENDRET:
	if ( q != NULL )
		EC_POINT_free(q);
	if ( p != NULL )
		BN_free(p);
	return ret;
}
int CIdKey::SetPrivateBlob(CBuffer *bp)
{
	CBuffer tmp(-1);

	if ( !m_bSecInit )
		return FALSE;

	bp->PutStr(TstrToMbs(GetName()));

	switch(m_Type) {
	case IDKEY_RSA2:
		{
			BIGNUM const *n = NULL, *e = NULL, *d = NULL, *p = NULL, *q = NULL, *iqmp = NULL;

			RSA_get0_key(m_Rsa, &n, &e, &d);
			RSA_get0_factors(m_Rsa, &p, &q);
			RSA_get0_crt_params(m_Rsa, NULL, NULL, &iqmp);

			bp->PutBIGNUM2(n);
			bp->PutBIGNUM2(e);
			bp->PutBIGNUM2(d);
			bp->PutBIGNUM2(iqmp);
			bp->PutBIGNUM2(p);
			bp->PutBIGNUM2(q);
		}
		break;
	case IDKEY_DSA2:
		{
			BIGNUM const *p = NULL, *q = NULL, *g = NULL;
			BIGNUM const *pub_key = NULL, *priv_key = NULL;

			DSA_get0_pqg(m_Dsa, &p, &q, &g);
			DSA_get0_key(m_Dsa, &pub_key, &priv_key);

			bp->PutBIGNUM2(p);
			bp->PutBIGNUM2(q);
			bp->PutBIGNUM2(g);
			bp->PutBIGNUM2(pub_key);
			bp->PutBIGNUM2(priv_key);
		}
		break;
	case IDKEY_ECDSA:
		bp->PutStr(TstrToMbs(GetEcNameFromNid(m_Nid)));
		bp->PutEcPoint(EC_KEY_get0_group(m_EcDsa), EC_KEY_get0_public_key(m_EcDsa));
		bp->PutBIGNUM2(EC_KEY_get0_private_key(m_EcDsa));
		break;
	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		bp->PutBuf(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
		bp->Put32Bit(m_PrivateKey.GetSize() + m_PublicKey.GetSize());
		bp->Apend(m_PrivateKey.GetPtr(), m_PrivateKey.GetSize());
		bp->Apend(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
		break;
	case IDKEY_XMSS:
		bp->PutStr(m_XmssKey.GetName());
		bp->PutBuf(m_XmssKey.m_pPubBuf + XMSS_OID_LEN, m_XmssKey.m_PubLen - XMSS_OID_LEN);
		if ( !m_XmssKey.SaveOpenSshSec(bp) )
			return FALSE;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

BOOL CIdKey::GetUserSid(CBuffer &buf)
{
	DWORD size = 0, dsz = 0;
	TCHAR host[MAX_COMPUTERNAME];
	TCHAR user[MAX_COMPUTERNAME];
	SID_NAME_USE snu;
	PSID pSid = NULL;
    LPTSTR pDomain = NULL;
	BOOL rt = FALSE;

	size = MAX_COMPUTERNAME;
	if ( !GetComputerName(host, &size) )
		return FALSE;

	size = MAX_COMPUTERNAME;
	if ( !GetUserName(user, &size) )
		goto ENDOF;

	LookupAccountName(host, user, NULL, &size, NULL, &dsz, &snu);

	if ( size == 0 || dsz == 0 )
		goto ENDOF;

	pSid    = new BYTE[size];
	pDomain = new TCHAR[dsz];

	if ( !LookupAccountName(host, user, pSid, &size, pDomain, &dsz, &snu) )
		goto ENDOF;

	buf.Apend((LPBYTE)pSid, size);
	rt = TRUE;

ENDOF:
	if ( pSid != NULL )
		delete [] pSid;

	if ( pDomain != NULL )
		delete [] pDomain;

	return rt;
}
BOOL CIdKey::GetHostSid(CBuffer &buf)
{
	DWORD size = 0, dsz = 0;
	TCHAR host[MAX_COMPUTERNAME];
	SID_NAME_USE snu;
	PSID pSid = NULL;
    LPTSTR pDomain = NULL;
	BOOL rt = FALSE;

	size = MAX_COMPUTERNAME;
	if ( !GetComputerName(host, &size) )
		return FALSE;

	LookupAccountName(NULL, host, NULL, &size, NULL, &dsz, &snu);

	if ( size == 0 || dsz == 0 )
		goto ENDOF;

	pSid    = new BYTE[size];
	pDomain = new TCHAR[dsz];

	if ( !LookupAccountName(NULL, host, pSid, &size, pDomain, &dsz, &snu) )
		goto ENDOF;

	buf.Apend((LPBYTE)pSid, size);
	rt = TRUE;

ENDOF:
	if ( pSid != NULL )
		delete [] pSid;

	if ( pDomain != NULL )
		delete [] pDomain;

	return rt;
}
void CIdKey::GetUserHostName(CString &str)
{
	if ( CompNameLenBugFix == FALSE ) {
		DWORD size;
		TCHAR buf[MAX_COMPUTERNAME];

		str = _T("");
		size = MAX_COMPUTERNAME_LENGTH;
		if ( GetUserName(buf, &size) )
			str += buf;

		str += _T("@");
		size = MAX_COMPUTERNAME_LENGTH;
		if ( GetComputerName(buf, &size) )
			str += buf;

	} else {
		DWORD size;
		DWORD max = MAX_COMPUTERNAME;
		TCHAR *pBuf = new TCHAR[max];

		str = _T("");
		size = max;
		if ( GetUserName(pBuf, &size) )
			str += pBuf;
		else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
			delete [] pBuf;
			max = size;
			pBuf = new TCHAR[max];
			if ( GetUserName(pBuf, &size) )
				str += pBuf;
		}

		str += _T("@");
		size = max;
		if ( GetComputerName(pBuf, &size) )
			str += pBuf;
		else if ( GetLastError() == ERROR_BUFFER_OVERFLOW ) {
			delete [] pBuf;
			max = size;
			pBuf = new TCHAR[max];
			if ( GetComputerName(pBuf, &size) )
				str += pBuf;
		}

		delete [] pBuf;
	}
}

void CIdKey::MakeKey(CBuffer *bp, LPCTSTR pass, int Mode)
{
	CString tmp;
	CBuffer mbs(-1);
	int dlen;
	u_char digest[EVP_MAX_MD_SIZE];

	switch(Mode) {
	case MAKEKEY_FIXTEXT:
		mbs = "RLoginPassKey";
		break;
	case MAKEKEY_USERHOST:
		GetUserHostName(tmp);
		mbs.SetMbsStr(tmp);
		break;
	case MAKEKEY_HOSTSID:
		if ( !GetHostSid(mbs) )
			throw _T("GetHostSid Error");
		break;
	case MAKEKEY_USERSID:
		if ( !GetUserSid(mbs) )
			throw _T("GetUserSid Error");
		break;
	}

	if ( pass != NULL )
		mbs.SetMbsStr(pass);

	dlen = MD_digest(EVP_sha1(), mbs.GetPtr(), mbs.GetSize(), digest, sizeof(digest));

	bp->Clear();
	bp->Apend(digest, dlen);
}

void CIdKey::Decrypt(CBuffer *bp, LPCTSTR str, LPCTSTR pass, int Mode)
{
	CBuffer key(-1);
	CBuffer tmp(-1);
	CCipher cip;

	tmp.Base64Decode(str);

	MakeKey(&key, pass, Mode);

	if ( Mode == MAKEKEY_USERHOST )
		cip.Init(_T("3des"), MODE_DEC, key.GetPtr(), key.GetSize());
	else
		cip.Init(_T("aes128-ctr"), MODE_DEC, key.GetPtr(), (-1));

	cip.Cipher(tmp.GetPtr(), tmp.GetSize(), bp);
}
void CIdKey::Encrypt(CString &str, LPBYTE buf, int len, LPCTSTR pass, int Mode)
{
	CBuffer key(-1);
	CBuffer tmp(-1), out(-1), sub(-1);
	CCipher cip;

	MakeKey(&key, pass, Mode);

	if ( Mode == MAKEKEY_USERHOST )
		cip.Init(_T("3des"), MODE_ENC, key.GetPtr(), key.GetSize());
	else
		cip.Init(_T("aes128-ctr"), MODE_ENC, key.GetPtr(), (-1));

	sub.Apend(buf, len);
	sub.RoundUp(16);
	cip.Cipher(sub.GetPtr(), sub.GetSize(), &tmp);
	out.Base64Encode(tmp.GetPtr(), tmp.GetSize());
	str = (LPCTSTR)out;
}
void CIdKey::DecryptStr(CString &out, LPCTSTR str, BOOL bLocalPass)
{
	CBuffer tmp(-1);
	LPCTSTR pass = NULL;

	// bLocalPass == FALSE　は、旧形式の互換モード
	if ( bLocalPass && !((CRLoginApp *)::AfxGetApp())->m_LocalPass.IsEmpty() )
		pass = ((CRLoginApp *)::AfxGetApp())->m_LocalPass;

	Decrypt(&tmp, str, pass, (bLocalPass ? ((CRLoginApp *)::AfxGetApp())->m_MakeKeyMode : MAKEKEY_USERHOST));
	out = MbsToTstr((LPCSTR)tmp);
}
void CIdKey::EncryptStr(CString &out, LPCTSTR str)
{
	CBuffer mbs(-1);
	LPCTSTR pass = NULL;

	if ( !((CRLoginApp *)::AfxGetApp())->m_LocalPass.IsEmpty() )
		pass = ((CRLoginApp *)::AfxGetApp())->m_LocalPass;

	mbs.SetMbsStr(str);
	Encrypt(out, mbs.GetPtr(), mbs.GetSize(), pass, ((CRLoginApp *)::AfxGetApp())->m_MakeKeyMode);
}
void CIdKey::Digest(CString &out, LPCTSTR str)
{
	int len;
	u_char tmp[EVP_MAX_MD_SIZE];
	CBuffer mbs(-1), digest(-1);

	mbs.SetMbsStr(str);
	mbs += "RLoginDigest";

	len = MD_digest(EVP_sha1(), mbs.GetPtr(), mbs.GetSize(), tmp, sizeof(tmp));

	digest.Base64Encode(tmp, len);
	out = (LPCTSTR)digest;
}
int CIdKey::CompPass(LPCTSTR pass)
{
	// Pageant Not Password Check
	if ( m_AgeantType != IDKEY_AGEANT_NONE )
		return 0;

	CString hash;
	Digest(hash, pass);
	return hash.Compare(m_Hash);
}
int CIdKey::InitPass(LPCTSTR pass)
{
	int n;
	CString str;
	CEditDlg dlg(CWnd::GetActiveWindow());

	if ( m_Type == IDKEY_NONE )
		return FALSE;

	if ( pass != NULL && Init(pass) )
		return TRUE;

	if ( m_Type == IDKEY_UNKNOWN )
		return FALSE;

	dlg.m_bPassword = TRUE;
	dlg.m_Edit = (pass != NULL ? pass : _T(""));
	dlg.m_WinText.LoadString(IDS_IDKEYFILELOAD);		// SSH鍵ファイルの読み込み
	dlg.m_Title.LoadString(IDS_IDKEYFILELOADCOM);		// 作成時に設定したパスフレーズを入力してください。

	dlg.m_Title += _T("\n");
	dlg.m_Title += GetName(IDKEY_NAME_CERT);

	if ( !m_Name.IsEmpty() ) {
		dlg.m_Title += _T("(");
		dlg.m_Title += m_Name;
		dlg.m_Title += _T(")");
	}

	str.Format(_T(" %dbits"), GetSize());
	dlg.m_Title += str;

	FingerPrint(str, SSHFP_DIGEST_SHA256, SSHFP_FORMAT_SIMPLE);
	dlg.m_Title += _T("\n");
	dlg.m_Title += str;

	for ( n = 0 ; n < 3 ; n++ ) {
		if ( dlg.DoModal() != IDOK )
			return FALSE;

		if ( Init(dlg.m_Edit) )
			return TRUE;

		if ( n == 0 ) {
			dlg.m_Title += _T("\n");
			dlg.m_Title += CStringLoad(IDE_IDKEYPASSERROR);
		}
	}
	return FALSE;
}
int CIdKey::ReadPublicKey(LPCTSTR str)
{
  try {
	int n;
	CString tmp;
	CBuffer buf(-1);
	BIGNUM *bn = NULL, *be = NULL;

	while ( *str == _T(' ') || *str == _T('\t') )
		str++;

	if ( _istdigit(*str) ) {	// Try SSH1 RSA
		if ( !Create(IDKEY_RSA1) )
			return FALSE;
		m_Nid = NID_sha1;

		for ( tmp.Empty() ; _istdigit(*str) ; )
			tmp += *(str++);
		if ( (n = _tstoi(tmp)) == 0 )
			return Close();

		while ( *str == _T(' ') || *str == _T('\t') )
			str++;
		for ( tmp.Empty() ; _istdigit(*str) ; )
			tmp += *(str++);

		if ( (be = BN_new()) == NULL || BN_dec2bn(&(be), (LPSTR)TstrToMbs(tmp)) == 0 )
			return Close();

		while ( *str == _T(' ') || *str == _T('\t') )
			str++;
		for ( tmp.Empty() ; _istdigit(*str) ; )
			tmp += *(str++);
		if ( (bn = BN_new()) == NULL || BN_dec2bn(&(bn), (LPSTR)TstrToMbs(tmp)) == 0 )
			return Close();

		RSA_set0_key(m_Rsa, bn, be, NULL);

	} else if ( (str = _tcschr(str, ' ')) != NULL ) {

		buf.Base64Decode(str + 1);

		if ( !GetBlob(&buf) )
			return FALSE;

	} else
		return FALSE;

	return TRUE;

  } catch(...) {
	return FALSE;
  }
}
int CIdKey::WritePublicKey(CString &str, BOOL bAddUser)
{
	char *p;
	CBuffer tmp(-1), buf(-1);
	CString user;

	if ( bAddUser )
		GetUserHostName(user);

	switch(m_Type) {
	case IDKEY_RSA1:
		{
			BIGNUM const *n = NULL, *e = NULL;

			RSA_get0_key(m_Rsa, &n, &e, NULL);

			if ( n == NULL || e == NULL )
				return FALSE;

			str.Format(_T("%u"), BN_num_bits(n));
			if ( (p = BN_bn2dec(e)) == NULL )
				return FALSE;
			str += _T(" ");
			str += MbsToTstr(p);
			free(p);
			if ( (p = BN_bn2dec(n)) == NULL )
				return FALSE;
			str += _T(" ");
			str += MbsToTstr(p);
			free(p);
		}
		break;
	case IDKEY_RSA2:
	case IDKEY_DSA2:
	case IDKEY_ECDSA:
	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
	case IDKEY_XMSS:
	case IDKEY_UNKNOWN:
		if ( !SetBlob(&tmp, bAddUser ? TRUE : FALSE) )
			return FALSE;
		buf.Base64Encode(tmp.GetPtr(), tmp.GetSize());
		str = GetName(IDKEY_NAME_CERT);
		str += _T(" ");
		str += (LPCTSTR)buf;
		if ( bAddUser ) {
			str += _T(" ");
			str += user;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
int CIdKey::ReadPrivateKey(LPCTSTR str, LPCTSTR pass, BOOL bHost)
{
	 try {
		int type, nid;
		CBuffer tmp(-1), work(-1);
		BIGNUM *p;

		Decrypt(&tmp, str, pass, (bHost ? MAKEKEY_USERHOST : MAKEKEY_FIXTEXT));

		if ( tmp.GetSize() <= 0 )
			throw _T("Data empty");

		type = tmp.Get8Bit() & (IDKEY_EXT_FLAG | IDKEY_TYPE_MASK);

		switch(type) {
		case IDKEY_RSA1:
		case IDKEY_RSA2:
			if ( !IDKEY_REQ(m_Type, type) || m_Rsa == NULL )
				throw _T("Rsa type or data empty");

			{
				BIGNUM *d = tmp.GetBIGNUM(NULL);
				BIGNUM *iqmp = tmp.GetBIGNUM(NULL);
				BIGNUM *q = tmp.GetBIGNUM(NULL);
				BIGNUM *p = tmp.GetBIGNUM(NULL);

				RSA_set0_key(m_Rsa, NULL, NULL, d);		// n,e check
				RSA_set0_factors(m_Rsa, p, q);
				RsaGenAddPara(iqmp);
			}
			break;

		case IDKEY_DSA2:
			if ( !IDKEY_REQ(m_Type, IDKEY_DSA2)|| m_Dsa == NULL )
				throw _T("Dsa type or data empty");
			m_Type = IDKEY_DSA2 | (m_Type & IDKEY_CERT_MASK);

			{
				BIGNUM *priv_key = tmp.GetBIGNUM(NULL);

				DSA_set0_key(m_Dsa, NULL, priv_key);	// pub_key check
			}
			break;

		case IDKEY_ECDSA:
			if ( !IDKEY_REQ(m_Type, IDKEY_ECDSA) || m_EcDsa == NULL )
				throw _T("Ecdsa type or data empty");
			m_Type = IDKEY_ECDSA | (m_Type & IDKEY_CERT_MASK);

			nid = tmp.Get32Bit();
			if ( m_Nid != nid || EC_GROUP_get_curve_name(EC_KEY_get0_group(m_EcDsa)) != nid )
				throw _T("Ecdsa group nid not match");
			if ( (p = BN_new()) == NULL )
				throw _T("Ecdsa bn new error");
			tmp.GetBIGNUM(p);
			if ( EC_KEY_set_private_key(m_EcDsa, p) != 1 ) {
				BN_free(p);
				throw _T("Ecdsa set private key error");
			}
			BN_free(p);
			break;

		case IDKEY_DSA2 | IDKEY_EXT_FLAG:
			if ( !IDKEY_REQ(m_Type, IDKEY_DSA2) || m_Dsa == NULL )
				throw _T("Dsa2ex type or data empty");
			m_Type = IDKEY_DSA2 | (m_Type & IDKEY_CERT_MASK);

			{
				BIGNUM *priv_key = tmp.GetBIGNUM2(NULL);

				DSA_set0_key(m_Dsa, NULL, priv_key);	// pub_key check
			}
			break;

		case IDKEY_ECDSA | IDKEY_EXT_FLAG:
			if ( !IDKEY_REQ(m_Type, IDKEY_ECDSA) || m_EcDsa == NULL )
				throw _T("EcdsaEx type or data empty");
			m_Type = IDKEY_ECDSA | (m_Type & IDKEY_CERT_MASK);

			nid = tmp.Get32Bit();
			if ( m_Nid != nid || EC_GROUP_get_curve_name(EC_KEY_get0_group(m_EcDsa)) != nid )
				throw _T("EcdsaEx group nid not match");
			if ( (p = BN_new()) == NULL )
				throw _T("EcdsaEx bn new error");
			tmp.GetBIGNUM2(p);
			if ( EC_KEY_set_private_key(m_EcDsa, p) != 1 ) {
				BN_free(p);
				throw _T("EcdsaEx set private key error");
			}
			BN_free(p);
			break;

		case IDKEY_ED25519:
			nid = NID_ED25519;
			goto NEWKEY;
		case IDKEY_ED448:
			nid = NID_ED448;
		NEWKEY:
			if ( !IDKEY_REQ(m_Type, type) || m_Nid != nid )
				throw _T("Ed25519/448 type or data empty");

			tmp.GetBuf(&work);
			if ( m_PublicKey.GetSize() != work.GetSize() || memcmp(m_PublicKey.GetPtr(), work.GetPtr(), m_PublicKey.GetSize()) != 0 )
				throw _T("Ed25519/448 public key not match");

			work.Clear();
			tmp.GetBuf(&work);
			if ( work.GetSize() <= m_PublicKey.GetSize() )
				throw _T("Ed25519/448 private key size error");

			m_PrivateKey.Clear(); 
			m_PrivateKey.Apend(work.GetPtr(), work.GetSize() - m_PublicKey.GetSize());

			if ( (m_PrivateKey.GetSize() + m_PublicKey.GetSize()) != work.GetSize() || memcmp(m_PublicKey.GetPtr(), work.GetPtr() + m_PrivateKey.GetSize(), m_PublicKey.GetSize()) != 0 )
				throw _T("Ed25519/448 private key not match");
			break;

		case IDKEY_ML_DSA:
		case IDKEY_ML_DSA_ES:
		case IDKEY_ML_DSA_ED:
		case IDKEY_SLH_DSA:
			nid = tmp.Get32Bit();
			if ( !IDKEY_REQ(m_Type, type) || m_Nid != nid )
				throw _T("MlDsa/NSlhDsa type or nid not match");

			tmp.GetBuf(&work);
			if ( m_PublicKey.GetSize() != work.GetSize() || memcmp(m_PublicKey.GetPtr(), work.GetPtr(), m_PublicKey.GetSize()) != 0 )
				throw _T("MlDsa/NSlhDsa public key not match");

			m_PrivateKey.Clear(); 
			tmp.GetBuf(&m_PrivateKey);
			break;

		case IDKEY_XMSS:
			if ( !IDKEY_REQ(m_Type, type) )
				throw _T("Xmss type not match");

			if ( !m_XmssKey.SetBufSize(tmp.Get32Bit()) )
				throw _T("Xmss buffer size error");

			work.Clear(); tmp.GetBuf(&work);
			if ( (work.GetSize() + XMSS_OID_LEN) != m_XmssKey.m_PubLen )
				throw _T("Xmss public key size error");
			memcpy(m_XmssKey.m_pPubBuf + XMSS_OID_LEN, work.GetPtr(), work.GetSize());

			work.Clear(); tmp.GetBuf(&work);
			if ( (work.GetSize() + XMSS_OID_LEN) != m_XmssKey.m_SecLen )
				throw _T("Xmss private key size error");
			memcpy(m_XmssKey.m_pSecBuf + XMSS_OID_LEN, work.GetPtr(), work.GetSize());
			break;

		default:
			throw _T("Key type error");
		}
		return TRUE;

	} catch(LPCTSTR msg) {
		::ThreadMessageBox(_T("%s"), msg);
		return FALSE;
	} catch(...) {
		return FALSE;
	}
}
int CIdKey::WritePrivateKey(CString &str, LPCTSTR pass)
{
	CBuffer tmp(-1), work(-1);

	switch(m_Type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		tmp.Put8Bit(m_Type & IDKEY_TYPE_MASK);
		{
			BIGNUM const *d = NULL, *iqmp = NULL, *q = NULL, *p = NULL;

			RSA_get0_key(m_Rsa, NULL, NULL, &d);
			RSA_get0_factors(m_Rsa, &p, &q);
			RSA_get0_crt_params(m_Rsa, NULL, NULL, &iqmp);

			tmp.PutBIGNUM(d);
			tmp.PutBIGNUM(iqmp);
			tmp.PutBIGNUM(q);
			tmp.PutBIGNUM(p);
		}
		break;
	case IDKEY_DSA2:
		tmp.Put8Bit(IDKEY_DSA2 | IDKEY_EXT_FLAG);
		{
			BIGNUM const *priv_key = NULL;

			DSA_get0_key(m_Dsa, NULL, &priv_key);

			tmp.PutBIGNUM2(priv_key);
		}
		break;
	case IDKEY_ECDSA:
		tmp.Put8Bit(IDKEY_ECDSA | IDKEY_EXT_FLAG);
		tmp.Put32Bit(m_Nid);
		tmp.PutBIGNUM2(EC_KEY_get0_private_key(m_EcDsa));
		break;
	case IDKEY_ED25519:
	case IDKEY_ED448:
		tmp.Put8Bit(m_Type & IDKEY_TYPE_MASK);
		tmp.PutBuf(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
		tmp.Put32Bit(m_PrivateKey.GetSize() + m_PublicKey.GetSize());
		tmp.Apend(m_PrivateKey.GetPtr(), m_PrivateKey.GetSize());
		tmp.Apend(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
		break;
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		tmp.Put8Bit(m_Type & IDKEY_TYPE_MASK);
		tmp.Put32Bit(m_Nid);
		tmp.PutBuf(m_PublicKey.GetPtr(), m_PublicKey.GetSize());
		tmp.PutBuf(m_PrivateKey.GetPtr(), m_PrivateKey.GetSize());
		break;
	case IDKEY_XMSS:
		tmp.Put8Bit(m_Type & IDKEY_TYPE_MASK);
		tmp.Put32Bit(m_XmssKey.m_oId);
		tmp.PutBuf(m_XmssKey.m_pPubBuf + XMSS_OID_LEN, m_XmssKey.m_PubLen - XMSS_OID_LEN);
		tmp.PutBuf(m_XmssKey.m_pSecBuf + XMSS_OID_LEN, m_XmssKey.m_SecLen - XMSS_OID_LEN);
		break;
	default:
		return FALSE;
	}

	m_bHostPass = FALSE;
	Encrypt(str, tmp.GetPtr(), tmp.GetSize(), pass, MAKEKEY_FIXTEXT);
	SetPass(pass);
	m_bSecInit = TRUE;

	return TRUE;
}

static const char authfile_id_string[] = "SSH PRIVATE KEY FILE FORMAT 1.1\n";

int CIdKey::SaveRsa1Key(FILE *fp, LPCTSTR pass)
{
	CBuffer tmp(-1);

	BIGNUM const *n = NULL, *e = NULL, *d = NULL;
	BIGNUM const *p = NULL, *q = NULL;
	BIGNUM const *iqmp = NULL;

	RSA_get0_key(m_Rsa, &n, &e, &d);
	RSA_get0_factors(m_Rsa, &p, &q);
	RSA_get0_crt_params(m_Rsa, NULL, NULL, &iqmp);

	tmp.Apend((LPBYTE)authfile_id_string, sizeof(authfile_id_string));
	tmp.Put8Bit(SSH_CIPHER_3DES);
	tmp.Put32Bit(0);
	tmp.Put32Bit(0);

	tmp.PutBIGNUM(n);
	tmp.PutBIGNUM(e);

	CString com;
	GetUserHostName(com);
	tmp.PutStr(TstrToMbs(com));

	int ck = rand();
	CBuffer out;
	out.Put16Bit(ck);
	out.Put16Bit(ck);

	out.PutBIGNUM(d);
	out.PutBIGNUM(iqmp);
	out.PutBIGNUM(q);
	out.PutBIGNUM(p);

	CCipher cip;
	CBuffer key;
	cip.MakeKey(&key, pass);
	if ( !cip.Init(cip.GetName(SSH_CIPHER_3DES), MODE_ENC, key.GetPtr(), key.GetSize()) )
		return FALSE;
	cip.Cipher(out.GetPtr(), out.GetSize(), &tmp);

	if ( fwrite(tmp.GetPtr(), tmp.GetSize(), 1, fp) != 1 )
		return FALSE;

	return TRUE;
}
int CIdKey::LoadRsa1Key(FILE *fp, LPCTSTR pass)
{
	try {
		fseek(fp, 0L, SEEK_END);
		LONGLONG llen = _ftelli64(fp);

		if ( llen > (256 * 1024) )
			throw _T("idkey rsa1 file too big error");

		long len = (long)llen;
		CBuffer buf(-1);
		LPBYTE p = buf.PutSpc(len);
		fseek(fp, 0L, SEEK_SET);
		fread(p, 1, len, fp);

		if ( memcmp(p, authfile_id_string, sizeof(authfile_id_string)) != 0 )
			throw _T("idkey rsa1 file load error");

		buf.Consume(sizeof(authfile_id_string));
		int ctype = buf.Get8Bit();		// cipher type
		buf.Get32Bit();					// reserved 
		buf.Get32Bit();					// reserved

		Create(IDKEY_RSA1);
		m_Nid = NID_sha1;

		{
			BIGNUM *n = buf.GetBIGNUM(NULL);
			BIGNUM *e = buf.GetBIGNUM(NULL);

			RSA_set0_key(m_Rsa, n, e, NULL);
		}

		CStringA com;
		buf.GetStr(com);

		CCipher cip;
		CBuffer out, key;
		cip.MakeKey(&key, pass);
		if ( !cip.Init(cip.GetName(ctype), MODE_DEC, key.GetPtr(), key.GetSize()) )
			throw _T("Chipher init error");
		cip.Cipher(buf.GetPtr(), buf.GetSize(), &out);

		int ck1 = out.Get8Bit();
		int ck2 = out.Get8Bit();
		if ( ck1 != out.Get8Bit() || ck2 != out.Get8Bit() )
			throw _T("idkey rsa1 check error");

		{
			BIGNUM *d = buf.GetBIGNUM(NULL);
			BIGNUM *iqmp = buf.GetBIGNUM(NULL);
			BIGNUM *q = buf.GetBIGNUM(NULL);
			BIGNUM *p = buf.GetBIGNUM(NULL);

			RSA_set0_key(m_Rsa, NULL, NULL, d);		// n,e check
			RSA_set0_factors(m_Rsa, p, q);
			RsaGenAddPara(iqmp);
		}

		if ( RSA_blinding_on(m_Rsa, NULL) != 1 )
			throw _T("idkey rsa1 blind error");

		m_Type = IDKEY_RSA1;
		//fclose(fp);
		return TRUE;

	} catch(...) {
		m_Type = IDKEY_NONE;
		//fclose(fp);
		return FALSE;
	}
}
static LPCTSTR fgetline(FILE *fp, CString &str)
{
	char *p;
	char buf[1024];

	str.Empty();
	while ( fgets(buf, 1024, fp) != NULL ) {
		if ( (p = strchr(buf, '\r')) != NULL )
			*p = '\0';
		if ( (p = strchr(buf, '\n')) != NULL )
			*p = '\0';
		str += MbsToTstr(buf);
		if ( str.GetLength() > 0 && str.Right(1).Compare(_T("\\")) == 0 ) {
			str.Delete(str.GetLength() - 1, 1);
			continue;
		}
		if ( !str.IsEmpty() )
			return str;
	}
	return NULL;
}

static const char openssh_auth_magic[] = "openssh-key-v1";

int CIdKey::LoadOpenSshKey(FILE *fp, LPCTSTR pass)
{
/*
 * openssh key format
 *
 */
	try {
		int st = 0;
		int len, keylen, ivlen;
		CString line, text;
		CStringA kdfname, encname, comment;
		CBuffer body(-1), keyiv(-1), kdf(-1), pub(-1), dec(-1);
		CCipher cip;
		CIdKey pubkey;

		fseek(fp, 0L, SEEK_SET);

		while ( fgetline(fp, line) != NULL ) {
			if ( line.CompareNoCase(_T("-----BEGIN OPENSSH PRIVATE KEY-----")) == 0 ) {
				if ( st == 0 ) {
					st = 1;
					continue;
				} else
					break;
			} else if ( line.CompareNoCase(_T("-----END OPENSSH PRIVATE KEY-----")) == 0 ) {
				st = 2;
				break;
			}

			text += line;
		}

		if ( st != 2 )
			throw _T("not found mark");

		if ( text.IsEmpty() )
			throw _T("not body");

		body.Base64Decode(text);

		if ( body.GetSize() < sizeof(openssh_auth_magic) )
			throw _T("no magic");

		if ( memcmp(body.GetPtr(), openssh_auth_magic, sizeof(openssh_auth_magic)) != 0 )
			throw _T("bad magic");
		body.Consume(sizeof(openssh_auth_magic));

		body.GetStr(encname);				// ciper name aes256-cbc ...

		body.GetStr(kdfname);					// kdfname
		if ( kdfname.Compare("none") != 0 && kdfname.Compare("bcrypt") != 0 )
			throw _T("unknown kdf name");
		body.GetBuf(&kdf);					// kdfopt

		if ( encname.Compare("none") != 0 && kdfname.Compare("none") == 0 )
			throw _T("requires kdf name");

		if ( body.Get32Bit() != 1 )			// number of key
			throw _T("Only one key supported");

		body.GetBuf(&pub);					// pubkey
		if ( !pubkey.GetBlob(&pub) )
			throw _T("bad public key");

		pubkey.FingerPrint(line, SSHFP_DIGEST_SHA256, SSHFP_FORMAT_SIMPLE);
		m_LoadMsg.Format(_T("%s(%d) %32.32s..."), pubkey.GetName(), pubkey.GetSize(), (LPCTSTR)line);

		len = body.Get32Bit();				// size of encrypted key blob

		keylen = cip.GetKeyLen(MbsToTstr(encname));
		ivlen  = cip.GetBlockSize(MbsToTstr(encname));
		keyiv.PutSpc(keylen + ivlen);

		if ( kdfname.Compare("bcrypt") == 0 ) {
			int round;
			CBuffer salt(-1), psx(-1);
			psx.SetMbsStr(pass);

			kdf.GetBuf(&salt);
			round = kdf.Get32Bit();
			
			bcrypt_pbkdf((const char *)psx.GetPtr(), psx.GetSize(), salt.GetPtr(), salt.GetSize(), keyiv.GetPtr(), keylen + ivlen, round);
		} else
			ZeroMemory(keyiv.GetPtr(), keylen + ivlen);

		if ( !cip.Init(MbsToTstr(encname), MODE_DEC, keyiv.GetPtr(), keylen, keyiv.GetPtr() + keylen) )
			throw _T("chipher init error");

		if ( !cip.Cipher(body.GetPtr(), len, &dec) )
			throw _T("chipher error");
		body.Consume(len);

		if ( dec.Get32Bit() != dec.Get32Bit() )		// check1 check2
			throw _T("check bytes missing");

		if ( !GetPrivateBlob(&dec) )				// blob
			throw _T("private key init error");

		dec.GetStr(comment);						// comment
		m_Name = MbsToTstr(comment);

		for ( int i = 1 ; dec.GetSize() > 0 ; i++ ) {
			if ( dec.Get8Bit() != i )
				throw _T("bad padding");
		}

		if ( ComperePublic(&pubkey) != 0 )
			throw _T("not match public key");

		//fclose(fp);
		return TRUE;

	} catch(...) {
		m_Type = IDKEY_NONE;
		//fclose(fp);
		return FALSE;
	}
}
int CIdKey::SaveOpenSshKey(FILE *fp, LPCTSTR pass)
{
	CBuffer body(-1), keyiv(-1), kdf(-1), blob(-1), psx(-1), enc, tmp;
	CString text;
	LPCSTR encname = "aes256-ctr";
	BYTE salt[16];
	int pad, keylen, ivlen, round;
	CCipher cip;
	BOOL bNoPass = FALSE;

	if ( !Init(pass) )
		return FALSE;

	if ( pass == NULL || *pass == _T('\0') ) {
		bNoPass = TRUE;
		encname = "none";
	}

	keylen = cip.GetKeyLen(MbsToTstr(encname));
	ivlen  = cip.GetBlockSize(MbsToTstr(encname));
	keyiv.PutSpc(keylen + ivlen);
	ZeroMemory(keyiv.GetPtr(), keylen + ivlen);
	round = 16;
	rand_buf(salt, 16);
	psx.SetMbsStr(pass);
	rand_buf(&pad, sizeof(pad));

	if ( !bNoPass && bcrypt_pbkdf((const char *)psx.GetPtr(), psx.GetSize(), salt, 16, keyiv.GetPtr(), keylen + ivlen, round) )
		return FALSE;

	kdf.PutBuf(salt, 16);
	kdf.Put32Bit(round);

	if ( !cip.Init(MbsToTstr(encname), MODE_ENC, keyiv.GetPtr(), keylen, keyiv.GetPtr() + keylen) )
		return FALSE;

	blob.Put32Bit(pad);
	blob.Put32Bit(pad);
	SetPrivateBlob(&blob);
	blob.PutStr(TstrToMbs(m_Name));
	for ( pad = 1 ; (blob.GetSize() % ivlen) != 0 ; pad++ )
		blob.Put8Bit(pad);

	if ( !cip.Cipher(blob.GetPtr(), blob.GetSize(), &enc) )
		return FALSE;

	body.Apend((LPBYTE)openssh_auth_magic, sizeof(openssh_auth_magic));
	body.PutStr(encname);

	if ( bNoPass ) {
		body.PutStr("none");
		body.Put32Bit(0);
	} else {
		body.PutStr("bcrypt");
		body.PutBuf(kdf.GetPtr(), kdf.GetSize());
	}

	body.Put32Bit(1);		// number of key
	blob.Clear(); SetBlob(&blob);
	body.PutBuf(blob.GetPtr(), blob.GetSize());
	body.PutBuf(enc.GetPtr(), enc.GetSize());

	tmp.Base64Encode(body.GetPtr(), body.GetSize());

	fputs("-----BEGIN OPENSSH PRIVATE KEY-----\n", fp);

	text.Empty();
	for ( LPCTSTR p = (LPCTSTR)tmp ; *p != _T('\0') ; ) {
		text += *(p++);
		if ( text.GetLength() > 70 || *p == _T('\0') ) {
			text += _T("\n");
			fputs(TstrToMbs(text), fp);
			text.Empty();
		}
	}

	fputs("-----END OPENSSH PRIVATE KEY-----\n", fp);

	return TRUE;
}
int CIdKey::LoadSecShKey(FILE *fp, LPCTSTR pass)
{
/*
 * Code to read ssh.com private keys.
 *
 *  "---- BEGIN SSH2 ENCRYPTED PRIVATE KEY ----"
 *  "Comment:..."
 *  "Base64..."
 *  "---- END SSH2 ENCRYPTED PRIVATE KEY ----"
 *
 * Body of key data
 *
 *  - uint32 0x3f6ff9eb       (magic number)
 *  - uint32 size             (total blob size)
 *  - string key-type         (see below)
 *  - string cipher-type      (tells you if key is encrypted)
 *  - string encrypted-blob
 *
 * The key type strings are ghastly. The RSA key I looked at had a type string of
 * 
 *   `if-modn{sign{rsa-pkcs1-sha1},encrypt{rsa-pkcs1v2-oaep}}'
 *   `dl-modp{sign{dsa-nist-sha1},dh{plain}}'
 *   `ec-modp'
 *
 * The encryption. The cipher-type string appears to be either
 *
 *    `none'
 *    `3des-cbc'
 *
 * The payload blob, for an RSA key, contains:
 *  - mpint e
 *  - mpint d
 *  - mpint n  (yes, the public and private stuff is intermixed)
 *  - mpint u  (presumably inverse of p mod q)
 *  - mpint p  (p is the smaller prime)
 *  - mpint q  (q is the larger)
 * 
 * For a DSA key, the payload blob contains:
 *  - uint32 0
 *  - mpint p
 *  - mpint g
 *  - mpint q
 *  - mpint y
 *  - mpint x
 *
 * For a ECDSA key, the payload blob contains:
 *  - uint32 1
 *  - string [identifier] ("nistp256" or "nistp384" or "nistp521")
 *  - mpint  n
 */
	try {
		int st = 0;
		int len;
		int type;
		BOOL encflag;
		CStringA str, encname;
		CString line, text;
		CBuffer body(-1), blob(-1);

		fseek(fp, 0L, SEEK_SET);

		while ( fgetline(fp, line) != NULL ) {
			if ( line.CompareNoCase(_T("---- BEGIN SSH2 ENCRYPTED PRIVATE KEY ----")) == 0 ) {
				if ( st == 0 ) {
					st = 1;
					continue;
				} else
					break;
			} else if ( line.CompareNoCase(_T("---- END SSH2 ENCRYPTED PRIVATE KEY ----")) == 0 )
				break;
			
			if ( st == 0 )
				continue;

			if ( line.Find(_T(':')) >= 0 ) {
				if ( st == 1 )
					continue;
				throw _T("header found in body of key data");
			} else if ( st == 1 )
				st = 2;

			text += line;
		}

		body.Base64Decode(text);

		if ( body.GetSize() < 8 )
			throw _T("key body not present");

		if ( body.Get32Bit() != 0x3f6ff9eb )
			throw _T("magic number error");

		if ( (len = body.Get32Bit()) == 0 || len > (body.GetSize() + 8) )
			throw _T("body size error");

		body.GetStr(str);
		if ( strncmp(str, "if-modn{sign{rsa", 16) == 0 )
			type = IDKEY_RSA2;
		else if ( strncmp(str, "dl-modp{sign{dsa", 16) == 0 )
			type = IDKEY_DSA2;
		else if ( strncmp(str, "ec-modp", 7) == 0 )
			type = IDKEY_ECDSA;
		else
			throw _T("key unknown type");

		body.GetStr(encname);
		if ( strcmp(str, "none") == 0 )
			encflag = FALSE;
		else
			encflag = true;

		if ( (len = body.Get32Bit()) == 0 || len > body.GetSize() )
			throw _T("body size error");

		if ( encflag ) {
			CBuffer tmp;
			u_char keybuf[32], iv[16];
			CCipher cip;

			str = pass;
			tmp.Apend((LPBYTE)(LPCSTR)str, str.GetLength());
			MD_digest(EVP_md5(), tmp.GetPtr(), tmp.GetSize(), keybuf, 16);
			tmp.Apend(keybuf, 16);
			MD_digest(EVP_md5(), tmp.GetPtr(), tmp.GetSize(), keybuf + 16, 16);

	        memset(iv, 0, sizeof(iv));
			if ( !cip.Init(MbsToTstr(encname), MODE_DEC, keybuf, (-1), iv) )
				throw _T("Chipher init error");
			cip.Cipher(body.GetPtr(), len, &blob);
		} else
			blob.Apend(body.GetPtr(), len);

		if ( (len = blob.Get32Bit()) == 0 || len > blob.GetSize() )
			throw _T("blob size error");

		Create(type);

		if ( type == IDKEY_RSA2 ) {
			m_Nid = NID_sha1;
 			BIGNUM *e = blob.GetBIGNUM_SecSh(NULL);			// mpint e
 			BIGNUM *d = blob.GetBIGNUM_SecSh(NULL);			// mpint d
			BIGNUM *n = blob.GetBIGNUM_SecSh(NULL);			// mpint n  (yes, the public and private stuff is intermixed)
			BIGNUM *iqmp = blob.GetBIGNUM_SecSh(NULL);		// mpint u  (presumably inverse of p mod q)
			BIGNUM *p = blob.GetBIGNUM_SecSh(NULL);			// mpint p  (p is the smaller prime)
			BIGNUM *q = blob.GetBIGNUM_SecSh(NULL);			// mpint q  (q is the larger)

			RSA_set0_key(m_Rsa, n, e, d);
			RSA_set0_factors(m_Rsa, p, q);
			RsaGenAddPara(iqmp);

		} else if ( type == IDKEY_DSA2 ) {
			m_Nid = NID_undef;
			if ( blob.Get32Bit() != 0 )						//  uint32 0
				throw _T("predefined DSA parameters not supported");

			BIGNUM *p = blob.GetBIGNUM_SecSh(NULL);			// mpint p
			BIGNUM *g = blob.GetBIGNUM_SecSh(NULL);			// mpint g
			BIGNUM *q = blob.GetBIGNUM_SecSh(NULL);			// mpint q
			BIGNUM *pub_key = blob.GetBIGNUM_SecSh(NULL);	// mpint y
			BIGNUM *priv_key = blob.GetBIGNUM_SecSh(NULL);	// mpint x

			DSA_set0_pqg(m_Dsa, p, q, g);
			DSA_set0_key(m_Dsa, pub_key, priv_key);

		} else if ( type == IDKEY_ECDSA ) {
			if ( blob.Get32Bit() != 1 )						//  uint32 1
				throw _T("predefined ECDSA parameters not supported");

			int n;
			EC_POINT *q;
			BIGNUM *p;
			BN_CTX *ctx;

			blob.GetStr(str);							// string [identifier] ("nistp256" or "nistp384" or "nistp521")
			if ( (n = GetIndexName(MbsToTstr(str))) < 0 )
				throw _T("ec curve name error");

			m_Nid = NidListTab[n].nid;
			if ( m_EcDsa != NULL )
				EC_KEY_free(m_EcDsa);
			if ( (m_EcDsa = EC_KEY_new_by_curve_name(m_Nid)) == NULL )
				throw _T("ec curve name error");

			if ( (q = EC_POINT_new(EC_KEY_get0_group(m_EcDsa))) == NULL )
				throw _T("ec get0 group error");

			if ( (p = blob.GetBIGNUM_SecSh(NULL)) == NULL )
				throw _T("ec get private bignum error");

			if ( EC_KEY_set_private_key(m_EcDsa, p) != 1 )
				throw _T("ec set private key error");

			if ((ctx = BN_CTX_new()) == NULL)
				throw _T("ec ctx new error");

			if ( !EC_POINT_mul(EC_KEY_get0_group(m_EcDsa), q, p, NULL, NULL, ctx))
				throw _T("ec point mul error");

			if ( EC_KEY_set_public_key(m_EcDsa, q) != 1 )
				throw _T("ec set public key error");

			BN_CTX_free(ctx);
			BN_free(p);
			EC_POINT_free(q);

		} else
			throw _T("key type error"); 

		//fclose(fp);
		return TRUE;

	} catch(...) {
		m_Type = IDKEY_NONE;
		//fclose(fp);
		return FALSE;
	}
}
static int getparse(LPCTSTR str, LPCTSTR cmd, CString &para)
{
	int len = (int)_tcslen(cmd);

	if ( _tcsnicmp(str, cmd, len) != 0 )
		return FALSE;

	str += len;
	while ( *str == _T(' ') || *str == _T('\t') )
		str++;

	if ( *str != _T(':') )
		return FALSE;
	str++;

	while ( *str == _T(' ') || *str == _T('\t') )
		str++;

	if ( *str == _T('"') ) {
		str++;
		para.Empty();
		while ( *str != _T('\0') && *str != _T('"') )
			para += *(str++);
	} else if ( *str == _T('\'') ) {
		str++;
		para.Empty();
		while ( *str != _T('\0') && *str != _T('\'') )
			para += *(str++);
	} else
		para = str;

	return TRUE;
}
int CIdKey::LoadPuttyKey(FILE *fp, LPCTSTR pass)
{
/*
 * PuTTY-User-Key-File-2: ssh-dss
 * Encryption: aes256-cbc
 * Comment: imported-openssh-key
 * Public-Lines: 10
 * Base64...
 * Private-Lines: 1
 * Base64...
 * Private-MAC: Base16...
 * Private-Hash: Base16... (PuTTY-User-Key-File-1) ???
 * 
 * for "ssh-rsa", it will be composed of
 *
 * "Public-Lines: " plus a number N.
 *
 *    string "ssh-rsa"
 *    mpint  exponent
 *    mpint  modulus
 *
 * "Private-Lines: " plus a number N,
 *
 *    mpint  private_exponent
 *    mpint  p                  (the larger of the two primes)
 *    mpint  q                  (the smaller prime)
 *    mpint  iqmp               (the inverse of q modulo p)
 *    data   padding            (to reach a multiple of the cipher block size)
 *
 * for "ssh-dss", it will be composed of
 *
 * "Public-Lines: " plus a number N.
 *
 *    string "ssh-rsa"
 *    mpint p
 *    mpint q
 *    mpint g
 *    mpint y
 *
 * "Private-Lines: " plus a number N,
 *
 *    mpint  x                  (the private key parameter)
 */
	try {
		int n, len;
		int type = IDKEY_NONE;
		CString line, para;
		CString keyname, encname, comment;
		CString pubkey, prikey;
		CBuffer pubblob(-1), priblob(-1), encblob(-1), pubsave(-1);
		CStringA str;
		int version = 0;
		CString keyDeriv;
		int argon2Memory = 0;
		int argon2Passes = 0;
		int argon2Parallelism = 0;
		CString argon2Salt;
		CString private_mac;

		fseek(fp, 0L, SEEK_SET);

		while ( fgetline(fp, line) != NULL ) {
			if ( getparse(line, _T("PuTTY-User-Key-File-3"), para) || getparse(line, _T("PuTTY-User-Key-File-2"), para) || getparse(line, _T("PuTTY-User-Key-File-1"), para) ) {
				version = _tstoi((LPCTSTR)line + 20);
				keyname = para;
				if ( para.CompareNoCase(_T("ssh-dss")) == 0 )
					type = IDKEY_DSA2;
				else if ( para.CompareNoCase(_T("ssh-rsa")) == 0 )
					type = IDKEY_RSA2;
				else if ( para.CompareNoCase(_T("ecdsa-sha2-nistp256")) == 0 || para.CompareNoCase(_T("ecdsa-sha2-nistp384")) == 0 || para.CompareNoCase(_T("ecdsa-sha2-nistp521")) == 0 )
					type = IDKEY_ECDSA;
				else if ( para.CompareNoCase(_T("ssh-ed25519")) == 0 )
					type = IDKEY_ED25519;
				else if ( para.CompareNoCase(_T("ssh-ed448")) == 0 )
					type = IDKEY_ED448;
				else
					throw _T("unknown type");

			} else if ( getparse(line, _T("Encryption"), para) ) {
				encname = para;
			} else if ( getparse(line, _T("Comment"), para) ) {
				comment = para;
			} else if ( getparse(line, _T("Public-Lines"), para) ) {
				len = _tstoi(para);
				pubkey.Empty();
				for ( n = 0 ; n < len && fgetline(fp, line) != NULL ; n++ )
					pubkey += line;

			} else if ( getparse(line, _T("Key-Derivation"), para) ) {
				keyDeriv = para;
			} else if ( getparse(line, _T("Argon2-Memory"), para) ) {
				argon2Memory = _tstoi(para);
			} else if ( getparse(line, _T("Argon2-Passes"), para) ) {
				argon2Passes = _tstoi(para);
			} else if ( getparse(line, _T("Argon2-Parallelism"), para) ) {
				argon2Parallelism = _tstoi(para);
			} else if ( getparse(line, _T("Argon2-Salt"), para) ) {
				argon2Salt = para;

			} else if ( getparse(line, _T("Private-Lines"), para) ) {
				len = _tstoi(para);
				prikey.Empty();
				for ( n = 0 ; n < len && fgetline(fp, line) != NULL ; n++ )
					prikey += line;

			} else if ( getparse(line, _T("Private-MAC"), para) ) {
				private_mac = para;
			} else if ( getparse(line, _T("Private-Hash"), para) ) {
				private_mac = para;
			}
		}

		if ( type == IDKEY_NONE || encname.IsEmpty() || pubkey.IsEmpty() || prikey.IsEmpty() )
			throw _T("key file format error");

		if ( version == 3 && (keyDeriv.IsEmpty() || argon2Memory == 0 || argon2Passes == 0 || argon2Parallelism == 0 || argon2Salt.IsEmpty()) )
			throw _T("key file version 3 format error");

		pubblob.Base64Decode(pubkey);
		pubsave = pubblob;

		switch(type) {
		case IDKEY_RSA2:
			m_Nid = NID_sha1;
			pubblob.GetStr(str);					// string "ssh-rsa"
			if ( str.Compare("ssh-rsa") != 0 )
				throw _T("key type error");
			{
				BIGNUM *e = pubblob.GetBIGNUM2(NULL);			// mpint  exponen
				BIGNUM *n = pubblob.GetBIGNUM2(NULL);			// mpint  modulus

				Create(type);
				RSA_set0_key(m_Rsa, n, e, NULL);
			}
			break;

		case IDKEY_DSA2:
			m_Nid = NID_undef;
			pubblob.GetStr(str);					// string "ssh-dss"
			if ( str.Compare("ssh-dss") != 0 )
				throw _T("key type error");
			{
				BIGNUM *p = pubblob.GetBIGNUM2(NULL);				// mpint p
				BIGNUM *q = pubblob.GetBIGNUM2(NULL);				// mpint q
				BIGNUM *g = pubblob.GetBIGNUM2(NULL);				// mpint g
				BIGNUM *pub_key = pubblob.GetBIGNUM2(NULL);			// mpint y

				Create(type);
				DSA_set0_pqg(m_Dsa, p, q, g);
				DSA_set0_key(m_Dsa, pub_key, NULL);
			}
			break;

		case IDKEY_ECDSA:
			pubblob.GetStr(str);					// string "ecdsa-sha2-xxx"
			if ( str.Left(11).Compare("ecdsa-sha2-") != 0 )
				throw _T("key type error");
			{
				EC_POINT *q;
				CStringA idxname;

				pubblob.GetStr(idxname);
				if ( (n = GetIndexName(MbsToTstr(idxname))) < 0 )
					throw _T("ec curve name error");

				Create(type);

				m_Nid = NidListTab[n].nid;
				if ( m_EcDsa != NULL )
					EC_KEY_free(m_EcDsa);
				if ( (m_EcDsa = EC_KEY_new_by_curve_name(m_Nid)) == NULL )
					throw _T("ec curve name error");

				if ( (q = EC_POINT_new(EC_KEY_get0_group(m_EcDsa))) == NULL )
					throw _T("ec get0 group error");
				if ( !pubblob.GetEcPoint(EC_KEY_get0_group(m_EcDsa), q) ) {
					EC_POINT_free(q);
					throw _T("ec ec point error");
				}
				if ( EC_METHOD_get_field_type(EC_GROUP_method_of(EC_KEY_get0_group(m_EcDsa))) == NID_X9_62_prime_field && key_ec_validate_public(EC_KEY_get0_group(m_EcDsa), q) != 0 ) {
					EC_POINT_free(q);
					throw _T("ec field type error");
				}
				if ( EC_KEY_set_public_key(m_EcDsa, q) != 1 ) {
					EC_POINT_free(q);
					throw _T("ec set public key error");
				}
				EC_POINT_free(q);
			}
			break;

		case IDKEY_ED25519:
			pubblob.GetStr(str);					// string "ssh-ed25519"
			if ( str.Compare("ssh-ed25519") != 0 )
				throw _T("key type error");

			Create(type);
			m_Nid = NID_ED25519;
			m_PublicKey.Clear();
			pubblob.GetBuf(&m_PublicKey);
			break;

		case IDKEY_ED448:
			pubblob.GetStr(str);					// string "ssh-ed448"
			if ( str.Compare("ssh-ed448") != 0 )
				throw _T("key type error");

			Create(type);
			m_Nid = NID_ED448;
			m_PublicKey.Clear();
			pubblob.GetBuf(&m_PublicKey);
			break;

		default:
			throw _T("key type error");
		}

		FingerPrint(line, SSHFP_DIGEST_SHA256, SSHFP_FORMAT_SIMPLE);
		m_LoadMsg.Format(_T("%s(%d) %32.32s..."), MbsToTstr(str), GetSize(), (LPCTSTR)line);

		pubblob.Move(pubsave);
		encblob.Base64Decode(prikey);

		if ( encname.Compare(_T("none")) != 0 ) {
			int i;
			int keylen, ivlen, maclen = 32;		// max 64 + 32 + 32 = 128
			const EVP_MD *md_mac = NULL;
			EVP_MD_CTX *md_ctx;
			unsigned int len;
			u_char key[128], ctr[4];
			CStringA mbs = TstrToMbs(pass);
			CCipher cip;
			CBuffer tmp, salt, empty;
			BYTE hash[EVP_MAX_MD_SIZE];

			if ( cip.GetIndex(encname) < 0 )
				throw _T("Chipher not suppoertd");

			if ( (keylen = cip.GetKeyLen(encname)) > 64 )
				throw _T("Chipher key len overflow");

			if ( (ivlen = cip.GetIvSize(encname)) > 32 )
				throw _T("Chipher iv len overflow");

			switch(version) {
			case 1: case 2:
				for ( i = 0 ; i < 7 && (i * 20) < keylen ; i++ ) {
					if ( (md_ctx = EVP_MD_CTX_new()) == NULL || EVP_DigestInit(md_ctx, EVP_sha1()) == 0 )
						throw _T("sha1 init error");
					ctr[0] = (u_char)(i >> 24);
					ctr[1] = (u_char)(i >> 16);
					ctr[2] = (u_char)(i >> 8);
					ctr[3] = (u_char)(i);
					if ( EVP_DigestUpdate(md_ctx, ctr, 4) == 0 ||
						 EVP_DigestUpdate(md_ctx, mbs, mbs.GetLength()) == 0 ||
						 EVP_DigestFinal(md_ctx, key + i * 20, &len) == 0 )
						throw _T("sha1 digest error");
					EVP_MD_CTX_free(md_ctx);
				}
				memset(key + keylen, 0, ivlen);
				tmp.Clear();
				tmp.Apend((BYTE *)"putty-private-key-file-mac-key", 30);
				tmp.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
				maclen = MD_digest(EVP_sha1(), tmp.GetPtr(), tmp.GetSize(), key + keylen + ivlen, 128 - keylen - ivlen);
				md_mac = EVP_sha1();
				break;

			case 3:
				if (      keyDeriv.CompareNoCase(_T("Argon2d"))  == 0 ) i = 0;
				else if ( keyDeriv.CompareNoCase(_T("Argon2i"))  == 0 ) i = 1;
				else if ( keyDeriv.CompareNoCase(_T("Argon2id")) == 0 ) i = 2;
				else throw _T("Argon2 Key-Derivation error");

				tmp.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
				salt.Base16Decode(argon2Salt);

				argon2(i, argon2Memory, argon2Passes, argon2Parallelism, &tmp, &salt, &empty, &empty, key, keylen + ivlen + maclen);
				md_mac = EVP_sha256();
				break;
			}

			if ( !cip.Init(encname, MODE_DEC, key, keylen, key + keylen) )
				throw _T("Chipher init error");

			cip.Cipher(encblob.GetPtr(), encblob.GetSize(), &priblob);

			if ( version == 1 ) {
				tmp.Clear();
				tmp.Apend(priblob.GetPtr(), priblob.GetSize());
			} else {
				tmp.Clear();
				tmp.PutStr(TstrToMbs(keyname));
				tmp.PutStr(TstrToMbs(encname));
				tmp.PutStr(TstrToMbs(comment));
				tmp.PutBuf(pubblob.GetPtr(), pubblob.GetSize());
				tmp.PutBuf(priblob.GetPtr(), priblob.GetSize());
			}

			len = HMAC_digest(md_mac, key + keylen + ivlen, maclen, tmp.GetPtr(), tmp.GetSize(), hash, sizeof(hash));
			salt.Base16Decode(private_mac);

			if ( memcmp(hash, salt.GetPtr(), len) != 0 )
				throw _T("Private Mac error");

		} else
			priblob = encblob;

		switch(type) {
		case IDKEY_RSA2:
			{
				BIGNUM *d = priblob.GetBIGNUM2(NULL);			// mpint  private_exponent
				BIGNUM *p = priblob.GetBIGNUM2(NULL);			// mpint  p    (the larger of the two primes)
				BIGNUM *q = priblob.GetBIGNUM2(NULL);			// mpint  q    (the smaller prime)
				BIGNUM *iqmp = priblob.GetBIGNUM2(NULL);		// mpint  iqmp (the inverse of q modulo p)

				RSA_set0_key(m_Rsa, NULL, NULL, d);
				RSA_set0_factors(m_Rsa, p, q);
				RsaGenAddPara(iqmp);
			}
			break;

		case IDKEY_DSA2:
			{
				BIGNUM *priv_key = priblob.GetBIGNUM2(NULL);		// mpint x

				DSA_set0_key(m_Dsa, NULL, priv_key);
			}
			break;

		case IDKEY_ECDSA:
			{
				BIGNUM *p;

				if ( (p = priblob.GetBIGNUM2(NULL)) == NULL )
					throw _T("bn new error");
				if ( EC_KEY_set_private_key(m_EcDsa, p) != 1 ) {
					BN_free(p);
					throw _T("ec set private key error");
				}
				BN_free(p);
			}
			break;

		case IDKEY_ED25519:
		case IDKEY_ED448:
			m_PrivateKey.Clear();
			priblob.GetBuf(&m_PrivateKey);
			break;
		}

		m_Name = comment;

		//fclose(fp);
		return TRUE;
	} catch(...) {
		m_Type = IDKEY_NONE;
		//fclose(fp);
		return FALSE;
	}
}
static void putbase16(FILE *fp, LPCSTR tag, LPCTSTR base)
{
	int n = 0;
	CString wrk;

	fprintf(fp, "%s: %d\n", tag, ((int)_tcslen(base) + 63) / 64);

	while ( *base != _T('\0') ) {
		wrk += *(base++);
		if ( ++n >= 64 ) {
			wrk += _T("\n");
			fputs(TstrToMbs(wrk), fp);
			wrk.Empty();
			n = 0;
		}
	}

	if ( !wrk.IsEmpty() ) {
		fputs(TstrToMbs(wrk), fp);
		fputs("\n", fp);
	}
}
int CIdKey::SavePuttyKey(FILE *fp, LPCTSTR pass, int ver)
{
	unsigned int len;
	int keylen, ivlen, maclen;
	LPCTSTR encname = _T("aes256-cbc");
	CBuffer pubblob(-1), priblob(-1), padblob(-1), encblob(-1);
	CBuffer tmp(-1), salt(-1), empty;
	CCipher cip;
	BYTE key[128];
	BYTE hash[EVP_MAX_MD_SIZE];
	const EVP_MD *md_mac;

	switch(m_Type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA2:
		{
			BIGNUM const *n = NULL, *e = NULL, *d = NULL, *p = NULL, *q = NULL, *iqmp = NULL;

			RSA_get0_key(m_Rsa, &n, &e, &d);
			RSA_get0_factors(m_Rsa, &p, &q);
			RSA_get0_crt_params(m_Rsa, NULL, NULL, &iqmp);

			pubblob.PutStr("ssh-rsa");
			pubblob.PutBIGNUM2(e);
			pubblob.PutBIGNUM2(n);

			priblob.PutBIGNUM2(d);
			priblob.PutBIGNUM2(p);
			priblob.PutBIGNUM2(q);
			priblob.PutBIGNUM2(iqmp);
		}
		break;
	case IDKEY_DSA2:
		{
			BIGNUM const *p = NULL, *q = NULL, *g = NULL;
			BIGNUM const *pub_key = NULL, *priv_key = NULL;

			DSA_get0_pqg(m_Dsa, &p, &q, &g);
			DSA_get0_key(m_Dsa, &pub_key, &priv_key);

			pubblob.PutStr("ssh-dss");
			pubblob.PutBIGNUM2(p);
			pubblob.PutBIGNUM2(q);
			pubblob.PutBIGNUM2(g);
			pubblob.PutBIGNUM2(pub_key);

			priblob.PutBIGNUM2(priv_key);
		}
		break;
	case IDKEY_ECDSA:
		pubblob.PutStr(TstrToMbs(GetName()));
		pubblob.PutStr(TstrToMbs(GetEcNameFromNid(m_Nid)));
		pubblob.PutEcPoint(EC_KEY_get0_group(m_EcDsa), EC_KEY_get0_public_key(m_EcDsa));

		priblob.PutBIGNUM2(EC_KEY_get0_private_key(m_EcDsa));
		break;
	case IDKEY_ED25519:
		pubblob.PutStr("ssh-ed25519");
		pubblob.PutBuf(m_PublicKey.GetPtr(), m_PublicKey.GetSize());

		priblob.PutBuf(m_PrivateKey.GetPtr(), m_PrivateKey.GetSize());
		break;
	case IDKEY_ED448:
		pubblob.PutStr("ssh-ed448");
		pubblob.PutBuf(m_PublicKey.GetPtr(), m_PublicKey.GetSize());

		priblob.PutBuf(m_PrivateKey.GetPtr(), m_PrivateKey.GetSize());
		break;
	default:
		return FALSE;
	}

	if ( cip.GetIndex(encname) < 0 )
		return FALSE;

	keylen = cip.GetKeyLen(encname);
	ivlen = cip.GetIvSize(encname);

	if ( ver == 3 ) {
		tmp = TstrToMbs(pass);
		rand_buf(salt.PutSpc(16), 16);
		md_mac = EVP_sha256();
		maclen = EVP_MD_size(md_mac);

		ASSERT((keylen + ivlen + maclen) <= 128);

		// Argon2id=2, Argon2-Memory=8192, Argon2-Passes=21, Argon2-Parallelism=1
		argon2(2, 8192, 21, 1, &tmp, &salt, &empty, &empty, key, keylen + ivlen + maclen);

	} else {
		int i;
		EVP_MD_CTX *md_ctx;
		u_char ctr[4];
		CStringA mbs = TstrToMbs(pass);

		for ( i = 0 ; i < 7 && (i * 20) < keylen ; i++ ) {
			if ( (md_ctx = EVP_MD_CTX_new()) == NULL || EVP_DigestInit(md_ctx, EVP_sha1()) == 0 )
				throw _T("sha1 init error");
			ctr[0] = (u_char)(i >> 24);
			ctr[1] = (u_char)(i >> 16);
			ctr[2] = (u_char)(i >> 8);
			ctr[3] = (u_char)(i);
			if ( EVP_DigestUpdate(md_ctx, ctr, 4) == 0 ||
					EVP_DigestUpdate(md_ctx, mbs, mbs.GetLength()) == 0 ||
					EVP_DigestFinal(md_ctx, key + i * 20, &len) == 0 )
				throw _T("sha1 digest error");
			EVP_MD_CTX_free(md_ctx);
		}

		memset(key + keylen, 0, ivlen);

		tmp.Clear();
		tmp.Apend((BYTE *)"putty-private-key-file-mac-key", 30);
		tmp.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
		md_mac = EVP_sha1();
		maclen = EVP_MD_size(md_mac);

		ASSERT((keylen + ivlen + maclen) <= 128);

		maclen = MD_digest(md_mac, tmp.GetPtr(), tmp.GetSize(), key + keylen + ivlen, maclen);
	}

	if ( !cip.Init(encname, MODE_ENC, key, keylen, key + keylen) )
		return FALSE;

	padblob.PutSpc(((priblob.GetSize() + 15) / 16) * 16);
	memcpy(padblob.GetPtr(), priblob.GetPtr(), priblob.GetSize());
	rand_buf(padblob.GetPtr() + priblob.GetSize(), padblob.GetSize() - priblob.GetSize());
	cip.Cipher(padblob.GetPtr(), padblob.GetSize(), &encblob);

	fprintf(fp, "PuTTY-User-Key-File-%d: %s\n", ver, TstrToMbs(GetName()));
	fputs("Encryption: aes256-cbc\n", fp);
	fprintf(fp, "Comment: %s\n", TstrToMbs(m_Name));

	tmp.Base64Encode(pubblob.GetPtr(), pubblob.GetSize());
	putbase16(fp, "Public-Lines", (LPCTSTR)tmp);

	if ( ver == 3 ) {
		fputs("Key-Derivation: Argon2id\n", fp);
		fputs("Argon2-Memory: 8192\n", fp);
		fputs("Argon2-Passes: 21\n", fp);
		fputs("Argon2-Parallelism: 1\n", fp);

		tmp.Base16Encode(salt.GetPtr(), salt.GetSize());
		fprintf(fp, "Argon2-Salt: %s\n", TstrToMbs((LPCTSTR)tmp));
	}

	tmp.Base64Encode(encblob.GetPtr(), encblob.GetSize());
	putbase16(fp, "Private-Lines", (LPCTSTR)tmp);

	if ( ver == 1 ) {
		tmp.Clear();
		tmp.Apend(pubblob.GetPtr(), pubblob.GetSize());
	} else {
		tmp.Clear();
		tmp.PutStr(TstrToMbs(GetName()));
		tmp.PutStr(TstrToMbs(encname));
		tmp.PutStr(TstrToMbs(m_Name));
		tmp.PutBuf(pubblob.GetPtr(), pubblob.GetSize());
		tmp.PutBuf(padblob.GetPtr(), padblob.GetSize());
	}

	len = HMAC_digest(md_mac, key + keylen + ivlen, maclen, tmp.GetPtr(), tmp.GetSize(), hash, sizeof(hash));
	tmp.Base16Encode(hash, len);
	fprintf(fp, "Private-%s: %s\n", (ver == 1 ? "Hash" : "MAC"), TstrToMbs((LPCTSTR)tmp));

	return TRUE;
}
EVP_PKEY *CIdKey::GetEvpPkey(int mode)
{
	EVP_PKEY *pkey = NULL;

	switch(m_Type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA2:
		if ( (pkey = EVP_PKEY_new()) == NULL || EVP_PKEY_set1_RSA(pkey, m_Rsa) == 0 )
			goto ERRENDOF;
		break;

	case IDKEY_DSA2:
		if ( (pkey = EVP_PKEY_new()) == NULL || EVP_PKEY_set1_DSA(pkey, m_Dsa) == 0 )
			goto ERRENDOF;
		break;

	case IDKEY_ECDSA:
		if ( (pkey = EVP_PKEY_new()) == NULL || EVP_PKEY_set1_EC_KEY(pkey, m_EcDsa) == 0 )
			goto ERRENDOF;
		break;

	case IDKEY_ED25519:
	case IDKEY_ED448:
	case IDKEY_ML_DSA:
	case IDKEY_SLH_DSA:
		switch(mode) {
		case GETPKEY_PUBLICKEY:
			pkey = EVP_PKEY_new_raw_public_key(m_Nid, NULL, m_PublicKey.GetPtr(), m_PublicKey.GetSize());
			break;
		case GETPKEY_PRIVATEKEY:
		case GETPKEY_KEYPAIR:
			pkey = EVP_PKEY_new_raw_private_key(m_Nid, NULL, m_PrivateKey.GetPtr(), m_PrivateKey.GetSize());
			break;
		}
		break;
	}

	return pkey;

ERRENDOF:
	if ( pkey != NULL )
		EVP_PKEY_free(pkey);

	return NULL;
}
int CIdKey::SetEvpPkey(EVP_PKEY *pk)
{
	int nid = EVP_PKEY_type(EVP_PKEY_get_id(pk));

	// 本来なら必要ないはず・・・
	const char *name;
	if ( nid == NID_undef && (name = EVP_PKEY_get0_type_name(pk)) != NULL )
		nid = OBJ_ln2nid(name);

	switch(nid) {
	case EVP_PKEY_RSA:
	case EVP_PKEY_RSA2:
		if ( m_Rsa != NULL )
			RSA_free(m_Rsa);
		m_Rsa = EVP_PKEY_get1_RSA(pk);
		m_Type = IDKEY_RSA2;
		m_Nid = NID_sha1;
		break;

	case EVP_PKEY_DSA:
	case EVP_PKEY_DSA1:
	case EVP_PKEY_DSA2:
	case EVP_PKEY_DSA3:
	case EVP_PKEY_DSA4:
		if ( m_Dsa != NULL )
			DSA_free(m_Dsa);
		m_Dsa = EVP_PKEY_get1_DSA(pk);
		m_Type = IDKEY_DSA2;
		m_Nid = NID_undef;
		break;

	case EVP_PKEY_EC:
		if ( m_EcDsa != NULL )
			EC_KEY_free(m_EcDsa);
		m_EcDsa = EVP_PKEY_get1_EC_KEY(pk);
		m_Nid = GetEcNidFromKey(m_EcDsa);
		if ( GetIndexNid(m_Nid) >= 0 )
			m_Type = IDKEY_ECDSA;
		break;

	case EVP_PKEY_ED25519:
		if ( !Create(IDKEY_ED25519) )
			return FALSE;
		m_Nid = nid;
		if ( !EdFromPkeyRaw(pk) )
			return FALSE;
		break;

	case EVP_PKEY_ED448:
		if ( !Create(IDKEY_ED448) )
			return FALSE;
		m_Nid = nid;
		if ( !EdFromPkeyRaw(pk) )
			return FALSE;
		break;

	case EVP_PKEY_ML_DSA_44:
	case EVP_PKEY_ML_DSA_65:
	case EVP_PKEY_ML_DSA_87:
		if ( !Create(IDKEY_ML_DSA) )
			return FALSE;
		m_Nid = nid;
		if ( !EdFromPkeyRaw(pk) )
			return FALSE;
		break;

	case EVP_PKEY_SLH_DSA_SHA2_128S:
	case EVP_PKEY_SLH_DSA_SHA2_128F:
	case EVP_PKEY_SLH_DSA_SHA2_192S:
	case EVP_PKEY_SLH_DSA_SHA2_192F:
	case EVP_PKEY_SLH_DSA_SHA2_256S:
	case EVP_PKEY_SLH_DSA_SHA2_256F:
	case EVP_PKEY_SLH_DSA_SHAKE_128S:
	case EVP_PKEY_SLH_DSA_SHAKE_128F:
	case EVP_PKEY_SLH_DSA_SHAKE_192S:
	case EVP_PKEY_SLH_DSA_SHAKE_192F:
	case EVP_PKEY_SLH_DSA_SHAKE_256S:
	case EVP_PKEY_SLH_DSA_SHAKE_256F:
		if ( !Create(IDKEY_SLH_DSA) )
			return FALSE;
		m_Nid = nid;
		if ( !EdFromPkeyRaw(pk) )
			return FALSE;
		break;

	default:
		m_Type = IDKEY_NONE;
		return FALSE;
	}

	return TRUE;
}
int	CIdKey::LoadPrivateKey(LPCTSTR file, LPCTSTR pass)
{
	FILE *fp;
	EVP_PKEY *pk;
	CString cert;
	CWaitCursor wait;

	m_LoadMsg.Empty();

	if ( (fp = _tfopen(file, _T("r"))) == NULL )
		return FALSE;

	if ( (pk = PEM_read_PrivateKey(fp, NULL, NULL, (void *)(LPCSTR)(*pass == _T('\0') ? NULL : TstrToMbs(pass)))) != NULL ) {
		if ( SetEvpPkey(pk) ) {
			EVP_PKEY_free(pk);
			goto ENDOF;
		}
		EVP_PKEY_free(pk);
	}

	if ( LoadSecShKey(fp, pass) )
		goto ENDOF;

	if ( LoadPuttyKey(fp, pass) )
		goto ENDOF;

	if ( LoadRsa1Key(fp, pass) )
		goto ENDOF;

	if ( LoadOpenSshKey(fp, pass) )
		goto ENDOF;

	fclose(fp);
	return FALSE;

ENDOF:
	fclose(fp);

	if ( !WritePrivateKey(m_SecBlob, pass) )
		return FALSE;

	m_FilePath = file;

	cert.Format(_T("%s-cert.pub"), file);
	if ( LoadCertPublicKey(cert) )
		return TRUE;

#ifdef	USE_X509
	cert.Format(_T("%s.pem"), file);
	LoadCertPublicKey(cert);
#endif

	return TRUE;
}

int CIdKey::SaveFileFormat(int fmt)
{
	switch(m_Type & IDKEY_TYPE_MASK) {
	case IDKEY_RSA1:
		fmt = EXPORT_STYLE_OLDRSA;
		break;
	case IDKEY_ED25519:
	case IDKEY_ED448:
		if ( fmt == EXPORT_STYLE_OSSLPEM )
			fmt = EXPORT_STYLE_OSSLPFX;
		break;
	case IDKEY_ML_DSA:
	case IDKEY_SLH_DSA:
		//if ( fmt != EXPORT_STYLE_OSSLPFX && fmt != EXPORT_STYLE_OPENSSH )
			fmt = EXPORT_STYLE_OSSLPFX;
		break;
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_XMSS:
		fmt = EXPORT_STYLE_OPENSSH;
		break;
	}

	return fmt;
}
int CIdKey::SavePrivateKey(int fmt, LPCTSTR file, LPCTSTR pass)
{
	FILE *fp;
	int rt = FALSE;
	CStringA mbs = TstrToMbs(pass);
	const EVP_CIPHER *cipher = EVP_aes_128_cbc();
	EVP_PKEY *pkey = NULL;
	CWaitCursor wait;

	if ( !Init(pass) )
		return FALSE;

	if ( mbs.IsEmpty() ) {
		if ( ::AfxMessageBox(CStringLoad(IDM_NONENCRYPTFILE), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return FALSE;
	}

	if ( (fp = _tfopen(file, _T("wb"))) == NULL )
		return FALSE;

	fmt = SaveFileFormat(fmt);

	switch(fmt) {
	case EXPORT_STYLE_OSSLPEM:
		switch(m_Type & IDKEY_TYPE_MASK) {
		case IDKEY_RSA2:
			rt = PEM_write_RSAPrivateKey(fp, m_Rsa, (mbs.IsEmpty() ? NULL : cipher), (unsigned char *)(mbs.IsEmpty() ? NULL : (LPCSTR)mbs), mbs.GetLength(), NULL, NULL);
			break;
		case IDKEY_DSA2:
			rt = PEM_write_DSAPrivateKey(fp, m_Dsa, (mbs.IsEmpty() ? NULL : cipher), (unsigned char *)(mbs.IsEmpty() ? NULL : (LPCSTR)mbs), mbs.GetLength(), NULL, NULL);
			break;
		case IDKEY_ECDSA:
			rt = PEM_write_ECPrivateKey(fp, m_EcDsa, (mbs.IsEmpty() ? NULL : cipher), (unsigned char *)(mbs.IsEmpty() ? NULL : (LPCSTR)mbs), mbs.GetLength(), NULL, NULL);
			break;
		}
		break;
	case EXPORT_STYLE_OSSLPFX:
		if ( (pkey = GetEvpPkey(GETPKEY_KEYPAIR)) != NULL ) {
			rt = PEM_write_PrivateKey(fp, pkey, (mbs.IsEmpty() ? NULL : cipher), (unsigned char *)(mbs.IsEmpty() ? NULL : (LPCSTR)mbs), mbs.GetLength(), NULL, NULL);
			EVP_PKEY_free(pkey);
		}
		break;
	case EXPORT_STYLE_OPENSSH:
		rt = SaveOpenSshKey(fp, pass);
		break;
	case EXPORT_STYLE_PUTTY2:
		rt = SavePuttyKey(fp, pass, 2);
		break;
	case EXPORT_STYLE_PUTTY3:
		rt = SavePuttyKey(fp, pass, 3);
		break;
	case EXPORT_STYLE_OLDRSA:
		rt = SaveRsa1Key(fp, pass);
		break;
	}
	
	fclose(fp);

	if ( rt && m_Cert )
		SaveCertPublicKey(file);

	return rt;
}
int CIdKey::SavePublicKey(LPCTSTR file)
{
	// RFC 4716  
	//---- BEGIN SSH2 PUBLIC KEY ----
	//Comment: "1024-bit RSA, converted from OpenSSH by me@example.com"
	//AAAAB3NzaC1yc2EAAAABIwAAAIEA1on8gxCGJJWSRT4uOrR13mUaUk0hRf4RzxSZ1zRb
	//YYFw8pfGesIFoEuVth4HKyF8k1y4mRUnYHP1XNMNMJl1JcEArC2asV8sHf6zSPVffozZ
	//5TT4SfsUu/iKy9lUcCfXzwre4WWZSXXcPff+EHtWshahu3WzBdnGxm5Xoi89zcE=
	//---- END SSH2 PUBLIC KEY ----

	FILE *fp;
	CBuffer body;
	CBuffer tmp;
	CStringA text;

	if ( !SetBlob(&body, FALSE) )
		return FALSE;

	if ( (fp = _tfopen(file, _T("wb"))) == NULL )
		return FALSE;

	tmp.Base64Encode(body.GetPtr(), body.GetSize());
	tmp.WstrConvert(NULL, 2);

	fputs("---- BEGIN SSH2 PUBLIC KEY ----\n", fp);

	text.Format("Comment: \"%d-bit %s, converted from RLogin by %s\"\n", GetSize(), TstrToMbs(GetName()), TstrToMbs(m_Name));
	fputs(text, fp);

	text.Empty();
	for ( LPCSTR p = (LPCSTR)tmp ; *p != '\0' ; ) {
		text += *(p++);
		if ( text.GetLength() > 70 || *p == '\0' ) {
			text += "\n";
			fputs(text, fp);
			text.Empty();
		}
	}

	fputs("---- END SSH2 PUBLIC KEY ----\n", fp);

	fclose(fp);

	return TRUE;
}
int CIdKey::ParseCertPublicKey(LPCTSTR str)
{
	int type, nid;
	CString name;
	CBuffer tmp(-1), work(-1);
	CIdKey ckey;

	try {
		while ( *str == _T(' ') || *str == _T('\t') )
			str++;
		while ( *str != _T('\0') && *str != _T(' ') && *str != _T('\t') )
			name += *(str++);
		while ( *str == _T(' ') || *str == _T('\t') )
			str++;

		if ( *str == _T('\0') )
			return FALSE;

		tmp.Base64Decode(str);

		if ( tmp.GetSize() == 0 )
			return FALSE;

		if ( !GetTypeFromName(name, type, nid) )
			return FALSE;

		if ( m_Type == IDKEY_RSA1 || m_Type != (type & IDKEY_TYPE_MASK) )
			return FALSE;

		m_CertBlob = tmp;

		if ( !ckey.GetBlob(&tmp) || ckey.m_Cert != (type & (IDKEY_CERTV00 | IDKEY_CERTV01)) )
			return FALSE;

		if ( ComperePublic(&ckey) != 0 )
			return FALSE;

		m_Cert = type & (IDKEY_CERTV00 | IDKEY_CERTV01 | IDKEY_CERTX509);

		return TRUE;

	} catch(...) {
		return FALSE;
	}
}
int CIdKey::LoadCertPublicKey(LPCTSTR file)
{
	int rt = FALSE;
	FILE *fp;
	char buf[4096];
	CString str;

	if ( (fp = _tfopen(file, _T("r"))) == NULL )
		return FALSE;

#ifdef	USE_X509
	// X509_get_version(pX509) == (3 - 1) ?

	// RFC 6187
    // string  "x509v3-ssh-dss" / "x509v3-ssh-rsa" / "x509v3-rsa2048-sha256" / "x509v3-ecdsa-sha2-[identifier]"
    // uint32  certificate-count
    // string  certificate[1..certificate-count]
    // uint32  ocsp-response-count
    // string  ocsp-response[0..ocsp-response-count]

	X509 *pX509;
	if ( (pX509 = PEM_read_X509(fp, NULL, NULL, NULL)) != NULL ) {
		EVP_PKEY *pk;
		if ( (pk = X509_get_pubkey(pX509)) != NULL ) {
			CIdKey key;
			if ( (rt = key.SetEvpPkey(pk)) != FALSE ) {
				if ( ComperePublic(&key) != 0 )
					rt = FALSE;
			}
			EVP_PKEY_free(pk);
		}
		if ( rt ) {
			CBuffer tmp(-1);
			int len = i2d_X509(pX509, NULL);
			u_char *p = tmp.PutSpc(len);
			i2d_X509(pX509, &p);

			m_CertBlob.Clear();
			str.Format(_T("x509v3-%s"), GetName());
			m_CertBlob.PutStr(TstrToMbs(str));
			m_CertBlob.Put32Bit(1);
			m_CertBlob.PutBuf(tmp.GetPtr(), tmp.GetSize());
			m_CertBlob.Put32Bit(0);

			m_Cert = IDKEY_CERTX509;
		}
		X509_free(pX509);
		fclose(fp);
		return rt;
	} else
		fseek(fp, 0L, SEEK_SET);
#endif

	if ( fgets(buf, 4096, fp) != NULL ) {
		str = buf;
		str.Trim(_T(" \t\r\n"));
		rt = ParseCertPublicKey(str);
	}

	fclose(fp);
	return rt;
}
int CIdKey::SaveCertPublicKey(LPCTSTR file)
{
	FILE *fp;
	CString str;

	str.Format(_T("%s-cert.pub"), file);
	if ( (fp = _tfopen(str, _T("wb"))) == NULL )
		return FALSE;

	WritePublicKey(str);
	fprintf(fp, "%s\n\r", TstrToMbs(str));

	fclose(fp);
	return TRUE;
}
int CIdKey::GetSize()
{
	int n;

	switch (m_Type) {
	case IDKEY_RSA1:
	case IDKEY_RSA2:
		return RSA_bits(m_Rsa);
	case IDKEY_DSA2:
		return DSA_bits(m_Dsa);
	case IDKEY_ECDSA:
		if ( (n = GetIndexNid(m_Nid)) < 0 )
			return 0;
		return NidListTab[n].bits;
	case IDKEY_ED25519:
		return 255;
	case IDKEY_ED448:
		return 448;
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
	case IDKEY_SLH_DSA:
		return GetDsaSize(m_Nid);
	case IDKEY_XMSS:
		return m_XmssKey.GetHeight();
	}
	return 0;
}
int CIdKey::GetHeight()
{
	if ( m_Type == IDKEY_XMSS )
		return m_XmssKey.GetHeight();
	else
		return GetSize();
}
BOOL CIdKey::IsNotSupport()
{ 
	if ( m_Type != IDKEY_DSA2 || m_Dsa == NULL )
		return FALSE;

	BIGNUM const *q = NULL;

	DSA_get0_pqg(m_Dsa, NULL, &q, NULL);

	if ( q == NULL || BN_num_bits(q) > 160 )
		return TRUE;
	
	return FALSE;
}

void CIdKey::FingerPrint(CString &str, int digest, int format)
{
#define	FLDSIZE_X		17
#define	FLDSIZE_Y		9

	int n, b, c;
	int x, y;
	const EVP_MD *md = NULL;
	LPCTSTR p, digname;
	CBuffer blob, base;
	CString work;
	int len;
	u_char tmp[EVP_MAX_MD_SIZE];
	u_char map[FLDSIZE_X][FLDSIZE_Y];
	static const char *augmentation_string = " .o+=*BOX@%&#/^SE";
	CStringA mbs;

	str.Empty();

	if ( format == SSHFP_FORMAT_BUBBLEBABBLE )
		digest = SSHFP_DIGEST_SHA1;
	if ( digest == SSHFP_DIGEST_MD5 )
		format = SSHFP_FORMAT_HEX;

	switch(digest) {
	case SSHFP_DIGEST_MD5:		 md = EVP_md5();		digname = _T("MD5");		break;
	case SSHFP_DIGEST_RIPEMD160: md = EVP_ripemd160();	digname = _T("RIPEMD160");	break;
	case SSHFP_DIGEST_SHA1:		 md = EVP_sha1();		digname = _T("SHA1");		break;
	case SSHFP_DIGEST_SHA256:	 md = EVP_sha256();		digname = _T("SHA256");		break;
	case SSHFP_DIGEST_SHA384:	 md = EVP_sha384();		digname = _T("SHA384");		break;
	case SSHFP_DIGEST_SHA512:	 md = EVP_sha512();		digname = _T("SHA512");		break;
	}

	if ( !SetBlob(&blob, FALSE) ) {
		if ( m_Type != IDKEY_RSA1 )
			return;

		BIGNUM const *n = NULL, *e = NULL;

		RSA_get0_key(m_Rsa, &n, &e, NULL);

		blob.PutBIGNUM2(n);
		blob.PutBIGNUM2(e);
	}

	len = MD_digest(md, blob.GetPtr(), blob.GetSize(), tmp, sizeof(tmp));

	if ( format == SSHFP_FORMAT_SIMPLE ) {
		base.Base64Encode(tmp, len);
		str = (LPCTSTR)base;
		return;
	}

	str.Format(_T("%d %s:\r\n"), GetSize(), digname);

	switch(format) {
	case SSHFP_FORMAT_HEX:
		// :00:00:00:00:00:00:00:00
		// :00:00:00:00:00:00:00:00
		for ( n = 0 ; n < len ; n++ ) {
			if ( n > 0 && (n % 8) == 0 )
				str += _T("\r\n");
			work.Format(_T(":%02x"), tmp[n]);
			str += work;
		}
		str += _T("\r\n");
		break;
	case SSHFP_FORMAT_BASE64:
		// xxxxxxxxxxxxxxxxxxxxxx
		// xxxxxxxxxxxxxxxxxxxxx=
		base.Base64Encode(tmp, len);
		p = (LPCTSTR)base;
		for ( n = 0 ; *p != 0 ; n++) {
			if ( n > 0 && (n % 22) == 0 )
				str += _T("\r\n");
			str += *(p++);
		}
		str += _T("\r\n");
		break;
	case SSHFP_FORMAT_BUBBLEBABBLE:
		// xaaaa-aaaaa-aaaaa-aaaaa-
		// aaaaa-aaaaa-aaaax
		base.BubbleBabble(tmp, len);
		p = (LPCTSTR)base;
		for ( n = 0 ; *p != 0 ; n++) {
			if ( n > 0 && (n % 24) == 0 )
				str += _T("\r\n");
			str += *(p++);
		}
		str += _T("\r\n");
		break;
	case SSHFP_FORMAT_RANDOMART:
		break;
	}

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
				y = FLDSIZE_Y - 1;

			map[x][y]++;
			c >>= 2;
		}
	}

	n = (int)strlen(augmentation_string) - 1;
	map[FLDSIZE_X / 2][FLDSIZE_Y / 2] = n - 1;
	map[x][y] = n;

	switch (m_Type) {
	case IDKEY_RSA1:
		work.Format(_T("+---[RSA1 %4d]---+\r\n"), GetSize());
		break;
	case IDKEY_RSA2:
		work.Format(_T("+---[RSA2 %4d]---+\r\n"), GetSize());
		break;
	case IDKEY_DSA2:
		work.Format(_T("+---[DSA2 %4d]---+\r\n"), GetSize());
		break;
	case IDKEY_ECDSA:
		work.Format(_T("+---[ECDS %4d]---+\r\n"), GetSize());
		break;
	case IDKEY_ED25519:
		work = _T("+---[ED25519  ]---+\r\n");
		break;
	case IDKEY_ED448:
		work = _T("+---[ED448    ]---+\r\n");
		break;
	case IDKEY_ML_DSA:
	case IDKEY_ML_DSA_ES:
	case IDKEY_ML_DSA_ED:
		work.Format(_T("+--[ML-DSA %4d]--+\r\n"), GetSize());
		break;
	case IDKEY_SLH_DSA:
		work.Format(_T("+--[SLH-DSA %4d]-+\r\n"), GetSize());
		break;
	default:
		work = _T("+---[Unknown  ]---+\r\n");
	}
	str += work;

	mbs.Empty();
	for ( y = 0 ; y < FLDSIZE_Y ; y++ ) {
		mbs += "|";
		for ( x = 0 ; x < FLDSIZE_X ; x++ )
			mbs += (map[x][y] >= n ? 'E' : augmentation_string[map[x][y]]);
		mbs += "|\r\n";
	}
	str += MbsToTstr(mbs);

	switch(digest) {
	case SSHFP_DIGEST_MD5:		 str += _T("+------[MD5]------+\r\n"); break;
	case SSHFP_DIGEST_RIPEMD160: str += _T("+---[RIPEMD160]---+\r\n"); break;
	case SSHFP_DIGEST_SHA1:		 str += _T("+------[SHA1]-----+\r\n"); break;
	case SSHFP_DIGEST_SHA256:	 str += _T("+-----[SHA256]----+\r\n"); break;
	case SSHFP_DIGEST_SHA384:	 str += _T("+-----[SHA384]----+\r\n"); break;
	case SSHFP_DIGEST_SHA512:	 str += _T("+-----[SHA512]----+\r\n"); break;
	}

	if ( (m_Cert & IDKEY_CERTV00) != 0 )
		str += _T("with cert-v00\r\n");
	else if ( (m_Cert & IDKEY_CERTV01) != 0 )
		str += _T("with cert-v01\r\n");
	else if ( (m_Cert & IDKEY_CERTX509) != 0 )
		str += _T("with x509v3\r\n");
}
int CIdKey::DnsDigest(int hash, CBuffer &digest)
{
	const EVP_MD *md;
	int len;
	u_char tmp[EVP_MAX_MD_SIZE];
	CBuffer blob;

	switch(hash) {
	case SSHFP_HASH_SHA1:
		md = EVP_sha1();
		break;
	case SSHFP_HASH_SHA256:
		md = EVP_sha256();
		break;
	default:
		return FALSE;
	}

	if ( !SetBlob(&blob, FALSE) ) {
		if ( m_Type != IDKEY_RSA1 )
			return FALSE;

		BIGNUM const *n = NULL, *e = NULL;

		RSA_get0_key(m_Rsa, &n, &e, NULL);

		blob.PutBIGNUM2(n);
		blob.PutBIGNUM2(e);
	}

	len = MD_digest(md, blob.GetPtr(), blob.GetSize(), tmp, sizeof(tmp));

	digest.Clear();
	digest.Apend(tmp, len);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CIdKeyTab

CIdKeyTab::CIdKeyTab()
{
	m_pSection = _T("IdKeyTab");
	Serialize(FALSE);
}
CIdKeyTab::~CIdKeyTab()
{
}
void CIdKeyTab::Init()
{
	m_Data.RemoveAll();
}
void CIdKeyTab::SetArray(CStringArrayExt &stra)
{
	CString str;
	stra.RemoveAll();
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_Data[n].SetString(str);
		stra.Add(str);
	}
}
void CIdKeyTab::GetArray(CStringArrayExt &stra)
{
	CIdKey tmp;
	m_Data.RemoveAll();
	for ( int n = 0 ; n < stra.GetSize() ; n++ ) {
		if ( tmp.GetString(stra[n]) )
			m_Data.Add(tmp);
	}
}
void CIdKeyTab::Serialize(int mode)
{
	int n;

	if ( mode ) {		// Write
		for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
			if ( m_Data[n].m_Uid >= 0 )
				m_Data[n].SetProfile(m_pSection);
		}
	} else {			// Read
		CIdKey tmp;
		CStringArrayExt entry;
		CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
		pApp->GetProfileKeys(m_pSection, entry);
		m_Data.RemoveAll();
		for ( n = 0 ; n < entry.GetSize() ; n++ ) {
			if ( !IsListEntry(entry[n]) )
				continue;
			if ( tmp.GetString(pApp->GetProfileString(m_pSection, entry[n], _T(""))) )
				m_Data.Add(tmp);
			else
				pApp->DelProfileEntry(m_pSection, entry[n]);
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
int CIdKeyTab::AddEntry(CIdKey &key, BOOL bDups)
{
	if ( bDups ) {
		for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
			if ( m_Data[n].Compere(&key) == 0 )
				return FALSE;
		}
	}

	key.m_Uid = ((CRLoginApp *)AfxGetApp())->GetProfileSeqNum(m_pSection, _T("ListMax"));
	m_Data.Add(key);
	key.SetProfile(m_pSection);

	return TRUE;
}
int CIdKeyTab::Add(CIdKey &key)
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].Compere(&key) == 0 )
			return FALSE;
	}
	m_Data.Add(key);
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
CIdKey *CIdKeyTab::ReloadUid(int uid)
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].m_Uid == uid ) {
			m_Data[n].GetProfile(m_pSection, uid);
			return &(m_Data[n]);
		}
	}
	return NULL;
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
	m_Type = PFD_REMOTE;
	m_TimeOut = 0;
	m_bClose = FALSE;
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
	m_Type  = data.m_Type;
	m_TimeOut = data.m_TimeOut;
	return *this;
}
