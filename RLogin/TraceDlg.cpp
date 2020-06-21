// TraceDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TraceDlg.h"

//////////////////////////////////////////////////////////////////////
// CTraceDlg ダイアログ

IMPLEMENT_DYNAMIC(CTraceDlg, CDialogExt)

CTraceDlg::CTraceDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CTraceDlg::IDD, pParent)
{
	m_pDocument = NULL;
	m_pSave = NULL;
	m_TraceMaxCount = 1000;
}

CTraceDlg::~CTraceDlg()
{
	if ( m_pSave != NULL )
		delete m_pSave;
}

void CTraceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, m_List);
}

void CTraceDlg::AddTraceNode(CTraceNode *pNode)
{
	int n = m_List.GetItemCount();
	LPBYTE p = pNode->m_Buffer.GetPtr();
	LPBYTE e = p + pNode->m_Buffer.GetSize();
	BOOL bnew = TRUE;
	CString str, tmp;
	CTraceNode *np;
	CGrapWnd *pWnd;

	if ( n >= m_TraceMaxCount ) {
		while ( n > 0 ) {
			np = (CTraceNode *)(m_List.GetItemData(0));

			if ( n < m_TraceMaxCount && np->m_pSave != NULL )
				break;

			if ( np->m_Index != (-1) && m_pDocument != NULL && (pWnd = m_pDocument->m_TextRam.GetGrapWnd(np->m_Index)) != NULL )
				pWnd->m_Use--;

			if ( m_pDocument == NULL || m_pDocument->m_TextRam.m_pTraceNow != np )
				delete np;

			if ( m_File.m_hFile != CFile::hFileNull )
				WriteString(0);

			m_List.DeleteItem(0);
			n--;
		}
	}

	if ( n > 0 && pNode == (CTraceNode *)(m_List.GetItemData(n - 1)) ) {
		bnew = FALSE;
		n--;
	}

	for ( ; p < e ; p++ ) {
		if ( str.GetLength() > 28 && (p + 1) < e ) {
			p = e - 1;
			if ( p[-1] == '\033' && p[0] == '\\' )
				p--;
			str += _T("...");
		}

		if ( p[0] == '\033' && (p + 1) < e ) {
			switch(p[1]) {
			case 'P':  str += _T("[DCS]"); p++; break;
			case '[':  str += _T("[CSI]"); p++; break;
			case '\\': str += _T("[ST]");  p++; break;
			case ']':  str += _T("[OSC]"); p++; break;
			case '^':  str += _T("[PM]");  p++; break;
			case '_':  str += _T("[APC]"); p++; break;
			default:   str += _T("[ESC]"); break;
			}
		} else if ( *p <= ' ' || *p >= 0x7F || *p == '[' || *p == ']' ) {
			tmp.Format(_T("[%02x]"), *p);
			str += tmp;
		} else {
			str += (TCHAR)(*p);
		}
	}

	if ( pNode->m_Flag == TRACE_SIXEL && pNode->m_Index != (-1) ) {	// DECSIXEL
		CStringA mbs;
		CGrapWnd *pWnd;
		if ( m_pDocument != NULL && (pWnd = m_pDocument->m_TextRam.GetGrapWnd(pNode->m_Index)) != NULL )
			pWnd->m_Use++;
		mbs.Format("\033P;;;%dq\033\\", pNode->m_Index);
		pNode->m_Buffer.Clear();
		pNode->m_Buffer.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
		pNode->m_Flag = TRACE_OUT;
	}

	if ( bnew )
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, (LPARAM)pNode);
	else
		m_List.SetItemText(n, 0, str);

	m_List.SetItemText(n, 1, pNode->m_Name);
	tmp.Format(_T("%d"), pNode->m_CurX);
	m_List.SetItemText(n, 2, tmp);
	tmp.Format(_T("%d"), pNode->m_CurY);
	m_List.SetItemText(n, 3, tmp);
	tmp.Format(_T("%05x"), pNode->m_Attr & 0x003FFFF);
	m_List.SetItemText(n, 4, tmp);
	tmp.Format(_T("%d"), pNode->m_ForCol);
	m_List.SetItemText(n, 5, tmp);
	tmp.Format(_T("%d"), pNode->m_BakCol);
	m_List.SetItemText(n, 6, tmp);

//	m_List.EnsureVisible(n, FALSE);
}
void CTraceDlg::SetString(int num, CString &str)
{
	int n;

	str.Empty();
	for ( n = 0 ; n < 7 ; n++ ) {
		if ( n > 0 )
			str += _T('\t');
		str += m_List.GetItemText(num, n);
	}
	str += _T("\r\n");
}
void CTraceDlg::WriteString(int num)
{
	CString tmp;
	CStringA mbs;

	SetString(num, tmp);
	mbs = tmp;
	m_File.Write(mbs, mbs.GetLength());
}
void CTraceDlg::DeleteAllItems()
{
	int n;
	CTraceNode *np;
	CGrapWnd *pWnd;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		np = (CTraceNode *)(m_List.GetItemData(n));

		if ( np->m_Index != (-1) && m_pDocument != NULL && (pWnd = m_pDocument->m_TextRam.GetGrapWnd(np->m_Index)) != NULL )
			pWnd->m_Use--;

		if ( m_pDocument == NULL || m_pDocument->m_TextRam.m_pTraceNow != np )
			delete np;

		if ( m_File.m_hFile != CFile::hFileNull )
			WriteString(n);
	}

	m_List.DeleteAllItems();
}

////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTraceDlg, CDialogExt)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST2, &CTraceDlg::OnLvnItemchangedList)
	ON_NOTIFY(NM_KILLFOCUS, IDC_LIST2, &CTraceDlg::OnNMKillfocusList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST2, &CTraceDlg::OnNMCustomdrawList)
	ON_COMMAND(IDM_TEK_CLOSE, &CTraceDlg::OnTekClose)
	ON_COMMAND(IDM_TEK_SAVE, &CTraceDlg::OnTekSave)
	ON_COMMAND(IDM_TEK_CLEAR, &CTraceDlg::OnTekClear)
	ON_COMMAND(ID_EDIT_COPY, &CTraceDlg::OnEditCopy)
	ON_UPDATE_COMMAND_UI(IDM_TEK_SAVE, &CTraceDlg::OnUpdateTekSave)
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()

