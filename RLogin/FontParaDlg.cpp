// FontParaDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "IConv.h"
#include "FontParaDlg.h"
#include "BlockDlg.h"
#include "Iso646Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFontParaDlg ダイアログ

IMPLEMENT_DYNAMIC(CFontParaDlg, CDialogExt)

CFontParaDlg::CFontParaDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CFontParaDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFontParaDlg)
	m_ShiftTemp = FALSE;
	m_CharSetTemp = _T("");
	m_ZoomTemp[0] = _T("100");
	m_ZoomTemp[1] = _T("100");
	m_OffsTemp[0] = _T("0");
	m_OffsTemp[1] = _T("0");
	m_BankTemp = _T("");
	m_CodeTemp = _T("");
	m_IContName = _T("");
	m_EntryName = _T("");
	m_FontName = _T("");
	m_FontNum  = 0;
	m_FontQuality = DEFAULT_QUALITY;
	m_pData = NULL;
	m_pFontTab = NULL;
	m_FontSize = 20;
	m_UniBlock = _T("");
	m_pTextRam = NULL;
	//}}AFX_DATA_INIT
}

void CFontParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontParaDlg)
	DDX_Check(pDX, IDC_SHIFT, m_ShiftTemp);
	DDX_CBString(pDX, IDC_CHARSET, m_CharSetTemp);
	DDX_CBString(pDX, IDC_DISPZOOMH, m_ZoomTemp[0]);
	DDX_CBString(pDX, IDC_DISPZOOMW, m_ZoomTemp[1]);
	DDX_CBString(pDX, IDC_DISPOFFSETH, m_OffsTemp[0]);
	DDX_CBString(pDX, IDC_DISPOFFSETW, m_OffsTemp[1]);
	DDX_CBString(pDX, IDC_CHARBANK, m_BankTemp);
	DDX_CBString(pDX, IDC_FONTCODE, m_CodeTemp);
	DDX_CBString(pDX, IDC_ICONVSET, m_IContName);
	DDX_Text(pDX, IDC_ENTRYNAME, m_EntryName);
	DDX_CBString(pDX, IDC_FACENAME, m_FontName);
	DDX_CBIndex(pDX, IDC_FONTNUM, m_FontNum);
	DDX_CBIndex(pDX, IDC_FONTQUALITY, m_FontQuality);
	DDX_CBString(pDX, IDC_OVERZERO, m_OverZero);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFontParaDlg, CDialogExt)
	//{{AFX_MSG_MAP(CFontParaDlg)
	ON_BN_CLICKED(IDC_FONTSEL, OnFontsel)
	ON_CBN_SELCHANGE(IDC_FONTNUM, &CFontParaDlg::OnCbnSelchangeFontnum)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_CHARSET, &CFontParaDlg::OnCbnSelchangeCharset)
	ON_CBN_EDITCHANGE(IDC_CHARSET, &CFontParaDlg::OnCbnEditchangeCharset)
	ON_BN_CLICKED(IDC_UNIBLOCK, &CFontParaDlg::OnBnClickedUniblock)
	ON_BN_CLICKED(IDC_ISO646SET, &CFontParaDlg::OnBnClickedIso646set)
END_MESSAGE_MAP()

static const struct _CharSetTab {
		int code;
		LPCTSTR name;
		LPCTSTR iconvset;
	} CharSetTab[] = {
		{ DEFAULT_CHARSET,		_T("DEFAULT"),			_T("UTF-16LE") },
		{ RUSSIAN_CHARSET,		_T("RUSSIAN"),			_T("CP866") },
		{ THAI_CHARSET,			_T("THAI"),				_T("CP874") },
		{ SHIFTJIS_CHARSET,		_T("SHIFTJIS"),			_T("CP932") },
		{ GB2312_CHARSET,		_T("GB2312"),			_T("CP936") },
		{ HANGEUL_CHARSET,		_T("HANGEUL"),			_T("CP949") },
		{ CHINESEBIG5_CHARSET,	_T("CHINESEBIG5"),		_T("CP950") },
		{ EASTEUROPE_CHARSET,	_T("EASTEUROPE"),		_T("CP1250") },
		{ ANSI_CHARSET,			_T("ANSI"),				_T("CP1252") },
		{ GREEK_CHARSET,		_T("GREEK"),			_T("CP1253") },
		{ TURKISH_CHARSET,		_T("TURKISH"),			_T("CP1254") },
		{ HEBREW_CHARSET,		_T("HEBREW"),			_T("CP1255") },
		{ ARABIC_CHARSET,		_T("ARABIC"),			_T("CP1256") },
		{ BALTIC_CHARSET,		_T("BALTIC"),			_T("CP1257") },
		{ VIETNAMESE_CHARSET,	_T("VIETNAMESE"),		_T("CP1258") },
		{ JOHAB_CHARSET,		_T("JOHAB"),			_T("CP1361") },
		{ MAC_CHARSET,			_T("MAC"),				_T("MAC") },
		{ SYMBOL_CHARSET,		_T("SYMBOL"),			_T("") },
		{ OEM_CHARSET,			_T("OEM"),				_T("") },
		{ (-1), NULL },
	};

