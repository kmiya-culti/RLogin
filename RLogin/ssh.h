// ssh.h: Cssh クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SSH_H__2A682FAC_4F24_4168_9082_C9CDF2DD19D7__INCLUDED_)
#define AFX_SSH_H__2A682FAC_4F24_4168_9082_C9CDF2DD19D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
#include "Data.h"
#include "SFtp.h"
#include "ProgDlg.h"

#include "zlib.h"

#include "openssl/des.h"
#include "openssl/bn.h"
#include "openssl/evp.h"
#include "openssl/err.h"
#include "openssl/hmac.h"
#include "openssl/md5.h"
#include "openssl/pem.h"

#define SSH_CIPHER_NONE         0       // none
#define SSH_CIPHER_DES          2       // des
#define SSH_CIPHER_3DES         3       // 3des
#define SSH_CIPHER_BLOWFISH     6		// blowfish

#define SSH2_CIPHER_3DES_CBC	10		// 3des-cbc
#define SSH2_CIPHER_3DES_ECB	11		// 3des-ecb
#define SSH2_CIPHER_3DES_CFB	12		// 3des-cfb
#define SSH2_CIPHER_3DES_OFB	13		// 3des-ofb
#define SSH2_CIPHER_3DES_CTR	14		// 3des-ctr
#define	SSH2_CIPHER_DES_CBC		15		// des-cbc

#define SSH2_CIPHER_BLF_CBC		20		// blowfish-cbc
#define SSH2_CIPHER_BLF_ECB		21		// blowfish-ecb
#define SSH2_CIPHER_BLF_CFB		22		// blowfish-cfb
#define SSH2_CIPHER_BLF_OFB		23		// blowfish-ofb
#define SSH2_CIPHER_BLF_CTR		24		// blowfish-ctr

#define	SSH2_CIPHER_IDEA_CBC	30		// idea-cbc
#define	SSH2_CIPHER_IDEA_ECB	31		// idea-ecb
#define	SSH2_CIPHER_IDEA_CFB	32		// idea-cfb
#define	SSH2_CIPHER_IDEA_OFB	33		// idea-ofb
#define	SSH2_CIPHER_IDEA_CTR	34		// idea-ctr

#define SSH2_CIPHER_CAST_CBC	40		// cast128-cbc
#define	SSH2_CIPHER_CAST_ECB	41		// cast128-ecb
#define	SSH2_CIPHER_CAST_CFB	42		// cast128-cfb
#define	SSH2_CIPHER_CAST_OFB	43		// cast128-ofb
#define	SSH2_CIPHER_CAST_CTR	44		// cast128-ctr

#define	SSH2_CIPHER_SEED_CBC	50		// seed-cbc
#define	SSH2_CIPHER_SEED_ECB	51		// seed-ecb
#define	SSH2_CIPHER_SEED_CFB	52		// seed-cfb
#define	SSH2_CIPHER_SEED_OFB	55		// seed-ofb
#define	SSH2_CIPHER_SEED_CTR	56		// seed-ctr

#define SSH2_CIPHER_ARCFOUR		60		// arcfour
#define SSH2_CIPHER_ARC128		61		// arcfour128
#define SSH2_CIPHER_ARC192		62		// arcfour192
#define SSH2_CIPHER_ARC256		63		// arcfour256

#define SSH2_CIPHER_TWF128		70		// twofish128-cbc
#define SSH2_CIPHER_TWF192		71		// twofish192-cbc
#define SSH2_CIPHER_TWF256		72		// twofish256-cbc
#define SSH2_CIPHER_TWF128R		75		// twofish128-ctr
#define SSH2_CIPHER_TWF192R		76		// twofish192-ctr
#define SSH2_CIPHER_TWF256R		77		// twofish256-ctr

#define SSH2_CIPHER_AES128		80		// aes128-cbc
#define SSH2_CIPHER_AES192		81		// aes192-cbc
#define SSH2_CIPHER_AES256		82		// aes256-cbc
#define SSH2_CIPHER_AES128R		85		// aes128-ctr
#define SSH2_CIPHER_AES192R		86		// aes192-ctr
#define SSH2_CIPHER_AES256R		87		// aes256-ctr

#define SSH2_CIPHER_CAM128		90		// camellia128-cbc
#define SSH2_CIPHER_CAM192		91		// camellia192-cbc
#define SSH2_CIPHER_CAM256		92		// camellia256-cbc
#define SSH2_CIPHER_CAM128R		95		// camellia128-ctr
#define SSH2_CIPHER_CAM192R		96		// camellia192-ctr
#define SSH2_CIPHER_CAM256R		97		// camellia256-ctr

#define SSH2_CIPHER_SEP128		100		// serpent128-cbc 
#define SSH2_CIPHER_SEP192		101		// serpent191-cbc 
#define SSH2_CIPHER_SEP256		102		// serpent256-cbc 
#define SSH2_CIPHER_SEP128R		105		// serpent128-ctr 
#define SSH2_CIPHER_SEP192R		106		// serpent191-ctr 
#define SSH2_CIPHER_SEP256R		107		// serpent256-ctr 

