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
	DDX_Control(pDX, IDC_HOSTADDR, m_HostWnd);
	DDX_Control(pDX, IDC_USERNAME, m_UserWnd);
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
	
	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	m_HostWnd.LoadHis(_T("PassDlgHostAddr"));
	m_HostWnd.SetWindowText(m_HostAddr);

	if ( !m_HostAddr.IsEmpty() )
		m_HostWnd.EnableWindow(FALSE);

	m_UserWnd.LoadHis(_T("PassDlgUserName"));
	m_UserWnd.SetWindowText(m_UserName);

	if ( !m_UserName.IsEmpty() )
		m_UserWnd.EnableWindow(FALSE);

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

void CPassDlg::OnOK()
{
	UpdateData(TRUE);

	m_HostWnd.GetWindowText(m_HostAddr);
	m_HostWnd.AddHis(m_HostAddr);
	m_HostWnd.SaveHis(_T("PassDlgHostAddr"));

	m_UserWnd.GetWindowText(m_UserName);
	m_UserWnd.AddHis(m_UserName);
	m_UserWnd.SaveHis(_T("PassDlgUserName"));

	CDialog::OnOK();
}
