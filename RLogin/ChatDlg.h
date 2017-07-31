#pragma once

#include "afxcmn.h"
#include "RegEx.h"
#include "Data.h"
#include "afxwin.h"
#include "DialogExt.h"

// CChatDlg ダイアログ

class CChatDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CChatDlg)

// コンストラクタ
public:
	CChatDlg(CWnd* pParent = NULL);
	virtual ~CChatDlg();

// ダイアログ データ
	enum { IDD = IDD_CHATDLG };

public:
	CTreeCtrl m_NodeTree;
	CString m_RecvStr;
	CString m_SendStr;
	CButton m_UpdNode;
	CButton m_DelNode;
	CStrScript m_Script;
	BOOL m_MakeChat;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnBnClickedNewnode();
	afx_msg void OnBnClickedNextnode();
	afx_msg void OnBnClickedUpdatenode();
	afx_msg void OnBnClickedDelnode();
	afx_msg void OnTvnSelchangedNodetree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnDeleteitemNodetree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickNodetree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
	DECLARE_MESSAGE_MAP()
};
