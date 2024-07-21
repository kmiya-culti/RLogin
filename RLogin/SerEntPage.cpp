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

IMPLEMENT_DYNCREATE(CSerEntPage, CTreePage)

CSerEntPage::CSerEntPage() : CTreePage(CSerEntPage::IDD)
, m_UsePassDlg(FALSE)
{
	m_UrlOpt = _T("#SERVEROPT");

	m_EntryName = _T("");
	m_HostName = _T("");
	m_PortName = _T("");
	m_UserName = _T("");
	m_PassName = _T("");
	m_TermName = _T("");
	m_KanjiCode = 0;
	m_ProtoType = 0;
	m_Memo = _T("");
	m_Group = _T("");
	m_DefComPort = _T("");
	m_IdkeyName = _T("");
	m_ProxyMode = 0;
	m_SSL_Keep = FALSE;
	m_ProxyHost = _T("");
	m_ProxyPort = _T("");
	m_ProxyUser = _T("");
	m_ProxyPass = _T("");
	m_ProxyCmd = _T("");
	m_ExtEnvStr = _T("");
	m_BeforeEntry = _T("");
	m_UsePassDlg = FALSE;
	m_UseProxyDlg = FALSE;
	m_IconName = _T("");
	m_bOptFixed = FALSE;
	m_OptFixEntry = _T("");
}
CSerEntPage::~CSerEntPage()
{
}

void CSerEntPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_ENTRYNAME, m_EntryName);
	DDX_CBStringExact(pDX, IDC_SERVERNAME, m_HostName);
	DDX_CBStringExact(pDX, IDC_SOCKNO, m_PortName);
	DDX_CBStringExact(pDX, IDC_LOGINNAME, m_UserName);
	DDX_Text(pDX, IDC_PASSWORD, m_PassName);
	DDX_CBStringExact(pDX, IDC_TERMNAME, m_TermName);
	DDX_Radio(pDX, IDC_KANJICODE1, m_KanjiCode);
	DDX_Radio(pDX, IDC_PROTO1, m_ProtoType);
	DDX_Text(pDX, IDC_ENTRYMEMO, m_Memo);
	DDX_CBStringExact(pDX, IDC_GROUP, m_Group);
	DDX_CBStringExact(pDX, IDC_BEFORE, m_BeforeEntry);
	DDX_Check(pDX, IDC_CHECK1, m_UsePassDlg);
	DDX_Check(pDX, IDC_CHECK2, m_bOptFixed);
	DDX_CBStringExact(pDX, IDC_OPTFIXENTRY, m_OptFixEntry);
}

BEGIN_MESSAGE_MAP(CSerEntPage, CTreePage)
	ON_BN_CLICKED(IDC_COMCONFIG, OnComconfig)
	ON_BN_CLICKED(IDC_KEYFILESELECT, OnKeyfileselect)
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
	ON_BN_CLICKED(IDC_TERMCAP, &CSerEntPage::OnTermcap)
	ON_EN_CHANGE(IDC_ENTRYMEMO, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_GROUP, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_BEFORE, OnUpdateEdit)
	ON_BN_CLICKED(IDC_CHECK1, OnUpdateOption)
	ON_BN_CLICKED(IDC_ICONFILE, &CSerEntPage::OnIconfile)
	ON_BN_CLICKED(IDC_CHECK2, OnUpdateOptFixed)
	ON_CBN_EDITCHANGE(IDC_OPTFIXENTRY, OnUpdateEdit)
END_MESSAGE_MAP()

void CSerEntPage::SetEnableWind()
{
	int n;
	CWnd *pWnd;
	static const struct {
		int		nId;
		BOOL	mode[6];
	} ItemTab[] = {			/*	drect	login	telnet	ssh		com		pipe */
		{ IDC_COMCONFIG,	  {	FALSE,	FALSE,	FALSE,	FALSE,	TRUE,	FALSE } },
		{ IDC_KEYFILESELECT,  {	FALSE,	FALSE,	FALSE,	TRUE,	FALSE,	FALSE } },
		{ IDC_SOCKNO,		  {	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE } },
		{ IDC_LOGINNAME,	  {	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE  } },
		{ IDC_PASSWORD,		  {	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE  } },
		{ IDC_TERMNAME,		  {	FALSE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_PROXYSET,		  {	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_TERMCAP,		  {	FALSE,	FALSE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_CHECK1,		  {	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE  } },
		{ 0 }
	};

	for ( n = 0 ; ItemTab[n].nId != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].nId)) != NULL )
			pWnd->EnableWindow(ItemTab[n].mode[m_ProtoType]);
	}
}

