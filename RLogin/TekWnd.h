#pragma once

#include "TextRam.h"
#include "GrapWnd.h"

// CTekWnd

class CTekWnd : public CFrameWndExt
{
	DECLARE_DYNAMIC(CTekWnd)

public:
	CWnd *m_pWnd;
	class CTextRam *m_pTextRam;

	void GinMouse(int ch, int x, int y);
	BOOL SaveDxf(LPCTSTR file);
	BOOL SaveTek(LPCTSTR file);
	BOOL SaveSvg(LPCTSTR file);

	CTekWnd(class CTextRam *pTextRam, CWnd *pWnd);
	virtual ~CTekWnd();

protected:
	virtual void PostNcDestroy();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnTekClose();
	afx_msg void OnTekSave();
	afx_msg void OnTekClear();
	afx_msg void OnTekMode();
	afx_msg void OnUpdateTekMode(CCmdUI *pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};


