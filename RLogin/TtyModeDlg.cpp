// TtyModeDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "Data.h"
#include "TextRam.h"
#include "TtyModeDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CTtyModeDlg ダイアログ

IMPLEMENT_DYNAMIC(CTtyModeDlg, CDialogExt)

CTtyModeDlg::CTtyModeDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CTtyModeDlg::IDD, pParent)
{
	m_pParamTab = NULL;
}

CTtyModeDlg::~CTtyModeDlg()
{
}

void CTtyModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MODE_LIST, m_List);
}


BEGIN_MESSAGE_MAP(CTtyModeDlg, CDialogExt)
END_MESSAGE_MAP()

// CTtyModeDlg メッセージ ハンドラー

static const LV_COLUMN InitListTab[4] = {
	{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("No."),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  80, _T("Name"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  80, _T("Value"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 200, _T("Memo"),	0, 0 },
};

static const struct _ttymode_name {
	BYTE		opcode;
	LPCTSTR		name;
	int			memo;
} ttymode_name[] = {
		{	1,		_T("VINTR"),	IDS_TTYMODE_LIST01		},
		{	2,		_T("VQUIT"),	IDS_TTYMODE_LIST02		},
		{	3,		_T("VERASE"),	IDS_TTYMODE_LIST03		},
		{	4,		_T("VKILL"),	IDS_TTYMODE_LIST04		},
		{	5,		_T("VEOF"),		IDS_TTYMODE_LIST05		},
		{	6,		_T("VEOL"),		IDS_TTYMODE_LIST06		},
		{	7,		_T("VEOL2"),	IDS_TTYMODE_LIST07		},
		{	8,		_T("VSTART"),	IDS_TTYMODE_LIST08		},
		{	9,		_T("VSTOP"),	IDS_TTYMODE_LIST09		},
		{	10,		_T("VSUSP"),	IDS_TTYMODE_LIST10		},
		{	11,		_T("VDSUSP"),	IDS_TTYMODE_LIST11		},
		{	12,		_T("VREPRINT"),	IDS_TTYMODE_LIST12		},
		{	13,		_T("VWERASE"),	IDS_TTYMODE_LIST13		},
		{	14,		_T("VLNEXT"),	IDS_TTYMODE_LIST14		},
		{	15,		_T("VFLUSH"),	IDS_TTYMODE_LIST15		},
		{	16,		_T("VSWTCH"),	IDS_TTYMODE_LIST16		},
		{	17,		_T("VSTATUS"),	IDS_TTYMODE_LIST17		},
		{	18,		_T("VDISCARD"),	IDS_TTYMODE_LIST18		},
		{	30,		_T("IGNPAR"),	IDS_TTYMODE_LIST19		},
		{	31,		_T("PARMRK"),	IDS_TTYMODE_LIST20		},
		{	32,		_T("INPCK"),	IDS_TTYMODE_LIST21		},
		{	33,		_T("ISTRIP"),	IDS_TTYMODE_LIST22		},
		{	34,		_T("INLCR"),	IDS_TTYMODE_LIST23		},
		{	35,		_T("IGNCR"),	IDS_TTYMODE_LIST24		},
		{	36,		_T("ICRNL"),	IDS_TTYMODE_LIST25		},
		{	37,		_T("IUCLC"),	IDS_TTYMODE_LIST26		},
		{	38,		_T("IXON"),		IDS_TTYMODE_LIST27		},
		{	39,		_T("IXANY"),	IDS_TTYMODE_LIST28		},
		{	40,		_T("IXOFF"),	IDS_TTYMODE_LIST29		},
		{	41,		_T("IMAXBEL"),	IDS_TTYMODE_LIST30		},
		{	42,		_T("IUTF8"),	IDS_TTYMODE_LIST56		},		// RFC 8160
		{	50,		_T("ISIG"),		IDS_TTYMODE_LIST31		},
		{	51,		_T("ICANON"),	IDS_TTYMODE_LIST32		},
		{	52,		_T("XCASE"),	IDS_TTYMODE_LIST33		},
		{	53,		_T("ECHO"),		IDS_TTYMODE_LIST34		},
		{	54,		_T("ECHOE"),	IDS_TTYMODE_LIST35		},
		{	55,		_T("ECHOK"),	IDS_TTYMODE_LIST36		},
		{	56,		_T("ECHONL"),	IDS_TTYMODE_LIST37		},
		{	57,		_T("NOFLSH"),	IDS_TTYMODE_LIST38		},
		{	58,		_T("TOSTOP"),	IDS_TTYMODE_LIST39		},
		{	59,		_T("IEXTEN"),	IDS_TTYMODE_LIST40		},
		{	60,		_T("ECHOCTL"),	IDS_TTYMODE_LIST41		},
		{	61,		_T("ECHOKE"),	IDS_TTYMODE_LIST42		},
		{	62,		_T("PENDIN"),	IDS_TTYMODE_LIST43		},
		{	70,		_T("OPOST"),	IDS_TTYMODE_LIST44		},
		{	71,		_T("OLCUC"),	IDS_TTYMODE_LIST45		},
		{	72,		_T("ONLCR"),	IDS_TTYMODE_LIST46		},
		{	73,		_T("OCRNL"),	IDS_TTYMODE_LIST47		},
		{	74,		_T("ONOCR"),	IDS_TTYMODE_LIST48		},
		{	75,		_T("ONLRET"),	IDS_TTYMODE_LIST49		},
		{	90,		_T("CS7"),		IDS_TTYMODE_LIST50		},
		{	91,		_T("CS8"),		IDS_TTYMODE_LIST51		},
		{	92,		_T("PARENB"),	IDS_TTYMODE_LIST52		},
		{	93,		_T("PARODD"),	IDS_TTYMODE_LIST53		},
		{	128,	_T("ISPEED"),	IDS_TTYMODE_LIST54		},
		{	129,	_T("OSPEED"),	IDS_TTYMODE_LIST55		},
		{	0,		NULL,			0						}
	};

void CTtyModeDlg::InitList()
{
	int n, i;
	CString str;

	m_List.DeleteAllItems();

	for ( n = 0 ; ttymode_name[n].opcode != 0 ; n++ ) {
		str.Format(_T("%d"), ttymode_name[n].opcode);
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_List.SetItemText(n, 1, ttymode_name[n].name);
		m_List.SetItemText(n, 3, CStringLoad(ttymode_name[n].memo));
		m_List.SetItemData(n, n);

		for ( i = 0 ; i < m_pParamTab->m_TtyMode.GetSize() ; i++ ) {
			if ( ttymode_name[n].opcode == m_pParamTab->m_TtyMode[i].opcode ) {
				if ( m_pParamTab->m_TtyMode[i].opcode < 30 && m_pParamTab->m_TtyMode[i].param < 32 )
					str.Format(_T("^%c"), _T('@') + m_pParamTab->m_TtyMode[i].param);
				else
					str.Format(_T("%d"), m_pParamTab->m_TtyMode[i].param);
				m_List.SetItemText(n, 2, str);
			}
		}
	}
}

static const INITDLGTAB ItemTab[] = {
	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_MODE_LIST,	ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ 0,	0 },
};

BOOL CTtyModeDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_List.InitColumn(_T("TtyModeDlg"), InitListTab, 4);
	m_List.m_EditSubItem = 0x04;

	InitList();

	return TRUE;
}

