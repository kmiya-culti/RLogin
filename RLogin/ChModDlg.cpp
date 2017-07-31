// ChModDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "ChModDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CChModDlg ダイアログ

IMPLEMENT_DYNAMIC(CChModDlg, CDialog)

CChModDlg::CChModDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChModDlg::IDD, pParent)
{
	for ( int n = 0 ; n < 9 ; n++ )
		m_Mode[n] = FALSE;
	m_Attr = 0;
}

void CChModDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	for ( int n = 0 ; n < 9 ; n++ )
		DDX_Check(pDX, IDC_CHMOD1 + n, m_Mode[n]);
}

BEGIN_MESSAGE_MAP(CChModDlg, CDialog)
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
