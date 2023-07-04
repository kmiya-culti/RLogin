///////////////////////////////////////////////////////
// Fifo.h : インターフェイス
//

#pragma once

#define	FIFO_BUFDEFMAX			(8 * 1024)
#define	FIFO_BUFSIZE			(2 * 1024)

#define	FIFO_BUFUPPER			(m_nBufSize * 4)
#define	FIFO_BUFLOWER			(m_nBufSize * 2)

#define	FIFO_STDIN				0
#define	FIFO_STDOUT				1
#define	FIFO_EXTIN				2
#define	FIFO_EXTOUT				3

#define	FIFO_TYPE_BASE			0
#define	FIFO_TYPE_WINDOW		1
#define	FIFO_TYPE_THREAD		2
#define	FIFO_TYPE_SYNC			3
#define	FIFO_TYPE_EVENT			4
#define	FIFO_TYPE_SOCKET		5
#define	FIFO_TYPE_LISTEN		6
#define	FIFO_TYPE_PIPE			7
#define	FIFO_TYPE_COM			8

#define	FIFO_THREAD_NONE		0
#define	FIFO_THREAD_EXEC		1
#define	FIFO_THREAD_ERROR		2

enum FifoCmd {
	FIFO_CMD_FDREAD,
	FIFO_CMD_FDWRITE,
	FIFO_CMD_FDEVENTS,
	FIFO_CMD_MSGQUEIN,
};

enum FifoMsgQueInCmd {
	FIFO_QCMD_LIMITSIZE,		// CFifoSocket
	FIFO_QCMD_GETSOCKOPT,		// CFifoSocket
	FIFO_QCMD_SETSOCKOPT,		// CFifoSocket
	FIFO_QCMD_IOCtl,			// CFifoSocket
	FIFO_QCMD_SSLCONNECT,		// CFifoSocket

	FIFO_QCMD_SENDWINSIZE,		// CFifoTelnet/CFifoLogin
	FIFO_QCMD_SENDBREAK,		// CFifoTelnet/CFifoPipe/CFifoCom
	FIFO_QCMD_COMMSTATE,		// CFifoCom
	FIFO_QCMD_SETDTRRTS,		// CFifoCom

	FIFO_QCMD_ENDOFDATA,		// CFifoStdio
	FIFO_QCMD_RCPDOWNLOAD,		// CFifoStdio

	FIFO_QCMD_SYNCRET,			// CFifoSsh
	FIFO_QCMD_RCPNEXT,			// CFifoRcp
	FIFO_QCMD_SENDPACKET,		// CFifoSsh
	FIFO_QCMD_KEEPALIVE,		// CFifoSsh
	FIFO_QCMD_CANCELPFD,		// CFifoSsh
};

class CFifoBuffer : public CObject
{
public:
	int m_nBufTop;
	int m_nBufBtm;
	int m_nBufMax;
	BYTE *m_pBuffer;

	CSemaphore m_Semaphore;

	int m_nReadNumber;
	int m_nWriteNumber;
	class CFifoBase *m_pReadBase;
	class CFifoBase *m_pWriteBase;

	BOOL m_bEndOf;

public:
	CFifoBuffer();
	~CFifoBuffer();

	int PutBuffer(LPBYTE pBuffer, int nBufLen);
	int GetBuffer(LPBYTE pBuffer, int nBufLen);

	int ResBuffer(LPBYTE pBuffer, int nBufLen);
	int Consume(int nBufLen);

	int BackBuffer(LPBYTE pBuffer, int nBufLen);

	inline int GetSize() { return (m_nBufBtm - m_nBufTop); }
	inline BYTE *GetTopPtr() { return (m_pBuffer + m_nBufTop); }
	inline BYTE *GetBtmPtr() { return (m_pBuffer + m_nBufBtm); }

	inline void Lock() { m_Semaphore.Lock(); }
	inline void Unlock() { m_Semaphore.Unlock(); }
};

typedef struct _FifoMsg {
	int cmd;
	int param;
	int msg;
	int len;
	BYTE *buffer;
	CEvent *pEvent;
	BOOL *pResult;
	struct _FifoMsg *next;
} FifoMsg;

