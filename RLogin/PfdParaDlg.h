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
	CPfdData m_Data;
	class CServerEntry *m_pEntry;

	void DisableWnd();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedListen(UINT nID);
};
