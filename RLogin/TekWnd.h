#pragma once

#include "TextRam.h"

// CTekWnd

class CTekWnd : public CFrameWnd
{
	DECLARE_DYNAMIC(CTekWnd)

public:
	CWnd *m_pWnd;
	class CTextRam *m_pTextRam;

	void GinMouse(int ch, int x, int y);
	BOOL SaveDxf(LPCSTR file);
	BOOL SaveTek(LPCSTR file);

	CTekWnd(class CTextRam *pTextRam, CWnd *pWnd);
	virtual ~CTekWnd();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
protected:
	virtual void PostNcDestroy();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
	afx_msg void OnDestroy();
	afx_msg void OnTekClose();
	afx_msg void OnTekSave();
	afx_msg void OnTekClear();
	afx_msg void OnTekMode();
	afx_msg void OnUpdateTekMode(CCmdUI *pCmdUI);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};


