#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "ToolBarEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolBarEx

IMPLEMENT_DYNAMIC(CToolBarEx, CToolBar)

CToolBarEx::CToolBarEx() : CToolBar()
{
	m_bDarkMode = FALSE;
}
CToolBarEx::~CToolBarEx()
{
	RemoveAll();
}

void CToolBarEx::RemoveAll()
{
	for ( int n = 0 ; n < m_DropItem.GetSize() ; n++ ) {
		if ( m_DropItem[n].plist != NULL )
			delete [] m_DropItem[n].plist;
		m_DropItem[n].plist = NULL;
	}

	m_DropItem.RemoveAll();
	m_ItemImage.RemoveAll();
}
void CToolBarEx::InitDropDown()
{
	if ( m_DropItem.GetSize() == 0 ) {
		SendMessage(TB_SETEXTENDEDSTYLE, 0, (LPARAM)0);
		return;
	}

	SendMessage(TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);

	for ( int n = 0 ; n < m_DropItem.GetSize() ; n++ )
		SetButtonStyle(m_DropItem[n].index, TBSTYLE_DROPDOWN);
}
void CToolBarEx::SetDropItem(int index, int item, int count, UINT *plist)
{
	ASSERT(count > 0 && plist != NULL);

	DropItem temp = { index, item, count, new UINT[count] };
	memcpy(temp.plist, plist, sizeof(UINT) * count);
	m_DropItem.Add(temp);
}

