#if !defined(AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_)
#define AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlgoDlg.h : ヘッダー ファイル
//
#include "ListCtrlMove.h"
#include "Data.h"

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg ダイアログ

class CAlgoDlg : public CDialog
{
// コンストラクション
public:
	CParamTab *m_pData;
	
	CAlgoDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CAlgoDlg)
	enum { IDD = IDD_ALGOPARADLG };
	//}}AFX_DATA
	CListCtrlMove m_List[12];
	BOOL m_EncShuffle;
	BOOL m_MacShuffle;

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CAlgoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CAlgoDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_)
