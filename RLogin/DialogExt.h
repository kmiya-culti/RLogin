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

// クラスファンクション
public:
	void SetBackColor(COLORREF color);
	inline BOOL IsDefineFont() { return (m_FontName.IsEmpty() ? FALSE : TRUE); }
	BOOL GetSizeAndText(SIZE *pSize, CString &title);
	void AddShortCutKey(UINT MsgID, UINT KeyCode, UINT KeyWith, UINT CtrlID, WPARAM wParam);

// オーバーライド
public:
	virtual INT_PTR DoModal();
	virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	inline BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL) { return Create(ATL_MAKEINTRESOURCE(nIDTemplate), pParentWnd); }
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
};
