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
	BOOL m_SSL_Keep;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnProtoType(UINT nID);
};
