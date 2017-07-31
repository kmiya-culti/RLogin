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
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60,	"番号",				0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	"セット時の動作",	0, 0 },
		{ LVCF_TEXT | LVCF_WIDTH, 0,  150,	"リセット時の動作",	0, 0 },
	};

static const struct _OptListTab {
	int		num;
	LPCSTR	ename;
	LPCSTR	dname;
} OptListTab[] = {
	// ANSI Screen Option	0-99(200-299)
	{	TO_ANSIKAM,		"キー入力を無視",				"キー入力を有効"				},
	{	TO_ANSIIRM,		"挿入モード",					"上書きモード"					},
	{	TO_ANSIERM,		"SPAの保護を無効",				"SPAの保護を有効"				},
	{	TO_ANSISRM,		"ローカルエコーを無効",			"ローカルエコーを有効"			},
	{	TO_ANSITSM,		"各行別のタブ設定",				"全行で同じタブ設定"			},
	{	TO_ANSILNM,		"LF/VT/FFで行頭に移動",			"LF/VT/FFで通常動作"			},

	// DEC Terminal Option	0-199
	{	TO_DECCKM,		"CKM有効のキーコードを送信",	"CKM無効のキーコードを送信"		},
	{	TO_DECANM,		"VT100(ANSI)モード",			"VT52モード"					},
	{	TO_DECCOLM,		"132文字表示モード",			"80文字表示モード"				},
	{	TO_DECSCLM,		"スムーズスクロールモード",		"ジャンプスクロールモード"		},
	{	TO_DECSCNM,		"反転表示モード",				"通常表示モード"				},
	{	TO_DECOM,		"CUPでSTBMの影響を受ける",		"CUPで常に同じ原点"				},
	{	TO_DECAWM,		"行末のオートワープを有効",		"行末を超える移動を行わない"	},
	{	TO_XTMOSREP,	"マウスリポートを有効",			"マウスリポートを無効"			},
	{	TO_DECPEX,		"MCでSTBMの影響を受けない",		"MCでSTRMの影響を受ける"		},
	{	TO_DECTCEM,		"カーソルの表示",				"カーソルを非表示"				},
//	{	TO_DECTEK,		"Tekモードに移行",				"Tekモードから抜ける"			},
	{	TO_XTMCSC,		"132文字モード有効",			"132文字モード無効"				},
	{	TO_XTMCUS,		"タブ(HT)のバグ修正モード",		"xtermタブバグ互換モード"		},
	{	TO_XTMRVW,		"行頭BSによる上行右端移動",		"行頭のBSで移動しない"			},
	{	TO_XTMABUF,		"全画面の保存",					"保存した画面の復帰"			},
	{	TO_DECBKM,		"BSキー変換を行わない",			"BSキーをDELキーに変換"			},
	{	TO_DECECM,		"SGRで空白文字を設定しない",	"SGRで空白文字カラーを設定"		},

	// XTerm Option			1000-1099(300-379)
	{	TO_XTNOMTRK,	"Normal mouse tracking",		"Mouse tracking 無効"			},
	{	TO_XTHILTRK,	"Hilite mouse tracking",		"Mouse tracking 無効"			},
	{	TO_XTBEVTRK,	"Button-event mouse tracking",	"Mouse tracking 無効"			},
	{	TO_XTAEVTRK,	"Any-event mouse tracking",		"Mouse tracking 無効"			},
	{	TO_XTFOCEVT,	"フォーカスイベントの有効",		"フォーカスイベント無効"		},
	{	TO_XTALTSCR,	"全画面の保存",					"保存した画面の復帰"			},
	{	TO_XTSRCUR,		"カーソル位置の保存",			"保存したカーソル位置の復帰"	},
	{	TO_XTALTCLR,	"拡張画面に切り換え",			"標準画面に切り換え"			},

	// XTerm Option 2		2000-2019(380-399)
	{	TO_XTBRPAMD,	"Bracketed Paste Mode 有効",	"Bracketed Paste Mode 無効"		},

	// RLogin Option		8400-8511(400-511)
	{	TO_RLGCWA,		"SGRで空白属性を設定しない",	"SGRで空白文字属性を設定"		},
	{	TO_RLGNDW,		"行末での遅延改行無効",			"行末での遅延改行有効"			},
	{	TO_RLGAWL,		"自動ブラウザ起動をする",		"自動ブラウザ起動無効"			},
	{	TO_RLBOLD,		"ボールド文字有効",				"ボールド文字無効"				},
	{	TO_RLBPLUS,		"BPlus/ZModem/Kermit自動",		"自動ファイル転送無効"			},
	{	TO_RLUNIAWH,	"Aタイプを半角で表示",			"Aタイプを全角で表示"			},
	{	TO_RLNORESZ,	"DECCOLMでリサイズ",			"ウィンドウをリサイズしない"	},
	{	TO_RLKANAUTO,	"漢字コードを自動追従",			"漢字コードを変更しない"		},
	{	TO_RLMOSWHL,	"ホイールの通常動作",			"ホイールをヌルヌルにする"		},
	{	TO_RLMSWAPP,	"ホイールでスクロールのみ",		"ホイールでキーコード発生"		},
	{	TO_RLPNAM,		"ノーマルモード(DECPNM)",		"アプリモード(DECPAM)"			},
	{	TO_IMECTRL,		"IMEオープン",					"IMEクロース"					},

	{	0,				NULL	}
};

