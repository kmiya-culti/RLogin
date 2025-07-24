// SshSigDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "SshSigDlg.h"
#include "IdKeyFileDlg.h"
#include "IdkeySelDLg.h"

#include <sys/stat.h>

// CSshSigDlg ダイアログ

IMPLEMENT_DYNAMIC(CSshSigDlg, CDialogExt)

CSshSigDlg::CSshSigDlg(CWnd* pParent /*=NULL*/)	: CDialogExt(CSshSigDlg::IDD, pParent)
{
	m_DataFile.Empty();
	m_KeyList = 0;
	m_KeyFile.Empty();
	m_NameSpace = _T("RLogin");
	m_Sign.Empty();
	m_KeyMode = 0;
	m_StrictVerify = FALSE;
	m_ThreadStat = 0;
	m_HashFp = NULL;
	m_HashCtx = NULL;
	m_ThreadStat = 0;
	m_pKey = NULL;
	m_HashFp = NULL;
	m_HashCtx = NULL;
	m_HashTimer = 0;
	m_HashFileSize = m_HashFilePos = 0;
}

CSshSigDlg::~CSshSigDlg()
{
}

void CSshSigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_CBStringExact(pDX, IDC_SSHSIG_DATAFILE, m_DataFile);
	DDX_CBIndex(pDX, IDC_SSHSIG_KEYLIST, m_KeyList);
	DDX_CBStringExact(pDX, IDC_SSHSIG_KEYFILE, m_KeyFile);
	DDX_CBStringExact(pDX, IDC_SSHSIG_NAMESPACE, m_NameSpace);
	DDX_Radio(pDX, IDC_SSHSIG_KEYSEL1, m_KeyMode);
	DDX_Check(pDX, IDC_SSHSIG_VERIFY_WITHKEY, m_StrictVerify);

	DDX_Text(pDX, IDC_SSHSIG_SIGN, m_Sign);
	DDX_Control(pDX, IDC_SSHSIG_PROG, m_HashProg);

	DDX_Control(pDX, IDC_SSHSIG_DATAFILE, m_DataFileCombo);
	DDX_Control(pDX, IDC_SSHSIG_KEYLIST, m_KeyListCombo);
	DDX_Control(pDX, IDC_SSHSIG_KEYFILE, m_KeyFileCombo);
	DDX_Control(pDX, IDC_SSHSIG_NAMESPACE, m_NameSpaceCombo);
}

