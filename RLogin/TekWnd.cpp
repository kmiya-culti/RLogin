// TekWnd.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "TekWnd.h"

#define	_USE_MATH_DEFINES
#include <math.h>

#define	absd(d)			((d) < 0.0 ? (0.0 - (d)) : (d))

#define	SVG_BEZ_DIVS	4

// CTekWnd

IMPLEMENT_DYNAMIC(CTekWnd, CFrameWndExt)

CTekWnd::CTekWnd(class CTextRam *pTextRam, CWnd *pWnd)
{
	m_pTextRam = pTextRam;
	m_pWnd = pWnd;

	m_OfsX = m_OfsY = 0.0;
	m_Width  = TEK_WIN_WIDTH;
	m_Height = TEK_WIN_HEIGHT;
	m_FontWidth  = (double)(TekFonts[0][0]);
	m_FontHeight = (double)(TekFonts[0][1]);
	m_pTransForm = NULL;
}
CTekWnd::~CTekWnd()
{
	SvgTransFormClear();
}

BEGIN_MESSAGE_MAP(CTekWnd, CFrameWndExt)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_DROPFILES()
	ON_COMMAND(IDM_TEK_CLOSE, &CTekWnd::OnTekClose)
	ON_COMMAND(IDM_TEK_SAVE, &CTekWnd::OnTekSave)
	ON_COMMAND(IDM_TEK_CLEAR, &CTekWnd::OnTekClear)
	ON_COMMAND(IDM_TEK_MODE, &CTekWnd::OnTekMode)
	ON_UPDATE_COMMAND_UI(IDM_TEK_MODE, &CTekWnd::OnUpdateTekMode)
	ON_COMMAND(ID_EDIT_COPY, &CTekWnd::OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CTekWnd::OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &CTekWnd::OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CTekWnd::OnUpdateEditPaste)
	ON_COMMAND(ID_FILE_PRINT, &CTekWnd::OnFilePrint)
END_MESSAGE_MAP()

// CTekWnd メッセージ ハンドラ


void CTekWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	m_pTextRam->TekDraw(&dc, rect, FALSE);
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
	cs.cx = AfxGetApp()->GetProfileInt(_T("TekWnd"), _T("cx"), MulDiv(640,      SYSTEM_DPI_X, DEFAULT_DPI_X));
	cs.cy = AfxGetApp()->GetProfileInt(_T("TekWnd"), _T("cy"), MulDiv(480 + 60, SYSTEM_DPI_Y, DEFAULT_DPI_Y));

	return CFrameWndExt::PreCreateWindow(cs);
}

int CTekWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndExt::OnCreate(lpCreateStruct) == -1)
		return -1;

	DragAcceptFiles(TRUE);

	return 0;
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
	} else if ( dlg.GetFileExt().CompareNoCase(_T("svg")) == 0 ) {
		SaveSvg(dlg.GetPathName());
		return;
	}

	if ( !image.Create(rect.Width(), rect.Height(), 24) )
		return;

	dc.Attach(image.GetDC());
	dc.FillSolidRect(rect, RGB(255, 255, 255));
	m_pTextRam->TekDraw(&dc, rect, FALSE);
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

#define	SVG_DIV		4.0

void CTekWnd::SaveSvgText(CBuffer &buf)
{
	CTextRam::TEKNODE *tp;
	CString str;
	CIConv iconv;
	CStringA mbs, xpos, ypos, tmp;
	int count;
	static const char *PenStyle[]    = { "none","1,1", "3,1,1,1","2,1","4,2","4,1,1,1,1,1","6,1,1,1","6,3" };

	buf += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";

	tmp.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" ViewBox=\"%d %d %g %g\">\n", 
		0, 0, TEK_WIN_WIDTH / SVG_DIV, TEK_WIN_HEIGHT / SVG_DIV);
	buf += (LPCSTR)tmp;

	for ( tp = m_pTextRam->m_Tek_Top ; tp != NULL ; tp = tp->next ) {
		switch(tp->md) {
		case 0:	// Line
			mbs.Empty();
			if ( tp->next != NULL && tp->md == tp->next->md && tp->st == tp->next->st ) {
				if ( tp->sx == tp->next->ex && tp->sy == tp->next->ey ) {
					//						tp->sx,sy			tp->ex,ey
					//	tp->next->sx,sy		tp->next->ex,ey
					tmp.Format("%g,%g ", tp->ex / SVG_DIV, (TEK_WIN_HEIGHT - tp->ey) / SVG_DIV);
					mbs += tmp;
					count = 1;

					do {
						tp = tp->next;
						tmp.Format("%g,%g ", tp->ex / SVG_DIV, (TEK_WIN_HEIGHT - tp->ey) / SVG_DIV);
						mbs += tmp;
						if ( ++count > 8 ) {
							mbs += "\n\t";
							count = 0;
						}
					} while ( tp->next != NULL && tp->md == tp->next->md && tp->st == tp->next->st && tp->sx == tp->next->ex && tp->sy == tp->next->ey );

					tmp.Format("%g,%g ", tp->sx / SVG_DIV, (TEK_WIN_HEIGHT - tp->sy) / SVG_DIV);
					mbs += tmp;

				} else if ( tp->ex == tp->next->sx && tp->ey == tp->next->sy ) {
					//	tp->sx,sy			tp->ex,ey
					//						tp->next->sx,sy		tp->next->ex,ey
					tmp.Format("%g,%g ", tp->sx / SVG_DIV, (TEK_WIN_HEIGHT - tp->sy) / SVG_DIV);
					mbs += tmp;
					count = 1;

					do {
						tp = tp->next;
						tmp.Format("%g,%g ", tp->sx / SVG_DIV, (TEK_WIN_HEIGHT - tp->sy) / SVG_DIV);
						mbs += tmp;
						if ( ++count > 8 ) {
							mbs += "\n\t";
							count = 0;
						}
					} while ( tp->next != NULL && tp->md == tp->next->md && tp->st == tp->next->st && tp->ex == tp->next->sx && tp->ey == tp->next->sy );

					tmp.Format("%g,%g ", tp->ex / SVG_DIV, (TEK_WIN_HEIGHT - tp->ey) / SVG_DIV);
					mbs += tmp;
				}
			}

			if ( mbs.IsEmpty() ) {
				tmp.Format("<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\" stroke=\"#%02X%02X%02X\" stroke-dasharray=\"%s\" stroke-width=\"%d\" />\n",
					tp->sx / SVG_DIV, (TEK_WIN_HEIGHT - tp->sy) / SVG_DIV, tp->ex / SVG_DIV, (TEK_WIN_HEIGHT- tp->ey) / SVG_DIV,
					GetRValue(TekColorTab[(tp->st / 8) % 16]), GetGValue(TekColorTab[(tp->st / 8) % 16]), GetBValue(TekColorTab[(tp->st / 8) % 16]),
					PenStyle[tp->st % 8],
					TekPenWidthTab[tp->st / 128]);
			} else {
				tmp.Format("<polyline fill=\"none\" stroke=\"#%02X%02X%02X\" stroke-dasharray=\"%s\" stroke-width=\"%d\" points=\"%s\"/>\n",
					GetRValue(TekColorTab[(tp->st / 8) % 16]), GetGValue(TekColorTab[(tp->st / 8) % 16]), GetBValue(TekColorTab[(tp->st / 8) % 16]),
					PenStyle[tp->st % 8],
					TekPenWidthTab[tp->st / 128],
					(LPCSTR)mbs);
			}
			buf += (LPCSTR)tmp;
			break;

		case 1:	// Text
			count = 0;
			mbs.Empty();
			xpos.Empty();
			ypos.Format("%g", (TEK_WIN_HEIGHT - tp->sy - TekFonts[tp->st & 7][1] / 8) / SVG_DIV);

			for ( ; ; ) {
				if ( (tp->ch & 0xFFFF0000) != 0 )
					str = (WCHAR)(tp->ch >> 16);
				str = (WCHAR)(tp->ch & 0xFFFF);
				iconv.StrToRemote(_T("UTF-8"), str, tmp);
				mbs.Insert(0, tmp);

				tmp.Format("%g%s", tp->sx / SVG_DIV, (xpos.IsEmpty() ? "" : " "));
				xpos.Insert(0, tmp);

				if ( tp->next != NULL && tp->md == tp->next->md && tp->st == tp->next->st && tp->ex == tp->next->ex &&
					 tp->ex == 0   && tp->sy == tp->next->sy && (tp->sx - TekFonts[tp->st & 7][0]) == tp->next->sx )
					tp = tp->next;
				else
					break;
			}

			while ( tp->next != NULL && tp->md == tp->next->md && tp->st == tp->next->st && tp->ex == tp->next->ex &&
					 tp->ex == 0   && tp->sy == tp->next->sy && (tp->sx + TekFonts[tp->st & 7][0]) == tp->next->sx ) {
				tp = tp->next;

				if ( (tp->ch & 0xFFFF0000) != 0 )
					str = (WCHAR)(tp->ch >> 16);
				str = (WCHAR)(tp->ch & 0xFFFF);
				iconv.StrToRemote(_T("UTF-8"), str, tmp);
				mbs += tmp;

				tmp.Format(" %g", tp->sx / SVG_DIV);
				xpos += tmp;
			}

			tmp.Empty();
			for ( LPCSTR p = mbs ; *p != '\0' ; p++ ) {
				if ( (BYTE)*p <= ' ' || strchr("<>&'\"\x7F", *p) != NULL ) {
					CStringA wrk;
					wrk.Format("&#%d;", *p);
					tmp += wrk;
				} else
					tmp += *p;
			}

			mbs.Format("<text x=\"%s\" y=\"%s\" font-family=\"monospace\" font-size=\"%g\" fill=\"#%02X%02X%02X\" rotate=\"%d\">%s</text>\n",
				(LPCSTR)xpos, (LPCSTR)ypos,
				TekFonts[tp->st & 7][1] / SVG_DIV,
				GetRValue(TekColorTab[(tp->st >> 3) & 0x0F]), GetGValue(TekColorTab[(tp->st >> 3) & 0x0F]), GetBValue(TekColorTab[(tp->st >> 3) & 0x0F]),
				(360 - tp->ex) % 360,
				(LPCSTR)tmp);
			buf += (LPCSTR)mbs;
			break;
		}
	}

	buf += "</svg>\n";
}
BOOL CTekWnd::SaveSvg(LPCTSTR file)
{
	FILE *fp;
	CBuffer buf;

	SaveSvgText(buf);

	if ( (fp = _tfopen(file, _T("wb"))) == NULL )
		return FALSE;

	fwrite(buf.GetPtr(), 1, buf.GetSize(), fp);
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

void CTekWnd::OnEditCopy()
{
	HANDLE hBitmap = NULL;
	CDC TempDC;
	CBitmap TempMap, *pOldMap;
	CRect rect(0, 0, TEK_WIN_WIDTH / 4, TEK_WIN_HEIGHT / 4);
	CBuffer buf;
	HGLOBAL hData = NULL;
	LPTSTR pData = NULL;

	TempDC.CreateCompatibleDC(NULL);
	TempMap.CreateBitmap(rect.Width(), rect.Height(), TempDC.GetDeviceCaps(PLANES), TempDC.GetDeviceCaps(BITSPIXEL), NULL);
	pOldMap = (CBitmap *)TempDC.SelectObject(&TempMap);

	TempDC.FillSolidRect(rect, RGB(255, 255, 255));
	m_pTextRam->TekDraw(&TempDC, rect, FALSE);

	TempDC.SelectObject(pOldMap);
	TempDC.DeleteDC();

	hBitmap = (HANDLE)TempMap.GetSafeHandle();
	TempMap.Detach();

	SaveSvgText(buf);

	if ( (hData = GlobalAlloc(GMEM_MOVEABLE, (buf.GetSize() + 1))) != NULL ) {
		if ( (pData = (TCHAR *)GlobalLock(hData)) != NULL ) {
			strcpy((LPSTR)pData, (LPCSTR)buf);
			GlobalUnlock(pData);
		} else {
			GlobalFree(hData);
			hData = NULL;
		}
	}

	if ( OpenClipboard() ) {
		if ( EmptyClipboard() ) {
			if ( hBitmap != NULL && SetClipboardData(CF_BITMAP, hBitmap) != NULL )
				hBitmap = NULL;

			if ( hData != NULL && SetClipboardData(RegisterClipboardFormat(_T("image/svg+xml")), hData) != NULL )
				hData = NULL;
		}
		CloseClipboard();
	}

	if ( hBitmap != NULL )
		DeleteObject(hBitmap);

	if ( hData != NULL )
		GlobalFree(hData);

	if ( hBitmap != NULL || hData != NULL )
		MessageBox(_T("TekWindow Clipboard paste error"), _T("Error"), MB_ICONHAND);
}
void CTekWnd::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pTextRam->m_Tek_Top != NULL ? TRUE : FALSE);
}
void CTekWnd::OnEditPaste()
{
	HGLOBAL hData;
	LPBYTE pData = NULL;
	CBuffer text;

	if ( !OpenClipboard() )
		return;

	if ( (hData = GetClipboardData(RegisterClipboardFormat(_T("image/svg+xml")))) != NULL && (pData = (LPBYTE)GlobalLock(hData)) != NULL ) {
		text.Apend(pData, (int)GlobalSize(hData));
		GlobalUnlock(hData);
		CloseClipboard();
		text.KanjiConvert(text.KanjiCheck());

	} else if ( (hData = GetClipboardData(CF_UNICODETEXT)) != NULL && (pData = (LPBYTE)GlobalLock(hData)) != NULL ) {
		text.Apend(pData, (int)GlobalSize(hData));
		GlobalUnlock(hData);
		CloseClipboard();

	} else {
		CloseClipboard();
		return;
	}

	LoadSvgText((LPCTSTR)text);
}
void CTekWnd::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsClipboardFormatAvailable(RegisterClipboardFormat(_T("image/svg+xml"))) || IsClipboardFormatAvailable(CF_UNICODETEXT) ? TRUE : FALSE);
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

