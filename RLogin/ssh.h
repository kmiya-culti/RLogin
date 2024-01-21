//////////////////////////////////////////////////////////////////////
// ssh.h : インターフェイス
//

#pragma once

#include "afxtempl.h"
#include "Data.h"
#include "SFtp.h"
#include "ProgDlg.h"

#include "zlib.h"

#include "openssl/bn.h"
#include "openssl/evp.h"
#include "openssl/err.h"
#include "openssl/hmac.h"
#include "openssl/kdf.h"
#include "openssl/pem.h"
#include "openssl/engine.h"

#include "openssl/provider.h"
#include "openssl/params.h"
#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>

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

#define	SSH2_CIPHER_GCM(n)		((n / 100) == 2)
#define	SSH2_AEAD_AES128GCM		200		// AEAD_AES_128_GCM
#define	SSH2_AEAD_AES192GCM		201		// AEAD_AES_192_GCM
#define	SSH2_AEAD_AES256GCM		202		// AEAD_AES_256_GCM

#define	SSH2_CIPHER_CCM(n)		((n / 100) == 3)
#define	SSH2_AEAD_AES128CCM		300		// AEAD_AES_128_CCM
#define	SSH2_AEAD_AES192CCM		301		// AEAD_AES_192_CCM
#define	SSH2_AEAD_AES256CCM		302		// AEAD_AES_256_CCM

#define	SSH2_CIPHER_AEAD(n)		((n / 100) == 4)
#define	SSH2_AEAD_CHACHAPOLY	400		// AEAD_CHACHA20_POLY1305

#define	SSH2_CIPHER_POLY(n)		((n / 100) == 5)
#define	SSH2_CHACHA20_POLY1305	500		// chacha20-poly1305@openssh.com

#define	SSH2_CIPHER_IVSIZEMAX	16
#define	SSH2_CIPHER_TAGSIZEMAX	16

#define	COMPLEVEL				6

#define	MODE_ENC				0
#define	MODE_DEC				1

#define INTBLOB_LEN				20
#define SIGBLOB_LEN				(2*INTBLOB_LEN)

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
	int CipherSshChahaPoly(LPBYTE inbuf, int len, CBuffer *out);
	int CipherAead(LPBYTE inbuf, int len, CBuffer *out);
	int CipherAeadCCM(LPBYTE inbuf, int len, CBuffer *out);
	int Cipher(LPBYTE inbuf, int len, CBuffer *out);
	int GetIndex(LPCTSTR name);
	int GetKeyLen(LPCTSTR name = NULL);
	int GetBlockSize(LPCTSTR name = NULL);
	int GetIvSize(LPCTSTR name = NULL);
	int GetTagSize(LPCTSTR name = NULL);
	BOOL IsAEAD(LPCTSTR name = NULL);
	BOOL IsPOLY(LPCTSTR name = NULL);
	LPCTSTR GetName(int num);
	int GetNum(LPCTSTR str);
	LPCTSTR GetTitle();
	void MakeKey(CBuffer *bp, LPCTSTR pass);

	static void CCipher::BenchMark(CBuffer *out);

	CCipher();
	~CCipher();
};

class CMacomp: public CObject
{
public:
	int m_Index;
	const EVP_MD *m_Md;
#ifdef	USE_MACCTX
	EVP_MAC_CTX *m_pMacCtx;
	EVP_MAC *m_pMac;
#else
	HMAC_CTX *m_pHmac;
#endif
	int m_KeyLen;
	LPBYTE m_KeyBuf;
	int m_BlockSize;
	void *m_UmacCtx;
	BOOL m_UmacMode;
	BOOL m_EtmMode;

	void Close();
	int Init(LPCTSTR name, LPBYTE key = NULL, int len = (-1));
	void Compute(DWORD seq, LPBYTE inbuf, int len, CBuffer *out);
	int GetIndex(LPCTSTR name);
	int GetKeyLen(LPCTSTR name = NULL);
	int GetBlockSize(LPCTSTR name = NULL);
	LPCTSTR GetTitle();
	inline BOOL IsEtm() { return m_EtmMode; }
	BOOL IsAEAD(LPCTSTR name = NULL);

