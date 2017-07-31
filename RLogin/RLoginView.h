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

#define	VTMID_MOUSEMOVE		1024
#define	VTMID_VISUALBELL	1025
#define	VTMID_BLINKUPDATE	1026
#define	VTMID_WHEELMOVE		1027
#define	VTMID_CARETUPDATE	1028
#define	VTMID_GOZIUPDATE	1029
#define	VTMID_SLEEPTIMER	1030
#define	VTMID_IMAGEUPDATE	1031
#define	VTMID_RCLICKCHECK	1032

#define	VIEW_SLEEP_MSEC		10000	// x 1msec = 10sec
#define	VIEW_SLEEP_MAX		60		// x 10sec = 600sec = 10min

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
	BOOL m_HaveBack;
	BOOL m_ActiveFlag;
	BOOL m_VisualBellFlag;
	int m_BlinkFlag;
	int m_ImageFlag;
	BOOL m_MouseEventFlag;
	int m_WheelDelta;
	BOOL m_WheelTimer;
	clock_t m_WheelClock;
	int m_WheelzDelta;
	BOOL m_PastNoCheck;
	BOOL m_ScrollOut;
	BOOL m_ClipUpdateLine;

	clock_t m_RDownClock;
	CPoint m_RDownPoint;
	int m_RDownStat;
	int m_RDownOfs;

	int m_ClipFlag;
	int m_ClipStaPos, m_ClipEndPos;
	int m_ClipStaOld, m_ClipEndOld;
	UINT m_ClipTimer;
	CPoint m_ClipSavePoint;
	UINT m_ClipKeyFlags;
	CToolTipCtrl m_ToolTip;
	UINT m_LastMouseFlags;
	clock_t m_LastMouseClock;
	CPoint m_FirstMousePoint;
	BOOL m_bLButtonTrClk;

	inline BOOL IsClipRectMode() { return (m_ClipKeyFlags & MK_CONTROL); }
	inline BOOL IsClipLineMode() { return (m_bLButtonTrClk || m_ClipKeyFlags & (MK_SHIFT | 0x1000)); }
	int GetClipboard(CBuffer *bp);
	int SetClipboard(CBuffer *bp);

	BOOL m_KeyMacFlag;
	CBuffer m_KeyMacBuf;

	class CGhostWnd *m_pGhost;
	class CMsgWnd m_MsgWnd;
	class CBtnWnd m_BtnWnd;
	class CGrapWnd *m_pSelectGrapWnd;

	BOOL m_SleepView;
	int m_SleepCount;
	int m_MatrixCols[COLS_MAX];

	BOOL m_GoziView;
#if   USE_GOZI == 1 || USE_GOZI == 2
	int m_GoziStyle;
	int m_GoziCount;
	CPoint m_GoziPos;
#elif USE_GOZI == 3 || USE_GOZI == 4
	struct _GoziData {
		int m_GoziStyle;
		int m_GoziCount;
		CPoint m_GoziPos;
	} m_GoziData[8];
	int m_GoziMax;
#endif	// USE_GOZI

#ifdef	USE_DIRECTWRITE
	ID2D1HwndRenderTarget *m_pRenderTarget;
#endif

	int m_CellCols;
	int m_CellLines;
	BYTE *m_pCellSize;

	void ResetCellSize();
	void SetCellSize(int x, int y, int sz);
	int GetGrapPos(int x, int y);

	void InvalidateTextRect(CRect &rect);
	void CalcPosRect(CRect &rect);
	void CalcGrapPoint(CPoint po, int *x, int *y);
	int HitTest(CPoint point);
	void SetFrameRect(int cx, int cy);

	int m_OrigCaretFlag;
	CSize m_OrigCaretSize;
	CRect m_OrigCaretRect;
	CSize m_OrigCaretMapSize;
	CBitmap m_OrigCaretBitmap;
	COLORREF m_OrigCaretColor;
	BOOL m_bOrigCaretAllocCol;

	COLORREF OrigCaretColor();
	void OrigCaretPos(POINT point);
	void OrigCaretSize(int width, int height);

	void DispCaret(BOOL bShow);
	void UpdateCaret();
	void KillCaret();
	void SetCaret();
	void ImmSetPos(int x, int y);
	int ImmOpenCtrl(int sw);

	void SendBuffer(CBuffer &buf, BOOL macflag = FALSE);
	void SetGhostWnd(BOOL sw);
	BOOL ModifyKeys(UINT nChar, int nStat);
	void CreateGrapImage(int type);
	void GetMousePos(int *sw, int *x, int *y);
	void PopUpMenu(CPoint point);
	BOOL SendPasteText(LPCWSTR wstr);

	inline int CalcGrapX(int x)	{ CRLoginDoc *pDoc = GetDocument(); return (m_Width  * x / m_Cols  + pDoc->m_TextRam.m_ScrnOffset.left); }
	inline int CalcGrapY(int y) { CRLoginDoc *pDoc = GetDocument(); return (m_Height * y / m_Lines + pDoc->m_TextRam.m_ScrnOffset.top); }

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
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnInitialUpdate();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
//	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDropFiles(HDROP hDropInfo);

	afx_msg LRESULT OnImeNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnImeComposition(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnImeRequest(WPARAM wParam, LPARAM lParam);

	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnXButtonDown(UINT nFlags, UINT nButton, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditMark();
	afx_msg void OnUpdateEditMark(CCmdUI *pCmdUI);
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void ClipboardPaste(UINT nID);
	afx_msg void OnUpdateClipboardPaste(CCmdUI* pCmdUI);
	afx_msg void OnClipboardMenu();

	afx_msg void OnImageGrapCopy();
	afx_msg void OnImageGrapSave();

	afx_msg void OnMacroRec();
	afx_msg void OnUpdateMacroRec(CCmdUI* pCmdUI);
	afx_msg void OnMacroPlay();
	afx_msg void OnUpdateMacroPlay(CCmdUI* pCmdUI);
	afx_msg void OnMacroHis(UINT nID);
	afx_msg void OnUpdateMacroHis(CCmdUI *pCmdUI);

	afx_msg void OnMouseEvent();
	afx_msg void OnUpdateMouseEvent(CCmdUI *pCmdUI);
	afx_msg void OnGoziview();
	afx_msg void OnUpdateGoziview(CCmdUI *pCmdUI);

	afx_msg void OnPagePrior();
	afx_msg void OnPageNext();
	afx_msg void OnSearchReg();
	afx_msg void OnSearchBack();
	afx_msg void OnSearchNext();
	afx_msg void OnSplitHeight();
	afx_msg void OnSplitWidth();
	afx_msg void OnSplitOver();
	afx_msg void OnSplitHeightNew();
	afx_msg void OnSplitWidthNew();
};

#ifndef _DEBUG  // RLoginView.cpp ファイルがデバッグ環境の時使用されます。
inline CRLoginDoc* CRLoginView::GetDocument()
   { return (CRLoginDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_RLOGINVIEW_H__03B57615_4E99_4D63_9DF0_4B7D3D47E193__INCLUDED_)