void CTekWnd::OnDropFiles(HDROP hDropInfo)
{
    int i, n;
	int max = MAX_PATH;
	TCHAR *pFileName = new TCHAR[max + 1];
	TCHAR *p;
    int FileCount;

    FileCount = DragQueryFile(hDropInfo, 0xffffffff, NULL, 0);

	for( i = 0 ; i < FileCount ; i++ ) {
		if (max < (n = DragQueryFile(hDropInfo, i, NULL, 0))) {
			max = n;
			delete[] pFileName;
			pFileName = new TCHAR[max + 1];
		}
		DragQueryFile(hDropInfo, i, pFileName, max);

		if ( (p = _tcsrchr(pFileName, _T('.'))) != NULL && _tcscmp(p, _T(".svg")) == 0 )
			LoadSvg(pFileName);
	}

	delete [] pFileName;
}

void CTekWnd::OnFilePrint()
{
	CDC dc;
    DOCINFO docinfo;
	CPrintDialog dlg(FALSE, PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION, this);
	CRect frame, margin;

	//if ( dlg.DoModal() != IDOK )
	if ( theApp.DoPrintDialog(&dlg) != IDOK )
		return;

	dc.Attach(dlg.CreatePrinterDC());

    memset(&docinfo, 0, sizeof(DOCINFO));
    docinfo.cbSize = sizeof(DOCINFO);
	docinfo.lpszDocName = _T("Tek40xx Print");

	margin.left   = 10;
	margin.right  = 10;
	margin.top    = 10;
	margin.bottom = 10;

	frame.left   = margin.left * 10 * dc.GetDeviceCaps(LOGPIXELSX) / 254;
	frame.top    = margin.top  * 10 * dc.GetDeviceCaps(LOGPIXELSY) / 254;
	frame.right  = dc.GetDeviceCaps(HORZRES) - margin.right  * 10 * dc.GetDeviceCaps(LOGPIXELSX) / 254;
	frame.bottom = dc.GetDeviceCaps(VERTRES) - margin.bottom * 10 * dc.GetDeviceCaps(LOGPIXELSY) / 254;

	dc.SetTextColor(RGB(0, 0, 0));
	dc.SetBkColor(RGB(255, 255, 255));
	dc.SetBkMode(TRANSPARENT);

	dc.StartDoc(&docinfo);
	dc.StartPage();

	BOOL bImg = m_pTextRam->IsOptEnable(TO_RLGRPIND);
	m_pTextRam->EnableOption(TO_RLGRPIND);

	m_pTextRam->TekDraw(&dc, frame, FALSE);

	m_pTextRam->SetOption(TO_RLGRPIND, bImg);

	dc.EndPage();
	dc.EndDoc();

	dc.Detach();
}

//////////////////////////////////////////////////////////

LPCTSTR CTekWnd::SvgTagStr(CStringIndex &xml, LPCTSTR name)
{
	int n;
	CStringIndex *pXml;
	
	for ( pXml = &xml ; pXml != NULL ; pXml = pXml->m_pOwner ) {
		if ( (n = pXml->Find(name)) >= 0 )
			return (*pXml)[n];
	}
	return _T("");
}

