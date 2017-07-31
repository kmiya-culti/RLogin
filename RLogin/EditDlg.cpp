// EditDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "EditDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CEditDlg ダイアログ

IMPLEMENT_DYNAMIC(CEditDlg, CDialogExt)

CEditDlg::CEditDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CEditDlg::IDD, pParent)
{
	m_Edit.Empty();
	m_Title.Empty();
	m_WinText.Empty();
	m_bPassword = FALSE;
}

void CEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_Edit);
	DDX_Text(pDX, IDC_TITLE, m_Title);
}

BEGIN_MESSAGE_MAP(CEditDlg, CDialogExt)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditDlg メッセージ ハンドラ

BOOL CEditDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	if ( !m_WinText.IsEmpty() )
		SetWindowText(m_WinText);

	if ( m_bPassword ) {
		CEdit *pWnd = (CEdit *)GetDlgItem(IDC_EDIT1);
		if ( pWnd != NULL ) {
			pWnd->ModifyStyle(0, ES_PASSWORD);
			pWnd->SetPasswordChar(_T('*'));
		}
	}

	return TRUE;
}
