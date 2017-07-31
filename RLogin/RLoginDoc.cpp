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
	ON_COMMAND(ID_LOG_OPEN, OnLogOpen)
	ON_UPDATE_COMMAND_UI(ID_LOG_OPEN, OnUpdateLogOpen)
	ON_COMMAND(IDC_LOADDEFAULT, OnLoadDefault)
	ON_COMMAND(IDC_SAVEDEFAULT, OnSaveDefault)
	ON_COMMAND(ID_SETOPTION, OnSetOption)
	ON_COMMAND(IDM_SFTP, OnSftp)
	ON_UPDATE_COMMAND_UI(IDM_SFTP, OnUpdateSftp)
	ON_COMMAND(ID_CHARSCRIPT_END, &CRLoginDoc::OnChatStop)
	ON_UPDATE_COMMAND_UI(ID_CHARSCRIPT_END, &CRLoginDoc::OnUpdateChatStop)
	ON_COMMAND_RANGE(IDM_KANJI_EUC, IDM_KANJI_UTF8, OnKanjiCodeSet)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_KANJI_EUC, IDM_KANJI_UTF8, OnUpdateKanjiCodeSet)
	ON_COMMAND_RANGE(IDM_XMODEM_UPLOAD, IDM_KERMIT_DOWNLOAD, OnXYZModem)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_XMODEM_UPLOAD, IDM_KERMIT_DOWNLOAD, OnUpdateXYZModem)
	ON_COMMAND(ID_SEND_BREAK, &CRLoginDoc::OnSendBreak)
	ON_UPDATE_COMMAND_UI(ID_SEND_BREAK, &CRLoginDoc::OnUpdateSendBreak)
	ON_COMMAND(IDM_TEKDISP, &CRLoginDoc::OnTekdisp)
	ON_UPDATE_COMMAND_UI(IDM_TEKDISP, &CRLoginDoc::OnUpdateTekdisp)
	ON_COMMAND_RANGE(IDM_RESET_TAB, IDM_RESET_ALL, &CRLoginDoc::OnScreenReset)
	ON_COMMAND(IDM_SOCKETSTATUS, &CRLoginDoc::OnSocketstatus)
	ON_UPDATE_COMMAND_UI(IDM_SOCKETSTATUS, &CRLoginDoc::OnUpdateSocketstatus)
	ON_COMMAND(IDM_SCRIPT, &CRLoginDoc::OnScript)
	ON_UPDATE_COMMAND_UI(IDM_SCRIPT, &CRLoginDoc::OnUpdateScript)
	ON_COMMAND_RANGE(IDM_SCRIPT_MENU1, IDM_SCRIPT_MENU10, OnScriptMenu)
	ON_COMMAND(IDM_IMAGEDISP, &CRLoginDoc::OnImagedisp)
	ON_UPDATE_COMMAND_UI(IDM_IMAGEDISP, &CRLoginDoc::OnUpdateImagedisp)
	ON_COMMAND(IDC_CANCELBTN, OnCancelBtn)
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
	m_DelayFlag = DELAY_NON;
	m_ActCharCount = 0;
	m_pMainWnd = (CMainFrame *)AfxGetMainWnd();
	m_TitleString.Empty();
	m_pStrScript = NULL;
	m_pScript = NULL;
	m_InPane = FALSE;
}

CRLoginDoc::~CRLoginDoc()
{
	if ( m_pLogFile != NULL ) {
		m_pLogFile->Close();
		delete m_pLogFile;
	}

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

BOOL CRLoginDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	if ( !m_pMainWnd->OpenServerEntry(m_ServerEntry) )
		return FALSE;

	switch(m_ServerEntry.m_SaveFlag) {
	case (-1):
		m_LoadMode = DOCTYPE_SESSION;
		break;
	case FALSE:
		m_LoadMode = DOCTYPE_MULTIFILE;
		break;
	case TRUE:
		m_LoadMode = DOCTYPE_REGISTORY;
		break;
	}
	m_ServerEntry.m_SaveFlag = FALSE;

	m_TextRam.m_bOpen = TRUE;
	m_TextRam.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_KeyTab.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_KeyMac.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_ParamTab.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_TextRam.SetKanjiMode(m_ServerEntry.m_KanjiCode);

	CRLoginView *pView;
	if ( (pView = (CRLoginView *)GetAciveView()) != NULL )
		pView->SetFrameRect(-1, -1);
	UpdateAllViews(NULL, UPDATE_INITSIZE, 0);
	m_TextRam.InitHistory();

	SetTitle(m_ServerEntry.m_EntryName);

	if ( !m_ServerEntry.m_ScriptStr.IsEmpty() )
		m_pScript->ExecStr(m_ServerEntry.m_ScriptStr);
	if ( !m_ServerEntry.m_ScriptFile.IsEmpty() )
		m_pScript->ExecFile(m_ServerEntry.m_ScriptFile);
	if ( m_TextRam.IsOptEnable(TO_RLSCRDEBUG) )
		m_pScript->OpenDebug(m_ServerEntry.m_EntryName);

	SetCmdInfo(((CRLoginApp *)AfxGetApp())->m_pCmdInfo);

	if ( !m_pScript->m_bOpenSock && !SocketOpen() )
		return FALSE;

	return TRUE;
}