	static int SpeedCheck();
	static void BenchMark(CBuffer *out);

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

#define	IDKEY_NONE					00000
#define	IDKEY_RSA1					00001
#define	IDKEY_RSA2					00002
#define	IDKEY_DSA2					00004
#define	IDKEY_ED25519				00010
#define	IDKEY_ED448					00011
#define	IDKEY_XMSS					00020
#define	IDKEY_UNKNOWN				00030
#define	IDKEY_ECDSA					00040

#define	IDKEY_DSA2EX				00104		// BIGNUM2 fix
#define	IDKEY_ECDSAEX				00140		// BIGNUM2 fix

#define	IDKEY_CERTV00				00200
#define	IDKEY_CERTV01				00400
#define	IDKEY_CERTX509				01000

#define	IDKEY_TYPE_MASK				00077
#define	IDKEY_CERT_MASK				01600

#define	IDKEY_AGEANT_NONE			0
#define	IDKEY_AGEANT_PUTTY			1
#define	IDKEY_AGEANT_WINSSH			2
#define	IDKEY_AGEANT_PUTTYPIPE		3

#define	SSHFP_KEY_RESERVED			0
#define	SSHFP_KEY_RSA				1
#define	SSHFP_KEY_DSA				2
#define	SSHFP_KEY_ECDSA				3
#define	SSHFP_KEY_ED25519			4 
#define	SSHFP_KEY_XMSS				5
#define	SSHFP_KEY_ED448				6

#define	SSHFP_HASH_RESERVED			0
#define	SSHFP_HASH_SHA1				1
#define	SSHFP_HASH_SHA256			2

#define	SSHFP_DIGEST_MD5			0
#define	SSHFP_DIGEST_RIPEMD160		1
#define	SSHFP_DIGEST_SHA1			2
#define	SSHFP_DIGEST_SHA256			3
#define	SSHFP_DIGEST_SHA384			4
#define	SSHFP_DIGEST_SHA512			5

#define	SSHFP_FORMAT_HEX			0
#define	SSHFP_FORMAT_BASE64			1
#define	SSHFP_FORMAT_BUBBLEBABBLE	2
#define	SSHFP_FORMAT_RANDOMART		3
#define	SSHFP_FORMAT_SIMPLE			4

#define DNS_TYPE_SSHFP				44

#define	MAKEKEY_FIXTEXT				0
#define	MAKEKEY_USERHOST			1
#define	MAKEKEY_HOSTSID				2
#define	MAKEKEY_USERSID				3

#define	EXPORT_STYLE_OSSLPEM		0
#define	EXPORT_STYLE_OSSLPFX		1
#define	EXPORT_STYLE_OPENSSH		2
#define	EXPORT_STYLE_PUTTY2			3
#define	EXPORT_STYLE_PUTTY3			4
#define	EXPORT_STYLE_OLDRSA			5

#define	GETPKEY_PUBLICKEY			1
#define	GETPKEY_PRIVATEKEY			2
#define	GETPKEY_KEYPAIR				3

typedef	struct _GenStatus {
	int		abort;
	int		type;
	int		pos;
	int		max;
	int		stat;
	int		zero;
	int		count;
} GENSTATUS;

class CXmssKey : public CObject
{
public:
	uint32_t m_oId;
	int m_PubLen;
	BYTE *m_pPubBuf;
	int m_SecLen;
	BYTE *m_pSecBuf;
	int m_PassLen;
	TCHAR *m_pPassBuf;
	CStringA m_EncName;
	CBuffer m_EncIvBuf;

	void RemoveAll();
	const CXmssKey & operator = (CXmssKey &data);
	BOOL SetBufSize(uint32_t oId = 0);
	BOOL SetBufSize(LPCSTR name);
	void SetPassBuf(LPCTSTR pass);
	BOOL MakeEncIvBuf();

	BOOL LoadOpenSshSec(CBuffer *bp);
	BOOL SaveOpenSshSec(CBuffer *bp);

	BOOL LoadStateFile(LPCTSTR fileName);
	BOOL SaveStateFile(LPCTSTR fileName);

