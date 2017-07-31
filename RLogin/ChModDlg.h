#if !defined(AFX_CHMODDLG_H__BAB8AA2E_642D_432D_8AA9_577E07A2C736__INCLUDED_)
#define AFX_CHMODDLG_H__BAB8AA2E_642D_432D_8AA9_577E07A2C736__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChModDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CChModDlg ダイアログ

class CChModDlg : public CDialog
{
// コンストラクション
public:
	int m_Attr;
	CChModDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CChModDlg)
	enum { IDD = IDD_CHMODDLG };
	//}}AFX_DATA
	BOOL	m_Mode[9];


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CChModDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CChModDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_CHMODDLG_H__BAB8AA2E_642D_432D_8AA9_577E07A2C736__INCLUDED_)
