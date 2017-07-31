#if !defined(AFX_IDKEYSELDLG_H__EDF57990_40BB_4B33_92B1_28A625E046D4__INCLUDED_)
#define AFX_IDKEYSELDLG_H__EDF57990_40BB_4B33_92B1_28A625E046D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IdkeySelDLg.h : ヘッダー ファイル
//

#include <afxmt.h>
#include <afxtempl.h>
#include "Data.h"
#include "ListCtrlExt.h"

/////////////////////////////////////////////////////////////////////////////
// CIdkeySelDLg ダイアログ

class CIdkeySelDLg : public CDialog
{
// コンストラクション
public:
	CIdKeyTab *m_pIdKeyTab;
	CParamTab *m_pParamTab;
	CWordArray m_Data;
	int m_EntryNum;
	CEvent *m_pKeyGenEvent;
	int m_KeyGenFlag;
	UINT m_GenIdKeyTimer;
	int m_GenIdKeyType;
	int m_GenIdKeyBits;
	BOOL m_GenIdKeyStat;
	int m_GenIdKeyMax;
	int m_GenIdKeyCount;
	CIdKey m_GenIdKey;
	CString m_OldIdKeyList;
	BOOL m_ListInit;

	void StartKeyGenThead();
	void ProcKeyGenThead();
	void EndofKeyGenThead();
	void OnKeyGenEndof();

	void InitList();
	void CopyToClipBorad(LPCSTR str);
	CIdkeySelDLg(CWnd* pParent = NULL);   // 標準のコンストラクタ
	~CIdkeySelDLg();

// ダイアログ データ
	//{{AFX_DATA(CIdkeySelDLg)
	enum { IDD = IDD_IDKEYSELDLG };
	CProgressCtrl	m_KeyGenProg;
	CListCtrlExt m_List;
	CString	m_Type;
	CString	m_Bits;
	CString	m_Name;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CIdkeySelDLg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CIdkeySelDLg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIdkeyUp();
	afx_msg void OnIdkeyDown();
	afx_msg void OnIdkeyDel();
	afx_msg void OnIdkeyInport();
	afx_msg void OnIdkeyExport();
	afx_msg void OnIdkeyCreate();
	afx_msg void OnDblclkIdkeyList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg void OnIdkeyCopy();
	afx_msg void OnEditUpdate();
	afx_msg void OnEditCheck();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnItemchangedIdkeyList(NMHDR *pNMHDR, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_IDKEYSELDLG_H__EDF57990_40BB_4B33_92B1_28A625E046D4__INCLUDED_)
