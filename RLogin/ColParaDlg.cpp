// ColParaDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "OPtDlg.h"
#include "TextRam.h"
#include "Data.h"
#include "ColParaDlg.h"
#include "TtyModeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColParaDlg プロパティ ページ

IMPLEMENT_DYNCREATE(CColParaDlg, CTreePage)

CColParaDlg::CColParaDlg() : CTreePage(CColParaDlg::IDD)
{
	m_ColSet = -1;
	for ( int n = 0 ; n < 24 ; n++ )
		m_Attrb[n] = 0;
	m_WakeUpSleep = 0;
	m_FontCol[0] = 0;
	m_FontCol[1] = 0;
	m_FontColName[0] = _T("0");
	m_FontColName[1] = _T("0");
	m_GlassStyle = FALSE;
	m_EmojiFontName = _T("");
	m_EmojiImageDir = _T("");
	m_EmojiColorEnable = FALSE;
	m_UrlOpt = _T("#COLOPT");
}
CColParaDlg::~CColParaDlg()
{
}

void CColParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TRANSPARENT, m_TransSlider);
	DDX_CBIndex(pDX, IDC_COLSET, m_ColSet);
	for ( int n = 0 ; n < 16 ; n++ )
		DDX_Control(pDX, IDC_BOX0 + n, m_ColBox[n]);
	DDX_Control(pDX, IDC_BOXTEXT, m_ColBox[16]);
	DDX_Control(pDX, IDC_BOXBACK, m_ColBox[17]);
	for ( int n = 0 ; n < 8 ; n++ )
		DDX_Check(pDX, IDC_ATTR1 + n, m_Attrb[n]);
	DDX_CBIndex(pDX, IDC_WAKEUPSLEEP, m_WakeUpSleep);
	DDX_Text(pDX, IDC_TEXTCOL, m_FontColName[0]);
	DDX_Text(pDX, IDC_BACKCOL, m_FontColName[1]);
	DDX_Check(pDX, IDC_GLASSSTYLE, m_GlassStyle);
	DDX_Control(pDX, IDC_SLIDER_CONTRAST, m_SliderConstrast);
	DDX_Control(pDX, IDC_SLIDER_BRIGHT, m_SliderBright);
	DDX_Control(pDX, IDC_SLIDER_HUECOL, m_SliderHuecol);
	DDX_CBString(pDX, IDC_EMOJIFONTNAME, m_EmojiFontName);
	DDX_Text(pDX, IDC_EMOJIIMAGEDIR, m_EmojiImageDir);
	DDX_Check(pDX, IDC_EMOJICOLENABLE, m_EmojiColorEnable);
}

BEGIN_MESSAGE_MAP(CColParaDlg, CTreePage)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_TRANSPARENT, OnReleasedcaptureTransparent)
	ON_CBN_SELENDOK(IDC_COLSET, OnSelendokColset)
	ON_EN_CHANGE(IDC_TEXTCOL, &CColParaDlg::OnEnChangeColor)
	ON_EN_CHANGE(IDC_BACKCOL, &CColParaDlg::OnEnChangeColor)
	ON_BN_CLICKED(IDC_GLASSSTYLE, &CColParaDlg::OnBnClickedGlassStyle)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_ATTR1, IDC_ATTR8, &CColParaDlg::OnUpdateCheck)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_CONTRAST, &CColParaDlg::OnNMReleasedcaptureContrast)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_BRIGHT, &CColParaDlg::OnNMReleasedcaptureContrast)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_HUECOL, &CColParaDlg::OnNMReleasedcaptureContrast)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_COLEDIT, &CColParaDlg::OnBnClickedColedit)
	ON_BN_CLICKED(IDC_EMOJIIMAGESEL, &CColParaDlg::OnBnClickedImageSel)
	ON_CBN_SELCHANGE(IDC_EMOJIFONTNAME, &CColParaDlg::OnUpdateTextRam)
	ON_EN_CHANGE(IDC_EMOJIIMAGEDIR, &CColParaDlg::OnUpdateTextRam)
	ON_BN_CLICKED(IDC_EMOJICOLENABLE, &CColParaDlg::OnUpdateTextRam)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColParaDlg メッセージ ハンドラ

