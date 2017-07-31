#if !defined(AFX_PROTOPAGE_H__E5EE664E_A0AF_4FF7_ADDD_49EC824E1D55__INCLUDED_)
#define AFX_PROTOPAGE_H__E5EE664E_A0AF_4FF7_ADDD_49EC824E1D55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProtoPage.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CProtoPage ダイアログ

class CProtoPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CProtoPage)

// コンストラクション
public:
	class COptDlg *m_pSheet;
	CParamTab ParamTabTemp;
	CProtoPage();
	~CProtoPage();

// ダイアログ データ
	//{{AFX_DATA(CProtoPage)
	enum { IDD = IDD_PROTOPAGE };
	UINT	m_DelayMsec;
	UINT	m_KeepAlive;
	//}}AFX_DATA
	BOOL m_Check[18];


// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CProtoPage)
	public:
	virtual BOOL OnApply();
	virtual void OnReset();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CProtoPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSshAlgo();
	afx_msg void OnSshIdkey();
	afx_msg void OnSshPfd();
	//}}AFX_MSG
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_PROTOPAGE_H__E5EE664E_A0AF_4FF7_ADDD_49EC824E1D55__INCLUDED_)
