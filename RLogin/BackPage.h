#pragma once

/////////////////////////////////////////////////////////////////////////////
// CBackPage �_�C�A���O

class CBackPage : public CTreePage
{
	DECLARE_DYNAMIC(CBackPage)

public:
	CBackPage();
	virtual ~CBackPage();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_BACKPAGE };

public:
	CString	m_BitMapFile;
	BOOL m_bEnable;
	int m_HAlign;
	int m_VAlign;
	int m_MapStyle;
	CString m_TextFormat;
	CSliderCtrl m_BitMapAlpha;
	CSliderCtrl m_BitMapBlend;
	CStatic m_ColorBox;
	COLORREF m_TextColor;
	LOGFONT m_LogFont;
	CStatic m_TabTextColorBox;
	CStatic m_TabBackColorBox;
	COLORREF m_TabTextColor;
	COLORREF m_TabBackColor;
	BOOL m_TabBackGradient;

public:
	void DoInit();

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBitMapFileSel();
	afx_msg void OnBnClickedTextfont();
	afx_msg void OnStnClickedTextcolor();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnUpdateEdit();
	afx_msg void OnUpdateSlider(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnClickedTabTextColor();
	afx_msg void OnStnClickedTabBackColor();
};
