#pragma once

#include "Data.h"
#include "DialogExt.h"

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
	CString m_Title;
	CString m_Status;
	CFont m_StatusFont;
	void *m_pValue;

	void GetStatusText(CString &status);
	void SetStatusText(LPCTSTR status);
	void AddStatusText(LPCTSTR status);

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
public:
	afx_msg void OnClose();
};
