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
	ON_COMMAND(ID_LOG_OPEN, OnLogOpen)
	ON_UPDATE_COMMAND_UI(ID_LOG_OPEN, OnUpdateLogOpen)
	ON_COMMAND(IDC_LOADDEFAULT, OnLoadDefault)
	ON_COMMAND(IDC_SAVEDEFAULT, OnSaveDefault)
	ON_COMMAND(ID_SETOPTION, OnSetOption)
	ON_COMMAND(IDM_SFTP, OnSftp)
	ON_UPDATE_COMMAND_UI(IDM_SFTP, OnUpdateSftp)
	ON_COMMAND(ID_CHARSCRIPT_END, &CRLoginDoc::OnChatStop)
	ON_UPDATE_COMMAND_UI(ID_CHARSCRIPT_END, &CRLoginDoc::OnUpdateChatStop)
	//}}AFX_MSG_MAP
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
	m_DelayFlag = FALSE;
	m_ActCharCount = 0;
	m_pMainWnd = (CMainFrame *)AfxGetMainWnd();
	m_TitleString.Empty();
	m_pScript = NULL;
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
	if ( m_DelayFlag )
		m_pMainWnd->DelTimerEvent(this);
}

BOOL CRLoginDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	if ( !m_pMainWnd->OpenServerEntry(m_ServerEntry) )
		return FALSE;

	m_LoadMode = (m_ServerEntry.m_SaveFlag ? 0 : 2);

	m_TextRam.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_KeyTab.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_KeyMac.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_ParamTab.Serialize(FALSE, m_ServerEntry.m_ProBuffer);
	m_TextRam.SetKanjiMode(m_ServerEntry.m_KanjiCode);

	if ( !SocketOpen() )
		return FALSE;

	UpdateAllViews(NULL, UPDATE_INITPARA, 0);
	m_TextRam.InitHistory();
	SetTitle(m_ServerEntry.m_EntryName);

	return TRUE;
}

BOOL CRLoginDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	m_LoadMode = 0;

	if ( !CDocument::OnOpenDocument(lpszPathName) )
		return FALSE;

	if ( m_LoadMode != 1 )
		return FALSE;
	
	if ( !SocketOpen() )
		return FALSE;

	UpdateAllViews(NULL, UPDATE_INITPARA, 0);
	m_TextRam.InitHistory();
	SetPathName(lpszPathName, TRUE);
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc シリアライゼーション

