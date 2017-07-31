// CharSetPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "OPtDlg.h"
#include "TextRam.h"
#include "Data.h"
#include "CharSetPage.h"
#include "FontParaDlg.h"
#include "IConvDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage プロパティ ページ

IMPLEMENT_DYNCREATE(CCharSetPage, CPropertyPage)

CCharSetPage::CCharSetPage() : CPropertyPage(CCharSetPage::IDD)
{
	//{{AFX_DATA_INIT(CCharSetPage)
	m_KanjiCode = -1;
	m_CharBankGL = -1;
	m_CharBankGR = -1;
	m_CharBank1 = _T("");
	m_CharBank2 = _T("");
	m_CharBank3 = _T("");
	m_CharBank4 = _T("");
	//}}AFX_DATA_INIT
	m_pSheet = NULL;
	m_pData = NULL;
}

CCharSetPage::~CCharSetPage()
{
}

void CCharSetPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCharSetPage)
	DDX_Control(pDX, IDC_FONTLIST, m_List);
	DDX_Radio(pDX, IDC_CHARSET1, m_KanjiCode);
	DDX_Radio(pDX, IDC_BANKGL1, m_CharBankGL);
	DDX_Radio(pDX, IDC_BANKGR1, m_CharBankGR);
	DDX_CBString(pDX, IDC_CHARBANK1, m_CharBank1);
	DDX_CBString(pDX, IDC_CHARBANK2, m_CharBank2);
	DDX_CBString(pDX, IDC_CHARBANK3, m_CharBank3);
	DDX_CBString(pDX, IDC_CHARBANK4, m_CharBank4);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCharSetPage, CPropertyPage)
	//{{AFX_MSG_MAP(CCharSetPage)
	ON_BN_CLICKED(IDC_FONTLISTNEW, OnFontListNew)
	ON_BN_CLICKED(IDC_FONTLISTEDIT, OnFontListEdit)
	ON_BN_CLICKED(IDC_FONTLISTDEL, OnFontListDel)
	ON_BN_CLICKED(IDC_ICONVSET, OnIconvSet)
	ON_NOTIFY(NM_DBLCLK, IDC_FONTLIST, OnDblclkFontlist)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHARSET1, IDC_CHARSET4, OnCharSet)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_BANKGL1,  IDC_BANKGL4,  OnUpdateCheck)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_BANKGR1,  IDC_BANKGR4,  OnUpdateCheck)
	ON_CONTROL_RANGE(CBN_EDITCHANGE, IDC_CHARBANK1, IDC_CHARBANK4, OnUpdateCheck)
	ON_CONTROL_RANGE(CBN_SELCHANGE,  IDC_CHARBANK1, IDC_CHARBANK4, OnUpdateCheck)
	ON_COMMAND(ID_EDIT_NEW, OnFontListNew)
	ON_COMMAND(ID_EDIT_UPDATE, OnFontListEdit)
	ON_COMMAND(ID_EDIT_DELETE, OnFontListDel)
	ON_COMMAND(ID_EDIT_DUPS, OnEditDups)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
END_MESSAGE_MAP()

void CCharSetPage::InitList()
{
	int n, i;
	CString code, bank;

	m_List.DeleteAllItems();
	for ( n = i = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_FontTab[n].m_EntryName.IsEmpty() )
			continue;
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, i, m_FontTab[n].m_EntryName, 0, 0, 0, n);
		CFontParaDlg::CodeSetName(n, bank, code);
		m_List.SetItemText(i, 1, bank);
		m_List.SetItemText(i, 2, code);
		m_List.SetItemText(i, 3, m_FontTab[n].m_FontName);
		m_List.SetItemData(i, n);
		i++;
	}
	m_List.DoSortItem();
}

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage メッセージ ハンドラ

static const LV_COLUMN InitListTab[6] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, "Entry", 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, "Bank",  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  40, "Code",  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 100, "Face",  0, 0 }, 
	};

