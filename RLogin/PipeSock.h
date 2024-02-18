//////////////////////////////////////////////////////////////////////
// PipeSock.h : インターフェイス
//

#pragma once

class CFifoPipe : public CFifoASync
{
public:
	HANDLE m_hIn[2];
	HANDLE m_hOut[2];

	volatile int m_InThreadMode;
	volatile int m_OutThreadMode;

	CWinThread *m_ProcThread;
	CWinThread *m_InThread;
	CWinThread *m_OutThread;

	CEvent m_AbortEvent[3];
	CEvent m_ThreadEvent[3];
	PROCESS_INFORMATION m_proInfo;

public:
	CFifoPipe(class CRLoginDoc *pDoc, class CExtSocket *pSock);
	~CFifoPipe();

	virtual void OnUnLinked(int nFd, BOOL bMid);

	BOOL IsPipeName(LPCTSTR path);
	BOOL Open(LPCTSTR pCommand);
	void Close();
	void SendBreak(int opt);

	BOOL FlowCtrlCheck(int nFd);
	BOOL WaitForEvent(int nFd, HANDLE hAbortEvent);

	void OnProcWait();
	void OnReadProc();
	void OnWriteProc();
	void OnReadWriteProc();
};

class CPipeSock : public CExtSocket
{
public:

public:
	CPipeSock(class CRLoginDoc *pDoc);
	virtual ~CPipeSock(void);

	virtual CFifoBase *FifoLinkLeft();
	virtual void SendBreak(int opt);

	void GetPathMaps(CStringMaps &maps);
	void GetDirMaps(CStringMaps &maps, LPCTSTR dir, BOOL pf = FALSE);
};
