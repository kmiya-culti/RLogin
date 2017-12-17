#pragma once

// CHisPage ダイアログ

class CHisPage : public CTreePage
{
	DECLARE_DYNAMIC(CHisPage)

// コンストラクション
public:
	CHisPage();
	virtual ~CHisPage();

// ダイアログ データ
	enum { IDD = IDD_HISPAGE };

public:
	BOOL m_Check[10];
	CString m_HisFile;
	CString	m_HisMax;
	CString m_LogFile;
	CString m_TimeFormat;
	int m_LogMode;
	int m_LogCode;
	CString m_TraceFile;
	CString m_TraceMax;

public:
	void DoInit();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

// インプリメンテーション
protected:
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	afx_msg void OnHisfileSel();
	afx_msg void OnAutoLogSel();
	afx_msg void OnTraceLogSel();
	DECLARE_MESSAGE_MAP()
};
