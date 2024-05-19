// DialogExt.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "Data.h"
#include "DialogExt.h"
#include "ComboBoxHis.h"

//////////////////////////////////////////////////////////////////////

CShortCutKey::CShortCutKey()
{
	m_MsgID   = 0;
	m_KeyCode = 0;
	m_KeyWith = 0;
	m_CtrlID  = 0;
	m_wParam  = 0;
}
CShortCutKey::~CShortCutKey()
{
}
const CShortCutKey & CShortCutKey::operator = (CShortCutKey &data)
{
	m_MsgID   = data.m_MsgID;
	m_KeyCode = data.m_KeyCode;
	m_KeyWith = data.m_KeyWith;
	m_CtrlID  = data.m_CtrlID;
	m_wParam  = data.m_wParam;

	return *this;
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CSizeGrip, CScrollBar)

CSizeGrip::CSizeGrip()
{
}
CSizeGrip::~CSizeGrip()
{
}

BEGIN_MESSAGE_MAP(CSizeGrip, CScrollBar)
    ON_WM_SETCURSOR()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CSizeGrip::Create(CWnd *pParentWnd, int cx, int cy)
{
    CRect frame, rect;

	ASSERT(pParentWnd != NULL);

    pParentWnd->GetClientRect(&frame);
    
    rect.left   = frame.right  - cx;
    rect.right  = frame.right;
    rect.top    = frame.bottom - cy;
    rect.bottom = frame.bottom;

    return CScrollBar::Create(WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP, rect, pParentWnd, 0);
}

void CSizeGrip::ParentReSize(int cx, int cy)
{
	CRect frame, rect;
	CWnd *pParentWnd = GetParent();

	ASSERT(pParentWnd != NULL);

    pParentWnd->GetClientRect(&frame);

    rect.left   = frame.right  - cx;
    rect.right  = frame.right;
    rect.top    = frame.bottom - cy;
    rect.bottom = frame.bottom;

	SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
}
BOOL CSizeGrip::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if ( pWnd == this  &&  nHitTest == HTCLIENT ) {
        ::SetCursor(::LoadCursor(NULL, IDC_SIZENWSE));
        return TRUE;
    }
    return CScrollBar::OnSetCursor(pWnd, nHitTest, message);
}
void CSizeGrip::OnPaint()
{
	CPaintDC dc(this);
	CDialogExt *pParent = (CDialogExt *)GetParent();
	int sz1 = MulDiv(1, pParent->m_NowDpi.cx, DEFAULT_DPI_X);
	int sz2 = MulDiv(2, pParent->m_NowDpi.cx, DEFAULT_DPI_X);
	int sz3 = MulDiv(3, pParent->m_NowDpi.cx, DEFAULT_DPI_X);
	CRect rect;
	int pos[6][2];

	GetClientRect(rect);

	pos[0][0] = pos[1][0] = pos[2][0] = rect.left + sz3 * 2;
	pos[3][0] = pos[4][0] = rect.left + sz3;
	pos[5][0] = rect.left;

	pos[0][1] = rect.top + sz1;
	pos[1][1] = pos[3][1] = rect.top + sz1 + sz3;
	pos[2][1] = pos[4][1] = pos[5][1] = rect.top + sz1 + sz3 * 2;

	for ( int n = 0 ; n < 6 ; n++ )
		dc.FillSolidRect(pos[n][0], pos[n][1], sz2, sz2, GetAppColor(COLOR_ACTIVEBORDER));
}

//////////////////////////////////////////////////////////////////////

#define	TABTOP_SIZE			2
#define	TABBTM_SIZE			3
#define	TABLINE_SIZE		1
#define	TABGAP_SIZE			4

#define	TABSTYLE_TOP		0
#define	TABSTYLE_BOTTOM		1
#define	TABSTYLE_LEFT		2
#define	TABSTYLE_RIGHT		3

IMPLEMENT_DYNAMIC(CTabCtrlExt, CTabCtrl)

CTabCtrlExt::CTabCtrlExt()
{
	m_ColTab[TABCOL_FACE]    = APPCOL_TABFACE;
	m_ColTab[TABCOL_TEXT]    = COLOR_GRAYTEXT;
	m_ColTab[TABCOL_BACK]    = APPCOL_DLGFACE;
	m_ColTab[TABCOL_SELFACE] = APPCOL_TABHIGH;
	m_ColTab[TABCOL_SELTEXT] = APPCOL_TABTEXT;
	m_ColTab[TABCOL_SELBACK] = APPCOL_TABHIGH;
	m_ColTab[TABCOL_BODER]   = APPCOL_TABSHADOW;

	m_bGradient = FALSE;
}

BEGIN_MESSAGE_MAP(CTabCtrlExt, CTabCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CTabCtrlExt::AdjustRect(BOOL bLarger, LPRECT lpRect)
{
	CTabCtrl::AdjustRect(bLarger, lpRect);

	lpRect->top    -= 1;
	lpRect->bottom -= 1;
	lpRect->left   -= 1;
	lpRect->right  += 1;
}
COLORREF CTabCtrlExt::GetColor(int num)
{
	if ( (m_ColTab[num] & TABCOL_COLREF) != 0 )
		return (COLORREF)(m_ColTab[num] & 0x00FFFFFF);
	else
		return GetAppColor((int)(m_ColTab[num] & 0xFF));
}
void CTabCtrlExt::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;
	int nTabIndex = (int)(lpDrawItemStruct->itemID);
	BOOL bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0 ? TRUE : FALSE;
	TC_ITEM tci;
	TCHAR title[MAX_PATH + 2] = { _T('\0') };
	int nSavedDC;
	COLORREF bc, tc, gc;
	CImageList* pImageList = GetImageList();
	int rotation = TABSTYLE_TOP;
	CFont *pOldFont = NULL, font;
	LOGFONT logfont;
	CSize sz;
	CPoint base;

	ASSERT(pDC != NULL);

	if ( nTabIndex < 0 )
		return;

	ZeroMemory(&tci, sizeof(tci));
	tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tci.pszText = title;
	tci.cchTextMax = MAX_PATH;

	if ( !GetItem(nTabIndex, &tci) ) {
		title[0] = _T('\0');
		tci.iImage = (-1);
		tci.lParam = NULL;
	}
	if ( (GetStyle() & TCS_VERTICAL) != 0 )
		rotation = (GetStyle() & TCS_RIGHT) != 0 ? TABSTYLE_RIGHT : TABSTYLE_LEFT;
	else if ( (GetStyle() & TCS_BOTTOM) != 0 )
		rotation = TABSTYLE_BOTTOM;

	nSavedDC = pDC->SaveDC();

	if ( rotation >= TABSTYLE_LEFT && (pOldFont = GetFont()) != NULL ) {
		pOldFont->GetLogFont(&logfont);
		logfont.lfEscapement = (rotation == TABSTYLE_LEFT != 0 ? 900 : 2700);
		font.CreateFontIndirect(&logfont);
		pOldFont = pDC->SelectObject(&font);
	}

	sz = pDC->GetTextExtent(title[0] == _T('\0') ? _T("ABC") : title);
	pDC->SetTextAlign(TA_LEFT | TA_BOTTOM);

	switch(rotation) {
	case TABSTYLE_TOP:
		base.x = rect.left + TABGAP_SIZE;
		base.y = rect.bottom - (rect.Height() - sz.cy) / 2;
		break;
	case TABSTYLE_BOTTOM:
		base.x = rect.left + TABGAP_SIZE;
		base.y = rect.bottom - (rect.Height() - sz.cy) / 2;
		break;
	case TABSTYLE_LEFT:
		base.x = rect.right - (rect.Width() - sz.cy) / 2;
		base.y = rect.bottom - TABGAP_SIZE;
		break;
	case TABSTYLE_RIGHT:
		base.x = rect.left + (rect.Width() - sz.cy) / 2;
		base.y = rect.top + TABGAP_SIZE;
		break;
	}

	if ( bSelected ) {
		bc = GetColor(TABCOL_SELFACE);
		tc = GetColor(TABCOL_SELTEXT);
		gc = GetColor(TABCOL_SELBACK);
	} else {
		bc = GetColor(TABCOL_FACE);
		tc = GetColor(TABCOL_TEXT);
		gc = GetColor(TABCOL_FACE);
	}

	if ( m_bGradient ) {
		TRIVERTEX tv[2] = { { 0, 0, (COLOR16)(GetRValue(bc) * 257), (COLOR16)(GetGValue(bc) * 257), (COLOR16)(GetBValue(bc) * 257), 0xffff },
							{ 0, 0, (COLOR16)(GetRValue(gc) * 257), (COLOR16)(GetGValue(gc) * 257), (COLOR16)(GetBValue(gc) * 257), 0xffff } };
		GRADIENT_RECT gr = { 0, 1 };
		DWORD dwMode = 0;

		switch(rotation) {
		case TABSTYLE_TOP:
			tv[0].x = rect.left;
			tv[0].y = rect.top + 2;
			tv[1].x = rect.right;
			tv[1].y = rect.bottom;
			dwMode = GRADIENT_FILL_RECT_V;
			pDC->FillSolidRect(CRect(rect.left + 2, rect.top, rect.right - 2, rect.top + 1), bc);
			pDC->FillSolidRect(CRect(rect.left + 1, rect.top + 1, rect.right - 1, rect.top + 2), bc);
			pDC->GradientFill(tv, 2, &gr, 1, dwMode);
			break;
		case TABSTYLE_BOTTOM:
			tv[0].x = rect.right;
			tv[0].y = rect.bottom - 2;
			tv[1].x = rect.left;
			tv[1].y = rect.top;
			dwMode = GRADIENT_FILL_RECT_V;
			pDC->FillSolidRect(CRect(rect.left + 2, rect.bottom, rect.right - 2, rect.bottom - 1), bc);
			pDC->FillSolidRect(CRect(rect.left + 1, rect.bottom - 1, rect.right - 1, rect.bottom - 2), bc);
			pDC->GradientFill(tv, 2, &gr, 1, dwMode);
			break;
		case TABSTYLE_LEFT:
			tv[0].x = rect.left + 2;
			tv[0].y = rect.top;
			tv[1].x = rect.right;
			tv[1].y = rect.bottom;
			dwMode = GRADIENT_FILL_RECT_H;
			pDC->FillSolidRect(CRect(rect.left, rect.top + 2, rect.left + 1, rect.bottom - 2), bc);
			pDC->FillSolidRect(CRect(rect.left + 1, rect.top + 1, rect.left + 2, rect.bottom - 1), bc);
			pDC->GradientFill(tv, 2, &gr, 1, dwMode);
			break;
		case TABSTYLE_RIGHT:
			tv[0].x = rect.right - 2;
			tv[0].y = rect.bottom;
			tv[1].x = rect.left;
			tv[1].y = rect.top;
			dwMode = GRADIENT_FILL_RECT_H;
			pDC->FillSolidRect(CRect(rect.right, rect.top + 2, rect.right - 1, rect.bottom - 2), bc);
			pDC->FillSolidRect(CRect(rect.right - 1, rect.top + 1, rect.right - 2, rect.bottom - 1), bc);
			pDC->GradientFill(tv, 2, &gr, 1, dwMode);
			break;
		}


	} else {
		switch(rotation) {
		case TABSTYLE_TOP:
			pDC->FillSolidRect(CRect(rect.left + 2, rect.top, rect.right - 2, rect.top + 1), bc);
			pDC->FillSolidRect(CRect(rect.left + 1, rect.top + 1, rect.right - 1, rect.top + 2), bc);
			pDC->FillSolidRect(CRect(rect.left, rect.top + 2, rect.right, rect.bottom), bc);
			break;
		case TABSTYLE_BOTTOM:
			pDC->FillSolidRect(CRect(rect.left + 2, rect.bottom, rect.right - 2, rect.bottom - 1), bc);
			pDC->FillSolidRect(CRect(rect.left + 1, rect.bottom - 1, rect.right - 1, rect.bottom - 2), bc);
			pDC->FillSolidRect(CRect(rect.left, rect.top, rect.right, rect.bottom - 2), bc);
			break;
		case TABSTYLE_LEFT:
			pDC->FillSolidRect(CRect(rect.left, rect.top + 2, rect.left + 1, rect.bottom - 2), bc);
			pDC->FillSolidRect(CRect(rect.left + 1, rect.top + 1, rect.left + 2, rect.bottom - 1), bc);
			pDC->FillSolidRect(CRect(rect.left + 2, rect.top, rect.right, rect.bottom), bc);
			break;
		case TABSTYLE_RIGHT:
			pDC->FillSolidRect(CRect(rect.right, rect.top + 2, rect.right - 1, rect.bottom - 2), bc);
			pDC->FillSolidRect(CRect(rect.right - 1, rect.top + 1, rect.right - 2, rect.bottom - 1), bc);
			pDC->FillSolidRect(CRect(rect.left, rect.top, rect.right - 2, rect.bottom), bc);
			break;
		}
	}

	if ( pImageList != 0 && tci.iImage >= 0 ) {
		IMAGEINFO info;
		pImageList->GetImageInfo(tci.iImage, &info);
		CRect ImageRect(info.rcImage);

		switch(rotation) {
		case TABSTYLE_TOP:
		case TABSTYLE_BOTTOM:
			pImageList->Draw(pDC, tci.iImage, CPoint(base.x, rect.top + (rect.Height() - ImageRect.Height()) / 2 + 1), ILD_TRANSPARENT);
			base.x += (ImageRect.Width() + TABGAP_SIZE);
			break;
		case TABSTYLE_LEFT:
			pImageList->Draw(pDC, tci.iImage, CPoint(rect.left + (rect.Width() - ImageRect.Width()) / 2 + 1, base.y - ImageRect.Height()), ILD_TRANSPARENT);
			base.y -= (ImageRect.Height() + TABGAP_SIZE);
			break;
		case TABSTYLE_RIGHT:
			pImageList->Draw(pDC, tci.iImage, CPoint(rect.left + (rect.Width() - ImageRect.Width()) / 2 + 1, base.y), ILD_TRANSPARENT);
			base.y += (ImageRect.Height() + TABGAP_SIZE);
			break;
		}
	}

	if ( title[0] != _T('\0') ) {
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(tc);

		switch(rotation) {
		case TABSTYLE_TOP:
		case TABSTYLE_BOTTOM:
			pDC->DrawText(title, CRect(base.x, base.y, rect.right, rect.top), DT_SINGLELINE | DT_TOP | DT_LEFT);
			break;
		case TABSTYLE_LEFT:
			pDC->DrawText(title, CRect(base.x, base.y, rect.left, rect.top), DT_SINGLELINE | DT_TOP | DT_LEFT);
			break;
		case TABSTYLE_RIGHT:
			pDC->DrawText(title, CRect(base.x, base.y, rect.right, rect.bottom), DT_SINGLELINE | DT_TOP | DT_LEFT);
			break;
		}
	}

	pDC->RestoreDC(nSavedDC);
}

