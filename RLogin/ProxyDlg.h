#pragma once

#include "DialogExt.h"
#include "afxwin.h"

// CProxyDlg ダイアログ

class CProxyDlg : public CDialogExt
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
	int m_ProxyType;
	int m_ProxyMode;
	int m_SSLType;
	int m_SSLMode;
	BOOL m_SSL_Keep;
	BOOL m_UsePassDlg;
	BOOL m_CmdFlag;
	CString m_ProxyCmd;

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
