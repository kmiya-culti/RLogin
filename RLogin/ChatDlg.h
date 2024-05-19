#pragma once

#include "afxcmn.h"
#include "RegEx.h"
#include "Data.h"
#include "afxwin.h"
#include "DialogExt.h"

// CChatDlg �_�C�A���O

class CChatDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CChatDlg)

// �R���X�g���N�^
public:
	CChatDlg(CWnd* pParent = NULL);
	virtual ~CChatDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_CHATDLG };

public:
	CTreeCtrlExt m_NodeTree;
	CString m_RecvStr;
	CString m_SendStr;
	CButton m_UpdNode;
	CButton m_DelNode;
	CStrScript m_Script;
	BOOL m_MakeChat;

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// �C���v�������e�[�V����
protected:
	afx_msg void OnBnClickedNewnode();
	afx_msg void OnBnClickedNextnode();
	afx_msg void OnBnClickedUpdatenode();
	afx_msg void OnBnClickedDelnode();
	afx_msg void OnTvnSelchangedNodetree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnDeleteitemNodetree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickNodetree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
	DECLARE_MESSAGE_MAP()
};
