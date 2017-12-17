#pragma once
#include "afxwin.h"

// CIso646Dlg ダイアログ

class CIso646Dlg : public CDialogExt
{
	DECLARE_DYNAMIC(CIso646Dlg)

public:
	CIso646Dlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CIso646Dlg();

// ダイアログ データ
	enum { IDD = IDD_ISO646DLG };

public:
	CComboBox m_FontSet;
	CComboBox m_DispSet;
	CStatic m_ViewBox[24];
	CString m_CharCode[12];

	int m_CharSet;
	CString m_FontName;
	CString m_FontSetName;
	CString m_DispSetName;
	DWORD m_Iso646Tab[12];
	CFont m_Font;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnCbnSelchangeFontDispSet();
	afx_msg void OnCbnSelchangeCodeset1();
	afx_msg void OnCbnUpdateCodeset(UINT nID);
	afx_msg void OnCbnSelchangeCodeset(UINT nID);
};
