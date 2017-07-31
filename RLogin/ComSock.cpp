// ComSock.cpp: CComSock クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ComSock.h"
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
	m_hCom     = NULL;
	m_ComPort  = 1;
	m_BaudRate = CBR_9600;
	m_ByteSize = 8;
	m_Parity   = NOPARITY;
	m_StopBits = ONESTOPBIT;
	m_FlowCtrl = m_SaveFlowCtrl = 1;
	m_Type     = ESCT_COMDEV;

	memset(&m_ComConf, 0, sizeof(COMMCONFIG));

	m_ThreadMode  = 0;
	m_pThredEvent = new CEvent(FALSE, TRUE);
	m_pSendEvent  = new CEvent(FALSE, TRUE);
	m_pStatEvent  = new CEvent(FALSE, TRUE);

	memset(&m_ReadOverLap,  0, sizeof(OVERLAPPED));
	memset(&m_WriteOverLap, 0, sizeof(OVERLAPPED));
	memset(&m_CommOverLap,  0, sizeof(OVERLAPPED));

	m_ReadOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_WriteOverLap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_CommOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_SendBreak = 0;
	m_bCommState = FALSE;

	m_CommError = 0;
	m_ModemStatus = 0;
	m_bGetStatus = FALSE;

	SetRecvBufSize(1024);
}
CComSock::~CComSock()
{
	Close();

	delete m_pThredEvent;
	delete m_pSendEvent;
	delete m_pStatEvent;

	CloseHandle(m_ReadOverLap.hEvent);
	CloseHandle(m_WriteOverLap.hEvent);
	CloseHandle(m_CommOverLap.hEvent);
}

static UINT ComEventThread(LPVOID pParam)
{
	CComSock *pThis = (CComSock *)pParam;

	pThis->OnReadWriteProc();
	pThis->m_pThredEvent->SetEvent();
	pThis->m_ThreadMode = 3;
	return 0;
}

