#pragma once

// CHisPage �_�C�A���O

class CHisPage : public CTreePage
{
	DECLARE_DYNAMIC(CHisPage)

// �R���X�g���N�V����
public:
	CHisPage();
	virtual ~CHisPage();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_HISPAGE };

public:
	BOOL m_Check[10];
	CString m_HisFile;
	CString	m_HisMax;
	CString m_LogFile;
	CString m_TimeFormat;
	int m_LogMode;
	int m_LogCode;
	CString m_TraceFile;
	CString m_TraceMax;

	CComboBoxHis m_LogCombo;
	CComboBoxHis m_HisCombo;
	CComboBoxHis m_TraceCombo;

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
	afx_msg void OnHisfileSel();
	afx_msg void OnAutoLogSel();
	afx_msg void OnTraceLogSel();
	DECLARE_MESSAGE_MAP()
};