void CTabCtrlExt::OnPaint()
{
	if ( (GetStyle() & (TCS_MULTILINE | TCS_BUTTONS)) != 0 ) {
		Default();
		return;
	}

	int n;
	CPoint last;
	CRect rect, frame;
	CPaintDC dc(this);
	DRAWITEMSTRUCT dis;
	CFont *pOldFont = GetFont();
	int nIndex = GetCurSel();
	COLORREF bdCol = GetColor(TABCOL_BODER);
	COLORREF bkCol = GetColor(TABCOL_SELBACK);
	CPen *pOldPen, bdPen(PS_SOLID, TABLINE_SIZE, bdCol);
	int dwStyle = GetStyle();
	CRect clip = dc.m_ps.rcPaint;

	pOldFont = dc.SelectObject(pOldFont);
	pOldPen = dc.SelectObject(&bdPen);

	ZeroMemory(&dis, sizeof(dis));
	dis.hDC = dc.GetSafeHdc();
	dis.CtlType = ODT_TAB;
	dis.itemAction = ODA_DRAWENTIRE;

	if ( GetItemCount() <= 0 )
		return;

	GetClientRect(frame);
	GetItemRect(0, rect);

	if ( (dwStyle & (TCS_BOTTOM | TCS_VERTICAL)) == 0 ) {
		// TOP Style (default)

		if ( frame.bottom > (rect.bottom + TABBTM_SIZE) )
			dc.FillSolidRect(frame.left, rect.bottom + TABBTM_SIZE, frame.Width(), frame.bottom - (rect.bottom + TABBTM_SIZE), bkCol);
		frame.bottom = rect.bottom + TABBTM_SIZE;

		last.SetPoint(0, frame.bottom - TABBTM_SIZE);

		for ( n = 0 ; n < GetItemCount() ; n++ ) {
			GetItemRect(n, rect);

			if ( last.x <= clip.right && rect.right >= clip.left ) {
				if ( rect.top < TABTOP_SIZE )
					rect.top = TABTOP_SIZE;
				if ( n == nIndex )
					rect.top -= TABTOP_SIZE;
				rect.bottom = frame.bottom - TABBTM_SIZE;

				dis.itemID = n;
				dis.rcItem = rect;
				dis.itemState = (n == nIndex ? ODS_SELECTED : 0);
				DrawItem(&dis);

				dc.FillSolidRect(last.x, last.y, rect.right - last.x, TABBTM_SIZE, bkCol);

				dc.MoveTo(last);
				dc.LineTo(rect.left, frame.bottom - TABBTM_SIZE);
				dc.LineTo(rect.left, rect.top + 2);
				dc.LineTo(rect.left + 2, rect.top);
				dc.LineTo(rect.right - 2, rect.top);
				dc.LineTo(rect.right, rect.top + 2);
				dc.LineTo(rect.right, frame.bottom - TABBTM_SIZE);

				if ( n != nIndex )
					dc.LineTo(rect.left, frame.bottom - TABBTM_SIZE);
			}

			last.SetPoint(rect.right, frame.bottom - TABBTM_SIZE);
		}

		if ( last.x <= clip.right ) {
			dc.FillSolidRect(last.x, last.y, frame.right - last.x, TABBTM_SIZE, bkCol);
			dc.MoveTo(last);
			dc.LineTo(frame.right, frame.bottom - TABBTM_SIZE);
		}

	} else if ( (dwStyle & (TCS_BOTTOM | TCS_VERTICAL)) == TCS_BOTTOM ) {
		// BOTTOM Style

		if ( (rect.top - TABBTM_SIZE) > frame.top )
			dc.FillSolidRect(frame.left, frame.top, frame.Width(), (rect.top - TABBTM_SIZE) - frame.top, bkCol);
		frame.top = rect.top - TABBTM_SIZE;
		frame.bottom -= 1;

		last.SetPoint(0, frame.top + TABBTM_SIZE);

		for ( n = 0 ; n < GetItemCount() ; n++ ) {
			GetItemRect(n, rect);

			if ( last.x <= clip.right && rect.right >= clip.left ) {
				if ( (frame.bottom - rect.bottom) < TABTOP_SIZE )
					rect.bottom = frame.bottom - TABTOP_SIZE;
				if ( n == nIndex )
					rect.bottom += TABTOP_SIZE;
				rect.top = frame.top + TABBTM_SIZE;

				dis.itemID = n;
				dis.rcItem = rect;
				dis.itemState = (n == nIndex ? ODS_SELECTED : 0);
				DrawItem(&dis);

				dc.FillSolidRect(last.x, last.y - TABBTM_SIZE, rect.right - last.x, TABBTM_SIZE, bkCol);

				dc.MoveTo(last);
				dc.LineTo(rect.left, frame.top + TABBTM_SIZE);
				dc.LineTo(rect.left, rect.bottom - 2);
				dc.LineTo(rect.left + 2, rect.bottom);
				dc.LineTo(rect.right - 2, rect.bottom);
				dc.LineTo(rect.right, rect.bottom - 2);
				dc.LineTo(rect.right, frame.top + TABBTM_SIZE);

				if ( n != nIndex )
					dc.LineTo(rect.left, frame.top + TABBTM_SIZE);
			}

			last.SetPoint(rect.right, frame.top + TABBTM_SIZE);
		}

		if ( last.x <= clip.right ) {
			dc.FillSolidRect(last.x, last.y - TABBTM_SIZE, frame.right - last.x, TABBTM_SIZE, bkCol);
			dc.MoveTo(last);
			dc.LineTo(frame.right, frame.top + TABBTM_SIZE);
		}

	} else if ( (dwStyle & (TCS_BOTTOM | TCS_VERTICAL)) == TCS_VERTICAL ) {
		// LEFT Style

		if ( frame.right > (rect.right + TABBTM_SIZE) )
			dc.FillSolidRect(rect.right + TABBTM_SIZE, frame.top, frame.right - (rect.right + TABBTM_SIZE), frame.Height(), bkCol);
		frame.right = rect.right + TABBTM_SIZE;

		last.SetPoint(frame.right - TABBTM_SIZE, 0);

		for ( n = 0 ; n < GetItemCount() ; n++ ) {
			GetItemRect(n, rect);

			if ( last.y <= clip.bottom && rect.bottom >= clip.top ) {
				if ( rect.left < TABTOP_SIZE )
					rect.left = TABTOP_SIZE;
				if ( n == nIndex )
					rect.left -= TABTOP_SIZE;
				rect.right = frame.right - TABBTM_SIZE;

				dis.itemID = n;
				dis.rcItem = rect;
				dis.itemState = (n == nIndex ? ODS_SELECTED : 0);
				DrawItem(&dis);

				dc.FillSolidRect(last.x, last.y, TABBTM_SIZE, rect.bottom - last.y, bkCol);

				dc.MoveTo(last);
				dc.LineTo(frame.right - TABBTM_SIZE, rect.top);
				dc.LineTo(rect.left + 2, rect.top);
				dc.LineTo(rect.left, rect.top + 2);
				dc.LineTo(rect.left, rect.bottom - 2);
				dc.LineTo(rect.left + 2, rect.bottom);
				dc.LineTo(frame.right - TABBTM_SIZE, rect.bottom);

				if ( n != nIndex )
					dc.LineTo(frame.right - TABBTM_SIZE, rect.top);
			}

			last.SetPoint(frame.right - TABBTM_SIZE, rect.bottom);
		}

		if ( last.y <= clip.bottom ) {
			dc.FillSolidRect(last.x, last.y, TABBTM_SIZE, frame.bottom - last.y, bkCol);
			dc.MoveTo(last);
			dc.LineTo(frame.right - TABBTM_SIZE, frame.bottom);
		}

	} else if ( (dwStyle & (TCS_RIGHT | TCS_VERTICAL)) == (TCS_RIGHT | TCS_VERTICAL) ) {	// TCS_RIGHT == TCS_BOTTOM
		// RIGHT Style

		if ( (rect.left - TABBTM_SIZE) > frame.left )
			dc.FillSolidRect(frame.left, frame.top, (rect.left - TABBTM_SIZE) - frame.left, frame.Height(), bkCol);
		frame.left = rect.left - TABBTM_SIZE;
		frame.right -= 1;

		last.SetPoint(frame.left + TABBTM_SIZE, 0);

		for ( n = 0 ; n < GetItemCount() ; n++ ) {
			GetItemRect(n, rect);

			if ( last.y <= clip.bottom && rect.bottom >= clip.top ) {
				if ( (frame.right - rect.right) < TABTOP_SIZE )
					rect.right = frame.right - TABTOP_SIZE;
				if ( n == nIndex )
					rect.right += TABTOP_SIZE;
				rect.left = frame.left + TABBTM_SIZE;

				dis.itemID = n;
				dis.rcItem = rect;
				dis.itemState = (n == nIndex ? ODS_SELECTED : 0);
				DrawItem(&dis);

				dc.FillSolidRect(last.x - TABBTM_SIZE, last.y, TABBTM_SIZE, rect.bottom - last.y, bkCol);

				dc.MoveTo(last);
				dc.LineTo(frame.left + TABBTM_SIZE, rect.top);
				dc.LineTo(rect.right - 2, rect.top);
				dc.LineTo(rect.right, rect.top + 2);
				dc.LineTo(rect.right, rect.bottom - 2);
				dc.LineTo(rect.right - 2, rect.bottom);
				dc.LineTo(frame.left + TABBTM_SIZE, rect.bottom);

				if ( n != nIndex )
					dc.LineTo(frame.left + TABBTM_SIZE, rect.top);
			}

			last.SetPoint(frame.left + TABBTM_SIZE, rect.bottom);
		}

		if ( last.y <= clip.bottom ) {
			dc.FillSolidRect(last.x - TABBTM_SIZE, last.y, TABBTM_SIZE, frame.bottom - last.y, bkCol);
			dc.MoveTo(last);
			dc.LineTo(frame.left + TABBTM_SIZE, frame.bottom);
		}
	}

	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldFont);
}
BOOL CTabCtrlExt::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(rect, GetColor(TABCOL_BACK));
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CDialogExt ダイアログ