CIdKey *CSshSigDlg::LoadKey()
{
	CIdKey *pKey = NULL;
	CIdKeyFileDlg dlg(this);
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( m_KeyMode == 0 ) {
		int uid = (int)m_KeyListCombo.GetItemData(m_KeyList);
		for ( int n = 0 ; n < m_IdKeyTab.GetSize() ; n++ ) {
			if ( uid == m_IdKeyTab[n].m_Uid ) {
				pKey = &(m_IdKeyTab[n]);
				break;
			}
		}

	} else if ( m_TempKey.m_bSecInit && m_TempKey.m_FilePath.Compare(m_KeyFile) == 0 ) {
		pKey = &m_TempKey;
	} else if ( m_TempKey.LoadPrivateKey(m_KeyFile, _T("")) ) {
		pKey = &m_TempKey;
	} else if ( _taccess(m_KeyFile, 04) == 0 ){
		dlg.m_OpenMode  = IDKFDMODE_OPEN;
		dlg.m_Title.LoadString(IDS_SSH_PASS_TITLE);		// = _T("SSH鍵ファイルの読み込み");
		dlg.m_Message.LoadString(IDS_SSH_PASS_MSG);		// = _T("作成時に設定したパスフレーズを入力してください");
		if ( !m_TempKey.m_LoadMsg.IsEmpty() ) {
			dlg.m_Message += _T("\n\n");
			dlg.m_Message += m_TempKey.m_LoadMsg;
		}
		dlg.m_IdkeyFile = m_KeyFile;

		if ( dlg.DoModal() != IDOK )
			return NULL;

		if ( m_TempKey.LoadPrivateKey(dlg.m_IdkeyFile, dlg.m_PassName) )
			pKey = &m_TempKey;
	}

	if ( pKey == NULL )
		MessageBox(CStringLoad(IDE_IDKEYLOADERROR), NULL, MB_ICONERROR);

	return pKey;
}
BOOL CSshSigDlg::ThreadCheck()
{
	if ( m_ThreadStat == 0 )
		return FALSE;

	if ( MessageBox(CStringLoad(IDT_SSHSIG_CANCEL), NULL, MB_ICONQUESTION | MB_YESNO) != IDYES )
		return TRUE;

	m_ThreadStat = 0;
	WaitForSingleObject(m_ThreadEvent, INFINITE);

	return FALSE;
}
FILE *CSshSigDlg::FileOpen(LPCTSTR filename)
{
	FILE *fp;
	struct _stati64 st;

	if ( _tstati64(filename, &st) || (st.st_mode & _S_IFMT) != _S_IFREG )
		return NULL;

	if ( (fp = _tfopen(filename, _T("rb"))) == NULL )
		return NULL;

	m_HashFileSize = st.st_size;
	m_HashFilePos = 0;

	if ( (m_HashFileDivs = (int)(m_HashFileSize / 1024)) <= 0 )
		m_HashFileDivs = 1;

	m_HashProg.SetRange32(0, (int)(m_HashFileSize / m_HashFileDivs));
	m_HashProg.SetPos(0);

	return fp;
}
void CSshSigDlg::SaveProfile()
{
	UpdateData(TRUE);

	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	pApp->WriteProfileString(_T("SshSigDlg"), _T("DataFile"), m_DataFile);
	pApp->WriteProfileString(_T("SshSigDlg"), _T("KeyFile"), m_KeyFile);
	pApp->WriteProfileString(_T("SshSigDlg"), _T("NameSpace"), m_NameSpace);
	pApp->WriteProfileInt(_T("SshSigDlg"), _T("KeyUid"), (int)m_KeyListCombo.GetItemData(m_KeyList));
	pApp->WriteProfileInt(_T("SshSigDlg"), _T("KeyMode"), m_KeyMode);
	pApp->GetProfileInt(_T("SshSigDlg"), _T("StrictVerify"), m_StrictVerify);

	m_DataFileCombo.AddHis(m_DataFile);
	m_KeyFileCombo.AddHis(m_KeyFile);
	m_NameSpaceCombo.AddHis(m_NameSpace);
}
void CSshSigDlg::KeyListInit(int uid)
{
	CString str;

	m_KeyListCombo.RemoveAll();

	for ( int n = 0 ; n < m_IdKeyTab.GetSize() ; n++ ) {
		switch(m_IdKeyTab[n].m_Type & IDKEY_TYPE_MASK) {
		case IDKEY_RSA1:
		case IDKEY_RSA2:
		case IDKEY_DSA2:
		case IDKEY_XMSS:
			str.Format(_T("%s %d"), m_IdKeyTab[n].GetName(), m_IdKeyTab[n].GetSize());
			break;
		default:
			str.Format(_T("%s"), m_IdKeyTab[n].GetName());
			break;
		}

		if ( !m_IdKeyTab[n].m_Name.IsEmpty() ) {
			str += _T(" (");
			str += m_IdKeyTab[n].m_Name;
			str += _T(")");
		}

		switch(m_IdKeyTab[n].m_AgeantType) {
		case IDKEY_AGEANT_NONE:
			if ( m_IdKeyTab[n].m_Type == IDKEY_UNKNOWN )
				str += _T(" Unkown");
			break;
		case IDKEY_AGEANT_PUTTY:
		case IDKEY_AGEANT_PUTTYPIPE:
			str += (m_IdKeyTab[n].m_bSecInit ? _T(" Pageant") : _T(" None"));
			break;
		case IDKEY_AGEANT_WINSSH:
			str += (m_IdKeyTab[n].m_bSecInit ? _T(" Wageant") : _T(" None"));
			break;
		}

		int i = m_KeyListCombo.AddString(str);
		m_KeyListCombo.SetItemData(i, (DWORD_PTR)m_IdKeyTab[n].m_Uid);
	}

	for ( int i = 0 ; i < m_KeyListCombo.GetCount() ; i++ ) {
		if ( (int)(m_KeyListCombo.GetItemData(i)) == uid ) {
			m_KeyList = i;
			break;
		}
	}
}