LPCTSTR CTekWnd::SvgParam(LPCTSTR p, CArray<double, double> &param, LPCTSTR pFormat)
{
	CString value;
	TCHAR f = _T('f');
	LPCTSTR s = pFormat;
	int after = (-1);
	double d = 0.0;
	static const LPCTSTR afterList[2][10] = {
		{ _T("deg"), _T("grad"), _T("rad"), NULL },
		{ _T("em"), _T("ex"), _T("px"), _T("in"), _T("cm"), _T("mm"), _T("pt"), _T("pc"), _T("%"),	NULL }
	};

	param.RemoveAll();

	while ( *p != _T('\0') ) {
		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;

		after = (-1);
		value.Empty();

		if ( *p != _T(',') ) {
			if ( s != NULL  ) {
				if ( *s == _T('\0') )
					s = pFormat;
				f = *(s++);
			}

			// 0	flag:						[01]
			// d	digit:						[+-]?
			// D	nonnegative-digit:			[0-9]+
			// f	floating-point:				[+-]?
			// F	nonnegative-floating-point: [0-9]* [.]? [0-9]* ([Ee] [+-]? [0-9]+)?
			// r	floating-point-rotate:		[+-]? [0-9]* [.]? [0-9]* ([Ee]? [+-]? [0-9]*) ("deg" | "grad" | "grad")?
			// l	floating-point-lenght:		[+-]? [0-9]* [.]? [0-9]* ([Ee]? [+-]? [0-9]*) ("em" | "ex" | "px" | "in" | "cm" | "mm" | "pt" | "pc" | "%")?

			switch(f) {
			case _T('0'):
				if ( *p == _T('0') || *p == _T('1') )
					value += *(p++);
				break;

			case _T('d'):
				if ( *p == _T('-') || *p == _T('+') )
					value += *(p++);
			case _T('D'):
				while ( *p != _T('\0') && *p >= _T('0') && *p <= _T('9') )
					value += *(p++);
				break;

			case _T('f'):
			case _T('r'):
			case _T('l'):
				if ( *p == _T('-') || *p == _T('+') )
					value += *(p++);
			case _T('F'):
			case _T('R'):
			case _T('L'):
				while ( *p != _T('\0') && *p >= _T('0') && *p <= _T('9') )
					value += *(p++);
				if ( *p == _T('.') )
					value += *(p++);
				while ( *p != _T('\0') && *p >= _T('0') && *p <= _T('9') )
					value += *(p++);
				if ( (*p == _T('e') || *p == _T('E')) && (p[1] == _T('-') || p[1] == _T('+') || (p[1] >= _T('0') && p[1] <= _T('9'))) ) {
					value += *(p++);
					if ( *p == _T('-') || *p == _T('+') )
						value += *(p++);
					while ( *p != _T('\0') && *p >= _T('0') && *p <= _T('9') )
						value += *(p++);
				}

#if 0
				if ( (*p >= _T(' ') && *p != _T(',')) && (f == _T('r') || f == _T('l') || f == _T('R') || f == _T('L')) && (*p == _T('%') || (*p >= _T('A') && *p <= _T('Z')) || (*p >= _T('a') && *p <= _T('z'))) ) {
					int n = ((f == _T('r') || f == _T('R')) ? 0 : 1);
					for ( int i = 0 ; after == (-1) && afterList[n][i] != NULL ; i++ ) {
						for ( int a = 0 ; ; a++ ) {
							if ( afterList[n][i][a] == _T('\0') ) {
								if ( p[a] <= _T(' ') || p[a] == _T(',') ) {
									after = n * 10 + i;
									p += a;
									break;
								}
							} else if ( afterList[n][i][a] != _totlower(p[a]) )
								break;
						}
					}
				}
#else
				if ( *p >= _T(' ') && (*p == _T('%') || (*p >= _T('A') && *p <= _T('Z')) || (*p >= _T('a') && *p <= _T('z'))) ) {
					if ( f == _T('r') || f == _T('R') ) {
						TCHAR c0 = _totlower(p[0]);
						if ( c0 == _T('d') && _totlower(p[1]) == _T('e') && _totlower(p[2]) == _T('g') && (p[3] <= _T(' ') || p[3] == _T(',')) ) {
							after = 0;
							p += 3;
						} else if ( c0 == _T('g') && _totlower(p[1]) == _T('r') && _totlower(p[2]) == _T('a') && _totlower(p[3]) == _T('d') && (p[4] <= _T(' ') || p[4] == _T(',')) ) {
							after = 1;
							p += 4;
						} else if ( c0 == _T('r') && _totlower(p[1]) == _T('a') && _totlower(p[2]) == _T('d') && (p[3] <= _T(' ') || p[3] == _T(',')) ) {
							after = 2;
							p += 3;
						}
					} else if ( f == _T('l') || f == _T('L') ) {
						TCHAR c0 = _totlower(p[0]);
						if ( c0 == _T('e') ) {
							TCHAR c1 = _totlower(p[1]);
							if ( c1 == _T('m') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
								after = 10;
								p += 2;
							} else if ( c1 == _T('x') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
								after = 11;
								p += 2;
							}
						} else if ( c0 == _T('p') ) {
							TCHAR c1 = _totlower(p[1]);
							if ( c1 == _T('x') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
								after = 12;
								p += 2;
							} else if ( c1 == _T('t') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
								after = 16;
								p += 2;
							} else if ( c1 == _T('c') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
								after = 17;
								p += 2;
							}
						} else if ( c0 == _T('i') && _totlower(p[1]) == _T('n') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
							after = 13;
							p += 2;
						} else if ( c0 == _T('c') && _totlower(p[1]) == _T('m') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
							after = 14;
							p += 2;
						} else if ( c0 == _T('m') && _totlower(p[1]) == _T('m') && (p[2] <= _T(' ') || p[2] == _T(',')) ) {
							after = 15;
							p += 2;
						} else if ( c0 == _T('%') && (p[1] <= _T(' ') || p[1] == _T(',')) ) {
							after = 18;
							p += 1;
						}
					}
				}
#endif
				break;
			}

			if ( value.IsEmpty() )
				break;
		}

		d = _tstof(value);

		switch(after) {
		case 0:		// deg
			d = fmod(d, 360.0);
			break;
		case 1:		// gred
			d = fmod(d * 360.0 / 400.0, 360.0);
			break;
		case 2:		// rad
			d = fmod(d * 360 / (M_PI * 2), 360.0);
			break;

		case 10:	// "em"
			d = d * m_FontHeight;
			break;
		case 11:	// "ex"
			d = d * m_FontHeight;
			break;
		case 12:	// "px"
			break;
		case 13:	// "in"
			d = d * m_NowDpi.cx;
			break;
		case 14:	// "cm"
			d = d * m_NowDpi.cx / 2.54;
			break;
		case 15:	// "mm"
			d = d * m_NowDpi.cx / 25.4;
			break;
		case 16:	// "pt"		1/72 inc
			d = d * m_NowDpi.cx / 72;
			break;
		case 17:	// "pc"		12 pt = 12/72 inc
			d = d * m_NowDpi.cx * 12 / 72;
			break;
		case 18:	// "%"
			d = m_Width * d / 100.0;
			break;
		}

		param.Add(d);

		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		if ( *p == _T(',') )
			p++;
	}

	return p;
}
double CTekWnd::SvgAngle(LPCTSTR p)
{
	// angle ::= number ("deg" | "grad" | "rad")?

	CArray<double, double> param;

	SvgParam(p, param, _T("r"));
	return (param.GetSize() > 0 ? param[0] : 0.0);
}
double CTekWnd::SvgLength(LPCTSTR p)
{
	// length ::= number ("em" | "ex" | "px" | "in" | "cm" | "mm" | "pt" | "pc" | "%")?

	CArray<double, double> param;

	SvgParam(p, param, _T("l"));
	return (param.GetSize() > 0 ? param[0] : 0.0);
}
void CTekWnd::SvgTransFormClear()
{
	TransFormParam *pParam;

	while ( (pParam = m_pTransForm) != NULL ) {
		m_pTransForm = pParam->next;
		delete pParam;
	}
}
void CTekWnd::SvgTransFromAngle(double &angle)
{
	TransFormParam *pParam;

	for ( pParam = m_pTransForm ; pParam != NULL ; pParam = pParam->next ) {
		switch(pParam->type) {
		case 3:		// rotate
			if ( pParam->param.GetSize() >= 1 ) {
				// rotate(angle)
				angle = fmod(angle + pParam->param[0], 360.0);
			}
			break;

		case 4:		// skewX
			if ( pParam->param.GetSize() >= 1 ) {
				// skewX(angle)
				angle = fmod(angle - pParam->param[0], 360.0);
			}
			break;

		case 5:		// skewY
			if ( pParam->param.GetSize() >= 1 ) {
				// skewY(angle)
				angle = fmod(angle + pParam->param[0], 360.0);
			}
			break;
		}
	}
}
void CTekWnd::SvgTransFromSize(double &sx, double &sy)
{
	double x, y;
	TransFormParam *pParam;

	for ( pParam = m_pTransForm ; pParam != NULL ; pParam = pParam->next ) {
		switch(pParam->type) {
		case 0:		// matrix
			if ( pParam->param.GetSize() >= 6 ) {
				// matrix(a, b, c, d, e, f)
				//  [a c e] => x = a * x + c * y + e
				//  [b d f]    y = b * x + d * y + f
				x = sx, y = sy;
				sx = pParam->param[0] * x + pParam->param[2] * y + pParam->param[4];
				sy = pParam->param[1] * x + pParam->param[3] * y + pParam->param[5];
			}
			break;

		case 2:		// scale
			if ( pParam->param.GetSize() >= 2 ) {
				// scale(sx,sy)
				// X = x * sx
				// Y = y * sy
				sx = sx * pParam->param[0];
				sy = sy * pParam->param[1];
			} else if ( pParam->param.GetSize() >= 1 ) {
				// scale(sx)
				// X = x * sx
				// Y = y * sx
				sx = sx * pParam->param[0];
				sy = sy * pParam->param[0];
			}
			break;
		}
	}
}
void CTekWnd::SvgTransFromCalc(double &sx, double &sy)
{
	double x, y, angle;
	TransFormParam *pParam;

	for ( pParam = m_pTransForm ; pParam != NULL ; pParam = pParam->next ) {
		switch(pParam->type) {
		case 0:		// matrix
			if ( pParam->param.GetSize() >= 6 ) {
				// matrix(a, b, c, d, e, f)
				//  [a c e] => x = a * x + c * y + e
				//  [b d f]    y = b * x + d * y + f
				x = sx, y = sy;
				sx = pParam->param[0] * x + pParam->param[2] * y + pParam->param[4];
				sy = pParam->param[1] * x + pParam->param[3] * y + pParam->param[5];
			}
			break;

		case 1:		// translate
			if ( pParam->param.GetSize() >= 2 ) {
				// translate(tx,ty)
				// X = x + tx
				// Y = y + ty
				sx = sx + pParam->param[0];
				sy = sy + pParam->param[1];
			} else if ( pParam->param.GetSize() >= 1 ) {
				// translate(tx)
				// X = x + tx
				// Y = y + tx
				sx = sx + pParam->param[0];
				sy = sy + pParam->param[0];
			}
			break;

		case 2:		// scale
			if ( pParam->param.GetSize() >= 2 ) {
				// scale(sx,sy)
				// X = x * sx
				// Y = y * sy
				sx = sx * pParam->param[0];
				sy = sy * pParam->param[1];
			} else if ( pParam->param.GetSize() >= 1 ) {
				// scale(sx)
				// X = x * sx
				// Y = y * sx
				sx = sx * pParam->param[0];
				sy = sy * pParam->param[0];
			}
			break;

		case 3:		// rotate
			if ( pParam->param.GetSize() >= 3 ) {
				// rotate(angle,cx,cy)
				// x = x - cx;
				// y = y - cx;
				// X = x * cos(angle) - y * sin(angle)
				// Y = x * sin(angle) + y * cos(angle)
				// X = cx + X
				// Y = cx + Y
				angle = M_PI * 2.0 * pParam->param[0] / 360.0;
				x = sx - pParam->param[1], y = sy - pParam->param[2];
				sx = pParam->param[1] + x * cos(angle) - y * sin(angle);
				sy = pParam->param[2] + x * sin(angle) + y * cos(angle);
			} else if ( pParam->param.GetSize() >= 1 ) {
				// rotate(angle)
				// X = x * cos(angle) - y * sin(angle)
				// Y = x * sin(angle) + y * cos(angle)
				angle = M_PI * 2.0 * pParam->param[0] / 360.0;
				x = sx, y = sy;
				sx = x * cos(angle) - y * sin(angle);
				sy = x * sin(angle) + y * cos(angle);
			}
			break;

		case 4:		// skewX
			if ( pParam->param.GetSize() >= 1 ) {
				// skewX(angle)
				// X = x + y * tan(angle)
				// Y = y
				angle = M_PI * 2.0 * pParam->param[0] / 360.0;
				sx = sx + sy * tan(angle);
			}
			break;

		case 5:		// skewY
			if ( pParam->param.GetSize() >= 1 ) {
				// skewY(angle)
				// X = x
				// Y = x * tan(angle) + y
				angle = M_PI * 2.0 * pParam->param[0] / 360.0;
				sy = sx * tan(angle) + sy;
			}
			break;
		}
	}
}
void CTekWnd::SvgTransForm(CStringIndex *pXml)
{
	int n;
	LPCTSTR p;
	TransFormParam *pParam;
	static const LPCTSTR FuncTab[] = { _T("matrix"), _T("translate"), _T("scale"), _T("rotate"), _T("skewX"), _T("skewY"), NULL };

	if ( pXml == NULL )
		return;

	SvgTransForm(pXml->m_pOwner);

	if ( (n = pXml->Find(_T("transform"))) < 0 )
		return;

	p = (*pXml)[n];
	CString func, value;

	while ( *p != _T('\0') ) {
		while ( *p != _T('\0') && (*p <= _T(' ') || *p == _T(',')) )
			p++;
		func.Empty();
		while ( *p != _T('\0') && *p > _T(' ') && *p != _T('(') )
			func += *(p++);
		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		if ( *(p++) != _T('(') )
			break;

		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		value.Empty();
		while ( *p != _T('\0') && *p >= _T(' ') && *p != _T(')') )
			value += *(p++);
		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		if ( *(p++) != _T(')') )
			break;

		for ( n = 0 ; FuncTab[n] != NULL ; n++ ) {
			if ( func.CompareNoCase(FuncTab[n]) == 0 ) {
				pParam = new TransFormParam;
				pParam->type = n;
				SvgParam(value, pParam->param);
				pParam->next = m_pTransForm;
				m_pTransForm = pParam;
				break;
			}
		}
	}
}
int CTekWnd::SvgToTekColor(LPCTSTR p)
{
	int r, g, b;
	COLORREF col = (-1);
	int idx = (-1);

#define	RoundMulDiv(d,m)	(BYTE)(((DWORD)(d) * 255 + (m / 2)) / m)

	while ( *p != _T('\0') && *p <= _T(' ') )
		p++;

	if ( *p == _T('#') ) {
		p++;
		switch(_tcslen(p)) {
		case 3:		// rgb
			if ( _stscanf(p, _T("%01x%01x%01x"), &r, &g, &b) == 3 )
				col = RGB(RoundMulDiv(r, 15), RoundMulDiv(g, 15), RoundMulDiv(b, 15));
			break;
		case 6:		// rrggbb
			if ( _stscanf(p, _T("%02x%02x%02x"), &r, &g, &b) == 3 )
				col = RGB(r, g, b);
			break;
		case 9:		// rrrgggbbb
			if ( _stscanf(p, _T("%03x%03x%03x"), &r, &g, &b) == 3 )
				col = RGB(RoundMulDiv(r, 4095), RoundMulDiv(g, 4095), RoundMulDiv(b, 4095));
			break;
		case 12:	// rrrrggggbbbb
			if ( _stscanf(p, _T("%04x%04x%04x"), &r, &g, &b) == 3 )
				col = RGB(RoundMulDiv(r, 65535), RoundMulDiv(g, 65535), RoundMulDiv(b, 65535));
			break;
		}
	} else if ( _tcsnicmp(p, _T("rgb("), 4) == 0 ) {
		p += 4;
		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		for ( r = 0 ; *p >= _T('0') && *p <= _T('9') ; p++ )
			r = r * 10 + (*p - '0');
		if ( *p == _T('%') ) {
			p++;
			r = r * 255 / 100;
		}
		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		if ( *p == _T(',') )
			p++;

		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		for ( g = 0 ; *p >= _T('0') && *p <= _T('9') ; p++ )
			g = g * 10 + (*p - '0');
		if ( *p == _T('%') ) {
			p++;
			g = g * 255 / 100;
		}
		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		if ( *p == _T(',') )
			p++;

		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
		for ( b = 0 ; *p >= _T('0') && *p <= _T('9') ; p++ )
			b = b * 10 + (*p - '0');
		if ( *p == _T('%') ) {
			p++;
			b = b * 255 / 100;
		}

		if ( r > 255 ) r = 255;
		if ( g > 255 ) g = 255;
		if ( b > 255 ) b = 255;

		col = RGB(r, g, b);

	} else if ( m_pTextRam != NULL )
		m_pTextRam->ParseColorName(p, col);

	if ( col != (-1) ) {
		int i;
		DWORD min = 0xFFFFFF;	// 255 * 255 * 2 + 255 * 255 * 6 + 255 * 255 = 585225 = 0x0008EE09

		r = GetRValue(col);
		g = GetGValue(col);
		b = GetBValue(col);

		for ( i = 0 ; i < 16 ; i++ ) {
			if ( col == TekColorTab[i] ) {
				idx = i;
				break;
			}
			int nr = GetRValue(TekColorTab[i]) - r;
			int ng = GetGValue(TekColorTab[i]) - g;
			int nb = GetBValue(TekColorTab[i]) - b;
			DWORD d = nr * nr * 2 + ng * ng * 6 + nb * nb;
			if ( min > d ) {
				idx = i;
				min = d;
			}
		}
	}

	return idx;
}
int CTekWnd::SvgToTekSt(CStringIndex &xml)
{
	// st
	// 0xxxx000		PenColor	{ RGB(128, 128, 128), RGB(  0,   0,   0), RGB(192, 0, 0), RGB(0, 192, 0), RGB(0, 0, 192), RGB(0, 192, 192), RGB(192, 0, 192), RGB(192, 192, 0), RGB(192, 192, 192), RGB( 64,  64,  64), RGB( 96, 0, 0), RGB(0,  96, 0), RGB(0, 0,  96), RGB(0,  96,  96), RGB( 96, 0,  96), RGB( 96,  96, 0) }
	// x0000000		PenWidth	{ 1, 2 }
	// 00000xxx		PenType		{ "none","1,1", "3,1,1,1","2,1","4,2","4,1,1,1,1,1","6,1,1,1","6,3" };

	// stroke="#606000" or stroke="green"
	// stroke-width="0.25"
	// stroke-dasharray="none"

	int n;
	LPCTSTR p;
	int st = 0;
	BYTE ptn = 0;
	CArray<double, double> param;

	if ( ((p = SvgTagStr(xml, _T("stroke"))) != NULL && *p != _T('\0')) || ((p = SvgTagStr(xml, _T("fill"))) != NULL && *p != _T('\0')) ) {
		int idx = SvgToTekColor(p);
		if ( idx >= 0 )
			st = (st & 0x87) | (BYTE)(idx << 3);
	}

	n = (int)(SvgLength(SvgTagStr(xml, _T("stroke-width"))) * TEK_WIN_WIDTH / m_Width + 0.5);
	if ( n > 5 )
		st = (st & 0x7F ) | 0x80;

	SvgParam(SvgTagStr(xml, _T("stroke-dasharray")), param, _T("l"));

	if ( param.GetSize() > 0 ) {
		//							  0      1      2          3     4     5             6         7
		// 00000xxx		PenType		{ "none","1,1", "3,1,1,1","2,1","4,2","4,1,1,1,1,1","6,1,1,1","6,3" };
		if ( param.GetSize() >= 6 && (param[0] / param[1]) > 2.0 &&
				absd(param[1] - param[2]) <= 0.1	&& absd(param[1] - param[3]) <= 0.1 &&
				absd(param[1] - param[4]) <= 0.1	&& absd(param[1] - param[5]) <= 0.1 )
			ptn = 5;
		else if ( param.GetSize() >= 4 && (param[0] / param[1]) > 5.0 &&
				absd(param[1] - param[2]) <= 0.1	&& absd(param[1] - param[3]) <= 0.1 )
			ptn = 6;
		else if ( param.GetSize() >= 4 && (param[0] / param[1]) > 2.0 &&
				absd(param[1] - param[2]) <= 0.1	&& absd(param[1] - param[3]) <= 0.1 )
			ptn = 2;
		else if ( param.GetSize() >= 2 && (param[0] / param[1]) > 5.0 )
			ptn = 6;
		else if ( param.GetSize() >= 2 && (param[0] / param[1]) > 4.0 )
			ptn = 5;
		else if ( param.GetSize() >= 2 && (param[0] / param[1]) > 3.0 )
			ptn = 2;
		else if ( param.GetSize() >= 2 && (param[0] / param[1]) > 2.0 )
			ptn = 3;
		else if ( param.GetSize() >= 1 )
			ptn = (param[0] > 5.0 ? 7 : (param[0] > 3.0 ? 4 : (param[0] > 2.0 ? 3 : 1)));

		st = (st & 0xF8) | ptn;
	}

	SvgTransFormClear();
	SvgTransForm(&xml);

	return st;
}
void CTekWnd::SvgToTekLine(double sx, double sy, double ex, double ey, int st)
{
	CTextRam::TEKNODE *tp = m_pTextRam->TekAlloc();

	SvgTransFromCalc(sx, sy);
	SvgTransFromCalc(ex, ey);

	tp->md = 0;
	tp->st = (BYTE)st;
	tp->sx = (short)((sx - m_OfsX) * TEK_WIN_WIDTH / m_Width + 0.5);
	tp->sy = TEK_WIN_HEIGHT - (short)((sy - m_OfsY) * TEK_WIN_HEIGHT / m_Height + 0.5);
	tp->ex = (short)((ex - m_OfsX) * TEK_WIN_WIDTH / m_Width + 0.5);
	tp->ey = TEK_WIN_HEIGHT - (short)((ey - m_OfsY) * TEK_WIN_HEIGHT / m_Height + 0.5);
	tp->ch = 0;

	if ( tp->sx == tp->ex && tp->sy == tp->ey )
		m_pTextRam->TekFree(tp);
	else
		m_pTextRam->TekAdd(tp);
}
void CTekWnd::SvgQuadBezier(double sx, double sy, double bx, double by, double ex, double ey, int st)
{
	int c, m;
	double t;
	double tx, ty;
	double dx = sx, dy = sy;

	t = 0.5;
	tx = pow(1.0 - t, 2) * sx + 2.0 * (1.0 - t) * t * bx + pow(t, 2) * ex;
	ty = pow(1.0 - t, 2) * sy + 2.0 * (1.0 - t) * t * by + pow(t, 2) * ey;

	m = abs((int)(sqrt(pow(sx - tx, 2) + pow(sy - ty, 2)) * TEK_WIN_WIDTH / m_Width + 0.5)) / SVG_BEZ_DIVS;

	if ( m < 2 ) {
		SvgToTekLine(dx, dy, tx, ty, st);

		dx = tx;
		dy = ty;

	} else {
		for ( c = 1 ; c < m ; c++ ) {
			t = (double)c / (double)m;
			tx = pow(1.0 - t, 2) * sx + 2.0 * (1.0 - t) * t * bx + pow(t, 2) * ex;
			ty = pow(1.0 - t, 2) * sy + 2.0 * (1.0 - t) * t * by + pow(t, 2) * ey;

			SvgToTekLine(dx, dy, tx, ty, st);

			dx = tx;
			dy = ty;
		}
	}

	SvgToTekLine(dx, dy, ex, ey, st);
}
void CTekWnd::SvgCubicBezier(double sx, double sy, double bx1, double by1, double bx2, double by2, double ex, double ey, int st)
{
	int c, m;
	double t;
	double tx, ty;
	double dx = sx, dy = sy;

	t = 0.5;
	tx = pow(1.0 - t, 3) * sx + 3.0 * pow(1.0 - t, 2) * t * bx1 + 3.0 * (1.0 - t) * pow(t, 2) * bx2 + pow(t, 3) * ex;
	ty = pow(1.0 - t, 3) * sy + 3.0 * pow(1.0 - t, 2) * t * by1 + 3.0 * (1.0 - t) * pow(t, 2) * by2 + pow(t, 3) * ey;

	m = abs((int)(sqrt(pow(sx - tx, 2) + pow(sy - ty, 2)) * TEK_WIN_WIDTH / m_Width + 0.5)) / SVG_BEZ_DIVS;

	if ( m < 2 ) {
		SvgToTekLine(dx, dy, tx, ty, st);

		dx = tx;
		dy = ty;

	} else if ( m < 20 ) {
		for ( c = 1 ; c < m ; c++ ) {
			t = (double)c / (double)m;
			tx = pow(1.0 - t, 3) * sx + 3.0 * pow(1.0 - t, 2) * t * bx1 + 3.0 * (1.0 - t) * pow(t, 2) * bx2 + pow(t, 3) * ex;
			ty = pow(1.0 - t, 3) * sy + 3.0 * pow(1.0 - t, 2) * t * by1 + 3.0 * (1.0 - t) * pow(t, 2) * by2 + pow(t, 3) * ey;

			SvgToTekLine(dx, dy, tx, ty, st);

			dx = tx;
			dy = ty;
		}
	}

	SvgToTekLine(dx, dy, ex, ey, st);
}