IMPLEMENT_DYNAMIC(CDialogExt, CDialog)

CDialogExt::CDialogExt(UINT nIDTemplate, CWnd *pParent)
	: CDialog(nIDTemplate, pParent)
{
	m_nIDTemplate = nIDTemplate;
	m_bBackWindow = FALSE;

	m_FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	m_FontSize = ::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9);

	m_InitDpi.cx = SYSTEM_DPI_X;
	m_InitDpi.cy = SYSTEM_DPI_Y;
	m_NowDpi = m_InitDpi;

	m_InitDlgRect.RemoveAll();
	m_MaxSize.SetSize(0, 0);
	m_MinSize.SetSize(0, 0);
	m_OfsSize.SetSize(0, 0);

	m_SaveProfile.Empty();
	m_LoadPosMode = LOADPOS_NONE;
	m_bGripDisable = FALSE;
	m_bReSizeDisable = FALSE;

	m_bDarkMode = FALSE;
}
CDialogExt::~CDialogExt()
{
	int n;

	for ( n = 0 ;  n < m_ComboBoxPtr.GetSize() ; n++ ) {
		CComboBoxExt *pCombo = (CComboBoxExt *)m_ComboBoxPtr[n];
		if ( pCombo->GetSafeHwnd() != NULL )
			pCombo->UnsubclassWindow();
		delete pCombo;
	}
}

LRESULT CALLBACK OwnerCheckBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if ( uMsg == WM_ENABLE ) {
		if ( wParam )
			ExSetWindowTheme(hWnd, L"", L"");
		else
			ExSetWindowTheme(hWnd, NULL, NULL);
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK OwnerGroupBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if ( uMsg == WM_PAINT ) {
		CWnd *pWnd = CWnd::FromHandle(hWnd);
		CPaintDC dc(pWnd);
		CRect rect, box;
		CString title;
		CSize sz;
		CFont *pOldFont;
		HBRUSH hbr;

		pWnd->GetClientRect(rect);
		pOldFont = dc.SelectObject(pWnd->GetFont());
		pWnd->GetWindowText(title);
		sz = dc.GetTextExtent(title);

		dc.Draw3dRect(rect.left, rect.top + sz.cy / 2, rect.Width() - 1, rect.Height() - sz.cy / 2 - 1, GetAppColor(COLOR_BTNSHADOW), GetAppColor(COLOR_BTNSHADOW));

		hbr = (HBRUSH)::SendMessage(pWnd->GetParent()->GetSafeHwnd(), WM_CTLCOLORBTN, (WPARAM)dc.GetSafeHdc(), (LPARAM)hWnd);

		box.left   = rect.left + MulDiv(6, SCREEN_DPI_X, DEFAULT_DPI_X) - 2;
		box.right  = rect.left + MulDiv(6, SCREEN_DPI_X, DEFAULT_DPI_X) + sz.cx + 2;
		box.top    = rect.top;
		box.bottom = rect.top + sz.cy;

		if ( box.right >= rect.right )
			box.right = rect.right -= 3;

		dc.FillRect(box, CBrush::FromHandle(hbr));

		box.left  += 2;
		box.right -= 2;

		dc.DrawText(title, box, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_VCENTER | DT_LEFT);

		dc.SelectObject(pOldFont);
		return TRUE;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
static BOOL CALLBACK EnumSetThemeProc(HWND hWnd , LPARAM lParam)
{
	CWnd *pWnd = CWnd::FromHandle(hWnd);
	CDialogExt *pParent = (CDialogExt *)lParam;
	TCHAR name[256];

	if ( ExSetWindowTheme == NULL )
		return FALSE;

	if ( pWnd->GetParent()->GetSafeHwnd() != pParent->GetSafeHwnd() )
		return TRUE;

	GetClassName(hWnd, name, 256);

	// TRACE("ClassName %s\n", TstrToMbs(name));

	if ( _tcscmp(name, _T("Button")) == 0 ) {
		if ( !pParent->m_bDarkMode && (GetAppColor(APPCOL_DLGTEXT) != GetSysColor(COLOR_MENUTEXT)) )
			ExSetWindowTheme(hWnd, L"", L"");
		else {
			switch(pWnd->GetStyle() & BS_TYPEMASK) {
			case BS_PUSHBUTTON:
			case BS_DEFPUSHBUTTON:
			case BS_PUSHBOX:
				ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
				break;
			case BS_CHECKBOX:
				if ( (pWnd->GetStyle() & BS_PUSHLIKE) != 0 )
					ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
				else if ( pParent->m_bDarkMode )
					ExSetWindowTheme(hWnd, L"", L"");
				else
					ExSetWindowTheme(hWnd, NULL, NULL);
				break;
#ifdef	USE_DARKMODE
			case BS_AUTOCHECKBOX:
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
				if ( pParent->m_bDarkMode ) {
					SetWindowSubclass(hWnd, OwnerCheckBoxProc, (UINT_PTR)hWnd, (DWORD_PTR)pParent);
					if ( pWnd->IsWindowEnabled() )
						ExSetWindowTheme(hWnd, L"", L"");
					else
						ExSetWindowTheme(hWnd, NULL, NULL);
				} else {
					RemoveWindowSubclass(hWnd, OwnerCheckBoxProc, (UINT_PTR)hWnd);
					ExSetWindowTheme(hWnd, NULL, NULL);
				}
				break;
			case BS_GROUPBOX:
				if ( pParent->m_bDarkMode ) {
					SetWindowSubclass(hWnd, OwnerGroupBoxProc, (UINT_PTR)hWnd, (DWORD_PTR)pParent);
					ExSetWindowTheme(hWnd, L"", L"");
				} else {
					RemoveWindowSubclass(hWnd, OwnerGroupBoxProc, (UINT_PTR)hWnd);
					ExSetWindowTheme(hWnd, NULL, NULL);
				}
				break;
#endif
			default:
				if ( pParent->m_bDarkMode )
					ExSetWindowTheme(hWnd, L"", L"");
				else
					ExSetWindowTheme(hWnd, NULL, NULL);
				break;
			}
		}
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	} else if ( _tcscmp(name, _T("ScrollBar")) == 0 ) {
		ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

#ifdef	USE_DARKMODE
	//} else if ( _tcscmp(name, _T("Static")) == 0 ) {
	//	ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
	//	SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	} else if ( _tcscmp(name, _T("Edit")) == 0 ) {
		// DarkMode_CFDが良いがスクロールバーがdarkにならない
		// DarkMode_ExplorerではWS_EX_CLIENTEDGEが気になる
		if ( (pWnd->GetStyle() & ES_MULTILINE) != 0 ) {
			if ( pParent->m_bDarkMode ) {
				if ( (pWnd->GetStyle() & WS_BORDER) == 0 && (pWnd->GetExStyle() & WS_EX_CLIENTEDGE) != 0 ) {
					pWnd->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, 0);
					pWnd->ModifyStyle(0, WS_BORDER, SWP_DRAWFRAME);
				}
			} else {
				if ( (pWnd->GetStyle() & WS_BORDER) != 0 && (pWnd->GetExStyle() & WS_EX_CLIENTEDGE) == 0 ) {
					pWnd->ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);
					pWnd->ModifyStyle(WS_BORDER, 0, SWP_DRAWFRAME);
				}
			}
			ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
		} else
			ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_CFD" : L"Explorer"), NULL);
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	} else if ( _tcscmp(name, _T("ComboBox")) == 0 ) {
		ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_CFD" : L"Explorer"), NULL);
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);
		DWORD pos = ((CComboBox *)pWnd)->GetEditSel();
		if ( pos != CB_ERR )
			((CComboBox *)pWnd)->SetEditSel(LOWORD(pos), LOWORD(pos));

	} else if ( _tcscmp(name, _T("SysListView32")) == 0 ) {
		CListCtrl *pList = (CListCtrl *)pWnd;

		pList->SetTextColor(GetAppColor(APPCOL_CTRLTEXT));
		pList->SetTextBkColor(GetAppColor(APPCOL_CTRLFACE));
		pList->SetBkColor(GetAppColor(APPCOL_CTRLFACE));

		//ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_ItemsView" : L"ItemsView"), NULL);
		//ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
		ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : NULL), NULL);
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	} else if ( _tcscmp(name, _T("SysTreeView32")) == 0 ) {
		CTreeCtrl *pTree = (CTreeCtrl *)pWnd;

		pTree->SetTextColor(GetAppColor(APPCOL_CTRLTEXT));
		pTree->SetBkColor(GetAppColor(APPCOL_CTRLFACE));
		//pTree->SetInsertMarkColor(GetAppColor(APPCOL_CTRLFACE));

		//ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
		ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : NULL), NULL);
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	} else if ( _tcscmp(name, _T("msctls_progress32")) == 0 ) {
		CProgressCtrl *pProg = (CProgressCtrl *)pWnd;

		if ( pParent->m_bDarkMode ) {
			pProg->ModifyStyle(0, WS_BORDER, SWP_DRAWFRAME);
			pProg->SetBkColor(GetAppColor(APPCOL_CTRLFACE));
			ExSetWindowTheme(hWnd, L"", L"");
		} else {
			pProg->ModifyStyle(WS_BORDER, 0, SWP_DRAWFRAME);
			pProg->SetBkColor(GetAppColor(APPCOL_CTRLFACE));
			ExSetWindowTheme(hWnd, NULL, NULL);
		}
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	} else if ( _tcscmp(name, _T("msctls_trackbar32")) == 0 ) {
		ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
		//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

	}
