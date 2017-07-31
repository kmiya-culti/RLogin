// TextRam.h: CTextRam クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEXTRAM_H__CBEA227A_D7D7_4213_88B1_4F4C0DF48089__INCLUDED_)
#define AFX_TEXTRAM_H__CBEA227A_D7D7_4213_88B1_4F4C0DF48089__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
#include <afxmt.h>
#include "Data.h"
#include "IConv.h"
#include "TekWnd.h"
#include "GhostWnd.h"

#define	COLS_MAX		200
#define	LINE_MAX		500
#define	DEF_TAB			8

#define	TEK_WIN_WIDTH	4096
#define	TEK_WIN_HEIGHT	3072

#define	EUC_SET			0
#define	SJIS_SET		1
#define	ASCII_SET		2
#define	UTF8_SET		3

#define ATT_BOLD		0x000001		// [1m bold or increased intensity
#define	ATT_HALF		0x000002		// [2m faint, decreased intensity or second colour
#define ATT_ITALIC		0x000004		// [3m italicized
#define ATT_UNDER		0x000008		// [4m singly underlined
#define ATT_SBLINK		0x000010		// [5m slowly blinking
#define ATT_REVS		0x000020		// [7m negative image
#define ATT_SECRET		0x000040		// [8m concealed characters
#define ATT_LINE		0x000080		// [9m crossed-out
#define ATT_BLINK		0x000100		// [6m rapidly blinking 
#define ATT_DUNDER		0x000200		// [21m doubly underlined
#define ATT_FRAME		0x000400		// [51m framed
#define ATT_CIRCLE		0x000800		// [52m encircled
#define ATT_OVER		0x001000		// [53m overlined
#define ATT_RSLINE		0x002000		// [60m right side line
#define ATT_RDLINE		0x004000		// [61m double line on the right side
#define ATT_LSLINE		0x008000		// [62m left side line
#define ATT_LDLINE		0x010000		// [63m double line on the left side
#define ATT_STRESS		0x020000		// [64m stress marking
#define	ATT_CLIP		0x800000		// Mouse Clip

#define CODE_MAX		0x0400
#define CODE_MASK		0x03FF
#define SET_MASK		0x0300
#define SET_94			0x0000
#define SET_96			0x0100
#define SET_94x94		0x0200
#define SET_96x96		0x0300
#define	SET_UNICODE		0x03FF

#define	EM_PROTECT		1
#define	EM_SELECTED		2

#define DM_SWSH			0
#define DM_DWSH			1
#define DM_DWDHT		2
#define DM_DWDHB		3

#define CM_ASCII		0
#define CM_1BYTE		1
#define CM_2BYTE		2
#define	CM_RESV			3

#define IS_ASCII(a) 	((a) == CM_ASCII)
#define IS_1BYTE(a) 	((a) == CM_1BYTE)
#define IS_2BYTE(a) 	((a) == CM_2BYTE)

