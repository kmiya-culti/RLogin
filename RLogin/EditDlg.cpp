// EditDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "EditDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CEditDlg ダイアログ

IMPLEMENT_DYNAMIC(CEditDlg, CDialogExt)

CEditDlg::CEditDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CEditDlg::IDD, pParent)
{
	m_Edit.Empty();
	m_Title.Empty();
	m_WinText.Empty();
	m_bPassword = FALSE;
}

void CEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_Edit);
	DDX_Text(pDX, IDC_EDIT2, m_Title);
}

BEGIN_MESSAGE_MAP(CEditDlg, CDialogExt)
	ON_WM_SIZE()
	ON_WM_SIZING()
END_MESSAGE_MAP()

static const INITDLGTAB ItemTab[] = {
	{ IDC_EDIT2,		ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ IDC_EDIT1,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ 0,	0 },
};

/////////////////////////////////////////////////////////////////////////////
// CEditDlg メッセージ ハンドラ

BOOL CEditDlg::OnInitDialog()
{
	CRect rect;
	CString text;
	BOOL bUpdate = FALSE;
	CEdit *pWnd;
	CDC *pDC;
	CFont *pFont, *pOld = NULL;
	TEXTMETRIC metric;
	int lines, height = 0;

	CDialogExt::OnInitDialog();

	if ( !m_WinText.IsEmpty() )
		SetWindowText(m_WinText);

	InitItemOffset(ItemTab);

	// CStaticは"\n"だけで改行するがCEditは"\r\n"にする必要がある？

	for ( LPCTSTR p = m_Title ; *p != _T('\0') ; ) {
		if ( p[0] == _T('\r') && p[1] == _T('\n') ) {
			text += *(p++);
			text += *(p++);
		} else if ( p[0] == _T('\n') ) {
			text += _T('\r');
			text += *(p++);
			bUpdate = TRUE;
		} else
			text += *(p++);
	}

	if ( bUpdate )
		m_Title = text;

	if ( m_bPassword ) {
		pWnd = (CEdit *)GetDlgItem(IDC_EDIT1);
		if ( pWnd != NULL ) {
			pWnd->ModifyStyle(0, ES_PASSWORD);
			pWnd->SetPasswordChar(_T('●'));
			pWnd->Invalidate(TRUE);
		}
	}

	UpdateData(FALSE);

	pWnd = (CEdit *)GetDlgItem(IDC_EDIT2);
	if ( pWnd != NULL && (pDC = pWnd->GetDC()) != NULL ) {
		pWnd->GetClientRect(rect);
		if ( (pFont = pWnd->GetFont()) != NULL )
			pOld = pDC->SelectObject(pFont);
		pDC->GetTextMetrics(&metric);
		if ( pOld != NULL )
			pDC->SelectObject(pOld);
		ReleaseDC(pDC);
		if ( (lines = pWnd->GetLineCount() + 1) < 2 )
			lines = 2;
		else if ( lines > 12 ) {
			lines = 12;
			pWnd->ModifyStyle(0, WS_VSCROLL);
		}
		height = metric.tmInternalLeading + (metric.tmHeight + metric.tmExternalLeading) * lines + metric.tmDescent - rect.Height();
	}

	GetWindowRect(rect);
	m_MinWidth  = rect.Width();
	m_MinHeight = rect.Height() + height;

	if ( height != 0 )
		MoveWindow(rect.left, rect.top, m_MinWidth, m_MinHeight, FALSE);

	return TRUE;
}

void CEditDlg::OnSize(UINT nType, int cx, int cy)
{
	SetItemOffset(ItemTab, cx, cy);
	CDialogExt::OnSize(nType, cx, cy);
	Invalidate(FALSE);
}

void CEditDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	int width  = MulDiv(m_MinWidth,  m_NowDpi.cx, m_InitDpi.cx);
	int height = MulDiv(m_MinHeight, m_NowDpi.cy, m_InitDpi.cy);

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


	CDialogExt::OnSizing(fwSide, pRect);
}
