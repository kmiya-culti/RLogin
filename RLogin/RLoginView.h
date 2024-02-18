/////////////////////////////////////////////////////////////////////////////
// CRLoginView

#pragma once

#include "Data.h"
#include "TextRam.h"
#include "MsgWnd.h"

#define	FGCARET_CREATE		0001
#define	FGCARET_ONOFF		0002
#define	FGCARET_FOCUS		0004
#define	FGCARET_MASK		0077

#define	FGCARET_DRAW		0100

#define	VTMID_MOUSEMOVE		1024
#define	VTMID_VISUALBELL	1025
#define	VTMID_WHEELMOVE		1026
#define	VTMID_SMOOTHSCR		1027
#define	VTMID_GOZIUPDATE	1028
#define	VTMID_SLEEPTIMER	1029
#define	VTMID_RCLICKCHECK	1030
#define	VTMID_INVALIDATE	1031

#define	VIEW_SLEEP_MSEC		10000	// x 1msec = 10sec
#define	VIEW_SLEEP_MAX		60		// x 10sec = 600sec = 10min

#define	DELAYINVALMINCLOCK	30		// �ŏ��^�C�}�l msec

#ifdef	USE_OLE
class CViewDropTarget : public COleDropTarget
{
public:
	CViewDropTarget();
	~CViewDropTarget();

	virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);

	BOOL DescToDrop(CWnd* pWnd, COleDataObject* pDataObject, HGLOBAL hDescInfo);
};
#endif	// USE_OLE

class CRLoginView : public CView
{
	DECLARE_DYNCREATE(CRLoginView)

protected:
	CRLoginView();
	virtual ~CRLoginView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
public:
	CRLoginDoc* GetDocument();
#else
public:
	inline CRLoginDoc* CRLoginView::GetDocument() { return (CRLoginDoc*)m_pDocument; }
#endif

// �I�y���[�V����
public:
	int m_Width, m_Height;
	int m_Cols, m_Lines;
	int m_HisMin;
	int m_HisOfs;
	int m_CharWidth, m_CharHeight;
	int m_TopOffset;
	CBmpFile m_BmpFile;
	CBitmap *m_pBitmap;
	BOOL m_HaveBack;
	BOOL m_ActiveFlag;
	BOOL m_VisualBellFlag;
	int m_BlinkFlag;
	int m_ImageFlag;
	BOOL m_MouseEventFlag;
	int m_WheelDelta;
	INT_PTR m_WheelTimer;
	clock_t m_WheelClock;
	int m_WheelzDelta;
	BOOL m_ScrollOut;
	BOOL m_ClipUpdateLine;
	CBitmap m_TekBitmap;
	BOOL m_bDarkMode;

	clock_t m_RDownClock;
	CPoint m_RDownPoint;
	int m_RDownStat;
	int m_RDownOfs;
	CPoint m_RMovePoint;
	int m_RMovePixel;
	INT_PTR m_SmoothTimer;
	INT_PTR m_RclickTimer;

	int m_ClipFlag;
	CCurPos m_ClipStaPosSave;
	CCurPos m_ClipStaPos, m_ClipEndPos;
	CCurPos m_ClipStaOld, m_ClipEndOld;
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
	BOOL m_KeyMacSizeCheck;
	CBuffer m_KeyMacBuf;

	class CGhostWnd *m_pGhost;
	class CMsgWnd m_MsgWnd;
	class CBtnWnd m_BtnWnd;
	class CGrapWnd *m_pSelectGrapWnd;

	BOOL m_SleepView;
	int m_SleepCount;
	CArray<int, int> m_MatrixCols;

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

#ifdef	USE_OLE
	CViewDropTarget m_DropTarget;
#endif

	int m_CellCols;
	int m_CellLines;
	BYTE *m_pCellSize;
	BOOL m_bImmActive;

	void ResetCellSize();
	void SetCellSize(int x, int y, int sz);
	int GetGrapPos(int x, int y);

	typedef struct _DeleyInval {
		clock_t		m_Clock;
		CRect		m_Rect;
	} DeleyInval;

