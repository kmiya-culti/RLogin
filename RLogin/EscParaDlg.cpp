// EscParaDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "EscParaDlg.h"

// CEscParaDlg ダイアログ

IMPLEMENT_DYNAMIC(CEscParaDlg, CDialogExt)

CEscParaDlg::CEscParaDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CEscParaDlg::IDD, pParent)
{
	m_Code = 0;
	m_Name = _T("NOP");
}

CEscParaDlg::~CEscParaDlg()
{
}

void CEscParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CODE, m_CodeCombo);
	DDX_Control(pDX, IDC_NAME, m_NameCombo);
}


BEGIN_MESSAGE_MAP(CEscParaDlg, CDialogExt)
END_MESSAGE_MAP()


// CEscParaDlg メッセージ ハンドラ

BOOL CEscParaDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	int n;
	CString tmp;

	for ( n = 0 ; n < 95 ; n++ ) {
		tmp.Format(_T("%c"), n + _T(' '));
		m_CodeCombo.AddString(tmp);
	}
	m_CodeCombo.SetCurSel(m_Code);

	m_pTextRam->SetEscNameCombo(&m_NameCombo);
	m_NameCombo.SelectString(-1, m_Name);

	return TRUE;
}

void CEscParaDlg::OnOK()
{
	m_Code = m_CodeCombo.GetCurSel();
	m_NameCombo.GetWindowText(m_Name);

	CDialogExt::OnOK();
}