static LPCTSTR	DefPortName[] = { _T(""), _T("login"), _T("telnet"), _T("ssh"), _T("COM1"), _T("") };

/////////////////////////////////////////////////////////////////////////////
// CSerEntPage メッセージ ハンドラ

void CSerEntPage::DoInit()
{
	m_EntryName   = m_pSheet->m_pEntry->m_EntryName;
	m_HostName    = m_pSheet->m_pEntry->m_HostNameProvs;
	m_PortName    = m_pSheet->m_pEntry->m_PortNameProvs;
	m_UserName    = m_pSheet->m_pEntry->m_UserNameProvs;
	m_PassName    = m_pSheet->m_pEntry->m_PassNameProvs;
	m_TermName    = m_pSheet->m_pEntry->m_TermName;
	m_IdkeyName   = m_pSheet->m_pEntry->m_IdkeyName;
	m_KanjiCode   = m_pSheet->m_pEntry->m_KanjiCode;
	m_ProtoType   = m_pSheet->m_pEntry->m_ProtoType;
	m_ProxyMode   = m_pSheet->m_pEntry->m_ProxyMode;
	m_ProxyHost   = m_pSheet->m_pEntry->m_ProxyHostProvs;
	m_ProxyPort   = m_pSheet->m_pEntry->m_ProxyPortProvs;
	m_ProxyUser   = m_pSheet->m_pEntry->m_ProxyUserProvs;
	m_ProxyPass   = m_pSheet->m_pEntry->m_ProxyPassProvs;
	m_ProxyCmd    = m_pSheet->m_pEntry->m_ProxyCmd;
	m_SSL_Keep    = m_pSheet->m_pEntry->m_ProxySSLKeep;
	m_Memo        = m_pSheet->m_pEntry->m_Memo;
	m_Group       = m_pSheet->m_pEntry->m_Group;
	m_BeforeEntry = m_pSheet->m_pEntry->m_BeforeEntry;
	m_IconName    = m_pSheet->m_pEntry->m_IconName;
	m_bOptFixed   = m_pSheet->m_pEntry->m_bOptFixed;
	m_OptFixEntry = m_pSheet->m_pEntry->m_OptFixEntry;

	m_ExtEnvStr   = m_pSheet->m_pParamTab->m_ExtEnvStr;

	m_UsePassDlg  = (m_pSheet->m_pTextRam->IsOptEnable(TO_RLUSEPASS) ? TRUE : FALSE);
	m_UseProxyDlg = (m_pSheet->m_pTextRam->IsOptEnable(TO_PROXPASS)  ? TRUE : FALSE);

	if ( m_PortName.IsEmpty() ) {
		if ( m_ProtoType == PROTO_COMPORT )
			m_PortName = m_DefComPort;
		else
			m_PortName = DefPortName[m_ProtoType];
	}

	UpdateData(FALSE);
}

