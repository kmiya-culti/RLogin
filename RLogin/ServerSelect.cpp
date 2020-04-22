// ServerSelect.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ServerSelect.h"
#include "OptDlg.h"
#include "Data.h"
#include "EditDlg.h"
#include "InitAllDlg.h"
#include "MsgChkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerSelect ダイアログ

IMPLEMENT_DYNAMIC(CServerSelect, CDialogExt)

CServerSelect::CServerSelect(CWnd* pParent /*=NULL*/)
	: CDialogExt(CServerSelect::IDD, pParent)
{
	m_EntryNum = (-1);
	m_pData = NULL;
	m_Group.Empty();
	m_DefaultEntryUid = (-1);
	m_bShowTabWnd = (-1);
	m_bShowTreeWnd = (-1);
	m_bTreeUpdate = FALSE;
	m_bTrackerActive = FALSE;
	m_TreeListPer = 200;
	m_bDragList = FALSE;
	m_pOPtDlg = NULL;
	m_pEditIndex = NULL;

	m_pTextRam  = new CTextRam;
	m_pKeyTab   = new CKeyNodeTab;
	m_pKeyMac   = new CKeyMacTab;
	m_pParamTab = new CParamTab;
}
CServerSelect::~CServerSelect()
{
	delete m_pTextRam;
	delete m_pKeyTab;
	delete m_pKeyMac;
	delete m_pParamTab;
}

void CServerSelect::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_SERVERLIST, m_List);
	DDX_Control(pDX, IDC_SERVERTAB, m_Tab);
	DDX_Control(pDX, IDC_SERVERTREE, m_Tree);
}

BEGIN_MESSAGE_MAP(CServerSelect, CDialogExt)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()

	ON_BN_CLICKED(IDC_NEWENTRY, OnNewEntry)
	ON_BN_CLICKED(IDC_EDITENTRY, OnEditEntry)
	ON_BN_CLICKED(IDC_DELENTRY, OnDelEntry)

	ON_COMMAND(ID_EDIT_NEW, OnNewEntry)
	ON_COMMAND(ID_EDIT_UPDATE, OnEditEntry)
	ON_COMMAND(ID_EDIT_DELETE, OnDelEntry)
	ON_COMMAND(ID_EDIT_DUPS, OnCopyEntry)
	ON_COMMAND(ID_EDIT_CHECK, OnCheckEntry)
	ON_COMMAND(ID_EDIT_COPY, &CServerSelect::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &CServerSelect::OnEditPaste)

	ON_COMMAND(IDM_SERV_INPORT, &CServerSelect::OnServInport)
	ON_COMMAND(IDM_SERV_EXPORT, &CServerSelect::OnServExport)
	ON_COMMAND(IDM_SERV_EXCHNG, &CServerSelect::OnServExchng)
	ON_COMMAND(IDM_SERV_PROTO, &CServerSelect::OnServProto)
	ON_COMMAND(IDC_SAVEDEFAULT, &CServerSelect::OnSaveDefault)
	ON_COMMAND(IDC_LOADDEFAULT, &CServerSelect::OnLoaddefault)
	ON_COMMAND(IDM_SHORTCUT, &CServerSelect::OnShortcut)

	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CServerSelect::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(IDM_SERV_EXPORT, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(IDM_SERV_EXCHNG, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(IDM_SERV_PROTO, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(IDC_SAVEDEFAULT, OnUpdateSaveDefault)
	ON_UPDATE_COMMAND_UI(IDC_LOADDEFAULT, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(IDM_SHORTCUT, OnUpdateEditEntry)

	ON_EN_KILLFOCUS(ID_EDIT_BOX, &CServerSelect::OnKillfocusEditBox)
	ON_EN_UPDATE(ID_EDIT_BOX, &CServerSelect::OnEnUpdateEditBox)

	ON_NOTIFY(NM_DBLCLK, IDC_SERVERLIST, OnDblclkServerlist)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_SERVERLIST, &CServerSelect::OnLvnBegindragServerlist)

	ON_NOTIFY(TCN_SELCHANGE, IDC_SERVERTAB, &CServerSelect::OnTcnSelchangeServertab)
	ON_NOTIFY(NM_RCLICK, IDC_SERVERTAB, &CServerSelect::OnNMRClickServertab)

	ON_NOTIFY(TVN_SELCHANGED, IDC_SERVERTREE, &CServerSelect::OnTvnSelchangedServertree)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_SERVERTREE, &CServerSelect::OnTvnEndlabeleditServertree)
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_SERVERTREE, &CServerSelect::OnTvnItemexpandedServertree)
	ON_NOTIFY(NM_RCLICK, IDC_SERVERTREE, &CServerSelect::OnNMRClickServertree)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_SERVERTREE, &CServerSelect::OnTvnBeginlabeleditServertree)
END_MESSAGE_MAP()

void CServerSelect::TreeExpandUpdate(HTREEITEM hTree, BOOL bExpand)
{
	int n;
	CString path;
	CStringIndex *pIndex;

	if ( hTree == NULL || (pIndex = (CStringIndex *)m_Tree.GetItemData(hTree)) == NULL )
		return;

	pIndex->GetPath(path);

	if ( bExpand ) {
		pIndex->m_Value = 0;
		if ( (n = m_TreeExpand.Find(path)) >= 0 )
			m_TreeExpand.RemoveAt(n);

	} else {
		pIndex->m_Value = 1;
		m_TreeExpand.Add(path);
	}
}
void CServerSelect::InitExpand(HTREEITEM hTree, UINT nCode)
{
	if ( !m_Tree.ItemHasChildren(hTree) )
		return;

	m_Tree.Expand(hTree, nCode);

	if ( (m_Tree.GetItemState(hTree, TVIS_EXPANDED) & TVIS_EXPANDED) != 0 ) {
		TreeExpandUpdate(hTree, TRUE);
		nCode = TVE_EXPAND;
	} else {
		TreeExpandUpdate(hTree, FALSE);
		nCode = TVE_COLLAPSE;
	}
	
	hTree = m_Tree.GetChildItem(hTree);

	while ( hTree != NULL ) {
		InitExpand(hTree, nCode);
		hTree = m_Tree.GetNextSiblingItem(hTree);
	}
}
void CServerSelect::InitTree(CStringIndex *pIndex, HTREEITEM hOwner, CStringIndex *pActive)
{
	int image = (pIndex->GetSize() > 0 ? 0 : (pIndex->m_TabData.GetSize() > 0 ? 7 : 2));
	LPCTSTR name = (pIndex->m_nIndex.IsEmpty() ? _T("...") : pIndex->m_nIndex);
	HTREEITEM hTree;
	
	if ( (hTree = m_Tree.InsertItem(name, image, image, hOwner)) == NULL ) 
		return;

	m_Tree.SetItemData(hTree, (DWORD_PTR)pIndex);

	if ( pIndex->m_Value == 0 )
		m_Tree.SetItemState(hTree, TVIS_EXPANDED, TVIS_EXPANDED);

	if ( pActive == pIndex )
		m_Tree.SelectItem(hTree);

	for ( int n = 0 ; n < pIndex->GetSize() ; n++ )
		InitTree(&((*pIndex)[n]), hTree, pActive);
}

void CServerSelect::InitList(CStringIndex *pIndex, BOOL bFolder)
{
	int n, i;
	CString str;

	m_List.DeleteAllItems();
	
	for ( i = 0 ; i < pIndex->m_TabData.GetSize() ; i++ ) {
		n = pIndex->m_TabData[i];

		m_List.InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, i, m_pData->GetAt(n).m_EntryName, 0, 0, 
			(m_DefaultEntryUid == m_pData->GetAt(n).m_Uid ? 1 : 2), n);

		m_List.SetItemText(i, 1, m_pData->GetAt(n).m_HostName);
		m_List.SetItemText(i, 2, m_pData->GetAt(n).m_UserName);
		m_List.SetItemText(i, 3, m_pData->GetAt(n).m_TermName);
		m_List.SetItemText(i, 4, m_pData->GetAt(n).GetKanjiCode());
		m_List.SetItemText(i, 5, m_pData->GetAt(n).m_PortName);
		m_List.SetItemText(i, 6, m_pData->GetAt(n).m_LastAccess.GetTime() != 0 ? m_pData->GetAt(n).m_LastAccess.Format(_T("%y/%m/%d %H:%M:%S")) : _T(""));
		m_List.SetItemText(i, 7, m_pData->GetAt(n).m_ListIndex);

		if ( m_pData->GetAt(n).m_bSelFlag ) {
			m_List.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_pData->GetAt(n).m_bSelFlag = FALSE;
		}

		if ( n == m_EntryNum )
			m_List.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
	
	if ( bFolder ) {
		for ( n = 0 ; n < pIndex->GetSize() ; n++, i++ ) {
			m_List.InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, i, (*pIndex)[n].m_nIndex, 0, 0, 0, (-1) - n);
			str.Format(_T("<-Tab (%d)"), (*pIndex)[n].m_TabData.GetSize());
			m_List.SetItemText(i, 1, str); //_T("<--Tab Group"));
		}
	}

	m_List.DoSortItem();
}

