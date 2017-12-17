// StatusDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ssh.h"
#include "StatusDlg.h"
#include "Script.h"

// CStatusDlg ダイアログ

IMPLEMENT_DYNAMIC(CStatusDlg, CDialogExt)

CStatusDlg::CStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CStatusDlg::IDD, pParent)
{
	m_Status.Empty();
	m_pValue = NULL;
}

CStatusDlg::~CStatusDlg()
{
}

void CStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Status);
}

BEGIN_MESSAGE_MAP(CStatusDlg, CDialogExt)
	ON_WM_SIZE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

// CStatusDlg メッセージ ハンドラ

BOOL CStatusDlg::OnInitDialog()
{
	CWnd *pWnd;

	CDialogExt::OnInitDialog();

	if ( !m_Title.IsEmpty() )
		SetWindowText(m_Title);

//	if ( (pWnd = GetDlgItem(IDC_EDIT1)) != NULL && m_StatusFont.CreateStockObject(OEM_FIXED_FONT) )
	if ( (pWnd = GetDlgItem(IDC_EDIT1)) != NULL && m_StatusFont.CreatePointFont(9 * 10, _T("Consolas")) )
		pWnd->SetFont(&m_StatusFont);

	return TRUE;
}

void CStatusDlg::OnSize(UINT nType, int cx, int cy)
{
	CWnd *pWnd;
	WINDOWPLACEMENT place;

	if ( (pWnd = GetDlgItem(IDC_EDIT1)) != NULL ) {
		pWnd->GetWindowPlacement(&place);
		place.rcNormalPosition.right  = cx;
		place.rcNormalPosition.bottom = cy;
		pWnd->SetWindowPlacement(&place);
	}

	CDialogExt::OnSize(nType, cx, cy);
}

void CStatusDlg::GetStatusText(CString &status)
{
	UpdateData(TRUE);
	status = m_Status;
}

void CStatusDlg::SetStatusText(LPCTSTR status)
{
	m_Status = status;
	UpdateData(FALSE);
}

void CStatusDlg::AddStatusText(LPCTSTR status)
{
	m_Status += status;
	UpdateData(FALSE);

	CEdit *pEdit;
	int len = m_Status.GetLength();

	if ( (pEdit = (CEdit *)GetDlgItem(IDC_EDIT1)) == NULL )
		return;

	pEdit->SetSel(len, len, FALSE);
}

void CStatusDlg::PostNcDestroy()
{
	if ( m_pValue != NULL ) {
		((CScriptValue *)m_pValue)->GetAt("pWnd") = (void *)NULL;
		delete this;
	}
}

void CStatusDlg::OnClose()
{
	CDialogExt::OnClose();

	if ( m_pValue != NULL )
		DestroyWindow();
}
