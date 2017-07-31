// ListCtrlExt.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
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
	m_SubMenuPos  = (-1);
	m_EditSubItem = 0;
	m_bSort = TRUE;
	m_bMove = FALSE;
}
CListCtrlExt::~CListCtrlExt()
{
}

BEGIN_MESSAGE_MAP(CListCtrlExt, CListCtrl)
	ON_WM_VSCROLL()
	ON_WM_KEYDOWN()
	ON_EN_KILLFOCUS(ID_EDIT_BOX, OnKillfocusEditBox)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnLvnBegindrag)
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, OnDblclk)
END_MESSAGE_MAP()

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int i1, i2, it;
	CStringLoad s1, s2;
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
	if ( (it = s1.CompareDigit(s2)) != 0 || pCompList->m_SortSubItem == pCompList->m_SortDupItem )
		return it;

	s1 = pCompList->GetItemText(i1, pCompList->m_SortDupItem);
	s2 = pCompList->GetItemText(i2, pCompList->m_SortDupItem);
	return s1.CompareDigit(s2);
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
		tmp.cx = MulDiv(tmp.cx, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY, 96);
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

	while ( GetColumn(n++, &tmp) ) {
		tmp.cx = MulDiv(tmp.cx, 96, ((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY);
		AfxGetApp()->WriteProfileInt(lpszSection, tmp.pszText, tmp.cx);
	}

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
	m_SubMenuPos = Pos;
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

	if ( m_SubMenuPos < 0 )
		return FALSE;

	CMenu *pSubMenu = m_PopUpMenu.GetSubMenu(m_SubMenuPos);
	CCmdUI state;
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CPoint point = pNMListView->ptAction;
	CWnd *pOwner = GetOwner();

	if ( pOwner == NULL || pSubMenu == NULL )
		return FALSE;

	state.m_pMenu = pSubMenu;
	state.m_nIndexMax = pSubMenu->GetMenuItemCount();
	for ( state.m_nIndex = 0 ; state.m_nIndex < state.m_nIndexMax ; state.m_nIndex++) {
		if ( (int)(state.m_nID = pSubMenu->GetMenuItemID(state.m_nIndex)) <= 0 )
			continue;
		state.m_pSubMenu = NULL;
		state.DoUpdate(pOwner, TRUE);
	}

	ClientToScreen(&point);
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, pOwner);
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
	int i, x, a;
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
	} else if ( m_SubMenuPos >= 0 && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_APPS ) {
		int n, flag;
		CCmdUI state;
		CPoint point;
		CRect rect;
		CWnd *pOwner = GetOwner();
		CMenu *pSubMenu = m_PopUpMenu.GetSubMenu(m_SubMenuPos);

		state.m_pMenu = pSubMenu;
		state.m_nIndexMax = pSubMenu->GetMenuItemCount();
		for ( state.m_nIndex = 0 ; state.m_nIndex < state.m_nIndexMax ; state.m_nIndex++) {
			if ( (int)(state.m_nID = pSubMenu->GetMenuItemID(state.m_nIndex)) <= 0 )
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

		pSubMenu->TrackPopupMenu(flag | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, pOwner);
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

/////////////////////////////////////////////////////////////////////////////

void CListCtrlExt::SwapItemText(int src, int dis)
{
	int n;
	CString text[2];
	BOOL bchk[2];
	LVCOLUMN lvc;
	LVITEM item[2];

	ZeroMemory(&lvc, sizeof(lvc));
	ZeroMemory(item, sizeof(LVITEM) * 2);

	lvc.mask = LVCF_WIDTH;
	for ( n = 0 ; GetColumn(n, &lvc) ; n++ ) {
		text[0] = GetItemText(src, n);
		text[1] = GetItemText(dis, n);

		SetItemText(src, n, text[1]);
		SetItemText(dis, n, text[0]);
	}

	item[0].mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	item[0].iItem = src;
	GetItem(&(item[0]));

	item[1].mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	item[1].iItem = dis;
	GetItem(&(item[1]));

	bchk[0] = GetLVCheck(src);
	bchk[1] = GetLVCheck(dis);

	item[0].iItem = dis;
	SetItem(&(item[0]));

	item[1].iItem = src;
	SetItem(&(item[1]));

	SetLVCheck(src, bchk[1]);
	SetLVCheck(dis, bchk[0]);
}
void CListCtrlExt::MoveItemText(int src, int dis)
{
	int n;
	CStringArray text;
	BOOL bchk;
	LVCOLUMN lvc;
	LVITEM item;

	ZeroMemory(&lvc, sizeof(lvc));
	ZeroMemory(&item, sizeof(item));

	lvc.mask = LVCF_WIDTH;
	for ( n = 0 ; GetColumn(n, &lvc) ; n++ )
		text.Add(GetItemText(src, n));

	item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	item.iItem = src;
	GetItem(&item);

	bchk = GetLVCheck(src);

	DeleteItem(src);
	InsertItem(dis, text[0]);

	for ( n = 1 ; GetColumn(n, &lvc) ; n++ )
		SetItemText(dis, n, text[n]);

	item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	item.iItem = dis;
	SetItem(&item);

	SetLVCheck(dis, bchk);
}

void CListCtrlExt::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int n, m;

	if ( !m_bMove ) {
		CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	switch(nChar) {
	case VK_UP:
		if ( (GetKeyState(VK_SHIFT) & 0x80) == 0 )
			break;
	case VK_ADD:
		if ( (n = GetSelectionMark()) <= 0 )
			break;
		SwapItemText(n - 1, n);
		SetSelectionMark(n - 1);
		SetItemState(n - 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		EnsureVisible(n - 1, 0);
		return;
	case VK_DOWN:
		if ( (GetKeyState(VK_SHIFT) & 0x80) == 0 )
			break;
	case VK_SUBTRACT:
		if ( (n = GetSelectionMark()) < 0 || (n + 1) >=  GetItemCount() )
			break;
		SwapItemText(n + 1, n);
		SetSelectionMark(n + 1);
		SetItemState(n + 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		EnsureVisible(n + 1, 0);
		return;

	case VK_HOME:
		if ( (GetKeyState(VK_SHIFT) & 0x80) == 0 )
			break;
		if ( (n = GetSelectionMark()) <= 0 )
			break;
		MoveItemText(n, 0);
		SetSelectionMark(0);
		SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		EnsureVisible(0, 0);
		return;
	case VK_END:
		if ( (GetKeyState(VK_SHIFT) & 0x80) == 0 )
			break;
		if ( (m = GetItemCount() - 1) < 0 )
			break;
		if ( (n = GetSelectionMark()) < 0 || n >= m )
			break;
		MoveItemText(n, m);
		SetSelectionMark(m);
		SetItemState(m, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		EnsureVisible(m, 0);
		return;
	}

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

class CRectTrackerList : public CRectTracker
{
public:
	CListCtrlExt *m_pOwner;

	virtual void OnChangedRect(const CRect& rectOld);
};

void CRectTrackerList::OnChangedRect(const CRect& rectOld)
{
	CPoint point;
	CRect rect;

	if ( !GetCursorPos(&point) )
		return;

	m_pOwner->ScreenToClient(&point);
	m_pOwner->GetClientRect(rect);

	if ( point.y < 0 )
		m_pOwner->Scroll(CSize(0, point.y));
	else if ( point.y > rect.Height() )
		m_pOwner->Scroll(CSize(0, point.y - rect.Height()));
}

void CListCtrlExt::OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	int item, y;
	CRect rect;
	CRectTrackerList tracker;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( !m_bMove )
		return;

	GetItemRect(pNMLV->iItem, rect, LVIR_LABEL);
	tracker.m_rect = rect;
	tracker.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;
	tracker.m_pOwner = this;

	if ( tracker.Track(this, pNMLV->ptAction, FALSE, NULL) ) {
		y = pNMLV->ptAction.y + (tracker.m_rect.top - rect.top);

		for ( item = GetTopIndex() ; item < GetItemCount() ; item++ ) {
			if ( !GetItemRect(item, rect, LVIR_LABEL) )
				break;
			if ( y < rect.top )
				break;
			if ( y >= rect.top && y <= rect.bottom )
				break;
		}

		if ( item != pNMLV->iItem ) {
			if ( item > pNMLV->iItem )
				item--;
			MoveItemText(pNMLV->iItem, item);
			SetSelectionMark(item);
			SetItemState(item, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			EnsureVisible(item, 0);
		}
	}

	Invalidate(FALSE);

	*pResult = 0;
}