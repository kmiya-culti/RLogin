#pragma once

#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CPassDlg �_�C�A���O

#define	PASSDLG_HOST	001
#define	PASSDLG_USER	002
#define	PASSDLG_PASS	004

class CPassDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CPassDlg)

// �R���X�g���N�V����
public:
	CPassDlg(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	enum { IDD = IDD_PASSDLG };

public:
	CProgressCtrl m_TimeLimit;
	CComboBoxHis m_HostWnd;
	CComboBoxHis m_PortWnd;
	CComboBoxHis m_UserWnd;
	CEdit m_PassWnd;
	CString m_HostAddr;
	CString m_PortName;
	CString	m_UserName;
	CString	m_PassName;
	CString	m_Prompt;
	BOOL m_PassEcho;

	CString m_Title;
	int m_Counter;
	int m_MaxTime;
	int m_Enable;

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// �C���v�������e�[�V����
protected:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
};