void CTekWnd::SvgLine(CStringIndex &xml)
{
	// <line x1="76.75" y1="580.5" x2="179" y2="580.5" stroke="#606000" stroke-dasharray="none" stroke-width="0.25" />

	int st = SvgToTekSt(xml);

	SvgToTekLine(SvgLength(SvgTagStr(xml, _T("x1"))), SvgLength(SvgTagStr(xml, _T("y1"))), SvgLength(SvgTagStr(xml, _T("x2"))), SvgLength(SvgTagStr(xml, _T("y2"))), st);
}
void CTekWnd::SvgPolyline(CStringIndex &xml)
{
	// 	<polyline fill="none" stroke="#E60012" stroke-miterlimit="10" points="182.809,142.181 177.05,145.06 171.291,151.299 "/>

	int n, count = 0;
	CArray<double, double> param;
	double x1, y1, x2, y2;
	int st = SvgToTekSt(xml);

	SvgParam(SvgTagStr(xml, _T("points")), param);

	for ( n = 0 ; (n + 1) < param.GetSize() ; n += 2 ) {
		x2 = param[n + 0];
		y2 = param[n + 1];

		if ( count++ > 0 )
			SvgToTekLine(x1, y1, x2, y2, st);

		x1 = x2;
		y1 = y2;
	}
}
void CTekWnd::SvgPolygon(CStringIndex &xml)
{
	// 	<polygon points="50,50 150,50 150,150, 50,150 125,75 150,150" stroke="black" stroke-width="1" fill="lightgreen"/>

	int n, count = 0;
	CArray<double, double> param;
	double x0, y0, x1, y1, x2, y2;
	int st = SvgToTekSt(xml);

	SvgParam(SvgTagStr(xml, _T("points")), param);

	for ( n = 0 ; (n + 1) < param.GetSize() ; n += 2 ) {
		x2 = param[n + 0];
		y2 = param[n + 1];

		if ( count++ > 0 )
			SvgToTekLine(x1, y1, x2, y2, st);
		else {
			x0 = x2;
			y0 = y2;
		}

		x1 = x2;
		y1 = y2;
	}

	if ( n > 0 )
		SvgToTekLine(x0, y0, x2, y2, st);
}
void CTekWnd::SvgRect(CStringIndex &xml)
{
	// <rect x="192.407" y="134.983" fill="none" stroke="#666666" stroke-width="0.5" stroke-miterlimit="10" width="47.991" height="47.991"/>
	// <rect x="50" y="50" rx="15" ry="10" width="120" height="100" stroke="black" stroke-width="1" fill="none"/>

	double x1, y1, x2, y2;
	double rx, ry;
	int st = SvgToTekSt(xml);

	x1 = SvgLength(SvgTagStr(xml, _T("x")));
	y1 = SvgLength(SvgTagStr(xml, _T("y")));
	x2 = x1 + SvgLength(SvgTagStr(xml, _T("width")));
	y2 = y1 + SvgLength(SvgTagStr(xml, _T("height")));

	rx = SvgLength(SvgTagStr(xml, _T("rx")));
	ry = SvgLength(SvgTagStr(xml, _T("ry")));

	if ( rx < 0.0 )
		rx = 0.0 - rx;
	if ( rx > ((x2 - x1) / 2.0) )
		rx = (x2 - x1) / 2.0;

	if ( ry < 0.0 )
		ry = 0.0 - ry;
	if ( ry > ((y2 - y1) / 2.0) )
		ry = (y2 - y1) / 2.0;

	if ( rx != 0.0 || ry != 0.0 ) {
		if ( rx == 0.0 )
			rx = ry;
		if ( ry == 0.0 )
			ry = rx;

		double vx = rx - rx * 0.551778477804468;
		double vy = ry - ry * 0.551778477804468;

		SvgCubicBezier(x1, y1 + ry, x1, y1 + vy, x1 + vx, y1, x1 + rx, y1, st);		// left up
		SvgToTekLine(x1 + rx, y1, x2 - rx, y1, st);									// left up right
		SvgCubicBezier(x2 - rx, y1, x2 - vx, y1, x2, y1 + vy, x2, y1 + ry, st);		// right up
		SvgToTekLine(x2, y1 + ry, x2, y2 - ry, st);									// right up down
		SvgCubicBezier(x2, y2 - ry,	x2, y2 - vy, x2 - vx, y2, x2 - rx, y2, st);		// right down
		SvgToTekLine(x2 - rx, y2, x1 + rx, y2, st);									// right down to left
		SvgCubicBezier(x1 + rx, y2, x1 + vx, y2, x1, y2 - vy, x1, y2 - ry, st);		// left down
		SvgToTekLine(x1, y2 - ry, x1, y1 + ry, st);									// left down to up

	} else {
		SvgToTekLine(x1, y1, x2, y1, st);	// up
		SvgToTekLine(x2, y1, x2, y2, st);	// right
		SvgToTekLine(x2, y2, x1, y2, st);	// down
		SvgToTekLine(x1, y2, x1, y1, st);	// left
	}
}
void CTekWnd::SvgPath(CStringIndex &xml)
{
	// <path fill="#FFFFFF" stroke="#000000" stroke-width="5" d="M832.75,524.25	z"/>

	int n;
	CArray<double, double> param;
	double x, y;
	double zx = 0.0, zy = 0.0;
	double nx = 0.0, ny = 0.0;
	double bx1 = 0.0, by1 = 0.0;
	double bx2 = 0.0, by2 = 0.0;
	LPCTSTR p = SvgTagStr(xml, _T("d"));
	BOOL bAbs = FALSE;
	int st = SvgToTekSt(xml);

	while ( *p != _T('\0') ) {
		bAbs = FALSE;

		switch(*p) {
		case _T('M'):	// Move to
			bAbs = TRUE;
		case _T('m'):
			// (x y)+
			p = SvgParam(++p, param);
			for ( n = 0 ; (n + 1) < (int)param.GetSize() ; n += 2 ) {
				x = param[n + 0];
				y = param[n + 1];
				if ( !bAbs ) {
					x += nx;
					y += ny;
				}

				if ( n > 0 )
					SvgToTekLine(nx, ny, x, y, st);
				else {
					zx = x;
					zy = y;
				}

				nx = bx1 = x;
				ny = by1 = y;
			}
			break;

		case _T('L'):	// Line to
			bAbs = TRUE;
		case _T('l'):
			// (x y)+
			p = SvgParam(++p, param);
			for ( n = 0 ; (n + 1) < (int)param.GetSize() ; n += 2 ) {
				x = param[n + 0];
				y = param[n + 1];
				if ( !bAbs ) {
					x += nx;
					y += ny;
				}
				SvgToTekLine(nx, ny, x, y, st);
				nx = bx1 = x;
				ny = by1 = y;
			}
			break;

		case _T('H'):	// Horizontal Line To
			bAbs = TRUE;
		case _T('h'):
			// (x)+
			p = SvgParam(++p, param);
			for ( n = 0 ; n < (int)param.GetSize() ; n++ ) {
				x = param[n];
				if ( !bAbs )
					x += nx;
				SvgToTekLine(nx, ny, x, ny, st);
				nx = bx1 = x;
			}
			break;

		case _T('V'):	// Vertical Line To
			bAbs = TRUE;
		case _T('v'):
			// (x)+
			p = SvgParam(++p, param);
			for ( n = 0 ; n < (int)param.GetSize() ; n++ ) {
				y = param[n];
				if ( !bAbs )
					y += ny;
				SvgToTekLine(nx, ny, nx, y, st);
				ny = by1 = y;
			}
			break;

		case _T('Z'):	// Close Path
		case _T('z'):
			p = SvgParam(++p, param);
			SvgToTekLine(nx, ny, zx, zy, st);
			nx = bx1 = zx;
			ny = by1 = zy;
			break;

		case _T('Q'):	// Quadratic Bezier curve	2次ベジエ曲線
			bAbs = TRUE;
		case _T('q'):
			// (x1 y1 x y)+
			p = SvgParam(++p, param);
			for ( n = 0 ; (n + 3) < (int)param.GetSize() ; n += 4 ) {
				bx1 = param[n + 0];
				by1 = param[n + 1];
				if ( !bAbs ) {
					bx1 += nx;
					by1 += ny;
				}
				x = param[n + 2];
				y = param[n + 3];
				if ( !bAbs ) {
					x += nx;
					y += ny;
				}

				SvgQuadBezier(nx, ny, bx1, by1, x, y, st);

				nx = x;
				ny = y;
				bx1 = nx + (nx - bx1);
				by1 = ny + (ny - by1);
			}
			break;
		case _T('T'):	// Smooth
			bAbs = TRUE;
		case _T('t'):
			// (x y)+
			p = SvgParam(++p, param);
			for ( n = 0 ; (n + 1) < (int)param.GetSize() ; n += 2 ) {
				x = param[n + 0];
				y = param[n + 1];
				if ( !bAbs ) {
					x += nx;
					y += ny;
				}

				SvgQuadBezier(nx, ny, bx1, by1, x, y, st);

				nx = x;
				ny = y;
				bx1 = nx + (nx - bx1);
				by1 = ny + (ny - by1);
			}
			break;

		case _T('C'):	// Cubic Bezier curve		3次ベジエ曲線
			bAbs = TRUE;
		case _T('c'):
			// (x1 y1 x2 y2 x y)+
			p = SvgParam(++p, param);
			for ( n = 0 ; (n + 5) < (int)param.GetSize() ; n += 6 ) {
				bx1 = param[n + 0];
				by1 = param[n + 1];
				if ( !bAbs ) {
					bx1 += nx;
					by1 += ny;
				}
				bx2 = param[n + 2];
				by2 = param[n + 3];
				if ( !bAbs ) {
					bx2 += nx;
					by2 += ny;
				}
				x = param[n + 4];
				y = param[n + 5];
				if ( !bAbs ) {
					x += nx;
					y += ny;
				}

				SvgCubicBezier(nx, ny, bx1, by1, bx2, by2, x, y, st);

				nx = x;
				ny = y;
				bx1 = nx + (nx - bx2);
				by1 = ny + (ny - by2);
			}
			break;
		case _T('S'):	// Smooth
			bAbs = TRUE;
		case _T('s'):
			// (x2 y2 x y)+
			p = SvgParam(++p, param);
			for ( n = 0 ; (n + 3) < (int)param.GetSize() ; n += 4 ) {
				bx2 = param[n + 0];
				by2 = param[n + 1];
				if ( !bAbs ) {
					bx2 += nx;
					by2 += ny;
				}
				x = param[n + 2];
				y = param[n + 3];
				if ( !bAbs ) {
					x += nx;
					y += ny;
				}

				SvgCubicBezier(nx, ny, bx1, by1, bx2, by2, x, y, st);

				nx = x;
				ny = y;
				bx1 = nx + (nx - bx2);
				by1 = ny + (ny - by2);
			}
			break;

		case _T('A'):	// Elliptical Arc Curves
			bAbs = TRUE;
		case _T('a'):
			// (rx ry x-axis-rotation large-arc-flag sweep-flag x y)+
			p = SvgParam(++p, param, _T("fff00ff"));
			for ( n = 0 ; (n + 6) < (int)param.GetSize() ; n += 7 ) {
				double rx = param[n + 0];
				double ry = param[n + 1];
				double angle = param[n + 2];
				int large = (int)(param[n + 3] + 0.5);
				int sweep = (int)(param[n + 4] + 0.5);
				x = param[n + 5];
				y = param[n + 6];
				if ( !bAbs ) {
					x += nx;
					y += ny;
				}

				if ( rx == 0.0 || ry == 0.0 ) {
					SvgToTekLine(nx, ny, x, y, st);

				} else {
					angle = M_PI / 180.0 * angle;

					// Calculate the whatchamacallit. F.6.5.1
					double hx1 = ((nx - x) / 2.0);
					double hy1 = ((ny - y) / 2.0);
					double hx =  hx1 * cos(angle) + hy1 * sin(angle);
					double hy = -hx1 * sin(angle) + hy1 * cos(angle);

					// correct out of range radii F.6.6
					double validateRadii = (hx * hx) / (rx * rx) + (hy * hy) / (ry * ry);
					if ( validateRadii > 1.0 ) {
						rx = rx * sqrt(validateRadii);
						ry = ry * sqrt(validateRadii);
					}

					// start calculating F.6.5.2
					double RX2(rx * rx);
					double RY2(ry * ry);
					double HX2(hx * hx);
					double HY2(hy * hy);

					double k = (RX2 * RY2 - RX2 * HY2 - RY2 * HX2) / (RX2 * HY2 + RY2 * HX2);
					k = sqrt(absd(k)) * (large == sweep ? -1.0 : 1.0);

					double cx1 = k * (rx * hy / ry);
					double cy1 = k * (-ry * hx / rx);

					// center of ellipse F.6.5.3
					double cx = cx1 * cos(angle) + -cy1 * sin(angle);
					double cy = cx1 * sin(angle) +  cy1 * cos(angle);
					cx += (nx + x) / 2.0;
					cy += (ny + y) / 2.0;

					// calculate angles F.6.5.4 to F.6.5.6
					double tx1 = (nx - cx);
					double ty1 = (ny - cy);
					double tx = cx +  tx1 * cos(angle) + ty1 * sin(angle);
					double ty = cy + -tx1 * sin(angle) + ty1 * cos(angle);
					double aS = (ty - cy) / ry;

					tx1 = (x - cx);
					ty1 = (y - cy);
					tx = cx +  tx1 * cos(angle) + ty1 * sin(angle);
					ty = cy + -tx1 * sin(angle) + ty1 * cos(angle);
					double aE = (ty - cy) / ry;

					// preventing out of range errors with asin due to floating point errors
					aS = min(aS, 1.0);
					aS = max(aS, -1.0);

					aE = min(aE, 1.0);
					aE = max(aE, -1.0);

					// get the angle 
					double startAngle = asin(aS);
					double endAngle   = asin(aE);

					if ( nx < cx )
						startAngle = M_PI - startAngle;

					if ( x < cx )
						endAngle = M_PI - endAngle;

					if ( startAngle < 0.0 )
						startAngle = M_PI * 2.0 + startAngle;

					if ( endAngle < 0.0 )
						endAngle = M_PI * 2.0 + endAngle;

					if ( sweep && startAngle > endAngle )
						startAngle = startAngle - M_PI * 2.0;

					if ( !sweep && endAngle > startAngle )
						endAngle = endAngle - M_PI * 2.0;

					double tA = (endAngle + startAngle) / 2;
					tx1 = rx * cos(tA);
					ty1 = ry * sin(tA);
					tx = cx + tx1 * cos(angle) + -ty1 * sin(angle);
					ty = cy + tx1 * sin(angle) +  ty1 * cos(angle);

					int m = abs((int)(sqrt(pow(nx - tx, 2) + pow(ny - ty, 2)) * TEK_WIN_WIDTH / m_Width + 0.5)) / SVG_BEZ_DIVS;

					if ( m < 2 )
						m = 2;
					else if ( m > 180 )
						m = 180;

					double sx = nx;
					double sy = ny;

					for ( int c = 1 ; c < m ; c++ ) {
						double sA = startAngle + (endAngle - startAngle) * c / m;
						double dx1 = rx * cos(sA);
						double dy1 = ry * sin(sA);
						double dx = cx + dx1 * cos(angle) + -dy1 * sin(angle);
						double dy = cy + dx1 * sin(angle) +  dy1 * cos(angle);

						SvgToTekLine(sx, sy, dx, dy, st);

						sx = dx;
						sy = dy;
					}

					SvgToTekLine(sx, sy, x, y, st);
				}

				nx = bx1 = x;
				ny = by1 = y;
			}
			break;

		default:		// skip next command
			while ( *p != _T('\0') && *p <= _T(' ') || _tcschr(_T("MmLlHhVvZzQqTtCcSsAa"), *p) == NULL )
				p++;
			break;
		}
	}
}
void CTekWnd::SvgCircle(CStringIndex &xml)
{
	// <circle cx="70" cy="60" r="50" stroke="black" stroke-width="1" fill="yellow"/>

	int n, m;
	double cx, cy, r, a;
	double sx, sy, x, y;
	int st = SvgToTekSt(xml);

	cx = SvgLength(SvgTagStr(xml, _T("cx")));
	cy = SvgLength(SvgTagStr(xml, _T("cy")));
	r  = SvgLength(SvgTagStr(xml, _T("r")));

	m = abs((int)(r * TEK_WIN_WIDTH / m_Width + 0.5)) / 2;

	if ( m < 5 )
		m = 5;
	else if ( m > 180 )
		m = 180;

	for ( n = 0 ; n <= m ; n++ ) {
		a = M_PI * 2 * n / m;
		x = cx + r * cos(a);
		y = cy + r * sin(a);

		if ( n > 0 )
			SvgToTekLine(sx, sy, x, y, st);

		sx = x;
		sy = y;
	}
}
void CTekWnd::SvgEllipse(CStringIndex &xml)
{
    // <ellipse cx="120" cy="140" rx="50" ry="30" stroke="black" stroke-width="1" fill="yellow"/>

	int n, m;
	double cx, cy, rx, ry, a;
	double sx, sy, x, y;
	int st = SvgToTekSt(xml);

	cx = SvgLength(SvgTagStr(xml, _T("cx")));
	cy = SvgLength(SvgTagStr(xml, _T("cy")));
	rx = SvgLength(SvgTagStr(xml, _T("rx")));
	ry = SvgLength(SvgTagStr(xml, _T("ry")));

	m = abs((int)(rx * TEK_WIN_WIDTH / m_Width + 0.5)) / 2;

	if ( m < 5 )
		m = 5;
	else if ( m > 180 )
		m = 180;

	for ( n = 0 ; n <= m ; n++ ) {
		a = M_PI * 2 * n / m;
		x = cx + rx * cos(a);
		y = cy + ry * sin(a);

		if ( n > 0 )
			SvgToTekLine(sx, sy, x, y, st);

		sx = x;
		sy = y;
	}
}
void CTekWnd::SvgSubText(double param[7], int st, int wmd, LPCTSTR p, CArray<double, double> list[5])
{
	CDC TempDC;
	CFont font, *pOldFont;
	CString str;
	CSize sz;

	TempDC.CreateCompatibleDC(NULL);
	font.CreateFont(0 - TekFonts[st & 7][1], 0, 0, 0, FW_DONTCARE, 0, 0, 0, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
	pOldFont = TempDC.SelectObject(&font);

	while ( *p != _T('\0') ) {
		//  x=0  y=1  dx=2 dy=3 rotate=4
		for ( int i = 0 ; i < 5 ; i++ ) {
			if ( list[i].GetSize() > 0 ) {
				param[i] = list[i][0];
				list[i].RemoveAt(0, 1);
			}
		}

		double fx = param[5];
		double fy = param[6];
		double angle = param[4];
		double sx = param[0] + param[2], sy = param[1] + param[3];

		SvgTransFromAngle(angle);
		SvgTransFromSize(fx, fy);
		SvgTransFromCalc(sx, sy);

		int fw = (int)(fx * TEK_WIN_WIDTH  / m_Width  + 0.5);
		int fh = (int)(fy * TEK_WIN_HEIGHT / m_Height + 0.5);

		if ( fh != TekFonts[st & 7][1] ) {
			int idx = 0;
			int dim = 0x7FFFFFFF;
			for ( int n = 0 ; n < 8 ; n++ ) {
				int d = abs(TekFonts[n][0] * TekFonts[n][1] - fw * fh);
				if ( dim > d ) {
					idx = n;
					dim = d;
				}
			}
			st = (st & 0xF8) | idx;
		}

		// TRACE("SelectFont %d(%d,%d) <- (%d,%d) (%g,%g)->(%g,%g)\n", st & 7, TekFonts[st & 7][0], TekFonts[st & 7][1], fw, fh, param[5], param[6], fx, fy);

		CTextRam::TEKNODE *tp = m_pTextRam->TekAlloc();

		tp->md = 1;
		tp->st = st;
		tp->sx = (short)((sx - m_OfsX) * TEK_WIN_WIDTH / m_Width + 0.5);
		tp->sy = TEK_WIN_HEIGHT - (short)((sy - m_OfsY) * TEK_WIN_HEIGHT / m_Height + 0.5) - TekFonts[st & 7][1] / 8;
		tp->ex = (short)(360 - angle) % 360;
		tp->ey = 0;

		if ( (p[0] & 0xFC00) == 0xD800 && (p[1] & 0xFC00) == 0xDC00 ) {
			tp->ch = ((DWORD)p[0] << 16) | p[1];
			str = *(p++);
			str += *(p++);
		} else {
			tp->ch = (DWORD)*p;
			str = *(p++);
		}

		sz = TempDC.GetTextExtent(str, 1);

		m_pTextRam->TekAdd(tp);

		switch(wmd) {
		case 0:		// lr
			param[0] += (sz.cx * m_Width / TEK_WIN_WIDTH);
			break;
		case 1:		// rl
			param[0] -= (sz.cx * m_Width / TEK_WIN_WIDTH);
			break;
		case 2:		// tb
			param[1] += (sz.cy * m_Height / TEK_WIN_HEIGHT);
			break;
		}
	}

	switch(wmd) {
	case 0:		// lr
	case 1:		// rl
		param[1] += param[3];	// y += dy
		break;
	case 2:		// tb
		param[0] += param[2];	// x += dx
		break;
	}

	TempDC.SelectObject(pOldFont);
	TempDC.DeleteDC();
}
void CTekWnd::SvgToTextSt(CStringIndex &xml, int &st, int &wmd, double &fw, double &fh)
{
	int n;
	LPCTSTR p;

	// tp->st
	// 00000xxx		FontSize
	// 0xxxx000		TextColor

	if ( (p = SvgTagStr(xml, _T("font-size"))) != NULL && *p != _T('\0') ) {
		fh = SvgLength(p);
		fw = fh * 0.635;	// TekFonts w/h average
	}

	if ( (n = xml.Find(_T("fill"))) >= 0 )
		p = xml[n];
	else if ( (n = xml.Find(_T("stroke"))) >= 0 )
		p = xml[n];
	else if ( ((p = SvgTagStr(xml, _T("fill"))) != NULL && *p == _T('\0')) || ((p = SvgTagStr(xml, _T("stroke"))) != NULL && *p == _T('\0')) )
		p = NULL;

	if ( p != NULL ) {
		int idx = SvgToTekColor(p);
		if ( idx >= 0 )
			st = (st & 0x87) | (BYTE)(idx << 3);
	}

	if ( (p = SvgTagStr(xml, _T("writing-mode"))) != NULL && *p != _T('\0') ) {
		if ( _tcsicmp(p, _T("lr-tb")) == 0 || _tcsicmp(p, _T("lr")) == 0 )
			wmd = 0;
		else if ( _tcsicmp(p, _T("rl-tb")) == 0 || _tcsicmp(p, _T("rl")) == 0 )
			wmd = 1;
		else if ( _tcsicmp(p, _T("tb-rl")) == 0 || _tcsicmp(p, _T("tb")) == 0 )
			wmd = 2;
	}
}
void CTekWnd::SvgTspan(CStringIndex &xml, double param[7], CArray<double, double> list[5])
{
	int n, i;
	int st = 0;
	int wmd = 0;
	CArray<double, double> tmp;
	double save_FontWidth = m_FontWidth;
	double save_FontHeight = m_FontHeight;
	static const LPCTSTR ParamList[] = { _T("x"), _T("y"), _T("dx"), _T("dy"), _T("rotate") };

	SvgToTextSt(xml, st, wmd, m_FontWidth, m_FontHeight);

	param[5] = m_FontWidth;
	param[6] = m_FontHeight;

	for ( n = 0 ; n < 5 ; n++ ) {
		if ( (i = xml.Find(ParamList[n])) >= 0 ) {
			SvgParam(xml[i], tmp, (n == 4 ? _T("r") : _T("l")));
			for ( i = 0 ; i < tmp.GetSize() ; i++ )
				list[n].InsertAt(i, tmp[i]);
		}
	}

	SvgTransFormClear();
	SvgTransForm(&xml);

	for ( n = 0 ; n < (int)xml.GetSize() ; n++ ) {
		if ( !xml[n].m_bChild )
			continue;

		if ( xml[n].m_nIndex.IsEmpty() )
			SvgSubText(param, st, wmd, (LPCTSTR)xml[n], list);
		else if ( xml[n].m_nIndex.Compare(_T("tspan")) == 0 )
			SvgTspan(xml[n], param, list);
		else if ( xml[n].m_nIndex.Compare(_T("tref")) == 0 )
			SvgSubText(param, st, wmd, (LPCTSTR)xml[n], list);
	}

	SvgSubText(param, st, wmd, (LPCTSTR)xml, list);

	param[5] = m_FontWidth  = save_FontWidth;
	param[6] = m_FontHeight = save_FontHeight;
}
void CTekWnd::SvgText(CStringIndex &xml)
{
	// <text x="868.75 881.5" y="672" dy="-2.53125" font-family="monospace" font-size="20.25" fill="#000000" rotate="0">&#32;123</text>

	int n;
	int st = 0;
	int wmd = 0;
	double param[7] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	CArray<double, double> list[5];
	double save_FontWidth = m_FontWidth;
	double save_FontHeight = m_FontHeight;

	SvgToTextSt(xml, st, wmd, m_FontWidth, m_FontHeight);

	param[5] = m_FontWidth;
	param[6] = m_FontHeight;

	SvgParam(SvgTagStr(xml, _T("x")), list[0], _T("l"));
	SvgParam(SvgTagStr(xml, _T("y")), list[1], _T("l"));
	SvgParam(SvgTagStr(xml, _T("dx")), list[2], _T("l"));
	SvgParam(SvgTagStr(xml, _T("dy")), list[3], _T("l"));
	SvgParam(SvgTagStr(xml, _T("rotate")), list[4], _T("r"));

	SvgTransFormClear();
	SvgTransForm(&xml);

	for ( n = 0 ; n < (int)xml.GetSize() ; n++ ) {
		if ( !xml[n].m_bChild )
			continue;

		SvgStyleCheck(xml, xml[n].m_nIndex);

		if ( xml[n].m_nIndex.IsEmpty() )
			SvgSubText(param, st, wmd, (LPCTSTR)xml[n], list);
		else if ( xml[n].m_nIndex.Compare(_T("tspan")) == 0 )
			SvgTspan(xml[n], param, list);
		else if ( xml[n].m_nIndex.Compare(_T("tref")) == 0 )
			SvgSubText(param, st, wmd, (LPCTSTR)xml[n], list);
	}

	SvgSubText(param, st, wmd, (LPCTSTR)xml, list);

	m_FontWidth  = save_FontWidth;
	m_FontHeight = save_FontHeight;
}

LPCTSTR CTekWnd::SvgStyleSkip(LPCTSTR p)
{
	while ( *p != _T('\0') && *p <= _T(' ') )
		p++;

	while ( p[0] == _T('/') && p[1] == _T('*') ) {
		for ( p += 2 ; *p != _T('\0') ; p++ ) {
			if ( p[0] == _T('*') && p[1] == _T('/') ) {
				p += 2;
				break;
			}
		}
		while ( *p != _T('\0') && *p <= _T(' ') )
			p++;
	}

	return p;
}
void CTekWnd::SvgStyleProp(CStringIndex &xml, LPCTSTR p)
{
	CString tmp, value;

	while ( *p != _T('\0') ) {
		p = SvgStyleSkip(p);

		tmp.Empty();
		while ( *p != _T('\0') ) {
			if ( *p <= _T(' ') || *p == _T(':') || *p == _T(';') )
				break;
			else
				tmp += *(p++);
		}

		p = SvgStyleSkip(p);
		if ( *p == _T(':') ) {
			p = SvgStyleSkip(p + 1);

			value.Empty();
			while ( *p != _T('\0') ) {
				if ( *p <= _T(' ') || *p == _T(';') )
					break;
				else
					value += *(p++);
			}

			p = SvgStyleSkip(p);
			if ( *p == _T(';') )
				p++;

			if ( !tmp.IsEmpty() && !value.IsEmpty() )
				xml[tmp] = value;
		} else	// if ( *p == _T(';') )
			p++;
	}
}
void CTekWnd::SvgStyleSet(CStringIndex &xml, LPCTSTR pBase, LPCTSTR pClass, LPCTSTR pPrefix)
{
	int n, i;
	CString tmp, name;

	while ( *pClass != _T('\0') ) {
		while ( *pClass != _T('\0') && *pClass <= _T(' ') )
			pClass++;

		tmp.Empty();
		while ( *pClass != _T('\0') && *pClass > _T(' ') )
			tmp += *(pClass++);

		if ( !tmp.IsEmpty() ) {
			name.Format(_T("%s%s"), (pPrefix == NULL ? _T("") : pPrefix), tmp);

			if ( (n = m_Style.Find(pBase)) >=0 && (i = m_Style[n].Find(name)) >= 0 )
				SvgStyleProp(xml, m_Style[n][i]);
		}
	}
}
void CTekWnd::SvgStyleCheck(CStringIndex &xml, LPCTSTR pBase)
{
	// base > id > class 
	// <rect />						[*][*]
	// <rect class="class"/>		[*][class]
	// <rect id="#id" />			[*][#id]
	// <rect />						[rect][*]
	// <rect class="class"/>		[rect][class]
	// <rect id="rect#id" />		[rect][#id]
	// <rect style="fill: red;"/>

	int i = xml.Find(_T("id"));
	int c = xml.Find(_T("class"));

	SvgStyleSet(xml, _T("*"), _T("*"));

	if ( c >= 0 )
		SvgStyleSet(xml, _T("*"), (LPCTSTR)xml[c], _T("."));

	if ( i >= 0 )
		SvgStyleSet(xml, _T("*"), (LPCTSTR)xml[i], _T("#"));

	SvgStyleSet(xml, pBase, _T("*"));

	if ( c >= 0 )
		SvgStyleSet(xml, pBase, (LPCTSTR)xml[c], _T("."));

	if ( i >= 0 )
		SvgStyleSet(xml, pBase, (LPCTSTR)xml[i], _T("#"));

	if ( (i = xml.Find(_T("style"))) >= 0 )
		SvgStyleProp(xml, (LPCTSTR)xml[i]);
}
LPCTSTR CTekWnd::SvgStyleParse(LPCTSTR p, BOOL bSkip)
{
	// ID selector
	// #a { color: #ff3300; font-size: 24px }
	// index["*"]["#a"]= "color: #ff3300; font-size: 24px"

	// class selector
	// circle.a { fill:red; }
	// index["circle"][".a"] = "fill:red"

	// type selector
	// circle { fill:red; }
	// index["circle"]["*"] = "fill:red"

	// universal selector
	// * { fill:red; }
	// index["*"]["*"] = "fill:red"

	// selector list
	// #a,#b{ fill:yellow; stroke:green;}
	// index["*"]["#a"] = "fill:yellow; stroke:green;"
	// index["*"]["#b"] = "fill:yellow; stroke:green;"

	// not support...
	// next-sibling combinator			img + p { font-weight: bold; }
	// child combinator					ul > li { margin: 2em; }
	// column combinator				col.selected || td { background: gray;}
	// subsequent-sibling combinator	img ~ p { color: red; }
	// descendant combinator			ul li { margin: 2em; }
	// namespace separator				* | a { color: red; }
	// pseudo-classes					#d:hover { fill: pink; }
	// pseudo-classes with param		p:is(:valid, :unsupported) { color: red; }
	// attribute selector				a[title] { color: purple; }
	// nesting selector					a { & b { color: purple; } }

	int n;
	CString tmp, base, name;
	CStringArray list[2];
	LPCTSTR s;
	BOOL bNotSupport = FALSE;

	while ( *p != _T('\0') ) {

		p = SvgStyleSkip(p);

		if ( *p == _T('}') )
			break;

		base.Empty();
		name.Empty();

		BOOL bDescendant = FALSE;
		while ( *p != _T('\0') ) {
			if ( *p == _T('{') || *p == _T(',') )
				break;
			else if ( *p <= _T(' ') ) {
				p = SvgStyleSkip(p);
				bDescendant = TRUE;
				continue;
			} else if ( _tcschr(_T("+>|~&"), *p) != NULL ) {
				p = SvgStyleSkip(p);
				base.Empty();
				name.Empty();
				bDescendant = FALSE;
				bNotSupport = TRUE;
				continue;
			} else if ( *p == _T(':') ) {
				p = SvgStyleSkip(p + 1);
				bDescendant = FALSE;
				bNotSupport = TRUE;
			} else if ( *p == _T('(') ) {
				while ( *p != _T('\0') && *p != _T(')') )
					name += *(p++);
				bNotSupport = TRUE;
			} else if ( *p == _T('[') ) {
				while ( *p != _T('\0') && *p != _T(']') )
					name += *(p++);
				bNotSupport = TRUE;
			} else if ( *p == _T('.') || *p == _T('#') ) {
				base = name;
				name.Empty();
			} else if ( bDescendant ) {
				base.Empty();
				name.Empty();
				bNotSupport = TRUE;
				bDescendant = FALSE;
			}

			if ( *p != _T('\0') )
				name += *(p++);
		}

		if ( !bSkip && !bNotSupport ) {
			if ( base.IsEmpty() ) {
				if ( !name.IsEmpty() && name[0] != _T('.') && name[0] != _T('#') ) {
					base = name;
					name.Empty();
				} else
					base = _T("*");
			}
			if ( name.IsEmpty() )
				name = _T("*");

			list[0].Add(base);
			list[1].Add(name);
		}

		if ( *p == _T(',') ) {
			p++;
			bNotSupport = FALSE;
			continue;
		} else if ( *p != _T('{') )	// syntax error
			break;
		p = SvgStyleSkip(p + 1);

		s = p;
		tmp.Empty();
		while ( *p != _T('\0') && *p != _T('}') ) {
			if ( *p == _T('{') )
				p = s = SvgStyleParse(s, TRUE);
			else 
				tmp += *(p++);
		}
		if ( *p != _T('}') )		// syntax error
			break;
		p++;

		if ( !tmp.IsEmpty() ) {
			for ( n = 0 ; n < (int)list[0].GetSize() && n < (int)list[1].GetSize() ; n++ )
				m_Style[list[0][n]][list[1][n]] = tmp;
		}

		list[0].RemoveAll();
		list[1].RemoveAll();

		bNotSupport = FALSE;
	}

	return p;
}
void CTekWnd::SvgStyle(CStringIndex &xml)
{
	// <style type="text/css">#a,#b { fill:yellow; stroke:green; } </style>
	// <style type="text/css"><![CDATA[ rect { fill: red; stroke: blue; stroke-width: 3 } ]]></style>

	int n;

	if ( (n = xml.Find(_T("type"))) >= 0 && xml[n].m_String.CompareNoCase(_T("text/css")) != 0 )
		return;

	SvgStyleParse((LPCTSTR)xml);
}

BOOL CTekWnd::SvgCmds(CStringIndex &xml)
{
	SvgStyleCheck(xml, xml.m_nIndex);

	if ( xml.m_nIndex.CompareNoCase(_T("g")) == 0 )
		SvgGroup(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("a")) == 0 )
		SvgGroup(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("svg")) == 0 )
		SvgRoot(xml, FALSE);
	else if ( xml.m_nIndex.CompareNoCase(_T("line")) == 0 )
		SvgLine(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("rect")) == 0 )
		SvgRect(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("polyline")) == 0 )
		SvgPolyline(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("polygon")) == 0 )
		SvgPolygon(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("path")) == 0 )
		SvgPath(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("circle")) == 0 )
		SvgCircle(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("ellipse")) == 0 )
		SvgEllipse(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("text")) == 0 )
		SvgText(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("defs")) == 0 )
		SvgDefs(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("use")) == 0 )
		SvgUse(xml);
	else if ( xml.m_nIndex.CompareNoCase(_T("style")) == 0 )
		SvgStyle(xml);
	else
		return FALSE;

	return TRUE;
}
void CTekWnd::SvgDefs(CStringIndex &xml)
{
	int n, i;

	for ( n = 0 ; n < xml.GetSize() ; n++ ) {
		if ( !xml[n].m_bChild )
			continue;

		if ( (i = xml[n].Find(_T("id"))) >= 0 )
			m_Defs[(LPCTSTR)xml[n][i]].m_pOwner = &(xml[n]);

		if ( xml[n].m_nIndex.CompareNoCase(_T("g")) == 0 )
			SvgDefs(xml[n]);
		else if ( xml.m_nIndex.CompareNoCase(_T("style")) == 0 )
			SvgStyle(xml);
	}
}
void CTekWnd::SvgUse(CStringIndex &xml)
{
	int n;
	double save_OfsX = m_OfsX;
	double save_OfsY = m_OfsY;
	double save_Width  = m_Width;
	double save_Height = m_Height;
	double save_FontWidth = m_FontWidth;
	double save_FontHeight = m_FontHeight;
	CStringIndex *pXml, *pOld, tmp;

	if ( (n = xml.Find(_T("href"))) < 0 && (n = xml.Find(_T("xlink:href"))) < 0 )
		return;

	LPCTSTR p = (LPCTSTR)xml[n];
	while ( *p != _T('\0') && *p != _T('#') )
		p++;

	if ( *(p++) != _T('#') || (n = m_Defs.Find(p)) < 0 )
		return;

	if ( (pXml = m_Defs[n].m_pOwner) == NULL )
		return;

	if ( pXml->m_Value > 0 )
		return;

	// m_Widthとm_Heightが変化する可能性があり元のサイズを保存
	double fw = m_FontWidth  * TEK_WIN_WIDTH  / m_Width;
	double fh = m_FontHeight * TEK_WIN_HEIGHT / m_Height;

	for ( n = 0 ; n < xml.GetSize() ; n++ ) {
		if ( xml[n].m_bChild )
			continue;
		else if ( xml[n].m_nIndex.CompareNoCase(_T("x")) == 0 )
			m_OfsX = m_OfsX - SvgLength(xml[n]);
		else if ( xml[n].m_nIndex.CompareNoCase(_T("y")) == 0 )
			m_OfsY = m_OfsY - SvgLength(xml[n]);
		else if ( xml[n].m_nIndex.CompareNoCase(_T("width")) == 0 )
			m_Width = SvgLength(xml[n]);
		else if ( xml[n].m_nIndex.CompareNoCase(_T("height")) == 0 )
			m_Height = SvgLength(xml[n]);
		else if ( xml[n].m_nIndex.CompareNoCase(_T("href")) == 0 )
			continue;
		else if ( xml[n].m_nIndex.CompareNoCase(_T("xlink:href")) == 0 )
			continue;
		else
			tmp[xml[n].m_nIndex] = xml[n];
	}

	// 保存した元サイズを現在のスケールで再調整
	m_FontWidth  = fw * m_Width  / TEK_WIN_WIDTH;
	m_FontHeight = fh * m_Height / TEK_WIN_HEIGHT;

	tmp.m_pOwner = xml.m_pOwner;
	pOld = pXml->m_pOwner;
	pXml->m_pOwner = &tmp;
	pXml->m_Value++;

	if ( pXml->m_nIndex.CompareNoCase(_T("symbol")) == 0 )
		SvgRoot(xml, FALSE);
	else
		SvgCmds(*pXml);

	pXml->m_Value--;
	pXml->m_pOwner = pOld;

	m_OfsX = save_OfsX;
	m_OfsY = save_OfsY;
	m_Width  = save_Width;
	m_Height = save_Height;
	m_FontWidth  = save_FontWidth;
	m_FontHeight = save_FontHeight;
}
void CTekWnd::SvgGroup(CStringIndex &xml)
{
	// <g id="xxx" >

	for ( int n = 0 ; n < xml.GetSize() ; n++ ) {
		if ( xml[n].m_bChild )
			SvgCmds(xml[n]);
	}
}
void CTekWnd::SvgRoot(CStringIndex &xml, BOOL bInit)
{
	// <svg x="0px" y="0px" width="841.89px" height="1190.55px" viewBox="0 0 100 100" >

	int n;
	double save_OfsX = m_OfsX;
	double save_OfsY = m_OfsY;
	double save_Width  = m_Width;
	double save_Height = m_Height;
	double save_FontWidth = m_FontWidth;
	double save_FontHeight = m_FontHeight;
	CStringIndex save_Defs = m_Defs;

	if ( bInit ) {
		xml.OwnerLink(NULL);

		if ( (n = xml.Find(_T("width"))) >= 0 )
			m_Width  = SvgLength(xml[n]);
		if ( (n = xml.Find(_T("height"))) >= 0 )
			m_Height = SvgLength(xml[n]);
	}

	if ( (n = xml.Find(_T("viewBox"))) >= 0 ) {
		CArray<double, double> param;
		SvgParam(xml[n], param);

		if ( param.GetSize() > 0 )
			m_OfsX = param[0];
		if ( param.GetSize() > 1 )
			m_OfsY = param[1];
		if ( param.GetSize() > 2 )
			m_Width = param[2];
		if ( param.GetSize() > 3 )
			m_Height = param[3];
	}

	if ( bInit ) {
		if ( (m_Width * TEK_WIN_HEIGHT / TEK_WIN_WIDTH) < m_Height ) {
			double d = m_Height * TEK_WIN_WIDTH / TEK_WIN_HEIGHT;
			m_OfsX = m_OfsX + (m_Width - d) / 2;
			m_Width = d;
		} else {
			double d = m_Width * TEK_WIN_HEIGHT / TEK_WIN_WIDTH;
			m_OfsY = m_OfsY + (m_Height - d) / 2;
			m_Height = d;
		}
	}
	
	m_FontWidth  = TekFonts[0][0] * m_Width  / TEK_WIN_WIDTH;
	m_FontHeight = TekFonts[0][1] * m_Height / TEK_WIN_HEIGHT;

	SvgGroup(xml);

	m_OfsX = save_OfsX;
	m_OfsY = save_OfsY;
	m_Width  = save_Width;
	m_Height = save_Height;
	m_FontWidth  = save_FontWidth;
	m_FontHeight = save_FontHeight;
	m_Defs = save_Defs;
}
BOOL CTekWnd::LoadSvgText(LPCTSTR text)
{
	CStringIndex xml(TRUE, TRUE);

	if ( !xml.GetXmlFormat(text) ) {
		::AfxMessageBox((LPCTSTR)xml);
		return FALSE;
	}

	m_OfsX = m_OfsY = 0.0;
	m_Width  = TEK_WIN_WIDTH / 4.0;
	m_Height = TEK_WIN_HEIGHT / 4.0;

	m_Defs.SetNoCase(TRUE);
	m_Style.SetNoCase(TRUE);
	m_Style.RemoveAll();

	for ( int n = 0 ; n < xml.GetSize() ; n++ ) {
		if ( xml[n].m_nIndex.CompareNoCase(_T("svg")) == 0 )
			SvgRoot(xml[n], TRUE);
		else if ( xml.m_nIndex.CompareNoCase(_T("style")) == 0 )
			SvgStyle(xml);
	}

	Invalidate(1);

	return TRUE;
}
BOOL CTekWnd::LoadSvg(LPCTSTR filename)
{
	CBuffer text;

	if ( !text.LoadFile(filename) )
		return FALSE;

	text.KanjiConvert(text.KanjiCheck());

	return LoadSvgText((LPCTSTR)text);
}
