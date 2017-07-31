// SerEntPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "ComSock.h"
#include "OptDlg.h"
#include "SerEntPage.h"
#include "ChatDlg.h"
#include "ProxyDlg.h"
#include "EnvDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSerEntPage プロパティ ページ

IMPLEMENT_DYNCREATE(CSerEntPage, CPropertyPage)

CSerEntPage::CSerEntPage() : CPropertyPage(CSerEntPage::IDD)
{
	//{{AFX_DATA_INIT(CSerEntPage)
	m_EntryName = _T("");
	m_HostName = _T("");
	m_PortName = _T("");
	m_UserName = _T("");
	m_PassName = _T("");
	m_TermName = _T("");
	m_KanjiCode = 0;
	m_ProtoType = 0;
	//}}AFX_DATA_INIT
	m_DefComPort = _T("");
	m_IdkeyName = _T("");
	m_Memo = _T("");
}

CSerEntPage::~CSerEntPage()
{
}

void CSerEntPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSerEntPage)
	DDX_Text(pDX, IDC_ENTRYNAME, m_EntryName);
	DDX_CBString(pDX, IDC_SERVERNAME, m_HostName);
	DDX_CBString(pDX, IDC_SOCKNO, m_PortName);
	DDX_CBString(pDX, IDC_LOGINNAME, m_UserName);
	DDX_Text(pDX, IDC_PASSWORD, m_PassName);
	DDX_CBString(pDX, IDC_TERMNAME, m_TermName);
	DDX_Radio(pDX, IDC_KANJICODE1, m_KanjiCode);
	DDX_Radio(pDX, IDC_PROTO1, m_ProtoType);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_ENTRYMEMO, m_Memo);
}


BEGIN_MESSAGE_MAP(CSerEntPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSerEntPage)
	ON_BN_CLICKED(IDC_COMCONFIG, OnComconfig)
	ON_BN_CLICKED(IDC_KEYFILESELECT, OnKeyfileselect)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_PROTO1, IDC_PROTO6, OnProtoType)
	ON_EN_CHANGE(IDC_ENTRYNAME, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SERVERNAME, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SOCKNO, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_LOGINNAME, OnUpdateEdit)
	ON_EN_CHANGE(IDC_PASSWORD, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_TERMNAME, OnUpdateEdit)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_KANJICODE1, IDC_KANJICODE4, OnUpdateCheck)
	ON_BN_CLICKED(IDC_CHATEDIT, &CSerEntPage::OnChatEdit)
	ON_BN_CLICKED(IDC_PROXYSET, &CSerEntPage::OnProxySet)
	ON_BN_CLICKED(IDC_TERMCAP, &CSerEntPage::OnBnClickedTermcap)
	ON_EN_CHANGE(IDC_ENTRYMEMO, OnUpdateEdit)
END_MESSAGE_MAP()

