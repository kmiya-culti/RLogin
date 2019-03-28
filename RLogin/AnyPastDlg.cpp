// AnyPastDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "AnyPastDlg.h"

// CAnyPastDlg ダイアログ

IMPLEMENT_DYNAMIC(CAnyPastDlg, CDialogExt)

CAnyPastDlg::CAnyPastDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CAnyPastDlg::IDD, pParent)
{
	m_EditText.Empty();
	m_NoCheck = FALSE;
	m_bUpdateText = FALSE;
	m_bDelayPast  = FALSE;
	m_bUpdateEnable = FALSE;
	m_CtrlCode[0] = m_CtrlCode[1] = m_CtrlCode[2] = 0;
}

CAnyPastDlg::~CAnyPastDlg()
{
}

void CAnyPastDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ICONBOX, m_IconBox);
	DDX_Check(pDX, IDC_CHECK1, m_NoCheck);
	DDX_Check(pDX, IDC_CHECK2, m_bDelayPast);
	DDX_Check(pDX, IDC_CHECK3, m_bUpdateText);
	DDX_Text(pDX, IDC_CTRLCODE1, m_CtrlCode[0]);
	DDX_Text(pDX, IDC_CTRLCODE2, m_CtrlCode[1]);
	DDX_Text(pDX, IDC_CTRLCODE3, m_CtrlCode[2]);
	DDX_Control(pDX, IDC_TEXTBOX, m_EditWnd);
}

BEGIN_MESSAGE_MAP(CAnyPastDlg, CDialogExt)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_EN_CHANGE(IDC_TEXTBOX, &CAnyPastDlg::OnUpdateEdit)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CTRLCONV1, IDC_CTRLCONV3, &CAnyPastDlg::OnCtrlConv)
	ON_BN_CLICKED(IDC_SHELLESC, &CAnyPastDlg::OnShellesc)
END_MESSAGE_MAP()

static const INITDLGTAB ItemTab[] = {
	{ IDC_SHELLESC,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TEXTBOX,		ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ IDC_CHECK1,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_CHECK2,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_CHECK3,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ 0,	0 },
};

void CAnyPastDlg::CtrlCount()
{
	CWnd *pWnd;

	UpdateData(TRUE);
	m_CtrlCode[0] = m_CtrlCode[1] = m_CtrlCode[2] = 0;

	for ( LPCTSTR p = m_EditText ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('\t') )
			m_CtrlCode[0] += 1;
		else if ( *p == _T('\r') )
			m_CtrlCode[1] += 1;
		else if ( *p != _T('\n') && *p != _T('\x1A') && *p < _T(' ') )
			m_CtrlCode[2] += 1;
	}

	UpdateData(FALSE);

	if ( (pWnd = GetDlgItem(IDC_CTRLCONV1)) != NULL )
		pWnd->EnableWindow(m_CtrlCode[0] == 0 ? FALSE : TRUE);

	if ( (pWnd = GetDlgItem(IDC_CTRLCONV2)) != NULL )
		pWnd->EnableWindow(m_CtrlCode[1] == 0 ? FALSE : TRUE);

	if ( (pWnd = GetDlgItem(IDC_CTRLCONV3)) != NULL )
		pWnd->EnableWindow(m_CtrlCode[2] == 0 ? FALSE : TRUE);

	if ( (pWnd = GetDlgItem(IDC_CHECK3)) != NULL )
		pWnd->EnableWindow(m_bUpdateEnable);
}
BOOL CAnyPastDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	m_IconBox.SetIcon(LoadIcon(NULL, IDI_QUESTION));

	m_bUpdateEnable = FALSE;
	m_EditWnd.SetWindowText(m_EditText);
	CtrlCount();

	CRect rect;
	int cx, cy;

	GetWindowRect(rect);
	m_MinWidth = rect.Width();
	m_MinHeight = rect.Height();

	cx = AfxGetApp()->GetProfileInt(_T("AnyPastDlg"), _T("cx"), rect.Width());
	cy = AfxGetApp()->GetProfileInt(_T("AnyPastDlg"), _T("cy"), rect.Height());

	if ( cx < rect.Width() )
		cx = rect.Width();
	if ( cy < rect.Height() )
		cy = rect.Height();

	MoveWindow(rect.left, rect.top, cx, cy, FALSE);

	AddShortCutKey(0, VK_RETURN, MASK_CTRL, 0, IDOK);

	return TRUE;
}

