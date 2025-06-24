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
	void InitList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
	afx_msg void OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
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
	void InitList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnItemchangedModeList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
};

/////////////////////////////////////////////////////////////////////////////
// CAppColDlg

class CAppColDlg : public CTtyModeDlg
{
	DECLARE_DYNAMIC(CAppColDlg)

public:
	CAppColDlg();
	virtual ~CAppColDlg();

public:
	CArray<int, int> m_nIndex;
	COLORREF m_ColTab[2][APPCOL_MAX];

public:
	void InitList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
	afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
};

/////////////////////////////////////////////////////////////////////////////
// CCodeFlagDlg

class CCodeFlagDlg : public CTtyModeDlg
{
	DECLARE_DYNAMIC(CCodeFlagDlg)

public:
	CCodeFlagDlg();
	virtual ~CCodeFlagDlg();

public:
	class COptDlg *m_pSheet;
	BOOL m_bUpdate;
	CCodeFlag m_CodeFlag;
	int m_LastSelBlock;
	CString m_DefFontName;

public:
	void GetCode(int nItem, DWORD &low, DWORD &high);
	void UpdateMemo(int nItem);
	void InitList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEditNew();
	afx_msg void OnEditUpdate();
	afx_msg void OnEditDelete();
	afx_msg void OnEditDups();
	afx_msg void OnEditCopyAll();
	afx_msg void OnEditPasteAll();
	afx_msg void OnEditDelAll();
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditEnable(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditDisable(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditSelect(CCmdUI *pCmdUI);
	afx_msg void OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult);
};