#define SSH2_CIPHER_CLE128		110		// clefia128-cbc 
#define SSH2_CIPHER_CLE192		111		// clefia191-cbc 
#define SSH2_CIPHER_CLE256		112		// clefia256-cbc 
#define SSH2_CIPHER_CLE128R		115		// clefia128-ctr 
#define SSH2_CIPHER_CLE192R		116		// clefia191-ctr 
#define SSH2_CIPHER_CLE256R		117		// clefia256-ctr 

#define	SSH2_GCM_TAGSIZE		16
#define	SSH2_CIPHER_GCM(n)		((n / 100) == 2)
#define	SSH2_AEAD_AES128GCM		200		// AEAD_AES_128_GCM
#define	SSH2_AEAD_AES192GCM		201		// AEAD_AES_192_GCM
#define	SSH2_AEAD_AES256GCM		202		// AEAD_AES_256_GCM

#define	SSH2_CCM_TAGSIZE		16
#define	SSH2_CIPHER_CCM(n)		((n / 100) == 3)
#define	SSH2_AEAD_AES128CCM		300		// AEAD_AES_128_CCM
#define	SSH2_AEAD_AES192CCM		301		// AEAD_AES_192_CCM
#define	SSH2_AEAD_AES256CCM		302		// AEAD_AES_256_CCM

#define	SSH2_POLY1305_TAGSIZE	16
#define	SSH2_CIPHER_POLY(n)		((n / 100) == 4)
#define	SSH2_CHACHA20_POLY1305	400		// chacha20-poly1305@openssh.com

#define	COMPLEVEL		6

#define	MODE_ENC		0
#define	MODE_DEC		1

#define INTBLOB_LEN     20
#define SIGBLOB_LEN     (2*INTBLOB_LEN)

#define CURVE25519_SIZE 32

#define	EVP_CTRL_POLY_IV_GEN	0x30
#define	EVP_CTRL_POLY_SET_TAG	0x31
#define	EVP_CTRL_POLY_GET_TAG	0x32

class CCipher: public CObject
{
public:
	int m_Index;
	LPBYTE m_IvBuf;
	LPBYTE m_KeyBuf;
	EVP_CIPHER_CTX *m_pEvp;
	int m_Mode;

	int Init(LPCTSTR name, int mode, LPBYTE key = NULL, int len = (-1), LPBYTE iv = NULL);
	void Close();
	int SetIvCounter(DWORD seq);
	int GeHeadData(LPBYTE inbuf);
	int Cipher(LPBYTE inbuf, int len, CBuffer *out);
	int GetIndex(LPCTSTR name);
	int GetKeyLen(LPCTSTR name = NULL);
	int GetBlockSize(LPCTSTR name = NULL);
	BOOL IsAEAD(LPCTSTR name = NULL);
	BOOL IsPOLY(LPCTSTR name = NULL);
	LPCTSTR GetName(int num);
	int GetNum(LPCTSTR str);
	LPCTSTR GetTitle();
	void MakeKey(CBuffer *bp, LPCTSTR pass);

	static void CCipher::BenchMark(CStringA &out);

	CCipher();
	~CCipher();
};

class CMacomp: public CObject
{
public:
	int m_Index;
	const EVP_MD *m_Md;
	HMAC_CTX *m_pHmac;
	int m_KeyLen;
	LPBYTE m_KeyBuf;
	int m_BlockSize;
	void *m_UmacCtx;
	BOOL m_UmacMode;
	BOOL m_EtmMode;

	int Init(LPCTSTR name, LPBYTE key = NULL, int len = (-1));
	void Compute(DWORD seq, LPBYTE inbuf, int len, CBuffer *out);
	int GetIndex(LPCTSTR name);
	int GetKeyLen(LPCTSTR name = NULL);
	int GetBlockSize(LPCTSTR name = NULL);
	LPCTSTR GetTitle();
	inline BOOL IsEtm() { return m_EtmMode; }
	BOOL IsAEAD(LPCTSTR name = NULL);

	static void BenchMark(CString &out);

	CMacomp();
	~CMacomp();
};

class CCompress: public CObject
{
public:
	int m_Mode;
	z_stream m_InCompStr;
	z_stream m_OutCompStr;

	int Init(LPCTSTR name, int mode, int level = 6);
	void Compress(LPBYTE inbuf, int len, CBuffer *out);
	void UnCompress(LPBYTE inbuf, int len, CBuffer *out);
	LPCTSTR GetTitle();

	CCompress();
	~CCompress();
};

#define	IDKEY_NONE		00000
#define	IDKEY_RSA1		00001
#define	IDKEY_RSA2		00002
#define	IDKEY_DSA2		00004
#define	IDKEY_ED25519	00010
#define	IDKEY_RESV		00020		// not use
#define	IDKEY_ECDSA		00040

#define	IDKEY_DSA2EX	00104		// BIGNUM2 fix
#define	IDKEY_ECDSAEX	00140		// BIGNUM2 fix

#define	IDKEY_CERTV00	00200
#define	IDKEY_CERTV01	00400
#define	IDKEY_CERTX509	01000

#define	IDKEY_TYPE_MASK	00077
#define	IDKEY_CERT_MASK	01600

