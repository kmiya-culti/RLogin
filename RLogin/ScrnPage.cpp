// ScrnPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "ScrnPage.h"


// CScrnPage ダイアログ

IMPLEMENT_DYNAMIC(CScrnPage, CPropertyPage)

#define	CHECKOPTMAX		1
#define	IDC_CHECKFAST	IDC_TERMCHECK1
static const int CheckOptTab[] = { TO_RLNORESZ };

CScrnPage::CScrnPage() : CPropertyPage(CScrnPage::IDD)
{
	m_ScrnFont = -1;
	m_FontSize = _T("");
	m_ColsMax[0] = _T("");
	m_ColsMax[1] = _T("");
	m_VisualBell = 0;
	m_RecvCrLf = 0;
	m_SendCrLf = 0;
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_pSheet = NULL;
	m_MouseMode[0] = 0;
	m_MouseMode[1] = 1;
	m_MouseMode[2] = 0;
	m_MouseMode[3] = 2;
	m_FontHw = 10;
}

CScrnPage::~CScrnPage()
{
}

void CScrnPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTermPage)
	DDX_Radio(pDX, IDC_SCRNSIZE1, m_ScrnFont);
	DDX_CBString(pDX, IDC_SCSZFONT, m_FontSize);
	DDX_Text(pDX, IDC_SCSZCOLS, m_ColsMax[0]);
	DDX_Text(pDX, IDC_SCSZCOLS2, m_ColsMax[1]);
	DDX_CBIndex(pDX, IDC_VISUALBELL, m_VisualBell);
	DDX_CBIndex(pDX, IDC_RECVCRLF, m_RecvCrLf);
	DDX_CBIndex(pDX, IDC_SENDCRLF, m_SendCrLf);
	//}}AFX_DATA_MAP
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);
	DDX_CBIndex(pDX, IDC_MOUSE_BTN1, m_MouseMode[0]);
	DDX_CBIndex(pDX, IDC_MOUSE_BTN2, m_MouseMode[1]);
	DDX_CBIndex(pDX, IDC_MOUSE_KEY1, m_MouseMode[2]);
	DDX_CBIndex(pDX, IDC_MOUSE_KEY2, m_MouseMode[3]);
	DDX_CBIndex(pDX, IDC_FONTHW, m_FontHw);
}

BEGIN_MESSAGE_MAP(CScrnPage, CPropertyPage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_SCRNSIZE1, IDC_SCRNSIZE2, OnUpdateCheck)
	ON_EN_CHANGE(IDC_SCSZCOLS, OnUpdateEdit)
	ON_EN_CHANGE(IDC_SCSZCOLS2, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SCSZFONT,  OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SCSZFONT,	 OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_FONTHW,     OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_VISUALBELL, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_RECVCRLF,   OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SENDCRLF,   OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_BTN1, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_BTN2, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_KEY1, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_KEY2, OnUpdateEdit)
END_MESSAGE_MAP()

// Mouse Event Tracking
// Left			xxxx xx00	1	// Right		xxxx xx01	2
// Middle		xxxx xx10	3	// Button 1		x1xx xx00	4
// Button 2		x1xx xx01	5	// Button 3		x1xx xx10	6
// Release		xxxx xx11
// shift		xxxx x1xx		// meta			xxxx 1xxx
// ctrl			xxx1 xxxx

static const BYTE MouseModeTab[] = {
	0x00,	0x01,	0x02,	0x40,	0x41,	0x42,
	0x04,	0x08,	0x10,
};

// CScrnPage メッセージ ハンドラ

