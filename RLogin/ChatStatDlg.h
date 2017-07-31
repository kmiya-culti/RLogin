#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CChatStatDlg ダイアログ

class CChatStatDlg : public CDialog
{
	DECLARE_DYNAMIC(CChatStatDlg)

public:
	CChatStatDlg(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~CChatStatDlg();

// ダイアログ データ
	enum { IDD = IDD_CHATSTAT };

protected:
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()

public:
	int m_Counter;
	CEdit m_Status;
	CStatic m_Title;
	CProgressCtrl m_TimeProg;

	void SetStaus(LPCSTR str);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