#define	SSHFP_KEY_RESERVED	0
#define	SSHFP_KEY_RSA		1
#define	SSHFP_KEY_DSA		2
#define	SSHFP_KEY_ECDSA		3
#define	SSHFP_KEY_ED25519	4 

#define	SSHFP_HASH_RESERVED	0
#define	SSHFP_HASH_SHA1		1
#define	SSHFP_HASH_SHA256	2

#define	SSHFP_DIGEST_MD5		0
#define	SSHFP_DIGEST_RIPEMD160	1
#define	SSHFP_DIGEST_SHA1		2
#define	SSHFP_DIGEST_SHA256		3
#define	SSHFP_DIGEST_SHA384		4
#define	SSHFP_DIGEST_SHA512		5

#define	SSHFP_FORMAT_HEX			0
#define	SSHFP_FORMAT_BASE64			1
#define	SSHFP_FORMAT_BUBBLEBABBLE	2
#define	SSHFP_FORMAT_RANDOMART		3
#define	SSHFP_FORMAT_SIMPLE			4

#define DNS_RDATACLASS_IN	1
#define DNS_RDATATYPE_SSHFP	44

#define ED25519_SECBYTES	64
#define ED25519_PUBBYTES	32
#define	ED25519_SIGBYTES	64

typedef struct _ed25519_key {
	BYTE pub[ED25519_PUBBYTES];
	BYTE sec[ED25519_SECBYTES];
} ED25519_KEY;

//#define	USE_X509

class CIdKey: public CObject
{
public:
	int m_Uid;
	BOOL m_Flag;
	CString m_Name;
	CString m_Hash;
	int m_Type;
	int m_Cert;
	RSA *m_Rsa;
	int m_RsaNid;
	DSA *m_Dsa;
	EC_KEY *m_EcDsa;
	int	 m_EcNid;
	ED25519_KEY *m_Ed25519;
	CString m_Work;
	CBuffer m_CertBlob;
	BOOL m_bSecInit;
	CString m_SecBlob;
	BOOL m_bHostPass;
	BOOL m_bPageant;

	int GetIndexNid(int nid);
	int GetIndexName(LPCTSTR name);
	const EVP_MD *GetEvpMdIdx(int idx);

	static int GetEcNidFromName(LPCTSTR name);
	static LPCTSTR GetEcNameFromNid(int nid);
	static const EVP_MD *GetEcEvpMdFromNid(int nid);
	int GetEcNidFromKey(EC_KEY *k);
	void RsaGenAddPara(BIGNUM *iqmp);

	int Init(LPCTSTR pass);
	int Create(int type);
	int Generate(int type, int bits, LPCTSTR pass);
	int Close();
	int ComperePublic(CIdKey *pKey);
	int Compere(CIdKey *pKey);

	void GetUserHostName(CString &str);
	void MakeKey(CBuffer *bp, LPCTSTR pass, BOOL bHost = TRUE);
	void Decrypt(CBuffer *bp, LPCTSTR str, LPCTSTR pass = NULL, BOOL bHost = TRUE);
	void Encrypt(CString &str, LPBYTE buf, int len, LPCTSTR pass = NULL, BOOL bHost = TRUE);
	void DecryptStr(CString &out, LPCTSTR str, BOOL bLocalPass = FALSE);
	void EncryptStr(CString &out, LPCTSTR str, BOOL bLocalPass = FALSE);

	void Digest(CString &out, LPCTSTR str);
	inline void SetPass(LPCTSTR pass) { Digest(m_Hash, pass); }
	int CompPass(LPCTSTR pass);
	int InitPass(LPCTSTR pass);

	LPCTSTR GetName(BOOL bCert = TRUE, BOOL bExtname = FALSE);
	int GetTypeFromName(LPCTSTR name);
	int HostVerify(LPCTSTR host);
	int ChkOldCertHosts(LPCTSTR host);

	int RsaSign(CBuffer *bp, LPBYTE buf, int len, LPCTSTR alg);
	int DssSign(CBuffer *bp, LPBYTE buf, int len);
	int EcDsaSign(CBuffer *bp, LPBYTE buf, int len);
	int Ed25519Sign(CBuffer *bp, LPBYTE buf, int len);
	int Sign(CBuffer *bp, LPBYTE buf, int len, LPCTSTR alg = NULL);

	int RsaVerify(CBuffer *bp, LPBYTE data, int datalen);
	int DssVerify(CBuffer *bp, LPBYTE data, int datalen);
	int EcDsaVerify(CBuffer *bp, LPBYTE data, int datalen);
	int Ed25519Verify(CBuffer *bp, LPBYTE data, int datalen);
	int Verify(CBuffer *bp, LPBYTE data, int datalen);

#ifdef	USE_X509
	int GetBlob_x509(CBuffer *bp, LPCSTR name);
#endif

	int GetBlob(CBuffer *bp);
	int SetBlob(CBuffer *bp, BOOL bCert = TRUE);

	int GetPrivateBlob(CBuffer *bp);
	int SetPrivateBlob(CBuffer *bp);

	int ReadPublicKey(LPCTSTR str);
	int WritePublicKey(CString &str, BOOL bAddUser = TRUE);

	int ReadPrivateKey(LPCTSTR str, LPCTSTR pass, BOOL bHost);
	int WritePrivateKey(CString &str, LPCTSTR pass);

