#pragma once

// CSockOptPage ダイアログ

class CSockOptPage : public CTreePage
{
	DECLARE_DYNAMIC(CSockOptPage)

public:
	CSockOptPage();
	virtual ~CSockOptPage();

// ダイアログ データ
	enum { IDD = IDD_SOCKOPTPAGE };

public:
	UINT m_DelayMsec;
	BOOL m_Check[15];
	int m_SelIPver;
	UINT m_SleepTime;
	CString m_GroupCast;
	UINT m_TransmitLimit;

public:
	void DoInit();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

protected:
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	DECLARE_MESSAGE_MAP()
};
