// MsgDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
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
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CMsgChkDlg メッセージ ハンドラー

BOOL CMsgChkDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	LOGFONT logfont;
	CFont *pFont = m_MsgWnd.GetFont();
	pFont->GetLogFont(&logfont);

	if ( m_MsgFont.CreatePointFont(11 * 10, logfont.lfFaceName) )
		m_MsgWnd.SetFont(&m_MsgFont);

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