void CTtyModeDlg::OnOK()
{
	int n, i;
	CString str;
	LPCTSTR p;
	ttymode_node node;

	m_pParamTab->m_TtyMode.RemoveAll();

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		i = (int)m_List.GetItemData(n);
		str = m_List.GetItemText(n, 2);

		p = str;
		node.opcode = ttymode_name[i].opcode;

		if ( p[0] == _T('^') && p[1] >= _T('@') && p[1] <= _T('_') )
			node.param  = p[1] - _T('@');
		else if ( *p != _T('\0') )
			node.param  = _tstoi(p);
		else
			node.opcode = 0;

		if ( node.opcode != 0 )
			m_pParamTab->m_TtyMode.Add(node);
	}

	CDialogExt::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CColEditDlg

IMPLEMENT_DYNAMIC(CColEditDlg, CTtyModeDlg)

CColEditDlg::CColEditDlg()
{
	memset(m_ColTab, 0, sizeof(m_ColTab));
}
CColEditDlg::~CColEditDlg()
{
}

BEGIN_MESSAGE_MAP(CColEditDlg, CTtyModeDlg)
	ON_COMMAND(ID_EDIT_COPY_ALL, &CColEditDlg::OnEditCopyAll)
	ON_COMMAND(ID_EDIT_PASTE_ALL, &CColEditDlg::OnEditPasteAll)
END_MESSAGE_MAP()

static const LV_COLUMN InitColListTab[8] = {
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  40, _T("No."),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  60, _T("Red"),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  60, _T("Green"),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  60, _T("Blue"),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  40, _T("No."),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  60, _T("Red"),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  60, _T("Green"),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  60, _T("Blue"),	0, 0 },
};

