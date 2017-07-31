#if !defined(AFX_LISTCTRLEXT_H__FDEC1774_78A9_4F4E_BFA3_B54860BA49B4__INCLUDED_)
#define AFX_LISTCTRLEXT_H__FDEC1774_78A9_4F4E_BFA3_B54860BA49B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListCtrlExt.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt ウィンドウ

class CListCtrlExt : public CListCtrl
{
// コンストラクション
public:
	CListCtrlExt();

// アトリビュート
public:

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CListCtrlExt)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	int m_SortSubItem;
	int m_SortReverse;
	int m_SortDupItem;
	CMenu m_PopUpMenu;
	CMenu *m_pSubMenu;

	int GetParamItem(int para);
	int GetSelectMarkData();
	void DoSortItem();
	void InitColumn(LPCSTR lpszSection, const LV_COLUMN *lpColumn, int nMax);
	void SaveColumn(LPCSTR lpszSection);
	void SetLVCheck(WPARAM ItemIndex, BOOL bCheck);
	BOOL GetLVCheck(WPARAM ItemIndex);
	void SetPopUpMenu(UINT nIDResource, int Pos);

	virtual ~CListCtrlExt();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CListCtrlExt)
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_LISTCTRLEXT_H__FDEC1774_78A9_4F4E_BFA3_B54860BA49B4__INCLUDED_)
