#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include "DialogExt.h"
#include "ListCtrlExt.h"

// CToolDlg ダイアログ

class CToolDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CToolDlg)

// コンストラクタ
public:
	CToolDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CToolDlg();

// ダイアログ データ
	enum { IDD = IDD_TOOLDLG };

public:
	CTreeCtrl m_MenuTree;
	CComboBox m_ToolSize;
	CListCtrlExt m_ToolList;

	CMenu m_DefMenu;
	CWordArray m_ToolId;
	CResDataBase m_DataBase;

	CWordArray m_ImageId;
	CImageList m_ImageList;

	int CToolDlg::GetImageIndex(UINT nId, BOOL bUpdate);
	void CToolDlg::AddMenuTree(CMenu *pMenu, HTREEITEM own);

	void InsertList(int pos, WORD id);
	void InitList();
	void LoadList();
	void ListUpdateImage(HTREEITEM root, WORD nId, int idx);
	void UpdateImage(WORD nId);

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
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
};
