// IConv.cpp: CIConv クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

//#include <errno.h>
#include "stdafx.h"
#include "RLogin.h"
#include "IConv.h"
#include "TextRam.h"
#include "Iso646Dlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static const WORD DecTCS[] = {
	0x25A1,	0x23B7,	0x250C,	0x2500,	0x2320,	0x2321,	0x2502,	0x23A1,	0x23A3,	0x23A4,	0x23A6,	0x239B,	0x239D,	0x239E,	0x23A0,	0x23A8,	// 02/00-02/15
	0x23AC,	0x23B2,	0x23B3,	0x2572,	0x2571,	0x2510,	0x2518,	0x25A1,	0x25A1,	0x25A1,	0x25A1,	0x25A1,	0x2264,	0x2260,	0x2265,	0x222B,	// 03/00-03/15
	0x2234,	0x221D,	0x221E,	0x00F7,	0x0394,	0x2207,	0x03A6,	0x0393,	0x223C,	0x2243,	0x0398,	0x00D7,	0x039B,	0x21D4,	0x21D2,	0x2261,	// 04/00-04/15
	0x03A0,	0x03A8,	0x25A1,	0x03A3,	0x25A1,	0x25A1,	0x221A,	0x03A9,	0x039E,	0x03A5,	0x2282,	0x2283,	0x2229,	0x222A,	0x2227,	0x2228,	// 05/00-05/15
	0x00AC,	0x03B1,	0x03B2,	0x03C7,	0x03B4,	0x03B5,	0x03C6,	0x03B3,	0x03B7,	0x03B9,	0x03B8,	0x03BA,	0x03BB,	0x25A1,	0x03BD,	0x2202,	// 06/00-06/15
	0x03C0,	0x03C8,	0x03C1,	0x03C3,	0x03C4,	0x25A1,	0x0192,	0x03C9,	0x03BE,	0x03C5,	0x03B6,	0x2190,	0x2191,	0x2192,	0x2193,	0x25A1,	// 07/00-07/15
};
static const WORD DecSGCS[] = {
	0x25C6,	0x2592,	0x2409,	0x240C,	0x240D,	0x240A,	0x00B0,	0x00B1,	0x2424,	0x240B,	0x2518,	0x2510,	0x250C,	0x2514,	0x253C,	0x23BA,	// 06/00-06/15
	0x23BB,	0x2500,	0x23BC,	0x23BD,	0x251C,	0x2524,	0x2534,	0x252C,	0x2502,	0x2264,	0x2265,	0x03C0,	0x2260,	0x00A3,	0x00B7,	0x007F,	// 07/00-07/15
};

static const WORD DecRussian[] = {	// KOI-7
	0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,	// 06/00-06/15
	0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x007F,	// 07/00-07/15
};
static const WORD DecHebrew[] = {
	0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,	// 06/00-06/15
	0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,	// 07/00-07/15
};
static const WORD DecGreek[] = {
	0x0060, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x03A7, 0x039F,	// 06/00-06/15
	0x03A0, 0x03A1, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x039E, 0x03A8, 0x03A9, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,	// 07/00-07/15
};
static const WORD DecTurkishSupp[] = {
	0x0020, 0x00A1, 0x00A2, 0x00A3, 0x0024, 0x00A5, 0x0026, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x002C, 0x002D, 0x0130, 0x002F,	// 02/00-02/15
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0034, 0x00B5, 0x00B6, 0x00B7, 0x0038, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x0131, 0x00BF,
	0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
	0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0152, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0178, 0x015E, 0x00DF,
	0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
	0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0153, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FF, 0x015F, 0x007F,	// 07/00-07/15
};
static const WORD DecHebrewSupp[] = {
	0x0020, 0x00A1, 0x00A2, 0x00A3, 0x0024, 0x00A5, 0x0026, 0x00A7, 0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x002C, 0x002D, 0x002E, 0x002F,	// 02/00-02/15
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0034, 0x00B5, 0x00B6, 0x00B7, 0x0038, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x003E, 0x00BF,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
	0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,	// 07/00-07/15
};
static const WORD DecGreekSupp[] = {
	0x0020, 0x00A1, 0x00A2, 0x00A3, 0x0024, 0x00A5, 0x0026, 0x00A7, 0x00A4, 0x00A9, 0x00AA, 0x00AB, 0x002C, 0x002D, 0x002E, 0x002F,	// 02/00-02/15
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0034, 0x00B5, 0x00B6, 0x00B7, 0x0038, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x003E, 0x00BF,
	0x03CA, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
	0x0050, 0x03A0, 0x03A1, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x03AC, 0x03AD, 0x03AE, 0x03AF, 0x005E, 0x03CC,
	0x03CB, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
	0x0070, 0x03C0, 0x03C1, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03C2, 0x03CD, 0x03CE, 0x0384, 0x007E, 0x007F,	// 07/00-07/15
};
static const WORD DecGraphicSupp[] = {
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,	// 02/00-02/15
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0152, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x0178, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0153, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x00FF, 0x007E, 0x007F,	// 07/00-07/15
};

