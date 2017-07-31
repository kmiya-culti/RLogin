// ComSock.cpp: CComSock クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ComSock.h"

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

	m_ThreadEnd   = FALSE;
	m_pReadEvent  = new CEvent(FALSE, TRUE);
	m_pWriteEvent = new CEvent(FALSE, TRUE);
	m_pEndEvent   = new CEvent(FALSE, TRUE);

	memset(&m_ReadOverLap,  0, sizeof(OVERLAPPED));
	memset(&m_WriteOverLap, 0, sizeof(OVERLAPPED));

	m_ReadOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_WriteOverLap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_UseReadOverLap  = FALSE;
	m_UseWriteOverLap = FALSE;

	m_EventMask = 0;
	m_EventCode = 0;

	SetRecvBufSize(1024);
}
CComSock::~CComSock()
{
	Close();
	delete m_pReadEvent;
	delete m_pWriteEvent;
	delete m_pEndEvent;
}

#define	DEBUGLOG(...)

//void CComSock::DEBUGLOG(LPCSTR str, ...)
//{
//	CStringA tmp;
//	va_list arg;
//	va_start(arg, str);
//	tmp.FormatV(str, arg);
////	OnReciveCallBack((void *)(LPCSTR)tmp, tmp.GetLength(), 0);
//	TRACE(tmp);
//	va_end(arg);
//}

static UINT ComEventThread(LPVOID pParam)
{
	LPARAM fd;
	DWORD Mask;
	COMSTAT comstat;
	CComSock *pSock = (CComSock *)pParam;

	while ( !pSock->m_ThreadEnd ) {
		if ( (pSock->m_EventMask & EV_RXCHAR) != 0 && ClearCommError(pSock->m_hCom, &Mask, &comstat) && comstat.cbInQue > 0 ) {
			pSock->m_pReadEvent->ResetEvent();
			pSock->m_EventCode = EV_RXCHAR;
			AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)pSock->m_hCom, FD_READ);
			WaitForSingleObject(pSock->m_pReadEvent->m_hObject, INFINITE);
		}
		if ( WaitCommEvent(pSock->m_hCom, &Mask, NULL) ) {
			fd = 0;
			if ( (Mask & (EV_RXCHAR | EV_RX80FULL | EV_DSR | EV_RLSD | EV_ERR)) != 0 ) {
				fd |= FD_READ;
				pSock->m_pReadEvent->ResetEvent();
			}
			if ( (Mask & (EV_TXEMPTY | EV_CTS | EV_RING | EV_RLSD | EV_ERR)) != 0 ) {
				fd |= FD_WRITE;
				pSock->m_pWriteEvent->ResetEvent();
			}
			if ( fd == 0 )
				continue;
			pSock->m_EventCode = Mask;
			AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)pSock->m_hCom, fd);
			if ( (fd & FD_READ) != 0 )
				WaitForSingleObject(pSock->m_pReadEvent->m_hObject, INFINITE);
			if ( (fd & FD_WRITE) != 0 )
				WaitForSingleObject(pSock->m_pWriteEvent->m_hObject, INFINITE);
		} else
	      ClearCommError(pSock->m_hCom, &Mask, NULL);
	}
	pSock->m_pEndEvent->SetEvent();
	return 0;
}

