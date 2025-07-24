// PfdListDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "Data.h"
#include "PfdListDlg.h"
#include "PfdParaDlg.h"
#include "InitAllDlg.h"

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
	m_TimeOut = 0;
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
	m_TimeOut     = (stra.GetSize() > 6 ? stra.GetVal(6) : 0);

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
	stra.AddVal(m_TimeOut);

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
	m_TimeOut     = data.m_TimeOut;

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
	ON_BN_CLICKED(IDC_PFDNEW, &CPfdListDlg::OnPfdNew)
	ON_BN_CLICKED(IDC_PFDEDIT, &CPfdListDlg::OnPfdEdit)
	ON_BN_CLICKED(IDC_PFDDEL, &CPfdListDlg::OnPfdDel)
	ON_COMMAND(ID_EDIT_NEW, &CPfdListDlg::OnPfdNew)
	ON_COMMAND(ID_EDIT_UPDATE, &CPfdListDlg::OnPfdEdit)
	ON_COMMAND(ID_EDIT_DELETE, &CPfdListDlg::OnPfdDel)
	ON_COMMAND(ID_EDIT_DUPS, &CPfdListDlg::OnEditDups)
	ON_COMMAND(ID_EDIT_DELALL, &CPfdListDlg::OnEditDelall)
	ON_COMMAND(ID_EDIT_COPY, &CPfdListDlg::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &CPfdListDlg::OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CPfdListDlg::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CPfdListDlg::OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, &CPfdListDlg::OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, &CPfdListDlg::OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, &CPfdListDlg::OnUpdateEditEntry)
	ON_NOTIFY(NM_DBLCLK, IDC_PFDLIST, &CPfdListDlg::OnDblclkPfdlist)
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

static const INITDLGTAB ItemTab[] = {
	{ IDC_PFDLIST,		ITM_RIGHT_RIGHT | ITM_BTM_BTM },

	{ IDC_PFDNEW,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_PFDEDIT,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_PFDDEL,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_CHECK1,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_CHECK2,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_EDIT1,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_EDIT2,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_EDIT3,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_TITLE1,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE2,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE3,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE4,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ 0,	0 },
};

BOOL CPfdListDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	CPfdData data;

	InitItemOffset(ItemTab);

	for ( int n = 0 ; n < m_PortFwd.GetSize() ; n++ ) {
		if ( data.GetString(m_PortFwd[n]) )
			m_Data.Add(data);
	}

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn(_T("PfdList"), InitListTab, 5);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 11);
	InitList();

	UpdateData(FALSE);

	SetSaveProfile(_T("PfdListDlg"));
	AddHelpButton(_T("#PORTFORWARD"));

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
	int n;
	CPfdParaDlg dlg;

	UpdateCheck();
	dlg.m_pEntry = m_pEntry;

	if ( dlg.DoModal() != IDOK )
		return;

	n = (int)m_Data.Add(dlg.m_Data);
	InitList();
	m_List.SetSelectMarkItem(m_List.GetParamItem(n));
}

void CPfdListDlg::OnPfdEdit() 
{
	int n, pos;
	CPfdParaDlg dlg;
	CDWordArray save;

	for ( pos = 0 ; pos < m_List.GetItemCount() ; pos++ ) {
		if ( m_List.GetItemState(pos, LVIS_SELECTED) != 0 )
			break;
	}
	if ( pos >= m_List.GetItemCount() )
		return;
		
	if ( (n = (int)m_List.GetItemData(pos)) < 0 )
		return;

	UpdateCheck();

	dlg.m_Data = m_Data[n];
	dlg.m_pEntry = m_pEntry;

	if ( dlg.DoModal() != IDOK )
		return;

	m_Data[n] = dlg.m_Data;

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
}

