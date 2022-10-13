// GrapWnd.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "GrapWnd.h"

// CFrameWndExt

IMPLEMENT_DYNAMIC(CFrameWndExt, CFrameWnd)

CFrameWndExt::CFrameWndExt()
{
}
CFrameWndExt::~CFrameWndExt()
{
}

BEGIN_MESSAGE_MAP(CFrameWndExt, CFrameWnd)
	ON_WM_CREATE()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

int CFrameWndExt::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if ( ExEnableNonClientDpiScaling != NULL )
		ExEnableNonClientDpiScaling(GetSafeHwnd());

	return 0;
}

LRESULT CFrameWndExt::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	MoveWindow((RECT *)lParam, TRUE);

	return TRUE;
}

// CGrapWnd

IMPLEMENT_DYNAMIC(CGrapWnd, CFrameWndExt)

CGrapWnd::CGrapWnd(class CTextRam *pTextRam)
{
	m_pTextRam = pTextRam;
	m_MaxX = 1024;
	m_MaxY = 1024;
	m_AspX = ASP_DIV;
	m_AspY = ASP_DIV;
	m_Maps = 0;
	m_pActMap = NULL;
	m_Type = 0;
	m_ImageIndex = (-1);
	m_Crc = 0;
	m_Use = 0;
	m_Alive = 0;
	m_pList[0] = m_pList[1] = m_pList[2] = NULL;
	m_TransIndex = (-1);
	m_bHaveAlpha = FALSE;
	m_SixelBackColor = m_SixelTransColor = (-1);
	m_pHistogram = NULL;
	m_ColMap = NULL;
	m_ColAlpha = NULL;
}
CGrapWnd::~CGrapWnd()
{
	if ( m_pHistogram != NULL )
		m_pHistogram->DestroyWindow();

	if ( m_ColMap != NULL && m_ColMap != m_pTextRam->m_pSixelColor )
		delete [] m_ColMap;

	if ( m_ColAlpha != NULL && m_ColAlpha != m_pTextRam->m_pSixelAlpha )
		delete [] m_ColAlpha;
}

BEGIN_MESSAGE_MAP(CGrapWnd, CFrameWndExt)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_COMMAND(IDM_TEK_CLOSE, &CGrapWnd::OnGrapClose)
	ON_COMMAND(IDM_TEK_SAVE, &CGrapWnd::OnGrapSave)
	ON_COMMAND(IDM_TEK_CLEAR, &CGrapWnd::OnGrapClear)
	ON_UPDATE_COMMAND_UI(IDM_TEK_SAVE, &CGrapWnd::OnUpdateGrapSave)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_EDIT_COPY, &CGrapWnd::OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CGrapWnd::OnUpdateEditCopy)
	ON_COMMAND(IDM_HISTOGRAM, &CGrapWnd::OnHistogram)
	ON_UPDATE_COMMAND_UI(IDM_HISTOGRAM, &CGrapWnd::OnUpdateHistogram)
END_MESSAGE_MAP()

// CGrapWnd メッセージ ハンドラー

BOOL CGrapWnd::PreCreateWindow(CREATESTRUCT& cs)
{
//	cs.hMenu = ::LoadMenu(cs.hInstance, MAKEINTRESOURCE(IDR_GRAPWND));
	((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_GRAPWND), cs.hMenu);

	cs.cx = AfxGetApp()->GetProfileInt(_T("GrapWnd"), _T("cx"), 640);
	cs.cy = AfxGetApp()->GetProfileInt(_T("GrapWnd"), _T("cy"), 480 + 60);

//	cs.hwndParent = ::AfxGetMainWnd()->m_hWnd;
	cs.hwndParent = CWnd::GetDesktopWindow()->m_hWnd;

	return CFrameWndExt::PreCreateWindow(cs);
}
void CGrapWnd::PostNcDestroy()
{
	if ( m_pTextRam->m_pImageWnd == this )
		m_pTextRam->m_pImageWnd = NULL;
	m_pTextRam->RemoveGrapWnd(this);
	delete this;
}
BOOL CGrapWnd::PreTranslateMessage(MSG* pMsg)
{
	CWnd *pWnd, *pMainWnd;
	if ( pMsg->message >= WM_KEYDOWN && pMsg->message <= WM_DEADCHAR ) {
		if ( (pMainWnd = AfxGetMainWnd()) != NULL ) {
			pMainWnd->SetFocus();
			if ( (pWnd = m_pTextRam->GetAciveView()) != NULL ) {
				((CMainFrame *)pMainWnd)->MDIActivate(pWnd->GetParentFrame());
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

void CGrapWnd::OnDestroy()
{
	if ( !IsIconic() && !IsZoomed() ) {
		CRect rect;
		GetWindowRect(&rect);
		AfxGetApp()->WriteProfileInt(_T("GrapWnd"), _T("x"),  rect.left);
		AfxGetApp()->WriteProfileInt(_T("GrapWnd"), _T("y"),  rect.top);
		AfxGetApp()->WriteProfileInt(_T("GrapWnd"), _T("cx"), rect.Width());
		AfxGetApp()->WriteProfileInt(_T("GrapWnd"), _T("cy"), rect.Height());
	}

	CFrameWndExt::OnDestroy();
}
void CGrapWnd::OnPaint()
{
	CPaintDC dc(this);
	CDC TempDC;
	CBitmap *pOldBitMap;
	CRect rect, box;
	CPoint po;
	int n, sx, sy, cx, cy;

	GetClientRect(rect);

	if ( m_pActMap != NULL ) {
		TempDC.CreateCompatibleDC(&dc);
		pOldBitMap = (CBitmap *)TempDC.SelectObject(m_pActMap);
		po = dc.GetBrushOrg();
		dc.SetStretchBltMode(HALFTONE);

		cx = m_MaxX * m_AspX / ASP_DIV;
		cy = m_MaxY * m_AspY / ASP_DIV;

		if ( m_pTextRam != NULL && m_pTextRam->IsOptEnable(TO_RLGRPIND) ) {
			if ( (cx * rect.Height() / rect.Width()) > cy ) {
				n = rect.Width() * cy / cx;
				box.left = 0;
				box.right = rect.right;
				box.top = (rect.Height() - n) / 2;
				box.bottom = box.top + n;
			} else {
				n = rect.Height() * cx / cy;
				box.left = (rect.Width() - n) / 2;
				box.right = box.left + n;
				box.top = 0;
				box.bottom = rect.bottom;
			}

			dc.FillSolidRect(rect, RGB(128, 128, 128));
			dc.StretchBlt(box.left, box.top, box.Width(), box.Height(), &TempDC, 0, 0, m_MaxX, m_MaxY, SRCCOPY);

		} else {
			if ( (rect.Width() * 100 / rect.Height()) > (cx * 100 / cy) ) {
				n = cx * rect.Height() / rect.Width();
				sx = 0;
				sy = ((cy - n) / 2) * ASP_DIV / m_AspY;
				cx = cx * ASP_DIV / m_AspX;
				cy = n  * ASP_DIV / m_AspY;

			} else {
				n = cy * rect.Width() / rect.Height();
				sx = ((cx - n) / 2) * ASP_DIV / m_AspX;
				sy = 0;
				cx = n  * ASP_DIV / m_AspX;
				cy = cy * ASP_DIV / m_AspY;
			}

			dc.StretchBlt(0, 0, rect.Width(), rect.Height(), &TempDC, sx, sy, cx, cy, SRCCOPY);
		}


		TempDC.SelectObject(pOldBitMap);
		dc.SetBrushOrg(po);

	} else
		dc.FillSolidRect(rect, RGB(128, 128, 128));
}
void CGrapWnd::DrawBlock(CDC *pDC, LPCRECT pRect, COLORREF bc, BOOL bEraBack, int sx, int sy, int ex, int ey, int scW, int scH, int chW, int chH, int cols, int lines)
{
	int mx, my, ox, oy;
	int vx, vy, dx, dy;
	int smd;
	CDC TempDC;
	CBitmap *pOldMap;
	CRect box, rect(*pRect);
	BLENDFUNCTION bf;

	if ( m_pActMap == NULL || sx >= ex || sy >= ey )
		return;

	dx = scW * m_BlockX / cols;
	dy = scH * m_BlockY / lines;

	mx = dx * ASP_DIV * m_CellX / m_AspX / chW;
	my = dy * ASP_DIV * m_CellY / m_AspY / chH;

	ox = (mx - m_MaxX) / 2;
	oy = (my - m_MaxY) / 2;

	box.left   = mx * sx / m_BlockX - ox;
	box.top    = my * sy / m_BlockY - oy;
	box.right  = mx * ex / m_BlockX - ox;
	box.bottom = my * ey / m_BlockY - oy;

	if ( box.Width() <= 0 )
		box.right = box.left + 1;

	if ( box.Height() <= 0 )
		box.bottom = box.top + 1;

	dx = rect.Width();
	dy = rect.Height();
	vx = box.Width();
	vy = box.Height();

	if ( box.left < 0 ) {
		rect.left -= (box.left * dx / vx);
		box.left = 0;
	}

	if ( box.right > m_MaxX ) {
		rect.right -= ((box.right - m_MaxX) * dx / vx);
		box.right = m_MaxX;
	}

	if ( box.top < 0 ) {
		rect.top -= (box.top * dy / vy);
		box.top = 0;
	}

	if ( box.bottom > m_MaxY ) {
		rect.bottom -= ((box.bottom - m_MaxY) * dy / vy);
		box.bottom = m_MaxY;
	}

	TempDC.CreateCompatibleDC(pDC);

	if ( IsGifAnime() ) {
		if ( m_GifAnimeClock < clock() ) {
			if ( ++m_GifAnimePos >= m_GifAnime.GetSize() )
				m_GifAnimePos = 0;
			m_GifAnimeClock = clock() + m_GifAnime[m_GifAnimePos].m_Delay;
		}
		pOldMap = (CBitmap *)TempDC.SelectObject((CBitmap *)&(m_GifAnime[m_GifAnimePos]));
		m_TransIndex = m_GifAnime[m_GifAnimePos].m_TransIndex;
		m_bHaveAlpha = m_GifAnime[m_GifAnimePos].m_bHaveAlpha;
	} else
		pOldMap = (CBitmap *)TempDC.SelectObject(m_pActMap);

	smd = pDC->SetStretchBltMode(HALFTONE);

	if ( m_bHaveAlpha ) {
		ZeroMemory(&bf, sizeof(bf));
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 0xff;
		bf.AlphaFormat = AC_SRC_ALPHA;
	}

	if ( m_TransIndex != (-1) ) {
		RGBQUAD coltab[1];
		if ( !bEraBack && (bc == RGB(0, 0, 0) || bc == RGB(255, 255, 255)) )
			bc = RGB(1, 1, 1);
		coltab[0].rgbRed   = GetRValue(bc);
		coltab[0].rgbGreen = GetGValue(bc);
		coltab[0].rgbBlue  = GetBValue(bc);
		coltab[0].rgbReserved = 0;
		SetDIBColorTable(TempDC.GetSafeHdc(), m_TransIndex, 1, coltab);
	}

	if ( bEraBack ) {
		if ( rect.left > pRect->left )
			pDC->FillSolidRect(pRect->left, pRect->top, rect.left - pRect->left, pRect->bottom - pRect->top, bc);

		if ( rect.right < pRect->right )
			pDC->FillSolidRect(rect.right, pRect->top, pRect->right - rect.right, pRect->bottom - pRect->top, bc);

		if ( rect.top > pRect->top )
			pDC->FillSolidRect(pRect->left, pRect->top, pRect->right - pRect->left, rect.top - pRect->top, bc);

		if ( rect.bottom < pRect->bottom )
			pDC->FillSolidRect(pRect->left, rect.bottom, pRect->right - pRect->left, pRect->bottom - rect.bottom, bc);

		if ( m_bHaveAlpha ) {
			pDC->FillSolidRect(rect, bc);
			pDC->AlphaBlend(rect.left, rect.top, rect.Width(), rect.Height(), &TempDC, box.left, box.top, box.Width(), box.Height(), bf);

		} else {
			if ( m_SixelTransColor != (-1) ) {
				int n = m_Maps ^ 1;

				if ( bc != m_SixelBackColor || m_Bitmap[n].GetSafeHandle() == NULL ) {
					CDC SecDC;
					CBitmap *pOldSecMap;

					if ( m_Bitmap[n].GetSafeHandle() == NULL )
						m_Bitmap[n].CreateCompatibleBitmap(pDC, m_MaxX, m_MaxY);

					SecDC.CreateCompatibleDC(pDC);
					pOldSecMap = (CBitmap *)SecDC.SelectObject(&(m_Bitmap[n]));
					SecDC.FillSolidRect(0, 0, m_MaxX, m_MaxX, bc);
					SecDC.TransparentBlt(0, 0, m_MaxX, m_MaxY, &TempDC, 0, 0, m_MaxX, m_MaxY, m_SixelTransColor);
					SecDC.SelectObject(pOldSecMap);
					SecDC.DeleteDC();

					m_SixelBackColor = bc;
				}

				TempDC.SelectObject(&(m_Bitmap[n]));
			}

			pDC->StretchBlt(rect.left, rect.top, rect.Width(), rect.Height(), &TempDC, box.left, box.top, box.Width(), box.Height(), SRCCOPY);
		}

	} else if ( m_bHaveAlpha ) {
		pDC->AlphaBlend(rect.left, rect.top, rect.Width(), rect.Height(), &TempDC, box.left, box.top, box.Width(), box.Height(), bf);

	} else {
		if ( m_SixelTransColor != (-1) )
			bc = m_SixelTransColor;
		pDC->TransparentBlt(rect.left, rect.top, rect.Width(), rect.Height(), &TempDC, box.left, box.top, box.Width(), box.Height(), bc);
	}

	TempDC.SelectObject(pOldMap);
	pDC->SetStretchBltMode(smd);
}

BOOL CGrapWnd::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
//	return CFrameWndExt::OnEraseBkgnd(pDC);
}

void CGrapWnd::OnGrapClose()
{
	DestroyWindow();
}
void CGrapWnd::SaveBitmap(int type)
{
	CDC TempDC, SaveDC;
	CBitmap *pOldBitMap;
	CImage image;
	static LPCTSTR extname[] = { _T("gif"), _T("jpg"), _T("png"), _T("bmp") };
	static LPCTSTR defname[] = { _T("*.gif"), _T("*.jpg"), _T("*.png"), _T("*.bmp") };
	CFileDialog dlg(FALSE, extname[type], defname[type], OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGGRAPHICS) , this);

	if ( m_pActMap == NULL || DpiAwareDoModal(dlg) != IDOK )
		return;

	if ( !image.Create(m_MaxX, m_MaxY, 24) )
		return;

	TempDC.CreateCompatibleDC(NULL);
	pOldBitMap = (CBitmap *)TempDC.SelectObject(m_pActMap);

	SaveDC.Attach(image.GetDC());
	SaveDC.BitBlt(0, 0, m_MaxX, m_MaxY, &TempDC, 0, 0, SRCCOPY);
	SaveDC.Detach();

	image.ReleaseDC();
	image.Save(dlg.GetPathName());
	TempDC.SelectObject(pOldBitMap);
}
BOOL CGrapWnd::SaveImage(HANDLE hBitmap, const GUID &guid, CBuffer &buf)
{
	CImage image;
	IStream *pStm;
	ULONG len;
	BYTE tmp[1024];

	try {
		if ( hBitmap == NULL )
			return FALSE;

		image.Attach((HBITMAP)hBitmap);

		if ( (pStm = SHCreateMemStream(NULL, 0)) == NULL )
			throw;

		if ( FAILED(image.Save(pStm, guid)) )
			throw;

		LARGE_INTEGER lz = { 0, 0 };
		pStm->Seek(lz, STREAM_SEEK_SET, NULL);

		while ( SUCCEEDED(pStm->Read(tmp, 1024, &len)) && len > 0 )
			buf.Apend(tmp, len);

		pStm->Release();
		image.Detach();

		return TRUE;

	} catch(...) {
		image.Detach();
		return FALSE;
	}
}
HANDLE CGrapWnd::GetBitmapHandle()
{
	HANDLE hBitmap;
	CDC TempDC, SaveDC;
	CBitmap TempMap;
	CBitmap *pOldBitMap[2];

	if ( m_pActMap == NULL )
		return NULL;

	TempDC.CreateCompatibleDC(NULL);
	pOldBitMap[0] = (CBitmap *)TempDC.SelectObject(m_pActMap);

	SaveDC.CreateCompatibleDC(NULL);
	TempMap.CreateBitmap(m_MaxX, m_MaxY, SaveDC.GetDeviceCaps(PLANES), SaveDC.GetDeviceCaps(BITSPIXEL), NULL);
	pOldBitMap[1] = (CBitmap *)SaveDC.SelectObject(&TempMap);

	SaveDC.BitBlt(0, 0, m_MaxX, m_MaxY, &TempDC, 0, 0, SRCCOPY);

	SaveDC.SelectObject(pOldBitMap[1]);
	TempDC.SelectObject(pOldBitMap[0]);

	hBitmap = (HANDLE)TempMap.GetSafeHandle();
	TempMap.Detach();
	return hBitmap;
}
HANDLE CGrapWnd::GetBitmapBlock(COLORREF bc, int sx, int sy, int ex, int ey, int chW, int chH, int cols, int lines, BITMAP *pMap)
{
	CRect rect;
	HANDLE hBitmap;
	CDC TempDC;
	CBitmap TempMap;
	CBitmap *pOldBitMap;

	if ( m_pActMap == NULL )
		return NULL;

	rect.left = 0; rect.right = (ex - sx) * chW;
	rect.top = 0; rect.bottom = (ey - sy) * chH;

	TempDC.CreateCompatibleDC(NULL);
	TempMap.CreateBitmap(rect.Width(), rect.Height(), TempDC.GetDeviceCaps(PLANES), TempDC.GetDeviceCaps(BITSPIXEL), NULL);
	pOldBitMap = (CBitmap *)TempDC.SelectObject(&TempMap);
	DrawBlock(&TempDC, rect, bc, TRUE, sx, sy, ex, ey, chW * cols, chH * lines, chW, chH, cols, lines);
	TempDC.SelectObject(pOldBitMap);

	if ( pMap != NULL )
		TempMap.GetBitmap(pMap);

	hBitmap = (HANDLE)TempMap.GetSafeHandle();
	TempMap.Detach();
	return hBitmap;
}
void CGrapWnd::OnGrapSave()
{
	SaveBitmap(0);
}
void CGrapWnd::OnUpdateGrapSave(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pActMap != NULL ? TRUE : FALSE);
}
void CGrapWnd::OnGrapClear()
{
	m_pActMap = NULL;
	Invalidate(FALSE);
}
void CGrapWnd::OnEditCopy()
{
	HANDLE hBitmap;

	if ( (hBitmap = GetBitmapHandle()) == NULL )
		return;

	if ( OpenClipboard() ) {
		if ( EmptyClipboard() )
			SetClipboardData(CF_BITMAP, hBitmap);
		else
			DeleteObject(hBitmap);

		CloseClipboard();

	} else
		DeleteObject(hBitmap);
}
void CGrapWnd::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pActMap != NULL ? TRUE : FALSE);
}

