#pragma once

#include "afxwin.h"
#include "DialogExt.h"

// CAnyPastDlg ダイアログ

class CAnyPastDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CAnyPastDlg)

public:
	CAnyPastDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CAnyPastDlg();

// ダイアログ データ
	enum { IDD = IDD_ANYPASTDIG };

public:
	CStatic m_IconBox;
	CEdit m_EditWnd;
	int m_CtrlCode[3];
	BOOL m_bUpdateText;
	BOOL m_bDelayPast;
	BOOL m_NoCheck;

	CString m_EditText;
	BOOL m_bUpdateEnable;
	int m_MinWidth;
	int m_MinHeight;

public:
	void CtrlCount();
	void SaveWindowRect();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateEdit();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnCtrlConv(UINT nID);
	afx_msg void OnShellesc();
};
