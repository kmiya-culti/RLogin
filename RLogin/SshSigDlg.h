#pragma once

#include "DialogExt.h"
#include "afxwin.h"

// CSshSigDlg ダイアログ

class CSshSigDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CSshSigDlg)

public:
	CSshSigDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CSshSigDlg();

	enum { IDD = IDD_SSHSIGDLG };

public:
	CString m_DataFile;
	int m_KeyList;
	CString m_KeyFile;
	CString m_NameSpace;
	CString m_Sign;
	int m_KeyMode;
	BOOL m_StrictVerify;
	CIdKeyTab m_IdKeyTab;
	CIdKey m_TempKey;

	CComboBoxHis m_DataFileCombo;
	CComboBoxHis m_KeyListCombo;
	CComboBoxHis m_KeyFileCombo;
	CComboBoxHis m_NameSpaceCombo;

	CIdKey *LoadKey();

public:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnVerify();
	afx_msg void OnDatafileSel();
	afx_msg void OnKeyfileSel();
	afx_msg void OnSignInport();
	afx_msg void OnSignExport();
	afx_msg void OnSignCopy();
	afx_msg void OnSignPast();
	afx_msg void OnKeyMode(UINT nID);
};