#else
	} else if ( _tcscmp(name, _T("Edit")) == 0 ) {
		if ( (pWnd->GetStyle() & ES_MULTILINE) != 0 ) {
			ExSetWindowTheme(hWnd, (pParent->m_bDarkMode ? L"DarkMode_Explorer" : L"Explorer"), NULL);
			//SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);
		}
	}
#endif

	return TRUE;
}
static BOOL CALLBACK EnumItemOfsProc(HWND hWnd , LPARAM lParam)
{
	int cx, cy;
	int id, mode;
	int style = 0;
	CWnd *pWnd = CWnd::FromHandle(hWnd);
	CDialogExt *pParent = (CDialogExt *)lParam;
	CRect rect;
	WINDOWPLACEMENT place;
	INITDLGRECT dlgtmp;
	TCHAR name[256];

	if ( pWnd->GetParent()->GetSafeHwnd() != pParent->GetSafeHwnd() )
		return TRUE;

	pParent->GetClientRect(rect);
	cx = rect.Width();
	cy = rect.Height();

	GetClassName(hWnd, name, 256);
	pWnd->GetWindowPlacement(&place);
	id = pWnd->GetDlgCtrlID();

//	TRACE(_T("%s\n"), name);

	if ( id == IDC_STATIC || _tcscmp(name, _T("Static")) == 0 ) {
		switch(pWnd->GetStyle() & SS_TYPEMASK) {
		case SS_ICON:
		case SS_BITMAP:
			style = 2;
			break;
		case SS_BLACKRECT:
		case SS_GRAYRECT:
		case SS_WHITERECT:
		case SS_BLACKFRAME:
		case SS_GRAYFRAME:
		case SS_WHITEFRAME:
		case SS_OWNERDRAW:
			style = 1;
			break;
		}

	} else if ( _tcscmp(name, _T("SysListView32")) == 0 || _tcscmp(name, _T("SysTreeView32")) == 0 || _tcscmp(name, _T("msctls_trackbar32")) == 0 ) {
		style = 1;

	} else if ( _tcscmp(name, _T("Edit")) == 0 ) {
		if ( (pWnd->GetStyle() & WS_VSCROLL) != 0 )
			style = 1;

	} else if ( _tcscmp(name, _T("ScrollBar")) == 0 )
		return TRUE;

	switch(style) {
	case 0:		// bottom固定
		rect.left   = place.rcNormalPosition.left   * ITM_PER_MAX / cx;
		rect.right  = place.rcNormalPosition.right  * ITM_PER_MAX / cx;
		rect.top    = place.rcNormalPosition.top    * ITM_PER_MAX / cy;
		rect.bottom = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
		mode = ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_OFS;
		break;
	case 1:		// すべて
		rect.left   = place.rcNormalPosition.left   * ITM_PER_MAX / cx;
		rect.right  = place.rcNormalPosition.right  * ITM_PER_MAX / cx;
		rect.top    = place.rcNormalPosition.top    * ITM_PER_MAX / cy;
		rect.bottom = place.rcNormalPosition.bottom * ITM_PER_MAX / cy;
		mode = ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER;
		break;
	case 2:		// right,bottom固定
		rect.left   = place.rcNormalPosition.left   * ITM_PER_MAX / cx;
		rect.right  = place.rcNormalPosition.right  - place.rcNormalPosition.left;
		rect.top    = place.rcNormalPosition.top    * ITM_PER_MAX / cy;
		rect.bottom = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
		mode = ITM_LEFT_PER | ITM_RIGHT_OFS | ITM_TOP_PER | ITM_BTM_OFS;
		break;
	}

	dlgtmp.id   = id;
	dlgtmp.hWnd = hWnd;
	dlgtmp.mode = mode;
	dlgtmp.rect = rect;

	pParent->m_InitDlgRect.Add(dlgtmp);

	return TRUE;
}
void CDialogExt::DefItemOffset()
{
	CRect rect;

	if ( m_InitDlgRect.GetSize() > 0 )
		return;

	GetWindowRect(rect);
	m_MinSize.SetSize(rect.Width(), rect.Height());

	EnumChildWindows(GetSafeHwnd(), EnumItemOfsProc, (LPARAM)this);
}
void CDialogExt::InitItemOffset(const INITDLGTAB *pTab, int ox, int oy, int nx, int ny, int mx, int my)
{
	int n;
	int cx, cy;
	CRect frame, rect;
	WINDOWPLACEMENT place;
	CWnd *pWnd;
	INITDLGRECT dlgtmp;

	GetWindowRect(rect);

	if ( nx <= 0 )
		nx = rect.Width();
	if ( ny <= 0 )
		ny = rect.Height();

	m_OfsSize.SetSize(ox, oy);
	m_MinSize.SetSize(nx, ny);
	m_MaxSize.SetSize(mx, my);

	GetClientRect(frame);			// frame.left == 0, frame.top == 0 
	cx = frame.Width();
	cy = frame.Height();

	m_InitDlgRect.RemoveAll();

	for ( n = 0 ; pTab[n].id != 0 ; n++ ) {
		rect.SetRectEmpty();

		if ( (pWnd = GetDlgItem(pTab[n].id)) != NULL ) {
			pWnd->GetWindowPlacement(&place);

			switch(pTab[n].mode & 00007) {
			case ITM_LEFT_LEFT:
				rect.left = place.rcNormalPosition.left;
				break;
			case ITM_LEFT_MID:
				rect.left = place.rcNormalPosition.left - cx / 2;
				break;
			case ITM_LEFT_RIGHT:
				rect.left = place.rcNormalPosition.left - cx;
				break;
			case ITM_LEFT_PER:
				rect.left = place.rcNormalPosition.left * ITM_PER_MAX / cx;
				break;
			case ITM_LEFT_OFS:
				rect.left = place.rcNormalPosition.right - place.rcNormalPosition.left;
				break;
			}

			switch(pTab[n].mode & 00070) {
			case ITM_RIGHT_LEFT:
				rect.right = place.rcNormalPosition.right;
				break;
			case ITM_RIGHT_MID:
				rect.right = place.rcNormalPosition.right - cx/ 2;
				break;
			case ITM_RIGHT_RIGHT:
				rect.right = place.rcNormalPosition.right - cx;
				break;
			case ITM_RIGHT_PER:
				rect.right = place.rcNormalPosition.right * ITM_PER_MAX /  cx;
				break;
			case ITM_RIGHT_OFS:
				rect.right = place.rcNormalPosition.right - place.rcNormalPosition.left;
				break;
			}

			switch(pTab[n].mode & 00700) {
			case ITM_TOP_TOP:
				rect.top = place.rcNormalPosition.top;
				break;
			case ITM_TOP_MID:
				rect.top = place.rcNormalPosition.top - cy / 2;
				break;
			case ITM_TOP_BTM:
				rect.top = place.rcNormalPosition.top - cy;
				break;
			case ITM_TOP_PER:
				rect.top = place.rcNormalPosition.top * ITM_PER_MAX / cy;
				break;
			case ITM_TOP_OFS:
				rect.top = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
				break;
			}

			switch(pTab[n].mode & 07000) {
			case ITM_BTM_TOP:
				rect.bottom = place.rcNormalPosition.bottom;
				break;
			case ITM_BTM_MID:
				rect.bottom = place.rcNormalPosition.bottom - cy / 2;
				break;
			case ITM_BTM_BTM:
				rect.bottom = place.rcNormalPosition.bottom - cy;
				break;
			case ITM_BTM_PER:
				rect.bottom = place.rcNormalPosition.bottom * ITM_PER_MAX / cy;
				break;
			case ITM_BTM_OFS:
				rect.bottom = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
				break;
			}
		}

		dlgtmp.id   = pTab[n].id;
		dlgtmp.hWnd = NULL;
		dlgtmp.mode = pTab[n].mode;
		dlgtmp.rect = rect;
		m_InitDlgRect.Add(dlgtmp);
	}
}
void CDialogExt::SetItemOffset(int cx, int cy)
{
	int n, i;
	WINDOWPLACEMENT place;
	CWnd *pWnd;

	for ( n = 0 ; n < (int)m_InitDlgRect.GetSize() ; n++ ) {

		if ( m_InitDlgRect[n].hWnd != NULL )
			pWnd = CWnd::FromHandle(m_InitDlgRect[n].hWnd);
		else
			pWnd = GetDlgItem(m_InitDlgRect[n].id);

		if ( pWnd == NULL )
			continue;

		pWnd->GetWindowPlacement(&place);

		i = m_InitDlgRect[n].rect.left * m_NowDpi.cx / m_InitDpi.cx;

		switch(m_InitDlgRect[n].mode & 00007) {
		case 0:
			place.rcNormalPosition.left = i + m_OfsSize.cx;
			break;
		case ITM_LEFT_MID:
			place.rcNormalPosition.left = i + cx / 2;
			break;
		case ITM_LEFT_RIGHT:
			place.rcNormalPosition.left = i + cx;
			break;
		case ITM_LEFT_PER:
			place.rcNormalPosition.left = cx * m_InitDlgRect[n].rect.left / ITM_PER_MAX;
			break;
		}

		i = m_InitDlgRect[n].rect.right * m_NowDpi.cx / m_InitDpi.cx;

		switch(m_InitDlgRect[n].mode & 00070) {
		case 0:
			place.rcNormalPosition.right = i + m_OfsSize.cx;
			break;
		case ITM_RIGHT_MID:
			place.rcNormalPosition.right = i + cx / 2;
			break;
		case ITM_RIGHT_RIGHT:
			place.rcNormalPosition.right = i + cx;
			break;
		case ITM_RIGHT_PER:
			place.rcNormalPosition.right = cx * m_InitDlgRect[n].rect.right / ITM_PER_MAX;
			break;
		case ITM_RIGHT_OFS:
			place.rcNormalPosition.right = place.rcNormalPosition.left + i;
			break;
		}

		if ( (m_InitDlgRect[n].mode & 00007) == ITM_LEFT_OFS )
			place.rcNormalPosition.left = place.rcNormalPosition.right - m_InitDlgRect[n].rect.left * m_NowDpi.cx / m_InitDpi.cx;

		i = m_InitDlgRect[n].rect.top * m_NowDpi.cy / m_InitDpi.cy;

		switch(m_InitDlgRect[n].mode & 00700) {
		case 0:
			place.rcNormalPosition.top = i + m_OfsSize.cy;
			break;
		case ITM_TOP_MID:
			place.rcNormalPosition.top = i + cy / 2;
			break;
		case ITM_TOP_BTM:
			place.rcNormalPosition.top = i + cy;
			break;
		case ITM_TOP_PER:
			place.rcNormalPosition.top = cy * m_InitDlgRect[n].rect.top / ITM_PER_MAX;
			break;
		}

		i = m_InitDlgRect[n].rect.bottom * m_NowDpi.cy / m_InitDpi.cy;

		switch(m_InitDlgRect[n].mode & 07000) {
		case 0:
			place.rcNormalPosition.bottom = i + m_OfsSize.cy;
			break;
		case ITM_BTM_MID:
			place.rcNormalPosition.bottom = i + cy / 2;
			break;
		case ITM_BTM_BTM:
			place.rcNormalPosition.bottom = i + cy;
			break;
		case ITM_BTM_PER:
			place.rcNormalPosition.bottom = cy * m_InitDlgRect[n].rect.bottom / ITM_PER_MAX;
			break;
		case ITM_BTM_OFS:
			place.rcNormalPosition.bottom = place.rcNormalPosition.top + i;
			break;
		}

		if ( (m_InitDlgRect[n].mode & 00700) == ITM_TOP_OFS )
			place.rcNormalPosition.top = place.rcNormalPosition.bottom - m_InitDlgRect[n].rect.top * m_NowDpi.cy / m_InitDpi.cy;

		place.showCmd = (!IsWindowVisible() || pWnd->IsWindowVisible() ? SW_SHOW : SW_HIDE);

		pWnd->SetWindowPlacement(&place);
	}
}

