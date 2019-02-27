// StatusDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ssh.h"
#include "StatusDlg.h"
#include "Script.h"
#include "PrnTextDlg.h"

// CStatusDlg ダイアログ

IMPLEMENT_DYNAMIC(CStatusDlg, CDialogExt)

CStatusDlg::CStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CStatusDlg::IDD, pParent)
{
	m_StatusText.Empty();
	m_OwnerType = 0;
	m_pValue = NULL;
	m_bEdit = FALSE;
}

CStatusDlg::~CStatusDlg()
{
}

void CStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_StatusWnd);
}

BEGIN_MESSAGE_MAP(CStatusDlg, CDialogExt)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_COMMAND(IDM_TEK_SAVE, &CStatusDlg::OnStatusSave)
	ON_COMMAND(IDM_TEK_CLOSE, &CStatusDlg::OnStatusClose)
	ON_COMMAND(IDM_TEK_CLEAR, &CStatusDlg::OnStatusClear)
	ON_COMMAND(ID_EDIT_COPY, &CStatusDlg::OnStatusCopy)
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CStatusDlg::OnFilePrintSetup)
	ON_COMMAND(ID_FILE_PRINT, &CStatusDlg::OnFilePrint)
END_MESSAGE_MAP()

// CStatusDlg メッセージ ハンドラ

BOOL CStatusDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	CWnd *pWnd;
	HMENU hMenu;

	if ( ((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_STATUSWND), hMenu) )
		SetMenu(CMenu::FromHandle(hMenu));

	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	if ( (pWnd = GetDlgItem(IDC_EDIT1)) != NULL && m_StatusFont.CreatePointFont(
			MulDiv(9 * 10, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96), _T("Consolas")) )
		pWnd->SetFont(&m_StatusFont);

	m_StatusWnd.SetWindowText(m_StatusText);

	if ( m_bEdit )
		m_StatusWnd.SetReadOnly(FALSE);

	return TRUE;
}

void CStatusDlg::OnSize(UINT nType, int cx, int cy)
{
	WINDOWPLACEMENT place;

	if ( m_StatusWnd.GetSafeHwnd() != NULL ) {
		m_StatusWnd.GetWindowPlacement(&place);
		place.rcNormalPosition.right  = cx;
		place.rcNormalPosition.bottom = cy;
		m_StatusWnd.SetWindowPlacement(&place);
	}

	CDialogExt::OnSize(nType, cx, cy);
}

void CStatusDlg::GetStatusText(CString &status)
{
	if ( m_bEdit )
		m_StatusWnd.GetWindowText(m_StatusText);

	status = m_StatusText;
}

void CStatusDlg::SetStatusText(LPCTSTR status)
{
	m_StatusText = status;
	m_StatusWnd.SetWindowText(m_StatusText);
	m_StatusWnd.SetSel(0, 0, FALSE);
}

void CStatusDlg::AddStatusText(LPCTSTR status)
{
	if ( _tcslen(status) < 8 ) {
		for ( ; *status != _T('\0') ; status++ ) {
			if ( *status == _T('\t') ) 
				m_StatusWnd.ReplaceSel(_T("\t"));
			else if ( *status == _T('\x0C') )
				m_StatusWnd.ReplaceSel(_T("\x0C"));
			else if ( *status != _T('\n') )
				m_StatusWnd.SendMessage(WM_CHAR, (WPARAM)*status, 0);
		}
	} else {
		//int nLen = m_StatusWnd.GetWindowTextLength();
		//m_StatusWnd.SetSel(nLen, nLen);
		m_StatusWnd.ReplaceSel(status);
	}
}

void CStatusDlg::PostNcDestroy()
{
	if ( m_pValue != NULL ) {
		switch(m_OwnerType) {
		case 1:
			((CScriptValue *)m_pValue)->GetAt("pWnd") = (void *)NULL;
			break;
		case 2:
			((CRLoginDoc *)m_pValue)->m_pStatusWnd = NULL;
			break;
		case 3:
			((CRLoginDoc *)m_pValue)->m_pMediaCopyWnd = NULL;
			break;
		}
	}

	delete this;
}

void CStatusDlg::OnCancel()
{
	CDialogExt::OnCancel();

	DestroyWindow();
}

void CStatusDlg::OnClose()
{
	CDialogExt::OnClose();

	DestroyWindow();
}

void CStatusDlg::OnStatusSave()
{
	CFile File;
	CStringA mbs;
	CFileDialog dlg(FALSE, _T("txt"), _T(""), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( dlg.DoModal() != IDOK )
		return;

	try {
		if ( File.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive) ) {
			if ( m_bEdit )
				m_StatusWnd.GetWindowText(m_StatusText);
			mbs = m_StatusText;
			File.Write((LPCSTR)mbs, mbs.GetLength());
			File.Close();
		}
	} catch(...) {
		MessageBox(_T("Save File Error"));
	}
}

void CStatusDlg::OnStatusClose()
{
	DestroyWindow();
}

