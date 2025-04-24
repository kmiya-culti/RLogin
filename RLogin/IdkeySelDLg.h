#pragma once

#include <afxmt.h>
#include <afxtempl.h>
#include "Data.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CIdkeySelDLg �_�C�A���O

class CIdkeySelDLg : public CDialogExt
{
	DECLARE_DYNAMIC(CIdkeySelDLg)
	
// �R���X�g���N�V����
public:
	CIdkeySelDLg(CWnd* pParent = NULL);
	virtual ~CIdkeySelDLg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_IDKEYSELDLG };

public:
	CProgressCtrl m_KeyGenProg;
	CListCtrlExt m_List;
	int m_ExportStyle;
	CString	m_Type;
	CString	m_Bits;
	CString	m_Name;

	class CParamTab *m_pParamTab;
	CIdKeyTab *m_pIdKeyTab;
	CStringArrayExt m_IdKeyList;
	CDWordArray m_Data;
	int m_EntryNum;
	CEvent *m_pKeyGenEvent;
	int m_KeyGenFlag;
	UINT m_GenIdKeyTimer;
	UINT m_GenElapse;
	int m_GenIdKeyType;
	int m_GenIdKeyBits;
	BOOL m_GenIdKeyStat;
	int m_GenIdKeyMax;
	int m_GenIdKeyCount;
	CString m_GenIdKeyPass;
	GENSTATUS m_GenStat;
	CIdKey m_GenIdKey;
	BOOL m_ListInit;
	BOOL m_bInitPageant;
	CString m_CreateStr;
	CString m_CancelStr;

	void StartKeyGenThead();
	void ProcKeyGenThead();
	void EndofKeyGenThead();
	void OnKeyGenEndof();

	void InitList();
	void SetBitsList();


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
	afx_msg void OnIdkeyUp();
	afx_msg void OnIdkeyDown();
	afx_msg void OnIdkeyDel();
	afx_msg void OnIdkeyInport();
	afx_msg void OnIdkeyExport();
	afx_msg void OnIdkeyCreate();
	afx_msg void OnDblclkIdkeyList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnIdkeyCopy();
	afx_msg void OnEditUpdate();
	afx_msg void OnEditCheck();
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);
	afx_msg void OnLvnItemchangedIdkeyList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnIdkeyCakey();
	afx_msg void OnSavePublicKey();
	afx_msg void OnCbnSelchangeIdkeyType();
};
