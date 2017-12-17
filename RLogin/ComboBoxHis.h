#pragma once


// CComboBoxHis

class CComboBoxHis : public CComboBox
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
