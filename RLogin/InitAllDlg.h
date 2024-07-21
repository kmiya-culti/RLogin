#pragma once

#include "afxwin.h"
#include "DialogExt.h"

// CInitAllDlg �_�C�A���O

class CInitAllDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CInitAllDlg)

public:
	CInitAllDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[
	virtual ~CInitAllDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_INITALLDLG };

public:
	CStatic m_MsgIcon;
	int m_InitType;
	CComboBox m_EntryCombo;
	CStatic m_TitleWnd;
	CServerEntry *m_pInitEntry;
	CStringLoad m_Title;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnInitType(UINT nID);
};
