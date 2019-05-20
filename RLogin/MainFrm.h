// MainFrm.h : CMainFrame クラスのインターフェイス
//

#pragma once

#include "Data.h"
#include "ExtSocket.h"
#include "TabBar.h"
#include "IConv.h"
#include "Ssh.h"
#include "MidiData.h"

#define	PANEFRAME_NOCHNG		0
#define	PANEFRAME_MAXIM			1
#define	PANEFRAME_WIDTH			2
#define	PANEFRAME_HEIGHT		3
#define	PANEFRAME_WINDOW		4
#define	PANEFRAME_WSPLIT		5
#define	PANEFRAME_HSPLIT		6
#define	PANEFRAME_WEVEN			7
#define	PANEFRAME_HEVEN			8

#define	PANEMIN_WIDTH			32
#define	PANEMIN_HEIGHT			32

class CPaneFrame : public CObject
{
public:
	class CMainFrame *m_pMain;
	class CPaneFrame *m_pOwn;
	class CPaneFrame *m_pLeft;
	class CPaneFrame *m_pRight;

	int m_Style;
	int m_BoderSize;
	BOOL m_bActive;
	BOOL m_bReqSize;

	CRect m_Frame;
	HWND m_hWnd;
	CStatic m_NullWnd;
	CServerEntry *m_pServerEntry;
	int m_TabIndex;

	void Attach(HWND hWnd);
	void CreatePane(int Style, HWND hWnd);
	CPaneFrame *InsertPane();
	class CPaneFrame *DeletePane(HWND hWnd);
	class CPaneFrame *GetPane(HWND hWnd);
	class CPaneFrame *GetActive();
	class CPaneFrame *GetNull();
	class CPaneFrame *GetEntry();
	int SetActive(HWND hWnd);
	int IsOverLap(CPaneFrame *pPane);
	BOOL IsTopLevel(CPaneFrame *pPane);
	int GetPaneCount(int count);
	BOOL IsReqSize();

	void GetNextPane(int mode, class CPaneFrame *pThis, class CPaneFrame **ppPane);
	void MoveFrame();
	void MoveParOwn(CRect &rect, int Style);

	void HitActive(CPoint &po);
	class CPaneFrame *HitTest(CPoint &po);
	int BoderRect(CRect &rect);

	void SetBuffer(CBuffer *buf, BOOL bEntry = TRUE);
	static class CPaneFrame *GetBuffer(class CMainFrame *pMain, class CPaneFrame *pPane, class CPaneFrame *pOwn, CBuffer *buf);

	CPaneFrame(class CMainFrame *pMain, HWND hWnd, class CPaneFrame *pOwn);
	~CPaneFrame();

#ifdef _DEBUG
	void Dump();
#endif
};

#define	TIMEREVENT_DOC		001
#define	TIMEREVENT_SOCK		002
#define	TIMEREVENT_SCRIPT	003
#define	TIMEREVENT_TEXTRAM	004
#define	TIMEREVENT_CLOSE	005
#define	TIMEREVENT_INTERVAL	010

#define	TIMERID_SLEEPMODE	1024
#define	TIMERID_MIDIEVENT	1025
#define	TIMERID_STATUSCLR	1026
#define	TIMERID_CLIPUPDATE	1027
#define	TIMERID_IDLETIMER	1028
#define	TIMERID_TIMEREVENT	1100

class CTimerObject : public CObject
{
public:
	int m_Id;
	int m_Mode;
	void *m_pObject;
	class CTimerObject *m_pList;
	CTimerObject();
	void CallObject();
};

class CMidiQue : public CObject
{
public:
	int m_mSec;
	DWORD m_Msg;
};

class CMenuBitMap : public CObject
{
public:
	int m_Id;
	CBitmap m_Bitmap;
};

#define	CLIPOPENTHREADMAX	3		// クリップボードアクセススレッド多重起動数
#define	CLIPOPENLASTMSEC	500		// 指定msec以内のクリップボードアップデートを無視する

#define	SOCKPARAHASH		4
#define	SOCKPARAMASK(fd)	(int)(((INT_PTR)fd / sizeof(INT_PTR)) & (SOCKPARAHASH - 1))

#define	DEFAULT_DPI_X		GlobalSystemDpi
#define	DEFAULT_DPI_Y		GlobalSystemDpi
#define	SCREEN_DPI_X		((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiX
#define	SCREEN_DPI_Y		((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)