	int SetEvpPkey(EVP_PKEY *pk);
	int LoadRsa1Key(FILE *fp, LPCTSTR pass);
	int SaveRsa1Key(FILE *fp, LPCTSTR pass);
	int LoadOpenSshKey(FILE *fp, LPCTSTR pass);
	int SaveOpenSshKey(FILE *fp, LPCTSTR pass);
	int LoadSecShKey(FILE *fp, LPCTSTR pass);
	int LoadPuttyKey(FILE *fp, LPCTSTR pass);

	BOOL IsNotSupport();

	int LoadPrivateKey(LPCTSTR file, LPCTSTR pass);
	int SavePrivateKey(int type, LPCTSTR file, LPCTSTR pass);
	int SavePublicKey(LPCTSTR file);

	int ParseCertPublicKey(LPCTSTR str);
	int LoadCertPublicKey(LPCTSTR file);
	int SaveCertPublicKey(LPCTSTR file);

	int GetString(LPCTSTR str);
	void SetString(CString &str);
	int GetProfile(LPCTSTR pSection, int Uid);
	void SetProfile(LPCTSTR pSection);
	void DelProfile(LPCTSTR pSection);
	const CIdKey & operator = (CIdKey &data);

	int GetSize();
	void FingerPrint(CString &str, int digest = SSHFP_DIGEST_SHA256, int format = SSHFP_FORMAT_BASE64);
	int DnsDigest(int hash, CBuffer &digest);

	CIdKey();
	~CIdKey();
};

class CIdKeyTab : public COptObject
{
public:
	CArray<CIdKey, CIdKey &> m_Data;

	CIdKeyTab();
	~CIdKeyTab();

	inline INT_PTR GetSize() { return m_Data.GetSize(); }
	inline CIdKey & GetAt(int nIndex) { return m_Data[nIndex]; }
	inline CIdKey & operator [](int nIndex) { return m_Data[nIndex]; }
	int AddEntry(CIdKey &key, BOOL bDups = TRUE);
	int Add(CIdKey &key);
	CIdKey *GetUid(int uid);
	void UpdateUid(int uid);
	void RemoveUid(int uid);

	void Init();
	void GetArray(CStringArrayExt &stra);
	void SetArray(CStringArrayExt &stra);
	void Serialize(int mode);
	const CIdKeyTab & operator = (CIdKeyTab &data);
};

class CPermit : public CObject
{
public:
	CString m_lHost;
	int m_lPort;
	CString m_rHost;
	int m_rPort;
	int m_Type;

	const CPermit & operator = (CPermit &data);

	CPermit();
	~CPermit();
};

#define	MAX_PACKETLEN			(1U << 31)

#define CHAN_SES_PACKET_DEFAULT (32 * 1024)
#define CHAN_SES_WINDOW_DEFAULT (64 * CHAN_SES_PACKET_DEFAULT)

#define	CHAN_OPEN_LOCAL		001
#define	CHAN_OPEN_REMOTE	002
#define	CHAN_LISTEN			004
#define	CHAN_PROXY_SOCKS	010
#define	CHAN_SOCKS5_AUTH	020
#define	CHAN_REMOTE_SOCKS	040
#define	CHAN_OK(n)			((((CChannel *)m_pChan[n])->m_Status & (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE)) == (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE))
#define	CHAN_MAXSIZE		200

#define	CEOF_IEOF		0001
#define	CEOF_ICLOSE		0002
#define	CEOF_SEOF		0004
#define	CEOF_SCLOSE		0010
#define	CEOF_DEAD		0020
#define	CEOF_IEMPY		0040
#define	CEOF_OEMPY		0100

#define	CEOF_OK(n)		((((CChannel *)m_pChan[n])->m_Eof & (CEOF_DEAD | CEOF_SEOF | CEOF_SCLOSE)) == 0)

#define	CHAN_REQ_PTY	0
#define	CHAN_REQ_SHELL	1
#define	CHAN_REQ_SUBSYS	2
#define	CHAN_REQ_EXEC	3
#define	CHAN_REQ_X11	4
#define	CHAN_REQ_ENV	5
#define	CHAN_REQ_AGENT	6
#define	CHAN_REQ_WSIZE	7

#define	SSHFT_NONE		0
#define	SSHFT_STDIO		1
#define	SSHFT_SFTP		2
#define	SSHFT_AGENT		3
#define	SSHFT_RCP		4
#define	SSHFT_X11		5

class CFilter : public CObject
{
public:
	int m_Type;
	class CChannel *m_pChan;
	class CFilter *m_pNext;
	CStringA m_Cmd;

	CFilter();
	virtual ~CFilter();
	virtual void Destroy();
	virtual void Close();
	virtual void OnConnect();
	virtual void Send(LPBYTE buf, int len);
	virtual void OnReceive(const void *lpBuf, int nBufLen);
	virtual void OnSendEmpty();
	virtual void OnRecvEmpty();
	virtual int GetSendSize();
	virtual int GetRecvSize();
};

