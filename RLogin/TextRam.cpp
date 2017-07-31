// TextRam.cpp: CTextRam ÉNÉâÉXÇÃÉCÉìÉvÉäÉÅÉìÉeÅ[ÉVÉáÉì
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdarg.h>
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "PipeSock.h"
#include "GrapWnd.h"

#include <iconv.h>
#include <imm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CFontNode

CFontNode::CFontNode()
{
	Init();
}
void CFontNode::Init()
{
	m_Shift       = 0;
	m_ZoomW       = 100;
	m_ZoomH       = 100;
	m_Offset      = 0;
	m_CharSet     = 0;
	m_EntryName   = "";
	m_IContName   = "";
	m_IndexName   = "";
	m_Quality     = DEFAULT_QUALITY;
	m_Init        = FALSE;
	for ( int n = 0 ; n < 16 ; n++ ) {
		m_FontName[n] = "";
		m_Hash[n]     = 0;
	}
}
void CFontNode::SetArray(CStringArrayExt &array)
{
	array.RemoveAll();

	array.AddVal(m_Shift);
	array.AddVal(m_ZoomH);
	array.AddVal(m_Offset);
	array.AddVal(m_CharSet);
	array.Add(m_FontName[0]);
	array.Add(m_EntryName);
	array.Add(m_IContName);
	array.AddVal(m_ZoomW);
	for ( int n = 1 ; n < 16 ; n++ )
		array.Add(m_FontName[n]);
	array.AddVal(m_Quality);
	array.Add(m_IndexName);
}
void CFontNode::GetArray(CStringArrayExt &array)
{
	Init();

	if ( array.GetSize() < 7 )
		return;

	m_Shift		  = array.GetVal(0);
	m_ZoomH		  = array.GetVal(1);
	m_Offset	  = array.GetVal(2);
	m_CharSet	  = array.GetVal(3);
	m_FontName[0] = array.GetAt(4);
	SetHash(0);

	m_EntryName   = array.GetAt(5);
	m_IContName   = array.GetAt(6);
	m_ZoomW		  = (array.GetSize() > 7 ? array.GetVal(7) : m_ZoomH);

	if ( m_IContName.Compare("GB2312-80") == 0 )	// Debug!!
		m_IContName = "GB_2312-80";

	for ( int n = 1 ; n < 16 ; n++ ) {				// 8 - 23
		if ( array.GetSize() > (7 + n) ) {
			m_FontName[n] = array.GetAt(7 + n);
			SetHash(n);
		}
	}

	if ( array.GetSize() > (7 + 16) )
		m_Quality = array.GetVal(7 + 16);

	if ( array.GetSize() > (8 + 16) )
		m_IndexName = array.GetAt(8 + 16);

	m_Init = TRUE;
}
void CFontNode::SetHash(int num)
{
	m_Hash[num] = 0;
	LPCSTR p = m_FontName[num];
	while ( *p != '\0' )
		m_Hash[num] = m_Hash[num] * 31 + *(p++);
}
CFontChacheNode *CFontNode::GetFont(int Width, int Height, int Style, int FontNum)
{
	if ( m_FontName[FontNum].IsEmpty() )
		FontNum = 0;
	
	return ((CRLoginApp *)AfxGetApp())->m_FontData.GetFont(m_FontName[FontNum],
				Width * m_ZoomW / 100, Height * m_ZoomH / 100, m_CharSet, Style, m_Quality, m_Hash[FontNum]);
}
const CFontNode & CFontNode::operator = (CFontNode &data)
{
	m_Shift     = data.m_Shift;
	m_ZoomW     = data.m_ZoomW;
	m_ZoomH     = data.m_ZoomH;
	m_Offset    = data.m_Offset;
	m_CharSet   = data.m_CharSet;
	m_EntryName = data.m_EntryName;
	m_IContName = data.m_IContName;
	m_IndexName = data.m_IndexName;
	m_Quality   = data.m_Quality;
	m_Init      = data.m_Init;
	for ( int n = 0 ; n < 16 ; n++ ) {
		m_FontName[n] = data.m_FontName[n];
		m_Hash[n]     = data.m_Hash[n];
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CFontTab

CFontTab::CFontTab()
{
	m_pSection = "FontTab";
	Init();
}
void CFontTab::Init()
{
	int n, i;
	static const struct _FontInitTab	{
		char	*name;
		WORD	mode;
		char	*scs;
		char	bank;
		char	*iset;
		WORD	cset;
		WORD	zoomw;
		WORD	zoomh;
		WORD	offset;
		char	*font[2];
	} FontInitTab[] = {
		{ "VT100-GRAPHIC",			SET_94,		"0",	'\x00', "",					SYMBOL_CHARSET,		100,	100,	0,	{ "Tera Special", "" } },
		{ "IBM437-GR",				SET_94,		"1",	'\x80', "IBM437",			DEFAULT_CHARSET,	100,	100,	0,	{ "Arial Unicode MS", "" } },

		{ "ISO646-US",				SET_94,		"@",	'\x00', "ISO646-US",		ANSI_CHARSET,		100,	100,	0,	{ "Lucida Console", "" } },
		{ "ASCII(ANSI X3.4-1968)",	SET_94,		"B",	'\x00', "ANSI_X3.4-1968",	ANSI_CHARSET,		100,	100,	0,	{ "Lucida Console", "" } },
		{ "JIS X 0201-Kana",		SET_94,		"I",	'\x80', "JISX0201-1976",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },
		{ "JIS X 0201-Roman",		SET_94,		"J",	'\x00', "JISX0201-1976",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },
		{ "GB 1988-80",				SET_94,		"T",	'\x00', "GB_1988-80",		DEFAULT_CHARSET,	100,	100,	0,	{ "", "" } },

		{ "Russian",				SET_94,		"&5",	'\x00',	"CP866",			RUSSIAN_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "Turkish",				SET_94,		"%2",	'\x00',	"CP1254",			TURKISH_CHARSET,	100,	100,	0,	{ "", "" } },

		/*
		{ "ChineseBig5",			SET_94x94,	"?",	'\x00',	"CP950",			CHINESEBIG5_CHARSET,100,	100,	0,	{ "", "" } },
		{ "Johab",					SET_94x94,	"?",	'\x00',	"CP1361",			JOHAB_CHARSET,		100,	100,	0,	{ "", "" } },
		*/

		{ "LATIN1 (ISO8859-1)",		SET_96,		"A",	'\x80', "LATIN1",			EASTEUROPE_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "LATIN2 (ISO8859-2)",		SET_96,		"B",	'\x80', "LATIN2",			DEFAULT_CHARSET,	100,	100,	0,	{ "Arial Unicode MS", "" } },
		{ "LATIN3 (ISO8859-3)",		SET_96,		"C",	'\x80', "LATIN3",			DEFAULT_CHARSET,	100,	100,	0,	{ "Arial Unicode MS", "" } },
		{ "LATIN4 (ISO8859-4)",		SET_96,		"D",	'\x80', "LATIN4",			DEFAULT_CHARSET,	100,	100,	0,	{ "Arial Unicode MS", "" } },
		{ "CYRILLIC (ISO8859-5)",	SET_96,		"L",	'\x80', "ISO8859-5",		DEFAULT_CHARSET,	100,	100,	0,	{ "Arial Unicode MS", "" } },
		{ "ARABIC (ISO8859-6)",		SET_96,		"G",	'\x80', "ISO8859-6",		ARABIC_CHARSET,		100,	100,	0,	{ "", "" } },
		{ "GREEK (ISO8859-7)",		SET_96,		"F",	'\x80', "ISO8859-7",		GREEK_CHARSET,		100,	100,	0,	{ "", "" } },
		{ "HEBREW (ISO8859-8)",		SET_96,		"H",	'\x80', "ISO8859-8",		HEBREW_CHARSET,		100,	100,	0,	{ "", "" } },
		{ "LATIN5 (ISO8859-9)",		SET_96,		"M",	'\x80', "LATIN5",			DEFAULT_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "LATIN6 (ISO8859-10)",	SET_96,		"V",	'\x80', "LATIN6",			DEFAULT_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "THAI (ISO8859-11)",		SET_96,		"T",	'\x80', "ISO-8859-11",		THAI_CHARSET,		100,	100,	0,	{ "", "" } },
		{ "VIETNAMESE (ISO8859-12)",SET_96,		"Z",	'\x80', "CP1258",			VIETNAMESE_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "BALTIC (ISO8859-13)",	SET_96,		"Y",	'\x80', "ISO-8859-13",		BALTIC_CHARSET,		100,	100,	0,	{ "", "" } },
		{ "CELTIC (ISO8859-14)",	SET_96,		"_",	'\x80', "ISO-8859-14",		DEFAULT_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "LATIN9 (ISO8859-15)",	SET_96,		"b",	'\x80', "ISO-8859-15",		DEFAULT_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "ROMANIAN (ISO8859-16)",	SET_96,		"f",	'\x80', "ISO-8859-16",		DEFAULT_CHARSET,	100,	100,	0,	{ "", "" } },

		{ "JIS X 0208-1978",		SET_94x94,	"@",	'\x00', "JIS_X0208-1983",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },
		{ "GB2312-1980",			SET_94x94,	"A",	'\x00', "GB_2312-80",		GB2312_CHARSET,		100,	100,	0,	{ "", "" } },
		{ "JIS X 0208-1983",		SET_94x94,	"B",	'\x00', "JIS_X0208-1983",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },
		{ "KSC5601-1987",			SET_94x94,	"C",	'\x00', "KS_C_5601-1987",	HANGEUL_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "JIS X 0212-1990",		SET_94x94,	"D",	'\x00', "JIS_X0212-1990",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },
		{ "ISO-IR-165",				SET_94x94,	"E",	'\x00', "ISO-IR-165",		DEFAULT_CHARSET,	100,	100,	0,	{ "", "" } },
		{ "JIS X 0213-2000.1",		SET_94x94,	"O",	'\x00', "JIS_X0213-2000.1",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },
		{ "JIS X 0213-2000.2",		SET_94x94,	"P",	'\x00', "JIS_X0213-2000.2",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },
		{ "JIS X 0213-2004.1",		SET_94x94,	"Q",	'\x00', "JIS_X0213-2000.1",	SHIFTJIS_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },

		{ "UNICODE",				SET_UNICODE,"",		'\x00', "UCS-4BE",			DEFAULT_CHARSET,	100,	100,	0,	{ "ÇlÇr ÉSÉVÉbÉN", "ÇlÇr ñæí©" } },

		{ NULL, 0, 0x00, NULL },
	};

	for ( n = 0 ; n < CODE_MAX ; n++ )
		m_Data[n].Init();

	for ( n = 0 ; FontInitTab[n].name != NULL ; n++ ) {
		i = IndexFind(FontInitTab[n].mode, FontInitTab[n].scs);
		m_Data[i].m_Shift       = FontInitTab[n].bank;
		m_Data[i].m_ZoomW       = FontInitTab[n].zoomw;
		m_Data[i].m_ZoomH       = FontInitTab[n].zoomh;
		m_Data[i].m_Offset      = FontInitTab[n].offset;
		m_Data[i].m_CharSet     = FontInitTab[n].cset;
		m_Data[i].m_FontName[0] = FontInitTab[n].font[0];
		m_Data[i].m_FontName[1] = FontInitTab[n].font[1];
		m_Data[i].m_EntryName   = FontInitTab[n].name;
		m_Data[i].m_IContName   = FontInitTab[n].iset;
		m_Data[i].m_IndexName   = FontInitTab[n].scs;
		m_Data[i].m_Init        = FALSE;
		m_Data[i].SetHash(0);
		m_Data[i].SetHash(1);
	}
}
void CFontTab::SetArray(CStringArrayExt &array)
{
	int n;
	CString str;
	CStringArrayExt tmp;

	array.RemoveAll();
	array.Add("-1");
	for ( n = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_Data[n].m_EntryName.IsEmpty() )
			continue;
		m_Data[n].SetArray(tmp);
		str.Format("%d", n);
		tmp.InsertAt(0, str);
		tmp.SetString(str);
		array.Add(str);
	}
}
void CFontTab::GetArray(CStringArrayExt &array)
{
	int n, i;
	CStringArrayExt tmp;

	if ( array.GetSize() == 0 || atoi(array[0]) != (-1) )
		Init();
	else {
		for ( n = 0 ; n < CODE_MAX ; n++ )
			m_Data[n].Init();
	}

	for ( n = 0 ; n < array.GetSize() ; n++ ) {
		tmp.GetString(array[n]);
		if ( tmp.GetSize() < 1 )
			continue;
		if ( (i = tmp.GetVal(0)) < 0 || i >= CODE_MAX )
			continue;
		tmp.RemoveAt(0);
		m_Data[i].GetArray(tmp);
		if ( m_Data[i].m_IndexName.IsEmpty() ) {
			if ( i == SET_UNICODE )
				m_Data[i].m_IndexName = "Unicode";
			else
				m_Data[i].m_IndexName = (CHAR)(i & 0xFF);
		}
	}
}
const CFontTab & CFontTab::operator = (CFontTab &data)
{
	for ( int n = 0 ; n < CODE_MAX ; n++ )
		m_Data[n] = data.m_Data[n];
	return *this;
}
int CFontTab::Find(LPCSTR entry)
{
	for ( int n = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_Data[n].m_EntryName.Compare(entry) == 0 )
			return n;
	}
	return 0;
}
int CFontTab::IndexFind(int code, LPCSTR name)
{
	int n, i;

	if ( name[1] == '\0' && name[0] >= '\x30' && name[0] <= '\x7E' )
		return (code | name[0]);
	else if ( strcmp(name, "Unicode") == 0 )
		return SET_UNICODE;

	for ( n = i = 0 ;  n < (255 - 0x4F) ; n++ ) {
		i = code | (n + (n < 0x30 ? 0 : 0x4F));			// 0x00-0x2F or 0x7F-0xFF
		if ( m_Data[i].m_IndexName.IsEmpty() )
			break;
		else if ( m_Data[i].m_IndexName.Compare(name) == 0 )
			return i;
	}
	return i;
}
void CFontTab::IndexRemove(int idx)
{
	int n, i;
	int bank = idx & SET_MASK;
	int code = idx & 0xFF;

	m_Data[idx].Init();

	if ( code >= 0x30 && code <= 0x7E )
		return;

	while ( code < 255 ) {
		i = bank | code;
		if ( ++code == 0x30 )
			code = 0x7F;
		n = bank | code;
		if ( n == 255 || m_Data[n].m_IndexName.IsEmpty() ) {
			m_Data[i].Init();
			break;
		}
		m_Data[i] = m_Data[n];
	}
}

//////////////////////////////////////////////////////////////////////
// CTextRam
//////////////////////////////////////////////////////////////////////

CTextRam::CTextRam()
{
	m_pDocument = NULL;
	m_VRam = NULL;
	m_Cols = m_ColsMax = 80;
	m_LineUpdate = 0;
	m_Lines = 25;
	m_Page = 0;
	m_TopY = 0;
	m_BtmY = m_Lines;
	m_LeftX = 0;
	m_RightX = m_Cols;
	m_HisMax = 100;
	m_HisPos = 0;
	m_HisLen = 0;
	m_HisUse = 0;
	m_pTextSave = m_pTextStack = NULL;
	m_DispCaret = 002;
	m_TypeCaret = 0; 
	m_UpdateRect.SetRectEmpty();
	m_UpdateFlag = FALSE;
	m_DelayMSec = 0;
	m_HisFile.Empty();
	m_LogFile.Empty();
	m_KeepAliveSec = 0;
	m_RecvCrLf = m_SendCrLf = 0;
	m_LastChar = 0;
	m_LastPos  = 0;
	m_DropFileMode = 0;
	m_WordStr.Empty();
	m_Exact = FALSE;
	m_MouseTrack = MOS_EVENT_NONE;
	m_MouseRect.SetRectEmpty();
	m_Loc_Mode  = 0;
	m_Loc_Pb    = 0;
	m_Loc_LastX = 0;
	m_Loc_LastY = 0;
	memset(m_MetaKeys, 0, sizeof(m_MetaKeys));
	m_TitleMode = WTTL_ENTRY;
	m_StsFlag = FALSE;
	m_StsMode = 0;
	m_StsLed  = 0;
	m_VtLevel = 65;
	m_TermId  = 10;
	m_LangMenu = 0;
	SetRetChar(FALSE);

	m_LineEditMode = FALSE;
	m_LineEditPos  = 0;
	m_LineEditBuff.Clear();
	m_LineEditMapsInit = 0;

	m_LogCharSet[0] = 1;
	m_LogCharSet[1] = 0;
	m_LogCharSet[2] = 3;
	m_LogCharSet[3] = 2;

	m_MouseMode[0] = 0;
	m_MouseMode[1] = 1;
	m_MouseMode[2] = 4;
	m_MouseMode[3] = 16;

	m_Tek_Top = m_Tek_Free = NULL;
	m_pTekWnd = NULL;

	fc_Init(EUC_SET);

	m_pSection = "TextRam";
	m_MinSize = 16;
	Serialize(FALSE);
}
CTextRam::~CTextRam()
{
	CTextSave *pSave;

	if ( m_VRam != NULL )
		delete m_VRam;

	while ( (pSave = m_pTextSave) != NULL ) {
		m_pTextSave = pSave->m_Next;
		delete pSave->m_VRam;
		delete pSave;
	}
	while ( (pSave = m_pTextStack) != NULL ) {
		m_pTextStack = pSave->m_Next;
		delete pSave->m_VRam;
		delete pSave;
	}

	TEKNODE *tp;
	while ( (tp = m_Tek_Top) != NULL ) {
		m_Tek_Top = tp->next;
		delete tp;
	}
	while ( (tp = m_Tek_Free) != NULL ) {
		m_Tek_Free = tp->next;
		delete tp;
	}

	if ( m_pTekWnd != NULL )
		m_pTekWnd->DestroyWindow();

	while ( m_GrapWndTab.GetSize() > 0 )
		((CWnd *)(m_GrapWndTab[0]))->DestroyWindow();
}

//////////////////////////////////////////////////////////////////////
// Window Function
//////////////////////////////////////////////////////////////////////

