#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CProgressWnd

class CProgressWnd : public CWnd
{
	DECLARE_DYNAMIC(CProgressWnd)

public:
	BOOL m_bFinish;
	BOOL m_bNoRange;
	int m_RangeLower;
	int m_RangeUpper;
	int m_RangePos;

	int m_RateMax;
	int m_RateLast;
	int m_LastPos;

	int m_DataMax;
	BYTE *m_pDataTab;

public:
	CProgressWnd();
	~CProgressWnd();

	BOOL InitDataTab(BOOL bInit);

	void SetRange(short nLower, short nUpper);
	void SetRange32(int nLower, int nUpper);

	void GetRange(int &nLower, int &nUpper);

	int GetPos();
	int SetPos(int nPos, int nRate = 0);

	inline void SetFinish() { m_bFinish = TRUE; }

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
};

/////////////////////////////////////////////////////////////////////////////
// CProgDlg ダイアログ

#define	PROG_UPDATE_RANGE		0
#define	PROG_UPDATE_POS			1
#define	PROG_UPDATE_FILENAME	2
#define	PROG_UPDATE_MESSAGE		3
#define	PROG_UPDATE_DESTORY		4

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
//	CProgressCtrl m_FileSize;
	CProgressWnd m_FileSize;
	CString	m_EndTime;
	CString	m_TotalSize;
	CString	m_TransRate;
	CString	m_FileName;
	CString m_Message;

public:
	int m_Div;
	LONGLONG m_LastSize;
	LONGLONG m_ResumeSize;
	LONGLONG m_UpdatePos, m_LastPos, m_ActivePos;
	clock_t m_StartClock, m_LastClock, m_UpdateClock;
	int m_LastRate;
	BOOL *m_pAbortFlag;
	BOOL m_UpdatePost;
	BOOL m_AutoDelete;
	class CExtSocket *m_pSock;
	class CRLoginDoc *m_pDocument;

	void SetRange(LONGLONG max, LONGLONG rem);
	void UpdatePos(LONGLONG pos);
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
	afx_msg void OnNcDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnProgUpdate(WPARAM wParam, LPARAM lParam);
};