const COLORREF	ColSetTab[COLSETTABMAX][16] = {
	{	RGB(  0,   0,   0),	RGB(196,  64,  64),	RGB( 64, 196,  64),	RGB(196, 196,  64),
		RGB( 64,  64, 196),	RGB(196,  64, 196),	RGB( 64, 196, 196),	RGB(196, 196, 196),
		RGB(128, 128, 128),	RGB(255,  96,  96),	RGB( 96, 255,  96),	RGB(255, 255,  96),
		RGB( 96,  96, 255),	RGB(255,  96, 255),	RGB( 96, 255, 255),	RGB(255, 255, 255)	},	// Black

	{	RGB( 26,   0,   0),	RGB(196,   0,   0),	RGB(  0, 196,   0),	RGB(127, 196,   0),
		RGB(  0,   0, 196),	RGB(196,   0, 127),	RGB(  0, 127, 196),	RGB(196,  96,   0),
		RGB( 96,  31,  31),	RGB(255,   0,   0),	RGB(  0, 255,   0),	RGB(196, 255,   0),
		RGB(  0,   0, 255),	RGB(255,   0, 196),	RGB(  0, 196, 255),	RGB(255, 127,   0)	},	// Amber

	{	RGB(  0,  26,   0),	RGB(196,   0,   0),	RGB(127, 196,   0),	RGB(196, 119,   0),
		RGB(  0,   0, 196),	RGB(127,   0, 196),	RGB(  0, 196, 168),	RGB(  0, 196,  31),
		RGB( 31,  96,  31),	RGB(255,   0,   0),	RGB(168, 255,   0),	RGB(255, 196,   0),
		RGB(  0,   0, 255),	RGB(196,   0, 255),	RGB(  0, 255, 196),	RGB(  0, 255, 127)	},	// Green

	{	RGB(  0,   0,  26),	RGB(196,   0,   0),	RGB(127, 196,   0),	RGB(196, 119,   0),
		RGB(  0,   0, 196),	RGB(127,   0, 196),	RGB(  0, 196, 168),	RGB(  0, 168, 196),
		RGB( 31,  64,  96),	RGB(255,   0,   0),	RGB(168, 255,   0),	RGB(255, 196,   0),
		RGB(  0,   0, 255),	RGB(196,   0, 255),	RGB(  0, 255, 196),	RGB(  0, 196, 255)	},	// Sian

	{	RGB(255, 255, 255),	RGB(196,  64,  64),	RGB( 32, 128,   0),	RGB(164, 128,  32),
		RGB(  0,  64, 128),	RGB(196,   0, 196),	RGB(  0, 128, 128),	RGB(  0,   0,   0),
		RGB(224, 224, 224),	RGB(255,  32,  32),	RGB( 32, 224,   0),	RGB(255, 224,   0),
		RGB( 32,  32, 224),	RGB(255,   0, 255),	RGB( 32, 196, 224),	RGB( 96,  96,  96)	},	// White

	{	RGB(  0,  43,  54),	RGB(203,  75,  22),	RGB( 88, 110, 117),	RGB(101, 123, 131),
		RGB(131, 148, 150),	RGB(108, 113, 196),	RGB(147, 161, 161),	RGB(238, 232, 213),
		RGB(  7,  54,  66),	RGB(220,  50,  47),	RGB(133, 153,   0),	RGB(181, 137,   0),
		RGB( 38, 139, 210),	RGB(211,  54, 130),	RGB( 42, 161, 152),	RGB(253, 246, 227)	},	// Solarized

	{	RGB( 38,  38,  38),	RGB(253, 115, 255),	RGB( 96, 255, 167),	RGB(253, 244, 198),
		RGB( 96, 107, 255),	RGB(253, 202, 150),	RGB(182, 255, 255),	RGB(238, 238, 238),
		RGB(123, 123, 123),	RGB(254, 156, 255),	RGB(171, 255, 206),	RGB(254, 223, 223),
		RGB(176, 182, 255),	RGB(254, 220, 181),	RGB(203, 255, 255),	RGB(255, 255, 255)	}, 	// Pastel

	{	RGB(  0,   0,   0),	RGB(249,  38, 114),	RGB(166, 226,  46),	RGB(244, 191, 117),
		RGB(102, 217, 239),	RGB(174, 129, 255),	RGB(161, 239, 228),	RGB(248, 248, 242),
		RGB(117, 113,  94),	RGB(204,   6,  78),	RGB(122, 172,  24),	RGB(240, 169,  69),
		RGB( 33, 199, 233),	RGB(126,  51, 255),	RGB( 95, 227, 210),	RGB(249, 248, 245) },	// Monokai

	{	RGB(  0,   0,	0),	RGB(204,   0,	0),	RGB( 78, 154,	6),	RGB(196, 160,	0),
		RGB( 52, 101, 164),	RGB(117,  80, 123),	RGB(  6, 152, 154),	RGB(211, 215, 207),
		RGB( 85,  87,  83),	RGB(239,  41,  41),	RGB(138, 226,  52),	RGB(252, 233,  79),
		RGB(114, 159, 207),	RGB(173, 127, 168),	RGB( 52, 226, 226),	RGB(238, 238, 236)	},	// Tango

	{	RGB( 12,  12,  12),	RGB(197,  15,  31),	RGB( 19, 161,  14),	RGB(193, 156,	0),
		RGB(  0,  55, 218),	RGB(136,  23, 152),	RGB( 58, 150, 221),	RGB(204, 204, 204),
		RGB(118, 118, 118),	RGB(231,  72,  86),	RGB( 22, 198,  12),	RGB(249, 241, 165),
		RGB( 59, 120, 255),	RGB(180,   0, 158),	RGB( 97, 214, 214),	RGB(242, 242, 242)	},	// Campbell

	{	RGB(  0,   0,   0),	RGB(255,  75,   0),	RGB(  3, 175, 122),	RGB(255, 241,   0),
		RGB(  0,  90, 255),	RGB(153,   0, 153),	RGB( 77, 196, 255),	RGB(200, 200, 203),
		RGB(132, 145, 158),	RGB(255, 202, 128),	RGB(119, 217, 168),	RGB(255, 255, 128),
		RGB(90, 180, 255),	RGB(201, 172, 230),	RGB(191, 228, 255),	RGB(255, 255, 255)	},	// CUD

	{	RGB( 56,  58,  66),	RGB(228,  86,  73),	RGB( 80, 161,  79),	RGB(193, 131,   1),
		RGB(  1, 132, 188),	RGB(166,  38, 164),	RGB(  9, 151, 179),	RGB(250, 250, 250),
		RGB( 79,  82,  93),	RGB(223, 108, 117),	RGB(152, 195, 121),	RGB(228, 192, 122),
		RGB( 97, 175, 239),	RGB(197, 119, 221),	RGB( 86, 181, 193),	RGB(255, 255, 255)	},	// One Half

	{	RGB(  0,   0,   0),	RGB(128,   0,   0),	RGB(  0, 128,   0),	RGB(128, 128,   0),
		RGB(  0,   0, 128),	RGB(128,   0, 128),	RGB(  0, 128, 128),	RGB(192, 192, 192),
		RGB(128, 128, 128),	RGB(255,   0,   0),	RGB(  0, 255,   0),	RGB(255, 255,   0),
		RGB(  0,   0, 255),	RGB(255,   0, 255),	RGB(  0, 255, 255),	RGB(255, 255, 255)	},	// Vintage
};

