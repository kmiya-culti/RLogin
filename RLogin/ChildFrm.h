// ChildFrm.h : CChildFrame クラスのインターフェイス
//

#pragma once

#include "TextRam.h"
#include "Data.h"

class CSplitterWndExt : public CSplitterWnd
{
	DECLARE_DYNCREATE(CSplitterWndExt)

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};

class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CChildFrame)

public:
	CChildFrame();
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// 実装
public:
	int m_Width, m_Height;
	int m_Cols, m_Lines;
	BOOL m_VScrollFlag;
	BOOL m_bInit;
	BOOL m_bDeletePane;

	void SetScrollBar(BOOL flag);

// コントロール用メンバ
protected:
	CSplitterWndExt m_wndSplitter;

// オーバーライド
public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow);
	virtual void OnUpdateFrameMenu(BOOL bActive, CWnd* pActiveWnd, HMENU hMenuAlt);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// 生成された、メッセージ割り当て関数
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnWindowClose();
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
};