void CTextRam::InitText(int Width, int Height)
{
	VRAM *tmp;
	int n, x;
	int cx, cy, ox, oy, cw, ch;
	int cols, lines;
	int colsmax, hismax;

	if ( IS_ENABLE(m_AnsiOpt, TO_RLFONT) ) {
		ch = m_DefFontSize;

		if ( (cw = ch / 2) <= 0 )
			cw = 1;

		if ( (cols = Width / cw) < 10 )
			cols = 10;
		else if ( cols > COLS_MAX )
			cols = COLS_MAX;

		if ( (lines = Height / ch) <= 0 )
			lines = 1;

	} else {
		if ( (cols = m_DefCols[0]) > COLS_MAX )
			cols = COLS_MAX;
		else if ( cols < 20 )
			cols = 20;

		if ( (cw  = Width / cols) <= 0 )
			cw = 1;

		if ( (lines = Height / (cw * 2)) <= 0 )
			lines = 1;

		if ( IsOptValue(TO_DECCOLM, 1) == 1 ) {
			if ( (cols = m_DefCols[1]) > COLS_MAX )
				cols = COLS_MAX;
			else if ( cols < 20 )
				cols = 20;

			if ( (cw  = Width / cols) <= 0 )
				cw = 1;
		}
	}

	if ( lines > LINE_MAX )
		lines = LINE_MAX;

	if ( (hismax = m_DefHisMax)  < (lines * 5) )
		hismax = lines * 5;
	else if ( hismax > HIS_MAX )
		hismax = HIS_MAX;

	if ( cols == m_Cols && hismax == m_HisMax && m_VRam != NULL ) {
		if ( lines == m_Lines )
			return;

		cx = m_CurX;
		cy = m_CurY;
		ox = m_Cols;
		oy = m_Lines;
		colsmax = m_ColsMax;

		m_HisPos += (m_Lines - lines);

	} else {
		colsmax = (m_ColsMax > cols && m_LineUpdate < m_Lines ? m_ColsMax : cols);
		tmp = new VRAM[colsmax * hismax];
		for ( n = 0 ; n < (colsmax * hismax) ; n++ )
			tmp[n] = m_DefAtt;

		if ( m_VRam != NULL ) {
			cx = (m_ColsMax < colsmax ? m_ColsMax : colsmax);
			cy = (m_HisMax < hismax ? m_HisMax : hismax);
			for ( n = 1 ; n <= cy ; n++ )
				memcpy(tmp + (hismax - n) * colsmax, GETVRAM(0, m_Lines - n), sizeof(VRAM) * cx);
			delete m_VRam;
			cx = m_CurX;
			cy = m_CurY;
			ox = m_Cols;
			oy = m_Lines;
		} else {
			cx = 0;
			cy = 0;
			ox = cols;
			oy = lines;
		}

		m_VRam   = tmp;
		m_HisMax = hismax;
		m_HisPos = hismax - lines;
	}

	m_Cols   = cols;
	m_Lines  = lines;
	m_ColsMax = colsmax;
	m_LineUpdate = 0;

	if ( m_HisLen <= oy && m_Lines < oy ) {
		for ( n = 0 ; n < oy ; n++ ) {
			tmp = GETVRAM(0, m_Lines - oy + n);
			for ( x = 0 ; x < m_Cols ; x++ ) {
				if ( tmp[x].ch != 0 )
					break;
			}
			if ( x < m_Cols )
				break;
			m_HisLen--;
		}
	}

	if ( m_HisLen < m_Lines )
		m_HisLen = m_Lines;
	else if ( m_HisLen >= m_HisMax )
		m_HisLen = m_HisMax - 1;

	RESET();

	if ( (m_CurX = cx) >= m_Cols )
		m_CurX = m_Cols - 1;

	if ( (m_CurY = m_Lines - oy + cy) < 0 ) {
		m_HisPos += m_CurY;
		if ( m_HisLen == m_Lines )
			m_HisLen += cy;
		else
			m_HisLen += m_CurY;
		m_CurY = 0;
	}

	if ( m_Cols != ox || m_Lines != oy )
		m_pDocument->SocketSendWindSize(m_Cols, m_Lines);

	m_RecvCrLf = IsOptValue(TO_RLRECVCR, 2);
	m_SendCrLf = IsOptValue(TO_RLECHOCR, 2);

//	DISPUPDATE();
//	FLUSH();
}
void CTextRam::InitScreen(int cols, int lines)
{
	if ( m_VRam == NULL )
		return;

	int n, cx, cy;
	int colsmax = (m_ColsMax > cols ? m_ColsMax : cols);
	VRAM *tmp;
	CRect rect, box;
	CWnd *pWnd = ::AfxGetMainWnd();

	if ( IS_ENABLE(m_AnsiOpt, TO_RLFONT) ) {
		if ( pWnd->IsIconic() || pWnd->IsZoomed() )
			return;

		pWnd->GetWindowRect(rect);
		pWnd->GetClientRect(box);
		cx = (rect.Width()  - box.Width())  + box.Width()  * cols  / m_Cols;
		cy = (rect.Height() - box.Height()) + box.Height() * lines / m_Lines;
		pWnd->SetWindowPos(NULL, rect.left, rect.top, cx, cy, SWP_SHOWWINDOW);

	} else if ( pWnd->IsIconic() || pWnd->IsZoomed() || !IsOptEnable(TO_RLNORESZ) ) {
		tmp = new VRAM[colsmax * m_HisMax];
		for ( n = 0 ; n < (colsmax * m_HisMax) ; n++ )
			tmp[n] = m_DefAtt;

		cx = (m_ColsMax < colsmax ? m_ColsMax : colsmax);
		for ( n = 1 ; n <= m_HisMax ; n++ )
			memcpy(tmp + (m_HisMax - n) * colsmax, GETVRAM(0, m_Lines - n), sizeof(VRAM) * cx);

		delete m_VRam;
		m_VRam    = tmp;
		m_HisPos  = m_HisMax - lines;
		m_Cols    = cols;
		m_ColsMax = colsmax;
		m_Lines   = lines;
		m_LineUpdate = 0;

		if ( m_CurX >= m_Cols )
			m_CurX = m_Cols - 1;

		if ( m_CurY >= m_Lines )
			m_CurY = m_Lines - 1;

		m_pDocument->SocketSendWindSize(m_Cols, m_Lines);
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITSIZE, NULL);

	} else {
		pWnd->GetWindowRect(rect);
		pWnd->GetClientRect(box);
		cx = (rect.Width()  - box.Width())  + box.Width()  * cols  / m_Cols;
		cy = (rect.Height() - box.Height()) + box.Height() * lines / m_Lines;
		pWnd->SetWindowPos(NULL, rect.left, rect.top, cx, cy, SWP_SHOWWINDOW);
	}
}

void CTextRam::OpenHisFile()
{
	int n;
	int num = 2;
	CString name = m_HisFile;
	CString base = name;
	
	if ( m_pDocument != NULL )
		m_pDocument->EntryText(name);

	if ( (n = name.ReverseFind('.')) >= 0 )
		base = name.Left(n);

	while ( num < 20 && !m_HisFhd.Open(name, CFile::modeReadWrite | CFile::modeCreate | CFile::modeNoTruncate | CFile::shareExclusive) )
		name.Format("%s(%d).rlh", base, num++);
}
void CTextRam::InitHistory()
{
	m_LineEditMapsInit = 0;

	if ( !IsOptEnable(TO_RLHISFILE) || m_HisFile.IsEmpty() || m_VRam == NULL )
		return;

	OpenHisFile();

	if ( m_HisFhd.m_hFile == NULL )
		return;

	VRAM *vp = NULL;

	try {
		CSpace spc;
		DWORD mx, my, nx;
		char head[5];
		BYTE tmp[8];

		m_HisFhd.SeekToBegin();
		if ( m_HisFhd.Read(head, 4) != 4 )
			AfxThrowFileException(CFileException::endOfFile);
		if ( strncmp(head, "RLH", 3) != 0 )
			AfxThrowFileException(CFileException::fileNotFound);
		if ( m_HisFhd.Read(&mx, sizeof(DWORD)) != sizeof(DWORD) )
			AfxThrowFileException(CFileException::endOfFile);
		if ( m_HisFhd.Read(&my, sizeof(DWORD)) != sizeof(DWORD) )
			AfxThrowFileException(CFileException::endOfFile);

		vp = new VRAM[mx];
		nx = ((int)mx < m_Cols ? mx : m_Cols);

		if ( head[3] == '2' ) {
			for ( DWORD n = 0 ; n < my && m_HisLen < (m_HisMax - 1) ; n++ ) {
				if ( m_HisFhd.Read(vp, sizeof(VRAM) * mx) != (sizeof(VRAM) * mx) )
					AfxThrowFileException(CFileException::endOfFile);
				memcpy(GETVRAM(0, (-1) - n), vp, sizeof(VRAM) * nx);
				m_HisLen++;
			}
		} else if ( head[3] == '1' ) {
			for ( DWORD n = 0 ; n < my && m_HisLen < (m_HisMax - 1) ; n++ ) {
				for ( int x = 0 ; x < mx ; x++ ) {
					if ( m_HisFhd.Read(tmp, 8) != 8 )
						AfxThrowFileException(CFileException::endOfFile);
					vp[x].ch = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
					vp[x].md = tmp[4] | (tmp[5] << 8);
					vp[x].em = tmp[5] >> 2;
					vp[x].dm = tmp[5] >> 4;
					vp[x].cm = tmp[5] >> 6;
					vp[x].at = tmp[6];
					vp[x].fc = tmp[7] & 0x0F;
					vp[x].bc = tmp[7] >> 4;
				}
				memcpy(GETVRAM(0, (-1) - n), vp, sizeof(VRAM) * nx);
				m_HisLen++;
			}
		} else
			AfxThrowFileException(CFileException::fileNotFound);

		delete vp;

		m_LineEditMapsTab[2].RemoveAll();
		if ( m_HisFhd.Read(&mx, sizeof(DWORD)) == sizeof(DWORD) ) {
			for ( nx = 0 ; nx < mx ; nx++ ) {
				if ( m_HisFhd.Read(&my, sizeof(DWORD)) != sizeof(DWORD) || my < 0 || my > 1024 )
					break;
				spc.SetSize(my);
				if ( m_HisFhd.Read(spc.GetPtr(), my) != my )
					break;
				m_LineEditHis.Add(spc);
				m_LineEditMapsTab[2].AddWStrBuf(spc.GetPtr(), spc.GetSize());
			}
		}

	} catch(...) {
		if ( vp != NULL )
			delete vp;
	}
}
void CTextRam::SaveHistory()
{
	if ( !IsOptEnable(TO_RLHISFILE) || m_HisFile.IsEmpty() || m_VRam == NULL )
		return;

	if ( m_HisFhd.m_hFile == CFile::hFileNull )
		OpenHisFile();

	try {
		VRAM *vp;
		DWORD n, i;

		m_HisFhd.SeekToBegin();
		m_HisFhd.Write("RLH2", 4);
		n = m_Cols; m_HisFhd.Write(&n, sizeof(DWORD));
		n = m_HisLen; m_HisFhd.Write(&n, sizeof(DWORD));
		for ( n = 0 ; (int)n < m_HisLen ; n++ ) {
			vp = GETVRAM(0, m_Lines - n - 1);
			m_HisFhd.Write(vp, sizeof(VRAM) * m_Cols);
		}

		n = (DWORD)m_LineEditHis.GetSize(); m_HisFhd.Write(&n, sizeof(DWORD));
		for ( i = 0 ; i < (DWORD)m_LineEditHis.GetSize() ; i++ ) {
			n = (DWORD)m_LineEditHis.GetAt(i).GetSize(); m_HisFhd.Write(&n, sizeof(DWORD));
			m_HisFhd.Write(m_LineEditHis.GetAt(i).GetPtr(), n);
		}

	} catch(...) {
		AfxMessageBox("ÉqÉXÉgÉäÅ[ÉoÉbÉNÉAÉbÉvÇ…é∏îsÇµÇ‹ÇµÇΩ");
	}
}
void CTextRam::HisRegCheck(int ch, DWORD pos)
{
	int i, a;
	CRegExRes res;

	if ( m_MarkReg.MatchChar(ch, pos, &res) && (res.m_Status == REG_MATCH || res.m_Status == REG_MATCHOVER) ) {
		for ( i = 0 ; i < res.GetSize() ; i++ ) {
			for ( a = res[i].m_SPos ; a < res[i].m_EPos ; a++ ) {
				if ( res.m_Idx[a] != 0xFFFFFFFF )
					m_VRam[res.m_Idx[a]].at |= ATT_MARK;
			}
		}
	}
}
int CTextRam::HisRegMark(LPCSTR str)
{
	int n, x;
	VRAM *vp;

	m_MarkPos = m_HisPos + m_Lines - m_HisLen;

	while ( m_MarkPos < 0 )
		m_MarkPos += m_HisMax;

	while ( m_MarkPos >= m_HisMax )
		m_MarkPos -= m_HisMax;

	if ( str == NULL || *str == '\0' ) {
		for ( n = 0 ; n < m_HisLen ; n++ ) {
			vp = m_VRam + m_ColsMax * m_MarkPos;
			for ( x = 0 ; x < m_Cols ; x++ )
				vp[x].at &= ~ATT_MARK;
			while ( ++m_MarkPos >= m_HisMax )
				m_MarkPos -= m_HisMax;
		}
		return 0;
	}

	m_MarkReg.Compile(str);
	m_MarkReg.MatchCharInit();

	m_MarkLen = 0;
	m_MarkEol = TRUE;

	return m_HisLen;
}
int CTextRam::HisRegNext()
{
	int n, x, ex, ch, mx;
	VRAM *vp;

	for ( mx = m_MarkLen + 500 ; m_MarkLen < mx && m_MarkLen < m_HisLen ; m_MarkLen++ ) {
		vp = m_VRam + m_ColsMax * m_MarkPos;
		for ( ex = m_Cols - 1 ; ex >= 0 ; ex-- ) {
			if ( vp[ex].ch >= ' ' )
				break;
			vp[ex].at &= ~ATT_MARK;
		}

		if ( m_MarkEol )
			HisRegCheck(L'\n', 0xFFFFFFFF);

		for ( x = 0 ; x <= ex ; x += n ) {
			vp[x].at &= ~ATT_MARK;
			if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].cm) && IS_2BYTE(vp[x + 1].cm) ) {
				ch = vp[x].ch;
				n = 2;
			} else if ( !IS_ASCII(vp[x].cm) || vp[x].ch == 0 ) {
				ch = ' ';
				n = 1;
			} else {
				ch = vp[x].ch;
				n = 1;
			}

			if ( (ch & 0xFFFF0000) != 0 )
				HisRegCheck(ch >> 16, m_ColsMax * m_MarkPos + x);
			HisRegCheck(ch & 0xFFFF, m_ColsMax * m_MarkPos + x);
		}

		if ( x < m_Cols ) {
			HisRegCheck(L'\r', 0xFFFFFFFF);
			m_MarkEol = TRUE;
		} else
			m_MarkEol = FALSE;

		while ( ++m_MarkPos >= m_HisMax )
			m_MarkPos -= m_HisMax;
	}

	return m_MarkLen;
}
int CTextRam::HisMarkCheck(int top, int line, class CRLoginView *pView)
{
	int x, y;
	VRAM *vp;

	for ( y = 0 ; y < line ; y++ ) {
		vp = GETVRAM(0, top + y);
		for ( x = 0 ; x < m_Cols ; x++ ) {
			if ( (vp->at & ATT_MARK) != 0 )
				return TRUE;
		}
	}
	return FALSE;
}

static const COLORREF DefColTab[16] = {
		RGB(  0,   0,   0),	RGB(196,   0,   0),
		RGB(  0, 196,   0),	RGB(196, 196,   0),
		RGB(  0,   0, 196),	RGB(196,   0, 196),
		RGB(  0, 196, 196),	RGB(196, 196, 196),

		RGB(128, 128, 128),	RGB(255,   0,   0),
		RGB(  0, 255,   0),	RGB(255, 255,   0),
		RGB(  0,   0, 255),	RGB(255,   0, 255),
		RGB(  0, 255, 255),	RGB(255, 255, 255),
	};
static const WORD DefBankTab[5][4] = {
	/* EUC */
	{ { SET_94    | 'J' }, 	{ SET_94x94 | 'Q' },
	  { SET_94    | 'I' }, 	{ SET_94x94 | 'P' } },
	/* SJIS */
	{ { SET_94    | 'J' }, 	{ SET_94    | 'I' },
	  { SET_94x94 | 'Q' }, 	{ SET_94x94 | 'P' } },
	/* ASCII */
	{ { SET_94    | 'B' }, 	{ SET_94    | '1' },
	  { SET_94    | 'J' }, 	{ SET_94    | 'I' } },
	/* UTF-8 */
	{ { SET_94    | 'B' }, 	{ SET_96    | 'A' },
	  { SET_94    | 'J' }, 	{ SET_94    | 'I' } },
	/* BIG5 */
	{ { SET_94    | 'T' }, 	{ SET_94    | '1' },
	  { SET_94x94 | 'A' }, 	{ SET_94x94 | 'E' } },
	};
static const VRAM TempAtt = {
	//  ch  at  ft  md  dm  cm  em  fc  bc
		0,  0,  0,  0,  0,  0,  0,  7,  0
};
static const char *DropCmdTab[] = {
	//	Non				BPlus			XModem			YModem
		"",				"bp -d %s\\r",	"rx %s\\r",		"rb\\r",
	//	ZModem			SCP				Kermit
		"rz\\r",		"scp -t %s",	"kermit -i -r",	"",
	};