BOOL CSerEntPage::OnInitDialog() 
{
	int n, i;
	CString str;
	CComboBox *pCombo;
	CButton *pCheck;
	BYTE ComAliveBits[COMALIVEBYTE];
	CServerEntryTab *pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);
	
	ASSERT(m_pSheet != NULL && m_pSheet->m_pEntry != NULL);

	CTreePage::OnInitDialog();

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SERVERNAME)) != NULL ) {
		if ( !m_pSheet->m_pEntry->m_HostNameProvs.IsEmpty() )
			pCombo->AddString(m_pSheet->m_pEntry->m_HostNameProvs);
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_HostName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_LOGINNAME)) != NULL ) {
		if ( !m_pSheet->m_pEntry->m_UserNameProvs.IsEmpty() )
			pCombo->AddString(m_pSheet->m_pEntry->m_UserNameProvs);
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_UserName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_TERMNAME)) != NULL ) {
		//if ( !m_pSheet->m_pEntry->m_TermName.IsEmpty() )
		//	pCombo->AddString(m_pSheet->m_pEntry->m_TermName);
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_TermName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SOCKNO)) != NULL ) {
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_PortNameProvs;
			if ( !str.IsEmpty() && _tcsncmp(str, _T("COM"), 3) != 0 && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
		CComSock::AliveComPort(ComAliveBits);
		for ( n = 1 ; n < COMALIVEBITS ; n++ ) {
			if ( (ComAliveBits[n / 8] & (1 << (n % 8))) != 0 ) {
				str.Format(_T("COM%d"), n);
				if ( (i = pCombo->FindStringExact((-1), str)) == CB_ERR )
					pCombo->AddString(str);
				if ( m_DefComPort.IsEmpty() )
					m_DefComPort = str;
			}
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_GROUP)) != NULL ) {
		if ( !m_pSheet->m_pEntry->m_Group.IsEmpty() )
			pCombo->AddString(m_pSheet->m_pEntry->m_Group);
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_Group;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_BEFORE)) != NULL ) {
		if ( !m_pSheet->m_pEntry->m_BeforeEntry.IsEmpty() )
			pCombo->AddString(m_pSheet->m_pEntry->m_BeforeEntry);
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_EntryName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_OPTFIXENTRY)) != NULL ) {
		if ( !m_pSheet->m_pEntry->m_OptFixEntry.IsEmpty() )
			pCombo->AddString(m_pSheet->m_pEntry->m_OptFixEntry);
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_EntryName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}

	if ( (m_pSheet->m_psh.dwFlags & PSH_NOAPPLYNOW) == 0 && (pCheck = (CButton *)GetDlgItem(IDC_CHECK2)) != NULL )
		pCheck->EnableWindow(FALSE);

	DoInit();
	SetEnableWind();

	SubclassComboBox(IDC_SERVERNAME);
	SubclassComboBox(IDC_LOGINNAME);
	SubclassComboBox(IDC_TERMNAME);
	SubclassComboBox(IDC_SOCKNO);
	SubclassComboBox(IDC_GROUP);
	SubclassComboBox(IDC_BEFORE);
	SubclassComboBox(IDC_OPTFIXENTRY);
	
	return TRUE;
}
BOOL CSerEntPage::OnApply() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pEntry != NULL);

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
	m_pSheet->m_pEntry->m_ProxyMode = m_ProxyMode;
	m_pSheet->m_pEntry->m_ProxyHost = m_ProxyHost;
	m_pSheet->m_pEntry->m_ProxyPort = m_ProxyPort;
	m_pSheet->m_pEntry->m_ProxyUser = m_ProxyUser;
	m_pSheet->m_pEntry->m_ProxyPass = m_ProxyPass;
	m_pSheet->m_pEntry->m_ProxyCmd  = m_ProxyCmd;
	m_pSheet->m_pEntry->m_Memo      = m_Memo;
	m_pSheet->m_pEntry->m_Group     = m_Group;
	m_pSheet->m_pEntry->m_HostNameProvs  = m_HostName;
	m_pSheet->m_pEntry->m_PortNameProvs  = m_PortName;
	m_pSheet->m_pEntry->m_UserNameProvs  = m_UserName;
	m_pSheet->m_pEntry->m_PassNameProvs  = m_PassName;
	m_pSheet->m_pEntry->m_ProxyHostProvs = m_ProxyHost;
	m_pSheet->m_pEntry->m_ProxyPortProvs = m_ProxyPort;
	m_pSheet->m_pEntry->m_ProxyUserProvs = m_ProxyUser;
	m_pSheet->m_pEntry->m_ProxyPassProvs = m_ProxyPass;
	m_pSheet->m_pEntry->m_ProxySSLKeep   = m_SSL_Keep;
	m_pSheet->m_pEntry->m_BeforeEntry    = m_BeforeEntry;
	m_pSheet->m_pParamTab->m_ExtEnvStr = m_ExtEnvStr;
	m_pSheet->m_pTextRam->SetOption(TO_RLUSEPASS, m_UsePassDlg);
	m_pSheet->m_pTextRam->SetOption(TO_PROXPASS, m_UseProxyDlg);
	m_pSheet->m_pEntry->m_IconName  = m_IconName;
	m_pSheet->m_pEntry->m_bPassOk   = TRUE;
	m_pSheet->m_pEntry->m_bOptFixed   = m_bOptFixed;
	m_pSheet->m_pEntry->m_OptFixEntry = m_OptFixEntry;

	return TRUE;
}

void CSerEntPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pEntry != NULL);

	DoInit();
	SetModified(FALSE);
}

