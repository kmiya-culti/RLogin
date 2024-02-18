//////////////////////////////////////////////////////////////////////
// ExtFileDialog.cpp
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "FileUpDown.h"
#include "ExtFileDialog.h"

//////////////////////////////////////////////////////////////////////
// CDialogRes 

IMPLEMENT_DYNAMIC(CDialogRes, CDialogExt)

CDialogRes::CDialogRes(UINT nIDTemplate, CWnd *pParent)	: CDialogExt(nIDTemplate, pParent)
{
}
CDialogRes::~CDialogRes()
{
}

BEGIN_MESSAGE_MAP(CDialogRes, CDialogExt)
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_SETTINGCHANGE()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

HBRUSH CDialogRes::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}
BOOL CDialogRes::OnEraseBkgnd(CDC* pDC)
{
	return CDialog::OnEraseBkgnd(pDC);
}
void CDialogRes::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDialog::OnSettingChange(uFlags, lpszSection);
}
LRESULT CDialogRes::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// CFileDownPage

IMPLEMENT_DYNAMIC(CFileDownPage, CDialogRes)

CFileDownPage::CFileDownPage() : CDialogRes(CFileDownPage::IDD)
{
	m_pUpDown = NULL;

	m_DownMode = 0;
	m_DownFrom.Empty();
	m_DownTo.Empty();
	m_DecMode = 0;
	m_bDownWait = FALSE;
	m_DownSec = 0;
	m_bDownCrLf = FALSE;
	m_DownCrLfMode = 0;
	m_bWithEcho = FALSE;
	m_bFileAppend = FALSE;
}
CFileDownPage::~CFileDownPage()
{
}
void CFileDownPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogRes::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO1, m_DownMode);
	DDX_Check(pDX, IDC_CHECK1, m_bDownCrLf);
	DDX_Check(pDX, IDC_CHECK2, m_bDownWait);
	DDX_Check(pDX, IDC_CHECK3, m_bWithEcho);
	DDX_Check(pDX, IDC_CHECK4, m_bFileAppend);
	DDX_CBStringExact(pDX, IDC_COMBO1, m_DownFrom);
	DDX_CBStringExact(pDX, IDC_COMBO2, m_DownTo);
	DDX_CBIndex(pDX, IDC_COMBO3, m_DecMode);
	DDX_CBIndex(pDX, IDC_COMBO4, m_DownCrLfMode);
	DDX_Text(pDX, IDC_EDIT1, m_DownSec);
}

BEGIN_MESSAGE_MAP(CFileDownPage, CDialogRes)
END_MESSAGE_MAP()

static const INITDLGTAB DownItemTab[] = {
	{ IDC_RADIO1,	ITM_RIGHT_RIGHT },
	{ IDC_RADIO2,	ITM_RIGHT_RIGHT },
	{ IDC_RADIO3,	ITM_RIGHT_RIGHT },

	{ IDC_CHECK1,	ITM_RIGHT_RIGHT },
	{ IDC_CHECK2,	ITM_RIGHT_RIGHT },
	{ IDC_CHECK3,	ITM_RIGHT_RIGHT },
	{ IDC_CHECK4,	ITM_RIGHT_RIGHT },

	{ IDC_COMBO1,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_COMBO2,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_COMBO3,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_COMBO4,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_EDIT1,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },

	{ 0,	0 },
};

