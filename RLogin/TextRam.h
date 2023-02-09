//////////////////////////////////////////////////////////////////////
// CTextRam

#pragma once

#include <afxtempl.h>
#include <afxmt.h>
#include "Data.h"
#include "IConv.h"
#include "TekWnd.h"
#include "GrapWnd.h"
#include "GhostWnd.h"

#define	FIX_VERSION		12				// AnsiOpt Bugfix version

#define	COLS_MAX		256
#define	LINE_MAX		128

#define	HIS_MAX			200000			// HIS_MAX * COLS_MAX * sizeof(CCharCell) = 6,553,600,000 byte
#define	DEF_TAB			8
#define	FKEY_MAX		24
#define	KANBUFMAX		128
#define	MACROMAX		64
#define	DEFCOLTAB		0				// Black=0, Amber=1, Green=2, Sian=3, White=4, Solarized=5, Pastel=6
#define	CMDHISMAX		200
#define	HISLISTMAX		30

#define	TEK_WIN_WIDTH	4096
#define	TEK_WIN_HEIGHT	3072

#define	DEFVAL_VTLEVEL	65
#define	DEFVAL_FIRMVER	331
#define	DEFVAL_TERMID	10
#define	DEFVAL_UNITID	0
#define	DEFVAL_KEYBID	0

#define	EUC_SET			0
#define	SJIS_SET		1
#define	ASCII_SET		2
#define	UTF8_SET		3
#define	BIG5_SET		4

#define	LOGMOD_RAW		0
#define	LOGMOD_CTRL		1
#define	LOGMOD_CHAR		2
#define	LOGMOD_LINE		3
#define	LOGMOD_PAGE		4
#define	LOGMOD_DEBUG	5

#define ATT_BOLD		0x00000001		// [1m bold or increased intensity
#define	ATT_HALF		0x00000002		// [2m faint, decreased intensity or second colour
#define ATT_ITALIC		0x00000004		// [3m italicized
#define ATT_UNDER		0x00000008		// [4m singly underlined
#define ATT_SBLINK		0x00000010		// [5m slowly blinking
#define ATT_REVS		0x00000020		// [7m negative image
#define ATT_SECRET		0x00000040		// [8m concealed characters
#define ATT_LINE		0x00000080		// [9m crossed-out
#define ATT_BLINK		0x00000100		// [6m rapidly blinking 
#define ATT_DUNDER		0x00000200		// [21m doubly underlined
#define ATT_FRAME		0x00000400		// [51m framed
#define ATT_CIRCLE		0x00000800		// [52m encircled
#define ATT_OVER		0x00001000		// [53m overlined
#define ATT_RSLINE		0x00002000		// [60m right side line
#define ATT_RDLINE		0x00004000		// [61m double line on the right side
#define ATT_LSLINE		0x00008000		// [62m left side line
#define ATT_LDLINE		0x00010000		// [63m double line on the left side
#define ATT_STRESS		0x00020000		// [64m stress marking
#define ATT_DOVER		0x00040000		//      double over line
#define	ATT_SUNDER		0x00080000		// [50m signle under line
//						0x00100000
#define	ATT_EXTVRAM		0x00200000		// extend vram
#define	ATT_SMARK		0x00400000		// Search Mark
//						0x00800000
#define	ATT_RTOL		0x01000000		// RtoL Char
#define	ATT_BORDER		0x02000000		// U+2500 Border Char
#define	ATT_EMOJI		0x04000000		// Color Emoji
#define	ATT_RETURN		0x08000000		// CR/LF Mark		// max 7 * 4 = 28 bit
#define	ATT_MASK		0x00FFFFFF
#define	ATT_MASKEXT		0x07FFFFFF		// ATT_RETURN Mask

// 保存されないアトリビュート
#define	ATT_CLIP		0x10000000		// Mouse Clip
// どれか１つ
#define	ATT_MIRROR		0x20000000		// Mirror Draw
#define	ATT_OVERCHAR	0x40000000		// Over Write Charctor
//						0x60000000
//						0x80000000
//						0xA0000000
//						0xC0000000
//						0xE0000000

#define	ATT_FCOL		0x10000000		// PUSH/POP SGR Fcol
#define	ATT_BCOL		0x20000000		// PUSH/POP SGR bcol

#define	ATT_EXTYPE(a)	((a) & 0xE0000000)

#define	BLINK_TIME		300				// ATT_BLINK time (ms)
#define	SBLINK_TIME		600				// ATT_SBLINK time (ms)

#define	RTF_NONE		0				// Not Create RTF
#define	RTF_ATTR		1				// Attribute
#define	RTF_COLOR		2				// Attribute + Color
#define	RTF_FONT		3				// Attribute + Color + Fonts
#define	RTF_JPEG		4				// Attribute + Color + Fonts + Jpeg
#define	RTF_BITMAP		5				// Attribute + Color + Fonts + Bitmap

#define	RTF_DPI			180				// RTF Print DPI 96-300?

#define CODE_MAX		0x0400
#define CODE_MASK		0x03FF
#define SET_MASK		0x0300
#define SET_94			0x0000
#define SET_96			0x0100
#define SET_94x94		0x0200
#define SET_96x96		0x0300
#define	SET_UNICODE		0x03FF

#define	EM_ISOPROTECT	1
#define	EM_DECPROTECT	2

#define DM_SWSH			0
#define DM_DWSH			1
#define DM_DWDHT		2
#define DM_DWDHB		3

#define CM_ASCII		0
#define CM_1BYTE		1
#define CM_2BYTE		2
#define	CM_IMAGE		3

#define IS_ASCII(a) 	((a) == CM_ASCII)
#define IS_1BYTE(a) 	((a) == CM_1BYTE)
#define IS_2BYTE(a) 	((a) == CM_2BYTE)
#define IS_IMAGE(a)	 	((a) == CM_IMAGE)

#define	BRKMBCS			'?'

// DEC Terminal Option	0-199
#define	TO_DECCKM		1			// Cursor key mode
#define	TO_DECANM		2			// ANSI/VT52 Mode
#define	TO_DECCOLM		3			// 80/132 Column mode
#define	TO_DECSCLM		4			// Scrolling mode
#define	TO_DECSCNM		5			// Light or Dark Screen
#define TO_DECOM		6			// Origin mode
#define TO_DECAWM		7			// Autowrap mode
#define TO_DECARM		8			// Autorepeat mode
#define	TO_XTMOSREP		9			// X10 mouse reporting
#define	TO_XTCBLINK		12			// Cursor Blnk
#define	TO_DECPEX		19			// Print Extent Mode
#define	TO_DECTCEM		25			// Text Cursor Enable Mode
#define	TO_DECTEK		38			// Graphics (Tek)
#define	TO_XTMCSC		40			// XTerm Column switch control
#define	TO_XTMCUS		41			// XTerm tab bug fix
#define TO_XTMRVW		45			// XTerm Reverse-wraparound mode
#define	TO_XTMABUF		47			// XTerm alternate buffer
#define	TO_DECNKM		66			// Numeric Keypad Mode
#define	TO_DECBKM		67			// Backarrow key mode (BS)
#define	TO_DECLRMM		69			// Left Right Margin Mode
#define	TO_DECSDM		80			// Sixel Display Mode Control reset = Scroll mode Enable
#define	TO_DECNCSM		95			// Set/Reset No Clearing Screen On Column Change
#define	TO_DECECM		117			// SGR space color disable
// ANSI Screen Option	0-99(200-299)
#define	TO_ANSIGATM		(1+200)		// GATM Guarded area transfer
#define	TO_ANSIKAM		(2+200)		// KAM Set Keyboard Action Mode
#define	TO_ANSIIRM		(4+200)		// IRM Insertion replacement mode
#define	TO_ANSISRTM		(5+200)		// SRTM Status reporting transfer
#define	TO_ANSIERM		(6+200)		// ERM Erasure mode
#define	TO_ANSIVEM		(7+200)		// VEM Vertical editing
#define	TO_ANSIHEM		(10+200)	// HEM Horiz editing
#define	TO_ANSIPUM		(11+200)	// PUM Positioning unit
#define	TO_ANSISRM		(12+200)	// SRM Set Send/Receive mode (Local echo off)
#define	TO_ANSIFEAM		(13+200)	// FEAM Format effector action
#define	TO_ANSIFETM		(14+200)	// FETM Format effector transfer
#define	TO_ANSIMATM		(15+200)	// MATM Multiple area transfer
#define	TO_ANSITTM		(16+200)	// TTM Transfer termination
#define	TO_ANSISATM		(17+200)	// SATM Selected area transfer
#define	TO_ANSITSM		(18+200)	// ISM Tabulation stop mode
#define	TO_ANSIEBM		(19+200)	// EBM Editing boundary
#define	TO_ANSILNM		(20+200)	// LNM Line feed/new line mode
// XTerm Option			1000-1079(300-379)
#define	TO_XTNOMTRK		(1000-700)	// X11 normal mouse tracking
#define	TO_XTHILTRK		(1001-700)	// X11 hilite mouse tracking
#define	TO_XTBEVTRK		(1002-700)	// X11 button-event mouse tracking
#define	TO_XTAEVTRK		(1003-700)	// X11 any-event mouse tracking
#define	TO_XTFOCEVT		(1004-700)	// Send Focus Event (CSI I / CSI O)
#define	TO_XTEXTMOS		(1005-700)	// Enable Extended Mouse Mode
#define	TO_XTSGRMOS		(1006-700)	// SGR Extended Mouse Mode
#define	TO_XTURXMOS		(1015-700)	// URXVT Extended Mouse Mode
#define	TO_XTALTSCR		(1047-700)	// Alternate/Normal screen buffer
#define	TO_XTSRCUR		(1048-700)	// Save/Restore cursor as in DECSC/DECRC
#define	TO_XTALTCLR		(1049-700)	// Alternate screen with clearing
#define	TO_XTLEGKEY		(1060-700)	// legacy keyboard emulation (X11R6)
#define	TO_XTPRICOL		(1070-700)	// Regis/Sixel Private Color Map
// XTerm Option 2		2000-2019(380-399)
#define	TO_XTBRPAMD		(2004-1620)	// Bracketed Paste Mode
// RLogin Option		8400-8511(400-511)
#define	TO_RLGCWA		400			// ESC[m space att enable
#define	TO_RLGNDW		401			// 行末での遅延改行を無効にする
#define	TO_RLGAWL		402			// http;//で始まる文字列をコピーすると自動でブラウザを起動する
#define	TO_RLBOLD		403			// ボールド文字を有効にする
#define	TO_RLFONT		404			// フォントサイズから一行あたりの文字数を決定
#define	TO_RLBPLUS		405			// BPlus/ZModemファイル転送を有効にする
#define	TO_RLALTBDIS	406			// 拡張バッファを使用しない
#define	TO_RLADBELL		416			// ベルコード(07)による動作を選択 x1
#define	TO_RLVSBELL		417			// ベルコード(07)による動作を選択 1x
#define	TO_RLUNIAWH		428			// UnicodeのAタイプの文字を半角として表示する
#define	TO_RLSPCTAB		429			// 選択した文字列から連続したスペースをタブに変換する
#define	TO_RLNORESZ		435			// DECCOLMの文字数切換でウィンドウをリサイズする
#define	TO_RLKANAUTO	437			// 漢字コードを自動で追従する
#define	TO_RLMOSWHL		438			// マウスホイールをヌルヌル禁止
#define	TO_RLMSWAPP		439			// マウスホイールのCKMモード時カーソルキー動作を禁止
#define	TO_RLPNAM		440			// 0:Normal Keypad (DECPNM) / 1:Application Keypad (DECPAM)
#define	TO_IMECTRL		441			// IME Open/Close
#define	TO_RLCKMESC		442			// ESCキーをDECCKMに含める		7727  -  Application Escape mode を有効にする。 
#define	TO_RLMSWAPE		443			// ホイールのキー変換強制		7786  -  マウスホイール - カーソルキー変換を有効にする。
#define	TO_RLTEKINWND	446			// Tekウィンドウをビューで描く
#define	TO_RLUNINOM		448			// Unicode ノーマライズを禁止する
#define	TO_RLUNIAHF		449			// Unicodeの半角時にサイズを調整する
#define	TO_RLMODKEY		450			// modfiyKeysを優先する
#define	TO_RLDRWLINE	451			// 罫線を線描画に置き換えない
#define	TO_RLSIXPOS		452			// Sixel表示後のカーソル位置設定
#define	TO_DRCSMMv1		453			// 8800 h=マッピング有効 l=マッピング無効
#define	TO_RLC1DIS		454			// C1制御を行わない
#define TO_RLCLSBACK	455			// 全画面消去でスクロールする
#define	TO_RLBRKMBCS	456			// 壊れたMBCSの代替え表示
#define	TO_TTCTH		457			// 8200 画面クリア(ED 2)時にカーソルを左上に移動する。
#define	TO_RLBOLDHC		458			// ボールド文字で高輝度を無効にする
#define	TO_RLYENKEY		459			// UTF-8の場合に\キーでU+00A5を送信する

