#pragma once

#include "TextRam.h"
#include "Data.h"
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CFontParaDlg ダイアログ

class CFontParaDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CFontParaDlg)
	
// コンストラクション
public:
	CFontParaDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	enum { IDD = IDD_FONTPARADLG };

public:
	BOOL	m_ShiftTemp;
	CString	m_CharSetTemp;
	CString	m_ZoomTemp[2];
	CString	m_OffsTemp[2];
	CString	m_BankTemp;
	CString	m_CodeTemp;
	CString m_IContName;
	CString m_EntryName;
	CString m_FontName;
	int m_FontNum;
	int m_FontQuality;
	int m_FontSize;
	CString m_UniBlock;
	CString m_Iso646Name[2];
	DWORD m_Iso646Tab[12];
	CString m_OverZero;

	int m_CodeSet;
	CString m_FontNameTab[16];
	class CFontNode *m_pData;
	class CFontTab *m_pFontTab;
	class CTextRam *m_pTextRam;

public:
	static int CharSetNo(LPCTSTR name);
	static LPCTSTR CharSetName(int code);
	static LPCTSTR IConvName(int code);

	int CodeSetNo(LPCTSTR bank, LPCTSTR code);
	void CodeSetName(int num, CString &bank, CString &code);
	void SetFontFace(int nID);

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// インプリメンテーション
protected:
	afx_msg void OnFontsel();
	afx_msg void OnCbnSelchangeFontnum();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCharset();
	afx_msg void OnCbnEditchangeCharset();
	afx_msg void OnBnClickedUniblock();
	afx_msg void OnBnClickedIso646set();
};
