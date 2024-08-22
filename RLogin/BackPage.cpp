// BackPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "OPtDlg.h"
#include "TextRam.h"
#include "Data.h"
#include "BackPage.h"

/////////////////////////////////////////////////////////////////////////////
// CBackPage ダイアログ

IMPLEMENT_DYNAMIC(CBackPage, CTreePage)

CBackPage::CBackPage() : CTreePage(CBackPage::IDD)
{
	m_BitMapFile.Empty();
	m_bEnable = FALSE;
	m_HAlign = 0;
	m_VAlign = 0;
	m_TextFormat.Empty();
	m_MapStyle = 0;
	m_UrlOpt = _T("#BACKSET");
	m_TextColor = m_TabTextColor = m_TabBackColor = 0;
	m_TabBackGradient = FALSE;
	m_bTabColDark = FALSE;
	ZeroMemory(&m_LogFont, sizeof(m_LogFont));
}
CBackPage::~CBackPage()
{
}

void CBackPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_CBStringExact(pDX, IDC_BACKFILE, m_BitMapFile);
	DDX_Control(pDX, IDC_BACKFILE, m_BitMapCombo);

	DDX_Check(pDX, IDC_TEXTMAPENABLE, m_bEnable);
	DDX_CBIndex(pDX, IDC_HALIGN, m_HAlign);
	DDX_CBIndex(pDX, IDC_VALIGN, m_VAlign);
	DDX_Text(pDX, IDC_TEXTFORMAT, m_TextFormat);
	DDX_Control(pDX, IDC_BITMAPALPHA, m_BitMapAlpha);
	DDX_Control(pDX, IDC_BITMAPBLEND, m_BitMapBlend);
	DDX_Control(pDX, IDC_TEXTCOLOR, m_ColorBox);
	DDX_CBIndex(pDX, IDC_MAPSTYLE, m_MapStyle);
	DDX_Control(pDX, IDC_TABTEXTCOL, m_TabTextColorBox);
	DDX_Control(pDX, IDC_TABBACKCOL, m_TabBackColorBox);
	DDX_Check(pDX, IDC_TABBACKGRAD, m_TabBackGradient);
}

BEGIN_MESSAGE_MAP(CBackPage, CTreePage)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_TEXTFONT, &CBackPage::OnBnClickedTextfont)
	ON_STN_CLICKED(IDC_TEXTCOLOR, &CBackPage::OnStnClickedTextcolor)
	ON_CBN_SELCHANGE(IDC_BACKFILE, &CBackPage::OnUpdateEdit)
	ON_CBN_EDITUPDATE(IDC_BACKFILE, &CBackPage::OnUpdateEdit)
	ON_BN_CLICKED(IDC_BACKSEL, &CBackPage::OnBitMapFileSel)
	ON_BN_CLICKED(IDC_TEXTMAPENABLE, &CBackPage::OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_HALIGN, &CBackPage::OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_VALIGN, &CBackPage::OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MAPSTYLE, &CBackPage::OnUpdateEdit)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_BITMAPALPHA, &CBackPage::OnUpdateSlider)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_BITMAPBLEND, &CBackPage::OnUpdateSlider)
	ON_STN_CLICKED(IDC_TABTEXTCOL, &CBackPage::OnStnClickedTabTextColor)
	ON_STN_CLICKED(IDC_TABBACKCOL, &CBackPage::OnStnClickedTabBackColor)
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

