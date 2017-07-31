#pragma once

#include "afxwin.h"
#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CTermPage ダイアログ

class CTermPage : public CTreePropertyPage
{
	DECLARE_DYNCREATE(CTermPage)

// コンストラクション
public:
	CTermPage();
	~CTermPage();

// ダイアログ データ
	enum { IDD = IDD_TERMPAGE };

public:
	BOOL m_Check[10];
	CProcTab m_ProcTab;
	CListCtrlExt m_List;

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
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnBnClickedEscedit();
	afx_msg void OnNMClickEsclist(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};
