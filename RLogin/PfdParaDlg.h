#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CPfdParaDlg ダイアログ

class CPfdParaDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CPfdParaDlg)

// コンストラクション
public:
	CPfdParaDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_PFDPARADLG };

public:
	int m_EntryNum;
	CStringArrayExt *m_pData;
	class CServerEntry *m_pEntry;
	CString	m_ListenHost;
	CString	m_ListenPort;
	CString	m_ConnectHost;
	CString	m_ConnectPort;
	int m_ListenType;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
};
