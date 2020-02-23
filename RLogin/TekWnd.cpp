// TekWnd.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "TekWnd.h"

// CTekWnd

IMPLEMENT_DYNAMIC(CTekWnd, CFrameWndExt)

CTekWnd::CTekWnd(class CTextRam *pTextRam, CWnd *pWnd)
{
	m_pTextRam = pTextRam;
	m_pWnd = pWnd;
}
CTekWnd::~CTekWnd()
{
}

BEGIN_MESSAGE_MAP(CTekWnd, CFrameWndExt)
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(IDM_TEK_CLOSE, &CTekWnd::OnTekClose)
	ON_COMMAND(IDM_TEK_SAVE, &CTekWnd::OnTekSave)
	ON_COMMAND(IDM_TEK_CLEAR, &CTekWnd::OnTekClear)
	ON_COMMAND(IDM_TEK_MODE, &CTekWnd::OnTekMode)
	ON_UPDATE_COMMAND_UI(IDM_TEK_MODE, &CTekWnd::OnUpdateTekMode)
END_MESSAGE_MAP()

// CTekWnd メッセージ ハンドラ


void CTekWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	m_pTextRam->TekDraw(&dc, rect);
}

void CTekWnd::PostNcDestroy()
{
	m_pTextRam->m_pTekWnd = NULL;
	delete this;
}

BOOL CTekWnd::PreCreateWindow(CREATESTRUCT& cs)
{
//	cs.hMenu = ::LoadMenu(cs.hInstance, MAKEINTRESOURCE(IDR_TEKWND));
	((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_TEKWND), cs.hMenu);

	cs.x = CW_USEDEFAULT;
	cs.y = CW_USEDEFAULT;
	cs.cx = AfxGetApp()->GetProfileInt(_T("TekWnd"), _T("cx"), 640);
	cs.cy = AfxGetApp()->GetProfileInt(_T("TekWnd"), _T("cy"), 480 + 60);

	return CFrameWndExt::PreCreateWindow(cs);
}

void CTekWnd::OnDestroy()
{
	if ( !IsIconic() && !IsZoomed() ) {
		CRect rect;
		GetWindowRect(&rect);
		AfxGetApp()->WriteProfileInt(_T("TekWnd"), _T("x"),  rect.left);
		AfxGetApp()->WriteProfileInt(_T("TekWnd"), _T("y"),  rect.top);
		AfxGetApp()->WriteProfileInt(_T("TekWnd"), _T("cx"), rect.Width());
		AfxGetApp()->WriteProfileInt(_T("TekWnd"), _T("cy"), rect.Height());
	}

	CFrameWndExt::OnDestroy();
}

void CTekWnd::OnTekClose()
{
	DestroyWindow();
}

