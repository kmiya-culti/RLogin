// ExtOptPage.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "ExtOptPage.h"
#include "InitAllDlg.h"

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
//	{	TO_RLNOCHKFONT,	IDT_EXTOPT_LIST24	},
//	{	TO_RLCOLEMOJI,	IDT_EXTOPT_LIST25	},
//	{	TO_RLNODELAY,	IDT_EXTOPT_LIST26	},
//	{	TO_IMECARET,	IDT_EXTOPT_LIST27	},
//	{	TO_DNSSSSHFP,	IDT_EXTOPT_LIST28	},
//	{	TO_RLENOEDPAST,	IDT_EXTOPT_LIST29	},
//	{	TO_RLTABGRAD,	IDT_EXTOPT_LIST30	},
//	{	TO_RLDELYEMOJI,	IDT_EXTOPT_LIST31	},
	{	0,				0					},
};

CExtOptPage::CExtOptPage() : CTreePage(CExtOptPage::IDD)
{
	m_InlineExt.Empty();
	m_UrlOpt = _T("#EXTOPT");
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
	ON_WM_DRAWITEM()
	ON_NOTIFY(NM_DBLCLK, IDC_EXTLIST, &CExtOptPage::OnNMDblclkExtlist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EXTLIST, &CExtOptPage::OnLvnItemchangedExtlist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_EXTLIST, &CExtOptPage::OnLvnGetInfoTipExtlist)
	ON_EN_CHANGE(IDC_EDIT1, &CExtOptPage::OnUpdate)
	ON_COMMAND(ID_EDIT_DELALL, &CExtOptPage::OnEditDelall)
	ON_COMMAND(ID_EDIT_COPY, &CExtOptPage::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &CExtOptPage::OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CExtOptPage::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CExtOptPage::OnUpdateEditPaste)
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

static const INITDLGTAB ItemTab[] = {
	{ IDC_EXTLIST,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_BTM_BTM },
	{ IDC_EDIT1,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_TITLE1,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_BTM_BTM },
	{ IDC_TITLE2,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_TITLE3,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_BTM_TOP },
	{ IDC_TITLE4,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_BTM | ITM_BTM_BTM },

	{ 0,				0 },
};

/////////////////////////////////////////////////////////////////////////////
// CExtOptPage メッセージ ハンドラー

BOOL CExtOptPage::OnInitDialog() 
{
	int n;
	CString str;

	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	InitItemOffset(ItemTab);

	m_ExtList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_INFOTIP);
	m_ExtList.InitColumn(_T("ExtOptPageExt"), InitListTab, 3);
	m_ExtList.SetPopUpMenu(IDR_POPUPMENU, 12);

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

void CExtOptPage::OnNMDblclkExtlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	BOOL sw = m_ExtList.GetLVCheck(pNMItemActivate->iItem);

	m_ExtList.SetLVCheck(pNMItemActivate->iItem, sw ? FALSE : TRUE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;

	*pResult = 0;
}
void CExtOptPage::OnLvnItemchangedExtlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	int change = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( (pNMLV->uNewState & 0x2000) != 0 && (pNMLV->uOldState & 0x1000) != 0 )			// LVCheck OFF -> ON
		change = 1;
	else if ( (pNMLV->uNewState & 0x1000) != 0 && (pNMLV->uOldState & 0x2000) != 0 )	// LVCheck ON -> OFF
		change = 2;

	if ( !m_ExtList.m_bSetLVCheck && change != 0 ) {
		if ( m_ExtList.GetItemState(pNMLV->iItem, LVIS_SELECTED) != 0 ) {
			for ( int n = 0 ; n < m_ExtList.GetItemCount() ; n++ ) {
				if ( n == pNMLV->iItem || m_ExtList.GetItemState(n, LVIS_SELECTED) == 0 )
					continue;
				m_ExtList.SetLVCheck(n, change == 1 ? TRUE : FALSE);
			}
		}

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

	if ( lstrcpyn(pGetInfoTip->pszText, str, n) == NULL )
		pGetInfoTip->pszText[0] = L'\0';

	*pResult = 0;
}

void CExtOptPage::OnUpdate()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CExtOptPage::OnEditDelall()
{
	int n, i;
	CInitAllDlg dlg;
	CTextRam *pTextRam = new CTextRam;

	dlg.m_Title.LoadString(IDS_INITEXTOPTTITLE);

	if ( dlg.DoModal() != IDOK )
		return;

	switch(dlg.m_InitType) {
	case 0:		// Init Default Entry
		pTextRam->Serialize(FALSE);
		break;

	case 1:		// Init Program Default
		pTextRam->Init();
		break;

	case 2:		// Copy Entry option
		ASSERT(dlg.m_pInitEntry != NULL);
		{
			CBuffer tmp(dlg.m_pInitEntry->m_ProBuffer.GetPtr(), dlg.m_pInitEntry->m_ProBuffer.GetSize());
			pTextRam->Serialize(FALSE, tmp);
		}
		break;
	}

	for ( i = 0 ; i < m_ExtList.GetItemCount() ; i++ ) {
		n = (int)m_ExtList.GetItemData(i);
		m_ExtList.SetLVCheck(i,  pTextRam->IsOptEnable(ExtListTab[n].num) ? TRUE : FALSE);
	}

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;

	delete pTextRam;
}
void CExtOptPage::OnEditCopy()
{
	int n;
	CString str, text;

	for ( n = 0 ; ExtListTab[n].num != 0 ; n++ ) {
		if ( ExtListTab[n].nid == 0 )
			continue;
		CStringArrayExt param(ExtListTab[n].nid);
		str.Format(_T("%d"), ExtListTab[n].num - 1000);

		text += (m_ExtList.GetLVCheck(m_ExtList.GetParamItem(n)) ? _T("Set") : _T("")); text += _T("\t");
		text += str; text += _T("\t");
		text += param[0]; text += _T("\t");
		text += param[1]; text += _T("\r\n");
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(text);
}
void CExtOptPage::OnEditPaste()
{
	LPCTSTR p;
	CString text, line;
	CStringArrayExt param;

	if ( !((CMainFrame *)::AfxGetMainWnd())->CopyClipboardData(text) )
		return;

	for ( p = text ; ; p++ ) {
		if ( *p == _T('\0') || *p == _T('\n') ) {
			param.GetString(line, _T('\t'));
			if ( param.GetSize() >= 4 ) {
				LPCTSTR s = param[1];
				int n, num = (-1);
				while ( *s != _T('\0') && *s <= _T(' ') )
					s++;
				if ( (num = _tstoi(s)) >= 0 || num <= 511 )			// RLogin SockOpt		1000-1511(0-511)
					num += 1000;
				else
					num = (-1);
				for ( n = 0 ; ExtListTab[n].num != 0 ; n++ ) {
					if ( ExtListTab[n].num == num ) {
						m_ExtList.SetLVCheck(m_ExtList.GetParamItem(n), param[0].CompareNoCase(_T("set")) == 0 ? TRUE : FALSE);
						break;
					}
				}
			}
			line.Empty();
			if ( *p == _T('\0') )
				break;
		} else if ( *p != '\r' )
			line += *p;
	}

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CExtOptPage::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}
void CExtOptPage::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardFormatAvailable(CF_UNICODETEXT) ? TRUE : FALSE);
}
