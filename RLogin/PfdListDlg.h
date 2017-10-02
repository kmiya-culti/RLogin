#pragma once

#include "ListCtrlExt.h"
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg ダイアログ

class CPfdData : public CObject
{
public:
	CString	m_ListenHost;
	CString	m_ListenPort;
	CString	m_ConnectHost;
	CString	m_ConnectPort;
	int m_ListenType;
	BOOL m_EnableFlag;

	BOOL GetString(LPCTSTR str);
	void SetString(CString &str);
	const CPfdData & operator = (CPfdData &data);

	CPfdData();
	~CPfdData();
};

class CPfdListDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CPfdListDlg)

// コンストラクション
public:
	CPfdListDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_PFDLISTDLG };

public:
	class CServerEntry *m_pEntry;
	CListCtrlExt	m_List;
	CStringArrayExt m_PortFwd;
	CArray<CPfdData, CPfdData &> m_Data;
	BOOL m_X11PortFlag;
	CString m_XDisplay;
	BOOL m_x11AuthFlag;
	CString m_x11AuthName;
	CString m_x11AuthData;

	void InitList();
	void UpdateCheck();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnPfdNew();
	afx_msg void OnPfdEdit();
	afx_msg void OnPfdDel();
	afx_msg void OnDblclkPfdlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditDups();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
};
