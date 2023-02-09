#pragma once

#include <openssl/ssl.h>

#ifdef	USE_FIFOBUF

#define	FIFO_BUFDEFMAX			(8 * 1024)
#define	FIFO_BUFSIZE			(2 * 1024)

#define	FIFO_FLOWUPER			(m_nBufSize * 4)
#define	FIFO_FLOWMIDDLE			(m_nBufSize * 3)
#define	FIFO_FLOWLOWER			(m_nBufSize * 2)

#define	FIFO_STDIN				0
#define	FIFO_STDOUT				1
#define	FIFO_EXTIN				2
#define	FIFO_EXTOUT				3

#define	FD_POSTMSG				(1 << 31)	// > FD_MAX_EVENTS(10)

#define	FIFO_TYPE_BASE			0
#define	FIFO_TYPE_WINDOW		1
#define	FIFO_TYPE_THREAD		2
#define	FIFO_TYPE_SYNC			3
#define	FIFO_TYPE_EVENT			4
#define	FIFO_TYPE_SOCKET		5

#define	FIFO_THREAD_NONE		0
#define	FIFO_THREAD_EXEC		1
#define	FIFO_THREAD_ERROR		2

enum FifoCmd {
	FIFO_CMD_FDEVENTS,
	FIFO_CMD_MSGQUEIN,
};

enum FifoMsgQueInCmd {
	FIFO_QCMD_LIMITSIZE,		// CFifoSocket
	FIFO_QCMD_GETSOCKOPT,		// CFifoSocket
	FIFO_QCMD_SETSOCKOPT,		// CFifoSocket
	FIFO_QCMD_IOCtl,			// CFifoSocket
	FIFO_QCMD_SSLCONNECT,		// CFifoSocket
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
};

enum DocMsgCmd {
	DOCMSG_REMOTESTR,
	DOCMSG_LOCALSTR,

	DOCMSG_USERNAME,
	DOCMSG_PASSNAME,
	DOCMSG_TERMNAME,

	DOCMSG_SCREENSIZE,
};

class CFifoBase : public CObject
{
public:
	int m_Type;
	CPtrArray m_FifoBuf;
	CPtrArray m_EventTab;
	CDWordArray m_fdAllowEvents;
	CDWordArray m_fdPostEvents;
	CSemaphore m_FifoSemaphore;
	CEvent m_MsgEvent;
	CList<FifoMsg *, FifoMsg *> m_MsgList;
	CSemaphore m_MsgSemaphore;
	class CRLoginDoc *m_pDocument;
	int m_nLastError;
	int m_nBufSize;

public:
	CFifoBase(class CRLoginDoc *pDoc);
	~CFifoBase();

	virtual BOOL IsOpen();
	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam) = NULL;
	virtual void SendCommand(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL, BOOL *pResult = NULL);
	virtual void Destroy();

	virtual void OnLinked(int nFd, BOOL bMid);
	virtual void OnUnLinked(int nFd, BOOL bMid);

	void ChkFifoSize(int nFd);
	void SendFdEvents(int nFd, int msg, void *pParam);
	BOOL SendCmdWait(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL);
	void SendFdCommand(int nFd, int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL, BOOL *pResult = NULL);
	void PostCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent = NULL, BOOL *pResult = NULL);
	void DeleteMsg(FifoMsg *pFifoMsg);
	void RemoveAllMsg();

	void SetFdEvents(int nFd, DWORD fdEvent);
	inline void ResetFdEvents(int nFd, DWORD fdEvent) { m_fdAllowEvents[nFd] &= ~fdEvent; }
	inline BOOL IsFdEvents(int nFd, DWORD fdEvent) { return ((m_fdAllowEvents[nFd] & fdEvent) != 0 ? TRUE : FALSE); }

	int Consume(int nFd, int nBufLen);
	int Peek(int nFd, LPBYTE pBuffer, int nBufLen);
	int Read(int nFd, LPBYTE pBuffer, int nBufLen);
	int Write(int nFd, LPBYTE pBuffer, int nBufLen);
	int Insert(int nFd, LPBYTE pBuffer, int nBufLen);
	BOOL HaveData(int nFd);
	int GetDataSize(int nFd);
	int GetXFd(int nFd);

	void SetFifo(int nFd, CFifoBuffer *pFifo);
	CFifoBuffer *GetFifo(int nFd);
	void RelFifo(CFifoBuffer *pFifo);

	inline void Lock() { m_FifoSemaphore.Lock(); }
	inline void Unlock() { m_FifoSemaphore.Unlock(); }

	inline int GetFifoSize() { return (int)m_FifoBuf.GetSize(); }
	inline CEvent *GetEvent(int nFd) { return (CEvent *)m_EventTab[nFd]; }

