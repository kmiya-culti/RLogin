// ToolDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "Data.h"
#include "ToolDlg.h"

//////////////////////////////////////////////////////////////////////
// CToolDlg ダイアログ

static WORD ToolBarCtrlId[] = { IDR_MAINFRAME, IDR_TOOLBAR2 };

IMPLEMENT_DYNAMIC(CToolDlg, CDialogExt)

CToolDlg::CToolDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CToolDlg::IDD, pParent)
{
	m_BarIdx = 0;
}
CToolDlg::~CToolDlg()
{
}

void CToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_MenuTree);
	DDX_Control(pDX, IDC_COMBO1, m_ToolSize);
	DDX_Control(pDX, IDC_LIST1, m_ToolList);
	DDX_Radio(pDX, IDC_RADIO1, m_BarIdx);
}

BEGIN_MESSAGE_MAP(CToolDlg, CDialogExt)
	ON_NOTIFY(TVN_BEGINDRAG, IDC_TREE1, &CToolDlg::OnTvnBegindragMenuTree)
	ON_COMMAND(ID_EDIT_NEW, &CToolDlg::OnEditNew)
	ON_UPDATE_COMMAND_UI(ID_EDIT_NEW, &CToolDlg::OnUpdateEditNew)
	ON_COMMAND(ID_EDIT_DELETE, &CToolDlg::OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, &CToolDlg::OnUpdateEditDelete)
	ON_COMMAND(ID_EDIT_DELALL, &CToolDlg::OnEditDelall)
	ON_COMMAND(IDM_ICONLOAD, &CToolDlg::OnIconload)
	ON_UPDATE_COMMAND_UI(IDM_ICONLOAD, &CToolDlg::OnUpdateIconload)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO2, OnToolBatSel)
END_MESSAGE_MAP()

static const INITDLGTAB ItemTab[] = {
	{ IDC_TREE1,		ITM_RIGHT_MID | ITM_BTM_BTM },
	{ IDC_LIST1,		ITM_LEFT_MID | ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ IDC_COMBO1,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_RADIO1,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_RADIO2,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_TREE_FRAME1,	ITM_RIGHT_MID | ITM_BTM_BTM },
	{ IDC_LIST_FRAME1,	ITM_LEFT_MID | ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ IDC_COMBO_TITLE1,	ITM_LEFT_MID | ITM_RIGHT_RIGHT },

	{ 0,	0 },
};

int CToolDlg::GetImageIndex(UINT nId, BOOL bUpdate)
{
	int idx;
	int cx, cy;
	CDC dc[2];
	CBitmap ImageMap;
	CBitmap *pBitmap;
	CBitmap *pOld[2];
	BITMAP mapinfo;
	CResBitmapBase *pResBitmap;

	for ( idx = 0 ; idx < m_ImageId.GetSize() ; idx++ ) {
		if ( m_ImageId[idx] == nId ) {
			if ( !bUpdate )
				return idx;
			m_ImageId.RemoveAt(idx);
			m_ImageList.Remove(idx);
			break;
		}
	}

	if ( ((pResBitmap = (CResBitmapBase *)m_DataBase.Find(nId, RT_BITMAP)) == NULL || pResBitmap->m_hBitmap == NULL) &&
		 ((pResBitmap = (CResBitmapBase *)(((CRLoginApp *)::AfxGetApp())->m_ResDataBase.Find(nId, RT_BITMAP))) == NULL || pResBitmap->IsUserMap() || pResBitmap->m_hBitmap == NULL) )
		return 0;	// null image idx

	pBitmap = CBitmap::FromHandle(pResBitmap->m_hBitmap);

	cx = GetSystemMetrics(SM_CXMENUCHECK);
	cy = GetSystemMetrics(SM_CYMENUCHECK);

	dc[0].CreateCompatibleDC(NULL);
	dc[1].CreateCompatibleDC(NULL);

	if ( !pBitmap->GetBitmap(&mapinfo) )
		return 0;

	pOld[0] = dc[0].SelectObject(pBitmap);

	ImageMap.CreateBitmap(cx, cy, dc[1].GetDeviceCaps(PLANES), dc[1].GetDeviceCaps(BITSPIXEL), NULL);

	pOld[1] = dc[1].SelectObject(&ImageMap);

	dc[1].FillSolidRect(0, 0, cx, cy, RGB(192, 192, 192));
	dc[1].TransparentBlt(0, 0, cx, cy, &(dc[0]), 0, 0, (mapinfo.bmWidth <= mapinfo.bmHeight ? mapinfo.bmWidth : mapinfo.bmHeight), mapinfo.bmHeight, RGB(192, 192, 192));

	dc[0].SelectObject(pOld[0]);
	dc[1].SelectObject(pOld[1]);

	idx = (int)m_ImageId.Add(nId);
	m_ImageList.Add(&ImageMap, RGB(192, 192, 192));

	return idx;
}

