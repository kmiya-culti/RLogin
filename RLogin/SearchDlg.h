#pragma once

#include "afxcmn.h"
#include "ComboBoxHis.h"
#include "afxwin.h"
#include "DialogExt.h"

// CSearchDlg ダイアログ

class CSearchDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CSearchDlg)

// コンストラクション
public:
	CSearchDlg(CWnd* pParent = NULL);
	virtual ~CSearchDlg();

// ダイアログ データ
	enum { IDD = IDD_SEARCHDLG };

public:
	CComboBoxHis m_SearchCombo;
	CString m_SearchStr;
	CProgressCtrl m_Prog;
	BOOL m_bRegExSerch;
	UINT_PTR m_TimerId;

// クラスデータ
public:
	class CRLoginDoc *m_pDoc;
	int m_Pos;
	int m_Max;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