	CList<DeleyInval, DeleyInval &> m_DeleyInvalList;
	BOOL m_bDelayInvalThread;
	DWORD m_DelayInvalWait;
	clock_t m_DelayInvalClock;
	CEvent *m_pDelayInvalEvent;
	CEvent *m_pDelayInvalSync;

	CToolTipCtrl m_VramTip;
	CCurPos m_VramTipPos;
	BOOL m_bVramTipDisp;

	UINT DelayInvalThread();
	void DelayInvalThreadStart();
	void DelayInvalThreadEndof();
	void DelayInvalThreadTimer(clock_t msec);

	void SetDelayInvalTimer();
	void AddDeleyInval(clock_t ick, CRect &rect);
	void PollingDeleyInval();
	BOOL OnIdle();

	void InvalidateTextRect(CRect &rect);
	void InvalidateFullText();
	void CalcPosRect(CRect &rect, CCurPos staPos, CCurPos endPos, BOOL bLine = FALSE);
	void CalcGrapPoint(CPoint po, int *x, int *y);
	int HitTest(CPoint point);
	void SetFrameRect(int cx, int cy);

	int m_CaretFlag;
	int m_CaretX, m_CaretY;
	CSize m_CaretSize;
	CRect m_CaretRect;
	CSize m_CaretMapSize;
	CSize m_CaretOffset;
	CBitmap m_CaretBitmap;
	COLORREF m_CaretColor;
	BOOL m_bCaretAllocCol;
	COLORREF m_CaretAllocColor;
	clock_t m_CaretBaseClock;
	int m_CaretAnimeMax;
	int m_CaretAnimeClock;

	COLORREF CaretColor();
	inline void CaretInitView() { m_CaretBaseClock = clock(); }

	void InvalidateCaret();
	void DispCaret(BOOL bShow);
	void UpdateCaret();
	void KillCaret();
	void SetCaret();
	void ImmSetPos(int x, int y);
	BOOL ImmOpenCtrl(int sw);

	void SendBuffer(CBuffer &buf, BOOL macflag = FALSE, BOOL delaySend = FALSE);
	void SetGhostWnd(BOOL sw);
	BOOL ModifyKeys(UINT nChar, int nStat);
	void CreateGrapImage(int type);
	void GetMousePos(int *sw, int *x, int *y);
	void PopUpMenu(CPoint point);
	void SendBracketedPaste(LPCWSTR str, BOOL bDelay);
	void SendPasteText(LPCWSTR wstr);

	void KillScrollTimer();
	BOOL SetDropFile(LPCTSTR FileName, BOOL &doCmd, BOOL &doSub);

	BOOL m_bSpeakDispText;
	CCurPos m_SpeakStaPos, m_SpeakEndPos;

	void SpeakTextPos(BOOL bDisp, CCurPos *pStaPos, CCurPos *pEndPos);

	inline int CalcGrapX(int x)	{ CRLoginDoc *pDoc = GetDocument(); return (m_Width  * x / m_Cols  + pDoc->m_TextRam.m_ScrnOffset.left); }
	inline int CalcGrapY(int y) { CRLoginDoc *pDoc = GetDocument(); return (m_Height * y / m_Lines + pDoc->m_TextRam.m_ScrnOffset.top + m_TopOffset); }

	inline class CChildFrame *GetFrameWnd() { return (class CChildFrame *)GetParentFrame(); }
	inline class CMainFrame *GetMainWnd() { return (class CMainFrame *)AfxGetMainWnd(); }
	inline class CRLoginApp *GetApp() { return (class CRLoginApp *)AfxGetApp(); }

protected:
	int OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

// �I�[�o�[���C�h
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

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
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
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);

	afx_msg LRESULT OnImeNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnImeComposition(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnImeRequest(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLogWrite(WPARAM wParam, LPARAM lParam);

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
	afx_msg void OnLineeditmode();
	afx_msg void OnUpdateLineeditmode(CCmdUI *pCmdUI);

	afx_msg void OnImageGrapCopy();
	afx_msg void OnImageGrapSave();
	afx_msg void OnImageGrapHistogram();
	afx_msg void OnUpdateHistogram(CCmdUI* pCmdUI);

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
