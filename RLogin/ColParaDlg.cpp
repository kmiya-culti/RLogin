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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColParaDlg プロパティ ページ

IMPLEMENT_DYNCREATE(CColParaDlg, CPropertyPage)

CColParaDlg::CColParaDlg() : CTreePropertyPage(CColParaDlg::IDD)
{
	m_ColSet = -1;
	m_BitMapFile = _T("");
	for ( int n = 0 ; n < 24 ; n++ )
		m_Attrb[n] = 0;
	m_WakeUpSleep = 0;
	m_FontCol[0] = 0;
	m_FontCol[1] = 0;
	m_FontColName[0] = _T("0");
	m_FontColName[1] = _T("0");
}
CColParaDlg::~CColParaDlg()
{
}

void CColParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TRANSPARENT, m_TransSlider);
	DDX_CBIndex(pDX, IDC_COLSET, m_ColSet);
	DDX_Text(pDX, IDC_BACKFILE, m_BitMapFile);
	for ( int n = 0 ; n < 16 ; n++ )
		DDX_Control(pDX, IDC_BOX0 + n, m_ColBox[n]);
	DDX_Control(pDX, IDC_BOXTEXT, m_ColBox[16]);
	DDX_Control(pDX, IDC_BOXBACK, m_ColBox[17]);
	for ( int n = 0 ; n < 8 ; n++ )
		DDX_Check(pDX, IDC_ATTR1 + n, m_Attrb[n]);
	DDX_CBIndex(pDX, IDC_COMBO2, m_WakeUpSleep);
	DDX_Text(pDX, IDC_TEXTCOL, m_FontColName[0]);
	DDX_Text(pDX, IDC_BACKCOL, m_FontColName[1]);
}

BEGIN_MESSAGE_MAP(CColParaDlg, CPropertyPage)
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_TRANSPARENT, OnReleasedcaptureTransparent)
	ON_CBN_SELENDOK(IDC_COLSET, OnSelendokColset)
	ON_BN_CLICKED(IDC_BACKSEL, OnBitMapFileSel)
	ON_EN_CHANGE(IDC_BACKFILE, OnUpdateEdit)
	ON_EN_CHANGE(IDC_TEXTCOL, &CColParaDlg::OnEnChangeColor)
	ON_EN_CHANGE(IDC_BACKCOL, &CColParaDlg::OnEnChangeColor)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColParaDlg メッセージ ハンドラ

static	const COLORREF	ColSetTab[][16] = {
	//{	RGB(  0,   0,   0),	RGB(196,   0,   0),	RGB(  0, 196,   0),	RGB(196, 196,   0),
	//	RGB(  0,   0, 196),	RGB(196,   0, 196),	RGB(  0, 196, 196),	RGB(196, 196, 196),
	//	RGB(128, 128, 128),	RGB(255,   0,   0),	RGB(  0, 255,   0),	RGB(255, 255,   0),
	//	RGB(  0,   0, 255),	RGB(255,   0, 255),	RGB(  0, 255, 255),	RGB(255, 255, 255)	},	// Black

	{	RGB(  0,   0,   0),	RGB(196,  64,  64),	RGB( 64, 196,  64),	RGB(196, 196,  64),
		RGB( 64,  64, 196),	RGB(196,  64, 196),	RGB( 64, 196, 196),	RGB(196, 196, 196),
		RGB(128, 128, 128),	RGB(255,  64,  64),	RGB( 64, 255,  64),	RGB(255, 255,  64),
		RGB( 64,  64, 255),	RGB(255,  64, 255),	RGB( 64, 255, 255),	RGB(255, 255, 255)	},	// Black

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
		RGB(  0,   64,128),	RGB(196,   0, 196),	RGB(  0, 128, 128),	RGB(  0,   0,   0),
		RGB(224, 224, 224),	RGB(255,  32,  32),	RGB( 32, 224,   0),	RGB(255, 224,   0),
		RGB( 32,  32, 224),	RGB(255,   0, 255),	RGB( 32, 196, 224),	RGB( 96,  96,  96)	},	// White

	{	RGB(  0,  43,  54),	RGB(203,  75,  22),	RGB( 88, 110, 117),	RGB(101, 123, 131),
		RGB(131, 148, 150),	RGB(108, 113, 196),	RGB(147, 161, 161),	RGB(238, 232, 213),
		RGB(  7,  54,  66),	RGB(220,  50,  47),	RGB(133, 153,   0),	RGB(181, 137,   0),
		RGB( 38, 139, 210),	RGB(211,  54, 130),	RGB( 42, 161, 152),	RGB(253, 246, 227)	},	// Solarized
};

