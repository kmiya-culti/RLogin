// ScriptDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "ScriptDlg.h"


// CScriptDlg ダイアログ

IMPLEMENT_DYNAMIC(CScriptDlg, CDialog)

CScriptDlg::CScriptDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScriptDlg::IDD, pParent)
	, m_FileName(_T(""))
{
	m_pSrc = NULL;
	m_Max = m_SPos = m_EPos = 0;
}

CScriptDlg::~CScriptDlg()
{
}

void CScriptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FILENAME, m_FileName);
	DDX_Control(pDX, IDC_TEXT, m_Text);
}


BEGIN_MESSAGE_MAP(CScriptDlg, CDialog)
END_MESSAGE_MAP()


// CScriptDlg メッセージ ハンドラ

BOOL CScriptDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if ( m_pSrc != NULL ) {
		m_Text.SetLimitText(m_Max + 2);
		m_Text.SetWindowText((LPCSTR)m_pSrc);
		UpdatePos(m_SPos, m_EPos);
	}

	return TRUE;
}
void CScriptDlg::UpdatePos(int SPos, int EPos)
{
	m_Text.SetSel(SPos, EPos, FALSE);
	int n = m_Text.GetLineCount();
	if ( (n = m_Text.LineFromChar(SPos) - 8) < 0 )
		n = 0;
	m_Text.LineScroll(n);
}