#pragma once

/////////////////////////////////////////////////////////////////////////////
// CSerEntPage ダイアログ

class CSerEntPage : public CTreePage
{
	DECLARE_DYNCREATE(CSerEntPage)

// コンストラクション
public:
	CSerEntPage();
	~CSerEntPage();

// ダイアログ データ
	enum { IDD = IDD_SERENTPAGE };

public:
	CString	m_EntryName;
	CString	m_HostName;
	CString	m_PortName;
	CString	m_UserName;
	CString	m_PassName;
	CString	m_TermName;
	int		m_KanjiCode;
	int		m_ProtoType;
	CString m_Memo;
	CString m_Group;
	CString m_DefComPort;
	CString m_IdkeyName;
	int m_ProxyMode;
	BOOL m_SSL_Keep;
	CString m_ProxyHost;
	CString m_ProxyPort;
	CString m_ProxyUser;
	CString m_ProxyPass;
	CString m_ProxyCmd;
	CString m_ProxySsh;
	CString m_ExtEnvStr;
	CString m_BeforeEntry;
	BOOL m_UsePassDlg;
	BOOL m_UseProxyDlg;
	CString m_IconName;
	BOOL m_bOptFixed;
	CString m_OptFixEntry;

public:
	void DoInit();
	void SetEnableWind();

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
	afx_msg void OnComconfig();
	afx_msg void OnKeyfileselect();
	afx_msg void OnProtoType(UINT nID);
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	afx_msg void OnChatEdit();
	afx_msg void OnUpdateOption();
	afx_msg void OnProxySet();
	afx_msg void OnTermcap();
	afx_msg void OnIconfile();
	afx_msg void OnUpdateOptFixed();
};
