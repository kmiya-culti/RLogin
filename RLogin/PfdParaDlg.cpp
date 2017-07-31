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

CPfdParaDlg::CPfdParaDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPfdParaDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPfdParaDlg)
	m_ListenHost = _T("");
	m_ListenPort = _T("");
	m_ConnectHost = _T("");
	m_ConnectPort = _T("");
	//}}AFX_DATA_INIT
	m_pData = NULL;
	m_pEntry = NULL;
	m_EntryNum = (-1);
}

void CPfdParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPfdParaDlg)
	DDX_CBString(pDX, IDC_LISTENHOST, m_ListenHost);
	DDX_CBString(pDX, IDC_LISTENPORT, m_ListenPort);
	DDX_CBString(pDX, IDC_CONNECTHOST, m_ConnectHost);
	DDX_CBString(pDX, IDC_CONNECTPORT, m_ConnectPort);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPfdParaDlg, CDialog)
	//{{AFX_MSG_MAP(CPfdParaDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPfdParaDlg メッセージ ハンドラ

BOOL CPfdParaDlg::OnInitDialog() 
{
	ASSERT(m_pData);

	CDialog::OnInitDialog();
	CStringArrayExt array;

	if ( m_EntryNum >= 0 && m_EntryNum < m_pData->m_PortFwd.GetSize() )
		array.GetString(m_pData->m_PortFwd[m_EntryNum]);
	else
		m_EntryNum = (-1);

	if ( array.GetSize() >= 4 ) {
		m_ListenHost  = array[0];
		m_ListenPort  = array[1];
		m_ConnectHost = array[2];
		m_ConnectPort = array[3];
	}

	CComboBox *pCombo;
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_LISTENHOST)) != NULL ) {
		pCombo->AddString("socks");
		pCombo->AddString("localhost");
		if ( m_pEntry != NULL && !m_pEntry->m_HostName.IsEmpty() )
			pCombo->AddString(m_pEntry->m_HostName);
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CONNECTHOST)) != NULL ) {
		pCombo->AddString("localhost");
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
	array.SetString(str);

	if ( m_EntryNum >= 0 )
		m_pData->m_PortFwd[m_EntryNum] = str;
	else
		m_pData->m_PortFwd.Add(str);

	CDialog::OnOK();
}
