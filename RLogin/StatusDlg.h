#pragma once

#include "Data.h"
#include "DialogExt.h"
#include "afxwin.h"

// CStatusDlg ダイアログ

class CStatusDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CStatusDlg)

// コンストラクション
public:
	CStatusDlg(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~CStatusDlg();

// ダイアログ データ
	enum { IDD = IDD_STATUS_DLG };

public:
	CEdit m_StatusWnd;
	CString m_Title;
	CString m_StatusText;
	CFont m_StatusFont;
	int m_OwnerType;
	void *m_pValue;
	BOOL m_bEdit;

	LPCTSTR GetStatusText();
	void SetStatusText(LPCTSTR status);
	void AddStatusText(LPCTSTR status);

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnCancel();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnStatusSave();
	afx_msg void OnStatusClose();
	afx_msg void OnStatusClear();
	afx_msg void OnStatusCopy();
	afx_msg void OnFilePrintSetup();
	afx_msg void OnFilePrint();
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDropFiles(HDROP hDropInfo);
};
