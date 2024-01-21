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
#ifdef	_UNICODE
			pWnd->SetPasswordChar(L'\u25CF');	// _T('●'));
#else
			pWnd->SetPasswordChar('*');
#endif
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

	if ( height != 0 ) {
		GetWindowRect(rect);
		m_MinSize.cx = rect.Width();
		m_MinSize.cy = rect.Height() + height;

		MoveWindow(rect.left, rect.top, m_MinSize.cx, m_MinSize.cy, FALSE);
	}

	return TRUE;
}