public:
	CMainFrame();
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// 実装
public:
	int m_IconShow;
	HICON m_hIcon;
	HICON m_hIconActive;
	NOTIFYICONDATA m_IconData;
	CImageList m_ImageGozi;
	CPtrArray m_SocketParam[SOCKPARAHASH];
	CPtrArray m_HostAddrParam;
	CPtrArray m_AfterIdParam;
	CServerEntryTab m_ServerEntryTab;
	CIdKeyTab m_IdKeyTab;
	CPaneFrame *m_pTopPane;
	CRect m_Frame;
	BOOL m_LastPaneFlag;
	CBuffer m_AllFileBuf;
	CString m_AllFilePath;
	int m_TimerSeqId;
	CTimerObject *m_pTimerUsedId;
	CTimerObject *m_pTimerFreeId;
	UINT_PTR m_SleepTimer;
	int m_SleepStatus;
	int m_SleepCount;
	int m_TransParValue;
	COLORREF m_TransParColor;
	class CMidiData *m_pMidiData;
	UINT_PTR m_MidiTimer;
	CList<class CMidiQue *, class CMidiQue *> m_MidiQue;
	volatile int m_InfoThreadCount;
	BOOL m_bTabBarShow;
	BOOL m_ScrollBarFlag;
	BOOL m_bVersionCheck;
	BOOL m_bGlassStyle;
	CPtrArray m_MenuMap;
	int m_SplitType;
	int m_ExecCount;
	HMENU m_StartMenuHand;
	BOOL m_bAllowClipChain;
	BOOL m_bClipEnable;
	BOOL m_bClipChain;
	HWND m_hNextClipWnd;
	UINT_PTR m_ClipTimer;
	BOOL m_bBroadCast;
	CList<CStringW, CStringW &> m_ClipBoard;
	int m_ScreenX;
	int m_ScreenY;
	int m_ScreenW;
	int m_ScreenH;
	UINT_PTR m_StatusTimer;
	UINT m_ScreenDpiX;
	UINT m_ScreenDpiY;
	CKeyNodeTab m_DefKeyTab;
	BOOL m_UseBitmapUpdate;
	UINT_PTR m_IdleTimer;
	BOOL m_bPostIdleMsg;
	clock_t m_LastClipUpdate;
	class CServerSelect *m_pServerSelect;

	BOOL WageantQuery(CBuffer *pInBuf, CBuffer *pOutBuf);
	BOOL PageantQuery(CBuffer *pInBuf, CBuffer *pOutBuf);

	BOOL AgeantInit();
	BOOL AgeantSign(int type, CBuffer *blob, CBuffer *sign, LPBYTE buf, int len);

	void SetTransPar(COLORREF rgb, int value, DWORD flag);
	void SetIconStyle();
	void SetIconData(HICON hIcon, LPCTSTR str);

	int SetAsyncSelect(SOCKET fd, CExtSocket *pSock, long lEvent = 0);
	void DelAsyncSelect(SOCKET fd, CExtSocket *pSock, BOOL useWsa = TRUE);
	int SetAsyncHostAddr(int mode, LPCTSTR pHostName, CExtSocket *pSock);
	int SetAsyncAddrInfo(int mode, LPCTSTR pHostName, int PortNum, void *pHint, CExtSocket *pSock);
	int SetAfterId(void *param);
	void UpdateServerEntry();
	int OpenServerEntry(CServerEntry &Entry);

	int SetTimerEvent(int msec, int mode, void *pParam);
	void DelTimerEvent(void *pParam, int Id = 0);
	void RemoveTimerEvent(CTimerObject *pObject);
	void FreeTimerEvent(CTimerObject *pObject);

	void SetMidiData(int nInit, int nPlay, LPCSTR mml);
	void SetMidiEvent(int msec, DWORD msg);

	void SetIdleTimer(BOOL bSw);
	void PostIdleMessage();

	void SetWakeUpSleep(int sec);
	void WakeUpSleep();

	BOOL IsConnectChild(CPaneFrame *pPane);
	void AddChild(CWnd *pWnd);
	void RemoveChild(CWnd *pWnd, BOOL bDelete);
	void ActiveChild(CWnd *pWnd);
	void MoveChild(CWnd *pWnd, CPoint point);
	BOOL IsWindowPanePoint(CPoint point);
	void SwapChild(CWnd *pLeft, CWnd *pRight);
	BOOL IsOverLap(HWND hWnd);
	BOOL IsTopLevelDoc(CRLoginDoc *pDoc);
	int GetTabIndex(CWnd *pWnd);
	void GetTabTitle(CWnd *pWnd, CString &title);
	CWnd *GetTabWnd(int idx);
	int GetTabCount();
	class CRLoginDoc *GetMDIActiveDocument();

	void GetFrameRect(CRect &frame);
	void AdjustRect(CRect &rect);

	void SplitWidthPane();
	void SplitHeightPane();
	CPaneFrame *GetPaneFromChild(HWND hWnd);

	CPaneFrame *m_pTrackPane;
	CRect m_TrackRect;
	CPoint m_TrackPoint;

	void OffsetTrack(CPoint point);
	void InvertTracker(CRect &rect);
	int PreLButtonDown(UINT nFlags, CPoint point);
	int GetExecCount();
	void SetActivePoint(CPoint point);

	volatile int m_bClipThreadCount;
	CMutexLock m_OpenClipboardLock;

	void ClipBoradStr(LPCWSTR str, CString &tmp);
	void SetClipBoardComboBox(CComboBox *pCombo);
	void SetClipBoardMenu(UINT nId, CMenu *pMenu);
	BOOL CopyClipboardData(CString &str);
	BOOL GetClipboardText(CString &str);
	BOOL SetClipboardText(LPCTSTR str, LPCSTR rtf = NULL);

	inline BOOL IsTimerIdleBusy() { return (m_IdleTimer != 0 ? TRUE : FALSE); }
	BOOL TrackPopupMenuIdle(CMenu *pMenu, UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect = 0);
	void SetMenuBitmap(CMenu *pMenu);
	CBitmap *GetMenuBitmap(UINT nId);
	void InitMenuBitmap();
	void SetStatusText(LPCTSTR message);

	CString m_VersionMessage;
	CString m_VersionPageUrl;

	void VersionCheckProc();
	void VersionCheck();

	inline CImageList *GetTabImageList() { return &(m_wndTabBar.m_ImageList); }
	inline int GetTabImageIndex(LPCTSTR filename) { return m_wndTabBar.GetImageIndex(filename); }
	inline void TabBarFontCheck() { m_wndTabBar.FontSizeCheck(); this->RecalcLayout(TRUE); }

