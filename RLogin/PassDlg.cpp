// PassDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "PassDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPassDlg ダイアログ

CPassDlg::CPassDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPassDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPassDlg)
	m_HostAddr = _T("");
	m_UserName = _T("");
	m_PassName = _T("");
	m_Prompt = _T("Password");
	//}}AFX_DATA_INIT
	m_Counter = 0;
	m_MaxTime = 60;
	m_Title = _T("");
}

void CPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPassDlg)
	DDX_Control(pDX, IDC_TIMELIMIT, m_TimeLimit);
	DDX_Text(pDX, IDC_HOSTADDR, m_HostAddr);
	DDX_Text(pDX, IDC_USERNAME, m_UserName);
	DDX_Text(pDX, IDC_PASSNAME, m_PassName);
	DDX_Text(pDX, IDC_PROMPT, m_Prompt);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPassDlg, CDialog)
	//{{AFX_MSG_MAP(CPassDlg)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
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
	
	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
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
