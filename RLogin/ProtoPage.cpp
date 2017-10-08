// ProtoPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "ProtoPage.h"
#include "AlgoDlg.h"
#include "IdkeySelDLg.h"
#include "PfdListDlg.h"
#include "TtyModeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProtoPage プロパティ ページ

IMPLEMENT_DYNCREATE(CProtoPage, CTreePage)

#define	CHECKOPTMAX		8
#define	CHECKOPTEXT		3
#define	IDC_CHECKFAST	IDC_PROTOCHECK1
static const int CheckOptTab[] = { TO_RLTENAT,  TO_RLTENEC,  TO_RLTENLM,
								   TO_SSH1MODE, TO_SSHPFORY, TO_SSHAGENT, TO_SSHKEEPAL, TO_TELKEEPAL,
								   TO_SSHSFENC, TO_SSHSFMAC, TO_SSHX11PF	};		// Extend

CProtoPage::CProtoPage() : CTreePage(CProtoPage::IDD)
{
	m_KeepAlive = 0;
	for ( int n = 0 ; n < CHECKOPTMAX + CHECKOPTEXT ; n++ )
		m_Check[n] = FALSE;
	m_RsaExt = 0;
	m_VerIdent.Empty();
}

CProtoPage::~CProtoPage()
{
}

void CProtoPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_KEEPALIVE, m_KeepAlive);
	DDX_Text(pDX, IDC_KEEPALIVE2, m_TelKeepAlive);
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_CHECKFAST + n, m_Check[n]);
	DDX_CBIndex(pDX, IDC_RSAEXTSEL, m_RsaExt);
	DDX_Text(pDX, IDC_VERINDENT, m_VerIdent);
}

BEGIN_MESSAGE_MAP(CProtoPage, CTreePage)
	ON_BN_CLICKED(IDC_SSHALGO, OnSshAlgo)
	ON_BN_CLICKED(IDC_SSHIDKEY, OnSshIdkey)
	ON_BN_CLICKED(IDC_SSHPFD, OnSshPfd)
	ON_BN_CLICKED(IDC_SSHTTYMODE, OnSshTtyMode)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_EN_CHANGE(IDC_DELAYMSEC, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_RSAEXTSEL, OnUpdateEdit)
	ON_EN_CHANGE(IDC_VERINDENT, OnUpdateEdit)
	ON_EN_CHANGE(IDC_KEEPALIVE, OnUpdateEdit)
	ON_EN_CHANGE(IDC_KEEPALIVE2, OnUpdateEdit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProtoPage メッセージ ハンドラ

void CProtoPage::DoInit()
{
	for ( int n = 0 ; n < CHECKOPTMAX + CHECKOPTEXT ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_KeepAlive = m_pSheet->m_pTextRam->m_SshKeepAlive;
	m_TelKeepAlive = m_pSheet->m_pTextRam->m_TelKeepAlive;
	
	for ( int n = 0 ; n < 12 ; n++ )
		m_AlgoTab[n] = m_pSheet->m_pParamTab->m_AlgoTab[n];

	m_IdKeyList = m_pSheet->m_pParamTab->m_IdKeyList;
	m_PortFwd   = m_pSheet->m_pParamTab->m_PortFwd;
	m_XDisplay  = m_pSheet->m_pParamTab->m_XDisplay;
	m_RsaExt    = m_pSheet->m_pParamTab->m_RsaExt;
	m_VerIdent  = m_pSheet->m_pParamTab->m_VerIdent;

	m_x11AuthFlag = m_pSheet->m_pParamTab->m_x11AuthFlag;
	m_x11AuthName = m_pSheet->m_pParamTab->m_x11AuthName;
	m_x11AuthData = m_pSheet->m_pParamTab->m_x11AuthData;

	UpdateData(FALSE);
}

BOOL CProtoPage::OnInitDialog() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL && m_pSheet->m_pParamTab != NULL);

	CTreePage::OnInitDialog();

	DoInit();

	return TRUE;
}
BOOL CProtoPage::OnApply() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL && m_pSheet->m_pParamTab != NULL);

	UpdateData(TRUE);

	for ( int n = 0 ; n < CHECKOPTMAX + CHECKOPTEXT ; n++ )
		m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	m_pSheet->m_pTextRam->m_SshKeepAlive = m_KeepAlive;
	m_pSheet->m_pTextRam->m_TelKeepAlive = m_TelKeepAlive;

	for ( int n = 0 ; n < 12 ; n++ )
		m_pSheet->m_pParamTab->m_AlgoTab[n] = m_AlgoTab[n];

	m_pSheet->m_pParamTab->m_IdKeyList = m_IdKeyList;
	m_pSheet->m_pParamTab->m_PortFwd   = m_PortFwd;
	m_pSheet->m_pParamTab->m_XDisplay  = m_XDisplay;
	m_pSheet->m_pParamTab->m_RsaExt    = m_RsaExt;
	m_pSheet->m_pParamTab->m_VerIdent  = m_VerIdent;

	m_pSheet->m_pParamTab->m_x11AuthFlag = m_x11AuthFlag;
	m_pSheet->m_pParamTab->m_x11AuthName = m_x11AuthName;
	m_pSheet->m_pParamTab->m_x11AuthData = m_x11AuthData;

	return TRUE;
}
void CProtoPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL && m_pSheet->m_pParamTab != NULL);

	DoInit();
	SetModified(FALSE);
}

