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
#include "openssl/des.h"
#include "openssl/bn.h"
#include "openssl/evp.h"
#include "openssl/err.h"
#include "openssl/hmac.h"
#include "openssl/md5.h"
#include "openssl/pem.h"
#include "zlib.h"

#define SSH_CIPHER_NONE         0       // none
#define SSH_CIPHER_DES          2       // des
#define SSH_CIPHER_3DES         3       // 3des
#define SSH_CIPHER_BLOWFISH     6		// blowfish

#define SSH2_CIPHER_3DES_CBC	10		// 3des-cbc
#define SSH2_CIPHER_3DES_ECB	11		// 3des-ecb
#define SSH2_CIPHER_3DES_CFB	12		// 3des-cfb
#define SSH2_CIPHER_3DES_OFB	13		// 3des-ofb
#define SSH2_CIPHER_3DES_CTR	14		// 3des-ctr

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

#define	COMPLEVEL		6

#define	MODE_ENC		0
#define	MODE_DEC		1

#define INTBLOB_LEN     20
#define SIGBLOB_LEN     (2*INTBLOB_LEN)

#define	DHMODE_GROUP_1		0
#define	DHMODE_GROUP_14		1
#define	DHMODE_GROUP_GEX	2
#define	DHMODE_GROUP_GEX256	3
#define	DHMODE_ECDH_S2_N256	4
#define	DHMODE_ECDH_S2_N384	5
#define	DHMODE_ECDH_S2_N521	6

class CCipher: public CObject
{
public:
	int m_Index;
	LPBYTE m_IvBuf;
	LPBYTE m_KeyBuf;
	EVP_CIPHER_CTX m_Evp;

	int Init(LPCSTR name, int mode, LPBYTE key = NULL, int len = (-1), LPBYTE iv = NULL);
	void Cipher(LPBYTE inbuf, int len, CBuffer *out);
	int GetIndex(LPCSTR name);
	int GetKeyLen(LPCSTR name = NULL);
	int GetBlockSize(LPCSTR name = NULL);
	LPCSTR GetName(int num);
	int GetNum(LPCSTR str);
	LPCSTR GetTitle();
	void MakeKey(CBuffer *bp, LPCSTR pass);

	static void CCipher::BenchMark(CString &out);

	CCipher();
	~CCipher();
};

class CMacomp: public CObject
{
public:
	int m_Index;
	const EVP_MD *m_Md;
	HMAC_CTX m_Ctx;
	int m_KeyLen;
	LPBYTE m_KeyBuf;
	int m_BlockSize;
	void *m_UmacCtx;
	BOOL m_UmacMode;

	int Init(LPCSTR name, LPBYTE key = NULL, int len = (-1));
	void Compute(DWORD seq, LPBYTE inbuf, int len, CBuffer *out);
	int GetIndex(LPCSTR name);
	int GetKeyLen(LPCSTR name = NULL);
	int GetBlockSize(LPCSTR name = NULL);
	LPCSTR GetTitle();

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

	int Init(LPCSTR name, int mode, int level = 6);
	void Compress(LPBYTE inbuf, int len, CBuffer *out);
	void UnCompress(LPBYTE inbuf, int len, CBuffer *out);
	LPCSTR GetTitle();

	CCompress();
	~CCompress();
};

#define	IDKEY_NONE		000
#define	IDKEY_RSA1		001
#define	IDKEY_RSA2		002
#define	IDKEY_DSA2		004
#define	IDKEY_ECDSA		040

class CIdKey: public CObject
{
public:
	int m_Uid;
	CString m_Name;
	CString m_Pass;
	int m_Type;
	RSA *m_Rsa;
	DSA *m_Dsa;
	BOOL m_Flag;
	int	 m_EcNid;
	EC_KEY *m_EcDsa;

	int Create(int type);
	int Generate(int type, int bits);
	int Close();
	int Compere(CIdKey *pKey);

	void GetUserHostName(CString &str);
	void MakeKey(CBuffer *bp, LPCSTR pass);
	void Decrypt(CBuffer *bp, LPCSTR str, LPCSTR pass = NULL);
	void Encrypt(CString &str, LPBYTE buf, int len, LPCSTR pass = NULL);
	void DecryptStr(CString &out, LPCSTR str);
	void EncryptStr(CString &out, LPCSTR str);

	LPCSTR GetName();
	int GetTypeFromName(LPCSTR name);
	int HostVerify(LPCSTR host);