// RLogin SockOpt		1000-1511(0-511)
#define	TO_RLTENAT		1406		// 自動ユーザー認証を行わない
#define	TO_RLTENEC		1407		// データの暗号化を禁止する
#define	TO_RLPOFF		1408		// 接続が切れると自動でプログラムを終了する
#define	TO_RLUSEPASS	1409		// 接続時にパスワード入力を求める
#define	TO_RLRCLICK		1410		// 右ダブルクリックだけでクリップボードからペーストする
#define	TO_RLCKCOPY		1411		// 左クリックの範囲指定だけでクリップボードにコピーする
#define	TO_RLDSECHO		1412		// キーボード入力をローカルエディットモードにする
#define	TO_RLDELAY		1413		// 行単位で指定時間待って送信する(ms)
#define	TO_RLHISFILE	1414		// ヒストリーを保存し次接続時に復元する
#define	TO_RLKEEPAL		1415		// TCPオプションのKeepAliveを有効にする
#define	TO_SSH1MODE		1420		// SSHバージョン１で接続する
#define	TO_SSHPFORY		1421		// ポートフォワードだけ行う （SSH2のみ）
#define	TO_SSHKEEPAL	1422		// KeepAliveパケットの送信間隔(sec)
#define	TO_SSHAGENT		1423		// エージェント転送を有効にする(SSH2のみ)
#define	TO_SSHSFENC		1424		// 暗号シャッフル
#define	TO_SSHSFMAC		1425		// 検証シャッフル
#define	TO_RLHISDATE	1430		// 通信ログを年月日を付けて自動作成する
#define	TO_RLLOGMODE	1431		// ファイルに保存する通信ログの形式を選択 x1
#define	TO_RLLOGMOD2	1432		// ファイルに保存する通信ログの形式を選択 1x
#define	TO_RLLOGCODE	1433		// ログの文字コードを選択(RAWモード以外で有効) x1
#define	TO_RLLOGCOD2	1434		// ログの文字コードを選択(RAWモード以外で有効) 1x
#define	TO_SSHX11PF		1436		// X11ポートフォワードを使用する
#define	TO_RLTENLM		1444		// TELNET LINEMODE を禁止する
#define	TO_RLSCRDEBUG	1445		// スクリプトデバックを行う
#define	TO_RLCURIMD		1452		// カーソルをIで表示
#define TO_RLRSPAST		1453		// 右クリックだけでクリップボードからペーストする
#define	TO_RLGWDIS		1454		// ゴーストウィンドウを表示しない
#define	TO_RLMWDIS		1455		// リサイズメッセージを表示しない
#define	TO_RLGRPIND		1456		// イメージをウィンドウ内で表示
#define	TO_RLLOGTIME	1457		// ログにタイムスタンプを追加
#define	TO_RLSTAYCLIP	1458		// 範囲選択をそのままにする
#define	TO_PROXPASS		1459		// プロキシーの入力を求める
#define	TO_SLEEPTIMER	1460		// スリープを行う
#define	TO_SETWINPOS	1461		// XTWOPでWindowsの操作を行う
#define	TO_RLWORDPP		1462		// プロポーショナルフォントをワードで調整
#define	TO_RLRBSCROLL	1463		// マウスのRボタンでスクロールする
#define	TO_RLGROUPCAST	1464		// グループキャストに含める
#define	TO_RLEDITPAST	1465		// 常にペーストを確認する
#define	TO_RLDELYPAST	1466		// ペースト時に改行確認する
#define	TO_RLUPDPAST	1467		// ペースト時の編集後コピーする
#define	TO_RLNTBCSEND	1468		// 同時送信しない
#define	TO_RLNTBCRECV	1469		// 同時受信しない
#define	TO_RLNOTCLOSE	1470		// 接続断でクロースしない
#define	TO_RLPAINWTAB	1471		// ペインの移動でタブも移動する
#define	TO_RLHIDPIFSZ	1472		// フォントサイズをHiDPIに追従する
#define	TO_RLBACKHALF	1473		// アクティブでないウィンドウの輝度を下げる
#define	TO_RLLOGFLUSH	1474		// ログの書き出しを定期的に行う
#define	TO_RLCARETANI	1475		// カレットアニメーション
#define	TO_RLTABINFO	1476		// タブにツールチップを表示する
#define	TO_RLREOPEN		1477		// 再接続を確認
#define	TO_RLDELCRLF	1478		// ペースト時に末尾のCRLFを削除
#define	TO_RLPSUPWIN	1479		// 積極的なウィンドウの描画更新
#define	TO_TELKEEPAL	1480		// KeepAliveパケットの送信間隔(sec)
#define	TO_SSHSFTPORY	1481		// 接続時にSFTPを起動する
#define	TO_RLTRSLIMIT	1482		// TCP/IPの帯域制限を行う
#define	TO_RLDOSAVE		1483		// SOS/PM/APCでファイルに保存する
#define	TO_RLNOCHKFONT	1484		// デフォルトフォントのCSetチェックを行わない
#define	TO_RLCOLEMOJI	1485		// カラー絵文字を表示する
#define	TO_RLNODELAY	1486		// TCPオプションのNoDelayを有効にする
#define	TO_RLDELAYRV	1487		// 無受信時間待って次を送信する(ms)
#define	TO_RLDELAYCR	1488		// 改行(CR)を確認し指定時間待って次を送信する(ms)
#define	TO_IMECARET		1489		// IMEがONの時にカレットの色を変える
#define	TO_DNSSSSHFP	1490		// DNSによるSSSHホスト鍵のチェックを行う
#define	TO_RLENOEDPAST	1491		// ペーストを確認しない

#define	IS_ENABLE(p,n)	(p[(n) / 32] & (1 << ((n) % 32)))

#define	WTTL_ENTRY		0000		// m_EntryName
#define	WTTL_HOST		0001		// m_HostName
#define	WTTL_PORT		0002		// m_HostName:m_PotrtName
#define	WTTL_USER		0003		// m_UserName
#define	WTTL_CHENG		0020		// OSC Title cheng disable
#define	WTTL_REPORT		0040		// XTWOP Title report disable

#define	ERM_ISOPRO		0001		// SPA,EPA,ERM	ED, EL, EF, EA
#define	ERM_DECPRO		0002		// DECSCA		DECSERA, DECSED, DECSEL
#define	ERM_SAVEDM		0004		// DECDHL...

#define	MOS_EVENT_NONE	0
#define	MOS_EVENT_X10	1			//    9 X10 mouse reporting
#define	MOS_EVENT_NORM	2			// 1000 X11 normal mouse tracking
#define	MOS_EVENT_HILT	3			// 1001 X11 hilite mouse tracking
#define	MOS_EVENT_BTNE	4			// 1002 X11 button-event mouse tracking
#define	MOS_EVENT_ANYE	5			// 1003 X11 any-event mouse tracking
#define	MOS_EVENT_LOCA	6			// DECLRP (Locator report)

#define	MOS_LOCA_INIT	0			//	Init Mode
#define	MOS_LOCA_LEDN	1			//	Left Down
#define	MOS_LOCA_LEUP	2			//	Left Up
#define	MOS_LOCA_RTDN	3			//	Right Down
#define	MOS_LOCA_RTUP	4			//	Right Up
#define	MOS_LOCA_MOVE	5			//	Mouse Move
#define	MOS_LOCA_REQ	6			//	Request
#define	MOS_LOCA_WHUP	7			//	Wheel Up
#define	MOS_LOCA_WHDOWN	8			//	Wheel Down

