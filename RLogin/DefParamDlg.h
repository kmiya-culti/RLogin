#pragma once

#include "DialogExt.h"

// CDefParamDlg ダイアログ

class CDefParamDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CDefParamDlg)

public:
	CDefParamDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CDefParamDlg();

// ダイアログ データ
	enum { IDD = IDD_DEFPARAMDLG };

public:
	int m_InitFlag;
	BOOL m_Check[6];
	CStatic m_MsgIcon;
	class CTextRam *m_pTextRam;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
};
