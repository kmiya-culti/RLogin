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
// CDialogExt ダイアログ

IMPLEMENT_DYNAMIC(CDialogExt, CDialog)

CDialogExt::CDialogExt(UINT nIDTemplate, CWnd *pParent)
	: CDialog(nIDTemplate, pParent)
{
	m_nIDTemplate = nIDTemplate;
	
	m_FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	m_FontSize = ::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9);

	m_InitDpi.cx = SYSTEM_DPI_X;
	m_InitDpi.cy = SYSTEM_DPI_Y;
	m_NowDpi = m_InitDpi;

	m_InitDlgRect.RemoveAll();
	m_MinSize.SetSize(0, 0);
	m_OfsSize.SetSize(0, 0);
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

void CDialogExt::InitItemOffset(const INITDLGTAB *pTab, int ox, int oy, int mx, int my)
{
	int n;
	int cx, cy;
	CRect frame, rect;
	WINDOWPLACEMENT place;
	CWnd *pWnd;
	INITDLGRECT dlgtmp;

	GetWindowRect(rect);
	if ( mx <= 0 )
		mx = rect.Width();
	if ( my <= 0 )
		my = rect.Height();

	m_MinSize.SetSize(mx, my);
	m_OfsSize.SetSize(ox, oy);

	GetClientRect(frame);			// frame.left == 0, frame.top == 0 
	cx = frame.Width();
	cy = frame.Height();

	m_InitDlgRect.RemoveAll();

	for ( n = 0 ; pTab[n].id != 0 ; n++ ) {
		rect.SetRectEmpty();

		if ( (pWnd = GetDlgItem(pTab[n].id)) != NULL ) {
			pWnd->GetWindowPlacement(&place);

			if ( pTab[n].mode & ITM_LEFT_MID )
				rect.left = place.rcNormalPosition.left - cx / 2;
			else if ( pTab[n].mode & ITM_LEFT_RIGHT )
				rect.left = place.rcNormalPosition.left - cx;
			else
				rect.left = place.rcNormalPosition.left;

			if ( pTab[n].mode & ITM_RIGHT_MID )
				rect.right = place.rcNormalPosition.right - cx/ 2;
			else if ( pTab[n].mode & ITM_RIGHT_RIGHT )
				rect.right = place.rcNormalPosition.right - cx;
			else
				rect.right = place.rcNormalPosition.right;

			if ( pTab[n].mode & ITM_TOP_MID )
				rect.top = place.rcNormalPosition.top - cy / 2;
			else if ( pTab[n].mode & ITM_TOP_BTM )
				rect.top = place.rcNormalPosition.top - cy;
			else
				rect.top = place.rcNormalPosition.top;

			if ( pTab[n].mode & ITM_BTM_MID )
				rect.bottom = place.rcNormalPosition.bottom - cy / 2;
			else if ( pTab[n].mode & ITM_BTM_BTM )
				rect.bottom = place.rcNormalPosition.bottom - cy;
			else
				rect.bottom = place.rcNormalPosition.bottom;
		}

		dlgtmp.id   = pTab[n].id;
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

		if ( (pWnd = GetDlgItem(m_InitDlgRect[n].id)) == NULL )
			continue;

		pWnd->GetWindowPlacement(&place);

		i = m_InitDlgRect[n].rect.left * m_NowDpi.cx / m_InitDpi.cx;

		if ( m_InitDlgRect[n].mode & ITM_LEFT_MID )
			place.rcNormalPosition.left = i + cx / 2;
		else if ( m_InitDlgRect[n].mode & ITM_LEFT_RIGHT )
			place.rcNormalPosition.left = i + cx;
		else
			place.rcNormalPosition.left = i + m_OfsSize.cx;

		i = m_InitDlgRect[n].rect.right * m_NowDpi.cx / m_InitDpi.cx;

		if ( m_InitDlgRect[n].mode & ITM_RIGHT_MID )
			place.rcNormalPosition.right = i + cx / 2;
		else if ( m_InitDlgRect[n].mode & ITM_RIGHT_RIGHT )
			place.rcNormalPosition.right = i + cx;
		else
			place.rcNormalPosition.right = i + m_OfsSize.cx;

		i = m_InitDlgRect[n].rect.top * m_NowDpi.cy / m_InitDpi.cy;

		if ( m_InitDlgRect[n].mode & ITM_TOP_MID )
			place.rcNormalPosition.top = i + cy / 2;
		else if ( m_InitDlgRect[n].mode & ITM_TOP_BTM )
			place.rcNormalPosition.top = i + cy;
		else
			place.rcNormalPosition.top = i + m_OfsSize.cy;

		i = m_InitDlgRect[n].rect.bottom * m_NowDpi.cy / m_InitDpi.cy;

		if ( m_InitDlgRect[n].mode & ITM_BTM_MID )
			place.rcNormalPosition.bottom = i + cy / 2;
		else if ( m_InitDlgRect[n].mode & ITM_BTM_BTM )
			place.rcNormalPosition.bottom = i + cy;
		else
			place.rcNormalPosition.bottom = i + m_OfsSize.cy;

		pWnd->SetWindowPlacement(&place);
	}
}