void CColParaDlg::DoInit()
{
	int n;

	for ( n = 0 ; n < 16 ; n++ )
		m_ColTab[n] = m_pSheet->m_pTextRam->m_DefColTab[n];

	m_FontCol[0] = m_pSheet->m_pTextRam->m_DefAtt.fc;
	m_FontCol[1] = m_pSheet->m_pTextRam->m_DefAtt.bc;

	m_FontColName[0].Format(_T("%d"), m_FontCol[0]);
	m_FontColName[1].Format(_T("%d"), m_FontCol[1]);

	for ( n = 0 ; n < 24 ; n++ )
		m_Attrb[n] = (m_pSheet->m_pTextRam->m_DefAtt.at & (1 << n) ? TRUE : FALSE);

	m_BitMapFile = m_pSheet->m_pTextRam->m_BitMapFile;

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

	UpdateData(FALSE);
}
BOOL CColParaDlg::OnInitDialog() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();

	int n;
	CButton *pWnd;
	BUTTON_IMAGELIST list;

	memset(&list, 0, sizeof(list));
	list.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;

	for ( n = 0 ; n < 8 ; n++ ) {
		m_ImageList[n].Create(IDB_ATTR1 + n, 40, 1, RGB(255, 255, 255));
		list.himl = m_ImageList[n];
		if ( (pWnd = (CButton *)GetDlgItem(IDC_ATTR1 + n)) != NULL )
			pWnd->SetImageList(&list);
	}

	m_TransSlider.SetRange(10, 255);

	DoInit();

	return TRUE;
}
BOOL CColParaDlg::OnApply() 
{
	int n;

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	UpdateData(TRUE);

	for ( n = 0 ; n < 16 ; n++ ) {
		m_pSheet->m_pTextRam->m_DefColTab[n] = m_ColTab[n];
		m_pSheet->m_pTextRam->m_ColTab[n] = m_ColTab[n];
	}

	m_FontCol[0] = _tstoi(m_FontColName[0]);
	m_FontCol[1] = _tstoi(m_FontColName[1]);

	m_pSheet->m_pTextRam->m_DefAtt.fc = m_FontCol[0];
	m_pSheet->m_pTextRam->m_DefAtt.bc = m_FontCol[1];
	m_pSheet->m_pTextRam->m_DefAtt.at = 0;
	for ( n = 0 ; n < 24 ; n++ )
		m_pSheet->m_pTextRam->m_DefAtt.at |= (m_Attrb[n] ? (1 << n) : 0);

	m_pSheet->m_pTextRam->m_AttNow = m_pSheet->m_pTextRam->m_DefAtt;
	m_pSheet->m_pTextRam->m_AttSpc = m_pSheet->m_pTextRam->m_AttNow;

	m_pSheet->m_pTextRam->m_BitMapFile  = m_BitMapFile;

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

	return TRUE;
}

void CColParaDlg::OnReset() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	DoInit();
	SetModified(FALSE);
}

void CColParaDlg::OnPaint() 
{
	int n;
	CRect rect;
	CPaintDC dc(this); // 描画用のデバイス コンテキスト

	for ( n = 0 ; n < 16 ; n++ ) {
		m_ColBox[n].GetWindowRect(rect);
		ScreenToClient(rect);
		dc.FillSolidRect(rect, m_ColTab[n]);
	}
	for ( n = 0 ; n < 2 ; n++ ) {
		m_ColBox[n + 16].GetWindowRect(rect);
		ScreenToClient(rect);
		dc.FillSolidRect(rect, m_FontCol[n] > 16 ? m_pSheet->m_pTextRam->m_ColTab[m_FontCol[n]] : m_ColTab[m_FontCol[n]]);
	}
}
void CColParaDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int n;
	CRect rect;

	for ( n = 0 ; n < 18 ; n++ ) {
		m_ColBox[n].GetWindowRect(rect);
		ScreenToClient(rect);
		if ( rect.PtInRect(point) ) {
			CColorDialog cdl((n < 16 ? m_ColTab[n] : (m_FontCol[n - 16] < 16 ? m_ColTab[m_FontCol[n - 16]] : m_pSheet->m_pTextRam->m_ColTab[m_FontCol[n - 16]])), CC_ANYCOLOR, this);
			if ( cdl.DoModal() != IDOK )
				break;
			if ( n < 16 ) {
				m_ColTab[n] = cdl.GetColor();
				m_ColSet = (-1);
			} else if ( m_FontCol[n - 16] < 16 ) {
				m_ColTab[m_FontCol[n - 16]] = cdl.GetColor();
				m_ColSet = (-1);
			} else
				m_pSheet->m_pTextRam->m_ColTab[m_FontCol[n - 16]] = cdl.GetColor();
			Invalidate(FALSE);
			UpdateData(FALSE);
			SetModified(TRUE);
			m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
			break;
		}
	}

	CPropertyPage::OnLButtonDblClk(nFlags, point);
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
						m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
						break;
					}
				}
			}
			break;
		}
	}
	CPropertyPage::OnLButtonDown(nFlags, point);
}

void CColParaDlg::OnBitMapFileSel() 
{
	UpdateData(TRUE);

	CFileDialog dlg(TRUE, _T("jpg"), m_BitMapFile, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGIMAGE), this);

	if ( dlg.DoModal() != IDOK )
		return;

	m_BitMapFile = dlg.GetPathName();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
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
void CColParaDlg::OnSelendokColset() 
{
	UpdateData(TRUE);

	if ( m_ColSet >= 0 )
		memcpy(m_ColTab, ColSetTab[m_ColSet], sizeof(m_ColTab));
	Invalidate(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CColParaDlg::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
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
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