int CFontParaDlg::CharSetNo(LPCTSTR name)
{
	int n;
	for ( n = 0 ; CharSetTab[n].name != NULL ; n++ ) {
		if ( _tcscmp(name, CharSetTab[n].name) == 0 )
			return CharSetTab[n].code;
	}
	return _tstoi(name);
}
LPCTSTR CFontParaDlg::CharSetName(int code)
{
	int n;
    static TCHAR tmp[8];
	for ( n = 0 ; CharSetTab[n].name != NULL ; n++ ) {
		if ( CharSetTab[n].code == code )
			return CharSetTab[n].name;
	}
	_stprintf(tmp, _T("%d"), code);
	return tmp;
}
LPCTSTR CFontParaDlg::IConvName(int code)
{
	int n;
	for ( n = 0 ; CharSetTab[n].name != NULL ; n++ ) {
		if ( CharSetTab[n].code == code )
			return CharSetTab[n].iconvset;
	}
	return _T("");
}

int CFontParaDlg::CodeSetNo(LPCTSTR bank, LPCTSTR code)
{
	int num = 0;

	if      ( _tcscmp(bank, _T("94")) == 0 )	num |= SET_94;
	else if ( _tcscmp(bank, _T("96")) == 0 )	num |= SET_96;
	else if ( _tcscmp(bank, _T("94x94")) == 0 )	num |= SET_94x94;
	else if ( _tcscmp(bank, _T("96x96")) == 0 )	num |= SET_96x96;

	if ( _tcscmp(code, _T("Unicode")) == 0 )
		num = SET_UNICODE;
	else if ( code[1] == _T('\0') && code[0] >= _T('\x30') && code[0] <= _T('\x7E') )
		num |= (code[0] & 0xFF);
	else
		num = m_pFontTab->IndexFind(num, code);

	return num;
}
void CFontParaDlg::CodeSetName(int num, CString &bank, CString &code)
{
	if ( num == SET_UNICODE ) {
		bank = _T("");
		code = _T("Unicode");
	} else {
		switch(num & SET_MASK) {
		case SET_94:	bank = _T("94"); break;
		case SET_96:	bank = _T("96"); break;
		case SET_94x94:	bank = _T("94x94"); break;
		case SET_96x96:	bank = _T("96x96"); break;
		}
		if ( (num & 0xFF) >= _T('\x30') && (num & 0xFF) <= _T('\x7E') )
			code.Format(_T("%c"), num & 0xFF);
		else
			code = m_pFontTab->m_Data[num].m_IndexName;
	}
}

static int CALLBACK EnumFontFamExComboAddStr(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam)
{
	CComboBox *pCombo = (CComboBox *)lParam;
	LPCTSTR name = lpelfe->elfLogFont.lfFaceName;

	if ( name[0] != _T('@') && pCombo->FindStringExact((-1), name) == CB_ERR )
		pCombo->AddString(name);

	return TRUE;
}
void CFontParaDlg::SetFontFace(int nID)
{
	CClientDC dc(this);
	LOGFONT logfont;
	CComboBox *pCombo = (CComboBox *)GetDlgItem(nID);

	if ( pCombo == NULL )
		return;

	for ( int n = pCombo->GetCount() - 1 ; n >= 0; n-- )
		pCombo->DeleteString(n);

	ZeroMemory(&logfont, sizeof(LOGFONT)); 
	logfont.lfCharSet = CharSetNo(m_CharSetTemp);
	::EnumFontFamiliesEx(dc.m_hDC, &logfont, (FONTENUMPROC)EnumFontFamExComboAddStr, (LPARAM)GetDlgItem(nID), 0);
}

