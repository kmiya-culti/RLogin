//////////////////////////////////////////////////////////////////////
// ControlBar...

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ControlBar.h"

/////////////////////////////////////////////////////////////////////////////
// CDockContextEx

CDockContextEx::CDockContextEx(CControlBar *pBar) : CDockContext(pBar)
{
	m_pDestroyWnd = NULL;
}
BOOL CDockContextEx::IsHitGrip(CControlBar *pBar, CPoint point)
{
	CRect rect, inside;
	BOOL bHorz = (pBar->m_dwStyle & CBRS_ORIENT_HORZ) != 0 ? TRUE : FALSE;

	if ( (pBar->m_dwStyle & CBRS_GRIPPER) == 0 )
		return FALSE;

	pBar->GetWindowRect(rect);

	inside = rect;
	pBar->CalcInsideRect(inside, bHorz);

	if ( bHorz ) {
		inside.right = inside.left;
		inside.left = rect.left;
	} else {
		inside.bottom = inside.top;
		inside.top = rect.top;
	}

	return (inside.PtInRect(point) ? TRUE : FALSE);
}
void CDockContextEx::StartDrag(CPoint pt)
{
	m_dwOverDockStyle = m_pBar->IsFloating() ? 0 : m_dwStyle;

	if ( m_dwOverDockStyle != 0 && !IsHitGrip(m_pBar, pt) )
		return;

	m_ptLast = pt;
	m_dwDockStyle = m_pBar->m_dwDockStyle;
	m_dwStyle = m_pBar->m_dwStyle & CBRS_ALIGN_ANY;
	m_bForceFrame = m_bFlip = m_bDitherLast = FALSE;

	//TrackLoop();
	::AfxGetMainWnd()->PostMessage(WM_DOCKBARDRAG, (WPARAM)m_pBar, (LPARAM)this);
}
void CDockContextEx::StartResize(int nHitTest, CPoint pt)
{
	CDockContext::StartResize(nHitTest, pt);
}
void CDockContextEx::ToggleDocking()
{
	CPoint point;

	if ( !m_pBar->IsFloating() ) {
		GetCursorPos(&point);

		if ( !IsHitGrip(m_pBar, point) )
			return;
	}

	CDockContext::ToggleDocking();
}
void CDockContextEx::EnableDocking(CControlBar *pBar, DWORD dwDockStyle)
{
	pBar->m_dwDockStyle = dwDockStyle;

	if ( pBar->m_pDockContext == NULL )
		pBar->m_pDockContext = new CDockContextEx(pBar);

	if ( pBar->m_hWndOwner == NULL )
		pBar->m_hWndOwner = pBar->GetParent()->GetSafeHwnd();
}
void CDockContextEx::TrackLoop()
{
	MSG msg;
	LONG count;
	CPoint point, ptOffset;
	CRect rect, frame;
	CRect BaseHorz, BaseVert;
	CRect SaveRect;
	CMiniDockFrameWnd *pFloatBar;
	CDockBarEx *pDockBar;
	BOOL bIdle = FALSE;
	BOOL bVert = FALSE;
	BOOL bFloat = TRUE;
	DWORD dwSaveDockBar = 0;
	DWORD dwOldStyle = 0;

#ifdef	USE_TRACKVIEW
	CTrackWnd trackHorz, trackVert;
	rect.SetRect(0,0,10,10);
	trackHorz.m_TypeCol = 1;
	trackHorz.Create(NULL, _T("Horz"), WS_TILED | WS_CHILD, rect, CWnd::GetDesktopWindow(), (-1));
	trackVert.m_TypeCol = 4;
	trackVert.Create(NULL, _T("Vert"), WS_TILED | WS_CHILD, rect, CWnd::GetDesktopWindow(), (-1));
#endif

	m_pBar->SetCapture();
	m_bDragging = FALSE;

	while ( ::GetCapture() == m_pBar->GetSafeHwnd() ) {
		for ( count = 0 ; bIdle || !::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE) ; count++ ) {
			bIdle = FALSE;
			if ( !((CRLoginApp *)AfxGetApp())->OnIdle(count) )
				break;
		}

		if (  !::GetMessage(&msg, NULL, 0, 0) )
			goto ENDOF;

		switch (msg.message) {
		case WM_MOUSEMOVE:
			if ( ::PeekMessage(&msg, NULL, NULL, NULL, PM_QS_PAINT | PM_NOREMOVE) )
				break;
		case WM_LBUTTONUP:
			point = msg.pt;
			ptOffset = point - m_ptLast;

			if ( !m_bDragging && (abs(ptOffset.x) >= ::GetSystemMetrics(SM_CXDRAG) || abs(ptOffset.y) >= ::GetSystemMetrics(SM_CYDRAG)) ) {

				m_pBar->GetWindowRect(rect);

				if ( (bFloat = m_pBar->IsFloating()) ) {
					pFloatBar = (CMiniDockFrameWnd *)m_pBar->m_pDockBar->GetParent();
					ASSERT_KINDOF(CMiniDockFrameWnd, pFloatBar);

					pFloatBar->GetWindowRect(frame);
					m_pBar->GetWindowRect(rect);

					if ( (m_pBar->m_dwStyle & CBRS_ORIENT_VERT) != 0 ) {
						m_rectDragHorz = CRect(point.x - rect.Height() / 2, point.y - rect.Width() / 2, point.x + rect.Height() / 2, point.y + rect.Width());
						m_rectDragVert = CRect(frame.left, frame.top, frame.right, frame.top + rect.Height());
						bVert = TRUE;
					} else {
						m_rectDragHorz = CRect(frame.left, frame.top, frame.right, frame.top + rect.Height());
						m_rectDragVert = CRect(point.x - rect.Height() / 2, frame.top, point.x + rect.Height() / 2, frame.top + rect.Width());
						bVert = FALSE;
					}

				} else if ( m_dwStyle == CBRS_ALIGN_LEFT || m_dwStyle == CBRS_ALIGN_RIGHT ) {
					m_rectDragHorz = CRect(rect.left - rect.Height() / 2, point.y - rect.Width() / 2, rect.left + rect.Height() / 2, point.y + rect.Width() / 2);
					m_rectDragVert = rect;
					bVert = TRUE;

				} else if ( m_dwStyle == CBRS_ALIGN_TOP || m_dwStyle == CBRS_ALIGN_BOTTOM ) {
					m_rectDragHorz = rect;
					m_rectDragVert = CRect(point.x - rect.Height() / 2, rect.top, point.x + rect.Height() / 2, rect.top + rect.Width());
					bVert = FALSE;
				}

				BaseHorz = m_rectDragHorz;
				BaseVert = m_rectDragVert;
				SaveRect = bVert ? m_rectDragVert : m_rectDragHorz;

				m_bDragging = TRUE;
			}

			if ( m_bDragging ) {
				m_rectDragHorz = BaseHorz + ptOffset;
				m_rectDragVert = BaseVert + ptOffset;
				rect = bVert ? m_rectDragVert : m_rectDragHorz;

#ifdef	USE_TRACKVIEW
				trackHorz.SetWindowPos(&CWnd::wndTopMost, m_rectDragHorz.left, m_rectDragHorz.top, m_rectDragHorz.Width(), m_rectDragHorz.Height(), SWP_SHOWWINDOW);
				trackVert.SetWindowPos(&CWnd::wndTopMost, m_rectDragVert.left, m_rectDragVert.top, m_rectDragVert.Width(), m_rectDragVert.Height(), SWP_SHOWWINDOW);
#endif

				m_dwOverDockStyle = CanDock() & m_dwDockStyle;

				if ( bFloat ) {
					if ( m_dwOverDockStyle != 0 && m_dwOverDockStyle != dwOldStyle ) {
						// フロートからドッキング
						dwSaveDockBar = 0;
						m_pDockSite->DockControlBar(m_pBar, GetDockBar(m_dwOverDockStyle), &rect);
						m_dwStyle = m_pBar->m_dwStyle & CBRS_ALIGN_ANY;
						bVert = ((m_dwStyle & CBRS_ORIENT_VERT) != 0 ? TRUE : FALSE);
						bFloat = FALSE;
					} else {
						// フロートで移動
						if ( SaveRect.left != rect.left || SaveRect.top != rect.top ) {
							pFloatBar->SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
							SaveRect = rect;
							dwOldStyle = 0;
						}
					}

				} else {
					if ( m_dwOverDockStyle == 0 ) {
						// フロートに変更
						dwSaveDockBar = 0;
						dwOldStyle = m_dwStyle;
						m_pDockSite->FloatControlBar(m_pBar, rect.TopLeft(), m_dwStyle);
						pFloatBar = (CMiniDockFrameWnd *)m_pBar->m_pDockBar->GetParent();
						SaveRect = rect;
						m_dwStyle = m_pBar->m_dwStyle & CBRS_ALIGN_ANY;
						bVert = ((m_dwStyle & CBRS_ORIENT_VERT) != 0 ? TRUE : FALSE);
						bFloat = TRUE;
						if ( dwOldStyle != m_dwStyle ) {
							rect = bVert ? m_rectDragVert : m_rectDragHorz;
							pFloatBar->SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
							SaveRect = rect;
						}
					} else if ( m_dwStyle == m_dwOverDockStyle ) {
						// 同じドックで移動
						if ( SaveRect.left != rect.left || SaveRect.top != rect.top ) {
							if ( (pDockBar = (CDockBarEx *)GetDockBar(m_dwOverDockStyle)) != NULL ) {
								if ( dwSaveDockBar == m_dwOverDockStyle ) {
									pDockBar->LoadBarPos(m_pBar);
								} else {
									pDockBar->SaveBarPos();
									dwSaveDockBar = m_dwOverDockStyle;
								}
							}
							m_pDockSite->DockControlBar(m_pBar, pDockBar, &rect);
							SaveRect = rect;
						}
					} else {
						// 違うドックに移動
						dwSaveDockBar = 0;
						m_pDockSite->DockControlBar(m_pBar, GetDockBar(m_dwOverDockStyle), &rect);
						m_dwStyle = m_pBar->m_dwStyle & CBRS_ALIGN_ANY;
						bVert = ((m_dwStyle & CBRS_ORIENT_VERT) != 0 ? TRUE : FALSE);
					}
				}
			}

			if ( msg.message == WM_LBUTTONUP )
				goto ENDOF;
			break;

		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if ( ((CRLoginApp *)::AfxGetApp())->IsIdleMessage(&msg) )
				bIdle = TRUE;
			break;
		}
	}

