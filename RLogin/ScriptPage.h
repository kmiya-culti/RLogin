#pragma once

// CScriptPage �_�C�A���O

class CScriptPage : public CTreePage
{
	DECLARE_DYNAMIC(CScriptPage)

// �R���X�g���N�V����
public:
	CScriptPage();
	virtual ~CScriptPage();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_SCRIPTPAGE };

public:
	BOOL m_Check[10];
	CString m_ScriptFile;
	CString m_ScriptStr;

	CComboBoxHis m_ScriptCombo;

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
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	afx_msg void OnBnClickedScriptSel();
	DECLARE_MESSAGE_MAP()
};
