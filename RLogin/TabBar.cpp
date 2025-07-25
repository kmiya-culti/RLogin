// TabBar.cpp: CTabBar クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "DialogExt.h"
#include "TabBar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define NEWCLICK_SIZE		18
#define	MINTAB_SIZE			48
#define	DEFTAB_COUNT		4

#define	TABTOP_SIZE			2
#define	TABBTM_SIZE			3
#define	TABLINE_SIZE		1

#define	TABSTYLE_TOP		0
#define	TABSTYLE_BOTTOM		1
#define	TABSTYLE_LEFT		2
#define	TABSTYLE_RIGHT		3

//////////////////////////////////////////////////////////////////////
// CTabCtrlBar

IMPLEMENT_DYNAMIC(CTabCtrlBar, CTabCtrlExt)

CTabCtrlBar::CTabCtrlBar()
{
	m_ColTab[TABCOL_FACE]    = APPCOL_BARFACE;
	m_ColTab[TABCOL_TEXT]    = COLOR_GRAYTEXT;
	m_ColTab[TABCOL_BACK]    = APPCOL_BARBACK;
	m_ColTab[TABCOL_SELFACE] = APPCOL_BARHIGH;
	m_ColTab[TABCOL_SELTEXT] = APPCOL_BARTEXT;
	m_ColTab[TABCOL_SELBACK] = APPCOL_BARHIGH;
	m_ColTab[TABCOL_BODER]   = APPCOL_BARBODER;
}
void CTabCtrlBar::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	int nTabIndex = (int)(lpDrawItemStruct->itemID);
	BOOL bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0 ? TRUE : FALSE;
	TC_ITEM tci;
	CChildFrame *pWnd;
	CTabBar *pParent;
	CRLoginDoc *pDoc;

	ZeroMemory(&tci, sizeof(tci));
	tci.mask = TCIF_PARAM;

	if ( bSelected && nTabIndex >= 0 && GetItem(nTabIndex, &tci) && (pParent = (CTabBar *)GetParent()) != NULL && (HWND)tci.lParam != NULL && (pWnd = (CChildFrame *)FromHandle((HWND)tci.lParam)) != NULL && (pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument())) != NULL ) {
		if ( pParent->m_bDarkMode ) {
				m_ColTab[TABCOL_SELFACE] = pDoc->m_TextRam.m_TabTextColor | TABCOL_COLREF;
				m_ColTab[TABCOL_SELTEXT] = pDoc->m_TextRam.m_TabBackColor | TABCOL_COLREF;
			} else {
				m_ColTab[TABCOL_SELFACE] = pDoc->m_TextRam.m_TabBackColor | TABCOL_COLREF;
				m_ColTab[TABCOL_SELTEXT] = pDoc->m_TextRam.m_TabTextColor | TABCOL_COLREF;
			}
			m_bGradient = (GetColor(TABCOL_SELFACE) != GetColor(TABCOL_SELBACK) && pDoc->m_TextRam.IsOptEnable(TO_RLTABGRAD) ? TRUE : FALSE);
	} else {
		m_ColTab[TABCOL_SELFACE] = APPCOL_BARHIGH;
		m_ColTab[TABCOL_SELTEXT] = APPCOL_BARTEXT;
		m_bGradient = FALSE;
	}

	CTabCtrlExt::DrawItem(lpDrawItemStruct);
}

BEGIN_MESSAGE_MAP(CTabCtrlBar, CTabCtrlExt)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CTabBar

CTabBar::CTabBar()
{
	m_GhostReq = m_GhostItem = (-1);
	m_pGhostView = NULL;
	m_bNumber = FALSE;
	m_ImageCount = 0;
	m_ImageSize = 16;
	m_FontSize = 0;
	m_SetCurTimer = 0;
	m_GhostWndTimer = 0;
	m_TabHeight = 16;
	m_BoderSize = 2;
	m_MinTabSize = MINTAB_SIZE;
	m_TabLines = 1;
	m_bMultiLine = FALSE;
	m_bTrackMode = FALSE;
}

CTabBar::~CTabBar()
{
}

IMPLEMENT_DYNAMIC(CTabBar, CControlBarEx)

BEGIN_MESSAGE_MAP(CTabBar, CControlBarEx)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
	ON_NOTIFY(TCN_SELCHANGE, IDC_MDI_TAB_CTRL, OnSelchange)
	ON_NOTIFY(TTN_GETDISPINFO, 0, OnGetDispInfo)
	ON_WM_RBUTTONDOWN()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////

BOOL CTabBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	m_dwStyle = (dwStyle & CBRS_ALL);

	dwStyle &= ~CBRS_ALL;
	dwStyle |= CCS_NOPARENTALIGN|CCS_NOMOVEY|CCS_NODIVIDER|CCS_NORESIZE;
//	if (pParentWnd->GetStyle() & WS_THICKFRAME) dwStyle |= SBARS_SIZEGRIP;

	CRect rect; rect.SetRectEmpty();
	return CWnd::Create(STATUSCLASSNAME, NULL, dwStyle, rect, pParentWnd, nID);
}

