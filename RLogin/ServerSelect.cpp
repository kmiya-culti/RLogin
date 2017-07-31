// ServerSelect.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ServerSelect.h"
#include "OptDlg.h"
#include "Data.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerSelect ダイアログ

CServerSelect::CServerSelect(CWnd* pParent /*=NULL*/)
	: CDialog(CServerSelect::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServerSelect)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
	//}}AFX_DATA_INIT
	m_EntryNum = (-1);
	m_pData = NULL;
}

void CServerSelect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerSelect)
	DDX_Control(pDX, IDC_SERVERLIST, m_List);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CServerSelect, CDialog)
	//{{AFX_MSG_MAP(CServerSelect)
	ON_BN_CLICKED(IDC_NEWENTRY, OnNewentry)
	ON_BN_CLICKED(IDC_EDITENTRY, OnEditentry)
	ON_BN_CLICKED(IDC_DELENTRY, OnDelentry)
	ON_NOTIFY(NM_DBLCLK, IDC_SERVERLIST, OnDblclkServerlist)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_EDIT_NEW, OnNewentry)
	ON_COMMAND(ID_EDIT_UPDATE, OnEditentry)
	ON_COMMAND(ID_EDIT_DELETE, OnDelentry)
	ON_COMMAND(ID_EDIT_DUPS, OnEditCopy)
	ON_COMMAND(ID_EDIT_CHECK, OnEditCheck)
	ON_COMMAND(IDM_SERV_INPORT, &CServerSelect::OnServInport)
	ON_COMMAND(IDM_SERV_EXPORT, &CServerSelect::OnServExport)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(IDM_SERV_EXPORT, OnUpdateEditEntry)
END_MESSAGE_MAP()

void CServerSelect::InitList()
{
	int n;

	m_List.DeleteAllItems();

	for ( n = 0 ; n < m_pData->GetSize() ; n++ ) {
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, m_pData->GetAt(n).m_EntryName, 0, 0, 0, n);
		m_List.SetItemText(n, 1, m_pData->GetAt(n).m_HostName);
		m_List.SetItemText(n, 2, m_pData->GetAt(n).m_UserName);
		m_List.SetItemText(n, 3, m_pData->GetAt(n).m_TermName);
		m_List.SetItemText(n, 4, m_pData->GetAt(n).GetKanjiCode());
		m_List.SetItemText(n, 5, m_pData->GetAt(n).m_PortName);
	}

	m_List.SetItemState(m_EntryNum, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_List.DoSortItem();
}

/////////////////////////////////////////////////////////////////////////////
// CServerSelect メッセージ ハンドラ

static const LV_COLUMN InitListTab[6] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,  80, "Entry",  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60, "Server", 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, "User",   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  50, "Term",   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  40, "Kanji",  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  40, "Socket", 0, 0 }, 
	};

BOOL CServerSelect::OnInitDialog() 
{
	CDialog::OnInitDialog();
	ASSERT(m_pData);

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_List.InitColumn("ServerSelect", InitListTab, 6);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 0);

	if ( m_EntryNum == (-1) )
		m_EntryNum = AfxGetApp()->GetProfileInt("ServerSelect", "LastAccess", (-1));
	InitList();

	return TRUE;
}
void CServerSelect::OnOK()
{
	m_EntryNum = m_List.GetSelectMarkData();
	AfxGetApp()->WriteProfileInt("ServerSelect", "LastAccess", m_EntryNum);
	m_List.SaveColumn("ServerSelect");

	int n, i;

	for ( n = 0 ; n < m_pData->GetSize() ; n++ )
		m_pData->GetAt(n).m_CheckFlag = FALSE;

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) != 0 ) {
			i = (int)m_List.GetItemData(n);
			m_pData->GetAt(i).m_CheckFlag = TRUE;
		}
	}

	CDialog::OnOK();
}

void CServerSelect::OnDblclkServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	PostMessage(WM_COMMAND, IDOK);
	*pResult = 0;
}

