#pragma once

#include "DialogExt.h"
#include "afxwin.h"
#include "afxcmn.h"

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
	CProgressCtrl m_HashProg;

	CIdKeyTab m_IdKeyTab;
	CIdKey m_TempKey;
	CIdKey *m_pKey;
	CIdKey m_VerifyKey;
	CBuffer m_VerifySign;
	CBuffer m_VerifyData;

	CComboBoxHis m_DataFileCombo;
	CComboBoxHis m_KeyListCombo;
	CComboBoxHis m_KeyFileCombo;
	CComboBoxHis m_NameSpaceCombo;

	int m_ThreadStat;
	CEvent m_ThreadEvent;
	FILE *m_HashFp;
	EVP_MD_CTX *m_HashCtx;

	UINT_PTR m_HashTimer;
	LONGLONG m_HashFileSize;
	LONGLONG m_HashFilePos;
	int m_HashFileDivs;

	CIdKey *LoadKey();
	BOOL ThreadCheck();
	FILE *FileOpen(LPCTSTR filename);
	void SaveProfile();

public:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnVerify();
	afx_msg void OnSignNext();
	afx_msg void OnVerifyNext();
	afx_msg void OnDatafileSel();
	afx_msg void OnKeyfileSel();
	afx_msg void OnSignInport();
	afx_msg void OnSignExport();
	afx_msg void OnSignCopy();
	afx_msg void OnSignPast();
	afx_msg void OnKeyMode(UINT nID);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
