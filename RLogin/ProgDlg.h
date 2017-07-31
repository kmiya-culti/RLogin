#if !defined(AFX_PROGDLG_H__04AEE5BF_73E6_479A_9221_42DC9C407DBA__INCLUDED_)
#define AFX_PROGDLG_H__04AEE5BF_73E6_479A_9221_42DC9C407DBA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProgDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CProgDlg ダイアログ

class CProgDlg : public CDialog
{
// コンストラクション
public:
	int m_Div;
	LONGLONG m_LastSize;
	LONGLONG m_ResumeSize;
	clock_t m_StartClock;
	BOOL m_AbortFlag;
	class CExtSocket *m_pSock;

	void SetRange(LONGLONG max, LONGLONG rem);
	void SetPos(LONGLONG pos);
	void SetFileName(LPCSTR file);
	void SetMessage(LPCSTR msg);

	CProgDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CProgDlg)
	enum { IDD = IDD_PROGDLG };
	CProgressCtrl	m_FileSize;
	CString	m_EndTime;
	CString	m_TotalSize;
	CString	m_TransRate;
	CString	m_FileName;
	CString m_Message;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CProgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CProgDlg)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_PROGDLG_H__04AEE5BF_73E6_479A_9221_42DC9C407DBA__INCLUDED_)