void CPfdListDlg::OnPfdDel() 
{
	int n, i, a, pos;

	if ( (pos = m_List.GetSelectionMark()) < 0 )
		return;

	for ( n = a = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;
		if ( (i = (int)m_List.GetItemData(n)) < 0 )
			continue;
		m_Data[i].m_ListenType = (-1);
		if ( n < pos )
			a++;
	}
	pos -= a;
	
	UpdateCheck();

	for ( i = 0 ; i < m_Data.GetSize() ; i++ ) {
		if ( m_Data[i].m_ListenType == (-1) )
			m_Data.RemoveAt(i--);
	}

	InitList();
	m_List.SetSelectMarkItem(pos);
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

	idx = (int)m_Data.Add(dlg.m_Data);
	InitList();
	m_List.SetSelectMarkItem(m_List.GetParamItem(idx));
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

void CPfdListDlg::OnEditDelall()
{
	CPfdData data;
	CParamTab ParamTab;
	CInitAllDlg dlg;

	dlg.m_Title.LoadString(IDS_INITPFDTITLE);

	if ( dlg.DoModal() != IDOK )
		return;

	switch(dlg.m_InitType) {
	case 0:		// Init Default Entry
		ParamTab.Serialize(FALSE);
		break;

	case 1:		// Init Program Default
		ParamTab.Init();
		break;

	case 2:		// Copy Entry option
		ASSERT(dlg.m_pInitEntry != NULL);
		{
			CBuffer tmp(dlg.m_pInitEntry->m_ProBuffer.GetPtr(), dlg.m_pInitEntry->m_ProBuffer.GetSize());
			CStringArrayExt stra;
			stra.GetBuffer(tmp);	// CTextRam::Serialize(mode, buf);
			stra.GetBuffer(tmp);	// m_FontTab.Serialize(mode, buf);
			stra.GetBuffer(tmp);	// KeyTab.Serialize(FALSE, tmp);
			stra.GetBuffer(tmp);	// KeyMac.Serialize(FALSE, tmp);
			ParamTab.Serialize(FALSE, tmp);
		}
		break;
	}

	m_Data.RemoveAll();

	for ( int n = 0 ; n < ParamTab.m_PortFwd.GetSize() ; n++ ) {
		if ( data.GetString(ParamTab.m_PortFwd[n]) )
			m_Data.Add(data);
	}

	InitList();
}

void CPfdListDlg::OnEditCopy()
{
	int n;
	CString text, tmp;

	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		text += (m_Data[n].m_EnableFlag ? _T("Enable") : _T("Disable")); text += _T("\t");
		text += ListenTypeName[m_Data[n].m_ListenType]; text += _T("\t");
		text += m_Data[n].m_ListenHost; text += _T("\t");
		text += m_Data[n].m_ListenPort; text += _T("\t");
		text += m_Data[n].m_ConnectHost; text += _T("\t");
		text += m_Data[n].m_ConnectPort; text += _T("\t");
		tmp.Format(_T("%d"), m_Data[n].m_TimeOut);
		text += tmp; text += _T("\r\n");
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(text);
}

void CPfdListDlg::OnEditPaste()
{
	LPCTSTR p;
	CString text, line;
	CStringArrayExt param;
	CPfdData tmp;

	if ( !((CMainFrame *)::AfxGetMainWnd())->CopyClipboardData(text) )
		return;

	m_Data.RemoveAll();

	for ( p = text ; ; p++ ) {
		if ( *p == _T('\0') || *p == _T('\n') ) {
			param.GetString(line, _T('\t'));
			if ( param.GetSize() >= 6 ) {
				tmp.m_EnableFlag = (param[0].CompareNoCase(_T("Enable")) == 0 ? TRUE : FALSE);
				if ( param[1].CompareNoCase(ListenTypeName[0]) == 0 ) tmp.m_ListenType = 0;
				else if ( param[1].CompareNoCase(ListenTypeName[1]) == 0 ) tmp.m_ListenType = 1;
				else if ( param[1].CompareNoCase(ListenTypeName[2]) == 0 ) tmp.m_ListenType = 2;
				else if ( param[1].CompareNoCase(ListenTypeName[3]) == 0 ) tmp.m_ListenType = 3;
				tmp.m_ListenHost = param[2];
				tmp.m_ListenPort = param[3];
				tmp.m_ConnectHost = param[4];
				tmp.m_ConnectPort = param[5];
				tmp.m_TimeOut = (param.GetSize() > 6 ? _tstoi(param[6]) : 0);
				m_Data.Add(tmp);
			}
			line.Empty();
			if ( *p == _T('\0') )
				break;
		} else if ( *p != '\r' )
			line += *p;
	}

	InitList();
}

void CPfdListDlg::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_Data.GetSize() > 0 ? TRUE : FALSE);
}

void CPfdListDlg::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardFormatAvailable(CF_UNICODETEXT) ? TRUE : FALSE);
}
