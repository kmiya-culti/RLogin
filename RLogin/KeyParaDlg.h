#if !defined(AFX_KEYPARADLG_H__7F65EA92_9A49_421E_BCB6_54D6E763AE37__INCLUDED_)
#define AFX_KEYPARADLG_H__7F65EA92_9A49_421E_BCB6_54D6E763AE37__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// KeyParaDlg.h : ヘッダー ファイル
//

#include "Data.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyParaDlg ダイアログ

class CKeyParaDlg : public CDialog
{
// コンストラクション
public:
	class CKeyNode *m_pData;

	CKeyParaDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CKeyParaDlg)
	enum { IDD = IDD_KEYPARADLG };
	BOOL	m_WithShift;
	BOOL	m_WithCtrl;
	BOOL	m_WithAlt;
	BOOL	m_WithAppli;
	CString	m_KeyCode;
	CString	m_Maps;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CKeyParaDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CKeyParaDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_KEYPARADLG_H__7F65EA92_9A49_421E_BCB6_54D6E763AE37__INCLUDED_)