void CGrapWnd::OnHistogram()
{
	if ( m_pActMap == NULL || m_pActMap->m_hObject == NULL || m_pHistogram != NULL )
		return;

	m_pHistogram = new CHistogram;
	m_pHistogram->m_pGrapWnd = this;
	m_pHistogram->Create(NULL, _T("Histogram"));
	m_pHistogram->SetBitmap((HBITMAP)m_pActMap->m_hObject);
	m_pHistogram->ShowWindow(SW_SHOW);
}
void CGrapWnd::OnUpdateHistogram(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pActMap != NULL && m_pHistogram == NULL ? TRUE : FALSE);
}

//////////////////////////////////////////////////////////////////////
// Static libs
	
void CGrapWnd::RGBtoHLS(COLORREF rgb, WORD hls[3])
{
	int R, G, B;				/* input RGB values */
	int H, L, S;				/* output HLS values */
	int cMax, cMin;				/* max and min RGB values */
	int Rdelta, Gdelta, Bdelta; /* intermediate value: % of spread from max*/

	/* get R, G, and B out of DWORD */
	R = GetRValue(rgb);
	G = GetGValue(rgb);
	B = GetBValue(rgb);

	/* calculate lightness */
	cMax = max(max(R, G), B);
	cMin = min(min(R, G), B);
	L = (((cMax + cMin) * HLSMAX) + RGBMAX) / (2 * RGBMAX);

	if ( cMax == cMin ) {
		/* r=g=b --> achromatic case */
		S = 0;						/* saturation */
		H = (HLSMAX * 2 / 3);		/* hue 240 */
		goto RETOF;
	}

	/* chromatic case */

	/* saturation */
	if ( L <= (HLSMAX / 2) )
		S = (((cMax - cMin) * HLSMAX) + ((cMax + cMin) / 2)) / (cMax + cMin);
	else
		S = (((cMax - cMin) * HLSMAX) + ((2 * RGBMAX - cMax - cMin) / 2)) / (2 * RGBMAX - cMax - cMin);

	/* hue */
	Rdelta = (((cMax - R) * (HLSMAX / 6)) + ((cMax - cMin) / 2) ) / (cMax - cMin);
	Gdelta = (((cMax - G) * (HLSMAX / 6)) + ((cMax - cMin) / 2) ) / (cMax - cMin);
	Bdelta = (((cMax - B) * (HLSMAX / 6)) + ((cMax - cMin) / 2) ) / (cMax - cMin);

	if ( R == cMax )
		H = Bdelta - Gdelta;
	else if ( G == cMax )
		H = (HLSMAX / 3) + Rdelta - Bdelta;
	else /* B == cMax */
		H = ((2 * HLSMAX) / 3) + Gdelta - Rdelta;

RETOF:
	if ( H < 0 )
		H += HLSMAX;
	if ( H > HLSMAX )
		H -= HLSMAX;

	hls[0] = (WORD)H;
	hls[1] = (WORD)L;
	hls[2] = (WORD)S;
}
COLORREF CGrapWnd::HLStoRGB(int hue, int lum, int sat)
{
	int n, c;
	int Magic1, Magic2;
	BYTE rgb[3];

	//if ( hue < 0 )
	//	hue = 0;
	//else if ( hue > HLSMAX )
	//	hue = HLSMAX;

	//if ( lum < 0 )
	//	lum = 0;
	//else if ( lum > HLSMAX )
	//	lum = HLSMAX;

	//if ( sat < 0 )
	//	sat = 0;
	//else if ( sat > HLSMAX )
	//	sat = HLSMAX;

	if ( sat == 0 ) {
		rgb[0] = rgb[1] = rgb[2] = (BYTE)(lum * RGBMAX / HLSMAX);

	} else {
        if ( lum <= (HLSMAX / 2) )
	        Magic2 = (lum * (HLSMAX + sat) + (HLSMAX / 2)) / HLSMAX;
        else
		    Magic2 = lum + sat - ((lum * sat) + (HLSMAX / 2)) / HLSMAX;

		Magic1 = 2 * lum - Magic2;

		for ( n = 0 ; n < 3 ; n++ ) {
			if ( hue < (HLSMAX / 6) )			// 360 / 6 = 60
				c = (Magic1 + (((Magic2 - Magic1) * hue + (HLSMAX / 12)) / (HLSMAX / 6)));
			else if ( hue < (HLSMAX / 2) )		// 360 / 2 = 120
				c = Magic2;
			else if ( hue < (HLSMAX * 2 / 3) )	// 360 * 2 / 3 = 240
				c = (Magic1 + (((Magic2 - Magic1) * ((HLSMAX * 2 / 3) - hue) + (HLSMAX / 12)) / (HLSMAX / 6)));
			else
				c = Magic1;

			rgb[n] = (BYTE)(c * RGBMAX / HLSMAX);

			if ( (hue -= (HLSMAX / 3)) < 0 )	// 360 / 3 = 120
				hue += HLSMAX;
		}
	}

//  hue 0=blue, 120=red, 240=green
	return RGB(rgb[0], rgb[1], rgb[2]);
}
CBitmap *CGrapWnd::GetBitmap(int width, int height)
{
	CDC TempDC;
	CClientDC DispDC(this);
	CBitmap *pOldBitMap;

	m_AspX = ASP_DIV;
	m_AspY = ASP_DIV;
	m_MaxX = width;
	m_MaxY = height;

	m_Type = TYPE_VRAM;
	m_pActMap = NULL;
	m_Maps = 0;

	if ( m_Bitmap[m_Maps].GetSafeHandle() != NULL )
		m_Bitmap[m_Maps].DeleteObject();

	m_Bitmap[m_Maps].CreateCompatibleBitmap(&DispDC, m_MaxX, m_MaxY);

	TempDC.CreateCompatibleDC(&DispDC);
	pOldBitMap = (CBitmap *)TempDC.SelectObject(&(m_Bitmap[m_Maps]));
	TempDC.FillSolidRect(0, 0, m_MaxX, m_MaxY, RGB(0, 0, 0));

	TempDC.SelectObject(pOldBitMap);
	m_pActMap = &(m_Bitmap[m_Maps]);

	return m_pActMap;
}
int CGrapWnd::Compare(CGrapWnd *pWnd)
{
	BITMAP src, dis;

	if ( m_Type != pWnd->m_Type || m_Crc != pWnd->m_Crc )
		return 1;

	if ( m_pActMap == NULL || pWnd->m_pActMap == NULL )
		return (-1);

	m_pActMap->GetBitmap(&src);
	pWnd->m_pActMap->GetBitmap(&dis);

	if ( src.bmWidth != dis.bmWidth || src.bmHeight != dis.bmHeight )
		return 1;

	if ( src.bmType != dis.bmType || src.bmPlanes != dis.bmPlanes || src.bmBitsPixel != dis.bmBitsPixel || src.bmWidthBytes != dis.bmWidthBytes )
		return (-1);

#if 1
	return 0;
#else
	int n, c;
	BYTE *map;

	n = src.bmWidthBytes * src.bmHeight;
	map = new BYTE[n * 2];

	m_pActMap->GetBitmapBits(n, map);
	pWnd->m_pActMap->GetBitmapBits(n, map + n);

	c = memcmp(map, map + n, n);
	delete [] map;

	return c;
#endif
}
void CGrapWnd::Copy(CGrapWnd *pWnd)
{
	HANDLE hHandle;

	m_pTextRam = pWnd->m_pTextRam;

	m_Type = pWnd->m_Type;
	m_MaxX = pWnd->m_MaxX;
	m_MaxY = pWnd->m_MaxY;
	m_AspX = pWnd->m_AspX;
	m_AspY = pWnd->m_AspY;

	m_ImageIndex = (-1);
	m_Crc = pWnd->m_Crc;

	m_Maps = 0;
	m_pActMap = NULL;

	m_TransIndex = pWnd->m_TransIndex;
	m_bHaveAlpha = pWnd->m_bHaveAlpha;
	m_SixelTransColor = pWnd->m_SixelTransColor;

	if ( pWnd->m_pActMap == NULL )
		return;

	if ( m_Bitmap[m_Maps].GetSafeHandle() != NULL )
		m_Bitmap[m_Maps].DeleteObject();

	if ( (hHandle = CopyImage(pWnd->m_pActMap->GetSafeHandle(), IMAGE_BITMAP, m_MaxX, m_MaxY, LR_CREATEDIBSECTION)) != NULL ) {
		m_Bitmap[m_Maps].Attach(hHandle);
		m_pActMap = &(m_Bitmap[m_Maps]);
	}
}
void CGrapWnd::SetMapCrc()
{
	int n, len;
	BYTE *map, *s;
	BITMAP bitmap;
	extern const unsigned long crc32tab[];

	m_Crc = 0;

	if ( m_pActMap == NULL || !m_pActMap->GetBitmap(&bitmap) )
		return;

	len = bitmap.bmWidthBytes * bitmap.bmHeight * bitmap.bmPlanes;
	map = s = new BYTE[len];
	len = m_pActMap->GetBitmapBits(len, map);

	for ( n = 0 ; n < len ; n++ )
		m_Crc = crc32tab[(m_Crc ^ *(s++)) & 0xff] ^ ((m_Crc >> 8) & 0x00ffffff);

	delete [] map;
}

//////////////////////////////////////////////////////////////////////
// ReGIS

#define	LRGB(n)		RGB(n * 255 / 100, n * 255 / 100, n * 255 / 100)
#define	XRGB(r,g,b)	RGB(r * 255 / 100, g * 255 / 100, b * 255 / 100)
#define	HRGB(f,c)	RGB((GetRValue(f) + GetRValue(c)) / 2, (GetGValue(f) + GetGValue(c)) / 2, (GetBValue(c) + GetBValue(c)) / 2)