void CServerSelect::InitEntry(int nUpdate)
{
	int n, i;
	CStringIndex *pIndex, *pOwner;
	BOOL bNest = FALSE;
	BOOL bShowTab = FALSE;
	BOOL bShowTree = FALSE;

	m_TabEntry.RemoveAll();

	m_Tab.DeleteAllItems();
	m_TabData.RemoveAll();

	m_bTreeUpdate = TRUE;
	m_Tree.DeleteAllItems();

	for ( n = 0 ; n < m_pData->GetSize() ; n++ )
		m_TabEntry.AddPath(m_pData->GetAt(n).m_Group, &bNest).m_TabData.Add(n);

	for ( n = i = 0 ; n < m_AddGroup.GetSize() ; n++, i++ ) {
		pIndex = &(m_TabEntry.AddPath(m_AddGroup[n], &bNest));
		if ( pIndex->GetSize() > 0 || pIndex->m_TabData.GetSize() > 0 )
			m_AddGroup.RemoveAt(n--);
	}

	if ( n != i || nUpdate != INIT_CALL_NONE ) {
		((CRLoginApp *)::AfxGetApp())->WriteProfileStringArray(_T("ServerSelect"), _T("AddGroup"), m_AddGroup);
		((CRLoginApp *)::AfxGetApp())->UpdateServerEntry();
	}

	if ( m_TabEntry.GetSize() == 0 ) {
		m_TabEntry.AddPath(_T(""));
		m_Group.Empty();
	}

	for ( n = 0 ; n < m_TreeExpand.GetSize() ; n++ ) {
		if ( (pIndex = m_TabEntry.FindPath(m_TreeExpand[n])) != NULL )
			pIndex->m_Value = 1;
		else
			m_TreeExpand.RemoveAt(n--);
	}

	// CArrayで構築しているので構造体のアドレスが変化する!!!
	m_TabEntry.OwnerLink(NULL);

	if ( m_EntryNum >= 0 && m_EntryNum < m_pData->GetSize() )
		m_Group = m_pData->GetAt(m_EntryNum).m_Group;

	if ( (pIndex = m_TabEntry.FindPath(m_Group)) == NULL ) {
		pIndex = &(m_TabEntry[0]);
		m_Group.Empty();
		pIndex->GetPath(m_Group);
	}

	if ( (pOwner = pIndex->m_pOwner) != NULL ) {
		n = 0;
		if ( pOwner->m_pOwner != NULL ) {
			m_Tab.InsertItem(n++, pOwner->m_nIndex);
			m_TabData.Add(pOwner);
		}
		for ( i = 0 ; i < pOwner->GetSize() ; i++ ) {
			m_Tab.InsertItem(n, (*pOwner)[i].m_nIndex);
			m_TabData.Add(&((*pOwner)[i]));
			if ( &((*pOwner)[i]) == pIndex )
				m_Tab.SetCurSel(n);
			n++;
		}
	}

	for ( n = 0 ; n < m_TabEntry.GetSize() ; n++ )
		InitTree(&(m_TabEntry[n]), TVI_ROOT, pIndex);
	m_bTreeUpdate = FALSE;

	if ( bNest )
		bShowTree = TRUE;
	else if ( m_TabEntry.GetSize() > 1 )
		bShowTab = TRUE;

	InitList(pIndex, bShowTab);

	if ( m_bShowTabWnd != bShowTab || m_bShowTreeWnd != bShowTree ) {
		CRect rect;

		m_bShowTabWnd  = bShowTab;
		m_bShowTreeWnd = bShowTree;

		GetClientRect(rect);
		SetItemOffset(rect.Width(), rect.Height());
	}

	if ( nUpdate == INIT_CALL_AND_EDIT ) {
		if ( m_bShowTabWnd )
			OpenTabEdit(m_Tab.GetCurSel());
		if ( m_bShowTreeWnd ) {
			HTREEITEM hTree = m_Tree.GetSelectedItem();
			if ( hTree != NULL )
				m_Tree.EditLabel(hTree);
		}
	}
}

static const INITDLGTAB ItemTab[] = {
	{ IDOK,				ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDCANCEL,			ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_NEWENTRY,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_EDITENTRY,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_DELENTRY,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_SERVERTREE,													 ITM_BTM_BTM  },
	{ IDC_SERVERTAB,	                 ITM_RIGHT_RIGHT },
	{ IDC_SERVERLIST,					 ITM_RIGHT_RIGHT |				 ITM_BTM_BTM },
	{ 0,	0 },
};

void CServerSelect::SetItemOffset(int cx, int cy)
{
	CDialogExt::SetItemOffset(ItemTab, cx, cy);

	int n;
	WINDOWPLACEMENT placeTree, placeTab, placeList;

	if ( m_Tree.GetSafeHwnd() == NULL || m_Tab.GetSafeHwnd() == NULL || m_List.GetSafeHwnd() == NULL )
		return;

	m_Tree.GetWindowPlacement(&placeTree);
	m_Tab.GetWindowPlacement(&placeTab);
	m_List.GetWindowPlacement(&placeList);

	if ( m_bShowTreeWnd ) {
		n = placeList.rcNormalPosition.left - placeTree.rcNormalPosition.right;
		placeTree.rcNormalPosition.right = cx * m_TreeListPer / 1000;
		placeList.rcNormalPosition.left = placeTree.rcNormalPosition.right + n;
	} else
		placeList.rcNormalPosition.left = placeTree.rcNormalPosition.left;

	if ( !m_bShowTabWnd )
		placeList.rcNormalPosition.top = placeTab.rcNormalPosition.top;

	placeTree.showCmd = m_bShowTreeWnd ? SW_NORMAL : SW_HIDE;
	placeTab.showCmd  = m_bShowTabWnd  ? SW_NORMAL : SW_HIDE;

	m_Tree.SetWindowPlacement(&placeTree);
	m_Tab.SetWindowPlacement(&placeTab);
	m_List.SetWindowPlacement(&placeList);
}