BOOL CScrnPage::OnInitDialog() 
{
	int n;

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();
	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ScrnFont = (m_pSheet->m_pTextRam->IsOptEnable(TO_RLFONT) ? 1 : 0);
	m_ColsMax[0].Format("%d",   m_pSheet->m_pTextRam->m_DefCols[0]);
	m_ColsMax[1].Format("%d",   m_pSheet->m_pTextRam->m_DefCols[1]);
	m_FontSize.Format("%d",  m_pSheet->m_pTextRam->m_DefFontSize);
	m_FontHw = m_pSheet->m_pTextRam->m_DefFontHw - 10;

	m_VisualBell = m_pSheet->m_pTextRam->IsOptValue(TO_RLADBELL, 2);
	m_RecvCrLf   = m_pSheet->m_pTextRam->IsOptValue(TO_RLRECVCR, 2);
	m_SendCrLf   = m_pSheet->m_pTextRam->IsOptValue(TO_RLECHOCR, 2);

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( MouseModeTab[n] == m_pSheet->m_pTextRam->m_MouseMode[0] )
			m_MouseMode[0] = n;
		if ( MouseModeTab[n] == m_pSheet->m_pTextRam->m_MouseMode[1] )
			m_MouseMode[1] = n;
	}
	for ( n = 0 ; n < 3 ; n++ ) {
		if ( MouseModeTab[n + 6] == m_pSheet->m_pTextRam->m_MouseMode[2] )
			m_MouseMode[2] = n;
		if ( MouseModeTab[n + 6] == m_pSheet->m_pTextRam->m_MouseMode[3] )
			m_MouseMode[3] = n;
	}

	UpdateData(FALSE);
	return TRUE;
}
BOOL CScrnPage::OnApply() 
{
	int n;

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	UpdateData(TRUE);
	for ( n = 0 ; n < CHECKOPTMAX ; n++ ) {
		if ( m_Check[n] )
			m_pSheet->m_pTextRam->EnableOption(CheckOptTab[n]);
		else
			m_pSheet->m_pTextRam->DisableOption(CheckOptTab[n]);
	}

	if ( m_ScrnFont == 1 )
		m_pSheet->m_pTextRam->EnableOption(TO_RLFONT);
	else
		m_pSheet->m_pTextRam->DisableOption(TO_RLFONT);

	m_pSheet->m_pTextRam->m_DefCols[0]  = atoi(m_ColsMax[0]);
	m_pSheet->m_pTextRam->m_DefCols[1]  = atoi(m_ColsMax[1]);
	m_pSheet->m_pTextRam->m_DefFontSize = atoi(m_FontSize);
	m_pSheet->m_pTextRam->m_DefFontHw   = m_FontHw + 10;

	m_pSheet->m_pTextRam->SetOptValue(TO_RLADBELL, 2, m_VisualBell);
	m_pSheet->m_pTextRam->SetOptValue(TO_RLRECVCR, 2, m_RecvCrLf);
	m_pSheet->m_pTextRam->SetOptValue(TO_RLECHOCR, 2, m_SendCrLf);

	m_pSheet->m_pTextRam->m_RecvCrLf = m_RecvCrLf;
	m_pSheet->m_pTextRam->m_SendCrLf = m_SendCrLf;

	m_pSheet->m_pTextRam->m_MouseMode[0] = MouseModeTab[m_MouseMode[0]];
	m_pSheet->m_pTextRam->m_MouseMode[1] = MouseModeTab[m_MouseMode[1]];
	m_pSheet->m_pTextRam->m_MouseMode[2] = MouseModeTab[m_MouseMode[2] + 6];
	m_pSheet->m_pTextRam->m_MouseMode[3] = MouseModeTab[m_MouseMode[3] + 6];

	return TRUE;
}
void CScrnPage::OnReset() 
{
	int n;

	if ( m_hWnd == NULL )
		return;

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ScrnFont = (m_pSheet->m_pTextRam->IsOptEnable(TO_RLFONT) ? 1 : 0);
	m_ColsMax[0].Format("%d",   m_pSheet->m_pTextRam->m_DefCols[0]);
	m_ColsMax[1].Format("%d",   m_pSheet->m_pTextRam->m_DefCols[1]);
	m_FontSize.Format("%d",  m_pSheet->m_pTextRam->m_DefFontSize);
	m_FontHw = m_pSheet->m_pTextRam->m_DefFontHw - 10;

	m_VisualBell = m_pSheet->m_pTextRam->IsOptValue(TO_RLADBELL, 2);
	m_RecvCrLf   = m_pSheet->m_pTextRam->IsOptValue(TO_RLRECVCR, 2);
	m_SendCrLf   = m_pSheet->m_pTextRam->IsOptValue(TO_RLECHOCR, 2);

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( MouseModeTab[n] == m_pSheet->m_pTextRam->m_MouseMode[0] )
			m_MouseMode[0] = n;
		if ( MouseModeTab[n] == m_pSheet->m_pTextRam->m_MouseMode[1] )
			m_MouseMode[1] = n;
	}
	for ( n = 0 ; n < 3 ; n++ ) {
		if ( MouseModeTab[n + 6] == m_pSheet->m_pTextRam->m_MouseMode[2] )
			m_MouseMode[2] = n;
		if ( MouseModeTab[n + 6] == m_pSheet->m_pTextRam->m_MouseMode[3] )
			m_MouseMode[3] = n;
	}

	UpdateData(FALSE);
	SetModified(FALSE);
}

void CScrnPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CScrnPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