CTermPage::CTermPage() : CPropertyPage(CTermPage::IDD)
{
	//{{AFX_DATA_INIT(CTermPage)
	//}}AFX_DATA_INIT
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = FALSE;
	m_pSheet = NULL;
	m_TtlMode = 0;
	m_TtlRep = m_TtlCng = FALSE;
}

CTermPage::~CTermPage()
{
}

void CTermPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTermPage)
	//}}AFX_DATA_MAP
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		DDX_Check(pDX, IDC_TERMCHECK1 + n, m_Check[n]);
	DDX_Control(pDX, IDC_ESCLIST, m_List);
	DDX_CBIndex(pDX, IDC_COMBO3, m_TtlMode);
	DDX_Check(pDX, IDC_TERMCHECK6, m_TtlRep);
	DDX_Check(pDX, IDC_TERMCHECK7, m_TtlCng);
}

BEGIN_MESSAGE_MAP(CTermPage, CPropertyPage)
	//{{AFX_MSG_MAP(CTermPage)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECKFAST, IDC_CHECKFAST + CHECKOPTMAX - 1, OnUpdateCheck)
	ON_BN_CLICKED(IDC_ESCEDIT, &CTermPage::OnBnClickedEscedit)
	ON_NOTIFY(NM_CLICK, IDC_ESCLIST, &CTermPage::OnNMClickEsclist)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_TERMCHECK6, IDC_TERMCHECK7, OnUpdateCheck)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CTermPage::OnCbnSelchangeCombo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTermPage メッセージ ハンドラ

BOOL CTermPage::OnInitDialog() 
{
	int n;
	CString str;

	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	CPropertyPage::OnInitDialog();
	for ( n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	m_TtlMode = m_pSheet->m_pTextRam->m_TitleMode & 7;
	m_TtlRep  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_REPORT) ? TRUE : FALSE;
	m_TtlCng  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_CHENG)  ? TRUE : FALSE;

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn("TermPageOpt", InitListTab, 3);

	m_List.DeleteAllItems();
	for ( n = 0 ; OptListTab[n].ename != NULL ; n++ ) {
		if ( OptListTab[n].num < 200) {				// DEC Terminal Option	0-199
			str.Format("?%d", OptListTab[n].num);
		} else if ( OptListTab[n].num < 300 ) {		// ANSI Screen Option	0-99(200-299)
			str.Format("%d", OptListTab[n].num - 200);
		} else if ( OptListTab[n].num < 380 ) {		// XTerm Option			1000-1079(300-379)
			str.Format("?%d", OptListTab[n].num + 700);
		} else if ( OptListTab[n].num < 400 ) {		// XTerm Option 2		2000-2019(380-399)
			str.Format("?%d", OptListTab[n].num + 1620);
		} else if ( OptListTab[n].num < 512 ) {		// RLogin Option		8400-8511(400-511)
			str.Format("?%d", OptListTab[n].num + 8000);
		}
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_List.SetItemText(n, 1, OptListTab[n].ename);
		m_List.SetItemText(n, 2, OptListTab[n].dname);
		m_List.SetLVCheck(n,  m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) ? TRUE : FALSE);
	}

	UpdateData(FALSE);
	return TRUE;
}
BOOL CTermPage::OnApply() 
{
	ASSERT(m_pSheet);
	ASSERT(m_pSheet->m_pTextRam);

	UpdateData(TRUE);
	for ( int n = 0 ; n < CHECKOPTMAX ; n++ ) {
		if ( m_Check[n] )
			m_pSheet->m_pTextRam->EnableOption(CheckOptTab[n]);
		else
			m_pSheet->m_pTextRam->DisableOption(CheckOptTab[n]);
	}

	m_pSheet->m_pTextRam->m_ProcTab = m_ProcTab;

	m_pSheet->m_pTextRam->m_TitleMode = m_TtlMode;
	if ( m_TtlRep )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_REPORT;
	if ( m_TtlCng )
		m_pSheet->m_pTextRam->m_TitleMode |= WTTL_CHENG;

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

	return TRUE;
}
void CTermPage::OnReset() 
{
	if ( m_hWnd == NULL )
		return;

	for ( int n = 0 ; n < CHECKOPTMAX ; n++ )
		m_Check[n] = (m_pSheet->m_pTextRam->IsOptEnable(CheckOptTab[n]) ? TRUE : FALSE);

	m_ProcTab = m_pSheet->m_pTextRam->m_ProcTab;

	m_TtlMode = m_pSheet->m_pTextRam->m_TitleMode & 7;
	m_TtlRep  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_REPORT) ? TRUE : FALSE;
	m_TtlCng  = (m_pSheet->m_pTextRam->m_TitleMode & WTTL_CHENG)  ? TRUE : FALSE;

	for ( int n = 0 ; OptListTab[n].ename != NULL ; n++ )
		m_List.SetLVCheck(n,  m_pSheet->m_pTextRam->IsOptEnable(OptListTab[n].num) ? TRUE : FALSE);

	UpdateData(FALSE);
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

void CTermPage::OnCbnSelchangeCombo()
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_TEXTRAM;
}
