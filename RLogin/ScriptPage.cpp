// ScriptPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "ScriptPage.h"

// CScriptPage ダイアログ

IMPLEMENT_DYNAMIC(CScriptPage, CTreePage)

#define	CHECKOPTMAX		1
#define	IDC_CHECKFAST	IDC_TERMCHECK1
static const int CheckOptTab[] = { TO_RLSCRDEBUG };

CScriptPage::CScriptPage() : CTreePage(CScriptPage::IDD)
	, m_ScriptStr(_T(""))
{
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_ScriptFile.Empty();
}

CScriptPage::~CScriptPage()
{
}

void CScriptPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);

	DDX_Text(pDX, IDC_SCRIPT_PATH, m_ScriptFile);
	DDX_Text(pDX, IDC_SCRIPT_STR, m_ScriptStr);
}

BEGIN_MESSAGE_MAP(CScriptPage, CTreePage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_EN_CHANGE(IDC_SCRIPT_PATH, OnUpdateEdit)
	ON_EN_CHANGE(IDC_SCRIPT_STR, OnUpdateEdit)
	ON_BN_CLICKED(IDC_SCRIPT_SEL, &CScriptPage::OnBnClickedScriptSel)
END_MESSAGE_MAP()


// CScriptPage メッセージ ハンドラー

void CScriptPage::DoInit()
{
	int n;

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ScriptFile = m_pSheet->m_pEntry->m_ScriptFile;
	m_ScriptStr  = m_pSheet->m_pEntry->m_ScriptStr;

	UpdateData(FALSE);
}
BOOL CScriptPage::OnInitDialog()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();
	DoInit();

	return TRUE;
}
BOOL CScriptPage::OnApply()
{
	int n;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	m_pSheet->m_pEntry->m_ScriptFile = m_ScriptFile;
	m_pSheet->m_pEntry->m_ScriptStr  = m_ScriptStr;

	return TRUE;
}
void CScriptPage::OnReset()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}

void CScriptPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CScriptPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CScriptPage::OnBnClickedScriptSel()
{
	CString file;

	UpdateData(TRUE);

	if ( m_ScriptFile.IsEmpty() )
		file.Format(_T("%s\\%s.txt"), ((CRLoginApp *)AfxGetApp())->m_BaseDir, m_pSheet->m_pEntry->m_EntryName);
	else
		file = m_ScriptFile;

	CRLoginDoc::EnvironPath(file);
	CFileDialog dlg(FALSE, _T("txt"), file, 0, CStringLoad(IDS_FILEDLGSCRIPT), this);

	if ( dlg.DoModal() != IDOK )
		return;

	m_ScriptFile = dlg.GetPathName();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
