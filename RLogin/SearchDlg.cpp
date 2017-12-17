// SearchDlg.cpp : 実装ファイル
//

#include "stdafx.h"
//#include "afxdialogex.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "SearchDlg.h"

// CSearchDlg ダイアログ

IMPLEMENT_DYNAMIC(CSearchDlg, CDialogExt)

CSearchDlg::CSearchDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CSearchDlg::IDD, pParent)
{
	m_SearchStr.Empty();

	m_pDoc = NULL;
	m_Pos = m_Max = 0;
	m_bRegExSerch = FALSE;
	m_TimerId = 0;
}

CSearchDlg::~CSearchDlg()
{
}

void CSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_Prog);
	DDX_Control(pDX, IDC_COMBO1, m_SearchCombo);
	DDX_Check(pDX, IDC_REGEXSERCH, m_bRegExSerch);
}

BEGIN_MESSAGE_MAP(CSearchDlg, CDialogExt)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CSearchDlg メッセージ ハンドラー

BOOL CSearchDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	m_SearchCombo.LoadHis(_T("SearchDlgStr"));
	m_SearchCombo.SetWindowText(m_SearchStr);

	m_bRegExSerch = AfxGetApp()->GetProfileInt(_T("SearchDlgStr"), _T("RegExSerch"), FALSE);

	UpdateData(FALSE);

	return TRUE;
}
void CSearchDlg::OnOK()
{
	if ( m_pDoc == NULL )
		goto ENDOFRET;

	UpdateData(TRUE);

	m_SearchCombo.GetWindowText(m_SearchStr);

	if ( (m_Max = m_pDoc->m_TextRam.HisRegMark(m_SearchStr, m_bRegExSerch)) <= 0 )
		goto ENDOFRET;

	m_Pos = 0;
	m_Prog.SetRange32(0, m_Max);
	m_Prog.SetPos(m_Pos);

	if ( m_Pos < m_Max )
		m_TimerId = SetTimer(1024, 300, NULL);

	m_SearchCombo.AddHis(m_SearchStr);
//	m_SearchCombo.SaveHis(_T("SearchDlgStr"));
	m_SearchCombo.SetWindowText(m_SearchStr);

	AfxGetApp()->WriteProfileInt(_T("SearchDlgStr"), _T("RegExSerch"), m_bRegExSerch);
	return;

ENDOFRET:
	if ( m_TimerId != 0 ) {
		KillTimer(m_TimerId);
		m_TimerId = 0;
	}

	CDialogExt::OnOK();
}
void CSearchDlg::OnCancel()
{
	if ( m_TimerId != 0 ) {
		KillTimer(m_TimerId);
		m_TimerId = 0;
	}

	m_Pos = m_Max;

	CDialogExt::OnCancel();
}
void CSearchDlg::OnTimer(UINT_PTR nIDEvent)
{
	if( m_Pos < m_Max ) {
		m_Pos = m_pDoc->m_TextRam.HisRegNext(250);
		m_Prog.PostMessage(PBM_SETPOS, m_Pos);

	} else {
		KillTimer(nIDEvent);
		CDialogExt::OnOK();
	}

	CDialogExt::OnTimer(nIDEvent);
}