#define	LOC_MODE_ENABLE		0x0001
#define	LOC_MODE_ONESHOT	0x0002
#define	LOC_MODE_FILTER		0x0004
#define	LOC_MODE_PIXELS		0x0008
#define	LOC_MODE_EVENT		0x0010
#define	LOC_MODE_UP			0x0020
#define	LOC_MODE_DOWN		0x0040

#define	UTF16BE			0
#define	UTF16LE			1

#define	RESET_CURSOR	0x000001
#define	RESET_TABS		0x000002
#define	RESET_BANK		0x000004
#define	RESET_ATTR		0x000008
#define	RESET_COLOR		0x000010
#define	RESET_TEK		0x000020
#define	RESET_SAVE		0x000040
#define	RESET_MOUSE		0x000080
#define	RESET_CHAR		0x000100
#define	RESET_OPTION	0x000200
#define	RESET_CLS		0x000400
#define	RESET_HISTORY	0x000800
#define	RESET_PAGE		0x001000
#define	RESET_MARGIN	0x002000
#define	RESET_SIZE		0x004000
#define	RESET_IDS		0x008000
#define	RESET_MODKEY	0x010000
#define	RESET_XTOPT		0x020000
#define	RESET_CARET		0x040000
#define	RESET_STATUS	0x080000
#define	RESET_TRANSMIT	0x100000
#define	RESET_RLMARGIN	0x002000

						// WithOut RESET_CLS RESET_HISTORY RESET_SIZE RESET_STATUS RESET_TRANSMIT
#define	RESET_ALL		(RESET_CURSOR | RESET_TABS | RESET_BANK | RESET_ATTR | RESET_COLOR | \
						 RESET_TEK | RESET_SAVE | RESET_MOUSE | RESET_CHAR | RESET_OPTION | \
						 RESET_PAGE | RESET_MARGIN | RESET_IDS | RESET_MODKEY | RESET_XTOPT | \
						 RESET_CARET | RESET_STATUS | RESET_RLMARGIN)

#define	RC_DCS			0
#define	RC_SOS			1
#define	RC_CSI			2
#define	RC_ST			3
#define	RC_OSC			4
#define	RC_PM			5
#define	RC_APC			6

#define	OSC52_READ		001
#define	OSC52_WRITE		002

#define	USFTCMAX		96
#define	USFTWMAX		24
#define	USFTHMAX		36
#define	USFTLNSZ		((USFTHMAX + 5) / 6)
#define	USFTCHSZ		(USFTWMAX * USFTLNSZ)

#define	DELAY_NON		0
#define	DELAY_PAUSE		1
#define	DELAY_WAIT		2

#define	UNICODE_MAX		0x0010FFFF		// U+000000 - U+10FFFF 21 bit (サロゲート可能範囲)
#define	UNICODE_UNKOWN	0x000025A1		// □

#define	UNI_NAR			0x00000001		// 1 width char
#define	UNI_WID			0x00000002		// 2 width char
#define	UNI_AMB			0x00000004		// A type char 1 or 2 width
#define	UNI_NSM			0x00000008		// Non Spaceing Mark char
#define	UNI_IVS			0x00000010		// IVS Mark
#define	UNI_SGH			0x00000020		// サロゲート Hi
#define	UNI_SGL			0x00000040		// サロゲート Low
#define	UNI_CTL			0x00000080		// RtoL LtoR Control char
#define	UNI_HNF			0x00000100		// Hangle Fast
#define	UNI_HNM			0x00000200		// Hangle Mid
#define	UNI_HNL			0x00000400		// Hangle Last
#define	UNI_RTL			0x00000800		// Right to Left block
#define	UNI_GAIJI		0x00001000		// Gaiji
#define	UNI_EMOJI		0x00002000		// Emoji
#define	UNI_EMODF		0x00004000		// Emoji Modifier
#define	UNI_BN			0x00008000		// Boundary Neutral
#define	UNI_GRP			0x00010000		// Spacing Mark in Block

										// CSI > Ps T/t		xterm CASE_RM_TITLE/CASE_SM_TITLE option flag bits m_XtOptFlag
#define	XTOP_SETHEX		0x0001			//    Ps = 0  -> Set window/icon labels using hexadecimal.
#define	XTOP_GETHEX		0x0002			//    Ps = 1  -> Query window/icon labels using hexadecimal.
#define	XTOP_SETUTF		0x0004			//    Ps = 2  -> Set window/icon labels using UTF-8.
#define	XTOP_GETUTF		0x0008			//    Ps = 3  -> Query window/icon labels using UTF-8.

#define	MODKEY_ALLOW	0				//    how to handle legacy/vt220 keyboard
#define	MODKEY_CURSOR	1				//    how to handle cursor-key modifiers
#define	MODKEY_FUNC		2				//    how to handle function-key modifiers
#define	MODKEY_KEYPAD	3				//    how to handle keypad key-modifiers
#define	MODKEY_OTHER	4				//    how to handle other key-modifiers
#define	MODKEY_STRING	5				//    how to handle string() modifiers
#define	MODKEY_MAX		6

#define	TERMPARA_VTLEVEL		0
#define	TERMPARA_FIRMVER		1
#define	TERMPARA_TERMID			2
#define	TERMPARA_UNITID			3
#define	TERMPARA_KEYBID			4

#define	EXTCOL_BEGIN			256
#define	EXTCOL_VT_TEXT_FORE		256				// 10  -> Change VT100 text foreground color to Pt.
#define	EXTCOL_VT_TEXT_BACK		257				// 11  -> Change VT100 text background color to Pt.
#define	EXTCOL_TEXT_CURSOR		258				// 12  -> Change text cursor color to Pt.
#define	EXTCOL_MOUSE_FORE		259				// 13  -> Change mouse foreground color to Pt.
#define	EXTCOL_MOUDE_BACK		260				// 14  -> Change mouse background color to Pt.
#define	EXTCOL_TEK_FORE			261				// 15  -> Change Tektronix foreground color to Pt.
#define	EXTCOL_TEK_BACK			262				// 16  -> Change Tektronix background color to Pt.
#define	EXTCOL_HILIGHT_BACK		263				// 17  -> Change highlight background color to Pt.
#define	EXTCOL_TEK_CURSOR		264				// 18  -> Change Tektronix cursor color to Pt.
#define	EXTCOL_HILIGHT_FORE		265				// 19  -> Change highlight foreground color to Pt.

#define	EXTCOL_SP_BEGIN			266
#define	EXTCOL_TEXT_BOLD		266				// 5;0 <- resource colorBD (BOLD).
#define	EXTCOL_TEXT_UNDER		267				// 5;1 <- resource colorUL (UNDERLINE).
#define	EXTCOL_TEXT_BLINK		268				// 5;2 <- resource colorBL (BLINK).
#define	EXTCOL_TEXT_REVERSE		269				// 5;3 <- resource colorRV (REVERSE).

#define	EXTCOL_MAX				270
#define	EXTCOL_MATRIX			26

#define	FNT_BITMAP_MONO			1
#define	FNT_BITMAP_COLOR		2

#define	MARCHK_NONE				0
#define	MARCHK_COLS				1
#define	MARCHK_LINES			2
#define	MARCHK_BOTH				3

#define	PROCTYPE_ESC			0
#define	PROCTYPE_CSI			1
#define	PROCTYPE_DCS			2
#define	PROCTYPE_CTRL			3
#define	PROCTYPE_TEK			4

#define	TRACE_OUT				0
#define	TRACE_NON				1
#define	TRACE_SIXEL				2
#define	TRACE_UNGET				3
#define	TRACE_IGNORE			4

#define	OSCCLIP_LF				0
#define	OSCCLIP_CR				1
#define	OSCCLIP_CRLF			2

#define	MEDIACOPY_NONE			0
#define	MEDIACOPY_RELAY			1
#define	MEDIACOPY_THROUGH		2

#define	STSMODE_HIDE			0
#define	STSMODE_INDICATOR		1
#define	STSMODE_INSCREEN		2
#define	STSMODE_DISABLE			3
#define	STSMODE_ENABLE			4
#define	STSMODE_MASK			3

#define	SAVEMODE_PARAM			0
#define	SAVEMODE_VRAM			1
#define	SAVEMODE_ALL			2

#define	SLEEPMODE_NONE			0
#define	SLEEPMODE_ACTIVE		1
#define	SLEEPMODE_ALLWAY		2

#define	UNIBLOCKTABMAX			308

typedef struct _UNIBLOCKTAB {
	DWORD	code;
	LPCTSTR	name;
} UNIBLOCKTAB;

extern const UNIBLOCKTAB UniBlockTab[UNIBLOCKTABMAX];

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

#define isbig51(c)		(((unsigned char)(c) >= 0xA1 && \
						  (unsigned char)(c) <= 0xC6) || \
						 ((unsigned char)(c) >= 0xC9 && \
						  (unsigned char)(c) <= 0xF9) )

#define isbig52(c)		(((unsigned char)(c) >= 0x40 && \
						  (unsigned char)(c) <= 0x7E) || \
						 ((unsigned char)(c) >= 0xA1 && \
						  (unsigned char)(c) <= 0xFE) )

enum ETextRamStat {
		ST_ENDOF,		ST_NULL,
		ST_CONF_LEVEL,
		ST_COLM_SET,	ST_COLM_SET_2,
		ST_DEC_OPT,
		ST_DOCS_OPT,	ST_DOCS_OPT_2,
		ST_TEK_OPT,
		ST_CHARSET_1,	ST_CHARSET_2
};

enum ETabSetNum {
		TAB_COLSSET,	TAB_COLSCLR,	TAB_COLSALLCLR,	TAB_COLSALLCLRACT,
		TAB_LINESET,	TAB_LINECLR,	TAB_LINEALLCLR,
		TAB_RESET,
		TAB_DELINE,		TAB_INSLINE,
		TAB_ALLCLR,
		TAB_COLSNEXT,	TAB_COLSBACK,
		TAB_CHT,		TAB_CBT,
		TAB_LINENEXT,	TAB_LINEBACK,
};

