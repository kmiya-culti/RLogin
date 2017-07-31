// IdkeySelDLg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "IdkeySelDLg.h"
#include "IdKeyFileDlg.h"
#include "EditDlg.h"

#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// CIdkeySelDLg ダイアログ

IMPLEMENT_DYNAMIC(CIdkeySelDLg, CDialogExt)

CIdkeySelDLg::CIdkeySelDLg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CIdkeySelDLg::IDD, pParent)
{
	m_Type = _T("RSA2");
	m_Bits = _T("2048");
	m_Name = _T("");
	m_pIdKeyTab = NULL;
	m_EntryNum = (-1);
	m_pKeyGenEvent = new CEvent(FALSE, TRUE);
	m_KeyGenFlag = 0;
	m_GenIdKeyTimer = NULL;
	m_ListInit = FALSE;
}
CIdkeySelDLg::~CIdkeySelDLg()
{
	EndofKeyGenThead();
	delete m_pKeyGenEvent;
}

void CIdkeySelDLg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_IDKEY_PROG, m_KeyGenProg);
	DDX_Control(pDX, IDC_IDKEY_LIST, m_List);
	DDX_CBString(pDX, IDC_IDKEY_TYPE, m_Type);
	DDX_CBString(pDX, IDC_IDKEY_BITS, m_Bits);
	DDX_Text(pDX, IDC_IDKEY_NAME, m_Name);
}

BEGIN_MESSAGE_MAP(CIdkeySelDLg, CDialogExt)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_IDKEY_UP, OnIdkeyUp)
	ON_BN_CLICKED(IDC_IDKEY_DOWN, OnIdkeyDown)
	ON_BN_CLICKED(IDC_IDKEY_COPY, OnIdkeyCopy)
	ON_BN_CLICKED(IDC_IDKEY_INPORT, OnIdkeyInport)
	ON_BN_CLICKED(IDC_IDKEY_EXPORT, OnIdkeyExport)
	ON_BN_CLICKED(IDC_IDKEY_CREATE, OnIdkeyCreate)
	ON_NOTIFY(NM_DBLCLK, IDC_IDKEY_LIST, OnDblclkIdkeyList)
	ON_WM_CLOSE()
	ON_COMMAND(ID_EDIT_UPDATE, OnEditUpdate)
	ON_COMMAND(ID_EDIT_DELETE, OnIdkeyDel)
	ON_COMMAND(ID_EDIT_COPY, OnIdkeyCopy)
	ON_COMMAND(ID_EDIT_CHECK, OnEditCheck)
	ON_UPDATE_COMMAND_UI(IDC_IDKEY_UP, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(IDC_IDKEY_DOWN, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(IDC_IDKEY_EXPORT, OnUpdateEditEntry)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IDKEY_LIST, OnLvnItemchangedIdkeyList)
	ON_COMMAND(ID_IDKEY_CAKEY, &CIdkeySelDLg::OnIdkeyCakey)
	ON_COMMAND(IDM_SAVEPUBLICKEY, &CIdkeySelDLg::OnSavePublicKey)
	ON_UPDATE_COMMAND_UI(IDM_SAVEPUBLICKEY, OnUpdateEditEntry)
END_MESSAGE_MAP()

void CIdkeySelDLg::InitList()
{
	int n;
	CString str;
	CIdKey *pKey;

	m_ListInit = TRUE;
	m_List.DeleteAllItems();
	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( (pKey = m_pIdKeyTab->GetUid(m_Data[n])) == NULL )
			continue;

		switch(pKey->m_Type) {
		case IDKEY_NONE:    str = _T(""); break;
		case IDKEY_RSA1:    str = _T("RSA1"); break;
		case IDKEY_RSA2:    str = _T("RSA2"); break;
		case IDKEY_DSA2:    str = _T("DSA2"); break;
		case IDKEY_ECDSA:   str = _T("ECDSA"); break;
		case IDKEY_ED25519: str = _T("ED25519"); break;
		}
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);

		str.Format(_T("%d"), pKey->GetSize());
		m_List.SetItemText(n, 1, str);

		m_List.SetItemText(n, 2, pKey->m_Name);

		switch(pKey->m_Cert) {
		case IDKEY_CERTV00:  str = _T("V00"); break;
		case IDKEY_CERTV01:  str = _T("V01"); break;
		case IDKEY_CERTX509: str = _T("x509"); break;
		default:             str = _T(""); break;
		}
		m_List.SetItemText(n, 3, str);

		m_List.SetItemData(n, n);
		m_List.SetLVCheck(n, pKey->m_Flag);
	}
	m_List.SetItemState(m_EntryNum, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_ListInit = FALSE;
}
static UINT KeyGenThread(LPVOID pParam)
{
	CIdkeySelDLg *pWnd = (CIdkeySelDLg *)pParam;
	pWnd->ProcKeyGenThead();
	pWnd->m_KeyGenFlag = 2;
	pWnd->m_pKeyGenEvent->SetEvent();
	return 0;
}
void CIdkeySelDLg::StartKeyGenThead()
{
	if ( m_KeyGenFlag != 0 )
		return;
	m_KeyGenFlag = 1;
	m_pKeyGenEvent->ResetEvent();
	AfxBeginThread(KeyGenThread, this, THREAD_PRIORITY_LOWEST);
	m_GenIdKeyTimer = (UINT)SetTimer(1024, 200, NULL);
}
void CIdkeySelDLg::ProcKeyGenThead()
{
	m_GenIdKeyStat = m_GenIdKey.Generate(m_GenIdKeyType, m_GenIdKeyBits, m_GenIdKeyPass);
}
void CIdkeySelDLg::EndofKeyGenThead()
{
	if ( m_KeyGenFlag == 0 )
		return;
	m_KeyGenFlag = FALSE;
	WaitForSingleObject(m_pKeyGenEvent->m_hObject, INFINITE);
	m_KeyGenFlag = 0;
	OnKeyGenEndof();
}

