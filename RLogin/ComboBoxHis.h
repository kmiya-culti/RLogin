#pragma once

// CComboBoxExt

class CComboBoxExt : public CComboBox
{
	DECLARE_DYNAMIC(CComboBoxExt)

public:
	CComboBoxExt();
	virtual ~CComboBoxExt();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
};


// CComboBoxHis

class CComboBoxHis : public CComboBoxExt
{
	DECLARE_DYNAMIC(CComboBoxHis)

public:
	CString m_Section;

	CComboBoxHis();
	virtual ~CComboBoxHis();

	void LoadHis(LPCTSTR lpszSection);
	void SaveHis(LPCTSTR lpszSection);
	void AddHis(LPCTSTR str);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
};
