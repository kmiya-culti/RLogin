// ExtSocket.h: CExtSocket クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EXTSOCKET_H__60F33E1D_5DAA_46A5_975E_01CB4B853E45__INCLUDED_)
#define AFX_EXTSOCKET_H__60F33E1D_5DAA_46A5_975E_01CB4B853E45__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxmt.h>
#include "Data.h"

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define	RECVMINSIZ		(m_RecvBufSize / 4)
#define	RECVBUFSIZ		(m_RecvBufSize    )
#define	RECVMAXSIZ		(m_RecvBufSize * 4)

#define	ESCT_DIRECT			0
#define	ESCT_RLOGIN			1
#define	ESCT_TELNET			2
#define	ESCT_SSH_MAIN		3
#define	ESCT_SSH_CHAN		4
#define	ESCT_COMDEV			5
#define	ESCT_PIPE			6

#define	ASYNC_GETADDR		000
#define	ASYNC_CREATE		001
#define	ASYNC_OPEN			002
#define	ASYNC_CREATEINFO	011
#define	ASYNC_OPENINFO		012

#define	SYNC_NONE			000
#define	SYNC_ACTIVE			001
#define	SYNC_EMPTY			010
#define	SYNC_EVENT			020

#define	RECV_ACTIVE			001
#define	RECV_DOCLOSE		002

#define	SEND_EMPTY			001

#define FD_RECIVE_EMPTY_BIT		(FD_MAX_EVENTS + 1)
#define FD_RECIVE_EMPTY			(1 << FD_RECIVE_EMPTY_BIT)
#define FD_SEND_EMPTY_BIT		(FD_MAX_EVENTS + 2)
#define FD_SEND_EMPTY			(1 << FD_SEND_EMPTY_BIT)

enum EProxyStat {
	PRST_NONE = 0,

	PRST_HTTP_START,
	PRST_HTTP_READLINE,
	PRST_HTTP_HEADCHECK,
	PRST_HTTP_BODYREAD,
	PRST_HTTP_CODECHECK,
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
	PRST_SOCKS5_CHAPCHECK
};

class CSockBuffer : public CObject
{
public:
	int			m_Type;
	int			m_Max;
	int			m_Ofs;
	int			m_Len;
	BYTE		*m_Data;
	CSockBuffer	*m_Left;
	CSockBuffer	*m_Right;

	void AddAlloc(int len);
	void Apend(LPBYTE buff, int len, int type);

	inline void Clear() { m_Len = m_Ofs = 0; }
	inline int GetSize() { return (m_Len - m_Ofs); }
	inline void AddSize(int len) { m_Len += len; }
	inline LPBYTE GetLast() { return (m_Data + m_Len); }
	inline LPBYTE GetPtr() { return (m_Data + m_Ofs); }
	inline int Consume(int len) { m_Ofs += len; return GetSize(); }
	inline int GetByte() { if ( m_Len <= m_Ofs ) return EOF; else return m_Data[m_Ofs++]; }

	CSockBuffer();
	~CSockBuffer();
};

#define	LISTENSOCKS		8

#define	DEBUGLOG(s, ...)

class CExtSocket : public CObject  
{
public:
	int m_Type;
	int m_ListenMax;
	int m_SocketEvent;
	SOCKET m_Fd, m_FdTab[LISTENSOCKS];
	class CRLoginDoc *m_pDocument;
	BOOL m_bConnect;

	CBuffer m_RecvBuff;
	CBuffer m_SendBuff;

private:
	int m_RecvSyncMode;
	int m_RecvBufSize;
	CSemaphore m_RecvSema;
	int m_OnRecvFlag;
	int m_OnSendFlag;
	BOOL m_DestroyFlag;
	CString m_RealHostAddr;
	CString m_RealRemoteAddr;
	UINT m_RealHostPort;
	UINT m_RealRemotePort;
	int m_RealSocketType;
	CEvent *m_pRecvEvent;

	int m_RecvSize;
	int m_SendSize;
	CSockBuffer *m_ProcHead;
	CSockBuffer *m_RecvHead;
	CSockBuffer *m_SendHead;
	CSockBuffer *m_FreeHead;
	CSemaphore m_FreeSema;

	int m_ProxyStatus;
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

	int m_SSL_mode;
	SSL_CTX *m_SSL_pCtx;
	SSL *m_SSL_pSock;