/////////////////////////////////////////////////////////////////////////////
// CIdkeySelDLg メッセージ ハンドラ

static const LV_COLUMN InitListTab[4] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,   80, _T("Type"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,   40, _T("Bits"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  100, _T("Name"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,   40, _T("Cert"), 0, 0 }, 
	};

BOOL CIdkeySelDLg::OnInitDialog() 
{
	int n, i, a;
	CIdKey *pKey;

	ASSERT(m_pIdKeyTab != NULL);

	CDialogExt::OnInitDialog();

	m_Data.RemoveAll();
	for ( n = 0 ; n < m_pIdKeyTab->GetSize() ; n++ ) {
		m_Data.Add(m_pIdKeyTab->GetAt(n).m_Uid);
		m_pIdKeyTab->GetAt(n).m_Flag = FALSE;
	}

	m_IdKeyList.SetString(m_OldIdKeyList);
	for ( n = (int)m_IdKeyList.GetSize() - 1 ; n >= 0 ; n-- ) {
		a = m_IdKeyList.GetVal(n);
		if ( (pKey = m_pIdKeyTab->GetUid(a)) == NULL )
			continue;
		for ( i = 0 ; i < m_Data.GetSize() ; i++ ) {
			if ( a == m_Data[i] ) {
				m_Data.RemoveAt(i);
				m_Data.InsertAt(0, a);
				pKey->m_Flag = TRUE;
				break;
			}
		}
	}

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	m_List.InitColumn(_T("IdkeySelDLg"), InitListTab, 4);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 2);
	m_List.m_bMove = TRUE;
	InitList();

	m_KeyGenProg.EnableWindow(FALSE);

	return TRUE;
}
void CIdkeySelDLg::OnOK() 
{
	ASSERT(m_pIdKeyTab != NULL);

	if ( m_KeyGenFlag != 0 ) {
		MessageBox(CStringLoad(IDE_MAKEIDKEY));
		return;
	}

	m_IdKeyList.RemoveAll();
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_List.GetLVCheck(n) ) {
			int i = (int)m_List.GetItemData(n);
			m_IdKeyList.AddVal(m_Data[i]);
		}
	}
	m_List.SaveColumn(_T("IdkeySelDLg"));

	CString str;
	m_IdKeyList.SetString(str);

	if ( str.Compare(m_OldIdKeyList) != 0 )
		CDialogExt::OnOK();
	else
		CDialogExt::OnCancel();
}

void CIdkeySelDLg::OnClose()
{
	if ( m_KeyGenFlag != 0 ) {
		MessageBox(CStringLoad(IDE_MAKEIDKEY));
		return;
	}
	CDialogExt::OnClose();
}
void CIdkeySelDLg::OnTimer(UINT_PTR nIDEvent) 
{
	switch(nIDEvent) {
	case 1024:
		if ( m_KeyGenFlag == 2 ) {
			KillTimer(m_GenIdKeyTimer);
			EndofKeyGenThead();
		} else {
			m_KeyGenProg.SetPos(m_GenIdKeyCount++);
			if ( m_GenIdKeyCount > m_GenIdKeyMax )
				m_GenIdKeyCount = 0;
		}
		break;
	}
	CDialogExt::OnTimer(nIDEvent);
}

