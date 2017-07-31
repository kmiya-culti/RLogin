// ScrnPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "ScrnPage.h"

/////////////////////////////////////////////////////////////////////////////
// CScrnPage ダイアログ

IMPLEMENT_DYNAMIC(CScrnPage, CTreePage)

#define	CHECKOPTMAX		1
#define	IDC_CHECKFAST	IDC_TERMCHECK1
static const int CheckOptTab[] = { TO_RLNORESZ };

CScrnPage::CScrnPage() : CTreePage(CScrnPage::IDD)
{
	m_ScrnFont = (-1);
	m_FontSize = _T("");
	m_ColsMax[0] = _T("");
	m_ColsMax[1] = _T("");
	m_VisualBell = 0;
	m_DefTypeCaret = 0;
	m_RecvCrLf = 0;
	m_SendCrLf = 0;
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_FontHw = 10;
	m_TtlMode = 0;
	m_TtlRep = m_TtlCng = FALSE;
}

CScrnPage::~CScrnPage()
{
}

void CScrnPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_SCRNSIZE1, m_ScrnFont);
	DDX_CBString(pDX, IDC_SCSZFONT, m_FontSize);
	DDX_Text(pDX, IDC_SCSZCOLS, m_ColsMax[0]);
	DDX_Text(pDX, IDC_SCSZCOLS2, m_ColsMax[1]);
	DDX_CBIndex(pDX, IDC_VISUALBELL, m_VisualBell);
	DDX_CBIndex(pDX, IDC_DEFCARET, m_DefTypeCaret);
	DDX_CBIndex(pDX, IDC_RECVCRLF, m_RecvCrLf);
	DDX_CBIndex(pDX, IDC_SENDCRLF, m_SendCrLf);
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);
	DDX_CBIndex(pDX, IDC_FONTHW, m_FontHw);
	DDX_CBIndex(pDX, IDC_COMBO3, m_TtlMode);
	DDX_Check(pDX, IDC_TERMCHECK2, m_TtlRep);
	DDX_Check(pDX, IDC_TERMCHECK3, m_TtlCng);
	DDX_CBString(pDX, IDC_SCRNOFFSLEFT, m_ScrnOffsLeft);
	DDX_CBString(pDX, IDC_SCRNOFFSRIGHT, m_ScrnOffsRight);
}

