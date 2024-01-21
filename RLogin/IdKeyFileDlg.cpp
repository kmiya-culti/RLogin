// IDKeyDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "Data.h"
#include "IdKeyFileDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg ダイアログ

IMPLEMENT_DYNAMIC(CIdKeyFileDlg, CDialogExt)

CIdKeyFileDlg::CIdKeyFileDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CIdKeyFileDlg::IDD, pParent)
{
	m_IdkeyFile = _T("");
	m_Message = _T("");
	m_PassName = _T("");
	m_bPassDisp = FALSE;

	m_Title = _T("IDKey File Load/Save");
	m_OpenMode = IDKFDMODE_LOAD;
	m_PassChar = _T('*');
}

void CIdKeyFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_IDKEYFILE, m_IdkeyFile);
	DDX_Text(pDX, IDC_MESSAGE, m_Message);
	DDX_Text(pDX, IDC_PASSNAME, m_PassName);
	DDX_Check(pDX, IDC_PASSDISP, m_bPassDisp);
}

BEGIN_MESSAGE_MAP(CIdKeyFileDlg, CDialogExt)
	ON_BN_CLICKED(IDC_IDKEYSEL, OnIdkeysel)
	ON_BN_CLICKED(IDC_PASSDISP, &CIdKeyFileDlg::OnBnClickedPassdisp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg メッセージ ハンドラ

BOOL CIdKeyFileDlg::OnInitDialog() 
{
	CWnd *pWnd;
	BOOL bRet = TRUE;

	CDialogExt::OnInitDialog();

	SetWindowText(m_Title);
	switch(m_OpenMode) {
	case IDKFDMODE_LOAD:		// IdKey File Load
	case IDKFDMODE_SAVE:		// IdKey File Save
		break;
	case IDKFDMODE_CREATE:		// IdKey File Create
		if ( (pWnd = GetDlgItem(IDC_IDKEYFILE)) != NULL )
			pWnd->EnableWindow(FALSE);
		if ( (pWnd = GetDlgItem(IDC_IDKEYSEL)) != NULL )
			pWnd->EnableWindow(FALSE);
		break;
	case IDKFDMODE_OPEN:		// IdKey File Open
		if ( (pWnd = GetDlgItem(IDC_PASSNAME)) != NULL )
			pWnd->SetFocus();
		bRet = FALSE;
		break;
	}

	if ( (pWnd = GetDlgItem(IDC_PASSNAME)) != NULL )
		m_PassChar = ((CEdit *)pWnd)->GetPasswordChar();

	SetSaveProfile(_T("IdKeyFileDlg"));

	return bRet;
}

void CIdKeyFileDlg::OnIdkeysel() 
{
	UpdateData(TRUE);

	CFileDialog dlg(m_OpenMode == IDKFDMODE_LOAD ? TRUE : FALSE, _T(""), m_IdkeyFile, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_IdkeyFile = dlg.GetPathName();
	UpdateData(FALSE);
}

void CIdKeyFileDlg::OnOK() 
{
	UpdateData(TRUE);

	CDialogExt::OnOK();
}

void CIdKeyFileDlg::OnBnClickedPassdisp()
{
	CEdit *pWnd;

	UpdateData(TRUE);

	if ( (pWnd = (CEdit *)GetDlgItem(IDC_PASSNAME)) == NULL )
		return;

	pWnd->SetPasswordChar(m_bPassDisp ? _T('\0' : m_PassChar));
	pWnd->Invalidate(TRUE);
}