void CIdkeySelDLg::OnIdkeyUp() 
{
	int n1, n2, d;

	if ( (m_EntryNum = m_List.GetSelectionMark()) <= 0 )
		return;

	n1 = (int)m_List.GetItemData(m_EntryNum - 1);
	n2 = (int)m_List.GetItemData(m_EntryNum);
	d = m_Data[n1];
	m_Data[n1] = m_Data[n2];
	m_Data[n2] = d;
	m_EntryNum -= 1;

	InitList();
}
void CIdkeySelDLg::OnIdkeyDown() 
{
	int n1, n2, d;

	if ( (m_EntryNum = m_List.GetSelectionMark()) < 0 )
		return;
	else if ( m_EntryNum >= (m_List.GetItemCount() - 1) )
		return;

	n1 = (int)m_List.GetItemData(m_EntryNum + 1);
	n2 = (int)m_List.GetItemData(m_EntryNum);
	d = m_Data[n1];
	m_Data[n1] = m_Data[n2];
	m_Data[n2] = d;
	m_EntryNum += 1;

	InitList();
}
void CIdkeySelDLg::OnIdkeyDel() 
{
	if ( (m_EntryNum = m_List.GetSelectionMark()) < 0 )
		return;

	if ( MessageBox(CStringLoad(IDE_DELETEKEYQES), _T("Warning"), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
		return;

	int n = (int)m_List.GetItemData(m_EntryNum);

	m_pIdKeyTab->RemoveUid(m_Data[n]);
	m_Data.RemoveAt(n);

	InitList();
}
void CIdkeySelDLg::OnIdkeyCopy() 
{
	if ( (m_EntryNum = m_List.GetSelectionMark()) < 0 )
		return;

	int n = (int)m_List.GetItemData(m_EntryNum);
	CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[n]);

	if ( pKey == NULL )
		return;

	CString str;
	CEditDlg dlg;

	pKey->WritePublicKey(str);

	dlg.m_WinText = _T("Public Key");
	dlg.m_Edit = str;
	dlg.m_Title.LoadString(IDS_PUBLICKEYCOPYMSG);

	if ( dlg.DoModal() == IDOK )
		((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(dlg.m_Edit);
}
void CIdkeySelDLg::OnIdkeyInport() 
{
	CIdKey key;
	CIdKeyFileDlg dlg;

	dlg.m_OpenMode = 1;
	dlg.m_Title.LoadString(IDS_IDKEYFILELOAD);
	dlg.m_Message.LoadString(IDS_IDKEYFILELOADCOM);

	if ( dlg.DoModal() != IDOK )
		return;

	if ( dlg.m_IdkeyFile.IsEmpty() ) {
		MessageBox(CStringLoad(IDE_USEIDKEYFILENAME));
		return;
	}

	if ( !key.LoadPrivateKey(dlg.m_IdkeyFile, dlg.m_PassName) ) {
		MessageBox(CStringLoad(IDE_IDKEYLOADERROR));
		return;
	} else if ( key.IsNotSupport() ) {
		MessageBox(CStringLoad(IDE_IDKEYNOTSUPPORT));
		return;
	}

	key.m_Flag = TRUE;

	if ( !m_pIdKeyTab->AddEntry(key) ) {
		MessageBox(CStringLoad(IDE_DUPIDKEYENTRY));
		return;
	}
	m_Data.InsertAt(0, key.m_Uid);

	m_EntryNum = 0;
	InitList();
}
void CIdkeySelDLg::OnIdkeyExport() 
{
	if ( (m_EntryNum = m_List.GetSelectionMark()) < 0 )
		return;

	int n = (int)m_List.GetItemData(m_EntryNum);
	CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[n]);
	CIdKeyFileDlg dlg;

	if ( pKey == NULL )
		return;

	dlg.m_OpenMode = 2;
	dlg.m_Title.LoadString(IDS_IDKEYFILESAVE);
	dlg.m_Message.LoadString(IDS_IDKEYFILESAVECOM);

	if ( dlg.DoModal() != IDOK )
		return;

	if ( dlg.m_IdkeyFile.IsEmpty() ) {
		MessageBox(CStringLoad(IDE_USEIDKEYFILENAME));
		return;
	}

	if ( pKey->CompPass(dlg.m_PassName) != 0 ) {
		MessageBox(CStringLoad(IDE_IDKEYPASSERROR));
		return;
	}

	if ( !pKey->SavePrivateKey(pKey->m_Type, dlg.m_IdkeyFile, dlg.m_PassName) )
		MessageBox(CStringLoad(IDE_IDKEYSAVEERROR));
}
void CIdkeySelDLg::OnIdkeyCreate() 
{
	CWnd *pWnd;
	CIdKeyFileDlg dlg;
	CString tmp;

	UpdateData(TRUE);

	m_GenIdKey.Close();
	m_GenIdKey.m_Name = m_Name;
	m_GenIdKey.m_Flag = TRUE;
	m_GenIdKeyType = IDKEY_DSA2;
	m_GenIdKeyBits = _tstoi(m_Bits);
	m_GenIdKeyStat = FALSE;

	if ( m_Type.Compare(_T("RSA1")) == 0 )
		m_GenIdKeyType = IDKEY_RSA1;
	else if ( m_Type.Compare(_T("RSA2")) == 0 )
		m_GenIdKeyType = IDKEY_RSA2;
	else if ( m_Type.Compare(_T("ECDSA")) == 0 )
		m_GenIdKeyType = IDKEY_ECDSA;
	else if ( m_Type.Compare(_T("ED25519")) == 0 )
		m_GenIdKeyType = IDKEY_ED25519;

	if ( m_GenIdKeyType == IDKEY_ECDSA && m_GenIdKeyBits > 521 ) {
		if ( MessageBox(CStringLoad(IDE_ECDSABITSIZEERR), _T("Warning"), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return;
	} else if ( (m_GenIdKeyType == IDKEY_RSA1 || m_GenIdKeyType == IDKEY_RSA2) && m_GenIdKeyBits <= 1024 ) {
		if ( MessageBox(CStringLoad(IDE_RSABITSIZEERR), _T("Warning"), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return;
	} else if ( m_GenIdKeyType == IDKEY_DSA2 && (m_GenIdKeyBits < 768 || m_GenIdKeyBits > 1024) ) {
		if ( MessageBox(CStringLoad(IDE_DSABITSIZEERR), _T("Warning"), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return;
	}

	dlg.m_OpenMode = 3;
	dlg.m_Title.LoadString(IDS_IDKEYCREATE);
	dlg.m_Message.LoadString(IDS_IDKEYCREATECOM);

	if ( dlg.DoModal() != IDOK )
		return;

	if ( dlg.m_PassName.IsEmpty() ) {
		if ( MessageBox(CStringLoad(IDE_USEPASSWORDIDKEY), _T("Warning"), MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return;
	}

	if ( (pWnd = GetDlgItem(IDC_IDKEY_TYPE)) != NULL )
		pWnd->EnableWindow(FALSE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_BITS)) != NULL )
		pWnd->EnableWindow(FALSE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_NAME)) != NULL )
		pWnd->EnableWindow(FALSE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_CREATE)) != NULL )
		pWnd->EnableWindow(FALSE);

	m_GenIdKeyPass  = dlg.m_PassName;
	m_GenIdKeyMax   = (int)(pow(1.5, (double)m_GenIdKeyBits / 256.0) * 2.56);
	m_GenIdKeyCount = 0;

	m_KeyGenProg.EnableWindow(TRUE);
	m_KeyGenProg.SetRange(0, m_GenIdKeyMax);
	m_KeyGenProg.SetPos(m_GenIdKeyCount);

	StartKeyGenThead();
}
void CIdkeySelDLg::OnKeyGenEndof()
{
	CWnd *pWnd;

	if ( (pWnd = GetDlgItem(IDC_IDKEY_TYPE)) != NULL )
		pWnd->EnableWindow(TRUE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_BITS)) != NULL )
		pWnd->EnableWindow(TRUE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_NAME)) != NULL )
		pWnd->EnableWindow(TRUE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_CREATE)) != NULL )
		pWnd->EnableWindow(TRUE);

	m_KeyGenProg.SetPos(0);
	m_KeyGenProg.EnableWindow(FALSE);

	if ( !m_GenIdKeyStat || !m_pIdKeyTab->AddEntry(m_GenIdKey) ) {
		MessageBox(CStringLoad(IDE_IDKEYCREATEERROR));
		return;
	}
	m_Data.InsertAt(0, m_GenIdKey.m_Uid);

	m_EntryNum = 0;
	InitList();
}
void CIdkeySelDLg::OnEditUpdate() 
{
	if ( (m_EntryNum = m_List.GetSelectionMark()) < 0 )
		return;

	int n = (int)m_List.GetItemData(m_EntryNum);
	CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[n]);
	CEditDlg dlg;

	if ( pKey == NULL )
		return;

	dlg.m_Title.LoadString(IDS_IDKEYRENAME);
	dlg.m_Edit  = pKey->m_Name;

	if ( dlg.DoModal() != IDOK )
		return;

	pKey->m_Name = dlg.m_Edit;
	m_pIdKeyTab->UpdateUid(pKey->m_Uid);

	InitList();
}
void CIdkeySelDLg::OnEditCheck()
{
	int n, i;
	CIdKeyTab tab;
	CString nstr, ostr;
	CWordArray apend, remove, update;

	for ( n = 0 ; n < tab.GetSize() ; n++ ) {
		for ( i = 0 ; i < m_pIdKeyTab->GetSize() ; i++ ) {
			if ( tab.GetAt(n).m_Uid == m_pIdKeyTab->GetAt(i).m_Uid )
				break;
		}
		if ( i < m_pIdKeyTab->GetSize() ) {
			tab.GetAt(n).SetString(nstr);
			m_pIdKeyTab->GetAt(i).SetString(ostr);
			if ( nstr.Compare(ostr) != 0 ) {
				update.Add(i);
				update.Add(n);
			}
		} else
			apend.Add(n);
	}

	for ( i = 0 ; i < m_pIdKeyTab->GetSize() ; i++ ) {
		for ( n = 0 ; n < tab.GetSize() ; n++ ) {
			if ( tab.GetAt(n).m_Uid == m_pIdKeyTab->GetAt(i).m_Uid )
				break;
		}
		if ( n >= tab.GetSize() )
			remove.Add(i);
	}

	for ( n = 0 ; n < update.GetSize() ; n += 2 )
		m_pIdKeyTab->m_Data[update[n]] = tab.GetAt(update[n + 1]);

	for ( n = 0 ; n < apend.GetSize() ; n++ ) {
		m_pIdKeyTab->m_Data.Add(tab.GetAt(apend[n]));
		m_Data.Add(tab.GetAt(apend[n]).m_Uid);
	}

	for ( n = 0 ; n < remove.GetSize() ; n++ ) {
		for ( i = 0 ; i < m_Data.GetSize() ; i++ ) {
			if ( m_Data[i] == m_pIdKeyTab->GetAt(remove[n]).m_Uid ) {
				m_Data.RemoveAt(i);
				break;
			}
		}
		m_pIdKeyTab->m_Data.RemoveAt(remove[n]);
	}

	InitList();
}
void CIdkeySelDLg::OnDblclkIdkeyList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	OnEditUpdate();
}
void CIdkeySelDLg::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}
void CIdkeySelDLg::OnLvnItemchangedIdkeyList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( m_ListInit == FALSE && (pNMLV->uNewState & LVIS_STATEIMAGEMASK) != (pNMLV->uOldState & LVIS_STATEIMAGEMASK) ) {
		int n = (int)m_List.GetItemData(pNMLV->iItem);
		CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[n]);
		if ( pKey != NULL )
			pKey->m_Flag = (m_List.GetLVCheck(pNMLV->iItem) ? TRUE : FALSE);
	}
	*pResult = 0;
}

void CIdkeySelDLg::OnIdkeyCakey()
{
	if ( (m_EntryNum = m_List.GetSelectionMark()) < 0 )
		return;

	int n = (int)m_List.GetItemData(m_EntryNum);
	CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[n]);

	if ( pKey == NULL )
		return;

	CFileDialog dlg(TRUE, _T(""), _T(""), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( dlg.DoModal() != IDOK )
		return;

	if ( !pKey->LoadCertPublicKey(dlg.GetPathName()) )
		MessageBox(CStringLoad(IDE_IDKEYLOADERROR));

	m_pIdKeyTab->UpdateUid(pKey->m_Uid);
	InitList();
}
void CIdkeySelDLg::OnSavePublicKey()
{
	if ( (m_EntryNum = m_List.GetSelectionMark()) < 0 )
		return;

	int n = (int)m_List.GetItemData(m_EntryNum);
	CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[n]);

	if ( pKey == NULL )
		return;

	CFileDialog dlg(FALSE, _T(""), _T(""), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( dlg.DoModal() != IDOK )
		return;

	if ( !pKey->SavePublicKey(dlg.GetPathName()) )
		MessageBox(CStringLoad(IDE_IDKEYSAVEERROR));
}