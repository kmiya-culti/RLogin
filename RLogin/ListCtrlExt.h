#pragma once

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt ウィンドウ

class CListCtrlExt : public CListCtrl
{
// コンストラクション
public:
	CListCtrlExt();
	virtual ~CListCtrlExt();

// オペレーション
public:
	int m_SortSubItem;
	int m_SortReverse;
	int m_SortDupItem;
	CMenuLoad m_PopUpMenu;
	int m_SubMenuPos;

	int m_EditSubItem;
	int m_EditFlag;
	int m_EditItem;
	int m_EditNum;
	CEdit m_EditWnd;
	CString m_EditOld;
	BOOL m_bSort;
	BOOL m_bMove;

	int GetParamItem(int para);
	int GetSelectMarkData();
	void DoSortItem();
	void InitColumn(LPCTSTR lpszSection, const LV_COLUMN *lpColumn, int nMax);
	void SaveColumn(LPCTSTR lpszSection);
	void SetLVCheck(WPARAM ItemIndex, BOOL bCheck);
	BOOL GetLVCheck(WPARAM ItemIndex);
	void SetPopUpMenu(UINT nIDResource, int Pos);
	void OpenEditBox(int item, int num, int fmt, CRect &rect);
	void EditItem(int item, int num);

	void SwapItemText(int src, int dis);
	void MoveItemText(int src, int dis);

// オーバーライド
protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillfocusEditBox();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
};