ENDOF:

#ifdef	USE_TRACKVIEW
	trackHorz.DestroyWindow();
	trackVert.DestroyWindow();
#endif

	ReleaseCapture();
	m_bDragging = FALSE;

	if ( m_pDestroyWnd != NULL ) {
		// CFrameWndを削除(CMiniDockFrameWndEx::DestroyWindow)
		switch(m_pDestroyWnd->m_DelayedDestroy) {
		case DELAYDESTORY_NONE:
		case DELAYDESTORY_RESERV:
			m_pDestroyWnd->m_DelayedDestroy = DELAYDESTORY_NONE;
			break;
		case DELAYDESTORY_REQUEST:
			m_pDestroyWnd->m_DelayedDestroy = DELAYDESTORY_EXEC;
			m_pDestroyWnd->DestroyWindow();
			break;
		}
		m_pDestroyWnd = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMiniDockFrameWndEx

IMPLEMENT_DYNCREATE(CMiniDockFrameWndEx, CMiniDockFrameWnd)

CMiniDockFrameWndEx::CMiniDockFrameWndEx()
{
	m_bDarkMode = FALSE;
	m_DelayedDestroy = DELAYDESTORY_NONE;
}

BOOL CMiniDockFrameWndEx::DestroyWindow()
{
	// CFrameWndの削除を遅らせる(CMiniDockFrameWndEx::OnNcLButtonDown)
	switch(m_DelayedDestroy) {
	case DELAYDESTORY_NONE:
	case DELAYDESTORY_EXEC:
		return CMiniDockFrameWnd::DestroyWindow();

	case DELAYDESTORY_RESERV:
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_HIDEWINDOW);
		m_DelayedDestroy = DELAYDESTORY_REQUEST;
		return TRUE;
	}
	return FALSE;
}
LRESULT CALLBACK OwnerDockBarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if ( uMsg == WM_ERASEBKGND ) {
		CControlBarEx::EraseBkgnd((CControlBar*)uIdSubclass, CDC::FromHandle((HDC)wParam), GetAppColor(APPCOL_BARBACK));
		return TRUE;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
BOOL CMiniDockFrameWndEx::Create(CWnd* pParent, DWORD dwBarStyle)
{
	BOOL ret = CMiniDockFrameWnd::Create(pParent, dwBarStyle);

	if ( ret && m_wndDockBar.GetSafeHwnd() != NULL )
		SetWindowSubclass(m_wndDockBar.GetSafeHwnd(), OwnerDockBarProc, (UINT_PTR)&m_wndDockBar, (DWORD_PTR)this);

	return ret;
}

BEGIN_MESSAGE_MAP(CMiniDockFrameWndEx, CMiniDockFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETTINGCHANGE()
	ON_WM_ERASEBKGND()
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()

int CMiniDockFrameWndEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CMiniDockFrameWnd::OnCreate(lpCreateStruct) == (-1) )
		return (-1);

	m_bDarkMode = ExDwmDarkMode(GetSafeHwnd());

	return 0;
}
void CMiniDockFrameWndEx::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	if ( bDarkModeSupport && lpszSection != NULL && _tcscmp(lpszSection, _T("ImmersiveColorSet")) == 0 ) {
		m_bDarkMode = ExDwmDarkMode(GetSafeHwnd());
		Invalidate(TRUE);
	}

	CMiniDockFrameWnd::OnSettingChange(uFlags, lpszSection);
}
BOOL CMiniDockFrameWndEx::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(rect, GetAppColor(APPCOL_BARBACK));
	return TRUE;
}
void CMiniDockFrameWndEx::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if ( nHitTest == HTCAPTION) {
		ActivateTopParent();

		if ( (m_wndDockBar.m_dwStyle & CBRS_FLOAT_MULTI) == 0 ) {
			int nPos = 1;
			CControlBar* pBar = NULL;
			while(pBar == NULL && nPos < m_wndDockBar.m_arrBars.GetSize())
				pBar = (CControlBar *)m_wndDockBar.m_arrBars[nPos];
			CDockContextEx *pCtx = (CDockContextEx *)pBar->m_pDockContext;

			ASSERT(pBar != NULL && pCtx != NULL && pCtx->m_pDestroyWnd == NULL);

			// CFrameWndの削除でghostwindowのような動作を抑制
			pCtx->m_pDestroyWnd = this;
			m_DelayedDestroy = DELAYDESTORY_RESERV;

			pBar->m_pDockContext->StartDrag(point);
			return;
		}
	}

	CMiniDockFrameWnd::OnNcLButtonDown(nHitTest, point);
}

/////////////////////////////////////////////////////////////////////////////
// CControlBarEx

IMPLEMENT_DYNAMIC(CControlBarEx, CControlBar)

CControlBarEx::CControlBarEx()
{
	m_bDarkMode = FALSE;
}

void CControlBarEx::DrawBorders(CControlBar *pBar, CDC* pDC, CRect& rect, COLORREF bkColor)
{
	CSize bs(1, 1);
	COLORREF bdColor = GetAppColor(APPCOL_BARSHADOW);

	pDC->FillSolidRect(CRect(rect.left, rect.bottom - 1, rect.right, rect.bottom), bkColor); 
	rect.bottom -= 1;

	if ( pBar->m_dwStyle & CBRS_BORDER_LEFT ) {
		pDC->FillSolidRect(CRect(rect.left, rect.top, rect.left + bs.cx, rect.bottom), bdColor);
		rect.left += bs.cx;
	}
	if ( pBar->m_dwStyle & CBRS_BORDER_TOP ) {
		pDC->FillSolidRect(CRect(rect.left, rect.top, rect.right, rect.top + bs.cy), bdColor); 
		rect.top += bs.cy;
	}
	if ( pBar->m_dwStyle & CBRS_BORDER_RIGHT ) {
		pDC->FillSolidRect(CRect(rect.right - bs.cx, rect.top, rect.right, rect.bottom), bdColor); 
		rect.right -= bs.cx;
	}
	if ( pBar->m_dwStyle & CBRS_BORDER_BOTTOM ) {
		pDC->FillSolidRect(CRect(rect.left, rect.bottom - bs.cy, rect.right, rect.bottom), bdColor); 
		rect.bottom -= bs.cy;
	}
}
void CControlBarEx::DrawGripper(CControlBar *pBar, CDC* pDC, const CRect& rect)
{
	if ( pBar->m_dwStyle & CBRS_ORIENT_HORZ ) {
		pDC->Draw3dRect(rect.left + AFX_CX_BORDER_GRIPPER, rect.top + AFX_CY_BORDER_GRIPPER * 2, 
			AFX_CX_GRIPPER, rect.Height() - AFX_CY_BORDER_GRIPPER * 4, GetAppColor(APPCOL_BARHIGH), GetAppColor(APPCOL_BARSHADOW));

	} else {
		pDC->Draw3dRect(rect.left + AFX_CX_BORDER_GRIPPER * 2, rect.top + AFX_CY_BORDER_GRIPPER, 
			rect.Width() - AFX_CX_BORDER_GRIPPER * 4, AFX_CY_GRIPPER, GetAppColor(APPCOL_BARHIGH), GetAppColor(APPCOL_BARSHADOW));
	}
}

