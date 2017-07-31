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
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
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
#if 0
	case VK_HOME:
		if ( (GetKeyState(VK_SHIFT) & 0x80) == 0 )
			break;
		if ( (n = GetSelectionMark()) <= 0 )
			break;
		tmp[0] = GetItemText(0, 0);
		tmp[1] = GetItemText(n, 0);
		SetItemText(0, 0, tmp[1]);
		SetItemText(n, 0, tmp[0]);
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
		tmp[0] = GetItemText(m, 0);
		tmp[1] = GetItemText(n, 0);
		SetItemText(m, 0, tmp[1]);
		SetItemText(n, 0, tmp[0]);
		SetSelectionMark(m);
		SetItemState(m, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		EnsureVisible(m, 0);
		return;
#endif
	}
	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CListCtrlMove::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n, i, y, max;
	CRect box[20];
	CRectTracker tracker;
	CString tmp;
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	for ( max = 0, n = GetTopIndex() ; max < 20 && n < GetItemCount() ; n++ ) {
		if ( !GetItemRect(n, box[max], LVIR_LABEL) )
			break;
		max++;
	}

	for ( n = 0 ; n < max ; n++ ) {
		if ( pNMListView->ptAction.y >= box[n].top && pNMListView->ptAction.y < box[n].bottom ) {
			tracker.m_rect = box[n];
			tracker.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;
			if ( tracker.Track(this, pNMListView->ptAction, FALSE, this) ) {
				y = pNMListView->ptAction.y + (tracker.m_rect.top - box[n].top);
				for ( i = 0 ; i < max ; i++ ) {
					if ( y >= box[i].top && y < box[i].bottom ) {
						if ( i != n ) {
							tmp = GetItemText(n + GetTopIndex(), 0);
							DeleteItem(n);
							if ( i > n )
								i--;
							InsertItem(i + GetTopIndex(), tmp);
							SetSelectionMark(i + GetTopIndex());
							SetItemState(i + GetTopIndex(), LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
							EnsureVisible(i + GetTopIndex(), 0);
						}
						break;
					}
				}
			}
			break;
		}
	}

	*pResult = 0;
}