CSize CTabBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CSize size;
	CWnd *pMain;
	CRect MainRect(0, 0, 32767, 32767);
	CRect InSideRect, TabRect;
	int FontSize = 12;
	int WinBdSize = 1;
	int rgBorders[3];	// バーの境界線の幅を取得 0=横, 1=縦, 2=セパレーター

	if ( (pMain = ::AfxGetMainWnd()) != NULL )
		pMain->GetClientRect(MainRect);

	if ( m_TabCtrl.GetSafeHwnd() != NULL ) {
		CFont *pFont = m_TabCtrl.GetFont();
		if ( pFont != NULL ) {
			LOGFONT LogFont;
			pFont->GetLogFont(&LogFont);
			if ( (FontSize = LogFont.lfHeight) < 0 )
				FontSize = 0 - FontSize;
			FontSize = FontSize * 12 / 10;
		}
	}
	
	DefWindowProc(SB_GETBORDERS, 0, (LPARAM)&rgBorders);
	m_BoderSize = rgBorders[1]; 
	WinBdSize = ::GetSystemMetrics(SM_CYBORDER);

	InSideRect.SetRect(0, 0, 100, 100);
	CalcInsideRect(InSideRect, bHorz);

	if ( m_bMultiLine )
		m_TabHeight = WinBdSize + m_BoderSize * 2 + FontSize + m_BoderSize * 2 + WinBdSize;
	else
		m_TabHeight = WinBdSize + m_BoderSize + FontSize + m_BoderSize + WinBdSize;

	if ( bHorz ) {
		size.cx = WinBdSize + MainRect.Width() + WinBdSize;
		size.cy = WinBdSize + (m_TabHeight + m_BoderSize + WinBdSize) * m_TabLines + (100 - InSideRect.Height());
	} else {
		size.cx = WinBdSize + (m_TabHeight + m_BoderSize + WinBdSize) * m_TabLines + (100 - InSideRect.Width());
		size.cy = MainRect.Height();
	}

	if ( IsVisible() && m_TabCtrl.m_hWnd != NULL && bHorz ) {
		InSideRect.SetRect(0, 0, size.cx, size.cy);
		CalcInsideRect(InSideRect, bHorz);
		m_TabCtrl.GetWindowRect(TabRect);

		if ( TabRect != InSideRect ) {
			m_TabCtrl.SetWindowPos(&wndTop, InSideRect.left, InSideRect.top, InSideRect.Width(), InSideRect.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
			ReSize(FALSE);
		}
	}

	return size;
}

BOOL CTabBar::PreTranslateMessage(MSG* pMsg) 
{
	if ( pMsg->hwnd == m_TabCtrl.m_hWnd && (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_RBUTTONDOWN) ) {
		// WM_LBUTTONDOWNをTabCtrlからTabBarに変更
		CPoint point(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
		::ClientToScreen(pMsg->hwnd, &point);
		ScreenToClient(&point);
		pMsg->hwnd = m_hWnd;
		pMsg->lParam = MAKELPARAM(point.x, point.y);
	}

	return CControlBarEx::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////
// CTabBar メッセージ ハンドラ

int CTabBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CControlBarEx::OnCreate(lpCreateStruct) == (-1) )
		return (-1);
	
	CRect rect; rect.SetRectEmpty();
	DWORD addStyle = (AfxGetApp()->GetProfileInt(_T("TabBar"), _T("ToolTip"), 0) == 0 ? TCS_TOOLTIPS : 0);

	m_bMultiLine = (AfxGetApp()->GetProfileInt(_T("TabBar"), _T("MultiLine"), 0) != 0 ? TRUE : FALSE);

	if ( m_bMultiLine )
		addStyle |= (TCS_MULTILINE | TCS_BUTTONS);

	addStyle |= TCS_OWNERDRAWFIXED;

	if ( !m_TabCtrl.Create(WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_FIXEDWIDTH | TCS_FORCELABELLEFT | addStyle, rect, this, IDC_MDI_TAB_CTRL) ) {
		TRACE0("Unable to create tab control bar\n");
		return (-1);
	}

	m_FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	m_FontSize = MulDiv(::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9), SCREEN_DPI_Y, SYSTEM_DPI_Y);

	if ( m_FontName.IsEmpty() || !m_TabFont.CreatePointFont(m_FontSize * 10, m_FontName) ) {
		m_FontName.Empty();
		CFont *font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
		m_TabCtrl.SetFont(font);
		SetFont(font);

	} else {
		m_TabCtrl.SetFont(&m_TabFont);
		SetFont(&m_TabFont);
	}

	m_TabCtrl.SetMinTabWidth(16);
	m_TabCtrl.m_bCloseBtn = TRUE;
	
	CToolTipCtrl *pToolTip = m_TabCtrl.GetToolTips();

	if ( pToolTip != NULL )
		pToolTip->SetMaxTipWidth(256);

	m_ImageSize = ICONIMG_SIZE;
	m_ImageList.Create(ICONIMG_SIZE, ICONIMG_SIZE, ILC_COLOR24 | ILC_MASK, 4, 4);

	// CheckBox ImageList idx, 0=OFF,1=ON,2=NONE,3=CLOSE,4=OPENCLOSE
	CBitmapEx bitmap;
	for ( int n = 0 ; n < 5 ; n++ ) {
		bitmap.LoadResBitmap(IDB_CHECKBOX1 + n, DEFAULT_DPI_X * ICONIMG_SIZE / 48, DEFAULT_DPI_X * ICONIMG_SIZE / 48, RGB(255, 0, 0));
		m_ImageList.Add(&bitmap, RGB(255, 0, 0));
		bitmap.DeleteObject();
	}

	SetBorders(2, 2, 2, 2);

	return 0;
}

void CTabBar::OnSize(UINT nType, int cx, int cy) 
{
	CControlBarEx::OnSize(nType, cx, cy);

	if ( m_TabCtrl.m_hWnd == NULL )
		return;

	CRect InSideRect, TabRect;
	InSideRect.SetRect(0, 0, cx, cy);
	CalcInsideRect(InSideRect, (GetBarStyle() & (CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM)) != 0 ? TRUE : FALSE);
	m_TabCtrl.GetWindowRect(TabRect);

	if ( TabRect != InSideRect ) {
		m_TabCtrl.SetWindowPos(&wndTop, InSideRect.left, InSideRect.top, InSideRect.Width(), InSideRect.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
		ReSize(FALSE);
	}
}

void CTabBar::FontSizeCheck()
{
	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = MulDiv(::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9), SCREEN_DPI_Y, SYSTEM_DPI_Y);

	if ( m_FontName.Compare(FontName) != 0 || m_FontSize != FontSize ) {
		m_FontName = FontName;
		m_FontSize = FontSize;

		if ( m_TabFont.GetSafeHandle() != NULL )
			m_TabFont.DeleteObject();

		if ( !m_TabFont.CreatePointFont(m_FontSize * 10, m_FontName) ) {
			CFont *font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
			m_TabCtrl.SetFont(font);
			SetFont(font);
		} else {
			m_TabCtrl.SetFont(&m_TabFont);
			SetFont(&m_TabFont);
		}
	}

	if ( m_ImageList.GetImageCount() > 0 && m_ImageSize != ICONIMG_SIZE ) {
		m_ImageList.DeleteImageList();

		m_ImageSize = ICONIMG_SIZE;
		m_ImageList.Create(ICONIMG_SIZE, ICONIMG_SIZE, ILC_COLOR24 | ILC_MASK, 4, 4);

		CBitmapEx bitmap;
		for ( int n = 0 ; n < 5 ; n++ ) {
			bitmap.LoadResBitmap(IDB_CHECKBOX1 + n, DEFAULT_DPI_X * ICONIMG_SIZE / 48, DEFAULT_DPI_Y * ICONIMG_SIZE / 48, RGB(255, 0, 0));
			m_ImageList.Add(&bitmap, RGB(255, 0, 0));
			bitmap.DeleteObject();
		}

		CPtrArray list;
		m_ImageFile.GetIndexList(list);

		for ( int n = 0 ; n < (int)list.GetSize() ; n++ ) {
			if ( list[n] == NULL )
				continue;

			CClientDC dc(&m_TabCtrl);
			CBmpFile image;
			CBitmap *pBitmap;
			COLORREF bc = RGB(255, 255, 255);

			if ( image.LoadFile((LPCTSTR)list[n]) && (pBitmap = image.GetBitmap(&dc, ICONIMG_SIZE, ICONIMG_SIZE, bc)) != NULL )
				m_ImageList.Add(pBitmap, bc);
		}

		m_TabCtrl.SetImageList(&m_ImageList);
		m_TabCtrl.Invalidate(FALSE);
	}
}

void CTabBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	int n, idx, sel = (-1);
	TC_ITEM tci, ntc;
	CString title;
	TCHAR tmp[MAX_PATH + 2] = { _T('\0') };
	CChildFrame *pWnd;
	CMainFrame *pMainframe = ((CMainFrame *)AfxGetMainWnd());
	CMDIChildWnd* pActive = (pMainframe == NULL ? NULL : pMainframe->MDIGetActive(NULL));
	CRLoginDoc *pActiveDoc = (pActive == NULL ? NULL : (CRLoginDoc *)pActive->GetActiveDocument());
	CRLoginDoc *pDoc;
	CRect rect;

	if ( m_bTrackMode )
		return;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
		tci.pszText = tmp;
		tci.cchTextMax = MAX_PATH;

		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;

		ntc.mask = 0;

		if ( (HWND)tci.lParam != NULL && (pWnd = (CChildFrame *)FromHandle((HWND)tci.lParam)) != NULL ) {
			pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument());
			pWnd->GetWindowText(title);
		} else {
			pWnd = NULL;
			pDoc = NULL;
			title.Empty();
		}

		if ( title.GetLength() >= MAX_PATH )
			title = title.Left(MAX_PATH -1);

		if ( title.Compare(tmp) != 0 ) {
			ntc.mask |= TCIF_TEXT;
			ntc.pszText = tmp;
			_tcsncpy(tmp, title, MAX_PATH);
		}

		if ( pMainframe != NULL && pMainframe->m_bBroadCast ) {
			idx = (pDoc != NULL && !pDoc->m_bCastLock ? 1 : 0);

			if ( pDoc == NULL || pActiveDoc == NULL || pMainframe == NULL )
				idx = 2;
			else if ( pDoc->m_TextRam.IsOptEnable(TO_RLNTBCRECV) )
				idx = 2;
			else if ( ((CRLoginApp *)::AfxGetApp())->m_bLookCast && !pMainframe->IsTopLevelDoc(pDoc) )
				idx = 2;
			else if ( !pActiveDoc->m_TextRam.m_GroupCast.IsEmpty() ) {
				if ( pActiveDoc->m_TextRam.m_GroupCast.Compare(pDoc->m_TextRam.m_GroupCast) != 0 )
					idx = 2;
			} else {
				if ( pDoc->m_TextRam.IsOptEnable(TO_RLGROUPCAST) )
					idx = 2;
			}
		} else if ( pDoc != NULL && !pDoc->m_ServerEntry.m_IconName.IsEmpty() )
			idx = GetImageIndex(pDoc->m_ServerEntry.m_IconName);
		else
			idx = (-1);

		if ( idx != tci.iImage ) {
			ntc.mask |= TCIF_IMAGE;
			ntc.iImage = idx;
		}

		if ( m_ImageList.GetImageCount() != m_ImageCount ) {
			m_ImageCount = m_ImageList.GetImageCount();
			m_TabCtrl.SetImageList(m_ImageCount > 0 ? &m_ImageList : NULL);
		}

		if ( ntc.mask != 0 )
			m_TabCtrl.SetItem(n, &ntc);

		if ( pActive != NULL && pWnd != NULL && pActive->m_hWnd == pWnd->m_hWnd )
			sel = n;
	}

	if ( sel >= 0 )
		m_TabCtrl.SetCurSel(sel);

	ReSize(TRUE);
}

BOOL CTabBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	int n;
	CPoint point;
	TCHITTESTINFO htinfo;

	if ( AfxGetApp()->GetProfileInt(_T("TabBar"), _T("GhostWnd"), 0) )
		return CControlBarEx::OnSetCursor(pWnd, nHitTest, message);

	GetCursorPos(&point);
	m_TabCtrl.ScreenToClient(&point);
	htinfo.pt = point;
	n = m_TabCtrl.HitTest(&htinfo);
	
	if ( n >= 0 && n == m_TabCtrl.GetCurSel() )
		n = (-1);

	if ( m_GhostItem >= 0 && m_GhostItem != n )
		SetGhostWnd(FALSE);

	if ( m_GhostReq != n ) {
		if ( m_GhostReq >= 0 && m_SetCurTimer != 0 ) {
			KillTimer(m_SetCurTimer);
			m_SetCurTimer = 0;
		}
		m_GhostReq = (-1);

		if ( n >= 0 ) {
			m_SetCurTimer = SetTimer(TBTMID_SETCURSOR, 2000, NULL);
			m_GhostReq = n;
		}
	}

	if ( pWnd == NULL )
		return TRUE;

	return CControlBarEx::OnSetCursor(pWnd, nHitTest, message);
}