BOOL CFileDownPage::OnInitDialog()
{
	int n;
	CComboBox *pCombo;
	CStringArray stra;

	CDialogRes::OnInitDialog();

	InitItemOffset(DownItemTab);

	CIConv::SetListArray(stra);
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL ) {
		for ( n = 0 ; n < stra.GetSize() ; n++ )
			pCombo->AddString(stra[n]);
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL ) {
		for ( n = 0 ; n < stra.GetSize() ; n++ )
			pCombo->AddString(stra[n]);
	}

	m_DownMode     = m_pUpDown->m_DownMode;
	m_DownFrom     = m_pUpDown->m_DownFrom;
	m_DownTo       = m_pUpDown->m_DownTo;
	m_DecMode      = m_pUpDown->m_DecMode;
	m_bDownCrLf    = m_pUpDown->m_bDownCrLf;
	m_DownCrLfMode = m_pUpDown->m_DownCrLfMode;
	m_bDownWait    = m_pUpDown->m_bDownWait;
	m_DownSec      = m_pUpDown->m_DownSec;
	m_bWithEcho    = m_pUpDown->m_bWithEcho;
	m_bFileAppend  = m_pUpDown->m_bFileAppend;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CFileDownPage::OnApply()
{
	UpdateData(TRUE);

	m_pUpDown->m_DownMode     = m_DownMode;
	m_pUpDown->m_DownFrom     = m_DownFrom;
	m_pUpDown->m_DownTo       = m_DownTo;
	m_pUpDown->m_DecMode      = m_DecMode;
	m_pUpDown->m_bDownCrLf    = m_bDownCrLf;
	m_pUpDown->m_DownCrLfMode = m_DownCrLfMode;
	m_pUpDown->m_bDownWait    = m_bDownWait;
	m_pUpDown->m_DownSec      = m_DownSec;
	m_pUpDown->m_bWithEcho    = m_bWithEcho;
	m_pUpDown->m_bFileAppend  = m_bFileAppend;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CFileUpConvPage

IMPLEMENT_DYNAMIC(CFileUpConvPage, CDialogRes)

CFileUpConvPage::CFileUpConvPage() : CDialogRes(CFileUpConvPage::IDD)
{
	m_pUpDown = NULL;

	m_UpMode = 0;
	m_UpCrLf = FALSE;
	m_UpFrom.Empty();
	m_UpTo.Empty();
	m_EncMode = 0;
	m_UpCrLfMode = 0;
}
CFileUpConvPage::~CFileUpConvPage()
{
}
void CFileUpConvPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogRes::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO1, m_UpMode);
	DDX_Check(pDX, IDC_CHECK1, m_UpCrLf);
	DDX_CBStringExact(pDX, IDC_COMBO1, m_UpFrom);
	DDX_CBStringExact(pDX, IDC_COMBO2, m_UpTo);
	DDX_CBIndex(pDX, IDC_COMBO3, m_EncMode);
	DDX_CBIndex(pDX, IDC_COMBO4, m_UpCrLfMode);
}

BEGIN_MESSAGE_MAP(CFileUpConvPage, CDialogRes)
END_MESSAGE_MAP()

static const INITDLGTAB UpConvItemTab[] = {
	{ IDC_RADIO1,	ITM_RIGHT_RIGHT },
	{ IDC_RADIO2,	ITM_RIGHT_RIGHT },
	{ IDC_RADIO3,	ITM_RIGHT_RIGHT },

	{ IDC_CHECK1,	ITM_RIGHT_RIGHT },

	{ IDC_COMBO1,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_COMBO2,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_COMBO3,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_COMBO4,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },

	{ 0,	0 },
};