BOOL CControlBarEx::DarkModeCheck(CControlBar *pBar)
{
	return ExDwmDarkMode(pBar->GetSafeHwnd());
}
BOOL CControlBarEx::SetCursor(CControlBar *pBar, UINT nHitTest, UINT message)
{
	CPoint point;
	GetCursorPos(&point);

	if ( CDockContextEx::IsHitGrip(pBar, point) && (pBar->m_dwDockStyle & CBRS_ORIENT_ANY) != 0 ) {
		if ( (pBar->m_dwDockStyle & CBRS_ORIENT_HORZ) == 0 )
			::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
		else if ( (pBar->m_dwDockStyle & CBRS_ORIENT_VERT) == 0 )
			::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
		else
			::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
		return TRUE;
	}

	return FALSE;
}
BOOL CControlBarEx::SettingChange(CControlBar *pBar, BOOL &bDarkMode, UINT uFlags, LPCTSTR lpszSection)
{
	if ( bDarkModeSupport && lpszSection != NULL && _tcscmp(lpszSection, _T("ImmersiveColorSet")) == 0 ) {
		bDarkMode = DarkModeCheck(pBar);
		pBar->RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
		return TRUE;
	} else
		return FALSE;
}
BOOL CControlBarEx::EraseBkgnd(CControlBar *pBar, CDC* pDC, COLORREF bkCol)
{
	CRect rect;
	pBar->GetWindowRect(rect);
	pDC->FillSolidRect(0, 0, rect.Width(), rect.Height(), bkCol);
	return TRUE;
}

void CControlBarEx::DrawBorders(CDC* pDC, CRect& rect)
{
	CControlBarEx::DrawBorders(this, pDC, rect, GetAppColor(APPCOL_BARBACK));
}
BOOL CControlBarEx::DrawThemedGripper(CDC* pDC, const CRect& rect, BOOL fCentered)
{
	CControlBarEx::DrawGripper(this, pDC, rect);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CControlBarEx, CControlBar)
	ON_WM_CREATE()
	ON_WM_SETCURSOR()
	ON_WM_SETTINGCHANGE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

int CControlBarEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CControlBar::OnCreate(lpCreateStruct) == (-1) )
		return (-1);

	m_bDarkMode = CControlBarEx::DarkModeCheck(this);

	return 0;
}
BOOL CControlBarEx::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ( pWnd == this  &&  nHitTest == HTCLIENT && CControlBarEx::SetCursor(this, nHitTest, message) )
		return TRUE;

	return CControlBar::OnSetCursor(pWnd, nHitTest, message);
}
void CControlBarEx::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CControlBarEx::SettingChange(this, m_bDarkMode, uFlags, lpszSection);
	CControlBar::OnSettingChange(uFlags, lpszSection);
}
BOOL CControlBarEx::OnEraseBkgnd(CDC* pDC)
{
	CControlBarEx::EraseBkgnd(this, pDC, GetAppColor(APPCOL_BARBACK));
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDockBarEx

IMPLEMENT_DYNAMIC(CDockBarEx, CDockBar)

CDockBarEx::CDockBarEx(BOOL bFloating) : CDockBar(bFloating)
{
	m_bDarkMode = FALSE;
}
void CDockBarEx::SaveBarPos()
{
	BarPosNode node;
	CControlBar *pBar;

	m_SavePos.RemoveAll();

	for ( int n = 0 ;  n < m_arrBars.GetSize() ; n++ ) {
		if ( (pBar = GetDockedControlBar(n)) == NULL )
			continue;
		node.hWnd = pBar->GetSafeHwnd();
		pBar->GetWindowRect(node.rect);
		ScreenToClient(node.rect);
		m_SavePos.Add(node);
	}
}
void CDockBarEx::LoadBarPos(CControlBar *pThis)
{
	CControlBar *pBar;

	for ( int n = 0 ;  n < m_arrBars.GetSize() ; n++ ) {
		if ( (pBar = GetDockedControlBar(n)) == NULL )
			continue;
		for ( int i = 0 ; i < m_SavePos.GetSize() ; i++ ) {
			if ( m_SavePos[i].hWnd != pBar->GetSafeHwnd() || m_SavePos[i].hWnd == pThis->GetSafeHwnd() )
				continue;
			pBar->SetWindowPos(NULL, m_SavePos[i].rect.left, m_SavePos[i].rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			break;
		}
	}
}

void CDockBarEx::DrawBorders(CDC* pDC, CRect& rect)
{
	CControlBarEx::DrawBorders(this, pDC, rect, GetAppColor(APPCOL_BARBACK));
}

BEGIN_MESSAGE_MAP(CDockBarEx, CDockBar)
	ON_WM_CREATE()
	ON_WM_SETTINGCHANGE()
	ON_WM_ERASEBKGND()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

int CDockBarEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CDockBar::OnCreate(lpCreateStruct) == (-1) )
		return (-1);

	m_bDarkMode = CControlBarEx::DarkModeCheck(this);

	return 0;
}
void CDockBarEx::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CControlBarEx::SettingChange(this, m_bDarkMode, uFlags, lpszSection);
	CDockBar::OnSettingChange(uFlags, lpszSection);
}
BOOL CDockBarEx::OnEraseBkgnd(CDC* pDC)
{
	CControlBarEx::EraseBkgnd(this, pDC, GetAppColor(APPCOL_BARBACK));
	return TRUE;
}
void CDockBarEx::OnRButtonDown(UINT nFlags, CPoint point)
{
	CMenuLoad PopUpMenu;
	CMenu *pSubMenu;

	PopUpMenu.LoadMenu(IDR_MAINFRAME);
	pSubMenu = PopUpMenu.GetSubMenu(1);

	GetCursorPos(&point);
	((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(pSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, ::AfxGetMainWnd());
}

/////////////////////////////////////////////////////////////////////////////
// CDialogBarEx

IMPLEMENT_DYNAMIC(CDialogBarEx, CDialogBar)

CDialogBarEx::CDialogBarEx()
{
	m_bDarkMode = FALSE;
}

static BOOL CALLBACK EnumSetThemeProc(HWND hWnd , LPARAM lParam)
{
	CWnd *pWnd = CWnd::FromHandle(hWnd);
	CDialogBarEx *pParent = (CDialogBarEx *)lParam;
	TCHAR name[256];

	if ( ExSetWindowTheme == NULL )
		return FALSE;

	if ( pWnd->GetParent()->GetSafeHwnd() != pParent->GetSafeHwnd() )
		return TRUE;

	GetClassName(hWnd, name, 256);

	if ( _tcscmp(name, _T("Button")) == 0 ) {
		if ( (pWnd->GetStyle() & BS_TYPEMASK) <= BS_DEFPUSHBUTTON )
			ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
		else if ( pParent->m_bDarkMode )
			ExSetWindowTheme(hWnd, L"", L"");
		else
			ExSetWindowTheme(hWnd, NULL, NULL);
		SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	} else if ( _tcscmp(name, _T("ScrollBar")) == 0 ) {
		ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
		SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);
	}

	return TRUE;
}
BOOL CDialogBarEx::Create(CWnd* pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID)
{
	ASSERT(pParentWnd != NULL);
	ASSERT(lpszTemplateName != NULL);

	// allow chance to modify styles
	m_dwStyle = (nStyle & CBRS_ALL);
	CREATESTRUCT cs;
	memset(&cs, 0, sizeof(cs));
	cs.style = (DWORD)nStyle | WS_CHILD;
	cs.hMenu = (HMENU)(UINT_PTR)nID;
	cs.hInstance = AfxGetInstanceHandle();
	cs.hwndParent = pParentWnd->GetSafeHwnd();
	if (!PreCreateWindow(cs))
		return FALSE;

	// create a modeless dialog
	HGLOBAL hDialog;
	HGLOBAL hInitData = NULL;
	void* lpInitData = NULL;
	LPCDLGTEMPLATE lpDialogTemplate;

	m_lpszTemplateName = lpszTemplateName;

	if ( !((CRLoginApp *)AfxGetApp())->LoadResDialog(m_lpszTemplateName, hDialog, hInitData) )
		return (-1);

	if ( hInitData != NULL )
		lpInitData = (void *)LockResource(hInitData);

	lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialog);

	CDialogTemplate dlgTemp(lpDialogTemplate);

	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = MulDiv(::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9), SCREEN_DPI_Y, SYSTEM_DPI_Y);

	m_InitDpi.cx = SCREEN_DPI_X;
	m_InitDpi.cy = SCREEN_DPI_Y;
	m_NowDpi = m_InitDpi;

	if ( !FontName.IsEmpty() || FontSize != 9 )
		dlgTemp.SetFont(FontName, FontSize);

	lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(dlgTemp.m_hTemplate);

	BOOL bSuccess = CreateDlgIndirect(lpDialogTemplate, pParentWnd, NULL);

	UnlockResource(dlgTemp.m_hTemplate);

	UnlockResource(hDialog);
	FreeResource(hDialog);

	if ( hInitData != NULL ) {
		UnlockResource(hInitData);
		FreeResource(hInitData);
	}

	if (!bSuccess)
		return FALSE;

	// dialog template MUST specify that the dialog
	//  is an invisible child window
	SetDlgCtrlID(nID);
	CRect rect;
	GetWindowRect(&rect);
	m_sizeDefault = rect.Size();    // set fixed size

	// force WS_CLIPSIBLINGS
	ModifyStyle(WS_BORDER, WS_CLIPSIBLINGS);

	if (!ExecuteDlgInit(lpszTemplateName))
		return FALSE;

	m_bDarkMode = CControlBarEx::DarkModeCheck(this);

	if ( bDarkModeSupport )
		EnumChildWindows(GetSafeHwnd(), EnumSetThemeProc, (LPARAM)this);

	// force the size to zero - resizing bar will occur later
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

	return TRUE;
}

