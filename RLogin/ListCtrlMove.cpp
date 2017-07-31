// ListCtrlMove.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "ListCtrlMove.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListCtrlMove

CListCtrlMove::CListCtrlMove()
{
}

CListCtrlMove::~CListCtrlMove()
{
}


BEGIN_MESSAGE_MAP(CListCtrlMove, CListCtrl)
	//{{AFX_MSG_MAP(CListCtrlMove)
	ON_WM_KEYDOWN()
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CListCtrlMove::OnLvnBegindrag)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CListCtrlMove::OnLvnBegindrag)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListCtrlMove メッセージ ハンドラ

void CListCtrlMove::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int n, m;
	CString tmp[2];

	switch(nChar) {
	case VK_UP:
		if ( (GetKeyState(VK_SHIFT) & 0x80) == 0 )
			break;
	case VK_ADD:
		if ( (n = GetSelectionMark()) <= 0 )
			break;
		tmp[0] = GetItemText(n - 1, 0);
		tmp[1] = GetItemText(n, 0);
		SetItemText(n - 1, 0, tmp[1]);
		SetItemText(n, 0, tmp[0]);
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
		tmp[0] = GetItemText(n + 1, 0);
		tmp[1] = GetItemText(n, 0);
		SetItemText(n + 1, 0, tmp[1]);
		SetItemText(n, 0, tmp[0]);
		SetSelectionMark(n + 1);
		SetItemState(n + 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		EnsureVisible(n + 1, 0);
		return;

	case VK_HOME:
		if ( (GetKeyState(VK_SHIFT) & 0x80) == 0 )
			break;
		if ( (n = GetSelectionMark()) <= 0 )
			break;
		tmp[0] = GetItemText(n, 0);
		DeleteItem(n);
		InsertItem(0, tmp[0]);
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
		tmp[0] = GetItemText(n, 0);
		DeleteItem(n);
		InsertItem(m, tmp[0]);
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
	CListCtrlMove *m_pOwner;

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

void CListCtrlMove::OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	int item, y;
	CRect rect;
	CRectTrackerList tracker;
	CString tmp;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

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
			tmp = GetItemText(pNMLV->iItem, 0);
			DeleteItem(pNMLV->iItem);
			if ( item > pNMLV->iItem )
				item--;
			InsertItem(item, tmp);
			SetSelectionMark(item);
			SetItemState(item, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			EnsureVisible(item, 0);
		}
	}

	Invalidate(FALSE);

	*pResult = 0;
}