BOOL CFileUpConvPage::OnInitDialog()
{
	int n;
	CComboBox *pCombo;
	CStringArray stra;

	CDialogRes::OnInitDialog();

	InitItemOffset(UpConvItemTab);

	CIConv::SetListArray(stra);
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL ) {
		for ( n = 0 ; n < stra.GetSize() ; n++ )
			pCombo->AddString(stra[n]);
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL ) {
		for ( n = 0 ; n < stra.GetSize() ; n++ )
			pCombo->AddString(stra[n]);
	}

	m_UpMode     = m_pUpDown->m_UpMode;
	m_UpCrLf     = m_pUpDown->m_bUpCrLf;
	m_UpFrom     = m_pUpDown->m_UpFrom;
	m_UpTo       = m_pUpDown->m_UpTo;
	m_EncMode    = m_pUpDown->m_EncMode;
	m_UpCrLfMode = m_pUpDown->m_UpCrLfMode;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CFileUpConvPage::OnApply()
{
	UpdateData(TRUE);

	m_pUpDown->m_UpMode     = m_UpMode;
	m_pUpDown->m_bUpCrLf    = m_UpCrLf;
	m_pUpDown->m_UpFrom     = m_UpFrom;
	m_pUpDown->m_UpTo       = m_UpTo;
	m_pUpDown->m_EncMode    = m_EncMode;
	m_pUpDown->m_UpCrLfMode = m_UpCrLfMode;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CFileUpSendPage

IMPLEMENT_DYNAMIC(CFileUpSendPage, CDialogRes)

CFileUpSendPage::CFileUpSendPage() : CDialogRes(CFileUpSendPage::IDD)
{
	m_pUpDown = NULL;

	m_SendMode = 0;
	m_RecvWait = FALSE;
	m_CrLfWait = FALSE;
	m_XonXoff  = FALSE;
	m_RecvEcho = FALSE;

	m_BlockSize = 0;
	m_BlockMsec = 0;
	m_CharUsec  = 0.0;
	m_LineMsec  = 0;
	m_RecvMsec  = 0;
	m_CrLfMsec  = 0;
}
CFileUpSendPage::~CFileUpSendPage()
{
}
void CFileUpSendPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogRes::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO4, m_SendMode);
	DDX_Check(pDX, IDC_CHECK2, m_RecvWait);
	DDX_Check(pDX, IDC_CHECK3, m_CrLfWait);
	DDX_Check(pDX, IDC_CHECK4, m_XonXoff);
	DDX_Check(pDX, IDC_CHECK5, m_RecvEcho);
	DDX_Text(pDX, IDC_EDIT1, m_BlockSize);
	DDX_Text(pDX, IDC_EDIT2, m_BlockMsec);
	DDX_Text(pDX, IDC_EDIT3, m_CharUsec);
	DDX_Text(pDX, IDC_EDIT4, m_LineMsec);
	DDX_Text(pDX, IDC_EDIT5, m_RecvMsec);
	DDX_Text(pDX, IDC_EDIT6, m_CrLfMsec);
}

BEGIN_MESSAGE_MAP(CFileUpSendPage, CDialogRes)
END_MESSAGE_MAP()

static const INITDLGTAB UpSendItemTab[] = {
	{ IDC_RADIO4,	ITM_RIGHT_RIGHT },
	{ IDC_RADIO5,	ITM_RIGHT_RIGHT },
	{ IDC_RADIO6,	ITM_RIGHT_RIGHT },

	{ IDC_CHECK2,	ITM_RIGHT_RIGHT },
	{ IDC_CHECK3,	ITM_RIGHT_RIGHT },
	{ IDC_CHECK4,	ITM_RIGHT_RIGHT },
	{ IDC_CHECK5,	ITM_RIGHT_RIGHT },

	{ IDC_EDIT1,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_EDIT2,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_EDIT3,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_EDIT4,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_EDIT5,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_EDIT6,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },

	{ 0,	0 },
};

BOOL CFileUpSendPage::OnInitDialog()
{
	CDialogRes::OnInitDialog();

	InitItemOffset(UpSendItemTab);

	m_SendMode = m_pUpDown->m_SendMode;
	m_RecvWait = m_pUpDown->m_bRecvWait;
	m_CrLfWait = m_pUpDown->m_bCrLfWait;
	m_XonXoff  = m_pUpDown->m_bXonXoff;
	m_RecvEcho = m_pUpDown->m_bRecvEcho;

	m_BlockSize = m_pUpDown->m_BlockSize;
	m_BlockMsec = m_pUpDown->m_BlockMsec;
	m_CharUsec  = (double)m_pUpDown->m_CharUsec / 1000.0;
	m_LineMsec  = m_pUpDown->m_LineMsec;
	m_RecvMsec  = m_pUpDown->m_RecvMsec;
	m_CrLfMsec  = m_pUpDown->m_CrLfMsec;


	UpdateData(FALSE);

	return TRUE;
}

