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

static const CTextRam::PROCTAB fc_CtrlTab[] = {
	{ 0x05,		0,			&CTextRam::fc_ENQ		},
	{ 0x07,		0,			&CTextRam::fc_BELL		},
	{ 0x08,		0,			&CTextRam::fc_BS		},
	{ 0x09,		0,			&CTextRam::fc_HT		},
	{ 0x0A,		0,			&CTextRam::fc_LF		},
	{ 0x0B,		0,			&CTextRam::fc_VT		},
	{ 0x0C,		0,			&CTextRam::fc_FF		},
	{ 0x0D,		0,			&CTextRam::fc_CR		},
	{ 0x0E,		0,			&CTextRam::fc_SO		},
	{ 0x0F,		0,			&CTextRam::fc_SI		},
	{ 0x10,		0,			&CTextRam::fc_DC0		},
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
	{ 0x00,		0x20,		&CTextRam::fc_RETRY		},
	{ 0x21,		0x7E,		&CTextRam::fc_TEXT2		},
	{ 0x7F,		0,			&CTextRam::fc_RETRY		},
	{ 0x80,		0xA0,		&CTextRam::fc_TEXT3		},
	{ 0xA1,		0xFE,		&CTextRam::fc_TEXT2		},
	{ 0xFF,		0,			&CTextRam::fc_TEXT3		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_96x96Tab[] = {
	{ 0x00,		0x1F,		&CTextRam::fc_RETRY		},
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
	{ 0x00,		0x3F,		&CTextRam::fc_RETRY		},
	{ 0x40,		0x7E,		&CTextRam::fc_SJIS2		},
	{ 0x7F,		0,			&CTextRam::fc_RETRY		},
	{ 0x80,		0xFC,		&CTextRam::fc_SJIS2		},
	{ 0xFD,		0xFF,		&CTextRam::fc_SJIS3		},
	{ 0,		0,			NULL } };

static const CTextRam::PROCTAB fc_Utf8Tab[] = {
	{ 0x20,		0x7E,		&CTextRam::fc_TEXT		},
	{ 0xA0,		0xBF,		&CTextRam::fc_UTF87		},
	{ 0xC0,		0xDF,		&CTextRam::fc_UTF81		},
	{ 0xE0,		0xEF,		&CTextRam::fc_UTF82		},
	{ 0xF0,		0xF7,		&CTextRam::fc_UTF83		},
	{ 0xFE,		0xFF,		&CTextRam::fc_UTF84		},
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Utf82Tab[] = {
	{ 0x00,		0x7F,		&CTextRam::fc_RETRY		},
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
	{ '=',		0,			&CTextRam::fc_POP		},	//											VT52 Enter alternate keypad mode.
	{ '>',		0,			&CTextRam::fc_POP		},	//											VT52 Exit alternate keypad mode.
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
	{ 'P',		0,			&CTextRam::fc_OSC		},	// DCS Device Control String
	{ 'Q',		0,			&CTextRam::fc_POP		},	// PU1 Private use one
	{ 'R',		0,			&CTextRam::fc_POP		},	// PU2 Private use two
	{ 'S',		0,			&CTextRam::fc_STS		},	// STS Set transmit state
	{ 'T',		0,			&CTextRam::fc_CCH		},	// CCH Cancel character
	{ 'U',		0,			&CTextRam::fc_MW		},	// MW Message waiting
	{ 'V',		0,			&CTextRam::fc_SPA		},	// SPA Start of guarded area				VT52 Print the line with the cursor.
	{ 'W',		0,			&CTextRam::fc_EPA		},	// EPA End of guarded area					VT52 Enter printer controller mode.
//	{ 'X',		0,			&CTextRam::fc_OSC		},	// SOS Start of String
	{ 'Y',		0,			&CTextRam::fc_V5MCP		},	//											VT52 Move cursor to column Pn.
	{ 'Z',		0,			&CTextRam::fc_SCI		},	// SCI Single character introducer			VT52 Identify (host to terminal).
	{ '[',		0,			&CTextRam::fc_CSI		},	// CSI Control sequence introducer
	{ '\\',		0,			&CTextRam::fc_POP		},	// ST String terminator
	{ ']',		0,			&CTextRam::fc_OSC		},	// OSC Operating System Command
//	{ '^',		0,			&CTextRam::fc_OSC		},	// PM Privacy message
//	{ '_',		0,			&CTextRam::fc_OSC		},	// APC Application Program Command
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
	{ 0x20,		0x2F,		&CTextRam::fc_CSI_EXT	},
	{ 0x30,		0x39,		&CTextRam::fc_CSI_DIGIT	},
	{ 0x3B,		0,			&CTextRam::fc_CSI_SEPA	},
	{ 0x3C,		0x3F,		&CTextRam::fc_CSI_EXT	},
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
	{ 'S',		0,			&CTextRam::fc_SU		},	// SU Scroll Up
	{ 'T',		0,			&CTextRam::fc_SD		},	// SD Scroll Down
	{ 'U',		0,			&CTextRam::fc_NP		},	// NP Next Page
	{ 'V',		0,			&CTextRam::fc_PP		},	// PP Preceding Page
	{ 'W',		0,			&CTextRam::fc_CTC		},	// CTC Cursor tabulation control
	{ 'X',		0,			&CTextRam::fc_ECH		},	// ECH Erase character
	{ 'Y',		0,			&CTextRam::fc_CVT		},	// CVT Cursor line tabulation
	{ 'Z',		0,			&CTextRam::fc_CBT		},	// CBT Move the active position n tabs backward.
	{ ']',		0,			&CTextRam::fc_LINUX		},	// LINUX console
	{ '^',		0,			&CTextRam::fc_SIMD		},	// SIMD Select implicit movement direction
	{ '`',		0,			&CTextRam::fc_HPA		},	// HPA Horizontal Position Absolute
	{ 'a',		0,			&CTextRam::fc_HPR		},	// HPR Horizontal Position Relative
	{ 'b',		0,			&CTextRam::fc_REP		},	// REP Repeat
	{ 'c',		0,			&CTextRam::fc_DA1		},	// DA1 Primary Device Attributes
	{ 'd',		0,			&CTextRam::fc_VPA		},	// VPA Vertical Line Position Absolute
	{ 'e',		0,			&CTextRam::fc_VPR		},	// VPR Vertical Position Relative
	{ 'f',		0,			&CTextRam::fc_HVP		},	// HVP Horizontal and Vertical Position
	{ 'g',		0,			&CTextRam::fc_TBC		},	// TBC Tab Clear
	{ 'h',		0,			&CTextRam::fc_SRM		},	// SM Mode Set
	{ 'i',		0,			&CTextRam::fc_MC		},	// MC Media copy
	{ 'j',		0,			&CTextRam::fc_HPB		},	// HPB Character position backward
	{ 'k',		0,			&CTextRam::fc_VPB		},	// VPB Line position backward
	{ 'l',		0,			&CTextRam::fc_SRM		},	// RM Mode ReSet
	{ 'm',		0,			&CTextRam::fc_SGR		},	// SGR Select Graphic Rendition
	{ 'n',		0,			&CTextRam::fc_DSR		},	// DSR Device status report
	{ 'o',		0,			&CTextRam::fc_DAQ		},	// DAQ Define area qualification
	{ 'p',		0,			&CTextRam::fc_DECBFAT	},	// DECBFAT Begin field attribute : DEC private
	{ 'q',		0,			&CTextRam::fc_DECLL		},	// DECLL Load LEDs
	{ 'r',		0,			&CTextRam::fc_DECSTBM	},	// DECSTBM Set Top and Bottom Margins
	{ 's',		0,			&CTextRam::fc_DECSC		},	// DECSC Save Cursol Pos
	{ 't',		0,			&CTextRam::fc_XTWOP		},	// XTWOP XTERM WINOPS
	{ 'u',		0,			&CTextRam::fc_DECRC		},	// DECRC Load Cursol Pos
	{ 'v',		0,			&CTextRam::fc_ORGSCD	},	// ORGSCD Set Cursol Display
	{ 'x',		0,			&CTextRam::fc_REQTPARM	},	// DECREQTPARM Request terminal parameters
	{ 'y',		0,			&CTextRam::fc_DECTST	},	// DECTST Invoke confidence test
	{ 'z',		0,			&CTextRam::fc_DECVERP	},	// DECVERP Set vertical pitch
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext1Tab[] = {
	{ 'J',		0,			&CTextRam::fc_DECSED	},	// DECSED Selective Erase in Display
	{ 'K',		0,			&CTextRam::fc_DECSEL	},	// DECSEL Selective Erase in Line
	{ 'W',		0,			&CTextRam::fc_DECST8C	},	// DECST8C Set Tab at every 8 columns
	{ 'h',		0,			&CTextRam::fc_DECSRET	},	// DECSET
	{ 'l',		0,			&CTextRam::fc_DECSRET	},	// DECRST
	{ 'n',		0,			&CTextRam::fc_DECDSR	},	// DECDSR Device status report
	{ 'r',		0,			&CTextRam::fc_DECSRET	},	// XTERM_RESTORE
	{ 's',		0,			&CTextRam::fc_DECSRET	},	// XTERM_SAVE
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext2Tab[] = {
	{ 'p',		0,			&CTextRam::fc_DECRQM	},	// DECRQM Request mode settings
	{ 'r',		0,			&CTextRam::fc_DECCARA	},	// DECCARA Change Attribute in Rectangle
	{ 't',		0,			&CTextRam::fc_DECRARA	},	// DECRARA Reverse Attribute in Rectangle
	{ 'v',		0,			&CTextRam::fc_DECCRA	},	// DECCRA Copy Rectangular Area
	{ 'x',		0,			&CTextRam::fc_DECFRA	},	// DECFRA Fill Rectangular Area
	{ 'z',		0,			&CTextRam::fc_DECERA	},	// DECERA Erase Rectangular Area
	{ '{',		0,			&CTextRam::fc_DECSERA	},	// DECSERA Selective Erase Rectangular Area
	{ 0,		0,			NULL } };
static const CTextRam::PROCTAB fc_Ext3Tab[] = {
	{ 0x40,		0x7F,		&CTextRam::fc_CSI_ETC	},
	{ 0,		0,			NULL } };

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
	{ 0x08,		0,			&CTextRam::fc_TEK_CUR	},	// LEFT
	{ 0x09,		0,			&CTextRam::fc_TEK_CUR	},	// RIGHT
	{ 0x0A,		0,			&CTextRam::fc_TEK_CUR	},	// DOWN
	{ 0x0B,		0,			&CTextRam::fc_TEK_CUR	},	// UP
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
void CTextRam::fc_Init(int mode)
{
	if ( !fc_Init_Flag ) {
		fc_Init_Flag = TRUE;

		for ( int i = 0 ; i < STAGE_MAX ; i++ ) {
			for ( int c = 0; c < 256 ; c++ )
				fc_Proc[i][c] = &CTextRam::fc_IGNORE;
		}

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
	}

	m_RetSync = FALSE;
	m_OscFlag = FALSE;
	fc_StPos = 0;

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
	fc_Stage = stage;
	fc_Func = fc_Proc[stage];
}
inline void CTextRam::fc_Push(int stage)
{
	ASSERT(fc_StPos < 16);
	fc_Stack[fc_StPos++] = fc_Stage;
	fc_Case(stage);
}

//////////////////////////////////////////////////////////////////////
// fc Procs...

void CTextRam::fc_IGNORE(int ch)
{
	if ( ch < 0x20 ) {
		CallReciveChar(ch);
		m_LastChar = ch;
	}
}
void CTextRam::fc_POP(int ch)
{
	if ( ch < 0x20 ) {
		CallReciveChar(ch);
		m_LastChar = ch;
	}
	if ( fc_StPos > 0 )
		fc_Case(fc_Stack[--fc_StPos]);
}
void CTextRam::fc_RETRY(int ch)
{
	if ( fc_StPos > 0 ) {
		fc_Case(fc_Stack[--fc_StPos]);
		fc_Call(ch);
	}
}
void CTextRam::fc_SESC(int ch)
{
	CallReciveChar(0x1B);
	m_LastChar = 0x1B;
	ch &= 0x1F;
	ch += '@';
	fc_Push(STAGE_ESC);
	fc_Call(ch);
}

//////////////////////////////////////////////////////////////////////
// fc Print

void CTextRam::fc_TEXT(int ch)
{
	if ( m_BankSG >= 0 ) {
		m_BankNow = m_BankTab[m_KanjiMode][m_BankSG];
		m_BankSG = (-1);
	} else
		m_BankNow = m_BankTab[m_KanjiMode][(ch & 0x80) == 0 ? m_BankGL : m_BankGR];

	m_SaveChar = m_BackChar = ch;

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
	if ( IsOptEnable(TO_RLKANAUTO) && !m_OscFlag ) {
		if ( m_BackChar >= 0xC0 && m_BackChar <= 0xF7 && ch >= 0x80 && ch <= 0xBF )
			SetKanjiMode(UTF8_SET);
		else if ( issjis1(m_BackChar) && issjis2(ch) )
			SetKanjiMode(SJIS_SET);
	}
}
void CTextRam::fc_SJIS1(int ch)
{
	m_BackChar = ch;
	m_BankNow  = m_BankTab[m_KanjiMode][2];
	fc_Push(STAGE_SJIS2);
}
void CTextRam::fc_SJIS2(int ch)
{
	int n;
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
	if ( IsOptEnable(TO_RLKANAUTO) && !m_OscFlag ) {
		if ( m_BackChar >= 0xC0 && m_BackChar <= 0xF7 && ch >= 0x80 && ch <= 0xBF )
			SetKanjiMode(UTF8_SET);
		else if ( m_BackChar >= 0xA0 && m_BackChar <= 0xFF && ch >= 0xA0 && ch <= 0xFF )
			SetKanjiMode(EUC_SET);
	}
}
void CTextRam::fc_UTF81(int ch)
{
	// 110x xxxx
	m_SaveChar = ch;
	m_BackChar = (ch & 0x1F) << 6;
	m_CodeLen = 1;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF82(int ch)
{
	// 1110 xxxx
	m_SaveChar = ch;
	m_BackChar = (ch & 0x0F) << 12;
	m_CodeLen = 2;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF83(int ch)
{
	// 1111 0xxx
	m_SaveChar = ch;
	m_BackChar = (ch & 0x07) << 18;
	m_CodeLen = 3;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF84(int ch)
{
	// 1111 111x BOM
	m_SaveChar = ch;
	m_BackChar = ch;
	m_CodeLen = 0;
	fc_Push(STAGE_UTF82);
}
void CTextRam::fc_UTF85(int ch)
{
	// 10xx xxxx
	int n;
	switch(m_CodeLen) {
	case 3:
		m_SaveChar = ch;
		m_BackChar |= (ch & 0x3F) << 12;
		m_CodeLen--;
		break;
	case 2:
		m_SaveChar = ch;
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
	if ( IsOptEnable(TO_RLKANAUTO) && !m_OscFlag ) {
		if ( issjis1(m_SaveChar) && issjis2(ch) )
			SetKanjiMode(SJIS_SET);
		else if ( m_SaveChar >= 0xA0 && m_SaveChar <= 0xFF && ch >= 0xA0 && ch <= 0xFF )
			SetKanjiMode(EUC_SET);
	}
}
void CTextRam::fc_UTF87(int ch)
{
	if ( IsOptEnable(TO_RLKANAUTO) && !m_OscFlag ) {
		if ( issjis1(m_SaveChar) && issjis2(ch) )
			SetKanjiMode(SJIS_SET);
		else if ( m_SaveChar >= 0xA0 && m_SaveChar <= 0xFF && ch >= 0xA0 && ch <= 0xFF )
			SetKanjiMode(EUC_SET);
	}
	m_SaveChar = ch;
}

//////////////////////////////////////////////////////////////////////
// fc CTRLs...

void CTextRam::fc_ENQ(int ch)
{
	if ( IsOptEnable(TO_RLBPLUS) )
		m_RetSync = TRUE;
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_BELL(int ch)
{
	BEEP();
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_BS(int ch)
{
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
	m_LastChar = ch;
}
void CTextRam::fc_HT(int ch)
{
	TABSET(TAB_COLSNEXT);
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_LF(int ch)
{
	switch(m_RecvCrLf) {
	case 0:		// CR+LF
		ONEINDEX();
		break;
	case 1:		// CR
		break;
	case 2:		// LF
		ONEINDEX();
		LOCATE(0, m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag )
			m_pDocument->OnDelayRecive(ch);
		break;
	case 3:		// CR|LF
		ONEINDEX();
		LOCATE(0, m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag )
			m_pDocument->OnDelayRecive(ch);
		break;
	}
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_VT(int ch)
{
	TABSET(TAB_LINENEXT);
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_FF(int ch)
{
	for ( int n = 0 ; n < m_Lines ; n++ )
		FORSCROLL(0, m_Lines);
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_CR(int ch)
{
	switch(m_RecvCrLf) {
	case 0:		// CR+LF
		LOCATE(0, m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag )
			m_pDocument->OnDelayRecive(ch);
		break;
	case 1:		// CR
		ONEINDEX();
		LOCATE(0, m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag )
			m_pDocument->OnDelayRecive(ch);
		break;
	case 2:		// LF
		break;
	case 3:		// CR|LF
		ONEINDEX();
		LOCATE(0, m_CurY);
		if ( IsOptEnable(TO_RLDELAY) && m_pDocument->m_DelayFlag )
			m_pDocument->OnDelayRecive(ch);
		break;
	}
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_SO(int ch)
{
	m_BankGL = 1;
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_SI(int ch)
{
	m_BankGL = 0;
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_DC0(int ch)
{
	if ( IsOptEnable(TO_RLBPLUS) )
		m_RetSync = TRUE;
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_CAN(int ch)
{
	if ( m_LastChar == '*' && IsOptEnable(TO_RLBPLUS) ) {
		m_LastChar = ch;
		m_RetSync = TRUE;
	}
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_ESC(int ch)
{
	fc_Push(STAGE_ESC);
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_A3CRT(int ch)
{
	// ADM-3 Cursole Right
	LOCATE(m_CurX + 1, m_CurY);
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_A3CLT(int ch)
{
	// ADM-3 Cursole Left
	LOCATE(m_CurX - 1, m_CurY);
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_A3CUP(int ch)
{
	// ADM-3 Cursole Up
	LOCATE(m_CurX, m_CurY - 1);
	CallReciveChar(ch);
	m_LastChar = ch;
}
void CTextRam::fc_A3CDW(int ch)
{
	// ADM-3 Cursole Down
	LOCATE(m_CurX, m_CurY + 1);
	CallReciveChar(ch);
	m_LastChar = ch;
}

//////////////////////////////////////////////////////////////////////
// fc ESC

void CTextRam::fc_DECHTS(int ch)
{
	// DECHTS Horizontal tab set
	TABSET(TAB_COLSSET);
	fc_POP(ch);
}
void CTextRam::fc_CCAHT(int ch)
{
	// DECCAHT Clear all horizontal tabs
	TABSET(TAB_COLSALLCLR);
	fc_POP(ch);
}
void CTextRam::fc_DECVTS(int ch)
{
	// DECVTS Vertical tab set
	TABSET(TAB_LINESET);
	fc_POP(ch);
}
void CTextRam::fc_CAVT(int ch)
{
	// DECCAVT Clear all vertical tabs
	TABSET(TAB_LINEALLCLR);
	fc_POP(ch);
}
void CTextRam::fc_BI(int ch)
{
	// DECBI Back Index
	if ( m_CurX == 0 )
		RIGHTSCROLL();
	else
		LOCATE(m_CurX - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_SC(int ch)
{
	// DECSC Save Cursor
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
	// DECRC Restore Cursor
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
	// DECFI Forward Index
	if ( (m_CurX + 1) >= m_Cols )
		LEFTSCROLL();
	else
		LOCATE(m_CurX + 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_V5CUP(int ch)
{
	// VT52 Cursor up.
	LOCATE(m_CurX, m_CurY - 1);
	fc_POP(ch);
}
void CTextRam::fc_BPH(int ch)
{
	// VT52 Cursor down								ANSI BPH Break permitted here
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(m_CurX, m_CurY + 1);
	fc_POP(ch);
}
void CTextRam::fc_NBH(int ch)
{
	// VT52 Cursor right							ANSI NBH No break here
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(m_CurX + 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_IND(int ch)
{
	// VT52 Cursor left								ANSI IND Index
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(m_CurX - 1, m_CurY);
	else
		ONEINDEX();
	fc_POP(ch);
}
void CTextRam::fc_NEL(int ch)
{
	//												ANSI NEL Next Line
	ONEINDEX();
	LOCATE(0, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_SSA(int ch)
{
	// VT52 Enter graphics mode.					ANSI SSA Start selected area
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) ) {
		m_BankTab[m_KanjiMode][0] = SET_94 | '0';
		m_BankTab[m_KanjiMode][1] = SET_94 | '0';
	}
	fc_POP(ch);
}
void CTextRam::fc_ESA(int ch)
{
	// VT52 Exit graphics mode.						ANSI ESA End selected area
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) ) {
		m_BankTab[m_KanjiMode][0] = SET_94 | 'B';
		m_BankTab[m_KanjiMode][1] = SET_94 | 'B';
	}
	fc_POP(ch);
}
void CTextRam::fc_HTS(int ch)
{
	// VT52 Cursor to home position.				ANSI HTS Character tabulation set
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		LOCATE(0, 0);
	else
		TABSET(TAB_COLSSET);
	fc_POP(ch);
}
void CTextRam::fc_HTJ(int ch)
{
	// VT52 Reverse line feed.						ANSI HTJ Character tabulation with justification
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		REVINDEX();
	else
		TABSET(TAB_COLSNEXT);
	fc_POP(ch);
}
void CTextRam::fc_VTS(int ch)
{
	// VT52 Erase from cursor to end of screen.		ANSI VTS Line tabulation set
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) ) {
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1, 1);
		ERABOX(0, m_CurY + 1, m_Cols, m_Lines, 1);
	} else
		TABSET(TAB_LINESET);
	fc_POP(ch);
}
void CTextRam::fc_PLD(int ch)
{
	// VT52 Erase from cursor to end of line.		ANSI PLD Partial line forward
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);
	else
		LOCATE(m_CurX, m_CurY + 1);
	fc_POP(ch);
}
void CTextRam::fc_PLU(int ch)
{
	//												ANSI PLU Partial line backward
	LOCATE(m_CurX, m_CurY - 1);
	fc_POP(ch);
}
void CTextRam::fc_RI(int ch)
{
	//												ANSI RI Reverse index
	REVINDEX();
	fc_POP(ch);
}
void CTextRam::fc_STS(int ch)
{
	//												ANSI STS Set transmit state
	fc_POP(ch);
}
void CTextRam::fc_CCH(int ch)
{
	//												ANSI CCH Cancel character
	fc_POP(ch);
}
void CTextRam::fc_MW(int ch)
{
	//												ANSI MW Message waiting
	fc_POP(ch);
}
void CTextRam::fc_SPA(int ch)
{
	// VT52 Print the line with the cursor.			ANSI SPA Start of guarded area
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		m_AttNow.em |= EM_ERM;
	fc_POP(ch);
}
void CTextRam::fc_EPA(int ch)
{
	// VT52 Enter printer controller mode.			ANSI EPA End of guarded area
	if ( !IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		m_AttNow.em &= ~EM_ERM;
	fc_POP(ch);
}
void CTextRam::fc_SCI(int ch)
{
	// VT52 Identify (host to terminal).			ANSI SCI Single character introducer
	if ( IS_ENABLE(m_AnsiOpt, TO_DECANM) )
		UNGETSTR("\033/Z");
	fc_POP(ch);
}
void CTextRam::fc_RIS(int ch)
{
	//												ANSI RIS Reset to initial state
	RESET();
	fc_POP(ch);
}
void CTextRam::fc_LMA(int ch)
{
	// HP fc_LMA LOCK
	m_TopY = m_CurY;
	fc_POP(ch);
}
void CTextRam::fc_USR(int ch)
{
	// HP USR UNLOCK */
	m_TopY = 0;
	fc_POP(ch);
}
void CTextRam::fc_V5EX(int ch)
{
	// VT52 Exit VT52 mode. Enter VT100 mode.
	m_AnsiOpt[TO_DECANM / 32] &= ~(1 << (TO_DECANM % 32));
	fc_POP(ch);
}
void CTextRam::fc_SS2(int ch)
{
	// SS2 Single shift 2
	m_BankSG = 2;
	fc_POP(ch);
}
void CTextRam::fc_SS3(int ch)
{
	// SS3 Single shift 3
	m_BankSG = 3;
	fc_POP(ch);
}
void CTextRam::fc_LS2(int ch)
{
	// LS2 Locking-shift two
	m_BankGL = 2;
	fc_POP(ch);
}
void CTextRam::fc_LS3(int ch)
{
	// LS3 Locking-shift three
	m_BankGL = 3;
	fc_POP(ch);
}
void CTextRam::fc_LS1R(int ch)
{
	// LS1R
	m_BankGR = 1;
	fc_POP(ch);
}
void CTextRam::fc_LS2R(int ch)
{
	// LS2R
	m_BankGR = 2;
	fc_POP(ch);
}
void CTextRam::fc_LS3R(int ch)
{
	// LS3R
	m_BankGR = 3;
	fc_POP(ch);
}
void CTextRam::fc_CSC0W(int ch)
{
	// CSC0W Char Set
	m_BackChar = 0;
	m_BackMode = SET_94x94;
	m_Status = ST_CHARSET_2;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC0(int ch)
{
	// CSC0 G0 charset
	m_BackChar = 0;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC1(int ch)
{
	// CSC1 G1 charset
	m_BackChar = 1;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC2(int ch)
{
	// CSC2 G2 charset
	m_BackChar = 2;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC3(int ch)
{
	// CSC3 G3 charset
	m_BackChar = 3;
	m_BackMode = SET_94;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC0A(int ch)
{
	// CSC0A G0 charset
	m_BackChar = 0;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC1A(int ch)
{
	// CSC1A G1 charset
	m_BackChar = 1;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC2A(int ch)
{
	// CSC2A G2 charset
	m_BackChar = 2;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_CSC3A(int ch)
{
	// CSC3A G3 charset
	m_BackChar = 3;
	m_BackMode = SET_96;
	m_Status = ST_CHARSET_1;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_V5MCP(int ch)
{
	// VT52 Move cursor to column Pn.
	m_Status = ST_COLM_SET;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_DECSOP(int ch)
{
	// DEC Screen Opt
	m_Status = ST_DEC_OPT;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_ACS(int ch)
{
	// ACS
	m_Status = ST_CONF_LEVEL;
	fc_Case(STAGE_STAT);
}
void CTextRam::fc_DOCS(int ch)
{
	// DOCS Designate other coding system
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

	case ST_CONF_LEVEL:
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

	case ST_COLM_SET:
		m_Status = ST_COLM_SET_2;
		m_BackChar = ch;
		break;
	case ST_COLM_SET_2:
		m_Status = ST_NON;
		LOCATE(ch - ' ', m_BackChar - ' ');
		break;

	case ST_DEC_OPT:
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


	case ST_DOCS_OPT:
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

	case ST_TEK_OPT:
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

void CTextRam::fc_OSC(int ch)
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
		while ( *p != L'\0' ) {
			if ( *p >= L'0' && *p <= L'9' ) {
				if ( m_AnsiPara.GetSize() <= 0 )
					m_AnsiPara.Add(0);
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
					COLORREF rgb = (n < 16 ? m_ColTab[n] : RGB(0, 0, 0));
					UNGETSTR("\033]4;%d;rgb:%04x/%04x/%04x%s", n,
						GetRValue(rgb), GetGValue(rgb), GetBValue(rgb),
						(ch == 0x07 ? "\007":"\033\\"));
				} else {
					int r, g, b;
					if ( n < 16 && swscanf(tmp, L"rgb:%x/%x/%x", &r, &g, &b) == 3 ) {
						m_ColTab[n] = RGB(r, g, b);
						DISPUPDATE();
					}
				}
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
	fc_Case(STAGE_CSI);
}
void CTextRam::fc_CSI_ESC(int ch)
{
	fc_Case(STAGE_ESC);
}
void CTextRam::fc_CSI_DIGIT(int ch)
{
	if ( m_AnsiPara.GetSize() <= 0 )
		m_AnsiPara.Add(0);
	int n = (int)m_AnsiPara.GetSize() - 1;
	m_AnsiPara[n] = m_AnsiPara[n] * 10 + ((ch & 0x7F) - '0');
}
void CTextRam::fc_CSI_SEPA(int ch)
{
	m_AnsiPara.Add(0);
}
void CTextRam::fc_CSI_EXT(int ch)
{
	if ( ch >= 0x20 && ch <= 0x2F ) {
		m_BackChar &= 0xFF0000;
		m_BackChar |= (ch << 8);
	} else
		m_BackChar = (ch << 16);

	switch(m_BackChar & 0x7F7F00) {
	case '?' << 16:
		fc_Case(STAGE_EXT1);
		break;
	case '$' << 8:
		fc_Case(STAGE_EXT2);
		break;
	default:
		fc_Case(STAGE_EXT3);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// fc CSI A-Z

void CTextRam::fc_ICH(int ch)
{
	// ICH	Insert Character
	for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
		INSCHAR();
	fc_POP(ch);
}
void CTextRam::fc_CUU(int ch)
{
	// CUU Cursor Up
	int n = GetAnsiPara(0, 1);
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
	// CUD Cursor Down
	int n = GetAnsiPara(0, 1);
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
	// CUF Cursor Forward
	LOCATE(m_CurX + GetAnsiPara(0, 1), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUB(int ch)
{
	// CUB Cursor Backward
	LOCATE(m_CurX - GetAnsiPara(0, 1), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CNL(int ch)
{
	// CNL Move the cursor to the next line.
	LOCATE(0, m_CurY + GetAnsiPara(0, 1));
	fc_POP(ch);
}
void CTextRam::fc_CPL(int ch)
{
	// CPL Move the cursor to the preceding line.
	LOCATE(0, m_CurY - GetAnsiPara(0, 1));
	fc_POP(ch);
}
void CTextRam::fc_CHA(int ch)
{
	// CHA Move the active position to the n-th character of the active line.
	LOCATE(GetAnsiPara(0, 1) - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_CUP(int ch)
{
	// CUP.Cursor Position
	if ( IsOptEnable(TO_DECOM) )
		LOCATE(GetAnsiPara(1, 1) - 1, GetAnsiPara(0, 1) - 1 + m_TopY);
	else
		LOCATE(GetAnsiPara(1, 1) - 1, GetAnsiPara(0, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_CHT(int ch)
{
	// CHT Move the active position n tabs forward.
	for ( int i = GetAnsiPara(0, 1) ; i > 0 ; i-- )
		TABSET(TAB_COLSNEXT);
	fc_POP(ch);
}
void CTextRam::fc_ED(int ch)
{
	// ED Erase in page
	switch(GetAnsiPara(0, 0)) {
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
	// EL Erase in line
	switch(GetAnsiPara(0, 0)) {
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
	// IL Insert Line
	if ( m_CurY >= m_TopY && m_CurY < m_BtmY ) {
		for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- ) {
			BAKSCROLL(m_CurY, m_BtmY);
			TABSET(TAB_INSLINE);
		}
	}
	fc_POP(ch);
}
void CTextRam::fc_DL(int ch)
{
	// DL Delete Line
	if ( m_CurY >= m_TopY && m_CurY < m_BtmY ) {
		for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- ) {
			FORSCROLL(m_CurY, m_BtmY);
			TABSET(TAB_DELINE);
		}
	}
	fc_POP(ch);
}
void CTextRam::fc_EF(int ch)
{
	// EF Erase in field
	switch(GetAnsiPara(0, 0)) {
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
	// EA Erase in area
	switch(GetAnsiPara(0, 0)) {
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
	// DCH Delete Character
	for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
		DELCHAR();
	fc_POP(ch);
}
void CTextRam::fc_SU(int ch)
{
	// SU Scroll Up
	for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
		FORSCROLL(m_TopY, m_BtmY);
	fc_POP(ch);
}
void CTextRam::fc_SD(int ch)
{
	// SD Scroll Down
	if ( m_AnsiPara.GetSize() != 5 || GetAnsiPara(0, 0) != 1 ) {
		for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
			BAKSCROLL(m_TopY, m_BtmY);
	}
	fc_POP(ch);
}
void CTextRam::fc_NP(int ch)
{
	// NP Next Page
	for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
		PostMessage(WM_VSCROLL, SB_PAGEDOWN, 0);
	fc_POP(ch);
}
void CTextRam::fc_PP(int ch)
{
	// PP Preceding Page
	for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
		PostMessage(WM_VSCROLL, SB_PAGEUP, 0);
	fc_POP(ch);
}
void CTextRam::fc_CTC(int ch)
{
	// CTC Cursor tabulation control
	switch(GetAnsiPara(0, 0)) {
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
	// ECH Erase character
	ERABOX(m_CurX, m_CurY, m_CurX + GetAnsiPara(0, 1), m_CurY + 1);
	fc_POP(ch);
}
void CTextRam::fc_CVT(int ch)
{
	// CVT Cursor line tabulation
	for ( int i = GetAnsiPara(0, 1) ; i > 0 ; i-- )
		TABSET(TAB_LINENEXT);
	fc_POP(ch);
}
void CTextRam::fc_CBT(int ch)
{
	// CBT Move the active position n tabs backward.
	for ( int i = GetAnsiPara(0, 1) ; i > 0 ; i-- )
		TABSET(TAB_COLSBACK);
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI a-z

void CTextRam::fc_SIMD(int ch)
{
	// SIMD Select implicit movement direction
	// 0 the direction of implicit movement is the same as that of the character progression
	// 1 the direction of implicit movement is opposite to that of the character progression.
	fc_POP(ch);
}
void CTextRam::fc_HPA(int ch)
{
	// HPA Horizontal Position Absolute
	LOCATE(GetAnsiPara(0, 1) - 1, m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_HPR(int ch)
{
	// HPR Horizontal Position Relative
	LOCATE(m_CurX + GetAnsiPara(0, 1), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_REP(int ch)
{
	// REP Repeat
	if ( m_LastChar != 0 ) {
		if ( (m_LastChar & 0xFFFFFF00) != 0 )
			m_LastChar = ' ';
		for ( int n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
			PUT1BYTE(m_LastChar, m_BankNow);
		m_LastChar = 0;
	}
	fc_POP(ch);
}
void CTextRam::fc_DA1(int ch)
{
	// DA1 Primary Device Attributes
	// UNGETSTR("\033[?1;0c");
	UNGETSTR("\033[?65;22;44c");
	fc_POP(ch);
}
void CTextRam::fc_VPA(int ch)
{
	// VPA Vertical Line Position Absolute
	LOCATE(m_CurX, GetAnsiPara(0, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_VPR(int ch)
{
	// VPR Vertical Position Relative
	LOCATE(m_CurX, m_CurY + GetAnsiPara(0, 1));
	fc_POP(ch);
}
void CTextRam::fc_HVP(int ch)
{
	// HVP Horizontal and Vertical Position
	LOCATE(GetAnsiPara(1, 1) - 1, GetAnsiPara(0, 1) - 1);
	fc_POP(ch);
}
void CTextRam::fc_TBC(int ch)
{
	// TBC Tab Clear
	switch(GetAnsiPara(0, 0)) {
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
void CTextRam::fc_SRM(int ch)
{
	// SM Mode Set
	// RM Mode ReSet
	int n, i;
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( (i = GetAnsiPara(n, 0)) >= 1 && i < 100 )
			ANSIOPT(ch & 0x7F, i + 200);
	}
	fc_POP(ch);
}
void CTextRam::fc_MC(int ch)
{
	// MC Media copy
	fc_POP(ch);
}
void CTextRam::fc_HPB(int ch)
{
	// HPB Character position backward
	LOCATE(m_CurX - GetAnsiPara(0, 1), m_CurY);
	fc_POP(ch);
}
void CTextRam::fc_VPB(int ch)
{
	// VPB Line position backward
	LOCATE(m_CurX, m_CurY - GetAnsiPara(0, 1));
	fc_POP(ch);
}
void CTextRam::fc_SGR(int ch)
{
	// SGR Select Graphic Rendition
	int n, i;
	if ( m_AnsiPara.GetSize() <= 0 )
		m_AnsiPara.Add(0);
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		switch(m_AnsiPara[n]) {
		case 0: m_AttNow.cl = m_DefAtt.cl; m_AttNow.at = m_DefAtt.at; break;
		case 1: m_AttNow.at |= ATT_BOLD;   break;	// 1 bold or increased intensity
		case 2: m_AttNow.at |= ATT_HALF;   break;	// 2 faint, decreased intensity or second colour
		case 3: m_AttNow.at |= ATT_ITALIC; break;	// 3 italicized
		case 4: m_AttNow.at |= ATT_UNDER;  break;	// 4 singly underlined
		case 5: m_AttNow.at |= ATT_SBLINK; break;	// 5 slowly blinking
		case 6: m_AttNow.at |= ATT_BLINK;  break;	// 6 rapidly blinking
		case 7: m_AttNow.at |= ATT_REVS;   break;	// 7 negative image
		case 8: m_AttNow.at |= ATT_SECRET; break;	// 8 concealed characters
		case 9: m_AttNow.at |= ATT_LINE;   break;	// 9 crossed-out

		//case 21: m_AttNow.at |= ATT_UNDER;  break;					// 21 doubly underlined
		case 22: m_AttNow.at &= ~(ATT_BOLD | ATT_HALF); break;		// 22 normal colour or normal intensity (neither bold nor faint)
		case 23: m_AttNow.at &= ~ATT_ITALIC; break;					// 23 not italicized, not fraktur
		case 24: m_AttNow.at &= ~ATT_UNDER; break;					// 24 not underlined (neither singly nor doubly)
		case 25: m_AttNow.at &= ~(ATT_SBLINK | ATT_BLINK); break;	// 25 steady (not blinking)
		case 27: m_AttNow.at &= ~ATT_REVS; break;					// 27 positive image
		case 28: m_AttNow.at &= ~ATT_SECRET; break;					// 28 revealed characters
		case 29: m_AttNow.at &= ~ATT_LINE; break;					// 29 not crossed out

		case 30: case 31: case 32: case 33:
		case 34: case 35: case 36: case 37:
			m_AttNow.cl &= 0xF0;
			m_AttNow.cl |= (m_AnsiPara[n] - 30);
			break;
		case 38:
			if ( GetAnsiPara(n + 1, 0) == 5 ) {	// 256 color
				if ( (i = GetAnsiPara(n + 2, 0)) < 16 ) {
					m_AttNow.cl &= 0xF0;
					m_AttNow.cl |= i;
				}
				n += 2;
			}
			break;
		case 39:
			m_AttNow.cl &= 0xF0;
			m_AttNow.cl |= (m_DefAtt.cl & 0x0F);
			break;
		case 40: case 41: case 42: case 43:
		case 44: case 45: case 46: case 47:
			m_AttNow.cl &= 0x0F;
			m_AttNow.cl |= ((m_AnsiPara[n] - 40) << 4);
			break;
		case 48:
			if ( GetAnsiPara(n + 1, 0) == 5 ) {	// 256 color
				if ( (i = GetAnsiPara(n + 2, 0)) < 16 ) {
					m_AttNow.cl &= 0x0F;
					m_AttNow.cl |= (i << 4);
				}
				n += 2;
			}
			break;
		case 49:
			m_AttNow.cl &= 0x0F;
			m_AttNow.cl |= (m_DefAtt.cl & 0xF0);
			break;

		case 90: case 91: case 92: case 93:
		case 94: case 95: case 96: case 97:
			m_AttNow.cl &= 0xF0;
			m_AttNow.cl |= (m_AnsiPara[n] - 90 + 8);
			break;
		case 100: case 101:  case 102: case 103:
		case 104: case 105:  case 106: case 107:
			m_AttNow.cl &= 0x0F;
			m_AttNow.cl |= ((m_AnsiPara[n] - 100 + 8) << 4);
			break;
		}
	}
	if ( IS_ENABLE(m_AnsiOpt, TO_DECECM) == 0 )
		m_AttSpc.cl = m_AttNow.cl;
	if ( IS_ENABLE(m_AnsiOpt, TO_RLGCWA) != 0 )
		m_AttSpc.at = m_AttNow.at;
	fc_POP(ch);
}
void CTextRam::fc_DSR(int ch)
{
	// DSR Device status report
	switch(GetAnsiPara(0, 0)) {
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
	// DAQ Define area qualification
	fc_POP(ch);
}
void CTextRam::fc_DECBFAT(int ch)
{
	// fDECBFAT Begin field attribute : DEC private
	int n = GetAnsiPara(0, 0);
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
	// DECLL Load LEDs
	fc_POP(ch);
}
void CTextRam::fc_DECSTBM(int ch)
{
	// DECSTBM Set Top and Bottom Margins
	int n, i;
	if ( (n = GetAnsiPara(0, 1) - 1) < 0 )
		n = 0;
	if ( (i = GetAnsiPara(1, m_Lines) - 1) < 0 || i >= m_Lines )
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
	// DECSC Save Cursol Pos
	m_Save_CurX = m_CurX;
	m_Save_CurY = m_CurY;
	fc_POP(ch);
}
void CTextRam::fc_XTWOP(int ch)
{
	int n = GetAnsiPara(0, 0);
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
		if ( GetAnsiPara(2, 0) > m_DefCols[0] ) {
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
		m_IConv.IConvStr("CP932", m_SendCharSet[m_KanjiMode], m_pDocument->GetTitle(), tmp);
		UNGETSTR("\033]%c%s\033\\", (n == 20 ? 'L':'l'), tmp);
		break;
	}
	fc_POP(ch);
}
void CTextRam::fc_DECRC(int ch)
{
	// DECRC Load Cursol Pos
	LOCATE(m_Save_CurX, m_Save_CurY);
	fc_POP(ch);
}
void CTextRam::fc_ORGSCD(int ch)
{
	/* Set Cursol Display */
	switch(GetAnsiPara(0, 0)) {
	case 0:  CURON(); break;
	default: CUROFF(); break;
	}
	fc_POP(ch);
}
void CTextRam::fc_REQTPARM(int ch)
{
	// DECREQTPARM Request terminal parameters 
	switch(GetAnsiPara(0, 0)) {
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
	// DECTST Invoke confidence test
	fc_POP(ch);
}
void CTextRam::fc_DECVERP(int ch)
{
	// DECVERP Set vertical pitch
	fc_POP(ch);
}
void CTextRam::fc_LINUX(int ch)
{
	// LINUX Linux Console */
	switch(GetAnsiPara(0, 0)) {
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
	// DECSED Selective Erase in Display
	switch(GetAnsiPara(0, 0)) {
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
	// DECSEL Selective Erase in Line
	switch(GetAnsiPara(0, 0)) {
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
	//  DECST8C Set Tab at every 8 columns
	TABSET(TAB_RESET);
	fc_POP(ch);
}
void CTextRam::fc_DECSRET(int ch)
{
	/* DECSET			DECRST */
	/* XTERM_RESTORE	XTERM_SAVE */

	int n, i;
	ch &= 0x7F;
	for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
		if ( (i = GetAnsiPara(n, 0)) < 1 )
			continue;
		else if ( i >= 1000 && i < 1100 )
			i -= 700;		// 300-399
		else if ( i >= 200 )
			continue;

		ANSIOPT(ch, i);
		switch(i) {
		case 2:			// DECANM ANSI/VT52 Mode
			if ( ch == 'l' )
				m_AnsiOpt[TO_DECANM / 32] |= (1 << (TO_DECANM % 32));
			else if ( ch == 'h' )
				m_AnsiOpt[TO_DECANM / 32] &= ~(1 << (TO_DECANM % 32));
			break;
		case 3:			// DECCOLM Column mode
		case 40:		// XTerm Column switch control
			if ( IsOptEnable(TO_XTMCSC) ) {
				InitCols();
				LOCATE(0, 0);
				ERABOX(0, 0, m_Cols, m_Lines, 2);
			}
			break;
		case 5:			// DECSCNM.Screen Mode: Light or Dark Screen
			DISPVRAM(0, 0, m_Cols, m_Lines);
			break;
		case 25:		// DECTCEM.Text Cursor Enable Mode
			if ( ch == 'l' )
				CUROFF();
			else if ( ch == 'h' )
				CURON();
			break;
		case 38:		// DECTEK
			if ( ch == 'h' ) {
				TekInit(4014);
				fc_Case(STAGE_TEK);
				return;
			}
			break;
		case 47:		// XTERM alternate buffer
			if ( ch == 'l' )
				LOADRAM();
			else if ( ch == 'h' )
				SAVERAM();
			break;
		case TO_XTMOSREP:
			m_MouseTrack = (IsOptEnable(i) ? 1 : 0);
			m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
			break;
		case TO_XTNOMTRK:
		case TO_XTHILTRK:
		case TO_XTBEVTRK:
		case TO_XTAEVTRK:
			m_MouseTrack = (IsOptEnable(i) ? (i - TO_XTNOMTRK + 2) : 0);
			m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
			break;
		}
	}
	fc_POP(ch);
}
void CTextRam::fc_DECDSR(int ch)
{
	// DECDSR Device status report
	switch(GetAnsiPara(0, 0)) {
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
		UNGETSTR("\033[?53n");	// no locator
		break;
	case 56:	// locator type
		UNGETSTR("\033[?57;0n");	// No locator
		break;
	case 63:	// Request checksum of macro definitions
		// DCS Pn ! ~ Ps ST
		UNGETSTR("\033P%d!~0\033\\", GetAnsiPara(1, 0));	// 
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
	// DECRQM Request mode settings
	int n;
	if ( (n = GetAnsiPara(0, 1)) > 199 )
		n = 199;
	UNGETSTR("\033[%d;%d$y", n, IsOptEnable(200 + n) ? 1 : 2);
	fc_POP(ch);
}
void CTextRam::fc_DECCARA(int ch)
{
	// DECCARA Change Attribute in Rectangle
	int n, i, x, y;
	VRAM *vp;
	SetAnsiParaArea(0);
	while ( m_AnsiPara.GetSize() < 5 )
		m_AnsiPara.Add(0);
	for ( y = m_AnsiPara[0] ; y <= m_AnsiPara[2] ; y++ ) {
		for ( x = m_AnsiPara[1] ; x <= m_AnsiPara[3] ; x++ ) {
			vp = GETVRAM(x, y);
			for ( n = 4 ; n < m_AnsiPara.GetSize() ; n++ ) {
				switch(m_AnsiPara[n]) {
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

				case 22: vp->at &= ~(ATT_BOLD | ATT_HALF); break;
				case 23: vp->at &= ~ATT_ITALIC; break;
				case 24: vp->at &= ~ATT_UNDER; break;
				case 25: vp->at &= ~(ATT_SBLINK | ATT_BLINK); break;
				case 27: vp->at &= ~ATT_REVS; break;
				case 28: vp->at &= ~ATT_SECRET; break;
				case 29: vp->at &= ~ATT_LINE; break;

				case 30: case 31: case 32: case 33:
				case 34: case 35: case 36: case 37:
					vp->cl &= 0xF0;
					vp->cl |= (m_AnsiPara[n] - 30);
					break;
				case 38:
					if ( GetAnsiPara(n + 1, 0) == 5 ) {	// 256 color
						if ( (i = GetAnsiPara(n + 2, 0)) < 16 ) {
							vp->cl &= 0xF0;
							vp->cl |= i;
						}
						n += 2;
					}
					break;
				case 39:
					vp->cl &= 0xF0;
					vp->cl |= (m_DefAtt.cl & 0x0F);
					break;
				case 40: case 41: case 42: case 43:
				case 44: case 45: case 46: case 47:
					vp->cl &= 0x0F;
					vp->cl |= ((m_AnsiPara[n] - 40) << 4);
					break;
				case 48:
					if ( GetAnsiPara(n + 1, 0) == 5 ) {	// 256 color
						if ( (i = GetAnsiPara(n + 2, 0)) < 16 ) {
							vp->cl &= 0x0F;
							vp->cl |= (i << 4);
						}
						n += 2;
					}
					break;
				case 49:
					vp->cl &= 0x0F;
					vp->cl |= (m_DefAtt.cl & 0xF0);
					break;

				case 90: case 91: case 92: case 93:
				case 94: case 95: case 96: case 97:
					vp->cl &= 0xF0;
					vp->cl |= (m_AnsiPara[n] - 90 + 8);
					break;
				case 100: case 101:  case 102: case 103:
				case 104: case 105:  case 106: case 107:
					vp->cl &= 0x0F;
					vp->cl |= ((m_AnsiPara[n] - 100 + 8) << 4);
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
	// DECRARA Reverse Attribute in Rectangle
	int n, x, y;
	VRAM *vp;	SetAnsiParaArea(0);
	while ( m_AnsiPara.GetSize() < 5 )
		m_AnsiPara.Add(0);
	for ( y = m_AnsiPara[0] ; y <= m_AnsiPara[2] ; y++ ) {
		for ( x = m_AnsiPara[1] ; x <= m_AnsiPara[3] ; x++ ) {
			vp = GETVRAM(x, y);
			for ( n = 4 ; n < m_AnsiPara.GetSize() ; n++ ) {
				switch(m_AnsiPara[n]) {
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
	// DECCRA Copy Rectangular Area
	int n, i, x, y, cx, cy;
	VRAM *vp, *cp;
	SetAnsiParaArea(0);
	if ( (cy = GetAnsiPara(5, 1) - 1) < 0 ) cy = 0;
	else if ( cy >= m_Lines ) cy = m_Lines - 1;
	if ( (cx = GetAnsiPara(6, 1) - 1) < 0 ) cx = 0;
	else if ( cx >= m_Cols ) cx = m_Cols - 1;
	for ( y = m_AnsiPara[0], i = cy ; y <= m_AnsiPara[2] && i < m_Lines ; y++, i++ ) {
		for ( x = m_AnsiPara[1], n = cx ; x <= m_AnsiPara[3] && n < m_Cols ; x++, n++ ) {
			vp = GETVRAM(x, y);
			cp = GETVRAM(n, i);
			*cp = *vp;
		}
	}
	DISPVRAM(cx, cy, m_AnsiPara[3] - m_AnsiPara[1] + 1, m_AnsiPara[2] - m_AnsiPara[0] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECFRA(int ch)
{
	// DECFRA Fill Rectangular Area
	int n, x, y;
	VRAM *vp;
	SetAnsiParaArea(1);
	n = GetAnsiPara(0, 0);
	for ( y = m_AnsiPara[1] ; y <= m_AnsiPara[3] ; y++ ) {
		for ( x = m_AnsiPara[2] ; x <= m_AnsiPara[4] ; x++ ) {
			vp = GETVRAM(x, y);
			vp->ch = (BYTE)n;
			vp->at = m_AttNow.at;
			vp->cl = m_AttNow.cl;
			vp->md = m_BankTab[m_KanjiMode][(n & 0x80) == 0 ? m_BankGL : m_BankGR];
		}
	}
	DISPVRAM(m_AnsiPara[2], m_AnsiPara[1], m_AnsiPara[4] - m_AnsiPara[2] + 1, m_AnsiPara[3] - m_AnsiPara[1] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECERA(int ch)
{
	// DECERA Erase Rectangular Area
	SetAnsiParaArea(0);
	ERABOX(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] + 1, m_AnsiPara[2] + 1);
	fc_POP(ch);
}
void CTextRam::fc_DECSERA(int ch)
{
	// DECSERA Selective Erase Rectangular Area
	SetAnsiParaArea(0);
	ERABOX(m_AnsiPara[1], m_AnsiPara[0], m_AnsiPara[3] + 1, m_AnsiPara[2] + 1);
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// fc CSI Ext...

void CTextRam::fc_CSI_ETC(int ch)
{
	int n;

	switch((m_BackChar | ch) & 0x7F7F7F) {
	case ('?' << 16) | ('$' << 8) | 'p':		// DECRQM Request Mode (DEC) Host to Terminal
		if ( (n = GetAnsiPara(0, 1)) > 199 )
			n = 199;
		UNGETSTR("\033[?%d;%d$y", n, IsOptEnable(n) ? 1 : 2);
		break;
	case ('?' << 16) | ('$' << 8) | 'y':		// Report Mode (DEC) Terminal to Host
		break;

	case (' ' << 8) | '@':		// SL Scroll left
		LEFTSCROLL();
		break;
	case (' ' << 8) | 'A':		// SR Scroll Right
		RIGHTSCROLL();
		break;
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
		break;
#endif
	case (' ' << 8) | '~':		// DECTME Terminal Mode Emulation
		if ( GetAnsiPara(0, 0) == 3 )
			m_AnsiOpt[TO_DECANM / 32] |= (1 << (TO_DECANM % 32));
		else
			m_AnsiOpt[TO_DECANM / 32] &= ~(1 << (TO_DECANM % 32));
		break;

	case ('!' << 8) | 'p':		// DECSTR Soft terminal reset
		RESET();
		break;
#if 0
	case ('!' << 8) | 's':		// DECFIL Right justification
	case ('!' << 8) | 'v':		// DECASFC Automatic sheet feeder control
	case ('!' << 8) | 'w':		// DECUND Select undeline character
	case ('!' << 8) | 'x':		// DECPTS Printwheel table select
	case ('!' << 8) | 'y':		// DECSS Select spacing
	case ('!' << 8) | '|':		// DECVEC Draw vector
	case ('!' << 8) | '}':		// DECFIN Document finishing
		break;
#endif
	case ('"' << 8) | 'p':		// DECSCL Set compatibility level
		break;
	case ('"' << 8) | 'q':		// DECSCA Select character attributes
		if ( GetAnsiPara(0, 0) == 1 )
			m_AttNow.em |= EM_ERM;
		else
			m_AttNow.em &= ~EM_ERM;
		break;
#if 0
	case ('"' << 8) | 's':		// DECPWA Page width alignment
	case ('"' << 8) | 'u':		//        Set Transmit Rate Limit
	case ('"' << 8) | 'v':		// DECRQDE Request device extent
	case ('"' << 8) | 'w':		// DECRPDE Report device extent
	case ('"' << 8) | 'z':		// DECDEN Select density
		break;

	case ('&' << 8) | 'q':		// DECSNC Set number of copies
	case ('&' << 8) | 'u':		// DECRQUPSS Request User-Preferred Supplemental Set
	case ('&' << 8) | 'w':		// DECLRP Locator report
	case ('&' << 8) | 'x':		// DECES Enable Session
		break;

	case ('\'' << 8) | 'q':		// DECSBCA Select bar code attributes
	case ('\'' << 8) | 'w':		// DECEFR Enable filter rectangle
	case ('\'' << 8) | 'z':		// DECELR Enable locator reports
	case ('\'' << 8) | '{':		// DECSLE Select locator events
	case ('\'' << 8) | '|':		// DECRQLP Request locator position
#endif
	case ('\'' << 8) | '}':		// DECIC Insert Column
		for ( n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
			INSCHAR();
		break;
	case ('\'' << 8) | '~':		// DECDC Delete Column(s)
		for ( n = GetAnsiPara(0, 1) ; n > 0 ; n-- )
			DELCHAR();
		break;
#if 0
	case ('*' << 8) | 'p':		// Select ProPrinter Character Set
	case ('*' << 8) | 'q':		// DECSRC Secure Reset Confirmation
	case ('*' << 8) | 'r':		// Select Communication Speed
	case ('*' << 8) | 's':		// Select Flow Control Type
	case ('*' << 8) | 'u':		// Select Communication Port
	case ('*' << 8) | 'x':		// DECSACE Select Attribute and Change Extent
	case ('*' << 8) | 'y':		// DECRQCRA Request Checksum of Rectangle Area
	case ('*' << 8) | 'z':		// DECINVM Invoke Macro
	case ('*' << 8) | '{':		// DECMSR Macro Space Report Response
	case ('*' << 8) | '|':		// DECSNLS Select number of lines per screen
	case ('*' << 8) | '}':		// DECLFKC Local Function Key Control
		break;

	case ('+' << 8) | 'p':		// DECSR Secure Reset
	case ('+' << 8) | 'q':		// DECELF Enable Local Functions
	case ('+' << 8) | 'r':		// DECSMKR Select Modifier Key Reporting
	case ('+' << 8) | 'v':		// DECMM Memory management
	case ('+' << 8) | 'w':		// DECSPP Set Port Parameter
	case ('+' << 8) | 'x':		// DECPQPLFM Program Key Free Memory Inquiry
	case ('+' << 8) | 'y':		// DECPKFMR Program Key Free Memory Report
	case ('+' << 8) | 'z':		// DECPKA Program Key Action
		break;

	case ('-' << 8) | 'p':		// DECARP Auto Repeat Rate
	case ('-' << 8) | 'q':		// DECCRTST CRT Saver Timing
	case ('-' << 8) | 'r':		// DECSEST Energy Saver Timing
		break;

	case (',' << 8) | 'p':		// Load Time of Day
	case (',' << 8) | 'u':		// Key Type Inquiry
	case (',' << 8) | 'v':		// Report Key Type
	case (',' << 8) | 'w':		// Key Definition Inquiry
	case (',' << 8) | 'x':		// Session Page Memory Allocation
	case (',' << 8) | 'y':		// Update Session
	case (',' << 8) | 'z':		// Down Line Load Allocation
	case (',' << 8) | '|':		// Assign Color
	case (',' << 8) | '{':		// Select Zero Symbol
		break;
#endif
	case (',' << 8) | '}':		// DECATC Alternate Text Colors
		switch(GetAnsiPara(0, 0)) {
		case 0: m_AttNow.at = 0; break;
		case 1: m_AttNow.at = ATT_BOLD; break;
		case 2: m_AttNow.at = ATT_REVS; break;
		case 3: m_AttNow.at = ATT_UNDER; break;
		case 4: m_AttNow.at = ATT_BLINK; break;
		case 5: m_AttNow.at = ATT_BOLD | ATT_REVS; break;
		case 6: m_AttNow.at = ATT_BOLD | ATT_UNDER; break;
		case 7: m_AttNow.at = ATT_BOLD | ATT_BLINK; break;
		case 8: m_AttNow.at = ATT_REVS | ATT_UNDER; break;
		case 9: m_AttNow.at = ATT_REVS | ATT_BLINK; break;
		case 10: m_AttNow.at = ATT_UNDER | ATT_BLINK; break;
		case 11: m_AttNow.at = ATT_BOLD | ATT_REVS | ATT_UNDER; break;
		case 12: m_AttNow.at = ATT_BOLD | ATT_REVS | ATT_BLINK; break;
		case 13: m_AttNow.at = ATT_BOLD | ATT_UNDER | ATT_BLINK; break;
		case 14: m_AttNow.at = ATT_REVS | ATT_UNDER | ATT_BLINK; break;
		case 15: m_AttNow.at = ATT_BOLD | ATT_REVS | ATT_UNDER | ATT_BLINK; break;
		}
		m_AttNow.cl = (GetAnsiPara(2, 0) << 4) | GetAnsiPara(1, 7);
		if ( IS_ENABLE(m_AnsiOpt, TO_DECECM) == 0 )
			m_AttSpc.cl = m_AttNow.cl;
		if ( IS_ENABLE(m_AnsiOpt, TO_RLGCWA) != 0 )
			m_AttSpc.at = m_AttNow.at;
		break;
	case (',' << 8) | '~':		// Play Sound
		break;

	case (')' << 8) | 'p':		// Select Digital Printed Data Type
	case (')' << 8) | '{':		// Select Color Look-Up Table
		break;

	case ('>' << 16) | 'c':		// DA2 Secondary Device Attributes
		UNGETSTR("\033[>65;10;1c");
		break;
	case ('=' << 16) | 'c':		// DA3 Tertiary Device Attributes
		UNGETSTR("\033P!|010205\033\\");
		break;
#if 0
	case ('=' << 16) | 'A':		// cons25 Set the border color to n
	case ('=' << 16) | 'B':		// cons25 Set bell pitch (p) and duration (d)
	case ('=' << 16) | 'C':		// cons25 Set global cursor type
	case ('=' << 16) | 'F':		// cons25 Set normal foreground color to n
	case ('=' << 16) | 'G':		// cons25 Set normal background color to n
	case ('=' << 16) | 'H':		// cons25 Set normal reverse foreground color to n
	case ('=' << 16) | 'I':		// cons25 Set normal reverse background color to n
		break;
#endif
	case ('=' << 16) | 'S':		// cons25 Set local cursor type
		switch(GetAnsiPara(0, 0)) {
		case 0:  CURON(); break;
		case 1:  CUROFF(); break;
		case 2:  CURON(); break;
		}
		break;
	}
	fc_POP(ch);
}

//////////////////////////////////////////////////////////////////////
// Tek Func

#define	TEK_FONT_WIDTH		TekFonts[m_Tek_Font][0]
#define	TEK_FONT_HEIGHT		TekFonts[m_Tek_Font][1]

static const WCHAR *TekPoint = L"・△＋＊○×□◇▽◎☆※＃＄％";
static const int TekFonts[4][2] = {
//		Width	Height
	{	55,		88,		},   	// 74 cols 35 lines
	{	51,		81,		},   	// 81 cols 38 lines
    {	34,		53,		},  	// 121 cols 58 lines
	{	31,		48,		},  	// 133 cols 64 lines
};
static const int SinTab[] = {	// sin(x)*1000 x=0 360+90..5
	1000,	 996,	 985,	 966,	 940,	 906,	 866,	 819,	 766,	 707,
	 643,	 574,	 500,	 423,	 342,	 259,	 174,	  87,	   0,	 -87,
	-174,	-259,	-342,	-423,	-500,	-574,	-643,	-707,	-766,	-819,
	-866,	-906,	-940,	-966,	-985,	-996,	-1000,	-996,	-985,	-966,
	-940,	-906,	-866,	-819,	-766,	-707,	-643,	-574,	-500,	-423,
	-342,	-259,	-174,	 -87,	   0,	  87,	 174,	 259,	 342,	 423,
	 500,	 574,	 643,	 707,	 766,	 819,	 866,	 906,	 940,	 966,
	 985,	 996,	1000,	 996,	 985,	 966,	 940,	 906,	 866,	 819,
	 766,	 707,	 643,	 574,	 500,	 423,	 342,	 259,	 174,	  87,
};

void CTextRam::TekInit(int ver)
{
	CString tmp;

	if ( m_pTekWnd != NULL ) {
		if ( m_Tek_Ver == ver || ver == 0 ) {
			if ( !m_pTekWnd->IsWindowVisible() )
				m_pTekWnd->ShowWindow(SW_SHOW);
			return;
		}
		m_Tek_Ver = ver;
		tmp.Format("Tek%dxx - %s", m_Tek_Ver / 100, m_pDocument->m_ServerEntry.m_EntryName);
		m_pTekWnd->SetWindowText(tmp);
	} else {
		if ( ver != 0 )
			m_Tek_Ver = ver;
		tmp.Format("Tek%dxx - %s", m_Tek_Ver / 100, m_pDocument->m_ServerEntry.m_EntryName);
		m_pTekWnd = new CTekWnd(this, ::AfxGetMainWnd());
		m_pTekWnd->Create(NULL, tmp);
	}

	if ( m_Tek_Ver == 4014 || ver == 0 ) {
		if ( !m_pTekWnd->IsWindowVisible() )
			m_pTekWnd->ShowWindow(SW_SHOW);
	}

	if ( m_Tek_Ver == 4014 ) {
		m_Tek_Line &= 0x1F;
		m_Tek_Base = 0;
		m_Tek_Angle = 0;
	}
}
void CTextRam::TekClose()
{
	if ( m_pTekWnd == NULL )
		return;
	m_pTekWnd->DestroyWindow();
}
void CTextRam::TekForcus(BOOL fg)
{
	if ( fg ) {		// to Tek Mode
		if ( fc_Stage != STAGE_TEK )
			fc_Push(STAGE_TEK);
	} else {		// to VT Mode
		if ( fc_Stage == STAGE_TEK )
			fc_POP(' ');
	}
	TekFlush();
}
void CTextRam::TekDraw(CDC *pDC, CRect &frame)
{
	int mv, dv;
	TEKNODE *tp;
	POINT po[2];
	CStringW tmp;
	int np = 0;
	int nf = 0;
	int na = 0;
	int OldMode;
	LOGBRUSH LogBrush;
	CFont font, *pOldFont;
	CPen pen, *pOldPen;
	static const DWORD PenExtTab[8][4]  = {	{ 8, 0, 8, 0 },	{ 1, 1, 1, 1 },	{ 1, 1, 3, 1 },	{ 2, 1, 2, 1 }, { 2, 2, 2, 2 },	{ 3, 1, 2, 1 },	{ 4, 2, 1, 2 },	{ 3, 2, 3, 2 }	};
	static const int PenTypeTab[]       = { PS_SOLID,		PS_DOT,			PS_USERSTYLE,	PS_USERSTYLE,	PS_DASH,		PS_DASHDOTDOT,	PS_DASHDOT,		PS_USERSTYLE };
	static const int PenWidthTab[]      = { 1, 2  };
	static const COLORREF PenColorTab[] = {	RGB(  0,   0,   0), RGB(192, 192, 192), RGB(192, 0, 0), RGB(0, 192, 0), RGB(0, 0, 192), RGB(0, 192, 192), RGB(192, 0, 192), RGB(192, 192, 0),
											RGB( 64,  64,  64), RGB( 96,  96,  96), RGB( 96, 0, 0), RGB(0,  96, 0), RGB(0, 0,  96), RGB(0,  96,  96), RGB( 96, 0,  96), RGB( 96,  96, 0) };

	memset(&LogBrush, 0, sizeof(LOGBRUSH));

	mv = frame.Width();
	dv = TEK_WIN_WIDTH;

	pen.CreatePen(PenTypeTab[np], PenWidthTab[np], RGB(0, 0, 0));
	font.CreateFont(0 - TekFonts[nf][1] * mv / dv, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "");

	pOldPen  = pDC->SelectObject(&pen);
	pOldFont = pDC->SelectObject(&font);
	OldMode  = pDC->SetBkMode(TRANSPARENT);

	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->FillSolidRect(frame, RGB(255, 255, 255));

	for ( tp = m_Tek_Top ; tp != NULL ; tp = tp->next ) {
		switch(tp->md) {
		case 0:	// Line
			if ( np != tp->st ) {
				np = tp->st;
				pen.DeleteObject();
				if ( PenTypeTab[np % 8] == PS_USERSTYLE ) {
					LogBrush.lbColor = PenColorTab[(np / 8) % 16];
					LogBrush.lbStyle = BS_SOLID;
					pen.CreatePen(PS_USERSTYLE, PenWidthTab[np / 128], &LogBrush, 4, PenExtTab[np % 8]);
				} else
					pen.CreatePen(PenTypeTab[np % 8], PenWidthTab[np / 128], PenColorTab[(np / 8) % 16]);
				pDC->SelectObject(&pen);
			}
			po[0].x = frame.left   + tp->sx * mv / dv;
			po[0].y = frame.bottom - tp->sy * mv / dv;
			po[1].x = frame.left   + tp->ex * mv / dv;
			po[1].y = frame.bottom - tp->ey * mv / dv;
			pDC->Polyline(po, 2);
			break;
		case 1:	// Text
			if ( nf != tp->st || na != tp->ex ) {
				nf = tp->st;
				na = tp->ex;
				font.DeleteObject();
				font.CreateFont(0 - TekFonts[nf][1] * mv / dv, 0, na * 10, 0, FW_DONTCARE, 0, 0, 0, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "");
				pDC->SelectObject(&font);
			}
			if ( (tp->ch & 0xFFFF0000) != 0 )
				tmp = (WCHAR)(tp->ch >> 16);
			tmp = (WCHAR)(tp->ch & 0xFFFF);
			::ExtTextOutW(pDC->m_hDC,
				frame.left   + tp->sx * mv / dv,
				frame.bottom - (tp->sy + TekFonts[nf][1]) * mv / dv,
				0, NULL, tmp, 1, NULL);
			break;
		}
	}

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldFont);
	pDC->SetBkMode(OldMode);
}
void CTextRam::TekFlush()
{
	if ( m_pTekWnd != NULL )
		m_pTekWnd->Invalidate(0);

	m_Tek_Flush = FALSE;

//	TRACE("Tek Flush\n");
}
void CTextRam::TekClear()
{
	TEKNODE *tp;

	while ( (tp = m_Tek_Top) != NULL ) {
		m_Tek_Top = tp->next;
		tp->next = m_Tek_Free;
		m_Tek_Free = tp;
	}

	if ( m_Tek_Ver == 4014 ) {
		m_Tek_Font = 0;
		m_Tek_Line = 0;
		m_Tek_Base = 0;
		m_Tek_Angle = 0;
		m_Tek_cX = 0; m_Tek_cY = TEK_WIN_HEIGHT - 88;
		m_Tek_lX = 0; m_Tek_lY = m_Tek_cY;
		m_Tek_wX = 0; m_Tek_wY = m_Tek_cY;
	}

	TekFlush();
}
void CTextRam::TekLine(int st, int sx, int sy, int ex, int ey)
{
	TEKNODE *tp;

	if ( (tp = m_Tek_Free) != NULL )
		m_Tek_Free = tp->next;
	else
		tp = new TEKNODE;

	tp->md = 0;
	tp->st = st;
	tp->sx = sx;
	tp->sy = sy;
	tp->ex = ex;
	tp->ey = ey;

	tp->next = m_Tek_Top;
	m_Tek_Top = tp;

	if ( m_Tek_Flush )
		TekFlush();

//	TRACE("Tek Line #%d %d,%d - %d,%d\n", st, sx, sy, ex, ey);
}
void CTextRam::TekText(int st, int sx, int sy, int ch)
{
	TEKNODE *tp;

	if ( (tp = m_Tek_Free) != NULL )
		m_Tek_Free = tp->next;
	else
		tp = new TEKNODE;

	tp->md = 1;
	tp->st = st;
	if ( m_Tek_Ver == 4105 ) {
		tp->sx = sx;
		tp->sy = sy;
		tp->ex = m_Tek_Angle;

		switch(m_Tek_Angle / 90) {		// m_Tek_Base == 0 ?
		case 0:		// Right
			tp->sy -= TekFonts[st][1] / 8;
			break;
		case 1:		// Up
			tp->sx -= TekFonts[st][1] / 8;
			tp->sy -= TekFonts[st][0];
			break;
		case 2:		// Left
			tp->sy += TekFonts[st][1] / 8;
			break;
		default:	// Down
			tp->sx += TekFonts[st][1] / 8;
			tp->sy += TekFonts[st][0];
			break;
		}
	} else {
		tp->sx = sx;
		tp->sy = sy;
		tp->ex = 0;
	}
	tp->ey = 0;
	tp->ch = ch;

	tp->next = m_Tek_Top;
	m_Tek_Top = tp;

	if ( m_Tek_Flush )
		TekFlush();

//	TRACE("Tek Text #%d %d,%d (%c)\n", st, sx, sy, ch);
}
void CTextRam::fc_TEK_IN(int ch)
{
	TekInit(4014);
	TekClear();
	fc_Case(STAGE_TEK);
}
void CTextRam::fc_TEK_CUR(int ch)
{
	switch(ch) {
	case 0x08:	// LEFT
		if ( (m_Tek_cX -= TEK_FONT_WIDTH) > 0 )
			break;
		m_Tek_cX = TEK_WIN_WIDTH - TEK_FONT_WIDTH;
	case 0x0B:	// UP
		if ( (m_Tek_cY += TEK_FONT_HEIGHT) >= TEK_WIN_HEIGHT )
			m_Tek_cY = TEK_WIN_HEIGHT - TEK_FONT_HEIGHT;
		break;

	case 0x09:	// RIGHT
		if ( (m_Tek_cX += TEK_FONT_WIDTH) < TEK_WIN_WIDTH )
			break;
		m_Tek_cX = 0;
	case 0x0A:	// DOWN
		if ( (m_Tek_cY -= TEK_FONT_HEIGHT) < 0 )
			m_Tek_cY = TEK_WIN_HEIGHT - TekFonts[m_Tek_Font][1];
		break;
	}
}
void CTextRam::fc_TEK_FLUSH(int ch)
{
	m_Tek_cX = 0;
	TekFlush();
}
void CTextRam::fc_TEK_MODE(int ch)
{
	// 0x1B ESC
	// 0x1C PT
	// 0x1D PLT
	// 0x1E IPL
	// 0x1F ALP
	m_Tek_Mode = m_BackChar = ch;
	m_Tek_Stat = 0;
	m_Tek_Pen = 0;
}
void CTextRam::fc_TEK_STAT(int ch)
{
	int n, x, y;

	switch(m_Tek_Mode) {
	case 0x1B:	// ESC
		m_Tek_Gin = FALSE;
		switch(ch) {
		case 0x03:				// VT_MODE
			fc_POP(' ');
			TekFlush();
			break;
		case 0x05:				// REPORT
			UNGETSTR("4%c%c%c%c\r", ' ' + ((m_Tek_cX >> 7) & 0x1F), ' ' + ((m_Tek_cX >> 2) & 0x1F), ' ' + ((m_Tek_cY >> 7) & 0x1F), ' ' + ((m_Tek_cY >> 2) & 0x1F));
			break;
		case 0x0C:				// PAGE
			TekClear();
			break;
		case 0x0E:				// APL Font Set
		case 0x0F:				// ASCII Font Set
			break;
		case 0x17:				// COPY
		case 0x18:				// BYP
			break;
		case 0x1A:				// GIN
			m_Tek_Gin = TRUE;
			break;
		case 0x38: case 0x39:
		case 0x3A: case 0x3B:	// CHAR_SIZE	Font Size 0-3
			m_Tek_Font = ch & 0x03;
			break;
		case '[':				// CSI
			m_Tek_Mode = '[';
			m_BackChar = 0;
			m_AnsiPara.RemoveAll();
			return;
		case ']':				// OSC
			m_Tek_Mode = ']';
			return;
		default:
			if ( ch == '%' || (ch >= 0x40 && ch <= 0x5F) ) {
				m_Tek_Mode = '%';
				m_BackChar = (ch << 8);
				return;
			} else if ( ch >= 0x60 && ch <= 0x7F )	// beam and vector selector 0-7 * 4
				m_Tek_Line = ch - 0x60;
			break;
		}
		m_Tek_Mode = 0x1F;	// ALP
		break;

	case 0x1E:	// IPL
		m_Tek_Gin = FALSE;
		switch(ch) {
		case ' ': m_Tek_Pen = 0; return;
		case 'A':             m_Tek_cX++; break;
		case 'B':             m_Tek_cX--; break;
		case 'D': m_Tek_cY++;             break;
		case 'E': m_Tek_cY++; m_Tek_cX++; break;
		case 'F': m_Tek_cY++; m_Tek_cX--; break;
		case 'H': m_Tek_cY--;             break;
		case 'I': m_Tek_cY--; m_Tek_cX++; break;
		case 'J': m_Tek_cY--; m_Tek_cX--; break;
		case 'P': m_Tek_Pen = 1; return;
		}
		if ( m_Tek_Pen )
			TekLine(m_Tek_Line, m_Tek_lX, m_Tek_lY, m_Tek_cX, m_Tek_cY);
		m_Tek_lX = m_Tek_cX; m_Tek_lY = m_Tek_cY;
		break;

	case 0x1F:	// ALP
		m_Tek_Gin = FALSE;
		n = 0;
		switch(m_KanjiMode) {
		case EUC_SET:
		case ASCII_SET:
			switch(m_Tek_Stat) {
			case 0:
				if ( ch >= ' ' && ch <= 0x7F || ch >= 0x80 && ch <= 0x8D || ch >= 0x90 && ch <= 0x9F ) {
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				} else if ( ch == 0x8E ) {
					m_Tek_Stat = 2;
				} else if ( ch == 0x8F ) {
					m_Tek_Stat = 3;
				} else if ( ch >= 0xA0 && ch <= 0xFF ) {
					m_BackChar = ch << 8;
					m_Tek_Stat = 1;
				}
				break;
			case 1:
				ch |= m_BackChar;
				ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], "UTF-16BE", ch);
				TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
				n = 2;
				m_Tek_Stat = 0;
				break;
			case 2:
				ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], "UTF-16BE", ch | 0x80);
				TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
				n = 1;
				m_Tek_Stat = 0;
				break;
			case 3:
				m_BackChar = ch << 8;
				m_Tek_Stat = 1;
				break;
			}
			break;
		case SJIS_SET:
			switch(m_Tek_Stat) {
			case 0:
				if ( ch >= ' ' && ch <= 0x7F ) {
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				} else if ( issjis1(ch) ) {
					m_BackChar = ch << 8;
					m_Tek_Stat = 1;
				} else if ( iskana(ch) ) {
					ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], "UTF-16BE", ch);
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				}
				break;
			case 1:
				if ( issjis2(ch) ) {
					ch |= m_BackChar;
					ch = m_IConv.IConvChar(m_SendCharSet[m_KanjiMode], "UTF-16BE", ch);
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 2;
				}
				m_Tek_Stat = 0;
				break;
			}
			break;
		case UTF8_SET:
			switch(m_Tek_Stat) {
			case 0:
				if ( ch >= ' ' && ch <= 0x7F ) {
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 1;
				} else if ( ch >= 0xC0 && ch <= 0xDF ) {
					m_BackChar = (ch & 0x1F) << 6;
					m_Tek_Stat = 3;
				} else if ( ch >= 0xE0 && ch <= 0xEF ) {
					m_BackChar = (ch & 0x0F) << 12;
					m_Tek_Stat = 2;
				} else if ( ch >= 0xF0 && ch <= 0xF7 ) {
					m_BackChar = (ch & 0x07) << 18;
					m_Tek_Stat = 1;
				}
				break;
			case 1:
				if ( ch >= 0x80 && ch <= 0xBF ) {
					m_BackChar |= (ch & 0x3F) << 12;
					m_Tek_Stat = 2;
				} else
					m_Tek_Stat = 0;
				break;
			case 2:
				if ( ch >= 0x80 && ch <= 0xBF ) {
					m_BackChar |= (ch & 0x3F) << 6;
					m_Tek_Stat = 3;
				} else
					m_Tek_Stat = 0;
				break;
			case 3:
				if ( ch >= 0x80 && ch <= 0xBF ) {
					ch = m_BackChar | (ch & 0x3F);
					ch = m_IConv.IConvChar("UCS-4BE", "UTF-16BE", ch);
					TekText(m_Tek_Font, m_Tek_cX, m_Tek_cY, ch);
					n = 2;
				}
				m_Tek_Stat = 0;
				break;
			}
			break;
		}
		if ( n > 0 ) {
			x = (TEK_FONT_WIDTH * n * SinTab[m_Tek_Angle / 5] / 1000);
			y = 0 - (TEK_FONT_WIDTH * n * SinTab[(m_Tek_Angle + 90) / 5] / 1000);

			m_Tek_cX += x;
			m_Tek_cY += y;

			if ( m_Tek_cX < 0 ) {
				m_Tek_cX += TEK_WIN_WIDTH;
				if ( (m_Tek_cY += y) >= TEK_WIN_HEIGHT )
					m_Tek_cY -= TEK_WIN_HEIGHT;
			} else if ( m_Tek_cX >= TEK_WIN_WIDTH ) {
				m_Tek_cX -= TEK_WIN_WIDTH;
				if ( (m_Tek_cY -= y) < 0 )
					m_Tek_cY += TEK_WIN_HEIGHT;
			}

			if ( m_Tek_cY < 0 ) {
				m_Tek_cY += TEK_WIN_HEIGHT;
				if ( (m_Tek_cX -= x) < 0 )
					m_Tek_cX += TEK_WIN_WIDTH;
			} else if ( m_Tek_cY >= TEK_WIN_HEIGHT ) {
				m_Tek_cY -= TEK_WIN_HEIGHT;
				if ( (m_Tek_cX += x) >= TEK_WIN_HEIGHT )
					m_Tek_cX -= TEK_WIN_WIDTH;
			}
		}
		break;

	case '[':	// CSI
		if ( ch >= 0x20 && ch <= 0x2F ) {
			m_BackChar &= 0xFF0000;
			m_BackChar |= (ch << 8);
		} else if ( ch >= 0x30 && ch <= 0x39 ) {
			if ( m_AnsiPara.GetSize() <= 0 )
				m_AnsiPara.Add(0);
			n = (int)m_AnsiPara.GetSize() - 1;
			m_AnsiPara[n] = m_AnsiPara[n] * 10 + ((ch & 0x7F) - '0');
		} else if ( ch == 0x3B ) {
			m_AnsiPara.Add(0);
		} else if ( ch >= 0x3C && ch <= 0x3F ) {
			m_BackChar = (ch << 16);
		} else if ( ch >= 0x40 ) {
			switch(m_BackChar | (ch & 0x7F)) {
			case ('?' << 16) | 'l':
				for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
					switch(GetAnsiPara(n, 0)) {
					case 38:	
						fc_POP(' ');
						TekFlush();
						break;
					}
				}
				break;
			case 'm':		// special color linetypes for MS-DOS Kermit v2.31 tektronix emulator
				for ( n = 0 ; n < m_AnsiPara.GetSize() ; n++ ) {
					switch(GetAnsiPara(n, 0)) {
					case 0:
						m_Tek_Line &= 0xBF;
						break;
					case 1:
						m_Tek_Line |= 0x40;
						break;
					case 30: case 31: case 32: case 33:
					case 34: case 35: case 36: case 37:
						m_Tek_Line = (m_Tek_Line & 0xC7) | ((GetAnsiPara(n, 0) & 0x07) << 3);
						break;
					}
				}
				break;
			}
			m_Tek_Mode = 0x1F;	// ALP
		}
		break;

	case ']':	// OSC
		if ( ch == 0x07 )
			m_Tek_Mode = 0x1F;	// ALP
		break;

	case '%':
		if ( ch == '%' )
			break;
		if ( ch == '!' || (ch >= 'A' && ch <= 'Z') )
			m_BackChar |= ch;

		m_Tek_Mode = 'A';
		m_Tek_Int = m_Tek_Pos = 0;
		m_Tek_Gin = FALSE;
		goto DOESC;

	case 'A':
		if ( ch >= 0x20 && ch <= 0x3F ) {
			m_Tek_Int = (m_Tek_Int << 4) | (ch & 0x0F);
			if ( (ch & 0x10) == 0 )
				m_Tek_Int = 0 - m_Tek_Int;
			m_Tek_Pos++;
		} else if ( ch >= 0x40 && ch <= 0x7F ) {
			m_Tek_Int = (m_Tek_Int << 6) | (ch & 0x3F);
			break;
		} else {
			m_Tek_Mode = 0x1F;	// ALP
			break;
		}

	DOESC:
		switch(m_BackChar) {
		case ('%' << 8) | '!':	// set mode 0=tek 1=ansi
			if ( m_Tek_Pos == 1 && m_Tek_Int == 1 ) {
				if ( m_pTekWnd != NULL && !m_pTekWnd->IsWindowVisible() )
					TekClose();
				fc_POP(' ');
				TekFlush();
			}
			break;

		case ('L' << 8) | 'Z':	// clear the dialog buffer
			TekFlush();
			break;

		case ('L' << 8) | 'G':	// vector
		case ('L' << 8) | 'F':	// move
		case ('L' << 8) | 'H':	// point
			m_Tek_Mode = 0x1D;
			break;
		case ('L' << 8) | 'T':	// string
			if ( m_Tek_Pos == 1 )
				m_Tek_Mode = 0x1F;	// ALP
			break;

		case ('L' << 8) | 'V':	// set dialog area 0=invisible 1=visible
			if ( m_Tek_Pos != 1 )
				break;
			if ( m_pTekWnd == NULL )
				TekInit(4105);
			if ( !m_pTekWnd->IsWindowVisible() && m_Tek_Int != 0 )
				m_pTekWnd->ShowWindow(SW_SHOW);
			break;

		case ('M' << 8) | 'C':	// set character size to 59 point height
			if ( m_Tek_Pos == 2 ) {		// Height
				n = m_Tek_Int * 14 / 10;
				if ( n >= TekFonts[0][1] )
					m_Tek_Font = 0;
				else if ( n >= TekFonts[1][1] )
					m_Tek_Font = 1;
				else if ( n >= TekFonts[2][1] )
					m_Tek_Font = 2;
				else
					m_Tek_Font = 3;
			}
			break;

		case ('M' << 8) | 'G':	// set character write mode to overstrike
			break;
		case ('M' << 8) | 'L':	// set line index (color)
			if ( m_Tek_Pos == 1 )
				m_Tek_Line = (m_Tek_Line & 0x87) | ((m_Tek_Int & 0x0F) << 3);
			break;
		case ('M' << 8) | 'M':	// set point size
			if ( m_Tek_Pos == 1 )
				m_Tek_Point = m_Tek_Int % 11;
			break;
		case ('M' << 8) | 'N':	// set character path to 0 (characters placed equal to rotation)
			if ( m_Tek_Pos == 1 )
				m_Tek_Base = m_Tek_Int;
			break;
		case ('M' << 8) | 'Q':	// set character precision to string
			break;

		case ('M' << 8) | 'R':	// set string angle
			if ( m_Tek_Pos == 1 ) {
				m_Tek_Angle = m_Tek_Int % 360;
				if ( m_Tek_Angle < 0 )
					m_Tek_Angle += 360;
			}
			break;

		case ('M' << 8) | 'T':	// set character text index to 1
			break;
		case ('M' << 8) | 'V':	// set line style
			if ( m_Tek_Pos == 1 )
				m_Tek_Line = (m_Tek_Line & 0xF8) | m_Tek_Int & 0x07;
			break;
		case ('R' << 8) | 'K':	// clear the view
		case ('S' << 8) | 'K':	// clear the segments
			TekClear();
			break;

		default:
			TRACE("Tek410x %c%c(%d:%d)\n", m_BackChar >> 8, m_BackChar & 0xFF, m_Tek_Int, m_Tek_Pos); 
			break;
		}

		m_Tek_Int = 0;
		break;

	case 0x1C:	// PT
	case 0x1D:	// PLT
		m_Tek_Gin = FALSE;
		if ( ch >= 0x40 && ch <= 0x5F ) {
			m_Tek_wX = (m_Tek_wX & 0xF83) | ((ch & 0x1F) << 2);
			m_Tek_cX = m_Tek_wX; m_Tek_cY = m_Tek_wY;
		} else if ( ch >= 0x20 && ch <= 0x3F ) {
			if ( m_Tek_Stat )
				m_Tek_wX = (m_Tek_wX & 0x7F) | ((ch & 0x1F) << 7);
			else
				m_Tek_wY = (m_Tek_wY & 0x7F) | ((ch & 0x1F) << 7);
			break;
		} else if ( ch >= 0x60 && ch <= 0x7F ) {
			if ( m_Tek_Stat ) {
				m_Tek_wX = (m_Tek_wX & 0xFFC) | ((m_Tek_wY >> 2) & 0x03);
				m_Tek_wY = (m_Tek_wY & 0xFFC) | ((m_Tek_wY >> 4) & 0x03);
			}
			m_Tek_wY = (m_Tek_wY & 0xF83) | ((ch & 0x1F) << 2);
			m_Tek_Stat = 1;
			break;
		} else if ( ch == 0x07 ) {
			m_Tek_Pen = 1;
			break;
		} else {
			break;
		}

		switch(m_BackChar) {
		case 0x1C:
			TekLine(m_Tek_Line, m_Tek_cX, m_Tek_cY, m_Tek_cX, m_Tek_cY);
			m_Tek_lX = m_Tek_cX; m_Tek_lY = m_Tek_cY;
			m_Tek_Stat = 0;
			m_Tek_Pen = 1;
			break;
		case 0x1D:
			if ( m_Tek_Pen )
				TekLine(m_Tek_Line, m_Tek_lX, m_Tek_lY, m_Tek_cX, m_Tek_cY);
			m_Tek_lX = m_Tek_cX; m_Tek_lY = m_Tek_cY;
			m_Tek_Stat = 0;
			m_Tek_Pen = 1;
			break;

		case ('L' << 8) | 'F':	// move
			break;
		case ('L' << 8) | 'G':	// vector
			TekLine(m_Tek_Line, m_Tek_lX, m_Tek_lY, m_Tek_cX, m_Tek_cY);
			break;
		case ('L' << 8) | 'H':	// point
			TekText(2, m_Tek_cX - TekFonts[2][0] / 4, m_Tek_cY - TekFonts[2][1] / 2, TekPoint[m_Tek_Point]);
			break;
		}

		m_Tek_lX = m_Tek_cX; m_Tek_lY = m_Tek_cY;
		m_Tek_Stat = 0;
		break;
	}
}
