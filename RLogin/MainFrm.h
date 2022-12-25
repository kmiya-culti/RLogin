//////////////////////////////////////////////////////////////////////
// CMainFrame

#pragma once

#include "Data.h"
#include "ExtSocket.h"
#include "TabBar.h"
#include "IConv.h"
#include "Ssh.h"
#include "MidiData.h"
#include "ComboBoxHis.h"
#include "Shobjidl.h"

#define	PANEFRAME_NOCHNG		0
#define	PANEFRAME_MAXIM			1
#define	PANEFRAME_WIDTH			2
#define	PANEFRAME_HEIGHT		3
#define	PANEFRAME_WINDOW		4
#define	PANEFRAME_WSPLIT		5
#define	PANEFRAME_HSPLIT		6
#define	PANEFRAME_WEVEN			7
#define	PANEFRAME_HEVEN			8
#define	PANEFRAME_HEDLG			9

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

class CDockContextEx : public CDockContext
{
public:
	explicit CDockContextEx(CControlBar *pBar);

	BOOL IsHitGrip(CPoint point);

	virtual void StartDrag(CPoint pt);
	virtual void StartResize(int nHitTest, CPoint pt);
	virtual void ToggleDocking();

	static void EnableDocking(CControlBar *pBar, DWORD dwDockStyle);

	void TrackLoop();
};

class CDialogBarEx : public CDialogBar
{
	DECLARE_DYNAMIC(CDialogBarEx)

public:
	CDialogBarEx();
	virtual ~CDialogBarEx();

public:
	CSize m_InitDpi;
	CSize m_NowDpi;
	CFont m_DpiFont;
	CSize m_ZoomMul;
	CSize m_ZoomDiv;
	CFont m_NewFont;

public:
	void DpiChanged();
	void FontSizeCheck();

public:
	BOOL Create(CWnd* pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	inline BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID) { return Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID); }

public:
	DECLARE_MESSAGE_MAP()
};

class CVoiceBar : public CDialogBarEx
{
	DECLARE_DYNAMIC(CVoiceBar)

public:
	CVoiceBar();
	virtual ~CVoiceBar();

	enum { IDD = IDD_VOICEBAR };

public:
	CComboBox m_VoiceDesc;
	CSliderCtrl m_VoiceRate;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

public:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);
	afx_msg void OnCbnSelchangeVoicedesc();
	afx_msg void OnNMReleasedcaptureVoicerate(NMHDR *pNMHDR, LRESULT *pResult);
};

class CQuickBar : public CDialogBarEx
{
	DECLARE_DYNAMIC(CQuickBar)

public:
	CQuickBar();
	virtual ~CQuickBar();

	enum { IDD = IDD_QUICKBAR };

public:
	CComboBoxHis m_EntryWnd;
	CComboBoxHis m_HostWnd;
	CComboBoxHis m_PortWnd;
	CComboBoxHis m_UserWnd;
	CEdit m_PassWnd;

public:
	void InitDialog();
	void SaveDialog();
	void SetComdLine(CString &cmds);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnCbnEditchangeEntryname();
};

class CTabDlgBar : public CControlBar
{
	DECLARE_DYNAMIC(CTabDlgBar)

public:
	CTabDlgBar();
	virtual ~CTabDlgBar();

public:
	CTabCtrl m_TabCtrl;
	CString m_FontName;
	int m_FontSize;
	CFont m_TabFont;
	CSize m_InitSize;
	CWnd *m_pShowWnd;
	CImageList m_ImageList;

	struct _DlgWndData {
		CWnd *pWnd;
		int nImage;
		CWnd *pParent;
		HMENU hMenu;
		CRect WinRect;
	};
	CPtrArray m_Data;

public:
	BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);
	void Add(CWnd *pWnd, int nImage);
	void Del(CWnd *pWnd);
	void Sel(CWnd *pWnd);
	BOOL IsInside(CWnd *pWnd);
	void *RemoveAt(int idx, CPoint point);
	void RemoveAll();
	void FontSizeCheck();
	void DpiChanged();
	void TabReSize();
	int HitPoint(CPoint point);
	void GetTitle(int nIndex, CString &title);
	void TrackLoop(CPoint ptScrn, int idx, CWnd *pMoveWnd, int nImage);

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};

#define	CLIPOPENTHREADMAX	3		// クリップボードアクセススレッド多重起動数
#define	CLIPOPENLASTMSEC	500		// 指定msec以内のクリップボードアップデートを無視する

