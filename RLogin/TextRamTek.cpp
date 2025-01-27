// TextRam.cpp: CTextRam クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdarg.h>
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "PipeSock.h"

#include <iconv.h>
#include <imm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Tek Func

#define	TEK_FONT_WIDTH		TekFonts[m_Tek_Font & 7][0]
#define	TEK_FONT_HEIGHT		TekFonts[m_Tek_Font & 7][1]

const int TekFonts[8][2] = {
//		Width	Height
	{	55,		88,		},   	// 74 cols 35 lines
	{	51,		81,		},   	// 81 cols 38 lines
    {	34,		53,		},  	// 121 cols 58 lines
	{	31,		48,		},  	// 133 cols 64 lines

	{	46,		71,		},		// 90 cols 43 lines
	{	43,		68,		},		// 96 cols 46 lines
	{	41,		64,		},		// 100 cols 48 lines
	{	37,		59,		},		// 110 cols 52 lines
};
const DWORD TekPenExtTab[8][4]  = {	{ 8, 0, 8, 0 },	{ 1, 1, 1, 1 },	{ 1, 1, 3, 1 },	{ 2, 1, 2, 1 }, { 2, 2, 2, 2 },	{ 3, 1, 2, 1 },	{ 4, 2, 1, 2 },	{ 3, 2, 3, 2 }	};
const int TekPenTypeTab[]       = { PS_SOLID,		PS_DOT,			PS_USERSTYLE,	PS_USERSTYLE,	PS_DASH,		PS_DASHDOTDOT,	PS_DASHDOT,		PS_USERSTYLE };
const int TekPenWidthTab[]      = { 1, 2  };
const COLORREF TekColorTab[]	= {	RGB(128, 128, 128), RGB(  0,   0,   0), RGB(192, 0, 0), RGB(0, 192, 0), RGB(0, 0, 192), RGB(0, 192, 192), RGB(192, 0, 192), RGB(192, 192, 0),
									RGB(192, 192, 192), RGB( 64,  64,  64), RGB( 96, 0, 0), RGB(0,  96, 0), RGB(0, 0,  96), RGB(0,  96,  96), RGB( 96, 0,  96), RGB( 96,  96, 0) };

