// PassDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "PassDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CPassDlg ダイアログ

IMPLEMENT_DYNAMIC(CPassDlg, CDialogExt)

CPassDlg::CPassDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CPassDlg::IDD, pParent)
{
	m_HostAddr = _T("");
	m_UserName = _T("");
	m_PassName = _T("");
	m_Prompt = _T("Password");
	m_Counter = 0;
	m_MaxTime = 60;
	m_Title = _T("");
	m_PassEcho = FALSE;
	m_Enable = PASSDLG_HOST | PASSDLG_USER | PASSDLG_PASS;
}

void CPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TIMELIMIT, m_TimeLimit);
	DDX_Control(pDX, IDC_HOSTADDR, m_HostWnd);
	DDX_Control(pDX, IDC_USERNAME, m_UserWnd);
	DDX_Control(pDX, IDC_PASSNAME, m_PassWnd);
	DDX_Text(pDX, IDC_PROMPT, m_Prompt);
}

BEGIN_MESSAGE_MAP(CPassDlg, CDialogExt)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPassDlg メッセージ ハンドラ

BOOL CPassDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();
	
	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

	m_HostWnd.LoadHis(_T("PassDlgHostAddr"));
	m_HostWnd.SetWindowText(m_HostAddr);

	if ( (m_Enable & PASSDLG_HOST) == 0 )
		m_HostWnd.EnableWindow(FALSE);

	m_UserWnd.LoadHis(_T("PassDlgUserName"));
	m_UserWnd.SetWindowText(m_UserName);

	if ( (m_Enable & PASSDLG_USER) == 0 )
		m_UserWnd.EnableWindow(FALSE);

	m_PassWnd.SetWindowText(m_PassName);

	if ( (m_Enable & PASSDLG_PASS) == 0 )
		m_PassWnd.EnableWindow(FALSE);

	if ( m_PassEcho ) {
		m_PassWnd.ModifyStyle(ES_PASSWORD, 0);
		m_PassWnd.SetPasswordChar(_T('\0'));
		m_PassWnd.Invalidate(TRUE);
	}

	if ( !m_HostAddr.IsEmpty() ) {
		if ( !m_UserName.IsEmpty() ) {
			m_PassWnd.SetSel(0, (-1));
			m_PassWnd.SetFocus();
		} else
			m_UserWnd.SetFocus();
	}

	m_TimeLimit.SetRange(0, m_MaxTime);
	SetTimer(1028, 1000, NULL);
	m_Counter = 0;

	UpdateData(FALSE);

	return FALSE;
}

void CPassDlg::OnTimer(UINT_PTR nIDEvent) 
{
	if ( nIDEvent == 1028 ) {
		m_TimeLimit.SetPos(++m_Counter);
		if ( m_Counter >= m_MaxTime )
			OnCancel();
	}

	CDialogExt::OnTimer(nIDEvent);
}

void CPassDlg::OnOK()
{
	m_HostWnd.GetWindowText(m_HostAddr);
	m_HostWnd.AddHis(m_HostAddr);

	m_UserWnd.GetWindowText(m_UserName);
	m_UserWnd.AddHis(m_UserName);

	m_PassWnd.GetWindowText(m_PassName);

	CDialogExt::OnOK();
}
