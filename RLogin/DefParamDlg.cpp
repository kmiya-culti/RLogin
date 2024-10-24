// DefParamDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"
#include "DefParamDlg.h"

// CDefParamDlg ダイアログ

IMPLEMENT_DYNAMIC(CDefParamDlg, CDialogExt)

CDefParamDlg::CDefParamDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CDefParamDlg::IDD, pParent)
{
	m_InitFlag = 0;

	for ( int n = 0 ; n < 6 ; n++ )
		m_Check[n] = FALSE;

	m_pTextRam = NULL;
}

CDefParamDlg::~CDefParamDlg()
{
}

void CDefParamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	for ( int n = 0 ; n < 7 ; n++ )
		DDX_Check(pDX, IDC_DEFPAMCHECK1 + n, m_Check[n]);

	DDX_Control(pDX, IDC_MSGICON, m_MsgIcon);
}


BEGIN_MESSAGE_MAP(CDefParamDlg, CDialogExt)
END_MESSAGE_MAP()

static struct _DefParaCheckTab {
	int		OptNum;
	BOOL	TrueFalse;
} DefParamCheckTab[] = {
	{		TO_ANSIKAM,		FALSE	},
	{		TO_ANSIIRM,		FALSE	},
	{		TO_ANSISRM,		TRUE	},
	{		TO_DECTCEM,		TRUE	},
	{		TO_XTFOCEVT,	FALSE	},
	{		TO_XTBRPAMD,	FALSE	},
	{		(-1),			FALSE	},
};

// CDefParamDlg メッセージ ハンドラー

BOOL CDefParamDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	int n, i;
	CWnd *pWnd, *pMsg;
	CString text, str, msg, tmp;
	LPCTSTR icon = IDI_QUESTION;

	if ( (pWnd = GetDlgItem(IDC_DEFPAMCHECK1)) != NULL ) {
		if ( m_pTextRam != NULL && (m_InitFlag & UMOD_ANSIOPT) != 0 ) {
			for ( i = 0 ; i < (32 * 16) ; i++ ) {

				// 0-99		->	200-299
				// 100-299	->	0-199
				// 300-		->	300-
				if ( i < 100 )
					n = i + 200;
				else if ( i < 300 )
					n = i - 100;
				else
					n = i;

				if ( IS_ENABLE(m_pTextRam->m_DefAnsiOpt, n) != IS_ENABLE(m_pTextRam->m_AnsiOpt, n) ) {
					if ( str.GetLength() > 32 ) {
						str += _T("...");
						break;
					}

					CTextRam::OptionString(n, tmp);

					if ( !str.IsEmpty() )
						str += _T(",");
					str += tmp;
				}
			}

			for ( n = 0 ; DefParamCheckTab[n].OptNum >= 0 ; n++ ) {
				if ( m_pTextRam->IsOptEnable(DefParamCheckTab[n].OptNum) != DefParamCheckTab[n].TrueFalse ) {

					CTextRam::OptionString(DefParamCheckTab[n].OptNum, tmp);

					if ( !msg.IsEmpty() )
						msg += _T(",");
					msg += tmp;
				}
			}
		}

		if ( str.IsEmpty() )
			str = _T("SM,RM,DECSET,DECRST");

		pWnd->GetWindowText(text);
		tmp.Format(_T("%s(%s)"), (LPCTSTR)text, (LPCTSTR)str);
		pWnd->SetWindowText(tmp);

		if ( !msg.IsEmpty() && (pMsg = GetDlgItem(IDC_DEFPARAMSG)) != NULL ) {
			pMsg->GetWindowText(text);
			tmp.Format(_T("%s\n%s(%s)"), (LPCTSTR)text, (LPCTSTR)CStringLoad(IDS_DEFPARACSIOPTMSG), (LPCTSTR)msg);
			pMsg->SetWindowText(tmp);

			icon = IDI_WARNING;
		}
	}

	m_MsgIcon.SetIcon(::LoadIcon(NULL, icon));

	m_Check[0] = (m_InitFlag & UMOD_ANSIOPT)  != 0 ? TRUE : FALSE;
	m_Check[1] = (m_InitFlag & UMOD_MODKEY)   != 0 ? TRUE : FALSE;
	m_Check[2] = (m_InitFlag & UMOD_COLTAB)   != 0 ? TRUE : FALSE;
	m_Check[3] = (m_InitFlag & UMOD_BANKTAB)  != 0 ? TRUE : FALSE;
	m_Check[4] = (m_InitFlag & UMOD_DEFATT)   != 0 ? TRUE : FALSE;
	m_Check[5] = (m_InitFlag & UMOD_CARET)    != 0 ? TRUE : FALSE;
	m_Check[6] = (m_InitFlag & UMOD_CODEFLAG) != 0 ? TRUE : FALSE;

	UpdateData(FALSE);

	SetSaveProfile(_T("DefParamDlg"));

	return TRUE;
}


void CDefParamDlg::OnOK()
{
	UpdateData(TRUE);

	if ( m_Check[0] ) m_InitFlag |= UMOD_ANSIOPT;  else m_InitFlag &= ~UMOD_ANSIOPT;
	if ( m_Check[1] ) m_InitFlag |= UMOD_MODKEY;   else m_InitFlag &= ~UMOD_MODKEY;
	if ( m_Check[2] ) m_InitFlag |= UMOD_COLTAB;   else m_InitFlag &= ~UMOD_COLTAB;
	if ( m_Check[3] ) m_InitFlag |= UMOD_BANKTAB;  else m_InitFlag &= ~UMOD_BANKTAB;
	if ( m_Check[4] ) m_InitFlag |= UMOD_DEFATT;   else m_InitFlag &= ~UMOD_DEFATT;
	if ( m_Check[5] ) m_InitFlag |= UMOD_CARET;    else m_InitFlag &= ~UMOD_CARET;
	if ( m_Check[6] ) m_InitFlag |= UMOD_CODEFLAG; else m_InitFlag &= ~UMOD_CODEFLAG;

	CDialogExt::OnOK();
}
