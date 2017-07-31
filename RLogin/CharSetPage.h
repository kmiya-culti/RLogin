#if !defined(AFX_CHARSETPAGE_H__D11C4465_3725_4437_A251_DA9869F56412__INCLUDED_)
#define AFX_CHARSETPAGE_H__D11C4465_3725_4437_A251_DA9869F56412__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CharSetPage.h : ヘッダー ファイル
//

#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage ダイアログ

class CCharSetPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CCharSetPage)

// コンストラクション
public:
	class COptDlg *m_pSheet;
	class CTextRam *m_pData;
	class CFontTab m_FontTab;
	void InitList();
	CCharSetPage();
	~CCharSetPage();

// ダイアログ データ
	//{{AFX_DATA(CCharSetPage)
	enum { IDD = IDD_CHARSETPAGE };
	CListCtrlExt	m_List;
	int		m_KanjiCode;
	int		m_CharBankGL;
	int		m_CharBankGR;
	CString	m_CharBank1;
	CString	m_CharBank2;
	CString	m_CharBank3;
	CString	m_CharBank4;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CCharSetPage)
	public:
	virtual BOOL OnApply();
	virtual void OnReset();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CCharSetPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnFontListNew();
	afx_msg void OnFontListEdit();
	afx_msg void OnFontListDel();
	afx_msg void OnIconvSet();
	afx_msg void OnDblclkFontlist(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg void OnCharSet(UINT nId);
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnEditDups();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_CHARSETPAGE_H__D11C4465_3725_4437_A251_DA9869F56412__INCLUDED_)