// DEC Terminal Option	0-199
#define	TO_DECCKM		1			// Cursor key mode
#define	TO_DECANM		2			// ANSI/VT52 Mode
#define	TO_DECCOLM		3			// 80/132 Column mode
#define	TO_DECSCNM		5			// Light or Dark Screen
#define TO_DECOM		6			// Origin mode
#define TO_DECAWM		7			// Autowrap mode
#define	TO_XTMOSREP		9			// X10 mouse reporting
#define	TO_DECTCEM		25			// Text Cursor Enable Mode
#define	TO_DECTEK		38			// Graphics (Tek)
#define	TO_XTMCSC		40			// XTerm Column switch control
#define TO_XTMRVW		45			// XTerm Reverse-wraparound mode
#define	TO_XTMABUF		47			// XTerm alternate buffer
#define	TO_DECECM		117			// SGR space color disable
// ANSI Screen Option	0-99(200-299)
#define	TO_ANSIIRM		(4+200)		// IRM Insertion replacement mode
#define	TO_ANSIERM		(6+200)		// ERM Erasure mode
#define	TO_ANSITSM		(18+200)	// ISM Tabulation stop mode
#define	TO_ANSILNM		(20+200)	// LNM Line feed/new line mode
// XTerm Option			1000-1099(300-399)
#define	TO_XTNOMTRK		(1000-700)	// X11 normal mouse tracking
#define	TO_XTHILTRK		(1001-700)	// X11 hilite mouse tracking
#define	TO_XTBEVTRK		(1002-700)	// X11 button-event mouse tracking
#define	TO_XTAEVTRK		(1003-700)	// X11 any-event mouse tracking
#define	TO_XTALTSCR		(1047-700)	// Alternate/Normal screen buffer
#define	TO_XTSRCUR		(1048-700)	// Save/Restore cursor as in DECSC/DECRC
#define	TO_XTALTCLR		(1049-700)	// Alternate screen with clearing
// RLogin Option		400-511
#define	TO_RLGCWA		400			// ESC[m space att enable
#define	TO_RLGNDW		401			// 行末での自動改行を有効にする
#define	TO_RLGAWL		402			// http;//で始まる文字列をコピーすると自動でブラウザを起動する
#define	TO_RLBOLD		403			// ボールド文字を有効にする
#define	TO_RLFONT		404			// フォントサイズから一行あたりの文字数を決定
#define	TO_RLBPLUS		405			// BPlus/ZModemファイル転送を有効にする
#define	TO_RLTENAT		406			// 自動ユーザー認証を行わない
#define	TO_RLTENEC		407			// データの暗号化を禁止する
#define	TO_RLPOFF		408			// 接続が切れると自動でプログラムを終了する
#define	TO_RLUSEPASS	409			// 接続時にパスワード入力を求める
#define	TO_RLRCLICK		410			// 右ダブルクリックだけでクリップボードからペーストする
#define	TO_RLCKCOPY		411			// 左クリックの範囲指定だけでクリップボードにコピーする
#define	TO_RLDSECHO		412			// キーボード入力をローカルエディットモードにする
#define	TO_RLDELAY		413			// 改行(CR)を確認し指定時間待って次を送信する(ms)
#define	TO_RLHISFILE	414			// ヒストリーを保存し次接続時に復元する
#define	TO_RLKEEPAL		415			// TCPオプションのKeepAliveを有効にする
#define	TO_RLADBELL		416			// ベルコード(07)による動作を選択 x1
#define	TO_RLVSBELL		417			// ベルコード(07)による動作を選択 1x
#define	TO_RLECHOCR		418			// 送信する改行コードの設定 x1
#define	TO_RLECHOLF		419			// 送信する改行コードの設定 1x
#define	TO_SSH1MODE		420			// SSHバージョン１で接続する
#define	TO_SSHPFORY		421			// ポートフォワードだけ行う （SSH2のみ）
#define	TO_SSHKEEPAL	422			// KeepAliveパケットの送信間隔(sec)
#define	TO_SSHAGENT		423			// エージェント転送を有効にする(SSH2のみ)
#define	TO_SSHSFENC		424			// 暗号シャッフル
#define	TO_SSHSFMAC		425			// 検証シャッフル
#define	TO_RLRECVCR		426			// 受信した改行コードの動作を設定 x1
#define	TO_RLRECVLF		427			// 受信した改行コードの動作を設定 1x
#define	TO_RLUNIAWH		428			// UnicodeのAタイプの文字を半角として表示する
#define	TO_RLSPCTAB		429			// 選択した文字列から連続したスペースをタブに変換する
#define	TO_RLHISDATE	430			// 通信ログを年月日を付けて自動作成する
#define	TO_RLLOGMODE	431			// ファイルに保存する通信ログの形式を選択 x1
#define	TO_RLLOGMOD2	432			// ファイルに保存する通信ログの形式を選択 1x
#define	TO_RLLOGCODE	433			// ログの文字コードを選択(RAWモード以外で有効) x1
#define	TO_RLLOGCOD2	434			// ログの文字コードを選択(RAWモード以外で有効) 1x
#define	TO_RLNORESZ		435			// DECCOLMの文字数切換でウィンドウをリサイズする
#define	TO_SSHX11PF		436			// X11ポートフォワードを使用する
#define	TO_RLKANAUTO	437			// 漢字コードを自動で追従する
#define	TO_RLMOSWHL		438			// マウスホイールをヌルヌル動かす

#define	IS_ENABLE(p,n)	(p[(n) / 32] & (1 << ((n) % 32)))