void CTekWnd::OnTekSave()
{
	CDC dc;
	CRect rect(0, 0, TEK_WIN_WIDTH / 4, TEK_WIN_HEIGHT / 4);
	CImage image;
	CFileDialog dlg(FALSE, _T("gif"), _T(""), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGTEKIMAGE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	if ( dlg.GetFileExt().CompareNoCase(_T("dxf")) == 0 ) {
		SaveDxf(dlg.GetPathName());
		return;
	} else if ( dlg.GetFileExt().CompareNoCase(_T("tek")) == 0 ) {
		SaveTek(dlg.GetPathName());
		return;
	}

	if ( !image.Create(rect.Width(), rect.Height(), 24) )
		return;

	dc.Attach(image.GetDC());
	m_pTextRam->TekDraw(&dc, rect);
	dc.Detach();
	image.ReleaseDC();
	image.Save(dlg.GetPathName());
}

BOOL CTekWnd::SaveDxf(LPCTSTR file)
{
	FILE *fp;
	CTextRam::TEKNODE *tp;
	CString str;
	static const double FontHeight[] = { 8.8,	8.2,	5.3,	4.8 };

	if ( (fp = _tfopen(file, _T("wt"))) == NULL )
		return FALSE;

	fprintf(fp, "  0\nSECTION\n");
	fprintf(fp, "  2\nENTITIES\n");

	for ( tp = m_pTextRam->m_Tek_Top ; tp != NULL ; tp = tp->next ) {
		switch(tp->md) {
		case 0:	// Line
			fprintf(fp, "  0\nLINE\n");
			fprintf(fp, "  8\n_0-0_\n");
			fprintf(fp, "  6\nCONTINUOUS\n");
			fprintf(fp, " 62\n%3d\n", tp->st & 0x0F);
			fprintf(fp, " 10\n%f\n", tp->sx / 10.0);
			fprintf(fp, " 20\n%f\n", tp->sy / 10.0);
			fprintf(fp, " 11\n%f\n", tp->ex / 10.0);
			fprintf(fp, " 21\n%f\n", tp->ey / 10.0);
			break;
		case 1:	// Text
			if ( (tp->ch & 0xFFFF0000) != 0 )
				str = (WCHAR)(tp->ch >> 16);
			str = (WCHAR)(tp->ch & 0xFFFF);
			fprintf(fp, "  0\nTEXT\n");
			fprintf(fp, "  8\n_0-0_\n");
			fprintf(fp, " 62\n  7\n");
			fprintf(fp, " 10\n%f\n", tp->sx / 10.0);
			fprintf(fp, " 20\n%f\n", tp->sy / 10.0);
			fprintf(fp, " 40\n%f\n", FontHeight[tp->st & 3] / 2.0);
			fprintf(fp, " 41\n  1\n");
			fprintf(fp, " 50\n  0\n");
			fprintf(fp, "  1\n%s\n", TstrToMbs(str));
			break;
		}
	}

	fprintf(fp, "  0\nENDSEC\n");
	fprintf(fp, "  0\nEOF\n");

	fclose(fp);
	return TRUE;
}

BOOL CTekWnd::SaveTek(LPCTSTR file)
{
	FILE *fp;
	CTextRam::TEKNODE *tp;
	CString str;
	int sm = (-1);
	int sl = (-1), sf = (-1);
	int sx = (-1), sy = (-1);

	if ( (fp = _tfopen(file, _T("wb"))) == NULL )
		return FALSE;

	fprintf(fp, "\033[?38h\033\014");

	for ( tp = m_pTextRam->m_Tek_Top ; tp != NULL ; tp = tp->next ) {
		switch(tp->md) {
		case 0:	// Line
			if ( sl != tp->st ) {
				fprintf(fp, "\033%c", (tp->st & 0x07) + 0x60);
				sl = tp->st;
				sm = (-1);
			}
			if ( sx != tp->sx || sy != tp->sy ) {
				fprintf(fp, "\035");
				fprintf(fp, "%c", ((tp->sy >> 7) & 0x1F) + 0x20);
				fprintf(fp, "%c", (((tp->sy & 3) << 2) | (tp->sx & 3)) + 0x60);
				fprintf(fp, "%c", ((tp->sy >> 2) & 0x1F) + 0x60);
				fprintf(fp, "%c", ((tp->sx >> 7) & 0x1F) + 0x20);
				fprintf(fp, "%c", ((tp->sx >> 2) & 0x1F) + 0x40);
			}
			if ( sm != tp->md ) {
				fprintf(fp, "\035\007");
				sm = tp->md;
			}
			fprintf(fp, "%c", ((tp->ey >> 7) & 0x1F) + 0x20);
			fprintf(fp, "%c", (((tp->ey & 3) << 2) | (tp->ex & 3)) + 0x60);
			fprintf(fp, "%c", ((tp->ey >> 2) & 0x1F) + 0x60);
			fprintf(fp, "%c", ((tp->ex >> 7) & 0x1F) + 0x20);
			fprintf(fp, "%c", ((tp->ex >> 2) & 0x1F) + 0x40);
			sx = tp->ex;
			sy = tp->ey;
			break;
		case 1:	// Text
			if ( (tp->ch & 0xFFFF0000) != 0 )
				str = (WCHAR)(tp->ch >> 16);
			str = (WCHAR)(tp->ch & 0xFFFF);
			if ( sf != tp->st ) {
				fprintf(fp, "\033%c", (tp->st & 0x03) + 0x38);
				sf = tp->st;
				sm = (-1);
			}
			if ( sx != tp->sx || sy != tp->sy ) {
				fprintf(fp, "\035");
				fprintf(fp, "%c", ((tp->sy >> 7) & 0x1F) + 0x20);
				fprintf(fp, "%c", (((tp->sy & 3) << 2) | (tp->sx & 3)) + 0x60);
				fprintf(fp, "%c", ((tp->sy >> 2) & 0x1F) + 0x60);
				fprintf(fp, "%c", ((tp->sx >> 7) & 0x1F) + 0x20);
				fprintf(fp, "%c", ((tp->sx >> 2) & 0x1F) + 0x40);
				sx = tp->sx;
				sy = tp->sy;
				sm = (-1);
			}
			if ( sm != tp->md ) {
				fprintf(fp, "\037");
				sm = tp->md;
			}
			fprintf(fp, "%s", TstrToMbs(str));
			break;
		}
	}

	fprintf(fp, "\015\033\003\033[?38l");

	fclose(fp);
	return TRUE;
}
void CTekWnd::OnTekClear()
{
	m_pTextRam->TekClear();
}

void CTekWnd::OnTekMode()
{
	m_pTextRam->TekForcus(m_pTextRam->IsTekMode() ? FALSE : TRUE);
}

void CTekWnd::OnUpdateTekMode(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pTextRam->IsTekMode() ? TRUE : FALSE);
}

