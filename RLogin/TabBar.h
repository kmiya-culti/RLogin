//////////////////////////////////////////////////////////////////////
// TabBar.h: CTabBar クラスのインターフェイス

#pragma once

#define	ICONIMG_SIZE	MulDiv(16, SCREEN_DPI_X, DEFAULT_DPI_X)

#define	TBTMID_SETCURSOR		1024
#define	TBTMID_GHOSTWMD			1025

class CTabCtrlBar : public CTabCtrlExt
{
	DECLARE_DYNAMIC(CTabCtrlBar)

public:
	CTabCtrlBar();

public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

public:
	DECLARE_MESSAGE_MAP()
};

class CTabBar : public CControlBarEx
{
	DECLARE_DYNAMIC(CTabBar)

public:
	CTabBar();
	virtual ~CTabBar();

public:
	CTabCtrlBar m_TabCtrl;
	CFont m_TabFont;
	int m_GhostReq;
	int m_GhostItem;
	class CRLoginView *m_pGhostView;
	BOOL m_bNumber;
	CStringBinary m_ImageFile;
	CImageList m_ImageList;
	int m_ImageCount;
	CString m_FontName;
	int m_FontSize;
	UINT_PTR m_SetCurTimer;
	UINT_PTR m_GhostWndTimer;
	int m_TabHeight;
	int m_BoderSize;
	int m_TabLines;
	int m_MinTabSize;
	CString m_ToolTipStr;
	BOOL m_bMultiLine;
	BOOL m_bTrackMode;

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);

	int GetImageIndex(LPCTSTR filename);
	void Add(CWnd *pWnd, int index);
	void Remove(CWnd *pWnd);
	int GetCurSel() { return m_TabCtrl.GetCurSel(); }
	int GetSize() { return m_TabCtrl.GetItemCount(); }
	CWnd *GetAt(int nIndex);
	void GetTitle(int nIndex, CString &title);
	int LineCount(BOOL bHorz);
	void ReSize(BOOL bCallLayout);
	void SetGhostWnd(BOOL sw);
	void NextActive();
	void PrevActive();
	void SelectActive(int idx);
	int GetIndex(CWnd *pWnd);
	int HitPoint(CPoint point);
	void SetTabTitle(BOOL bNumber);
	void ReSetAllTab();
	void FontSizeCheck();
	void MultiLine();

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