BOOL CFileUpSendPage::OnApply()
{
	UpdateData(TRUE);

	m_pUpDown->m_SendMode  = m_SendMode;
	m_pUpDown->m_bRecvWait = m_RecvWait;
	m_pUpDown->m_bCrLfWait = m_CrLfWait;
	m_pUpDown->m_bXonXoff  = m_XonXoff;
	m_pUpDown->m_bRecvEcho = m_RecvEcho;

	m_pUpDown->m_BlockSize = m_BlockSize;
	m_pUpDown->m_BlockMsec = m_BlockMsec;
	m_pUpDown->m_CharUsec  = (int)(m_CharUsec * 1000.0);
	m_pUpDown->m_LineMsec  = m_LineMsec;
	m_pUpDown->m_RecvMsec  = m_RecvMsec;
	m_pUpDown->m_CrLfMsec  = m_CrLfMsec;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CExtFileDialog

IMPLEMENT_DYNAMIC(CExtFileDialog, CFileDialog)

CExtFileDialog::CExtFileDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dwSize, BOOL bVistaStyle, int nDialogMode, class CSyncSock *pOwner) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
{
	m_DialogMode = nDialogMode;
	m_pOwner = pOwner;
	m_FrameRightOfs = 10;

	m_pUpConv   = NULL;
	m_pUpSend   = NULL;
	m_pDownPage = NULL;

	if ( !bVistaStyle && nDialogMode != 0 ) {
		SetTemplate(0, IDD);

		m_pUpConv   = new CFileUpConvPage();
		m_pUpSend   = new CFileUpSendPage();
		m_pDownPage = new CFileDownPage();

		m_pUpConv->m_pUpDown   = (CFileUpDown *)pOwner;
		m_pUpSend->m_pUpDown   = (CFileUpDown *)pOwner;
		m_pDownPage->m_pUpDown = (CFileUpDown *)pOwner;

	} else
		m_DialogMode = 0;
}
CExtFileDialog::~CExtFileDialog()
{
	if ( m_pUpConv != NULL )
		delete m_pUpConv;
	if ( m_pUpSend != NULL )
		delete m_pUpSend;
	if ( m_pDownPage != NULL )
		delete m_pDownPage;
}
void CExtFileDialog::DoDataExchange(CDataExchange* pDX)
{
	CFileDialog::DoDataExchange(pDX);

	if ( m_DialogMode != 0 ) {
		DDX_Control(pDX, IDC_TAB1, m_TabCtrl);
		DDX_Control(pDX, IDC_FRAME, m_Frame);
	}
}

BEGIN_MESSAGE_MAP(CExtFileDialog, CFileDialog)
	ON_WM_DESTROY()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CExtFileDialog::OnTcnSelchangeTab1)
	ON_WM_SIZE()
END_MESSAGE_MAP()
	
void CExtFileDialog::FontSizeCheck()
{
	CDC *pDc = GetDC();
	CFont *pFont;
	CSize ZoomMul, ZoomDiv, AddSize;
	int AddTop;
	CRect rect;
	WINDOWPLACEMENT place;
	CString FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	int FontSize = MulDiv(::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9), SCREEN_DPI_Y, SYSTEM_DPI_Y);

	if ( FontName.IsEmpty() && FontSize == 9 )
		return;

	if ( m_NewFont.GetSafeHandle() != NULL ) {
		CDialogExt::GetDlgFontBase(pDc, &m_NewFont, ZoomDiv);
		m_NewFont.DeleteObject();
	} else if ( (pFont = GetFont()) != NULL ) {
		CDialogExt::GetDlgFontBase(pDc, pFont, ZoomDiv);
	} else
		return;

	if ( !m_NewFont.CreatePointFont(FontSize * 10, FontName) )
		return;

	CDialogExt::GetDlgFontBase(pDc, &m_NewFont, ZoomMul);
	ReleaseDC(pDc);

	m_TabCtrl.GetWindowPlacement(&place);
	AddTop = place.rcNormalPosition.bottom;
	place.rcNormalPosition.right = place.rcNormalPosition.left + (place.rcNormalPosition.right - place.rcNormalPosition.left) * ZoomMul.cx / ZoomDiv.cx;
	place.rcNormalPosition.bottom = place.rcNormalPosition.top + (place.rcNormalPosition.bottom - place.rcNormalPosition.top) * ZoomMul.cy / ZoomDiv.cy;
	AddTop = place.rcNormalPosition.bottom - AddTop;
	m_TabCtrl.SetWindowPlacement(&place);
	m_TabCtrl.SetFont(&m_NewFont, TRUE);

	m_Frame.GetWindowPlacement(&place);
	AddSize.cx = place.rcNormalPosition.right - place.rcNormalPosition.left;
	AddSize.cy = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
	place.rcNormalPosition.right = place.rcNormalPosition.left + (place.rcNormalPosition.right - place.rcNormalPosition.left) * ZoomMul.cx / ZoomDiv.cx;
	place.rcNormalPosition.bottom = place.rcNormalPosition.top + (place.rcNormalPosition.bottom - place.rcNormalPosition.top) * ZoomMul.cy / ZoomDiv.cy;
	place.rcNormalPosition.top += AddTop;
	place.rcNormalPosition.bottom += AddTop;
	AddSize.cx = (place.rcNormalPosition.right - place.rcNormalPosition.left) - AddSize.cx;
	AddSize.cy = (place.rcNormalPosition.bottom - place.rcNormalPosition.top) - AddSize.cy;
	m_Frame.SetWindowPlacement(&place);
	m_Frame.SetFont(&m_NewFont, TRUE);

	GetWindowRect(rect);
	SetWindowPos(NULL, 0, 0, rect.Width() + AddSize.cx, rect.Height() + AddSize.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_SHOWWINDOW);
}