enum ETextRamStat {
		ST_NON,
		ST_CONF_LEVEL,
		ST_COLM_SET,	ST_COLM_SET_2,
		ST_DEC_OPT,
		ST_DOCS_OPT,	ST_DOCS_OPT_2,
		ST_TEK_OPT,
		ST_CHARSET_1,	ST_CHARSET_2,
		ST_CSI_SKIP,
};

enum ETabSetNum {
		TAB_COLSSET,	TAB_COLSCLR,	TAB_COLSALLCLR,	TAB_COLSALLCLRACT,
		TAB_LINESET,	TAB_LINECLR,	TAB_LINEALLCLR,
		TAB_RESET,
		TAB_DELINE,		TAB_INSLINE,
		TAB_ALLCLR,
		TAB_COLSNEXT,	TAB_COLSBACK,
		TAB_LINENEXT,	TAB_LINEBACK,
};

enum EStageNum {
		STAGE_ESC = 0,	STAGE_CSI,		STAGE_EXT1,		STAGE_EXT2,		STAGE_EXT3,		// Use 0,1,2,3,4 !!! Look up fc_Case()
		STAGE_EXT4,		STAGE_EUC,
		STAGE_94X94,	STAGE_96X96,
		STAGE_SJIS,		STAGE_SJIS2,
		STAGE_UTF8,		STAGE_UTF82,
		STAGE_OSC1,		STAGE_OSC2,		STAGE_OSC3,		STAGE_OSC4,
		STAGE_TEK,
		STAGE_STAT,
		STAGE_MAX,
};

#define	UTF16BE			0
#define	UTF16LE			1

#define	RESET_CURSOR	0x0001
#define	RESET_TABS		0x0002
#define	RESET_BANK		0x0004
#define	RESET_ATTR		0x0008
#define	RESET_COLOR		0x0010
#define	RESET_TEK		0x0020
#define	RESET_SAVE		0x0040
#define	RESET_MOUSE		0x0080
#define	RESET_CHAR		0x0100
#define	RESET_OPTION	0x0200
#define	RESET_CLS		0x0400

#define issjis1(c)		(((unsigned char)(c) >= 0x81 && \
						  (unsigned char)(c) <= 0x9F) || \
						 ((unsigned char)(c) >= 0xE0 && \
						  (unsigned char)(c) <= 0xFC) )

#define issjis2(c)		(((unsigned char)(c) >= 0x40 && \
						  (unsigned char)(c) <= 0x7E) || \
						 ((unsigned char)(c) >= 0x80 && \
						  (unsigned char)(c) <= 0xFC) )

#define iskana(c)		((unsigned char)(c) >= 0xA0 && \
						 (unsigned char)(c) <= 0xDF)

typedef	struct _Vram {
	DWORD	ch;
	DWORD	at:28;
	  DWORD	ft:4;
	WORD	md:10;
	  WORD	em:2;
	  WORD	dm:2;
	  WORD	cm:2;
	BYTE	fc;
	BYTE	bc;
} VRAM;

class CFontNode : public CObject
{
public:
	BYTE m_Shift;
	int m_ZoomW;
	int m_ZoomH;
	int m_Offset;
	int m_CharSet;
	CString m_FontName[16];
	CString m_EntryName;
	CString m_IContName;
	int m_Hash[16];
	int m_Quality;

	void SetHash(int num);
	void Init();
	void SetArray(CStringArrayExt &array);
	void GetArray(CStringArrayExt &array);
	CFontChacheNode *GetFont(int Width, int Height, int Style, int FontNum);
	const CFontNode & operator = (CFontNode &data);
	CFontNode();
};

class CFontTab : public COptObject
{
public:
	CFontNode m_Data[CODE_MAX];

	void Init();
	void SetArray(CStringArrayExt &array);
	void GetArray(CStringArrayExt &array);

	int Find(LPCSTR entry);
	inline CFontNode & operator[](int nIndex) { return m_Data[nIndex]; }
	const CFontTab & operator = (CFontTab &data);
	CFontTab();
};

class CProcNode : public CObject
{
public:
	int		m_Type;
	int		m_Code;
	CString	m_Name;

	const CProcNode & operator = (CProcNode &data);
	CProcNode();
};

class CProcTab : public COptObject
{
public:
	CArray<CProcNode, CProcNode &> m_Data;