static int WordComp(const void *src, const void *dis)
{
	return (int)((WORD)(INT_PTR)src) - (int)(*((WORD *)dis));
}
int CToolBarEx::GetItemImage(int item)
{
	int n;

	if ( BinaryFind((void *)(INT_PTR)item, m_ItemImage.GetData(), sizeof(WORD), (int)m_ItemImage.GetSize(), WordComp, &n) )
		return n;

	return (-1);
}
int CToolBarEx::AddItemImage(int item)
{
	int n;

	if ( item == 0 )
		return (-1);

	if ( BinaryFind((void *)(INT_PTR)item, m_ItemImage.GetData(), sizeof(WORD), (int)m_ItemImage.GetSize(), WordComp, &n) )
		return n;

	m_ItemImage.InsertAt(n, (WORD)item);

	return n;
}
void CToolBarEx::CreateItemImage(int width, int height)
{
	int n, i, max;
	CBitmap Bitmap, ImageMap;
	CImageList ImageList[3];
	CDC SrcDC, DisDC;
	CBitmap *pSrcOld, *pDisOld;
	BITMAP mapinfo;
	CFont font, *pOldFont;
	CString str;
	CSize sz;

	max = (int)m_ItemImage.GetSize();

	// BitmapリソースからImageListを作成
	ImageList[0].Create(width, height, ILC_COLOR24 | ILC_MASK, max, 1);
	ImageList[1].Create(width, height, ILC_COLOR24 | ILC_MASK, max, 1);
	ImageList[2].Create(width, height, ILC_COLOR24 | ILC_MASK, max, 1);

	DisDC.CreateCompatibleDC(NULL);
	SrcDC.CreateCompatibleDC(NULL);

	ImageMap.CreateBitmap(width, height, DisDC.GetDeviceCaps(PLANES), DisDC.GetDeviceCaps(BITSPIXEL), NULL);
	pDisOld = DisDC.SelectObject(&ImageMap);
	
	font.CreateFont(height / 2, 0, 0, 0, 0, FALSE, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
	pOldFont = DisDC.SelectObject(&font);

	for ( n = 0 ; n < max ; n++ ) {
		if ( ((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(m_ItemImage[n]), Bitmap) ) {
			pSrcOld = SrcDC.SelectObject(&Bitmap);
			Bitmap.GetBitmap(&mapinfo);

			for ( i = 0 ; i < 3 ; i++ ) {
				DisDC.FillSolidRect(0, 0, width, height, RGB(192, 192, 192));
				DisDC.TransparentBlt(0, 0, width, height, &SrcDC,
					(mapinfo.bmWidth >= (mapinfo.bmHeight * 3) ? (mapinfo.bmHeight * i) : 0), 0,
					(mapinfo.bmWidth <= mapinfo.bmHeight ? mapinfo.bmWidth : mapinfo.bmHeight), mapinfo.bmHeight, RGB(192, 192, 192));

				DisDC.SelectObject(pDisOld);
				ImageList[i].Add(&ImageMap, RGB(192, 192, 192));
				pDisOld = DisDC.SelectObject(&ImageMap);
			}

			SrcDC.SelectObject(pSrcOld);
			Bitmap.DeleteObject();

		} else {
			((CRLoginApp *)::AfxGetApp())->LoadResString(MAKEINTRESOURCE(m_ItemImage[n]), str);

			if ( str.IsEmpty() )
				str.Format(_T("%4d"), m_ItemImage[n]);

			LPCTSTR p;
			CString line[2];

			if ( (p = _tcsrchr(str, _T('\n'))) != NULL )
				p++;
			else
				p = str;

			if ( *p != _T('\0') )
				line[0] += *(p++);
			if ( *p != _T('\0') )
				line[0] += *(p++);

			if ( *p != _T('\0') )
				line[1] += *(p++);
			if ( *p != _T('\0') )
				line[1] += *(p++);

			for ( i = 0 ; i < 3 ; i++ ) {
				DisDC.SetBkMode(TRANSPARENT);
				DisDC.SetTextColor(RGB(255 - 60 * i, 0, 0));

				DisDC.FillSolidRect(0, 0, width, height, RGB(192, 192, 192));

				sz = DisDC.GetTextExtent(line[0]);
				DisDC.TextOut((width - sz.cx) / 2, (height / 2 - sz.cy) / 2, line[0]);

				sz = DisDC.GetTextExtent(line[1]);
				DisDC.TextOut((width - sz.cx) / 2, (height / 2) + (height / 2 - sz.cy) / 2, line[1]);

				DisDC.SelectObject(pDisOld);
				ImageList[i].Add(&ImageMap, RGB(192, 192, 192));
				pDisOld = DisDC.SelectObject(&ImageMap);
			}
		}
	}

	DisDC.SelectObject(&pOldFont);
	DisDC.SelectObject(pDisOld);
	ImageMap.DeleteObject();

	DisDC.DeleteDC();
	SrcDC.DeleteDC();

	GetToolBarCtrl().SetImageList(&(ImageList[0]));
	GetToolBarCtrl().SetHotImageList(&(ImageList[1]));
	GetToolBarCtrl().SetDisabledImageList(&(ImageList[2]));

	ImageList[0].Detach();
	ImageList[1].Detach();
	ImageList[2].Detach();

	// Image Index set
	max = GetToolBarCtrl().GetButtonCount();
	for ( n = 0 ; n < max ; n++ ) {
		UINT nID;
		UINT nStyle;
		int iImage;

		GetButtonInfo(n, nID, nStyle, iImage);

		if ( nID != 0 && (iImage = GetItemImage(nID)) >= 0 )
			SetButtonInfo(n, nID, nStyle, iImage);
	}
}

void CToolBarEx::DrawBorders(CDC* pDC, CRect& rect)
{
	CControlBarEx::DrawBorders(this, pDC, rect, GetAppColor(APPCOL_BARBACK));
}
BOOL CToolBarEx::DrawThemedGripper(CDC* pDC, const CRect& rect, BOOL fCentered)
{
	CControlBarEx::DrawGripper(this, pDC, rect);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CToolBarEx, CToolBar)
	ON_NOTIFY_REFLECT(TBN_DROPDOWN, &CToolBarEx::OnDropDown)
	ON_WM_CREATE()
	ON_WM_SETCURSOR()
	ON_WM_SETTINGCHANGE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CToolBarEx::OnDropDown(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTOOLBAR pNMToolBar = (LPNMTOOLBAR)pNMHDR;
	CMenu DefMenu, PopUpMenu;
	int ix, id;
	CString str;
	CRect rect;
	UINT nID, nStyle;
	int iImage;
	int x, y;

	*pResult = 0;

	for ( ix = 0 ; ix < m_DropItem.GetSize() ; ix++ ) {
		if ( m_DropItem[ix].item == pNMToolBar->iItem )
			break;
	}

	if ( ix >= m_DropItem.GetSize() || m_DropItem[ix].plist == NULL )
		return;

	if ( !CMenuLoad::GetPopUpMenu(IDR_RLOGINTYPE, DefMenu) || !PopUpMenu.CreatePopupMenu() )
		return;

	for ( int n = 0 ; n < m_DropItem[ix].count ; n++ ) {
		id = m_DropItem[ix].plist[n];
		DefMenu.GetMenuString(id, str, MF_BYCOMMAND);
		PopUpMenu.AppendMenu(MF_STRING, (UINT_PTR)id, str);
	}

	((CMainFrame *)::AfxGetMainWnd())->SetMenuBitmap(&PopUpMenu);

	rect = pNMToolBar->rcButton;
	ClientToScreen(rect);

	x = rect.left;
	y = rect.bottom;

	if ( !IsFloating() ) {
		switch(GetBarStyle() & CBRS_ALIGN_ANY) {
		case CBRS_ALIGN_LEFT:
			x = rect.right;
			y = rect.top;
			break;
		case CBRS_ALIGN_TOP:
			x = rect.left;
			y = rect.bottom;
			break;
		case CBRS_ALIGN_RIGHT:
			x = rect.right;
			y = rect.top;
			break;
		case CBRS_ALIGN_BOTTOM:
			x = rect.left;
			y = rect.bottom;
			break;
		}
	}

	id = ((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(&PopUpMenu, TPM_NONOTIFY | TPM_RETURNCMD, x, y, this, NULL);

	if ( id <= 0 )
		return;

	::AfxGetMainWnd()->PostMessage(WM_COMMAND, (WPARAM)id);

	if ( m_DropItem[ix].item == id )
		return;

	GetButtonInfo(m_DropItem[ix].index, nID, nStyle, iImage);

	m_DropItem[ix].item = id;
	iImage = GetItemImage(id);

	SetButtonInfo(m_DropItem[ix].index, id, nStyle, iImage);

	CString key;
	key.Format(_T("%d-%d"), GetDlgCtrlID(), m_DropItem[ix].index);
	::AfxGetApp()->WriteProfileInt(_T("ToolBarEx"), key, id);
}

int CToolBarEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CToolBar::OnCreate(lpCreateStruct) == (-1) )
		return (-1);

	m_bDarkMode = CControlBarEx::DarkModeCheck(this);

	return 0;
}
BOOL CToolBarEx::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ( pWnd == this  &&  nHitTest == HTCLIENT && CControlBarEx::SetCursor(this, nHitTest, message) )
		return TRUE;

	return CToolBar::OnSetCursor(pWnd, nHitTest, message);
}
void CToolBarEx::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	// CToolBarは、バグっぽいので注意
	if ( !CControlBarEx::SettingChange(this, m_bDarkMode, uFlags, lpszSection) )
		CToolBar::OnSettingChange(uFlags, lpszSection);
}
BOOL CToolBarEx::OnEraseBkgnd(CDC* pDC)
{
	CControlBarEx::EraseBkgnd(this, pDC, GetAppColor(APPCOL_BARBACK));
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CStatusBarEx

IMPLEMENT_DYNAMIC(CStatusBarEx, CStatusBar)

CStatusBarEx::CStatusBarEx() : CStatusBar()
{
	m_bDarkMode = FALSE;
	m_NowDpi.cx = SYSTEM_DPI_X;
	m_NowDpi.cy = SYSTEM_DPI_Y;
	m_lpIDArray = NULL;
	m_nIDCount = 0;
}

BOOL CStatusBarEx::SetIndicators(const UINT* lpIDArray, int nIDCount)
{
	BOOL bRet = CStatusBar::SetIndicators(lpIDArray, nIDCount);

	for ( int n = 0 ; bRet && n < nIDCount ; n++ )
		SetPaneStyle(n, GetPaneStyle(n) | SBT_OWNERDRAW);

	m_lpIDArray = lpIDArray;
	m_nIDCount = nIDCount;

	return bRet;
}
void CStatusBarEx::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect(lpDrawItemStruct->rcItem);
	LPCTSTR str = (LPCTSTR)(lpDrawItemStruct->itemData);
	CFont *pFont = GetFont();

    if ( str == NULL || *str == _T('\0') )
		return;

	if ( pFont != NULL )
		pFont = pDC->SelectObject(pFont);

	pDC->SetTextColor(GetAppColor(APPCOL_BARTEXT));
	pDC->SetBkMode(TRANSPARENT);
	pDC->DrawText(str, (int)_tcslen(str), rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

	if ( pFont != NULL )
		pDC->SelectObject(pFont);
}
void CStatusBarEx::UpdateWidth(int nMul, int nDiv)
{
	int n;
	int cxWidth;
	UINT nID, nStyle;

	for ( n = 0 ; n < GetCount() ; n++ ) {
		GetPaneInfo(n, nID, nStyle, cxWidth);
		SetPaneInfo(n, nID, nStyle, MulDiv(cxWidth, nMul, nDiv));
	}
	UpdateAllPanes(TRUE, FALSE);
}
void CStatusBarEx::DpiChanged()
{
	CFont *pFont;
	LOGFONT LogFont;

	if ( (pFont = GetFont()) == NULL )
		return;

	pFont->GetLogFont(&LogFont);

	if ( m_NewFont.GetSafeHandle() != NULL )
		m_NewFont.DeleteObject();

	LogFont.lfHeight = MulDiv(LogFont.lfHeight, SCREEN_DPI_Y, m_NowDpi.cy);

	m_NewFont.CreateFontIndirect(&LogFont);

	UpdateWidth(SCREEN_DPI_X, m_NowDpi.cx);

	m_NowDpi.cx = SCREEN_DPI_X;
	m_NowDpi.cy = SCREEN_DPI_Y;
}
void CStatusBarEx::FontSizeCheck()
{
	CDC *pDc = GetDC();
	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = ::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9);
	CSize ZoomDiv, ZoomMul;
		
	CDialogExt::GetDlgFontBase(pDc, GetFont(), ZoomDiv);

	if ( m_NewFont.GetSafeHandle() != NULL )
		m_NewFont.DeleteObject();

	if ( !FontName.IsEmpty() || FontSize != 9 )
		m_NewFont.CreatePointFont(MulDiv(FontSize * 10, SCREEN_DPI_Y, SYSTEM_DPI_Y), FontName);

	CDialogExt::GetDlgFontBase(pDc, GetFont(), ZoomMul);
	ReleaseDC(pDc);

	UpdateWidth(ZoomMul.cx, ZoomDiv.cx);

	m_NowDpi.cx = SCREEN_DPI_X;
	m_NowDpi.cy = SCREEN_DPI_Y;
}