class CChannel : public CExtSocket
{
public:
	class Cssh *m_pSsh;
	int m_Status;
	int m_RemoteID;
	int m_LocalID;
	int m_LocalComs;
	int m_LocalWind;
	int m_LocalPacks;
	int m_RemoteWind;
	int m_RemoteMax;
	CString m_TypeName;
	int m_Eof;
	CChannel *m_pNext;
	CString m_lHost, m_rHost;
	int m_lPort, m_rPort;

	CBuffer m_Input;
	CBuffer m_Output;
	ULONGLONG m_ReadByte;
	ULONGLONG m_WriteByte;
	CTime m_ConnectTime;

	class CFilter *m_pFilter;

	const CChannel & operator = (CChannel &data);

	CChannel();
	virtual ~CChannel();

	void Close();
	int Send(const void *lpBuf, int nBufLen, int nFlags = 0);
	void OnError(int err);
	void OnClose();
	void OnAccept(SOCKET hand);
	void OnConnect();
	void OnReceiveCallBack(void *lpBuf, int nBufLen, int nFlags);
	int OnReceiveProcBack(void *lpBuf, int nBufLen, int nFlags);
	int GetSendSize();
	int GetRecvSize();
	void OnRecvEmpty();
	void OnSendEmpty();

	void DoClose();
	int CreateListen(LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress, UINT nRemotePort);
};

class CStdIoFilter : public CFilter
{
public:
	CStdIoFilter();
	void OnReceive(const void *lpBuf, int nBufLen);
	void OnSendEmpty();
	void OnRecvEmpty();
	int GetSendSize();
	int GetRecvSize();
};

class CX11Filter : public CFilter
{
public:
	CBuffer m_Buffer;

	CX11Filter();
	void OnReceive(const void *lpBuf, int nBufLen);
	void OnConnect();
	int GetSendSize();
	int GetRecvSize();
};

class CSFtpFilter : public CFilter
{
public:
	class CSFtp *m_pSFtp;

	CSFtpFilter();
	void Destroy();
	void OnConnect();
	void OnReceive(const void *lpBuf, int nBufLen);
	int GetSendSize();
};

class CAgent : public CFilter
{
public:
	CBuffer m_RecvBuf;

	CAgent();
	~CAgent();

	void OnReceive(const void *lpBuf, int nBufLen);
	int GetSendSize();

	BOOL IsLocked();
	LPCTSTR GetRsaSignAlg(CIdKey *key, int flag);
	CIdKey *GetIdKey(CIdKey *key, LPCTSTR pass);
	void ReceiveBuffer(CBuffer *bp);
};

class CRcpUpload : public CFilter
{
public:
	class Cssh *m_pSsh;
	CStringList m_FileList;
	CString m_Path;
	CString m_File;
	struct _stati64 m_Stat;
	int m_Fd;
	LONGLONG m_Size;
	int m_Mode;
	class CProgDlg m_ProgDlg;

	void Destroy();
	void Close();
	void OnReceive(const void *lpBuf, int nBufLen);
	void OnClose();
	void OnSendEmpty();

	CRcpUpload();
	~CRcpUpload();
};

#define	DHMODE_GROUP_1		0
#define	DHMODE_GROUP_14		1
#define	DHMODE_GROUP_GEX	2
#define	DHMODE_GROUP_GEX256	3
#define	DHMODE_ECDH_S2_N256	4
#define	DHMODE_ECDH_S2_N384	5
#define	DHMODE_ECDH_S2_N521	6
#define DHMODE_CURVE25519	7
#define	DHMODE_GROUP_14_256	8
#define	DHMODE_GROUP_15_512	9
#define	DHMODE_GROUP_16_512	10
#define	DHMODE_GROUP_17_512	11
#define	DHMODE_GROUP_18_512	12

#define	AUTH_MODE_NONE		0
#define	AUTH_MODE_PUBLICKEY	1
#define	AUTH_MODE_PASSWORD	2
#define	AUTH_MODE_KEYBOARD	3
#define	AUTH_MODE_HOSTBASED	4
#define	AUTH_MODE_GSSAPI	5

#define	PROP_KEX_ALGS		0
#define	PROP_HOST_KEY_ALGS	1
#define	PROP_ENC_ALGS_CTOS	2		// 0
#define	PROP_ENC_ALGS_STOC	3		// 1
#define	PROP_MAC_ALGS_CTOS	4		// 2
#define	PROP_MAC_ALGS_STOC	5		// 3
#define	PROP_COMP_ALGS_CTOS	6		// 4
#define	PROP_COMP_ALGS_STOC	7		// 5
#define	PROP_LANG_CTOS		8
#define	PROP_LANG_STOC		9

#define	DHGEX_MIN_BITS		1024
#define	DHGEX_MAX_BITS		8192

enum EAuthStat {
	AST_START = 0,
	AST_PUBKEY_TRY,
	AST_HOST_TRY,
	AST_PASS_TRY,
	AST_KEYB_TRY,
	AST_GSSAPI_TRY,
	AST_DONE
};

class Cssh : public CExtSocket 
{
public:
	Cssh(class CRLoginDoc *pDoc);
	virtual ~Cssh();

	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	void Close();
	void OnConnect();
	void OnReceiveCallBack(void* lpBuf, int nBufLen, int nFlags = 0);
	int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	void SendWindSize(int x, int y);
	void OnTimer(UINT_PTR nIDEvent);
	void OnRecvEmpty();
	void OnSendEmpty();
	void GetStatus(CString &str);
	int GetRecvSize();
	int GetSendSize();

