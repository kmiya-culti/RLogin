// FontParaDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "FontParaDlg.h"
#include "IConv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFontParaDlg ダイアログ

CFontParaDlg::CFontParaDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFontParaDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFontParaDlg)
	m_ShiftTemp = FALSE;
	m_CharSetTemp = _T("");
	m_ZoomTemp[0] = _T("100");
	m_ZoomTemp[1] = _T("100");
	m_OffsTemp = _T("0");
	m_BankTemp = _T("");
	m_CodeTemp = _T("");
	m_IContName = _T("");
	m_EntryName = _T("");
	m_FontName = _T("");
	m_FontNum  = 0;
	//}}AFX_DATA_INIT
	m_pData = NULL;
}

void CFontParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontParaDlg)
	DDX_Check(pDX, IDC_SHIFT, m_ShiftTemp);
	DDX_CBString(pDX, IDC_CHARSET, m_CharSetTemp);
	DDX_CBString(pDX, IDC_DISPZOOM, m_ZoomTemp[0]);
	DDX_CBString(pDX, IDC_DISPZOOM2, m_ZoomTemp[1]);
	DDX_CBString(pDX, IDC_DISPOFFSET, m_OffsTemp);
	DDX_CBString(pDX, IDC_CHARBANK, m_BankTemp);
	DDX_CBString(pDX, IDC_FONTCODE, m_CodeTemp);
	DDX_CBString(pDX, IDC_ICONVSET, m_IContName);
	DDX_Text(pDX, IDC_ENTRYNAME, m_EntryName);
	DDX_Text(pDX, IDC_FACENAME, m_FontName);
	//}}AFX_DATA_MAP
	DDX_CBIndex(pDX, IDC_FONTNUM, m_FontNum);
}

BEGIN_MESSAGE_MAP(CFontParaDlg, CDialog)
	//{{AFX_MSG_MAP(CFontParaDlg)
	ON_BN_CLICKED(IDC_FONTSEL, OnFontsel)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_FONTNUM, &CFontParaDlg::OnCbnSelchangeFontnum)
END_MESSAGE_MAP()

