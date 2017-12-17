#pragma once

#include "afxwin.h"
#include "DialogExt.h"

// CCsiParaDlg ダイアログ

class CCsiParaDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CCsiParaDlg)

// コンストラクション
public:
	CCsiParaDlg(CWnd* pParent = NULL);
	virtual ~CCsiParaDlg();

// ダイアログ データ
	enum { IDD = IDD_CSIPARADLG };

public:
	CComboBox m_Ext1Combo;
	CComboBox m_Ext2Combo;
	CComboBox m_CodeCombo;
	CComboBox m_NameCombo;

	int m_Type;
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