void CColParaDlg::ColSetCheck()
{
	int n;

	m_ColSet = -1;
	for ( n = 0 ; n < COLSETTABMAX ; n++ ) {
		if ( memcmp(m_ColTab, ColSetTab[n], sizeof(COLORREF) * 16) == 0 ) {
			m_ColSet = n;
			break;
		}
	}
}
void CColParaDlg::DoInit()
{
	int n;

	m_SliderConstrast.SetPos(50);
	m_SliderBright.SetPos(50);
	m_SliderHuecol.SetPos(150);

	SetDarkLight();
	Invalidate(FALSE);

	for ( n = 0 ; n < 16 ; n++ )
		m_ColTab[n] = m_pSheet->m_pTextRam->m_ColTab[n];

	ColSetCheck();

	m_FontCol[0] = m_pSheet->m_pTextRam->m_AttNow.std.fcol;
	m_FontCol[1] = m_pSheet->m_pTextRam->m_AttNow.std.bcol;

	m_FontColName[0].Format(_T("%d"), m_FontCol[0]);
	m_FontColName[1].Format(_T("%d"), m_FontCol[1]);

	for ( n = 0 ; n < 24 ; n++ )
		m_Attrb[n] = (m_pSheet->m_pTextRam->m_AttNow.std.attr & (1 << n) ? TRUE : FALSE);

	n = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("LayeredWindow"), 255);
	m_TransSlider.SetPos(n);

	n = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("WakeUpSleep"), 0);
	if      ( n ==    0 ) m_WakeUpSleep = 0;
	else if ( n <=   60 ) m_WakeUpSleep = 1;
	else if ( n <=  180 ) m_WakeUpSleep = 2;
	else if ( n <=  600 ) m_WakeUpSleep = 3;
	else if ( n <= 1200 ) m_WakeUpSleep = 4;
	else if ( n <= 1800 ) m_WakeUpSleep = 5;
	else if ( n <= 2400 ) m_WakeUpSleep = 6;
	else if ( n <= 3000 ) m_WakeUpSleep = 7;
	else                  m_WakeUpSleep = 8;

	m_GlassStyle = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("GlassStyle"), 255);

	m_EmojiColorEnable = m_pSheet->m_pTextRam->IsOptEnable(TO_RLCOLEMOJI);
	m_EmojiFontName = AfxGetApp()->GetProfileString(_T("RLoginApp"), _T("EmojiFontName"), _T("Segoe UI emoji"));
	m_EmojiImageDir = AfxGetApp()->GetProfileString(_T("RLoginApp"), _T("EmojiImageDir"), _T(""));

	UpdateData(FALSE);
}
void CColParaDlg::SetDarkLight()
{
	int n;

	// Contrast  0 -  50 - 100	->	10.0 - 1.0 - 0.1
	// Bright    0 -  50 - 100	->	10.0 - 1.0 - 0.1
	// Huecol   50 - 150 - 250  ->  0 - 100

	n = m_SliderConstrast.GetPos();
	m_Constrast = pow(100.0, (double)(100 - n) / 100.0) / 10.0;

	n = m_SliderBright.GetPos();
	m_Bright = pow(100.0, (double)(100 - n) / 100.0) / 10.0;

	n = m_SliderHuecol.GetPos();

	if ( (n += 150) > 300 )
		n -= 300;

	if ( n < 100 ) {
		m_Huecol[0] = 100 - n;
		m_Huecol[1] = n;
		m_Huecol[2] = 0;
	} else if ( n < 200 ) {
		m_Huecol[0] = 0;
		m_Huecol[1] = 200 - n;
		m_Huecol[2] = n - 100;
	} else {
		m_Huecol[0] = n - 200;
		m_Huecol[1] = 0;
		m_Huecol[2] = 300 - n;
	}
}
COLORREF CColParaDlg::EditColor(int num)
{
	int n, col[3];
	double rgb[3];
	BYTE r, g, b;

	rgb[0] = (double)GetRValue(m_ColTab[num]);
	rgb[1] = (double)GetGValue(m_ColTab[num]);
	rgb[2] = (double)GetBValue(m_ColTab[num]);

	for ( n = 0 ; n < 3 ; n++ ) {
		col[n] = (int)(((rgb[n] - 128.0) * m_Constrast + 128.0) * m_Bright);
		if ( col[n] < 0 )
			col[n] = 0;
		else if ( col[n] > 255 )
			col[n] = 255;
	}

	r = (BYTE)((col[0] * m_Huecol[0] + col[1] * m_Huecol[1] + col[2] * m_Huecol[2]) / 100);
	g = (BYTE)((col[1] * m_Huecol[0] + col[2] * m_Huecol[1] + col[0] * m_Huecol[2]) / 100);
	b = (BYTE)((col[2] * m_Huecol[0] + col[0] * m_Huecol[1] + col[1] * m_Huecol[2]) / 100);

	return RGB(r, g, b);
}
static int CALLBACK EnumFontFamExComboAddStr(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam)
{
	CComboBox *pCombo = (CComboBox *)lParam;
	LPCTSTR name = lpelfe->elfLogFont.lfFaceName;

	if ( name[0] != _T('@') && pCombo->FindStringExact((-1), name) == CB_ERR )
		pCombo->AddString(name);

	return TRUE;
}
BOOL CColParaDlg::OnInitDialog() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	int n;
	CButton *pWnd;
	BUTTON_IMAGELIST list;
	CBitmapEx Bitmap;
	int width, height;

	memset(&list, 0, sizeof(list));
	list.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;

	width  = MulDiv(40, m_NowDpi.cx, DEFAULT_DPI_X);
	height = MulDiv(15, m_NowDpi.cy, DEFAULT_DPI_Y);

	for ( n = 0 ; n < 8 ; n++ ) {
//		m_ImageList[n].Create(IDB_ATTR1 + n, 40, 1, RGB(255, 255, 255));

		Bitmap.LoadResBitmap(IDB_ATTR1 + n, m_NowDpi.cx, m_NowDpi.cy, RGB(255, 255, 255));
		m_ImageList[n].Create(width, height, ILC_COLOR24 | ILC_MASK, 1, 1);
		m_ImageList[n].Add(&Bitmap, RGB(255, 255, 255));
		Bitmap.DeleteObject();

		list.himl = m_ImageList[n];
		if ( (pWnd = (CButton *)GetDlgItem(IDC_ATTR1 + n)) != NULL )
			pWnd->SetImageList(&list);
	}

	m_TransSlider.SetRange(10, 255);

	m_SliderConstrast.SetRange(0, 100);
	m_SliderConstrast.SetPos(50);

	m_SliderBright.SetRange(0, 100);
	m_SliderBright.SetPos(50);

	m_SliderHuecol.SetRange(50, 250);
	m_SliderHuecol.SetPos(150);

	CRect rect;
	m_ColBox[0].GetWindowRect(rect);
	ScreenToClient(rect);
	m_InvRect.left   = rect.left;
	m_InvRect.top    = rect.top;

	m_ColBox[15].GetWindowRect(rect);
	ScreenToClient(rect);
	m_InvRect.right  = rect.right;
	m_InvRect.bottom = rect.bottom;

