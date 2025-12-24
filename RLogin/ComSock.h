//////////////////////////////////////////////////////////////////////
// ComSock.h : インターフェイス
//

#pragma once

#define	COMFLOW_NONE		0
#define	COMFLOW_RTSCTS		1
#define	COMFLOW_XONXOFF		2
#define	COMFLOW_USERDEF		3

#define	COMQUEUESIZE		(4 * 1024)
#define	COMBUFSIZE			(1024)

#define	COMALIVEBYTE		33	// COMn n=1-256 = (256 / 8 + 1) = 33
#define	COMALIVEBITS		(COMALIVEBYTE * 8)

class CFifoCom : public CFifoASync
{
public:
	HANDLE m_hCom;

	volatile enum { THREAD_NONE = 0, THREAD_RUN, THREAD_ENDOF } m_ThreadMode;
	CWinThread *m_pComThread;
	CEvent m_AbortEvent;
	CEvent m_ThreadEvent;

	UINT m_SendWait[4];
	int m_SendCrLf;

public:
	CFifoCom(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoCom();

	virtual void OnUnLinked(int nFd, BOOL bMid);

	BOOL Open(HANDLE hCom = INVALID_HANDLE_VALUE);
	void Close();

	BOOL FlowCtrlCheck(int nFd);
	int CalcReadByte();
	void CalcWriteByteMsec(int ByteSec, int &maxSize, int &maxMsec);
	void OnReadWriteProc();
};

class CComSock : public CExtSocket
{
public:
	HANDLE m_hCom;
	int m_ComPort;
	COMMCONFIG *m_pComConf;
	class CComMoniDlg *m_pComMoni;

	UINT m_SendWait[3];
	int m_SendCrLf;

	DWORD m_ComEvent;
	DWORD m_CommError;
	DWORD m_ModemStatus;
	COMSTAT m_ComStat;

	int m_RecvByteSec;
	int m_SendByteSec;

	DWORD m_fInXOutXMode;

public:
	CComSock(class CRLoginDoc *pDoc);
	virtual ~CComSock();

	virtual CFifoBase *FifoLinkLeft();
	virtual BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	virtual void Close();

	virtual void GetStatus(CString &str);
	virtual void SetXonXoff(int sw);
	virtual void SendBreak(int opt = 0);

	int GetByteSec();
	BOOL SetComConf();
	BOOL SetComCtrl(DWORD ctrl);

	void GetRecvSendByteSec(int &recvByte, int &sendByte);
	void ComMoniter();

	void SetDcdParam(CStringArrayExt &param);
	inline void SetDcdStr(LPCTSTR str) { CStringArrayExt param; param.GetString(str, _T(';')); SetDcdParam(param); }
	BOOL LoadComConf(LPCTSTR ComSetStr, int ComPort, BOOL bOpen = FALSE);
	BOOL SaveComConf(CString &str);
	BOOL SetupComConf(CString &ComSetStr, int &ComPort, CWnd *pOwner = NULL);

	static void SetFlowCtrlMode(DCB *pDCB, int FlowCtrl, LPCTSTR UserDef);
	static int GetFlowCtrlMode(DCB *pDCB, CString &UserDef);
	static void AliveComPort(BYTE pComAliveBits[COMALIVEBYTE]);
};
