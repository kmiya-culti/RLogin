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
#include "ExtFileDialog.h"

// CStatusDlg ダイアログ

IMPLEMENT_DYNAMIC(CStatusDlg, CDialogExt)

CStatusDlg::CStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CStatusDlg::IDD, pParent)
{
	m_StatusText.Empty();
	m_OwnerType = 0;
	m_pValue = NULL;
	m_bEdit = FALSE;
	SizeGripDisable(TRUE);
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
	ON_WM_DROPFILES()
	ON_COMMAND(IDM_TEK_SAVE, &CStatusDlg::OnStatusSave)
	ON_COMMAND(IDM_TEK_CLOSE, &CStatusDlg::OnStatusClose)
	ON_COMMAND(IDM_TEK_CLEAR, &CStatusDlg::OnStatusClear)
	ON_COMMAND(ID_EDIT_COPY, &CStatusDlg::OnStatusCopy)
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CStatusDlg::OnFilePrintSetup)
	ON_COMMAND(ID_FILE_PRINT, &CStatusDlg::OnFilePrint)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

// CStatusDlg メッセージ ハンドラ

BOOL CStatusDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	DragAcceptFiles(TRUE);

	CWnd *pWnd;
	HMENU hMenu;

	if ( ((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_STATUSWND), hMenu) )
		SetMenu(CMenu::FromHandle(hMenu));

	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	if ( (pWnd = GetDlgItem(IDC_EDIT1)) != NULL && m_StatusFont.CreatePointFont(MulDiv(m_FontSize * 10, m_NowDpi.cy, SYSTEM_DPI_Y), _T("Consolas")) )
		pWnd->SetFont(&m_StatusFont);

	m_StatusWnd.SetWindowText(m_StatusText);

	if ( m_bEdit )
		m_StatusWnd.SetReadOnly(FALSE);

	SetSaveProfile(_T("StatusDlg"));
	SetLoadPosition(LOADPOS_MAINWND);

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

LPCTSTR CStatusDlg::GetStatusText()
{
	if ( m_bEdit && m_StatusWnd.GetModify() )
		m_StatusWnd.GetWindowText(m_StatusText);

	return m_StatusText;
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
			((CScriptValue *)m_pValue)->GetAt(_T("pWnd")) = (void *)NULL;
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
	int s, e;
	INT_PTR res;
	CFile File;
	CBuffer tmp;
	CConvFileDialog dlg(FALSE, _T("txt"), _T(""), OFN_OVERWRITEPROMPT | OFN_ENABLESIZING, CStringLoad(IDS_FILEDLGALLFILE), this);

	m_StatusWnd.GetSel(s, e);
	res = DpiAwareDoModal(dlg);
	m_StatusWnd.SetSel(s, e, TRUE);

	if ( res != IDOK )
		return;

	try {
		if ( File.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive) ) {
			tmp = (LPCTSTR)GetStatusText();
			tmp.WstrConvert(dlg.m_Code, dlg.m_CrLf);
			File.Write(tmp.GetPtr(), tmp.GetSize());
			File.Close();
		}
	} catch(...) {
		::AfxMessageBox(_T("Save File Error"), MB_ICONERROR);
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
	else
		((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(GetStatusText());
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
	CStringLoad DefFontName(IDS_DEFFONTNAME0);

	if ( dlg.DoModal() != IDOK )
		return;

	dc.Attach(dlg.CreatePrinterDC());

    memset(&docinfo, 0, sizeof(DOCINFO));
    docinfo.cbSize = sizeof(DOCINFO);
	docinfo.lpszDocName = m_Title.IsEmpty() ? _T("Mdedia Copy Print") : m_Title;

	fontName = ::AfxGetApp()->GetProfileString(_T("PrintText"), _T("FontName"), DefFontName);
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

	GetStatusText();

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

LRESULT CStatusDlg::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = CDialogExt::OnDpiChanged(wParam, lParam);
	CWnd *pWnd;

	if ( (pWnd = GetDlgItem(IDC_EDIT1)) != NULL ) {
		if ( m_StatusFont.GetSafeHandle() != NULL )
			m_StatusFont.DeleteObject();
		if ( m_StatusFont.CreatePointFont(MulDiv(9 * 10, m_NowDpi.cy, SYSTEM_DPI_Y), _T("Consolas")) )
			pWnd->SetFont(&m_StatusFont);
	}

	return result;
}

void CStatusDlg::OnDropFiles(HDROP hDropInfo)
{
    int i, n;
	int max = MAX_PATH;
	TCHAR* pFileName = new TCHAR[max + 1];
    int FileCount;

    FileCount = DragQueryFile(hDropInfo, 0xffffffff, NULL, 0);

	for( i = 0 ; i < FileCount ; i++ ) {
		if (max < (n = DragQueryFile(hDropInfo, i, NULL, 0))) {
			max = n;
			delete[] pFileName;
			pFileName = new TCHAR[max + 1];
		}
		DragQueryFile(hDropInfo, i, pFileName, max);

		CBuffer text;
		text.LoadFile(pFileName);
		text.KanjiConvert(text.KanjiCheck());

		SetStatusText((LPCTSTR)text);
	}

	delete [] pFileName;
}
