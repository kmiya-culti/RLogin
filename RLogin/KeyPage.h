#pragma once

#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyPage ダイアログ

class CKeyPage : public CTreePage
{
	DECLARE_DYNCREATE(CKeyPage)

// コンストラクション
public:
	CKeyPage();
	virtual ~CKeyPage();

// ダイアログ データ
	enum { IDD = IDD_KEYLISTPAGE };

public:
	CListCtrlExt m_List;
	BOOL m_KeyMap[60];
	int m_KeyLayout;

	CKeyNodeTab m_KeyTab;
	DWORD m_MetaKeys[256 / 32];

public:
	void InitList();
	void DoInit();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

// インプリメンテーション
protected:
	afx_msg void OnKeyAsnNew();
	afx_msg void OnKeyAsnEdit();
	afx_msg void OnKeyAsnDel();
	afx_msg void OnDblclkKeyList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditDups();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedAllSet();
	afx_msg void OnBnClickedAllClr();
	afx_msg void OnBnClickedMetakey(UINT nID);
	DECLARE_MESSAGE_MAP()
};