	const char *GetName();
	int GetBits();
	int GetHeight();
	int KeyPair(GENSTATUS *gs);
	int GetSignByte();
	int Sign(unsigned char *sm, unsigned long long *smlen, const unsigned char *m, unsigned long long mlen);
	int Verify(unsigned char *m, unsigned long long *mlen, const unsigned char *sm, unsigned long long smlen);

	CXmssKey();
	~CXmssKey();
};

class CIdKey : public CObject
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
	CBuffer m_PublicKey;
	CBuffer m_PrivateKey;
	CXmssKey m_XmssKey;
	CString m_Work;
	CBuffer m_CertBlob;
	BOOL m_bSecInit;
	CString m_SecBlob;
	BOOL m_bHostPass;
	int m_AgeantType;
	CString m_FilePath;
	CString m_LoadMsg;
	CString m_TypeName;
	CBuffer m_TypeBlob;

	int GetIndexNid(int nid);
	int GetIndexName(LPCTSTR name);
	const EVP_MD *GetEvpMdIdx(int idx);

	static int GetEcNidFromName(LPCTSTR name);
	static LPCTSTR GetEcNameFromNid(int nid);
	static const EVP_MD *GetEcEvpMdFromNid(int nid);
	int GetEcNidFromKey(EC_KEY *k);
	void RsaGenAddPara(BIGNUM *iqmp);
	int EdFromPkeyRaw(const EVP_PKEY *pkey);

	int Init(LPCTSTR pass);
	int Create(int type);
	int Generate(int type, int bits, LPCTSTR pass, GENSTATUS *gs = NULL);
	int Close();
	int ComperePublic(CIdKey *pKey);
	int Compere(CIdKey *pKey);

	BOOL GetUserSid(CBuffer &buf);
	BOOL GetHostSid(CBuffer &buf);
	void GetUserHostName(CString &str);
	void MakeKey(CBuffer *bp, LPCTSTR pass, int Mode);
	void Decrypt(CBuffer *bp, LPCTSTR str, LPCTSTR pass, int Mode);
	void Encrypt(CString &str, LPBYTE buf, int len, LPCTSTR pass, int Mode);
	void DecryptStr(CString &out, LPCTSTR str, BOOL bLocalPass = TRUE);
	void EncryptStr(CString &out, LPCTSTR str);

	void Digest(CString &out, LPCTSTR str);
	inline void SetPass(LPCTSTR pass) { Digest(m_Hash, pass); }
	int CompPass(LPCTSTR pass);
	int InitPass(LPCTSTR pass);

	LPCTSTR GetName(BOOL bCert = TRUE, BOOL bExtname = FALSE);
	int GetTypeFromName(LPCTSTR name);
	BOOL KnownHostsCheck(LPCTSTR dig);
	int HostVerify(LPCTSTR host, UINT port, class Cssh *pSsh = NULL);
	int ChkOldCertHosts(LPCTSTR host);

	int XmssSign(CBuffer *bp, LPBYTE buf, int len);
	int Sign(CBuffer *bp, LPBYTE buf, int len, LPCTSTR alg = NULL);

	int XmssVerify(CBuffer *sig, LPBYTE data, int datalen);
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

	EVP_PKEY *GetEvpPkey(int mode = GETPKEY_KEYPAIR);
	int SetEvpPkey(EVP_PKEY *pk);

	int LoadRsa1Key(FILE *fp, LPCTSTR pass);
	int SaveRsa1Key(FILE *fp, LPCTSTR pass);
	int LoadOpenSshKey(FILE *fp, LPCTSTR pass);
	int SaveOpenSshKey(FILE *fp, LPCTSTR pass);
	int LoadSecShKey(FILE *fp, LPCTSTR pass);
	int LoadPuttyKey(FILE *fp, LPCTSTR pass);
	int SavePuttyKey(FILE *fp, LPCTSTR pass, int ver);

	BOOL IsNotSupport();

	int LoadPrivateKey(LPCTSTR file, LPCTSTR pass);
	int SavePrivateKey(int fmt, LPCTSTR file, LPCTSTR pass);
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
	int GetHeight();
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
	CIdKey *ReloadUid(int uid);

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
	BOOL m_bClose;

