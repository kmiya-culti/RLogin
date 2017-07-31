// ChildFrm.h : CChildFrame クラスのインターフェイス
//

#pragma once

#include "TextRam.h"
#include "Data.h"

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

	void SetScrollBar(BOOL flag);

// コントロール用メンバ
protected:
	CSplitterWnd m_wndSplitter;

// オーバーライド
public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow);
	virtual void OnUpdateFrameMenu(BOOL bActive, CWnd* pActiveWnd, HMENU hMenuAlt);

// 生成された、メッセージ割り当て関数
protected:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnWindowClose();
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
