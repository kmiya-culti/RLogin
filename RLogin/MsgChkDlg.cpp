// MsgDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "MsgChkDlg.h"

//////////////////////////////////////////////////////////////////////
// CMsgChkDlg ダイアログ

IMPLEMENT_DYNAMIC(CMsgChkDlg, CDialogExt)

CMsgChkDlg::CMsgChkDlg(CWnd* pParent /*=NULL*/)	: CDialogExt(CMsgChkDlg::IDD, pParent)
{
	m_bNoCheck = FALSE;
	m_bNoChkEnable = FALSE;
	m_nType = MB_ICONQUESTION | MB_YESNO;
	m_BtnRes[0] = IDYES;
	m_BtnRes[1] = IDNO;
	m_BtnRes[2] = IDCLOSE;
	m_pParent = NULL;
}

CMsgChkDlg::~CMsgChkDlg()
{
}

void CMsgChkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BOX1, m_BackWnd);
	DDX_Control(pDX, IDC_ICONBOX, m_IconWnd);
	DDX_Control(pDX, IDC_MSGTEXT, m_MsgWnd);
	DDX_Check(pDX, IDC_NOCHECK, m_bNoCheck);
}

BEGIN_MESSAGE_MAP(CMsgChkDlg, CDialogExt)
	ON_WM_DRAWITEM()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BUTTON1, &CMsgChkDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CMsgChkDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CMsgChkDlg::OnBnClickedButton3)
END_MESSAGE_MAP()

static const INITDLGTAB ItemTab[] = {
	{ IDC_BOX1,			ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_BTM },
	{ IDC_ICONBOX,		ITM_LEFT_LEFT | ITM_RIGHT_LEFT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_MSGTEXT,		ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_BTM },
	{ IDC_NOCHECK,		ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_BUTTON1,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_BUTTON2,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_BUTTON2,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ 0,	0 },
};

//////////////////////////////////////////////////////////////////////
// CMsgChkDlg メッセージ ハンドラー

