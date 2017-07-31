// TermPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "TermPage.h"
#include "EscDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTermPage プロパティ ページ

IMPLEMENT_DYNCREATE(CTermPage, CPropertyPage)

//#define	CHECKOPTMAX		5
//#define	IDC_CHECKFAST	IDC_TERMCHECK1
//static const int CheckOptTab[] = { TO_RLBOLD, TO_RLGNDW, TO_RLGCWA, TO_RLUNIAWH, TO_RLKANAUTO };

static const LV_COLUMN InitListTab[3] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60,	_T("番号"),				0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("セット時の動作"),	0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("リセット時の動作"),	0, 0 },
	};

static const struct _OptListTab {
	int		num;
	LPCTSTR	ename;
	LPCTSTR	dname;
	LPCTSTR tip;
} OptListTab[] = {
	// ANSI Screen Option	0-99(200-299)
	{	TO_ANSIKAM,		_T("キー入力を無視"),				_T("キー入力を有効"),				NULL	},
	{	TO_ANSIIRM,		_T("挿入モード"),					_T("上書きモード"),					NULL	},
	{	TO_ANSIERM,		_T("SPAの保護を無効"),				_T("SPAの保護を有効"),				NULL	},
	{	TO_ANSISRM,		_T("ローカルエコーを無効"),			_T("ローカルエコーを有効"),			NULL	},
	{	TO_ANSITSM,		_T("各行別のタブ設定"),				_T("全行で同じタブ設定"),			NULL	},
	{	TO_ANSILNM,		_T("LF/VT/FFで行頭に移動"),			_T("LF/VT/FFで通常動作"),			NULL	},

	// DEC Terminal Option	0-199
	{	TO_DECCKM,		_T("CKM有効のキーコードを送信"),	_T("CKM無効のキーコードを送信"),	NULL	},
	{	TO_DECANM,		_T("VT100(ANSI)モード"),			_T("VT52モード"),					NULL	},
	{	TO_DECCOLM,		_T("132文字表示モード"),			_T("80文字表示モード"),				NULL	},
	{	TO_DECSCLM,		_T("スムーズスクロールモード"),		_T("ジャンプスクロールモード"),		NULL	},
	{	TO_DECSCNM,		_T("反転表示モード"),				_T("通常表示モード"),				NULL	},
	{	TO_DECOM,		_T("CUPでSTBMの影響を受ける"),		_T("CUPで常に同じ原点"),			NULL	},
	{	TO_DECAWM,		_T("行末のオートワープを有効"),		_T("行末を超える移動を行わない"),	NULL	},
	{	TO_XTMOSREP,	_T("マウスリポートを有効"),			_T("マウスリポートを無効"),			NULL	},
	{	TO_DECPEX,		_T("MCでSTBMの影響を受けない"),		_T("MCでSTRMの影響を受ける"),		NULL	},
	{	TO_DECTCEM,		_T("カーソルの表示"),				_T("カーソルを非表示"),				NULL	},
//	{	TO_DECTEK,		_T("Tekモードに移行"),				_T("Tekモードから抜ける"),			NULL	},
	{	TO_XTMCSC,		_T("132文字モード有効"),			_T("132文字モード無効"),			NULL	},
	{	TO_XTMCUS,		_T("タブ(HT)のバグ修正モード"),		_T("xtermタブバグ互換モード"),		NULL	},
	{	TO_XTMRVW,		_T("行頭BSによる上行右端移動"),		_T("行頭のBSで移動しない"),			NULL	},
	{	TO_XTMABUF,		_T("全画面の保存"),					_T("保存した画面の復帰"),			NULL	},
	{	TO_DECBKM,		_T("BSキー変換を行わない"),			_T("BSキーをDELキーに変換"),		NULL	},
	{	TO_DECLRMM,		_T("左右マージン有効"),				_T("左右マージン無効"),				NULL	},
	{	TO_DECSDM,		_T("Sixelを別ウィンドウで表示"),	_T("Sixelスクロールモード表示"),	NULL	},
	{	TO_DECNCSM,		_T("画面サイズ変更でクリアしない"),	_T("画面サイズ変更でクリアする"),	NULL	},
	{	TO_DECECM,		_T("SGRで空白文字を設定しない"),	_T("SGRで空白文字カラーを設定"),	NULL	},

	// XTerm Option			1000-1099(300-379)
	{	TO_XTNOMTRK,	_T("Normal mouse tracking"),		_T("Mouse tracking 無効"),			NULL	},
	{	TO_XTHILTRK,	_T("Hilite mouse tracking"),		_T("Mouse tracking 無効"),			NULL	},
	{	TO_XTBEVTRK,	_T("Button-event mouse tracking"),	_T("Mouse tracking 無効"),			NULL	},
	{	TO_XTAEVTRK,	_T("Any-event mouse tracking"),		_T("Mouse tracking 無効"),			NULL	},
	{	TO_XTFOCEVT,	_T("フォーカスイベントの有効"),		_T("フォーカスイベント無効"),		NULL	},
	{	TO_XTEXTMOS,	_T("Extended Mouse Mode"),			_T("Disable Extended Mode"),		NULL	},
	{	TO_XTSGRMOS,	_T("SGR Mouse Mode"),				_T("Disable SGR Mode"),				NULL	},
	{	TO_XTURXMOS,	_T("URXVT Mouse Mode"),				_T("Disable URXVT Mode"),			NULL	},
	{	TO_XTALTSCR,	_T("全画面の保存"),					_T("保存した画面の復帰"),			NULL	},
	{	TO_XTSRCUR,		_T("カーソル位置の保存"),			_T("保存したカーソル位置の復帰"),	NULL	},
	{	TO_XTALTCLR,	_T("拡張画面に切り換え"),			_T("標準画面に切り換え"),			NULL	},

	// XTerm Option 2		2000-2019(380-399)
	{	TO_XTBRPAMD,	_T("Bracketed Paste Mode 有効"),	_T("Bracketed Paste Mode 無効"),	NULL	},

	// RLogin Option		8400-8511(400-511)
	{	TO_RLGCWA,		_T("SGRで空白属性を設定しない"),	_T("SGRで空白文字属性を設定"),		_T("SGR(ESC[m)により変更される文字属性を空白文字にも適用します。\nこの設定は、FreeBSDのcons25の仕様に合わせる物です")	},
	{	TO_RLGNDW,		_T("行末での遅延改行無効"),			_T("行末での遅延改行有効"),			_T("行末で文字表示の自動改行を次の文字表示まで遅らせます\nこの設定は、FreeBSDのcons25の仕様に合わせる物です")	},
//	{	TO_RLGAWL,		_T("自動ブラウザ起動をする"),		_T("自動ブラウザ起動無効"),			NULL	},
	{	TO_RLBOLD,		_T("重ね書きでボールドを表示"),		_T("高輝度でボールドを表示"),		_T("重ね書きでボールド(太文字)を表示します。\nただし字体が崩れて表示されてしまいます")	},
//	{	TO_RLBPLUS,		_T("BPlus/ZModem/Kermit自動"),		_T("自動ファイル転送無効"),			NULL	},
	{	TO_RLALTBDIS,	_T("画面のスタックを禁止"),			_T("画面のスタックが有効"),			_T("DECSET/DECRST(CSI?47h/l)で現在の画面をスタックに積みます。\nオン時の動作(１つの拡張画面の切り替え)がxtermの動作です")	},
	{	TO_RLUNIAWH,	_T("Aタイプを半角で表示"),			_T("Aタイプを全角で表示"),			_T("UNICODE仕様書のEastAsianWidth.txtでAタイプ(Ambiguous)で定義された文字の表示幅を設定します。フォントによりかなり違いがあります")	},
//	{	TO_RLNORESZ,	_T("DECCOLMでリサイズ"),			_T("ウィンドウをリサイズしない"),	NULL	},
	{	TO_RLKANAUTO,	_T("漢字コードを自動追従"),			_T("漢字コードを変更しない"),		_T("漢字コードがエラーを起こした場合に自動で変更します。\nエラーを起こすまで変更しないので文字化けは、起こります")	},
	{	TO_RLPNAM,		_T("ノーマルモード(DECPNM)"),		_T("アプリモード(DECPAM)"),			NULL	},
	{	TO_IMECTRL,		_T("IMEオープン"),					_T("IMEクロース"),					NULL	},
	{	TO_RLCKMESC,	_T("7727 ESCキーのCKM有効"),		_T("ESCキーのCKM無効"),				_T("DECCKMによるキーコードの変更にESCキーを含めます。\n本来ESCのキーコードは'\\033'だけを返しますがCKM時に'\\0330['を返すようにします")	},
//	{	TO_RLMSWAPE,	_T("7786 ホイールのキー変換"),		_T("ホイールの通常動作"),			NULL	},
	{	TO_RLTEKINWND,	_T("Tekをコンソールで表示"),		_T("Tekコンソールを無効"),			NULL	},
	{	TO_RLUNINOM,	_T("UTF-8ノーマライズを禁止"),		_T("UTF-8ノーマライズを行う"),		_T("UNICODEの文字変換処理をしないでそのままの形で表示するようにします。")	},
	{	TO_RLUNIAHF,	_T("Unicde半角の調整をしない"),		_T("Unicde半角の調整をする"),		_T("一部のフォントで半角・全角になるように調整して表示ます")	},
//	{	TO_RLMODKEY,	_T("modifyKeysを優先する"),			_T("ショートカットを優先"),			NULL	},
	{	TO_RLDRWLINE,	_T("罫線を文字で表示する"),			_T("罫線を線で表示する"),			_T("罫線を線で描画せずにフォントの持つ本来の文字として表示します")	},
	{	TO_RLSIXPOS,	_T("Sixel画像の右にカーソル"),		_T("Sixel画像の下にカーソル"),		_T("スクロールモード時のSixel画像表示後のカーソル位置を設定します")	},
	{	TO_DRCSMMv1,	_T("8800 Unicodeマッピング有効"),	_T("Unicodeマッピング無効"),		_T("ISO-2022コードセットをUnicode16面にマッピングを有効・無効にします")	},

	{	0,				NULL,								NULL,								NULL	}
}, ExtListTab[] = {
	// RLogin SockOpt		1000-1511(0-511)
//	{	TO_RLTENAT,		_T("自動ユーザー認証を禁止"),		_T(""),								NULL	},
//	{	TO_RLTENEC,		_T("データの暗号化を禁止する"),		_T(""),								NULL	},
//	{	TO_RLPOFF,		_T("自動でプログラムを終了"),		_T(""),								NULL	},
//	{	TO_RLUSEPASS,	_T("パスワード入力を求める"),		_T(""),								NULL	},
//	{	TO_RLRCLICK,	_T("右ダブルクリックでペースト"),	_T(""),								NULL	},
//	{	TO_RLCKCOPY,	_T("左クリックだけでコピー"),		_T(""),								NULL	},
//	{	TO_RLDSECHO,	_T("ローカルエディットモード"),		_T(""),								NULL	},
//	{	TO_RLDELAY,		_T("指定時間待って次を送信"),		_T(""),								NULL	},
//	{	TO_RLHISFILE,	_T("ヒストリーを保存・復帰"),		_T(""),								NULL	},
//	{	TO_RLKEEPAL,	_T("KeepAliveを有効にする"),		_T(""),								NULL	},
//	{	TO_SSH1MODE,	_T("SSHバージョン１で接続"),		_T(""),								NULL	},
//	{	TO_SSHPFORY,	_T("ポートフォワードだけ行う"),		_T(""),								NULL	},
//	{	TO_SSHKEEPAL,	_T("KeepAliveパケットの送信"),		_T(""),								NULL	},
//	{	TO_SSHAGENT,	_T("エージェント転送を有効"),		_T(""),								NULL	},
//	{	TO_SSHSFENC,	_T("暗号シャッフル"),				_T(""),								NULL	},
//	{	TO_SSHSFMAC,	_T("検証シャッフル"),				_T(""),								NULL	},
//	{	TO_RLHISDATE,	_T("信ログを自動作成"),				_T(""),								NULL	},
//	{	TO_RLLOGMODE,	_T("通信ログの形式1"),				_T(""),								NULL	},
//	{	TO_RLLOGMOD2,	_T("通信ログの形式2"),				_T(""),								NULL	},
//	{	TO_RLLOGCODE,	_T("ログの文字コード1"),			_T(""),								NULL	},
//	{	TO_RLLOGCOD2,	_T("ログの文字コード2"),			_T(""),								NULL	},
//	{	TO_SSHX11PF,	_T("X11ポートフォワード有効"),		_T(""),								NULL	},
//	{	TO_RLTENLM,		_T("TELNET LINEMODE を禁止"),		_T(""),								NULL	},
//	{	TO_RLSCRDEBUG,	_T("スクリプトデバック有効"),		_T(""),								NULL	},
	{	TO_RLCURIMD,	_T("カーソルをIで表示"),			_T("カーソルを通常表示"),			_T("カーソルが画面内の場合にIで表示するようにします。\n制御コードによるマウスコントロール中には、矢印とIが入れ替わります")	},
	{	TO_RLRSPAST,	_T("右クリックでペースト"),			_T("右クリックでメニュー選択"),		_T("画面内で右クリックした場合の動作を変更します。\nクリップボードオプションの右ダブルクリックの設定と重なりますので注意してください")	},
	{	TO_RLGWDIS,		_T("ゴースト表示しない"),			_T("タブにマウスでゴースト表示"),	_T("この接続でのゴースト表示を禁止します。\nすべて表示しないようにするにはレジストリを直接操作する必要があります。\nHCU\\Software\\Culti\\RLogin\\TabBar\\GhostWnd")	},
	{	TO_RLMWDIS,		_T("サイズを表示しない"),			_T("画面サイズを表示する"),			_T("画面サイズが変更されると画面中央に横ｘ縦サイズを自動で表示します。\n表示は、約３秒ほどで自動で消えます")	},
	{	TO_RLGRPIND,	_T("イメージを全表示する"),			_T("イメージを部分表示する"),		_T("Sixel/Imageを外部ウィンドウで表示する場合の表示方法を選択します\n全表示では、上下/左右に余白が表示されます")	},
	{	0,				NULL,								NULL,								NULL	}
};

