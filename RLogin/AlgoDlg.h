#if !defined(AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_)
#define AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlgoDlg.h : �w�b�_�[ �t�@�C��
//
#include "ListCtrlExt.h"
#include "DialogExt.h"
#include "Data.h"

/////////////////////////////////////////////////////////////////////////////
// CAlgoDlg �_�C�A���O

class CAlgoDlg : public CDialogExt
{
// �R���X�g���N�V����
public:
	CAlgoDlg(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	enum { IDD = IDD_ALGOPARADLG };

public:
	CListCtrlExt m_List[12];
	CStringArrayExt m_AlgoTab[12];
	BOOL m_EncShuffle;
	BOOL m_MacShuffle;

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedReset();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // !defined(AFX_ALGODLG_H__E8F1EDB4_475E_4080_860F_4743CEE9751F__INCLUDED_)
