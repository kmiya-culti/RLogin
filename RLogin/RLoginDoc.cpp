// RLoginDoc.cpp : CRLoginDoc クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "ExtSocket.h"
#include "Login.h"
#include "Telnet.h"
#include "Ssh.h"
#include "ComSock.h"
#include "PipeSock.h"
#include "Data.h"
#include "PassDlg.h"
#include "OptDlg.h"
#include "Wininet.h"
#include "EditDlg.h"
#include "StatusDlg.h"
#include "TraceDlg.h"
#include "Script.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc

IMPLEMENT_DYNCREATE(CRLoginDoc, CDocument)

BEGIN_MESSAGE_MAP(CRLoginDoc, CDocument)
	//{{AFX_MSG_MAP(CRLoginDoc)

	ON_COMMAND(ID_FILE_CLOSE, &CRLoginDoc::OnFileClose)
	ON_COMMAND(IDC_CANCELBTN, OnCancelBtn)

	ON_COMMAND(ID_LOG_OPEN, OnLogOpen)
	ON_UPDATE_COMMAND_UI(ID_LOG_OPEN, OnUpdateLogOpen)
	ON_COMMAND(ID_CHARSCRIPT_END, &CRLoginDoc::OnChatStop)
	ON_UPDATE_COMMAND_UI(ID_CHARSCRIPT_END, &CRLoginDoc::OnUpdateChatStop)
	ON_COMMAND(IDM_SFTP, OnSftp)
	ON_UPDATE_COMMAND_UI(IDM_SFTP, OnUpdateSftp)
	ON_COMMAND(ID_SETOPTION, OnSetOption)
	ON_COMMAND_RANGE(IDM_KANJI_EUC, IDM_KANJI_UTF8, OnKanjiCodeSet)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_KANJI_EUC, IDM_KANJI_UTF8, OnUpdateKanjiCodeSet)
	ON_COMMAND_RANGE(IDM_XMODEM_UPLOAD, IDM_SIMPLE_DOWNLOAD, OnXYZModem)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_XMODEM_UPLOAD, IDM_SIMPLE_DOWNLOAD, OnUpdateXYZModem)
	ON_COMMAND(ID_SEND_BREAK, &CRLoginDoc::OnSendBreak)
	ON_UPDATE_COMMAND_UI(ID_SEND_BREAK, &CRLoginDoc::OnUpdateSendBreak)
	ON_COMMAND(IDM_TEKDISP, &CRLoginDoc::OnTekdisp)
	ON_UPDATE_COMMAND_UI(IDM_TEKDISP, &CRLoginDoc::OnUpdateTekdisp)
	ON_COMMAND(IDM_SOCKETSTATUS, &CRLoginDoc::OnSocketstatus)
	ON_UPDATE_COMMAND_UI(IDM_SOCKETSTATUS, &CRLoginDoc::OnUpdateSocketstatus)
	ON_COMMAND(IDM_SCRIPT, &CRLoginDoc::OnScript)
	ON_UPDATE_COMMAND_UI(IDM_SCRIPT, &CRLoginDoc::OnUpdateScript)
	ON_COMMAND(IDM_IMAGEDISP, &CRLoginDoc::OnImagedisp)
	ON_UPDATE_COMMAND_UI(IDM_IMAGEDISP, &CRLoginDoc::OnUpdateImagedisp)
	ON_COMMAND(IDM_REOPENSOCK, &CRLoginDoc::OnSockReOpen)

	ON_COMMAND(IDM_TRACEDISP, &CRLoginDoc::OnTracedisp)
	ON_COMMAND(IDC_LOADDEFAULT, OnLoadDefault)
	ON_COMMAND(IDC_SAVEDEFAULT, OnSaveDefault)
	ON_COMMAND_RANGE(IDM_RESET_TAB, IDM_RESET_ALL, &CRLoginDoc::OnScreenReset)
	ON_COMMAND_RANGE(IDM_SCRIPT_MENU1, IDM_SCRIPT_MENU10, OnScriptMenu)

	ON_UPDATE_COMMAND_UI(IDM_RESET_SIZE, &CRLoginDoc::OnUpdateResetSize)
	ON_COMMAND(IDM_TITLEEDIT, &CRLoginDoc::OnTitleedit)
	ON_COMMAND(IDM_COMMONITER, &CRLoginDoc::OnCommoniter)
	ON_UPDATE_COMMAND_UI(IDM_COMMONITER, &CRLoginDoc::OnUpdateCommoniter)
	ON_UPDATE_COMMAND_UI(IDM_TRACEDISP, &CRLoginDoc::OnUpdateTracedisp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc クラスの構築/消滅

CRLoginDoc::CRLoginDoc()
{
	m_pSock = NULL;
	m_TextRam.m_pDocument = this;
	m_pLogFile = NULL;
	m_pBPlus = NULL;
	m_pZModem = NULL;
	m_pKermit = NULL;
	m_bDelayPast = FALSE;
	m_DelayFlag = DELAY_NON;
	m_pMainWnd = (CMainFrame *)AfxGetMainWnd();
	m_SockStatus.Empty();
	m_pStrScript = NULL;
	m_pScript = NULL;
	m_InPane = FALSE;
	m_AfterId = (-1);
	m_LogSendRecv = LOGDEBUG_NONE;
	m_ScriptFile.Empty();
	m_bReqDlg = FALSE;
	m_TitleName.Empty();
	m_bCastLock = FALSE;
	m_ConnectTime = 0;
	m_CloseTime = 0;
	m_pStatusWnd = NULL;
	m_pMediaCopyWnd = NULL;
	m_CmdsPath.Empty();
	m_bExitPause = FALSE;
}

CRLoginDoc::~CRLoginDoc()
{
	LogClose();

	if ( m_pBPlus != NULL )
		delete m_pBPlus;
	if ( m_pZModem != NULL )
		delete m_pZModem;
	if ( m_pKermit != NULL )
		delete m_pKermit;

	if ( m_pScript != NULL )
		delete m_pScript;

	if ( m_DelayFlag != DELAY_NON )
		m_pMainWnd->DelTimerEvent(this);
}
BOOL CRLoginDoc::ScriptInit()
{
	ASSERT(m_pScript == NULL);

	//if ( m_pScript != NULL ) {
	//	m_pScript->Stop();
	//	delete m_pScript;
	//	m_pScript = NULL;
	//}

	m_pScript = new CScript;
	m_pScript->SetDocument(this);

	if ( !m_ServerEntry.m_ScriptFile.IsEmpty() ) {
		CString file(m_ServerEntry.m_ScriptFile);
		EntryText(file);
		m_pScript->ExecFile(file);
	}

	if ( !m_ServerEntry.m_ScriptStr.IsEmpty() )
		m_pScript->ExecStr(m_ServerEntry.m_ScriptStr);

	if ( !m_ScriptFile.IsEmpty() )
		m_pScript->ExecFile(m_ScriptFile);

	if ( m_TextRam.IsOptEnable(TO_RLSCRDEBUG) )
		m_pScript->OpenDebug(m_ServerEntry.m_EntryName);

	// スクリプトの実行許可
	m_pScript->m_bInit = TRUE;

	// スクリプトがあればオープンしない
	if ( m_pScript->m_CodePos != (-1) )
		return FALSE;

	// スクリプト終了時にソケットをオープンしない
	m_pScript->m_bOpenSock = FALSE;

	// ソケットをオープン許可
	return TRUE;
}
BOOL CRLoginDoc::InitDocument()
{
	m_TextRam.SetKanjiMode(m_ServerEntry.m_KanjiCode);

	SetTitle(m_ServerEntry.m_EntryName);
	SetCmdInfo(((CRLoginApp *)AfxGetApp())->m_pCmdInfo);

	if ( ScriptInit() && !SocketOpen() )
		return FALSE;

	CRLoginView *pView;
	if ( (pView = (CRLoginView *)GetAciveView()) != NULL && pView->ImmOpenCtrl(2) == 1 ) {
		m_TextRam.EnableOption(TO_IMECTRL);
		pView->ImmOpenCtrl(1);
	} else {
		pView->ImmOpenCtrl(1);
		m_TextRam.DisableOption(TO_IMECTRL);
		pView->ImmOpenCtrl(0);
	}

	return TRUE;
}
BOOL CRLoginDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	if ( !m_pMainWnd->OpenServerEntry(m_ServerEntry) )
		return FALSE;

	LoadOption(m_ServerEntry, m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab);

	if ( m_ServerEntry.m_bOptFixed ) {
		CString ExtEnvStr = m_ParamTab.m_ExtEnvStr;
		BOOL UsePass = m_TextRam.IsOptEnable(TO_RLUSEPASS);
		BOOL ProxyPass = m_TextRam.IsOptEnable(TO_PROXPASS);

		InitOptFixCheck(m_ServerEntry.m_Uid);

		if ( !SetOptFixEntry(m_ServerEntry.m_OptFixEntry) )
			LoadDefOption(m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab);

		m_ParamTab.m_ExtEnvStr = ExtEnvStr;
		m_TextRam.SetOption(TO_RLUSEPASS, UsePass);
		m_TextRam.SetOption(TO_PROXPASS, ProxyPass);
	}

	m_TextRam.m_bOpen = TRUE;
	m_TextRam.m_pServerEntry = &m_ServerEntry;

	if ( !InitDocument() )
		return FALSE;

	return TRUE;
}

