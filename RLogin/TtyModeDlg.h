#pragma once

#include "afxcmn.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CTtyModeDlg

class CTtyModeDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CTtyModeDlg)

public:
	CTtyModeDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CTtyModeDlg();

// ダイアログ データ
	enum { IDD = IDD_TTYMODEDLG };

public:
	CListCtrlExt m_List;
	class CParamTab *m_pParamTab;
	int m_MinWidth;
	int m_MinHeight;

	void InitList();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
};

/////////////////////////////////////////////////////////////////////////////
// CColEditDlg

class CColEditDlg : public CTtyModeDlg
{
	DECLARE_DYNAMIC(CColEditDlg)

public:
	CColEditDlg();
	virtual ~CColEditDlg();

	COLORREF m_ColTab[16];

public:
	virtual void InitList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
};
