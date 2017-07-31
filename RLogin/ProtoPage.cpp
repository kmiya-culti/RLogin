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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProtoPage プロパティ ページ

IMPLEMENT_DYNCREATE(CProtoPage, CPropertyPage)

#define	CHECKOPTMAX		12
#define	CHECKOPTEXT		2
#define	IDC_CHECKFAST	IDC_PROTOCHECK1
static const int CheckOptTab[] = { TO_RLTENAT, TO_RLTENEC,   TO_SSH1MODE,  TO_SSHPFORY,
								   TO_RLPOFF,  TO_RLUSEPASS, TO_RLDSECHO,  TO_RLBPLUS,
								   TO_RLDELAY, TO_RLKEEPAL,  TO_SSHKEEPAL, TO_SSHAGENT,
								   TO_SSHSFENC,TO_SSHSFMAC	};								// Extend

CProtoPage::CProtoPage() : CPropertyPage(CProtoPage::IDD)
{
	//{{AFX_DATA_INIT(CProtoPage)
	m_DelayMsec = 0;
	m_KeepAlive = 0;
	//}}AFX_DATA_INIT
	for ( int n = 0 ; n < CHECKOPTMAX + CHECKOPTEXT ; n++ )
		m_Check[n] = FALSE;
	m_pSheet = NULL;
}

CProtoPage::~CProtoPage()
{
}

void CProtoPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProtoPage)
	DDX_Text(pDX, IDC_DELAYMSEC, m_DelayMsec);
	DDV_MinMaxUInt(pDX, m_DelayMsec, 0, 3000);
	DDX_Text(pDX, IDC_KEEPALIVE, m_KeepAlive);
	//}}AFX_DATA_MAP
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_PROTOCHECK1 + n, m_Check[n]);
}

BEGIN_MESSAGE_MAP(CProtoPage, CPropertyPage)
	//{{AFX_MSG_MAP(CProtoPage)
	ON_BN_CLICKED(IDC_SSHALGO, OnSshAlgo)
	ON_BN_CLICKED(IDC_SSHIDKEY, OnSshIdkey)
	ON_BN_CLICKED(IDC_SSHPFD, OnSshPfd)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_EN_CHANGE(IDC_DELAYMSEC, OnUpdateEdit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProtoPage メッセージ ハンドラ

BOOL CProtoPage::OnInitDialog() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);
	ASSERT(m_pSheet->m_pParamTab);

	CPropertyPage::OnInitDialog();
	for ( int n = 0 ; n < CHECKOPTMAX + CHECKOPTEXT ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_DelayMsec = m_pSheet->m_pTextRam->m_DelayMSec;
	m_KeepAlive = m_pSheet->m_pTextRam->m_KeepAliveSec;
	ParamTabTemp = *(m_pSheet->m_pParamTab);

	UpdateData(FALSE);

	return TRUE;
}
BOOL CProtoPage::OnApply() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);
	ASSERT(m_pSheet->m_pParamTab);

	UpdateData(TRUE);
	for ( int n = 0 ; n < CHECKOPTMAX + CHECKOPTEXT ; n++ ) {
		if ( m_Check[n] )
			m_pSheet->m_pTextRam->EnableOption(CheckOptTab[n]);
		else
			m_pSheet->m_pTextRam->DisableOption(CheckOptTab[n]);
	}

	m_pSheet->m_pTextRam->m_DelayMSec    = m_DelayMsec;
	m_pSheet->m_pTextRam->m_KeepAliveSec = m_KeepAlive;
	*(m_pSheet->m_pParamTab) = ParamTabTemp;

	return TRUE;
}
void CProtoPage::OnReset() 
{
	if ( m_hWnd == NULL )
		return;

	for ( int n = 0 ; n < CHECKOPTMAX + CHECKOPTEXT ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_DelayMsec = m_pSheet->m_pTextRam->m_DelayMSec;
	m_KeepAlive = m_pSheet->m_pTextRam->m_KeepAliveSec;
	ParamTabTemp = *(m_pSheet->m_pParamTab);

	UpdateData(FALSE);
	SetModified(FALSE);
}

void CProtoPage::OnSshAlgo() 
{
	ASSERT(m_pSheet);
	CAlgoDlg dlg;
	dlg.m_pData = &ParamTabTemp;
	dlg.m_EncShuffle = m_Check[CHECKOPTMAX + 0];
	dlg.m_MacShuffle = m_Check[CHECKOPTMAX + 1];
	if ( dlg.DoModal() != IDOK )
		return;
	SetModified(TRUE);
	m_Check[CHECKOPTMAX + 0] = dlg.m_EncShuffle;
	m_Check[CHECKOPTMAX + 1] = dlg.m_MacShuffle;
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
void CProtoPage::OnSshIdkey() 
{
	ASSERT(m_pSheet);
	CIdkeySelDLg dlg;
	dlg.m_pIdKeyTab = &(((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab);
	dlg.m_pParamTab = &ParamTabTemp;
	if ( dlg.DoModal() != IDOK )
		return;
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
void CProtoPage::OnSshPfd() 
{
	ASSERT(m_pSheet);
	CPfdListDlg dlg;
	dlg.m_pData = &ParamTabTemp;
	dlg.m_pEntry = m_pSheet->m_pEntry;
	dlg.m_X11PortFlag = (m_pSheet->m_pTextRam->IsOptEnable(TO_SSHX11PF) ? TRUE : FALSE);
	dlg.m_XDisplay = ParamTabTemp.m_XDisplay;
	dlg.DoModal();
	if ( !dlg.m_ModifiedFlag )
		return;
	if ( dlg.m_X11PortFlag )
		m_pSheet->m_pTextRam->EnableOption(TO_SSHX11PF);
	else
		m_pSheet->m_pTextRam->DisableOption(TO_SSHX11PF);
	ParamTabTemp.m_XDisplay = dlg.m_XDisplay;
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
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
