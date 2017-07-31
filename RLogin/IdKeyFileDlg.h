#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg ダイアログ

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
	CString	m_PassName;
	CString	m_PassName2;
	CStringLoad	m_Message;

	int m_OpenMode;
	CStringLoad m_Title;

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnIdkeysel();
	DECLARE_MESSAGE_MAP()
};
