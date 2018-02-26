// ComSock.h: CComSock クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMSOCK_H__E6EEA8E0_5B98_4B94_BB4E_A5C319F6491A__INCLUDED_)
#define AFX_COMSOCK_H__E6EEA8E0_5B98_4B94_BB4E_A5C319F6491A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ExtSocket.h"

#define	COMFLOW_NONE		0
#define	COMFLOW_RTSCTS		1
#define	COMFLOW_XONXOFF		2
#define	COMFLOW_USERDEF		3

#define	COMQUEUESIZE		4096
#define	COMBUFSIZE			(COMQUEUESIZE / 4)

#define	COMALIVEBYTE		33	// COMn n=1-256 = (256 / 8 + 1) = 33
#define	COMALIVEBITS		(COMALIVEBYTE * 8)

class CComSock : public CExtSocket  
{
public:
	HANDLE m_hCom;
	int m_ComPort;
	COMMCONFIG *m_pComConf;
	UINT m_SendWait[2];

	int m_SendBreak;
	BOOL m_bCommState;
	DWORD m_CommError;
	DWORD m_ModemStatus;
	DWORD m_ComCtrl;
	BOOL m_bGetStatus;
	BOOL m_bRetCode;
	int m_MsecByte;
	class CComMoniDlg *m_pComMoni;
	int m_RecvByteSec;
	int m_SendByteSec;
	DWORD m_fInXOutXMode;

	volatile enum { THREAD_NONE = 0, THREAD_RUN, THREAD_ENDOF, THREAD_DONE } m_ThreadMode;

	CEvent *m_pThreadEvent;
	CEvent *m_pSendEvent;
	CEvent *m_pRecvEvent;
	CEvent *m_pStatEvent;

	CSemaphore m_RecvSema;
	CSemaphore m_SendSema;

	OVERLAPPED m_ReadOverLap;
	OVERLAPPED m_WriteOverLap;
	OVERLAPPED m_CommOverLap;

public:
	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	BOOL AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	void Close();
	BOOL SetComConf();
	void SetXonXoff(int sw);
	void SendBreak(int opt = 0);
	BOOL SetComCtrl(DWORD ctrl);
	int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	void OnReceive(int nFlags);
	void OnSend();
	void OnRecvEmpty();
	void GetStatus(CString &str);
	int GetRecvSize();
	int GetSendSize();
	void GetRecvSendByteSec(int &recvByte, int &sendByte);
	void ComMoniter();

	void OnReadWriteProc();

	BOOL LoadComConf(LPCTSTR ComSetStr, int ComPort, BOOL bOpen = FALSE);
	BOOL SaveComConf(CString &str);
	BOOL SetupComConf(CString &ComSetStr, int &ComPort, CWnd *pOwner = NULL);

	static void SetFlowCtrlMode(DCB *pDCB, int FlowCtrl, LPCTSTR UserDef);
	static int GetFlowCtrlMode(DCB *pDCB, CString &UserDef);
	static void AliveComPort(BYTE pComAliveBits[COMALIVEBYTE]);

	CComSock(class CRLoginDoc *pDoc);
	virtual ~CComSock();
};

#endif // !defined(AFX_COMSOCK_H__E6EEA8E0_5B98_4B94_BB4E_A5C319F6491A__INCLUDED_)
