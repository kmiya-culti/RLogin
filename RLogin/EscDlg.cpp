// EscDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "EscDlg.h"
#include "EscParaDlg.h"
#include "CsiParaDlg.h"

// CEscDlg ダイアログ

IMPLEMENT_DYNAMIC(CEscDlg, CDialogExt)

CEscDlg::CEscDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CEscDlg::IDD, pParent)
{
	m_pTextRam = NULL;
	m_pProcTab = NULL;

	m_DefEsc = new LPCTSTR [95];
	m_DefCsi = new LPCTSTR [5355];
	m_DefDcs = new LPCTSTR [5355];
	m_NewEsc = new CString [95];
	m_NewCsi = new CString [5355];
	m_NewDcs = new CString [5355];
}

CEscDlg::~CEscDlg()
{
	delete [] m_DefEsc;
	delete [] m_DefCsi;
	delete [] m_DefDcs;
	delete [] m_NewEsc;
	delete [] m_NewCsi;
	delete [] m_NewDcs;
}

void CEscDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ESCLIST, m_EscList);
	DDX_Control(pDX, IDC_CSILIST, m_CsiList);
	DDX_Control(pDX, IDC_DCSLIST, m_DcsList);
	DDX_Control(pDX, IDC_PARALIST, m_ParaList);
}

void CEscDlg::InitEscList()
{
	int n, i;
	CString tmp;

	m_EscList.DeleteAllItems();
	for ( n = i = 0 ; n < 95 ; n++ ) {
		if ( _tcscmp(m_NewEsc[n], _T("NOP")) == 0 && _tcscmp(m_DefEsc[n], _T("NOP")) == 0 )
			continue;
		tmp.Format(_T("%c"), n + _T(' '));
		m_EscList.InsertItem(LVIF_TEXT | LVIF_PARAM, i, tmp, 0, 0, 0, n);
		m_EscList.SetItemText(i, 1, m_NewEsc[n]);
		i++;
	}
	m_EscList.DoSortItem();
}
void CEscDlg::InitCsiList()
{
	int n, i, c;
	CString tmp;

	m_CsiList.DeleteAllItems();
	for ( n = i = 0 ; n < 5355 ; n++ ) {
		if ( _tcscmp(m_NewCsi[n], _T("NOP")) == 0 && _tcscmp(m_DefCsi[n], _T("NOP")) == 0 )
			continue;

		tmp.Format(_T("%c"), (n % 63) + _T('@'));
		if ( (c = (n / 63) % 17) > 0 )
			tmp.Insert(0, c + ' ' - 1);
		if ( (c = n / (63 * 17)) > 0 )
			tmp.Insert(0, c + '<' - 1);

		m_CsiList.InsertItem(LVIF_TEXT | LVIF_PARAM, i, tmp, 0, 0, 0, n);
		m_CsiList.SetItemText(i, 1, m_NewCsi[n]);
		i++;
	}
	m_CsiList.DoSortItem();
}
void CEscDlg::InitDcsList()
{
	int n, i, c;
	CString tmp;

	m_DcsList.DeleteAllItems();
	for ( n = i = 0 ; n < 5355 ; n++ ) {
		if ( _tcscmp(m_NewDcs[n], _T("NOP")) == 0 && _tcscmp(m_DefDcs[n], _T("NOP")) == 0 )
			continue;

		tmp.Format(_T("%c"), (n % 63) + _T('@'));
		if ( (c = (n / 63) % 17) > 0 )
			tmp.Insert(0, c + ' ' - 1);
		if ( (c = n / (63 * 17)) > 0 )
			tmp.Insert(0, c + '<' - 1);

		m_DcsList.InsertItem(LVIF_TEXT | LVIF_PARAM, i, tmp, 0, 0, 0, n);
		m_DcsList.SetItemText(i, 1, m_NewDcs[n]);
		i++;
	}
	m_DcsList.DoSortItem();
}
void CEscDlg::InitParaList()
{
	int n;
	CString tmp;
	static struct _InitParaListTab {
		int	id;
		LPCTSTR		name;
		int			memo;
	} InitTab[5] = {
		{ TERMPARA_VTLEVEL,		_T("VT Level"),			IDS_TERMPARA_VTLEVEL	},
		{ TERMPARA_FIRMVER,		_T("Firmware Id"),		IDS_TERMPARA_FIRMVER	},
		{ TERMPARA_TERMID,		_T("Terminal Id"),		IDS_TERMPARA_TERMID		},
		{ TERMPARA_UNITID,		_T("Unit Id"),			IDS_TERMPARA_UNITID		},
		{ TERMPARA_KEYBID,		_T("Keyboard Id"),		IDS_TERMPARA_KEYBID		},
	};

	m_ParaList.DeleteAllItems();
	for ( n = 0 ; n < 5 ; n++ ) {
		m_ParaList.InsertItem(LVIF_TEXT | LVIF_PARAM, 0, InitTab[n].name, 0, 0, 0, InitTab[n].id);
		tmp.Format(_T("%d"), m_TermPara[InitTab[n].id]);
		m_ParaList.SetItemText(0, 1, tmp);
		m_ParaList.SetItemText(0, 2, CStringLoad(InitTab[n].memo));
	}
	m_ParaList.DoSortItem();
}

