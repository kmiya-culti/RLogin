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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	KEYTABMAX	52

/////////////////////////////////////////////////////////////////////////////
// CKeyPage プロパティ ページ

IMPLEMENT_DYNCREATE(CKeyPage, CPropertyPage)

CKeyPage::CKeyPage() : CTreePropertyPage(CKeyPage::IDD)
{
	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		m_KeyMap[n] = FALSE;
}

CKeyPage::~CKeyPage()
{
}

void CKeyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_KEYLIST, m_List);

	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		DDX_Check(pDX, IDC_METAKEY1 + n, m_KeyMap[n]);
}

BEGIN_MESSAGE_MAP(CKeyPage, CPropertyPage)
	ON_BN_CLICKED(IDC_KEYASNNEW, OnKeyAsnNew)
	ON_BN_CLICKED(IDC_KEYASNEDIT, OnKeyAsnEdit)
	ON_BN_CLICKED(IDC_KEYASNDEL, OnKeyAsnDel)
	ON_NOTIFY(NM_DBLCLK, IDC_KEYLIST, OnDblclkKeyList)
	ON_COMMAND(ID_EDIT_NEW, OnKeyAsnNew)
	ON_COMMAND(ID_EDIT_UPDATE, OnKeyAsnEdit)
	ON_COMMAND(ID_EDIT_DELETE, OnKeyAsnDel)
	ON_COMMAND(ID_EDIT_DUPS, OnEditDups)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
	ON_BN_CLICKED(IDC_ALL_SET, &CKeyPage::OnBnClickedAllSet)
	ON_BN_CLICKED(IDC_ALL_CLR, &CKeyPage::OnBnClickedAllClr)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_METAKEY1, IDC_METAKEY1 + KEYTABMAX - 1, &CKeyPage::OnBnClickedMetakey)
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
		{ LVCF_TEXT | LVCF_WIDTH, 0,  80, _T("Code"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 100, _T("With"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, _T("String"),  0, 0 }, 
	};

static const BYTE VKeyTab[] = {
	'1',			'2',			'3',			'4',			'5',
	'6',			'7',			'8',			'9',			'0',
	VK_OEM_MINUS,	VK_OEM_7,		VK_OEM_5,		VK_BACK,		VK_TAB,
	'Q',			'W',			'E',			'R',			'T',
	'Y',			'U',			'I',			'O',			'P',
	VK_OEM_3,		VK_OEM_4,
	'A',			'S',			'D',			'F',			'G',
	'H',			'J',			'K',			'L',			VK_OEM_PLUS,
	VK_OEM_1,		VK_OEM_6,		VK_RETURN,
	'Z',			'X',			'C',			'V',			'B',
	'N',			'M',			VK_OEM_COMMA,	VK_OEM_PERIOD,	VK_OEM_2,
	VK_OEM_102,		VK_SPACE,
};

void CKeyPage::DoInit()
{
	m_KeyTab = *(m_pSheet->m_pKeyTab);
	memcpy(m_MetaKeys, m_pSheet->m_pTextRam->m_MetaKeys, sizeof(m_MetaKeys));

	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		m_KeyMap[n] = (m_MetaKeys[VKeyTab[n] / 32] & (1 << (VKeyTab[n] % 32)) ? TRUE : FALSE);

	InitList();
	UpdateData(FALSE);
}
BOOL CKeyPage::OnInitDialog() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pKeyTab != NULL);

	CPropertyPage::OnInitDialog();

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn(_T("CKeyPage"), InitListTab, 3);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);

	DoInit();

	return TRUE;
}
BOOL CKeyPage::OnApply() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pKeyTab != NULL);

	UpdateData(TRUE);

	memset(m_MetaKeys, 0, sizeof(m_MetaKeys));

	for ( int n = 0 ; n < KEYTABMAX ; n++ ) {
		if ( m_KeyMap[n] )
			m_MetaKeys[VKeyTab[n] / 32] |= (1 << (VKeyTab[n] % 32));
	}

	memcpy(m_pSheet->m_pTextRam->m_MetaKeys, m_MetaKeys, sizeof(m_MetaKeys));
	*(m_pSheet->m_pKeyTab) = m_KeyTab;

	m_List.SaveColumn(_T("CKeyPage"));

	return TRUE;
}
void CKeyPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pKeyTab != NULL);

	DoInit();
	SetModified(FALSE);
}

void CKeyPage::OnKeyAsnNew() 
{
	CKeyNode tmp;
	CKeyParaDlg dlg;

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

	m_KeyTab.RemoveAt(n);

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

void CKeyPage::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CKeyPage::OnBnClickedAllSet()
{
	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		m_KeyMap[n] = TRUE;
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}

void CKeyPage::OnBnClickedAllClr()
{
	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		m_KeyMap[n] = FALSE;
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}

void CKeyPage::OnBnClickedMetakey(UINT nID)
{
	int n = nID - IDC_METAKEY1;

	UpdateData(TRUE);
	m_KeyMap[n] = (m_KeyMap[n] ? FALSE : TRUE);
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
