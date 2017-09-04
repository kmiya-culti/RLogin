// TermPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "TermPage.h"
#include "EscDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTermPage プロパティ ページ

IMPLEMENT_DYNCREATE(CTermPage, CTreePage)

static const LV_COLUMN InitListTab[3] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60,	_T("No."),		0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("Set"),		0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("Reset"),	0, 0 },
	};

//IDT_TERM_LIST01
static const struct _OptListTab {
	int		num;
	int		nid;
} OptListTab[] = {
	// ANSI Screen Option	0-99(200-299)
	{	TO_ANSIKAM,		IDT_TERM_LIST01	},
	{	TO_ANSIIRM,		IDT_TERM_LIST02	},
	{	TO_ANSIERM,		IDT_TERM_LIST03	},
	{	TO_ANSISRM,		IDT_TERM_LIST04	},
	{	TO_ANSITSM,		IDT_TERM_LIST05	},
	{	TO_ANSILNM,		IDT_TERM_LIST06	},

	// DEC Terminal Option	0-199
	{	TO_DECCKM,		IDT_TERM_LIST07	},
	{	TO_DECANM,		IDT_TERM_LIST08	},
	{	TO_DECCOLM,		IDT_TERM_LIST09	},
	{	TO_DECSCLM,		IDT_TERM_LIST10	},
	{	TO_DECSCNM,		IDT_TERM_LIST11	},
	{	TO_DECOM,		IDT_TERM_LIST12	},
	{	TO_DECAWM,		IDT_TERM_LIST13	},
	{	TO_DECARM,		IDT_TERM_LIST14	},
	{	TO_XTMOSREP,	IDT_TERM_LIST15	},
	{	TO_XTCBLINK,	IDT_TERM_LIST16	},
	{	TO_DECPEX,		IDT_TERM_LIST17	},
	{	TO_DECTCEM,		IDT_TERM_LIST18	},
	{	TO_DECTEK,		IDT_TERM_LIST19	},
	{	TO_XTMCSC,		IDT_TERM_LIST20	},
	{	TO_XTMCUS,		IDT_TERM_LIST21	},
	{	TO_XTMRVW,		IDT_TERM_LIST22	},
	{	TO_XTMABUF,		IDT_TERM_LIST23	},
	{	TO_DECNKM,		IDT_TERM_LIST51	},		// TO_DECNKM == TO_RLPNAM
	{	TO_DECBKM,		IDT_TERM_LIST24	},
	{	TO_DECLRMM,		IDT_TERM_LIST25	},
	{	TO_DECSDM,		IDT_TERM_LIST26	},
	{	TO_DECNCSM,		IDT_TERM_LIST27	},
	{	TO_DECECM,		IDT_TERM_LIST28	},

	// XTerm Option			1000-1099(300-379)
	{	TO_XTNOMTRK,	IDT_TERM_LIST29	},
	{	TO_XTHILTRK,	IDT_TERM_LIST30	},
	{	TO_XTBEVTRK,	IDT_TERM_LIST31	},
	{	TO_XTAEVTRK,	IDT_TERM_LIST32	},
	{	TO_XTFOCEVT,	IDT_TERM_LIST33	},
	{	TO_XTEXTMOS,	IDT_TERM_LIST34	},
	{	TO_XTSGRMOS,	IDT_TERM_LIST35	},
	{	TO_XTURXMOS,	IDT_TERM_LIST36	},
	{	TO_XTALTSCR,	IDT_TERM_LIST37	},
	{	TO_XTSRCUR,		IDT_TERM_LIST38	},
	{	TO_XTALTCLR,	IDT_TERM_LIST39	},
	{	TO_XTPRICOL,	IDT_TERM_LIST40	},

	// XTerm Option 2		2000-2019(380-399)
	{	TO_XTBRPAMD,	IDT_TERM_LIST41	},

	// RLogin Option		8400-8511(400-511)
	{	TO_RLGCWA,		IDT_TERM_LIST42	},
	{	TO_RLGNDW,		IDT_TERM_LIST43	},
	{	TO_RLGAWL,		0 /* IDT_TERM_LIST44 */	},
	{	TO_RLBOLD,		IDT_TERM_LIST45	},
	{	TO_RLBPLUS,		0 /* IDT_TERM_LIST46 */	},
	{	TO_RLALTBDIS,	IDT_TERM_LIST47	},
	{	TO_RLUNIAWH,	IDT_TERM_LIST48	},
	{	TO_RLNORESZ,	0 /* IDT_TERM_LIST49 */	},
	{	TO_RLKANAUTO,	IDT_TERM_LIST50	},
	{	TO_RLPNAM,		IDT_TERM_LIST51	},
	{	TO_IMECTRL,		IDT_TERM_LIST52	},
	{	TO_RLCKMESC,	IDT_TERM_LIST53	},
	{	TO_RLMSWAPE,	0 /* IDT_TERM_LIST54 */	},
	{	TO_RLTEKINWND,	IDT_TERM_LIST55	},
	{	TO_RLUNINOM,	IDT_TERM_LIST56	},
	{	TO_RLUNIAHF,	IDT_TERM_LIST57	},
	{	TO_RLMODKEY,	0 /* IDT_TERM_LIST58 */	},
	{	TO_RLDRWLINE,	IDT_TERM_LIST59	},
	{	TO_RLSIXPOS,	IDT_TERM_LIST60	},
	{	TO_DRCSMMv1,	IDT_TERM_LIST61	},
	{	TO_RLC1DIS,		IDT_TERM_LIST62	},
	{	TO_RLCLSBACK,	IDT_TERM_LIST63	},
	{	TO_RLBRKMBCS,	IDT_TERM_LIST64	},
	{	TO_TTCTH,		IDT_TERM_LIST65	},
	{	TO_RLBOLDHC,	IDT_TERM_LIST66	},
	{	0,				0				}
};