BEGIN_MESSAGE_MAP(CEscDlg, CDialogExt)
	ON_NOTIFY(NM_DBLCLK, IDC_ESCLIST, &CEscDlg::OnNMDblclkEsclist)
	ON_NOTIFY(NM_DBLCLK, IDC_CSILIST, &CEscDlg::OnNMDblclkCsilist)
	ON_NOTIFY(NM_DBLCLK, IDC_DCSLIST, &CEscDlg::OnNMDblclkDcslist)
	ON_BN_CLICKED(IDC_RESET, &CEscDlg::OnBnClickedReset)
END_MESSAGE_MAP()


// CEscDlg メッセージ ハンドラ

static const LV_COLUMN InitListTab[] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0, 60, _T("Code"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 80, _T("Func"),  0, 0 }, 
	};
static const LV_COLUMN ParaListTab[] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0, 80, _T("Name"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 60, _T("Id"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 200, _T("Memo"),  0, 0 }, 
	};

BOOL CEscDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	int n, i, c;

	ASSERT(m_pTextRam != NULL && m_pProcTab != NULL);

	m_EscList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_EscList.InitColumn(_T("CEscList"), InitListTab, 2);
	m_CsiList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_CsiList.InitColumn(_T("CCsiList"), InitListTab, 2);
	m_DcsList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_DcsList.InitColumn(_T("CDcsList"), InitListTab, 2);

	m_ParaList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_ParaList.InitColumn(_T("CParaList"), ParaListTab, 3);
	m_ParaList.m_EditSubItem = 0x02;

	m_pTextRam->EscCsiDefName(m_DefEsc, m_DefCsi, m_DefDcs);

	for ( n = 0 ; n < 95 ; n++ )
		m_NewEsc[n] = m_DefEsc[n];

	for ( n = 0 ; n < 5355 ; n++ )
		m_NewCsi[n] = m_DefCsi[n];

	for ( n = 0 ; n < 5355 ; n++ )
		m_NewDcs[n] = m_DefDcs[n];

	for ( n = 0 ; n< m_pProcTab->GetSize() ; n++ ) {
		switch((*m_pProcTab)[n].m_Type) {
		case PROCTYPE_ESC:
			if ( (c = (*m_pProcTab)[n].m_Code - _T(' ')) < 0 && c >= 95 )
				break;
			m_NewEsc[c] = (*m_pProcTab)[n].m_Name;
			break;
		case PROCTYPE_CSI:
			if ( (c = ((*m_pProcTab)[n].m_Code & 0x7F) - _T('@')) < 0 && c >= 63 )
				break;
			if ( (i = (((*m_pProcTab)[n].m_Code >> 8) & 0x7F) - _T(' ')) >= 0 && i < 16 )
				c += (63 * (i + 1));
			if ( (i = (((*m_pProcTab)[n].m_Code >> 16) & 0x7F) - _T('<')) >= 0 && i < 4 )
				c += ((63 * 17) * (i + 1));
			m_NewCsi[c] = (*m_pProcTab)[n].m_Name;
			break;
		case PROCTYPE_DCS:
			if ( (c = ((*m_pProcTab)[n].m_Code & 0x7F) - _T('@')) < 0 && c >= 63 )
				break;
			if ( (i = (((*m_pProcTab)[n].m_Code >> 8) & 0x7F) - _T(' ')) >= 0 && i < 16 )
				c += (63 * (i + 1));
			if ( (i = (((*m_pProcTab)[n].m_Code >> 16) & 0x7F) - _T('<')) >= 0 && i < 4 )
				c += ((63 * 17) * (i + 1));
			m_NewDcs[c] = (*m_pProcTab)[n].m_Name;
			break;
		}
	}

	for ( n = 0 ; n < 5 ; n++ )
		m_TermPara[n] = m_pTextRam->m_DefTermPara[n];

	InitEscList();
	InitCsiList();
	InitDcsList();
	InitParaList();

	return TRUE;
}

