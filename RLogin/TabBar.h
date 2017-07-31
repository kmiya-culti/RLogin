// TabBar.h: CTabBar クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TABBAR_H__36393842_91F7_4016_B288_B47BE78E2A20__INCLUDED_)
#define AFX_TABBAR_H__36393842_91F7_4016_B288_B47BE78E2A20__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CTabBar : public CControlBar  
{
	DECLARE_DYNAMIC(CTabBar)

public:
	CTabCtrl m_TabCtrl;

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	void Add(CWnd *pWnd);
	void Remove(CWnd *pWnd);
	int GetCurSel() { return m_TabCtrl.GetCurSel(); }
	int GetSize() { return m_TabCtrl.GetItemCount(); }
	CWnd *GetAt(int nIndex);
	void ReSize();

	CTabBar();
	virtual ~CTabBar();

protected:
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTabBar)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

protected:
	// Generated message map functions
	//{{AFX_MSG(CTabBar)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_TABBAR_H__36393842_91F7_4016_B288_B47BE78E2A20__INCLUDED_)
