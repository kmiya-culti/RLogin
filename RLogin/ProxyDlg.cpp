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
	DDX_Radio(pDX, IDC_SSL_RADIO1, m_SSLMode);
	DDX_Check(pDX, IDC_SSL_KEEP, m_SSL_Keep);
	DDX_Check(pDX, IDC_CHECK1, m_UsePassDlg);
	DDX_Check(pDX, IDC_CHECK2, m_CmdFlag);
	DDX_Text(pDX, IDC_PROXYCMD, m_ProxyCmd);
}


BEGIN_MESSAGE_MAP(CProxyDlg, CDialogExt)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO5, OnProtoType)
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
		BOOL	mode[7];
	} ItemTab[] = {			/*	none	http	http(b)	socks4	socks5	ssl		cmd		*/
		{ IDC_SERVERNAME,	{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_SOCKNO,		{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_USERNAME,		{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_PASSWORD,		{	FALSE,	TRUE,	TRUE,	FALSE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_CHECK1,		{	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE	} },
		{ IDC_CHECK2,		{	TRUE,	FALSE,	FALSE,	FALSE,	FALSE,	FALSE,	TRUE	} },
		{ 0 }
	};

	UpdateData(TRUE);

	if ( nID == IDC_CHECK2 )
		nID = IDC_RADIO1 + 6;

	if ( nID >= IDC_SSL_RADIO1 && nID <= IDC_SSL_RADIO6 )
		nID = IDC_RADIO1 + m_ProxyType;

	if ( m_ProxyType == 0 && m_SSLMode != 0 )
		nID = IDC_RADIO1 + 5;

	for ( n = 0 ; ItemTab[n].nId != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].nId)) != NULL )
			pWnd->EnableWindow(ItemTab[n].mode[nID - IDC_RADIO1]);
	}

	if ( (pWnd = GetDlgItem(IDC_SSL_KEEP)) != NULL )
		pWnd->EnableWindow(m_SSLMode != 0 ? TRUE : FALSE);

	for ( n = 0 ; n <= 5 ; n++ ) {
		if ( (pWnd = GetDlgItem(IDC_RADIO1 + n)) != NULL )
			pWnd->EnableWindow(m_CmdFlag ? FALSE : TRUE);
	}

	for ( n = 0 ; n <= 1 ; n++ ) {
		if ( (pWnd = GetDlgItem(IDC_SSL_RADIO1 + n)) != NULL )
			pWnd->EnableWindow(m_CmdFlag ? FALSE : TRUE);
	}

	if ( (pWnd = GetDlgItem(IDC_PROXYCMD)) != NULL )
		pWnd->EnableWindow(m_CmdFlag ? TRUE : FALSE);
}

BOOL CProxyDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	// 0=0, 1=1, 2=3, 3=4, 4=2
	if ( (m_ProxyType = m_ProxyMode) == 5 ) {
		m_CmdFlag = TRUE;
		m_ProxyType = 0;
		m_SSLMode = 0;
	} else if ( m_ProxyType == 4  )
		m_ProxyType = 2;
	else if ( m_ProxyType >= 2  )
		m_ProxyType += 1;

	if ( m_SSLMode != 0 )
		m_SSLMode = 1;

	UpdateData(FALSE);

	OnProtoType(IDC_RADIO1 + m_ProxyType);

	SubclassComboBox(IDC_SOCKNO);

	return TRUE;
}

void CProxyDlg::OnOK()
{
	UpdateData(TRUE);

	// 0=0, 1=1, 2=4, 3=2, 4=3
	if ( (m_ProxyMode = m_ProxyType) == 0 && m_SSLMode == 0 && m_CmdFlag )
		m_ProxyMode = 5;
	else if ( m_ProxyType == 2  )
		m_ProxyMode = 4;
	else if ( m_ProxyType > 2  )
		m_ProxyMode -= 1;

	UpdateData(FALSE);

	CDialogExt::OnOK();
}
