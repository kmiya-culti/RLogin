#if !defined(AFX_PFDDLG_H__0088D685_A5BA_46E4_BACC_95E3762F079C__INCLUDED_)
#define AFX_PFDDLG_H__0088D685_A5BA_46E4_BACC_95E3762F079C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PfdDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CPfdParaDlg ダイアログ

class CPfdParaDlg : public CDialog
{
// コンストラクション
public:
	int m_EntryNum;
	class CParamTab *m_pData;
	class CServerEntry *m_pEntry;
	CPfdParaDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CPfdParaDlg)
	enum { IDD = IDD_PFDPARADLG };
	CString	m_ListenHost;
	CString	m_ListenPort;
	CString	m_ConnectHost;
	CString	m_ConnectPort;
	//}}AFX_DATA


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CPfdParaDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CPfdParaDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_PFDDLG_H__0088D685_A5BA_46E4_BACC_95E3762F079C__INCLUDED_)