enum EStageNum {
		STAGE_ESC = 0,	STAGE_CSI,		STAGE_EXT1,		STAGE_EXT2,		STAGE_EXT3,		// Use 0,1,2,3,4 !!! Look up fc_Case()
		STAGE_EXT4,		STAGE_EUC,
		STAGE_94X94,	STAGE_96X96,
		STAGE_SJIS,		STAGE_SJIS2,
		STAGE_BIG5,		STAGE_BIG52,
		STAGE_UTF8,		STAGE_UTF82,
		STAGE_OSC1,		STAGE_OSC2,
		STAGE_TEK,
		STAGE_STAT,		STAGE_GOTOXY,
		STAGE_MAX,
};

///////////////////////////////////////////////////////

#define	EATT_FRGBCOL	0x0001
#define	EATT_BRGBCOL	0x0002

#define	MAXCHARSIZE		12
#define	MAXEXTSIZE		(MAXCHARSIZE - 5)	// WORD eatt + COLORREF frgb + COLORREF brgb = 10 / WCHAR = 5

typedef struct _Vram {
	union {
		DWORD	dchar;
		struct {
			DWORD	id:12;		// イメージ番号
			  DWORD	iy:10;		// イメージ横位置
			  DWORD	ix:10;		// イメージ縦位置
		} image;
		WCHAR	wcbuf[2];
	} pack;

	DWORD	attr:28;		// アトリビュート
	  DWORD	font:4;			// フォント番号

	WORD	bank:10;		// フォントバンク
	  WORD	eram:2;			// 消去属性
	  WORD	zoom:2;			// 拡大属性
	  WORD	mode:2;			// 文字種
	BYTE	fcol;			// 文字色番号
	BYTE	bcol;			// 背景色番号
} STDVRAM;

typedef struct _ExtVram {
	WORD		eatt;		// 拡張属性
	COLORREF	brgb;		// 背景色 RGB
	COLORREF	frgb;		// 文字色 RGB	CCharCellではstd.packと共有されるので注意
	STDVRAM		std;
} EXTVRAM;

///////////////////////////////////////////////////////
// sizeof(CCharCell) == 20 + 12 = 32 Byte

class CCharCell
{
public:
	WCHAR		m_Data[MAXCHARSIZE - 2];	// WCHAR * (12 - m_Vram.pack.wcbuf[2]) = 20 Byte
	STDVRAM		m_Vram;						// DWORD * 3 = 12 Byte
	time_t		m_Atime;

	inline void Empty() { if ( !IS_IMAGE(m_Vram.mode) ) m_Data[0] = 0; }
	inline BOOL IsEmpty() { return (IS_IMAGE(m_Vram.mode) || m_Data[0] == 0 ? TRUE : FALSE); }
	inline operator LPCWSTR () { return (IS_IMAGE(m_Vram.mode) ? L"" : m_Data); }
	inline operator DWORD () { return ((m_Data[0] == 0 ? 0 : (m_Data[1] == 0 ? m_Data[0] : ((m_Data[0] << 16) | m_Data[1])))); }

//	inline int Compare(LPCWSTR str) { return wcscmp(str, (LPCWSTR)*this); }
	inline int Compare(LPCWSTR str) { return 0; }

	inline WORD GetEatt() { return *((WORD *)(m_Data + MAXEXTSIZE + 0)); }
	inline COLORREF GetBrgb() { return *((COLORREF *)(m_Data + MAXEXTSIZE + 1)); }
	inline COLORREF GetFrgb() { return *((COLORREF *)(m_Data + MAXEXTSIZE + 3)); }

	inline void SetEatt(WORD eatt) { *((WORD *)(m_Data + MAXEXTSIZE + 0)) = eatt; }
	inline void SetBrgb(COLORREF brgb) { *((COLORREF *)(m_Data + MAXEXTSIZE + 1)) = brgb; }
	inline void SetFrgb(COLORREF frgb) { *((COLORREF *)(m_Data + MAXEXTSIZE + 3)) = frgb; }

	void operator = (DWORD ch);
	void operator = (LPCWSTR str);
	void operator += (DWORD ch);

	void GetEXTVRAM(EXTVRAM &evram);

	inline const CCharCell & operator = (CCharCell &data) { memcpy(this, &data, sizeof(CCharCell)); return *this; }
	inline void operator = (STDVRAM &ram) {	m_Vram = ram; m_Vram.attr &= ~ATT_EXTVRAM; *this = ram.pack.dchar; m_Atime = 0; }
	inline void operator = (EXTVRAM &evram) { *this = evram.std; GetEXTVRAM(evram); }

	void SetBuffer(CBuffer &buf);
	void GetBuffer(CBuffer &buf);
	void SetString(CString &str);
	void GetString(LPCTSTR str);

	void Read(CFile &file, int ver = 3);
	void Write(CFile &file);

	static void Copy(CCharCell *dis, CCharCell *src, int size);
	static void Fill(CCharCell *dis, EXTVRAM &vram, int size);
};

///////////////////////////////////////////////////////

#define	MEMMAPCACHE		4

typedef struct _MemMapNode {
	struct _MemMapNode *pNext;
	BOOL bMap;
	ULONGLONG Offset;
	void *pAddress;
} MEMMAPNODE;

class CMemMap : public CObject
{
public:
	HANDLE m_hFile;
	int m_ColsMax;
	int m_LineMax;
	MEMMAPNODE *m_pMapTop;
	MEMMAPNODE m_MapData[MEMMAPCACHE];
	ULONGLONG m_MapSize;
	ULONGLONG m_MapPage;

	BOOL NewMap(int cols_max, int line_max, int his_max);
	CCharCell *GetMapRam(int x, int y);

	CMemMap();
	~CMemMap();
};

///////////////////////////////////////////////////////

#define	FONTSTYLE_NONE			000
#define	FONTSTYLE_BOLD			001
#define	FONTSTYLE_ITALIC		002
#define	FONTSTYLE_UNDER			004
#define	FONTSTYLE_FULLWIDTH		010

class CFontNode : public CObject
{
public:
	BYTE m_Shift;
	int m_ZoomW;
	int m_ZoomH;
	int m_OffsetW;
	int m_OffsetH;
	int m_CharSet;
	CString m_FontName[16];
	CString m_EntryName;
	CString m_IContName;
	CString m_IndexName;
	int m_Quality;
	BOOL m_Init;
	int m_UserFontWidth;
	int m_UserFontHeight;
	CBitmap m_UserFontMap;
	BYTE m_UserFontDef[96 / 8];
	int m_FontWidth;
	int m_FontHeight;
	CBitmap m_FontMap;
	int m_MapType;
	CString m_UniBlock;
	CString m_Iso646Name[2];
	DWORD m_Iso646Tab[12];
	CString m_OverZero;
	COLORREF *m_pTransColor;
	int m_JpSet;

	void Init();
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);
	CFontChacheNode *GetFont(int Width, int Height, int Style, int FontNum, class CTextRam *pTextRam);
	const CFontNode & operator = (CFontNode &data);
	void SetUserBitmap(int code, int width, int height, CBitmap *pMap, int ofx, int ofy, int asx, int asy, COLORREF bc, COLORREF tc);
	void SetUserFont(int code, int width, int height, LPBYTE map);
	BOOL SetFontImage(int width, int height);
	int Compare(CFontNode &data);
	inline LPCTSTR GetEntryName() { return (!m_EntryName.IsEmpty() ? m_EntryName : m_IndexName); }

	inline BOOL IsOverChar(LPCTSTR str) { return (!m_OverZero.IsEmpty() && _tcscmp(str, _T("0")) == 0 ? TRUE : FALSE); }
	inline LPCTSTR OverCharStr(LPCTSTR str) { return m_OverZero; }

	CFontNode();
	~CFontNode();
};

typedef struct _UniBlockNode {
	DWORD	code;
	int		index;
	LPCTSTR name;
} UniBlockNode;

class CUniBlockTab : public CObject
{
public:
	CArray<UniBlockNode, UniBlockNode &> m_Data;

	inline void RemoveAll() { m_Data.RemoveAll(); }
	inline void RemoveAt(int nIndex) { m_Data.RemoveAt(nIndex); }
	inline INT_PTR GetSize() { return m_Data.GetSize(); }
	inline UniBlockNode & operator[](int nIndex) { return m_Data[nIndex]; }

	const CUniBlockTab & operator = (CUniBlockTab &data);
	int Add(DWORD code, int index, LPCTSTR name = NULL);
	int Find(DWORD code);

	static LPCTSTR GetCode(LPCTSTR str, DWORD &code);
	void SetBlockCode(LPCTSTR str, int index);
};

class CFontTab : public COptObject
{
public:
	CFontNode *m_Data;
	CUniBlockTab m_UniBlockTab;

	void InitUniBlock();

	void Init();
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);
	void SetIndex(int mode, CStringIndex &index);
	void DiffIndex(CFontTab &orig, CStringIndex &index);

	int Find(LPCTSTR entry);
	int IndexFind(int code, LPCTSTR name);
	void IndexRemove(int idx);
	inline CFontNode & operator[](int nIndex) { return m_Data[nIndex]; }
	const CFontTab & operator = (CFontTab &data);

	CFontTab();
	~CFontTab();
};

class CTextBitMap : public COptObject
{
public:
	BOOL m_bEnable;
	int m_WidthAlign;
	int m_HeightAlign;
	CString m_Text;
	COLORREF m_TextColor;
	LOGFONT m_LogFont;

	void Init();
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);
	void SetIndex(int mode, CStringIndex &index);
	void DiffIndex(CTextBitMap &orig, CStringIndex &index);

	const CTextBitMap & operator = (CTextBitMap &data);
	const BOOL operator == (CTextBitMap &data);

	CTextBitMap();
	~CTextBitMap();
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

	void Add(int type, int code, LPCTSTR name);
	inline void RemoveAll() { m_Data.RemoveAll(); }
	inline int GetSize() { return (int)m_Data.GetSize(); }
	inline CProcNode & operator[](int nIndex) { return m_Data[nIndex]; }

	void Init();
	void SetIndex(int mode, CStringIndex &index);
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);

	const CProcTab & operator = (CProcTab &data);
	CProcTab();
};