void CColEditDlg::InitList()
{
	int n;
	CString str;

	m_List.DeleteAllItems();

	for ( n = 0 ; n < 8 ; n++ ) {
		str.Format(_T("%d"), n);
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		str.Format(_T("%d"), GetRValue(m_ColTab[n]));
		m_List.SetItemText(n, 1, str);
		str.Format(_T("%d"), GetGValue(m_ColTab[n]));
		m_List.SetItemText(n, 2, str);
		str.Format(_T("%d"), GetBValue(m_ColTab[n]));
		m_List.SetItemText(n, 3, str);

		str.Format(_T("%d"), n + 8);
		m_List.SetItemText(n, 4, str);
		str.Format(_T("%d"), GetRValue(m_ColTab[n + 8]));
		m_List.SetItemText(n, 5, str);
		str.Format(_T("%d"), GetGValue(m_ColTab[n + 8]));
		m_List.SetItemText(n, 6, str);
		str.Format(_T("%d"), GetBValue(m_ColTab[n + 8]));
		m_List.SetItemText(n, 7, str);

		m_List.SetItemData(n, n);
	}
}

BOOL CColEditDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

//	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_List.InitColumn(_T("ColEditDlg"), InitColListTab, 8);
	m_List.m_EditSubItem = 0xEE;
	m_List.SetPopUpMenu(IDR_POPUPMENU, 4);

	InitList();

	SetWindowText(_T("Color Table Edit"));

	return TRUE;
}

void CColEditDlg::OnOK()
{
	int n, i;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		i = (int)m_List.GetItemData(n);
		m_ColTab[i]     = RGB(_tstoi(m_List.GetItemText(n, 1)), _tstoi(m_List.GetItemText(n, 2)), _tstoi(m_List.GetItemText(n, 3)));
		m_ColTab[i + 8] = RGB(_tstoi(m_List.GetItemText(n, 5)), _tstoi(m_List.GetItemText(n, 6)), _tstoi(m_List.GetItemText(n, 7)));
	}

	CDialogExt::OnOK();
}


void CColEditDlg::OnEditCopyAll()
{
	int n, i;
	CString tmp, str;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		i = (int)m_List.GetItemData(n);
		m_ColTab[i]     = RGB(_tstoi(m_List.GetItemText(n, 1)), _tstoi(m_List.GetItemText(n, 2)), _tstoi(m_List.GetItemText(n, 3)));
		m_ColTab[i + 8] = RGB(_tstoi(m_List.GetItemText(n, 5)), _tstoi(m_List.GetItemText(n, 6)), _tstoi(m_List.GetItemText(n, 7)));
	}

	for ( n = 0 ; n < 16 ; n++ ) {
		str.Format(_T("%d\t"), GetRValue(m_ColTab[n]));
		tmp += str;
		str.Format(_T("%d\t"), GetGValue(m_ColTab[n]));
		tmp += str;
		str.Format(_T("%d\r\n"), GetBValue(m_ColTab[n]));
		tmp += str;
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(tmp);
}

void CColEditDlg::OnEditPasteAll()
{
	int n;
	LPCTSTR p;
	CString str, tmp;
	CStringArrayExt line, pam;

	if ( !((CMainFrame *)::AfxGetMainWnd())->CopyClipboardData(str) )
		return;

	for ( p = str ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('\n') ) {
			if ( !tmp.IsEmpty() )
				line.Add(tmp);
			tmp.Empty();
		} else if ( *p != _T('\r') ) {
			tmp += *p;
		}
	}
	if ( !tmp.IsEmpty() )
		line.Add(tmp);

	for ( n = 0 ; n < 16 && n < line.GetSize() ; n++ ) {
		pam.GetString(line[n]);
		if ( pam.GetSize() >= 3 )
			m_ColTab[n] = RGB(pam.GetVal(0), pam.GetVal(1), pam.GetVal(2));
	}

	InitList();
}

/////////////////////////////////////////////////////////////////////////////
// CKnownHostsDlg

IMPLEMENT_DYNAMIC(CKnownHostsDlg, CTtyModeDlg)

CKnownHostsDlg::CKnownHostsDlg()
{
}
CKnownHostsDlg::~CKnownHostsDlg()
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ )
		delete m_Data[n];
}

BEGIN_MESSAGE_MAP(CKnownHostsDlg, CTtyModeDlg)
END_MESSAGE_MAP()

static const LV_COLUMN InitKnownHostsTab[3] = {
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, 0,  160, _T("Host"),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, 0,   40, _T("Port"),	0, 0 },
	{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, 0,  240, _T("Key"),	0, 0 },
};

void CKnownHostsDlg::InitList()
{
	int n;
	KNOWNHOSTDATA *pData;

	m_List.DeleteAllItems();

	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		pData = (KNOWNHOSTDATA *)m_Data.GetAt(n);
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, pData->host, 0, 0, 0, n);
		m_List.SetItemText(n, 1, pData->port);
		m_List.SetItemText(n, 2, pData->digest);
		m_List.SetLVCheck(n, FALSE);

		m_List.SetItemData(n, n);
	}
}

