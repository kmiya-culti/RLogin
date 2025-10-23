// Iso646Dlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Iso646Dlg.h"

//////////////////////////////////////////////////////////////////////
// CIso646Dlg ダイアログ

IMPLEMENT_DYNAMIC(CIso646Dlg, CDialogExt)

static const WCHAR FullWidth[] = { 0xFF03, 0xFF04, 0xFF20, 0xFF3B, 0xFF3C, 0xFF3D, 0xFF3E, 0xFF40, 0xFF5B, 0xFF5C, 0xFF5D, 0xFF5E, 0xFF3F };
static const struct _iso646tab {
	LPCTSTR name;
	int ccsid;							//	8500 <= RLogin original page code
	WCHAR   code[13];
} Iso646Tab[] = {
	{ _T("US (ANSI X3.4-1968)"),			367,	{ 0x0023, 0x0024, 0x0040, 0x005B, 0x005C, 0x005D, 0x005E, 0x0060, 0x007B, 0x007C, 0x007D, 0x007E, 0x005F } },
	{ _T("JP (JIS X 0201)"),				895,	{ 0x0023, 0x0024, 0x0040, 0x005B, 0x00A5, 0x005D, 0x005E, 0x0060, 0x007B, 0x007C, 0x007D, 0x203E, 0x005F } },
	{ _T("KR (KS C 5636-1989)"),			8500,	{ 0x0023, 0x0024, 0x0040, 0x005B, 0x20A9, 0x005D, 0x005E, 0x0060, 0x007B, 0x007C, 0x007D, 0x203E, 0x005F } },
	{ _T("CN (GB/T 1988-80)"),				8501,	{ 0x0023, 0x00A5, 0x0040, 0x005B, 0x005C, 0x005D, 0x005E, 0x0060, 0x007B, 0x007C, 0x007D, 0x203E, 0x005F } },
	{ _T("TW (CNS 5205-1996)"),				8502,	{ 0x0023, 0x0024, 0x0040, 0x005B, 0x005C, 0x005D, 0x005E, 0x0060, 0x007B, 0x007C, 0x007D, 0x203E, 0x005F } },
	{ _T("IRV (ISO 646:1983)"),				8503,	{ 0x0023, 0x0024, 0x0040, 0x005B, 0x005C, 0x005D, 0x02C6, 0x0060, 0x007B, 0x007C, 0x007D, 0x02DC, 0x005F } },
	{ _T("GB (BS 4730)"),					1013,	{ 0x00A3, 0x0024, 0x0040, 0x005B, 0x005C, 0x005D, 0x02C6, 0x0060, 0x007B, 0x007C, 0x007D, 0x02DC, 0x005F } },
	{ _T("DK (DS 2089)"),					1017,	{ 0x0023, 0x0024, 0x0040, 0x00C6, 0x00D8, 0x00C5, 0x02C6, 0x0060, 0x00E6, 0x00F8, 0x00E5, 0x02DC, 0x005F } },
	{ _T("NO (NS 4551 version 1)"),			1016,	{ 0x0023, 0x0024, 0x0040, 0x00C6, 0x00D8, 0x00C5, 0x02C6, 0x0060, 0x00E6, 0x00F8, 0x00E5, 0x00AF, 0x005F } },
	{ _T("NO-2 (NS 4551 version 2)"),		20108,	{ 0x00A7, 0x0024, 0x0040, 0x00C6, 0x00D8, 0x00C5, 0x02C6, 0x0060, 0x00E6, 0x00F8, 0x00E5, 0x007C, 0x005F } },
	{ _T("FI (SFS 4017)"),					1018,	{ 0x0023, 0x00A4, 0x0040, 0x00C4, 0x00D6, 0x00C5, 0x02C6, 0x0060, 0x00E4, 0x00F6, 0x00E5, 0x02DC, 0x005F } },
	{ _T("SE-C (SEN 85 02 00 Annex C)"),	20107,	{ 0x0023, 0x00A4, 0x00C9, 0x00C4, 0x00D6, 0x00C5, 0x00DC, 0x00E9, 0x00E4, 0x00F6, 0x00E5, 0x00FC, 0x005F } },
	{ _T("DE (DIN 66003)"),					20106,	{ 0x0023, 0x0024, 0x00A7, 0x00C4, 0x00D6, 0x00DC, 0x02C6, 0x0060, 0x00E4, 0x00F6, 0x00FC, 0x00DF, 0x005F } },
	{ _T("HU (MSZ 7795/3)"),				8504,	{ 0x0023, 0x00A4, 0x00C1, 0x00C9, 0x00D6, 0x00DC, 0x02C6, 0x00E1, 0x00E9, 0x00F6, 0x00FC, 0x02DD, 0x005F } },
	{ _T("FR (AFNOR NF Z 62010-1982)"),		1010,	{ 0x00A3, 0x0024, 0x00E0, 0x00B0, 0x00E7, 0x00A7, 0x005E, 0x00B5, 0x00E9, 0x00F9, 0x00E8, 0x00A8, 0x005F } },
	{ _T("FR-0 (AFNOR NF Z 62010-1973)"),	1104,	{ 0x00A3, 0x0024, 0x00E0, 0x00B0, 0x00E7, 0x00A7, 0x02C6, 0x00B5, 0x00E9, 0x00F9, 0x00E8, 0x00A8, 0x005F } },
	{ _T("CA-1 (CSA Z243.4-1985)"),			1020,	{ 0x0023, 0x0024, 0x00E0, 0x00E2, 0x00E7, 0x00EA, 0x00EE, 0x00F4, 0x00E9, 0x00F9, 0x00E8, 0x00FB, 0x005F } },
	{ _T("CA-2 (CSA Z243.4-1985)"),			8505,	{ 0x0023, 0x0024, 0x00E0, 0x00E2, 0x00E7, 0x00EA, 0x00C9, 0x00F4, 0x00E9, 0x00F9, 0x00E8, 0x00FB, 0x005F } },
	{ _T("IE (NSAI 433:1996)"),				8506,	{ 0x00A3, 0x0024, 0x00D3, 0x00C9, 0x00CD, 0x00DA, 0x00C1, 0x00F3, 0x00E9, 0x00ED, 0x00FA, 0x00E1, 0x005F } },
	{ _T("CU (NC 99-10:81)"),				8507,	{ 0x0023, 0x00A4, 0x0040, 0x00A1, 0x00D1, 0x005D, 0x00BF, 0x0060, 0x00B4, 0x00F1, 0x005B, 0x00A8, 0x005F } },
	{ _T("YU (JUS I.B1.002)"),				8508,	{ 0x0023, 0x0024, 0x017D, 0x0160, 0x0110, 0x0106, 0x010C, 0x017E, 0x0161, 0x0111, 0x0107, 0x010D, 0x005F } },

	{ _T("United Kingdom (NRCS)"),			1101,	{ 0x00A3, 0x0024, 0x0040, 0x005B, 0x005C, 0x005D, 0x005E, 0x0060, 0x007B, 0x007C, 0x007D, 0x007E, 0x005F } },
	{ _T("Norwegian/Danish (NRCS)"),		1105,	{ 0x0023, 0x0024, 0x00C4, 0x00C6, 0x00D8, 0x00C5, 0x00DC, 0x00E4, 0x00E6, 0x00F8, 0x00E5, 0x00FC, 0x005F } },	// 6	E	`
	{ _T("Dutch (NRCS)"),					1102,	{ 0x00A3, 0x0024, 0x00BE, 0x0133, 0x00BD, 0x007C, 0x005E, 0x0060, 0x00A8, 0x0192, 0x00BC, 0x00B4, 0x005F } },	// 4
	{ _T("Finnish (NRCS)"),					1103,	{ 0x0023, 0x0024, 0x0040, 0x00C4, 0x00D6, 0x00C5, 0x00DC, 0x00E9, 0x00E4, 0x00F6, 0x00E5, 0x00FC, 0x005F } },	// 5	C
	{ _T("French (NRCS)"),					1104,	{ 0x00A3, 0x0024, 0x00E0, 0x00B0, 0x00E7, 0x00A7, 0x005E, 0x0060, 0x00E9, 0x00F9, 0x00E8, 0x00A8, 0x005F } },	// R	f
	{ _T("French Canadian (NRCS)"),			1020,	{ 0x0023, 0x0024, 0x00E0, 0x00E2, 0x00E7, 0x00EA, 0x00EE, 0x00F4, 0x00E9, 0x00F9, 0x00E8, 0x00FB, 0x005F } },	// 9	Q
	{ _T("German (NRCS)"),					1011,	{ 0x0023, 0x0024, 0x00A7, 0x00C4, 0x00D6, 0x00DC, 0x005E, 0x0060, 0x00E4, 0x00F6, 0x00FC, 0x00DF, 0x005F } },	// K
	{ _T("Italian (NRCS)"),					1012,	{ 0x00A3, 0x0024, 0x00A7, 0x00B0, 0x00E7, 0x00E9, 0x005E, 0x00F9, 0x00E0, 0x00F2, 0x00E8, 0x00EC, 0x005F } },	// Y
	{ _T("Japanese Roman (NRCS)"),			895,	{ 0x0023, 0x0024, 0x0040, 0x005B, 0x00A5, 0x005D, 0x005E, 0x0060, 0x007B, 0x007C, 0x007D, 0x203E, 0x005F } },
	{ _T("Portuguese (NRCS)"),				1015,	{ 0x0023, 0x0024, 0x0040, 0x00C3, 0x00C7, 0x00D5, 0x005E, 0x0060, 0x00E3, 0x00E7, 0x00F5, 0x007E, 0x005F } },	// %6
	{ _T("Spanish (NRCS)"),					1023,	{ 0x00A3, 0x0024, 0x00A7, 0x00A1, 0x00D1, 0x00BF, 0x005E, 0x0060, 0x00B0, 0x00F1, 0x00E7, 0x007E, 0x005F } },	// Z
	{ _T("Swedish (NRCS)"),					1106,	{ 0x0023, 0x0024, 0x0040, 0x00C4, 0x00D6, 0x00C5, 0x00DC, 0x00E9, 0x00E4, 0x00F6, 0x00E5, 0x00FC, 0x005F } },	// 7	H
	{ _T("Swiss (NRCS)"),					1021,	{ 0x00F9, 0x0024, 0x00E0, 0x00E9, 0x00E7, 0x00EA, 0x00EE, 0x00F4, 0x00E4, 0x00F6, 0x00FC, 0x00FB, 0x00E8 } },	// =

	{ _T("Serbo Croatian (NRCS)"),			8509,	{ 0x0023, 0x0024, 0x017D, 0x0160, 0x0110, 0x0106, 0x010C, 0x017E, 0x0161, 0x0111, 0x0107, 0x010D, 0x005F } },	// %3
	{ _T("Turkish (NRCS)"),					8510,	{ 0x0023, 0x0024, 0x0130, 0x015E, 0x00D6, 0x00C7, 0x00dC, 0x011E, 0x015F, 0x00F6, 0x007D, 0x00E7, 0x005F } },

	{ NULL }
};

