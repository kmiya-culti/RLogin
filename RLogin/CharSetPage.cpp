// CharSetPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "OPtDlg.h"
#include "TextRam.h"
#include "Data.h"
#include "CharSetPage.h"
#include "FontParaDlg.h"
#include "IConvDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage プロパティ ページ

IMPLEMENT_DYNCREATE(CCharSetPage, CTreePage)

CCharSetPage::CCharSetPage() : CTreePage(CCharSetPage::IDD)
{
	m_KanjiCode = -1;
	m_CharBankGL = -1;
	m_CharBankGR = -1;
	m_CharBank1 = _T("");
	m_CharBank2 = _T("");
	m_CharBank3 = _T("");
	m_CharBank4 = _T("");
	m_AltFont = 0;
	m_ListIndex = (-1);
	m_UrlOpt = _T("#CHAROPT");
}

CCharSetPage::~CCharSetPage()
{
}

void CCharSetPage::DoDataExchange(CDataExchange* pDX)
{
	CTreePage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_FONTLIST, m_List);
	DDX_Radio(pDX, IDC_CHARSET1, m_KanjiCode);
	DDX_Radio(pDX, IDC_BANKGL1, m_CharBankGL);
	DDX_Radio(pDX, IDC_BANKGR1, m_CharBankGR);
	DDX_CBStringExact(pDX, IDC_CHARBANK1, m_CharBank1);
	DDX_CBStringExact(pDX, IDC_CHARBANK2, m_CharBank2);
	DDX_CBStringExact(pDX, IDC_CHARBANK3, m_CharBank3);
	DDX_CBStringExact(pDX, IDC_CHARBANK4, m_CharBank4);
	DDX_CBIndex(pDX, IDC_FONTNUM, m_AltFont);
	DDX_CBStringExact(pDX, IDC_FONTNAME, m_DefFontName);
	DDX_Control(pDX, IDC_FONTSAMPLE, m_FontSample);
}

BEGIN_MESSAGE_MAP(CCharSetPage, CTreePage)
	ON_BN_CLICKED(IDC_FONTLISTNEW, OnFontListNew)
	ON_BN_CLICKED(IDC_FONTLISTEDIT, OnFontListEdit)
	ON_BN_CLICKED(IDC_FONTLISTDEL, OnFontListDel)
	ON_BN_CLICKED(IDC_ICONVSET, OnIconvSet)
	ON_NOTIFY(NM_DBLCLK, IDC_FONTLIST, OnDblclkFontlist)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHARSET1, IDC_CHARSET4, OnCharSet)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_BANKGL1,  IDC_BANKGL4,  OnUpdateCheck)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_BANKGR1,  IDC_BANKGR4,  OnUpdateCheck)
	ON_CONTROL_RANGE(CBN_EDITCHANGE, IDC_CHARBANK1, IDC_CHARBANK4, OnUpdateCheck)
	ON_CONTROL_RANGE(CBN_SELCHANGE,  IDC_CHARBANK1, IDC_CHARBANK4, OnUpdateCheck)
	ON_COMMAND(ID_EDIT_NEW, OnFontListNew)
	ON_COMMAND(ID_EDIT_UPDATE, OnFontListEdit)
	ON_COMMAND(ID_EDIT_DELETE, OnFontListDel)
	ON_COMMAND(ID_EDIT_DUPS, OnEditDups)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
	ON_CBN_SELCHANGE(IDC_FONTNUM, &CCharSetPage::OnCbnSelchangeFontnum)
	ON_COMMAND(ID_EDIT_DELALL, &CCharSetPage::OnEditDelall)
	ON_CBN_EDITCHANGE(IDC_FONTNAME, &CCharSetPage::OnUpdateFontName)
	ON_CBN_SELCHANGE(IDC_FONTNAME, &CCharSetPage::OnCbnSelchangeFontName)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_FONTLIST, &CCharSetPage::OnLvnItemchangedFontlist)
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()

