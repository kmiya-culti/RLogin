#pragma once

#include "afxcmn.h"
#include "afxwin.h"
#include "DialogExt.h"

// CScriptDlg ダイアログ

class CScriptDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CScriptDlg)

public:
	CScriptDlg(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~CScriptDlg();

// ダイアログ データ
	enum { IDD = IDD_SCRIPTDLG };

public:
	class CScript *m_pScript;
	CString m_FileName;
	CEdit m_Text;
	CComboBox m_CodeStack;
	LPCTSTR m_pSrc;
	int m_Max;
	int m_SPos;
	int m_EPos;

	void UpdatePos(int SPos, int EPos);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual void OnCancel();

protected:
	afx_msg void OnClose();
	afx_msg void OnBnClickedContinue();
	DECLARE_MESSAGE_MAP()
};