class CTraceNode : public CObject
{
public:
	int m_CurX;
	int m_CurY;
	int m_Attr;
	int m_ForCol;
	int m_BakCol;
	int m_Flag;
	int m_Index;
	CString m_Name;
	CBuffer m_Buffer;
	class CTextSave *m_pSave;
	class CTraceNode *m_Next;
	class CTraceNode *m_Back;

	CTraceNode();
	~CTraceNode();
};

typedef struct _SAVEPARAM {
	EXTVRAM m_AttNow;
	EXTVRAM m_AttSpc;

	int m_CurX;
	int m_CurY;
	int m_TopY;
	int m_BtmY;
	int m_LeftX;
	int m_RightX;

	BOOL m_DoWarp;
	BOOL m_Exact;
	DWORD m_LastChar;
	int m_LastFlag;
	POINT m_LastPos;
	POINT m_LastCur;
	int m_LastSize;
	int m_LastAttr;
	WCHAR m_LastStr[MAXCHARSIZE + 2];
	BOOL m_bRtoL;
	BOOL m_bJoint;

	int m_BankGL;
	int m_BankGR;
	int m_BankSG;
	WORD m_BankTab[5][4];

	BOOL m_Decom;
	BOOL m_Decawm;
	DWORD m_AnsiOpt[16];

	void *m_TabMap;
} SAVEPARAM;

class CTextSave : public CObject
{
public:
	class CTextSave *m_pNext;
	class CTextRam *m_pTextRam;

	BOOL m_bAll;

	CCharCell *m_pCharCell;
	EXTVRAM m_AttNow;
	EXTVRAM m_AttSpc;

	int m_Cols;
	int m_Lines;
	int m_CurX;
	int m_CurY;
	int m_TopY;
	int m_BtmY;
	int m_LeftX;
	int m_RightX;

	BOOL m_DoWarp;
	BOOL m_Exact;
	DWORD m_LastChar;
	int m_LastFlag;
	POINT m_LastPos;
	POINT m_LastCur;
	int m_LastSize;
	int m_LastAttr;
	WCHAR m_LastStr[MAXCHARSIZE + 2];
	BOOL m_bRtoL;
	BOOL m_bJoint;

	DWORD m_AnsiOpt[16];

	int m_BankGL;
	int m_BankGR;
	int m_BankSG;
	WORD m_BankTab[5][4];

	void *m_TabMap;
	SAVEPARAM m_SaveParam;

	int m_StsMode;
	int m_StsLine;
	SAVEPARAM m_StsParam;

	int m_XtMosPointMode;
	DWORD m_XtOptFlag;
	int m_DispCaret;
	int m_TypeCaret;

	CWordArray m_GrapIdx;

	CTextSave();
	~CTextSave();
};

class CTabFlag : public CObject
{
public:
	struct _TabFlagParam {
		int Cols;
		int Line;
		int Element;
		int Size;
	} m_Param;
	int m_DefTabSize;
	BYTE *m_pData;
	BYTE *m_pSingleColsFlag;
	BYTE *m_pMultiColsFlag;
	BYTE *m_pLineFlag;

	void InitTab();
	void ResetAll(int cols_max, int line_max, int def_tab);
	void SizeCheck(int cols_max, int line_max, int def_tab);

	BOOL IsSingleColsFlag(int x);
	BOOL IsMultiColsFlag(int x, int y);
	inline BOOL IsColsFlag(int x, int y, BOOL bMulti) { if ( bMulti ) return IsMultiColsFlag(x, y); else return IsSingleColsFlag(x); }
	BOOL IsLineFlag(int y);

	void SetSingleColsFlag(int x);
	void ClrSingleColsFlag(int x);
	void SetMultiColsFlag(int x, int y);
	void ClrMultiColsFlag(int x, int y);
	void SetLineflag(int y);
	void ClrLineflag(int y);

	void AllClrSingle();
	void AllClrMulti();
	void LineClrMulti(int y);
	void AllClrLine();

	void CopyLine(int dy, int sy);

	void *SaveFlag(void *pData);
	void LoadFlag(void *pData);
	void *CopyFlag(void *pData);

	CTabFlag();
	~CTabFlag();
};

typedef struct _CmdHistory {
	time_t st;
	time_t et;
	CString user;
	CString curd;
	CString cmds;
	CString exit;
	BOOL emsg;
	int habs;
} CMDHIS;

class CTextRam : public COptObject
{
public:	// Options
	COLORREF m_DefColTab[16];
	CString m_SendCharSet[5];
	DWORD m_DefAnsiOpt[16];
	int m_DefCols[2];
	int m_DefHisMax;
	int m_DefFontSize;
	int m_FontSize;
	int m_DefFontHw;
	EXTVRAM m_DefAtt;
	int m_KanjiMode;
	int m_BankGL;
	int m_BankGR;
	WORD m_DefBankTab[5][4];
	int m_WheelSize;
	CString m_BitMapFile;
	int m_BitMapAlpha;
	int m_BitMapBlend;
	int m_BitMapStyle;
	int m_DelayUSecChar;	// us
	int m_DelayMSecLine;	// ms
	int m_DelayMSecRecv;
	int m_DelayMSecCrLf;
	CString m_HisFile;
	int m_SshKeepAlive;
	int m_TelKeepAlive;
	CString m_LogFile;
	int m_DropFileMode;
	CString m_DropFileCmd[8];
	CString m_WordStr;
	int m_MouseTrack;
	BYTE m_MouseMode[4];
	CRect m_MouseRect;
	int m_MouseOldSw;
	CPoint m_MouseOldPos;
	DWORD m_MetaKeys[256 / 32];
	CProcTab m_ProcTab;
	CBuffer m_FuncKey[FKEY_MAX];
	int m_ClipFlag;
	int m_ClipCrLf;
	CStringArrayExt m_ShellExec;
	int m_DefModKey[MODKEY_MAX];
	CRect m_ScrnOffset;
	CString m_TimeFormat;
	CString m_DefFontName[16];
	CString m_TraceLogFile;
	int m_TraceMaxCount;
	int m_DefTypeCaret;
	COLORREF m_DefCaretColor;
	int m_FixVersion;
	int m_SleepMax;
	int m_LogMode;
	int m_DefTermPara[5];
	CString m_GroupCast;
	CStringArrayExt m_InlineExt;
	CString m_TitleName;
	int m_RtfMode;
	int m_RecvCrLf;
	int m_SendCrLf;
	BOOL m_bInit;
	int m_SleepMode;
	int m_ImeTypeCaret;
	COLORREF m_ImeCaretColor;

	void Init();
	void SetIndex(int mode, CStringIndex &index);
	void DiffIndex(CTextRam &orig, CStringIndex &index);
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);
	void ScriptInit(int cmds, int shift, class CScriptValue &value);
	void ScriptTable(const struct _ScriptCmdsDefs *defs, class CScriptValue &value, int mode);
	void ScriptValue(int cmds, class CScriptValue &value, int mode);
	void Serialize(int mode);
	void Serialize(int mode, CBuffer &buf);
	void Serialize(CArchive& ar);

public:
	class CRLoginDoc *m_pDocument;
	class CServerEntry *m_pServerEntry;
	class CFontTab m_FontTab;
	class CTextBitMap m_TextBitMap;

	CMemMap *m_pMemMap;

	BOOL m_bOpen;
	EXTVRAM m_AttSpc;
	EXTVRAM m_AttNow;

	int m_Cols;
	int m_Lines;
	int m_CurX;
	int m_CurY;
	int m_TopY;
	int m_BtmY;
	int m_LeftX;
	int m_RightX;
	int m_DefTab;
	COLORREF *m_ColTab;
	DWORD m_AnsiOpt[16];
	DWORD m_OptTab[16];
	int m_ModKey[MODKEY_MAX];
	CString m_ModKeyList[MODKEY_MAX];
	char m_ModKeyTab[256];

	BOOL m_bSixelColInit;
	COLORREF *m_pSixelColor;
	BYTE *m_pSixelAlpha;

	int m_ColsMax;
	int m_LineUpdate;
	BOOL m_bReSize;
	int m_ReqCols;
	int m_ReqLines;
	int m_CharWidth;
	int m_CharHeight;

	int m_HisMax;
	int m_HisPos;
	int m_HisLen;
	int m_HisUse;
	int m_HisAbs;
	CFileExt m_HisFhd;

	BOOL m_DispCaret;
	int m_TypeCaret;
	COLORREF m_CaretColor;

	BOOL m_DoWarp;
	BOOL m_Exact;
	DWORD m_LastChar;
	int m_LastFlag;
	CPoint m_LastPos;
	CPoint m_LastCur;
	int m_LastSize;
	int m_LastAttr;
	CStringW m_LastStr;
	BOOL m_bRtoL;
	BOOL m_bJoint;
	time_t m_Atime;

	DWORD m_BackChar;
	DWORD m_SurroChar;
	int m_BackMode;
	int m_Status;

	CParaIndex m_AnsiPara;
	int m_OscMode;
	int m_OscLast;
	CBuffer m_OscPara;
	CTabFlag m_TabFlag;
	BOOL m_RetSync;
	CString m_StrPara;
	DWORD m_MacroExecFlag[MACROMAX / 32];
	CBuffer *m_Macro;

	int m_FirmVer;
	int m_UnitId;
	int m_VtLevel;
	int m_TermId;
	int m_KeybId;

	int m_StsMode;
	int m_StsLine;
	CStringW m_StsText;
	SAVEPARAM m_StsParam;

	int m_StsLed;
	int m_LangMenu;
	LPCTSTR m_RetChar[7];
	int m_ImageIndex;
	int m_bOscActive;
	LPCTSTR m_OscName;
	CString m_SeqMsg;
	BOOL m_bIntTimer;
	int m_IntCounter;
	int m_MediaCopyMode;

	WORD m_BankTab[5][4];
	int m_BankNow;
	int m_BankSG;
	int m_CodeLen;

	SAVEPARAM m_SaveParam;

	void SaveParam(SAVEPARAM *pSave);
	void LoadParam(SAVEPARAM *pSave, BOOL bAll);
	void SwapParam(SAVEPARAM *pSave, BOOL bAll);
	void CopyParam(SAVEPARAM *pDis, SAVEPARAM *pSrc);

	int m_Page;
	CPtrArray m_PageTab;
	int m_AltIdx;
	CTextSave *m_pAltSave[2];

	CTextSave *m_pTextSave;
	CTextSave *m_pTextStack;
	CIConv m_IConv;
	CRect m_UpdateRect;
	BOOL m_UpdateFlag;
	BOOL m_FrameCheck;
	clock_t m_UpdateClock;