BOOL CTekWnd::PreTranslateMessage(MSG* pMsg)
{
	CWnd *pWnd;
	if ( pMsg->message >= WM_KEYDOWN && pMsg->message <= WM_DEADCHAR ) {
		if ( m_pTextRam->IsTekMode() ) {
			if ( (pWnd = m_pTextRam->GetAciveView()) != NULL ) {
				pMsg->hwnd = pWnd->GetSafeHwnd();
				m_pTextRam->m_Tek_Flush = TRUE;
			} else {
				m_pWnd->SetFocus();
				pMsg->hwnd = ::GetFocus();
			}
		} else {
			m_pWnd->SetFocus();
			if ( (pWnd = m_pTextRam->GetAciveView()) != NULL ) {
				((CMainFrame *)m_pWnd)->MDIActivate(pWnd->GetParentFrame());
				pWnd->SetFocus();
				pMsg->hwnd = pWnd->GetSafeHwnd();
			} else
				pMsg->hwnd = ::GetFocus();
		}
		TranslateMessage(pMsg);
		DispatchMessage(pMsg);
		return TRUE;
	}

	return CFrameWndExt::PreTranslateMessage(pMsg);
}

void CTekWnd::GinMouse(int ch, int x, int y)
{
	if ( !m_pTextRam->m_Tek_Gin )
		return;

	CRect rect;
	GetClientRect(rect);

	x = x * TEK_WIN_WIDTH  / rect.Width();
	y = TEK_WIN_HEIGHT - y * TEK_WIN_HEIGHT / rect.Height();

	m_pTextRam->UNGETSTR(_T("%c%c%c%c%c"), ch,
		((x >> 7) & 0x1F) + 0x20, ((x >> 2) & 0x1F) + 0x20,
		((y >> 7) & 0x1F) + 0x20, ((y >> 2) & 0x1F) + 0x20);
}
void CTekWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	GinMouse('L', point.x, point.y);
	CFrameWndExt::OnLButtonDown(nFlags, point);
}

void CTekWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	GinMouse('l', point.x, point.y);
	CFrameWndExt::OnLButtonUp(nFlags, point);
}

void CTekWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	GinMouse('R', point.x, point.y);
	CFrameWndExt::OnRButtonDown(nFlags, point);
}

void CTekWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	GinMouse('r', point.x, point.y);
	CFrameWndExt::OnRButtonUp(nFlags, point);
}
