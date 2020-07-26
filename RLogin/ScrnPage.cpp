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

#define	CHECKOPTMAX		5
#define	IDC_CHECKFAST	IDC_OPTCHECK1
static const int CheckOptTab[] = { TO_RLNORESZ, TO_SETWINPOS, TO_RLHIDPIFSZ, TO_RLBACKHALF, TO_IMECARET };

CScrnPage::CScrnPage() : CTreePage(CScrnPage::IDD)
{
	m_ScrnFont = (-1);
	m_FontSize = _T("");
	m_ColsMax[0] = _T("");
	m_ColsMax[1] = _T("");
	m_VisualBell = 0;
	m_TypeCaret = 0;
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_FontHw = 10;
	m_TtlMode = 0;
	m_TtlRep = m_TtlCng = FALSE;
	m_CaretColor = RGB(0, 0, 0);
	m_TitleName.Empty();
	m_SleepMode = 0;
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
	DDX_CBIndex(pDX, IDC_DEFCARET, m_TypeCaret);
	DDX_CBIndex(pDX, IDC_IMECARET, m_ImeTypeCaret);
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_CHECKFAST + n, m_Check[n]);
	DDX_CBIndex(pDX, IDC_FONTHW, m_FontHw);
	DDX_Check(pDX, IDC_TERMCHECK1, m_TtlRep);
	DDX_Check(pDX, IDC_TERMCHECK2, m_TtlCng);
	DDX_CBString(pDX, IDC_SCRNOFFSLEFT, m_ScrnOffsLeft);
	DDX_CBString(pDX, IDC_SCRNOFFSRIGHT, m_ScrnOffsRight);
	DDX_Control(pDX, IDC_CARETCOL, m_ColBox);
	DDX_Control(pDX, IDC_IMECARETCOL, m_ImeColBox);
	DDX_CBString(pDX, IDC_TITLENAME, m_TitleName);
	DDX_CBIndex(pDX, IDC_SLEEPMODE, m_SleepMode);
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
	ON_CBN_SELCHANGE(IDC_DEFCARET, OnUpdateEditCaret)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_TERMCHECK1, IDC_TERMCHECK2, OnUpdateCheck)
	ON_CBN_EDITCHANGE(IDC_SCRNOFFSLEFT,  OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SCRNOFFSLEFT,	 OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SCRNOFFSRIGHT,  OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SCRNOFFSRIGHT,	 OnUpdateEdit)
	ON_WM_DRAWITEM()
	ON_STN_CLICKED(IDC_CARETCOL, &CScrnPage::OnStnClickedCaretCol)
	ON_STN_CLICKED(IDC_IMECARETCOL, &CScrnPage::OnStnClickedImeCaretCol)
	ON_CBN_EDITCHANGE(IDC_TITLENAME, &CScrnPage::OnUpdateEdit)
	ON_CBN_SELENDCANCEL(IDC_TITLENAME, &CScrnPage::OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_SLEEPMODE, &CScrnPage::OnUpdateEdit)
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
	if ( (pWnd = GetDlgItem(IDC_OPTCHECK1)) != NULL )
		pWnd->EnableWindow(m_ScrnFont == 0 ? TRUE : FALSE);
	if ( (pWnd = GetDlgItem(IDC_OPTCHECK3)) != NULL )		// TO_RLHIDPIFSZ
		pWnd->EnableWindow(m_ScrnFont != 0 ? TRUE : FALSE);
	if ( (pWnd = GetDlgItem(IDC_SCSZFONT)) != NULL )
		pWnd->EnableWindow(m_ScrnFont != 0 ? TRUE : FALSE);
}
void CScrnPage::InitFontSize()
{
	int n;
	int dpi, fontSize;
	double pixDpi;
	CString tmp;
	CClientDC dc(this);
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_SCSZFONT);

	// LOGPIXELSY 96/inc	POINT 72/inc

	dpi = dc.GetDeviceCaps(LOGPIXELSY);

	if ( m_Check[2] )	// m_pSheet->m_pTextRam->IsOptEnable(TO_RLHIDPIFSZ)
		pixDpi = (double)dpi;
	else
		pixDpi = (double)MulDiv(dpi, SCREEN_DPI_Y, DEFAULT_DPI_Y);

	for ( n = pCombo->GetCount() - 1 ; n >= 0; n-- )
		pCombo->DeleteString(n);

	for ( n = 2 ; n <= 48; n++ ) {
		tmp.Format(_T("%d (%.2f)"), n, (double)n * 72.0 / pixDpi);
		pCombo->AddString(tmp);
	}

	fontSize = m_FontSize.IsEmpty() ? m_pSheet->m_pTextRam->m_DefFontSize : _tstoi(m_FontSize);
	m_FontSize.Format(_T("%d (%.2f)"), fontSize, (double)(fontSize) * 72.0 / pixDpi);
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

	m_FontSize.Empty();
	InitFontSize();

	m_VisualBell = m_pSheet->m_pTextRam->IsOptValue(TO_RLADBELL, 2);

	if ( (m_TypeCaret = m_pSheet->m_pTextRam->m_TypeCaret) > 0 )
		m_TypeCaret--;

	m_CaretColor = m_pSheet->m_pTextRam->m_CaretColor;

	m_ImeTypeCaret = m_pSheet->m_pTextRam->m_ImeTypeCaret;
	m_ImeCaretColor = m_pSheet->m_pTextRam->m_ImeCaretColor;

	m_ColBox.Invalidate(FALSE);
	m_ImeColBox.Invalidate(FALSE);

	m_TtlMode = m_pSheet->m_pTextRam->m_TitleMode & 7;
	m_TtlRep  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_REPORT) ? TRUE : FALSE;
	m_TtlCng  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_CHENG)  ? TRUE : FALSE;

	m_TitleName = m_pSheet->m_pTextRam->m_TitleName;

	if ( m_TitleName.IsEmpty() ) {
		switch(m_TtlMode & 0007) {
		case WTTL_ENTRY:
			m_TitleName = _T("%E");
			break;
		case WTTL_HOST:
			m_TitleName = _T("%S");
			break;
		case WTTL_PORT:
			m_TitleName = _T("%S:%p");
			break;
		case WTTL_USER:
			m_TitleName = _T("%U");
			break;
		}
	}

	m_ScrnOffsLeft.Format(_T("%d"),   m_pSheet->m_pTextRam->m_ScrnOffset.left);
	m_ScrnOffsRight.Format(_T("%d"),  m_pSheet->m_pTextRam->m_ScrnOffset.right);

	m_SleepMode = m_pSheet->m_pTextRam->m_SleepMode;

	InitDlgItem();
	UpdateData(FALSE);
}
BOOL CScrnPage::OnInitDialog() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	DoInit();

	SubclassComboBox(IDC_SCSZFONT);
	SubclassComboBox(IDC_SCRNOFFSLEFT);
	SubclassComboBox(IDC_SCRNOFFSRIGHT);
	SubclassComboBox(IDC_TITLENAME);

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

	m_pSheet->m_pTextRam->m_TypeCaret = m_TypeCaret + 1;
	m_pSheet->m_pTextRam->m_CaretColor = m_CaretColor;
	
	m_pSheet->m_pTextRam->m_ImeTypeCaret = m_ImeTypeCaret;
	m_pSheet->m_pTextRam->m_ImeCaretColor = m_ImeCaretColor;

	m_pSheet->m_pTextRam->m_TitleMode = m_TtlMode;

	if ( m_TtlRep )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_REPORT;
	if ( m_TtlCng )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_CHENG;

	m_pSheet->m_pTextRam->m_TitleName = m_TitleName;

	if ( (n = _tstoi(m_ScrnOffsLeft)) != m_pSheet->m_pTextRam->m_ScrnOffset.left ) {
		m_pSheet->m_pTextRam->m_ScrnOffset.left = n;
		m_pSheet->m_ModFlag |= UMOD_RESIZE;
	}
	if ( (n = _tstoi(m_ScrnOffsRight)) != m_pSheet->m_pTextRam->m_ScrnOffset.right ) {
		m_pSheet->m_pTextRam->m_ScrnOffset.right = n;
		m_pSheet->m_ModFlag |= UMOD_RESIZE;
	}

	m_pSheet->m_pTextRam->m_SleepMode = m_SleepMode;

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

	switch(nID) {
	case IDC_SCRNSIZE1:
	case IDC_SCRNSIZE2:
		m_pSheet->m_ModFlag |= UMOD_ANSIOPT;
		UpdateData(TRUE);
		InitDlgItem();
		break;

	case IDC_OPTCHECK1:		// TO_RLNORESZ
		m_pSheet->m_ModFlag |= UMOD_ANSIOPT;
		break;
	case IDC_OPTCHECK3:		// TO_RLHIDPIFSZ
		UpdateData(TRUE);
		InitFontSize();
		UpdateData(FALSE);
		break;
	}
}
void CScrnPage::OnUpdateEditCaret()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_CARET);
}
void CScrnPage::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	CRect rect;

	if ( nIDCtl == IDC_CARETCOL ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_ColBox.GetClientRect(rect);
		dc.FillSolidRect(rect, m_CaretColor);
		dc.Detach();
	} else if ( nIDCtl == IDC_IMECARETCOL ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_ImeColBox.GetClientRect(rect);
		dc.FillSolidRect(rect, m_ImeCaretColor);
		dc.Detach();
	} else
		CTreePage::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
void CScrnPage::OnStnClickedCaretCol()
{
	CColorDialog cdl(m_CaretColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this);

	if ( DpiAwareDoModal(cdl) != IDOK )
		return;

	m_CaretColor = cdl.GetColor();
	m_ColBox.Invalidate(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_CARET);
}
void CScrnPage::OnStnClickedImeCaretCol()
{
	CColorDialog cdl(m_ImeCaretColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this);

	if ( DpiAwareDoModal(cdl) != IDOK )
		return;

	m_ImeCaretColor = cdl.GetColor();
	m_ImeColBox.Invalidate(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM);
}