void CServerSelect::OnNewentry() 
{
	CServerEntry Entry;
	CTextRam TextRam;
	CKeyNodeTab KeyTab;
	CKeyMacTab KeyMac;
	CParamTab ParamTab;
	COptDlg dlg("Server New Entry", this);

	dlg.m_pEntry    = &Entry;
	dlg.m_pTextRam  = &TextRam;
	dlg.m_pKeyTab   = &KeyTab;
	dlg.m_pParamTab = &ParamTab;
	dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;

	if ( dlg.DoModal() != IDOK )
		return;

	Entry.m_ProBuffer.Clear();
	TextRam.Serialize(TRUE,  Entry.m_ProBuffer);
	KeyTab.Serialize(TRUE,   Entry.m_ProBuffer);
	KeyMac.Serialize(TRUE,   Entry.m_ProBuffer);
	ParamTab.Serialize(TRUE, Entry.m_ProBuffer);

	m_EntryNum = m_pData->AddEntry(Entry);
	InitList();
}
void CServerSelect::OnEditentry() 
{
	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	CServerEntry Entry;
	CTextRam TextRam;
	CKeyNodeTab KeyTab;
	CKeyMacTab KeyMac;
	CParamTab ParamTab;

	Entry = m_pData->GetAt(m_EntryNum);
	TextRam.Serialize(FALSE,  Entry.m_ProBuffer);
	KeyTab.Serialize(FALSE,   Entry.m_ProBuffer);
	KeyMac.Serialize(FALSE,   Entry.m_ProBuffer);
	ParamTab.Serialize(FALSE, Entry.m_ProBuffer);

	COptDlg dlg("Server Edit Entry", this);

	dlg.m_pEntry    = &Entry;
	dlg.m_pTextRam  = &TextRam;
	dlg.m_pKeyTab   = &KeyTab;
	dlg.m_pParamTab = &ParamTab;
	dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;

	if ( dlg.DoModal() != IDOK )
		return;

	Entry.m_ProBuffer.Clear();
	TextRam.Serialize(TRUE,  Entry.m_ProBuffer);
	KeyTab.Serialize(TRUE,   Entry.m_ProBuffer);
	KeyMac.Serialize(TRUE,   Entry.m_ProBuffer);
	ParamTab.Serialize(TRUE, Entry.m_ProBuffer);

	m_pData->m_Data[m_EntryNum] = Entry;
	m_pData->UpdateAt(m_EntryNum);
	InitList();
}
void CServerSelect::OnDelentry() 
{
	CString tmp;
	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;
	tmp.Format("'%s'サーバーエントリーを削除してもよいですか？", m_pData->m_Data[m_EntryNum].m_EntryName);
	if ( MessageBox(tmp, "Question", MB_YESNO | MB_ICONQUESTION ) != IDYES )
		return;
	m_pData->RemoveAt(m_EntryNum);
	InitList();
}
void CServerSelect::OnEditCopy() 
{
	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;
	int n, i;
	CServerEntry tmp;
	tmp = m_pData->GetAt(m_EntryNum);
	for ( i = 2 ; ; i++ ) {
		tmp.m_EntryName.Format("%s(%d)", m_pData->GetAt(m_EntryNum).m_EntryName, i);
		for ( n = 0 ; n < m_pData->GetSize() ; n++ ) {
			if ( tmp.m_EntryName.Compare(m_pData->GetAt(n).m_EntryName) == 0 )
				break;
		}
		if ( n >= m_pData->GetSize() )
			break;
	}
	tmp.m_Uid = (-1);
	m_EntryNum = m_pData->AddEntry(tmp);
	InitList();
}
void CServerSelect::OnEditCheck()
{
	int n, i;
	CServerEntryTab tab;
	CBuffer nbuf, obuf;
	CWordArray apend, remove, update;

	for ( n = 0 ; n < tab.GetSize() ; n++ ) {
		for ( i = 0 ; i < m_pData->GetSize() ; i++ ) {
			if ( tab.GetAt(n).m_Uid == m_pData->GetAt(i).m_Uid )
				break;
		}
		if ( i < m_pData->GetSize() ) {
			nbuf.Clear();
			tab.GetAt(n).SetBuffer(nbuf);
			obuf.Clear();
			m_pData->GetAt(i).SetBuffer(obuf);
			if ( nbuf.GetSize() != obuf.GetSize() || memcmp(nbuf.GetPtr(), obuf.GetPtr(), nbuf.GetSize()) != 0 ) {
				update.Add(i);
				update.Add(n);
			}
		} else
			apend.Add(n);
	}
	for ( i = 0 ; i < m_pData->GetSize() ; i++ ) {
		for ( n = 0 ; n < tab.GetSize() ; n++ ) {
			if ( tab.GetAt(n).m_Uid == m_pData->GetAt(i).m_Uid )
				break;
		}
		if ( n >= tab.GetSize() )
			remove.Add(i);
	}

	for ( n = 0 ; n < apend.GetSize() ; n++ )
		m_pData->m_Data.Add(tab.GetAt(apend[n]));
	for ( n = 0 ; n < update.GetSize() ; n += 2 )
		m_pData->m_Data[update[n]] = tab.GetAt(update[n + 1]);
	for ( n = 0 ; n < remove.GetSize() ; n++ )
		m_pData->m_Data.RemoveAt(remove[n]);

	InitList();
}