public:
	LPCSTR DocMsgStrA(int msg, CStringA &str);
	LPCWSTR DocMsgStrW(int msg, CStringW &str);
#ifdef	_UNICODE
	#define	DocMsgStr(msg)	DocMsgStrW(msg)
	#define	DOCMSG_TYPE_STRINGT	DOCMSG_TYPE_STRINGW
#else
	#define	DocMsgStr(msg)	DocMsgStrA(msg)
	#define	DOCMSG_TYPE_STRINGT	DOCMSG_TYPE_STRINGA
#endif
	void DocMsgSize(int msg, CSize &size);
	void DocMsgRect(int msg, CRect &rect);
	void DocMsgIntPtr(int msg, int *pInt);
	
public:
	static void Link(CFifoBase *pLeft, int lFd, CFifoBase *pRight, int rFd);
	static void UnLink(CFifoBase *pMid, int nFd, BOOL bMid);

	static inline void MidLink(CFifoBase *pLeft, int lFd, CFifoBase *pMid, int mFd, CFifoBase *pRight, int rFd) { Link(pLeft, lFd, pRight, rFd); Link(pMid, mFd, pRight, rFd); }
	static inline void MidUnLink(CFifoBase *pLeft, int lFd, CFifoBase *pMid, int mFd, CFifoBase *pRight, int rFd) { UnLink(pMid, mFd, TRUE); UnLink(pRight, rFd, FALSE); }
};

class CFifoWorkThread : public CWinThread
{
	DECLARE_DYNCREATE(CFifoWorkThread)

public:
	class CFifoBase *m_pFifoBase;

public:
	CFifoWorkThread();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

protected:
	afx_msg void OnFifoMsg(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
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
	CFifoWnd(class CRLoginDoc *pDoc);
	~CFifoWnd();

	void PostMessage(WPARAM wParam, LPARAM lParam);
	void SendStr(int nFd, LPCSTR mbs);

	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);
	virtual void MsgPump(WPARAM wParam);
	virtual void SendCommand(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL, BOOL *pResult = NULL);
	virtual void Destroy();
	
	virtual int Send(int nFd, BYTE *pBuffer, int nBufLen);
	virtual int Recived(int nFd, BYTE *pBuffer, int nBufLen);

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnAccept(int nFd, SOCKET socket);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);
};

class CFifoThread : public CFifoWnd
{
public:
	CFifoWorkThread *m_pWinThread;

public:
	CFifoThread(class CRLoginDoc *pDoc);
	~CFifoThread();

	BOOL ThreadOpen();
	BOOL ThreadClose();

	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);
	virtual void MsgPump(WPARAM wParam);
	virtual void SendCommand(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL, BOOL *pResult = NULL);
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

