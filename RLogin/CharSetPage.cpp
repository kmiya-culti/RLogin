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

CCharSetPage::CCharSetPage() : CTreePropertyPage(CCharSetPage::IDD)
{
	m_KanjiCode = -1;
	m_CharBankGL = -1;
	m_CharBankGR = -1;
	m_CharBank1 = _T("");
	m_CharBank2 = _T("");
	m_CharBank3 = _T("");
	m_CharBank4 = _T("");
	m_AltFont = 0;
}

CCharSetPage::~CCharSetPage()
{
}

void CCharSetPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_FONTLIST, m_List);
	DDX_Radio(pDX, IDC_CHARSET1, m_KanjiCode);
	DDX_Radio(pDX, IDC_BANKGL1, m_CharBankGL);
	DDX_Radio(pDX, IDC_BANKGR1, m_CharBankGR);
	DDX_CBString(pDX, IDC_CHARBANK1, m_CharBank1);
	DDX_CBString(pDX, IDC_CHARBANK2, m_CharBank2);
	DDX_CBString(pDX, IDC_CHARBANK3, m_CharBank3);
	DDX_CBString(pDX, IDC_CHARBANK4, m_CharBank4);
	DDX_CBIndex(pDX, IDC_FONTNUM, m_AltFont);
}

BEGIN_MESSAGE_MAP(CCharSetPage, CPropertyPage)
	ON_BN_CLICKED(IDC_FONTLISTNEW, OnFontListNew)
	ON_BN_CLICKED(IDC_FONTLISTEDIT, OnFontListEdit)
	ON_BN_CLICKED(IDC_FONTLISTDEL, OnFontListDel)
	ON_BN_CLICKED(IDC_ICONVSET, OnIconvSet)
	ON_NOTIFY(NM_DBLCLK, IDC_FONTLIST, OnDblclkFontlist)
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
	ON_CBN_SELCHANGE(IDC_FONTNUM, &CCharSetPage::OnCbnSelchangeFontnum)
END_MESSAGE_MAP()

void CCharSetPage::InitList()
{
	int n, i;
	static LPCTSTR bankTab[] = { _T("94"), _T("96"), _T("94x94"), _T("96x96") };

	m_List.DeleteAllItems();
	for ( n = i = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_FontTab[n].m_EntryName.IsEmpty() )
			continue;
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, i, m_FontTab[n].m_EntryName, 0, 0, 0, n);
		m_List.SetItemText(i, 1, (n == SET_UNICODE ? _T("") : bankTab[n >> 8]));
		m_List.SetItemText(i, 2, m_FontTab[n].m_IndexName);
		m_List.SetItemText(i, 3, m_FontTab[n].m_FontName[m_AltFont]);
		m_List.SetItemData(i, n);
		i++;
	}
	m_List.DoSortItem();
}

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage メッセージ ハンドラ

static const LV_COLUMN InitListTab[6] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, _T("Entry"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Bank"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Code"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 100, _T("Face"),  0, 0 }, 
	};

