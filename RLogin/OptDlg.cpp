// OptDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "InitAllDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreePropertyPage

CTreePropertyPage::CTreePropertyPage()
	:CPropertyPage(CHisPage::IDD)
{
	m_pSheet = NULL;
	m_hTreeItem = NULL;
	m_pOwn = NULL;
}
CTreePropertyPage::CTreePropertyPage(UINT nIDTemplate, UINT nIDCaption , DWORD dwSize)
	:CPropertyPage(nIDTemplate, nIDCaption, dwSize)
{
	m_pSheet = NULL;
	m_hTreeItem = NULL;
	m_pOwn = NULL;
}
void CTreePropertyPage::OnReset()
{
}

/////////////////////////////////////////////////////////////////////////////
// COptDlg

IMPLEMENT_DYNAMIC(COptDlg, CPropertySheet)

COptDlg::COptDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_ModFlag   = 0;
	m_pEntry    = NULL;
	m_pTextRam  = NULL;
	m_pKeyTab   = NULL;
	m_pKeyMac   = NULL;
	m_pParamTab = NULL;
	m_pDocument = NULL;

	AddPage(&m_SerEntPage,	NULL);
	AddPage(&m_SockOptPage,	&m_SerEntPage);
	AddPage(&m_ProtoPage,	&m_SerEntPage);
	AddPage(&m_ScriptPage,	&m_SerEntPage);
	AddPage(&m_ScrnPage,	NULL);
	AddPage(&m_TermPage,	&m_ScrnPage);
	AddPage(&m_HisPage,		&m_ScrnPage);
	AddPage(&m_ClipPage,	NULL);
	AddPage(&m_MousePage,	&m_ClipPage);
	AddPage(&m_CharSetPage,	NULL);
	AddPage(&m_ColorPage,	NULL);
	AddPage(&m_KeyPage,		NULL);
}
	
/////////////////////////////////////////////////////////////////////////////