void CDialogExt::GetActiveDpi(CSize &dpi, CWnd *pWnd, CWnd *pParent)
{
	UINT d;

	if ( pParent != NULL ) {
		CRuntimeClass *pClass = pParent->GetRuntimeClass();

		while ( pClass != NULL ) {
			if ( strcmp("CDialogExt", pClass->m_lpszClassName) == 0 ) {
				dpi = ((CDialogExt *)pParent)->m_NowDpi;
				return;
			}
			pClass = pClass->m_pBaseClass;
		}
	}

	if ( ExGetDpiForWindow != NULL && pWnd != NULL && pWnd->GetSafeHwnd() != NULL && (d = ExGetDpiForWindow(pWnd->GetSafeHwnd())) != 0 ) {
		dpi.cx = dpi.cy = d;

	} else if ( ExGetDpiForMonitor != NULL && pWnd != NULL && pWnd->GetSafeHwnd() != NULL && (::AfxGetMainWnd() == NULL || pWnd->GetSafeHwnd() != ::AfxGetMainWnd()->GetSafeHwnd()) ) {
		UINT DpiX, DpiY;
		HMONITOR hMonitor;
		CRect rect;

		pWnd->GetWindowRect(rect);
		hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
		ExGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &DpiX, &DpiY);

		dpi.cx = DpiX;
		dpi.cy = DpiY;

	} else if ( theApp.m_pMainWnd != NULL ) {
		dpi.cx = SCREEN_DPI_X;
		dpi.cy = SCREEN_DPI_Y;

	} else {
		dpi.cx = SYSTEM_DPI_X;
		dpi.cy = SYSTEM_DPI_Y;
	}
}
BOOL CDialogExt::IsDialogExt(CWnd *pWnd)
{
	CRuntimeClass *pClass = pWnd->GetRuntimeClass();

	while ( pClass != NULL ) {
		if ( strcmp("CDialogExt", pClass->m_lpszClassName) == 0 )
			return TRUE;
		pClass = pClass->m_pBaseClass;
	}

	return FALSE;
}
void CDialogExt::GetDlgFontBase(CDC *pDC, CFont *pFont, CSize &size)
{
	CSize TextSize;
	TEXTMETRIC TextMetric;
	CFont *pOld;

	pOld = pDC->SelectObject(pFont);
	pDC->GetTextMetrics(&TextMetric);
	::GetTextExtentPoint32(pDC->GetSafeHdc(), _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), 52, &TextSize);
	size.cx = (TextSize.cx + 26) / 52;
	size.cy = (TextMetric.tmHeight + TextMetric.tmExternalLeading);
	pDC->SelectObject(pOld);
}

