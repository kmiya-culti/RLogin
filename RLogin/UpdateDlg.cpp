// UpdateDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "UpdateDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUpdateDlg ダイアログ


CUpdateDlg::CUpdateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUpdateDlg::IDD, pParent)
{
	m_DoExec = ::AfxGetApp()->GetProfileInt("CUpdateDlg", "Jobs", 1);
	//{{AFX_DATA_INIT(CUpdateDlg)
	m_FileName = _T("");
	m_Jobs = m_DoExec & 0x7F;
	//}}AFX_DATA_INIT
}


void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUpdateDlg)
	DDX_Text(pDX, IDC_FILENAME, m_FileName);
	DDX_Radio(pDX, IDC_RADIO1, m_Jobs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUpdateDlg, CDialog)
	//{{AFX_MSG_MAP(CUpdateDlg)
	ON_BN_CLICKED(IDC_BUTTON1, OnExec)
	ON_BN_CLICKED(IDC_BUTTON2, OnAllExec)
	ON_BN_CLICKED(IDC_BUTTON3, OnAbort)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpdateDlg メッセージ ハンドラ

void CUpdateDlg::OnExec() 
{
	UpdateData(TRUE);
	::AfxGetApp()->WriteProfileInt("CUpdateDlg", "Jobs", m_Jobs);
	m_DoExec = m_Jobs;
	CDialog::OnOK();
}

void CUpdateDlg::OnAllExec() 
{
	UpdateData(TRUE);
	::AfxGetApp()->WriteProfileInt("CUpdateDlg", "Jobs", m_Jobs);
	m_DoExec = m_Jobs | 0x80;
	CDialog::OnOK();
}

void CUpdateDlg::OnAbort() 
{
	m_DoExec = UDO_ABORT | 0x80;
	CDialog::OnCancel();
}

BOOL CUpdateDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CWnd *pWnd = GetDlgItem(IDC_RADIO4);

	if ( pWnd != NULL && m_DoResume == FALSE )
		pWnd->SetWindowText(_T("再開できません上書きになります"));
	
	return TRUE;
}
