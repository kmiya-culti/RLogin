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
	CString	m_Title;

	BOOL m_bPassword;
	CString m_WinText;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
};