#define	SOCKPARAHASH		4
#define	SOCKPARAMASK(fd)	(int)(((INT_PTR)fd / sizeof(INT_PTR)) & (SOCKPARAHASH - 1))


#define	DEFAULT_DPI_X		96
#define	DEFAULT_DPI_Y		96
#define	SYSTEM_DPI_X		GlobalSystemDpi
#define	SYSTEM_DPI_Y		GlobalSystemDpi
#define	SCREEN_DPI_X		((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiX
#define	SCREEN_DPI_Y		((CMainFrame *)::AfxGetMainWnd())->m_ScreenDpiY

#define	SPEAKQUESIZE		2

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
	BOOL m_bMenuBarShow;
	BOOL m_bTabBarShow;
	BOOL m_bTabDlgBarShow;
	BOOL m_bQuickConnect;
	BOOL m_ScrollBarFlag;
	BOOL m_bVersionCheck;
	BOOL m_bGlassStyle;
	BOOL m_bCharTooltip;
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
	class CHistoryDlg *m_pHistoryDlg;
	class CAnyPastDlg *m_pAnyPastDlg;
	BOOL m_PastNoCheck;

	BOOL WageantQuery(CBuffer *pInBuf, CBuffer *pOutBuf, LPCTSTR pipename);
	BOOL PageantQuery(CBuffer *pInBuf, CBuffer *pOutBuf);

	LPCTSTR PagentPipeName();

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

	void AddHistory(void *pCmdHis);
	
	void AddTabDlg(CWnd *pWnd, int nImage, CPoint point);
	void AddTabDlg(CWnd *pWnd, int nImage);
	void DelTabDlg(CWnd *pWnd);
	void SelTabDlg(CWnd *pWnd);
	BOOL IsInsideDlg(CWnd *pWnd);

	BOOL IsConnectChild(CPaneFrame *pPane);
	void AddChild(CWnd *pWnd);
	void RemoveChild(CWnd *pWnd, BOOL bDelete);
	void ActiveChild(class CChildFrame *pWnd);
	void MoveChild(CWnd *pWnd, CPoint point);
	CPaneFrame *GetWindowPanePoint(CPoint point);
	void SwapChild(CWnd *pLeft, CWnd *pRight);
	BOOL IsOverLap(HWND hWnd);
	BOOL IsTopLevelDoc(CRLoginDoc *pDoc);
	int GetTabIndex(CWnd *pWnd);
	void GetTabTitle(CWnd *pWnd, CString &title);
	CWnd *GetTabWnd(int idx);
	int GetTabCount();
	class CRLoginDoc *GetMDIActiveDocument();

	void GetCtrlBarRect(LPRECT rect, CControlBar *pCtrl);
	void GetFrameRect(CRect &frame);
	void FrameToClient(LPRECT lpRect);
	void FrameToClient(LPPOINT lpPoint);
	void ClientToFrame(LPRECT lpRect);
	void ClientToFrame(LPPOINT lpPoint);

	void SplitWidthPane();
	void SplitHeightPane(BOOL bDialog = FALSE);
	CPaneFrame *GetPaneFromChild(HWND hWnd);

	CPaneFrame *m_pTrackPane;
	CRect m_TrackRect;
	CRect m_TrackBase;
	CRect m_TrackLast;
	CPoint m_TrackPoint;

	void OffsetTrack(CPoint point);
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
	void AnyPastDlgClose(int DocSeqNumber);
	void AnyPastDlgOpen(LPCWSTR str, int DocSeqNumber);

	inline BOOL IsTimerIdleBusy() { return (m_IdleTimer != 0 ? TRUE : FALSE); }
	BOOL TrackPopupMenuIdle(CMenu *pMenu, UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect = 0);
	void SetMenuBitmap(CMenu *pMenu);
	CBitmap *GetMenuBitmap(UINT nId);
	void InitMenuBitmap();
	void SetStatusText(LPCTSTR message);

	CString m_VersionMessage;
	CString m_VersionPageUrl;

	int VersionCheckProc();
	void VersionCheck();

	CComPtr<ITaskbarList3> m_pTaskbarList;

	void SetTaskbarProgress(int state, int value);

	BOOL m_bVoiceEvent;
	struct _SpeakData {
		ULONG num;
		CString text;
		CArray<CCurPos, CCurPos &> pos;
		long skip;
		int abs;
		int line;
	} m_SpeakData[SPEAKQUESIZE];
	int m_SpeakQueLen;
	int m_SpeakQuePos;
	int m_SpeakQueTop;
	class CRLoginView *m_pSpeakView;
	class CRLoginDoc *m_pSpeakDoc;
	int m_SpeakCols;
	int m_SpeakLine;
	int m_SpeakAbs;
	int m_SpeakActive[3];

	BOOL SpeakQueIn();
	void Speak(LPCTSTR str);
	void SpeakUpdate(int x, int y);
	inline BOOL SpeakViewCheck(class CRLoginView *pWnd) { return (m_bVoiceEvent && m_pSpeakView == pWnd ? TRUE : FALSE); }

	inline CImageList *GetTabImageList() { return &(m_wndTabBar.m_ImageList); }
	inline int GetTabImageIndex(LPCTSTR filename) { return m_wndTabBar.GetImageIndex(filename); }
	inline void BarFontCheck() { m_wndTabBar.FontSizeCheck(); m_wndQuickBar.FontSizeCheck(); m_wndVoiceBar.FontSizeCheck(); m_wndTabDlgBar.FontSizeCheck(); RecalcLayout(TRUE); }
	inline void QuickBarInit() { m_wndQuickBar.InitDialog(); }
	inline void TabDlgShow(BOOL bShow) { ShowControlBar(&m_wndTabDlgBar, bShow, FALSE); }
	inline BOOL TabDlgInDrag(CPoint point, CWnd *pWnd, int nImage) { if ( !m_bTabDlgBarShow ) return FALSE; m_wndTabDlgBar.TrackLoop(point, (-7), pWnd, nImage); return TRUE; }

