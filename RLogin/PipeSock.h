#pragma once
#include "extsocket.h"

class CPipeSock : public CExtSocket
{
public:
	HANDLE m_hIn[2];
	HANDLE m_hOut[2];

	volatile int m_InThreadMode;
	volatile int m_OutThreadMode;

	CWinThread *m_InThread;
	CWinThread *m_OutThread;

	CEvent *m_pInEvent;
	CEvent *m_pOutEvent;
	CEvent *m_pSendEvent;

	CSemaphore m_RecvSema;
	CSemaphore m_SendSema;

	PROCESS_INFORMATION m_proInfo;

	OVERLAPPED m_ReadOverLap;
	OVERLAPPED m_WriteOverLap;

public:
	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	BOOL AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	void Close();
	int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	void SendBreak(int opt);
	void OnRecive(int nFlags);
	void OnSend();
	int OnIdle();
	int GetRecvSize();
	int GetSendSize();

	void GetPathMaps(CStringMaps &maps);
	void GetDirMaps(CStringMaps &maps, LPCTSTR dir, BOOL pf = FALSE);

	void OnReadProc();
	void OnWriteProc();
	void OnReadWriteProc();

	CPipeSock(class CRLoginDoc *pDoc);
	virtual ~CPipeSock(void);
};