void CToolDlg::AddMenuTree(CMenu *pMenu, HTREEITEM own)
{
	int n, a;
	int idx;
	UINT id;
	CString str;
	CMenu *pSubMenu;
	HTREEITEM item;

	for ( n = 0 ; n < pMenu->GetMenuItemCount() ; n++ ) {
		pMenu->GetMenuString(n, str, MF_BYPOSITION);
		id = pMenu->GetMenuItemID(n);
		pSubMenu = pMenu->GetSubMenu(n);

		if ( id == 0 )					// Separator
			continue;
		else if ( id == (UINT)(-1) )	// SubMenu
			idx = 1;
		else
			idx = GetImageIndex(id, FALSE);

		while ( (a = str.Find(_T('&'))) >= 0 )
			str.Delete(a, 1);

		if ( (item = m_MenuTree.InsertItem(str, own)) == NULL )
			continue;

		m_MenuTree.SetItemData(item, (DWORD_PTR)id);
		m_MenuTree.SetItemImage(item, idx, idx);

		if ( pSubMenu != NULL )
			AddMenuTree(pSubMenu, item);
	}
}
void CToolDlg::InsertList(int pos, WORD id)
{
	int a, idx;
	CStringLoad str;

	if ( id == 0 ) {
		str.LoadString(IDS_SEPARATESTRING);
		idx = 2;
	} else if ( id == 1 ) {
		str.LoadString(IDS_DROPBEGINSTRING);
		idx = 3;
	} else if ( id == 2 ) {
		str.LoadString(IDS_DROPENDSTRING);
		idx = 4;
	} else {
		m_DefMenu.GetMenuString(id, str, MF_BYCOMMAND);
		idx = GetImageIndex(id, FALSE);
	}

	while ( (a = str.Find(_T('&'))) >= 0 )
		str.Delete(a, 1);

	m_ToolList.InsertItem(pos, str, idx);
	m_ToolList.SetItemData(pos, (DWORD_PTR)id);
}
void CToolDlg::InitList(BOOL bMenuInit)
{
	int n;
	HTREEITEM item;

	if ( bMenuInit ) {
		// Image List Delete
		while ( m_ImageId.GetSize() > 5 )
			m_ImageId.RemoveAt(5);
		while ( m_ImageList.GetImageCount() > 5 )
			m_ImageList.Remove(5);
	}

	// Tool List Init
	m_ToolList.DeleteAllItems();

	for ( n = 0 ; n < m_ToolId[m_BarIdx].GetSize() ; n++ )
		InsertList(n, m_ToolId[m_BarIdx][n]);

	if ( bMenuInit ) {
		// Menu Tree Init
		m_MenuTree.DeleteAllItems();
		AddMenuTree(&m_DefMenu, NULL);

		if ( (item = m_MenuTree.InsertItem(CStringLoad(IDS_SEPARATESTRING), NULL)) != NULL ) {
			m_MenuTree.SetItemData(item, (DWORD_PTR)0);
			m_MenuTree.SetItemImage(item, 2, 2);
		}
		if ( (item = m_MenuTree.InsertItem(CStringLoad(IDS_DROPBEGINSTRING), NULL)) != NULL ) {
			m_MenuTree.SetItemData(item, (DWORD_PTR)1);
			m_MenuTree.SetItemImage(item, 3, 3);
		}
		if ( (item = m_MenuTree.InsertItem(CStringLoad(IDS_DROPENDSTRING), NULL)) != NULL ) {
			m_MenuTree.SetItemData(item, (DWORD_PTR)2);
			m_MenuTree.SetItemImage(item, 4, 4);
		}
	}
}
void CToolDlg::LoadList()
{
	int n;

	m_ToolId[m_BarIdx].RemoveAll();

	for ( n = 0 ; n < m_ToolList.GetItemCount() ; n++ )
		m_ToolId[m_BarIdx].Add((WORD)m_ToolList.GetItemData(n));
}
void CToolDlg::ListUpdateImage(HTREEITEM root, WORD nId, int idx)
{
	HTREEITEM item;

	if ( root == NULL )
		return;

	if ( (WORD)(m_MenuTree.GetItemData(root)) == nId )
		m_MenuTree.SetItemImage(root, idx, idx);
	
	if ( (item = m_MenuTree.GetNextItem(root, TVGN_NEXT)) != NULL )
		ListUpdateImage(item, nId, idx);

	if ( (item = m_MenuTree.GetNextItem(root, TVGN_CHILD)) != NULL )
		ListUpdateImage(item, nId, idx);
}
void CToolDlg::UpdateImage(WORD nId)
{
	int n;
	LVITEM item;
	int idx = GetImageIndex(nId, TRUE);

	for ( n = 0 ; n < m_ToolList.GetItemCount() ; n++ ) {
		item.mask = LVIF_IMAGE | LVIF_PARAM;
		item.iItem = n;

		if ( m_ToolList.GetItem(&item) && (WORD)(item.lParam) == nId ) {
			item.mask = LVIF_IMAGE;
			item.iImage = idx;
			m_ToolList.SetItem(&item);
		}
	}
	m_ToolList.Invalidate(FALSE);

	ListUpdateImage(m_MenuTree.GetRootItem(), nId, idx);
	m_MenuTree.Invalidate(FALSE);
}