BOOL CRLoginDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	m_LoadMode = DOCTYPE_NONE;

	if ( !CDocument::OnOpenDocument(lpszPathName) )
		return FALSE;

	m_TextRam.m_bOpen = TRUE;

	if ( m_LoadMode == DOCTYPE_ENTRYFILE ) {
		UpdateAllViews(NULL, UPDATE_INITPARA, 0);
		m_TextRam.InitHistory();
		SetPathName(lpszPathName, TRUE);

		if ( !m_ServerEntry.m_ScriptStr.IsEmpty() )
			m_pScript->ExecStr(m_ServerEntry.m_ScriptStr);
		if ( !m_ServerEntry.m_ScriptFile.IsEmpty() )
			m_pScript->ExecFile(m_ServerEntry.m_ScriptFile);
		if ( m_TextRam.IsOptEnable(TO_RLSCRDEBUG) )
			m_pScript->OpenDebug(m_ServerEntry.m_EntryName);

		SetCmdInfo(((CRLoginApp *)AfxGetApp())->m_pCmdInfo);

		if ( !m_pScript->m_bOpenSock && !SocketOpen() )
			return FALSE;
	}
	
	return (m_LoadMode == DOCTYPE_NONE ? FALSE : TRUE);
}

void CRLoginDoc::OnFileClose()
{
	if ( m_pSock != NULL && m_pSock->m_bConnect && AfxMessageBox(CStringLoad(IDS_FILECLOSEQES), MB_ICONQUESTION | MB_YESNO) != IDYES )
		return;

	if ( m_InPane )
		::AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_PANE_DELETE);

	CDocument::OnFileClose();
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc シリアライゼーション