BOOL CIso646Dlg::Iso646Maping(LPCTSTR name, DWORD code[13])
{
	for ( int n = 0 ; Iso646Tab[n].name != NULL ; n++ ) {
		if ( _tcscmp(Iso646Tab[n].name, name) == 0 ) {
			for ( int i = 0 ; i < 13 ; i++ )
				code[i] = (DWORD)Iso646Tab[n].code[i];
			return TRUE;
		}
	}
	return FALSE;
}
int CIso646Dlg::CodePage(LPCTSTR name)
{
	int code;

	if ( _tcsncmp(name, _T("CP0"), 3) != 0 )
		return (-1);

	if ( (code = _tstoi(name + 3)) <= 0 )
		return (-1);

	for ( int n = 0 ; Iso646Tab[n].name != NULL ; n++ ) {
		if ( Iso646Tab[n].ccsid == code )
			return n;
	}
	return (-1);
}
DWORD CIso646Dlg::TableMaping(int idx, DWORD ch)
{
	switch(ch) {
	case 0x0023: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[0]); break;
	case 0x0024: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[1]); break;
	case 0x0040: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[2]); break;
	case 0x005B: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[3]); break;
	case 0x005C: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[4]); break;
	case 0x005D: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[5]); break;
	case 0x005E: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[6]); break;
	case 0x005F: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[12]); break;
	case 0x0060: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[7]); break;
	case 0x007B: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[8]); break;
	case 0x007C: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[9]); break;
	case 0x007D: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[10]); break;
	case 0x007E: ch = CTextRam::UCS2toUCS4(Iso646Tab[idx].code[11]); break;
	}
	return ch;
}
void CIso646Dlg::CodeIndex(int idx, CDwordIndex &code)
{
	for ( int n = 0x20 ; n < 0x80 ; n++ )
		code[(DWORD)n] = (DWORD)n;

	code[(DWORD)Iso646Tab[idx].code[0]]  = (DWORD)0x0023;
	code[(DWORD)Iso646Tab[idx].code[1]]  = (DWORD)0x0024;
	code[(DWORD)Iso646Tab[idx].code[2]]  = (DWORD)0x0040;
	code[(DWORD)Iso646Tab[idx].code[3]]  = (DWORD)0x005B;
	code[(DWORD)Iso646Tab[idx].code[4]]  = (DWORD)0x005C;
	code[(DWORD)Iso646Tab[idx].code[5]]  = (DWORD)0x005D;
	code[(DWORD)Iso646Tab[idx].code[6]]  = (DWORD)0x005E;
	code[(DWORD)Iso646Tab[idx].code[12]] = (DWORD)0x005F;
	code[(DWORD)Iso646Tab[idx].code[7]]  = (DWORD)0x0060;
	code[(DWORD)Iso646Tab[idx].code[8]]  = (DWORD)0x007B;
	code[(DWORD)Iso646Tab[idx].code[9]]  = (DWORD)0x007C;
	code[(DWORD)Iso646Tab[idx].code[10]] = (DWORD)0x007D;
	code[(DWORD)Iso646Tab[idx].code[11]] = (DWORD)0x007E;
}

