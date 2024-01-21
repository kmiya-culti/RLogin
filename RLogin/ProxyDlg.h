#pragma once

#include "DialogExt.h"
#include "afxwin.h"

// CProxyDlg �_�C�A���O

class CProxyDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CProxyDlg)

// �R���X�g���N�V����
public:
	CProxyDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^
	virtual ~CProxyDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_PROXYDLG };

public:
	CString m_ServerName;
	CString m_PortName;
	CString m_UserName;
	CString m_PassWord;
	int m_ProxyType;
	int m_ProxyMode;
	int m_SSLType;
	int m_SSLMode;
	BOOL m_SSL_Keep;
	BOOL m_UsePassDlg;
	BOOL m_CmdFlag;
	CString m_ProxyCmd;

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnProtoType(UINT nID);
};