#define	SINDIV	8192
static const int SinTab[] = {
	8192,	8191,	8187,	8181,	8172,	8161,	8147,	8131,	8112,	8091,
	8068,	8041,	8013,	7982,	7949,	7913,	7875,	7834,	7791,	7746,
	7698,	7648,	7595,	7541,	7484,	7424,	7363,	7299,	7233,	7165,
	7094,	7022,	6947,	6870,	6791,	6710,	6627,	6542,	6455,	6366,
	6275,	6183,	6088,	5991,	5893,	5793,	5691,	5587,	5482,	5374,
	5266,	5155,	5043,	4930,	4815,	4699,	4581,	4462,	4341,	4219,
	4096,	3972,	3846,	3719,	3591,	3462,	3332,	3201,	3069,	2936,
	2802,	2667,	2531,	2395,	2258,	2120,	1982,	1843,	1703,	1563,
	1423,	1282,	1140,	998,	856,	714,	571,	429,	286,	143,
		0,	-143,	-286,	-429,	-571,	-714,	-856,	-998,	-1140,	-1282,
	-1423,	-1563,	-1703,	-1843,	-1982,	-2120,	-2258,	-2395,	-2531,	-2667,
	-2802,	-2936,	-3069,	-3201,	-3332,	-3462,	-3591,	-3719,	-3846,	-3972,
	-4096,	-4219,	-4341,	-4462,	-4581,	-4699,	-4815,	-4930,	-5043,	-5155,
	-5266,	-5374,	-5482,	-5587,	-5691,	-5793,	-5893,	-5991,	-6088,	-6183,
	-6275,	-6366,	-6455,	-6542,	-6627,	-6710,	-6791,	-6870,	-6947,	-7022,
	-7094,	-7165,	-7233,	-7299,	-7363,	-7424,	-7484,	-7541,	-7595,	-7648,
	-7698,	-7746,	-7791,	-7834,	-7875,	-7913,	-7949,	-7982,	-8013,	-8041,
	-8068,	-8091,	-8112,	-8131,	-8147,	-8161,	-8172,	-8181,	-8187,	-8191,
	-8192,	-8191,	-8187,	-8181,	-8172,	-8161,	-8147,	-8131,	-8112,	-8091,
	-8068,	-8041,	-8013,	-7982,	-7949,	-7913,	-7875,	-7834,	-7791,	-7746,
	-7698,	-7648,	-7595,	-7541,	-7484,	-7424,	-7363,	-7299,	-7233,	-7165,
	-7094,	-7022,	-6947,	-6870,	-6791,	-6710,	-6627,	-6542,	-6455,	-6366,
	-6275,	-6183,	-6088,	-5991,	-5893,	-5793,	-5691,	-5587,	-5482,	-5374,
	-5266,	-5155,	-5043,	-4930,	-4815,	-4699,	-4581,	-4462,	-4341,	-4219,
	-4096,	-3972,	-3846,	-3719,	-3591,	-3462,	-3332,	-3201,	-3069,	-2936,
	-2802,	-2667,	-2531,	-2395,	-2258,	-2120,	-1982,	-1843,	-1703,	-1563,
	-1423,	-1282,	-1140,	-998,	-856,	-714,	-571,	-429,	-286,	-143,
		0,	143,	286,	429,	571,	714,	856,	998,	1140,	1282,
	1423,	1563,	1703,	1843,	1982,	2120,	2258,	2395,	2531,	2667,
	2802,	2936,	3069,	3201,	3332,	3462,	3591,	3719,	3846,	3972,
	4096,	4219,	4341,	4462,	4581,	4699,	4815,	4930,	5043,	5155,
	5266,	5374,	5482,	5587,	5691,	5793,	5893,	5991,	6088,	6183,
	6275,	6366,	6455,	6542,	6627,	6710,	6791,	6870,	6947,	7022,
	7094,	7165,	7233,	7299,	7363,	7424,	7484,	7541,	7595,	7648,
	7698,	7746,	7791,	7834,	7875,	7913,	7949,	7982,	8013,	8041,
	8068,	8091,	8112,	8131,	8147,	8161,	8172,	8181,	8187,	8191,
	8192,	8191,	8187,	8181,	8172,	8161,	8147,	8131,	8112,	8091,
	8068,	8041,	8013,	7982,	7949,	7913,	7875,	7834,	7791,	7746,
	7698,	7648,	7595,	7541,	7484,	7424,	7363,	7299,	7233,	7165,
	7094,	7022,	6947,	6870,	6791,	6710,	6627,	6542,	6455,	6366,
	6275,	6183,	6088,	5991,	5893,	5793,	5691,	5587,	5482,	5374,
	5266,	5155,	5043,	4930,	4815,	4699,	4581,	4462,	4341,	4219,
	4096,	3972,	3846,	3719,	3591,	3462,	3332,	3201,	3069,	2936,
	2802,	2667,	2531,	2395,	2258,	2120,	1982,	1843,	1703,	1563,
	1423,	1282,	1140,	998,	856,	714,	571,	429,	286,	143,
};
static const char MarkTab[11][38] = {
	{ 10,0,7,-7,0,-10,-7,-7,-10,0,-7,7,0,10,7,7,10,0,126 },
	{ 0,14,0,-14,127,-14,0,14,0,126 },
	{ 0,26,0,-26,127,-26,0,26,0,126 },
	{ 0,26,0,-26,127,-26,0,26,0,127,18,18,-18,-18,127,-18,18,18,-18,126 },
	{ 0,25,10,23,18,18,23,10,25,0,23,-10,18,-18,10,-23,0,-25,-10,-23,-18,-18,-23,-10,-25,0,-23,10,-18,18,-10,23,0,25,126 },
	{ 24,24,-24,-24,127,-24,24,24,-24,126 },
	{ 25,25,25,-25,-25,-25,-25,25,25,25,126 },
	{ 0,30,30,0,0,-30,-30,0,0,30,126 },
	{ 25,25,25,-25,-25,-25,-25,25,25,25,127,0,10,0,-10,127,-10,0,10,0,126 },
	{ 0,30,30,0,0,-30,-30,0,0,30,127,15,15,-15,-15,127,15,-15,-15,15,126 },
	{ 25,25,25,-25,-25,-25,-25,25,25,25,127,25,25,-25,-25,127,25,-25,-25,25,126 },
};

