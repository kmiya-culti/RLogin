// OptDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "OptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptDlg

IMPLEMENT_DYNAMIC(COptDlg, CPropertySheet)

COptDlg::COptDlg(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_ModFlag   = 0;
	m_pEntry    = NULL;
	m_pTextRam  = NULL;
	m_pKeyTab   = NULL;
	m_pParamTab = NULL;
	m_pDocument = NULL;

	AddPage(&m_SerEntPage);	 m_SerEntPage.m_pSheet  = this;
	AddPage(&m_TermPage);	 m_TermPage.m_pSheet    = this;
	AddPage(&m_ScrnPage);	 m_ScrnPage.m_pSheet    = this;
	AddPage(&m_HisPage);     m_HisPage.m_pSheet     = this;
	AddPage(&m_ProtoPage);	 m_ProtoPage.m_pSheet   = this;
	AddPage(&m_CharSetPage); m_CharSetPage.m_pSheet = this;
	AddPage(&m_ColorPage);   m_ColorPage.m_pSheet   = this;
	AddPage(&m_KeyPage);     m_KeyPage.m_pSheet     = this;
}

COptDlg::COptDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_ModFlag   = 0;
	m_pEntry    = NULL;
	m_pTextRam  = NULL;
	m_pKeyTab   = NULL;
	m_pParamTab = NULL;
	m_pDocument = NULL;

	AddPage(&m_SerEntPage);	 m_SerEntPage.m_pSheet  = this;
	AddPage(&m_TermPage);    m_TermPage.m_pSheet    = this;
	AddPage(&m_ScrnPage);	 m_ScrnPage.m_pSheet    = this;
	AddPage(&m_HisPage);     m_HisPage.m_pSheet     = this;
	AddPage(&m_ProtoPage);   m_ProtoPage.m_pSheet   = this;
	AddPage(&m_CharSetPage); m_CharSetPage.m_pSheet = this;
	AddPage(&m_ColorPage);   m_ColorPage.m_pSheet   = this;
	AddPage(&m_KeyPage);     m_KeyPage.m_pSheet     = this;
}

COptDlg::~COptDlg()
{
}

BEGIN_MESSAGE_MAP(COptDlg, CPropertySheet)
	//{{AFX_MSG_MAP(COptDlg)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_DOINIT, OnDoInit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptDlg メッセージ ハンドラ

BOOL COptDlg::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	int n;
	CWnd *pWnd[4];
	CRect rect[4], temp, frame;
	static int idTab[] = { IDC_DOINIT, IDOK, IDCANCEL, ID_APPLY_NOW };

	for ( n = 1 ; n < 4 ; n++ ) {
		if ( (pWnd[n] = GetDlgItem(idTab[n])) != NULL ) {
			pWnd[n]->GetWindowRect(rect[n]);
			ScreenToClient(rect[n]);
		}
	}

	if ( pWnd[1] == NULL || pWnd[2] == NULL )
		return bResult;

	rect[0] = rect[1];
	rect[0].left  -= (rect[2].left  - rect[1].left);
	rect[0].right -= (rect[2].right - rect[1].right);
	m_DoInit.Create("初期化", BS_PUSHBUTTON, rect[0], this, IDC_DOINIT);

	CFont *pFont = pWnd[1]->GetFont();
	m_DoInit.SetFont(pFont, FALSE);
	m_DoInit.ShowWindow(SW_SHOW);
	pWnd[0] = &m_DoInit;

	GetClientRect(frame);
	temp.left  = rect[0].left;
	temp.right = ((m_psh.dwFlags & PSH_NOAPPLYNOW) == 0 ? rect[3].right : rect[2].right);
	int offs  = temp.left + temp.Width() / 2 - frame.Width() / 2;

	for ( n = 0 ; n < 4 ; n++ ) {
		if ( pWnd[n] == NULL )
			continue;
		rect[n].left  -= offs;
		rect[n].right -= offs;
		pWnd[n]->MoveWindow(rect[n]);
	}

	return bResult;
}
void COptDlg::OnDoInit()
{
	if ( MessageBox("すべての設定を標準に戻しますか？", "初期化", MB_ICONQUESTION | MB_YESNO) != IDYES )
		return;

	m_pTextRam->Serialize(FALSE);
	m_pKeyTab->Serialize(FALSE);
	m_pParamTab->Serialize(FALSE);

	m_SerEntPage.OnReset();
	m_TermPage.OnReset();
	m_ScrnPage.OnReset();
	m_ProtoPage.OnReset();
	m_CharSetPage.OnReset();
	m_ColorPage.OnReset();
	m_KeyPage.OnReset();
	m_HisPage.OnReset();

	if ( m_pDocument != NULL )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	m_ModFlag   = 0;
}

BOOL COptDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	BOOL rt = CPropertySheet::OnCommand(wParam, lParam);
	if ( wParam == ID_APPLY_NOW && m_pDocument != NULL )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
	return rt;
}
