#pragma once

#include "DialogExt.h"

// CDefParamDlg �_�C�A���O

class CDefParamDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CDefParamDlg)

public:
	CDefParamDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[
	virtual ~CDefParamDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_DEFPARAMDLG };

public:
	int m_InitFlag;
	BOOL m_Check[7];
	CStatic m_MsgIcon;
	class CTextRam *m_pTextRam;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
};