BEGIN_MESSAGE_MAP(CStatusBarEx, CStatusBar)
	ON_WM_CREATE()
	ON_WM_SETTINGCHANGE()
	ON_WM_ERASEBKGND()
	ON_MESSAGE(WM_GETFONT, OnGetFont)
END_MESSAGE_MAP()

int CStatusBarEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CStatusBar::OnCreate(lpCreateStruct) == (-1) )
		return (-1);
	
	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = MulDiv(::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9), SCREEN_DPI_Y, SYSTEM_DPI_Y);

	if ( !FontName.IsEmpty() || FontSize != 9 )
		m_NewFont.CreatePointFont(MulDiv(FontSize * 10, SCREEN_DPI_Y, SYSTEM_DPI_Y), FontName);

	m_NowDpi.cx = SCREEN_DPI_X;
	m_NowDpi.cy = SCREEN_DPI_Y;

	m_bDarkMode = CControlBarEx::DarkModeCheck(this);

	return 0;
}
void CStatusBarEx::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CControlBarEx::SettingChange(this, m_bDarkMode, uFlags, lpszSection);
	CStatusBar::OnSettingChange(uFlags, lpszSection);
}
BOOL CStatusBarEx::OnEraseBkgnd(CDC* pDC)
{
	CControlBarEx::EraseBkgnd(this, pDC, GetAppColor(APPCOL_BARBACK));
	return TRUE;
}
LRESULT CStatusBarEx::OnGetFont(WPARAM, LPARAM)
{
	return (m_NewFont.GetSafeHandle() != NULL ? (LRESULT)m_NewFont.GetSafeHandle() : Default());
}