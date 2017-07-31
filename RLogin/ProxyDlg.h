#pragma once

// CProxyDlg ダイアログ

class CProxyDlg : public CDialog
{
	DECLARE_DYNAMIC(CProxyDlg)

// コンストラクション
public:
	CProxyDlg(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~CProxyDlg();

// ダイアログ データ
	enum { IDD = IDD_PROXYDLG };

public:
	CString m_ServerName;
	CString m_PortName;
	CString m_UserName;
	CString m_PassWord;
	int m_ProxyMode;
	int m_SSLMode;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	afx_msg void OnProtoType(UINT nID);
	DECLARE_MESSAGE_MAP()
};