void CTabBar::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent) {
	case TBTMID_SETCURSOR:
		KillTimer(nIDEvent);
		if ( m_GhostReq >= 0 && (GetStyle() & WS_VISIBLE) != 0 )
			SetGhostWnd(TRUE);
		break;

	case TBTMID_GHOSTWMD:
		OnSetCursor(NULL, 0, 0);
		if ( m_GhostItem >= 0 && (GetStyle() & WS_VISIBLE) == 0 )
			SetGhostWnd(FALSE);
		break;
	}
	CControlBarEx::OnTimer(nIDEvent);
}

void CTabBar::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TC_ITEM tci;
	int idx = m_TabCtrl.GetCurSel();

	tci.mask = TCIF_PARAM;
	if ( m_TabCtrl.GetItem(idx, &tci) ) {
		CWnd *pFrame = FromHandle((HWND) tci.lParam);
		ASSERT(pFrame != NULL);
		ASSERT_KINDOF(CMDIChildWnd, pFrame);
		((CMDIFrameWnd *)AfxGetMainWnd())->MDIActivate(pFrame);
	}

	*pResult = 0;
}

void CTabBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CRect rect;

	m_TabCtrl.GetWindowRect(rect);
	ScreenToClient(rect);

	if ( rect.PtInRect(point) && HitPoint(point) == (-1) )
		AfxGetMainWnd()->PostMessage(WM_COMMAND, (WPARAM)ID_FILE_NEW);
	else
		CControlBarEx::OnLButtonDblClk(nFlags, point);
}

void CTabBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int hit = (-3);
	int hitBtn = (-1);
	int TypeCol = 0;
	BOOL bMove = FALSE;
	BOOL bOtherMove;
	BOOL bCmdLine;
	CPoint capos;
	CSize size(4, 4);
	CRect rect, rectFirst, rectLast;
	int idx = (-1);
	int offset;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CWnd *pDeskTop = CWnd::GetDesktopWindow();
	HWND hTabWnd;
	CChildFrame *pChild;
	CRLoginDoc *pDoc;
	TC_ITEM tci;
	TCHAR Text[MAX_PATH + 2];
	CTrackWnd track;
	CString title;
	MSG msg;
	clock_t stc = clock() - (CLOCKS_PER_SEC * 2);
	int count = m_TabCtrl.GetItemCount();
	BOOL bIdle = FALSE;

	ASSERT(pApp != NULL && pMain != NULL);

	if ( (idx = HitPoint(point)) < 0 || !m_TabCtrl.GetItemRect(idx, rect) ) {
		CControlBar::OnLButtonDown(nFlags, point);
		return;
	}

	capos = point;
	ClientToScreen(&capos);
	m_TabCtrl.ScreenToClient(&capos);
	hitBtn = m_TabCtrl.HitPoint(idx, capos);

	m_TabCtrl.ClientToScreen(rect);
	ScreenToClient(rect);

	if ( (pChild = (CChildFrame *)GetAt(idx)) == NULL ||
		 (pDoc = (CRLoginDoc*)pChild->GetActiveDocument()) == NULL ||
		 (hTabWnd = pChild->GetSafeHwnd()) == NULL )
		return;

	if ( pMain->m_bBroadCast && hitBtn == 1 ) {
		pDoc->m_bCastLock = (pDoc->m_bCastLock ? FALSE : TRUE);
		return;
	}

	if ( idx != m_TabCtrl.GetCurSel() && pMain != NULL ) {
		pMain->MDIActivate(pChild);
		m_TabCtrl.SetCurSel(idx);
		m_TabCtrl.GetItemRect(idx, rect);
		m_TabCtrl.ClientToScreen(rect);
		ScreenToClient(rect);
	}

	if ( m_TabCtrl.m_bCloseBtn && hitBtn == 2 ) {
		pMain->PostMessage(WM_COMMAND, (WPARAM)ID_FILE_CLOSE);
		return;
	}

	switch(pDoc->m_ServerEntry.m_DocType) {
	case DOCTYPE_NONE:
		bOtherMove = FALSE;
		bCmdLine   = FALSE;
		break;
	case DOCTYPE_ENTRYFILE:
	case DOCTYPE_MULTIFILE:
		bOtherMove = TRUE;
		bCmdLine   = FALSE;
		break;
	case DOCTYPE_REGISTORY:
	case DOCTYPE_SESSION:
		bOtherMove = TRUE;
		bCmdLine   = TRUE;
		break;
	}

	if ( GetCapture() != NULL )
		return;

	SetCapture();

	offset = rect.top - point.y - 1;

	ClientToScreen(rect);
	pDeskTop->ScreenToClient(rect);
	rectFirst = rectLast = rect;
	m_bTrackMode = TRUE;

	GetTitle(idx, title);
	track.Create(NULL, title, WS_TILED | WS_CHILD, rect, pDeskTop, (-1));
	track.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	for ( ; ; ) {
		for ( int idlecount = 0 ; bIdle || !::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE) ; idlecount++ ) {
			bIdle = FALSE;
			if ( !((CRLoginApp *)AfxGetApp())->OnIdle(idlecount) )
				break;
		}

		// タブバーの操作をチェック
		if (  !::GetMessage(&msg, NULL, 0, 0) || m_TabCtrl.GetItemCount() != count ) {
			track.DestroyWindow();
			ReleaseCapture();
			m_bTrackMode = FALSE;
			return;
		}

		switch (msg.message) {
		case WM_MOUSEMOVE:
			if ( ::PeekMessage(&msg, NULL, NULL, NULL, PM_QS_PAINT | PM_NOREMOVE) )
				break;
		case WM_LBUTTONUP:
			capos.x = GET_X_LPARAM(msg.lParam);
			capos.y = GET_Y_LPARAM(msg.lParam);

			hit = HitPoint(capos);

			rect.left   = rectFirst.left   + (capos.x - point.x);
			rect.right  = rectFirst.right  + (capos.x - point.x);
			rect.top    = rectFirst.top    + (capos.y - point.y);
			rect.bottom = rectFirst.bottom + (capos.y - point.y);

			if ( hit == (-2) ) {
				if ( bOtherMove ) {
					CPoint wnpos(capos.x, capos.y + offset);
					ClientToScreen(&wnpos);

					if ( CRLoginApp::GetRLoginFromPoint(wnpos) != NULL )
						TypeCol = (bCmdLine ? 4 : 3);
					else
						TypeCol = (bCmdLine ? 2 : 1);
				} else
					TypeCol = 1;

			} else if ( hit == (-1) ) {
				if ( !m_bNumber ) {
					SetTabTitle(TRUE);
					GetTitle(idx, title);
					track.SetText(title);
				}
				if ( (clock() - stc) > (CLOCKS_PER_SEC * 2) ) {
					pMain->SendMessage(WM_COMMAND, IDM_DISPWINIDX);
					stc = clock();
				}
				CPoint tmp = capos;
				ClientToScreen(&tmp);
				pMain->ScreenToClient(&tmp);
				TypeCol = (pMain->GetWindowPanePoint(tmp) != NULL ? 0 : 5);

			} else if ( hit == (-3) ) {
				TypeCol = 5;

			} else {
				TypeCol = 0;
			}
			
			if ( TypeCol != track.m_TypeCol ) {
				track.m_TypeCol = TypeCol;
				track.Invalidate();
			}

			if ( bMove || abs(rect.left - rectLast.left) > size.cx || abs(rect.top - rectLast.top) > size.cy ) {
				if ( !m_bNumber ) {
					SetTabTitle(TRUE);
					GetTitle(idx, title);
					track.SetText(title);
				}
				if ( !rect.EqualRect(rectLast) ) {
					track.SetWindowPos(&wndTopMost, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
					track.Invalidate();
				}
				rectLast = rect;
				bMove = TRUE;
			}

			if ( hit >= 0 && hit != idx ) {		// Move Tab

				tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
				tci.pszText = Text;
				tci.cchTextMax = MAX_PATH;
				m_TabCtrl.GetItem(idx, &tci);

				m_TabCtrl.DeleteItem(idx);
				tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
				m_TabCtrl.InsertItem(hit, &tci);

				track.SetWindowText(tci.pszText);
				track.ShowWindow(SW_HIDE);

				idx = hit;
				bMove = FALSE;
				size.cx = rectFirst.Width()  / 6;
				size.cy = rectFirst.Height() / 3;
			}

			if ( msg.message == WM_MOUSEMOVE )
				break;

			track.DestroyWindow();
			ReleaseCapture();
			m_bTrackMode = FALSE;

			if ( m_bNumber )
				SetTabTitle(FALSE);

			ClientToScreen(&capos);

			if ( hit == (-1) ) {						// Move Pane
				pMain->ScreenToClient(&capos);
				pMain->MoveChild(FromHandle(hTabWnd), capos);
				pMain->PostMessage(WM_COMMAND, IDM_DISPWINIDX);

			} else if ( hit == (-2) && bOtherMove ) {	// Other RLogin Exec
				capos.y = capos.y - GetSystemMetrics(SM_CYCAPTION) / 2;
				pApp->OpenRLogin(pDoc, &capos);
			}

			return;

		//case WM_PAINT:
		//case WM_ERASEBKGND:
		//case WM_CAPTURECHANGED:
		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if ( ((CRLoginApp *)::AfxGetApp())->IsIdleMessage(&msg) )
				bIdle = TRUE;
			break;
		}
	}
}
void CTabBar::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTTDISPINFO* pINFO = (NMTTDISPINFO*)pNMHDR;
	CChildFrame *pChild;
	CRLoginDoc *pDoc;
	CTime tm;

	if ( (pChild = (CChildFrame *)GetAt((int)pINFO->hdr.idFrom)) == NULL ||
		 (pDoc = (CRLoginDoc *)pChild->GetActiveDocument()) == NULL )
		return;

	if ( !pDoc->m_TextRam.IsOptEnable(TO_RLTABINFO) )
		return;

	tm = pDoc->m_ConnectTime;
	m_ToolTipStr.Format(_T("%s\r\n%s@%s\r\n%s"),
		(LPCTSTR)pDoc->m_ServerEntry.m_EntryName,
		pDoc->m_ServerEntry.m_ProtoType == PROTO_COMPORT ? (LPCTSTR)pDoc->m_ServerEntry.m_PortName : (LPCTSTR)pDoc->m_ServerEntry.m_UserName, 
		(LPCTSTR)pDoc->m_ServerEntry.m_HostName,
		(LPCTSTR)tm.Format(_T("%c")));

	pINFO->lpszText = (LPTSTR)(LPCTSTR)m_ToolTipStr;
	pINFO->hinst = NULL;

	*pResult = 0;
}

