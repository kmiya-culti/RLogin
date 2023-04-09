// IConvDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "IConvDlg.h"
#include "IConv.h"

/////////////////////////////////////////////////////////////////////////////
// CIConvDlg ダイアログ

IMPLEMENT_DYNAMIC(CIConvDlg, CDialogExt)

CIConvDlg::CIConvDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CIConvDlg::IDD, pParent)
{
}

void CIConvDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	for ( int n = 0 ; n < 4 ; n++ )
		DDX_CBStringExact(pDX, IDC_CHARSET1 + n, m_CharSet[n]);
}

BEGIN_MESSAGE_MAP(CIConvDlg, CDialogExt)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIConvDlg メッセージ ハンドラ

BOOL CIConvDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	int n, i;
	CStringArray stra;
	CComboBox *pCombo;
	
	CIConv::SetListArray(stra);
	for ( i = 0 ; i < 4 ; i++ ) {
		if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CHARSET1 + i)) != NULL ) {
			for ( n = 0 ; n < stra.GetSize() ; n++ )
				pCombo->AddString(stra[n]);
		}

		SubclassComboBox(IDC_CHARSET1 + i);
	}
	
	return TRUE;
}
