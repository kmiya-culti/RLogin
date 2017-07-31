// ChildFrm.cpp : CChildFrame クラスの実装
//
#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "Script.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_WINDOW_CLOSE, OnWindowClose)
	ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()


// CChildFrame コンストラクション/デストラクション

CChildFrame::CChildFrame()
{
	m_Width  = 10;
	m_Height = 10;
	m_Cols   = 80;
	m_Lines  = 25;
	m_VScrollFlag = FALSE;
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	BOOL result;

	result = m_wndSplitter.Create(this, 2, 1, CSize(40, 20), pContext, WS_CHILD | WS_VISIBLE | WS_VSCROLL | SPLS_DYNAMIC_SPLIT);

	if ( (m_VScrollFlag = ((CMainFrame *)AfxGetMainWnd())->m_ScrollBarFlag) == FALSE && result ) {
		m_wndSplitter.SetScrollStyle(0);
		m_wndSplitter.RecalcLayout();
	}

	return result;
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: CREATESTRUCT cs を変更して、Window クラスまたはスタイルを変更します。

	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style  = WS_CHILD | WS_VISIBLE | FWS_ADDTOTITLE;
//	cs.style |= WS_OVERLAPPED | WS_SYSMENU;
//	cs.style |= WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

//	cs.style &= ~(WS_BORDER | WS_DLGFRAME);
//	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

//	if ( AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("Style"), 1) == 1 )
//		cs.style |= WS_MAXIMIZE;
//	else {
//		cs.x  = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("x"), cs.x);
//		cs.y  = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("y"), cs.y);
//		cs.cx = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("cx"), cs.cx);
//		cs.cy = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("cy"), cs.cy);
//	}

	CRect rect;
	((CMainFrame *)AfxGetMainWnd())->GetFrameRect(rect);
	cs.cx = rect.Width();
	cs.cy = rect.Height();

	return TRUE;
}

void CChildFrame::ActivateFrame(int nCmdShow)
{
//	if ( AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("Style"), 1) == 1 )
//		nCmdShow = SW_SHOWMAXIMIZED;
	CMDIChildWnd::ActivateFrame(nCmdShow);
}

void CChildFrame::SetScrollBar(BOOL flag)
{
	if ( m_VScrollFlag != flag ) {
		m_VScrollFlag = flag;
		m_wndSplitter.SetScrollStyle(flag ? WS_VSCROLL : 0);
		m_wndSplitter.RecalcLayout();
	}
}

// CChildFrame 診断

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG


// CChildFrame メッセージ ハンドラ

void CChildFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

//	if ( nType != SIZE_MINIMIZED )
//		AfxGetApp()->WriteProfileInt(_T("ChildFrame"), _T("Style"), (nType == SIZE_MAXIMIZED ? 1 : 0));
}

int CChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	((CMainFrame *)AfxGetMainWnd())->AddChild(this);

	return 0;
}

void CChildFrame::OnDestroy() 
{
//	if ( !IsIconic() && !IsZoomed() ) {
//		CRect rect;
//		GetWindowRect(&rect);
//		AfxGetApp()->WriteProfileInt(_T("ChildFrame"), _T("x"), rect.left);
//		AfxGetApp()->WriteProfileInt(_T("ChildFrame"), _T("y"), rect.top);
//		AfxGetApp()->WriteProfileInt(_T("ChildFrame"), _T("cx"), rect.Width());
//		AfxGetApp()->WriteProfileInt(_T("ChildFrame"), _T("cy"), rect.Height());
//	}
	((CMainFrame *)AfxGetMainWnd())->RemoveChild(this);
	CMDIChildWnd::OnDestroy();
}

void CChildFrame::OnWindowClose() 
{
	PostMessage(WM_CLOSE);
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
	
	if ( bActivate && pActivateWnd->m_hWnd == m_hWnd )
		((CMainFrame *)AfxGetMainWnd())->ActiveChild(this);	
}

void CChildFrame::OnUpdateFrameMenu(BOOL bActive, CWnd* pActiveWnd, HMENU hMenuAlt)
{
	CMenu cMenu;
	CMainFrame *pFrame = (CMainFrame *)GetMDIFrame();
	CRLoginDoc *pDoc = (CRLoginDoc *)GetActiveDocument();

	if ( bActive && AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("VMenu"), TRUE) == FALSE )
		pFrame->SetMenu(NULL);
	else if ( !bActive && pActiveWnd == NULL && pFrame != NULL && pFrame->m_StartMenuHand != NULL ) {
		cMenu.Attach(pFrame->m_StartMenuHand);
		pFrame->SetMenu(&cMenu);
		cMenu.Detach();
	} else {
		CMDIChildWnd::OnUpdateFrameMenu(bActive, pActiveWnd, hMenuAlt);
		if ( bActive && pActiveWnd != NULL && pDoc != NULL && pFrame != NULL ) {
			pDoc->m_KeyMac.SetHisMenu(pFrame);
			if ( pDoc->m_pScript != NULL )
				pDoc->m_pScript->SetMenu(pFrame);
		}
	}
}

BOOL CChildFrame::PreTranslateMessage(MSG* pMsg)
{
	if ( (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) && 
			    (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6) && (GetKeyState(VK_CONTROL) & 0x80) != 0 )
		return TRUE;

	return CMDIChildWnd::PreTranslateMessage(pMsg);
}