static const WORD IBM437[] = {
	0x0020, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,	// 00/00-00/15 
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8, 0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC, 
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302, 
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,	// 08/00-08/15
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, 
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, 
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, 
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567, 
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, 
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229, 
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0, 
};
static const struct _CodePageList {
	int			codepage;
	LPCTSTR		name;
} CodePageList[] = {
	{ 37,		_T("CP037") },
	{ 154,		_T("CP154") },
	{ 273,		_T("CP273") },
	{ 278,		_T("CP278") },
	{ 280,		_T("CP280") },
	{ 284,		_T("CP284") },
	{ 285,		_T("CP285") },
	{ 297,		_T("CP297") },
	{ 367,		_T("CP367") },
	{ 423,		_T("CP423") },
	{ 424,		_T("CP424") },
	{ 437,		_T("CP437") },
	{ 500,		_T("CP500") },
	{ 737,		_T("CP737") },
	{ 775,		_T("CP775") },
	{ 819,		_T("CP819") },
	{ 850,		_T("CP850") },
	{ 852,		_T("CP852") },
	{ 853,		_T("CP853") },
	{ 855,		_T("CP855") },
	{ 856,		_T("CP856") },
	{ 857,		_T("CP857") },
	{ 858,		_T("CP858") },
	{ 860,		_T("CP860") },
	{ 861,		_T("CP861") },
	{ 862,		_T("CP862") },
	{ 863,		_T("CP863") },
	{ 864,		_T("CP864") },
	{ 865,		_T("CP865") },
	{ 866,		_T("CP866") },
	{ 869,		_T("CP869") },
	{ 870,		_T("CP870") },
	{ 871,		_T("CP871") },
	{ 874,		_T("CP874") },
	{ 875,		_T("CP875") },
	{ 880,		_T("CP880") },
	{ 905,		_T("CP905") },
	{ 922,		_T("CP922") },
	{ 924,		_T("CP00924") },
	{ 932,		_T("CP932") },
	{ 936,		_T("CP936") },
	{ 943,		_T("CP943") },
	{ 949,		_T("CP949") },
	{ 950,		_T("CP950") },
	{ 1025,		_T("CP1025") },
	{ 1026,		_T("CP1026") },
	{ 1046,		_T("CP1046") },
	{ 1047,		_T("CP1047") },
	{ 1097,		_T("CP1097") },
	{ 1112,		_T("CP1112") },
	{ 1122,		_T("CP1122") },
	{ 1123,		_T("CP1123") },
	{ 1124,		_T("CP1124") },
	{ 1125,		_T("CP1125") },
	{ 1129,		_T("CP1129") },
	{ 1130,		_T("CP1130") },
	{ 1131,		_T("CP1131") },
	{ 1132,		_T("CP1132") },
	{ 1133,		_T("CP1133") },
	{ 1137,		_T("CP1137") },
	{ 1140,		_T("CP01140") },
	{ 1141,		_T("CP01141") },
	{ 1142,		_T("CP01142") },
	{ 1143,		_T("CP01143") },
	{ 1144,		_T("CP01144") },
	{ 1145,		_T("CP01145") },
	{ 1146,		_T("CP01146") },
	{ 1147,		_T("CP01147") },
	{ 1148,		_T("CP01148") },
	{ 1149,		_T("CP01149") },
	{ 1153,		_T("CP1153") },
	{ 1154,		_T("CP1154") },
	{ 1155,		_T("CP1155") },
	{ 1156,		_T("CP1156") },
	{ 1157,		_T("CP1157") },
	{ 1158,		_T("CP1158") },
	{ 1160,		_T("CP1160") },
	{ 1161,		_T("CP1161") },
	{ 1162,		_T("CP1162") },
	{ 1163,		_T("CP1163") },
	{ 1164,		_T("CP1164") },
	{ 1166,		_T("CP1166") },
	{ 1250,		_T("CP1250") },
	{ 1251,		_T("CP1251") },
	{ 1252,		_T("CP1252") },
	{ 1253,		_T("CP1253") },
	{ 1254,		_T("CP1254") },
	{ 1255,		_T("CP1255") },
	{ 1256,		_T("CP1256") },
	{ 1257,		_T("CP1257") },
	{ 1258,		_T("CP1258") },
	{ 1361,		_T("CP1361") },
	{ 4971,		_T("CP4971") },
	{ 12712,	_T("CP12712") },
	{ 16804,	_T("CP16804") },
	{ 50221,	_T("CP50221") },
};

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CIConv::CIConv()
{
	m_Mode = 0;
	m_Cd = NULL;
	m_Left = NULL;
	m_Right = NULL;
	ZeroMemory(m_Table, sizeof(m_Table));
	m_ErrCount = 0;
	m_CodePage = 0;
	m_DecTab = NULL;
}

