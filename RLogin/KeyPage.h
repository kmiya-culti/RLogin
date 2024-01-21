#pragma once

#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyPage �_�C�A���O

class CKeyPage : public CTreePage
{
	DECLARE_DYNCREATE(CKeyPage)

// �R���X�g���N�V����
public:
	CKeyPage();
	virtual ~CKeyPage();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_KEYLISTPAGE };

public:
	CListCtrlExt m_List;
	BOOL m_KeyMap[60];
	int m_KeyLayout;

	CKeyNodeTab m_KeyTab;
	DWORD m_MetaKeys[256 / 32];
	CDWordArray m_HideKeys;

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
	afx_msg void OnKeyAsnNew();
	afx_msg void OnKeyAsnEdit();
	afx_msg void OnKeyAsnDel();
	afx_msg void OnEditDups();
	afx_msg void OnEditDelall();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedAllSet();
	afx_msg void OnBnClickedAllClr();
	afx_msg void OnBnClickedMetakey(UINT nID);
	afx_msg void OnDblclkKeyList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	DECLARE_MESSAGE_MAP()
};