	const CPermit & operator = (CPermit &data);

	CPermit();
	~CPermit();
};

///////////////////////////////////////////////////////

#define	MAX_PACKETLEN			(1U << 31)

#define CHAN_SES_PACKET_DEFAULT (32 * 1024)
#define CHAN_SES_WINDOW_DEFAULT (64 * CHAN_SES_PACKET_DEFAULT)

#define	CHAN_OPEN_LOCAL			001
#define	CHAN_OPEN_REMOTE		002

#define	CHAN_ENDOF_LOCAL		1
#define	CHAN_ENDOF_REMOTE		2
#define	CHAN_CLOSE_LOCAL		3
#define	CHAN_CLOSE_REMOTE		4

#define	SSHFT_NONE				0
#define	SSHFT_STDIO				1
#define	SSHFT_SFTP				2
#define	SSHFT_AGENT				3
#define	SSHFT_RCP				4
#define	SSHFT_X11				5
#define	SSHFT_LOCAL_LISTEN		6
#define	SSHFT_REMOTE_LISTEN		7
#define	SSHFT_LOCAL_SOCKS		8
#define	SSHFT_REMOTE_SOCKS		9
#define	SSHFT_SOCKET_LOCAL		10
#define	SSHFT_SOCKET_REMOTE		11

#define	CHAN_REQ_PTY			0
#define	CHAN_REQ_SHELL			1
#define	CHAN_REQ_SUBSYS			2
#define	CHAN_REQ_EXEC			3
#define	CHAN_REQ_X11			4
#define	CHAN_REQ_ENV			5
#define	CHAN_REQ_AGENT			6
#define	CHAN_REQ_WSIZE			7

#define	CHAN_MAXCONNECT			200

class CFifoSsh : public CFifoThread
{
public:
	CFifoSsh(class CRLoginDoc *pDoc, class CExtSocket *pSock);

	int GetFreeFifo(int nFd);

	virtual void EndOfData(int nFd);

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnAccept(int nFd, SOCKET socket);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);
};

class CFifoStdio : public CFifoDocument
{
public:
	CFifoStdio(class CRLoginDoc *pDoc, class CExtSocket *pSock);

	virtual void EndOfData(int nFd);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);
};

class CFifoSftp : public CFifoBase
{
public:
	class CSFtp *m_pSFtp;

public:
	CFifoSftp(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CSFtp *pSFtp);
	~CFifoSftp();

	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);
};

class CFifoSocks : public CFifoSync
{
public:
	int m_rFd;
	int m_wFd;
	int m_ChanId;

	int m_RetCode;
	CStringA m_HostName;
	int m_HostPort;

public:
	CFifoSocks(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan);

	virtual void OnLinked(int nFd, BOOL bMid);
	virtual void OnUnLinked(int nFd, BOOL bMid);
	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);

	static UINT DecodeSocksWorker(LPVOID pParam);
	void DecodeSocks();
};

class CFifoAgent : public CFifoThread
{
public:
	class CFifoChannel *m_pChan;
	CBuffer m_RecvBuffer;

public:
	CFifoAgent(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan);

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);

	BOOL IsLocked();
	LPCTSTR GetRsaSignAlg(CIdKey *key, int flag);
	CIdKey *GetIdKey(CIdKey *key, LPCTSTR pass);
	void ReceiveBuffer(CBuffer *bp);
};

class CFifoX11 : public CFifoWnd
{
public:
	class CFifoChannel *m_pChan;

	CBuffer m_Buffer;

public:
	CFifoX11(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan);

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnAccept(int nFd, SOCKET socket);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);

	BOOL OnReceive(const void *lpBuf, int nBufLen);
};

class CFifoRcp : public CFifoSync
{
public:
	class CFifoChannel *m_pChan;
	int m_Mode;
	CString m_PathName;
	CStringA m_FileName;
	class CProgDlg *m_pProgDlg;
	BOOL m_bAbortFlag;

public:
	CFifoRcp(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan, int Mode);