	void Add(int type, int code, LPCSTR name);
	inline void RemoveAll() { m_Data.RemoveAll(); }
	inline int GetSize() { return m_Data.GetSize(); }
	inline CProcNode & operator[](int nIndex) { return m_Data[nIndex]; }

	void Init();
	void SetArray(CStringArrayExt &array);
	void GetArray(CStringArrayExt &array);

	const CProcTab & operator = (CProcTab &data);
	CProcTab();
};

class CTextSave : public CObject
{
public:
	class CTextSave *m_Next;

	VRAM *m_VRam;
	VRAM m_AttNow;
	VRAM m_AttSpc;

	int m_Cols;
	int m_Lines;
	int m_CurX;
	int m_CurY;
	int m_TopY;
	int m_BtmY;

	int m_DoWarp;
	DWORD m_AnsiOpt[16];

	int m_BankGL;
	int m_BankGR;
	int m_BankSG;
	WORD m_BankTab[4][4];

	BYTE m_TabMap[LINE_MAX][COLS_MAX / 8 + 1];

	VRAM m_Save_AttNow;
	VRAM m_Save_AttSpc;

	int m_Save_CurX;
	int m_Save_CurY;

	BOOL m_Save_DoWarp;
	DWORD m_Save_AnsiOpt[16];

	int m_Save_BankGL;
	int m_Save_BankGR;
	int m_Save_BankSG;
	WORD m_Save_BankTab[4][4];

	BYTE m_Save_TabMap[LINE_MAX][COLS_MAX / 8 + 1];
};

class CTextRam : public COptObject
{
public:	// Options
	COLORREF m_DefColTab[16];
	CString m_SendCharSet[4];
	DWORD m_DefAnsiOpt[16];
	int m_DefCols[2];
	int m_DefHisMax;
	int m_DefFontSize;
	VRAM m_DefAtt;
	int m_KanjiMode;
	int m_BankGL;
	int m_BankGR;
	WORD m_DefBankTab[4][4];
	int m_WheelSize;
	CString m_BitMapFile;
	int m_DelayMSec;
	CString m_HisFile;
	int m_KeepAliveSec;
	CString m_LogFile;
	int m_DropFileMode;
	CString m_DropFileCmd[8];
	CString m_WordStr;
	int m_MouseTrack;
	BYTE m_MouseMode[4];
	DWORD m_MetaKeys[256 / 32];
	CProcTab m_ProcTab;

	void Init();
	void SetArray(CStringArrayExt &array);
	void GetArray(CStringArrayExt &array);
	void Serialize(int mode);
	void Serialize(int mode, CBuffer &buf);
	void Serialize(CArchive& ar);
	const CTextRam & operator = (CTextRam &data);

public:
	class CRLoginDoc *m_pDocument;
	class CFontTab m_FontTab;

	VRAM *m_VRam;
	VRAM m_AttSpc;
	VRAM m_AttNow;

	int m_Cols;
	int m_Lines;
	int m_CurX;
	int m_CurY;
	int m_TopY;
	int m_BtmY;
	int m_DefTab;
	COLORREF m_ColTab[256];
	DWORD m_AnsiOpt[16];

	int m_ColsMax;
	int m_LineUpdate;

	int m_HisMax;
	int m_HisPos;
	int m_HisLen;
	int m_HisUse;
	CFile m_HisFhd;

	int m_DispCaret;
	BOOL m_DoWarp;
	int m_Status;
	int m_RecvCrLf;
	int m_SendCrLf;

	int m_BackChar;
	int m_BackMode;
	int m_LastChar;
	int m_LastPos;

	CWordArray m_AnsiPara;
	BOOL m_OscFlag;
	int m_OscMode;
	CStringW m_OscPara;
	BYTE m_TabMap[LINE_MAX][COLS_MAX / 8 + 1];
	BOOL m_RetSync;
	BOOL m_Exact;

	WORD m_BankTab[4][4];
	int m_BankNow;
	int m_BankSG;
	int m_CodeLen;

	VRAM m_Save_AttNow;
	VRAM m_Save_AttSpc;
	int m_Save_CurX;
	int m_Save_CurY;
	DWORD m_Save_AnsiOpt[16];
	int m_Save_BankGL;
	int m_Save_BankGR;
	int m_Save_BankSG;
	BOOL m_Save_DoWarp;
	WORD m_Save_BankTab[4][4];
	BYTE m_Save_TabMap[LINE_MAX][COLS_MAX / 8 + 1];