void CEscDlg::OnOK()
{
	int n, i, c;
	CString tmp;

	m_pProcTab->RemoveAll();

	for ( n = 0 ; n < 95 ; n++ ) {
		if ( m_NewEsc[n].Compare(m_DefEsc[n]) == 0 )
			continue;
		m_pProcTab->Add(PROCTYPE_ESC, n + _T(' '), m_NewEsc[n]);
	}

	for ( n = 0 ; n < 5355 ; n++ ) {
		if ( m_NewCsi[n].Compare(m_DefCsi[n]) == 0 )
			continue;

		c = (n % 63) + _T('@');
		if ( (i = (n / 63) % 17) > 0 )
			c |= ((i + _T(' ') - 1) << 8);
		if ( (i = n / (63 * 17)) > 0 )
			c |= ((i + _T('<') - 1) << 16);

		m_pProcTab->Add(PROCTYPE_CSI, c, m_NewCsi[n]);
	}

	for ( n = 0 ; n < 5355 ; n++ ) {
		if ( m_NewDcs[n].Compare(m_DefDcs[n]) == 0 )
			continue;

		c = (n % 63) + _T('@');
		if ( (i = (n / 63) % 17) > 0 )
			c |= ((i + _T(' ') - 1) << 8);
		if ( (i = n / (63 * 17)) > 0 )
			c |= ((i + _T('<') - 1) << 16);

		m_pProcTab->Add(PROCTYPE_DCS, c, m_NewDcs[n]);
	}

	for ( n = 0 ; n < 4 ; n++ ) {
		i = (int)m_ParaList.GetItemData(n);
		tmp = m_ParaList.GetItemText(n, 1);
		switch(i) {
		case TERMPARA_VTLEVEL:
			m_pTextRam->m_DefTermPara[TERMPARA_VTLEVEL] = m_pTextRam->m_VtLevel = _tstoi(tmp);
			break;
		case TERMPARA_FIRMVER:
			m_pTextRam->m_DefTermPara[TERMPARA_FIRMVER] = m_pTextRam->m_FirmVer = _tstoi(tmp);
			break;
		case TERMPARA_TERMID:
			m_pTextRam->m_DefTermPara[TERMPARA_TERMID] = m_pTextRam->m_TermId = _tstoi(tmp);
			break;
		case TERMPARA_UNITID:
			m_pTextRam->m_DefTermPara[TERMPARA_UNITID] = m_pTextRam->m_UnitId = _tstoi(tmp);
			break;
		case TERMPARA_KEYBID:
			m_pTextRam->m_DefTermPara[TERMPARA_KEYBID] = m_pTextRam->m_KeybId = _tstoi(tmp);
			break;
		}
	}

	CDialogExt::OnOK();
}

void CEscDlg::OnNMDblclkEsclist(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n;
	CEscParaDlg dlg;
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	dlg.m_pTextRam = m_pTextRam;
	dlg.m_Code = (int)m_EscList.GetItemData(pNMItemActivate->iItem);
	dlg.m_Name = m_NewEsc[dlg.m_Code];

	if ( dlg.DoModal() == IDOK ) {
		m_NewEsc[dlg.m_Code] = dlg.m_Name;
		InitEscList();
		if ( (n = m_EscList.GetParamItem(dlg.m_Code)) >= 0 ) {
			m_EscList.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
			m_EscList.EnsureVisible(n, FALSE);
		}
	}

	*pResult = 0;
}

void CEscDlg::OnNMDblclkCsilist(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n;
	CCsiParaDlg dlg;
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	dlg.m_Type = PROCTYPE_CSI;
	dlg.m_pTextRam = m_pTextRam;
	dlg.m_Code = (int)m_CsiList.GetItemData(pNMItemActivate->iItem);
	dlg.m_Name = m_NewCsi[dlg.m_Code];

	if ( dlg.DoModal() == IDOK ) {
		m_NewCsi[dlg.m_Code] = dlg.m_Name;
		InitCsiList();
		if ( (n = m_CsiList.GetParamItem(dlg.m_Code)) >= 0 ) {
			m_CsiList.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
			m_CsiList.EnsureVisible(n, FALSE);
		}
	}

	*pResult = 0;
}

void CEscDlg::OnNMDblclkDcslist(NMHDR *pNMHDR, LRESULT *pResult)
{
	int n;
	CCsiParaDlg dlg;
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	dlg.m_Type = PROCTYPE_DCS;
	dlg.m_pTextRam = m_pTextRam;
	dlg.m_Code = (int)m_DcsList.GetItemData(pNMItemActivate->iItem);
	dlg.m_Name = m_NewDcs[dlg.m_Code];

	if ( dlg.DoModal() == IDOK ) {
		m_NewDcs[dlg.m_Code] = dlg.m_Name;
		InitDcsList();
		if ( (n = m_DcsList.GetParamItem(dlg.m_Code)) >= 0 ) {
			m_DcsList.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
			m_DcsList.EnsureVisible(n, FALSE);
		}
	}

	*pResult = 0;
}

void CEscDlg::OnBnClickedReset()
{
	int n;

	for ( n = 0 ; n < 95 ; n++ )
		m_NewEsc[n] = m_DefEsc[n];

	for ( n = 0 ; n < 5355 ; n++ )
		m_NewCsi[n] = m_DefCsi[n];

	for ( n = 0 ; n < 5355 ; n++ )
		m_NewDcs[n] = m_DefDcs[n];

	m_TermPara[TERMPARA_VTLEVEL] = DEFVAL_VTLEVEL;
	m_TermPara[TERMPARA_FIRMVER] = DEFVAL_FIRMVER;
	m_TermPara[TERMPARA_TERMID]  = DEFVAL_TERMID;
	m_TermPara[TERMPARA_UNITID]  = DEFVAL_UNITID;
	m_TermPara[TERMPARA_KEYBID]  = DEFVAL_KEYBID;

	InitEscList();
	InitCsiList();
	InitDcsList();
	InitParaList();
}
