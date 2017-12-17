// ModKeyPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "OPtDlg.h"
#include "TextRam.h"
#include "Data.h"
#include "ModKeyPage.h"

// CModKeyPage ダイアログ

IMPLEMENT_DYNAMIC(CModKeyPage, CTreePage)

CModKeyPage::CModKeyPage() : CTreePage(CModKeyPage::IDD)
{
	for ( int n = 0 ; n < 5 ; n++ ) {
		m_KeyList[n].Empty();
		m_ModType[n] = 0;
	}
	m_bOverShortCut = FALSE;
}

CModKeyPage::~CModKeyPage()
{
}

void CModKeyPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);
	for ( int n = 0 ; n < 5 ; n++ ) {
		DDX_Text(pDX, IDC_KEYCODE1 + n, m_KeyList[n]);
		DDX_CBIndex(pDX, IDC_MODCODE1 + n, m_ModType[n]);
	}
	DDX_Check(pDX, IDC_CHECK1, m_bOverShortCut);
}

BEGIN_MESSAGE_MAP(CModKeyPage, CTreePage)
	ON_CONTROL_RANGE(EN_CHANGE, IDC_KEYCODE1, IDC_KEYCODE5, &CModKeyPage::OnEnChangeKeycode)
	ON_CONTROL_RANGE(CBN_SELCHANGE, IDC_MODCODE1, IDC_MODCODE5, &CModKeyPage::OnCbnSelchangeModcode)
	ON_BN_CLICKED(IDC_CHECK1, &CModKeyPage::OnBnClickedCheck1)
END_MESSAGE_MAP()

// CModKeyPage メッセージ ハンドラー

void CModKeyPage::DoInit()
{
	for ( int n = 0 ; n < 5 ; n++ ) {
		m_KeyList[n]    = m_pSheet->m_pTextRam->m_ModKeyList[n + 1];
		m_ModType[n]    = m_pSheet->m_pTextRam->m_ModKey[n + 1] + 1;
	}
	m_bOverShortCut = m_pSheet->m_pTextRam->IsOptEnable(TO_RLMODKEY);

	UpdateData(FALSE);
}
BOOL CModKeyPage::OnInitDialog()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	DoInit();

	return TRUE;
}

BOOL CModKeyPage::OnApply()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	for ( int n = 0 ; n < 5 ; n++ ) {
		m_pSheet->m_pTextRam->m_ModKey[n + 1]     = m_ModType[n] - 1;
		m_pSheet->m_pTextRam->m_ModKeyList[n + 1] = m_KeyList[n];
	}

	m_pSheet->m_pTextRam->InitModKeyTab();
	m_pSheet->m_pTextRam->SetOption(TO_RLMODKEY, m_bOverShortCut);

	return TRUE;
}
void CModKeyPage::OnReset()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}

void CModKeyPage::OnEnChangeKeycode(UINT nID)
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_MODKEY);
}
void CModKeyPage::OnCbnSelchangeModcode(UINT nID)
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_MODKEY);
}
void CModKeyPage::OnBnClickedCheck1()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_ANSIOPT);
}
