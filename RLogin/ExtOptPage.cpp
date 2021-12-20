// ExtOptPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "ExtOptPage.h"

// CExtOptPage ダイアログ

IMPLEMENT_DYNAMIC(CExtOptPage, CTreePage)

static const LV_COLUMN InitListTab[3] = {
	{ LVCF_TEXT | LVCF_WIDTH, 0,  60,	_T("No."),		0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("Set"),		0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("Reset"),	0, 0 },
};

//IDT_EXTOPT_LIST01
static const struct _OptListTab {
	int		num;
	int		nid;
} ExtListTab[] = {
	// RLogin SockOpt		1000-1511(0-511)
//	{	TO_RLCURIMD,	IDT_EXTOPT_LIST01	},
//	{	TO_RLRSPAST,	IDT_EXTOPT_LIST02	},
//	{	TO_RLGWDIS,		IDT_EXTOPT_LIST03	},
	{	TO_RLMWDIS,		IDT_EXTOPT_LIST04	},
	{	TO_RLGRPIND,	IDT_EXTOPT_LIST05	},
//	{	TO_RLSTAYCLIP,	IDT_EXTOPT_LIST06	},
//	{	TO_SETWINPOS,	IDT_EXTOPT_LIST07	},
	{	TO_RLWORDPP,	IDT_EXTOPT_LIST08	},
//	{	TO_RLRBSCROLL,	IDT_EXTOPT_LIST09	},
//	{	TO_RLEDITPAST,	IDT_EXTOPT_LIST10	},
//	{	TO_RLNTBCSEND,	IDT_EXTOPT_LIST11	},
//	{	TO_RLNTBCRECV,	IDT_EXTOPT_LIST12	},
//	{	TO_RLNOTCLOSE,	IDT_EXTOPT_LIST13	},
	{	TO_RLPAINWTAB,	IDT_EXTOPT_LIST14	},
//	{	TO_RLHIDPIFSZ,	IDT_EXTOPT_LIST15	},
//	{	TO_RLBACKHALF,	IDT_EXTOPT_LIST16	},
	{	TO_RLLOGFLUSH,	IDT_EXTOPT_LIST17	},
	{	TO_RLCARETANI,	IDT_EXTOPT_LIST18	},
//	{	TO_RLTABINFO,	IDT_EXTOPT_LIST19	},
//	{	TO_RLREOPEN,	IDT_EXTOPT_LIST20	},
//	{	TO_RLDELCRLF,	IDT_EXTOPT_LIST21	},
	{	TO_RLPSUPWIN,	IDT_EXTOPT_LIST22	},
	{	TO_RLDOSAVE,	IDT_EXTOPT_LIST23	},
	{	TO_RLNOCHKFONT,	IDT_EXTOPT_LIST24	},
//	{	TO_RLCOLEMOJI,	IDT_EXTOPT_LIST25	},
//	{	TO_RLNODELAY,	IDT_EXTOPT_LIST26	},
//	{	TO_IMECARET,	IDT_EXTOPT_LIST27	},
//	{	TO_DNSSSSHFP,	IDT_EXTOPT_LIST28	},
	{	0,				0					},
};

CExtOptPage::CExtOptPage() : CTreePage(CExtOptPage::IDD)
{
	m_InlineExt.Empty();
}
CExtOptPage::~CExtOptPage()
{
}

void CExtOptPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EXTLIST, m_ExtList);
	DDX_Text(pDX, IDC_EDIT1, m_InlineExt);
}

BEGIN_MESSAGE_MAP(CExtOptPage, CTreePage)
	ON_NOTIFY(NM_CLICK, IDC_EXTLIST, &CExtOptPage::OnNMClickExtlist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_EXTLIST, &CExtOptPage::OnLvnGetInfoTipExtlist)
	ON_EN_CHANGE(IDC_EDIT1, &CExtOptPage::OnUpdate)
END_MESSAGE_MAP()

void CExtOptPage::DoInit()
{
	int n, i;

	for ( i = 0 ; i < m_ExtList.GetItemCount() ; i++ ) {
		n = (int)m_ExtList.GetItemData(i);
		m_ExtList.SetLVCheck(i,  m_pSheet->m_pTextRam->IsOptEnable(ExtListTab[n].num) ? TRUE : FALSE);
	}

	m_pSheet->m_pTextRam->m_InlineExt.SetString(m_InlineExt, _T('|'));

	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CExtOptPage メッセージ ハンドラー

BOOL CExtOptPage::OnInitDialog() 
{
	int n;
	CString str;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	m_ExtList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_INFOTIP);
	m_ExtList.InitColumn(_T("ExtOptPageExt"), InitListTab, 3);

	m_ExtList.DeleteAllItems();
	for ( n = 0 ; ExtListTab[n].num != 0 ; n++ ) {
		CStringArrayExt param(ExtListTab[n].nid);

		str.Format(_T("%d"), ExtListTab[n].num - 1000);
		m_ExtList.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_ExtList.SetItemText(n, 1, param[0]);
		m_ExtList.SetItemText(n, 2, param[1]);
	}

	DoInit();

	return TRUE;
}
BOOL CExtOptPage::OnApply() 
{
	int n, i;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	for ( i = 0 ; i < m_ExtList.GetItemCount() ; i++ ) {
		n = (int)m_ExtList.GetItemData(i);

		m_pSheet->m_pTextRam->SetOption(ExtListTab[n].num, m_ExtList.GetLVCheck(i) ? TRUE : FALSE);
	}

	m_pSheet->m_pTextRam->m_InlineExt.GetString(m_InlineExt, _T('|'));

	return TRUE;
}
void CExtOptPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}

void CExtOptPage::OnNMClickExtlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if ( pNMItemActivate->iSubItem == 0 ) {
		SetModified(TRUE);
		m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
	}

	*pResult = 0;
}
void CExtOptPage::OnLvnGetInfoTipExtlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	int n = (int)m_ExtList.GetItemData(pGetInfoTip->iItem);
	CStringArrayExt param(ExtListTab[n].nid);
	CString str;

	if ( param.GetSize() >= 3 && !param[2].IsEmpty() )
		str = param[2]; 
	else
		str.Format(_T("%s/%s"), (LPCTSTR)param[0], (LPCTSTR)param[1]);

	if ( (n = str.GetLength() + sizeof(TCHAR)) > pGetInfoTip->cchTextMax )
		n = pGetInfoTip->cchTextMax;

	lstrcpyn(pGetInfoTip->pszText, str, n);

	*pResult = 0;
}
void CExtOptPage::OnUpdate()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
