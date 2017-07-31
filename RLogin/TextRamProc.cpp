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
#include "GrapWnd.h"
#include "CancelDlg.h"

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
static CTextRam::ESCNAMEPROC	*fc_pDcsProc = NULL;

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
	{ 0x20,		0x7F,		&CTextRam::fc_TEXT		},
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

static const CTextRam::PROCTAB fc_Big51Tab[] = {
	{ 0x20,		0x7F,		&CTextRam::fc_TEXT		},
	{ 0xA1,		0xC6,		&CTextRam::fc_BIG51		},
	{ 0xC7,		0xC8,		&CTextRam::fc_TEXT		},
	{ 0xC9,		0xF9,		&CTextRam::fc_BIG51		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Big52Tab[] = {
	{ 0x00,		0x3F,		&CTextRam::fc_BIG53		},
	{ 0x40,		0x7E,		&CTextRam::fc_BIG52		},
	{ 0x7F,		0xA0,		&CTextRam::fc_BIG53		},
	{ 0xA1,		0xFE,		&CTextRam::fc_BIG52		},
	{ 0xFF,		0,			&CTextRam::fc_BIG53		},
	{ 0,		0,			NULL } };
	
static const CTextRam::PROCTAB fc_Utf8Tab[] = {
	{ 0x20,		0x7F,		&CTextRam::fc_TEXT		},
	{ 0xA0,		0xBF,		&CTextRam::fc_UTF87		},
	{ 0xC0,		0xDF,		&CTextRam::fc_UTF81		},	// UCS-2 5+6=11 bit
	{ 0xE0,		0xEF,		&CTextRam::fc_UTF82		},	// UCS-2 4+6+6=16 bit
	{ 0xF0,		0xF7,		&CTextRam::fc_UTF83		},	// UCS-2 3+6+6+6=21 bit
	{ 0xF8,		0xFB,		&CTextRam::fc_UTF88		},	// UCS-4 2+6+6+6+6=26 bit ?
	{ 0xFC,		0xFD,		&CTextRam::fc_UTF89		},	// UCS-4 1+6+6+6+6+6=31 bit ?
//	{ 0xF8,		0xFD,		&CTextRam::fc_UTF87		},	// UCS-4 Error
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
	{ 0x20,		0x7E,		&CTextRam::fc_POP		},
	{ 0x7F,		0,			&CTextRam::fc_IGNORE	},
	{ 0x80,		0x9F,		&CTextRam::fc_CESC		},
	{ 0xA0,		0xFE,		&CTextRam::fc_POP		},
	{ 0xFF,		0,			&CTextRam::fc_IGNORE	},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_GotoTab[] = {
	{ 0x18,		0,			&CTextRam::fc_POP		},
	{ 0x1A,		0,			&CTextRam::fc_POP		},
	{ 0x1B,		0,			&CTextRam::fc_IGNORE	},
	{ 0x20,		0xFF,		&CTextRam::fc_GOTOXY	},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_StatTab[] = {
	{ 0x18,		0,			&CTextRam::fc_POP		},
	{ 0x1A,		0,			&CTextRam::fc_POP		},
	{ 0x1B,		0,			&CTextRam::fc_IGNORE	},
	{ 0x20,		0x7E,		&CTextRam::fc_STAT		},
	{ 0x7F,		0,			&CTextRam::fc_IGNORE	},
	{ 0x80,		0x9F,		&CTextRam::fc_CESC		},
	{ 0xA0,		0xFE,		&CTextRam::fc_STAT		},
	{ 0xFF,		0,			&CTextRam::fc_IGNORE	},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Esc2Tab[] = {
	{ ' ',		0,			&CTextRam::fc_ACS		},	// 2/0  ACS Announce code structure
	{ '!',		0,			&CTextRam::fc_ESCI		},	// 2/1
	{ '"',		0,			&CTextRam::fc_ESCI		},	// 2/2
	{ '#',		0,			&CTextRam::fc_DECSOP	},	// 2/3  DECSOP Screen Opt
	{ '$',		0,			&CTextRam::fc_CSC0W		},	// 2/4  CSC0W Char Set
	{ '%',		0,			&CTextRam::fc_DOCS		},	// 2/5  DOCS Designate other coding system
	{ '&',		0,			&CTextRam::fc_ESCI		},	// 2/6
	{ '\'',		0,			&CTextRam::fc_ESCI		},	// 2/7
	{ '(',		0,			&CTextRam::fc_CSC0		},	// 2/8  CSC0 G0 charset
	{ ')',		0,			&CTextRam::fc_CSC1		},	// 2/9  CSC1 G1 charset
	{ '*',		0,			&CTextRam::fc_CSC2		},	// 2/10 CSC2 G2 charset
	{ '+',		0,			&CTextRam::fc_CSC3		},	// 2/11 CSC3 G3 charset
	{ ',',		0,			&CTextRam::fc_CSC0A		},	// 2/12 CSC0A G0 charset
	{ '-',		0,			&CTextRam::fc_CSC1A		},	// 2/13 CSC1A G1 charset
	{ '.',		0,			&CTextRam::fc_CSC2A		},	// 2/14 CSC2A G2 charset
	{ '/',		0,			&CTextRam::fc_CSC3A		},	// 2/15 CSC3A G3 charset

	{ '1',		0,			&CTextRam::fc_DECHTS	},	// DECHTS Horizontal tab set
	{ '2',		0,			&CTextRam::fc_DECAHT	},	// DECCAHT Clear all horizontal tabs
	{ '3',		0,			&CTextRam::fc_DECVTS	},	// DECVTS Vertical tab set
	{ '4',		0,			&CTextRam::fc_DECAVT	},	// DECCAVT Clear all vertical tabs
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
//	{ 'S',		0,			&CTextRam::fc_STS		},	// STS Set transmit state
//	{ 'T',		0,			&CTextRam::fc_CCH		},	// CCH Cancel character
//	{ 'U',		0,			&CTextRam::fc_MW		},	// MW Message waiting
	{ 'V',		0,			&CTextRam::fc_SPA		},	// SPA Start of guarded area				VT52 Print the line with the cursor.
	{ 'W',		0,			&CTextRam::fc_EPA		},	// EPA End of guarded area					VT52 Enter printer controller mode.
	{ 'X',		0,			&CTextRam::fc_SOS		},	// SOS Start of String
	{ 'Y',		0,			&CTextRam::fc_V5MCP		},	//											VT52 Move cursor to column Pn.
	{ 'Z',		0,			&CTextRam::fc_SCI		},	// SCI Single character introducer			VT52 Identify (host to terminal).
	{ '[',		0,			&CTextRam::fc_CSI		},	// CSI Control sequence introducer
	{ '\\',		0,			&CTextRam::fc_POP		},	// ST String terminator
	{ ']',		0,			&CTextRam::fc_OSC		},	// OSC Operating System Command				LINUX Linux Console
	{ '^',		0,			&CTextRam::fc_PM		},	// PM Privacy message
	{ '_',		0,			&CTextRam::fc_APC		},	// APC Application Program Command
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
	{ 0x3A,		0,			&CTextRam::fc_CSI_PUSH	},	// :
	{ 0x3B,		0,			&CTextRam::fc_CSI_SEPA	},	// ;
	{ 0x3C,		0x3F,		&CTextRam::fc_CSI_EXT	},	// <=>?
	{ 0x40,		0x7E,		&CTextRam::fc_POP		},
	{ 0x7F,		0,			&CTextRam::fc_IGNORE	},
	{ 0x80,		0x9F,		&CTextRam::fc_CESC		},
	{ 0xA0,		0xAF,		&CTextRam::fc_CSI_EXT	},
	{ 0xB0,		0xB9,		&CTextRam::fc_CSI_DIGIT	},
	{ 0xBA,		0,			&CTextRam::fc_CSI_PUSH	},
	{ 0xBB,		0,			&CTextRam::fc_CSI_SEPA	},
	{ 0xBC,		0xBF,		&CTextRam::fc_CSI_EXT	},
	{ 0xC0,		0xFE,		&CTextRam::fc_POP		},
	{ 0xFF,		0,			&CTextRam::fc_IGNORE	},
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
	{ 'i',		0,			&CTextRam::fc_MC		},	// MC Media copy
	{ 'j',		0,			&CTextRam::fc_HPB		},	// HPB Character position backward
	{ 'k',		0,			&CTextRam::fc_VPB		},	// VPB Line position backward
	{ 'l',		0,			&CTextRam::fc_RM		},	// RM Mode ReSet
	{ 'm',		0,			&CTextRam::fc_SGR		},	// SGR Select Graphic Rendition
	{ 'n',		0,			&CTextRam::fc_DSR		},	// DSR Device status report
	{ 'o',		0,			&CTextRam::fc_DAQ		},	// DAQ Define area qualification
	{ 'p',		0,			&CTextRam::fc_ORGBFAT	},	// ORGBFAT Begin field attribute				DECSSL Select Set-Up Language
	{ 'q',		0,			&CTextRam::fc_DECLL		},	// DECLL Load LEDs
	{ 'r',		0,			&CTextRam::fc_DECSTBM	},	// DECSTBM Set Top and Bottom Margins
	{ 's',		0,			&CTextRam::fc_SCOSC		},	// SCOSC Save Cursol Pos						DECSLRM Set left and right margins
	{ 't',		0,			&CTextRam::fc_XTWOP		},	// XTWOP XTERM WINOPS							DECSLPP Set lines per physical page
	{ 'u',		0,			&CTextRam::fc_SCORC		},	// SCORC Load Cursol Pos						DECSHTS Set horizontal tab stops
	{ 'v',		0,			&CTextRam::fc_ORGSCD	},	// ORGSCD Set Cursol Display					DECSVTS Set vertical tab stops
	{ 'x',		0,			&CTextRam::fc_REQTPARM	},	// DECREQTPARM Request terminal parameters
	{ 'y',		0,			&CTextRam::fc_DECTST	},	// DECTST Invoke confidence test
//	{ 'z',		0,			&CTextRam::fc_DECVERP	},	// DECVERP Set vertical pitch
//	{ '~',		0,			&CTextRam::fc_DECFNK	},	// DECFNK Function Key
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext1Tab[] = {			// ('?' << 16) | sc
//	{ 'E',		0,			&CTextRam::fc_POP		},	//		ERASE_STATUS
//	{ 'F',		0,			&CTextRam::fc_POP		},	//		FROM_STATUS
//	{ 'H',		0,			&CTextRam::fc_POP		},	//		HIDE_STATUS
	{ 'J',		0,			&CTextRam::fc_DECSED	},	// DECSED Selective Erase in Display
	{ 'K',		0,			&CTextRam::fc_DECSEL	},	// DECSEL Selective Erase in Line
//	{ 'S',		0,			&CTextRam::fc_POP		},	//		SHOW_STATUS
//	{ 'T',		0,			&CTextRam::fc_POP		},	//		TO_STATUS
	{ 'W',		0,			&CTextRam::fc_DECST8C	},	// DECST8C Set Tab at every 8 columns
	{ 'h',		0,			&CTextRam::fc_DECSET	},	// DECSET
	{ 'i',		0,			&CTextRam::fc_DECMC		},	// DECMC Media Copy (DEC)
	{ 'l',		0,			&CTextRam::fc_DECRST	},	// DECRST
	{ 'n',		0,			&CTextRam::fc_DECDSR	},	// DECDSR Device status report
	{ 'r',		0,			&CTextRam::fc_XTREST	},	// XTREST
	{ 's',		0,			&CTextRam::fc_XTSAVE	},	// XTSAVE
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext2Tab[] = {			// ('$' << 8) | sc
	{ 'p',		0,			&CTextRam::fc_DECRQM	},	// DECRQM Request mode settings
//	{ 'q',		0,			&CTextRam::fc_POP		},	// DECSDDT Select Disconnect Delay Time
	{ 'r',		0,			&CTextRam::fc_DECCARA	},	// DECCARA Change Attribute in Rectangle
//	{ 's',		0,			&CTextRam::fc_POP		},	// DECSPRTT Select Printer Type
	{ 't',		0,			&CTextRam::fc_DECRARA	},	// DECRARA Reverse Attribute in Rectangle
	{ 'u',		0,			&CTextRam::fc_DECRQTSR	},	// DECRQTSR Request Terminal State Report
	{ 'v',		0,			&CTextRam::fc_DECCRA	},	// DECCRA Copy Rectangular Area
	{ 'w',		0,			&CTextRam::fc_DECRQPSR	},	// DECRQPSR Request Presentation State Report
	{ 'x',		0,			&CTextRam::fc_DECFRA	},	// DECFRA Fill Rectangular Area
	{ 'z',		0,			&CTextRam::fc_DECERA	},	// DECERA Erase Rectangular Area
	{ '{',		0,			&CTextRam::fc_DECSERA	},	// DECSERA Selective Erase Rectangular Area
	{ '|',		0,			&CTextRam::fc_DECSCPP	},	// DECSCPP Set Ps columns per page
	{ '}',		0,			&CTextRam::fc_DECSASD	},	// DECSASD Select Active Status Display
	{ '~',		0,			&CTextRam::fc_DECSSDT	},	// DECSSDT Set Status Display (Line) Type
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext3Tab[] = {			// (' ' << 8) | sc
	{ '@',		0,			&CTextRam::fc_SL		},	// SL Scroll left
	{ 'A',		0,			&CTextRam::fc_SR		},	// SR Scroll Right
//	{ 'B',		0,			&CTextRam::fc_POP		},	// GSM Graphic size modification
//	{ 'C',		0,			&CTextRam::fc_POP		},	// GSS Graphic size selection
	{ 'D',		0,			&CTextRam::fc_FNT		},	// FNT Font selection
//	{ 'E',		0,			&CTextRam::fc_POP		},	// TSS Thin space specification
//	{ 'F',		0,			&CTextRam::fc_POP		},	// JFY Justify
//	{ 'G',		0,			&CTextRam::fc_POP		},	// SPI Spacing increment
//	{ 'H',		0,			&CTextRam::fc_POP		},	// QUAD Quad
//	{ 'I',		0,			&CTextRam::fc_POP		},	// SSU Select size unit
//	{ 'J',		0,			&CTextRam::fc_POP		},	// PFS Page format selection
//	{ 'K',		0,			&CTextRam::fc_POP		},	// SHS Select character spacing
//	{ 'L',		0,			&CTextRam::fc_POP		},	// SVS Select line spacing
//	{ 'M',		0,			&CTextRam::fc_POP		},	// IGS Identify graphic subrepertoire
//	{ 'N',		0,			&CTextRam::fc_POP		},	// HTSA Character tabulation set absolute
//	{ 'O',		0,			&CTextRam::fc_POP		},	// IDCS Identify device control string
	{ 'P',		0,			&CTextRam::fc_PPA		},	// PPA Page Position Absolute
	{ 'Q',		0,			&CTextRam::fc_PPR		},	// PPR Page Position Relative
	{ 'R',		0,			&CTextRam::fc_PPB		},	// PPB Page Position Backwards
//	{ 'S',		0,			&CTextRam::fc_POP		},	// SPD Select presentation directions
//	{ 'T',		0,			&CTextRam::fc_POP		},	// DTA Dimension text area
//	{ 'U',		0,			&CTextRam::fc_POP		},	// SLH Set line home
//	{ 'V',		0,			&CTextRam::fc_POP		},	// SLL Set line limit
//	{ 'W',		0,			&CTextRam::fc_POP		},	// FNK Function key
//	{ 'X',		0,			&CTextRam::fc_POP		},	// SPQR Select print quality and rapidity
//	{ 'Y',		0,			&CTextRam::fc_POP		},	// SEF Sheet eject and feed
//	{ 'Z',		0,			&CTextRam::fc_POP		},	// PEC Presentation expand or contract
//	{ '[',		0,			&CTextRam::fc_POP		},	// SSW Set space width
//	{ '\'',		0,			&CTextRam::fc_POP		},	// SACS Set additional character separation
//	{ ']',		0,			&CTextRam::fc_POP		},	// SAPV Select alternative presentation variants
//	{ '^',		0,			&CTextRam::fc_POP		},	// STAB Description: Selective tabulation
//	{ '_',		0,			&CTextRam::fc_POP		},	// GCC Graphic character combination
//	{ '`',		0,			&CTextRam::fc_POP		},	// TATE Tabulation aligned trailing edge
//	{ 'a',		0,			&CTextRam::fc_POP		},	// TALE Tabulation aligned leading edge
//	{ 'b',		0,			&CTextRam::fc_POP		},	// TAC Tabulation aligned centred
//	{ 'c',		0,			&CTextRam::fc_POP		},	// TCC Tabulation centred on character
//	{ 'd',		0,			&CTextRam::fc_POP		},	// TSR Tabulation stop remove
//	{ 'e',		0,			&CTextRam::fc_POP		},	// SCO Select character orientation
//	{ 'f',		0,			&CTextRam::fc_POP		},	// SRCS Set reduced character separation
//	{ 'g',		0,			&CTextRam::fc_POP		},	// SCS Set character spacing
//	{ 'h',		0,			&CTextRam::fc_POP		},	// SLS Set line spacing
//	{ 'i',		0,			&CTextRam::fc_POP		},	// SPH Set page home
//	{ 'j',		0,			&CTextRam::fc_POP		},	// SPL Set page limit
//	{ 'k',		0,			&CTextRam::fc_POP		},	// SCP Select character path
//	{ 'p',		0,			&CTextRam::fc_POP		},	// DECSSCLS Set Scroll Speed
	{ 'q',		0,			&CTextRam::fc_DECSCUSR	},	// DECSCUSR Set Cursor Style
//	{ 'r',		0,			&CTextRam::fc_POP		},	// DECKCV Set Key Click Volume
//	{ 's',		0,			&CTextRam::fc_POP		},	// DECNS New sheet
//	{ 't',		0,			&CTextRam::fc_POP		},	// DECSWBV Set Warning Bell Volume
//	{ 'u',		0,			&CTextRam::fc_POP		},	// DECSMBV Set Margin Bell Volume
//	{ 'v',		0,			&CTextRam::fc_POP		},	// DECSLCK Set Lock Key Style
//	{ 'w',		0,			&CTextRam::fc_POP		},	// DECSITF Select input tray failover
//	{ 'x',		0,			&CTextRam::fc_POP		},	// DECSDPM Set Duplex Print Mode
//	{ 'z',		0,			&CTextRam::fc_POP		},	// DECVPFS Variable page format select
//	{ '{',		0,			&CTextRam::fc_POP		},	// DECSSS Set sheet size
//	{ '|',		0,			&CTextRam::fc_POP		},	// DECRVEC Draw relative vector
//	{ '}',		0,			&CTextRam::fc_POP		},	// DECKBD Keyboard Language Selection
	{ '~',		0,			&CTextRam::fc_DECTME	},	// DECTME Terminal Mode Emulation
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_Ext4Tab[] = {
	{ 0x40,		0x7E,		&CTextRam::fc_CSI_ETC	},
	{ 0x7F,		0,			&CTextRam::fc_IGNORE	},
	{ 0,		0,			NULL } };
static const CTextRam::CSIEXTTAB fc_CsiExtTab[] = {
	{ 				('!'  << 8) | 'p',		&CTextRam::fc_DECSTR	},	// DECSTR Soft terminal reset
//	{ 				('!'  << 8) | 's',		&CTextRam::fc_POP		},	// DECFIL Right justification
//	{ 				('!'  << 8) | 'v',		&CTextRam::fc_POP		},	// DECASFC Automatic sheet feeder control
//	{ 				('!'  << 8) | 'w',		&CTextRam::fc_POP		},	// DECUND Select undeline character
//	{ 				('!'  << 8) | 'x',		&CTextRam::fc_POP		},	// DECPTS Printwheel table select
//	{ 				('!'  << 8) | 'y',		&CTextRam::fc_POP		},	// DECSS Select spacing
//	{ 				('!'  << 8) | '|',		&CTextRam::fc_POP		},	// DECVEC Draw vector
//	{ 				('!'  << 8) | '}',		&CTextRam::fc_POP		},	// DECFIN Document finishing
	{ 				('"'  << 8) | 'p',		&CTextRam::fc_DECSCL	},	// DECSCL Set compatibility level
	{ 				('"'  << 8) | 'q',		&CTextRam::fc_DECSCA	},	// DECSCA Select character attributes
//	{ 				('"'  << 8) | 's',		&CTextRam::fc_POP		},	// DECPWA Page width alignment
//	{ 				('"'  << 8) | 'u',		&CTextRam::fc_POP		},	// DECSTRL Set Transmit Rate Limit
	{ 				('"'  << 8) | 'v',		&CTextRam::fc_DECRQDE	},	// DECRQDE Request device extent
//	{ 				('"'  << 8) | 'z',		&CTextRam::fc_POP		},	// DECDEN Select density
//	{ 				('&'  << 8) | 'q',		&CTextRam::fc_POP		},	// DECSNC Set number of copies
	{ 				('&'  << 8) | 'u',		&CTextRam::fc_DECRQUPSS	},	// DECRQUPSS Request User-Preferred Supplemental Set
//	{ 				('&'  << 8) | 'x',		&CTextRam::fc_POP		},	// DECES Enable Session
//	{ 				('\'' << 8) | 'q',		&CTextRam::fc_POP		},	// DECSBCA Select bar code attributes
	{ 				('\'' << 8) | 'w',		&CTextRam::fc_DECEFR	},	// DECEFR Enable filter rectangle
	{ 				('\'' << 8) | 'z',		&CTextRam::fc_DECELR	},	// DECELR Enable locator reports
	{ 				('\'' << 8) | '{',		&CTextRam::fc_DECSLE	},	// DECSLE Select locator events
	{ 				('\'' << 8) | '|',		&CTextRam::fc_DECRQLP	},	// DECRQLP Request locator position
	{ 				('\'' << 8) | '}',		&CTextRam::fc_DECIC		},	// DECIC Insert Column
	{ 				('\'' << 8) | '~',		&CTextRam::fc_DECDC		},	// DECDC Delete Column(s)
//	{ 				(')'  << 8) | 'p',		&CTextRam::fc_POP		},	// DECSDPT Select Digital Printed Data Type
	{ 				(')'  << 8) | '{',		&CTextRam::fc_DECSTGLT	},	// DECSTGLT Select Color Look-Up Table
//	{ 				('*'  << 8) | 'p',		&CTextRam::fc_POP		},	// DECSPPCS Select ProPrinter Character Set
//	{ 				('*'  << 8) | 'r',		&CTextRam::fc_POP		},	// DECSCS Select Communication Speed
//	{ 				('*'  << 8) | 's',		&CTextRam::fc_POP		},	// DECSFC Select Flow Control Type
//	{ 				('*'  << 8) | 'u',		&CTextRam::fc_POP		},	// DECSCP Select Communication Port
	{ 				('*'  << 8) | 'x',		&CTextRam::fc_DECSACE	},	// DECSACE Select Attribute and Change Extent
	{ 				('*'  << 8) | 'y',		&CTextRam::fc_DECRQCRA	},	// DECRQCRA Request Checksum of Rectangle Area
	{ 				('*'  << 8) | 'z',		&CTextRam::fc_DECINVM	},	// DECINVM Invoke Macro
//	{ 				('*'  << 8) | '|',		&CTextRam::fc_POP		},	// DECSNLS Select number of lines per screen
//	{ 				('*'  << 8) | '}',		&CTextRam::fc_POP		},	// DECLFKC Local Function Key Control
	{ 				('+'  << 8) | 'p',		&CTextRam::fc_DECSR		},	// DECSR Secure Reset
//	{ 				('+'  << 8) | 'q',		&CTextRam::fc_POP		},	// DECELF Enable Local Functions
//	{ 				('+'  << 8) | 'r',		&CTextRam::fc_POP		},	// DECSMKR Select Modifier Key Reporting
//	{ 				('+'  << 8) | 'v',		&CTextRam::fc_POP		},	// DECMM Memory management
//	{ 				('+'  << 8) | 'w',		&CTextRam::fc_POP		},	// DECSPP Set Port Parameter
	{ 				('+'  << 8) | 'x',		&CTextRam::fc_DECPQPLFM	},	// DECPQPLFM Program Key Free Memory Inquiry
//	{ 				('+'  << 8) | 'z',		&CTextRam::fc_POP		},	// DECPKA Program Key Action
//	{ 				('-'  << 8) | 'p',		&CTextRam::fc_POP		},	// DECARR Auto Repeat Rate
//	{ 				('-'  << 8) | 'q',		&CTextRam::fc_POP		},	// DECCRTST CRT Saver Timing
//	{ 				('-'  << 8) | 'r',		&CTextRam::fc_POP		},	// DECSEST Energy Saver Timing
//	{ 				(','  << 8) | 'p',		&CTextRam::fc_POP		},	// DECLTOD Load Time of Day
//	{ 				(','  << 8) | 'v',		&CTextRam::fc_POP		},	// DECRPKT Report Key Type
//	{ 				(','  << 8) | 'w',		&CTextRam::fc_POP		},	// DECRQKD Request Key Definition
//	{ 				(','  << 8) | 'x',		&CTextRam::fc_POP		},	// DECSPMA Session Page Memory Allocation
//	{ 				(','  << 8) | 'y',		&CTextRam::fc_POP		},	// DECUS Update Session
//	{ 				(','  << 8) | 'z',		&CTextRam::fc_POP		},	// DECDLDA Down Line Load Allocation
	{ 				(','  << 8) | '|',		&CTextRam::fc_DECAC		},	// DECAC Assign Color
//	{ 				(','  << 8) | '{',		&CTextRam::fc_POP		},	// DECSZS Select Zero Symbol
	{ 				(','  << 8) | 'q',		&CTextRam::fc_DECTID	},	// DECTID Select Terminal ID
	{ 				(','  << 8) | '}',		&CTextRam::fc_DECATC	},	// DECATC Alternate Text Colors
	{ 				(','  << 8) | '~',		&CTextRam::fc_DECPS		},	// DECPS Play Sound
	{ ('<' << 16)				| 's',		&CTextRam::fc_TTIMESV	},	// TTIMESV IME の開閉状態を保存する。
	{ ('<' << 16)				| 't',		&CTextRam::fc_TTIMEST	},	// TTIMEST IME の開閉状態を設定する。
	{ ('<' << 16)				| 'r',		&CTextRam::fc_TTIMERS	},	// TTIMERS IME の開閉状態を復元する。
	{ ('<' << 16) |	('!'  << 8)	| 'i',		&CTextRam::fc_RLIMGCP	},	// RLIMGCP 現画面をImageにコピーする
//	{ ('=' << 16)				| 'A',		&CTextRam::fc_POP		},	// cons25 Set the border color to n
//	{ ('=' << 16)				| 'B',		&CTextRam::fc_POP		},	// cons25 Set bell pitch (p) and duration (d)
//	{ ('=' << 16)				| 'C',		&CTextRam::fc_POP		},	// cons25 Set global cursor type
//	{ ('=' << 16)				| 'F',		&CTextRam::fc_POP		},	// cons25 Set normal foreground color to n
//	{ ('=' << 16)				| 'G',		&CTextRam::fc_POP		},	// cons25 Set normal background color to n
//	{ ('=' << 16)				| 'H',		&CTextRam::fc_POP		},	// cons25 Set normal reverse foreground color to n
//	{ ('=' << 16)				| 'I',		&CTextRam::fc_POP		},	// cons25 Set normal reverse background color to n
	{ ('=' << 16)				| 'S',		&CTextRam::fc_C25LCT	},	// C25LCT cons25 Set local cursor type
	{ ('=' << 16)				| 'c',		&CTextRam::fc_DA3		},	// DA3 Tertiary Device Attributes
	{ ('>' << 16)				| 'T',		&CTextRam::fc_XTRMTT	},	// xterm CASE_RM_TITLE
	{ ('>' << 16)				| 'm',		&CTextRam::fc_XTMDKEY	},	// xterm CASE_SET_MOD_FKEYS
	{ ('>' << 16)				| 'n',		&CTextRam::fc_XTMDKYD	},	// xterm CASE_SET_MOD_FKEYS0
//	{ ('>' << 16)				| 'p',		&CTextRam::fc_XTHDPT	},	// xterm CASE_HIDE_POINTER
	{ ('>' << 16)				| 'c',		&CTextRam::fc_DA2		},	// DA2 Secondary Device Attributes
	{ ('>' << 16)				| 't',		&CTextRam::fc_XTSMTT	},	// xterm CASE_SM_TITLE
	{ ('?' << 16) | ('$'  << 8) | 'p',		&CTextRam::fc_DECRQMH	},	// DECRQMH Request Mode (DEC) Host to Terminal
	{							    0,		NULL } };

static const CTextRam::PROCTAB fc_Osc1Tab[] = {
	{ 0x07,		0,			&CTextRam::fc_OSC_CMD	},	// BELL (ST)
	{ 0x18,		0,			&CTextRam::fc_OSC_CAN	},
	{ 0x1A,		0,			&CTextRam::fc_OSC_CAN	},
	{ 0x1B,		0,			&CTextRam::fc_OSC_CMD	},
	{ 0x20,		0x2F,		&CTextRam::fc_OSC_CMD	},
	{ 0x30,		0x39,		&CTextRam::fc_CSI_DIGIT	},
	{ 0x3A,		0,			&CTextRam::fc_CSI_PUSH	},
	{ 0x3B,		0,			&CTextRam::fc_CSI_SEPA	},
	{ 0x3C,		0x3F,		&CTextRam::fc_OSC_CMD	},
	{ 0x40,		0x7E,		&CTextRam::fc_OSC_CMD	},
	{ 0x9C,		0,			&CTextRam::fc_OSC_CMD	},	// ST String terminator
	{ 0xA0,		0xAF,		&CTextRam::fc_OSC_CMD	},
	{ 0xB0,		0xB9,		&CTextRam::fc_CSI_DIGIT	},
	{ 0xBA,		0,			&CTextRam::fc_CSI_PUSH	},
	{ 0xBB,		0,			&CTextRam::fc_CSI_SEPA	},
	{ 0xBC,		0xBF,		&CTextRam::fc_OSC_CMD	},
	{ 0xC0,		0xFE,		&CTextRam::fc_OSC_CMD	},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Osc2Tab[] = {
	{ 0x00,		0xFF,		&CTextRam::fc_OSC_PAM	},
	{ 0x18,		0,			&CTextRam::fc_OSC_CAN	},
	{ 0x1A,		0,			&CTextRam::fc_OSC_CAN	},
	{ 0,		0,			NULL } };
static const CTextRam::CSIEXTTAB fc_DcsExtTab[] = {
	{							 '|',		&CTextRam::fc_DECUDK	},	// DECUDK User Defined Keys
	{							 'p',		&CTextRam::fc_DECREGIS	},	// DECREGIS ReGIS graphics
	{							 'q',		&CTextRam::fc_DECSIXEL	},	// DECSIXEL Sixel graphics
//	{							 'r',		&CTextRam::fc_POP		},	// DECLBAN Load Banner Message
//	{							 'v',		&CTextRam::fc_POP		},	// DECLANS Load Answerback Message
//	{							 'x',		&CTextRam::fc_POP		},	// DECtalk commands
//	{							 'y',		&CTextRam::fc_POP		},	// DECLFF Load font file
	{							 '{',		&CTextRam::fc_DECDLD	},	// DECDLD Dynamically Redefinable Character Sets Extension
//	{							 '}',		&CTextRam::fc_POP		},	// DECATFF Assign to font file
//	{							 '~',		&CTextRam::fc_POP		},	// DECDTFF Delete type family or font file
//	{				('!' << 8) | 'q',		&CTextRam::fc_POP		},	// DECGIDIS Enter GIDIS mode
//	{				('!' << 8) | 'u',		&CTextRam::fc_POP		},	// DECAUPSS Assign User-Preferred Supp Set
	{				('!' << 8) | 'z',		&CTextRam::fc_DECDMAC	},	// DECDMAC Define Macro
	{				('!' << 8) | '{',		&CTextRam::fc_DECSTUI	},	// DECSTUI Set Terminal Unit ID
//	{				('"' << 8) | 'x',		&CTextRam::fc_POP		},	// DECPFK Program Function Key
//	{				('"' << 8) | 'y',		&CTextRam::fc_POP		},	// DECPAK Program Alphanumeric Key
//	{				('"' << 8) | 'z',		&CTextRam::fc_POP		},	// DECCKD Copy Key Default
	{				('$' << 8) | 'p',		&CTextRam::fc_DECRSTS	},	// DECRSTS Restore Terminal State
	{				('$' << 8) | 'q',		&CTextRam::fc_DECRQSS	},	// DECRQSS Request Selection or Setting
	{				('$' << 8) | 't',		&CTextRam::fc_DECRSPS	},	// DECRSPS Restore Presentation State
//	{				('$' << 8) | 'w',		&CTextRam::fc_POP		},	// DECLKD Locator key definition
//	{				('+' << 8) | 'p',		&CTextRam::fc_POP		},	// XTSTCAP Set Termcap/Terminfo Data (xterm, experimental)
	{				('+' << 8) | 'q',		&CTextRam::fc_XTRQCAP	},	// XTRQCAP Request Termcap/Terminfo String (xterm, experimental)
	{							   0,		NULL } };

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

static int	fc_EscNameTabMax = 62;
static CTextRam::ESCNAMEPROC fc_EscNameTab[] = {
	{	_T("ACS"),		&CTextRam::fc_ACS,		NULL,	NULL	},
	{	_T("APC"),		&CTextRam::fc_APC,		NULL,	NULL	},
	{	_T("BPH"),		&CTextRam::fc_BPH,		NULL,	NULL	},
	{	_T("CSC0"),		&CTextRam::fc_CSC0,		NULL,	NULL	},
	{	_T("CSC0A"),	&CTextRam::fc_CSC0A,	NULL,	NULL	},
	{	_T("CSC0W"),	&CTextRam::fc_CSC0W,	NULL,	NULL	},
	{	_T("CSC1"),		&CTextRam::fc_CSC1,		NULL,	NULL	},
	{	_T("CSC1A"),	&CTextRam::fc_CSC1A,	NULL,	NULL	},
	{	_T("CSC2"),		&CTextRam::fc_CSC2,		NULL,	NULL	},
	{	_T("CSC2A"),	&CTextRam::fc_CSC2A,	NULL,	NULL	},
	{	_T("CSC3"),		&CTextRam::fc_CSC3,		NULL,	NULL	},
	{	_T("CSC3A"),	&CTextRam::fc_CSC3A,	NULL,	NULL	},
	{	_T("CSI"),		&CTextRam::fc_CSI,		NULL,	NULL	},
	{	_T("DCS"),		&CTextRam::fc_DCS,		NULL,	NULL	},
	{	_T("DECBI"),	&CTextRam::fc_BI,		NULL,	NULL	},
	{	_T("DECCAHT"),	&CTextRam::fc_DECAHT,	NULL,	NULL	},
	{	_T("DECCAVT"),	&CTextRam::fc_DECAVT,	NULL,	NULL	},
	{	_T("DECFI"),	&CTextRam::fc_FI,		NULL,	NULL	},
	{	_T("DECHTS"),	&CTextRam::fc_DECHTS,	NULL,	NULL	},
	{	_T("DECPAM"),	&CTextRam::fc_DECPAM,	NULL,	NULL	},
	{	_T("DECPNM"),	&CTextRam::fc_DECPNM,	NULL,	NULL	},
	{	_T("DECRC"),	&CTextRam::fc_RC,		NULL,	NULL	},
	{	_T("DECSC"),	&CTextRam::fc_SC,		NULL,	NULL	},
	{	_T("DECSOP"),	&CTextRam::fc_DECSOP,	NULL,	NULL	},
	{	_T("DECVTS"),	&CTextRam::fc_DECVTS,	NULL,	NULL	},
	{	_T("DOCS"),		&CTextRam::fc_DOCS,		NULL,	NULL	},
	{	_T("EPA"),		&CTextRam::fc_EPA,		NULL,	NULL	},
	{	_T("ESA"),		&CTextRam::fc_ESA,		NULL,	NULL	},
	{	_T("ESCI"),		&CTextRam::fc_ESCI,		NULL,	NULL	},
	{	_T("HTJ"),		&CTextRam::fc_HTJ,		NULL,	NULL	},
	{	_T("HTS"),		&CTextRam::fc_HTS,		NULL,	NULL	},
	{	_T("IND"),		&CTextRam::fc_IND,		NULL,	NULL	},
	{	_T("LMA"),		&CTextRam::fc_LMA,		NULL,	NULL	},
	{	_T("LS1R"),		&CTextRam::fc_LS1R,		NULL,	NULL	},
	{	_T("LS2"),		&CTextRam::fc_LS2,		NULL,	NULL	},
	{	_T("LS2R"),		&CTextRam::fc_LS2R,		NULL,	NULL	},
	{	_T("LS3"),		&CTextRam::fc_LS3,		NULL,	NULL	},
	{	_T("LS3R"),		&CTextRam::fc_LS3R,		NULL,	NULL	},
	{	_T("NBH"),		&CTextRam::fc_NBH,		NULL,	NULL	},
	{	_T("NEL"),		&CTextRam::fc_NEL,		NULL,	NULL	},
	{	_T("NOP"),		&CTextRam::fc_POP,		NULL,	NULL	},
	{	_T("OSC"),		&CTextRam::fc_OSC,		NULL,	NULL	},
	{	_T("PLD"),		&CTextRam::fc_PLD,		NULL,	NULL	},
	{	_T("PLU"),		&CTextRam::fc_PLU,		NULL,	NULL	},
	{	_T("PM"),		&CTextRam::fc_PM,		NULL,	NULL	},
	{	_T("RI"),		&CTextRam::fc_RI,		NULL,	NULL	},
	{	_T("RIS"),		&CTextRam::fc_RIS,		NULL,	NULL	},
	{	_T("SCI"),		&CTextRam::fc_SCI,		NULL,	NULL	},
	{	_T("SOS"),		&CTextRam::fc_SOS,		NULL,	NULL	},
	{	_T("SPA"),		&CTextRam::fc_SPA,		NULL,	NULL	},
	{	_T("SS2"),		&CTextRam::fc_SS2,		NULL,	NULL	},
	{	_T("SS3"),		&CTextRam::fc_SS3,		NULL,	NULL	},
	{	_T("SSA"),		&CTextRam::fc_SSA,		NULL,	NULL	},
	{	_T("USR"),		&CTextRam::fc_USR,		NULL,	NULL	},
	{	_T("V5CUP"),	&CTextRam::fc_V5CUP,	NULL,	NULL	},
	{	_T("V5EX"),		&CTextRam::fc_V5EX,		NULL,	NULL	},
	{	_T("V5MCP"),	&CTextRam::fc_V5MCP,	NULL,	NULL	},
	{	_T("VTS"),		&CTextRam::fc_VTS,		NULL,	NULL	},
	{	NULL,			NULL,					NULL,	NULL	},
};

static int	fc_CsiNameTabMax = 126;
static CTextRam::ESCNAMEPROC fc_CsiNameTab[] = {
	{	_T("C25LCT"),	&CTextRam::fc_C25LCT,	NULL,	NULL 	},
	{	_T("CBT"),		&CTextRam::fc_CBT,		NULL,	NULL	},
	{	_T("CHA"),		&CTextRam::fc_CHA,		NULL,	NULL	},
	{	_T("CHT"),		&CTextRam::fc_CHT,		NULL,	NULL	},
	{	_T("CNL"),		&CTextRam::fc_CNL,		NULL,	NULL	},
	{	_T("CPL"),		&CTextRam::fc_CPL,		NULL,	NULL	},
	{	_T("CTC"),		&CTextRam::fc_CTC,		NULL,	NULL	},
	{	_T("CUB"),		&CTextRam::fc_CUB,		NULL,	NULL	},
	{	_T("CUD"),		&CTextRam::fc_CUD,		NULL,	NULL	},
	{	_T("CUF"),		&CTextRam::fc_CUF,		NULL,	NULL	},
	{	_T("CUP"),		&CTextRam::fc_CUP,		NULL,	NULL	},
	{	_T("CUU"),		&CTextRam::fc_CUU,		NULL,	NULL	},
	{	_T("CVT"),		&CTextRam::fc_CVT,		NULL,	NULL	},
	{	_T("DA1"),		&CTextRam::fc_DA1,		NULL,	NULL	},
	{	_T("DA2"),		&CTextRam::fc_DA2,		NULL,	NULL	},
	{	_T("DA3"),		&CTextRam::fc_DA3,		NULL,	NULL	},
	{	_T("DAQ"),		&CTextRam::fc_DAQ,		NULL,	NULL	},
	{	_T("DCH"),		&CTextRam::fc_DCH,		NULL,	NULL	},
	{	_T("DECAC"),	&CTextRam::fc_DECAC,	NULL,	NULL	},
	{	_T("DECATC"),	&CTextRam::fc_DECATC,	NULL,	NULL	},
	{	_T("DECCARA"),	&CTextRam::fc_DECCARA,	NULL,	NULL	},
	{	_T("DECCRA"),	&CTextRam::fc_DECCRA,	NULL,	NULL	},
	{	_T("DECDC"),	&CTextRam::fc_DECDC,	NULL,	NULL	},
	{	_T("DECDSR"),	&CTextRam::fc_DECDSR,	NULL,	NULL	},
	{	_T("DECEFR"),	&CTextRam::fc_DECEFR,	NULL,	NULL	},
	{	_T("DECELR"),	&CTextRam::fc_DECELR,	NULL,	NULL	},
	{	_T("DECERA"),	&CTextRam::fc_DECERA,	NULL,	NULL	},
	{	_T("DECFRA"),	&CTextRam::fc_DECFRA,	NULL,	NULL	},
	{	_T("DECIC"),	&CTextRam::fc_DECIC,	NULL,	NULL	},
	{	_T("DECINVM"),	&CTextRam::fc_DECINVM,	NULL,	NULL	},
	{	_T("DECLL"),	&CTextRam::fc_DECLL,	NULL,	NULL	},
	{	_T("DECMC"),	&CTextRam::fc_DECMC,	NULL,	NULL	},
	{	_T("DECPQPLFM"),&CTextRam::fc_DECPQPLFM,NULL,	NULL	},
	{	_T("DECPS"),	&CTextRam::fc_DECPS,	NULL,	NULL	},
	{	_T("DECRARA"),	&CTextRam::fc_DECRARA,	NULL,	NULL	},
	{	_T("DECRQCRA"),	&CTextRam::fc_DECRQCRA,	NULL,	NULL	},
	{	_T("DECRQDE"),	&CTextRam::fc_DECRQDE,	NULL,	NULL	},
	{	_T("DECRQLP"),	&CTextRam::fc_DECRQLP,	NULL,	NULL	},
	{	_T("DECRQM"),	&CTextRam::fc_DECRQM,	NULL,	NULL	},
	{	_T("DECRQMH"),	&CTextRam::fc_DECRQMH,	NULL,	NULL	},
	{	_T("DECRQPSR"),	&CTextRam::fc_DECRQPSR,	NULL,	NULL	},
	{	_T("DECRQTSR"),	&CTextRam::fc_DECRQTSR,	NULL,	NULL	},
	{	_T("DECRQUPSS"),&CTextRam::fc_DECRQUPSS,NULL,	NULL	},
	{	_T("DECRST"),	&CTextRam::fc_DECRST,	NULL,	NULL	},
	{	_T("DECSACE"),	&CTextRam::fc_DECSACE,	NULL,	NULL	},
	{	_T("DECSASD"),	&CTextRam::fc_DECSASD,	NULL,	NULL	},
	{	_T("DECSCA"),	&CTextRam::fc_DECSCA,	NULL,	NULL	},
	{	_T("DECSCL"),	&CTextRam::fc_DECSCL,	NULL,	NULL	},
	{	_T("DECSCPP"),	&CTextRam::fc_DECSCPP,	NULL,	NULL	},
	{	_T("DECSCUSR"),	&CTextRam::fc_DECSCUSR,	NULL,	NULL	},
	{	_T("DECSED"),	&CTextRam::fc_DECSED,	NULL,	NULL	},
	{	_T("DECSEL"),	&CTextRam::fc_DECSEL,	NULL,	NULL	},
	{	_T("DECSERA"),	&CTextRam::fc_DECSERA,	NULL,	NULL	},
	{	_T("DECSET"),	&CTextRam::fc_DECSET,	NULL,	NULL	},
	{	_T("DECSHTS"),	&CTextRam::fc_DECSHTS,	NULL,	NULL	},
	{	_T("DECSLE"),	&CTextRam::fc_DECSLE,	NULL,	NULL	},
	{	_T("DECSLPP"),	&CTextRam::fc_DECSLPP,	NULL,	NULL	},
	{	_T("DECSLRM"),	&CTextRam::fc_DECSLRM,	NULL,	NULL	},
	{	_T("DECSR"),	&CTextRam::fc_DECSR,	NULL,	NULL	},
	{	_T("DECSSDT"),	&CTextRam::fc_DECSSDT,	NULL,	NULL	},
	{	_T("DECSSL"),	&CTextRam::fc_DECSSL,	NULL,	NULL	},
	{	_T("DECST8C"),	&CTextRam::fc_DECST8C,	NULL,	NULL	},
	{	_T("DECSTBM"),	&CTextRam::fc_DECSTBM,	NULL,	NULL	},
	{	_T("DECSTGLT"),	&CTextRam::fc_DECSTGLT,	NULL,	NULL	},
	{	_T("DECSTR"),	&CTextRam::fc_DECSTR,	NULL,	NULL	},
	{	_T("DECSVTS"),	&CTextRam::fc_DECSVTS,	NULL,	NULL	},
	{	_T("DECTID"),	&CTextRam::fc_DECTID,	NULL,	NULL	},
	{	_T("DECTME"),	&CTextRam::fc_DECTME,	NULL,	NULL	},
	{	_T("DECTST"),	&CTextRam::fc_DECTST,	NULL,	NULL	},
	{	_T("DL"),		&CTextRam::fc_DL,		NULL,	NULL	},
	{	_T("DSR"),		&CTextRam::fc_DSR,		NULL,	NULL	},
	{	_T("EA"),		&CTextRam::fc_EA,		NULL,	NULL	},
	{	_T("ECH"),		&CTextRam::fc_ECH,		NULL,	NULL	},
	{	_T("ED"),		&CTextRam::fc_ED,		NULL,	NULL	},
	{	_T("EF"),		&CTextRam::fc_EF,		NULL,	NULL	},
	{	_T("EL"),		&CTextRam::fc_EL,		NULL,	NULL	},
	{	_T("FNT"),		&CTextRam::fc_FNT,		NULL,	NULL	},
	{	_T("HPA"),		&CTextRam::fc_HPA,		NULL,	NULL	},
	{	_T("HPB"),		&CTextRam::fc_HPB,		NULL,	NULL	},
	{	_T("HPR"),		&CTextRam::fc_HPR,		NULL,	NULL	},
	{	_T("HVP"),		&CTextRam::fc_HVP,		NULL,	NULL	},
	{	_T("ICH"),		&CTextRam::fc_ICH,		NULL,	NULL	},
	{	_T("IL"),		&CTextRam::fc_IL,		NULL,	NULL	},
	{	_T("LINUX"),	&CTextRam::fc_LINUX,	NULL,	NULL	},
	{	_T("MC"),		&CTextRam::fc_MC,		NULL,	NULL	},
	{	_T("NOP"),		&CTextRam::fc_POP,		NULL,	NULL	},
	{	_T("NP"),		&CTextRam::fc_NP,		NULL,	NULL	},
	{	_T("ORGBFAT"),	&CTextRam::fc_ORGBFAT,	NULL,	NULL	},
	{	_T("ORGSCD"),	&CTextRam::fc_ORGSCD,	NULL,	NULL	},
	{	_T("PP"),		&CTextRam::fc_PP,		NULL,	NULL	},
	{	_T("PPA"),		&CTextRam::fc_PPA,		NULL,	NULL	},
	{	_T("PPB"),		&CTextRam::fc_PPB,		NULL,	NULL	},
	{	_T("PPR"),		&CTextRam::fc_PPR,		NULL,	NULL	},
	{	_T("REP"),		&CTextRam::fc_REP,		NULL,	NULL	},
	{	_T("REQTPARM"),	&CTextRam::fc_REQTPARM,	NULL,	NULL	},
	{	_T("RLIMGCP"),	&CTextRam::fc_RLIMGCP,	NULL,	NULL	},
	{	_T("RM"),		&CTextRam::fc_RM,		NULL,	NULL	},
	{	_T("SCORC"),	&CTextRam::fc_SCORC,	NULL,	NULL	},
	{	_T("SCOSC"),	&CTextRam::fc_SCOSC,	NULL,	NULL	},
	{	_T("SD"),		&CTextRam::fc_SD,		NULL,	NULL	},
	{	_T("SGR"),		&CTextRam::fc_SGR,		NULL,	NULL	},
	{	_T("SL"),		&CTextRam::fc_SL,		NULL,	NULL	},
	{	_T("SM"),		&CTextRam::fc_SM,		NULL,	NULL	},
	{	_T("SR"),		&CTextRam::fc_SR,		NULL,	NULL	},
	{	_T("SU"),		&CTextRam::fc_SU,		NULL,	NULL	},
	{	_T("TBC"),		&CTextRam::fc_TBC,		NULL,	NULL	},
	{	_T("TTIMERS"),	&CTextRam::fc_TTIMERS,	NULL,	NULL	},
	{	_T("TTIMEST"),	&CTextRam::fc_TTIMEST,	NULL,	NULL	},
	{	_T("TTIMESV"),	&CTextRam::fc_TTIMESV,	NULL,	NULL	},
	{	_T("VPA"),		&CTextRam::fc_VPA,		NULL,	NULL	},
	{	_T("VPB"),		&CTextRam::fc_VPB,		NULL,	NULL	},
	{	_T("VPR"),		&CTextRam::fc_VPR,		NULL,	NULL	},
	{	_T("XTMDKEY"),	&CTextRam::fc_XTMDKEY,	NULL,	NULL	},
	{	_T("XTMDKYD"),	&CTextRam::fc_XTMDKYD,	NULL,	NULL	},
	{	_T("XTREST"),	&CTextRam::fc_XTREST,	NULL,	NULL	},
	{	_T("XTRMTT"),	&CTextRam::fc_XTRMTT,	NULL,	NULL	},
	{	_T("XTSAVE"),	&CTextRam::fc_XTSAVE,	NULL,	NULL	},
	{	_T("XTSMTT"),	&CTextRam::fc_XTSMTT,	NULL,	NULL	},
	{	_T("XTWOP"),	&CTextRam::fc_XTWOP,	NULL,	NULL	},
	{	NULL,			NULL,					NULL,	NULL	},
};

static int	fc_DcsNameTabMax = 11;
static CTextRam::ESCNAMEPROC fc_DcsNameTab[] = {
	{	_T("DECDLD"),	&CTextRam::fc_DECDLD,	NULL,	NULL	},
	{	_T("DECDMAC"),	&CTextRam::fc_DECDMAC,	NULL,	NULL	},
	{	_T("DECREGIS"),	&CTextRam::fc_DECREGIS, NULL,	NULL	},
	{	_T("DECRQSS"),	&CTextRam::fc_DECRQSS,	NULL,	NULL	},
	{	_T("DECRSPS"),	&CTextRam::fc_DECRSPS,	NULL,	NULL	},
	{	_T("DECRSTS"),	&CTextRam::fc_DECRSTS,	NULL,	NULL	},
	{	_T("DECSIXEL"),	&CTextRam::fc_DECSIXEL, NULL,	NULL	},
	{	_T("DECSTUI"),	&CTextRam::fc_DECSTUI,	NULL,	NULL	},
	{	_T("DECUDK"),	&CTextRam::fc_DECUDK,	NULL,	NULL	},
	{	_T("NOP"),		&CTextRam::fc_POP,		NULL,	NULL	},
	{	_T("XTRQCAP"),	&CTextRam::fc_XTRQCAP,	NULL,	NULL	},
	{	NULL,			NULL,					NULL,	NULL	},
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
	m_pSection = _T("ProcTab");
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
void CProcTab::SetIndex(int mode, CStringIndex &index)
{
	int n;
	CProcNode node;

	if ( mode ) {
		index.SetSize(m_Data.GetSize());
		for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
			index[n][_T("Type")] = m_Data[n].m_Type;
			index[n][_T("Code")] = m_Data[n].m_Code;
			index[n][_T("Name")] = m_Data[n].m_Name;
		}
	} else {
		m_Data.RemoveAll();
		for ( n = 0 ; n < index.GetSize() ; n++ ) {
			node.m_Type = index[n][_T("Type")];
			node.m_Code = index[n][_T("Code")];
			node.m_Name = index[n][_T("Name")];
			m_Data.Add(node);
		}
	}
}
void CProcTab::SetArray(CStringArrayExt &stra)
{
	int n;
	CString tmp;

	stra.RemoveAll();
	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		tmp.Format(_T("%d,%d,%s,"), m_Data[n].m_Type, m_Data[n].m_Code, m_Data[n].m_Name);
		stra.Add(tmp);
	}
}
void CProcTab::GetArray(CStringArrayExt &stra)
{
	int n;
	CProcNode node;
	CStringArrayExt tmp;

	m_Data.RemoveAll();
	for ( n = 0 ; n < stra.GetSize() ; n++ ) {
		tmp.GetString(stra[n], ',');
		if ( tmp.GetSize() < 3  )
			continue;
		node.m_Type = _tstoi(tmp[0]);
		node.m_Code = _tstoi(tmp[1]);
		node.m_Name = tmp[2];
		m_Data.Add(node);
	}
}
void CProcTab::Add(int type, int code, LPCTSTR name)
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
	return _tcscmp(((CTextRam::ESCNAMEPROC *)src)->name, ((CTextRam::ESCNAMEPROC *)dis)->name);
}
static int	ExtTabCodeCmp(const void *src, const void *dis)
{
	return (*((int *)src) - ((CTextRam::CSIEXTTAB *)dis)->code);
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
	int i, n;
	ESCNAMEPROC *tp;

	if ( !fc_Init_Flag ) {
		fc_Init_Flag = TRUE;

		for ( i = 0; i < 256 ; i++ )
			fc_Proc[0][i] = &CTextRam::fc_IGNORE;

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

		fc_Init_Proc(STAGE_BIG5,  fc_CtrlTab);
		fc_Init_Proc(STAGE_BIG5,  fc_Big51Tab);
		fc_Init_Proc(STAGE_BIG52, fc_Big52Tab);

		fc_Init_Proc(STAGE_UTF8,  fc_CtrlTab);
		fc_Init_Proc(STAGE_UTF8,  fc_Utf8Tab);
		fc_Init_Proc(STAGE_UTF8,  fc_SEscTab);
		fc_Init_Proc(STAGE_UTF82, fc_Utf82Tab);

		fc_Init_Proc(STAGE_ESC, fc_CtrlTab);
		fc_Init_Proc(STAGE_ESC, fc_EscTab);
		fc_Init_Proc(STAGE_ESC, fc_Esc2Tab, 0x80);

		fc_Init_Proc(STAGE_GOTOXY, fc_CtrlTab);
		fc_Init_Proc(STAGE_GOTOXY, fc_GotoTab);

		fc_Init_Proc(STAGE_STAT, fc_CtrlTab);
		fc_Init_Proc(STAGE_STAT, fc_StatTab);

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
		fc_Init_Proc(STAGE_OSC2, fc_Osc2Tab);

		fc_Init_Proc(STAGE_TEK, fc_TekTab);			// TEK

		fc_pEscProc = fc_InitProcName(fc_EscNameTab, &fc_EscNameTabMax);
		fc_pCsiProc = fc_InitProcName(fc_CsiNameTab, &fc_CsiNameTabMax);
		fc_pDcsProc = fc_InitProcName(fc_DcsNameTab, &fc_DcsNameTabMax);
	}

	memcpy(m_LocalProc[0], fc_Proc[STAGE_ESC], sizeof(m_LocalProc[0]));
	memcpy(m_LocalProc[1], fc_Proc[STAGE_CSI], sizeof(m_LocalProc[1]));
	memcpy(m_LocalProc[2], fc_Proc[STAGE_EXT1], sizeof(m_LocalProc[2]));
	memcpy(m_LocalProc[3], fc_Proc[STAGE_EXT2], sizeof(m_LocalProc[3]));
	memcpy(m_LocalProc[4], fc_Proc[STAGE_EXT3], sizeof(m_LocalProc[4]));

	m_CsiExt.RemoveAll();
	for ( i = 0 ; fc_CsiExtTab[i].proc != NULL ; i++ ) {
		if ( !BinaryFind((void *)&(fc_CsiExtTab[i].code), m_CsiExt.GetData(), sizeof(CSIEXTTAB), m_CsiExt.GetSize(), ExtTabCodeCmp, &n) )
			m_CsiExt.InsertAt(n, (CTextRam::CSIEXTTAB)(fc_CsiExtTab[i]));
	}

	m_DcsExt.RemoveAll();
	for ( i = 0 ; fc_DcsExtTab[i].proc != NULL ; i++ ) {
		if ( !BinaryFind((void *)&(fc_DcsExtTab[i].code), m_DcsExt.GetData(), sizeof(CSIEXTTAB), m_DcsExt.GetSize(), ExtTabCodeCmp, &n) )
			m_DcsExt.InsertAt(n, (CTextRam::CSIEXTTAB)(fc_DcsExtTab[i]));
	}

	for ( n = 0 ; n < m_ProcTab.GetSize() ; n++ ) {
		switch(m_ProcTab[n].m_Type) {
		case PROCTYPE_ESC:
			EscNameProc(m_ProcTab[n].m_Code, m_ProcTab[n].m_Name);
			break;
		case PROCTYPE_CSI:
			CsiNameProc(m_ProcTab[n].m_Code, m_ProcTab[n].m_Name);
			break;
		case PROCTYPE_DCS:
			DcsNameProc(m_ProcTab[n].m_Code, m_ProcTab[n].m_Name);
			break;
		}
	}

	m_RetSync = FALSE;
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
	case BIG5_SET:
		fc_Case(STAGE_BIG5);
		break;
	}

	fc_TimerReset();
}
void CTextRam::fc_TraceCall(int ch)
{
#ifdef	DEBUG
	static struct {
		LPCTSTR		name;
		union {
			void (CTextRam::*proc)(int ch);
			BYTE byte[sizeof(void (CTextRam::*)(int))];
		} data;
	} funcname[] = {
		{ _T("A3CDW"),		&CTextRam::fc_A3CDW		},
		{ _T("A3CLT"),		&CTextRam::fc_A3CLT		},
		{ _T("A3CRT"),		&CTextRam::fc_A3CRT		},
		{ _T("A3CUP"),		&CTextRam::fc_A3CUP		},
		{ _T("BEL"),		&CTextRam::fc_BEL		},
		{ _T("BIG51"),		&CTextRam::fc_BIG51		},
		{ _T("BIG52"),		&CTextRam::fc_BIG52		},
		{ _T("BIG53"),		&CTextRam::fc_BIG53		},
		{ _T("BS"),			&CTextRam::fc_BS		},
		{ _T("CAN"),		&CTextRam::fc_CAN		},
		{ _T("CR"),			&CTextRam::fc_CR		},
		{ _T("CSI_DIGIT"),	&CTextRam::fc_CSI_DIGIT	},
		{ _T("CSI_ESC"),	&CTextRam::fc_CSI_ESC	},
		{ _T("CSI_EXT"),	&CTextRam::fc_CSI_EXT	},
		{ _T("CSI_PUSH"),	&CTextRam::fc_CSI_PUSH	},
		{ _T("CSI_SEPA"),	&CTextRam::fc_CSI_SEPA	},
		{ _T("DLE"),		&CTextRam::fc_DLE		},
		{ _T("ENQ"),		&CTextRam::fc_ENQ		},
		{ _T("ESC"),		&CTextRam::fc_ESC		},
		{ _T("FF"),			&CTextRam::fc_FF		},
		{ _T("HT"),			&CTextRam::fc_HT		},
		{ _T("IGNORE"),		&CTextRam::fc_IGNORE	},
		{ _T("LF"),			&CTextRam::fc_LF		},
		{ _T("OSC_CMD"),	&CTextRam::fc_OSC_CMD	},
		{ _T("OSC_PAM"),	&CTextRam::fc_OSC_PAM	},
		{ _T("POP"),		&CTextRam::fc_POP		},
		{ _T("RETRY"),		&CTextRam::fc_RETRY		},
		{ _T("SESC"),		&CTextRam::fc_SESC		},
		{ _T("SI"),			&CTextRam::fc_SI		},
		{ _T("SJIS1"),		&CTextRam::fc_SJIS1		},
		{ _T("SJIS2"),		&CTextRam::fc_SJIS2		},
		{ _T("SJIS3"),		&CTextRam::fc_SJIS3		},
		{ _T("SO"),			&CTextRam::fc_SO		},
		{ _T("SOH"),		&CTextRam::fc_SOH		},
		{ _T("STAT"),		&CTextRam::fc_STAT		},
		{ _T("TEK_DOWN"),	&CTextRam::fc_TEK_DOWN	},
		{ _T("TEK_FLUSH"),	&CTextRam::fc_TEK_FLUSH	},
		{ _T("TEK_LEFT"),	&CTextRam::fc_TEK_LEFT	},
		{ _T("TEK_MODE"),	&CTextRam::fc_TEK_MODE	},
		{ _T("TEK_RIGHT"),	&CTextRam::fc_TEK_RIGHT	},
		{ _T("TEK_STAT"),	&CTextRam::fc_TEK_STAT	},
		{ _T("TEK_UP"),		&CTextRam::fc_TEK_UP	},
		{ _T("TEXT"),		&CTextRam::fc_TEXT		},
		{ _T("TEXT2"),		&CTextRam::fc_TEXT2		},
		{ _T("TEXT3"),		&CTextRam::fc_TEXT3		},
		{ _T("UTF81"),		&CTextRam::fc_UTF81		},
		{ _T("UTF82"),		&CTextRam::fc_UTF82		},
		{ _T("UTF83"),		&CTextRam::fc_UTF83		},
		{ _T("UTF84"),		&CTextRam::fc_UTF84		},
		{ _T("UTF85"),		&CTextRam::fc_UTF85		},
		{ _T("UTF86"),		&CTextRam::fc_UTF86		},
		{ _T("UTF87"),		&CTextRam::fc_UTF87		},
		{ _T("UTF88"),		&CTextRam::fc_UTF88		},
		{ _T("UTF89"),		&CTextRam::fc_UTF89		},
		{ _T("VT"),			&CTextRam::fc_VT		},
		{ NULL,				NULL					},
	};
	static char *stage[] = {
		"ESC",		"CSI",		"EXT1",		"EXT2",		"EXT3",
		"EXT4",		"EUC",
		"94X94",	"96X96",
		"SJIS",		"SJIS2",
		"BIG5",		"BIG52",
		"UTF8",		"UTF82",
		"OSC1",		"OSC2",
		"TEK",
		"STAT",
	};
	int n;
	LPCTSTR name = _T("NOP");
	ESCNAMEPROC tmp;

	tmp.data.proc = m_Func[ch];
	for ( n = 0 ; funcname[n].name != NULL ; n++ ) {
		if ( memcmp(funcname[n].data.byte, tmp.data.byte, sizeof(void (CTextRam::*)(int))) == 0 )
			break;
	}
	if ( funcname[n].name != NULL )
		name = funcname[n].name;
	else {
		name = EscProcName(m_Func[ch]);
		if ( _tcscmp(name, _T("NOP")) == 0 ) {	
			name = CsiProcName(m_Func[ch]);
			if ( _tcscmp(name, _T("NOP")) == 0 )
				name = DcsProcName(m_Func[ch]);
		}
	}

	TRACE("%s : %02x %s\n", stage[m_Stage], ch, TstrToMbs(name));
#endif
}
inline void CTextRam::fc_Call(int ch)
{
//	fc_TraceCall(ch);
	(this->*m_Func[ch])(ch);
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

static int NameProcCmp(const void *src, const void *dis)
{
	return _tcscmp((LPCTSTR)src, ((CTextRam::ESCNAMEPROC *)dis)->name);
}
void CTextRam::EscNameProc(int ch, LPCTSTR name)
{
	int n;
	void (CTextRam::*proc)(int) = &CTextRam::fc_POP;

	if ( BinaryFind((void *)name, (void *)fc_EscNameTab, sizeof(ESCNAMEPROC), fc_EscNameTabMax, NameProcCmp, &n) )
		proc = fc_EscNameTab[n].data.proc;

	m_LocalProc[0][ch] = proc;
	m_LocalProc[0][ch | 0x80] = proc;
}
LPCTSTR	CTextRam::EscProcName(void (CTextRam::*proc)(int ch))
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
	return _T("NOP");
}
void CTextRam::SetEscNameCombo(CComboBox *pCombo)
{
	for ( int n = 0 ; n < fc_EscNameTabMax ; n++ )
		pCombo->AddString(fc_EscNameTab[n].name);
}

void CTextRam::CsiNameProc(int code, LPCTSTR name)
{
	int n;
	void (CTextRam::*proc)(int) = &CTextRam::fc_POP;
	CSIEXTTAB tmp;

	if ( BinaryFind((void *)name, (void *)fc_CsiNameTab, sizeof(ESCNAMEPROC), fc_CsiNameTabMax, NameProcCmp, &n) )
		proc = fc_CsiNameTab[n].data.proc;

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
		if ( BinaryFind((void *)&code, m_CsiExt.GetData(), sizeof(CSIEXTTAB), m_CsiExt.GetSize(), ExtTabCodeCmp, &n) )
			m_CsiExt[n].proc = proc;
		else {
			tmp.code = code;
			tmp.proc = proc;
			m_CsiExt.InsertAt(n, tmp);
		}
		break;
	}
}
LPCTSTR	CTextRam::CsiProcName(void (CTextRam::*proc)(int ch))
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
	return _T("NOP");
}
void CTextRam::SetCsiNameCombo(CComboBox *pCombo)
{
	for ( int n = 0 ; n < fc_CsiNameTabMax ; n++ )
		pCombo->AddString(fc_CsiNameTab[n].name);
}

void CTextRam::DcsNameProc(int code, LPCTSTR name)
{
	int n;
	void (CTextRam::*proc)(int) = &CTextRam::fc_POP;
	CSIEXTTAB tmp;

	if ( BinaryFind((void *)name, (void *)fc_DcsNameTab, sizeof(ESCNAMEPROC), fc_DcsNameTabMax, NameProcCmp, &n) )
		proc = fc_DcsNameTab[n].data.proc;

	if ( BinaryFind((void *)&code, m_DcsExt.GetData(), sizeof(CSIEXTTAB), m_DcsExt.GetSize(), ExtTabCodeCmp, &n) )
		m_DcsExt[n].proc = proc;
	else {
		tmp.code = code;
		tmp.proc = proc;
		m_DcsExt.InsertAt(n, tmp);
	}
}
LPCTSTR	CTextRam::DcsProcName(void (CTextRam::*proc)(int ch))
{
	int c;
	ESCNAMEPROC tmp;
	ESCNAMEPROC *tp;

	tmp.data.proc = proc;
	for ( tp = fc_pDcsProc ; tp != NULL ; ) {
		if ( (c = memcmp(tp->data.byte, tmp.data.byte, sizeof(void (CTextRam::*)(int)))) == 0 )
			return tp->name;
		else if ( c < 0 )
			tp = tp->left;
		else
			tp = tp->right;
	}
	return _T("NOP");
}
void CTextRam::SetDcsNameCombo(CComboBox *pCombo)
{
	for ( int n = 0 ; n < fc_DcsNameTabMax ; n++ )
		pCombo->AddString(fc_DcsNameTab[n].name);
}

void CTextRam::EscCsiDefName(LPCTSTR *esc, LPCTSTR *csi,  LPCTSTR *dcs)
{
	int n, c;

	for ( n = 0 ; n < 95 ; n++ )		// SP - ~ = 95
		esc[n] = _T("NOP");

	for ( n = 0 ; fc_Esc2Tab[n].proc != NULL ; n++ )
		esc[fc_Esc2Tab[n].sc - ' '] = EscProcName(fc_Esc2Tab[n].proc);

	for ( n = 0 ; n < 5355 ; n++ )		// (@ - ~) x (SP - / + 1) x (< - ? + 1) = 63 * 17 * 5 = 5355
		csi[n] = _T("NOP");

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

	for ( n = 0 ; n < 5355 ; n++ )		// (@ - ~) x (SP - / + 1) x (< - ? + 1) = 63 * 17 * 5 = 5355
		dcs[n] = _T("NOP");

	for ( n = 0 ; fc_DcsExtTab[n].proc != NULL ; n++ ) {
		c = (fc_DcsExtTab[n].code & 0x7F) - '@';
		if ( (fc_DcsExtTab[n].code & 0x7F0000) != 0 )
			c += ((((fc_DcsExtTab[n].code >> 16) & 0x7F) - '<' + 1) * 63 * 17);
		if ( (fc_DcsExtTab[n].code & 0x007F00) != 0 )
			c += ((((fc_DcsExtTab[n].code >>  8) & 0x7F) - ' ' + 1) * 63);
		dcs[c] = DcsProcName(fc_DcsExtTab[n].proc);
	}
}
void CTextRam::ParseColor(int cmd, int idx, LPCTSTR para, int ch)
{
	int r, g, b;

	if ( idx < 0 || idx >= EXTCOL_MAX )
		return;

	if ( para[0] == '?' ) {
		if ( (cmd >= 10 && cmd <= 19) || (cmd >= 110 && cmd <= 119) )
			UNGETSTR(_T("%s%d;rgb:%04x/%04x/%04x%s"), m_RetChar[RC_OSC], cmd,
				GetRValue(m_ColTab[idx]), GetGValue(m_ColTab[idx]), GetBValue(m_ColTab[idx]),
				(ch == 0x07 ? _T("\007") : m_RetChar[RC_ST]));
		else if ( cmd == 5 )
			UNGETSTR(_T("%s%d;%d;rgb:%04x/%04x/%04x%s"), m_RetChar[RC_OSC], cmd, idx - EXTCOL_SP_BEGIN,
				GetRValue(m_ColTab[idx]), GetGValue(m_ColTab[idx]), GetBValue(m_ColTab[idx]),
				(ch == 0x07 ? _T("\007") : m_RetChar[RC_ST]));
		else
			UNGETSTR(_T("%s%d;%d;rgb:%04x/%04x/%04x%s"), m_RetChar[RC_OSC], cmd, idx,
				GetRValue(m_ColTab[idx]), GetGValue(m_ColTab[idx]), GetBValue(m_ColTab[idx]),
				(ch == 0x07 ? _T("\007") : m_RetChar[RC_ST]));

	} else if ( para[0] == _T('#') ) {
		switch(_tcslen(para)) {
		case 4:		// #rgb
			if ( _stscanf(para, _T("#%01x%01x%01x"), &r, &g, &b) == 3 )
				m_ColTab[idx] = RGB(r, g, b);
			break;
		case 7:		// #rrggbb
			if ( _stscanf(para, _T("#%02x%02x%02x"), &r, &g, &b) == 3 )
				m_ColTab[idx] = RGB(r, g, b);
			break;
		case 10:	// #rrrgggbbb
			if ( _stscanf(para, _T("#%03x%03x%03x"), &r, &g, &b) == 3 )
				m_ColTab[idx] = RGB(r, g, b);
			break;
		case 13:	// #rrrrggggbbbb
			if ( _stscanf(para, _T("#%04x%04x%04x"), &r, &g, &b) == 3 )
				m_ColTab[idx] = RGB(r, g, b);
			break;
		}
		DISPUPDATE();

	} else if ( _tcsncmp(para, _T("rgb:"), 4) == 0 ) {
		if ( _stscanf(para, _T("rgb:%x/%x/%x"), &r, &g, &b) == 3 )
			m_ColTab[idx] = RGB(r, g, b);
		DISPUPDATE();

	} else if ( _tcscmp(para, _T("reset")) == 0 ) {
		if ( idx < 16 ) {
			m_ColTab[idx] = m_DefColTab[idx];
		} else if ( idx < 232 ) {				// colors 16-231 are a 6x6x6 color cube
			r = ((idx - 16) / 6 / 6) % 6;
			g = ((idx - 16) / 6) % 6;
			b = (idx - 16) % 6;
			m_ColTab[idx] = RGB(r * 51, g * 51, b * 51);
		} else if ( idx < 256 ) {			// colors 232-255 are a grayscale ramp, intentionally leaving out
			r = (idx - 232) * 11;
			g = (idx - 232) * 11;
			b = (idx - 232) * 11;
			m_ColTab[idx] = RGB(r, g, b);
		} else if ( idx == EXTCOL_VT_TEXT_FORE ) {
			m_ColTab[idx] = m_DefColTab[m_DefAtt.fc];
		} else if ( idx == EXTCOL_VT_TEXT_BACK ) {
			m_ColTab[idx] = m_DefColTab[m_DefAtt.bc];
		}
		DISPUPDATE();
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
void CTextRam::fc_CESC(int ch)
{
	fc_KANJI(ch);
	CallReciveChar(ch);
	ch &= 0x1F;
	ch += '@';
	fc_Case(STAGE_ESC);
	fc_Call(ch);
}

//////////////////////////////////////////////////////////////////////
// fc KANJI

static const WORD UnDefCp932[] = {	// 47
	0x81AD, 0x81B8, 0x81C0, 0x81C8, 0x81CF, 0x81DA, 0x81E9, 0x81F0, 
	0x81F8, 0x81FC, 0x8240, 0x824F, 0x8259, 0x8260, 0x827A, 0x8281, 
	0x829B, 0x829F, 0x82F2, 0x8340, 0x8397, 0x839F, 0x83B7, 0x83BF, 
	0x83D7, 0x8440, 0x8461, 0x8470, 0x8492, 0x849F, 0x84BF, 0x8740, 
	0x875E, 0x875F, 0x8776, 0x877E, 0x879D, 0x889F, 0x9873, 0x989F, 
	0xEAA5, 0xED40, 0xEEED, 0xEEEF, 0xEF40, 0xF040, 0xFC4C, 
};
static const WORD UnDefEuc1[] = {	// 42	EUCJP-MS-1
	0xA2AF, 0xA2BA, 0xA2C2, 0xA2CA, 0xA2D1, 0xA2DC, 0xA2EB, 0xA2F2,
	0xA2FA, 0xA2FE, 0xA3A1, 0xA3B0, 0xA3BA, 0xA3C1, 0xA3DB, 0xA3E1,
	0xA3FB, 0xA4A1, 0xA4F4, 0xA5A1, 0xA5F7, 0xA6A1, 0xA6B9, 0xA6C1,
	0xA6D9, 0xA7A1, 0xA7C2, 0xA7D1, 0xA7F2, 0xA8A1, 0xA8C1, 0xADA1,
	0xADBF, 0xADC0, 0xADD7, 0xADDF, 0xADFD, 0xB0A1, 0xCFD4, 0xD0A1,
	0xF4A7, 0xF5A1,
};
static const WORD UnDefEuc2[] = {	// 48	EUCJP-MS-2
	0xA1A1, 0xA2AF, 0xA2BA, 0xA2C2, 0xA2C5, 0xA2EB, 0xA2F2, 0xA6E1, 
	0xA6E6, 0xA6E7, 0xA6E8, 0xA6E9, 0xA6EB, 0xA6EC, 0xA6ED, 0xA6F1, 
	0xA6FD, 0xA7C2, 0xA7CF, 0xA7F2, 0xA8A1, 0xA9A1, 0xA9A3, 0xA9A4, 
	0xA9A5, 0xA9A6, 0xA9A7, 0xA9A8, 0xA9AA, 0xA9AB, 0xA9AE, 0xA9AF, 
	0xA9B1, 0xA9C1, 0xA9D1, 0xAAA1, 0xAAB9, 0xAABA, 0xAAF8, 0xABA1, 
	0xABBC, 0xABBD, 0xABC4, 0xABC5, 0xABF8, 0xB0A1, 0xEDE4, 0xF3F3, 
};

static int IsKanjiCode(WORD code, const WORD *tab, int len)
{
	int n, b, m;

	b = 0;
	m = len - 1;
	while ( b <= m ) {
		n = (b + m) / 2;
		if ( code == tab[n] )
			return (n & 1);
		else if ( code > tab[n] )
			b = n + 1;
		else
			m = n - 1;
	}
	return ((b - 1) & 1);
}
void CTextRam::fc_KANJI(int ch)
{
	if ( ch >= 128 || m_Kan_Buf[(m_Kan_Pos - 1) & (KANBUFMAX - 1)] >= 128 ) {
		m_Kan_Buf[m_Kan_Pos++] = ch; 
		m_Kan_Pos &= (KANBUFMAX - 1);
//		if ( m_Kan_Pos == 0 ) fc_KANCHK();
	}
}
void CTextRam::fc_KANCHK()
{
	int n, ch, skip = 1;
	int sjis_st = 0, sjis_bk = 0;
	int euc_st = 0, euc_bk = 0;
	int utf8_st = 0;
	double sjis_rs = 0.5;
	double euc_rs = 0.5;
	double utf8_rs = 0.5;

#define	POSITIVE(r)		(1.0 - (1.0 - (r)) * 0.9)
#define	HPOSITIVE(r)	(1.0 - (1.0 - (r)) * 0.99)
#define	NEGATIVE(r)		((r) * 0.7)

	for ( n = m_Kan_Pos + 1; n != m_Kan_Pos ; n = (n + 1) & (KANBUFMAX - 1) ) {
		ch = m_Kan_Buf[n];

		if ( skip && ch >= 128 )
			continue;
		skip = 0;

		// SJIS
		// 1 Byte	0x81 - 0x9F or 0xE0 - 0xFC or 0xA0-0xDF
		// 2 Byte	0x40 - 0x7E or 0x80 - 0xFC
		switch(sjis_st) {
		case 0:
			sjis_bk = ch;
			if ( issjis1(ch) )
				sjis_st = 1;
			else if ( iskana(ch) )
				sjis_rs = HPOSITIVE(sjis_rs);
			else if ( (ch & 0x80) != 0 )
				sjis_rs = NEGATIVE(sjis_rs);
			break;
		case 1:
			sjis_st = 0;
			if ( issjis2(ch) && IsKanjiCode((sjis_bk << 8) | ch, UnDefCp932, 47) )
				sjis_rs = POSITIVE(sjis_rs);
			else
				sjis_rs = NEGATIVE(sjis_rs);
			break;
		}

		// EUC
		// 1 Byte	0xA1 - 0xFE		or 0x8F			or 0x8E
		// 2 Byte	0xA1 - 0xFE		0xA1 - 0xFE		0xA0 - 0xDF
		// 3 Byte					0xA1 - 0xFE

		switch(euc_st) {
		case 0:
			euc_bk = ch;
			if ( ch >= 0xA1 && ch <= 0xFE )
				euc_st = 1;
			else if ( ch == 0x8E )
				euc_st = 4;
			else if ( ch == 0x8F )
				euc_st = 2;
			else if ( (ch & 0x80) != 0 )
				euc_rs = NEGATIVE(euc_rs);
			break;
		case 1:		// EUCJP-MS-1	2 Byte
			euc_st = 0;
			if ( ch >= 0xA1 && ch <= 0xFE && IsKanjiCode((euc_bk << 8) | ch, UnDefEuc1, 42) )
				euc_rs = POSITIVE(euc_rs);
			else
				euc_rs = NEGATIVE(euc_rs);
			break;
		case 2:		// EUCJP-MS-2	1 Byte
			euc_bk = ch;
			if ( ch >= 0xA1 && ch <= 0xFE )
				euc_st = 3;
			else {
				euc_st = 0;
				euc_rs = NEGATIVE(euc_rs);
			}
			break;
		case 3:		// EUCJP-MS-2	2 Byte
			euc_st = 0;
			if ( ch >= 0xA1 && ch <= 0xFE && IsKanjiCode((euc_bk << 8) | ch, UnDefEuc2, 48) )
				euc_rs = POSITIVE(euc_rs);
			else
				euc_rs = NEGATIVE(euc_rs);
			break;
		case 4:		// Kana
			euc_st = 0;
			if ( ch >= 0xA0 && ch <= 0xDF )
				euc_rs = POSITIVE(euc_rs);
			else
				euc_rs = NEGATIVE(euc_rs);
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
				utf8_rs = NEGATIVE(utf8_rs);
			break;
		case 1:
			if ( ch >= 0x80 && ch <= 0xBF )
				utf8_st = 2;
			else {
				utf8_st = 0;
				utf8_rs = NEGATIVE(utf8_rs);
			}
			break;
		case 2:
			if ( ch >= 0x80 && ch <= 0xBF )
				utf8_st = 3;
			else {
				utf8_st = 0;
				utf8_rs = NEGATIVE(utf8_rs);
			}
			break;
		case 3:
			utf8_st = 0;
			if ( ch >= 0x80 && ch <= 0xBF )
				utf8_rs = POSITIVE(utf8_rs);
			else
				utf8_rs = NEGATIVE(utf8_rs);
			break;
		case 4:
			utf8_st = 0;
			if ( ch >= 0xFE && ch <= 0xFF )
				utf8_rs = POSITIVE(utf8_rs);
			else
				utf8_rs = NEGATIVE(utf8_rs);
			break;
		}
	}

	n = m_KanjiMode;

	TRACE("SJIS %f, EUC %f, UTF8 %f\n", sjis_rs, euc_rs, utf8_rs);

	if ( sjis_rs > 0.7 && sjis_rs > euc_rs && sjis_rs > utf8_rs )
		n = SJIS_SET;
	else if ( euc_rs > 0.7 && euc_rs > sjis_rs && euc_rs > utf8_rs )
		n = EUC_SET;
	else if ( utf8_rs > 0.7 && utf8_rs > sjis_rs && utf8_rs > euc_rs )
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
		//if ( (ch &= 0x7F) == 0x7F )
		//	break;
		//INSMDCK(1);
		//PUT1BYTE(ch, m_BankNow);
		//break;
	case SET_96:
		ch &= 0x7F;
		INSMDCK(1);
		PUT1BYTE(ch, m_BankNow);
		break;
	case SET_94x94:
		if ( (ch & 0x7F) < 0x21 || (ch & 0x7F) > 0x7E )
			break;
		fc_Push(STAGE_94X94);
		goto CODELEN;
	case SET_96x96:
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
void CTextRam::fc_BIG51(int ch)
{
	m_BackChar = ch;
	m_BankNow  = SET_UNICODE;
	fc_Push(STAGE_BIG52);
}
void CTextRam::fc_BIG52(int ch)
{
	int n;

	m_BackChar = (m_BackChar << 8) | ch;
	if ( (n = m_IConv.IConvChar(m_SendCharSet[BIG5_SET], _T("UTF-16BE"), m_BackChar)) == 0 )
		n = 0x25A1;
	INSMDCK(2);
	PUT2BYTE(n, m_BankNow);
	fc_POP(ch);
}
void CTextRam::fc_BIG53(int ch)
{
	fc_POP(ch);

	if ( ch < 0x20 )
		fc_Call(ch);
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
	int n, cf;
	CVram *vp;

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
		if ( m_BackChar > UNICODE_MAX )			// U+000000 - U+10FFFF 21 bit
			m_BackChar = UNICODE_UNKOWN;		// □

		// 以降、サロゲートされたUTF-16で処理
		m_BackChar = UCS4toUCS2(m_BackChar);
		cf = UnicodeCharFlag(m_BackChar);

		if ( !IsOptEnable(TO_RLUNINOM) ) {		// TO_RLUNINOM(8448)が解除(default)

			// Unicode Normalization
			// ２文字から１文字のみノーマライズ
			// U+1100 U+1161        -> U+AC00
			// U+1100 U+1161 U+11A8 -> U+AC00 U+11A8 -> U+AC01
			if ( (n = UnicodeNomal(m_LastChar, m_BackChar)) != 0 ) {
				m_LastChar = 0;
				m_LastFlag = 0;
				m_BackChar = n;
				cf = UnicodeCharFlag(m_BackChar);
				LOCATE(m_LastPos % COLS_MAX, m_LastPos / COLS_MAX);
			}

			// Mark文字をそれなりに解釈
			// 200E;N # LEFT-TO-RIGHT MARK
			// 200F;N # RIGHT-TO-LEFT MARK
			// 202A;N # LEFT-TO-RIGHT EMBEDDING
			// 202B;N # RIGHT-TO-LEFT EMBEDDING
			// 202C;N # POP DIRECTIONAL FORMATTING
			// 202D;N # LEFT-TO-RIGHT OVERRIDE
			// 202E;N # RIGHT-TO-LEFT OVERRIDE
			if ( (cf & UNI_CTL) != 0 ) {
				switch(m_BackChar) {
				case 0x200E:	// LEFT-TO-RIGHT MARK
				case 0x202A:	// LEFT-TO-RIGHT EMBEDDING
				case 0x202D:	// LEFT-TO-RIGHT OVERRIDE
					//PushRtl(FALSE);
					m_bRtoL = FALSE;
					break;
				case 0x200F:	// RIGHT-TO-LEFT MARK
				case 0x202B:	// RIGHT-TO-LEFT EMBEDDING
				case 0x202E:	// RIGHT-TO-LEFT OVERRIDE
					//PushRtl(TRUE);
					m_bRtoL = TRUE;
					break;
				case 0x202C:	// POP DIRECTIONAL FORMATTING
					//PopRtl();
					m_bRtoL = FALSE;
					break;
				}
				goto BREAK;
			}

			// ノーマライズされないハングル字母は、重ねて表示(表示が変!)
			// U+1100-11FF ハングル字母
			// U+1100-115F は初声子音
			// U+1160-11A2 は中声母音
			// U+11A8-11F9 は終声子音
			if ( (m_LastFlag & UNI_HNF) != 0 ) {
				if ( (cf & UNI_HNM) != 0 ) {
					PUTADD(m_LastPos % COLS_MAX, m_LastPos / COLS_MAX, m_BackChar, cf);
					goto BREAK;
				} else if ( (cf & UNI_HNL) != 0 ) {
					PUTADD(m_LastPos % COLS_MAX, m_LastPos / COLS_MAX, m_BackChar, cf);
					m_LastChar = m_LastFlag = 0;
					goto BREAK;
				}
			}
		}

		// VARIATION SELECTOR
		// U+180B - U+180D
		// U+FE00 - U+FE0F
		// U+E0100 = U+DB40 U+DD00 - U+E01EF = U+DB40 U+DDEF
		// Non Spaceing Mark (with VARIATION SELECTOR !!)
		if ( m_LastChar > 0 && (cf & (UNI_IVS | UNI_NSM)) != 0 ) {
			PUTADD(m_LastPos % COLS_MAX, m_LastPos / COLS_MAX, m_BackChar, cf);
			goto BREAK;
		}

		// U+100000-10FFFD 16面外字
		// U+10ddcc
		//     dd		dscs x256
		//       cc		code x256
		// U+100000 = U+DBC0 U+DC00
		// U+10FFFF = U+DBFF U+DFFF
		if ( m_BackChar >= 0xDBC0DC00L && m_BackChar <= 0xDBFFDFFFL && IsOptEnable(TO_DRCSMMv1) ) {
			n = UCS2toUCS4(m_BackChar);
			CString tmp;
			tmp.Format(_T(" %c"), (n >> 8) & 0xFF);
			INSMDCK(1);
			PUT1BYTE(n & 0xFF, m_FontTab.IndexFind(SET_94, tmp));
			m_LastFlag = cf;
			goto BREAK;
		}

		if ( (cf & UNI_WID) != 0 )
			n = 2;
		else if ( (cf & UNI_AMB) != 0 )
			n = (IsOptEnable(TO_RLUNIAWH) ? 1 : 2);
		else
			n = 1;

		// 1 Cell type Unicode
		if ( n == 1 ) {
			INSMDCK(1);
			if ( m_BackChar < 0x0080 )
				PUT1BYTE(m_BackChar & 0x7F, m_BankTab[m_KanjiMode][0]);
			else if ( m_BackChar < 0x0100 )
				PUT1BYTE(m_BackChar & 0x7F, m_BankTab[m_KanjiMode][1]);
			else
				PUT1BYTE(m_BackChar, SET_UNICODE, ((cf & UNI_RTL) ? ATT_RTOL : 0));
			m_LastFlag = cf;

		// 2 Cell type Unicode
		} else {
			INSMDCK(2);
			PUT2BYTE(m_BackChar, SET_UNICODE, ((cf & UNI_RTL) ? ATT_RTOL : 0));
			m_LastFlag = cf;
		}

	BREAK:
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
	CallReciveChar(ch);

	m_DoWarp = FALSE;

	GetMargin(MARCHK_NONE);

	if ( --m_CurX < m_Margin.left ) {
		if ( IsOptEnable(TO_XTMRVW) != 0 )
			m_CurX = m_Margin.right - 1;
		else
			m_CurX = m_Margin.left;
	}
}
void CTextRam::fc_HT(int ch)
{
	fc_KANJI(ch);
	CallReciveChar(ch);

	TABSET(TAB_COLSNEXT);
}
void CTextRam::fc_LF(int ch)
{
	fc_KANJI(ch);
	CallReciveChar(ch);

	switch(IsOptEnable(TO_ANSILNM) ? 2 : m_RecvCrLf) {
	case 2:		// LF
	case 3:		// CR|LF
		LOCATE(GetLeftMargin(), m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag == DELAY_WAIT )
			m_pDocument->OnDelayRecive(ch);
	case 0:		// CR+LF
		ONEINDEX();
	case 1:		// CR
		break;
	}
}
void CTextRam::fc_VT(int ch)
{
	CallReciveChar(ch);

	TABSET(TAB_LINENEXT);

	if ( IsOptEnable(TO_ANSILNM) )
		LOCATE(GetLeftMargin(), m_CurY);
}
void CTextRam::fc_FF(int ch)
{
	CallReciveChar(ch);

	ONEINDEX();

	if ( IsOptEnable(TO_ANSILNM) )
		LOCATE(GetLeftMargin(), m_CurY);
}
void CTextRam::fc_CR(int ch)
{
	fc_KANJI(ch);
	CallReciveChar(ch);

	switch(m_RecvCrLf) {
	case 1:		// CR
	case 3:		// CR|LF
		ONEINDEX();
	case 0:		// CR+LF
		LOCATE(GetLeftMargin(), m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag == DELAY_WAIT )
			m_pDocument->OnDelayRecive(ch);
	case 2:		// LF
		break;
	}
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
	CallReciveChar(ch);
	fc_Push(STAGE_ESC);
}
void CTextRam::fc_A3CRT(int ch)
{
	// ADM-3 Cursole Right
	CallReciveChar(ch);
	LOCATE(m_CurX + 1, m_CurY);
}
void CTextRam::fc_A3CLT(int ch)
{
	// ADM-3 Cursole Left
	CallReciveChar(ch);
	LOCATE(m_CurX - 1, m_CurY);
}
void CTextRam::fc_A3CUP(int ch)
{
	// ADM-3 Cursole Up
	CallReciveChar(ch);
	LOCATE(m_CurX, m_CurY - 1);
}
void CTextRam::fc_A3CDW(int ch)
{
	// ADM-3 Cursole Down
	CallReciveChar(ch);
	LOCATE(m_CurX, m_CurY + 1);
}

//////////////////////////////////////////////////////////////////////
// fc ESC

void CTextRam::fc_DECHTS(int ch)
{
	// ESC 1	DECHTS Horizontal tab set
	TABSET(TAB_COLSSET);
	fc_POP(ch);
}
void CTextRam::fc_DECAHT(int ch)
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
void CTextRam::fc_DECAVT(int ch)
{
	// ESC 4	DECCAVT Clear all vertical tabs
	TABSET(TAB_LINEALLCLR);
	fc_POP(ch);
}
void CTextRam::fc_BI(int ch)
{
	// ESC 6	DECBI Back Index
	if ( m_CurX == GetLeftMargin() )
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
	m_Save_Decom  = (IsOptEnable(TO_DECOM) ? TRUE : FALSE);
	memcpy(m_Save_BankTab, m_BankTab, sizeof(m_BankTab));
//	memcpy(m_Save_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
//	memcpy(m_Save_TabMap, m_TabMap, sizeof(m_TabMap));
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
	SetOption(TO_DECOM, m_Save_Decom);
	memcpy(m_BankTab, m_Save_BankTab, sizeof(m_BankTab));
//	memcpy(m_AnsiOpt, m_Save_AnsiOpt, sizeof(m_AnsiOpt));
//	memcpy(m_TabMap, m_Save_TabMap, sizeof(m_TabMap));
	fc_POP(ch);
}
void CTextRam::fc_FI(int ch)
{
	// ESC 9	DECFI Forward Index
	if ( (m_CurX + 1) == GetRightMargin() )
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
	if ( !IsOptEnable(TO_DECANM) )
		LOCATE(m_CurX, m_CurY + 1);
	fc_POP(ch);
}
void CTextRam::fc_NBH(int ch)
{
	// ESC C	VT52 Cursor right								ANSI NBH No break here
	if ( !IsOptEnable(TO_DECANM) )
		LOCATE(m_CurX + 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_IND(int ch)
{
	// ESC D	VT52 Cursor left								ANSI IND Index
	if ( !IsOptEnable(TO_DECANM) )
		LOCATE(m_CurX - 1, m_CurY);
	else
		ONEINDEX();
	fc_POP(ch);
}
void CTextRam::fc_NEL(int ch)
{
	// ESC E													ANSI NEL Next Line

	LOCATE((IsOptEnable(TO_DECOM) ? GetLeftMargin() : 0), m_CurY);
//	LOCATE(GetLeftMargin(), m_CurY);
	ONEINDEX();

	fc_POP(ch);
}
void CTextRam::fc_SSA(int ch)
{
	// ESC F	VT52 Enter graphics mode.						ANSI SSA Start selected area
	if ( !IsOptEnable(TO_DECANM) ) {
		m_BankTab[m_KanjiMode][0] = SET_94 | '0';
		m_BankTab[m_KanjiMode][1] = SET_94 | '0';
	}
	// else
	//	m_AttNow.em |= EM_SELECTED;

	fc_POP(ch);
}
void CTextRam::fc_ESA(int ch)
{
	// ESC F	VT52 Exit graphics mode.						ANSI ESA End selected area
	if ( !IsOptEnable(TO_DECANM) ) {
		m_BankTab[m_KanjiMode][0] = SET_94 | 'B';
		m_BankTab[m_KanjiMode][1] = SET_94 | 'B';
	}
	// else
	//	m_AttNow.em &= ~EM_SELECTED;

	fc_POP(ch);
}
void CTextRam::fc_HTS(int ch)
{
	// ESC H	VT52 Cursor to home position.					ANSI HTS Character tabulation set
	if ( !IsOptEnable(TO_DECANM) )
		LOCATE(0, 0);
	else
		TABSET(TAB_COLSSET);
	fc_POP(ch);
}
void CTextRam::fc_HTJ(int ch)
{
	// ESC I	VT52 Reverse line feed.							ANSI HTJ Character tabulation with justification
	if ( !IsOptEnable(TO_DECANM) )
		REVINDEX();
	else
		TABSET(TAB_COLSNEXT);
	fc_POP(ch);
}
void CTextRam::fc_VTS(int ch)
{
	// ESC J	VT52 Erase from cursor to end of screen.		ANSI VTS Line tabulation set
	if ( !IsOptEnable(TO_DECANM) ) {
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines);
	} else
		TABSET(TAB_LINESET);
	fc_POP(ch);
}
void CTextRam::fc_PLD(int ch)
{
	// ESC K	VT52 Erase from cursor to end of line.			ANSI PLD Partial line forward
	if ( !IsOptEnable(TO_DECANM) )
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, ERM_SAVEDM);
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
	if ( IsOptEnable(TO_DECANM) )
		m_AttNow.em |= EM_ISOPROTECT;
	fc_POP(ch);
}
void CTextRam::fc_EPA(int ch)
{
	// ESC W	VT52 Enter printer controller mode.				ANSI EPA End of guarded area
	if ( IsOptEnable(TO_DECANM) )
		m_AttNow.em &= ~EM_ISOPROTECT;
	fc_POP(ch);
}
void CTextRam::fc_V5MCP(int ch)
{
	// ESC Y	VT52 Move cursor to column Pn.
	m_Status = ST_COLM_SET;
	fc_Case(STAGE_GOTOXY);
}
void CTextRam::fc_SCI(int ch)
{
	// ESC Z	VT52 Identify (host to terminal).				DECID(DA1) Primary Device Attributes		// ANSI SCI Single character introducer
	if ( !IsOptEnable(TO_DECANM) ) {
		UNGETSTR(_T("\033/Z"));
		fc_POP(ch);
	} else {
		m_AnsiPara.RemoveAll();
		m_AnsiPara.Add(PARA_NOT);
		fc_DA1(ch);
	}
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
	EnableOption(TO_DECANM);
	fc_POP(ch);
}
void CTextRam::fc_DECPAM(int ch)
{
	// ESC =	DECPAM Application Keypad				VT52 Enter alternate keypad mode.
	EnableOption(TO_RLPNAM);
	fc_POP(ch);
}
void CTextRam::fc_DECPNM(int ch)
{
	// ESC >	DECPNM Normal Keypad						VT52 Exit alternate keypad mode.
	DisableOption(TO_RLPNAM);
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
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC0(int ch)
{
	// ESC (	CSC0 G0 charset
	m_BackChar = 0;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC1(int ch)
{
	// ESC )	CSC1 G1 charset
	m_BackChar = 1;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC2(int ch)
{
	// ESC *	CSC2 G2 charset
	m_BackChar = 2;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC3(int ch)
{
	// ESC +	CSC3 G3 charset
	m_BackChar = 3;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC0A(int ch)
{
	// ESC ,	CSC0A G0 charset
	m_BackChar = 0;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC1A(int ch)
{
	// ESC -	CSC1A G1 charset
	m_BackChar = 1;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC2A(int ch)
{
	// ESC .	CSC2A G2 charset
	m_BackChar = 2;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC3A(int ch)
{
	// ESC /	CSC3A G3 charset
	m_BackChar = 3;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	m_StrPara.Empty();
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_DECSOP(int ch)
{
	// ESC #	DEC Screen Opt
	m_BackChar = 0;
	m_Status = ST_DEC_OPT;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_ACS(int ch)
{
	// ESC 20	ACS
	m_BackChar = 0;
	m_Status = ST_CONF_LEVEL;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_ESCI(int ch)
{
	// ESC 2/x
	m_BackChar = 0;
	m_Status = ST_NULL;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_DOCS(int ch)
{
	// ESC %	DOCS Designate other coding system
	m_BackChar = 0;
	m_Status = ST_DOCS_OPT;
	fc_Case(STAGE_STAT);
}

//////////////////////////////////////////////////////////////////////
// fc ESC ...

void CTextRam::fc_GOTOXY(int ch)
{
	switch(m_Status) {
	case ST_COLM_SET:
		m_Status = ST_COLM_SET_2;
		m_BackChar = ch;
		break;
	case ST_COLM_SET_2:
		LOCATE(ch - ' ', m_BackChar - ' ');
		fc_POP(ch);
		break;
	}
}
void CTextRam::fc_STAT(int ch)
{
	switch(m_Status) {
	case ST_NULL:
		ch &= 0x7F;
		m_BackChar = (m_BackChar << 8) | ch;
		if ( ch >= 0x20 && ch <= 0x2F )
			return;
		break;

	case ST_CONF_LEVEL:			// ESC 20...	ACS
		ch &= 0x7F;
		m_BackChar = (m_BackChar << 8) | ch;
		if ( ch >= 0x20 && ch <= 0x2F )
			return;

		switch(m_BackChar) {
		case 'F':				// S7C1T Send 7-bit C1 Control character to host
			SetRetChar(FALSE);
			break;
		case 'G':				// S8C1T Send 8-bit C1 Control character to host
			SetRetChar(TRUE);
			break;

		case 'L':				// Level 1		G0=ASCII G1=Latan GL=G0 GR=G1
		case 'M':				// Level 2		G0=ASCII G1=Latan GL=G0 GR=G1
			m_BankTab[m_KanjiMode][0] = SET_94 | 'B';
			m_BankTab[m_KanjiMode][1] = SET_96 | 'A';
			m_BankGL = 0;
			m_BankGR = 1;
			break;
		case 'N':				// Level 3		G0=ASCII GL=G0
			m_BankTab[m_KanjiMode][0] = SET_94 | 'B';
			m_BankGL = 0;
			break;
		}
		break;

	case ST_DEC_OPT:			// ESC #...
		ch &= 0x7F;
		m_BackChar = (m_BackChar << 8) | ch;
		if ( ch >= 0x20 && ch <= 0x2F )
			return;

		switch(m_BackChar) {
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
			{
				int n, i;
				CVram *vp;

				for ( n = 0 ; n < m_Lines ; n++ ) {
					vp = GETVRAM(0, n);
					for ( i = 0 ; i < m_Cols ; i++ ) {
						*vp = m_AttNow;
						vp->pr.md = m_BankTab[m_KanjiMode][0];
						*vp = (DWORD)'E';
						vp++;
					}
				}
				RESET(RESET_CURSOR);
				DISPVRAM(0, 0, m_Cols, m_Lines);
			}
			break;
		case '9':		// DECFPP Perform pending motion
			break;
		}
		break;


	case ST_DOCS_OPT:			// ESC %...		DOCS
		ch &= 0x7F;
		m_BackChar = (m_BackChar << 8) | ch;

		switch(m_BackChar) {
		case '8':	// LINUX switch to UTF-8
		case '@': 	// ECMA-35 return to ECMA-35 coding system
		case 'A':	// switch to CSA T 500-1983
		case 'B':	// switch to UCS Transformation Format One (UTF-1)
		case 'C':	// switch to Data Syntax I of CCITT Rec.T.101 
		case 'D':	// switch to Data Syntax II of CCITT Rec.T.101
		case 'E':	// switch to Photo-Videotex Data Syntax of CCITT Rec. T.101
		case 'F': 	// switch to Audio Data Syntax of CCITT Rec. T.101
		case 'G': 	// switch to UTF-8
		case 'H':	// switch to VEMMI Data Syntax of ITU-T Rec. T.107
			break;

		case ('/' << 8) | '@':	// switch to ISO/IEC 10646:1993, UCS-2, Level 1
		case ('/' << 8) | 'A':	// switch to ISO/IEC 10646:1993, UCS-4, Level 1
		case ('/' << 8) | 'B':	// Switch to Virtual Terminal service Transparent Set
		case ('/' << 8) | 'C':	// switch to ISO/IEC 10646:1993, UCS-2, Level 2
		case ('/' << 8) | 'D':	// switch to ISO/IEC 10646:1993, UCS-4, Level 2
		case ('/' << 8) | 'E':	// switch to ISO/IEC 10646:1993, UCS-2, Level 3
		case ('/' << 8) | 'F':	// switch to ISO/IEC 10646:1993, UCS-4, Level 3
		case ('/' << 8) | 'G':	// switch to UTF-8 Level 1
		case ('/' << 8) | 'H':	// switch to UTF-8 Level 2
		case ('/' << 8) | 'I':	// switch to UTF-8 Level 3
		case ('/' << 8) | 'J':	// switch to UTF-16 Level 1
		case ('/' << 8) | 'K':	// switch to UTF-16 Level 2
		case ('/' << 8) | 'L':	// switch to UTF-16 Level 3
			break;

		case '!':
			m_Status = ST_TEK_OPT;
			m_Tek_Int = 0;
			return;

		default:
			if ( ch >= 0x20 && ch <= 0x2F )
				return;
			break;
		}

	case ST_TEK_OPT:			// ESC %!...
		if ( ch >= 0x40 && ch <= 0x7F ) {
			m_Tek_Int = (m_Tek_Int << 6) | (ch & 0x3F);
			return;
		} else if ( ch >= 0x20 && ch <= 0x3F ) {
			m_Tek_Int = (m_Tek_Int << 4) | (ch & 0x0F);
			if ( (ch & 0x10) == 0 )
				m_Tek_Int = 0 - m_Tek_Int;
			if ( m_Tek_Int == 0 ) {		//  set tek mode
				TekInit(4105);
				fc_Case(STAGE_TEK);
				return;
			}
		}
		break;

	case ST_CHARSET_2:
		     if ( ch == '(' ) { m_BackChar = 0; m_BackMode = SET_94x94; m_Status = ST_CHARSET_1; return; }
		else if ( ch == ')' ) { m_BackChar = 1; m_BackMode = SET_94x94; m_Status = ST_CHARSET_1; return; }
		else if ( ch == '+' ) { m_BackChar = 2; m_BackMode = SET_94x94; m_Status = ST_CHARSET_1; return; }
		else if ( ch == '*' ) { m_BackChar = 3; m_BackMode = SET_94x94; m_Status = ST_CHARSET_1; return; }
		else if ( ch == ',' ) { m_BackChar = 0; m_BackMode = SET_96x96; m_Status = ST_CHARSET_1; return; }
		else if ( ch == '-' ) { m_BackChar = 1; m_BackMode = SET_96x96; m_Status = ST_CHARSET_1; return; }
		else if ( ch == '.' ) { m_BackChar = 2; m_BackMode = SET_96x96; m_Status = ST_CHARSET_1; return; }
		else if ( ch == '/' ) { m_BackChar = 3; m_BackMode = SET_96x96; m_Status = ST_CHARSET_1; return; }
		// no break;
	case ST_CHARSET_1:
		m_StrPara += (CHAR)ch;
		if ( ch >= '\x30' && ch <= '\x7E' )
			m_BankTab[m_KanjiMode][m_BackChar] = m_FontTab.IndexFind(m_BackMode, m_StrPara);
		else
			return;
		break;
	}

	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc DCS/SOS/APC/PM/OSC ...

void CTextRam::fc_TimerSet(LPCTSTR name)
{
	m_bOscActive = TRUE;
	m_IntCounter = 0;
	m_OscName    = name;

	if ( !m_bIntTimer ) {
		m_bIntTimer = TRUE;
		((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(1000, TIMEREVENT_INTERVAL | TIMEREVENT_TEXTRAM, this);
	}

	if ( m_pCanDlg != NULL )
		m_pCanDlg->DestroyWindow();
}
void CTextRam::fc_TimerReset()
{
	m_bOscActive = FALSE;
	m_IntCounter = 0;

	if ( m_pCanDlg != NULL )
		m_pCanDlg->DestroyWindow();
}
void CTextRam::fc_TimerAbort(BOOL bOut)
{
	m_bOscActive = FALSE;
	m_IntCounter = 0;

	fc_POP('?');

	if ( bOut && m_OscPara.GetSize() > 0 )
		PUTSTR(m_OscPara.GetPtr(), m_OscPara.GetSize());
}

void CTextRam::fc_DCS(int ch)
{
	m_OscMode = 'P';
	m_BackChar = 0;
	m_CodeLen = 0;
	m_OscPara.Clear();
	m_AnsiPara.RemoveAll();
	m_AnsiPara.Add(PARA_NOT);
	fc_Case(STAGE_OSC1);
	fc_TimerSet(_T("DCS"));
}
void CTextRam::fc_SOS(int ch)
{
	m_OscMode = 'X';
	m_BackChar = 0;
	m_CodeLen = 0;
	m_OscPara.Clear();
	fc_Case(STAGE_OSC2);
	fc_TimerSet(_T("SOS"));
}
void CTextRam::fc_APC(int ch)
{
	m_OscMode = '_';
	m_BackChar = 0;
	m_CodeLen = 0;
	m_OscPara.Clear();
	fc_Case(STAGE_OSC2);
	fc_TimerSet(_T("APC"));
}
void CTextRam::fc_PM(int ch)
{
	m_OscMode = '^';
	m_BackChar = 0;
	m_CodeLen = 0;
	m_OscPara.Clear();
	fc_Case(STAGE_OSC2);
	fc_TimerSet(_T("PM"));
}
void CTextRam::fc_OSC(int ch)
{
	m_OscMode = ']';
	m_BackChar = 0;
	m_CodeLen = 0;
	m_OscPara.Clear();
	fc_Case(STAGE_OSC2);
	fc_TimerSet(_T("OSC"));
}
void CTextRam::fc_OSC_CMD(int ch)
{
	if ( m_LastChar == '\033' && ch == '\\' ) {
		fc_OSC_ST(ch);

	} else if ( (m_OscMode == ']' && ch == 0x07) || ((m_KanjiMode == EUC_SET || m_KanjiMode == ASCII_SET || m_KanjiMode == UTF8_SET) && ch == 0x9C) ) {
		fc_OSC_ST(ch);

	} else {
		m_LastChar = ch;
		ch &= 0x7F;

		if ( ch >= 0x20 && ch <= 0x2F ) {			// SP!"#$%&'()*+,-./
			m_BackChar &= 0xFF0000;
			m_BackChar |= (ch << 8);

		} else if ( ch >= 0x3C && ch <= 0x3F ) {	// <=>?
			m_BackChar = (ch << 16);

		} else if ( ch >= 0x40 && ch <= 0x7E ) {
			m_BackChar &= 0xFFFF00;
			m_BackChar |= ch;
			m_OscPara.Clear();
			fc_Case(STAGE_OSC2);
		}
	}
}
void CTextRam::fc_OSC_PAM(int ch)
{
	if ( m_LastChar == '\033' && ch == '\\' ) {
		if ( m_OscPara.GetSize() > 0 )
			m_OscPara.ConsumeEnd(1);
		fc_OSC_ST(ch);

	} else if ( m_OscMode == ']' && ch == 0x07 ) {
		fc_OSC_ST(ch);

	} else if ( (m_KanjiMode == EUC_SET || m_KanjiMode == ASCII_SET) && ch == 0x9C ) {
		fc_OSC_ST(ch);

	} else if ( m_KanjiMode == UTF8_SET ) {
		m_OscPara.Put8Bit(ch);
		m_LastChar = ch;

		if ( m_CodeLen > 0 && ch >= 0x80 && ch <= 0xBF )
			m_CodeLen--;
		else {
			if ( ch == 0x9C ) {
				m_OscPara.ConsumeEnd(1);
				fc_OSC_ST(ch);
			} else if ( ch >= 0xC0 && ch <= 0xDF )
				m_CodeLen = 1;
			else if ( ch >= 0xE0 && ch <= 0xEF )
				m_CodeLen = 2;
			else if ( ch >= 0xF0 && ch <= 0xF7 )
				m_CodeLen = 3;
			else if ( ch >= 0xF8 && ch <= 0xFB )
				m_CodeLen = 4;
			else if ( ch >= 0xFC && ch <= 0xFD )
				m_CodeLen = 5;
			else
				m_CodeLen = 0;
		}

	} else {
		m_OscPara.Put8Bit(ch);
		m_LastChar = ch;
	}
}
void CTextRam::fc_OSC_ST(int ch)
{
	int n;

	fc_TimerReset();

	switch(m_OscMode) {
	case 'P':	// DCS
		if ( BinaryFind((void *)&m_BackChar, m_DcsExt.GetData(), sizeof(CSIEXTTAB), m_DcsExt.GetSize(), ExtTabCodeCmp, &n) ) 
			(this->*m_DcsExt[n].proc)(ch);
		else
			fc_POP(ch);
		break;

	case 'X':	// SOS
	case '_':	// APC
	case '^':	// PM
		fc_OSCNULL(ch);
		break;

	case ']':	// OSC
		fc_OSCEXE(ch);
		break;
	}

	m_OscPara.Clear();
}
void CTextRam::fc_OSC_CAN(int ch)
{
	fc_TimerReset();
	m_OscPara.Clear();
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc DCS... 

void CTextRam::fc_DECUDK(int ch)
{
	// DCS ('P' << 24) | '|':						DECUDK User Defined Keys
	/*
		DCS Ps1 ; Ps2 ; Ps3 | D...D ST

		Ps1
		0 (default) or none Clear all keys before loading new values.
		1 Load new UDK values; clear old values only when redefined.

		Ps2
		0 or none Lock the keys. If you want to load new values into the keys, then you must unlock the keys by using Set-Up.
		1 Do not lock the keys against future redefinition.

		Ps3
		0, 2, or none Defines the shifted function key.
		1 Defines the unshifted function key.
		3 Defines the alternate unshifted function key.
		4 Defines the alternate shifted function key.

		D...D
		Keyn/Stn/Dir;...
	*/

	int n, key;
	CStringArrayExt node, data;

	if ( GetAnsiPara(0, 0, 0) == 0 ) {
		for ( n = 0 ; n < FKEY_MAX ; n++ )
			m_FuncKey[n].Clear();
	}

	node.GetString(MbsToTstr((LPCSTR)m_OscPara), _T(';'));

	for ( n = 0 ; n < node.GetSize() ; n++ ) {
		data.GetString(node[n], _T('/'));
		if ( data.GetSize() < 2 )
			continue;

		key = data.GetVal(0);
		if ( key >= 11 && key <= 15 )		// F1 - F5
			key = key - 11 + 0;
		else if ( key >= 17 && key <= 21 )	// F6 - F10
			key = key - 17 + 5;
		else if ( key >= 23 && key <= 26 )	// F11 - F14
			key = key - 23 + 10;
		else if ( key >= 28 && key <= 29 )	// F15 - F16
			key = key - 28 + 14;
		else if ( key >= 31 && key <= 34 )	// F17 - F20
			key = key - 31 + 16;
		else
			continue;

		m_FuncKey[key].Base16Decode(data[1]);
	}

	fc_POP(ch);
}
void CTextRam::fc_DECREGIS(int ch)
{
	// DCS ('P' << 24) | 'p'					DECREGIS ReGIS graphics

	int n;
	CString tmp;
	CGrapWnd *pGrapWnd;

	n = GetAnsiPara(0, 0, 0);
	if ( (n & 1) == 0 && (pGrapWnd = (CGrapWnd *)LastGrapWnd(TYPE_REGIS)) != NULL )
		pGrapWnd->SetReGIS(n, m_OscPara);
	else {
		tmp.Format(_T("ReGIS - %s"), m_pDocument->m_ServerEntry.m_EntryName);
		pGrapWnd = new CGrapWnd(this);
		pGrapWnd->Create(NULL, tmp);
		pGrapWnd->SetReGIS(n, m_OscPara);
		pGrapWnd->ShowWindow(SW_SHOW);
		AddGrapWnd((void *)pGrapWnd);
	}

	fc_POP(ch);
}
void CTextRam::fc_DECSIXEL(int ch)
{
	// DCS ('P' << 24) | 'q'					DECSIXEL Sixel graphics

	CString tmp;
	CGrapWnd *pGrapWnd, *pTempWnd;

	fc_POP(ch);
	
	if ( this->IsOptEnable(TO_DECSDM) ) {	// Sixel Scroll Mode Disable (DECSDM = set)
		tmp.Format(_T("Sixel - %s"), m_pDocument->m_ServerEntry.m_EntryName);
		pGrapWnd = new CGrapWnd(this);
		pGrapWnd->Create(NULL, tmp);
		pGrapWnd->SetSixelProc(GetAnsiPara(0, 0, 0), GetAnsiPara(1, 0, 0), GetAnsiPara(2, 0, 0), m_OscPara);
		pGrapWnd->ShowWindow(SW_SHOW);
		AddGrapWnd((void *)pGrapWnd);

	} else {								// Sixel Scroll Mode Enable (DECSDM = reset)
		pGrapWnd = new CGrapWnd(this);
		pGrapWnd->Create(NULL, _T(""));
		pGrapWnd->SetSixelProc(GetAnsiPara(0, 0, 0), GetAnsiPara(1, 0, 0), GetAnsiPara(2, 0, 0), m_OscPara, m_ColTab[m_AttNow.bc]);

		// Delete Non Display GrapWnd
		if ( ChkGrapWnd() )	if ( ChkGrapWnd() )	ChkGrapWnd();

		if ( (pTempWnd = GetGrapWnd(m_ImageIndex)) != NULL )
			pTempWnd->DestroyWindow();

		pGrapWnd->WaitForSixel();

		if ( pGrapWnd->m_pActMap == NULL ) {
			pGrapWnd->DestroyWindow();
			return;
		}

		int x, y, w, h;
		int cx, cy, dx, dy;
		CVram *vp;
		CRLoginView *pView;

		GetMargin(MARCHK_NONE);
		if ( (cx = m_Margin.right - m_CurX) <= 0 )
			cx = m_Cols - m_CurX;

		if ( m_pDocument != NULL && (pView = (CRLoginView *)m_pDocument->GetAciveView()) != NULL ) {
			w = pView->m_CharWidth;
			h = pView->m_CharHeight;
		} else {
			w = 6 + cx;
			h = w * m_DefFontHw / 10;
		}

		dx = (pGrapWnd->m_MaxX * pGrapWnd->m_AspX + 99) / 100;
		dy = (pGrapWnd->m_MaxY * pGrapWnd->m_AspY + 99) / 100;

		if ( (cx * w) < dx ) {
			pGrapWnd->m_AspX = pGrapWnd->m_AspX * (cx * w) / dx;
			pGrapWnd->m_AspY = pGrapWnd->m_AspY * (cx * w) / dx;
			dx = (pGrapWnd->m_MaxX * pGrapWnd->m_AspX + 99) / 100;
			dy = (pGrapWnd->m_MaxY * pGrapWnd->m_AspY + 99) / 100;
		}

		cx = (dx + w - 1) / w;
		cy = (dy + h - 1) / h;

		for ( y = 0 ; y < cy ; y++ ) {
			vp = GETVRAM(m_CurX, m_CurY);
			for ( x = 0 ; x < cx && (m_CurX + x) < m_Margin.right ; x++ ) {
				vp->pr.pk.im.id = m_ImageIndex;
				vp->pr.pk.im.ix = x;
				vp->pr.pk.im.iy = y;
				vp->pr.md = SET_96 | 'A';
				vp->pr.em = m_AttNow.em;
				vp->pr.cm = CM_IMAGE;
				vp->pr.at = m_AttNow.at;
				vp->pr.ft = m_AttNow.ft;
				vp->pr.fc = m_AttNow.fc;
				vp->pr.bc = m_AttNow.bc;
				vp++;
			}
			if ( IsOptEnable(TO_RLSIXPOS) != 0 ) {
				if ( (y + 1) < cy )
					ONEINDEX();
			} else
				ONEINDEX();
		}
		DISPUPDATE();

		pGrapWnd->m_BlockX = cx;
		pGrapWnd->m_BlockY = cy;
		pGrapWnd->m_BlockW = w;
		pGrapWnd->m_BlockH = h;

		pGrapWnd->m_ImageIndex = m_ImageIndex++;
		AddGrapWnd((void *)pGrapWnd);

		TRACE("AddGrapWnd %d(%d)\n", m_ImageIndex - 1, m_GrapWndTab.GetSize());

		if ( m_ImageIndex >= 4096 )		// index max 12 bit
			m_ImageIndex = 0;

		if ( IsOptEnable(TO_RLSIXPOS) != 0 ) {
			if ( (m_CurX += cx) >= m_Margin.right ) {
				if ( IsOptEnable(TO_DECAWM) != 0 ) {
					if ( IsOptEnable(TO_RLGNDW) != 0 ) {
						LOCATE(m_Margin.left, m_CurY);
						ONEINDEX();
					} else {
						LOCATE(m_Margin.right - 1, m_CurY);
						m_DoWarp = TRUE;
					}
				} else 
					LOCATE(m_Margin.right - 1, m_CurY);
			} else
				LOCATE(m_CurX, m_CurY);
		}
	}
}
void CTextRam::fc_DECDLD(int ch)
{
	// DCS ('P' << 24) | '{'					DECDLD Dynamically Redefinable Character Sets Extension

	// DCS Pfn ; Pcn ; Pe ; Pcmw ; Pss ; Pt ; Pcmh ; Pcss { <Dscs><sixel-font-pattern> ST
	//	Pfn Font number
	//		0,1,2
	//	Pcn Starting character
	//		0x20 + Pcn
	//	Pe 	Erase control
	//		0 = Erases all characters in the DRCS buffer with this number, width, and rendition.
	//		1 = Erases only characters in locations being reloaded. 
	//		2 = Erases all renditions of the soft character set (normal, bold, 80-column, 132-column).
	//	Pcmw Character matrix width
	//		0 = 10 pixels wide for 80 columns, 6 pixels wide for 132 columns. (default).
	//		2 = 5 x 10 pixel cell (width  height).
	//		3 = 6 x 10 pixel cell (width  height).
	//		4 = 7 x 10 pixel cell (width  height).
	//		5 = 5 pixels wide.
	//		6 = 6 pixels wide.
	//		7 = 7 pixels wide.
	//		8 = 8 pixels wide.
	//		9 = 9 pixels wide.
	//		10 = 10 pixels wide.
	//	Pss Font set size
	//		0,1 = 80 columns, 24 lines. (default)
	//		0 = 80 columns, 24 lines
	//		1 = 80 columns, 24 (25 or 26) lines
	//		2 = 132 columns, 24 lines
	//		11 = 80 columns, 36 (40 or 42) lines
	//		12 = 132 columns, 36 lines
	//		21 = 80 columns, 48 (50 or 52)
	//	Pt Text or full cell
	//		0 = text. (default)
	//		1 = text.
	//		2 = full cell.
	//		3 = Sixel		XXXXXXXXXXXX RLogin ext
	//	Pcmh Character matrix height
	//		0 or omitted = 16 pixels high. (default)
	//		1 = 1 pixel high.
	//		8 = 8 pixels high.
	//		9 = 9 pixels high.
	//		16 = 16 pixels high.
	//	Pcss character set size
	//		0 = Dscs is 94 character set
	//		1 = Dscs is 96 character set
	//	Dscs
	//		Chacter set name
	//	sixel-font-pattern
	//		0x3F(?) + binary
	//			A        B       C
	//		????????/????????/????????;
	//		1 2 3 4 5 6 7 8 9 0
	//		_ _ _ _ _ _ _ _ _ _ b0
	//		_ _ _ _ _ _ _ _ _ _ b1
	//		_ _ _ _ _ _ _ _ _ _ b2
	//		_ _ * * * * _ _ _ _ b3 Group A
	//		_ _ * _ _ _ * _ _ _ b4
	//		_ _ * _ _ _ _ * _ _ b5
	//		_ _ * _ _ _ _ _ * _ b6
	//		_ _ * _ _ _ _ _ * _ b0
	//		_ _ * _ _ _ _ _ * _ b1
	//		_ _ * _ _ _ _ _ * _ b2
	//		_ _ * _ _ _ _ _ * _ b3 Group B
	//		_ _ * _ _ _ _ * _ _ b4
	//		_ _ * _ _ _ * _ _ _ b5
	//		_ _ * * * * _ _ _ _ b6
	//		_ _ _ _ _ _ _ _ _ _ b0
	//		_ _ _ _ _ _ _ _ _ _ b1
	//		_ _ _ _ _ _ _ _ _ _ b2
	//		_ _ _ _ _ _ _ _ _ _ b3 Group C

	int n, i, x, b, idx;
	LPCSTR p;
	int Pfn, Pcn, Pe, Pcss, Pt;
	int Pcmw = 10, Pcmh = 0;
	CString Pscs;
	CStringArrayExt node, data;
	BYTE map[USFTCHSZ];
	CGrapWnd *pGrapWnd;

	fc_POP(ch);

	Pfn  = GetAnsiPara(0, 0, 0);
	Pcn  = GetAnsiPara(1, 0, 0) + 0x20;
	Pe   = GetAnsiPara(2, 0, 0);
	Pcmw = GetAnsiPara(3, 0, 0);
	Pt   = GetAnsiPara(5, 0, 0);
	Pcss = GetAnsiPara(7, 0, 0);

	//if ( Pcss == 0 && Pcn < 0x21 )
	//	Pcn = 0x21;

	switch(Pcmw) {
	case 0:
		Pcmw = (IsOptEnable(TO_DECCOLM) ? 9 : 15);
		break;
	case 2:
		Pcmw = 5;
		Pcmh = 10;
		break;
	case 3:
		Pcmw = 6;
		Pcmh = 10;
		break;
	case 4:
		Pcmw = 7;
		Pcmh = 10;
		break;
	}

	if ( Pcmh == 0 && (Pcmh = GetAnsiPara(6, 0, 0)) == 0 )
		Pcmh = 12;

	if ( Pe == 2 ) {
		for ( n = 0 ; n < CODE_MAX ; n++ ) {
			if ( m_FontTab[n].m_UserFontMap.GetSafeHandle() != NULL )
				m_FontTab[n].m_UserFontMap.DeleteObject();
		}
	}

	if ( Pcmw < 5 || Pcmw > USFTWMAX || Pcmh < 1 || Pcmh > USFTHMAX )
		return;

	p = (LPCSTR)m_OscPara;
	while ( *p != '\0' ) {
		if ( *p >= 0x30 && *p <= 0x7E ) {
			Pscs += *(p++);
			break;
		} else
			Pscs += *(p++);
	}

	if ( Pscs.IsEmpty() )
		return;

	idx = m_FontTab.IndexFind((Pcss == 0 ? SET_94 : SET_96), Pscs);

	if ( Pe == 0 ) {
		if ( m_FontTab[idx].m_UserFontMap.GetSafeHandle() != NULL )
			m_FontTab[idx].m_UserFontMap.DeleteObject();
	}

	if ( Pt == 3 ) {				// Sixel
		pGrapWnd = new CGrapWnd(this);
		pGrapWnd->Create(NULL, _T("DCS"));
		pGrapWnd->SetSixel(9, 0, 0, p, m_ColTab[m_AttNow.bc]);
		for ( n = i = 0 ; ((i + 1) * Pcmh) <= pGrapWnd->m_MaxY && (Pcn + n) < 0x80 ; i++ ) {
			for ( x = 0 ; ((x + 1) * Pcmw) <= pGrapWnd->m_MaxX && (Pcn + n) < 0x80 ; x++, n++ )
				m_FontTab[idx].SetUserBitmap(Pcn + n, Pcmw, Pcmh, pGrapWnd->m_pActMap, x * Pcmw, i * Pcmh);
		}
		pGrapWnd->DestroyWindow();

	} else {
		node.GetString(MbsToTstr(p), _T(';'));
		for ( n = 0 ; n < node.GetSize() && (Pcn + n) < 0x80 ; n++ ) {
			data.GetString(node[n], _T('/'));
			memset(map, 0, USFTCHSZ);
			for ( i = 0 ; i < data.GetSize() && i < USFTLNSZ ; i++ ) {
				for ( x = 0 ; x < data[i].GetLength() && x < USFTWMAX ; x++ ) {
					if ( (b = data[i][x] - 0x3F) < 0 )
						b = 0;
					map[x + i * USFTWMAX] = b;
				}
			}
			m_FontTab[idx].SetUserFont(Pcn + n, Pcmw, Pcmh, map);
		}
	}
}
void CTextRam::fc_DECRSTS(int ch)
{
	// DCS ('P' << 24) | ('$' << 8) | 'p'		DECRSTS Restore Terminal State

	int n;
	CStringArrayExt node, value;
	int Pc, Pu, Px, Py, Pz;

	switch(GetAnsiPara(0, 0, 0)) {
	case 1:		// Selects the format of the terminal state report (DECTSR)
		break;

	case 2:		// DECCTR Color Table Restore
		// Pc; Pu; Px; Py; Pz / Pc; Pu; Px; Py; Pz / ...
		node.GetString(MbsToTstr((LPCSTR)m_OscPara), _T('/'));
		for ( n = 0 ; n < node.GetSize() ; n++ ) {
			value.GetString(node[n], _T(';'));
			if ( value.GetSize() < 5 )
				continue;
			Pc = value.GetVal(0);
			Pu = value.GetVal(1);
			Px = value.GetVal(2);
			Py = value.GetVal(3);
			Pz = value.GetVal(4);
			if ( Pc < 0 || Pc > 255 )
				continue;
			switch(Pu) {
			case 1:		// HLS
				m_ColTab[Pc] = CGrapWnd::HLStoRGB(Px * 100 / 360, Py * 100 / 100, Pz * 100 / 100);
				break;
			case 2:		// RGB
				m_ColTab[Pc] = RGB(Px * 255 / 100, Py * 255 / 100, Pz * 255 / 100);
				break;
			}
		}
		break;
	}

	fc_POP(ch);
}
void CTextRam::fc_DECRQSS(int ch)
{
	// DCS ('P' << 24) | ('$' << 8) | 'q'		DECRQSS Request Selection or Setting

	int n;
	LPCSTR p;
	CString str, wrk;

	p = (LPCSTR)m_OscPara;
	for ( n = 0 ; *p != '\0' ; p++ )
		n = (n << 8) | *p;

	switch(n) {
	case 'm':					// SGR
		str = m_RetChar[RC_DCS];
		str += _T("1$r0");
		if ( (m_AttNow.at & ATT_BOLD)   != 0 ) str += _T(";1");
		if ( (m_AttNow.at & ATT_HALF)   != 0 ) str += _T(";2");
		if ( (m_AttNow.at & ATT_ITALIC) != 0 ) str += _T(";3");
		if ( (m_AttNow.at & ATT_UNDER)  != 0 ) str += _T(";4");
		if ( (m_AttNow.at & ATT_SBLINK) != 0 ) str += _T(";5");
		if ( (m_AttNow.at & ATT_BLINK)  != 0 ) str += _T(";6");
		if ( (m_AttNow.at & ATT_REVS)   != 0 ) str += _T(";7");
		if ( (m_AttNow.at & ATT_SECRET) != 0 ) str += _T(";8");
		if ( (m_AttNow.at & ATT_LINE)   != 0 ) str += _T(";9");
		if ( (m_AttNow.at & ATT_DUNDER) != 0 ) str += _T(";21");
		if ( (m_AttNow.at & ATT_FRAME)  != 0 ) str += _T(";51");
		if ( (m_AttNow.at & ATT_CIRCLE) != 0 ) str += _T(";52");
		if ( (m_AttNow.at & ATT_OVER)   != 0 ) str += _T(";53");
		if ( (m_AttNow.at & ATT_RSLINE) != 0 ) str += _T(";60");
		if ( (m_AttNow.at & ATT_RDLINE) != 0 ) str += _T(";61");
		if ( (m_AttNow.at & ATT_LSLINE) != 0 ) str += _T(";62");
		if ( (m_AttNow.at & ATT_LDLINE) != 0 ) str += _T(";63");
		if ( (m_AttNow.at & ATT_STRESS) != 0 ) str += _T(";64");

		if ( m_AttNow.ft > 0 ) { wrk.Format(_T(";%d"), m_AttNow.ft + 10); str += wrk; }

		if ( m_AttNow.fc == m_DefAtt.fc ) str += _T("");
		else if ( m_AttNow.fc < 8 ) { wrk.Format(_T(";%d"), m_AttNow.fc + 30); str += wrk; }
		else if ( m_AttNow.fc < 16 ) { wrk.Format(_T(";%d"), m_AttNow.fc - 8 + 90); str += wrk; }
		else { wrk.Format(_T(";38;5;%d"), m_AttNow.fc); str += wrk; }

		if ( m_AttNow.bc == m_DefAtt.bc ) str += _T("");
		else if ( m_AttNow.bc < 8 ) { wrk.Format(_T(";%d"), m_AttNow.fc + 40); str += wrk; }
		else if ( m_AttNow.bc < 16 ) { wrk.Format(_T(";%d"), m_AttNow.bc - 8 + 100); str += wrk; }
		else { wrk.Format(_T(";48;5;%d"), m_AttNow.bc); str += wrk; }

		str += _T("m");
		str += m_RetChar[RC_ST];
		UNGETSTR(_T("%s"), str);
		break;

	case 'r':					// DECSTBM
		UNGETSTR(_T("%s1$r%d;%dr%s"), m_RetChar[RC_DCS], m_TopY + 1, m_BtmY + 1 - 1, m_RetChar[RC_ST]);
		break;

	case 's':					// DECSLRM Set left and right margins
		UNGETSTR(_T("%s1$r%d;%ds%s"), m_RetChar[RC_DCS], m_LeftX + 1, m_RightX + 1 - 1, m_RetChar[RC_ST]);
		//UNGETSTR("%s0$r%s", m_RetChar[RC_DCS], m_RetChar[RC_ST]);
		break;

	case 't':					// DECSLPP Set lines per physical page
		UNGETSTR(_T("%s1$r%dt%s"), m_RetChar[RC_DCS], m_Lines, m_RetChar[RC_ST]);
		//UNGETSTR("%s0$r%s", m_RetChar[RC_DCS], m_RetChar[RC_ST]);
		break;

	case ('$' << 8) | '|':		// DECSCPP
		UNGETSTR(_T("%s1$r%d$|%s"), m_RetChar[RC_DCS], m_Cols, m_RetChar[RC_ST]);
		break;

	case ('*' << 8) | 'x':		// DECSACE Select Attribute and Change Extent
		UNGETSTR(_T("%s1$r%d*x%s"), m_RetChar[RC_DCS], m_Exact ? 2 : 1, m_RetChar[RC_ST]);
		break;

	case ('*' << 8) | '|':		// DECSNLS
		UNGETSTR(_T("%s1$r%d*|%s"), m_RetChar[RC_DCS], m_Lines, m_RetChar[RC_ST]);
		break;

	case ('"' << 8) | 'p':		// DECSCL
		if ( m_VtLevel == 61 )
			UNGETSTR(_T("%s1$r%d\"p%s"), m_RetChar[RC_DCS], m_VtLevel, m_RetChar[RC_ST]);
		else
			UNGETSTR(_T("%s1$r%d;%d\"p%s"), m_RetChar[RC_DCS], m_VtLevel, *(m_RetChar[0]) == '\033' ? 1 : 0, m_RetChar[RC_ST]);
		break;

	case ('"' << 8) | 'q':		// DECSCA
		UNGETSTR(_T("%s1$r%d\"q%s"), m_RetChar[RC_DCS], (m_AttNow.em & EM_DECPROTECT) != 0 ? 1 : 0, m_RetChar[RC_ST]);
		break;

	case ('$' << 8) | '}':		// DECSASD Select active status display
		UNGETSTR(_T("%s1$r%d$}%s"), m_RetChar[RC_DCS], m_StsFlag ? 1 : 0, m_RetChar[RC_ST]);
		break;

	case ('$' << 8) | '~':		// DECSSDT Select status display type
		UNGETSTR(_T("%s1$r%d$~%s"), m_RetChar[RC_DCS], m_StsMode, m_RetChar[RC_ST]);
		break;

	case (',' << 8) | 'q':		// DECTID Select Terminal ID
		UNGETSTR(_T("%s1$r%d,q%s"), m_RetChar[RC_DCS], m_TermId, m_RetChar[RC_ST]);
		break;

	case (' ' << 8) | 'q':		// DECSCUSR Set Cursor Style
		UNGETSTR(_T("%s1$r%d q%s"), m_RetChar[RC_DCS], m_TypeCaret, m_RetChar[RC_ST]);
		break;

	//case '|':					// DECTTC Select transmit termination character
	//case ('\'' << 8) | 's':	// DECTLTC
	//case ('+' << 8) | 'q':	// DECELF Enable local functions
	//case ('+' << 8) | 'r':	// DECSMKR Select modifier key reporting
	//case ('*' << 8) | '}':	// DECLFKC Local function key control
	default:
		UNGETSTR(_T("%s0$r%s"), m_RetChar[RC_DCS], m_RetChar[RC_ST]);		// DECRPSS
		break;
	}

	fc_POP(ch);
}
void CTextRam::fc_DECRSPS(int ch)
{
	// DCS Ps $ t D...D ST						DECRSPS Restore Presentation State

	// Ps Data String Format
	//	0 Error, restore ignored.
	//	1 Selects the format of the cursor information report (DECCIR).
	//	2 Selects the format of the tab stop report (DECTABSR).

	int n, x;
	CStringArrayExt node;

	switch(GetAnsiPara(0, 0, 0)) {
	case 1:		// DECCIR
		// DCS 1 $ t Pr; Pc; Pp; Srend; Satt; Sflag; Pgl; Pgr; Scss; Sdesig ST
		node.GetString(MbsToTstr((LPCSTR)m_OscPara), _T(';'));
		if ( node.GetSize() > 1 ) {					// Pr ; Pc
			LOCATE(node.GetVal(1) - 1, node.GetVal(0) - 1);
		}
		if ( node.GetSize() > 2 ) {					// Pp
			m_Page = node.GetVal(2) - 1;
		}
		if ( node.GetSize() > 3 ) {					// Srend
			n = node[3][0];
			m_AttNow.at &= ~(ATT_REVS | ATT_SBLINK | ATT_BLINK | ATT_UNDER | ATT_BOLD);
			if ( (n & 8) != 0 ) m_AttNow.at |= ATT_REVS;
			if ( (n & 4) != 0 ) m_AttNow.at |= ATT_BLINK;
			if ( (n & 2) != 0 ) m_AttNow.at |= ATT_UNDER;
			if ( (n & 1) != 0 ) m_AttNow.at |= ATT_BOLD;
		}
		if ( node.GetSize() > 4 ) {					// Satt
			if ( (node[4][0] & 1) != 0 )
				EnableOption(TO_ANSIERM);
			else
				DisableOption(TO_ANSIERM);
		}
		if ( node.GetSize() > 5 ) {					// Sflag
			n = node[5][0];
			if ( (n & 8) != 0 )
				EnableOption(TO_DECAWM);
			else
				DisableOption(TO_DECAWM);
			if ( (n & 4) != 0 )
				m_BankSG = 3;
			else if ( (n & 2) != 0 )
				m_BankSG = 2;
			else
				m_BankSG = (-1);
			if ( (n & 1) != 0 )
				EnableOption(TO_DECOM);
			else
				DisableOption(TO_DECOM);
		}
		if ( node.GetSize() > 6 ) {					// Pgl
			m_BankGL = node[6][0];
		}
		if ( node.GetSize() > 7 ) {					// Pgr
			m_BankGR = node[7][0];
		}
		if ( node.GetSize() > 8 ) {					// Scss
			n = node[8][0];
			if ( (n & 8) != 0 )
				m_BankTab[m_KanjiMode][3] |= SET_96;
			else
				m_BankTab[m_KanjiMode][3] &= ~SET_96;
			if ( (n & 4) != 0 )
				m_BankTab[m_KanjiMode][2] |= SET_96;
			else
				m_BankTab[m_KanjiMode][2] &= ~SET_96;
			if ( (n & 2) != 0 )
				m_BankTab[m_KanjiMode][1] |= SET_96;
			else
				m_BankTab[m_KanjiMode][1] &= ~SET_96;
			if ( (n & 1) != 0 )
				m_BankTab[m_KanjiMode][0] |= SET_96;
			else
				m_BankTab[m_KanjiMode][0] &= ~SET_96;
		}
		if ( node.GetSize() > 9 ) {					// Scss
			LPCTSTR p = node[9];
			CString tmp;
			for ( n = 0 ; n < 4 ; n++ ) {
				tmp.Empty();
				m_BankTab[m_KanjiMode][n] &= ~SET_94x94;
				while ( *p != _T('\0') ) {
					if ( *p >= 0x30 && *p <= 0x7E ) {
						tmp += *(p++);
						break;
					} else if ( *p == _T('/') ) {
						p++;
						m_BankTab[m_KanjiMode][n] |= SET_94x94;
					} else
						tmp += *(p++);
				}
				m_BankTab[m_KanjiMode][n] = m_FontTab.IndexFind(m_BankTab[m_KanjiMode][n] & SET_MASK, tmp);
			}
		}
		break;

	case 2:		// DECTABSR
		x = m_CurX;
		TABSET(TAB_COLSALLCLR);
		node.GetString(MbsToTstr((LPCSTR)m_OscPara), _T('/'));
		for ( n = 0 ; n < node.GetSize() ; n++ ) {
			if ( (m_CurX = node.GetVal(n) - 1) >= 0 && m_CurX < m_Cols )
				TABSET(TAB_COLSSET);
		}
		m_CurX = x;
		break;
	}

	fc_POP(ch);
}
void CTextRam::fc_DECDMAC(int ch)
{
	// DCS Pid ; Pdt ; Pen ! z D...D ST			DECDMAC Define Macro

	// Pid is the macro ID number. Pid can be any number between 0 and 63,

	// Pdt defines how the terminal treats new macro definitions.
	//	0 or omitted	DECDMAC deletes the old macro with the same ID number before defining this new macro.
	//	1				DECDMAC deletes all current macro definitions before defining this macro.
	//	Other			The terminal ignores the macro.

	// Pen selects the encoding format for the text of the macro definition.
	//	0 or omitted	Uses standard ASCII characters in the macro.
	//	1				Uses hex pairs in the macro.
	//	Other			The terminal ignores the macro.

	int n;
	int Pid = GetAnsiPara(0, 0, 0);
	int Pdt = GetAnsiPara(1, 0, 0);
	int Pen = GetAnsiPara(2, 0, 0);

	if ( Pid >= 64 || Pdt > 1 || Pen > 1 || (m_MacroExecFlag[Pid / 32] & (1 << (Pid % 32))) != 0 )
		goto ENDRET;

	if ( Pdt == 1 ) {
		for ( n = 0 ; n < 64 ; n++ ) {
			if ( (m_MacroExecFlag[n / 32] & (1 << (n % 32))) == 0 )
				m_Macro[n].Clear();
		}
	} else
		m_Macro[Pid].Clear();

	if ( Pen == 1 )
		GetHexPara(MbsToTstr((LPCSTR)m_OscPara), m_Macro[Pid]);
	else
		m_Macro[Pid] = m_OscPara;

ENDRET:
	fc_POP(ch);
}
void CTextRam::fc_DECSTUI(int ch)
{
	// DCS ! {						DECSTUI Set Terminal Unit ID

	LPCSTR p = (LPCSTR)m_OscPara;
	
	m_UnitId = 0;
	while ( *p != '\0' ) {
		if ( *p >= '0' && *p <= '9' )
			m_UnitId = m_UnitId * 16 + (*(p++) - '0');
		else if ( *p >= 'A' && *p <= 'F' )
			m_UnitId = m_UnitId * 16 + (*(p++) - 'A' + 10);
		else if ( *p >= 'a' && *p <= 'f' )
			m_UnitId = m_UnitId * 16 + (*(p++) - 'a' + 10);
		else
			break;
	}

	fc_POP(ch);
}
void CTextRam::fc_XTRQCAP(int ch)
{
	//DCS ('P' << 24) | ('+' << 8) | 'q'		XTRQCAP Request Termcap/Terminfo String (xterm, experimental)

	int n;
	LPCSTR p;
	CString tmp, str, wrk;
	CBuffer buf, res, hex;

	p = (LPCSTR)m_OscPara;
	str.Empty();
	n = 0;
	while ( *p != '\0' ) {
		tmp.Empty();
		while ( *p != '\0' && *p != ';' )
			tmp += *(p++);
		if ( !tmp.IsEmpty() && m_pDocument->m_KeyTab.FindCapInfo(tmp, &buf) ) {
			n = 1;
			if ( !str.IsEmpty() )
				str += _T(';');
			str += tmp;
			str += _T('=');
			res.Clear();
			m_pDocument->m_TextRam.m_IConv.StrToRemote(m_pDocument->m_TextRam.m_SendCharSet[m_pDocument->m_TextRam.m_KanjiMode], &buf, &res);
			hex.Base16Encode(res.GetPtr(), res.GetSize());
			str += (LPCTSTR)hex;
		}
		if ( *p == ';' )
			p++;
	}
	UNGETSTR(_T("%s%d+r%s%s"), m_RetChar[RC_DCS], n, str, m_RetChar[RC_ST]);

	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc OSC

void CTextRam::fc_OSCEXE(int ch)
{
	// OSC (']' << 24)					OSC Operating System Command

	int n, cmd;
	LPCSTR p;
	LPCTSTR s;
	CString tmp, wrk, ctr;

	p = (LPCSTR)m_OscPara;
	while ( *p != '\0' && *p != ';' )
		tmp += *(p++);
	if ( *p == ';' )
		p++;
	if ( tmp.IsEmpty() )
		goto ENDRET;

	cmd = _tstoi(tmp);
	switch(cmd) {
	case 0:		// 0 -> Change Icon Name and Window Title to Pt
	case 1:		// 1 -> Change Icon Name to Pt
	case 2:		// 2 -> Change Window Title to Pt
		if ( (m_TitleMode & WTTL_CHENG) == 0 ) {
			if ( (m_XtOptFlag & XTOP_SETUTF) != 0 )
				m_IConv.RemoteToStr(m_SendCharSet[UTF8_SET], p, tmp);
			else
				m_IConv.RemoteToStr(m_SendCharSet[m_KanjiMode], p, tmp);

			if ( (m_XtOptFlag & XTOP_SETHEX) != 0 ) {
				CBuffer buf;
				buf.Base16Decode(tmp);
				if ( (m_XtOptFlag & XTOP_SETUTF) != 0 )
					m_IConv.RemoteToStr(m_SendCharSet[UTF8_SET], (LPCSTR)buf, tmp);
				else
					m_IConv.RemoteToStr(m_SendCharSet[m_KanjiMode], (LPCSTR)buf, tmp);
			}
			wrk.Empty();
			for ( n = 0, s = tmp ; n < 1024 && *s != _T('\0') ; n++, s++ ) {
				if ( *s < _T(' ') || *s == 0x7F ) {	// || (*s >= 0x80 && *s <= 0x9F) ) {
					ctr.Format(_T("<%02X>"), *s);
					wrk += ctr;
				} else
					wrk += *s;
			}
			if ( n >= 1024 )
				wrk = _T("Too many Title Strings...");
			m_pDocument->SetTitle(wrk);
		}
		break;

	case 3:		// 3  -> Set X property on top-level window.
		break;

	case 4:		// 4;c;spec -> Change Color Number c to the color specified by spec.
		while ( *p != '\0' ) {
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			if ( tmp.IsEmpty() )
				break;
			n = _tstoi(tmp);
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			ParseColor(cmd, n, tmp, ch);
		}
		break;

	case 5:		// 5;n;spec -> Change Special Color Number c to the color specified by spec.
		while ( *p != '\0' ) {
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			if ( tmp.IsEmpty() )
				break;
			n = EXTCOL_SP_BEGIN + _tstoi(tmp);
				//	n = 0  <- resource colorBD (BOLD).
				//	n = 1  <- resource colorUL (UNDERLINE).
				//	n = 2  <- resource colorBL (BLINK).
				//	n = 3  <- resource colorRV (REVERSE).
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			if ( n < EXTCOL_MAX ) {
				m_ColTab[n] = m_ColTab[m_AttNow.fc];
				ParseColor(cmd, n, tmp, ch);
			}
		}
		break;

	case 10:	// 10  -> Change VT100 text foreground color to Pt.
	case 12:	// 12  -> Change text cursor color to Pt.
	case 13:	// 13  -> Change mouse foreground color to Pt.
	case 15:	// 15  -> Change Tektronix foreground color to Pt.
	case 18:	// 18  -> Change Tektronix cursor color to Pt.
	case 19:	// 19  -> Change highlight foreground color to Pt.
		while ( *p != '\0' ) {
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			n = EXTCOL_BEGIN + (cmd - 10);
			m_ColTab[n] = m_ColTab[m_AttNow.fc];
			ParseColor(cmd, n, tmp, ch);
			if ( cmd == 10 )
				m_AttNow.fc = GETCOLIDX(GetRValue(m_ColTab[n]), GetGValue(m_ColTab[n]), GetBValue(m_ColTab[n]));
		}
		break;
	case 11:	// 11  -> Change VT100 text background color to Pt.
	case 14:	// 14  -> Change mouse background color to Pt.
	case 16:	// 16  -> Change Tektronix background color to Pt.
	case 17:	// 17  -> Change highlight background color to Pt.
		while ( *p != '\0' ) {
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			n = EXTCOL_BEGIN + (cmd - 10);
			m_ColTab[n] = m_ColTab[m_AttNow.bc];
			ParseColor(cmd, n, tmp, ch);
			if ( cmd == 11 )
				m_AttNow.bc = GETCOLIDX(GetRValue(m_ColTab[n]), GetGValue(m_ColTab[n]), GetBValue(m_ColTab[n]));
		}
		break;

	case 46:	// 46  -> Change Log File to Pt.
		break;

	case 50:	// 50;[?][+-][0-9]font		Set Font to Pt.
		while ( *p != '\0' ) {
			CHAR md = '\0';
			int num = 0;
			while ( *p != '\0' && *p == ' ' )
				p++;
			if ( *p == '?' || *p == '#' )
				md = *(p++);
			while ( *p != '\0' && *p == ' ' )
				p++;
			tmp.Empty();
			if ( *p == '+' || *p == '-' )
				tmp += *(p++);
			while ( *p >= '0' && *p <= '9' )
				tmp += *(p++);
			num = _tstoi(tmp);
			while ( *p != '\0' && *p == ' ' )
				p++;
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			if ( md == '?' )
				UNGETSTR(_T("%s50%s"), m_RetChar[RC_OSC], (ch == 0x07 ? _T("\007") : m_RetChar[RC_ST]));
		}
		break;

	case 52:	// 52  -> Manipulate Selection Data.
		tmp.Empty();
		while ( *p != '\0' && *p != ';' )
			tmp += *(p++);
		if ( *p == ';' ) {
			p++;
			// tmp = "s c p 0 1 2 3 4 7"	 SELECT, PRIMARY, CLIPBOARD, CUT_BUFFER0 - 7
			CBuffer clip, work;
			CRLoginView *pView;
			if ( strcmp(p, "?") == 0 ) {	// Get Clipboad
				if ( m_pDocument != NULL && (pView = (CRLoginView *)m_pDocument->GetAciveView()) != NULL && (m_pDocument->m_TextRam.m_ClipFlag & OSC52_READ) != 0 ) {
					pView->GetClipboad(&clip);
					work.Base64Encode(clip.GetPtr(), clip.GetSize());
				}
				UNGETSTR(_T("%s52;%s;%s%s"), m_RetChar[RC_OSC], tmp, (LPCTSTR)work, (ch == 0x07 ? _T("\007") : m_RetChar[RC_ST]));
			} else {						// Set Clipboad
				if ( m_pDocument != NULL && (pView = (CRLoginView *)m_pDocument->GetAciveView()) != NULL && (m_pDocument->m_TextRam.m_ClipFlag & OSC52_WRITE) != 0 ) {
					clip.Base64Decode(MbsToTstr(p));
					pView->SetClipboad(&clip);
				}
			}
		}
		break;

	case 104:	// 104;c -> Reset Color Number c.
		while ( *p != '\0' ) {
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			if ( tmp.IsEmpty() )
				break;
			n = _tstoi(tmp);
			ParseColor(cmd, n, _T("reset"), ch);
		}
		break;

	case 105:	// 105;c -> Reset Special Color Number c. 
		while ( *p != '\0' ) {
			tmp.Empty();
			while ( *p != '\0' && *p != ';' )
				tmp += *(p++);
			if ( *p == ';' )
				p++;
			if ( tmp.IsEmpty() )
				break;
			n = EXTCOL_SP_BEGIN + _tstoi(tmp);
				//	n = 0  <- resource colorBD (BOLD).
				//	n = 1  <- resource colorUL (UNDERLINE).
				//	n = 2  <- resource colorBL (BLINK).
				//	n = 3  <- resource colorRV (REVERSE).
			if ( n < EXTCOL_MAX ) {
				m_ColTab[n] = m_ColTab[m_AttNow.fc];
				ParseColor(cmd, n, _T("reset"), ch);
			}
		}
		break;

	case 110:	// 110  -> Reset VT100 text foreground color.
	case 112:	// 112  -> Reset text cursor color.
	case 113:	// 113  -> Reset mouse foreground color.
	case 115:	// 115  -> Reset Tektronix foreground color.
	case 118:	// 118  -> Reset Tektronix cursor color.
	case 119:	// 119  -> Reset highlight foreground color to Pt.
		n = EXTCOL_BEGIN + (cmd - 110);
		m_ColTab[n] = m_ColTab[m_AttNow.fc];
		ParseColor(cmd, n, _T("reset"), ch);
		if ( cmd == 110 )
			m_AttNow.fc = GETCOLIDX(GetRValue(m_ColTab[n]), GetGValue(m_ColTab[n]), GetBValue(m_ColTab[n]));
		break;
	case 111:	// 111  -> Reset VT100 text background color.
	case 114:	// 114  -> Reset mouse background color.
	case 116:	// 116  -> Reset Tektronix background color.
	case 117:	// 117  -> Reset highlight background color.
		n = EXTCOL_BEGIN + (cmd - 110);
		m_ColTab[n] = m_ColTab[m_AttNow.bc];
		ParseColor(cmd, n, _T("reset"), ch);
		if ( cmd == 111 )
			m_AttNow.bc = GETCOLIDX(GetRValue(m_ColTab[n]), GetGValue(m_ColTab[n]), GetBValue(m_ColTab[n]));
		break;

	case 800:	// Original Kanji Code Set
		tmp.Empty();
		while ( *p != '\0' && *p != ';' )
			tmp += *(p++);
		if ( tmp.Compare(_T("?")) == 0 )
			UNGETSTR(_T("%s800;%d%s"), m_RetChar[RC_OSC], m_KanjiMode, (ch == 0x07 ? _T("\007") : m_RetChar[RC_ST]));
		else if ( tmp.GetLength() > 0 && (n = _tstoi(tmp)) >= 0 && n <= BIG5_SET ) {
			SetKanjiMode(n);
			return;
		}
		break;
	}

ENDRET:
	fc_POP(ch);
}
void CTextRam::fc_OSCNULL(int ch)
{
	// SOS ('X' << 24)			SOS Start of String
	// PM  ('^' << 24)			PM
	// APC ('_' << 24)			APC Application Program Command

	CFile file;
	CFileDialog dlg(FALSE, _T("txt"), (m_OscMode == 'X' ? _T("SOS") : (m_OscMode == '^' ? _T("PM") : _T("APC"))), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), AfxGetMainWnd());

	if ( m_FileSaveFlag && dlg.DoModal() == IDOK ) {
		if ( file.Open(dlg.GetPathName(), CFile::modeWrite) ) {
			file.Write(m_OscPara.GetPtr(), m_OscPara.GetSize());
			file.Close();
		}
	} else
		m_FileSaveFlag = FALSE;

	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI... 

void CTextRam::fc_CSI(int ch)
{
	m_BackChar = 0;
	m_AnsiPara.RemoveAll();
	m_AnsiPara.Add(PARA_NOT);
	fc_Case(STAGE_CSI);
}
void CTextRam::fc_CSI_ESC(int ch)
{
	fc_Case(STAGE_ESC);
}

//////////////////////////////////////////////////////////////////////
//
//	CSI m			-> Size=1 Cmds="m"		Para[0]=NOT
//	CSI 31 m		-> Size=1 Cmds="m"		Para[0]=31
//	CSI 31 ; m		-> Size=2 Cmds="m"		Para[0]=31, Para[1]=NOT
//	CSI ; 31 m		-> Size=2 Cmds="m"		Para[0]=NOT, Para[1]=31
//
//	CSI > m			-> Size=1 Cmds=">m"		Para[0]=NOT
//	CSI 31 > m		-> Size=2 Cmds="m"		Para[0]=31, Para[1]=">"
//	CSI 31 ; > m	-> Size=2 Cmds="m"		Para[0]=31, Para[1]=">"
//	CSI ; > 31 m	-> Size=2 Cmds="m"		Para[0]=NOT, Para[1]=">31"
//
//	CSI ! m			-> Size=1 Cmds="!m"		Para[0]=NOT
//	CSI ! 31 m		-> Size=2 Cmds="m"		Para[0]="!", Para[1]=31
//	CSI 31 ! ; m	-> Size=2 Cmds="m"		Para[0]="31!", Para[1]=NOT
//	CSI 31 ! ; ! m	-> Size=2 Cmds="!m"		Para[0]="31!", Para[1]=NOT
//

void CTextRam::fc_CSI_RST(int ch)
{
	m_BackChar = ch;

	switch(m_BackChar & 0x7F7F7F00) {
	case 0:
		fc_Case(STAGE_CSI);
		break;
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
void CTextRam::fc_CSI_DIGIT(int ch)
{
	int n, a;
	DWORD v;

	// CSI ! 31 -> [0]=OPT, [1]=NOT
	if ( (m_BackChar & 0x0000FF00) != 0 ) {
		m_AnsiPara.AddOpt(m_BackChar >> 8, TRUE);
		fc_CSI_RST(m_BackChar & 0xFFFF0000);
	}

	ASSERT(m_AnsiPara.GetSize());

	n = (int)m_AnsiPara.GetSize() - 1;
	a = (int)m_AnsiPara[n].GetSize() - 1;

	if ( a < 0 ) {
		m_AnsiPara[n].m_Str += (TCHAR)ch;
		v = m_AnsiPara[n];
	} else {
		m_AnsiPara[n][a].m_Str += (TCHAR)ch;
		v = m_AnsiPara[n][a];
	}

	if ( v != PARA_MAX && v != PARA_OPT ) {		// Max Value

		if ( v == PARA_NOT )			// Not Init Value
			v = 0;

		ch = ((ch & 0x7F) - '0');

		if ( v >= ((PARA_MAX - ch) / 10) )
			v = PARA_MAX;
		else
			v = v * 10 + ch;

		if ( a < 0 )
			m_AnsiPara[n] = v;
		else
			m_AnsiPara[n][a] = v;
	}
}
void CTextRam::fc_CSI_PUSH(int ch)
{
	// CSI ! :  -> [0]=OPT, [1][0]=NOT
	if ( (m_BackChar & 0x0000FF00) != 0 ) {
		m_AnsiPara.AddOpt(m_BackChar >> 8, FALSE); 
		fc_CSI_RST(m_BackChar & 0xFFFF0000);
	}

	ASSERT(m_AnsiPara.GetSize());

	m_AnsiPara[m_AnsiPara.GetSize() - 1].Add(PARA_NOT);
}
void CTextRam::fc_CSI_SEPA(int ch)
{
	// CSI ! ; -> [0]=OPT, [1]=NOT
	if ( (m_BackChar & 0x0000FF00) != 0 ) {
		m_AnsiPara.AddOpt(m_BackChar >> 8, FALSE); 
		fc_CSI_RST(m_BackChar & 0xFFFF0000);
	}

	m_AnsiPara.Add(PARA_NOT);
}
void CTextRam::fc_CSI_EXT(int ch)
{
	int n, a;

	ASSERT(m_AnsiPara.GetSize());

	if ( ch >= 0x20 && ch <= 0x2F ) {					// SP!"#$%&'()*+,-./
		m_BackChar &= 0xFFFF0000;
		m_BackChar |= (ch << 8);

	} else {											// <=>?
		// CSI ! > -> [0]=OPT, [1]=NOT
		if ( (m_BackChar & 0x0000FF00) != 0 ) {
			m_AnsiPara.AddOpt(m_BackChar >> 8, TRUE); 
			m_BackChar &= 0xFFFF0000;
		}

		n = (int)m_AnsiPara.GetSize() - 1;
		a = (int)m_AnsiPara[n].GetSize() - 1;

		// CSI >
		if ( n == 0 && m_AnsiPara[0] == PARA_NOT && a < 0 ) {
			m_BackChar = (m_BackChar << 8) & 0xFF000000;
			m_BackChar |= (ch << 16);

		// CSI ; >
		} else if ( a < 0 ) {
			if ( m_AnsiPara[n] != PARA_NOT && m_AnsiPara[n] != PARA_OPT )
				n = m_AnsiPara.Add(PARA_NOT);
			m_AnsiPara[n].m_Str += (TCHAR)ch;
			m_AnsiPara[n] = PARA_OPT;

		// CSI : >
		} else {
			if ( m_AnsiPara[n][a] != PARA_NOT && m_AnsiPara[n][a] != PARA_OPT )
				a = m_AnsiPara[n].Add(PARA_NOT);
			m_AnsiPara[n][a].m_Str += (TCHAR)ch;
			m_AnsiPara[n][a] = PARA_OPT;
		}
	}

	fc_CSI_RST(m_BackChar);
}

//////////////////////////////////////////////////////////////////////
// fc CSI @-Z

void CTextRam::fc_ICH(int ch)
{
	// CSI @	ICH	Insert Character

	for ( int n = GetAnsiPara(0, 1, 1, m_Cols) ; n > 0 ; n-- )
		INSCHAR();

	fc_POP(ch);
}
void CTextRam::fc_CUU(int ch)
{
	// CSI A	CUU Cursor Up
	int m = GetTopMargin();
	if ( (m_CurY -= GetAnsiPara(0, 1, 0)) < m )
		m_CurY = m;
	LOCATE(m_CurX, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUD(int ch)
{
	// CSI B	CUD Cursor Down
	int m = GetBottomMargin();
	if ( (m_CurY += GetAnsiPara(0, 1, 0)) >= m )
		m_CurY = m - 1;
	LOCATE(m_CurX, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUF(int ch)
{
	// CSI C	CUF Cursor Forward

	int m = GetRightMargin();
	if ( (m_CurX += GetAnsiPara(0, 1, 1)) >= m )
		m_CurX = m - 1;
	LOCATE(m_CurX, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUB(int ch)
{
	// CSI D	CUB Cursor Backward
	int m = GetLeftMargin();
	if ( (m_CurX -= GetAnsiPara(0, 1, 1)) < m )
		m_CurX = m;
	LOCATE(m_CurX, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CNL(int ch)
{
	// CSI E	CNL Move the cursor to the next line.
	int m = GetBottomMargin();
	if ( (m_CurY += GetAnsiPara(0, 1, 1)) >= m )
		m_CurY = m - 1;
	LOCATE((IsOptEnable(TO_DECOM) ? GetLeftMargin() : 0), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CPL(int ch)
{
	// CSI F	CPL Move the cursor to the preceding line.
	int m = GetTopMargin();
	if ( (m_CurY -= GetAnsiPara(0, 1, 1)) < m )
		m_CurY = m;
	LOCATE((IsOptEnable(TO_DECOM) ? GetLeftMargin() : 0), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CHA(int ch)
{
	// CSI G	CHA Move the active position to the n-th character of the active line.
	if ( IsOptEnable(TO_DECOM) ) {
		GetMargin(MARCHK_NONE);
		if ( (m_CurX = GetAnsiPara(0, 1, 1) - 1 + m_Margin.left) >= m_Margin.right )
			m_CurX = m_Margin.right - 1;
		LOCATE(m_CurX, m_CurY);
	} else
		LOCATE(GetAnsiPara(0, 1, 1) - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUP(int ch)
{
	// CSI H	CUP Cursor Position
	if ( IsOptEnable(TO_DECOM) ) {
		GetMargin(MARCHK_NONE);
		if ( (m_CurX = GetAnsiPara(1, 1, 1) - 1 + m_Margin.left) >= m_Margin.right )
			m_CurX = m_Margin.right - 1;
		if ( (m_CurY = GetAnsiPara(0, 1, 1) - 1 + m_Margin.top) >= m_Margin.bottom )
			m_CurY = m_Margin.bottom - 1;
		LOCATE(m_CurX, m_CurY);
	} else
		LOCATE(GetAnsiPara(1, 1, 1) - 1, GetAnsiPara(0, 1, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_CHT(int ch)
{
	// CSI I	CHT Move the active position n tabs forward.
	for ( int i = GetAnsiPara(0, 1, 0, m_Cols) ; i > 0 ; i-- )
		TABSET(TAB_COLSNEXT);
	fc_POP(ch);
}
void CTextRam::fc_ED(int ch)
{
	// CSI J	ED Erase in page
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, ERM_ISOPRO);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines, ERM_ISOPRO);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1, ERM_ISOPRO);
		ERABOX(0, 0, m_Cols, m_CurY, ERM_ISOPRO);
		break;
	case 2:
//		LOCATE(0, 0);
		ERABOX(0, 0, m_Cols, m_Lines, ERM_ISOPRO);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_EL(int ch)
{
	// CSI K	EL Erase in line
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, ERM_ISOPRO | ERM_SAVEDM);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1, ERM_ISOPRO | ERM_SAVEDM);
		break;
	case 2:
		ERABOX(0, m_CurY, m_Cols, m_CurY + 1, ERM_ISOPRO | ERM_SAVEDM);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_IL(int ch)
{
	// CSI L	IL Insert Line

	if ( GetMargin(MARCHK_LINES) ) {
		for ( int n = GetAnsiPara(0, 1, 1, m_Lines) ; n > 0 ; n-- )
			BAKSCROLL(m_Margin.left, m_CurY, m_Margin.right, m_Margin.bottom);
		//	TABSET(TAB_INSLINE);	with ?
	}
	fc_POP(ch);
}
void CTextRam::fc_DL(int ch)
{
	// CSI M	DL Delete Line

	if ( GetMargin(MARCHK_LINES) ) {
		for ( int n = GetAnsiPara(0, 1, 1, m_Lines) ; n > 0 ; n-- )
			FORSCROLL(m_Margin.left, m_CurY, m_Margin.right, m_Margin.bottom);
		//	TABSET(TAB_DELINE);		with ?
	}

	fc_POP(ch);
}
void CTextRam::fc_EF(int ch)
{
	// CSI N	EF Erase in field
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, ERM_ISOPRO);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines, ERM_ISOPRO);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1, ERM_ISOPRO);
		ERABOX(0, 0, m_Cols, m_CurY, ERM_ISOPRO);
		break;
	case 2:
//		LOCATE(0, 0);
		ERABOX(0, 0, m_Cols, m_Lines, ERM_ISOPRO);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_EA(int ch)
{
	// CSI O	EA Erase in area
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, ERM_ISOPRO);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines, ERM_ISOPRO);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1, ERM_ISOPRO);
		ERABOX(0, 0, m_Cols, m_CurY, ERM_ISOPRO);
		break;
	case 2:
//		LOCATE(0, 0);
		ERABOX(0, 0, m_Cols, m_Lines, ERM_ISOPRO);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DCH(int ch)
{
	// CSI P	DCH Delete Character

	for ( int n = GetAnsiPara(0, 1, 1, m_Cols) ; n > 0 ; n-- )
		DELCHAR();

	fc_POP(ch);
}
void CTextRam::fc_SEE(int ch)
{
	// CSI Q	SEE SELECT EDITING EXTENT
	fc_POP(ch);
}
void CTextRam::fc_SU(int ch)
{
	// CSI S	SU Scroll Up

	GetMargin(MARCHK_NONE);

	for ( int n = GetAnsiPara(0, 1, 1, m_Lines) ; n > 0 ; n-- )
		FORSCROLL(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);

	fc_POP(ch);
}
void CTextRam::fc_SD(int ch)
{
	// CSI T	SD Scroll Down or xterm Track Mouse
	if ( m_AnsiPara.GetSize() > 1 || GetAnsiPara(0, 1, 0) == 0 ) {
		// Sequence: CSI Pn1 ; Pn2 ; Pn3 ; Pn4 ; Pn5 T
		// Description: Initiate hilite mouse tracking
		// Pn1		0:disable 1:enable
		// Pn2,Pn3	start postion Pn2=x, Pn3=y top(1,1)
		// Pn4		start line
		// Pn5		end line

		m_MouseRect.left   = 0;
		m_MouseRect.right  = m_Cols - 1;
		m_MouseRect.top    = GetAnsiPara(3, 1, 1) - 1;
		m_MouseRect.bottom = GetAnsiPara(4, 1, 1) - 1;

		if ( m_MouseRect.top < 0 )
			m_MouseRect.top = 0;
		else if ( m_MouseRect.top >= m_Lines )
			m_MouseRect.top = m_Lines - 1;
		if ( m_MouseRect.bottom <  m_MouseRect.top )
			m_MouseRect.bottom = m_MouseRect.top;
		else if ( m_MouseRect.bottom >= m_Lines )
			m_MouseRect.bottom    = m_Lines - 1;

	} else {
		GetMargin(MARCHK_NONE);

		for ( int n = GetAnsiPara(0, 1, 1, m_Lines) ; n > 0 ; n-- )
			BAKSCROLL(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
	}
	fc_POP(ch);
}
void CTextRam::fc_NP(int ch)
{
	// CSI U	NP Next Page
	for ( int n = GetAnsiPara(0, 1, 1, 100) ; n > 0 ; n-- )
		PostMessage(WM_VSCROLL, SB_PAGEDOWN, 0);
	fc_POP(ch);
}
void CTextRam::fc_PP(int ch)
{
	// CSI V	PP Preceding Page
	for ( int n = GetAnsiPara(0, 1, 1, 100) ; n > 0 ; n-- )
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
	ERABOX(m_CurX, m_CurY, m_CurX + GetAnsiPara(0, 1, 1), m_CurY + 1, ERM_ISOPRO | ERM_SAVEDM);
	fc_POP(ch);
}
void CTextRam::fc_CVT(int ch)
{
	// CSI Y	CVT Cursor line tabulation

	for ( int n = GetAnsiPara(0, 1, 0, m_Cols) ; n > 0 ; n-- )
		TABSET(TAB_LINENEXT);

	fc_POP(ch);
}
void CTextRam::fc_CBT(int ch)
{
	// CSI Z	CBT Move the active position n tabs backward.

	for ( int n = GetAnsiPara(0, 1, 0, m_Cols) ; n > 0 ; n-- )
		TABSET(TAB_COLSBACK);

	fc_POP(ch);
}
void CTextRam::fc_SRS(int ch)
{
	// CSI [	SRS START REVERSED STRING
	fc_POP(ch);
}
void CTextRam::fc_PTX(int ch)
{
	// CSI \	PTX PARALLEL TEXTS
	fc_POP(ch);
}
void CTextRam::fc_SDS(int ch)
{
	// CSI ]	SDS START DIRECTED STRING
	fc_POP(ch);
}
void CTextRam::fc_SIMD(int ch)
{
	// CSI ^	SIMD Select implicit movement direction
	// 0 the direction of implicit movement is the same as that of the character progression
	// 1 the direction of implicit movement is opposite to that of the character progression.
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI a-z

void CTextRam::fc_HPA(int ch)
{
	// CSI `	HPA Horizontal Position Absolute
	if ( IsOptEnable(TO_DECOM) ) {
		GetMargin(MARCHK_NONE);
		if ( (m_CurX = GetAnsiPara(0, 1, 1) - 1 + m_Margin.left) < m_Margin.left )
			m_CurX = m_Margin.left;
		else if ( m_CurX >= m_Margin.right )
			m_CurX = m_Margin.right - 1;
		LOCATE(m_CurX, m_CurY);
	} else
		LOCATE(GetAnsiPara(0, 1, 1) - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_HPR(int ch)
{
	// CSI a	HPR Horizontal Position Relative
	if ( IsOptEnable(TO_DECOM) ) {
		GetMargin(MARCHK_NONE);
		if ( (m_CurX += GetAnsiPara(0, 1, 0)) < m_Margin.left )
			m_CurX = m_Margin.left;
		else if ( m_CurX >= m_Margin.right )
			m_CurX = m_Margin.right - 1;
		LOCATE(m_CurX, m_CurY);
	} else
		LOCATE(m_CurX + GetAnsiPara(0, 1, 0), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_REP(int ch)
{
	// CSI b	REP Repeat
	if ( m_LastChar != 0 ) {
		if ( (m_LastChar & 0xFFFFFF00) != 0 )
			m_LastChar = ' ';
		for ( int n = GetAnsiPara(0, 1, 1, m_Cols * m_Lines) ; n > 0 ; n-- )
			PUT1BYTE(m_LastChar, m_BankNow);
		m_LastChar = 0;
	}
	fc_POP(ch);
}
void CTextRam::fc_DA1(int ch)
{
	// CSI c	DA1 Primary Device Attributes
	//
	//	 1 132 columns							vt400
	//	 2 Printer port extension				vt400
	//	 3 ReGIS Graphics						kermit
	//	 4 Sixel Graphics						kermit
	//	 6 Selective erase						vt400
	//	 7 soft character set (DRCS)			vt400
	//	 8 user-defined-keys (UDK)				vt400
	//	 9 national replacement charsets		kermit
	//	10 text ruling vector
	//	11 25th status line
	//	12 Serbo-Croatian (SCS)					vt500
	//	13 local editing mode					kermit
	//	14 8-bit architecture
	//	15 Technical character set				vt400
	//	16 locator device port (ReGIS)			kermit
	//	17 terminal state reports
	//	18 Windowing capability					vt400
	//	19 two sessions							vt400
	//	21 Horizontal scrolling					vt400
	//	22 ANSI color, VT525					vt500
	//	23 Greek extension						vt500
	//	24 Turkish extension					vt500
	//	29 ANSI text locator					DXterm
	//	39 page memory extension
	//	42 ISO Latin-2 character set			vt500
	//	44 VT PCTerm							vt500
	//	45 Soft key mapping						vt500
	//	46 ASCII terminal emulation				vt500

	if ( m_TermId >= 10 )		// VT520
		UNGETSTR(_T("%s?65;1;2;3;4;6;7;8;9;15;18;21;22;29;39;42;44c"), m_RetChar[RC_CSI]);
	else if ( m_TermId >= 9 )	// VT420 ID (Intnl) CSI ? 64; 1; 2; 7; 8; 9; 15; 18; 21 c
		UNGETSTR(_T("%s?64;1;2;7;8;9;15;18;21c"), m_RetChar[RC_CSI]);
	else if ( m_TermId >= 7 )	// VT320 ID (Intnl) CSI ? 63; 1; 2; 7; 8; 9 c
		UNGETSTR(_T("%s?63;1;2;7;8;9c"), m_RetChar[RC_CSI]);
	else if ( m_TermId >= 5 )	// VT220 ID (Intnl) CSI ? 62; 1; 2; 7; 8; 9 c
		UNGETSTR(_T("%s?62;1;2;7;8;9c"), m_RetChar[RC_CSI]);
	else if ( m_TermId >= 2 )	// VT102 ID ESC [ ? 6 c
		UNGETSTR(_T("%s?6c"), m_RetChar[RC_CSI]);
	else if ( m_TermId >= 1 )	// VT101 ID ESC [ ? 1; 0 c
		UNGETSTR(_T("%s?1;0c"), m_RetChar[RC_CSI]);
	else if ( m_TermId >= 0 )	// VT100 ID ESC [ ? 1; 2 c
		UNGETSTR(_T("%s?1;2c"), m_RetChar[RC_CSI]);

	fc_POP(ch);
}
void CTextRam::fc_VPA(int ch)
{
	// CSI d	VPA Vertical Line Position Absolute
	if ( IsOptEnable(TO_DECOM) ) {
		GetMargin(MARCHK_NONE);
		if ( (m_CurY = GetAnsiPara(0, 1, 1) - 1 + m_Margin.top) < m_Margin.top )
			m_CurY = m_Margin.top;
		else if ( m_CurY >= m_Margin.bottom )
			m_CurY = m_Margin.bottom - 1;
		LOCATE(m_CurX, m_CurY);
	} else
		LOCATE(m_CurX, GetAnsiPara(0, 1, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_VPR(int ch)
{
	// CSI e	VPR Vertical Position Relative
	if ( IsOptEnable(TO_DECOM) ) {
		GetMargin(MARCHK_NONE);
		if ( (m_CurY += GetAnsiPara(0, 1, 0)) < m_Margin.top )
			m_CurY = m_Margin.top;
		else if ( m_CurY >= m_Margin.bottom )
			m_CurY = m_Margin.bottom - 1;
		LOCATE(m_CurX, m_CurY);
	} else
		LOCATE(m_CurX, m_CurY + GetAnsiPara(0, 1, 0));
	fc_POP(ch);
}
void CTextRam::fc_HVP(int ch)
{
	// CSI f	HVP Horizontal and Vertical Position

	if ( IsOptEnable(TO_DECOM) ) {
		GetMargin(MARCHK_NONE);
		if ( (m_CurX = GetAnsiPara(1, 1, 1) - 1 + m_Margin.left) >= m_Margin.right )
			m_CurX = m_Margin.right - 1;
		if ( (m_CurY = GetAnsiPara(0, 1, 1) - 1 + m_Margin.top) >= m_Margin.bottom )
			m_CurY = m_Margin.bottom - 1;
		LOCATE(m_CurX, m_CurY);
	} else
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

	if ( (m_ClipFlag & OSC52_WRITE) == 0 )
		return;

	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		if ( IsOptEnable(TO_DECPEX) )
			EditCopy(GetCalcPos(0, 0), GetCalcPos(m_Cols - 1, m_Lines - 1), FALSE, TRUE);
		else
			EditCopy(GetCalcPos(0, m_TopY), GetCalcPos(m_Cols - 1, m_BtmY - 1), FALSE, TRUE);
		break;
	case 1:
		EditCopy(GetCalcPos(0, m_CurY), GetCalcPos(m_Cols - 1, m_CurY), FALSE, TRUE);
		break;
	}
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
	int n, i, a;

	if ( m_AnsiPara.GetSize() <= 0 )
		m_AnsiPara.Add(0);

	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		TRACE("Para %s\n", CStringA(m_AnsiPara[n].m_Str));
		if ( m_AnsiPara[n].IsOpt() )
			continue;
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

		case 21: m_AttNow.at |= ATT_DUNDER; break;					// 21 doubly underlined
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
		case 48:
			a = n;	// base index
			// ;(Semicolon) Style	38;5;n or 38;2;r;g;b
			if ( m_AnsiPara[n].GetSize() == 0 && m_AnsiPara.GetSize() > (n + 1) ) {
				m_AnsiPara[a].Add(m_AnsiPara[++n]);		// 5/2

				// 38;5:n or 38;2:r:g:b
				if ( m_AnsiPara[n].GetSize() > 0 ) {
					for ( i = 0 ; i < m_AnsiPara[n].GetSize() ; i++ )
						m_AnsiPara[a].Add(m_AnsiPara[n][i]);

				// 38;5;n
				} else if ( m_AnsiPara[a][0] == 5 && m_AnsiPara.GetSize() > (n + 1) ) {
					m_AnsiPara[a].Add(m_AnsiPara[++n]);	// n

				// 38;2;r;g;b or 38;2;r:g:b
				} else if ( m_AnsiPara[a][0] == 2 && m_AnsiPara.GetSize() > (n + 1) ) {
					m_AnsiPara[a].Add(m_AnsiPara[++n]);	// r

					// 38;2;r:g:b
					if ( m_AnsiPara[n].GetSize() > 0 ) {
						for ( i = 0 ; i < m_AnsiPara[n].GetSize() ; i++ )
							m_AnsiPara[a].Add(m_AnsiPara[n][i]);

					// 38;2;r;g;b
					} else if ( m_AnsiPara.GetSize() > (n + 2) ) {
						m_AnsiPara[a].Add(m_AnsiPara[++n]);	// g
						m_AnsiPara[a].Add(m_AnsiPara[++n]);	// b
					}
				}
			}

			// Para[a]    = 38
			// Para[a][0] = 2 or 5
			// Para[a][1] = n    r
			// Para[a][2] =      g
			// Para[a][3] =      b

			if ( m_AnsiPara[a].GetSize() < 1 )
				break;

			for ( i = 0 ; i < m_AnsiPara[a].GetSize() ; i++ ) {
				if ( m_AnsiPara[a][i].IsEmpty() )
					m_AnsiPara[a][i] = 0;
			}

			switch(m_AnsiPara[a][0]) {
			case 2:				// RGB color
				if ( m_AnsiPara[a].GetSize() < 4 )
					break;
				if ( m_AnsiPara[a] == 38 )
					m_AttNow.fc = GETCOLIDX(m_AnsiPara[a][1], m_AnsiPara[a][2], m_AnsiPara[a][3]);
				else
					m_AttNow.bc = GETCOLIDX(m_AnsiPara[a][1], m_AnsiPara[a][2], m_AnsiPara[a][3]);
				break;
			case 5:				// 256 index color
				if ( m_AnsiPara[a].GetSize() < 2 || m_AnsiPara[a][1] > 255 )
					break;
				if ( m_AnsiPara[a] == 38 )
					m_AttNow.fc = m_AnsiPara[a][1];
				else
					m_AttNow.bc = m_AnsiPara[a][1];
				break;
			}
			break;

		case 39:
			m_AttNow.fc = m_DefAtt.fc;
			break;
		case 40: case 41: case 42: case 43:
		case 44: case 45: case 46: case 47:
			m_AttNow.bc = (GetAnsiPara(n, 0, 0) - 40);
			break;
		case 49:
			m_AttNow.bc = m_DefAtt.bc;
			break;

		case 51: m_AttNow.at |= ATT_FRAME;  m_FrameCheck = TRUE; break;		// 51  framed
		case 52: m_AttNow.at |= ATT_CIRCLE; m_FrameCheck = TRUE; break;		// 52  encircled
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
	if ( IsOptEnable(TO_DECECM) == 0 ) {
		m_AttSpc.fc = m_AttNow.fc;
		m_AttSpc.bc = m_AttNow.bc;
	}
	if ( IsOptEnable(TO_RLGCWA) != 0 ) {
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
		UNGETSTR(_T("%s0n"), m_RetChar[RC_CSI]);
		break;
	case 6:	// a report of the active presentation position or of the active data position in the form of ACTIVE POSITION REPORT (CPR) is requested
		if ( IsOptEnable(TO_DECOM) ) {
			int x = m_CurX;
			int y = m_CurY;

			GetMargin(MARCHK_NONE);

			if ( x < m_Margin.left )
				x = m_Margin.left;
			else if ( x >= m_Margin.right )
				x = m_Margin.right - 1;

			if ( y < m_Margin.top )
				y = m_Margin.top;
			else if ( y >= m_Margin.bottom )
				y = m_Margin.bottom - 1;

			UNGETSTR(_T("%s%d;%dR"), m_RetChar[RC_CSI], y - m_Margin.top + 1, x - m_Margin.left + 1);
		} else
			UNGETSTR(_T("%s%d;%dR"), m_RetChar[RC_CSI], m_CurY + 1, m_CurX + 1);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DAQ(int ch)
{
	// CSI o	DAQ Define area qualification
	/*
		0 unprotected and unguarded
		1 protected and guarded
		2 graphic character input
		3 numeric input
		4 alphabetic input
		5 input aligned on the last character position of the qualified area
		6 fill with ZEROs
		7 set a character tabulation stop at the active presentation position
			(the first character position of the qualified area) to indicate the
			beginning of a field
		8 protected and unguarded
		9 fill with SPACEs
		10 input aligned on the first character position of the qualified area
		11 the order of the character positions in the input field is
			reversed, i.e. the last position in each line becomes the first and
			vice versa; input begins at the new first position.
	*/

	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		m_AttNow.em &= ~EM_ISOPROTECT;
		break;
	case 1:
	case 8:
		m_AttNow.em |= EM_ISOPROTECT;
		break;
	}

	fc_POP(ch);
}
void CTextRam::fc_ORGBFAT(int ch)
{
	// CSI p	ORGBFAT Begin field attribute : DEC private
	int n = GetAnsiPara(0, 0, 0);
	m_AttNow.at = 0;
	if ( (n &  1) != 0 ) m_AttNow.at |= ATT_BOLD;
	if ( (n &  2) != 0 ) m_AttNow.at |= ATT_BLINK;
	if ( (n &  4) != 0 ) m_AttNow.at |= ATT_SECRET;
	if ( (n &  8) != 0 ) m_AttNow.at |= ATT_UNDER;
	if ( (n & 16) != 0 ) m_AttNow.at |= ATT_REVS;
	if ( IsOptEnable(TO_RLGCWA) != 0 )
		m_AttSpc.at = m_AttNow.at;
	fc_POP(ch);
}
void CTextRam::fc_DECSSL(int ch)
{
	// CSI p	DECSSL Select Set-Up Language
	// 0, 1 or none English
	// 2 French
	// 3 German
	// 4 Spanish
	// 5 Italian
	m_LangMenu = GetAnsiPara(0, 1, 1);
	fc_POP(ch);
}
void CTextRam::fc_DECLL(int ch)
{
	// CSI q	DECLL Load LEDs

	int n;
	CString str;

	if ( m_AnsiPara.GetSize() <= 0 )
		m_AnsiPara.Add(0);

	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( m_AnsiPara[n].IsOpt() )
			continue;
		switch(GetAnsiPara(n, 0, 0)) {
		case 0:	// Clear All LEDs (default)
			m_StsLed = 0;
			break;
		case 1:	// Light L1	Scroll Lock LED
			m_StsLed |= 001;
			break;
		case 2:	// Light L2 Num Lock LED
			m_StsLed |= 002;
			break;
		case 3:	// Light L3	Caps Lock LED
			m_StsLed |= 004;
			break;
		case 4:	// Light L4
			m_StsLed |= 010;
			break;
		}
	}

	str.Format(_T("%s%s%s%s"), (m_StsLed & 001) ? _T("1") : _T("-"), (m_StsLed & 002) ? _T("2") : _T("-"), (m_StsLed & 004) ? _T("3") : _T("-"), (m_StsLed & 010) ? _T("4") : _T("-"));
	((CMainFrame *)AfxGetMainWnd())->SetMessageText(str);

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
void CTextRam::fc_DECSLRM(int ch)
{
	// CSI s	DECSLRM Set left and right margins
	// Sets left margin to Pn1, right margin to Pn2

	if ( (m_LeftX  = GetAnsiPara(0, 1, 1) - 1) < 0 )
		m_LeftX = 0;
	else if ( m_LeftX >= m_Cols )
		m_LeftX = m_Cols - 1;

	if ( (m_RightX = GetAnsiPara(1, m_Cols, 1) - 1 + 1) < m_LeftX )
		m_RightX = m_LeftX + 1;
	else if ( m_RightX > m_Cols )
		m_RightX = m_Cols;

	fc_POP(ch);
}
void CTextRam::fc_DECSLPP(int ch)
{
	// CSI t	DECSLPP Set Lines Per Page

	InitScreen(m_Cols, GetAnsiPara(0, m_Lines, 2));

	fc_POP(ch);
}
void CTextRam::fc_DECSHTS(int ch)
{
	// CSI u	DECSHTS Set horizontal tab stops
	int n, s;
	//TABSET(TAB_COLSALLCLR);
	s = m_CurX;
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( m_AnsiPara[n].IsOpt() )
			continue;
		m_CurX = GetAnsiPara(n, 1, 1) - 1;
		if ( m_CurX >= 0 && m_CurX < m_Cols )
			TABSET(TAB_COLSSET);
	}
	m_CurX = s;
	fc_POP(ch);
}
void CTextRam::fc_DECSVTS(int ch)
{
	// CSI v	DECSVTS Set vertical tab stops
	int n, s;
	//TABSET(TAB_LINEALLCLR);
	s = m_CurY;
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( m_AnsiPara[n].IsOpt() )
			continue;
		m_CurY = GetAnsiPara(n, 1, 1) - 1;
		if ( m_CurY >= 0 && m_CurY < m_Lines )
			TABSET(TAB_LINESET);
	}
	m_CurY = s;
	fc_POP(ch);
}
void CTextRam::fc_SCOSC(int ch)
{
	// CSI s	SCOSC Save Cursol Pos

	if ( IsOptEnable(TO_DECLRMM) )
		fc_DECSLRM(ch);
	else {
		m_Save_CurX   = m_CurX;
		m_Save_CurY   = m_CurY;
		m_Save_DoWarp = m_DoWarp;
		fc_POP(ch);
	}
}
void CTextRam::fc_XTWOP(int ch)
{
	// CSI t	XTWOP XTERM WINOPS
	int i;
	int n = GetAnsiPara(0, 0, 0);
	int w = 6;
	int h = 12;
	CRLoginView *pView;

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
		if ( IsOptEnable(TO_RLFONT) ) {				// ReSize Cols, Lines
			InitScreen(GetAnsiPara(2, m_Cols, 4), GetAnsiPara(1, m_Lines, 2));
		} else {
			if ( GetAnsiPara(2, 0, 0) > m_DefCols[0] )
				EnableOption(TO_DECCOLM);
			else
				DisableOption(TO_DECCOLM);
			InitScreen(m_DefCols[IsOptValue(TO_DECCOLM, 1)], GetAnsiPara(1, m_Lines, 2));
		}
		break;
    case 9:			/* Maximize or restore mx = p1 */
		break;
    case 11:    	/* Report the window's state ESC[1/2t */
		UNGETSTR(_T("%s1t"), m_RetChar[RC_CSI]);
		break;
    case 13:    	/* Report the window's position ESC[3;x;yt */
		UNGETSTR(_T("%s3;%d;%dt"), m_RetChar[RC_CSI], 0, 0);
		break;
    case 14:    	/* Report the window's size in pixels ESC[4;h;wt */
		if ( m_pDocument != NULL && (pView = (CRLoginView *)m_pDocument->GetAciveView()) != NULL ) {
			w = pView->m_CharWidth;
			h = pView->m_CharHeight;
		}
		UNGETSTR(_T("%s4;%d;%dt"), m_RetChar[RC_CSI], m_Lines * h, m_Cols * w);
		break;
    case 18:    	/* Report the text's size in characters ESC[8;l;ct */
		UNGETSTR(_T("%s8;%d;%dt"), m_RetChar[RC_CSI], m_Lines, m_Cols);
		break;
    case 19:    	/* Report the screen's size, in characters ESC[9;h;wt */
		if ( m_pDocument != NULL && (pView = (CRLoginView *)m_pDocument->GetAciveView()) != NULL ) {
			w = pView->m_CharWidth;
			h = pView->m_CharHeight;
		}
		UNGETSTR(_T("%s9;%d;%dt"), m_RetChar[RC_CSI], h, w);
		break;
    case 20:    	/* Report the icon's label ESC]LxxxxESC\ */
    case 21:   		/* Report the window's title ESC]lxxxxxxESC\ */
		if ( (m_TitleMode & WTTL_REPORT) == 0 ) {
			CStringA mbs, tmp;
			if ( (m_XtOptFlag & XTOP_GETUTF) != 0 )
				m_IConv.StrToRemote(m_SendCharSet[UTF8_SET], m_pDocument->GetTitle(), mbs);
			else
				m_IConv.StrToRemote(m_SendCharSet[m_KanjiMode], m_pDocument->GetTitle(), mbs);

			if ( (m_XtOptFlag & XTOP_GETHEX) != 0 ) {
				CBuffer buf;
				buf.Base16Encode((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
				mbs.Empty();
				for ( LPCTSTR p = buf ; *p != _T('\0') ; )
					mbs += *(p++);
			}

			tmp.Format("%s%c%s%s", TstrToMbs(m_RetChar[RC_OSC]), (n == 20 ? 'L':'l'), mbs, TstrToMbs(m_RetChar[RC_ST]));
			m_pDocument->SocketSend((void *)(LPCSTR)mbs, mbs.GetLength());

		} else
			UNGETSTR(_T("%s%c%s"), m_RetChar[RC_OSC], (n == 20 ? 'L':'l'), m_RetChar[RC_ST]);
		break;

	case 22:		/* save the window's title(s) on stack  */
		switch(GetAnsiPara(1, 0, 0)) {
		case 0:
		case 1:
		case 2:
			m_TitleStack.Add(m_pDocument->GetTitle());
			break;
		}
		break;
	case 23:		/* restore the window's title(s) from stack */
		switch(GetAnsiPara(1, 0, 0)) {
		case 0:
		case 1:
		case 2:
			if ( (i = m_TitleStack.GetSize() - 1) >= 0 ) {
				m_pDocument->SetTitle(m_TitleStack[i]);
				m_TitleStack.RemoveAt(i);
			}
			break;
		}
		break;

	default:
		// CSI Pn t	DECSLPP Set Lines Per Page
		if ( n >= 24 ) {
			fc_DECSLPP(ch);
			return;
		}
		break;
	}

	fc_POP(ch);
}
void CTextRam::fc_SCORC(int ch)
{
	// CSI u	SCORC Load Cursol Pos
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
		UNGETSTR(_T("%s2;1;1;112;112;1;0x"), m_RetChar[RC_CSI]);
		break;
	case 1:
		UNGETSTR(_T("%s3;1;1;112;112;1;0x"), m_RetChar[RC_CSI]);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECTST(int ch)
{
	// CSI y	DECTST Invoke confidence test

	switch(GetAnsiPara(0, 0, 0)) {
	case 0:		// All Tests" (1,2,3,6)
	case 1:		// Power-Up Self Test
	case 2:		// RS-232 Port Data Loopback Test
	case 3:		// Printer Port Loopback Test
	case 4:		// Speed Select and Speed Indicator Test
	case 5:		// Reserved - No action
	case 6:		// RS-232 Port Modem Control Line Loopback Test
	case 7:		// DEC-423 Port Loopback Test
	case 8:		// Parallel Port Loopback Test
	case 9:		// Repeat (Loop On) Other Tests In Parameter String
		LOCATE(0, 0);
		ERABOX(0, 0, m_Cols, m_Lines);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECVERP(int ch)
{
	// CSI z	DECVERP Set vertical pitch
	fc_POP(ch);
}
void CTextRam::fc_DECFNK(int ch)
{
	// CSI Ps1 ; Ps2 ~	DECFNK Function Key
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
void CTextRam::fc_DECSED(int ch)
{
	// CSI ? J	DECSED Selective Erase in Display
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, ERM_DECPRO | ERM_SAVEDM);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines, ERM_DECPRO | ERM_SAVEDM);
		break;
	case 1:
		ERABOX(0, 0, m_Cols, m_CurY, ERM_DECPRO | ERM_SAVEDM);
		ERABOX(0, m_CurY, m_CurX + 1, m_CurY + 1, ERM_DECPRO | ERM_SAVEDM);
		break;
	case 2:
		ERABOX(0, 0, m_Cols, m_Lines, ERM_DECPRO | ERM_SAVEDM);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECSEL(int ch)
{
	// CSI ? K	DECSEL Selective Erase in Line
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, ERM_DECPRO | ERM_SAVEDM);
		break;
	case 1:
		ERABOX(0, m_CurY, m_CurX, m_CurY + 1, ERM_DECPRO | ERM_SAVEDM);
		break;
	case 2:
		ERABOX(0, m_CurY, m_Cols, m_CurY + 1, ERM_DECPRO | ERM_SAVEDM);
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
void CTextRam::fc_DECMC(int ch)
{
	// CSI ? i	DECMC	Media Copy (DEC)

	fc_POP(ch);

	if ( (m_ClipFlag & OSC52_WRITE) == 0 )
		return;

	switch(GetAnsiPara(0, 1, 0)) {
	case 0:
		if ( IsOptEnable(TO_DECPEX) )
			EditCopy(GetCalcPos(0, 0), GetCalcPos(m_Cols - 1, m_Lines - 1), FALSE, TRUE);
		else
			EditCopy(GetCalcPos(0, m_TopY), GetCalcPos(m_Cols - 1, m_BtmY - 1), FALSE, TRUE);
		break;
	case 1:
		EditCopy(GetCalcPos(0, m_CurY), GetCalcPos(m_Cols - 1, m_CurY), FALSE, TRUE);
		break;
	case 10:
		EditCopy(GetCalcPos(0, 0), GetCalcPos(m_Cols - 1, m_Lines - 1), FALSE, TRUE);
	case 11:
		EditCopy(GetCalcPos(0, 0 - m_HisLen + m_Lines), GetCalcPos(m_Cols - 1, m_Lines - 1), FALSE, TRUE);
		break;
	}
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
		else if ( i >= 8400 && i < 8512 )
			i -= 8000;		// 400-511
		else if ( i == 7727 )
			i = TO_RLCKMESC;	// 7727 - Application Escape mode を有効にする。				Application Escape mode を無効にする。  
		else if ( i == 7786 )
			i = TO_RLMSWAPE;	// 7786 - マウスホイール - カーソルキー変換を有効にする。		カーソルキー変換を無効にする。  
		else if ( i == 8800 )
			i = TO_DRCSMMv1;	// 8800	- Unicode16面のISO-2022マッピング有効					マッピング無効
		else if ( i >= 200 )
			continue;

		ANSIOPT(ch, i);

		switch(i) {
		case TO_DECCOLM:	// 3 DECCOLM Column mode
		case TO_XTMCSC:		// 40 XTERM Column switch control
			if ( IsOptEnable(TO_XTMCSC) ) {
				InitScreen(m_DefCols[IsOptValue(TO_DECCOLM, 1)], m_Lines);
				if ( !IsOptEnable(TO_DECNCSM) ) {
					LOCATE(0, 0);
					ERABOX(0, 0, m_Cols, m_Lines);
				}
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
				return;		// NO fc_POP
			}
			break;

		case TO_XTMABUF:	// 47 XTERM alternate buffer
		case TO_XTALTSCR:	// 1047 XTERM Use Alternate/Normal screen buffer
			if ( IsOptEnable(TO_RLALTBDIS) )
				break;
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
			if ( IsOptEnable(TO_RLALTBDIS) )
				break;
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
			m_MouseTrack = (IsOptEnable(i) ? MOS_EVENT_X10 : MOS_EVENT_NONE);
			m_MouseOldSw = (-1);
			m_MouseRect.SetRectEmpty();
			m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
			break;
		case TO_XTNOMTRK:	// 1000 X11 normal mouse tracking
		case TO_XTHILTRK:	// 1001 X11 hilite mouse tracking
		case TO_XTBEVTRK:	// 1002 X11 button-event mouse tracking
		case TO_XTAEVTRK:	// 1003 X11 any-event mouse tracking
			m_MouseTrack = (IsOptEnable(i) ? (i - TO_XTNOMTRK + MOS_EVENT_NORM) : MOS_EVENT_NONE);
			m_MouseOldSw = (-1);
			m_MouseRect.SetRectEmpty();
			m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
			break;

		case TO_XTEXTMOS:	// 1005 X11 Enable Extended Mouse Mode
		case TO_XTSGRMOS:	// 1006 X11 SGR Extended Mouse Mode
		case TO_XTURXMOS:	// 1015 X11 URXVT Extended Mouse Mode
			if ( ch == 'h' ) {
				DisableOption(TO_XTEXTMOS);
				DisableOption(TO_XTSGRMOS);
				DisableOption(TO_XTURXMOS);
				EnableOption(i);	// Last select Enable
			} else if ( ch == 'l' ) {
				DisableOption(TO_XTEXTMOS);
				DisableOption(TO_XTSGRMOS);
				DisableOption(TO_XTURXMOS);
			}
			break;

		case TO_IMECTRL:	// IME Open/Close
			{
				int rt = (-1);
				CRLoginView *pView;
				if ( (pView = (CRLoginView *)GetAciveView()) != NULL ) {
					switch(ch) {
					case 'h':	// CSI ? h	DECSET
						rt = pView->ImmOpenCtrl(1);	// open
						break;
					case 'l':	// CSI ? l	DECRST
						rt = pView->ImmOpenCtrl(0);	// close
						break;
					case 'r':	// CSI ? r	XTERM_RESTORE
						if ( IsOptEnable(i) )
							rt = pView->ImmOpenCtrl(1);	// open
						else
							rt = pView->ImmOpenCtrl(0);	// close
						break;
					case 's':	// CSI ? s	XTERM_SAVE
						rt = pView->ImmOpenCtrl(2);	// stat
						break;
					}
					if ( rt == 1 )
						EnableOption(i);
					else if ( rt == 0 )
						DisableOption(i);

					if ( ch == 's' )
						ANSIOPT(ch, i);
				}
			}
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
		UNGETSTR(_T("%s?20n"), m_RetChar[RC_CSI]);	// malfunction detected [also LCP01]
		break;
	case 6:		// a report of the active presentation position or of the active data position in the form of ACTIVE POSITION REPORT (CPR) is requested
		if ( IsOptEnable(TO_DECOM) ) {
			int x = m_CurX;
			int y = m_CurY;

			GetMargin(MARCHK_NONE);

			if ( x < m_Margin.left )
				x = m_Margin.left;
			else if ( x >= m_Margin.right )
				x = m_Margin.right - 1;

			if ( y < m_Margin.top )
				y = m_Margin.top;
			else if ( y >= m_Margin.bottom )
				y = m_Margin.bottom - 1;

		} else
			UNGETSTR(_T("%s?%d;%d;%dR"), m_RetChar[RC_CSI], m_CurY + 1, m_CurX + 1, m_Page + 1);
		break;
	case 15:	// printer status
		UNGETSTR(_T("%s?13n"), m_RetChar[RC_CSI]);		// no printer
		break;
	case 25:	// User Definable Key status
		UNGETSTR(_T("%s?21n"), m_RetChar[RC_CSI]);		// UDKs are locked
		break;
	case 26:	// keyboard dialect
		UNGETSTR(_T("%s?27;1n"), m_RetChar[RC_CSI]);	// North American/ASCII
		break;
	case 53:	// xterm locator status
		UNGETSTR(_T("%s?50n"), m_RetChar[RC_CSI]);		// locator ready
		break;
	case 55:	// locator status
		UNGETSTR(_T("%s?%dn"), m_RetChar[RC_CSI], ((m_Loc_Mode & LOC_MODE_ENABLE) == 0 ? 50 : 58));	// locator ready/busy
		break;
	case 56:	// locator type
		UNGETSTR(_T("%s?57;1n"), m_RetChar[RC_CSI]);	// Locator is a mouse
		break;
	case 62:	// Macro Space Report
		UNGETSTR(_T("%s%d*{"), m_RetChar[RC_CSI], 4096 / 16);	// The terminal indicates the number of bytes available for macro
		break;
	case 63:	// Request checksum of macro definitions
		{
			// DCS Pn ! ~ Ps ST
			int Pid = GetAnsiPara(1, 0, 0);
			int sum = 0;
			if ( Pid < 64 ) {
				LPBYTE p = m_Macro[Pid].GetPtr();
				for ( int n = m_Macro[Pid].GetSize() ; n > 0 ; n-- )
					sum += *(p++);
			}
			UNGETSTR(_T("%s%d!~%04x%s"), m_RetChar[RC_DCS], Pid, sum & 0xFFFF, m_RetChar[RC_ST]);
		}
		break;
	case 75:	// Data Integrity Report
		UNGETSTR(_T("%s?70n"), m_RetChar[RC_CSI]);		// Ready, no power loss or comm errors
		break;
	case 85:	// Multiple-Session Configuration Status Report
		UNGETSTR(_T("%s?83n"), m_RetChar[RC_CSI]);		// The terminal is not configured for multiple-session operation.
		break;

	case 8840:	// TNREPTAMB UnicodeAタイプ文字がシングル幅の時8841, ダブル幅の時8842
		UNGETSTR(_T("%s?%dn"), m_RetChar[RC_CSI], (IsOptEnable(TO_RLUNIAWH) ? 8841 : 8842));
		break;
	}
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI $ ...
//
void CTextRam::fc_DECRQM(int ch)
{
	// CSI $p	DECRQM Request mode settings
	int n, f = 0;

	if ( (n = GetAnsiPara(0, 0, 0)) > 0 && n < 100 )
		f = IsOptEnable(200 + n) ? 1 : 2;

	UNGETSTR(_T("%s%d;%d$y"), m_RetChar[RC_CSI], n, f);
	fc_POP(ch);
}
void CTextRam::fc_DECCARA(int ch)
{
	// CSI $r	DECCARA Change Attribute in Rectangle
	int n, i, x, y;
	CVram *vp;

	SetAnsiParaArea(0);

	while ( m_AnsiPara.GetSize() < 5 )
		m_AnsiPara.Add(0);

	if ( !m_Exact ) {
		m_AnsiPara[1] = 0;
		m_AnsiPara[3] = m_Cols - 1;
	}

	for ( y = m_AnsiPara[0] ; y <= m_AnsiPara[2] ; y++ ) {
		if ( y < m_TopY || y >= m_BtmY )
			continue;

		for ( x = m_AnsiPara[1] ; x <= m_AnsiPara[3] ; x++ ) {
			if ( IsOptEnable(TO_DECLRMM) && (x < m_LeftX || x >= m_RightX) )
				continue;

			vp = GETVRAM(x, y);
			for ( n = 4 ; n < m_AnsiPara.GetSize() ; n++ ) {
				if ( m_AnsiPara[n].IsOpt() )
					continue;
				switch(GetAnsiPara(n, 0, 0)) {
				case 0: vp->pr.at = 0; break;
				case 1: vp->pr.at |= ATT_BOLD;   break;
				case 2: vp->pr.at |= ATT_HALF;   break;
				case 3: vp->pr.at |= ATT_ITALIC; break;
				case 4: vp->pr.at |= ATT_UNDER;  break;
				case 5: vp->pr.at |= ATT_SBLINK; break;
				case 6: vp->pr.at |= ATT_BLINK;  break;
				case 7: vp->pr.at |= ATT_REVS;   break;
				case 8: vp->pr.at |= ATT_SECRET; break;
				case 9: vp->pr.at |= ATT_LINE;   break;
				case 21: vp->pr.at |= ATT_DUNDER; break;

				case 10: case 11: case 12:
				case 13: case 14: case 15:
				case 16: case 17: case 18:
				case 19: case 20:
					vp->pr.ft = GetAnsiPara(n, 0, 0) - 10;
					break;

				case 22: vp->pr.at &= ~(ATT_BOLD | ATT_HALF); break;
				case 23: vp->pr.at &= ~ATT_ITALIC; break;
				case 24: vp->pr.at &= ~ATT_UNDER; break;
				case 25: vp->pr.at &= ~(ATT_SBLINK | ATT_BLINK); break;
				case 27: vp->pr.at &= ~ATT_REVS; break;
				case 28: vp->pr.at &= ~ATT_SECRET; break;
				case 29: vp->pr.at &= ~ATT_LINE; break;

				case 30: case 31: case 32: case 33:
				case 34: case 35: case 36: case 37:
					vp->pr.fc = (GetAnsiPara(n, 0, 0) - 30);
					break;
				case 38:
					if ( GetAnsiPara(n + 1, 0, 0) == 5 ) {	// 256 color
						if ( (i = GetAnsiPara(n + 2, 0, 0)) < 256 )
							vp->pr.fc = i;
						n += 2;
					}
					break;
				case 39:
					vp->pr.fc = m_DefAtt.fc;
					break;
				case 40: case 41: case 42: case 43:
				case 44: case 45: case 46: case 47:
					vp->pr.bc = (GetAnsiPara(n, 0, 0) - 40);
					break;
				case 48:
					if ( GetAnsiPara(n + 1, 0, 0) == 5 ) {	// 256 color
						if ( (i = GetAnsiPara(n + 2, 0, 0)) < 256 )
							vp->pr.bc = i;
						n += 2;
					}
					break;
				case 49:
					vp->pr.bc = m_DefAtt.bc;
					break;

				case 51: vp->pr.at |= ATT_FRAME; break;					// 51  framed
				case 52: vp->pr.at |= ATT_CIRCLE; break;					// 52  encircled
				case 53: vp->pr.at |= ATT_OVER;   break;					// 53  overlined
				case 54: vp->pr.at &= ~(ATT_FRAME | ATT_CIRCLE); break;	// 54  not framed, not encircled
				case 55: vp->pr.at &= ~ATT_OVER; break;					// 55  not overlined

				case 60: vp->pr.at |= ATT_RSLINE; break;					// 60  ideogram underline or right side line
				case 61: vp->pr.at |= ATT_RDLINE; break;					// 61  ideogram double underline or double line on the right side
				case 62: vp->pr.at |= ATT_LSLINE; break;					// 62  ideogram overline or left side line
				case 63: vp->pr.at |= ATT_LDLINE; break;					// 63  ideogram double overline or double line on the left side
				case 64: vp->pr.at |= ATT_STRESS; break;					// 64  ideogram stress marking
				case 65: vp->pr.at &= ~(ATT_RSLINE | ATT_RDLINE |
							ATT_LSLINE | ATT_LDLINE | ATT_STRESS); break;	// 65  cancels the effect of the rendition aspects established by parameter values 60 to 64

				case 90: case 91: case 92: case 93:
				case 94: case 95: case 96: case 97:
					vp->pr.fc = (GetAnsiPara(n, 0, 0) - 90 + 8);
					break;
				case 100: case 101:  case 102: case 103:
				case 104: case 105:  case 106: case 107:
					vp->pr.bc = (GetAnsiPara(n, 0, 0) - 100 + 8);
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
	CVram *vp;

	SetAnsiParaArea(0);

	while ( m_AnsiPara.GetSize() < 5 )
		m_AnsiPara.Add(0);

	if ( !m_Exact ) {
		m_AnsiPara[1] = 0;
		m_AnsiPara[3] = m_Cols - 1;
	}

	for ( y = m_AnsiPara[0] ; y <= m_AnsiPara[2] ; y++ ) {
		if ( y < m_TopY || y >= m_BtmY )
			continue;

		for ( x = m_AnsiPara[1] ; x <= m_AnsiPara[3] ; x++ ) {
			if ( IsOptEnable(TO_DECLRMM) && (x < m_LeftX || x >= m_RightX) )
				continue;

			vp = GETVRAM(x, y);
			for ( n = 4 ; n < m_AnsiPara.GetSize() ; n++ ) {
				if ( m_AnsiPara[n].IsOpt() )
					continue;
				switch(GetAnsiPara(n, 0, 0)) {
				case 0: vp->pr.at ^= (ATT_BOLD | ATT_HALF | ATT_ITALIC | ATT_UNDER | ATT_SBLINK | ATT_BLINK | ATT_REVS | ATT_SECRET | ATT_LINE); break;
				case 1: vp->pr.at ^= ATT_BOLD; break;
				case 2: vp->pr.at ^= ATT_HALF; break;
				case 3: vp->pr.at ^= ATT_ITALIC; break;
				case 4: vp->pr.at ^= ATT_UNDER; break;
				case 5: vp->pr.at ^= ATT_SBLINK; break;
				case 6: vp->pr.at ^= ATT_BLINK; break;
				case 7: vp->pr.at ^= ATT_REVS; break;
				case 8: vp->pr.at ^= ATT_SECRET; break;
				case 9: vp->pr.at ^= ATT_LINE; break;
				}
			}
		}
	}

	DISPVRAM(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] - m_AnsiPara[1] + 1, m_AnsiPara[2] - m_AnsiPara[0] + 1);

	fc_POP(ch);
}
void CTextRam::fc_DECRQTSR(int ch)
{
	// CSI $u	DECRQTSR Request Terminal State Report

	switch(GetAnsiPara(0, 0, 0)) {
	case 1:
		/*
		 * Request  CSI 1 $ u			Host request for a terminal state report
		 * Response DCS 1 $ s Ps ST		DECTSR Terminal state report
		*/
		UNGETSTR(_T("%s0$s%s"), m_RetChar[RC_DCS], m_RetChar[RC_ST]);
		break;
	case 2:
		/*
		 * Request  CSI 2 $ u			Host request for color table repor
		 * Response DCS 2 $ s Ps ST		DECCTR Color table report
			This is sent by the terminal in response to DECRQTSR.  Ps contains the
			terminal's colour table.  The palette entries are separated by slashes, and
			each one containe (semicolon-separated):

			Color ???
			Colour space (1 => HLS, 2 => RGB)
			Components (H;L;S or R;G;B)
		*/
		{
			int n;
			CString str, tmp;
			COLORREF hls;
			int Ps = GetAnsiPara(1, 0, 0);

			switch(Ps) {
			case 1:		// HLS
				for ( n = 0 ; n < 256 ; n++ ) {
					hls = CGrapWnd::RGBtoHLS(m_ColTab[n]);
					tmp.Format(_T("%d;1;%d;%d;%d"), n,
						GetRValue(hls) * 360 / 100,
						GetGValue(hls) * 100 / 100,
						GetBValue(hls) * 100 / 100);
					if ( n > 0 )
						str += '/';
					str += tmp;
				}
				break;
			case 2:		// RGB
				for ( n = 0 ; n < 256 ; n++ ) {
					tmp.Format(_T("%d;2;%d;%d;%d"), n,
						GetRValue(m_ColTab[n]) * 100 / 255,
						GetGValue(m_ColTab[n]) * 100 / 255,
						GetBValue(m_ColTab[n]) * 100 / 255);
					if ( n > 0 )
						str += '/';
					str += tmp;
				}
				break;
			}

			UNGETSTR(_T("%s2$s%s%s"), m_RetChar[RC_DCS], str, m_RetChar[RC_ST]);
		}
		break;
	}

	fc_POP(ch);
}
void CTextRam::fc_DECCRA(int ch)
{
	// CSI $v	DECCRA Copy Rectangular Area

	//	   0     1     2     3     4     5     6     7
	// CSI Pts ; Pls ; Pbs ; Prs ; Pps ; Ptd ; Pld ; Ppd $ v 
	//     sy    sx    ey    ex    sp    dy    dx    dp

	COPY(GetAnsiPara(4, m_Page + 1, 1) - 1, GetAnsiPara(1, 1, 1) - 1,      GetAnsiPara(0, 1, 1) - 1,
											GetAnsiPara(3, m_Cols, 1) - 1, GetAnsiPara(2, m_Lines, 1) - 1,
		 GetAnsiPara(7, m_Page + 1, 1) - 1, GetAnsiPara(6, 1, 1) - 1,      GetAnsiPara(5, 1, 1) - 1);

	fc_POP(ch);
}
void CTextRam::fc_DECRQPSR(int ch)
{
	// CSI $w	DECRQPSR Request Presentation State Report
	int n, i;
	CString str, wrk;

	switch(GetAnsiPara(0, 0, 0)) {
	case 1:
		/*
		 * From Kermit 3.13 & VT220 pocket guide
		 *
		 * Request  CSI 1 $ w		  cursor information report
		 * Response DCS 1 $ u Pr; Pc; Pp; Srend; Satt; Sflag; Pgl; Pgr; Scss; Sdesig ST		DECCIR
		 *        Pr is cursor row (counted from origin as 1,1)
		 *		  Pc is cursor column
		 *		  Pp is 1, video page, a constant for VT320s
		 *		  Srend = 40h + 8 (rev video on) + 4 (blinking on)
		 *				   + 2 (underline on) + 1 (bold on)
		 *		  Satt = 40h + 1  (selective erase on)
		 *		  Sflag = 40h + 8 (autowrap pending) + 4 (SS3 pending)
		 *				  + 2 (SS2 pending) + 1 (Origin mode on)
		 *		  Pgl = char set in GL (0 = G0, 1 = G1, 2 = G2, 3 = G3)
		 *		  Pgr = char set in GR (same as for Pgl)
		 *		  Scss = 40h + 8 (G3 is 96 char) + 4 (G2 is 96 char)
		 *				  + 2 (G1 is 96 char) + 1 (G0 is 96 char)
		 *		  Sdesig is string of character idents for sets G0...G3, with
		 *				  no separators between set idents.
		 *		  If NRCs are active the set idents (all 94 byte types) are:
		 *		  British	  A	  Italian	  Y
		 *		  Dutch 	  4	  Norwegian/Danish ' (hex 60) or E or 6
		 *		  Finnish	  5 or C  Portuguese	  %6 or g or L
		 *		  French	  R or f  Spanish	  Z
		 *		  French Canadian 9 or Q  Swedish	  7 or H
		 *		  German	  K	  Swiss 	  =
		 *		  Hebrew	  %=
		 *		  (MS Kermit uses any choice when there are multiple)
		 */
		UNGETSTR(_T("%s1$u%d;%d;%d;%c;%c;%c;%d;%d;%c;%s%s%s%s%s%s%s%s%s"), m_RetChar[RC_DCS],
			m_CurY + 1, m_CurX + 1, m_Page + 1,
			0x40 + ((m_AttNow.at & ATT_REVS)  != 0 ? 8 : 0) + ((m_AttNow.at & (ATT_SBLINK | ATT_BLINK)) != 0 ? 4 : 0) + ((m_AttNow.at & ATT_UNDER) != 0 ? 2 : 0) + ((m_AttNow.at & ATT_BOLD)  != 0 ? 1 : 0),
			0x40 + (IsOptEnable(TO_ANSIERM) ? 1 : 0),
			0x40 + (IsOptEnable(TO_DECAWM) ? 8 : 0) + (m_BankSG == 3 ? 4 : 0) + (m_BankSG == 2 ? 2 : 0) + (IsOptEnable(TO_DECOM) ? 1 : 0),
			m_BankGL, m_BankGR,
			0x40 + ((m_BankTab[m_KanjiMode][3] & SET_96) != 0 ? 8 : 0) + ((m_BankTab[m_KanjiMode][2] & SET_96) != 0 ? 4 : 0) + ((m_BankTab[m_KanjiMode][1] & SET_96) != 0 ? 2 : 0) + ((m_BankTab[m_KanjiMode][0] & SET_96) != 0 ? 1 : 0),
			((m_BankTab[m_KanjiMode][0] & SET_94x94) != 0 ? _T("/") : _T("")), m_FontTab[m_BankTab[m_KanjiMode][0]].m_IndexName,
			((m_BankTab[m_KanjiMode][1] & SET_94x94) != 0 ? _T("/") : _T("")), m_FontTab[m_BankTab[m_KanjiMode][1]].m_IndexName,
			((m_BankTab[m_KanjiMode][2] & SET_94x94) != 0 ? _T("/") : _T("")), m_FontTab[m_BankTab[m_KanjiMode][2]].m_IndexName,
			((m_BankTab[m_KanjiMode][3] & SET_94x94) != 0 ? _T("/") : _T("")), m_FontTab[m_BankTab[m_KanjiMode][3]].m_IndexName,
			m_RetChar[RC_ST]);
		break;

	case 2:
		/*
		 * Request  CSI 2 $ w		  tab stop report
		 * Response DCS 2 $ u Pc/Pc/...Pc ST		DECTABSR
		 *	  Pc are column numbers (from 1) where tab stops occur. Note the
		 *	  separator "/" occurs in a real VT320 but should have been ";".
		 */
		str = m_RetChar[RC_DCS];
		str += _T("2$u");
		wrk.Empty();
		i = (IsOptEnable(TO_ANSITSM) ? (m_CurY + 1) : 0);
		for ( n = 0 ; n < m_Cols ; n++ ) {
			if ( (m_TabMap[i][n / 8 + 1] & (0x80 >> (n % 8))) != 0 ) {
				if ( !wrk.IsEmpty() )
					str += _T("/");
				wrk.Format(_T("%d"), n + 1);
				str += wrk;
			}
		}
		str += m_RetChar[RC_ST];
		UNGETSTR(_T("%s"), str);
		break;
	}

	fc_POP(ch);
}
void CTextRam::fc_DECFRA(int ch)
{
	// CSI $x	DECFRA Fill Rectangular Area
	int n, x, y;
	CVram *vp;
	SetAnsiParaArea(1);
	n = GetAnsiPara(0, 0, 0);
	for ( y = m_AnsiPara[1] ; y <= m_AnsiPara[3] ; y++ ) {
		for ( x = m_AnsiPara[2] ; x <= m_AnsiPara[4] ; x++ ) {
			vp = GETVRAM(x, y);
			*vp = (DWORD)n;
			vp->pr.at = m_AttNow.at;
			vp->pr.fc = m_AttNow.fc;
			vp->pr.bc = m_AttNow.bc;
			vp->pr.em = m_AttNow.em;
			vp->pr.md = m_BankTab[m_KanjiMode][(n & 0x80) == 0 ? m_BankGL : m_BankGR];
			vp->pr.cm = CM_ASCII;
		}
	}
	DISPVRAM(m_AnsiPara[2], m_AnsiPara[1], m_AnsiPara[4] - m_AnsiPara[2] + 1, m_AnsiPara[3] - m_AnsiPara[1] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECERA(int ch)
{
	// CSI $z	DECERA Erase Rectangular Area
	SetAnsiParaArea(0);
	ERABOX(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] + 1, m_AnsiPara[2] + 1, ERM_SAVEDM);
	fc_POP(ch);
}
void CTextRam::fc_DECSERA(int ch)
{
	// CSI ${	DECSERA Selective Erase Rectangular Area
	SetAnsiParaArea(0);
	ERABOX(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] + 1, m_AnsiPara[2] + 1, ERM_DECPRO | ERM_SAVEDM);
	fc_POP(ch);
}
void CTextRam::fc_DECSCPP(int ch)
{
	// CSI $|	DECSCPP Set Ps columns per page

	if ( IsOptEnable(TO_RLFONT) ) {
		InitScreen(GetAnsiPara(0, m_Cols, 4), m_Lines);
	} else {
		if ( GetAnsiPara(0, 0, 0) > m_DefCols[0] )
			EnableOption(TO_DECCOLM);
		else
			DisableOption(TO_DECCOLM);
		InitScreen(m_DefCols[IsOptValue(TO_DECCOLM, 1)], m_Lines);
	}

	fc_POP(ch);
}
void CTextRam::fc_DECSASD(int ch)
{
	// CSI $}	DECSASD Select Active Status Display
	m_StsFlag = GetAnsiPara(0, 0, 0) == 1 ? TRUE : FALSE;
	m_StsPara.Empty();
	fc_POP(ch);
}
void CTextRam::fc_DECSSDT(int ch)
{
	//	CSI $~	DECSSDT Set Status Display (Line) Type
	m_StsMode = GetAnsiPara(0, 0, 0);
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI SP ...
//
void CTextRam::fc_SL(int ch)
{
	// CSI (' ' << 8) | '@'		SL Scroll left
	int n;

	for ( n = GetAnsiPara(0, 1, 1, m_Cols) ; n > 0 ; n-- )
		LEFTSCROLL();
	fc_POP(ch);
}
void CTextRam::fc_SR(int ch)
{
	// CSI (' ' << 8) | 'A'		SR Scroll Right
	int n;

	for ( n = GetAnsiPara(0, 1, 1, m_Cols) ; n > 0 ; n-- )
		RIGHTSCROLL();
	fc_POP(ch);
}
void CTextRam::fc_FNT(int ch)
{
	// CSI (' ' << 8) | 'D'		FNT Font selection
	if ( (m_AttNow.ft = GetAnsiPara(0, 0, 0)) > 10 )
		m_AttNow.ft = 10;
	fc_POP(ch);
}
void CTextRam::fc_PPA(int ch)
{
	// CSI (' ' << 8) | 'P'		PPA Page Position Absolute
	SETPAGE(GetAnsiPara(0, 1, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_PPR(int ch)
{
	// CSI (' ' << 8) | 'Q'		PPR Page Position Relative
	SETPAGE(m_Page + GetAnsiPara(0, 1, 0));
	fc_POP(ch);
}
void CTextRam::fc_PPB(int ch)
{
	// CSI (' ' << 8) | 'R'		PPB Page Position Backwards
	SETPAGE(m_Page - GetAnsiPara(0, 1, 0));
	fc_POP(ch);
}
void CTextRam::fc_DECSCUSR(int ch)
{
	// CSI (' ' << 8) | 'q'		DECSCUSR Set Cursor Style

	// Where ps can be 0, 1, 2, 3, 4 meaning Blinking Block, Blinking Block, Steady Block, Blink Underline, Steady Underline, respectively.

	// case 0:		// meaning Blinking Block
	// case 1:		// Blinking Block
	// case 2:		// Steady Block
	// case 3:		// Blink Underline
	// case 4:		// Steady Underline

	m_TypeCaret = GetAnsiPara(0, 0, 0);
	m_pDocument->UpdateAllViews(NULL, UPDATE_TYPECARET, NULL);

	fc_POP(ch);
}
void CTextRam::fc_DECTME(int ch)
{
	// CSI Ps SP ~				DECTME Terminal Mode Emulation
	//	Ps Terminal Mode

	switch(GetAnsiPara(0, 0, 0)) {
	case 3:		// VT52
		DisableOption(TO_DECANM);
		break;

	case 0:
	case 1:		// VT520 (VT Level 5)
	case 2:		// VT100
	case 4:		// VT PCTerm
	case 5:		// WYSE 60/160
	case 6:		// WYSE PCTerm
	case 7:		// WYSE 50/50+
	case 8:		// WYSE 150/120
	case 9:		// TVI 950
	case 10:	// TVI 925
	case 11:	// TVI 910+
	case 12:	// ADDS A2
	case 13:	// SCO Console
	case 14:	// WYSE 325
	default:
		EnableOption(TO_DECANM);
		break;
	}

	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI Ext...

void CTextRam::fc_CSI_ETC(int ch)
{
	int n;
	int d = (m_BackChar | ch) & 0x7F7F7F7F;

	if ( BinaryFind((void *)&d, m_CsiExt.GetData(), sizeof(CSIEXTTAB), m_CsiExt.GetSize(), ExtTabCodeCmp, &n) ) 
		(this->*m_CsiExt[n].proc)(ch);
	else
		fc_POP(ch);
}
void CTextRam::fc_DECRQMH(int ch)
{
	// CSI ('?' << 16) | ('$' << 8) | 'p'	DECRQMH Request Mode (DEC) Host to Terminal
	int i, f = 0;

	i = GetAnsiPara(0, 0, 0);

	if ( i > 0 && i < 200 )				// 1-199				DEC Terminal Option
		f = IsOptEnable(i) ? 1 : 2;
	else if ( i >= 1000 && i < 1080 )	// 1000-1079(300-379)	XTerm Option
		f = IsOptEnable(i - 700) ? 1 : 2;
	if ( i >= 2000 && i < 2020 )		// 2000-2019(380-399)	XTerm Option 2
		f = IsOptEnable(i - 1620) ? 1 : 2;
	else if ( i >= 8400 && i < 8512 )	// 8400-8511(400-511)	RLogin Option
		f = IsOptEnable(i - 8000) ? 1 : 2;

	else if ( i == 7727 )				// 7727 - Application Escape mode を有効にする。				Application Escape mode を無効にする。  
		f = IsOptEnable(TO_RLCKMESC) ? 1 : 2;
	else if ( i == 7786 )				// 7786 - マウスホイール - カーソルキー変換を有効にする。		マウスホイール - カーソルキー変換を無効にする。 
		f = IsOptEnable(TO_RLMSWAPE) ? 1 : 2;
	else if ( i == 8840 )				// 8840 - TNAMB Aタイプをダブル幅の文字にする					シングル幅にする
		f = IsOptEnable(TO_RLUNIAWH) ? 2 : 1;

	UNGETSTR(_T("%s?%d;%d$y"), m_RetChar[RC_CSI], i, f);
	fc_POP(ch);
}

void CTextRam::fc_DECSTR(int ch)
{
	// CSI ('!' << 8) | 'p'		DECSTR Soft terminal reset

	RESET();
	fc_POP(ch);
}

void CTextRam::fc_DECSCL(int ch)
{
	// CSI Ps1 ; Ps2 "p			DECSCL Set compatibility level

	//CSI 6 1 " p -> Level 1 (VT100) compatibility
	//CSI 6 2 " p -> Level 2 (VT200) compatibility, 8-bit controls
	//CSI 6 2 ; 0 " p -> ditto
	//CSI 6 2 ; 1 " p -> ditto, 7-bit controls
	//CSI 6 2 ; 2 " p -> ditto, 8-bit controls
	//Ps1 = 63 selects Level 3 (VT300)
	//Ps1 = 64 selects Level 4 (VT400)

	int n;

	RESET();

	if ( (n = GetAnsiPara(0, 0, 0)) == 61 )
		m_VtLevel = n;
	else if ( n >= 62 && n <= 65 ) {
		m_VtLevel = n;
		SetRetChar(GetAnsiPara(1, 0, 0) == 1 ? FALSE : TRUE);
	}

	fc_POP(ch);
}
void CTextRam::fc_DECSCA(int ch)
{
	// CSI ('"' << 8) | 'q'		DECSCA Select character attributes

	if ( GetAnsiPara(0, 0, 0) == 1 )
		m_AttNow.em |= EM_DECPROTECT;
	else
		m_AttNow.em &= ~EM_DECPROTECT;

	fc_POP(ch);
}
void CTextRam::fc_DECRQDE(int ch)
{
	// CSI ('"' << 8) | 'v'		DECRQDE Request device extent
	// CSI Ph ; Pw ; Pml ; Pmt ; Pmp " w

	UNGETSTR(_T("%s%d;%d;%d;%d;%d\"w"), m_RetChar[RC_CSI], m_Lines, m_Cols, m_LeftX + 1, m_TopY + 1, m_Page + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECRQUPSS(int ch)
{
	// CSI ('&' << 8) | 'u'		DECRQUPSS Request User-Preferred Supplemental Set
	// DCS 1 ! u A ST			The user-preferred supplemental set is ISO Latin-1 supplemental.

	UNGETSTR(_T("%s1!uA%s"), m_RetChar[RC_DCS], m_RetChar[RC_ST]);
	fc_POP(ch);
}

void CTextRam::fc_DECEFR(int ch)
{
	// CSI ('\'' << 8) | 'w'		DECEFR Enable filter rectangle

	if ( m_AnsiPara.GetSize() >= 4 ) {
		m_Loc_Rect.top    = GetAnsiPara(0, 1, 1) - 1;
		m_Loc_Rect.left   = GetAnsiPara(1, 1, 1) - 1;
		m_Loc_Rect.bottom = GetAnsiPara(2, 1, 1) - 1;
		m_Loc_Rect.right  = GetAnsiPara(3, 1, 1) - 1;
		m_Loc_Rect.NormalizeRect();
	} else
		m_Loc_Rect.SetRectEmpty();

	m_Loc_Mode |= LOC_MODE_FILTER;

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

	LocReport(MOS_LOCA_INIT, 0, 0, 0);

	fc_POP(ch);
}
void CTextRam::fc_DECSLE(int ch)
{
	// CSI ('\'' << 8) | '{'		DECSLE Select locator events
	int n;

	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( m_AnsiPara[n].IsOpt() )
			continue;
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
	LocReport(MOS_LOCA_INIT, 0, 0, 0);

	fc_POP(ch);
}
void CTextRam::fc_DECRQLP(int ch)
{
	// CSI ('\'' << 8) | '|'		DECRQLP Request locator position

	LocReport(MOS_LOCA_REQ, 0, 0, 0);
	fc_POP(ch);
}
void CTextRam::fc_DECIC(int ch)
{
	// CSI ('\'' << 8) | '}'		DECIC Insert Column(s)

	int n, i;

	if ( GetMargin(MARCHK_BOTH) ) {
		for ( n = GetAnsiPara(0, 1, 1, m_Cols) ; n > 0 ; n-- ) {
			for ( i = m_CurY ; m_CurY < m_Margin.bottom ; m_CurY++ )
				INSCHAR();
			m_CurY = i;
		}
	}

	fc_POP(ch);
}
void CTextRam::fc_DECDC(int ch)
{
	// CSI ('\'' << 8) | '~'		DECDC Delete Column(s)

	int n, i;

	if ( GetMargin(MARCHK_BOTH) ) {
		for ( n = GetAnsiPara(0, 1, 1, m_Cols) ; n > 0 ; n-- ) {
			for ( i = m_CurY ; m_CurY < m_Margin.bottom ; m_CurY++ )
				DELCHAR();
			m_CurY = i;
		}
	}

	fc_POP(ch);
}

void CTextRam::fc_DECSACE(int ch)
{
	// CSI ('*' << 8) | 'x'		DECSACE Select Attribute and Change Extent
	m_Exact = (GetAnsiPara(0, 1, 1) == 2 ? TRUE : FALSE);
	fc_POP(ch);
}
void CTextRam::fc_DECRQCRA(int ch)
{
	// CSI ('*' << 8) | 'y'		DECRQCRA Request Checksum of Rectangle Area

	// CSI Pid ; Pp ; Pt;Pl;Pb;Pr * y

	int x, y, page, sum = 0;
	CTextSave *pSave;
	CVram *vp;

	SetAnsiParaArea(2);

	if ( (page = GetAnsiPara(1, m_Page + 1, 1) - 1) < 0 )
		page = 0;
	else if ( page > 100 )
		page = 100;

	if ( page == m_Page ) {
		for ( y = m_AnsiPara[2] ; y <= m_AnsiPara[4] ; y++ ) {
			vp = GETVRAM(m_AnsiPara[3], y);
			for ( x = m_AnsiPara[3] ; x <= m_AnsiPara[5] ; x++, vp++ )
				sum += (DWORD)(*vp);
		}

	} else {
		pSave = GETPAGE(page);
		for ( y = m_AnsiPara[2] ; y <= m_AnsiPara[4] ; y++ ) {
			vp = pSave->m_VRam + m_AnsiPara[3] + pSave->m_Cols * y;
			for ( x = m_AnsiPara[3] ; x <= m_AnsiPara[5] ; x++, vp++ )
				sum += (DWORD)(*vp);
		}
	}

	// DECCKSR DCS Pn ! ~ Ps ST
	UNGETSTR(_T("%s%d!~%04x%s"), m_RetChar[RC_DCS], GetAnsiPara(0, 0, 0), sum & 0xFFFF, m_RetChar[RC_ST]);

	fc_POP(ch);
}
void CTextRam::fc_DECINVM(int ch)
{
	// CSI ('*' << 8) | 'z'		DECINVM Invoke Macro

	int n;
	LPBYTE p;
	int Pid = GetAnsiPara(0, 0, 0);

	fc_POP(ch);		// XXXXXXXXX fc_Callの必ず前に

	if ( Pid < 64 && (m_MacroExecFlag[Pid / 32] & (1 << (Pid % 32))) == 0 ) {
		m_MacroExecFlag[Pid / 32] |= (1 << (Pid % 32));
		p = m_Macro[Pid].GetPtr();
		for ( n = 0 ; n < m_Macro[Pid].GetSize() ; n++ )
			fc_Call(*(p++));
		m_MacroExecFlag[Pid / 32] &= ~(1 << (Pid % 32));
	}
}
void CTextRam::fc_DECSR(int ch)
{
	// CSI ('+'  << 8) | 'p'	DECSR Secure Reset

	RESET();
	UNGETSTR(_T("%s%d*q"), m_RetChar[RC_CSI], GetAnsiPara(0, 0, 0));		// CSI * q	DECSRC Secure Reset Confirmation
	fc_POP(ch);
}
void CTextRam::fc_DECPQPLFM(int ch)
{
	// CSI ('+'  << 8) | 'x'	DECRQPKFM Request Program Key Free Memory

	UNGETSTR(_T("%s%d;%d+y"), m_RetChar[RC_CSI], 768, 768);		// CSI + y	DECPKFMR Program Key Free Memory Report
	fc_POP(ch);
}

void CTextRam::fc_DECAC(int ch)
{
	// CSI Ps1 Ps2 Ps3 , |		DECAC Assign Color

	// Ps1 Item
	//	1 Normal text
	//	2 Window frame
	// Ps2 Foreground
	//	0-15 color index
	// Ps3 Background
	//	0-15 color index

	if ( GetAnsiPara(0, 0, 0) == 1 ) {
		m_AttNow.fc = GetAnsiPara(1, m_AttNow.fc, 0, 255);
		m_AttNow.bc = GetAnsiPara(2, m_AttNow.bc, 0, 255);
		if ( IsOptEnable(TO_DECECM) == 0 ) {
			m_AttSpc.fc = m_AttNow.fc;
			m_AttSpc.bc = m_AttNow.bc;
		}
	}

	fc_POP(ch);
}
void CTextRam::fc_DECTID(int ch)
{
	// CSI (',' << 8) | 'q'		DECTID Select Terminal ID
	m_TermId = GetAnsiPara(0, 10, 0);
	fc_POP(ch);
}
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

	m_AttNow.fc = GetAnsiPara(1, m_AttNow.fc, 0, 255);
	m_AttNow.bc = GetAnsiPara(2, m_AttNow.bc, 0, 255);
	if ( IsOptEnable(TO_DECECM) == 0 ) {
		m_AttSpc.fc = m_AttNow.fc;
		m_AttSpc.bc = m_AttNow.bc;
	}
	if ( IsOptEnable(TO_RLGCWA) != 0 )
		m_AttSpc.at = m_AttNow.at;

	fc_POP(ch);
}
void CTextRam::fc_DECPS(int ch)
{
	// CSI Pv ; Pd ; Pn , ~		DECPS Play Sound
	/*
		Pv	Sound volume
			0 = off
			1...3 (low)
			4...7 (high)
		Pd	Duration of note in 1/32 of a second
		Pn	Note
			Pn	Note	(Hz)	Pn	Note	 (Hz)
			1	C5				14	C#6		1047
			2	C#5				15	D6
			3	D5				16	D#6
			4	D#5		632		17	E6
			5	E5				18	F6
			6	F5				19	F#6
			7	F#5				20	G6
			8	G5				21	G#6
			9	G#5		847		22	A6
			10	A5				23	A#6
			11	A#5		944		24	B6
			12	B5				25	C7
			13	C6
	*/
	int n;
	DWORD msg = 0x00000090;	// Note On

	if ( (n = GetAnsiPara(0, 0, 0) * 127 / 7) > 127 )
		n = 127;
	msg |= (n << 16);	// velocity

	if ( (n = GetAnsiPara(2, 0, 0) + 59) > 127 )
		n = 127;
	msg |= (n << 8);	// note

	((CMainFrame *)AfxGetMainWnd())->SetMidiEvent(0, msg);

	n = GetAnsiPara(1, 0, 0) * 1000 / 32;	// time
	msg &= 0x0000FFFF;	// velocity = 0

	((CMainFrame *)AfxGetMainWnd())->SetMidiEvent(n, msg);

	fc_POP(ch);
}

void CTextRam::fc_DECSTGLT(int ch)
{
	// CSI (')' << 8 | '{'		DECSTGLT Select Color Look-Up Table
	int n, c;
	switch(GetAnsiPara(0, 0, 0)) {
	case 0:		// Mono Monochrome or gray level color map
		for ( n = 0 ; n < 16 ; n++ ) {
			c = 255 * n / 15;
			m_ColTab[n] = RGB(c, c, c);
		}
		break;
	case 1:		// Alternate color Text attributes (bold, blink, underline, reverse)
	case 2:		// Alternate color -
	case 3:		// ANSI SGR color ANSI SGR color parameters
		memcpy(m_ColTab, m_DefColTab, sizeof(m_ColTab));
		break;
	}
	DISPVRAM(0, 0, m_Cols, m_Lines);
	fc_POP(ch);
}

void CTextRam::fc_DA2(int ch)
{
	// CSI ('>' << 16) | 'c'	DA2 Secondary Device Attributes

	if ( m_AnsiPara.GetSize() == 0 || m_AnsiPara[0].IsEmpty() || m_AnsiPara[0] == 0 )
		UNGETSTR(_T("%s>65;100;1c"), m_RetChar[RC_CSI]);
	fc_POP(ch);
}

void CTextRam::fc_DA3(int ch)
{
	// CSI ('=' << 16) | 'c'	DA3 Tertiary Device Attributes

	if ( m_AnsiPara.GetSize() == 0 || m_AnsiPara[0].IsEmpty() || m_AnsiPara[0] == 0 )
		UNGETSTR(_T("%s!|%04x%s"), m_RetChar[RC_DCS], m_UnitId, m_RetChar[RC_ST]);
	fc_POP(ch);
}
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

void CTextRam::fc_TTIMESV(int ch)
{
	//	CSI ('<' << 16) | 's',		TTIMESV IME の開閉状態を保存する。
	m_AnsiPara.RemoveAll();
	m_AnsiPara.Add(TO_IMECTRL + 8000);
	fc_DECSRET('s');
}
void CTextRam::fc_TTIMEST(int ch)
{
	//	CSI ('<' << 16) | 't',		TTIMEST IME の開閉状態を設定する。

	int n = GetAnsiPara(0, 0, 0);
	m_AnsiPara.RemoveAll();
	m_AnsiPara.Add(TO_IMECTRL + 8000);
	fc_DECSRET(n == 0 ? 'l' : 'h');
}
void CTextRam::fc_TTIMERS(int ch)
{
	//	CSI ('<' << 16) | 'r',		TTIMERS IME の開閉状態を復元する。
	m_AnsiPara.RemoveAll();
	m_AnsiPara.Add(TO_IMECTRL + 8000);
	fc_DECSRET('r');
}
void CTextRam::fc_XTRMTT(int ch)
{
	// CSI { ('>' << 16) | 'T',		xterm CASE_RM_TITLE
	//    Ps = 0  -> Do not set window/icon labels using hexadecimal.
	//    Ps = 1  -> Do not query window/icon labels using hexadecimal.
	//    Ps = 2  -> Do not set window/icon labels using UTF-8.
	//    Ps = 3  -> Do not query window/icon labels using UTF-8.

	for ( int n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( !m_AnsiPara[n].IsEmpty() )
			m_XtOptFlag &= ~(1 << m_AnsiPara[n]);
	}
	fc_POP(ch);
}
void CTextRam::fc_XTMDKEY(int ch)
{
	// CSI > Ps; Ps m
	//	  Set or reset resource-values used by xterm to decide whether
	//	  to construct escape sequences holding information about the
	//	  modifiers pressed with a given key.  The first parameter iden-
	//	  tifies the resource to set/reset.  The second parameter is the
	//	  value to assign to the resource.  If the second parameter is
	//	  omitted, the resource is reset to its initial value.
	//	    Ps = 0  -> modifyKeyboard.			default 0
	//	    Ps = 1  -> modifyCursorKeys.		default 2
	//	    Ps = 2  -> modifyFunctionKeys.		default 2
	//	    Ps = 4  -> modifyOtherKeys.			default 0
	//	    Ps = 5  -> modifyStringKeys.		default 0
	//	  If no parameters are given, all resources are reset to their
	//	  initial values.

	if ( m_AnsiPara.GetSize() > 0 && !m_AnsiPara[0].IsEmpty() ) {
		int n = m_AnsiPara[0];
		int f = GetAnsiPara(1, m_DefModKey[n], 0, 5);
		if ( n < MODKEY_MAX )
			m_ModKey[n] = f;
	} else {
		for ( int n = 1 ; n < MODKEY_MAX ; n++ )
			m_ModKey[n] = m_DefModKey[n];
	}

	fc_POP(ch);
}
void CTextRam::fc_XTMDKYD(int ch)
{
	//CSI > Ps n
	//	  Disable modifiers which may be enabled via the CSI > Ps; Ps m
	//	  sequence.  This corresponds to a resource value of "-1", which
	//	  cannot be set with the other sequence.  The parameter identi-
	//	  fies the resource to be disabled:
	//	    Ps = 0  -> modifyKeyboard.
	//	    Ps = 1  -> modifyCursorKeys.
	//	    Ps = 2  -> modifyFunctionKeys.
	//	    Ps = 4  -> modifyOtherKeys.
	//	  If the parameter is omitted, modifyFunctionKeys is disabled.
	//	  When modifyFunctionKeys is disabled, xterm uses the modifier
	//	  keys to make an extended sequence of functions rather than
	//	  adding a parameter to each function key to denote the modifiers.

	if ( m_AnsiPara.GetSize() > 0 && !m_AnsiPara[0].IsEmpty() ) {
		if ( m_AnsiPara[0] < MODKEY_MAX )
			m_ModKey[m_AnsiPara[0]] = (-1);
	} else
		m_ModKey[MODKEY_FUNC] = (-1);

	fc_POP(ch);
}
void CTextRam::fc_XTHDPT(int ch)
{
	//CSI > Ps p
	//	  Set resource value pointerMode.  This is used by xterm to
	//	  decide whether to hide the pointer cursor as the user types.
	//	  Valid values for the parameter:
	//	    Ps = 0  -> never hide the pointer.
	//	    Ps = 1  -> hide if the mouse tracking mode is not enabled.
	//	    Ps = 2  -> always hide the pointer, except when leaving the window.
	//	    Ps = 3  -> always hide the pointer, even if leaving/entering the window.
	//    If no parameter is given, xterm uses the default,
	//	  which is 1 .

	fc_POP(ch);
}
void CTextRam::fc_XTSMTT(int ch)
{
	// CSI ('>' << 16) | 't',		xterm CASE_SM_TITLE
	//    Ps = 0  -> Set window/icon labels using hexadecimal.
	//    Ps = 1  -> Query window/icon labels using hexadecimal.
	//    Ps = 2  -> Set window/icon labels using UTF-8.
	//    Ps = 3  -> Query window/icon labels using UTF-8.  (See discussion of "Title Modes")

	for ( int n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( !m_AnsiPara[n].IsEmpty() )
			m_XtOptFlag |= (1 << m_AnsiPara[n]);
	}
	fc_POP(ch);
}
void CTextRam::fc_RLIMGCP(int ch)
{
	// CSI ('<' << 16) | ('!' << 8) | 'c'		RLIMGCP

	CRLoginView *pView;

	if ( m_pDocument != NULL && (pView = (CRLoginView *)m_pDocument->GetAciveView()) != NULL )
		pView->CreateGrapImage(GetAnsiPara(0, 0, 0, 4));

	fc_POP(ch);
}