CIso646Dlg::CIso646Dlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CIso646Dlg::IDD, pParent)
{
	m_CharSet = DEFAULT_CHARSET;
	ZeroMemory(m_Iso646Tab, sizeof(m_Iso646Tab));
}
CIso646Dlg::~CIso646Dlg()
{
}

void CIso646Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_FONTSET, m_FontSet);
	DDX_Control(pDX, IDC_DISPSET, m_DispSet);
	for ( int n = 0 ; n < 26 ; n++ )
		DDX_Control(pDX, IDC_FONTVIEW1 + n, m_ViewBox[n]);
	for ( int n = 0 ; n < 13 ; n++ )
		DDX_CBStringExact(pDX, IDC_CODESET1 + n, m_CharCode[n]);
}

void CIso646Dlg::SetViewBox(int num, DWORD code)
{
	CString str;

	code = CTextRam::UCS4toUCS2(code);
	str.Empty();

	if ( (code & 0xFFFF0000) != 0 )
		str += (WCHAR)(code >> 16);
	str += (WCHAR)code;
	
	m_ViewBox[num].SetWindowText(str);
}


BEGIN_MESSAGE_MAP(CIso646Dlg, CDialogExt)
	ON_CBN_SELCHANGE(IDC_FONTSET, &CIso646Dlg::OnCbnSelchangeFontDispSet)
	ON_CBN_SELCHANGE(IDC_DISPSET, &CIso646Dlg::OnCbnSelchangeFontDispSet)
	ON_CONTROL_RANGE(CBN_EDITUPDATE, IDC_CODESET1, IDC_CODESET12, &CIso646Dlg::OnCbnUpdateCodeset)
	ON_CONTROL_RANGE(CBN_SELCHANGE, IDC_CODESET1, IDC_CODESET12, &CIso646Dlg::OnCbnSelchangeCodeset)
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CIso646Dlg メッセージ ハンドラー