	virtual void OnLinked(int nFd, BOOL bMid);
	virtual void OnUnLinked(int nFd, BOOL bMid);

	static UINT RcpUpDowndWorker(LPVOID pParam);

	BOOL ReadLine(int nFd, CStringA &str);
	LPCTSTR MakePath(LPCTSTR base, LPCSTR name, CString &path);

	void RcpUpLoad();
	void RcpDownLoad();
};

///////////////////////////////////////////////////////

class CFifoChannel : public CObject
{
public:
	int m_Type;
	int m_Status;
	int m_Stage;

	int m_RemoteID;
	int m_LocalID;

	int m_LocalComs;
	int m_LocalWind;
	int m_LocalPacks;

	int m_RemoteComs;
	int m_RemotePacks;

	CStringA m_TypeName;

	CString m_lHost, m_rHost;
	int m_lPort, m_rPort;

	BOOL m_bClosed;
	CFifoBase *m_pFifoBase;
};

///////////////////////////////////////////////////////

#define	SSH2_STAT_HAVEPROP				00001
#define	SSH2_STAT_HAVEKEYS				00002
#define	SSH2_STAT_HAVESESS				00004
#define	SSH2_STAT_HAVELOGIN				00010
#define	SSH2_STAT_HAVESTDIO				00020
#define	SSH2_STAT_HAVESHELL				00040
#define	SSH2_STAT_HAVEPFWD				00100
#define	SSH2_STAT_HAVEAGENT				00200
#define	SSH2_STAT_SENTKEXINIT			00400
#define	SSH2_STAT_AUTHGSSAPI			01000
#define	SSH2_STAT_HAVERSAPUB			02000

#define	DHMODE_GROUP_1					0
#define	DHMODE_GROUP_14					1
#define	DHMODE_GROUP_GEX				2
#define	DHMODE_GROUP_GEX256				3
#define	DHMODE_ECDH_S2_N256				4
#define	DHMODE_ECDH_S2_N384				5
#define	DHMODE_ECDH_S2_N521				6
#define	DHMODE_GROUP_14_256				7
#define	DHMODE_GROUP_15_512				8
#define	DHMODE_GROUP_16_512				9
#define	DHMODE_GROUP_17_512				10
#define	DHMODE_GROUP_18_512				11
#define DHMODE_CURVE25519				12
#define DHMODE_CURVE448					13
#define	DHMODE_SNT761X25519				14
#define DHMODE_RSA1024SHA1				15
#define DHMODE_RSA2048SHA2				16

#define	AUTH_MODE_NONE					0
#define	AUTH_MODE_PUBLICKEY				1
#define	AUTH_MODE_PASSWORD				2
#define	AUTH_MODE_KEYBOARD				3
#define	AUTH_MODE_HOSTBASED				4
#define	AUTH_MODE_GSSAPI				5

#define	PROP_KEX_ALGS					0
#define	PROP_HOST_KEY_ALGS				1
#define	PROP_ENC_ALGS_CTOS				2		// 0
#define	PROP_ENC_ALGS_STOC				3		// 1
#define	PROP_MAC_ALGS_CTOS				4		// 2
#define	PROP_MAC_ALGS_STOC				5		// 3
#define	PROP_COMP_ALGS_CTOS				6		// 4
#define	PROP_COMP_ALGS_STOC				7		// 5
#define	PROP_LANG_CTOS					8
#define	PROP_LANG_STOC					9

#define	DHGEX_MIN_BITS					1024
#define	DHGEX_MAX_BITS					8192

#define sntrup761_PUBLICKEYBYTES		1158
#define sntrup761_SECRETKEYBYTES		1763
#define sntrup761_CIPHERTEXTBYTES		1039
#define sntrup761_BYTES					32

#define	KEXSTRICT_CHECK					0
#define	KEXSTRICT_DISABLE				1
#define	KEXSTRICT_ENABLE				2

#define	EXTINFOSTAT_CHECK				0
#define	EXTINFOSTAT_SEND				1
#define	EXTINFOSTAT_DONE				2

