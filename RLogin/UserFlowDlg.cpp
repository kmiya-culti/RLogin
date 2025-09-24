// UserFlowDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ComSock.h"
#include "UserFlowDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CUserFlowDlg ダイアログ

IMPLEMENT_DYNAMIC(CUserFlowDlg, CDialogExt)

CUserFlowDlg::CUserFlowDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CUserFlowDlg::IDD, pParent)
{
	m_pDCB = NULL;

	for (int n = 0; n < 6; n++) {
		m_Value[n] = 0;
		m_Check[n] = FALSE;
	}
}

CUserFlowDlg::~CUserFlowDlg()
{
}

void CUserFlowDlg::DoDataExchange(CDataExchange* pDX)
{
	int n;

	CDialogExt::DoDataExchange(pDX);

	for ( n = 0 ; n < 2 ; n++ )
		DDX_CBIndex(pDX, IDC_COMBO1 + n, m_Value[n]);

	for ( n = 0 ; n < 6 ; n++ )
		DDX_Check(pDX, IDC_CHECK1 + n, m_Check[n]);

	for ( n = 0 ; n < 2 ; n++ )
		DDX_Text(pDX, IDC_EDIT1 + n, m_Value[2 + n]);

	for ( n = 0 ; n < 2 ; n++ )
		DDX_Text(pDX, IDC_EDIT3 + n, m_CtrlChar[n]);
}

BEGIN_MESSAGE_MAP(CUserFlowDlg, CDialogExt)
	ON_CONTROL_RANGE(CBN_SELCHANGE,  IDC_COMBO1, IDC_COMBO2, OnUpdate)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECK1,  IDC_CHECK6,  OnUpdate)
	ON_CONTROL_RANGE(EN_CHANGE, IDC_EDIT1, IDC_EDIT4, OnUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserFlowDlg メッセージ ハンドラー


BOOL CUserFlowDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	ASSERT(m_pDCB != NULL);

	m_Value[0] = m_pDCB->fDtrControl;
	m_Check[0] = m_pDCB->fOutxDsrFlow;
	m_Check[1] = m_pDCB->fDsrSensitivity;

	m_Value[1] = m_pDCB->fRtsControl;
	m_Check[2] = m_pDCB->fOutxCtsFlow;

	m_Check[3] = m_pDCB->fInX;
	m_Check[4] = m_pDCB->fOutX;
	m_Check[5] = m_pDCB->fTXContinueOnXoff;

	m_Value[2] = 100 - 100 * m_pDCB->XoffLim / COMQUEUESIZE;
	m_Value[3] = 100 * m_pDCB->XonLim / COMQUEUESIZE;

	m_Value[4] = m_pDCB->XoffChar;
	m_Value[5] = m_pDCB->XonChar;

	for ( int n = 0 ; n < 2 ; n++ ) {
		if ( m_Value[4 + n] < 32 )
			m_CtrlChar[n].Format(_T("^%c"), _T('@') + m_Value[4 + n]);
		else
			m_CtrlChar[n].Format(_T("%d"), m_Value[4 + n]);
	}

	UpdateData(FALSE);

	SetSaveProfile(_T("UserFlowDlg"));
	AddHelpButton(_T("#COMFLOW"));

	return TRUE;
}
void CUserFlowDlg::OnOK()
{
	UpdateData(TRUE);

	for ( int n = 0 ; n < 2 ; n++ ) {
		LPCTSTR p = m_CtrlChar[n];

		if ( p[0] == _T('^') && p[1] >= _T('@') && p[1] <= _T('_') )
			m_Value[4 + n] = p[1] - _T('@');
		else if ( *p != _T('\0') )
			m_Value[4 + n]  = _tstoi(p);
		else
			m_Value[4 + n] = 0;
	}

	if ( m_Value[2] < 50 )
		m_Value[2] = 50;
	else if ( m_Value[2] > 90 )
		m_Value[2] = 90;

	if ( m_Value[3] > 50 )
		m_Value[3] = 50;
	else if ( m_Value[3] < 10 )
		m_Value[3] = 10;

	m_pDCB->fDtrControl			= m_Value[0];
	m_pDCB->fOutxDsrFlow		= m_Check[0];
	m_pDCB->fDsrSensitivity		= m_Check[1];

	m_pDCB->fRtsControl			= m_Value[1];
	m_pDCB->fOutxCtsFlow		= m_Check[2];

	m_pDCB->fInX				= m_Check[3];
	m_pDCB->fOutX				= m_Check[4];
	m_pDCB->fTXContinueOnXoff	= m_Check[5];

	m_pDCB->XoffLim				= COMQUEUESIZE - COMQUEUESIZE * m_Value[2] / 100;
	m_pDCB->XonLim				= COMQUEUESIZE * m_Value[3] / 100;
	m_pDCB->XoffChar			= m_Value[4];
	m_pDCB->XonChar				= m_Value[5];

	CDialogExt::OnOK();
}
void CUserFlowDlg::OnUpdate(UINT nID)
{
}