CIConv::~CIConv()
{
	if ( m_Left != NULL )
		delete m_Left;

	if ( m_Right != NULL )
		delete m_Right;

	if ( m_Cd != NULL && m_Cd != (iconv_t)(-1) )
	    iconv_close(m_Cd);
}
void CIConv::IConvClose()
{
	if ( m_Left != NULL )
		delete m_Left;
	m_Left = NULL;

	if ( m_Right != NULL )
		delete m_Right;
	m_Right = NULL;

	if ( m_Cd != NULL && m_Cd != (iconv_t)(-1) )
	    iconv_close(m_Cd);

	m_Cd = NULL;
	m_ErrCount = 0;
}

/******************
SJIS	JISX213-2
0xF040	0x2121		0xF09F	0x2821
0xF140	0x2321		0xF19F	0x2421
0xF240	0x2521		0xF29F	0x2C21
0xF340	0x2D21		0xF39F	0x2E21
0xF440	0x2F21		0xF49F	0x6E21
0xF540	0x6F21		0xF59F	0x7021
0xF640	0x7121		0xF69F	0x7221
0xF740	0x7321		0xF79F	0x7421
0xF840	0x7521		0xF89F	0x7621
0xF940	0x7721		0xF99F	0x7821
0xFA40	0x7921		0xFA9F	0x7A21
0xFB40	0x7B21		0xFB9F	0x7C21
0xFC40	0x7D21		0xFC9F	0x7E21
*******************/

DWORD CIConv::JisToSJis(DWORD cd)
{
	DWORD hi,lo;

	hi = (cd >> 8) & 0xFF;
	lo = cd & 0xFF;

	if ( (hi & 1) != 0 )
		lo += (lo <= 0x5F ? 0x1F : 0x20);
	else
		lo += 0x7E;

	if ( (cd & 0xFF0000) == 0 ) {
		if ( hi <= 0x5E )
			hi = (hi + 0xE1) / 2;
		else
			hi = (hi + 0x161) / 2;
	} else {
		if ( hi >= 0x6E )
			hi = (hi + 0x17B) / 2;
		else if ( hi == 0x21 || (hi >= 0x23 && hi <= 0x25) )
			hi = (hi + 0x1BF) / 2;
		else if ( hi == 0x28 || (hi >= 0x2C && hi <= 0x2F) )
			hi = (hi + 0x1B9) / 2;
		else
			return 0x8140;
	}

	return (hi << 8 | lo);
}

