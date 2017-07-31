#pragma once

#include "ComboBoxHis.h"

/////////////////////////////////////////////////////////////////////////////
// CPassDlg ダイアログ

class CPassDlg : public CDialog
{
	DECLARE_DYNAMIC(CPassDlg)

// コンストラクション
public:
	CPassDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_PASSDLG };

public:
	CProgressCtrl m_TimeLimit;
	CComboBoxHis m_HostWnd;
	CComboBoxHis m_UserWnd;
	CString m_HostAddr;
	CString	m_UserName;
	CString	m_PassName;
	CString	m_Prompt;
	BOOL m_PassEcho;

	CString m_Title;
	int m_Counter;
	int m_MaxTime;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
};
