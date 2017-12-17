#pragma once

#include "DialogExt.h"

// CScriptPage ダイアログ

class CScriptPage : public CTreePage
{
	DECLARE_DYNAMIC(CScriptPage)

// コンストラクション
public:
	CScriptPage();
	virtual ~CScriptPage();

// ダイアログ データ
	enum { IDD = IDD_SCRIPTPAGE };

public:
	BOOL m_Check[10];
	CString m_ScriptFile;
	CString m_ScriptStr;

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
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	afx_msg void OnBnClickedScriptSel();
	DECLARE_MESSAGE_MAP()
};