void CServerSelect::UpdateDefaultEntry(int num)
{
	CServerEntry *pEntry;

	if ( m_DefaultEntryUid == (-1) )
		return;

	pEntry = &(m_pData->GetAt(num));

	if ( pEntry->m_Uid != m_DefaultEntryUid )
		return;

	if ( MessageBox(CStringLoad(IDS_DEFENTRYEDITMSG), _T("Question"), MB_ICONQUESTION | MB_YESNO) != IDYES )
		return;

	CRLoginDoc::LoadOption(*pEntry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
	CRLoginDoc::SaveDefOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
}
void CServerSelect::UpdateListIndex()
{
	int n, i;

	for ( i = 0 ; i < m_List.GetItemCount() ; i++ ) {
		if ( (n = (int)m_List.GetItemData(i)) >= 0 ) {
			m_pData->GetAt(n).m_ListIndex.Format(_T("%04d"), i);
			m_List.SetItemText(i, 7, m_pData->GetAt(n).m_ListIndex);
		}
	}
	m_List.SetSortCols(7);
}

/////////////////////////////////////////////////////////////////////////////
// CServerSelect メッセージ ハンドラ

static const LV_COLUMN InitListTab[8] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  80, _T("Entry"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60, _T("Server"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("User"),   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Term"),   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Kanji"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Socket"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,   0, _T("Last"),   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,   0, _T("Index"),  0, 0 }, 
	};

BOOL CServerSelect::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	int cx, cy;
	CRect rect;
	CBitmap BitMap;
	CBuffer buf;

	ASSERT(m_pData != NULL);

	m_TabEntry.SetNoCase(FALSE);
	m_TabEntry.SetNoSort(FALSE);

	((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(IDB_BITMAP4), BitMap);
	m_ImageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 4);
	m_ImageList.Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	//m_List.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn(_T("ServerSelect"), InitListTab, 8);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 0);

	m_Tree.SetImageList(&m_ImageList, TVSIL_NORMAL);

	if ( m_EntryNum == (-1) ) {
		m_EntryNum = AfxGetApp()->GetProfileInt(_T("ServerSelect"), _T("LastAccess"), (-1));
		if ( m_EntryNum >= m_pData->GetSize() )
			m_EntryNum = (-1);
	}

	if ( m_EntryNum < 0 && m_pData->GetSize() > 0 )
		m_Group = m_pData->GetAt(0).m_Group;

	m_DefaultEntryUid = ::AfxGetApp()->GetProfileInt(_T("ServerSelect"), _T("DefaultEntry"), (-1));

	InitItemOffset(ItemTab);
	m_TreeListPer = AfxGetApp()->GetProfileInt(_T("ServerSelect"), _T("TreePer"), m_TreeListPer);

	GetWindowRect(rect);
	m_MinWidth = rect.Width();
	m_MinHeight = rect.Height();

	cx = AfxGetApp()->GetProfileInt(_T("ServerSelect"), _T("cx"), rect.Width());
	cy = AfxGetApp()->GetProfileInt(_T("ServerSelect"), _T("cy"), rect.Height());
	if ( cx < rect.Width() )
		cx = rect.Width();
	if ( cy < rect.Height() )
		cy = rect.Height();
	MoveWindow(rect.left, rect.top, cx, cy, FALSE);

	((CRLoginApp *)::AfxGetApp())->GetProfileStringArray(_T("ServerSelect"), _T("AddGroup"), m_AddGroup);
	((CRLoginApp *)::AfxGetApp())->GetProfileStringArray(_T("ServerSelect"), _T("TreeExpand"), m_TreeExpand);

	((CRLoginApp *)::AfxGetApp())->GetProfileBuffer(_T("ServerSelect"), _T("AddEntryData"), buf);

	m_pData->InitGetUid();
	while ( buf.GetSize() >= (4 + 4 + 8) ) {
		int uid;
		int index;
		time_t last;
		CServerEntry *pEntry;

		uid   = buf.Get32Bit();
		index = buf.Get32Bit();
		last  = buf.Get64Bit();

		if ( (pEntry = m_pData->GetUid(uid)) != NULL ) {
			pEntry->m_ListIndex.Format(_T("%04d"), index);
			pEntry->m_LastAccess = last;
		}
	}

	InitEntry(INIT_CALL_NONE);

	if ( m_pData->GetSize() == 0 && ((CRLoginApp *)::AfxGetApp())->GetExtFilePath(NULL, _T(".rlg"), m_InitPathName) ) {
		if ( MessageBox(CStringLoad(IDS_INITENTRYINPORT), _T("Question"), MB_ICONQUESTION | MB_YESNO) == IDYES )
			PostMessage(WM_COMMAND, IDM_SERV_INPORT);
		else
			m_InitPathName.Empty();
	}

	return TRUE;
}

void CServerSelect::SaveWindowStyle()
{
	CRect rect;

	GetWindowRect(rect);
	AfxGetApp()->WriteProfileInt(_T("ServerSelect"), _T("cx"), MulDiv(rect.Width(), m_InitDpi.cx, m_NowDpi.cx));
	AfxGetApp()->WriteProfileInt(_T("ServerSelect"), _T("cy"), MulDiv(rect.Height(), m_InitDpi.cy, m_NowDpi.cy));
	AfxGetApp()->WriteProfileInt(_T("ServerSelect"), _T("TreePer"), m_TreeListPer);

	((CRLoginApp *)::AfxGetApp())->WriteProfileStringArray(_T("ServerSelect"), _T("TreeExpand"), m_TreeExpand);

	m_List.SaveColumn(_T("ServerSelect"));

	CBuffer buf;
	for ( int n = 0 ; n < m_pData->GetSize() ; n++ ) {
		if ( m_pData->GetAt(n).m_ListIndex.IsEmpty() && m_pData->GetAt(n).m_LastAccess == 0 )
			continue;

		buf.Put32Bit(m_pData->GetAt(n).m_Uid);
		buf.Put32Bit(_tstoi(m_pData->GetAt(n).m_ListIndex));
		buf.Put64Bit(m_pData->GetAt(n).m_LastAccess.GetTime());
	}
	((CRLoginApp *)::AfxGetApp())->WriteProfileBinary(_T("ServerSelect"), _T("AddEntryData"), buf.GetPtr(), buf.GetSize());
}

void CServerSelect::EntryNameCheck(CServerEntry &entry)
{
	int n, i;
	int num = 2;
	CString name, tmp;

	name = entry.m_EntryName;
	n = i = name.GetLength();

	if ( n > 0 && name[n - 1] == _T(')') ) {
		n--;
		while ( n > 0 && name[n - 1] >= _T('0') && name[n - 1] <= _T('9') ) {
			tmp += name[n - 1];
			n--;
		}
		if ( n > 0 && name[n - 1] == _T('(') ) {
			n--;
			name.Delete(n, i - n);
			num = _tstoi(tmp) + 1;
		}
	}

	for ( ; ; ) {
		for ( n = 0 ; n < m_pData->GetSize() ; n++ ) {
			if ( entry.m_EntryName.Compare(m_pData->GetAt(n).m_EntryName) == 0 )
				break;
		}
		if ( n >= m_pData->GetSize() )
			break;

		entry.m_EntryName.Format(_T("%s(%d)"), name, num++);
	}
}

void CServerSelect::OnOK()
{
	int n, i;
	int count = 0;

	for ( n = 0 ; n < m_pData->GetSize() ; n++ )
		m_pData->GetAt(n).m_CheckFlag = FALSE;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) != 0 ) {
			if ( (i = (int)m_List.GetItemData(n)) >= 0 ) {
				m_pData->GetAt(i).m_CheckFlag = TRUE;
				m_pData->GetAt(i).m_LastAccess = CTime::GetCurrentTime();
				count++;
			} else {
				if ( !m_Group.IsEmpty() )
					m_Group += _T('\\');
				m_Group += m_List.GetItemText(n, 0);
				m_EntryNum = (-1);
				InitEntry(INIT_CALL_NONE);
				return;
			}
		}
	}

	//if ( count <= 0 )
	//	return;

	m_EntryNum = m_List.GetSelectMarkData();
	AfxGetApp()->WriteProfileInt(_T("ServerSelect"), _T("LastAccess"), m_EntryNum);

	SaveWindowStyle();
	CDialogExt::OnOK();
}

void CServerSelect::OnCancel()
{
	SaveWindowStyle();
	CDialogExt::OnCancel();
}

void CServerSelect::OnClose()
{
	SaveWindowStyle();
	CDialogExt::OnClose();
}

/////////////////////////////////////////////////////////////////////////////

void CServerSelect::OnNewEntry() 
{
	CServerEntry Entry;
	COptDlg dlg(_T("Server New Entry"), this);

	Entry.m_Group = m_Group;
	CRLoginDoc::LoadDefOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

	dlg.m_pEntry    = &Entry;
	dlg.m_pTextRam  = m_pTextRam;
	dlg.m_pKeyTab   = m_pKeyTab;
	dlg.m_pKeyMac   = m_pKeyMac;
	dlg.m_pParamTab = m_pParamTab;
	dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;

	if ( dlg.DoModal() != IDOK )
		return;

	m_pTextRam->InitDefParam(FALSE);
	CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

	m_EntryNum = m_pData->AddEntry(Entry);

	InitEntry(INIT_CALL_UPDATE);
}

void CServerSelect::OnEditEntry() 
{
	int n;
	int num;
	CServerEntry Entry;
	CStringIndex index;
	int Count = 0;
	INT_PTR retId;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) != 0 && (int)m_List.GetItemData(n) >= 0 )
			Count++;
	}

	if ( Count <= 0 || (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	Entry = m_pData->GetAt(m_EntryNum);
	CRLoginDoc::LoadOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

	COptDlg dlg(_T("Server Edit Entry"), this);

	dlg.m_pEntry    = &Entry;
	dlg.m_pTextRam  = m_pTextRam;
	dlg.m_pKeyTab   = m_pKeyTab;
	dlg.m_pKeyMac   = m_pKeyMac;
	dlg.m_pParamTab = m_pParamTab;
	dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;

	m_pOPtDlg = &dlg;
	retId = dlg.DoModal();
	m_pOPtDlg = NULL;

	if ( retId != IDOK )
		return;

	m_pTextRam->InitDefParam(FALSE);
	CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

	if ( Count > 1 ) {
		CRLoginDoc::DiffIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, m_pData->GetAt(m_EntryNum), index);

		CStringLoad str(IDS_ENTRYMULTIEDIT);
		index.MsgStr(str);

		if ( MessageBox(str, _T("Edit Multiple Entries"), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;

		for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
			if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
				continue;

			Entry.Init();
			CRLoginDoc::LoadInitOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

			if ( (num = (int)m_List.GetItemData(n)) < 0 )
				continue;
			Entry = m_pData->GetAt(num);

			CRLoginDoc::LoadOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
			CRLoginDoc::LoadIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, index);
			CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

			Entry.m_bSelFlag = TRUE;
			m_pData->m_Data[num] = Entry;
			m_pData->UpdateAt(num);
			UpdateDefaultEntry(num);
		}

	} else {
		m_pData->m_Data[m_EntryNum] = Entry;
		m_pData->UpdateAt(m_EntryNum);
		UpdateDefaultEntry(m_EntryNum);
	}

	InitEntry(INIT_CALL_UPDATE);
}