	int m_SSHVer;
	CString m_HostName;
	int m_StdChan;
	CFilter *m_pListFilter;
	CArray<CIdKey, CIdKey &> m_IdKeyTab;
	CString m_x11AuthName, m_x11LocalName;
	CBuffer m_x11AuthData, m_x11LocalData;

	CMutex *m_pAgentMutex;
	CStringA m_AgentPass;

private:
	CString m_ServerVerStr;
	CString m_ClientVerStr;

	int m_InPackStat;
	int m_InPackLen;
	int m_InPackPad;
	CBuffer m_Incom;
	int m_PacketStat;

	CCompress m_EncCmp;
	CCompress m_DecCmp;
	CCipher m_EncCip;
	CCipher m_DecCip;
	CMacomp m_EncMac;
	CMacomp m_DecMac;

	BYTE m_SessionId[64];
	int m_SessionIdLen;
	BYTE m_SessionKey[64];
	CIdKey m_HostKey;
	CIdKey *m_pIdKey;
	int m_IdKeyPos;
	int m_AuthReqTab[10];

	int m_ServerFlag;
	int m_SupportCipher;
	int m_SupportAuth;
	int m_CipherMode;
	int m_CipTabMax;
	int m_CipTab[8];
	int m_CompMode;

	CIdKey *SelectIdKeyEntry();
	int SMsgPublicKey(CBuffer *bp);
	int SMsgAuthRsaChallenge(CBuffer *bp);
	void ReceivePacket(CBuffer *bp);
	void SendPacket(CBuffer *bp);
	void SendDisconnect(LPCSTR str);

	int m_SSH2Status;

#define	SSH2_STAT_HAVEPROP		00001
#define	SSH2_STAT_HAVEKEYS		00002
#define	SSH2_STAT_HAVESESS		00004
#define	SSH2_STAT_HAVELOGIN		00010
#define	SSH2_STAT_HAVESTDIO		00020
#define	SSH2_STAT_HAVESHELL		00040
#define	SSH2_STAT_HAVEPFWD		00100
#define	SSH2_STAT_HAVEAGENT		00200
#define	SSH2_STAT_SENTKEXINIT	00400
#define	SSH2_STAT_AUTHGSSAPI	01000

	DWORD m_SendPackSeq;
	DWORD m_RecvPackSeq;
	DWORD m_SendPackLen;
	DWORD m_RecvPackLen;
	CBuffer m_InPackBuf;
	BYTE m_Cookie[16];
	CString m_SProp[10], m_CProp[10], m_VProp[10];
	LPBYTE m_VKey[6];
	CBuffer m_HisPeer;
	CBuffer m_MyPeer;
	int m_NeedKeyLen;
	int m_DhMode;
	DH *m_SaveDh;
	int m_EcdhCurveNid;
	EC_KEY *m_EcdhClientKey;
	const EC_GROUP *m_EcdhGroup;
	BYTE m_CurveClientKey[CURVE25519_SIZE];
	BYTE m_CurveClientPubkey[CURVE25519_SIZE];
	int m_AuthStat;
	int m_AuthMode;
	CString m_AuthMeta;
	CString m_AuthLog;
	CWordArray m_GlbReqMap;
	CWordArray m_OpnReqMap;
	CWordArray m_ChnReqMap;
	CChannel *m_pChanNext;
	CChannel *m_pChanFree;
	CChannel *m_pChanFree2;
	int m_OpenChanCount;
	CPtrArray m_pChan;
	CArray<CPermit, CPermit &> m_Permit;
	CBuffer m_StdInRes;
	CRcpUpload m_RcpCmd;
	int m_bPfdConnect;
	BOOL m_bExtInfo;
	CStringIndex m_ExtInfo;
	int m_DhGexReqBits;
	time_t m_ConnectTime;
	int m_KeepAliveSendCount;
	int m_KeepAliveReplyCount;
	int m_KeepAliveRecvGlobalCount;
	int m_KeepAliveRecvChannelCount;

	CredHandle m_SSPI_hCredential;
	CtxtHandle *m_SSPI_phContext;
	CString m_SSPI_TargetName;
	CtxtHandle m_SSPI_hNewContext;
	TimeStamp m_SSPI_tsExpiry;

	void SendTextMsg(LPCSTR str, int len);
	void PortForward();
	int MatchList(LPCTSTR client, LPCTSTR server, CString &str);
	int ChannelOpen();
	void ChannelClose(int id, BOOL bClose = FALSE);
	LPCTSTR GetSigAlgs();
	BOOL IsServerVersion(LPCTSTR pattan);
	void UserAuthNextState();
	void AddAuthLog(LPCTSTR str, ...);

	void SendMsgKexInit();
	void SendMsgNewKeys();
	void SendMsgKexDhInit();
	void SendMsgKexDhGexRequest();
	int SendMsgKexEcdhInit();
	void SendMsgKexCurveInit();

	void SendMsgServiceRequest(LPCSTR str);
	int SendMsgUserAuthRequest(LPCSTR str);

