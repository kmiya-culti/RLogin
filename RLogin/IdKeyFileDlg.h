#pragma once

#include "DialogExt.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg ダイアログ

#define	IDKFDMODE_LOAD		1
#define	IDKFDMODE_SAVE		2
#define	IDKFDMODE_CREATE	3
#define	IDKFDMODE_OPEN		4

class CIdKeyFileDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CIdKeyFileDlg)

// コンストラクション
public:
	CIdKeyFileDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_IDKEYFILEDLG };

public:
	CString	m_IdkeyFile;
	CStringLoad	m_Message;
	CString	m_PassName;
	BOOL m_bPassDisp;

	int m_OpenMode;
	CStringLoad m_Title;
	TCHAR m_PassChar;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnIdkeysel();
	afx_msg void OnBnClickedPassdisp();
	DECLARE_MESSAGE_MAP()
};