void CServerSelect::OnDelEntry() 
{
	int n, i;
	CString tmp, msg;
	CDWordArray tab;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;
		if ( (i = (int)m_List.GetItemData(n)) < 0 )
			continue;

		if ( !tmp.IsEmpty() )
			tmp += _T(',');
		tmp += m_pData->m_Data[i].m_EntryName;

		if ( m_pData->m_Data[i].m_Uid >= 0 )
			tab.Add(m_pData->m_Data[i].m_Uid);
	}

	if ( (n = tmp.GetLength()) > 50 )
		tmp.Delete(40, n - 40);

	if ( (n = (int)tab.GetSize()) <= 0 )
		return;
	else if ( n > 1 ) {
		msg.Format(_T("...[%d]"), n);
		tmp += msg;
	}
	
	msg.LoadString(IDS_SERVERENTRYDELETE);
	msg += tmp;

	if ( MessageBox(msg, _T("Question"), MB_YESNO | MB_ICONQUESTION ) != IDYES )
		return;

	for ( n = 0 ; n < tab.GetSize() ; n++ ) {
		for ( i = 0 ; i < m_pData->GetSize() ; i++ ) {
			if ( tab[n] == m_pData->m_Data[i].m_Uid ) {
				m_pData->RemoveAt(i);
				break;
			}
		}
	}

	InitEntry(INIT_CALL_UPDATE);
}

void CServerSelect::OnCopyEntry() 
{
	CServerEntry tmp;

	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	tmp = m_pData->GetAt(m_EntryNum);
	EntryNameCheck(tmp);

	tmp.m_Uid = (-1);
	m_EntryNum = m_pData->AddEntry(tmp);

	InitEntry(INIT_CALL_UPDATE);
}

void CServerSelect::OnCheckEntry()
{
	int n, i;
	CServerEntryTab tab;
	CBuffer nbuf, obuf;
	CWordArray apend, remove, update;

	tab.Serialize(FALSE);

	for ( n = 0 ; n < tab.GetSize() ; n++ ) {
		for ( i = 0 ; i < m_pData->GetSize() ; i++ ) {
			if ( tab.GetAt(n).m_Uid == m_pData->GetAt(i).m_Uid )
				break;
		}
		if ( i < m_pData->GetSize() ) {
			nbuf.Clear();
			tab.GetAt(n).SetBuffer(nbuf);
			obuf.Clear();
			m_pData->GetAt(i).SetBuffer(obuf);
			if ( nbuf.GetSize() != obuf.GetSize() || memcmp(nbuf.GetPtr(), obuf.GetPtr(), nbuf.GetSize()) != 0 ) {
				update.Add(i);
				update.Add(n);
			}
		} else
			apend.Add(n);
	}
	for ( i = 0 ; i < m_pData->GetSize() ; i++ ) {
		for ( n = 0 ; n < tab.GetSize() ; n++ ) {
			if ( tab.GetAt(n).m_Uid == m_pData->GetAt(i).m_Uid )
				break;
		}
		if ( n >= tab.GetSize() )
			remove.Add(i);
	}

	for ( n = 0 ; n < apend.GetSize() ; n++ )
		m_pData->m_Data.Add(tab.GetAt(apend[n]));
	for ( n = 0 ; n < update.GetSize() ; n += 2 )
		m_pData->m_Data[update[n]] = tab.GetAt(update[n + 1]);
	for ( n = 0 ; n < remove.GetSize() ; n++ )
		m_pData->m_Data.RemoveAt(remove[n]);

	m_DefaultEntryUid = ::AfxGetApp()->GetProfileInt(_T("ServerSelect"), _T("DefaultEntry"), (-1));
	((CRLoginApp *)::AfxGetApp())->GetProfileStringArray(_T("ServerSelect"), _T("AddGroup"), m_AddGroup);

	InitEntry(INIT_CALL_NONE);

	if ( m_pOPtDlg != NULL ) {
		MessageBox(CStringLoad(IDE_ENTRYUPDATEEERROR), _T("Warning"), MB_ICONWARNING);
		m_pOPtDlg->PostMessage(WM_COMMAND, IDCANCEL);
	}
}

void CServerSelect::OnServInport()
{
	CString PathName;
	CFileDialog dlg(TRUE, _T("rlg"), _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGRLOGIN), this);

	if ( m_InitPathName.IsEmpty() ) {
		if ( DpiAwareDoModal(dlg) != IDOK )
			return;
		PathName = dlg.GetPathName();
	} else {
		PathName = m_InitPathName;
		m_InitPathName.Empty();
	}

	CFile File;

	if ( !File.Open(PathName, CFile::modeRead | CFile::shareExclusive) ) {
		MessageBox(_T("File Open Error"));
		return;
	}

	CWaitCursor wait;
	CArchive Archive(&File, CArchive::load);

	int n;
	CHAR tmp[32];
	CServerEntry Entry;
	int ver = 0;

	TRY {
		memset(tmp, 0, 16);
		for ( n = 0 ; n < 16 && Archive.Read(&(tmp[n]), 1) == 1 ; n++ ) {
			if ( tmp[n] == '\n' )
				break;
		}

		if ( strncmp((LPSTR)tmp, "RLG2", 4) == 0 )
			ver = 2;
		else if ( strncmp((LPSTR)tmp, "RLG3", 4) == 0 )
			ver = 3;
		else if ( strncmp((LPSTR)tmp, "RLG4", 4) == 0 )
			ver = 4;
		else if ( strncmp((LPSTR)tmp, "\"RLG5", 5) == 0 )
			ver = 5;

		if ( ver == 5 ) {
			CString str;
			CBuffer buf;
			CStringIndex index;

			tmp[n] = '\0';
			str = tmp;
			buf.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));

			while ( index.ReadString(Archive, str) )
				buf.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));

			index.SetNoCase(TRUE);
			index.SetNoSort(TRUE);

			if ( !index.GetJsonFormat(buf) || _tcsncmp(index.m_nIndex, _T("RLG5"), 4) != 0 )
				AfxThrowArchiveException(CArchiveException::badIndex, Archive.GetFile()->GetFileTitle());

			for ( n = 0 ; n < index.GetSize() ; n++ ) {
				if ( index[n].Find(_T("Entry")) < 0 )
					continue;

				CRLoginDoc::LoadIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, index[n]);

				Entry.m_Uid = (-1);
				Entry.m_bSelFlag = TRUE;

				//m_pParamTab->m_IdKeyList.RemoveAll();
				CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

				m_EntryNum = m_pData->AddEntry(Entry);

				Entry.Init();
				CRLoginDoc::LoadInitOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
			}

		} else if ( ver == 3 || ver == 4 ) {
			for ( ; ; ) {
				CStringIndex index;

				index.RemoveAll();
				index.SetNoCase(TRUE);
				index.SetNoSort(TRUE);
				index.Serialize(Archive, NULL, ver);

				CRLoginDoc::LoadIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, index);

				Entry.m_Uid = (-1);
				Entry.m_bSelFlag = TRUE;

				//m_pParamTab->m_IdKeyList.RemoveAll();
				CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

				m_EntryNum = m_pData->AddEntry(Entry);

				memset(tmp, 0, 16);
				for ( n = 0 ; n < 16 && Archive.Read(&(tmp[n]), 1) == 1 ; n++ ) {
					if ( tmp[n] == '\n' )
						break;
				}
				if ( (ver == 3 && strncmp((LPSTR)tmp, "RLG31", 5) != 0) ||
					 (ver == 4 && strncmp((LPSTR)tmp, "RLG41", 5) != 0) )
					break;

				Entry.Init();
				CRLoginDoc::LoadInitOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
			}

		} else if ( ver == 2 ) {
			for ( ; ; ) {
				Entry.Serialize(Archive);
				m_pTextRam->Serialize(Archive);
				m_pKeyTab->Serialize(Archive);
				m_pKeyMac->Serialize(Archive);
				m_pParamTab->Serialize(Archive);

				Entry.m_Uid = (-1);
				Entry.m_bSelFlag = TRUE;

				//m_pParamTab->m_IdKeyList.RemoveAll();
				CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

				m_EntryNum = m_pData->AddEntry(Entry);

				memset(tmp, 0, 16);
				for ( n = 0 ; n < 16 && Archive.Read(&(tmp[n]), 1) == 1 ; n++ ) {
					if ( tmp[n] == '\n' )
						break;
				}
				if ( strncmp((LPSTR)tmp, "RLG21", 5) != 0 )
					break;

				Entry.Init();
				CRLoginDoc::LoadInitOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
			}

		} else
			AfxThrowArchiveException(CArchiveException::badIndex, Archive.GetFile()->GetFileTitle());

	} CATCH_ALL(e) {
		MessageBox(_T("File Read Error"));
	} END_CATCH_ALL

	Archive.Close();
	File.Close();

	InitEntry(INIT_CALL_UPDATE);
}

