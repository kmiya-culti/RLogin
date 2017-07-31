// PfdDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "Data.h"
#include "PfdParaDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPfdParaDlg ダイアログ

IMPLEMENT_DYNAMIC(CPfdParaDlg, CDialog)

CPfdParaDlg::CPfdParaDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPfdParaDlg::IDD, pParent)
{
	m_ListenHost = _T("");
	m_ListenPort = _T("");
	m_ConnectHost = _T("");
	m_ConnectPort = _T("");
	m_pData = NULL;
	m_pEntry = NULL;
	m_EntryNum = (-1);
	m_ListenType = 0;
}

void CPfdParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_CBString(pDX, IDC_LISTENHOST, m_ListenHost);
	DDX_CBString(pDX, IDC_LISTENPORT, m_ListenPort);
	DDX_CBString(pDX, IDC_CONNECTHOST, m_ConnectHost);
	DDX_CBString(pDX, IDC_CONNECTPORT, m_ConnectPort);
	DDX_Radio(pDX, IDC_RADIO1, m_ListenType);
}

BEGIN_MESSAGE_MAP(CPfdParaDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPfdParaDlg メッセージ ハンドラ

BOOL CPfdParaDlg::OnInitDialog() 
{
	ASSERT(m_pData);

	CDialog::OnInitDialog();
	CStringArrayExt array;

	if ( m_EntryNum >= 0 && m_EntryNum < m_pData->GetSize() )
		array.GetString(m_pData->GetAt(m_EntryNum));
	else
		m_EntryNum = (-1);

	if ( array.GetSize() >= 5 ) {
		m_ListenHost  = array[0];
		m_ListenPort  = array[1];
		m_ConnectHost = array[2];
		m_ConnectPort = array[3];
		m_ListenType  = array.GetVal(4);
	}

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

	UpdateData(FALSE);

	return TRUE;
}

void CPfdParaDlg::OnOK() 
{
	ASSERT(m_pData);

	CString str;
	CStringArrayExt array;

	UpdateData(TRUE);
	array.Add(m_ListenHost);
	array.Add(m_ListenPort);
	array.Add(m_ConnectHost);
	array.Add(m_ConnectPort);
	array.AddVal(m_ListenType);
	array.SetString(str);

	if ( m_EntryNum >= 0 )
		(*m_pData)[m_EntryNum] = str;
	else
		m_pData->Add(str);

	CDialog::OnOK();
}