typedef struct _DocMsg {
	int type;
	class CRLoginDoc *doc;
	void *pIn;
	void *pOut;
} DocMsg;

enum DocMsgType {
	DOCMSG_TYPE_STRINGA,
	DOCMSG_TYPE_STRINGW,
	DOCMSG_TYPE_SIZE,
	DOCMSG_TYPE_RECT,
	DOCMSG_TYPE_INTPTR,
	DOCMSG_TYPE_DWORDPTR,
	DOCMSG_TYPE_STRPTR,
	DOCMSG_TYPE_INT,
};

enum DocMsgCmd {
	DOCMSG_REMOTESTR,		// CFifoTelnet CFifoSsh
	DOCMSG_LOCALSTR,		// CFifoSsh

	DOCMSG_USERNAME,		// CFifoLogin, CFifoTelnet
	DOCMSG_PASSNAME,		// CFifoTelnet
	DOCMSG_TERMNAME,		// CFifoLogin, CFifoTelnet
	DOCMSG_ENVSTR,			// CFifoTelnet

	DOCMSG_SCREENSIZE,		// CFifoLogin, CFifoTelnet
	DOCMSG_LINEMODE,		// CFifoTelnet
	DOCMSG_TTYMODE,			// CFifoTelnet
	DOCMSG_KEEPALIVE,		// CFifoTelnet

	DOCMSG_ENTRYTEXT,		// CFifoSsh
	DOCMSG_LOGIT,			// CFifoSsh
	DOCMSG_COMMAND,			// CFifoSsh
	DOCMSG_SETTIMER,		// CFifoSsh

	DOCMSG_MESSAGE,			// AfxMessageBox->CRLoginApp::DoMessageBox
};

class CFifoBase : public CObject
{
public:
	int m_Type;
	class CRLoginDoc *m_pDocument;
	class CExtSocket *m_pSock;

	CPtrArray m_FifoBuf;
	CPtrArray m_EventTab;
	CDWordArray m_fdAllowEvents;
	CDWordArray m_fdPostEvents;
	CSemaphore m_FifoSemaphore;
	CEvent m_MsgEvent;
	FifoMsg *m_pMsgTop;
	FifoMsg *m_pMsgLast;
	FifoMsg *m_pMsgFree;
	CSemaphore m_MsgSemaphore;

	int m_nBufSize;
	int m_nLimitSize;
	BOOL m_bFlowCtrl;
	int m_nLastError;

	int m_nBufMax;
	BYTE *m_pBuffer;

public:
	CFifoBase(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoBase();

	virtual BOOL IsOpen();
	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);
	virtual void SendCommand(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL, BOOL *pResult = NULL);
	virtual void Destroy();

	virtual void OnLinked(int nFd, BOOL bMid);
	virtual void OnUnLinked(int nFd, BOOL bMid);

	void ChkFifoSize(int nFd);
	void SendFdEvents(int nFd, int msg, void *pParam);
	BOOL SendCmdWait(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL);
	void SendFdCommand(int nFd, int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL, BOOL *pResult = NULL);

	void MsgAddTail(FifoMsg *pFifoMsg);
	FifoMsg *MsgRemoveHead();
	void PostCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent = NULL, BOOL *pResult = NULL);
	void DeleteMsg(FifoMsg *pFifoMsg);
	void RemoveAllMsg();

	void SetFdEvents(int nFd, DWORD fdEvent);
	void ResetFdEvents(int nFd, DWORD fdEvent);
	inline BOOL IsFdEvents(int nFd, DWORD fdEvent) { return ((m_fdAllowEvents[nFd] & fdEvent) != 0); }

	int Consume(int nFd, int nBufLen);
	int Peek(int nFd, LPBYTE pBuffer, int nBufLen);
	int Read(int nFd, LPBYTE pBuffer, int nBufLen);
	int Write(int nFd, LPBYTE pBuffer, int nBufLen);
	int Insert(int nFd, LPBYTE pBuffer, int nBufLen);
	BOOL HaveData(int nFd);
	int GetDataSize(int nFd);
	int GetXFd(int nFd);
	BYTE *TempBuffer(int nSize);
	int MoveBuffer(int nFd, class CBuffer *bp);
	void Reset(int nFd);

	void SetFifo(int nFd, CFifoBuffer *pFifo);
	CFifoBuffer *GetFifo(int nFd);
	inline void RelFifo(CFifoBuffer *pFifo) { pFifo->Unlock(); }

	inline void Lock() { m_FifoSemaphore.Lock(); }
	inline void Unlock() { m_FifoSemaphore.Unlock(); }

	inline int GetFifoSize() { return (int)m_FifoBuf.GetSize(); }
	inline CEvent *GetEvent(int nFd) { return (CEvent *)m_EventTab[nFd]; }