void CAnyPastDlg::SaveWindowRect()
{
	if ( !IsIconic() ) {
		CRect rect;
		GetWindowRect(rect);
		AfxGetApp()->WriteProfileInt(_T("AnyPastDlg"), _T("cx"), MulDiv(rect.Width(), m_InitDpi.cx, m_NowDpi.cx));
		AfxGetApp()->WriteProfileInt(_T("AnyPastDlg"), _T("cy"), MulDiv(rect.Height(), m_InitDpi.cy, m_NowDpi.cy));
	}
}
void CAnyPastDlg::OnOK()
{
	m_EditWnd.GetWindowText(m_EditText);
	SaveWindowRect();
	CDialogExt::OnOK();
}
void CAnyPastDlg::OnCancel()
{
	SaveWindowRect();
	CDialogExt::OnCancel();
}

void CAnyPastDlg::OnUpdateEdit()
{
	m_bUpdateEnable = TRUE;
	m_EditWnd.GetWindowText(m_EditText);
	CtrlCount();
}

void CAnyPastDlg::OnSize(UINT nType, int cx, int cy)
{
	SetItemOffset(ItemTab, cx, cy);
	CDialogExt::OnSize(nType, cx, cy);
	Invalidate(FALSE);
}

void CAnyPastDlg::OnSizing(UINT fwSide, LPRECT pRect)
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

void CAnyPastDlg::OnCtrlConv(UINT nID)
{
	CString tmp, str;

	m_EditWnd.GetWindowText(tmp);

	for ( LPCTSTR p = tmp ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('\t') )
			str += (nID == IDC_CTRLCONV1 ? _T(' ') :  *p);
		else if ( *p == _T('\r') ) {
			if ( nID == IDC_CTRLCONV2 ) {
				if ( p[1] == _T('\n') )
					p++;
				if ( p[1] != _T('\0') )
					str += _T(' ');
			} else
				str += *p;
		} else if ( *p != _T('\n') && *p != _T('\x1A') && *p < _T(' ') )
			str += (nID == IDC_CTRLCONV3 ? _T(' ') :  *p);
		else
			str += *p;
	}

	m_bUpdateEnable = TRUE;
	m_EditWnd.SetSel(0, -1, FALSE);
	m_EditWnd.ReplaceSel(str, TRUE);
	m_EditWnd.SetFocus();
}

void CAnyPastDlg::OnShellesc()
{
	int st, ed;
	CString tmp, work, str;
	LPCTSTR p;

	m_EditWnd.GetSel(st, ed);

	if ( st == ed )
		return;

	m_EditWnd.GetWindowText(m_EditText);
	tmp = m_EditText.Mid(st, ed - st);

	for ( p = tmp ; *p != _T('\0') ; ) {
		if ( _tcschr(_T(" \"$@&'()^|[]{};*?<>`\\"), *p) != NULL ) {
			str += _T('\\');
			str += *(p++);
		} else if ( *p == _T('\t') ) {
			str += _T("\\t");
			p++;
		} else if ( *p != _T('\r') && *p != _T('\n') && *p != _T('\x1A') && *p < _T(' ') ) {
			work.Format(_T("\\%03o"), *p);
			str += work;
			p++;
		} else {
			str += *(p++);
		}
	}

	m_bUpdateEnable = TRUE;
	m_EditWnd.ReplaceSel(str, TRUE);
	m_EditWnd.SetFocus();
}