CTermPage::CTermPage() : CTreePage(CTermPage::IDD)
{
}
CTermPage::~CTermPage()
{
}

void CTermPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_ESCLIST, m_OptList);
}

BEGIN_MESSAGE_MAP(CTermPage, CTreePage)
	ON_BN_CLICKED(IDC_ESCEDIT, &CTermPage::OnBnClickedEscedit)
	ON_NOTIFY(NM_CLICK, IDC_ESCLIST, &CTermPage::OnNMClickOptlist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_ESCLIST, &CTermPage::OnLvnGetInfoTipEsclist)
END_MESSAGE_MAP()

void CTermPage::DoInit()
{
	int n, i;

	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	for ( i = 0 ; i < m_OptList.GetItemCount() ; i++ ) {
		n = (int)m_OptList.GetItemData(i);
		m_OptList.SetLVCheck(i,  m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) ? TRUE : FALSE);
	}

	UpdateData(FALSE);
}

int CTermPage::IsSupport(int opt)
{
	// 0	Not Support
	// 1	Support Option Set
	// 2	... Reset
	// 3	Permanently Option Set
	// 4	... Reset

	for ( int n = 0 ; OptListTab[n].num != 0 ; n++ ) {
		if ( OptListTab[n].num == opt )
			return (OptListTab[n].nid != 0 ? 1 : 3);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CTermPage メッセージ ハンドラ

BOOL CTermPage::OnInitDialog() 
{
	int n, i;
	CString str;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	m_OptList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_INFOTIP);
	m_OptList.InitColumn(_T("TermPageOpt"), InitListTab, 3);

	m_OptList.DeleteAllItems();
	for ( n = i = 0 ; OptListTab[n].num != 0 ; n++ ) {
		if ( OptListTab[n].nid == 0 )
			continue;
		CStringArrayExt param(OptListTab[n].nid);
		CTextRam::OptionString(OptListTab[n].num, str);
		m_OptList.InsertItem(LVIF_TEXT | LVIF_PARAM, i, str, 0, 0, 0, n);
		m_OptList.SetItemText(i, 1, param[0]);
		m_OptList.SetItemText(i, 2, param[1]);
		i++;
	}

	DoInit();

	return TRUE;
}
BOOL CTermPage::OnApply() 
{
	int n, i;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	m_pSheet->m_pTextRam->m_ProcTab = m_ProcTab;

	for ( i = 0 ; i < m_OptList.GetItemCount() ; i++ ) {
		n = (int)m_OptList.GetItemData(i);

		if ( m_OptList.GetLVCheck(i) ) {
			if ( m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) )
				continue;
			if ( !m_pSheet->m_pTextRam->IsInitText() || m_pSheet->m_pTextRam->m_pDocument == NULL ) {
				m_pSheet->m_pTextRam->EnableOption(OptListTab[n].num);
				continue;
			}
			m_pSheet->m_pTextRam->m_AnsiPara.RemoveAll();
			if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
				m_pSheet->m_pTextRam->EnableOption(OptListTab[n].num);
			} else if ( OptListTab[n].num < 380 ) {		// XTerm Option			1000-1079(300-379)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 700);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 400 ) {		// XTerm Option 2		2000-2019(380-399)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 1620);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		8400-8511(400-511)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 8000);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			}
		} else {
			if ( !m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) )
				continue;
			if ( !m_pSheet->m_pTextRam->IsInitText() || m_pSheet->m_pTextRam->m_pDocument == NULL ) {
				m_pSheet->m_pTextRam->DisableOption(OptListTab[n].num);
				continue;
			}
			m_pSheet->m_pTextRam->m_AnsiPara.RemoveAll();
			if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
				m_pSheet->m_pTextRam->DisableOption(OptListTab[n].num);
			} else if ( OptListTab[n].num < 380 ) {		// XTerm Option			1000-1079(300-379)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 700);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 400 ) {		// XTerm Option 2		2000-2019(380-399)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 1620);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		8400-8511(400-511)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 8000);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			}
		}
	}

	return TRUE;
}
void CTermPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}

void CTermPage::OnBnClickedEscedit()
{
	CEscDlg dlg;

	dlg.m_pTextRam = m_pSheet->m_pTextRam;
	dlg.m_pProcTab = &m_ProcTab;

	if ( dlg.DoModal() != IDOK )
		return;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CTermPage::OnNMClickOptlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if ( pNMItemActivate->iSubItem == 0 ) {
		SetModified(TRUE);
		m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_ANSIOPT);
	}

	*pResult = 0;
}
void CTermPage::OnLvnGetInfoTipEsclist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	int n = (int)m_OptList.GetItemData(pGetInfoTip->iItem);
	CStringArrayExt param(OptListTab[n].nid);
	CString str;

	if ( param.GetSize() >= 3 && !param[2].IsEmpty() )
		str = param[2];
	else
		str.Format(_T("%s/%s"), param[0], param[1]);

	if ( (n = str.GetLength() + sizeof(TCHAR)) > pGetInfoTip->cchTextMax )
		n = pGetInfoTip->cchTextMax;

	lstrcpyn(pGetInfoTip->pszText, str, n);

	*pResult = 0;
}
