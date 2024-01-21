// AutoRenDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "Ssh.h"
#include "SFtp.h"
#include "AutoRenDlg.h"

// CAutoRenDlg ダイアログ

IMPLEMENT_DYNAMIC(CAutoRenDlg, CDialogExt)

CAutoRenDlg::CAutoRenDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CAutoRenDlg::IDD, pParent)
{
	m_Exec = 0;
}

CAutoRenDlg::~CAutoRenDlg()
{
}

void CAutoRenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Name[0]);
	DDX_Text(pDX, IDC_EDIT2, m_Name[1]);
	DDX_Text(pDX, IDC_EDIT3, m_Name[2]);
	DDX_Text(pDX, IDC_NAMEOK, m_NameOK);
	DDX_Radio(pDX, IDC_RADIO1, m_Exec);
}


BEGIN_MESSAGE_MAP(CAutoRenDlg, CDialogExt)
	ON_BN_CLICKED(IDC_BUTTON1, &CAutoRenDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CAutoRenDlg::OnBnClickedButton2)
	ON_EN_UPDATE(IDC_EDIT3, &CAutoRenDlg::OnEnUpdateEdit3)
END_MESSAGE_MAP()


// CAutoRenDlg メッセージ ハンドラ

void CAutoRenDlg::OnBnClickedButton1()
{
	UpdateData(TRUE);
	OnOK();
}

void CAutoRenDlg::OnBnClickedButton2()
{
	UpdateData(TRUE);
	OnOK();
	m_Exec |= 0x80;
}

void CAutoRenDlg::OnEnUpdateEdit3()
{
	CString tmp;
	CFileNode node;

	UpdateData(TRUE);
	node.AutoRename(m_Name[2], tmp, 1);
	m_NameOK = (tmp.Compare(m_Name[2]) == 0 ? UniToTstr(L"\u25CB") : UniToTstr(L"\u00D7"));	// _T("○") : _T("×")));
	UpdateData(FALSE);
}

BOOL CAutoRenDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();
	
	SetSaveProfile(_T("AutoRenDlg"));

	return TRUE;
}
