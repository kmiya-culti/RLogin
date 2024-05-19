#pragma once

#include "afxcmn.h"
#include "Data.h"
#include "ListCtrlExt.h"
#include "DialogExt.h"

/////////////////////////////////////////////////////////////////////////////
// CServerSelect �_�C�A���O

#define	INIT_CALL_NONE			0
#define	INIT_CALL_UPDATE		1
#define	INIT_CALL_AND_EDIT		2

class CServerSelect : public CDialogExt
{
	DECLARE_DYNAMIC(CServerSelect)

// �R���X�g���N�V����
	CServerSelect(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^
	virtual ~CServerSelect();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_SERVERLIST };

public:
	CListCtrlExt m_List;
	CTabCtrlExt m_Tab;
	CTreeCtrlExt m_Tree;

// �N���X�f�[�^
public:
	int m_EntryNum;
	CString m_Group;
	CStringIndex m_TabEntry;
	class CServerEntryTab *m_pData;
	CStringArrayExt m_AddGroup;
	CStringArrayExt m_TreeExpand;
	int m_DefaultEntryUid;
	CTextRam *m_pTextRam;
	CKeyNodeTab *m_pKeyTab;
	CKeyMacTab *m_pKeyMac;
	CParamTab *m_pParamTab;
	CImageList m_ImageList;
	CPtrArray m_TabData;
	BOOL m_bShowTabWnd;
	BOOL m_bShowTreeWnd;
	BOOL m_bTreeUpdate;
	CPoint m_TrackerPoint;
	CRect m_TrackerRect;
	CRect m_TrackerLast;
	CRect m_TrackerMove;
	BOOL m_bTrackerActive;
	int m_TreeListPer;
	int m_DragImage;
	int m_DragActive;
	BOOL m_bDragList;
	int m_DragNumber;
	class COptDlg *m_pOPtDlg;
	CEdit m_EditWnd;
	CStringIndex *m_pEditIndex;
	CRect m_EditNow, m_EditRect, m_EditMax;
	CString m_InitPathName;

// �N���X�t�@���N�V����
public:
	virtual void SetItemOffset(int cx, int cy);
	void TreeExpandUpdate(HTREEITEM hTree, BOOL bExpand);
	void InitExpand(HTREEITEM hTree, UINT nCode);
	void InitTree(CStringIndex *pIndex, HTREEITEM hOwner, CStringIndex *pActive);
	void InitList(CStringIndex *pIndex, BOOL bFolder);
	void InitEntry(int nUpdate);
	void UpdateDefaultEntry(int num);
	void UpdateListIndex();

	BOOL GetTrackerRect(CRect &rect, CRect &move);
	void OffsetTracker(CPoint point);
	CStringIndex *DragIndex(CPoint point);
	void UpdateGroupName(CStringIndex *pIndex, LPCTSTR newName);
	void SetIndexList(CStringIndex *pIndex, BOOL bSelect);
	BOOL OpenTabEdit(int num);
	void SaveWindowStyle();
	void EntryNameCheck(CServerEntry &entry);

	static BOOL IsJsonEntryText(LPCTSTR str);

// �I�[�o�[���C�h
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

// �C���v�������e�[�V����
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();

	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKillfocusEditBox();
	afx_msg void OnEnUpdateEditBox();

	afx_msg void OnNewEntry();
	afx_msg void OnEditEntry();
	afx_msg void OnDelEntry();
	afx_msg void OnCopyEntry();
	afx_msg void OnCheckEntry();
	afx_msg void OnServInport();
	afx_msg void OnServExport();
	afx_msg void OnAllExport();
	afx_msg void OnServProto();
	afx_msg void OnSaveDefault();
	afx_msg void OnServExchng();
	afx_msg void OnLoaddefault();
	afx_msg void OnShortcut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();

	afx_msg void OnUpdateSaveDefault(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditEntry(CCmdUI* pCmdUI);

	afx_msg void OnDblclkServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnBegindragServerlist(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnTcnSelchangeServertab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickServertab(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnTvnSelchangedServertree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnBeginlabeleditServertree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnEndlabeleditServertree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickServertree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnItemexpandedServertree(NMHDR *pNMHDR, LRESULT *pResult);
};