//////////////////////////////////////////////////////////////////////

int CTabBar::GetImageIndex(LPCTSTR filename)
{
	int idx = (-1);
	CStringBinary *pNode;
	
	if ( (pNode = m_ImageFile.Find(filename)) == NULL ) {
		CClientDC dc(&m_TabCtrl);
		CBmpFile image;
		CBitmap *pBitmap;
		COLORREF bc = RGB(255, 255, 255);

		if ( image.LoadFile(filename) && (pBitmap = image.GetBitmap(&dc, ICONIMG_SIZE, ICONIMG_SIZE, bc)) != NULL )
			idx = m_ImageList.Add(pBitmap, bc);

		m_ImageFile[filename].m_Value = idx;
		return idx;

	} else
		return pNode->m_Value;
}
void CTabBar::Add(CWnd *pWnd, int index)
{
	TC_ITEM tci;
	CString title;

	if ( index == (-1) )
		index = m_TabCtrl.GetItemCount();

	tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	tci.pszText = _T("");
	tci.lParam = NULL;
	tci.iImage = (-1);

	while ( index >= m_TabCtrl.GetItemCount() )
		m_TabCtrl.InsertItem(m_TabCtrl.GetItemCount(), &tci);

	pWnd->GetWindowText(title);
	tci.pszText = title.LockBuffer();
	tci.lParam = (LPARAM)(pWnd->m_hWnd);
	tci.iImage = (-1);

	m_TabCtrl.SetItem(index, &tci);

	title.UnlockBuffer();
	ReSize(TRUE);
}