	CTextSave *m_pTextSave;
	CTextSave *m_pTextStack;
	CIConv m_IConv;
	CRect m_UpdateRect;

	BOOL m_LineEditMode;
	int m_LineEditIndex;
	int m_LineEditX, m_LineEditY;
	int m_LineEditPos;
	CBuffer m_LineEditBuff;
	int m_LineEditHisPos;
	CSpace m_LineEditNow;
	CArray<CSpace, CSpace &> m_LineEditHis;
	int m_LineEditMapsPos;
	int m_LineEditMapsTop;
	CStringW m_LineEditMapsDir;
	CStringW m_LineEditMapsStr;
	int m_LineEditMapsInit;
	int m_LineEditMapsIndex;
	CStringMaps m_LineEditMapsTab[3];
	int m_LogCharSet[4];
	
	// Window Fonction
	BOOL IsInitText() { return (m_VRam == NULL ? FALSE : TRUE); }
	void InitText(int Width, int Height);
	void InitCols();
	int Write(LPBYTE lpBuf, int nBufLen);
	int LineEdit(CBuffer &buf);
	void LineEditEcho();
	void LineEditCwd(int sx, int sy, CStringW &cwd);
	void SetKanjiMode(int mode);

	void OpenHisFile();
	void InitHistory();
	void SaveHistory();

//	CRegEx	m_RegWord[4];
//	void HisKeyWord();

	struct DrawWork {
		int		att;
		int		fcn, bcn;
		int		mod, fnm;
		int		dmf, csz;
	};

	int IsWord(WCHAR ch);
	int GetPos(int x, int y);
	BOOL IncPos(int &x, int &y);
	BOOL DecPos(int &x, int &y);
	void EditWordPos(int *sps, int *eps);
	void EditCopy(int sps, int eps, BOOL rectflag = FALSE);
	void StrOut(CDC* pDC, LPCRECT pRect, struct DrawWork &prop, int len, char *str, int sln, int *spc, class CRLoginView *pView);
	void DrawVram(CDC *pDC, int x1, int y1, int x2, int y2, class CRLoginView *pView);
	
	CWnd *GetAciveView();
	void PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

	BOOL IsOptEnable(int opt);
	void EnableOption(int opt);
	void DisableOption(int opt);
	void ReversOption(int opt);
	int IsOptValue(int opt, int len);
	void SetOptValue(int opt, int len, int value);

	inline int GetCalcPos(int x, int y) { return (m_Cols * (m_HisMax + y) + x); }
	inline void SetCalcPos(int pos, int *x, int *y) { *x = pos % m_Cols; *y = (pos / m_Cols - m_HisMax); }
	inline int GetDm(int y) { VRAM *vp = GETVRAM(0, y); return vp->dm; }
	inline void SetDm(int y, int dm) { VRAM *vp = GETVRAM(0, y); vp->dm = dm; }

	void OnClose();
	void CallReciveLine(int y);
	void CallReciveChar(int ch);
	int UnicodeWidth(DWORD code);

	// Static Lib
	static void CTextRam::MsToIconvUnicode(WCHAR *str, int len, LPCSTR charset);
	static DWORD CTextRam::IconvToMsUnicode(DWORD code);
	static DWORD CTextRam::UnicodeNomal(DWORD code1, DWORD code2);

	// Low Level
	void RESET(int mode = RESET_CURSOR | RESET_TABS | RESET_BANK | RESET_ATTR | RESET_COLOR | RESET_TEK | RESET_SAVE | RESET_MOUSE | RESET_CHAR);
	VRAM *GETVRAM(int cols, int lines);
	void UNGETSTR(LPCSTR str, ...);
	void BEEP();
	void FLUSH();
	void CUROFF();
	void CURON();
	void DISPVRAM(int sx, int sy, int w, int h);
	void DISPUPDATE();
	int BLINKUPDATE(class CRLoginView *pView);