void CServerSelect::OnServExport()
{
	int n;
	CFile File;
	CStringIndex index;
	CServerEntry Entry;

	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	CFileDialog dlg(FALSE, _T("rlg"), _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGRLOGIN), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	if ( !File.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive) ) {
		MessageBox(_T("File Crate Error"));
		return;
	}

	CWaitCursor wait;
	CArchive Archive(&File, CArchive::store | CArchive::bNoFlushOnDelete);

	TRY {
		for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
			if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
				continue;

			if ( (m_EntryNum = (int)m_List.GetItemData(n)) < 0 )
				continue;

			Entry = m_pData->GetAt(m_EntryNum);
			CRLoginDoc::LoadOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

			index.RemoveAll();
			index.SetNoCase(TRUE);
			index.SetNoSort(TRUE);

			CRLoginDoc::SaveIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, index);

			Archive.Write("RLG310\r\n", 8);
			index.Serialize(Archive, NULL);
			Archive.Write("ENDOF\r\n", 7);
		}

	} CATCH_ALL(e) {
		MessageBox(_T("File Write Error"));
	} END_CATCH_ALL

	Archive.Close();
	File.Close();
}

void CServerSelect::OnServExchng()
{
	CFileDialog dlg(TRUE, _T("rlg"), _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGRLOGIN), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	CFile File;

	if ( !File.Open(dlg.GetPathName(), CFile::modeRead | CFile::shareExclusive) ) {
		MessageBox(_T("File Open Error"));
		return;
	}

	CWaitCursor wait;
	CArchive Archive(&File, CArchive::load);

	int n;
	CServerEntry Entry;
	CStringIndex index;

	index.SetNoCase(TRUE);
	index.SetNoSort(TRUE);

	TRY {
		index.Serialize(Archive, NULL);
	} CATCH_ALL(e) {
		MessageBox(_T("File Read Error"));
	} END_CATCH_ALL

	Archive.Close();
	File.Close();

	if ( index.GetSize() == 0 )
		return;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;

		if ( (m_EntryNum = (int)m_List.GetItemData(n)) < 0 )
			continue;

		Entry = m_pData->GetAt(m_EntryNum);
		CRLoginDoc::LoadOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
		CRLoginDoc::LoadIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, index);
		CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

		m_pData->m_Data[m_EntryNum] = Entry;
		m_pData->UpdateAt(m_EntryNum);
		UpdateDefaultEntry(m_EntryNum);
	}

	InitEntry(INIT_CALL_UPDATE);
}

void CServerSelect::OnServProto()
{
	CString proto, option;
	CServerEntry *pEntry;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
	CEditDlg dlg;

	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 || pApp == NULL )
		return;

	pEntry = &(m_pData->m_Data[m_EntryNum]);

	switch(pEntry->m_ProtoType) {
	case PROTO_LOGIN:
		proto = _T("login");
		break;
	case PROTO_TELNET:
		proto = _T("telnet");
		break;
	case PROTO_SSH:
		proto = _T("ssh");
		break;
	default:
		MessageBox(CStringLoad(IDE_PROTOCOLENTRY));
		return;
	}

	if ( pEntry->m_EntryName.IsEmpty() ) {
		MessageBox(CStringLoad(IDE_USESERVERENTRY));
		return;
	}
	option.Format(_T("/entry \"%s\" /inuse"), pEntry->m_EntryName);

	dlg.m_WinText.LoadString(IDS_PROTODLGTITLE);
	dlg.m_Title = proto;
	dlg.m_Title += CStringLoad(IDS_PRORODLGCOMM);
	dlg.m_Edit = option;

	if ( dlg.DoModal() != IDOK )
		return;
	option = dlg.m_Edit;

	pApp->RegisterShellProtocol(proto, option);
}

void CServerSelect::OnSaveDefault()
{
	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	CServerEntry *pEntry;

	pEntry = &(m_pData->GetAt(m_EntryNum));
	CRLoginDoc::LoadOption(*pEntry,*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
	CRLoginDoc::SaveDefOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

	if ( pEntry->m_Uid != (-1) )
		::AfxGetApp()->WriteProfileInt(_T("ServerSelect"), _T("DefaultEntry"), pEntry->m_Uid);

	m_DefaultEntryUid = pEntry->m_Uid;

	InitEntry(INIT_CALL_NONE);
}

void CServerSelect::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CServerSelect::OnUpdateSaveDefault(CCmdUI *pCmdUI)
{
	int n = m_List.GetSelectMarkData();

	if ( n >= 0 && m_List.GetSelectMarkCount() == 1 && !m_pData->GetAt(n).m_bOptFixed )
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CServerSelect::OnLoaddefault()
{
	int n;
	CBuffer ProBuffer;
	CInitAllDlg dlg;

	if ( dlg.DoModal() != IDOK )
		return;

	switch(dlg.m_InitType) {
	case 0:		// Init Default Entry
		CRLoginDoc::LoadDefOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
		break;

	case 1:		// Init Program Default
		CRLoginDoc::LoadInitOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
		break;

	case 2:		// Copy Entry option
		ASSERT(dlg.m_pInitEntry != NULL);
		CRLoginDoc::LoadOption(*(dlg.m_pInitEntry), *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
		break;
	}

	m_pTextRam->Serialize(TRUE,  ProBuffer);
	m_pKeyTab->Serialize(TRUE,   ProBuffer);
	m_pKeyMac->Serialize(TRUE,   ProBuffer);
	m_pParamTab->Serialize(TRUE, ProBuffer);

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;

		if ( (m_EntryNum = (int)m_List.GetItemData(n)) < 0 )
			continue;

		m_pData->GetAt(m_EntryNum).m_ProBuffer = ProBuffer;
		m_pData->UpdateAt(m_EntryNum);
	}
}

void CServerSelect::OnShortcut()
{
	int n, i;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;

		if ( (i = (int)m_List.GetItemData(n)) < 0 )
			continue;

		if ( m_pData->GetAt(i).m_EntryName.IsEmpty() )
			continue;

		if ( !((CRLoginApp *)AfxGetApp())->CreateDesktopShortcut(m_pData->GetAt(i).m_EntryName) ) {
			MessageBox(_T("Can't Create Desktop Shortcut"));
			break;
		}
	}
}

void CServerSelect::OnEditCopy()
{
	int n, i;
	CStringIndex index;
	CServerEntry Entry;
	CBuffer mbs;

	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	index.m_nIndex = _T("RLG510");
	index.SetNoCase(TRUE);
	index.SetNoSort(TRUE);

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;

		if ( (i = (int)m_List.GetItemData(n)) < 0 )
			continue;

		Entry = m_pData->GetAt(i);
		CRLoginDoc::LoadOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

		CRLoginDoc::SaveIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, index.Add());
	}

	index.SetJsonFormat(mbs, 0, JSON_TCODE);
	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText((LPCTSTR)mbs);
}

void CServerSelect::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CServerSelect::OnEditPaste()
{
	int n;
	CString str;
	CStringIndex index;
	CServerEntry Entry;

	if ( !((CMainFrame *)::AfxGetMainWnd())->CopyClipboardData(str) )
		return;

	index.SetNoCase(TRUE);
	index.SetNoSort(TRUE);

	if ( !index.GetJsonFormat(str) || _tcsncmp(index.m_nIndex, _T("RLG5"), 4) != 0 )
		return;

	for ( n = 0 ; n < index.GetSize() ; n++ ) {
		if ( index[n].Find(_T("Entry")) < 0 )
			continue;

		Entry.Init();
		CRLoginDoc::LoadInitOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

		CRLoginDoc::LoadIndex(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab, index[n]);

		Entry.m_Uid = (-1);
		Entry.m_bSelFlag = TRUE;
		CRLoginDoc::SaveOption(Entry, *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);

		EntryNameCheck(Entry);
		m_EntryNum = m_pData->AddEntry(Entry);
	}

	InitEntry(INIT_CALL_UPDATE);
}