void CTabBar::Remove(CWnd *pWnd)
{
	int n;
	TC_ITEM tci;
	CRLoginView *pView;
	BOOL reset = FALSE;

	if ( m_pGhostView != NULL && (pView = (CRLoginView *)((CChildFrame *)pWnd)->GetActiveView()) != NULL && pView->m_hWnd == m_pGhostView->m_hWnd )
		SetGhostWnd(FALSE);

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM;
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		if ( (HWND)(tci.lParam) == pWnd->m_hWnd ) {
			// スクロール中にすべてのタブが消えると再表示されないバグがCTabCtrlにある
			if ( n == (m_TabCtrl.GetItemCount() - 1) ) {
				CRect rect;
				if ( m_TabCtrl.GetItemRect(n, rect) && rect.left <= 2 )
					reset = TRUE;
			}
			m_TabCtrl.DeleteItem(n);
			if ( reset )
				ReSetAllTab();
			break;
		}
	}
	ReSize(TRUE);
}

CWnd *CTabBar::GetAt(int nIndex)
{
	TC_ITEM tci;

	tci.mask = TCIF_PARAM;

	if ( !m_TabCtrl.GetItem(nIndex, &tci) )
		return NULL;

	return ((HWND)tci.lParam != NULL ? FromHandle((HWND)tci.lParam) : NULL);
}
void CTabBar::GetTitle(int nIndex, CString &title)
{
	TC_ITEM tci;
	TCHAR tmp[MAX_PATH + 2];
	CWnd *pWnd;
	CString str;

	ZeroMemory(&tci, sizeof(tci));
	tci.mask = TCIF_TEXT | TCIF_PARAM;
	tci.pszText = tmp;
	tci.cchTextMax = MAX_PATH;

	if ( !m_TabCtrl.GetItem(nIndex, &tci) )
		return;

	str = tci.pszText;

	if ( str.IsEmpty() && tci.lParam != NULL ) {
		pWnd = (CChildFrame *)FromHandle((HWND)tci.lParam);
		pWnd->GetWindowText(str);
	}

	if ( m_bNumber )
		title = str;
	else
		title.Format(_T("%d %s"), nIndex + 1, (LPCTSTR)str);
}

int CTabBar::LineCount(BOOL bHorz)
{
	int n;
	int line = 0;
	int top = (-1);
	CRect rect;

	if ( !m_bMultiLine || m_TabCtrl.GetItemCount() <= 0 )
		return 1;

	if ( bHorz ) {
		for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
			if ( !m_TabCtrl.GetItemRect(n, rect) )
				break;
			if ( top != rect.top ) {
				top = rect.top;
				line++;
			}
		}
	} else {
		for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
			if ( !m_TabCtrl.GetItemRect(n, rect) )
				continue;
			if ( top != rect.left ) {
				top = rect.left;
				line++;
			}
		}
	}

	return line;
}
void CTabBar::ReSize(BOOL bCallLayout)
{
	int width, height, lines;
	int count = m_TabCtrl.GetItemCount();
	CRect rect;
	BOOL bSetSize = FALSE;
	BOOL bHorz = (GetBarStyle() & (CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM)) != 0 ? TRUE : FALSE;
	DWORD reqStyle;

	if ( m_TabCtrl.GetSafeHwnd() == NULL )
		return;

	switch(GetBarStyle() & (CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM | CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT)) {
	case CBRS_ALIGN_TOP:	reqStyle = 0; break;
	case CBRS_ALIGN_BOTTOM: reqStyle = TCS_BOTTOM; break;
	case CBRS_ALIGN_LEFT:	reqStyle = TCS_VERTICAL; break;
	case CBRS_ALIGN_RIGHT:	reqStyle = TCS_VERTICAL | TCS_RIGHT; break;
	}

	if ( m_bMultiLine )
		reqStyle |= (TCS_MULTILINE | TCS_BUTTONS);

	if ( (m_TabCtrl.GetStyle() & (TCS_MULTILINE | TCS_VERTICAL | TCS_BOTTOM | TCS_RIGHT)) != reqStyle )
		m_TabCtrl.ModifyStyle((TCS_MULTILINE | TCS_VERTICAL | TCS_BOTTOM | TCS_RIGHT), reqStyle);

	m_TabCtrl.GetClientRect(rect);

	if ( count < DEFTAB_COUNT )
		count = DEFTAB_COUNT;

	if ( bHorz ) {
		if ( m_bMultiLine ) {
			if ( (m_MinTabSize = (rect.Width() - NEWCLICK_SIZE - m_BoderSize * 20) / 10) < (MINTAB_SIZE * 2) )
				m_MinTabSize = MINTAB_SIZE * 2;

			if ( (width = (rect.Width() - NEWCLICK_SIZE - m_BoderSize * 20) / count) < m_MinTabSize ) {
				int n = (rect.Width() - NEWCLICK_SIZE) / m_MinTabSize;
				if ( n > 0 )
					width = (rect.Width() - NEWCLICK_SIZE - m_BoderSize * 2 * n) / n;
				else
					width = m_MinTabSize;
			}
		} else {
			if ( (m_MinTabSize = (rect.Width() - NEWCLICK_SIZE) / 20) < MINTAB_SIZE )
				m_MinTabSize = MINTAB_SIZE;

			if ( (width = (rect.Width() - NEWCLICK_SIZE) / count) < m_MinTabSize )
				width = m_MinTabSize;
		}

		height = m_TabHeight;

		if ( m_TabCtrl.GetItemRect(0, rect) && (width != rect.Width() || height != rect.Height()) ) {
			m_TabCtrl.SetItemSize(CSize(width, height));
			bSetSize = TRUE;
		}

	} else {
		if ( m_bMultiLine ) {
			if ( (m_MinTabSize = (rect.Height() - NEWCLICK_SIZE - m_BoderSize * 20) / 10) < (MINTAB_SIZE * 2) )
				m_MinTabSize = MINTAB_SIZE * 2;

			if ( (height = (rect.Height() - NEWCLICK_SIZE - m_BoderSize * 20) / count) < m_MinTabSize ) {
				int n = (rect.Height() - NEWCLICK_SIZE) / m_MinTabSize;
				if ( n > 0 )
					height = (rect.Height() - NEWCLICK_SIZE - m_BoderSize * 2 * n) / n;
				else
					height = m_MinTabSize;
			}
		} else {
			if ( (m_MinTabSize = (rect.Height() - NEWCLICK_SIZE) / 20) < MINTAB_SIZE )
				m_MinTabSize = MINTAB_SIZE;

			if ( (height = (rect.Height() - NEWCLICK_SIZE) / count) < m_MinTabSize )
				height = m_MinTabSize;
		}

		width = m_TabHeight;

		if ( m_TabCtrl.GetItemRect(0, rect) && (width != rect.Width() || height != rect.Height()) ) {
			m_TabCtrl.SetItemSize(CSize(height, width));	// 注意
			bSetSize = TRUE;
		}
	}

	lines = LineCount(bHorz);

	if ( lines != m_TabLines || (bSetSize && bCallLayout) ) {
		m_TabLines = lines;
		((CMainFrame *)::AfxGetMainWnd())->RecalcLayout(FALSE);
	}
}

