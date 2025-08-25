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
	m_ExportStyle = EXPORT_STYLE_OPENSSH;
	m_Type = _T("ED25519");
	m_Bits = _T("");
	m_Name = _T("");

	m_pParamTab = NULL;
	m_pIdKeyTab = NULL;

	m_EntryNum = (-1);
	m_pKeyGenEvent = new CEvent(FALSE, TRUE);
	m_KeyGenFlag = 0;
	m_GenIdKeyTimer = NULL;
	m_GenElapse = 200;

	m_GenIdKeyType = 0;
	m_GenIdKeyBits = 0;
	m_GenIdKeyStat = FALSE;
	m_GenIdKeyMax = 0;
	m_GenIdKeyCount = 0;
	ZeroMemory(&m_GenStat, sizeof(m_GenStat));

	m_ListInit = FALSE;
	m_bInitPageant = FALSE;
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
	DDX_CBIndex(pDX, IDC_EXPORT_STYLE, m_ExportStyle);
	DDX_CBStringExact(pDX, IDC_IDKEY_TYPE, m_Type);
	DDX_CBStringExact(pDX, IDC_IDKEY_BITS, m_Bits);
	DDX_Text(pDX, IDC_IDKEY_NAME, m_Name);
}

BEGIN_MESSAGE_MAP(CIdkeySelDLg, CDialogExt)
	ON_WM_TIMER()

	ON_NOTIFY(NM_DBLCLK, IDC_IDKEY_LIST, OnDblclkIdkeyList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IDKEY_LIST, OnLvnItemchangedIdkeyList)

	ON_BN_CLICKED(IDC_IDKEY_UP, OnIdkeyUp)
	ON_UPDATE_COMMAND_UI(IDC_IDKEY_UP, OnUpdateEditEntry)
	ON_BN_CLICKED(IDC_IDKEY_DOWN, OnIdkeyDown)
	ON_UPDATE_COMMAND_UI(IDC_IDKEY_DOWN, OnUpdateEditEntry)
	ON_BN_CLICKED(IDC_IDKEY_COPY, OnIdkeyCopy)
	ON_BN_CLICKED(IDC_IDKEY_INPORT, OnIdkeyInport)
	ON_BN_CLICKED(IDC_IDKEY_EXPORT, OnIdkeyExport)
	ON_UPDATE_COMMAND_UI(IDC_IDKEY_EXPORT, OnUpdateEditEntry)
	ON_BN_CLICKED(IDC_IDKEY_CREATE, OnIdkeyCreate)

	ON_COMMAND(ID_EDIT_UPDATE, OnEditUpdate)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_COMMAND(ID_EDIT_DELETE, OnIdkeyDel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_COMMAND(ID_EDIT_COPY, OnIdkeyCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditEntry)
	ON_COMMAND(ID_EDIT_CHECK, OnEditCheck)
	ON_COMMAND(ID_IDKEY_CAKEY, &CIdkeySelDLg::OnIdkeyCakey)
	ON_COMMAND(IDM_SAVEPUBLICKEY, &CIdkeySelDLg::OnSavePublicKey)
	ON_UPDATE_COMMAND_UI(IDM_SAVEPUBLICKEY, OnUpdateEditEntry)
	ON_CBN_SELCHANGE(IDC_IDKEY_TYPE, &CIdkeySelDLg::OnCbnSelchangeIdkeyType)
END_MESSAGE_MAP()

void CIdkeySelDLg::InitList()
{
	int n, bits;
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
		case IDKEY_ED448:   str = _T("ED448"); break;
		case IDKEY_ML_DSA:	str = _T("MLDSA"); break;
		case IDKEY_ML_DSA_ES	:str = _T("MLDSA-ES"); break;
		case IDKEY_ML_DSA_ED	:str = _T("MLDSA-ED"); break;
		case IDKEY_SLH_DSA:	str = pKey->IsSlhDsaShake() ? (pKey->IsSlhDsaStype() ? _T("SLHSHAKES") : _T("SLHSHAKEF")) : (pKey->IsSlhDsaStype() ? _T("SLHSHA2S") : _T("SLHSHA2F")); break;
		case IDKEY_XMSS:    str = _T("XMSS"); break;
		case IDKEY_UNKNOWN: str = pKey->m_TypeName; break;
		}
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);

		if ( (bits = pKey->GetHeight()) > 0 )
			str.Format(_T("%d"), pKey->GetHeight());
		else
			str.Empty();
		m_List.SetItemText(n, 1, str);

		m_List.SetItemText(n, 2, pKey->m_Name);

		if ( pKey->m_AgeantType == IDKEY_AGEANT_PUTTY ) {
			str = pKey->m_bSecInit ? _T("Pageant") : _T("None");
		} else if ( pKey->m_AgeantType == IDKEY_AGEANT_WINSSH ) {
			str = pKey->m_bSecInit ? _T("Wageant") : _T("None");
		} else if ( pKey->m_AgeantType == IDKEY_AGEANT_PUTTYPIPE ) {
			str = pKey->m_bSecInit ? _T("Pageant") : _T("None");
		} else if ( pKey->m_Type == IDKEY_UNKNOWN ) {
			str = _T("Unknown");
		} else {
			switch(pKey->m_Cert) {
			case IDKEY_CERTV00:  str = _T("V00"); break;
			case IDKEY_CERTV01:  str = _T("V01"); break;
			case IDKEY_CERTX509: str = _T("x509"); break;
			default:             str = _T(""); break;
			}
		}
		m_List.SetItemText(n, 3, str);

		m_List.SetItemData(n, n);
		m_List.SetLVCheck(n, pKey->m_Flag);
	}

	if ( m_EntryNum >= 0 ) {
		m_List.SetItemState(m_EntryNum, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_List.EnsureVisible(m_EntryNum, FALSE);
	}

	m_ListInit = FALSE;
}
void CIdkeySelDLg::SetBitsList()
{
	int bits;
	CComboBox *pCombo;

	if ( (pCombo = (CComboBox *)this->GetDlgItem(IDC_IDKEY_BITS)) == NULL )
		return;

	bits = _tstoi(m_Bits);

	for ( int n = pCombo->GetCount() - 1 ;  n >= 0 ; n-- )
		pCombo->DeleteString(n);

	if ( m_Type.Compare(_T("RSA1")) == 0 || m_Type.Compare(_T("RSA2")) == 0 ) {
		pCombo->AddString(_T("2048"));
		pCombo->AddString(_T("3072"));
		pCombo->AddString(_T("4096"));
		pCombo->AddString(_T("6144"));
		pCombo->AddString(_T("8192"));
		pCombo->AddString(_T("12288"));
		pCombo->AddString(_T("16384"));
		pCombo->EnableWindow(TRUE);
		if ( bits < 2048 ) bits = 3072;
	} else if ( m_Type.Compare(_T("DSA2")) == 0 ) {
		//pCombo->AddString(_T("768"));
		pCombo->AddString(_T("1024"));
		pCombo->EnableWindow(TRUE);
		if ( bits < 768 || bits > 1024 ) bits = 1024;
	} else if ( m_Type.Compare(_T("ECDSA")) == 0 ) {
		pCombo->AddString(_T("256"));
		pCombo->AddString(_T("384"));
		pCombo->AddString(_T("521"));
		pCombo->EnableWindow(TRUE);
		if ( bits < 256 || bits > 521 ) bits = 256;
	} else if ( m_Type.Compare(_T("XMSS")) == 0 ) {
		pCombo->AddString(_T("10"));
		pCombo->AddString(_T("16"));
		pCombo->AddString(_T("20"));
		pCombo->EnableWindow(TRUE);
		if ( bits < 10 || bits > 20 ) bits = 10;
	} else if ( m_Type.Compare(_T("MLDSA")) == 0 || m_Type.Compare(_T("MLDSA-ES")) == 0 || m_Type.Compare(_T("MLDSA-ED")) == 0 ) {
		pCombo->AddString(_T("44"));
		pCombo->AddString(_T("65"));
		pCombo->AddString(_T("87"));
		pCombo->EnableWindow(TRUE);
		if ( bits < 44 || bits > 87 ) bits = 87;
	} else if ( m_Type.Compare(_T("SLHSHA2S"))  == 0 || m_Type.Compare(_T("SLHSHA2F"))  == 0 ||
			    m_Type.Compare(_T("SLHSHAKES")) == 0 || m_Type.Compare(_T("SLHSHAKEF")) == 0 ) {
		pCombo->AddString(_T("128"));
		pCombo->AddString(_T("192"));
		pCombo->AddString(_T("256"));
		pCombo->EnableWindow(TRUE);
		if ( bits < 128 || bits > 256 ) bits = 256;
	} else {
		pCombo->EnableWindow(FALSE);
	}

	m_Bits.Format(_T("%d"), bits);
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
	AfxBeginThread(KeyGenThread, this, THREAD_PRIORITY_NORMAL);
	m_GenIdKeyTimer = (UINT)SetTimer(1024, m_GenElapse, NULL);
}
void CIdkeySelDLg::ProcKeyGenThead()
{
	m_GenStat.abort = 0;
	m_GenIdKeyStat = m_GenIdKey.Generate(m_GenIdKeyType, m_GenIdKeyBits, m_GenIdKeyPass, &m_GenStat);
	// m_GenIdKey.Generateでopenssl関数を使用
	OPENSSL_thread_stop();
}
void CIdkeySelDLg::EndofKeyGenThead()
{
	if ( m_KeyGenFlag == 0 )
		return;
	m_KeyGenFlag = 0;
	WaitForEvent(m_pKeyGenEvent->m_hObject, _T("KeyGen Thread"));
	OnKeyGenEndof();
}