static BOOL CALLBACK DpiChangedProc(HWND hWnd , LPARAM lParam)
{
	CWnd *pWnd = CWnd::FromHandle(hWnd);
	CDialogBarEx *pParent = (CDialogBarEx *)lParam;
	CRect rect;

	if ( pWnd->GetParent()->GetSafeHwnd() != pParent->GetSafeHwnd() )
		return TRUE;

	pWnd->GetWindowRect(rect);
	pParent->ScreenToClient(rect);

	rect.left   = MulDiv(rect.left,   pParent->m_ZoomMul.cx, pParent->m_ZoomDiv.cx);
	rect.right  = MulDiv(rect.right,  pParent->m_ZoomMul.cx, pParent->m_ZoomDiv.cx);
	rect.top    = MulDiv(rect.top,    pParent->m_ZoomMul.cy, pParent->m_ZoomDiv.cy);
	rect.bottom = MulDiv(rect.bottom, pParent->m_ZoomMul.cy, pParent->m_ZoomDiv.cy);

	int height = rect.Height();
	rect.top = (pParent->m_sizeDefault.cy - height) / 2;
	rect.bottom = rect.top + height;

	if ( pWnd->SendMessage(WM_DPICHANGED, MAKEWPARAM(SCREEN_DPI_X, SCREEN_DPI_Y), (LPARAM)((RECT *)rect)) == FALSE ) {
		pWnd->SetFont(pParent->GetFont(), FALSE);
		if ( (pParent->GetStyle() & WS_SIZEBOX) == 0 )
			pWnd->MoveWindow(rect, FALSE);
	}

	return TRUE;
}
void CDialogBarEx::DpiChanged()
{
	CFont *pFont;
	LOGFONT LogFont;
	CRect rect, client;

	GetWindowRect(rect);
	GetClientRect(client);

	if ( client.Width() <= 0 || client.Height() <= 0 ) {
		m_ZoomMul.cx = SCREEN_DPI_X;
		m_ZoomDiv.cx = m_NowDpi.cx;
		m_ZoomMul.cy = SCREEN_DPI_Y;
		m_ZoomDiv.cy = m_NowDpi.cy;

		m_sizeDefault.cx = MulDiv(m_sizeDefault.cx, SCREEN_DPI_X, m_NowDpi.cx);
		m_sizeDefault.cy = MulDiv(m_sizeDefault.cy, SCREEN_DPI_Y, m_NowDpi.cy);

	} else {
		rect.right  += (MulDiv(client.Width(),  SCREEN_DPI_X, m_NowDpi.cx) - client.Width());
		rect.bottom += (MulDiv(client.Height(), SCREEN_DPI_Y, m_NowDpi.cy) - client.Height());

		//MoveWindow(rect, FALSE);
		SetWindowPos(&wndTop ,0, 0, rect.Width(), rect.Height(), ((GetStyle() & WS_VISIBLE) != 0 ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOMOVE);
		m_sizeDefault = rect.Size();    // set fixed size

		GetClientRect(rect);

		m_ZoomMul.cx = rect.Width();
		m_ZoomDiv.cx = client.Width();
		m_ZoomMul.cy = rect.Height();
		m_ZoomDiv.cy = client.Height();
	}

	if ( (pFont = GetFont()) != NULL ) {
		pFont->GetLogFont(&LogFont);

		if ( m_NewFont.GetSafeHandle() != NULL )
			m_NewFont.DeleteObject();

		LogFont.lfHeight = MulDiv(LogFont.lfHeight, SCREEN_DPI_Y, m_NowDpi.cy);

		m_NewFont.CreateFontIndirect(&LogFont);
	}

	m_NowDpi.cx = SCREEN_DPI_X;
	m_NowDpi.cy = SCREEN_DPI_Y;
	
	EnumChildWindows(GetSafeHwnd(), DpiChangedProc, (LPARAM)this);
}

static BOOL CALLBACK FontSizeCheckProc(HWND hWnd , LPARAM lParam)
{
	CRect rect;
	CWnd *pWnd = CWnd::FromHandle(hWnd);
	CDialogBarEx *pParent = (CDialogBarEx *)lParam;

	if ( pWnd->GetParent()->GetSafeHwnd() != pParent->GetSafeHwnd() )
		return TRUE;

	pWnd->GetWindowRect(rect);
	pParent->ScreenToClient(rect);

	rect.left   = MulDiv(rect.left,   pParent->m_ZoomMul.cx, pParent->m_ZoomDiv.cx);
	rect.right  = MulDiv(rect.right,  pParent->m_ZoomMul.cx, pParent->m_ZoomDiv.cx);
	rect.top    = MulDiv(rect.top,    pParent->m_ZoomMul.cy, pParent->m_ZoomDiv.cy);
	rect.bottom = MulDiv(rect.bottom, pParent->m_ZoomMul.cy, pParent->m_ZoomDiv.cy);

	int height = rect.Height();
	rect.top = (pParent->m_sizeDefault.cy - height) / 2;
	rect.bottom = rect.top + height;

	pWnd->SetFont(pParent->GetFont(), FALSE);
	pWnd->MoveWindow(rect, FALSE);

	return TRUE;
}
void CDialogBarEx::FontSizeCheck()
{
	CDC *pDc = GetDC();
	CRect rect;
	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = ::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9);

	CDialogExt::GetDlgFontBase(pDc, GetFont(), m_ZoomDiv);

	if ( m_NewFont.GetSafeHandle() != NULL )
		m_NewFont.DeleteObject();

	if ( !FontName.IsEmpty() || FontSize != 9 )
		m_NewFont.CreatePointFont(MulDiv(FontSize * 10, SCREEN_DPI_Y, SYSTEM_DPI_Y), FontName);

	m_InitDpi.cx = SCREEN_DPI_X;
	m_InitDpi.cy = SCREEN_DPI_Y;

	CDialogExt::GetDlgFontBase(pDc, GetFont(), m_ZoomMul);
	ReleaseDC(pDc);

	m_sizeDefault.cx = MulDiv(m_sizeDefault.cx, m_ZoomMul.cx, m_ZoomDiv.cx);
	m_sizeDefault.cy = MulDiv(m_sizeDefault.cy, m_ZoomMul.cy, m_ZoomDiv.cy);

	EnumChildWindows(GetSafeHwnd(), FontSizeCheckProc, (LPARAM)this);
	Invalidate();
}

void CDialogBarEx::DrawBorders(CDC* pDC, CRect& rect)
{
	CControlBarEx::DrawBorders(this, pDC, rect, GetAppColor(APPCOL_BARBACK));
}
BOOL CDialogBarEx::DrawThemedGripper(CDC* pDC, const CRect& rect, BOOL fCentered)
{
	CControlBarEx::DrawGripper(this, pDC, rect);
	return TRUE;
}
CSize CDialogBarEx::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CSize sz = CDialogBar::CalcFixedLayout(bStretch, bHorz);
	Invalidate(TRUE);
	return sz;
}

BEGIN_MESSAGE_MAP(CDialogBarEx, CDialogBar)
	ON_WM_SETCURSOR()
	ON_WM_CTLCOLOR()
	ON_WM_SETTINGCHANGE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CDialogBarEx::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ( pWnd == this  &&  nHitTest == HTCLIENT && CControlBarEx::SetCursor(this, nHitTest, message) )
		return TRUE;

	return CDialogBar::OnSetCursor(pWnd, nHitTest, message);
}
afx_msg HBRUSH CDialogBarEx::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);

	switch(nCtlColor) {
	case CTLCOLOR_MSGBOX:		// Message box
	case CTLCOLOR_EDIT:			// Edit control
	case CTLCOLOR_LISTBOX:		// List-box control
		break;

	case CTLCOLOR_BTN:			// Button control
	case CTLCOLOR_DLG:			// Dialog box
	case CTLCOLOR_SCROLLBAR:
	case CTLCOLOR_STATIC:		// Static control
		hbr = GetAppColorBrush(APPCOL_BARBACK);
		pDC->SetTextColor(GetAppColor(APPCOL_BARTEXT));
		pDC->SetBkMode(TRANSPARENT);
		break;
	}

	return hbr;
}
void CDialogBarEx::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	if ( CControlBarEx::SettingChange(this, m_bDarkMode, uFlags, lpszSection) )
		EnumChildWindows(GetSafeHwnd(), EnumSetThemeProc, (LPARAM)this);

	CDialogBar::OnSettingChange(uFlags, lpszSection);
}
BOOL CDialogBarEx::OnEraseBkgnd(CDC* pDC)
{
	CControlBarEx::EraseBkgnd(this, pDC, GetAppColor(APPCOL_BARBACK));
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVoiceBar

IMPLEMENT_DYNAMIC(CVoiceBar, CDialogBarEx)

void CVoiceBar::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_VOICEDESC, m_VoiceDesc);
	DDX_Control(pDX, IDC_VOICERATE, m_VoiceRate);
}

