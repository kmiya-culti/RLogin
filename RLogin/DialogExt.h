#pragma once

#include <afxtempl.h>
#include "UahMenuBar.h"

class CShortCutKey : public CObject
{
public:
	UINT m_MsgID;
	UINT m_KeyCode;
	UINT m_KeyWith;
	UINT m_CtrlID;
	WPARAM m_wParam;

	const CShortCutKey & operator = (CShortCutKey &data);

	CShortCutKey();
	~CShortCutKey();
};

#define	ITM_LEFT_LEFT	00000		// 固定
#define	ITM_LEFT_MID	00001		// 左右中心から
#define	ITM_LEFT_RIGHT	00002		// 右端から
#define	ITM_LEFT_PER	00003		// 横幅の相対値
#define	ITM_LEFT_OFS	00004		// 右から固定
#define	ITM_RIGHT_LEFT	00000
#define	ITM_RIGHT_MID	00010
#define	ITM_RIGHT_RIGHT	00020
#define	ITM_RIGHT_PER	00030
#define	ITM_RIGHT_OFS	00040
#define	ITM_TOP_TOP		00000
#define	ITM_TOP_MID		00100
#define	ITM_TOP_BTM		00200
#define	ITM_TOP_PER		00300
#define	ITM_TOP_OFS		00400
#define	ITM_BTM_TOP		00000
#define	ITM_BTM_MID		01000
#define	ITM_BTM_BTM		02000
#define	ITM_BTM_PER		03000
#define	ITM_BTM_OFS		04000

#define ITM_PER_MAX		1000

typedef struct	_InitDlgTab {
	UINT	id;
	int		mode;
} INITDLGTAB;

typedef struct	_InitDlgRect {
	UINT	id;
	HWND    hWnd;
	int		mode;
	RECT	rect;
} INITDLGRECT;

class CSizeGrip : public CScrollBar
{
    DECLARE_DYNAMIC(CSizeGrip)

public:
	CSizeGrip();
    virtual ~CSizeGrip();

public:
    BOOL Create(CWnd *pParentWnd, int cx, int cy);
	void ParentReSize(int cx, int cy);

public:
	DECLARE_MESSAGE_MAP()
	BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message);
};

//////////////////////////////////////////////////////////////////////
// CDialogExt ダイアログ

#define	GRIP_SIZE_CX		MulDiv(11, m_NowDpi.cx, DEFAULT_DPI_X)
#define	GRIP_SIZE_CY		MulDiv(11, m_NowDpi.cy, DEFAULT_DPI_Y)

#define	LOADPOS_NONE		0
#define	LOADPOS_PROFILE		1
#define	LOADPOS_PARENT		2
#define	LOADPOS_MAINWND		3

class CDialogExt : public CDialog
{
	DECLARE_DYNAMIC(CDialogExt)

// コンストラクター
public:
	CDialogExt(UINT nIDTemplate, CWnd* pParent = NULL);
	virtual ~CDialogExt();

// クラスデータ
public:
	UINT m_nIDTemplate;
	BOOL m_bBackWindow;
	CBrush m_DarkBrush;
	CString m_FontName;
	int m_FontSize;
	CArray<CShortCutKey, CShortCutKey &> m_Data;
	CArray<INITDLGRECT, INITDLGRECT &> m_InitDlgRect;
	CSize m_MaxSize, m_MinSize, m_OfsSize;
	CSize m_InitDpi, m_NowDpi;
	CSize m_ZoomMul, m_ZoomDiv;
	CFont m_DpiFont;
	CPtrArray m_ComboBoxPtr;
	CSizeGrip m_SizeGrip;
	BOOL m_bGripDisable;
	BOOL m_bReSizeDisable;
	CString m_SaveProfile;
	int m_LoadPosMode;
	BOOL m_bDarkMode;

// クラスファンクション
public:
	void DefItemOffset();
	void InitItemOffset(const INITDLGTAB *pTab, int ox = 0, int oy = 0, int nx = 0, int ny = 0, int mx = 0, int my = 0);
	virtual void SetItemOffset(int cx, int cy);
	inline void SetBackWindow(BOOL sw) { m_bBackWindow = sw; }
	inline BOOL IsDefineFont() { return (m_FontName.IsEmpty() && m_FontSize == 9 ? FALSE : TRUE); }
	BOOL GetSizeAndText(SIZE *pSize, CString &title, CWnd *pParent);
	void AddShortCutKey(UINT MsgID, UINT KeyCode, UINT KeyWith, UINT CtrlID, WPARAM wParam);
	void SubclassComboBox(int nID);
	void CheckMoveWindow(CRect &rect, BOOL bRepaint);
	inline void CheckMoveWindow(int sx, int sy, int cx, int cy, BOOL bRepaint) { CheckMoveWindow(CRect(sx, sy, sx + cx, sy + cy), bRepaint); }
	inline void SetSaveProfile(LPCTSTR name) { m_SaveProfile = name; }
	inline void SetLoadPosition(int mode) { m_LoadPosMode = mode; }
	virtual void LoadSaveDialogSize(BOOL bSave);
	inline void SizeGripDisable(BOOL bDisable) { m_bGripDisable = bDisable; }
	inline void SetReSizeDisable(BOOL bDisable) { m_bReSizeDisable = bDisable; }

	static void GetActiveDpi(CSize &dpi, CWnd *pWnd, CWnd *pParent);
	static BOOL IsDialogExt(CWnd *pWnd);
	static void GetDlgFontBase(CDC *pDC, CFont *pFont, CSize &size);

// オーバーライド
public:
	virtual INT_PTR DoModal();
	virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	inline BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL) { return Create(ATL_MAKEINTRESOURCE(nIDTemplate), pParentWnd); }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex,BOOL bSysMenu);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);
	afx_msg LRESULT OnUahDrawMenu(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUahDrawMenuItem(WPARAM wParam, LPARAM lParam);
};