void CExtFileDialog::WindowSizeCheck(int cx, int cy)
{
	int n;
	WINDOWPLACEMENT place;

	if ( m_WindowSize.cx == cx && m_WindowSize.cy == cy )
		return;

	if ( m_TabCtrl.GetSafeHwnd() != NULL ) {
		m_TabCtrl.GetWindowPlacement(&place);
		place.rcNormalPosition.right = cx - m_FrameRightOfs;
		m_TabCtrl.SetWindowPlacement(&place);
		m_TabCtrl.Invalidate(FALSE);
	}

	if ( m_Frame.GetSafeHwnd() != NULL ) {
		m_Frame.GetWindowPlacement(&place);
		place.rcNormalPosition.right = cx - m_FrameRightOfs;
		m_Frame.SetWindowPlacement(&place);
		m_Frame.Invalidate(FALSE);
	}

	switch(m_DialogMode) {
	case EXTFILEDLG_DOWNLOAD:
		if ( m_pDownPage->GetSafeHwnd() == NULL )
			break;
		m_pDownPage->GetWindowPlacement(&place);
		place.rcNormalPosition.right = cx - m_FrameRightOfs;
		m_pDownPage->SetWindowPlacement(&place);
		m_pDownPage->Invalidate(FALSE);
		break;

	case EXTFILEDLG_UPLOAD:
		if ( m_pUpConv->GetSafeHwnd() == NULL || m_pUpSend->GetSafeHwnd() == NULL )
			break;
		m_pUpConv->GetWindowPlacement(&place);
		place.rcNormalPosition.right = cx - m_FrameRightOfs;
		m_pUpConv->SetWindowPlacement(&place);
		m_pUpConv->Invalidate(FALSE);

		m_pUpSend->GetWindowPlacement(&place);
		place.rcNormalPosition.right = cx - m_FrameRightOfs;
		m_pUpSend->SetWindowPlacement(&place);
		m_pUpSend->Invalidate(FALSE);

		n = m_TabCtrl.GetCurSel();
		m_pUpConv->ShowWindow(n == 0 ? SW_SHOW : SW_HIDE);
		m_pUpSend->ShowWindow(n == 1 ? SW_SHOW : SW_HIDE);
		break;
	}

	m_WindowSize.cx = cx;
	m_WindowSize.cy = cy;
}

