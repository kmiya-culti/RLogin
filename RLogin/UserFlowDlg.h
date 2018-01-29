#pragma once

#include "DialogExt.h"

// CUserFlowDlg ダイアログ

class CUserFlowDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CUserFlowDlg)

public:
	CUserFlowDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CUserFlowDlg();

// ダイアログ データ
	enum { IDD = IDD_USERFLOWDLG };

	DCB *m_pDCB;
	int m_Value[6];
	BOOL m_Check[6];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdate(UINT nID);
};