// CTraceDlg メッセージ ハンドラー

static const LV_COLUMN InitListTab[7] = {
	{ LVCF_TEXT | LVCF_WIDTH, 0, 220, _T("Log"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 100, _T("Name"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Cols"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Line"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Attr"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Color"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Back"),	0, 0 },
};

BOOL CTraceDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	if ( m_TraceMaxCount < (TRACE_SAVE * 3) )
		m_TraceMaxCount = (TRACE_SAVE * 3);
	else if ( m_TraceMaxCount > 200000 )
		m_TraceMaxCount = 200000;

	if ( !m_TraceLogFile.IsEmpty() && !m_File.Open(m_TraceLogFile, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyWrite) )
		AfxMessageBox(CStringLoad(IDE_LOGOPENERROR));

	if ( m_File.m_hFile != CFile::hFileNull )
		m_File.SeekToEnd();

	m_List.m_bSort = FALSE;
	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_List.InitColumn(_T("TraceDlg"), InitListTab, 7);
	m_List.SetItemCount(m_TraceMaxCount);

	HMENU hMenu;
	((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_TRACEWND), hMenu);
	SetMenu(CMenu::FromHandle(hMenu));

	return FALSE;
}
void CTraceDlg::OnOK()
{
//	CDialogExt::OnOK();
}
void CTraceDlg::OnCancel()
{
//	CDialogExt::OnCancel();
}

void CTraceDlg::OnSize(UINT nType, int cx, int cy)
{
	WINDOWPLACEMENT place;

	if ( m_List.GetSafeHwnd() != NULL ) {
		m_List.GetWindowPlacement(&place);
		place.rcNormalPosition.right  = cx;
		place.rcNormalPosition.bottom = cy;
		m_List.SetWindowPlacement(&place);
	}

	CDialogExt::OnSize(nType, cx, cy);
}

void CTraceDlg::OnClose()
{
	DeleteAllItems();

	m_List.SaveColumn(_T("TraceDlg"));

	if ( m_File.m_hFile != CFile::hFileNull )
		m_File.Close();

	((CMainFrame *)::AfxGetMainWnd())->DelTabDlg(this);

	CDialogExt::OnClose();

	if ( m_pDocument != NULL ) {
		m_pDocument->m_TextRam.SetTraceLog(FALSE);
		DestroyWindow();
	}
}

void CTraceDlg::PostNcDestroy()
{
	CDialogExt::PostNcDestroy();

	if ( m_pDocument != NULL ) {
		m_pDocument->m_TextRam.m_pTraceWnd = NULL;
		delete this;
	}
}

void CTraceDlg::OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int num = pNMLV->iItem;
	CTraceNode *np = NULL;
	LPBYTE p, e;

	*pResult = 0;

	if ( pNMLV->uNewState != (LVIS_FOCUSED | LVIS_SELECTED) )
		return;

	if ( num < 0 || num >= m_List.GetItemCount() || m_pDocument == NULL )
		return;

	if ( m_pSave == NULL )
		m_pSave = m_pDocument->m_TextRam.GETSAVERAM(SAVEMODE_ALL);

	for ( n = num ; n > 0 ; n-- ) {
		np = (CTraceNode *)(m_List.GetItemData(n));
		if ( np->m_pSave != NULL )
			break;
	}

	m_pDocument->m_TextRam.m_bTraceActive = TRUE;
	for ( ; n <= num ; n++ ) {
		np = (CTraceNode *)(m_List.GetItemData(n));
		if ( np->m_pSave != NULL )
			m_pDocument->m_TextRam.SETSAVERAM(np->m_pSave);
		else if ( np->m_Flag == TRACE_OUT ) {
			p = np->m_Buffer.GetPtr();
			e = p + np->m_Buffer.GetSize();
			while ( p < e )
				m_pDocument->m_TextRam.fc_FuncCall(*(p++));
		}
	}
	m_pDocument->m_TextRam.m_bTraceActive = FALSE;

	m_pDocument->m_TextRam.FLUSH();
}

void CTraceDlg::OnNMKillfocusList(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ( m_pSave != NULL ) {
		m_pDocument->m_TextRam.SETSAVERAM(m_pSave);
		delete m_pSave;
		m_pSave = NULL;
	}

	*pResult = 0;
}

void CTraceDlg::OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTraceNode *np;
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	switch(pLVCD->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		if ( pLVCD->iSubItem == 0 && (np = (CTraceNode *)(m_List.GetItemData((int)pLVCD->nmcd.dwItemSpec))) != NULL ) {
			if ( np->m_Flag == TRACE_OUT )
				pLVCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
			else if ( np->m_Flag == TRACE_NON )
				pLVCD->clrText = RGB(127, 127, 127);
			else if ( np->m_Flag == TRACE_UNGET )
				pLVCD->clrText = RGB(127,   0,   0);
		} else
			pLVCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
        *pResult = CDRF_NEWFONT;
		break;
	default:
        *pResult = 0;
		break;
	}
}

void CTraceDlg::OnTekClose()
{
	PostMessage(WM_CLOSE);
}

void CTraceDlg::OnTekSave()
{
	int n;
	CFileDialog dlg(FALSE, _T("txt"), _T("TRACE"), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGLOGFILE) , this);

	if ( m_File.m_hFile != CFile::hFileNull ) {
		for ( n = 0 ; n < m_List.GetItemCount() ; n++ )
			WriteString(n);
		m_File.Close();
		return;
	}

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	if ( !m_File.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyWrite) )
		AfxMessageBox(CStringLoad(IDE_LOGOPENERROR));

	if ( m_File.m_hFile != CFile::hFileNull )
		m_File.SeekToEnd();
}
void CTraceDlg::OnUpdateTekSave(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_File.m_hFile != CFile::hFileNull ? 1 : 0);
}

void CTraceDlg::OnEditCopy()
{
	int n;
	CString str;
	CString tmp;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		SetString(n, tmp);
		str += tmp;
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(str);
}

void CTraceDlg::OnTekClear()
{
	DeleteAllItems();

	if ( m_pDocument != NULL )
		m_pDocument->m_TextRam.SetTraceLog(TRUE);
}

void CTraceDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if ( nHitTest != HTCAPTION || !((CMainFrame *)::AfxGetMainWnd())->TabDlgInDrag(point, this, 6) )
		CDialogExt::OnNcLButtonDown(nHitTest, point);
}

