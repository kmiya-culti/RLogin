// GhostWnd.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "GhostWnd.h"

// CGhostWnd

IMPLEMENT_DYNAMIC(CGhostWnd, CWnd)

CGhostWnd::CGhostWnd()
{
	m_pView = NULL;
	m_pDoc  = NULL;
}
CGhostWnd::~CGhostWnd()
{
}

BEGIN_MESSAGE_MAP(CGhostWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CGhostWnd メッセージ ハンドラ

BOOL CGhostWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。

	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	cs.dwExStyle |= (WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);

	return CWnd::PreCreateWindow(cs);
}

int CGhostWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  ここに特定な作成コードを追加してください。

	SetLayeredWindowAttributes(0, 0, LWA_ALPHA);
	m_Timer = 0;
	SetTimer(1024, 50, NULL);

	return 0;
}

void CGhostWnd::OnPaint()
{
	int sx = 0;
	int sy = 0;
	int ex = m_pView->m_Cols  - 1;
	int ey = m_pView->m_Lines - 1;

	CPaintDC dc(this); // device context for painting

	if ( !dc.IsPrinting() ) {
		CRect rect(dc.m_ps.rcPaint);
		sx = rect.left   * m_pView->m_Cols  / m_pView->m_Width;
		ex = (rect.right + m_pView->m_CharWidth - 1)  * m_pView->m_Cols  / m_pView->m_Width - 1;
		sy = rect.top    * m_pView->m_Lines / m_pView->m_Height;
		ey = (rect.bottom + m_pView->m_CharHeight - 1) * m_pView->m_Lines / m_pView->m_Height - 1;

		if ( m_pView->m_pBitmap != NULL ) {
			CDC TempDC;
			CBitmap *pOldBitMap;
			TempDC.CreateCompatibleDC(&dc);
			pOldBitMap = (CBitmap *)TempDC.SelectObject(m_pView->m_pBitmap);
			dc.BitBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, &TempDC, rect.left, rect.top, SRCCOPY);
			TempDC.SelectObject(pOldBitMap);
			dc.SetBkMode(TRANSPARENT);
		}
	}

	if ( m_pDoc->m_TextRam.IsInitText() )
		m_pDoc->m_TextRam.DrawVram(&dc, sx, sy, ex, ey, m_pView, TRUE);
}

void CGhostWnd::PostNcDestroy()
{
	CWnd::PostNcDestroy();
	delete this;
}

void CGhostWnd::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent) {
	case 1024:
		SetLayeredWindowAttributes(0, ++m_Timer * 8, LWA_ALPHA);
		if ( m_Timer >= 12 )
			KillTimer(nIDEvent);
		break;
	}

	CWnd::OnTimer(nIDEvent);
}
