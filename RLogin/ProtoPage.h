#pragma once

/////////////////////////////////////////////////////////////////////////////
// CProtoPage ダイアログ

class CProtoPage : public CTreePropertyPage
{
	DECLARE_DYNCREATE(CProtoPage)

// コンストラクション
public:
	CProtoPage();
	~CProtoPage();

// ダイアログ データ
	enum { IDD = IDD_PROTOPAGE };

public:
	UINT m_KeepAlive;
	BOOL m_Check[18];
	CStringArrayExt m_AlgoTab[12];
	CStringArrayExt m_IdKeyList;
	CStringArrayExt m_PortFwd;
	CString m_XDisplay;

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
	afx_msg void OnSshAlgo();
	afx_msg void OnSshIdkey();
	afx_msg void OnSshPfd();
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	DECLARE_MESSAGE_MAP()
};
