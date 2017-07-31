#if !defined(AFX_EDITDLG_H__3257BE5A_3B06_4EF0_A85E_E1784808AA58__INCLUDED_)
#define AFX_EDITDLG_H__3257BE5A_3B06_4EF0_A85E_E1784808AA58__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CEditDlg ダイアログ

class CEditDlg : public CDialog
{
// コンストラクション
public:
	CEditDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CEditDlg)
	enum { IDD = IDD_EDITDLG };
	CString	m_Edit;
	CString	m_Title;
	CString m_WinText;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CEditDlg)
		// メモ: ClassWizard はこの位置にメンバ関数を追加します。
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_EDITDLG_H__3257BE5A_3B06_4EF0_A85E_E1784808AA58__INCLUDED_)