#ifdef	USE_DIRECTWRITE
	CClientDC dc(this);
	LOGFONT logfont;
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_EMOJIFONTNAME);

	if ( pCombo != NULL ) {
		ZeroMemory(&logfont, sizeof(LOGFONT)); 
		logfont.lfCharSet = DEFAULT_CHARSET;
		::EnumFontFamiliesEx(dc.m_hDC, &logfont, (FONTENUMPROC)EnumFontFamExComboAddStr, (LPARAM)pCombo, 0);

		if ( !CRLoginApp::IsWinVerCheck(_WIN32_WINNT_WINBLUE, VER_GREATER_EQUAL) )
			pCombo->EnableWindow(FALSE);
	}
#else
	CWnd *pDlgLtem;
	if ( (pDlgLtem = GetDlgItem(IDC_EMOJICOLENABLE)) != NULL )
		pDlgLtem->EnableWindow(FALSE);
	if ( (pDlgLtem = GetDlgItem(IDC_EMOJIFONTNAME)) != NULL )
		pDlgLtem->EnableWindow(FALSE);
	if ( (pDlgLtem = GetDlgItem(IDC_EMOJIIMAGEDIR)) != NULL )
		pDlgLtem->EnableWindow(FALSE);
	if ( (pDlgLtem = GetDlgItem(IDC_EMOJIIMAGESEL)) != NULL )
		pDlgLtem->EnableWindow(FALSE);