	int SendMsgChannelOpen(int n, LPCSTR type, LPCTSTR lhost = NULL, int lport = 0, LPCTSTR rhost = NULL, int rport = 0);
	void SendMsgChannelRequesstShell(int id);
	void SendMsgChannelRequesstSubSystem(int id, LPCSTR cmds);
	void SendMsgChannelRequesstExec(int id);

	void SendMsgGlobalRequest(int num, LPCSTR str, LPCTSTR rhost = NULL, int rport = 0);
	void SendMsgKeepAlive();
	void SendMsgUnimplemented();
	void SendDisconnect2(int st, LPCSTR str);

	int SSH2MsgKexInit(CBuffer *bp);
	int SSH2MsgKexDhReply(CBuffer *bp);
	int SSH2MsgKexDhGexGroup(CBuffer *bp);
	int SSH2MsgKexDhGexReply(CBuffer *bp);
	int SSH2MsgKexEcdhReply(CBuffer *bp);
	int SSH2MsgKexCurveReply(CBuffer *bp);
	int SSH2MsgNewKeys(CBuffer *bp);
	int SSH2MsgExtInfo(CBuffer *bp);

	int SSH2MsgUserAuthPkOk(CBuffer *bp);
	int SSH2MsgUserAuthPasswdChangeReq(CBuffer *bp);
	int SSH2MsgUserAuthInfoRequest(CBuffer *bp);
	int SSH2MsgUserAuthGssapiProcess(CBuffer *bp, int type);

	int SSH2MsgChannelOpen(CBuffer *bp);
	int SSH2MsgChannelOpenReply(CBuffer *bp, int type);
	int SSH2MsgChannelClose(CBuffer *bp);
	int SSH2MsgChannelData(CBuffer *bp, int type);
	int SSH2MsgChannelEof(CBuffer *bp);
	int SSH2MsgChannelRequesst(CBuffer *bp);
	int SSH2MsgChannelAdjust(CBuffer *bp);
	int SSH2MsgChannelRequestReply(CBuffer *bp, int type);

	int SSH2MsgGlobalHostKeys(CBuffer *bp);
	int SSH2MsgGlobalRequest(CBuffer *bp);
	int SSH2MsgGlobalRequestReply(CBuffer *bp, int type);

	void ReceivePacket2(CBuffer *bp);
	void SendPacket2(CBuffer *bp);

public:
	void LogIt(LPCTSTR format, ...);
	int GetOpenChanCount() { return m_OpenChanCount; }
	void OpenSFtpChannel();
	void SendMsgChannelData(int id);
	void SendMsgChannelOpenConfirmation(int id);
	void SendMsgChannelOpenFailure(int id);
	void DecodeProxySocks(int id, CBuffer &resvbuf);
	void ChannelCheck(int n);
	void ChannelPolling(int id);
	void ChannelAccept(int id, SOCKET hand);
	void OpenRcpUpload(LPCTSTR file);
};

//////////////////////////////////////////////////////////////////////
// OpenSSH C lib
//////////////////////////////////////////////////////////////////////

#if	OPENSSL_VERSION_NUMBER < 0x10100000L
extern EVP_MD_CTX *EVP_MD_CTX_new(void);
extern void EVP_MD_CTX_free(EVP_MD_CTX *pCtx);
extern HMAC_CTX *HMAC_CTX_new(void);
extern void HMAC_CTX_free(HMAC_CTX *pHmac);
const EVP_CIPHER *EVP_CIPHER_CTX_cipher(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_block_size(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_encrypting(const EVP_CIPHER_CTX *ctx);
int EVP_PKEY_id(const EVP_PKEY *pkey);
int RSA_bits(const RSA *r);
int RSA_size(const RSA *r);
void RSA_get0_key(const RSA *r, const BIGNUM **n, const BIGNUM **e, const BIGNUM **d);
void RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q);
void RSA_get0_crt_params(const RSA *r, const BIGNUM **dmp1, const BIGNUM **dmq1, const BIGNUM **iqmp);
int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d);
int RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q);
int RSA_set0_crt_params(RSA *r, BIGNUM *dmp1, BIGNUM *dmq1, BIGNUM *iqmp);
int DSA_bits(const DSA *dsa);
void DSA_get0_pqg(const DSA *d, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g);
void DSA_get0_key(const DSA *d, const BIGNUM **pub_key, const BIGNUM **priv_key);
int DSA_set0_pqg(DSA *d, BIGNUM *p, BIGNUM *q, BIGNUM *g);
int DSA_set0_key(DSA *d, BIGNUM *pub_key, BIGNUM *priv_key);
void DSA_SIG_get0(const DSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps);
int DSA_SIG_set0(DSA_SIG *sig, BIGNUM *r, BIGNUM *s);
void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps);
int ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s);
int DH_bits(const DH *dh);
void DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g);
int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g);
void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key);
int DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key);
int DH_set_length(DH *dh, long length);
#endif

