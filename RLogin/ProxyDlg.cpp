// ProxyDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "ProxyDlg.h"


// CProxyDlg ダイアログ

IMPLEMENT_DYNAMIC(CProxyDlg, CDialog)

CProxyDlg::CProxyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProxyDlg::IDD, pParent)
	, m_ServerName(_T(""))
	, m_PortName(_T(""))
	, m_UserName(_T(""))
	, m_PassWord(_T(""))
	, m_ProxyMode(0)
{

}

CProxyDlg::~CProxyDlg()
{
}

void CProxyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SERVERNAME, m_ServerName);
	DDX_CBString(pDX, IDC_SOCKNO, m_PortName);
	DDX_Text(pDX, IDC_USERNAME, m_UserName);
	DDX_Text(pDX, IDC_PASSWORD, m_PassWord);
	DDX_Radio(pDX, IDC_RADIO1, m_ProxyMode);
}


BEGIN_MESSAGE_MAP(CProxyDlg, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO4, OnProtoType)
END_MESSAGE_MAP()


// CProxyDlg メッセージ ハンドラ

void CProxyDlg::OnProtoType(UINT nID)
{
	int n;
	CWnd *pWnd;
	static const struct {
		int		nId;
		BOOL	mode[4];
	} ItemTab[] = {			/*	none	http	socks4	socks5 */
		{ IDC_SERVERNAME,	{	FALSE,	TRUE,	TRUE,	TRUE	} },
		{ IDC_SOCKNO,		{	FALSE,	TRUE,	TRUE,	TRUE	} },
		{ IDC_USERNAME,		{	FALSE,	TRUE,	TRUE,	TRUE	} },
		{ IDC_PASSWORD,		{	FALSE,	TRUE,	FALSE,	TRUE	} },
		{ 0 }
	};

	for ( n = 0 ; ItemTab[n].nId != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].nId)) != NULL )
			pWnd->EnableWindow(ItemTab[n].mode[nID - IDC_RADIO1]);
	}
}

BOOL CProxyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	OnProtoType(IDC_RADIO1 + m_ProxyMode);

	return TRUE;
}
