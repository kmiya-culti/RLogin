#if !defined(AFX_SERENTPAGE_H__2E276794_7C77_47D8_A0C3_64C182B45077__INCLUDED_)
#define AFX_SERENTPAGE_H__2E276794_7C77_47D8_A0C3_64C182B45077__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SerEntPage.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CSerEntPage ダイアログ

class CSerEntPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSerEntPage)

// コンストラクション
public:
	class CServerEntryTab *m_pTab;
	class COptDlg *m_pSheet;
	CString m_DefComPort;
	CString m_IdkeyName;
	int m_ProxyMode;
	CString m_ProxyHost;
	CString m_ProxyPort;
	CString m_ProxyUser;
	CString m_ProxyPass;
	CString m_ExtEnvStr;

	void SetEnableWind();
	CSerEntPage();
	~CSerEntPage();

// ダイアログ データ
	//{{AFX_DATA(CSerEntPage)
	enum { IDD = IDD_SERENTPAGE };
	CString	m_EntryName;
	CString	m_HostName;
	CString	m_PortName;
	CString	m_UserName;
	CString	m_PassName;
	CString	m_TermName;
	int		m_KanjiCode;
	int		m_ProtoType;
	CString m_Memo;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CSerEntPage)
	public:
	virtual BOOL OnApply();
	virtual void OnReset();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CSerEntPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnComconfig();
	afx_msg void OnKeyfileselect();
	//}}AFX_MSG
	afx_msg void OnProtoType(UINT nID);
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnChatEdit();
	afx_msg void OnProxySet();
	afx_msg void OnBnClickedTermcap();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_SERENTPAGE_H__2E276794_7C77_47D8_A0C3_64C182B45077__INCLUDED_)
