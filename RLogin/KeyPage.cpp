// KeyPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "OPtDlg.h"
#include "TextRam.h"
#include "Data.h"
#include "KeyPage.h"
#include "KeyParaDlg.h"
#include "MetaDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CKeyPage プロパティ ページ

IMPLEMENT_DYNCREATE(CKeyPage, CPropertyPage)

#define	CHECKOPTMAX		4
#define	IDC_CHECKFAST	IDC_PROTOCHECK1
static const int CheckOptTab[] = { TO_RLRCLICK, TO_RLGAWL, TO_RLCKCOPY, TO_RLSPCTAB };

CKeyPage::CKeyPage() : CPropertyPage(CKeyPage::IDD)
{
	//{{AFX_DATA_INIT(CKeyPage)
		// メモ - ClassWizard はこの位置にメンバの初期化処理を追加します。
	//}}AFX_DATA_INIT
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_pSheet = NULL;
	m_DropMode = 0;
	m_CmdWork = _T("");
	m_WordStr = _T("");
}

CKeyPage::~CKeyPage()
{
}

void CKeyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CKeyPage)
	DDX_Control(pDX, IDC_KEYLIST, m_List);
	//}}AFX_DATA_MAP
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_PROTOCHECK1 + n, m_Check[n]);
	DDX_CBIndex(pDX, IDC_COMBO1, m_DropMode);
	DDX_Text(pDX, IDC_EDIT1, m_CmdWork);
	DDX_Text(pDX, IDC_EDIT3, m_WordStr);
}

BEGIN_MESSAGE_MAP(CKeyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CKeyPage)
	ON_BN_CLICKED(IDC_KEYASNNEW, OnKeyAsnNew)
	ON_BN_CLICKED(IDC_KEYASNEDIT, OnKeyAsnEdit)
	ON_BN_CLICKED(IDC_KEYASNDEL, OnKeyAsnDel)
	ON_NOTIFY(NM_DBLCLK, IDC_KEYLIST, OnDblclkKeyList)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_COMMAND(ID_EDIT_NEW, OnKeyAsnNew)
	ON_COMMAND(ID_EDIT_UPDATE, OnKeyAsnEdit)
	ON_COMMAND(ID_EDIT_DELETE, OnKeyAsnDel)
	ON_COMMAND(ID_EDIT_DUPS, OnEditDups)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
	ON_EN_CHANGE(IDC_EDIT1, &CKeyPage::OnUpdateChangeDropMode)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CKeyPage::OnCbnSelchangeDropMode)
	ON_EN_CHANGE(IDC_EDIT3, &CKeyPage::OnEnChangeEdit3)
	ON_BN_CLICKED(IDC_KEYASNMETA, &CKeyPage::OnKeyAsnMeta)
END_MESSAGE_MAP()

void CKeyPage::InitList()
{
	int n;

	m_List.DeleteAllItems();
	for ( n = 0 ; n < m_KeyTab.GetSize() ; n++ ) {
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, m_KeyTab[n].GetCode(), 0, 0, 0, n);
		m_List.SetItemText(n, 1, m_KeyTab[n].GetMask());
		m_List.SetItemText(n, 2, m_KeyTab[n].GetMaps());
	}
	m_List.DoSortItem();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyPage メッセージ ハンドラ

static const LV_COLUMN InitListTab[] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  80, "Code", 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 100, "With",  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, "String",  0, 0 }, 
	};

BOOL CKeyPage::OnInitDialog() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pKeyTab);

	CPropertyPage::OnInitDialog();
	
	m_KeyTab = *(m_pSheet->m_pKeyTab);
	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn("CKeyPage", InitListTab, 3);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);
	InitList();

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_DropMode = m_pSheet->m_pTextRam->m_DropFileMode;
	for ( int n = 0 ; n < 8 ; n++ )
		m_DropCmd[n] = m_pSheet->m_pTextRam->m_DropFileCmd[n];
	m_CmdWork  = m_DropCmd[m_DropMode];

	m_WordStr = m_pSheet->m_pTextRam->m_WordStr;

	UpdateData(FALSE);

	return TRUE;
}
BOOL CKeyPage::OnApply() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pKeyTab);

	*(m_pSheet->m_pKeyTab) = m_KeyTab;

	UpdateData(TRUE);
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ ) {
		if ( m_Check[n] )
			m_pSheet->m_pTextRam->EnableOption(CheckOptTab[n]);
		else
			m_pSheet->m_pTextRam->DisableOption(CheckOptTab[n]);
	}

	m_pSheet->m_pTextRam->m_DropFileMode = m_DropMode;
	for ( int n = 0 ; n < 8 ; n++ )
		m_pSheet->m_pTextRam->m_DropFileCmd[n] = m_DropCmd[n];

	m_pSheet->m_pTextRam->m_WordStr = m_WordStr;

	m_List.SaveColumn("CKeyPage");
	return TRUE;
}
void CKeyPage::OnReset() 
{
	if ( m_hWnd == NULL )
		return;

	m_KeyTab = *(m_pSheet->m_pKeyTab);
	InitList();

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_DropMode = m_pSheet->m_pTextRam->m_DropFileMode;
	for ( int n = 0 ; n < 8 ; n++ )
		m_DropCmd[n] = m_pSheet->m_pTextRam->m_DropFileCmd[n];
	m_CmdWork  = m_DropCmd[m_DropMode];

	m_WordStr = m_pSheet->m_pTextRam->m_WordStr;

	UpdateData(FALSE);
	SetModified(FALSE);
}

void CKeyPage::OnKeyAsnNew() 
{
	CKeyParaDlg dlg;
	CKeyNode tmp;

	dlg.m_pData = &tmp;
	if ( dlg.DoModal() != IDOK )
		return;
	m_KeyTab.Add(tmp);
	InitList();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnKeyAsnEdit() 
{
	int n;
	CKeyParaDlg dlg;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	dlg.m_pData = &(m_KeyTab[n]);
	if ( dlg.DoModal() != IDOK )
		return;
	InitList();
	if ( (n = m_List.GetParamItem(n)) >= 0 ) {
		m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
		m_List.EnsureVisible(n, FALSE);
	}
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnKeyAsnDel() 
{
	int n;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	m_KeyTab.m_Node.RemoveAt(n);
	InitList();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnEditDups() 
{
	int n;
	CKeyParaDlg dlg;
	CKeyNode tmp;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	tmp = m_KeyTab[n];
	dlg.m_pData = &tmp;
	if ( dlg.DoModal() != IDOK )
		return;
	m_KeyTab.Add(tmp);
	InitList();
	if ( (n = m_List.GetParamItem(n)) >= 0 ) {
		m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
		m_List.EnsureVisible(n, FALSE);
	}
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnDblclkKeyList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnKeyAsnEdit();
	*pResult = 0;
}

void CKeyPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CKeyPage::OnUpdateChangeDropMode()
{
	UpdateData(TRUE);
	m_DropCmd[m_DropMode] = m_CmdWork;
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CKeyPage::OnCbnSelchangeDropMode()
{
	UpdateData(TRUE);
	m_CmdWork = m_DropCmd[m_DropMode];
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CKeyPage::OnEnChangeEdit3()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}

void CKeyPage::OnKeyAsnMeta()
{
	CMetaDlg dlg;

	dlg.m_pMetaMap = m_pSheet->m_pTextRam->m_MetaKeys;

	if ( dlg.DoModal() != IDOK )
		return;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
