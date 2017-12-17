// MsgWnd.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "MsgWnd.h"

//////////////////////////////////////////////////////////////////////////////////
// CMsgWnd

IMPLEMENT_DYNCREATE(CMsgWnd, CWnd)

CMsgWnd::CMsgWnd()
{
	m_Mixval = 100;
	m_BackColor = RGB(0, 0, 0);
	m_TextColor = RGB(255, 255, 255);
	m_FontSize = 25;
	m_bTimerEnable = TRUE;
	m_bMultiLine = FALSE;
}
CMsgWnd::~CMsgWnd()
{
}

BEGIN_MESSAGE_MAP(CMsgWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CMsgWnd メッセージ ハンドラー

BOOL CMsgWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	cs.dwExStyle |= (WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);

	return CWnd::PreCreateWindow(cs);
}

int CMsgWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if ( m_bTimerEnable ) {
		m_Timer = 0;
		SetTimer(1024, 50, NULL);
	}

	return 0;
}

void CMsgWnd::Message(LPCTSTR str, CWnd *pWnd, COLORREF bc)
{
	CString line, tmp;
	CSize sz;
	CSize text(100, 40);
	CRect rect(0, 0, 100, 40);
	CDC *pDC = NULL;
	CFont font, *pOldFont;
	BOOL trim;

	if ( pWnd == NULL )
		pWnd = ::AfxGetMainWnd();

	if ( pWnd != NULL )
		pWnd->GetClientRect(rect);

	if ( m_hWnd != NULL )
		pDC = GetDC();
	else if ( pWnd != NULL )
		pDC = pWnd->GetDC();

	if ( pDC != NULL ) {
		m_FontSize = 25;
		font.CreateFont(MulDiv(m_FontSize, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96), 0, 0, 0, FW_BOLD, FALSE, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
		pOldFont = pDC->SelectObject(&font);
	}

	if ( str != NULL ) {
		m_Msg.Empty();
		text.cx = 30;
		text.cy = 6;

		trim = FALSE;
		line.Empty();

		for ( ;  ; str++ ) {
			if ( *str == _T('\0') || *str == _T('\n') ) {
				sz.cx = 0;
				sz.cy = MulDiv(30, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96);

				while ( !line.IsEmpty() ) {
					tmp = line;
					if ( trim )
						tmp += _T("…");

					if ( pDC != NULL )
						sz = pDC->GetTextExtent(tmp);
					else {
						sz.cx = tmp.GetLength() * MulDiv(m_FontSize / 2, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiX, 96);
						sz.cy = MulDiv(m_FontSize, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96);
					}

					if ( sz.cx < rect.Width() )
						break;

					if ( pDC != NULL && m_FontSize > 19 ) {
						m_FontSize -= 2;
						pDC->SelectObject(pOldFont);
						font.DeleteObject();
						font.CreateFont(MulDiv(m_FontSize, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96), 0, 0, 0, FW_BOLD, FALSE, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
						pOldFont = pDC->SelectObject(&font);
					} else {
						line.Delete(line.GetLength() - 1);
						trim = TRUE;
					}
				}

				if ( trim )
					line += _T("…");

				if ( *str == _T('\n') ) {
					line += _T("\r\n");
					m_bMultiLine = TRUE;
				}

				m_Msg += line;

				if ( text.cx < sz.cx )
					text.cx = sz.cx + 6;
				text.cy += sz.cy;

				trim = FALSE;
				line.Empty();

				if ( *str == _T('\0') )
					break;

			} else if ( *str >= _T(' ') ) {
				line += *str;
			}
		}
	}

	if ( pDC != NULL ) {
		pDC->SelectObject(pOldFont);

		if ( m_hWnd != NULL )
			ReleaseDC(pDC);
		else
			pWnd->ReleaseDC(pDC);
	}

	rect.left = (rect.left + rect.right - text.cx) / 2; rect.right  = rect.left + text.cx;
	rect.top  = (rect.top + rect.bottom - text.cy) / 2; rect.bottom = rect.top  + text.cy;

	if ( m_hWnd == NULL )
		Create(NULL, _T("MsgWnd"), WS_TILED, rect, pWnd, (-1));
	else {
		if ( pWnd != NULL )
			pWnd->InvalidateRect(rect, FALSE);
		Invalidate(FALSE);
	}

	m_TextColor = RGB(255 - GetRValue(bc), 255 - GetGValue(bc), 255 - GetBValue(bc));
	m_BackColor = bc;

	m_Mixval = 100;
	m_Timer = 0;

	SetWindowPos(&wndTopMost, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS | SWP_DEFERERASE);
}

void CMsgWnd::OnPaint()
{
	int x, y;
	CSize sz;
	CRect rect;
	CFont font, *pOldFont;
	COLORREF col;
	CPaintDC dc(this); // device context for painting

	GetClientRect(rect);

	font.CreateFont(MulDiv(m_FontSize, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96), 0, 0, 0, FW_BOLD, FALSE, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
	pOldFont = dc.SelectObject(&font);

	dc.SetBkMode(TRANSPARENT);

	sz = dc.GetTextExtent(m_Msg);
	x = (rect.Width()  - sz.cx) / 2;
	y = (rect.Height() - sz.cy) / 2;

	dc.SetTextColor(m_BackColor);

	if ( m_Mixval == 100 ) {
		if ( m_bMultiLine ) {
			dc.DrawText(m_Msg, CRect(rect.left + 2, rect.top, rect.right, rect.bottom), DT_CENTER | DT_VCENTER | DT_HIDEPREFIX);
			dc.DrawText(m_Msg, CRect(rect.left - 2, rect.top, rect.right, rect.bottom), DT_CENTER | DT_VCENTER | DT_HIDEPREFIX);
			dc.DrawText(m_Msg, CRect(rect.left, rect.top + 2, rect.right, rect.bottom), DT_CENTER | DT_VCENTER | DT_HIDEPREFIX);
			dc.DrawText(m_Msg, CRect(rect.left, rect.top - 2, rect.right, rect.bottom), DT_CENTER | DT_VCENTER | DT_HIDEPREFIX);
		} else {
			dc.TextOut(x + 2, y,     m_Msg);
			dc.TextOut(x - 2, y,     m_Msg);
			dc.TextOut(x    , y + 2, m_Msg);
			dc.TextOut(x    , y - 2, m_Msg);
		}
	}

	col = RGB(GetRValue(m_BackColor) * (100 - m_Mixval) / 100 + GetRValue(m_TextColor) * m_Mixval / 100,
			  GetBValue(m_BackColor) * (100 - m_Mixval) / 100 + GetGValue(m_TextColor) * m_Mixval / 100,
			  GetBValue(m_BackColor) * (100 - m_Mixval) / 100 + GetBValue(m_TextColor) * m_Mixval / 100);

	dc.SetTextColor(col);

	if ( m_bMultiLine )
		dc.DrawText(m_Msg, rect, DT_CENTER | DT_VCENTER | DT_HIDEPREFIX);
	else
		dc.TextOut(x, y, m_Msg);

	dc.SelectObject(pOldFont);
}

void CMsgWnd::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent) {

	case 1024:
		m_Timer++;
		if ( m_Timer >= 60 ) {
			KillTimer(nIDEvent);
			PostMessage(WM_CLOSE);
		} else if ( m_Timer > 50 ) {
			m_Mixval = 100 - (m_Timer - 50) * 10;
			Invalidate(m_Timer > 51 ? FALSE : TRUE);
		}
		break;
	}

	CWnd::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////////////////
// CBtnWnd

IMPLEMENT_DYNCREATE(CBtnWnd, CWnd)

CBtnWnd::CBtnWnd()
{
	m_ImageSeq = 0;
	m_pView = NULL;
}

CBtnWnd::~CBtnWnd()
{
}

BEGIN_MESSAGE_MAP(CBtnWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// CMsgWnd メッセージ ハンドラー

BOOL CBtnWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	cs.dwExStyle |= (WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
	cs.cx = 24;
	cs.cy = 24;

	return CWnd::PreCreateWindow(cs);
}

int CBtnWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CBitmap BitMap;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if ( m_ImageList.m_hImageList == NULL ) {
		//BitMap.LoadBitmap(IDB_BITMAP9);
		((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(IDB_BITMAP9), BitMap);
		m_ImageList.Create(24, 24, ILC_COLOR24 | ILC_MASK, 4, 4);
		m_ImageList.Add(&BitMap, RGB(128, 128, 128));
		BitMap.DeleteObject();
	}

	SetTimer(1024, 200, NULL);

	return 0;
}

void CBtnWnd::DoButton(CWnd *pWnd, class CTextRam *pTextRam)
{
	CRect rect(0, 0, 24, 24);

	if ( pWnd != NULL && pTextRam != NULL ) {
		m_pView = (CRLoginView *)pWnd;

		pWnd->GetClientRect(rect);
		rect.left = rect.right - 32; rect.right  = rect.left + 24;
		rect.top  = rect.top   +  8; rect.bottom = rect.top  + 24;

		if ( m_hWnd == NULL )
			Create(NULL, _T("BtnWnd"), WS_TILED, rect, pWnd, IDC_CANCELBTN);

		SetWindowPos(&wndTopMost, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_ASYNCWINDOWPOS | SWP_DEFERERASE);

	} else if ( pWnd != NULL && m_hWnd != NULL ) {

		pWnd->GetClientRect(rect);
		rect.left = rect.right - 32; rect.right  = rect.left + 24;
		rect.top  = rect.top   +  8; rect.bottom = rect.top  + 24;

		SetWindowPos(&wndTopMost, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_ASYNCWINDOWPOS | SWP_DEFERERASE);

	} else if ( m_hWnd != NULL ) {
		m_pView = NULL;
		KillTimer(1024);
		PostMessage(WM_CLOSE);
	}
}

void CBtnWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	m_ImageList.Draw(&dc, m_ImageSeq < 4 ? m_ImageSeq : (7 - m_ImageSeq), CPoint(0, 0), ILD_NORMAL);	// ILD_BLEND50 or ILD_NORMAL
}

void CBtnWnd::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent) {
	case 1024:
		if ( ++m_ImageSeq >= 8 )
			m_ImageSeq = 0;
		Invalidate(FALSE);
		break;
	}

	CWnd::OnTimer(nIDEvent);
}

void CBtnWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ( m_pView != NULL )
		m_pView->PostMessage(WM_COMMAND, IDC_CANCELBTN, 0);

	CWnd::OnLButtonDown(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////////////
// CTrackWnd

IMPLEMENT_DYNCREATE(CTrackWnd, CWnd)

CTrackWnd::CTrackWnd()
{
	m_TypeCol = 0;
}

CTrackWnd::~CTrackWnd()
{
}

BEGIN_MESSAGE_MAP(CTrackWnd, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

// CTrackWnd メッセージ ハンドラー

BOOL CTrackWnd::PreCreateWindow(CREATESTRUCT& cs)
{
//	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	cs.dwExStyle |= (WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);

	return CWnd::PreCreateWindow(cs);
}

void CTrackWnd::SetText(LPCTSTR str)
{
	CString text;

	for ( int n = 0 ; n < 40 && *str != _T('\0') ; n++, str++ ) {
		if ( *str >= _T(' ') )
			text += *str;
	}

	SetWindowText(text);
	Invalidate(FALSE);
}

void CTrackWnd::OnPaint()
{
	int x, y;
	CSize sz;
	CRect rect;
	CString str;
	CFont font, *pOldFont;
	COLORREF bc;
	CPaintDC dc(this); // device context for painting

	GetWindowText(str);
	GetClientRect(rect);

	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = MulDiv(::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9), ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96);

	font.CreateFont(0 - MulDiv(FontSize, dc.GetDeviceCaps(LOGPIXELSY), 72), 0, 0, 0, 0, FALSE, 0, 0, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, FontName);
	pOldFont = dc.SelectObject(&font);

	dc.SetBkMode(TRANSPARENT);

	switch(m_TypeCol) {
	case 0:
		bc = GetSysColor(COLOR_WINDOW);
		break;
	case 1:
		bc = RGB(255, 64, 64);
		break;
	case 2:
		bc = RGB(255, 255, 64);
		break;
	case 3:
		bc = RGB(64, 255, 64);
		break;
	case 4:
		bc = RGB(64, 255, 255);
		break;
	case 5:
		bc = RGB(96, 96, 96);
		break;
	}

	dc.FillSolidRect(rect, bc);
	dc.Draw3dRect(rect, GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_WINDOWFRAME));

	sz = dc.GetTextExtent(str);
	x = 8;
//	x = (rect.Width()  - sz.cx) / 2;
	y = (rect.Height() - sz.cy) / 2;

	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	dc.TextOut(x, y, str);

	dc.SelectObject(pOldFont);
}