void CComSock::ModEventMask(DWORD add, DWORD del)
{
	m_EventMask |= add;
	m_EventMask &= ~del;

	if ( m_hCom != NULL )
		SetCommMask(m_hCom, m_EventMask);
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

	ModEventMask(EV_RXCHAR | EV_RX80FULL | EV_DSR | EV_CTS | EV_RING | EV_RLSD | EV_ERR, 0);

//	GetApp()->SetSocketIdle(this);
	GetMainWnd()->SetAsyncSelect((SOCKET)m_hCom, this, 0);

	m_UseReadOverLap  = FALSE;
	m_UseWriteOverLap = FALSE;

	m_ThreadEnd = FALSE;
	m_pEndEvent->ResetEvent();
	AfxBeginThread(ComEventThread, this, THREAD_PRIORITY_NORMAL);

	if ( m_pDocument != NULL )
		m_pDocument->OnSocketConnect();

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

	m_ThreadEnd = TRUE;
	m_pReadEvent->SetEvent();
	m_pWriteEvent->SetEvent();
	m_EventMask = 0;
	SetCommMask(m_hCom, 0);
	WaitForSingleObject(m_pEndEvent->m_hObject, INFINITE);

	PurgeComm(m_hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	CloseHandle(m_hCom);
	m_hCom = NULL;
}
void CComSock::SendBreak(int opt)
{
	if ( m_hCom == NULL )
		return;
	SetCommBreak(m_hCom);
	Sleep(opt != 0 ? 300 : 100);
	ClearCommBreak(m_hCom);
}
int CComSock::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	DWORD n = 0;

	m_SendBuff.Apend((LPBYTE)lpBuf, nBufLen);
	if ( m_UseWriteOverLap == FALSE ) {
		while ( (m_WriteOverLapByte = m_SendBuff.GetSize()) > 0 ) {
			if ( (m_EventMask & EV_TXEMPTY) == 0 )
				ModEventMask(EV_TXEMPTY, 0);
			if ( m_WriteOverLapByte > 1024 ) 
				m_WriteOverLapByte = 1024;
			memcpy(m_WriteOverLapBuf, m_SendBuff.GetPtr(), m_WriteOverLapByte);
			if ( WriteFile(m_hCom, m_WriteOverLapBuf, m_WriteOverLapByte, &n, &m_WriteOverLap) ) {
				DEBUGLOG("Send Write %d(%d)\r\n", m_WriteOverLapByte, n);
				m_SendBuff.Consume(n);
				continue;
			} else if ( ::GetLastError() != ERROR_IO_PENDING ) {
				DEBUGLOG("Send Write Error %d\r\n", m_WriteOverLapByte);
				ClearCommError(m_hCom, &n, NULL);
				break;
			}
			DEBUGLOG("Send OverLap Wait %d\r\n", m_WriteOverLapByte);
			m_UseWriteOverLap = TRUE;
			break;
		}
		if ( !m_UseWriteOverLap && (m_EventMask & EV_TXEMPTY) != 0 )
			ModEventMask(0, EV_TXEMPTY);

	} else if ( (m_EventMask & EV_TXEMPTY) == 0 )
			ModEventMask(EV_TXEMPTY, 0);

	return nBufLen;
}
void CComSock::SetXonXoff(int sw)
{
	if ( m_SaveFlowCtrl != 2 )
		return;
	m_FlowCtrl = (sw ? 2 : 0);
	SetConfig();
	SetCommState(m_hCom, &(m_ComConf.dcb));
}