CTermPage::CTermPage() : CTreePropertyPage(CTermPage::IDD)
{
	//for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
	//	m_Check[n] = FALSE;
}

CTermPage::~CTermPage()
{
}

void CTermPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	//for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
	//	DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);
	DDX_Control(pDX, IDC_ESCLIST, m_OptList);
	DDX_Control(pDX, IDC_EXTLIST, m_ExtList);
}

BEGIN_MESSAGE_MAP(CTermPage, CPropertyPage)
//	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_BN_CLICKED(IDC_ESCEDIT, &CTermPage::OnBnClickedEscedit)
	ON_NOTIFY(NM_CLICK, IDC_ESCLIST, &CTermPage::OnNMClickOptlist)
	ON_NOTIFY(NM_CLICK, IDC_EXTLIST, &CTermPage::OnNMClickExtlist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_ESCLIST, &CTermPage::OnLvnGetInfoTipEsclist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_EXTLIST, &CTermPage::OnLvnGetInfoTipExtlist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTermPage メッセージ ハンドラ

void CTermPage::DoInit()
{
	int n, i;

	//for ( n = 0 ; n < CHECKOPTMAX ; n++ )
	//	m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	for ( i = 0 ; i < m_OptList.GetItemCount() ; i++ ) {
		n = m_OptList.GetItemData(i);
		m_OptList.SetLVCheck(i,  m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) ? TRUE : FALSE);
	}

	for ( i = 0 ; i < m_ExtList.GetItemCount() ; i++ ) {
		n = m_ExtList.GetItemData(i);
		m_ExtList.SetLVCheck(i,  m_pSheet->m_pTextRam->IsOptEnable(ExtListTab[n].num) ? TRUE : FALSE);
	}

	UpdateData(FALSE);
}
BOOL CTermPage::OnInitDialog() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();

	int n;
	CString str;

	m_OptList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_INFOTIP);
	m_OptList.InitColumn(_T("TermPageOpt"), InitListTab, 3);

	m_OptList.DeleteAllItems();
	for ( n = 0 ; OptListTab[n].ename != NULL ; n++ ) {
		if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
			str.Format(_T("?%d"), OptListTab[n].num);
		} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
			str.Format(_T("%d"), OptListTab[n].num - 200);
		} else if ( OptListTab[n].num < 380 ) {		// XTerm Option			1000-1079(300-379)
			str.Format(_T("?%d"), OptListTab[n].num + 700);
		} else if ( OptListTab[n].num < 400 ) {		// XTerm Option 2		2000-2019(380-399)
			str.Format(_T("?%d"), OptListTab[n].num + 1620);
		} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		8400-8511(400-511)
			str.Format(_T("?%d"), OptListTab[n].num + 8000);
		}
		m_OptList.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_OptList.SetItemText(n, 1, OptListTab[n].ename);
		m_OptList.SetItemText(n, 2, OptListTab[n].dname);
	}

	m_ExtList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_INFOTIP);
	m_ExtList.InitColumn(_T("TermPageExt"), InitListTab, 3);

	m_ExtList.DeleteAllItems();
	for ( n = 0 ; ExtListTab[n].ename != NULL ; n++ ) {
		str.Format(_T("%d"), ExtListTab[n].num - 1000);
		m_ExtList.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_ExtList.SetItemText(n, 1, ExtListTab[n].ename);
		m_ExtList.SetItemText(n, 2, ExtListTab[n].dname);
	}

	DoInit();

	return TRUE;
}
BOOL CTermPage::OnApply() 
{
	int n, i;

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	UpdateData(TRUE);

	m_pSheet->m_pTextRam->m_ProcTab = m_ProcTab;

	for ( i = 0 ; i < m_OptList.GetItemCount() ; i++ ) {
		n = m_OptList.GetItemData(i);

		if ( m_OptList.GetLVCheck(i) ) {
			if ( m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) )
				continue;
			if ( m_pSheet->m_pTextRam->m_VRam == NULL || m_pSheet->m_pTextRam->m_pDocument == NULL ) {
				m_pSheet->m_pTextRam->EnableOption(OptListTab[n].num);
				continue;
			}
			m_pSheet->m_pTextRam->m_AnsiPara.RemoveAll();
			if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
				m_pSheet->m_pTextRam->EnableOption(OptListTab[n].num);
			} else if ( OptListTab[n].num < 380 ) {		// XTerm Option			1000-1079(300-379)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 700);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 400 ) {		// XTerm Option 2		2000-2019(380-399)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 1620);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		8400-8511(400-511)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 8000);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('h');
			}
		} else {
			if ( !m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) )
				continue;
			if ( m_pSheet->m_pTextRam->m_VRam == NULL || m_pSheet->m_pTextRam->m_pDocument == NULL ) {
				m_pSheet->m_pTextRam->DisableOption(OptListTab[n].num);
				continue;
			}
			m_pSheet->m_pTextRam->m_AnsiPara.RemoveAll();
			if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
				m_pSheet->m_pTextRam->DisableOption(OptListTab[n].num);
			} else if ( OptListTab[n].num < 380 ) {		// XTerm Option			1000-1079(300-379)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 700);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 400 ) {		// XTerm Option 2		2000-2019(380-399)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 1620);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		8400-8511(400-511)
				m_pSheet->m_pTextRam->m_AnsiPara.Add(OptListTab[n].num + 8000);
				m_pSheet->m_pTextRam->fc_Push(STAGE_CSI);
				m_pSheet->m_pTextRam->fc_DECSRET('l');
			}
		}
	}

	for ( i = 0 ; i < m_ExtList.GetItemCount() ; i++ ) {
		n = m_ExtList.GetItemData(i);

		m_pSheet->m_pTextRam->SetOption(ExtListTab[n].num, m_ExtList.GetLVCheck(i) ? TRUE : FALSE);
	}


	//for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
	//	m_pSheet->m_pTextRam->SetOption(CheckOptTab[n], m_Check[n]);

	return TRUE;
}
void CTermPage::OnReset() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	DoInit();
	SetModified(FALSE);
}

