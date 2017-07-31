#pragma once

//////////////////////////////////////////////////////////////////////
// TabBar.h: CTabBar クラスのインターフェイス

class CTabBar : public CControlBar  
{
	DECLARE_DYNAMIC(CTabBar)

public:
	CTabBar();
	virtual ~CTabBar();

public:
	CTabCtrl m_TabCtrl;
	int m_GhostReq;
	int m_GhostItem;
	class CRLoginView *m_pGhostView;

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);

	void Add(CWnd *pWnd);
	void Remove(CWnd *pWnd);
	int GetCurSel() { return m_TabCtrl.GetCurSel(); }
	int GetSize() { return m_TabCtrl.GetItemCount(); }
	CWnd *GetAt(int nIndex);
	void ReSize();
	void SetGhostWnd(BOOL sw);

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
};
