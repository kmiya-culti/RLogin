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


#define	VK_106JP	0
#define	VK_101US	1

#define	VK_NON		0xFF
#define	KEYTABMAX	56

/////////////////////////////////////////////////////////////////////////////
// CKeyPage プロパティ ページ

IMPLEMENT_DYNCREATE(CKeyPage, CTreePage)

CKeyPage::CKeyPage() : CTreePage(CKeyPage::IDD)
{
	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		m_KeyMap[n] = FALSE;
	m_KeyLayout = VK_101US;
}

CKeyPage::~CKeyPage()
{
}

void CKeyPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_KEYLIST, m_List);

	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		DDX_Check(pDX, IDC_METAKEY1 + n, m_KeyMap[n]);
}

BEGIN_MESSAGE_MAP(CKeyPage, CTreePage)
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
	ON_COMMAND(ID_EDIT_DELALL, &CKeyPage::OnEditDelall)
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

//	106 JP
//	---			0	(-)VK_OEM_MINUS		(^)VK_OEM_7			(\)VK_OEM_5
//				P	(@)VK_OEM_3			([)VK_OEM_4			---
//				L	(;)VK_OEM_PLUS		(:)VK_OEM_1			(])VK_OEM_6
//				M	(,)VK_OEM_COMMA		(.)VK_OEM_PERIOD	(/)VK_OEM_2		(\)VK_OEM_102
//
//	101 US
//	(`)VK_OEM_3	0	(~)VK_OEM_MINUS		(=)VK_OEM_PLUS		---
//				P	([)VK_OEM_4			(])VK_OEM_6			(＼)VK_OEM_5
//				L	(;)VK_OEM_1			(')VK_OEM_7			---
//				M	(,)VK_OEM_COMMA		(.)VK_OEM_PERIOD	(/)VK_OEM_2		---

