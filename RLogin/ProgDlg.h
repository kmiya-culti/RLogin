#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CProgDlg ダイアログ

class CProgDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CProgDlg)

// コンストラクション
public:
	CProgDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CProgDlg)
	enum { IDD = IDD_PROGDLG };

public:
	CProgressCtrl	m_FileSize;
	CString	m_EndTime;
	CString	m_TotalSize;
	CString	m_TransRate;
	CString	m_FileName;
	CString m_Message;

public:
	int m_Div;
	LONGLONG m_LastSize;
	LONGLONG m_ResumeSize;
	clock_t m_StartClock;
	BOOL m_AbortFlag;
	class CExtSocket *m_pSock;

	void SetRange(LONGLONG max, LONGLONG rem);
	void SetPos(LONGLONG pos);
	void SetFileName(LPCTSTR file);
	void SetMessage(LPCTSTR msg);

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnCancel();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
};
