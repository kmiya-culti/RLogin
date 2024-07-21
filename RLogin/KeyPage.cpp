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
#include "InitAllDlg.h"

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
	m_UrlOpt = _T("#KEYOPT");

	for ( int n = 0 ; n < KEYTABMAX ; n++ )
		m_KeyMap[n] = FALSE;
	m_KeyLayout = VK_101US;
	ZeroMemory(m_MetaKeys, sizeof(m_MetaKeys));
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
	ON_COMMAND(ID_EDIT_NEW, OnKeyAsnNew)
	ON_COMMAND(ID_EDIT_UPDATE, OnKeyAsnEdit)
	ON_COMMAND(ID_EDIT_DELETE, OnKeyAsnDel)
	ON_COMMAND(ID_EDIT_DUPS, OnEditDups)
	ON_COMMAND(ID_EDIT_DELALL, &CKeyPage::OnEditDelall)
	ON_COMMAND(ID_EDIT_COPY, &CKeyPage::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &CKeyPage::OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CKeyPage::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CKeyPage::OnUpdateEditPaste)
	ON_BN_CLICKED(IDC_KEYASNNEW, OnKeyAsnNew)
	ON_BN_CLICKED(IDC_KEYASNEDIT, OnKeyAsnEdit)
	ON_BN_CLICKED(IDC_KEYASNDEL, OnKeyAsnDel)
	ON_BN_CLICKED(IDC_ALL_SET, &CKeyPage::OnBnClickedAllSet)
	ON_BN_CLICKED(IDC_ALL_CLR, &CKeyPage::OnBnClickedAllClr)
	ON_NOTIFY(NM_DBLCLK, IDC_KEYLIST, OnDblclkKeyList)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_METAKEY1, IDC_METAKEY1 + KEYTABMAX - 1, &CKeyPage::OnBnClickedMetakey)
	ON_WM_ACTIVATE()
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
static const LPCWSTR VKeyName[2][KEYTABMAX] = {
	// VK_106JP
	{	L"1",		L"2",		L"3",		L"4",		L"5",
		L"6",		L"7",		L"8",		L"9",		L"0",
		L"-",		L"^",		L"\uFFE5",	L"BS",		L"TAB",		// _T("￥"),
		L"Q",		L"W",		L"E",		L"R",		L"T",
		L"Y",		L"U",		L"I",		L"O",		L"P",
		L"@",		L"[",		L"A",		L"S",		L"D",
		L"F",		L"G",		L"H",		L"J",		L"K",
		L"L",		L";",		L":",		L"]",		L"\u2190",	// _T("←"),
		L"Z",		L"X",		L"C",		L"V",		L"B",
		L"N",		L"M",		L",",		L".",		L"/",
		L"\uFF3C",	L"SP",		L"",		L"",		L"",		// _T("＼"),
		L"",		},
	// VK_101US
	{	L"1",		L"2",		L"3",		L"4",		L"5",
		L"6",		L"7",		L"8",		L"9",		L"0",
		L"-",		L"=",		L"",		L"",		L"TAB",
		L"Q",		L"W",		L"E",		L"R",		L"T",
		L"Y",		L"U",		L"I",		L"O",		L"P",
		L"[",		L"]",		L"A",		L"S",		L"D",
		L"F",		L"G",		L"H",		L"J",		L"K",
		L"L",		L";",		L"'",		L"",		L"",
		L"Z",		L"X",		L"C",		L"V",		L"B",
		L"N",		L"M",		L",",		L".",		L"/",
		L"",		L"SP",		L"`",		L"\uFF3C",	L"BS",		// _T("＼"),
		L"\u2190",	},												// _T("←"),
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
		if ( VKeyTab[m_KeyLayout][n] == VK_NON ) {
			pWnd->ShowWindow(SW_HIDE);
			m_HideKeys.Add(IDC_METAKEY1 + n);
		} else {
			pWnd->SetWindowText(UniToTstr(VKeyName[m_KeyLayout][n]));
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
	m_List.SetPopUpMenu(IDR_POPUPMENU, 11);

	DoInit();

	DefItemOffset();

	int cx, cy;
	CRect rect;
	CWnd *pWnd;
	WINDOWPLACEMENT place;

	GetClientRect(rect);
	cx = rect.Width();
	cy = rect.Height();

	for ( int n = 0 ; n < m_InitDlgRect.GetSize() ; n++ ) {
		if ( (pWnd = CWnd::FromHandle(m_InitDlgRect[n].hWnd)) == NULL || !pWnd->GetWindowPlacement(&place) )
			continue;

		if ( m_InitDlgRect[n].id == IDC_KEYLIST || m_InitDlgRect[n].id == IDC_TITLE1 ) {
			m_InitDlgRect[n].mode &= 00077;
			m_InitDlgRect[n].mode |= (ITM_TOP_TOP | ITM_BTM_BTM);
			m_InitDlgRect[n].rect.top    = place.rcNormalPosition.top;
			m_InitDlgRect[n].rect.bottom = place.rcNormalPosition.bottom - cy;
		} else {
			m_InitDlgRect[n].mode &= 00077;
			m_InitDlgRect[n].mode |= (ITM_TOP_BTM | ITM_BTM_BTM);
			m_InitDlgRect[n].rect.top    = place.rcNormalPosition.top    - cy;
			m_InitDlgRect[n].rect.bottom = place.rcNormalPosition.bottom - cy;
		}
	}

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
	int n;
	CKeyNode tmp;
	CKeyParaDlg dlg;

	dlg.m_pData = &tmp;

	if ( dlg.DoModal() != IDOK )
		return;

	n = m_KeyTab.Add(tmp);

	InitList();
	m_List.SetSelectMarkItem(m_List.GetParamItem(n));

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnKeyAsnEdit() 
{
	int n, pos;
	CKeyParaDlg dlg;
	CDWordArray save;

	for ( pos = 0 ; pos < m_List.GetItemCount() ; pos++ ) {
		if ( m_List.GetItemState(pos, LVIS_SELECTED) != 0 )
			break;
	}
	if ( pos >= m_List.GetItemCount() )
		return;

	if ( (n = (int)m_List.GetItemData(pos)) < 0 )
		return;

	dlg.m_pData = &(m_KeyTab[n]);

	if ( dlg.DoModal() != IDOK )
		return;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( n != pos && m_List.GetItemState(n, LVIS_SELECTED) != 0 )
			save.Add((DWORD)m_List.GetItemData(n));
	}

	InitList();

	if ( save.GetSize() > 0 ) {
		pos = m_List.GetParamItem((int)save[0]);
		for ( n = 1 ; n < save.GetSize() ; n++ )
			m_List.SetItemState(m_List.GetParamItem((int)save[n]), LVIS_SELECTED, LVIS_SELECTED);
	}
	m_List.SetSelectMarkItem(pos);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnKeyAsnDel() 
{
	int n, i, a, pos;

	if ( (pos = m_List.GetSelectionMark()) < 0 )
		return;

	for ( n = a = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;
		if ( (i = (int)m_List.GetItemData(n)) < 0 )
			continue;
		m_KeyTab[i].m_Code = (-1);
		if ( n < pos )
			a++;
	}
	pos -= a;

	for ( i = 0 ; i < m_KeyTab.GetSize() ; i++ ) {
		if ( m_KeyTab[i].m_Code == (-1) )
			m_KeyTab.RemoveAt(i--);
	}

	InitList();
	m_List.SetSelectMarkItem(pos);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}
void CKeyPage::OnEditDups() 
{
	int n, pos;
	CKeyParaDlg dlg;
	CKeyNode tmp;

	if ( (pos = m_List.GetSelectionMark()) < 0 )
		return;

	if ( (n = (int)m_List.GetItemData(pos)) < 0 )
		return;

	tmp = m_KeyTab[n];
	dlg.m_pData = &tmp;

	if ( dlg.DoModal() != IDOK )
		return;

	pos = m_List.GetParamItem(m_KeyTab.Add(tmp));

	InitList();
	m_List.SetSelectMarkItem(pos);

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
	CInitAllDlg dlg;

	dlg.m_Title.LoadString(IDS_INITKEYCODETITLE);

	if ( dlg.DoModal() != IDOK )
		return;

	switch(dlg.m_InitType) {
	case 0:		// Init Default Entry
		m_KeyTab.Serialize(FALSE);
		break;

	case 1:		// Init Program Default
		m_KeyTab.Init();
		break;

	case 2:		// Copy Entry option
		ASSERT(dlg.m_pInitEntry != NULL);
		{
			CBuffer tmp(dlg.m_pInitEntry->m_ProBuffer.GetPtr(), dlg.m_pInitEntry->m_ProBuffer.GetSize());
			CStringArrayExt stra;
			stra.GetBuffer(tmp);	// CTextRam::Serialize(mode, buf);
			stra.GetBuffer(tmp);	// m_FontTab.Serialize(mode, buf);
			m_KeyTab.Serialize(FALSE, tmp);
		}
		break;
	}

	InitList();

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}

void CKeyPage::OnEditCopy()
{
	int n;
	CString text;

	for ( n = 0 ; n < m_KeyTab.GetSize() ; n++ ) {
		text += m_KeyTab[n].GetCode(); text += _T("\t");
		text += m_KeyTab[n].GetMask(); text += _T("\t");
		text += m_KeyTab[n].GetMaps(); text += _T("\r\n");
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(text);
}

void CKeyPage::OnEditPaste()
{
	LPCTSTR p;
	CString text, line;
	CStringArrayExt param;
	CKeyNode tmp;

	if ( !((CMainFrame *)::AfxGetMainWnd())->CopyClipboardData(text) )
		return;

	m_KeyTab.RemoveAll();

	for ( p = text ; ; p++ ) {
		if ( *p == _T('\0') || *p == _T('\n') ) {
			param.GetString(line, _T('\t'));
			if ( param.GetSize() >= 3 ) {
				tmp.SetCode(param[0]);
				tmp.SetMask(param[1]);
				tmp.SetMaps(param[2]);
				if ( tmp.m_Code > 0 && tmp.m_Code < 255 )
					m_KeyTab.Add(tmp);
			}
			line.Empty();
			if ( *p == _T('\0') )
				break;
		} else if ( *p != '\r' )
			line += *p;
	}

	InitList();

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_KEYTAB;
}

void CKeyPage::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_KeyTab.GetSize() > 0 ? TRUE : FALSE);
}

void CKeyPage::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardFormatAvailable(CF_UNICODETEXT) ? TRUE : FALSE);
}

void CKeyPage::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CTreePage::OnActivate(nState, pWndOther, bMinimized);

	for ( int n = 0 ; n < m_HideKeys.GetSize() ; n++ ) {
		CButton *pWnd = (CButton *)GetDlgItem(m_HideKeys[n]);
		if ( pWnd != NULL && pWnd->IsWindowVisible() )
			pWnd->ShowWindow(SW_HIDE);
	}
}