public:
	LPCSTR DocMsgStrA(int msg, CStringA &str);
	LPCWSTR DocMsgStrW(int msg, CStringW &str);
#ifdef	_UNICODE
	#define	DocMsgStr(msg, str)	DocMsgStrW(msg, str)
	#define	DOCMSG_TYPE_STRINGT	DOCMSG_TYPE_STRINGW
#else
	#define	DocMsgStr(msg, str)	DocMsgStrA(msg, str)
	#define	DOCMSG_TYPE_STRINGT	DOCMSG_TYPE_STRINGA
#endif
	void DocMsgSize(int msg, CSize &size);
	void DocMsgRect(int msg, CRect &rect);
	void DocMsgIntPtr(int msg, int *pInt);
	LPCTSTR DocMsgLocalStr(LPCSTR str, CString &tstr);
	LPCSTR DocMsgRemoteStr(LPCTSTR str, CStringA &mbs);
	void DocMsgLineMode(int sw);
	DWORD DocMsgTtyMode(int mode, DWORD defVal);
	DWORD DocMsgKeepAlive(void *pThis);
	void DodMsgStrPtr(int msg, LPCTSTR str);
	void DocMsgCommand(int cmdId);
	int DocMsgSetTimer(int msec, int mode, void *pParam);
	
public:
	static void Link(CFifoBase *pLeft, int lFd, CFifoBase *pRight, int rFd);
	static void UnLink(CFifoBase *pMid, int nFd, BOOL bMid);

	static inline void MidLink(CFifoBase *pLeft, int lFd, CFifoBase *pMid, int mFd, CFifoBase *pRight, int rFd) { Link(pLeft, lFd, pRight, rFd); Link(pMid, mFd, pRight, rFd); }
	static inline void MidUnLink(CFifoBase *pLeft, int lFd, CFifoBase *pMid, int mFd, CFifoBase *pRight, int rFd) { UnLink(pMid, mFd, TRUE); UnLink(pRight, rFd, FALSE); }
};

class CFifoWnd : public CFifoBase
{
public:
	UINT m_WndMsg;
	CWnd *m_pWnd;
	BOOL m_bMsgPump;
	BOOL m_bPostMsg;
	BOOL m_bDestroy;

public:
	CFifoWnd(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoWnd();

	virtual void OnLinked(int nFd, BOOL bMid);
	virtual void OnUnLinked(int nFd, BOOL bMid);

	virtual void PostMessage(int cmd, int param);
	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);
	virtual void MsgPump(WPARAM wParam);
	virtual void SendCommand(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL, BOOL *pResult = NULL);
	virtual void Destroy();
	
	virtual void EndOfData(int nFd);
	virtual void Recived(int nFd, BYTE *pBuffer, int nBufLen);

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnAccept(int nFd, SOCKET socket);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);
};