BOOL CExtFileDialog::OnInitDialog()
{
	CString title;
	CRect frame, rect;

	CFileDialog::OnInitDialog();

	// WM_GETDLGCODE loop ... HELP!!!
	// https://groups.google.com/forum/#!topic/comp.os.ms-windows.programmer.controls/y0tPgUrDpGk

	if ( m_DialogMode != 0 ) {
		ModifyStyleEx(0, WS_EX_CONTROLPARENT);
		FontSizeCheck();
	}

	GetWindowRect(rect);
	m_WindowSize.cx = rect.Width();
	m_WindowSize.cy = rect.Height();

	if ( m_Frame.GetSafeHwnd() != NULL ) {
		m_Frame.GetWindowRect(frame);
		m_FrameRightOfs = frame.left - rect.left;	// ¶‰E“¯‚¶—]”’
	}

	switch(m_DialogMode) {
	case EXTFILEDLG_DOWNLOAD:
		if ( !m_pDownPage->Create(CFileDownPage::IDD, this) )
			return FALSE;

		m_pDownPage->ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME | WS_POPUP | WS_BORDER, WS_CHILD);
		m_pDownPage->ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, 0);
		m_pDownPage->SetParent(this);

		m_Frame.GetClientRect(frame);
		m_Frame.ClientToScreen(frame);
		ScreenToClient(frame);

		m_pDownPage->MoveWindow(frame, FALSE);

		m_pDownPage->ShowWindow(SW_SHOW);

		m_pDownPage->GetWindowText(title);
		m_TabCtrl.InsertItem(0, title);
		break;

	case EXTFILEDLG_UPLOAD:
		if ( !m_pUpConv->Create(CFileUpConvPage::IDD, this) )
			return FALSE;

		m_pUpConv->ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME | WS_POPUP | WS_BORDER, WS_CHILD);
		m_pUpConv->ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, 0);
		m_pUpConv->SetParent(this);

		if ( !m_pUpSend->Create(CFileUpSendPage::IDD, this) )
			return FALSE;

		m_pUpSend->ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME | WS_POPUP | WS_BORDER, WS_CHILD);
		m_pUpSend->ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, 0);
		m_pUpSend->SetParent(this);

		m_Frame.GetClientRect(frame);
		m_Frame.ClientToScreen(frame);
		ScreenToClient(frame);

		m_pUpConv->MoveWindow(frame, FALSE);
		m_pUpSend->MoveWindow(frame, FALSE);

		m_pUpConv->ShowWindow(SW_SHOW);
		m_pUpSend->ShowWindow(SW_HIDE);

		m_pUpConv->GetWindowText(title);
		m_TabCtrl.InsertItem(0, title);

		m_pUpSend->GetWindowText(title);
		m_TabCtrl.InsertItem(1, title);
		break;
	}

	return TRUE;
}

void CExtFileDialog::OnInitDone()
{
	CRect rect;

	GetWindowRect(rect);
	WindowSizeCheck(rect.Width(), rect.Height());

	CFileDialog::OnInitDone();
}

void CExtFileDialog::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n = m_TabCtrl.GetCurSel();

	switch(m_DialogMode) {
	case EXTFILEDLG_UPLOAD:
		m_pUpConv->ShowWindow(n == 0 ? SW_SHOW : SW_HIDE);
		m_pUpSend->ShowWindow(n == 1 ? SW_SHOW : SW_HIDE);
		break;
	}

	*pResult = 0;
}

void CExtFileDialog::OnDestroy()
{
	switch(m_DialogMode) {
	case EXTFILEDLG_DOWNLOAD:
		m_pDownPage->OnApply();
		break;
	case EXTFILEDLG_UPLOAD:
		m_pUpConv->OnApply();
		m_pUpSend->OnApply();
		break;
	}

	CFileDialog::OnDestroy();
}

void CExtFileDialog::OnSize(UINT nType, int cx, int cy)
{
	CFileDialog::OnSize(nType, cx, cy);

	if ( nType != SIZE_MINIMIZED )
		WindowSizeCheck(cx, cy);
}

//////////////////////////////////////////////////////////////////////
// CConvFileDialog

IMPLEMENT_DYNAMIC(CConvFileDialog, CFileDialog)

CConvFileDialog::CConvFileDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dwSize, BOOL bVistaStyle) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
{
	m_Code = SYSTEMICONV;
	m_CrLf = 0;
	m_WindowSize.SetSize(0, 0);
	m_ComboRightOfs = 0;

	if ( !bVistaStyle )
		SetTemplate(0, IDD);
}
CConvFileDialog::~CConvFileDialog()
{
}
void CConvFileDialog::DoDataExchange(CDataExchange* pDX)
{
	CFileDialog::DoDataExchange(pDX);

	DDX_CBStringExact(pDX, IDC_COMBO1, m_Code);
	DDX_CBIndex(pDX, IDC_COMBO2, m_CrLf);
}