//////////////////////////////////////////////////////////////////////
// CCmdHisDlg ダイアログ

IMPLEMENT_DYNAMIC(CCmdHisDlg, CDialogExt)

CCmdHisDlg::CCmdHisDlg(CWnd* pParent /*=NULL*/)	: CDialogExt(CTraceDlg::IDD, pParent)
{
	m_pDocument = NULL;
	m_bScrollMode = FALSE;
}

CCmdHisDlg::~CCmdHisDlg()
{
}

void CCmdHisDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, m_List);
}

void CCmdHisDlg::SetCmdHis(int num, CMDHIS *pCmdHis)
{
	CTime tm;

	if ( num < 0 )
		num = m_List.GetItemCount();

	tm = pCmdHis->st;
	m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, num, tm.Format(_T("%x")), 0, 0, 0, (LPARAM)pCmdHis);
	m_List.SetItemText(num, 1, tm.Format(_T("%X")));
	m_List.SetItemText(num, 2, pCmdHis->user);
	m_List.SetItemText(num, 3, pCmdHis->curd);
	m_List.SetItemText(num, 4, pCmdHis->cmds);
	m_List.SetItemText(num, 5, pCmdHis->exit.IsEmpty() && pCmdHis->emsg ? _T("wait") : pCmdHis->exit);
	m_List.SetItemText(num, 6, ExecTimeString(pCmdHis));

	m_List.EnsureVisible(num, FALSE);
}
static UINT MessageThread(LPVOID pParam)
{
	CString *pMsg = (CString *)pParam;

	MessageBeep(MB_ICONINFORMATION);
	::AfxMessageBox(*pMsg, MB_ICONINFORMATION);
	delete pMsg;

	return 0;
}
LPCTSTR CCmdHisDlg::ExecTimeString(CMDHIS *pCmdHis)
{
	int sec = (int)(pCmdHis->et - pCmdHis->st);
	static CString str;

	if ( pCmdHis->exit.IsEmpty() || sec <= 0 )
		return _T("");

	if ( sec < 60 )
		str.Format(_T("%dsec"), sec);
	else if ( sec < (60 * 60) )
		str.Format(_T("%d:%02d"), sec / 60, sec % 60);
	else
		str.Format(_T("%d:%02d:%02d"), sec / 3600, (sec / 60) % 60, sec % 60);

	return str;
}
void CCmdHisDlg::InfoExitStatus(CMDHIS *pCmdHis)
{
	int n;
	CString str, *pMsg;
	LPCTSTR p;

	str += _T("Command : ");

	for ( n = 0, p = pCmdHis->cmds ; n < 40 && *p != _T('\0') ; n++, p++ ) {
		if ( *p == _T('\r') )
			str += _T('↓');
		else if ( *p != _T('\n') && *p < _T(' ') )
			str += _T('.');
		else if ( *p != _T('\n') )
			str += *p;
	}

	str += _T("\nExit Status : ");
	str += pCmdHis->exit;

	str += _T("\nExec Time : ");
	str += ExecTimeString(pCmdHis);

	pMsg = new CString(str);
	AfxBeginThread(MessageThread, pMsg, THREAD_PRIORITY_NORMAL);
}
void CCmdHisDlg::SetExitStatus(CMDHIS *pCmdHis)
{
	int n;
	CString str;
	CStringA mbs;

	for ( n = m_List.GetItemCount() - 1 ; n >= 0 ; n-- ) {
		if ( (CMDHIS *)m_List.GetItemData(n) == pCmdHis ) {
			m_List.SetItemText(n, 5, pCmdHis->exit);
			m_List.SetItemText(n, 6, ExecTimeString(pCmdHis));

			if ( m_File.m_hFile != CFile::hFileNull ) {
				SetString(n, str);
				mbs = str;
				m_File.Write(mbs, mbs.GetLength() * sizeof(CHAR));
			}

			break;
		}
	}
}
void CCmdHisDlg::DelCmdHis(CMDHIS *pCmdHis)
{
	int n;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( (CMDHIS *)m_List.GetItemData(n) == pCmdHis ) {
			m_List.DeleteItem(n);
			break;
		}
	}
}
void CCmdHisDlg::InitList()
{
	ASSERT(m_pDocument != NULL);

	int n;
	CMDHIS *pCmdHis;
	POSITION pos = m_pDocument->m_TextRam.m_CommandHistory.GetTailPosition();

	m_List.DeleteAllItems();
	m_List.SetItemCount(CMDHISMAX + 1);

	for ( n = 0 ; pos != NULL ; n++ ) {
		pCmdHis = m_pDocument->m_TextRam.m_CommandHistory.GetPrev(pos);
		SetCmdHis(n, pCmdHis);
	}
}
void CCmdHisDlg::SetString(int num, CString &str)
{
	int n;
	LPCTSTR p;
	CString text, tmp;

	for ( n = 0 ; n < 7 ; n++ ) {
		if ( n > 0 )
			str += _T('\t');

		text = m_List.GetItemText(num, n);
		p = text;

		while ( *p != _T('\0') ) {
			if ( *p < _T(' ') || *p == _T('\x7F') ) {
				switch(*p) {
				case _T('\b'): str += _T("\\b"); break;
				case _T('\t'): str += _T("\\t"); break;
				case _T('\n'): str += _T("\\n"); break;
				case _T('\a'): str += _T("\\a"); break;
				case _T('\f'): str += _T("\\f"); break;
				case _T('\r'): str += _T("\\r"); break;
				default:
					tmp.Format(_T("\\x%02X"), *p);
					str += tmp;
					break;
				}
				p++;

			} else
				str += *(p++);
		}
	}

	str += _T("\r\n");
}
LPCTSTR CCmdHisDlg::ShellEscape(LPCTSTR str)
{
	CString work;
	static CString retstr;

	retstr.Empty();
	while ( *str != _T('\0') ) {
		if ( _tcschr(_T(" \"$@&'()^|[]{};*?<>`\\"), *str) != NULL ) {
			retstr += _T('\\');
			retstr += *(str++);
		} else if ( *str == _T('\t') ) {
			retstr += _T("\\t");
			str++;
		} else if ( *str != _T('\r') && *str != _T('\n') && *str < _T(' ') ) {
			work.Format(_T("\\%03o"), *str);
			retstr += work;
			str++;
		} else {
			retstr += *(str++);
		}
	}

	return retstr;
}

