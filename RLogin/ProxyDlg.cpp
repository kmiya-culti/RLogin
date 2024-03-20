// ProxyDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "ProxyDlg.h"


// CProxyDlg ダイアログ

IMPLEMENT_DYNAMIC(CProxyDlg, CDialogExt)

CProxyDlg::CProxyDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CProxyDlg::IDD, pParent)
{
	m_ServerName = _T("");
	m_PortName   = _T("");
	m_UserName   = _T("");
	m_PassWord   = _T("");
	m_ProxyType  = 0;
	m_ProxyMode  = 0;
	m_SSLType    = 0;
	m_SSLMode    = 0;
	m_SSL_Keep   = FALSE;
	m_UsePassDlg = FALSE;
	m_CmdFlag    = FALSE;
	m_ProxyCmd   = _T("");
}

CProxyDlg::~CProxyDlg()
{
}

void CProxyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SERVERNAME, m_ServerName);
	DDX_CBStringExact(pDX, IDC_SOCKNO, m_PortName);
	DDX_Text(pDX, IDC_USERNAME, m_UserName);
	DDX_Text(pDX, IDC_PASSWORD, m_PassWord);
	DDX_Radio(pDX, IDC_RADIO1, m_ProxyType);
	DDX_Radio(pDX, IDC_SSL_RADIO1, m_SSLType);
	DDX_Check(pDX, IDC_SSL_KEEP, m_SSL_Keep);
	DDX_Check(pDX, IDC_CHECK1, m_UsePassDlg);
	DDX_Check(pDX, IDC_CHECK2, m_CmdFlag);
	DDX_Text(pDX, IDC_PROXYCMD, m_ProxyCmd);
}


BEGIN_MESSAGE_MAP(CProxyDlg, CDialogExt)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO7, OnProtoType)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_SSL_RADIO1, IDC_SSL_RADIO6, OnProtoType)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECK2, IDC_CHECK2, OnProtoType)
END_MESSAGE_MAP()


// CProxyDlg メッセージ ハンドラ

void CProxyDlg::OnProtoType(UINT nID)
{
	int n;
	CWnd *pWnd;
	static const struct {
		int		nId;
		BOOL	mode[9];
	} ItemTab[] = {			/*	none	http	http(b)	http/2	http/3	socks4	socks5	ssl		cmd		*/
		{ IDC_SERVERNAME,	{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_SOCKNO,		{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_USERNAME,		{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_PASSWORD,		{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_CHECK1,		{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_CHECK2,		{	TRUE,	FALSE,	FALSE,	FALSE,	FALSE,	FALSE,	FALSE,	FALSE,	TRUE	} },
		{ 0 }
	};

	UpdateData(TRUE);

	if ( nID == IDC_CHECK2 )
		nID = IDC_RADIO1 + 8;

	if ( nID >= IDC_SSL_RADIO1 && nID <= IDC_SSL_RADIO6 )
		nID = IDC_RADIO1 + m_ProxyType;

	if ( m_ProxyType == 0 && m_SSLType != 0 )
		nID = IDC_RADIO1 + 7;

	for ( n = 0 ; ItemTab[n].nId != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].nId)) != NULL )
			pWnd->EnableWindow(ItemTab[n].mode[nID - IDC_RADIO1]);
	}

	if ( (pWnd = GetDlgItem(IDC_SSL_KEEP)) != NULL )
		pWnd->EnableWindow(m_ProxyType != 0 && m_SSLType == 1 ? TRUE : FALSE);

	for ( n = 0 ; n <= 7 ; n++ ) {
		if ( (pWnd = GetDlgItem(IDC_RADIO1 + n)) != NULL )
			pWnd->EnableWindow(m_CmdFlag ? FALSE : TRUE);
	}

	for ( n = 0 ; n <= 2 ; n++ ) {
		if ( (pWnd = GetDlgItem(IDC_SSL_RADIO1 + n)) != NULL )
			pWnd->EnableWindow(m_CmdFlag ? FALSE : TRUE);
	}

	if ( (pWnd = GetDlgItem(IDC_PROXYCMD)) != NULL )
		pWnd->EnableWindow(m_CmdFlag ? TRUE : FALSE);

}
 
BOOL CProxyDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	// m_ProxyMode = CServerEntry::m_ProxyMode & 7
	// m_ProxyMode 0 == None			m_ProxyType = 0
	// m_ProxyMode 1 == HTTP			m_ProxyType = 1
	// m_ProxyMode 2 == SOCKS4			m_ProxyType = 5
	// m_ProxyMode 3 == SOCKS5			m_ProxyType = 6
	// m_ProxyMode 4 == HTTP(Basic)		m_ProxyType = 2
	// m_ProxyMode 5 == ProxyCommand	m_CmdFlag = TRUE
	// m_ProxyMode 6 == HTTP/2			m_ProxyType = 3
	// m_ProxyMode 7 == HTTP/3			m_ProxyType = 4

	// m_SSLMode = CServerEntry::m_ProxyMode >> 3
	// m_SSLMode 0   == None			m_SSLType = 0
	// m_SSLMode 1-6 == SSL/TLS			m_SSLType = 1
	// m_SSLMode 7   == QUIC			m_SSLType = 2

	m_CmdFlag = FALSE;

	switch(m_ProxyMode) {
	case 0:		// None
		m_ProxyType = 0;
		break;
	case 1:		// HTTP
		m_ProxyType = 1;
		break;
	case 2:		// SOCKS4
		m_ProxyType = 5;
		break;
	case 3:		// SOCKS5
		m_ProxyType = 6;
		break;
	case 4:		// HTTP(Basic)
		m_ProxyType = 2;
		break;
	case 5:		// ProxyCommand
		m_ProxyType = 0;
		m_SSLMode   = 0;
		m_CmdFlag   = TRUE;
		break;
	case 6:		// HTTP/2
		m_ProxyType = 3;
		break;
	case 7:		// HTTP/3
		m_ProxyType = 4;
		break;
	}

	switch(m_SSLMode) {
	case 0:		// Nome
		m_SSLType = 0;
		break;
	case 1:		// SSL/TLS
	case 2:	case 3: case 4: case 5:	case 6:
		m_SSLType = 1;
		break;
	case 7:		// QUIC
		m_SSLType = 2;
		break;
	}

	UpdateData(FALSE);

	OnProtoType(IDC_RADIO1 + m_ProxyType);

	SubclassComboBox(IDC_SOCKNO);

	SetSaveProfile(_T("ProxyDlg"));
	AddHelpButton(_T("#PROXYSET"));

	return TRUE;
}

void CProxyDlg::OnOK()
{
	UpdateData(TRUE);

	if ( m_CmdFlag ) {
		m_ProxyMode = 5;
		m_SSLMode = 0;
	} else {
		switch(m_ProxyType) {
		case 0:		// None
			m_ProxyMode = 0;
			break;
		case 1:		// HTTP
			m_ProxyMode = 1;
			break;
		case 2:		// HTTP(Basic)
			m_ProxyMode = 4;
			break;
		case 3:		// HTTP/2
			m_ProxyMode = 6;
			break;
		case 4:		// HTTP/3
			m_ProxyMode = 7;
			break;
		case 5:		// SOCKS4
			m_ProxyMode = 2;
			break;
		case 6:		// SOCKS5
			m_ProxyMode = 3;
			break;
		}

		switch(m_SSLType) {
		case 0:		// Nome
			m_SSLMode = 0;
			break;
		case 1:		// SSL/TLS
			m_SSLMode = 1;
			break;
		case 2:		// QUIC
			m_SSLMode = 7;
			break;
		}
	}

	CDialogExt::OnOK();
}
