// ChatDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "ChatDlg.h"


// CChatDlg ダイアログ

IMPLEMENT_DYNAMIC(CChatDlg, CDialog)

CChatDlg::CChatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChatDlg::IDD, pParent)
	, m_RecvStr(_T(""))
	, m_SendStr(_T(""))
	, m_MakeChat(FALSE)
{
}

CChatDlg::~CChatDlg()
{
}

void CChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NODETREE, m_NodeTree);
	DDX_Text(pDX, IDC_RECVSTR, m_RecvStr);
	DDX_Text(pDX, IDC_SENDSTR, m_SendStr);
	DDX_Control(pDX, IDC_UPDATENODE, m_UpdNode);
	DDX_Control(pDX, IDC_DELNODE, m_DelNode);
	DDX_Check(pDX, IDC_MAKECHAT, m_MakeChat);
}


BEGIN_MESSAGE_MAP(CChatDlg, CDialog)
	ON_BN_CLICKED(IDC_NEWNODE, &CChatDlg::OnBnClickedNewnode)
	ON_BN_CLICKED(IDC_NEXTNODE, &CChatDlg::OnBnClickedNextnode)
	ON_BN_CLICKED(IDC_UPDATENODE, &CChatDlg::OnBnClickedUpdatenode)
	ON_BN_CLICKED(IDC_DELNODE, &CChatDlg::OnBnClickedDelnode)
	ON_NOTIFY(TVN_SELCHANGED, IDC_NODETREE, &CChatDlg::OnTvnSelchangedNodetree)
	ON_NOTIFY(TVN_DELETEITEM, IDC_NODETREE, &CChatDlg::OnTvnDeleteitemNodetree)
END_MESSAGE_MAP()


// CChatDlg メッセージ ハンドラ

BOOL CChatDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_Script.SetTreeCtrl(m_NodeTree);

	m_RecvStr.Empty();
	m_SendStr.Empty();
	m_MakeChat = m_Script.m_MakeChat;

	UpdateData(FALSE);

	return TRUE;
}

void CChatDlg::OnOK()
{
	UpdateData(TRUE);
	m_Script.GetTreeCtrl(m_NodeTree);
	m_Script.m_MakeChat = m_MakeChat;
	CDialog::OnOK();
}

void CChatDlg::OnBnClickedNewnode()
{
	CStrScriptNode *np;
	HTREEITEM hti = m_NodeTree.GetSelectedItem();
	CString tmp;

	if ( hti == NULL )
		hti = TVI_ROOT;

	UpdateData(TRUE);
	np = new CStrScriptNode;
	np->m_RecvStr = m_RecvStr;
	np->m_SendStr = m_SendStr;
	np->m_Reg.Compile(np->m_RecvStr);

	tmp.Format("%s/%s", m_RecvStr, m_SendStr);

	if ( (hti = m_NodeTree.InsertItem(tmp, hti)) == NULL )
		delete np;
	else
		m_NodeTree.SetItemData(hti, (DWORD_PTR)np);

	m_NodeTree.SelectItem(hti);
}

void CChatDlg::OnBnClickedNextnode()
{
	CStrScriptNode *np;
	HTREEITEM hti = m_NodeTree.GetSelectedItem();
	CString tmp;

	if ( hti == NULL || (hti = m_NodeTree.GetParentItem(hti)) == NULL )
		hti = TVI_ROOT;

	UpdateData(TRUE);
	np = new CStrScriptNode;
	np->m_RecvStr = m_RecvStr;
	np->m_SendStr = m_SendStr;
	np->m_Reg.Compile(np->m_RecvStr);

	tmp.Format("%s/%s", m_RecvStr, m_SendStr);

	if ( (hti = m_NodeTree.InsertItem(tmp, hti)) == NULL )
		delete np;
	else
		m_NodeTree.SetItemData(hti, (DWORD_PTR)np);

	m_NodeTree.SelectItem(hti);
}

void CChatDlg::OnBnClickedUpdatenode()
{
	CStrScriptNode *np;
	HTREEITEM hti = m_NodeTree.GetSelectedItem();
	CString tmp;

	if ( hti == NULL )
		return;

	UpdateData(TRUE);
	np = (CStrScriptNode *)m_NodeTree.GetItemData(hti);
	np->m_RecvStr = m_RecvStr;
	np->m_SendStr = m_SendStr;
	np->m_Reg.Compile(np->m_RecvStr);

	tmp.Format("%s/%s", m_RecvStr, m_SendStr);
	m_NodeTree.SetItemText(hti, tmp);
}

void CChatDlg::OnBnClickedDelnode()
{
	CStrScriptNode *np;
	HTREEITEM hti = m_NodeTree.GetSelectedItem();

	if ( hti == NULL )
		return;

	np = (CStrScriptNode *)m_NodeTree.GetItemData(hti);
	m_NodeTree.DeleteItem(hti);
//	m_NodeTree.SelectItem(NULL);
}

void CChatDlg::OnTvnSelchangedNodetree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	CStrScriptNode *np;
	HTREEITEM hti = m_NodeTree.GetSelectedItem();

	m_UpdNode.EnableWindow(hti != NULL ? TRUE : FALSE);
	m_DelNode.EnableWindow(hti != NULL ? TRUE : FALSE);

	if ( hti == NULL ) {
		m_RecvStr.Empty();
		m_SendStr.Empty();
	} else if ( (np = (CStrScriptNode *)m_NodeTree.GetItemData(hti)) != NULL ) {
		m_RecvStr = np->m_RecvStr;
		m_SendStr = np->m_SendStr;
	}

	UpdateData(FALSE);
}

void CChatDlg::OnTvnDeleteitemNodetree(NMHDR *pNMHDR, LRESULT *pResult)
{
	CStrScriptNode *np;
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	if ( (np = (CStrScriptNode *)(pNMTreeView->itemOld.lParam)) != NULL )
		delete np;
	*pResult = 0;
}