DWORD CIConv::SJisToJis(DWORD cd)
{
	DWORD hi,lo;

	hi = (cd >> 8) & 0xFF;
	lo = cd & 0xFF;

	if ( hi <= 0x9F )
		hi = (hi - 0x71) * 2 + 1;
	else if ( hi <= 0xEF )
		hi = (hi - 0xB1) * 2 + 1;
	else
		hi = (hi - 0x60) * 2 + 1;

	if ( lo >= 0x9F ) {
		lo -= 0x7E;
		hi++;
	} else
		lo -= (lo >= 0x80 ? 0x20 : 0x1F);

	if ( hi >= 0x12A )
		hi += 0x44;
	else if ( hi >= 0x126 || hi == 0x122 )
		hi += 0x06;

	return (hi << 8 | lo);
}

typedef struct _DECCHARTAB {
	LPCTSTR	name;
	int sc;
	const WORD *table;
} DECCHARTAB;

static int IConvNameCmp(const void *src, const void *dis)
{
	return _tcscmp((LPCTSTR)src, ((DECCHARTAB *)dis)->name);
}
class CIConv *CIConv::GetIConv(LPCTSTR from, LPCTSTR to)
{
	int n;
	int sc = 0x20, ec = 0x80;
	const WORD *tab;
	static const DECCHARTAB DecCharTab[] = {
		{	_T("DEC_GREEK"),			0x60,	DecGreek		},
		{	_T("DEC_GREEK_SUPP"),		0x20,	DecGreekSupp	},
		{	_T("DEC_HEBREW"),			0x60,	DecHebrew		},
		{	_T("DEC_HEBREW_SUPP"),		0x20,	DecHebrewSupp	},
		{	_T("DEC_RUSSIAN"),			0x60,	DecRussian		},
		{	_T("DEC_SGCS-GR"),			0x60,	DecSGCS			},
		{	_T("DEC_TCS-GR"),			0x20,	DecTCS			},
		{	_T("DEC_TURKISH_SUPP"),		0x20,	DecTurkishSupp	},
	};

	if ( m_Cd == NULL && m_Mode == 0 ) {
		m_From = from;
		m_To   = to;
		m_Mode = 0;

		if ( _tcscmp(from, _T("EUC-JISX0213")) == 0 )
			m_Mode |= 0x01;
		else if ( _tcscmp(from, _T("SHIFT_JISX0213")) == 0 )
			m_Mode |= 0x02;
		else if ( _tcscmp(from, _T("JIS_X0213-2000.1")) == 0 )
			m_Mode |= 0x03;
		else if ( _tcscmp(from, _T("JIS_X0213-2000.2")) == 0 )
			m_Mode |= 0x04;
		else if ( _tcsncmp(from, _T("DEC_"), 4) == 0 && BinaryFind((void *)from, (void *)DecCharTab, sizeof(DECCHARTAB), sizeof(DecCharTab) / sizeof(DECCHARTAB), IConvNameCmp, &n) ) {
			m_DecTab = DecCharTab[n].table;
			m_Mode |= (DecCharTab[n].sc == 0x20 ? 0x05 : 0x06);
		} else if ( _tcscmp(from, _T("437")) == 0 || _tcscmp(from, _T("CP437")) == 0 || _tcscmp(from, _T("IBM437")) == 0 || _tcscmp(from, _T("CSPC8CODEPAGE437")) == 0 )
			m_Mode |= 0x07;
		else if ( (m_CodePage = CIso646Dlg::CodePage(from)) >= 0 )
			m_Mode |= 0x08;

		if ( _tcscmp(to, _T("EUC-JISX0213")) == 0 )
			m_Mode |= 0x0100;
		else if ( _tcscmp(to, _T("SHIFT_JISX0213")) == 0 )
			m_Mode |= 0x0200;
		else if ( _tcscmp(to, _T("JIS_X0213-2000.1")) == 0 )
			m_Mode |= 0x0300;
		else if ( _tcscmp(to, _T("JIS_X0213-2000.2")) == 0 )
			m_Mode |= 0x0400;
		else if ( _tcsncmp(to, _T("DEC_"), 4) == 0 && BinaryFind((void *)to, (void *)DecCharTab, sizeof(DECCHARTAB), sizeof(DecCharTab) / sizeof(DECCHARTAB), IConvNameCmp, &n) ) {
			sc = DecCharTab[n].sc;
			tab = DecCharTab[n].table;
			m_Mode |= 0x0500;
		} else if ( (n = CIso646Dlg::CodePage(to)) >= 0 ) {
			m_CodeIndex.RemoveAll();
			CIso646Dlg::CodeIndex(n, m_CodeIndex);
			m_Mode |= 0x0600;
		}

		if ( (m_Mode & 0xFF) >= 0x05 )
			from = _T("UCS-4BE");
		else if ( (m_Mode & 0xFF) >= 0x03 )
			from = _T("EUC-JISX0213");

		if ( (m_Mode & 0xFF00) == 0x0300 || (m_Mode & 0xFF00) == 0x0400 )
			to = _T("EUC-JISX0213");
		else if ( (m_Mode & 0xFF00) == 0x0500 ) {
			m_CodeIndex.RemoveAll();
			for ( int n = sc ; n < ec ; n++ )
				m_CodeIndex[(DWORD)tab[n - sc]] = (DWORD)n;
			to = _T("UCS-4BE");
		}

	    m_Cd = iconv_open(TstrToMbs(to), TstrToMbs(from));

		if ( m_Cd == (iconv_t)(-1) ) {
			CString msg;
			msg.Format(_T("iconv not supported '%s' to '%s'"), from, to); 
			CWinApp::ShowAppMessageBox(NULL, msg, MB_ICONERROR, 0);
		}

		return this;
	}

