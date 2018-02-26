// ComSock.cpp: CComSock クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ComSock.h"
#include "ComMoniDlg.h"
#include "ComInitDlg.h"

#include <winbase.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CComSock::CComSock(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type     = ESCT_COMDEV;

	m_hCom     = INVALID_HANDLE_VALUE;
	m_ComPort = (-1);
	m_pComConf = NULL;

	m_SendWait[0] = 0;
	m_SendWait[1] = 0;
	m_MsecByte = 0;

	m_ThreadMode  = THREAD_NONE;
	m_pThreadEvent = new CEvent(FALSE, TRUE);
	m_pSendEvent   = new CEvent(FALSE, TRUE);
	m_pRecvEvent   = new CEvent(FALSE, TRUE);
	m_pStatEvent   = new CEvent(FALSE, TRUE);

	memset(&m_ReadOverLap,  0, sizeof(OVERLAPPED));
	memset(&m_WriteOverLap, 0, sizeof(OVERLAPPED));
	memset(&m_CommOverLap,  0, sizeof(OVERLAPPED));

	m_ReadOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_WriteOverLap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_CommOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_SendBreak = 0;
	m_bCommState = FALSE;
	m_ComCtrl = 0;

	m_CommError = 0;
	m_ModemStatus = 0;
	m_bGetStatus = FALSE;

	m_pComMoni = NULL;
	m_RecvByteSec = 0;
	m_SendByteSec = 0;
	m_fInXOutXMode = 0;

	SetRecvBufSize(COMBUFSIZE);
}
CComSock::~CComSock()
{
	Close();

	delete m_pThreadEvent;
	delete m_pSendEvent;
	delete m_pRecvEvent;
	delete m_pStatEvent;

	CloseHandle(m_ReadOverLap.hEvent);
	CloseHandle(m_WriteOverLap.hEvent);
	CloseHandle(m_CommOverLap.hEvent);

	if ( m_pComConf != NULL )
		delete m_pComConf;
}

static UINT ComEventThread(LPVOID pParam)
{
	CComSock *pThis = (CComSock *)pParam;

	pThis->OnReadWriteProc();
	pThis->m_pThreadEvent->SetEvent();
	pThis->m_ThreadMode = CComSock::THREAD_DONE;
	return 0;
}

BOOL CComSock::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
	COMMTIMEOUTS ComTime;

	if ( m_hCom != INVALID_HANDLE_VALUE )
		return FALSE;

	if ( !LoadComConf(lpszHostAddress, nHostPort, TRUE) ) {
		::AfxMessageBox(GetFormatErrorMessage(m_pDocument->m_ServerEntry.m_EntryName, lpszHostAddress, nHostPort, _T("GetCommConfig"), ::GetLastError()), MB_ICONSTOP);
		return FALSE;
	}

	SetCommState(m_hCom, &(m_pComConf->dcb));
	SetupComm(m_hCom, COMQUEUESIZE, COMQUEUESIZE);
	PurgeComm(m_hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	EscapeCommFunction(m_hCom, m_pComConf->dcb.fDtrControl == DTR_CONTROL_DISABLE ? CLRDTR : SETDTR);
	EscapeCommFunction(m_hCom, m_pComConf->dcb.fRtsControl == RTS_CONTROL_DISABLE ? CLRRTS : SETRTS);

	GetCommTimeouts(m_hCom, &ComTime);
	ComTime.ReadIntervalTimeout         = 10;
	ComTime.ReadTotalTimeoutMultiplier  = 0;
	ComTime.ReadTotalTimeoutConstant    = 50;
	ComTime.WriteTotalTimeoutMultiplier = (m_MsecByte <= 0 ? 1 : m_MsecByte);
	ComTime.WriteTotalTimeoutConstant   = 50;
	SetCommTimeouts(m_hCom, &ComTime);

	GetCommModemStatus(m_hCom, &m_ModemStatus);
	SetCommMask(m_hCom, EV_RXCHAR | EV_RX80FULL | EV_DSR | EV_TXEMPTY | EV_CTS | EV_RING | EV_RLSD | EV_ERR);

	GetMainWnd()->SetAsyncSelect((SOCKET)m_hCom, this, 0);

	m_ThreadMode = THREAD_RUN;
	m_pThreadEvent->ResetEvent();
	AfxBeginThread(ComEventThread, this, THREAD_PRIORITY_NORMAL);

	AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, WSAMAKESELECTREPLY(FD_CONNECT, 0));
	return TRUE;
}
BOOL CComSock::AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);
}
void CComSock::Close()
{
	if ( m_hCom == INVALID_HANDLE_VALUE )
		return;

	//	GetApp()->DelSocketIdle(this);
	GetMainWnd()->DelAsyncSelect((SOCKET)m_hCom, this, FALSE);

	if ( m_ThreadMode == THREAD_RUN ) {
		m_ThreadMode = THREAD_ENDOF;
		CancelIo(m_hCom);
		m_pSendEvent->SetEvent();
		m_pRecvEvent->SetEvent();
		SetEvent(m_ReadOverLap.hEvent);
		SetEvent(m_WriteOverLap.hEvent);
		SetEvent(m_CommOverLap.hEvent);
		WaitForSingleObject(m_pThreadEvent->m_hObject, INFINITE);
		m_ThreadMode = THREAD_NONE;
	}

	SetCommMask(m_hCom, 0);
	PurgeComm(m_hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	EscapeCommFunction(m_hCom, CLRDTR);
	EscapeCommFunction(m_hCom, CLRRTS);
	CloseHandle(m_hCom);
	m_hCom = NULL;

	if ( m_pComMoni != NULL )
		m_pComMoni->DestroyWindow();

	CExtSocket::Close();
}
void CComSock::SendBreak(int opt)
{
	if ( m_hCom == INVALID_HANDLE_VALUE )
		return;

	m_SendSema.Lock();
	m_SendBreak = (opt != 0 ? 300 : 100);
	m_pSendEvent->SetEvent();
	m_SendSema.Unlock();
}
int CComSock::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	if ( lpBuf == NULL || nBufLen <= 0 )
		return 0;

	m_SendSema.Lock();
	m_SendBuff.Apend((LPBYTE)lpBuf, nBufLen);
	m_pSendEvent->SetEvent();
	m_SendSema.Unlock();

	return nBufLen;
}
BOOL CComSock::SetComConf()
{
	if ( m_hCom == INVALID_HANDLE_VALUE || m_pComConf == NULL )
		return FALSE;

	m_SendSema.Lock();
	m_pStatEvent->ResetEvent();
	m_bRetCode = FALSE;
	m_bCommState = TRUE;
	m_pSendEvent->SetEvent();
	m_SendSema.Unlock();

	if ( m_ThreadMode == THREAD_RUN )
		WaitForSingleObject(m_pStatEvent->m_hObject, INFINITE);

	return m_bRetCode;
}
void CComSock::SetXonXoff(int sw)
{
	if ( m_hCom == INVALID_HANDLE_VALUE || m_pComConf == NULL )
		return;

	if ( sw ) {		// OFF->ON
		if ( m_pComConf->dcb.fInX != 0 || m_pComConf->dcb.fOutX != 0 || m_fInXOutXMode == 0 )
			return;
		m_pComConf->dcb.fInX  = m_fInXOutXMode & 1;
		m_pComConf->dcb.fOutX = (m_fInXOutXMode >> 4) & 1;
		m_fInXOutXMode = 0;
	} else {		// ON->OFF
		if ( m_pComConf->dcb.fInX == 0 && m_pComConf->dcb.fOutX == 0 )
			return;
		m_fInXOutXMode = m_pComConf->dcb.fInX | (m_pComConf->dcb.fOutX << 4);
		m_pComConf->dcb.fInX  = 0;
		m_pComConf->dcb.fOutX = 0;
	}

	SetComConf();
}
BOOL CComSock::SetComCtrl(DWORD ctrl)
{
	m_SendSema.Lock();
	m_pStatEvent->ResetEvent();
	m_bRetCode = FALSE;
	m_ComCtrl = ctrl;
	m_pSendEvent->SetEvent();
	m_SendSema.Unlock();

	if ( m_ThreadMode == THREAD_RUN )
		WaitForSingleObject(m_pStatEvent->m_hObject, INFINITE);

	return m_bRetCode;
}
int CComSock::GetRecvSize()
{
	int len = CExtSocket::GetRecvSize();
	
	m_RecvSema.Lock();
	len += m_RecvBuff.GetSize();
	m_RecvSema.Unlock();

	return len;
}
int CComSock::GetSendSize()
{
	int len = CExtSocket::GetSendSize();

	m_SendSema.Lock();
	len += m_SendBuff.GetSize();
	m_SendSema.Unlock();

	return len;
}
void CComSock::GetRecvSendByteSec(int &recvByte, int &sendByte)
{
	m_RecvSema.Lock();
	recvByte = m_RecvByteSec;
	m_RecvByteSec = 0;
	m_RecvSema.Unlock();

	m_SendSema.Lock();
	sendByte = m_SendByteSec;
	m_SendByteSec = 0;
	m_SendSema.Unlock();
}

