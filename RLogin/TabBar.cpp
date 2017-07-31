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

#define OBM_CLOSE           32754

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CTabBar::CTabBar()
{
	m_GhostReq = m_GhostItem = (-1);
	m_pGhostView = NULL;
	m_bNumber = FALSE;
	m_ImageCount = 0;
}

CTabBar::~CTabBar()
{
}

IMPLEMENT_DYNAMIC(CTabBar, CControlBar)

BEGIN_MESSAGE_MAP(CTabBar, CControlBar)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
	ON_NOTIFY(TCN_SELCHANGE, IDC_MDI_TAB_CTRL, OnSelchange)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////

BOOL CTabBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	m_dwStyle = (dwStyle & CBRS_ALL);

	dwStyle &= ~CBRS_ALL;
	dwStyle |= CCS_NOPARENTALIGN|CCS_NOMOVEY|CCS_NODIVIDER|CCS_NORESIZE;
	if (pParentWnd->GetStyle() & WS_THICKFRAME) dwStyle |= SBARS_SIZEGRIP;

	CRect rect; rect.SetRectEmpty();
	return CWnd::Create(STATUSCLASSNAME, NULL, dwStyle, rect, pParentWnd, nID);
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

//////////////////////////////////////////////////////////////////////
// CTabBar メッセージ ハンドラ

int CTabBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CControlBar::OnCreate(lpCreateStruct) == (-1) )
		return (-1);
	
	CRect rect; rect.SetRectEmpty();
	if ( !m_TabCtrl.Create(WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_FIXEDWIDTH | TCS_FORCELABELLEFT | TCS_MULTILINE, rect, this, IDC_MDI_TAB_CTRL) ) {
		TRACE0("Unable to create tab control bar\n");
		return (-1);
	}

	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = ::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9);	

	if ( FontName.IsEmpty() || !m_TabFont.CreatePointFont(FontSize * 10, FontName) ) {
		CFont *font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
		m_TabCtrl.SetFont(font);
		SetFont(font);

	} else {
		m_TabCtrl.SetFont(&m_TabFont);
		SetFont(&m_TabFont);
	}

//	m_TabCtrl.SetPadding(CSize(2, 3));
	m_TabCtrl.SetMinTabWidth(16);

	m_ImageList.Create(ICONIMG_SIZE, ICONIMG_SIZE, ILC_COLOR24 | ILC_MASK, 4, 4);

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
	int n, idx;
	TC_ITEM tci, ntc;
	CString title;
	TCHAR tmp[MAX_PATH + 2];
	CChildFrame *pWnd;
	CMDIFrameWnd *pMainframe = ((CMDIFrameWnd *)AfxGetMainWnd());
	CMDIChildWnd* pActive = (pMainframe == NULL ? NULL : pMainframe->MDIGetActive(NULL));
	CRLoginDoc *pDoc;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
		tci.pszText = tmp;
		tci.cchTextMax = MAX_PATH;

		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;

		ntc.mask = 0;
		pWnd = (CChildFrame *)FromHandle((HWND)tci.lParam);

		pWnd->GetWindowText(title);
		if ( title.Compare(tmp) != 0 ) {
			ntc.mask |= TCIF_TEXT;
			ntc.pszText = tmp;
			_tcsncpy(tmp, title, MAX_PATH);
		}

		if ( (pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument())) != NULL && !pDoc->m_ServerEntry.m_IconName.IsEmpty() )
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

		if ( ntc.mask != 0 ) {
			// SetItemではiImageを設定するとテキストと重なるバグあり? しかたなくInsert/Deleteで代用
			if ( (ntc.mask & TCIF_IMAGE) != 0 ) {
				ntc.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
				ntc.pszText = tmp;
				ntc.lParam = tci.lParam;
				m_TabCtrl.InsertItem(n, &ntc);
				m_TabCtrl.DeleteItem(n + 1);
			} else
				m_TabCtrl.SetItem(n, &ntc);
		}

		if ( pActive != NULL && pActive->m_hWnd == pWnd->m_hWnd )
			m_TabCtrl.SetCurSel(n);
	}

	ReSize();
}

BOOL CTabBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	int n;
	CRect rect;
	TCHITTESTINFO htinfo;

	if ( AfxGetApp()->GetProfileInt(_T("TabBar"), _T("GhostWnd"), 0) )
		return CControlBar::OnSetCursor(pWnd, nHitTest, message);

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

	if ( pWnd == NULL )
		return TRUE;

	return CControlBar::OnSetCursor(pWnd, nHitTest, message);
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
		OnSetCursor(NULL, 0, 0);
		if ( m_GhostItem >= 0 && (GetStyle() & WS_VISIBLE) == 0 )
			SetGhostWnd(FALSE);
		break;
	}
	CControlBar::OnTimer(nIDEvent);
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
	int idx = m_TabCtrl.GetCurSel();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();

	CControlBar::OnLButtonDblClk(nFlags, point);

	if ( idx < 0 || !m_TabCtrl.GetItemRect(idx, rect) || !rect.PtInRect(point) )
		pMain->PostMessage(WM_COMMAND, (WPARAM)ID_FILE_NEW);
}

void CTabBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int hit = (-3);
	int TypeCol = 0;
	BOOL bMove = FALSE;
	BOOL bOtherMove;
	BOOL bCmdLine;
	CPoint capos;
	CSize size(4, 4);
	CRect rect, rectFirst, rectLast;
	int idx = m_TabCtrl.GetCurSel();
	int offset;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CWnd *pDeskTop = CWnd::GetDesktopWindow();
	CChildFrame *pChild;
	CRLoginDoc *pDoc;
	TC_ITEM tci;
	TCHAR Text[MAX_PATH + 2];
	CTrackWnd track;
	CString title;
	MSG msg;
	clock_t stc = clock() - (CLOCKS_PER_SEC * 2);

	CControlBar::OnLButtonDown(nFlags, point);

	if ( idx < 0 || !m_TabCtrl.GetItemRect(idx, rect) || !rect.PtInRect(point) )
		return;

	if ( (pChild = (CChildFrame *)GetAt(idx)) == NULL || (pDoc = (CRLoginDoc*)pChild->GetActiveDocument()) == NULL )
		return;

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

	// CTabBar = point 
	// m_TabCtrl = rect
	offset = rect.top - point.y - 1;

	ClientToScreen(rect);
	pDeskTop->ScreenToClient(rect);
	rectFirst = rectLast = rect;

	GetTitle(idx, title);
	track.Create(NULL, title, WS_TILED | WS_CHILD, rect, pDeskTop, (-1));
	track.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	for ( ; ; ) {
		if ( CWnd::GetCapture() != this ) {
			track.DestroyWindow();
			return;
		}

		::GetMessage(&msg, NULL, 0, 0);

		switch (msg.message) {
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
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
				TypeCol = 0;

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

			if ( msg.message != WM_LBUTTONUP )
				break;

			track.DestroyWindow();
			ReleaseCapture();

			if ( m_bNumber )
				SetTabTitle(FALSE);

			ClientToScreen(&capos);

			if ( hit == (-1) ) {						// Move Pane
				pMain->ScreenToClient(&capos);
				pMain->MoveChild(pChild, capos);
				pMain->PostMessage(WM_COMMAND, IDM_DISPWINIDX);

			} else if ( hit == (-2) && bOtherMove ) {	// Other RLogin Exec
				capos.y = capos.y - GetSystemMetrics(SM_CYCAPTION) / 2;
				pApp->OpenRLogin(pDoc, &capos);
			}

			return;

		//case WM_PAINT:
		//case WM_ERASEBKGND:
		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			break;
		}
	}
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
		COLORREF bc = GetSysColor(COLOR_WINDOW);

		if ( image.LoadFile(filename) && (pBitmap = image.GetBitmap(&dc, ICONIMG_SIZE, ICONIMG_SIZE, bc)) != NULL )
			idx = m_ImageList.Add(pBitmap, bc);

		m_ImageFile[filename].m_Value = idx;
		return idx;

	} else
		return pNode->m_Value;
}
void CTabBar::Add(CWnd *pWnd)
{
	TC_ITEM tci;
	CString title;

	pWnd->GetWindowText(title);
	tci.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	tci.pszText = title.LockBuffer();
	tci.lParam = (LPARAM)(pWnd->m_hWnd);
	tci.iImage = (-1);

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
void CTabBar::GetTitle(int nIndex, CString &title)
{
	TC_ITEM tci;
	TCHAR tmp[MAX_PATH + 2];

	tci.mask = TCIF_TEXT;
	tci.pszText = tmp;
	tci.cchTextMax = MAX_PATH;

	if ( !m_TabCtrl.GetItem(nIndex, &tci) )
		return;

	if ( m_bNumber )
		title = tci.pszText;
	else
		title.Format(_T("%d %s"), nIndex + 1, tci.pszText);
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
	width = (rect.Width() - 18) / n;

	m_TabCtrl.GetItemRect(0, rect);
	if ( width == rect.Width() )
		return;

	sz.cx = (width > 16 ? width : 16);
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

int CTabBar::HitPoint(CPoint point)
{
	CRect rect;
	CWnd *pMain = AfxGetMainWnd();

	ClientToScreen(&point);

	pMain->GetWindowRect(rect);

	if ( !rect.PtInRect(point) )
		return (-2);				// Close

	m_TabCtrl.GetWindowRect(rect);

	if ( rect.PtInRect(point) ) {
		ScreenToClient(&point);
		for ( int n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
			if ( m_TabCtrl.GetItemRect(n, rect) && rect.PtInRect(point) )
				return n;			// Move Tab
		}

	} else {
		pMain->GetClientRect(rect);
		pMain->ClientToScreen(rect);

		if ( rect.PtInRect(point) )
			return (-1);			// Move Pane
	}

	return (-3);
}
void CTabBar::SetTabTitle(BOOL bNumber)
{
	int n;
	TC_ITEM tci;
	CString title, work;
	TCHAR tmp[MAX_PATH + 2];
	CWnd *pWnd;

	m_bNumber = bNumber;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.pszText = tmp;
		tci.cchTextMax = MAX_PATH;

		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;

		pWnd = FromHandle((HWND)tci.lParam);
		pWnd->GetWindowText(work);

		if ( m_bNumber )
			title.Format(_T("%d %s"), n + 1, work);
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

		if ( !pMain->IsOverLap((HWND)tci.lParam) )
			return;

		m_GhostItem = m_GhostReq;
		SetTimer(1025, 200, NULL);

		if ( (pFrame = (CChildFrame *)GetAt(m_GhostItem)) != NULL && (m_pGhostView = (CRLoginView *)pFrame->GetActiveView()) != NULL )
			m_pGhostView->SetGhostWnd(TRUE);

	} else {		// Destory Ghost Wnd
		if ( m_GhostItem >= 0 )
			KillTimer(1025);

		if ( m_pGhostView != NULL )
			m_pGhostView->SetGhostWnd(FALSE);

		m_GhostItem = (-1);
		m_pGhostView = NULL;
	}
}
