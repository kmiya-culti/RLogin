#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include "DialogExt.h"
#include "ListCtrlExt.h"

// CToolDlg �_�C�A���O

class CToolDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CToolDlg)

// �R���X�g���N�^
public:
	CToolDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[
	virtual ~CToolDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_TOOLDLG };

public:
	CTreeCtrlExt m_MenuTree;
	CComboBox m_ToolSize;
	CListCtrlExt m_ToolList;
	int m_BarIdx;

	CMenu m_DefMenu;
	CWordArray m_ToolId[2];
	CResDataBase m_DataBase;

	CWordArray m_ImageId;
	CImageList m_ImageList;

	int CToolDlg::GetImageIndex(UINT nId, BOOL bUpdate);
	void CToolDlg::AddMenuTree(CMenu *pMenu, HTREEITEM own);

	void InsertList(int pos, WORD id);
	void InitList(BOOL bMenuInit);
	void LoadList();
	void ListUpdateImage(HTREEITEM root, WORD nId, int idx);
	void UpdateImage(WORD nId);

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnTvnBegindragMenuTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEditNew();
	afx_msg void OnUpdateEditNew(CCmdUI *pCmdUI);
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnEditDelall();
	afx_msg void OnIconload();
	afx_msg void OnUpdateIconload(CCmdUI *pCmdUI);
	afx_msg void OnToolBatSel(UINT nID);
};
