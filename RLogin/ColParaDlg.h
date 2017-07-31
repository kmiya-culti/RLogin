#if !defined(AFX_COLPARADLG_H__B7BC0C15_B28B_47B6_847C_98B453BC8506__INCLUDED_)
#define AFX_COLPARADLG_H__B7BC0C15_B28B_47B6_847C_98B453BC8506__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColParaDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// CColParaDlg ダイアログ

class CColParaDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CColParaDlg)

// コンストラクション
public:
	class COptDlg *m_pSheet;
	COLORREF m_ColTab[16];
	BOOL m_Attrb[24];
	CImageList m_ImageList[8];

	CColParaDlg();
	~CColParaDlg();

// ダイアログ データ
	//{{AFX_DATA(CColParaDlg)
	enum { IDD = IDD_COLORPARADLG };
	CSliderCtrl	m_TransSlider;
	int		m_ColSet;
	CString	m_BitMapFile;
	int		m_WakeUpSleep;
	//}}AFX_DATA
	CStatic	m_ColBox[18];
	int m_FontCol[2];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CColParaDlg)
	public:
	virtual BOOL OnApply();
	virtual void OnReset();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CColParaDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnReleasedcaptureTransparent(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelendokColset();
	afx_msg void OnBitMapFileSel();
	//}}AFX_MSG
	afx_msg void OnUpdateEdit();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_COLPARADLG_H__B7BC0C15_B28B_47B6_847C_98B453BC8506__INCLUDED_)
