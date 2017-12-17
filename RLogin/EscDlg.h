#pragma once

#include "afxcmn.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

// CEscDlg ダイアログ

class CEscDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CEscDlg)

// コンストラクション
public:
	CEscDlg(CWnd* pParent = NULL);
	virtual ~CEscDlg();

// ダイアログ データ
	enum { IDD = IDD_ESCDLG };

public:
	CListCtrlExt m_EscList;
	CListCtrlExt m_CsiList;
	CListCtrlExt m_DcsList;
	CListCtrlExt m_ParaList;

public:
	CTextRam *m_pTextRam;
	CProcTab *m_pProcTab;
	LPCTSTR *m_DefEsc;
	LPCTSTR *m_DefCsi;
	LPCTSTR *m_DefDcs;
	CString *m_NewEsc;
	CString *m_NewCsi;
	CString *m_NewDcs;
	int m_TermPara[5];

	void InitEscList();
	void InitCsiList();
	void InitDcsList();
	void InitParaList();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnNMDblclkEsclist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkCsilist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkDcslist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedReset();
	DECLARE_MESSAGE_MAP()
};