static const BYTE VKeyTab[2][KEYTABMAX] = {
	// VK_106JP
	{	'1',			'2',			'3',			'4',			'5',
		'6',			'7',			'8',			'9',			'0',
		VK_OEM_MINUS,	VK_OEM_7,		VK_OEM_5,		VK_BACK,		VK_TAB,
		'Q',			'W',			'E',			'R',			'T',
		'Y',			'U',			'I',			'O',			'P',
		VK_OEM_3,		VK_OEM_4,		'A',			'S',			'D',
		'F',			'G',			'H',			'J',			'K',
		'L',			VK_OEM_PLUS,	VK_OEM_1,		VK_OEM_6,		VK_RETURN,
		'Z',			'X',			'C',			'V',			'B',
		'N',			'M',			VK_OEM_COMMA,	VK_OEM_PERIOD,	VK_OEM_2,
		VK_OEM_102,		VK_SPACE,
		VK_NON,			VK_NON,			VK_NON,			VK_NON,			},
	// VK_101US
	{	'1',			'2',			'3',			'4',			'5',
		'6',			'7',			'8',			'9',			'0',
		VK_OEM_MINUS,	VK_OEM_PLUS,	VK_NON,			VK_NON,			VK_TAB,
		'Q',			'W',			'E',			'R',			'T',
		'Y',			'U',			'I',			'O',			'P',
		VK_OEM_4,		VK_OEM_6,		'A',			'S',			'D',
		'F',			'G',			'H',			'J',			'K',
		'L',			VK_OEM_1,		VK_OEM_7,		VK_NON,			VK_NON,
		'Z',			'X',			'C',			'V',			'B',
		'N',			'M',			VK_OEM_COMMA,	VK_OEM_PERIOD,	VK_OEM_2,
		VK_NON,			VK_SPACE,
		VK_OEM_3,		VK_OEM_5,		VK_BACK,		VK_RETURN,		},
};
static const LPCTSTR VKeyName[2][KEYTABMAX] = {
	// VK_106JP
	{	_T("1"),		_T("2"),		_T("3"),		_T("4"),		_T("5"),
		_T("6"),		_T("7"),		_T("8"),		_T("9"),		_T("0"),
		_T("-"),		_T("^"),		_T("￥"),		_T("BS"),		_T("TAB"),
		_T("Q"),		_T("W"),		_T("E"),		_T("R"),		_T("T"),
		_T("Y"),		_T("U"),		_T("I"),		_T("O"),		_T("P"),
		_T("@"),		_T("["),		_T("A"),		_T("S"),		_T("D"),
		_T("F"),		_T("G"),		_T("H"),		_T("J"),		_T("K"),
		_T("L"),		_T(";"),		_T(":"),		_T("]"),		_T("←"),
		_T("Z"),		_T("X"),		_T("C"),		_T("V"),		_T("B"),
		_T("N"),		_T("M"),		_T(","),		_T("."),		_T("/"),
		_T("＼"),		_T("SP"),		_T(""),			_T(""),			_T(""),
		_T(""),			},
	// VK_101US
	{	_T("1"),		_T("2"),		_T("3"),		_T("4"),		_T("5"),
		_T("6"),		_T("7"),		_T("8"),		_T("9"),		_T("0"),
		_T("-"),		_T("="),		_T(""),			_T(""),			_T("TAB"),
		_T("Q"),		_T("W"),		_T("E"),		_T("R"),		_T("T"),
		_T("Y"),		_T("U"),		_T("I"),		_T("O"),		_T("P"),
		_T("["),		_T("]"),		_T("A"),		_T("S"),		_T("D"),
		_T("F"),		_T("G"),		_T("H"),		_T("J"),		_T("K"),
		_T("L"),		_T(";"),		_T("'"),		_T(""),			_T(""),
		_T("Z"),		_T("X"),		_T("C"),		_T("V"),		_T("B"),
		_T("N"),		_T("M"),		_T(","),		_T("."),		_T("/"),
		_T(""),			_T("SP"),		_T("`"),		_T("＼"),		_T("BS"),
		_T("←"),		},
};
void CKeyPage::DoInit()
{
	HKL hk;
	CButton *pWnd;

	hk = GetKeyboardLayout(0);

	switch(HIWORD(hk)) {
	case 0x0411:
		m_KeyLayout = VK_106JP;
		break;
	default:
		m_KeyLayout = VK_101US;
		break;
	}

	m_KeyTab = *(m_pSheet->m_pKeyTab);
	memcpy(m_MetaKeys, m_pSheet->m_pTextRam->m_MetaKeys, sizeof(m_MetaKeys));

	for ( int n = 0 ; n < KEYTABMAX ; n++ ) {
		if ( (pWnd = (CButton *)GetDlgItem(IDC_METAKEY1 + n)) == NULL )
			continue;
		if ( VKeyTab[m_KeyLayout][n] == VK_NON )
			pWnd->ShowWindow(SW_HIDE);
		else {
			pWnd->SetWindowText(VKeyName[m_KeyLayout][n]);
			pWnd->ShowWindow(SW_SHOW);
			m_KeyMap[n] = (m_MetaKeys[VKeyTab[m_KeyLayout][n] / 32] & (1 << (VKeyTab[m_KeyLayout][n] % 32)) ? TRUE : FALSE);
		}
	}

	InitList();
	UpdateData(FALSE);
}
BOOL CKeyPage::OnInitDialog() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pKeyTab != NULL);

	CTreePage::OnInitDialog();

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn(_T("CKeyPage"), InitListTab, 3);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 6);

	DoInit();

	return TRUE;
}
BOOL CKeyPage::OnApply() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pKeyTab != NULL);

	UpdateData(TRUE);

	memset(m_MetaKeys, 0, sizeof(m_MetaKeys));

	for ( int n = 0 ; n < KEYTABMAX ; n++ ) {
		if ( m_KeyMap[n] && VKeyTab[m_KeyLayout][n] != VK_NON )
			m_MetaKeys[VKeyTab[m_KeyLayout][n] / 32] |= (1 << (VKeyTab[m_KeyLayout][n] % 32));
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

void CKeyPage::OnEditDelall()
{
	if ( MessageBox(CStringLoad(IDS_ALLINITREQ), _T("Warning"), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
		return;

	m_KeyTab.Init();
	InitList();

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