BEGIN_MESSAGE_MAP(CConvFileDialog, CFileDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CConvFileDialog::WindowSizeCheck(int cx, int cy)
{
	CWnd *pWnd;
	WINDOWPLACEMENT place;

	if ( m_WindowSize.cx == cx && m_WindowSize.cy == cy )
		return;

	if ( (pWnd = GetDlgItem(IDC_COMBO1)) != NULL ) {
		pWnd->GetWindowPlacement(&place);
		place.rcNormalPosition.right = cx - m_ComboRightOfs;
		pWnd->SetWindowPlacement(&place);
		pWnd->Invalidate(FALSE);
	}

	if ( (pWnd = GetDlgItem(IDC_COMBO2)) != NULL ) {
		pWnd->GetWindowPlacement(&place);
		place.rcNormalPosition.right = cx - m_ComboRightOfs;
		pWnd->SetWindowPlacement(&place);
		pWnd->Invalidate(FALSE);
	}

	m_WindowSize.cx = cx;
	m_WindowSize.cy = cy;
}

BOOL CConvFileDialog::OnInitDialog()
{
	CRect rect;
	CComboBox *pWnd;
	WINDOWPLACEMENT place;

	CFileDialog::OnInitDialog();

	//ModifyStyleEx(0, WS_EX_CONTROLPARENT);

	GetWindowRect(rect);
	m_WindowSize.cx = rect.Width();
	m_WindowSize.cy = rect.Height();

	if ( (pWnd = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL ) {
		pWnd->GetWindowPlacement(&place);
		m_ComboRightOfs = m_WindowSize.cx - place.rcNormalPosition.right;
		pWnd->AddString(_T("DEFAULT"));
		pWnd->AddString(_T("CP932"));
		pWnd->AddString(_T("SHIFT-JIS"));
		pWnd->AddString(_T("EUC-JP-MS"));
		pWnd->AddString(_T("EUC-JP"));
		pWnd->AddString(_T("UTF-7"));
		pWnd->AddString(_T("UTF-8"));
		pWnd->AddString(_T("UTF-16BE"));
		pWnd->AddString(_T("UTF-16LE"));
		pWnd->AddString(_T("UTF-32BE"));
		pWnd->AddString(_T("UTF-32LE"));
		pWnd->AddString(_T("UCS-2BE"));
		pWnd->AddString(_T("UCS-2LE"));
		pWnd->AddString(_T("UCS-4BE"));
		pWnd->AddString(_T("UCS-4LE"));
	}

	if ( (pWnd = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL ) {
		pWnd->AddString(_T("CR+LF"));
		pWnd->AddString(_T("CR"));
		pWnd->AddString(_T("LF"));
	}

	m_Code = ::AfxGetApp()->GetProfileString(_T("ConvFileDialog"), _T("Code"), _T("DEFAULT"));
	m_CrLf = ::AfxGetApp()->GetProfileInt(_T("ConvFileDialog"), _T("CrLf"), 0);

	UpdateData(FALSE);

	return TRUE;
}

void CConvFileDialog::OnInitDone()
{
	CRect rect;

	GetWindowRect(rect);
	WindowSizeCheck(rect.Width(), rect.Height());

	CFileDialog::OnInitDone();
}

void CConvFileDialog::OnDestroy()
{
	UpdateData(TRUE);

	::AfxGetApp()->WriteProfileString(_T("ConvFileDialog"), _T("Code"), m_Code);
	::AfxGetApp()->WriteProfileInt(_T("ConvFileDialog"), _T("CrLf"), m_CrLf);

	CFileDialog::OnDestroy();
}

void CConvFileDialog::OnSize(UINT nType, int cx, int cy)
{
	CFileDialog::OnSize(nType, cx, cy);

	if ( nType != SIZE_MINIMIZED )
		WindowSizeCheck(cx, cy);
}