BOOL CCharSetPage::OnInitDialog() 
{
	ASSERT(m_pSheet);
	m_pData = m_pSheet->m_pTextRam;
	ASSERT(m_pData);

	CPropertyPage::OnInitDialog();

	m_FontTab    = m_pData->m_FontTab;
	m_KanjiCode  = m_pData->m_KanjiMode;
	m_CharBankGL = m_pData->m_BankGL;
	m_CharBankGR = m_pData->m_BankGR;
	m_CharBank1  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][0]].m_EntryName;
	m_CharBank2  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][1]].m_EntryName;
	m_CharBank3  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][2]].m_EntryName;
	m_CharBank4  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][3]].m_EntryName;

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn("CharSetPage", InitListTab, 4);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);
	InitList();

	int n, i;
	CComboBox *pCombo[4];
	LPCSTR pStr;

	for ( i = 0 ; i < 4 ; i++ )
		pCombo[i] = (CComboBox *)GetDlgItem(IDC_CHARBANK1 + i);
	for ( n = 0 ; n < CODE_MAX ; n++ ) {
		pStr = m_FontTab[n].m_EntryName;
		if ( *pStr == '\0' )
			continue;
		for ( i = 0 ; i < 4 ; i++ ) {
			if ( pCombo[i] != NULL && pCombo[i]->FindString((-1), pStr) == CB_ERR )
				pCombo[i]->AddString(pStr);
		}
	}

	UpdateData(FALSE);
	return TRUE;
}
BOOL CCharSetPage::OnApply() 
{
	ASSERT(m_pData);
	UpdateData(TRUE);

	m_pData->SetKanjiMode(m_KanjiCode);
	m_pData->m_BankGL    = m_CharBankGL;
	m_pData->m_BankGR    = m_CharBankGR;
	m_pData->m_BankTab[m_KanjiCode][0] = m_FontTab.Find(m_CharBank1);
	m_pData->m_BankTab[m_KanjiCode][1] = m_FontTab.Find(m_CharBank2);
	m_pData->m_BankTab[m_KanjiCode][2] = m_FontTab.Find(m_CharBank3);
	m_pData->m_BankTab[m_KanjiCode][3] = m_FontTab.Find(m_CharBank4);
	memcpy(m_pData->m_DefBankTab, m_pData->m_BankTab, sizeof(m_pData->m_BankTab));
	m_pData->m_FontTab = m_FontTab;

	m_List.SaveColumn("CharSetPage");
	return TRUE;
}
void CCharSetPage::OnReset() 
{
	if ( m_hWnd == NULL )
		return;

	m_FontTab    = m_pData->m_FontTab;
	m_KanjiCode  = m_pData->m_KanjiMode;
	m_CharBankGL = m_pData->m_BankGL;
	m_CharBankGR = m_pData->m_BankGR;
	memcpy(m_pData->m_BankTab, m_pData->m_DefBankTab, sizeof(m_pData->m_BankTab));
	m_CharBank1  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][0]].m_EntryName;
	m_CharBank2  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][1]].m_EntryName;
	m_CharBank3  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][2]].m_EntryName;
	m_CharBank4  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][3]].m_EntryName;

	InitList();
	UpdateData(FALSE);
	SetModified(FALSE);
}
void CCharSetPage::OnCharSet(UINT nID) 
{
	UpdateData(TRUE);
	m_CharBank1  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][0]].m_EntryName;
	m_CharBank2  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][1]].m_EntryName;
	m_CharBank3  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][2]].m_EntryName;
	m_CharBank4  = m_FontTab[m_pData->m_BankTab[m_KanjiCode][3]].m_EntryName;
	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnFontListNew() 
{
	CFontParaDlg dlg;
	CFontNode tmp;
	dlg.m_CodeSet = 0;
	dlg.m_pData   = &tmp;
	if ( dlg.DoModal() != IDOK )
		return;
	m_FontTab[dlg.m_CodeSet] = tmp;
	InitList();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnFontListEdit() 
{
	int n;
	CFontParaDlg dlg;
	CFontNode tmp;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	tmp = m_FontTab[n];
	dlg.m_CodeSet = n;
	dlg.m_pData   = &tmp;
	if ( dlg.DoModal() != IDOK )
		return;
	m_FontTab[dlg.m_CodeSet] = tmp;
	InitList();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnFontListDel() 
{
	int n;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	m_FontTab[n].Init();
	InitList();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnEditDups() 
{
	int n;
	CFontParaDlg dlg;
	CFontNode tmp;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	tmp = m_FontTab[n];
	dlg.m_CodeSet = n;
	dlg.m_pData   = &tmp;
	if ( dlg.DoModal() != IDOK )
		return;
	m_FontTab[dlg.m_CodeSet] = tmp;
	InitList();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnIconvSet() 
{
	int n;
	CIConvDlg dlg;

	for ( n = 0 ; n < 4 ; n++ )
		dlg.m_CharSet[n] = m_pSheet->m_pTextRam->m_SendCharSet[n];

	if ( dlg.DoModal() != IDOK )
		return;

	for ( n = 0 ; n < 4 ; n++ )
		m_pSheet->m_pTextRam->m_SendCharSet[n] = dlg.m_CharSet[n];

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnDblclkFontlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnFontListEdit();
	*pResult = 0;
}
void CCharSetPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}
