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

#define	CHECKOPTMAX		6
#define	IDC_CHECKFAST	IDC_TERMCHECK1
static const int CheckOptTab[] = { TO_RLBOLD, TO_RLGNDW, TO_RLGCWA, TO_RLUNIAWH, TO_RLHISDATE, TO_RLKANAUTO };

static const LV_COLUMN InitListTab[2] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  80,	"Mode",			0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  200,	"Description",	0, 0 },
	};

static const struct _OptListTab {
	int		num;
	LPCSTR	name;
} OptListTab[] = {
	// ANSI Screen Option	0-99(200-299)
	{	TO_ANSIIRM,		"Insertion replacement mode"		},
	{	TO_ANSIERM,		"Erasure mode"						},
	{	TO_ANSITSM,		"Tabulation stop mode"				},
	{	TO_ANSILNM,		"Line feed/New line mode"			},

	// DEC Terminal Option	0-199
	{	TO_DECCKM,		"Cursor key mode"					},
	{	TO_DECANM,		"ANSI/VT52 mode"					},
	{	TO_DECCOLM,		"132/80 Column mode"				},
	{	TO_DECSCLM,		"Scrolling mode"					},
	{	TO_DECSCNM,		"Black/White Screen mode"			},
	{	TO_DECOM,		"Origin mode"						},
	{	TO_DECAWM,		"Autowrap mode"						},
	{	TO_XTMOSREP,	"X10 mouse reporting"				},
	{	TO_DECTCEM,		"Text Cursor enable mode"			},
//	{	TO_DECTEK,		"Graphics (Tek)"					},
	{	TO_XTMCSC,		"Column switch control"				},
	{	TO_XTMCUS,		"XTerm tab wrap bugfix"				},
	{	TO_XTMRVW,		"Reverse-wraparound mode"			},
	{	TO_XTMABUF,		"Alternate Screen mode"				},
	{	TO_DECECM,		"SGR space color disable"			},

	// XTerm Option			1000-1099(300-399)
	{	TO_XTNOMTRK,	"X11 normal mouse tracking"			},
	{	TO_XTHILTRK,	"X11 hilite mouse tracking"			},
	{	TO_XTBEVTRK,	"X11 button-event mouse tracking"	},
	{	TO_XTAEVTRK,	"X11 any-event mouse tracking"		},
	{	TO_XTALTSCR,	"Alternate/Normal screen buffer"	},
	{	TO_XTSRCUR,		"Save/Restore cursor"				},
	{	TO_XTALTCLR,	"Alternate screen with clearing"	},

	{	0,				NULL	}
};

CTermPage::CTermPage() : CPropertyPage(CTermPage::IDD)
{
	//{{AFX_DATA_INIT(CTermPage)
	//}}AFX_DATA_INIT
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_pSheet = NULL;
	m_LogMode = 0;
	m_LogCode = 0;
	m_TtlMode = 0;
	m_TtlRep = m_TtlCng = FALSE;
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
	DDX_Control(pDX, IDC_ESCLIST, m_List);
	DDX_CBIndex(pDX, IDC_COMBO1, m_LogMode);
	DDX_CBIndex(pDX, IDC_COMBO2, m_LogCode);
	DDX_CBIndex(pDX, IDC_COMBO3, m_TtlMode);
	DDX_Check(pDX, IDC_TERMCHECK7, m_TtlRep);
	DDX_Check(pDX, IDC_TERMCHECK8, m_TtlCng);
}

BEGIN_MESSAGE_MAP(CTermPage, CPropertyPage)
	//{{AFX_MSG_MAP(CTermPage)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_BN_CLICKED(IDC_AUTOLOG_SEL, OnBnClickedAutologSel)
	ON_BN_CLICKED(IDC_ESCEDIT, &CTermPage::OnBnClickedEscedit)
	ON_NOTIFY(NM_CLICK, IDC_ESCLIST, &CTermPage::OnNMClickEsclist)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_TERMCHECK7, IDC_TERMCHECK9, OnUpdateCheck)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CTermPage::OnCbnSelchangeCombo)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CTermPage::OnCbnSelchangeCombo)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CTermPage::OnCbnSelchangeCombo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTermPage メッセージ ハンドラ