enum EAuthStat {
	AST_START = 0,
	AST_PUBKEY_TRY,
	AST_HOST_TRY,
	AST_PASS_TRY,
	AST_KEYB_TRY,
	AST_GSSAPI_TRY,
	AST_DONE
};

typedef struct _DelayedCheck {
	int type;
	int	nFd;
	int fdEvent;
	void *pParam;
	BOOL bDelBuf;
} DelayedCheck;

class Cssh : public CExtSocket 
{
public:
	int m_SSHVer;
	CString m_HostName;
	UINT m_HostPort;

	CStringA m_ServerPrompt;
	CString m_ServerVerStr;
	CString m_ClientVerStr;

	int m_SSH2Status;
	BOOL m_bKnownHostUpdate;

	CArray<CIdKey, CIdKey &> m_IdKeyTab;

	CString m_x11AuthName, m_x11LocalName;
	CBuffer m_x11AuthData, m_x11LocalData;
	BOOL m_bx11pfdEnable;

	CMutex *m_pAgentMutex;
	CStringA m_AgentPass;

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
	BOOL m_bKeybIntrReq;
	BOOL m_bReqRsaSha1;

	int m_ServerFlag;
	int m_SupportCipher;
	int m_SupportAuth;
	int m_CipherMode;
	int m_CipTabMax;
	int m_CipTab[8];
	int m_CompMode;

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
	CBuffer m_SessHash;
	int m_NeedKeyLen;
	int m_KexStrictStat;

	int m_DhMode;
	DH *m_SaveDh;
	int m_DhGexReqBits;

	int m_EcdhCurveNid;
	EC_KEY *m_EcdhClientKey;
	const EC_GROUP *m_EcdhGroup;

	EVP_PKEY *m_CurveEvpKey;
	CBuffer m_CurveClientPubkey;
	CBuffer m_SntrupClientKey;

	CBuffer m_RsaHostBlob;
	CBuffer m_RsaTranBlob;
	CBuffer m_RsaSharedKey;
	CBuffer m_RsaOaepSecret;

	int m_AuthStat;
	int m_AuthMode;
	CString m_AuthMeta;
	CString m_AuthLog;
	BOOL m_bAuthAgentReqEnable;

	CWordArray m_GlbReqMap;
	CWordArray m_OpnReqMap;
	CWordArray m_ChnReqMap;

	int m_OpenChanCount;

	CArray<CPermit, CPermit &> m_Permit;
	CStringArrayExt m_PortFwdTable;
	int m_bPfdConnect;

	//CRcpUpload m_RcpCmd;

	int m_ExtInfoStat;
	CStringIndex m_ExtInfo;

	int m_KeepAliveTiimerId;
	int m_KeepAliveSendCount;
	int m_KeepAliveReplyCount;
	int m_KeepAliveRecvGlobalCount;
	int m_KeepAliveRecvChannelCount;

	CredHandle m_SSPI_hCredential;
	CtxtHandle *m_SSPI_phContext;
	CString m_SSPI_TargetName;
	CtxtHandle m_SSPI_hNewContext;
	TimeStamp m_SSPI_tsExpiry;

	int m_LastFreeId;
	CPtrArray m_FifoCannel;
	CStringList m_FileList;
	CPtrList m_DelayedCheck;

public:
	Cssh(class CRLoginDoc *pDoc);
	virtual ~Cssh();
		
	virtual CFifoBase *FifoLinkMid();
	virtual CFifoDocument *FifoLinkRight();
	virtual BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	virtual void Close();

	virtual void OnRecvSocket(void *lpBuf, int nBufLen, int nFlags);
	virtual void OnTimer(UINT_PTR nIDEvent);

	virtual void SendWindSize();
	virtual void SendBreak(int opt = 0);
	virtual int GetRecvSize();

	virtual void GetStatus(CString &str);
	virtual void ResetOption();

public:
	CString m_WorkStr;
	CStringA m_WorkMbs;

	inline LPCTSTR LocalStr(LPCSTR str) { return m_pFifoMid->DocMsgLocalStr(str, m_WorkStr); }
	inline LPCSTR RemoteStr(LPCTSTR str) { return m_pFifoMid->DocMsgRemoteStr(str, m_WorkMbs); }

