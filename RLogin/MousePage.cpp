// MousePage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "MousePage.h"

/////////////////////////////////////////////////////////////////////////////
// CMousePage ダイアログ

IMPLEMENT_DYNAMIC(CMousePage, CTreePage)

#define	CHECKOPTMAX		4
#define	IDC_CHECKFAST	IDC_OPTCHECK1
static const int CheckOptTab[] = { TO_RLMSWAPP, TO_RLMOSWHL, TO_RLCURIMD, TO_RLRBSCROLL };

CMousePage::CMousePage() : CTreePage(CMousePage::IDD)
{
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_WheelSize = _T("");
	m_MouseMode[0] = 0;
	m_MouseMode[1] = 1;
	m_MouseMode[2] = 0;
	m_MouseMode[3] = 2;
	m_DropMode = 0;
	m_CmdWork = _T("");
	m_MouseOnTab = 0;
	m_UrlOpt = _T("#MOUSEOPT");
}

CMousePage::~CMousePage()
{
}

void CMousePage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_CHECKFAST + n, m_Check[n]);
	DDX_CBStringExact(pDX, IDC_WHEELSIZE, m_WheelSize);
	DDX_CBIndex(pDX, IDC_MOUSE_BTN1, m_MouseMode[0]);
	DDX_CBIndex(pDX, IDC_MOUSE_BTN2, m_MouseMode[1]);
	DDX_CBIndex(pDX, IDC_MOUSE_KEY1, m_MouseMode[2]);
	DDX_CBIndex(pDX, IDC_MOUSE_KEY2, m_MouseMode[3]);
	DDX_CBIndex(pDX, IDC_COMBO1, m_DropMode);
	DDX_Text(pDX, IDC_EDIT1, m_CmdWork);
	DDX_CBIndex(pDX, IDC_COMBO2, m_MouseOnTab);
}

BEGIN_MESSAGE_MAP(CMousePage, CTreePage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_CBN_EDITCHANGE(IDC_WHEELSIZE, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_WHEELSIZE,	 OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_BTN1, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_BTN2, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_KEY1, OnUpdateEdit)
	ON_CBN_SELCHANGE(IDC_MOUSE_KEY2, OnUpdateEdit)
	ON_EN_CHANGE(IDC_EDIT1, OnUpdateChangeDropMode)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeDropMode)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnUpdateEdit)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMousePage メッセージ ハンドラー

// Mouse Event Tracking
// Left			xxxx xx00	1	// Right		xxxx xx01	2
// Middle		xxxx xx10	3	// Button 1		x1xx xx00	4
// Button 2		x1xx xx01	5	// Button 3		x1xx xx10	6
// Release		xxxx xx11
// shift		xxxx x1xx		// meta			xxxx 1xxx
// ctrl			xxx1 xxxx

static const BYTE MouseModeTab[] = {
	0x00,	0x01,	0x02,	0x40,	0x41,	0x42,
	0x04,	0x08,	0x10,
};

void CMousePage::DoInit()
{
	int n;

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_WheelSize.Format(_T("%d"), m_pSheet->m_pTextRam->m_WheelSize);

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( MouseModeTab[n] == m_pSheet->m_pTextRam->m_MouseMode[0] )
			m_MouseMode[0] = n;
		if ( MouseModeTab[n] == m_pSheet->m_pTextRam->m_MouseMode[1] )
			m_MouseMode[1] = n;
	}
	for ( n = 0 ; n < 3 ; n++ ) {
		if ( MouseModeTab[n + 6] == m_pSheet->m_pTextRam->m_MouseMode[2] )
			m_MouseMode[2] = n;
		if ( MouseModeTab[n + 6] == m_pSheet->m_pTextRam->m_MouseMode[3] )
			m_MouseMode[3] = n;
	}

	m_DropMode = m_pSheet->m_pTextRam->m_DropFileMode;

	for ( n = 0 ; n < 8 ; n++ )
		m_DropCmd[n] = m_pSheet->m_pTextRam->m_DropFileCmd[n];

	m_CmdWork  = m_DropCmd[m_DropMode];

	m_MouseOnTab = (m_pSheet->m_pTextRam->IsOptEnable(TO_RLGWDIS)   ? 0 : 1) |
				   (m_pSheet->m_pTextRam->IsOptEnable(TO_RLTABINFO) ? 2 : 0);

	UpdateData(FALSE);
}

BOOL CMousePage::OnInitDialog()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	DoInit();

	SubclassComboBox(IDC_WHEELSIZE);

	return TRUE;
}

BOOL CMousePage::OnApply()
{
	int n;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	m_pSheet->m_pTextRam->m_WheelSize    = _tstoi(m_WheelSize);

	m_pSheet->m_pTextRam->m_MouseMode[0] = MouseModeTab[m_MouseMode[0]];
	m_pSheet->m_pTextRam->m_MouseMode[1] = MouseModeTab[m_MouseMode[1]];
	m_pSheet->m_pTextRam->m_MouseMode[2] = MouseModeTab[m_MouseMode[2] + 6];
	m_pSheet->m_pTextRam->m_MouseMode[3] = MouseModeTab[m_MouseMode[3] + 6];
	
	m_pSheet->m_pTextRam->m_DropFileMode = m_DropMode;

	for ( n = 0 ; n < 8 ; n++ )
		m_pSheet->m_pTextRam->m_DropFileCmd[n] = m_DropCmd[n];

	m_pSheet->m_pTextRam->SetOption(TO_RLGWDIS,   (m_MouseOnTab & 1) != 0 ? FALSE : TRUE);
	m_pSheet->m_pTextRam->SetOption(TO_RLTABINFO, (m_MouseOnTab & 2) != 0 ? TRUE : FALSE);

	return TRUE;
}

void CMousePage::OnReset()
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}

void CMousePage::OnUpdateCheck(UINT nID) 
{
	int n = nID - IDC_CHECKFAST;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;

	if ( n >= 0 && n < CHECKOPTMAX && CheckOptTab[n] < 1000 )
		m_pSheet->m_ModFlag |= UMOD_ANSIOPT;
}

void CMousePage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CMousePage::OnUpdateChangeDropMode()
{
	UpdateData(TRUE);
	m_DropCmd[m_DropMode] = m_CmdWork;
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CMousePage::OnCbnSelchangeDropMode()
{
	UpdateData(TRUE);
	m_CmdWork = m_DropCmd[m_DropMode];
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
