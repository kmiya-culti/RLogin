#pragma once

#include "DialogExt.h"
#include "afxwin.h"

// CComMoniDlg ダイアログ

#define	PORTNUM_DCD			0
#define	PORTNUM_RxD			1
#define	PORTNUM_TxD			2
#define	PORTNUM_DTR			3
#define	PORTNUM_GND			4
#define	PORTNUM_DSR			5
#define	PORTNUM_RTS			6
#define	PORTNUM_CTS			7
#define	PORTNUM_RI			8

#define	PORTCOLOR_OFF		0
#define	PORTCOLOR_OUTON		1
#define	PORTCOLOR_INON		2
#define	PORTCOLOR_TOGGLE	3
#define	PORTCOLOR_OUTCTRL	4

#define	GRAPDATA_MAX		180

typedef struct _GRAPDATA {
	DWORD ByteSec[2];
	DWORD ModSts[2];
} GRAPDATA;

class CComMoniDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CComMoniDlg)

public:
	CComMoniDlg(class CComSock *pSock, CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CComMoniDlg();

// ダイアログ データ
	enum { IDD = IDD_COMMONIDLG };

public:
	class CComSock *m_pSock;
	BOOL m_bActive;
	CButton m_ErrBtn[6];
	CButton m_ComConfig;
	CButton m_ComCtrlDtr;
	CButton m_ComCtrlRts;
	CStatic m_ColBox[9];
	CStatic m_GrapBox;
	CString m_BaudRate;
	CString m_DataBits;
	CString m_ParityBit;
	CString m_StopBits;
	int m_FlowCtrl;
	CString m_UserDef;
	int m_SendWait[3];
	WORD m_XoffLim;
	WORD m_XonLim;
	char m_XonChar;
	char m_XoffChar;
	int m_PortColor[9];
	int m_RecvByte;
	int m_SendByte;
	DWORD m_ModStsLast;
	DWORD m_ModStsXor;
	DWORD m_ModStsDisp;
	DWORD m_LastError;

	int m_DataTop;
	GRAPDATA *m_pData;

	void SetCommError();
	void SetModemStatus();
	void DrawGrapData(CDC *pDC, CRect &rect);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnCommEvent();
	afx_msg void OnBnClickedComconfig();
	afx_msg void OnBnClickedClearerror();
	afx_msg void OnBnClickedCtrlDtr();
	afx_msg void OnBnClickedCtrlRts();
	afx_msg void OnCbnSelchange();
	afx_msg void OnBnClickedUserflow();
};
