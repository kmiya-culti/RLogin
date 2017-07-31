#pragma once

/////////////////////////////////////////////////////////////////////////////
// CEditDlg ダイアログ

class CEditDlg : public CDialog
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

	CString m_WinText;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
};
