// ListCtrlExt.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "ListCtrlExt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt

CListCtrlExt::CListCtrlExt()
{
	m_SortSubItem = 0;
	m_SortReverse = 0;
	m_SortDupItem = 0;
	m_pSubMenu    = NULL;
}
CListCtrlExt::~CListCtrlExt()
{
}

BEGIN_MESSAGE_MAP(CListCtrlExt, CListCtrl)
	//{{AFX_MSG_MAP(CListCtrlExt)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, OnRclick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int i1, i2, it;
	CString s1, s2;
	LV_FINDINFO lvinfo;
	CListCtrlExt *pCompList = (CListCtrlExt *)lParamSort;

	lvinfo.flags = LVFI_PARAM;
	lvinfo.lParam = lParam1;
	i1 = pCompList->FindItem(&lvinfo);
	lvinfo.lParam = lParam2;
	i2 = pCompList->FindItem(&lvinfo);

	if ( i1 == (-1) || i2 == (-1) )
		return 0;

	if ( pCompList->m_SortReverse ) {
		it = i1;
		i1 = i2;
		i2 = it;
	}

	s1 = pCompList->GetItemText(i1, pCompList->m_SortSubItem);
	s2 = pCompList->GetItemText(i2, pCompList->m_SortSubItem);
	if ( (it = s1.Compare(s2)) != 0 || pCompList->m_SortSubItem == pCompList->m_SortDupItem )
		return it;

	s1 = pCompList->GetItemText(i1, pCompList->m_SortDupItem);
	s2 = pCompList->GetItemText(i2, pCompList->m_SortDupItem);
	return s1.Compare(s2);
}

int CListCtrlExt::GetSelectMarkData()
{
	int n;
	if ( (n = GetSelectionMark()) >= 0 )
		return (int)GetItemData(n);
	return (-1);
}
void CListCtrlExt::DoSortItem()
{
	SortItems(CompareFunc, (DWORD_PTR)this);
}
void CListCtrlExt::InitColumn(LPCSTR lpszSection, const LV_COLUMN *lpColumn, int nMax)
{
	int n;
	LV_COLUMN tmp;

	for ( n = 0 ; n < nMax ; n++ ) {
		tmp = lpColumn[n];
		tmp.cx = AfxGetApp()->GetProfileInt(lpszSection, tmp.pszText, tmp.cx);
		InsertColumn(n, &tmp);
	}

	m_SortSubItem = AfxGetApp()->GetProfileInt(lpszSection, "SortItem", 0);
	m_SortReverse = AfxGetApp()->GetProfileInt(lpszSection, "SortRevs", 0);
	m_SortDupItem = AfxGetApp()->GetProfileInt(lpszSection, "SortDups", 0);
}
void CListCtrlExt::SaveColumn(LPCSTR lpszSection)
{
	int n = 0;
	LV_COLUMN tmp;
	TCHAR name[256];

	memset(&tmp, 0, sizeof(tmp));
	tmp.pszText = name;
	tmp.cchTextMax = 256;
	tmp.mask = LVCF_WIDTH | LVCF_TEXT;
	while ( GetColumn(n++, &tmp) )
		AfxGetApp()->WriteProfileInt(lpszSection, tmp.pszText, tmp.cx);

	AfxGetApp()->WriteProfileInt(lpszSection, "SortItem", m_SortSubItem);
	AfxGetApp()->WriteProfileInt(lpszSection, "SortRevs", m_SortReverse);
	AfxGetApp()->WriteProfileInt(lpszSection, "SortDups", m_SortDupItem);
}
void CListCtrlExt::SetLVCheck(WPARAM ItemIndex, BOOL bCheck)
{
	ListView_SetItemState(m_hWnd, ItemIndex, UINT((int(bCheck) + 1) << 12), LVIS_STATEIMAGEMASK);
}
BOOL CListCtrlExt::GetLVCheck(WPARAM ItemIndex)
{
	return ListView_GetCheckState(m_hWnd, ItemIndex);
}
void CListCtrlExt::SetPopUpMenu(UINT nIDResource, int Pos)
{
	m_PopUpMenu.LoadMenu(nIDResource);
	m_pSubMenu = m_PopUpMenu.GetSubMenu(Pos);
}

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt メッセージ ハンドラ

void CListCtrlExt::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if ( m_SortSubItem == pNMListView->iSubItem )
		m_SortReverse ^= 1;
	else
		m_SortReverse = 0;
	m_SortSubItem = pNMListView->iSubItem;
	SortItems(CompareFunc, (DWORD_PTR)this);
	*pResult = 0;
}
BOOL CListCtrlExt::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	if ( m_pSubMenu == NULL )
		return FALSE;

	CCmdUI state;
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CPoint point = pNMListView->ptAction;
	CWnd *pOwner = GetOwner();

	if ( pOwner == NULL )
		return FALSE;

	state.m_pMenu = m_pSubMenu;
	state.m_nIndexMax = m_pSubMenu->GetMenuItemCount();
	for ( state.m_nIndex = 0 ; state.m_nIndex < state.m_nIndexMax ; state.m_nIndex++) {
		if ( (int)(state.m_nID = m_pSubMenu->GetMenuItemID(state.m_nIndex)) <= 0 )
			continue;
		state.m_pSubMenu = NULL;
		state.DoUpdate(pOwner, TRUE);
	}

	ClientToScreen(&point);
	m_pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, pOwner);
	return TRUE;
}
