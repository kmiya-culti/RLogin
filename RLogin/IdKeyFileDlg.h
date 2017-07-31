#if !defined(AFX_IDKEYDLG_H__EC4DF700_9033_47DC_919B_2F324A15997C__INCLUDED_)
#define AFX_IDKEYDLG_H__EC4DF700_9033_47DC_919B_2F324A15997C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IDKeyDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg ダイアログ

class CIdKeyFileDlg : public CDialog
{
// コンストラクション
public:
	int m_OpenMode;
	CString m_Title;
	CIdKeyFileDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CIdKeyFileDlg)
	enum { IDD = IDD_IDKEYFILEDLG };
	CString	m_IdkeyFile;
	CString	m_PassName;
	CString	m_PassName2;
	CString	m_Message;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CIdKeyFileDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CIdKeyFileDlg)
	afx_msg void OnIdkeysel();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_IDKEYDLG_H__EC4DF700_9033_47DC_919B_2F324A15997C__INCLUDED_)
