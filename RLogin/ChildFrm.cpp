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
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
//	return m_wndSplitter.Create(this,
//		2, 2,			// TODO: 行と列の数を調整してください。
//		CSize(10, 10),	// TODO: 最小ペインのサイズを変更します。
//		pContext);
	return m_wndSplitter.Create(this, 2, 1, CSize(40, 20), pContext,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | SPLS_DYNAMIC_SPLIT);
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

//	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

//	if ( AfxGetApp()->GetProfileInt("ChildFrame", "Style", 1) == 1 )
//		cs.style |= WS_MAXIMIZE;
//	else {
//		cs.x  = AfxGetApp()->GetProfileInt("ChildFrame", "x", cs.x);
//		cs.y  = AfxGetApp()->GetProfileInt("ChildFrame", "y", cs.y);
//		cs.cx = AfxGetApp()->GetProfileInt("ChildFrame", "cx", cs.cx);
//		cs.cy = AfxGetApp()->GetProfileInt("ChildFrame", "cy", cs.cy);
//	}

	CRect rect;
	((CMainFrame *)AfxGetMainWnd())->GetFrameRect(rect);
	cs.cx = rect.Width();
	cs.cy = rect.Height();

	return TRUE;
}

void CChildFrame::ActivateFrame(int nCmdShow)
{
//	if ( AfxGetApp()->GetProfileInt("ChildFrame", "Style", 1) == 1 )
//		nCmdShow = SW_SHOWMAXIMIZED;
	CMDIChildWnd::ActivateFrame(nCmdShow);
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

	if ( nType != SIZE_MINIMIZED )
		AfxGetApp()->WriteProfileInt("ChildFrame", "Style", (nType == SIZE_MAXIMIZED ? 1 : 0));
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
	if ( !IsIconic() && !IsZoomed() ) {
		CRect rect;
		GetWindowRect(&rect);
		AfxGetApp()->WriteProfileInt("ChildFrame", "x", rect.left);
		AfxGetApp()->WriteProfileInt("ChildFrame", "y", rect.top);
		AfxGetApp()->WriteProfileInt("ChildFrame", "cx", rect.Width());
		AfxGetApp()->WriteProfileInt("ChildFrame", "cy", rect.Height());
	}
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
