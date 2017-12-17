#pragma once

#include "afxcmn.h"
#include "DialogExt.h"

// CCertKeyDlg ダイアログ

class CCertKeyDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CCertKeyDlg)

public:
	CCertKeyDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CCertKeyDlg();

// ダイアログ データ
	enum { IDD = IDD_CERTKEYDLG };

public:
	CString m_Digest;
	BOOL m_SaveKeyFlag;
	CString m_HostName;
	CProgressCtrl m_TimeLimit;
	CFont m_DigestFont;

	int m_Counter;
	int m_MaxTime;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
};