void CRLoginDoc::SetIndex(int mode, CStringIndex &index)
{
	// mode == TRUE		CRLoginDoc -> CStringIndex
	//         FALSE	CRLoginDoc <- CStringIndex

	m_ServerEntry.SetIndex(mode, index[_T("Entry")]);
	m_ParamTab.SetIndex(mode, index[_T("Protocol")]);
	m_TextRam.SetIndex(mode, index[_T("Screen")]);
	m_TextRam.m_FontTab.SetIndex(mode, index[_T("Fontset")]);
	m_KeyTab.SetIndex(mode, index[_T("Keycode")]);
	m_KeyMac.SetIndex(mode, index[_T("Keymacro")]);
}

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
		m_LoadMode = DOCTYPE_ENTRYFILE;

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
			m_LoadMode = DOCTYPE_ENTRYFILE;

		} else if ( strncmp(tmp, "RLG2", 4) == 0 ) {
			m_ServerEntry.Serialize(ar);
			m_TextRam.Serialize(ar);
			m_KeyTab.Serialize(ar);
			m_KeyMac.Serialize(ar);
			m_ParamTab.Serialize(ar);

			m_TextRam.SetKanjiMode(m_ServerEntry.m_KanjiCode);
			m_LoadMode = DOCTYPE_ENTRYFILE;

		} else if ( strncmp(tmp, "RLM", 3) == 0 ) {
			m_pMainWnd->m_AllFilePath = ar.GetFile()->GetFilePath();
			m_pMainWnd->m_AllFileBuf.Clear();
			while ( (n = ar.Read(tmp, 4096)) > 0 )
				m_pMainWnd->m_AllFileBuf.Apend((LPBYTE)tmp, n);
			m_pMainWnd->PostMessage(WM_COMMAND, ID_FILE_ALL_LOAD, 0);
			m_LoadMode = DOCTYPE_MULTIFILE;

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
	if ( pCmdInfo == NULL )
		return;

	if ( pCmdInfo->m_Proto != (-1) )
		m_ServerEntry.m_ProtoType = pCmdInfo->m_Proto;

	if ( !pCmdInfo->m_Addr.IsEmpty() )
		m_ServerEntry.m_HostNameProvs = m_ServerEntry.m_HostName = pCmdInfo->m_Addr;

	if ( !pCmdInfo->m_Port.IsEmpty() )
		m_ServerEntry.m_PortName = pCmdInfo->m_Port;

	if ( !pCmdInfo->m_User.IsEmpty() )
		m_ServerEntry.m_UserNameProvs = m_ServerEntry.m_UserName = pCmdInfo->m_User;

	if ( !pCmdInfo->m_Pass.IsEmpty() )
		m_ServerEntry.m_PassNameProvs = m_ServerEntry.m_PassName = pCmdInfo->m_Pass;

	if ( !pCmdInfo->m_Term.IsEmpty() )
		m_ServerEntry.m_TermName = pCmdInfo->m_Term;

	m_InPane = pCmdInfo->m_InPane;
}
void CRLoginDoc::SetEntryProBuffer()
{
	m_ServerEntry.m_ProBuffer.Clear();
	m_TextRam.Serialize(TRUE, m_ServerEntry.m_ProBuffer);
	m_KeyTab.Serialize(TRUE, m_ServerEntry.m_ProBuffer);
	m_KeyMac.Serialize(TRUE, m_ServerEntry.m_ProBuffer);
	m_ParamTab.Serialize(TRUE, m_ServerEntry.m_ProBuffer);
}
void CRLoginDoc::DeleteContents() 
{
	SocketClose();

	if ( m_ServerEntry.m_SaveFlag ) {
		SetEntryProBuffer();
		m_pMainWnd->m_ServerEntryTab.AddEntry(m_ServerEntry);
		m_ServerEntry.m_SaveFlag = FALSE;
	}

	if ( m_pScript != NULL ) {
		m_pScript->Stop();
		delete m_pScript;
	}

	m_pScript = new CScript;
	m_pScript->SetDocument(this);

	m_ServerEntry.Init();
	m_TextRam.m_bOpen = FALSE;
	m_InPane = FALSE;

	CDocument::DeleteContents();
}
void CRLoginDoc::SetStatus(LPCTSTR str)
{
	int n;
	CString tmp;
	CString title;

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

	if ( str != NULL && *str != _T('\0') ) {
		((CMainFrame *)AfxGetMainWnd())->SetMessageText(str);
		m_TitleString = str;
	}

	if ( !m_TitleString.IsEmpty() && (m_TextRam.m_TitleMode & WTTL_ALGO) != 0 ) {
		tmp.Format(_T("(%s)"), m_TitleString);
		title += tmp;
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
BOOL CRLoginDoc::EntryText(CString &name)
{
	CEditDlg dlg;
	CTime tm = CTime::GetCurrentTime();
	CString tmp;
	LPCTSTR str = name;
	BOOL st = FALSE;

	while ( *str != _T('\0') ) {
		if ( *str == _T('%') ) {
			switch(str[1]) {
			case _T('E'):
				tmp += m_ServerEntry.m_EntryName;
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
				dlg.m_WinText = _T("FileName");
				dlg.m_Title = m_ServerEntry.m_EntryName;
				dlg.m_Edit  = tmp;
				tmp.Empty();
				if ( dlg.DoModal() == IDOK )
					tmp = dlg.m_Edit;
				st = TRUE;
				break;
			case _T('%'):
				tmp += _T('%');
				break;
			default:
				tmp += str[0];
				tmp += str[1];
				break;
			}
			str += 2;
		} else
			tmp += *(str++);
	}

	if ( st )
		name = tmp;

	return st;
}
void CRLoginDoc::SendScript(LPCWSTR str, LPCWSTR match)
{
	int n;
	WCHAR c;
	CEditDlg dlg;
	CTime tm = CTime::GetCurrentTime();
	CBuffer buf;
	CStringW tmp;

	while ( *str != L'\0' ) {
		if ( *str == L'%' ) {
			switch(str[1]) {
			case L'E':
				tmp += m_ServerEntry.m_EntryName;
				break;
			case L'U':
				tmp += m_ServerEntry.m_UserName;
				break;
			case L'P':
				tmp += m_ServerEntry.m_PassName;
				break;
			case L'T':
				tmp += m_ServerEntry.m_TermName;
				break;
			case L'S':
				tmp += m_ServerEntry.m_HostName;
				break;
			case L'p':
				tmp += m_ServerEntry.m_PortName;
				break;
			case L'D':
				tmp += tm.Format(_T("%y%m%d"));
				break;
			case L't':
				tmp += tm.Format(_T("%H%M%S"));
				break;
			case L'I':
				dlg.m_WinText = _T("ChatScript");
				if ( match != NULL ) dlg.m_Title = match;
				dlg.m_Edit  = tmp;
				tmp.Empty();
				if ( dlg.DoModal() == IDOK )
					tmp = dlg.m_Edit;
				break;
			case L'%':
				tmp += L'%';
				break;
			}
			str += 2;
		} else if ( *str == L'\\' ) {
			switch(str[1]) {
			case L'a': tmp += L'\x07'; str += 2; break;
			case L'b': tmp += L'\x08'; str += 2; break;
			case L't': tmp += L'\x09'; str += 2; break;
			case L'n': tmp += L'\x0A'; str += 2; break;
			case L'v': tmp += L'\x0B'; str += 2; break;
			case L'f': tmp += L'\x0C'; str += 2; break;
			case L'r': tmp += L'\x0D'; str += 2; break;
			case L'\\': tmp += L'\\'; str += 2; break;

			case L'x': case L'X':
				str += 2;
				for ( n = c = 0 ; n < 2 ; n++ ) {
					if ( *str >= L'0' && *str <= L'9' )
						c = c * 16 + (*(str++) - L'0');
					else if ( *str >= L'A' && *str <= L'F' )
						c = c * 16 + (*(str++) - L'A' + 10);
					else if ( *str >= L'a' && *str <= L'f' )
						c = c * 16 + (*(str++) - L'a' + 10);
					else
						break;
				}
				tmp += c;
				break;

			case L'0': case L'1': case L'2': case L'3':
			case L'4': case L'5': case L'6': case L'7':
				str += 1;
				for ( n = c = 0 ; n < 3 ; n++ ) {
					if ( *str >= L'0' && *str <= L'7' )
						c = c * 8 + (*(str++) - L'0');
						break;
				}
				tmp += c;
				break;

			default: str += 1; break;
			}
		} else
			tmp += *(str++);
	}

	if ( tmp.IsEmpty() )
		return;

	buf.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
	SendBuffer(buf);
}
void CRLoginDoc::OnReciveChar(DWORD ch, int pos)
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
			SetStatus(NULL);
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

int CRLoginDoc::DelaySend()
{
	int n = 0;

	if ( m_pSock == NULL ) {
		m_DelayBuf.Clear();
		return FALSE;
	}

	while ( n < m_DelayBuf.GetSize() ) {
		if ( m_DelayBuf.GetAt(n++) == '\r' ) {
			m_pSock->Send(m_DelayBuf.GetPtr(), n, 0);
			m_DelayBuf.Consume(n);
			m_DelayFlag = DELAY_WAIT;
			m_pMainWnd->SetTimerEvent(DELAY_ECHO_MSEC, TIMEREVENT_DOC, this);
			return TRUE;
		}
	}
	if ( n > 0 ) {
		m_pSock->Send(m_DelayBuf.GetPtr(), m_DelayBuf.GetSize(), 0);
		m_DelayBuf.Clear();
	}

	return FALSE;
}
void CRLoginDoc::OnDelayRecive(int ch)
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

	UpdateAllViews(NULL, UPDATE_GOTOXY, NULL);

	if ( m_pScript != NULL )
		m_pScript->OnConnect();

	m_ServerEntry.m_ChatScript.ExecInit();
	if ( m_ServerEntry.m_ChatScript.Status() != SCPSTAT_NON ) {
		m_pStrScript = &(m_ServerEntry.m_ChatScript);
		OnReciveChar(0, (-1));
	}

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

		if ( m_pLogFile != NULL ) {
			m_pLogFile->Close();
			delete m_pLogFile;
			m_pLogFile = NULL;
		}

		if ( (m_pLogFile = new CFileExt) == NULL )
			return;

		file.Format(_T("%s%s%s"), dirs, name, exts);

		for ( num = 1 ; num < 20 ; num++ ) {
			if ( m_pLogFile->Open(file, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive) ) {
				m_pLogFile->SeekToEnd();
				break;
			}
			file.Format(_T("%s%s-%d%s"), dirs, name, num, exts);
		}
		if ( num >= 20 ) {
			file.Format(_T("LogFile Open Error '%s%s%s'"), dirs, name, exts);
			AfxMessageBox(file);
			delete m_pLogFile;
			m_pLogFile = NULL;
		}

		m_TextRam.m_LogTimeFlag = TRUE;
	}

	SetStatus(_T("Connect"));
}
void CRLoginDoc::OnSocketError(int err)
{
//	SocketClose();
	SetStatus(_T("Error"));
	CString tmp;
	if ( m_ErrorPrompt.IsEmpty() )
		m_ErrorPrompt.Format(_T("WinSock Have Error #%d"), err);
	tmp.Format(_T("%s Server Entry Scoket Error\n%s:%s Connection\n%s"),
		m_ServerEntry.m_EntryName, m_ServerEntry.m_HostName, m_ServerEntry.m_PortName, m_ErrorPrompt);
	AfxMessageBox(tmp);
	m_ErrorPrompt.Empty();
//	m_pMainWnd->PostMessage(WM_COMMAND, ID_FILE_CLOSE, 0 );
	OnFileClose();
}
void CRLoginDoc::OnSocketClose()
{
	if ( m_pSock == NULL )
		return;

	if ( m_pScript != NULL )
		m_pScript->OnClose();

	if ( m_pBPlus != NULL )
		m_pBPlus->DoAbort();
	if ( m_pZModem != NULL )
		m_pZModem->DoAbort();
	if ( m_pKermit != NULL )
		m_pKermit->DoAbort();

	BOOL bCanExit = FALSE;

	if ( m_TextRam.IsOptEnable(TO_RLPOFF) ) {
		bCanExit = TRUE;
		CRLoginDoc *pDoc;
		CDocTemplate *pDocTemp = GetDocTemplate();
		ASSERT(pDocTemp);
		POSITION pos = pDocTemp->GetFirstDocPosition();

		while ( pos != NULL ) {
			pDoc = (CRLoginDoc *)(pDocTemp->GetNextDoc(pos));
			ASSERT(pDoc);
			if ( pDoc != this && pDoc->m_pSock != NULL )
				bCanExit = FALSE;
		}
	}

	UpdateAllViews(NULL, UPDATE_GOTOXY, NULL);
	SetStatus(_T("Close"));

//	m_pMainWnd->PostMessage(WM_COMMAND, (bCanExit ? ID_APP_EXIT : ID_FILE_CLOSE), 0 );
	if ( bCanExit )
		m_pMainWnd->PostMessage(WM_COMMAND, ID_APP_EXIT, 0 );
	else
		OnFileClose();
}
int CRLoginDoc::OnSocketRecive(LPBYTE lpBuf, int nBufLen, int nFlags)
{
	int n;
	BOOL sync = FALSE;

//	TRACE("OnSocketRecive %d(%d)Byte\n", nBufLen, m_ActCharCount);

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

	if ( IsActCount() ) {
		if ( !m_pMainWnd->IsIconic() )
			return 0;
		ClearActCount();
	}

	if ( nBufLen > 4096 )
		nBufLen = 4096;

	n = m_TextRam.Write(lpBuf, nBufLen, &sync);

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
	nBufLen = n;

	if ( m_pLogFile != NULL && m_TextRam.IsOptValue(TO_RLLOGMODE, 2) == LOGMOD_RAW )
		m_pLogFile->Write(lpBuf, nBufLen);

	m_pMainWnd->WakeUpSleep();

	if ( m_pScript != NULL && m_pScript->m_SockMode == DATA_BUF_BOTH )
		m_pScript->SetSockBuff(lpBuf, nBufLen);

	return nBufLen;
}