void CDialogExt::SetBackColor(COLORREF color)
{
	m_BkBrush.CreateSolidBrush(color);
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

#if 1
	// モニターを基準に調整

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
#else
	// 仮想画面サイズを基準に調整

	if ( rect.left < mi.rcWork.left ) {
		rect.right += (mi.rcWork.left - rect.left);
		rect.left  += (mi.rcWork.left - rect.left);
	}

	if ( rect.top < mi.rcWork.top ) {
		rect.bottom += (mi.rcWork.top - rect.top);
		rect.top    += (mi.rcWork.top - rect.top);
	}

	if ( rect.right > mi.rcWork.right ) {
		rect.left  -= (rect.right - mi.rcWork.right);
		rect.right -= (rect.right - mi.rcWork.right);
		if ( rect.left < mi.rcWork.left ) {
			rect.left  = mi.rcWork.left;
			rect.right = mi.rcWork.right;
		}
	}

	if ( rect.bottom > mi.rcWork.bottom ) {
		rect.top    -= (rect.bottom - mi.rcWork.bottom);
		rect.bottom -= (rect.bottom - mi.rcWork.bottom);
		if ( rect.top < mi.rcWork.top ) {
			rect.top    = mi.rcWork.top;
			rect.bottom = mi.rcWork.bottom;
		}
	}
#endif

	MoveWindow(rect, bRepaint);
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

	if ( IsDefineFont() )
		dlgTemp.SetFont(m_FontName, m_FontSize);
	//else {
	//	CString name;
	//	WORD size;
	//	dlgTemp.GetFont(name, size);
	//	if ( m_FontSize != size )
	//		dlgTemp.SetFont(name, m_FontSize);
	//}

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

	if ( IsDefineFont() )
		dlgTemp.SetFont(m_FontName, m_FontSize);
	//else {
	//	CString name;
	//	WORD size;
	//	dlgTemp.GetFont(name, size);
	//	if ( m_FontSize != size )
	//		dlgTemp.SetFont(name, m_FontSize);
	//}

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

BEGIN_MESSAGE_MAP(CDialogExt, CDialog)
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_MESSAGE(WM_INITDIALOG, HandleInitDialog)
	ON_WM_CREATE()
	ON_WM_INITMENUPOPUP()
	ON_WM_SIZE()
	ON_WM_SIZING()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CDialogExt メッセージ ハンドラー

afx_msg HBRUSH CDialogExt::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	if ( m_BkBrush.m_hObject == NULL )
		return hbr;

	switch(nCtlColor) {
	case CTLCOLOR_BTN:			// Button control
	case CTLCOLOR_SCROLLBAR:	// Scroll-bar control
	case CTLCOLOR_LISTBOX:		// List-box control
	case CTLCOLOR_MSGBOX:		// Message box
	case CTLCOLOR_EDIT:			// Edit control
		break;

	case CTLCOLOR_DLG:			// Dialog box
		hbr = m_BkBrush;
		break;

	case CTLCOLOR_STATIC:		// Static control
		hbr = m_BkBrush;
		pDC->SetBkMode(TRANSPARENT);
//		pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
		break;
	}

	return hbr;
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
	MoveWindow(ReqRect, FALSE);
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
	}

	return CDialog::PreTranslateMessage(pMsg);
}

LRESULT CDialogExt::HandleInitDialog(WPARAM wParam, LPARAM lParam)
{
	CSize dpi;
	LRESULT result = CDialog::HandleInitDialog(wParam, lParam);

	GetActiveDpi(dpi, this, GetParent());

	if ( m_NowDpi.cx != dpi.cx || m_NowDpi.cy != dpi.cy ) {
		CRect rect, client;
		GetWindowRect(rect);
		GetClientRect(client);
		rect.right  += (MulDiv(client.Width(),  dpi.cx, m_InitDpi.cx) - client.Width());
		rect.bottom += (MulDiv(client.Height(), dpi.cy, m_InitDpi.cy) - client.Height());
		SendMessage(WM_DPICHANGED, MAKEWPARAM(dpi.cx, dpi.cy), (LPARAM)((RECT *)rect));
	}

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
	if ( m_InitDlgRect.GetSize() > 0 && nType != SIZE_MINIMIZED ) {
		SetItemOffset(cx, cy);
		Invalidate(FALSE);
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

	CDialog::OnSizing(fwSide, pRect);
}