//////////////////////////////////////////////////////////////////////
// CToolDlg メッセージ ハンドラー

BOOL CToolDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	int n;
	CResToolBarBase *pResToolBar;
	int sz = 16;
	CBuffer buf;

	// Init Menu
	CMenuLoad::GetPopUpMenu(IDR_RLOGINTYPE, m_DefMenu);

	// Init ImageList
	m_ImageList.Create(GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK), ILC_COLOR24 | ILC_MASK, 0, 4);
	m_MenuTree.SetImageList(&m_ImageList, TVSIL_NORMAL);

	// Add ImageList Null, Sub, Sepa
	for ( n = 0 ; n < 5 ; n++ )
		GetImageIndex(IDB_MENUMAP1 + n, TRUE);

	// レジストリから読み込み
	((CRLoginApp *)::AfxGetApp())->GetProfileBuffer(_T("RLoginApp"), _T("ResDataBase"), buf);
	if ( buf.GetSize() > (sizeof(DWORD) * 5) )
		m_DataBase.Serialize(FALSE, buf);

	// ツールバー読み込み
	for ( int i = 0 ; i < 2 ; i++ ) {
		m_ToolId[i].RemoveAll();
		if ( (pResToolBar = (CResToolBarBase *)(((CRLoginApp *)::AfxGetApp())->m_ResDataBase.Find(ToolBarCtrlId[i], RT_TOOLBAR))) != NULL ) {
			for ( n = 0 ; n < pResToolBar->m_Item.GetSize() ; n++ ) {
				if ( pResToolBar->m_Item[n] == 0 && (n + 2) < pResToolBar->m_Item.GetSize() && pResToolBar->m_Item[n + 1] == 0 && pResToolBar->m_Item[n + 2] != 0 ) {
					n += 2;
					m_ToolId[i].Add(1);
					for ( ; n < pResToolBar->m_Item.GetSize() ; n++ ) {
						if ( pResToolBar->m_Item[n] == 0 )
							break;
						m_ToolId[i].Add(pResToolBar->m_Item[n]);
					}
					m_ToolId[i].Add(2);
				} else
					m_ToolId[i].Add(pResToolBar->m_Item[n]);
			}
			sz = pResToolBar->m_Width;
		}
	}

	CRect rect;
	m_ToolList.GetClientRect(rect);

	m_ToolList.InsertColumn(0, _T(""), LVCFMT_LEFT, rect.Width() * 9 / 10);
	m_ToolList.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_ToolList.m_bMove = TRUE;
	m_ToolList.SetPopUpMenu(IDR_POPUPMENU, 7);

	InitList(TRUE);
	m_ToolSize.SetCurSel((sz - 10) / 2);

	SetSaveProfile(_T("ToolDlg"));
	AddHelpButton(_T("#TOOLBAR"));

	return TRUE;
}

