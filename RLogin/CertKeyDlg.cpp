// CertKeyDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "CertKeyDlg.h"
//#include "afxdialogex.h"

// CCertKeyDlg ダイアログ

IMPLEMENT_DYNAMIC(CCertKeyDlg, CDialogExt)

CCertKeyDlg::CCertKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CCertKeyDlg::IDD, pParent)
{
	m_HostName.Empty();
	m_Digest.Empty();
	m_SaveKeyFlag = TRUE;
	m_Counter = 0;
	m_MaxTime = 60;
}

CCertKeyDlg::~CCertKeyDlg()
{
}

void CCertKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_DIGEST, m_Digest);
	DDX_Check(pDX, IDC_SAVEKEY, m_SaveKeyFlag);
	DDX_Text(pDX, IDC_HOSTNAME, m_HostName);
	DDX_Control(pDX, IDC_TIMELIMIT, m_TimeLimit);
}

BEGIN_MESSAGE_MAP(CCertKeyDlg, CDialogExt)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CCertKeyDlg メッセージ ハンドラー

BOOL CCertKeyDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	CWnd *pWnd;

	m_TimeLimit.SetRange(0, m_MaxTime);
	SetTimer(1028, 1000, NULL);
	m_Counter = 0;

	if ( (pWnd = GetDlgItem(IDC_DIGEST)) != NULL && m_DigestFont.CreatePointFont(MulDiv(9 * 10, m_NowDpi.cy, SYSTEM_DPI_Y), _T("Consolas")) )
		pWnd->SetFont(&m_DigestFont);

	m_SaveKeyFlag = ::AfxGetApp()->GetProfileInt(_T("CCertKeyDlg"), _T("SaveKeyFlag"), TRUE);
	UpdateData(FALSE);

	return TRUE;
}

void CCertKeyDlg::OnTimer(UINT_PTR nIDEvent)
{
	if ( nIDEvent == 1028 ) {
		m_TimeLimit.SetPos(++m_Counter);
		if ( m_Counter >= m_MaxTime )
			OnCancel();
	}
	CDialogExt::OnTimer(nIDEvent);
}


void CCertKeyDlg::OnOK()
{
	UpdateData(TRUE);
	::AfxGetApp()->WriteProfileInt(_T("CCertKeyDlg"), _T("SaveKeyFlag"), m_SaveKeyFlag);

	CDialogExt::OnOK();
}