	int RsaSign(CBuffer *bp, LPBYTE buf, int len);
	int DssSign(CBuffer *bp, LPBYTE buf, int len);
	int EcDsaSign(CBuffer *bp, LPBYTE buf, int len);
	int Sign(CBuffer *bp, LPBYTE buf, int len);

	int RsaVerify(CBuffer *bp, LPBYTE data, int datalen);
	int DssVerify(CBuffer *bp, LPBYTE data, int datalen);
	int EcDsaVerify(CBuffer *bp, LPBYTE data, int datalen);
	int Verify(CBuffer *bp, LPBYTE data, int datalen);

	int GetBlob(CBuffer *bp);
	int SetBlob(CBuffer *bp);

	int ReadPublicKey(LPCSTR str);
	int WritePublicKey(CString &str);

	int ReadPrivateKey(LPCSTR str, LPCSTR pass);
	int WritePrivateKey(CString &str, LPCSTR pass);

	int LoadRsa1Key(FILE *fp, LPCSTR pass);
	int SaveRsa1Key(FILE *fp, LPCSTR pass);

	int LoadPrivateKey(LPCSTR file, LPCSTR pass);
	int SavePrivateKey(int type, LPCSTR file, LPCSTR pass);

	int GetString(LPCSTR str);
	void SetString(CString &str);
	int GetProfile(LPCSTR pSection, int Uid);
	void SetProfile(LPCSTR pSection);
	void DelProfile(LPCSTR pSection);
	const CIdKey & operator = (CIdKey &data);

	int GetSize();
	void FingerPrint(CString &str);

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
	int AddEntry(CIdKey &key);
	CIdKey *GetUid(int uid);
	void UpdateUid(int uid);
	void RemoveUid(int uid);

	void Init();
	void GetArray(CStringArrayExt &array);
	void SetArray(CStringArrayExt &array);
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
#define	CHAN_OK(n)		(m_Chan[n].m_Status == (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE))

#define	CEOF_IEOF		0001
#define	CEOF_ICLOSE		0002
#define	CEOF_SEOF		0004
#define	CEOF_SCLOSE		0010
#define	CEOF_DEAD		0020
#define	CEOF_IEMPY		0040
#define	CEOF_OEMPY		0100

class CFilter : public CObject
{
public:
	int m_Type;
	class CChannel *m_pChan;
	class CFilter *m_pNext;
	CString m_Cmd;

	CFilter();
	virtual ~CFilter();
	virtual void Destroy();
	virtual void Close();
	virtual void OnConnect();
	virtual void Send(LPBYTE buf, int len);
	virtual void OnRecive(const void *lpBuf, int nBufLen);
	virtual void OnSendEmpty();
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
	int m_Next;
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
	void OnReciveCallBack(void *lpBuf, int nBufLen, int nFlags);
	int OnReciveProcBack(void *lpBuf, int nBufLen, int nFlags);
	int GetSendSize();
	int GetRecvSize();
	void OnRecvEmpty();
	void OnSendEmpty();

	void DoClose();
	int Create(LPCTSTR lpszHostAddress, UINT nHostPort, LPCSTR lpszRemoteAddress, UINT nRemotePort);
};

class CStdIoFilter : public CFilter
{
public:
	CStdIoFilter();
	void OnRecive(const void *lpBuf, int nBufLen);
	int GetSendSize();
};

class CSFtpFilter : public CFilter
{
public:
	class CSFtp *m_pSFtp;

	CSFtpFilter();
	void Destroy();
	void OnConnect();
	void OnRecive(const void *lpBuf, int nBufLen);
};

class CAgent : public CFilter
{
public:
	CBuffer m_RecvBuf;

	CAgent();
	void OnRecive(const void *lpBuf, int nBufLen);

	CIdKey *GetIdKey(CIdKey *key);
	void ReciveBuffer(CBuffer *bp);
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
	void OnRecive(const void *lpBuf, int nBufLen);
	void OnClose();
	void OnSendEmpty();

	void HostKanjiStr(LPCSTR str, CString &tmp);

	CRcpUpload();
	~CRcpUpload();
};

class Cssh : public CExtSocket 
{
public:
	Cssh(class CRLoginDoc *pDoc);
	virtual ~Cssh();

	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	void Close();
	void OnReciveCallBack(void* lpBuf, int nBufLen, int nFlags = 0);
	int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	void SendWindSize(int x, int y);
	void OnTimer(UINT_PTR nIDEvent);
	void OnRecvEmpty();
	void OnSendEmpty();
	void GetStatus(CString &str);

	int m_SSHVer;
	CString m_HostName;
	int m_StdChan;
	CFilter *m_pListFilter;

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
	CIdKey m_IdKey;
	int m_IdKeyPos;
	CIdKey m_HostKey;