BOOL CKnownHostsDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn(_T("CKnownHostsDlg"), InitKnownHostsTab, 3);

	int n, i;
	LPCTSTR p;
	CStringArrayExt list, entry;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
	CIdKey key;
	KNOWNHOSTDATA *pData;

	m_Data.RemoveAll();

	pApp->GetProfileKeys(_T("KnownHosts"), list);
	for ( n = 0 ; n < list.GetSize() ; n++ ) {

		// 古い形式(KnownHosts\host-xxx)
		if ( (p = _tcsrchr(list[n], _T('-'))) != NULL && (key.GetTypeFromName(p + 1) & IDKEY_TYPE_MASK) != IDKEY_NONE ) {
			pData = new KNOWNHOSTDATA;
			pData->key = list[n];
			pData->del = FALSE;
			pData->type = 0;

			pData->host.Empty();
			for ( LPCTSTR s = list[n] ; s < p ; )
				pData->host += *(s++);
			pData->port = _T("");
			pData->digest = pApp->GetProfileString(_T("KnownHosts"), list[n], _T(""));

			m_Data.Add(pData);

		// 新しい形式(KnownHosts\host:nnn)
		} else if ( (p = _tcsrchr(list[n], _T(':'))) != NULL && _tstoi(p + 1) != 0 ) {
			pApp->GetProfileStringArray(_T("KnownHosts"), list[n], entry);
			for ( i = 0 ; i < entry.GetSize() ; i++ ) {
				pData = new KNOWNHOSTDATA;
				pData->key = list[n];
				pData->del = FALSE;
				pData->type = 2;
				pData->host.Empty();
				for ( LPCTSTR s = list[n] ; s < p ; )
					pData->host += *(s++);
				pData->port = p + 1;
				pData->digest = entry[i];

				m_Data.Add(pData);
			}

		// 古い形式(KnownHosts\host)
		} else {
			pApp->GetProfileStringArray(_T("KnownHosts"), list[n], entry);
			for ( i = 0 ; i < entry.GetSize() ; i++ ) {
				pData = new KNOWNHOSTDATA;
				pData->key = list[n];
				pData->del = FALSE;
				pData->type = 1;
				pData->host = list[n];
				pData->port = _T("ssh");
				pData->digest = entry[i];

				m_Data.Add(pData);
			}
		}
	}


	InitList();
	SetWindowText(_T("Known Host Keys Delete"));
	SetDlgItemText(IDOK, _T("DELETE"));

	return TRUE;
}

void CKnownHostsDlg::OnOK()
{
	int n, i;
	int dels = 0;
	KNOWNHOSTDATA *pData;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
	CStringArrayExt entry;
	CString msg;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		i = (int)m_List.GetItemData(n);
		if ( m_List.GetLVCheck(n) && (i = (int)m_List.GetItemData(n)) < (int)m_Data.GetSize() ) {
			((KNOWNHOSTDATA *)m_Data.GetAt(i))->del = TRUE;
			dels++;
		}
	}

	msg.Format(CStringLoad(IDS_KNOWNHOSTDELMSG), dels);

	if ( dels > 0 && MessageBox(msg, _T("Question"), MB_ICONQUESTION | MB_YESNO) == IDYES ) {
		for ( n = 0 ; n < m_Data.GetSize() ; ) {
			pData = (KNOWNHOSTDATA *)m_Data.GetAt(n);
			if ( !pData->del ) {
				n++;
				continue;
			}

			switch(pData->type) {
			case 0:	// KnownHosts\host-xxx
				pApp->DelProfileEntry(_T("KnownHosts"), pData->key);
				n++;
				break;

			case 1:	// KnownHosts\host
			case 2:	// KnownHosts\host:nnn
				for ( i = n ; i >= 0 && pData->key.Compare(((KNOWNHOSTDATA *)m_Data.GetAt(i))->key) == 0 ; )
					i--;
				i++;

				entry.RemoveAll();
				for ( ; i < m_Data.GetSize() && pData->key.Compare(((KNOWNHOSTDATA *)m_Data.GetAt(i))->key) == 0 ; i++ ) {
					if ( !((KNOWNHOSTDATA *)m_Data.GetAt(i))->del )
						entry.Add(((KNOWNHOSTDATA *)m_Data.GetAt(i))->digest);
				}
				n = i;

				if ( entry.GetSize() > 0 )
					pApp->WriteProfileStringArray(_T("KnownHosts"), pData->key, entry);
				else
					pApp->DelProfileEntry(_T("KnownHosts"), pData->key);

				break;
			}
		}
	}

	CDialogExt::OnOK();
}