BOOL CTermPage::OnInitDialog() 
{
	int n;
	CString str;

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();
	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_LogFile = m_pSheet->m_pTextRam->m_LogFile;
	m_LogMode = m_pSheet->m_pTextRam->IsOptValue(TO_RLLOGMODE, 2);
	m_LogCode = m_pSheet->m_pTextRam->IsOptValue(TO_RLLOGCODE, 2);
	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	m_TtlMode = m_pSheet->m_pTextRam->m_TitleMode & 7;
	m_TtlRep  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_REPORT) ? TRUE : FALSE;
	m_TtlCng  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_CHENG)  ? TRUE : FALSE;

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn("TermPageOpt", InitListTab, 2);

	m_List.DeleteAllItems();
	for ( n = 0 ; OptListTab[n].name != NULL ; n++ ) {
		if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
			str.Format("?%d", OptListTab[n].num);
		} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
			str.Format("%d", OptListTab[n].num - 200);
		} else if ( OptListTab[n].num < 400 ) {		// XTerm Option			1000-1099(300-399)
			str.Format("?%d", OptListTab[n].num + 700);
		} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		400-511
			str.Format("?%d", OptListTab[n].num + 1600);
		}
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_List.SetItemText(n, 1, OptListTab[n].name);
		m_List.SetLVCheck(n,  m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) ? TRUE : FALSE);
	}

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

	m_pSheet->m_pTextRam->m_TitleMode = m_TtlMode;
	if ( m_TtlRep )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_REPORT;
	if ( m_TtlCng )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_CHENG;

	for ( int n = 0 ; OptListTab[n].name != NULL ; n++ ) {
		if ( m_List.GetLVCheck(n) ) {
			if ( m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) )
				continue;
			if ( m_pSheet->m_pTextRam->m_VRam == NULL || m_pSheet->m_pTextRam->m_pDocument == NULL ) {
				m_pSheet->m_pTextRam->EnableOption(OptListTab[n].num);
				continue;
			}
			m_pSheet->m_pTextRam->m_AnsiPara.RemoveAll();
			if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
				m_pSheet->m_pTextRam->EnableOption(OptListTab[n].num);
			} else if ( OptListTab[n].num < 400 ) {		// XTerm Option			1000-1099(300-399)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 700);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		400-511
				m_pSheet->m_pTextRam->EnableOption(OptListTab[n].num);
			}
		} else {
			if ( !m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) )
				continue;
			if ( m_pSheet->m_pTextRam->m_VRam == NULL || m_pSheet->m_pTextRam->m_pDocument == NULL ) {
				m_pSheet->m_pTextRam->DisableOption(OptListTab[n].num);
				continue;
			}
			m_pSheet->m_pTextRam->m_AnsiPara.RemoveAll();
			if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
				m_pSheet->m_pTextRam->DisableOption(OptListTab[n].num);
			} else if ( OptListTab[n].num < 400 ) {		// XTerm Option			1000-1099(300-399)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 700);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		400-511
				m_pSheet->m_pTextRam->DisableOption(OptListTab[n].num);
			}
		}
	}

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

	m_TtlMode = m_pSheet->m_pTextRam->m_TitleMode & 7;
	m_TtlRep  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_REPORT) ? TRUE : FALSE;
	m_TtlCng  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_CHENG)  ? TRUE : FALSE;

	for ( int n = 0 ; OptListTab[n].name != NULL ; n++ )
		m_List.SetLVCheck(n,  m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) ? TRUE : FALSE);

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

void CTermPage::OnNMClickEsclist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if ( pNMItemActivate->iSubItem == 0 ) {
		SetModified(TRUE);
		m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
	}

	*pResult = 0;
}

void CTermPage::OnCbnSelchangeCombo()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
