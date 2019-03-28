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
// CTreePage

IMPLEMENT_DYNAMIC(CTreePage, CDialogExt)

CTreePage::CTreePage(UINT nIDTemplate)
	:CDialogExt(nIDTemplate)
{
	m_nIDTemplate = nIDTemplate;
	m_pSheet = NULL;
	m_hTreeItem = NULL;
	m_pOwn = NULL;

	SetBackColor(GetSysColor(COLOR_WINDOW));
}
CTreePage::~CTreePage()
{
}
void CTreePage::SetModified(BOOL bModified)
{
	if ( m_pSheet != NULL )
		m_pSheet->SetModified(bModified);
}

void CTreePage::OnReset()
{
}
BOOL CTreePage::OnApply()
{
	return FALSE;
}

BEGIN_MESSAGE_MAP(CTreePage, CDialogExt)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptDlg

IMPLEMENT_DYNAMIC(COptDlg, CDialogExt)

COptDlg::COptDlg(LPCTSTR pszCaption, CWnd* pParentWnd)
	:CDialogExt(COptDlg::IDD, pParentWnd)
{
	if ( pszCaption != NULL )
		m_Title = pszCaption;

	m_ModFlag   = 0;
	m_bModified = FALSE;
	m_bOptFixed = FALSE;

	m_pEntry    = NULL;
	m_pTextRam  = NULL;
	m_pKeyTab   = NULL;
	m_pKeyMac   = NULL;
	m_pParamTab = NULL;
	m_pDocument = NULL;

#if 0
	TRACE("CSerEntPage %d\n", sizeof(CSerEntPage));
	TRACE("CKeyPage %d\n", sizeof(CKeyPage));
	TRACE("CColParaDlg %d\n", sizeof(CColParaDlg));
	TRACE("CCharSetPage %d\n", sizeof(CCharSetPage));
	TRACE("CSockOptPage %d\n", sizeof(CSockOptPage));
	TRACE("CProtoPage %d\n", sizeof(CProtoPage));
	TRACE("CTermPage %d\n", sizeof(CTermPage));
	TRACE("CExtOptPage %d\n", sizeof(CExtOptPage));
	TRACE("CScrnPage %d\n", sizeof(CScrnPage));
	TRACE("CHisPage %d\n", sizeof(CHisPage));
	TRACE("CClipPage %d\n", sizeof(CClipPage));
	TRACE("CMousePage %d\n", sizeof(CMousePage));
	TRACE("CScriptPage %d\n", sizeof(CScriptPage));
	TRACE("CModKeyPage %d\n", sizeof(CModKeyPage));
	TRACE("CBackPage %d\n", sizeof(CBackPage));
	TRACE("COptDlg %d\n", sizeof(COptDlg));

	TRACE("CServerEntry %d\n", sizeof(CServerEntry));
	TRACE("CTextRam %d\n", sizeof(CTextRam));
	TRACE("CKeyNodeTab %d\n", sizeof(CKeyNodeTab));
	TRACE("CKeyMacTab %d\n", sizeof(CKeyMacTab));
	TRACE("CParamTab %d\n", sizeof(CParamTab));
#endif

	m_pSerEntPage  = new CSerEntPage;
	m_pKeyPage     = new CKeyPage;
	m_pColorPage   = new CColParaDlg;
	m_pCharSetPage = new CCharSetPage;
	m_pSockOptPage = new CSockOptPage;
	m_pProtoPage   = new CProtoPage;
	m_pTermPage    = new CTermPage;
	m_pExtOptPage  = new CExtOptPage;
	m_pScrnPage    = new CScrnPage;
	m_pHisPage     = new CHisPage;
	m_pClipPage    = new CClipPage;
	m_pMousePage   = new CMousePage;
	m_pScriptPage  = new CScriptPage;
	m_pModKeyPage  = new CModKeyPage;
	m_pBackPage    = new CBackPage;

	AddPage(m_pSerEntPage,	NULL);
	AddPage(m_pSockOptPage,	m_pSerEntPage);
	AddPage(m_pProtoPage,	m_pSerEntPage);
	AddPage(m_pScriptPage,	m_pSerEntPage);
	AddPage(m_pScrnPage,	NULL);
	AddPage(m_pTermPage,	m_pScrnPage);
	AddPage(m_pExtOptPage,	m_pScrnPage);
	AddPage(m_pHisPage,		m_pScrnPage);
	AddPage(m_pClipPage,	NULL);
	AddPage(m_pMousePage,	m_pClipPage);
	AddPage(m_pCharSetPage,	NULL);
	AddPage(m_pColorPage,	NULL);
	AddPage(m_pBackPage,	m_pColorPage);
	AddPage(m_pKeyPage,		NULL);
	AddPage(m_pModKeyPage,	m_pKeyPage);

	m_ActivePage = (-1);
	m_psh.dwFlags = 0;
}
COptDlg::~COptDlg()
{
	delete m_pSerEntPage;
	delete m_pKeyPage;
	delete m_pColorPage;
	delete m_pCharSetPage;
	delete m_pSockOptPage;
	delete m_pProtoPage;
	delete m_pTermPage;
	delete m_pExtOptPage;
	delete m_pScrnPage;
	delete m_pHisPage;
	delete m_pClipPage;
	delete m_pMousePage;
	delete m_pScriptPage;
	delete m_pModKeyPage;
	delete m_pBackPage;
}

