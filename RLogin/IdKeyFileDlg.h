#pragma once

#include "DialogExt.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CIdKeyFileDlg �_�C�A���O

#define	IDKFDMODE_LOAD		1
#define	IDKFDMODE_SAVE		2
#define	IDKFDMODE_CREATE	3
#define	IDKFDMODE_OPEN		4

class CIdKeyFileDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CIdKeyFileDlg)

// �R���X�g���N�V����
public:
	CIdKeyFileDlg(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	enum { IDD = IDD_IDKEYFILEDLG };

public:
	CString	m_IdkeyFile;
	CStringLoad	m_Message;
	CString	m_PassName;
	BOOL m_bPassDisp;

	CComboBoxHis m_IdkeyFileCombo;

	int m_OpenMode;
	CStringLoad m_Title;
	TCHAR m_PassChar;

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// �C���v�������e�[�V����
protected:
	afx_msg void OnIdkeysel();
	afx_msg void OnBnClickedPassdisp();
	DECLARE_MESSAGE_MAP()
};