	// Mid Level
	int GetAnsiPara(int index, int defvalue);
	void SetAnsiParaArea(int top);
	void LOCATE(int x, int y);
	void ERABOX(int sx, int sy, int ex, int ey, int df = 0);
	void FORSCROLL(int sy, int ey);
	void BAKSCROLL(int sy, int ey);
	void LEFTSCROLL();
	void RIGHTSCROLL();
	void DOWARP();
	void INSCHAR();
	void DELCHAR();
	void ONEINDEX();
	void REVINDEX();
	void PUT1BYTE(int ch, int md);
	void PUT2BYTE(int ch, int md);
	void INSMDCK(int len);
	void ANSIOPT(int opt, int bit);
	void SAVERAM();
	void LOADRAM();
	void PUSHRAM();
	void POPRAM();
	void TABSET(int sw);
	void PUTSTR(LPBYTE lpBuf, int nBufLen);

	CTextRam();
	virtual ~CTextRam();

	typedef struct _PROCTAB {
		BYTE	sc, ec;
		void (CTextRam::*proc)(int ch);
	} PROCTAB;

	typedef struct _CSIEXTTAB {
		int		code;
		void (CTextRam::*proc)(int ch);
	} CSIEXTTAB;

	typedef struct _ESCNAMEPROC {
		LPCSTR		name;
		union {
			void (CTextRam::*proc)(int ch);
			BYTE byte[sizeof(void (CTextRam::*)(int))];
		} data;
		struct _ESCNAMEPROC	*left;
		struct _ESCNAMEPROC	*right;
	} ESCNAMEPROC;

	void ((CTextRam::**m_Func)(int ch));
	int m_Stage;
	int m_StPos;
	int m_Stack[16];
	void (CTextRam::*m_LocalProc[5][256])(int ch);
	CArray<CTextRam::CSIEXTTAB, CTextRam::CSIEXTTAB &> m_CsiExt;

	void fc_Init_Proc(int stage, const PROCTAB *tp, int b = 0);
	ESCNAMEPROC *fc_InitProcName(CTextRam::ESCNAMEPROC *tab, int *max);
	void fc_Init(int mode);
	inline void fc_Call(int ch) { (this->*m_Func[ch])(ch); }
	inline void fc_Case(int stage);
	inline void fc_Push(int stage);

	void EscNameProc(int ch, LPCSTR name);
	LPCSTR EscProcName(void (CTextRam::*proc)(int ch));
	void SetEscNameCombo(CComboBox *pCombo);

	void CsiNameProc(int code, LPCSTR name);
	LPCSTR CsiProcName(void (CTextRam::*proc)(int ch));
	void SetCsiNameCombo(CComboBox *pCombo);

	void EscCsiDefName(LPCSTR *esc, LPCSTR *csi);

#define	KANBUFMAX		128

	int m_Kan_Pos;
	BYTE m_Kan_Buf[KANBUFMAX];
	
	void fc_KANCHK();
	inline void fc_KANJI(int ch) { if ( ch >= 128 || m_Kan_Buf[(m_Kan_Pos - 1) & (KANBUFMAX - 1)] >= 128 ) { m_Kan_Buf[m_Kan_Pos++] = ch; m_Kan_Pos &= (KANBUFMAX - 1); } }

	// Print...
	void fc_IGNORE(int ch);
	void fc_POP(int ch);
	void fc_RETRY(int ch);
	void fc_TEXT(int ch);
	void fc_TEXT2(int ch);
	void fc_TEXT3(int ch);
	void fc_SJIS1(int ch);
	void fc_SJIS2(int ch);
	void fc_SJIS3(int ch);
	void fc_UTF81(int ch);
	void fc_UTF82(int ch);
	void fc_UTF83(int ch);
	void fc_UTF84(int ch);
	void fc_UTF85(int ch);
	void fc_UTF86(int ch);
	void fc_UTF87(int ch);
	void fc_UTF88(int ch);
	void fc_UTF89(int ch);
	void fc_SESC(int ch);
	void fc_STAT(int ch);

	// Ctrl...
	void fc_SOH(int ch);
	void fc_ENQ(int ch);
	void fc_BEL(int ch);
	void fc_BS(int ch);
	void fc_HT(int ch);
	void fc_LF(int ch);
	void fc_VT(int ch);
	void fc_FF(int ch);
	void fc_CR(int ch);
	void fc_SO(int ch);
	void fc_SI(int ch);
	void fc_DLE(int ch);
	void fc_CAN(int ch);
	void fc_ESC(int ch);
	void fc_A3CUP(int ch);
	void fc_A3CDW(int ch);
	void fc_A3CLT(int ch);
	void fc_A3CRT(int ch);

