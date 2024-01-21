#pragma once

class CToolBarEx : public CToolBar
{
	DECLARE_DYNAMIC(CToolBarEx)

public:
	CToolBarEx();
	virtual ~CToolBarEx();

public:
	typedef struct _DropItem {
		int		index;
		int     item;
		int     count;
		UINT    *plist;
	} DropItem;

	CArray<DropItem, DropItem &> m_DropItem;
	CWordArray m_ItemImage;
	BOOL m_bDarkMode;

public:
	void RemoveAll();
	void InitDropDown();
	void SetDropItem(int index, int item, int count, UINT *plist);

	int GetItemImage(int item);
	int AddItemImage(int item);
	void CreateItemImage(int width, int height);

	virtual void DrawBorders(CDC* pDC, CRect& rect);
	virtual BOOL DrawThemedGripper(CDC* pDC, const CRect& rect, BOOL fCentered);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

class CStatusBarEx : public CStatusBar
{
	DECLARE_DYNAMIC(CStatusBarEx)

public:
	CStatusBarEx();
	
public:
	BOOL m_bDarkMode;

	BOOL SetIndicators(const UINT* lpIDArray, int nIDCount);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
