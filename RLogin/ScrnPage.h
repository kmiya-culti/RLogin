#pragma once


// CScrnPage ダイアログ

class CScrnPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CScrnPage)

public:
	class COptDlg *m_pSheet;

	CScrnPage();   // 標準コンストラクタ
	virtual ~CScrnPage();
	virtual BOOL OnApply();
	virtual void OnReset();

// ダイアログ データ
	enum { IDD = IDD_SCRNPAGE };
	int		m_ScrnFont;
	CString	m_FontSize;
	CString	m_ColsMax[2];
	int m_VisualBell;
	int m_RecvCrLf;
	int m_SendCrLf;
	BOOL m_Check[10];
	int m_MouseMode[4];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	DECLARE_MESSAGE_MAP()
};