#endif

	DoInit();

	return TRUE;
}
BOOL CColParaDlg::OnApply() 
{
	int n;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	SetDarkLight();

	for ( n = 0 ; n < 16 ; n++ )
		m_pSheet->m_pTextRam->m_ColTab[n] = EditColor(n);

	m_FontCol[0] = _tstoi(m_FontColName[0]);
	m_FontCol[1] = _tstoi(m_FontColName[1]);

	m_pSheet->m_pTextRam->m_AttNow.std.fcol = m_FontCol[0];
	m_pSheet->m_pTextRam->m_AttNow.std.bcol = m_FontCol[1];
	m_pSheet->m_pTextRam->m_AttNow.std.attr = 0;
	for ( n = 0 ; n < 24 ; n++ )
		m_pSheet->m_pTextRam->m_AttNow.std.attr |= (m_Attrb[n] ? (1 << n) : 0);

	m_pSheet->m_pTextRam->m_AttSpc = m_pSheet->m_pTextRam->m_AttNow;

	switch(m_WakeUpSleep) {
	case 0: n = 0; break;
	case 1: n = 60; break;
	case 2: n = 180; break;
	case 3: n = 600; break;
	case 4: n = 1200; break;
	case 5: n = 1800; break;
	case 6: n = 2400; break;
	case 7: n = 3000; break;
	case 8: n = 3600; break;
	}
	((CMainFrame *)AfxGetMainWnd())->SetWakeUpSleep(n);

#ifdef	USE_DIRECTWRITE
	m_pSheet->m_pTextRam->SetOption(TO_RLCOLEMOJI, m_EmojiColorEnable);
	pApp->EmojiImageInit(m_EmojiFontName, m_EmojiImageDir);
#endif

	return TRUE;
}