void CTextRam::Init()
{
	m_DefCols[0]	= 80;
	m_DefCols[1]	= 132;
	m_Page          = 0;
	m_DefHisMax		= 2000;
	m_DefFontSize	= 16;
	m_KanjiMode		= EUC_SET;
	m_BankGL		= 0;
	m_BankGR		= 1;
	m_DefAtt		= TempAtt;
	memcpy(m_DefColTab, DefColTab, sizeof(m_DefColTab));
	memcpy(m_ColTab, m_DefColTab, sizeof(m_ColTab));
	memset(m_AnsiOpt, 0, sizeof(DWORD) * 16);
	EnableOption(TO_ANSISRM);	//  12 SRM Set Send/Receive mode (Local echo off)
	EnableOption(TO_DECANM);	//  ?2 ANSI/VT52 Mode
	EnableOption(TO_DECAWM);	//  ?7 Autowrap mode
	EnableOption(TO_DECTCEM);	// ?25 Text Cursor Enable Mode
	EnableOption(TO_XTMCUS);	// ?41 XTerm tab bug fix
	EnableOption(TO_XTMRVW);	// ?45 XTerm Reverse-wraparound mod
	EnableOption(TO_DECBKM);	// ?67 Backarrow key mode (BS)
	memcpy(m_DefAnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_DefBankTab, DefBankTab, sizeof(m_DefBankTab));
	memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));
	m_SendCharSet[0] = "EUCJP-MS";
	m_SendCharSet[1] = "SHIFT-JIS";
	m_SendCharSet[2] = "ISO-2022-JP";
	m_SendCharSet[3] = "UTF-8";
	m_SendCharSet[4] = "BIG-5";
	m_WheelSize      = 2;
	m_BitMapFile     = "";
	m_DelayMSec      = 0;
	m_HisFile        = "";
	m_KeepAliveSec   = 0;
	m_DropFileMode   = 0;
	for ( int n = 0 ; n < 8 ; n++ )
		m_DropFileCmd[n] = DropCmdTab[n];
	m_WordStr        = "\\/._";
	m_MouseTrack     = MOS_EVENT_NONE;
	m_MouseRect.SetRectEmpty();
	m_MouseMode[0]   = 0;
	m_MouseMode[1]   = 1;
	m_MouseMode[2]   = 4;
	m_MouseMode[3]   = 16;
	memset(m_MetaKeys, 0, sizeof(m_MetaKeys));
	m_TitleMode = WTTL_ENTRY;
	m_VtLevel = 65;
	m_TermId  = 10;
	m_LangMenu = 0;
	m_StsFlag = FALSE;
	m_StsMode = 0;
	m_StsLed  = 0;
	SetRetChar(FALSE);
	for ( int n = 0 ; n < 64 ; n++ )
		m_Macro[n].Clear();

	m_ProcTab.Init();

	RESET();
}
void CTextRam::SetArray(CStringArrayExt &array)
{
	CString str;
	CStringArrayExt tmp;

	array.RemoveAll();

	array.AddVal(m_DefCols[0]);
	array.AddVal(m_DefHisMax);
	array.AddVal(m_DefFontSize);
	array.AddVal(m_KanjiMode);
	array.AddVal(m_BankGL);
	array.AddVal(m_BankGR);
	array.AddBin(&m_DefAtt, sizeof(VRAM));
	array.AddBin(m_DefColTab,  sizeof(m_DefColTab));
	array.AddBin(m_AnsiOpt, sizeof(m_AnsiOpt));
	array.AddBin(m_DefBankTab, sizeof(m_DefBankTab));
	array.Add(m_SendCharSet[0]);
	array.Add(m_SendCharSet[1]);
	array.Add(m_SendCharSet[2]);
	array.Add(m_SendCharSet[3]);
	array.AddVal(m_WheelSize);
	array.Add(m_BitMapFile);
	array.AddVal(m_DelayMSec);
	array.Add(m_HisFile);
	array.AddVal(m_KeepAliveSec);
	array.Add(m_LogFile);
	array.AddVal(m_DefCols[1]);
	array.AddVal(m_DropFileMode);
	for ( int n = 0 ; n < 8 ; n++ )
		array.Add(m_DropFileCmd[n]);
	array.Add(m_WordStr);
	for ( int n = 0 ; n < 4 ; n++ )
		array.AddVal(m_MouseMode[n]);
	array.AddBin(m_MetaKeys,  sizeof(m_MetaKeys));

	m_ProcTab.SetArray(tmp);
	tmp.SetString(str, ';');
	array.Add(str);

	array.AddVal(4);	// AnsiOpt Bugfix
	array.AddVal(m_TitleMode);
	array.Add(m_SendCharSet[4]);
}
void CTextRam::GetArray(CStringArrayExt &array)
{
	int n;
	BYTE tmp[16];
	CStringArrayExt ext;

	Init();

	m_DefCols[0]  = array.GetVal(0);
	m_DefHisMax   = array.GetVal(1);
	m_DefFontSize = array.GetVal(2);
	m_KanjiMode   = array.GetVal(3);
	m_BankGL      = array.GetVal(4);
	m_BankGR      = array.GetVal(5);

	if ( (n = array.GetBin(6, tmp, 16)) == sizeof(VRAM) )
		memcpy(&m_DefAtt, tmp, sizeof(VRAM));
	else if ( n == 8 ) {
		memset(&m_DefAtt, 0, sizeof(VRAM));
		m_DefAtt.at = tmp[6];
		m_DefAtt.fc = tmp[7] & 0x0F;
		m_DefAtt.bc = tmp[7] >> 4;
	}

	array.GetBin(7, m_DefColTab,  sizeof(m_DefColTab));
	memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));
	array.GetBin(8, m_DefAnsiOpt, sizeof(m_DefAnsiOpt));
	memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));

	memcpy(m_DefBankTab, DefBankTab, sizeof(m_DefBankTab));
	array.GetBin(9, m_DefBankTab, sizeof(m_DefBankTab));
	memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));

	m_SendCharSet[0] = array.GetAt(10);
	m_SendCharSet[1] = array.GetAt(11);
	m_SendCharSet[2] = array.GetAt(12);
	m_SendCharSet[3] = array.GetAt(13);

	m_WheelSize    = array.GetVal(14);
	m_BitMapFile   = array.GetAt(15);

	m_DelayMSec    = (array.GetSize() > 16 ? array.GetVal(16) : 0);
	m_HisFile      = (array.GetSize() > 17 ? array.GetAt(17) : "");
	m_KeepAliveSec = (array.GetSize() > 18 ? array.GetVal(18) : 0);
	m_LogFile      = (array.GetSize() > 19 ? array.GetAt(19) : "");
	m_DefCols[1]   = (array.GetSize() > 20 ? array.GetVal(20) : 132);

	m_DropFileMode = (array.GetSize() > 21 ? array.GetVal(21) : 0);
	for ( n = 0 ; n < 8 ; n++ )
		m_DropFileCmd[n]  = (array.GetSize() > (22 + n) ? array.GetAt(22 + n) : DropCmdTab[n]);

	m_WordStr      = (array.GetSize() > 30 ? array.GetAt(30) : "\\/._");

	m_MouseMode[0] = (array.GetSize() > 31 ? array.GetVal(31) : 0);
	m_MouseMode[1] = (array.GetSize() > 32 ? array.GetVal(32) : 1);
	m_MouseMode[2] = (array.GetSize() > 33 ? array.GetVal(33) : 4);
	m_MouseMode[3] = (array.GetSize() > 34 ? array.GetVal(34) : 16);

	if ( array.GetSize() > 35 )
		array.GetBin(35, m_MetaKeys, sizeof(m_MetaKeys));

	if ( array.GetSize() > 36 ) {
		ext.GetString(array.GetAt(36), ';');
		m_ProcTab.GetArray(ext);
	}

	n = (array.GetSize() > 37 ? array.GetVal(37) : 0);
	if ( n < 1 ) {
		m_AnsiOpt[TO_DECANM / 32] ^= (1 << (TO_DECANM % 32));	//  ?2 ANSI/VT52 Mode
		EnableOption(TO_DECTCEM);	// ?25 Text Cursor Enable Mode
	}
	if ( n < 2 )
		EnableOption(TO_XTMCUS);	// ?41 XTerm tab bug fix
	if ( n < 3 ) {
		EnableOption(TO_DECBKM);	// ?67 Backarrow key mode (BS
		EnableOption(TO_ANSISRM);	//  12 SRM Set Send/Receive mode (Local echo off)
	}
	if ( n < 4 )
		EnableOption(TO_ANSISRM);	//  12 SRM Set Send/Receive mode (Local echo off)

	DisableOption(TO_IMECTRL);
	memcpy(m_DefAnsiOpt, m_AnsiOpt, sizeof(m_DefAnsiOpt));

	if ( array.GetSize() > 38 )
		m_TitleMode = array.GetVal(38);

	if ( array.GetSize() > 39 )
		m_SendCharSet[4] = array.GetAt(39);

	RESET();
}
void CTextRam::Serialize(int mode)
{
	COptObject::Serialize(mode);
	m_FontTab.Serialize(mode);
}
void CTextRam::Serialize(int mode, CBuffer &buf)
{
	COptObject::Serialize(mode, buf);
	m_FontTab.Serialize(mode, buf);
}
void CTextRam::Serialize(CArchive& ar)
{
	COptObject::Serialize(ar);
	m_FontTab.Serialize(ar);
}
const CTextRam & CTextRam::operator = (CTextRam &data)
{
	m_DefCols[0]  = data.m_DefCols[0];
	m_DefCols[1]  = data.m_DefCols[1];
	m_DefHisMax   = data.m_DefHisMax;
	m_DefFontSize = data.m_DefFontSize;
	m_KanjiMode   = data.m_KanjiMode;
	m_BankGL      = data.m_BankGL;
	m_BankGR      = data.m_BankGR;
	m_DefAtt	  = data.m_DefAtt;
	memcpy(m_DefColTab,  data.m_DefColTab,  sizeof(m_DefColTab));
	memcpy(m_ColTab,  data.m_ColTab,  sizeof(m_ColTab));
	memcpy(m_DefAnsiOpt, data.m_DefAnsiOpt, sizeof(m_DefAnsiOpt));
	memcpy(m_AnsiOpt, data.m_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_DefBankTab, data.m_DefBankTab, sizeof(m_DefBankTab));
	memcpy(m_BankTab, data.m_BankTab, sizeof(m_BankTab));
	m_SendCharSet[0] = data.m_SendCharSet[0];
	m_SendCharSet[1] = data.m_SendCharSet[1];
	m_SendCharSet[2] = data.m_SendCharSet[2];
	m_SendCharSet[3] = data.m_SendCharSet[3];
	m_SendCharSet[4] = data.m_SendCharSet[4];
	m_WheelSize    = data.m_WheelSize;
	m_HisFile      = data.m_HisFile;
	m_DropFileMode = data.m_DropFileMode;
	for ( int n = 0 ; n < 8 ; n++ )
		m_DropFileCmd[n]  = data.m_DropFileCmd[n];
	m_WordStr      = data.m_WordStr;
	memcpy(m_MouseMode, data.m_MouseMode, sizeof(m_MouseMode));
	fc_Init(m_KanjiMode);
	memcpy(m_MetaKeys,  data.m_MetaKeys,  sizeof(m_MetaKeys));
	m_ProcTab = data.m_ProcTab;

	return *this;
}

//////////////////////////////////////////////////////////////////////

