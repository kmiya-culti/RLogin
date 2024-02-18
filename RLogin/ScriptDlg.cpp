// ScriptDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "Script.h"
#include "ScriptDlg.h"

// CScriptDlg ダイアログ

IMPLEMENT_DYNAMIC(CScriptDlg, CDialogExt)

CScriptDlg::CScriptDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CScriptDlg::IDD, pParent)
{
	m_FileName.Empty();
	m_pScript = NULL;
	m_pSrc = NULL;
	m_Max = m_SPos = m_EPos = 0;
}

CScriptDlg::~CScriptDlg()
{
}

void CScriptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FILENAME, m_FileName);
	DDX_Control(pDX, IDC_TEXT, m_Text);
	DDX_Control(pDX, IDC_CODESTACK, m_CodeStack);
}

BEGIN_MESSAGE_MAP(CScriptDlg, CDialogExt)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CONTINUE, &CScriptDlg::OnBnClickedContinue)
END_MESSAGE_MAP()

static const INITDLGTAB ItemTab[] = {
	{ IDC_FILENAME,		ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_TEXT,			ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_BTM },
	{ IDC_CODESTACK,	ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_CONTINUE,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
    { IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },

	{ 0,	0 },
};

// CScriptDlg メッセージ ハンドラ

BOOL CScriptDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	CodeStack *cp;
	CWnd *pWnd;

	if ( m_pSrc != NULL ) {
		m_Text.SetLimitText(m_Max + 2);
		m_Text.SetWindowText(m_pSrc);
		UpdatePos(m_SPos, m_EPos);
	}

	if ( m_pScript == NULL ) {
		if ( (pWnd = GetDlgItem(IDC_CONTINUE)) != NULL )
			pWnd->EnableWindow(FALSE);
		if ( (pWnd = GetDlgItem(IDCANCEL)) != NULL )
			pWnd->EnableWindow(FALSE);
		m_CodeStack.EnableWindow(FALSE);
	} else {
		if ( (pWnd = GetDlgItem(IDOK)) != NULL )
			pWnd->SetWindowText(CStringLoad(IDS_SCRIPTDLGSTEP));
		for ( cp = m_pScript->m_CodeStack ; cp != NULL ; cp = cp->next )
			m_CodeStack.AddString(cp->name);
		m_CodeStack.SetCurSel(0);
	}

	SetSaveProfile(_T("ScriptDlg"));

	return TRUE;
}
void CScriptDlg::UpdatePos(int SPos, int EPos)
{
	int n;
	m_Text.SetSel(SPos, EPos, FALSE);
	if ( (n = m_Text.LineFromChar(SPos) - 8) < 0 )
		n = 0;
	m_Text.LineScroll(n - m_Text.GetFirstVisibleLine());
}

void CScriptDlg::PostNcDestroy()
{
	if ( m_pScript != NULL )
		m_pScript->m_pSrcDlg = NULL;
	delete this;
}
void CScriptDlg::OnClose()
{
	CDialogExt::OnClose();
	DestroyWindow();
}
void CScriptDlg::OnOK()
{
	if ( m_pScript != NULL ) {
		try {
			m_pScript->Exec();
		} catch(LPCTSTR pMsg) {
			CString tmp;
			tmp.Format(_T("Script Error '%s'"), pMsg);
			::AfxMessageBox(tmp, MB_ICONERROR);
		} catch(...) {
			::AfxMessageBox(_T("Script Unkown Error"), MB_ICONERROR);
		}
		return;
	}

	CDialogExt::OnOK();
	DestroyWindow();
}
void CScriptDlg::OnCancel()
{
	if ( m_pScript != NULL )
		m_pScript->Stop();

	CDialogExt::OnCancel();
	DestroyWindow();
}
void CScriptDlg::OnBnClickedContinue()
{
	CDialogExt::OnCancel();
	DestroyWindow();
}
