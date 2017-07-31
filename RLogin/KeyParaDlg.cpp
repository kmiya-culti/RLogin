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
}

BEGIN_MESSAGE_MAP(CKeyParaDlg, CDialogExt)
	ON_BN_CLICKED(IDC_MENUBTN, &CKeyParaDlg::OnBnClickedMenubtn)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyParaDlg メッセージ ハンドラ

BOOL CKeyParaDlg::OnInitDialog() 
{
	ASSERT(m_pData != NULL);

	CDialogExt::OnInitDialog();
	
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

void CKeyParaDlg::OnBnClickedMenubtn()
{
	int n, id;
	CString str;
	CMenu PopUpMenu;
	CMenu DefMenu, *pMenu;
	CWnd *pWnd;
	CRect rect;
	LPCTSTR p;

	if ( !PopUpMenu.CreatePopupMenu() )
		return;

	if ( !DefMenu.LoadMenu(IDR_RLOGINTYPE) )
		return;

	((CMainFrame *)::AfxGetMainWnd())->SetMenuBitmap(&DefMenu);

	for ( n = 0 ; n < DefMenu.GetMenuItemCount() ; n++ ) {
		DefMenu.GetMenuString(n, str, MF_BYPOSITION);
		pMenu = DefMenu.GetSubMenu(n);
		PopUpMenu.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)pMenu->GetSafeHmenu(), str);
	}

	pMenu = PopUpMenu.GetSubMenu(1);
	pMenu->DeleteMenu(ID_MACRO_HIS1, MF_BYCOMMAND);
	for ( n = 0 ; n < 5 ; n++ ) {
		str.Format(_T("&%d 以前に記録したキー操作"), n + 1);
		pMenu->AppendMenu(MF_STRING, ID_MACRO_HIS1 + n, str);
	}

	for ( n = 0 ; n < 5 ; n++ ) {
		str.Format(_T("&%d スクリプトメニュー"), n + 1);
		pMenu->InsertMenu(ID_CHARSCRIPT_END, MF_BYCOMMAND, IDM_SCRIPT_MENU1 + n, str);
	}

	pMenu = pMenu->GetSubMenu(4);
	pMenu->DeleteMenu(IDM_CLIPBOARD_HIS1, MF_BYCOMMAND);
	for ( n = 0 ; n < 10 ; n++ ) {
		str.Format(_T("&%d 保存されたクリップボード"), (n + 1) % 10);
		pMenu->AppendMenu(MF_STRING, IDM_CLIPBOARD_HIS1 + n, str);
	}

	pMenu = PopUpMenu.GetSubMenu(3);
	pMenu->AppendMenu(MF_SEPARATOR);
	for ( n = 0 ; n < 10 ; n++ ) {
		str.Format(_T("&%d ウィンドウ"), (n + 1) % 10);
		pMenu->AppendMenu(MF_STRING, AFX_IDM_FIRST_MDICHILD + n, str);
	}

	if ( (pWnd = GetDlgItem(IDC_MENUBTN)) == NULL )
		return;

	pWnd->GetWindowRect(rect);

	id = PopUpMenu.TrackPopupMenuEx(TPM_NONOTIFY | TPM_RETURNCMD, rect.right, rect.top, this, NULL);

	if ( id == 0 )
		return;

	if ( id == IDM_NEWCONNECT )
		id = ID_FILE_NEW;

	if ( (p = CKeyNodeTab::GetCmdsStr(id)) == NULL )
		return;

	m_Maps = p;
	UpdateData(FALSE);
}
