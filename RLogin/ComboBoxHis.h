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

#define	COMBOHIS_NOUPDATE	000
#define	COMBOHIS_UPDATE		001
#define	COMBOHIS_ADDHIS		002

#define	COMBOHIS_SAVEHIS	(COMBOHIS_UPDATE | COMBOHIS_ADDHIS)

class CComboBoxHis : public CComboBoxExt
{
	DECLARE_DYNAMIC(CComboBoxHis)

public:
	CString m_Section;
	int m_UpdateFlag;

	CComboBoxHis();
	virtual ~CComboBoxHis();

	void LoadHis(LPCTSTR lpszSection);
	void SaveHis(LPCTSTR lpszSection);
	void AddHis(LPCTSTR str);
	void RemoveAll();

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnCbnEditchange();
	afx_msg void OnCbnSelchange();
};
