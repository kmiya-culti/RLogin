#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CScriptDlg ダイアログ

class CScriptDlg : public CDialog
{
	DECLARE_DYNAMIC(CScriptDlg)

public:
	CScriptDlg(CWnd* pParent = NULL);   // 標準コンストラクタ
	virtual ~CScriptDlg();

// ダイアログ データ
	enum { IDD = IDD_SCRIPTDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
public:
	CString m_FileName;
	CEdit m_Text;
	LPBYTE m_pSrc;
	int m_Max;
	int m_SPos;
	int m_EPos;

	void UpdatePos(int SPos, int EPos);
	virtual BOOL OnInitDialog();
};