void CRLoginDoc::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {			// TODO: この位置に保存用のコードを追加してください。
		ar.WriteString("RLG200\n");
		m_ServerEntry.Serialize(ar);
		m_TextRam.Serialize(ar);
		m_KeyTab.Serialize(ar);
		m_KeyMac.Serialize(ar);
		m_ParamTab.Serialize(ar);
		m_LoadMode = 1;

	} else {						// TODO: この位置に読み込み用のコードを追加してください。
		int n;
		BYTE tmp[4096];

		ar.ReadString((LPSTR)tmp, 4096);
		if ( strncmp((LPSTR)tmp, "RLG2", 4) == 0 ) {
			m_ServerEntry.Serialize(ar);
			m_TextRam.Serialize(ar);
			m_KeyTab.Serialize(ar);
			m_KeyMac.Serialize(ar);
			m_ParamTab.Serialize(ar);
			m_TextRam.SetKanjiMode(m_ServerEntry.m_KanjiCode);
			m_LoadMode = 1;

		} else if ( strncmp((LPSTR)tmp, "RLM", 3) == 0 ) {
			m_pMainWnd->m_AllFilePath = ar.GetFile()->GetFilePath();
			m_pMainWnd->m_AllFileBuf.Clear();
			while ( (n = ar.Read(tmp, 4096)) > 0 )
				m_pMainWnd->m_AllFileBuf.Apend(tmp, n);
			m_pMainWnd->PostMessage(WM_COMMAND, ID_FILE_ALL_LOAD, 0);
			m_LoadMode = 2;

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

	m_ServerEntry.Init();
	CDocument::DeleteContents();
}

void CRLoginDoc::SetStatus(LPCSTR str)
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
		title.Format("%s:%d", m_ServerEntry.m_HostName, CExtSocket::GetPortNum(m_ServerEntry.m_PortName));
		break;
	case WTTL_USER:
		title = m_ServerEntry.m_UserName;
		break;
	}

	if ( str != NULL && *str != '\0' ) {
		((CMainFrame *)AfxGetMainWnd())->SetMessageText(str);
		m_TitleString = str;
	}

	if ( !m_TitleString.IsEmpty() && (m_TextRam.m_TitleMode & WTTL_ALGO) != 0 ) {
		tmp.Format("(%s)", m_TitleString);
		title += tmp;
	}

	if ( m_pScript != NULL ) {
		if ( (n = m_pScript->Status()) == SCPSTAT_MAKE )
			title += "...Chat Makeing";
		else if ( n == SCPSTAT_EXEC )
			title += "...Chat Execute";
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
	LPCSTR str = name;
	BOOL st = FALSE;

	while ( *str != '\0' ) {
		if ( *str == '%' ) {
			switch(str[1]) {
			case 'E':
				tmp += m_ServerEntry.m_EntryName;
				st = TRUE;
				break;
			case 'U':
				tmp += m_ServerEntry.m_UserName;
				st = TRUE;
				break;
			case 'P':
				tmp += m_ServerEntry.m_PassName;
				st = TRUE;
				break;
			case 'T':
				tmp += m_ServerEntry.m_TermName;
				st = TRUE;
				break;
			case 'S':
				tmp += m_ServerEntry.m_HostName;
				st = TRUE;
				break;
			case 'p':
				tmp += m_ServerEntry.m_PortName;
				st = TRUE;
				break;
			case 'D':
				tmp += tm.Format("%y%m%d");
				st = TRUE;
				break;
			case 't':
				tmp += tm.Format("%H%M%S");
				st = TRUE;
				break;
			case 'I':
				dlg.m_WinText = "FileName";
				dlg.m_Title = m_ServerEntry.m_EntryName;
				dlg.m_Edit  = tmp;
				tmp.Empty();
				if ( dlg.DoModal() == IDOK )
					tmp = dlg.m_Edit;
				st = TRUE;
				break;
			case '%':
				tmp += '%';
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

	while ( *str != '\0' ) {
		if ( *str == '%' ) {
			switch(str[1]) {
			case 'E':
				tmp += m_ServerEntry.m_EntryName;
				break;
			case 'U':
				tmp += m_ServerEntry.m_UserName;
				break;
			case 'P':
				tmp += m_ServerEntry.m_PassName;
				break;
			case 'T':
				tmp += m_ServerEntry.m_TermName;
				break;
			case 'S':
				tmp += m_ServerEntry.m_HostName;
				break;
			case 'p':
				tmp += m_ServerEntry.m_PortName;
				break;
			case 'D':
				tmp += tm.Format("%y%m%d");
				break;
			case 't':
				tmp += tm.Format("%H%M%S");
				break;
			case 'I':
				dlg.m_WinText = "ChatScript";
				if ( match != NULL ) dlg.m_Title = match;
				dlg.m_Edit  = tmp;
				tmp.Empty();
				if ( dlg.DoModal() == IDOK )
					tmp = dlg.m_Edit;
				break;
			case '%':
				tmp += '%';
				break;
			}
			str += 2;
		} else if ( *str == '\\' ) {
			switch(str[1]) {
			case 'a': tmp += '\x07'; str += 2; break;
			case 'b': tmp += '\x08'; str += 2; break;
			case 't': tmp += '\x09'; str += 2; break;
			case 'n': tmp += '\x0A'; str += 2; break;
			case 'v': tmp += '\x0B'; str += 2; break;
			case 'f': tmp += '\x0C'; str += 2; break;
			case 'r': tmp += '\x0D'; str += 2; break;
			case '\\': tmp += '\\'; str += 2; break;

			case 'x': case 'X':
				str += 2;
				for ( n = c = 0 ; n < 2 ; n++ ) {
					if ( *str >= '0' && *str <= '9' )
						c = c * 16 + (*(str++) - '0');
					else if ( *str >= 'A' && *str <= 'F' )
						c = c * 16 + (*(str++) - 'A' + 10);
					else if ( *str >= 'a' && *str <= 'f' )
						c = c * 16 + (*(str++) - 'a' + 10);
					else
						break;
				}
				tmp += c;
				break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				str += 1;
				for ( n = c = 0 ; n < 3 ; n++ ) {
					if ( *str >= '0' && *str <= '7' )
						c = c * 8 + (*(str++) - '0');
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
void CRLoginDoc::OnReciveChar(int ch)
{
	LPCWSTR str;

	while ( m_pScript != NULL && (str = m_pScript->ExecChar(ch)) != NULL ) {
		SendScript(str, m_pScript->m_Res.m_Str);
		if ( m_pScript->m_Exec == NULL ) {
			m_pScript->ExecStop();
			m_pScript = NULL;
			SetStatus(NULL);
			break;
		}
		ch = 0;
	}
}
void CRLoginDoc::OnSendBuffer(CBuffer &buf)
{
	if ( m_pScript != NULL && m_pScript->m_MakeFlag )
		m_pScript->SendStr((LPCWSTR)(buf.GetPtr()), buf.GetSize() / sizeof(WCHAR), &m_ServerEntry);
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
			m_DelayFlag = TRUE;
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
	m_DelayFlag = FALSE;

	if ( ch >= 0 ) {
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);
		if ( m_DelayBuf.GetSize() > 0 && m_TextRam.m_DelayMSec > 0 )
			m_pMainWnd->SetTimerEvent(m_TextRam.m_DelayMSec, TIMEREVENT_DOC, this);
		else if ( DelaySend() )
			m_pMainWnd->SetTimerEvent(DELAY_ECHO_MSEC, TIMEREVENT_DOC, this);
	} else {
		if ( DelaySend() )
			m_pMainWnd->SetTimerEvent(DELAY_ECHO_MSEC, TIMEREVENT_DOC, this);
	}
}

void CRLoginDoc::OnSocketConnect()
{
	if ( m_pSock == NULL )
		return;
	m_ServerEntry.m_Script.ExecInit();
	if ( m_ServerEntry.m_Script.Status() != SCPSTAT_NON ) {
		m_pScript = &(m_ServerEntry.m_Script);
		OnReciveChar(0);
	}
	SetStatus("Connect");

	if ( m_TextRam.IsOptEnable(TO_RLHISDATE) && !m_TextRam.m_LogFile.IsEmpty() ) {
		int n;
		int num = 1;
		CString file, dirs, name, exts;

		if ( (n = m_TextRam.m_LogFile.ReverseFind('\\')) >= 0 || (n = m_TextRam.m_LogFile.ReverseFind(':')) >= 0 ) {
			dirs = m_TextRam.m_LogFile.Left(n + 1);
			name = m_TextRam.m_LogFile.Mid(n + 1);
		} else {
			dirs = "";
			name = m_TextRam.m_LogFile;
		}

		if ( (n = name.ReverseFind('.')) >= 0 ) {
			exts = name.Mid(n);
			name.Delete(n, exts.GetLength());
		}

		if ( !EntryText(name) ) {
			name += "-%D";
			EntryText(name);
		}

		if ( m_pLogFile != NULL ) {
			m_pLogFile->Close();
			delete m_pLogFile;
			m_pLogFile = NULL;
		}

		if ( (m_pLogFile = new CFile) == NULL )
			return;

		file.Format("%s%s%s", dirs, name, exts);

		while ( !m_pLogFile->Open(file, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive) ) {
			if ( ++num > 20 ) {
				delete m_pLogFile;
				m_pLogFile = NULL;
				return;
			}
			file.Format("%s%s-%d%s", dirs, name, num, exts);
		}

		m_pLogFile->SeekToEnd();
	}
}
void CRLoginDoc::OnSocketError(int err)
{
//	SocketClose();
	SetStatus("Error");
	CString tmp;
	if ( m_ErrorPrompt.IsEmpty() )
		m_ErrorPrompt.Format("WinSock Have Error #%d", err);
	tmp.Format("%s Server Entry Scoket Error\n%s:%s Connection\n%s",
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
	SetStatus("Close");

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

	if ( m_pLogFile != NULL && m_TextRam.IsOptValue(TO_RLLOGMODE, 2) == 0 )
		m_pLogFile->Write(lpBuf, nBufLen);

	m_pMainWnd->WakeUpSleep();

	return nBufLen;
}

int CRLoginDoc::SocketOpen()
{
	BOOL rt;
	int num;
	CPassDlg dlg;
	CString proxy[3];
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	if ( InternetAttemptConnect(0) != ERROR_SUCCESS )
		return FALSE;

	if ( pApp->m_pCmdInfo != NULL ) {
		if ( pApp->m_pCmdInfo->m_Proto != (-1) )
			m_ServerEntry.m_ProtoType = pApp->m_pCmdInfo->m_Proto;
		if ( !pApp->m_pCmdInfo->m_Addr.IsEmpty() )
			m_ServerEntry.m_HostReal = m_ServerEntry.m_HostName = pApp->m_pCmdInfo->m_Addr;
		if ( !pApp->m_pCmdInfo->m_Port.IsEmpty() )
			m_ServerEntry.m_PortName = pApp->m_pCmdInfo->m_Port;
		if ( !pApp->m_pCmdInfo->m_User.IsEmpty() )
			m_ServerEntry.m_UserReal = m_ServerEntry.m_UserName = pApp->m_pCmdInfo->m_User;
		if ( !pApp->m_pCmdInfo->m_Pass.IsEmpty() )
			m_ServerEntry.m_PassReal = m_ServerEntry.m_PassName = pApp->m_pCmdInfo->m_Pass;
		if ( !pApp->m_pCmdInfo->m_Term.IsEmpty() )
			m_ServerEntry.m_TermName = pApp->m_pCmdInfo->m_Term;
	}

	num = CExtSocket::GetPortNum(m_ServerEntry.m_PortName);

	rt = FALSE;
	switch(m_ServerEntry.m_ProxyMode & 7) {
	case 1:	// HTTP
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
		dlg.m_HostAddr = m_ServerEntry.m_ProxyHost;
		dlg.m_UserName = m_ServerEntry.m_ProxyUser;
		dlg.m_PassName = m_ServerEntry.m_ProxyPass;
		dlg.m_Prompt   = "Proxy Password";
		dlg.m_MaxTime  = 120;

		if ( dlg.DoModal() != IDOK )
			return FALSE;

		proxy[0] = dlg.m_HostAddr;
		proxy[1] = dlg.m_UserName;
		proxy[2] = dlg.m_PassName;

	} else {
		proxy[0] = m_ServerEntry.m_ProxyHost;
		proxy[1] = m_ServerEntry.m_ProxyUser;
		proxy[2] = m_ServerEntry.m_ProxyPass;
	}

	if ( m_ServerEntry.m_HostReal.IsEmpty() ||
		((m_ServerEntry.m_ProtoType == PROTO_TELNET || m_ServerEntry.m_ProtoType == PROTO_SSH) && 
		 (m_TextRam.IsOptEnable(TO_RLUSEPASS) || m_ServerEntry.m_UserReal.IsEmpty() || m_ServerEntry.m_PassReal.IsEmpty())) ) {

		dlg.m_Title    = m_ServerEntry.m_EntryName;
		dlg.m_HostAddr = m_ServerEntry.m_HostReal;
		dlg.m_UserName = m_ServerEntry.m_UserReal;
		dlg.m_PassName = m_ServerEntry.m_PassReal;
		dlg.m_Prompt   = "Password";
		dlg.m_MaxTime  = 120;

		if ( dlg.DoModal() != IDOK )
			return FALSE;

		m_ServerEntry.m_HostName = dlg.m_HostAddr;
		m_ServerEntry.m_UserName = dlg.m_UserName;
		m_ServerEntry.m_PassName = dlg.m_PassName;
	}

	if ( m_ServerEntry.m_HostName.IsEmpty() )
		return FALSE;

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
			proxy[0], CExtSocket::GetPortNum(m_ServerEntry.m_ProxyPort),
			proxy[1], proxy[2], m_ServerEntry.m_HostName, num);
	else 
		rt = m_pSock->AsyncOpen(m_ServerEntry.m_HostName, num);

	if ( !rt ) {
		SocketClose();
		return FALSE;
	}

	SetStatus("Open");

	static const char *ProtoName[] = { "TCP", "Login", "Telnet", "SSH", "COM", "PIPE" };
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
	if ( m_TextRam.IsOptEnable(TO_RLDELAY) ) {
		m_DelayBuf.Apend((LPBYTE)lpBuf, nBufLen);
		if ( !m_DelayFlag && DelaySend() )
			m_pMainWnd->SetTimerEvent(DELAY_ECHO_MSEC, TIMEREVENT_DOC, this);
	} else 
		m_pSock->Send(lpBuf, nBufLen);
}
void CRLoginDoc::SocketSendWindSize(int x, int y)
{
	if ( m_pSock == NULL )
		return;
	m_pSock->SendWindSize(x, y);
}

void CRLoginDoc::OnLogOpen() 
{
	if ( m_pLogFile != NULL ) {
		m_pLogFile->Close();
		delete m_pLogFile;
		m_pLogFile = NULL;
		return;
	}

	CFileDialog dlg(FALSE, "log", "RLOGIN", OFN_HIDEREADONLY,
		"Log Files (*.log;*.txt)|*.log;*.txt|All Files (*.*)|*.*||", AfxGetMainWnd());

	if ( dlg.DoModal() != IDOK )
		return;

	if ( (m_pLogFile = new CFile) == NULL )
		return;

	if ( !m_pLogFile->Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive) ) {
		AfxMessageBox("Can't open LogFile");
		delete m_pLogFile;
		m_pLogFile = NULL;
		return;
	}

	m_pLogFile->SeekToEnd();
}
void CRLoginDoc::OnUpdateLogOpen(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_pLogFile != NULL ? 1 : 0);
}

void CRLoginDoc::OnLoadDefault() 
{
	m_TextRam.Serialize(FALSE);
	m_KeyTab.Serialize(FALSE);
	m_KeyMac.Serialize(FALSE);
	m_ParamTab.Serialize(FALSE);

	if ( m_LoadMode == 1 )
		SetModifiedFlag(TRUE);
	else if ( m_LoadMode == 2 )
		m_pMainWnd->m_ModifiedFlag = TRUE;

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
	dlg.m_pParamTab = &(m_ParamTab);
	dlg.m_pDocument = this;

	if ( dlg.DoModal() != IDOK )
		return;

	if ( m_LoadMode == 1 )
		SetModifiedFlag(TRUE);
	else if ( m_LoadMode == 2 )
		m_pMainWnd->m_ModifiedFlag = TRUE;

	UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
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
}
void CRLoginDoc::OnUpdateXYZModem(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pSock != NULL ? TRUE : FALSE);
}

void CRLoginDoc::OnChatStop()
{
	if ( m_pScript != NULL ) {
		m_pScript->ExecStop();
		m_pScript = NULL;
	}
	SetStatus(NULL);
}
void CRLoginDoc::OnUpdateChatStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pScript != NULL && m_pScript->Status() != SCPSTAT_NON ? TRUE : FALSE);
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

void CRLoginDoc::DoDropFile()
{
	char *p;
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

	if ( (p = strrchr((char *)(LPCSTR)path, '\\')) != NULL || (p = strrchr((char *)(LPCSTR)path, ':')) != NULL )
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
	else
		m_TextRam.TekClose();
}
void CRLoginDoc::OnUpdateTekdisp(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_TextRam.m_pTekWnd != NULL ? TRUE : FALSE);
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
	case IDM_RESET_ALL:   mode = RESET_CURSOR | RESET_TABS | RESET_BANK | RESET_ATTR | RESET_COLOR | RESET_TEK | RESET_SAVE | RESET_MOUSE | RESET_CHAR | RESET_OPTION | RESET_CLS; break;
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