void CTabBar::NextActive()
{
	int idx = GetCurSel();
	CChildFrame *pWnd;

	if ( idx < 0 || idx >= GetSize() )
		return;

	if ( ++idx >= GetSize() )
		idx = 0;

	if ( (pWnd = (CChildFrame *)GetAt(idx)) != NULL )
		pWnd->MDIActivate();
}
void CTabBar::PrevActive()
{
	int idx = GetCurSel();
	CChildFrame *pWnd;

	if ( idx < 0 || idx >= GetSize() )
		return;

	if ( --idx < 0 )
		idx = GetSize() - 1;
	
	if ( (pWnd = (CChildFrame *)GetAt(idx)) != NULL )
		pWnd->MDIActivate();
}
void CTabBar::SelectActive(int idx)
{
	CChildFrame *pWnd;

	if ( idx < 0 || idx >= GetSize() )
		return;

	if ( (pWnd = (CChildFrame *)GetAt(idx)) != NULL )
		pWnd->MDIActivate();
}
void CTabBar::DeleteActive()
{
	int idx = GetCurSel();
	CChildFrame *pWnd;

	if ( idx < 0 || idx >= GetSize() )
		return;

	if ( ++idx >= GetSize() ) {
		if ( (idx -= 2) < 0 )
			idx = 0;
	}

	if ( (pWnd = (CChildFrame *)GetAt(idx)) != NULL )
		pWnd->MDIActivate();
}
int CTabBar::GetIndex(CWnd *pWnd)
{
	int n;
	CWnd *pFrame;

	for ( n = 0 ; n < GetSize() ; n++ ) {
		if ( (pFrame = GetAt(n)) != NULL && pFrame->m_hWnd == pWnd->m_hWnd )
			return n;
	}
	return (-1);
}

int CTabBar::HitPoint(CPoint point)
{
	CRect rect;
	CWnd *pMain = AfxGetMainWnd();

	// スクリーン座標
	ClientToScreen(&point);

	pMain->GetWindowRect(rect);

	if ( !rect.PtInRect(point) )
		return (-2);				// メインウィンドウ外

	pMain->GetClientRect(rect);
	pMain->ClientToScreen(rect);

	if ( !rect.PtInRect(point) )
		return (-3);				// メインクライアント外

	// CTabBarクライアント座標
	m_TabCtrl.ScreenToClient(&point);
	for ( int n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		if ( m_TabCtrl.GetItemRect(n, rect) && rect.PtInRect(point) )
			return n;				// タブ内
	}

	return (-1);					// メインクライアント内
}

void CTabBar::SetTabTitle(BOOL bNumber)
{
	int n;
	TC_ITEM tci;
	CString title, work;
	TCHAR tmp[MAX_PATH + 2] = { _T('\0') };
	CWnd *pWnd;

	m_bNumber = bNumber;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.pszText = tmp;
		tci.cchTextMax = MAX_PATH;

		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;

		if ( (HWND)tci.lParam != NULL ) {
			pWnd = FromHandle((HWND)tci.lParam);
			pWnd->GetWindowText(work);
		} else
			work.Empty();

		if ( m_bNumber )
			title.Format(_T("%d %s"), n + 1, (LPCTSTR)work);
		else
			title = work;

		if ( title.Compare(tmp) != 0 ) {
			tci.mask = TCIF_TEXT;
			tci.pszText = title.LockBuffer();
			m_TabCtrl.SetItem(n, &tci);
			title.UnlockBuffer();
		}
	}
}

void CTabBar::SetGhostWnd(BOOL sw)
{
	TC_ITEM tci;
	CChildFrame *pFrame;
	CMainFrame *pMain;

	if ( sw ) {		// Create Ghost Wnd
		tci.mask = TCIF_PARAM;
		if ( !m_TabCtrl.GetItem(m_GhostReq, &tci) )
			return;

		if ( (pMain = (CMainFrame *)AfxGetMainWnd()) == NULL )
			return;

		if ( (HWND)tci.lParam == NULL || !pMain->IsOverLap((HWND)tci.lParam) )
			return;

		m_GhostItem = m_GhostReq;
		m_GhostWndTimer = SetTimer(TBTMID_GHOSTWMD, 200, NULL);

		if ( (pFrame = (CChildFrame *)GetAt(m_GhostItem)) != NULL && (m_pGhostView = (CRLoginView *)pFrame->GetActiveView()) != NULL )
			m_pGhostView->SetGhostWnd(TRUE);

	} else {		// Destory Ghost Wnd
		if ( m_GhostItem >= 0 && m_GhostWndTimer != 0 ) {
			KillTimer(m_GhostWndTimer);
			m_GhostWndTimer = 0;
		}

		if ( m_pGhostView != NULL )
			m_pGhostView->SetGhostWnd(FALSE);

		m_GhostItem = (-1);
		m_pGhostView = NULL;
	}
}
void CTabBar::ReSetAllTab()
{
	int n, max, idx = (-1);
	TC_ITEM *pTci;
	TCHAR tmp[MAX_PATH + 2];
	CStringArray texts;
	
	if ( (max = m_TabCtrl.GetItemCount()) <= 0 )
		return;

	idx = m_TabCtrl.GetCurSel();

	pTci = new TC_ITEM[max];

	for ( n = 0 ; n < max ; n++ ) {
		pTci[n].mask = TCIF_PARAM | TCIF_IMAGE | TCIF_TEXT;
		pTci[n].pszText = tmp;
		pTci[n].cchTextMax = MAX_PATH;
		pTci[n].lParam = NULL;
		pTci[n].iImage = (-1);

		m_TabCtrl.GetItem(n, &(pTci[n]));
		texts.Add(pTci[n].pszText);
	}

	m_TabCtrl.DeleteAllItems();

	for ( n = 0 ; n < max ; n++ ) {
		pTci[n].mask = TCIF_PARAM | TCIF_IMAGE | TCIF_TEXT;
		pTci[n].pszText = texts[n].LockBuffer();

		m_TabCtrl.InsertItem(n, &(pTci[n]));
		texts[n].UnlockBuffer();
	}

	if ( idx >= 0 )
		m_TabCtrl.SetCurSel(idx);

	delete [] pTci;
}