BEGIN_MESSAGE_MAP(CSshSigDlg, CDialogExt)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_SSHSIG_VERIFY, &CSshSigDlg::OnVerify)
	ON_BN_CLICKED(IDC_SSHSIG_DATAFILE_SEL, &CSshSigDlg::OnDatafileSel)
	ON_BN_CLICKED(IDC_SSHSIG_KEYLIST_SEL, &CSshSigDlg::OnKeylistSel)
	ON_BN_CLICKED(IDC_SSHSIG_KEYFILE_SEL, &CSshSigDlg::OnKeyfileSel)
	ON_BN_CLICKED(IDC_SSHSIG_INPORT, &CSshSigDlg::OnSignInport)
	ON_BN_CLICKED(IDC_SSHSIG_EXPORT, &CSshSigDlg::OnSignExport)
	ON_BN_CLICKED(IDC_SSHSIG_SIGN_COPY, &CSshSigDlg::OnSignCopy)
	ON_BN_CLICKED(IDC_SSHSIG_SIGN_PAST, &CSshSigDlg::OnSignPast)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_SSHSIG_KEYSEL1, IDC_SSHSIG_KEYSEL2, OnKeyMode)
	ON_COMMAND(IDM_SSHSIG_SIGNEXT, &CSshSigDlg::OnSignNext)
	ON_COMMAND(IDM_SSHSIG_VERIFYNEXT, &CSshSigDlg::OnVerifyNext)
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CSshSigDlg メッセージ ハンドラー

static const INITDLGTAB ItemTab[] = {
	{ IDC_SSHSIG_DATAFILE,			ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_DATAFILE_SEL,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },

	{ IDC_SSHSIG_KEYSEL1,			ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_KEYSEL2,			ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_KEYLIST,			ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_KEYFILE,			ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_KEYFILE_SEL,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_KEYLIST_SEL,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },

	{ IDC_SSHSIG_NAMESPACE,			ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_VERIFY_WITHKEY,	ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },

	{ IDC_SSHSIG_SIGN,				ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_BTM },
	{ IDC_SSHSIG_PROG,				ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_SSHSIG_INPORT,			ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_SSHSIG_EXPORT,			ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_SSHSIG_SIGN_PAST,			ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_SSHSIG_SIGN_COPY,			ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_SSHSIG_VERIFY,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDOK,							ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,						ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },

	{ IDC_TITLE1,					ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_TITLE2,					ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_TITLE3,					ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_TITLE4,					ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_BTM },

	{ IDC_TITLE5,					ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_TITLE6,					ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_TITLE7,					ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ 0,						0 },
};

BOOL CSshSigDlg::OnInitDialog()
{
	int uid;
	CWnd *pWnd;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	ASSERT(pApp != NULL && pMain != NULL);

	m_DataFile = pApp->GetProfileString(_T("SshSigDlg"), _T("DataFile"), _T(""));
	m_KeyFile = pApp->GetProfileString(_T("SshSigDlg"), _T("KeyFile"), _T(""));
	m_NameSpace = pApp->GetProfileString(_T("SshSigDlg"), _T("NameSpace"), _T("RLogin"));
	uid = pApp->GetProfileInt(_T("SshSigDlg"), _T("KeyUid"), (-1));
	m_KeyMode = pApp->GetProfileInt(_T("SshSigDlg"), _T("KeyMode"), 0);
	m_StrictVerify = pApp->GetProfileInt(_T("SshSigDlg"), _T("StrictVerify"), 0);

	pMain->AgeantInit();
	m_IdKeyTab = pMain->m_IdKeyTab;
	KeyListInit(uid);
	
	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYLIST)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 0 ? TRUE : FALSE);

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYFILE)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 1 ? TRUE : FALSE);

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYFILE_SEL)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 1 ? TRUE : FALSE);

	UpdateData(FALSE);

	m_DataFileCombo.LoadHis(_T("SshSigDataFile"));
	m_KeyFileCombo.LoadHis(_T("SshSigKeyFile"));
	m_NameSpaceCombo.LoadHis(_T("SshSigNameSpace"));

	SetLoadPosition(LOADPOS_PROFILE);
	SetSaveProfile(_T("SshSigDlg"));
	AddHelpButton(_T("#SSHSIG"));

	return TRUE;
}

