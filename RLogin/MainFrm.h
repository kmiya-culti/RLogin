// MainFrm.h : CMainFrame クラスのインターフェイス
//

#pragma once

#include "Data.h"
#include "ExtSocket.h"
#include "TabBar.h"
#include "IConv.h"
#include "Ssh.h"

typedef BOOL __stdcall SETLAYER( HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags );

#define	PANEFRAME_NOCHNG		0
#define	PANEFRAME_MAXIM			1
#define	PANEFRAME_WIDTH			2
#define	PANEFRAME_HEIGHT		3
#define	PANEFRAME_WINDOW		4

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

	CRect m_Frame;
	HWND m_hWnd;
	CStatic m_NullWnd;
	CServerEntry *m_pServerEntry;

	void Attach(HWND hWnd);
	void CreatePane(int Style, HWND hWnd);
	class CPaneFrame *DeletePane(HWND hWnd);
	class CPaneFrame *GetPane(HWND hWnd);
	class CPaneFrame *GetActive();
	class CPaneFrame *GetNull();
	class CPaneFrame *GetEntry();
	int SetActive(HWND hWnd);

	void MoveFrame();
	void MoveParOwn(CRect &rect, int Style);

	void HitActive(CPoint &po);
	class CPaneFrame *HitTest(CPoint &po);
	int BoderRect(CRect &rect);

	void SetBuffer(CBuffer *buf);
	static class CPaneFrame *GetBuffer(class CMainFrame *pMain, class CPaneFrame *pPane, class CPaneFrame *pOwn, CBuffer *buf);

	CPaneFrame(class CMainFrame *pMain, HWND hWnd, class CPaneFrame *pOwn);
	~CPaneFrame();
};

#define	TIMEREVENT_DOC		001
#define	TIMEREVENT_SOCK		002
#define	TIMEREVENT_INTERVAL	010

#define	TIMERID_SLEEPMODE	1024
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

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// 属性
public:

// 操作
public:

// オーバーライド
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void RecalcLayout(BOOL bNotify = TRUE);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// 実装
public:
	int m_IconShow;
	HICON m_hIcon;
	HICON m_hIconActive;
	NOTIFYICONDATA m_IconData;
	CImageList m_ImageList[3];
	CPtrArray m_SocketParam;
	CPtrArray m_HostAddrParam;
	CServerEntryTab m_ServerEntryTab;
	CIdKeyTab m_IdKeyTab;
	CPaneFrame *m_pTopPane;
	CRect m_Frame;
	CString m_StatusString;
	BOOL m_LastPaneFlag;
	CBuffer m_AllFileBuf;
	CString m_AllFilePath;
	BOOL m_ModifiedFlag;
	int m_TimerSeqId;
	CTimerObject *m_pTimerUsedId;
	CTimerObject *m_pTimerFreeId;
	UINT_PTR m_SleepTimer;
	int m_SleepStatus;
	int m_SleepCount;
	int m_TransParValue;

	void SetTransPar(COLORREF rgb, int value, DWORD flag);
	void SetIconStyle();
	void SetIconData(HICON hIcon, LPCSTR str);

	int SetAsyncSelect(SOCKET fd, CExtSocket *pSock, long lEvent = 0);
	void DelAsyncSelect(SOCKET fd, CExtSocket *pSock);
	int SetAsyncHostAddr(LPCSTR pHostName, CExtSocket *pSock);
	int OpenServerEntry(CServerEntry &Entry);
	BOOL SaveModified( );

	int SetTimerEvent(int msec, int mode, void *pParam);
	void DelTimerEvent(void *pParam);

	void SetWakeUpSleep(int sec);
	void WakeUpSleep();

	void AddChild(CWnd *pWnd);
	void RemoveChild(CWnd *pWnd);
	void ActiveChild(CWnd *pWnd);
	void MoveChild(CWnd *pWnd, CPoint point);

	void GetFrameRect(CRect &frame);
	void AdjustRect(CRect &rect);

	CPaneFrame *m_pTrackPane;
	CRect m_TrackRect;
	CPoint m_TrackPoint;

	void OffsetTrack(CPoint point);
	void InvertTracker(CRect &rect);
	int PreLButtonDown(UINT nFlags, CPoint point);

	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // コントロール バー用メンバ
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CTabBar		m_wndTabBar;

// 生成された、メッセージ割り当て関数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnPaneWsplit();
	afx_msg void OnPaneHsplit();
	afx_msg void OnPaneDelete();
	afx_msg void OnPaneSave();
	afx_msg void OnWindowCascade();
	afx_msg void OnWindowTileHorz();
	afx_msg void OnUpdateWindowCascade(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowTileHorz(CCmdUI* pCmdUI);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnFileAllSave();
	//}}AFX_MSG
	afx_msg LRESULT OnWinSockSelect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetHostAddr(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIConMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnThreadMsg(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateIndicatorSock(CCmdUI* pCmdUI);
	afx_msg void OnFileAllLoad();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
};


