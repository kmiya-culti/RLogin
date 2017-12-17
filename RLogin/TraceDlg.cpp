// TraceDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TraceDlg.h"

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
	ON_WM_INITMENUPOPUP()
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

	if ( !m_TraceLogFile.IsEmpty() && !m_File.Open(m_TraceLogFile, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive) )
		AfxMessageBox(IDE_LOGOPENERROR);

	if ( m_File.m_hFile != CFile::hFileNull )
		m_File.SeekToEnd();

	m_List.m_bSort = FALSE;
	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_List.InitColumn(_T("TraceDlg"), InitListTab, 7);
	m_List.SetItemCount(m_TraceMaxCount);

	HMENU hMenu;
	((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_GRAPWND), hMenu);
	SetMenu(CMenu::FromHandle(hMenu));

	return TRUE;
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
		m_pSave = m_pDocument->m_TextRam.GETSAVERAM(TRUE);

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

	if ( dlg.DoModal() != IDOK )
		return;

	if ( !m_File.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive) )
		AfxMessageBox(IDE_LOGOPENERROR);

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

void CTraceDlg::OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex,BOOL bSysMenu)
{
    ASSERT(pPopupMenu != NULL);
    // Check the enabled state of various menu items.

    CCmdUI state;
    state.m_pMenu = pPopupMenu;
    ASSERT(state.m_pOther == NULL);
    ASSERT(state.m_pParentMenu == NULL);

    // Determine if menu is popup in top-level menu and set m_pOther to
    // it if so (m_pParentMenu == NULL indicates that it is secondary popup).
    HMENU hParentMenu;
    if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
        state.m_pParentMenu = pPopupMenu;    // Parent == child for tracking popup.
    else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
    {
        CWnd* pParent = this;
           // Child windows don't have menus--need to go to the top!
        if (pParent != NULL &&
           (hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
        {
           int nIndexMax = ::GetMenuItemCount(hParentMenu);
           for (int nIndex = 0; nIndex < nIndexMax; nIndex++)
           {
            if (::GetSubMenu(hParentMenu, nIndex) == pPopupMenu->m_hMenu)
            {
                // When popup is found, m_pParentMenu is containing menu.
                state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
                break;
            }
           }
        }
    }

    state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
    for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
      state.m_nIndex++)
    {
        state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
        if (state.m_nID == 0)
           continue; // Menu separator or invalid cmd - ignore it.

        ASSERT(state.m_pOther == NULL);
        ASSERT(state.m_pMenu != NULL);
        if (state.m_nID == (UINT)-1)
        {
           // Possibly a popup menu, route to first item of that popup.
           state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
           if (state.m_pSubMenu == NULL ||
            (state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
            state.m_nID == (UINT)-1)
           {
            continue;       // First item of popup can't be routed to.
           }
           state.DoUpdate(this, TRUE);   // Popups are never auto disabled.
        }
        else
        {
           // Normal menu item.
           // Auto enable/disable if frame window has m_bAutoMenuEnable
           // set and command is _not_ a system command.
           state.m_pSubMenu = NULL;
           state.DoUpdate(this, FALSE);
        }

        // Adjust for menu deletions and additions.
        UINT nCount = pPopupMenu->GetMenuItemCount();
        if (nCount < state.m_nIndexMax)
        {
           state.m_nIndex -= (state.m_nIndexMax - nCount);
           while (state.m_nIndex < nCount &&
            pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
           {
            state.m_nIndex++;
           }
        }
        state.m_nIndexMax = nCount;
    }
}