BEGIN_MESSAGE_MAP(CCmdHisDlg, CDialogExt)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_ADDCMDHIS, &CCmdHisDlg::OnAddCmdHis)
	ON_COMMAND(IDM_TEK_CLOSE, &CCmdHisDlg::OnTekClose)
	ON_COMMAND(IDM_TEK_SAVE, &CCmdHisDlg::OnTekSave)
	ON_UPDATE_COMMAND_UI(IDM_TEK_SAVE, &CCmdHisDlg::OnUpdateTekSave)
	ON_COMMAND(IDM_TEK_CLEAR, &CCmdHisDlg::OnTekClear)
	ON_COMMAND(ID_EDIT_COPY, &CCmdHisDlg::OnEditCopy)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST2, &CCmdHisDlg::OnNMCustomdrawList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST2, &CCmdHisDlg::OnLvnItemchangedList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, &CCmdHisDlg::OnNMDblclkList2)
	ON_COMMAND(IDM_CMDHIS_CMDS, &CCmdHisDlg::OnCmdhisCmds)
	ON_UPDATE_COMMAND_UI(IDM_CMDHIS_CMDS, &CCmdHisDlg::OnUpdateCmdhisCmds)
	ON_COMMAND(IDM_CMDHIS_CURD, &CCmdHisDlg::OnCmdhisCurd)
	ON_UPDATE_COMMAND_UI(IDM_CMDHIS_CURD, &CCmdHisDlg::OnUpdateCmdhisCurd)
	ON_COMMAND(IDM_CMDHIS_MSG, &CCmdHisDlg::OnCmdhisMsg)
	ON_UPDATE_COMMAND_UI(IDM_CMDHIS_MSG, &CCmdHisDlg::OnUpdateCmdhisMsg)
	ON_COMMAND(IDM_CMDHIS_SCRM, &CCmdHisDlg::OnCmdhisScrm)
	ON_UPDATE_COMMAND_UI(IDM_CMDHIS_SCRM, &CCmdHisDlg::OnUpdateCmdhisScrm)
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()

// CTraceDlg メッセージ ハンドラー

static const LV_COLUMN InitCmdHisListTab[7] = {
	{ LVCF_TEXT | LVCF_WIDTH, 0,   0, _T("Date"),				0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  60, _T("Time"),				0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  80, _T("User Host"),			0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 150, _T("Current Directory"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 150, _T("Command Line"),		0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Exit"),				0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  80, _T("Exec Time"),			0, 0 },
};

BOOL CCmdHisDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	m_List.m_bSort = FALSE;
	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_List.InitColumn(_T("CmdHisDlg"), InitCmdHisListTab, 7);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 9);

	HMENU hMenu;
	((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_TRACEWND), hMenu);
	SetMenu(CMenu::FromHandle(hMenu));

	InitList();

	AddShortCutKey(0, VK_SPACE,  0, 0, IDM_CMDHIS_SCRM);
	AddShortCutKey(0, VK_RETURN, 0, 0, IDM_CMDHIS_CMDS);
	AddShortCutKey(0, VK_TAB,	 0, 0, IDM_CMDHIS_CURD);
	AddShortCutKey(0, VK_ESCAPE, 0, 0, IDM_CMDHIS_MSG);
	AddShortCutKey(0, 'C', MASK_CTRL, 0, ID_EDIT_COPY);

	return FALSE;
}

void CCmdHisDlg::OnOK()
{
//	CDialogExt::OnOK();
}
void CCmdHisDlg::OnCancel()
{
//	CDialogExt::OnCancel();
}

void CCmdHisDlg::OnSize(UINT nType, int cx, int cy)
{
	WINDOWPLACEMENT place;

	if ( m_List.GetSafeHwnd() != NULL ) {
		m_List.GetWindowPlacement(&place);
		place.rcNormalPosition.right  = cx;
		place.rcNormalPosition.bottom = cy;
		m_List.SetWindowPlacement(&place);
	}

	CDialogExt::OnSize(nType, cx, cy);
}

void CCmdHisDlg::OnClose()
{
	m_List.SaveColumn(_T("CmdHisDlg"));

	if ( m_File.m_hFile != CFile::hFileNull )
		m_File.Close();

	((CMainFrame *)::AfxGetMainWnd())->DelTabDlg(this);

	CDialogExt::OnClose();
	DestroyWindow();
}

void CCmdHisDlg::PostNcDestroy()
{
	ASSERT(m_pDocument != NULL);

	CDialogExt::PostNcDestroy();

	m_pDocument->m_TextRam.m_pCmdHisWnd = NULL;
	delete this;
}

LRESULT CCmdHisDlg::OnAddCmdHis(WPARAM wParam, LPARAM lParam)
{
	switch(wParam) {
	case 0:
		SetCmdHis((-1), (CMDHIS *)lParam);
		break;
	case 1:
		SetExitStatus((CMDHIS *)lParam);
		break;
	}

	return TRUE;
}

void CCmdHisDlg::OnTekClose()
{
	PostMessage(WM_CLOSE);
}

void CCmdHisDlg::OnTekSave()
{
	int n;
	CString str;
	CStringA mbs;
	CFileDialog dlg(FALSE, _T("txt"), m_Title, OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGLOGFILE) , this);

	if ( m_File.m_hFile != CFile::hFileNull ) {
		m_File.Close();
		return;
	}

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	if ( !m_File.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyWrite) ) {
		AfxMessageBox(CStringLoad(IDE_LOGOPENERROR));
		return;
	}

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		str.Empty();
		SetString(n, str);
		mbs += str;
	}

	m_File.SeekToEnd();
	m_File.Write(mbs, mbs.GetLength() * sizeof(CHAR));
}
void CCmdHisDlg::OnUpdateTekSave(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_File.m_hFile != CFile::hFileNull ? 1 : 0);
}

void CCmdHisDlg::OnEditCopy()
{
	int n;
	CString str;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) != 0 )
			SetString(n, str);
	}

	if ( str.IsEmpty() ) {
		for ( n = 0 ; n < m_List.GetItemCount() ; n++ )
			SetString(n, str);
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(str);
}

