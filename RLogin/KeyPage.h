#if !defined(AFX_KEYPAGE_H__F66CCD3B_B988_43DE_9750_57294D881F0E__INCLUDED_)
#define AFX_KEYPAGE_H__F66CCD3B_B988_43DE_9750_57294D881F0E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// KeyPage.h : ヘッダー ファイル
//

#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyPage ダイアログ

class CKeyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CKeyPage)

// コンストラクション
public:
	class COptDlg *m_pSheet;
	CKeyNodeTab m_KeyTab;
	void InitList();
	CKeyPage();
	~CKeyPage();

// ダイアログ データ
	//{{AFX_DATA(CKeyPage)
	enum { IDD = IDD_KEYLISTPAGE };
	CListCtrlExt	m_List;
	//}}AFX_DATA
	BOOL m_Check[4];
	int m_DropMode;
	CString m_DropCmd[8];
	CString m_CmdWork;
	CString m_WordStr;

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CKeyPage)
	public:
	virtual BOOL OnApply();
	virtual void OnReset();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CKeyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnKeyAsnNew();
	afx_msg void OnKeyAsnEdit();
	afx_msg void OnKeyAsnDel();
	afx_msg void OnDblclkKeyList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnEditDups();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUpdateChangeDropMode();
	afx_msg void OnCbnSelchangeDropMode();
	afx_msg void OnEnChangeEdit3();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_KEYPAGE_H__F66CCD3B_B988_43DE_9750_57294D881F0E__INCLUDED_)