void CCharSetPage::InitList()
{
	int n, i;
	static LPCTSTR bankTab[] = { _T("94"), _T("96"), _T("94x94"), _T("96x96") };

	m_List.DeleteAllItems();
	for ( n = i = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_FontTab[n].m_EntryName.IsEmpty() )
			continue;
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, i, m_FontTab[n].m_EntryName, 0, 0, 0, n);
		m_List.SetItemText(i, 1, (n == SET_UNICODE ? _T("") : bankTab[n >> 8]));
		m_List.SetItemText(i, 2, m_FontTab[n].m_IndexName);
		m_List.SetItemText(i, 3, CFontParaDlg::CharSetName(m_FontTab[n].m_CharSet));
		m_List.SetItemText(i, 4, m_FontTab[n].m_FontName[m_AltFont]);
		m_List.SetItemText(i, 5, m_FontTab[n].m_CharSet == DEFAULT_CHARSET ? _T("") : ((int)m_FontSet[m_FontTab[n].m_FontName[m_AltFont].IsEmpty() ? m_DefFontName : m_FontTab[n].m_FontName[m_AltFont]][m_FontTab[n].m_CharSet] == 1 ? _T("○") : _T("×")));
		m_List.SetItemData(i, n);
		i++;
	}
	m_List.DoSortItem();
}

