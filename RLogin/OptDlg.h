#pragma once

#include "afxcmn.h"
#include "afxwin.h"
#include <afxtempl.h>
#include "DialogExt.h"
#include "MsgWnd.h"

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
	LPCTSTR m_UrlOpt;

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
#include "ExtOptPage.h"
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
#include "BackPage.h"

/////////////////////////////////////////////////////////////////////////////
// COptDlg

#define	UMOD_ENTRY		000001
#define	UMOD_KEYTAB		000002
#define	UMOD_PARAMTAB	000004
#define	UMOD_TEXTRAM	000010
#define	UMOD_ANSIOPT	000020
#define	UMOD_MODKEY		000040
#define	UMOD_COLTAB		000100
#define	UMOD_BANKTAB	000200
#define	UMOD_DEFATT		000400
#define	UMOD_RESIZE		001000
#define	UMOD_CARET		002000
#define	UMOD_TABCOLOR	004000
#define	UMOD_CODEFLAG	010000

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
	CTreeCtrlExt m_Tree;
	CStatic m_Frame;
	CButton m_Button[4];

	CSerEntPage *m_pSerEntPage;
	CKeyPage *m_pKeyPage;
	CColParaDlg *m_pColorPage;
	CCharSetPage *m_pCharSetPage;
	CSockOptPage *m_pSockOptPage;
	CProtoPage *m_pProtoPage;
	CTermPage *m_pTermPage;
	CExtOptPage *m_pExtOptPage;
	CScrnPage *m_pScrnPage;
	CHisPage *m_pHisPage;
	CClipPage *m_pClipPage;
	CMousePage *m_pMousePage;
	CScriptPage *m_pScriptPage;
	CModKeyPage *m_pModKeyPage;
	CBackPage *m_pBackPage;

	CServerEntry *m_pEntry;
	CTextRam *m_pTextRam;
	CKeyNodeTab *m_pKeyTab;
	CKeyMacTab *m_pKeyMac;
	CParamTab *m_pParamTab;
	class CRLoginDoc *m_pDocument;
	int m_ModFlag;
	BOOL m_bModified;
	CString m_Title;
	CMsgWnd m_MsgWnd;
	BOOL m_bOptFixed;

	struct _OptTab {
		DWORD	dwFlags;
	} m_psh;
	CPtrArray m_Tab;
	int m_ActivePage;

// クラスファンクション
public:
	void AddPage(CTreePage *pPage, CTreePage *pOwn = NULL);
	inline CTreePage *GetPage(int nPage) { return (CTreePage *)m_Tab[nPage]; }
	inline int GetPageCount() { return (int)m_Tab.GetSize(); }
	BOOL CreatePage(int nPage);
	void SetActivePage(int nPage);
	void SetModified(BOOL bModified);
	void PageReSize();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDoInit();
	afx_msg void OnApplyNow();
	afx_msg void OnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHelpbtn();
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
};
