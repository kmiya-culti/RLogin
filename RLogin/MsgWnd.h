#pragma once

//////////////////////////////////////////////////////////////////////////////////
// CMsgWnd フレーム

class CMsgWnd : public CWnd
{
	DECLARE_DYNCREATE(CMsgWnd)

public:
	int m_Timer;
	CString m_Msg;
	int m_Mixval;
	int m_FontSize;
	COLORREF m_BackColor;
	COLORREF m_TextColor;
	BOOL m_bTimerEnable;
	BOOL m_bMultiLine;

	CMsgWnd();
	virtual ~CMsgWnd();

	void Message(LPCTSTR str, CWnd *pWnd = NULL, COLORREF bc = RGB(0, 0, 0));

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};


//////////////////////////////////////////////////////////////////////////////////
// CBtnWnd フレーム

class CBtnWnd : public CWnd
{
	DECLARE_DYNCREATE(CBtnWnd)

public:
	int m_ImageSeq;
	CImageList m_ImageList;
	class CRLoginView *m_pView;

	CBtnWnd();
	virtual ~CBtnWnd();

	void DoButton(CWnd *pWnd, class CTextRam *pTextRam);

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

//////////////////////////////////////////////////////////////////////////////////
// CTrackWnd フレーム

class CTrackWnd : public CWnd
{
	DECLARE_DYNCREATE(CTrackWnd)

public:
	int m_TypeCol;

	CTrackWnd();
	virtual ~CTrackWnd();

	void SetText(LPCTSTR str);

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
};
