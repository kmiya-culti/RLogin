#pragma once

/////////////////////////////////////////////////////////////////////////////
// CProtoPage ダイアログ

class CProtoPage : public CTreePage
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
	UINT m_TelKeepAlive;
	BOOL m_Check[18];
	CStringArrayExt m_AlgoTab[12];
	CStringArrayExt m_IdKeyList;
	CStringArrayExt m_PortFwd;
	CString m_XDisplay;
	BOOL  m_x11AuthFlag;
	CString m_x11AuthName;
	CString m_x11AuthData;
	int m_RsaExt;
	CString m_VerIdent;
	int m_StdIoBufSize;

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
	afx_msg void OnSshAlgo();
	afx_msg void OnSshIdkey();
	afx_msg void OnSshPfd();
	afx_msg void OnSshTtyMode();
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
};