BEGIN_MESSAGE_MAP(CVoiceBar, CDialogBarEx)
	ON_MESSAGE(WM_INITDIALOG, &CVoiceBar::HandleInitDialog)
	ON_CBN_SELCHANGE(IDC_VOICEDESC, &CVoiceBar::OnCbnSelchangeVoicedesc)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_VOICERATE, &CVoiceBar::OnNMReleasedcaptureVoicerate)
END_MESSAGE_MAP()

LRESULT CVoiceBar::HandleInitDialog(WPARAM wParam, LPARAM lParam)
{
	long rate = 0;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	ISpVoice *pVoice = pApp->VoiceInit();

	UpdateData(FALSE);

	if ( pVoice != NULL ) {
		pApp->SetVoiceListCombo(&m_VoiceDesc);
		pVoice->GetRate(&rate);
	}

	m_VoiceRate.SetRange(0, 20);
	m_VoiceRate.SetTic(10);
	m_VoiceRate.SetPos(rate + 10);

	DpiChanged();

	return 0;
}
void CVoiceBar::OnCbnSelchangeVoicedesc()
{
	int n;
	CString desc;
	long rate;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	
	if ( (n = m_VoiceDesc.GetCurSel()) >= 0 )
		m_VoiceDesc.GetLBText(n, desc);

	rate = m_VoiceRate.GetPos() - 10;

	pApp->SetVoice(desc, rate);
}
void CVoiceBar::OnNMReleasedcaptureVoicerate(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnCbnSelchangeVoicedesc();
	*pResult = 0;
}


/////////////////////////////////////////////////////////////////////////////
// CQuickBar

IMPLEMENT_DYNAMIC(CQuickBar, CDialogBarEx)

void CQuickBar::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_ENTRYNAME, m_EntryWnd);
	DDX_Control(pDX, IDC_HOSTNAME, m_HostWnd);
	DDX_Control(pDX, IDC_PORTNAME, m_PortWnd);
	DDX_Control(pDX, IDC_USERNAME, m_UserWnd);
	DDX_Control(pDX, IDC_PASSNAME, m_PassWnd);
}

void CQuickBar::InitDialog()
{
	CString str;
	CServerEntryTab *pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);

	UpdateData(FALSE);
	
	m_EntryWnd.RemoveAll();
	for ( int n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
		str = pTab->m_Data[n].m_EntryName;
		if ( !str.IsEmpty() && m_EntryWnd.FindStringExact((-1), str) == CB_ERR )
			m_EntryWnd.AddString(str);
	}

	m_HostWnd.LoadHis(_T("QuickBarHostAddr"));
	m_PortWnd.LoadHis(_T("QuickBarSocketPort"));
	m_UserWnd.LoadHis(_T("QuickBarUserName"));

	m_EntryWnd.SetWindowText(AfxGetApp()->GetProfileString(_T("QuickBar"), _T("EntryName"), _T("")));
	m_HostWnd.SetWindowText(AfxGetApp()->GetProfileString(_T("QuickBar"), _T("HostName"), _T("")));
	m_PortWnd.SetWindowText(AfxGetApp()->GetProfileString(_T("QuickBar"), _T("PortName"), _T("")));
	m_UserWnd.SetWindowText(AfxGetApp()->GetProfileString(_T("QuickBar"), _T("UserName"), _T("")));

	// EntryName Update Connect Button Flag
	OnCbnEditchangeEntryname();

	DpiChanged();
}
void CQuickBar::SaveDialog()
{
	CString str;

	m_EntryWnd.GetWindowText(str);
	AfxGetApp()->WriteProfileString(_T("QuickBar"), _T("EntryName"), str);

	m_HostWnd.GetWindowText(str);
	AfxGetApp()->WriteProfileString(_T("QuickBar"), _T("HostName"), str);

	m_PortWnd.GetWindowText(str);
	AfxGetApp()->WriteProfileString(_T("QuickBar"), _T("PortName"), str);

	m_UserWnd.GetWindowText(str);
	AfxGetApp()->WriteProfileString(_T("QuickBar"), _T("UserName"), str);
}
void CQuickBar::SetComdLine(CString &cmds)
{
	CString str, fmt;

	m_EntryWnd.GetWindowText(str);
	if ( !str.IsEmpty() ) {
		if ( !cmds.IsEmpty() ) cmds += _T(" ");
		fmt.Format(_T("/entry \"%s\""), (LPCTSTR)str);
		cmds += fmt;
	}

	m_HostWnd.GetWindowText(str);
	if ( !str.IsEmpty() ) {
		m_HostWnd.AddHis(str);
		if ( !cmds.IsEmpty() ) cmds += _T(" ");
		fmt.Format(_T("/ip \"%s\""), (LPCTSTR)str);
		cmds += fmt;
	}

	m_PortWnd.GetWindowText(str);
	if ( !str.IsEmpty() ) {
		m_PortWnd.AddHis(str);
		if ( !cmds.IsEmpty() ) cmds += _T(" ");
		fmt.Format(_T("/port \"%s\""), (LPCTSTR)str);
		cmds += fmt;
	}

	m_UserWnd.GetWindowText(str);
	if ( !str.IsEmpty() ) {
		m_UserWnd.AddHis(str);
		if ( !cmds.IsEmpty() ) cmds += _T(" ");
		fmt.Format(_T("/user \"%s\""), (LPCTSTR)str);
		cmds += fmt;
	}

	m_PassWnd.GetWindowText(str);
	if ( !str.IsEmpty() ) {
		if ( !cmds.IsEmpty() ) cmds += _T(" ");
		fmt.Format(_T("/pass \"%s\""), (LPCTSTR)str);
		cmds += fmt;
	}

	// Save Last Data
	SaveDialog();
}

BEGIN_MESSAGE_MAP(CQuickBar, CDialogBarEx)
	ON_CBN_EDITCHANGE(IDC_ENTRYNAME, &CQuickBar::OnCbnEditchangeEntryname)
	ON_CBN_SELCHANGE(IDC_ENTRYNAME, &CQuickBar::OnCbnEditchangeEntryname)
END_MESSAGE_MAP()

void CQuickBar::OnCbnEditchangeEntryname()
{
	CString str;
	BOOL rc = FALSE;

	if ( m_EntryWnd.GetCurSel() >= 0 )
		rc = TRUE;
	else {
		m_EntryWnd.GetWindowText(str);
		if ( !str.IsEmpty() && m_EntryWnd.FindStringExact((-1), str) != CB_ERR )
			rc = TRUE;
	}

	((CMainFrame *)::AfxGetMainWnd())->m_bQuickConnect = rc;
}

/////////////////////////////////////////////////////////////////////////////
// CTabDlgBar

IMPLEMENT_DYNAMIC(CTabDlgBar, CControlBarEx)

CTabDlgBar::CTabDlgBar()
{
	m_InitSize.cx = m_InitSize.cy = 0;
	m_pShowWnd = NULL;
	m_FontSize = 0;
}

CTabDlgBar::~CTabDlgBar()
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ )
		delete (struct _DlgWndData *)m_Data[n];
	m_Data.RemoveAll();
}

BOOL CTabDlgBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	m_dwStyle = (dwStyle & CBRS_ALL);

	dwStyle &= ~CBRS_ALL;
	dwStyle |= CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NODIVIDER | CCS_NORESIZE;

	CRect rect; rect.SetRectEmpty();
	return CWnd::Create(STATUSCLASSNAME, NULL, dwStyle, rect, pParentWnd, nID);
}

void CTabDlgBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	int n;
	TC_ITEM tci;
	CString title;
	TCHAR tmp[MAX_PATH + 2] = { _T('\0') };
	CWnd *pWnd;
	BOOL bUpdate = FALSE;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.pszText = tmp;
		tci.cchTextMax = MAX_PATH;

		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;

		tci.mask = 0;

		pWnd = (CWnd *)tci.lParam;
		pWnd->GetWindowText(title);

		if ( title.GetLength() >= MAX_PATH )
			title = title.Left(MAX_PATH -1);

		if ( title.Compare(tmp) != 0 ) {
			tci.mask |= TCIF_TEXT;
			tci.pszText = (LPWSTR)(LPCWSTR)TstrToUni(title);
		}

		if ( tci.mask != 0 ) {
			m_TabCtrl.SetItem(n, &tci);
			bUpdate = TRUE;
		}
	}

	if ( bUpdate && m_pShowWnd != NULL )
		m_pShowWnd->RedrawWindow();
}
void CTabDlgBar::TabReSize()
{
	CRect rect;
	m_TabCtrl.GetClientRect(rect);
	m_TabCtrl.AdjustRect(FALSE, &rect);

	for ( int n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		TC_ITEM tci;
		tci.mask = TCIF_PARAM;
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		CWnd *pWnd = (CWnd *)tci.lParam;
		pWnd->SetWindowPos(&wndTop , rect.left, rect.top, rect.Width(), rect.Height(), 
			SWP_NOACTIVATE | (pWnd == m_pShowWnd ? SWP_SHOWWINDOW : 0));
	}
}
CSize CTabDlgBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CSize size;

	CMainFrame *pMain;
	CRect MainRect(0, 0, 32767, 32767);

	if ( (pMain = (CMainFrame *)::AfxGetMainWnd()) != NULL ) {
		pMain->GetClientRect(MainRect);

		if ( m_InitSize.cx <= 0 )
			m_InitSize.cx = MainRect.Height() * ::AfxGetApp()->GetProfileInt(_T("TabDlgBar"), _T("IntWidth"),  20) / 100;

		if ( m_InitSize.cy <= 0 )
			m_InitSize.cy = MainRect.Height() * ::AfxGetApp()->GetProfileInt(_T("TabDlgBar"), _T("IntHeight"), 20) / 100;

		pMain->GetCtrlBarRect(MainRect, this);
		MainRect.right += (::GetSystemMetrics(SM_CXBORDER) * 2);
	}

	size.cx = (bHorz ? MainRect.Width() : m_InitSize.cx);
	size.cy = (bHorz ? m_InitSize.cy    : MainRect.Height());

	if ( IsVisible() && m_TabCtrl.m_hWnd != NULL ) {
		CRect oldr, rect(CPoint(0, 0), size);
		CalcInsideRect(rect, bHorz);
		m_TabCtrl.GetWindowRect(oldr);

		if ( oldr != rect ) {
			m_TabCtrl.SetWindowPos(&wndTop , rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
			TabReSize();
		}
	}

	return size;
}
void CTabDlgBar::Add(CWnd *pWnd, int nImage)
{
	int n;
	TC_ITEM tci;
	CString title;
	CRect rect;

	pWnd->GetWindowText(title);

	if ( title.GetLength() >= MAX_PATH )
		title = title.Left(MAX_PATH -1);

	tci.mask    = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	tci.pszText = (LPWSTR)(LPCWSTR)TstrToUni(title);
	tci.lParam  = (LPARAM)pWnd;
	tci.iImage  = nImage;

	n = m_TabCtrl.GetItemCount();
	m_TabCtrl.InsertItem(n, &tci);
	
	m_TabCtrl.GetClientRect(rect);
	m_TabCtrl.AdjustRect(FALSE, rect);

	struct _DlgWndData *pData = new struct _DlgWndData;
	pData->pWnd    = pWnd;
	pData->nImage  = nImage;
	pData->pParent = pWnd->GetParent();
	pData->hMenu   = pWnd->GetMenu()->GetSafeHmenu();
	pWnd->GetWindowRect(pData->WinRect);
	m_Data.Add(pData);

	pWnd->SetParent(&m_TabCtrl);
	pWnd->ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_POPUP, WS_CHILD);
	pWnd->SetMenu(NULL);

	if ( m_pShowWnd != NULL )
		m_pShowWnd->ShowWindow(SW_HIDE);

	pWnd->SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(),
			SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS | SWP_DEFERERASE);

	m_TabCtrl.SetCurSel(n);
	m_pShowWnd = pWnd;
}
void CTabDlgBar::Del(CWnd *pWnd)
{
	int n;
	TC_ITEM tci;

	if ( m_pShowWnd == pWnd ) {
		m_pShowWnd->ShowWindow(SW_HIDE);
		m_pShowWnd = NULL;
	}

	tci.mask = TCIF_PARAM;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		if ( tci.lParam != (LPARAM)pWnd )
			continue;

		m_TabCtrl.DeleteItem(n);

		if ( m_pShowWnd == NULL ) {
			if ( n >= m_TabCtrl.GetItemCount() )
				n--;
			if ( n >= 0 ) {
				m_TabCtrl.SetCurSel(n);
				if ( m_TabCtrl.GetItem(n, &tci) ) {
					m_pShowWnd = (CWnd *)tci.lParam;
					m_pShowWnd->ShowWindow(SW_SHOWNOACTIVATE);
				}
			}
		} else
			m_pShowWnd->RedrawWindow();

		break;
	}

	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		struct _DlgWndData *pData = (struct _DlgWndData *)m_Data[n];
		if ( pData->pWnd == pWnd ) {
			m_Data.RemoveAt(n);
			delete pData;
			break;
		}
	}
}
void CTabDlgBar::Sel(CWnd *pWnd)
{
	int n;
	TC_ITEM tci;

	tci.mask = TCIF_PARAM;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		if ( tci.lParam != (LPARAM)pWnd )
			continue;
		if ( m_TabCtrl.GetCurSel() == n )
			return;

		m_TabCtrl.SetCurSel(n);

		if ( m_pShowWnd != NULL )
			m_pShowWnd->ShowWindow(SW_HIDE);

		m_pShowWnd = (CWnd *)tci.lParam;
		m_pShowWnd->ShowWindow(SW_SHOWNOACTIVATE);
	}
}
BOOL CTabDlgBar::IsInside(CWnd *pWnd)
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		struct _DlgWndData *pData = (struct _DlgWndData *)m_Data[n];
		if ( pData->pWnd->GetSafeHwnd() == pWnd->GetSafeHwnd() )
			return TRUE;
	}

	return FALSE;
}
void *CTabDlgBar::RemoveAt(int idx, CPoint point)
{
	TC_ITEM tci;
	
	ZeroMemory(&tci, sizeof(tci));
	tci.mask = TCIF_PARAM;

	if ( !m_TabCtrl.GetItem(idx, &tci) )
		return NULL;

	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		struct _DlgWndData *pData = (struct _DlgWndData *)m_Data[n];

		if ( pData->pWnd != (CWnd *)tci.lParam )
			continue;

		point.x -= pData->WinRect.Width() / 2;
		point.y -= GetSystemMetrics(SM_CYCAPTION) / 2;

		pData->pWnd->SetParent(pData->pParent);
		pData->pWnd->ModifyStyle(WS_CHILD, WS_CAPTION | WS_THICKFRAME | WS_POPUP, SWP_HIDEWINDOW);
		pData->pWnd->SetMenu(CMenu::FromHandle(pData->hMenu));
		ExDwmDarkMode(pData->pWnd->GetSafeHwnd());
		pData->pWnd->SetWindowPos(NULL, point.x, point.y, pData->WinRect.Width(), pData->WinRect.Height(),
			SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS | SWP_DEFERERASE);

		if ( pData->pWnd == m_pShowWnd )
			m_pShowWnd = NULL;

		m_Data.RemoveAt(n);
		m_TabCtrl.DeleteItem(idx);

		if ( idx >= m_TabCtrl.GetItemCount() )
			idx--;

		if ( idx >= 0 ) {
			m_TabCtrl.SetCurSel(idx);
			tci.mask = TCIF_PARAM;
			if ( m_TabCtrl.GetItem(idx, &tci) ) {
				if ( m_pShowWnd != NULL )
					m_pShowWnd->ShowWindow(SW_HIDE);
				m_pShowWnd = (CWnd *)tci.lParam;
				m_pShowWnd->ShowWindow(SW_SHOWNOACTIVATE);
			}
		}

		return pData;
	}

	return NULL;
}
void CTabDlgBar::RemoveAll()
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		struct _DlgWndData *pData = (struct _DlgWndData *)m_Data[n];

		pData->pWnd->SetParent(pData->pParent);
		pData->pWnd->ModifyStyle(WS_CHILD, WS_CAPTION | WS_THICKFRAME | WS_POPUP, SWP_HIDEWINDOW);
		pData->pWnd->SetMenu(CMenu::FromHandle(pData->hMenu));
		ExDwmDarkMode(pData->pWnd->GetSafeHwnd());
		pData->pWnd->SetWindowPos(NULL, pData->WinRect.left, pData->WinRect.top, pData->WinRect.Width(), pData->WinRect.Height(),
			SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS | SWP_DEFERERASE);
		delete pData;
	}

	m_Data.RemoveAll();
	m_TabCtrl.DeleteAllItems();
	m_pShowWnd = NULL;
}
void CTabDlgBar::FontSizeCheck()
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
}
void CTabDlgBar::DpiChanged()
{
	CRect rect;

	FontSizeCheck();

	m_TabCtrl.GetClientRect(rect);
	m_TabCtrl.AdjustRect(FALSE, rect);

	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		struct _DlgWndData *pData = (struct _DlgWndData *)m_Data[n];
		pData->pWnd->SendMessage(WM_DPICHANGED, MAKEWPARAM(SCREEN_DPI_X, SCREEN_DPI_Y), (LPARAM)((RECT *)rect));
	}
}

