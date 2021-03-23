#pragma once

#include <afxtempl.h>

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

#define	ITM_LEFT_MID	00001
#define	ITM_LEFT_RIGHT	00002
#define	ITM_RIGHT_MID	00010
#define	ITM_RIGHT_RIGHT	00020
#define	ITM_TOP_MID		00100
#define	ITM_TOP_BTM		00200
#define	ITM_BTM_MID		01000
#define	ITM_BTM_BTM		02000

#define ITM_PER_MAX		2000

typedef struct	_InitDlgTab {
	UINT	id;
	int		mode;
} INITDLGTAB;

typedef struct	_InitDlgRect {
	UINT	id;
	int		mode;
	RECT	rect;
} INITDLGRECT;

//////////////////////////////////////////////////////////////////////
// CDialogExt ダイアログ

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
	CBrush m_BkBrush;
	CString m_FontName;
	int m_FontSize;
	CArray<CShortCutKey, CShortCutKey &> m_Data;
	CArray<INITDLGRECT, INITDLGRECT &> m_InitDlgRect;
	CSize m_MinSize, m_OfsSize;
	CSize m_InitDpi, m_NowDpi;
	CSize m_ZoomMul, m_ZoomDiv;
	CFont m_DpiFont;
	CPtrArray m_ComboBoxPtr;

// クラスファンクション
public:
	void InitItemOffset(const INITDLGTAB *pTab, int ox = 0, int oy = 0, int mx = 0, int my = 0);
	virtual void SetItemOffset(int cx, int cy);
	void SetBackColor(COLORREF color);
	inline BOOL IsDefineFont() { return (m_FontName.IsEmpty() ? FALSE : TRUE); }
	BOOL GetSizeAndText(SIZE *pSize, CString &title, CWnd *pParent);
	void AddShortCutKey(UINT MsgID, UINT KeyCode, UINT KeyWith, UINT CtrlID, WPARAM wParam);
	void SubclassComboBox(int nID);
	void CheckMoveWindow(CRect &rect, BOOL bRepaint);

	static void GetActiveDpi(CSize &dpi, CWnd *pWnd, CWnd *pParent);
	static BOOL IsDialogExt(CWnd *pWnd);

// オーバーライド
public:
	virtual INT_PTR DoModal();
	virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	inline BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL) { return Create(ATL_MAKEINTRESOURCE(nIDTemplate), pParentWnd); }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);
	afx_msg void OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex,BOOL bSysMenu);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
};