	int m_ServerFlag;
	int m_SupportCipher;
	int m_SupportAuth;
	int m_CipherMode;
	int m_CipTabMax;
	int m_CipTab[8];
	int m_CompMode;

	int SMsgPublicKey(CBuffer *bp);
	int SMsgAuthRsaChallenge(CBuffer *bp);
	void RecivePacket(CBuffer *bp);
	void SendPacket(CBuffer *bp);
	void SendDisconnect(LPCSTR str);

	int m_SSH2Status;
#define	SSH2_STAT_HAVEPROP		0001
#define	SSH2_STAT_HAVEKEYS		0002
#define	SSH2_STAT_HAVESESS		0004
#define	SSH2_STAT_HAVELOGIN		0010
#define	SSH2_STAT_HAVESTDIO		0020
#define	SSH2_STAT_HAVESHELL		0040
#define	SSH2_STAT_HAVEPFWD		0100
#define	SSH2_STAT_HAVEAGENT		0200
#define	SSH2_STAT_SENTKEXINIT	0400

	DWORD m_SendPackSeq;
	DWORD m_RecvPackSeq;
	DWORD m_SendPackLen;
	DWORD m_RecvPackLen;
	CBuffer m_InPackBuf;
	BYTE m_Cookie[16];
	CString m_SProp[10], m_CProp[10], m_VProp[6];
	LPBYTE m_VKey[6];
	CBuffer m_HisPeer;
	CBuffer m_MyPeer;
	int m_NeedKeyLen;
	int m_DhMode;
	DH *m_SaveDh;
	int m_EcdhCurveNid;
	EC_KEY *m_EcdhClientKey;
	const EC_GROUP *m_EcdhGroup;
	int m_AuthStat;
	int m_AuthMode;
	CWordArray m_GlbReqMap;
	CWordArray m_OpnReqMap;
	CWordArray m_ChnReqMap;
	int m_ChanNext;
	int m_ChanFree;
	int m_ChanFree2;
	int m_OpenChanCount;
	CArray<CChannel, CChannel &> m_Chan;
	CArray<CPermit, CPermit &> m_Permit;
	CBuffer m_StdInRes;
	CRcpUpload m_RcpCmd;

	void LogIt(LPCSTR format, ...);
	void PortForward();
	int MatchList(LPCSTR client, LPCSTR server, CString &str);
	int ChannelOpen();
	void ChannelClose(int id);
	int SetIdKeyList();

	void SendMsgKexInit();
	void SendMsgNewKeys();
	void SendMsgKexDhInit();
	void SendMsgKexDhGexRequest();
	int SendMsgKexEcdhInit();

	void SendMsgServiceRequest(LPCSTR str);
	int SendMsgUserAuthRequest(LPCSTR str);

	int SendMsgChannelOpen(int n, LPCSTR type, LPCSTR lhost = NULL, int lport = 0, LPCSTR rhost = NULL, int rport = 0);
	void SendMsgChannelRequesstShell(int id);
	void SendMsgChannelRequesstSubSystem(int id);
	void SendMsgChannelRequesstExec(int id);

	void SendMsgGlobalRequest(int num, LPCSTR str, LPCSTR rhost = NULL, int rport = 0);
	void SendMsgKeepAlive();
	void SendMsgUnimplemented();
	void SendDisconnect2(int st, LPCSTR str);

	int SSH2MsgKexInit(CBuffer *bp);
	int SSH2MsgKexDhReply(CBuffer *bp);
	int SSH2MsgKexDhGexGroup(CBuffer *bp);
	int SSH2MsgKexDhGexReply(CBuffer *bp);
	int SSH2MsgKexEcdhReply(CBuffer *bp);
	int SSH2MsgNewKeys(CBuffer *bp);

	int SSH2MsgUserAuthPkOk(CBuffer *bp);
	int SSH2MsgUserAuthPasswdChangeReq(CBuffer *bp);
	int SSH2MsgUserAuthInfoRequest(CBuffer *bp);

	int SSH2MsgChannelOpen(CBuffer *bp);
	int SSH2MsgChannelOpenReply(CBuffer *bp, int type);
	int SSH2MsgChannelClose(CBuffer *bp);
	int SSH2MsgChannelData(CBuffer *bp, int type);
	int SSH2MsgChannelEof(CBuffer *bp);
	int SSH2MsgChannelRequesst(CBuffer *bp);
	int SSH2MsgChannelAdjust(CBuffer *bp);
	int SSH2MsgChannelRequestReply(CBuffer *bp, int type);

