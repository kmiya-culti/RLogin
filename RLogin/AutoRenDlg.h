#pragma once


// CAutoRenDlg ダイアログ

class CAutoRenDlg : public CDialog
{
	DECLARE_DYNAMIC(CAutoRenDlg)

public:
	CAutoRenDlg(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~CAutoRenDlg();

// ダイアログ データ
	enum { IDD = IDD_AUTORENDLG };

public:
	int m_Exec;
	CString m_Name[3];
	CString m_NameOK;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnEnUpdateEdit3();
	virtual BOOL OnInitDialog();
};
