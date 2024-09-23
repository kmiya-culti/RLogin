#pragma once

// CClipPage �_�C�A���O

class CClipPage : public CTreePage
{
	DECLARE_DYNAMIC(CClipPage)

// �R���X�g���N�V����
public:
	CClipPage();
	virtual ~CClipPage();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_CLIPBOARDPAGE };

public:
	BOOL m_Check[9];
	CString m_WordStr;
	BOOL m_ClipFlag[2];
	CString m_ShellStr;
	int m_RClick;
	int m_ClipCrLf;
	int m_RtfMode;
	int m_PastEdit;

	CComboBoxHis m_WordStrCombo;
	CComboBoxHis m_ShellStrCombo;

public:
	void DoInit();

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();

public:
	virtual BOOL OnApply();
	virtual void OnReset();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateChange();
};
