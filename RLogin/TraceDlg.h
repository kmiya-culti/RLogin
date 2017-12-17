#pragma once

#include "afxcmn.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

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
	afx_msg void OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex,BOOL bSysMenu);
};
