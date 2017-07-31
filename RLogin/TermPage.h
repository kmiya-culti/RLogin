#pragma once

#include "afxwin.h"
#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CTermPage ダイアログ

class CTermPage : public CTreePage
{
	DECLARE_DYNCREATE(CTermPage)

// コンストラクション
public:
	CTermPage();
	~CTermPage();

// ダイアログ データ
	enum { IDD = IDD_TERMPAGE };

public:
	CProcTab m_ProcTab;
	CListCtrlExt m_OptList;

public:
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
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedEscedit();
	afx_msg void OnNMClickOptlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTipEsclist(NMHDR *pNMHDR, LRESULT *pResult);
};
