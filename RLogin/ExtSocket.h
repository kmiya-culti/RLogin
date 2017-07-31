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

#define	RECVMINSIZ	(2 * 1024)
#define	RECVBUFSIZ	(32 * 1024)
#define	RECVMAXSIZ	(128 * 1024)

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

	void Alloc(int len);
	inline void Clear() { m_Len = m_Ofs = 0; }
	inline int GetSize() { return (m_Len - m_Ofs); }
	inline void SetSize(int len) { m_Len += len; }
	inline LPBYTE GetLast() { return (m_Data + m_Len); }
	inline LPBYTE GetPtr() { return (m_Data + m_Ofs); }
	inline int Consume(int len) { m_Ofs += len; return GetSize(); }
	inline int GetByte() { if ( m_Len <= m_Ofs ) return EOF; else return m_Data[m_Ofs++]; }

	CSockBuffer();
	~CSockBuffer();
};

class CExtSocket : public CObject  
{
public:
	int m_Type;
	SOCKET m_Fd, m_Fdv6;
	class CRLoginDoc *m_pDocument;

	CBuffer m_RecvBuff;
	CBuffer m_SendBuff;

private:
	int m_SocketEvent;
	int m_RecvOverSize;
	int m_RecvSyncMode;
	CSemaphore m_RecvSema;
	int m_OnRecvFlag;
	int m_OnSendFlag;
	BOOL m_DestroyFlag;
	int m_AsyncMode;
	CString m_RealHostAddr;
	CString m_RealRemoteAddr;
	UINT m_RealHostPort;
	UINT m_RealRemotePort;
	int m_RealSocketType;
	CEvent *m_pRecvEvent;

	int m_ProcSize[2];
	int m_RecvSize[2];
	int m_SendSize[2];
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

public:
	virtual BOOL Create(LPCTSTR lpszHostAddress, UINT nHostPort, LPCSTR lpszRemoteAddress = NULL, int nSocketType = SOCK_STREAM);
	virtual BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	virtual void Close();
	virtual void SetXonXoff(int sw);

	virtual int Recive(void *lpBuf, int nBufLen, int nFlags = 0);
	virtual int Send(const void *lpBuf, int nBufLen, int nFlags = 0);
	virtual void SendWindSize(int x, int y);
	virtual void SendBreak(int opt = 0);

	virtual int GetRecvSize();
	virtual int GetSendSize();
	virtual void OnRecvEmpty();
	virtual void OnSendEmpty();
	virtual void GetStatus(CString &str);

	virtual void OnAsyncHostByName(LPCSTR pHostName);
	virtual void OnGetHostByName(LPCSTR pHostName);
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

	int AsyncGetHostByName(LPCSTR pHostName);
	BOOL AsyncCreate(LPCTSTR lpszHostAddress, UINT nHostPort, LPCSTR lpszRemoteAddress = NULL, int nSocketType = SOCK_STREAM);
	virtual BOOL AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	BOOL ProxyOpen(int mode, LPCSTR ProxyHost, UINT ProxyPort, LPCSTR ProxyUser, LPCSTR ProxyPass, LPCSTR RealHost, UINT RealPort);

	int Accept(class CExtSocket *pSock, SOCKET hand);
	int Attach(class CRLoginDoc *pDoc, SOCKET fd);

	BOOL IOCtl(long lCommand, DWORD* lpArgument );
	BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel = SOL_SOCKET );
	BOOL GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel = SOL_SOCKET );

	static void GetPeerName(int fd, CString &host, int *port);
	static void GetHostName(int af, void *addr, CString &host);
	static int GetPortNum(LPCSTR str);
	static BOOL SokcetCheck(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);

	void SetRecvSyncMode(BOOL mode);
	int SyncRecive(void* lpBuf, int nBufLen, int nSec, BOOL *pAbort);
	void SyncReciveBack(void *lpBuf, int nBufLen);
	int SyncRecvSize();
	void SyncAbort();

	inline int IsOpen() { return (m_Fd == (-1) ? FALSE  : TRUE); }
	inline int GetLastError() { return 0; }
	inline void SetRecvSize(int size) { m_RecvOverSize = size; }
	inline class CRLoginDoc *GetDocument() { return m_pDocument; }
	inline class CMainFrame *GetMainWnd() { return (class CMainFrame *)AfxGetMainWnd(); }
	inline class CRLoginApp *GetApp() { return (class CRLoginApp *)AfxGetApp(); }

	void Destroy();
	CExtSocket(class CRLoginDoc *pDoc);
	virtual ~CExtSocket();
};

#endif // !defined(AFX_EXTSOCKET_H__60F33E1D_5DAA_46A5_975E_01CB4B853E45__INCLUDED_)
