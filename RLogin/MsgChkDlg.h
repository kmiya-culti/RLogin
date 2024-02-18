#pragma once

#include "afxwin.h"
#include "DialogExt.h"

// CMsgChkDlg �_�C�A���O

class CMsgChkDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CMsgChkDlg)

public:
	CMsgChkDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[
	virtual ~CMsgChkDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MSGDLG };

public:
	CStatic m_IconWnd;
	CStatic m_MsgWnd;
	CStatic m_BackWnd;
	BOOL m_bNoCheck;

	UINT m_nType;
	CWnd *m_pParent;
	BOOL m_bNoChkEnable;
	CString m_Title;
	CString m_MsgText;
	int m_BtnRes[3];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnCancel();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
};