class CFifoWorkThread : public CWinThread
{
	DECLARE_DYNCREATE(CFifoWorkThread)

public:
	BOOL m_bDestroy;

public:
	CFifoWorkThread();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

protected:
	afx_msg void OnFifoMsg(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

class CFifoThread : public CFifoWnd
{
public:
	CFifoWorkThread *m_pWinThread;

public:
	CFifoThread(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoThread();

	BOOL ThreadOpen();
	BOOL ThreadClose();

	virtual void PostMessage(int cmd, int param);
	virtual BOOL IsOpen();
	virtual void Destroy();

	virtual void OnLinked(int nFd, BOOL bMid);
	virtual void OnUnLinked(int nFd, BOOL bMid);
};

class CFifoSync : public CFifoBase
{
public:
	int m_Threadtatus;
	CWinThread *m_pWinThread;
	CEvent m_AbortEvent;
	int m_TunnelFd;
	CPtrArray m_nFdBuffer;

public:
	CFifoSync(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoSync();

	virtual BOOL IsOpen();
	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);
	virtual void SendCommand(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL);
	virtual void Destroy();

	BOOL WaitEvents(HANDLE hEvent, DWORD mSec);
	int Recv(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec = INFINITE);
	int Send(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec = INFINITE);
	inline void TunnelReadWrite(int nFd) { m_TunnelFd = nFd; }

	CBuffer *nFdBuffer(int nFd);

	int RecvByte(int nFd, DWORD mSec = INFINITE);
	int RecvBuffer(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec = INFINITE);
	inline int RecvSize(int nFd) { return GetDataSize(nFd) + nFdBuffer(nFd)->GetSize(); }
	void RecvBaek(int nFd, LPBYTE pBuffer = NULL, int nBufLen = 0);
	void RecvClear(int nFd);

	inline void SendByte(int nFd, int ch) { nFdBuffer(nFd)->Put8Bit(ch); }
	inline void SendBuffer(int nFd, LPBYTE pBuffer, int nBufLen) { nFdBuffer(nFd)->Apend(pBuffer, nBufLen); }
	inline void SendFlush(int nFd, int mSec) { CBuffer *bp = nFdBuffer(nFd); Send(nFd, bp->GetPtr(), bp->GetSize(), mSec); bp->Clear(); }

	BOOL Open(AFX_THREADPROC pfnThreadProc, LPVOID pParam, int nPriority = THREAD_PRIORITY_NORMAL);
	void Close();
	void Abort();
};

class CFifoASync : public CFifoBase
{
public:
	CArray<HANDLE, HANDLE> m_hWaitEvents;

public:
	CFifoASync(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoASync();

	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);

	int ReadWriteEvent();
};

class CFifoSocket : public CFifoASync
{
public:
	SOCKET m_hSocket;
	WSAEVENT m_SockEvent;
	BOOL m_bAbort;
	CEvent m_AbortEvent;
	int m_Threadtatus;
	CWinThread *m_pWinThread;
	SSL *m_SSL_pSock;

	CString m_HostAddress;
	UINT m_nHostPort;
	UINT m_nSocketPort;
	int m_nFamily;
	int m_nSocketType;

public:
	CFifoSocket(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoSocket();

	virtual BOOL IsOpen();
	virtual void Destroy();
	virtual void OnUnLinked(int nFd, BOOL bMid);

	void ThreadEnd();
	BOOL Open(LPCTSTR lpszHostAddress = NULL, UINT nHostPort = 0, UINT nSocketPort = 0, int nFamily = AF_UNSPEC, int nSocketType = SOCK_STREAM);
	BOOL Close();
	BOOL ReOpen();

	BOOL Attach(SOCKET hSocket);
	void Detach();

	BOOL AddInfoOpen();
	BOOL SocketLoop();
};

class CFifoListen : public CFifoASync
{
public:
	CArray<WSAEVENT, WSAEVENT> m_SocketEvent;
	CArray<SOCKET, SOCKET> m_Socket;
	int m_nLastError;

	BOOL m_bAbort;
	CEvent m_AbortEvent;
	int m_Threadtatus;
	CWinThread *m_pWinThread;

	CString m_HostAddress;
	UINT m_nHostPort;
	int m_nFamily;
	int m_nSocketType;
	int m_nBacklog;

public:
	CFifoListen(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoListen();

	void Destroy();

	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, int nFamily = AF_UNSPEC, int nSocketType = SOCK_STREAM, int nBacklog = 128);
	BOOL Close();

	BOOL AddInfoOpen();
	BOOL ListenProc();
};

class CFifoProxy : public CFifoWnd
{
public:

public:
	CFifoProxy(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoProxy();

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);
};

class CFifoDocument : public CFifoWnd
{
public:
	clock_t m_RecvStart;
	BOOL m_bContinue;
	BOOL m_bClosed;

public:
	CFifoDocument(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoDocument();

	virtual void EndOfData(int nFd);

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnClose(int nFd, int nLastError);
	virtual void OnConnect(int nFd);

	BOOL OnIdle();
};
