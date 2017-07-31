// TabBar.cpp: CTabBar クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TabBar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CTabBar::CTabBar()
{
	m_GhostReq = m_GhostItem = (-1);
	m_pGhostView = NULL;
}

CTabBar::~CTabBar()
{
}

IMPLEMENT_DYNAMIC(CTabBar, CControlBar)

BEGIN_MESSAGE_MAP(CTabBar, CControlBar)
	//{{AFX_MSG_MAP(CTabBar)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_NOTIFY(TCN_SELCHANGE, IDC_MDI_TAB_CTRL, OnSelchange)
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CTabBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	m_dwStyle = (dwStyle & CBRS_ALL);

	dwStyle &= ~CBRS_ALL;
	dwStyle |= CCS_NOPARENTALIGN|CCS_NOMOVEY|CCS_NODIVIDER|CCS_NORESIZE;
	if (pParentWnd->GetStyle() & WS_THICKFRAME) dwStyle |= SBARS_SIZEGRIP;

	CRect rect; rect.SetRectEmpty();
	return CWnd::Create(STATUSCLASSNAME, NULL, dwStyle, rect, pParentWnd, nID);
}

//////////////////////////////////////////////////////////////////////
// CTabBar メッセージ ハンドラ

int CTabBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CControlBar::OnCreate(lpCreateStruct) == (-1) )
		return (-1);
	
	CRect rect; rect.SetRectEmpty();
	if ( !m_TabCtrl.Create(WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_FIXEDWIDTH | TCS_FORCELABELLEFT, rect, this, IDC_MDI_TAB_CTRL) ) {
		TRACE0("Unable to create tab control bar\n");
		return (-1);
	}
 
	CFont *font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
	m_TabCtrl.SetFont(font);

	return 0;
}

void CTabBar::OnSize(UINT nType, int cx, int cy) 
{
	CControlBar::OnSize(nType, cx, cy);
	if ( m_TabCtrl.m_hWnd == NULL )
		return;
	CRect rect;
	GetWindowRect(rect);
	m_TabCtrl.AdjustRect(TRUE, &rect);
	m_TabCtrl.SetWindowPos(&wndTop ,0, 0, rect.Width(), rect.Height(), SWP_SHOWWINDOW);
}

void CTabBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	int n;
	TC_ITEM tci;
	CString title;
	TCHAR tmp[MAX_PATH + 2];
	CWnd *pWnd;
	CMDIFrameWnd *pMainframe = ((CMDIFrameWnd *)AfxGetMainWnd());
	CMDIChildWnd* pActive = (pMainframe == NULL ? NULL : pMainframe->MDIGetActive(NULL));

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.pszText = tmp;
		tci.cchTextMax = MAX_PATH;
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		pWnd = FromHandle((HWND)tci.lParam);
		pWnd->GetWindowText(title);
		if ( title.Compare(tmp) != 0 ) {
			tci.mask = TCIF_TEXT;
			tci.pszText = title.LockBuffer();
			m_TabCtrl.SetItem(n, &tci);
			title.UnlockBuffer();
		}
		if ( pActive != NULL && pActive->m_hWnd == pWnd->m_hWnd )
			m_TabCtrl.SetCurSel(n);
	}
	ReSize();
}

void CTabBar::Add(CWnd *pWnd)
{
	TC_ITEM tci;
	CString title;

	pWnd->GetWindowText(title);
	tci.mask = TCIF_PARAM | TCIF_TEXT;
	tci.pszText = title.LockBuffer();
	tci.lParam = (LPARAM)(pWnd->m_hWnd);
	m_TabCtrl.InsertItem(m_TabCtrl.GetItemCount(), &tci);
	title.UnlockBuffer();
	ReSize();
}