/////////////////////////////////////////////////////////////////////////////
// CFontParaDlg メッセージ ハンドラ

BOOL CFontParaDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	int n;
	CComboBox *pCombo;
	CStringArray stra;

	ASSERT(m_pData != NULL && m_pFontTab != NULL);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CHARSET)) != NULL ) {
		for ( n = 0 ; CharSetTab[n].name != NULL ; n++ )
			pCombo->AddString(CharSetTab[n].name);
	}

	CIConv::SetListArray(stra);
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_ICONVSET)) != NULL ) {
		for ( n = 0 ; n < stra.GetSize() ; n++ )
			pCombo->AddString(stra[n]);
	}

	CodeSetName(m_CodeSet, m_BankTemp, m_CodeTemp);
	m_CharSetTemp = CharSetName(m_pData->m_CharSet);
	m_ShiftTemp = (m_pData->m_Shift != 0 ? TRUE : FALSE);
	m_ZoomTemp[0].Format(_T("%d"), m_pData->m_ZoomH);
	m_ZoomTemp[1].Format(_T("%d"), m_pData->m_ZoomW);
	m_OffsTemp[0].Format(_T("%d"), m_pData->m_OffsetH);
	m_OffsTemp[1].Format(_T("%d"), m_pData->m_OffsetW);
	m_IContName = m_pData->m_IContName;
	m_EntryName = m_pData->m_EntryName;
	m_FontQuality = m_pData->m_Quality;
	m_UniBlock = m_pData->m_UniBlock;
	m_Iso646Name[0] = m_pData->m_Iso646Name[0];
	m_Iso646Name[1] = m_pData->m_Iso646Name[1];
	memcpy(m_Iso646Tab, m_pData->m_Iso646Tab, sizeof(m_Iso646Tab));

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_FONTCODE)) != NULL ) {
		if ( m_CodeTemp.GetLength() == 1 && m_CodeTemp[0] >= _T('a') && m_CodeTemp[0] <= _T('z') )
			pCombo->AddString(m_CodeTemp);
		else if ( pCombo->FindStringExact((-1), m_CodeTemp) == CB_ERR )
			pCombo->AddString(m_CodeTemp);
	}

	for ( n = 0 ; n < 16 ; n++ )
		m_FontNameTab[n] = m_pData->m_FontName[n];

	SetFontFace(IDC_FACENAME);
	m_FontName  = m_FontNameTab[m_FontNum];

	m_OverZero = m_pData->m_OverZero;

	UpdateData(FALSE);

	return TRUE;
}
void CFontParaDlg::OnOK()
{
	ASSERT(m_pData != NULL);

	UpdateData(TRUE);

	m_CodeSet            = CodeSetNo(m_BankTemp, m_CodeTemp);
	m_pData->m_CharSet   = CharSetNo(m_CharSetTemp);
	m_pData->m_Shift     = (m_ShiftTemp ? 0x80 : 0x00);
	m_pData->m_ZoomH     = _tstoi(m_ZoomTemp[0]);
	m_pData->m_ZoomW     = _tstoi(m_ZoomTemp[1]);
	m_pData->m_OffsetH   = _tstoi(m_OffsTemp[0]);
	m_pData->m_OffsetW   = _tstoi(m_OffsTemp[1]);
	m_pData->m_IContName = m_IContName;
	m_pData->m_EntryName = m_EntryName;
	m_pData->m_Quality   = m_FontQuality;
	m_pData->m_IndexName = m_CodeTemp;
	m_pData->m_UniBlock  = m_UniBlock;
	m_pData->m_Iso646Name[0] = m_Iso646Name[0];
	m_pData->m_Iso646Name[1] = m_Iso646Name[1];
	memcpy(m_pData->m_Iso646Tab, m_Iso646Tab, sizeof(m_Iso646Tab));
	m_pData->m_OverZero = m_OverZero;

	m_FontNameTab[m_FontNum] = m_FontName;

	for ( int n = 0 ; n < 16 ; n++ )
		m_pData->m_FontName[n] = m_FontNameTab[n];

	CDialogExt::OnOK();
}