int CRLoginDoc::SocketOpen()
{
	BOOL rt;
	int num;
	CPassDlg dlg;
	CStringArrayExt hosts;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	if ( InternetAttemptConnect(0) != ERROR_SUCCESS )
		return FALSE;

	num = CExtSocket::GetPortNum(m_ServerEntry.m_PortName);

	rt = FALSE;
	switch(m_ServerEntry.m_ProxyMode & 7) {
	case 1:	// HTTP
	case 4:	// HTTP(Basic)
	case 3:	// SOCKS5
		if ( m_ServerEntry.m_ProxyPass.IsEmpty() )
			rt = TRUE;
		// no break;
	case 2:	// SOCKS4
		if ( m_ServerEntry.m_ProxyHost.IsEmpty() || m_ServerEntry.m_ProxyUser.IsEmpty() )
			rt = TRUE;
		break;
	}

	if ( rt ) {
		dlg.m_Title    = m_ServerEntry.m_EntryName;
		dlg.m_HostAddr = m_ServerEntry.m_ProxyHostProvs;
		dlg.m_UserName = m_ServerEntry.m_ProxyUserProvs;
		dlg.m_PassName = m_ServerEntry.m_ProxyPassProvs;
		dlg.m_Prompt   = _T("Proxy Password");
		dlg.m_MaxTime  = 120;

		if ( dlg.DoModal() != IDOK )
			return FALSE;

		m_ServerEntry.m_ProxyHost = dlg.m_HostAddr;
		m_ServerEntry.m_ProxyUser = dlg.m_UserName;
		m_ServerEntry.m_ProxyPass = dlg.m_PassName;
	}

	if ( m_ServerEntry.m_HostName.IsEmpty() ||
		((m_ServerEntry.m_ProtoType == PROTO_TELNET || m_ServerEntry.m_ProtoType == PROTO_SSH || m_ServerEntry.m_ProtoType == PROTO_LOGIN) && 
			(m_TextRam.IsOptEnable(TO_RLUSEPASS) || m_ServerEntry.m_UserName.IsEmpty() || m_ServerEntry.m_PassName.IsEmpty())) ) {

		dlg.m_Title    = m_ServerEntry.m_EntryName;
		dlg.m_HostAddr = m_ServerEntry.m_HostNameProvs;
		dlg.m_UserName = m_ServerEntry.m_UserNameProvs;
		dlg.m_PassName = m_ServerEntry.m_PassNameProvs;
		dlg.m_Prompt   = _T("Password");
		dlg.m_MaxTime  = 120;

		if ( dlg.DoModal() != IDOK )
			return FALSE;

		m_ServerEntry.m_HostName = dlg.m_HostAddr;
		m_ServerEntry.m_UserName = dlg.m_UserName;
		m_ServerEntry.m_PassName = dlg.m_PassName;
	}

	if ( m_ServerEntry.m_HostName.IsEmpty() )
		return FALSE;

	if ( m_ServerEntry.m_ProtoType <= PROTO_SSH ) {
		hosts.GetParam(m_ServerEntry.m_HostName);

		if ( hosts.GetSize() > 1 ) {
			CCommandLineInfoEx cmds;
			cmds.ParseParam(_T("inpane"), TRUE, FALSE);
			SetEntryProBuffer();
			for ( int n = 1 ; n < hosts.GetSize() ; n++ ) {
				m_ServerEntry.m_HostName.Format(_T("'%s'"), hosts[n]);
				pApp->m_pServerEntry = &m_ServerEntry;
				pApp->OpenProcsCmd(&cmds);
			}
			m_InPane = TRUE;
			m_ServerEntry.m_HostName = hosts[0];

		} else if ( hosts.GetSize() > 0 )
			m_ServerEntry.m_HostName = hosts[0];
	}

	switch(m_ServerEntry.m_ProtoType) {
	case PROTO_DIRECT:  m_pSock = new CExtSocket(this);  break;
	case PROTO_LOGIN:   m_pSock = new CLogin(this);		 break;	// login
	case PROTO_TELNET:  m_pSock = new CTelnet(this);	 break;	// telnet
	case PROTO_SSH:     m_pSock = new Cssh(this);		 break;	// ssh
	case PROTO_COMPORT: m_pSock = new CComSock(this);	 break;	// com port
	case PROTO_PIPE:    m_pSock = new CPipeSock(this);   break; // pipe console
	}

	if ( m_ServerEntry.m_ProxyMode != 0 )
		rt = m_pSock->ProxyOpen(m_ServerEntry.m_ProxyMode,
			m_ServerEntry.m_ProxyHost, CExtSocket::GetPortNum(m_ServerEntry.m_ProxyPort),
			m_ServerEntry.m_ProxyUser, m_ServerEntry.m_ProxyPass, m_ServerEntry.m_HostName, num);
	else 
		rt = m_pSock->AsyncOpen(m_ServerEntry.m_HostName, num);

	if ( !rt ) {
		SocketClose();
		return FALSE;
	}

	SetStatus(_T("Open"));

	static LPCTSTR ProtoName[] = { _T("TCP"), _T("Login"), _T("Telnet"), _T("SSH"), _T("COM"), _T("PIPE") };
	m_pMainWnd->m_StatusString = ProtoName[m_ServerEntry.m_ProtoType];

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
int CRLoginDoc::SocketRecive(void *lpBuf, int nBufLen)
{
	if ( m_pSock == NULL )
		return 0;
	return m_pSock->Recive(lpBuf, nBufLen, 0);
}
void CRLoginDoc::SocketSend(void *lpBuf, int nBufLen)
{
	if ( m_pSock == NULL )
		return;

	//if ( m_pScript != NULL && m_pScript->m_ConsMode == DATA_BUF_HAVE ) {
	//	m_pScript->SetConsBuff((LPBYTE)lpBuf, nBufLen);
	//	return;
	//}

	if ( m_TextRam.IsOptEnable(TO_RLDELAY) ) {
		m_DelayBuf.Apend((LPBYTE)lpBuf, nBufLen);
		if ( m_DelayFlag == DELAY_NON )
			DelaySend();
	} else 
		m_pSock->Send(lpBuf, nBufLen);
}
void CRLoginDoc::SocketSendWindSize(int x, int y)
{
	if ( m_pSock == NULL )
		return;
	m_pSock->SendWindSize(x, y);
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

void CRLoginDoc::OnLogOpen() 
{
	if ( m_pLogFile != NULL ) {
		m_pLogFile->Close();
		delete m_pLogFile;
		m_pLogFile = NULL;
		return;
	}

	CFileDialog dlg(FALSE, _T("log"), _T("RLOGIN"), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGLOGFILE), AfxGetMainWnd());

	if ( dlg.DoModal() != IDOK )
		return;

	if ( (m_pLogFile = new CFileExt) == NULL )
		return;

	if ( !m_pLogFile->Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive) ) {
		AfxMessageBox(IDE_LOGOPENERROR);
		delete m_pLogFile;
		m_pLogFile = NULL;
		return;
	}

	m_pLogFile->SeekToEnd();
	m_TextRam.m_LogTimeFlag = TRUE;
}
void CRLoginDoc::OnUpdateLogOpen(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_pLogFile != NULL ? 1 : 0);
}