void COptDlg::AddPage(CTreePropertyPage *pPage, CTreePropertyPage *pOwn)
{
	pPage->m_pSheet = this;
	pPage->m_pOwn = pOwn;
	CPropertySheet::AddPage(pPage);
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(COptDlg, CPropertySheet)
	ON_BN_CLICKED(IDC_DOINIT, OnDoInit)
	ON_NOTIFY(TVN_SELCHANGED, IDC_LIST1, OnSelchangedTree)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptDlg メッセージ ハンドラ

BOOL COptDlg::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	int n;
	CWnd *pWnd[4];
	CRect rect[4], temp, frame;
	static int idTab[] = { IDC_DOINIT, IDOK, IDCANCEL, ID_APPLY_NOW };

	CTreePropertyPage *pPage;
	CTabCtrl* pTab = GetTabControl();
	int addWidth = 100;
	int tabHeight;
	HTREEITEM hti, own;

	TCHAR szTab [256];
	TCITEM item;
	item.mask = TCIF_TEXT;
	item.cchTextMax = 255;
	item.pszText = szTab;

	ASSERT(pTab != NULL);

	if ( (n = pTab->GetItemCount()) <= 0 )
		tabHeight = 0;
	else {
		pTab->GetItemRect(0, temp);
		if ( n > 1 ) {
			pTab->GetItemRect(n - 1, frame);
			if ( temp.top > frame.top )
				temp.top = frame.top;
			if ( temp.bottom < frame.bottom )
				temp.bottom = frame.bottom;
		}
		tabHeight = temp.Height();
	}

	GetWindowRect(frame);
	SetWindowPos(NULL, frame.left - addWidth / 2, frame.top + tabHeight / 2, frame.Width() + 100, frame.Height() - tabHeight, SWP_NOZORDER | SWP_NOACTIVATE);

	pTab->GetWindowRect(frame);
	pTab->MoveWindow(addWidth + 8, 0 - tabHeight + 8, frame.Width(), frame.Height());
	pTab->ModifyStyle(WS_TABSTOP, 0);
	pTab->ShowWindow(SW_HIDE);

	temp.left = addWidth + 8;
	temp.right = temp.left + frame.Width();
	temp.top = 8;
	temp.bottom = temp.top + frame.Height() - tabHeight;
	temp.DeflateRect(2, 2);
	m_wndFrame.Create(NULL, WS_CHILD | WS_VISIBLE | SS_BLACKFRAME, temp, this);

	temp.left = 8;
	temp.right = temp.left + addWidth;
	m_wndTree.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_HASLINES | TVS_DISABLEDRAGDROP, temp, this, IDC_LIST1);

	for ( n = 0; n < GetPageCount() ; n++ ) {
		if ( (pPage = (CTreePropertyPage *)GetPage(n)) == NULL )
			continue;
		if ( !pTab->GetItem(n, &item) )
			continue;
		own = (pPage->m_pOwn != NULL ? pPage->m_pOwn->m_hTreeItem : NULL);
		if ( (hti = m_wndTree.InsertItem(szTab, own)) != NULL ) {
			m_wndTree.SetItemData(hti, (DWORD_PTR)pPage);
			m_wndTree.SetItemState(hti, TVIS_EXPANDED, TVIS_EXPANDED);
			pPage->m_hTreeItem = hti;
		}
	}

	if ( GetPageCount() > 0 && (pPage = (CTreePropertyPage *)GetPage(0)) != NULL ) {
		SetActivePage(pPage);
		m_wndTree.SetItemState(pPage->m_hTreeItem, TVIS_SELECTED, TVIS_SELECTED);
	}

	for ( n = 1 ; n < 4 ; n++ ) {
		if ( (pWnd[n] = GetDlgItem(idTab[n])) != NULL ) {
			pWnd[n]->GetWindowRect(rect[n]);
			ScreenToClient(rect[n]);
		}
	}

	if ( pWnd[1] == NULL || pWnd[2] == NULL )
		return bResult;

	rect[0] = rect[1];
	rect[0].left  -= (rect[2].left  - rect[1].left);
	rect[0].right -= (rect[2].right - rect[1].right);
	m_DoInit.Create(CStringLoad(IDS_INITBUTTON), BS_PUSHBUTTON, rect[0], this, IDC_DOINIT);

	CFont *pFont = pWnd[1]->GetFont();
	m_DoInit.SetFont(pFont, FALSE);
	m_DoInit.ShowWindow(SW_SHOW);
	pWnd[0] = &m_DoInit;

	GetClientRect(frame);
	temp.left  = rect[0].left;
	temp.right = ((m_psh.dwFlags & PSH_NOAPPLYNOW) == 0 ? rect[3].right : rect[2].right);
	int offs  = temp.left + temp.Width() / 2 - frame.Width() / 2;

	for ( n = 0 ; n < 4 ; n++ ) {
		if ( pWnd[n] == NULL )
			continue;
		rect[n].left   -= offs;
		rect[n].right  -= offs;
		rect[n].top    -= tabHeight;
		rect[n].bottom -= tabHeight;
		pWnd[n]->MoveWindow(rect[n]);
	}

	return bResult;
}
void COptDlg::OnDoInit()
{
	CInitAllDlg dlg;

	if ( dlg.DoModal() != IDOK )
		return;

	//if ( MessageBox(CStringLoad(IDS_ALLINITREQ), _T("Question"), MB_ICONQUESTION | MB_YESNO) != IDYES )
	//	return;

	if ( dlg.m_InitFlag ) {
		m_pTextRam->Init();
		m_pTextRam->m_FontTab.Init();
		m_pKeyTab->Init();
		m_pKeyMac->Init();
		m_pParamTab->Init();
	} else {
		m_pTextRam->Serialize(FALSE);
		m_pKeyTab->Serialize(FALSE);
		m_pKeyMac->Serialize(FALSE);
		m_pParamTab->Serialize(FALSE);
	}

	int n;
	CTreePropertyPage *pPage;

	for ( n = 0; n < GetPageCount() ; n++ ) {
		if ( (pPage = (CTreePropertyPage *)GetPage(n)) == NULL )
			continue;
		if ( pPage->m_hWnd != NULL )
			pPage->OnReset();
	}

	if ( m_pDocument != NULL )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	m_ModFlag   = 0;
}

BOOL COptDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	BOOL rt = CPropertySheet::OnCommand(wParam, lParam);

	if ( wParam == ID_APPLY_NOW && m_pDocument != NULL )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	return rt;
}

void COptDlg::OnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	HTREEITEM hti;
	CPropertyPage *pPage;

	if ( (hti = m_wndTree.GetSelectedItem()) != NULL && (pPage = (CPropertyPage *)m_wndTree.GetItemData(hti)) != NULL )
		SetActivePage(pPage);
}