static int CALLBACK EnumFontFamExComboAddStr(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam)
{
	CCharSetPage *pWnd = (CCharSetPage *)lParam;
	CComboBox *pCombo = (CComboBox *)pWnd->GetDlgItem(IDC_FONTNAME);
	LPCTSTR name = lpelfe->elfLogFont.lfFaceName;

	if ( name[0] != _T('@') && pCombo->FindStringExact((-1), name) == CB_ERR )
		pCombo->AddString(name);

	pWnd->m_FontSet[name][lpelfe->elfLogFont.lfCharSet] = 1;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage メッセージ ハンドラ

static const LV_COLUMN InitListTab[6] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0, 110, _T("Entry"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Bank"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Code"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  70, _T("CSet"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  90, _T("Face"),  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  30, _T("In"),    0, 0 }, 
	};

void CCharSetPage::DoInit()
{
	m_AltFont    = m_pSheet->m_pTextRam->m_AttNow.std.font;
	m_FontTab    = m_pSheet->m_pTextRam->m_FontTab;
	m_KanjiCode  = m_pSheet->m_pTextRam->m_KanjiMode;
	m_CharBankGL = m_pSheet->m_pTextRam->m_BankGL;
	m_CharBankGR = m_pSheet->m_pTextRam->m_BankGR;

	for ( int n = 0 ; n < 16 ; n++ )
		m_DefFontTab[n] = m_pSheet->m_pTextRam->m_DefFontName[n];
	m_DefFontName = m_DefFontTab[m_AltFont];

	memcpy(m_BankTab, m_pSheet->m_pTextRam->m_BankTab, sizeof(m_BankTab));
	m_CharBank1  = m_FontTab[m_BankTab[m_KanjiCode][0]].GetEntryName();
	m_CharBank2  = m_FontTab[m_BankTab[m_KanjiCode][1]].GetEntryName();
	m_CharBank3  = m_FontTab[m_BankTab[m_KanjiCode][2]].GetEntryName();
	m_CharBank4  = m_FontTab[m_BankTab[m_KanjiCode][3]].GetEntryName();

	for ( int n = 0 ; n < 4 ; n++ )
		m_SendCharSet[n] = m_pSheet->m_pTextRam->m_SendCharSet[n];

	InitList();
	UpdateData(FALSE);
}
BOOL CCharSetPage::OnInitDialog() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	CTreePage::OnInitDialog();

	int n, i;
	CComboBox *pCombo[4];
	LPCTSTR pStr;
	CClientDC dc(this);
	LOGFONT logfont;

	ZeroMemory(&logfont, sizeof(LOGFONT)); 
	logfont.lfCharSet = DEFAULT_CHARSET;
	::EnumFontFamiliesEx(dc.m_hDC, &logfont, (FONTENUMPROC)EnumFontFamExComboAddStr, (LPARAM)this, 0);

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn(_T("CharSetPage"), InitListTab, 6);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 6);

	DoInit();

	for ( i = 0 ; i < 4 ; i++ )
		pCombo[i] = (CComboBox *)GetDlgItem(IDC_CHARBANK1 + i);

	for ( n = 0 ; n < CODE_MAX ; n++ ) {
		pStr = m_FontTab[n].m_EntryName;
		if ( *pStr == _T('\0') )
			continue;
		for ( i = 0 ; i < 4 ; i++ ) {
			if ( pCombo[i] != NULL && pCombo[i]->FindStringExact((-1), pStr) == CB_ERR )
				pCombo[i]->AddString(pStr);
		}
	}

	SubclassComboBox(IDC_CHARBANK1);
	SubclassComboBox(IDC_CHARBANK2);
	SubclassComboBox(IDC_CHARBANK3);
	SubclassComboBox(IDC_CHARBANK4);
	SubclassComboBox(IDC_FONTNAME);
	
	return TRUE;
}
BOOL CCharSetPage::OnApply() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	UpdateData(TRUE);

	m_pSheet->m_pTextRam->m_AttNow.std.font = m_AltFont;
	m_pSheet->m_pTextRam->SetKanjiMode(m_KanjiCode);
	m_pSheet->m_pTextRam->m_BankGL    = m_CharBankGL;
	m_pSheet->m_pTextRam->m_BankGR    = m_CharBankGR;
	m_pSheet->m_pTextRam->m_FontTab   = m_FontTab;

	m_DefFontTab[m_AltFont] = m_DefFontName;

	for ( int  n = 0 ; n < 16 ; n++ )
		m_pSheet->m_pTextRam->m_DefFontName[n] = m_DefFontTab[n];

	m_BankTab[m_KanjiCode][0] = m_FontTab.Find(m_CharBank1);
	m_BankTab[m_KanjiCode][1] = m_FontTab.Find(m_CharBank2);
	m_BankTab[m_KanjiCode][2] = m_FontTab.Find(m_CharBank3);
	m_BankTab[m_KanjiCode][3] = m_FontTab.Find(m_CharBank4);

	memcpy(m_pSheet->m_pTextRam->m_BankTab, m_BankTab, sizeof(m_BankTab));

	for ( int n = 0 ; n < 4 ; n++ )
		m_pSheet->m_pTextRam->m_SendCharSet[n] = m_SendCharSet[n];

	m_List.SaveColumn(_T("CharSetPage"));

	return TRUE;
}
void CCharSetPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pTextRam != NULL);

	DoInit();
	SetModified(FALSE);
}
void CCharSetPage::OnCharSet(UINT nID) 
{
	UpdateData(TRUE);
	m_CharBank1  = m_FontTab[m_BankTab[m_KanjiCode][0]].m_EntryName;
	m_CharBank2  = m_FontTab[m_BankTab[m_KanjiCode][1]].m_EntryName;
	m_CharBank3  = m_FontTab[m_BankTab[m_KanjiCode][2]].m_EntryName;
	m_CharBank4  = m_FontTab[m_BankTab[m_KanjiCode][3]].m_EntryName;
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_BANKTAB);
}
void CCharSetPage::OnFontListNew() 
{
	CFontParaDlg dlg;
	CFontNode tmp;

	dlg.m_CodeSet = 255;
	dlg.m_pData   = &tmp;
	dlg.m_FontNum = m_AltFont;
	dlg.m_pFontTab = &m_FontTab;
	dlg.m_pTextRam = m_pSheet->m_pTextRam;
	dlg.m_FontSize = m_pSheet->m_pTextRam->m_DefFontSize;

	if ( dlg.DoModal() != IDOK )
		return;

	m_FontTab[dlg.m_CodeSet] = tmp;
	m_FontTab.InitUniBlock();
	InitList();

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnFontListEdit() 
{
	int n;
	CFontParaDlg dlg;
	CFontNode tmp;

	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;

	tmp = m_FontTab[n];
	dlg.m_CodeSet = n;
	dlg.m_pData   = &tmp;
	dlg.m_FontNum = m_AltFont;
	dlg.m_pFontTab = &m_FontTab;
	dlg.m_pTextRam = m_pSheet->m_pTextRam;
	dlg.m_FontSize = m_pSheet->m_pTextRam->m_DefFontSize;

	if ( dlg.DoModal() != IDOK )
		return;

	m_FontTab[dlg.m_CodeSet] = tmp;
	m_FontTab.InitUniBlock();
	InitList();

	if ( (n = m_List.GetParamItem(dlg.m_CodeSet)) >= 0 ) {
		m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
		m_List.EnsureVisible(n, FALSE);
	}
	m_ListIndex = dlg.m_CodeSet;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnFontListDel() 
{
	int n;

	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;

	m_FontTab.IndexRemove(n);
	InitList();

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnEditDups() 
{
	int n;
	CFontParaDlg dlg;
	CFontNode tmp;

	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;

	tmp = m_FontTab[n];
	dlg.m_CodeSet = n;
	dlg.m_pData   = &tmp;
	dlg.m_FontNum = m_AltFont;
	dlg.m_pFontTab = &m_FontTab;
	dlg.m_pTextRam = m_pSheet->m_pTextRam;
	dlg.m_FontSize = m_pSheet->m_pTextRam->m_DefFontSize;

	if ( dlg.DoModal() != IDOK )
		return;

	m_FontTab[dlg.m_CodeSet] = tmp;
	m_FontTab.InitUniBlock();
	InitList();

	if ( (n = m_List.GetParamItem(dlg.m_CodeSet)) >= 0 ) {
		m_List.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
		m_List.EnsureVisible(n, FALSE);
	}

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnIconvSet() 
{
	int n;
	CIConvDlg dlg;

	for ( n = 0 ; n < 4 ; n++ )
		dlg.m_CharSet[n] = m_SendCharSet[n];

	if ( dlg.DoModal() != IDOK )
		return;

	for ( n = 0 ; n < 4 ; n++ )
		m_SendCharSet[n] = dlg.m_CharSet[n];

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnDblclkFontlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnFontListEdit();
	*pResult = 0;
}
void CCharSetPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_BANKTAB);
}
void CCharSetPage::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CCharSetPage::OnCbnSelchangeFontnum()
{
	int oldAltFont = m_AltFont;
	UpdateData(TRUE);
	InitList();
	m_DefFontTab[oldAltFont] = m_DefFontName;
	m_DefFontName = m_DefFontTab[m_AltFont];
	m_FontSample.Invalidate(FALSE);
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CCharSetPage::OnEditDelall()
{
	if ( MessageBox(CStringLoad(IDS_ALLINITREQ), _T("Warning"), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
		return;

	m_FontTab.Init();

	UpdateData(TRUE);
	InitList();
	UpdateData(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CCharSetPage::OnUpdateFontName()
{
	UpdateData(TRUE);

	m_DefFontName;
	m_DefFontTab[m_AltFont] = m_DefFontName;
	m_ListIndex = (-1);

	InitList();
	m_FontSample.Invalidate(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
void CCharSetPage::OnCbnSelchangeFontName()
{
	int n;
	CComboBox *pCombo = (CComboBox *)this->GetDlgItem(IDC_FONTNAME);

	if ( pCombo == NULL || (n = pCombo->GetCurSel()) < 0 )
		return;

	UpdateData(TRUE);

	pCombo->GetLBText(n, m_DefFontName);
	m_DefFontTab[m_AltFont] = m_DefFontName;
	m_ListIndex = (-1);

	UpdateData(FALSE);

	InitList();
	m_FontSample.Invalidate(FALSE);

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CCharSetPage::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC *pDC;
	CFont font, *pOld;
	LOGFONT logfont;
	CRect rect(lpDrawItemStruct->rcItem);
	CSize sz;
	int height;
	COLORREF fc, bc;
	CStringLoad sample(IDS_FONTSAMPLESTRING);

	switch(nIDCtl) {
	case IDC_FONTSAMPLE:
		UpdateData(TRUE);

		if ( m_pSheet->m_pScrnPage->m_hWnd != NULL ) {
			m_pSheet->m_pScrnPage->UpdateData(TRUE);
			height = _tstoi(m_pSheet->m_pScrnPage->m_FontSize);
		} else
			height = m_pSheet->m_pTextRam->m_DefFontSize;

		if ( m_pSheet->m_pColorPage->m_hWnd != NULL ) {
			m_pSheet->m_pColorPage->UpdateData(TRUE);
			if ( m_pSheet->m_pColorPage->m_FontCol[0] < 16 )
				fc = m_pSheet->m_pColorPage->m_ColTab[m_pSheet->m_pColorPage->m_FontCol[0]];
			else
				fc = m_pSheet->m_pTextRam->m_ColTab[m_pSheet->m_pColorPage->m_FontCol[0]];

			if ( m_pSheet->m_pColorPage->m_FontCol[1] < 16 )
				bc = m_pSheet->m_pColorPage->m_ColTab[m_pSheet->m_pColorPage->m_FontCol[1]];
			else
				bc = m_pSheet->m_pTextRam->m_ColTab[m_pSheet->m_pColorPage->m_FontCol[1]];
		} else {
			fc = m_pSheet->m_pTextRam->m_ColTab[m_pSheet->m_pTextRam->m_DefAtt.std.fcol];
			bc = m_pSheet->m_pTextRam->m_ColTab[m_pSheet->m_pTextRam->m_DefAtt.std.bcol];
		}

		pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
		pDC->FillSolidRect(rect, bc);

		ZeroMemory(&logfont, sizeof(logfont));
		logfont.lfWidth		= 0;
		logfont.lfHeight	= height;
		logfont.lfCharSet	= m_ListIndex < 0 ? DEFAULT_CHARSET : m_FontTab[m_ListIndex].m_CharSet;
		logfont.lfWeight	= FW_DONTCARE;
		logfont.lfItalic	= FALSE;
		logfont.lfUnderline	= FALSE;
		logfont.lfQuality   = DEFAULT_QUALITY;

		if ( m_ListIndex >= 0 && !m_FontTab[m_ListIndex].m_FontName[m_AltFont].IsEmpty() )
			_tcsncpy(logfont.lfFaceName, m_FontTab[m_ListIndex].m_FontName[m_AltFont], LF_FACESIZE);
		else
			_tcsncpy(logfont.lfFaceName, m_DefFontName, LF_FACESIZE);

		font.CreateFontIndirect(&logfont);

		if ( m_ListIndex >= 0 ) {
			CIConv iconv;
			union { DWORD d; WCHAR c[2]; } wc;
			if ( (m_ListIndex & SET_MASK) <= SET_96 ) {
				sample.Empty();
				for ( LPCSTR p = "012 abcABC \\|~" ; *p != '\0' ; p++ ) {
					wc.d = iconv.IConvChar(m_FontTab[m_ListIndex].m_IContName, _T("UTF-16BE"), *p | m_FontTab[m_ListIndex].m_Shift);
					if ( (wc.d & 0xFFFF0000) != 0 ) {
						sample += wc.c[1];
						sample += wc.c[0];
					} else if ( (wc.d & 0xFFFF) != 0 ) {
						sample += wc.c[0];
					}
				}
			} else if ( m_FontTab[m_ListIndex].m_CharSet == SHIFTJIS_CHARSET )
				sample = L"\U00003042\U00003044\U00003046 \U000065e5\U0000672c\U00008a9e";
			else if ( m_FontTab[m_ListIndex].m_CharSet == HANGEUL_CHARSET )
				sample = L"\U0000c544\U0000c774\U0000c6b0 \U0000d55c\U0000ae00";
			else if ( m_FontTab[m_ListIndex].m_CharSet == GB2312_CHARSET )
				sample = L"\U00004e02\U00004e04\U00004e05 \U00007b80\U00004f53\U00005b57";
			else if ( m_FontTab[m_ListIndex].m_CharSet == CHINESEBIG5_CHARSET )
				sample = L"\U00004e11\U00004e10\U00004e0d \U00007e41\U00004f53\U00005b57";
		}

		pOld = pDC->SelectObject(&font);
		sz = pDC->GetTextExtent(sample);

		pDC->SetTextColor(fc);
		pDC->SetBkColor(bc);
		pDC->TextOut(rect.left + (rect.Width() - sz.cx) / 2, rect.top + (rect.Height() - sz.cy) / 2, sample);

		pDC->SelectObject(pOld);
		break;
	default:
		CTreePage::OnDrawItem(nIDCtl, lpDrawItemStruct);
		break;
	}
}

void CCharSetPage::OnLvnItemchangedFontlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( pNMLV->iItem < 0 || pNMLV->uNewState != (LVIS_FOCUSED | LVIS_SELECTED) ) {
		if ( m_ListIndex >= 0 ) {
			m_ListIndex = (-1);
			m_FontSample.Invalidate(FALSE);
		}
	} else {
		m_ListIndex = (int)m_List.GetItemData(pNMLV->iItem);
		m_FontSample.Invalidate(FALSE);
	}

	*pResult = 0;
}