void CColParaDlg::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}
void CColParaDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int n;
	CRect rect;

	for ( n = 0 ; n < 18 ; n++ ) {
		m_ColBox[n].GetWindowRect(rect);
		ScreenToClient(rect);
		if ( rect.PtInRect(point) ) {
			CColorDialog cdl((n < 16 ? m_ColTab[n] : (m_FontCol[n - 16] < 16 ? m_ColTab[m_FontCol[n - 16]] : m_pSheet->m_pTextRam->m_ColTab[m_FontCol[n - 16]])), CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this);
			if ( DpiAwareDoModal(cdl) != IDOK )
				break;
			if ( n < 16 )
				m_ColTab[n] = cdl.GetColor();
			else if ( m_FontCol[n - 16] < 16 )
				m_ColTab[m_FontCol[n - 16]] = cdl.GetColor();
			else
				m_pSheet->m_pTextRam->m_ColTab[m_FontCol[n - 16]] = cdl.GetColor();
			ColSetCheck();
			Invalidate(FALSE);
			UpdateData(FALSE);
			SetModified(TRUE);
			m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_COLTAB);
			break;
		}
	}

	CTreePage::OnLButtonDblClk(nFlags, point);
}
void CColParaDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int n, i;
	CRect rect, tmp;
	CRectTracker tracker;

	for ( n = 0 ; n < 16 ; n++ ) {
		m_ColBox[n].GetWindowRect(rect);
		ScreenToClient(rect);
		if ( rect.PtInRect(point) ) {
			tracker.m_rect  = rect;
			tracker.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;

			if ( tracker.Track(this, point, FALSE, this) ) {
				for ( i = 0 ; i < 2 ; i++ ) {
					m_ColBox[16 + i].GetWindowRect(rect);
					ScreenToClient(&rect);
					if ( !(tracker.m_rect.left   > rect.right  ||
						   tracker.m_rect.right  < rect.left   ||
						   tracker.m_rect.top    > rect.bottom ||
						   tracker.m_rect.bottom < rect.top) ) {
						m_FontCol[i] = n;
						m_FontColName[i].Format(_T("%d"), n);
						UpdateData(FALSE);
						Invalidate(FALSE);
						SetModified(TRUE);
						m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_DEFATT);
						break;
					}
				}
			}
			break;
		}
	}
	CTreePage::OnLButtonDown(nFlags, point);
}

void CColParaDlg::OnReleasedcaptureTransparent(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n = m_TransSlider.GetPos();
	CMainFrame *pWnd = (CMainFrame *)AfxGetMainWnd();

	if ( pWnd != NULL )
		pWnd->SetTransPar(0, n, LWA_ALPHA);

	((CMainFrame *)AfxGetMainWnd())->m_TransParValue = n;
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("LayeredWindow"), n);
	*pResult = 0;
}
void CColParaDlg::OnBnClickedGlassStyle()
{
	UpdateData(TRUE);

	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("GlassStyle"), m_GlassStyle);
	ExDwmEnableWindow(::AfxGetMainWnd()->GetSafeHwnd(), m_GlassStyle);
	((CMainFrame *)::AfxGetMainWnd())->m_bGlassStyle = m_GlassStyle;
}

