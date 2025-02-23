// SshSigDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "SshSigDlg.h"
#include "IdKeyFileDlg.h"

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

	DDX_Control(pDX, IDC_SSHSIG_DATAFILE, m_DataFileCombo);
	DDX_Control(pDX, IDC_SSHSIG_KEYLIST, m_KeyListCombo);
	DDX_Control(pDX, IDC_SSHSIG_KEYFILE, m_KeyFileCombo);
	DDX_Control(pDX, IDC_SSHSIG_NAMESPACE, m_NameSpaceCombo);
}

CIdKey *CSshSigDlg::LoadKey()
{
	CIdKey *pKey = NULL;
	CIdKeyFileDlg dlg;
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
	} else {
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

		if ( !m_TempKey.LoadPrivateKey(dlg.m_IdkeyFile, dlg.m_PassName) ) {
			::AfxMessageBox(CStringLoad(IDE_IDKEYLOADERROR), MB_ICONERROR);
			return NULL;
		}

		pKey = &m_TempKey;
	}

	return pKey;
}

BEGIN_MESSAGE_MAP(CSshSigDlg, CDialogExt)
	ON_BN_CLICKED(IDC_SSHSIG_VERIFY, &CSshSigDlg::OnVerify)
	ON_BN_CLICKED(IDC_SSHSIG_DATAFILE_SEL, &CSshSigDlg::OnDatafileSel)
	ON_BN_CLICKED(IDC_SSHSIG_KEYFILE_SEL, &CSshSigDlg::OnKeyfileSel)
	ON_BN_CLICKED(IDC_SSHSIG_INPORT, &CSshSigDlg::OnSignInport)
	ON_BN_CLICKED(IDC_SSHSIG_EXPORT, &CSshSigDlg::OnSignExport)
	ON_BN_CLICKED(IDC_SSHSIG_SIGN_COPY, &CSshSigDlg::OnSignCopy)
	ON_BN_CLICKED(IDC_SSHSIG_SIGN_PAST, &CSshSigDlg::OnSignPast)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_SSHSIG_KEYSEL1, IDC_SSHSIG_KEYSEL2, OnKeyMode)
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

	{ IDC_SSHSIG_NAMESPACE,			ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },
	{ IDC_SSHSIG_VERIFY_WITHKEY,	ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_TOP },

	{ IDC_SSHSIG_SIGN,				ITM_LEFT_LEFT | ITM_RIGHT_RIGHT | ITM_TOP_TOP | ITM_BTM_BTM },
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
	CString str;
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

	for ( int n = 0 ; n < m_IdKeyTab.GetSize() ; n++ ) {
		str.Format(_T("%s %d (%s)"), m_IdKeyTab[n].GetName(), m_IdKeyTab[n].GetSize(), m_IdKeyTab[n].m_Name);
		switch(m_IdKeyTab[n].m_AgeantType) {
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

	SetSaveProfile(_T("SshSigDlg"));
	AddHelpButton(_T("#SSHSIG"));

	return TRUE;
}

void CSshSigDlg::OnCancel()
{
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	UpdateData(TRUE);

	pApp->WriteProfileString(_T("SshSigDlg"), _T("DataFile"), m_DataFile);
	pApp->WriteProfileString(_T("SshSigDlg"), _T("KeyFile"), m_KeyFile);
	pApp->WriteProfileString(_T("SshSigDlg"), _T("NameSpace"), m_NameSpace);
	pApp->WriteProfileInt(_T("SshSigDlg"), _T("KeyUid"), (int)m_KeyListCombo.GetItemData(m_KeyList));
	pApp->WriteProfileInt(_T("SshSigDlg"), _T("KeyMode"), m_KeyMode);
	pApp->GetProfileInt(_T("SshSigDlg"), _T("StrictVerify"), m_StrictVerify);

	m_DataFileCombo.AddHis(m_DataFile);
	m_KeyFileCombo.AddHis(m_KeyFile);
	m_NameSpaceCombo.AddHis(m_NameSpace);

	CDialogExt::OnCancel();
}
void CSshSigDlg::OnOK()
{
	CIdKey *pKey;
	CBuffer sig;

	UpdateData(TRUE);

	if ( (pKey = LoadKey()) == NULL )
		return;

	CWaitCursor wait;

	if ( !pKey->FileSign(m_DataFile, &sig, TstrToMbs(m_NameSpace), _T("")) )
		return;

	m_Sign = (LPCTSTR)sig;
	UpdateData(FALSE);
}
void CSshSigDlg::OnVerify()
{
	CIdKey wkey;
	CIdKey *pKey = &wkey;
	CBuffer sig;

	UpdateData(TRUE);

	if ( m_StrictVerify && (pKey = LoadKey()) == NULL )
		return;

	sig = (LPCTSTR)m_Sign;
	
	CWaitCursor wait;

	if ( pKey->FileVerify(m_DataFile, &sig, (m_StrictVerify ? TstrToMbs(m_NameSpace) : NULL)) )
		::AfxMessageBox(_T("SSH key verification was successful"), MB_ICONINFORMATION);
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

	if ( !buf.LoadFile(dlg.GetPathName()) )
		::ThreadMessageBox(_T("SSH sign file load error\n'%s'"), dlg.GetPathName());

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

	if ( !buf.SaveFile(dlg.GetPathName()) )
		::ThreadMessageBox(_T("SSH sign file save error\n'%s'"), dlg.GetPathName());
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

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYFILE)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 1 ? TRUE : FALSE);

	if ( (pWnd = GetDlgItem(IDC_SSHSIG_KEYFILE_SEL)) != NULL )
		pWnd->EnableWindow(m_KeyMode == 1 ? TRUE : FALSE);
}