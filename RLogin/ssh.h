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

#define SSH2_CIPHER_3DES		10		// 3des-cbc
#define SSH2_CIPHER_BLOWFISH	11		// blowfish-cbc
#define SSH2_CIPHER_CAST128		12		// cast128-cbc
#define SSH2_CIPHER_ARCFOUR		13		// arcfour
#define SSH2_CIPHER_ARC128		14		// arcfour128
#define SSH2_CIPHER_ARC256		15		// arcfour256
#define SSH2_CIPHER_AES128		16		// aes128-cbc
#define SSH2_CIPHER_AES192		17		// aes192-cbc
#define SSH2_CIPHER_AES256		18		// aes256-cbc
#define SSH2_CIPHER_AES128R		19		// aes128-ctr
#define SSH2_CIPHER_AES192R		20		// aes192-ctr
#define SSH2_CIPHER_AES256R		21		// aes256-ctr

#define	COMPLEVEL		6

#define	MODE_ENC		0
#define	MODE_DEC		1

#define INTBLOB_LEN     20
#define SIGBLOB_LEN     (2*INTBLOB_LEN)

#define	DHMODE_GROUP_1		0
#define	DHMODE_GROUP_14		1
#define	DHMODE_GROUP_GEX	2
#define	DHMODE_GROUP_GEX256	3

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
	LPCSTR GetMatchList(LPCSTR str);
	LPCSTR GetTitle();
	void MakeKey(CBuffer *bp, LPCSTR pass);

	CCipher();
	~CCipher();
};

class CMacomp: public CObject
{
public:
	int m_Index;
	const EVP_MD *m_Md;
	int m_KeyLen;
	LPBYTE m_KeyBuf;
	int m_BlockSize;

	int Init(LPCSTR name, LPBYTE key = NULL, int len = (-1));
	void Compute(int seq, LPBYTE inbuf, int len, CBuffer *out);
	int GetIndex(LPCSTR name);
	int GetKeyLen(LPCSTR name = NULL);
	int GetBlockSize(LPCSTR name = NULL);
	LPCSTR GetMatchList(LPCSTR str);
	LPCSTR GetTitle();

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
	LPCSTR GetMatchList(LPCSTR str);
	LPCSTR GetTitle();

	CCompress();
	~CCompress();
};

#define	IDKEY_NONE	000
#define	IDKEY_RSA1	001
#define	IDKEY_RSA2	002
#define	IDKEY_DSA2	004

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
	int Sign(CBuffer *bp, LPBYTE buf, int len);

	int RsaVerify(CBuffer *bp, LPBYTE data, int datalen);
	int DssVerify(CBuffer *bp, LPBYTE data, int datalen);
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

#define CHAN_SES_PACKET_DEFAULT (32 * 1024)
#define CHAN_SES_WINDOW_DEFAULT (16 * CHAN_SES_PACKET_DEFAULT)

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

	int m_SSHVer;
	CString m_HostName;
	int m_StdChan;

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

	int m_SendPackSeq;
	int m_RecvPackSeq;
	CBuffer m_InPackBuf;
	BYTE m_Cookie[16];
	CString m_SProp[10], m_CProp[10], m_VProp[6];
	LPBYTE m_VKey[6];
	CBuffer m_HisPeer;
	CBuffer m_MyPeer;
	int m_NeedKeyLen;
	int m_DhMode;
	DH *m_SaveDh;
	int m_AuthStat;
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

	void SendMsgNewKeys();
	void SendMsgKexDhInit();
	void SendMsgKexDhGexRequest();

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
	int SSH2MsgNewKeys(CBuffer *bp);

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

#endif // !defined(AFX_SSH_H__2A682FAC_4F24_4168_9082_C9CDF2DD19D7__INCLUDED_)
