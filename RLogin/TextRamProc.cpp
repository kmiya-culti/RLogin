// TextRam.cpp: CTextRam クラスのインプリメンテーション
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

#include <iconv.h>
#include <imm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// fc Init

static int fc_Init_Flag = FALSE;
static void (CTextRam::*fc_Proc[STAGE_MAX][256])(int ch);
static CTextRam::ESCNAMEPROC	*fc_pEscProc = NULL;
static CTextRam::ESCNAMEPROC	*fc_pCsiProc = NULL;

static const CTextRam::PROCTAB fc_CtrlTab[] = {
	{ 0x01,		0,			&CTextRam::fc_SOH		},
	{ 0x05,		0,			&CTextRam::fc_ENQ		},
	{ 0x07,		0,			&CTextRam::fc_BEL		},
	{ 0x08,		0,			&CTextRam::fc_BS		},
	{ 0x09,		0,			&CTextRam::fc_HT		},
	{ 0x0A,		0,			&CTextRam::fc_LF		},
	{ 0x0B,		0,			&CTextRam::fc_VT		},
	{ 0x0C,		0,			&CTextRam::fc_FF		},
	{ 0x0D,		0,			&CTextRam::fc_CR		},
	{ 0x0E,		0,			&CTextRam::fc_SO		},
	{ 0x0F,		0,			&CTextRam::fc_SI		},
	{ 0x10,		0,			&CTextRam::fc_DLE		},
	{ 0x18,		0,			&CTextRam::fc_CAN		},
	{ 0x1B,		0,			&CTextRam::fc_ESC		},
	{ 0x1C,		0,			&CTextRam::fc_A3CRT		},	// ADM-3 Cursole Right
	{ 0x1D,		0,			&CTextRam::fc_A3CLT		},	// ADM-3 Cursole Left
	{ 0x1E,		0,			&CTextRam::fc_A3CUP		},	// ADM-3 Cursole Up
	{ 0x1F,		0,			&CTextRam::fc_A3CDW		},	// ADM-3 Cursole Down
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_SEscTab[] = {
	{ 0x80,		0x9F,		&CTextRam::fc_SESC		},
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_EucTab[] = {
	{ 0x20,		0x7F,		&CTextRam::fc_TEXT		},
	{ 0x8E,		0,			&CTextRam::fc_SESC		},	// SS2 Single shift 2
	{ 0x8F,		0,			&CTextRam::fc_SESC		},	// SS3 Single shift 3
	{ 0xA0,		0xFF,		&CTextRam::fc_TEXT		},
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_94x94Tab[] = {
	{ 0x00,		0x20,		&CTextRam::fc_TEXT3		},
	{ 0x21,		0x7E,		&CTextRam::fc_TEXT2		},
	{ 0x7F,		0,			&CTextRam::fc_TEXT3		},
	{ 0x80,		0xA0,		&CTextRam::fc_TEXT3		},
	{ 0xA1,		0xFE,		&CTextRam::fc_TEXT2		},
	{ 0xFF,		0,			&CTextRam::fc_TEXT3		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_96x96Tab[] = {
	{ 0x00,		0x1F,		&CTextRam::fc_TEXT3		},
	{ 0x20,		0x7F,		&CTextRam::fc_TEXT2		},
	{ 0x80,		0x9F,		&CTextRam::fc_TEXT3		},
	{ 0xA0,		0xFF,		&CTextRam::fc_TEXT2		},
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_Sjis1Tab[] = {
	{ 0x20,		0x7E,		&CTextRam::fc_TEXT		},
	{ 0x81,		0x9F,		&CTextRam::fc_SJIS1		},
	{ 0xA0,		0xDF,		&CTextRam::fc_TEXT		},
	{ 0xE0,		0xFC,		&CTextRam::fc_SJIS1		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Sjis2Tab[] = {
	{ 0x00,		0x3F,		&CTextRam::fc_SJIS3		},
	{ 0x40,		0x7E,		&CTextRam::fc_SJIS2		},
	{ 0x7F,		0,			&CTextRam::fc_SJIS3		},
	{ 0x80,		0xFC,		&CTextRam::fc_SJIS2		},
	{ 0xFD,		0xFF,		&CTextRam::fc_SJIS3		},
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_Utf8Tab[] = {
	{ 0x20,		0x7E,		&CTextRam::fc_TEXT		},
	{ 0xA0,		0xBF,		&CTextRam::fc_UTF87		},
	{ 0xC0,		0xDF,		&CTextRam::fc_UTF81		},
	{ 0xE0,		0xEF,		&CTextRam::fc_UTF82		},
	{ 0xF0,		0xF7,		&CTextRam::fc_UTF83		},	// Unicode 21 bit
//	{ 0xF8,		0xFB,		&CTextRam::fc_UTF88		},	// UCS-4 ? 26 bit
//	{ 0xFC,		0xFD,		&CTextRam::fc_UTF89		},	// UCS-4 ? 31 bit
	{ 0xF8,		0xFD,		&CTextRam::fc_UTF87		},
	{ 0xFE,		0xFF,		&CTextRam::fc_UTF84		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Utf82Tab[] = {
	{ 0x00,		0x7F,		&CTextRam::fc_UTF86		},
	{ 0x80,		0xBF,		&CTextRam::fc_UTF85		},
	{ 0xC0,		0xFD,		&CTextRam::fc_UTF86		},
	{ 0xFE,		0xFF,		&CTextRam::fc_POP		},
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_EscTab[] = {
	{ 0x0C,		0,			&CTextRam::fc_TEK_IN	},
	{ 0x18,		0,			&CTextRam::fc_POP		},
	{ 0x1A,		0,			&CTextRam::fc_POP		},
	{ 0x1B,		0,			&CTextRam::fc_IGNORE	},
	{ 0x20,		0x7F,		&CTextRam::fc_POP		},
	{ 0x80,		0x9F,		&CTextRam::fc_SESC		},
	{ 0xA0,		0xFF,		&CTextRam::fc_POP		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Esc2Tab[] = {
	{ ' ',		0,			&CTextRam::fc_ACS		},	// ACS Announce code structure
	{ '#',		0,			&CTextRam::fc_DECSOP	},	// DECSOP Screen Opt
	{ '$',		0,			&CTextRam::fc_CSC0W		},	// CSC0W Char Set
	{ '%',		0,			&CTextRam::fc_DOCS		},	// DOCS Designate other coding system
	{ '(',		0,			&CTextRam::fc_CSC0		},	// CSC0 G0 charset
	{ ')',		0,			&CTextRam::fc_CSC1		},	// CSC1 G1 charset
	{ '*',		0,			&CTextRam::fc_CSC2		},	// CSC2 G2 charset
	{ '+',		0,			&CTextRam::fc_CSC3		},	// CSC3 G3 charset
	{ ',',		0,			&CTextRam::fc_CSC0A		},	// CSC0A G0 charset
	{ '-',		0,			&CTextRam::fc_CSC1A		},	// CSC1A G1 charset
	{ '.',		0,			&CTextRam::fc_CSC2A		},	// CSC2A G2 charset
	{ '/',		0,			&CTextRam::fc_CSC3A		},	// CSC3A G3 charset
	{ '1',		0,			&CTextRam::fc_DECHTS	},	// DECHTS Horizontal tab set
	{ '2',		0,			&CTextRam::fc_CCAHT		},	// DECCAHT Clear all horizontal tabs
	{ '3',		0,			&CTextRam::fc_DECVTS	},	// DECVTS Vertical tab set
	{ '4',		0,			&CTextRam::fc_CAVT		},	// DECCAVT Clear all vertical tabs
	{ '6',		0,			&CTextRam::fc_BI		},	// DECBI Back Index
	{ '7',		0,			&CTextRam::fc_SC		},	// DECSC Save Cursor
	{ '8',		0,			&CTextRam::fc_RC		},	// DECRC Restore Cursor
	{ '9',		0,			&CTextRam::fc_FI		},	// DECFI Forward Index
	{ '<',		0,			&CTextRam::fc_V5EX		},	//											VT52 Exit VT52 mode. Enter VT100 mode.
	{ '=',		0,			&CTextRam::fc_DECPAM	},	// DECPAM Application Keypad				VT52 Enter alternate keypad mode.
	{ '>',		0,			&CTextRam::fc_DECPNM	},	// DECPNM Normal Keypad						VT52 Exit alternate keypad mode.
	{ 'A',		0,			&CTextRam::fc_V5CUP		},	//											VT52 Cursor up.
	{ 'B',		0,			&CTextRam::fc_BPH		},	// BPH Break permitted here					VT52 Cursor down
	{ 'C',		0,			&CTextRam::fc_NBH		},	// NBH No break here						VT52 Cursor right
	{ 'D',		0,			&CTextRam::fc_IND		},	// IND Index								VT52 Cursor left
	{ 'E',		0,			&CTextRam::fc_NEL		},	// NEL Next Line
	{ 'F',		0,			&CTextRam::fc_SSA		},	// SSA Start selected area					VT52 Enter graphics mode.
	{ 'G',		0,			&CTextRam::fc_ESA		},	// ESA End selected area					VT52 Exit graphics mode.
	{ 'H',		0,			&CTextRam::fc_HTS		},	// HTS Character tabulation set				VT52 Cursor to home position.
	{ 'I',		0,			&CTextRam::fc_HTJ		},	// HTJ Character tabulation with just		VT52 Reverse line feed.
	{ 'J',		0,			&CTextRam::fc_VTS		},	// VTS Line tabulation set					VT52 Erase cursor screen.
	{ 'K',		0,			&CTextRam::fc_PLD		},	// PLD Partial line forward					VT52 Erase from cursor to end of line.
	{ 'L',		0,			&CTextRam::fc_PLU		},	// PLU Partial line backward
	{ 'M',		0,			&CTextRam::fc_RI		},	// RI Reverse index
	{ 'N',		0,			&CTextRam::fc_SS2		},	// SS2 Single shift 2
	{ 'O',		0,			&CTextRam::fc_SS3		},	// SS3 Single shift 3
	{ 'P',		0,			&CTextRam::fc_DCS		},	// DCS Device Control String
//	{ 'Q',		0,			&CTextRam::fc_POP		},	// PU1 Private use one
//	{ 'R',		0,			&CTextRam::fc_POP		},	// PU2 Private use two
	{ 'S',		0,			&CTextRam::fc_STS		},	// STS Set transmit state
//	{ 'T',		0,			&CTextRam::fc_CCH		},	// CCH Cancel character
//	{ 'U',		0,			&CTextRam::fc_MW		},	// MW Message waiting
	{ 'V',		0,			&CTextRam::fc_SPA		},	// SPA Start of guarded area				VT52 Print the line with the cursor.
	{ 'W',		0,			&CTextRam::fc_EPA		},	// EPA End of guarded area					VT52 Enter printer controller mode.
//	{ 'X',		0,			&CTextRam::fc_SOS		},	// SOS Start of String
	{ 'Y',		0,			&CTextRam::fc_V5MCP		},	//											VT52 Move cursor to column Pn.
	{ 'Z',		0,			&CTextRam::fc_SCI		},	// SCI Single character introducer			VT52 Identify (host to terminal).
	{ '[',		0,			&CTextRam::fc_CSI		},	// CSI Control sequence introducer
	{ '\\',		0,			&CTextRam::fc_POP		},	// ST String terminator
	{ ']',		0,			&CTextRam::fc_OSC		},	// OSC Operating System Command
//	{ '^',		0,			&CTextRam::fc_PM		},	// PM Privacy message
//	{ '_',		0,			&CTextRam::fc_APC		},	// APC Application Program Command
	{ 'c',		0,			&CTextRam::fc_RIS		},	// RIS Reset to initial state
	{ 'l',		0,			&CTextRam::fc_LMA		},	// HP LMA Lock memory above
	{ 'm',		0,			&CTextRam::fc_USR		},	// HP USR Unlock scrolling region
	{ 'n',		0,			&CTextRam::fc_LS2		},	// LS2 Locking-shift two
	{ 'o',		0,			&CTextRam::fc_LS3		},	// LS3 Locking-shift three
	{ '|',		0,			&CTextRam::fc_LS3R		},	// LS3R
	{ '}',		0,			&CTextRam::fc_LS2R		},	// LS2R
	{ '~',		0,			&CTextRam::fc_LS1R		},	// LS1R
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_CsiTab[] = {
	{ 0x18,		0,			&CTextRam::fc_POP		},
	{ 0x1A,		0,			&CTextRam::fc_POP		},
	{ 0x1B,		0,			&CTextRam::fc_CSI_ESC	},
	{ 0x20,		0x2F,		&CTextRam::fc_CSI_EXT	},	// SP!"#$%&'()*+,-./
	{ 0x30,		0x39,		&CTextRam::fc_CSI_DIGIT	},
	{ 0x3B,		0,			&CTextRam::fc_CSI_SEPA	},
	{ 0x3C,		0x3F,		&CTextRam::fc_CSI_EXT	},	// <=>?
	{ 0x40,		0x7F,		&CTextRam::fc_POP		},
	{ 0x80,		0x9F,		&CTextRam::fc_SESC		},
	{ 0xA0,		0xAF,		&CTextRam::fc_CSI_EXT	},
	{ 0xB0,		0xB9,		&CTextRam::fc_CSI_DIGIT	},
	{ 0xBB,		0,			&CTextRam::fc_CSI_SEPA	},
	{ 0xBC,		0xBF,		&CTextRam::fc_CSI_EXT	},
	{ 0xC0,		0xFF,		&CTextRam::fc_POP		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_AnsiTab[] = {
	{ '@',		0,			&CTextRam::fc_ICH		},	// ICH Insert Character
	{ 'A',		0,			&CTextRam::fc_CUU		},	// CUU Cursor Up
	{ 'B',		0,			&CTextRam::fc_CUD		},	// CUD Cursor Down
	{ 'C',		0,			&CTextRam::fc_CUF		},	// CUF Cursor Forward
	{ 'D',		0,			&CTextRam::fc_CUB		},	// CUB Cursor Backward
	{ 'E',		0,			&CTextRam::fc_CNL		},	// CNL Move the cursor to the next line.
	{ 'F',		0,			&CTextRam::fc_CPL		},	// CPL Move the cursor to the preceding line.
	{ 'G',		0,			&CTextRam::fc_CHA		},	// CHA Move the active position to the n-th character of the active line.
	{ 'H',		0,			&CTextRam::fc_CUP		},	// CUP Cursor Position
	{ 'I',		0,			&CTextRam::fc_CHT		},	// CHT Move the active position n tabs forward.
	{ 'J',		0,			&CTextRam::fc_ED		},	// ED Erase in page
	{ 'K',		0,			&CTextRam::fc_EL		},	// EL Erase in line
	{ 'L',		0,			&CTextRam::fc_IL		},	// IL Insert Line
	{ 'M',		0,			&CTextRam::fc_DL		},	// DL Delete Line
	{ 'N',		0,			&CTextRam::fc_EF		},	// EF Erase in field
	{ 'O',		0,			&CTextRam::fc_EA		},	// EA Erase in area
	{ 'P',		0,			&CTextRam::fc_DCH		},	// DCH Delete Character
//	{ 'Q',		0,			&CTextRam::fc_SEE		},	// SEE SELECT EDITING EXTENT
//	{ 'R',		0,			&CTextRam::fc_CPR		},	// CPR ACTIVE POSITION REPORT
	{ 'S',		0,			&CTextRam::fc_SU		},	// SU Scroll Up
	{ 'T',		0,			&CTextRam::fc_SD		},	// SD Scroll Down
	{ 'U',		0,			&CTextRam::fc_NP		},	// NP Next Page
	{ 'V',		0,			&CTextRam::fc_PP		},	// PP Preceding Page
	{ 'W',		0,			&CTextRam::fc_CTC		},	// CTC Cursor tabulation control
	{ 'X',		0,			&CTextRam::fc_ECH		},	// ECH Erase character
	{ 'Y',		0,			&CTextRam::fc_CVT		},	// CVT Cursor line tabulation
	{ 'Z',		0,			&CTextRam::fc_CBT		},	// CBT Move the active position n tabs backward.
//	{ '[',		0,			&CTextRam::fc_SRS		},	// SRS START REVERSED STRING
//	{ '\\',		0,			&CTextRam::fc_PTX		},	// PTX PARALLEL TEXTS
//	{ ']',		0,			&CTextRam::fc_SDS		},	// SDS START DIRECTED STRING
//	{ '^',		0,			&CTextRam::fc_SIMD		},	// SIMD Select implicit movement direction
	{ '`',		0,			&CTextRam::fc_HPA		},	// HPA Horizontal Position Absolute
	{ 'a',		0,			&CTextRam::fc_HPR		},	// HPR Horizontal Position Relative
	{ 'b',		0,			&CTextRam::fc_REP		},	// REP Repeat
	{ 'c',		0,			&CTextRam::fc_DA1		},	// DA1 Primary Device Attributes
	{ 'd',		0,			&CTextRam::fc_VPA		},	// VPA Vertical Line Position Absolute
	{ 'e',		0,			&CTextRam::fc_VPR		},	// VPR Vertical Position Relative
	{ 'f',		0,			&CTextRam::fc_HVP		},	// HVP Horizontal and Vertical Position
	{ 'g',		0,			&CTextRam::fc_TBC		},	// TBC Tab Clear
	{ 'h',		0,			&CTextRam::fc_SM		},	// SM Mode Set
//	{ 'i',		0,			&CTextRam::fc_MC		},	// MC Media copy
	{ 'j',		0,			&CTextRam::fc_HPB		},	// HPB Character position backward
	{ 'k',		0,			&CTextRam::fc_VPB		},	// VPB Line position backward
	{ 'l',		0,			&CTextRam::fc_RM		},	// RM Mode ReSet
	{ 'm',		0,			&CTextRam::fc_SGR		},	// SGR Select Graphic Rendition
	{ 'n',		0,			&CTextRam::fc_DSR		},	// DSR Device status report
//	{ 'o',		0,			&CTextRam::fc_DAQ		},	// DAQ Define area qualification
	{ 'p',		0,			&CTextRam::fc_DECBFAT	},	// DECBFAT Begin field attribute : DEC private
//	{ 'q',		0,			&CTextRam::fc_DECLL		},	// DECLL Load LEDs
	{ 'r',		0,			&CTextRam::fc_DECSTBM	},	// DECSTBM Set Top and Bottom Margins
	{ 's',		0,			&CTextRam::fc_DECSC		},	// DECSC Save Cursol Pos
	{ 't',		0,			&CTextRam::fc_XTWOP		},	// XTWOP XTERM WINOPS
	{ 'u',		0,			&CTextRam::fc_DECRC		},	// DECRC Load Cursol Pos
	{ 'v',		0,			&CTextRam::fc_ORGSCD	},	// ORGSCD Set Cursol Display
	{ 'x',		0,			&CTextRam::fc_REQTPARM	},	// DECREQTPARM Request terminal parameters
//	{ 'y',		0,			&CTextRam::fc_DECTST	},	// DECTST Invoke confidence test
//	{ 'z',		0,			&CTextRam::fc_DECVERP	},	// DECVERP Set vertical pitch
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext1Tab[] = {			// ('?' << 16) | sc
	{ 'J',		0,			&CTextRam::fc_DECSED	},	// DECSED Selective Erase in Display
	{ 'K',		0,			&CTextRam::fc_DECSEL	},	// DECSEL Selective Erase in Line
	{ 'W',		0,			&CTextRam::fc_DECST8C	},	// DECST8C Set Tab at every 8 columns
	{ 'h',		0,			&CTextRam::fc_DECSET	},	// DECSET
	{ 'l',		0,			&CTextRam::fc_DECRST	},	// DECRST
	{ 'n',		0,			&CTextRam::fc_DECDSR	},	// DECDSR Device status report
	{ 'r',		0,			&CTextRam::fc_XTREST	},	// XTREST
	{ 's',		0,			&CTextRam::fc_XTSAVE	},	// XTSAVE
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext2Tab[] = {			// ('$' << 8) | sc
	{ 'p',		0,			&CTextRam::fc_DECRQM	},	// DECRQM Request mode settings
	{ 'r',		0,			&CTextRam::fc_DECCARA	},	// DECCARA Change Attribute in Rectangle
	{ 't',		0,			&CTextRam::fc_DECRARA	},	// DECRARA Reverse Attribute in Rectangle
	{ 'v',		0,			&CTextRam::fc_DECCRA	},	// DECCRA Copy Rectangular Area
	{ 'x',		0,			&CTextRam::fc_DECFRA	},	// DECFRA Fill Rectangular Area
	{ 'z',		0,			&CTextRam::fc_DECERA	},	// DECERA Erase Rectangular Area
	{ '{',		0,			&CTextRam::fc_DECSERA	},	// DECSERA Selective Erase Rectangular Area
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext3Tab[] = {			// (' ' << 8) | sc
	{ '@',		0,			&CTextRam::fc_SL		},	// SL Scroll left
	{ 'A',		0,			&CTextRam::fc_SR		},	// SRScroll Right
	{ '~',		0,			&CTextRam::fc_DECTME	},	// DECTME Terminal Mode Emulation
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext4Tab[] = {
	{ 0x40,		0x7F,		&CTextRam::fc_CSI_ETC	},
	{ 0,		0,			NULL } };
static const CTextRam::CSIEXTTAB fc_CsiExtTab[] = {
	{ ('?' << 16) | ('$'  << 8) | 'p',		&CTextRam::fc_DECRQMH	},	// DECRQMH Request Mode (DEC) Host to Terminal
	{ 				('!'  << 8) | 'p',		&CTextRam::fc_DECSTR	},	// DECSTR Soft terminal reset
	{ 				('"'  << 8) | 'q',		&CTextRam::fc_DECSCA	},	// DECSCA Select character attributes
	{ 				('\'' << 8) | 'w',		&CTextRam::fc_DECEFR	},	// DECEFR Enable filter rectangle
	{ 				('\'' << 8) | 'z',		&CTextRam::fc_DECELR	},	// DECELR Enable locator reports
	{ 				('\'' << 8) | '{',		&CTextRam::fc_DECSLE	},	// DECSLE Select locator events
	{ 				('\'' << 8) | '|',		&CTextRam::fc_DECRQLP	},	// DECRQLP Request locator position
	{ 				('\'' << 8) | '}',		&CTextRam::fc_DECIC		},	// DECIC Insert Column
	{ 				('\'' << 8) | '~',		&CTextRam::fc_DECDC		},	// DECDC Delete Column(s)
	{ 				('*'  << 8) | 'x',		&CTextRam::fc_DECSACE	},	// DECSACE Select Attribute and Change Extent
	{ 				(','  << 8) | '}',		&CTextRam::fc_DECATC	},	// DECATC Alternate Text Colors
	{ ('>' << 16)				| 'c',		&CTextRam::fc_DA2		},	// DA2 Secondary Device Attributes
	{ ('=' << 16)				| 'c',		&CTextRam::fc_DA3		},	// DA3 Tertiary Device Attributes
	{ ('=' << 16)				| 'S',		&CTextRam::fc_C25LCT	},	// C25LCT cons25 Set local cursor type
	{							    0,		NULL } };

static const CTextRam::PROCTAB fc_Osc1Tab[] = {
	{ 0x07,		0,			&CTextRam::fc_OSC_ST	},	// BELL (ST)
	{ 0x0E,		0,			&CTextRam::fc_SO		},
	{ 0x0F,		0,			&CTextRam::fc_SI		},
	{ 0x18,		0,			&CTextRam::fc_OSC_POP	},
	{ 0x1A,		0,			&CTextRam::fc_OSC_POP	},
	{ 0x1B,		0,			&CTextRam::fc_OSC_ESC	},
	{ 0x9C,		0,			&CTextRam::fc_OSC_ST	},	// ST String terminator
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Osc2Tab[] = {
	{ 0x07,		0,			&CTextRam::fc_OSC_EST	},	// BELL (ST)
	{ 0x0E,		0,			&CTextRam::fc_SO		},
	{ 0x0F,		0,			&CTextRam::fc_SI		},
	{ 0x18,		0,			&CTextRam::fc_POP		},
	{ 0x1A,		0,			&CTextRam::fc_POP		},
	{ 0x1B,		0,			&CTextRam::fc_IGNORE	},
	{ 0x20,		0xFF,		&CTextRam::fc_POP		},
	{ 0x9C,		0,			&CTextRam::fc_OSC_EST	},	// ST String terminator
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Osc3Tab[] = {
	{ 'N',		0,			&CTextRam::fc_SS2		},	// SS2 Single shift 2
	{ 'O',		0,			&CTextRam::fc_SS3		},	// SS3 Single shift 3
	{ 'n',		0,			&CTextRam::fc_LS2		},	// LS2 Locking-shift two
	{ 'o',		0,			&CTextRam::fc_LS3		},	// LS3 Locking-shift three
	{ '|',		0,			&CTextRam::fc_LS3R		},	// LS3R
	{ '}',		0,			&CTextRam::fc_LS2R		},	// LS2R
	{ '~',		0,			&CTextRam::fc_LS1R		},	// LS1R
	{ '$',		0,			&CTextRam::fc_CSC0W		},	// CSC0W Char Set
	{ '(',		0,			&CTextRam::fc_CSC0		},	// CSC0 G0 charset
	{ ')',		0,			&CTextRam::fc_CSC1		},	// CSC1 G1 charset
	{ '*',		0,			&CTextRam::fc_CSC2		},	// CSC2 G2 charset
	{ '+',		0,			&CTextRam::fc_CSC3		},	// CSC3 G3 charset
	{ ',',		0,			&CTextRam::fc_CSC0A		},	// CSC0A G0 charset
	{ '-',		0,			&CTextRam::fc_CSC1A		},	// CSC1A G1 charset
	{ '.',		0,			&CTextRam::fc_CSC2A		},	// CSC2A G2 charset
	{ '/',		0,			&CTextRam::fc_CSC3A		},	// CSC3A G3 charset
	{ '\\',		0,			&CTextRam::fc_OSC_EST	},	// ST String terminator
	{ '[',		0,			&CTextRam::fc_OSC_CSI	},	// CSI Control sequence introducer
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_TekTab[] = {
	{ 0x00,		0xFF,		&CTextRam::fc_TEK_STAT	},	// STAT
	{ 0x08,		0,			&CTextRam::fc_TEK_LEFT	},	// LEFT
	{ 0x09,		0,			&CTextRam::fc_TEK_RIGHT	},	// RIGHT
	{ 0x0A,		0,			&CTextRam::fc_TEK_DOWN	},	// DOWN
	{ 0x0B,		0,			&CTextRam::fc_TEK_UP	},	// UP
	{ 0x0D,		0,			&CTextRam::fc_TEK_FLUSH	},	// FLUSH
	{ 0x1B,		0,			&CTextRam::fc_TEK_MODE	},	// ESC
	{ 0x1C,		0,			&CTextRam::fc_TEK_MODE	},	// PT
	{ 0x1D,		0,			&CTextRam::fc_TEK_MODE	},	// PLT
	{ 0x1E,		0,			&CTextRam::fc_TEK_MODE	},	// IPL
	{ 0x1F,		0,			&CTextRam::fc_TEK_MODE	},	// ALP
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_StatTab[] = {
	{ 0x00,		0xFF,		&CTextRam::fc_STAT		},
	{ 0,		0,			NULL } };

static int	fc_EscNameTabMax = 57;
static CTextRam::ESCNAMEPROC fc_EscNameTab[] = {
	{	"ACS",		&CTextRam::fc_ACS,		NULL,	NULL	},
	{	"BPH",		&CTextRam::fc_BPH,		NULL,	NULL	},
	{	"CCH",		&CTextRam::fc_CCH,		NULL,	NULL	},
	{	"CSC0",		&CTextRam::fc_CSC0,		NULL,	NULL	},
	{	"CSC0A",	&CTextRam::fc_CSC0A,	NULL,	NULL	},
	{	"CSC0W",	&CTextRam::fc_CSC0W,	NULL,	NULL	},
	{	"CSC1",		&CTextRam::fc_CSC1,		NULL,	NULL	},
	{	"CSC1A",	&CTextRam::fc_CSC1A,	NULL,	NULL	},
	{	"CSC2",		&CTextRam::fc_CSC2,		NULL,	NULL	},
	{	"CSC2A",	&CTextRam::fc_CSC2A,	NULL,	NULL	},
	{	"CSC3",		&CTextRam::fc_CSC3,		NULL,	NULL	},
	{	"CSC3A",	&CTextRam::fc_CSC3A,	NULL,	NULL	},
	{	"CSI",		&CTextRam::fc_CSI,		NULL,	NULL	},
	{	"DCS",		&CTextRam::fc_DCS,		NULL,	NULL	},
	{	"DECBI",	&CTextRam::fc_BI,		NULL,	NULL	},
	{	"DECCAHT",	&CTextRam::fc_CCAHT,	NULL,	NULL	},
	{	"DECCAVT",	&CTextRam::fc_CAVT,		NULL,	NULL	},
	{	"DECFI",	&CTextRam::fc_FI,		NULL,	NULL	},
	{	"DECHTS",	&CTextRam::fc_DECHTS,	NULL,	NULL	},
	{	"DECPAM",	&CTextRam::fc_DECPAM,	NULL,	NULL	},
	{	"DECPNM",	&CTextRam::fc_DECPNM,	NULL,	NULL	},
	{	"DECRC",	&CTextRam::fc_RC,		NULL,	NULL	},
	{	"DECSC",	&CTextRam::fc_SC,		NULL,	NULL	},
	{	"DECSOP",	&CTextRam::fc_DECSOP,	NULL,	NULL	},
	{	"DECVTS",	&CTextRam::fc_DECVTS,	NULL,	NULL	},
	{	"DOCS",		&CTextRam::fc_DOCS,		NULL,	NULL	},
	{	"EPA",		&CTextRam::fc_EPA,		NULL,	NULL	},
	{	"ESA",		&CTextRam::fc_ESA,		NULL,	NULL	},
	{	"HTJ",		&CTextRam::fc_HTJ,		NULL,	NULL	},
	{	"HTS",		&CTextRam::fc_HTS,		NULL,	NULL	},
	{	"IND",		&CTextRam::fc_IND,		NULL,	NULL	},
	{	"LMA",		&CTextRam::fc_LMA,		NULL,	NULL	},
	{	"LS1R",		&CTextRam::fc_LS1R,		NULL,	NULL	},
	{	"LS2",		&CTextRam::fc_LS2,		NULL,	NULL	},
	{	"LS2R",		&CTextRam::fc_LS2R,		NULL,	NULL	},
	{	"LS3",		&CTextRam::fc_LS3,		NULL,	NULL	},
	{	"LS3R",		&CTextRam::fc_LS3R,		NULL,	NULL	},
	{	"MW",		&CTextRam::fc_MW,		NULL,	NULL	},
	{	"NBH",		&CTextRam::fc_NBH,		NULL,	NULL	},
	{	"NEL",		&CTextRam::fc_NEL,		NULL,	NULL	},
	{	"NOP",		&CTextRam::fc_POP,		NULL,	NULL	},
	{	"OSC",		&CTextRam::fc_OSC,		NULL,	NULL	},
	{	"PLD",		&CTextRam::fc_PLD,		NULL,	NULL	},
	{	"PLU",		&CTextRam::fc_PLU,		NULL,	NULL	},
	{	"RI",		&CTextRam::fc_RI,		NULL,	NULL	},
	{	"RIS",		&CTextRam::fc_RIS,		NULL,	NULL	},
	{	"SCI",		&CTextRam::fc_SCI,		NULL,	NULL	},
	{	"SPA",		&CTextRam::fc_SPA,		NULL,	NULL	},
	{	"SS2",		&CTextRam::fc_SS2,		NULL,	NULL	},
	{	"SS3",		&CTextRam::fc_SS3,		NULL,	NULL	},
	{	"SSA",		&CTextRam::fc_SSA,		NULL,	NULL	},
	{	"STS",		&CTextRam::fc_STS,		NULL,	NULL	},
	{	"USR",		&CTextRam::fc_USR,		NULL,	NULL	},
	{	"V5CUP",	&CTextRam::fc_V5CUP,	NULL,	NULL	},
	{	"V5EX",		&CTextRam::fc_V5EX,		NULL,	NULL	},
	{	"V5MCP",	&CTextRam::fc_V5MCP,	NULL,	NULL	},
	{	"VTS",		&CTextRam::fc_VTS,		NULL,	NULL	},
	{	NULL,		NULL,					NULL,	NULL	},
};

static int	fc_CsiNameTabMax = 86;
static CTextRam::ESCNAMEPROC fc_CsiNameTab[] = {
	{	"C25LCT",	&CTextRam::fc_C25LCT,	NULL,	NULL 	},
	{	"CBT",		&CTextRam::fc_CBT,		NULL,	NULL	},
	{	"CHA",		&CTextRam::fc_CHA,		NULL,	NULL	},
	{	"CHT",		&CTextRam::fc_CHT,		NULL,	NULL	},
	{	"CNL",		&CTextRam::fc_CNL,		NULL,	NULL	},
	{	"CPL",		&CTextRam::fc_CPL,		NULL,	NULL	},
	{	"CTC",		&CTextRam::fc_CTC,		NULL,	NULL	},
	{	"CUB",		&CTextRam::fc_CUB,		NULL,	NULL	},
	{	"CUD",		&CTextRam::fc_CUD,		NULL,	NULL	},
	{	"CUF",		&CTextRam::fc_CUF,		NULL,	NULL	},
	{	"CUP",		&CTextRam::fc_CUP,		NULL,	NULL	},
	{	"CUU",		&CTextRam::fc_CUU,		NULL,	NULL	},
	{	"CVT",		&CTextRam::fc_CVT,		NULL,	NULL	},
	{	"DA1",		&CTextRam::fc_DA1,		NULL,	NULL	},
	{	"DA2",		&CTextRam::fc_DA2,		NULL,	NULL	},
	{	"DA3",		&CTextRam::fc_DA3,		NULL,	NULL	},
	{	"DAQ",		&CTextRam::fc_DAQ,		NULL,	NULL	},
	{	"DCH",		&CTextRam::fc_DCH,		NULL,	NULL	},
	{	"DECATC",	&CTextRam::fc_DECATC,	NULL,	NULL	},
	{	"DECBFAT",	&CTextRam::fc_DECBFAT,	NULL,	NULL	},
	{	"DECCARA",	&CTextRam::fc_DECCARA,	NULL,	NULL	},
	{	"DECCRA",	&CTextRam::fc_DECCRA,	NULL,	NULL	},
	{	"DECDC",	&CTextRam::fc_DECDC,	NULL,	NULL	},
	{	"DECDSR",	&CTextRam::fc_DECDSR,	NULL,	NULL	},
	{	"DECEFR",	&CTextRam::fc_DECEFR,	NULL,	NULL	},
	{	"DECELR",	&CTextRam::fc_DECELR,	NULL,	NULL	},
	{	"DECERA",	&CTextRam::fc_DECERA,	NULL,	NULL	},
	{	"DECFRA",	&CTextRam::fc_DECFRA,	NULL,	NULL	},
	{	"DECIC",	&CTextRam::fc_DECIC,	NULL,	NULL	},
	{	"DECLL",	&CTextRam::fc_DECLL,	NULL,	NULL	},
	{	"DECRARA",	&CTextRam::fc_DECRARA,	NULL,	NULL	},
	{	"DECRC",	&CTextRam::fc_DECRC,	NULL,	NULL	},
	{	"DECRQLP",	&CTextRam::fc_DECRQLP,	NULL,	NULL	},
	{	"DECRQM",	&CTextRam::fc_DECRQM,	NULL,	NULL	},
	{	"DECRQMH",	&CTextRam::fc_DECRQMH,	NULL,	NULL	},
	{	"DECRST",	&CTextRam::fc_DECRST,	NULL,	NULL	},
	{	"DECSACE",	&CTextRam::fc_DECSACE,	NULL,	NULL	},
	{	"DECSC",	&CTextRam::fc_DECSC,	NULL,	NULL	},
	{	"DECSCA",	&CTextRam::fc_DECSCA,	NULL,	NULL	},
	{	"DECSED",	&CTextRam::fc_DECSED,	NULL,	NULL	},
	{	"DECSEL",	&CTextRam::fc_DECSEL,	NULL,	NULL	},
	{	"DECSERA",	&CTextRam::fc_DECSERA,	NULL,	NULL	},
	{	"DECSET",	&CTextRam::fc_DECSET,	NULL,	NULL	},
	{	"DECSLE",	&CTextRam::fc_DECSLE,	NULL,	NULL	},
	{	"DECST8C",	&CTextRam::fc_DECST8C,	NULL,	NULL	},
	{	"DECSTBM",	&CTextRam::fc_DECSTBM,	NULL,	NULL	},
	{	"DECSTR",	&CTextRam::fc_DECSTR,	NULL,	NULL	},
	{	"DECTME",	&CTextRam::fc_DECTME,	NULL,	NULL	},
	{	"DECTST",	&CTextRam::fc_DECTST,	NULL,	NULL	},
	{	"DECVERP",	&CTextRam::fc_DECVERP,	NULL,	NULL	},
	{	"DL",		&CTextRam::fc_DL,		NULL,	NULL	},
	{	"DSR",		&CTextRam::fc_DSR,		NULL,	NULL	},
	{	"EA",		&CTextRam::fc_EA,		NULL,	NULL	},
	{	"ECH",		&CTextRam::fc_ECH,		NULL,	NULL	},
	{	"ED",		&CTextRam::fc_ED,		NULL,	NULL	},
	{	"EF",		&CTextRam::fc_EF,		NULL,	NULL	},
	{	"EL",		&CTextRam::fc_EL,		NULL,	NULL	},
	{	"HPA",		&CTextRam::fc_HPA,		NULL,	NULL	},
	{	"HPB",		&CTextRam::fc_HPB,		NULL,	NULL	},
	{	"HPR",		&CTextRam::fc_HPR,		NULL,	NULL	},
	{	"HVP",		&CTextRam::fc_HVP,		NULL,	NULL	},
	{	"ICH",		&CTextRam::fc_ICH,		NULL,	NULL	},
	{	"IL",		&CTextRam::fc_IL,		NULL,	NULL	},
	{	"LINUX",	&CTextRam::fc_LINUX,	NULL,	NULL	},
	{	"MC",		&CTextRam::fc_MC,		NULL,	NULL	},
	{	"NOP",		&CTextRam::fc_POP,		NULL,	NULL	},
	{	"NP",		&CTextRam::fc_NP,		NULL,	NULL	},
	{	"ORGSCD",	&CTextRam::fc_ORGSCD,	NULL,	NULL	},
	{	"PP",		&CTextRam::fc_PP,		NULL,	NULL	},
	{	"REP",		&CTextRam::fc_REP,		NULL,	NULL	},
	{	"REQTPARM",	&CTextRam::fc_REQTPARM,	NULL,	NULL	},
	{	"RM",		&CTextRam::fc_RM,		NULL,	NULL	},
	{	"SD",		&CTextRam::fc_SD,		NULL,	NULL	},
	{	"SGR",		&CTextRam::fc_SGR,		NULL,	NULL	},
	{	"SIMD",		&CTextRam::fc_SIMD,		NULL,	NULL	},
	{	"SL",		&CTextRam::fc_SL,		NULL,	NULL	},
	{	"SM",		&CTextRam::fc_SM,		NULL,	NULL	},
	{	"SR",		&CTextRam::fc_SR,		NULL,	NULL	},
	{	"SU",		&CTextRam::fc_SU,		NULL,	NULL	},
	{	"TBC",		&CTextRam::fc_TBC,		NULL,	NULL	},
	{	"VPA",		&CTextRam::fc_VPA,		NULL,	NULL	},
	{	"VPB",		&CTextRam::fc_VPB,		NULL,	NULL	},
	{	"VPR",		&CTextRam::fc_VPR,		NULL,	NULL	},
	{	"XTREST",	&CTextRam::fc_XTREST,	NULL,	NULL	},
	{	"XTSAVE",	&CTextRam::fc_XTSAVE,	NULL,	NULL	},
	{	"XTWOP",	&CTextRam::fc_XTWOP,	NULL,	NULL	},
	{	NULL,		NULL,					NULL,	NULL	},
};

//////////////////////////////////////////////////////////////////////
// CProcNode

CProcNode::CProcNode()
{
	m_Type = 0;
	m_Code = 0;
	m_Name.Empty();
}
const CProcNode & CProcNode::operator = (CProcNode &data)
{
	m_Type = data.m_Type;
	m_Code = data.m_Code;
	m_Name = data.m_Name;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CProcTab

CProcTab::CProcTab()
{
	m_pSection = "ProcTab";
	m_Data.RemoveAll();
}
const CProcTab & CProcTab::operator = (CProcTab &data)
{
	m_Data.RemoveAll();
	for ( int n = 0 ; n < data.m_Data.GetSize() ; n++ )
		m_Data.Add(data.m_Data[n]);
	return *this;
}
void CProcTab::Init()
{
	m_Data.RemoveAll();
}
void CProcTab::SetArray(CStringArrayExt &array)
{
	int n;
	CString tmp;

	array.RemoveAll();
	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		tmp.Format("%d,%d,%s,", m_Data[n].m_Type, m_Data[n].m_Code, m_Data[n].m_Name);
		array.Add(tmp);
	}
}
void CProcTab::GetArray(CStringArrayExt &array)
{
	int n;
	CProcNode node;
	CStringArrayExt tmp;

	m_Data.RemoveAll();
	for ( n = 0 ; n < array.GetSize() ; n++ ) {
		tmp.GetString(array[n], ',');
		if ( tmp.GetSize() < 3  )
			continue;
		node.m_Type = atoi(tmp[0]);
		node.m_Code = atoi(tmp[1]);
		node.m_Name = tmp[2];
		m_Data.Add(node);
	}
}
void CProcTab::Add(int type, int code, LPCSTR name)
{
	CProcNode node;

	node.m_Type = type;
	node.m_Code = code;
	node.m_Name = name;
	m_Data.Add(node);
}

//////////////////////////////////////////////////////////////////////
// CTextRam::fc_xxx

void CTextRam::fc_Init_Proc(int stage, const PROCTAB *tp, int b)
{
	int c;

	if ( b == 0 ) {
		for ( ; tp->proc != NULL ; tp++ ) {
			if ( tp->ec == 0 )
				fc_Proc[stage][tp->sc] = tp->proc;
			else {
				for ( c = tp->sc ; c <= tp->ec ; c++ )
					fc_Proc[stage][c] = tp->proc;
			}
		}
	} else {
		for ( ; tp->proc != NULL ; tp++ ) {
			if ( tp->ec == 0 ) {
				fc_Proc[stage][tp->sc] = tp->proc;
				fc_Proc[stage][tp->sc | b] = tp->proc;
			} else {
				for ( c = tp->sc ; c <= tp->ec ; c++ ) {
					fc_Proc[stage][c] = tp->proc;
					fc_Proc[stage][c | b] = tp->proc;
				}
			}
		}
	}
}
static int fc_ProcNameCmp(const void *src, const void *dis)
{
	return strcmp(((CTextRam::ESCNAMEPROC *)src)->name, ((CTextRam::ESCNAMEPROC *)dis)->name);
}
CTextRam::ESCNAMEPROC *CTextRam::fc_InitProcName(CTextRam::ESCNAMEPROC *tab, int *max)
{
	int n, c;
	ESCNAMEPROC *tp, *np, *bp = NULL;

	//for ( n = 0 ; tab[n].name != NULL ; n++ );
	//qsort(tab, n, sizeof(CTextRam::ESCNAMEPROC), fc_ProcNameCmp);

	for ( n = 0 ; tab[n].name != NULL ; n++ ) {
		np = &(tab[n]);
		np->left = np->right = NULL;
		if ( n == 0 ) {
			bp = np;
			continue;
		}
		for ( tp = bp ; ; ) {
			if ( (c = memcmp(tp->data.byte, np->data.byte, sizeof(void (CTextRam::*)(int)))) == 0 )
				break;
			else if ( c < 0 ) {
				if ( tp->left == NULL ) {
					tp->left = np;
					break;
				} else
					tp = tp->left;
			} else {
				if ( tp->right == NULL ) {
					tp->right = np;
					break;
				} else
					tp = tp->right;
			}
		}
	}
	*max = n;
	return bp;
}
void CTextRam::fc_Init(int mode)
{
	int i, n, c, b, m;
	ESCNAMEPROC *tp;

	if ( !fc_Init_Flag ) {
		fc_Init_Flag = TRUE;

		for ( c = 0; c < 256 ; c++ )
			fc_Proc[0][c] = &CTextRam::fc_IGNORE;

		for ( i = 1 ; i < STAGE_MAX ; i++ )
			memcpy(fc_Proc[i], fc_Proc[0], sizeof(fc_Proc[0]));

		fc_Init_Proc(STAGE_EUC, fc_CtrlTab);
		fc_Init_Proc(STAGE_EUC, fc_EucTab);
		fc_Init_Proc(STAGE_EUC, fc_SEscTab);

		fc_Init_Proc(STAGE_94X94, fc_94x94Tab);
		fc_Init_Proc(STAGE_96X96, fc_96x96Tab);

		fc_Init_Proc(STAGE_SJIS,  fc_CtrlTab);
		fc_Init_Proc(STAGE_SJIS,  fc_Sjis1Tab);
		fc_Init_Proc(STAGE_SJIS2, fc_Sjis2Tab);

		fc_Init_Proc(STAGE_UTF8,  fc_CtrlTab);
		fc_Init_Proc(STAGE_UTF8,  fc_Utf8Tab);
		fc_Init_Proc(STAGE_UTF8,  fc_SEscTab);
		fc_Init_Proc(STAGE_UTF82, fc_Utf82Tab);

		fc_Init_Proc(STAGE_ESC, fc_CtrlTab);
		fc_Init_Proc(STAGE_ESC, fc_EscTab);
		fc_Init_Proc(STAGE_ESC, fc_Esc2Tab, 0x80);

		fc_Init_Proc(STAGE_CSI, fc_CtrlTab);
		fc_Init_Proc(STAGE_CSI, fc_CsiTab);
		fc_Init_Proc(STAGE_CSI, fc_AnsiTab, 0x80);

		fc_Init_Proc(STAGE_EXT1, fc_CtrlTab);
		fc_Init_Proc(STAGE_EXT1, fc_CsiTab);
		fc_Init_Proc(STAGE_EXT1, fc_Ext1Tab, 0x80);

		fc_Init_Proc(STAGE_EXT2, fc_CtrlTab);
		fc_Init_Proc(STAGE_EXT2, fc_CsiTab);
		fc_Init_Proc(STAGE_EXT2, fc_Ext2Tab, 0x80);

		fc_Init_Proc(STAGE_EXT3, fc_CtrlTab);
		fc_Init_Proc(STAGE_EXT3, fc_CsiTab);
		fc_Init_Proc(STAGE_EXT3, fc_Ext3Tab, 0x80);

		fc_Init_Proc(STAGE_EXT4, fc_CtrlTab);
		fc_Init_Proc(STAGE_EXT4, fc_CsiTab);
		fc_Init_Proc(STAGE_EXT4, fc_Ext4Tab, 0x80);

		fc_Init_Proc(STAGE_OSC1, fc_Osc1Tab);
		fc_Init_Proc(STAGE_OSC1, fc_EucTab);		// EUC ASCII
		fc_Init_Proc(STAGE_OSC2, fc_Osc1Tab);
		fc_Init_Proc(STAGE_OSC2, fc_Sjis1Tab);		// SJIS
		fc_Init_Proc(STAGE_OSC3, fc_Osc1Tab);
		fc_Init_Proc(STAGE_OSC3, fc_Utf8Tab);		// UTF8
		fc_Init_Proc(STAGE_OSC4, fc_Osc2Tab);
		fc_Init_Proc(STAGE_OSC4, fc_Osc3Tab, 0x80);	// ESC

		fc_Init_Proc(STAGE_TEK, fc_TekTab);			// TEK
		fc_Init_Proc(STAGE_STAT, fc_StatTab);

		fc_pEscProc = fc_InitProcName(fc_EscNameTab, &fc_EscNameTabMax);
		fc_pCsiProc = fc_InitProcName(fc_CsiNameTab, &fc_CsiNameTabMax);
	}

	memcpy(m_LocalProc[0], fc_Proc[STAGE_ESC], sizeof(m_LocalProc[0]));
	memcpy(m_LocalProc[1], fc_Proc[STAGE_CSI], sizeof(m_LocalProc[1]));
	memcpy(m_LocalProc[2], fc_Proc[STAGE_EXT1], sizeof(m_LocalProc[2]));
	memcpy(m_LocalProc[3], fc_Proc[STAGE_EXT2], sizeof(m_LocalProc[3]));
	memcpy(m_LocalProc[4], fc_Proc[STAGE_EXT3], sizeof(m_LocalProc[4]));

	m_CsiExt.RemoveAll();
	for ( i = 0 ; fc_CsiExtTab[i].proc != NULL ; i++ ) {
		b = 0;
		m = m_CsiExt.GetSize() - 1;
		while ( b <= m ) {
			n = (b + m) / 2;
			if ( (c = fc_CsiExtTab[i].code - m_CsiExt[n].code) == 0 )
				break;
			else if ( c < 0 )
				b = n + 1;
			else
				m = n - 1;
		}
		if ( b > m )
			m_CsiExt.InsertAt(b, (CTextRam::CSIEXTTAB)(fc_CsiExtTab[i]));
	}

	for ( n = 0 ; n < m_ProcTab.GetSize() ; n++ ) {
		switch(m_ProcTab[n].m_Type) {
		case 0:	// ESC
			EscNameProc(m_ProcTab[n].m_Code, m_ProcTab[n].m_Name);
			break;
		case 1:	// CSI
			CsiNameProc(m_ProcTab[n].m_Code, m_ProcTab[n].m_Name);
			break;
		}
	}

	m_RetSync = FALSE;
	m_OscFlag = FALSE;
	m_StPos = 0;

	switch(mode) {
	case EUC_SET:
		fc_Case(STAGE_EUC);
		break;
	case SJIS_SET:
		fc_Case(STAGE_SJIS);
		break;
	case ASCII_SET:
		fc_Case(STAGE_EUC);
		break;
	case UTF8_SET:
		fc_Case(STAGE_UTF8);
		break;
	}
}
inline void CTextRam::fc_Case(int stage)
{
	if ( (m_Stage = stage) <= STAGE_EXT3 )
		m_Func = m_LocalProc[stage];
	else
		m_Func = fc_Proc[stage];
}
inline void CTextRam::fc_Push(int stage)
{
	ASSERT(m_StPos < 16);
	m_Stack[m_StPos++] = m_Stage;
	fc_Case(stage);
}

//////////////////////////////////////////////////////////////////////
// ESC/CSI Proc Name Func...

void CTextRam::EscNameProc(int ch, LPCSTR name)
{
	int n, c;
	int b = 0;
	int m = fc_EscNameTabMax - 1;
	void (CTextRam::*proc)(int) = &CTextRam::fc_POP;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = strcmp(fc_EscNameTab[n].name, name)) == 0 ) {
			proc = fc_EscNameTab[n].data.proc;
			break;
		} else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}

	m_LocalProc[0][ch] = proc;
	m_LocalProc[0][ch | 0x80] = proc;
}
LPCSTR	CTextRam::EscProcName(void (CTextRam::*proc)(int ch))
{
	int c;
	ESCNAMEPROC tmp;
	ESCNAMEPROC *tp;

	tmp.data.proc = proc;
	for ( tp = fc_pEscProc ; tp != NULL ; ) {
		if ( (c = memcmp(tp->data.byte, tmp.data.byte, sizeof(void (CTextRam::*)(int)))) == 0 )
			return tp->name;
		else if ( c < 0 )
			tp = tp->left;
		else
			tp = tp->right;
	}
	return "NOP";
}
void CTextRam::SetEscNameCombo(CComboBox *pCombo)
{
	for ( int n = 0 ; n < fc_EscNameTabMax ; n++ )
		pCombo->AddString(fc_EscNameTab[n].name);
}

void CTextRam::CsiNameProc(int code, LPCSTR name)
{
	int n, c;
	int b = 0;
	int m = fc_CsiNameTabMax - 1;
	void (CTextRam::*proc)(int) = &CTextRam::fc_POP;
	CSIEXTTAB tmp;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = strcmp(fc_CsiNameTab[n].name, name)) == 0 ) {
			proc = fc_CsiNameTab[n].data.proc;
			break;
		} else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}

	switch(code & 0x7F7F00) {
	case 0:
		m_LocalProc[1][code & 0x7F] = proc;
		m_LocalProc[1][code & 0x7F | 0x80] = proc;
		break;
	case '?' << 16:
		m_LocalProc[2][code & 0x7F] = proc;
		m_LocalProc[2][code & 0x7F | 0x80] = proc;
		break;
	case '$' << 8:
		m_LocalProc[3][code & 0x7F] = proc;
		m_LocalProc[3][code & 0x7F | 0x80] = proc;
		break;
	case ' ' << 8:
		m_LocalProc[4][code & 0x7F] = proc;
		m_LocalProc[4][code & 0x7F | 0x80] = proc;
		break;
	default:
		b = 0;
		m = m_CsiExt.GetSize() - 1;
		while ( b <= m ) {
			n = (b + m) / 2;
			if ( (c = code - m_CsiExt[n].code) == 0 ) {
				m_CsiExt[n].proc = proc;
				break;
			} else if ( c < 0 )
				b = n + 1;
			else
				m = n - 1;
		}
		if ( b > m ) {
			tmp.code = code;
			tmp.proc = proc;
			m_CsiExt.InsertAt(b, tmp);
		}
		break;
	}
}
LPCSTR	CTextRam::CsiProcName(void (CTextRam::*proc)(int ch))
{
	int c;
	ESCNAMEPROC tmp;
	ESCNAMEPROC *tp;

	tmp.data.proc = proc;
	for ( tp = fc_pCsiProc ; tp != NULL ; ) {
		if ( (c = memcmp(tp->data.byte, tmp.data.byte, sizeof(void (CTextRam::*)(int)))) == 0 )
			return tp->name;
		else if ( c < 0 )
			tp = tp->left;
		else
			tp = tp->right;
	}
	return "NOP";
}
void CTextRam::SetCsiNameCombo(CComboBox *pCombo)
{
	for ( int n = 0 ; n < fc_CsiNameTabMax ; n++ )
		pCombo->AddString(fc_CsiNameTab[n].name);
}
void CTextRam::EscCsiDefName(LPCSTR *esc, LPCSTR *csi)
{
	int n, c;

	for ( n = 0 ; n < 95 ; n++ )		// SP - ~ = 95
		esc[n] = "NOP";

	for ( n = 0 ; fc_Esc2Tab[n].proc != NULL ; n++ )
		esc[fc_Esc2Tab[n].sc - ' '] = EscProcName(fc_Esc2Tab[n].proc);

	for ( n = 0 ; n < 5355 ; n++ )		// (@ - ~) x (SP - / + 1) x (< - ? + 1) = 63 * 17 * 5 = 5355
		csi[n] = "NOP";

	for ( n = 0 ; fc_AnsiTab[n].proc != NULL ; n++ )
		csi[fc_AnsiTab[n].sc - '@'] = CsiProcName(fc_AnsiTab[n].proc);

	for ( n = 0 ; fc_Ext1Tab[n].proc != NULL ; n++ )		// ('?' << 16) | sc
		csi[fc_Ext1Tab[n].sc - '@' + 63 * 17 * 4] = CsiProcName(fc_Ext1Tab[n].proc);

	for ( n = 0 ; fc_Ext2Tab[n].proc != NULL ; n++ )		// ('$' << 8) | sc
		csi[fc_Ext2Tab[n].sc - '@' + 63 * 5] = CsiProcName(fc_Ext2Tab[n].proc);

	for ( n = 0 ; fc_Ext3Tab[n].proc != NULL ; n++ )		// (' ' << 8) | sc
		csi[fc_Ext3Tab[n].sc - '@' + 63 * 1] = CsiProcName(fc_Ext3Tab[n].proc);

	for ( n = 0 ; fc_CsiExtTab[n].proc != NULL ; n++ ) {
		c = (fc_CsiExtTab[n].code & 0x7F) - '@';
		if ( (fc_CsiExtTab[n].code & 0x7F0000) != 0 )
			c += ((((fc_CsiExtTab[n].code >> 16) & 0x7F) - '<' + 1) * 63 * 17);
		if ( (fc_CsiExtTab[n].code & 0x007F00) != 0 )
			c += ((((fc_CsiExtTab[n].code >>  8) & 0x7F) - ' ' + 1) * 63);
		csi[c] = CsiProcName(fc_CsiExtTab[n].proc);
	}
}

//////////////////////////////////////////////////////////////////////
// fc Procs...

void CTextRam::fc_IGNORE(int ch)
{
	fc_KANJI(ch);
	if ( ch < 0x20 )
		CallReciveChar(ch);
}
void CTextRam::fc_POP(int ch)
{
	if ( ch < 0x20 )
		CallReciveChar(ch);
	if ( m_StPos > 0 )
		fc_Case(m_Stack[--m_StPos]);
}
void CTextRam::fc_RETRY(int ch)
{
	if ( m_StPos > 0 ) {
		fc_Case(m_Stack[--m_StPos]);
		fc_Call(ch);
	}
}
void CTextRam::fc_SESC(int ch)
{
	fc_KANJI(ch);
	CallReciveChar(0x1B);
	ch &= 0x1F;
	ch += '@';
	fc_Push(STAGE_ESC);
	fc_Call(ch);
}

//////////////////////////////////////////////////////////////////////
// fc KANJI

void CTextRam::fc_KANCHK()
{
	int n, ch;
	BOOL skip = FALSE;
	int sjis_st = 0, sjis_rs = 0;
	int euc_st  = 0, euc_rs  = 0;
	int utf8_st = 0, utf8_rs = 0;

	for ( n = m_Kan_Pos + 1; n != m_Kan_Pos ; n = (n + 1) & (KANBUFMAX - 1) ) {
		ch = m_Kan_Buf[n];

		if ( !skip && (ch & 0x80) != 0 )
			continue;
		skip = TRUE;

		// SJIS
		// 1 Byte	0x81 - 0x9F or 0xE0 - 0xFC or 0xA0-0xDF
		// 2 Byte	0x40 - 0x7E or 0x80 - 0xFC
		switch(sjis_st) {
		case 0:
			if ( issjis1(ch) )
				sjis_st = 1;
			else if ( iskana(ch) )
				sjis_rs |= 004;
			else if ( (ch & 0x80) != 0 )
				sjis_rs |= (sjis_rs != 0 ? 002 : 000);
			else
				sjis_rs |= 004;
			break;
		case 1:
			sjis_st = 0;
			if ( issjis2(ch) )
				sjis_rs |= 001;
			else
				sjis_rs |= 002;
			break;
		}

		// EUC
		// 1 Byte	0xA0 - 0xFF or 0x8E		or 0x8F
		// 2 Byte	0xA0 - 0xFF
		// 3 Byte							0xA0 - 0xFF

		switch(euc_st) {
		case 0:
			if ( ch >= 0xA0 )
				euc_st = 1;
			else if ( ch == 0x8E )
				euc_st = 1;
			else if ( ch == 0x8F )
				euc_st = 2;
			else if ( (ch & 0x80) != 0 )
				euc_rs |=  (euc_rs != 0 ? 002 : 000);
			else
				euc_rs |= 004;
			break;
		case 1:
			euc_st = 0;
			if ( ch >= 0xA0 )
				euc_rs |= 001;
			else
				euc_rs |= 002;
			break;
		case 2:
			if ( ch >= 0xA0 )
				euc_st = 1;
			else {
				euc_st = 0;
				euc_rs |= 002;
			}
			break;
		}

		// UTF-8
		// 1 Byte	0xC0 - 0xDF(2 Byte) or	0xE0 - 0xEF(3 Byte) or	0xF0 - 0xF7(4 Byte)	or	0xFE - 0xFF(2 Byte)
		// 2 Byte	0x80 - 0xBF

		switch(utf8_st) {
		case 0:
			if ( ch >= 0xC0 && ch <= 0xDF )
				utf8_st = 3;
			else if ( ch >= 0xE0 && ch <= 0xEF )
				utf8_st = 2;
			else if ( ch >= 0xF0 && ch <= 0xF7 )
				utf8_st = 1;
			else if ( ch >= 0xFE && ch <= 0xFF )
				utf8_st = 4;
			else if ( (ch & 0x80) != 0 )
				utf8_rs |= (utf8_rs != 0 ? 002 : 000);
			else
				utf8_rs |= 004;
			break;
		case 1:
			if ( ch >= 0x80 && ch <= 0xBF )
				utf8_st = 2;
			else {
				utf8_st = 0;
				utf8_rs |= 002;
			}
			break;
		case 2:
			if ( ch >= 0x80 && ch <= 0xBF )
				utf8_st = 3;
			else {
				utf8_st = 0;
				utf8_rs |= 002;
			}
			break;
		case 3:
			utf8_st = 0;
			if ( ch >= 0x80 && ch <= 0xBF )
				utf8_rs |= 001;
			else
				utf8_rs |= 002;
			break;
		case 4:
			utf8_st = 0;
			if ( ch >= 0xFE && ch <= 0xFF )
				utf8_rs |= 001;
			else
				utf8_rs |= 002;
			break;
		}
	}

	n = m_KanjiMode;
	sjis_rs &= 3;
	euc_rs  &= 3;
	utf8_rs &= 3;

	if ( sjis_rs == 1 && euc_rs != 1 && utf8_rs != 1 )
		n = SJIS_SET;
	else if ( sjis_rs != 1 && euc_rs == 1 && utf8_rs != 1 )
		n = EUC_SET;
	if ( sjis_rs != 1 && euc_rs != 1 && utf8_rs == 1 )
		n = UTF8_SET;

	if ( m_KanjiMode != n )
		SetKanjiMode(n);
}

//////////////////////////////////////////////////////////////////////
// fc Print

void CTextRam::fc_TEXT(int ch)
{
	fc_KANJI(ch);

	if ( m_BankSG >= 0 ) {
		m_BankNow = m_BankTab[m_KanjiMode][m_BankSG];
		m_BankSG = (-1);
	} else
		m_BankNow = m_BankTab[m_KanjiMode][(ch & 0x80) == 0 ? m_BankGL : m_BankGR];

	m_BackChar = ch;

	switch(m_BankNow & SET_MASK) {
	case SET_94:
		ch &= 0x7F;
		if ( ch >= 0x21 && ch <= 0x7E ) {
			INSMDCK(1);
			PUT1BYTE(ch, m_BankNow);
		} else if ( ch == 0x20 ) {
			INSMDCK(1);
			PUT1BYTE(0x20, m_BankNow);
		}
		break;
	case SET_96:
		ch &= 0x7F;
		if ( ch >= 0x20 && ch <= 0x7F ) {
			INSMDCK(1);
			PUT1BYTE(ch, m_BankNow);
		}
		break;
	case SET_94x94:
		if ( (ch & 0x7F) < 0x21 || (ch & 0x7F) > 0x7E )
			break;
		fc_Push(STAGE_94X94);
		goto CODELEN;
	case SET_96x96:
		if ( (ch & 0x7F) < 0x21 || (ch & 0x7F) > 0x7E )
			break;
		fc_Push(STAGE_96X96);
	CODELEN:
		switch(m_BankNow & CODE_MASK) {
		case 0x00:
		case 0x10: m_CodeLen = 2 - 1; break;
		case 0x20: m_CodeLen = 3 - 1; break;
		case 0x30: m_CodeLen = 4 - 1; break;
		}
		break;
	}
}
void CTextRam::fc_TEXT2(int ch)
{
	fc_KANJI(ch);

	m_BackChar = (m_BackChar << 8) | ch;
	if ( --m_CodeLen <= 0 ) {
		INSMDCK(2);
		PUT2BYTE(m_BackChar & 0x7F7F7F7F, m_BankNow);
	}
	fc_POP(ch);
}
void CTextRam::fc_TEXT3(int ch)
{
	fc_POP(ch);

	if ( ch < 0x20 )
		fc_Call(ch);
	else
		fc_KANJI(ch);

	if ( IsOptEnable(TO_RLKANAUTO) )
		fc_KANCHK();
}
void CTextRam::fc_SJIS1(int ch)
{
	fc_KANJI(ch);

	m_BackChar = ch;
	m_BankNow  = m_BankTab[m_KanjiMode][2];
	fc_Push(STAGE_SJIS2);
}
void CTextRam::fc_SJIS2(int ch)
{
	int n;

	fc_KANJI(ch);

	m_BackChar = (m_BackChar << 8) | ch;
	if ( (n = m_IConv.IConvChar(m_SendCharSet[SJIS_SET], m_FontTab[m_BankNow].m_IContName, m_BackChar)) == 0 ) {
		m_BankNow = m_BankTab[m_KanjiMode][3];
		if ( (n = m_IConv.IConvChar(m_SendCharSet[SJIS_SET], m_FontTab[m_BankNow].m_IContName, m_BackChar)) == 0 ) {
			m_BankNow = m_BankTab[m_KanjiMode][2];
			n = 0x2222;
		}
	}
	INSMDCK(2);
	PUT2BYTE(n, m_BankNow);
	fc_POP(ch);
}
void CTextRam::fc_SJIS3(int ch)
{
	fc_POP(ch);

	if ( ch < 0x20 )
		fc_Call(ch);
	else
		fc_KANJI(ch);

	if ( IsOptEnable(TO_RLKANAUTO) )
		fc_KANCHK();
}
void CTextRam::fc_UTF81(int ch)
{
	// 110x xxxx
	fc_KANJI(ch);
	m_BackChar = (ch & 0x1F) << 6;
	m_CodeLen = 1;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF82(int ch)
{
	// 1110 xxxx
	fc_KANJI(ch);
	m_BackChar = (ch & 0x0F) << 12;
	m_CodeLen = 2;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF83(int ch)
{
	// 1111 0xxx
	fc_KANJI(ch);
	m_BackChar = (ch & 0x07) << 18;
	m_CodeLen = 3;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF88(int ch)
{
	// 1111 10xx
	fc_KANJI(ch);
	m_BackChar = (ch & 0x03) << 24;
	m_CodeLen = 4;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF89(int ch)
{
	// 1111 110x
	fc_KANJI(ch);
	m_BackChar = (ch & 0x01) << 30;
	m_CodeLen = 5;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF84(int ch)
{
	// 1111 111x BOM
	fc_KANJI(ch);
	m_BackChar = ch;
	m_CodeLen = 0;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF85(int ch)
{
	// 10xx xxxx
	int n;

	fc_KANJI(ch);

	switch(m_CodeLen) {
	case 5:
		m_BackChar |= (ch & 0x3F) << 24;
		m_CodeLen--;
		break;
	case 4:
		m_BackChar |= (ch & 0x3F) << 18;
		m_CodeLen--;
		break;
	case 3:
		m_BackChar |= (ch & 0x3F) << 12;
		m_CodeLen--;
		break;
	case 2:
		m_BackChar |= (ch & 0x3F) << 6;
		m_CodeLen--;
		break;
	case 1:
		m_BackChar |= (ch & 0x3F);
		if ( (n = UnicodeNomal(m_LastChar, m_BackChar)) != 0 ) {
			m_BackChar = n;
			if ( m_OscFlag ) {
				if ( (n = m_OscPara.GetLength()) > 0 )
					m_OscPara.Delete(n - 1);
			} else
				LOCATE(m_LastPos % COLS_MAX, m_LastPos / COLS_MAX);
		}
		if ( UnicodeWidth(m_BackChar) == 1 ) {
			INSMDCK(1);
			if ( m_BackChar < 0x0080 )
				PUT1BYTE(m_BackChar & 0x7F, m_BankTab[m_KanjiMode][0]);
			else if ( m_BackChar < 0x0100 )
				PUT1BYTE(m_BackChar & 0x7F, m_BankTab[m_KanjiMode][1]);
			else
				PUT1BYTE(m_BackChar, SET_UNICODE);
		} else {
			INSMDCK(2);
			PUT2BYTE(m_BackChar, SET_UNICODE);
		}
		m_BankNow  = SET_UNICODE;
	case 0:
		fc_POP(ch);
		break;
	}
}
void CTextRam::fc_UTF86(int ch)
{
	fc_POP(ch);

	if ( ch < 0x20 )
		fc_Call(ch);
	else
		fc_KANJI(ch);

	if ( IsOptEnable(TO_RLKANAUTO) )
		fc_KANCHK();
}
void CTextRam::fc_UTF87(int ch)
{
	fc_KANJI(ch);

	if ( IsOptEnable(TO_RLKANAUTO) )
		fc_KANCHK();
}

//////////////////////////////////////////////////////////////////////
// fc CTRLs...

void CTextRam::fc_SOH(int ch)
{
	if ( IsOptEnable(TO_RLBPLUS) )
		m_RetSync = TRUE;
	CallReciveChar(ch);
}
void CTextRam::fc_ENQ(int ch)
{
	if ( IsOptEnable(TO_RLBPLUS) )
		m_RetSync = TRUE;
	CallReciveChar(ch);
}
void CTextRam::fc_BEL(int ch)
{
	BEEP();
	CallReciveChar(ch);
}
void CTextRam::fc_BS(int ch)
{
	fc_KANJI(ch);
	if ( --m_CurX < 0 ) {
		if ( IS_ENABLE(m_AnsiOpt, TO_XTMRVW) != 0 ) {
			m_CurX = m_Cols - 1;
			if ( --m_CurY < 0 )
				m_CurY = 0;
		} else
			m_CurX = 0;
	}
	m_DoWarp = FALSE;
	CallReciveChar(ch);
}
void CTextRam::fc_HT(int ch)
{
	fc_KANJI(ch);
	TABSET(TAB_COLSNEXT);
	CallReciveChar(ch);
}
void CTextRam::fc_LF(int ch)
{
	fc_KANJI(ch);

	switch(IsOptEnable(TO_ANSILNM) ? 2 : m_RecvCrLf) {
	case 2:		// LF
	case 3:		// CR|LF
		LOCATE(0, m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag )
			m_pDocument->OnDelayRecive(ch);
	case 0:		// CR+LF
		ONEINDEX();
	case 1:		// CR
		break;
	}
	CallReciveChar(ch);
}
void CTextRam::fc_VT(int ch)
{
	TABSET(TAB_LINENEXT);
	if ( IsOptEnable(TO_ANSILNM) )
		LOCATE(0, m_CurY);
	CallReciveChar(ch);
}
void CTextRam::fc_FF(int ch)
{
	for ( int n = 0 ; n < m_Lines ; n++ )
		FORSCROLL(0, m_Lines);
	if ( IsOptEnable(TO_ANSILNM) )
		LOCATE(0, m_CurY);
	CallReciveChar(ch);
}
void CTextRam::fc_CR(int ch)
{
	fc_KANJI(ch);

	switch(m_RecvCrLf) {
	case 1:		// CR
	case 3:		// CR|LF
		ONEINDEX();
	case 0:		// CR+LF
		LOCATE(0, m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag )
			m_pDocument->OnDelayRecive(ch);
	case 2:		// LF
		break;
	}
	CallReciveChar(ch);
}
void CTextRam::fc_SO(int ch)
{
	m_BankGL = 1;
	CallReciveChar(ch);
}
void CTextRam::fc_SI(int ch)
{
	m_BankGL = 0;
	CallReciveChar(ch);
}
void CTextRam::fc_DLE(int ch)
{
	if ( IsOptEnable(TO_RLBPLUS) )
		m_RetSync = TRUE;
	CallReciveChar(ch);
}
void CTextRam::fc_CAN(int ch)
{
	if ( m_LastChar == '*' && IsOptEnable(TO_RLBPLUS) )
		m_RetSync = TRUE;
	CallReciveChar(ch);
}
void CTextRam::fc_ESC(int ch)
{
	fc_KANJI(ch);
	fc_Push(STAGE_ESC);
	CallReciveChar(ch);
}
void CTextRam::fc_A3CRT(int ch)
{
	// ADM-3 Cursole Right
	LOCATE(m_CurX + 1, m_CurY);
	CallReciveChar(ch);
}
void CTextRam::fc_A3CLT(int ch)
{
	// ADM-3 Cursole Left
	LOCATE(m_CurX - 1, m_CurY);
	CallReciveChar(ch);
}
void CTextRam::fc_A3CUP(int ch)
{
	// ADM-3 Cursole Up
	LOCATE(m_CurX, m_CurY - 1);
	CallReciveChar(ch);
}
void CTextRam::fc_A3CDW(int ch)
{
	// ADM-3 Cursole Down
	LOCATE(m_CurX, m_CurY + 1);
	CallReciveChar(ch);
}

//////////////////////////////////////////////////////////////////////
// fc ESC

void CTextRam::fc_DECHTS(int ch)
{
	// ESC 1	DECHTS Horizontal tab set
	TABSET(TAB_COLSSET);
	fc_POP(ch);
}
void CTextRam::fc_CCAHT(int ch)
{
	// ESC 2	DECCAHT Clear all horizontal tabs
	TABSET(TAB_COLSALLCLR);
	fc_POP(ch);
}
void CTextRam::fc_DECVTS(int ch)
{
	// ESC 3	DECVTS Vertical tab set
	TABSET(TAB_LINESET);
	fc_POP(ch);
}
void CTextRam::fc_CAVT(int ch)
{
	// ESC 4	DECCAVT Clear all vertical tabs
	TABSET(TAB_LINEALLCLR);
	fc_POP(ch);
}
void CTextRam::fc_BI(int ch)
{
	// ESC 6	DECBI Back Index
	if ( m_CurX == 0 )
		RIGHTSCROLL();
	else
		LOCATE(m_CurX - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_SC(int ch)
{
	// ESC 7	DECSC Save Cursor
	m_Save_CurX   = m_CurX;
	m_Save_CurY   = m_CurY;
	m_Save_AttNow = m_AttNow;
	m_Save_AttSpc = m_AttSpc;
	m_Save_BankGL = m_BankGL;
	m_Save_BankGR = m_BankGR;
	m_Save_BankSG = m_BankSG;
	m_Save_DoWarp = m_DoWarp;
	memcpy(m_Save_BankTab, m_BankTab, sizeof(m_BankTab));
	memcpy(m_Save_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_Save_TabMap, m_TabMap, sizeof(m_TabMap));
	fc_POP(ch);
}
void CTextRam::fc_RC(int ch)
{
	// ESC 8	DECRC Restore Cursor
	m_CurX   = m_Save_CurX;
	m_CurY   = m_Save_CurY;
	m_AttNow = m_Save_AttNow;
	m_AttSpc = m_Save_AttSpc;
	m_BankGL = m_Save_BankGL;
	m_BankGR = m_Save_BankGR;
	m_BankSG = m_Save_BankSG;
	m_DoWarp = m_Save_DoWarp;
	memcpy(m_BankTab, m_Save_BankTab, sizeof(m_BankTab));
	memcpy(m_AnsiOpt, m_Save_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_TabMap, m_Save_TabMap, sizeof(m_TabMap));
	fc_POP(ch);
}
void CTextRam::fc_FI(int ch)
{
	// ESC 9	DECFI Forward Index
	if ( (m_CurX + 1) >= m_Cols )
		LEFTSCROLL();
	else
		LOCATE(m_CurX + 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_V5CUP(int ch)
{
	// ESC A	VT52 Cursor up.
	LOCATE(m_CurX, m_CurY - 1);
	fc_POP(ch);
}
void CTextRam::fc_BPH(int ch)
{
	// ESC B	VT52 Cursor down								ANSI BPH Break permitted here
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(m_CurX, m_CurY + 1);
	fc_POP(ch);
}
void CTextRam::fc_NBH(int ch)
{
	// ESC C	VT52 Cursor right								ANSI NBH No break here
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(m_CurX + 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_IND(int ch)
{
	// ESC D	VT52 Cursor left								ANSI IND Index
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(m_CurX - 1, m_CurY);
	else
		ONEINDEX();
	fc_POP(ch);
}
void CTextRam::fc_NEL(int ch)
{
	// ESC E													ANSI NEL Next Line
	ONEINDEX();
	LOCATE(0, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_SSA(int ch)
{
	// ESC F	VT52 Enter graphics mode.						ANSI SSA Start selected area
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) ) {
		m_BankTab[m_KanjiMode][0] = SET_94 | '0';
		m_BankTab[m_KanjiMode][1] = SET_94 | '0';
	} else
		m_AttNow.em |= EM_SELECTED;
	fc_POP(ch);
}
void CTextRam::fc_ESA(int ch)
{
	// ESC F	VT52 Exit graphics mode.						ANSI ESA End selected area
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) ) {
		m_BankTab[m_KanjiMode][0] = SET_94 | 'B';
		m_BankTab[m_KanjiMode][1] = SET_94 | 'B';
	} else
		m_AttNow.em &= ~EM_SELECTED;
	fc_POP(ch);
}
void CTextRam::fc_HTS(int ch)
{
	// ESC H	VT52 Cursor to home position.					ANSI HTS Character tabulation set
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(0, 0);
	else
		TABSET(TAB_COLSSET);
	fc_POP(ch);
}
void CTextRam::fc_HTJ(int ch)
{
	// ESC I	VT52 Reverse line feed.							ANSI HTJ Character tabulation with justification
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		REVINDEX();
	else
		TABSET(TAB_COLSNEXT);
	fc_POP(ch);
}
void CTextRam::fc_VTS(int ch)
{
	// ESC J	VT52 Erase from cursor to end of screen.		ANSI VTS Line tabulation set
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) ) {
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, 1);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines, 1);
	} else
		TABSET(TAB_LINESET);
	fc_POP(ch);
}
void CTextRam::fc_PLD(int ch)
{
	// ESC K	VT52 Erase from cursor to end of line.			ANSI PLD Partial line forward
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
	else
		LOCATE(m_CurX, m_CurY + 1);
	fc_POP(ch);
}
void CTextRam::fc_PLU(int ch)
{
	// ESC L													ANSI PLU Partial line backward
	LOCATE(m_CurX, m_CurY - 1);
	fc_POP(ch);
}
void CTextRam::fc_RI(int ch)
{
	// ESC M													ANSI RI Reverse index
	REVINDEX();
	fc_POP(ch);
}
void CTextRam::fc_STS(int ch)
{
	// ESC S													ANSI STS Set transmit state
	fc_POP(ch);
}
void CTextRam::fc_CCH(int ch)
{
	// ESC T													ANSI CCH Cancel character
	fc_POP(ch);
}
void CTextRam::fc_MW(int ch)
{
	// ESC U													ANSI MW Message waiting
	fc_POP(ch);
}
void CTextRam::fc_SPA(int ch)
{
	// ESC V	VT52 Print the line with the cursor.			ANSI SPA Start of guarded area
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		m_AttNow.em |= EM_PROTECT;
	fc_POP(ch);
}
void CTextRam::fc_EPA(int ch)
{
	// ESC W	VT52 Enter printer controller mode.				ANSI EPA End of guarded area
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		m_AttNow.em &= ~EM_PROTECT;
	fc_POP(ch);
}
void CTextRam::fc_SCI(int ch)
{
	// ESC Z	VT52 Identify (host to terminal).				ANSI SCI Single character introducer
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		UNGETSTR("\033/Z");
	fc_POP(ch);
}
void CTextRam::fc_RIS(int ch)
{
	// ESC c													ANSI RIS Reset to initial state
	RESET();
	fc_POP(ch);
}
void CTextRam::fc_LMA(int ch)
{
	// ESC l	HP fc_LMA LOCK
	m_TopY = m_CurY;
	fc_POP(ch);
}
void CTextRam::fc_USR(int ch)
{
	// ESC m	HP USR UNLOCK
	m_TopY = 0;
	fc_POP(ch);
}
void CTextRam::fc_V5EX(int ch)
{
	// ESC <	VT52 Exit VT52 mode. Enter VT100 mode.
	m_AnsiOpt[TO_DECANM / 32] |= (1 << (TO_DECANM % 32));
	fc_POP(ch);
}
void CTextRam::fc_DECPAM(int ch)
{
	// ESC =	DECPAM Application Keypad				VT52 Enter alternate keypad mode.
	m_AnsiOpt[TO_DECPAM / 32] |= (1 << (TO_DECPAM % 32));
	fc_POP(ch);
}
void CTextRam::fc_DECPNM(int ch)
{
	// ESC >	DECPNM Normal Keypad						VT52 Exit alternate keypad mode.
	m_AnsiOpt[TO_DECPAM / 32] &= ~(1 << (TO_DECPAM % 32));
	fc_POP(ch);
}
void CTextRam::fc_SS2(int ch)
{
	// ESC O	SS2 Single shift 2
	m_BankSG = 2;
	fc_POP(ch);
}
void CTextRam::fc_SS3(int ch)
{
	// ESC P	SS3 Single shift 3
	m_BankSG = 3;
	fc_POP(ch);
}
void CTextRam::fc_LS2(int ch)
{
	// ESC n	LS2 Locking-shift two
	m_BankGL = 2;
	fc_POP(ch);
}
void CTextRam::fc_LS3(int ch)
{
	// ESC o	LS3 Locking-shift three
	m_BankGL = 3;
	fc_POP(ch);
}
void CTextRam::fc_LS1R(int ch)
{
	// ESC ~	LS1R
	m_BankGR = 1;
	fc_POP(ch);
}
void CTextRam::fc_LS2R(int ch)
{
	// ESC }	LS2R
	m_BankGR = 2;
	fc_POP(ch);
}
void CTextRam::fc_LS3R(int ch)
{
	// ESC |	LS3R
	m_BankGR = 3;
	fc_POP(ch);
}
void CTextRam::fc_CSC0W(int ch)
{
	// ESC $	CSC0W Char Set
	m_BackChar = 0;
	m_BackMode = SET_94x94;
	m_Status = ST_CHARSET_2;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC0(int ch)
{
	// ESC (	CSC0 G0 charset
	m_BackChar = 0;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC1(int ch)
{
	// ESC )	CSC1 G1 charset
	m_BackChar = 1;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC2(int ch)
{
	// ESC *	CSC2 G2 charset
	m_BackChar = 2;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC3(int ch)
{
	// ESC +	CSC3 G3 charset
	m_BackChar = 3;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC0A(int ch)
{
	// ESC ,	CSC0A G0 charset
	m_BackChar = 0;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC1A(int ch)
{
	// ESC -	CSC1A G1 charset
	m_BackChar = 1;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC2A(int ch)
{
	// ESC .	CSC2A G2 charset
	m_BackChar = 2;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC3A(int ch)
{
	// ESC /	CSC3A G3 charset
	m_BackChar = 3;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_V5MCP(int ch)
{
	// ESC A	VT52 Move cursor to column Pn.
	m_Status = ST_COLM_SET;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_DECSOP(int ch)
{
	// ESC #	DEC Screen Opt
	m_Status = ST_DEC_OPT;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_ACS(int ch)
{
	// ESC 20	ACS
	m_Status = ST_CONF_LEVEL;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_DOCS(int ch)
{
	// ESC %	DOCS Designate other coding system
	m_Status = ST_DOCS_OPT;
	fc_Case(STAGE_STAT);
}

//////////////////////////////////////////////////////////////////////
// fc ESC ...

void CTextRam::fc_STAT(int ch)
{
	int n, i;
	VRAM *vp;

	switch(m_Status) {

	case ST_CONF_LEVEL:			// ESC 20...	ACS
		m_Status = ST_NON;
		switch(ch) {
		case 'L':	// Level 1		G0=ASCII G1=Latan GL=G0 GR=G1
		case 'M':	// Level 2		G0=ASCII G1=Latan GL=G0 GR=G1
			m_BankTab[m_KanjiMode][0] = SET_94 | 'B';
			m_BankTab[m_KanjiMode][1] = SET_96 | 'A';
			m_BankGL = 0;
			m_BankGR = 1;
			break;
		case 'N':	// Level 3		G0=ASCII GL=G0
			m_BankTab[m_KanjiMode][0] = SET_94 | 'B';
			m_BankGL = 0;
			break;
		}
		break;

	case ST_COLM_SET:			// ESC Y...		V5MCP
		m_Status = ST_COLM_SET_2;
		m_BackChar = ch;
		break;
	case ST_COLM_SET_2:
		m_Status = ST_NON;
		LOCATE(ch - ' ', m_BackChar - ' ');
		break;

	case ST_DEC_OPT:			// ESC #...
		m_Status = ST_NON;
		switch(ch) {
		case '3':		// DECDHL Double-width, double-height line (top half)
			SetDm(m_CurY, DM_DWDHT);
			LOCATE(m_CurX, m_CurY);
			DISPVRAM(0, m_CurY, m_Cols, 1);
			break;
		case '4':		// DECDHL Double-width, double-height line (bottom half)
			SetDm(m_CurY, DM_DWDHB);
			LOCATE(m_CurX, m_CurY);
			DISPVRAM(0, m_CurY, m_Cols, 1);
			break;
		case '5':		// DECSWL Single-width, single-height line
			SetDm(m_CurY, DM_SWSH);
			DISPVRAM(0, m_CurY, m_Cols, 1);
			break;
		case '6':		// DECDWL Double-width, single-height line
			SetDm(m_CurY, DM_DWSH);
			LOCATE(m_CurX, m_CurY);
			DISPVRAM(0, m_CurY, m_Cols, 1);
			break;
		case '7':		// DECHCP Hardcopy
			break;
		case '8':		// DECALN Screen Alignment Pattern
			for ( n = 0 ; n < m_Lines ; n++ ) {
				vp = GETVRAM(0, n);
				for ( i = 0 ; i < m_Cols ; i++ ) {
					*vp = m_AttNow;
					vp->md = m_BankTab[m_KanjiMode][0];
					vp->ch = 'E';
					vp++;
				}
			}
			DISPVRAM(0, 0, m_Cols, m_Lines);
			break;
		case '9':		// DECFPP Perform pending motion
			break;
		}
		break;


	case ST_DOCS_OPT:			// ESC %...		DOCS
		m_Status = ST_NON;
		switch(ch) {
		case '%':
			m_Status = ST_DOCS_OPT;
			break;
		case '!':	// tek410x mode switch
			m_Status = ST_TEK_OPT;
			m_Tek_Int = 0;
			break;
		case '/':
			m_Status = ST_DOCS_OPT_2;
			break;
		case '8':	// LINUX switch to UTF-8
		case '@':	// ECMA-35 return to ECMA-35 coding system
		case 'A':	// switch to CSA T 500-1983
		case 'B':	// switch to UCS Transformation Format One (UTF-1)
		case 'C':	// switch to Data Syntax I of CCITT Rec.T.101 
		case 'D':	// switch to Data Syntax II of CCITT Rec.T.101
		case 'E':	// switch to Photo-Videotex Data Syntax of CCITT Rec. T.101
		case 'F':	// switch to Audio Data Syntax of CCITT Rec. T.101
		case 'G':	// switch to UTF-8
		case 'H':	// switch to VEMMI Data Syntax of ITU-T Rec. T.107
			break;
		}
		break;
	case ST_DOCS_OPT_2:
		m_Status = ST_NON;
		switch(ch) {
		case '@':	// switch to ISO/IEC 10646:1993, UCS-2, Level 1
		case 'A':	// switch to ISO/IEC 10646:1993, UCS-4, Level 1
		case 'B':	// Switch to Virtual Terminal service Transparent Set
		case 'C':	// switch to ISO/IEC 10646:1993, UCS-2, Level 2
		case 'D':	// switch to ISO/IEC 10646:1993, UCS-4, Level 2
		case 'E':	// switch to ISO/IEC 10646:1993, UCS-2, Level 3
		case 'F':	// switch to ISO/IEC 10646:1993, UCS-4, Level 3
		case 'G':	// switch to UTF-8 Level 1
		case 'H':	// switch to UTF-8 Level 2
		case 'I':	// switch to UTF-8 Level 3
		case 'J':	// switch to UTF-16 Level 1
		case 'K':	// switch to UTF-16 Level 2
		case 'L':	// switch to UTF-16 Level 3
			break;
		}
		break;

	case ST_TEK_OPT:			// ESC %!...
		if ( ch >= 0x40 && ch <= 0x7F )
			m_Tek_Int = (m_Tek_Int << 6) | (ch & 0x3F);
		else if ( ch >= 0x20 && ch <= 0x3F ) {
			m_Tek_Int = (m_Tek_Int << 4) | (ch & 0x0F);
			if ( (ch & 0x10) == 0 )
				m_Tek_Int = 0 - m_Tek_Int;
			m_Status = ST_NON;
			if ( m_Tek_Int == 0 ) {		//  set tek mode
				TekInit(4105);
				fc_Case(STAGE_TEK);
				return;
			}
		} else
			m_Status = ST_NON;
		break;

	case ST_CHARSET_1:
		m_Status = ST_NON;
		m_BankTab[m_KanjiMode][m_BackChar] = m_BackMode | ch;
		break;

	case ST_CHARSET_2:
		switch(ch) {
		case '(': m_BackChar = 0; m_BackMode = SET_94x94; break;
		case ')': m_BackChar = 1; m_BackMode = SET_94x94; break;
		case '+': m_BackChar = 2; m_BackMode = SET_94x94; break;
		case '*': m_BackChar = 3; m_BackMode = SET_94x94; break;
		case ',': m_BackChar = 0; m_BackMode = SET_96x96; break;
		case '-': m_BackChar = 1; m_BackMode = SET_96x96; break;
		case '.': m_BackChar = 2; m_BackMode = SET_96x96; break;
		case '/': m_BackChar = 3; m_BackMode = SET_96x96; break;
		default:
			m_Status = ST_NON;
			m_BankTab[m_KanjiMode][m_BackChar] = m_BackMode | ch;
			break;
		}
		break;

	case ST_CSI_SKIP:
		if ( ch == 0x18 || ch == 0x1A || (ch >= 0x40 && ch <= 0x7F) || (ch >= 0xA0 && ch <= 0xFF) )
			m_Status = ST_NON;
		else if ( ch == 0x1B )
			fc_Case(STAGE_OSC4);
		break;
	}

	if ( m_Status == ST_NON )
		fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc DCS/SOS/APC/PM/OSC ...

void CTextRam::fc_DCS(int ch)
{
	fc_OSC_CH('P');
}
void CTextRam::fc_SOS(int ch)
{
	fc_OSC_CH('X');
}
void CTextRam::fc_APC(int ch)
{
	fc_OSC_CH('_');
}
void CTextRam::fc_PM(int ch)
{
	fc_OSC_CH('^');
}
void CTextRam::fc_OSC(int ch)
{
	fc_OSC_CH(']');
}
void CTextRam::fc_OSC_CH(int ch)
{
	m_OscMode = ch & 0x7F;
	m_OscPara.Empty();
	m_OscFlag = TRUE;

	switch(m_KanjiMode) {
	case EUC_SET:
		fc_Case(STAGE_OSC1);
		break;
	case SJIS_SET:
		fc_Case(STAGE_OSC2);
		break;
	case ASCII_SET:
		fc_Case(STAGE_OSC1);
		break;
	case UTF8_SET:
		fc_Case(STAGE_OSC3);
		break;
	}
}
void CTextRam::fc_OSC_POP(int ch)
{
	m_OscFlag = FALSE;
	fc_POP(ch);
}
void CTextRam::fc_OSC_ESC(int ch)
{
	fc_Push(STAGE_OSC4);
}
void CTextRam::fc_OSC_EST(int ch)
{
	if ( ch == 0x07 && m_OscMode != ']' )	// OSC
		return;
	fc_OSC_ST(ch);
	fc_POP(ch);
}
void CTextRam::fc_OSC_CSI(int ch)
{
	// OSC in CSI
	m_Status = ST_CSI_SKIP;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_OSC_ST(int ch)
{
	int n;
	CStringW tmp;
	WCHAR *p;

	if ( ch == 0x07 && m_OscMode != ']' )	// OSC
		return;

	switch(m_OscMode) {
	case 'P':	// DCS Device Control String
		p = (LPWSTR)(LPCWSTR)m_OscPara;
		m_BackChar = 0;
		m_AnsiPara.RemoveAll();
		m_AnsiPara.Add(0);
		while ( *p != L'\0' ) {
			if ( *p >= L'0' && *p <= L'9' ) {
				int n = (int)m_AnsiPara.GetSize() - 1;
				m_AnsiPara[n] = m_AnsiPara[n] * 10 + (*(p++) - L'0');
			} else if ( *p == L';' ) {
				m_AnsiPara.Add(0);
				p++;
			} else if ( *p >= 0x20 && *p <= 0x2F ) {
				m_BackChar &= 0xFF0000;
				m_BackChar |= ((*(p++) & 0x7F) << 8);
			} else if ( *p >= 0x3C && *p <= 0x3F ) {
				m_BackChar = ((*(p++) & 0x7F) << 16);
			} else if ( *p >= 0x40 && *p <= 0x7E ) {
				m_BackChar |= (*(p++) & 0x7F);
				break;
			} else if ( *p >= 0xC0 && *p <= 0xFE ) {
				m_BackChar |= (*(p++) & 0x7F);
				break;
			} else
				p++;
		}

		switch(m_BackChar) {
		case 'r':					// Load Banner Message
		case 'v':					// Load Answerback Message
		case '{':					// Down-line Load1
		case '|':					// User Defined Keys
		case ('$' << 8) | 'p':		// Restore Terminal State
			break;
		case ('$' << 8) | 'q':		// Request Selection or Setting
			for ( n = 0 ; *p != L'\0' ; p++ )
				n = (n << 16) | *p;
			switch(n) {
			case ('"' << 16) | 'p':		// DECSCL
				UNGETSTR("\033P1$r65\033\\");
				break;
			case 'm':					// SGR
			case 'r':					// DECSTBM
			case ('$' << 16) | '|':		// DECSCPP
			case ('$' << 16) | '}':		// DECSASD
			case ('$' << 16) | '~':		// DECSSDT
			case ('"' << 16) | 'q':		// DECSCA
			case ('*' << 16) | '|':		// DECSNLS
			default:
				UNGETSTR("\033P0$r\033\\");
				break;
			}
			break;
#if 0
		case ('$' << 8) | 's':		// Request Terminal State Report
		case ('$' << 8) | 't':		// Restore Presentation State
		case ('$' << 8) | 'u':		// Tabulation Stop Report
			break;
		case ('!' << 8) | 'u':		// Assign User-Preferred Supp Set
		case ('!' << 8) | 'z':		// Define Macro
		case ('!' << 8) | '{':		// Set Terminal Unit ID
		case ('!' << 8) | '|':		// Report Terminal Unit ID
			break;
		case ('"' << 8) | 'x':		// Program Function Key
		case ('"' << 8) | 'y':		// Program Alphanumeric Key
		case ('"' << 8) | 'z':		// Copy Key Default
		case ('"' << 8) | '{':		// Report Function Key Definition
		case ('"' << 8) | '~':		// Report Modifiers/Key State
			break;
#endif
		}
		break;

	case 'X':	// SOS Start of String
		break;

	case ']':	// OSC Operating System Command
		p = (LPWSTR)(LPCWSTR)m_OscPara;
		while ( *p != L'\0' && *p != L';' )
			tmp += *(p++);
		if ( *p == L';' )
			p++;
		if ( tmp.IsEmpty() )
			break;
		switch(_wtoi(tmp)) {
		case 0:		// Change Icon Name and Window Title to Pt
		case 1:		// Change Icon Name to Pt
		case 2:		// Change Window Title to Pt
			if ( (m_TitleMode & WTTL_CHENG) == 0 )
				m_pDocument->SetTitle(CString(p));
			break;
		case 4:		// color RGB	4;Pc;rgb:Pr/Pg/Pb
			while ( *p != L'\0' ) {
				tmp.Empty();
				while ( *p != L'\0' && *p != L';' )
					tmp += *(p++);
				if ( *p == L';' )
					p++;
				if ( tmp.IsEmpty() )
					break;
				n = _wtoi(tmp);
				tmp.Empty();
				while ( *p != L'\0' && *p != L';' )
					tmp += *(p++);
				if ( *p == L';' )
					p++;
				if ( tmp.Compare(L"?") == 0 ) {
					COLORREF rgb = (n < 256 ? m_ColTab[n] : RGB(0, 0, 0));
					UNGETSTR("\033]4;%d;rgb:%04x/%04x/%04x%s", n,
						GetRValue(rgb), GetGValue(rgb), GetBValue(rgb),
						(ch == 0x07 ? "\007":"\033\\"));
				} else {
					int r, g, b;
					if ( n < 256 && swscanf(tmp, L"rgb:%x/%x/%x", &r, &g, &b) == 3 ) {
						m_ColTab[n] = RGB(r, g, b);
						DISPUPDATE();
					}
				}
			}
			break;
		case 50:		// font		50;[?][+-][0-9]font
			while ( *p != L'\0' ) {
				WCHAR md = L'\0';
				int num = 0;
				while ( *p != L'\0' && *p == L' ' )
					p++;
				if ( *p == L'?' || *p == L'#' )
					md = *(p++);
				while ( *p != L'\0' && *p == L' ' )
					p++;
				tmp.Empty();
				if ( *p == L'+' || *p == L'-' )
					tmp += *(p++);
				while ( *p >= L'0' && *p <= L'9' )
					tmp += *(p++);
				num = _wtoi(tmp);
				while ( *p != L'\0' && *p == L' ' )
					p++;
				tmp.Empty();
				while ( *p != L'\0' && *p != L';' )
					tmp += *(p++);
				if ( *p == L';' )
					p++;
				if ( md == L'?' )
					UNGETSTR("\033]50%s", (ch == 0x07 ? "\007":"\033\\"));
			}
			break;
		}
		break;

	case '^':	// PM
	case '_':	// APC Application Program Command
		break;
	}

	m_OscFlag = FALSE;
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI... 

void CTextRam::fc_CSI(int ch)
{
	m_BackChar = 0;
	m_AnsiPara.RemoveAll();
	m_AnsiPara.Add(0xFFFF);
	fc_Case(STAGE_CSI);
}
void CTextRam::fc_CSI_ESC(int ch)
{
	fc_Case(STAGE_ESC);
}
void CTextRam::fc_CSI_DIGIT(int ch)
{
	int n = (int)m_AnsiPara.GetSize() - 1;
	if ( m_AnsiPara[n] == 0xFFFF )
		m_AnsiPara[n] = 0;
	m_AnsiPara[n] = m_AnsiPara[n] * 10 + ((ch & 0x7F) - '0');
}
void CTextRam::fc_CSI_SEPA(int ch)
{
	m_AnsiPara.Add(0xFFFF);
}
void CTextRam::fc_CSI_EXT(int ch)
{
	if ( ch >= 0x20 && ch <= 0x2F ) {
		m_BackChar &= 0xFF0000;
		m_BackChar |= (ch << 8);			// SP!"#$%&'()*+,-./
	} else
		m_BackChar = (ch << 16);			// <=>?

	switch(m_BackChar & 0x7F7F00) {
	case '?' << 16:
		fc_Case(STAGE_EXT1);
		break;
	case '$' << 8:
		fc_Case(STAGE_EXT2);
		break;
	case ' ' << 8:
		fc_Case(STAGE_EXT3);
		break;
	default:
		fc_Case(STAGE_EXT4);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// fc CSI @-Z

void CTextRam::fc_ICH(int ch)
{
	// CSI @	ICH	Insert Character
	for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		INSCHAR();
	fc_POP(ch);
}
void CTextRam::fc_CUU(int ch)
{
	// CSI A	CUU Cursor Up
	int n = GetAnsiPara(0, 1, 1);
	if ( m_CurY >= m_TopY ) {
		if ( (m_CurY -= n) < m_TopY )
			m_CurY = m_TopY;
	} else {
		if ( (m_CurY -= n) < 0 )
			m_CurY = 0;
	}
	LOCATE(m_CurX, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUD(int ch)
{
	// CSI B	CUD Cursor Down
	int n = GetAnsiPara(0, 1, 1);
	if ( m_CurY < m_BtmY ) {
		if ( (m_CurY += n) >= m_BtmY )
			m_CurY = m_BtmY - 1;
	} else {
		if ( (m_CurY += n) >= m_Lines )
			m_CurY = m_Lines - 1;
	}
	LOCATE(m_CurX, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUF(int ch)
{
	// CSI C	CUF Cursor Forward
	LOCATE(m_CurX + GetAnsiPara(0, 1, 1), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUB(int ch)
{
	// CSI D	CUB Cursor Backward
	LOCATE(m_CurX - GetAnsiPara(0, 1, 1), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CNL(int ch)
{
	// CSI E	CNL Move the cursor to the next line.
	LOCATE(0, m_CurY + GetAnsiPara(0, 1, 1));
	fc_POP(ch);
}
void CTextRam::fc_CPL(int ch)
{
	// CSI F	CPL Move the cursor to the preceding line.
	LOCATE(0, m_CurY - GetAnsiPara(0, 1, 1));
	fc_POP(ch);
}
void CTextRam::fc_CHA(int ch)
{
	// CSI G	CHA Move the active position to the n-th character of the active line.
	LOCATE(GetAnsiPara(0, 1, 1) - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUP(int ch)
{
	// CSI H	CUP Cursor Position
	if ( IsOptEnable(TO_DECOM) )
		LOCATE(GetAnsiPara(1, 1, 1) - 1, GetAnsiPara(0, 1, 1) - 1 + m_TopY);
	else
		LOCATE(GetAnsiPara(1, 1, 1) - 1, GetAnsiPara(0, 1, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_CHT(int ch)
{
	// CSI I	CHT Move the active position n tabs forward.
	for ( int i = GetAnsiPara(0, 1, 0) ; i > 0 ; i-- )
		TABSET(TAB_COLSNEXT);
	fc_POP(ch);
}
void CTextRam::fc_ED(int ch)
{
	// CSI J	ED Erase in page
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, 1);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines, 1);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1, 1);
		ERABOX(0, 0, m_Cols, m_CurY, 1);
		break;
	case 2:
		LOCATE(0, 0);
		ERABOX(0, 0, m_Cols, m_Lines, 1);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_EL(int ch)
{
	// CSI K	EL Erase in line
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1);
		break;
	case 2:
		ERABOX(0, m_CurY, m_Cols, m_CurY + 1);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_IL(int ch)
{
	// CSI L	IL Insert Line
	if ( m_CurY >= m_TopY && m_CurY < m_BtmY ) {
		for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- ) {
			BAKSCROLL(m_CurY, m_BtmY);
			TABSET(TAB_INSLINE);
		}
	}
	fc_POP(ch);
}
void CTextRam::fc_DL(int ch)
{
	// CSI M	DL Delete Line
	if ( m_CurY >= m_TopY && m_CurY < m_BtmY ) {
		for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- ) {
			FORSCROLL(m_CurY, m_BtmY);
			TABSET(TAB_DELINE);
		}
	}
	fc_POP(ch);
}
void CTextRam::fc_EF(int ch)
{
	// CSI N	EF Erase in field
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1);
		ERABOX(0, 0, m_Cols, m_CurY);
		break;
	case 2:
		LOCATE(0, 0);
		ERABOX(0, 0, m_Cols, m_Lines);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_EA(int ch)
{
	// CSI O	EA Erase in area
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1);
		ERABOX(0, 0, m_Cols, m_CurY);
		break;
	case 2:
		LOCATE(0, 0);
		ERABOX(0, 0, m_Cols, m_Lines);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DCH(int ch)
{
	// CSI P	DCH Delete Character
	for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		DELCHAR();
	fc_POP(ch);
}
void CTextRam::fc_SU(int ch)
{
	// CSI S	SU Scroll Up
	for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		FORSCROLL(m_TopY, m_BtmY);
	fc_POP(ch);
}
void CTextRam::fc_SD(int ch)
{
	// CSI T	SD Scroll Down
	if ( m_AnsiPara.GetSize() != 5 || GetAnsiPara(0, 0, 0) != 1 ) {
		for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
			BAKSCROLL(m_TopY, m_BtmY);
	}
	fc_POP(ch);
}
void CTextRam::fc_NP(int ch)
{
	// CSI U	NP Next Page
	for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		PostMessage(WM_VSCROLL, SB_PAGEDOWN, 0);
	fc_POP(ch);
}
void CTextRam::fc_PP(int ch)
{
	// CSI V	PP Preceding Page
	for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		PostMessage(WM_VSCROLL, SB_PAGEUP, 0);
	fc_POP(ch);
}
void CTextRam::fc_CTC(int ch)
{
	// CSI W	CTC Cursor tabulation control
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:	// a character tabulation stop is set at the active presentation position
		TABSET(TAB_COLSSET);
		break;
	case 1:	// a line tabulation stop is set at the active line (the line that contains the active presentation position)
		TABSET(TAB_LINESET);
		break;
	case 2:	// the character tabulation stop at the active presentation position is cleared
		TABSET(TAB_COLSCLR);
		break;
	case 3:	// the line tabulation stop at the active line is cleared
		TABSET(TAB_LINECLR);
		break;
	case 4:	// all character tabulation stops in the active line are cleared
		TABSET(TAB_COLSALLCLRACT);
		break;
	case 5:	// all character tabulation stops are cleared
		TABSET(TAB_COLSALLCLR);
		break;
	case 6:	// all line tabulation stops are cleared
		TABSET(TAB_LINEALLCLR);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_ECH(int ch)
{
	// CSI X	ECH Erase character
	ERABOX(m_CurX, m_CurY, m_CurX + GetAnsiPara(0, 1, 1), m_CurY + 1);
	fc_POP(ch);
}
void CTextRam::fc_CVT(int ch)
{
	// CSI Y	CVT Cursor line tabulation
	int i = GetAnsiPara(0, 1, 0);
	if ( i > m_Cols )
		i = m_Cols;
	for ( ; i > 0 ; i-- )
		TABSET(TAB_LINENEXT);
	fc_POP(ch);
}
void CTextRam::fc_CBT(int ch)
{
	// CSI Z	CBT Move the active position n tabs backward.
	int i = GetAnsiPara(0, 1, 0);
	if ( i > m_Cols )
		i = m_Cols;
	for ( ; i > 0 ; i-- )
		TABSET(TAB_COLSBACK);
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI a-z

void CTextRam::fc_SIMD(int ch)
{
	// CSI ^	SIMD Select implicit movement direction
	// 0 the direction of implicit movement is the same as that of the character progression
	// 1 the direction of implicit movement is opposite to that of the character progression.
	fc_POP(ch);
}
void CTextRam::fc_HPA(int ch)
{
	// CSI `	HPA Horizontal Position Absolute
	LOCATE(GetAnsiPara(0, 1, 1) - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_HPR(int ch)
{
	// CSI a	HPR Horizontal Position Relative
	LOCATE(m_CurX + GetAnsiPara(0, 1, 1), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_REP(int ch)
{
	// CSI b	REP Repeat
	if ( m_LastChar != 0 ) {
		if ( (m_LastChar & 0xFFFFFF00) != 0 )
			m_LastChar = ' ';
		for ( int n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
			PUT1BYTE(m_LastChar, m_BankNow);
		m_LastChar = 0;
	}
	fc_POP(ch);
}
void CTextRam::fc_DA1(int ch)
{
	// CSI c	DA1 Primary Device Attributes
	// UNGETSTR("\033[?1;0c");
	UNGETSTR("\033[?65;22;44c");
	fc_POP(ch);
}
void CTextRam::fc_VPA(int ch)
{
	// CSI d	VPA Vertical Line Position Absolute
	LOCATE(m_CurX, GetAnsiPara(0, 1, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_VPR(int ch)
{
	// CSI e	VPR Vertical Position Relative
	LOCATE(m_CurX, m_CurY + GetAnsiPara(0, 1, 0));
	fc_POP(ch);
}
void CTextRam::fc_HVP(int ch)
{
	// CSI f	HVP Horizontal and Vertical Position
	LOCATE(GetAnsiPara(1, 1, 1) - 1, GetAnsiPara(0, 1, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_TBC(int ch)
{
	// CSI g	TBC Tab Clear
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:	// the character tabulation stop at the active presentation position is cleared
		TABSET(TAB_COLSCLR);
		break;
	case 1:	// the line tabulation stop at the active line is cleared
		TABSET(TAB_LINECLR);
		break;
	case 2:	// all character tabulation stops in the active line are cleared
		TABSET(TAB_COLSALLCLRACT);
		break;
	case 3:	// all character tabulation stops are cleared
		TABSET(TAB_COLSALLCLR);
		break;
	case 4:	// all line tabulation stops are cleared
		TABSET(TAB_LINEALLCLR);
		break;
	case 5:	// all tabulation stops are cleared
		TABSET(TAB_ALLCLR);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_SM(int ch)
{
	// CSI h	SM Mode Set
	int n, i;
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( (i = GetAnsiPara(n, 0, 0)) >= 1 && i < 100 )
			ANSIOPT('h', i + 200);
	}
	fc_POP(ch);
}
void CTextRam::fc_RM(int ch)
{
	// CSI l	RM Mode ReSet
	int n, i;
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( (i = GetAnsiPara(n, 0, 0)) >= 1 && i < 100 )
			ANSIOPT('l', i + 200);
	}
	fc_POP(ch);
}
void CTextRam::fc_MC(int ch)
{
	// CSI i	MC Media copy
	fc_POP(ch);
}
void CTextRam::fc_HPB(int ch)
{
	// CSI j	HPB Character position backward
	LOCATE(m_CurX - GetAnsiPara(0, 1, 0), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_VPB(int ch)
{
	// CSI k	VPB Line position backward
	LOCATE(m_CurX, m_CurY - GetAnsiPara(0, 1, 0));
	fc_POP(ch);
}
void CTextRam::fc_SGR(int ch)
{
	// CSI m	SGR Select Graphic Rendition
	int n, i;
	if ( m_AnsiPara.GetSize() <= 0 )
		m_AnsiPara.Add(0);
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		switch(GetAnsiPara(n, 0, 0)) {
		case 0: m_AttNow.fc = m_DefAtt.fc; m_AttNow.bc = m_DefAtt.bc; m_AttNow.at = m_DefAtt.at; break;
		case 1: m_AttNow.at |= ATT_BOLD;   break;	// 1 bold or increased intensity
		case 2: m_AttNow.at |= ATT_HALF;   break;	// 2 faint, decreased intensity or second colour
		case 3: m_AttNow.at |= ATT_ITALIC; break;	// 3 italicized
		case 4: m_AttNow.at |= ATT_UNDER;  break;	// 4 singly underlined
		case 5: m_AttNow.at |= ATT_SBLINK; break;	// 5 slowly blinking
		case 6: m_AttNow.at |= ATT_BLINK;  break;	// 6 rapidly blinking
		case 7: m_AttNow.at |= ATT_REVS;   break;	// 7 negative image
		case 8: m_AttNow.at |= ATT_SECRET; break;	// 8 concealed characters
		case 9: m_AttNow.at |= ATT_LINE;   break;	// 9 crossed-out

		case 10: case 11: case 12:					// 10-20 alternative font
		case 13: case 14: case 15:
		case 16: case 17: case 18:
		case 19: case 20:
			m_AttNow.ft = GetAnsiPara(n, 0, 0) - 10;
			break;

		case 21: m_AttNow.at |= ATT_DUNDER; break;	// 21 doubly underlined
		case 22: m_AttNow.at &= ~(ATT_BOLD | ATT_HALF); break;		// 22 normal colour or normal intensity (neither bold nor faint)
		case 23: m_AttNow.at &= ~ATT_ITALIC; break;					// 23 not italicized, not fraktur
		case 24: m_AttNow.at &= ~(ATT_UNDER | ATT_DUNDER); break;	// 24 not underlined (neither singly nor doubly)
		case 25: m_AttNow.at &= ~(ATT_SBLINK | ATT_BLINK); break;	// 25 steady (not blinking)
		case 27: m_AttNow.at &= ~ATT_REVS; break;					// 27 positive image
		case 28: m_AttNow.at &= ~ATT_SECRET; break;					// 28 revealed characters
		case 29: m_AttNow.at &= ~ATT_LINE; break;					// 29 not crossed out

		case 30: case 31: case 32: case 33:
		case 34: case 35: case 36: case 37:
			m_AttNow.fc = (GetAnsiPara(n, 0, 0) - 30);
			break;
		case 38:
			if ( GetAnsiPara(n + 1, 0, 0) == 5 ) {	// 256 color
				if ( (i = GetAnsiPara(n + 2, 0, 0)) < 256 )
					m_AttNow.fc = i;
				n += 2;
			}
			break;
		case 39:
			m_AttNow.fc = m_DefAtt.fc;
			break;
		case 40: case 41: case 42: case 43:
		case 44: case 45: case 46: case 47:
			m_AttNow.bc = (GetAnsiPara(n, 0, 0) - 40);
			break;
		case 48:
			if ( GetAnsiPara(n + 1, 0, 0) == 5 ) {	// 256 color
				if ( (i = GetAnsiPara(n + 2, 0, 0)) < 256 )
					m_AttNow.bc = i;
				n += 2;
			}
			break;
		case 49:
			m_AttNow.bc = m_DefAtt.bc;
			break;

		case 51: m_AttNow.at |= ATT_FRAME; break;					// 51  framed
		case 52: m_AttNow.at |= ATT_CIRCLE; break;					// 52  encircled
		case 53: m_AttNow.at |= ATT_OVER;   break;					// 53  overlined
		case 54: m_AttNow.at &= ~(ATT_FRAME | ATT_CIRCLE); break;	// 54  not framed, not encircled
		case 55: m_AttNow.at &= ~ATT_OVER; break;					// 55  not overlined

		case 60: m_AttNow.at |= ATT_RSLINE; break;					// 60  ideogram underline or right side line
		case 61: m_AttNow.at |= ATT_RDLINE; break;					// 61  ideogram double underline or double line on the right side
		case 62: m_AttNow.at |= ATT_LSLINE; break;					// 62  ideogram overline or left side line
		case 63: m_AttNow.at |= ATT_LDLINE; break;					// 63  ideogram double overline or double line on the left side
		case 64: m_AttNow.at |= ATT_STRESS; break;					// 64  ideogram stress marking
		case 65: m_AttNow.at &= ~(ATT_RSLINE | ATT_RDLINE |
					ATT_LSLINE | ATT_LDLINE | ATT_STRESS); break;	// 65  cancels the effect of the rendition aspects established by parameter values 60 to 64

		case 90: case 91: case 92: case 93:
		case 94: case 95: case 96: case 97:
			m_AttNow.fc = (GetAnsiPara(n, 0, 0) - 90 + 8);
			break;
		case 100: case 101:  case 102: case 103:
		case 104: case 105:  case 106: case 107:
			m_AttNow.bc = (GetAnsiPara(n, 0, 0) - 100 + 8);
			break;
		}
	}
	if ( IS_ENABLE(m_AnsiOpt, TO_DECECM) == 0 ) {
		m_AttSpc.fc = m_AttNow.fc;
		m_AttSpc.bc = m_AttNow.bc;
	}
	if ( IS_ENABLE(m_AnsiOpt, TO_RLGCWA) != 0 ) {
		m_AttSpc.at = m_AttNow.at;
		m_AttSpc.ft = m_AttNow.ft;
	}
	fc_POP(ch);
}
void CTextRam::fc_DSR(int ch)
{
	// CSI n	DSR Device status report
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:	// ready, no malfunction detected
	case 1:	// busy, another DSR must be requested later
	case 2:	// busy, another DSR will be sent later
	case 3:	// some malfunction detected, another DSR must be requested later
	case 4:	// some malfunction detected, another DSR will be sent later
		break;
	case 5:	// a DSR is requested
		UNGETSTR("\033[0n");
		break;
	case 6:	// a report of the active presentation position or of the active data position in the form of ACTIVE POSITION REPORT (CPR) is requested
		UNGETSTR("\033[%d;%dR", m_CurY + 1, m_CurX + 1);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DAQ(int ch)
{
	// CSI o	DAQ Define area qualification
	fc_POP(ch);
}
void CTextRam::fc_DECBFAT(int ch)
{
	// CSI p	DECBFAT Begin field attribute : DEC private
	int n = GetAnsiPara(0, 0, 0);
	m_AttNow.at = 0;
	if ( (n &  1) != 0 ) m_AttNow.at |= ATT_BOLD;
	if ( (n &  2) != 0 ) m_AttNow.at |= ATT_BLINK;
	if ( (n &  4) != 0 ) m_AttNow.at |= ATT_SECRET;
	if ( (n &  8) != 0 ) m_AttNow.at |= ATT_UNDER;
	if ( (n & 16) != 0 ) m_AttNow.at |= ATT_REVS;
	if ( IS_ENABLE(m_AnsiOpt, TO_RLGCWA) != 0 )
		m_AttSpc.at = m_AttNow.at;
	fc_POP(ch);
}
void CTextRam::fc_DECLL(int ch)
{
	// CSI q	DECLL Load LEDs
	fc_POP(ch);
}
void CTextRam::fc_DECSTBM(int ch)
{
	// CSI r	DECSTBM Set Top and Bottom Margins
	int n, i;
	if ( (n = GetAnsiPara(0, 1, 1) - 1) < 0 )
		n = 0;
	if ( (i = GetAnsiPara(1, m_Lines, 1) - 1) < 0 || i >= m_Lines )
		i = m_Lines - 1;
	if ( n < i ) {
		m_TopY = n;
		m_BtmY = i + 1;
	}
	if ( m_CurY < m_TopY )
		LOCATE(m_CurX, m_TopY);
	else if ( m_CurY > m_BtmY )
		LOCATE(m_CurX, m_BtmY - 1);
	fc_POP(ch);
}
void CTextRam::fc_DECSC(int ch)
{
	// CSI s	DECSC Save Cursol Pos
	m_Save_CurX   = m_CurX;
	m_Save_CurY   = m_CurY;
	m_Save_DoWarp = m_DoWarp;
	fc_POP(ch);
}
void CTextRam::fc_XTWOP(int ch)
{
	// CSI t	XTWOP XTERM WINOPS
	int n = GetAnsiPara(0, 0, 0);
    switch (n) {
    case 1:			/* Restore (de-iconify) window */
    case 2:			/* Minimize (iconify) window */
    case 3:			/* Move the window to the given position x = p1, y = p2 */
    case 4:			/* Resize the window to given size in pixels w = p1, h = p2 */
    case 5:			/* Raise the window to the front of the stack */
    case 6:			/* Lower the window to the bottom of the stack */
    case 7:			/* Refresh the window */
		break;
    case 8:			/* Resize the text-area, in characters h = p1, w = p2 */
		if ( GetAnsiPara(2, 0, 0) > m_DefCols[0] ) {
			if ( !IsOptEnable(TO_DECCOLM) ) {
				EnableOption(TO_DECCOLM);
				InitCols();
			}
		} else if ( IsOptEnable(TO_DECCOLM) ) {
			DisableOption(TO_DECCOLM);
			InitCols();
		}
		break;
    case 9:			/* Maximize or restore mx = p1 */
		break;
    case 11:    	/* Report the window's state ESC[1/2t */
		UNGETSTR("\033[1t");
		break;
    case 13:    	/* Report the window's position ESC[3;x;yt */
		UNGETSTR("\033[3;%d;%dt", 0, 0);
		break;
    case 14:    	/* Report the window's size in pixels ESC[4;h;wt */
		UNGETSTR("\033[4;%d;%dt", m_Lines * 12, m_Cols * 6);
		break;
    case 18:    	/* Report the text's size in characters ESC[8;l;ct */
		UNGETSTR("\033[8;%d;%dt", m_Lines, m_Cols);
		break;
    case 19:    	/* Report the screen's size, in characters ESC[9;h;wt */
		UNGETSTR("\033[9;%d;%dt", 12, 6);
		break;
    case 20:    	/* Report the icon's label ESC]LxxxxESC\ */
    case 21:   		/* Report the window's title ESC]lxxxxxxESC\ */
		CString tmp;
		if ( (m_TitleMode & WTTL_REPORT) == 0 )
			m_IConv.IConvStr("CP932", m_SendCharSet[m_KanjiMode], m_pDocument->GetTitle(), tmp);
		UNGETSTR("\033]%c%s\033\\", (n == 20 ? 'L':'l'), tmp);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECRC(int ch)
{
	// CSI u	DECRC Load Cursol Pos
	LOCATE(m_Save_CurX, m_Save_CurY);
	m_DoWarp = m_Save_DoWarp;
	fc_POP(ch);
}
void CTextRam::fc_ORGSCD(int ch)
{
	// CSI v	ORGSCD Set Cursol Display
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:  CURON(); break;
	default: CUROFF(); break;
	}
	fc_POP(ch);
}
void CTextRam::fc_REQTPARM(int ch)
{
	// CSI x	DECREQTPARM Request terminal parameters 
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		UNGETSTR("\033[2;1;1;112;112;1;0x");
		break;
	case 1:
		UNGETSTR("\033[3;1;1;112;112;1;0x");
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECTST(int ch)
{
	// CSI y	DECTST Invoke confidence test
	fc_POP(ch);
}
void CTextRam::fc_DECVERP(int ch)
{
	// CSI z	DECVERP Set vertical pitch
	fc_POP(ch);
}
void CTextRam::fc_LINUX(int ch)
{
	// CSI ]	LINUX Linux Console
	switch(GetAnsiPara(0, 0, 0)) {
	case 1:		// Under Line Color
	case 2:		// Dim Color
	case 8:		// Now Color Default
	case 9:		// Screen blank Time sec
	case 10:	// Bell feq
	case 11:	// Bell msec
	case 12:	// Active Screen
	case 13:	// Screen Unblank
	case 14:	// VESA powerdown interval 
		break;
	}
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI ? ...
//
//	case ('?' << 16) | 'E':		/* ERASE_STATUS */
//	case ('?' << 16) | 'F':		/* FROM_STATUS */
//	case ('?' << 16) | 'H':		/* HIDE_STATUS */
//	case ('?' << 16) | 'S':		/* SHOW_STATUS */
//	case ('?' << 16) | 'T':		/* TO_STATUS */

void CTextRam::fc_DECSED(int ch)
{
	// CSI ? J	DECSED Selective Erase in Display
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines);
		break;
	case 1:
		ERABOX(0, 0, m_Cols, m_CurY);
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1);
		break;
	case 2:
		ERABOX(0, 0, m_Cols, m_Lines);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECSEL(int ch)
{
	// CSI ? K	DECSEL Selective Erase in Line
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX, m_CurY + 1);
		break;
	case 2:
		ERABOX(0, m_CurY, m_Cols, m_CurY + 1);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECST8C(int ch)
{
	// CSI ? W	DECST8C Set Tab at every 8 columns
	TABSET(TAB_RESET);
	fc_POP(ch);
}
void CTextRam::fc_DECSET(int ch)
{
	// CSI ? h	DECSET
	fc_DECSRET('h');
}
void CTextRam::fc_DECRST(int ch)
{
	// CSI ? l	DECRST
	fc_DECSRET('l');
}
void CTextRam::fc_XTREST(int ch)
{
	// CSI ? r	XTREST
	fc_DECSRET('r');
}
void CTextRam::fc_XTSAVE(int ch)
{
	// CSI ? s	XTSAVE
	fc_DECSRET('s');
}
void CTextRam::fc_DECSRET(int ch)
{
	// CSI ? h	DECSET
	// CSI ? l	DECRST
	// CSI ? r	XTERM_RESTORE
	// CSI ? s	XTERM_SAVE
	int n, i;
	ch &= 0x7F;
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( (i = GetAnsiPara(n, 0, 0)) < 1 )
			continue;
		else if ( i >= 1000 && i < 1080 )
			i -= 700;		// 300-379
		else if ( i >= 2000 && i < 2020 )
			i -= 1620;		// 380-399
		else if ( i >= 200 )
			continue;

		ANSIOPT(ch, i);

		switch(i) {
		case TO_DECCOLM:	// 3 DECCOLM Column mode
		case TO_XTMCSC:		// 40 XTERM Column switch control
			if ( IsOptEnable(TO_XTMCSC) ) {
				InitCols();
				LOCATE(0, 0);
				ERABOX(0, 0, m_Cols, m_Lines, 2);
			}
			break;
		case TO_DECSCNM:	// 5 DECSCNM Screen Mode: Light or Dark Screen
			DISPVRAM(0, 0, m_Cols, m_Lines);
			break;
		case TO_DECTCEM:	// 25 DECTCEM Text Cursor Enable Mode
			if ( IsOptEnable(TO_DECTCEM) )
				CURON();
			else
				CUROFF();
			break;
		case TO_DECTEK:		// 38 DECTEK
			if ( ch == 'h' ) {
				TekInit(4014);
				fc_Case(STAGE_TEK);
				return;
			}
			break;

		case TO_XTMABUF:	// 47 XTERM alternate buffer
		case TO_XTALTSCR:	// 1047 XTERM Use Alternate/Normal screen buffer
			if ( ch == 'l' )
				POPRAM();
			else if ( ch == 'h' )
				PUSHRAM();
			ANSIOPT(ch, i);
			break;
		case TO_XTSRCUR:	// 1048 XTERM Save/Restore cursor as in DECSC/DECRC
			if ( ch == 'l' ) {
				LOCATE(m_Save_CurX, m_Save_CurY);
				m_DoWarp = m_Save_DoWarp;
			} else if ( ch == 'h' ) {
				m_Save_CurX = m_CurX;
				m_Save_CurY = m_CurY;
				m_Save_DoWarp = m_DoWarp;
			}
			break;
		case TO_XTALTCLR:	// 1049 XTERM Use Alternate/Normal screen buffer, clearing it first
			if ( ch == 'l' )
				LOADRAM();
			else if ( ch == 'h' ) {
				m_Save_CurX = m_CurX;
				m_Save_CurY = m_CurY;
				m_Save_DoWarp = m_DoWarp;
				SAVERAM();
				ERABOX(0, 0, m_Cols, m_Lines);
				LOCATE(0, 0);
			}
			ANSIOPT(ch, i);
			break;

		case TO_XTMOSREP:	// 9 X10 mouse reporting
			m_MouseTrack = (IsOptEnable(i) ? 1 : 0);
			m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
			break;
		case TO_XTNOMTRK:	// 1000 X11 normal mouse tracking
		case TO_XTHILTRK:	// 1001 X11 hilite mouse tracking
		case TO_XTBEVTRK:	// 1002 X11 button-event mouse tracking
		case TO_XTAEVTRK:	// 1003 X11 any-event mouse tracking
			m_MouseTrack = (IsOptEnable(i) ? (i - TO_XTNOMTRK + 2) : 0);
			m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
			break;
		}
	}
	fc_POP(ch);
}
void CTextRam::fc_DECDSR(int ch)
{
	// CSI ? n	DECDSR Device status report
	switch(GetAnsiPara(0, 0, 0)) {
	case 1:		// [LA50] disable all unsolicited status reports.
	case 2:		// [LA50] enable unsolicited brief reports and send an extended one.
	case 3:		// [LA50] enable unsolicited extended reports and send one.
		UNGETSTR("\033[?20n");	// malfunction detected [also LCP01]
		break;
	case 6:		// a report of the active presentation position or of the active data position in the form of ACTIVE POSITION REPORT (CPR) is requested
		UNGETSTR("\033[?%d;%dR", m_CurY + 1, m_CurX + 1);
		break;
	case 15:	// printer status
		UNGETSTR("\033[?13n");	// no printer
		break;
	case 25:	// User Definable Key status
		UNGETSTR("\033[?21n");	// UDKs are locked
		break;
	case 26:	// keyboard dialect
		UNGETSTR("\033[?27;1n");	// North American/ASCII
		break;
	case 55:	// locator status
		UNGETSTR("\033[?%dn", ((m_Loc_Mode & LOC_MODE_ENABLE) == 0 ? 50 : 58));	// locator ready/busy
		break;
	case 56:	// locator type
		UNGETSTR("\033[?57;1n");	// Locator is a mouse
		break;
	case 63:	// Request checksum of macro definitions
		// DCS Pn ! ~ Ps ST
		UNGETSTR("\033P%d!~0\033\\", GetAnsiPara(1, 0, 0));	// 
		break;
	}
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI $ ...
//
//	case ('$' << 8) | 'q':		// Select Disconnect Delay Time
//	case ('$' << 8) | 's':		// Select Printer Type
//	case ('$' << 8) | 'u':		// DECRQTSR Request Terminal State Report
//	case ('$' << 8) | 'w':		// DECRQPSR Request Presentation State Report
//	case ('$' << 8) | 'y':		// DECRPM Report mode settings
//	case ('$' << 8) | '|':		// DECSCPP Set Ps columns per page
//	case ('$' << 8) | '}':		// DECSASD Select Active Status Display
//	case ('$' << 8) | '~':		// DECSSDT Set Status Display (Line) Type

void CTextRam::fc_DECRQM(int ch)
{
	// CSI $p	DECRQM Request mode settings
	int n;
	if ( (n = GetAnsiPara(0, 1, 1)) > 199 )
		n = 199;
	UNGETSTR("\033[%d;%d$y", n, IsOptEnable(200 + n) ? 1 : 2);
	fc_POP(ch);
}
void CTextRam::fc_DECCARA(int ch)
{
	// CSI $r	DECCARA Change Attribute in Rectangle
	int n, i, x, y;
	VRAM *vp;
	SetAnsiParaArea(0);
	while ( m_AnsiPara.GetSize() < 5 )
		m_AnsiPara.Add(0);
	if ( !m_Exact ) {
		m_AnsiPara[1] = 0;
		m_AnsiPara[3] = m_Cols - 1;
	}
	for ( y = m_AnsiPara[0] ; y <= m_AnsiPara[2] ; y++ ) {
		for ( x = m_AnsiPara[1] ; x <= m_AnsiPara[3] ; x++ ) {
			vp = GETVRAM(x, y);
			for ( n = 4 ; n < m_AnsiPara.GetSize() ; n++ ) {
				switch(GetAnsiPara(n, 0, 0)) {
				case 0: vp->at = 0; break;
				case 1: vp->at |= ATT_BOLD;   break;
				case 2: vp->at |= ATT_HALF;   break;
				case 3: vp->at |= ATT_ITALIC; break;
				case 4: vp->at |= ATT_UNDER;  break;
				case 5: vp->at |= ATT_SBLINK; break;
				case 6: vp->at |= ATT_BLINK;  break;
				case 7: vp->at |= ATT_REVS;   break;
				case 8: vp->at |= ATT_SECRET; break;
				case 9: vp->at |= ATT_LINE;   break;
				case 21: vp->at |= ATT_DUNDER; break;

				case 10: case 11: case 12:
				case 13: case 14: case 15:
				case 16: case 17: case 18:
				case 19: case 20:
					vp->ft = GetAnsiPara(n, 0, 0) - 10;
					break;

				case 22: vp->at &= ~(ATT_BOLD | ATT_HALF); break;
				case 23: vp->at &= ~ATT_ITALIC; break;
				case 24: vp->at &= ~ATT_UNDER; break;
				case 25: vp->at &= ~(ATT_SBLINK | ATT_BLINK); break;
				case 27: vp->at &= ~ATT_REVS; break;
				case 28: vp->at &= ~ATT_SECRET; break;
				case 29: vp->at &= ~ATT_LINE; break;

				case 30: case 31: case 32: case 33:
				case 34: case 35: case 36: case 37:
					vp->fc = (GetAnsiPara(n, 0, 0) - 30);
					break;
				case 38:
					if ( GetAnsiPara(n + 1, 0, 0) == 5 ) {	// 256 color
						if ( (i = GetAnsiPara(n + 2, 0, 0)) < 256 )
							vp->fc = i;
						n += 2;
					}
					break;
				case 39:
					vp->fc = m_DefAtt.fc;
					break;
				case 40: case 41: case 42: case 43:
				case 44: case 45: case 46: case 47:
					vp->bc = (GetAnsiPara(n, 0, 0) - 40);
					break;
				case 48:
					if ( GetAnsiPara(n + 1, 0, 0) == 5 ) {	// 256 color
						if ( (i = GetAnsiPara(n + 2, 0, 0)) < 256 )
							vp->bc = i;
						n += 2;
					}
					break;
				case 49:
					vp->bc = m_DefAtt.bc;
					break;

				case 51: vp->at |= ATT_FRAME; break;					// 51  framed
				case 52: vp->at |= ATT_CIRCLE; break;					// 52  encircled
				case 53: vp->at |= ATT_OVER;   break;					// 53  overlined
				case 54: vp->at &= ~(ATT_FRAME | ATT_CIRCLE); break;	// 54  not framed, not encircled
				case 55: vp->at &= ~ATT_OVER; break;					// 55  not overlined

				case 60: vp->at |= ATT_RSLINE; break;					// 60  ideogram underline or right side line
				case 61: vp->at |= ATT_RDLINE; break;					// 61  ideogram double underline or double line on the right side
				case 62: vp->at |= ATT_LSLINE; break;					// 62  ideogram overline or left side line
				case 63: vp->at |= ATT_LDLINE; break;					// 63  ideogram double overline or double line on the left side
				case 64: vp->at |= ATT_STRESS; break;					// 64  ideogram stress marking
				case 65: vp->at &= ~(ATT_RSLINE | ATT_RDLINE |
							ATT_LSLINE | ATT_LDLINE | ATT_STRESS); break;	// 65  cancels the effect of the rendition aspects established by parameter values 60 to 64

				case 90: case 91: case 92: case 93:
				case 94: case 95: case 96: case 97:
					vp->fc = (GetAnsiPara(n, 0, 0) - 90 + 8);
					break;
				case 100: case 101:  case 102: case 103:
				case 104: case 105:  case 106: case 107:
					vp->bc = (GetAnsiPara(n, 0, 0) - 100 + 8);
					break;
				}
			}
		}
	}
	DISPVRAM(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] - m_AnsiPara[1] + 1, m_AnsiPara[2] - m_AnsiPara[0] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECRARA(int ch)
{
	// CSI $t	DECRARA Reverse Attribute in Rectangle
	int n, x, y;
	VRAM *vp;
	SetAnsiParaArea(0);
	while ( m_AnsiPara.GetSize() < 5 )
		m_AnsiPara.Add(0);
	if ( !m_Exact ) {
		m_AnsiPara[1] = 0;
		m_AnsiPara[3] = m_Cols - 1;
	}
	for ( y = m_AnsiPara[0] ; y <= m_AnsiPara[2] ; y++ ) {
		for ( x = m_AnsiPara[1] ; x <= m_AnsiPara[3] ; x++ ) {
			vp = GETVRAM(x, y);
			for ( n = 4 ; n < m_AnsiPara.GetSize() ; n++ ) {
				switch(GetAnsiPara(n, 0, 0)) {
				case 0: vp->at ^= (ATT_BOLD | ATT_HALF | ATT_ITALIC | ATT_UNDER | ATT_SBLINK | ATT_BLINK | ATT_REVS | ATT_SECRET | ATT_LINE); break;
				case 1: vp->at ^= ATT_BOLD; break;
				case 2: vp->at ^= ATT_HALF; break;
				case 3: vp->at ^= ATT_ITALIC; break;
				case 4: vp->at ^= ATT_UNDER; break;
				case 5: vp->at ^= ATT_SBLINK; break;
				case 6: vp->at ^= ATT_BLINK; break;
				case 7: vp->at ^= ATT_REVS; break;
				case 8: vp->at ^= ATT_SECRET; break;
				case 9: vp->at ^= ATT_LINE; break;
				}
			}
		}
	}
	DISPVRAM(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] - m_AnsiPara[1] + 1, m_AnsiPara[2] - m_AnsiPara[0] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECCRA(int ch)
{
	// CSI $v	DECCRA Copy Rectangular Area
	WORD dm;
	int n, cx, cy, sx, sy, nx, ny;
	VRAM *sp, *np;

	SetAnsiParaArea(0);

	if ( (cy = GetAnsiPara(5, 1, 1) - 1) < 0 )
		cy = 0;
	else if ( cy >= m_Lines )
		cy = m_Lines - 1;

	if ( (cx = GetAnsiPara(6, 1, 1) - 1) < 0 )
		cx = 0;
	else if ( cx >= m_Cols )
		cx = m_Cols - 1;

	if ( m_AnsiPara[0] > cy || (m_AnsiPara[0] == cy && m_AnsiPara[1] >= cx) ) {
		for ( sy = m_AnsiPara[0], ny = cy ; sy <= m_AnsiPara[2] && ny < m_Lines ; sy++, ny++ ) {
			for ( sx = m_AnsiPara[1], nx = cx ; sx <= m_AnsiPara[3] && nx < m_Cols ; sx++, nx++ ) {
				sp = GETVRAM(sx, sy);
				np = GETVRAM(nx, ny);
				if ( nx == 0 ) {
					dm = np->dm;
					*np = *sp;
					np->dm = dm;
				} else
					*np = *sp;
			}
		}
	} else {
		for ( sy = m_AnsiPara[2], ny = cy + m_AnsiPara[2] - m_AnsiPara[1] ; sy >= m_AnsiPara[1] ; sy--, ny-- ) {
			if ( ny < m_Lines ) {
				for ( sx = m_AnsiPara[3], nx = cx + m_AnsiPara[3] - m_AnsiPara[1] ; sx >= m_AnsiPara[1] ; sx--, nx-- ) {
					if ( nx < m_Cols ) {
						sp = GETVRAM(sx, sy);
						np = GETVRAM(nx, ny);
						if ( nx == 0 ) {
							dm = np->dm;
							*np = *sp;
							np->dm = dm;
						} else
							*np = *sp;
					}
				}
			}
		}
	}

	DISPVRAM(cx, cy, m_AnsiPara[3] - m_AnsiPara[1] + 1, m_AnsiPara[2] - m_AnsiPara[0] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECFRA(int ch)
{
	// CSI $x	DECFRA Fill Rectangular Area
	int n, x, y;
	VRAM *vp;
	SetAnsiParaArea(1);
	n = GetAnsiPara(0, 0, 0);
	for ( y = m_AnsiPara[1] ; y <= m_AnsiPara[3] ; y++ ) {
		for ( x = m_AnsiPara[2] ; x <= m_AnsiPara[4] ; x++ ) {
			vp = GETVRAM(x, y);
			vp->ch = (BYTE)n;
			vp->at = m_AttNow.at;
			vp->fc = m_AttNow.fc;
			vp->bc = m_AttNow.bc;
			vp->md = m_BankTab[m_KanjiMode][(n & 0x80) == 0 ? m_BankGL : m_BankGR];
		}
	}
	DISPVRAM(m_AnsiPara[2], m_AnsiPara[1], m_AnsiPara[4] - m_AnsiPara[2] + 1, m_AnsiPara[3] - m_AnsiPara[1] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECERA(int ch)
{
	// CSI $z	DECERA Erase Rectangular Area
	SetAnsiParaArea(0);
	ERABOX(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] + 1, m_AnsiPara[2] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECSERA(int ch)
{
	// CSI ${	DECSERA Selective Erase Rectangular Area
	SetAnsiParaArea(0);
	ERABOX(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] + 1, m_AnsiPara[2] + 1);
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI SP ...
//
void CTextRam::fc_SL(int ch)
{
	// CSI (' ' << 8) | '@'		SL Scroll left
	int n;

	for ( n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		LEFTSCROLL();
	fc_POP(ch);
}
void CTextRam::fc_SR(int ch)
{
	// CSI (' ' << 8) | 'A'		SR Scroll Right
	int n;

	for ( n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		RIGHTSCROLL();
	fc_POP(ch);
}

#if 0
	case (' ' << 8) | 'B':		// GSM Graphic size modification
	case (' ' << 8) | 'C':		// GSS Graphic size selection
	case (' ' << 8) | 'D':		// FNT Font selection
	case (' ' << 8) | 'E':		// TSS Thin space specification
	case (' ' << 8) | 'F':		// JFY Justify
	case (' ' << 8) | 'G':		// SPI Spacing increment
	case (' ' << 8) | 'H':		// QUAD Quad
	case (' ' << 8) | 'I':		// SSU Select size unit
	case (' ' << 8) | 'J':		// PFS Page format selection
	case (' ' << 8) | 'K':		// SHS Select character spacing
	case (' ' << 8) | 'L':		// SVS Select line spacing
	case (' ' << 8) | 'M':		// IGS Identify graphic subrepertoire
	case (' ' << 8) | 'N':		// HTSA Character tabulation set absolute
	case (' ' << 8) | 'O':		// IDCS Identify device control string
	case (' ' << 8) | 'P':		// PPA Page Position Absolute
	case (' ' << 8) | 'Q':		// PPR Page Position Relative
	case (' ' << 8) | 'R':		// PPB Page Position Backwards
	case (' ' << 8) | 'S':		// SPD Select presentation directions
	case (' ' << 8) | 'T':		// DTA Dimension text area
	case (' ' << 8) | 'U':		// SLH Set line home
	case (' ' << 8) | 'V':		// SLL Set line limit
	case (' ' << 8) | 'W':		// FNK Function key
	case (' ' << 8) | 'X':		// SPQR Select print quality and rapidity
	case (' ' << 8) | 'Y':		// SEF Sheet eject and feed
	case (' ' << 8) | 'Z':		// PEC Presentation expand or contract
	case (' ' << 8) | '[':		// SSW Set space width
	case (' ' << 8) | '\\':		// SACS Set additional character separation
	case (' ' << 8) | ']':		// SAPV Select alternative presentation variants
	case (' ' << 8) | '^':		// STAB Description: Selective tabulation
	case (' ' << 8) | '_':		// GCC Graphic character combination
	case (' ' << 8) | '`':		// TATE Tabulation aligned trailing edge
	case (' ' << 8) | 'a':		// TALE Tabulation aligned leading edge
	case (' ' << 8) | 'b':		// TAC Tabulation aligned centred
	case (' ' << 8) | 'c':		// TCC Tabulation centred on character
	case (' ' << 8) | 'd':		// TSR Tabulation stop remove
	case (' ' << 8) | 'e':		// SCO Select character orientation
	case (' ' << 8) | 'f':		// SRCS Set reduced character separation
	case (' ' << 8) | 'g':		// SCS Set character spacing
	case (' ' << 8) | 'h':		// SLS Set line spacing
	case (' ' << 8) | 'i':		// SPH Set page home
	case (' ' << 8) | 'j':		// SPL Set page limit
	case (' ' << 8) | 'k':		// SCP Select character path
	case (' ' << 8) | 'l':		//
	case (' ' << 8) | 'm':		//
	case (' ' << 8) | 'n':		//
	case (' ' << 8) | 'o':		//
	case (' ' << 8) | 'p':		// DECSSCLS Set Scroll Speed
	case (' ' << 8) | 'q':		// DECSCUSR Set Cursor Style
	case (' ' << 8) | 'r':		// DECKCV Set Key Click Volume
	case (' ' << 8) | 's':		// DECNS New sheet
	case (' ' << 8) | 't':		// DECWBV Set Warning Bell Volume
	case (' ' << 8) | 'u':		// DECMBV Set Margin Bell Volume
	case (' ' << 8) | 'v':		// DECSLCK Set Lock Key Style
	case (' ' << 8) | 'w':		// DECSITF Select input tray failover
	case (' ' << 8) | 'x':		// DECSDPM Set Duplex Print Mode
	case (' ' << 8) | 'y':		//
	case (' ' << 8) | 'z':		// DECVPFS Variable page format select
	case (' ' << 8) | '{':		// DECSSS Set sheet size
	case (' ' << 8) | '|':		// DECRVEC Draw relative vector
	case (' ' << 8) | '}':		// DECKBD Keyboard Language Selection
#endif

void CTextRam::fc_DECTME(int ch)
{
	// CSI (' ' << 8) | '~'		DECTME Terminal Mode Emulation

	if ( GetAnsiPara(0, 0, 0) == 3 )
		m_AnsiOpt[TO_DECANM / 32] &= ~(1 << (TO_DECANM % 32));
	else
		m_AnsiOpt[TO_DECANM / 32] |= (1 << (TO_DECANM % 32));

	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI Ext...

void CTextRam::fc_CSI_ETC(int ch)
{
	int n, c, d, b, m;

	d = (m_BackChar | ch) & 0x7F7F7F;
	b = 0;
	m = m_CsiExt.GetSize() - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = d - m_CsiExt[n].code) == 0 ) {
			(this->*m_CsiExt[n].proc)(ch);
			return;
		} else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECRQMH(int ch)
{
	// CSI ('?' << 16) | ('$' << 8) | 'p'	DECRQMH Request Mode (DEC) Host to Terminal
	int n;

	if ( (n = GetAnsiPara(0, 1, 1)) > 199 )
		n = 199;
	UNGETSTR("\033[?%d;%d$y", n, IsOptEnable(n) ? 1 : 2);
	fc_POP(ch);
}
void CTextRam::fc_DECSTR(int ch)
{
	// CSI ('!' << 8) | 'p'		DECSTR Soft terminal reset

	RESET();
	fc_POP(ch);
}

#if 0
	case ('!' << 8) | 's':		// DECFIL Right justification
	case ('!' << 8) | 'v':		// DECASFC Automatic sheet feeder control
	case ('!' << 8) | 'w':		// DECUND Select undeline character
	case ('!' << 8) | 'x':		// DECPTS Printwheel table select
	case ('!' << 8) | 'y':		// DECSS Select spacing
	case ('!' << 8) | '|':		// DECVEC Draw vector
	case ('!' << 8) | '}':		// DECFIN Document finishing
	case ('"' << 8) | 'p':		// DECSCL Set compatibility level
#endif

void CTextRam::fc_DECSCA(int ch)
{
	// CSI ('"' << 8) | 'q'		DECSCA Select character attributes

	if ( GetAnsiPara(0, 0, 0) == 1 )
		m_AttNow.em |= EM_PROTECT;
	else
		m_AttNow.em &= ~EM_PROTECT;

	fc_POP(ch);
}

#if 0
	case ('"' << 8) | 's':		// DECPWA Page width alignment
	case ('"' << 8) | 'u':		//        Set Transmit Rate Limit
	case ('"' << 8) | 'v':		// DECRQDE Request device extent
	case ('"' << 8) | 'w':		// DECRPDE Report device extent
	case ('"' << 8) | 'z':		// DECDEN Select density
	case ('&' << 8) | 'q':		// DECSNC Set number of copies
	case ('&' << 8) | 'u':		// DECRQUPSS Request User-Preferred Supplemental Set
	case ('&' << 8) | 'w':		// DECLRP Locator report
	case ('&' << 8) | 'x':		// DECES Enable Session
	case ('\'' << 8) | 'q':		// DECSBCA Select bar code attributes
#endif

void CTextRam::fc_DECEFR(int ch)
{
	// CSI ('\'' << 8) | 'w'		DECEFR Enable filter rectangle

	if ( m_AnsiPara.GetSize() >= 4 ) {
		m_Loc_Rect.top    = GetAnsiPara(0, 1, 1) - 1;
		m_Loc_Rect.left   = GetAnsiPara(1, 1, 1) - 1;
		m_Loc_Rect.bottom = GetAnsiPara(2, 1, 1) - 1;
		m_Loc_Rect.right  = GetAnsiPara(3, 1, 1) - 1;
		m_Loc_Mode |= LOC_MODE_FILTER;
	} else
		m_Loc_Mode &= ~LOC_MODE_FILTER;

	fc_POP(ch);
}
void CTextRam::fc_DECELR(int ch)
{
	// CSI ('\'' << 8) | 'z'		DECELR Enable locator reports

	switch(GetAnsiPara(0, 0, 0)) {
	case 0:	// locator disabled (default)
		m_Loc_Mode &= ~LOC_MODE_ENABLE;
		m_Loc_Mode &= ~LOC_MODE_ONESHOT;
		break;
	case 1:	// locator reports enabled 
		m_Loc_Mode |= LOC_MODE_ENABLE;
		m_Loc_Mode &= ~LOC_MODE_ONESHOT;
		break;
	case 2:	// one shot (allow one report, then disable)
		m_Loc_Mode |= LOC_MODE_ENABLE;
		m_Loc_Mode |= LOC_MODE_ONESHOT;
		break;
	}
	m_Loc_Mode &= ~LOC_MODE_FILTER;
	switch(GetAnsiPara(1, 0, 0)) {
	case 0:	// default to character cells
	case 2:	// character cells
		m_Loc_Mode &= ~LOC_MODE_PIXELS;
		break;
	case 1:	// device physical pixels
		m_Loc_Mode |= LOC_MODE_PIXELS;
		break;
	}
	LocReport(0, 0, 0, 0);

	fc_POP(ch);
}
void CTextRam::fc_DECSLE(int ch)
{
	// CSI ('\'' << 8) | '{'		DECSLE Select locator events
	int n;

	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		switch(GetAnsiPara(n, 0, 0)) {
		case 0:	// respond only to explicit host requests (default, also cancels any pending filter rectangle) 
			m_Loc_Mode &= ~LOC_MODE_EVENT;
			break;
		case 1:	// report button down transitions 
			m_Loc_Mode |= LOC_MODE_EVENT;
			m_Loc_Mode |= LOC_MODE_DOWN;
			break;
		case 2:	// do not report button down transitions 
			m_Loc_Mode |= LOC_MODE_EVENT;
			m_Loc_Mode &= ~LOC_MODE_DOWN;
			break;
		case 3:	// report button up transitions 
			m_Loc_Mode |= LOC_MODE_EVENT;
			m_Loc_Mode |= LOC_MODE_UP;
			break;
		case 4:	// do not report button up transitions
			m_Loc_Mode |= LOC_MODE_EVENT;
			m_Loc_Mode &= ~LOC_MODE_UP;
			break;
		}
	}
	LocReport(0, 0, 0, 0);

	fc_POP(ch);
}
void CTextRam::fc_DECRQLP(int ch)
{
	// CSI ('\'' << 8) | '|'		DECRQLP Request locator position

	LocReport(2, 0, 0, 0);
	fc_POP(ch);
}
void CTextRam::fc_DECIC(int ch)
{
	// CSI ('\'' << 8) | '}'		DECIC Insert Column
	int n;

	for ( n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		INSCHAR();

	fc_POP(ch);
}
void CTextRam::fc_DECDC(int ch)
{
	// CSI ('\'' << 8) | '~'		DECDC Delete Column(s)
	int n;

	for ( n = GetAnsiPara(0, 1, 1) ; n > 0 ; n-- )
		DELCHAR();

	fc_POP(ch);
}

#if 0
	case ('*' << 8) | 'p':		// Select ProPrinter Character Set
	case ('*' << 8) | 'q':		// DECSRC Secure Reset Confirmation
	case ('*' << 8) | 'r':		// Select Communication Speed
	case ('*' << 8) | 's':		// Select Flow Control Type
	case ('*' << 8) | 'u':		// Select Communication Port
#endif

void CTextRam::fc_DECSACE(int ch)
{
	// CSI ('*' << 8) | 'x'		DECSACE Select Attribute and Change Extent
	m_Exact = (GetAnsiPara(0, 1, 1) == 2 ? TRUE : FALSE);
	fc_POP(ch);
}

#if 0
	case ('*' << 8) | 'y':		// DECRQCRA Request Checksum of Rectangle Area
	case ('*' << 8) | 'z':		// DECINVM Invoke Macro
	case ('*' << 8) | '{':		// DECMSR Macro Space Report Response
	case ('*' << 8) | '|':		// DECSNLS Select number of lines per screen
	case ('*' << 8) | '}':		// DECLFKC Local Function Key Control
	case ('+' << 8) | 'p':		// DECSR Secure Reset
	case ('+' << 8) | 'q':		// DECELF Enable Local Functions
	case ('+' << 8) | 'r':		// DECSMKR Select Modifier Key Reporting
	case ('+' << 8) | 'v':		// DECMM Memory management
	case ('+' << 8) | 'w':		// DECSPP Set Port Parameter
	case ('+' << 8) | 'x':		// DECPQPLFM Program Key Free Memory Inquiry
	case ('+' << 8) | 'y':		// DECPKFMR Program Key Free Memory Report
	case ('+' << 8) | 'z':		// DECPKA Program Key Action
	case ('-' << 8) | 'p':		// DECARP Auto Repeat Rate
	case ('-' << 8) | 'q':		// DECCRTST CRT Saver Timing
	case ('-' << 8) | 'r':		// DECSEST Energy Saver Timing
	case (',' << 8) | 'p':		// Load Time of Day
	case (',' << 8) | 'u':		// Key Type Inquiry
	case (',' << 8) | 'v':		// Report Key Type
	case (',' << 8) | 'w':		// Key Definition Inquiry
	case (',' << 8) | 'x':		// Session Page Memory Allocation
	case (',' << 8) | 'y':		// Update Session
	case (',' << 8) | 'z':		// Down Line Load Allocation
	case (',' << 8) | '|':		// Assign Color
	case (',' << 8) | '{':		// Select Zero Symbol
#endif

void CTextRam::fc_DECATC(int ch)
{
	// CSI (',' << 8) | '}'		DECATC Alternate Text Colors

	switch(GetAnsiPara(0, 0, 0)) {
	case 0: m_AttNow.at = 0; break;
	case 1: m_AttNow.at = ATT_BOLD; break;
	case 2: m_AttNow.at = ATT_REVS; break;
	case 3: m_AttNow.at = ATT_UNDER; break;
	case 4: m_AttNow.at = ATT_SBLINK; break;
	case 5: m_AttNow.at = ATT_BOLD | ATT_REVS; break;
	case 6: m_AttNow.at = ATT_BOLD | ATT_UNDER; break;
	case 7: m_AttNow.at = ATT_BOLD | ATT_SBLINK; break;
	case 8: m_AttNow.at = ATT_REVS | ATT_UNDER; break;
	case 9: m_AttNow.at = ATT_REVS | ATT_SBLINK; break;
	case 10: m_AttNow.at = ATT_UNDER | ATT_SBLINK; break;
	case 11: m_AttNow.at = ATT_BOLD | ATT_REVS | ATT_UNDER; break;
	case 12: m_AttNow.at = ATT_BOLD | ATT_REVS | ATT_SBLINK; break;
	case 13: m_AttNow.at = ATT_BOLD | ATT_UNDER | ATT_SBLINK; break;
	case 14: m_AttNow.at = ATT_REVS | ATT_UNDER | ATT_SBLINK; break;
	case 15: m_AttNow.at = ATT_BOLD | ATT_REVS | ATT_UNDER | ATT_SBLINK; break;
	}
	m_AttNow.fc = GetAnsiPara(1, 7, 0);
	m_AttNow.bc = (GetAnsiPara(2, 0, 0) << 4);
	if ( IS_ENABLE(m_AnsiOpt, TO_DECECM) == 0 ) {
		m_AttSpc.fc = m_AttNow.fc;
		m_AttSpc.bc = m_AttNow.bc;
	}
	if ( IS_ENABLE(m_AnsiOpt, TO_RLGCWA) != 0 )
		m_AttSpc.at = m_AttNow.at;

	fc_POP(ch);
}

#if 0
	case (',' << 8) | '~':		// Play Sound
	case (')' << 8) | 'p':		// Select Digital Printed Data Type
	case (')' << 8) | '{':		// Select Color Look-Up Table
#endif


void CTextRam::fc_DA2(int ch)
{
	// CSI ('>' << 16) | 'c'	DA2 Secondary Device Attributes

	UNGETSTR("\033[>65;10;1c");
	fc_POP(ch);
}
void CTextRam::fc_DA3(int ch)
{
	// CSI ('=' << 16) | 'c'		DA3 Tertiary Device Attributes

	UNGETSTR("\033P!|010205\033\\");
	fc_POP(ch);
}

#if 0
	case ('=' << 16) | 'A':		// cons25 Set the border color to n
	case ('=' << 16) | 'B':		// cons25 Set bell pitch (p) and duration (d)
	case ('=' << 16) | 'C':		// cons25 Set global cursor type
	case ('=' << 16) | 'F':		// cons25 Set normal foreground color to n
	case ('=' << 16) | 'G':		// cons25 Set normal background color to n
	case ('=' << 16) | 'H':		// cons25 Set normal reverse foreground color to n
	case ('=' << 16) | 'I':		// cons25 Set normal reverse background color to n
#endif

void CTextRam::fc_C25LCT(int ch)
{
	// CSI ('=' << 16) | 'S'	C25LCT cons25 Set local cursor type

	switch(GetAnsiPara(0, 0, 0)) {
	case 0:  CURON(); break;
	case 1:  CUROFF(); break;
	case 2:  CURON(); break;
	}
	fc_POP(ch);
}
