// ComSock.h: CComSock クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMSOCK_H__E6EEA8E0_5B98_4B94_BB4E_A5C319F6491A__INCLUDED_)
#define AFX_COMSOCK_H__E6EEA8E0_5B98_4B94_BB4E_A5C319F6491A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ExtSocket.h"

class CComSock : public CExtSocket  
{
public:
	HANDLE m_hCom;
	CString m_ComName;
	int m_ComPort;
	int m_SendBreak;
	BOOL m_bCommState;
	DWORD m_CommError;
	DWORD m_ModemStatus;
	BOOL m_bGetStatus;

	volatile int m_ThreadMode;

	CEvent *m_pThredEvent;
	CEvent *m_pSendEvent;
	CEvent *m_pStatEvent;

	CSemaphore m_RecvSema;
	CSemaphore m_SendSema;

	OVERLAPPED m_ReadOverLap;
	OVERLAPPED m_WriteOverLap;
	OVERLAPPED m_CommOverLap;

private:
	COMMCONFIG m_ComConf;
	COMMTIMEOUTS m_ComTime;
	int m_BaudRate;
	int m_ByteSize;
	int m_Parity;
	int m_StopBits;
	int m_FlowCtrl;
	int m_SaveFlowCtrl;

public:
	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	BOOL AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);
	void Close();
	void SetXonXoff(int sw);
	void SendBreak(int opt = 0);
	int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	void OnRecive(int nFlags);
	void OnSend();
	void GetStatus(CString &str);
	int GetRecvSize();
	int GetSendSize();

	void OnReadWriteProc();

	void GetConfig();
	void SetConfig();

	void GetMode(LPCTSTR str);
	void SetMode(CString &str);

	void ConfigDlg(CWnd *pWnd, CString &str);
	static DWORD AliveComPort();

	CComSock(class CRLoginDoc *pDoc);
	virtual ~CComSock();
};

#endif // !defined(AFX_COMSOCK_H__E6EEA8E0_5B98_4B94_BB4E_A5C319F6491A__INCLUDED_)