void CBackPage::DoInit()
{
	m_BitMapFile = m_pSheet->m_pTextRam->m_BitMapFile;

	m_bEnable    = m_pSheet->m_pTextRam->m_TextBitMap.m_bEnable;
	m_TextFormat = m_pSheet->m_pTextRam->m_TextBitMap.m_Text;
	m_TextColor  = m_pSheet->m_pTextRam->m_TextBitMap.m_TextColor;
	m_MapStyle   = m_pSheet->m_pTextRam->m_BitMapStyle;

	memcpy(&m_LogFont, &(m_pSheet->m_pTextRam->m_TextBitMap.m_LogFont), sizeof(m_LogFont));

	switch(m_pSheet->m_pTextRam->m_TextBitMap.m_WidthAlign) {
	case DT_LEFT:   m_HAlign = 0; break;
	case DT_CENTER: m_HAlign = 1; break;
	case DT_RIGHT:  m_HAlign = 2; break;
	}

	switch(m_pSheet->m_pTextRam->m_TextBitMap.m_HeightAlign) {
	case DT_TOP:     m_VAlign = 0; break;
	case DT_VCENTER: m_VAlign = 1; break;
	case DT_BOTTOM:  m_VAlign = 2; break;
	}

	m_BitMapAlpha.SetRange(0, 511);
	m_BitMapAlpha.SetTicFreq(32);
	m_BitMapAlpha.SetPos(m_pSheet->m_pTextRam->m_BitMapAlpha);

	m_BitMapBlend.SetRange(0, 255);
	m_BitMapBlend.SetTicFreq(16);
	m_BitMapBlend.SetPos(m_pSheet->m_pTextRam->m_BitMapBlend);

	if ( m_bDarkMode ) {
		m_TabBackColor = m_pSheet->m_pTextRam->m_TabTextColor;
		m_TabTextColor = m_pSheet->m_pTextRam->m_TabBackColor;
		m_bTabColDark = TRUE;
	} else {
		m_TabTextColor = m_pSheet->m_pTextRam->m_TabTextColor;
		m_TabBackColor = m_pSheet->m_pTextRam->m_TabBackColor;
		m_bTabColDark = FALSE;
	}

	m_TabBackGradient = m_pSheet->m_pTextRam->IsOptEnable(TO_RLTABGRAD);

	UpdateData(FALSE);

	m_ColorBox.Invalidate(FALSE);
	m_TabTextColorBox.Invalidate(FALSE);
	m_TabBackColorBox.Invalidate(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CBackPage メッセージ ハンドラー

BOOL CBackPage::OnInitDialog()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	DoInit();

	m_BitMapCombo.LoadHis(_T("BackPageBitMapFile"));

	return TRUE;
}
BOOL CBackPage::OnApply()
{
	UpdateData(TRUE);

	m_pSheet->m_pTextRam->m_BitMapFile = m_BitMapFile;

	switch(m_HAlign) {
	case 0: m_pSheet->m_pTextRam->m_TextBitMap.m_WidthAlign = DT_LEFT;   break;
	case 1: m_pSheet->m_pTextRam->m_TextBitMap.m_WidthAlign = DT_CENTER; break;
	case 2: m_pSheet->m_pTextRam->m_TextBitMap.m_WidthAlign = DT_RIGHT; break;
	}

	switch(m_VAlign) {
	case 0: m_pSheet->m_pTextRam->m_TextBitMap.m_HeightAlign = DT_TOP;     break;
	case 1: m_pSheet->m_pTextRam->m_TextBitMap.m_HeightAlign = DT_VCENTER; break;
	case 2: m_pSheet->m_pTextRam->m_TextBitMap.m_HeightAlign = DT_BOTTOM;  break;
	}

	m_pSheet->m_pTextRam->m_TextBitMap.m_bEnable   = m_bEnable;
	m_pSheet->m_pTextRam->m_TextBitMap.m_Text      = m_TextFormat;
	m_pSheet->m_pTextRam->m_TextBitMap.m_TextColor = m_TextColor;

	memcpy(&(m_pSheet->m_pTextRam->m_TextBitMap.m_LogFont), &m_LogFont, sizeof(m_LogFont));

	m_pSheet->m_pTextRam->m_BitMapAlpha = m_BitMapAlpha.GetPos();
	m_pSheet->m_pTextRam->m_BitMapBlend = m_BitMapBlend.GetPos();
	m_pSheet->m_pTextRam->m_BitMapStyle = m_MapStyle;

	if ( m_bTabColDark ) {
		m_pSheet->m_pTextRam->m_TabTextColor = m_TabBackColor;
		m_pSheet->m_pTextRam->m_TabBackColor = m_TabTextColor;
	} else {
		m_pSheet->m_pTextRam->m_TabTextColor = m_TabTextColor;
		m_pSheet->m_pTextRam->m_TabBackColor = m_TabBackColor;
	}

	if ( m_TabBackGradient != m_pSheet->m_pTextRam->IsOptEnable(TO_RLTABGRAD) ) {
		m_pSheet->m_pTextRam->SetOption(TO_RLTABGRAD, m_TabBackGradient);
		m_pSheet->m_ModFlag |= UMOD_TABCOLOR;
	}

	m_BitMapCombo.AddHis(m_BitMapFile);

	return TRUE;
}
void CBackPage::OnReset()
{
	DoInit();
	SetModified(FALSE);
}
void CBackPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CBackPage::OnBitMapFileSel() 
{
	CString file;

	UpdateData(TRUE);
	file = m_BitMapFile;

	CRLoginDoc::EnvironPath(file);
	CFileDialog dlg(TRUE, _T("jpg"), file, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGIMAGE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_BitMapFile = dlg.GetPathName();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CBackPage::OnBnClickedTextfont()
{
	CFontDialog font(&m_LogFont, CF_NOVERTFONTS | CF_SCREENFONTS | CF_INACTIVEFONTS, NULL, this);

	if ( DpiAwareDoModal(font) != IDOK )
		return;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CBackPage::OnStnClickedTextcolor()
{
	CColorDialog cdl(m_TextColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this);

	if ( DpiAwareDoModal(cdl) != IDOK )
		return;

	m_TextColor = cdl.GetColor();
	m_ColorBox.Invalidate(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CBackPage::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	CRect rect;

	if ( nIDCtl == IDC_TEXTCOLOR ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_ColorBox.GetClientRect(rect);
		dc.FillSolidRect(rect, m_TextColor);
		dc.Detach();
	} else if ( nIDCtl == IDC_TABTEXTCOL ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_TabTextColorBox.GetClientRect(rect);
		dc.FillSolidRect(rect, m_TabTextColor);
		dc.Detach();
	} else if ( nIDCtl == IDC_TABBACKCOL ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_TabBackColorBox.GetClientRect(rect);
		dc.FillSolidRect(rect, m_TabBackColor);
		dc.Detach();
	} else
		CTreePage::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CBackPage::OnUpdateSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;

	*pResult = 0;
}

void CBackPage::OnStnClickedTabTextColor()
{
	CColorDialog cdl(m_TabTextColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this);

	if ( DpiAwareDoModal(cdl) != IDOK )
		return;

	m_TabTextColor = cdl.GetColor();
	m_TabTextColorBox.Invalidate(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_TABCOLOR);
}
void CBackPage::OnStnClickedTabBackColor()
{
	CColorDialog cdl(m_TabBackColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this);

	if ( DpiAwareDoModal(cdl) != IDOK )
		return;

	m_TabBackColor = cdl.GetColor();
	m_TabBackColorBox.Invalidate(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_TABCOLOR);
}
void CBackPage::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDialogExt::OnSettingChange(uFlags, lpszSection);

	if ( m_bTabColDark != m_bDarkMode ) {
		COLORREF col = m_TabTextColor;
		m_TabTextColor = m_TabBackColor;
		m_TabBackColor = col;
		m_TabTextColorBox.Invalidate(FALSE);
		m_TabBackColorBox.Invalidate(FALSE);
		m_bTabColDark = m_bDarkMode;
	}
}
