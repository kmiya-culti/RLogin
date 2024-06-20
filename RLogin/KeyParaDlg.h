#pragma once

#include "Data.h"
#include "DialogExt.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyParaDlg ダイアログ

class CKeyParaDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CKeyParaDlg)

// コンストラクション
public:
	CKeyParaDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_KEYPARADLG };

public:
	BOOL	m_WithShift;
	BOOL	m_WithCtrl;
	BOOL	m_WithAlt;
	BOOL	m_WithAppli;
	BOOL	m_WithCkm;
	BOOL	m_WithVt100;
	BOOL	m_WithVt52;
	BOOL	m_WithNum;
	BOOL	m_WithScr;
	BOOL	m_WithCap;
	CString	m_KeyCode;
	CString	m_Maps;
	CButton m_EditCtrl;
	CButton m_EditKey;
	HWND m_hMapsEdit;
	BOOL m_bCtrlMode;
	HWND m_hKeyEdit;
	BOOL m_bKeyInsert;

	class CKeyNode *m_pData;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// インプリメンテーション
protected:
	afx_msg void OnBnClickedMenubtn();
	afx_msg void OnBnClickedEditctrl();
	afx_msg void OnBnClickedEditKey();
	DECLARE_MESSAGE_MAP()
};
