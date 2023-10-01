#if !defined(AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_)
#define AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlgoDlg.h : ヘッダー ファイル
//
#include "ListCtrlExt.h"
#include "DialogExt.h"
#include "Data.h"

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg ダイアログ

class CAlgoDlg : public CDialogExt
{
// コンストラクション
public:
	CAlgoDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_ALGOPARADLG };

public:
	CListCtrlExt m_List[12];
	CStringArrayExt m_AlgoTab[12];
	BOOL m_EncShuffle;
	BOOL m_MacShuffle;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedReset();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // !defined(AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_)