void CComSock::GetStatus(CString &str)
{
	int n;
	CString tmp;

	CExtSocket::GetStatus(str);

	str += _T("\r\n");

	m_RecvSema.Lock();
	n = m_RecvBuff.GetSize();
	m_RecvSema.Unlock();
	tmp.Format(_T("Receive Buffer: %d Bytes\r\n"), n);
	str += tmp;

	m_SendSema.Lock();
	n = m_SendBuff.GetSize();
	m_SendSema.Unlock();
	tmp.Format(_T("Send Buffer: %d Bytes\r\n"), n);
	str += tmp;

	tmp.Format(_T("Thread Status: %d\r\n"), m_ThreadMode);
	str += tmp;

	m_SendSema.Lock();
	m_pStatEvent->ResetEvent();
	m_bGetStatus = TRUE;
	m_pSendEvent->SetEvent();
	m_SendSema.Unlock();

	if ( m_ThreadMode == THREAD_RUN )
		WaitForSingleObject(m_pStatEvent->m_hObject, INFINITE);

	str += _T("\r\n");

	tmp.Format(_T("Open Device: COM%d\r\n"), m_ComPort);
	str += tmp;

	str += _T("Configuration: ");
	SaveComConf(tmp);
	str += tmp;
	str += _T("\r\n");

	tmp.Format(_T("Modem Status: CTS(%s) DSR(%s) RING(%s) RLSD(%s)\r\n"),
		((m_ModemStatus & MS_CTS_ON)  != 0 ? _T("ON") : _T("OFF")),
		((m_ModemStatus & MS_DSR_ON)  != 0 ? _T("ON") : _T("OFF")),
		((m_ModemStatus & MS_RING_ON) != 0 ? _T("ON") : _T("OFF")),
		((m_ModemStatus & MS_RLSD_ON) != 0 ? _T("ON") : _T("OFF")));
	str += tmp;

	str += _T("Error Status:");
	if ( m_CommError == 0 ) str += _T(" No Error");
	else {
		if ( (m_CommError & CE_BREAK)    != 0 ) str += _T(" Received Break,");
		if ( (m_CommError & CE_FRAME)    != 0 ) str += _T(" Frame Error,");
		if ( (m_CommError & CE_IOE)      != 0 ) str += _T(" I/O Error,");
		if ( (m_CommError & CE_OVERRUN)  != 0 ) str += _T(" Overrun Error,");
		if ( (m_CommError & CE_RXOVER)   != 0 ) str += _T(" Receive Overflow,");
		if ( (m_CommError & CE_RXPARITY) != 0 ) str += _T(" Parity Error,");
		if ( (m_CommError & CE_TXFULL)   != 0 ) str += _T(" Send Buffer Full,");
	}
	str += _T("\r\n");
}
void CComSock::ComMoniter()
{
	if ( m_pComMoni == NULL ) {
		m_pComMoni = new CComMoniDlg(this);
		m_pComMoni->Create(IDD_COMMONIDLG, CWnd::GetDesktopWindow());
		m_pComMoni->ShowWindow(SW_SHOW);
	} else
		m_pComMoni->DestroyWindow();
}

