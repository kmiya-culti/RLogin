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
}
CComSock::~CComSock()
{
	Close();
}

BOOL CComSock::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
	Close();
	GetMode(lpszHostAddress);

	CString tmp;
	tmp.Format((m_ComPort >= 10 ? _T("\\\\.\\COM%d") : _T("COM%d")), m_ComPort);

	if ( (m_hCom = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) == NULL ) {
		CString errmsg;
		errmsg.Format(_T("ComSocket Open Error '%s'"), lpszHostAddress);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
	}

	DWORD sz = sizeof(COMMCONFIG);
	GetCommConfig(m_hCom, &m_ComConf, &sz);
	GetCommTimeouts(m_hCom, &m_ComTime);

	SetConfig();
	SetCommState(m_hCom, &(m_ComConf.dcb));
	m_SaveFlowCtrl = m_FlowCtrl;

	m_ComTime.ReadIntervalTimeout         = 0;
	m_ComTime.ReadTotalTimeoutMultiplier  = 0;
	m_ComTime.ReadTotalTimeoutConstant    = 10;
	m_ComTime.WriteTotalTimeoutMultiplier = 0;
	m_ComTime.WriteTotalTimeoutConstant   = 10;

	SetCommTimeouts(m_hCom, &m_ComTime);

	GetApp()->SetSocketIdle(this);

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
	CloseHandle(m_hCom);
	m_hCom = NULL;
	GetApp()->DelSocketIdle(this);
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

	if ( m_SendBuff.GetSize() > 0 )
		m_SendBuff.Apend((LPBYTE)lpBuf, nBufLen);
	else {
		if ( m_hCom != NULL )
			WriteFile(m_hCom, lpBuf, nBufLen, &n, NULL);
		if ( (nBufLen -= n) > 0 )
			m_SendBuff.Apend((LPBYTE)lpBuf + n, nBufLen);
	}
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

int CComSock::OnIdle()
{
	DWORD n = 0;
	BYTE tmp[4096];

	if ( CExtSocket::OnIdle() )
		return TRUE;

	if ( m_hCom == NULL )
		return FALSE;

	if ( m_SendBuff.GetSize() > 0 ) {
		if ( WriteFile(m_hCom, m_SendBuff.GetPtr(), m_SendBuff.GetSize(), &n, NULL) && n > 0 )
			m_SendBuff.Consume(n);
	}

	for ( n = 0 ; ReadFile(m_hCom, tmp, 4096, &n, NULL) && n > 0 ; n = 0 )
		OnReciveCallBack(tmp, n, 0);

	return TRUE;
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