BOOL CServerSelect::IsJsonEntryText(LPCTSTR str)
{
	int n;
	LPCTSTR ptn = _T(":[{");

	// "RLG510":[{

	while ( *str != _T('\0') && *str <= _T(' ') )
		str++;

	if ( _tcsncmp(str, _T("\"RLG5"), 5) != 0 )
		return FALSE;

	if ( str[5] < _T('0') || str[5] > _T('9') )
		return FALSE;

	if ( str[6] < _T('0') || str[6] > _T('9') )
		return FALSE;

	if ( str[7] != _T('"') )
		return FALSE;

	str += 8;

	for ( n = 0 ; ptn[n] != _T('\0') ; n++ ) {
		while ( *str != _T('\0') && *str <= _T(' ') )
			str++;

		if ( *str != ptn[n] )
			return FALSE;
		str++;
	}

	while ( *str != _T('\0') )
		str++;

	while ( *str <= _T(' ') )
		str--;

	if ( *str != _T(']') )
		return FALSE;
	str--;

	while ( *str <= _T(' ') )
		str--;

	if ( *str != _T('}') )
		return FALSE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CServerSelect::OpenTabEdit(int num)
{
	CRect rect;

	if ( num < 0 || num >= m_Tab.GetItemCount() || !m_Tab.GetItemRect(num, rect) )
		return FALSE;

	rect.InflateRect(4, 4);
	m_Tab.ClientToScreen(rect);
	ScreenToClient(rect);

	if ( num >= m_TabData.GetSize() || (m_pEditIndex = (CStringIndex *)m_TabData[num]) == NULL )
		return FALSE;

	m_EditWnd.Create(ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, ID_EDIT_BOX);

	m_EditWnd.GetWindowRect(m_EditRect);
	ScreenToClient(m_EditRect);
	m_Tab.GetWindowRect(m_EditMax);
	ScreenToClient(m_EditMax);
	m_EditNow = m_EditRect;

	m_EditWnd.SetWindowText(m_pEditIndex->m_nIndex);
	m_EditWnd.SetSel(0, -1);
	m_EditWnd.SetFocus();

	return TRUE;
}

void CServerSelect::OnKillfocusEditBox() 
{
	if ( m_pEditIndex != NULL ) {
		CString group;

		m_EditWnd.GetWindowText(group);

		if ( group.Compare(m_pEditIndex->m_nIndex) != 0 ) {
			UpdateGroupName(m_pEditIndex, group);

			m_Group.Empty();
			m_pEditIndex->GetPath(m_Group);

			InitEntry(INIT_CALL_UPDATE);
		}

		m_pEditIndex = NULL;
	}

	m_EditWnd.PostMessage(WM_CLOSE, 0, 0);
}

void CServerSelect::OnEnUpdateEditBox()
{
	CRect rect;
	CSize sz;
	CString text;
	CDC *pDC = m_EditWnd.GetDC();

	m_EditWnd.GetWindowText(text);
	text += _T("AA");

	sz = pDC->GetTextExtent(text);
	m_EditWnd.ReleaseDC(pDC);

	rect = m_EditNow;

	if ( sz.cx < rect.Width() ) {
		if ( sz.cx < m_EditRect.Width() )
			sz.cx = m_EditRect.Width();
		rect.left = m_EditRect.left;
		rect.right = rect.left + sz.cx;

	} else if ( sz.cx > rect.Width() ) {
		rect.left = m_EditRect.left;
		rect.right = rect.left + sz.cx;
	}

	if ( rect.right > m_EditMax.right ) {
		rect.left -= (rect.right - m_EditMax.right);
		rect.right = rect.left + sz.cx;
	}

	if ( rect.left < m_EditMax.left )
		rect.left = m_EditMax.left;

	if ( rect.left != m_EditNow.left || rect.Width() != m_EditNow.Width() ) {
		m_EditNow = rect;
		m_EditWnd.SetWindowPos(NULL, m_EditNow.left, m_EditNow.top, m_EditNow.Width(), m_EditNow.Height(), SWP_NOZORDER | SWP_DEFERERASE);
	}
}

BOOL CServerSelect::PreTranslateMessage(MSG* pMsg)
{
	CWnd *pWnd;

	if ( pMsg->message == WM_KEYDOWN && ((pWnd = GetFocus()) != NULL && pWnd->GetSafeHwnd() == m_List.GetSafeHwnd()) ) {
		if ( pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT ) {
			// Listにフォーカスがある場合は、左右キーでグループ移動
			if ( m_bShowTabWnd ) {
				pMsg->hwnd = m_Tab.GetSafeHwnd();
			} else if ( m_bShowTreeWnd ) {
				pMsg->wParam = (pMsg->wParam == VK_LEFT ? VK_UP : VK_DOWN);
				pMsg->hwnd = m_Tree.GetSafeHwnd();
			}
		} else if ( (GetKeyState(VK_CONTROL) & 0x80) != 0 ) {
			// Ctrl+UP/DOWNで移動
			int n;
			switch(pMsg->wParam) {
			case VK_UP:
				if ( (n = m_List.GetSelectionMark()) <= 0 )
					break;
				m_List.MoveItemText(n - 1, n);
				m_List.EnsureVisible(n - 1, 0);
				UpdateListIndex();
				return TRUE;
			case VK_DOWN:
				if ( (n = m_List.GetSelectionMark()) < 0 || (n + 1) >=  m_List.GetItemCount() )
					break;
				m_List.MoveItemText(n + 1, n);
				m_List.EnsureVisible(n + 1, 0);
				UpdateListIndex();
				return TRUE;
			}
		}

	} else if ( pMsg->message == WM_CHAR && pMsg->hwnd == m_List.GetSafeHwnd() ) {
		switch(pMsg->wParam) {
		case 0x03:		// Ctrl+C
			PostMessage(WM_COMMAND, ID_EDIT_COPY);
			break;
		case 0x16:		// Ctrl+V
			PostMessage(WM_COMMAND, ID_EDIT_PASTE);
			break;
		}

	} else if ( m_pEditIndex != NULL && pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE) ) {
		// グループ名編集中はDLGデフォルト動作しない
		if ( pMsg->wParam == VK_ESCAPE )
			m_pEditIndex = NULL;
		m_List.SetFocus();
		return TRUE;

	} else if ( pMsg->message >= WM_RBUTTONDOWN && pMsg->hwnd == GetSafeHwnd() ) {
		// Tabの空き領域でもNM_RCLICKを呼ぶ
		CRect rect;
		CPoint point(pMsg->lParam);
		::ClientToScreen(pMsg->hwnd, &point);
		m_Tab.GetWindowRect(rect);
		if ( rect.PtInRect(point) ) {
			m_Tab.ScreenToClient(&point);
			pMsg->hwnd = m_Tab.GetSafeHwnd();
			pMsg->lParam = MAKEWPARAM(point.x, point.y);
		}

	} else if ( pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST && pMsg->hwnd == m_Tab.GetSafeHwnd() ) {
		if ( m_EditWnd.GetSafeHwnd() != NULL ) {
			// 重なっているのでチェック
			CRect rect;
			CPoint point(pMsg->lParam);
			::ClientToScreen(pMsg->hwnd, &point);
			m_EditWnd.GetWindowRect(rect);
			if ( rect.PtInRect(point) ) {
				m_EditWnd.ScreenToClient(&point);
				pMsg->hwnd = m_EditWnd.GetSafeHwnd();
				pMsg->lParam = MAKEWPARAM(point.x, point.y);
			}

		} else if ( pMsg->message == WM_LBUTTONDOWN ) {
			// タブグループ名編集に移行
			int n;
			CRect rect;
			TCHITTESTINFO info;
			info.pt.x = LOWORD(pMsg->lParam);
			info.pt.y = HIWORD(pMsg->lParam);

			if ( (n = m_Tab.HitTest(&info)) >= 0 && n == m_Tab.GetCurSel() && OpenTabEdit(n) )
				return TRUE;
		}

	} else if ( pMsg->message == WM_PAINT && pMsg->hwnd == m_Tab.GetSafeHwnd() && m_EditWnd.GetSafeHwnd() != NULL ) {
		// 重なっているので再描画
		m_EditWnd.Invalidate(FALSE);
	}

	return CDialogExt::PreTranslateMessage(pMsg);
}

void CServerSelect::OnSize(UINT nType, int cx, int cy)
{
	SetItemOffset(cx, cy);
	CDialogExt::OnSize(nType, cx, cy);
	Invalidate(TRUE);
}

void CServerSelect::OnSizing(UINT fwSide, LPRECT pRect)
{
	//case WMSZ_LEFT:			// 1 Left edge
	//case WMSZ_RIGHT:			// 2 Right edge
	//case WMSZ_TOP:			// 3 Top edge
	//case WMSZ_TOPLEFT:		// 4 Top-left corner
	//case WMSZ_TOPRIGHT:		// 5 Top-right corner
	//case WMSZ_BOTTOM:			// 6 Bottom edge
	//case WMSZ_BOTTOMLEFT:		// 7 Bottom-left corner
	//case WMSZ_BOTTOMRIGHT:	// 8 Bottom-right corner

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

BOOL CServerSelect::GetTrackerRect(CRect &rect, CRect &move)
{
	CRect TreeRect, ListRect;

	if ( !m_bShowTreeWnd )
		return FALSE;

	m_List.GetWindowRect(ListRect);
	m_Tree.GetWindowRect(TreeRect);

	ScreenToClient(ListRect);
	ScreenToClient(TreeRect);

	rect.left   = TreeRect.right;
	rect.right  = ListRect.left;
	rect.top    = TreeRect.top;
	rect.bottom = TreeRect.bottom;

	move.left   = TreeRect.left  + 50;
	move.right  = ListRect.right - 50;
	move.top    = TreeRect.top;
	move.bottom = TreeRect.bottom;

	return TRUE;
}

void CServerSelect::InvertTracker(CRect &rect)
{
	CDC* pDC = GetDC();
	CBrush* pBrush = CDC::GetHalftoneBrush();
	HBRUSH hOldBrush = NULL;

	if (pBrush != NULL)
		hOldBrush = (HBRUSH)SelectObject(pDC->m_hDC, pBrush->m_hObject);

	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);

	if (hOldBrush != NULL)
		SelectObject(pDC->m_hDC, hOldBrush);

	ReleaseDC(pDC);
}

