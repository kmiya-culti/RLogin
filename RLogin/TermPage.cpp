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

#define	CHECKOPTMAX		5
#define	IDC_CHECKFAST	IDC_TERMCHECK1
static const int CheckOptTab[] = { TO_RLBOLD, TO_RLGNDW, TO_RLGCWA, TO_RLUNIAWH, TO_RLKANAUTO };

static const LV_COLUMN InitListTab[3] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60,	_T("番号"),				0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("セット時の動作"),	0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	_T("リセット時の動作"),	0, 0 },
	};

static const struct _OptListTab {
	int		num;
	LPCTSTR	ename;
	LPCTSTR	dname;
} OptListTab[] = {
	// ANSI Screen Option	0-99(200-299)
	{	TO_ANSIKAM,		_T("キー入力を無視"),				_T("キー入力を有効")				},
	{	TO_ANSIIRM,		_T("挿入モード"),					_T("上書きモード")					},
	{	TO_ANSIERM,		_T("SPAの保護を無効"),				_T("SPAの保護を有効")				},
	{	TO_ANSISRM,		_T("ローカルエコーを無効"),			_T("ローカルエコーを有効")			},
	{	TO_ANSITSM,		_T("各行別のタブ設定"),				_T("全行で同じタブ設定")			},
	{	TO_ANSILNM,		_T("LF/VT/FFで行頭に移動"),			_T("LF/VT/FFで通常動作")			},

	// DEC Terminal Option	0-199
	{	TO_DECCKM,		_T("CKM有効のキーコードを送信"),	_T("CKM無効のキーコードを送信")		},
	{	TO_DECANM,		_T("VT100(ANSI)モード"),			_T("VT52モード")					},
	{	TO_DECCOLM,		_T("132文字表示モード"),			_T("80文字表示モード")				},
	{	TO_DECSCLM,		_T("スムーズスクロールモード"),		_T("ジャンプスクロールモード")		},
	{	TO_DECSCNM,		_T("反転表示モード"),				_T("通常表示モード")				},
	{	TO_DECOM,		_T("CUPでSTBMの影響を受ける"),		_T("CUPで常に同じ原点")				},
	{	TO_DECAWM,		_T("行末のオートワープを有効"),		_T("行末を超える移動を行わない")	},
	{	TO_XTMOSREP,	_T("マウスリポートを有効"),			_T("マウスリポートを無効")			},
	{	TO_DECPEX,		_T("MCでSTBMの影響を受けない"),		_T("MCでSTRMの影響を受ける")		},
	{	TO_DECTCEM,		_T("カーソルの表示"),				_T("カーソルを非表示")				},
//	{	TO_DECTEK,		_T("Tekモードに移行"),				_T("Tekモードから抜ける")			},
	{	TO_XTMCSC,		_T("132文字モード有効"),			_T("132文字モード無効")				},
	{	TO_XTMCUS,		_T("タブ(HT)のバグ修正モード"),		_T("xtermタブバグ互換モード")		},
	{	TO_XTMRVW,		_T("行頭BSによる上行右端移動"),		_T("行頭のBSで移動しない")			},
	{	TO_XTMABUF,		_T("全画面の保存"),					_T("保存した画面の復帰")			},
	{	TO_DECBKM,		_T("BSキー変換を行わない"),			_T("BSキーをDELキーに変換")			},
	{	TO_DECECM,		_T("SGRで空白文字を設定しない"),	_T("SGRで空白文字カラーを設定")		},

	// XTerm Option			1000-1099(300-379)
	{	TO_XTNOMTRK,	_T("Normal mouse tracking"),		_T("Mouse tracking 無効")			},
	{	TO_XTHILTRK,	_T("Hilite mouse tracking"),		_T("Mouse tracking 無効")			},
	{	TO_XTBEVTRK,	_T("Button-event mouse tracking"),	_T("Mouse tracking 無効")			},
	{	TO_XTAEVTRK,	_T("Any-event mouse tracking"),		_T("Mouse tracking 無効")			},
	{	TO_XTFOCEVT,	_T("フォーカスイベントの有効"),		_T("フォーカスイベント無効")		},
	{	TO_XTEXTMOS,	_T("Extended Mouse Mode"),			_T("Disable Extended Mode")			},
	{	TO_XTSGRMOS,	_T("SGR Mouse Mode"),				_T("Disable SGR Mode")				},
	{	TO_XTURXMOS,	_T("URXVT Mouse Mode"),				_T("Disable URXVT Mode")			},
	{	TO_XTALTSCR,	_T("全画面の保存"),					_T("保存した画面の復帰")			},
	{	TO_XTSRCUR,		_T("カーソル位置の保存"),			_T("保存したカーソル位置の復帰")	},
	{	TO_XTALTCLR,	_T("拡張画面に切り換え"),			_T("標準画面に切り換え")			},

	// XTerm Option 2		2000-2019(380-399)
	{	TO_XTBRPAMD,	_T("Bracketed Paste Mode 有効"),	_T("Bracketed Paste Mode 無効")		},

	// RLogin Option		8400-8511(400-511)
//	{	TO_RLGCWA,		_T("SGRで空白属性を設定しない"),	_T("SGRで空白文字属性を設定")		},
//	{	TO_RLGNDW,		_T("行末での遅延改行無効"),			_T("行末での遅延改行有効")			},
//	{	TO_RLGAWL,		_T("自動ブラウザ起動をする"),		_T("自動ブラウザ起動無効")			},
//	{	TO_RLBOLD,		_T("ボールド文字有効"),				_T("ボールド文字無効")				},
//	{	TO_RLBPLUS,		_T("BPlus/ZModem/Kermit自動"),		_T("自動ファイル転送無効")			},
//	{	TO_RLUNIAWH,	_T("Aタイプを半角で表示"),			_T("Aタイプを全角で表示")			},
//	{	TO_RLNORESZ,	_T("DECCOLMでリサイズ"),			_T("ウィンドウをリサイズしない")	},
//	{	TO_RLKANAUTO,	_T("漢字コードを自動追従"),			_T("漢字コードを変更しない")		},
	{	TO_RLPNAM,		_T("ノーマルモード(DECPNM)"),		_T("アプリモード(DECPAM)")			},
	{	TO_IMECTRL,		_T("IMEオープン"),					_T("IMEクロース")					},
	{	TO_RLCKMESC,	_T("7727 ESCキーのCKM有効"),		_T("ESCキーのCKM無効")				},
//	{	TO_RLMSWAPE,	_T("7786 ホイールのキー変換"),		_T("ホイールの通常動作")			},

	{	0,				NULL,								NULL							}
};

