#pragma once

/////////////////////////////////////////////////////////////////////////////
// CListCtrlMove ウィンドウ

class CListCtrlMove : public CListCtrl
{
// コンストラクション
public:
	CListCtrlMove();
	virtual ~CListCtrlMove();

// オペレーション
public:

// オーバーライド
protected:

// インプリメンテーション
protected:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};