static const struct _CharSetTab {
		int code;
		LPCSTR name;
		LPCSTR iconvset;
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

int CFontParaDlg::CharSetNo(LPCSTR name)
{
	int n;
	for ( n = 0 ; CharSetTab[n].name != NULL ; n++ ) {
		if ( strcmp(name, CharSetTab[n].name) == 0 )
			return CharSetTab[n].code;
	}
	return atoi(name);
}
LPCSTR CFontParaDlg::CharSetName(int code)
{
	int n;
    static char tmp[8];
	for ( n = 0 ; CharSetTab[n].name != NULL ; n++ ) {
		if ( CharSetTab[n].code == code )
			return CharSetTab[n].name;
	}
	sprintf(tmp, "%d", code);
	return tmp;
}
LPCSTR CFontParaDlg::IConvName(int code)
{
	int n;
	for ( n = 0 ; CharSetTab[n].name != NULL ; n++ ) {
		if ( CharSetTab[n].code == code )
			return CharSetTab[n].iconvset;
	}
	return "";
}

int CFontParaDlg::CodeSetNo(LPCSTR bank, LPCSTR code)
{
	int num = 0;

	if      ( strcmp(bank, "94") == 0 )		num |= SET_94;
	else if ( strcmp(bank, "96") == 0 )		num |= SET_96;
	else if ( strcmp(bank, "94x94") == 0 )	num |= SET_94x94;
	else if ( strcmp(bank, "96x96") == 0 )	num |= SET_96x96;

	if ( strcmp(code, "Unicode") == 0 )		num = SET_UNICODE;
	else									num |= (code[0] & 0xFF);

	return num;
}
void CFontParaDlg::CodeSetName(int num, CString &bank, CString &code)
{
	if ( num == SET_UNICODE ) {
		bank = "";
		code = "Unicode";
	} else {
		switch(num & SET_MASK) {
		case SET_94:	bank = "94"; break;
		case SET_96:	bank = "94"; break;
		case SET_94x94:	bank = "94x94"; break;
		case SET_96x96:	bank = "96x96"; break;
		}
		code.Format("%c", num & 0xFF);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFontParaDlg メッセージ ハンドラ

BOOL CFontParaDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int n;
	CComboBox *pCombo;
	CStringArray array;

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CHARSET)) != NULL ) {
		for ( n = 0 ; CharSetTab[n].name != NULL ; n++ )
			pCombo->AddString(CharSetTab[n].name);
	}

	CIConv::SetListArray(array);
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_ICONVSET)) != NULL ) {
		for ( n = 0 ; n < array.GetSize() ; n++ )
			pCombo->AddString(array[n]);
	}

	ASSERT(m_pData);
	CodeSetName(m_CodeSet, m_BankTemp, m_CodeTemp);
	m_CharSetTemp = CharSetName(m_pData->m_CharSet);
	m_ShiftTemp = (m_pData->m_Shift != 0 ? TRUE : FALSE);
	m_ZoomTemp[0].Format("%d", m_pData->m_ZoomH);
	m_ZoomTemp[1].Format("%d", m_pData->m_ZoomW);
	m_OffsTemp.Format("%d", m_pData->m_Offset);
	m_IContName = m_pData->m_IContName;
	m_EntryName = m_pData->m_EntryName;

	for ( n = 0 ; n < 16 ; n++ )
		m_FontNameTab[n] = m_pData->m_FontName[n];
	m_FontName  = m_FontNameTab[m_FontNum];

	UpdateData(FALSE);

	return TRUE;
}
void CFontParaDlg::OnOK()
{
	UpdateData(TRUE);

	ASSERT(m_pData);
	m_CodeSet            = CodeSetNo(m_BankTemp, m_CodeTemp);
	m_pData->m_CharSet   = CharSetNo(m_CharSetTemp);
	m_pData->m_Shift     = (m_ShiftTemp ? 0x80 : 0x00);
	m_pData->m_ZoomH     = atoi(m_ZoomTemp[0]);
	m_pData->m_ZoomW     = atoi(m_ZoomTemp[1]);
	m_pData->m_Offset    = atoi(m_OffsTemp);
	m_pData->m_IContName = m_IContName;
	m_pData->m_EntryName = m_EntryName;

	m_FontNameTab[m_FontNum] = m_FontName;
	for ( int n = 0 ; n < 16 ; n++ ) {
		m_pData->m_FontName[n] = m_FontNameTab[n];
		m_pData->SetHash(n);
	}

	CDialog::OnOK();
}

void CFontParaDlg::OnFontsel() 
{
	LOGFONT LogFont;

	UpdateData(TRUE);
	memset(&(LogFont), 0, sizeof(LOGFONT));
	LogFont.lfWidth          = 0;
	LogFont.lfHeight         = 20;
	LogFont.lfWeight         = FW_DONTCARE;
	LogFont.lfCharSet        = CharSetNo(m_CharSetTemp);
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DEFAULT_QUALITY;
	LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	if ( m_FontName.IsEmpty() && m_FontNum > 0 )
	    strcpy(LogFont.lfFaceName, m_FontNameTab[0]);
	else
	    strcpy(LogFont.lfFaceName, m_FontName);

	CFontDialog font(&LogFont,
		CF_NOVERTFONTS | CF_SCREENFONTS | CF_SELECTSCRIPT,
		NULL, this);

	if ( font.DoModal() != IDOK )
		return;

    m_FontNameTab[m_FontNum] = m_FontName = LogFont.lfFaceName;
	UpdateData(FALSE);
}

void CFontParaDlg::OnCbnSelchangeFontnum()
{
	m_FontNameTab[m_FontNum] = m_FontName;
	UpdateData(TRUE);
	m_FontName  = m_FontNameTab[m_FontNum];
	UpdateData(FALSE);
}
