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
// CHeaderCtrlExt

CHeaderCtrlExt::CHeaderCtrlExt()
{
	m_HitItem = (-1);
}

BEGIN_MESSAGE_MAP(CHeaderCtrlExt, CHeaderCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

void CHeaderCtrlExt::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	HDITEM hdi;
	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	int nSavedDC = pDC->SaveDC();
	CRect rect = lpDrawItemStruct->rcItem;
	TCHAR title[256];
	UINT fmt = DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_VCENTER;

	ZeroMemory(&hdi, sizeof(hdi));
	hdi.mask = HDI_TEXT | HDI_FORMAT;
	hdi.pszText = title;
	hdi.cchTextMax = 256;
	GetItem(lpDrawItemStruct->itemID, &hdi);

	//TRACE("DrawItem %d %x %x\n", lpDrawItemStruct->itemID, lpDrawItemStruct->itemState, hdi.fmt);

	if ( (lpDrawItemStruct->itemState & ODS_FOCUS) != 0 )
		pDC->FillSolidRect(rect, GetAppColor(APPCOL_CTRLSHADOW));
	else
		pDC->FillSolidRect(rect, GetAppColor(APPCOL_CTRLFACE));

	pDC->FillSolidRect(rect.right - 1, rect.top, 1, rect.Height(), GetAppColor(COLOR_3DLIGHT));

	pDC->SelectObject(GetFont());
	pDC->SetTextColor(GetAppColor(APPCOL_CTRLTEXT));
	pDC->SetBkMode(TRANSPARENT);

	switch(hdi.fmt & (HDF_LEFT | HDF_RIGHT | HDF_CENTER)) {
	case HDF_LEFT:		fmt |= DT_LEFT;		break;
	case HDF_RIGHT:		fmt |= DT_RIGHT;	break;
	case HDF_CENTER:	fmt |= DT_CENTER;	break;
	}

	rect.left  += 4;
	rect.right -= 4;

	pDC->DrawText(title, rect, fmt);

	pDC->RestoreDC(nSavedDC);
}
void CHeaderCtrlExt::OnPaint()
{
	int n;
	CRect rect;
	CPaintDC dc(this);
	DRAWITEMSTRUCT dis;
	CRect clip = dc.m_ps.rcPaint;

	ZeroMemory(&dis, sizeof(dis));
	dis.hDC = dc.GetSafeHdc();
	dis.CtlType = ODT_HEADER;
	dis.itemAction = ODA_DRAWENTIRE;

	for ( n = 0 ; n < GetItemCount() ; n++ ) {
		GetItemRect(n, rect);
		
		dis.itemState = n == m_HitItem ? ODS_FOCUS : 0;

		if ( rect.left < clip.right && rect.right > clip.left ) {
			dis.itemID = n;
			dis.rcItem = rect;
			DrawItem(&dis);
		}
	}
}
BOOL CHeaderCtrlExt::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(rect, GetAppColor(APPCOL_CTRLFACE));
	return TRUE;
}

void CHeaderCtrlExt::OnMouseMove(UINT nFlags, CPoint point)
{
	int item;
	CRect rect;
	HDHITTESTINFO hdhit;

	ZeroMemory(&hdhit, sizeof(hdhit));
	hdhit.pt = point;

	if ( (item = HitTest(&hdhit)) != m_HitItem ) {
		if ( m_HitItem != (-1) ) {
			GetItemRect(m_HitItem, rect);
			InvalidateRect(rect, FALSE);
		}
		if ( (m_HitItem = item) != (-1) ) {
			GetItemRect(m_HitItem, rect);
			InvalidateRect(rect, FALSE);
		}
	}

	CHeaderCtrl::OnMouseMove(nFlags, point);
}
void CHeaderCtrlExt::OnMouseLeave()
{
	if ( m_HitItem != (-1) ) {
		CRect rect;
		GetItemRect(m_HitItem, rect);
		InvalidateRect(rect, FALSE);
		m_HitItem = (-1);
	}
	CHeaderCtrl::OnMouseLeave();
}

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt

CListCtrlExt::CListCtrlExt()
{
	//m_SortSubItem = 0;
	//m_SortReverse = 0;
	//m_SortDupItem = 0;
	m_SubMenuPos  = (-1);
	m_EditSubItem = 0;
	m_bSort = TRUE;
	m_bMove = FALSE;
	m_Dpi.cx = SYSTEM_DPI_X;
	m_Dpi.cy = SYSTEM_DPI_Y;
	m_bSetLVCheck = FALSE;
	m_EditFlag = EDITFLAG_NONE;
}
CListCtrlExt::~CListCtrlExt()
{
}

BEGIN_MESSAGE_MAP(CListCtrlExt, CListCtrl)
	ON_WM_VSCROLL()
	ON_WM_KEYDOWN()
	ON_EN_KILLFOCUS(ID_EDIT_BOX, OnKillfocusEditBox)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_NOTIFY_REFLECT_EX(LVN_BEGINDRAG, OnLvnBegindrag)
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, OnDblclk)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
#ifdef	USE_DARKMODE
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
#endif
END_MESSAGE_MAP()

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int n, ret;
	int i1, i2;
	CStringLoad s1, s2;
	LV_FINDINFO lvinfo;
	CListCtrlExt *pCompList = (CListCtrlExt *)lParamSort;

	if ( lParam1 < 0 && lParam2 >= 0 )
		return (-1);
	else if ( lParam1 >= 0 && lParam2 < 0 )
		return 1;

	lvinfo.flags = LVFI_PARAM;
	lvinfo.lParam = lParam1;
	i1 = pCompList->FindItem(&lvinfo);
	lvinfo.lParam = lParam2;
	i2 = pCompList->FindItem(&lvinfo);

	if ( i1 == (-1) || i2 == (-1) )
		return 0;

	for ( n = 0 ; n < pCompList->m_SortItem.GetSize() ; n++ ) {
		s1 = pCompList->GetItemText(i1, pCompList->m_SortItem[n] & SORTMASK_ITEM);
		s2 = pCompList->GetItemText(i2, pCompList->m_SortItem[n] & SORTMASK_ITEM);
		if ( (ret = s1.CompareDigit(s2)) != 0 ) {
			if ( (pCompList->m_SortItem[n] & SORTMASK_REVS) != 0 )
				ret = (ret > 0 ? (-1) : 1);
			return ret;
		}
	}

	return 0;
}
void CListCtrlExt::SetSelectMarkItem(int item)
{
	int count;

	if ( item < 0 || (count = GetItemCount()) <= 0 )
		return;

	if ( item >= count )
		item = count - 1;

	SetSelectionMark(item);
	SetItemState(item, LVIS_SELECTED, LVIS_SELECTED);
	EnsureVisible(item, FALSE);
	SetFocus();
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
int CListCtrlExt::GetSelectMarkCount()
{
	int n, i;

	for ( n = i = 0 ; n < GetItemCount() ; n++ ) {
		if ( GetItemState(n, LVIS_SELECTED) != 0 )
			i++;
	}

	return i;
}
void CListCtrlExt::DoSortItem()
{
	SortItems(CompareFunc, (DWORD_PTR)this);
}
void CListCtrlExt::InitColumn(LPCTSTR lpszSection, const LV_COLUMN *lpColumn, int nMax)
{
	int n, i;
	LV_COLUMN tmp;
	CBuffer buf;
	CDialogExt *pParent = (CDialogExt *)GetParent();
	CSize dsz(7, 12);
	CSize fsz(7, 12);

#ifdef	USE_DARKMODE
	CHeaderCtrl *pWnd = GetHeaderCtrl();
	if ( pWnd != NULL )
		m_HeadCtrl.SubclassWindow(pWnd->GetSafeHwnd());
#endif

	if ( pParent != NULL && CDialogExt::IsDialogExt(pParent) ) {
		m_Dpi.cx = pParent->m_NowDpi.cx;
		m_Dpi.cy = pParent->m_NowDpi.cy;
		dsz = pParent->m_DefFsz;
		fsz = pParent->m_NowFsz;
	}

	if ( nMax > 0 && AfxGetApp()->GetProfileInt(lpszSection, lpColumn[0].pszText, (-1)) != (-1) && AfxGetApp()->GetProfileInt(lpszSection, _T("ListCtrlFontSize"), 0) == 0 )
		fsz = dsz;

	for ( n = 0 ; n < nMax ; n++ ) {
		tmp = lpColumn[n];
		tmp.cx = AfxGetApp()->GetProfileInt(lpszSection, tmp.pszText, tmp.cx);
		tmp.cx = MulDiv(tmp.cx, m_Dpi.cx * fsz.cx, DEFAULT_DPI_X * dsz.cx);
		InsertColumn(n, &tmp);
	}

	m_SortItem.SetSize(nMax);
	for ( n = 0 ; n < nMax ; n++ )
		m_SortItem[n] = n;

	((CRLoginApp *)AfxGetApp())->GetProfileBuffer(lpszSection, _T("SortItemTab"), buf);

	if ( buf.GetSize() == 0 ) {
		int item = AfxGetApp()->GetProfileInt(lpszSection, _T("SortItem"), 0);
		int revs = AfxGetApp()->GetProfileInt(lpszSection, _T("SortRevs"), 0);
		int dups = AfxGetApp()->GetProfileInt(lpszSection, _T("SortDups"), 0);

		if ( revs != 0 ) {
			item |= SORTMASK_REVS;
			dups |= SORTMASK_REVS;
		}
		if ( dups != item )
			buf.Put16Bit(dups);
		buf.Put16Bit(item);

		((CRLoginApp *)AfxGetApp())->WriteProfileBinary(lpszSection, _T("SortItemTab"), buf.GetPtr(), buf.GetSize());

		((CRLoginApp *)AfxGetApp())->DelProfileEntry(lpszSection, _T("SortItem"));
		((CRLoginApp *)AfxGetApp())->DelProfileEntry(lpszSection, _T("SortRevs"));
		((CRLoginApp *)AfxGetApp())->DelProfileEntry(lpszSection, _T("SortDups"));
	}

	while ( buf.GetSize() >= 2 ) {
		i = buf.Get16Bit();
		for ( n = 0 ; n < m_SortItem.GetSize() ; n++ ) {
			if ( (m_SortItem[n] & SORTMASK_ITEM) == (i & SORTMASK_ITEM) ) {
				m_SortItem.RemoveAt(n);
				m_SortItem.InsertAt(0, i);
				break;
			}
		}
	}
}
void CListCtrlExt::SaveColumn(LPCTSTR lpszSection)
{
	int n;
	LV_COLUMN tmp;
	TCHAR name[256];
	CBuffer buf;
	CDialogExt *pParent = (CDialogExt *)GetParent();
	CSize dsz(7, 12);
	CSize fsz(7, 12);

	if ( pParent != NULL && CDialogExt::IsDialogExt(pParent) ) {
		m_Dpi.cx = pParent->m_NowDpi.cx;
		m_Dpi.cy = pParent->m_NowDpi.cy;
		dsz = pParent->m_DefFsz;
		fsz = pParent->m_NowFsz;
	}

	memset(&tmp, 0, sizeof(tmp));
	tmp.pszText = name;
	tmp.cchTextMax = 256;
	tmp.mask = LVCF_WIDTH | LVCF_TEXT;

	for ( n = 0 ; GetColumn(n, &tmp) ; n++ ) {
		tmp.cx = MulDiv(tmp.cx, DEFAULT_DPI_Y * dsz.cx, m_Dpi.cx * fsz.cx);
		AfxGetApp()->WriteProfileInt(lpszSection, tmp.pszText, tmp.cx);
	}

	AfxGetApp()->WriteProfileInt(lpszSection, _T("ListCtrlFontSize"), fsz.cx);

	for ( n = (int)m_SortItem.GetSize() - 1 ; n >= 0 ; n-- )
		buf.Put16Bit(m_SortItem[n]);

	((CRLoginApp *)AfxGetApp())->WriteProfileBinary(lpszSection, _T("SortItemTab"), buf.GetPtr(), buf.GetSize());

#ifdef	USE_DARKMODE
	if ( m_HeadCtrl.GetSafeHwnd() != NULL )
			m_HeadCtrl.UnsubclassWindow();
#endif
}
void CListCtrlExt::SetLVCheck(WPARAM ItemIndex, BOOL bCheck)
{
	m_bSetLVCheck = TRUE;
	ListView_SetItemState(m_hWnd, ItemIndex, UINT((int(bCheck) + 1) << 12), LVIS_STATEIMAGEMASK);
	m_bSetLVCheck = FALSE;
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

void CListCtrlExt::SetSortCols(int subitem)
{
	int n;

	for ( n = (int)m_SortItem.GetSize() ; n <= subitem ; n++ )
		m_SortItem.InsertAt(0, n);

	for ( n = 0 ; n < m_SortItem.GetSize() ; n++ ) {
		if ( (m_SortItem[n] & SORTMASK_ITEM) == subitem ) {
			m_SortItem.RemoveAt(n);
			m_SortItem.InsertAt(0, subitem);
			break;
		}
	}
}

void CListCtrlExt::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if ( !m_bSort )
		return;

	*pResult = 0;

	for ( n = (int)m_SortItem.GetSize() ; n <= pNMListView->iSubItem ; n++ )
		m_SortItem.InsertAt(0, n);

	if ( (m_SortItem[0] & SORTMASK_ITEM) == pNMListView->iSubItem )
		m_SortItem[0] ^= SORTMASK_REVS;
	else {
		for ( n = 0 ; n < m_SortItem.GetSize() ; n++ ) {
			if ( (m_SortItem[n] & SORTMASK_ITEM) == pNMListView->iSubItem ) {
				m_SortItem.RemoveAt(n);
				m_SortItem.InsertAt(0, pNMListView->iSubItem);
				break;
			}
		}
	}

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
	((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(pSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, pOwner);
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
	m_EditFlag = EDITFLAG_EDIT;
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
	BOOL bOpen = FALSE;

	if ( m_EditSubItem == 0 )
		return FALSE;

	for ( n = GetTopIndex() ; n < GetItemCount() ; n++ ) {
		if ( !GetItemRect(n, rect, LVIR_LABEL) )
			break;
		if ( pNMListView->ptAction.y >= rect.top && pNMListView->ptAction.y < rect.bottom ) {
			pNMListView->iItem = n;
			lvc.mask = LVCF_WIDTH | LVCF_FMT ;
			x = 0 - GetScrollPos(SB_HORZ);
			for ( i = 0 ; GetColumn(i, &lvc) ; i++, x += a ) {
				a =  lvc.cx;
				if ( pNMListView->ptAction.x >= x && pNMListView->ptAction.x < (x + a) && (m_EditSubItem & (1 << i)) != 0 ) {
					rect.left  = x;
					rect.right = x + a;
					OpenEditBox(n, i, lvc.fmt, rect);
					bOpen = TRUE;
					break;
				}
			}
			break;
		}
	}

	if ( !bOpen )
		return FALSE;
	
	*pResult = 0;
	return TRUE;
}
void CListCtrlExt::OnKillfocusEditBox() 
{
	CString str;
	int NextNum = 32;

	if ( m_EditFlag == EDITFLAG_NONE )
		return;

	m_EditWnd.GetWindowText(str);
	SetItemText(m_EditItem, m_EditNum, str);
	m_EditWnd.PostMessage(WM_CLOSE, 0, 0);

	if ( m_EditFlag == EDITFLAG_NEXT ) {
		for ( NextNum = m_EditNum + 1 ; NextNum < 32 ; NextNum++ ) {
			if ( (m_EditSubItem & (1 << NextNum)) != 0 )
				break;
		}
	}
	m_EditFlag = EDITFLAG_NONE;

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

	if ( NextNum < 32 )
		EditItem(m_EditItem, NextNum);
}
BOOL CListCtrlExt::PreTranslateMessage(MSG* pMsg) 
{
	if ( m_EditWnd.m_hWnd != NULL ) {
		if ( pMsg->message == WM_CHAR && pMsg->wParam == VK_RETURN ) {
			m_EditWnd.PostMessage(WM_KILLFOCUS, 0, 0);
			return TRUE;
		} else if ( pMsg->message == WM_CHAR && pMsg->wParam == VK_TAB ) {
			m_EditFlag = EDITFLAG_NEXT;
			m_EditWnd.PostMessage(WM_KILLFOCUS, 0, 0);
			return TRUE;
		} else if ( pMsg->message == WM_CHAR && pMsg->wParam == VK_ESCAPE ) {
			m_EditFlag = EDITFLAG_NONE;
			m_EditWnd.PostMessage(WM_CLOSE, 0, 0);
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

		((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(pSubMenu, flag | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, pOwner);
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
int CListCtrlExt::ItemHitTest(CPoint point)
{
	int item, ofs = 4;
	CRect frame, rect;

	ScreenToClient(&point);
	GetClientRect(frame);

	if ( point.x >= frame.left && point.x <= frame.right ) {
		if ( point.y >= frame.top && point.y <= frame.bottom ) {
			for ( item = GetTopIndex() ; item < GetItemCount() ; item++ ) {
				if ( !GetItemRect(item, rect, LVIR_LABEL) )
					break;
				if ( point.y >= rect.top && point.y <= rect.bottom )
					return item;
				else if ( rect.top >= frame.bottom )
					break;
			}
		}

		//if ( GetItemRect(GetTopIndex(), rect, LVIR_LABEL) ) {
		//	frame.top = rect.top;
		//	ofs = rect.Height() / 2;
		//}

		//if ( point.y < frame.top && point.y > (frame.top - ofs) ) {
		//	Scroll(CSize(0, 0 - ofs * 2));
		//} else if ( point.y > frame.bottom && point.y < (frame.bottom + ofs) ) {
		//	Scroll(CSize(0, ofs * 2));
		//}
	}

	return (-1);
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

BOOL CListCtrlExt::OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	int item, y;
	CRect rect;
	CRectTrackerList tracker;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( !m_bMove )
		return FALSE;

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
	return TRUE;
}

LRESULT CListCtrlExt::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	int n;
	LV_COLUMN tmp;
	CSize dpi;

	dpi.cx = LOWORD(wParam);
	dpi.cy = HIWORD(wParam);

	memset(&tmp, 0, sizeof(tmp));
	tmp.mask = LVCF_WIDTH;

	for ( n = 0 ; GetColumn(n, &tmp) ; n++ ) {
		tmp.cx = MulDiv(tmp.cx, dpi.cy, m_Dpi.cy);
		SetColumn(n, &tmp);
	}

	m_Dpi = dpi;

	// Font/Sizeは変更しないのでFALSE
	return FALSE;
}

BOOL CListCtrlExt::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR *pHdr = (NMHDR *)lParam;

	if ( message == WM_NOTIFY && pHdr->code == LVN_COLUMNCLICK && !m_bSort )
		return FALSE;

	return CListCtrl::OnChildNotify(message, wParam, lParam, pResult);
}

#ifdef	USE_DARKMODE
BOOL CListCtrlExt::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(rect, GetAppColor(APPCOL_CTRLFACE));
	return TRUE;
}
HBRUSH CListCtrlExt::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CListCtrl::OnCtlColor(pDC, pWnd, nCtlColor);

	switch(nCtlColor) {
	case CTLCOLOR_EDIT:			// Edit control
		hbr = GetAppColorBrush(APPCOL_CTRLFACE);
		pDC->SetTextColor(GetAppColor(APPCOL_CTRLTEXT));
		pDC->SetBkMode(TRANSPARENT);
		break;
	}

	return hbr;
}

void CListCtrlExt::OnPaint()
{
	int item, subItem;
	CWnd *pWnd;
	CRect rect, irct, mrct, srct, trct;
	CPaintDC dc(this);
	int nSavedDC = dc.SaveDC();
	CRect clip = dc.m_ps.rcPaint;
	CImageList *pImage = GetImageList(LVSIL_SMALL);
	BOOL bCheck = (GetExtendedStyle() & LVS_EX_CHECKBOXES) != 0 ? TRUE : FALSE;
	BOOL bFull = (GetExtendedStyle() & LVS_EX_FULLROWSELECT) != 0 ? TRUE : FALSE;
	BOOL bSel = FALSE;
	int markItem = GetSelectionMark();
	BOOL bFourcs = FALSE;
	LVCOLUMN lvc;
	LVITEM lvi;
	NMLVCUSTOMDRAW hdr;
	LRESULT result = CDRF_DODEFAULT;
	AFX_NOTIFY notify;
	int flag = 0;
	COLORREF tcol, bcol;

	if ( (pWnd = GetFocus()) != NULL && pWnd->GetSafeHwnd() == GetSafeHwnd() )
		bFourcs = TRUE;

	ZeroMemory(&lvc, sizeof(lvc));
	ZeroMemory(&lvi, sizeof(lvi));

	ZeroMemory(&hdr, sizeof(hdr));
	notify.pNMHDR = (NMHDR *)&hdr;
	notify.pResult = &result;

	hdr.nmcd.hdr.hwndFrom = GetSafeHwnd();
	hdr.nmcd.hdr.idFrom = GetDlgCtrlID();
	hdr.nmcd.hdr.code = NM_CUSTOMDRAW;
	hdr.nmcd.hdc = dc.GetSafeHdc();

	hdr.dwItemType = LVCDI_ITEM;
	hdr.clrText = GetAppColor(APPCOL_CTRLTEXT);
	hdr.clrTextBk = hdr.clrFace = GetAppColor(APPCOL_CTRLFACE);

	hdr.nmcd.dwDrawStage = CDDS_PREPAINT;
	result = CDRF_DODEFAULT;
	GetParent()->OnCmdMsg((UINT)GetDlgCtrlID(), MAKELONG(NM_CUSTOMDRAW, WM_NOTIFY), &notify, NULL);
	if ( (result & CDRF_NOTIFYITEMDRAW) != 0 )
		flag |= 001;
	
	if ( bCheck && m_ImageList.GetSafeHandle() == NULL ) {
		CBitmapEx bitmap;
		m_ImageList.Create(MulDiv(14, SCREEN_DPI_X, DEFAULT_DPI_X), MulDiv(14, SCREEN_DPI_Y, DEFAULT_DPI_Y), ILC_COLOR24 | ILC_MASK, 2, 2);
		for ( int n = 0 ; n < 3 ; n++ ) {
			bitmap.LoadResBitmap(IDB_CHECKBOX1 + n, SCREEN_DPI_X, SCREEN_DPI_Y, RGB(255, 0, 0));
			m_ImageList.Add(&bitmap, RGB(255, 0, 0));
			bitmap.DeleteObject();
		}
	}

	for ( item = GetTopIndex() ; item < GetItemCount() ; item++ ) {
		if ( !GetItemRect(item, rect, LVIR_BOUNDS) || !GetItemRect(item, irct, LVIR_ICON) )
			break;

		srct = rect;
		srct.left = irct.right;

		bSel = (GetItemState(item, LVIS_SELECTED) != 0 ? TRUE : FALSE);

		if ( rect.top < clip.bottom && rect.bottom > clip.top ) {

			lvc.mask = LVCF_WIDTH | LVCF_FMT;

			mrct.left = mrct.right = 0 - GetScrollPos(SB_HORZ);
			mrct.top = rect.top;
			mrct.bottom = rect.bottom;

			dc.SelectObject(GetFont());
			dc.SetBkMode(TRANSPARENT);

			if ( (flag & 001) != 0 ) {
				hdr.nmcd.dwItemSpec = item;
				hdr.nmcd.rc = mrct;
				hdr.nmcd.dwDrawStage = CDDS_ITEMPREPAINT;

				GetParent()->OnCmdMsg((UINT)GetDlgCtrlID(), MAKELONG(NM_CUSTOMDRAW, WM_NOTIFY), &notify, NULL);
				if ( (result & CDRF_NOTIFYSUBITEMDRAW) != 0 )
					flag |= 002;
			}

			for ( subItem = 0 ; GetColumn(subItem, &lvc) ; subItem++ ) {
				mrct.left  = mrct.right;
				mrct.right += lvc.cx;

				if ( mrct.left < clip.right && mrct.right > clip.left ) {
					UINT fmt = DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_VCENTER;
					CString text = GetItemText(item, subItem);
					trct = mrct;

					if ( (flag & 002) != 0 ) {
						hdr.iSubItem = subItem;
						hdr.nmcd.rc = mrct;

						hdr.nmcd.dwDrawStage = CDDS_ITEMPREPAINT | CDDS_SUBITEM;
						GetParent()->OnCmdMsg((UINT)GetDlgCtrlID(), MAKELONG(NM_CUSTOMDRAW, WM_NOTIFY), &notify, NULL);
					}

					tcol = hdr.clrText;
					bcol = hdr.clrTextBk;

					switch(lvc.fmt & LVCFMT_JUSTIFYMASK) {
					case LVCFMT_LEFT:	fmt |= DT_LEFT;		break;
					case LVCFMT_RIGHT:	fmt |= DT_RIGHT;	break;
					case LVCFMT_CENTER:	fmt |= DT_CENTER;	break;
					}

					if ( bSel && (bFull || subItem == 0) ) {
						tcol = GetAppColor(bFourcs ? APPCOL_CTRLHTEXT : APPCOL_CTRLTEXT);
						bcol = GetAppColor(bFourcs ? APPCOL_CTRLHIGH : APPCOL_CTRLSHADOW);
					}

					if ( subItem == 0 ) {
						trct.left = irct.right;
						if ( !bFull )
							srct = trct;
						if ( irct.left < irct.right ) {
							lvi.mask = LVIF_IMAGE;
							lvi.iItem = item;
							if ( pImage != NULL && GetItem(&lvi) && lvi.iItem >= 0 )
								pImage->Draw(&dc, lvi.iImage, CPoint(irct.left, irct.top), ILD_TRANSPARENT);
							trct.left  += 4;
						} else if ( bCheck && m_ImageList.GetSafeHandle() != NULL ) {
							m_ImageList.Draw(&dc, (GetLVCheck(item) ? 1 : 0), CPoint(mrct.left, mrct.top + (mrct.Height() - MulDiv(14, SCREEN_DPI_Y, DEFAULT_DPI_Y)) / 2), ILD_TRANSPARENT);
						}
						mrct.left = irct.right;
					} else
						trct.left  += 4;

					trct.right -= 4;

					dc.SetTextColor(tcol);
					dc.FillSolidRect(mrct, bcol);
					dc.DrawText(text, trct, fmt);
				}
			}

			if ( !bFourcs && bSel && markItem == item )
				dc.DrawFocusRect(srct);

			flag &= ~002;
		}
	}

	dc.RestoreDC(nSavedDC);
}
void CListCtrlExt::OnMouseMove(UINT nFlags, CPoint point)
{
	//チラつくのでデフォルト動作をスキップ
	//CListCtrl::OnMouseMove(nFlags, point);
}
void CListCtrlExt::OnMouseLeave()
{
	//CListCtrl::OnMouseLeave();
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreeCtrlExt

BEGIN_MESSAGE_MAP(CTreeCtrlExt, CTreeCtrl)
#ifdef	USE_DARKMODE
	ON_WM_PAINT()
#endif
END_MESSAGE_MAP()

void CTreeCtrlExt::OnPaint()
{
	CRect rect;
	BOOL bFocus = (::GetFocus() == GetSafeHwnd() ? TRUE : FALSE);
	HTREEITEM hItem = GetSelectedItem();
	TVITEM tv;
	TCHAR text[256];
	CDC *pDC;

	if ( hItem != NULL ) {
		ZeroMemory(&tv, sizeof(tv));
		tv.mask = TVIF_HANDLE | TVIF_TEXT;
		tv.hItem = hItem;
		tv.cchTextMax = 256;
		tv.pszText = text;
		if ( !GetItem(&tv) || !GetItemRect(hItem, rect, TRUE) )
			hItem = NULL;
	}

	CTreeCtrl::OnPaint();

	if ( hItem != NULL && (pDC = GetDC()) != NULL ) {
		UINT fmt = DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_VCENTER | DT_LEFT;

		pDC->SelectObject(GetFont());
		pDC->SetTextColor(GetAppColor(bFocus ? APPCOL_CTRLHTEXT : APPCOL_CTRLTEXT));
		pDC->SetBkMode(TRANSPARENT);

		pDC->FillSolidRect(rect, GetAppColor(bFocus ? APPCOL_CTRLHIGH : APPCOL_CTRLSHADOW));

		rect.left  += 2;
		rect.right -= 2;
		pDC->DrawText(text, rect, fmt);

		ReleaseDC(pDC);
	}
}