	if ( (n = m_From.Compare(from)) == 0 && (n = m_To.Compare(to)) == 0 )
		return this;
	else if ( n < 0 ) {
		if ( m_Left == NULL )
			m_Left = new CIConv;
		return m_Left->GetIConv(from, to);
	} else {
		if ( m_Right == NULL )
			m_Right = new CIConv;
		return m_Right->GetIConv(from, to);
	}
}

void CIConv::IConvSub(LPCTSTR from, LPCTSTR to, CBuffer *in, CBuffer *out)
{
	int res = 0;
	CIConv *pIconv;
    char *pInStr, *pOutStr;
    size_t nInLen, nInTry, nInPos;
	size_t nOuPos, nOutMax;
	char *pOutBuf;

	if ( (pIconv = GetIConv(from, to)) == NULL || pIconv->m_Cd == (iconv_t)(-1) )
		return;

	pInStr = (char *)(in->GetPtr());
	nInLen = in->GetSize();
	in->Consume((int)nInLen);

	nOutMax = nInLen * 4;
	pOutBuf = new char[nOutMax];

    while ( nInLen > 0 && res != (-1) ) {
		nInPos = nInTry = nInLen;

		pOutStr = pOutBuf;
		nOuPos = nOutMax;
		res = (int)iconv(pIconv->m_Cd, &pInStr, &nInPos, &pOutStr, &nOuPos);

		if ( nOuPos < nOutMax )
			out->Apend((LPBYTE)pOutBuf, (int)(nOutMax - nOuPos));

		nInLen -= (nInTry - nInPos);

		if ( res == (-1) ) {
			if ( errno == E2BIG ) {
				// E2BIG	outbuf に十分な空きがない。 

				delete [] pOutBuf;
				nOutMax *= 2;
				pOutBuf = new char[nOutMax];
				res = 0;

			} else if ( errno == EILSEQ || errno == EINVAL ) {	
				// EILSEQ	入力に無効なマルチバイト文字列があった。
				// EINVAL	入力に不完全なマルチバイト文字列があった。

				m_ErrCount++;
				if ( nInLen > 0 ) {
					pInStr++;
					nInLen--;
					res = 0;
				}
			}
		}
	}

	delete [] pOutBuf;
}
BOOL CIConv::IConvBuf(LPCTSTR from, LPCTSTR to, CBuffer *in, CBuffer *out)
{
	int res = 0;
	CIConv *pIconv;
    char *pInStr, *pOutStr;
    size_t nInLen, nInTry, nInPos;
	size_t nOuPos, nOutMax;
	char *pOutBuf;

	if ( (pIconv = GetIConv(from, to)) == NULL || pIconv->m_Cd == (iconv_t)(-1) )
		return FALSE;

	pInStr = (char *)(in->GetPtr());
	nInLen = in->GetSize();

	nOutMax = nInLen * 4;
	pOutBuf = new char[nOutMax];

    while ( nInLen > 0 && res != (-1) ) {
		nInPos = nInTry = nInLen;

		pOutStr = pOutBuf;
		nOuPos = nOutMax;
		res = (int)iconv(pIconv->m_Cd, &pInStr, &nInPos, &pOutStr, &nOuPos);

		if ( nOuPos < nOutMax )
			out->Apend((LPBYTE)pOutBuf, (int)(nOutMax - nOuPos));

		nInLen -= (nInTry - nInPos);
		in->Consume((int)(nInTry - nInPos));

		if ( res == (-1) && errno == E2BIG ) {
			delete [] pOutBuf;
			nOutMax *= 2;
			pOutBuf = new char[nOutMax];
			res = 0;
		}
	}

	delete [] pOutBuf;

	return (res == (-1) ? FALSE : TRUE);
}
BOOL CIConv::StrToRemote(LPCTSTR to, CBuffer *in, CBuffer *out)
{
	CBuffer bIn;
	BOOL rt = TRUE;

	bIn.Apend(in->GetPtr(), in->GetSize());
	CTextRam::MsToIconvUniStr(to, (LPWSTR)bIn.GetPtr(), bIn.GetSize() / sizeof(WCHAR));

	while ( (bIn.GetSize() / sizeof(WCHAR)) > 0 ) {
		if ( !IConvBuf(_T("UTF-16LE"), to, &bIn, out) ) {
			if ( (bIn.GetSize() / sizeof(WCHAR)) <= 0 )
				break;
			// iconv error recovery
			bIn.Consume(sizeof(WCHAR));
			out->PutByte('?');
			rt = FALSE;
		}
	}

	return rt;
}
BOOL CIConv::StrToRemote(LPCTSTR to, LPCTSTR in, CStringA &out)
{
#ifdef	_UNICODE
	CBuffer bIn((LPBYTE)in, (int)_tcslen(in) * sizeof(TCHAR));
#else
	CStringW str(in);
	CBuffer bIn((LPBYTE)(LPCWSTR)str, str.GetLength() * sizeof(WCHAR));
#endif
	CBuffer bOut;
	BOOL rt = StrToRemote(to, &bIn, &bOut);
	out = (LPCSTR)bOut;

	return rt;
}
void CIConv::RemoteToStr(LPCTSTR from, CBuffer *in, CBuffer *out)
{
	CBuffer mid;

	if ( IConvBuf(from, _T("UTF-16LE"), in, &mid) ) {
		CTextRam::UnicodeNomalStr((LPWSTR)mid.GetPtr(), mid.GetSize() / sizeof(WCHAR), *out);
		CTextRam::IconvToMsUniStr(from, (LPWSTR)out->GetPtr(), out->GetSize() / sizeof(WCHAR));
		return;
	}

	// iconv error recovery
	LPCSTR p = (LPCSTR)in->GetPtr();
	int len = in->GetSize() / sizeof(CHAR);

	out->Clear();
	for ( int n = 0 ; n < len ; n++, p++ )
		out->PutWord((*p & 0x80) == 0 ? (WCHAR)*p : L'?');
}
void CIConv::RemoteToStr(LPCTSTR from, LPCSTR in, CString &out)
{
	CBuffer bIn((LPBYTE)in, (int)strlen(in));
	CBuffer bOut;

	RemoteToStr(from, &bIn, &bOut);
	out = (LPCWSTR)bOut;
}