void CRLoginDoc::OnLoadDefault() 
{
	if ( AfxMessageBox(IDS_ALLINITREQ, MB_ICONQUESTION | MB_YESNO) != IDYES )
		return;

	m_TextRam.Serialize(FALSE);
	m_KeyTab.Serialize(FALSE);
	m_KeyMac.Serialize(FALSE);
	m_ParamTab.Serialize(FALSE);

	if ( m_LoadMode == DOCTYPE_ENTRYFILE )
		SetModifiedFlag(TRUE);
	else if ( m_LoadMode == DOCTYPE_MULTIFILE )
		m_pMainWnd->m_ModifiedFlag = TRUE;
	else if ( m_LoadMode == DOCTYPE_REGISTORY )
		m_ServerEntry.m_SaveFlag = TRUE;

	UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
}
void CRLoginDoc::OnSaveDefault() 
{
	m_TextRam.Serialize(TRUE);
	m_KeyTab.Serialize(TRUE);
	m_KeyMac.Serialize(TRUE);
	m_ParamTab.Serialize(TRUE);
}
void CRLoginDoc::OnSetOption() 
{
	COptDlg dlg(m_ServerEntry.m_EntryName, AfxGetMainWnd());

	dlg.m_pEntry    = &m_ServerEntry;
	dlg.m_pTextRam  = &(m_TextRam);
	dlg.m_pKeyTab   = &(m_KeyTab);
	dlg.m_pKeyMac   = &(m_KeyMac);
	dlg.m_pParamTab = &(m_ParamTab);
	dlg.m_pDocument = this;

	if ( dlg.DoModal() != IDOK )
		return;

	if ( (dlg.m_ModFlag & (UMOD_ANSIOPT | UMOD_MODKEY | UMOD_COLTAB | UMOD_BANKTAB | UMOD_DEFATT)) != 0 )
		m_TextRam.InitDefParam(TRUE, dlg.m_ModFlag);

	if ( m_LoadMode == DOCTYPE_ENTRYFILE )
		SetModifiedFlag(TRUE);
	else if ( m_LoadMode == DOCTYPE_MULTIFILE )
		m_pMainWnd->m_ModifiedFlag = TRUE;
	else if ( m_LoadMode == DOCTYPE_REGISTORY )
		m_ServerEntry.m_SaveFlag = TRUE;

	UpdateAllViews(NULL, (dlg.m_ModFlag & UMOD_RESIZE) != 0 ? UPDATE_RESIZE : UPDATE_INITPARA, NULL);
}

