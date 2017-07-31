#pragma once

#include "afxwin.h"
#include "afxcmn.h"

// CChatStatDlg ダイアログ

class CChatStatDlg : public CDialog
{
	DECLARE_DYNAMIC(CChatStatDlg)

// コンストラクション
public:
	CChatStatDlg(CWnd* pParent = NULL);
	virtual ~CChatStatDlg();

// ダイアログ データ
	enum { IDD = IDD_CHATSTAT };

public:
	int m_Counter;
	CEdit m_Status;
	CStatic m_Title;
	CProgressCtrl m_TimeProg;

public:
	void SetStaus(LPCTSTR str);

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
