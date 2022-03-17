// HisPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "HisPage.h"

/////////////////////////////////////////////////////////////////////////////
// CHisPage ダイアログ

IMPLEMENT_DYNAMIC(CHisPage, CTreePage)

#define	CHECKOPTMAX		3
#define	IDC_CHECKFAST	IDC_TERMCHECK1
static const int CheckOptTab[] = { TO_RLHISDATE, TO_RLHISFILE, TO_RLLOGTIME };

CHisPage::CHisPage() : CTreePage(CHisPage::IDD)
{
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_HisMax = _T("");
	m_LogMode = 0;
	m_LogCode = 0;
	m_UrlOpt = _T("#HISOPT");
}
CHisPage::~CHisPage()
{
}

void CHisPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);
	DDX_Text(pDX, IDC_HISMAX, m_HisMax);
	DDX_CBIndex(pDX, IDC_COMBO1, m_LogMode);
	DDX_CBIndex(pDX, IDC_COMBO2, m_LogCode);
	DDX_Text(pDX, IDC_HISFILE_PATH, m_HisFile);
	DDX_Text(pDX, IDC_AUTOLOG_PATH, m_LogFile);
	DDX_Text(pDX, IDC_COMBO3, m_TimeFormat);
	DDX_Text(pDX, IDC_TRACEFILE_PATH, m_TraceFile);
	DDX_Text(pDX, IDC_TRACE_MAX, m_TraceMax);
}

BEGIN_MESSAGE_MAP(CHisPage, CTreePage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_EN_CHANGE(IDC_HISMAX, OnUpdateEdit)
	ON_EN_CHANGE(IDC_HISFILE_PATH, OnUpdateEdit)
	ON_EN_CHANGE(IDC_AUTOLOG_PATH, OnUpdateEdit)
	ON_EN_CHANGE(IDC_TRACEFILE_PATH, OnUpdateEdit)
	ON_EN_CHANGE(IDC_TRACE_MAX, OnUpdateEdit)
	ON_BN_CLICKED(IDC_HISFILE_SEL, OnHisfileSel)
	ON_BN_CLICKED(IDC_AUTOLOG_SEL, OnAutoLogSel)
	ON_BN_CLICKED(IDC_TRACEFILE_SEL, OnTraceLogSel)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnUpdateEdit)
	ON_CBN_EDITUPDATE(IDC_COMBO3, OnUpdateEdit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHisPage メッセージ ハンドラ

void CHisPage::DoInit()
{
	int n;

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_HisMax.Format(_T("%d"),    m_pSheet->m_pTextRam->m_DefHisMax);
	m_HisFile = m_pSheet->m_pTextRam->m_HisFile;

	m_LogFile = m_pSheet->m_pTextRam->m_LogFile;
	m_LogMode = m_pSheet->m_pTextRam->m_LogMode;
	m_LogCode = m_pSheet->m_pTextRam->IsOptValue(TO_RLLOGCODE, 2);

	m_TimeFormat = m_pSheet->m_pTextRam->m_TimeFormat;

	m_TraceMax.Format(_T("%d"),    m_pSheet->m_pTextRam->m_TraceMaxCount);
	m_TraceFile = m_pSheet->m_pTextRam->m_TraceLogFile;

	UpdateData(FALSE);
}
BOOL CHisPage::OnInitDialog()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	DoInit();

	SubclassComboBox(IDC_COMBO3);

	return TRUE;
}
BOOL CHisPage::OnApply() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	int n;

	UpdateData(TRUE);

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	if ( m_pSheet->m_pTextRam->IsOptEnable(TO_RLHISFILE) && m_HisFile.IsEmpty() )
		m_HisFile.Format(_T("%s\\%s.rlh"), (LPCTSTR)((CRLoginApp *)AfxGetApp())->m_BaseDir, (LPCTSTR)m_pSheet->m_pEntry->m_EntryName);

	m_pSheet->m_pTextRam->m_DefHisMax   = _tstoi(m_HisMax);
	m_pSheet->m_pTextRam->m_HisFile     = m_HisFile;

	m_pSheet->m_pTextRam->m_LogFile     = m_LogFile;
	m_pSheet->m_pTextRam->m_LogMode     = m_LogMode;
	m_pSheet->m_pTextRam->SetOptValue(TO_RLLOGCODE, 2, m_LogCode);

	m_pSheet->m_pTextRam->m_TimeFormat = m_TimeFormat;

	m_pSheet->m_pTextRam->m_TraceMaxCount = _tstoi(m_TraceMax);
	m_pSheet->m_pTextRam->m_TraceLogFile  = m_TraceFile;

	return TRUE;
}
void CHisPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}
void CHisPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CHisPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CHisPage::OnHisfileSel() 
{
	CString file;

	UpdateData(TRUE);

	if ( m_HisFile.IsEmpty() )
		file.Format(_T("%s\\%s.rlh"), (LPCTSTR)((CRLoginApp *)AfxGetApp())->m_BaseDir, (LPCTSTR)m_pSheet->m_pEntry->m_EntryName);
	else
		file = m_HisFile;

	CRLoginDoc::EnvironPath(file);
	CFileDialog dlg(FALSE, _T("rlh"), file, 0, CStringLoad(IDS_FILEDLGHISTORY), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_HisFile = dlg.GetPathName();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CHisPage::OnAutoLogSel()
{
	CString file;

	UpdateData(TRUE);

	if ( m_LogFile.IsEmpty() )
		file.Format(_T("%s\\%s.txt"), (LPCTSTR)((CRLoginApp *)AfxGetApp())->m_BaseDir, (LPCTSTR)m_pSheet->m_pEntry->m_EntryName);
	else
		file = m_LogFile;

	CRLoginDoc::EnvironPath(file);
	CFileDialog dlg(FALSE, _T("txt"), file, 0, CStringLoad(IDS_FILEDLGLOGFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_LogFile = dlg.GetPathName();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CHisPage::OnTraceLogSel()
{
	CString file;

	UpdateData(TRUE);
	file = m_TraceFile;

	CRLoginDoc::EnvironPath(file);
	CFileDialog dlg(FALSE, _T("txt"), file, 0, CStringLoad(IDS_FILEDLGLOGFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_TraceFile = dlg.GetPathName();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
