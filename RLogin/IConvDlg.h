#if !defined(AFX_ICONVDLG_H__D1B96FC1_A63B_43A5_9294_B35B09DA31EC__INCLUDED_)
#define AFX_ICONVDLG_H__D1B96FC1_A63B_43A5_9294_B35B09DA31EC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IConvDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CIConvDlg ダイアログ

class CIConvDlg : public CDialog
{
// コンストラクション
public:
	CIConvDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CIConvDlg)
	enum { IDD = IDD_ICONVDLG };
	//}}AFX_DATA
	CString	m_CharSet[4];


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CIConvDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CIConvDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_ICONVDLG_H__D1B96FC1_A63B_43A5_9294_B35B09DA31EC__INCLUDED_)