/////////////////////////////////////////////////////////////////////////////

void COptDlg::AddPage(CTreePage *pPage, CTreePage *pOwn)
{
	pPage->m_pSheet = this;
	pPage->m_pOwn   = pOwn;
	pPage->m_nPage  = (int)m_Tab.Add(pPage);
}
BOOL COptDlg::CreatePage(int nPage)
{
	CRect frame;
	CTreePage *pPage;

	if ( nPage < 0 || nPage >= GetPageCount() || (pPage = GetPage(nPage)) == NULL )
		return FALSE;

	if ( pPage->m_hWnd != NULL )
		return TRUE;

	if ( !pPage->Create(pPage->m_nIDTemplate, this) )
		return FALSE;

	pPage->ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME | WS_POPUP | WS_BORDER, WS_CHILD);
	pPage->ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, 0);
	pPage->SetParent(this);

	m_Frame.GetClientRect(frame);
	m_Frame.ClientToScreen(frame);
	ScreenToClient(frame);
	pPage->MoveWindow(frame, FALSE);

	return TRUE;
}
void COptDlg::SetActivePage(int nPage)
{
	CTreePage *pPage;

	if ( m_ActivePage >= 0 && (pPage = GetPage(m_ActivePage)) != NULL )
		pPage->ShowWindow(SW_HIDE);

	m_ActivePage = (-1);

	if ( CreatePage(nPage) && (pPage = GetPage(nPage)) != NULL ) {
		pPage->ShowWindow(SW_SHOW);
		m_Tree.SetItemState(pPage->m_hTreeItem, TVIS_SELECTED, TVIS_SELECTED);
		m_ActivePage = nPage;
	}

	if ( nPage > 0 && m_bOptFixed && (m_psh.dwFlags & PSH_NOAPPLYNOW) != 0 ) {
		pPage->EnableWindow(FALSE);
		m_MsgWnd.m_bTimerEnable = FALSE;
		m_MsgWnd.Message(CStringLoad(IDS_OPTFIXEDMSG), this, GetSysColor(COLOR_WINDOW));
	} else {
		pPage->EnableWindow(TRUE);
		if ( m_MsgWnd.m_hWnd != NULL )
			m_MsgWnd.DestroyWindow();
	}
}
void COptDlg::SetModified(BOOL bModified)
{
	if ( bModified && !m_bModified ) {
		m_Button[3].EnableWindow(TRUE);
		m_bModified = TRUE;
	}
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(COptDlg, CDialogExt)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TABTREE, &COptDlg::OnSelchangedTree)
	ON_BN_CLICKED(IDC_DOINIT, &COptDlg::OnDoInit)
	ON_BN_CLICKED(ID_APPLY_NOW, &COptDlg::OnApplyNow)
END_MESSAGE_MAP()

void COptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TABTREE,  m_Tree);
	DDX_Control(pDX, IDC_FRAME,    m_Frame);

	DDX_Control(pDX, IDC_DOINIT,   m_Button[0]);
	DDX_Control(pDX, IDOK,         m_Button[1]);
	DDX_Control(pDX, IDCANCEL,     m_Button[2]);
	DDX_Control(pDX, ID_APPLY_NOW, m_Button[3]);
}

/////////////////////////////////////////////////////////////////////////////
// COptDlg メッセージ ハンドラ

static INITDLGTAB ItemTab[] = {
	{ IDC_DOINIT,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ ID_APPLY_NOW,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TABTREE,		ITM_BTM_BTM },
	{ IDC_FRAME,		ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ 0,				0 },
};