	void FifoCmdSendPacket(int type, CBuffer *bp);
	void PostSendPacket(int type, CBuffer *bp);

// SSH version 1
	void SendDisconnect(LPCSTR str);
	void SendPacket(CBuffer *bp);
	int SMsgPublicKey(CBuffer *bp);
	CIdKey *SelectIdKeyEntry();
	int SMsgAuthRsaChallenge(CBuffer *bp);
	void ReceivePacket(CBuffer *bp);

// SSH version 2
	void LogIt(LPCTSTR format, ...);
	void SendTextMsg(LPCSTR str, int len);
	int MatchList(LPCTSTR client, LPCTSTR server, CString &str);

	LPCTSTR GetSigAlgs();
	BOOL IsServerVersion(LPCTSTR pattan);
	void UserAuthNextState();
	void AddAuthLog(LPCTSTR str, ...);
	BOOL IsStriStr(LPCSTR str, LPCSTR ptn);

// Channel func
	inline int IdToFdIn(int id)  { return(id * 2 + 0); }
	inline int IdToFdOut(int id) { return(id * 2 + 1); }
	inline int FdToId(int nFd)   { return(nFd / 2); }
	inline int GetOpenChanCount() { return m_OpenChanCount; }

	void DelayedCheckExec();
	void ChannelCheck(int nFd, int fdEvent = 0, void *pParam = NULL);

	class CFifoChannel *GetFifoChannel(int id);
	BOOL ChannelCreate(int id, LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress, UINT nRemotePort);
	BOOL ChannelOpen(int id, LPCTSTR lpszHostAddress, UINT nHostPort);
	void ChannelClose(int id, int nStat = 0);
	void ChannelPolling(int id);
	void ChannelAccept(int id, SOCKET socket);
	void SocksResult(class CFifoSocks *pFifoSocks);

	void PortForward(BOOL bReset);
	void CancelPortForward();
	void OpenSFtpChannel();
	void OpenRcpUpload(LPCTSTR file);
	void OpenRcpDownload(LPCTSTR file);

// SSH ver2 SendMsg...
	void SendMsgKexInit();
	void SendMsgKexDhInit();
	void SendMsgKexDhGexRequest();
	int SendMsgKexEcdhInit();
	void SendMsgKexCurveInit();
	void SendMsgNewKeys();

	void SendMsgServiceRequest(LPCSTR str);
	int SendMsgUserAuthRequest(LPCSTR str);

	void PostSendMsgChannelOpen(int id, LPCSTR type);
	int SendMsgChannelOpen(int id, LPCSTR type, LPCTSTR lhost = NULL, int lport = 0, LPCTSTR rhost = NULL, int rport = 0);
	void SendMsgChannelOpenConfirmation(int id);
	void SendMsgChannelOpenFailure(int id);
	void SendMsgChannelData(int id);

	void SendMsgChannelRequesstShell(int id);
	void SendMsgChannelRequesstSubSystem(int id, LPCSTR cmds);
	void SendMsgChannelRequesstExec(int id, LPCSTR cmds);

	void SendMsgGlobalRequest(int num, LPCSTR str, LPCTSTR rhost = NULL, int rport = 0);
	void SendMsgKeepAlive();
	void SendMsgUnimplemented();
	void SendDisconnect2(int st, LPCSTR str);
	void SendMsgPong(LPCSTR msg);

	void SendPacket2(CBuffer *bp);

	int SSH2MsgKexInit(CBuffer *bp);
	int HostVerifyKey(CBuffer *sign, CBuffer *addb, CBuffer *skey, const EVP_MD *evp_md);
	int SSH2MsgKexDhReply(CBuffer *bp);
	int SSH2MsgKexDhGexGroup(CBuffer *bp);
	int SSH2MsgKexDhGexReply(CBuffer *bp);
	int SSH2MsgKexEcdhReply(CBuffer *bp);
	int SSH2MsgKexCurveReply(CBuffer *bp);
	int SSH2MsgKexRsaPubkey(CBuffer *bp);
	int SSH2MsgKexRsaDone(CBuffer *bp);

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
	int SSH2MsgChannelAdjust(CBuffer *bp);