BOOL CIso646Dlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	int n, i;
	CString str;
	CComboBox *pCombo;
	LOGFONT logfont;
	CRect rect;

	if ( m_FontSetName.IsEmpty() )
		m_FontSetName = _T("US");
	if ( m_DispSetName.IsEmpty() )
		m_DispSetName = _T("US");

	for ( n = 0 ; Iso646Tab[n].name != NULL ; n++ ) {
		m_FontSet.AddString(Iso646Tab[n].name);
		if ( _tcsncmp(m_FontSetName, Iso646Tab[n].name, m_FontSetName.GetLength()) == 0 )
			m_FontSet.SetCurSel(n);

		m_DispSet.AddString(Iso646Tab[n].name);
		if ( _tcsncmp(m_DispSetName, Iso646Tab[n].name, m_DispSetName.GetLength()) == 0 )
			m_DispSet.SetCurSel(n);

		for ( i = 0 ; i < 13 ; i++ ) {
			if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CODESET1 + i)) == NULL )
				continue;
			str.Format(_T("U+%04X"), Iso646Tab[n].code[i]);
			if ( pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}

	if ( m_FontSet.GetCurSel() < 0 )
		m_FontSet.SetCurSel(0);		// US

	if ( m_DispSet.GetCurSel() < 0 )
		m_DispSet.SetCurSel(0);		// US

	for ( n = 0 ; n < 13 ; n++ ) {
		if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CODESET1 + n)) != NULL ) {
			str.Format(_T("U+%04X"), FullWidth[n]);
			if ( pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
			if ( n == 4 ) {
				pCombo->AddString(_T("U+2216"));	// SET MINUS
				pCombo->AddString(_T("U+2572"));	// BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT
				pCombo->AddString(_T("U+FFFFF"));	// BackSlash Mirror
			} else if ( n == 11 )
				pCombo->AddString(_T("U+223C"));	// TILDE OPERATOR
		}
		m_CharCode[n].Format(_T("U+%04X"), m_Iso646Tab[n]);

		SubclassComboBox(IDC_CODESET1 + n);
	}

	UpdateData(FALSE);

	m_ViewBox[0].GetClientRect(rect);
	ZeroMemory(&logfont, sizeof(logfont));
	logfont.lfHeight	= rect.Height();
	logfont.lfCharSet	= m_CharSet;
	_tcsncpy(logfont.lfFaceName, m_FontName, sizeof(logfont.lfFaceName) / sizeof(TCHAR));
	m_Font.CreateFontIndirect(&logfont);
	
	for ( n = 0 ; n < 26 ; n++ ) {
		m_ViewBox[n].SetFont(&m_Font);
		if ( n < 13 )
			continue;
		SetViewBox(n, m_Iso646Tab[n - 13]);
	}

	SetSaveProfile(_T("Iso646Dlg"));
	AddHelpButton(_T("#ISO646"));

	return TRUE;
}

