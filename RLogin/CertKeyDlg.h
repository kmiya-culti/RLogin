#pragma once

#include "afxcmn.h"
#include "DialogExt.h"

// CCertKeyDlg �_�C�A���O

class CCertKeyDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CCertKeyDlg)

public:
	CCertKeyDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[
	virtual ~CCertKeyDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_CERTKEYDLG };

public:
	CString m_Digest;
	BOOL m_SaveKeyFlag;
	CString m_HostName;
	CProgressCtrl m_TimeLimit;
	CFont m_DigestFont;
	BOOL m_bStatusMode;

	int m_Counter;
	int m_MaxTime;

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();

// �C���v�������e�[�V����
protected:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
};
