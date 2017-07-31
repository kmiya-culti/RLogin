#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CChModDlg ダイアログ

class CChModDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CChModDlg)

// コンストラクション
public:
	CChModDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_CHMODDLG };

public:
	int m_Attr;
	BOOL	m_Mode[9];

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
};