void CSshSigDlg::OnClose()
{
	if ( ThreadCheck() )
		return;

	SaveProfile();

	CDialogExt::OnClose();
	DestroyWindow();
}
void CSshSigDlg::OnDestroy()
{
	if ( m_ThreadStat != 0 ) {
		m_ThreadStat = 0;
		WaitForSingleObject(m_ThreadEvent, INFINITE);
	}

	CDialogExt::OnDestroy();
}
void CSshSigDlg::PostNcDestroy()
{
	theApp.RemoveSshSigDlg(this);
	CDialogExt::PostNcDestroy();
	delete this;
}

void CSshSigDlg::OnCancel()
{
	if ( ThreadCheck() )
		return;

	SaveProfile();

	CDialogExt::OnCancel();
	DestroyWindow();
}

static UINT FileHashThread(LPVOID pParam)
{
	int n;
	BYTE tmp[16384];
	CSshSigDlg *pThis = (CSshSigDlg *)pParam;

	while ( pThis->m_ThreadStat != 0 && (n = (int)fread(tmp, 1, 16384, pThis->m_HashFp)) > 0 ) {
		if ( EVP_DigestUpdate(pThis->m_HashCtx, tmp, n) <= 0 )
			break;
		pThis->m_HashFilePos += n;
	}

	switch(pThis->m_ThreadStat) {
	case 0:
		fclose(pThis->m_HashFp);
		pThis->m_HashFp = NULL;
		EVP_MD_CTX_free(pThis->m_HashCtx);
		pThis->m_HashCtx = NULL;
		break;
	case 1:		// Sign
		pThis->PostMessage(WM_COMMAND, IDM_SSHSIG_SIGNEXT);
		break;
	case 2:
		pThis->PostMessage(WM_COMMAND, IDM_SSHSIG_VERIFYNEXT);
		break;
	}

	pThis->m_ThreadStat = 0;
	pThis->m_ThreadEvent.SetEvent();
	return 0;
}

/*
  draft-josefsson-sshsig-format-00

  3.  Armored format

  -----BEGIN SSH SIGNATURE-----
  Tq0Fb56xhtuE1/lK9H9RZJfON4o6hE9R4ZGFX98gy0+fFJ/1d2/RxnZky0Y7GojwrZkrHT
  ...
  FgCqVWAQ==
  -----END SSH SIGNATURE-----
  
  4.  Blob format

  byte[6] "SSHSIG"
    uint32 0x01
    string publickey
    string namespace
    string reserved
    string hash_algorithm
    string signature

  5.  Signed Data, of which the signature goes into the blob above

   byte[6] "SSHSIG"
    string namespace
    string reserved
    string hash_algorithm
    string H(message)
*/

