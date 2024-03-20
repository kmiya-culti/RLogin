#pragma once

#include "DialogExt.h"

// CComInitDlg ダイアログ

#define	COMMPROP_TYPE_BAUD		0
#define	COMMPROP_TYPE_DATABITS	1
#define	COMMPROP_TYPE_PARITY	2
#define	COMMPROP_TYPE_STOPBITS	3

extern const LPCTSTR ParityBitsName[];
extern const LPCTSTR StopBitsName[];

class CComInitDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CComInitDlg)

public:
	CComInitDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CComInitDlg();

// ダイアログ データ
	enum { IDD = IDD_COMINITDLG };

public:
	class CComSock *m_pSock;
	int m_ComIndex;
	CString m_BaudRate;
	CString m_DataBits;
	CString m_ParityBit;
	CString m_StopBits;
	int m_FlowCtrl;
	CString m_UserDef;
	WORD m_XoffLim;
	WORD m_XonLim;
	char m_XonChar;
	char m_XoffChar;
	int m_SendWait[2];
	CStringIndex m_ComDevList;

	static void CommPropCombo(int Type, CComboBox *pCombo, COMMPROP *pCommProp);

	void CommPropCheck();
	BOOL CComInitDlg::GetComDeviceList(LPCTSTR name);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedUserflow();
	afx_msg void OnBnClickedDefconf();
	afx_msg void OnCbnSelchangeComdevlist();
};
