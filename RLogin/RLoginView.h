// RLoginView.h : CRLoginView クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RLOGINVIEW_H__03B57615_4E99_4D63_9DF0_4B7D3D47E193__INCLUDED_)
#define AFX_RLOGINVIEW_H__03B57615_4E99_4D63_9DF0_4B7D3D47E193__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Data.h"
#include "TextRam.h"
#include "MsgWnd.h"

#define	FGCARET_CREATE	001
#define	FGCARET_ONOFF	002
#define	FGCARET_FOCUS	004
#define	FGCARET_REDRAW	010

class CRLoginView : public CView
{
	DECLARE_DYNCREATE(CRLoginView)

protected: // シリアライズ機能のみから作成します。
	CRLoginView();

public:
	virtual ~CRLoginView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// アトリビュート
public:
	CRLoginDoc* GetDocument();

// オペレーション
public:
	int m_Width, m_Height;
	int m_Cols, m_Lines;
	int m_HisMin;
	int m_HisOfs;
	int m_DispCaret;
	int m_CaretX, m_CaretY;
	int m_CharWidth, m_CharHeight;
	CBmpFile m_BmpFile;
	CBitmap *m_pBitmap;
	BOOL m_ActiveFlag;
	BOOL m_VisualBellFlag;
	int m_BlinkFlag;
	BOOL m_MouseEventFlag;
	volatile BOOL m_BroadCast;
	int m_WheelDelta;
	BOOL m_WheelTimer;
	BOOL m_PastNoCheck;

	int m_ClipFlag;
	int m_ClipStaPos, m_ClipEndPos;
	int m_ClipStaOld, m_ClipEndOld;
	UINT m_ClipTimer;
	CPoint m_ClipSavePoint;
	UINT m_ClipKeyFlags;

	inline BOOL IsClipRectMode() { return (m_ClipKeyFlags & MK_CONTROL); }
	inline BOOL IsClipLineMode() { return (m_ClipKeyFlags & (MK_SHIFT | 0x1000)); }
	int GetClipboad(CBuffer *bp);
	int SetClipboad(CBuffer *bp);

	BOOL m_KeyMacFlag;
	CBuffer m_KeyMacBuf;

	class CGhostWnd *m_pGhost;
	class CMsgWnd m_MsgWnd;

	BOOL m_GoziView;
	int m_GoziStyle;
	int m_GoziCount;
	CPoint m_GoziPos;

#ifdef	USE_DIRECTWRITE
	ID2D1HwndRenderTarget *m_pRenderTarget;
#endif

	void InvalidateTextRect(CRect rect);
	void CalcPosRect(CRect &rect);
	void CalcTextRect(CRect &rect);
	void CalcGrapPoint(CPoint po, int *x, int *y);
	void SetFrameRect(int cx, int cy);
	void ImmSetPos(int x, int y);
	int ImmOpenCtrl(int sw);
	void SetCaret();
	void SendBroadCastMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void SendBroadCast(CBuffer &buf);
	void SendBuffer(CBuffer &buf, BOOL macflag = FALSE);
	void SetGhostWnd(BOOL sw);
	BOOL ModifyKeys(UINT nChar, int nStat);

	inline int CalcGrapX(int x) { CRLoginDoc *pDoc = GetDocument(); return (m_Width  * x / m_Cols  + pDoc->m_TextRam.m_ScrnOffset.left); }
	inline int CalcGrapY(int y) { return (m_Height * y / m_Lines); }

	inline class CChildFrame *GetFrameWnd() { return (class CChildFrame *)GetParentFrame(); }
	inline class CMainFrame *GetMainWnd() { return (class CMainFrame *)AfxGetMainWnd(); }
	inline class CRLoginApp *GetApp() { return (class CRLoginApp *)AfxGetApp(); }

protected:
	int OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

// オーバーライド
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnDraw(CDC* pDC);

// インプリメンテーション
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
//	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LRESULT OnImeNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnImeComposition(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnImeRequest(WPARAM wParam, LPARAM lParam);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnMacroRec();
	afx_msg void OnUpdateMacroRec(CCmdUI* pCmdUI);
	afx_msg void OnMacroPlay();
	afx_msg void OnUpdateMacroPlay(CCmdUI* pCmdUI);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnMacroHis(UINT nID);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseEvent();
	afx_msg void OnUpdateMouseEvent(CCmdUI *pCmdUI);
	afx_msg void OnBroadcast();
	afx_msg void OnUpdateBroadcast(CCmdUI *pCmdUI);
	afx_msg void OnEditCopyAll();
	afx_msg void OnPagePrior();
	afx_msg void OnPageNext();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSearchReg();
	afx_msg void OnSearchBack();
	afx_msg void OnSearchNext();
	afx_msg void OnGoziview();
	afx_msg void OnUpdateGoziview(CCmdUI *pCmdUI);
	afx_msg void OnSplitHeight();
	afx_msg void OnSplitWidth();
	afx_msg void OnSplitOver();
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // RLoginView.cpp ファイルがデバッグ環境の時使用されます。
inline CRLoginDoc* CRLoginView::GetDocument()
   { return (CRLoginDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_RLOGINVIEW_H__03B57615_4E99_4D63_9DF0_4B7D3D47E193__INCLUDED_)
