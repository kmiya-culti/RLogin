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
	m_SerchMode = 0;
	m_RegEscChar = FALSE;
	m_RegNoCase = FALSE;
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
	DDX_Radio(pDX, IDC_RADIO1, m_SerchMode);
	DDX_Check(pDX, IDC_CHECK1, m_RegEscChar);
	DDX_Check(pDX, IDC_CHECK2, m_RegNoCase);
}

BEGIN_MESSAGE_MAP(CSearchDlg, CDialogExt)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CSearchDlg メッセージ ハンドラー

BOOL CSearchDlg::OnInitDialog()
{
	int RegFlag;

	CDialogExt::OnInitDialog();

	m_SearchCombo.LoadHis(_T("SearchDlgStr"));
	m_SearchCombo.SetWindowText(m_SearchStr);

	RegFlag = AfxGetApp()->GetProfileInt(_T("SearchDlgStr"), _T("RegExFlag"), (-1));

	if ( RegFlag == (-1) ) {
		if ( AfxGetApp()->GetProfileInt(_T("SearchDlgStr"), _T("RegExSerch"), FALSE) ) {
			m_SerchMode = 2;
			m_RegEscChar = TRUE;
			m_RegNoCase  = FALSE;
		} else {
			m_SerchMode = 0;
			m_RegEscChar = FALSE;
			m_RegNoCase  = FALSE;
		}
	} else {
		switch(RegFlag & REG_FLAG_MODEMASK) {
		case REG_FLAG_SIMPLE:
			m_SerchMode = 0;
			break;
		case REG_FLAG_WILDCARD:
			m_SerchMode = 1;
			break;
		default:	// REG_FLAG_REGEX
			m_SerchMode = 2;
			break;
		}
		m_RegEscChar = (RegFlag & REG_FLAG_ESCCHAR) != 0 ? TRUE : FALSE;
		m_RegNoCase  = (RegFlag & REG_FLAG_NOCASE)  != 0 ? TRUE : FALSE;
	}

	UpdateData(FALSE);

	return TRUE;
}
void CSearchDlg::OnOK()
{
	int RegFlag = REG_FLAG_SIMPLE;
	BOOL bRegEx = TRUE;
	CString str;

	if ( m_pDoc == NULL )
		goto ENDOFRET;

	UpdateData(TRUE);

	m_SearchCombo.GetWindowText(m_SearchStr);

	switch(m_SerchMode) {
	case 0: RegFlag = REG_FLAG_SIMPLE;   break;
	case 1: RegFlag = REG_FLAG_WILDCARD; break;
	case 2: RegFlag = REG_FLAG_REGEX;    break;
	}

	if ( m_RegEscChar )
		RegFlag |= REG_FLAG_ESCCHAR;

	if ( m_RegNoCase )
		RegFlag |= REG_FLAG_NOCASE;

	if ( RegFlag == REG_FLAG_SIMPLE ) {
		bRegEx = FALSE;
		str = m_SearchStr;
	} else
		str = CRegEx::SimpleRegEx(m_SearchStr, RegFlag);

	if ( (m_Max = m_pDoc->m_TextRam.HisRegMark(str, bRegEx)) <= 0 )
		goto ENDOFRET;

	m_Pos = 0;
	m_Prog.SetRange32(0, m_Max);
	m_Prog.SetPos(m_Pos);

	if ( m_Pos < m_Max )
		m_TimerId = SetTimer(1024, 300, NULL);

	m_SearchCombo.AddHis(m_SearchStr);
//	m_SearchCombo.SaveHis(_T("SearchDlgStr"));
	m_SearchCombo.SetWindowText(m_SearchStr);

	AfxGetApp()->WriteProfileInt(_T("SearchDlgStr"), _T("RegExFlag"), RegFlag);
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
