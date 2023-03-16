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

CPfdData::CPfdData()
{
	m_ListenHost.Empty();
	m_ListenPort.Empty();
	m_ConnectHost.Empty();
	m_ConnectPort.Empty();
	m_ListenType = 0;
	m_EnableFlag = TRUE;
}
CPfdData::~CPfdData()
{
}
BOOL CPfdData::GetString(LPCTSTR str)
{
	CStringArrayExt stra;

	stra.GetString(str);

	if ( stra.GetSize() < 5 )
		return FALSE;

	m_ListenHost  = stra[0];
	m_ListenPort  = stra[1];
	m_ConnectHost = stra[2];
	m_ConnectPort = stra[3];
	m_ListenType  = stra.GetVal(4);
	m_EnableFlag  = (stra.GetSize() > 5 ? stra.GetVal(5) : TRUE);

	return TRUE;
}
void CPfdData::SetString(CString &str)
{
	CStringArrayExt stra;

	stra.Add(m_ListenHost);
	stra.Add(m_ListenPort);
	stra.Add(m_ConnectHost);
	stra.Add(m_ConnectPort);
	stra.AddVal(m_ListenType);
	stra.AddVal(m_EnableFlag);

	stra.SetString(str);
}
const CPfdData & CPfdData::operator = (CPfdData &data)
{
	m_ListenHost  = data.m_ListenHost;
	m_ListenPort  = data.m_ListenPort;
	m_ConnectHost = data.m_ConnectHost;
	m_ConnectPort = data.m_ConnectPort;
	m_ListenType  = data.m_ListenType;
	m_EnableFlag  = data.m_EnableFlag;

	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg ダイアログ

IMPLEMENT_DYNAMIC(CPfdListDlg, CDialogExt)

static LPCTSTR	ListenTypeName[] = { _T("local"), _T("Lproxy"), _T("remote"), _T("Rproxy") };

CPfdListDlg::CPfdListDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CPfdListDlg::IDD, pParent)
{
	m_pEntry = NULL;
	m_X11PortFlag = FALSE;
	m_x11AuthFlag = FALSE;
	m_XDisplay.Empty();
}

void CPfdListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PFDLIST, m_List);
	DDX_Text(pDX, IDC_EDIT1, m_XDisplay);
	DDX_Text(pDX, IDC_EDIT2, m_x11AuthName);
	DDX_Text(pDX, IDC_EDIT3, m_x11AuthData);
	DDX_Check(pDX, IDC_CHECK1, m_X11PortFlag);
	DDX_Check(pDX, IDC_CHECK2, m_x11AuthFlag);
}

BEGIN_MESSAGE_MAP(CPfdListDlg, CDialogExt)
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
	int n;

	m_List.DeleteAllItems();

	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, ListenTypeName[m_Data[n].m_ListenType], 0, 0, 0, n);
		m_List.SetItemText(n, 1, m_Data[n].m_ListenHost);
		m_List.SetItemText(n, 2, m_Data[n].m_ListenPort);
		if ( (m_Data[n].m_ListenType & 1) == 0 ) {
			m_List.SetItemText(n, 3, m_Data[n].m_ConnectHost);
			m_List.SetItemText(n, 4, m_Data[n].m_ConnectPort);
		} else {
			m_List.SetItemText(n, 3, _T(""));
			m_List.SetItemText(n, 4, _T(""));
		}
		m_List.SetLVCheck(n, m_Data[n].m_EnableFlag);
		m_List.SetItemData(n, n);
	}

	m_List.DoSortItem();
}
void CPfdListDlg::UpdateCheck()
{
	int n, i;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( (i = (int)m_List.GetItemData(n)) < 0 || i >= m_Data.GetSize() )
			continue;
		m_Data[i].m_EnableFlag = m_List.GetLVCheck(n);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg メッセージ ハンドラ

static const LV_COLUMN InitListTab[5] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  70, _T("Type"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, _T("Listened Host"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Port"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, _T("Connect Host"),   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Port"),   0, 0 }, 
	};

BOOL CPfdListDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	CPfdData data;

	for ( int n = 0 ; n < m_PortFwd.GetSize() ; n++ ) {
		if ( data.GetString(m_PortFwd[n]) )
			m_Data.Add(data);
	}

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn(_T("PfdList"), InitListTab, 5);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);
	InitList();

	UpdateData(FALSE);

	return TRUE;
}

void CPfdListDlg::OnOK() 
{
	int n, i;
	CString str;

	UpdateData(TRUE);

	m_PortFwd.RemoveAll();

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( (i = (int)m_List.GetItemData(n)) < 0 || i >= m_Data.GetSize() )
			continue;
		m_Data[i].m_EnableFlag = m_List.GetLVCheck(n);
		m_Data[i].SetString(str);
		m_PortFwd.Add(str);
	}

	m_List.SaveColumn(_T("PfdList"));

	CDialogExt::OnOK();
}

void CPfdListDlg::OnPfdNew() 
{
	CPfdParaDlg dlg;

	UpdateCheck();
	dlg.m_pEntry = m_pEntry;

	if ( dlg.DoModal() != IDOK )
		return;

	m_Data.Add(dlg.m_Data);
	InitList();
}

void CPfdListDlg::OnPfdEdit() 
{
	int idx;
	CPfdParaDlg dlg;

	if ( (idx = m_List.GetSelectMarkData()) < 0 || idx >= m_Data.GetSize() )
		return;

	UpdateCheck();

	dlg.m_Data = m_Data[idx];
	dlg.m_pEntry = m_pEntry;

	if ( dlg.DoModal() != IDOK )
		return;

	m_Data[idx] = dlg.m_Data;
	InitList();
}

void CPfdListDlg::OnPfdDel() 
{
	int idx;

	if ( (idx = m_List.GetSelectMarkData()) < 0 || idx >= m_Data.GetSize() )
		return;

	UpdateCheck();
	m_Data.RemoveAt(idx);
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
	int idx;
	CPfdParaDlg dlg;

	if ( (idx = m_List.GetSelectMarkData()) < 0 || idx >= m_Data.GetSize() )
		return;

	UpdateCheck();

	dlg.m_Data = m_Data[idx];
	dlg.m_pEntry = m_pEntry;

	if ( dlg.DoModal() != IDOK )
		return;

	m_Data.Add(dlg.m_Data);
	InitList();
}
