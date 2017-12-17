#pragma once

#include "afxcmn.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

// CEnvDlg ダイアログ

class CEnvDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CEnvDlg)

// コンストラクション
public:
	CEnvDlg(CWnd* pParent = NULL);
	virtual ~CEnvDlg();

// ダイアログ データ
	enum { IDD = IDD_ENVDLG };

public:
	CStringIndex m_Env;
	CListCtrlExt m_List;
	BOOL m_ListUpdate;

	void InitList();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnBnClickedInfocap();
	afx_msg void OnHdnItemchangedEnvList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEditNew();
	afx_msg void OnEditUpdate();
	afx_msg void OnEditDelete();
	afx_msg void OnEditDups();
	DECLARE_MESSAGE_MAP()
};
