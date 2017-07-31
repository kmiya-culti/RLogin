#include "afxwin.h"
#if !defined(AFX_TERMPAGE_H__2EBB49D9_A9BC_47C5_BF83_CB0E6952429B__INCLUDED_)
#define AFX_TERMPAGE_H__2EBB49D9_A9BC_47C5_BF83_CB0E6952429B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TermPage.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CTermPage ダイアログ

class CTermPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CTermPage)

// コンストラクション
public:
	class COptDlg *m_pSheet;
	CTermPage();
	~CTermPage();

// ダイアログ データ
	//{{AFX_DATA(CTermPage)
	enum { IDD = IDD_TERMPAGE };
	//}}AFX_DATA
	BOOL m_Check[10];
	CString m_LogFile;
	int m_LogMode;
	int m_LogCode;
	CProcTab m_ProcTab;

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CTermPage)
	public:
	virtual BOOL OnApply();
	virtual void OnReset();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CTermPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg void OnUpdateCheck(UINT nId);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAutologSel();
	afx_msg void OnBnClickedEscedit();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_TERMPAGE_H__2EBB49D9_A9BC_47C5_BF83_CB0E6952429B__INCLUDED_)
