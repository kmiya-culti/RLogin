#if !defined(AFX_LISTCTRLMOVE_H__4197EC8E_98EE_4FB3_8322_AE8CFB79AB1F__INCLUDED_)
#define AFX_LISTCTRLMOVE_H__4197EC8E_98EE_4FB3_8322_AE8CFB79AB1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListCtrlMove.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CListCtrlMove ウィンドウ

class CListCtrlMove : public CListCtrl
{
// コンストラクション
public:
	CListCtrlMove();

// アトリビュート
public:

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CListCtrlMove)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CListCtrlMove();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CListCtrlMove)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_LISTCTRLMOVE_H__4197EC8E_98EE_4FB3_8322_AE8CFB79AB1F__INCLUDED_)