void CRLoginDoc::OnSftp()
{
	if ( m_pSock != NULL && m_pSock->m_Type == ESCT_SSH_MAIN && ((Cssh *)(m_pSock))->m_SSHVer == 2 )
		((Cssh *)(m_pSock))->OpenSFtpChannel();
}
void CRLoginDoc::OnUpdateSftp(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pSock != NULL && m_pSock->m_Type == ESCT_SSH_MAIN && ((Cssh *)(m_pSock))->m_SSHVer == 2);
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
#if 1
	if ( m_pSock == NULL )
		return;

	if ( nID == IDM_KERMIT_UPLOAD || nID == IDM_KERMIT_DOWNLOAD ) {
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
	}
#else
	CFileDialog dlg(TRUE, _T(""), _T(""), OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, CStringLoad(IDS_FILEDLGALLFILE), NULL);
	POSITION pos;
	CString fileName;
	CImage image;
	int c, x, y;
	COLORREF rgb;
	BYTE map[12][4];
	CString str;

	if ( dlg.DoModal() != IDOK )
		return;

	pos = dlg.GetStartPosition();
	while ( pos != NULL ) {
		fileName = dlg.GetNextPathName(pos);
		if ( image.Load(fileName) )
			continue;
		memset(map, 0, 12 * 4);
		for ( y = 0 ; y < image.GetHeight() && y < 24 ; y++ ) {
			for ( x = 0 ;  x < image.GetWidth() && x < 12 ; x++ ) {
				rgb = image.GetPixel(x, y);
				if ( rgb == 0 )
					map[x][y / 6] |= (1 << (y % 6));
			}
		}
		str.Empty();
		for ( y = 0 ; y < 4 ; y++ ) {
			for ( x = 0 ; x < 12 ; x++ )
				str += (char)(map[x][y] + 0x3F);
			str += (y < 3 ? '/' : ';');
		}
		TRACE("%s %s\n", fileName, str);
		image.Destroy();
	}
