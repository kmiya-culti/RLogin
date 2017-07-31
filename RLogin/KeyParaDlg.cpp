// KeyParaDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "KeyParaDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CKeyParaDlg ダイアログ

CKeyParaDlg::CKeyParaDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CKeyParaDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CKeyParaDlg)
	m_WithShift = FALSE;
	m_WithCtrl  = FALSE;
	m_WithAlt   = FALSE;
	m_WithAppli = FALSE;
	m_WithCkm   = FALSE;
	m_WithVt52  = FALSE;
	m_WithNum   = FALSE;
	m_WithScr   = FALSE;
	m_WithCap   = FALSE;
	m_KeyCode = _T("");
	m_Maps = _T("");
	//}}AFX_DATA_INIT
	m_pData = NULL;
}

void CKeyParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CKeyParaDlg)
	DDX_Check(pDX, IDC_KEYSTAT1, m_WithShift);
	DDX_Check(pDX, IDC_KEYSTAT2, m_WithCtrl);
	DDX_Check(pDX, IDC_KEYSTAT3, m_WithAlt);
	DDX_Check(pDX, IDC_KEYSTAT4, m_WithAppli);
	DDX_Check(pDX, IDC_KEYSTAT5, m_WithCkm);
	DDX_Check(pDX, IDC_KEYSTAT6, m_WithVt52);
	//DDX_Check(pDX, IDC_KEYSTAT7, m_WithNum);
	//DDX_Check(pDX, IDC_KEYSTAT8, m_WithScr);
	//DDX_Check(pDX, IDC_KEYSTAT9, m_WithCap);
	DDX_CBString(pDX, IDC_KEYCODE, m_KeyCode);
	DDX_CBString(pDX, IDC_KEYMAPS, m_Maps);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CKeyParaDlg, CDialog)
	//{{AFX_MSG_MAP(CKeyParaDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyParaDlg メッセージ ハンドラ

BOOL CKeyParaDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	ASSERT(m_pData);
	m_KeyCode   = m_pData->GetCode();
	m_WithShift = (m_pData->m_Mask & MASK_SHIFT)  ? TRUE : FALSE;
	m_WithCtrl  = (m_pData->m_Mask & MASK_CTRL)   ? TRUE : FALSE;
	m_WithAlt   = (m_pData->m_Mask & MASK_ALT)    ? TRUE : FALSE;
	m_WithAppli = (m_pData->m_Mask & MASK_APPL)   ? TRUE : FALSE;
	m_WithCkm   = (m_pData->m_Mask & MASK_CKM)    ? TRUE : FALSE;
	m_WithVt52  = (m_pData->m_Mask & MASK_VT52)   ? TRUE : FALSE;
	//m_WithNum   = (m_pData->m_Mask & MASK_NUMLCK) ? TRUE : FALSE;
	//m_WithScr   = (m_pData->m_Mask & MASK_SCRLCK) ? TRUE : FALSE;
	//m_WithCap   = (m_pData->m_Mask & MASK_CAPLCK) ? TRUE : FALSE;
	m_Maps = m_pData->GetMaps();
	
	m_pData->SetComboList((CComboBox *)GetDlgItem(IDC_KEYCODE));
	CKeyNodeTab::SetComboList((CComboBox *)GetDlgItem(IDC_KEYMAPS));

	UpdateData(FALSE);

	return TRUE;
}

void CKeyParaDlg::OnOK() 
{
	UpdateData(TRUE);
	ASSERT(m_pData);
	m_pData->SetCode(m_KeyCode);
	m_pData->m_Mask = 0;
	if ( m_WithShift ) m_pData->m_Mask |= MASK_SHIFT;
	if ( m_WithCtrl )  m_pData->m_Mask |= MASK_CTRL;
	if ( m_WithAlt  )  m_pData->m_Mask |= MASK_ALT;
	if ( m_WithAppli ) m_pData->m_Mask |= MASK_APPL;
	if ( m_WithCkm )   m_pData->m_Mask |= MASK_CKM;
	if ( m_WithVt52 )  m_pData->m_Mask |= MASK_VT52;
	//if ( m_WithNum )   m_pData->m_Mask |= MASK_NUMLCK;
	//if ( m_WithScr )   m_pData->m_Mask |= MASK_SCRLCK;
	//if ( m_WithCap )   m_pData->m_Mask |= MASK_CAPLCK;
	m_pData->SetMaps(m_Maps);
	CDialog::OnOK();
}
