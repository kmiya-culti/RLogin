// PassDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "PassDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CPassDlg ダイアログ

IMPLEMENT_DYNAMIC(CPassDlg, CDialog)

CPassDlg::CPassDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPassDlg::IDD, pParent)
{
	m_HostAddr = _T("");
	m_UserName = _T("");
	m_PassName = _T("");
	m_Prompt = _T("Password");
	m_Counter = 0;
	m_MaxTime = 60;
	m_Title = _T("");
}

void CPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TIMELIMIT, m_TimeLimit);
	DDX_Text(pDX, IDC_HOSTADDR, m_HostAddr);
	DDX_Text(pDX, IDC_USERNAME, m_UserName);
	DDX_Text(pDX, IDC_PASSNAME, m_PassName);
	DDX_Text(pDX, IDC_PROMPT, m_Prompt);
}

BEGIN_MESSAGE_MAP(CPassDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPassDlg メッセージ ハンドラ

BOOL CPassDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CWnd *pWnd;

	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	if ( !m_HostAddr.IsEmpty() && (pWnd = GetDlgItem(IDC_HOSTADDR)) != NULL )
		pWnd->EnableWindow(FALSE);

	if ( !m_UserName.IsEmpty() && (pWnd = GetDlgItem(IDC_USERNAME)) != NULL )
		pWnd->EnableWindow(FALSE);

	m_TimeLimit.SetRange(0, m_MaxTime);
	SetTimer(1028, 1000, NULL);
	m_Counter = 0;
	
	return TRUE;
}

void CPassDlg::OnTimer(UINT_PTR nIDEvent) 
{
	if ( nIDEvent == 1028 ) {
		m_TimeLimit.SetPos(++m_Counter);
		if ( m_Counter >= m_MaxTime )
			OnCancel();
	}

	CDialog::OnTimer(nIDEvent);
}