BEGIN_MESSAGE_MAP(CTabDlgBar, CControlBarEx)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABDLGBAR_TAB, &CTabDlgBar::OnSelchange)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

int CTabDlgBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CControlBarEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect; rect.SetRectEmpty();

	if ( !m_TabCtrl.Create(WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_BOTTOM | TCS_FORCELABELLEFT, rect, this, IDC_TABDLGBAR_TAB) )
		return (-1);

	m_FontName  = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	m_FontSize = MulDiv(::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9), SCREEN_DPI_Y, SYSTEM_DPI_Y);

	if ( m_FontName.IsEmpty() || !m_TabFont.CreatePointFont(m_FontSize * 10, m_FontName) ) {
		CFont *font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
		m_TabCtrl.SetFont(font);
		SetFont(font);
	} else {
		m_TabCtrl.SetFont(&m_TabFont);
		SetFont(&m_TabFont);
	}

	SetBorders(2, 5, 2, 4);

	CBitmapEx BitMap;
	BitMap.LoadResBitmap(IDB_BITMAP4, SCREEN_DPI_X, SCREEN_DPI_Y, RGB(192, 192, 192));
	m_ImageList.Create(MulDiv(16, SCREEN_DPI_X, DEFAULT_DPI_X), MulDiv(16, SCREEN_DPI_Y, DEFAULT_DPI_Y), ILC_COLOR24 | ILC_MASK, 0, 4);
	m_ImageList.Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();
	m_TabCtrl.SetImageList(&m_ImageList);

	return 0;
}

