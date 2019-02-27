// CsiParaDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "CsiParaDlg.h"

// CCsiParaDlg ダイアログ

IMPLEMENT_DYNAMIC(CCsiParaDlg, CDialogExt)

CCsiParaDlg::CCsiParaDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CCsiParaDlg::IDD, pParent)
{
	m_Type = PROCTYPE_CSI;
	m_Code = 0;
	m_Name = _T("NOP");
}

CCsiParaDlg::~CCsiParaDlg()
{
}

void CCsiParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CEXT1, m_Ext1Combo);
	DDX_Control(pDX, IDC_CEXT2, m_Ext2Combo);
	DDX_Control(pDX, IDC_CODE, m_CodeCombo);
	DDX_Control(pDX, IDC_NAME, m_NameCombo);
}


BEGIN_MESSAGE_MAP(CCsiParaDlg, CDialogExt)
END_MESSAGE_MAP()


// CCsiParaDlg メッセージ ハンドラ

BOOL CCsiParaDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	int n;
	CString tmp;

	m_Ext1Combo.AddString(_T(" "));
	for ( n = 0 ; n < 4 ; n++ ) {
		tmp.Format(_T("%c"), n + _T('<'));
		m_Ext1Combo.AddString(tmp);
	}
	m_Ext1Combo.SetCurSel(m_Code / (63 * 17));

	m_Ext2Combo.AddString(_T(" "));
	m_Ext2Combo.AddString(_T("SP"));
	for ( n = 1 ; n < 16 ; n++ ) {
		tmp.Format(_T("%c"), n + _T(' '));
		m_Ext2Combo.AddString(tmp);
	}
	m_Ext2Combo.SetCurSel((m_Code / 63) % 17);

	if ( m_Type == PROCTYPE_ESC ) {
		for ( n = 0 ; n < 95 ; n++ ) {
			tmp.Format(_T("%c"), n + _T(' '));
			m_CodeCombo.AddString(tmp);
		}
		m_CodeCombo.SetCurSel(m_Code);
	} else {
		for ( n = 0 ; n < 63 ; n++ ) {
			tmp.Format(_T("%c"), n + _T('@'));
			m_CodeCombo.AddString(tmp);
		}
		m_CodeCombo.SetCurSel(m_Code % 63);
	}

	if ( m_Type == PROCTYPE_ESC ) {
		SetWindowText(_T("ESC Sequence"));
		m_Ext1Combo.EnableWindow(FALSE);
		m_Ext2Combo.EnableWindow(FALSE);
		m_pTextRam->SetEscNameCombo(&m_NameCombo);
	} else if ( m_Type == PROCTYPE_CSI ) {
		SetWindowText(_T("CSI Sequence"));
		m_pTextRam->SetCsiNameCombo(&m_NameCombo);
	} else if ( m_Type == PROCTYPE_DCS ) {
		SetWindowText(_T("DCS Sequence"));
		m_pTextRam->SetDcsNameCombo(&m_NameCombo);
	}

	m_NameCombo.SelectString(-1, m_Name);

	return TRUE;
}

void CCsiParaDlg::OnOK()
{
	m_Code = m_CodeCombo.GetCurSel();
	m_Code += (m_Ext2Combo.GetCurSel() * 63);
	m_Code += (m_Ext1Combo.GetCurSel() * 63 * 17);

	m_NameCombo.GetWindowText(m_Name);

	CDialogExt::OnOK();
}