void CIso646Dlg::OnOK()
{
	int n;
	DWORD code;
	CString str;
	BOOL bError = FALSE;

	UpdateData(TRUE);

	if ( (n = m_FontSet.GetCurSel()) >= 0 ) {
		m_FontSet.GetLBText(n, str);
		if ( (n = str.Find(_T(' '))) >= 0 )
			m_FontSetName = str.Left(n);
		else
			m_FontSetName = str;
	} else
		m_FontSetName = _T("US");

	if ( (n = m_DispSet.GetCurSel()) >= 0 ) {
		m_DispSet.GetLBText(n, str);
		if ( (n = str.Find(_T(' '))) >= 0 )
			m_DispSetName = str.Left(n);
		else
			m_DispSetName = str;
	} else
		m_DispSetName = _T("US");

	for ( n = 0 ; n < 13 ; n++ ) {
		CUniBlockTab::GetCode(m_CharCode[n], code);
		if ( code >= UNICODE_MAX ) {
			m_Iso646Tab[n] = Iso646Tab[0].code[n];
			bError = TRUE;
		} else
			m_Iso646Tab[n] = code;
	}

	if ( bError )
		::AfxMessageBox(_T("Error in the code Definition"), MB_ICONERROR);

	CDialogExt::OnOK();
}

