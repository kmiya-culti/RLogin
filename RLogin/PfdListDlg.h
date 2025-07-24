#pragma once

#include "ListCtrlExt.h"
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg �_�C�A���O

class CPfdData : public CObject
{
public:
	CString	m_ListenHost;
	CString	m_ListenPort;
	CString	m_ConnectHost;
	CString	m_ConnectPort;
	int m_ListenType;
	BOOL m_EnableFlag;
	DWORD m_TimeOut;

	BOOL GetString(LPCTSTR str);
	void SetString(CString &str);
	const CPfdData & operator = (CPfdData &data);

	CPfdData();
	~CPfdData();
};

class CPfdListDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CPfdListDlg)

// �R���X�g���N�V����
public:
	CPfdListDlg(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	enum { IDD = IDD_PFDLISTDLG };

public:
	class CServerEntry *m_pEntry;
	CListCtrlExt	m_List;
	CStringArrayExt m_PortFwd;
	CArray<CPfdData, CPfdData &> m_Data;
	BOOL m_X11PortFlag;
	CString m_XDisplay;
	BOOL m_x11AuthFlag;
	CString m_x11AuthName;
	CString m_x11AuthData;

	void InitList();
	void UpdateCheck();

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// �C���v�������e�[�V����
protected:
	afx_msg void OnPfdNew();
	afx_msg void OnPfdEdit();
	afx_msg void OnPfdDel();
	afx_msg void OnEditDups();
	afx_msg void OnEditDelall();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnDblclkPfdlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	DECLARE_MESSAGE_MAP()
};