#define	LINEEDIT_NONE		0
#define	LINEEDIT_ACTIVE		1
#define	LINEEDIT_SAVE		2

	BOOL m_LineEditDisable;
	int m_LineEditMode;
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
	BOOL m_LogTimeFlag;
	int m_LogCurY;
	int m_TitleMode;
	DWORD m_XtOptFlag;
	int m_XtMosPointMode;
	CStringArrayExt m_TitleStack;
	CRect m_Margin;
	class CGrapWnd *m_pImageWnd;

	CTextRam();
	virtual ~CTextRam();
	
	// Window Fonction
	inline BOOL IsInitText() { return (m_bOpen && m_pMemMap != NULL ? TRUE : FALSE); }

	void InitText(int Width, int Height);
	void InitScreen(int cols, int lines);
	int Write(LPBYTE lpBuf, int nBufLen, BOOL *sync);
	void OnTimer(int id);
	void UpdateScrnMaxSize();

	void LineEditCwd(int sx, int sy, CStringW &cwd);
	void LineEditEcho();
	void LineEditSwitch();
	BOOL IsLineEditEnable();
	BOOL LineEdit(CBuffer &buf);
	void SetKanjiMode(int mode);

	BOOL OpenHisFile();
	void InitHistory();
	void SaveHistory();
	void SaveLogFile();

	// Serch Word
	CRegEx m_MarkReg;
	int m_MarkPos;
	int m_MarkLen;
	BOOL m_MarkEol;

	BOOL m_bSimpSerch;
	CStringD m_SimpStr;
	DWORD m_SimpPos;
	int m_SimpLen;
	COLORREF m_MarkColor;

	void HisRegCheck(DWORD ch, DWORD pos);
	void HisMarkClear();
	int HisRegMark(LPCTSTR str, BOOL bRegEx);
	int HisRegNext(int msec);
	int HisMarkCheck(int top, int line, class CRLoginView *pView);

	// TextRam
	struct DrawWork {
		int		attr, zoom;
		int		fcol, bcol;
		int		bank, font;
		int		csz;
		int		idx, stx, edx, sty;
		int		size, tlen;
		int		cols, line;
		WCHAR	*pText;
		INT		*pSpace;
		WORD	eatt;
		COLORREF frgb;
		COLORREF brgb;
		clock_t	aclock;
		BOOL	print;
		int		descent;
	};

	int IsWord(DWORD ch);
	int GetPos(int x, int y);
	BOOL IncPos(int &x, int &y);
	BOOL DecPos(int &x, int &y);
	void EditWordPos(CCurPos *sps, CCurPos *eps);
	void EditCopy(CCurPos sps, CCurPos eps, BOOL rectflag = FALSE, BOOL lineflag = FALSE);
	void EditMark(CCurPos sps, CCurPos eps, BOOL rectflag = FALSE, BOOL lineflag = FALSE);
	void GetVram(int staX, int endX, int staY, int endY, CBuffer *pBuf);
	void GetLine(int sy, CString &str);
	void MediaCopyStr(LPCTSTR str);
	void MediaCopyDchar(DWORD ch);
	void MediaCopy(int y1, int y2);
	BOOL IsEmptyLine(int sy);
	void GetCellSize(int *x, int *y);
	void GetScreenSize(int *pCx, int *pCy, int *pSx, int *pSy);

	BOOL SpeakLine(int cols, int line, CString &text, CArray<CCurPos, CCurPos &> &pos);
	BOOL SpeakCheck(CCurPos sPos, CCurPos ePos, LPCTSTR str);

	void DrawBitmap(CDC *pDestDC, CRect &rect, CDC *pSrcDC, int width, int height, DWORD dwRop);
	void DrawLine(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, BOOL bEraBack, struct DrawWork &prop, class CRLoginView *pView);
	void DrawChar(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, BOOL bEraBack, struct DrawWork &prop, class CRLoginView *pView);
	void DrawHoriLine(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, struct DrawWork &prop, class CRLoginView *pView);
	void DrawVertLine(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, struct DrawWork &prop, class CRLoginView *pView);
	void DrawOverChar(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, struct DrawWork &prop, class CRLoginView *pView);
	void DrawString(CDC *pDC, CRect &rect, struct DrawWork &prop, class CRLoginView *pView);
	void DrawVram(CDC *pDC, int x1, int y1, int x2, int y2, class CRLoginView *pView, BOOL bPrint);

	CWnd *GetAciveView();
	void PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

	BOOL IsOptEnable(int opt);
	void EnableOption(int opt);
	void DisableOption(int opt);
	void ReversOption(int opt);
	int IsOptValue(int opt, int len);
	void SetOptValue(int opt, int len, int value);
	inline void SetOption(int opt, BOOL sw) { if ( sw ) EnableOption(opt); else DisableOption(opt); }
	int InitDefParam(BOOL bCheck, int modFlag = (-1));
	void InitModKeyTab();

	inline CCurPos GetCalcPos(int x, int y) { return CCurPos(x, y + m_HisPos + m_HisMax); }
	inline void SetCalcPos(CCurPos pos, int *x, int *y) { *x = pos.cx; *y = pos.cy - m_HisPos - m_HisMax; }
	inline int GetDm(int y) { return GETVRAM(0, y)->m_Vram.zoom; }
	inline void SetDm(int y, int dm) { GETVRAM(0, y)->m_Vram.zoom = dm; }
	inline void SetRet(int y) { GETVRAM(0, y)->m_Vram.attr |= ATT_RETURN; }

	inline COLORREF GetCharColor(EXTVRAM &evram) { return ((evram.eatt & EATT_FRGBCOL) != 0 ? evram.frgb : m_ColTab[evram.std.fcol]); }
	inline COLORREF GetBackColor(EXTVRAM &evram) { return ((evram.eatt & EATT_BRGBCOL) != 0 ? evram.brgb : m_ColTab[evram.std.bcol]); }

	inline int GetLeftMargin() { return (IsOptEnable(TO_DECLRMM) ? m_LeftX : 0); }
	inline int GetRightMargin() { return (IsOptEnable(TO_DECLRMM) ? m_RightX : m_Cols); }
	inline int GetTopMargin() { return m_TopY; }
	inline int GetBottomMargin() { return m_BtmY; }
	inline int GetLineSize() { return (m_Lines - m_StsLine); }

	BOOL GetMargin(int bCheck = MARCHK_NONE);

	void OnClose();
	void CallReceiveLine(int y);
	BOOL CallReceiveChar(DWORD ch, LPCTSTR name = NULL);
	int UnicodeCharFlag(DWORD code);
	int UnicodeWidth(DWORD code);
	void SetRetChar(BOOL f8);

	// Static Lib
	static int JapanCharSet(LPCTSTR name);
	static DWORD IconvToMsUnicode(int jpset, DWORD code);
	static DWORD IconvToMsUnicode(LPCTSTR charset, DWORD code) { return IconvToMsUnicode(JapanCharSet(charset), code); }
	static void IconvToMsUniStr(LPCTSTR charset, LPWSTR str, int len);
	static DWORD MsToIconvUnicode(int jpset, DWORD code);
	static DWORD MsToIconvUnicode(LPCTSTR charset, DWORD code) { return MsToIconvUnicode(JapanCharSet(charset), code); }
	static void MsToIconvUniStr(LPCTSTR charset, LPWSTR str, int len);
	static DWORD UCS2toUCS4(DWORD code);
	static DWORD UCS4toUCS2(DWORD code);
	static void UCS4ToWStr(DWORD code, CStringW &str);
	static DWORD UnicodeNomal(DWORD code1, DWORD code2);
	static void UnicodeNomalStr(LPCWSTR p, int len, CBuffer &out);
	static int OptionToIndex(int value, BOOL bAnsi = FALSE);
	static int IndexToOption(int value);
	static void OptionString(int value, CString &str);
	static void IncDscs(int &Pcss, CString &str);
	static void GetCurrentTimeFormat(LPCTSTR fmt, CString &str);

	// Low Level
	void RESET(int mode);
	CCharCell *GETVRAM(int cols, int lines);
	void UNGETSTR(LPCTSTR str, ...);
	void BEEP();
	void FLUSH();
	void CUROFF();
	void CURON();
	void DISPVRAM(int sx, int sy, int w, int h);
	void DISPUPDATE();
	void DISPRECT(int sx, int sy, int ex, int ey);
	int GETCOLIDX(int red, int green, int blue);

	// Mid Level
	int GetAnsiPara(int index, int defvalue, int minvalue, int maxvalue = -1);
	void SetAnsiParaArea(int top);
	LPCTSTR GetHexPara(LPCTSTR str, CBuffer &buf);
	void AddOscClipCrLf(CString &text);
	void SetStatusMode(int fMode, int nLine = 1);

	void LOCATE(int x, int y);
	void ERABOX(int sx, int sy, int ex, int ey, int df = 0);
	void FORSCROLL(int sx, int sy, int ex, int ey);
	void BAKSCROLL(int sx, int sy, int ex, int ey);
	void LEFTSCROLL();
	void RIGHTSCROLL();
	void DOWARP();
	void INSCHAR(BOOL bMargin = TRUE);
	void DELCHAR();
	void FILLCHAR(int ch);
	void ONEINDEX();
	void REVINDEX();
	void PUT1BYTE(DWORD ch, int md, int at = 0, LPCWSTR str = NULL);
	void PUT2BYTE(DWORD ch, int md, int at = 0, LPCWSTR str = NULL);
	void PUTADD(int x, int y, DWORD ch);
	void INSMDCK(int len);
	void ANSIOPT(int opt, int bit);
	CTextSave *GETSAVERAM(int fMode);
	void SETSAVERAM(CTextSave *pSave);
	void SAVERAM();
	void LOADRAM();
	void PUSHRAM();
	void POPRAM();
	void ALTRAM(int idx);
	void SETPAGE(int page);
	CTextSave *GETPAGE(int page);
	void COPY(int sp, int sx, int sy, int ex, int ey, int dp, int dx, int dy);
	void TABSET(int sw);
	void PUTSTR(LPBYTE lpBuf, int nBufLen);

	typedef struct _PROCTAB {
		BYTE	sc, ec;
		void (CTextRam::*proc)(DWORD ch);
	} PROCTAB;

	typedef struct _CSIEXTTAB {
		DWORD	code;
		void (CTextRam::*proc)(DWORD ch);
	} CSIEXTTAB;

	typedef struct _ESCNAMEPROC {
		LPCTSTR	name;
		union {
			void (CTextRam::*proc)(DWORD ch);
			BYTE byte[sizeof(void (CTextRam::*)(DWORD))];
		} data;
		struct _ESCNAMEPROC	*next;
		BYTE	type;
		BYTE	flag;
	} ESCNAMEPROC;

	void (CTextRam::**m_Func)(DWORD ch);
	int m_Stage;
	int m_StPos;
	int m_Stack[16];
	void (CTextRam::*m_LocalProc[5][256])(DWORD ch);
	CArray<CTextRam::CSIEXTTAB, CTextRam::CSIEXTTAB &> m_CsiExt;
	CArray<CTextRam::CSIEXTTAB, CTextRam::CSIEXTTAB &> m_DcsExt;

	void fc_Init_Proc(int stage, const PROCTAB *tp, int b = 0);
	int fc_ProcHash(void (CTextRam::*proc)(DWORD ch));
	void fc_InitProcName(CTextRam::ESCNAMEPROC *tab, int max);
	void fc_Init(int mode);

	// Trace Func
	class CTraceDlg *m_pTraceWnd;
	class CTraceNode *m_pTraceTop;
	class CTraceNode *m_pTraceNow;
	void (CTextRam::*m_TraceFunc)(DWORD ch);
	void (CTextRam::*m_pCallPoint)(DWORD ch);
	ESCNAMEPROC *m_pTraceProc;
	int m_TraceLogMode;
	int m_TraceSaveCount;
	BOOL m_bTraceUpdate;
	BOOL m_bTraceActive;
	CBuffer m_TraceSendBuf;

	void SetTraceLog(BOOL bSw);
	void fc_TraceLogChar(DWORD ch);
	void fc_TraceLogParam(CString &name);
	void fc_TraceLogFlush(ESCNAMEPROC *pProc, BOOL bParam);
	void fc_TraceCall(DWORD ch);
	void fc_FuncCall(DWORD ch) { (this->*m_Func[ch])(ch); }
	inline void fc_RenameCall(DWORD ch) { m_TraceFunc = m_Func[ch]; (this->*m_TraceFunc)(ch); }

	// Proc
	inline void fc_Call(DWORD ch) { (this->*m_pCallPoint)(ch); }
	void fc_Case(int stage);
	void fc_Push(int stage);

	ESCNAMEPROC *FindProcName(void (CTextRam::*proc)(DWORD ch));

	void EscNameProc(DWORD ch, LPCTSTR name);
	LPCTSTR EscProcName(void (CTextRam::*proc)(DWORD ch));
	void SetEscNameCombo(CComboBox *pCombo);

	void CsiNameProc(int code, LPCTSTR name);
	LPCTSTR CsiProcName(void (CTextRam::*proc)(DWORD ch));
	void SetCsiNameCombo(CComboBox *pCombo);

	void DcsNameProc(int code, LPCTSTR name);
	LPCTSTR DcsProcName(void (CTextRam::*proc)(DWORD ch));
	void SetDcsNameCombo(CComboBox *pCombo);

	void EscCsiDefName(LPCTSTR *esc, LPCTSTR *csi, LPCTSTR *dcs);
	void ParseColor(int cmd, int idx, LPCTSTR para, DWORD ch);
	void ToHexStr(CBuffer &buf, CString &out);

	// Kanji Check
	int m_Kan_Pos;
	BYTE *m_Kan_Buf;

	typedef struct _KANCODEWORK {
		int		sjis_st, euc_st, utf8_st;
		int		sjis_bk, euc_bk;
		double	sjis_rs, euc_rs, utf8_rs;
	} KANCODEWORK;
	
	static int IsKanjiCode(WORD code, const WORD *tab, int len);
	static void KanjiCodeInit(KANCODEWORK *work);
	static void KanjiCodeCheck(int ch, KANCODEWORK *work);

	void fc_KANJI(DWORD ch);
	void fc_KANBRK();
	void fc_KANCHK();

	int m_ColStackUsed;
	int m_ColStackLast;
	COLORREF *m_ColStackTab[10];

	typedef struct _SGRSTACK {
		DWORD		attr;
		DWORD		mask;
		BYTE		fcol;
		BYTE		bcol;
		WORD		eatt;
		COLORREF	brgb;
		COLORREF	frgb;
	} SGRSTACK;

	CArray<SGRSTACK, SGRSTACK &> m_SgrStack;

	// Print...
	void fc_IGNORE(DWORD ch);
	void fc_POP(DWORD ch);
	void fc_TEXT(DWORD ch);
	void fc_TEXT2(DWORD ch);
	void fc_TEXT3(DWORD ch);
	void fc_SJIS1(DWORD ch);
	void fc_SJIS2(DWORD ch);
	void fc_SJIS3(DWORD ch);
	void fc_BIG51(DWORD ch);
	void fc_BIG52(DWORD ch);
	void fc_BIG53(DWORD ch);
	void fc_UTF81(DWORD ch);
	void fc_UTF82(DWORD ch);
	void fc_UTF83(DWORD ch);
	void fc_UTF84(DWORD ch);
	void fc_UTF85(DWORD ch);
	void fc_UTF86(DWORD ch);
	void fc_UTF87(DWORD ch);
	void fc_UTF88(DWORD ch);
	void fc_UTF89(DWORD ch);
	void fc_SESC(DWORD ch);
	void fc_CESC(DWORD ch);

	// Ctrl...
	void fc_SOH(DWORD ch);
	void fc_ENQ(DWORD ch);
	void fc_BEL(DWORD ch);
	void fc_BS(DWORD ch);
	void fc_HT(DWORD ch);
	void fc_LF(DWORD ch);
	void fc_VT(DWORD ch);
	void fc_FF(DWORD ch);
	void fc_CR(DWORD ch);
	void fc_SO(DWORD ch);
	void fc_SI(DWORD ch);
	void fc_DLE(DWORD ch);
	void fc_CAN(DWORD ch);
	void fc_ESC(DWORD ch);
	void fc_A3CUP(DWORD ch);
	void fc_A3CDW(DWORD ch);
	void fc_A3CLT(DWORD ch);
	void fc_A3CRT(DWORD ch);

	// ESC...
	void fc_DECHTS(DWORD ch);
	void fc_DECAHT(DWORD ch);
	void fc_DECVTS(DWORD ch);
	void fc_DECAVT(DWORD ch);
	void fc_BI(DWORD ch);
	void fc_SC(DWORD ch);
	void fc_RC(DWORD ch);
	void fc_FI(DWORD ch);
	void fc_V5CUP(DWORD ch);
	void fc_BPH(DWORD ch);
	void fc_NBH(DWORD ch);
	void fc_IND(DWORD ch);
	void fc_NEL(DWORD ch);
	void fc_SSA(DWORD ch);
	void fc_ESA(DWORD ch);
	void fc_HTS(DWORD ch);
	void fc_HTJ(DWORD ch);
	void fc_VTS(DWORD ch);
	void fc_PLD(DWORD ch);
	void fc_PLU(DWORD ch);
	void fc_RI(DWORD ch);
	void fc_STS(DWORD ch);
	void fc_CCH(DWORD ch);
	void fc_MW(DWORD ch);
	void fc_SPA(DWORD ch);
	void fc_EPA(DWORD ch);
	void fc_SCI(DWORD ch);
	void fc_RIS(DWORD ch);
	void fc_LMA(DWORD ch);
	void fc_USR(DWORD ch);
	void fc_V5EX(DWORD ch);
	void fc_DECPAM(DWORD ch);
	void fc_DECPNM(DWORD ch);
	void fc_SS2(DWORD ch);
	void fc_SS3(DWORD ch);
	void fc_LS2(DWORD ch);
	void fc_LS3(DWORD ch);
	void fc_LS1R(DWORD ch);
	void fc_LS2R(DWORD ch);
	void fc_LS3R(DWORD ch);
	void fc_GZM4(DWORD ch);
	void fc_GZD4(DWORD ch);
	void fc_G1D4(DWORD ch);
	void fc_G2D4(DWORD ch);
	void fc_G3D4(DWORD ch);
	void fc_GZD6(DWORD ch);
	void fc_G1D6(DWORD ch);
	void fc_G2D6(DWORD ch);
	void fc_G3D6(DWORD ch);
	void fc_V5MCP(DWORD ch);
	void fc_DECSOP(DWORD ch);
	void fc_ACS(DWORD ch);
	void fc_ESCI(DWORD ch);
	void fc_DOCS(DWORD ch);

	// Ext ESC
	void fc_STAT(DWORD ch);
	void fc_GOTOXY(DWORD ch);

	// ESC [ CSI
	void fc_CSI(DWORD ch);
	void fc_CSI_ESC(DWORD ch);
	void fc_CSI_RST(DWORD ch);
	void fc_CSI_DIGIT(DWORD ch);
	void fc_CSI_PUSH(DWORD ch);
	void fc_CSI_SEPA(DWORD ch);
	void fc_CSI_EXT(DWORD ch);
	void fc_CSI_ETC(DWORD ch);

	// CSI @-Z
	void fc_ICH(DWORD ch);
	void fc_CUU(DWORD ch);
	void fc_CUD(DWORD ch);
	void fc_CUF(DWORD ch);
	void fc_CUB(DWORD ch);
	void fc_CNL(DWORD ch);
	void fc_CPL(DWORD ch);
	void fc_CHA(DWORD ch);
	void fc_CUP(DWORD ch);
	void fc_CHT(DWORD ch);
	void fc_ED(DWORD ch);
	void fc_EL(DWORD ch);
	void fc_IL(DWORD ch);
	void fc_DL(DWORD ch);
	void fc_EF(DWORD ch);
	void fc_EA(DWORD ch);
	void fc_DCH(DWORD ch);
	void fc_SEE(DWORD ch);
	void fc_SU(DWORD ch);
	void fc_SD(DWORD ch);
	void fc_NP(DWORD ch);
	void fc_PP(DWORD ch);
	void fc_CTC(DWORD ch);
	void fc_ECH(DWORD ch);
	void fc_CVT(DWORD ch);
	void fc_CBT(DWORD ch);
	// CSI a-z
	void fc_HPR(DWORD ch);
	void fc_REP(DWORD ch);
	void fc_DA1(DWORD ch);
	void fc_VPA(DWORD ch);
	void fc_VPR(DWORD ch);
	void fc_HVP(DWORD ch);
	void fc_TBC(DWORD ch);
	void fc_SM(DWORD ch);
	void fc_RM(DWORD ch);
	void fc_MC(DWORD ch);
	void fc_HPB(DWORD ch);
	void fc_VPB(DWORD ch);
	void fc_SGR(DWORD ch);
	void fc_DSR(DWORD ch);
	void fc_DAQ(DWORD ch);
	void fc_RLBFAT(DWORD ch);
	void fc_DECSSL(DWORD ch);	
	void fc_DECLL(DWORD ch);
	void fc_DECSTBM(DWORD ch);
	void fc_DECSLRM(DWORD ch);
	void fc_DECSLPP(DWORD ch);
	void fc_DECSHTS(DWORD ch);
	void fc_DECSVTS(DWORD ch);
	void fc_SCOSC(DWORD ch);
	void fc_XTWINOPS(DWORD ch);
	void fc_SCORC(DWORD ch);
	void fc_RLSCD(DWORD ch);
	void fc_REQTPARM(DWORD ch);
	void fc_DECTST(DWORD ch);
	void fc_DECVERP(DWORD ch);
	void fc_SRS(DWORD ch);
	void fc_PTX(DWORD ch);
	void fc_SDS(DWORD ch);
	void fc_SIMD(DWORD ch);
	void fc_HPA(DWORD ch);
	void fc_LINUX(DWORD ch);
	void fc_DECFNK(DWORD ch);
	// CSI ? ...
	void fc_DECSED(DWORD ch);
	void fc_DECSEL(DWORD ch);
	void fc_DECST8C(DWORD ch);
	void fc_DECSET(DWORD ch);
	void fc_DECMC(DWORD ch);
	void fc_DECRST(DWORD ch);
	void fc_XTSMGRAPHICS(DWORD ch);
	void fc_XTRESTORE(DWORD ch);
	void fc_XTSAVE(DWORD ch);
	void fc_DECDSR(DWORD ch);
	void fc_DECSRET(DWORD ch);
	// CSI $ ...
	void fc_DECRQM(DWORD ch);
	void fc_DECCARA(DWORD ch);
	void fc_DECRARA(DWORD ch);
	void fc_DECRQTSR(DWORD ch);
	void fc_DECCRA(DWORD ch);
	void fc_DECRQPSR(DWORD ch);
	void fc_DECFRA(DWORD ch);
	void fc_DECERA(DWORD ch);
	void fc_DECSERA(DWORD ch);
	void fc_DECSCPP(DWORD ch);
	void fc_DECSASD(DWORD ch);
	void fc_DECSSDT(DWORD ch);
	// CSI ...
	void fc_DECRQMH(DWORD ch);
	void fc_SL(DWORD ch);
	void fc_SR(DWORD ch);
	void fc_FNT(DWORD ch);
	void fc_PPA(DWORD ch);
	void fc_PPR(DWORD ch);
	void fc_PPB(DWORD ch);
	void fc_DECSCUSR(DWORD ch);
	void fc_DECTME(DWORD ch);
	void fc_DECSTR(DWORD ch);
	void fc_DECSCL(DWORD ch);
	void fc_DECSCA(DWORD ch);
	void fc_DECRQDE(DWORD ch);
	void fc_DECRQUPSS(DWORD ch);
	void fc_DECEFR(DWORD ch);
	void fc_DECELR(DWORD ch);
	void fc_DECSLE(DWORD ch);
	void fc_DECRQLP(DWORD ch);
	void fc_DECIC(DWORD ch);
	void fc_DECDC(DWORD ch);
	void fc_DECPS(DWORD ch);
	void fc_XTPUSHCOLORS(DWORD ch);
	void fc_XTPOPCOLORS(DWORD ch);
	void fc_XTREPORTCOLORS(DWORD ch);
	void fc_XTPUSHSGR(DWORD ch);
	void fc_XTPOPSGR(DWORD ch);
	void fc_XTREPORTSGR(DWORD ch);
	void fc_DECSTGLT(DWORD ch);
	void fc_DECSACE(DWORD ch);
	void fc_DECRQCRA(DWORD ch);
	void fc_DECINVM(DWORD ch);
	void fc_DECSR(DWORD ch);
	void fc_DECPQPLFM(DWORD ch);
	void fc_DECAC(DWORD ch);
	void fc_DECTID(DWORD ch);
	void fc_DECATC(DWORD ch);
	void fc_DA2(DWORD ch);
	void fc_DA3(DWORD ch);
	void fc_C25LCT(DWORD ch);
	void fc_TTIMESV(DWORD ch);
	void fc_TTIMEST(DWORD ch);
	void fc_TTIMERS(DWORD ch);
	void fc_XTRMTITLE(DWORD ch);
	void fc_XTMODKEYS(DWORD ch);
	void fc_XTDISMODKEYS(DWORD ch);
	void fc_XTSMPOINTER(DWORD ch);
	void fc_XTSMTITLE(DWORD ch);
	void fc_XTVERSION(DWORD ch);
	void fc_RLIMGCP(DWORD ch);
	void fc_RLCURCOL(DWORD ch);
	void fc_SCOSNF(DWORD ch);
	void fc_SCOSNB(DWORD ch);

	// ESC PX^_] DCS/PM/APC/SOS/OSC
	void fc_TimerSet(LPCTSTR name);
	void fc_TimerReset();
	void fc_TimerAbort(BOOL bOut);
	void fc_DCS(DWORD ch);
	void fc_PM(DWORD ch);
	void fc_APC(DWORD ch);
	void fc_SOS(DWORD ch);
	void fc_OSC(DWORD ch);
	void fc_OSC_CMD(DWORD ch);
	void fc_OSC_PAM(DWORD ch);
	void fc_OSC_ST(DWORD ch);
	void fc_OSC_CAN(DWORD ch);
	void fc_DECUDK(DWORD ch);
	void fc_DECREGIS(DWORD ch);
	void fc_DECSIXEL(DWORD ch);
	void fc_DECDLD(DWORD ch);
	void fc_DECRSTS(DWORD ch);
	void fc_DECRQSS(DWORD ch);
	void fc_DECRSPS(DWORD ch);
	void fc_DECDMAC(DWORD ch);
	void fc_DECSTUI(DWORD ch);
	void fc_XTRQCAP(DWORD ch);
	void fc_RLMML(DWORD ch);
	void fc_OSCEXE(DWORD ch);
	void fc_OSCNULL(DWORD ch);

	// TEK
	typedef struct _TEKNODE {
		struct _TEKNODE *next;
		BYTE md, st, rv[2];
		short sx, sy, ex, ey;
		DWORD ch;
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
	void TekClear(BOOL bFlush = TRUE);
	void TekLine(int st, int sx, int sy, int ex, int ey);
	void TekText(int st, int sx, int sy, DWORD ch);
	void TekMark(int st, int no, int sx, int sy);
	void TekEdge(int ag);

	void fc_TEK_IN(DWORD ch);
	void fc_TEK_LEFT(DWORD ch);
	void fc_TEK_RIGHT(DWORD ch);
	void fc_TEK_DOWN(DWORD ch);
	void fc_TEK_UP(DWORD ch);
	void fc_TEK_FLUSH(DWORD ch);
	void fc_TEK_MODE(DWORD ch);
	void fc_TEK_STAT(DWORD ch);

	// DEC Locator
	int m_Loc_Mode;
	CRect m_Loc_Rect;
	CPoint m_MousePos;

	void LocReport(int md, int sw, int x, int y);
	void MouseReport(int md, int sw, int x, int y);

	// ReGIS/Sixel/Image
	class CGrapWnd *m_pActGrapWnd;
	class CGrapWnd *m_pWorkGrapWnd;
	clock_t m_GrapWndChkCLock;
	int m_GrapWndChkStat;
	int m_GrapWndChkPos;
	BYTE m_GrapWndUseMap[4096];
	class CGrapWnd *pGrapListIndex[8];
	class CGrapWnd *pGrapListImage[8];
	class CGrapWnd *pGrapListType;

	class CGrapWnd *GetGrapWnd(int index);
	class CGrapWnd *CmpGrapWnd(class CGrapWnd *pWnd);
	void AddGrapWnd(class CGrapWnd *pWnd);
	void RemoveGrapWnd(class CGrapWnd *pWnd);
	void *LastGrapWnd(int type);
	void ChkGrapWnd(int sec);
	void SizeGrapWnd(class CGrapWnd *pWnd, int cx, int cy, BOOL bAspect);
	void DispGrapWnd(class CGrapWnd *pGrapWnd, BOOL bNextCols);

	// iTerm2
	int m_iTerm2Mark;
	CPoint m_iTerm2MaekPos[4];
	CString m_iTerm2Prompt;
	CString m_iTerm2Command;
	CString m_iTerm2RemoteHost;
	CString m_iTerm2CurrentDir;
	CString m_iTerm2Version;
	CString m_iTerm2Shell;
	CString m_iTerm2ExitStatus;
	CStringIndex m_iTerm2SetUserVar;
	CList<CMDHIS *, CMDHIS *> m_CommandHistory;
	class CCmdHisDlg *m_pCmdHisWnd;

	void iTerm2Ext(LPCSTR param);
};