BOOL CRLoginDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	m_ServerEntry.m_DocType = DOCTYPE_NONE;

	if ( !CDocument::OnOpenDocument(lpszPathName) )
		return FALSE;

	m_TextRam.m_bOpen = TRUE;

	if ( m_ServerEntry.m_DocType == DOCTYPE_ENTRYFILE ) {

		SetPathName(lpszPathName, TRUE);

		if ( !InitDocument() )
			return FALSE;
	}
	
	return (m_ServerEntry.m_DocType == DOCTYPE_NONE ? FALSE : TRUE);
}
BOOL CRLoginDoc::DoFileSave()
{
	if ( m_ServerEntry.m_DocType == DOCTYPE_MULTIFILE ) {
		m_pMainWnd->SendMessage(WM_COMMAND, ID_FILE_ALL_SAVE);
		return TRUE;

	} else if ( m_ServerEntry.m_DocType == DOCTYPE_REGISTORY ) {
		if ( m_ServerEntry.m_bOptFixed ) {
			if ( ::AfxMessageBox(IDS_OPTFIXEDSAVEMSG, MB_ICONQUESTION | MB_YESNO) != IDYES )
				return FALSE;
			m_ServerEntry.m_bOptFixed = FALSE;
		}
		SetEntryProBuffer();
		m_pMainWnd->m_ServerEntryTab.AddEntry(m_ServerEntry);
		return TRUE;

	} else if ( m_ServerEntry.m_DocType == DOCTYPE_ENTRYFILE || m_ServerEntry.m_DocType == DOCTYPE_SESSION ) {
		return CDocument::DoFileSave();
	}

	return FALSE;
}
void CRLoginDoc::OnFileClose()
{
	if ( ((CMainFrame *)::AfxGetMainWnd())->IsTimerIdleBusy() )
		return;

	if ( m_pSock != NULL && m_pSock->m_bConnect ) {
		if ( AfxMessageBox(CStringLoad(IDS_FILECLOSEQES), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
	}

	if ( IsCanExit() )
		m_pMainWnd->PostMessage(WM_COMMAND, ID_APP_EXIT, 0 );

	if ( m_InPane ) {
		POSITION pos;
		CRLoginView *pView;
		CChildFrame *pChild;

		pos = GetFirstViewPosition();
		while ( pos != NULL ) {
			if ( (pView = (CRLoginView *)GetNextView(pos)) != NULL && (pChild = pView->GetFrameWnd()) != NULL )
				pChild->m_bDeletePane = TRUE;
		}
	}

	CDocument::OnFileClose();
}
void CRLoginDoc::OnIdle()
{
	if ( m_pLogFile != NULL && m_pSock != NULL && m_pSock->GetRecvProcSize() == 0 )
		m_pLogFile->Flush();

	m_TextRam.ChkGrapWnd(60);
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc Static

void CRLoginDoc::LoadOption(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab)
{
	// ServerEntryのm_ProBufferから読み込み
	CBuffer tmp(ServerEntry.m_ProBuffer.GetPtr(), ServerEntry.m_ProBuffer.GetSize());
	TextRam.Serialize(FALSE, tmp);
	KeyTab.Serialize(FALSE, tmp);
	KeyMac.Serialize(FALSE, tmp);
	ParamTab.Serialize(FALSE, tmp);
}
void CRLoginDoc::SaveOption(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab)
{
	// ServerEntryのm_ProBufferへ書き込み
	ServerEntry.m_ProBuffer.Clear();
	TextRam.Serialize(TRUE, ServerEntry.m_ProBuffer);
	KeyTab.Serialize(TRUE, ServerEntry.m_ProBuffer);
	KeyMac.Serialize(TRUE, ServerEntry.m_ProBuffer);
	ParamTab.Serialize(TRUE, ServerEntry.m_ProBuffer);
}
void CRLoginDoc::LoadIndex(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab, CStringIndex &index)
{
	// StringIndexから読み込み
	ServerEntry.SetIndex(FALSE, index[_T("Entry")]);
	ParamTab.SetIndex(FALSE, index[_T("Protocol")]);
	TextRam.SetIndex(FALSE, index[_T("Screen")]);
	TextRam.m_FontTab.SetIndex(FALSE, index[_T("Fontset")]);
	TextRam.m_TextBitMap.SetIndex(FALSE, index[_T("TextBitMap")]);
	KeyTab.SetIndex(FALSE, index[_T("Keycode")]);
#ifndef	USE_KEYMACGLOBAL
	KeyMac.SetIndex(FALSE, index[_T("Keymacro")]);
#endif
}
void CRLoginDoc::SaveIndex(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab, CStringIndex &index)
{
	// StringIndexへ書き込み
	ServerEntry.SetIndex(TRUE, index[_T("Entry")]);
	ParamTab.SetIndex(TRUE, index[_T("Protocol")]);
	TextRam.SetIndex(TRUE, index[_T("Screen")]);
	TextRam.m_FontTab.SetIndex(TRUE, index[_T("Fontset")]);
	TextRam.m_TextBitMap.SetIndex(TRUE, index[_T("TextBitMap")]);
	KeyTab.SetIndex(TRUE, index[_T("Keycode")]);
#ifndef	USE_KEYMACGLOBAL
	KeyMac.SetIndex(TRUE, index[_T("Keymacro")]);
#endif
}
void CRLoginDoc::DiffIndex(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab, CServerEntry &OrigEntry, CStringIndex &index)
{
	CTextRam *pOrigTextRam   = new CTextRam;
	CKeyNodeTab *pOrigKeyTab = new CKeyNodeTab;
	CKeyMacTab *pOrigKeyMac  = new CKeyMacTab;
	CParamTab *pOrigParamTab = new CParamTab;

	LoadOption(OrigEntry, *pOrigTextRam, *pOrigKeyTab, *pOrigKeyMac, *pOrigParamTab);

	index.RemoveAll();
	index.SetNoCase(TRUE);
	index.SetNoSort(TRUE);

	ServerEntry.DiffIndex(OrigEntry, index[_T("Entry")]);
	ParamTab.DiffIndex(*pOrigParamTab, index[_T("Protocol")]);
	TextRam.DiffIndex(*pOrigTextRam, index[_T("Screen")]);
	TextRam.m_FontTab.DiffIndex(pOrigTextRam->m_FontTab, index[_T("Fontset")]);
	TextRam.m_TextBitMap.DiffIndex(pOrigTextRam->m_TextBitMap, index[_T("TextBitMap")]);
	KeyTab.DiffIndex(*pOrigKeyTab, index[_T("Keycode")]);

	delete pOrigTextRam;
	delete pOrigKeyTab;
	delete pOrigKeyMac;
	delete pOrigParamTab;
}
void CRLoginDoc::LoadInitOption(CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab)
{
	TextRam.Init();
	TextRam.m_FontTab.Init();
	KeyTab.Init();
	KeyMac.Init();
	ParamTab.Init();
}
void CRLoginDoc::LoadDefOption(CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab)
{
	TextRam.Serialize(FALSE);
	KeyTab.Serialize(FALSE);
#ifndef	USE_KEYMACGLOBAL
	KeyMac.Serialize(FALSE);
#endif
	ParamTab.Serialize(FALSE);
}
void CRLoginDoc::SaveDefOption(CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab)
{
	TextRam.Serialize(TRUE);
	KeyTab.Serialize(TRUE);
#ifndef	USE_KEYMACGLOBAL
	KeyMac.Serialize(TRUE);
#endif
	ParamTab.Serialize(TRUE);

	((CMainFrame *)::AfxGetMainWnd())->m_DefKeyTab.Serialize(FALSE);
	((CMainFrame *)::AfxGetMainWnd())->m_DefKeyTab.CmdsInit();
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc シリアライゼーション

void CRLoginDoc::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {			// TODO: この位置に保存用のコードを追加してください。
#if	1
		CStringIndex index;
		index.SetNoCase(TRUE);
		index.SetNoSort(TRUE);
		SetIndex(TRUE, index);
		ar.Write("RLG300\r\n", 8);
		index.Serialize(ar, NULL);
#else
		ar.Write("RLG200\n", 7);
		m_ServerEntry.Serialize(ar);
		m_TextRam.Serialize(ar);
		m_KeyTab.Serialize(ar);
		m_KeyMac.Serialize(ar);
		m_ParamTab.Serialize(ar);
#endif
		m_ServerEntry.m_DocType = DOCTYPE_ENTRYFILE;

	} else {						// TODO: この位置に読み込み用のコードを追加してください。
		int n;
		CHAR tmp[4096];

		memset(tmp, 0, 16);
		for ( n = 0 ; n < 16 && ar.Read(&(tmp[n]), 1) == 1 ; n++ ) {
			if ( tmp[n] == '\n' )
				break;
		}
		if ( strncmp(tmp, "RLG3", 4) == 0 ) {
			CStringIndex index;
			index.SetNoCase(TRUE);
			index.SetNoSort(TRUE);
			index.Serialize(ar, NULL);
			SetIndex(FALSE, index);
			m_TextRam.SetKanjiMode(m_ServerEntry.m_KanjiCode);
			m_ServerEntry.m_DocType = DOCTYPE_ENTRYFILE;

		} else if ( strncmp(tmp, "RLG2", 4) == 0 ) {
			m_ServerEntry.Serialize(ar);
			m_TextRam.Serialize(ar);
			m_KeyTab.Serialize(ar);
			m_KeyMac.Serialize(ar);
			m_ParamTab.Serialize(ar);

			m_TextRam.SetKanjiMode(m_ServerEntry.m_KanjiCode);
			m_ServerEntry.m_DocType = DOCTYPE_ENTRYFILE;

		} else if ( strncmp(tmp, "RLM", 3) == 0 ) {
			m_pMainWnd->m_AllFilePath = ar.GetFile()->GetFilePath();
			m_pMainWnd->m_AllFileBuf.Clear();
			while ( (n = ar.Read(tmp, 4096)) > 0 )
				m_pMainWnd->m_AllFileBuf.Apend((LPBYTE)tmp, n);
			m_pMainWnd->PostMessage(WM_COMMAND, ID_FILE_ALL_LOAD, 0);
			m_ServerEntry.m_DocType = DOCTYPE_MULTIFILE;

		} else
			AfxThrowArchiveException(CArchiveException::badIndex, ar.GetFile()->GetFileTitle());
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc クラスの診断

#ifdef _DEBUG
void CRLoginDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRLoginDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc コマンド

void CRLoginDoc::SetCmdInfo(CCommandLineInfoEx *pCmdInfo)
{
	CString tmp;

	switch(m_ServerEntry.m_DocType) {
	case DOCTYPE_NONE:
		m_CmdLine.Empty();
		break;
	case DOCTYPE_REGISTORY:
		m_CmdLine.Format(_T("/entry \"%s\""), m_ServerEntry.m_EntryName);
		break;
	case DOCTYPE_ENTRYFILE:
	case DOCTYPE_MULTIFILE:
		tmp = GetPathName();
		if ( tmp.IsEmpty() ) tmp = _T("NONAME.rlg");
		m_CmdLine.Format(_T("\"%s\""), tmp);
		break;
	case DOCTYPE_SESSION:
		m_CmdLine.Empty();
		break;
	}

	if ( pCmdInfo == NULL || pCmdInfo->m_nShellCommand == CCommandLineInfo::FileDDE )
		return;

	if ( pCmdInfo->m_Proto != (-1) ) {
		switch(pCmdInfo->m_Proto) {
		case PROTO_DIRECT:  m_CmdLine += _T(" /direct"); break;
		case PROTO_LOGIN:	m_CmdLine += _T(" /login"); break;
		case PROTO_TELNET:	m_CmdLine += _T(" /telnet"); break;
		case PROTO_SSH:		m_CmdLine += _T(" /ssh"); break;
		case PROTO_COMPORT:	m_CmdLine += _T(" /com"); break;
		case PROTO_PIPE:	m_CmdLine += _T(" /pipe"); break;
		}
		m_ServerEntry.m_ProtoType = pCmdInfo->m_Proto;
	}

	if ( !pCmdInfo->m_Addr.IsEmpty() ) {
		tmp.Format(_T(" /ip %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_Addr));
		m_CmdLine += tmp;
		m_ServerEntry.m_HostName = pCmdInfo->m_Addr;
	}

	if ( !pCmdInfo->m_Port.IsEmpty() ) {
		tmp.Format(_T(" /port %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_Port));
		m_CmdLine += tmp;
		m_ServerEntry.m_PortName = pCmdInfo->m_Port;
	}

	if ( !pCmdInfo->m_User.IsEmpty() ) {
		tmp.Format(_T(" /user %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_User));
		m_CmdLine += tmp;
		m_ServerEntry.m_UserName = pCmdInfo->m_User;
	}

	if ( !pCmdInfo->m_Pass.IsEmpty() ) {
		tmp.Format(_T(" /pass %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_Pass));
		m_CmdLine += tmp;
		m_ServerEntry.m_PassName = pCmdInfo->m_Pass;
	}

	if ( !pCmdInfo->m_Term.IsEmpty() ) {
		tmp.Format(_T(" /term %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_Term));
		m_CmdLine += tmp;
		m_ServerEntry.m_TermName = pCmdInfo->m_Term;
	}

	if ( !pCmdInfo->m_Idkey.IsEmpty() ) {
		tmp.Format(_T(" /idkey %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_Idkey));
		m_CmdLine += tmp;
		m_ServerEntry.m_IdkeyName = pCmdInfo->m_Idkey;
	}

	if ( !pCmdInfo->m_Title.IsEmpty() ) {
		tmp.Format(_T(" /title %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_Title));
		m_CmdLine += tmp;
		m_TitleName = pCmdInfo->m_Title;
	}

	if ( !pCmdInfo->m_Script.IsEmpty() ) {
		tmp.Format(_T(" /script %s"), CCommandLineInfoEx::ShellEscape(pCmdInfo->m_Script));
		m_CmdLine += tmp;
		m_ScriptFile = pCmdInfo->m_Script;
	}

	if ( pCmdInfo->m_ReqDlg ) {
		m_CmdLine += _T(" /req");
		m_bReqDlg = pCmdInfo->m_ReqDlg;
	}

	//if ( pCmdInfo->m_InUse == INUSE_ACTWIN )
	//	m_CmdLine += _T(" /inuse");
	//else if ( pCmdInfo->m_InUse == INUSE_ALLWIN )
	//	m_CmdLine += _T(" /inusea");

	if ( pCmdInfo->m_InPane ) {
		//m_CmdLine += _T(" /inpne");
		m_InPane  = pCmdInfo->m_InPane;
	}

	if ( pCmdInfo->m_AfterId != (-1) ) {
		//tmp.Format(_T(" /after %d"), pCmdInfo->m_AfterId);
		//m_CmdLine += tmp;
		m_AfterId = pCmdInfo->m_AfterId;
	}

	if ( !pCmdInfo->m_Path.IsEmpty() )
		m_CmdsPath = pCmdInfo->m_Path;

	if ( m_ServerEntry.m_DocType == DOCTYPE_SESSION && (m_ServerEntry.m_HostName.IsEmpty() || m_ServerEntry.m_UserName.IsEmpty() || m_ServerEntry.m_PassName.IsEmpty() ) )
		m_bReqDlg = TRUE;
}
void CRLoginDoc::DeleteContents() 
{
	if ( m_pStatusWnd != NULL )
		m_pStatusWnd->DestroyWindow();

	if ( m_pMediaCopyWnd != NULL )
		m_pMediaCopyWnd->DestroyWindow();

	if ( m_TextRam.m_pTraceWnd != NULL )
		m_TextRam.m_pTraceWnd->SendMessage(WM_CLOSE);

	SocketClose();

	if ( m_pScript != NULL ) {
		m_pScript->Stop();
		delete m_pScript;
		m_pScript = NULL;
	}

	m_ServerEntry.Init();
	m_TextRam.m_bOpen = FALSE;
	m_InPane = FALSE;
	m_AfterId = (-1);
	m_bCastLock = FALSE;
	m_ConnectTime = 0;
	m_CloseTime = 0;
	m_CmdsPath.Empty();
	m_bExitPause = FALSE;

	CDocument::DeleteContents();
}
void CRLoginDoc::SetDocTitle()
{
	int n;
	CString title;

	if ( !m_TitleName.IsEmpty() ) {
		title = m_TitleName;

	} else if ( !m_TextRam.m_TitleName.IsEmpty() ) {
		title = m_TextRam.m_TitleName;
		EntryText(title);

	} else {
		switch(m_TextRam.m_TitleMode & 0007) {
		case WTTL_ENTRY:
			title = m_ServerEntry.m_EntryName;
			break;
		case WTTL_HOST:
			title = m_ServerEntry.m_HostName;
			break;
		case WTTL_PORT:
			title.Format(_T("%s:%d"), m_ServerEntry.m_HostName, CExtSocket::GetPortNum(m_ServerEntry.m_PortName));
			break;
		case WTTL_USER:
			title = m_ServerEntry.m_UserName;
			break;
		}
	}

	if ( m_pStrScript != NULL ) {
		if ( (n = m_pStrScript->Status()) == SCPSTAT_MAKE )
			title += _T("...Chat Makeing");
		else if ( n == SCPSTAT_EXEC )
			title += _T("...Chat Execute");
	}

	SetTitle(title);
}

void CRLoginDoc::SendBuffer(CBuffer &buf)
{
	POSITION pos;
	CRLoginView *pView;
	if ( (pos = GetFirstViewPosition()) != NULL && (pView = (CRLoginView *)GetNextView(pos)) != NULL )
		pView->SendBuffer(buf, TRUE);
}

void CRLoginDoc::SetMenu(CMenu *pMenu)
{
	int n, a;
	CMenuLoad DefMenu;
	CMenu *pSubMenu, *pClipMenu;
	CString str;
	UINT ThisId = 0;

	if ( !DefMenu.LoadMenu(IDR_RLOGINTYPE) )
		return;

#if 0
	// Reset Menu
	while ( pMenu->GetMenuItemCount() > 0 )
		pMenu->DeleteMenu(0, MF_BYPOSITION);

	for ( n = 0 ; n < DefMenu.GetMenuItemCount() ; n++ ) {
		DefMenu.GetMenuString(n, str, MF_BYPOSITION);
		if ( (pSubMenu = DefMenu.GetSubMenu(n)) != NULL ) {
			pMenu->AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)pSubMenu->GetSafeHmenu(), str);
			DefMenu.RemoveMenu(n, MF_BYPOSITION);
			n--;
		}
	}
#else
	CMenuLoad::CopyMenu(&DefMenu, pMenu);
#endif

	// Add ReOpen Menu
	if ( m_pSock != NULL && !m_pSock->m_bConnect )
		pMenu->InsertMenu(ID_FILE_CLOSE, MF_BYCOMMAND, IDM_REOPENSOCK, CStringLoad(IDS_REOPENSOCK));
	
	// Key History Menu
	if ( (pSubMenu = CMenuLoad::GetItemSubMenu(ID_MACRO_HIS1, pMenu)) != NULL ) {
		pSubMenu->DeleteMenu(ID_MACRO_HIS1, MF_BYCOMMAND);
#ifdef	USE_KEYMACGLOBAL
		((CRLoginApp *)::AfxGetApp())->m_KeyMacGlobal.SetHisMenu(pSubMenu);
#else
		m_KeyMac.SetHisMenu(pSubMenu);
#endif
	}

	// Script Menu
	if ( m_pScript != NULL )
		m_pScript->SetMenu(pMenu);

	// Clipboard Menu
	if ( (pClipMenu = CMenuLoad::GetItemSubMenu(IDM_CLIPBOARD_HIS1, pMenu)) != NULL ) {
		pClipMenu->DeleteMenu(IDM_CLIPBOARD_HIS1, MF_BYCOMMAND);
		((CMainFrame *)::AfxGetMainWnd())->SetClipBoardMenu(IDM_CLIPBOARD_HIS1, pClipMenu);
	}

	// Window Menu
	if ( (pSubMenu = CMenuLoad::GetItemSubMenu(IDM_FIRST_MDICHILD, pMenu)) != NULL ) {
		pSubMenu->DeleteMenu(IDM_FIRST_MDICHILD, MF_BYCOMMAND);

		CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
		CChildFrame *pChild;
		CRLoginDoc *pDoc;

		for ( n = 0 ; (pChild = (CChildFrame *)pMain->GetTabWnd(n)) != NULL ; n++ ) {
			if ( (pDoc = (CRLoginDoc *)pChild->GetActiveDocument()) != NULL ) {
				str.Format(_T("&%d %s"), n + 1, pDoc->GetTitle());
				pSubMenu->AppendMenu(MF_STRING, AFX_IDM_FIRST_MDICHILD + n, str);
				if ( pDoc == this )
					ThisId = AFX_IDM_FIRST_MDICHILD + n;
			}
		}
	}

	// Short Cut
	m_KeyTab.CmdsInit();
	for ( n = 0 ; n < m_KeyTab.m_Cmds.GetSize() ; n++ ) {
		if ( pMenu->GetMenuString(m_KeyTab.m_Cmds[n].m_Id, str, MF_BYCOMMAND) <= 0 ) {
			if ( pClipMenu != NULL && m_KeyTab.m_Cmds[n].m_Id == IDM_CLIPBOARD_MENU )
				CMenuLoad::UpdateMenuShortCut(pMenu, pClipMenu, m_KeyTab.m_Cmds[n].m_Menu);
			continue;
		}

		if ( (a = str.Find(_T('\t'))) >= 0 )
			str.Truncate(a);

		m_KeyTab.m_Cmds[n].m_Text = str;
		m_KeyTab.m_Cmds[n].SetMenu(pMenu);
	}

	// This Window Checked
	if ( ThisId != 0 )
		pMenu->CheckMenuItem(ThisId, MF_BYCOMMAND | MF_CHECKED);
}

void CRLoginDoc::EnvironText(CString &env, CString &str)
{
	int n;
	TCHAR c = _T('\0');
	DWORD size;
	TCHAR buf[MAX_COMPUTERNAME_LENGTH + 2];
	CString work, src, dis;
	LPCTSTR s;

	if ( (n = env.Find(_T(':'))) >= 0 ) {
		src = (LPCTSTR)env + n + 1;
		env.Delete(n, env.GetLength() - n);
		if ( (n = src.Find(_T('='))) >= 0 || (n = src.Find(_T(','))) >= 0 ) {
			c = src[n];
			dis = (LPCTSTR)src + n + 1;
			src.Delete(n, src.GetLength() - n);
		} else if ( src.CompareNoCase(_T("L")) == 0 || src.CompareNoCase(_T("U")) == 0 )
			c = src[0];
	}

	if ( env.IsEmpty() )
		return;

	if ( (s = _tgetenv(env)) == NULL ) {
		if ( env.CompareNoCase(_T("USER")) == 0 || env.CompareNoCase(_T("USERNAME")) == 0 ) {
			size = MAX_COMPUTERNAME_LENGTH;
			if ( !GetUserName(buf, &size) )
				return;
			s = buf;
		} else if ( env.CompareNoCase(_T("COMPUTER")) == 0 || env.CompareNoCase(_T("COMPUTERNAME")) == 0 ) {
			size = MAX_COMPUTERNAME_LENGTH;
			if ( !GetComputerName(buf, &size) )
				return;
			s = buf;
		} else if ( env.CompareNoCase(_T("BASEDIR")) == 0 ) {
			s = ((CRLoginApp *)::AfxGetApp())->m_BaseDir;
		} else if ( env.CompareNoCase(_T("RLOGINDIR")) == 0 ) {
			s = ((CRLoginApp *)::AfxGetApp())->m_ExecDir;
		} else if ( env.CompareNoCase(_T("RLOGINPATH")) == 0 ) {
			s = ((CRLoginApp *)::AfxGetApp())->m_PathName;
		} else
			return;
	}

	switch(c) {
	case _T('='):
		if ( (n = src.GetLength()) <= 0 )
			break;
		while ( *s != _T('\0') ) {
			if ( _tcsncmp(s, src, n) == 0 ) {
				s += n;
				work += dis;
			} else
				work += *(s++);
		}
		s = work;
		break;

	case _T(','):
		for ( n = _tstoi(src) ; n > 0 && *s != _T('\0') ; n-- )
			s++;
		for ( n = _tstoi(dis) ; n > 0 && *s != _T('\0') ; n-- )
			work += *(s++);
		s = work;
		break;

	case _T('L'):
		for ( n = 0 ; *s != _T('\0') ; n++ ) {
			if ( n > 0 && *s >= _T('A') && *s <= _T('Z') )
				work += (TCHAR)(*(s++) + 0x20);
			else if ( n == 0 && *s >= _T('a') && *s <= _T('z') )
				work += (TCHAR)(*(s++) - 0x20);
			else
				work += *(s++);
		}
		s = work;
		break;

	case _T('l'):
		while ( *s != _T('\0') ) {
			if ( *s >= _T('A') && *s <= _T('Z') )
				work += (TCHAR)(*(s++) + 0x20);
			else
				work += *(s++);
		}
		s = work;
		break;

	case _T('U'):
	case _T('u'):
		while ( *s != _T('\0') ) {
			if ( *s >= _T('a') && *s <= _T('z') )
				work += (TCHAR)(*(s++) - 0x20);
			else
				work += *(s++);
		}
		s = work;
		break;
	}

	str += s;
}
void CRLoginDoc::EnvironPath(CString &path)
{
	LPCTSTR s = path;
	LPCTSTR e, p;
	CString tmp, env;

	if ( (e = _tcsrchr(s, _T('\\'))) == NULL )
		return;

	while ( s < e ) {
		if ( s[0] == _T('%') && s[1] == _T('{') && (p = _tcschr(s + 2, _T('}'))) != NULL && p < e ) {
			env.Empty();
			for ( s += 2 ; s < p ; )
				env += *(s++);
			s++;
			EnvironText(env, tmp);
		} else
			tmp += *(s++);
	}

	path = tmp;
	path += e;
}
BOOL CRLoginDoc::EntryText(CString &name, LPCWSTR match)
{
	int n;
	TCHAR c;
	CEditDlg dlg;
	CPassDlg pass;
	CTime tm = CTime::GetCurrentTime();
	CString tmp;
	LPCTSTR str = name;
	BOOL st = FALSE;
	DWORD size;
	TCHAR buf[MAX_COMPUTERNAME_LENGTH + 2];
	CString work, env, src, dis;
	LPCTSTR s;

	while ( *str != _T('\0') ) {
		if ( *str == _T('%') ) {
			switch(str[1]) {
			case _T('E'):
				tmp += m_ServerEntry.m_EntryName;
				st = TRUE;
				break;
			case _T('G'):
				tmp += m_ServerEntry.m_Group;
				st = TRUE;
				break;
			case _T('U'):
				tmp += m_ServerEntry.m_UserName;
				st = TRUE;
				break;
			case _T('P'):
				tmp += m_ServerEntry.m_PassName;
				st = TRUE;
				break;
			case _T('T'):
				tmp += m_ServerEntry.m_TermName;
				st = TRUE;
				break;
			case _T('S'):
				tmp += m_ServerEntry.m_HostName;
				st = TRUE;
				break;
			case _T('p'):
				tmp += m_ServerEntry.m_PortName;
				st = TRUE;
				break;
			case _T('D'):
				tmp += tm.Format(_T("%y%m%d"));
				st = TRUE;
				break;
			case _T('t'):
				tmp += tm.Format(_T("%H%M%S"));
				st = TRUE;
				break;
			case _T('I'):
				if ( match != NULL ) {
					dlg.m_WinText = _T("ChatScript");
					dlg.m_Title = UniToTstr(match);
					dlg.m_Edit  = tmp;
					tmp.Empty();
					if ( dlg.DoModal() == IDOK )
						tmp = dlg.m_Edit;
				} else {
					dlg.m_WinText = _T("FileName");
					dlg.m_Title = m_ServerEntry.m_EntryName;
					dlg.m_Edit  = tmp;
					tmp.Empty();
					if ( dlg.DoModal() == IDOK )
						tmp = dlg.m_Edit;
					st = TRUE;
				}
				break;
			case L'i':
				pass.m_Title    = m_ServerEntry.m_EntryName;
				pass.m_HostAddr = m_ServerEntry.m_HostName;
				pass.m_PortName = m_ServerEntry.m_PortName;
				pass.m_UserName = m_ServerEntry.m_UserName;
				pass.m_PassName = m_ServerEntry.m_PassName;
				pass.m_Prompt   = _T("Password");
				pass.m_MaxTime  = 120;
				if ( pass.DoModal() == IDOK ) {
					m_ServerEntry.m_HostName = pass.m_HostAddr;
					m_ServerEntry.m_PortName = pass.m_PortName;
					m_ServerEntry.m_UserName = pass.m_UserName;
					m_ServerEntry.m_PassName = pass.m_PassName;
				}
				break;
			case _T('s'):
				tmp += m_SockStatus;
				st = TRUE;
				break;
			case _T('u'):
				size = MAX_COMPUTERNAME_LENGTH;
				if ( GetUserName(buf, &size) )
					tmp += buf;
				st = TRUE;
				break;
			case _T('h'):
				size = MAX_COMPUTERNAME_LENGTH;
				if ( GetComputerName(buf, &size) )
					tmp += buf;
				st = TRUE;
				break;
			case _T('%'):
				tmp += _T('%');
				st = TRUE;
				break;
			case _T('{'):
				env.Empty();
				for ( s = str + 2 ; *s != _T('}') ; ) {
					if ( *s == _T('\0') )
						break;
					env += *(s++);
				}
				if ( *s != _T('}') ) {
					tmp += str[0];
					tmp += str[1];
					break;
				}
				str = s + 1 - 2;
				EnvironText(env, tmp);
				st = TRUE;
				break;
			default:
				tmp += str[0];
				tmp += str[1];
				break;
			}
			str += 2;

		} else if ( match == NULL && *str == _T('$') ) {
			if ( str[1] == '$' ) {
				tmp += *(str++);
				str++;
				st = TRUE;
			} else if ( str[1] == '{' ) {
				env.Empty();
				for ( s = str + 2 ; *s != _T('}') ; ) {
					if ( *s == _T('\0') )
						break;
					env += *(s++);
				}
				if ( *s != _T('}') ) {
					tmp += *(str++);
					tmp += *(str++);
					break;
				}
				str = s + 1;
				EnvironText(env, tmp);
				st = TRUE;
			} else {
				env.Empty();
				for ( str++ ; *str != '\0' && _istalnum(*str) ; )
					env += *(str++);
				EnvironText(env, tmp);
				st = TRUE;
			}

		} else if ( match != NULL && *str == _T('\\') ) {
			switch(str[1]) {
			case _T('a'): tmp += _T('\x07'); str += 2; break;
			case _T('b'): tmp += _T('\x08'); str += 2; break;
			case _T('t'): tmp += _T('\x09'); str += 2; break;
			case _T('n'): tmp += _T('\x0A'); str += 2; break;
			case _T('v'): tmp += _T('\x0B'); str += 2; break;
			case _T('f'): tmp += _T('\x0C'); str += 2; break;
			case _T('r'): tmp += _T('\x0D'); str += 2; break;
			case _T('\\'): tmp += _T('\\'); str += 2; break;

			case _T('x'): case _T('X'):
				str += 2;
				for ( n = c = 0 ; n < 2 ; n++ ) {
					if ( *str >= _T('0') && *str <= _T('9') )
						c = c * 16 + (*(str++) - _T('0'));
					else if ( *str >= _T('A') && *str <= _T('F') )
						c = c * 16 + (*(str++) - _T('A') + 10);
					else if ( *str >= _T('a') && *str <= _T('f') )
						c = c * 16 + (*(str++) - _T('a') + 10);
					else
						break;
				}
				tmp += c;
				break;

			case _T('0'): case _T('1'): case _T('2'): case _T('3'):
			case _T('4'): case _T('5'): case _T('6'): case _T('7'):
				str += 1;
				for ( n = c = 0 ; n < 3 ; n++ ) {
					if ( *str >= _T('0') && *str <= _T('7') )
						c = c * 8 + (*(str++) - _T('0'));
						break;
				}
				tmp += c;
				break;

			default:
				str += 1;
				break;
			}
			st = TRUE;

		} else
			tmp += *(str++);
	}

	if ( st )
		name = tmp;

	return st;
}

void CRLoginDoc::SendScript(LPCWSTR str, LPCWSTR match)
{
	CString work;
	CStringW tmp;
	CBuffer buf;

	work = UniToTstr(str);
	EntryText(work, match);
	tmp = TstrToUni(work);

	if ( tmp.IsEmpty() )
		return;

	buf.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
	SendBuffer(buf);
}
int CRLoginDoc::DelaySend()
{
	int n = 0;

	if ( m_pSock == NULL ) {
		m_DelayBuf.Clear();
		m_bDelayPast = FALSE;
		return FALSE;
	}

	while ( n < m_DelayBuf.GetSize() ) {
		if ( m_DelayBuf.GetAt(n++) == '\r' ) {
			m_pSock->Send(m_DelayBuf.GetPtr(), n, 0);
			m_DelayBuf.Consume(n);
			m_DelayFlag = DELAY_WAIT;
			m_pMainWnd->SetTimerEvent(DELAY_ECHO_MSEC, TIMEREVENT_DOC, this);
			if ( m_DelayBuf.GetSize() == 0 )
				m_bDelayPast = FALSE;
			return TRUE;
		}
	}
	if ( n > 0 ) {
		m_pSock->Send(m_DelayBuf.GetPtr(), m_DelayBuf.GetSize(), 0);
		m_DelayBuf.Clear();
		m_bDelayPast = FALSE;
	}

	return FALSE;
}

void CRLoginDoc::InitOptFixCheck(int Uid)
{
	int n;
	CServerEntryTab *pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);

	m_OptFixCheck.RemoveAll();
	m_OptFixCheck.SetSize(pTab->m_Data.GetSize());

	for ( n = 0 ; n < m_OptFixCheck.GetSize() ; n++ )
		m_OptFixCheck[n] = (Uid == pTab->m_Data[n].m_Uid ? TRUE : FALSE);
}
BOOL CRLoginDoc::SetOptFixEntry(LPCTSTR entryName)
{
	int n;
	CServerEntryTab *pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);

	if ( entryName == NULL || entryName[0] == _T('\0') )
		return FALSE;

	for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
		if ( pTab->m_Data[n].m_EntryName.Compare(entryName) == 0 ) {
			if ( m_OptFixCheck[n] != FALSE ) {
				AfxMessageBox(IDE_OPTFIXDEEPENTRY);
				return FALSE;
			}

			m_OptFixCheck[n] = TRUE;

			if ( pTab->m_Data[n].m_bOptFixed )
				return SetOptFixEntry(pTab->m_Data[n].m_OptFixEntry);

			LoadOption(pTab->m_Data[n], m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab);

			// Fix 2019.09.11
			m_ServerEntry.m_ScriptFile = pTab->m_Data[n].m_ScriptFile;
			m_ServerEntry.m_ScriptStr  = pTab->m_Data[n].m_ScriptStr;

			return TRUE;
		}
	}

	AfxMessageBox(IDE_OPTFIXNOTFOUND);
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CRLoginDoc::LogOpen(LPCTSTR filename)
{
	if ( m_pLogFile == NULL && (m_pLogFile = new CFileExt) == NULL )
		return FALSE;

	if ( !m_pLogFile->Open(filename, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyWrite) )
		return FALSE;

	if ( m_TextRam.IsOptEnable(TO_RLLOGFLUSH) )
		m_pLogFile->m_bWriteFlush = TRUE;

	m_pLogFile->SeekToEnd();
	m_TextRam.m_LogTimeFlag = TRUE;
	m_TextRam.m_LogCurY = (-1);

	if ( m_TextRam.m_LogMode == LOGMOD_CTRL ) {
		m_TextRam.m_TraceLogMode |= 002;
		m_TextRam.m_pCallPoint = &CTextRam::fc_TraceCall;
	}

	return TRUE;
}
BOOL CRLoginDoc::LogClose()
{
	if ( (m_TextRam.m_TraceLogMode & 002) != 0 ) {
		m_TextRam.m_TraceLogMode &= ~002;
		m_TextRam.m_pCallPoint = (m_TextRam.m_TraceLogMode != 0 ? &CTextRam::fc_TraceCall : &CTextRam::fc_FuncCall);
	}

	if ( m_pLogFile == NULL )
		return FALSE;

	if ( m_TextRam.m_LogMode == LOGMOD_LINE && m_TextRam.m_LogCurY != (-1) )
		m_TextRam.CallReceiveLine(m_TextRam.m_LogCurY - m_TextRam.m_HisPos);

	LogWrite(NULL, 0, LOGDEBUG_NONE);
	m_TextRam.SaveLogFile();
	m_pLogFile->Close();

	delete m_pLogFile;
	m_pLogFile = NULL;

	return TRUE;
}
void CRLoginDoc::LogWrite(LPBYTE lpBuf, int nBufLen, int SendRecv)
{
	CStringA mbs;

	if ( m_pLogFile == NULL || m_TextRam.m_LogMode != LOGMOD_DEBUG )
		return;

NEWLINE:
	if ( SendRecv != m_LogSendRecv ) {
		if ( m_LogSendRecv != LOGDEBUG_NONE )
			m_pLogFile->Write("\r\n", 2);

		if ( nBufLen > 0 ) {
			CTime now = CTime::GetCurrentTime();

			if ( m_TextRam.IsOptEnable(TO_RLLOGTIME) ) {
				mbs = now.Format(m_TextRam.m_TimeFormat);
				m_pLogFile->Write((LPCSTR)mbs, mbs.GetLength());
			}

			if ( SendRecv == LOGDEBUG_RECV )
				m_pLogFile->Write("RECV:", 5);
			else if ( SendRecv == LOGDEBUG_SEND )
				m_pLogFile->Write("SEND:", 5);
			else if ( SendRecv == LOGDEBUG_INSIDE )
				m_pLogFile->Write("DEBUG:", 6);

			m_LogSendRecv = SendRecv;

		} else
			m_LogSendRecv = LOGDEBUG_NONE;
	}

	while ( nBufLen-- > 0 ) {
		if ( *lpBuf < ' ' || *lpBuf == '[' || *lpBuf >= 0x7F ) {
			mbs.Format("[%02x]", *lpBuf);
			m_pLogFile->Write((LPCSTR)mbs, mbs.GetLength());
			if ( *(lpBuf++) == 0x0A ) {
				m_LogSendRecv = LOGDEBUG_FLASH;
				goto NEWLINE;
			}
		} else
			m_pLogFile->Write(lpBuf++, 1);
	}
}
void CRLoginDoc::LogDebug(LPCSTR str, ...)
{
	CStringA tmp;
	va_list arg;

	if ( m_pLogFile == NULL || m_TextRam.m_LogMode != LOGMOD_DEBUG )
		return;

	va_start(arg, str);
	tmp.FormatV(str, arg);
	va_end(arg);

	LogWrite((LPBYTE)(LPCSTR)tmp, tmp.GetLength(), LOGDEBUG_INSIDE);
	LogWrite(NULL, 0, LOGDEBUG_FLASH);
}
void CRLoginDoc::LogDump(LPBYTE lpBuf, int nBufLen)
{
	int n, addr = 0;
	CStringA tmp, str;

	if ( m_pLogFile == NULL || m_TextRam.m_LogMode != LOGMOD_DEBUG )
		return;

	while ( nBufLen > 0 ) {
		str.Format("%04x  ", addr);
		tmp = str;

		for ( n = 0 ; n < 16 ; n++ ) {
			if ( n == 8 )
				tmp += ' ';
			if ( n < nBufLen ) {
				str.Format("%02x ", lpBuf[n]);
				tmp += str;
			} else
				tmp += "   ";
		}
		tmp += ' ';

		for ( n = 0 ; n < nBufLen && n < 16 ; n++ ) {
			if ( n == 8 )
				tmp += ' ';
			if ( lpBuf[n] < ' ' || lpBuf[n] >= 0x7F )
				tmp += '.';
			else
				tmp += (CHAR)lpBuf[n];
		}

		LogWrite((LPBYTE)(LPCSTR)tmp, tmp.GetLength(), LOGDEBUG_INSIDE);
		LogWrite(NULL, 0, LOGDEBUG_FLASH);

		tmp.Empty();
		nBufLen -= 16;
		lpBuf   += 16;
		addr    += 16;
	}
}
void CRLoginDoc::LogInit()
{
	if ( m_TextRam.IsOptEnable(TO_RLHISDATE) && !m_TextRam.m_LogFile.IsEmpty() ) {
		int n, num;
		CString file, dirs, name, exts;

		if ( (n = m_TextRam.m_LogFile.ReverseFind(_T('\\'))) >= 0 || (n = m_TextRam.m_LogFile.ReverseFind(_T(':'))) >= 0 ) {
			dirs = m_TextRam.m_LogFile.Left(n + 1);
			name = m_TextRam.m_LogFile.Mid(n + 1);
		} else {
			dirs = _T("");
			name = m_TextRam.m_LogFile;
		}

		if ( (n = name.ReverseFind(_T('.'))) >= 0 ) {
			exts = name.Mid(n);
			name.Delete(n, exts.GetLength());
		}

		if ( !EntryText(name) ) {
			name += _T("-%D");
			EntryText(name);
		}

		LogClose();

		if ( (m_pLogFile = new CFileExt) == NULL )
			return;

		file.Format(_T("%s%s%s"), dirs, name, exts);

		for ( num = 1 ; num < 20 ; num++ ) {
			if ( LogOpen(file) )
				break;
			file.Format(_T("%s%s-%d%s"), dirs, name, num, exts);
		}
		if ( num >= 20 ) {
			if ( m_pLogFile != NULL ) {
				delete m_pLogFile;
				m_pLogFile = NULL;
			}
			file.Format(_T("LogFile Open Error '%s%s%s'"), dirs, name, exts);
			AfxMessageBox(file);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

void CRLoginDoc::DoDropFile()
{
	TCHAR *p;
	CString path;
	CStringW cmd, file;
	CKeyNode fmt;
	CBuffer tmp;

	if ( m_pBPlus != NULL && !m_pBPlus->m_ResvPath.IsEmpty() )
		path = m_pBPlus->m_ResvPath.GetHead();
	else if ( m_pZModem != NULL && !m_pZModem->m_ResvPath.IsEmpty() )
		path = m_pZModem->m_ResvPath.GetHead();
	else if ( m_pKermit != NULL && !m_pKermit->m_ResvPath.IsEmpty() )
		path = m_pKermit->m_ResvPath.GetHead();
	else
		return;

	if ( (p = _tcsrchr((TCHAR *)(LPCTSTR)path, _T('\\'))) != NULL || (p = _tcsrchr((TCHAR *)(LPCTSTR)path, _T(':'))) != NULL )
		file = p + 1;
	else
		file = path;

	fmt.SetMaps(m_TextRam.m_DropFileCmd[m_TextRam.m_DropFileMode]);
	fmt.CommandLine(file, cmd);
	tmp.Apend((LPBYTE)(LPCWSTR)cmd, cmd.GetLength() * sizeof(WCHAR));
	SendBuffer(tmp);

	switch(m_TextRam.m_DropFileMode) {
	case 2: m_pZModem->DoProc(1); break;
	case 3: m_pZModem->DoProc(2); break;
	case 4: m_pZModem->DoProc(7); break;
	case 6: m_pKermit->DoProc(1); break;
	case 7: m_pKermit->DoProc(3); break;
	}
}

CWnd *CRLoginDoc::GetAciveView()
{
	POSITION pos;
	CWnd *pView = NULL;
	CWnd *pWnd;
	CWnd *pAct = ::AfxGetMainWnd()->GetActiveWindow();

	pos = GetFirstViewPosition();
	while ( pos != NULL ) {
		pWnd = GetNextView(pos);
		if ( pView == NULL )
			pView = pWnd;
		if ( pAct != NULL && pAct->GetSafeHwnd() == pWnd->GetSafeHwnd() )
			return pWnd;
		if ( pWnd->GetSafeHwnd() == ::GetFocus() )
			return pWnd;
	}
	return pView;
}
int CRLoginDoc::GetViewCount()
{
	int count;
	POSITION pos = GetFirstViewPosition();

	for ( count = 0 ; pos != NULL ; count++ )
		GetNextView(pos);

	return count;
}
BOOL CRLoginDoc::IsCanExit()
{
	if ( !m_TextRam.IsOptEnable(TO_RLPOFF) )
		return FALSE;

	CRLoginDoc *pDoc;
	CDocTemplate *pDocTemp = GetDocTemplate();

	if ( pDocTemp != NULL ) {
		POSITION pos = pDocTemp->GetFirstDocPosition();

		while (pos != NULL) {
			pDoc = (CRLoginDoc *)(pDocTemp->GetNextDoc(pos));
			if ( pDoc != NULL && pDoc != this && pDoc->m_pSock != NULL)
				return FALSE;
		}
	}

	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////

void CRLoginDoc::OnReceiveChar(DWORD ch, int pos)
{
	LPCWSTR str;

	if ( m_pScript != NULL && !m_pScript->m_bConsOut ) {
		if ( m_pScript->m_RegMatch < 0 )
			m_pScript->OnSockChar(CTextRam::UCS2toUCS4(ch));
		if ( m_pScript->m_ConsMode != DATA_BUF_NONE && ch != 0 ) {
			DWORD tmp[2];
			tmp[0] = ch;
			tmp[1] = pos;
			m_pScript->SetConsBuff((LPBYTE)tmp, 2 * sizeof(DWORD));
		}
	}

	while ( m_pStrScript != NULL && (str = m_pStrScript->ExecChar(ch)) != NULL ) {
		SendScript(str, m_pStrScript->m_Res.m_Str);
		if ( m_pStrScript->m_Exec == NULL ) {
			m_pStrScript->ExecStop();
			m_pStrScript = NULL;
			SetDocTitle();
			break;
		}
		ch = 0;
	}
}
void CRLoginDoc::OnSendBuffer(CBuffer &buf)
{
	if ( m_pStrScript != NULL && m_pStrScript->m_MakeFlag )
		m_pStrScript->SendStr((LPCWSTR)(buf.GetPtr()), buf.GetSize() / sizeof(WCHAR), &m_ServerEntry);
}
void CRLoginDoc::OnDelayReceive(int ch)
{
	((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);

	if ( ch >= 0 && m_TextRam.m_DelayMSec > 0 ) {
		m_DelayFlag = DELAY_PAUSE;
		m_pMainWnd->SetTimerEvent(m_TextRam.m_DelayMSec, TIMEREVENT_DOC, this);
		return;
	}

	m_DelayFlag = DELAY_NON;
	DelaySend();
}

void CRLoginDoc::OnSocketConnect()
{
	if ( m_pSock == NULL )
		return;

	if ( m_ServerEntry.m_HostName.Compare(m_ServerEntry.m_HostNameProvs) != 0 )
		UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	UpdateAllViews(NULL, UPDATE_GOTOXY, NULL);

	if ( m_pScript != NULL )
		m_pScript->OnConnect();

	m_ServerEntry.m_ChatScript.ExecInit();
	if ( m_ServerEntry.m_ChatScript.Status() != SCPSTAT_NON ) {
		m_pStrScript = &(m_ServerEntry.m_ChatScript);
		OnReceiveChar(0, (-1));
	}

	if ( m_AfterId != (-1) ) {
		((CMainFrame *)::AfxGetMainWnd())->PostMessage(WM_AFTEROPEN, (WPARAM)m_AfterId, (LPARAM)0);
		m_AfterId = (-1);
	}

	time(&m_ConnectTime);
	SetStatus(_T("Connect"));
}
void CRLoginDoc::OnSocketError(int err)
{
	CWnd *pWnd;
	CString tmp;

	SetStatus(_T("Error"));
	time(&m_CloseTime);

	if ( m_pStrScript != NULL ) {
		m_pStrScript->ExecStop();
		m_pStrScript = NULL;
	}

	if ( m_pScript != NULL )
		m_pScript->OnClose();

	if ( m_pBPlus != NULL )
		m_pBPlus->DoAbort();
	if ( m_pZModem != NULL )
		m_pZModem->DoAbort();
	if ( m_pKermit != NULL )
		m_pKermit->DoAbort();

	if ( m_AfterId != (-1) ) {
		((CMainFrame *)::AfxGetMainWnd())->PostMessage(WM_AFTEROPEN, (WPARAM)m_AfterId, (LPARAM)err);
		m_AfterId = (-1);
	}

	if ( m_ErrorPrompt.IsEmpty() )
		tmp = CExtSocket::GetFormatErrorMessage(m_ServerEntry.m_EntryName, m_ServerEntry.m_HostName, CExtSocket::GetPortNum(m_ServerEntry.m_PortName), _T("WinSock"), err);
	else
		tmp.Format(_T("%s Server Entry Socket Error\n%s:%s Connection\n%s\n"), m_ServerEntry.m_EntryName, m_ServerEntry.m_HostName, m_ServerEntry.m_PortName, m_ErrorPrompt);

	m_ErrorPrompt.Empty();

	if ( (pWnd = GetAciveView()) != NULL ) {
		tmp += _T("\n");
		tmp += CStringLoad(IDS_SOCKREOPEN);
		if ( AfxMessageBox(tmp, MB_ICONERROR | MB_YESNO) == IDYES )
			pWnd->PostMessage(WM_COMMAND, IDM_REOPENSOCK, (LPARAM)0);
		else if ( m_TextRam.IsOptEnable(TO_RLNOTCLOSE) || ((CMainFrame *)::AfxGetMainWnd())->IsTimerIdleBusy() )
			UpdateAllViews(NULL, UPDATE_DISPMSG, (CObject *)_T("Error"));
		else
			pWnd->PostMessage(WM_COMMAND, ID_FILE_CLOSE, (LPARAM)0);
	} else {
		AfxMessageBox(tmp);
		OnFileClose();
	}
}
void CRLoginDoc::OnSocketClose()
{
	if ( m_pSock == NULL )
		return;

	if ( m_pStrScript != NULL ) {
		m_pStrScript->ExecStop();
		m_pStrScript = NULL;
	}

	if ( m_pScript != NULL )
		m_pScript->OnClose();

	if ( m_pBPlus != NULL )
		m_pBPlus->DoAbort();
	if ( m_pZModem != NULL )
		m_pZModem->DoAbort();
	if ( m_pKermit != NULL )
		m_pKermit->DoAbort();

	UpdateAllViews(NULL, UPDATE_GOTOXY, NULL);
	SetStatus(_T("Close"));
	time(&m_CloseTime);

	CWnd *pWnd = GetAciveView();

	if (m_TextRam.IsOptEnable(TO_RLREOPEN) && pWnd != NULL && AfxMessageBox(IDS_SOCKREOPEN, MB_ICONQUESTION | MB_YESNO) == IDYES )
		pWnd->PostMessage(WM_COMMAND, IDM_REOPENSOCK, (LPARAM)0);
	else if ( m_TextRam.IsOptEnable(TO_RLNOTCLOSE) || ((CMainFrame *)::AfxGetMainWnd())->IsTimerIdleBusy() )
		UpdateAllViews(NULL, UPDATE_DISPMSG, (CObject *)_T("Closed"));
	else if ( m_bExitPause ) {
		m_bExitPause = FALSE;
		UpdateAllViews(NULL, UPDATE_DISPMSG, (CObject *)_T("Paused"));
		m_pMainWnd->SetTimerEvent(DELAY_CLOSE_SOCKET, TIMEREVENT_CLOSE, this);
	} else if ( IsCanExit() )
		m_pMainWnd->PostMessage(WM_COMMAND, ID_APP_EXIT, 0 );
	else if ( pWnd != NULL )
		pWnd->PostMessage(WM_COMMAND, ID_FILE_CLOSE, (LPARAM)0);
	else
		OnFileClose();
}
int CRLoginDoc::OnSocketReceive(LPBYTE lpBuf, int nBufLen, int nFlags)
{
	int n;
	BOOL sync = FALSE;

//	TRACE("OnSocketReceive %dByte\n", nBufLen);

	if ( !m_TextRam.IsInitText() )
		return 0;

	if ( nFlags != 0 )
		return nBufLen;

	if ( m_pScript != NULL ) {
		if ( m_pScript->IsSockOver() ) {
			for ( n = 0 ; n < 100 ; n++ ) { 
				if ( !m_pScript->Exec() )
					break;
			}
		}
		if ( m_pScript->IsSockOver() )
			return 0;
		if ( m_pScript->m_SockMode == DATA_BUF_HAVE ) {
			m_pScript->SetSockBuff(lpBuf, nBufLen);
			return nBufLen;
		}
	}

	n = m_TextRam.Write(lpBuf, (nBufLen < RECVDEFSIZ ? nBufLen : RECVDEFSIZ), &sync);

	if ( sync ) {
		m_pSock->SetRecvSyncMode(TRUE);
		if ( lpBuf[n] == 0x18 ) {	// CAN
			if ( m_pZModem == NULL )
				m_pZModem = new CZModem(this, AfxGetMainWnd());
			m_pZModem->DoProc(7);	// ZMODEM UpDown
		} else if ( lpBuf[n] == 0x01 ) {
			if ( m_pKermit == NULL )
				m_pKermit = new CKermit(this, AfxGetMainWnd());
			m_pKermit->DoProc(0);	// Kermit DownLoad
		} else {
			if ( m_pBPlus == NULL )
				m_pBPlus = new CBPlus(this, AfxGetMainWnd());
			m_pBPlus->DoProc(lpBuf[n]);
		}
	}

	if ( (nBufLen - n + m_pSock->GetRecvProcSize()) > 0 )
		((CMainFrame *)::AfxGetMainWnd())->PostIdleMessage();		// OnIdleの誘発

	nBufLen = n;

	// 即時画面更新
	if ( m_TextRam.IsOptEnable(TO_RLPSUPWIN) )
		UpdateAllViews(NULL, UPDATE_UPDATEWINDOW, NULL);

	if ( m_pLogFile != NULL ) {
		if ( m_TextRam.m_LogMode == LOGMOD_RAW )
			m_pLogFile->Write(lpBuf, nBufLen);
		else if ( m_TextRam.m_LogMode == LOGMOD_DEBUG )
			LogWrite(lpBuf, nBufLen, LOGDEBUG_RECV);

		if ( m_pLogFile->IsWriteError() )
			LogClose();
	}

	m_pMainWnd->WakeUpSleep();
	UpdateAllViews(NULL, UPDATE_WAKEUP, NULL);

	if ( m_pScript != NULL && m_pScript->m_SockMode == DATA_BUF_BOTH )
		m_pScript->SetSockBuff(lpBuf, nBufLen);

	return nBufLen;
}
void CRLoginDoc::OnSockReOpen()
{
	SocketClose();
	SocketOpen();
}

/////////////////////////////////////////////////////////////////////////////

int CRLoginDoc::SocketOpen()
{
	BOOL rt;
	CPassDlg dlg;
	CStringArrayExt hosts;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	if ( m_pSock != NULL )
		return FALSE;

	if ( InternetAttemptConnect(0) != ERROR_SUCCESS )
		return FALSE;

	if ( !m_ServerEntry.m_BeforeEntry.IsEmpty() && m_ServerEntry.m_BeforeEntry.Compare(m_ServerEntry.m_EntryName) != 0 && !((CRLoginApp *)::AfxGetApp())->IsOnlineEntry(m_ServerEntry.m_BeforeEntry) ) {
		CString cmds;
		cmds.Format(_T("/Entry \"%s\" /After %d"), m_ServerEntry.m_BeforeEntry, ((CMainFrame *)::AfxGetMainWnd())->SetAfterId((void *)this));
		pApp->OpenCommandLine(cmds);
		return TRUE;
	}

	EntryText(m_ServerEntry.m_UserName);
	EntryText(m_ServerEntry.m_ProxyUser);

	if ( m_ServerEntry.m_ReEntryFlag )
		goto SKIPINPUT;

	// 0=NONE, 1=HTTP, 2=SOCKS4, 3=SOCKS5, 4=HTTP(Basic)
	if ( (m_ServerEntry.m_ProxyMode & 7) > 0 && (m_TextRam.IsOptEnable(TO_PROXPASS) || !m_ServerEntry.m_bPassOk) ) {

		dlg.m_Title    = m_ServerEntry.m_EntryName;
		dlg.m_Title   += _T("(Proxy Server)");
		dlg.m_HostAddr = m_ServerEntry.m_ProxyHost;
		dlg.m_PortName = m_ServerEntry.m_ProxyPort;
		dlg.m_UserName = m_ServerEntry.m_ProxyUser;
		dlg.m_PassName = m_ServerEntry.m_ProxyPass;
		dlg.m_Prompt   = _T("Proxy Password");
		dlg.m_MaxTime  = 120;

		if ( dlg.DoModal() != IDOK )
			return FALSE;

		m_ServerEntry.m_ProxyHost = dlg.m_HostAddr;
		m_ServerEntry.m_ProxyPort = dlg.m_PortName;
		m_ServerEntry.m_ProxyUser = dlg.m_UserName;
		m_ServerEntry.m_ProxyPass = dlg.m_PassName;

		if ( !m_ServerEntry.m_bPassOk ) {
			m_ServerEntry.m_ProxyPassProvs = m_ServerEntry.m_ProxyPass;
			if ( !m_ServerEntry.m_PassName.IsEmpty() )
				m_ServerEntry.m_bPassOk = TRUE;
			SetModifiedFlag(TRUE);
		}
	}

	// 0=PROTO_DIRECT, 1=PROTO_LOGIN, 2=PROTO_TELNET, 3=PROTO_SSH, 4=PROTO_COMPORT 5=PROTO_PIPE
	if ( m_ServerEntry.m_HostName.IsEmpty() || m_bReqDlg ||
		 ((m_TextRam.IsOptEnable(TO_RLUSEPASS) || !m_ServerEntry.m_bPassOk) &&
		   m_ServerEntry.m_ProtoType >= PROTO_LOGIN && m_ServerEntry.m_ProtoType <= PROTO_COMPORT) ) {

		dlg.m_Title    = m_ServerEntry.m_EntryName;
		dlg.m_HostAddr = m_ServerEntry.m_HostName;
		dlg.m_PortName = m_ServerEntry.m_PortName;
		dlg.m_UserName = m_ServerEntry.m_UserName;
		dlg.m_PassName = m_ServerEntry.m_PassName;
		dlg.m_Prompt   = _T("Password");
		if ( m_ServerEntry.m_ProtoType == PROTO_SSH )
			dlg.m_Prompt += _T("/phrase");
		dlg.m_MaxTime  = 120;

		if ( dlg.DoModal() != IDOK )
			return FALSE;

		m_ServerEntry.m_HostName = dlg.m_HostAddr;
		m_ServerEntry.m_PortName = dlg.m_PortName;
		m_ServerEntry.m_UserName = dlg.m_UserName;
		m_ServerEntry.m_PassName = dlg.m_PassName;

		if ( !m_ServerEntry.m_bPassOk ) {
			m_ServerEntry.m_PassNameProvs = m_ServerEntry.m_PassName;
			m_ServerEntry.m_bPassOk = TRUE;
			SetModifiedFlag(TRUE);
		}

		CCommandLineInfoEx cmds;
		cmds.SetString(m_CmdLine);
		cmds.m_Addr = m_ServerEntry.m_HostName;
		//cmds.m_User = m_ServerEntry.m_UserName;
		//cmds.m_Pass = m_ServerEntry.m_PassName;	// あまりにもセキュリティーが低い！
		cmds.GetString(m_CmdLine);
	}

SKIPINPUT:

	if ( m_ServerEntry.m_HostName.IsEmpty() )
		return FALSE;

	if ( m_ServerEntry.m_ProtoType <= PROTO_SSH ) {
		hosts.GetParam(m_ServerEntry.m_HostName);

		if ( hosts.GetSize() > 1 ) {
			if ( hosts.GetSize() > 20 && AfxMessageBox(IDS_TOOMANYHOSTNAME, MB_ICONWARNING | MB_YESNO) != IDYES )
				return FALSE;
			CCommandLineInfoEx cmds;
			cmds.ParseParam(_T("inpane"), TRUE, FALSE);
			SetEntryProBuffer();
			for ( int n = 1 ; n < hosts.GetSize() ; n++ ) {
				m_ServerEntry.m_HostName.Format(_T("'%s'"), hosts[n]);
				m_ServerEntry.m_ReEntryFlag = TRUE;
				pApp->m_pServerEntry = &m_ServerEntry;
				pApp->OpenProcsCmd(&cmds);
			}
			m_InPane = TRUE;
			m_ServerEntry.m_HostName = hosts[0];

		} else if ( hosts.GetSize() > 0 )
			m_ServerEntry.m_HostName = hosts[0];
	}

	// AfxMessageBoxでメッセージが先に進む可能性あり
	LogInit();

	switch(m_ServerEntry.m_ProtoType) {
	case PROTO_DIRECT:  m_pSock = new CExtSocket(this);  break;
	case PROTO_LOGIN:   m_pSock = new CLogin(this);		 break;	// login
	case PROTO_TELNET:  m_pSock = new CTelnet(this);	 break;	// telnet
	case PROTO_SSH:     m_pSock = new Cssh(this);		 break;	// ssh
	case PROTO_COMPORT: m_pSock = new CComSock(this);	 break;	// com port
	case PROTO_PIPE:    m_pSock = new CPipeSock(this);   break; // pipe console
	}

	if ( m_ServerEntry.m_ProxyMode != 0 )
		rt = m_pSock->ProxyOpen(m_ServerEntry.m_ProxyMode, m_ServerEntry.m_ProxySSLKeep,
			m_ServerEntry.m_ProxyHost, CExtSocket::GetPortNum(m_ServerEntry.m_ProxyPort),
			m_ServerEntry.m_ProxyUser, m_ServerEntry.m_ProxyPass, m_ServerEntry.m_HostName,
			CExtSocket::GetPortNum(m_ServerEntry.m_PortName));
	else 
		rt = m_pSock->AsyncOpen(m_ServerEntry.m_HostName,
			CExtSocket::GetPortNum(m_ServerEntry.m_PortName));

	if ( !rt ) {
		SocketClose();
		return FALSE;
	}

	SetStatus(_T("Open"));
	SetDocTitle();

	return TRUE;
}
void CRLoginDoc::SocketClose()
{
	if ( m_pSock == NULL )
		return;
	if ( m_pBPlus != NULL )
		m_pBPlus->DoAbort();
	if ( m_pZModem != NULL )
		m_pZModem->DoAbort();
	if ( m_pKermit != NULL )
		m_pKermit->DoAbort();
	m_pSock->Destroy();
	m_pSock = NULL;
	m_TextRam.OnClose();
}
int CRLoginDoc::SocketReceive(void *lpBuf, int nBufLen)
{
	if ( m_pSock == NULL )
		return 0;
	return m_pSock->Receive(lpBuf, nBufLen, 0);
}
void CRLoginDoc::SocketSend(void *lpBuf, int nBufLen, BOOL delaySend)
{
	if ( m_pSock == NULL )
		return;

	//if ( m_pScript != NULL && m_pScript->m_ConsMode == DATA_BUF_HAVE ) {
	//	m_pScript->SetConsBuff((LPBYTE)lpBuf, nBufLen);
	//	return;
	//}

	if ( delaySend || m_bDelayPast || m_TextRam.IsOptEnable(TO_RLDELAY) ) {
		m_bDelayPast = TRUE;
		m_DelayBuf.Apend((LPBYTE)lpBuf, nBufLen);
		if ( m_DelayFlag == DELAY_NON )
			DelaySend();
	} else 
		m_pSock->Send(lpBuf, nBufLen);

	if ( m_pLogFile != NULL && m_TextRam.m_LogMode == LOGMOD_DEBUG )
		LogWrite((LPBYTE)lpBuf, nBufLen, LOGDEBUG_SEND);
}
LPCSTR CRLoginDoc::Utf8Str(LPCTSTR str)
{
	m_TextRam.m_IConv.StrToRemote(m_TextRam.m_SendCharSet[UTF8_SET], str, m_WorkMbs);
	return m_WorkMbs;
}
LPCSTR CRLoginDoc::RemoteStr(LPCTSTR str)
{
	m_TextRam.m_IConv.StrToRemote(m_TextRam.m_SendCharSet[m_TextRam.m_KanjiMode], str, m_WorkMbs);
	return m_WorkMbs;
}
LPCTSTR CRLoginDoc::LocalStr(LPCSTR str)
{
	m_TextRam.m_IConv.RemoteToStr(m_TextRam.m_SendCharSet[m_TextRam.m_KanjiMode], str, m_WorkStr);
	return m_WorkStr;
}

/////////////////////////////////////////////////////////////////////////////

void CRLoginDoc::OnLogOpen() 
{
	if ( LogClose() )
		return;

	CFileDialog dlg(FALSE, _T("log"), _T("RLOGIN"), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGLOGFILE), AfxGetMainWnd());

	if ( dlg.DoModal() != IDOK )
		return;

	if ( !LogOpen(dlg.GetPathName()) ) {
		AfxMessageBox(IDE_LOGOPENERROR);
		delete m_pLogFile;
		m_pLogFile = NULL;
		return;
	}
}
void CRLoginDoc::OnUpdateLogOpen(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_pLogFile != NULL ? 1 : 0);
}

void CRLoginDoc::OnLoadDefault() 
{
	if ( AfxMessageBox(IDS_ALLINITREQ, MB_ICONQUESTION | MB_YESNO) != IDYES )
		return;

	LoadDefOption(m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab);

	SetModifiedFlag(TRUE);
	UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
}
void CRLoginDoc::OnSaveDefault() 
{
	SaveDefOption(m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab);
}
void CRLoginDoc::OnSetOption() 
{
	int LogMode = m_TextRam.m_LogMode;
	COptDlg dlg(m_ServerEntry.m_EntryName, AfxGetMainWnd());

	dlg.m_pEntry    = &m_ServerEntry;
	dlg.m_pTextRam  = &(m_TextRam);
	dlg.m_pKeyTab   = &(m_KeyTab);
	dlg.m_pKeyMac   = &(m_KeyMac);
	dlg.m_pParamTab = &(m_ParamTab);
	dlg.m_pDocument = this;

	if ( dlg.DoModal() != IDOK )
		return;

	if ( (dlg.m_ModFlag & (UMOD_ANSIOPT | UMOD_MODKEY | UMOD_COLTAB | UMOD_BANKTAB | UMOD_DEFATT | UMOD_CARET)) != 0 )
		dlg.m_ModFlag = m_TextRam.InitDefParam(TRUE, dlg.m_ModFlag);

	if ( dlg.m_ModFlag != 0 || dlg.m_bModified ) {
		SetModifiedFlag(TRUE);
		if ( m_pSock != NULL )
			m_pSock->ResetOption();
	}

	if ( m_pLogFile != NULL && LogMode != m_TextRam.m_LogMode ) {
		int save = m_TextRam.m_LogMode;
		CString FilePath = m_pLogFile->GetFilePath();

		m_TextRam.m_LogMode = LogMode;
		LogClose();
		m_TextRam.m_LogMode = save;
		LogOpen(FilePath);
	}

	UpdateAllViews(NULL, (dlg.m_ModFlag & UMOD_RESIZE) != 0 ? UPDATE_RESIZE : UPDATE_INITPARA, NULL);
}

void CRLoginDoc::OnSftp()
{
	if ( m_pSock != NULL && m_pSock->m_bConnect && m_pSock->m_Type == ESCT_SSH_MAIN && ((Cssh *)(m_pSock))->m_SSHVer == 2 )
		((Cssh *)(m_pSock))->OpenSFtpChannel();
}
void CRLoginDoc::OnUpdateSftp(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pSock != NULL && m_pSock->m_bConnect && m_pSock->m_Type == ESCT_SSH_MAIN && ((Cssh *)(m_pSock))->m_SSHVer == 2);
}

void CRLoginDoc::OnKanjiCodeSet(UINT nID)
{
	m_TextRam.SetKanjiMode(nID - IDM_KANJI_EUC);
}
void CRLoginDoc::OnUpdateKanjiCodeSet(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck((int)(pCmdUI->m_nID - IDM_KANJI_EUC) == m_TextRam.m_KanjiMode ? 1 : 0);
}

void CRLoginDoc::OnXYZModem(UINT nID)
{
	if ( m_pSock == NULL || !m_pSock->m_bConnect )
		return;

	if ( nID == IDM_KERMIT_UPLOAD || nID == IDM_KERMIT_DOWNLOAD || nID == IDM_SIMPLE_UPLOAD || nID == IDM_SIMPLE_DOWNLOAD ) {
		if ( m_pKermit == NULL )
			m_pKermit = new CKermit(this, AfxGetMainWnd());
	} else {
		if ( m_pZModem == NULL )
			m_pZModem = new CZModem(this, AfxGetMainWnd());
	}

	switch(nID) {
	case IDM_XMODEM_UPLOAD:   m_pZModem->DoProc(1); break;
	case IDM_YMODEM_UPLOAD:   m_pZModem->DoProc(2); break;
	case IDM_ZMODEM_UPLOAD:   m_pZModem->DoProc(3); break;
	case IDM_XMODEM_DOWNLOAD: m_pZModem->DoProc(4); break;
	case IDM_YMODEM_DOWNLOAD: m_pZModem->DoProc(5); break;
	case IDM_ZMODEM_DOWNLOAD: m_pZModem->DoProc(6); break;
	case IDM_KERMIT_UPLOAD:   m_pKermit->DoProc(1); break;
	case IDM_KERMIT_DOWNLOAD: m_pKermit->DoProc(0); break;
	case IDM_SIMPLE_UPLOAD:	  m_pKermit->DoProc(3); break;
	case IDM_SIMPLE_DOWNLOAD: m_pKermit->DoProc(4); break;
	}
}
void CRLoginDoc::OnUpdateXYZModem(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pSock != NULL && m_pSock->m_bConnect ? TRUE : FALSE);
}

void CRLoginDoc::OnChatStop()
{
	if ( m_pStrScript != NULL ) {
		if ( m_pStrScript->m_MakeFlag )
			SetModifiedFlag(TRUE);
		m_pStrScript->ExecStop();
		m_pStrScript = NULL;
	} else if ( m_pScript != NULL )
		m_pScript->OpenDebug();
	SetDocTitle();
}
void CRLoginDoc::OnUpdateChatStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pStrScript != NULL && m_pStrScript->Status() != SCPSTAT_NON ? TRUE : (m_pScript != NULL && m_pScript->m_CodePos != (-1) ? TRUE : FALSE));
}

void CRLoginDoc::OnSendBreak()
{
	if ( m_pSock != NULL ) // && m_pSock->m_Type == ESCT_COMDEV )
		m_pSock->SendBreak(1);
}
void CRLoginDoc::OnUpdateSendBreak(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pSock != NULL && m_pSock->m_bConnect && 
		(m_pSock->m_Type == ESCT_COMDEV || m_pSock->m_Type == ESCT_SSH_MAIN) ? TRUE : FALSE);
}
void CRLoginDoc::OnScriptMenu(UINT nID)
{
	if ( m_pScript != NULL )
		m_pScript->Call(m_pScript->m_MenuTab[nID - IDM_SCRIPT_MENU1].func);
}

void CRLoginDoc::OnTekdisp()
{
	if ( m_TextRam.m_pTekWnd == NULL )
		m_TextRam.TekInit(0);
	else if ( !m_TextRam.m_pTekWnd->IsWindowVisible() )
		m_TextRam.TekInit(0);
	else
		m_TextRam.TekClose();
}
void CRLoginDoc::OnUpdateTekdisp(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_TextRam.m_pTekWnd != NULL && m_TextRam.m_pTekWnd->IsWindowVisible() ? TRUE : FALSE);
}
void CRLoginDoc::OnImagedisp()
{
	CRLoginView *pView;

	if ( (pView = (CRLoginView *)GetAciveView()) == NULL )
		return;

	if ( m_TextRam.m_pImageWnd == NULL )
		pView->CreateGrapImage(0);
	else if ( m_TextRam.m_pImageWnd->IsWindowVisible() )
		pView->CreateGrapImage(0);
	else
		m_TextRam.m_pImageWnd->ShowWindow(SW_SHOW);
}
void CRLoginDoc::OnUpdateImagedisp(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_TextRam.m_pImageWnd != NULL ? TRUE : FALSE);
}
void CRLoginDoc::OnScreenReset(UINT nID)
{
	int mode = 0;

	switch(nID) {
	case IDM_RESET_TAB:   mode = RESET_TABS; break;
	case IDM_RESET_BANK:  mode = RESET_BANK; break;
	case IDM_RESET_ATTR:  mode = RESET_ATTR | RESET_COLOR; break;
	case IDM_RESET_TEK:   mode = RESET_TEK; break;
	case IDM_RESET_ESC:   mode = RESET_BANK | RESET_CHAR | RESET_OPTION | RESET_XTOPT | RESET_MODKEY | RESET_TRANSMIT; break;
	case IDM_RESET_MOUSE: mode = RESET_MOUSE; break;
	case IDM_RESET_SCREEN:mode = RESET_PAGE | RESET_CURSOR | RESET_CARET | RESET_MARGIN | RESET_RLMARGIN | RESET_BANK | RESET_ATTR |
								 RESET_COLOR | RESET_CHAR | RESET_CLS | RESET_XTOPT | RESET_OPTION | RESET_STATUS; break;
	case IDM_RESET_SIZE:  mode = RESET_SIZE; break;
	case IDM_RESET_ALL:   mode = RESET_ALL | RESET_CLS | RESET_HISTORY | RESET_TRANSMIT; break;
	}

	m_TextRam.RESET(mode);
	m_TextRam.FLUSH();

	if ( (mode & RESET_TRANSMIT) != 0 ) {
		if ( m_pBPlus != NULL )
			m_pBPlus->DoAbort();
		if ( m_pZModem != NULL )
			m_pZModem->DoAbort();
		if ( m_pKermit != NULL )
			m_pKermit->DoAbort();
	}
}

void CRLoginDoc::OnSocketstatus()
{
	if ( m_pSock == NULL )
		return;

	if ( m_pStatusWnd == NULL ) {
		CString status;
		m_pSock->GetStatus(status);
		m_pStatusWnd = new CStatusDlg;
		m_pStatusWnd->m_OwnerType = 2;
		m_pStatusWnd->m_pValue = this;
		m_pStatusWnd->m_Title = m_ServerEntry.m_EntryName;
		m_pStatusWnd->Create(IDD_STATUS_DLG, CWnd::GetDesktopWindow());
		m_pStatusWnd->ShowWindow(SW_SHOW);
		m_pStatusWnd->SetStatusText(status);
	} else
		m_pStatusWnd->DestroyWindow();
}
void CRLoginDoc::OnUpdateSocketstatus(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pSock != NULL ? TRUE : FALSE);
	pCmdUI->SetCheck(m_pStatusWnd != NULL ? TRUE : FALSE);
}

void CRLoginDoc::OnScript()
{
	if ( m_pScript == NULL )
		return;

	CFileDialog dlg(FALSE, _T("txt"), _T(""), 0, CStringLoad(IDS_FILEDLGSCRIPT), AfxGetMainWnd());

	if ( dlg.DoModal() != IDOK )
		return;

	if ( m_pScript->m_Code.GetSize() > 0 && AfxMessageBox(IDS_SCRIPTNEW, MB_ICONQUESTION | MB_YESNO) == IDYES ) {
		if ( m_pScript != NULL )
			delete m_pScript;
		m_pScript = new CScript;
		m_pScript->SetDocument(this);
	}

	m_pScript->ExecFile(dlg.GetPathName());
	m_pScript->m_bInit = TRUE;
}
void CRLoginDoc::OnUpdateScript(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pScript != NULL && m_pScript->IsExec() ? FALSE : TRUE);
}
void CRLoginDoc::OnCancelBtn()
{
	m_TextRam.fc_TimerAbort(FALSE);
}
void CRLoginDoc::OnTracedisp()
{
	if ( m_TextRam.m_pTraceWnd == NULL ) {
		m_TextRam.SetTraceLog(TRUE);
		m_TextRam.m_pTraceWnd = new CTraceDlg(NULL);
		m_TextRam.m_pTraceWnd->m_pDocument = this;
		m_TextRam.m_pTraceWnd->m_Title = GetTitle();
		m_TextRam.m_pTraceWnd->m_TraceLogFile  = m_TextRam.m_TraceLogFile;
		EntryText(m_TextRam.m_pTraceWnd->m_TraceLogFile);
		m_TextRam.m_pTraceWnd->m_TraceMaxCount = m_TextRam.m_TraceMaxCount;
		m_TextRam.m_pTraceWnd->Create(IDD_TRACEDLG, CWnd::GetDesktopWindow());
		m_TextRam.m_pTraceWnd->ShowWindow(SW_SHOW);
//		::AfxGetMainWnd()->SetFocus();
	} else
		m_TextRam.m_pTraceWnd->SendMessage(WM_CLOSE);
}
void CRLoginDoc::OnUpdateTracedisp(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_TextRam.m_pTraceWnd != NULL ? TRUE : FALSE);
}

/////////////////////////////////////////////////////////////////////////////

static const ScriptCmdsDefs DocBase[] = {
	{	"Entry",		1	},
	{	"Screen",		2	},
	{	"KeyCode",		3	},
	{	"KeyMacro",		4	},
	{	"ssh",			5	},
	{	"Title",		6	},
	{	"Command",		7	},
	{	"Log",			8	},
	{	"Open",			9	},
	{	"Close",		10	},
	{	"Destroy",		11	},
	{	"DoOpen",		12	},
	{	"Connect",		13	},
	{	"Abort",		14	},
	{	"CmdLine",		15	},
	{	NULL,			0	},
}, DocLog[] = {
	{	"File",			20	},
	{	"Mode",			21	},
	{	"Code",			22	},
	{	"Open",			23	},
	{	"Close",		24	},
	{	NULL,			0	},
};

void CRLoginDoc::ScriptInit(int cmds, int shift, class CScriptValue &value)
{
	value.m_DocCmds = cmds;

	m_ServerEntry.ScriptInit((DocBase[0].cmds << shift) | cmds, shift + 6, value[DocBase[0].name]);
	m_TextRam.ScriptInit    ((DocBase[1].cmds << shift) | cmds, shift + 6, value[DocBase[1].name]);
	m_KeyTab.ScriptInit     ((DocBase[2].cmds << shift) | cmds, shift + 6, value[DocBase[2].name]);
	m_KeyMac.ScriptInit     ((DocBase[3].cmds << shift) | cmds, shift + 6, value[DocBase[3].name]);
	m_ParamTab.ScriptInit   ((DocBase[4].cmds << shift) | cmds, shift + 6, value[DocBase[4].name]);

	for ( int n = 5 ; DocBase[n].name != NULL ; n++ )
		value[DocBase[n].name].m_DocCmds = (DocBase[n].cmds << shift) | cmds;

	for ( int n = 0 ; DocLog[n].name != NULL ; n++ )
		value["Log"][DocLog[n].name].m_DocCmds = (DocLog[n].cmds << shift) | cmds;
}
void CRLoginDoc::ScriptValue(int cmds, class CScriptValue &value, int mode)
{
	int n, i;
	CString str;

	switch(cmds & 0x3F) {
	case 0:				// Document
		if ( mode == DOC_MODE_SAVE ) {
			for ( n = 0 ; DocBase[n].name != NULL ; n++ ) {
				if ( (i = value.Find(DocBase[n].name)) >= 0 )
					ScriptValue(DocBase[n].cmds, value[i], mode);
			}
		} else if ( mode == DOC_MODE_IDENT ) {
			for ( n = 0 ; DocBase[n].name != NULL ; n++ )
					ScriptValue(DocBase[n].cmds, value[DocBase[n].name], mode);
		}
		break;

	case 1:				// Document.Entry
		m_ServerEntry.ScriptValue(cmds >> 6, value, mode);
		break;
	case 2:				// Document.Screen
		m_TextRam.ScriptValue(cmds >> 6, value, mode);
		break;
	case 3:				// Document.KeyCode
		m_KeyTab.ScriptValue(cmds >> 6, value, mode);
		break;
	case 4:				// Document.KeyMacro
		m_KeyMac.ScriptValue(cmds >> 6, value, mode);
		break;
	case 5:				// Document.ssh
		m_ParamTab.ScriptValue(cmds >> 6, value, mode);
		break;

	case 6:				// Document.Title
		str = GetTitle();
		value.SetStr(str, mode);
		if ( mode == DOC_MODE_SAVE )
			SetTitle(str);
		break;

	case 7:				// Document.Commmand(n)
		if ( mode == DOC_MODE_CALL ) {
			CWnd *pWnd = GetAciveView();
			if ( (n = (int)value[0]) <= 0 )
				n = m_KeyTab.GetCmdsKey((LPCWSTR)value[0]);
			if ( pWnd != NULL && n > 0 )
				pWnd->PostMessage(WM_COMMAND, (WPARAM)n);
		}
		break;

	case 8:				// Document.Log
		if ( mode == DOC_MODE_SAVE ) {
			for ( n = 0 ; DocLog[n].name != NULL ; n++ ) {
				if ( (i = value.Find(DocLog[n].name)) >= 0 )
					ScriptValue(DocLog[n].cmds, value[i], mode);
			}
		} else if ( mode == DOC_MODE_IDENT ) {
			for ( n = 0 ; DocLog[n].name != NULL ; n++ )
					ScriptValue(DocLog[n].cmds, value[DocLog[n].name], mode);
			value = (int)(m_pLogFile == NULL ? 0 : 1);
		}
		break;

	case 9:				// Document.Open()
		if ( mode == DOC_MODE_CALL )
			SocketOpen();
		break;
	case 10:			// Document.Close()
		if ( mode == DOC_MODE_CALL )
			SocketClose();
		break;
	case 11:			// Document.Destroy()
		if ( mode == DOC_MODE_CALL ) {
			CWnd *pWnd = GetAciveView();
			if ( pWnd != NULL )
				pWnd->PostMessage(WM_COMMAND, ID_FILE_CLOSE, 0 );
		}
		break;
	case 12:			// Document.DoOpen
		value.SetInt(m_pScript->m_bOpenSock, mode);
		break;
	case 13:			// Document.Connect
		n = (m_pSock == NULL ? FALSE : m_pSock->m_bConnect);
		value.SetInt(n, mode);
		break;
	case 14:			// Document.Abort
		value.SetInt(m_pScript->m_bAbort, mode);
		break;
	case 15:			// Document.CmdLine
		value.SetStr(m_CmdLine, mode);
		break;

	case 20:				// Document.Log.File
		if ( mode == DOC_MODE_IDENT ) {
			if ( m_pLogFile != NULL )
				value = (LPCTSTR)m_pLogFile->GetFilePath();
			else
				value = (int)0;
		}
		break;
	case 21:				// Document.Log.Mode
		value.SetInt(m_TextRam.m_LogMode, mode);
		break;
	case 22:				// Document.Log.Code
		n = m_TextRam.IsOptValue(TO_RLLOGCODE, 2);
		value.SetInt(n, mode);
		if ( m_pLogFile == NULL )
			m_TextRam.SetOptValue(TO_RLLOGCODE, 2, n);
		break;
	case 23:				// Document.Log.Open(f)
		if ( mode == DOC_MODE_CALL ) {
			LogClose();
			value = (LogOpen((LPCTSTR)value[0]) ? (int)0 : (int)1);
		}
		break;
	case 24:				// Document.Log.Close()
		if ( mode == DOC_MODE_CALL )
			LogClose();
		break;
	}
}
void CRLoginDoc::OnUpdateResetSize(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_TextRam.m_bReSize);
}
void CRLoginDoc::OnTitleedit()
{
	CEditDlg dlg;

	dlg.m_WinText = _T("Title Edit");
	dlg.m_Edit = GetTitle();
	dlg.m_Title.LoadString(IDS_TITLEDITMSG);

	if ( dlg.DoModal() != IDOK )
		return;

	m_TitleName = dlg.m_Edit;
	SetTitle(m_TitleName);
}
void CRLoginDoc::OnCommoniter()
{
	if ( m_pSock != NULL && m_pSock->m_Type == ESCT_COMDEV )
		((CComSock *)m_pSock)->ComMoniter();
}
void CRLoginDoc::OnUpdateCommoniter(CCmdUI *pCmdUI)
{
	if ( m_pSock != NULL && m_pSock->m_Type == ESCT_COMDEV ) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(((CComSock *)m_pSock)->m_pComMoni != NULL ? TRUE : FALSE);
	} else {
		pCmdUI->Enable(FALSE);
		pCmdUI->SetCheck(FALSE);
	}
}
