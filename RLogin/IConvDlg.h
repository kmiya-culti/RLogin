#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CIConvDlg ダイアログ

class CIConvDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CIConvDlg)

// コンストラクション
public:
	CIConvDlg(CWnd* pParent = NULL);

// ダイアログ データ
	enum { IDD = IDD_ICONVDLG };

public:
	CString	m_CharSet[4];

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
};
