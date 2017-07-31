// IConvDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "IConvDlg.h"
#include "IConv.h"

/////////////////////////////////////////////////////////////////////////////
// CIConvDlg ダイアログ

IMPLEMENT_DYNAMIC(CIConvDlg, CDialog)

CIConvDlg::CIConvDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIConvDlg::IDD, pParent)
{
}

void CIConvDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	for ( int n = 0 ; n < 4 ; n++ )
		DDX_CBString(pDX, IDC_CHARSET1 + n, m_CharSet[n]);
}

BEGIN_MESSAGE_MAP(CIConvDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIConvDlg メッセージ ハンドラ

BOOL CIConvDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int n, i;
	CStringArray array;
	CComboBox *pCombo;
	
	CIConv::SetListArray(array);
	for ( i = 0 ; i < 4 ; i++ ) {
		if ( (pCombo = (CComboBox *)GetDlgItem(IDC_CHARSET1 + i)) != NULL ) {
			for ( n = 0 ; n < array.GetSize() ; n++ )
				pCombo->AddString(array[n]);
		}
	}
	
	return TRUE;
}
