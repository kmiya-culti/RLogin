#pragma once

#include "afxwin.h"
#include "DialogExt.h"

// CEscParaDlg ダイアログ

class CEscParaDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CEscParaDlg)

// コンストラクション
public:
	CEscParaDlg(CWnd* pParent = NULL);
	virtual ~CEscParaDlg();

// ダイアログ データ
	enum { IDD = IDD_ESCPARADLG };

public:
	CComboBox m_CodeCombo;
	CComboBox m_NameCombo;

	int m_Code;
	CString m_Name;
	class CTextRam *m_pTextRam;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
};
