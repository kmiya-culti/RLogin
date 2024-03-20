// PfdDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "Data.h"
#include "PfdListDlg.h"
#include "PfdParaDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPfdParaDlg ダイアログ

IMPLEMENT_DYNAMIC(CPfdParaDlg, CDialogExt)

CPfdParaDlg::CPfdParaDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CPfdParaDlg::IDD, pParent)
{
	m_pEntry = NULL;
}

void CPfdParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_CBStringExact(pDX, IDC_LISTENHOST, m_Data.m_ListenHost);
	DDX_CBStringExact(pDX, IDC_LISTENPORT, m_Data.m_ListenPort);
	DDX_CBStringExact(pDX, IDC_CONNECTHOST, m_Data.m_ConnectHost);
	DDX_CBStringExact(pDX, IDC_CONNECTPORT, m_Data.m_ConnectPort);
	DDX_Radio(pDX, IDC_RADIO1, m_Data.m_ListenType);
}

void CPfdParaDlg::DisableWnd()
{
	CWnd *pWnd;

	if ( (pWnd = GetDlgItem(IDC_CONNECTHOST)) != NULL )
		pWnd->EnableWindow(PFD_IS_SOCKS(m_Data.m_ListenType) ? FALSE : TRUE);

	if ( (pWnd = GetDlgItem(IDC_CONNECTPORT)) != NULL )
		pWnd->EnableWindow(PFD_IS_SOCKS(m_Data.m_ListenType) ? FALSE : TRUE);
}

BEGIN_MESSAGE_MAP(CPfdParaDlg, CDialogExt)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO4, OnBnClickedListen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPfdParaDlg メッセージ ハンドラ

BOOL CPfdParaDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	CComboBox *pCombo;

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_LISTENHOST)) != NULL ) {
		pCombo->AddString(_T("localhost"));
		if ( m_pEntry != NULL && !m_pEntry->m_HostName.IsEmpty() )
			pCombo->AddString(m_pEntry->m_HostName);
	}

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CONNECTHOST)) != NULL ) {
		pCombo->AddString(_T("localhost"));
		if ( m_pEntry != NULL && !m_pEntry->m_HostName.IsEmpty() )
			pCombo->AddString(m_pEntry->m_HostName);
	}

	DisableWnd();
	UpdateData(FALSE);

	SubclassComboBox(IDC_LISTENHOST);
	SubclassComboBox(IDC_LISTENPORT);
	SubclassComboBox(IDC_CONNECTHOST);
	SubclassComboBox(IDC_CONNECTPORT);

	SetSaveProfile(_T("PfdParaDlg"));
	AddHelpButton(_T("#PFWDLG"));

	return TRUE;
}

void CPfdParaDlg::OnOK() 
{
	CString str;
	CStringArrayExt stra;

	UpdateData(TRUE);

	if ( m_Data.m_ListenHost.IsEmpty() || m_Data.m_ListenPort.IsEmpty() ) {
		::AfxMessageBox(CStringLoad(IDE_PFDLISTENEMPTY), MB_ICONERROR);
		return;
	}

	CDialogExt::OnOK();
}

void CPfdParaDlg::OnBnClickedListen(UINT nID)
{
	UpdateData(TRUE);
	DisableWnd();
}