void CIso646Dlg::OnCbnSelchangeFontDispSet()
{
	int n;
	int fn, dn;
	DWORD code;

	if ( (fn = m_FontSet.GetCurSel()) < 0 )
		return;

	if ( (dn = m_DispSet.GetCurSel()) < 0 )
		return;

	for ( n = 0 ; n < 13 ; n++ ) {
		if ( Iso646Tab[fn].code[n] != Iso646Tab[dn].code[n] ) {
			if ( Iso646Tab[0].code[n] != Iso646Tab[dn].code[n] )
				code = Iso646Tab[dn].code[n];	// Convert
			else
				code = FullWidth[n];			// FullWidth Convert
		} else
			code = Iso646Tab[0].code[n];		// No Convert

		m_CharCode[n].Format(_T("U+%04X"), code);
		SetViewBox(13 + n, code);
	}

	UpdateData(FALSE);
}

void CIso646Dlg::OnCbnUpdateCodeset(UINT nID)
{
	int n = nID - IDC_CODESET1;
	DWORD code;

	UpdateData(TRUE);

	CUniBlockTab::GetCode(m_CharCode[n], code);
	SetViewBox(13 + n, code);
}
void CIso646Dlg::OnCbnSelchangeCodeset(UINT nID)
{
	int n;
	DWORD code;
	CString str;
	CComboBox *pCombo;

	if ( (pCombo = (CComboBox *)GetDlgItem(nID)) == NULL )
		return;
	if ( (n = pCombo->GetCurSel()) < 0 )
		return;

	pCombo->GetLBText(n, str);
	CUniBlockTab::GetCode(str, code);
	SetViewBox(13 + (nID - IDC_CODESET1), code);
}

void CIso646Dlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if ( nIDCtl >= IDC_FONTVIEW1 && nIDCtl < (IDC_FONTVIEW1 + 26) ) {
		int num = nIDCtl - IDC_FONTVIEW1;
		CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
		CFont *pOldFont;
		CString str;
		CRect rect;
		CSize sz;
		BOOL bMirror = FALSE;

		m_ViewBox[num].GetWindowText(str);
		m_ViewBox[num].GetClientRect(rect);

		if ( str[0] == 0xDBBF && str[1] == 0xDFFF ) {	// U+FFFFF BackSlash Mirror !!!
			str = _T("/");
			bMirror = TRUE;
		}
		
		pOldFont = pDC->SelectObject(&m_Font);
		sz = pDC->GetTextExtent(str);

		pDC->SetTextColor(GetAppColor(COLOR_MENUTEXT));
		pDC->SetBkColor(GetAppColor(COLOR_MENU));

		pDC->FillSolidRect(rect, GetAppColor(COLOR_MENU));
		pDC->TextOut(rect.left + (rect.Width() - sz.cx) / 2, rect.top + (rect.Height() - sz.cy) / 2, str);

		if ( bMirror )
			pDC->StretchBlt(rect.Width() - 1, 0, 0 - rect.Width(), rect.Height(), pDC, rect.left, rect.top, rect.Width(), rect.Height(), SRCCOPY);

		pDC->SelectObject(pOldFont);
	} else
		CDialogExt::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