void CTextRam::SetKanjiMode(int mode)
{
	m_KanjiMode = mode;
	fc_Init(mode);
}
int CTextRam::Write(LPBYTE lpBuf, int nBufLen, BOOL *sync)
{
	int n;

	if ( m_LineEditMode )
		LOADRAM();

	for ( n = 0 ; n < nBufLen && !m_UpdateFlag ; n++ ) {
		fc_Call(lpBuf[n]);
		if ( m_RetSync ) {
			m_RetSync = FALSE;
			*sync = TRUE;
			break;
		}
	}

	if ( m_UpdateFlag ) {
		m_UpdateFlag = FALSE;
		m_pDocument->SetActCount();
	}

	if ( m_LineEditMode || m_LineEditBuff.GetSize() > 0 ) {
		if ( IsOptEnable(TO_RLDSECHO) ) {
			SAVERAM();
			m_LineEditX = m_CurX;
			m_LineEditY = m_CurY;
			m_LineEditIndex = 0;
			m_LineEditMode = TRUE;
			LineEditEcho();
		} else {
			m_LineEditBuff.Clear();
			m_LineEditMode = FALSE;
		}
	}

	FLUSH();
	return n;
}
void CTextRam::LineEditCwd(int ex, int sy, CStringW &cwd)
{
	int n, x, ch;
	VRAM *vp;
	CBuffer str;

	vp = GETVRAM(0, sy);
	for ( x = 0 ; x < ex ; x += n ) {
		if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].cm) && IS_2BYTE(vp[x + 1].cm) ) {
			//ch = m_IConv.IConvChar(m_FontTab[vp[x].md & CODE_MASK].m_IContName, "UTF-16BE", vp[x].ch);		// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
			//ch = IconvToMsUnicode(ch);
			ch = vp[x].ch;
			if ( (ch & 0xFFFF0000) != 0 )	// 01020304 > 02 01 04 03 (BE->LE 2Word)
				str.PutWord(ch >> 16);
			str.PutWord(ch);
			n = 2;
		} else if ( !IS_ASCII(vp[x].cm) || vp[x].ch == 0 ) {
			ch = ' ';
			str.PutWord(ch);
			n = 1;
		} else {
			//ch = m_IConv.IConvChar(m_FontTab[vp[x].md & CODE_MASK].m_IContName, "UTF-16BE", vp[x].ch | m_FontTab[vp[x].md & CODE_MASK].m_Shift);		// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
			//ch = IconvToMsUnicode(ch);
			ch = vp[x].ch;
			if ( (ch & 0xFFFF0000) != 0 )	// 01020304 > 02 01 04 03 (BE->LE 2Word)
				str.PutWord(ch >> 16);
			str.PutWord(ch);
			n = 1;
		}
	}
	str.PutWord(0);
	cwd = (LPWSTR)str.GetPtr();
}
void CTextRam::LineEditEcho()
{
	CBuffer tmp;
	CBuffer left, right;
	int cx, cy;
	BOOL cf, gf;

	left.Apend(m_LineEditBuff.GetPtr(), m_LineEditPos * sizeof(WCHAR));
	right.Apend(m_LineEditBuff.GetPos(m_LineEditPos * sizeof(WCHAR)), m_LineEditBuff.GetSize() - m_LineEditPos * sizeof(WCHAR));

	m_CurX = m_LineEditX;
	m_CurY = m_LineEditY;

	cf = IsOptEnable(TO_DECAWM) ? TRUE : FALSE;
	gf = IsOptEnable(TO_RLGNDW) ? TRUE : FALSE;

	EnableOption(TO_DECAWM);	// Auto Round Up
	EnableOption(TO_RLGNDW);	// No Do Warp

	tmp.Clear();
	MsToIconvUnicode((WCHAR *)(left.GetPtr()), left.GetSize() / sizeof(WCHAR), m_SendCharSet[m_KanjiMode]);
	m_IConv.IConvBuf("UCS-2LE", m_SendCharSet[m_KanjiMode], &left, &tmp);
	PUTSTR(tmp.GetPtr(), tmp.GetSize());
	cx = m_CurX; cy = m_CurY;

	tmp.Clear();
	MsToIconvUnicode((WCHAR *)(right.GetPtr()), right.GetSize() / sizeof(WCHAR), m_SendCharSet[m_KanjiMode]);
	m_IConv.IConvBuf("UCS-2LE", m_SendCharSet[m_KanjiMode], &right, &tmp);
	PUTSTR(tmp.GetPtr(), tmp.GetSize());
	ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);

	if ( m_LineEditIndex > 0 ) {
		m_LineEditY -= m_LineEditIndex;
		m_LineEditIndex = 0;
	}

	m_CurX = cx; m_CurY = cy;

	if ( !cf ) DisableOption(TO_DECAWM);
	if ( !gf ) DisableOption(TO_RLGNDW);
}
int CTextRam::LineEdit(CBuffer &buf)
{
	int n;
	int rt = FALSE;
	int ec = FALSE;
	int ds = FALSE;
	int sr = FALSE;
	int len;
	CBuffer tmp;
	CString dir;
	WCHAR *wp, *sp;
	CStringW wstr;

	if ( m_LineEditMode == FALSE ) {
		SAVERAM();
		m_LineEditMode = TRUE;
		m_LineEditPos = 0;
		m_LineEditX = m_CurX;
		m_LineEditY = m_CurY;
		m_LineEditHisPos = (-1);
		m_LineEditIndex = 0;
		m_LineEditMapsPos = (-1);
	}

	tmp = buf;
	len = tmp.GetSize() / sizeof(WCHAR);
	wp = (WCHAR *)tmp.GetPtr();
	buf.Clear();

	while ( len-- > 0 ) {
		switch(*wp) {
		case 'V'-'@':
			AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_EDIT_PASTE);
			break;

		case 'C'-'@':
			if ( m_pDocument != NULL && m_pDocument->m_pSock != NULL )
				m_pDocument->m_pSock->SendBreak(0);
			goto CLEARBUF;
		case 'Z'-'@':
			if ( m_pDocument != NULL && m_pDocument->m_pSock != NULL )
				m_pDocument->m_pSock->SendBreak(1);
			// goto CLEARBUF;
		case 'X'-'@':
		CLEARBUF:
			m_LineEditPos = 0;
			m_LineEditBuff.Clear();
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			rt = TRUE;
			break;

		case 'I'-'@':	// TAB
			if ( m_LineEditMapsPos < 0 ) {
				for ( m_LineEditMapsTop = m_LineEditPos ; m_LineEditMapsTop > 0 ; m_LineEditMapsTop-- ) {
					wp = (WCHAR *)m_LineEditBuff.GetPos((m_LineEditMapsTop - 1) * sizeof(WCHAR));
					if ( *wp == ' ' )
						break;
				}

				m_LineEditMapsDir.Empty();
				m_LineEditMapsStr.Empty();
				for ( n = m_LineEditMapsTop ; n < m_LineEditPos ; n++ ) {
					wp = (WCHAR *)m_LineEditBuff.GetPos(n * sizeof(WCHAR));
					m_LineEditMapsStr += *wp;
				}

				if ( m_pDocument != NULL && m_pDocument->m_pSock != NULL && m_pDocument->m_pSock->m_Type == ESCT_PIPE ) {
					if ( m_LineEditMapsTop == 0 ) {
						m_LineEditMapsIndex = 0;
						if ( (m_LineEditMapsInit & 001) == 0 ) {
							m_LineEditMapsInit |= 001;
							((CPipeSock *)(m_pDocument->m_pSock))->GetPathMaps(m_LineEditMapsTab[m_LineEditMapsIndex]);
						}
					} else {
						m_LineEditMapsIndex = 1;
						if ( m_LineEditMapsStr.GetAt(0) == '\\' || m_LineEditMapsStr.Find(':') >= 0 ) {
							ds = TRUE;
							if ( (n = m_LineEditMapsStr.ReverseFind('\\')) >= 0 )
								dir = m_LineEditMapsStr.Left(n);
							else if ( (n = m_LineEditMapsStr.ReverseFind(':')) >= 0 )
								dir = m_LineEditMapsStr.Left(n + 1);
							else
								dir = m_LineEditMapsStr;
						} else {
							LineEditCwd(m_LineEditX - 1, m_LineEditY, wstr);
							dir = wstr;
							if ( (n = m_LineEditMapsStr.ReverseFind('\\')) >= 0 ) {
								SetCurrentDirectory(dir);
								m_LineEditMapsDir = m_LineEditMapsStr.Left(n + 1);
								dir = m_LineEditMapsStr.Left(n);
								m_LineEditMapsStr = m_LineEditMapsStr.Mid(n + 1);
							}
						}
						((CPipeSock *)(m_pDocument->m_pSock))->GetDirMaps(m_LineEditMapsTab[m_LineEditMapsIndex], dir, ds);
					}
				} else
					m_LineEditMapsIndex = 2;

				if ( (m_LineEditMapsPos = m_LineEditMapsTab[m_LineEditMapsIndex].Find(m_LineEditMapsStr)) < 0 )
					break;
				wstr = m_LineEditMapsDir + m_LineEditMapsTab[m_LineEditMapsIndex].GetAt(m_LineEditMapsPos);

			} else if ( (m_LineEditMapsPos = m_LineEditMapsTab[m_LineEditMapsIndex].Next(m_LineEditMapsStr, m_LineEditMapsPos + 1)) >= 0 ) {
				wstr = m_LineEditMapsDir + m_LineEditMapsTab[m_LineEditMapsIndex].GetAt(m_LineEditMapsPos);
			} else {
				wstr = m_LineEditMapsDir + m_LineEditMapsStr;
				m_LineEditMapsPos = (-1);
				m_LineEditMapsIndex = 2;
			}

			if ( m_LineEditMapsIndex < 2 && wstr.Find(' ') >= 0 ) {
				wstr.Insert(0, '"');
				wstr += '"';
			}

			tmp.Clear();
			tmp.Apend(m_LineEditBuff.GetPtr(), m_LineEditMapsTop * sizeof(WCHAR));
			tmp.Apend((LPBYTE)(LPCWSTR)wstr, wstr.GetLength() * sizeof(WCHAR));
			tmp.Apend(m_LineEditBuff.GetPos(m_LineEditPos * sizeof(WCHAR)), m_LineEditBuff.GetSize() - m_LineEditPos * sizeof(WCHAR));
			m_LineEditBuff = tmp;
			m_LineEditPos = m_LineEditMapsTop + wstr.GetLength();
			ec = TRUE;
			break;

		case 'H'-'@':	// BS
			if ( m_LineEditPos <= 0 )
				break;
			m_LineEditPos--;
		case 0x7F:	// DEL
			n = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			if ( m_LineEditPos >= n )
				break;
			sp = (WCHAR *)m_LineEditBuff.GetPos(m_LineEditPos * sizeof(WCHAR));
			memcpy(sp, sp + 1, (n - m_LineEditPos) * sizeof(WCHAR));
			m_LineEditBuff.ConsumeEnd(sizeof(WCHAR));
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			if ( m_LineEditBuff.GetSize() <= 0 )
				rt = TRUE;
			break;

		case 0x1C:	// LEFT
			if ( m_LineEditPos <= 0 )
				break;
			m_LineEditPos--;
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;
		case 0x1D:	// RIGHT
			if ( m_LineEditPos >= (int)(m_LineEditBuff.GetSize() / sizeof(WCHAR)) )
				break;
			m_LineEditPos++;
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;

		case 0x1E:	// UP
			if ( (m_LineEditHisPos + 1) >= m_LineEditHis.GetSize() )
				break;
			if ( ++m_LineEditHisPos <= 0 )
				m_LineEditNow.PutBuf(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
			m_LineEditBuff.Clear();
			m_LineEditBuff.Apend(m_LineEditHis.GetAt(m_LineEditHisPos).GetPtr(), m_LineEditHis.GetAt(m_LineEditHisPos).GetSize());
			m_LineEditPos = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;
		case 0x1F:	// DOWN
			if ( m_LineEditHisPos < 0 )
				break;
			if ( --m_LineEditHisPos < 0 ) {
				m_LineEditBuff.Clear();
				m_LineEditBuff.Apend(m_LineEditNow.GetPtr(), m_LineEditNow.GetSize());
				m_LineEditPos = m_LineEditBuff.GetSize() / sizeof(WCHAR);
				ec = TRUE;
				break;
			}
			m_LineEditBuff.Clear();
			m_LineEditBuff.Apend(m_LineEditHis.GetAt(m_LineEditHisPos).GetPtr(), m_LineEditHis.GetAt(m_LineEditHisPos).GetSize());
			m_LineEditPos = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;

		case 0x0D:
			if ( m_LineEditBuff.GetSize() > 0 ) {
				m_LineEditNow.PutBuf(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
				if ( m_LineEditHisPos >= 0 )
					m_LineEditHis.RemoveAt(m_LineEditHisPos);
				else {
					for ( n = 0 ; n < (int)m_LineEditHis.GetSize() ; n++ ) {
						if ( m_LineEditNow.GetSize() == m_LineEditHis.GetAt(n).GetSize() &&
							memcmp(m_LineEditNow.GetPtr(), m_LineEditHis.GetAt(n).GetPtr(), m_LineEditNow.GetSize()) == 0 ) {
							m_LineEditHis.RemoveAt(n);
							break;
						}
					}
				}
				m_LineEditHis.InsertAt(0, m_LineEditNow);
				if ( (n = (int)m_LineEditHis.GetSize()) > 200 )
					m_LineEditHis.RemoveAt(n - 1);
				m_LineEditMapsTab[2].AddWStrBuf(m_LineEditNow.GetPtr(), m_LineEditNow.GetSize());
			}
			m_LineEditBuff.PutWord(0x0D);
			buf.Apend(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
			m_LineEditBuff.Clear();
			m_LineEditPos = 0;
			m_LineEditHisPos = (-1);
			m_LineEditMapsPos = (-1);
			rt = TRUE;
			break;

		default:
			n = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			if ( m_LineEditPos >= n )
				m_LineEditBuff.PutWord(*wp);
			else {
				m_LineEditBuff.PutWord(0x00);	// Dummy
				sp = (WCHAR *)m_LineEditBuff.GetPtr();
				for ( ; m_LineEditPos <= n ; n-- )
					sp[n] = sp[n - 1];
				sp[m_LineEditPos] = *wp;
			}
			m_LineEditPos++;
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			if ( (*wp & 0xFF00) == 0 && *wp < L' ' )
				sr = TRUE;
			break;
		}
		wp++;
	}

	if ( sr ) {
		buf.Apend(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
		m_LineEditBuff.Clear();
		m_LineEditPos = 0;
		m_LineEditHisPos = (-1);
		m_LineEditMapsPos = (-1);
		rt = TRUE;
		ec = FALSE;
	}

	if ( ec )
		LineEditEcho();

	if ( rt ) {
		LOADRAM();
		m_LineEditMode = FALSE;
	}

	if ( ec || rt )
		FLUSH();

	return rt;
}
int CTextRam::IsWord(WCHAR ch)
{
	if ( ch == 0 )
		return 0;
	else if ( ch >= 0x3040 && ch <= 0x309F )		// 3040 - 309F Ç–ÇÁÇ™Ç»
		return 2;
	else if ( ch >= 0x30A0 && ch <= 0x30FF )		// 30A0 - 30FF ÉJÉ^ÉJÉi
		return 3;
	else if ( UnicodeWidth(UCS2toUCS4(ch)) == 2 && iswalnum(ch) )
		return 4;
	else if ( iswalnum(ch) || m_WordStr.Find(ch) >= 0 )
		return 1;
	return 0;
}
int CTextRam::GetPos(int x, int y)
{
	if ( x < 0 ) {
		if ( --y < (0 - m_HisLen + m_Lines) )
			return 0;
		x += m_Cols;
	} else if ( x >= m_Cols ) {
		if ( ++y >= m_Lines )
			return 0;
		x -= m_Cols;
	}
	return GETVRAM(x, y)->ch;
}
BOOL CTextRam::IncPos(int &x, int &y)
{
	if ( ++x >= m_Cols ) {
		if ( ++y >= m_Lines ) {
			x = m_Cols - 1;
			y = m_Lines - 1;
			return TRUE;
		}
		x = 0;
	}
	return FALSE;
}
BOOL CTextRam::DecPos(int &x, int &y)
{
	if ( --x < 0 ) {
		if ( --y < (0 - m_HisLen + m_Lines) ) {
			x = 0;
			y = (0 - m_HisLen + m_Lines);
			return TRUE;
		}
		x = m_Cols - 1;
	}
	return FALSE;
}
void CTextRam::EditWordPos(int *sps, int *eps)
{
	int n, c;
	int tx, ty;
	int sx, sy;
	int ex, ey;
	CStringW tmp;

	SetCalcPos(*sps, &sx, &sy);

	if ( sx > 0 && IS_2BYTE(GETVRAM(sx, sy)->cm) )
		sx--;

	if ( IS_ENABLE(m_AnsiOpt, TO_RLGAWL) != 0 ) {
		for ( tx = 0, ty = sy ; tx <= sx ; tx++ ) {
			tmp.Empty();
			for ( n = 0 ; n < 6 ; n++ )
				tmp += (WCHAR)GetPos(tx + n, ty);
			if ( wcsncmp(tmp, L"http:", 5) != 0 && wcsncmp(tmp, L"https:", 6) != 0 && wcsncmp(tmp, L"ftp:", 4) != 0 )
				continue;

			ex = tx;
			ey = ty;
			for ( n = 0 ; (c = GetPos(ex + 1, ey)) != 0 ; n++ ) {
				if ( !iswalnum(c) && wcschr(L"!\"#$%&'()=~|-^\\@[;:],./`{+*}<>?_", c) == NULL )
					break;
				IncPos(ex, ey);
			}
			if ( (tx + n) >= sx ) {
				sx = tx;
				sy = ty;
				goto SKIP;
			}
		}
	}

	ex = sx;
	ey = sy;

	switch(IsWord(GetPos(sx, sy))) {
	case 0:
	case 1:	// is alnum
		while ( IsWord(GetPos(sx - 1, sy)) == 1 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 1 )
			IncPos(ex, ey);
		break;
	case 2:	// Ç–ÇÁÇ™Ç»
		while ( IsWord(GetPos(sx - 1, sy)) == 2 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(sx - 1, sy)) == 4 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 2 )
			IncPos(ex, ey);
		break;
	case 3:	// ÉJÉ^ÉJÉi
		while ( IsWord(GetPos(sx - 1, sy)) == 3 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 3 )
			IncPos(ex, ey);
		break;
	case 4:	// ÇªÇÃëº
		while ( IsWord(GetPos(sx - 1, sy)) == 4 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 4 )
			IncPos(ex, ey);
		while ( IsWord(GetPos(ex + 1, ey)) == 2 )
			IncPos(ex, ey);
		break;
	}

SKIP:
	if ( IS_2BYTE(GETVRAM(sx, sy)->cm) )
		DecPos(sx, sy);

	if ( IS_1BYTE(GETVRAM(ex, ey)->cm) )
		IncPos(ex, ey);

	*sps = GetCalcPos(sx, sy);
	*eps = GetCalcPos(ex, ey);
}
void CTextRam::EditCopy(int sps, int eps, BOOL rectflag, BOOL lineflag)
{
	int n, x, y, ch, sx, ex, tc;
	int x1, y1, x2, y2;
	HGLOBAL hClipData;
	WCHAR *pData;
	VRAM *vp;
	CBuffer tmp, str;
	LPWSTR p;

	SetCalcPos(sps, &x1, &y1);
	SetCalcPos(eps, &x2, &y2);

	if ( rectflag && x1 > x2 ) {
		x = x1; x1 = x2; x2 = x;
	}

	for ( y = y1 ; y <= y2 ; y++ ) {
		vp = GETVRAM(0, y);
		
		if ( rectflag ) {
			sx = x1;
			ex = x2;
		} else if ( lineflag ) {
			sx = 0;
			ex = m_Cols - 1;
		} else {
			sx = (y == y1 ? x1 : 0);
			ex = (y == y2 ? x2 : (m_Cols - 1));
		}

		if ( vp->dm != 0 ) {
			sx /= 2;
			ex /= 2;
		}

		while ( sx <= ex && vp[ex].ch < ' ' )
			ex--;

		if ( IS_2BYTE(vp[sx].cm) )
			sx++;

		tc = 0;
		str.Clear();
		for ( x = sx ; x <= ex ; x += n ) {
			if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].cm) && IS_2BYTE(vp[x + 1].cm) ) {
				//ch = m_IConv.IConvChar(m_FontTab[vp[x].md & CODE_MASK].m_IContName, "UTF-16BE", vp[x].ch);		// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
				//ch = IconvToMsUnicode(ch);
				ch = vp[x].ch;
				if ( (ch & 0xFFFF0000) != 0 )	// 01020304 > 02 01 04 03 (BE->LE 2Word)
					str.PutWord(ch >> 16);
				str.PutWord(ch);
				n = 2;
			} else if ( !IS_ASCII(vp[x].cm) || vp[x].ch == 0 ) {
				ch = ' ';
				str.PutWord(ch);
				n = 1;
			} else {
				//ch = m_IConv.IConvChar(m_FontTab[vp[x].md & CODE_MASK].m_IContName, "UTF-16BE", vp[x].ch | m_FontTab[vp[x].md & CODE_MASK].m_Shift);		// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
				//ch = IconvToMsUnicode(ch);
				ch = vp[x].ch;
				if ( (ch & 0xFFFF0000) != 0 )	// 01020304 > 02 01 04 03 (BE->LE 2Word)
					str.PutWord(ch >> 16);
				str.PutWord(ch);
				n = 1;
			}

			if ( n == 1 && ch == ' ' && IS_ENABLE(m_AnsiOpt, TO_RLSPCTAB) != 0 ) {
				tc++;
				if ( (x % m_DefTab) == (m_DefTab - 1) ) {
//					if ( tc > (m_DefTab / 2) ) {
					if ( tc > 1 ) {
						str.ConsumeEnd(tc * 2);
						str.PutWord('\t');
					}
					tc = 0;
				}
			} else
				tc = 0;
		}

		if ( (y < y2 && x < m_Cols) || (y == y2 && lineflag) ) {
			str.PutWord('\r');
			str.PutWord('\n');
		}

		tmp.Apend(str.GetPtr(), str.GetSize());
	}

	tmp.PutWord(0);
	p = (LPWSTR)(tmp.GetPtr());

	if ( IS_ENABLE(m_AnsiOpt, TO_RLGAWL) != 0 && tmp.GetSize() > 10 && p[0] == 'h' && p[1] == 't' && p[2] == 't' && p[3] == 'p' && p[4] == ':' )
		ShellExecuteW(AfxGetMainWnd()->GetSafeHwnd(), NULL, (LPCWSTR)(tmp.GetPtr()), NULL, NULL, SW_NORMAL);

	if ( (hClipData = GlobalAlloc(GMEM_MOVEABLE, tmp.GetSize())) == NULL )
		return;

	if ( (pData = (WCHAR *)GlobalLock(hClipData)) == NULL )
		goto ENDOF;

	memcpy(pData, tmp.GetPtr(), tmp.GetSize());
	GlobalUnlock(hClipData);

	if ( !AfxGetMainWnd()->OpenClipboard() )
		goto ENDOF;

	if ( !EmptyClipboard() ) {
		CloseClipboard();
		goto ENDOF;
	}

	SetClipboardData(CF_UNICODETEXT, hClipData);
	CloseClipboard();
	goto ENDOF2;

ENDOF:
	GlobalFree(hClipData);
ENDOF2:
	return;
}

