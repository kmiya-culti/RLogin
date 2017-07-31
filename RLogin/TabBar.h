#pragma once

#define	ICONIMG_SIZE	16

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
	BOOL m_bNumber;
	CStringBinary m_ImageFile;
	CImageList m_ImageList;
	int m_ImageCount;

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);

	int GetImageIndex(LPCTSTR filename);
	void Add(CWnd *pWnd);
	void Remove(CWnd *pWnd);
	int GetCurSel() { return m_TabCtrl.GetCurSel(); }
	int GetSize() { return m_TabCtrl.GetItemCount(); }
	CWnd *GetAt(int nIndex);
	void GetTitle(int nIndex, CString &title);
	void ReSize();
	void SetGhostWnd(BOOL sw);
	void NextActive();
	void PrevActive();
	void SelectActive(int idx);
	int GetIndex(CWnd *pWnd);
	int HitPoint(CPoint point);
	void SetTabTitle(BOOL bNumber);

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