void CTextRam::TekInit(int ver)
{
	CString tmp;

	if ( m_pTekWnd != NULL ) {
		if ( m_Tek_Ver == ver || ver == 0 ) {
			if ( !m_pTekWnd->IsWindowVisible() )
				m_pTekWnd->ShowWindow(SW_SHOW);
			return;
		}
		m_Tek_Ver = ver;
		tmp.Format(_T("Tek%dxx - %s"), m_Tek_Ver / 100, (LPCTSTR)m_pDocument->m_ServerEntry.m_EntryName);
		m_pTekWnd->SetWindowText(tmp);
	} else {
		if ( ver != 0 )
			m_Tek_Ver = ver;
		tmp.Format(_T("Tek%dxx - %s"), m_Tek_Ver / 100, (LPCTSTR)m_pDocument->m_ServerEntry.m_EntryName);
		m_pTekWnd = new CTekWnd(this, ::AfxGetMainWnd());
		m_pTekWnd->Create(NULL, tmp);

		//m_pTekWnd->CreateEx(WS_EX_WINDOWEDGE,
		//			AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, AfxGetApp()->LoadCursor(IDC_ARROW), (HBRUSH)(COLOR_WINDOW)),
		//			tmp, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER, CRect(0, 0, 0, 0), NULL, NULL, NULL);

		//m_pTekWnd->SetMenu(NULL);
		//m_pTekWnd->ModifyStyle(WS_CAPTION | WS_BORDER | WS_DLGFRAME | WS_THICKFRAME, WS_TILED);
		//m_pTekWnd->ModifyStyleEx(WS_EX_OVERLAPPEDWINDOW, WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT);
		//m_pTekWnd->SetLayeredWindowAttributes(RGB(255, 255, 255), 0, LWA_COLORKEY);
	}

	if ( m_Tek_Ver == 4014 || ver == 0 ) {
		if ( !m_pTekWnd->IsWindowVisible() )
			m_pTekWnd->ShowWindow(SW_SHOW);
	}

	if ( m_Tek_Ver == 4014 ) {
		m_Tek_Line  = 0x08 | (m_Tek_Line & 0x07);
		m_Tek_Font  = 0x08 | (m_Tek_Font & 0x07);
		m_Tek_Base  = 0;
		m_Tek_Angle = 0;
	}
}
void CTextRam::TekClose()
{
	if ( m_pTekWnd == NULL )
		return;
	m_pTekWnd->DestroyWindow();
}
void CTextRam::TekForcus(BOOL fg)
{
	if ( fg ) {		// to Tek Mode
		if ( m_Stage != STAGE_TEK )
			fc_Push(STAGE_TEK);
	} else {		// to VT Mode
		if ( m_Stage == STAGE_TEK )
			fc_POP(' ');
	}
	TekFlush();
}
void CTextRam::TekDraw(CDC *pDC, CRect &frame, BOOL bOnView)
{
	int ox = 0, oy = 0;
	int mv, dv;
	TEKNODE *tp;
	POINT po[2];
	CStringW tmp;
	int np = 0;
	int nf = 0;
	int na = 0;
	int OldMode;
	UINT OldAlign;
	LOGBRUSH LogBrush;
	CFont font, *pOldFont;
	CPen pen, *pOldPen;
	BOOL bFullScreen = FALSE;

	memset(&LogBrush, 0, sizeof(LOGBRUSH));

	if ( !bOnView && IsOptEnable(TO_RLGRPIND) )
		bFullScreen = TRUE;

	if ( bFullScreen ) {
		if ( (frame.Width() * TEK_WIN_HEIGHT / TEK_WIN_WIDTH) < frame.Height() ) {
			mv = frame.Width();
			dv = TEK_WIN_WIDTH;
			oy = TEK_WIN_HEIGHT * mv / dv;
			oy = (frame.Height() - oy) / 2;
		} else {
			mv = frame.Height();
			dv = TEK_WIN_HEIGHT;
			ox = TEK_WIN_WIDTH * mv / dv;
			ox = (frame.Width() - ox) / 2;
		}

	} else {
		if ( (frame.Width() * TEK_WIN_HEIGHT / TEK_WIN_WIDTH) < frame.Height() ) {
			mv = frame.Height();
			dv = TEK_WIN_HEIGHT;
			ox = TEK_WIN_WIDTH * mv /dv;
			ox = (frame.Width() - ox) / 2;
		} else {
			mv = frame.Width();
			dv = TEK_WIN_WIDTH;
			oy = TEK_WIN_HEIGHT * mv /dv;
			oy = (frame.Height() - oy) / 2;
		}
	}

	pen.CreatePen(TekPenTypeTab[np], TekPenWidthTab[np], RGB(0, 0, 0));
	font.CreateFont(0 - TekFonts[nf & 7][1] * mv / dv, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));

	pOldPen  = pDC->SelectObject(&pen);
	pOldFont = pDC->SelectObject(&font);
	OldMode  = pDC->SetBkMode(TRANSPARENT);
	OldAlign = pDC->SetTextAlign(TA_LEFT | TA_BOTTOM);

	pDC->SetTextColor(TekColorTab[(nf >> 3) & 0x0F]);
