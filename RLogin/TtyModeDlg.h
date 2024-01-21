#pragma once

#include "afxcmn.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CTtyModeDlg

class CTtyModeDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CTtyModeDlg)

public:
	CTtyModeDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CTtyModeDlg();

// ダイアログ データ
	enum { IDD = IDD_TTYMODEDLG };

public:
	CListCtrlExt m_List;
	class CParamTab *m_pParamTab;

	void InitList();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CColEditDlg

class CColEditDlg : public CTtyModeDlg
{
	DECLARE_DYNAMIC(CColEditDlg)

public:
	CColEditDlg();
	virtual ~CColEditDlg();

	COLORREF m_ColTab[16];

public:
	virtual void InitList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
	afx_msg void OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult);
};

/////////////////////////////////////////////////////////////////////////////
// CKnownHostsDlg

class CKnownHostsDlg : public CTtyModeDlg
{
	DECLARE_DYNAMIC(CKnownHostsDlg)

public:
	CKnownHostsDlg();
	virtual ~CKnownHostsDlg();

public:
	typedef struct _KnownHostData {
		int			type;
		BOOL		del;
		CString		key;
		CString		host;
		CString		port;
		CString		digest;
	} KNOWNHOSTDATA;

	CPtrArray m_Data;

public:
	virtual void InitList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnItemchangedModeList(NMHDR *pNMHDR, LRESULT *pResult);
};