CTermPage::CTermPage() : CTreePropertyPage(CTermPage::IDD)
{
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
}

CTermPage::~CTermPage()
{
}

void CTermPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);
	DDX_Control(pDX, IDC_ESCLIST, m_List);
}

BEGIN_MESSAGE_MAP(CTermPage, CPropertyPage)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_BN_CLICKED(IDC_ESCEDIT, &CTermPage::OnBnClickedEscedit)
	ON_NOTIFY(NM_CLICK, IDC_ESCLIST, &CTermPage::OnNMClickEsclist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTermPage メッセージ ハンドラ

void CTermPage::DoInit()
{
	int n;

	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	for ( n = 0 ; OptListTab[n].ename != NULL ; n++ )
		m_List.SetLVCheck(n,  m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) ? TRUE : FALSE);

	UpdateData(FALSE);
}
BOOL CTermPage::OnInitDialog() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();

	int n;
	CString str;

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn(_T("TermPageOpt"), InitListTab, 3);

	m_List.DeleteAllItems();
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
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_List.SetItemText(n, 1, OptListTab[n].ename);
		m_List.SetItemText(n, 2, OptListTab[n].dname);
	}

	DoInit();

	return TRUE;
}
BOOL CTermPage::OnApply() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	UpdateData(TRUE);

	m_pSheet->m_pTextRam->m_ProcTab = m_ProcTab;

	for ( int n = 0 ; OptListTab[n].ename != NULL ; n++ ) {
		if ( m_List.GetLVCheck(n) ) {
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

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ ) {
		if ( m_Check[n] )
			m_pSheet->m_pTextRam->EnableOption(CheckOptTab[n]);
		else
			m_pSheet->m_pTextRam->DisableOption(CheckOptTab[n]);
	}

	return TRUE;
}
void CTermPage::OnReset() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	DoInit();
	SetModified(FALSE);
}
void CTermPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}

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

void CTermPage::OnNMClickEsclist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if ( pNMItemActivate->iSubItem == 0 ) {
		SetModified(TRUE);
		m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
	}

	*pResult = 0;
}
