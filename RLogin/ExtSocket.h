//////////////////////////////////////////////////////////////////////
// ExtSocket.h : インターフェイス
//

#pragma once

#include <afxmt.h>
#include "Data.h"
#include "Fifo.h"

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/quic.h>

#define	RECVDEFSIZ			(2 * 1024)

#define	ESCT_DIRECT			0
#define	ESCT_RLOGIN			1
#define	ESCT_TELNET			2
#define	ESCT_COMDEV			3
#define	ESCT_PIPE			4
#define	ESCT_SSH_MAIN		5
#define	ESCT_SSH_CHAN		6

#ifdef	USE_DEBUGLOG
#define	DEBUGLOG			m_pDocument->LogDebug
#define	DEBUGDUMP			m_pDocument->LogDump
#else
#define	DEBUGLOG(f,...)
#define	DEBUGDUMP(p,s)
#endif

enum EProxyStat {
	PRST_NONE = 0,

	PRST_HTTP_START,
	PRST_HTTP_READLINE,
	PRST_HTTP_HEADCHECK,
	PRST_HTTP_BODYREAD,
	PRST_HTTP_CODECHECK,
	PRST_HTTP_ERROR,
	PRST_HTTP_BASIC_START,
	PRST_HTTP_BASIC,
	PRST_HTTP_DIGEST,

	PRST_SOCKS4_START,
	PRST_SOCKS4_READMSG,
	PRST_SOCKS4_CHECKMSG,
	PRST_SOCKS4_ENDOF,

	PRST_SOCKS5_START,
	PRST_SOCKS5_READMSG,
	PRST_SOCKS5_SENDAUTH,
	PRST_SOCKS5_READAUTH,
	PRST_SOCKS5_SENDCONNECT,
	PRST_SOCKS5_READCONNECT,
	PRST_SOCKS5_READSTAT,
	PRST_SOCKS5_CONNECT,
	PRST_SOCKS5_ERROR,
	PRST_SOCKS5_CHAPSTART,
	PRST_SOCKS5_CHAPMSG,
	PRST_SOCKS5_CHAPCODE,
	PRST_SOCKS5_CHAPSTAT,
	PRST_SOCKS5_CHAPCHECK,

	PRST_HTTP2_START,
	PRST_HTTP2_TUNNEL,
	PRST_HTTP2_REQUEST,
	PRST_HTTP2_CONNECT,
	PRST_HTTP2_BASIC,
	PRST_HTTP2_DIGEST,

	PRST_HTTP3_START,
	PRST_HTTP3_TUNNEL,
	PRST_HTTP3_CONNECT,
	PRST_HTTP3_BASIC,
	PRST_HTTP3_DIGEST,
};

class CExtSocket : public CObject  
{
public:
	int m_Type;
	class CRLoginDoc *m_pDocument;

	CFifoBase *m_pFifoLeft;
	CFifoBase *m_pFifoMid;
	CFifoDocument *m_pFifoRight;

	CFifoProxy *m_pFifoProxy;
	CFifoSSL *m_pFifoSSL;
	CFifoSync *m_pFifoSync;

	SOCKET m_Fd;
	BOOL m_bConnect;

	int m_RecvBufSize;

	CString m_RealHostAddr;
	UINT m_RealHostPort;
	UINT m_RealRemotePort;
	int m_RealSocketType;

	int m_ProxyStatus;
	int m_ProxyLastError;
	CString m_ProxyHostAddr;
	CString m_ProxyHost;
	int m_ProxyPort;
	CString m_ProxyUser;
	CString m_ProxyPass;
	CString m_ProxyStr;
	CStringArray m_ProxyResult;
	int m_ProxyCode;
	int m_ProxyLength;
	int m_ProxyNum;
	CStringIndex m_ProxyAuth;
	BOOL m_ProxyConnect;
	CBuffer m_ProxyBuff;
	BOOL m_ProxyCmdMode;
	class CExtSocket *m_pSshProxy;

	class CHttp2Ctx *m_pHttp2Ctx;
	class CHttp3Ctx *m_pHttp3Ctx;

	int m_SSL_mode;
	SSL_CTX *m_SSL_pCtx;
	SSL *m_SSL_pSock;
	BOOL m_SSL_keep;
	CString m_SSL_Msg;
	CStringA m_SSL_alpn;

public:
	CExtSocket(class CRLoginDoc *pDoc);
	virtual ~CExtSocket();

	void Destroy();
	void PostClose(int nLastError);