void CCmdHisDlg::OnTekClear()
{
	ASSERT(m_pDocument != NULL);

	m_pDocument->m_TextRam.m_CommandHistory.RemoveAll();
	InitList();
}

void CCmdHisDlg::OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult)
{
	ASSERT(m_pDocument != NULL);

	CMDHIS *pCmdHis;
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	switch(pLVCD->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		if ( (pCmdHis = (CMDHIS *)(m_List.GetItemData((int)pLVCD->nmcd.dwItemSpec))) != NULL ) {
			if ( (m_pDocument->m_TextRam.m_HisAbs - pCmdHis->habs) <= (m_pDocument->m_TextRam.m_HisLen - m_pDocument->m_TextRam.m_Lines) )
				pLVCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
			else
				pLVCD->clrText = RGB(127, 127, 127);
		} else
			pLVCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
        *pResult = CDRF_NEWFONT;
		break;
	default:
        *pResult = 0;
		break;
	}
}

void CCmdHisDlg::OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult)
{
	ASSERT(m_pDocument != NULL);

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	CRLoginView *pView = (CRLoginView *)m_pDocument->GetAciveView();
	CMDHIS *pCmdHis = NULL;

	if ( pNMLV->iItem >= 0 )
		pCmdHis = (CMDHIS *)m_List.GetItemData(pNMLV->iItem);

	if ( m_bScrollMode && pCmdHis != NULL && pView != NULL ) {
		int ofs = m_pDocument->m_TextRam.m_HisAbs - pCmdHis->habs;
		if ( ofs <= (m_pDocument->m_TextRam.m_HisLen - m_pDocument->m_TextRam.m_Lines) ) {
			pView->m_HisOfs = ofs;
			m_pDocument->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
		}
	}

	*pResult = 0;
}

void CCmdHisDlg::OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	ASSERT(m_pDocument != NULL);

	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CRLoginView *pView = (CRLoginView *)m_pDocument->GetAciveView();
	CMDHIS *pCmdHis = NULL;

	if ( pNMItemActivate->iItem >= 0 )
		pCmdHis = (CMDHIS *)m_List.GetItemData(pNMItemActivate->iItem);

	if ( pCmdHis != NULL && pView != NULL ) {
		pView->SendPasteText(TstrToUni(pCmdHis->cmds));

		if ( pView->m_HisOfs != 0 ) {
			pView->m_HisOfs = 0;
			m_pDocument->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
		}

		pView->SetFocus();
	}

	*pResult = 0;
}

void CCmdHisDlg::OnCmdhisCmds()
{
	CString cmds;

	ASSERT(m_pDocument != NULL);

	for ( int n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) != 0 ) {
			CMDHIS *pCmdHis = (CMDHIS *)m_List.GetItemData(n);
			if ( !cmds.IsEmpty() )
				cmds += _T("\r\n");
			cmds += pCmdHis->cmds;
		}
	}

	CRLoginView *pView = (CRLoginView *)m_pDocument->GetAciveView();

	if ( cmds.IsEmpty() || pView == NULL )
		return;

	pView->SendPasteText(TstrToUni(cmds));

	if ( pView->m_HisOfs != 0 ) {
		pView->m_HisOfs = 0;
		m_pDocument->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
	}

	pView->SetFocus();
}
void CCmdHisDlg::OnUpdateCmdhisCmds(CCmdUI *pCmdUI)
{
	int num = m_List.GetSelectionMark();

	pCmdUI->Enable(num >= 0 && m_List.GetItemState(num, LVIS_SELECTED) != 0 ? TRUE : FALSE);
}

void CCmdHisDlg::OnCmdhisCurd()
{
	ASSERT(m_pDocument != NULL);

	int num = m_List.GetSelectionMark();

	if ( num < 0 )
		return;

	CMDHIS *pCmdHis = (CMDHIS *)m_List.GetItemData(num);
	CRLoginView *pView = (CRLoginView *)m_pDocument->GetAciveView();

	if ( pCmdHis == NULL || pView == NULL )
		return;

	pView->SendPasteText(TstrToUni(ShellEscape(pCmdHis->curd)));

	if ( pView->m_HisOfs != 0 ) {
		pView->m_HisOfs = 0;
		m_pDocument->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
	}

	pView->SetFocus();
}
void CCmdHisDlg::OnUpdateCmdhisCurd(CCmdUI *pCmdUI)
{
	int num = m_List.GetSelectionMark();

	pCmdUI->Enable(num >= 0 && m_List.GetItemState(num, LVIS_SELECTED) != 0 ? TRUE : FALSE);
}

void CCmdHisDlg::OnCmdhisMsg()
{
	int num = m_List.GetSelectionMark();
	CMDHIS *pCmdHis;
	
	if ( num >= 0 && m_List.GetItemState(num, LVIS_SELECTED) != 0 && (pCmdHis = (CMDHIS *)m_List.GetItemData(num)) != NULL && pCmdHis->exit.IsEmpty() ) {
		pCmdHis->emsg = (pCmdHis->emsg ? FALSE : TRUE);
		m_List.SetItemText(num, 5, pCmdHis->emsg ? _T("wait") : _T("")); 
	}
}
void CCmdHisDlg::OnUpdateCmdhisMsg(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	int num = m_List.GetSelectionMark();
	CMDHIS *pCmdHis;
	
	if ( num >= 0 && m_List.GetItemState(num, LVIS_SELECTED) != 0 && (pCmdHis = (CMDHIS *)m_List.GetItemData(num)) != NULL && pCmdHis->exit.IsEmpty() ) {
		pCmdUI->SetCheck(pCmdHis->emsg ? 1 : 0);
		bEnable = TRUE;
	}

	pCmdUI->Enable(bEnable);
}

void CCmdHisDlg::OnCmdhisScrm()
{
	CString str = m_Title;
	int num = m_List.GetSelectionMark();
	CRLoginView *pView = (CRLoginView *)m_pDocument->GetAciveView();
	CMDHIS *pCmdHis;
	int ofs = 0;

	m_bScrollMode = (m_bScrollMode ? FALSE : TRUE);

	if ( m_bScrollMode )
		str += _T(" (Scroll Mode)");

	SetWindowText(str);

	if ( pView == NULL )
		return;

	if ( m_bScrollMode ) {
		if ( num >= 0 && (pCmdHis = (CMDHIS *)m_List.GetItemData(num)) != NULL ) {
			ofs = m_pDocument->m_TextRam.m_HisAbs - pCmdHis->habs;
			if ( ofs > (m_pDocument->m_TextRam.m_HisLen - m_pDocument->m_TextRam.m_Lines) )
				ofs = pView->m_HisOfs;
		}
	}

	if ( pView->m_HisOfs != ofs ) {
		pView->m_HisOfs = ofs;
		m_pDocument->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
	}
}
void CCmdHisDlg::OnUpdateCmdhisScrm(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bScrollMode ? 1 : 0);
}

void CCmdHisDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if ( nHitTest != HTCAPTION || !((CMainFrame *)::AfxGetMainWnd())->TabDlgInDrag(point, this, 2) )
		CDialogExt::OnNcLButtonDown(nHitTest, point);
}

//////////////////////////////////////////////////////////////////////
// CHistoryDlg ダイアログ

IMPLEMENT_DYNAMIC(CHistoryDlg, CDialogExt)

CHistoryDlg::CHistoryDlg(CWnd* pParent /*=NULL*/)	: CDialogExt(CTraceDlg::IDD, pParent)
{
	m_bHorz = TRUE;
	m_bTrackMode = FALSE;
	m_PaneSize[0] = 0;
	m_PaneSize[1] = 333;
	m_PaneSize[2] = 666;
	m_PaneSize[3] = 1000;
}

CHistoryDlg::~CHistoryDlg()
{
}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	for ( int n = 0 ; n < 3 ; n++ ) {
		DDX_Control(pDX, IDC_TITLEBOX1 + n, m_TitleBox[n]);
		DDX_Control(pDX, IDC_LIST1 + n, m_ListBox[n]);
	}
}

void CHistoryDlg::Add(int nList, LPCTSTR str)
{
	int n, i;
	DWORD param = 0;
	CString tmp;

	if ( *str == _T('\0') || _tcslen(str) > (32 * 1024) )
		return;

	for ( n = 0 ; n < m_ListBox[nList].GetItemCount() ; n++ ) {
		tmp = m_ListBox[nList].GetItemText(n, 0);
		if ( tmp.Compare(str) == 0 ) {
			param = (DWORD)m_ListBox[nList].GetItemData(n) + 1;
			if ( n == 0 ) {
				m_ListBox[nList].SetItemData(n, (LPARAM)param);
				return;
			}
			m_ListBox[nList].DeleteItem(n);
			break;
		}
	}

	m_ListBox[nList].InsertItem(LVIF_TEXT | LVIF_PARAM, 0, str, 0, 0, 0, param);

	while ( (n = m_ListBox[nList].GetItemCount()) > HISLISTMAX ) {
		DWORD back = (DWORD)m_ListBox[nList].GetItemData(n - 1);
		for ( i = 2 ; i < (HISLISTMAX / 2) ; i++ ) {
			param = back;
			back = (DWORD)m_ListBox[nList].GetItemData(n - i);
			if ( param <= back )
				break;
		}
		m_ListBox[nList].DeleteItem(n - i + 1);
	}
}

BEGIN_MESSAGE_MAP(CHistoryDlg, CDialogExt)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_SIZING()
	ON_NOTIFY_RANGE(NM_DBLCLK, IDC_LIST1, IDC_LIST3, &CHistoryDlg::OnNMDblclkListCtrl)
	ON_MESSAGE(WM_ADDCMDHIS, &CHistoryDlg::OnAddCmdHis)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CHistoryDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	CString str;
	CBuffer buf;
	CRect frame, rect;

	GetClientRect(frame);
	m_TitleBox[0].GetWindowRect(rect);
	ScreenToClient(rect);
	m_Boder.left = rect.left;
	m_Boder.top  = rect.top;
	m_ListBox[2].GetWindowRect(rect);
	ScreenToClient(rect);
	m_Boder.right  = frame.right  - rect.right;
	m_Boder.bottom = frame.bottom - rect.bottom;

	for ( int n = 0 ; n < 3 ; n++ )
		m_ListBox[n].InsertColumn(0, _T(""), LVCFMT_LEFT, 800);

	((CRLoginApp *)::AfxGetApp())->GetProfileBuffer(_T("HistoryDlg"), _T("HisData"), buf);

	for ( int n = 0 ; n < 3 ; n++ ) {
		if ( buf.GetSize() < sizeof(WORD) )
			break;
		int count = buf.GetWord();
		for ( int i = 0 ; i < count ; i++ ) {
			if ( buf.GetSize() < (sizeof(DWORD) * 2) )
				break;
			DWORD param = buf.GetDword();
			buf.GetStrT(str);
			m_ListBox[n].InsertItem(LVIF_TEXT | LVIF_PARAM, i, str, 0, 0, 0, (LPARAM)param);
		}
	}

	m_PaneSize[1] = ((CRLoginApp *)::AfxGetApp())->GetProfileInt(_T("HistoryDlg"), _T("Separete1"), 333);
	m_PaneSize[2] = ((CRLoginApp *)::AfxGetApp())->GetProfileInt(_T("HistoryDlg"), _T("Separete2"), 666);

	GetClientRect(frame);
	int x  = ::AfxGetApp()->GetProfileInt(_T("HistoryDlg"), _T("x"),  frame.left);
	int y  = ::AfxGetApp()->GetProfileInt(_T("HistoryDlg"), _T("y"),  frame.top);
	int cx = ::AfxGetApp()->GetProfileInt(_T("HistoryDlg"), _T("cx"), frame.Width());
	int cy = ::AfxGetApp()->GetProfileInt(_T("HistoryDlg"), _T("cy"), frame.Height());
	SetWindowPos(NULL, x, y, cx, cy, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);

	return FALSE;
}

void CHistoryDlg::OnOK()
{
	NMITEMACTIVATE nm;
	LRESULT result;
	CWnd *pWnd = GetFocus();

	for ( int n = 0 ; n < 3 ; n++ ) {
		if ( pWnd->GetSafeHwnd() == m_ListBox[n].GetSafeHwnd() ) {
			ZeroMemory(&nm, sizeof(nm));
			nm.iItem = m_ListBox[n].GetSelectionMark();
			OnNMDblclkListCtrl(IDC_LIST1 + n, (NMHDR *)&nm, &result);
			break;
		}
	}

	//	CDialogExt::OnOK();
}
void CHistoryDlg::OnCancel()
{
	//	CDialogExt::OnCancel();
}

void CHistoryDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	int width  = MulDiv(50, m_NowDpi.cx, m_InitDpi.cx);
	int height = MulDiv(100, m_NowDpi.cy, m_InitDpi.cy);

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
void CHistoryDlg::ReSize(int cx, int cy)
{
	CRect rect, frame;
	CSize boder;
	int tcy, lcy, btm = 0;
	WINDOWPLACEMENT titlePlace, listPlace;

	frame.left   = MulDiv(m_Boder.left, m_NowDpi.cx, m_InitDpi.cx);
	frame.top    = MulDiv(m_Boder.top,  m_NowDpi.cy, m_InitDpi.cy);
	frame.right  = cx;
	frame.bottom = cy;

	boder.cx     = MulDiv(m_Boder.right,  m_NowDpi.cx, m_InitDpi.cx);
	boder.cy     = MulDiv(m_Boder.bottom, m_NowDpi.cy, m_InitDpi.cy);

	for ( int n = 0 ; n < 3 ; n++ ) {
		m_TitleBox[n].GetWindowPlacement(&titlePlace);
		m_ListBox[n].GetWindowPlacement(&listPlace);

		tcy = titlePlace.rcNormalPosition.bottom - titlePlace.rcNormalPosition.top;
		lcy = listPlace.rcNormalPosition.top - titlePlace.rcNormalPosition.top;

		if ( cx > cy ) {
			// Horizon
			rect.left   = frame.left + frame.Width() * m_PaneSize[n] / 1000;
			rect.right  = frame.left + frame.Width() * m_PaneSize[n + 1] / 1000 - boder.cx;
			rect.top    = frame.top;
			rect.bottom = frame.bottom - boder.cy;
			m_bHorz = TRUE;
		} else {
			// Vertical
			rect.left   = frame.left;
			rect.right  = frame.right - boder.cx;
			rect.top    = frame.top + frame.Height() * m_PaneSize[n] / 1000;
			rect.bottom = frame.top + frame.Height() * m_PaneSize[n + 1] / 1000 - boder.cy;

			switch(n) {
			case 0:
				if ( rect.bottom > (frame.bottom - lcy * 2 - boder.cy * 2) )
					rect.bottom = frame.bottom - lcy * 2 - boder.cy * 2;
				break;
			case 1:
				if ( rect.top > (frame.bottom - lcy * 2 - boder.cy * 2) )
					rect.top = frame.bottom - lcy * 2 - boder.cy * 2;
				if ( rect.bottom > (frame.bottom - lcy - boder.cy) )
					rect.bottom = frame.bottom - lcy - boder.cy;
				break;
			case 2:
				if ( n == 2 && rect.top > (frame.bottom - lcy - boder.cy) )
					rect.top = frame.bottom - lcy - boder.cy;
				break;
			}

			if ( rect.top < btm )
				rect.top = btm;
			if ( rect.bottom < btm )
				rect.bottom = btm;

			m_bHorz = FALSE;
		}

		titlePlace.rcNormalPosition.left   = rect.left;
		titlePlace.rcNormalPosition.right  = rect.right;
		titlePlace.rcNormalPosition.top    = rect.top;
		titlePlace.rcNormalPosition.bottom = rect.top + tcy;

		listPlace.rcNormalPosition.left    = rect.left;
		listPlace.rcNormalPosition.right   = rect.right;
		listPlace.rcNormalPosition.top     = rect.top + lcy;
		listPlace.rcNormalPosition.bottom  = rect.bottom;

		if ( listPlace.rcNormalPosition.bottom < listPlace.rcNormalPosition.top )
			listPlace.rcNormalPosition.bottom = listPlace.rcNormalPosition.top;

		btm = listPlace.rcNormalPosition.bottom + boder.cy;

		m_TitleBox[n].SetWindowPlacement(&titlePlace);
		m_ListBox[n].SetWindowPlacement(&listPlace);
	}

	if ( m_bHorz ) {
		for ( int n = 0 ; n < 2 ; n++ ) {
			m_TitleBox[n].GetWindowRect(rect);
			m_SizeRect[n].top  = rect.top;

			m_ListBox[n].GetWindowRect(rect);
			m_SizeRect[n].left = rect.right;
			m_SizeRect[n].bottom = rect.bottom;

			m_SizeRange[n].left = rect.left;
			m_SizeRange[n].top = rect.top;

			m_TitleBox[n + 1].GetWindowRect(rect);
			m_SizeRect[n].right = rect.left;

			m_ListBox[n + 1].GetWindowRect(rect);
			m_SizeRange[n].right = rect.right;
			m_SizeRange[n].bottom = rect.bottom;

			ScreenToClient(m_SizeRect[n]);
			ScreenToClient(m_SizeRange[n]);
		}
	} else {
		for ( int n = 0 ; n < 2 ; n++ ) {
			m_ListBox[n].GetWindowRect(rect);
			m_SizeRect[n].top  = rect.bottom;
			m_SizeRect[n].left = rect.left;
			m_SizeRect[n].right = rect.right;

			m_SizeRange[n].left = rect.left;
			m_SizeRange[n].top = rect.top;

			m_TitleBox[n + 1].GetWindowRect(rect);
			m_SizeRect[n].bottom = rect.top;

			m_ListBox[n + 1].GetWindowRect(rect);
			m_SizeRange[n].right = rect.right;
			m_SizeRange[n].bottom = rect.bottom - lcy;

			ScreenToClient(m_SizeRect[n]);
			ScreenToClient(m_SizeRange[n]);
		}
	}
}
void CHistoryDlg::OnSize(UINT nType, int cx, int cy)
{
	if ( m_TitleBox[0].GetSafeHwnd() != NULL && m_ListBox[0].GetSafeHwnd() != NULL )
		ReSize(cx, cy);

	CDialogExt::OnSize(nType, cx, cy);
}

