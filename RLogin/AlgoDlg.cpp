// AlgoDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "AlgoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg ダイアログ


CAlgoDlg::CAlgoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAlgoDlg::IDD, pParent)
{
	m_EncShuffle = FALSE;
	m_MacShuffle = FALSE;
}

void CAlgoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	for ( int n = 0 ; n < 12 ; n++ )
		DDX_Control(pDX, IDC_ALGO_LIST1 + n, m_List[n]);

	DDX_Check(pDX, IDC_CHECK1, m_EncShuffle);
	DDX_Check(pDX, IDC_CHECK2, m_MacShuffle);
}

BEGIN_MESSAGE_MAP(CAlgoDlg, CDialog)
	//{{AFX_MSG_MAP(CAlgoDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg メッセージ ハンドラ

BOOL CAlgoDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int n, i;

	for ( n = 0 ; n < 12 ; n++ ) {
		m_List[n].InsertColumn(0, _T(""), LVCFMT_LEFT, (n < 9 ? 120 : (n < 10 ? 260 : 160)));
		for ( i = 0 ; i < m_AlgoTab[n].GetSize() ; i++ )
			m_List[n].InsertItem(i, m_AlgoTab[n][i]);
	}
		
	return TRUE;
}

void CAlgoDlg::OnOK() 
{
	int n, i;

	for ( n = 0 ; n < 12 ; n++ ) {
		m_AlgoTab[n].RemoveAll();
		for ( i = 0 ; i < m_List[n].GetItemCount() ; i++ )
			m_AlgoTab[n].Add(m_List[n].GetItemText(i, 0));
	}
	CDialog::OnOK();
}
