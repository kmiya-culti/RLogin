// TermPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "TermPage.h"
#include "EscDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTermPage プロパティ ページ

IMPLEMENT_DYNCREATE(CTermPage, CPropertyPage)

#define	CHECKOPTMAX		9
#define	IDC_CHECKFAST	IDC_TERMCHECK1
static const int CheckOptTab[] = { TO_DECAWM, TO_XTMRVW, TO_DECCKM, TO_RLBOLD, TO_RLGNDW, TO_RLGCWA, TO_RLUNIAWH, TO_RLHISDATE, TO_RLKANAUTO };

CTermPage::CTermPage() : CPropertyPage(CTermPage::IDD)
{
	//{{AFX_DATA_INIT(CTermPage)
	//}}AFX_DATA_INIT
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_pSheet = NULL;
	m_LogMode = 0;
	m_LogCode = 0;
}

CTermPage::~CTermPage()
{
}

void CTermPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTermPage)
	//}}AFX_DATA_MAP
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);
	DDX_CBIndex(pDX, IDC_COMBO1, m_LogMode);
	DDX_CBIndex(pDX, IDC_COMBO2, m_LogCode);
}

BEGIN_MESSAGE_MAP(CTermPage, CPropertyPage)
	//{{AFX_MSG_MAP(CTermPage)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_BN_CLICKED(IDC_AUTOLOG_SEL, OnBnClickedAutologSel)
	ON_BN_CLICKED(IDC_ESCEDIT, &CTermPage::OnBnClickedEscedit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTermPage メッセージ ハンドラ

BOOL CTermPage::OnInitDialog() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_LogFile = m_pSheet->m_pTextRam->m_LogFile;
	m_LogMode = m_pSheet->m_pTextRam->IsOptValue(TO_RLLOGMODE, 2);
	m_LogCode = m_pSheet->m_pTextRam->IsOptValue(TO_RLLOGCODE, 2);
	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	UpdateData(FALSE);
	return TRUE;
}
BOOL CTermPage::OnApply() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	UpdateData(TRUE);
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ ) {
		if ( m_Check[n] )
			m_pSheet->m_pTextRam->EnableOption(CheckOptTab[n]);
		else
			m_pSheet->m_pTextRam->DisableOption(CheckOptTab[n]);
	}

	m_pSheet->m_pTextRam->m_LogFile     = m_LogFile;
	m_pSheet->m_pTextRam->SetOptValue(TO_RLLOGMODE, 2, m_LogMode);
	m_pSheet->m_pTextRam->SetOptValue(TO_RLLOGCODE, 2, m_LogCode);
	m_pSheet->m_pTextRam->m_ProcTab = m_ProcTab;

	return TRUE;
}
void CTermPage::OnReset() 
{
	if ( m_hWnd == NULL )
		return;

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_LogFile = m_pSheet->m_pTextRam->m_LogFile;
	m_LogMode = m_pSheet->m_pTextRam->IsOptValue(TO_RLLOGMODE, 2);
	m_LogCode = m_pSheet->m_pTextRam->IsOptValue(TO_RLLOGCODE, 2);
	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	UpdateData(FALSE);
	SetModified(FALSE);
}
void CTermPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CTermPage::OnBnClickedAutologSel()
{
	if ( m_LogFile.IsEmpty() )
		m_LogFile.Format("%s\\%s.txt", ((CRLoginApp *)AfxGetApp())->m_BaseDir, m_pSheet->m_pEntry->m_EntryName);

	CFileDialog dlg(FALSE, "txt", m_LogFile, 0,
		"ログ ファイル (*.txt)|*.txt|All Files (*.*)|*.*||", this);

	if ( dlg.DoModal() != IDOK )
		return;

	m_LogFile = dlg.GetPathName();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CTermPage::OnBnClickedEscedit()
{
	CEscDlg dlg;

	dlg.m_pTextRam = m_pSheet->m_pTextRam;
	dlg.m_pProcTab = &m_ProcTab;

	if ( dlg.DoModal() != IDOK )
		return;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
