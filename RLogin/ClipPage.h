#pragma once

// CClipPage ダイアログ

class CClipPage : public CTreePage
{
	DECLARE_DYNAMIC(CClipPage)

// コンストラクション
public:
	CClipPage();
	virtual ~CClipPage();

// ダイアログ データ
	enum { IDD = IDD_CLIPBOARDPAGE };

public:
	BOOL m_Check[9];
	CString m_WordStr;
	BOOL m_ClipFlag[2];
	CString m_ShellStr;
	int m_RClick;
	int m_ClipCrLf;
	int m_RtfMode;
	int m_PastEdit;

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
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateChange();
};
