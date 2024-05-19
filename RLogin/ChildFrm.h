//////////////////////////////////////////////////////////////////////
// CChildFrame

#pragma once

#include "TextRam.h"
#include "Data.h"

class CSplitterWndExt : public CSplitterWnd
{
	DECLARE_DYNCREATE(CSplitterWndExt)

public:
	virtual void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rect);

protected:
	DECLARE_MESSAGE_MAP()
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

// ����
public:
	int m_Width, m_Height;
	int m_Cols, m_Lines;
	BOOL m_VScrollFlag;
	BOOL m_bInit;
	BOOL m_bDeletePane;

	void SetScrollBar(BOOL flag);

// �R���g���[���p�����o
protected:
	CSplitterWndExt m_wndSplitter;

// �I�[�o�[���C�h
public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow);
	virtual void OnUpdateFrameMenu(BOOL bActivate, CWnd* pActivateWnd, HMENU hMenuAlt);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// �������ꂽ�A���b�Z�[�W���蓖�Ċ֐�
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnWindowClose();
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
};