void CComSock::OnReceive(int nFlags)
{
	int n;
	BYTE buff[COMBUFSIZE];

	m_RecvSema.Lock();

	while ( (n = m_RecvBuff.GetSize()) > 0 ) {
		if ( n > COMBUFSIZE )
			n = COMBUFSIZE;
		memcpy(buff, m_RecvBuff.GetPtr(), n);
		m_RecvBuff.Consume(n);
		m_RecvSema.Unlock();

		OnReceiveCallBack(buff, n, nFlags);
		m_RecvSema.Lock();
	}

	m_RecvSema.Unlock();
}
void CComSock::OnRecvEmpty()
{
	m_RecvSema.Lock();
	m_pRecvEvent->SetEvent();
	m_RecvSema.Unlock();
}
void CComSock::OnSend()
{
	// AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, WSAMAKESELECTREPLY(FD_WRITE, 0));
	// Send Empty Call

	CExtSocket::OnSendEmpty();
}

void CComSock::OnReadWriteProc()
{
	DWORD n;
	BOOL bRes;
	BOOL bSendWait = FALSE;
	int HandleCount = 0;
	HANDLE HandleTab[6];
	DWORD HaveError = 0;
	DWORD ComEvent  = 0;
	DWORD ReadByte  = 0;
	DWORD WriteByte = 0;
	DWORD WriteTop = 0;
	DWORD WriteDone = 0;
	BYTE *ReadBuf  = new BYTE[COMBUFSIZE];
	BYTE *WriteBuf = new BYTE[COMBUFSIZE];
	CEvent ReadTimerEvent(FALSE, TRUE);
	CEvent WriteTimerEvent(FALSE, TRUE);
	MMRESULT WriteTimerHandle = NULL;
	BOOL bCommOverLap = FALSE;
	int WriteWaitMsec = 0;
	int TimerWaitMsec = 0;
	clock_t WriteReqClock = clock();
	clock_t WriteNextClock = clock();
	DWORD LastModemStatus;
	enum { READ_HAVE_DATA = 0, READ_RX_WAIT, READ_OVERLAP, READ_EVENT_WAIT } ReadStat = READ_HAVE_DATA;
	enum { WRITE_CHECK_DATA = 0, WRITE_TX_WAIT, WRITE_OVERLAP, WRITE_EVENT_WAIT, WRITE_TIMER_WAIT } WriteStat = WRITE_CHECK_DATA;

	GetCommModemStatus(m_hCom, &LastModemStatus);

	while ( m_ThreadMode == THREAD_RUN ) {

		// WaitCommEvent or CommOverLap
		for ( ; ; ) {
			if ( bCommOverLap )
				bRes = GetOverlappedResult(m_hCom, &m_CommOverLap, &n, FALSE);
			else
				bRes = WaitCommEvent(m_hCom, &ComEvent, &m_CommOverLap);

			if ( bRes ) {
				if ( ReadStat == READ_RX_WAIT && (ComEvent & (EV_RXCHAR | EV_RX80FULL | EV_DSR | EV_CTS | EV_RING | EV_RLSD)) != 0 )
					ReadStat = READ_HAVE_DATA;

				if ( WriteStat == WRITE_TX_WAIT && (ComEvent & (EV_TXEMPTY | EV_DSR | EV_CTS | EV_RLSD)) != 0 )
					WriteStat = WRITE_CHECK_DATA;

				if ( (ComEvent & (EV_CTS | EV_DSR | EV_RLSD | EV_BREAK | EV_ERR | EV_RING)) != 0 ) {
					ClearCommError(m_hCom, &n, NULL); m_CommError |= n;
					GetCommModemStatus(m_hCom, &m_ModemStatus);

					if ( m_pComMoni != NULL && m_pComMoni->m_hWnd != NULL && m_pComMoni->m_bActive )
						m_pComMoni->PostMessage(WM_COMMAND, (WPARAM)ID_COMM_EVENT_MONI);

					if ( (m_ModemStatus & MS_DSR_ON) == 0 && (LastModemStatus & MS_DSR_ON) != 0 && m_pComConf->dcb.fDsrSensitivity != 0 )
						AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, WSAMAKESELECTREPLY(FD_CLOSE, 0));

					LastModemStatus = m_ModemStatus;
				}

				if ( !bCommOverLap )
					break;

				bCommOverLap = FALSE;

			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				bCommOverLap = TRUE;
				break;

			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// ReadFile or ReadOverLap
		for ( ; ; ) {
			if ( ReadStat == READ_OVERLAP )
				bRes = GetOverlappedResult(m_hCom, &m_ReadOverLap, &n, FALSE);
			else if ( ReadStat == READ_HAVE_DATA  ) {
				ClearCommError(m_hCom, &n, NULL);
				bRes = ReadFile(m_hCom, ReadBuf, COMBUFSIZE, &n, &m_ReadOverLap);
			} else
				break;

			if ( bRes ) {
				if ( n > 0 ) {
					ReadByte += n;
					m_RecvSema.Lock();
					m_RecvByteSec += n;
					m_RecvBuff.Apend(ReadBuf, n);
					if ( (CExtSocket::GetRecvSize() + m_RecvBuff.GetSize()) > (COMBUFSIZE * 4) ) {
						m_pRecvEvent->ResetEvent();
						ReadStat = READ_EVENT_WAIT;
					} else
						ReadStat = READ_HAVE_DATA;
					m_RecvSema.Unlock();
				} else
					ReadStat = READ_RX_WAIT;

			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				ReadStat = READ_OVERLAP;
				break;

			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// PostMsg Call OnReceive
		if ( ReadByte > 0 ) {
			ReadByte = 0;
			AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, WSAMAKESELECTREPLY(FD_READ, 0));
		}

		// WriteOverLap
		if ( WriteStat == WRITE_OVERLAP ) {
			if ( GetOverlappedResult(m_hCom, &m_WriteOverLap, &n, FALSE) ) {
				if ( n > 0 && (WriteTop += n) >= WriteByte && bSendWait ) {
					// COMデバイスにより完了タイミングが異なる
					// UARTデバイスはFIFOのようでm_MsecByte * (WriteByte - FifoSize)
					// USBシリアルはメインメモリのようで0msec
					// EV_TXEMPTYも同様で実送信完了は、判断できない
					// 送信途中にCTSコントールがあった場合を考慮して論理時間以上の場合は、
					// この完了から送信遅延を行う
					clock_t now = clock();
					if ( (now - WriteReqClock) < (m_MsecByte * (int)WriteByte) )
						WriteNextClock = WriteReqClock + m_MsecByte * WriteByte + WriteWaitMsec;
					else
						WriteNextClock = now + WriteWaitMsec;
				}
				WriteStat = WRITE_CHECK_DATA;
			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				WriteStat = WRITE_OVERLAP;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// Check ThreadCommand and WriteData
		if ( WriteStat == WRITE_CHECK_DATA ) {
			// SendBreak
			if ( m_SendBreak != 0 ) {
				m_bRetCode = SetCommBreak(m_hCom);
				Sleep(m_SendBreak);
				ClearCommBreak(m_hCom);

				m_SendSema.Lock();
				m_SendBreak = 0;
				m_SendSema.Unlock();
			}

			// CommState
			if ( m_bCommState ) {
				if ( m_pComConf != NULL ) {
					m_bRetCode = SetCommState(m_hCom, &(m_pComConf->dcb));
					EscapeCommFunction(m_hCom, m_pComConf->dcb.fDtrControl == DTR_CONTROL_DISABLE ? CLRDTR : SETDTR);
					EscapeCommFunction(m_hCom, m_pComConf->dcb.fRtsControl == RTS_CONTROL_DISABLE ? CLRRTS : SETRTS);
				} else
					m_bRetCode = FALSE;

				m_SendSema.Lock();
				m_bCommState = FALSE;
				m_pStatEvent->SetEvent();
				m_SendSema.Unlock();
			}

			// GetStatus
			if ( m_bGetStatus ) {
				m_bRetCode = GetCommModemStatus(m_hCom, &m_ModemStatus);

				m_SendSema.Lock();
				m_bGetStatus = FALSE;
				m_pStatEvent->SetEvent();
				m_SendSema.Unlock();
			}

			// Set Com DTR/RTS
			if ( m_ComCtrl != 0 ) {
				m_bRetCode = EscapeCommFunction(m_hCom, m_ComCtrl);

				m_SendSema.Lock();
				m_ComCtrl = 0;
				m_pStatEvent->SetEvent();
				m_SendSema.Unlock();
			}
		}

		// WriteFile Loop
		while ( WriteStat == WRITE_CHECK_DATA ) {
			if ( WriteTop >= WriteByte ) {
				WriteDone = WriteByte;
				WriteTop = WriteByte = 0;
				WriteWaitMsec = 0;
				m_SendSema.Lock();
				if ( m_SendBuff.GetSize() > 0 ) {
					WriteReqClock = clock();
					if ( bSendWait && WriteNextClock > WriteReqClock ) {
						m_SendSema.Unlock();
						TimerWaitMsec =  (WriteNextClock - WriteReqClock) * 1000 / CLOCKS_PER_SEC;
						WriteStat = WRITE_TIMER_WAIT;
						break;
					}

					if ( (WriteByte = m_SendBuff.GetSize()) > COMBUFSIZE )
						WriteByte = COMBUFSIZE;
					memcpy(WriteBuf, m_SendBuff.GetPtr(), WriteByte);

					BYTE CrLf = (m_pDocument->m_TextRam.m_SendCrLf == 0 ? '\r' : '\n');

					if ( m_SendWait[0] != 0 ) {
						if ( m_SendWait[1] != 0 && WriteBuf[0] == CrLf )
							WriteWaitMsec = m_SendWait[1];
						else
							WriteWaitMsec = m_SendWait[0];

						WriteByte = 1;
						bSendWait = TRUE;
					} else if ( m_SendWait[1] != 0 ) {
						for ( n = 0 ; n < WriteByte ; ) {
							if ( WriteBuf[n++] == CrLf ) {
								WriteWaitMsec = m_SendWait[1];
								break;
							}
						}

						WriteByte = n;
						bSendWait = TRUE;
					} else
						bSendWait = FALSE;

					m_SendBuff.Consume(WriteByte);
					m_SendByteSec += WriteByte;
					m_SendSema.Unlock();

				} else {
					m_pSendEvent->ResetEvent();
					m_SendSema.Unlock();
					if ( WriteDone > 0 )
						AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, WSAMAKESELECTREPLY(FD_WRITE, 0));
					WriteStat = WRITE_EVENT_WAIT;
					break;
				}
			}

			ClearCommError(m_hCom, &n, NULL);
			if ( WriteFile(m_hCom, WriteBuf + WriteTop, WriteByte - WriteTop, &n, &m_WriteOverLap) ) {
				if ( n > 0 ) {
					if ( (WriteTop += n) >= WriteByte && bSendWait ) {
						clock_t now = clock();
						if ( (now - WriteReqClock) < (m_MsecByte * (int)WriteByte) )
							WriteNextClock = WriteReqClock + m_MsecByte * WriteByte + WriteWaitMsec;
						else
							WriteNextClock = now + WriteWaitMsec;
					}
				} else
					WriteStat = WRITE_TX_WAIT;

			} else if ( (n = ::GetLastError()) == ERROR_IO_PENDING ) {
				WriteStat = WRITE_OVERLAP;

			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// Event Set
		HandleCount = 0;

		switch(ReadStat) {
		case READ_EVENT_WAIT:
			HandleTab[HandleCount++] = m_pRecvEvent->m_hObject;
			break;
		case READ_OVERLAP:
			HandleTab[HandleCount++] = m_ReadOverLap.hEvent;
			break;
		case READ_RX_WAIT:
			// CommEvent EV_RXCHAR Recovery Timer
			ReadTimerEvent.ResetEvent();
			timeSetEvent(500, 50, (LPTIMECALLBACK)(ReadTimerEvent.m_hObject), 0, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
			HandleTab[HandleCount++] = ReadTimerEvent.m_hObject;
			break;
		}

		switch(WriteStat) {
		case WRITE_EVENT_WAIT:
			HandleTab[HandleCount++] = m_pSendEvent->m_hObject;
			break;
		case WRITE_OVERLAP:
			HandleTab[HandleCount++] = m_WriteOverLap.hEvent;
			break;
		case WRITE_TX_WAIT:
			// CommEvent EV_TXEMPTY Recovery Timer
			WriteTimerEvent.ResetEvent();
			timeSetEvent(500, 50, (LPTIMECALLBACK)(WriteTimerEvent.m_hObject), 0, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
			HandleTab[HandleCount++] = WriteTimerEvent.m_hObject;
			break;
		case WRITE_TIMER_WAIT:
			if ( WriteTimerHandle == NULL ) {
				WriteTimerEvent.ResetEvent();
				WriteTimerHandle = timeSetEvent(TimerWaitMsec, 0, (LPTIMECALLBACK)(WriteTimerEvent.m_hObject), 0, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
			}
			HandleTab[HandleCount++] = WriteTimerEvent.m_hObject;
			break;
		}

		if ( bCommOverLap )
			HandleTab[HandleCount++] = m_CommOverLap.hEvent;

		TRACE("ComThredProc CommOverLap=%d, ReadStat=%d, WriteStat=%d, WaitFor=%d, CommEvent=%04x, CommError=%04x\n",
			bCommOverLap, ReadStat, WriteStat, HandleCount, ComEvent, m_CommError);

		// Wait For Event
		ASSERT(HandleCount > 0);
		n = WaitForMultipleObjects(HandleCount, HandleTab, FALSE, INFINITE);
		if ( n >= WAIT_OBJECT_0 && (n -= WAIT_OBJECT_0) < (DWORD)HandleCount ) {
			if ( HandleTab[n] == m_pRecvEvent->m_hObject && ReadStat == READ_EVENT_WAIT )
				ReadStat = READ_HAVE_DATA;
			else if ( HandleTab[n] == ReadTimerEvent.m_hObject && ReadStat == READ_RX_WAIT )
				ReadStat = READ_HAVE_DATA;
			else if ( HandleTab[n] == m_pSendEvent->m_hObject && WriteStat == WRITE_EVENT_WAIT )
				WriteStat = WRITE_CHECK_DATA;
			else if ( HandleTab[n] == WriteTimerEvent.m_hObject && WriteStat == WRITE_TX_WAIT )
				WriteStat = WRITE_CHECK_DATA;
			else if ( HandleTab[n] == WriteTimerEvent.m_hObject && WriteStat == WRITE_TIMER_WAIT ) {
				WriteTimerHandle = NULL;
				WriteStat = WRITE_CHECK_DATA;
			}
		}
	}

ERRENDOF:
	if ( HaveError != 0 )
		AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, WSAMAKESELECTREPLY(0, HaveError));

	if ( ReadStat == READ_OVERLAP || WriteStat == WRITE_OVERLAP || bCommOverLap )
		CancelIo(m_hCom);

	delete [] ReadBuf;
	delete [] WriteBuf;
}

static struct _ComTab {
		LPCTSTR	name;
		int		mode;
		int		value;
	} ComTab[] = {
		{	_T("110"),		1,	CBR_110		},
		{	_T("300"),		1,	CBR_300		},
		{	_T("600"),		1,	CBR_600		},
		{	_T("1200"),		1,	CBR_1200	},
		{	_T("2400"),		1,	CBR_2400	},
		{	_T("4800"),		1,	CBR_4800	},
		{	_T("9600"),		1,	CBR_9600	},
		{	_T("14400"),	1,	CBR_14400	},
		{	_T("19200"),	1,	CBR_19200	},
		{	_T("28800"),	1,	28800		},
		{	_T("38400"),	1,	CBR_38400	},
		{	_T("56000"),	1,	CBR_56000	},
		{	_T("57600"),	1,	CBR_57600	},
		{	_T("115200"),	1,	CBR_115200	},
		{	_T("128000"),	1,	CBR_128000	},
		{	_T("230400"),	1,	230400		},
		{	_T("256000"),	1,	CBR_256000	},
		{	_T("460800"),	1,	460800		},
		{	_T("512000"),	1,	512000		},
		{	_T("921600"),	1,	921600		},

		{	_T("5"),		2,	5			},
		{	_T("6"),		2,	6			},
		{	_T("7"),		2,	7			},
		{	_T("8"),		2,	8			},

		{	_T("NOP"),		3,	NOPARITY	},
		{	_T("ODD"),		3,	ODDPARITY	},
		{	_T("EVEN"),		3,	EVENPARITY	},
		{	_T("MARK"),		3,	MARKPARITY	},
		{	_T("SPC"),		3,	SPACEPARITY	},

		{	_T("1"),		4,	ONESTOPBIT		},
		{	_T("1.5"),		4,	ONE5STOPBITS	},
		{	_T("2"),		4,	TWOSTOPBITS		},

		{	_T("NOC"),		5,	COMFLOW_NONE	},
		{	_T("CTS"),		5,	COMFLOW_RTSCTS	},
		{	_T("XON"),		5,	COMFLOW_XONXOFF	},

		{	NULL,			0,	0			},
	};

BOOL CComSock::LoadComConf(LPCTSTR ComSetStr, int ComPort, BOOL bOpen)
{
	int n, i;
	DWORD sz = sizeof(COMMCONFIG);
	BYTE *temp = new BYTE[sz];
	CStringArrayExt param;
	HANDLE hCom = INVALID_HANDLE_VALUE;
	CString ComName;

	ZeroMemory(temp, sz);

	// ComSetStrをNULLでデバイスの規定値を取得
	if ( ComSetStr != NULL )
		param.GetString(ComSetStr, _T(';'));

	// ComSetStr内のCOM設定を優先（古いバージョン互換の為）
	for ( n = 0 ; n < param.GetSize() ; n++ ) {
		if ( _tcsnicmp(param[n], _T("COM"), 3) == 0 ) {
			ComPort = _tstoi((LPCTSTR)param[n] + 3);
			break;
		}
	}

	// CExtSocket::GetPortNum()では、マイナス値を返す！
	if ( ComPort < 0 )
		ComPort = 0 - ComPort;
	else if ( ComPort == 0 ) {
		::SetLastError(ERROR_PATH_NOT_FOUND);
		return FALSE;
	}
	m_ComPort = ComPort;

	// GetDefaultCommConfigは、\\.\COMnを使用しない
	ComName.Format(_T("COM%d"), ComPort);
	if ( !GetDefaultCommConfig(ComName, (LPCOMMCONFIG)temp, &sz) ) {
		if ( ComPort >= 10 )
			ComName.Format(_T("\\\\.\\COM%d"), ComPort);

		if ( (hCom = CreateFile(ComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE )
			return FALSE;

		if ( !GetCommConfig(hCom, (LPCOMMCONFIG)temp, &sz) ) {
			::CloseHandle(hCom);
			return FALSE;
		}
	}

	// ReSize COMMCONFIG ?
	if ( sz > sizeof(COMMCONFIG) ) {
		delete [] temp;
		temp = new BYTE[sz];
		ZeroMemory(temp, sz);

		if ( hCom == INVALID_HANDLE_VALUE ) {
			if ( !GetDefaultCommConfig(ComName, (LPCOMMCONFIG)temp, &sz) )
				return FALSE;
		} else  if ( !GetCommConfig(hCom, (LPCOMMCONFIG)temp, &sz) ) {
			::CloseHandle(hCom);
			return FALSE;
		}
	}

	if ( bOpen ) { 
		if ( hCom == INVALID_HANDLE_VALUE ) {
			if ( ComPort >= 10 )
				ComName.Format(_T("\\\\.\\COM%d"), ComPort);
			if ( (m_hCom = CreateFile(ComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE )
				return FALSE;
		} else
			m_hCom = hCom;
	} else if ( hCom != INVALID_HANDLE_VALUE )
		::CloseHandle(hCom);

	if ( m_pComConf != NULL )
		delete m_pComConf;
	m_pComConf = (COMMCONFIG *)temp;

	// Default DCB Parameter

	//	XoffLim	受信バッファの空き容量が XoffLim バイト未満になると 
	//			XoffChar が送信され、 受信不可能であることを示します。 
	m_pComConf->dcb.XoffLim  = COMQUEUESIZE - COMQUEUESIZE * 75 / 100;

	//	XonLim	受信バッファのデータが XonLim バイト以下になると 
	//			XonChar が送信され、 受信可能であることを示します。  
	m_pComConf->dcb.XonLim   = COMQUEUESIZE * 25 / 100;

	m_pComConf->dcb.XonChar  = 0x11;	// ^Q
	m_pComConf->dcb.XoffChar = 0x13;	// ^S
	m_SendWait[0] = 0;
	m_SendWait[1] = 0;

	for ( n = 0 ; n < param.GetSize() ; n++ ) {
		for ( i = 0 ; ComTab[i].name != NULL ; i++ ) {
			if ( param[n].Compare(ComTab[i].name) == 0 ) {
				switch(ComTab[i].mode) {
				case 1: m_pComConf->dcb.BaudRate = ComTab[i].value; break;
				case 2: m_pComConf->dcb.ByteSize = ComTab[i].value; break;
				case 3: m_pComConf->dcb.Parity   = ComTab[i].value; break;
				case 4: m_pComConf->dcb.StopBits = ComTab[i].value; break;
				case 5: SetFlowCtrlMode(&(m_pComConf->dcb), ComTab[i].value, NULL); break;
				}
				break;
			}
		}
		if ( ComTab[i].name != NULL )
			continue;

		if ( _tcsncmp(param[n], _T("USER="), 5) == 0 ) {
			SetFlowCtrlMode(&(m_pComConf->dcb), COMFLOW_USERDEF, (LPCTSTR)param[n] + 5);

		} else if ( _tcsncmp(param[n], _T("XOFFL="), 6) == 0 ) {
			m_pComConf->dcb.XoffLim = COMQUEUESIZE - COMQUEUESIZE * _tstoi((LPCTSTR)param[n] + 6) / 100;
			if ( m_pComConf->dcb.XoffLim < (COMQUEUESIZE - COMQUEUESIZE * 95 / 100) )
				m_pComConf->dcb.XoffLim = (COMQUEUESIZE - COMQUEUESIZE * 95 / 100);
			else if ( m_pComConf->dcb.XoffLim > (COMQUEUESIZE - COMQUEUESIZE * 50 / 100) )
				m_pComConf->dcb.XoffLim = (COMQUEUESIZE - COMQUEUESIZE * 50 / 100);

		} else if ( _tcsncmp(param[n], _T("XONL="), 5) == 0 ) {
			m_pComConf->dcb.XonLim = COMQUEUESIZE * _tstoi((LPCTSTR)param[n] + 5) / 100;
			if ( m_pComConf->dcb.XonLim > (COMQUEUESIZE * 50 / 100) )
				m_pComConf->dcb.XonLim = (COMQUEUESIZE * 50 / 100);
			else if ( m_pComConf->dcb.XonLim < (COMQUEUESIZE * 5 / 100) )
				m_pComConf->dcb.XonLim = (COMQUEUESIZE * 5 / 100);

		} else if ( _tcsncmp(param[n], _T("XONC="), 5) == 0 ) {
			m_pComConf->dcb.XonChar = (char)_tstoi((LPCTSTR)param[n] + 5);
		} else if ( _tcsncmp(param[n], _T("XOFFC="), 6) == 0 ) {
			m_pComConf->dcb.XoffChar = (char)_tstoi((LPCTSTR)param[n] + 6);

		} else if ( _tcsncmp(param[n], _T("WC="), 3) == 0 ) {
			m_SendWait[0] = _tstoi((LPCTSTR)param[n] + 3);
		} else if ( _tcsncmp(param[n], _T("WL="), 3) == 0 ) {
			m_SendWait[1] = _tstoi((LPCTSTR)param[n] + 3);

		} else if ( (i = _tstoi(param[n])) >= 100 ) {
			m_pComConf->dcb.BaudRate = i;
		}
	}

	return TRUE;
}
BOOL CComSock::SaveComConf(CString &str)
{
	int n, r;
	int b = 0;
	CString tmp;

	str.Empty();

	if ( m_pComConf == NULL )
		return FALSE;

	for ( n = 0 ; ComTab[n].name != NULL ; n++ ) {
		switch(ComTab[n].mode) {
		case 1: r = (m_pComConf->dcb.BaudRate == ComTab[n].value ? TRUE : FALSE); break;
		case 2: r = (m_pComConf->dcb.ByteSize == ComTab[n].value ? TRUE : FALSE); break;
		case 3: r = (m_pComConf->dcb.Parity   == ComTab[n].value ? TRUE : FALSE); break;
		case 4: r = (m_pComConf->dcb.StopBits == ComTab[n].value ? TRUE : FALSE); break;
		}
		if ( r == TRUE ) {
			str += ComTab[n].name;
			str += _T(";");
			b |= (1 << ComTab[n].mode);
		}
	}

	if ( (b & (1 << 1)) == 0 ) {
		tmp.Format(_T("%d;"), m_pComConf->dcb.BaudRate);
		str += tmp;
	}

	n = GetFlowCtrlMode(&(m_pComConf->dcb), tmp);
	switch(n) {
	case COMFLOW_NONE:		str += _T("NOC;"); break;
	case COMFLOW_RTSCTS:	str += _T("CTS;"); break;
	case COMFLOW_XONXOFF:	str += _T("XON;"); break;
	case COMFLOW_USERDEF:	str += _T("USER="); str += tmp; str += _T(";"); break;
	}

	if ( m_pComConf->dcb.XoffLim != (COMQUEUESIZE - COMQUEUESIZE * 75 / 100) ) {
		tmp.Format(_T("XOFFL=%d;"), 100 - 100 * m_pComConf->dcb.XoffLim / COMQUEUESIZE);
		str += tmp;
	}
	if ( m_pComConf->dcb.XonLim != (COMQUEUESIZE * 25 / 100) ) {
		tmp.Format(_T("XONL=%d;"), m_pComConf->dcb.XonLim * 100 / COMQUEUESIZE);
		str += tmp;
	}
	if ( m_pComConf->dcb.XonChar != 0x11 ) {
		tmp.Format(_T("XONC=%d;"), m_pComConf->dcb.XonChar);
		str += tmp;
	}
	if ( m_pComConf->dcb.XoffChar != 0x13 ) {
		tmp.Format(_T("XOFFC=%d;"), m_pComConf->dcb.XoffChar);
		str += tmp;
	}

	if ( m_SendWait[0] != 0 ) {
		tmp.Format(_T("WC=%d;"), m_SendWait[0]);
		str += tmp;
	}
	if ( m_SendWait[1] != 0 ) {
		tmp.Format(_T("WL=%d;"), m_SendWait[1]);
		str += tmp;
	}

	return TRUE;
}
BOOL CComSock::SetupComConf(CString &ComSetStr, int &ComPort, CWnd *pOwner)
{
	CComInitDlg dlg(pOwner);

	if ( !LoadComConf(ComSetStr, ComPort) ) {
		ThreadMessageBox(_T("COM%d Load Config Error '%s'"), ComPort, ComSetStr);
		return FALSE;
	}

	dlg.m_pSock = this;

	if ( dlg.DoModal() != IDOK )
		return FALSE;

	if ( !SaveComConf(ComSetStr) )
		return FALSE;

	if ( ComPort != m_ComPort )
		ComPort = m_ComPort;

	return TRUE;
}

void CComSock::SetFlowCtrlMode(DCB *pDCB, int FlowCtrl, LPCTSTR UserDef)
{
	switch(FlowCtrl) {
	case COMFLOW_NONE:
		pDCB->fDtrControl		= DTR_CONTROL_ENABLE;
		pDCB->fOutxDsrFlow		= FALSE;
		pDCB->fDsrSensitivity	= FALSE;
		pDCB->fTXContinueOnXoff	= FALSE;
		pDCB->fRtsControl		= RTS_CONTROL_ENABLE;
		pDCB->fOutxCtsFlow		= FALSE;
		pDCB->fInX				= FALSE;
		pDCB->fOutX				= FALSE;
		break;
	case COMFLOW_RTSCTS:
		pDCB->fDtrControl		= DTR_CONTROL_ENABLE;
		pDCB->fOutxDsrFlow		= FALSE;
		pDCB->fDsrSensitivity	= FALSE;
		pDCB->fTXContinueOnXoff	= FALSE;
		pDCB->fRtsControl		= RTS_CONTROL_HANDSHAKE;
		pDCB->fOutxCtsFlow		= TRUE;
		pDCB->fInX				= FALSE;
		pDCB->fOutX				= FALSE;
		break;
	case COMFLOW_XONXOFF:
		pDCB->fDtrControl		= DTR_CONTROL_ENABLE;
		pDCB->fOutxDsrFlow		= FALSE;
		pDCB->fDsrSensitivity	= FALSE;
		pDCB->fTXContinueOnXoff	= FALSE;
		pDCB->fRtsControl		= RTS_CONTROL_ENABLE;
		pDCB->fOutxCtsFlow		= FALSE;
		pDCB->fInX				= TRUE;
		pDCB->fOutX				= TRUE;
		break;
	case COMFLOW_USERDEF:
		if ( UserDef != NULL ) {
			LPCTSTR p = UserDef;
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fDtrControl			= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fOutxDsrFlow		= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fDsrSensitivity		= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fTXContinueOnXoff	= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fRtsControl			= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fOutxCtsFlow		= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fInX				= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fOutX				= *(p++) - _T('0');
		}
		break;
	}

}
int CComSock::GetFlowCtrlMode(DCB *pDCB, CString &UserDef)
{
	if (	pDCB->fDtrControl		== DTR_CONTROL_ENABLE &&
			pDCB->fOutxDsrFlow		== FALSE &&
			pDCB->fDsrSensitivity	== FALSE &&
			pDCB->fTXContinueOnXoff	== FALSE &&
			pDCB->fRtsControl		== RTS_CONTROL_ENABLE &&
			pDCB->fOutxCtsFlow		== FALSE &&
			pDCB->fInX				== FALSE &&
			pDCB->fOutX				== FALSE )
		return COMFLOW_NONE;
	else if (
			pDCB->fDtrControl		== DTR_CONTROL_ENABLE &&
			pDCB->fOutxDsrFlow		== FALSE &&
			pDCB->fDsrSensitivity	== FALSE &&
			pDCB->fTXContinueOnXoff	== FALSE &&
			pDCB->fRtsControl		== RTS_CONTROL_HANDSHAKE &&
			pDCB->fOutxCtsFlow		== TRUE &&
			pDCB->fInX				== FALSE &&
			pDCB->fOutX				== FALSE )
		return COMFLOW_RTSCTS;
	else if (
			pDCB->fDtrControl		== DTR_CONTROL_ENABLE &&
			pDCB->fOutxDsrFlow		== FALSE &&
			pDCB->fDsrSensitivity	== FALSE &&
			pDCB->fTXContinueOnXoff	== FALSE &&
			pDCB->fRtsControl		== RTS_CONTROL_ENABLE &&
			pDCB->fOutxCtsFlow		== FALSE &&
			pDCB->fInX				== TRUE &&
			pDCB->fOutX				== TRUE )
		return COMFLOW_XONXOFF;
	else {
		UserDef.Format(_T("%c%c%c%c%c%c%c%c"),
			pDCB->fDtrControl		+ _T('0'),
			pDCB->fOutxDsrFlow		+ _T('0'),
			pDCB->fDsrSensitivity	+ _T('0'),
			pDCB->fTXContinueOnXoff	+ _T('0'),
			pDCB->fRtsControl		+ _T('0'),
			pDCB->fOutxCtsFlow		+ _T('0'),
			pDCB->fInX				+ _T('0'),
			pDCB->fOutX				+ _T('0'));
		return COMFLOW_USERDEF;
	}
}
void CComSock::AliveComPort(BYTE pComAliveBits[COMALIVEBYTE])
{
	int n;
	TCHAR *p;
	int ReTry = 4;	// 64,128,256...
	int nBufLen = (64 * 1024);
	TCHAR *pDevBuf = new TCHAR[nBufLen];
	COMMCONFIG conf;
	CString name;

	ZeroMemory(pComAliveBits, COMALIVEBYTE);

RETRY:
	if ( QueryDosDevice(NULL, pDevBuf, nBufLen) > 0 ) {
		for ( p = pDevBuf ; *p != _T('\0') ; ) {
			if ( _tcsnicmp(p, _T("COM"), 3) == 0 && (n = _tstoi(p + 3)) > 0 && n < COMALIVEBITS )
				pComAliveBits[n / 8] |= (1 << (n % 8));
			while ( *(p++) != _T('\0') );
		}

	} else if ( --ReTry > 0 && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
		nBufLen *= 2;
		delete pDevBuf;
		pDevBuf = new TCHAR[nBufLen];
		goto RETRY;

	} else {
		for ( n = 1 ; n < 10 ; n++ ) {
			name.Format(_T("COM%d"), n);
			DWORD sz = sizeof(COMMCONFIG);
			if ( GetDefaultCommConfig(name, &conf, &sz) && n < COMALIVEBITS )
				pComAliveBits[n / 8] |= (1 << (n % 8));
		}
	}

	delete pDevBuf;
}
