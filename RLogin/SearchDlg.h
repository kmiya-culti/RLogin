#pragma once

#include "afxcmn.h"
#include "afxwin.h"
#include "DialogExt.h"

// CSearchDlg �_�C�A���O

class CSearchDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CSearchDlg)

// �R���X�g���N�V����
public:
	CSearchDlg(CWnd* pParent = NULL);
	virtual ~CSearchDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_SEARCHDLG };

public:
	CComboBoxHis m_SearchCombo;
	CString m_SearchStr;
	CProgressCtrl m_Prog;
	int m_SerchMode;
	BOOL m_RegEscChar;
	BOOL m_RegNoCase;
	UINT_PTR m_TimerId;

// �N���X�f�[�^
public:
	class CRLoginDoc *m_pDoc;
	int m_Pos;
	int m_Max;

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