	// ESC...
	void fc_DECHTS(int ch);
	void fc_CCAHT(int ch);
	void fc_DECVTS(int ch);
	void fc_CAVT(int ch);
	void fc_BI(int ch);
	void fc_SC(int ch);
	void fc_RC(int ch);
	void fc_FI(int ch);
	void fc_V5CUP(int ch);
	void fc_BPH(int ch);
	void fc_NBH(int ch);
	void fc_IND(int ch);
	void fc_NEL(int ch);
	void fc_SSA(int ch);
	void fc_ESA(int ch);
	void fc_HTS(int ch);
	void fc_HTJ(int ch);
	void fc_VTS(int ch);
	void fc_PLD(int ch);
	void fc_PLU(int ch);
	void fc_RI(int ch);
	void fc_STS(int ch);
	void fc_CCH(int ch);
	void fc_MW(int ch);
	void fc_SPA(int ch);
	void fc_EPA(int ch);
	void fc_SCI(int ch);
	void fc_RIS(int ch);
	void fc_LMA(int ch);
	void fc_USR(int ch);
	void fc_V5EX(int ch);
	void fc_SS2(int ch);
	void fc_SS3(int ch);
	void fc_LS2(int ch);
	void fc_LS3(int ch);
	void fc_LS1R(int ch);
	void fc_LS2R(int ch);
	void fc_LS3R(int ch);
	void fc_CSC0W(int ch);
	void fc_CSC0(int ch);
	void fc_CSC1(int ch);
	void fc_CSC2(int ch);
	void fc_CSC3(int ch);
	void fc_CSC0A(int ch);
	void fc_CSC1A(int ch);
	void fc_CSC2A(int ch);
	void fc_CSC3A(int ch);
	void fc_V5MCP(int ch);
	void fc_DECSOP(int ch);
	void fc_ACS(int ch);
	void fc_DOCS(int ch);

	// ESC [ CSI
	void fc_CSI(int ch);
	void fc_CSI_ESC(int ch);
	void fc_CSI_DIGIT(int ch);
	void fc_CSI_SEPA(int ch);
	void fc_CSI_EXT(int ch);
	void fc_CSI_ETC(int ch);

	// CSI @-Z
	void fc_ICH(int ch);
	void fc_CUU(int ch);
	void fc_CUD(int ch);
	void fc_CUF(int ch);
	void fc_CUB(int ch);
	void fc_CNL(int ch);
	void fc_CPL(int ch);
	void fc_CHA(int ch);
	void fc_CUP(int ch);
	void fc_CHT(int ch);
	void fc_ED(int ch);
	void fc_EL(int ch);
	void fc_IL(int ch);
	void fc_DL(int ch);
	void fc_EF(int ch);
	void fc_EA(int ch);
	void fc_DCH(int ch);
	void fc_SU(int ch);
	void fc_SD(int ch);
	void fc_NP(int ch);
	void fc_PP(int ch);
	void fc_CTC(int ch);
	void fc_ECH(int ch);
	void fc_CVT(int ch);
	void fc_CBT(int ch);
	// CSI a-z
	void fc_HPR(int ch);
	void fc_REP(int ch);
	void fc_DA1(int ch);
	void fc_VPA(int ch);
	void fc_VPR(int ch);
	void fc_HVP(int ch);
	void fc_TBC(int ch);
	void fc_SM(int ch);
	void fc_RM(int ch);
	void fc_MC(int ch);
	void fc_HPB(int ch);
	void fc_VPB(int ch);
	void fc_SGR(int ch);
	void fc_DSR(int ch);
	void fc_DAQ(int ch);
	void fc_DECBFAT(int ch);
	void fc_DECLL(int ch);
	void fc_DECSTBM(int ch);
	void fc_DECSC(int ch);
	void fc_XTWOP(int ch);
	void fc_DECRC(int ch);
	void fc_ORGSCD(int ch);
	void fc_REQTPARM(int ch);
	void fc_DECTST(int ch);
	void fc_DECVERP(int ch);
	void fc_SIMD(int ch);
	void fc_HPA(int ch);
	void fc_LINUX(int ch);
	// CSI ? ...
	void fc_DECSED(int ch);
	void fc_DECSEL(int ch);
	void fc_DECST8C(int ch);
	void fc_DECSET(int ch);
	void fc_DECRST(int ch);
	void fc_XTREST(int ch);
	void fc_XTSAVE(int ch);
	void fc_DECDSR(int ch);
	void fc_DECSRET(int ch);
	// CSI $ ...
	void fc_DECRQM(int ch);
	void fc_DECCARA(int ch);
	void fc_DECRARA(int ch);
	void fc_DECCRA(int ch);
	void fc_DECFRA(int ch);
	void fc_DECERA(int ch);
	void fc_DECSERA(int ch);
	// CSI ...
	void fc_DECRQMH(int ch);
	void fc_SL(int ch);
	void fc_SR(int ch);
	void fc_DECTME(int ch);
	void fc_DECSTR(int ch);
	void fc_DECSCA(int ch);
	void fc_DECEFR(int ch);
	void fc_DECELR(int ch);
	void fc_DECSLE(int ch);
	void fc_DECRQLP(int ch);
	void fc_DECIC(int ch);
	void fc_DECDC(int ch);
	void fc_DECSACE(int ch);
	void fc_DECATC(int ch);
	void fc_DA2(int ch);
	void fc_DA3(int ch);
	void fc_C25LCT(int ch);


