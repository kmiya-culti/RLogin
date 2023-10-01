// AlgoDlg.cpp : �C���v�������e�[�V���� �t�@�C��
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
// CAlgoDlg �_�C�A���O


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
	ON_WM_SIZE()
END_MESSAGE_MAP()

static const INITDLGTAB ItemTab[] = {
	{ IDC_ALGO_LIST1,	ITM_RIGHT_PER | ITM_BTM_PER },
	{ IDC_ALGO_LIST2,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_BTM_PER },
	{ IDC_ALGO_LIST3,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_BTM_PER },

	{ IDC_ALGO_LIST4,	ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_ALGO_LIST5,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_ALGO_LIST6,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },

	{ IDC_ALGO_LIST7,	ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_BTM },
	{ IDC_ALGO_LIST8,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_BTM },
	{ IDC_ALGO_LIST9,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_BTM },

	{ IDC_ALGO_LIST10,	ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_ALGO_LIST11,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_ALGO_LIST12,	ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },

	{ IDC_RESET,		ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_GROUP1,		ITM_RIGHT_RIGHT | ITM_BTM_PER },
	{ IDC_GROUP2,		ITM_RIGHT_RIGHT | ITM_TOP_PER | ITM_BTM_BTM },

	{ IDC_TITLE1,		ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_TITLE2,		ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_TITLE3,		ITM_TOP_PER | ITM_BTM_PER },

	{ IDC_TITLE4,		ITM_LEFT_PER | ITM_RIGHT_PER },
	{ IDC_TITLE5,		ITM_LEFT_PER | ITM_RIGHT_PER },
	{ IDC_TITLE6,		ITM_LEFT_PER | ITM_RIGHT_PER },

	{ IDC_TITLE7,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_TITLE8,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_TITLE9,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },

	{ IDC_CHECK1,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_CHECK2,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },
	{ IDC_TITLE10,		ITM_LEFT_PER | ITM_RIGHT_PER | ITM_TOP_PER | ITM_BTM_PER },

	{ 0,	0 },
};

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg ���b�Z�[�W �n���h��

BOOL CAlgoDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	int n, i;
	CRect rect;

	for ( n = 0 ; n < 12 ; n++ ) {
		m_List[n].InsertColumn(0, _T(""), LVCFMT_LEFT, 200);
		for ( i = 0 ; i < m_AlgoTab[n].GetSize() ; i++ )
			m_List[n].InsertItem(i, m_AlgoTab[n][i]);
		m_List[n].m_bMove = TRUE;

		m_List[n].GetClientRect(rect);
		m_List[n].SetColumnWidth(0, rect.Width() * 95 / 100);
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

void CAlgoDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogExt::OnSize(nType, cx, cy);

	int n;
	CRect rect;

	for ( n = 0 ; n < 12 ; n++ ) {
		if ( m_List[n].GetSafeHwnd() == NULL )
			continue;
		m_List[n].GetClientRect(rect);
		m_List[n].SetColumnWidth(0, rect.Width() * 95 / 100);
	}
}