void CDialogExt::CheckMoveWindow(CRect &rect, BOOL bRepaint)
{
	HMONITOR hMonitor;
    MONITORINFOEX  mi;

	hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &mi);

	// モニターを基準に調整
	if ( (mi.dwFlags & MONITORINFOF_PRIMARY) != 0 ) {
		CRect work;
		SystemParametersInfo(SPI_GETWORKAREA, 0, work, 0);
		mi.rcMonitor = work;
	}

	if ( rect.left < mi.rcMonitor.left ) {
		rect.right += (mi.rcMonitor.left - rect.left);
		rect.left  += (mi.rcMonitor.left - rect.left);
	}

	if ( rect.top < mi.rcMonitor.top ) {
		rect.bottom += (mi.rcMonitor.top - rect.top);
		rect.top    += (mi.rcMonitor.top - rect.top);
	}

	if ( rect.right > mi.rcMonitor.right ) {
		rect.left  -= (rect.right - mi.rcMonitor.right);
		rect.right -= (rect.right - mi.rcMonitor.right);
		if ( rect.left < mi.rcMonitor.left ) {
			rect.left  = mi.rcMonitor.left;
			rect.right = mi.rcMonitor.right;
		}
	}

	if ( rect.bottom > mi.rcMonitor.bottom ) {
		rect.top    -= (rect.bottom - mi.rcMonitor.bottom);
		rect.bottom -= (rect.bottom - mi.rcMonitor.bottom);
		if ( rect.top < mi.rcMonitor.top ) {
			rect.top    = mi.rcMonitor.top;
			rect.bottom = mi.rcMonitor.bottom;
		}
	}

	MoveWindow(rect, bRepaint);
}
void CDialogExt::LoadSaveDialogSize(BOOL bSave)
{
	CRect rect, client;
	int sx, sy;
	int cx, cy;
	int dx, dy;
	CWnd *pWnd;
	CSize fsz = m_NowFsz;

	GetWindowRect(rect);
	GetClientRect(client);

	if ( bSave ) {
		//AfxGetApp()->WriteProfileInt(m_SaveProfile, _T("cx"), MulDiv(rect.Width(),  DEFAULT_DPI_X * m_DefFsz.cx, m_NowDpi.cx * m_NowFsz.cx));
		//AfxGetApp()->WriteProfileInt(m_SaveProfile, _T("cy"), MulDiv(rect.Height(), DEFAULT_DPI_Y * m_DefFsz.cy, m_NowDpi.cy * m_NowFsz.cy));
		//AfxGetApp()->WriteProfileInt(m_SaveProfile, _T("WithFontSize"), m_FontSize);

		AfxGetApp()->WriteProfileInt(m_SaveProfile, _T("clientX"), MulDiv(client.Width(),  DEFAULT_DPI_X * m_DefFsz.cx, m_NowDpi.cx * m_NowFsz.cx));
		AfxGetApp()->WriteProfileInt(m_SaveProfile, _T("clientY"), MulDiv(client.Height(), DEFAULT_DPI_Y * m_DefFsz.cy, m_NowDpi.cy * m_NowFsz.cy));

		if ( m_LoadPosMode == LOADPOS_PROFILE ) {
			AfxGetApp()->WriteProfileInt(m_SaveProfile, _T("sx"), MulDiv(rect.left, DEFAULT_DPI_X, m_NowDpi.cx));
			AfxGetApp()->WriteProfileInt(m_SaveProfile, _T("sy"), MulDiv(rect.top,  DEFAULT_DPI_Y, m_NowDpi.cy));
		}

	} else {
		if ( AfxGetApp()->GetProfileInt(m_SaveProfile, _T("clientX"), 0) > 0 ) {
			dx = rect.Width()  - client.Width();
			dy = rect.Height() - client.Height();
			cx = AfxGetApp()->GetProfileInt(m_SaveProfile, _T("clientX"), MulDiv(client.Width(),  DEFAULT_DPI_X * m_DefFsz.cx, m_NowDpi.cx * m_NowFsz.cx));
			cy = AfxGetApp()->GetProfileInt(m_SaveProfile, _T("clientY"), MulDiv(client.Height(), DEFAULT_DPI_Y * m_DefFsz.cy, m_NowDpi.cy * m_NowFsz.cy));
			cx = MulDiv(cx, m_NowDpi.cx * m_NowFsz.cx, DEFAULT_DPI_X * m_DefFsz.cx) + dx;
			cy = MulDiv(cy, m_NowDpi.cy * m_NowFsz.cy, DEFAULT_DPI_Y * m_DefFsz.cy) + dy;

			if ( AfxGetApp()->GetProfileInt(m_SaveProfile, _T("WithFontSize"), 0) != 0 ) {
				((CRLoginApp *)AfxGetApp())->DelProfileEntry(m_SaveProfile, _T("cx"));
				((CRLoginApp *)AfxGetApp())->DelProfileEntry(m_SaveProfile, _T("cy"));
				((CRLoginApp *)AfxGetApp())->DelProfileEntry(m_SaveProfile, _T("WithFontSize"));
			}
		} else {
			if ( AfxGetApp()->GetProfileInt(m_SaveProfile, _T("cx"), (-1)) != (-1) && AfxGetApp()->GetProfileInt(m_SaveProfile, _T("WithFontSize"), 0) == 0 )
				fsz = m_DefFsz;
			cx = AfxGetApp()->GetProfileInt(m_SaveProfile, _T("cx"), MulDiv(rect.Width(),  DEFAULT_DPI_X * m_DefFsz.cx, m_NowDpi.cx * fsz.cx));
			cy = AfxGetApp()->GetProfileInt(m_SaveProfile, _T("cy"), MulDiv(rect.Height(), DEFAULT_DPI_Y * m_DefFsz.cy, m_NowDpi.cy * fsz.cy));
			cx = MulDiv(cx, m_NowDpi.cx * fsz.cx, DEFAULT_DPI_X * m_DefFsz.cx);
			cy = MulDiv(cy, m_NowDpi.cy * fsz.cy, DEFAULT_DPI_Y * m_DefFsz.cy);
		}

		if ( cx < rect.Width() )
			cx = rect.Width();
		if ( cy < rect.Height() )
			cy = rect.Height();

		sx = rect.left;
		sy = rect.top;

		switch(m_LoadPosMode) {
		case LOADPOS_PROFILE:
			sx = AfxGetApp()->GetProfileInt(m_SaveProfile, _T("sx"), MulDiv(rect.left, DEFAULT_DPI_X, m_NowDpi.cx));
			sy = AfxGetApp()->GetProfileInt(m_SaveProfile, _T("sy"), MulDiv(rect.top,  DEFAULT_DPI_Y, m_NowDpi.cy));
			sx = MulDiv(sx, m_NowDpi.cx, DEFAULT_DPI_X);
			sy = MulDiv(sy, m_NowDpi.cy, DEFAULT_DPI_Y);
			break;
		case LOADPOS_PARENT:
			if ( (pWnd = GetParent()) != NULL && !pWnd->IsIconic() && pWnd->IsWindowVisible() ) {
				pWnd->GetWindowRect(rect);
				sx = rect.left + (rect.Width()  - cx) / 2;
				sy = rect.top  + (rect.Height() - cy) / 2;
			}
			break;
		case LOADPOS_MAINWND:
			if ( (pWnd = ::AfxGetMainWnd()) != NULL && !pWnd->IsIconic() && pWnd->IsWindowVisible() ) {
				pWnd->GetWindowRect(rect);
				sx = rect.left + (rect.Width()  - cx) / 2;
				sy = rect.top  + (rect.Height() - cy) / 2;
			}
			break;
		}

		CheckMoveWindow(sx, sy, cx, cy, FALSE);
	}
}

#pragma pack(push, 1)
	typedef struct _DLGTEMPLATEEX {
		WORD dlgVer;
		WORD signature;
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		WORD cDlgItems;
		short x;
		short y;
		short cx;
		short cy;
	} DLGTEMPLATEEX, *LPCDLGTEMPLATEEX;
#pragma pack(pop)

BOOL CDialogExt::GetSizeAndText(SIZE *pSize, CString &title, CWnd *pParent)
{
	HGLOBAL hDialog;
	HGLOBAL hInitData = NULL;
	LPCDLGTEMPLATEEX lpDialogTemplate;
	WORD *wp;
	CSize dpi;

	if ( !((CRLoginApp *)AfxGetApp())->LoadResDialog(m_lpszTemplateName, hDialog, hInitData) )
		return FALSE;

	if ( (lpDialogTemplate = (LPCDLGTEMPLATEEX)LockResource(hDialog)) == NULL ) {
		FreeResource(hDialog);
		return FALSE;
	}

	CDialogTemplate dlgTemp((LPCDLGTEMPLATE)lpDialogTemplate);

	if ( IsDefineFont() )
		dlgTemp.SetFont(m_FontName, m_FontSize);
	//else {
	//	CString name;
	//	WORD size;
	//	dlgTemp.GetFont(name, size);
	//	if ( m_FontSize != size )
	//		dlgTemp.SetFont(name, m_FontSize);
	//}

	dlgTemp.GetSizeInPixels(pSize);

	if ( lpDialogTemplate->signature == 0xFFFF )
		wp = (WORD *)((LPCDLGTEMPLATEEX)lpDialogTemplate + 1);
	else
		wp = (WORD *)((LPCDLGTEMPLATE)lpDialogTemplate + 1);

	if ( *wp == (WORD)(-1) )        // Skip menu name string or ordinal
		wp += 2; // WORDs
	else
		while( *(wp++) != 0 );

	if ( *wp == (WORD)(-1) )        // Skip class name string or ordinal
		wp += 2; // WORDs
	else
		while( *(wp++) != 0 );

	// caption string
	title = (WCHAR *)wp;

	UnlockResource(hDialog);
	FreeResource(hDialog);

	if ( hInitData != NULL )
		FreeResource(hInitData);

	GetActiveDpi(dpi, NULL, pParent);

	if ( m_InitDpi.cx != dpi.cx || m_InitDpi.cy != dpi.cy ) {
		pSize->cx = MulDiv(pSize->cx, dpi.cx, m_InitDpi.cx);
		pSize->cy = MulDiv(pSize->cy, dpi.cy, m_InitDpi.cy);
	}

	return TRUE;
}

void CDialogExt::GetFontSize(CDialogTemplate *pDlgTemp, CSize &fsz)
{
	CString name;
	WORD size;
	CDC TempDC;
	CFont Font;

	pDlgTemp->GetFont(name, size);
	TempDC.CreateCompatibleDC(NULL);
	Font.CreatePointFont(size * 10, name, &TempDC);
	GetDlgFontBase(&TempDC, &Font, fsz);
}
BOOL CDialogExt::Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
{
	HGLOBAL hDialog;
	HGLOBAL hInitData = NULL;
	void* lpInitData = NULL;
	LPCDLGTEMPLATE lpDialogTemplate;

	m_lpszTemplateName = lpszTemplateName;

	if ( IS_INTRESOURCE(m_lpszTemplateName) && m_nIDHelp == 0 )
		m_nIDHelp = LOWORD((DWORD_PTR)m_lpszTemplateName);

	if ( !((CRLoginApp *)AfxGetApp())->LoadResDialog(m_lpszTemplateName, hDialog, hInitData) )
		return (-1);

	if ( hInitData != NULL )
		lpInitData = (void *)LockResource(hInitData);

	lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialog);

	CDialogTemplate dlgTemp(lpDialogTemplate);

	GetFontSize(&dlgTemp, m_DefFsz);

	if ( IsDefineFont() ) {
		dlgTemp.SetFont(m_FontName, m_FontSize);
		GetFontSize(&dlgTemp, m_NowFsz);
	} else
		m_NowFsz = m_DefFsz;

	lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(dlgTemp.m_hTemplate);

	BOOL bResult = CreateIndirect(lpDialogTemplate, pParentWnd, lpInitData);

	UnlockResource(dlgTemp.m_hTemplate);

	UnlockResource(hDialog);
	FreeResource(hDialog);

	if ( hInitData != NULL ) {
		UnlockResource(hInitData);
		FreeResource(hInitData);
	}

	return bResult;
}
INT_PTR CDialogExt::DoModal()
{
	HGLOBAL hDialog;
	HGLOBAL hInitData = NULL;
	LPCDLGTEMPLATE lpDialogTemplate;

	if ( !((CRLoginApp *)AfxGetApp())->LoadResDialog(m_lpszTemplateName, hDialog, hInitData) )
		return (-1);

	lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialog);

	CDialogTemplate dlgTemp(lpDialogTemplate);

	GetFontSize(&dlgTemp, m_DefFsz);

	if ( IsDefineFont() ) {
		dlgTemp.SetFont(m_FontName, m_FontSize);
		GetFontSize(&dlgTemp, m_NowFsz);
	} else
		m_NowFsz = m_DefFsz;

	lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(dlgTemp.m_hTemplate);

	LPCTSTR pSaveTempName = m_lpszTemplateName;
	LPCDLGTEMPLATE pSaveDiaTemp = m_lpDialogTemplate;

	m_lpszTemplateName = NULL;
	m_lpDialogTemplate = NULL;

	InitModalIndirect(lpDialogTemplate, m_pParentWnd, hInitData);
	INT_PTR result = CDialog::DoModal();

	m_lpszTemplateName = pSaveTempName;
	m_lpDialogTemplate = pSaveDiaTemp;

	UnlockResource(dlgTemp.m_hTemplate);

	UnlockResource(hDialog);
	FreeResource(hDialog);

	if ( hInitData != NULL )
		FreeResource(hInitData);

	return result;
}
void CDialogExt::AddShortCutKey(UINT MsgID, UINT KeyCode, UINT KeyWith, UINT CtrlID, WPARAM wParam)
{
	//	AddShortCutKey(0, VK_RETURN, MASK_CTRL, 0, IDOK);
	//	AddShortCutKey(0, VK_RETURN, MASK_CTRL, IDC_BUTTON, BN_CLICKED);

	CShortCutKey data;

	data.m_MsgID   = MsgID;
	data.m_KeyCode = KeyCode;
	data.m_KeyWith = KeyWith;
	data.m_CtrlID  = CtrlID;
	data.m_wParam  = wParam;

	m_Data.Add(data);
}
void CDialogExt::AddHelpButton(LPCTSTR url)
{
	m_HelpUrl = url;
	ModifyStyleEx(0, WS_EX_CONTEXTHELP);
}
void CDialogExt::AddToolTip(CWnd *pWnd, LPCTSTR msg)
{
	if ( m_toolTip.GetSafeHwnd() == NULL )
		m_toolTip.Create(this, TTS_ALWAYSTIP | TTS_BALLOON);

    m_toolTip.AddTool(pWnd, msg);
}

