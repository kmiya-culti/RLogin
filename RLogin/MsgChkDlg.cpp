// MsgDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "MsgChkDlg.h"

//////////////////////////////////////////////////////////////////////
// CMsgChkDlg ダイアログ

IMPLEMENT_DYNAMIC(CMsgChkDlg, CDialogExt)

CMsgChkDlg::CMsgChkDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CMsgChkDlg::IDD, pParent)
{
	m_bNoCheck = FALSE;
}

CMsgChkDlg::~CMsgChkDlg()
{
}

void CMsgChkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ICONBOX, m_IconWnd);
	DDX_Control(pDX, IDC_MSGTEXT, m_MsgWnd);
	DDX_Check(pDX, IDC_NOCHECK, m_bNoCheck);
}

BEGIN_MESSAGE_MAP(CMsgChkDlg, CDialogExt)
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CMsgChkDlg メッセージ ハンドラー

BOOL CMsgChkDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	CFont *pFont;
	LOGFONT logfont;
	
	if ( (pFont = m_MsgWnd.GetFont()) != NULL ) {
		pFont->GetLogFont(&logfont);

		logfont.lfWidth  = logfont.lfWidth  * 11 / 9;
		logfont.lfHeight = logfont.lfHeight * 11 / 9;

		if ( m_MsgFont.CreateFontIndirect(&logfont) )
			m_MsgWnd.SetFont(&m_MsgFont);
	}

	m_BkBrush.CreateSolidBrush(RGB(255, 255, 255));

	m_IconWnd.SetIcon(LoadIcon(NULL, IDI_QUESTION));
	m_MsgWnd.SetWindowText(m_MsgText);
	SetWindowText(m_Title);

	return TRUE;
}

void CMsgChkDlg::OnCancel()
{
	UpdateData(TRUE);

	CDialogExt::OnCancel();
}


HBRUSH CMsgChkDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogExt::OnCtlColor(pDC, pWnd, nCtlColor);

	switch(pWnd->GetDlgCtrlID()) {
	case IDC_MSGTEXT:
		pDC->SetBkColor(RGB(255, 255, 255));
	case IDC_ICONBOX:
		hbr = m_BkBrush;
		break;
	}

	return hbr;
}

LRESULT CMsgChkDlg::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = CDialogExt::OnDpiChanged(wParam, lParam);

	CFont *pFont;
	LOGFONT logfont;
	
	if ( (pFont = m_MsgWnd.GetFont()) != NULL ) {
		pFont->GetLogFont(&logfont);

		if ( pFont->GetSafeHandle() != m_MsgFont.GetSafeHandle() ) {
			logfont.lfWidth  = logfont.lfWidth  * 11 / 9;
			logfont.lfHeight = logfont.lfHeight * 11 / 9;
		}

		if ( m_MsgFont.GetSafeHandle() != NULL )
			m_MsgFont.DeleteObject();

		if ( m_MsgFont.CreateFontIndirect(&logfont) )
			m_MsgWnd.SetFont(&m_MsgFont);

		Invalidate(TRUE);
	}

	return result;
}