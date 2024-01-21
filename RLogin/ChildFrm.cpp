// ChildFrm.cpp : CChildFrame �N���X�̎���
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

/////////////////////////////////////////////////////////////////////////////
// CSplitterWndExt

IMPLEMENT_DYNCREATE(CSplitterWndExt, CSplitterWnd)

BEGIN_MESSAGE_MAP(CSplitterWndExt, CSplitterWnd)
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

BOOL CSplitterWndExt::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// return CSplitterWnd::OnSetCursor(pWnd, nHitTest, message);
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}
void CSplitterWndExt::OnMouseMove(UINT nFlags, CPoint point)
{
	if ( GetCapture() == this || HitTest(point) != 0 )
		CSplitterWnd::OnMouseMove(nFlags, point);
	else
		CWnd::OnMouseMove(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_WINDOW_CLOSE, OnWindowClose)
	ON_WM_MDIACTIVATE()
	ON_WM_MOVE()
END_MESSAGE_MAP()

// CChildFrame �R���X�g���N�V����/�f�X�g���N�V����

CChildFrame::CChildFrame()
{
	m_Width  = 10;
	m_Height = 10;
	m_Cols   = 80;
	m_Lines  = 25;
	m_VScrollFlag = FALSE;
	m_bInit = FALSE;
	m_bDeletePane = FALSE;
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	BOOL result;

	result = m_wndSplitter.Create(this, 2, 1, CSize(80, 20), pContext, WS_CHILD | WS_VISIBLE | WS_VSCROLL | SPLS_DYNAMIC_SPLIT);

	if ( (m_VScrollFlag = ((CMainFrame *)AfxGetMainWnd())->m_ScrollBarFlag) == FALSE && result ) {
		m_wndSplitter.SetScrollStyle(0);
		m_wndSplitter.RecalcLayout();
	}

	return result;
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: CREATESTRUCT cs ��ύX���āAWindow �N���X�܂��̓X�^�C����ύX���܂��B

	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style  = WS_CHILD | WS_VISIBLE | FWS_ADDTOTITLE;
//	cs.style |= WS_OVERLAPPED | WS_SYSMENU;
//	cs.style |= WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

//	cs.style &= ~(WS_BORDER | WS_DLGFRAME);
//	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	CRect rect;
	((CMainFrame *)AfxGetMainWnd())->GetFrameRect(rect);
	cs.cx = rect.Width();
	cs.cy = rect.Height();

	//TRACE("Child Style ");
	//if ( (cs.style & WS_BORDER) != NULL ) TRACE("WS_BORDER ");
	//if ( (cs.style & WS_DLGFRAME) != NULL ) TRACE("WS_DLGFRAME ");
	//if ( (cs.style & WS_THICKFRAME) != NULL ) TRACE("WS_THICKFRAME ");
	//if ( (cs.dwExStyle & WS_EX_WINDOWEDGE) != NULL ) TRACE("WS_EX_WINDOWEDGE ");
	//if ( (cs.dwExStyle & WS_EX_CLIENTEDGE) != NULL ) TRACE("WS_EX_CLIENTEDGE ");
	//if ( (cs.dwExStyle & WS_EX_DLGMODALFRAME) != NULL ) TRACE("WS_EX_DLGMODALFRAME ");
	//if ( (cs.dwExStyle & WS_EX_TOOLWINDOW) != NULL ) TRACE("WS_EX_TOOLWINDOW ");
	//TRACE("\n");

	return TRUE;
}

void CChildFrame::ActivateFrame(int nCmdShow)
{
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

// CChildFrame �f�f

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


// CChildFrame ���b�Z�[�W �n���h��

void CChildFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
}

void CChildFrame::OnMove(int x, int y)
{
	CMDIChildWnd::OnMove(x, y);

	CRLoginDoc *pDoc = (CRLoginDoc *)GetActiveDocument();

	if ( pDoc != NULL && pDoc->m_TextRam.m_BitMapStyle >= MAPING_PAN )
		pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
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
	((CMainFrame *)AfxGetMainWnd())->RemoveChild(this, m_bDeletePane);
	CMDIChildWnd::OnDestroy();
}

void CChildFrame::OnWindowClose() 
{
	CRLoginDoc *pDoc;

	if ( (pDoc = (CRLoginDoc *)GetActiveDocument()) != NULL && pDoc->GetViewCount() <= 1 )
		PostMessage(WM_COMMAND, ID_FILE_CLOSE);
	else
		PostMessage(WM_CLOSE);
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
	
	if ( bActivate && pActivateWnd->m_hWnd == m_hWnd ) {
		m_bInit = TRUE;
		((CMainFrame *)AfxGetMainWnd())->ActiveChild(this);	
	}
}

void CChildFrame::OnUpdateFrameMenu(BOOL bActivate, CWnd* pActivateWnd, HMENU hMenuAlt)
{
	CMainFrame *pFrame = (CMainFrame *)GetMDIFrame();

	if ( bActivate && AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("VMenu"), TRUE) == FALSE ) {
		pFrame->SetMenu(NULL);
	} else if ( !bActivate && pActivateWnd == NULL && pFrame != NULL && pFrame->m_StartMenuHand != NULL ) {
		CMDIChildWnd::OnUpdateFrameMenu(bActivate, pActivateWnd, pFrame->m_StartMenuHand);
	} else {
		CMDIChildWnd::OnUpdateFrameMenu(bActivate, pActivateWnd, hMenuAlt);
	}
}

BOOL CChildFrame::PreTranslateMessage(MSG* pMsg)
{
	if ( pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN ) {
		if ( (pMsg->wParam == VK_TAB || (pMsg->wParam >= VK_F1 && pMsg->wParam <= VK_F12)) && (GetKeyState(VK_CONTROL) & 0x80) != 0 )
			return TRUE;
	}

	return CMDIChildWnd::PreTranslateMessage(pMsg);
}
