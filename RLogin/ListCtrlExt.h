#pragma once

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt ウィンドウ

#define	SORTMASK_REVS		0x8000
#define	SORTMASK_ITEM		0x7FFF

#define	EDITFLAG_NONE		0
#define	EDITFLAG_EDIT		1
#define	EDITFLAG_NEXT		2

class CHeaderCtrlExt : public CHeaderCtrl
{
public:
	CHeaderCtrlExt();

	int m_HitItem;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
};

class CListCtrlExt : public CListCtrl
{
// コンストラクション
public:
	CListCtrlExt();
	virtual ~CListCtrlExt();

// オペレーション
public:
	CWordArray m_SortItem;
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
	CSize m_Dpi;
	BOOL m_bSetLVCheck;
#ifdef	USE_DARKMODE
	CHeaderCtrlExt m_HeadCtrl;
#endif

	void SetSelectMarkItem(int item);
	int GetParamItem(int para);
	int GetSelectMarkData();
	int GetSelectMarkCount();
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
	int ItemHitTest(CPoint point);
	void SetSortCols(int subitem);

// オーバーライド
protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillfocusEditBox();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
