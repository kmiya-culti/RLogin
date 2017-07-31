#pragma once

/////////////////////////////////////////////////////////////////////////////
// CColParaDlg ダイアログ

class CColParaDlg : public CTreePropertyPage
{
	DECLARE_DYNCREATE(CColParaDlg)

// コンストラクション
public:
	CColParaDlg();
	~CColParaDlg();

// ダイアログ データ
	enum { IDD = IDD_COLORPARADLG };

public:
	CSliderCtrl	m_TransSlider;
	int		m_ColSet;
	CString	m_BitMapFile;
	int		m_WakeUpSleep;
	CStatic	m_ColBox[18];
	int m_FontCol[2];
	CString m_FontColName[2];
	COLORREF m_ColTab[16];
	BOOL m_Attrb[24];
	BOOL m_GlassStyle;

	CImageList m_ImageList[8];

public:
	void DoInit();

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

// インプリメンテーション
protected:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnReleasedcaptureTransparent(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelendokColset();
	afx_msg void OnBitMapFileSel();
	afx_msg void OnUpdateEdit();
	afx_msg void OnEnChangeColor();
	afx_msg void OnBnClickedGlassStyle();
	afx_msg void OnUpdateCheck(UINT nId);
	DECLARE_MESSAGE_MAP()
};