void CSerEntPage::OnComconfig() 
{
	int Port = (-1);
	CComSock com(NULL);
	CComboBox *pCombo;

	UpdateData(TRUE);

	if ( _tcsnicmp(m_PortName, _T("COM"), 3) == 0 )
		Port = _tstoi((LPCTSTR)m_PortName + 3);

	if ( !com.SetupComConf(m_HostName, Port, this) )
		return;

	m_PortName.Format(_T("COM%d"), Port);

	UpdateData(FALSE);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SERVERNAME)) != NULL )
		pCombo->SetWindowText(m_HostName);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnKeyfileselect() 
{
	CString file;

	UpdateData(TRUE);
	file = m_IdkeyName;

	CRLoginDoc::EnvironPath(file);
	CFileDialog dlg(TRUE, _T(""), file, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK ) {
		if ( m_IdkeyName.IsEmpty() || ::AfxMessageBox(CStringLoad(IDS_IDKEYFILEDELREQ), MB_ICONQUESTION | MB_YESNO) != IDYES )
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

	switch(m_ProtoType) {
	case PROTO_COMPORT:
		if ( _tcsnicmp(m_PortName, _T("COM"), 3) != 0 )
			m_PortName = m_DefComPort;
		break;

	case PROTO_SSH:
	case PROTO_LOGIN:
	case PROTO_TELNET:
		if ( _tstoi(m_PortName) <= 0 )
			m_PortName = DefPortName[m_ProtoType];
		break;

	case PROTO_DIRECT:
	case PROTO_PIPE:
		break;
	}

	UpdateData(FALSE);
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
void CSerEntPage::OnUpdateOption() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
void CSerEntPage::OnChatEdit()
{
	CChatDlg dlg;

	dlg.m_Script = m_pSheet->m_pEntry->m_ChatScript;

	if ( dlg.DoModal() != IDOK )
		return;

	m_pSheet->m_pEntry->m_ChatScript = dlg.m_Script;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
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
	dlg.m_SSL_Keep   = m_SSL_Keep;
	dlg.m_UsePassDlg = m_UseProxyDlg;
	dlg.m_ProxyCmd   = m_ProxyCmd;

	if ( dlg.DoModal() != IDOK )
		return;

	m_ProxyMode   = dlg.m_ProxyMode | (dlg.m_SSLMode << 3);
	m_ProxyHost   = dlg.m_ServerName;
	m_ProxyPort   = dlg.m_PortName;
	m_ProxyUser   = dlg.m_UserName;
	m_ProxyPass   = dlg.m_PassWord;
	m_SSL_Keep    = dlg.m_SSL_Keep;
	m_UseProxyDlg = dlg.m_UsePassDlg;
	m_ProxyCmd    = dlg.m_ProxyCmd;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_ENTRY | UMOD_PARAMTAB);
}

void CSerEntPage::OnTermcap()
{
	int n;
	CEnvDlg dlg;
	CComboBox *pCombo;

	UpdateData(TRUE);

	dlg.m_Env.SetNoSort(TRUE);
	dlg.m_Env.GetString(m_ExtEnvStr);

	if ( !m_TermName.IsEmpty() ) {
		dlg.m_Env[_T("TERM")].m_Value  = 0;
		dlg.m_Env[_T("TERM")].m_String = m_TermName;
	}

	if ( dlg.DoModal() != IDOK )
		return;

	if ( (n = dlg.m_Env.Find(_T("TERM"))) >= 0 ) {
		dlg.m_Env[_T("TERM")].m_Value = 0;
		if ( !dlg.m_Env[_T("TERM")].m_String.IsEmpty() )
			m_TermName = dlg.m_Env[_T("TERM")];
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
void CSerEntPage::OnIconfile()
{
	UpdateData(TRUE);

	CFileDialog dlg(TRUE, _T(""), m_IconName, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGICONGRAP), this);

	if ( DpiAwareDoModal(dlg) != IDOK ) {
		if ( m_IconName.IsEmpty() || ::AfxMessageBox(CStringLoad(IDS_ICONFILEDELREQ), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
		m_IconName.Empty();
	} else
		m_IconName = dlg.GetPathName();

	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnUpdateOptFixed()
{
	UpdateData(TRUE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
	m_pSheet->m_bOptFixed = m_bOptFixed;
}
