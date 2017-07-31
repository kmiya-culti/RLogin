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
	m_Type     = 5;

	memset(&m_ComConf, 0, sizeof(COMMCONFIG));
}
CComSock::~CComSock()
{
	Close();
}

BOOL CComSock::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	Close();
	GetMode(lpszHostAddress);

	if ( (m_hCom = CreateFile(m_ComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) == NULL ) {
		CString errmsg;
		errmsg.Format("ComSocket Open Error '%s'", lpszHostAddress);
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
		char	*name;
		int		mode;
		int		value;
	} ComTab[] = {
		{ "COM1",		0,	1			},
		{ "COM2",		0,	2			},
		{ "COM3",		0,	3			},
		{ "COM4",		0,	4			},
		{ "COM5",		0,	5			},
		{ "COM6",		0,	6			},
		{ "COM7",		0,	7			},
		{ "COM8",		0,	8			},
		{ "COM9",		0,	9			},
		{ "COM10",		0,	10			},
		{ "COM11",		0,	11			},
		{ "COM12",		0,	12			},
		{ "COM13",		0,	13			},
		{ "COM14",		0,	14			},
		{ "COM15",		0,	15			},
		{ "COM16",		0,	16			},

		{ "110",		1,	CBR_110		},
		{ "300",		1,	CBR_300		},
		{ "600",		1,	CBR_600		},
		{ "1200",		1,	CBR_1200	},
		{ "2400",		1,	CBR_2400	},
		{ "4800",		1,	CBR_4800	},
		{ "9600",		1,	CBR_9600	},
		{ "14400",		1,	CBR_14400	},
		{ "19200",		1,	CBR_19200	},
		{ "28800",		1,	28800		},
		{ "38400",		1,	CBR_38400	},
		{ "56000",		1,	CBR_56000	},
		{ "57600",		1,	CBR_57600	},
		{ "115200",		1,	CBR_115200	},
		{ "128000",		1,	CBR_128000	},
		{ "230400",		1,	230400		},
		{ "256000",		1,	CBR_256000	},
		{ "460800",		1,	460800		},
		{ "512000",		1,	512000		},
		{ "921600",		1,	921600		},

		{ "5",			2,	5			},
		{ "6",			2,	6			},
		{ "7",			2,	7			},
		{ "8",			2,	8			},

		{ "EVEN",		3,	EVENPARITY	},
		{ "ODD",		3,	ODDPARITY	},
		{ "NOP",		3,	NOPARITY	},
		{ "MARK",		3,	MARKPARITY	},
		{ "SPC",		3,	SPACEPARITY	},

		{ "1",			4,	ONESTOPBIT	},
		{ "1.5",		4,	ONE5STOPBITS },
		{ "2",			4,	TWOSTOPBITS	},

		{ "NOC",		5,	0			},
		{ "CTS",		5,	1			},
		{ "XON",		5,	2			},

		{ NULL,			0,	0			},
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
	m_ComName.Format("COM%d", m_ComPort);

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

void CComSock::GetMode(LPCSTR str)
{
	int i, a;
	CString work = str;
	CStringArray array;

	array.RemoveAll();
	for ( i = 0 ; i < work.GetLength() ; i = a + 1 ) {
		if ( (a = work.Find(';', i)) < 0 )
			a = work.GetLength();
		if ( (a - i) > 0 )
			array.Add(work.Mid(i, a - i));
	}

	for ( i = 0 ; i < array.GetSize() ; i++ ) {
		for ( a = 0 ; ComTab[a].name != NULL ; a++ ) {
			if ( array[i].CompareNoCase(ComTab[a].name) != 0 )
				continue;
			switch(ComTab[a].mode) {
			case 0: m_ComPort  = ComTab[a].value; break;
			case 1: m_BaudRate = ComTab[a].value; break;
			case 2: m_ByteSize = ComTab[a].value; break;
			case 3: m_Parity   = ComTab[a].value; break;
			case 4: m_StopBits = ComTab[a].value; break;
			case 5: m_FlowCtrl = ComTab[a].value; break;
			}
		}
	}

	m_ComName.Format("COM%d", m_ComPort);
}
void CComSock::SetMode(CString &str)
{
	int n, r;

	str = "";
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
			str += ";";
		}
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
int CComSock::AliveComPort()
{
	int n;
	int port = 0;
	COMMCONFIG conf;
	CString name;

	for ( n = 1 ; n <= 16 ; n++ ) {
		name.Format("COM%d", n);
		DWORD sz = sizeof(COMMCONFIG);
		if ( GetDefaultCommConfig(name, &conf, &sz) )
			port |= (1 << n);
	}
	return port;
}