BEGIN_MESSAGE_MAP(CDialogExt, CDialog)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_INITMENUPOPUP()
	ON_WM_SETTINGCHANGE()
	ON_WM_NCPAINT()
	ON_WM_NCACTIVATE()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_MESSAGE(WM_INITDIALOG, HandleInitDialog)
	ON_MESSAGE(WM_UAHDRAWMENU, OnUahDrawMenu)
	ON_MESSAGE(WM_UAHDRAWMENUITEM, OnUahDrawMenuItem)
	ON_BN_CLICKED(IDC_HELPBTN, OnBnClickedHelpbtn)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CDialogExt メッセージ ハンドラー

afx_msg HBRUSH CDialogExt::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

#ifdef	USE_DARKMODE
	switch(nCtlColor) {
	case CTLCOLOR_MSGBOX:		// Message box
	case CTLCOLOR_EDIT:			// Edit control
	case CTLCOLOR_LISTBOX:		// List-box control
		hbr = GetAppColorBrush(APPCOL_CTRLFACE);
		pDC->SetTextColor(GetAppColor(APPCOL_CTRLTEXT));
		pDC->SetBkMode(TRANSPARENT);
		break;
	case CTLCOLOR_BTN:			// Button control
	case CTLCOLOR_DLG:			// Dialog box
	case CTLCOLOR_SCROLLBAR:
	case CTLCOLOR_STATIC:		// Static control
		hbr = GetAppColorBrush(m_bBackWindow ? APPCOL_DLGOPTFACE : APPCOL_DLGFACE);
		pDC->SetTextColor(GetAppColor(APPCOL_DLGTEXT));
		pDC->SetBkMode(TRANSPARENT);
		break;
	}
#else
	switch(nCtlColor) {
	case CTLCOLOR_MSGBOX:		// Message box
	case CTLCOLOR_EDIT:			// Edit control
	case CTLCOLOR_LISTBOX:		// List-box control
		break;
	case CTLCOLOR_BTN:			// Button control
	case CTLCOLOR_DLG:			// Dialog box
	case CTLCOLOR_SCROLLBAR:
	case CTLCOLOR_STATIC:		// Static control
		hbr = GetAppColorBrush(m_bBackWindow ? APPCOL_DLGOPTFACE : APPCOL_DLGFACE);
		pDC->SetTextColor(GetAppColor(APPCOL_DLGTEXT));
		pDC->SetBkMode(TRANSPARENT);
		break;
	}
#endif

	return hbr;
}
BOOL CDialogExt::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(rect, GetAppColor(m_bBackWindow ? APPCOL_DLGOPTFACE : APPCOL_DLGFACE));
	return TRUE;
}
void CDialogExt::DrawSystemBar()
{
	if ( GetMenu() == NULL )
		return;

	CRect window, client;
	CDC *pDC = GetWindowDC();

	GetWindowRect(window);
	GetClientRect(client);
	ClientToScreen(client);

	// メニューバー下の線をダークモード対応で塗る
	pDC->FillSolidRect(0, client.top - window.top - 1, window.Width(), 1, GetAppColor(COLOR_BTNSHADOW));

	ReleaseDC(pDC);
}
void CDialogExt::OnNcPaint()
{
	Default();

	if ( bDarkModeSupport )
		DrawSystemBar();
}
BOOL CDialogExt::OnNcActivate(BOOL bActive)
{
	BOOL ret = CDialog::OnNcActivate(bActive);

	if ( bDarkModeSupport )
		DrawSystemBar();

	return ret;
}

afx_msg LRESULT CDialogExt::OnKickIdle(WPARAM wParam, LPARAM lParam)
{
	DWORD d = GetCurrentThreadId();
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();

	return (pApp->m_nThreadID == d ? pApp->OnIdle((LONG)lParam) : FALSE);
}

static BOOL CALLBACK EnumWindowsProc(HWND hWnd , LPARAM lParam)
{
	CWnd *pWnd = CWnd::FromHandle(hWnd);
	CDialogExt *pParent = (CDialogExt *)lParam;
	CRect rect;

	if ( pWnd->GetParent()->GetSafeHwnd() != pParent->GetSafeHwnd() )
		return TRUE;

	pWnd->GetWindowRect(rect);
	pParent->ScreenToClient(rect);

	rect.left   = MulDiv(rect.left,   pParent->m_ZoomMul.cx, pParent->m_ZoomDiv.cx);
	rect.right  = MulDiv(rect.right,  pParent->m_ZoomMul.cx, pParent->m_ZoomDiv.cx);
	rect.top    = MulDiv(rect.top,    pParent->m_ZoomMul.cy, pParent->m_ZoomDiv.cy);
	rect.bottom = MulDiv(rect.bottom, pParent->m_ZoomMul.cy, pParent->m_ZoomDiv.cy);

	if ( pWnd->SendMessage(WM_DPICHANGED, MAKEWPARAM(pParent->m_NowDpi.cx, pParent->m_NowDpi.cy), (LPARAM)((RECT *)rect)) == FALSE ) {
		if ( pParent->m_DpiFont.GetSafeHandle() != NULL )
			pWnd->SetFont(&(pParent->m_DpiFont), FALSE);

		if ( (pParent->GetStyle() & WS_SIZEBOX) == 0 )
			pWnd->MoveWindow(rect, FALSE);
	}

	return TRUE;
}

LRESULT CDialogExt::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	CFont *pFont;
	LOGFONT LogFont;
	CRect NewRect;
	CRect OldRect;
	CRect ReqRect = (RECT *)lParam;

	m_NowDpi.cx = LOWORD(wParam);
	m_NowDpi.cy = HIWORD(wParam);

	GetClientRect(OldRect);
	CheckMoveWindow(ReqRect, FALSE);
	GetClientRect(NewRect);

	m_ZoomMul.cx = NewRect.Width();
	m_ZoomDiv.cx = OldRect.Width();
	m_ZoomMul.cy = NewRect.Height();
	m_ZoomDiv.cy = OldRect.Height();

	if ( (pFont = GetFont()) != NULL ) {
		pFont->GetLogFont(&LogFont);

		if ( m_DpiFont.GetSafeHandle() != NULL )
			m_DpiFont.DeleteObject();

		LogFont.lfHeight = MulDiv(LogFont.lfHeight, m_NowDpi.cy, m_InitDpi.cy);

		m_DpiFont.CreateFontIndirect(&LogFont);
	}

	EnumChildWindows(GetSafeHwnd(), EnumWindowsProc, (LPARAM)this);

	Invalidate(TRUE);

	return TRUE;
}

BOOL CDialogExt::PreTranslateMessage(MSG* pMsg)
{
	int n;
	CShortCutKey *pShortCut;
	CWnd *pWnd = NULL;

	if ( pMsg->message == WM_KEYDOWN ) {
		for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
			pShortCut = &(m_Data[n]);

			if ( pShortCut->m_KeyCode != (UINT)(pMsg->wParam) )
				continue;

			if ( (pShortCut->m_KeyWith & MASK_SHIFT) != 0 && (GetKeyState(VK_SHIFT)   & 0x80) == 0 )
				continue;
			if ( (pShortCut->m_KeyWith & MASK_CTRL)  != 0 && (GetKeyState(VK_CONTROL) & 0x80) == 0 )
				continue;
			if ( (pShortCut->m_KeyWith & MASK_ALT)   != 0 && (GetKeyState(VK_MENU)    & 0x80) == 0 )
				continue;

			if ( pShortCut->m_MsgID != 0 && ((pWnd = GetDlgItem(pShortCut->m_MsgID)) == NULL || pMsg->hwnd != pWnd->GetSafeHwnd()) )
				continue;

			if ( pShortCut->m_CtrlID != 0 ) {	// Control identifier
				if ( (pWnd = GetDlgItem(pShortCut->m_CtrlID)) == NULL )
					continue;
				PostMessage(WM_COMMAND, MAKEWPARAM(pShortCut->m_CtrlID, pShortCut->m_wParam), (LPARAM)(pWnd->GetSafeHwnd()));

			} else								// IDOK or IDCANCEL ...
				PostMessage(WM_COMMAND, pShortCut->m_wParam);

			return TRUE;
		}

	} else if ( pMsg->message >= WM_MOUSEFIRST && WM_MOUSELAST >= pMsg->message && m_toolTip.GetSafeHwnd() != NULL ) {
		m_toolTip.RelayEvent(pMsg);

	} else if ( pMsg->message == WM_NCLBUTTONDOWN && pMsg->wParam == HTHELP ) {
		PostMessage(WM_COMMAND, MAKEWPARAM(IDC_HELPBTN, BN_CLICKED));
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

LRESULT CDialogExt::HandleInitDialog(WPARAM wParam, LPARAM lParam)
{
	CSize dpi;
	CRect rect, client;
	LRESULT result = CDialog::HandleInitDialog(wParam, lParam);

	if ( !m_bReSizeDisable && (GetStyle() & WS_SIZEBOX) != 0 && m_InitDlgRect.GetSize() == 0 )
		DefItemOffset();

	GetActiveDpi(dpi, this, GetParent());

	if ( m_NowDpi.cx != dpi.cx || m_NowDpi.cy != dpi.cy ) {
		GetWindowRect(rect);
		GetClientRect(client);
		rect.right  += (MulDiv(client.Width(),  dpi.cx, m_InitDpi.cx) - client.Width());
		rect.bottom += (MulDiv(client.Height(), dpi.cy, m_InitDpi.cy) - client.Height());
		SendMessage(WM_DPICHANGED, MAKEWPARAM(dpi.cx, dpi.cy), (LPARAM)((RECT *)rect));
	}

	if ( bDarkModeSupport )
		EnumChildWindows(GetSafeHwnd(), EnumSetThemeProc, (LPARAM)this);

	if ( !m_SaveProfile.IsEmpty() )
		LoadSaveDialogSize(FALSE);

	return result;
}

void CDialogExt::SubclassComboBox(int nID)
{
	CWnd *pWnd = GetDlgItem(nID);
	CComboBoxExt *pCombo = new CComboBoxExt;

	if ( pWnd == NULL || pCombo == NULL )
		return;

	pCombo->SubclassWindow(pWnd->GetSafeHwnd());
	m_ComboBoxPtr.Add(pCombo);
}

int CDialogExt::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CDialog::OnCreate(lpCreateStruct) == (-1) )
		return (-1);

	CRect frame, oldClient, newClient;

	GetClientRect(oldClient);

	if ( ExEnableNonClientDpiScaling != NULL ) {
		ExEnableNonClientDpiScaling(GetSafeHwnd());

		GetClientRect(newClient);

		if ( oldClient.Width() != newClient.Width() || oldClient.Height() != newClient.Height() ) {
			// クライアントサイズ基準でウィンドウサイズを調整
			GetWindowRect(frame);
			frame.right  += (oldClient.Width()  - newClient.Width());
			frame.bottom += (oldClient.Height() - newClient.Height());
			MoveWindow(frame, FALSE);
		}
	}

	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), FALSE);

	if ( !m_bGripDisable && (lpCreateStruct->style & WS_SIZEBOX) != 0 )
		m_SizeGrip.Create(this, GRIP_SIZE_CX, GRIP_SIZE_CY);

	m_bDarkMode = ExDwmDarkMode(GetSafeHwnd());

	return 0;
}

