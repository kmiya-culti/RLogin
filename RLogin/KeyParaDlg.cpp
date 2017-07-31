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
	m_WithCtrl = FALSE;
	m_WithAppli = FALSE;
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
	DDX_Check(pDX, IDC_KEYSTAT3, m_WithAppli);
	DDX_CBString(pDX, IDC_KEYCODE, m_KeyCode);
	DDX_Text(pDX, IDC_KEYMAPS, m_Maps);
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
	m_WithShift = (m_pData->m_Mask & MASK_SHIFT) ? TRUE : FALSE;
	m_WithCtrl  = (m_pData->m_Mask & MASK_CTRL)  ? TRUE : FALSE;
	m_WithAppli = (m_pData->m_Mask & MASK_APPL)   ? TRUE : FALSE;
	m_Maps = m_pData->GetMaps();
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
	if ( m_WithAppli ) m_pData->m_Mask |= MASK_APPL;
	m_pData->SetMaps(m_Maps);
	CDialog::OnOK();
}
