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

IMPLEMENT_DYNAMIC(CDialogRes, CDialog)

CDialogRes::CDialogRes(UINT nIDTemplate, CWnd *pParent)	: CDialog(nIDTemplate, pParent)
{
	m_nIDTemplate = nIDTemplate;
	
	m_FontName = ::AfxGetApp()->GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
	m_FontSize = ::AfxGetApp()->GetProfileInt(_T("Dialog"), _T("FontSize"), 9);
}
CDialogRes::~CDialogRes()
{
}
BOOL CDialogRes::Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
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
	else {
		CString name;
		WORD size;
		dlgTemp.GetFont(name, size);
		if ( m_FontSize != size )
			dlgTemp.SetFont(name, m_FontSize);
	}

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

BEGIN_MESSAGE_MAP(CDialogRes, CDialog)
END_MESSAGE_MAP()

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
	DDX_CBString(pDX, IDC_COMBO1, m_DownFrom);
	DDX_CBString(pDX, IDC_COMBO2, m_DownTo);
	DDX_CBIndex(pDX, IDC_COMBO3, m_DecMode);
	DDX_CBIndex(pDX, IDC_COMBO4, m_DownCrLfMode);
	DDX_Text(pDX, IDC_EDIT1, m_DownSec);
}

BEGIN_MESSAGE_MAP(CFileDownPage, CDialogRes)
END_MESSAGE_MAP()

BOOL CFileDownPage::OnInitDialog()
{
	int n;
	CComboBox *pCombo;
	CStringArray stra;

	CDialogRes::OnInitDialog();

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
	DDX_CBString(pDX, IDC_COMBO1, m_UpFrom);
	DDX_CBString(pDX, IDC_COMBO2, m_UpTo);
	DDX_CBIndex(pDX, IDC_COMBO3, m_EncMode);
	DDX_CBIndex(pDX, IDC_COMBO4, m_UpCrLfMode);
}

BEGIN_MESSAGE_MAP(CFileUpConvPage, CDialog)
END_MESSAGE_MAP()

BOOL CFileUpConvPage::OnInitDialog()
{
	int n;
	CComboBox *pCombo;
	CStringArray stra;

	CDialogRes::OnInitDialog();

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

BEGIN_MESSAGE_MAP(CFileUpSendPage, CDialog)
END_MESSAGE_MAP()

BOOL CFileUpSendPage::OnInitDialog()
{
	CDialogRes::OnInitDialog();

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

BOOL CExtFileDialog::OnInitDialog()
{
	CString title;
	CRect frame;

	CFileDialog::OnInitDialog();

	// WM_GETDLGCODE loop ... HELP!!!
	// https://groups.google.com/forum/#!topic/comp.os.ms-windows.programmer.controls/y0tPgUrDpGk

	if ( m_DialogMode != 0 ) {
		ModifyStyleEx(0, WS_EX_CONTROLPARENT);
		FontSizeCheck();
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