	CSockBuffer *AllocBuffer();
	void FreeBuffer(CSockBuffer *sp);
	CSockBuffer *AddTail(CSockBuffer *sp, CSockBuffer *head);
	CSockBuffer *AddHead(CSockBuffer *sp, CSockBuffer *head);
	CSockBuffer *RemoveHead(CSockBuffer *head);
	int ReciveCall();
	int ReciveProc();

	BOOL ProxyReadLine();
	BOOL ProxyReadBuff(int len);
	int ProxyFunc();
	int SSLConnect();
	void SSLClose();

#ifndef	NOIPV6
	ADDRINFOT *m_AddrInfoTop;
	ADDRINFOT *m_AddrInfoNext;

	int GetFamily();
	BOOL SyncOpenAddrInfo(UINT nSockPort);
	BOOL OpenAddrInfo();
#endif

public:
	virtual BOOL Create(LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress = NULL, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	virtual BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	virtual void Close();
	virtual void SetXonXoff(int sw);

	virtual int Recive(void *lpBuf, int nBufLen, int nFlags = 0);
	virtual int Send(const void *lpBuf, int nBufLen, int nFlags = 0);
	virtual void SendWindSize(int x, int y);
	virtual void SendBreak(int opt = 0);
	void SendFlash(int sec);

	virtual int GetRecvSize();
	virtual int GetSendSize();
	virtual void OnRecvEmpty();
	virtual void OnSendEmpty();
	virtual void GetStatus(CString &str);

	virtual void OnAsyncHostByName(int mode, LPCTSTR pHostName);
	virtual void OnGetHostByName(LPCTSTR pHostName);
	virtual void OnReciveCallBack(void *lpBuf, int nBufLen, int nFlags);
	virtual int OnReciveProcBack(void *lpBuf, int nBufLen, int nFlags);

	void OnPreConnect();
	void OnPreClose();
	virtual void OnError(int err);
	virtual void OnConnect();
	virtual void OnAccept(SOCKET hand);
	virtual void OnClose();
	virtual void OnRecive(int nFlags);
	virtual void OnSend();
	virtual int OnIdle();
	virtual void OnTimer(UINT_PTR nIDEvent);

	int AsyncGetHostByName(LPCTSTR pHostName);
	BOOL AsyncCreate(LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress = NULL, int nSocketType = SOCK_STREAM);
	virtual BOOL AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	BOOL ProxyOpen(int mode, LPCTSTR ProxyHost, UINT ProxyPort, LPCTSTR ProxyUser, LPCTSTR ProxyPass, LPCTSTR RealHost, UINT RealPort);

	int Accept(class CExtSocket *pSock, SOCKET hand);
	int Attach(class CRLoginDoc *pDoc, SOCKET fd);

	BOOL IOCtl(long lCommand, DWORD* lpArgument );
	BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel = SOL_SOCKET );
	BOOL GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel = SOL_SOCKET );

	static void GetPeerName(int fd, CString &host, int *port);
	static void GetSockName(int fd, CString &host, int *port);
	static void GetHostName(struct sockaddr *addr, int addrlen, CString &host);
	static int GetPortNum(LPCTSTR str);
	static BOOL SokcetCheck(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);

	void SetRecvSyncMode(BOOL mode);
	int SyncRecive(void* lpBuf, int nBufLen, int nSec, BOOL *pAbort);
	void SyncReciveBack(void *lpBuf, int nBufLen);
	void SyncAbort();

	inline int IsOpen() { return (m_Fd == (-1) ? FALSE  : TRUE); }
	inline BOOL IsOverFlow() { return (GetRecvSize() > RECVMAXSIZ ? TRUE : FALSE); }
	inline int GetLastError() { return 0; }
	inline class CRLoginDoc *GetDocument() { return m_pDocument; }
	inline class CMainFrame *GetMainWnd() { return (class CMainFrame *)AfxGetMainWnd(); }
	inline class CRLoginApp *GetApp() { return (class CRLoginApp *)AfxGetApp(); }
	inline void SetRecvBufSize(int size) { m_RecvBufSize = size; }

	void Destroy();

	CExtSocket(class CRLoginDoc *pDoc);
	virtual ~CExtSocket();

#ifndef DEBUGLOG
	void DEBUGLOG(LPCSTR str, ...);
#endif
};

#endif // !defined(AFX_EXTSOCKET_H__60F33E1D_5DAA_46A5_975E_01CB4B853E45__INCLUDED_)