void CServerSelect::OffsetTracker(CPoint point)
{
	int w = m_TrackerRect.Width();

	m_TrackerRect.left  += (point.x - m_TrackerPoint.x);
	m_TrackerRect.right += (point.x - m_TrackerPoint.x);

	if ( m_TrackerRect.left < m_TrackerMove.left ) {
		m_TrackerRect.left  = m_TrackerMove.left;
		m_TrackerRect.right = m_TrackerRect.left + w;
	} else if ( m_TrackerRect.right > m_TrackerMove.right ) {
		m_TrackerRect.right = m_TrackerMove.right;
		m_TrackerRect.left  = m_TrackerRect.right - w;
	}
}

CStringIndex *CServerSelect::DragIndex(CPoint point)
{
	int n;
	UINT uFlags;
	HTREEITEM hItem;
	TCHITTESTINFO info;
	CStringIndex *pIndex = NULL;

	if ( m_bShowTreeWnd ) {
		m_Tree.ScreenToClient(&point);

		if ( (hItem = m_Tree.HitTest(point, &uFlags)) != NULL && (uFlags & TVHT_ONITEM) != 0 )
			pIndex = (CStringIndex *)m_Tree.GetItemData(hItem);

	} else if ( m_bShowTabWnd ) {
		m_Tab.ScreenToClient(&point);

		info.pt = point;
		info.flags = 0;
		if ( (n = m_Tab.HitTest(&info)) >= 0 && (info.flags & TCHT_ONITEM) != 0 && n < m_TabData.GetSize() )
			pIndex = (CStringIndex *)(m_TabData[n]);
	}

	return pIndex;
}

void CServerSelect::UpdateGroupName(CStringIndex *pIndex, LPCTSTR newName)
{
	int n;
	CString path;

	if ( newName != NULL ) {
		pIndex->GetPath(path);
		if ( (n = m_AddGroup.Find(path)) >= 0 )
			m_AddGroup.RemoveAt(n);

		pIndex->m_nIndex = newName;
		path.Empty();
		pIndex->GetPath(path);

		if ( pIndex->GetSize() <= 0 && pIndex->m_TabData.GetSize() <= 0 && (n = m_AddGroup.Find(path)) < 0 )
			m_AddGroup.Add(path);

	} else
		pIndex->GetPath(path);

	for ( n = 0 ; n < pIndex->m_TabData.GetSize() ; n++ ) {
		m_pData->GetAt(pIndex->m_TabData[n]).m_Group = path;
		m_pData->UpdateAt(pIndex->m_TabData[n]);
	}

	for ( n = 0 ; n < pIndex->GetSize() ; n++ )
		UpdateGroupName(&((*pIndex)[n]), NULL);
}

void CServerSelect::SetIndexList(CStringIndex *pIndex, BOOL bSelect)
{
	int n;
	BOOL bForcus = FALSE;

	m_Group.Empty();
	pIndex->GetPath(m_Group);
	m_EntryNum = (-1);
	InitList(pIndex, FALSE);

	if ( !bSelect )
		return;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( (int)m_List.GetItemData(n) >= 0 ) {
			if ( !bForcus ) {
				m_List.SetItemState(n, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				bForcus = TRUE;
			} else
				m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
		} else
			m_List.SetItemState(n, NULL, LVIS_SELECTED);
	}

	m_List.SetFocus();
}

void CServerSelect::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ( m_pEditIndex != NULL )
		m_List.SetFocus();

	if ( GetTrackerRect(m_TrackerRect, m_TrackerMove) && m_TrackerRect.PtInRect(point) ) {
		InvertTracker(m_TrackerRect);
		m_TrackerPoint = point;
		m_bTrackerActive = TRUE;
		SetCapture();
	}

	CDialogExt::OnLButtonDown(nFlags, point);
}

void CServerSelect::OnMouseMove(UINT nFlags, CPoint point)
{
	if ( m_bTrackerActive ) {
		InvertTracker(m_TrackerRect);
		OffsetTracker(point);
		InvertTracker(m_TrackerRect);
		m_TrackerPoint = point;

	} else if ( m_bDragList ) {
		CPoint po = point;
		ClientToScreen(&po);
		int image = (DragIndex(po) != NULL || m_List.ItemHitTest(po) >= 0 ? m_DragImage : 8);

		if ( m_DragActive == image )
			m_ImageList.DragMove(po);
		else {
			m_ImageList.DragLeave(GetDesktopWindow());
			m_ImageList.EndDrag();
			m_ImageList.BeginDrag(image, CPoint(12, 8));
			m_ImageList.DragEnter(GetDesktopWindow(), po);
			m_DragActive = image;
		}
	}

	CDialogExt::OnMouseMove(nFlags, point);
}

void CServerSelect::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ( m_bTrackerActive ) {
		InvertTracker(m_TrackerRect);
		OffsetTracker(point);
		m_bTrackerActive = FALSE;
		ReleaseCapture();

		CRect rect;
		GetClientRect(rect);
		m_TreeListPer = m_TrackerRect.left * 1000 / rect.Width();
		SetItemOffset(rect.Width(), rect.Height());

	} else if ( m_bDragList ) {
		m_ImageList.DragLeave(GetDesktopWindow());
		m_ImageList.EndDrag();
		m_bDragList = FALSE;
		ReleaseCapture();

		int n, i;
		CPoint po = point;
		CStringIndex *pIndex;

		ClientToScreen(&po);
		if ( (pIndex = DragIndex(po)) != NULL ) {
			m_Group.Empty();
			pIndex->GetPath(m_Group);
			m_EntryNum = m_DragNumber;
			for ( i = 0 ; i < m_List.GetItemCount() ; i++ ) {
				if ( m_List.GetItemState(i, LVIS_SELECTED) != 0 && (n = (int)m_List.GetItemData(i)) >= 0 ) {
					m_pData->GetAt(n).m_bSelFlag = TRUE;
					m_pData->GetAt(n).m_Group = m_Group;
					m_pData->UpdateAt(n);
				}
			}
			InitEntry(INIT_CALL_UPDATE);

		} else if ( (n = m_List.ItemHitTest(po)) >= 0 ) {
			for ( i = 0 ; i < m_List.GetItemCount() ; i++ ) {
				if ( m_List.GetItemState(i, LVIS_SELECTED) != 0 ) {
					if ( i > n )
						m_List.MoveItemText(i, n++);
					else if ( i < n )
						m_List.MoveItemText(i--, n);
				}
			}

			UpdateListIndex();
		}
	}

	CDialogExt::OnLButtonUp(nFlags, point);
}

BOOL CServerSelect::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CRect rect, move;
	CPoint point;

	GetCursorPos(&point);
	ScreenToClient(&point);

	if ( !m_bTrackerActive && GetTrackerRect(rect, move) && rect.PtInRect(point) ) {
		LPCTSTR id = ATL_MAKEINTRESOURCE(AFX_IDC_HSPLITBAR);
		HINSTANCE hInst = AfxFindResourceHandle(id, ATL_RT_GROUP_CURSOR);
		HCURSOR hCursor = NULL;

		if ( hInst != NULL )
			hCursor = ::LoadCursorW(hInst, id);

		if ( hCursor == NULL )
			hCursor = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);

		if ( hCursor != NULL )
			::SetCursor(hCursor);

		return TRUE;
	}

	return CDialogExt::OnSetCursor(pWnd, nHitTest, message);
}

/////////////////////////////////////////////////////////////////////////////

void CServerSelect::OnDblclkServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	PostMessage(WM_COMMAND, IDOK);
}

void CServerSelect::OnLvnBegindragServerlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( m_pEditIndex != NULL )
		m_List.SetFocus();

	m_DragNumber = (int)m_List.GetItemData(pNMLV->iItem);

	if ( m_DragNumber < 0 ) //|| (!m_bShowTreeWnd && !m_bShowTabWnd) )
		return;

	SetCapture();
	m_bDragList = TRUE;
	m_DragImage = (m_List.GetSelectedCount() > 1 ? 7 : 2);
	m_DragActive = m_DragImage;

	m_ImageList.BeginDrag(m_DragImage, CPoint(12, 8));
	m_ImageList.DragEnter(GetDesktopWindow(), pNMLV->ptAction);
}