void CProtoPage::OnSshAlgo() 
{
	int n;
	CAlgoDlg dlg;

	for ( n = 0 ; n < 12 ; n++ )
		dlg.m_AlgoTab[n] = m_AlgoTab[n];

	dlg.m_EncShuffle = m_Check[CHECKOPTMAX + 0];
	dlg.m_MacShuffle = m_Check[CHECKOPTMAX + 1];

	if ( dlg.DoModal() != IDOK )
		return;

	for ( n = 0 ; n < 12 ; n++ )
		m_AlgoTab[n] = dlg.m_AlgoTab[n];

	m_Check[CHECKOPTMAX + 0] = dlg.m_EncShuffle;
	m_Check[CHECKOPTMAX + 1] = dlg.m_MacShuffle;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
void CProtoPage::OnSshIdkey() 
{
	CIdkeySelDLg dlg;

	((CMainFrame *)::AfxGetMainWnd())->PageantInit();

	dlg.m_pParamTab = m_pSheet->m_pParamTab;
	dlg.m_pIdKeyTab = &(((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab);
	dlg.m_IdKeyList = m_IdKeyList;

	if ( dlg.DoModal() != IDOK )
		return;

	m_IdKeyList = dlg.m_IdKeyList;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_PARAMTAB;
}
void CProtoPage::OnSshPfd() 
{
	CPfdListDlg dlg;

	dlg.m_pEntry      = m_pSheet->m_pEntry;

	dlg.m_X11PortFlag = m_Check[CHECKOPTMAX + 2];
	dlg.m_PortFwd     = m_PortFwd;
	dlg.m_XDisplay    = m_XDisplay;
	dlg.m_x11AuthFlag = m_x11AuthFlag;
	dlg.m_x11AuthName = m_x11AuthName;
	dlg.m_x11AuthData = m_x11AuthData;

	if ( dlg.DoModal() != IDOK )
		return;

	m_Check[CHECKOPTMAX + 2] = dlg.m_X11PortFlag;
	m_PortFwd     = dlg.m_PortFwd;
	m_XDisplay    = dlg.m_XDisplay;
	m_x11AuthFlag = dlg.m_x11AuthFlag;
	m_x11AuthName = dlg.m_x11AuthName;
	m_x11AuthData = dlg.m_x11AuthData;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
void CProtoPage::OnSshTtyMode()
{
	CTtyModeDlg dlg;

	dlg.m_pParamTab = m_pSheet->m_pParamTab;

	if ( dlg.DoModal() != IDOK )
		return;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_PARAMTAB);
}
void CProtoPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
void CProtoPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
