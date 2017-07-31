// ChModDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "ChModDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChModDlg ダイアログ


CChModDlg::CChModDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChModDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChModDlg)
	//}}AFX_DATA_INIT
	for ( int n = 0 ; n < 9 ; n++ )
		m_Mode[n] = FALSE;
	m_Attr = 0;
}

void CChModDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChModDlg)
	//}}AFX_DATA_MAP
	for ( int n = 0 ; n < 9 ; n++ )
		DDX_Check(pDX, IDC_CHMOD1 + n, m_Mode[n]);
}


BEGIN_MESSAGE_MAP(CChModDlg, CDialog)
	//{{AFX_MSG_MAP(CChModDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChModDlg メッセージ ハンドラ

BOOL CChModDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int n, b = 0400;

	for ( n = 0 ; n < 9 ; n++, b >>= 1 )
		m_Mode[n] = ((m_Attr & b) ? TRUE : FALSE);

	UpdateData(FALSE);

	return TRUE;
}

void CChModDlg::OnOK() 
{
	int n, b = 0400;

	UpdateData(TRUE);

	for ( m_Attr = n = 0 ; n < 9 ; n++, b >>= 1 ) {
		if ( m_Mode[n] )
			m_Attr |= b;
	}
	
	CDialog::OnOK();
}
