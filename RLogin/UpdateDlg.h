#if !defined(AFX_UPDATEDLG_H__AB223F2C_26CF_470A_949A_3FFB6683F923__INCLUDED_)
#define AFX_UPDATEDLG_H__AB223F2C_26CF_470A_949A_3FFB6683F923__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UpdateDlg.h : ヘッダー ファイル
//

#define	UDO_OVERWRITE		0
#define	UDO_MTIMECHECK		1
#define	UDO_UPDCHECK		2
#define	UDO_RESUME			3
#define	UDO_NOUPDATE		4
#define	UDO_ABORT			10

/////////////////////////////////////////////////////////////////////////////
// CUpdateDlg ダイアログ

class CUpdateDlg : public CDialog
{
// コンストラクション
public:
	int m_DoExec;
	int m_DoResume;
	CUpdateDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CUpdateDlg)
	enum { IDD = IDD_UPDATEDLG };
	CString	m_FileName;
	int		m_Jobs;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CUpdateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CUpdateDlg)
	afx_msg void OnExec();
	afx_msg void OnAllExec();
	afx_msg void OnAbort();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_UPDATEDLG_H__AB223F2C_26CF_470A_949A_3FFB6683F923__INCLUDED_)
