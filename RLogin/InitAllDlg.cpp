// InitAllDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "InitAllDlg.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"

// CInitAllDlg ダイアログ

IMPLEMENT_DYNAMIC(CInitAllDlg, CDialogExt)

CInitAllDlg::CInitAllDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CInitAllDlg::IDD, pParent)
{
	m_InitType = 0;
	m_pInitEntry = NULL;
}

CInitAllDlg::~CInitAllDlg()
{
}

void CInitAllDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MSGICON, m_MsgIcon);
	DDX_Radio(pDX, IDC_RADIO1, m_InitType);
	DDX_Control(pDX, IDC_COMBO1, m_EntryCombo);
	DDX_Control(pDX, IDC_TITLE, m_TitleWnd);
}


BEGIN_MESSAGE_MAP(CInitAllDlg, CDialogExt)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO3, OnInitType)
END_MESSAGE_MAP()


// CInitAllDlg メッセージ ハンドラー


BOOL CInitAllDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	m_MsgIcon.SetIcon(::LoadIcon(NULL, IDI_QUESTION));

	int n;
	CString str;
	CServerEntryTab *pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);
	CWnd *pWnd;

	for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
		str = pTab->m_Data[n].m_EntryName;
		if ( m_EntryCombo.FindStringExact((-1), str) == CB_ERR )
			m_EntryCombo.AddString(str);
	}

	if ( n > 0 )
		m_EntryCombo.SetCurSel(0);
	else if ( (pWnd = GetDlgItem(IDC_RADIO3)) != NULL )
		pWnd->EnableWindow(FALSE);

	if ( m_InitType != 2 )
		m_EntryCombo.EnableWindow(FALSE);

	if ( !m_Title.IsEmpty() )
		m_TitleWnd.SetWindowText(m_Title);

	SetSaveProfile(_T("InitAllDlg"));

	return TRUE;
}

void CInitAllDlg::OnOK()
{
	int n;
	CString str;
	CServerEntryTab *pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);
		
	UpdateData(TRUE);

	if ( (n = m_EntryCombo.GetCurSel()) >= 0 )
		m_EntryCombo.GetLBText(n, str);

	for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
		if ( str.Compare(pTab->m_Data[n].m_EntryName) == 0 ) {
			m_pInitEntry = &(pTab->m_Data[n]);
			break;
		}
	}

	if ( m_InitType == 2 && m_pInitEntry == NULL )
		return;

	CDialogExt::OnOK();
}

void CInitAllDlg::OnInitType(UINT nID) 
{
	UpdateData(TRUE);

	m_EntryCombo.EnableWindow(m_InitType == 2 ? TRUE : FALSE);
}