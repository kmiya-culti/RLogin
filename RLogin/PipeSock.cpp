#include "StdAfx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "PipeSock.h"

CPipeSock::CPipeSock(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = 6;

	m_hIn[0] =  m_hIn[1]  = NULL;
	m_hOut[0] = m_hOut[1] = NULL;

	m_InThreadMode  = 0;
	m_OutThreadMode = 0;

	m_InThread = NULL;
	m_OutThread = NULL;

	m_pInEvent   = new CEvent(FALSE, TRUE);
	m_pOutEvent  = new CEvent(FALSE, TRUE);
	m_pWaitEvent = new CEvent(FALSE, TRUE);

	memset(&m_proInfo, 0, sizeof(PROCESS_INFORMATION));
}

CPipeSock::~CPipeSock(void)
{
	Close();
	delete m_pInEvent;
	delete m_pOutEvent;
	delete m_pWaitEvent;
}
static UINT PipeInThread(LPVOID pParam)
{
	CPipeSock *pThis = (CPipeSock *)pParam;

	pThis->OnReadProc();
	pThis->m_pInEvent->SetEvent();
	pThis->m_InThreadMode = 3;
	return 0;
}
static UINT PipeOutThread(LPVOID pParam)
{
	CPipeSock *pThis = (CPipeSock *)pParam;

	pThis->OnWriteProc();
	pThis->m_pOutEvent->SetEvent();
	pThis->m_OutThreadMode = 3;
	return 0;
}
BOOL CPipeSock::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	Close();

	SECURITY_ATTRIBUTES secAtt;
	STARTUPINFO startInfo;

	secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAtt.lpSecurityDescriptor = NULL;
	secAtt.bInheritHandle = TRUE;

	if ( !CreatePipe(&(m_hIn[0]), &(m_hIn[1]), &secAtt, 0) ) {
		CString errmsg;
		errmsg.Format("PipeSocket StdIn CreatePipe Error '%s'", lpszHostAddress);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
	}

	secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAtt.lpSecurityDescriptor = NULL;
	secAtt.bInheritHandle = TRUE;

	if ( !CreatePipe(&(m_hOut[0]), &(m_hOut[1]), &secAtt, 0) ) {
		CString errmsg;
		errmsg.Format("PipeSocket StdOut CreatePipe Error '%s'", lpszHostAddress);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
	}

	memset(&startInfo,0,sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.dwFlags = STARTF_USEFILLATTRIBUTE | STARTF_USECOUNTCHARS | STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	startInfo.wShowWindow = SW_HIDE;
	startInfo.hStdInput  = m_hOut[0];
	startInfo.hStdOutput = m_hIn[1];
	startInfo.hStdError  = m_hIn[1];

	if ( !CreateProcess(NULL, (LPSTR)(LPCSTR)lpszHostAddress, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startInfo, &m_proInfo) ) {
		CString errmsg;
		errmsg.Format("PipeSocket CreateProcess Error '%s'", lpszHostAddress);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
	}

	m_SendBuff.Clear();
	m_pWaitEvent->ResetEvent();

	m_InThreadMode = 1;
	m_pInEvent->ResetEvent();
	m_InThread = AfxBeginThread(PipeInThread, this, THREAD_PRIORITY_BELOW_NORMAL);

	m_OutThreadMode = 1;
	m_pOutEvent->ResetEvent();
	m_OutThread = AfxBeginThread(PipeOutThread, this, THREAD_PRIORITY_BELOW_NORMAL);

	GetApp()->SetSocketIdle(this);
	return TRUE;
}
BOOL CPipeSock::AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);
}
void CPipeSock::Close()
{
	if ( m_proInfo.hProcess != NULL ) {
		DWORD ec;
		if ( GetExitCodeProcess(m_proInfo.hProcess, &ec) ) {
			if ( ec == STILL_ACTIVE )
				TerminateProcess(m_proInfo.hProcess, 0);
		}
		WaitForSingleObject(m_proInfo.hProcess, INFINITE);
		CloseHandle(m_proInfo.hProcess);
		CloseHandle(m_proInfo.hThread);
		m_proInfo.hProcess = NULL;
	}

	if ( m_hIn[1] != NULL )
	   CloseHandle(m_hIn[1]);
	if ( m_hOut[0] != NULL )
	   CloseHandle(m_hOut[0]);

	if ( m_InThreadMode != 0 ) {
		m_InThreadMode = 2;
		WaitForSingleObject(m_pInEvent->m_hObject, INFINITE);
		m_InThreadMode = 0;
	}
	if ( m_OutThreadMode != 0 ) {
		m_OutThreadMode = 2;
		m_pWaitEvent->SetEvent();
		WaitForSingleObject(m_pOutEvent->m_hObject, INFINITE);
		m_OutThreadMode = 0;
	}

	if ( m_hIn[0] != NULL )
	   CloseHandle(m_hIn[0]);
	if ( m_hOut[1] != NULL )
	   CloseHandle(m_hOut[1]);

	m_hIn[0] =  m_hIn[1]  = NULL;
	m_hOut[0] = m_hOut[1] = NULL;

	GetApp()->DelSocketIdle(this);
}
void CPipeSock::SendBreak(int opt)
{
	if ( m_proInfo.hProcess == NULL )
		return;
	GenerateConsoleCtrlEvent((opt ? CTRL_BREAK_EVENT : CTRL_C_EVENT), m_proInfo.dwProcessId);
}
int CPipeSock::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	if ( lpBuf == NULL || nBufLen <= 0 )
		return 0;
	m_SendSema.Lock();
	m_SendBuff.Apend((LPBYTE)lpBuf, nBufLen);
	m_pWaitEvent->SetEvent();
	m_SendSema.Unlock();
	return nBufLen;
}
int CPipeSock::OnIdle()
{
	if ( CExtSocket::OnIdle() )
		return TRUE;

	if ( m_hIn == NULL )
		return FALSE;

	m_RecvSema.Lock();
	if ( m_ReadBuff.GetSize() > 0 ) {
		OnReciveCallBack(m_ReadBuff.GetPtr(), m_ReadBuff.GetSize(), 0);
		m_ReadBuff.Clear();
	}
	m_RecvSema.Unlock();

	if ( m_proInfo.hProcess != NULL ) {
		DWORD ec;
		if ( GetExitCodeProcess(m_proInfo.hProcess, &ec) ) {
			if ( ec != STILL_ACTIVE )
				OnClose();
		}
	}

	return TRUE;
}
void CPipeSock::OnReadProc()
{
	DWORD n;
	BYTE buff[4096];

	while ( m_InThreadMode == 1 ) {
		if ( !ReadFile(m_hIn[0], buff, 4096, &n, NULL ) )
			break;
		if ( n > 0 ) {
			m_RecvSema.Lock();
			m_ReadBuff.Apend(buff, n);
			m_RecvSema.Unlock();
		}
	}
}
void CPipeSock::OnWriteProc()
{
	DWORD n;
	CBuffer tmp;

	while ( m_OutThreadMode == 1 ) {
		m_SendSema.Lock();
		if ( m_SendBuff.GetSize() > 0 ) {
			tmp.Apend(m_SendBuff.GetPtr(), m_SendBuff.GetSize());
			m_SendBuff.Clear();
			m_SendSema.Unlock();
			while ( tmp.GetSize() > 0 ) {
				if ( !WriteFile(m_hOut[1], tmp.GetPtr(), tmp.GetSize(), &n, NULL) )
					return;
				tmp.Consume(n);
			}
			FlushFileBuffers(m_hOut[1]);
		} else {
			m_pWaitEvent->ResetEvent();
			m_SendSema.Unlock();
			WaitForSingleObject(m_pWaitEvent->m_hObject, INFINITE);
		}
	}
}
void CPipeSock::GetPathMaps(CStringMaps &maps)
{
	int i, a;
	DWORD sz;
	CString tmp;
	CStringArrayExt dirs, exts;
	CFileFind finder;
	BOOL bWork;
	CStringW wstr;

	if ( (sz = GetEnvironmentVariable("PATH", "", 0)) > 0 )
		GetEnvironmentVariable("PATH", tmp.GetBufferSetLength(sz), sz);
	dirs.GetString(tmp, ';');
	tmp.ReleaseBuffer();

	if ( (sz = GetEnvironmentVariable("PATHEXT", "", 0)) > 0 )
		GetEnvironmentVariable("PATHEXT", tmp.GetBufferSetLength(sz), sz);
	exts.GetString(tmp, ';');
	tmp.ReleaseBuffer();

	maps.RemoveAll();
	for ( i = 0 ; i < (int)dirs.GetSize() ; i++ ) {
		tmp.Format("%s\\*.*", dirs[i]);
		bWork = finder.FindFile(tmp);
		while ( bWork ) {
			bWork = finder.FindNextFile();
			if ( finder.IsDirectory() || finder.IsTemporary() )
				continue;
			tmp = finder.GetFileName();
			if ( (a = tmp.ReverseFind('.')) >= 0 && exts.FindNoCase(tmp.Mid(a)) >= 0 ) {
				wstr = (tmp.Mid(a).CompareNoCase(".exe") == 0 ? tmp.Left(a) : tmp);
				maps.Add(wstr);
			}
		}
		finder.Close();
	}
}
void CPipeSock::GetDirMaps(CStringMaps &maps, LPCSTR dir, BOOL pf)
{
	CString tmp;
	CFileFind finder;
	BOOL bWork;
	CStringW wstr;

	maps.RemoveAll();
	tmp.Format("%s\\*.*", dir);
	bWork = finder.FindFile(tmp);
	while ( bWork ) {
		bWork = finder.FindNextFile();
		if ( finder.IsDots() || finder.IsHidden() || finder.IsTemporary() )
			continue;
		wstr = (pf ? finder.GetFilePath() : finder.GetFileName());
		if ( finder.IsDirectory() )
			wstr += '\\';
		maps.Add(wstr);
	}
	finder.Close();
}