void CTabDlgBar::OnSize(UINT nType, int cx, int cy)
{
	CControlBarEx::OnSize(nType, cx, cy);

	if ( m_TabCtrl.m_hWnd == NULL )
		return;

	CRect oldr, rect(0, 0, cx, cy);
	CalcInsideRect(rect, (GetBarStyle() & (CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM)) != 0 ? TRUE : FALSE);
	m_TabCtrl.GetWindowRect(oldr);

	if ( oldr != rect ) {
		m_TabCtrl.SetWindowPos(&wndTop , rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW);
		TabReSize();
	}
}
void CTabDlgBar::OnSelchange(NMHDR *pNMHDR, LRESULT *pResult) 
{
	int n;
	TC_ITEM tci;
	CWnd *pWnd;

	*pResult = 0;
	tci.mask = TCIF_PARAM;

	if ( (n = m_TabCtrl.GetCurSel()) < 0 || !m_TabCtrl.GetItem(n, &tci) )
		return;

	if ( m_pShowWnd != NULL )
		m_pShowWnd->ShowWindow(SW_HIDE);

	pWnd = (CWnd *)tci.lParam;
	pWnd->ShowWindow(SW_SHOWNOACTIVATE);
	m_pShowWnd = pWnd;
}
BOOL CTabDlgBar::PreTranslateMessage(MSG* pMsg)
{
	if ( pMsg->hwnd == m_TabCtrl.m_hWnd && (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST) ) {
		// WM_MOUSE*をTabCtrlからTabBarに変更
		CPoint point(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
		::ClientToScreen(pMsg->hwnd, &point);
		ScreenToClient(&point);
		pMsg->hwnd = m_hWnd;
		pMsg->lParam = MAKELPARAM(point.x, point.y);
	}

	return CControlBarEx::PreTranslateMessage(pMsg);
}
int CTabDlgBar::HitPoint(CPoint point)
{
	CRect rect, inside;

	if ( (GetStyle() & WS_VISIBLE) == 0 ) {
		((CMainFrame *)AfxGetMainWnd())->GetWindowRect(rect);
		return rect.PtInRect(point) ? (-6) : (-7);
	}

	GetWindowRect(rect);
	if ( !rect.PtInRect(point) ) {
		((CMainFrame *)AfxGetMainWnd())->GetWindowRect(rect);
		return rect.PtInRect(point) ? (-6) : (-7);		// (-6) CTabDlgBar外 (-7) CMainFrame外
	}

	inside = rect;

	switch(GetBarStyle() & CBRS_ALIGN_ANY) {
	case CBRS_ALIGN_LEFT:
		CalcInsideRect(inside, FALSE);
		rect.left = inside.right;
		if ( rect.PtInRect(point) )
			return (-1);
		break;
	case CBRS_ALIGN_TOP:
		CalcInsideRect(inside, TRUE);
		rect.top = inside.bottom;
		if ( rect.PtInRect(point) )
			return (-2);
		break;
	case CBRS_ALIGN_RIGHT:
		CalcInsideRect(inside, FALSE);
		rect.right = inside.left;
		if ( rect.PtInRect(point) )
			return (-3);
		break;
	case CBRS_ALIGN_BOTTOM:
		CalcInsideRect(inside, TRUE);
		rect.bottom = inside.top;
		if ( rect.PtInRect(point) )
			return (-4);
		break;
	}

	// CTabBarクライアント座標
	m_TabCtrl.ScreenToClient(&point);

	for ( int n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		if ( m_TabCtrl.GetItemRect(n, rect) && rect.PtInRect(point) )
			return n;				// タブ内
	}

	return (-5);					// CTabDlgBar内
}
void CTabDlgBar::GetTitle(int nIndex, CString &title)
{
	TC_ITEM tci;
	TCHAR tmp[MAX_PATH + 2];

	ZeroMemory(&tci, sizeof(tci));
	tci.mask = TCIF_TEXT | TCIF_PARAM;
	tci.pszText = tmp;
	tci.cchTextMax = MAX_PATH;

	if ( !m_TabCtrl.GetItem(nIndex, &tci) )
		return;

	title = tci.pszText;
}

void CTabDlgBar::TrackLoop(CPoint ptScrn, int idx, CWnd *pMoveWnd, int nImage)
{
	int n;
	int hit;
	int count;
	int TypeCol = 0;
	MSG msg;
	CPoint ptOffset, ptLast;
	CRect TrackRect, TrackBase;
	CRect MainClient, MainFrame;
	CSize InitSize, MoveOfs;
	CTrackWnd track;
	CString title;
	TC_ITEM tci;
	TCHAR Text[MAX_PATH + 2];
	BOOL bDrag = FALSE;
	BOOL bReSize = FALSE;
	BOOL bGetRect = FALSE;
	BOOL bIdle = FALSE;

	if ( !IsFloating() && idx <= (-1) && idx >= (-4) )
		bReSize = TRUE;
	else if ( idx < 0 && pMoveWnd == NULL )
		return;

	if ( pMoveWnd != NULL ) {
		CRect rect;
		pMoveWnd->GetWindowRect(rect);
		MoveOfs.cx = ptScrn.x - rect.left;
		MoveOfs.cy = ptScrn.y - rect.top;
		pMoveWnd->SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
	}

	SetCapture();
	ptLast = ptScrn;

	while ( CWnd::GetCapture() == this ) {
		for ( count = 0 ; bIdle || !::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE) ; count++ ) {
			bIdle = FALSE;
			if ( !((CRLoginApp *)AfxGetApp())->OnIdle(count) )
				break;
		}

		if (  !::GetMessage(&msg, NULL, 0, 0) )
			goto ENDOF;

		switch (msg.message) {
		case WM_MOUSEMOVE:
			if ( ::PeekMessage(&msg, NULL, NULL, NULL, PM_QS_PAINT | PM_NOREMOVE) )
				break;
		case WM_LBUTTONUP:
			ptScrn = msg.pt;
			ptOffset = ptScrn - ptLast;

			if ( !bDrag && (abs(ptOffset.x) >= ::GetSystemMetrics(SM_CXDRAG) || abs(ptOffset.y) >= ::GetSystemMetrics(SM_CYDRAG)) ) {
				if ( bReSize ) {
					GetWindowRect(TrackRect);
					TrackBase = TrackRect;
					InitSize = m_InitSize;
					((CMainFrame *)AfxGetMainWnd())->GetClientRect(MainClient);
					MainFrame = ((CMainFrame *)AfxGetMainWnd())->m_Frame;
					((CMainFrame *)AfxGetMainWnd())->ClientToScreen(MainClient);
					((CMainFrame *)AfxGetMainWnd())->ClientToScreen(MainFrame);

				} else if ( idx >= 0 ) {
					m_TabCtrl.GetItemRect(idx, TrackRect);
					m_TabCtrl.ClientToScreen(TrackRect);
					GetTitle(idx, title);

					track.Create(NULL, title, WS_TILED | WS_CHILD, TrackRect, CWnd::GetDesktopWindow(), (-1));
					bGetRect = TRUE;
				}

				bDrag = TRUE;
			}

			if ( bDrag ) {
				if ( bReSize ) {
					switch(idx) {
					case (-1):	// CBRS_ALIGN_LEFT
						TrackRect.right = TrackBase.right + (ptScrn.x - ptLast.x);
						if ( TrackRect.right < (n = MainClient.left + MainClient.Width() / 10) )
							TrackRect.right = n;
						else if ( TrackRect.right > (n = MainFrame.right - PANEMIN_WIDTH) )
							TrackRect.right = n;
						InitSize.cx = TrackRect.Width();
						break;
					case (-2):	// CBRS_ALIGN_TOP
						TrackRect.bottom = TrackBase.bottom + (ptScrn.y - ptLast.y);
						if ( TrackRect.bottom < (n = MainClient.top + MainClient.Height() / 10) )
							TrackRect.bottom = n;
						else if ( TrackRect.bottom > (n = MainFrame.bottom - PANEMIN_HEIGHT) )
							TrackRect.bottom = n;
						InitSize.cy = TrackRect.Height();
						break;
					case (-3):	// CBRS_ALIGN_RIGHT
						TrackRect.left = TrackBase.left + (ptScrn.x - ptLast.x);
						if ( TrackRect.left < (n = MainFrame.left + PANEMIN_WIDTH) )
							TrackRect.left = n;
						else if ( TrackRect.left > (n = MainClient.right - MainClient.Width() / 10) )
							TrackRect.left = n;
						InitSize.cx = TrackRect.Width();
						break;
					case (-4):	// CBRS_ALIGN_BOTTOM
						TrackRect.top = TrackBase.top + (ptScrn.y - ptLast.y);
						if ( TrackRect.top < (n = MainFrame.top + PANEMIN_HEIGHT) )
							TrackRect.top = n;
						else if ( TrackRect.top > (n = MainClient.bottom - MainClient.Height() / 10) )
							TrackRect.top = n;
						InitSize.cy = TrackRect.Height();
						break;
					}

					if ( m_InitSize != InitSize ) {
						m_InitSize = InitSize;
						Invalidate(FALSE);
						((CMainFrame *)AfxGetMainWnd())->RecalcLayout(TRUE);
					}

					if ( msg.message == WM_LBUTTONUP ) {
						::AfxGetApp()->WriteProfileInt(_T("TabDlgBar"), _T("IntWidth"),  m_InitSize.cx * 100 / MainClient.Width());
						::AfxGetApp()->WriteProfileInt(_T("TabDlgBar"), _T("IntHeight"), m_InitSize.cy * 100 / MainClient.Height());
						goto ENDOF;
					}

				} else {
					TrackRect.OffsetRect(ptOffset);
					ptLast = ptScrn;

					hit = HitPoint(ptScrn);

					if ( hit < (-5) ) {
						if ( track.GetSafeHwnd() != NULL ) {
							track.DestroyWindow();

							struct _DlgWndData *pData = (struct _DlgWndData *)RemoveAt(idx, ptScrn);
							if ( pData != NULL ) {
								pMoveWnd = pData->pWnd;
								nImage = pData->nImage;
								MoveOfs.cx = pData->WinRect.Width() / 2;
								MoveOfs.cy = GetSystemMetrics(SM_CYCAPTION) / 2;
								delete pData;
							}

							idx = m_TabCtrl.GetItemCount() > 0 ? m_TabCtrl.GetCurSel() : (-1);
							bGetRect = FALSE;

						} else if ( pMoveWnd != NULL ) {
							if ( hit == (-6) && (GetStyle() & WS_VISIBLE) == 0 )
								((CMainFrame *)::AfxGetMainWnd())->TabDlgShow(TRUE);

							pMoveWnd->SetWindowPos(NULL, ptScrn.x - MoveOfs.cx, ptScrn.y - MoveOfs.cy, 0, 0,
								SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
						}

					} else {
						if ( track.GetSafeHwnd() == NULL ) {
							if ( pMoveWnd != NULL ) {
								Add(pMoveWnd, nImage);
								idx = m_TabCtrl.GetItemCount() - 1;
								pMoveWnd = NULL;
								bGetRect = FALSE;
							}

							if ( idx >= 0 && !bGetRect ) {
								m_TabCtrl.GetItemRect(idx, TrackRect);
								m_TabCtrl.ClientToScreen(TrackRect);
								GetTitle(idx, title);

								ptOffset.x = ptScrn.x - TrackRect.left - TrackRect.Width() / 2;
								ptOffset.y = ptScrn.y - TrackRect.top  - TrackRect.Height() / 2;
								TrackRect.OffsetRect(ptOffset);

								track.Create(NULL, title, WS_TILED | WS_CHILD, TrackRect, CWnd::GetDesktopWindow(), (-1));
								bGetRect = TRUE;
							}
						}

						if ( track.GetSafeHwnd() != NULL ) {
							TypeCol = hit >= 0 || hit == (-7) ? 0 : 5;
							track.SetWindowPos(&wndTopMost, TrackRect.left, TrackRect.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_SHOWWINDOW);

							if ( track.m_TypeCol != TypeCol ) {
								track.m_TypeCol = TypeCol;
								track.Invalidate();
							}
						}
					}

					if ( msg.message == WM_LBUTTONUP ) {
						if ( idx >= 0 && hit >= 0 && hit != idx ) {		// Move Tab
							tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
							tci.pszText = Text;
							tci.cchTextMax = MAX_PATH;
							m_TabCtrl.GetItem(idx, &tci);

							m_TabCtrl.DeleteItem(idx);
							tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
							m_TabCtrl.InsertItem(hit, &tci);

							idx = hit;

							if ( m_pShowWnd != NULL )
								m_pShowWnd->Invalidate();

						} else if ( hit < (-5) ) {
							if ( m_TabCtrl.GetItemCount() == 0 )
								((CMainFrame *)::AfxGetMainWnd())->TabDlgShow(FALSE);
						}
						goto ENDOF;
					}
				}

			} else if ( msg.message == WM_LBUTTONUP )
				goto ENDOF;

			break;

		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if ( ((CRLoginApp *)::AfxGetApp())->IsIdleMessage(&msg) )
				bIdle = TRUE;
			break;
		}
	}

ENDOF:
	if ( track.GetSafeHwnd() != NULL )
		track.DestroyWindow();

	ReleaseCapture();
}
void CTabDlgBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	int idx;
	CPoint ptScrn;
	TC_ITEM tci;

	ptScrn = point;
	ClientToScreen(&ptScrn);

	if ( (idx = HitPoint(ptScrn)) < 0 ) {
		if ( idx == (-5) ) {
			CControlBar::OnLButtonDown(nFlags, point);
			return;
		} else if ( idx <= (-6) ) {
			CWnd::OnLButtonDown(nFlags, point);
			return;
		}

	} else if ( idx != m_TabCtrl.GetCurSel() ) {
		m_TabCtrl.SetCurSel(idx);
		tci.mask = TCIF_PARAM;
		if ( m_TabCtrl.GetItem(idx, &tci) ) {
			if ( m_pShowWnd != NULL )
				m_pShowWnd->ShowWindow(SW_HIDE);
			m_pShowWnd = (CWnd *)tci.lParam;
			m_pShowWnd->ShowWindow(SW_SHOWNOACTIVATE);
		}
	}

	TrackLoop(ptScrn, idx, NULL, (-1));
}

BOOL CTabDlgBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint point;
	CRect rect, inside;
	HCURSOR hCursor;
	LPCTSTR pCurName = NULL;

	if ( (GetStyle() & WS_VISIBLE) != 0 && !IsFloating() ) {
		GetCursorPos(&point);
		GetWindowRect(rect);
		inside = rect;

		switch(GetBarStyle() & CBRS_ALIGN_ANY) {
		case CBRS_ALIGN_LEFT:
			CalcInsideRect(inside, FALSE);
			rect.left = inside.right;
			pCurName = IDC_SIZEWE;
			break;
		case CBRS_ALIGN_TOP:
			CalcInsideRect(inside, TRUE);
			rect.top = inside.bottom;
			pCurName = IDC_SIZENS;
			break;
		case CBRS_ALIGN_RIGHT:
			CalcInsideRect(inside, FALSE);
			rect.right = inside.left;
			pCurName = IDC_SIZEWE;
			break;
		case CBRS_ALIGN_BOTTOM:
			CalcInsideRect(inside, TRUE);
			rect.bottom = inside.top;
			pCurName = IDC_SIZENS;
			break;
		}

		if ( pCurName != NULL && rect.PtInRect(point) && (hCursor = AfxGetApp()->LoadStandardCursor(pCurName)) != NULL ) {
			::SetCursor(hCursor);
			return TRUE;
		}
	}

	return CControlBarEx::OnSetCursor(pWnd, nHitTest, message);
}
