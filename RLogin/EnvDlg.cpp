// EnvDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "Data.h"
#include "EnvDlg.h"
#include "InfoCapDlg.h"

// CEnvDlg ダイアログ

IMPLEMENT_DYNAMIC(CEnvDlg, CDialogExt)

CEnvDlg::CEnvDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CEnvDlg::IDD, pParent)
{
	m_ListUpdate = FALSE;
}

CEnvDlg::~CEnvDlg()
{
}

void CEnvDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ENV_LIST, m_List);
}


BEGIN_MESSAGE_MAP(CEnvDlg, CDialogExt)
	ON_BN_CLICKED(IDC_INFOCAP, &CEnvDlg::OnBnClickedInfocap)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ENV_LIST, &CEnvDlg::OnHdnItemchangedEnvList)
	ON_COMMAND(ID_EDIT_NEW, &CEnvDlg::OnEditNew)
	ON_COMMAND(ID_EDIT_UPDATE, &CEnvDlg::OnEditUpdate)
	ON_COMMAND(ID_EDIT_DELETE, &CEnvDlg::OnEditDelete)
	ON_COMMAND(ID_EDIT_DUPS, &CEnvDlg::OnEditDups)
END_MESSAGE_MAP()

static const LV_COLUMN InitListTab[3] = {
	{ LVCF_TEXT | LVCF_WIDTH, 0,  20, _T(" "),		0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 100, _T("Name"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 300, _T("Value"),	0, 0 },
};

static const INITDLGTAB ItemTab[] = {
	{ IDC_ENV_LIST,		ITM_RIGHT_RIGHT | ITM_BTM_BTM  },

	{ IDC_INFOCAP,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM  },
	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM  },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM  },

	{ 0,	0 },
};

void CEnvDlg::InitList()
{
	int n;

	m_ListUpdate = TRUE;
	m_List.DeleteAllItems();
	for ( n = 0 ; n < m_Env.GetSize() ; n++ ) {
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, _T(""), 0, 0, 0, n);
		m_List.SetItemText(n, 1, m_Env[n].m_nIndex);
		m_List.SetItemText(n, 2, m_Env[n].m_String);
		m_List.SetCheck(n, m_Env[n].m_Value);
		m_List.SetItemData(n, n);
	}

	m_List.DoSortItem();
	m_ListUpdate = FALSE;
}

// CEnvDlg メッセージ ハンドラ

BOOL CEnvDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn(_T("EnvDlg"), InitListTab, 3);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);
	m_List.m_EditSubItem = 0x06;

	InitList();

	SetSaveProfile(_T("EnvDlg"));
	AddHelpButton(_T("#ENVSET"));

	return TRUE;
}

void CEnvDlg::OnOK()
{
	CDialogExt::OnOK();
}

void CEnvDlg::OnBnClickedInfocap()
{
	int n;
	CInfoCapDlg dlg;

	if ( (n = m_Env.Find(_T("TERMCAP"))) >= 0 )
		dlg.m_TermCap = m_Env[n];

	if ( dlg.DoModal() != IDOK )
		return;

	m_Env[_T("TERM")]    = dlg.m_EntryName;
	m_Env[_T("TERMCAP")] = dlg.m_TermCap;

	InitList();
}

void CEnvDlg::OnHdnItemchangedEnvList(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n;
	LPNMLISTVIEW phdr = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	*pResult = 0;

	if ( m_ListUpdate )
		return;

	if ( (phdr->uChanged & LVIF_TEXT) != 0 ) {
		if ( (n = (int)m_List.GetItemData(phdr->iItem)) < (int)m_Env.GetSize() ) {
			switch(phdr->iSubItem) {
			case 1:
				m_Env[n].m_nIndex = m_List.GetItemText(phdr->iItem, phdr->iSubItem);
				break;
			case 2:
				m_Env[n].m_String = m_List.GetItemText(phdr->iItem, phdr->iSubItem);
				break;
			}
		}
	}

	if ( (phdr->uChanged & LVIF_STATE) != 0 ) {
		if ( (n = (int)m_List.GetItemData(phdr->iItem)) < (int)m_Env.GetSize() )
			m_Env[n].m_Value = (m_List.GetCheck(phdr->iItem) ? 1 : 0);
	}
}

void CEnvDlg::OnEditNew()
{
	int n = m_Env.GetSize();
	m_Env.m_Array.SetSize(n + 1);
	InitList();
	if ( (n = m_List.GetParamItem(n)) < 0 )
		return;
	m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
	m_List.EditItem(n, 1);
}

void CEnvDlg::OnEditUpdate()
{
	int n;
	if ( (n = m_List.GetSelectionMark()) < 0 )
		return;
	m_List.EditItem(n, 2);
}

void CEnvDlg::OnEditDelete()
{
	int n;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	m_Env.m_Array.RemoveAt(n);
	InitList();
}

void CEnvDlg::OnEditDups()
{
	int n, i;
	if ( (i = m_List.GetSelectMarkData()) < 0 )
		return;
	n = m_Env.GetSize();
	m_Env.m_Array.SetSize(n + 1);
	m_Env[n] = m_Env[i];
	InitList();
	if ( (n = m_List.GetParamItem(n)) < 0 )
		return;
	m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
	m_List.EnsureVisible(n, FALSE);
}
