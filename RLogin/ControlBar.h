//////////////////////////////////////////////////////////////////////
// ControlBar...

#pragma once

#define	DELAYDESTORY_NONE		0
#define	DELAYDESTORY_RESERV		1
#define	DELAYDESTORY_REQUEST	2
#define	DELAYDESTORY_EXEC		3

class CDockContextEx : public CDockContext
{
public:
	explicit CDockContextEx(CControlBar *pBar);

public:
	class CMiniDockFrameWndEx *m_pDestroyWnd;

public:
	virtual void StartDrag(CPoint pt);
	virtual void StartResize(int nHitTest, CPoint pt);
	virtual void ToggleDocking();

	static void EnableDocking(CControlBar *pBar, DWORD dwDockStyle);
	static BOOL IsHitGrip(CControlBar *pBar, CPoint point);

	void TrackLoop();
};

class CControlBarEx : public CControlBar
{
	DECLARE_DYNAMIC(CControlBarEx)

public:
	CControlBarEx();

public:
	BOOL m_bDarkMode;

public:
	static void DrawBorders(CControlBar *pBar, CDC* pDC, CRect& rect, COLORREF bkColor);
	static void DrawGripper(CControlBar *pBar, CDC* pDC, const CRect& rect);

	static BOOL DarkModeCheck(CControlBar *pBar);
	static BOOL SetCursor(CControlBar *pBar, UINT nHitTest, UINT message);
	static BOOL SettingChange(CControlBar *pBar, BOOL &bDarkMode, UINT uFlags, LPCTSTR lpszSection);
	static BOOL EraseBkgnd(CControlBar *pBar, CDC* pDC, COLORREF bkCol);

public:
	virtual void DrawBorders(CDC* pDC, CRect& rect);
	virtual BOOL DrawThemedGripper(CDC* pDC, const CRect& rect, BOOL fCentered);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

class CDockBarEx : public CDockBar
{
	DECLARE_DYNAMIC(CDockBarEx)

public:
	CDockBarEx(BOOL bFloating = FALSE);

public:
	BOOL m_bDarkMode;

	typedef struct _BarPosNode {
		HWND hWnd;
		CRect rect;
	} BarPosNode;
	CArray<BarPosNode, BarPosNode &> m_SavePos;

public:
	void SaveBarPos();
	void LoadBarPos(CControlBar *pThis);

	virtual void DrawBorders(CDC* pDC, CRect& rect);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

class CMiniDockFrameWndEx : public CMiniDockFrameWnd
{
	DECLARE_DYNCREATE(CMiniDockFrameWndEx)

public:
	CMiniDockFrameWndEx();

public:
	BOOL m_bDarkMode;
	int m_DelayedDestroy;
	CDockBarEx m_wndDockBarEx;

	virtual BOOL DestroyWindow();
	virtual BOOL Create(CWnd* pParent, DWORD dwBarStyle);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
};

class CDialogBarEx : public CDialogBar
{
	DECLARE_DYNAMIC(CDialogBarEx)

public:
	CDialogBarEx();

public:
	CSize m_InitDpi;
	CSize m_NowDpi;
	CSize m_ZoomMul;
	CSize m_ZoomDiv;
	CFont m_NewFont;
	BOOL m_bDarkMode;

public:
	void DpiChanged();
	void FontSizeCheck();

public:
	BOOL Create(CWnd* pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	inline BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID) { return Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID); }
	inline CFont *GetFont() { return (m_NewFont.GetSafeHandle() != NULL ? &m_NewFont : CDialogBar::GetFont()); }

	virtual void DrawBorders(CDC* pDC, CRect& rect);
	virtual BOOL DrawThemedGripper(CDC* pDC, const CRect& rect, BOOL fCentered);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

class CVoiceBar : public CDialogBarEx
{
	DECLARE_DYNAMIC(CVoiceBar)

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

class CTabDlgBar : public CControlBarEx
{
	DECLARE_DYNAMIC(CTabDlgBar)

public:
	CTabDlgBar();
	virtual ~CTabDlgBar();

public:
	CTabCtrlExt m_TabCtrl;
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
