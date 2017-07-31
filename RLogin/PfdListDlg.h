#if !defined(AFX_PFDLISTDLG_H__05A17F54_1C87_49D0_AB61_2836AABF3FDC__INCLUDED_)
#define AFX_PFDLISTDLG_H__05A17F54_1C87_49D0_AB61_2836AABF3FDC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PfdListDlg.h : ヘッダー ファイル
//

#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg ダイアログ

class CPfdListDlg : public CDialog
{
// コンストラクション
public:
	class CParamTab *m_pData;
	class CServerEntry *m_pEntry;
	BOOL m_ModifiedFlag;
	void InitList();
	CPfdListDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CPfdListDlg)
	enum { IDD = IDD_PFDLISTDLG };
	CListCtrlExt	m_List;
	//}}AFX_DATA
	BOOL m_X11PortFlag;
	CString m_XDisplay;

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CPfdListDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CPfdListDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnPfdNew();
	afx_msg void OnPfdEdit();
	afx_msg void OnPfdDel();
	afx_msg void OnDblclkPfdlist(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg void OnEditDups();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUpdateItem();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_PFDLISTDLG_H__05A17F54_1C87_49D0_AB61_2836AABF3FDC__INCLUDED_)
