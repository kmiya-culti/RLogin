#pragma once

#include "DialogExt.h"
#include "afxcmn.h"
#include "afxwin.h"

// CResTransDlg ダイアログ

class CResTransDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CResTransDlg)

public:
	CResTransDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CResTransDlg();

// ダイアログ データ
	enum { IDD = IDD_RESTRANSDLG };

public:
	CString m_ClientId;
	CString m_ClientSecret;
	CString m_TransFrom;
	CString m_TransTo;
	CProgressCtrl m_TransProgress;
	CButton m_TransExec;

	CResDataBase m_ResDataBase;
	CTranslateStringTab m_TransStrTab;
	CString m_ResFileName;
	int m_TransMax;
	int m_TransPos;
	int m_TransNext;
	CString m_MsToken;

	enum {  TRANSPROC_START,
			TRANSPROC_TOKENREQ, TANSPROC_TOKENRESULT, TANSPROC_TRANSREQ, TANSPROC_TRANSRESULT, 
			TRANSPROC_GOOGLEREQ, TANSPROC_GOOGLERESULT,
			TANSPROC_PAUSE, TANSPROC_RETRY, TRANSPROC_WAIT, TRANSPROC_ENDOF };

	int m_ProcStat;
	BOOL m_bProcAbort;

	BOOL TransErrMsg(CString *pMsg, LPCTSTR errMsg);
	void TranslateProc(UINT msg, WPARAM wParam, LPARAM lParam);
	BOOL IsTranslateProcExec();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedTransexec();
	afx_msg void OnNMClickSyslink1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnHttpMessage(WPARAM wParam, LPARAM lParam);
};