	int SSH2MsgChannelRequesst(CBuffer *bp);
	int SSH2MsgChannelRequestReply(CBuffer *bp, int type);

	int SSH2MsgGlobalHostKeys(CBuffer *bp);
	int SSH2MsgGlobalRequest(CBuffer *bp);
	int SSH2MsgGlobalRequestReply(CBuffer *bp, int type);

	void ReceivePacket2(CBuffer *bp);
};

//////////////////////////////////////////////////////////////////////
// OpenSSH C lib
//////////////////////////////////////////////////////////////////////

extern int RLOGIN_provider_init(const OSSL_CORE_HANDLE *handle, const OSSL_DISPATCH *in, const OSSL_DISPATCH **out, void **provctx);
extern void RLOGIN_provider_finish();

extern const EVP_CIPHER *evp_aes_ctr(void);
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
extern int dh_estimate(int bits);
extern int key_ec_validate_public(const EC_GROUP *group, const EC_POINT *pub);

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

extern int HMAC_digest(const EVP_MD *md, BYTE *key, int keylen, BYTE *in, int inlen, BYTE *out, int outlen);
extern int MD_digest(const EVP_MD *md, BYTE *in, int inlen, BYTE *out, int outlen);

// openbsd-compat
extern int crypto_verify_32(const unsigned char *x,const unsigned char *y);
extern int bcrypt_pbkdf(const char *pass, size_t passlen, const unsigned char *salt, size_t saltlen, unsigned char *key, size_t keylen, unsigned int rounds);

// argon2
void argon2(uint32_t flavour, uint32_t mem, uint32_t passes, uint32_t parallel, CBuffer *P, CBuffer *S, CBuffer *K, CBuffer *X, u_char *out, uint32_t taglen);

// xmss.cpp
#define XMSS_OID_LEN 4

int xmss_str_to_oid(uint32_t *oid, const char *s);
int xmssmt_str_to_oid(uint32_t *oid, const char *s);

const char *xmss_oid_to_str(uint32_t oid);
const char *xmssmt_oid_to_str(uint32_t oid);

int xmss_keypair(unsigned char *pk, unsigned char *sk, const uint32_t oid, GENSTATUS *gs);
int xmss_sign(unsigned char *sk, unsigned char *sm, unsigned long long *smlen, const unsigned char *m, unsigned long long mlen);
int xmss_sign_open(unsigned char *m, unsigned long long *mlen, const unsigned char *sm, unsigned long long smlen, const unsigned char *pk);
int xmss_sign_bytes(uint32_t oid);
int xmss_bits(uint32_t oid);
int xmss_height(uint32_t oid);
int xmss_key_bytes(uint32_t oid, int *plen, int *slen);
int xmss_sk_bytes(uint32_t oid, int *ilen, int *slen);

int xmss_load_openssh_sk(uint32_t oid, unsigned char *sk, CBuffer *bp, CStringA *encname, CBuffer *ivbuf);
int xmss_save_openssh_sk(uint32_t oid, unsigned char *sk, CBuffer *bp, const char *encname, unsigned char *ivbuf, int ivlen);

int xmssmt_keypair(unsigned char *pk, unsigned char *sk, const uint32_t oid);
int xmssmt_sign(unsigned char *sk, unsigned char *sm, unsigned long long *smlen, const unsigned char *m, unsigned long long mlen);
int xmssmt_sign_open(unsigned char *m, unsigned long long *mlen, const unsigned char *sm, unsigned long long smlen, const unsigned char *pk);
int xmssmt_sign_bytes(uint32_t oid);
int xmssmt_bits(uint32_t oid);
int xmssmt_height(uint32_t oid);
int xmssmt_key_bytes(uint32_t oid, int *plen, int *slen);

// sntrup761.cpp
int	sntrup761_keypair(unsigned char *pk, unsigned char *sk);
int	sntrup761_enc(unsigned char *cstr, unsigned char *k, const unsigned char *pk);
int	sntrup761_dec(unsigned char *k, const unsigned char *cstr, const unsigned char *sk);