#endif
}
void CRLoginDoc::OnUpdateXYZModem(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pSock != NULL ? TRUE : FALSE);
}

void CRLoginDoc::OnChatStop()
{
	if ( m_pStrScript != NULL ) {
		if ( m_pStrScript->m_MakeFlag ) {
			if ( m_LoadMode == DOCTYPE_ENTRYFILE )
				SetModifiedFlag(TRUE);
			else if ( m_LoadMode == DOCTYPE_REGISTORY )
				m_ServerEntry.m_SaveFlag = TRUE;
		}
		m_pStrScript->ExecStop();
		m_pStrScript = NULL;
	} else if ( m_pScript != NULL )
		m_pScript->OpenDebug();
	SetStatus(NULL);
}
void CRLoginDoc::OnUpdateChatStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pStrScript != NULL && m_pStrScript->Status() != SCPSTAT_NON ? TRUE : (m_pScript != NULL && m_pScript->m_CodePos != (-1) ? TRUE : FALSE));
}

void CRLoginDoc::OnSendBreak()
{
	if ( m_pSock != NULL && m_pSock->m_Type == ESCT_COMDEV )
		m_pSock->SendBreak(1);
}
void CRLoginDoc::OnUpdateSendBreak(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pSock != NULL && m_pSock->m_Type == ESCT_COMDEV ? TRUE : FALSE);
}
void CRLoginDoc::OnScriptMenu(UINT nID)
{
	if ( m_pScript != NULL )
		m_pScript->Call(m_pScript->m_MenuTab[nID - IDM_SCRIPT_MENU1].func);
}

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
	}
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
void CRLoginDoc::OnScreenReset(UINT nID)
{
	int mode = 0;
	switch(nID) {
	case IDM_RESET_TAB:   mode = RESET_TABS; break;
	case IDM_RESET_BANK:  mode = RESET_BANK; break;
	case IDM_RESET_ATTR:  mode = RESET_ATTR | RESET_COLOR; break;
	case IDM_RESET_TEK:   mode = RESET_TEK; break;
	case IDM_RESET_ESC:   mode = RESET_SAVE | RESET_CHAR; break;
	case IDM_RESET_MOUSE: mode = RESET_MOUSE; break;
	case IDM_RESET_ALL:   mode = RESET_PAGE | RESET_CURSOR | RESET_MARGIN | RESET_TABS | RESET_BANK | RESET_ATTR | RESET_COLOR | RESET_TEK | RESET_SAVE | RESET_MOUSE | RESET_CHAR | RESET_OPTION | RESET_CLS; break;
	}
	m_TextRam.RESET(mode);
	m_TextRam.FLUSH();
}