void CTextRam::StrOut(CDC* pDC, LPCRECT pRect, struct DrawWork &prop, int len, char *str, int sln, int *spc, class CRLoginView *pView)
{
	int n, x, y;
	int at = prop.att;
	int md = ETO_CLIPPED;
	int st = 0;
	int wd = (prop.dmf == 0 ? 1 : 2);
	int hd = (prop.dmf <= 1 ? 1 : 2);
	int rv = FALSE;
	COLORREF fc, bc, tc;
	CFontChacheNode *pFontCache;
	CFont *pFont;

	if ( (at & ATT_BOLD) != 0 && !IS_ENABLE(m_AnsiOpt, TO_RLBOLD) && prop.fcn < 16 )
		prop.fcn ^= 0x08;

	if ( (at & ATT_ITALIC) != 0 )
		st |= 2;

	fc = m_ColTab[prop.fcn];
	bc = m_ColTab[prop.bcn];

	if ( (at & ATT_REVS) != 0 ) {
		tc = fc;
		fc = bc;
		bc = tc;
		rv = TRUE;
	}

	if ( (at & (ATT_SBLINK | ATT_BLINK)) != 0 ) {
		if ( (pView->m_BlinkFlag & 4) == 0 ) {
			pView->SetTimer(1026, 250, NULL);
			pView->m_BlinkFlag = 4;
		} else if ( ((at & ATT_BLINK) != 0 && (pView->m_BlinkFlag & 1) != 0) ||
				    ((at & ATT_BLINK) == 0 && (pView->m_BlinkFlag & 2) != 0) ) {
			tc = fc;
			fc = bc;
			bc = tc;
			rv = TRUE;
		}
	}

	if ( (at & ATT_SECRET) != 0 )
		fc = bc;

	if ( (at & ATT_CLIP) != 0 || (pView->m_ActiveFlag && (at & ATT_MARK) != 0) ) {
		tc = fc;
		fc = bc;
		bc = tc;
		if ( pView->m_pBitmap != NULL )
			pDC->InvertRect(pRect);
		if ( rv )
			bc = RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2);
	}


	if ( prop.mod < 0 ) {
		if ( pView->m_pBitmap == NULL )
			pDC->FillSolidRect(pRect, bc);
	} else {
		if ( pView->m_pBitmap == NULL || rv )
			md = ETO_OPAQUE | ETO_CLIPPED;

		if ( (at & ATT_HALF) != 0 )
			fc = RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2);

		if ( (pFontCache = m_FontTab[prop.mod].GetFont(pView->m_CharWidth * wd, pView->m_CharHeight * hd, st, prop.fnm)) != NULL && prop.csz > 1 && pFontCache->m_KanWidMul > 100 )
			pFontCache = m_FontTab[prop.mod].GetFont(pView->m_CharWidth * wd * pFontCache->m_KanWidMul / 100, pView->m_CharHeight * hd, st, prop.fnm);

		pFont = (pFontCache != NULL ? pFontCache->m_pFont : CFont::FromHandle((HFONT)GetStockObject(SYSTEM_FONT)));
		pDC->SelectObject(pFont);

		x = pRect->left;
		y = pRect->top - pView->m_CharHeight * m_FontTab[prop.mod].m_Offset / 100 - (prop.dmf == 3 ? pView->m_CharHeight : 0);

		pDC->SetTextColor(fc);
		pDC->SetBkColor(bc);

		::ExtTextOutW(pDC->m_hDC, x, y, md, pRect, (LPCWSTR)str, len / 2, spc);

		if ( (at & ATT_BOLD) != 0 && (IS_ENABLE(m_AnsiOpt, TO_RLBOLD) || prop.fcn >= 16) ) {
			int oldMode = pDC->SetBkMode(TRANSPARENT);
			::ExtTextOutW(pDC->m_hDC, x + 1, y, ETO_CLIPPED, pRect, (LPCWSTR)str, len / 2, spc);
			pDC->SetBkMode(oldMode);
		}
	}

	if ( (at & (ATT_OVER | ATT_LINE | ATT_UNDER | ATT_DUNDER | ATT_STRESS)) != 0 ) {
		CPen cPen(PS_SOLID, 1, fc);
		CPen *oPen = pDC->SelectObject(&cPen);
		POINT point[2];
		point[0].x = pRect->left;
		point[1].x = pRect->right;

		if ( (at & ATT_OVER) != 0 ) {
			point[0].y = pRect->top;
			point[1].y = pRect->top;
			pDC->Polyline(point, 2);
		}

		if ( (at & ATT_LINE) != 0 ) {
			point[0].y = (pRect->top + pRect->bottom) / 2;
			point[1].y = (pRect->top + pRect->bottom) / 2;
			pDC->Polyline(point, 2);
		} else if ( (at & ATT_STRESS) != 0 ) {
			point[0].y = (pRect->top + pRect->bottom) / 2 - 1;
			point[1].y = (pRect->top + pRect->bottom) / 2 - 1;
			pDC->Polyline(point, 2);
			point[0].y = (pRect->top + pRect->bottom) / 2 + 1;
			point[1].y = (pRect->top + pRect->bottom) / 2 + 1;
			pDC->Polyline(point, 2);
		}

		if ( (at & ATT_UNDER) != 0 ) {
			point[0].y = pRect->bottom - 2;
			point[1].y = pRect->bottom - 2;
			pDC->Polyline(point, 2);
		} else if ( (at & ATT_DUNDER) != 0 ) {
			point[0].y = pRect->bottom - 1;
			point[1].y = pRect->bottom - 1;
			pDC->Polyline(point, 2);
			point[0].y = pRect->bottom - 3;
			point[1].y = pRect->bottom - 3;
			pDC->Polyline(point, 2);
		}

		pDC->SelectObject(oPen);
	}

	if ( (at & (ATT_FRAME | ATT_CIRCLE | ATT_RSLINE | ATT_RDLINE | ATT_LSLINE | ATT_LDLINE)) != 0 ) {
		CPen cPen(PS_SOLID, 1, fc);
		CPen *oPen = pDC->SelectObject(&cPen);
		POINT point[9];

		y = pRect->left;
		for ( x = 0 ; x < sln ; x++ ) {
			if ( (at & ATT_FRAME) != 0 ) {
				point[0].x = y; point[0].y = pRect->top + 1;
				point[1].x = y + spc[x] - 2; point[1].y = pRect->top + 1;
				point[2].x = y + spc[x] - 2; point[2].y = pRect->bottom - 1;
				point[3].x = y; point[3].y = pRect->bottom - 1;
				point[4].x = y; point[4].y = pRect->top + 1;
				pDC->Polyline(point, 5);
			} else if ( (at & ATT_CIRCLE) != 0 ) {
				n = spc[x] / 3;
				point[0].x = y + n; point[0].y = pRect->top + 1;
				point[1].x = y + spc[x] - 2 - n; point[1].y = pRect->top + 1;
				point[2].x = y + spc[x] - 2; point[2].y = pRect->top + 1 + n;
				point[3].x = y + spc[x] - 2; point[3].y = pRect->bottom - 1 - n;
				point[4].x = y + spc[x] - 2 - n; point[4].y = pRect->bottom - 1;
				point[5].x = y + n; point[5].y = pRect->bottom - 1;
				point[6].x = y; point[6].y = pRect->bottom - 1 - n;
				point[7].x = y; point[7].y = pRect->top + 1 + n;
				point[8].x = y + n; point[8].y = pRect->top + 1;
				pDC->Polyline(point, 9);
			}

			if ( (at & ATT_RSLINE) != 0 ) {
				point[0].x = y + spc[x] - 1; point[0].y = pRect->top;
				point[1].x = y + spc[x] - 1; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
			} else if ( (at & ATT_RDLINE) != 0 ) {
				point[0].x = y + spc[x] - 1; point[0].y = pRect->top;
				point[1].x = y + spc[x] - 1; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
				point[0].x = y + spc[x] - 3; point[0].y = pRect->top;
				point[1].x = y + spc[x] - 3; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
			}

			if ( (at & ATT_LSLINE) != 0 ) {
				point[0].x = y; point[0].y = pRect->top;
				point[1].x = y; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
			} else if ( (at & ATT_LDLINE) != 0 ) {
				point[0].x = y; point[0].y = pRect->top;
				point[1].x = y; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
				point[0].x = y + 2; point[0].y = pRect->top;
				point[1].x = y + 2; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
			}

			y += spc[x];
		}

		pDC->SelectObject(oPen);
	}

	if ( IS_ENABLE(m_AnsiOpt, TO_DECSCNM) )
		pDC->InvertRect(pRect);

	if ( pView->m_VisualBellFlag )
		pDC->InvertRect(pRect);
}
void CTextRam::DrawVram(CDC *pDC, int x1, int y1, int x2, int y2, class CRLoginView *pView)
{
	int ch, x, y;
	struct DrawWork work, prop;
	int pos, spos, epos;
	int csx, cex, csy, cey;
	VRAM *vp, *tp;
	int len, sln;
	char tmp[COLS_MAX * 8];
	int spc[COLS_MAX];
	CFont *pFontOld = pDC->SelectObject(CFont::FromHandle((HFONT)GetStockObject(SYSTEM_FONT)));
	RECT rect;

	if ( pView->m_ClipFlag ) {
		if ( pView->m_ClipStaPos <= pView->m_ClipEndPos ) {
			spos = pView->m_ClipStaPos;
			epos = pView->m_ClipEndPos;
		} else {
			spos = pView->m_ClipEndPos;
			epos = pView->m_ClipStaPos;
		}
		if ( pView->IsClipRectMode() ) {
			SetCalcPos(spos, &csx, &csy);
			SetCalcPos(epos, &cex, &cey);
			if ( csx > cex ) {
				x = csx; csx = cex; cex = x;
				spos = GetCalcPos(csx, csy);
				epos = GetCalcPos(cex, cey);
			}
		} else if ( pView->IsClipLineMode() ) {
			SetCalcPos(spos, &csx, &csy);
			spos = GetCalcPos(0, csy);
			SetCalcPos(epos, &cex, &cey);
			epos = GetCalcPos(m_Cols - 1, cey);
		}
	}

	for ( y = y1 ; y < y2 ; y++ ) {
		len = sln = 0;
		memset(&prop, 0, sizeof(prop));
		rect.top    = pView->CalcGrapY(y);
		rect.bottom = pView->CalcGrapY(y + 1);
		tp = GETVRAM(0, y - pView->m_HisOfs + pView->m_HisMin);
		work.dmf = tp->dm;

		for ( x = (work.dmf != 0 ? (x1 / 2) : x1) ; x < x2 ; ) {
			if ( work.dmf != 0 && x >= (m_Cols / 2) )
				break;

			if ( x >= 0 && x < m_Cols ) {
				vp = tp + x;
				if ( x > 0 && IS_2BYTE(vp[0].cm) && IS_1BYTE(vp[-1].cm) ) {
					x--;
					vp--;
				}
				work.att = vp->at;
				work.fnm = vp->ft;
				work.fcn = vp->fc;
				work.bcn = vp->bc;
				work.mod = vp->md & CODE_MASK;
				work.csz = 1;
				ch = vp->ch;
				if ( x < (m_Cols - 1) && IS_1BYTE(vp[0].cm) && IS_2BYTE(vp[1].cm) ) {
					//ch = m_IConv.IConvChar(m_FontTab[cm].m_IContName, "UTF-16BE", ch);			// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
					//ch = IconvToMsUnicode(ch);
					work.csz = 2;
				} else if ( !IS_ASCII(vp->cm) ) {
					ch = 0;
				//} else {
				//	ch |= m_FontTab[cm].m_Shift;
				//	ch = m_IConv.IConvChar(m_FontTab[cm].m_IContName, "UTF-16BE", ch);			// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
				//	ch = IconvToMsUnicode(ch);
				}
				if ( ch == 0 )
					work.mod = (-1);
			} else {
				work.att = m_DefAtt.at;
				work.fnm = m_DefAtt.ft;
				work.fcn = m_DefAtt.fc;
				work.bcn = m_DefAtt.bc;
				work.mod = (-1);
				work.csz = 1;
				ch = 0;
			}

			if ( pView->m_ClipFlag ) {
				pos = GetCalcPos((work.dmf != 0 ? (x * 2) : x), y - pView->m_HisOfs + pView->m_HisMin);
				if ( spos <= pos && pos <= epos ) {
					if ( pView->IsClipRectMode() ) {
						if ( work.dmf != 0 ) {
							if ( (x * 2) >= csx && (x * 2) <= cex )
								work.att |= ATT_CLIP;
						} else if ( x >= csx && x <= cex )
							work.att |= ATT_CLIP;
					} else
						work.att |= ATT_CLIP;
				}
			}

			if ( memcmp(&prop, &work, sizeof(work)) != 0 ) {
				if ( len > 0 ) {
					rect.right = pView->CalcGrapX(x) * (work.dmf ? 2 : 1);
					StrOut(pDC, &rect, prop, len, tmp, sln, spc, pView);
				}
				memcpy(&prop, &work, sizeof(work));
				len = sln = 0;
				rect.left = pView->CalcGrapX(x) * (work.dmf ? 2 : 1);
			}

			if ( work.mod != (-1) ) {
				if ( (ch & 0xFFFF0000) != 0 ) {				// 01020304 > 02 01 04 03 (BE->LE 2Word)
					tmp[len++] = (char)(ch >> 16);
					tmp[len++] = (char)(ch >> 24);
				}
				tmp[len++] = (char)(ch);
				tmp[len++] = (char)(ch >> 8);
			} else
				len += work.csz;

			spc[sln++] = (pView->CalcGrapX(x + work.csz) - pView->CalcGrapX(x)) * (work.dmf ? 2 : 1);
			x += work.csz;
		}
		if ( len > 0 ) {
			rect.right = pView->CalcGrapX(x) * (work.dmf ? 2 : 1);
			StrOut(pDC, &rect, prop, len, tmp, sln, spc, pView);
		}
	}

	pDC->SelectObject(pFontOld);
}

CWnd *CTextRam::GetAciveView()
{
	return m_pDocument->GetAciveView();
}
void CTextRam::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	CWnd *pView = GetAciveView();
	if ( pView != NULL )
		pView->PostMessage(message, wParam, lParam);
}
BOOL CTextRam::IsOptEnable(int opt)
{
	return IS_ENABLE(m_AnsiOpt, opt);
}
void CTextRam::EnableOption(int opt)
{
	m_AnsiOpt[opt / 32] |= (1 << (opt % 32));
}
void CTextRam::DisableOption(int opt)
{
	m_AnsiOpt[opt / 32] &= ~(1 << (opt % 32));
}
void CTextRam::ReversOption(int opt)
{
	m_AnsiOpt[opt / 32] ^= (1 << (opt % 32));
}
int CTextRam::IsOptValue(int opt, int len)
{
	int n;
	int v = 0;
	int b = 1;
	for ( n = 0 ; n < len ; n++ ) {
		if ( IsOptEnable(opt) )
			v |= b;
		opt++;
		b <<= 1;
	}
	return v;
}
void CTextRam::SetOptValue(int opt, int len, int value)
{
	int n;
	for ( n = 0 ; n < len ; n++ ) {
		if ( (value & 1) != 0 )
			EnableOption(opt);
		else
			DisableOption(opt);
		opt++;
		value >>= 1;
	}
}

void CTextRam::OnClose()
{
	int x, y;
	int my = m_Lines;
	VRAM *vp;

	if ( m_VRam == NULL )
		return;

	SaveHistory();

	if ( m_pDocument == NULL || m_pDocument->m_pLogFile == NULL || IsOptValue(TO_RLLOGMODE, 2) != LOGMOD_LINE )
		return;

	while ( my > 0 ) {
		vp = GETVRAM(0, my - 1);
		for ( x = 0 ; x < m_Cols ; x++ ) {
			if ( vp[x].ch != 0 )
				break;
		}
		if ( x < m_Cols )
			break;
		my--;
	}

	for ( y = 0 ; y < my ; y++ )
		CallReciveLine(y);
}
void CTextRam::CallReciveLine(int y)
{
	int n, i;
	CStringW tmp;

	if ( m_pDocument == NULL || m_pDocument->m_pLogFile == NULL || IsOptValue(TO_RLLOGMODE, 2) != LOGMOD_LINE )
		return;

	LineEditCwd(m_Cols, y, tmp);
	n = tmp.GetLength();
	for ( i = 0 ; i < n && tmp[n - i - 1] == L' ' ; )
		i++;
	if ( i > 0 ) {
		tmp.Delete(n - i, i);
		tmp += L"\r\n";
	}

	CBuffer in, out;
	in.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
	m_IConv.IConvBuf("UCS-2LE", m_SendCharSet[m_LogCharSet[IsOptValue(TO_RLLOGCODE, 2)]], &in, &out);
	m_pDocument->m_pLogFile->Write(out.GetPtr(), out.GetSize());
}
void CTextRam::CallReciveChar(int ch)
{
	static const WCHAR *CtrlName[] = {
		L"NUL",	L"SOH",	L"STX",	L"ETX",	L"EOT",	L"ACK",	L"ENQ",	L"BEL",
		L"BS",	L"HT",	L"LF",	L"VT",	L"FF",	L"CR",	L"SO",	L"SI",
		L"DLE",	L"DC1",	L"DC2",	L"DC3",	L"DC4",	L"NAK",	L"SYN",	L"ETB",
		L"CAN",	L"EM",	L"SUB",	L"ESC",	L"FS",	L"GS",	L"RS",	L"US",
	};

	if ( m_pDocument == NULL )
		return;

	m_pDocument->OnReciveChar(ch);

	if ( m_pDocument->m_pLogFile == NULL )
		return;

	int md = IsOptValue(TO_RLLOGMODE, 2);

#if 0
	BOOL ct = ((ch & 0xFFFFFF00) != 0 || ch >= 0x20 || ch == 0x09 ? FALSE : TRUE);
	if ( m_LogStat == FALSE ) {
		if ( ct == FALSE )
			m_LogStat = TRUE;
		return;
	} else {
		if ( ct == FALSE )
			return;
		m_LogStat = FALSE;
	}
#endif

	if ( md == LOGMOD_RAW || md == LOGMOD_LINE )
		return;

	CStringW tmp;
	if ( ch >= 0 && ch < 0x20 && ch != 0x09 && ch != 0x0A && ch != 0x0D ) {
		if ( md == LOGMOD_CHAR && ch != 0x08 )
			return;
		tmp.Format(L"<%s>", CtrlName[ch]);
	} else {
		if ( (ch & 0xFFFF0000) != 0 )
			tmp = (WCHAR)(ch >> 16);
		tmp += (WCHAR)(ch & 0xFFFF);
	}

	CBuffer in, out;
	in.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
	m_IConv.IConvBuf("UCS-2LE", m_SendCharSet[m_LogCharSet[IsOptValue(TO_RLLOGCODE, 2)]], &in, &out);
	m_pDocument->m_pLogFile->Write(out.GetPtr(), out.GetSize());
}