BOOL COptDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	int n;
	int cx, cy;
	CRect frame, rect;
	CSize size, pageSize;
	CTreePage *pPage;
	HTREEITEM hti, own;
	CString title;
	WINDOWPLACEMENT place;
	CWnd *pWnd;
	CRect ItemOfs[10];

	// ページの作成と最大サイズ取得

	m_Frame.GetClientRect(frame);
	m_Frame.ClientToScreen(frame);
	ScreenToClient(frame);

	size.cx = frame.Width();
	size.cy = frame.Height();

	for ( n = 0 ; n < GetPageCount() ; n++ ) {
		if ( (pPage = GetPage(n)) == NULL )
			continue;

		if ( !pPage->GetSizeAndText(&pageSize, title, this) )
			continue;

		if ( n < 2 ) {
			pPage->Create(pPage->m_nIDTemplate, this);
			pPage->ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME | WS_POPUP | WS_BORDER, WS_CHILD);
			pPage->ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, 0);
			pPage->SetParent(this);
			pPage->MoveWindow(frame, FALSE);
		}

		if ( size.cx < pageSize.cx )
			size.cx = pageSize.cx;
		if ( size.cy < pageSize.cy )
			size.cy = pageSize.cy;

		own = (pPage->m_pOwn != NULL ? pPage->m_pOwn->m_hTreeItem : NULL);
		if ( (hti = m_Tree.InsertItem(title, own)) != NULL ) {
			m_Tree.SetItemData(hti, (DWORD_PTR)pPage);
			m_Tree.SetItemState(hti, TVIS_EXPANDED, TVIS_EXPANDED);
			pPage->m_hTreeItem = hti;
		}
	}

	// ボタン再配置

	if ( (m_psh.dwFlags & PSH_NOAPPLYNOW) != 0 ) {
		for ( int n = 0 ; n < 4 ; n++ ) {
			m_Button[n].GetWindowRect(ItemOfs[n]);
			ScreenToClient(ItemOfs[n]);
		}

		cx = (ItemOfs[3].right - ItemOfs[2].right) / 2;
		for ( int n = 0 ; n < 3 ; n++ ) {
			ItemOfs[n].left  += cx;
			ItemOfs[n].right += cx;
			m_Button[n].MoveWindow(ItemOfs[n]);
		}
	}

	// アイテムオフセット取得
	CDialogExt::InitItemOffset(ItemTab);

	// ウィンドウサイズ設定

	GetWindowRect(frame);
	m_Frame.GetClientRect(rect);
	cx = size.cx - rect.Width();
	cy = size.cy - rect.Height();
	frame.right  += cx;
	frame.bottom += cy;
	MoveWindow(frame, FALSE);

	// アイテム再配置

	GetClientRect(frame);
	CDialogExt::SetItemOffset(ItemTab, frame.Width(), frame.Height());

	if ( (m_psh.dwFlags & PSH_NOAPPLYNOW) != 0 && (pWnd = GetDlgItem(ID_APPLY_NOW)) != NULL )
		pWnd->ShowWindow(SW_HIDE);

	if ( (pWnd = GetDlgItem(IDC_FRAME)) != NULL ) {
		pWnd->GetWindowPlacement(&place);
		ItemOfs[0] = place.rcNormalPosition;
	}

	for ( n = 0 ; n < GetPageCount() ; n++ ) {
		if ( (pPage = GetPage(n)) == NULL || pPage->m_hWnd == NULL )
			continue;

		pPage->GetWindowPlacement(&place);

		place.rcNormalPosition = ItemOfs[0];
		place.showCmd = SW_HIDE;

		pPage->SetWindowPlacement(&place);
	}

	// ダイアログ初期化

	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	m_Button[3].EnableWindow(FALSE);

	m_bOptFixed = m_pEntry->m_bOptFixed;

	SetActivePage(0);
	m_Tree.SetFocus();

	// CharSetPageの初期化が遅いので先に作成
	CreatePage(10);

	return FALSE;
}
void COptDlg::OnOK()
{
	int n;
	CTreePage *pPage;

	for ( n = 0 ; n < GetPageCount() ; n++ ) {
		if ( (pPage = GetPage(n)) == NULL )
			continue;
		if ( pPage->m_hWnd != NULL )
			pPage->OnApply();
	}

	CDialogExt::OnOK();
}
void COptDlg::OnApplyNow()
{
	int n;
	CTreePage *pPage;

	for ( n = 0 ; n < GetPageCount() ; n++ ) {
		if ( (pPage = GetPage(n)) == NULL )
			continue;
		if ( pPage->m_hWnd != NULL )
			pPage->OnApply();
	}

	if ( m_pDocument != NULL )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
}
void COptDlg::OnDoInit()
{
	CInitAllDlg dlg;

	if ( dlg.DoModal() != IDOK )
		return;

	switch(dlg.m_InitType) {
	case 0:		// Init Default Entry
		CRLoginDoc::LoadDefOption(*m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
		break;

	case 1:		// Init Program Default
		m_pTextRam->Init();
		m_pTextRam->m_FontTab.Init();
		m_pKeyTab->Init();
		m_pKeyMac->Init();
		m_pParamTab->Init();
		break;

	case 2:		// Copy Entry option
		ASSERT(dlg.m_pInitEntry != NULL);
		CRLoginDoc::LoadOption(*(dlg.m_pInitEntry), *m_pTextRam, *m_pKeyTab, *m_pKeyMac, *m_pParamTab);
		break;
	}

	int n;
	CTreePage *pPage;

	for ( n = 0; n < GetPageCount() ; n++ ) {
		if ( !CreatePage(n) || (pPage = GetPage(n)) == NULL )
			continue;
		if ( pPage->m_hWnd != NULL )
			pPage->OnReset();
	}

	if ( m_pDocument != NULL )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	m_ModFlag   = 0;
}
void COptDlg::OnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	HTREEITEM hti;
	CTreePage *pPage;

	if ( (hti = m_Tree.GetSelectedItem()) != NULL && (pPage = (CTreePage *)m_Tree.GetItemData(hti)) != NULL ) {
		SetActivePage(pPage->m_nPage);
		m_Tree.SetFocus();
	}
}