void CComSock::OnRecvEmpty()
{
	if ( (m_EventMask & EV_RXCHAR) == 0 )
		ModEventMask(EV_RXCHAR | EV_RX80FULL, 0);
}
void CComSock::OnRecive(int nFlags)
{
	DWORD n = 0;
	DWORD comerr;
	COMSTAT comstat;

	if ( m_hCom == NULL )
		goto ENDRET;

	if ( m_UseReadOverLap ) {
		if ( GetOverlappedResult(m_hCom, &m_ReadOverLap, &n, FALSE) ) {
			DEBUGLOG("OnRecive OverLap %d:%d\r\n", n, m_EventCode);
			OnReciveCallBack(m_ReadOverLapBuf, n, 0);
			m_UseReadOverLap = FALSE;
		} else if ( (n = ::GetLastError()) != ERROR_IO_INCOMPLETE && n != ERROR_IO_PENDING ) {
			DEBUGLOG("OnRecive OverLap Error %d\r\n", n);
			m_UseReadOverLap = FALSE;
			ClearCommError(m_hCom, &n, NULL);
			goto ENDRET;
		} else {
			DEBUGLOG("OnRecive OverLap incomplete %d\r\n", m_EventCode);
		}
	}

	if ( !m_UseReadOverLap && (m_EventMask & EV_RXCHAR) != 0 ) {
		for ( ; ; ) {
			if ( !ClearCommError(m_hCom, &comerr, &comstat) )
				break;
			DEBUGLOG("OnRecive InQue %d(%04x)\r\n", comstat.cbInQue, comerr);
			if ( comstat.cbInQue <= 0 )
				break;
			if ( ReadFile(m_hCom, m_ReadOverLapBuf, (comstat.cbInQue < 1024 ? comstat.cbInQue : 1024), &n, &m_ReadOverLap) ) {
				if ( n == 0 )
					break;
				DEBUGLOG("OnRecive Read %d\r\n", n);
				OnReciveCallBack(m_ReadOverLapBuf, n, 0);
			} else if ( ::GetLastError() == ERROR_IO_PENDING ) {
				if ( GetOverlappedResult(m_hCom, &m_ReadOverLap, &n, TRUE) ) {
					DEBUGLOG("OnRecive Read OverLap %d\r\n", n);
					OnReciveCallBack(m_ReadOverLapBuf, n, 0);
				} else {
					DEBUGLOG("OnRecive OverLap Wait\r\n");
					m_UseReadOverLap = TRUE;
					break;
				}
			} else {
				DEBUGLOG("OnRecive Read Error\r\n");
				ClearCommError(m_hCom, &comerr, NULL);
				break;
			}
			if ( IsOverFlow() ) {
				DEBUGLOG("OnRecive OverFlow %d\r\n", GetRecvSize());
				if ( (m_EventMask & EV_RXCHAR) != 0 )
					ModEventMask(0, EV_RXCHAR | EV_RX80FULL);
				break;
			}
		}
	}

ENDRET:
	m_pReadEvent->SetEvent();
}
void CComSock::OnSend()
{
	DWORD n = 0;

	if ( m_hCom == NULL )
		goto ENDRET;

	if ( m_UseWriteOverLap ) {
		if ( GetOverlappedResult(m_hCom, &m_WriteOverLap, &n, FALSE) ) {
			DEBUGLOG("OnSend OverLap %d(%d):%d\r\n", m_WriteOverLapByte, n, m_EventCode);
			m_SendBuff.Consume(n);
			m_UseWriteOverLap = FALSE;
		} else if ( (n = ::GetLastError()) != ERROR_IO_INCOMPLETE && n != ERROR_IO_PENDING ) {
			DEBUGLOG("OnSend OverLap Error %d\r\n", n);
			ModEventMask(0, EV_TXEMPTY);
			m_UseWriteOverLap = FALSE;
			ClearCommError(m_hCom, &n, NULL);
			goto ENDRET;
		} else {
			DEBUGLOG("OnSend OverLap incomplete %d\r\n", m_EventCode);
		}
	}

	if ( !m_UseWriteOverLap ) {
		while ( (m_WriteOverLapByte = m_SendBuff.GetSize()) > 0 ) {
			if ( (m_EventMask & EV_TXEMPTY) == 0 )
				ModEventMask(EV_TXEMPTY, 0);
			if ( m_WriteOverLapByte > 1024 )
				m_WriteOverLapByte = 1024;
			memcpy(m_WriteOverLapBuf, m_SendBuff.GetPtr(), m_WriteOverLapByte);
			if ( WriteFile(m_hCom, m_WriteOverLapBuf, m_WriteOverLapByte, &n, &m_WriteOverLap) ) {
				DEBUGLOG("OnSend Write %d(%d)\r\n", m_WriteOverLapByte, n);
				m_SendBuff.Consume(n);
				continue;
			} else if ( ::GetLastError() != ERROR_IO_PENDING ) {
				DEBUGLOG("OnSend Write Error %d\r\n", m_WriteOverLapByte);
				ClearCommError(m_hCom, &n, NULL);
				break;
			}
			DEBUGLOG("OnSend Write OverLap Wait %d\r\n", m_WriteOverLapByte);
			m_UseWriteOverLap = TRUE;
			break;
		}
		if ( !m_UseWriteOverLap ) {
			DEBUGLOG("OnSend Write Disable\r\n");
			if ( (m_EventMask & EV_TXEMPTY) != 0 )
				ModEventMask(0, EV_TXEMPTY);
		}
	}

ENDRET:
	m_pWriteEvent->SetEvent();
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
		m_ComConf.dcb.fRtsControl	= RTS_CONTROL_DISABLE;
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
		m_ComConf.dcb.fRtsControl	= RTS_CONTROL_DISABLE;
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
	DWORD port = 0;
	COMMCONFIG conf;
	CString name;

	for ( n = 1 ; n <= 31 ; n++ ) {
		name.Format(_T("COM%d"), n);
		DWORD sz = sizeof(COMMCONFIG);
		if ( GetDefaultCommConfig(name, &conf, &sz) )
			port |= (1 << n);
	}
	return port;
}