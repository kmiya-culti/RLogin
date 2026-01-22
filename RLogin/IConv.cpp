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

typedef struct _DECCHARTAB {
	LPCTSTR	name;
	int sc;
	const WORD *table;
} DECCHARTAB;

static const DECCHARTAB DecCharTab[] = {
	{	_T("DEC_GRAPHIC_SUPP"),		0x20,	DecGraphicSupp	},
	{	_T("DEC_GREEK"),			0x60,	DecGreek		},
	{	_T("DEC_GREEK_SUPP"),		0x20,	DecGreekSupp	},
	{	_T("DEC_HEBREW"),			0x60,	DecHebrew		},
	{	_T("DEC_HEBREW_SUPP"),		0x20,	DecHebrewSupp	},
	{	_T("DEC_RUSSIAN"),			0x60,	DecRussian		},
	{	_T("DEC_SGCS-GR"),			0x60,	DecSGCS			},
	{	_T("DEC_TCS-GR"),			0x20,	DecTCS			},
	{	_T("DEC_TURKISH_SUPP"),		0x20,	DecTurkishSupp	},
};

static const LPCTSTR CodePageList[] = {
	_T("CP37 EBCDIC US-Canada"),
	_T("CP437 United States"),
	_T("CP500 EBCDIC International"),
	_T("CP708 Arabic (ASMO 708)"),
	_T("CP709 Arabic (ASMO-449+, BCON V4)"),
	_T("CP710 Arabic - Transparent Arabic"),
	_T("CP720 Arabic (DOS)"),
	_T("CP737 Greek (DOS)"),
	_T("CP775 Baltic (DOS)"),
	_T("CP850 Western European (DOS)"),
	_T("CP852 Central European (DOS)"),
	_T("CP855 Cyrillic (primarily Russian)"),
	_T("CP857 Turkish (DOS)"),
	_T("CP858 Latin 1 + Euro symbol"),
	_T("CP860 Portuguese (DOS)"),
	_T("CP861 Icelandic (DOS)"),
	_T("CP862 Hebrew (DOS)"),
	_T("CP863 French Canadian (DOS)"),
	_T("CP864 Arabic (864)"),
	_T("CP865 Nordic (DOS)"),
	_T("CP866 Cyrillic (DOS)"),
	_T("CP869 Greek, Modern (DOS)"),
	_T("CP870 EBCDIC Multilingual Latin 2"),
	_T("CP874 Thai (Windows)"),
	_T("CP875 EBCDIC Greek Modern"),
	_T("CP932 Japanese (Shift-JIS)"),
	_T("CP936 Chinese Simplified (GB2312)"),
	_T("CP949 Korean (Unified Hangul Code)"),
	_T("CP950 Chinese Traditional (Big5)"),
	_T("CP1026 EBCDIC Turkish (Latin 5)"),
	_T("CP1047 EBCDIC Latin 1/Open System"),
	_T("CP1140 EBCDIC (US-Canada-Euro)"),
	_T("CP1141 EBCDIC (Germany-Euro)"),
	_T("CP1142 EBCDIC (Denmark-Norway-Euro)"),
	_T("CP1143 EBCDIC (Finland-Sweden-Euro)"),
	_T("CP1144 EBCDIC (Italy-Euro)"),
	_T("CP1145 EBCDIC (Spain-Euro)"),
	_T("CP1146 EBCDIC (UK-Euro)"),
	_T("CP1147 EBCDIC (France-Euro)"),
	_T("CP1148 EBCDIC (International-Euro)"),
	_T("CP1149 EBCDIC (Icelandic-Euro)"),
	_T("CP1200 UTF-16LE"),
	_T("CP1201 UTF-16BE"),
	_T("CP1250 Central European (Windows)"),
	_T("CP1251 Cyrillic (Windows)"),
	_T("CP1252 Western European (Windows)"),
	_T("CP1253 Greek (Windows)"),
	_T("CP1254 Turkish (Windows)"),
	_T("CP1255 Hebrew (Windows)"),
	_T("CP1256 Arabic (Windows)"),
	_T("CP1257 Baltic (Windows)"),
	_T("CP1258 Vietnamese (Windows)"),
	_T("CP1361 Korean (Johab)"),
	_T("CP10000 Western European (Mac)"),
	_T("CP10001 Japanese (Mac)"),
	_T("CP10002 Chinese Traditional (Mac)"),
	_T("CP10003 Korean (Mac)"),
	_T("CP10004 Arabic (Mac)"),
	_T("CP10005 Hebrew (Mac)"),
	_T("CP10006 Greek (Mac)"),
	_T("CP10007 Cyrillic (Mac)"),
	_T("CP10008 Chinese Simplified (Mac)"),
	_T("CP10010 Romanian (Mac)"),
	_T("CP10017 Ukrainian (Mac)"),
	_T("CP10021 Thai (Mac)"),
	_T("CP10029 Central European (Mac)"),
	_T("CP10079 Icelandic (Mac)"),
	_T("CP10081 Turkish (Mac)"),
	_T("CP10082 Croatian (Mac)"),
	_T("CP12000 UTF-32LE"),
	_T("CP12001 UTF-32BE"),
	_T("CP20000 Chinese Traditional (CNS)"),
	_T("CP20001 TCA Taiwan"),
	_T("CP20002 Chinese Traditional (Eten)"),
	_T("CP20003 IBM5550 Taiwan"),
	_T("CP20004 TeleText Taiwan"),
	_T("CP20005 Wang Taiwan"),
	_T("CP20105 Western European (IA5)"),
	_T("CP20106 IA5 German (7-bit)"),
	_T("CP20107 IA5 Swedish (7-bit)"),
	_T("CP20108 IA5 Norwegian (7-bit)"),
	_T("CP20127 US-ASCII (7-bit)"),
	_T("CP20261 T.61"),
	_T("CP20269 ISO 6937 Non-Spacing Accent"),
	_T("CP20273 EBCDIC Germany"),
	_T("CP20277 EBCDIC Denmark-Norway"),
	_T("CP20278 EBCDIC Finland-Sweden"),
	_T("CP20280 EBCDIC Italy"),
	_T("CP20284 EBCDIC Latin America-Spain"),
	_T("CP20285 EBCDIC United Kingdom"),
	_T("CP20290 EBCDIC JPN Katakana Extended"),
	_T("CP20297 EBCDIC France"),
	_T("CP20420 EBCDIC Arabic"),
	_T("CP20423 EBCDIC Greek"),
	_T("CP20424 EBCDIC Hebrew"),
	_T("CP20833 EBCDIC Korean Extended"),
	_T("CP20838 EBCDIC Thai"),
	_T("CP20866 Cyrillic (KOI8-R)"),
	_T("CP20871 EBCDIC Icelandic"),
	_T("CP20880 EBCDIC Cyrillic Russian"),
	_T("CP20905 EBCDIC Turkish"),
	_T("CP20924 EBCDIC Latin 1/Open System"),
	_T("CP20932 JPN (JIS 0208-1990/0212-1990)"),
	_T("CP20936 Chinese Simplified (GB2312-80)"),
	_T("CP20949 Korean Wansung"),
	_T("CP21025 EBCDIC Cyrillic Serbian-Bulgarian"),
	_T("CP21027 (deprecated)"),
	_T("CP21866 Cyrillic (KOI8-U)"),
	_T("CP28591 ISO 8859-1 Latin 1"),
	_T("CP28592 ISO 8859-2 Central European"),
	_T("CP28593 ISO 8859-3 Latin 3"),
	_T("CP28594 ISO 8859-4 Baltic"),
	_T("CP28595 ISO 8859-5 Cyrillic"),
	_T("CP28596 ISO 8859-6 Arabic"),
	_T("CP28597 ISO 8859-7 Greek"),
	_T("CP28598 ISO 8859-8 Hebrew"),
	_T("CP28599 ISO 8859-9 Turkish"),
	_T("CP28603 ISO 8859-13 Estonian"),
	_T("CP28605 ISO 8859-15 Latin 9"),
	_T("CP29001 Europa 3"),
	_T("CP38598 ISO 8859-8 Hebrew (ISO-Logical)"),
	_T("CP50220 ISO 2022 JPN with No-HW Katakana"),
	_T("CP50221 ISO 2022 JPN with HW Katakana"),
	_T("CP50222 ISO 2022 JPN JIS X 0201-1989"),
	_T("CP50225 ISO 2022 Korean"),
	_T("CP50227 ISO 2022 Simplified Chinese"),
	_T("CP50229 ISO 2022 Traditional Chinese"),
	_T("CP50930 EBCDIC JPN Katakana Extended"),
	_T("CP50931 EBCDIC US-Canada and Japanese"),
	_T("CP50933 EBCDIC Korean Extended and Korean"),
	_T("CP50935 EBCDIC Simplified Chinese Extended"),
	_T("CP50936 EBCDIC Simplified Chinese"),
	_T("CP50937 EBCDIC US-Canada and Tr-Chinese"),
	_T("CP50939 EBCDIC Japanese Latin Extended"),
	_T("CP51932 EUC-JP Japanese"),
	_T("CP51936 EUC-CN Simplified Chinese"),
	_T("CP51949 EUC-KR Korean"),
	_T("CP51950 EUC-CN Traditional Chinese"),
	_T("CP52936 HZ-GB2312 Simplified Chinese"),
	_T("CP54936 GB18030 Simplified Chinese"),
	_T("CP57002 ISCII Devanagari"),
	_T("CP57003 ISCII Bangla"),
	_T("CP57004 ISCII Tamil"),
	_T("CP57005 ISCII Telugu"),
	_T("CP57006 ISCII Assamese"),
	_T("CP57007 ISCII Odia"),
	_T("CP57008 ISCII Kannada"),
	_T("CP57009 ISCII Malayalam"),
	_T("CP57010 ISCII Gujarati"),
	_T("CP57011 ISCII Punjabi"),
	_T("CP65000 UTF-7"),
	_T("CP65001 UTF-8"),
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
	m_FromCodePage = 0;
	m_ToCodePage = 0;
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

static int IConvNameCmp(const void *src, const void *dis)
{
	return _tcscmp((LPCTSTR)src, ((DECCHARTAB *)dis)->name);
}
class CIConv *CIConv::GetIConv(LPCTSTR from, LPCTSTR to)
{
	int n;
	int sc = 0x20, ec = 0x80;
	const WORD *tab;

	if ( m_Cd == NULL && m_Mode == 0 ) {
		m_From = from;
		m_To   = to;
		m_Mode = 0;

		/*if ( _tcscmp(from, _T("EUC-JISX0213")) == 0 )
			m_Mode |= 0x01;
		else if ( _tcscmp(from, _T("SHIFT_JISX0213")) == 0 )
			m_Mode |= 0x02;
		else */if ( _tcscmp(from, _T("JIS_X0213-2000.1")) == 0 )
			m_Mode |= 0x03;
		else if ( _tcscmp(from, _T("JIS_X0213-2000.2")) == 0 )
			m_Mode |= 0x04;
		else if ( _tcsncmp(from, _T("DEC_"), 4) == 0 && BinaryFind((void *)from, (void *)DecCharTab, sizeof(DECCHARTAB), sizeof(DecCharTab) / sizeof(DECCHARTAB), IConvNameCmp, &n) ) {
			m_DecTab = DecCharTab[n].table;
			m_Mode |= (DecCharTab[n].sc == 0x20 ? 0x05 : 0x06);
		} else if ( _tcscmp(from, _T("437")) == 0 || _tcscmp(from, _T("CP437")) == 0 || _tcscmp(from, _T("IBM437")) == 0 || _tcscmp(from, _T("CSPC8CODEPAGE437")) == 0 )
			m_Mode |= 0x07;
		else if ( (m_FromCodePage = CIso646Dlg::CodePage(from)) >= 0 )
			m_Mode |= 0x08;
		else if ( _tcsncmp(from, _T("CP"), 2) == 0 && (m_FromCodePage = _tstoi(from + 2)) > 0 && IHaveCodePage(m_FromCodePage) )
			m_Mode |= 0x09;
		else if ( _tcsncmp(from, _T("ICONV-"), 6) == 0 )
			from += 6;

		/*if ( _tcscmp(to, _T("EUC-JISX0213")) == 0 )
			m_Mode |= 0x0100;
		else if ( _tcscmp(to, _T("SHIFT_JISX0213")) == 0 )
			m_Mode |= 0x0200;
		else */if ( _tcscmp(to, _T("JIS_X0213-2000.1")) == 0 )
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
		} else if ( _tcsncmp(to, _T("CP"), 2) == 0 && (m_ToCodePage = _tstoi(to + 2)) > 0 && IHaveCodePage(m_ToCodePage) )
			m_Mode |= 0x0700;
		else if ( _tcsncmp(to, _T("ICONV-"), 6) == 0 )
			to += 6;

		if ( (m_Mode & 0xFF) == 0x09 )
			from = _T("UCS-2LE");
		else if ( (m_Mode & 0xFF) >= 0x05 )
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
		} else if ( (m_Mode & 0xFF00) == 0x0600 )
			to = _T("UCS-4BE");
		else if ( (m_Mode & 0xFF00) == 0x0700 )
			to = _T("UCS-2LE");

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

CBuffer *CIConv::ExtFromConv(CBuffer *in)
{
	int len = in->GetSize();
	BYTE *p = in->GetPtr();

	switch(m_Mode & 0x00FF) {
	case 0x03:	// JIS_X0213-2000.1			EUC-JISX0213
		m_SaveBuf.Clear();
		for ( ; len > 0 ; len-- ) {
			if ( len > 1 && p[0] >= 0x21 && p[0] <= 0x7E && p[1] >= 0x21 && p[1] <= 0x7E ) {
				m_SaveBuf.PutByte(p[0] | 0x80);
				m_SaveBuf.PutByte(p[1] | 0x80);
				p += 2;
			} else if ( p[0] >= 0x80 ) {
				m_SaveBuf.PutByte(0x8E);
				m_SaveBuf.PutByte(*(p++));
			} else {
				m_SaveBuf.PutByte(*(p++));
			}
		}
		in = &m_SaveBuf;
		break;
	case 0x04:	// JIS_X0213-2000.2			EUC-JISX0213
		m_SaveBuf.Clear();
		for ( ; len > 0 ; len-- ) {
			if ( len > 1 && p[0] >= 0x21 && p[0] <= 0x7E && p[1] >= 0x21 && p[1] <= 0x7E ) {
				m_SaveBuf.PutByte(0x8F);
				m_SaveBuf.PutByte(p[0] | 0x80);
				m_SaveBuf.PutByte(p[1] | 0x80);
				p += 2;
			} else if ( p[0] >= 0x80 ) {
				m_SaveBuf.PutByte(0x8E);
				m_SaveBuf.PutByte(*(p++));
			} else {
				m_SaveBuf.PutByte(*(p++));
			}
		}
		in = &m_SaveBuf;
		break;
	case 0x05:	// DEC_						UCS-4BE
		m_SaveBuf.Clear();
		for ( ; len > 0 ; len-- ) {
			if ( *p >= 0x0020 && *p <= 0x007F )
				m_SaveBuf.Put32Bit(m_DecTab[*p - 0x20]);
			else
				m_SaveBuf.Put32Bit(*p);
			p++;
		}
		in = &m_SaveBuf;
		break;
	case 0x06:	// DEC_						UCS-4BE
		m_SaveBuf.Clear();
		for ( ; len > 0 ; len-- ) {
			if ( *p >= 0x0060 && *p <= 0x007F )
				m_SaveBuf.Put32Bit(m_DecTab[*p - 0x60]);
			else
				m_SaveBuf.Put32Bit(*p);
			p++;
		}
		in = &m_SaveBuf;
		break;
	case 0x07:	// IBM437					UCS-4BE
		m_SaveBuf.Clear();
		for ( ; len > 0 ; len-- ) {
			if ( *p >= 0x0000 && *p <= 0x00FF )
				m_SaveBuf.Put32Bit(IBM437[*p]);
			else
				m_SaveBuf.Put32Bit(*p);
			p++;
		}
		in = &m_SaveBuf;
		break;
	case 0x08:	// CP0						UCS-4BE
		m_SaveBuf.Clear();
		for ( ; len > 0 ; len-- ) {
			m_SaveBuf.Put32Bit(CIso646Dlg::TableMaping(m_FromCodePage, *p));
			p++;
		}
		in = &m_SaveBuf;
		break;
	case 0x09:	// CP						UCS-2LE
		m_SaveBuf.Clear();
		len = ::MultiByteToWideChar(m_FromCodePage, 0, (LPCSTR)p, in->GetSize(), NULL, 0);
		len = ::MultiByteToWideChar(m_FromCodePage, 0, (LPCSTR)p, in->GetSize(), (LPWSTR)m_SaveBuf.PutSpc(len * sizeof(WCHAR)), len);
		in = &m_SaveBuf;
		break;
	}

	return in;
}
void CIConv::ExtToConv(CBuffer *out)
{
	switch(m_Mode & 0xFF00) {
	case 0x0300:	// JIS_X0213-2000.1			EUC-JISX0213
		{
			int len = out->GetSize();
			BYTE *p = out->GetPtr();
			m_SaveBuf.Clear();
			for ( ; len > 0 ; len-- ) {
				if ( len > 3 && p[0] == 0x8F && p[1] >= 0xA1 && p[2] >= 0xA1 ) {
					m_SaveBuf.PutByte('?');
					p += 3;
				} else if ( len > 2 && p[0] == 0x8E && p[1] >= 0xA1 ) {
					m_SaveBuf.PutByte(p[1]);
					p += 2;
				} else if ( len > 2 && p[0] >= 0xA1 && p[1] >= 0xA1 ) {
					m_SaveBuf.PutByte(p[0] & 0x7F);
					m_SaveBuf.PutByte(p[1] & 0x7F);
					p += 2;
				} else
					m_SaveBuf.PutByte(*(p++));
			}
			out->Swap(m_SaveBuf);
		}
		break;
	case 0x0400:	// JIS_X0213-2000.2			EUC-JISX0213
		{
			int len = out->GetSize();
			BYTE *p = out->GetPtr();
			m_SaveBuf.Clear();
			for ( ; len > 0 ; len-- ) {
				if ( len > 3 && p[0] == 0x8F && p[1] >= 0xA1 && p[2] >= 0xA1 ) {
					m_SaveBuf.PutByte(p[1] & 0x7F);
					m_SaveBuf.PutByte(p[2] & 0x7F);
					p += 3;
				} else if ( len > 2 && p[0] == 0x8E && p[1] >= 0xA1 ) {
					m_SaveBuf.PutByte(p[1]);
					p += 2;
				} else if ( len > 2 && p[0] >= 0xA1 && p[1] >= 0xA1 ) {
					m_SaveBuf.PutByte('?');
					p += 2;
				} else
					m_SaveBuf.PutByte(*(p++));
			}
			out->Swap(m_SaveBuf);
		}
		break;
	case 0x0500:	// DEC_						UCS-4BE
	case 0x0600:	// CP0						UCS-4BE
		{
			m_SaveBuf.Clear();
			while ( out->GetSize() >= 4 ) {
				int ch = out->Get32Bit();
				int n = m_CodeIndex.Find(ch);
				m_SaveBuf.PutByte(n != 0 ? n : (ch < 0x80 ? ch : '?'));
			}
			out->Swap(m_SaveBuf);
		}
		break;
	case 0x0700:	// CP						UCS-2LE
		{
			int len = ::WideCharToMultiByte(m_ToCodePage, 0, (LPCWSTR)out->GetPtr(), out->GetSize() / sizeof(WCHAR), NULL, 0, NULL, NULL );
			m_SaveBuf.Clear();
			::WideCharToMultiByte(m_ToCodePage, 0, (LPCWSTR)out->GetPtr(), out->GetSize() / sizeof(WCHAR), (LPSTR)m_SaveBuf.PutSpc(len), len, NULL, NULL );
			out->Swap(m_SaveBuf);
		}
		break;
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

	in = pIconv->ExtFromConv(in);

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

	pIconv->ExtToConv(out);
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

	in = pIconv->ExtFromConv(in);

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

	pIconv->ExtToConv(out);

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

	switch(cp->m_Mode & 0x00FF) {
	case 0x0003:		// JISX0213.1 > EUC-JISX0213
		ch |= 0x8080;
		break;
	case 0x0004:		// JISX0213.2 > EUC-JISX0213
		ch |= 0x8F8080;
		break;

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
		ch = CIso646Dlg::TableMaping(cp->m_FromCodePage, ch);
		from = _T("UCS-4BE");
		break;
	case 0x0009:		// CP
		if ( (ch & 0xFF000000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 24);
		if ( (ch & 0xFFFF0000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 16);
		if ( (ch & 0xFFFFFF00) != 0 )
			itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
		n = ::MultiByteToWideChar(cp->m_FromCodePage, 0, (LPCSTR)itmp, n, (LPWSTR)otmp, 32) * sizeof(WCHAR);
		ch = 0;
		for ( int i = 0 ; i < n ; i++ )
			ch = (ch << 8) | otmp[i];
		n = 0;
		from = _T("UCS-2LE");
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
	case 0x0300:	// EUC-JISX0213 > JISX0213.1
		if ( ch >= 0xA1A1 && ch <= 0xFEFE )
			ch &= 0x7F7F;
		else
			ch = 0x0000;
		break;
	case 0x0400:	// EUC-JISX0213 > JISX0213.2
		if ( ch >= 0x8FA1A1 && ch <= 0x8FFEFE )
			ch &= 0x7F7F;
		else
			ch = 0x0000;
		break;
	case 0x0500:	// DEC....
	case 0x0600:	// CP0....
		if ( (n = cp->m_CodeIndex.Find(ch)) != 0 )
			ch = n;
		break;
	case 0x0700:	// CP
		n = ::WideCharToMultiByte(cp->m_ToCodePage, 0, (LPCWSTR)otmp, (32 - (int)ots) / sizeof(WCHAR), (LPSTR)inp, 32, NULL, NULL );
		ch = 0;
		for ( int i = 0 ; i < n ; i++ )
			ch = (ch << 8) | inp[i];
		break;
	}

	if ( (od & 0xFFFFFF00) == 0 )
		cp->m_Table[od] = ch;
	return ch;
}

BOOL CIConv::IHaveCodePage(int cp)
{
	CPINFO cpInfo;
	return GetCPInfo(cp, &cpInfo);
}

static int GetCharSet(unsigned int namescount, const char * const * names, void* data)
{
	unsigned int i, n;
	CStringArray *pArray = (CStringArray *)data;

	for ( i = 0 ; i < namescount ; i++ ) {
		if ( strncmp(names[i], "CP", 2) == 0 && (n = atoi(names[i] + 2)) > 0 ) {
			if ( !CIConv::IHaveCodePage(n) )
				pArray->Add(MbsToTstr(names[i]));
			else if ( namescount == 1 ) {
				CString tmp;
				tmp.Format(_T("ICONV-%s"), MbsToTstr(names[i]));
				pArray->Add(tmp);
			}
		} else
			pArray->Add(MbsToTstr(names[i]));
	}
	return 0;
}
 static int __cdecl IConvListCmp(const void *src, const void *dis)
 {
	 CStringLoad *pSrc = (CStringLoad *)src;
	 CStringLoad *pDis = (CStringLoad *)dis;

	 return pSrc->CompareDigit((LPCTSTR)(*pDis));
 }

 void CIConv::SetListArray(CStringArray &stra)
{
	stra.RemoveAll();
	iconvlist(GetCharSet, &stra);

	stra.Add(_T("JIS_X0213-2000.1"));
	stra.Add(_T("JIS_X0213-2000.2"));

	for ( int n = 0 ; n < (sizeof(DecCharTab) / sizeof(DECCHARTAB)) ; n++ )
		stra.Add(DecCharTab[n].name);

	for ( int n = 0 ; n < (sizeof(CodePageList) / sizeof(LPCTSTR)) ; n++ ) {
		if ( CIConv::IHaveCodePage(_tstoi(CodePageList[n] + 2)) )
			stra.Add(CodePageList[n]);
	}

	CIso646Dlg::CodePageList(stra);

	qsort((void *)stra.GetData(), stra.GetSize(), sizeof(CString), IConvListCmp);
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