void CServerSelect::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CServerSelect::OnServInport()
{
	CFileDialog dlg(TRUE, "rlg", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "RLogin ﾌｧｲﾙ (*.rlg)|*.rlg|All Files (*.*)|*.*||", this);

	if ( dlg.DoModal() != IDOK )
		return;

	CFile File;

	if ( !File.Open(dlg.GetPathName(), CFile::modeRead | CFile::shareExclusive) ) {
		MessageBox("File Open Error");
		return;
	}

	CWaitCursor wait;
	CArchive Archive(&File, CArchive::load);

	CServerEntry Entry;
	CTextRam TextRam;
	CKeyNodeTab KeyTab;
	CKeyMacTab KeyMac;
	CParamTab ParamTab;
	BYTE tmp[256];

	TRY {
		Archive.ReadString((LPSTR)tmp, 256);
		if ( strncmp((LPSTR)tmp, "RLG2", 4) != 0 )
			AfxThrowArchiveException(CArchiveException::badIndex, Archive.GetFile()->GetFileTitle());

		for ( ; ; ) {
			Entry.Serialize(Archive);
			TextRam.Serialize(Archive);
			KeyTab.Serialize(Archive);
			KeyMac.Serialize(Archive);
			ParamTab.Serialize(Archive);

			Entry.m_Uid = (-1);
			Entry.m_ProBuffer.Clear();
			ParamTab.m_IdKeyList.RemoveAll();
			TextRam.Serialize(TRUE,  Entry.m_ProBuffer);
			KeyTab.Serialize(TRUE,   Entry.m_ProBuffer);
			KeyMac.Serialize(TRUE,   Entry.m_ProBuffer);
			ParamTab.Serialize(TRUE, Entry.m_ProBuffer);

			m_EntryNum = m_pData->AddEntry(Entry);

			if ( !Archive.ReadString((LPSTR)tmp, 256) )
				break;
			if ( strncmp((LPSTR)tmp, "RLG21", 5) != 0 )
				break;
		}
	} CATCH_ALL(e) {
		MessageBox("File Read Error");
	} END_CATCH_ALL

	Archive.Close();
	File.Close();

	InitList();
}
void CServerSelect::OnServExport()
{
	int n;
	CServerEntry Entry;
	CTextRam TextRam;
	CKeyNodeTab KeyTab;
	CKeyMacTab KeyMac;
	CParamTab ParamTab;

	if ( (m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;

	CFileDialog dlg(FALSE, "rlg", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "RLogin ﾌｧｲﾙ (*.rlg)|*.rlg|All Files (*.*)|*.*||", this);

	if ( dlg.DoModal() != IDOK )
		return;

	CFile File;

	if ( !File.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive) ) {
		MessageBox("File Crate Error");
		return;
	}

	CWaitCursor wait;
	CArchive Archive(&File, CArchive::store | CArchive::bNoFlushOnDelete);

	TRY {
		for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
			if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
				continue;
			m_EntryNum = (int)m_List.GetItemData(n);
			Entry = m_pData->GetAt(m_EntryNum);

			TextRam.Serialize(FALSE,  Entry.m_ProBuffer);
			KeyTab.Serialize(FALSE,   Entry.m_ProBuffer);
			KeyMac.Serialize(FALSE,   Entry.m_ProBuffer);
			ParamTab.Serialize(FALSE, Entry.m_ProBuffer);

			Archive.WriteString("RLG210\n");
			Entry.Serialize(Archive);
			TextRam.Serialize(Archive);
			KeyTab.Serialize(Archive);
			KeyMac.Serialize(Archive);
			ParamTab.Serialize(Archive);
		}
	} CATCH_ALL(e) {
		MessageBox("File Write Error");
	} END_CATCH_ALL

	Archive.Close();
	File.Close();
}
