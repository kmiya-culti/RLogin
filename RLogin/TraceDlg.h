#pragma once

#include "afxcmn.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"
#include "afxwin.h"

//////////////////////////////////////////////////////////////////////
// CTraceDlg ダイアログ

#define	TRACE_SAVE	20

class CTraceDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CTraceDlg)

public:
	CTraceDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CTraceDlg();

// ダイアログ データ
	enum { IDD = IDD_TRACEDLG };

public:
	CListCtrlExt m_List;
	CString m_Title;
	class CRLoginDoc *m_pDocument;
	class CTextSave *m_pSave;
	CFileExt m_File;
	CString m_TraceLogFile;
	int m_TraceMaxCount;

	void AddTraceNode(CTraceNode *pNode);
	void SetString(int num, CString &str);
	void WriteString(int num);
	void DeleteAllItems();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual void OnCancel();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMKillfocusList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTekClose();
	afx_msg void OnTekSave();
	afx_msg void OnUpdateTekSave(CCmdUI *pCmdUI);
	afx_msg void OnTekClear();
	afx_msg void OnEditCopy();
};

//////////////////////////////////////////////////////////////////////
// CCmdHisDlg ダイアログ

class CCmdHisDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CCmdHisDlg)

public:
	CCmdHisDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CCmdHisDlg();

// ダイアログ データ
	enum { IDD = IDD_TRACEDLG };

public:
	CListCtrlExt m_List;
	class CRLoginDoc *m_pDocument;
	CString m_Title;
	CFileExt m_File;
	BOOL m_bScrollMode;

	void SetCmdHis(int num, CMDHIS *pCmdHis);
	void SetExitStatus(CMDHIS *pCmdHis);
	void DelCmdHis(CMDHIS *pCmdHis);
	void InitList();
	void SetString(int num, CString &str);

	static LPCTSTR ExecTimeString(CMDHIS *pCmdHis);
	static void InfoExitStatus(CMDHIS *pCmdHis);
	static LPCTSTR ShellEscape(LPCTSTR str);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnTekClose();
	afx_msg void OnTekSave();
	afx_msg void OnUpdateTekSave(CCmdUI *pCmdUI);
	afx_msg void OnTekClear();
	afx_msg void OnEditCopy();
	afx_msg void OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCmdhisCmds();
	afx_msg void OnUpdateCmdhisCmds(CCmdUI *pCmdUI);
	afx_msg void OnCmdhisCurd();
	afx_msg void OnUpdateCmdhisCurd(CCmdUI *pCmdUI);
	afx_msg void OnCmdhisMsg();
	afx_msg void OnUpdateCmdhisMsg(CCmdUI *pCmdUI);
	afx_msg void OnCmdhisScrm();
	afx_msg void OnUpdateCmdhisScrm(CCmdUI *pCmdUI);
	afx_msg LRESULT OnAddCmdHis(WPARAM wParam, LPARAM lParam);
};

//////////////////////////////////////////////////////////////////////
// CHistoryDlg ダイアログ

#define	HISBOX_CMDS		0
#define	HISBOX_CDIR		1
#define	HISBOX_CLIP		2

class CHistoryDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CHistoryDlg)

public:
	CHistoryDlg(CWnd* pParent = NULL);
	virtual ~CHistoryDlg();

// ダイアログ データ
	enum { IDD = IDD_HISTORYDLG };

public:
	CStatic m_TitleBox[3];
	CListCtrl m_ListBox[3];
	CRect m_Boder;
	BOOL m_bHorz;
	BOOL m_bTrackMode;
	CRect m_TrackRect;
	CRect m_TrackBase;
	CRect m_TrackRange;
	CPoint m_TrackPoint;
	int m_TrackPos;
	int m_PaneSize[4];
	CRect m_SizeRect[2];
	CRect m_SizeRange[2];

	void Add(int nList, LPCTSTR str);
	void OffsetTrack(CPoint point);
	void InvertTracker(CRect &rect);
	void ReSize(int cx, int cy);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual void PostNcDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnNMDblclkListCtrl(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnAddCmdHis(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
