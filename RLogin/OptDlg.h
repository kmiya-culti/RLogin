#pragma once

/////////////////////////////////////////////////////////////////////////////
// CTreePropertyPage

class CTreePropertyPage : public CPropertyPage
{
public:
	class COptDlg *m_pSheet;
	HTREEITEM m_hTreeItem;
	class CTreePropertyPage *m_pOwn;

	CTreePropertyPage();
	explicit CTreePropertyPage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
	virtual void OnReset();
};

#include "SerEntPage.h"
#include "TermPage.h"
#include "ScriptPage.h"
#include "ScrnPage.h"
#include "SockOptPage.h"
#include "ProtoPage.h"
#include "CharSetPage.h"
#include "ColParaDlg.h"
#include "KeyPage.h"
#include "HisPage.h"
#include "ClipPage.h"
#include "MousePage.h"
#include "ModKeyPage.h"

/////////////////////////////////////////////////////////////////////////////
// COptDlg

#define	UMOD_ENTRY		0001
#define	UMOD_KEYTAB		0002
#define	UMOD_PARAMTAB	0004
#define	UMOD_TEXTRAM	0010
#define	UMOD_ANSIOPT	0020
#define	UMOD_MODKEY		0040
#define	UMOD_COLTAB		0100
#define	UMOD_BANKTAB	0200
#define	UMOD_DEFATT		0400

class COptDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(COptDlg)

// コンストラクション
public:
	COptDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// クラスデータ
public:
	CButton m_DoInit;
	CStatic m_wndFrame;
	CTreeCtrl m_wndTree;

	CSerEntPage m_SerEntPage;
	CKeyPage m_KeyPage;
	CColParaDlg m_ColorPage;
	CCharSetPage m_CharSetPage;
	CSockOptPage m_SockOptPage;
	CProtoPage m_ProtoPage;
	CTermPage m_TermPage;
	CScrnPage m_ScrnPage;
	CHisPage m_HisPage;
	CClipPage m_ClipPage;
	CMousePage m_MousePage;
	CScriptPage m_ScriptPage;
	CModKeyPage m_ModKeyPage;

	CServerEntry *m_pEntry;
	CTextRam *m_pTextRam;
	CKeyNodeTab *m_pKeyTab;
	CKeyMacTab *m_pKeyMac;
	CParamTab *m_pParamTab;
	class CRLoginDoc *m_pDocument;
	int m_ModFlag;

// クラスファンクション
public:
	void AddPage(CTreePropertyPage *pPage, CTreePropertyPage *pOwn = NULL);

// オーバーライド
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	afx_msg void OnDoInit();
	afx_msg void OnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};