// コントロール バー用メンバ
protected: 
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CTabBar		m_wndTabBar;
	CQuickBar	m_wndQuickBar;
	CTabDlgBar	m_wndTabDlgBar;
	CVoiceBar   m_wndVoiceBar;

// オーバーライド
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void RecalcLayout(BOOL bNotify = TRUE);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// 生成された、メッセージ割り当て関数
protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnEnterMenuLoop(BOOL bIsTrackPopupMenu);
	afx_msg void OnExitMenuLoop(BOOL bIsTrackPopupMenu);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
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
	afx_msg void OnViewQuickbar();
	afx_msg void OnUpdateViewQuickbar(CCmdUI *pCmdUI);
	afx_msg void OnViewVoicebar();
	afx_msg void OnUpdateViewVoicebar(CCmdUI *pCmdUI);
	afx_msg void OnViewTabDlgbar();
	afx_msg void OnUpdateViewTabDlgbar(CCmdUI *pCmdUI);
	afx_msg void OnViewTabbar();
	afx_msg void OnUpdateViewTabbar(CCmdUI *pCmdUI);
	afx_msg void OnViewScrollbar();
	afx_msg void OnUpdateViewScrollbar(CCmdUI *pCmdUI);
	afx_msg void OnViewHistoryDlg();
	afx_msg void OnUpdateHistoryDlg(CCmdUI *pCmdUI);

	afx_msg void OnVersioncheck();
	afx_msg void OnUpdateVersioncheck(CCmdUI *pCmdUI);
	afx_msg void OnNewVersionFound();
	afx_msg void OnClipchain();
	afx_msg void OnUpdateClipchain(CCmdUI *pCmdUI);
	afx_msg void OnSpeakText();
	afx_msg void OnUpdateSpeakText(CCmdUI *pCmdUI);
	afx_msg void OnSpeakBack();
	afx_msg void OnSpeakNext();

	afx_msg void OnFileAllLoad();
	afx_msg void OnFileAllSave();
	afx_msg void OnBroadcast();
	afx_msg void OnUpdateBroadcast(CCmdUI *pCmdUI);
	afx_msg void OnToolcust();
	afx_msg void OnDeleteOldEntry();
	afx_msg void OnTabmultiline();
	afx_msg void OnUpdateTabmultiline(CCmdUI *pCmdUI);
	afx_msg void OnQuickConnect();
	afx_msg void OnUpdateConnect(CCmdUI *pCmdUI);
	afx_msg void OnKnownhostdel();
	afx_msg void OnChartooltip();
	afx_msg void OnUpdateChartooltip(CCmdUI *pCmdUI);

	afx_msg LRESULT OnWinSockSelect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetHostAddr(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIConMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnThreadMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAfterOpen(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetClipboard(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNullMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSpeakMsg(WPARAM wParam, LPARAM lParam);
};