void CColParaDlg::OnSelendokColset() 
{
	UpdateData(TRUE);

	if ( m_ColSet >= 0 )
		memcpy(m_ColTab, ColSetTab[m_ColSet], sizeof(m_ColTab));

	m_SliderConstrast.SetPos(50);
	m_SliderBright.SetPos(50);
	m_SliderHuecol.SetPos(150);
	SetDarkLight();

	Invalidate(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_COLTAB);
}
void CColParaDlg::OnUpdateCheck(UINT nId)
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_DEFATT);
}
void CColParaDlg::OnEnChangeColor()
{
	UpdateData(TRUE);

	if ( (m_FontCol[0] = _tstoi(m_FontColName[0])) < 0 )
		m_FontCol[0] = 0;
	else if ( m_FontCol[0] > 255 )
		m_FontCol[0] = 255;

	if ( (m_FontCol[1] = _tstoi(m_FontColName[1])) < 0 )
		m_FontCol[1] = 0;
	else if ( m_FontCol[1] > 255 )
		m_FontCol[1] = 255;

	Invalidate(FALSE);
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_DEFATT);
}
void CColParaDlg::OnNMReleasedcaptureContrast(NMHDR *pNMHDR, LRESULT *pResult)
{
	SetDarkLight();
	Invalidate(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_COLTAB;

	*pResult = 0;
}
void CColParaDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if ( pScrollBar != NULL ) {
		switch(pScrollBar->GetDlgCtrlID()) {
		case IDC_SLIDER_BRIGHT:
		case IDC_SLIDER_CONTRAST:
		case IDC_SLIDER_HUECOL:
			SetDarkLight();
			InvalidateRect(m_InvRect, FALSE);
			break;
		}
	}
	CTreePage::OnVScroll(nSBCode, nPos, pScrollBar);
}
void CColParaDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	int n = nIDCtl - IDC_BOX0;
	CDC dc;
	CRect rect;

	if ( nIDCtl >= IDC_BOX0 && nIDCtl <= IDC_BOX15 ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_ColBox[n].GetClientRect(rect);
		dc.FillSolidRect(rect, EditColor(n));
		dc.Detach();
	} else if ( nIDCtl == IDC_BOXTEXT || nIDCtl == IDC_BOXBACK ) {
		n = (nIDCtl == IDC_BOXTEXT ? 0 : 1);
		dc.Attach(lpDrawItemStruct->hDC);
		m_ColBox[n + 16].GetClientRect(rect);
		dc.FillSolidRect(rect, m_FontCol[n] > 16 ? m_pSheet->m_pTextRam->m_ColTab[m_FontCol[n]] : EditColor(m_FontCol[n]));
		dc.Detach();
	} else
		CTreePage::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CColParaDlg::OnBnClickedColedit()
{
	int n;
	CColEditDlg dlg;

	UpdateData(TRUE);
	SetDarkLight();

	for ( n = 0 ; n < 16 ; n++ )
		dlg.m_ColTab[n] = EditColor(n);

	if ( dlg.DoModal() != IDOK )
		return;

	m_SliderConstrast.SetPos(50);
	m_SliderBright.SetPos(50);
	m_SliderHuecol.SetPos(150);
	SetDarkLight();

	memcpy(m_ColTab, dlg.m_ColTab, sizeof(m_ColTab));

	ColSetCheck();
	UpdateData(FALSE);
	Invalidate(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_COLTAB;
}

void CColParaDlg::OnBnClickedImageSel()
{
	CDirDialog dlg;

	UpdateData(TRUE);

	dlg.m_strInitDir = _T("");
	dlg.m_strSelDir  = m_EmojiImageDir.IsEmpty() ? ((CRLoginApp *)AfxGetApp())->m_EmojiImageDir : m_EmojiImageDir;
	dlg.m_strTitle.LoadString(IDS_EMOJIMAGEMSG);
	dlg.m_strWindowTitle = _T("Emoji Image Folder");

	if ( !dlg.DoBrowse(this) )
		return;

	dlg.m_strPath.TrimRight(_T(" \t\\"));
	m_EmojiImageDir = dlg.m_strPath;

	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CColParaDlg::OnUpdateTextRam()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
