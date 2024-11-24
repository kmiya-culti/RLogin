#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "Data.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

// CBlockDlg ダイアログ

class CBlockDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CBlockDlg)

public:
	CBlockDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CBlockDlg();

// ダイアログ データ
	enum { IDD = IDD_BLOCKDLG };

public:
	CListCtrlExt m_BlockList;
	CStatic m_PreViewBox;
	CScrollBar m_Scroll;

	int m_CodeSet;
	int m_FontNum;
	class CFontNode *m_pFontNode;
	class CFontTab *m_pFontTab;
	class CTextRam *m_pTextRam;
	CUniBlockTab m_UniBlockTab;
	CUniBlockTab m_UniBlockNow;
	int m_SelBlock;
	int m_ScrollPos;
	int m_ScrollMax;
	int m_ScrollPage;
	int m_WheelDelta;
	BOOL m_bSelCodeMode;
	DWORD m_SelCodeSta;
	DWORD m_SelCodeEnd;
	int m_LastSelBlock;
	CString m_DefFontName;

public:
	void InitList();
	void DrawPreView(HDC hDC);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnLvnItemchangedBlocklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawBlocklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};