BOOL CMsgChkDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	WINDOWPLACEMENT place;
	CButton *pButton[4];

	pButton[0] = (CButton *)GetDlgItem(IDC_BUTTON1);
	pButton[1] = (CButton *)GetDlgItem(IDC_BUTTON2);
	pButton[2] = (CButton *)GetDlgItem(IDC_BUTTON3);
	pButton[3] = (CButton *)GetDlgItem(IDC_NOCHECK);

	ASSERT(pButton[0] != NULL && pButton[1] != NULL && pButton[2] != NULL && pButton[3] != NULL);

	switch(m_nType & MB_TYPEMASK) {
	case MB_OK:
		pButton[2]->GetWindowPlacement(&place);
		pButton[0]->SetWindowPlacement(&place);

		pButton[0]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_OK));

		m_BtnRes[0] = IDOK;
		m_BtnRes[1] = IDCLOSE;
		m_BtnRes[2] = IDCLOSE;
		break;

	case MB_OKCANCEL:
		pButton[1]->GetWindowPlacement(&place);
		pButton[0]->SetWindowPlacement(&place);
		pButton[2]->GetWindowPlacement(&place);
		pButton[1]->SetWindowPlacement(&place);

		pButton[0]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_OK));
		pButton[1]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_CANCEL));

		m_BtnRes[0] = IDOK;
		m_BtnRes[1] = IDCANCEL;
		m_BtnRes[2] = IDCLOSE;
		break;

	case MB_ABORTRETRYIGNORE:
		pButton[0]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_ABORT));
		pButton[1]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_RETRY));
		pButton[2]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_IGNORE));

		m_BtnRes[0] = IDABORT;
		m_BtnRes[1] = IDRETRY;
		m_BtnRes[2] = IDIGNORE;
		break;

	case MB_YESNOCANCEL:
		pButton[0]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_YES));
		pButton[1]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_NO));
		pButton[2]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_CANCEL));

		m_BtnRes[0] = IDYES;
		m_BtnRes[1] = IDNO;
		m_BtnRes[2] = IDCANCEL;
		break;

	case MB_YESNO:
		pButton[1]->GetWindowPlacement(&place);
		pButton[0]->SetWindowPlacement(&place);
		pButton[2]->GetWindowPlacement(&place);
		pButton[1]->SetWindowPlacement(&place);

		pButton[0]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_YES));
		pButton[1]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_NO));

		m_BtnRes[0] = IDYES;
		m_BtnRes[1] = IDNO;
		m_BtnRes[2] = IDCLOSE;
		break;

	case MB_RETRYCANCEL:
		pButton[1]->GetWindowPlacement(&place);
		pButton[0]->SetWindowPlacement(&place);
		pButton[2]->GetWindowPlacement(&place);
		pButton[1]->SetWindowPlacement(&place);

		pButton[0]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_RETRY));
		pButton[1]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_CANCEL));

		m_BtnRes[0] = IDRETRY;
		m_BtnRes[1] = IDCANCEL;
		m_BtnRes[2] = IDCLOSE;
		break;

	case MB_CANCELTRYCONTINUE:
		pButton[0]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_CANCEL));
		pButton[1]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_TRYAGAIN));
		pButton[2]->SetWindowText(CStringLoad(IDS_MESSAGEBOX_CONTINUE));

		m_BtnRes[0] = IDCANCEL;
		m_BtnRes[1] = IDTRYAGAIN;
		m_BtnRes[2] = IDCONTINUE;
		break;
	}

	switch(m_nType & MB_DEFMASK) {
	case MB_DEFBUTTON2:
		pButton[0]->SetButtonStyle(pButton[0]->GetButtonStyle() & ~BS_DEFPUSHBUTTON, FALSE);
		pButton[1]->SetButtonStyle(pButton[1]->GetButtonStyle() | BS_DEFPUSHBUTTON, FALSE);
	case MB_DEFBUTTON3:
		pButton[0]->SetButtonStyle(pButton[0]->GetButtonStyle() & ~BS_DEFPUSHBUTTON, FALSE);
		pButton[2]->SetButtonStyle(pButton[2]->GetButtonStyle() | BS_DEFPUSHBUTTON, FALSE);
		break;
	}

	switch(m_nType & MB_ICONMASK) {
	case MB_ICONHAND:
		m_IconWnd.SetIcon(LoadIcon(NULL, IDI_HAND));
		MessageBeep(MB_ICONHAND);
		break;
	case MB_ICONQUESTION:
		m_IconWnd.SetIcon(LoadIcon(NULL, IDI_QUESTION));
		MessageBeep(MB_ICONQUESTION);
		break;
	case MB_ICONEXCLAMATION:
		m_IconWnd.SetIcon(LoadIcon(NULL, IDI_EXCLAMATION));
		MessageBeep(MB_ICONEXCLAMATION);
		break;
	case MB_ICONASTERISK:
		m_IconWnd.SetIcon(LoadIcon(NULL, IDI_ASTERISK));
		MessageBeep(MB_ICONASTERISK);
		break;
	}

	int mdx = 0;
	CDC *pDC = m_MsgWnd.GetDC();
	CFont *pOld = pDC->SelectObject(m_MsgWnd.GetFont());
	CRect rect, txrt;
	CSize sz;
	CString tmp;
	CStringArray line;

	for ( LPCTSTR p = m_MsgText ; *p != _T('\0') ; ) {
		if ( p[0] == _T('\r') && p[1] == _T('\n') ) {
			p += 2;
			line.Add(tmp);
			tmp.Empty();
		} else if ( *p == _T('\n') ) {
			p += 1;
			line.Add(tmp);
			tmp.Empty();
		} else
			tmp += *(p++);
	}
	if ( !tmp.IsEmpty() )
		line.Add(tmp);

	m_MsgWnd.GetClientRect(rect);

	for ( int n = 0 ; n < line.GetSize() ; n++ ) {
		sz = pDC->GetTextExtent(line[n]);
		if ( sz.cx < rect.Width() )
			continue;
		int b = 0;
		int m = line[n].GetLength();
		while ( b <= m ) {
			int i = (b + m) / 2;
			sz = pDC->GetTextExtent(line[n].Left(i));
			if ( sz.cx <= rect.Width() )
				b = i + 1;
			else
				m = i - 1;
		}
		tmp = line[n].Right(line[n].GetLength() - m);
		line.InsertAt(n, line[n].Left(m));
		line[n + 1] = tmp;
	}

	tmp.Empty();
	for ( int n = 0 ; n < line.GetSize() ; n++ ) {
		tmp += line[n];
		tmp += _T('\n');
	}

	if ( line.GetSize() <= 1 )
		m_MsgWnd.ModifyStyle(0, SS_CENTERIMAGE);

	if ( !m_bNoChkEnable ) {
		pButton[0]->GetWindowRect(txrt);
		ScreenToClient(txrt);
		mdx = txrt.left - MulDiv(12, m_NowDpi.cx, 96);
	}

	txrt = rect;
	pDC->DrawText(tmp, txrt, DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL);

	pDC->SelectObject(pOld);
	m_MsgWnd.ReleaseDC(pDC);

	sz.SetSize(0, 0);
	if ( txrt.Width() < rect.Width() && (sz.cx = rect.Width() - txrt.Width()) > mdx )
		sz.cx = mdx;
	if ( txrt.Height() > rect.Height() )
		sz.cy = txrt.Height() - rect.Height();

	InitItemOffset(ItemTab);

	if ( sz.cx > 2 || sz.cy > 0 ) {
		GetWindowRect(rect);
		m_MinSize.cx = rect.Width()  - sz.cx;
		m_MinSize.cy = rect.Height() + sz.cy;
		SetWindowPos(NULL, 0, 0, m_MinSize.cx, m_MinSize.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
	}

	m_MsgWnd.SetWindowText(m_MsgText);

	if ( m_Title.IsEmpty() )
		m_Title = theApp.m_pszAppName;
	SetWindowText(m_Title);

	GetWindowRect(rect);
	sz.cx = rect.Width();
	sz.cy = rect.Height();

	if ( m_pParent ==  NULL )
		m_pParent = ::AfxGetMainWnd();

	if ( m_pParent !=  NULL ) {
		m_pParent->GetWindowRect(rect);
		int sx = rect.left + (rect.Width()  - sz.cx) / 2;
		int sy = rect.top  + (rect.Height() - sz.cy) / 2;
		SetWindowPos(NULL, sx, sy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);
	}

	if ( !m_bNoChkEnable )
		pButton[3]->ShowWindow(SW_HIDE);

	if ( m_BtnRes[1] == IDCLOSE ) 
		pButton[1]->ShowWindow(SW_HIDE);

	if ( m_BtnRes[2] == IDCLOSE ) 
		pButton[2]->ShowWindow(SW_HIDE);

	return TRUE;
}

void CMsgChkDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect;

	if ( nIDCtl >= IDC_BOX1 ) {
		m_BackWnd.GetClientRect(rect);
		pDC->FillSolidRect(rect, GetAppColor(COLOR_WINDOW));
	} else
		CDialogExt::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
HBRUSH CMsgChkDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogExt::OnCtlColor(pDC, pWnd, nCtlColor);

	switch(pWnd->GetDlgCtrlID()) {
	case IDC_MSGTEXT:
		pDC->SetTextColor(GetAppColor(COLOR_WINDOWTEXT));
		pDC->SetBkColor(GetAppColor(COLOR_WINDOW));
	case IDC_ICONBOX:
		hbr = GetAppColorBrush(COLOR_WINDOW);
		break;
	}

	return hbr;
}

void CMsgChkDlg::OnCancel()
{
	UpdateData(TRUE);
	EndDialog(IDCLOSE);
}
void CMsgChkDlg::OnBnClickedButton1()
{
	UpdateData(TRUE);
	EndDialog(m_BtnRes[0]);
}
void CMsgChkDlg::OnBnClickedButton2()
{
	UpdateData(TRUE);
	EndDialog(m_BtnRes[1]);
}
void CMsgChkDlg::OnBnClickedButton3()
{
	UpdateData(TRUE);
	EndDialog(m_BtnRes[2]);
}
