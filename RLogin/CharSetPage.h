#pragma once

#include "ListCtrlExt.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CCharSetPage �_�C�A���O

class CCharSetPage : public CTreePage
{
	DECLARE_DYNCREATE(CCharSetPage)

// �R���X�g���N�V����
public:
	CCharSetPage();
	~CCharSetPage();

// �_�C�A���O �f�[�^
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
	CString m_DefFontName;
	CStatic m_FontSample;
	int		m_ListIndex;
	CStringBinary m_FontSet;
	BOOL m_bDisableCharSet;

public:
	class CFontTab m_FontTab;
	WORD m_BankTab[5][4];
	CString m_SendCharSet[5];
	CString m_DefFontTab[16];

public:
	void InitList();
	void DoInit();

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
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
	afx_msg void OnEditDelall();
	afx_msg void OnUpdateFontName();
	afx_msg void OnCbnSelchangeFontName();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnLvnItemchangedFontlist(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnBnClickedCheck1();
};