	int SSH2MsgGlobalRequest(CBuffer *bp);
	int SSH2MsgGlobalRequestReply(CBuffer *bp, int type);

	void RecivePacket2(CBuffer *bp);
	void SendPacket2(CBuffer *bp);

public:
	int GetOpenChanCount() { return m_OpenChanCount; }
	void OpenSFtpChannel();
	void SendMsgChannelData(int id);
	void SendMsgChannelOpenConfirmation(int id);
	void SendMsgChannelOpenFailure(int id);
	void DecodeProxySocks(int id);
	void ChannelCheck(int n);
	void ChannelPolling(int id);
	void ChannelAccept(int id, SOCKET hand);
	void OpenRcpUpload(LPCSTR file);
};

//////////////////////////////////////////////////////////////////////
// OpenSSH C lib
//////////////////////////////////////////////////////////////////////

extern const EVP_CIPHER *evp_aes_128_ctr(void);
extern const EVP_CIPHER *evp_camellia_128_ctr(void);
extern const EVP_CIPHER *evp_seed_ctr(void);
extern const EVP_CIPHER *evp_twofish_ctr(void);
extern const EVP_CIPHER *evp_twofish_cbc(void);
extern const EVP_CIPHER *evp_serpent_ctr(void);
extern const EVP_CIPHER *evp_serpent_cbc(void);
extern const EVP_CIPHER *evp_bf_ctr(void);
extern const EVP_CIPHER *evp_cast5_ctr(void);
extern const EVP_CIPHER *evp_idea_ctr(void);
extern const EVP_CIPHER *evp_des3_ctr(void);
extern const EVP_CIPHER *evp_ssh1_3des(void);
extern const EVP_CIPHER *evp_ssh1_bf(void);

extern void rsa_public_encrypt(BIGNUM *out, BIGNUM *in, RSA *key);
extern int	ssh_crc32(LPBYTE buf, int len);
extern int dh_pub_is_valid(DH *dh, BIGNUM *dh_pub);
extern int dh_gen_key(DH *dh, int need);
extern DH *dh_new_group_asc(const char *gen, const char *modulus);
extern DH *dh_new_group1(void);
extern DH *dh_new_group14(void);
extern u_char *kex_dh_hash(LPCSTR client_version_string, LPCSTR server_version_string, LPBYTE ckexinit, int ckexinitlen, LPBYTE skexinit, int skexinitlen, LPBYTE serverhostkeyblob, int sbloblen, BIGNUM *client_dh_pub, BIGNUM *server_dh_pub, BIGNUM *shared_secret);
extern int	dh_estimate(int bits);
extern u_char *kex_gex_hash(LPCSTR client_version_string, LPCSTR server_version_string, LPBYTE ckexinit, int ckexinitlen, LPBYTE skexinit, int skexinitlen, LPBYTE serverhostkeyblob, int sbloblen, int min, int wantbits, int max, BIGNUM *prime, BIGNUM *gen, BIGNUM *client_dh_pub, BIGNUM *server_dh_pub, BIGNUM *shared_secret, int *hashlen, const EVP_MD *evp_md);
extern int key_ec_validate_public(const EC_GROUP *group, const EC_POINT *pub);
extern u_char *kex_ecdh_hash(const EVP_MD *evp_md, const EC_GROUP *ec_group, LPCSTR client_version_string, LPCSTR server_version_string, LPBYTE ckexinit, int ckexinitlen, LPBYTE skexinit, int skexinitlen, LPBYTE serverhostkeyblob, int sbloblen, const EC_POINT *client_dh_pub, const EC_POINT *server_dh_pub, BIGNUM *shared_secret, int *hashlen);
extern u_char *derive_key(int id, int need, u_char *hash, int hashlen, BIGNUM *shared_secret, u_char *session_id, int sesslen, const EVP_MD *evp_md);

extern void *mm_zalloc(void *mm, unsigned int ncount, unsigned int size);
extern void mm_zfree(void *mm, void *address);

extern struct umac_ctx *UMAC_open(int len);
extern void UMAC_init(struct umac_ctx *ctx, u_char *key, int len);
extern void UMAC_update(struct umac_ctx *ctx, const u_char *input, size_t len);
extern void UMAC_final(struct umac_ctx *ctx, u_char *tag, u_char *nonce);
extern void UMAC_close(struct umac_ctx *ctx);

#endif // !defined(AFX_SSH_H__2A682FAC_4F24_4168_9082_C9CDF2DD19D7__INCLUDED_)