BEGIN_MESSAGE_MAP(CScrnPage, CTreePage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_SCRNSIZE1, IDC_SCRNSIZE2, OnUpdateCheck)
	ON_EN_CHANGE(IDC_SCSZCOLS, OnUpdateEdit)
	ON_EN_CHANGE(IDC_SCSZCOLS2, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SCSZFONT,  OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SCSZFONT,	 OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_FONTHW,     OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_VISUALBELL, OnUpdateEditOpt)
	ON_CBN_SELCHANGE(IDC_DEFCARET, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_RECVCRLF,   OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SENDCRLF,   OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnCbnSelchangeCombo)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_TERMCHECK2, IDC_TERMCHECK3, OnUpdateCheck)
	ON_CBN_EDITCHANGE(IDC_SCRNOFFSLEFT,  OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SCRNOFFSLEFT,	 OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SCRNOFFSRIGHT,  OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SCRNOFFSRIGHT,	 OnUpdateEdit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScrnPage メッセージ ハンドラ

void CScrnPage::InitDlgItem()
{
	CWnd *pWnd;

	if ( (pWnd = GetDlgItem(IDC_SCSZCOLS)) != NULL )
		pWnd->EnableWindow(m_ScrnFont == 0 ? TRUE : FALSE);
	if ( (pWnd = GetDlgItem(IDC_SCSZCOLS2)) != NULL )
		pWnd->EnableWindow(m_ScrnFont == 0 ? TRUE : FALSE);
	if ( (pWnd = GetDlgItem(IDC_TERMCHECK1)) != NULL )
		pWnd->EnableWindow(m_ScrnFont == 0 ? TRUE : FALSE);
	if ( (pWnd = GetDlgItem(IDC_SCSZFONT)) != NULL )
		pWnd->EnableWindow(m_ScrnFont != 0 ? TRUE : FALSE);
}
void CScrnPage::DoInit()
{
	int n;

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ScrnFont = (m_pSheet->m_pTextRam->IsOptEnable(TO_RLFONT) ? 1 : 0);
	m_ColsMax[0].Format(_T("%d"), m_pSheet->m_pTextRam->m_DefCols[0]);
	m_ColsMax[1].Format(_T("%d"), m_pSheet->m_pTextRam->m_DefCols[1]);

	m_FontHw = m_pSheet->m_pTextRam->m_DefFontHw - 10;

	// LOGPIXELSY 96/inc	POINT 72/inc
	m_FontSize.Format(_T("%d (%.2f)"), m_pSheet->m_pTextRam->m_DefFontSize, (double)(m_pSheet->m_pTextRam->m_DefFontSize) * 72.0 / m_PixDpiY);

	m_VisualBell = m_pSheet->m_pTextRam->IsOptValue(TO_RLADBELL, 2);
	if ( (m_DefTypeCaret = m_pSheet->m_pTextRam->m_DefTypeCaret) > 0 )
		m_DefTypeCaret--;

	m_RecvCrLf   = m_pSheet->m_pTextRam->m_RecvCrLf;
	m_SendCrLf   = m_pSheet->m_pTextRam->m_SendCrLf;

	m_TtlMode = m_pSheet->m_pTextRam->m_TitleMode & 7;
	m_TtlRep  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_REPORT) ? TRUE : FALSE;
	m_TtlCng  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_CHENG)  ? TRUE : FALSE;

	m_ScrnOffsLeft.Format(_T("%d"),   m_pSheet->m_pTextRam->m_ScrnOffset.left);
	m_ScrnOffsRight.Format(_T("%d"),  m_pSheet->m_pTextRam->m_ScrnOffset.right);

	InitDlgItem();
	UpdateData(FALSE);
}
BOOL CScrnPage::OnInitDialog() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	int n;
	CString tmp;
	CClientDC dc(this);
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_SCSZFONT);

	m_PixDpiY = (double)MulDiv(dc.GetDeviceCaps(LOGPIXELSY), ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96);

	for ( n = pCombo->GetCount() - 1 ; n >= 0; n-- )
		pCombo->DeleteString(n);

	for ( n = 2 ; n < 20 ; n++ ) {
		tmp.Format(_T("%d (%.2f)"), n, (double)n * 72.0 / m_PixDpiY);
		pCombo->AddString(tmp);
	}

	DoInit();

	return TRUE;
}
BOOL CScrnPage::OnApply() 
{
	int n;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	m_pSheet->m_pTextRam->SetOption(TO_RLFONT, (m_ScrnFont == 1 ? TRUE : FALSE));

	m_pSheet->m_pTextRam->m_DefCols[0]  = _tstoi(m_ColsMax[0]);
	m_pSheet->m_pTextRam->m_DefCols[1]  = _tstoi(m_ColsMax[1]);

	m_pSheet->m_pTextRam->m_DefFontSize = _tstoi(m_FontSize);
	m_pSheet->m_pTextRam->m_DefFontHw   = m_FontHw + 10;

	m_pSheet->m_pTextRam->SetOptValue(TO_RLADBELL, 2, m_VisualBell);

	m_pSheet->m_pTextRam->m_DefTypeCaret = m_DefTypeCaret + 1;
	m_pSheet->m_pTextRam->m_TypeCaret    = m_pSheet->m_pTextRam->m_DefTypeCaret;

	m_pSheet->m_pTextRam->m_RecvCrLf = m_RecvCrLf;
	m_pSheet->m_pTextRam->m_SendCrLf = m_SendCrLf;

	m_pSheet->m_pTextRam->m_TitleMode = m_TtlMode;
	if ( m_TtlRep )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_REPORT;
	if ( m_TtlCng )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_CHENG;

	if ( (n = _tstoi(m_ScrnOffsLeft)) != m_pSheet->m_pTextRam->m_ScrnOffset.left ) {
		m_pSheet->m_pTextRam->m_ScrnOffset.left = n;
		m_pSheet->m_ModFlag |= UMOD_RESIZE;
	}
	if ( (n = _tstoi(m_ScrnOffsRight)) != m_pSheet->m_pTextRam->m_ScrnOffset.right ) {
		m_pSheet->m_pTextRam->m_ScrnOffset.right = n;
		m_pSheet->m_ModFlag |= UMOD_RESIZE;
	}

	return TRUE;
}
void CScrnPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}

void CScrnPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CScrnPage::OnUpdateEditOpt() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_ANSIOPT);
}
void CScrnPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;

	if ( nID == IDC_SCRNSIZE1 || nID == IDC_SCRNSIZE2 ) {
		m_pSheet->m_ModFlag |= UMOD_ANSIOPT;
		UpdateData(TRUE);
		InitDlgItem();
	} else if ( nID >= IDC_CHECKFAST && nID <= (IDC_CHECKFAST + CHECKOPTMAX - 1) )
		m_pSheet->m_ModFlag |= UMOD_ANSIOPT;
}
void CScrnPage::OnCbnSelchangeCombo()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