void CTabBar::MultiLine()
{
	m_bMultiLine = (m_bMultiLine ? FALSE : TRUE);

	AfxGetApp()->WriteProfileInt(_T("TabBar"), _T("MultiLine"), m_bMultiLine ? 1 :0);

	if ( m_bMultiLine )
		m_TabCtrl.ModifyStyle(0, TCS_MULTILINE | TCS_BUTTONS);
	else
		m_TabCtrl.ModifyStyle(TCS_MULTILINE | TCS_BUTTONS, 0);

	ReSize(TRUE);
}

void CTabBar::OnRButtonDown(UINT nFlags, CPoint point)
{
//	CControlBar::OnRButtonDown(nFlags, point);

	int cmd, idx;
	CMenuLoad menu;
	CDWordArray nIdTab;
	CMainFrame *pMain = ((CMainFrame *)::AfxGetMainWnd());

	if ( !pMain->m_ServerEntryTab.SetMenuEntry(&menu, 1000) )
		return;

	if ( (idx = HitPoint(point)) >= 0 ) {
		UINT nId;
		CMenuLoad PopUpMenu;
		CMenu *pSubMenu;
		CString str;
		CBitmap *pBitmap;

		if ( PopUpMenu.LoadMenu(IDR_POPUPMENU) && (pSubMenu = PopUpMenu.GetSubMenu(10)) != NULL ) {
			for ( int n = 0 ; n < pSubMenu->GetMenuItemCount() ; n++ ) {
				nId = pSubMenu->GetMenuItemID(n);
				switch(nId) {
				case (-1):	// SubMenu ?
					break;
				case 0:
					menu.AppendMenu(MF_SEPARATOR, NULL);
					break;
				case IDM_RIGHTCLOSE:
					pSubMenu->GetMenuString(n, str, MF_BYPOSITION);
					menu.AppendMenu(MF_STRING | ((idx + 1) < m_TabCtrl.GetItemCount() ? 0 : MF_GRAYED), nIdTab.GetSize() + 100, str);
					nIdTab.Add(nId);
					break;
				case IDM_OTHERCLOSE:
					pSubMenu->GetMenuString(n, str, MF_BYPOSITION);
					menu.AppendMenu(MF_STRING | (m_TabCtrl.GetItemCount() > 1 ? 0 : MF_GRAYED), nIdTab.GetSize() + 100, str);
					nIdTab.Add(nId);
					break;
				default:
					pSubMenu->GetMenuString(n, str, MF_BYPOSITION);
					menu.AppendMenu(MF_STRING, (UINT)nIdTab.GetSize() + 100, str);
					if ( (pBitmap = pMain->GetMenuBitmap(nId)) != NULL )
						menu.SetMenuItemBitmaps((UINT)nIdTab.GetSize() + 100, MF_BYCOMMAND, pBitmap, NULL);
					nIdTab.Add(nId);
					break;
				}
			}
		}
	}

	ClientToScreen(&point);
	cmd = pMain->TrackPopupMenuIdle(&menu, TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, this);
	pMain->m_ServerEntryTab.DelMenuEntry(&menu);

	if ( cmd >= 100 && cmd < 1000 && idx >= 0 ) {
		CChildFrame *pWnd;

		switch(nIdTab[cmd - 100]) {
		case IDM_OTHERCLOSE:
			for ( int n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
				if ( n != idx && (pWnd = (CChildFrame *)GetAt(n)) != NULL )
					pWnd->PostMessage(WM_COMMAND, ID_FILE_CLOSE);
			}
			break;
		case IDM_RIGHTCLOSE:
			for ( int n = idx + 1 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
				if ( (pWnd = (CChildFrame *)GetAt(n)) != NULL )
					pWnd->PostMessage(WM_COMMAND, ID_FILE_CLOSE);
			}
			break;
		case ID_SPLIT_HEIGHT:
		case ID_SPLIT_WIDTH:
		case ID_SPLIT_OVER:
			if ( (pWnd = (CChildFrame *)GetAt(idx)) != NULL ) {
				pMain->MDIActivate(pWnd);
				pWnd->PostMessage(WM_COMMAND, nIdTab[cmd - 100]);
			}
			break;
		default:
			if ( (pWnd = (CChildFrame *)GetAt(idx)) != NULL )
				pWnd->PostMessage(WM_COMMAND, nIdTab[cmd - 100]);
			break;
		}

	} else if ( cmd >= 1000 && cmd < (pMain->m_ServerEntryTab.GetSize() + 1000) ) {
		for ( int n = 0 ; n < pMain->m_ServerEntryTab.GetSize() ; n++ )
			pMain->m_ServerEntryTab.m_Data[n].m_CheckFlag = FALSE;
		pMain->m_ServerEntryTab.m_Data[cmd - 1000].m_CheckFlag = TRUE;

		pMain->PostMessage(WM_COMMAND, (WPARAM)ID_FILE_NEW);
	}
}

BOOL CTabBar::OnEraseBkgnd(CDC* pDC)
{
	CControlBarEx::EraseBkgnd(this, pDC, GetAppColor(APPCOL_BARBACK));
	return TRUE;
}