void CHistoryDlg::OnClose()
{
	DWORD param;
	CString str;
	CBuffer buf;

	if ( !IsIconic() && !IsZoomed() && !((CMainFrame *)::AfxGetMainWnd())->IsInsideDlg(this) ) {
		CRect rect;
		GetWindowRect(&rect);
		AfxGetApp()->WriteProfileInt(_T("HistoryDlg"), _T("x"),  rect.left);
		AfxGetApp()->WriteProfileInt(_T("HistoryDlg"), _T("y"),  rect.top);
		AfxGetApp()->WriteProfileInt(_T("HistoryDlg"), _T("cx"), rect.Width());
		AfxGetApp()->WriteProfileInt(_T("HistoryDlg"), _T("cy"), rect.Height());
	}

	((CRLoginApp *)::AfxGetApp())->WriteProfileInt(_T("HistoryDlg"), _T("Separete1"), m_PaneSize[1]);
	((CRLoginApp *)::AfxGetApp())->WriteProfileInt(_T("HistoryDlg"), _T("Separete2"), m_PaneSize[2]);

	for ( int n = 0 ; n < 3 ; n++ ) {
		buf.PutWord(m_ListBox[n].GetItemCount());
		for ( int i = 0 ; i < m_ListBox[n].GetItemCount() ; i++ ) {
			param = (DWORD)m_ListBox[n].GetItemData(i);
			str = m_ListBox[n].GetItemText(i, 0);
			buf.PutDword(param);
			buf.PutStrT(str);
		}
	}

	((CRLoginApp *)::AfxGetApp())->WriteProfileBinary(_T("HistoryDlg"), _T("HisData"), buf.GetPtr(), buf.GetSize());
	((CMainFrame *)::AfxGetMainWnd())->DelTabDlg(this);

	CDialogExt::OnClose();
	DestroyWindow();
}

void CHistoryDlg::PostNcDestroy()
{
	CDialogExt::PostNcDestroy();
	
	CMainFrame *pMain;
	if ( (pMain = ((CMainFrame *)::AfxGetMainWnd())) != NULL )
		pMain->m_pHistoryDlg = NULL;

	delete this;
}

void CHistoryDlg::OnNMDblclkListCtrl(UINT id, NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nList = id - IDC_LIST1;
	CRLoginDoc *pDoc = ((CMainFrame *)::AfxGetMainWnd())->GetMDIActiveDocument();

	if ( pDoc != NULL && pNMItemActivate->iItem >= 0 ) {
		CString str = m_ListBox[nList].GetItemText(pNMItemActivate->iItem, 0);
		CRLoginView *pView = (CRLoginView *)pDoc->GetAciveView();

		if ( pView != NULL ) {
			if ( pView->m_HisOfs != 0 ) {
				pView->m_HisOfs = 0;
				pDoc->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
			}

			if ( nList == HISBOX_CDIR )
				str = CCmdHisDlg::ShellEscape(str);

			pView->SendPasteText(TstrToUni(str));
			pView->SetFocus();
		}
	}

	*pResult = 0;
}

LRESULT CHistoryDlg::OnAddCmdHis(WPARAM wParam, LPARAM lParam)
{
	CMDHIS *pCmdHis = (CMDHIS *)lParam;

	Add(HISBOX_CMDS, pCmdHis->cmds);
	Add(HISBOX_CDIR, pCmdHis->curd);

	return TRUE;
}

void CHistoryDlg::OffsetTrack(CPoint point)
{
	int n;

	if ( m_bHorz ) {
		m_TrackRect = m_TrackBase + CPoint(point.x - m_TrackPoint.x, 0);
		if ( (n = m_TrackRange.left - m_TrackRect.left) > 0 ) {
			m_TrackRect.left += n;
			m_TrackRect.right += n;
		} else if ( (n = m_TrackRect.right - m_TrackRange.right) > 0 ) {
			m_TrackRect.left -= n;
			m_TrackRect.right -= n;
		}
	} else {
		m_TrackRect = m_TrackBase + CPoint(0, point.y - m_TrackPoint.y);
		if ( (n = m_TrackRange.top - m_TrackRect.top) > 0 ) {
			m_TrackRect.top += n;
			m_TrackRect.bottom += n;
		} else if ( (n = m_TrackRect.bottom - m_TrackRange.bottom) > 0 ) {
			m_TrackRect.top -= n;
			m_TrackRect.bottom -= n;
		}
	}

	CRect frame;
	GetClientRect(frame);
	frame.left += MulDiv(m_Boder.left, m_NowDpi.cx, m_InitDpi.cx);
	frame.top  += MulDiv(m_Boder.top,  m_NowDpi.cy, m_InitDpi.cy);

	if ( m_bHorz )
		m_PaneSize[m_TrackPos + 1] = 1000 * (m_TrackRect.left + 1) / frame.Width();
	else
		m_PaneSize[m_TrackPos + 1] = 1000 * (m_TrackRect.bottom - 2) / frame.Height();

	GetClientRect(frame);
	ReSize(frame.Width(), frame.Height());
}
void CHistoryDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	for ( int n = 0 ; n < 2 ; n++ ) {
		if ( m_SizeRect[n].PtInRect(point) ) {
			m_TrackPoint = point;
			m_TrackRange = m_SizeRange[n];
			m_TrackBase = m_TrackRect = m_SizeRect[n];
			m_bTrackMode = TRUE;
			m_TrackPos = n;
			SetCapture();
			return;
		}
	}

	CDialogExt::OnLButtonDown(nFlags, point);
}
void CHistoryDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if ( m_bTrackMode ) {
		OffsetTrack(point);
		return;
	}

	CDialogExt::OnMouseMove(nFlags, point);
}
void CHistoryDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ( m_bTrackMode ) {
		OffsetTrack(point);
		m_bTrackMode = FALSE;
		ReleaseCapture();
		return;
	}

	CDialogExt::OnLButtonUp(nFlags, point);
}

BOOL CHistoryDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint point;
	HCURSOR hCursor;

	GetCursorPos(&point);
	ScreenToClient(&point);

	if ( m_SizeRect[0].PtInRect(point) || m_SizeRect[1].PtInRect(point) ) {
		if ( (hCursor = AfxGetApp()->LoadStandardCursor(m_bHorz ? IDC_SIZEWE : IDC_SIZENS)) != NULL ) {
			::SetCursor(hCursor);
			return TRUE;
		}
	}

	return CDialogExt::OnSetCursor(pWnd, nHitTest, message);
}

void CHistoryDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if ( nHitTest != HTCAPTION || !((CMainFrame *)::AfxGetMainWnd())->TabDlgInDrag(point, this, 7) )
		CDialogExt::OnNcLButtonDown(nHitTest, point);
}