void CDialogExt::OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex,BOOL bSysMenu)
{
    ASSERT(pPopupMenu != NULL);
    // Check the enabled state of various menu items.

    CCmdUI state;
    state.m_pMenu = pPopupMenu;
    ASSERT(state.m_pOther == NULL);
    ASSERT(state.m_pParentMenu == NULL);

    // Determine if menu is popup in top-level menu and set m_pOther to
    // it if so (m_pParentMenu == NULL indicates that it is secondary popup).
    HMENU hParentMenu;
    if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
        state.m_pParentMenu = pPopupMenu;    // Parent == child for tracking popup.
    else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
    {
        CWnd* pParent = this;
           // Child windows don't have menus--need to go to the top!
        if (pParent != NULL &&
           (hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
        {
           int nIndexMax = ::GetMenuItemCount(hParentMenu);
           for (int nIndex = 0; nIndex < nIndexMax; nIndex++)
           {
            if (::GetSubMenu(hParentMenu, nIndex) == pPopupMenu->m_hMenu)
            {
                // When popup is found, m_pParentMenu is containing menu.
                state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
                break;
            }
           }
        }
    }

    state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
    for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
      state.m_nIndex++)
    {
        state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
        if (state.m_nID == 0)
           continue; // Menu separator or invalid cmd - ignore it.

        ASSERT(state.m_pOther == NULL);
        ASSERT(state.m_pMenu != NULL);
        if (state.m_nID == (UINT)-1)
        {
           // Possibly a popup menu, route to first item of that popup.
           state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
           if (state.m_pSubMenu == NULL ||
            (state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
            state.m_nID == (UINT)-1)
           {
            continue;       // First item of popup can't be routed to.
           }
           state.DoUpdate(this, TRUE);   // Popups are never auto disabled.
        }
        else
        {
           // Normal menu item.
           // Auto enable/disable if frame window has m_bAutoMenuEnable
           // set and command is _not_ a system command.
           state.m_pSubMenu = NULL;
           state.DoUpdate(this, FALSE);
        }

        // Adjust for menu deletions and additions.
        UINT nCount = pPopupMenu->GetMenuItemCount();
        if (nCount < state.m_nIndexMax)
        {
           state.m_nIndex -= (state.m_nIndexMax - nCount);
           while (state.m_nIndex < nCount &&
            pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
           {
            state.m_nIndex++;
           }
        }
        state.m_nIndexMax = nCount;
    }
}

void CDialogExt::OnSize(UINT nType, int cx, int cy)
{
	if ( nType != SIZE_MINIMIZED ) {
		if ( m_InitDlgRect.GetSize() > 0 ) {
			SetItemOffset(cx, cy);
			Invalidate(TRUE);
		}

		if ( m_SizeGrip.GetSafeHwnd() != NULL )
			m_SizeGrip.ParentReSize(GRIP_SIZE_CX, GRIP_SIZE_CY);
	}

	CDialog::OnSize(nType, cx, cy);
}

void CDialogExt::OnSizing(UINT fwSide, LPRECT pRect)
{
	if ( m_MinSize.cx > 0 && m_MinSize.cy > 0 ) {
		int width  = MulDiv(m_MinSize.cx, m_NowDpi.cx, m_InitDpi.cx);
		int height = MulDiv(m_MinSize.cy, m_NowDpi.cy, m_InitDpi.cy);

		if ( (pRect->right - pRect->left) < width ) {
			if ( fwSide == WMSZ_LEFT || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_BOTTOMLEFT )
				pRect->left = pRect->right - width;
			else
				pRect->right = pRect->left + width;
		}

		if ( (pRect->bottom - pRect->top) < height ) {
			if ( fwSide == WMSZ_TOP || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT )
				pRect->top = pRect->bottom - height;
			else
				pRect->bottom = pRect->top + height;
		}
	}

	if ( m_MaxSize.cx > 0 || m_MaxSize.cy > 0 ) {
		int width  = MulDiv(m_MaxSize.cx, m_NowDpi.cx, m_InitDpi.cx);
		int height = MulDiv(m_MaxSize.cy, m_NowDpi.cy, m_InitDpi.cy);

		if ( m_MaxSize.cx > 0 && (pRect->right - pRect->left) > width ) {
			if ( fwSide == WMSZ_LEFT || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_BOTTOMLEFT )
				pRect->left = pRect->right - width;
			else
				pRect->right = pRect->left + width;
		}

		if ( m_MaxSize.cy > 0 && (pRect->bottom - pRect->top) > height ) {
			if ( fwSide == WMSZ_TOP || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT )
				pRect->top = pRect->bottom - height;
			else
				pRect->bottom = pRect->top + height;
		}
	}

	CDialog::OnSizing(fwSide, pRect);
}

void CDialogExt::OnDestroy()
{
	if ( !m_SaveProfile.IsEmpty() && !IsIconic() && !IsZoomed() )
		LoadSaveDialogSize(TRUE);

	CDialog::OnDestroy();
}

void CDialogExt::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	if ( bDarkModeSupport && lpszSection != NULL && _tcscmp(lpszSection, _T("ImmersiveColorSet")) == 0 ) {
		m_bDarkMode = ExDwmDarkMode(GetSafeHwnd());
		EnumChildWindows(GetSafeHwnd(), EnumSetThemeProc, (LPARAM)this);
		RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
	}

	CDialog::OnSettingChange(uFlags, lpszSection);
}

LRESULT CDialogExt::OnUahDrawMenu(WPARAM wParam, LPARAM lParam)
{
	if ( !bDarkModeSupport )
		return Default();

	UAHMENU *pUahMenu = (UAHMENU *)lParam;
	CDC *pDC = CDC::FromHandle(pUahMenu->hdc);
	MENUBARINFO mbi;
	CRect rect, rcWindow;

	ASSERT(pUahMenu != NULL && pDC != NULL);

	ZeroMemory(&mbi, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	GetMenuBarInfo(GetSafeHwnd(), OBJID_MENU, 0, &mbi);

	GetWindowRect(rcWindow);
	rect = mbi.rcBar;
    rect.OffsetRect(-rcWindow.left, -rcWindow.top);

	pDC->FillSolidRect(rect, GetAppColor(APPCOL_MENUFACE));
	return TRUE;
}
LRESULT CDialogExt::OnUahDrawMenuItem(WPARAM wParam, LPARAM lParam)
{
	if ( !bDarkModeSupport )
		return Default();

	UAHDRAWMENUITEM *pUahDrawMenuItem = (UAHDRAWMENUITEM *)lParam;
	CMenu *pMenu = CMenu::FromHandle(pUahDrawMenuItem->um.hmenu);
	int npos = pUahDrawMenuItem->umi.iPosition;
	UINT state = pUahDrawMenuItem->dis.itemState;
	CDC *pDC = CDC::FromHandle(pUahDrawMenuItem->dis.hDC);
	CRect rect = pUahDrawMenuItem->dis.rcItem;
	DWORD dwFlags = DT_SINGLELINE | DT_VCENTER | DT_CENTER;
	CString title;
	COLORREF TextColor = GetAppColor(APPCOL_MENUTEXT);
	COLORREF BackColor = GetAppColor(APPCOL_MENUFACE);
	int OldBkMode = pDC->SetBkMode(TRANSPARENT);

	ASSERT(pUahDrawMenuItem != NULL && pDC != NULL && pMenu != NULL);
	
	pMenu->GetMenuString(npos, title, MF_BYPOSITION);

	if ( (state & ODS_NOACCEL) != 0 )
        dwFlags |= DT_HIDEPREFIX;

	if ( (state & (ODS_INACTIVE | ODS_GRAYED | ODS_DISABLED)) != 0 )
		TextColor = GetAppColor(COLOR_GRAYTEXT);

	if ( (state & (ODS_HOTLIGHT | ODS_SELECTED)) != 0 )
		BackColor = GetAppColor(APPCOL_MENUHIGH);

	TextColor = pDC->SetTextColor(TextColor);
	pDC->FillSolidRect(rect, BackColor);
	pDC->DrawText(title, rect, dwFlags);

	pDC->SetTextColor(TextColor);
	pDC->SetBkMode(OldBkMode);

	return TRUE;
}
void CDialogExt::OnBnClickedHelpbtn()
{
	CStringLoad url(IDS_OPTIONHELPURL);
	url += m_HelpUrl;
	ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), NULL, url, NULL, NULL, SW_NORMAL);
}