//	pDC->FillSolidRect(frame, RGB(255, 255, 255));

	for ( tp = m_Tek_Top ; tp != NULL ; tp = tp->next ) {
		switch(tp->md) {
		case 0:	// Line
			if ( np != tp->st ) {
				np = tp->st;
				pen.DeleteObject();
				if ( TekPenTypeTab[np % 8] == PS_USERSTYLE ) {
					LogBrush.lbColor = TekColorTab[(np / 8) % 16];
					LogBrush.lbStyle = BS_SOLID;
					pen.CreatePen(PS_USERSTYLE, TekPenWidthTab[np / 128], &LogBrush, 4, TekPenExtTab[np % 8]);
				} else
					pen.CreatePen(TekPenTypeTab[np % 8], TekPenWidthTab[np / 128], TekColorTab[(np / 8) % 16]);
				pDC->SelectObject(&pen);
			}
			po[0].x = frame.left   + tp->sx * mv / dv + ox;
			po[0].y = frame.bottom - tp->sy * mv / dv - oy;
			po[1].x = frame.left   + tp->ex * mv / dv + ox;
			po[1].y = frame.bottom - tp->ey * mv / dv - oy;
			pDC->Polyline(po, 2);
			break;
		case 1:	// Text
			if ( (nf & 7) != (tp->st & 7) || na != tp->ex ) {
				nf = tp->st;
				na = tp->ex;
				font.DeleteObject();
				font.CreateFont(0 - TekFonts[nf & 7][1] * mv / dv, 0, na * 10, 0, FW_DONTCARE, 0, 0, 0, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
				pDC->SelectObject(&font);
			}
			pDC->SetTextColor(TekColorTab[(tp->st >> 3) & 0x0F]);
			if ( (tp->ch & 0xFFFF0000) != 0 )
				tmp = (WCHAR)(tp->ch >> 16);
			tmp = (WCHAR)(tp->ch & 0xFFFF);
			::ExtTextOutW(pDC->m_hDC,
				frame.left   + tp->sx * mv / dv + ox,
				frame.bottom - tp->sy * mv / dv - oy,
				0, NULL, tmp, tmp.GetLength(), NULL);
			break;
		}
	}

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldFont);
	pDC->SetBkMode(OldMode);
	pDC->SetTextAlign(OldAlign);
}
void CTextRam::TekFlush()
{
	if ( m_pTekWnd != NULL )
		m_pTekWnd->Invalidate(1);

	m_Tek_Flush = FALSE;

	if ( m_pDocument != NULL )
		m_pDocument->UpdateAllViews(NULL, UPDATE_TEKFLUSH, NULL);

//	TRACE("Tek Flush\n");
}
CTextRam::TEKNODE *CTextRam::TekAlloc()
{
	TEKNODE *tp;

	if ( (tp = m_Tek_Free) != NULL )
		m_Tek_Free = tp->next;
	else
		tp = new TEKNODE;

	return tp;
}
void CTextRam::TekFree(TEKNODE *tp)
{
	tp->next = m_Tek_Free;
	m_Tek_Free = tp;
}
void CTextRam::TekAdd(TEKNODE *tp)
{
	if ( m_Tek_Top == NULL || m_Tek_Btm == NULL ) {
		tp->next = NULL;
		m_Tek_Top = m_Tek_Btm = tp;
	} else {
		tp->next = NULL;
		m_Tek_Btm->next = tp;
		m_Tek_Btm = tp;
	}
}
void CTextRam::TekClear(BOOL bFlush)
{
	int n;
	TEKNODE *tp;

	tp = m_Tek_Free;
	for ( n = 0 ; tp != NULL ; n++ )
		tp = tp->next;

	for ( ; (tp = m_Tek_Top) != NULL ; n++ ) {
		m_Tek_Top = tp->next;
		if ( n < 100 )
			TekFree(tp);
		else
			delete tp;
	}
	m_Tek_Btm = NULL;

	if ( m_Tek_Ver == 4014 ) {
		m_Tek_Font = 0x08;
		m_Tek_Base = 0;
		m_Tek_Angle = 0;
	}

	m_Tek_Line = 0x08;
	m_Tek_cX = 0; m_Tek_cY = TEK_WIN_HEIGHT - 1;
	m_Tek_lX = 0; m_Tek_lY = m_Tek_cY;
	m_Tek_wX = 0; m_Tek_wY = m_Tek_cY;

	if ( bFlush )
		TekFlush();
}
void CTextRam::TekLine(int st, int sx, int sy, int ex, int ey)
{
	TEKNODE *tp = TekAlloc();

	tp->md = 0;
	tp->st = st;
	tp->sx = sx;
	tp->sy = sy;
	tp->ex = ex;
	tp->ey = ey;

	TekAdd(tp);

	if ( m_Tek_Flush )
		TekFlush();

//	TRACE("Tek Line #%d %d,%d - %d,%d\n", st, sx, sy, ex, ey);
}
void CTextRam::TekText(int st, int sx, int sy, DWORD ch)
{
	TEKNODE *tp = TekAlloc();

	tp->md = 1;
	tp->st = st;
	if ( m_Tek_Ver == 4105 ) {
		tp->sx = sx - (TEK_FONT_HEIGHT * SinTab[m_Tek_Angle + 90] / SINDIV / 6);
		tp->sy = sy - (TEK_FONT_HEIGHT * SinTab[m_Tek_Angle] / SINDIV / 6);
		tp->ex = m_Tek_Angle;
	} else {
		tp->sx = sx;
		tp->sy = sy;
		tp->ex = 0;
	}
	tp->ey = 0;
	tp->ch = ch;

	TekAdd(tp);

	if ( m_Tek_Flush )
		TekFlush();

//	TRACE("Tek Text #%d %d,%d (%c)\n", st, sx, sy, ch);
}
void CTextRam::TekMark(int st, int no, int sx, int sy)
{
	int n, i, x = 0, y = 0, lx, ly;
	BOOL b = FALSE;

	if ( no < 0 ) no = 0;
	else if ( no > 10 ) no = 10;

	for ( n = 0 ; MarkTab[no][n] != 126 ; ) {
		i = MarkTab[no][n++];
		if ( i != 127 ) {
			x = sx + i * 2 / 3;
			y = sy + MarkTab[no][n++] * 2 / 3;
			if ( b )
				TekLine(st, lx, ly, x, y);
			b = TRUE;
		} else
			b = FALSE;
		lx = x; ly = y;
	}
}
void CTextRam::TekEdge(int ag)
{
	int a, b;
	int r = (m_Tek_Angle + m_Tek_Base * 90 + ag * 90) % 360;
	int sx = m_Tek_cX + (TEK_WIN_WIDTH * SinTab[r] / SINDIV);
	int sy = m_Tek_cY - (TEK_WIN_WIDTH * SinTab[r + 90] / SINDIV);

	if ( m_Tek_cX == sx )
		m_Tek_cY = (sy < m_Tek_cY ? 0 : (TEK_WIN_HEIGHT - 1));
	else if ( m_Tek_cY == sy )
		m_Tek_cX = (sx < m_Tek_cX ? 0 : (TEK_WIN_WIDTH - 1));
	else {
		a = (m_Tek_cY - sy) * 1000 / (m_Tek_cX - sx);
		b = sy - a * sx / 1000;
		m_Tek_cX = (sx < m_Tek_cX ? 0 : (TEK_WIN_WIDTH - 1));
		if ( (m_Tek_cY = a * m_Tek_cX / 1000 + b) < 0 ) {
			m_Tek_cY = 0;
			m_Tek_cX = (m_Tek_cY - b) * 1000 / a;
		} else if ( m_Tek_cY >= TEK_WIN_HEIGHT ) {
			m_Tek_cY = TEK_WIN_HEIGHT - 1;
			m_Tek_cX = (m_Tek_cY - b) * 1000 / a;
		}
	}
}
void CTextRam::fc_TEK_IN(DWORD ch)
{
	// ESC 0C	Tek4014 Enable
	TekInit(4014);
	TekClear();
	fc_Case(STAGE_TEK);
}
void CTextRam::fc_TEK_LEFT(DWORD ch)
{
	// 0x08 LEFT

	int w = ((m_Tek_Base & 1) == 0 ? TEK_FONT_WIDTH : TEK_FONT_HEIGHT);
	int r = (m_Tek_Angle + m_Tek_Base * 90 + 180) % 360;
	int ox = (w * SinTab[r] / SINDIV);
	int oy = (w * SinTab[r + 90] / SINDIV);

	m_Tek_cX += ox;
	m_Tek_cY -= oy;

	if ( m_Tek_cX < 0 || m_Tek_cX >= TEK_WIN_WIDTH || m_Tek_cY < 0 || m_Tek_cY >= TEK_WIN_HEIGHT ) {
		TekEdge(0);	// Right
		m_Tek_cX += ox;
		m_Tek_cY -= oy;
		fc_TEK_UP(0x0B);
	}
}
void CTextRam::fc_TEK_UP(DWORD ch)
{
	// 0x0B UP

	int h = ((m_Tek_Base & 1) == 0 ? TEK_FONT_HEIGHT : TEK_FONT_WIDTH);
	int r = (m_Tek_Angle + m_Tek_Base * 90 + 90) % 360;

	m_Tek_cX += (h * SinTab[r] / SINDIV);
	m_Tek_cY -= (h * SinTab[r + 90] / SINDIV);

	if ( m_Tek_cX < 0 || m_Tek_cX >= TEK_WIN_WIDTH || m_Tek_cY < 0 || m_Tek_cY >= TEK_WIN_HEIGHT )
		TekEdge(3);	// Bottom
}
void CTextRam::fc_TEK_RIGHT(DWORD ch)
{
	// 0x09 RIGHT

	int w = ((m_Tek_Base & 1) == 0 ? TEK_FONT_WIDTH : TEK_FONT_HEIGHT);
	int r = (m_Tek_Angle + m_Tek_Base * 90) % 360;

	m_Tek_cX += (w * SinTab[r] / SINDIV);
	m_Tek_cY -= (w * SinTab[r + 90] / SINDIV);

	if ( m_Tek_cX < 0 || m_Tek_cX >= TEK_WIN_WIDTH || m_Tek_cY < 0 || m_Tek_cY >= TEK_WIN_HEIGHT ) {
		TekEdge(2);	// Left
		fc_TEK_DOWN(0x0A);
	}
}
void CTextRam::fc_TEK_DOWN(DWORD ch)
{
	// 0x0A DOWN

	int h = ((m_Tek_Base & 1) == 0 ? TEK_FONT_HEIGHT : TEK_FONT_WIDTH);
	int r = (m_Tek_Angle + m_Tek_Base * 90 + 270) % 360;
	int ox = (h * SinTab[r] / SINDIV);
	int oy = (h * SinTab[r + 90] / SINDIV);

	m_Tek_cX += ox;
	m_Tek_cY -= oy;

	if ( m_Tek_cX < 0 || m_Tek_cX >= TEK_WIN_WIDTH || m_Tek_cY < 0 || m_Tek_cY >= TEK_WIN_HEIGHT ) {
		TekEdge(1);	// Top
		m_Tek_cX += ox;
		m_Tek_cY -= oy;
	}
}
void CTextRam::fc_TEK_FLUSH(DWORD ch)
{
	TekEdge(2);	// Left
	TekFlush();
}
void CTextRam::fc_TEK_MODE(DWORD ch)
{
	// 0x1B ESC
	// 0x1C PT
	// 0x1D PLT
	// 0x1E IPL
	// 0x1F ALP
	m_Tek_Mode = m_BackChar = ch;
	m_Tek_Stat = 0;
	m_Tek_Pen = 0;
}
void CTextRam::fc_TEK_STAT(DWORD ch)
{
	int n;

	switch(m_Tek_Mode) {
	case 0x1B:	// ESC
		m_Tek_Gin = FALSE;
		switch(ch) {
		case 0x03:				// VT_MODE
			fc_POP(' ');
			TekFlush();
			break;
		case 0x05:				// REPORT
			UNGETSTR(_T("4%c%c%c%c\r"), ' ' + ((m_Tek_cX >> 7) & 0x1F), ' ' + ((m_Tek_cX >> 2) & 0x1F), ' ' + ((m_Tek_cY >> 7) & 0x1F), ' ' + ((m_Tek_cY >> 2) & 0x1F));
			break;
		case 0x0C:				// PAGE
			TekClear();
			break;
		case 0x0E:				// APL Font Set
		case 0x0F:				// ASCII Font Set
			break;
		case 0x17:				// COPY
		case 0x18:				// BYP
			break;
		case 0x1A:				// GIN
			m_Tek_Gin = TRUE;
			break;
		case 0x38: case 0x39:
		case 0x3A: case 0x3B:	// CHAR_SIZE	Font Size 0-3
			m_Tek_Font = 0x08 | ch & 0x03;
			break;
		case '[':				// CSI
			m_Tek_Mode = '[';
			m_BackChar = 0;
			m_AnsiPara.RemoveAll();
			return;
		case ']':				// OSC
			m_Tek_Mode = ']';
			return;
		default:
			if ( ch == '%' || (ch >= 0x40 && ch <= 0x5F) ) {
				m_Tek_Mode = '%';
				m_BackChar = (ch << 8);
				return;
			} else if ( ch >= 0x60 && ch <= 0x67 )	// beam and vector selector 0-7 nomal
				m_Tek_Line = 0x08 | (ch & 0x07);
			else if ( ch >= 0x68 && ch <= 0x6F )	// beam and vector selector 0-7 defocused
				m_Tek_Line = 0x00 | (ch & 0x07);
			else if ( ch >= 0x70 && ch <= 0x77 )	// beam and vector selector 0-7 white-thru
				m_Tek_Line = 0x18 | (ch & 0x07);
			break;
		}
		m_Tek_Mode = 0x1F;	// ALP
		break;

	case 0x1E:	// IPL
		m_Tek_Gin = FALSE;
		switch(ch) {
		case ' ': m_Tek_Pen = 0; return;
		case 'A':             m_Tek_cX++; break;
		case 'B':             m_Tek_cX--; break;
		case 'D': m_Tek_cY++;             break;
		case 'E': m_Tek_cY++; m_Tek_cX++; break;
		case 'F': m_Tek_cY++; m_Tek_cX--; break;
		case 'H': m_Tek_cY--;             break;
		case 'I': m_Tek_cY--; m_Tek_cX++; break;
		case 'J': m_Tek_cY--; m_Tek_cX--; break;
		case 'P': m_Tek_Pen = 1; return;
		}
		if ( m_Tek_Pen )
			TekLine(m_Tek_Line, m_Tek_lX, m_Tek_lY, m_Tek_cX, m_Tek_cY);
		m_Tek_lX = m_Tek_cX; m_Tek_lY = m_Tek_cY;
		break;

	case 0x1F:	// ALP
		m_Tek_Gin = FALSE;
		n = 0;
		switch(m_KanjiMode) {
		case EUC_SET:
		case ASCII_SET:
			switch(m_Tek_Stat) {
			case 0:
				if ( ch >= ' ' && ch <= 0x7F || ch >= 0x80 && ch <= 0x8D || ch >= 0x90 && ch <= 0x9F ) {
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				} else if ( ch == 0x8E ) {
					m_Tek_Stat = 2;
				} else if ( ch == 0x8F ) {
					m_Tek_Stat = 3;
				} else if ( ch >= 0xA0 && ch <= 0xFF ) {
					m_BackChar = ch << 8;
					m_Tek_Stat = 1;
				}
				break;
			case 1:
				ch |= m_BackChar;
				ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], _T("UTF-16BE"), ch);
				TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
				n = 2;
				m_Tek_Stat = 0;
				break;
			case 2:
				ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], _T("UTF-16BE"), ch | 0x80);
				TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
				n = 1;
				m_Tek_Stat = 0;
				break;
			case 3:
				m_BackChar = ch << 8;
				m_Tek_Stat = 1;
				break;
			}
			break;
		case SJIS_SET:
			switch(m_Tek_Stat) {
			case 0:
				if ( ch >= ' ' && ch <= 0x7F ) {
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				} else if ( issjis1(ch) ) {
					m_BackChar = ch << 8;
					m_Tek_Stat = 1;
				} else if ( iskana(ch) ) {
					ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], _T("UTF-16BE"), ch);
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				}
				break;
			case 1:
				if ( issjis2(ch) ) {
					ch |= m_BackChar;
					ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], _T("UTF-16BE"), ch);
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 2;
				}
				m_Tek_Stat = 0;
				break;
			}
			break;
		case UTF8_SET:
			switch(m_Tek_Stat) {
			case 0:
				if ( ch >= ' ' && ch <= 0x7F ) {
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				} else if ( ch >= 0xC0 && ch <= 0xDF ) {
					m_BackChar = (ch & 0x1F) << 6;
					m_Tek_Stat = 3;
				} else if ( ch >= 0xE0 && ch <= 0xEF ) {
					m_BackChar = (ch & 0x0F) << 12;
					m_Tek_Stat = 2;
				} else if ( ch >= 0xF0 && ch <= 0xF7 ) {
					m_BackChar = (ch & 0x07) << 18;
					m_Tek_Stat = 1;
				}
				break;
			case 1:
				if ( ch >= 0x80 && ch <= 0xBF ) {
					m_BackChar |= (ch & 0x3F) << 12;
					m_Tek_Stat = 2;
				} else
					m_Tek_Stat = 0;
				break;
			case 2:
				if ( ch >= 0x80 && ch <= 0xBF ) {
					m_BackChar |= (ch & 0x3F) << 6;
					m_Tek_Stat = 3;
				} else
					m_Tek_Stat = 0;
				break;
			case 3:
				if ( ch >= 0x80 && ch <= 0xBF ) {
					ch = m_BackChar | (ch & 0x3F);
					ch = m_IConv.IConvChar(_T("UCS-4BE"), _T("UTF-16BE"), ch);
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 2;
				}
				m_Tek_Stat = 0;
				break;
			}
			break;
		}
		for ( ; n > 0 ; n-- )
			fc_TEK_RIGHT(0x09);
		break;

	case '[':	// CSI
		if ( ch >= 0x20 && ch <= 0x2F ) {
			m_BackChar &= 0xFF0000;
			m_BackChar |= (ch << 8);
		} else if ( ch >= 0x30 && ch <= 0x39 ) {
			if ( m_AnsiPara.GetSize() <= 0 )
				m_AnsiPara.Add(0);
			n = (int)m_AnsiPara.GetSize() - 1;
			m_AnsiPara[n] = m_AnsiPara[n] * 10 + ((ch & 0x7F) - '0');
		} else if ( ch == 0x3B ) {
			m_AnsiPara.Add(0);
		} else if ( ch >= 0x3C && ch <= 0x3F ) {
			m_BackChar = (ch << 16);
		} else if ( ch >= 0x40 ) {
			switch(m_BackChar | (ch & 0x7F)) {
			case ('?' << 16) | 'l':
				for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
					switch(GetAnsiPara(n, 0, 0)) {
					case 38:	
						fc_POP(' ');
						TekFlush();
						break;
					}
				}
				break;
			case 'm':		// special color linetypes for MS-DOS Kermit v2.31 tektronix emulator
				for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
					switch(GetAnsiPara(n, 0, 0)) {
					case 0:
						m_Tek_Line &= 0xBF;
						break;
					case 1:
						m_Tek_Line |= 0x40;
						break;
					case 30: case 31: case 32: case 33:
					case 34: case 35: case 36: case 37:
						m_Tek_Line = (m_Tek_Line & 0xC7) | ((GetAnsiPara(n, 0, 0) & 0x07) << 3);
						break;
					}
				}
				break;
			}
			m_Tek_Mode = 0x1F;	// ALP
		}
		break;

	case ']':	// OSC
		if ( ch == 0x07 )
			m_Tek_Mode = 0x1F;	// ALP
		break;

	case '%':
		if ( ch == '%' )
			break;
		if ( ch == '!' || (ch >= 'A' && ch <= 'Z') )
			m_BackChar |= ch;

		m_Tek_Mode = 'A';
		m_Tek_Int = m_Tek_Pos = 0;
		m_Tek_Gin = FALSE;
		goto DOESC;

	case 'A':
		if ( ch >= 0x20 && ch <= 0x3F ) {
			m_Tek_Int = (m_Tek_Int << 4) | (ch & 0x0F);
			if ( (ch & 0x10) == 0 )
				m_Tek_Int = 0 - m_Tek_Int;
			m_Tek_Pos++;
		} else if ( ch >= 0x40 && ch <= 0x7F ) {
			m_Tek_Int = (m_Tek_Int << 6) | (ch & 0x3F);
			break;
		} else {
			m_Tek_Mode = 0x1F;	// ALP
			break;
		}

	DOESC:
		switch(m_BackChar) {
		case ('%' << 8) | '!':	// set mode 0=tek 1=ansi
			if ( m_Tek_Pos == 1 && m_Tek_Int == 1 ) {
				if ( m_pTekWnd != NULL && !m_pTekWnd->IsWindowVisible() )
					TekClose();
				fc_POP(' ');
				TekFlush();
			}
			break;

		case ('L' << 8) | 'Z':	// clear the dialog buffer
			TekFlush();
			break;

		case ('L' << 8) | 'G':	// vector
		case ('L' << 8) | 'F':	// move
		case ('L' << 8) | 'H':	// point
			m_Tek_Mode = 0x1D;
			m_Tek_Stat = 0;
			break;
		case ('L' << 8) | 'T':	// string
			if ( m_Tek_Pos == 1 )
				m_Tek_Mode = 0x1F;	// ALP
			break;

		case ('L' << 8) | 'V':	// set dialog area 0=invisible 1=visible
			if ( m_Tek_Pos != 1 )
				break;
			if ( m_pTekWnd == NULL )
				TekInit(4105);
			if ( !m_pTekWnd->IsWindowVisible() && m_Tek_Int != 0 )
				m_pTekWnd->ShowWindow(SW_SHOW);
			break;

		case ('M' << 8) | 'C':	// set character size Width,Heigh point
			if ( m_Tek_Pos == 2 ) {		// Height
				n = m_Tek_Int * 14 / 10;
				if ( n >= TekFonts[0][1] )
					m_Tek_Font = (m_Tek_Font & 0xF8) | 0;
				else if ( n >= TekFonts[1][1] )
					m_Tek_Font = (m_Tek_Font & 0xF8) | 1;
				else if ( n >= TekFonts[4][1] )
					m_Tek_Font = (m_Tek_Font & 0xF8) | 4;
				else if ( n >= TekFonts[5][1] )
					m_Tek_Font = (m_Tek_Font & 0xF8) | 5;
				else if ( n >= TekFonts[6][1] )
					m_Tek_Font = (m_Tek_Font & 0xF8) | 6;
				else if ( n >= TekFonts[7][1] )
					m_Tek_Font = (m_Tek_Font & 0xF8) | 7;
				else if ( n >= TekFonts[2][1] )
					m_Tek_Font = (m_Tek_Font & 0xF8) | 2;
				else
					m_Tek_Font = (m_Tek_Font & 0xF8) | 3;
			}
			break;

		case ('M' << 8) | 'G':	// set character write mode to overstrike
			break;
		case ('M' << 8) | 'L':	// set line index (color)
			if ( m_Tek_Pos == 1 )
				m_Tek_Line = (m_Tek_Line & 0x87) | ((m_Tek_Int & 0x0F) << 3);
			break;
		case ('M' << 8) | 'M':	// set point size
			if ( m_Tek_Pos == 1 )
				m_Tek_Point = m_Tek_Int % 11;
			break;
		case ('M' << 8) | 'N':	// set character path to 0 (characters placed equal to rotation)
			if ( m_Tek_Pos == 1 )
				m_Tek_Base = m_Tek_Int;
			break;
		case ('M' << 8) | 'Q':	// set character precision to string
			break;

		case ('M' << 8) | 'R':	// set string angle
			if ( m_Tek_Pos == 1 ) {
				m_Tek_Angle = m_Tek_Int % 360;
				if ( m_Tek_Angle < 0 )
					m_Tek_Angle += 360;
			}
			break;

		case ('M' << 8) | 'T':	// set character text index (color)
			if ( m_Tek_Pos == 1 )
				m_Tek_Font = (m_Tek_Font & 0x87) | ((m_Tek_Int & 0x0F) << 3);
			break;
		case ('M' << 8) | 'V':	// set line style
			if ( m_Tek_Pos == 1 )
				m_Tek_Line = (m_Tek_Line & 0xF8) | m_Tek_Int & 0x07;
			break;
		case ('R' << 8) | 'K':	// clear the view
		case ('S' << 8) | 'K':	// clear the segments
			TekClear();
			break;

		default:
			TRACE("Tek410x %c%c(%d:%d)\n", m_BackChar >> 8, m_BackChar & 0xFF, m_Tek_Int, m_Tek_Pos); 
			break;
		}

		m_Tek_Int = 0;
		break;

	case 0x1C:	// PT
	case 0x1D:	// PLT
		m_Tek_Gin = FALSE;
		if ( ch >= 0x40 && ch <= 0x5F ) {
			m_Tek_wX = (m_Tek_wX & 0xF83) | ((ch & 0x1F) << 2);
			m_Tek_cX = m_Tek_wX; m_Tek_cY = m_Tek_wY;
			m_Tek_Stat = 0;
		} else if ( ch >= 0x20 && ch <= 0x3F ) {
			if ( m_Tek_Stat )
				m_Tek_wX = (m_Tek_wX & 0x7F) | ((ch & 0x1F) << 7);
			else
				m_Tek_wY = (m_Tek_wY & 0x7F) | ((ch & 0x1F) << 7);
			break;
		} else if ( ch >= 0x60 && ch <= 0x7F ) {
			if ( m_Tek_Stat ) {
				m_Tek_wX = (m_Tek_wX & 0xFFC) | ((m_Tek_wY >> 2) & 0x03);
				m_Tek_wY = (m_Tek_wY & 0xFFC) | ((m_Tek_wY >> 4) & 0x03);
			} else {
				m_Tek_wX &= 0xFFC;
				m_Tek_wY &= 0xFFC;
			}
			m_Tek_wY = (m_Tek_wY & 0xF83) | ((ch & 0x1F) << 2);
			m_Tek_Stat = 1;
			break;
		} else if ( ch == 0x07 ) {
			m_Tek_Pen = 1;
			break;
		} else {
			break;
		}

		switch(m_BackChar) {
		case 0x1C:
			TekLine(m_Tek_Line, m_Tek_cX, m_Tek_cY, m_Tek_cX, m_Tek_cY);
			m_Tek_Pen = 1;
			break;
		case 0x1D:
			if ( m_Tek_Pen )
				TekLine(m_Tek_Line, m_Tek_lX, m_Tek_lY, m_Tek_cX, m_Tek_cY);
			m_Tek_Pen = 1;
			break;

		case ('L' << 8) | 'F':	// move
			break;
		case ('L' << 8) | 'G':	// vector
			TekLine(m_Tek_Line, m_Tek_lX, m_Tek_lY, m_Tek_cX, m_Tek_cY);
			break;
		case ('L' << 8) | 'H':	// point
			TekMark(m_Tek_Line & 0x78, m_Tek_Point, m_Tek_cX, m_Tek_cY);
			break;
		}

		m_Tek_lX = m_Tek_cX; m_Tek_lY = m_Tek_cY;
		break;
	}
}