public:
	CFifoSync(class CRLoginDoc *pDoc);
	~CFifoSync();

	virtual BOOL IsOpen();
	virtual void FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam);
	virtual void SendCommand(int cmd, int param = 0, int msg = 0, int len = 0, void *buf = NULL, CEvent *pEvent = NULL);
	virtual void Destroy();

	BOOL WaitEvents(HANDLE hEvent, DWORD mSec);
	int Recv(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec = INFINITE);
	int Send(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec = INFINITE);
	inline void TunnelReadWrite(int nFd) { m_TunnelFd = nFd; }

	BOOL Open(AFX_THREADPROC pfnThreadProc, LPVOID pParam, int nPriority = THREAD_PRIORITY_NORMAL);
	void Close();
	void Abort();
};

class CFifoASync : public CFifoBase
{
public:
	CArray<HANDLE, HANDLE> m_hWaitEvents;

public:
	CFifoASync(class CRLoginDoc *pDoc);

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
	int m_nLimitSize;
	SSL *m_SSL_pSock;

	CString m_HostAddress;
	UINT m_nHostPort;
	UINT m_nSocketPort;
	int m_nFamily;
	int m_nSocketType;

public:
	CFifoSocket(class CRLoginDoc *pDoc);
	~CFifoSocket();

	virtual BOOL IsOpen();
	virtual void Destroy();

	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nFamily = AF_UNSPEC, int nSocketType = SOCK_STREAM);
	BOOL Close();

	BOOL Attach(SOCKET hSocket);
	void Detach();

	BOOL AddInfoOpen();
	BOOL SocketLoop();
};

class CListenSocket : public CObject
{
public:
	int m_nFd;
	CFifoBase *m_pFifoBase;
	CArray<WSAEVENT, WSAEVENT> m_SocketEvent;
	CArray<SOCKET, SOCKET> m_Socket;
	int m_nLastError;

	CEvent m_AbortEvent;
	int m_Threadtatus;
	CWinThread *m_pWinThread;

	CString m_HostAddress;
	UINT m_nHostPort;
	int m_nFamily;
	int m_nSocketType;
	int m_nBacklog;

public:
	CListenSocket(int nFd, CFifoBase *pFifoBase);
	~CListenSocket();

	void Destroy();

	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, int nFamily = AF_UNSPEC, int nSocketType = SOCK_STREAM, int nBacklog = 128);
	BOOL Close();

	BOOL AddInfoOpen();
	BOOL ListenProc();
};

class CFifoProxy : public CFifoWnd
{
public:
	class CExtSocket *m_pSock;

public:
	CFifoProxy(class CRLoginDoc *pDoc, class CExtSocket *pSock);

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnAccept(int nFd, SOCKET socket);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);
};

//#define	USE_AVERAGE

class CFifoMiddle : public CFifoWnd
{
public:
	class CExtSocket *m_pSock;
	BOOL m_bFlowCtrl;

#ifdef	USE_AVERAGE
	struct {
		clock_t start;
		int lastSize;
		LONGLONG totalSize;
		int average;
	} m_Average[4];
#endif

public:
	CFifoMiddle(class CRLoginDoc *pDoc, class CExtSocket *pSock);

#ifdef	USE_AVERAGE
	void Average(int nFd, int len);
	int Read(int nFd, LPBYTE pBuffer, int nBufLen);
	int Write(int nFd, LPBYTE pBuffer, int nBufLen);
#endif

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnAccept(int nFd, SOCKET socket);
	virtual void OnConnect(int nFd);
	virtual void OnClose(int nFd, int nLastError);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);
};

class CFifoDocument : public CFifoWnd
{
public:
	class CExtSocket *m_pSock;
	clock_t m_RecvStart;
	BOOL m_bContinue;

public:
	CFifoDocument(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	CFifoDocument::~CFifoDocument();

	virtual void OnRead(int nFd);
	virtual void OnWrite(int nFd);
	virtual void OnClose(int nFd, int nLastError);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);

	BOOL OnIdle();
};

class CFifoLogin : CFifoThread
{
public:
	void SendWindSize(int nFd);

	virtual int Recived(int nFd, BYTE *pBuffer, int nBufLen);
	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnConnect(int nFd);
};

#endif	// USE_FIFOBUF