void CRLoginDoc::OnSocketstatus()
{
	CStatusDlg dlg;

	if ( m_pSock == NULL )
		return;

	m_pSock->GetStatus(dlg.m_Status);

	dlg.DoModal();
}
void CRLoginDoc::OnUpdateSocketstatus(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pSock != NULL ? TRUE : FALSE);
}

void CRLoginDoc::OnScript()
{
	if ( m_pScript == NULL )
		return;

	CFileDialog dlg(FALSE, _T("txt"), _T(""), 0, CStringLoad(IDS_FILEDLGSCRIPT), AfxGetMainWnd());

	if ( dlg.DoModal() != IDOK )
		return;

	if ( m_pScript->m_Code.GetSize() > 0 && AfxMessageBox(IDS_SCRIPTNEW, MB_ICONQUESTION | MB_YESNO) == IDYES ) {
		m_pScript->SetMenu(AfxGetMainWnd(), TRUE);
		if ( m_pScript != NULL )
			delete m_pScript;
		m_pScript = new CScript;
		m_pScript->SetDocument(this);
	}

	m_pScript->ExecFile(dlg.GetPathName());
}
void CRLoginDoc::OnUpdateScript(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pScript != NULL && m_pScript->IsExec() ? FALSE : TRUE);
}
void CRLoginDoc::OnCancelBtn()
{
	m_TextRam.fc_TimerAbort(FALSE);
}

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

	case 20:				// Document.Log.File
		if ( mode == DOC_MODE_IDENT ) {
			if ( m_pLogFile != NULL )
				value = (LPCTSTR)m_pLogFile->GetFilePath();
			else
				value = (int)0;
		}
		break;
	case 21:				// Document.Log.Mode
		n = m_TextRam.IsOptValue(TO_RLLOGMODE, 2);
		value.SetInt(n, mode);
		if ( m_pLogFile == NULL )
			m_TextRam.SetOptValue(TO_RLLOGMODE, 2, n);
		break;
	case 22:				// Document.Log.Code
		n = m_TextRam.IsOptValue(TO_RLLOGCODE, 2);
		value.SetInt(n, mode);
		if ( m_pLogFile == NULL )
			m_TextRam.SetOptValue(TO_RLLOGCODE, 2, n);
		break;
	case 23:				// Document.Log.Open(f)
		if ( mode == DOC_MODE_CALL ) {
			if ( m_pLogFile != NULL ) {
				m_pLogFile->Close();
				delete m_pLogFile;
			}
			if ( (m_pLogFile = new CFileExt) != NULL && m_pLogFile->Open((LPCTSTR)value[0], CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive) ) {
				m_pLogFile->SeekToEnd();
				m_TextRam.m_LogTimeFlag = TRUE;
				value = (int)0;
			} else
				value = (int)1;
		}
		break;
	case 24:				// Document.Log.Close()
		if ( mode == DOC_MODE_CALL && m_pLogFile != NULL ) {
			m_pLogFile->Close();
			delete m_pLogFile;
			m_pLogFile = NULL;
		}
		break;
	}
}
