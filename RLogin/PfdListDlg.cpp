// PfdListDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "Data.h"
#include "PfdListDlg.h"
#include "PfdParaDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg ダイアログ

IMPLEMENT_DYNAMIC(CPfdListDlg, CDialog)

static LPCTSTR	ListenTypeName[] = { _T("local"), _T("socks"), _T("remote") };

CPfdListDlg::CPfdListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPfdListDlg::IDD, pParent)
{
	m_pEntry = NULL;
	m_X11PortFlag = FALSE;
	m_XDisplay.Empty();
}

void CPfdListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PFDLIST, m_List);
	DDX_Text(pDX, IDC_EDIT1, m_XDisplay);
	DDX_Check(pDX, IDC_CHECK1, m_X11PortFlag);
}

BEGIN_MESSAGE_MAP(CPfdListDlg, CDialog)
	ON_BN_CLICKED(IDC_PFDNEW, OnPfdNew)
	ON_BN_CLICKED(IDC_PFDEDIT, OnPfdEdit)
	ON_BN_CLICKED(IDC_PFDDEL, OnPfdDel)
	ON_NOTIFY(NM_DBLCLK, IDC_PFDLIST, OnDblclkPfdlist)
	ON_COMMAND(ID_EDIT_NEW, OnPfdNew)
	ON_COMMAND(ID_EDIT_UPDATE, OnPfdEdit)
	ON_COMMAND(ID_EDIT_DELETE, OnPfdDel)
	ON_COMMAND(ID_EDIT_DUPS, OnEditDups)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
END_MESSAGE_MAP()

void CPfdListDlg::InitList()
{
	int n, i;
	CStringArrayExt stra;

	m_List.DeleteAllItems();

	for ( n = 0 ; n < m_PortFwd.GetSize() ; n++ ) {
		stra.GetString(m_PortFwd[n]);
		if ( stra.GetSize() < 5 ) {
			m_PortFwd.RemoveAt(n);
			n--;
			continue;
		}
		i = stra.GetVal(4);
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, ListenTypeName[i], 0, 0, 0, n);
		m_List.SetItemText(n, 1, stra[0]);
		m_List.SetItemText(n, 2, stra[1]);
		m_List.SetItemText(n, 3, stra[2]);
		m_List.SetItemText(n, 4, stra[3]);
	}
	m_List.DoSortItem();
}

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg メッセージ ハンドラ

static const LV_COLUMN InitListTab[5] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60, _T("Type"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, _T("Listened Host"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60, _T("Port"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, _T("Connect Host"),   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60, _T("Port"),   0, 0 }, 
	};

BOOL CPfdListDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn(_T("PfdList"), InitListTab, 5);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);
	InitList();

	return TRUE;
}

void CPfdListDlg::OnOK() 
{
	m_List.SaveColumn(_T("PfdList"));

	CDialog::OnOK();
}

void CPfdListDlg::OnPfdNew() 
{
	CPfdParaDlg dlg;

	dlg.m_pData = &m_PortFwd;
	dlg.m_pEntry = m_pEntry;

	dlg.DoModal();

	InitList();
}

void CPfdListDlg::OnPfdEdit() 
{
	CPfdParaDlg dlg;

	dlg.m_pData = &m_PortFwd;
	dlg.m_pEntry = m_pEntry;

	if ( (dlg.m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	dlg.DoModal();

	InitList();
}

void CPfdListDlg::OnPfdDel() 
{
	int n;

	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;

	m_PortFwd.RemoveAt(n);

	InitList();
}

void CPfdListDlg::OnDblclkPfdlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnPfdEdit();
	*pResult = 0;
}

void CPfdListDlg::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CPfdListDlg::OnEditDups() 
{
	int n;
	CStringArrayExt stra;
	CPfdParaDlg dlg;

	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;

	stra.GetString(m_PortFwd[n]);
	if ( stra.GetSize() >= 5 ) {
		dlg.m_ListenHost  = stra[0];
		dlg.m_ListenPort  = stra[1];
		dlg.m_ConnectHost = stra[2];
		dlg.m_ConnectPort = stra[3];
		dlg.m_ListenType  = stra.GetVal(4);
	}

	dlg.m_pData = &m_PortFwd;
	dlg.m_pEntry = m_pEntry;

	dlg.DoModal();

	InitList();
}
