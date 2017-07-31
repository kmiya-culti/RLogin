#pragma once

// CAutoRenDlg ダイアログ

class CAutoRenDlg : public CDialog
{
	DECLARE_DYNAMIC(CAutoRenDlg)

// コンストラクタ
public:
	CAutoRenDlg(CWnd* pParent = NULL);
	virtual ~CAutoRenDlg();

// ダイアログ データ
	enum { IDD = IDD_AUTORENDLG };

public:
	int m_Exec;
	CString m_Name[3];
	CString m_NameOK;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnEnUpdateEdit3();
	DECLARE_MESSAGE_MAP()
};