void CSerEntPage::SetEnableWind()
{
	int n;
	CWnd *pWnd;
	static const char *DefPortName[] = { "", "login", "telnet", "ssh", "COM1", "" };
	static const struct {
		int		nId;
		BOOL	mode[6];
	} ItemTab[] = {			/*	drect	login	telnet	ssh		com		pipe */
		{ IDC_COMCONFIG,	  {	FALSE,	FALSE,	FALSE,	FALSE,	TRUE,	FALSE } },
		{ IDC_KEYFILESELECT,  {	FALSE,	FALSE,	FALSE,	TRUE,	FALSE,	FALSE } },
		{ IDC_SOCKNO,		  {	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE } },
		{ IDC_LOGINNAME,	  {	FALSE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_PASSWORD,		  {	FALSE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_TERMNAME,		  {	FALSE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_PROXYSET,		  {	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_TERMCAP,		  {	FALSE,	FALSE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ 0 }
	};

	for ( n = 0 ; ItemTab[n].nId != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].nId)) != NULL )
			pWnd->EnableWindow(ItemTab[n].mode[m_ProtoType]);
	}

	switch(m_ProtoType) {
	case PROTO_COMPORT:
		if ( m_PortName.Left(3).Compare("COM") != 0 )
			m_PortName = m_DefComPort;
		break;

	case PROTO_SSH:
	case PROTO_LOGIN:
	case PROTO_TELNET:
		if ( atoi(m_PortName) <= 0 )
			m_PortName = DefPortName[m_ProtoType];
		break;

	case PROTO_DIRECT:
	case PROTO_PIPE:
		break;
	}

	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CSerEntPage メッセージ ハンドラ

BOOL CSerEntPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	int n, i;
	CComboBox *pCombo;
	CString str;
	CComSock com(NULL);

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pEntry);

	m_EntryName = m_pSheet->m_pEntry->m_EntryName;
	m_HostName  = m_pSheet->m_pEntry->m_HostReal;
	m_PortName  = m_pSheet->m_pEntry->m_PortName;
	m_UserName  = m_pSheet->m_pEntry->m_UserReal;
	m_PassName  = m_pSheet->m_pEntry->m_PassReal;
	m_TermName  = m_pSheet->m_pEntry->m_TermName;
	m_IdkeyName = m_pSheet->m_pEntry->m_IdkeyName;
	m_KanjiCode = m_pSheet->m_pEntry->m_KanjiCode;
	m_ProtoType = m_pSheet->m_pEntry->m_ProtoType;
	m_ProxyMode = m_pSheet->m_pEntry->m_ProxyMode;
	m_ProxyHost = m_pSheet->m_pEntry->m_ProxyHost;
	m_ProxyPort = m_pSheet->m_pEntry->m_ProxyPort;
	m_ProxyUser = m_pSheet->m_pEntry->m_ProxyUser;
	m_ProxyPass = m_pSheet->m_pEntry->m_ProxyPass;
	m_Memo      = m_pSheet->m_pEntry->m_Memo;
	m_ExtEnvStr = m_pSheet->m_pParamTab->m_ExtEnvStr;

	if ( m_PortName.Compare("serial") == 0 ) {
		com.GetMode(m_HostName);
		m_PortName = com.m_ComName;
	}
	UpdateData(FALSE);

	int pb = com.AliveComPort();
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SOCKNO)) != NULL ) {
		for ( n = 1 ; n <= 16 ; n++ ) {
			str.Format("COM%d", n);
			if ( (pb & (1 << n)) != 0 ) {
				if ( (i = pCombo->FindString((-1), str)) == CB_ERR )
					pCombo->AddString(str);
				if ( m_DefComPort.IsEmpty() )
					m_DefComPort = str;
			} else if ( (i = pCombo->FindString((-1), str)) != CB_ERR ) {
				pCombo->DeleteString(i);
			}
		}
	}

	m_pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SERVERNAME)) != NULL ) {
		for ( n = 0 ; n < m_pTab->m_Data.GetSize() ; n++ ) {
			str = m_pTab->m_Data[n].m_HostName;
			if ( !str.IsEmpty() && pCombo->FindString((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_LOGINNAME)) != NULL ) {
		for ( n = 0 ; n < m_pTab->m_Data.GetSize() ; n++ ) {
			str = m_pTab->m_Data[n].m_UserName;
			if ( !str.IsEmpty() && pCombo->FindString((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_TERMNAME)) != NULL ) {
		for ( n = 0 ; n < m_pTab->m_Data.GetSize() ; n++ ) {
			str = m_pTab->m_Data[n].m_TermName;
			if ( !str.IsEmpty() && pCombo->FindString((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SOCKNO)) != NULL ) {
		for ( n = 0 ; n < m_pTab->m_Data.GetSize() ; n++ ) {
			str = m_pTab->m_Data[n].m_PortName;
			if ( !str.IsEmpty() && pCombo->FindString((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}

	SetEnableWind();
	
	return TRUE;
}
BOOL CSerEntPage::OnApply() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pEntry);

	UpdateData(TRUE);

	m_pSheet->m_pEntry->m_EntryName = m_EntryName;
	m_pSheet->m_pEntry->m_HostName  = m_HostName;
	m_pSheet->m_pEntry->m_PortName  = m_PortName;
	m_pSheet->m_pEntry->m_UserName  = m_UserName;
	m_pSheet->m_pEntry->m_PassName  = m_PassName;
	m_pSheet->m_pEntry->m_TermName  = m_TermName;
	m_pSheet->m_pEntry->m_IdkeyName = m_IdkeyName;
	m_pSheet->m_pEntry->m_KanjiCode = m_KanjiCode;
	m_pSheet->m_pEntry->m_ProtoType = m_ProtoType;
	m_pSheet->m_pEntry->m_HostReal  = m_HostName;
	m_pSheet->m_pEntry->m_UserReal  = m_UserName;
	m_pSheet->m_pEntry->m_PassReal  = m_PassName;
	m_pSheet->m_pEntry->m_ProxyMode = m_ProxyMode;
	m_pSheet->m_pEntry->m_ProxyHost = m_ProxyHost;
	m_pSheet->m_pEntry->m_ProxyPort = m_ProxyPort;
	m_pSheet->m_pEntry->m_ProxyUser = m_ProxyUser;
	m_pSheet->m_pEntry->m_ProxyPass = m_ProxyPass;
	m_pSheet->m_pEntry->m_Memo      = m_Memo;
	m_pSheet->m_pParamTab->m_ExtEnvStr = m_ExtEnvStr;

	return TRUE;
}

void CSerEntPage::OnReset() 
{
	if ( m_hWnd == NULL )
		return;

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pEntry);

	m_EntryName = m_pSheet->m_pEntry->m_EntryName;
	m_HostName  = m_pSheet->m_pEntry->m_HostReal;
	m_PortName  = m_pSheet->m_pEntry->m_PortName;
	m_UserName  = m_pSheet->m_pEntry->m_UserReal;
	m_PassName  = m_pSheet->m_pEntry->m_PassReal;
	m_TermName  = m_pSheet->m_pEntry->m_TermName;
	m_IdkeyName = m_pSheet->m_pEntry->m_IdkeyName;
	m_KanjiCode = m_pSheet->m_pEntry->m_KanjiCode;
	m_ProtoType = m_pSheet->m_pEntry->m_ProtoType;
	m_ProxyMode = m_pSheet->m_pEntry->m_ProxyMode;
	m_ProxyHost = m_pSheet->m_pEntry->m_ProxyHost;
	m_ProxyPort = m_pSheet->m_pEntry->m_ProxyPort;
	m_ProxyUser = m_pSheet->m_pEntry->m_ProxyUser;
	m_ProxyPass = m_pSheet->m_pEntry->m_ProxyPass;
	m_Memo      = m_pSheet->m_pEntry->m_Memo;
	m_ExtEnvStr = m_pSheet->m_pParamTab->m_ExtEnvStr;

	UpdateData(FALSE);
	SetModified(FALSE);
}

void CSerEntPage::OnComconfig() 
{
	CComSock com(NULL);
	UpdateData(TRUE);
	if ( m_PortName.Left(3).CompareNoCase("COM") == 0 ) {
		com.m_ComPort = (-1);
		com.GetMode(m_HostName);
		if ( com.m_ComPort != atoi(m_PortName.Mid(3)) )
			m_HostName = m_PortName;
	}
	com.ConfigDlg(this, m_HostName);
	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnKeyfileselect() 
{
	UpdateData(TRUE);
	CFileDialog dlg(TRUE, "", m_IdkeyName, OFN_HIDEREADONLY, "All Files (*.*)|*.*||", this);
	if ( dlg.DoModal() != IDOK ) {
		if ( m_IdkeyName.IsEmpty() || MessageBox("認証キーファイルの設定を解除しますか？", "Question", MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
		m_IdkeyName.Empty();
	} else
		m_IdkeyName = dlg.GetPathName();
	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnProtoType(UINT nID) 
{
	UpdateData(TRUE);
	SetEnableWind();
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnChatEdit()
{
	CChatDlg dlg;

	dlg.m_Script = m_pSheet->m_pEntry->m_Script;

	if ( dlg.DoModal() == IDOK ) {
		m_pSheet->m_pEntry->m_Script = dlg.m_Script;
		SetModified(TRUE);
		m_pSheet->m_ModFlag |= UMOD_ENTRY;
	}
}

void CSerEntPage::OnProxySet()
{
	CProxyDlg dlg;

	dlg.m_ProxyMode  = m_ProxyMode & 7;
	dlg.m_SSLMode    = m_ProxyMode >> 3;
	dlg.m_ServerName = m_ProxyHost;
	dlg.m_PortName   = m_ProxyPort;
	dlg.m_UserName   = m_ProxyUser;
	dlg.m_PassWord   = m_ProxyPass;

	if ( dlg.DoModal() != IDOK )
		return;

	m_ProxyMode = dlg.m_ProxyMode | (dlg.m_SSLMode << 3);
	m_ProxyHost = dlg.m_ServerName;
	m_ProxyPort = dlg.m_PortName;
	m_ProxyUser = dlg.m_UserName;
	m_ProxyPass = dlg.m_PassWord;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}

void CSerEntPage::OnBnClickedTermcap()
{
	int n;
	CEnvDlg dlg;
	CComboBox *pCombo;

	UpdateData(TRUE);

	dlg.m_Env.SetNoSort(TRUE);
	dlg.m_Env.GetString(m_ExtEnvStr);

	if ( !m_TermName.IsEmpty() ) {
		dlg.m_Env["TERM"].m_Value  = 0;
		dlg.m_Env["TERM"].m_String = m_TermName;
	}

	if ( dlg.DoModal() != IDOK )
		return;

	if ( (n = dlg.m_Env.Find("TERM")) >= 0 ) {
		dlg.m_Env["TERM"].m_Value = 0;
		if ( !dlg.m_Env["TERM"].m_String.IsEmpty() )
			m_TermName = dlg.m_Env["TERM"];
	}

	dlg.m_Env.SetString(m_ExtEnvStr);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_TERMNAME)) != NULL ) {
		if ( pCombo->FindStringExact(0, m_TermName) < 0 )
			pCombo->AddString(m_TermName);
	}

	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_PARAMTAB;
}
