#if !defined(AFX_PASSDLG_H__C6105632_C787_472B_8B8A_70302DB393EE__INCLUDED_)
#define AFX_PASSDLG_H__C6105632_C787_472B_8B8A_70302DB393EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PassDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CPassDlg ダイアログ

class CPassDlg : public CDialog
{
// コンストラクション
public:
	int m_Counter;
	int m_MaxTime;
	CString m_Title;
	CPassDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CPassDlg)
	enum { IDD = IDD_PASSDLG };
	CProgressCtrl	m_TimeLimit;
	CString m_HostAddr;
	CString	m_UserName;
	CString	m_PassName;
	CString	m_Prompt;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CPassDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CPassDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_PASSDLG_H__C6105632_C787_472B_8B8A_70302DB393EE__INCLUDED_)
