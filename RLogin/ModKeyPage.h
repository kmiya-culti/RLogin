#pragma once


// CModKeyPage ダイアログ

class CModKeyPage : public CTreePage
{
	DECLARE_DYNAMIC(CModKeyPage)

public:
	CModKeyPage();
	virtual ~CModKeyPage();

// ダイアログ データ
	enum { IDD = IDD_MODKEYPAGE };

public:
	CString m_KeyList[MODKEY_MAX];
	int m_ModType[MODKEY_MAX];
	BOOL m_bOverShortCut;

public:
	void DoInit();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

protected:
	afx_msg void OnEnChangeKeycode(UINT nID);
	afx_msg void OnCbnSelchangeModcode(UINT nID);
	afx_msg void OnBnClickedCheck1();
	DECLARE_MESSAGE_MAP()
};
