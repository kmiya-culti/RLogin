// KeyParaDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "KeyParaDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyParaDlg ダイアログ

IMPLEMENT_DYNAMIC(CKeyParaDlg, CDialogExt)

CKeyParaDlg::CKeyParaDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CKeyParaDlg::IDD, pParent)
{
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

	m_hMapsEdit = NULL;
	m_bCtrlMode = FALSE;

	m_pData = NULL;
}

void CKeyParaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

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
	DDX_Control(pDX, IDC_EDITCTRL, m_EditCtrl);
}

BEGIN_MESSAGE_MAP(CKeyParaDlg, CDialogExt)
	ON_BN_CLICKED(IDC_MENUBTN, &CKeyParaDlg::OnBnClickedMenubtn)
	ON_BN_CLICKED(IDC_EDITCTRL, &CKeyParaDlg::OnBnClickedEditctrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyParaDlg メッセージ ハンドラ

BOOL CKeyParaDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	ASSERT(m_pData != NULL);
	
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
	
	CWnd *pWnd;
	if ( (pWnd = GetDlgItem(IDC_KEYMAPS)) != NULL && (pWnd = pWnd->GetWindow(GW_CHILD)) != NULL )
		m_hMapsEdit = pWnd->GetSafeHwnd();

	return TRUE;
}

void CKeyParaDlg::OnOK() 
{
	ASSERT(m_pData != NULL);

	UpdateData(TRUE);

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
	CDialogExt::OnOK();
}

BOOL CKeyParaDlg::PreTranslateMessage(MSG* pMsg)
{
	if ( m_bCtrlMode && m_hMapsEdit != NULL && (pMsg->message == WM_KEYDOWN || pMsg->message == WM_CHAR) && pMsg->hwnd == m_hMapsEdit ) {
		if ( pMsg->message == WM_KEYDOWN ) {
			switch(pMsg->wParam) {
			case VK_CANCEL:
				pMsg->message = WM_CHAR;
				pMsg->wParam  = 0x03;
				break;
			case VK_TAB:
				pMsg->message = WM_CHAR;
				pMsg->wParam  = 0x09;
				break;
			case VK_CLEAR:
				pMsg->message = WM_CHAR;
				pMsg->wParam  = 0x0C;
				break;
			case VK_RETURN:
				pMsg->message = WM_CHAR;
				pMsg->wParam  = 0x0D;
				break;
			case VK_ESCAPE:
				pMsg->message = WM_CHAR;
				pMsg->wParam  = 0x1B;
				break;
			//case VK_PRIOR:
			//case VK_NEXT:
			//case VK_END:
			//case VK_HOME:
			//case VK_LEFT:
			//case VK_UP:
			//case VK_RIGHT:
			//case VK_DOWN:
			//case VK_PRINT:
			//case VK_EXECUTE:
			//case VK_SNAPSHOT:
			//case VK_INSERT:
			//case VK_DELETE:
			//case VK_HELP:
			//	return TRUE;
			}
		}
		if ( pMsg->message == WM_CHAR && pMsg->wParam < _T(' ') ) {
			CString tmp;
			tmp.Format(_T("\\%03o"), (int)pMsg->wParam);
			::SendMessage(m_hMapsEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(LPCTSTR)tmp);
			return TRUE;
		}
	}
	return CDialogExt::PreTranslateMessage(pMsg);
}

void CKeyParaDlg::OnBnClickedMenubtn()
{
	int id;
	CMenu PopUpMenu;
	CWnd *pWnd;
	CRect rect;
	LPCTSTR p;

	if ( !CMenuLoad::GetPopUpMenu(IDR_RLOGINTYPE, PopUpMenu) )
		return;

	if ( (pWnd = GetDlgItem(IDC_MENUBTN)) == NULL )
		return;

	pWnd->GetWindowRect(rect);

	id = ((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(&PopUpMenu, TPM_NONOTIFY | TPM_RETURNCMD, rect.right, rect.top, this, NULL);

	if ( id == 0 )
		return;

	if ( (p = CKeyNodeTab::GetCmdsStr(id)) == NULL )
		return;

	UpdateData(TRUE);
	m_Maps = p;
	UpdateData(FALSE);
}

void CKeyParaDlg::OnBnClickedEditctrl()
{
	m_bCtrlMode = (m_EditCtrl.GetCheck() == BST_CHECKED ? TRUE : FALSE);

	if ( m_hMapsEdit != NULL )
		::SetFocus(m_hMapsEdit);
}