/////////////////////////////////////////////////////////////////////////////
// CIdkeySelDLg メッセージ ハンドラ

static const LV_COLUMN InitListTab[4] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,   80, _T("Type"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,   40, _T("Bits"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,   80, _T("Name"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,   60, _T("Ext"), 0, 0 }, 
	};

static const INITDLGTAB ItemTab[] = {
	{ IDC_TITLE1,		ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ IDC_TITLE2,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE3,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_IDKEY_LIST,	ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ IDC_IDKEY_UP,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_IDKEY_DOWN,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_IDKEY_COPY,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_IDKEY_INPORT,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_EXPORT_STYLE,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_IDKEY_EXPORT,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_IDKEY_TYPE,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_IDKEY_BITS,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_IDKEY_NAME,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_IDKEY_CREATE,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_IDKEY_PROG,	ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM  },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM  },

	{ IDC_TITLE4,		ITM_RIGHT_RIGHT },
	{ IDC_TITLE5,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE6,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE7,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE8,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE9,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE10,		ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM},

	{ 0,	0 },
};

BOOL CIdkeySelDLg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	int n, i, a;
	CIdKey *pKey;
	CWnd *pWnd;

	ASSERT(m_pIdKeyTab != NULL);

	InitItemOffset(ItemTab);

	m_Data.RemoveAll();
	for ( n = a = 0 ; n < m_pIdKeyTab->GetSize() ; n++ ) {
		if ( m_pIdKeyTab->GetAt(n).m_AgeantType != IDKEY_AGEANT_NONE ) {
			m_bInitPageant = TRUE;
			if ( m_pParamTab != NULL && !m_pParamTab->m_bInitPageant ) {
				m_Data.InsertAt(a++, m_pIdKeyTab->GetAt(n).m_Uid);
				m_pIdKeyTab->GetAt(n).m_Flag = TRUE;
				continue;
			}
		}
		m_Data.Add(m_pIdKeyTab->GetAt(n).m_Uid);
		m_pIdKeyTab->GetAt(n).m_Flag = FALSE;
	}

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

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | (m_pParamTab != NULL ? LVS_EX_CHECKBOXES : 0));
	m_List.InitColumn(_T("IdkeySelDLg"), InitListTab, 4);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 2);
	m_List.m_bMove = TRUE;
	InitList();

	m_KeyGenProg.EnableWindow(FALSE);

	m_CreateStr = _T("Create");
	m_CancelStr = _T("Cancel");

	if ( (pWnd = GetDlgItem(IDC_IDKEY_CREATE)) != NULL )
		pWnd->GetWindowText(m_CreateStr);

	if ( (pWnd = GetDlgItem(IDCANCEL)) != NULL )
		pWnd->GetWindowText(m_CancelStr);

	if ( m_pParamTab == NULL ) {
		if ( (pWnd = GetDlgItem(IDC_TITLE4)) != NULL )
			pWnd->EnableWindow(FALSE);
		if ( (pWnd = GetDlgItem(IDC_IDKEY_UP)) != NULL )
			pWnd->EnableWindow(FALSE);
		if ( (pWnd = GetDlgItem(IDC_IDKEY_DOWN)) != NULL )
			pWnd->EnableWindow(FALSE);
		if ( (pWnd = GetDlgItem(IDC_IDKEY_COPY)) != NULL )
			pWnd->EnableWindow(FALSE);
	}

	if ( (pWnd = GetDlgItem(IDC_IDKEY_TYPE)) != NULL ) {
		((CComboBox *)pWnd)->AddString(_T("MLDSA"));		// draft-sfluhrer-ssh-mldsa-04
		((CComboBox *)pWnd)->AddString(_T("MLDSA-ES"));		// draft-sun-ssh-composite-sigs-01
		((CComboBox *)pWnd)->AddString(_T("MLDSA-ED"));		// draft-sun-ssh-composite-sigs-01
		//((CComboBox *)pWnd)->AddString(_T("SLHSHA2S"));
		//((CComboBox *)pWnd)->AddString(_T("SLHSHA2F"));		// draft-josefsson-ssh-sphincs-00 (Expires 2025/08/08)
		//((CComboBox *)pWnd)->AddString(_T("SLHSHAKES"));
		//((CComboBox *)pWnd)->AddString(_T("SLHSHAKEF"));
	}

	SetBitsList();
	UpdateData(FALSE);

	SubclassComboBox(IDC_IDKEY_TYPE);
	SubclassComboBox(IDC_IDKEY_BITS);

	SetSaveProfile(_T("IdkeySelDLg"));
	AddHelpButton(_T("#SSHKEYSET"));

	return TRUE;
}
void CIdkeySelDLg::OnOK() 
{
	ASSERT(m_pIdKeyTab != NULL);

	if ( m_KeyGenFlag != 0 ) {
		if ( MessageBox(CStringLoad(IDE_MAKEIDKEY), NULL, MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL )
			m_GenStat.abort = 1;
		return;
	}

	m_IdKeyList.RemoveAll();
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_List.GetLVCheck(n) ) {
			int i = (int)m_List.GetItemData(n);
			m_IdKeyList.AddVal(m_Data[i]);
		}
	}

	if ( m_pParamTab != NULL && m_bInitPageant && !m_pParamTab->m_bInitPageant )
		m_pParamTab->m_bInitPageant = TRUE;

	m_List.SaveColumn(_T("IdkeySelDLg"));

	CDialogExt::OnOK();
}
void CIdkeySelDLg::OnCancel()
{
	if ( m_KeyGenFlag != 0 ) {
		if ( MessageBox(CStringLoad(IDE_MAKEIDKEY), NULL, MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL )
			m_GenStat.abort = 1;
		return;
	}
	CDialogExt::OnCancel();
}

void CIdkeySelDLg::OnTimer(UINT_PTR nIDEvent) 
{
	switch(nIDEvent) {
	case 1024:
		if ( m_KeyGenFlag == 2 ) {
			m_KeyGenProg.SetPos(m_GenIdKeyMax);
			KillTimer(m_GenIdKeyTimer);
			EndofKeyGenThead();
		} else if ( m_GenStat.max > 0 ) {
			if ( m_GenIdKeyMax != m_GenStat.max ) {
				m_GenIdKeyMax = m_GenStat.max;
				m_KeyGenProg.SetRange(0, (short)m_GenIdKeyMax);
			}
			if ( m_GenIdKeyCount != m_GenStat.pos ) {
				m_GenIdKeyCount = m_GenStat.pos;
				m_KeyGenProg.SetPos(m_GenIdKeyCount);
			}
		} else {
			m_KeyGenProg.SetPos(m_GenIdKeyCount++);
			if ( m_GenIdKeyCount >= m_GenIdKeyMax ) {
				int n = m_GenIdKeyMax / 5;
				m_GenIdKeyMax += (n <= 1 ? 2 : n);
				m_KeyGenProg.SetRange(0, m_GenIdKeyMax);
				m_KeyGenProg.SetPos(m_GenIdKeyCount);
			}
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
	CDWordArray UidTab;

	for ( int n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) != 0 )
			UidTab.Add(m_Data[(int)m_List.GetItemData(n)]);
	}

	if ( UidTab.GetSize() == 0 || MessageBox(CStringLoad(IDE_DELETEKEYQES), NULL, MB_ICONWARNING | MB_OKCANCEL) != IDOK )
		return;

	for ( int n = 0 ; n < (int)UidTab.GetSize() ; n++ ) {
		for ( int i = 0 ; n < (int)m_Data.GetSize() ; i++ ) {
			if ( UidTab[n] == m_Data[i] ) {
				m_pIdKeyTab->RemoveUid(m_Data[i]);
				m_Data.RemoveAt(i);
				break;
			}
		}
	}

	InitList();
}
void CIdkeySelDLg::OnIdkeyCopy() 
{
	int count = 0;
	CString str, tmp;

	for ( int n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;

		CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[(int)m_List.GetItemData(n)]);

		if ( pKey == NULL )
			continue;

		pKey->WritePublicKey(tmp);
		str += tmp;
		str += _T("\r\n");
		count++;
	}

	if ( count <= 0 )
		return;

	tmp.Format(CStringLoad(IDS_PUBLICKEYCOPYMSG), count);

	if ( MessageBox(tmp, NULL, MB_ICONINFORMATION | MB_YESNO) == IDYES )
		((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(str);
}
void CIdkeySelDLg::OnIdkeyInport() 
{
	CIdKey key;
	CIdKeyFileDlg dlg(this);

	dlg.m_OpenMode = IDKFDMODE_LOAD;
	dlg.m_Title.LoadString(IDS_IDKEYFILELOAD);
	dlg.m_Message.LoadString(IDS_IDKEYFILELOADCOM);

	if ( dlg.DoModal() != IDOK )
		return;

	if ( dlg.m_IdkeyFile.IsEmpty() ) {
		MessageBox(CStringLoad(IDE_USEIDKEYFILENAME), NULL, MB_ICONINFORMATION);
		return;
	}

	if ( !key.LoadPrivateKey(dlg.m_IdkeyFile, dlg.m_PassName) ) {
		MessageBox(CStringLoad(IDE_IDKEYLOADERROR), NULL, MB_ICONINFORMATION);
		return;
	} else if ( key.IsNotSupport() ) {
		MessageBox(CStringLoad(IDE_IDKEYNOTSUPPORT), NULL, MB_ICONINFORMATION);
		return;
	}

	key.m_Flag = TRUE;
	key.m_FilePath.Empty();

	if ( !m_pIdKeyTab->AddEntry(key) ) {
		CString msg, finger;
		key.FingerPrint(finger, SSHFP_DIGEST_SHA256, SSHFP_FORMAT_SIMPLE);
		msg.Format(_T("%s\n\n%s(%d) %s\n%32.32s..."), CStringLoad(IDE_DUPIDKEYENTRY), key.GetName(), key.GetSize(), (LPCTSTR)key.m_Name, (LPCTSTR)finger);
		MessageBox(msg, NULL, MB_ICONINFORMATION);
		return;
	}
	m_Data.InsertAt(0, key.m_Uid);

	m_EntryNum = 0;
	InitList();
}
void CIdkeySelDLg::OnIdkeyExport() 
{
	CIdKeyFileDlg dlg(this);
	CString finger;

	UpdateData(TRUE);

	for ( int n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;

		CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[(int)m_List.GetItemData(n)]);

		if ( pKey == NULL )
			continue;

		if ( pKey->m_AgeantType != IDKEY_AGEANT_NONE ) {
			MessageBox(CStringLoad(IDE_PAGEANTKEYEDIT), NULL, MB_ICONINFORMATION);
			continue;
		}

		pKey->FingerPrint(finger, SSHFP_DIGEST_SHA256, SSHFP_FORMAT_SIMPLE);

		dlg.m_OpenMode = IDKFDMODE_SAVE;
		dlg.m_Title.LoadString(IDS_IDKEYFILESAVE);
		dlg.m_Message.Format(_T("%s%s\n\n%s(%d) %s\n%32.32s..."), 
			(LPCTSTR)CStringLoad(IDS_IDKEYFILESAVECOM), 
			(pKey->SaveFileFormat(m_ExportStyle) != m_ExportStyle ? (LPCTSTR)CStringLoad(IDS_XMSSSAVEFORMAT) : _T("")),
			pKey->GetName(), pKey->GetSize(), (LPCTSTR)pKey->m_Name, (LPCTSTR)finger);

		for ( ; ; ) {
			if ( dlg.DoModal() != IDOK )
				return;

			if ( dlg.m_IdkeyFile.IsEmpty() ) {
				MessageBox(CStringLoad(IDE_USEIDKEYFILENAME), NULL, MB_ICONERROR);
				continue;
			}

			if ( pKey->CompPass(dlg.m_PassName) != 0 ) {
				MessageBox(CStringLoad(IDE_IDKEYPASSERROR), NULL, MB_ICONERROR);
				continue;
			}

			if ( _taccess(dlg.m_IdkeyFile, 00) == 0 ) {
				if ( MessageBox(CStringLoad(IDS_FILEOVERWRITEMSG), NULL, MB_YESNO | MB_ICONWARNING) != IDYES )
					continue;
			}

			break;
		}

		if ( !pKey->SavePrivateKey(m_ExportStyle, dlg.m_IdkeyFile, dlg.m_PassName) )
			MessageBox(CStringLoad(IDE_IDKEYSAVEERROR), NULL, MB_ICONERROR);
	}
}
void CIdkeySelDLg::OnIdkeyCreate() 
{
	CWnd *pWnd;
	CIdKeyFileDlg dlg(this);
	CString tmp;

	if ( m_KeyGenFlag != 0 ) {
		m_GenStat.abort = 1;
		return;
	}

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
	else if ( m_Type.Compare(_T("ED448")) == 0 )
		m_GenIdKeyType = IDKEY_ED448;
	else if ( m_Type.Compare(_T("XMSS")) == 0 )
		m_GenIdKeyType = IDKEY_XMSS;
	else if ( m_Type.Compare(_T("MLDSA")) == 0 )
		m_GenIdKeyType = IDKEY_ML_DSA;
	else if ( m_Type.Compare(_T("MLDSA-ES")) == 0 )
		m_GenIdKeyType = IDKEY_ML_DSA_ES;
	else if ( m_Type.Compare(_T("MLDSA-ED")) == 0 )
		m_GenIdKeyType = IDKEY_ML_DSA_ED;
	else if ( m_Type.Compare(_T("SLHSHA2F"))  == 0 || m_Type.Compare(_T("SLHSHA2S"))  == 0 ||
			  m_Type.Compare(_T("SLHSHAKEF")) == 0 || m_Type.Compare(_T("SLHSHAKES")) == 0 )
		m_GenIdKeyType = IDKEY_SLH_DSA;


	if ( m_GenIdKeyType == IDKEY_ECDSA && (m_GenIdKeyBits < 256 || m_GenIdKeyBits > 521) ) {
		MessageBox(CStringLoad(IDE_ECDSABITSIZEERR), NULL, MB_ICONERROR);
		return;
	} else if ( (m_GenIdKeyType == IDKEY_RSA1 || m_GenIdKeyType == IDKEY_RSA2) && m_GenIdKeyBits <= 2048 ) {
		if ( MessageBox(CStringLoad(IDE_RSABITSIZEERR), NULL, MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return;
	} else if ( m_GenIdKeyType == IDKEY_DSA2 && (m_GenIdKeyBits < 768 || m_GenIdKeyBits > 1024) ) {
		if ( MessageBox(CStringLoad(IDE_DSABITSIZEERR), NULL, MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return;
	} else if ( m_GenIdKeyType == IDKEY_XMSS && (m_GenIdKeyBits < 10 || m_GenIdKeyBits > 20) ) {
		MessageBox(CStringLoad(IDE_XMSSBITSIZEERR), NULL, MB_ICONWARNING);
		return;
	} else if ( (m_GenIdKeyType == IDKEY_ML_DSA || m_GenIdKeyType == IDKEY_ML_DSA_ES || m_GenIdKeyType == IDKEY_ML_DSA_ED) && (m_GenIdKeyBits < 44 || m_GenIdKeyBits > 87) ) {
		MessageBox(_T("ml-dsa size error use 44/65/87"), NULL, MB_ICONERROR);
		return;
	} else if ( m_GenIdKeyType == IDKEY_SLH_DSA && (m_GenIdKeyBits < 128 || m_GenIdKeyBits > 256) ) {
		MessageBox(_T("slh-dsa size error use 128/192/256"), NULL, MB_ICONERROR);
		return;
	}

	dlg.m_OpenMode = IDKFDMODE_CREATE;
	dlg.m_Title.LoadString(IDS_IDKEYCREATE);
	dlg.m_Message.LoadString(IDS_IDKEYCREATECOM);

	if ( dlg.DoModal() != IDOK )
		return;

	if ( dlg.m_PassName.IsEmpty() ) {
		if ( MessageBox(CStringLoad(IDE_USEPASSWORDIDKEY), NULL, MB_ICONWARNING | MB_OKCANCEL) != IDOK )
			return;
	}

	if ( (pWnd = GetDlgItem(IDC_IDKEY_TYPE)) != NULL )
		pWnd->EnableWindow(FALSE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_BITS)) != NULL )
		pWnd->EnableWindow(FALSE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_NAME)) != NULL )
		pWnd->EnableWindow(FALSE);
	if ( (pWnd = GetDlgItem(IDC_IDKEY_CREATE)) != NULL )
		pWnd->SetWindowText(m_CancelStr);

	m_GenIdKeyPass  = dlg.m_PassName;
	m_GenIdKeyCount = 0;

	m_GenElapse = 200;		// ms
	m_GenIdKeyMax = 10;		// 200 * 10 = 2sec

	m_KeyGenProg.EnableWindow(TRUE);
	m_KeyGenProg.SetRange(0, m_GenIdKeyMax);
	m_KeyGenProg.SetPos(m_GenIdKeyCount);

	m_GenStat.abort = 0;
	m_GenStat.type = m_GenIdKeyType;
	m_GenStat.max = m_GenStat.pos = 0;

	if ( m_Type.Compare(_T("SLHSHA2S")) == 0 || m_Type.Compare(_T("SLHSHAKES")) == 0 )
		m_GenIdKeyBits |= 0x10000;
	if ( m_Type.Compare(_T("SLHSHAKEF")) == 0 || m_Type.Compare(_T("SLHSHAKES")) == 0 )
		m_GenIdKeyBits |= 0x20000;

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
		pWnd->SetWindowText(m_CreateStr);

	m_KeyGenProg.SetPos(0);
	m_KeyGenProg.EnableWindow(FALSE);

	if ( !m_GenIdKeyStat || !m_pIdKeyTab->AddEntry(m_GenIdKey) ) {
		MessageBox(CStringLoad(IDE_IDKEYCREATEERROR), NULL, MB_ICONINFORMATION);
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
	CEditDlg dlg(this);

	if ( pKey == NULL )
		return;

	if ( pKey->m_AgeantType != IDKEY_AGEANT_NONE ) {
		MessageBox(CStringLoad(IDE_PAGEANTKEYEDIT), NULL, MB_ICONINFORMATION);
		return;
	}

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

	if ( pKey->m_AgeantType != IDKEY_AGEANT_NONE ) {
		MessageBox(CStringLoad(IDE_PAGEANTKEYEDIT), NULL, MB_ICONINFORMATION);
		return;
	}

	CFileDialog dlg(TRUE, _T(""), _T(""), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	if ( !pKey->LoadCertPublicKey(dlg.GetPathName()) )
		MessageBox(CStringLoad(IDE_IDKEYLOADERROR), NULL, MB_ICONERROR);

	m_pIdKeyTab->UpdateUid(pKey->m_Uid);
	InitList();
}
void CIdkeySelDLg::OnSavePublicKey()
{
	for ( int n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemState(n, LVIS_SELECTED) == 0 )
			continue;

		CIdKey *pKey = m_pIdKeyTab->GetUid(m_Data[(int)m_List.GetItemData(n)]);

		if ( pKey == NULL )
			continue;

		CFileDialog dlg(FALSE, _T(""), _T(""), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), this);

		if ( DpiAwareDoModal(dlg) != IDOK )
			return;

		if ( !pKey->SavePublicKey(dlg.GetPathName()) )
			MessageBox(CStringLoad(IDE_IDKEYSAVEERROR), NULL, MB_ICONERROR);
	}
}

void CIdkeySelDLg::OnCbnSelchangeIdkeyType()
{
	UpdateData(TRUE);

	SetBitsList();

	UpdateData(FALSE);
}
