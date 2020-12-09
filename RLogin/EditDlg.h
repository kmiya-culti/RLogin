#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CEditDlg ダイアログ

class CEditDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CEditDlg)

// コンストラクション
public:
	CEditDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_EDITDLG };

public:
	CString	m_Edit;
	CStringLoad	m_Title;

	BOOL m_bPassword;
	CStringLoad m_WinText;

	int m_MinWidth;
	int m_MinHeight;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
};