	virtual CFifoBase *FifoLinkLeft();
	virtual CFifoBase *FifoLinkMid();
	virtual CFifoDocument *FifoLinkRight();

	virtual void FifoLink();
	virtual void FifoUnlink();

	virtual BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	virtual void Close();

	virtual void SendSocket(const void *lpBuf, int nBufLen, int nFlags = 0);
	virtual void OnRecvSocket(void *lpBuf, int nBufLen, int nFlags);
	virtual void SendProtocol(const void *lpBuf, int nBufLen, int nFlags = 0);
	virtual void OnRecvProtocol(void *lpBuf, int nBufLen, int nFlags);
	virtual void SendDocument(const void *lpBuf, int nBufLen, int nFlags = 0);
	virtual int OnRecvDocument(void *lpBuf, int nBufLen, int nFlags);

	virtual void GetStatus(CString &str);
	virtual void ResetOption();
	virtual void SetXonXoff(int sw);
	virtual void SendWindSize();
	virtual void SendBreak(int opt = 0);

	virtual int GetRecvSize();
	virtual int GetSendSize();
	virtual int GetRecvProcSize();
	virtual int GetSendProcSize();
	virtual int GetRecvSocketSize();
	virtual int GetSendSocketSize();

	virtual void OnConnect();
	virtual void OnClose();
	virtual void OnError(int err);
	virtual void OnAccept(SOCKET hand);

	virtual void OnTimer(UINT_PTR nIDEvent);

	int GetFamily();
	BOOL ProxyOpen(int mode, BOOL keep, LPCTSTR ProxyHost, UINT ProxyPort, LPCTSTR ProxyUser, LPCTSTR ProxyPass, LPCTSTR RealHost, UINT RealPort);
	BOOL ProxyCommand(LPCTSTR pCommand);

	void SetRecvSyncMode(BOOL mode);
	void SyncRecvClear();
	int SyncReceive(void* lpBuf, int nBufLen, int mSec, BOOL *pAbort);
	void SyncReceiveBack(void *lpBuf, int nBufLen);
	int SyncSend(const void *lpBuf, int nBufLen, int mSec, BOOL *pAbort);
	int SyncExtSend(const void *lpBuf, int nBufLen, int mSec, BOOL *pAbort);
	void SyncAbort();

	BOOL SSLInit();
	int SSLCheck();
	void SSLClose(BOOL bUnLink = FALSE);

	BOOL ProxyCheck();
	BOOL ProxyReadLine();
	BOOL ProxyReadBuff(int len);
	BOOL ProxyMakeDigest(CString &digest);
	BOOL ProxyReOpen();
	BOOL ProxyFunc();

	BOOL IOCtl(long lCommand, DWORD* lpArgument );
	BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel = SOL_SOCKET );
	BOOL GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel = SOL_SOCKET );

	static BIO_ADDR *GetPeerBioAddr(SOCKET fd);
	static void GetPeerName(SOCKET fd, CString &host, int *port);
	static void GetSockName(SOCKET fd, CString &host, int *port);
	static void GetHostName(struct sockaddr *addr, int addrlen, CString &host);
	static int GetPortNum(LPCTSTR str);
	static BOOL SokcetCheck(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	static void PunyCodeEncode(LPCWSTR str, CString &out);
	static void PunyCodeAdress(LPCTSTR str, CString &out);
	static LPCTSTR GetFormatErrorMessage(LPCTSTR entry, LPCTSTR host, int port, LPCTSTR type, int err);

	inline int IsOpen() { return (m_Fd == (-1) ? FALSE  : TRUE); }
	inline class CRLoginDoc *GetDocument() { return m_pDocument; }
	inline class CMainFrame *GetMainWnd() { return (class CMainFrame *)AfxGetMainWnd(); }
	inline class CRLoginApp *GetApp() { return (class CRLoginApp *)AfxGetApp(); }
	inline void SetRecvBufSize(int size) { m_RecvBufSize = size; }
	inline BOOL IsSyncMode() { return (m_pFifoSync != NULL ? TRUE : FALSE); }

	CString m_WorkStr;
	CStringA m_WorkMbs;

	inline LPCTSTR LocalStr(LPCSTR str) { return m_pFifoMid->DocMsgLocalStr(str, m_WorkStr); }
	inline LPCSTR RemoteStr(LPCTSTR str) { return m_pFifoMid->DocMsgRemoteStr(str, m_WorkMbs); }

#ifdef	USE_FIFOMONITER
	void FifoMoniter(int nList[10]);
#endif
};
