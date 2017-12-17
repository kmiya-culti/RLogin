#pragma once


// CGhostWnd

class CGhostWnd : public CWnd
{
	DECLARE_DYNAMIC(CGhostWnd)

public:
	class CRLoginView *m_pView;
	class CRLoginDoc *m_pDoc;
	int m_Timer;

	CGhostWnd();
	virtual ~CGhostWnd();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};