void CSshSigDlg::OnOK()
{
	if ( ThreadCheck() )
		return;

	UpdateData(TRUE);

	m_DataFileCombo.AddHis(m_DataFile);
	m_KeyFileCombo.AddHis(m_KeyFile);
	m_NameSpaceCombo.AddHis(m_NameSpace);

	try {
		if ( (m_pKey = LoadKey()) == NULL )
			return;

		if ( (m_HashFp = FileOpen(m_DataFile)) == NULL )
			throw _T("Sign file open error");

		if ( !m_pKey->InitPass(_T("")) )
			throw _T("Sign init key error");

		if ( (m_HashCtx = EVP_MD_CTX_new()) == NULL )
			throw _T("Sign md ctx new error");

		if ( EVP_DigestInit(m_HashCtx, EVP_sha512()) <= 0 )
			throw _T("Sign digest init error");

		m_ThreadStat = 1;
		m_ThreadEvent.ResetEvent();

		if ( AfxBeginThread(FileHashThread, this, THREAD_PRIORITY_NORMAL) == NULL )
			throw _T("Sign begin thread error");

		m_HashTimer = SetTimer(1025, 200, NULL);

		return;	// wait OnSignNext

	} catch(LPCTSTR msg) {
		MessageBox(msg, NULL, MB_ICONERROR);
	}

	if ( m_HashCtx != NULL ) {
		EVP_MD_CTX_free(m_HashCtx);
		m_HashCtx = NULL;
	}

	if ( m_HashFp != NULL ) {
		fclose(m_HashFp);
		m_HashFp = NULL;
	}
}
void CSshSigDlg::OnSignNext()
{
	int hlen;
	CBuffer tmp, body, sig;
	CString line;
	u_char hash[EVP_MAX_MD_SIZE];

	try {
		if ( m_HashFilePos < m_HashFileSize )
			throw _T("Sign file read or digest update error");

		if ( EVP_DigestFinal(m_HashCtx, hash, (unsigned int *)&hlen) <= 0 )
			throw _T("Sign digest final error");

		if ( m_NameSpace.IsEmpty()  )
			m_NameSpace = _T("RLogin");

		tmp.Clear();
		tmp.Apend((LPBYTE)"SSHSIG", 6);
		tmp.PutStr(TstrToUtf8(m_NameSpace));
		tmp.PutStr("");
		tmp.PutStr("sha512");
		tmp.PutBuf(hash, hlen);

		if ( (m_pKey->m_Type & IDKEY_TYPE_MASK) == IDKEY_RSA1 || (m_pKey->m_Type & IDKEY_TYPE_MASK) == IDKEY_RSA2 )
			m_pKey->m_Nid = NID_sha512;

		if ( !m_pKey->Sign(&sig, tmp.GetPtr(), tmp.GetSize()) )
			throw _T("Sign key sign error");

		tmp.Clear();
		m_pKey->SetBlob(&tmp, FALSE);

		body.Apend((LPBYTE)"SSHSIG", 6);
		body.Put32Bit(0x01);
		body.PutBuf(tmp.GetPtr(), tmp.GetSize());
		body.PutStr(TstrToUtf8(m_NameSpace));
		body.PutStr("");
		body.PutStr("sha512");
		body.PutBuf(sig.GetPtr(), sig.GetSize());

		tmp.Base64Encode(body.GetPtr(), body.GetSize());

		m_Sign = _T("-----BEGIN SSH SIGNATURE-----\r\n");

		line.Empty();
		for ( LPCTSTR p = (LPCTSTR)tmp ; *p != _T('\0') ; ) {
			line += *(p++);
			if ( line.GetLength() > 70 || *p == _T('\0') ) {
				line += _T("\r\n");
				m_Sign += (LPCTSTR)line;
				line.Empty();
			}
		}

		m_Sign += _T("-----END SSH SIGNATURE-----\r\n");

		UpdateData(FALSE);

	} catch(LPCTSTR msg) {
		MessageBox(msg, NULL, MB_ICONERROR);
	}

	if ( m_HashFp != NULL ) {
		fclose(m_HashFp);
		m_HashFp = NULL;
	}

	if ( m_HashCtx != NULL ) {
		EVP_MD_CTX_free(m_HashCtx);
		m_HashCtx = NULL;
	}
}

