// IDKeyDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "Data.h"
#include "IdKeyFileDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg ダイアログ

IMPLEMENT_DYNAMIC(CIdKeyFileDlg, CDialog)

CIdKeyFileDlg::CIdKeyFileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIdKeyFileDlg::IDD, pParent)
{
	m_IdkeyFile = _T("");
	m_PassName = _T("");
	m_PassName2 = _T("");
	m_Message = _T("");

	m_Title = _T("IDKey File Load/Save");
	m_OpenMode = 0;
}

void CIdKeyFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_IDKEYFILE, m_IdkeyFile);
	DDX_Text(pDX, IDC_PASSNAME, m_PassName);
	DDX_Text(pDX, IDC_PASSNAME2, m_PassName2);
	DDX_Text(pDX, IDC_MESSAGE, m_Message);
}

BEGIN_MESSAGE_MAP(CIdKeyFileDlg, CDialog)
	ON_BN_CLICKED(IDC_IDKEYSEL, OnIdkeysel)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg メッセージ ハンドラ

BOOL CIdKeyFileDlg::OnInitDialog() 
{
	CWnd *pWnd;
	CDialog::OnInitDialog();
	
	SetWindowText(m_Title);
	switch(m_OpenMode) {
	case 1:
		if ( (pWnd = GetDlgItem(IDC_PASSNAME2)) != NULL )
			pWnd->EnableWindow(FALSE);
		break;
	case 3:
		if ( (pWnd = GetDlgItem(IDC_IDKEYFILE)) != NULL )
			pWnd->EnableWindow(FALSE);
		if ( (pWnd = GetDlgItem(IDC_IDKEYSEL)) != NULL )
			pWnd->EnableWindow(FALSE);
		break;
	}
	return TRUE;
}

void CIdKeyFileDlg::OnIdkeysel() 
{
	UpdateData(TRUE);

	CFileDialog dlg(m_OpenMode == 1 ? TRUE : FALSE, _T(""), m_IdkeyFile, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( dlg.DoModal() != IDOK )
		return;

	m_IdkeyFile = dlg.GetPathName();
	UpdateData(FALSE);
}

void CIdKeyFileDlg::OnOK() 
{
	UpdateData(TRUE);
	if ( m_OpenMode != 1 && m_PassName.Compare(m_PassName2) != 0 ) {
		MessageBox(CStringLoad(IDE_PASSWORDNOMATCH));
		return;
	}
	CDialog::OnOK();
}