void CTabBar::Remove(CWnd *pWnd)
{
	int n;
	TC_ITEM tci;
	CRLoginView *pView;

	if ( m_pGhostView != NULL && (pView = (CRLoginView *)((CChildFrame *)pWnd)->GetActiveView()) != NULL && pView->m_hWnd == m_pGhostView->m_hWnd )
		SetGhostWnd(FALSE);

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM;
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		if ( (HWND)(tci.lParam) == pWnd->m_hWnd ) {
			m_TabCtrl.DeleteItem(n);
			break;
		}
	}
	ReSize();
}

CWnd *CTabBar::GetAt(int nIndex)
{
	TC_ITEM tci;
	tci.mask = TCIF_PARAM;
	if ( !m_TabCtrl.GetItem(nIndex, &tci) )
		return NULL;
	return FromHandle((HWND)tci.lParam);
}

void CTabBar::ReSize()
{
	int width;
	CRect rect;
	CSize sz;
	int n = m_TabCtrl.GetItemCount();
	CWnd *pWnd = GetParent();

	if ( pWnd == NULL || n <= 0 )
		return;

	if ( n < 4 ) n = 4;
	pWnd->GetClientRect(rect);
	width = (rect.Width() - 2) / n;

	m_TabCtrl.GetItemRect(0, rect);
	if ( width == rect.Width() )
		return;

	sz.cx = width;
	sz.cy = rect.Height();
	sz = m_TabCtrl.SetItemSize(sz);
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
int CTabBar::GetIndex(CWnd *pWnd)
{
	int n;
	CWnd *pFrame;

	for ( n = 0 ; n <  GetSize() ; n++ ) {
		if ( (pFrame = GetAt(n)) != NULL && pFrame->m_hWnd == pWnd->m_hWnd )
			return n;
	}
	return (-1);
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
void CTabBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect rect;
	TC_ITEM tci;
	int idx = m_TabCtrl.GetCurSel();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CWnd *pLeft = GetAt(idx);
	CWnd *pRight;
	CString title;
	TCHAR Text[MAX_PATH + 2];

	CControlBar::OnLButtonDown(nFlags, point);

	if ( pLeft == NULL || !m_TabCtrl.GetItemRect(idx, rect) || !rect.PtInRect(point) )
		return;

	CRectTracker tracker(rect, CRectTracker::hatchedBorder);

	if ( !tracker.Track(this, point, FALSE, CWnd::GetDesktopWindow()) )
		return;

	m_TabCtrl.Invalidate(FALSE);

	point.x = point.x - rect.left + tracker.m_rect.left;
	point.y = point.y - rect.top + tracker.m_rect.top;

	for ( int n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		if ( n == idx || !m_TabCtrl.GetItemRect(n, rect) || !rect.PtInRect(point) )
			continue;

#if 1
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.pszText = Text;
		tci.cchTextMax = MAX_PATH;
		if ( !m_TabCtrl.GetItem(idx, &tci) )
			return;

		m_TabCtrl.DeleteItem(idx);
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		m_TabCtrl.InsertItem(n, &tci);
#else
		if ( (pRight = GetAt(n)) == NULL )
			return;

		pLeft->GetWindowText(title);
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.lParam = (LPARAM)pLeft->m_hWnd;
		tci.pszText = title.LockBuffer();
		m_TabCtrl.SetItem(n, &tci);
		title.UnlockBuffer();

		pRight->GetWindowText(title);
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.lParam = (LPARAM)pRight->m_hWnd;
		tci.pszText = title.LockBuffer();
		m_TabCtrl.SetItem(idx, &tci);
		title.UnlockBuffer();

		pMain->SwapChild(pLeft, pRight);
#endif
		return;
	}

	ClientToScreen(&point);
	pMain->GetWindowRect(rect);

	if ( rect.PtInRect(point) ) {
		pMain->ScreenToClient(&point);
		pMain->MoveChild(pLeft, point);
		return;
	}

	pLeft->PostMessage(WM_CLOSE);
}

CSize CTabBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	ASSERT_VALID(this);
	ASSERT(::IsWindow(m_hWnd));

	// determinme size of font being used by the status bar
	TEXTMETRIC tm;
	{
		CClientDC dc(NULL);
		HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
		HGDIOBJ hOldFont = NULL;
		if (hFont != NULL)
			hOldFont = dc.SelectObject(hFont);
		VERIFY(dc.GetTextMetrics(&tm));
		if (hOldFont != NULL)
			dc.SelectObject(hOldFont);
	}

	// get border information
	CRect rect; rect.SetRectEmpty();
	CalcInsideRect(rect, bHorz);
	int rgBorders[3];
	DefWindowProc(SB_GETBORDERS, 0, (LPARAM)&rgBorders);

	// determine size, including borders
	CSize size;
	size.cx = 32767;
	size.cy = tm.tmHeight - tm.tmInternalLeading - 1
		+ rgBorders[1] * 2 + ::GetSystemMetrics(SM_CYBORDER) * (2 + 3)
		- rect.Height();

	return size;
}