void CSshSigDlg::OnVerify()
{
	if ( ThreadCheck() )
		return;

	UpdateData(TRUE);

	m_DataFileCombo.AddHis(m_DataFile);
	m_KeyFileCombo.AddHis(m_KeyFile);
	m_NameSpaceCombo.AddHis(m_NameSpace);

	try {
		if ( (m_HashFp = FileOpen(m_DataFile)) == NULL )
			throw _T("Verify file open error");

		m_pKey = NULL;
		if ( m_StrictVerify && (m_pKey = LoadKey()) == NULL )
			return;

		int st = 0;
		CString line, text;
		CBuffer body, tmp, name;
		const EVP_MD *md = NULL;

		tmp = (LPCTSTR)m_Sign;
		text.Empty();
		while ( tmp.ReadString(line) ) {
			line.Trim(_T("\t\r\n"));

			if ( line.CompareNoCase(_T("-----BEGIN SSH SIGNATURE-----")) == 0 ) {
				if ( st == 0 ) {
					st = 1;
					continue;
				} else
					break;
			} else if ( line.CompareNoCase(_T("-----END SSH SIGNATURE-----")) == 0 )
				break;
			
			if ( st == 0 )
				continue;

			text += line;
		}

		body.Base64Decode(text);

		if ( body.GetSize() < 6 || memcmp(body.GetPtr(), "SSHSIG", 6) != 0 )
			throw _T("Verify sshsig magic mismatch");
		body.Consume(6);

		if ( body.Get32Bit() != 0x01 )	// SSHSIG_VERSION
			throw _T("Verify sshsig version mismatch");

		// string publickey
		tmp.Clear();
		body.GetBuf(&tmp);

		if ( !m_VerifyKey.GetBlob(&tmp) )
			throw _T("Verify public key load error");

		if ( m_pKey != NULL && m_pKey->Compere(&m_VerifyKey) != 0 )
			throw _T("Verify public key no macth");

		// string namespace
		name.Clear();
		body.GetBuf(&name);

		if ( m_StrictVerify && m_NameSpace.Compare(Utf8ToTstr(name)) != 0 )
			throw _T("Verify namespace no macth");

		// string reserved
		tmp.Clear();
		body.GetBuf(&tmp);

		// string hash_algorithm
		tmp.Clear();
		body.GetBuf(&tmp);

		if ( strcmp((LPCSTR)tmp, "sha256") == 0 )
			md = EVP_sha256();
		else if ( strcmp((LPCSTR)tmp, "sha512") == 0 )
			md = EVP_sha512();
		else
			throw _T("Verify hash algorithm not support");

		if ( (m_VerifyKey.m_Type & IDKEY_TYPE_MASK) == IDKEY_RSA1 || (m_VerifyKey.m_Type & IDKEY_TYPE_MASK) == IDKEY_RSA2 )
			m_VerifyKey.m_Nid = (md == EVP_sha256() ? NID_sha256 : NID_sha512);

		// string signature
		m_VerifySign.Clear();
		body.GetBuf(&m_VerifySign);

		m_VerifyData.Clear();
		m_VerifyData.Apend((LPBYTE)"SSHSIG", 6);
		m_VerifyData.PutStr((LPCSTR)name);
		m_VerifyData.PutStr("");
		m_VerifyData.PutStr((LPCSTR)tmp);
		// m_VerifyData.PutBuf(hash, hlen);

		if ( (m_HashCtx = EVP_MD_CTX_new()) == NULL )
			throw _T("Verify md ctx new error");

		if ( EVP_DigestInit(m_HashCtx, md) <= 0 )
			throw _T("Verify digest init error");

		m_ThreadStat = 2;
		m_ThreadEvent.ResetEvent();

		if ( AfxBeginThread(FileHashThread, this, THREAD_PRIORITY_NORMAL) == NULL )
			throw _T("Verify begin thread error");

		m_HashTimer = SetTimer(1025, 200, NULL);

		return;	// wait OnVerifyNext

	} catch(LPCTSTR msg) {
		MessageBox(msg, NULL, MB_ICONERROR);
	}

	if ( m_HashFp != NULL ) {
		fclose(m_HashFp);
		m_HashFp = NULL;
	}

	if ( m_HashCtx != NULL ) {
		EVP_MD_CTX_free(m_HashCtx);
		m_HashCtx = NULL;
	}
}
void CSshSigDlg::OnVerifyNext()
{
	int hlen;
	u_char hash[EVP_MAX_MD_SIZE];

	try {
		if ( m_HashFilePos < m_HashFileSize )
			throw _T("Verify file read or digest update error");

		if ( EVP_DigestFinal(m_HashCtx, hash, (unsigned int *)&hlen) <= 0 )
			throw _T("Verify digest final error");

		m_VerifyData.PutBuf(hash, hlen);

		if ( !m_VerifyKey.Verify(&m_VerifySign, m_VerifyData.GetPtr(), m_VerifyData.GetSize()) )
			throw _T("Verify verification failed");

		MessageBox(_T("SSH key verification was successful"), NULL, MB_ICONINFORMATION);

	} catch(LPCTSTR msg) {
		MessageBox(msg, NULL, MB_ICONERROR);
	}

	if ( m_HashFp != NULL ) {
		fclose(m_HashFp);
		m_HashFp = NULL;
	}

	if ( m_HashCtx != NULL ) {
		EVP_MD_CTX_free(m_HashCtx);
		m_HashCtx = NULL;
	}
}