/////////////////////////////////////////////////////////////////////////////

void CServerSelect::OnTcnSelchangeServertab(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	if ( m_pEditIndex != NULL )
		m_Tab.SetFocus();

	int n = m_Tab.GetCurSel();

	if ( n >= 0 && n < m_TabData.GetSize() ) {
		CStringIndex *pIndex = (CStringIndex *)m_TabData[n];
		m_Group.Empty();
		pIndex->GetPath(m_Group);
		m_EntryNum = (-1);
		InitEntry(INIT_CALL_NONE);
	}
}

void CServerSelect::OnNMRClickServertab(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	if ( m_pEditIndex != NULL )
		m_Tab.SetFocus();

	int n, id;
	TCHITTESTINFO info;
	CStringIndex *pIndex = NULL;
	CMenuLoad PopUpMenu;
	CMenu *pSubMenu;
	CString path;

	if ( !PopUpMenu.LoadMenu(IDR_POPUPMENU) || (pSubMenu = PopUpMenu.GetSubMenu(8)) == NULL )
		return;

	if ( !GetCursorPos(&info.pt) )
		return;

	m_Tab.ScreenToClient(&info.pt);

	if ( (n = m_Tab.HitTest(&info)) >= 0 && n < m_Tab.GetItemCount() && n < m_TabData.GetSize() && (pIndex = (CStringIndex *)m_TabData[n]) != NULL ) {
		if ( m_Tab.GetCurSel() != n ) {
			m_Tab.SetCurSel(n);
			m_Group.Empty();
			pIndex->GetPath(m_Group);
			m_EntryNum = (-1);
			InitList(pIndex, FALSE);
		}

		if ( pIndex->m_TabData.GetSize() > 0 || pIndex->GetSize() > 0 )
			pSubMenu->EnableMenuItem(IDM_TREECTRLDELETE, MF_DISABLED);
		if ( pIndex->m_TabData.GetSize() <= 0 )
			pSubMenu->EnableMenuItem(IDM_TREECTRLSELECT, MF_DISABLED);

	} else {
		pSubMenu->EnableMenuItem(IDM_TREECTRLRENAME, MF_DISABLED);
		pSubMenu->EnableMenuItem(IDM_TREECTRLDELETE, MF_DISABLED);
		pSubMenu->EnableMenuItem(IDM_TREECTRLSELECT, MF_DISABLED);
	}

	pSubMenu->EnableMenuItem(IDM_TREECTRLEXPAND, MF_DISABLED);

	m_Tab.ClientToScreen(&info.pt);
	id = ((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(pSubMenu, TPM_NONOTIFY | TPM_RETURNCMD, info.pt.x, info.pt.y, this, NULL);

	switch(id) {
	case IDM_TREECTRLNEW:
		m_Group = _T("NewGroup");
		for ( n = 1 ; m_TabEntry.FindPath(m_Group) != NULL ; n++ )
			m_Group.Format(_T("NewGroup%d"), n);
		m_AddGroup.Add(m_Group);
		m_EntryNum = (-1);
		InitEntry(INIT_CALL_AND_EDIT);
		break;

	case IDM_TREECTRLRENAME:
		OpenTabEdit(n);
		break;

	case IDM_TREECTRLDELETE:
		pIndex->GetPath(path);
		if ( (n = m_AddGroup.Find(path)) >= 0 )
			m_AddGroup.RemoveAt(n);
		InitEntry(INIT_CALL_UPDATE);
		break;

	case IDM_TREECTRLSELECT:
		m_Tab.SetCurSel(n);
		SetIndexList(pIndex, TRUE);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////

void CServerSelect::OnTvnSelchangedServertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	CStringIndex *pIndex;
	HTREEITEM hti = m_Tree.GetSelectedItem();
	//LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if ( !m_bTreeUpdate && hti != NULL && (pIndex = (CStringIndex *)m_Tree.GetItemData(hti)) != NULL ) {
		m_Group.Empty();
		pIndex->GetPath(m_Group);
		m_EntryNum = (-1);
		InitList(pIndex, FALSE);
	}
}

void CServerSelect::OnTvnBeginlabeleditServertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	if ( pTVDispInfo->item.hItem != NULL )
		m_pEditIndex = (CStringIndex *)m_Tree.GetItemData(pTVDispInfo->item.hItem);
}

void CServerSelect::OnTvnEndlabeleditServertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	if ( pTVDispInfo->item.hItem != NULL && pTVDispInfo->item.pszText != NULL && m_pEditIndex != NULL && (CStringIndex *)m_Tree.GetItemData(pTVDispInfo->item.hItem) == m_pEditIndex ) {
		UpdateGroupName(m_pEditIndex, pTVDispInfo->item.pszText);

		m_Group.Empty();
		m_pEditIndex->GetPath(m_Group);

		InitEntry(INIT_CALL_UPDATE);
	}

	m_pEditIndex = NULL;
}

void CServerSelect::OnNMRClickServertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	int n, id;
	CMenuLoad PopUpMenu;
	CMenu *pSubMenu;
	UINT uFlags;
	CPoint point;
	HTREEITEM hTree;
	CStringIndex *pIndex = NULL;
	CString path;

	if ( !PopUpMenu.LoadMenu(IDR_POPUPMENU) || (pSubMenu = PopUpMenu.GetSubMenu(8)) == NULL )
		return;

	if ( !GetCursorPos(&point) )
		return;

	m_Tree.ScreenToClient(&point);

	if ( (hTree = m_Tree.HitTest(point, &uFlags)) != NULL && (uFlags & TVHT_ONITEM) != 0 && (pIndex = (CStringIndex *)m_Tree.GetItemData(hTree)) != NULL ) {
		if ( m_Tree.GetSelectedItem() != hTree ) {
			m_Tree.SelectItem(hTree);
			m_Group.Empty();
			pIndex->GetPath(m_Group);
			m_EntryNum = (-1);
			InitList(pIndex, FALSE);
		}

		if ( pIndex->m_TabData.GetSize() > 0 || pIndex->GetSize() > 0 )
			pSubMenu->EnableMenuItem(IDM_TREECTRLDELETE, MF_DISABLED);
		if ( pIndex->GetSize() <= 0 )
			pSubMenu->EnableMenuItem(IDM_TREECTRLEXPAND, MF_DISABLED);
		if ( pIndex->m_TabData.GetSize() <= 0 )
			pSubMenu->EnableMenuItem(IDM_TREECTRLSELECT, MF_DISABLED);

	} else {
		pSubMenu->EnableMenuItem(IDM_TREECTRLRENAME, MF_DISABLED);
		pSubMenu->EnableMenuItem(IDM_TREECTRLDELETE, MF_DISABLED);
		pSubMenu->EnableMenuItem(IDM_TREECTRLSELECT, MF_DISABLED);
		hTree = m_Tree.GetRootItem();
	}

	m_Tree.ClientToScreen(&point);
	id = ((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(pSubMenu, TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, this, NULL);

	switch(id) {
	case IDM_TREECTRLNEW:
		if ( pIndex != NULL ) {
			pIndex->GetPath(path);
			m_Group.Format(_T("%s\\SubGroup"), path);
			for ( n = 1 ; m_TabEntry.FindPath(m_Group) != NULL ; n++ )
				m_Group.Format(_T("%s\\SubGroup%d"), path, n);
		} else {
			m_Group = _T("NewGroup");
			for ( n = 1 ; m_TabEntry.FindPath(m_Group) != NULL ; n++ )
				m_Group.Format(_T("NewGroup%d"), n);
		}
		m_AddGroup.Add(m_Group);
		m_EntryNum = (-1);
		InitEntry(INIT_CALL_AND_EDIT);
		break;

	case IDM_TREECTRLRENAME:
		m_Tree.EditLabel(hTree);
		break;

	case IDM_TREECTRLDELETE:
		pIndex->GetPath(path);
		if ( (n = m_AddGroup.Find(path)) >= 0 )
			m_AddGroup.RemoveAt(n);
		InitEntry(INIT_CALL_UPDATE);
		break;

	case IDM_TREECTRLSELECT:
		m_Tree.SelectItem(hTree);
		SetIndexList(pIndex, TRUE);
		break;

	case IDM_TREECTRLEXPAND:
		InitExpand(hTree, TVE_TOGGLE);
		break;
	}
}

void CServerSelect::OnTvnItemexpandedServertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	switch(pNMTreeView->action) {
	case TVE_COLLAPSE:
		TreeExpandUpdate(pNMTreeView->itemNew.hItem, FALSE);
		break;
	case TVE_EXPAND:
		TreeExpandUpdate(pNMTreeView->itemNew.hItem, TRUE);
		break;
	}
}