int CTextRam::UnicodeWidth(DWORD code)
{
    int n, b, m;
#define UNIWIDTABSIZE   379
	static const DWORD UnicodeWidthTab[] = {
		0x0000A1, 0x0000A2, 0x0000A4, 0x0000A5, 0x0000A7, 0x0000A9, 0x0000AA, 0x0000AB,
		0x0000AD, 0x0000AF, 0x0000B0, 0x0000B5, 0x0000B6, 0x0000BB, 0x0000BC, 0x0000C0,
		0x0000C6, 0x0000C7, 0x0000D0, 0x0000D1, 0x0000D7, 0x0000D9, 0x0000DE, 0x0000E2,
		0x0000E6, 0x0000E7, 0x0000E8, 0x0000EB, 0x0000EC, 0x0000EE, 0x0000F0, 0x0000F1,
		0x0000F2, 0x0000F4, 0x0000F7, 0x0000FB, 0x0000FC, 0x0000FD, 0x0000FE, 0x0000FF,
		0x000101, 0x000102, 0x000111, 0x000112, 0x000113, 0x000114, 0x00011B, 0x00011C,
		0x000126, 0x000128, 0x00012B, 0x00012C, 0x000131, 0x000134, 0x000138, 0x000139,
		0x00013F, 0x000143, 0x000144, 0x000145, 0x000148, 0x00014C, 0x00014D, 0x00014E,
		0x000152, 0x000154, 0x000166, 0x000168, 0x00016B, 0x00016C, 0x0001CE, 0x0001CF,
		0x0001D0, 0x0001D1, 0x0001D2, 0x0001D3, 0x0001D4, 0x0001D5, 0x0001D6, 0x0001D7,
		0x0001D8, 0x0001D9, 0x0001DA, 0x0001DB, 0x0001DC, 0x0001DD, 0x000251, 0x000252,
		0x000261, 0x000262, 0x0002C4, 0x0002C5, 0x0002C7, 0x0002C8, 0x0002C9, 0x0002CC,
		0x0002CD, 0x0002CE, 0x0002D0, 0x0002D1, 0x0002D8, 0x0002DC, 0x0002DD, 0x0002DE,
		0x0002DF, 0x0002E0, 0x000300, 0x000370, 0x000391, 0x0003A2, 0x0003A3, 0x0003AA,
		0x0003B1, 0x0003C2, 0x0003C3, 0x0003CA, 0x000401, 0x000402, 0x000410, 0x000450,
		0x000451, 0x000452, 0x001100, 0x00115A, 0x00115F, 0x001160, 0x002010, 0x002011,
		0x002013, 0x002017, 0x002018, 0x00201A, 0x00201C, 0x00201E, 0x002020, 0x002023,
		0x002024, 0x002028, 0x002030, 0x002031, 0x002032, 0x002034, 0x002035, 0x002036,
		0x00203B, 0x00203C, 0x00203E, 0x00203F, 0x002074, 0x002075, 0x00207F, 0x002080,
		0x002081, 0x002085, 0x0020AC, 0x0020AD, 0x002103, 0x002104, 0x002105, 0x002106,
		0x002109, 0x00210A, 0x002113, 0x002114, 0x002116, 0x002117, 0x002121, 0x002123,
		0x002126, 0x002127, 0x00212B, 0x00212C, 0x002153, 0x002155, 0x00215B, 0x00215F,
		0x002160, 0x00216C, 0x002170, 0x00217A, 0x002190, 0x00219A, 0x0021B8, 0x0021BA,
		0x0021D2, 0x0021D3, 0x0021D4, 0x0021D5, 0x0021E7, 0x0021E8, 0x002200, 0x002201,
		0x002202, 0x002204, 0x002207, 0x002209, 0x00220B, 0x00220C, 0x00220F, 0x002210,
		0x002211, 0x002212, 0x002215, 0x002216, 0x00221A, 0x00221B, 0x00221D, 0x002221,
		0x002223, 0x002224, 0x002225, 0x002226, 0x002227, 0x00222D, 0x00222E, 0x00222F,
		0x002234, 0x002238, 0x00223C, 0x00223E, 0x002248, 0x002249, 0x00224C, 0x00224D,
		0x002252, 0x002253, 0x002260, 0x002262, 0x002264, 0x002268, 0x00226A, 0x00226C,
		0x00226E, 0x002270, 0x002282, 0x002284, 0x002286, 0x002288, 0x002295, 0x002296,
		0x002299, 0x00229A, 0x0022A5, 0x0022A6, 0x0022BF, 0x0022C0, 0x002312, 0x002313,
		0x002329, 0x00232B, 0x002460, 0x0024EA, 0x0024EB, 0x00254C, 0x002550, 0x002574,
		0x002580, 0x002590, 0x002592, 0x002596, 0x0025A0, 0x0025A2, 0x0025A3, 0x0025AA,
		0x0025B2, 0x0025B4, 0x0025B6, 0x0025B8, 0x0025BC, 0x0025BE, 0x0025C0, 0x0025C2,
		0x0025C6, 0x0025C9, 0x0025CB, 0x0025CC, 0x0025CE, 0x0025D2, 0x0025E2, 0x0025E6,
		0x0025EF, 0x0025F0, 0x002605, 0x002607, 0x002609, 0x00260A, 0x00260E, 0x002610,
		0x002614, 0x002616, 0x00261C, 0x00261D, 0x00261E, 0x00261F, 0x002640, 0x002641,
		0x002642, 0x002643, 0x002660, 0x002662, 0x002663, 0x002666, 0x002667, 0x00266B,
		0x00266C, 0x00266E, 0x00266F, 0x002670, 0x00273D, 0x00273E, 0x002776, 0x002780,
		0x002E80, 0x002E9A, 0x002E9B, 0x002EF4, 0x002F00, 0x002FD6, 0x002FF0, 0x002FFC,
		0x003000, 0x00303F, 0x003041, 0x003097, 0x003099, 0x003100, 0x003105, 0x00312D,
		0x003131, 0x00318F, 0x003190, 0x0031B8, 0x0031C0, 0x0031D0, 0x0031F0, 0x00321F,
		0x003220, 0x003244, 0x003250, 0x0032FF, 0x003300, 0x004DB6, 0x004E00, 0x009FBC,
		0x00A000, 0x00A48D, 0x00A490, 0x00A4C7, 0x00AC00, 0x00D7A4, 0x00E000, 0x00FA2E,
		0x00FA30, 0x00FA6B, 0x00FA70, 0x00FADA, 0x00FE00, 0x00FE1A, 0x00FE30, 0x00FE53,
		0x00FE54, 0x00FE67, 0x00FE68, 0x00FE6C, 0x00FF01, 0x00FF61, 0x00FFE0, 0x00FFE7,
		0x00FFFD, 0x00FFFE, 0x020000, 0x02FFFE, 0x030000, 0x03FFFE, 0x0E0100, 0x0E01F0,
		0x0F0000, 0x0FFFFE, 0x100000 };
		
#define UNIWIDTABSIZEA   66
	static const DWORD UnicodeWidthTabA[] = {
		0x001100, 0x00115A, 0x00115F, 0x001160, 0x002329, 0x00232B, 0x002E80, 0x002E9A,
		0x002E9B, 0x002EF4, 0x002F00, 0x002FD6, 0x002FF0, 0x002FFC, 0x003000, 0x00303F,
		0x003041, 0x003097, 0x003099, 0x003100, 0x003105, 0x00312D, 0x003131, 0x00318F,
		0x003190, 0x0031B8, 0x0031C0, 0x0031D0, 0x0031F0, 0x00321F, 0x003220, 0x003244,
		0x003250, 0x0032FF, 0x003300, 0x004DB6, 0x004E00, 0x009FBC, 0x00A000, 0x00A48D,
		0x00A490, 0x00A4C7, 0x00AC00, 0x00D7A4, 0x00F900, 0x00FA2E, 0x00FA30, 0x00FA6B,
		0x00FA70, 0x00FADA, 0x00FE10, 0x00FE1A, 0x00FE30, 0x00FE53, 0x00FE54, 0x00FE67,
		0x00FE68, 0x00FE6C, 0x00FF01, 0x00FF61, 0x00FFE0, 0x00FFE7, 0x020000, 0x02FFFE,
		0x030000, 0x03FFFE };


    b = 0;
	if ( IsOptEnable(TO_RLUNIAWH) ) {
		m = UNIWIDTABSIZEA - 1;
	    while ( b <= m ) {
			n = (b + m) / 2;
			if ( code >= UnicodeWidthTabA[n] )
			    b = n + 1;
			else
			    m = n - 1;
		}
	} else {
		m = UNIWIDTABSIZE - 1;
	    while ( b <= m ) {
			n = (b + m) / 2;
			if ( code >= UnicodeWidthTab[n] )
			    b = n + 1;
			else
			    m = n - 1;
		}
	}
    return ((b & 1) ? 2 : 1);
}


//////////////////////////////////////////////////////////////////////
// Static Lib
//////////////////////////////////////////////////////////////////////

void CTextRam::MsToIconvUnicode(WCHAR *str, int len, LPCSTR charset)
{
	if ( strcmp(charset, "EUC-JISX0213") == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
			case 0xFFE0: *str = 0x00A2; break;		/* Åë 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* Åí 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* Å  0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	} else if ( strcmp(charset, "SHIFT_JISX0213") == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
//			case 0x005C: *str = 0x00A5; break;		/* \  0x5C          U+00A5 U+005C */
//			case 0x007E: *str = 0x203E; break;		/* ~  0x7E          U+203E U+007E */
			case 0xFFE0: *str = 0x00A2; break;		/* Åë 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* Åí 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* Å  0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	} else if ( strcmp(charset, "EUC-JP") == 0 || strcmp(charset, "EUCJP") == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
			case 0x2225: *str = 0x2016; break;		/* Åa 0x8161(01-34) U+2016 U+2225 */
			case 0xFF0D: *str = 0x2212; break;		/* Å| 0x817C(01-61) U+2212 U+FF0D */
			case 0xFFE0: *str = 0x00A2; break;		/* Åë 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* Åí 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* Å  0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	} else if ( strcmp(charset, "SHIFT_JIS") == 0 || strcmp(charset, "MS_KANJI") == 0 ||
			    strcmp(charset, "SHIFT-JIS") == 0 || strcmp(charset, "SJIS") == 0 || strcmp(charset, "CSSHIFTJIS") == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
//			case 0x005C: *str = 0x00A5; break;		/* \  0x5C          U+00A5 U+005C */
//			case 0x007E: *str = 0x203E; break;		/* ~  0x7E          U+203E U+007E */
			case 0x2225: *str = 0x2016; break;		/* Åa 0x8161(01-34) U+2016 U+2225 */
			case 0xFF0D: *str = 0x2212; break;		/* Å| 0x817C(01-61) U+2212 U+FF0D */
			case 0xFF5E: *str = 0x301C; break;		/* Å` 0x8160(01-33) U+301C U+FF5E */
			case 0xFFE0: *str = 0x00A2; break;		/* Åë 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* Åí 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* Å  0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	}
}

DWORD CTextRam::IconvToMsUnicode(DWORD code)
{
	switch(code) {							/*                  iconv  MS     */
	case 0x00A2: code = 0xFFE0; break;		/* Åë 0x8191(01-81) U+00A2 U+FFE0 */
	case 0x00A3: code = 0xFFE1; break;		/* Åí 0x8192(01-82) U+00A3 U+FFE1 */
	case 0x00A5: code = 0x005C; break;		/* \  0x5C          U+00A5 U+005C */
	case 0x00AC: code = 0xFFE2; break;		/* Å  0x81CA(02-44) U+00AC U+FFE2 */
	case 0x2014: code = 0x2015; break;		/* Å\ 0x815C(01-29) U+2014 U+2015 */
	case 0x2016: code = 0x2225; break;		/* Åa 0x8161(01-34) U+2016 U+2225 */
	case 0x203E: code = 0x007E; break;		/* ~  0x7E          U+203E U+007E */
	case 0x2212: code = 0xFF0D; break;		/* Å| 0x817C(01-61) U+2212 U+FF0D */
	case 0x301C: code = 0xFF5E; break;		/* Å` 0x8160(01-33) U+301C U+FF5E */
	}
	return code;
}
DWORD CTextRam::UnicodeNomal(DWORD code1, DWORD code2)
{
	#include "UniNomalTab.h"
	static BOOL UniNomInitFlag = FALSE;
	static struct _UNINOMTAB *HashTab[256];

	int n, c, hs;
	struct _UNINOMTAB *tp;

	if ( !UniNomInitFlag ) {
		for ( n = 0 ; n < 256 ; n++ )
			HashTab[n] = NULL;
		for ( n = 0 ; n < UNINOMALTABMAX ; n++ ) {
			hs = (UniNomalTab[n].code[0] * 31 + UniNomalTab[n].code[1]) & 255;
			UniNomalTab[n].next = HashTab[hs];
			HashTab[hs] = &(UniNomalTab[n]);
		}
		UniNomInitFlag = TRUE;
	}

	code1 = UCS2toUCS4(code1);
	code2 = UCS2toUCS4(code2);

	hs = (code1 * 31 + code2) & 255;
	for ( tp = HashTab[hs] ; tp != NULL ; tp = tp->next ) {
		if ( (c = (int)(tp->code[0] - code1)) == 0 && (c = (int)(tp->code[1] - code2)) == 0 )
			return UCS4toUCS2(tp->code[2]);
		else if ( c < 0 )
			break;
	}
	return 0;
}
DWORD CTextRam::UCS2toUCS4(DWORD code)
{
	//  1101 10xx	U+D800 - U+DBFF	è„à ÉTÉçÉQÅ[Ég
	//	1101 11xx	U+DC00 - U+DFFF	â∫à ÉTÉçÉQÅ[Ég
	if ( (code & 0xFC00FC00L) == 0xD800DC00L )
		code = (((code & 0x03FF0000L) >> 6) | (code & 0x3FF)) + 0x10000L;
	return code;
}
DWORD CTextRam::UCS4toUCS2(DWORD code)
{
	if ( (code & 0xFFFF0000L) >= 0x00010000L && (code & 0xFFFF0000L) < 0x00100000L ) {
		code -= 0x10000L;
		code = (((code & 0xFFC00L) << 2) | (code & 0x3FF)) | 0xD800DC00L;
	}
	return code;
}

//////////////////////////////////////////////////////////////////////

void CTextRam::SetRetChar(BOOL f8)
{
	if ( f8 ) {
		m_RetChar[RC_DCS] = "\x90";
		m_RetChar[RC_SOS] = "\x98";
		m_RetChar[RC_CSI] = "\x9b";
		m_RetChar[RC_ST]  = "\x9c";
		m_RetChar[RC_OSC] = "\x9d";
		m_RetChar[RC_PM]  = "\x9e";
		m_RetChar[RC_APC] = "\x9f";
	} else {
		m_RetChar[RC_DCS] = "\033P";
		m_RetChar[RC_SOS] = "\033X";
		m_RetChar[RC_CSI] = "\033[";
		m_RetChar[RC_ST]  = "\033\\";
		m_RetChar[RC_OSC] = "\033]";
		m_RetChar[RC_PM]  = "\033^";
		m_RetChar[RC_APC] = "\033_";
	}
}
void CTextRam::AddGrapWnd(void *pWnd)
{
	m_GrapWndTab.Add(pWnd);
}
void CTextRam::RemoveGrapWnd(void *pWnd)
{
	for ( int n = 0 ; n < m_GrapWndTab.GetSize() ; n++ ) {
		if ( m_GrapWndTab[n] == pWnd ) {
			m_GrapWndTab.RemoveAt(n);
			break;
		}
	}
}
void *CTextRam::LastGrapWnd(int type)
{
	for ( int n = m_GrapWndTab.GetSize() - 1 ; n >= 0 ; n-- ) {
		CGrapWnd *pWnd = (CGrapWnd *)(m_GrapWndTab[n]);
		if ( pWnd->m_Type == type )
			return (void *)pWnd;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////
// Low Level
//////////////////////////////////////////////////////////////////////

void CTextRam::RESET(int mode)
{
	if ( mode & RESET_CURSOR ) {
		if ( m_VRam != NULL )
			SETPAGE(0);
		else
			m_Page = 0;

		m_CurX = 0;
		m_CurY = 0;
		m_TopY = 0;
		m_BtmY = m_Lines;
		m_LeftX = 0;
		m_RightX = m_Cols;

		m_DoWarp = FALSE;
		m_Status = ST_NON;
		m_LastChar = 0;
		m_LastPos  = 0;
		m_Kan_Pos = 0;
		memset(m_Kan_Buf, 0, sizeof(m_Kan_Buf));
	}

	if ( mode & RESET_TABS ) {
		m_DefTab = DEF_TAB;
		TABSET(TAB_RESET);
	}

	if ( mode & RESET_BANK ) {
		m_BankGL = 0;
		m_BankGR = 1;
		m_BankSG = (-1);
		memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));
	}

	if ( mode & RESET_ATTR ) {
		m_DefAtt.ch = 0;
		m_DefAtt.md = m_BankTab[m_KanjiMode][m_BankGL];
		m_DefAtt.em = 0;
		m_DefAtt.dm = 0;
		m_DefAtt.cm = 0;
		m_DefAtt.at = 0;
		m_AttSpc = m_DefAtt;
		m_AttNow = m_DefAtt;
	}

	if ( mode & RESET_COLOR ) {
		int n = 16;
		memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));

		// colors 16-231 are a 6x6x6 color cube
		for ( int r = 0 ; r < 6 ; r++ ) {
			for ( int g = 0 ; g < 6 ; g++ ) {
				for ( int b = 0 ; b < 6 ; b++ )
					m_ColTab[n++] = RGB(r * 51, g * 51, b * 51);
			}
		}
		// colors 232-255 are a grayscale ramp, intentionally leaving out
		for ( int g = 0 ; g < 24 ; g++ )
			m_ColTab[n++] = RGB(g * 11, g * 11, g * 11);
	}

	if ( mode & RESET_OPTION ) {
		memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));
		m_VtLevel = 65;
		m_TermId  = 10;
		m_LangMenu = 0;
		SetRetChar(FALSE);
	}

	if ( mode & RESET_CLS )
		ERABOX(0, 0, m_Cols, m_Lines);

	if ( mode & RESET_SAVE ) {
		m_Save_CurX = m_CurX;
		m_Save_CurY = m_CurY;
		m_Save_AttNow = m_AttNow;
		m_Save_AttSpc = m_AttSpc;
		m_Save_BankGL = m_BankGL;
		m_Save_BankGR = m_BankGR;
		m_Save_BankSG = m_BankSG;
		m_Save_DoWarp = m_DoWarp;
		memcpy(m_Save_BankTab, m_BankTab, sizeof(m_BankTab));
		memcpy(m_Save_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
		memcpy(m_Save_TabMap, m_TabMap, sizeof(m_TabMap));
		for ( int n = 0 ; n < 64 ; n++ )
			m_Macro[n].Clear();
	}

	m_RecvCrLf = IsOptValue(TO_RLRECVCR, 2);
	m_SendCrLf = IsOptValue(TO_RLECHOCR, 2);

	if ( mode & RESET_MOUSE ) {
		m_MouseTrack = MOS_EVENT_NONE;
		m_MouseRect.SetRectEmpty();
		m_Loc_Mode  = 0;
		m_Loc_Pb    = 0;
		m_Loc_LastX = 0;
		m_Loc_LastY = 0;
	}

	if ( mode & RESET_CHAR ) {
		m_Exact = FALSE;
		m_StsFlag = FALSE;
		m_StsMode = 0;
		m_StsLed  = 0;
		fc_Init(m_KanjiMode);
	}

	if ( mode & RESET_TEK ) {
		m_Tek_Ver  = 4014;
		m_Tek_Mode = 0x1F;
		m_Tek_Pen  = 0;
		m_Tek_Stat = 0;
		m_Tek_Line = 0x08;
		m_Tek_Font = 0x08;
		m_Tek_Angle = 0;
		m_Tek_Base  = 0;
		m_Tek_Point = 0;
		m_Tek_Gin   = FALSE;
		m_Tek_Flush = FALSE;
		m_Tek_Int   = 0;
		m_Tek_cX = 0; m_Tek_cY = TEK_WIN_HEIGHT - 87;
		m_Tek_lX = 0; m_Tek_lY = m_Tek_cY;
		m_Tek_wX = 0; m_Tek_wY = m_Tek_cY;
		TekClear();
	}
}
VRAM *CTextRam::GETVRAM(int cols, int lines)
{
	int pos = m_HisPos + lines;
	while ( pos < 0 )
		pos += m_HisMax;
	while ( pos >= m_HisMax )
		pos -= m_HisMax;
	return (m_VRam + cols + m_ColsMax * pos);
}
void CTextRam::UNGETSTR(LPCSTR str, ...)
{
	CString tmp;
	va_list arg;
	va_start(arg, str);
	tmp.FormatV(str, arg);
	if ( m_pDocument != NULL )
		m_pDocument->SocketSend((void *)(LPCSTR)tmp, tmp.GetLength());
	va_end(arg);
}
void CTextRam::BEEP()
{
	if ( !IS_ENABLE(m_AnsiOpt, TO_RLADBELL) )
		MessageBeep(MB_OK);
	if ( IS_ENABLE(m_AnsiOpt, TO_RLVSBELL) )
		m_pDocument->UpdateAllViews(NULL, UPDATE_VISUALBELL, NULL);
}
void CTextRam::FLUSH()
{
	if ( m_UpdateRect == CRect(0, 0, m_Cols, m_Lines) )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
	else if ( m_UpdateRect.left < m_UpdateRect.right && m_UpdateRect.top < m_UpdateRect.bottom )
		m_pDocument->UpdateAllViews(NULL, UPDATE_TEXTRECT, (CObject *)&m_UpdateRect);
	else
		m_pDocument->UpdateAllViews(NULL, UPDATE_GOTOXY, NULL);

	m_UpdateRect.left   = m_Cols;
	m_UpdateRect.top    = m_Lines;
	m_UpdateRect.right  = 0;
	m_UpdateRect.bottom = 0;

	m_HisUse = 0;
}
void CTextRam::CUROFF()
{
	m_DispCaret &= ~002;
}	
void CTextRam::CURON()
{
	m_DispCaret |= 002;
}
void CTextRam::DISPUPDATE()
{
	m_UpdateRect.left   = 0;
	m_UpdateRect.top    = 0;
	m_UpdateRect.right  = m_Cols;
	m_UpdateRect.bottom = m_Lines;

	if ( IsOptEnable(TO_DECSCLM) )
		m_UpdateFlag = TRUE;
	else
		m_pDocument->IncActCount();
}
void CTextRam::DISPVRAM(int sx, int sy, int w, int h)
{
	int ex = sx + w;
	int ey = sy + h;

	if ( m_UpdateRect.left > sx )
		m_UpdateRect.left = sx;
	if ( m_UpdateRect.right < ex )
		m_UpdateRect.right = ex;
	if ( m_UpdateRect.top> sy )
		m_UpdateRect.top = sy;
	if ( m_UpdateRect.bottom < ey )
		m_UpdateRect.bottom = ey;

	if ( IsOptEnable(TO_DECSCLM) )
		m_UpdateFlag = TRUE;
	else
		m_pDocument->IncActCount();
}
int CTextRam::BLINKUPDATE(class CRLoginView *pView)
{
	int x, y, dm;
	VRAM *vp;
	int rt = 0;
	int mk = ATT_BLINK;
	CRect rect;

	if ( (pView->m_BlinkFlag & 1) == 0 )
		mk |= ATT_SBLINK;

	for ( y = 0 ; y < m_Lines ; y++ ) {
		vp = GETVRAM(0, y - pView->m_HisOfs + pView->m_HisMin);
		dm = vp->dm;
		rect.left   = m_Cols;
		rect.right  = 0;
		rect.top    = y - pView->m_HisOfs + pView->m_HisMin;
		rect.bottom = rect.top + 1;
		for ( x = 0 ; x < m_Cols ; x++ ) {
			if ( (vp->at & mk) != 0 ) {
				if ( rect.left > x ) {
					rect.left  = x;
					rect.right = x + 1;
					rt |= 3;
				} else if ( rect.right == x ) {
					rect.right = x + 1;
					rt |= 3;
				} else {
					if ( rect.left < rect.right ) {
						if ( dm ) {
							rect.left  *= 2;
							rect.right *= 2;
						}
						m_pDocument->UpdateAllViews(NULL, UPDATE_BLINKRECT, (CObject *)&rect);
					}
					rect.left  = x;
					rect.right = x + 1;
					rt |= 3;
				}
			} else if ( (vp->at & ATT_SBLINK) != 0 )
				rt |= 2;
			vp++;
		}
		if ( (rt & 1) != 0 ) {
			if ( dm ) {
				rect.left  *= 2;
				rect.right *= 2;
			}
			m_pDocument->UpdateAllViews(NULL, UPDATE_BLINKRECT, (CObject *)&rect);
			rt &= 0xFE;
		}
	}
	return rt;
}