BOOL CComSock::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
	Close();
	GetMode(lpszHostAddress);

	CString tmp;
	tmp.Format((m_ComPort >= 10 ? _T("\\\\.\\COM%d") : _T("COM%d")), m_ComPort);

	if ( (m_hCom = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE ) {
		CString errmsg;
		errmsg.Format(_T("ComSocket Open Error '%s'"), lpszHostAddress);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		m_hCom = NULL;
		return FALSE;
	}

	DWORD sz = sizeof(COMMCONFIG);
	GetCommConfig(m_hCom, &m_ComConf, &sz);
	GetCommTimeouts(m_hCom, &m_ComTime);

	SetConfig();
	SetCommState(m_hCom, &(m_ComConf.dcb));
	PurgeComm(m_hCom, PURGE_TXABORT | PURGE_RXABORT);
	m_SaveFlowCtrl = m_FlowCtrl;

	m_ComTime.ReadIntervalTimeout         = 0;
	m_ComTime.ReadTotalTimeoutMultiplier  = 0;
	m_ComTime.ReadTotalTimeoutConstant    = 10;
	m_ComTime.WriteTotalTimeoutMultiplier = 0;
	m_ComTime.WriteTotalTimeoutConstant   = 10;
	SetCommTimeouts(m_hCom, &m_ComTime);

	SetCommMask(m_hCom, EV_RXCHAR | EV_RX80FULL | EV_DSR | EV_TXEMPTY | EV_CTS | EV_RING | EV_RLSD | EV_ERR);

	GetMainWnd()->SetAsyncSelect((SOCKET)m_hCom, this, 0);

	m_ThreadMode = 1;
	m_pThredEvent->ResetEvent();
	AfxBeginThread(ComEventThread, this, THREAD_PRIORITY_BELOW_NORMAL);

//	CExtSocket::OnConnect();
	AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, FD_CONNECT);

	return TRUE;
}
BOOL CComSock::AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);
}
void CComSock::Close()
{
	if ( m_hCom == NULL )
		return;

	//	GetApp()->DelSocketIdle(this);
	GetMainWnd()->DelAsyncSelect((SOCKET)m_hCom, this, FALSE);

	if ( m_ThreadMode != 0 ) {
		m_ThreadMode = 2;
		CancelIo(m_hCom);
		m_pSendEvent->SetEvent();
		SetEvent(m_ReadOverLap.hEvent);
		SetEvent(m_WriteOverLap.hEvent);
		SetEvent(m_CommOverLap.hEvent);
		WaitForSingleObject(m_pThredEvent->m_hObject, INFINITE);
		m_ThreadMode = 0;
	}

	SetCommMask(m_hCom, 0);
	PurgeComm(m_hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	CloseHandle(m_hCom);
	m_hCom = NULL;

	CExtSocket::Close();
}
void CComSock::SendBreak(int opt)
{
	if ( m_hCom == NULL )
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
void CComSock::SetXonXoff(int sw)
{
	if ( m_SaveFlowCtrl != 2 )
		return;

	m_SendSema.Lock();
	m_FlowCtrl = (sw ? 2 : 0);
	m_bCommState = TRUE;
	m_pSendEvent->SetEvent();
	m_pStatEvent->ResetEvent();
	m_SendSema.Unlock();

	if ( m_ThreadMode == 1 )
		WaitForSingleObject(m_pStatEvent->m_hObject, INFINITE);
}

void CComSock::GetStatus(CString &str)
{
	int n;
	CString tmp;

	CExtSocket::GetStatus(str);

	str += _T("\r\n");

	m_SendSema.Lock();
	n = m_RecvBuff.GetSize();
	m_SendSema.Unlock();
	tmp.Format(_T("Recive Buffer: %d Bytes\r\n"), n);
	str += tmp;

	m_RecvSema.Lock();
	n = m_SendBuff.GetSize();
	m_RecvSema.Unlock();
	tmp.Format(_T("Send Buffer: %d Bytes\r\n"), n);
	str += tmp;

	tmp.Format(_T("Thread Status: %d\r\n"), m_ThreadMode);
	str += tmp;

	m_SendSema.Lock();
	m_bGetStatus = TRUE;
	m_pSendEvent->SetEvent();
	m_pStatEvent->ResetEvent();
	m_SendSema.Unlock();

	if ( m_ThreadMode == 1 )
		WaitForSingleObject(m_pStatEvent->m_hObject, INFINITE);

	str += _T("\r\n");

	str += _T("Configuration: ");
	SetMode(tmp);
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
		if ( (m_CommError & CE_BREAK)    != 0 ) str += _T(" Recived Break,");
		if ( (m_CommError & CE_FRAME)    != 0 ) str += _T(" Frame Error,");
		if ( (m_CommError & CE_IOE)      != 0 ) str += _T(" I/O Error,");
		if ( (m_CommError & CE_OVERRUN)  != 0 ) str += _T(" Overrun Error,");
		if ( (m_CommError & CE_RXOVER)   != 0 ) str += _T(" Recive Overflow,");
		if ( (m_CommError & CE_RXPARITY) != 0 ) str += _T(" Parity Error,");
		if ( (m_CommError & CE_TXFULL)   != 0 ) str += _T(" Send Buffer Full,");
	}
	str += _T("\r\n");
}

void CComSock::OnRecive(int nFlags)
{
	int n;
	BYTE buff[1024];

	m_RecvSema.Lock();

	while ( (n = m_RecvBuff.GetSize()) > 0 ) {
		if ( n > 1024 )
			n = 1024;
		memcpy(buff, m_RecvBuff.GetPtr(), n);
		m_RecvBuff.Consume(n);
		m_RecvSema.Unlock();

		OnReciveCallBack(buff, n, nFlags);
		m_RecvSema.Lock();
	}

	m_RecvSema.Unlock();
}
void CComSock::OnReadWriteProc()
{
	DWORD n;
	int HandleCount = 0;
	HANDLE HandleTab[5];
	BOOL bReadOverLap   = FALSE;
	BOOL bWriteOverLap  = FALSE;
	BOOL bCommOverLap   = FALSE;
	BOOL bHaveRecvData  = TRUE;
	BOOL bHaveSendData  = TRUE;
	BOOL bHaveSendReady = TRUE;
	DWORD HaveError = 0;
	DWORD ComEvent  = 0;
	DWORD ReadByte  = 0;
	DWORD WriteByte = 0;
	DWORD WriteTop = 0;
	BYTE ReadBuf[1024];
	BYTE WriteBuf[1024];
	CEvent TimerEvent(FALSE, TRUE);

	while ( m_ThreadMode == 1 ) {

		HandleCount = 0;
		
		// SendBreak
		if ( m_SendBreak != 0 ) {
			SetCommBreak(m_hCom);
			Sleep(m_SendBreak);
			ClearCommBreak(m_hCom);

			m_SendSema.Lock();
			m_SendBreak = 0;
			m_SendSema.Unlock();
		}

		// CommState
		if ( m_bCommState ) {
			SetConfig();
			SetCommState(m_hCom, &(m_ComConf.dcb));

			m_SendSema.Lock();
			m_bCommState = FALSE;
			m_pStatEvent->SetEvent();
			m_SendSema.Unlock();
		}

		// GetStatus
		if ( m_bGetStatus ) {
			GetCommModemStatus(m_hCom, &m_ModemStatus);
			ClearCommError(m_hCom, &m_CommError, NULL);

			m_SendSema.Lock();
			m_bGetStatus = FALSE;
			m_pStatEvent->SetEvent();
			m_SendSema.Unlock();
		}

		// ReadOverLap
		if ( bReadOverLap ) {
			if ( GetOverlappedResult(m_hCom, &m_ReadOverLap, &n, FALSE) ) {
				if ( n > 0 ) {
					ReadByte += n;
					m_RecvSema.Lock();
					m_RecvBuff.Apend(ReadBuf, n);
					m_RecvSema.Unlock();
				} else
					bHaveRecvData = FALSE;
				bReadOverLap = FALSE;
			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_ReadOverLap.hEvent;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// WriteOverLap
		if ( bWriteOverLap ) {
			if ( GetOverlappedResult(m_hCom, &m_WriteOverLap, &n, FALSE) ) {
				if ( n > 0 )
					WriteTop += n;
				else
					bHaveSendReady = FALSE;
				bWriteOverLap = FALSE;
			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_WriteOverLap.hEvent;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// CommOverLap
		if ( bCommOverLap ) {
			if ( GetOverlappedResult(m_hCom, &m_CommOverLap, &n, FALSE) ) {
				if ( (ComEvent & (EV_TXEMPTY | EV_DSR | EV_CTS)) != 0 )
					bHaveSendReady = TRUE;
				if ( (ComEvent & (EV_RXCHAR | EV_RX80FULL | EV_DSR | EV_CTS | EV_RING | EV_RLSD)) != 0 )
					bHaveRecvData = TRUE;
				if ( (ComEvent & EV_ERR) != 0 )
					ClearCommError(m_hCom, &ComEvent, NULL);
				bCommOverLap = FALSE;
			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_CommOverLap.hEvent;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// WaitCommEvent
		if ( !bCommOverLap ) {
			if ( WaitCommEvent(m_hCom, &ComEvent, &m_CommOverLap) ) {
				if ( (ComEvent & (EV_TXEMPTY | EV_DSR | EV_CTS)) != 0 )
					bHaveSendReady = TRUE;
				if ( (ComEvent & (EV_RXCHAR | EV_RX80FULL | EV_DSR | EV_CTS | EV_RING | EV_RLSD)) != 0 )
					bHaveRecvData = TRUE;
				if ( (ComEvent & EV_ERR) != 0 )
					ClearCommError(m_hCom, &ComEvent, NULL);
			} else if ( (n = ::GetLastError()) == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_CommOverLap.hEvent;
				bCommOverLap = TRUE;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// ReadFile
		if ( !bReadOverLap && bHaveRecvData  ) {
			if ( ReadFile(m_hCom, ReadBuf, 1024, &n, &m_ReadOverLap) ) {
				if ( n > 0 ) {
					ReadByte += n;
					m_RecvSema.Lock();
					m_RecvBuff.Apend(ReadBuf, n);
					m_RecvSema.Unlock();
				} else
					bHaveRecvData = FALSE;
			} else if ( (n = ::GetLastError()) == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_ReadOverLap.hEvent;
				bReadOverLap = TRUE;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// PostMsg Call OnRecive
		if ( ReadByte > 0 ) {
			ReadByte = 0;
			AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, FD_READ);
		}

		// Check WriteData
		if ( !bWriteOverLap && bHaveSendData && WriteTop >= WriteByte ) {
			WriteTop = 0;
			m_SendSema.Lock();
			if ( (WriteByte = m_SendBuff.GetSize()) > 0 ) {
				if ( WriteByte > 1024 )
					WriteByte = 1024;
				memcpy(WriteBuf, m_SendBuff.GetPtr(), WriteByte);
				m_SendBuff.Consume(WriteByte);
				m_SendSema.Unlock();
			} else {
				m_pSendEvent->ResetEvent();
				m_SendSema.Unlock();
				bHaveSendData = FALSE;
			}
		}

		// WriteFile
		if ( !bWriteOverLap && bHaveSendReady && WriteTop < WriteByte ) {
			while ( WriteTop < WriteByte ) {
				if ( WriteFile(m_hCom, WriteBuf + WriteTop, WriteByte - WriteTop, &n, &m_WriteOverLap) ) {
					if ( n > 0 ) {
						WriteTop += n;
					} else {
						bHaveSendReady = FALSE;
						break;
					}
				} else if ( (n = ::GetLastError()) == ERROR_IO_PENDING ) {
					HandleTab[HandleCount++] = m_WriteOverLap.hEvent;
					bWriteOverLap = TRUE;
					break;
				} else {
					HaveError = n;
					goto ERRENDOF;
				}
			}
		}

		// ReadData Loop Check
		if ( !bReadOverLap && !bCommOverLap && bHaveRecvData ) {
			if ( !bWriteOverLap && !bHaveSendData && bHaveSendReady )
				bHaveSendData = TRUE;
			continue;
		}

		// WriteData Loop Check
		if ( !bWriteOverLap && bHaveSendData && bHaveSendReady ) {
			if ( !bReadOverLap && !bCommOverLap && !bHaveRecvData )
				bHaveRecvData = TRUE;
			continue;
		}

		// NoOverLap or NotSendReady Timer Set
		if ( HandleCount == 0 || !bHaveSendReady ) {
			TimerEvent.ResetEvent();
			timeSetEvent(300, 10, (LPTIMECALLBACK)(TimerEvent.m_hObject), 0, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
			HandleTab[HandleCount++] = TimerEvent.m_hObject;
		}

		// SendData Event Add
		if ( !bHaveSendData )
			HandleTab[HandleCount++] = m_pSendEvent->m_hObject;

		TRACE("PipeThredProc CommOverLap=%d, ReadOverLap=%d, WriteOverLap=%d, HaveRecvData=%d, HaveSendData=%d, WaitFor=%d\n",
			bCommOverLap, bReadOverLap, bWriteOverLap, bHaveRecvData, bHaveSendData, HandleCount);

		// Wait For Event
		ASSERT(HandleCount > 0);
		n = WaitForMultipleObjects(HandleCount, HandleTab, FALSE, INFINITE);
		if ( n >= WAIT_OBJECT_0 && (n -= WAIT_OBJECT_0) < (DWORD)HandleCount ) {
			if ( HandleTab[n] == m_pSendEvent->m_hObject )
				bHaveSendData = TRUE;
			else if ( HandleTab[n] == TimerEvent.m_hObject ) {
				bHaveRecvData  = TRUE;
				bHaveSendReady = TRUE;
			}
		}
	}

ERRENDOF:
	if ( HaveError != 0 )
		AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hCom, WSAMAKESELECTREPLY(0, HaveError));

	if ( bReadOverLap || bWriteOverLap || bCommOverLap )
		CancelIo(m_hCom);
}

static struct _ComTab {
		LPCTSTR	name;
		int		mode;
		int		value;
	} ComTab[] = {
		{	_T("COM1"),		0,	1			},
		{	_T("COM2"),		0,	2			},
		{	_T("COM3"),		0,	3			},
		{	_T("COM4"),		0,	4			},
		{	_T("COM5"),		0,	5			},
		{	_T("COM6"),		0,	6			},
		{	_T("COM7"),		0,	7			},
		{	_T("COM8"),		0,	8			},
		{	_T("COM9"),		0,	9			},
		{	_T("COM10"),	0,	10			},
		{	_T("COM11"),	0,	11			},
		{	_T("COM12"),	0,	12			},
		{	_T("COM13"),	0,	13			},
		{	_T("COM14"),	0,	14			},
		{	_T("COM15"),	0,	15			},
		{	_T("COM16"),	0,	16			},
		{	_T("COM17"),	0,	17			},
		{	_T("COM18"),	0,	18			},
		{	_T("COM19"),	0,	19			},
		{	_T("COM20"),	0,	20			},
		{	_T("COM21"),	0,	21			},
		{	_T("COM22"),	0,	22			},
		{	_T("COM23"),	0,	23			},
		{	_T("COM24"),	0,	24			},
		{	_T("COM25"),	0,	25			},
		{	_T("COM26"),	0,	26			},
		{	_T("COM27"),	0,	27			},
		{	_T("COM28"),	0,	28			},
		{	_T("COM29"),	0,	29			},
		{	_T("COM30"),	0,	30			},
		{	_T("COM31"),	0,	31			},

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

		{	_T("EVEN"),		3,	EVENPARITY	},
		{	_T("ODD"),		3,	ODDPARITY	},
		{	_T("NOP"),		3,	NOPARITY	},
		{	_T("MARK"),		3,	MARKPARITY	},
		{	_T("SPC"),		3,	SPACEPARITY	},

		{	_T("1"),		4,	ONESTOPBIT	},
		{	_T("1.5"),		4,	ONE5STOPBITS },
		{	_T("2"),		4,	TWOSTOPBITS	},

		{	_T("NOC"),		5,	0			},
		{	_T("CTS"),		5,	1			},
		{	_T("XON"),		5,	2			},

		{	NULL,			0,	0			},
	};

void CComSock::GetConfig()
{
	m_BaudRate = m_ComConf.dcb.BaudRate;
	m_ByteSize = m_ComConf.dcb.ByteSize;
	m_Parity   = m_ComConf.dcb.Parity;
	m_StopBits = m_ComConf.dcb.StopBits;
	m_FlowCtrl = (m_ComConf.dcb.fOutxCtsFlow ? 1 : (m_ComConf.dcb.fOutX ? 2 : 0));
}
void CComSock::SetConfig()
{
	m_ComName.Format(_T("COM%d"), m_ComPort);

	DWORD sz = sizeof(COMMCONFIG);
	GetDefaultCommConfig(m_ComName, &m_ComConf, &sz);

	m_ComConf.dcb.BaudRate		= m_BaudRate;
	m_ComConf.dcb.ByteSize		= m_ByteSize;
	m_ComConf.dcb.Parity		= m_Parity;
	m_ComConf.dcb.StopBits		= m_StopBits;

	switch(m_FlowCtrl) {
	case 0:		// Non
		m_ComConf.dcb.fDtrControl	= DTR_CONTROL_ENABLE;
		m_ComConf.dcb.fOutxDsrFlow	= FALSE;
		m_ComConf.dcb.fRtsControl	= RTS_CONTROL_ENABLE;
		m_ComConf.dcb.fOutxCtsFlow	= FALSE;
		m_ComConf.dcb.fInX			= FALSE;
		m_ComConf.dcb.fOutX			= FALSE;
		break;
	case 1:		// Haed
		m_ComConf.dcb.fDtrControl	= DTR_CONTROL_ENABLE;
		m_ComConf.dcb.fOutxDsrFlow	= FALSE;
		m_ComConf.dcb.fRtsControl	= RTS_CONTROL_HANDSHAKE;
		m_ComConf.dcb.fOutxCtsFlow	= TRUE;
		m_ComConf.dcb.fInX			= FALSE;
		m_ComConf.dcb.fOutX			= FALSE;
		break;
	case 2:		// XonOff
		m_ComConf.dcb.fDtrControl	= DTR_CONTROL_ENABLE;
		m_ComConf.dcb.fOutxDsrFlow	= FALSE;
		m_ComConf.dcb.fRtsControl	= RTS_CONTROL_ENABLE;
		m_ComConf.dcb.fOutxCtsFlow	= FALSE;
		m_ComConf.dcb.fInX			= TRUE;
		m_ComConf.dcb.fOutX			= TRUE;
		break;
	}
}

void CComSock::GetMode(LPCTSTR str)
{
	int i, a, b;
	CString work = str;
	CStringArray stra;

	stra.RemoveAll();
	for ( i = 0 ; i < work.GetLength() ; i = a + 1 ) {
		if ( (a = work.Find(_T(';'), i)) < 0 )
			a = work.GetLength();
		if ( (a - i) > 0 )
			stra.Add(work.Mid(i, a - i));
	}

	for ( i = 0 ; i < stra.GetSize() ; i++ ) {
		for ( a = 0 ; ComTab[a].name != NULL ; a++ ) {
			if ( stra[i].CompareNoCase(ComTab[a].name) == 0 ) {
				switch(ComTab[a].mode) {
				case 0: m_ComPort  = ComTab[a].value; break;
				case 1: m_BaudRate = ComTab[a].value; break;
				case 2: m_ByteSize = ComTab[a].value; break;
				case 3: m_Parity   = ComTab[a].value; break;
				case 4: m_StopBits = ComTab[a].value; break;
				case 5: m_FlowCtrl = ComTab[a].value; break;
				}
				break;
			}
		}
		if ( ComTab[a].name == NULL ) {
			if ( stra[i].Left(3).CompareNoCase(_T("COM")) == 0 ) {
				m_ComPort = _tstoi(stra[i].Mid(3));
			} else if ( (b = _tstoi(stra[i])) >= 100 ) {
				m_BaudRate = b;
			}
		}
	}

	m_ComName.Format(_T("COM%d"), m_ComPort);
}
void CComSock::SetMode(CString &str)
{
	int n, r, b = 0;
	CString tmp;

	str = _T("");

	for ( n = 0 ; ComTab[n].name != NULL ; n++ ) {
		switch(ComTab[n].mode) {
		case 0: r = (m_ComPort  == ComTab[n].value ? TRUE : FALSE); break;
		case 1: r = (m_BaudRate == ComTab[n].value ? TRUE : FALSE); break;
		case 2: r = (m_ByteSize == ComTab[n].value ? TRUE : FALSE); break;
		case 3: r = (m_Parity   == ComTab[n].value ? TRUE : FALSE); break;
		case 4: r = (m_StopBits == ComTab[n].value ? TRUE : FALSE); break;
		case 5: r = (m_FlowCtrl == ComTab[n].value ? TRUE : FALSE); break;
		}
		if ( r == TRUE ) {
			str += ComTab[n].name;
			str += _T(";");
			b |= (1 << ComTab[n].mode);
		}
	}
	if ( (b & (1 << 0)) == 0 ) {
		tmp.Format(_T("COM%d;"), m_ComPort);
		str += tmp;
	}
	if ( (b & (1 << 1)) == 0 ) {
		tmp.Format(_T("%d;"), m_BaudRate);
		str += tmp;
	}
}

void CComSock::ConfigDlg(CWnd *pWnd, CString &str)
{
	GetMode(str);
	SetConfig();

	if ( !CommConfigDialog(m_ComName, pWnd->m_hWnd, &m_ComConf) )
		return;

	GetConfig();
	SetMode(str);
}
DWORD CComSock::AliveComPort()
{
	int n;
	int port = 0;
	TCHAR *p;
	int ReTry = 4;	// 64,128,256...
	int nBufLen = (64 * 1024);
	TCHAR *pDevBuf = new TCHAR[nBufLen];
	COMMCONFIG conf;
	CString name;

RETRY:
	if ( QueryDosDevice(NULL, pDevBuf, nBufLen) > 0 ) {
		for ( p = pDevBuf ; *p != _T('\0') ; ) {
			if ( _tcsncmp(p, _T("COM"), 3) == 0 && (n = _tstoi(p + 3)) > 0 )
				port |= (1 << n);
			while ( *(p++) != _T('\0') );
		}

	} else if ( --ReTry > 0 && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
		nBufLen *= 2;
		delete pDevBuf;
		pDevBuf = new TCHAR[nBufLen];
		goto RETRY;

	} else {
		for ( n = 1 ; n <= 31 ; n++ ) {
			name.Format(_T("COM%d"), n);
			DWORD sz = sizeof(COMMCONFIG);
			if ( GetDefaultCommConfig(name, &conf, &sz) )
				port |= (1 << n);
		}
	}

	delete pDevBuf;

	return port;
}