void CToolDlg::OnOK()
{
	int n;
	int sz = 16;
	CBuffer buf;
	CResToolBarBase *pResToolBar;
	CResDataBase *pDataBase = &(((CRLoginApp *)::AfxGetApp())->m_ResDataBase);

	LoadList();

	if ( (n = m_ToolSize.GetCurSel()) >= 0 )
		sz = n * 2 + 10;

	// システムのツールバーを更新
	for ( int i = 0 ; i < 2 ; i++ ) {
		if ( (pResToolBar = (CResToolBarBase *)pDataBase->Find(ToolBarCtrlId[i], RT_TOOLBAR)) != NULL ) {
			pResToolBar->m_Item.RemoveAll();

			BOOL bDrop = FALSE;
			for ( n = 0 ; n < m_ToolId[i].GetSize() ; n++ ) {
				if ( m_ToolId[i][n] == 1 ) {
					if ( bDrop )
						pResToolBar->m_Item.Add(0);
					pResToolBar->m_Item.Add(0);
					pResToolBar->m_Item.Add(0);
					bDrop = TRUE;
				} else if ( m_ToolId[i][n] == 2 ) {
					if ( bDrop )
						pResToolBar->m_Item.Add(0);
					bDrop = FALSE;
				} else {
					if ( m_ToolId[i][n] == 0 && bDrop ) {
						pResToolBar->m_Item.Add(0);
						bDrop = FALSE;
					}
					pResToolBar->m_Item.Add(m_ToolId[i][n]);
				}
			}

			pResToolBar->m_Width  = sz;
			pResToolBar->m_Height = sz;

			// ローカルのツールバーを更新
			m_DataBase.Add(pResToolBar->m_ResId, RT_TOOLBAR, pResToolBar);
		}
	}

	// システムのユーザービットマップを削除
	for ( n = 0 ; n < pDataBase->m_Bitmap.GetSize() ; n++ ) {
		if ( pDataBase->m_Bitmap[n].IsUserMap() ) {
			pDataBase->m_Bitmap.RemoveAt(n);
			n--;
		}
	}

	// システムのツールバーアイコンを再読み込み
	pDataBase->InitToolBarBitmap(MAKEINTRESOURCE(IDR_MAINFRAME), IDB_BITMAP1);
	pDataBase->InitToolBarBitmap(MAKEINTRESOURCE(IDR_TOOLBAR2),  IDB_BITMAP10);

	// ローカルの内蔵ビットマットを削除
	// ローカルのユーザービットマップをシステムにコピー
	for ( n = 0 ; n < m_DataBase.m_Bitmap.GetSize() ; n++ ) {
		if ( !m_DataBase.m_Bitmap[n].IsUserMap() ) {
			m_DataBase.m_Bitmap.RemoveAt(n);
			n--;
		} else
			pDataBase->Add(m_DataBase.m_Bitmap[n].m_ResId, RT_BITMAP, (void *)&(m_DataBase.m_Bitmap[n]));
	}

	// レジストリに保存
	m_DataBase.Serialize(TRUE, buf);
	((CRLoginApp *)::AfxGetApp())->WriteProfileBinary(_T("RLoginApp"), _T("ResDataBase"), buf.GetPtr(), buf.GetSize());

	// メニュー画像も再構成
	((CMainFrame *)::AfxGetMainWnd())->InitMenuBitmap();

	// DropDownメニューの最後の記録をすべて削除
	((CRLoginApp *)::AfxGetApp())->DelProfileSection(_T("ToolBarEx"));

	CDialogExt::OnOK();
}

void CToolDlg::OnTvnBegindragMenuTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	int item;
	CPoint point;
	CRect rect;
	CRectTracker tracker;
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if ( pNMTreeView->itemNew.lParam != (UINT)(-1) ) {
		m_MenuTree.SelectItem(pNMTreeView->itemNew.hItem);
		m_MenuTree.GetItemRect(pNMTreeView->itemNew.hItem, rect, FALSE);
		m_MenuTree.ClientToScreen(rect);
		ScreenToClient(rect);

		point = pNMTreeView->ptDrag;
		m_MenuTree.ClientToScreen(&point);
		ScreenToClient(&point);

		tracker.m_rect = rect;
		tracker.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;

		if ( tracker.Track(this, point, FALSE, this) ) {
			point.x = point.x + (tracker.m_rect.left - rect.left);
			point.y = point.y + (tracker.m_rect.top - rect.top);

			m_ToolList.GetWindowRect(rect);
			ClientToScreen(&point);

			if ( rect.PtInRect(point) ) {
				m_ToolList.ScreenToClient(&point);

				for ( item = m_ToolList.GetTopIndex() ; item < m_ToolList.GetItemCount() ; item++ ) {
					if ( !m_ToolList.GetItemRect(item, rect, LVIR_LABEL) )
						break;
					if ( rect.PtInRect(point) ) {
						InsertList(item, (WORD)(pNMTreeView->itemNew.lParam));
						m_ToolList.SetSelectionMark(item);
						m_ToolList.SetItemState(item, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
						m_ToolList.EnsureVisible(item, 0);
						m_ToolList.SetFocus();
					}
				}
			}
		}
	}

	*pResult = 0;
}

