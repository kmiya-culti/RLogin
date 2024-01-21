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

IMPLEMENT_DYNAMIC(CUpdateDlg, CDialogExt)

CUpdateDlg::CUpdateDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CUpdateDlg::IDD, pParent)
{
	m_DoExec = ::AfxGetApp()->GetProfileInt(_T("CUpdateDlg"), _T("Jobs"), 1);

	m_FileName = _T("");
	m_Jobs = m_DoExec & 0x7F;
}

void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_FILENAME, m_FileName);
	DDX_Radio(pDX, IDC_RADIO1, m_Jobs);
}

BEGIN_MESSAGE_MAP(CUpdateDlg, CDialogExt)
	ON_BN_CLICKED(IDC_BUTTON1, OnExec)
	ON_BN_CLICKED(IDC_BUTTON2, OnAllExec)
	ON_BN_CLICKED(IDC_BUTTON3, OnAbort)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpdateDlg メッセージ ハンドラ

void CUpdateDlg::OnExec() 
{
	UpdateData(TRUE);
	::AfxGetApp()->WriteProfileInt(_T("CUpdateDlg"), _T("Jobs"), m_Jobs);
	m_DoExec = m_Jobs;
	CDialogExt::OnOK();
}

void CUpdateDlg::OnAllExec() 
{
	UpdateData(TRUE);
	::AfxGetApp()->WriteProfileInt(_T("CUpdateDlg"), _T("Jobs"), m_Jobs);
	m_DoExec = m_Jobs | 0x80;
	CDialogExt::OnOK();
}

void CUpdateDlg::OnAbort() 
{
	m_DoExec = UDO_ABORT | 0x80;
	CDialogExt::OnCancel();
}

BOOL CUpdateDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	CWnd *pWnd = GetDlgItem(IDC_RADIO4);

	if ( pWnd != NULL && m_DoResume == FALSE )
		pWnd->SetWindowText(CStringLoad(IDE_UPDATERESUMEERROR));

	SetSaveProfile(_T("UpdateDlg"));
	
	return TRUE;
}