void CFontParaDlg::OnFontsel() 
{
	LOGFONT LogFont;

	UpdateData(TRUE);
	memset(&(LogFont), 0, sizeof(LOGFONT));
	LogFont.lfWidth          = 0;
	LogFont.lfHeight         = m_FontSize;
	LogFont.lfWeight         = FW_DONTCARE;
	LogFont.lfCharSet        = CharSetNo(m_CharSetTemp);
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DEFAULT_QUALITY;
	LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	if ( m_FontName.IsEmpty() && m_FontNum > 0 )
	    _tcsncpy(LogFont.lfFaceName, m_FontNameTab[0], LF_FACESIZE);
	else
	    _tcsncpy(LogFont.lfFaceName, m_FontName, LF_FACESIZE);

#define	CF_INACTIVEFONTS	0x02000000L

	CFontDialog font(&LogFont, CF_NOVERTFONTS | CF_SCREENFONTS | CF_SELECTSCRIPT | CF_INACTIVEFONTS, NULL, this);

	if ( font.DoModal() != IDOK )
		return;

    m_FontNameTab[m_FontNum] = m_FontName = LogFont.lfFaceName;

	UpdateData(FALSE);
}

void CFontParaDlg::OnCbnSelchangeFontnum()
{
	int OldFondNum = m_FontNum;

	UpdateData(TRUE);
	m_FontNameTab[OldFondNum] = m_FontName;

	m_FontName  = m_FontNameTab[m_FontNum];
	UpdateData(FALSE);
}

void CFontParaDlg::OnCbnSelchangeCharset()
{
	int n;
	CComboBox *pCombo = (CComboBox *)this->GetDlgItem(IDC_CHARSET);

	if ( pCombo == NULL || (n = pCombo->GetCurSel()) < 0 )
		return;

	UpdateData(TRUE);
	pCombo->GetLBText(n, m_CharSetTemp);
	UpdateData(FALSE);

	SetFontFace(IDC_FACENAME);
}

void CFontParaDlg::OnCbnEditchangeCharset()
{
	UpdateData(TRUE);
	SetFontFace(IDC_FACENAME);
}

void CFontParaDlg::OnBnClickedUniblock()
{
	CBlockDlg dlg;
	CFontNode tmp;

	UpdateData(TRUE);

	tmp = *m_pData;
	tmp.m_CharSet   = CharSetNo(m_CharSetTemp);
	tmp.m_Quality   = m_FontQuality;
	tmp.m_IndexName = m_CodeTemp;
	tmp.m_FontName[m_FontNum] = m_FontName;
	tmp.m_UniBlock = m_UniBlock;

	dlg.m_CodeSet   = CodeSetNo(m_BankTemp, m_CodeTemp);
	dlg.m_FontNum   = m_FontNum;
	dlg.m_pFontNode = &tmp;
	dlg.m_pFontTab  = m_pFontTab;
	dlg.m_pTextRam  = m_pTextRam;

	if ( dlg.DoModal() == IDOK )
		m_UniBlock = tmp.m_UniBlock;
}

void CFontParaDlg::OnBnClickedIso646set()
{
	CIso646Dlg dlg;

	UpdateData(TRUE);

	dlg.m_FontName = (m_pTextRam != NULL && m_FontName.IsEmpty() ? m_pTextRam->m_DefFontName[m_FontNum] : m_FontName);
	dlg.m_CharSet  = CharSetNo(m_CharSetTemp);
	dlg.m_FontSetName = m_Iso646Name[0];
	dlg.m_DispSetName = m_Iso646Name[1];
	memcpy(dlg.m_Iso646Tab, m_Iso646Tab, sizeof(m_Iso646Tab));

	if ( dlg.DoModal() != IDOK )
		return;

	m_Iso646Name[0] = dlg.m_FontSetName;
	m_Iso646Name[1] = dlg.m_DispSetName;
	memcpy(m_Iso646Tab, dlg.m_Iso646Tab, sizeof(m_Iso646Tab));
}
