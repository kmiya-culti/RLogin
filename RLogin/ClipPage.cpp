// ClipPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "OPtDlg.h"
#include "TextRam.h"
#include "Data.h"
#include "ClipPage.h"

/////////////////////////////////////////////////////////////////////////////
// CClipPage ダイアログ

IMPLEMENT_DYNAMIC(CClipPage, CTreePage)

#define	CHECKOPTMAX		5
#define	IDC_CHECKFAST	IDC_OPTCHECK1
static const int CheckOptTab[] = { TO_RLCKCOPY, TO_RLSTAYCLIP, TO_RLSPCTAB, TO_RLGAWL, TO_RLDELCRLF };

CClipPage::CClipPage() : CTreePage(CClipPage::IDD)
{
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_WordStr = _T("");
	m_ClipFlag[0] = FALSE;
	m_ClipFlag[1] = FALSE;
	m_ShellStr = _T("http://|ftp://");
	m_RClick = 0;
	m_ClipCrLf = 0;
	m_PastEdit = 1;
	m_UrlOpt = _T("#CLIPOPT");
}
CClipPage::~CClipPage()
{
}

void CClipPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_CHECKFAST + n, m_Check[n]);

	DDX_Text(pDX, IDC_EDIT3, m_WordStr);
	DDX_Check(pDX, IDC_CHECK1, m_ClipFlag[0]);
	DDX_Check(pDX, IDC_CHECK2, m_ClipFlag[1]);
	DDX_Text(pDX, IDC_EDIT4, m_ShellStr);
	DDX_Radio(pDX, IDC_RADIO1, m_RClick);
	DDX_CBIndex(pDX, IDC_COMBO1, m_ClipCrLf);
	DDX_CBIndex(pDX, IDC_COMBO2, m_RtfMode);
	DDX_Radio(pDX, IDC_RADIO4, m_PastEdit);
}

BEGIN_MESSAGE_MAP(CClipPage, CTreePage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, &CClipPage::OnUpdateCheck)
	ON_EN_CHANGE(IDC_EDIT3, &CClipPage::OnUpdateChange)
	ON_EN_CHANGE(IDC_EDIT4, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_CHECK1, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_CHECK2, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_RADIO1, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_RADIO2, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_RADIO3, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_RADIO4, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_RADIO5, &CClipPage::OnUpdateChange)
	ON_BN_CLICKED(IDC_RADIO6, &CClipPage::OnUpdateChange)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CClipPage::OnUpdateChange)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CClipPage::OnUpdateChange)
END_MESSAGE_MAP()

// CClipPage メッセージ ハンドラー

void CClipPage::DoInit()
{
	int n;

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_WordStr = m_pSheet->m_pTextRam->m_WordStr;

	m_ClipFlag[0] = (m_pSheet->m_pTextRam->m_ClipFlag & OSC52_READ)  != 0 ? TRUE : FALSE;
	m_ClipFlag[1] = (m_pSheet->m_pTextRam->m_ClipFlag & OSC52_WRITE) != 0 ? TRUE : FALSE;

	m_ClipCrLf = m_pSheet->m_pTextRam->m_ClipCrLf;
	m_RtfMode  = m_pSheet->m_pTextRam->m_RtfMode;

	m_pSheet->m_pTextRam->m_ShellExec.SetString(m_ShellStr, _T('|'));

	if ( m_pSheet->m_pTextRam->IsOptEnable(TO_RLRSPAST) )
		m_RClick = 2;
	else if ( m_pSheet->m_pTextRam->IsOptEnable(TO_RLRCLICK) )
		m_RClick = 1;
	else
		m_RClick = 0;

	if ( m_pSheet->m_pTextRam->IsOptEnable(TO_RLEDITPAST) )
		m_PastEdit = 2;
	else if ( m_pSheet->m_pTextRam->IsOptEnable(TO_RLENOEDPAST) )
		m_PastEdit = 0;
	else
		m_PastEdit = 1;

	UpdateData(FALSE);
}
BOOL CClipPage::OnInitDialog()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();
	DoInit();

	return TRUE;
}

BOOL CClipPage::OnApply()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	m_pSheet->m_pTextRam->m_WordStr = m_WordStr;

	m_pSheet->m_pTextRam->m_ClipFlag  = 0;
	m_pSheet->m_pTextRam->m_ClipFlag |= (m_ClipFlag[0] ? OSC52_READ  : 0);
	m_pSheet->m_pTextRam->m_ClipFlag |= (m_ClipFlag[1] ? OSC52_WRITE : 0);

	m_pSheet->m_pTextRam->m_ClipCrLf = m_ClipCrLf;
	m_pSheet->m_pTextRam->m_RtfMode  = m_RtfMode;

	m_pSheet->m_pTextRam->m_ShellExec.GetString(m_ShellStr, _T('|'));

	switch(m_RClick) {
	case 0:
		m_pSheet->m_pTextRam->DisableOption(TO_RLRSPAST);
		m_pSheet->m_pTextRam->DisableOption(TO_RLRCLICK);
		break;
	case 1:
		m_pSheet->m_pTextRam->DisableOption(TO_RLRSPAST);
		m_pSheet->m_pTextRam->EnableOption(TO_RLRCLICK);
		break;
	case 2:
		m_pSheet->m_pTextRam->EnableOption(TO_RLRSPAST);
		m_pSheet->m_pTextRam->DisableOption(TO_RLRCLICK);
		break;
	}

	switch(m_PastEdit) {
	case 0:
		m_pSheet->m_pTextRam->DisableOption(TO_RLEDITPAST);
		m_pSheet->m_pTextRam->EnableOption(TO_RLENOEDPAST);
		break;
	case 1:
		m_pSheet->m_pTextRam->DisableOption(TO_RLEDITPAST);
		m_pSheet->m_pTextRam->DisableOption(TO_RLENOEDPAST);
		break;
	case 2:
		m_pSheet->m_pTextRam->EnableOption(TO_RLEDITPAST);
		m_pSheet->m_pTextRam->DisableOption(TO_RLENOEDPAST);
		break;
	}

	return CTreePage::OnApply();
}

void CClipPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}

void CClipPage::OnUpdateCheck(UINT nID) 
{
	int n = nID - IDC_CHECKFAST;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;

	if ( n >= 0 && n < CHECKOPTMAX && CheckOptTab[n] < 1000 )
		m_pSheet->m_ModFlag |= UMOD_ANSIOPT;
}

void CClipPage::OnUpdateChange()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
