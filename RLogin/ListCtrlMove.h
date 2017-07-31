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
	DECLARE_MESSAGE_MAP()
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
};