void CToolDlg::OnEditNew()
{
	int pos;
	WORD id;
	HTREEITEM item;

	if ( (item = m_MenuTree.GetSelectedItem()) == NULL )
		return;

	id = (WORD)m_MenuTree.GetItemData(item);

	if ( id == (WORD)(-1) )
		return;

	if ( (pos = m_ToolList.GetSelectionMark()) < 0 )
		return;

	InsertList(pos, id);
	m_ToolList.SetSelectionMark(pos);
	m_ToolList.SetItemState(pos, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	m_ToolList.EnsureVisible(pos, 0);
	m_ToolList.SetFocus();
}
void CToolDlg::OnUpdateEditNew(CCmdUI *pCmdUI)
{
	HTREEITEM item;

	pCmdUI->Enable((item = m_MenuTree.GetSelectedItem()) != NULL && (UINT)m_MenuTree.GetItemData(item) != (UINT)(-1) ? TRUE : FALSE);
}

void CToolDlg::OnEditDelete()
{
	int pos;

	if ( (pos = m_ToolList.GetSelectionMark()) < 0 )
		return;

	m_ToolList.DeleteItem(pos);
}
void CToolDlg::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_ToolList.GetSelectionMark() >= 0 ? TRUE : FALSE);
}

void CToolDlg::OnEditDelall()
{
	int n, sz = 0;
	CResDataBase DataBase;
	CResToolBarBase *pResToolBar;

	// デフォルトのプログラム内リソースから読み込み
	DataBase.AddToolBar(MAKEINTRESOURCE(ToolBarCtrlId[m_BarIdx]));
	pResToolBar = (CResToolBarBase *)(DataBase.Find(ToolBarCtrlId[m_BarIdx], RT_TOOLBAR));

	if ( pResToolBar != NULL ) {
		m_ToolId[m_BarIdx].RemoveAll();
		for ( n = 0 ; n < pResToolBar->m_Item.GetSize() ; n++ ) {
			if ( pResToolBar->m_Item[n] == 0 && (n + 2) < pResToolBar->m_Item.GetSize() && pResToolBar->m_Item[n + 1] == 0 && pResToolBar->m_Item[n + 2] != 0 ) {
				n += 2;
				m_ToolId[m_BarIdx].Add(1);
				for ( ; n < pResToolBar->m_Item.GetSize() ; n++ ) {
					if ( pResToolBar->m_Item[n] == 0 )
						break;
					m_ToolId[m_BarIdx].Add(pResToolBar->m_Item[n]);
				}
				m_ToolId[m_BarIdx].Add(2);
			} else
				m_ToolId[m_BarIdx].Add(pResToolBar->m_Item[n]);
		}
		sz = pResToolBar->m_Width;
	}

	// ユーザーアイコンをすべて削除
	m_DataBase.m_Bitmap.RemoveAll();
	m_DataBase.InitToolBarBitmap(MAKEINTRESOURCE(IDR_MAINFRAME), IDB_BITMAP1);
	m_DataBase.InitToolBarBitmap(MAKEINTRESOURCE(IDR_TOOLBAR2),  IDB_BITMAP10);

	InitList(TRUE);
	m_ToolSize.SetCurSel((sz - 10) / 2);
}

void CToolDlg::OnIconload()
{
	int pos;
	WORD id;
	CResBitmapBase work;
	CFileDialog dlg(TRUE, _T(""), _T(""), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGIMAGE), this);

	if ( (pos = m_ToolList.GetSelectionMark()) < 0 )
		return;

	if ( (id = (WORD)(m_ToolList.GetItemData(pos))) == 0 || id == (WORD)(-1) )
		return;

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	if ( !work.LoadBitmap(id, dlg.GetPathName()) )
		return;

	m_DataBase.Add(id, RT_BITMAP, &work);

	UpdateImage(id);
}
void CToolDlg::OnUpdateIconload(CCmdUI *pCmdUI)
{
	int pos;

	pCmdUI->Enable((pos = m_ToolList.GetSelectionMark()) >= 0 && (WORD)(m_ToolList.GetItemData(pos)) != 0 ? TRUE : FALSE);
}

void CToolDlg::OnToolBatSel(UINT nID)
{
	LoadList();
	UpdateData(TRUE);
	InitList(FALSE);
}