extern const EVP_CIPHER *evp_aes_128_ctr(void);
extern const EVP_CIPHER *evp_camellia_128_ctr(void);
extern const EVP_CIPHER *evp_seed_ctr(void);
extern const EVP_CIPHER *evp_twofish_ctr(void);
extern const EVP_CIPHER *evp_twofish_cbc(void);
extern const EVP_CIPHER *evp_serpent_ctr(void);
extern const EVP_CIPHER *evp_serpent_cbc(void);
extern const EVP_CIPHER *evp_clefia_ctr(void);
extern const EVP_CIPHER *evp_clefia_cbc(void);
extern const EVP_CIPHER *evp_bf_ctr(void);
extern const EVP_CIPHER *evp_cast5_ctr(void);
extern const EVP_CIPHER *evp_idea_ctr(void);
extern const EVP_CIPHER *evp_des3_ctr(void);
extern const EVP_CIPHER *evp_ssh1_3des(void);
extern const EVP_CIPHER *evp_ssh1_bf(void);
extern const EVP_CIPHER *evp_chachapoly_256(void);

extern void rsa_public_encrypt(BIGNUM *out, BIGNUM *in, RSA *key);
extern int	ssh_crc32(LPBYTE buf, int len);
extern int dh_pub_is_valid(DH *dh, const BIGNUM *dh_pub);
extern int dh_gen_key(DH *dh, int need);
extern DH *dh_new_group_asc(const char *gen, const char *modulus);
extern DH *dh_new_group1(void);
extern DH *dh_new_group14(void);
extern DH *dh_new_group15(void);
extern DH *dh_new_group16(void);
extern DH *dh_new_group17(void);
extern DH *dh_new_group18(void);
extern int kex_dh_hash(BYTE *digest, LPCSTR client_version_string, LPCSTR server_version_string, LPBYTE ckexinit, int ckexinitlen, LPBYTE skexinit, int skexinitlen, LPBYTE serverhostkeyblob, int sbloblen, BIGNUM *client_dh_pub, BIGNUM *server_dh_pub, BIGNUM *shared_secret, const EVP_MD *evp_md);
extern int dh_estimate(int bits);
extern int kex_gex_hash(BYTE *digest, LPCSTR client_version_string, LPCSTR server_version_string, LPBYTE ckexinit, int ckexinitlen, LPBYTE skexinit, int skexinitlen, LPBYTE serverhostkeyblob, int sbloblen, int min, int wantbits, int max, BIGNUM *prime, BIGNUM *gen, BIGNUM *client_dh_pub, BIGNUM *server_dh_pub, BIGNUM *shared_secret, const EVP_MD *evp_md);
extern int key_ec_validate_public(const EC_GROUP *group, const EC_POINT *pub);
extern int kex_ecdh_hash(BYTE *digest, LPCSTR client_version_string, LPCSTR server_version_string, LPBYTE ckexinit, int ckexinitlen, LPBYTE skexinit, int skexinitlen, LPBYTE serverhostkeyblob, int sbloblen, const EC_GROUP *ec_group, const EC_POINT *client_dh_pub, const EC_POINT *server_dh_pub, BIGNUM *shared_secret, const EVP_MD *evp_md);
extern int kex_c25519_hash(BYTE *digest, LPCSTR client_version_string, LPCSTR server_version_string, LPBYTE ckexinit, int ckexinitlen, LPBYTE skexinit, int skexinitlen, LPBYTE serverhostkeyblob, int sbloblen, const BYTE client_dh_pub[CURVE25519_SIZE], const BYTE server_dh_pub[CURVE25519_SIZE], BIGNUM *shared_secret, const EVP_MD *evp_md);
extern u_char *derive_key(int id, int need, u_char *hash, int hashlen, BIGNUM *shared_secret, u_char *session_id, int sesslen, const EVP_MD *evp_md);

extern void *mm_zalloc(void *mm, unsigned int ncount, unsigned int size);
extern void mm_zfree(void *mm, void *address);
extern void rand_buf(void *buf, int len);
extern int rand_int(int n);
extern long rand_long(long n);
extern double rand_double();

extern struct umac_ctx *UMAC_open(int len);
extern void UMAC_init(struct umac_ctx *ctx, u_char *key, int len);
extern void UMAC_update(struct umac_ctx *ctx, const u_char *input, size_t len);
extern void UMAC_final(struct umac_ctx *ctx, u_char *tag, u_char *nonce);
extern void UMAC_close(struct umac_ctx *ctx);

// curve25519.c
extern int crypto_scalarmult_curve25519(unsigned char *, const unsigned char *, const unsigned char *);

// openbsd-compat
extern int crypto_hash_sha512(unsigned char *out,const unsigned char *in,unsigned long long inlen);
extern int crypto_verify_32(const unsigned char *x,const unsigned char *y);
extern int bcrypt_pbkdf(const char *pass, size_t passlen, const unsigned char *salt, size_t saltlen, unsigned char *key, size_t keylen, unsigned int rounds);

// ssh-ed2519
extern int crypto_sign_ed25519_keypair(unsigned char *pk, unsigned char *sk);
extern int crypto_sign_ed25519(unsigned char *sm, unsigned long long *smlen, const unsigned char *m, unsigned long long mlen, const unsigned char *sk);
extern int crypto_sign_ed25519_open(unsigned char *m,unsigned long long *mlen, const unsigned char *sm,unsigned long long smlen, const unsigned char *pk);


#endif // !defined(AFX_SSH_H__2A682FAC_4F24_4168_9082_C9CDF2DD19D7__INCLUDED_)