	// ESC PX^_] DCS/PM/APC/SOS/OSC
	void fc_DCS(int ch);
	void fc_PM(int ch);
	void fc_APC(int ch);
	void fc_SOS(int ch);
	void fc_OSC(int ch);
	void fc_OSC_CH(int ch);
	void fc_OSC_POP(int ch);
	void fc_OSC_ESC(int ch);
	void fc_OSC_CSI(int ch);
	void fc_OSC_EST(int ch);
	void fc_OSC_ST(int ch);

	// TEK
	typedef struct _TEKNODE {
		struct _TEKNODE *next;
		BYTE md, st, rv[2];
		short sx, sy, ex, ey;
		int ch;
	} TEKNODE;

	int m_Tek_Ver;
	int m_Tek_Mode;
	int m_Tek_Stat;
	int m_Tek_Pen;
	BOOL m_Tek_Gin;
	int m_Tek_Line;
	int m_Tek_Font;
	int m_Tek_Point;
	int m_Tek_Angle;
	int m_Tek_Base;
	int m_Tek_cX, m_Tek_cY;
	int m_Tek_lX, m_Tek_lY;
	int m_Tek_wX, m_Tek_wY;
	int m_Tek_Int;
	int m_Tek_Pos;
	BOOL m_Tek_Flush;
	TEKNODE *m_Tek_Top;
	TEKNODE *m_Tek_Free;
	class CTekWnd *m_pTekWnd;

	inline BOOL IsTekMode() { return (m_Stage == STAGE_TEK ? TRUE : FALSE); }
	void TekInit(int ver);
	void TekClose();
	void TekForcus(BOOL fg);
	void TekFlush();
	void TekDraw(CDC *pDC, CRect &frame);
	void TekClear();
	void TekLine(int st, int sx, int sy, int ex, int ey);
	void TekText(int st, int sx, int sy, int ch);
	void TekMark(int st, int no, int sx, int sy);
	void TekEdge(int ag);

	void fc_TEK_IN(int ch);
	void fc_TEK_LEFT(int ch);
	void fc_TEK_RIGHT(int ch);
	void fc_TEK_DOWN(int ch);
	void fc_TEK_UP(int ch);
	void fc_TEK_FLUSH(int ch);
	void fc_TEK_MODE(int ch);
	void fc_TEK_STAT(int ch);

#define	LOC_MODE_ENABLE		0x0001
#define	LOC_MODE_ONESHOT	0x0002
#define	LOC_MODE_FILTER		0x0004
#define	LOC_MODE_PIXELS		0x0008
#define	LOC_MODE_EVENT		0x0010
#define	LOC_MODE_UP			0x0020
#define	LOC_MODE_DOWN		0x0040

	// DEC Locator
	int m_Loc_Mode;
	CRect m_Loc_Rect;
	int m_Loc_Pb;
	int m_Loc_LastS;
	int m_Loc_LastX;
	int m_Loc_LastY;

	void LocReport(int md, int sw, int x, int y);
};

#endif // !defined(AFX_TEXTRAM_H__CBEA227A_D7D7_4213_88B1_4F4C0DF48089__INCLUDED_)