void CSshSigDlg::OnDatafileSel()
{
	UpdateData(TRUE);
	CFileDialog dlg(TRUE, _T(""), m_DataFile, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_DataFileCombo.UpdateHis();
	m_DataFile = dlg.GetPathName();
	UpdateData(FALSE);
}
void CSshSigDlg::OnKeylistSel()
{
	int uid = (-1);
	CIdkeySelDLg dlg(this);
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	UpdateData(TRUE);
	uid = (int)m_KeyListCombo.GetItemData(m_KeyList);

	pMain->AgeantInit();

	dlg.m_pParamTab = NULL;
	dlg.m_pIdKeyTab = &(pMain->m_IdKeyTab);

	if ( dlg.DoModal() != IDOK )
		return;

	m_IdKeyTab = pMain->m_IdKeyTab;
	KeyListInit(uid);
	UpdateData(FALSE);
}
void CSshSigDlg::OnKeyfileSel()
{
	UpdateData(TRUE);
	CFileDialog dlg(TRUE, _T(""), m_KeyFile, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_KeyFileCombo.UpdateHis();
	m_KeyFile = dlg.GetPathName();
	UpdateData(FALSE);
}

void CSshSigDlg::OnSignInport()
{
	CBuffer buf;
	CFileDialog dlg(TRUE, _T("sig"), _T(""), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	FormatErrorReset();

	if ( !buf.LoadFile(dlg.GetPathName()) ) {
		CStringLoad msg;
		::FormatErrorMessage(msg, _T("SSH sign file load error\n\n%s"), dlg.GetPathName());
		MessageBox(msg, NULL, MB_ICONERROR);
	}

	buf.StrConvert(_T("UTF-8"));

	m_Sign = (LPCTSTR)buf;
	UpdateData(FALSE);
}
void CSshSigDlg::OnSignExport()
{
	CBuffer buf;
	CFileDialog dlg(FALSE, _T("sig"), _T(""), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	UpdateData(TRUE);

	buf = (LPCTSTR)m_Sign;
	buf.WstrConvert(_T("UTF-8"), 2);	// DEFAULT, LF

	FormatErrorReset();

	if ( !buf.SaveFile(dlg.GetPathName()) ) {
		CStringLoad msg;
		::FormatErrorMessage(msg, _T("SSH sign file save error\n\n%s"), dlg.GetPathName());
		MessageBox(msg, NULL, MB_ICONERROR);
	}
}

void CSshSigDlg::OnSignCopy()
{
	UpdateData(TRUE);

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(m_Sign);
}
void CSshSigDlg::OnSignPast()
{
	if ( !((CMainFrame *)::AfxGetMainWnd())->CopyClipboardData(m_Sign) )
		return;

	UpdateData(FALSE);
}

void CSshSigDlg::OnKeyMode(UINT nID)
{
	CWnd *pWnd;

	UpdateData(TRUE);

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYLIST)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 0 ? TRUE : FALSE);

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYLIST_SEL)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 0 ? TRUE : FALSE);

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYFILE)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 1 ? TRUE : FALSE);

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYFILE_SEL)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 1 ? TRUE : FALSE);
}

void CSshSigDlg::OnTimer(UINT_PTR nIDEvent)
{
	m_HashProg.SetPos((int)(m_HashFilePos / m_HashFileDivs));

	if ( m_ThreadStat == 0 || m_HashFilePos >= m_HashFileSize ) {
		KillTimer(m_HashTimer);
		m_HashTimer = 0;
	}

	CDialogExt::OnTimer(nIDEvent);
}