// コントロール バー用メンバ
protected: 
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CTabBar		m_wndTabBar;

// オーバーライド
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void RecalcLayout(BOOL bNotify = TRUE);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// 生成された、メッセージ割り当て関数
protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnEnterMenuLoop(BOOL bIsTrackPopupMenu);
	afx_msg void OnExitMenuLoop(BOOL bIsTrackPopupMenu);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnClose();
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

	afx_msg void OnDrawClipboard();
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnClipboardUpdate();

	afx_msg void OnUpdateIndicatorSock(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorStat(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorKmod(CCmdUI* pCmdUI);

	afx_msg BOOL OnToolTipText(UINT nId, NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnPaneWsplit();
	afx_msg void OnPaneHsplit();
	afx_msg void OnPaneDelete();
	afx_msg void OnPaneSave();
	afx_msg void OnWinodwSelect(UINT nID);
	afx_msg void OnActiveMove(UINT nID);
	afx_msg void OnUpdateActiveMove(CCmdUI *pCmdUI);

	afx_msg void OnWindowCascade();
	afx_msg void OnUpdateWindowCascade(CCmdUI* pCmdUI);
	afx_msg void OnWindowTileHorz();
	afx_msg void OnUpdateWindowTileHorz(CCmdUI* pCmdUI);
	afx_msg void OnWindowRotation();
	afx_msg void OnUpdateWindowRotation(CCmdUI *pCmdUI);
	afx_msg void OnWindowPrev();
	afx_msg void OnUpdateWindowPrev(CCmdUI *pCmdUI);
	afx_msg void OnWinodwNext();
	afx_msg void OnUpdateWinodwNext(CCmdUI *pCmdUI);

	afx_msg void OnViewMenubar();
	afx_msg void OnUpdateViewMenubar(CCmdUI *pCmdUI);
	afx_msg void OnViewTabbar();
	afx_msg void OnUpdateViewTabbar(CCmdUI *pCmdUI);
	afx_msg void OnViewScrollbar();
	afx_msg void OnUpdateViewScrollbar(CCmdUI *pCmdUI);
	afx_msg void OnVersioncheck();
	afx_msg void OnUpdateVersioncheck(CCmdUI *pCmdUI);
	afx_msg void OnNewVersionFound();
	afx_msg void OnClipchain();
	afx_msg void OnUpdateClipchain(CCmdUI *pCmdUI);

	afx_msg void OnFileAllLoad();
	afx_msg void OnFileAllSave();
	afx_msg void OnBroadcast();
	afx_msg void OnUpdateBroadcast(CCmdUI *pCmdUI);
	afx_msg void OnToolcust();
	afx_msg void OnDeleteOldEntry();
	afx_msg void OnTabmultiline();
	afx_msg void OnUpdateTabmultiline(CCmdUI *pCmdUI);

	afx_msg LRESULT OnWinSockSelect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetHostAddr(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIConMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnThreadMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAfterOpen(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetClipboard(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNullMessage(WPARAM wParam, LPARAM lParam);
};