//void CTermPage::OnUpdateCheck(UINT nID) 
//{
//	SetModified(TRUE);
//	m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_ANSIOPT);
//}

void CTermPage::OnBnClickedEscedit()
{
	CEscDlg dlg;

	dlg.m_pTextRam = m_pSheet->m_pTextRam;
	dlg.m_pProcTab = &m_ProcTab;

	if ( dlg.DoModal() != IDOK )
		return;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

void CTermPage::OnNMClickOptlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if ( pNMItemActivate->iSubItem == 0 ) {
		SetModified(TRUE);
		m_pSheet->m_ModFlag |= (UMOD_TEXTRAM | UMOD_ANSIOPT);
	}

	*pResult = 0;
}

void CTermPage::OnNMClickExtlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if ( pNMItemActivate->iSubItem == 0 ) {
		SetModified(TRUE);
		m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
	}

	*pResult = 0;
}


void CTermPage::OnLvnGetInfoTipEsclist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	int n = m_OptList.GetItemData(pGetInfoTip->iItem);
	CString str;

	if ( OptListTab[n].tip != NULL )
		str = OptListTab[n].tip;
	else
		str.Format(_T("%s/%s"), OptListTab[n].ename, OptListTab[n].dname);

	if ( (n = str.GetLength() + sizeof(TCHAR)) > pGetInfoTip->cchTextMax )
		n = pGetInfoTip->cchTextMax;

	lstrcpyn(pGetInfoTip->pszText, str, n);

	*pResult = 0;
}


void CTermPage::OnLvnGetInfoTipExtlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	int n = m_ExtList.GetItemData(pGetInfoTip->iItem);
	CString str;

	if ( ExtListTab[n].tip != NULL )
		str = ExtListTab[n].tip; 
	else
		str.Format(_T("%s/%s"), ExtListTab[n].ename, ExtListTab[n].dname);

	if ( (n = str.GetLength() + sizeof(TCHAR)) > pGetInfoTip->cchTextMax )
		n = pGetInfoTip->cchTextMax;

	lstrcpyn(pGetInfoTip->pszText, str, n);

	*pResult = 0;
}
