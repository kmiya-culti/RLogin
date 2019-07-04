// SockOptPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "SockOptPage.h"

// CSockOptPage ダイアログ

IMPLEMENT_DYNAMIC(CSockOptPage, CTreePage)

#define	CHECKOPTMAX		13
#define	IDC_CHECKFAST	IDC_OPTCHECK1
static const int CheckOptTab[] = {
	TO_RLDELAY,		TO_RLBPLUS,		TO_RLDSECHO,	TO_RLPOFF,		TO_RLKEEPAL, 
	TO_SLEEPTIMER,	TO_RLGROUPCAST,	TO_RLNTBCRECV,	TO_RLNTBCSEND,	TO_RLNOTCLOSE, 
	TO_RLREOPEN,	TO_RLTRSLIMIT,	TO_RLNODELAY,
};

CSockOptPage::CSockOptPage() : CTreePage(CSockOptPage::IDD)
{
	m_DelayMsec = 0;
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_SelIPver = 0;
	m_SleepTime = 10;
	m_TransmitLimit = 1000;
}

CSockOptPage::~CSockOptPage()
{
}

void CSockOptPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_DELAYMSEC, m_DelayMsec);
	DDV_MinMaxUInt(pDX, m_DelayMsec, 0, 3000);
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_CHECKFAST + n, m_Check[n]);
	DDX_Radio(pDX, IDC_RADIO1, m_SelIPver);
	DDX_Text(pDX, IDC_SLEEPTIME, m_SleepTime);
	DDX_Text(pDX, IDC_GROUPCAST, m_GroupCast);
	DDX_Text(pDX, IDC_TRANSMITLIMIT, m_TransmitLimit);
}


BEGIN_MESSAGE_MAP(CSockOptPage, CTreePage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO3, OnUpdateCheck)
	ON_EN_CHANGE(IDC_DELAYMSEC, OnUpdateEdit)
	ON_EN_CHANGE(IDC_SLEEPTIME, OnUpdateEdit)
	ON_EN_CHANGE(IDC_GROUPCAST, OnUpdateEdit)
	ON_EN_CHANGE(IDC_TRANSMITLIMIT, OnUpdateEdit)
END_MESSAGE_MAP()


// CSockOptPage メッセージ ハンドラー

void CSockOptPage::DoInit()
{
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_DelayMsec     = m_pSheet->m_pTextRam->m_DelayMSec;
	m_SelIPver      = m_pSheet->m_pParamTab->m_SelIPver;
	m_SleepTime     = m_pSheet->m_pTextRam->m_SleepMax / 6;
	m_GroupCast     = m_pSheet->m_pTextRam->m_GroupCast;
	m_TransmitLimit = m_pSheet->m_pParamTab->m_TransmitLimit;

	UpdateData(FALSE);
}
BOOL CSockOptPage::OnInitDialog()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL && m_pSheet->m_pParamTab != NULL);

	CTreePage::OnInitDialog();

	DoInit();

	return TRUE;
}
BOOL CSockOptPage::OnApply()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL && m_pSheet->m_pParamTab != NULL);

	UpdateData(TRUE);

	if ( m_SleepTime <= 0 )
		m_SleepTime = 1;

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	m_pSheet->m_pTextRam->m_DelayMSec   = m_DelayMsec;
	m_pSheet->m_pParamTab->m_SelIPver   = m_SelIPver;
	m_pSheet->m_pTextRam->m_SleepMax    = m_SleepTime * 6;
	m_pSheet->m_pTextRam->m_GroupCast   = m_GroupCast;
	m_pSheet->m_pParamTab->m_TransmitLimit = m_TransmitLimit;

	return TRUE;
}
void CSockOptPage::OnReset()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL && m_pSheet->m_pParamTab != NULL);

	DoInit();
	SetModified(FALSE);
}

void CSockOptPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);
}
void CSockOptPage::OnUpdateCheck(UINT nID) 
{
	int n = nID - IDC_CHECKFAST;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_PARAMTAB);

	if ( n >= 0 && n < CHECKOPTMAX && CheckOptTab[n] < 1000 )
		m_pSheet->m_ModFlag |= UMOD_ANSIOPT;
}
