#pragma once

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
	DECLARE_DYNAMIC(CUpdateDlg)

// コンストラクション
public:
	CUpdateDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_UPDATEDLG };

public:
	CString	m_FileName;
	int		m_Jobs;

	int m_DoExec;
	int m_DoResume;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// インプリメンテーション
protected:
	afx_msg void OnExec();
	afx_msg void OnAllExec();
	afx_msg void OnAbort();
	DECLARE_MESSAGE_MAP()
};
