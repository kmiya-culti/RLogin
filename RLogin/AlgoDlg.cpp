// AlgoDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "AlgoDlg.h"
#include "InitAllDlg.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg ダイアログ


CAlgoDlg::CAlgoDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CAlgoDlg::IDD, pParent)
{
	m_EncShuffle = FALSE;
	m_MacShuffle = FALSE;
}

void CAlgoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	for ( int n = 0 ; n < 12 ; n++ )
		DDX_Control(pDX, IDC_ALGO_LIST1 + n, m_List[n]);

	DDX_Check(pDX, IDC_CHECK1, m_EncShuffle);
	DDX_Check(pDX, IDC_CHECK2, m_MacShuffle);
}

BEGIN_MESSAGE_MAP(CAlgoDlg, CDialogExt)
	//{{AFX_MSG_MAP(CAlgoDlg)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_RESET, &CAlgoDlg::OnBnClickedReset)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg メッセージ ハンドラ

BOOL CAlgoDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	int n, i;

	for ( n = 0 ; n < 12 ; n++ ) {
		m_List[n].InsertColumn(0, _T(""), LVCFMT_LEFT, (n >= 9 && n <= 10 ? 260 : 150));
		for ( i = 0 ; i < m_AlgoTab[n].GetSize() ; i++ )
			m_List[n].InsertItem(i, m_AlgoTab[n][i]);
		m_List[n].m_bMove = TRUE;
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
	CDialogExt::OnOK();
}
void CAlgoDlg::OnBnClickedReset()
{
	int n, i;
	CTextRam *pTextRam   = new CTextRam;
	CKeyNodeTab *pKeyTab = new CKeyNodeTab;
	CKeyMacTab *pKeyMac  = new CKeyMacTab;
	CParamTab *pParamTab = new CParamTab;
	CInitAllDlg dlg;

	if ( dlg.DoModal() != IDOK )
		goto ENDOF;

	switch(dlg.m_InitType) {
	case 0:		// Init Default Entry
		pParamTab->Serialize(FALSE);
		break;

	case 1:		// Init Program Default
		pParamTab->Init();
		break;

	case 2:		// Copy Entry option
		ASSERT(dlg.m_pInitEntry != NULL);
		CRLoginDoc::LoadOption(*(dlg.m_pInitEntry), *pTextRam, *pKeyTab, *pKeyMac, *pParamTab);
		break;
	}

	for ( n = 0 ; n < 12 ; n++ ) {
		m_AlgoTab[n] = pParamTab->m_AlgoTab[n];
		m_List[n].DeleteAllItems();
		for ( i = 0 ; i < m_AlgoTab[n].GetSize() ; i++ )
			m_List[n].InsertItem(i, m_AlgoTab[n][i]);
	}

ENDOF:
	delete pTextRam;
	delete pKeyTab;
	delete pKeyMac;
	delete pParamTab;
}