static const CGrapWnd::ReGISPROC	BasicCmd[] = {
		//				c<num>					c[x,y]					c(...)					c"..."					c
		{	'S',	{	&CGrapWnd::ParseVector,	&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'W',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'P',	{	&CGrapWnd::ParseVector,	&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'V',	{	&CGrapWnd::ParseVector,	&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'C',	{	NULL,					&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'T',	{	&CGrapWnd::ParseVector,	&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	&CGrapWnd::ParseString,	NULL					}	},
		{	'L',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	&CGrapWnd::ParseString,	NULL					}	},
		{	'R',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'F',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
// S Screen Provides screen controls, such as erasing the screen. 
static const CGrapWnd::ReGISPROC	SCmd[] = {
		{	'A',	{	NULL,					&CGrapWnd::ParseBacket,	NULL,					NULL,					NULL					}	},
		{	'H',	{	NULL,					&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					&CGrapWnd::ParseSingel	}	},
		{	'M',	{	&CGrapWnd::ParseNumber,	NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'I',	{	&CGrapWnd::ParseNumber,	NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'T',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'E',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'W',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'C',	{	&CGrapWnd::ParseNumber,	NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'P',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
static const CGrapWnd::ReGISPROC	SHCmd[] = {
		{	'P',	{	NULL,					&CGrapWnd::ParseBacket,	NULL,					NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
static const CGrapWnd::ReGISPROC	SWCmd[] = {
		{	'M',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
static const CGrapWnd::ReGISPROC	SCCmd[] = {
		{	'H',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'I',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	W Write Provides writing controls, such as writing shades. 
static const CGrapWnd::ReGISPROC	WCmd[] = {
		{	'M',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'P',	{	&CGrapWnd::ParseVector,	NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'I',	{	&CGrapWnd::ParseNumber,	NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'F',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'V',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'R',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'C',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'E',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'N',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'S',	{	&CGrapWnd::ParseNumber,	&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	&CGrapWnd::ParseString,	NULL					}	},
		{	'L',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	P Position Moves the graphics cursor without performing any writing. 
static const CGrapWnd::ReGISPROC	PCmd[] = {
		{	'W',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'B',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'S',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'E',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'P',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	V Vector Draws vectors (straight lines) between the screen locations you specify in the command. 
static const CGrapWnd::ReGISPROC	VCmd[] = {
		{	'B',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'S',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'E',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'W',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	C Curve Draws circles and arcs, using the screen locations you specify in the command. 
static const CGrapWnd::ReGISPROC	CCmd[] = {
		{	'A',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'C',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'B',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'S',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'E',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'W',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	T Text Controls the display of text strings in graphics, and lets you specify characters to display. 
static const CGrapWnd::ReGISPROC	TCmd[] = {
		{	'A',	{	&CGrapWnd::ParseNumber,	NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'S',	{	&CGrapWnd::ParseNumber,	&CGrapWnd::ParseBacket,	NULL,					NULL,					NULL					}	},
		{	'U',	{	NULL,					&CGrapWnd::ParseBacket,	NULL,					NULL,					NULL					}	},
		{	'H',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'D',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'I',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					NULL,					NULL					}	},
		{	'B',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'E',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'W',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'M',	{	NULL,					&CGrapWnd::ParseBacket,	NULL,					NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
static const CGrapWnd::ReGISPROC	TACmd[] = {
		{	'L',	{	NULL,					NULL,					NULL,					&CGrapWnd::ParseString,	NULL					}	},
		{	'R',	{	NULL,					NULL,					NULL,					&CGrapWnd::ParseString,	NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	L Load Defines and loads alternate characters you can display with the text command. 
static const CGrapWnd::ReGISPROC	LCmd[] = {
		{	'A',	{	&CGrapWnd::ParseNumber,	NULL,					NULL,					&CGrapWnd::ParseString,	NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	R Report Reports information such as the active position and error codes. 
static const CGrapWnd::ReGISPROC	RCmd[] = {
		{	'P',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'M',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'L',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'E',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'I',	{	NULL,					NULL,					NULL,					NULL,					&CGrapWnd::ParseSingel	}	},
		{	'P',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
//////////////////////////////////////////////////////////////////////
//	F Polygon Fill Fills in a single closed figure, such as a circle or square. 
static const CGrapWnd::ReGISPROC	FCmd[] = {
		{	'P',	{	&CGrapWnd::ParseVector,	&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'V',	{	&CGrapWnd::ParseVector,	&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'C',	{	NULL,					&CGrapWnd::ParseBacket,	&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	'W',	{	NULL,					NULL,					&CGrapWnd::ParseOption,	NULL,					NULL					}	},
		{	0,		{	NULL,					NULL,					NULL,					NULL,					NULL					}	},
	};
static const POINT FontTab[17][2] = {
		{	{   9,  10	},	{   8,  10	}	},	// S0
		{	{   9,  20	},	{   8,  20	}	},	// S1
		{	{  18,  30	},	{  16,  30	}	},	// S2
		{	{  27,  45	},	{  24,  45	}	},	// S3
		{	{  36,  60	},	{  32,  60	}	},	// S4
		{	{  45,  75	},	{  40,  75	}	},	// S5
		{	{  51,  90	},	{  48,  90	}	},	// S6
		{	{  53, 105	},	{  56, 105	}	},	// S7
		{	{  72, 120	},	{  64, 120	}	},	// S8
		{	{  81, 135	},	{  72, 135	}	},	// S9
		{	{  90, 150	},	{  80, 150	}	},	// S10
		{	{  99, 165	},	{  88, 165	}	},	// S11
		{	{ 108, 180	},	{  96, 180	}	},	// S12
		{	{ 117, 195	},	{ 104, 195	}	},	// S13
		{	{ 126, 210	},	{ 112, 210	}	},	// S14
		{	{ 133, 225	},	{ 120, 225	}	},	// S15
		{	{ 144, 240	},	{ 128, 240	}	},	// S16
	};
static const COLORREF DefColTab[2][16] = {
	{
		LRGB( 0),	// 0 Black
		LRGB(20),	// 1 Gray-2
		LRGB(40),	// 2 Gray-4
		LRGB(60),	// 3 Gray-6
		LRGB(10),	// 4 Gray-1
		LRGB(30),	// 5 Gray-3
		LRGB(50),	// 6 Gray-5
		LRGB(70),	// 7 White
		LRGB( 0),	// 8 Black
		LRGB(20),	// 9 Gray-2
		LRGB(40),	// 10 Gray-4
		LRGB(60),	// 11 Gray-6
		LRGB(10),	// 12 Gray-1
		LRGB(30),	// 13 Gray-3
		LRGB(50),	// 14 Gray-5
		LRGB(70),	// 15 White 7
	}, {
		XRGB( 0,  0,  0),	//  0 Black
		XRGB(20, 20, 80),	//  1 Blue
		XRGB(80, 13, 13),	//  2 Red
		XRGB(20, 80, 20),	//  3 Green
		XRGB(80, 20, 80),	//  4 Magenta
		XRGB(20, 80, 80),	//  5 Cyan
		XRGB(80, 80, 20),	//  6 Yellow
		XRGB(53, 53, 53), 	//  7 Gray 50%
		XRGB(26, 26, 26), 	//  8 Gray 25%
		XRGB(33, 33, 60), 	//  9 Blue*
		XRGB(60, 26, 26), 	// 10 Red*
		XRGB(33, 60, 33), 	// 11 Green*
		XRGB(60, 33, 60), 	// 12 Magenta*
		XRGB(33, 60, 60),	// 13 Cyan*
		XRGB(60, 60, 33),	// 14 Yellow*
		XRGB(80, 80, 80), 	// 15 Gray 75%
	}
};
static const BYTE DefPtnTab[10] = {
		0x00,				// 0 0000 0000 All-off write pattern 
		0xFF,				// 1 1111 1111 All-on write pattern 
		0xF0,				// 2 1111 0000 Dash pattern 
		0xE4,				// 3 1110 0100 Dash-dot pattern 
		0xAA,				// 4 1010 1010 Dot pattern 
		0xEA,				// 5 1110 1010 Dash-dot-dot pattern 
		0x88,				// 6 1000 1000 Sparse dot pattern 
		0x84,				// 7 1000 0100 Asymmetrical sparse dot pattern 
		0xC8,				// 8 1100 1000 Sparse dash-dot pattern
		0x86,				// 9 1000 0110 Sparse dot-dash pattern 
	};

void CGrapWnd::InitColMap()
{
	int n, i, a, b, c;

	if ( !m_pTextRam->IsOptEnable(TO_XTPRICOL) ) {
		if ( m_pTextRam->m_pSixelColor == NULL )
			m_pTextRam->m_pSixelColor = new COLORREF [SIXEL_PALET + 1];
		if ( m_pTextRam->m_pSixelAlpha == NULL )
			m_pTextRam->m_pSixelAlpha = new BYTE [SIXEL_PALET + 1];

		m_ColMap   = m_pTextRam->m_pSixelColor;
		m_ColAlpha = m_pTextRam->m_pSixelAlpha;

		if ( m_pTextRam->m_bSixelColInit )
			return;

		m_pTextRam->m_bSixelColInit = TRUE;

	} else {
		m_ColMap   = new COLORREF [SIXEL_PALET + 1];
		m_ColAlpha = new BYTE [SIXEL_PALET + 1];
	}

	i = (m_pTextRam->m_TermId < VT_COLMAP_ID ? 0 : 1);

	// ReGisのパレット16色を初期化
	for ( n = 0 ; n < 16 ; n++ ) {
		m_ColMap[n] = DefColTab[i][n];

		// 背景色と同じパレットは、補色に変更
		if ( m_ColMap[n] == m_BakCol )
			m_ColMap[n] ^= RGB(128, 128, 128);
	}

	// colors 16-231 are a 6x6x6 color cube
	for ( a = 0 ; a < 6 ; a++ ) {
		for ( b = 0 ; b < 6 ; b++ ) {
			for ( c = 0 ; c < 6 ; c++ )
				m_ColMap[n++] = RGB(a * 51, b * 51, c * 51);
		}
	}

	// colors 232-255 are a grayscale ramp, intentionally leaving out
	for ( a = 0 ; a < 24 ; a++ )
		m_ColMap[n++] = RGB(a * 11, a * 11, a * 11);

	// ext palet
	for ( ; n < SIXEL_PALET ; n++ )
		m_ColMap[n] = m_BakCol;

	memset(m_ColAlpha, 0xFF, sizeof(BYTE) * SIXEL_PALET + 1);
}
int CGrapWnd::GetChar(LPCSTR &p)
{
	int ch = (-1);

	while ( *p != '\0' && *p <= ' ' )
		p++;

	if ( *p >= 'a' && *p <= 'z' ) {
		ch = toupper(*p);
		p++;
	} else if ( *p != '\0' )
		ch = *(p++);

	return ch;
}
int CGrapWnd::GetDigit(LPCSTR &p, int val)
{
	int vn = 0;
	int sn = 0;
	BOOL bn = FALSE;

	while ( *p != '\0' && *p <= ' ' )
		p++;

	while ( *p != '\0' ) {
		if ( *p == '+' )
			sn = 1;
		else if ( *p == '-' )
			sn = (-1);
		else
			break;
		p++;
	}

	while ( isdigit(*p) ) {
		vn = vn * 10 + (*(p++) - '0');
		bn = TRUE;
	}

	return (sn == 0 ? (bn ? vn : val) : (val + vn * sn));
}
int CGrapWnd::GetBacket(LPCSTR &p, int &x, int &y)
{
	int ch;

	x = GetDigit(p, x);
	if ( (ch = GetChar(p)) == ',' ) {
		y = GetDigit(p, y);
		ch = GetChar(p);
	}
	return ch;
}
int CGrapWnd::GetColor(LPCSTR &p, COLORREF &rgb)
{
	int n, ch;
	int h = 0, l = 0, s = 0;

	while ( (ch = GetChar(p)) != (-1) ) {
		switch(ch) {
		case 'D':	// dark (black) 
			rgb = XRGB(0, 0, 0);
			break;
		case 'C':	// cyan (from blue and green) 
			rgb = XRGB(80, 13, 13);
			break;
		case 'Y':	// yellow (from red and green) 
			rgb = XRGB(80, 80, 20);
			break;
		case 'M':	// magenta (from red and blue) 
			rgb = XRGB(80, 20, 80);
			break;
		case 'W':	// white 
			rgb = XRGB(80, 80, 80);
			break;

		case 'R':	// red
			if ( isdigit(*p) ) {
				n = GetDigit(p, 0);
				if ( n < 0 ) n = 0; else if ( n > 100 ) n = 100;
				rgb = RGB(n * 255 / 100, GetGValue(rgb), GetBValue(rgb));
			} else
				rgb = XRGB(80, 13, 13);
			break;
		case 'G':	// green 
			if ( isdigit(*p) ) {
				n = GetDigit(p, 0);
				if ( n < 0 ) n = 0; else if ( n > 100 ) n = 100;
				rgb = RGB(GetRValue(rgb), n * 255 / 100, GetBValue(rgb));
			} else
				rgb = XRGB(20, 80, 20);
			break;
		case 'B':	// blue 
			if ( isdigit(*p) ) {
				n = GetDigit(p, 0);
				if ( n < 0 ) n = 0; else if ( n > 100 ) n = 100;
				RGB(GetRValue(rgb), GetGValue(rgb), n * 255 / 100);
			} else
				rgb = XRGB(20, 20, 80);
			break;

		case 'H':
			if ( isdigit(*p) ) {
				h = GetDigit(p, 0);
				if ( h < 0 ) h = 0; else if ( h > 360 ) h = 360;
			} else
				h = 0;
			rgb = HLStoRGB(h * HLSMAX / 360, l * HLSMAX / 100, s * HLSMAX / 100);
			break;
		case 'L':
			if ( isdigit(*p) ) {
				l = GetDigit(p, 0);
				if ( l < 0 ) l = 0; else if ( l > 100 ) l = 100;
			} else
				l = 0;
			rgb = HLStoRGB(h * HLSMAX / 360, l * HLSMAX / 100, s * HLSMAX / 100);
			break;
		case 'S':
			if ( isdigit(*p) ) {
				s = GetDigit(p, 0);
				if ( s < 0 ) s = 0; else if ( s > 100 ) s = 100;
			} else
				s = 0;
			rgb = HLStoRGB(h * HLSMAX / 360, l * HLSMAX / 100, s * HLSMAX / 100);
			break;

		case 'A':
			break;

		default:
			return ch;
		}
	}

	return ch;
}
int CGrapWnd::GetHexTab(LPCSTR &p, int max, BYTE *tab)
{
	int n;
	int ps = 0;

	while ( *p != '\0' ) {
		while ( *p != '\0' && *p <= ' ' )
			p++;

		if ( !isxdigit(*p) )
			break;

		for ( n = 0 ; *p != '\0' && isxdigit(*p) ; ) {
			if ( isdigit(*p) )
				n = n * 16 + (*(p++) - '0');
			else
				n = n * 16 + (toupper(*(p++)) - 'A' + 10);
		}
		if ( ps < max )
			tab[ps++] = (BYTE)n;

		if ( *p == ',' )
			p++;
	}
	return ps;
}

int CGrapWnd::ParseCmds(LPCSTR &p, int cmd, const CGrapWnd::ReGISPROC *pCmd, const CGrapWnd::ReGISPROC cmdTab[])
{
	int n, ch, ct = 0;

	while ( (ch = GetChar(p)) != (-1) ) {

		if ( ch == ')' ) {
			break;

		} else if ( isdigit(ch) || ch == '-' || ch == '+' ) {
			if ( pCmd == NULL || pCmd->proc[0] == NULL )
				return (-1);
			if ( (this->*pCmd->proc[0])(--p, pCmd->ch | cmd, ct = 0) == (-1) )
				return (-1);

		} else if ( ch == '[' ) {
			if ( pCmd == NULL || pCmd->proc[1] == NULL )
				return (-1);
			if ( (this->*pCmd->proc[1])(p, pCmd->ch | cmd, ct++) != ']' )
				return (-1);

		} else if ( ch == '(' ) {
			if ( pCmd == NULL || pCmd->proc[2] == NULL )
				return (-1);
			if ( (this->*pCmd->proc[2])(p, pCmd->ch | cmd, ct = 0) != ')' )
				return (-1);

		} else if ( ch == '"' || ch == '\'' ) {
			if ( pCmd == NULL || pCmd->proc[3] == NULL )
				return (-1);
			if ( (this->*pCmd->proc[3])(--p, pCmd->ch | cmd, ct = 0) != ch )
				return (-1);

		} else if ( ch == '@' ) {
			// @<call letter> Invoke macrograph Inserts the contents of the macrograph specified by <call letter> into the ReGIS command string. The <call letter> is not case sensitive.
			// @:<call letter> <definition>@; Define macrograph Defines a macrograph and selects <call letter> to identify the macrograph. The <call letter> is not case sensitive.
			// @. Clear all macrographs Deletes all macrograph definitions.
			// @:<call letter>@; Clear defined macrograph Deletes the macrograph identified by <call letter>.
			//	<call letter> is a letter of the alphabet used to identify the macrograph you are defining. The call letter is case insensitive. For example, 'a' and 'A' identify the same macrograph.
			//	<definition> is the macrograph's definition.
			if ( (ch = GetChar(p)) == EOF ) {
				return (-1);
			} else if ( ch == ':' ) {
				if ( (ch = GetChar(p)) == EOF )
					return (-1);
				if ( ch >= 'A' && ch <= 'Z' ) {
					n = ch - 'A';
					m_Macro[n].Empty();
					while ( *p !='\0' ) {
						if ( p[0] == '@' && p[1] == ';' ) {
							p += 2;
							break;
						} else
							m_Macro[n] += *(p++);
					}
				} else
					return (-1);
			} else if ( ch == '.' ) {
				for ( n = 0 ; n < 26 ; n++ )
					m_Macro[n].Empty();
			} else if ( ch >= 'A' && ch <= 'Z' ) {
				LPCSTR s = m_Macro[ch - 'A'];
				if ( *s != '\0' )
					ParseCmds(s, cmd, pCmd, cmdTab);
			} else
				return (-1);

		} else if ( ch == ',' || ch == ';' ) {
			ct = 0;
			continue;

		} else {
			for ( n = 0 ; cmdTab[n].ch != 0 ; n++ ) {
				if ( ch == cmdTab[n].ch )
					break;
			}
			if ( cmdTab[n].ch == 0 )
				break;
			pCmd = &(cmdTab[n]);
			ct = 0;

			if ( cmdTab == BasicCmd )
				ParseInit(pCmd->ch);

			if ( pCmd->proc[4] != NULL && (this->*pCmd->proc[4])(p, pCmd->ch | cmd, ct) == (-1) )
				return (-1);
		}
	}
	return ch;
}

void CGrapWnd::PushWparam()
{
	if ( m_LocPush )
		return;
	m_LocPush = TRUE;
	memcpy(&m_SaveWopt, &m_Wopt, sizeof(m_Wopt));
}
void CGrapWnd::PopWparam()
{
	if ( !m_LocPush )
		return;
	m_LocPush = FALSE;
	memcpy(&m_Wopt, &m_SaveWopt, sizeof(m_Wopt));
}

//////////////////////////////////////////////////////////////////////

void CGrapWnd::ReSizeScreen()
{
	CClientDC DispDC(this);

	m_TempDC.SelectObject(m_pOldMap);

	if ( m_Bitmap[0].GetSafeHandle() != NULL )
		m_Bitmap[0].DeleteObject();
	if ( m_Bitmap[1].GetSafeHandle() != NULL )
		m_Bitmap[1].DeleteObject();
	if ( m_Bitmap[2].GetSafeHandle() != NULL )
		m_Bitmap[2].DeleteObject();

	m_Bitmap[m_Maps].CreateCompatibleBitmap(&DispDC, m_MaxX, m_MaxY);
	m_pOldMap = (CBitmap *)m_TempDC.SelectObject(&(m_Bitmap[m_Maps]));
	m_TempDC.FillSolidRect(0, 0, m_MaxX, m_MaxY, m_BakCol);
}
void CGrapWnd::SelectPage(int page)
{
	CClientDC DispDC(this);

	m_TempDC.SelectObject(m_pOldMap);

	if ( m_Bitmap[page].GetSafeHandle() == NULL ) {
		m_Bitmap[page].CreateCompatibleBitmap(&DispDC, m_MaxX, m_MaxY);
		m_pOldMap = (CBitmap *)m_TempDC.SelectObject(&(m_Bitmap[page]));
		m_TempDC.FillSolidRect(0, 0, m_MaxX, m_MaxY, m_BakCol);
	} else
		m_pOldMap = (CBitmap *)m_TempDC.SelectObject(&(m_Bitmap[page]));
}
void CGrapWnd::CreatePen()
{
	int n, c, len;
	int bit = m_Wopt.BitPtn ^ m_Wopt.NegPtn;
	COLORREF fc = (m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol);
	LOGBRUSH LogBrush;
	DWORD map[32];

	if ( m_ActPen.m_hObject != NULL && m_BitPen == bit && m_MulPen == m_Wopt.MulPtn && m_ColPen == fc && m_LwPen == m_Wopt.LineWidth )
		return;

	m_BitPen = bit;
	m_MulPen = m_Wopt.MulPtn;
	m_ColPen = fc;
	m_LwPen  = m_Wopt.LineWidth;

	if ( m_ActPen.m_hObject != NULL )
		m_ActPen.DeleteObject();

	if ( bit == 0xFF ) {
		m_ActPen.CreatePen(PS_SOLID, m_LwPen, m_ColPen);
		return;
	}

	memset(&LogBrush, 0, sizeof(LOGBRUSH));
	LogBrush.lbColor = m_ColPen;
	LogBrush.lbStyle = BS_SOLID;

	for ( c = 0x80, len = 0 ; c != 0 && len < 32 ; ) {
		if ( (len & 1) == 0 ) {		// dots
			for ( n = 0 ; c != 0 && (bit & c) != 0 ; n++ )
				c >>= 1;
		} else {					// space
			for ( n = 0 ; c != 0 && (bit & c) == 0 ; n++ )
				c >>= 1;
		}
		map[len++] = n * m_MulPen;
	}

	m_ActPen.CreatePen(PS_USERSTYLE | PS_GEOMETRIC, m_LwPen, &LogBrush, len, map);
}
void CGrapWnd::CreateBrush()
{
	int n, c;
	int bit = m_Wopt.BitPtn ^ m_Wopt.NegPtn;
	COLORREF fc = (m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol);
	COLORREF bc = m_BakCol;
	CDC dc;
	CBitmap bitmap, *pOldMap;
	CFont font, *pOldFont;
	CString str;
	static const int hatidx[] = {
		HS_BDIAGONAL,	// 45°の右下がりハッチ
		HS_CROSS,		// 水平と垂直の格子ハッチ
		HS_DIAGCROSS,	// 45°の斜め格子ハッチ
		HS_FDIAGONAL,	// 45°の右上がりハッチ
		HS_HORIZONTAL,	// 水平線ハッチ
		HS_VERTICAL,	// 垂直線ハッチ
	};

	if ( !m_Wopt.ShaCtl && !m_FilFlag ) {
		if ( m_ActBrush.m_hObject == NULL ) {
			m_ActBrush.CreateSolidBrush(fc);
			m_BitBrush  = 0xFF;
			m_MulBrush  = 1;
			m_ColBrush  = m_Wopt.IntCol;
			m_CharBrush = 0;
			m_HatBrush  = 0;
			m_MapBrush = FALSE;
		}
		return;
	}

	if ( m_ActBrush.m_hObject != NULL && m_CharBrush == m_Wopt.ShaChar && m_BitBrush == bit && m_MulBrush == m_Wopt.MulPtn && m_ColBrush == fc && m_HatBrush == m_Wopt.HatchIdx )
		return;

	m_BitBrush  = bit;
	m_MulBrush  = m_Wopt.MulPtn;
	m_ColBrush  = fc;
	m_CharBrush = m_Wopt.ShaChar;
	m_HatBrush  = m_Wopt.HatchIdx;
	m_MapBrush  = FALSE;

	if ( m_ActBrush.m_hObject != NULL )
		m_ActBrush.DeleteObject();

	if ( m_CharBrush == 0 ) {
		if ( m_HatBrush >= 1 ) {
			m_ActBrush.CreateHatchBrush(hatidx[(m_HatBrush - 1) % 6], fc);
			return;
		} else if ( bit == 0xFF ) {
			m_ActBrush.CreateSolidBrush(fc);
			return;
		}
	}

	dc.CreateCompatibleDC(NULL);

	if ( m_Wopt.NegPtn ) {
		fc = RGB(255, 255, 255);
		bc = RGB(0, 0, 0);
	} else {
		fc = RGB(0, 0, 0);
		bc = RGB(255, 255, 255);
	}

	if ( m_CharBrush != 0 ) {
		bitmap.CreateBitmap(8, 20, 1, 1, NULL);
		pOldMap = dc.SelectObject(&bitmap);

		font.CreateFont(0 - 20, 8, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
		pOldFont = dc.SelectObject(&font);
		dc.SetTextAlign(TA_LEFT | TA_TOP);
		dc.SetBkMode(OPAQUE);
		dc.SetTextColor(fc);
		dc.SetBkColor(bc);

		str = (CHAR)m_CharBrush;
		dc.TextOut(0, 0, str);

		dc.SelectObject(pOldFont);
		dc.SelectObject(pOldMap);

	} else {
		bitmap.CreateBitmap(1, 8 * m_MulBrush, 1, 1, NULL);
		pOldMap = dc.SelectObject(&bitmap);

		for ( n = 0, c = 0x80 ; c != 0 ; n += m_MulBrush, c >>= 1 )
			dc.FillSolidRect(0, n, 1, m_MulBrush, ((bit & c) != 0 ? fc : bc));

		dc.SelectObject(pOldMap);
	}

	m_ActBrush.CreatePatternBrush(&bitmap);
	m_MapBrush = TRUE;
}
void CGrapWnd::TransparentOpen()
{
	if ( !m_MapBrush || m_Wopt.WriteStyle == WRST_REPLACE )
		return;
	COLORREF bc = m_BakCol ^ (m_Wopt.WriteStyle == WRST_ERASE ? RGB(255,255,255) : 0);
	if ( m_Bitmap[2].GetSafeHandle() == NULL )
		m_Bitmap[2].CreateCompatibleBitmap(&m_TempDC, m_MaxX, m_MaxY);
	m_TempDC.SelectObject(&(m_Bitmap[2]));
	m_TempDC.FillSolidRect(0, 0, m_MaxX, m_MaxY, bc);
}
void CGrapWnd::TransparentClose()
{
	if ( !m_MapBrush || m_Wopt.WriteStyle == WRST_REPLACE )
		return;
	CDC dc;
	COLORREF bc = m_BakCol ^ (m_Wopt.WriteStyle == WRST_ERASE ? RGB(255,255,255) : 0);
	m_TempDC.SelectObject(&(m_Bitmap[m_Maps]));
	dc.CreateCompatibleDC(&m_TempDC);
	dc.SelectObject(&(m_Bitmap[2]));
	m_TempDC.TransparentBlt(0, 0, m_MaxX, m_MaxY, &dc, 0, 0, m_MaxX, m_MaxY, bc);
}
void CGrapWnd::LineTo(int x, int y)
{
	if ( x == m_Pos.x && y == m_Pos.y ) {
		m_TempDC.SetPixelV(x, y, m_Wopt.IntCol);
		return;
	}

	CPoint po[4];
	CPen *pOldPen;
	CBrush *pOldBrush;

	CreatePen();
	CreateBrush();

	pOldPen   = m_TempDC.SelectObject(&m_ActPen);
	pOldBrush = m_TempDC.SelectObject(&m_ActBrush);

	m_TempDC.SetBkMode(m_Wopt.WriteStyle == WRST_REPLACE ? OPAQUE : TRANSPARENT);
	m_TempDC.SetROP2(m_Wopt.WriteStyle == WRST_COMPLEMENT ? R2_NOTXORPEN : R2_COPYPEN);
	m_TempDC.SetTextColor(m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol);
	m_TempDC.SetBkColor(m_BakCol);

	po[0]   = m_Pos;
	po[1].x = x;
	po[1].y = y;

	if ( m_FilFlag ) {
		m_TempDC.LineTo(x, y);
	} else if ( m_Wopt.ShaCtl ) {
		TransparentOpen();
		po[2].x = x;
		po[2].y = m_Wopt.ShaPos.y;
		po[3].x = m_Pos.x;
		po[3].y = m_Wopt.ShaPos.y;
		m_TempDC.Polygon(po, 4);
		TransparentClose();
	} else
		m_TempDC.Polyline(po, 2);

	m_Pos.x = x;
	m_Pos.y = y;

	m_TempDC.SelectObject(pOldPen);
	m_TempDC.SelectObject(pOldBrush);
}
void CGrapWnd::ArcTo(int x, int y)
{
	CRect rect;
	CPoint ct, st, ed, po;
	int r = (int)(sqrt(pow((double)(x - m_Pos.x), 2) + pow((double)(y - m_Pos.y), 2)));
	CPen *pOldPen;
	CBrush *pOldBrush;

	CreatePen();
	CreateBrush();

	pOldPen   = m_TempDC.SelectObject(&m_ActPen);
	pOldBrush = m_TempDC.SelectObject(&m_ActBrush);

	m_TempDC.SetBkMode(m_Wopt.WriteStyle == WRST_REPLACE ? OPAQUE : TRANSPARENT);
	m_TempDC.SetROP2(m_Wopt.WriteStyle == WRST_COMPLEMENT ? R2_NOTXORPEN : R2_COPYPEN);
	m_TempDC.SetTextColor(m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol);
	m_TempDC.SetBkColor(m_BakCol);

	if ( (m_ArcFlag & 001) != 0 ) {	// (C)
		rect.left   = x - r;
		rect.right  = x + r;
		rect.top    = y - r;
		rect.bottom = y + r;
		ct.x = x;
		ct.y = y;
		st.x = m_Pos.x;
		st.y = m_Pos.y;
	} else {
		rect.left   = m_Pos.x - r;
		rect.right  = m_Pos.x + r;
		rect.top    = m_Pos.y - r;
		rect.bottom = m_Pos.y + r;
		ct.x = m_Pos.x;
		ct.y = m_Pos.y;
		st.x = x;
		st.y = y;
	}

	if ( (m_ArcFlag & 002) != 0 ) {	// (A<deg>)
		double q = atan2((double)(st.y - ct.y), (double)(st.x - ct.x));
		q = q - (double)m_ArcDegs * 3.141592 / 180.0;
		if ( m_ArcDegs > 0 ) {
			ed.x = (int)(ct.x + r * cos(q));
			ed.y = (int)(ct.y + r * sin(q));
			if ( (m_ArcFlag & 001) != 0 )
				m_Pos = ed;
		} else {
			ed = st;
			st.x = (int)(ct.x + r * cos(q));
			st.y = (int)(ct.y + r * sin(q));
			if ( (m_ArcFlag & 001) != 0 )
				m_Pos = st;
		}
	} else {
		ed = st;
	}

	if ( m_FilFlag ) {
		m_TempDC.ArcTo(rect, st, ed);
	} else if ( m_Wopt.ShaCtl ) {
		TransparentOpen();
		m_TempDC.BeginPath();
		m_TempDC.MoveTo(ed);
		po.x = ed.x;
		po.y = m_Wopt.ShaPos.y;
		m_TempDC.LineTo(po);
		po.x = st.x;
		po.y = m_Wopt.ShaPos.y;
		m_TempDC.LineTo(po);
		m_TempDC.ArcTo(rect, st, ed);
		m_TempDC.EndPath();
		m_TempDC.FillPath();
		TransparentClose();

	} else
		m_TempDC.Arc(rect, st, ed);

	m_TempDC.SelectObject(pOldPen);
	m_TempDC.SelectObject(pOldBrush);
}
void CGrapWnd::UserFontDraw(int x, int y, COLORREF fc, COLORREF bc, BYTE *ptn)
{
	int nx, ny;
	int ix, iy;
	int dx, dy;
	double r = (double)(m_Topt.FontAngle) * 3.141592 / 180.0;

	for ( ny = 0 ; ny < m_Topt.UnitSize.y ; ny++ ) {
		for ( nx = 0 ; nx < m_Topt.UnitSize.x ; nx++ ) {
			ix = nx *  8 / m_Topt.UnitSize.x;
			iy = ny * 10 / m_Topt.UnitSize.y;

			dx = x + (int)((double)nx * cos(r) + (double)ny * sin(r) + 0.5);
			dy = y - (int)((double)nx * sin(r) - (double)ny * cos(r) + 0.5);

			if ( (ptn[iy] & (0x80 >> ix)) != 0 )
				m_TempDC.SetPixelV(dx, dy, fc);
			else if ( m_Wopt.WriteStyle == WRST_REPLACE )
				m_TempDC.SetPixelV(dx, dy, bc);
		}
	}
}
void CGrapWnd::TextTo(LPCSTR str)
{
	CFont font, *pOldFont;
	int nx, sr;
	CPoint po, so;
	double r = (double)(m_Topt.FontAngle) * 3.141592 / 180.0;
	CBuffer in, out;
	LPCWSTR s;
	DWORD ch;
	COLORREF fc, bc;

	if ( m_Wopt.NegPtn ) {
		fc = m_BakCol;
		bc = m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol;
	} else {
		fc = m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol;
		bc = m_BakCol;
	}

	font.CreateFont(0 - m_Topt.UnitSize.y, m_Topt.UnitSize.x, m_Topt.FontAngle * 10, 0, FW_DONTCARE, (m_Topt.FontItalic < 0 ? TRUE : FALSE), 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
	pOldFont = m_TempDC.SelectObject(&font);

	m_TempDC.SetTextAlign(TA_LEFT | TA_TOP);
	m_TempDC.SetBkMode(m_Wopt.WriteStyle == WRST_REPLACE ? OPAQUE : TRANSPARENT);
	m_TempDC.SetROP2(m_Wopt.WriteStyle == WRST_COMPLEMENT ? R2_NOTXORPEN : R2_COPYPEN);
	m_TempDC.SetTextColor(fc);
	m_TempDC.SetBkColor(bc);

	in.Apend((LPBYTE)(str), (int)strlen(str));
	m_pTextRam->m_IConv.RemoteToStr(m_pTextRam->m_SendCharSet[m_pTextRam->m_KanjiMode], &in, &out);

	so = m_Pos;
	nx = 0;

	for ( s = out ; *s != L'\0' ; s += sr ) {
		if ( *s >= L' ' ) {
			po.x = m_Pos.x + (int)((double)(m_Topt.OffsetSize.x) * cos(r) + (double)(m_Topt.OffsetSize.y) * sin(r) + 0.5);
			po.y = m_Pos.y - (int)((double)(m_Topt.OffsetSize.x) * sin(r) - (double)(m_Topt.OffsetSize.y) * cos(r) + 0.5);

			//  1101 10xx	U+D800 - U+DBFF	上位サロゲート	1101 11xx	U+DC00 - U+DFFF	下位サロゲート
			if ( (s[0] & 0xFC00) == 0xD800 && (s[1] & 0xFC00) == 0xDC00 ) {
				ch = (((DWORD)(s[0]) << 16) | ((DWORD)(s[1])));
				sr = 2;
			} else {
				ch = *s;
				sr = 1;
			}
			// VARIATION SELECTOR
			// U+180B - U+180D or U+FE00 - U+FE0F
			// U+DB40 U+DD00 - U+DB40 U+DDEF
			if ( (s[sr] >= 0x180B && s[sr] <= 0x180D) || (s[sr] >= 0xFE00 && s[sr] <= 0xFE0F) )
				sr += 1;
			else if ( s[sr] == 0xDB40 && s[sr + 1] >= 0xDD00 && s[sr + 1] <= 0xDDEF )
				sr += 2;

			if ( m_Topt.FontSelect == 0 )
				::TextOutW(m_TempDC.m_hDC, po.x, po.y, s, sr);
			else {
				if ( (ch = ch - L' ') < 0 ) ch = 0;
				else if ( ch > 95 ) ch = 95;
				UserFontDraw(po.x, po.y, fc, bc, m_UserFont[m_Topt.FontSelect][ch]);
			}

			nx += m_pTextRam->UnicodeWidth(ch);
			m_Pos.x = so.x + (int)((double)(m_Topt.SpaceSize.x * nx) * cos(r) + (double)(m_Topt.SpaceSize.y * nx) * sin(r) + 0.5);
			m_Pos.y = so.y - (int)((double)(m_Topt.SpaceSize.x * nx) * sin(r) - (double)(m_Topt.SpaceSize.y * nx) * cos(r) + 0.5);
		} else if ( *s == L'\r' ) {
			m_Pos = so;
			nx = 0;
			sr = 1;
		} else if ( *s == L'\n' ) {
			m_Pos.x = m_Pos.x + (int)((double)(m_Topt.DispSize.y) * sin(r) + 0.5);
			m_Pos.y = m_Pos.y + (int)((double)(m_Topt.DispSize.y) * cos(r) + 0.5);
			so = m_Pos;
			nx = 0;
			sr = 1;
		} else {
			sr = 1;
		}
	}

	m_TempDC.SelectObject(pOldFont);
}
void CGrapWnd::SmoothLine(BOOL closed)
{
	int n;
	double a, b, q, r, k;
	CPoint tmp;
	CArray<CPoint, CPoint &> po;
	int max = (int)m_ArcPoint.GetSize();

	if ( max < 2 ) {
		for ( n = 0 ; n < max ; n++ )
			po.Add(m_ArcPoint[n]);
		if ( !closed )
			m_Pos = m_ArcPoint[max-1];

	} else {
		k = 0.5522847 * 0.5;

		for ( n = 0 ; n < (max - 1) ; n++ ) {
			if ( n == 0 ) {
				po.Add(m_ArcPoint[n]);
				if ( closed ) {
					a = (double)(m_ArcPoint[n+1].x - m_ArcPoint[max-1].x);
					b = (double)(m_ArcPoint[n+1].y - m_ArcPoint[max-1].y);
				} else {
					a = 0.0;
					b = 0.0;
				}
			} else {
				a = (double)(m_ArcPoint[n+1].x - m_ArcPoint[n-1].x);
				b = (double)(m_ArcPoint[n+1].y - m_ArcPoint[n-1].y);
			}

			q = atan2(b, a);
			r = sqrt(a * a + b * b);
			tmp.x = m_ArcPoint[n].x + (int)(r * k * cos(q));
			tmp.y = m_ArcPoint[n].y + (int)(r * k * sin(q));
			po.Add(tmp);

			if ( (n + 2) < max ) {
				a = (double)(m_ArcPoint[n].x - m_ArcPoint[n+2].x);
				b = (double)(m_ArcPoint[n].y - m_ArcPoint[n+2].y);
			} else if ( closed ) {
				a = (double)(m_ArcPoint[n].x - m_ArcPoint[0].x);
				b = (double)(m_ArcPoint[n].y - m_ArcPoint[0].y);
			} else {
				a = 0.0;
				b = 0.0;
			}

			q = atan2(b, a);
			r = sqrt(a * a + b * b);
			tmp.x = m_ArcPoint[n+1].x + (int)(r * k * cos(q));
			tmp.y = m_ArcPoint[n+1].y + (int)(r * k * sin(q));
			po.Add(tmp);

			po.Add(m_ArcPoint[n+1]);
		}

		if ( closed ) {
			a = (double)(m_ArcPoint[0].x - m_ArcPoint[max-2].x);
			b = (double)(m_ArcPoint[0].y - m_ArcPoint[max-2].y);

			q = atan2(b, a);
			r = sqrt(a * a + b * b);
			tmp.x = m_ArcPoint[max-1].x + (int)(r * k * cos(q));
			tmp.y = m_ArcPoint[max-1].y + (int)(r * k * sin(q));
			po.Add(tmp);

			a = (double)(m_ArcPoint[max-1].x - m_ArcPoint[1].x);
			b = (double)(m_ArcPoint[max-1].y - m_ArcPoint[1].y);

			q = atan2(b, a);
			r = sqrt(a * a + b * b);
			tmp.x = m_ArcPoint[0].x + (int)(r * k * cos(q));
			tmp.y = m_ArcPoint[0].y + (int)(r * k * sin(q));
			po.Add(tmp);

			po.Add(m_ArcPoint[0]);

		} else
			m_Pos = m_ArcPoint[max-1];
	}

	CPen *pOldPen;
	CBrush *pOldBrush;

	CreatePen();
	CreateBrush();

	pOldPen   = m_TempDC.SelectObject(&m_ActPen);
	pOldBrush = m_TempDC.SelectObject(&m_ActBrush);

	m_TempDC.SetBkMode(m_Wopt.WriteStyle == WRST_REPLACE ? OPAQUE : TRANSPARENT);
	m_TempDC.SetROP2(m_Wopt.WriteStyle == WRST_COMPLEMENT ? R2_NOTXORPEN : R2_COPYPEN);
	m_TempDC.SetTextColor(m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol);
	m_TempDC.SetBkColor(m_BakCol);

	if ( m_FilFlag )
		m_TempDC.PolyBezierTo(po.GetData() + 1, (int)po.GetSize() - 1);		// XXXX バグ？仕様？
	else if ( m_Wopt.ShaCtl ) {
		TransparentOpen();
		m_TempDC.BeginPath();
		m_TempDC.MoveTo(po[po.GetSize() - 1]);
		tmp.x = po[po.GetSize() - 1].x;
		tmp.y = m_Wopt.ShaPos.y;
		m_TempDC.LineTo(tmp);
		tmp.x = po[0].x;
		tmp.y = m_Wopt.ShaPos.y;
		m_TempDC.LineTo(tmp);
		m_TempDC.LineTo(po[0]);
		m_TempDC.PolyBezierTo(po.GetData() + 1, (int)po.GetSize() - 1);
		m_TempDC.EndPath();
		m_TempDC.FillPath();
		TransparentClose();
	} else
		m_TempDC.PolyBezier(po.GetData(), (int)po.GetSize());

	m_TempDC.SelectObject(pOldPen);
	m_TempDC.SelectObject(pOldBrush);
}
void CGrapWnd::FillOpen()
{
	m_TempDC.BeginPath();
	m_TempDC.MoveTo(m_Pos);
}
void CGrapWnd::FillClose()
{
	CBrush *pOldBrush;

	CreateBrush();
	pOldBrush = m_TempDC.SelectObject(&m_ActBrush);

	m_TempDC.SetBkMode(m_Wopt.WriteStyle == WRST_REPLACE ? OPAQUE : TRANSPARENT);
	m_TempDC.SetROP2(m_Wopt.WriteStyle == WRST_COMPLEMENT ? R2_NOTXORPEN : R2_COPYPEN);
	m_TempDC.SetTextColor(m_Wopt.WriteStyle == WRST_ERASE ? m_BakCol : m_Wopt.IntCol);
	m_TempDC.SetBkColor(m_BakCol);

	TransparentOpen();
	m_TempDC.EndPath();
	m_TempDC.FillPath();
	TransparentClose();

	m_TempDC.SelectObject(pOldBrush);
}

//////////////////////////////////////////////////////////////////////

void CGrapWnd::ParseInit(int cmd)
{
	if ( (m_ArcFlag & 070) != 0 && m_ArcPoint.GetSize() > 1 ) {
		SmoothLine((m_ArcFlag & 010) != 0 ? TRUE : FALSE);
		m_ArcPoint.RemoveAll();
	}

	m_ArcFlag = 0;
	m_UseXPos = FALSE;
	m_FilFlag = FALSE;

	PopWparam();
}
int CGrapWnd::ParseNumber(LPCSTR &p, int cmd, int num)
{
	int ch = 0;
	int val = GetDigit(p, 0);

	switch(cmd) {
	case ('S' << 8) | 'M':
		// S(M<num>)	Output mapping values
		if ( (m_ActMap = val) > 15 )
			m_ActMap = 15;
		break;
	case ('S' << 8) | 'I':
		// S(I<n>) 0 Background intensity (monochrome) Selects output map location <n> for the background.
		if ( val < 0 ) val = 0; else if ( val > 15 ) val = 15;
		m_BakCol = m_ColMap[val];
  		break;
	case ('S' << 8) | 'T':
		// S(T<0 to 32767>) None Time delay Selects the number of ticks of the real time clock to count for a delay.
  		break;
	case ('S' << 8) | 'C':
		// S(C<0 or 1>) 1 Graphics cursor on/off Turns the graphics output cursor of (C0) or on (C1).
		break;
	case ('S' << 16) | ('C' << 8 ) | 'H':
		// S(C1(H<n>)) 0 (diamond) Graphics output cursor H selects the output cursor style.
		//	0 or 1 = diamond.
		//	2 = crosshair.
		break;
	case ('S' << 16) | ('C' << 8 ) | 'I':
		// S(C1(I<n>)) 0 (crosshair) Graphics input cursor I selects the input cursor style.
		//	0 or 2 = crosshair.
		//	1 = diamond.
		//	3 = rubber band line.
		//	4 = rubber band rectangle.
		break;
	case ('S' << 8) | 'P':
		// S(P<0 or 1>) 0 (first page) Display graphics page Selects which graphics page the terminal displays. Useful for single sessions only.
		//	0 = first page.
		//	1 = second page.
		m_Maps = (val == 1 ? 1 : 0);
		SelectPage(m_Maps);
		break;

	case ('S' << 16) | ('W' << 8 ) | 'M':
		// S(W(M<n>>)) Current value set in write command Pixel vector multiplier Selects a multiplication factor of <n> for each PV value in a scroll command. <n> defines the number of coordinates affected by each PV value. This is temporary write command, in effect until the next command key letter.
	case ('P' << 16) | ('W' << 8) | 'M':
	case ('W' << 8) | 'M':
		// W(M<n>) 1 PV multiplication Defines a multiplication factor of <n> for all pixel vector (PV) values used in later commands. Can serve as a temporary write control for other types of commands.
		m_Wopt.Pvalue = val;
		break;
	case ('W' << 16) | ('P' << 8 ) | 'M':
		// W(P(M<1 to 16>)) 2 Pattern multiplication Selects how many consecutive pixels <1 to 16> to write each bit off pattern memory to. You can use this option in three ways.
		//	•with the select standard pattern option
		//	•with the specify binary pattern option
		//	•by itself, to define a multiplication factor for the last specified pattern 
		if ( (m_Wopt.MulPtn = val) < 1 ) m_Wopt.MulPtn = 1; else if ( m_Wopt.MulPtn > 16 ) m_Wopt.MulPtn = 16;
		break;
	case ('W' << 8) | 'I':
		// S(I<0 to 15>) 7 (VT340) Select foreground intensity or color Selects the output map address (<0 to 3>) to use for writing.
		if ( val < 0 ) val = 0 ; else if ( val > 15 ) val = 15;
		m_Wopt.IntCol = m_ColMap[val];
		break;
	case ('W' << 8) | 'F':
		// W(F<0 to 15>) 15 (VT340) Plane select Selects which of the terminal's bitmap planes ReGIS can write to.
		break;
	case ('W' << 8) | 'N':
		// W(N<0 or 1>) 0 Negative pattern control When set to (N1), reverses the effect of currently selected write pattern.
		m_Wopt.NegPtn = (val == 0 ? 0x00 : 0xFF);
		break;
	case ('W' << 8) | 'S':
		// W(S<0 or 1>) 0 Shading on/off control When set to (S1), turns on shading with currently selected pattern. The shading reference line is defined by the Y-coordinate of the active position when (S1) is selected.
		m_Wopt.ShaCtl = (val == 0 ? FALSE : TRUE);
		m_Wopt.ShaPos = m_Pos;
		m_Wopt.ShaChar = 0;
		m_Wopt.HatchIdx = (val >= 2 ? (val - 1) : 0);
		break;
	case ('W' << 8) | 'L':
		if ( (m_Wopt.LineWidth = val) < 1 )
			m_Wopt.LineWidth = 1;
		break;

	case ('P' << 8) | 'P':
		// P(P<pn>) None Select graphics page option Moves the cursor from the current page to page <pn> in graphics page memory, where <pn> is 0 or 1.
		if ( val != 1 ) val = 0;
		SelectPage(val);
		break;

	case ('C' << 8) | 'A':
		// C(A<degrees>)[X,Y] 360 Arc with center at current position [X,Y] defines the starting point for an arc.
		//	<degrees> is a signed value the determines the size and direction of arc.
		//	+ = counterclockwise<
		//	- = clockwise
		m_ArcFlag |= 002;
		m_ArcDegs = val;
		break;

	case ('L' << 8) | 'A':
		// L(A<1 to 3>) 1 Select character set Selects one of three loadable character sets to use for any following load commands.
		//  (A<1 to 3>"<name>") You can use this option with the select character set option
		if ( (m_ActFont = val) < 1 ) m_ActFont = 1;
		else if ( m_ActFont > 3 ) m_ActFont = 3;
		while ( *p != '\0' && *p <= ' ' )
			p++;
		if ( *p == '\'' || *p == '"' ) {
			m_UserFontName[m_ActFont].Empty();
			ch = *(p++);
			while ( *p != '\0' && *p != ch )
				m_UserFontName[m_ActFont] += (CHAR)(*(p++));
			if ( *p == ch )
				p++;
		}
		break;

	case ('T' << 8) | 'A':
		// T(A<0 to 3>) 0 Select character set Selects one of four possible character sets (<0 to 3>) to use for text string characters.
		//	(A0) built-in set 
		//	(A1 to 3) one of three loadable sets 
		if ( (m_Topt.FontSelect = val) < 0 ) m_Topt.FontSelect = 0;
		else if ( m_Topt.FontSelect > 3 ) m_Topt.FontSelect = 3;
		break;
	case ('T' << 8) | 'S':
		// T(S<0 to 16>) 1 Standard character cell size Selects 1 of 17 standard sets. The set defines the display cell, unit cell, and character positioning values for text characters.
		if ( val < 0 ) val = 0; else if ( val > 16 ) val = 16;
		m_Topt.DispSize = FontTab[val][0];
		m_Topt.UnitSize = FontTab[val][1];
		m_Topt.SpaceSize.x = m_Topt.DispSize.x;
		m_Topt.SpaceSize.y = 0;
		break;
	case ('T' << 8) | 'H':
		// T(H<height>) 2 Height multiplier Changes the height of the display cell and unit cell, without affecting the width or positioning values. New height is equal to 10 times the specified multiplier (<1-256>).
		m_Topt.UnitSize.y = val * 10;
		break;
	case ('T' << 8) | 'D':
		// T(D<a> S<0 to 16>) (D0 S1) String tilt Selects a tilt angle for text strings, relative to the current baseline.
		//	<a> Selects the degrees of the tilt. (45 degree increments) 
		//	<0 to 16> Selects a standard size to use for positioning tilted characters. 
		m_Topt.FontAngle = val;
		m_Topt.SpaceSize.x = m_Topt.DispSize.x;
		m_Topt.SpaceSize.y = 0;
		break;
	case ('T' << 8) | 'I':
		// T(I<a>) 0 Italics Selects a tilt angle <a> for italic characters without changing their orientation to the current baseline. Angle is ± 22 or 45 degrees.
		m_Topt.FontItalic = val;
		break;
 	}

	return ch;
}
int CGrapWnd::ParseVector(LPCSTR &p, int cmd, int num)
{
	switch(cmd) {
	case 'S':
		// S<0 to 7> 0 Scrolling with PV offset
		break;

	case ('W' << 8) | 'P':
		// S(P<0 to 9>) 1 Select standard pattern Selects 1 of 10 stored writing patterns.
		// S(P<binary>) 1 Specify binary pattern Lets you create your own writing pattern, up to 8 bits in length.
		{
			int n, b;
			for ( n = 0 ; isdigit(p[n]) ; n++ )
				;
			if ( n <= 1 ) {
				for ( n = 0 ; isdigit(*p) ; )
					n = n * 10 + (*(p++) - '0');
				m_Wopt.BitPtn = m_PtnMap[n];
			} else {
				for ( n = 0, b = 0x80 ; isdigit(*p) ; b >>= 1 ) {
					if ( *(p++) != '0' )
						n |= b;
				}
				m_Wopt.BitPtn = n;
			}
			m_Wopt.ShaChar = 0;
			m_Wopt.HatchIdx = 0;
		}
		break;

	case ('F' << 8) | 'P':
		FillClose();
	case 'P':
		// P<PV> None Cursor positioning with PV values The pixel vector values select a direction and distance to move, relative to the current cursor position.
		while ( isdigit(*p) ) {
			switch(*(p++)) {
			case '0': m_Pos.x += m_Wopt.Pvalue; break;
			case '1': m_Pos.x += m_Wopt.Pvalue; m_Pos.y -= m_Wopt.Pvalue; break;
			case '2': m_Pos.y -= m_Wopt.Pvalue; break;
			case '3': m_Pos.x -= m_Wopt.Pvalue; m_Pos.y -= m_Wopt.Pvalue; break;
			case '4': m_Pos.x -= m_Wopt.Pvalue; break;
			case '5': m_Pos.x -= m_Wopt.Pvalue; m_Pos.y += m_Wopt.Pvalue; break;
			case '6': m_Pos.y += m_Wopt.Pvalue; break;
			case '7': m_Pos.x += m_Wopt.Pvalue; m_Pos.y += m_Wopt.Pvalue; break;
			}
		}
		if ( cmd == (('F' << 8) | 'P') )
			FillOpen();
 		break;

	case ('F' << 8) | 'V':
	case 'V':
		// V<PV> Draw line (with PV value) Draws a line from the current active position to a relative position defined by <PV>. The PV value defines a direction.
		while ( isdigit(*p) ) {
			int x = m_Pos.x, y = m_Pos.y;

			switch(*(p++)) {
			case '0': x += m_Wopt.Pvalue; break;
			case '1': x += m_Wopt.Pvalue; y -= m_Wopt.Pvalue; break;
			case '2': y -= m_Wopt.Pvalue; break;
			case '3': x -= m_Wopt.Pvalue; y -= m_Wopt.Pvalue; break;
			case '4': x -= m_Wopt.Pvalue; break;
			case '5': x -= m_Wopt.Pvalue; y += m_Wopt.Pvalue; break;
			case '6': y += m_Wopt.Pvalue; break;
			case '7': x += m_Wopt.Pvalue; y += m_Wopt.Pvalue; break;
			}

			LineTo(x, y);
		}
 		break;

	case 'T':
		// T<PV> None PV spacing Selects PV value to use for superscripts, subscripts, and overstriking.
		while ( isdigit(*p) ) {
			switch(*(p++)) {
			case '0':
				m_Topt.OffsetSize.x += (m_Topt.UnitSize.x / 2);
				break;
			case '1':	//	1 Superscript – Moves the character up from the baseline and away from the previous character. 
				m_Topt.OffsetSize.y -= (m_Topt.UnitSize.y / 2);
				break;
			case '2':	//	2 Superscript – Moves the character straight up from the baseline. 
				m_Topt.OffsetSize.y -= m_Topt.UnitSize.y;
				break;
			case '3':	//	3 moves the character up and back toward the previous character.
				m_Topt.OffsetSize.x -= (m_Topt.UnitSize.x / 2);
				m_Topt.OffsetSize.y -= (m_Topt.UnitSize.y / 2);
				break;
			case '4':	//	4 Overstrike – A 44 value moves the character back over the previous character cell. 
				m_Topt.OffsetSize.x -= (m_Topt.UnitSize.x / 2);
				break;
			case '5':	// 5 moves the character down and back toward the previous character.
				m_Topt.OffsetSize.x += (m_Topt.UnitSize.x / 2);
				m_Topt.OffsetSize.y += (m_Topt.UnitSize.y / 2);
				break;
			case '6':	//	6 Subscript – Moves the character straight down from the baseline. 
				m_Topt.OffsetSize.y += m_Topt.UnitSize.y;
				break;
			case '7':	//	7 Subscript – Moves the character down from the baseline and away from the previous character. 
				m_Topt.OffsetSize.y += (m_Topt.UnitSize.y / 2);
				break;
			}
		}
  		break;
	}

	while ( isdigit(*p) )
		p++;
	return *p;
}
int CGrapWnd::ParseBacket(LPCSTR &p, int cmd, int num)
{
	int ch, x = 0, y = 0;

	switch(cmd) {
	case ('S' << 8) | 'A':
		if ( (num & 1) != 0 ) {
			x = m_MaxX - 1;
			y = m_MaxY - 1;
		}
		break;
	case ('F' << 8) | 'V':
	case ('F' << 8) | 'C':
	case ('F' << 8) | 'P':
	case 'V':
	case 'C':
	case 'P':
		x = m_Pos.x;
		y = m_Pos.y;
		break;
	case ('W' << 8) | 'S':
		x = m_Wopt.ShaPos.x;
		y = m_Wopt.ShaPos.y;
		break;
	case ('T' << 8) | 'S':
		x = m_Topt.DispSize.x;
		y = m_Topt.DispSize.y;
 		break;
	case ('T' << 8) | 'U':
		x = m_Topt.UnitSize.x;
		y = m_Topt.UnitSize.y;
 		break;
	case ('T' << 8) | 'M':
		x = 1;
		y = 2;
		break;
	}

	ch = GetBacket(p, x, y);

	switch(cmd) {
	case 'S':
		// Scrolling with relative X and Y values
		// S[X,Y]				[0,0]
		break;
	case ('S' << 8) | 'A':
		// S(A[X1,Y1][X2,Y2]) [0,0][799,479] Display addressing Lets you define screen addressing that uses a different size or orientation than the default VT300 screen.
		if ( (num & 1) != 0 && (m_MaxX != x || m_MaxY != y) ) {
			if ( (m_MaxX = x + 1) < 10 )
				m_MaxX = 10;
			if ( (m_MaxY = y + 1) < 10 )
				m_MaxY = 10;
			ReSizeScreen();
		}
		break;
	case ('S' << 8) | 'H':
		// S(H[X,Y]) [0,0] Print defined area (one position) Uses an [X,Y] screen coordinate and the current cursor position to define opposite corners of the area to print.
		// S(H[X,Y][X,Y]) [0,0][799,479] Print defined area (two positions) Uses two [X,Y] screen coordinates to define opposite corners of the area to print.
		break;
	case ('S' << 16) | ('H' << 8 ) | 'P':
		// S(H(P[X,Y])) [50,0] Print offset suboption Defines where the upper-left corner of an image will print, using a relative offset from the current printhead location.
		// The default at power-up is [50,0], until you define a new value. Any new value remains in effect until redefined.
		break;

	case ('W' << 8) | 'S':
		// W(S[,Y]) Current Y position Select horizontal shading reference line Selects a line defined by [,Y], which can be either an absolute or relative value.
		// W(S(X)[X]) Current X position Select vertical shading reference line Selects a line defined by [X], which can be either an absolute or relative value.
		if ( m_UseXPos )
			m_Wopt.ShaPos.x = x;
		else
			m_Wopt.ShaPos.y = y;
		m_UseXPos = FALSE;
		break;

	case ('F' << 8) | 'P':
		FillClose();
		m_Pos.x = x;
		m_Pos.y = y;
		FillOpen();
		break;
	case 'P':
		// P[X,Y] [0,0] Cursor positioning with [X,Y] values The [X,Y] values can be absolute, relative, or absolute/relative screen coordinates.
		m_Pos.x = x;
		m_Pos.y = y;
		break;

	case ('F' << 8) | 'V':
	case 'V':
		// V[] Draw dot Draws one dot (a single pixel at the current active position. Does not move the cursor.
		// V[X,Y] Draw line (with coordinate) Draws a line from the current active position to the [X,Y] position. You can use absolute, relative, or absolute/relative values for [X,Y].
		LineTo(x, y);
		break; 

	case ('F' << 8) | 'C':
	case 'C':
		// C[X,Y] None Circle with center at current position [X,Y] defines a point on the circumference of the circle.
		if ( (m_ArcFlag & 030) != 0 ) {
			CPoint po(x, y);
			m_ArcPoint.Add(po);
			m_Pos = po;
		} else {
			ArcTo(x, y);
		}
 		break;

	case 'T':
		// T[X,Y] [9,0]* Character positioning Lets you vary spacing between text characters. [X,Y] values are relative.
		m_Topt.SpaceSize.x = x;
		m_Topt.SpaceSize.y = y;
		break;
	case ('T' << 8) | 'S':
		// T(S[<width,height>]) [9,20]* Display cell size Lets you change the size of the screen area used for each character.
		m_Topt.DispSize.x = x;
		m_Topt.DispSize.y = y;
		m_Topt.SpaceSize.x = x;
		m_Topt.SpaceSize.y = 0;
 		break;
	case ('T' << 8) | 'U':
		// T(U[<width,height>]) [8,20]* Unit cell size Lets you change scaling of characters.
		m_Topt.UnitSize.x = x;
		m_Topt.UnitSize.y = y;
		break;
	case ('T' << 8) | 'M':
		// T(M[width,height]) [1,2] Size multiplication Selects a new unit cell size. Multiplies the height and width of the standard S1 unit cell by the factors you select. The maximum width factor is 16.
		m_Topt.DispSize.x =  9 * x;
		m_Topt.DispSize.y = 20 * y;
		m_Topt.UnitSize.x =  8 * x;
		m_Topt.UnitSize.y = 20 * y;
		m_Topt.SpaceSize.x = 9 * x;
		m_Topt.SpaceSize.y = 0;
		break;
	}

	return ch;
}
int CGrapWnd::ParseOption(LPCSTR &p, int cmd, int num)
{
	int ch = 0;
	COLORREF rgb;

	switch(cmd) {
	case 'S':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, SCmd)) != ')' )
			return (-1);
		break;
	case ('S' << 8) | 'H':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, SHCmd)) != ')' )
			return (-1);
 		break;
	case ('S' << 8) | 'M':
		// S(M<n>(<Lvalue>))	S(M<n>(<RGB>))		(M<n>(HLS))
		if ( (ch = GetColor(p, rgb)) != ')' )
			return (-1);
		m_ColMap[m_ActMap] = rgb;
 		break;
	case ('S' << 8) | 'I':
		// S(I(RGB)) D Background intensity (RGB color) Selects the output map location containing the closest color to the RGB value you specified.
		// S(I(HLS)) L0 Background intensity (HLS color) Selects the output map location containing the closest color to the HLS value you specified.
		if ( (ch = GetColor(p, rgb)) != ')' )
			return (-1);
		m_BakCol = rgb;
 		break;
	case ('S' << 8) | 'W':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, SWCmd)) != ')' )
			return (-1);
 		break;

	case 'W':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, WCmd)) != ')' )
			return (-1);
		break;
	case ('W' << 8) | 'P':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, SWCmd)) != ')' )
			return (-1);
 		break;
	case ('W' << 8) | 'I':
		// W(I(<RGB>)) Current color map Select foreground intensity or color (RGB) Selects the output map address to use for color writing with RGB values. Selects the color closest to the RGB value specified.
		// W(I(<HLS>)) None Select foreground intensity or color (HLS) Selects the output map address to use for color writing with HLS values. Selects the color closest to the HLS value specified.
		if ( (ch = GetColor(p, rgb)) != ')' )
			return (-1);
		m_Wopt.IntCol = rgb;
 		break;
	case ('W' << 8) | 'S':
		// W(S(X)[X]) Current X position Select vertical shading reference line Selects a line defined by [X], which can be either an absolute or relative value.
		if ( (ch = GetChar(p)) != 'X' )
			return (-1);
		if ( (ch = GetChar(p)) != ')' )
			return (-1);
		m_UseXPos = TRUE;
		break;

	case 'P':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, PCmd)) != ')' )
			return (-1);
		break;
	case ('P' << 8) | 'W':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, SWCmd)) != ')' )
			return (-1);
 		break;

	case 'V':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, VCmd)) != ')' )
			return (-1);
		break;
	case ('V' << 8) | 'W':
		PushWparam();
		if ( (ch = ParseCmds(p, 'W' << 8, NULL, WCmd)) != ')' )
			return (-1);
 		break;

	case 'C':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, CCmd)) != ')' )
			return (-1);
 		break;
	case ('C' << 8) | 'W':
		PushWparam();
		if ( (ch = ParseCmds(p, 'W' << 8, NULL, WCmd)) != ')' )
			return (-1);
 		break;

	case 'T':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, TCmd)) != ')' )
			return (-1);
		break;
	case ('T' << 8) | 'A':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, TACmd)) != ')' )
			return (-1);
		break;
	case ('T' << 8) | 'W':
		PushWparam();
		if ( (ch = ParseCmds(p, 'W' << 8, NULL, WCmd)) != ')' )
			return (-1);
 		break;

	case 'L':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, LCmd)) != ')' )
			return (-1);
		break;

	case 'R':
		if ( (ch = ParseCmds(p, cmd << 8, NULL, RCmd)) != ')' )
			return (-1);
		break;
	case ('R' << 8) | 'M':
	case ('R' << 8) | 'P':
		// (M(<call letter>)) None Macrograph contents Reports the contents of the macrograph identified by <call letter>.
		// (M(=)) None Macrograph storage status Reports how much space the terminal has assigned to macrograph storage, and how much of that space is currently free.
		// (P(I)) None Report position interactive Requests an input cursor position report.
		while ( (ch = GetChar(p)) != (-1) && ch != ')' )
			;
		break;

	case 'F':
		m_FilFlag = TRUE;
		FillOpen();
		if ( (ch = ParseCmds(p, cmd << 8, NULL, FCmd)) != ')' )
			return (-1);
		FillClose();
		m_FilFlag = FALSE;
		break;
	case ('F' << 8) | 'P':
		FillClose();
		if ( (ch = ParseCmds(p, 'P' << 8, NULL, PCmd)) != ')' )
			return (-1);
		FillOpen();
 		break;
	case ('F' << 8) | 'V':
		if ( (ch = ParseCmds(p, 'V' << 8, NULL, VCmd)) != ')' )
			return (-1);
 		break;
	case ('F' << 8) | 'C':
		if ( (ch = ParseCmds(p, 'C' << 8, NULL, CCmd)) != ')' )
			return (-1);
 		break;
	case ('F' << 8) | 'W':
		PushWparam();
		if ( (ch = ParseCmds(p, 'W' << 8, NULL, WCmd)) != ')' )
			return (-1);
 		break;
	}
	return ch;
}
int CGrapWnd::ParseString(LPCSTR &p, int cmd, int num)
{
	int n;
	int ch, em = GetChar(p);
	CStringA str;

	while ( *p != '\0' && *p != em )
		str += (TCHAR)(*(p++));
	ch = GetChar(p);

	switch(cmd) {
	case ('W' << 8) | 'S':
		// W(S'<character>') None Select shading character Selects a character to use for shading, instead of writing pattern.
		m_Wopt.ShaCtl = TRUE;
		m_Wopt.ShaPos = m_Pos;
		m_Wopt.ShaChar = str.IsEmpty() ? 0 : str[0];
		m_Wopt.HatchIdx = 0;
		break;

	case 'T':
		// T'xxx'
		TextTo(str);
		break;
	case ('T' << 16) | ('A' << 8 ) | 'L':
		// T(A0(L"<designator>")) "(B" Select character set (left) Used with the (A0) option to select a built-in 7-bit set for the left side (GL) of the code table. Default: ASCII set.
		break;
	case ('T' << 16) | ('A' << 8 ) | 'R':
		// T(A0(R"<designator>")) "-A" Select character set (right) Used with the (A0) option to select a built-in 7-bit or 8-bit set for the right side (GR) of the code table. Default: ISO Latin-1 supplemental graphic set.
		break;

	case 'L':
		// L"<ASCII>"<hex pairs> – Load character cell Loads a character into the currently selected loadable set.
		//	<ASCII> is an ASCII character you use to select the loadable character in other commands. 
		//	<hex pairs> define the bit pattern for each line of the character. 
		if ( !str.IsEmpty() ) {
			if ( (n = str[0] - ' ') < 0 ) n = 0; 
			else if ( n > 95 ) n = 95;
		} else
			n = 0;
		GetHexTab(p, 10, m_UserFont[m_ActFont][n]);
		break;
	case ('L' << 8) | 'A':
		// L(A"<name>") " " Specify name Selects a name of up to 10 characters for the currently selected loadable character set. You can use this option with the select character set option: (A<1 to 3>"<name>").
		for ( n = 1 ; n < 4 ; n++ ) {
			if ( str.Compare(m_UserFontName[n]) == 0 ) {
				m_ActFont = n;
				break;
			}
		}
		break; 
	}

	return ch;
}
int CGrapWnd::ParseSingel(LPCSTR &p, int cmd, int num)
{
	int ch = 0;

	switch(cmd) {
	case ('S' << 8) | 'H':
		// S(H) None Print complete screen
 		break;
	case ('S' << 8) | 'E':
		// S(E) None Screen erase (current background) Erases the screen and sets the screen to the current background intensity.
		m_TempDC.FillSolidRect(0, 0, m_MaxX, m_MaxY, m_BakCol);
		break;

		// W(V,R,C, or E) (V) Writing style Default style is
		//	(V) for overlay writing.
		//	(R) for replace writing
		//	(C) for complement writing
		//	(E) for erase writing
	case ('W' << 8) | 'V':
		m_Wopt.WriteStyle = WRST_OVERLAY;
		break;
	case ('W' << 8) | 'R':
		m_Wopt.WriteStyle = WRST_REPLACE;
		break;
	case ('W' << 8) | 'C':
		m_Wopt.WriteStyle = WRST_COMPLEMENT;
		break;
	case ('W' << 8) | 'E':
		m_Wopt.WriteStyle = WRST_ERASE;
		break;

	case ('P' << 8) | 'B':
		// P(B) None Begin a bounded position stack Pushes the current active position onto the stack. This position becomes the active position again after a corresponding (E) option in the stack.
		// no break;
	case ('V' << 8) | 'B':
		// V(B) Begin a bounded position stack Saves the current active position by pushing it on the stack. This is the starting point for a line.
		m_PosStack.AddHead(m_Pos);
		break;
	case ('P' << 8) | 'S':
		// P(S) None Start an unbounded position stack Pushes a dummy position onto the stack. When ReGIS reaches an (E) option in the stack, the active position stays at its current location.
	case ('V' << 8) | 'S':
		// V(S) Start an unbounded position stack Saves a dummy position, by pushing it onto the stack.
		{
			CPoint po(0xFFFFFFFF, 0xFFFFFFFF);
			m_PosStack.AddHead(po);
		}
		break; 
	case ('P' << 8) | 'E':
		// P(E) None End of bounded or unbounded position stack Selects the active position at the end of a position stack. The active position is based on the corresponding (B) option in a bounded stack or (S) option in an unbounded stack.
		if ( !m_PosStack.IsEmpty() ) {
			CPoint po = m_PosStack.RemoveHead();
			if ( po.x != 0xFFFFFFFF && po.y != 0xFFFFFFFF )
				m_Pos = po;
		}
		break;
	case ('V' << 8) | 'E':
		// V(E) End of bounded position stack Draws a line to the position saved by the previous (B) option from the position specified before the (E) option. Then pops the saved position off the stack.
		if ( !m_PosStack.IsEmpty() ) {
			CPoint po = m_PosStack.RemoveHead();
			if ( po.x != 0xFFFFFFFF && po.y != 0xFFFFFFFF )
				LineTo(po.x, po.y);
		}
		break;

	case ('C' << 8) | 'C':
		// C(C)[X,Y] None Circle with center at specified position [X,Y] defines the center of the circle. The current active position defines a point on the circumference.
		// C(A<degrees>C)[X,Y] 360 Arc with center at specified position [X,Y] defines the arc's center. The starting point for the arc is the current active position.
		//	<degrees> is a signed value the determines the size and direction of arc.
		//	+ = counterclockwise
		//	- = clockwise
		m_ArcFlag |= 001;
 		break;
 
		// C(B)<positions>(E) None Closed curve sequence Defines a closed curve based on [X,Y] positions specified in the sequence.
		// C(S)<positions>(E) None Open curve sequence Defines an open curve based on [X,Y] positions specified in the sequence.
	case ('C' << 8) | 'B':
		m_ArcPoint.RemoveAll();
		m_ArcPoint.Add(m_Pos);
		m_ArcFlag |= 010;
		break;
	case ('C' << 8) | 'S':
		m_ArcPoint.RemoveAll();
		m_ArcPoint.Add(m_Pos);
		m_ArcFlag |= 020;
		break;
	case ('C' << 8) | 'E':
		if ( m_ArcPoint.GetSize() > 1 )
			SmoothLine((m_ArcFlag & 010) != 0 ? TRUE : FALSE);
		m_ArcPoint.RemoveAll();
		m_ArcFlag &= ~070;
		break;

	case ('R' << 8) | 'P':
	case ('R' << 8) | 'L':
	case ('R' << 8) | 'E':
	case ('R' << 8) | 'I':
		// (P) None Cursor position Reports the current active position.
		// (L) None Character set Reports which character set (1 to 3) is selected for loading.
		// (E) None Error Reports the last error found by the parser.
		// (In) 0 Graphics input mode Selects one-shot mode (0) or multiple mode (1).
		break;

	case ('T' << 8) | 'B':
		memcpy(&m_SaveTopt, &m_Topt, sizeof(m_Topt));
		break;
	case ('T' << 8) | 'E':
		// T(B)<options>(E) Text controls in effect Temporary text control Selects temporary option values for one text command. Temporary values remain in effect until you use (E).
		memcpy(&m_Topt, &m_SaveTopt, sizeof(m_Topt));
		break; 
	}
	return ch;
}

void CGrapWnd::SetReGIS(int mode, LPCSTR p)
{
	CClientDC DispDC(this);

	if ( m_Type != TYPE_REGIS || (mode & 1) != 0 || m_Bitmap[0].GetSafeHandle() == NULL ) {

		m_MaxX = 800;
		m_MaxY = 480;
		m_AspX = ASP_DIV;
		m_AspY = ASP_DIV;
		m_Maps = 0;

		m_ActMap = 0;
		m_BakCol = RGB(255, 255, 255);
		InitColMap();

		memcpy(m_PtnMap, DefPtnTab, sizeof(m_PtnMap));

		m_Wopt.Pvalue = 1;
		m_Wopt.IntCol = m_ColMap[0];
		m_Wopt.BitPtn = m_PtnMap[1];
		m_Wopt.NegPtn = 0x00;
		m_Wopt.MulPtn = 2;
		m_Wopt.ShaCtl = FALSE;
		m_Wopt.ShaChar = 0;
		m_Wopt.ShaPos.x = 0;
		m_Wopt.ShaPos.y = 0;
		m_Wopt.LineWidth = 1;
		m_Wopt.HatchIdx  = 0;
		m_Wopt.WriteStyle = WRST_OVERLAY;

		m_LocPush = FALSE;

		m_UseXPos = FALSE;
		m_Pos.x = 0;
		m_Pos.y = 0;
		m_PosStack.RemoveAll();

		m_Topt.DispSize.x = 9; m_Topt.DispSize.y = 20;
		m_Topt.UnitSize.x = 8; m_Topt.UnitSize.y = 20;
		m_Topt.SpaceSize.x = 9; m_Topt.SpaceSize.y = 0;
		m_Topt.OffsetSize.x = 0; m_Topt.OffsetSize.y = 0;
		m_Topt.FontItalic = 0;
		m_Topt.FontAngle  = 0;
		m_Topt.FontSelect = 0;
		memcpy(&m_SaveTopt, &m_Topt, sizeof(m_Topt));

		m_ActFont = 0;
		memset(m_UserFont, 0, sizeof(m_UserFont));

		m_ArcFlag = 0;
		m_ArcDegs = 0;
		m_FilFlag = FALSE;

		m_Type = TYPE_REGIS;
		m_pActMap = NULL;

		if ( m_Bitmap[0].GetSafeHandle() != NULL )
			m_Bitmap[0].DeleteObject();
		if ( m_Bitmap[1].GetSafeHandle() != NULL )
			m_Bitmap[1].DeleteObject();
		if ( m_Bitmap[2].GetSafeHandle() != NULL )
			m_Bitmap[2].DeleteObject();

		m_TempDC.CreateCompatibleDC(&DispDC);
		m_Bitmap[m_Maps].CreateCompatibleBitmap(&DispDC, m_MaxX, m_MaxY);
		m_pOldMap = (CBitmap *)m_TempDC.SelectObject(&(m_Bitmap[m_Maps]));
		m_TempDC.FillSolidRect(0, 0, m_MaxX, m_MaxY, m_BakCol);

	} else {
		m_pActMap = NULL;
		m_TempDC.CreateCompatibleDC(&DispDC);
		m_pOldMap = (CBitmap *)m_TempDC.SelectObject(&(m_Bitmap[m_Maps]));
	}

	ParseCmds(p, 0, NULL, BasicCmd);

	m_TempDC.SelectObject(m_pOldMap);
	m_TempDC.DeleteDC();

	m_pActMap = &(m_Bitmap[m_Maps]);
	Invalidate(FALSE);
}

//////////////////////////////////////////////////////////////////////
// Sixel

void CGrapWnd::SixelMaxInit()
{
										//Pu=1 HLS				Pu=2 RGB				Pu=3 RGB 255
	static const DWORD DefMax[4][4] = { { 360, 100, 100, 100 }, { 100, 100, 100, 100 }, { 255, 255, 255, 255 } };
	int n, i;

	for ( n = 0 ; n < 3 ; n++ ) {
		for ( i = 0 ; i < 4 ; i++ ) {
			if ( m_SixelMaxTab[n][i] == 0 )
				m_SixelMaxTab[n][i] = DefMax[n][i];
		}
	}
}

void CGrapWnd::SixelStart(int aspect, int mode, int grid, COLORREF bc)
{
	CClientDC DispDC(this);

	m_SixelStat       = 0;
	m_SixelWidth      = 0;
	m_SixelHeight     = 0;
	m_SixelPointX     = 0;
	m_SixelPointY     = 0;
	m_SixelRepCount   = 1;
	m_SixelColorIndex = 0;
	m_SixelValue      = 0;
	m_SixelValueInit  = FALSE;

	m_SixelParam.RemoveAll();
	ZeroMemory(m_SixelMaxTab, sizeof(m_SixelMaxTab));
	SixelMaxInit();

	m_AspX = ASP_DIV;
	m_AspY = ASP_DIV;
	m_MaxX = 1024;
	m_MaxY = 1024;

	m_Type = TYPE_SIXEL;
	m_pActMap = NULL;
	m_Maps = 0;
	m_Crc = 0;
	m_TransIndex = (-1);
	m_bHaveAlpha = FALSE;
	m_pIndexMap = NULL;

	// DECTID < 5(VT220) なら背景色をデフォルト白色でパレットをモノクロに
	// mode == 1 なら背景色を補色に

	if ( m_pTextRam->m_TermId < VT_COLMAP_ID ) {
		if ( mode < 2 )
			bc = RGB(255, 255, 255);
	} else {
		if ( mode == 1 )
			bc ^= RGB(255, 255, 255);
	}

	m_BakCol = bc;
	InitColMap();

	// VT330/340
	// http://vt100.net/docs/vt3xx-gp/chapter14.html
	// 0,1=2:1 2=5:1 3,4=3:1 5,6=2:1 7,8,9=1:1 default=2:1 (V:H)

	// VT382
	// http://manx.classiccmp.org/mirror/vt100.net/dec/ek-vt382-rm-001.pdf
	// 0,7,8,9=1:1 1,5,6=2:1 2=5:1 3,4=3:1 default=1:1 (V:H)

	// xterm
	// 0,1=5:1 2=3:1 3-6=2:1 7-9=1:1 defult=2:1 (V:H)

	// Genicom GEK 00031B Printer
	// http://www.manualslib.com/manual/60628/Genicom-Gek-00031b.html?page=260#manual
	// Pn1	0=50:100 1=50:100 2=50:200 3=65:200 4=40:100 5=50:100 6=67:45 7=55:67 8=45:50 9=50:50 (Vdpi:Hdpi)
	// Pn3	1=240 2=240 3=240 4=180 5=140 6=120 7=120 8=90 9=90 10=70 11-19=60 (Hdpi)
	// 0,1=2:1 2=4:1 3=3:1 4=2.5:1 5=2:1 6=1.5:1 7=1.2:1 8=1.1:1 9=1:1 default=2:1 (V:H)

	// GENICOM Matrix Printer LA36
	// http://www.theprinterplace.com/printers/manuals/GEB-00010A.pdf
	// Pn1	0=72:144 1=72:144 2=72:360 3=72:180 4=72:180 5=72:144 6=72:144 7=72:144 8=72:144 9=72:72 (Vdpi:Hdpi)
	// Pn3	1=360 2=360 3=180 4=180 5=144 6=144 7=144 8=90 9=90 10-15=72 16-19=45 20=36 (Hdpi)
	// 0,1=2:1 2=5:1 3,4=2.5:1 5,6,7,8=2:1 9=1:1 default=2:1 (V:H)

	// DEC LN03
	// http://www.vt100.net/docs/0ln03-rm/
	// 0=67:133 1=67:133 2=74:333 3=74:222 4=133:167 5=73:133 6=74:111 7=73:95 8=75:83 9=74:74 (Vdpi:Hdpi)
	// 0,1=2:1 2=4.5:1 3=3:1 4=2.5:1 5=1.83:1 6=1.5:1 7=1.3:1 8=1.12:1 9=1:1 default=2:1 (V:H)

	// DEC LJ250
	// http://ftp.mirrorservice.org/sites/www.bitsavers.org/pdf/dec/printer/lj250/
	// Pn1	0=72:144 1=72:144 2=72:180 3=72:180 4=72:180 5=72:144 6=72:144 7=72:144 8=72:144 9=72:72
	// Pn3	1,2,3,4=180 5,6,7=144 8,9=90 10-19=72 20=36
	// 0,1=2:1 2,3,4=2.5:1 5,6,7,8=2:1 9=1:1 default=2:1 (V:H)

	// プリンタのSixel仕様から想定
	// grid		dpi=１グリッドあたりの解像度 1/dpi=グリッドサイズ(ピクセル)で計算すれば納得
	//			72:144(dpi) -> 0.0139:0.0069(inc) -> 2:1(aspect)

	// aspect	横の解像度を上げて比を変えている(Hdpi 20%=360dpi->100%=72dpi Vdpi=72dpi)
	// grid		縦の解像度を下げて比を変えない(Vdpi 20%=360dpi->100%=72dpi->200%=36dpi Hdpi=Vdpi:ascpet)

	// RLoginでの設定
	// 0,1=50% 2=20% 3=30% 4=40% 5=60% 6=70% 7=80% 8=90% 9=100% (H%) -> (V:H... V=100/H%)
	// 0,1=2:1 2=5:1 3=3.33:1 4=2.5:1 5=1.67:1 6=1.43:1 7=1.25:1 8=1.11:1 9=1:1 default=2:1 (V:H)

	switch(aspect) {
	case 0:  m_AspX = ASP_DIV *  50 / 100; break;
	case 1:  m_AspX = ASP_DIV *  50 / 100; break;
	case 2:  m_AspX = ASP_DIV *  20 / 100; break;
	case 3:  m_AspX = ASP_DIV *  30 / 100; break;
	case 4:  m_AspX = ASP_DIV *  40 / 100; break;
	case 5:  m_AspX = ASP_DIV *  60 / 100; break;
	case 6:  m_AspX = ASP_DIV *  70 / 100; break;
	case 7:  m_AspX = ASP_DIV *  80 / 100; break;
	case 8:  m_AspX = ASP_DIV *  90 / 100; break;
	case 9:  m_AspX = ASP_DIV * 100 / 100; break;
	default: m_AspX = ASP_DIV *  50 / 100; break;
	}
	if ( grid == 0 )
		m_AspY = ASP_DIV;
	else if ( grid <= 1000 ) {
		m_AspY = grid * ASP_DIV * 10 / m_AspX;
		m_AspX = grid * 10;
	}

	if ( m_Bitmap[0].GetSafeHandle() != NULL )
		m_Bitmap[0].DeleteObject();

	if ( m_Bitmap[1].GetSafeHandle() != NULL )
		m_Bitmap[1].DeleteObject();

	m_SixelBackColor = m_SixelTransColor = bc;

	m_SixelTempDC.CreateCompatibleDC(&DispDC);
	m_Bitmap[m_Maps].CreateCompatibleBitmap(&DispDC, m_MaxX, m_MaxY);
	m_SixelTempDC.SelectObject(&(m_Bitmap[m_Maps]));
	m_SixelTempDC.FillSolidRect(0, 0, m_MaxX, m_MaxY, m_SixelBackColor);

	m_pAlphaMap = new BYTE[m_MaxX * m_MaxY];
	ZeroMemory(m_pAlphaMap, m_MaxX * m_MaxY);

	if ( mode == 5 ) {	// sixel or mode
		m_pIndexMap = new WORD[m_MaxX * m_MaxY];
		ZeroMemory(m_pIndexMap, m_MaxX * m_MaxY * sizeof(WORD));
	}
}
void CGrapWnd::SixelResize()
{
	int n;
	CDC SecDC;
	CBitmap *pOldSecMap;
	BITMAP info;
	BYTE *pAlpha;
	WORD *pIndex;

	n = m_Maps ^ 1;

	m_Bitmap[n].CreateCompatibleBitmap(&m_SixelTempDC, m_MaxX, m_MaxY);

	SecDC.CreateCompatibleDC(&m_SixelTempDC);
	pOldSecMap = (CBitmap *)SecDC.SelectObject(&(m_Bitmap[n]));
	SecDC.FillSolidRect(0, 0, m_MaxX, m_MaxX, m_SixelBackColor);
	SecDC.BitBlt(0, 0, m_MaxX, m_MaxY, &m_SixelTempDC, 0, 0, SRCCOPY);
	SecDC.SelectObject(pOldSecMap);
	SecDC.DeleteDC();

	m_SixelTempDC.SelectObject(&(m_Bitmap[n]));
	ZeroMemory(&info, sizeof(info));
	m_Bitmap[m_Maps].GetBitmap(&info);
	m_Bitmap[m_Maps].DeleteObject();
	m_Maps = n;

	pAlpha = new BYTE[m_MaxX * m_MaxY];
	ZeroMemory(pAlpha, m_MaxX * m_MaxY);

	int mx = m_MaxX < info.bmWidth  ? m_MaxX : info.bmWidth;
	int my = m_MaxY < info.bmHeight ? m_MaxY : info.bmHeight;

	for ( int y = 0 ; y < my ; y++ )
		memcpy(pAlpha + m_MaxX * y, m_pAlphaMap + info.bmWidth * y, mx);

	delete [] m_pAlphaMap;
	m_pAlphaMap = pAlpha;

	if ( m_pIndexMap == NULL )
		return;

	pIndex = new WORD[m_MaxX * m_MaxY];
	ZeroMemory(pIndex, m_MaxX * m_MaxY * sizeof(WORD));

	for ( int y = 0 ; y < my ; y++ )
		memcpy(pIndex + m_MaxX * y, m_pIndexMap + info.bmWidth * y, mx * sizeof(WORD));

	delete [] m_pIndexMap;
	m_pIndexMap = pIndex;
}
void CGrapWnd::SixelData(int ch)
{
	int n, i, bit, mask, line;
	DWORD Px, Py, Pz, Pa;
	DWORD Mx, My, Mz, Ma;

	for ( ; ; ) {
		if ( m_SixelStat == 0 ) {
			if ( ch == '"' ) { 				// DECGRA Set Raster Attributes				"Pan;Pad;Ph;Pv 
				m_SixelStat = 1;

			} else if ( ch == '!' ) {		// DECGRI Graphics Repeat Introducer		!Pn
				m_SixelStat = 2;

			} else if ( ch == '#' ) {		// DECGCI Graphics Color Introducer			#Pc;Pu;Px;Py;Pz;Pa 
				m_SixelStat = 3;

			} else if ( ch == '*' ) {		// RLGCIMAX									*Pu;Px;Py;Pz;Pa
				m_SixelStat = 4;

			} else if ( ch == '$' ) {		// DECGCR Graphics Carriage Return			$
				m_SixelPointX   = 0;
				m_SixelRepCount = 1;

			} else if ( ch == '-' ) {		// DECGNL Graphics Next Line				-
				m_SixelPointX   = 0;
				m_SixelPointY  += 6;
				m_SixelRepCount = 1;

			} else if ( ch >= '?' && ch <= '\x7E' ) {

				if ( m_MaxX < (m_SixelPointX + m_SixelRepCount) || m_MaxY < (m_SixelPointY + 6) ) {
					while ( m_MaxX < (m_SixelPointX + m_SixelRepCount) )
						m_MaxX *= 2;
					while ( m_MaxY < (m_SixelPointY + 6) )
						m_MaxY *= 2;

					if ( m_MaxX > GRAPMAX_X )
						m_MaxX = GRAPMAX_X;
					if ( m_MaxY > GRAPMAX_Y )
						m_MaxY = GRAPMAX_Y;

					SixelResize();
				}

				if ( (bit = ch - '?') <= 0 ) {
					m_SixelPointX += m_SixelRepCount;

				} else if ( m_pIndexMap != NULL ) {	// sixel or mode
					for ( n = 0 ; n < m_SixelRepCount ; n++ ) {
						mask = 0x01;
						for ( i = 0 ; i < 6 ; i++ ) {
							if ( m_SixelPointX >= m_MaxX || (m_SixelPointY + i) >= m_MaxY )
								break;
							if ( (bit & mask) != 0 ) {
								m_pIndexMap[m_SixelPointX + (m_SixelPointY + i) * m_MaxX] |= m_SixelColorIndex;
								if ( m_SixelWidth <= m_SixelPointX )
									m_SixelWidth = m_SixelPointX + 1;
								if ( m_SixelHeight <= (m_SixelPointY + i) )
									m_SixelHeight = m_SixelPointY + i + 1;
							}
							mask <<= 1;
						}
						m_SixelPointX += 1;
					}

				} else if ( m_SixelPointX < GRAPMAX_X && m_SixelPointY < GRAPMAX_Y ) {
					if ( m_ColMap[m_SixelColorIndex] == m_SixelTransColor )
						m_SixelTransColor = (-1);

					if ( m_ColAlpha[m_SixelColorIndex] != RGBMAX )
						m_bHaveAlpha = TRUE;

					mask = 0x01;
					if ( m_SixelRepCount <= 1 ) {
						for ( i = 0 ; i < 6 ; i++ ) {
							if ( (bit & mask) != 0 ) {
								m_pAlphaMap[m_SixelPointX + m_MaxX * (m_SixelPointY + i)] = m_ColAlpha[m_SixelColorIndex];
								m_SixelTempDC.SetPixelV(m_SixelPointX, m_SixelPointY + i, m_ColMap[m_SixelColorIndex]);
								if ( m_SixelWidth <= m_SixelPointX )
									m_SixelWidth = m_SixelPointX + 1;
								if ( m_SixelHeight <= (m_SixelPointY + i) )
									m_SixelHeight = m_SixelPointY + i + 1;
							}
							mask <<= 1;
						}
						m_SixelPointX += 1;
					} else {
						for ( i = 0 ; i < 6 ; i++ ) {
							if ( (bit & mask) != 0 ) {
								memset(m_pAlphaMap + m_SixelPointX + m_MaxX * (m_SixelPointY + i), m_ColAlpha[m_SixelColorIndex], m_SixelRepCount);
								line = mask << 1;
								for ( n = 1 ; (i + n) < 6 ; n++ ) {
									if ( (bit & line) == 0 )
										break;
									line <<= 1;
									memset(m_pAlphaMap + m_SixelPointX + m_MaxX * (m_SixelPointY + i + n), m_ColAlpha[m_SixelColorIndex], m_SixelRepCount);
								}
								m_SixelTempDC.FillSolidRect(m_SixelPointX, m_SixelPointY + i, m_SixelRepCount, n, m_ColMap[m_SixelColorIndex]);

								if ( m_SixelWidth <= (m_SixelPointX + m_SixelRepCount - 1)  )
									m_SixelWidth = m_SixelPointX + m_SixelRepCount - 1 + 1;
								if ( m_SixelHeight <= (m_SixelPointY + i + n - 1) )
									m_SixelHeight = m_SixelPointY + i + n - 1 + 1;

								i += (n - 1);
								mask <<= (n - 1);
							}
							mask <<= 1;
						}
						m_SixelPointX += m_SixelRepCount;
					}
				}

				m_SixelRepCount = 1;
			}

		} else {
			if ( isdigit(ch) ) {
				if ( !m_SixelValueInit ) {
					m_SixelValueInit = TRUE;
					m_SixelValue = 0;
				}
				m_SixelValue = m_SixelValue * 10 + (ch - '0');

			} else if ( ch == ' ' || ch == '\t' ) {
				m_SixelValueInit = FALSE;

			} else if ( ch == ';' ) {
				m_SixelParam.Add(m_SixelValueInit ? m_SixelValue : 0);
				m_SixelValueInit = FALSE;

			} else {
				if ( m_SixelValueInit )
					m_SixelParam.Add(m_SixelValue);
				m_SixelValueInit = FALSE;

				switch(m_SixelStat) {
				case 1:		// DECGRA
					if ( m_SixelParam.GetSize() > 0 ) m_AspY = m_SixelParam[0] * ASP_DIV;
					if ( m_SixelParam.GetSize() > 1 ) m_AspX = m_SixelParam[1] * ASP_DIV;
					if ( m_SixelParam.GetSize() > 2 ) m_MaxX = m_SixelWidth  = m_SixelParam[2];
					if ( m_SixelParam.GetSize() > 3 ) m_MaxY = m_SixelHeight = m_SixelParam[3];

					if ( m_SixelWidth  <= 0 ) m_SixelWidth  = 1; else if ( m_SixelWidth  > GRAPMAX_X ) m_SixelWidth  = GRAPMAX_X;
					if ( m_SixelHeight <= 0 ) m_SixelHeight = 1; else if ( m_SixelHeight > GRAPMAX_Y ) m_SixelHeight = GRAPMAX_Y;

					if ( m_AspX <= 0 ) m_AspX = ASP_DIV; else if ( m_AspX > (ASP_DIV * 1000) ) m_AspX = ASP_DIV * 1000;
					if ( m_AspY <= 0 ) m_AspY = ASP_DIV; else if ( m_AspY > (ASP_DIV * 1000) ) m_AspY = ASP_DIV * 1000;
					if ( m_MaxX <= 0 ) m_MaxX = 1; else if ( m_MaxX > GRAPMAX_X ) m_MaxX = GRAPMAX_X;
					if ( m_MaxY <= 0 ) m_MaxY = 1; else if ( m_MaxY > GRAPMAX_Y ) m_MaxY = GRAPMAX_Y;

					SixelResize();
					break;

				case 2:		// DECGRI
					if ( m_SixelParam.GetSize() > 0 )
						m_SixelRepCount = m_SixelParam[0];
					if ( m_SixelRepCount > m_MaxX )
						m_SixelRepCount = m_MaxX;
					break;
				
				case 3:		// DECGCI
					if ( m_SixelParam.GetSize() > 0 )
						m_SixelColorIndex = m_SixelParam[0] % SIXEL_PALET;

					// Pu ? 1=HLS, 2=RGB, 3=R8G8B8
					if ( m_SixelParam.GetSize() < 5 || (n = m_SixelParam[1] - 1) < 0 || n > 2 )
						break;

					Mx = m_SixelMaxTab[n][0];
					My = m_SixelMaxTab[n][1];
					Mz = m_SixelMaxTab[n][2];
					Ma = m_SixelMaxTab[n][3];

					if ( (Px = m_SixelParam[2]) > Mx ) Px = Mx;
					if ( (Py = m_SixelParam[3]) > My ) Py = My;
					if ( (Pz = m_SixelParam[4]) > Mz ) Pz = Mz;
					if ( m_SixelParam.GetSize() <= 5 || (Pa = m_SixelParam[5]) > Ma ) Pa = Ma;	// Alpha Default Max

					if ( m_SixelParam[1] == 1 ) {			// HLS
						m_ColMap[m_SixelColorIndex] = HLStoRGB(
							(Px * HLSMAX + Mx / 2) / Mx,
							(Py * HLSMAX + My / 2) / My,
							(Pz * HLSMAX + Mz / 2) / Mz);

					} else {								// RGB / R8G8B8
						m_ColMap[m_SixelColorIndex] = RGB(
							(BYTE)((Px * RGBMAX + Mx / 2) / Mx),
							(BYTE)((Py * RGBMAX + My / 2) / My),
							(BYTE)((Pz * RGBMAX + Mz / 2) / Mz));
					}

					m_ColAlpha[m_SixelColorIndex] = (BYTE)((Pa * RGBMAX + Ma / 2) / Ma);
					break;

				case 4:		// RLGCIMAX
					if ( m_SixelParam.GetSize() < 1 || (n = m_SixelParam[0] - 1) < 0 || n > 2 )
						break;

					for ( i = 0 ; i < 4 ; i++ )
						m_SixelMaxTab[n][i] = ((i + 1) < m_SixelParam.GetSize() ? m_SixelParam[i + 1] : 0);

					// Max Value Check and Init
					SixelMaxInit();
					break;
				}

				m_SixelStat = 0;
				m_SixelParam.RemoveAll();

				continue;	// Loop !!!
			}
		}

		break;	// No Loop !!!
	}
}
void CGrapWnd::SixelEndof(BOOL bAlpha)
{
	if ( m_MaxX > m_SixelWidth || m_MaxY > m_SixelHeight ) {
		m_MaxX = m_SixelWidth;
		m_MaxY = m_SixelHeight;

		if ( m_MaxX > 0 && m_MaxY > 0 )
			SixelResize();
	}

	if ( m_pIndexMap != NULL ) {
		for ( int y = 0 ; y < m_MaxY ; y++ ) {
			for ( int x = 0 ; x < m_MaxX ; x++ ) {
				int i = m_pIndexMap[x + y * m_MaxX];

				if ( m_ColMap[i] == m_SixelTransColor )
					m_SixelTransColor = (-1);

				if ( m_ColAlpha[i] != RGBMAX )
					m_bHaveAlpha = TRUE;

				m_pAlphaMap[x + y * m_MaxX] = m_ColAlpha[i];
				m_SixelTempDC.SetPixelV(x, y, m_ColMap[i]);
			}
		}
		delete [] m_pIndexMap;
	}

	if ( *m_pAlphaMap == 0xFF && memcmp(m_pAlphaMap, m_pAlphaMap + 1, (size_t)(m_MaxX * m_MaxY - 1)) == 0 ) {
		// すべてのビットマップを描画
		m_SixelTransColor = (-1);
		m_bHaveAlpha = FALSE;

	} else if ( bAlpha || m_bHaveAlpha ) {
		// ビットマップをアルファ付きに変更
		CImage tmp;
		tmp.CreateEx(m_MaxX, m_MaxY, 32, BI_RGB, NULL, CImage::createAlphaChannel);
		::BitBlt(tmp.GetDC(), 0, 0, m_MaxX, m_MaxY, m_SixelTempDC, 0, 0, SRCCOPY);
		tmp.ReleaseDC();

		for ( int y = 0 ; y < m_MaxY ; y++ ) {
			for ( int x = 0 ; x < m_MaxX ; x++ ) {
				BYTE *p = (BYTE *)tmp.GetPixelAddress(x, y);
				if ( (p[3] = m_pAlphaMap[x + y * m_MaxX]) == 0x00 )
					p[0] = p[1] = p[2] = 0x00;
				else if ( p[3] < 255 ) {
					p[0] = p[0] * p[3] / 255;
					p[1] = p[1] * p[3] / 255;
					p[2] = p[2] * p[3] / 255;
				}
			}
		}

		m_Bitmap[m_Maps].DeleteObject();
		m_Bitmap[m_Maps].Attach(tmp);
		tmp.Detach();

		m_SixelTransColor = (-1);
		m_bHaveAlpha = TRUE;

	} else if ( m_SixelTransColor == (-1) ) {
		// m_SixelTransColorを使用して単色透明
		m_SixelTransColor = (m_SixelBackColor + 1) & 0x00FFFFFF;
		for ( int n = 0 ; n < SIXEL_PALET ; n++ ) {
			if ( m_ColMap[n] == m_SixelTransColor ) {
				// 使用していない色を探す（随時パレット変更されていると不十分の可能性あり）
				m_SixelTransColor = (m_SixelTransColor + 1) & 0x00FFFFFF;
				n = (-1);
			}
		}
		for ( int y = 0 ; y < m_MaxY ; y++ ) {
			for ( int x = 0 ; x < m_MaxX ; x++ ) {
				if ( m_pAlphaMap[x + y * m_MaxX] == 0x00 )
					m_SixelTempDC.SetPixelV(x, y, m_SixelTransColor);
			}
		}
	}

	m_SixelTempDC.DeleteDC();
	delete [] m_pAlphaMap;

	m_pActMap = &(m_Bitmap[m_Maps]);
	m_SixelBackColor = (-1);

	if ( m_hWnd != NULL )
		Invalidate(FALSE);

	SetMapCrc();
}
void CGrapWnd::SetSixel(int aspect, int mode, int grid, LPCSTR str, COLORREF bc)
{
	SixelStart(aspect, mode, grid, bc);

	while ( *str != '\0' )
		SixelData(*(str++));

	SixelEndof();
}

//////////////////////////////////////////////////////////////////////
// list func

void CGrapWnd::ListAdd(class CGrapWnd **pTop, int idx)
{
	m_pList[idx] = *pTop;
	*pTop = this;
}
void CGrapWnd::ListRemove(class CGrapWnd **pTop, int idx)
{
	CGrapWnd *pPos, *pBack;

	pPos = pBack = *pTop;

	while ( pPos != NULL ) {
		if ( pPos == this ) {
			if ( pPos == pBack )
				*pTop = pPos->m_pList[idx];
			else
				pBack->m_pList[idx] = pPos->m_pList[idx];
			break;
		}
		pBack = pPos;
		pPos = pPos->m_pList[idx];
	}
}
class CGrapWnd *CGrapWnd::ListFindImage(class CGrapWnd *pPos)
{
	while ( pPos != NULL ) {
		if ( Compare(pPos) == 0 )
			return pPos;
		pPos = pPos->m_pList[GRAPLIST_IMAGE];
	}
	return NULL;
}
class CGrapWnd *CGrapWnd::ListFindIndex(class CGrapWnd *pPos, int index)
{
	while ( pPos != NULL ) {
		if ( pPos->m_ImageIndex == index )
			return pPos;
		pPos = pPos->m_pList[GRAPLIST_INDEX];
	}
	return NULL;
}
class CGrapWnd *CGrapWnd::ListFindType(class CGrapWnd *pPos, int type)
{
	while ( pPos != NULL ) {
		if ( pPos->m_Type == type )
			return pPos;
		pPos = pPos->m_pList[GRAPLIST_TYPE];
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////
// CGifAnime

CGifAnime::CGifAnime(void)
{
	m_Delay = 300;	// ms
	m_TransIndex = (-1);
	m_bHaveAlpha = FALSE;
	m_Motd = 0;
}
CGifAnime::~CGifAnime()
{
}
const CGifAnime & CGifAnime::operator = (CGifAnime &data)
{
	m_Delay = data.m_Delay;
	m_TransIndex = data.m_TransIndex;
	m_bHaveAlpha = data.m_bHaveAlpha;
	m_Motd = data.m_Motd;

	if ( m_hObject != NULL )
		DeleteObject();

	if ( data.m_hObject != NULL ) {
		Attach(data.m_hObject);
		data.Detach();
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////

BOOL CGrapWnd::LoadMemory(LPBYTE lpBuf, int len, CImage &image)
{
	HGLOBAL hGLB = NULL;
	LPSTREAM pStm = NULL;
	LPBYTE pPtr;
	BOOL ret = FALSE;

	m_SixelBackColor  = (-1);
	m_SixelTransColor = (-1);

	if ( (hGLB = ::GlobalAlloc(GMEM_MOVEABLE, (DWORD)len)) == NULL )
		goto ENDOF;

	if ( (pPtr = (LPBYTE)::GlobalLock(hGLB)) == NULL )
		goto ENDOF;
	
	memcpy(pPtr, lpBuf, len);

	::GlobalUnlock(hGLB);

	if ( ::CreateStreamOnHGlobal(hGLB, TRUE, &pStm) != S_OK )
		goto ENDOF;

	if ( SUCCEEDED(image.Load(pStm)) )
		ret = TRUE;

ENDOF:
	if ( pStm != NULL )
		pStm->Release();

	if ( hGLB != NULL )
		GlobalFree(hGLB);

	return ret;
}
void CGrapWnd::SetImage(CImage &image, BOOL bCheckAlpha)
{
	m_AspX = ASP_DIV;
	m_AspY = ASP_DIV;
	m_MaxX = image.GetWidth();
	m_MaxY = image.GetHeight();

	m_Type = TYPE_SIXEL;
	m_pActMap = NULL;
	m_Maps = 0;
	m_Crc = 0;

	if ( m_Bitmap[0].GetSafeHandle() != NULL )
		m_Bitmap[0].DeleteObject();

	if ( m_Bitmap[1].GetSafeHandle() != NULL )
		m_Bitmap[1].DeleteObject();

	m_bHaveAlpha = FALSE;

	// GIFの透過色パレットだから1,2,4,8のはず・・・
	if ( m_TransIndex != (-1) && image.GetBPP() != 1 && image.GetBPP() != 2 && image.GetBPP() != 4 && image.GetBPP() != 8 )
		m_TransIndex = (-1);

	if ( bCheckAlpha && image.GetBPP() == 32 ) {
		m_bHaveAlpha = TRUE;
		// alpha値により色度を下げる
		// AlphaBlendのAC_SRC_OVERでは
		// Dst = Src + Dst * (1 - Src.Alpha)
		// で・・・
		// Dst = Src * Src.Alpha + Dst * (1 - Src.Alha)
		// になるようにSrcを先に計算
		for ( int y = 0 ; y < image.GetHeight() ; y++ ) {
			for ( int x = 0 ;  x < image.GetWidth() ; x++ ) {
				LPBYTE p = (BYTE *)image.GetPixelAddress(x, y);
				if ( p[3] == 0x00 )
					p[0] = p[1] = p[2] = 0x00;
				else if ( p[3] < 255 ) {
					p[0] = p[0] * p[3] / 255;
					p[1] = p[1] * p[3] / 255;
					p[2] = p[2] * p[3] / 255;
				}
			}
		}
	}

	m_pActMap = &(m_Bitmap[m_Maps]);
	m_pActMap->Attach((HBITMAP)image);
	image.Detach();

	SetMapCrc();
}
void CGrapWnd::AddGifAnime(CImage &image, int width, int height, CRect &rect, int bkidx, COLORREF bkcol, int delay, int motd, int trsidx)
{
	int n;
	int idx;
	CGifAnime anime;
	CImage tmp, work;

	if ( width == image.GetWidth() && height == image.GetHeight() && trsidx == (-1) && image.GetBPP() != 32 ) {
		// imageをanimeに移動
		anime.Attach((HBITMAP)image);
		image.Detach();

		anime.m_Delay = delay * CLOCKS_PER_SEC / 1000;
		anime.m_TransIndex = (-1);
		anime.m_bHaveAlpha = FALSE;
		anime.m_Motd = motd;
		m_GifAnime.Add(anime);

		return;
	}

	tmp.CreateEx(width, height, 32, BI_RGB, NULL, CImage::createAlphaChannel);

	if ( (n = (int)m_GifAnime.GetSize() - 1) >= 0 && motd != 2 ) {
		// 前画像を背景に
		work.Attach((HBITMAP)m_GifAnime[n]);
		work.BitBlt(tmp.GetDC(), 0, 0);
		tmp.ReleaseDC();
		work.Detach();

	} else {
		if ( bkidx != (-1) && bkidx == trsidx ) {
			// 背景を透過
			for ( int y = 0 ; y < tmp.GetHeight() ; y++ ) {
				for ( int x = 0 ;  x < tmp.GetWidth() ; x++ ) {
					LPBYTE p = (BYTE *)tmp.GetPixelAddress(x, y);
					p[0] = p[1] = p[2] = p[3] = 0x00;
				}
			}
		} else {
			// 背景を塗りつぶし
			CDC *pDC = CDC::FromHandle(tmp.GetDC());
			pDC->FillSolidRect(0, 0, width, height, bkcol);
			tmp.ReleaseDC();
		}
	}

	if ( image.GetBPP() == 32 ) {
		// アルファを適用
		for ( int y = 0 ; y < image.GetHeight() ; y++ ) {
			for ( int x = 0 ;  x < image.GetWidth() ; x++ ) {
				LPBYTE p = (BYTE *)image.GetPixelAddress(x, y);
				if ( p[3] == 0x00 )
					p[0] = p[1] = p[2] = 0x00;
				else if ( p[3] < 255 ) {
					p[0] = p[0] * p[3] / 255;
					p[1] = p[1] * p[3] / 255;
					p[2] = p[2] * p[3] / 255;
				}
			}
		}

		BLENDFUNCTION bf;
		ZeroMemory(&bf, sizeof(bf));
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 0xFF;
		bf.AlphaFormat = AC_SRC_ALPHA;

		::AlphaBlend(tmp.GetDC(), rect.left, rect.top, rect.Width(), rect.Height(), image.GetDC(), 0, 0, image.GetWidth(), image.GetHeight(), bf);
		image.ReleaseDC();
		tmp.ReleaseDC();

	} else if ( trsidx != (-1) && image.GetBPP() <= 8 ) {
		// 透過色以外を描画
		n = image.GetMaxColorTableEntries();
		RGBQUAD *paltab = new RGBQUAD[n];
		image.GetColorTable(0, n, paltab);
		while ( n-- > 0 )
			paltab[n].rgbReserved = 255;

		switch(image.GetBPP()) {
		case 1:
			for ( int y = 0 ; y < rect.Height() ; y++ ) {
				for ( int x = 0 ; x < rect.Width() ; x++ ) {
					BYTE *s = (BYTE *)image.GetPixelAddress(x, y);
					if ( (idx = *s & (1 << (x % 8))) != trsidx ) {
						RGBQUAD *p = (RGBQUAD *)tmp.GetPixelAddress(rect.left + x, rect.top + y);
						*p = paltab[idx];
					}
				}
			}
			break;
		case 2:
			for ( int y = 0 ; y < rect.Height() ; y++ ) {
				for ( int x = 0 ; x < rect.Width() ; x++ ) {
					BYTE *s = (BYTE *)image.GetPixelAddress(x, y);
					if ( (idx = *s & (3 << (x % 4))) != trsidx ) {
						RGBQUAD *p = (RGBQUAD *)tmp.GetPixelAddress(rect.left + x, rect.top + y);
						*p = paltab[idx];
					}
				}
			}
			break;
		case 4:
			for ( int y = 0 ; y < rect.Height() ; y++ ) {
				for ( int x = 0 ; x < rect.Width() ; x++ ) {
					BYTE *s = (BYTE *)image.GetPixelAddress(x, y);
					if ( (idx = *s & (15 << (x % 2))) != trsidx ) {
						RGBQUAD *p = (RGBQUAD *)tmp.GetPixelAddress(rect.left + x, rect.top + y);
						*p = paltab[idx];
					}
				}
			}
			break;
		case 8:
			for ( int y = 0 ; y < rect.Height() ; y++ ) {
				for ( int x = 0 ; x < rect.Width() ; x++ ) {
					BYTE *s = (BYTE *)image.GetPixelAddress(x, y);
					if ( (idx = *s) != trsidx ) {
						RGBQUAD *p = (RGBQUAD *)tmp.GetPixelAddress(rect.left + x, rect.top + y);
						*p = paltab[idx];
					}
				}
			}
			break;
		}

		delete [] paltab;

	} else {
		// 画像をコピー
		::BitBlt(tmp.GetDC(), rect.left, rect.top, rect.Width(), rect.Height(), image.GetDC(), 0, 0, SRCCOPY);
		image.ReleaseDC();
		tmp.ReleaseDC();
	}

	// tmpをanimeに移動、imageは削除
	anime.Attach((HBITMAP)tmp);
	tmp.Detach();

	anime.m_Delay = delay * CLOCKS_PER_SEC / 1000;
	anime.m_TransIndex = (-1);
	anime.m_bHaveAlpha = TRUE;
	anime.m_Motd = motd;
	m_GifAnime.Add(anime);

	image.Destroy();
}
BOOL CGrapWnd::LoadGifAnime(LPBYTE lpBuf, int len)
{
#pragma pack(push, 1)
	struct _GifHead {
		BYTE	tag[6];	// GIF89a
		WORD	width;
		WORD	height;
			BYTE	SGCT:3;
			BYTE	SF:1;
			BYTE	CR:3;
			BYTE	GCTF:1;
		BYTE	bkindex;
		BYTE	Aspect;
		// BYTE		palet[3][GCTF == 1 ? (1 << (SGCT + 1)) : 0];
	} *pGifHead, *pTmpHead;

	struct _GifCtrl {
		BYTE	id;		// 0x21
		BYTE	label;	// 0xf9
		BYTE	size;	// 0x04
			BYTE	trflag:1;
			BYTE	user:1;
			BYTE	motd:3;		// 1=Merge, 2=BackColor, 3=Restore
			BYTE	resv:3;
		WORD	delay;	// x10ms
		BYTE	tridx;
		BYTE	term;	// 0x00
	} *pGifCtrl, *pActCtrl = NULL;

	struct _GifImage {
		BYTE	id;		// 0x2c
		WORD	left;
		WORD	top;
		WORD	width;
		WORD	height;
			BYTE	SLCT:3;
			BYTE	R:2;
			BYTE	SF:1;
			BYTE	IF:1;
			BYTE	LCTF:1;
		// BYTE		palet[3][LCTF == 1 ? (1 << (SLCT + 1)) : 0];
		// BYTE		lzw;

		// BYTE		size;
		// BYTE		data[size];
		// ........
		// BYTE		size == 0
	} *pGifImage, *pTmpImage;
#pragma pack(pop)

	int n;
	int head;
	int pos = 0;
	CImage image;
	CBuffer work;
	CRect rect;
	int bkidx = (-1);
	COLORREF bkcol = RGB(0, 0, 0);
	int width, height;

	m_GifAnimePos = 0;
	m_GifAnime.RemoveAll();

	pGifHead = (struct _GifHead *)(lpBuf + pos);
	if ( (pos += sizeof(struct _GifHead)) > len )
		return FALSE;

	if ( memcmp(pGifHead->tag, "GIF89a", 6) != 0 && memcmp(pGifHead->tag, "GIF87a", 6) != 0 )
		return FALSE;

	if ( pGifHead->GCTF != 0 ) {
		n = 1 << (pGifHead->SGCT + 1);
		if ( pGifHead->bkindex < n ) {
			bkidx = pGifHead->bkindex;
			bkcol = RGB(lpBuf[pos + pGifHead->bkindex * 3], lpBuf[pos + pGifHead->bkindex * 3 + 1], lpBuf[pos + pGifHead->bkindex * 3 + 2]);
		}
		if ( (pos += (n * 3)) > len )
			return FALSE;
	}

	head   = pos;
	width  = pGifHead->width;
	height = pGifHead->height;

	while ( pos < len ) {
		if ( lpBuf[pos] == 0x2c ) {			// Image Block

			if ( (pos + 10) > len )
				return FALSE;

			pGifImage = (struct _GifImage *)(lpBuf + pos);

			// CImageでは、gifのイメージデータのオフセット、サイズがヘッダと違う場合に
			// 正常に背景色・透過色が読めないようだ・・・・？

			// 今回の画像サイズを保存
			rect.left   = pGifImage->left;
			rect.top    = pGifImage->top;
			rect.right  = pGifImage->left + pGifImage->width;
			rect.bottom = pGifImage->top  + pGifImage->height;

			// GIFのメモリイメージを作成
			work.Clear();				// New Image
			work.Apend(lpBuf, head);	// Gig Header Copy

			// GIF画像サイズ再設定
			pTmpHead = (struct _GifHead *)work.GetPos(0);
			pTmpHead->width  = pGifImage->width;
			pTmpHead->height = pGifImage->height;

			if ( pActCtrl != NULL ) {	// Ctrl Extension
				work.Apend((BYTE *)pActCtrl, 3 + pActCtrl->size);
				work.PutByte(0);			// Term
			}

			n = work.GetSize();
			work.Apend(lpBuf + pos, 10);	// Image Block
			pos += 10;

			// imageオフセットクリア
			pTmpImage = (struct _GifImage *)work.GetPos(n);
			pTmpImage->left = 0;
			pTmpImage->top  = 0;
			
			if ( pGifImage->LCTF != 0 ) {	// Local Palet
				n = (1 << (pGifImage->SLCT + 1)) * 3;
				if ( (pos + n) > len )
					return FALSE;
				work.Apend(lpBuf + pos, n);
				pos += n;
			}

			if ( (pos + 2) > len )
				return FALSE;

			work.PutByte(lpBuf[pos++]);		// lzw

			while ( pos < len && (n = lpBuf[pos++]) > 0 ) {
				work.PutByte(n);				// size
				work.Apend(lpBuf + pos, n);		// data
				pos += n;
			}
			work.PutByte(0);		// lbt

			work.PutByte(0x3b);		// Trailer

			// CImageで画像読み取り
			if ( !LoadMemory(work.GetPtr(), work.GetSize(), image) )
				return FALSE;

			// アニメに保存
			AddGifAnime(image, width, height, rect, bkidx, bkcol,
				(pActCtrl != NULL ? (pActCtrl->delay * 10) : 300),
				(pActCtrl != NULL ?  pActCtrl->motd : 0),
				(pActCtrl != NULL && pActCtrl->trflag != 0 ? pActCtrl->tridx : (-1)));

		} else if ( lpBuf[pos] == 0x21 && (pos + 3) < len ) {	// Extension

			pGifCtrl = (struct _GifCtrl *)(lpBuf + pos);

			if ( pGifCtrl->label == 0xf9 )		// Ctrl Extension
				pActCtrl = pGifCtrl;

			// Next Block
			pos += 3;	// id + label + size
			pos += pGifCtrl->size;

			// term != 0
			while ( pos < len && (n = lpBuf[pos++]) > 0 )
				pos += n;

		} else if ( lpBuf[pos] == 0x3b ) {	// Trailer
			break;

		} else								// Unkown ?
			return FALSE;
	}

	if ( m_GifAnime.GetSize() == 0 )
		return FALSE;

	m_GifAnimePos = 0;
	m_GifAnimeClock = clock() + m_GifAnime[m_GifAnimePos].m_Delay;

	image.Attach((HBITMAP)m_GifAnime[m_GifAnimePos]);
	SetImage(image, FALSE);

	m_TransIndex = m_GifAnime[m_GifAnimePos].m_TransIndex;
	m_bHaveAlpha = m_GifAnime[m_GifAnimePos].m_bHaveAlpha;

	return TRUE;
}
BOOL CGrapWnd::LoadPicture(LPBYTE lpBuf, int len)
{
	CImage image;

	if ( LoadGifAnime(lpBuf, len) )
		return TRUE;

	if ( !LoadMemory(lpBuf, len, image) )
		return FALSE;

	// m_TransIndex = CBmpFile::GifTrnsIndex(lpBuf, len);

	SetImage(image, TRUE);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////

CHistogram::CHistogram()
{
	m_MaxValue = 0;
	m_pGrapWnd = NULL;
	m_TheadFlag = 0;
}
CHistogram::~CHistogram()
{
	if ( m_pGrapWnd != NULL )
		m_pGrapWnd->m_pHistogram = NULL;

	if ( m_TheadFlag != 0 ) {
		m_TheadFlag = 2;
		while ( m_TheadFlag != 0 )
			Sleep(100);
	}
}

IMPLEMENT_DYNAMIC(CHistogram, CFrameWndExt)

BEGIN_MESSAGE_MAP(CHistogram, CFrameWndExt)
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()

int CHistogram::CalcMaxValue()
{
	int n, i;
	int max = 0;

	for ( n = 0 ; n < 3 ; n++ ) {
		for ( i = 0 ; i < 256 ; i++ ) {
			if ( max < m_Histgram[n][i] )
				max = m_Histgram[n][i];
		}
	}

	return max;
}
void CHistogram::CalcHistogram()
{
	int x, y;
	BYTE *p;
	COLORREF rgb;

	ZeroMemory(m_Histgram, sizeof(int) * 3 * 256);

	if ( m_Image.IsDIBSection() ) {
		for ( y = 0 ; y < m_Image.GetHeight() ; y++ ) {
			for ( x = 0 ; x < m_Image.GetWidth() ; x++ ) {
				p = (BYTE *)m_Image.GetPixelAddress(x, y);
				m_Histgram[0][p[2]]++;
				m_Histgram[1][p[1]]++;
				m_Histgram[2][p[0]]++;
			}
			if ( m_TheadFlag != 1 )
				break;
		}

	} else {
		for ( y = 0 ; y < m_Image.GetHeight() ; y++ ) {
			for ( x = 0 ; x < m_Image.GetWidth() ; x++ ) {
				rgb = m_Image.GetPixel(x, y);
				m_Histgram[0][GetRValue(rgb)]++;
				m_Histgram[1][GetGValue(rgb)]++;
				m_Histgram[2][GetBValue(rgb)]++;
			}
			if ( m_TheadFlag != 1 )
				break;
		}
	}

	m_Image.Destroy();
	m_MaxValue = CalcMaxValue();
	m_TheadFlag = 0;
}
static UINT CountRGBThread(LPVOID pParam)
{
	CHistogram *pWnd = (CHistogram *)pParam;
	pWnd->CalcHistogram();
	return 0;
}
void CHistogram::SetBitmap(HBITMAP hBitmap)
{
	CImage SrcImage;

	SrcImage.Attach(hBitmap);

	if ( !m_Image.CreateEx(SrcImage.GetWidth(), SrcImage.GetHeight(), SrcImage.GetBPP(), BI_RGB, NULL, 0) ) {
		SrcImage.Detach();
		return;
	}

	SrcImage.BitBlt(m_Image.GetDC(), 0, 0);
	m_Image.ReleaseDC();
	SrcImage.Detach();

	m_TheadFlag = 1;
	AfxBeginThread(CountRGBThread, this, THREAD_PRIORITY_NORMAL);

	SetTimer(1024, 300, NULL);
}

void CHistogram::OnPaint()
{
	int n, i, v, max;
	CRect frame, box;
	int val[3], pos[3];
	COLORREF col;
	CPaintDC dc(this);
	static const COLORREF ctb[3] = { RGB(255, 64, 64), RGB(64, 255, 64), RGB(64, 64, 255) };

	GetClientRect(frame);

	dc.FillSolidRect(frame, RGB(32, 32, 32));

	if ( (max = m_MaxValue) == 0 )
		max = CalcMaxValue();

	for ( n = 0 ; n < 256 ; n++ ) {
		pos[0] = 0; val[0] = m_Histgram[0][n] * frame.Height() / max;
		pos[1] = 1; val[1] = m_Histgram[1][n] * frame.Height() / max;
		pos[2] = 2; val[2] = m_Histgram[2][n] * frame.Height() / max;

#define	Swap(a,b)	{ v = pos[a]; pos[a] = pos[b]; pos[b] = v; }

		if ( val[pos[0]] < val[pos[1]] )
			Swap(0, 1);
		if ( val[pos[0]] < val[pos[2]] )
			Swap(0, 2);
		if ( val[pos[1]] < val[pos[2]] )
			Swap(1, 2);

		col = 0;

		for ( i = 0 ; i < 3 ; i++ ) {
			box.left   = frame.left + n * frame.Width() / 256;
			box.right  = frame.left + (n + 1) * frame.Width() / 256;
			box.top    = frame.bottom - val[pos[i]];
			box.bottom = frame.bottom;

			col |= ctb[pos[i]];
			dc.FillSolidRect(box, col);
		}
	}
}

BOOL CHistogram::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.cx = 500;
	cs.cy = 300;

	return CFrameWndExt::PreCreateWindow(cs);
}

void CHistogram::OnTimer(UINT_PTR nIDEvent)
{
	if ( m_TheadFlag == 0 )
		KillTimer(nIDEvent);

	Invalidate(FALSE);

	CFrameWndExt::OnTimer(nIDEvent);
}
