#pragma once

#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage ダイアログ

class CCharSetPage : public CTreePropertyPage
{
	DECLARE_DYNCREATE(CCharSetPage)

// コンストラクション
public:
	CCharSetPage();
	~CCharSetPage();

// ダイアログ データ
	enum { IDD = IDD_CHARSETPAGE };

public:
	CListCtrlExt	m_List;
	int		m_KanjiCode;
	int		m_CharBankGL;
	int		m_CharBankGR;
	CString	m_CharBank1;
	CString	m_CharBank2;
	CString	m_CharBank3;
	CString	m_CharBank4;
	int		m_AltFont;

public:
	class CFontTab m_FontTab;
	WORD m_BankTab[5][4];
	CString m_SendCharSet[5];

public:
	void InitList();
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
	afx_msg void OnFontListNew();
	afx_msg void OnFontListEdit();
	afx_msg void OnFontListDel();
	afx_msg void OnIconvSet();
	afx_msg void OnDblclkFontlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCharSet(UINT nId);
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnEditDups();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	afx_msg void OnCbnSelchangeFontnum();
	DECLARE_MESSAGE_MAP()
};