void CCharSetPage::DoInit()
{

	m_AltFont    = m_pSheet->m_pTextRam->m_AttNow.ft;
	m_FontTab    = m_pSheet->m_pTextRam->m_FontTab;
	m_KanjiCode  = m_pSheet->m_pTextRam->m_KanjiMode;
	m_CharBankGL = m_pSheet->m_pTextRam->m_BankGL;
	m_CharBankGR = m_pSheet->m_pTextRam->m_BankGR;

	memcpy(m_BankTab, m_pSheet->m_pTextRam->m_BankTab, sizeof(m_BankTab));
	m_CharBank1  = m_FontTab[m_BankTab[m_KanjiCode][0]].m_EntryName;
	m_CharBank2  = m_FontTab[m_BankTab[m_KanjiCode][1]].m_EntryName;
	m_CharBank3  = m_FontTab[m_BankTab[m_KanjiCode][2]].m_EntryName;
	m_CharBank4  = m_FontTab[m_BankTab[m_KanjiCode][3]].m_EntryName;

	for ( int n = 0 ; n < 4 ; n++ )
		m_SendCharSet[n] = m_pSheet->m_pTextRam->m_SendCharSet[n];

	InitList();
	UpdateData(FALSE);
}
BOOL CCharSetPage::OnInitDialog() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn(_T("CharSetPage"), InitListTab, 4);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);

	DoInit();

	int n, i;
	CComboBox *pCombo[4];
	LPCTSTR pStr;

	for ( i = 0 ; i < 4 ; i++ )
		pCombo[i] = (CComboBox *)GetDlgItem(IDC_CHARBANK1 + i);

	for ( n = 0 ; n < CODE_MAX ; n++ ) {
		pStr = m_FontTab[n].m_EntryName;
		if ( *pStr == _T('\0') )
			continue;
		for ( i = 0 ; i < 4 ; i++ ) {
			if ( pCombo[i] != NULL && pCombo[i]->FindString((-1), pStr) == CB_ERR )
				pCombo[i]->AddString(pStr);
		}
	}

	return TRUE;
}
BOOL CCharSetPage::OnApply() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	UpdateData(TRUE);

	m_pSheet->m_pTextRam->m_AttNow.ft = m_AltFont;
	m_pSheet->m_pTextRam->SetKanjiMode(m_KanjiCode);
	m_pSheet->m_pTextRam->m_BankGL    = m_CharBankGL;
	m_pSheet->m_pTextRam->m_BankGR    = m_CharBankGR;
	m_pSheet->m_pTextRam->m_FontTab   = m_FontTab;

	m_BankTab[m_KanjiCode][0] = m_FontTab.Find(m_CharBank1);
	m_BankTab[m_KanjiCode][1] = m_FontTab.Find(m_CharBank2);
	m_BankTab[m_KanjiCode][2] = m_FontTab.Find(m_CharBank3);
	m_BankTab[m_KanjiCode][3] = m_FontTab.Find(m_CharBank4);

	memcpy(m_pSheet->m_pTextRam->m_BankTab, m_BankTab, sizeof(m_BankTab));

	for ( int n = 0 ; n < 4 ; n++ )
		m_pSheet->m_pTextRam->m_SendCharSet[n] = m_SendCharSet[n];

	m_List.SaveColumn(_T("CharSetPage"));

	return TRUE;
}
void CCharSetPage::OnReset() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	DoInit();
	SetModified(FALSE);
}
void CCharSetPage::OnCharSet(UINT nID) 
{
	UpdateData(TRUE);
	m_CharBank1  = m_FontTab[m_BankTab[m_KanjiCode][0]].m_EntryName;
	m_CharBank2  = m_FontTab[m_BankTab[m_KanjiCode][1]].m_EntryName;
	m_CharBank3  = m_FontTab[m_BankTab[m_KanjiCode][2]].m_EntryName;
	m_CharBank4  = m_FontTab[m_BankTab[m_KanjiCode][3]].m_EntryName;
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_BANKTAB);
}
void CCharSetPage::OnFontListNew() 
{
	CFontParaDlg dlg;
	CFontNode tmp;

	dlg.m_CodeSet = 255;
	dlg.m_pData   = &tmp;
	dlg.m_FontNum = m_AltFont;
	dlg.m_pFontTab = &m_FontTab;

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
	dlg.m_FontNum = m_AltFont;
	dlg.m_pFontTab = &m_FontTab;

	if ( dlg.DoModal() != IDOK )
		return;

	m_FontTab[dlg.m_CodeSet] = tmp;
	InitList();
	if ( (n = m_List.GetParamItem(dlg.m_CodeSet)) >= 0 ) {
		m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
		m_List.EnsureVisible(n, FALSE);
	}

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnFontListDel() 
{
	int n;

	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;

	m_FontTab.IndexRemove(n);
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
	dlg.m_FontNum = m_AltFont;
	dlg.m_pFontTab = &m_FontTab;

	if ( dlg.DoModal() != IDOK )
		return;

	m_FontTab[dlg.m_CodeSet] = tmp;
	InitList();
	if ( (n = m_List.GetParamItem(dlg.m_CodeSet)) >= 0 ) {
		m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
		m_List.EnsureVisible(n, FALSE);
	}

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnIconvSet() 
{
	int n;
	CIConvDlg dlg;

	for ( n = 0 ; n < 4 ; n++ )
		dlg.m_CharSet[n] = m_SendCharSet[n];

	if ( dlg.DoModal() != IDOK )
		return;

	for ( n = 0 ; n < 4 ; n++ )
		m_SendCharSet[n] = dlg.m_CharSet[n];

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
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_BANKTAB);
}
void CCharSetPage::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CCharSetPage::OnCbnSelchangeFontnum()
{
	UpdateData(TRUE);
	InitList();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