DWORD CIConv::IConvChar(LPCTSTR from, LPCTSTR to, DWORD ch)
{
	int n = 0;
	DWORD od = ch;
	CIConv *cp;
    char *inp, *otp;
    size_t ins, ots;
    unsigned char itmp[32], otmp[32];

	cp = GetIConv(from, to);
	if ( cp->m_Cd == (iconv_t)(-1) )
		return ch;

	if ( (ch & 0xFFFFFF00) == 0 && cp->m_Table[ch] != 0 )
		return cp->m_Table[ch];

	switch(cp->m_Mode) {
	case 0x0101:		// EUC-JISX0213 > EUC-JISX0213
		goto ENDOF;
	case 0x0201:		// EUC-JISX0213 > SHIFT_JISX0213
		ch = JisToSJis(ch & 0x17F7F);
		goto ENDOF;
	case 0x0301:		// EUC-JISX0213 > JISX0213.1
		if ( ch >= 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;
	case 0x0401:		// EUC-JISX0213 > JISX0213.2
		if ( ch < 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;

	case 0x0102:		// SHIFT_JISX0213 > EUC-JISX0213
		ch = SJisToJis(ch);
		if ( ch >= 0x10000 )
			ch |= 0x8F0000;
		ch |= 0x8080;
		goto ENDOF;
	case 0x0202:		// SHIFT_JISX0213 > SHIFT_JISX0213
		goto ENDOF;
	case 0x0302:		// SHIFT_JISX0213 > JISX0213.1
		ch = SJisToJis(ch);
		if ( ch >= 0x10000 )
			ch = 0x0000;
		goto ENDOF;
	case 0x0402:		// SHIFT_JISX0213 > JISX0213.2
		ch = SJisToJis(ch);
		if ( ch < 0x10000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;

	case 0x0003:		// JISX0213.1 > ???
		ch |= 0x8080;
		break;
	case 0x0103:		// JISX0213.1 > EUC-JISX0213
		ch |= 0x8080;
		goto ENDOF;
	case 0x0203:		// JISX0213.1 > SHIFT_JISX0213
		ch = JisToSJis(ch);
		goto ENDOF;
	case 0x0303:		// JISX0213.1 > JISX0213.1
		goto ENDOF;
	case 0x0403:		// JISX0213.1 > JISX0213.2
		ch = 0x0000;
		goto ENDOF;

	case 0x0004:		// JISX0213.2 > ???
		ch |= 0x8F8080;
		break;
	case 0x0104:		// JISX0213.2 > EUC-JISX0213
		ch |= 0x8F8080;
		goto ENDOF;
	case 0x0204:		// JISX0213.2 > SHIFT_JISX0213
		ch = JisToSJis(ch | 0x10000);
		goto ENDOF;
	case 0x0304:		// JISX0213.2 > JISX0213.1
		ch = 0x0000;
		goto ENDOF;
	case 0x0404:		// JISX0213.2 > JISX0213.2
		goto ENDOF;

	case 0x0005:		// DEC_xxx
		ch &= 0x7F;
		if ( ch >= 0x0020 && ch <= 0x007F )
			ch = cp->m_DecTab[ch - 0x20];
		from = _T("UCS-4BE");
		break;
	case 0x0006:		// DEC_xxx
		ch &= 0x7F;
		if ( ch >= 0x0060 && ch <= 0x007F )
			ch = cp->m_DecTab[ch - 0x60];
		from = _T("UCS-4BE");
		break;
	case 0x0007:		// IBM437
		if ( ch >= 0x0000 && ch <= 0x00FF )
			ch = IBM437[ch];
		from = _T("UCS-4BE");
		break;
	case 0x0008:		// CP0....	NRCS
		ch = CIso646Dlg::TableMaping(cp->m_CodePage, ch);
		from = _T("UCS-4BE");
		break;
	}
	
	if ( _tcscmp(from, _T("UCS-4BE")) == 0 ) {
		itmp[n++] = (unsigned char)(ch >> 24);
		itmp[n++] = (unsigned char)(ch >> 16);
		itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	} else if ( _tcscmp(from, _T("UTF-16BE")) == 0 ) {
		if ( (ch & 0xFFFF0000) != 0 ) {
			itmp[n++] = (unsigned char)(ch >> 24);
			itmp[n++] = (unsigned char)(ch >> 16);
		}
		itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	} else {
		if ( (ch & 0xFF000000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 24);
		if ( (ch & 0xFFFF0000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 16);
		if ( (ch & 0xFFFFFF00) != 0 )
			itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	}

    inp = (char *)itmp;
    ins = n;
    otp = (char *)otmp;
    ots = 32;

    while ( ins > 0 ) {
        if ( iconv(cp->m_Cd, (char **)&inp, &ins, &otp, &ots) == (size_t)(-1) )
	        return 0x0000;
    }
	iconv(cp->m_Cd, NULL, NULL, &otp, &ots);

	ch = 0;
	otp = (char *)otmp;
	for ( n = (int)(32 - ots) ; n > 0 ;  n-- )
		ch = (ch << 8) | (unsigned char)(*(otp++));

	switch(cp->m_Mode & 0xFF00) {
	case 0x0300:		// JISX0213.1
		if ( ch >= 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		break;
	case 0x0400:	// JISX0213.2
		if ( ch < 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		break;
	case 0x0500:	// DEC....
	case 0x0600:	// CP0....
		if ( ch >= 0x20 && ch <= 0x7F && (ch = cp->m_CodeIndex.Find(ch)) == 0 )
			ch = L'?';
		break;
	}

ENDOF:
	if ( (od & 0xFFFFFF00) == 0 )
		cp->m_Table[od] = ch;
	return ch;
}

static int GetCharSet(unsigned int namescount, const char * const * names, void* data)
{
	unsigned int i;
	CStringArray *pArray = (CStringArray *)data;

	for ( i = 0 ; i < namescount ; i++ )
		pArray->Add(MbsToTstr(names[i]));
	return 0;
}
void CIConv::SetListArray(CStringArray &stra)
{
	stra.RemoveAll();
	iconvlist(GetCharSet, &stra);
	stra.Add(_T("JIS_X0213-2000.1"));
	stra.Add(_T("JIS_X0213-2000.2"));
	stra.Add(_T("DEC_TCS-GR"));
	stra.Add(_T("DEC_SGCS-GR"));
	stra.Add(_T("DEC_RUSSIAN"));
	stra.Add(_T("DEC_HEBREW"));
	stra.Add(_T("DEC_GREEK"));
	stra.Add(_T("DEC_TURKISH_SUPP"));
	stra.Add(_T("DEC_HEBREW_SUPP"));
	stra.Add(_T("DEC_GREEK_SUPP"));
}

typedef struct _IconvCheckList {
	BOOL bFind;
	LPCSTR name;
} IconvCheckList;

static int ChkList(unsigned int namescount, const char * const * names, void* data)
{
	unsigned int i;
	IconvCheckList *res = (IconvCheckList *)data;

	for ( i = 0 ; i < namescount ; i++ ) {
		if ( strcmp(res->name, names[i]) == 0 ) {
			res->bFind = TRUE;
			return 1;
		}
	}

	return 0;
}
BOOL CIConv::IsIconvList(LPCSTR name)
{
	IconvCheckList res = { FALSE, name };
	iconvlist(ChkList, &res);
	return res.bFind;
}

static int CodePageCmp(const void *src, const void *dis)
{
	int *pInt = (int *)src;
	struct _CodePageList *pList = (struct _CodePageList *)dis;

	return (*pInt - pList->codepage);
}
LPCTSTR CIConv::GetCodePageName(int codepage)
{
	int n;

#if 0
	CStringArray stra;
	CIConv::SetListArray(stra);
	TRACE("} CodePageList[] = {\n");
	for ( int n = 0 ; n < stra.GetSize() ; n++ ) {
		if ( stra[n][0] == _T('C') && stra[n][1] == _T('P') )
			TRACE("\t{ %d,\t\t_T(\"%s\") },\n", _tstoi((LPCTSTR)stra[n] + 2), TstrToMbs(stra[n]));
	}
	TRACE("};\n");
#endif

	if ( BinaryFind(&codepage, (void *)CodePageList, sizeof(struct _CodePageList), sizeof(CodePageList) / sizeof(struct _CodePageList), CodePageCmp, &n) )
		return CodePageList[n].name;

	return NULL;
}