BOOL CTabBar::PreTranslateMessage(MSG* pMsg) 
{
	if ( pMsg->hwnd == 	m_TabCtrl.m_hWnd && pMsg->message == WM_LBUTTONDOWN ) {
		if ( !CControlBar::PreTranslateMessage(pMsg) )
			return FALSE;
		CPoint point(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
		::ClientToScreen(pMsg->hwnd, &point);
		ScreenToClient(&point);
		pMsg->hwnd = m_hWnd;
		pMsg->lParam = MAKELPARAM(point.x, point.y);
	}

	return CControlBar::PreTranslateMessage(pMsg);
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
		if ( !pMain->IsOverLap((HWND)tci.lParam) )
			return;
//		TRACE("Ghost Req %d\n", m_GhostReq);
		m_GhostItem = m_GhostReq;
		SetTimer(1025, 200, NULL);
		if ( (pFrame = (CChildFrame *)GetAt(m_GhostItem)) != NULL && (m_pGhostView = (CRLoginView *)pFrame->GetActiveView()) != NULL )
			m_pGhostView->SetGhostWnd(TRUE);
	} else {		// Destory Ghost Wnd
//		TRACE("Ghost Close %d\n", m_GhostItem);
		if ( m_GhostItem >= 0 )
			KillTimer(1025);
		if ( m_pGhostView != NULL )
			m_pGhostView->SetGhostWnd(FALSE);
		m_GhostItem = (-1);
		m_pGhostView = NULL;
	}
}

BOOL CTabBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	int n;
	TCHITTESTINFO htinfo;

	if ( AfxGetApp()->GetProfileInt(_T("TabBar"), _T("GhostWnd"), 0) )
		return TRUE;

	GetCursorPos(&htinfo.pt);
	m_TabCtrl.ScreenToClient(&htinfo.pt);
	n = m_TabCtrl.HitTest(&htinfo);

	if ( n >= 0 && n == m_TabCtrl.GetCurSel() )
		n = (-1);

	if ( m_GhostItem >= 0 && m_GhostItem != n )
		SetGhostWnd(FALSE);

	if ( m_GhostReq != n ) {
		if ( m_GhostReq >= 0 )
			KillTimer(1024);
		m_GhostReq = (-1);

		if ( n >= 0 ) {
			SetTimer(1024, 2000, NULL);
			m_GhostReq = n;
		}
	}

	return TRUE;
//	return CControlBar::OnSetCursor(pWnd, nHitTest, message);
}

void CTabBar::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent) {
	case 1024:
		KillTimer(1024);
		if ( m_GhostReq >= 0 && (GetStyle() & WS_VISIBLE) != 0 )
			SetGhostWnd(TRUE);
		break;

	case 1025:
		OnSetCursor(this, 0, 0);
		if ( m_GhostItem >= 0 && (GetStyle() & WS_VISIBLE) == 0 )
			SetGhostWnd(FALSE);
		break;
	}
	CControlBar::OnTimer(nIDEvent);
}