//////////////////////////////////////////////////////////////////////
// Mid Level
//////////////////////////////////////////////////////////////////////

int CTextRam::GetAnsiPara(int index, int defvalue, int limit)
{
	if ( index >= m_AnsiPara.GetSize() || m_AnsiPara[index] == 0xFFFF || m_AnsiPara[index] < limit )
		return defvalue;
	return m_AnsiPara[index];
}
void CTextRam::SetAnsiParaArea(int top)
{
	while ( m_AnsiPara.GetSize() < top )
		m_AnsiPara.Add(0);

	for ( int n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( m_AnsiPara[n] == 0xFFFF )
			m_AnsiPara[n] = 0;
	}

	// Start Line
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(1);
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Lines )
		m_AnsiPara[top] = m_Lines - 1;
	top++;

	// Start Cols
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(1);
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Cols )
		m_AnsiPara[top] = m_Cols - 1;
	top++;

	// End Line
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(m_Lines);
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Lines )
		m_AnsiPara[top] = m_Lines - 1;
	top++;

	// End Cols
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(m_Cols);
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Cols )
		m_AnsiPara[top] = m_Cols - 1;
}
void CTextRam::LocReport(int md, int sw, int x, int y)
{
	// DECLRP(Locator report): CSI Pe ; Pb ; Pr ; Pc ; Pp & w
	// Pe is the event code 
	// Pb is the button code 
	// Pr is the row coordinate 
	// Pc is the column coordinate 
	// Pp is the third coordinate (page number) 
	//
	// Pe, the event code indicates what event caused this report to be generated.
	// The following event codes are defined:
	// 0 - request, the terminal received an explicit request for a locator report, but the locator is unavailable 
	// 1 - request, the terminal received an explicit request for a locator report 
	// 2 - left button down 
	// 3 - left button up 
	// 4 - middle button down 
	// 5 - middle button up 
	// 6 - right button down 
	// 7 - right button up 
	// 8 - fourth button down 
	// 9 - fourth button up 
	// 10 - locator outside filter rectangle 
	//
	// Pb is the button code, ASCII decimal 0-15 indicating which buttons are
	// down if any. The state of the four buttons on the locator correspond
	// to the low four bits of the decimal value, "1" means button depressed
	//
	// 0 - no buttons down 
	// 1 - right 
	// 2 - middle 
	// 4 - left 
	// 8 - fourth 

	int Pe = 0;

	if ( md == MOS_LOCA_INIT ) {			// Init Mode
		m_MouseTrack = ((m_Loc_Mode & LOC_MODE_ENABLE) != 0 ? MOS_EVENT_LOCA : MOS_EVENT_NONE);
		m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
		return;

	} else if ( md == MOS_LOCA_REQ ) {		// Request
		if ( (m_Loc_Mode & LOC_MODE_ENABLE) != 0 )
			Pe = 1;

	} else {					// Mouse Event
		m_Loc_LastX = x;
		m_Loc_LastY = y;
		m_Loc_Pb    = 0;

		if ( (sw & MK_RBUTTON) != 0 )
			m_Loc_Pb |= 1;

		if ( (sw & MK_LBUTTON) != 0 )
			m_Loc_Pb |= 4;

		switch(md) {
		case MOS_LOCA_LEDN:	// Left Down
			Pe = 2;
			break;
		case MOS_LOCA_LEUP:	// Left Up
			Pe = 3;
			break;
		case MOS_LOCA_RTDN:	// Right Down
			Pe = 6;
			break;
		case MOS_LOCA_RTUP:	// Right Up
			Pe = 7;
			break;
		case MOS_LOCA_MOVE:	// Mouse Move
			if ( (m_Loc_Mode & LOC_MODE_FILTER) == 0 )
				return;
			if ( !m_Loc_Rect.IsRectEmpty() && m_Loc_Rect.PtInRect(CPoint(m_Loc_LastX, m_Loc_LastY)) )
				return;
			Pe = 10;
			m_Loc_Mode &= ~LOC_MODE_FILTER;
			break;
		}

		if ( (m_Loc_Mode & LOC_MODE_ONESHOT) == 0 ) {
			if ( (m_Loc_Mode & LOC_MODE_EVENT) == 0 )
				return;
			else if ( (md == MOS_LOCA_LEDN || md == MOS_LOCA_RTDN) && (m_Loc_Mode & LOC_MODE_DOWN) == 0 )
				return;
			else if ( (md == MOS_LOCA_LEUP || md == MOS_LOCA_RTUP) && (m_Loc_Mode & LOC_MODE_UP) == 0 )
				return;
		}
	}

	x = m_Loc_LastX;
	y = m_Loc_LastY;

	if ( (m_Loc_Mode & LOC_MODE_PIXELS) != 0 ) {
		x *= 6;
		y *= 12;
	}

	UNGETSTR("%s%d;%d;%d;%d;%d&w", m_RetChar[RC_CSI], Pe, m_Loc_Pb, y + 1, x + 1, 0);

	if ( (m_Loc_Mode & LOC_MODE_ONESHOT) != 0 ) {
		m_Loc_Mode &= ~(LOC_MODE_ENABLE | LOC_MODE_ONESHOT);
		m_MouseTrack = MOS_EVENT_NONE;
		m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
	}
}
LPCSTR CTextRam::GetHexPara(LPCSTR str, CBuffer &buf)
{
	// ! Pn ; D...D ;		! is the repeat sequence introducer.

	int n;
	CBuffer tmp;

	buf.Clear();
	while ( *str != '\0' ) {
		if ( *str == ';' ) {
			str++;
			break;

		} else if ( *str == '!' ) {
			str++;
			for ( n = 0 ; isdigit(*str) ; )
				n = n * 10 + (*(str++) - '0');
			if ( *str != ';' )
				break;
			str++;
			str = GetHexPara(str, tmp);
			while ( n-- > 0 )
				buf.Apend(tmp.GetPtr(), tmp.GetSize());

		} else if ( isxdigit(str[0]) && isxdigit(str[1]) ) {
			str = tmp.Base16Decode(str);
			buf.Apend(tmp.GetPtr(), tmp.GetSize());
		} else
			break;
	}
	return str;
}

void CTextRam::LOCATE(int x, int y)
{
	m_DoWarp = FALSE;
	if ( (m_CurX = x) >= m_Cols )
		m_CurX = m_Cols - 1;
	else if ( m_CurX < 0 )
		m_CurX = 0;
	if ( (m_CurY = y) >= m_Lines )
		m_CurY = m_Lines - 1;
	else if ( m_CurY < 0 )
		m_CurY = 0;
	if ( m_CurX >= (m_Cols / 2) && GetDm(m_CurY) != 0 )
		m_CurX = (m_Cols / 2) - 1;
}
void CTextRam::ERABOX(int sx, int sy, int ex, int ey, int df)
{
	int x, y, dm;
	VRAM *vp, *tp;

	m_DoWarp = FALSE;

	if ( sx < 0 ) sx = 0; else if ( sx >= m_Cols  ) return;
	if ( sy < 0 ) sy = 0; else if ( sy >= m_Lines ) return;
	if ( ex < sx ) return; else if ( ex > m_Cols  ) ex = m_Cols;
	if ( ey < sy ) return; else if ( ey > m_Lines ) ey = m_Lines;

	if ( (df & ERM_ISOPRO) != 0 && IsOptEnable(TO_ANSIERM) )
		df &= ~ERM_ISOPRO;

	for ( y = sy ; y < ey ; y++ ) {
		tp = GETVRAM(0, y);
		dm = tp->dm;
		vp = tp + sx;

		switch(df & (ERM_ISOPRO | ERM_DECPRO)) {
		case 0:		// clear em
			for ( x = sx ; x < ex ; x++ )
				*(vp++) = m_AttSpc;
			break;
		case ERM_ISOPRO:
			for ( x = sx ; x < ex ; x++, vp++ ) {
				if ( (vp->em & EM_ISOPROTECT) == 0 )
					*vp = m_AttSpc;
			}
			break;
		case ERM_DECPRO:
			for ( x = sx ; x < ex ; x++, vp++ ) {
				if ( (vp->em & EM_DECPROTECT) == 0 )
					*vp = m_AttSpc;
			}
			break;
		}

		if ( (df & ERM_SAVEDM) == 0 ) {
			tp->dm = 0;
			DISPVRAM(sx, y, ex - sx, 1);
		} else {
			if ( (tp->dm = dm) != 0 )
				DISPVRAM(sx * 2, y, (ex - sx) * 2, 1);
			else
				DISPVRAM(sx, y, ex - sx, 1);
		}
	}
}
void CTextRam::FORSCROLL(int sy, int ey)
{
	ASSERT(sy >= 0 && sy < m_Lines);
	ASSERT(ey >= 0 && ey <= m_Lines);

	m_DoWarp = FALSE;
	if ( sy == 0 && ey == m_Lines && !m_LineEditMode ) {
		CallReciveLine(0);
		m_HisUse++;
		if ( (m_HisPos += 1) >= m_HisMax )
			m_HisPos -= m_HisMax;
		if ( m_HisLen < (m_HisMax - 1) )
			m_HisLen += 1;
		if ( m_LineUpdate < m_Lines )
			m_LineUpdate++;
	} else {
		for ( int y = sy + 1; y < ey ; y++ )
			memcpy(GETVRAM(0, y - 1), GETVRAM(0, y), sizeof(VRAM) * m_Cols);
	}
	ERABOX(0, ey - 1, m_Cols, ey);
	DISPVRAM(0, sy, m_Cols, ey - sy);
}
void CTextRam::BAKSCROLL(int sy, int ey)
{
	ASSERT(sy >= 0 && sy < m_Lines);
	ASSERT(ey >= 0 && ey <= m_Lines);

	m_DoWarp = FALSE;
	for ( int y = ey - 1; y > sy ; y-- )
		memcpy(GETVRAM(0, y), GETVRAM(0, y - 1), sizeof(VRAM) * m_Cols);
	ERABOX(0, sy, m_Cols, sy + 1);
	DISPVRAM(0, sy, m_Cols, ey - sy);
}
void CTextRam::LEFTSCROLL()
{
	int n, i, dm;
	VRAM *vp;

	for ( n = 0 ; n < m_Lines ; n++ ) {
		vp = GETVRAM(0, n);
		dm = vp->dm;
		for ( i = 1 ; i < m_Cols ; i++ )
			vp[i - 1] = vp[i];
		vp[m_Cols - 1] = m_AttSpc;
		vp->dm = dm;
	}
	DISPVRAM(0, 0, m_Cols, m_Lines);
}
void CTextRam::RIGHTSCROLL()
{
	int n, i, dm;
	VRAM *vp;

	for ( n = 0 ; n < m_Lines ; n++ ) {
		vp = GETVRAM(0, n);
		dm = vp->dm;
		for ( i = m_Cols - 1 ; i > 0 ; i-- )
			vp[i] = vp[i - 1];
		vp[0] = m_AttSpc;
		vp->dm = dm;
	}
	DISPVRAM(0, 0, m_Cols, m_Lines);
}
void CTextRam::DOWARP()
{
	if ( !m_DoWarp )
		return;
	m_DoWarp = FALSE;
	m_CurX = 0;
	ONEINDEX();
	m_LastPos -= COLS_MAX;
}
void CTextRam::INSCHAR()
{
	int n, dm;
	VRAM *vp;

	m_DoWarp = FALSE;
	vp = GETVRAM(0, m_CurY);
	dm = vp->dm;
	for ( n = m_Cols - 1 ; n > m_CurX ; n-- )
		vp[n] = vp[n - 1];
	vp[n] = m_AttSpc;
	vp->dm = dm;
	if ( dm != 0 )
		DISPVRAM(0, m_CurY, m_Cols, 1);
	else
		DISPVRAM(m_CurX, m_CurY, m_Cols - m_CurX, 1);
}
void CTextRam::DELCHAR()
{
	int n, dm;
	VRAM *vp;

	m_DoWarp = FALSE;
	vp = GETVRAM(0, m_CurY);
	dm = vp->dm;
	for ( n = m_CurX + 1; n < m_Cols ; n++ )
		vp[n - 1] = vp[n];
	vp[n - 1] = m_AttSpc;
	vp->dm = dm;
	if ( dm != 0 )
		DISPVRAM(0, m_CurY, m_Cols, 1);
	else
		DISPVRAM(m_CurX, m_CurY, m_Cols - m_CurX, 1);
}
void CTextRam::ONEINDEX()
{
	m_DoWarp = FALSE;
	if ( m_CurY < m_BtmY ) {
		if ( ++m_CurY >= m_BtmY ) {
			m_CurY = m_BtmY - 1;
			FORSCROLL(m_TopY, m_BtmY);
			m_LineEditIndex++;
		}
	} else if ( ++m_CurY >= m_Lines )
		m_CurY = m_Lines - 1;
}
void CTextRam::REVINDEX()
{
	m_DoWarp = FALSE;
	if ( m_CurY >= m_TopY ) {
		if ( --m_CurY < m_TopY ) {
			m_CurY = m_TopY;
			BAKSCROLL(m_TopY, m_BtmY);
		}
	} else if ( --m_CurY < 0 )
		m_CurY = 0;
}
void CTextRam::PUT1BYTE(int ch, int md)
{
	if ( m_StsFlag ) {
		md &= CODE_MASK;
		ch |= m_FontTab[md].m_Shift;
		ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, "UTF-16BE", ch);			// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
		ch = IconvToMsUnicode(ch);
		m_LastChar = ch;
		m_LastPos  = 0;
		if ( (ch & 0xFFFF0000) != 0 )
			m_StsPara += (WCHAR)(ch >> 16);
		m_StsPara += (WCHAR)ch;
		((CMainFrame *)AfxGetMainWnd())->SetMessageText(CString(m_StsPara));
		return;
	}

	DOWARP();

	VRAM *vp;
	int dm = GetDm(m_CurY);
	int mx = (dm != 0 ? (m_Cols / 2) : m_Cols);

	vp = GETVRAM(m_CurX, m_CurY);
	vp->md = (WORD)md;
	vp->em = m_AttNow.em;
