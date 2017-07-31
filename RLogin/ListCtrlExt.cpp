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
	m_EditSubItem = 0;
	m_bSort = TRUE;
}
CListCtrlExt::~CListCtrlExt()
{
}

BEGIN_MESSAGE_MAP(CListCtrlExt, CListCtrl)
	//{{AFX_MSG_MAP(CListCtrlExt)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, OnDblclk)
	ON_WM_VSCROLL()
	ON_EN_KILLFOCUS(ID_EDIT_BOX, OnKillfocusEditBox)
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
int CListCtrlExt::GetParamItem(int para)
{
	LV_FINDINFO lvinfo;

	lvinfo.flags  = LVFI_PARAM;
	lvinfo.lParam = para;
	return FindItem(&lvinfo);
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
void CListCtrlExt::InitColumn(LPCTSTR lpszSection, const LV_COLUMN *lpColumn, int nMax)
{
	int n;
	LV_COLUMN tmp;

	for ( n = 0 ; n < nMax ; n++ ) {
		tmp = lpColumn[n];
		tmp.cx = AfxGetApp()->GetProfileInt(lpszSection, tmp.pszText, tmp.cx);
		InsertColumn(n, &tmp);
	}

	m_SortSubItem = AfxGetApp()->GetProfileInt(lpszSection, _T("SortItem"), 0);
	m_SortReverse = AfxGetApp()->GetProfileInt(lpszSection, _T("SortRevs"), 0);
	m_SortDupItem = AfxGetApp()->GetProfileInt(lpszSection, _T("SortDups"), 0);
}
void CListCtrlExt::SaveColumn(LPCTSTR lpszSection)
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

	AfxGetApp()->WriteProfileInt(lpszSection, _T("SortItem"), m_SortSubItem);
	AfxGetApp()->WriteProfileInt(lpszSection, _T("SortRevs"), m_SortReverse);
	AfxGetApp()->WriteProfileInt(lpszSection, _T("SortDups"), m_SortDupItem);
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

	*pResult = 0;

	if ( !m_bSort )
		return;

	if ( m_SortSubItem == pNMListView->iSubItem )
		m_SortReverse ^= 1;
	else
		m_SortReverse = 0;

	m_SortSubItem = pNMListView->iSubItem;
	SortItems(CompareFunc, (DWORD_PTR)this);
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
void CListCtrlExt::OpenEditBox(int item, int num, int fmt, CRect &rect)
{
	CString tmp = GetItemText(item, num);

	if ( m_EditWnd.m_hWnd != NULL )
		m_EditWnd.DestroyWindow();

	rect.InflateRect(4, 4);
	if ( rect.left <= 0 )
		rect.left = 0;

	ClientToScreen(rect);
	ScreenToClient(rect);

	m_EditWnd.Create((fmt & LVCFMT_RIGHT ? ES_RIGHT : 0) | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, ID_EDIT_BOX);
	m_EditWnd.SetWindowText(tmp);
	m_EditWnd.SetSel(0, -1);
	m_EditWnd.SetFocus();

	m_EditItem = item;
	m_EditNum  = num;
	m_EditOld  = tmp;
	m_EditFlag = TRUE;
}
void CListCtrlExt::EditItem(int item, int num)
{
	int n, i, x, a;
	CRect rect;
	LVCOLUMN lvc;

	EnsureVisible(item, FALSE);
	if ( !GetItemRect(item, rect, LVIR_LABEL) )
		return;
	lvc.mask = LVCF_WIDTH | LVCF_FMT ;
	x = 0 - GetScrollPos(SB_HORZ);
	for ( i = 0 ; GetColumn(i, &lvc) ; i++, x += a ) {
		a =  lvc.cx;
		if ( i == num ) {
			rect.left  = x;
			rect.right = x + a;
			OpenEditBox(item, num, lvc.fmt, rect);
			break;
		}
	}
}
BOOL CListCtrlExt::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n, i, x, a;
	CRect rect;
	LVCOLUMN lvc;
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if ( m_EditSubItem == 0 )
		return FALSE;

	for ( n = GetTopIndex() ; n < GetItemCount() ; n++ ) {
		if ( !GetItemRect(n, rect, LVIR_LABEL) )
			break;
		if ( pNMListView->ptAction.y >= rect.top && pNMListView->ptAction.y < rect.bottom ) {
			lvc.mask = LVCF_WIDTH | LVCF_FMT ;
			x = 0 - GetScrollPos(SB_HORZ);
			for ( i = 0 ; GetColumn(i, &lvc) ; i++, x += a ) {
				a =  lvc.cx;
				if ( pNMListView->ptAction.x >= x && pNMListView->ptAction.x < (x + a) && (m_EditSubItem & (1 << i)) != 0 ) {
					rect.left  = x;
					rect.right = x + a;
					OpenEditBox(n, i, lvc.fmt, rect);
					break;
				}
			}
			break;
		}
	}
	
	*pResult = 0;
	return TRUE;
}
void CListCtrlExt::OnKillfocusEditBox() 
{
	CString str;

	if ( m_EditFlag == FALSE )
		return;

	m_EditWnd.GetWindowText(str);
	SetItemText(m_EditItem, m_EditNum, str);
	m_EditWnd.PostMessage(WM_CLOSE, 0, 0);
	m_EditFlag = FALSE;

	NMLISTVIEW nmHead;
	CWnd *pWnd = GetOwner();

	if ( pWnd != NULL ) {
		memset(&nmHead, 0, sizeof(NMLISTVIEW));

		nmHead.hdr.code = LVN_ITEMCHANGED;
		nmHead.hdr.idFrom = GetDlgCtrlID();
		nmHead.hdr.hwndFrom = m_hWnd;

		nmHead.iItem    = m_EditItem;
		nmHead.iSubItem = m_EditNum;
		nmHead.uChanged = LVIF_TEXT;

		pWnd->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)(&nmHead));
	}
}
BOOL CListCtrlExt::PreTranslateMessage(MSG* pMsg) 
{
	if ( m_EditWnd.m_hWnd != NULL ) {
		if ( pMsg->message == WM_CHAR && pMsg->wParam == 0x0D ) {
			m_EditWnd.PostMessage(WM_KILLFOCUS, 0, 0);
			return TRUE;
		} else if ( pMsg->message == WM_CHAR && pMsg->wParam == VK_ESCAPE ) {
			m_EditWnd.PostMessage(WM_CLOSE, 0, 0);
			m_EditFlag = FALSE;
			return TRUE;
		} else if ( pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST ) {
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;
		}
	} else if ( m_pSubMenu != NULL && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_APPS ) {
		int n, flag;
		CCmdUI state;
		CPoint point;
		CRect rect;
		CWnd *pOwner = GetOwner();

		state.m_pMenu = m_pSubMenu;
		state.m_nIndexMax = m_pSubMenu->GetMenuItemCount();
		for ( state.m_nIndex = 0 ; state.m_nIndex < state.m_nIndexMax ; state.m_nIndex++) {
			if ( (int)(state.m_nID = m_pSubMenu->GetMenuItemID(state.m_nIndex)) <= 0 )
				continue;
			state.m_pSubMenu = NULL;
			state.DoUpdate(pOwner, TRUE);
		}


		if ( (n = GetSelectionMark()) >= 0 && GetItemRect(n, rect, LVIR_LABEL) ) {
			point.x = (rect.left + rect.right) / 2;
			point.y = (rect.top + rect.bottom) / 2;
			ClientToScreen(&point);
			flag = TPM_LEFTALIGN | TPM_LEFTBUTTON;
		} else {
			pOwner->GetWindowRect(rect);
			point.x = (rect.left + rect.right) / 2;
			point.y = (rect.top + rect.bottom) / 2;
			flag = TPM_CENTERALIGN | TPM_VCENTERALIGN;
		}

		m_pSubMenu->TrackPopupMenu(flag | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, pOwner);
		return TRUE;
	}
	return CListCtrl::PreTranslateMessage(pMsg);
}

void CListCtrlExt::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if ( m_EditWnd.m_hWnd != NULL ) {
		m_EditWnd.SendMessage(WM_KILLFOCUS, 0, 0);
		m_EditWnd.DestroyWindow();
	}
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}
