#pragma once

#include "afxcmn.h"
#include "afxwin.h"
#include <afxtempl.h>
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CTreePage

class CTreePage : public CDialogExt
{
	DECLARE_DYNAMIC(CTreePage)

public:
	UINT m_nIDTemplate;
	class COptDlg *m_pSheet;
	HTREEITEM m_hTreeItem;
	class CTreePage *m_pOwn;
	int m_nPage;

	CTreePage(UINT nIDTemplate);
	virtual ~CTreePage();

	void SetModified(BOOL bModified);

// オーバーライド
public:
	virtual void OnReset();
	virtual BOOL OnApply();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
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

#define	UMOD_ENTRY		00001
#define	UMOD_KEYTAB		00002
#define	UMOD_PARAMTAB	00004
#define	UMOD_TEXTRAM	00010
#define	UMOD_ANSIOPT	00020
#define	UMOD_MODKEY		00040
#define	UMOD_COLTAB		00100
#define	UMOD_BANKTAB	00200
#define	UMOD_DEFATT		00400
#define	UMOD_RESIZE		01000

class COptDlg : public CDialogExt
{
	DECLARE_DYNAMIC(COptDlg)

// コンストラクション
public:
	COptDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL);
	virtual ~COptDlg();

// ダイアログ データ
	enum { IDD = IDD_OPTDLG };

// クラスデータ
public:
	CTreeCtrl m_Tree;
	CStatic m_Frame;
	CButton m_Button[4];

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
	BOOL m_bModified;
	CString m_Title;

	struct _OptTab {
		DWORD	dwFlags;
	} m_psh;
	CPtrArray m_Tab;
	int m_ActivePage;

// クラスファンクション
public:
	void AddPage(CTreePage *pPage, CTreePage *pOwn = NULL);
	inline CTreePage *GetPage(int nPage) { return (CTreePage *)m_Tab[nPage]; }
	inline int GetPageCount() { return m_Tab.GetSize(); }
	BOOL CreatePage(int nPage);
	void SetActivePage(int nPage);
	void SetModified(BOOL bModified);

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDoInit();
	afx_msg void OnApplyNow();
};