//	vp->dm = m_AttNow.dm;	no Init
	vp->cm = CM_ASCII;
	vp->at = m_AttNow.at;
	vp->ft = m_AttNow.ft;
	vp->fc = m_AttNow.fc;
	vp->bc = m_AttNow.bc;

	md &= CODE_MASK;
	ch |= m_FontTab[md].m_Shift;
	ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, "UTF-16BE", ch);			// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
	ch = IconvToMsUnicode(ch);

	vp->ch = ch;

	if ( dm != 0 )
		DISPVRAM(m_CurX * 2, m_CurY, 2, 1);
	else
		DISPVRAM(m_CurX, m_CurY, 1, 1);

	m_LastChar = ch;
	m_LastPos  = m_CurX + m_CurY * COLS_MAX;

	if ( ch != 0 )
		CallReciveChar(ch);

	if ( ++m_CurX >= mx ) {
		if ( IS_ENABLE(m_AnsiOpt, TO_DECAWM) != 0 ) {
			if ( IS_ENABLE(m_AnsiOpt, TO_RLGNDW) != 0 ) {
				ONEINDEX();
				m_CurX = 0;
				m_LastPos -= COLS_MAX;
			} else {
				m_CurX = mx - 1;
				m_DoWarp = TRUE;
			}
		} else 
			m_CurX = mx - 1;
	}
}
void CTextRam::PUT2BYTE(int ch, int md)
{
	if ( m_StsFlag ) {
		md &= CODE_MASK;
		ch |= m_FontTab[md].m_Shift;
		ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, "UTF-16BE", ch);			// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
		ch = IconvToMsUnicode(ch);
		m_LastChar = ch;
		m_LastPos  = 0;
		if ( (ch & 0xFFFF0000) != 0 )
			m_StsPara += (WCHAR)(ch >> 16);
		m_StsPara += (WCHAR)ch;
		((CMainFrame *)AfxGetMainWnd())->SetMessageText(CString(m_StsPara));
		return;
	}

	if ( !m_DoWarp && (m_CurX + 1) >= m_Cols )
		PUT1BYTE(0, md);

	DOWARP();

	VRAM *vp;
	int dm = GetDm(m_CurY);
	int mx = (dm != 0 ? (m_Cols / 2) : m_Cols);

	vp = GETVRAM(m_CurX, m_CurY);
	vp[0].md = vp[1].md = (WORD)md;
	vp[0].em = vp[1].em = m_AttNow.em;
//	vp[0].dm = vp[1].dm = m_AttNow.dm;	no Init
	vp[0].cm = CM_1BYTE;
	vp[1].cm = CM_2BYTE;
	vp[0].at = vp[1].at = m_AttNow.at;
	vp[0].ft = vp[1].ft = m_AttNow.ft;
	vp[0].fc = vp[1].fc = m_AttNow.fc;
	vp[0].bc = vp[1].bc = m_AttNow.bc;

	md &= CODE_MASK;
	ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, "UTF-16BE", ch);			// Charïœä∑Ç≈ÇÕUTF-16BEÇégópÅI
	ch = IconvToMsUnicode(ch);

	vp[0].ch = vp[1].ch = ch;

	if ( dm != 0 )
		DISPVRAM(m_CurX * 2, m_CurY, 4, 1);
	else
		DISPVRAM(m_CurX, m_CurY, 2, 1);

	m_LastChar = ch;
	m_LastPos  = m_CurX + m_CurY * COLS_MAX;

	if ( ch != 0 )
		CallReciveChar(ch);

	if ( (m_CurX += 2) >= mx ) {
		if ( IS_ENABLE(m_AnsiOpt, TO_DECAWM) != 0 ) {
			if ( IS_ENABLE(m_AnsiOpt, TO_RLGNDW) != 0 ) {
				ONEINDEX();
				m_CurX = 0;
				m_LastPos -= COLS_MAX;
			} else {
				m_CurX = mx - 1;
				m_DoWarp = TRUE;
			}
		} else 
			m_CurX = mx - 1;
	}
}
void CTextRam::ANSIOPT(int opt, int bit)
{
	ASSERT(opt >= 0 && opt < 512 );

	switch(opt) {
	case 'h':
		m_AnsiOpt[bit / 32] |= (1 << (bit % 32));
		break;
	case 'l':
		m_AnsiOpt[bit / 32] &= ~(1 << (bit % 32));
		break;
	case 'r':
		m_AnsiOpt[bit / 32] &= ~(1 << (bit % 32));
		m_AnsiOpt[bit / 32] |= (m_Save_AnsiOpt[bit / 32] & (1 << (bit % 32)));
		break;
	case 's':
		m_Save_AnsiOpt[bit / 32] &= ~(1 << (bit % 32));
		m_Save_AnsiOpt[bit / 32] |= (m_AnsiOpt[bit / 32] & (1 << (bit % 32)));
		break;
	}
}
void CTextRam::INSMDCK(int len)
{
	if ( IS_ENABLE(m_AnsiOpt, TO_ANSIIRM) == 0 )
		return;
	while ( len-- > 0 )
		INSCHAR();
}
void CTextRam::SAVERAM()
{
	int n;
	CTextSave *pNext;
	CTextSave *pSave = new CTextSave;

	pSave->m_VRam = new VRAM[m_Cols * m_Lines];
	for ( n = 0 ; n < m_Lines ; n++ )
		memcpy(pSave->m_VRam + n * m_Cols, GETVRAM(0, n), sizeof(VRAM) * m_Cols);

	pSave->m_AttNow = m_AttNow;
	pSave->m_AttSpc = m_AttSpc;
	pSave->m_Cols   = m_Cols;
	pSave->m_Lines  = m_Lines;
	pSave->m_CurX   = m_CurX;
	pSave->m_CurY   = m_CurY;
	pSave->m_TopY   = m_TopY;
	pSave->m_BtmY   = m_BtmY;
	pSave->m_LeftX  = m_LeftX;
	pSave->m_RightX = m_RightX;
	pSave->m_DoWarp = m_DoWarp;
	memcpy(pSave->m_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
	pSave->m_BankGL = m_BankGL;
	pSave->m_BankGR = m_BankGR;
	pSave->m_BankSG = m_BankSG;
	memcpy(pSave->m_BankTab, m_BankTab, sizeof(m_BankTab));
	memcpy(pSave->m_TabMap, m_TabMap, sizeof(m_TabMap));

	pSave->m_Save_CurX   = m_Save_CurX;
	pSave->m_Save_CurY   = m_Save_CurY;
	pSave->m_Save_AttNow = m_Save_AttNow;
	pSave->m_Save_AttSpc = m_Save_AttSpc;
	pSave->m_Save_BankGL = m_Save_BankGL;
	pSave->m_Save_BankGR = m_Save_BankGR;
	pSave->m_Save_BankSG = m_Save_BankSG;
	pSave->m_Save_DoWarp = m_Save_DoWarp;
	memcpy(pSave->m_Save_BankTab, m_Save_BankTab, sizeof(m_Save_BankTab));
	memcpy(pSave->m_Save_AnsiOpt, m_Save_AnsiOpt, sizeof(m_Save_AnsiOpt));
	memcpy(pSave->m_Save_TabMap, m_Save_TabMap, sizeof(m_Save_TabMap));

	pSave->m_Next = m_pTextSave;
	m_pTextSave = pNext = pSave;

	for ( n = 0 ; pSave->m_Next != NULL ; n++ ) {
		pNext = pSave;
		pSave = pSave->m_Next;
	}
	if ( n > 16 ) {
		pNext->m_Next = pSave->m_Next;
		delete pSave->m_VRam;
		delete pSave;
	}
}
void CTextRam::LOADRAM()
{
	int n;
	CTextSave *pSave;

	if ( (pSave = m_pTextSave) == NULL )
		return;
	m_pTextSave = pSave->m_Next;

	m_AttNow = pSave->m_AttNow;
	m_AttSpc = pSave->m_AttSpc;
	if ( (m_CurX = pSave->m_CurX) >= m_Cols )  m_CurX = m_Cols - 1;
	if ( (m_CurY = pSave->m_CurY) >= m_Lines ) m_CurY = m_Lines - 1;
	if ( (m_TopY = pSave->m_TopY) >= m_Lines ) m_TopY = m_Lines - 1;
	if ( (m_BtmY = pSave->m_BtmY) > m_Lines )  m_BtmY = m_Lines;
	if ( (m_LeftX  = pSave->m_LeftX) >= m_Cols ) m_LeftX  = m_Lines - 1;
	if ( (m_RightX = pSave->m_RightX) > m_Cols ) m_RightX = m_Cols;
	m_DoWarp = pSave->m_DoWarp;
	memcpy(m_AnsiOpt, pSave->m_AnsiOpt, sizeof(DWORD) * 12);	// 0 - 384
	m_BankGL = pSave->m_BankGL;
	m_BankGR = pSave->m_BankGR;
	m_BankSG = pSave->m_BankSG;
	memcpy(m_BankTab, pSave->m_BankTab, sizeof(m_BankTab));
	memcpy(m_TabMap, pSave->m_TabMap, sizeof(m_TabMap));

	m_Save_CurX   = pSave->m_Save_CurX;
	m_Save_CurY   = pSave->m_Save_CurY;
	m_Save_AttNow = pSave->m_Save_AttNow;
	m_Save_AttSpc = pSave->m_Save_AttSpc;
	m_Save_BankGL = pSave->m_Save_BankGL;
	m_Save_BankGR = pSave->m_Save_BankGR;
	m_Save_BankSG = pSave->m_Save_BankSG;
	m_Save_DoWarp = pSave->m_Save_DoWarp;
	memcpy(m_Save_BankTab, pSave->m_Save_BankTab, sizeof(m_BankTab));
	memcpy(m_Save_AnsiOpt, pSave->m_Save_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_Save_TabMap, pSave->m_Save_TabMap, sizeof(m_TabMap));

	m_RecvCrLf = IsOptValue(TO_RLRECVCR, 2);
	m_SendCrLf = IsOptValue(TO_RLECHOCR, 2);

	for ( n = 0 ; n < pSave->m_Lines && n < m_Lines ; n++ ) {
		if ( pSave->m_Cols >= m_Cols )
			memcpy(GETVRAM(0, n), pSave->m_VRam + n * pSave->m_Cols, sizeof(VRAM) * m_Cols);
		else
			memcpy(GETVRAM(0, n), pSave->m_VRam + n * pSave->m_Cols, sizeof(VRAM) * pSave->m_Cols);
	}

	DISPVRAM(0, 0, m_Cols, m_Lines);

	delete pSave->m_VRam;
	delete pSave;
}
void CTextRam::PUSHRAM()
{
	CTextSave *pSave;

	SAVERAM();

	if ( (pSave = m_pTextStack) != NULL ) {
		m_pTextStack = pSave->m_Next;
		pSave->m_Next = m_pTextSave;
		m_pTextSave = pSave;
		LOADRAM();
	} else {
		ERABOX(0, 0, m_Cols, m_Lines);
		LOCATE(0, 0);
	}
}
void CTextRam::POPRAM()
{
	CTextSave *pSave;

	SAVERAM();

	if ( (pSave = m_pTextSave) != NULL ) {
		m_pTextSave = pSave->m_Next;
		pSave->m_Next = m_pTextStack;
		m_pTextStack = pSave;
	}

	if ( m_pTextSave != NULL ) {
		LOADRAM();
	} else {
		ERABOX(0, 0, m_Cols, m_Lines);
		LOCATE(0, 0);
	}
}
void CTextRam::SETPAGE(int page)
{
	CTextSave *pSave;

	if ( page < 0 )
		page = 0;
	else if ( page > 100 )
		page = 100;

	if ( m_PageTab.GetSize() <= m_Page )
		m_PageTab.SetSize(m_Page + 1);

	if ( m_PageTab.GetSize() <= page )
		m_PageTab.SetSize(page + 1);

	if ( (pSave = (CTextSave *)m_PageTab[m_Page]) != NULL ) {
		delete pSave->m_VRam;
		delete pSave;
		m_PageTab[m_Page] = NULL;
	}

	SAVERAM();

	if ( (pSave = m_pTextSave) != NULL ) {
		m_pTextSave = pSave->m_Next;
		m_PageTab[m_Page] = pSave;
	}

	if ( (pSave = (CTextSave *)m_PageTab[page]) != NULL ) {
		m_PageTab[page] = NULL;
		pSave->m_Next = m_pTextSave;
		m_pTextSave = pSave;
		LOADRAM();
	} else {
		ERABOX(0, 0, m_Cols, m_Lines);
		LOCATE(0, 0);
	}

	m_Page = page;
}
void CTextRam::TABSET(int sw)
{
	int n, i;


	switch(sw) {
	case TAB_COLSSET:		// Cols Set
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			m_TabMap[m_CurY + 1][m_CurX / 8 + 1] |= (0x80 >> (m_CurX % 8));
		else							// SINGLE
			m_TabMap[0][m_CurX / 8 + 1] |= (0x80 >> (m_CurX % 8));
		break;
	case TAB_COLSCLR:		// Cols Clear
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			m_TabMap[m_CurY + 1][m_CurX / 8 + 1] &= ~(0x80 >> (m_CurX % 8));
		else							// SINGLE
			m_TabMap[0][m_CurX / 8 + 1] &= ~(0x80 >> (m_CurX % 8));
		break;
	case TAB_COLSALLCLR:	// Cols All Claer
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			memset(&(m_TabMap[m_CurY + 1][1]), 0, COLS_MAX / 8);
		else							// SINGLE
			memset(&(m_TabMap[0][1]), 0, COLS_MAX / 8);
		break;

	case TAB_COLSALLCLRACT:	// Cols All Clear if Active Line
		if ( (m_TabMap[m_CurY + 1][0] & 001) != 0 )
			memset(&(m_TabMap[m_CurY + 1][1]), 0, COLS_MAX / 8);
		break;

	case TAB_LINESET:		// Line Set
		m_TabMap[m_CurY + 1][0] |= 001;
		break;
	case TAB_LINECLR:		// Line Clear
		m_TabMap[m_CurY + 1][0] &= ~001;
		break;
	case TAB_LINEALLCLR:	// Line All Clear
		for ( n = 0 ; n < m_Lines ; n++ )
			m_TabMap[n + 1][0] &= ~001;
		break;

	case TAB_RESET:			// Reset
		m_TabMap[0][0] = 001;
		memset(&(m_TabMap[0][1]), 0, COLS_MAX / 8);
		for ( i = 0 ; i < LINE_MAX ; i += m_DefTab )
			m_TabMap[0][i / 8 + 1] |= (0x80 >> (i % 8));
		for ( n = 1 ; n <= m_Lines ; n++ )
			memcpy(&(m_TabMap[n][0]), &(m_TabMap[0][0]), COLS_MAX / 8 + 1);
		break;

	case TAB_DELINE:		// Delete Line
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_CurY + 1 ; n < m_Lines ; n++ )
				memcpy(m_TabMap[n], m_TabMap[n + 1], COLS_MAX / 8 + 1);
			m_TabMap[m_Lines][0] = 0;
			memset(&(m_TabMap[m_Lines][1]), 0, COLS_MAX / 8);
		}
		break;
	case TAB_INSLINE:		// Insert Line
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_Lines - 1 ; n > m_CurY ; n-- )
				memcpy(m_TabMap[n + 1], m_TabMap[n], COLS_MAX / 8 + 1);
			m_TabMap[m_CurY + 1][0] = 0;
			memset(&(m_TabMap[m_CurY + 1][1]), 0, COLS_MAX / 8);
		}
		break;

	case TAB_ALLCLR:		// Cols Line All Clear
		for ( n = 0 ; n <= m_Lines ; n++ )
			memset(&(m_TabMap[n][0]), 0, COLS_MAX / 8 + 1);
		break;

	case TAB_COLSNEXT:		// Cols Tab Stop
		if ( IsOptEnable(TO_XTMCUS) )
			DOWARP();
		i = (IsOptEnable(TO_ANSITSM) ? (m_CurY + 1) : 0);
		for ( n = m_CurX + 1 ; n < m_Cols ; n++ ) {
			if ( (m_TabMap[i][n / 8 + 1] & (0x80 >> (n % 8))) != 0 )
				break;
		}
		if ( !IsOptEnable(TO_XTMCUS) ) {
			if ( n >= m_Cols )
				n = m_Cols - 1;
			if ( n != m_CurX )
				LOCATE(n, m_CurY);
		} else
			LOCATE(n, m_CurY);
		break;
	case TAB_COLSBACK:		// Cols Back Tab Stop
		i = (IsOptEnable(TO_ANSITSM) ? (m_CurY + 1) : 0);
		for ( n = m_CurX - 1 ; n > 0 ; n-- ) {
			if ( (m_TabMap[i][n / 8 + 1] & (0x80 >> (n % 8))) != 0 )
				break;
		}
		LOCATE(n, m_CurY);
		break;

	case TAB_LINENEXT:		// Line Tab Stop
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_CurY + 1 ; n < m_Lines ; n++ ) {
				if ( (m_TabMap[n + 1][0] & 001) != 0 )
					break;
			}
		} else
			n = m_CurY + 1;
		LOCATE(m_CurX, n);
		break;
	case TAB_LINEBACK:		// Line Back Tab Stop
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_CurY - 1 ; n > 0 ; n-- ) {
				if ( (m_TabMap[n + 1][0] & 001) != 0 )
					break;
			}
		} else
			n = m_CurY - 1;
		LOCATE(m_CurX, n);
		break;
	}
}
void CTextRam::PUTSTR(LPBYTE lpBuf, int nBufLen)
{
	while ( nBufLen-- > 0 )
		fc_Call(*(lpBuf++));
	m_RetSync = FALSE;
}
int CTextRam::MCHAR(int val)
{
	if ( (val += ' ') > 255 )
		val = 255;
	else if ( val < ' ' )
		val = ' ';
	return val;
}