void CStatusDlg::OnStatusClear()
{
	m_StatusText.Empty();
	m_StatusWnd.SetWindowText(m_StatusText);
}

void CStatusDlg::OnStatusCopy()
{
	int sta, end;

	m_StatusWnd.GetSel(sta, end);

	if ( sta != end )
		m_StatusWnd.Copy();
	else {
		if ( m_bEdit )
			m_StatusWnd.GetWindowText(m_StatusText);

		((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(m_StatusText);
	}
}

void CStatusDlg::OnFilePrintSetup()
{
	CPrnTextDlg dlg(this);

	dlg.DoModal();
}

void CStatusDlg::OnFilePrint()
{
	CDC dc;
    DOCINFO docinfo;
	CPrintDialog dlg(FALSE, PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION, this);
	CRect frame, rect;
	CFont font, *pOldFont;
	CSize dsz, sz;
	CString fontName;
	int fontSize;
	int fontLine;
	CRect margin;
	CString str;
	LPCTSTR p;
	BOOL bPrint = FALSE;
	BOOL bFF = FALSE;

	if ( dlg.DoModal() != IDOK )
		return;

	dc.Attach(dlg.CreatePrinterDC());

    memset(&docinfo, 0, sizeof(DOCINFO));
    docinfo.cbSize = sizeof(DOCINFO);
	docinfo.lpszDocName = m_Title.IsEmpty() ? _T("Mdedia Copy Print") : m_Title;

	fontName = ::AfxGetApp()->GetProfileString(_T("PrintText"), _T("FontName"), PRINTFONTNAME);
	fontSize = ::AfxGetApp()->GetProfileInt(_T("PrintText"),    _T("FontSize"), PRINTFONTSIZE);
	fontLine = ::AfxGetApp()->GetProfileInt(_T("PrintText"),    _T("FontLine"), PRINTLINESIZE);

	margin.left   = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginLeft"),   PRINTMARGINLEFT);
	margin.right  = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginRight"),  PRINTMARGINRIGHT);
	margin.top    = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginTop"),    PRINTMARGINTOP);
	margin.bottom = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginBottom"), PRINTMARGINBOTTOM);

	frame.left   = margin.left * 10 * dc.GetDeviceCaps(LOGPIXELSX) / 254;
	frame.top    = margin.top  * 10 * dc.GetDeviceCaps(LOGPIXELSY) / 254;
	frame.right  = dc.GetDeviceCaps(HORZRES) - margin.right  * 10 * dc.GetDeviceCaps(LOGPIXELSX) / 254;
	frame.bottom = dc.GetDeviceCaps(VERTRES) - margin.bottom * 10 * dc.GetDeviceCaps(LOGPIXELSY) / 254;

	font.CreateFont(fontSize * 10, 0, 0, 0, 0, FALSE, 0, 0, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, fontName);
	pOldFont = dc.SelectObject(&font);

	dsz.cx = fontSize / 2 * dc.GetDeviceCaps(LOGPIXELSX) / 72;
	dsz.cy = fontLine     * dc.GetDeviceCaps(LOGPIXELSY) / 72;

	dc.SetTextColor(RGB(0, 0, 0));
	dc.SetBkColor(RGB(255, 255, 255));
	dc.SetBkMode(TRANSPARENT);

	if ( m_bEdit )
		m_StatusWnd.GetWindowText(m_StatusText);

	dc.StartDoc(&docinfo);
	dc.StartPage();

	rect.left   = frame.left;
	rect.top    = frame.top;
	rect.right  = rect.left;
	rect.bottom = rect.top + dsz.cy;

	for ( p = m_StatusText ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('\r') ) {
			bPrint = TRUE;

		} else if ( *p == _T('\x0C') ) {
			bPrint = TRUE;
			bFF = TRUE;

		} else if ( *p == _T('\t') || *p >= _T(' ') ) {
			str += *p;
			sz = dc.GetTabbedTextExtent(str, 0, NULL);

			if ( (rect.left + sz.cx) > frame.right ) {
				bPrint = TRUE;
				str.Delete(str.GetLength() - 1, 1);
				p--;
			} else {
				rect.right = rect.left + sz.cx;
			}
		}

		if ( bPrint ) {
			if ( rect.bottom > frame.bottom ) {
				dc.EndPage();
				dc.StartPage();

				sz.cy = rect.Height();
				rect.top    = frame.top;
				rect.bottom = rect.top + sz.cy;
			}

			if ( !str.IsEmpty() )
				dc.TabbedTextOut(rect.left, rect.top, str, 0, NULL, 0);

			str.Empty();
			rect.top    = rect.bottom;
			rect.right  = rect.left;
			rect.bottom = rect.top + dsz.cy;
			bPrint = FALSE;

			if ( bFF ) {
				dc.EndPage();
				dc.StartPage();

				rect.top    = frame.top;
				rect.bottom = rect.top + dsz.cy;
				bFF = FALSE;
			}
		}
	}

	dc.EndPage();
	dc.EndDoc();
	dc.SelectObject(pOldFont);

	dc.Detach();
}
