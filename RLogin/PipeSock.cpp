#include "StdAfx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "PipeSock.h"

CPipeSock::CPipeSock(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = ESCT_PIPE;

	m_hIn[0] =  m_hIn[1]  = NULL;
	m_hOut[0] = m_hOut[1] = NULL;

	m_InThreadMode  = 0;
	m_OutThreadMode = 0;

	m_InThread = NULL;
	m_OutThread = NULL;

	m_pInEvent   = new CEvent(FALSE, TRUE);
	m_pOutEvent  = new CEvent(FALSE, TRUE);
	m_pSendEvent = new CEvent(FALSE, TRUE);

	memset(&m_proInfo, 0, sizeof(PROCESS_INFORMATION));

	memset(&m_ReadOverLap,  0, sizeof(OVERLAPPED));
	memset(&m_WriteOverLap, 0, sizeof(OVERLAPPED));

	m_ReadOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_WriteOverLap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CPipeSock::~CPipeSock(void)
{
	Close();

	delete m_pInEvent;
	delete m_pOutEvent;
	delete m_pSendEvent;

	CloseHandle(m_ReadOverLap.hEvent);
	CloseHandle(m_WriteOverLap.hEvent);
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
static UINT PipeInOutThread(LPVOID pParam)
{
	CPipeSock *pThis = (CPipeSock *)pParam;

	pThis->OnReadWriteProc();
	pThis->m_pOutEvent->SetEvent();
	pThis->m_OutThreadMode = 3;
	return 0;
}
BOOL CPipeSock::IsPipeName(LPCTSTR path)
{
	// \\.\pipe\NAME
	// \\server\pipe\NAME

	if ( *(path++) != _T('\\') )
		return FALSE;

	if ( *(path++) != _T('\\') )
		return FALSE;

	if ( *path == _T('\\') )
		return FALSE;

	while ( *path != _T('\0') ) {
		if ( *(path++) == _T('\\') )
			break;
	}
	
	if ( _tcsncmp(path, _T("pipe\\"), 5) != 0 )
		return FALSE;
	path += 5;

	if ( *path == _T('\\') || *path == _T('\0') )
		return FALSE;

	return TRUE;
}
BOOL CPipeSock::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
	Close();

	if ( IsPipeName(lpszHostAddress) ) {

		if ( (m_hIn[0] = CreateFile(lpszHostAddress, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE ) {
			::AfxMessageBox(GetFormatErrorMessage(m_pDocument->m_ServerEntry.m_EntryName, lpszHostAddress, 0, _T("OpenPipe"), ::GetLastError()), MB_ICONSTOP);
			return FALSE;
		}

		m_OutThreadMode = 1;
		m_pOutEvent->ResetEvent();
		m_OutThread = AfxBeginThread(PipeInOutThread, this, THREAD_PRIORITY_BELOW_NORMAL);

	} else {

		SECURITY_ATTRIBUTES secAtt;
		STARTUPINFO startInfo;

		secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
		secAtt.lpSecurityDescriptor = NULL;
		secAtt.bInheritHandle = TRUE;

		if ( !CreatePipe(&(m_hIn[0]), &(m_hIn[1]), &secAtt, 0) ) {
			::AfxMessageBox(GetFormatErrorMessage(m_pDocument->m_ServerEntry.m_EntryName, NULL, 0, _T("CreatePipe"), ::GetLastError()), MB_ICONSTOP);
			return FALSE;
		}

		secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
		secAtt.lpSecurityDescriptor = NULL;
		secAtt.bInheritHandle = TRUE;

		if ( !CreatePipe(&(m_hOut[0]), &(m_hOut[1]), &secAtt, 0) ) {
			::AfxMessageBox(GetFormatErrorMessage(m_pDocument->m_ServerEntry.m_EntryName, NULL, 0, _T("CreatePipe"), ::GetLastError()), MB_ICONSTOP);
			return FALSE;
		}

		memset(&startInfo,0,sizeof(STARTUPINFO));
		startInfo.cb = sizeof(STARTUPINFO);
		startInfo.dwFlags = STARTF_USEFILLATTRIBUTE | STARTF_USECOUNTCHARS | STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		startInfo.wShowWindow = SW_HIDE;
		startInfo.hStdInput  = m_hOut[0];
		startInfo.hStdOutput = m_hIn[1];
		startInfo.hStdError  = m_hIn[1];

		if ( !CreateProcess(NULL, (LPTSTR)(LPCTSTR)lpszHostAddress, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startInfo, &m_proInfo) ) {
			::AfxMessageBox(GetFormatErrorMessage(m_pDocument->m_ServerEntry.m_EntryName, lpszHostAddress, 0, _T("CreateProcess"), ::GetLastError()), MB_ICONSTOP);
			return FALSE;
		}

		m_InThreadMode = 1;
		m_pInEvent->ResetEvent();
		m_InThread = AfxBeginThread(PipeInThread, this, THREAD_PRIORITY_BELOW_NORMAL);

		m_OutThreadMode = 1;
		m_pOutEvent->ResetEvent();
		m_OutThread = AfxBeginThread(PipeOutThread, this, THREAD_PRIORITY_BELOW_NORMAL);
	}

	GetApp()->AddIdleProc(IDLEPROC_SOCKET, this);
	GetMainWnd()->SetAsyncSelect((SOCKET)m_hIn[0], this, 0);

//	CExtSocket::OnConnect();
	GetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hIn[0], FD_CONNECT);

	return TRUE;
}
BOOL CPipeSock::AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);
}
void CPipeSock::Close()
{
	if ( m_hIn[0] != NULL )
		GetMainWnd()->DelAsyncSelect((SOCKET)m_hIn[0], this, FALSE);

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
		m_pSendEvent->SetEvent();
		SetEvent(m_ReadOverLap.hEvent);
		SetEvent(m_WriteOverLap.hEvent);
		WaitForSingleObject(m_pOutEvent->m_hObject, INFINITE);
		m_OutThreadMode = 0;
	}

	if ( m_hIn[0] != NULL )
	   CloseHandle(m_hIn[0]);
	if ( m_hOut[1] != NULL )
	   CloseHandle(m_hOut[1]);

	m_hIn[0] =  m_hIn[1]  = NULL;
	m_hOut[0] = m_hOut[1] = NULL;

	GetApp()->DelIdleProc(IDLEPROC_SOCKET, this);

	m_SendBuff.Clear();
	m_pSendEvent->ResetEvent();

	CExtSocket::Close();
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
	m_pSendEvent->SetEvent();
	m_SendSema.Unlock();
	return nBufLen;
}
int CPipeSock::GetRecvSize()
{
	int len = CExtSocket::GetRecvSize();
	
	m_RecvSema.Lock();
	len += m_RecvBuff.GetSize();
	m_RecvSema.Unlock();

	return len;
}
int CPipeSock::GetSendSize()
{
	int len = CExtSocket::GetSendSize();

	m_SendSema.Lock();
	len += m_SendBuff.GetSize();
	m_SendSema.Unlock();

	return len;
}

void CPipeSock::OnReceive(int nFlags)
{
	int n;
	BYTE buff[1024];

	m_RecvSema.Lock();
	while ( (n = m_RecvBuff.GetSize()) > 0 ) {
		if ( n > 1024 )
			n = 1024;
		memcpy(buff, m_RecvBuff.GetPtr(), n);
		m_RecvBuff.Consume(n);
		m_RecvSema.Unlock();
		OnReceiveCallBack(buff, n, nFlags);
		m_RecvSema.Lock();
	}
	m_RecvSema.Unlock();
}
void CPipeSock::OnSend()
{
	// AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hIn[0], FD_WRITE);
	// Send Empty Call

	CExtSocket::OnSendEmpty();
}
int CPipeSock::OnIdle()
{
	if ( CExtSocket::OnIdle() )
		return TRUE;

	if ( m_proInfo.hProcess != NULL ) {
		DWORD ec;
		if ( GetExitCodeProcess(m_proInfo.hProcess, &ec) ) {
			if ( ec != STILL_ACTIVE )
				OnClose();
		}
	}

	return FALSE;
}

void CPipeSock::OnReadProc()
{
	DWORD n;
	BYTE buff[1024];

	while ( m_InThreadMode == 1 ) {
		if ( !ReadFile(m_hIn[0], buff, 1024, &n, NULL ) )
			break;
		if ( n > 0 ) {
			m_RecvSema.Lock();
			m_RecvBuff.Apend(buff, n);
			m_RecvSema.Unlock();
			AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hIn[0], FD_READ);
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
			m_pSendEvent->ResetEvent();
			m_SendSema.Unlock();
			WaitForSingleObject(m_pSendEvent->m_hObject, INFINITE);
		}
	}
}
void CPipeSock::OnReadWriteProc()
{
	DWORD n;
	int HandleCount = 0;
	HANDLE HandleTab[4];
	BOOL bReadOverLap   = FALSE;
	BOOL bWriteOverLap  = FALSE;
	BOOL bHaveRecvData  = TRUE;
	BOOL bHaveSendData  = TRUE;
	BOOL bHaveSendReady = TRUE;
	DWORD HaveError = 0;
	DWORD ReadByte  = 0;
	DWORD WriteByte = 0;
	DWORD WriteTop = 0;
	BYTE ReadBuf[1024];
	BYTE WriteBuf[1024];
	CEvent TimerEvent(FALSE, TRUE);

	while ( m_OutThreadMode == 1 ) {

		HandleCount = 0;

		// ReadOverLap
		if ( bReadOverLap ) {
			if ( GetOverlappedResult(m_hIn[0], &m_ReadOverLap, &n, FALSE) ) {
				if ( n > 0 ) {
					ReadByte += n;
					m_RecvSema.Lock();
					m_RecvBuff.Apend(ReadBuf, n);
					m_RecvSema.Unlock();
				} else
					bHaveRecvData = FALSE;
				bReadOverLap = FALSE;
			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_ReadOverLap.hEvent;
				bReadOverLap = TRUE;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// ReadFile
		if ( !bReadOverLap && bHaveRecvData ) {
			if ( ReadFile(m_hIn[0], ReadBuf, 1024, &n, &m_ReadOverLap) ) {
				if ( n > 0 ) {
					ReadByte += n;
					m_RecvSema.Lock();
					m_RecvBuff.Apend(ReadBuf, n);
					m_RecvSema.Unlock();
				} else
					bHaveRecvData = FALSE;
			} else if ( (n = ::GetLastError()) == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_ReadOverLap.hEvent;
				bReadOverLap = TRUE;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// PostMsg Call OnReceive
		if ( ReadByte > 0 ) {
			ReadByte = 0;
			AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hIn[0], FD_READ);
		}

		// WriteOverLap
		if ( bWriteOverLap ) {
			if ( GetOverlappedResult(m_hIn[0], &m_WriteOverLap, &n, FALSE) ) {
				if ( n > 0 )
					WriteTop += n;
				else
					bHaveSendReady = FALSE;
				bWriteOverLap = FALSE;
			} else if ( (n = ::GetLastError()) == ERROR_IO_INCOMPLETE || n == ERROR_IO_PENDING ) {
				HandleTab[HandleCount++] = m_WriteOverLap.hEvent;
				bWriteOverLap = TRUE;
			} else {
				HaveError = n;
				goto ERRENDOF;
			}
		}

		// Check WriteData
		if ( !bWriteOverLap && bHaveSendData && WriteTop >= WriteByte ) {
			WriteTop = 0;
			m_SendSema.Lock();
			if ( (WriteByte = m_SendBuff.GetSize()) > 0 ) {
				if ( WriteByte > 1024 )
					WriteByte = 1024;
				memcpy(WriteBuf, m_SendBuff.GetPtr(), WriteByte);
				m_SendBuff.Consume(WriteByte);
				if ( m_SendBuff.GetSize() == 0 )
					AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hIn[0], FD_WRITE);
				m_SendSema.Unlock();
			} else {
				m_pSendEvent->ResetEvent();
				m_SendSema.Unlock();
				bHaveSendData = FALSE;
			}
		}

		// WriteFile
		if ( !bWriteOverLap && bHaveSendReady && WriteTop < WriteByte ) {
			while ( WriteTop < WriteByte ) {
				if ( WriteFile(m_hIn[0], WriteBuf + WriteTop, WriteByte - WriteTop, &n, &m_WriteOverLap) ) {
					if ( n > 0 ) {
						WriteTop += n;
					}else {
						bHaveSendReady = FALSE;
						break;
					}
				} else if ( (n = ::GetLastError()) == ERROR_IO_PENDING ) {
					HandleTab[HandleCount++] = m_WriteOverLap.hEvent;
					bWriteOverLap = TRUE;
					break;
				} else {
					HaveError = n;
					goto ERRENDOF;
				}
			}
		}

		// ReadData Loop Check
		if ( !bReadOverLap && bHaveRecvData ) {
			if ( !bWriteOverLap && !bHaveSendData && bHaveSendReady )
				bHaveSendData = TRUE;
			continue;
		}

		// WriteData Loop Check
		if ( !bWriteOverLap && bHaveSendData && bHaveSendReady ) {
			if ( !bReadOverLap && !bHaveRecvData )
				bHaveRecvData = TRUE;
			continue;
		}

		// NoOverLap or NotSendReady Timer Set
		if ( HandleCount == 0 || !bHaveSendReady ) {
			TimerEvent.ResetEvent();
			timeSetEvent(300, 10, (LPTIMECALLBACK)(TimerEvent.m_hObject), 0, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
			HandleTab[HandleCount++] = TimerEvent.m_hObject;
		}

		// SendData Event Add
		if ( !bHaveSendData )
			HandleTab[HandleCount++] = m_pSendEvent->m_hObject;

		TRACE("PipeThredProc ReadOverLap=%d, WriteOverLap=%d, HaveSendData=%d, HaveRecvData=%d, WaitFor=%d\n",
			bReadOverLap, bWriteOverLap, bHaveSendData, bHaveRecvData, HandleCount);

		// Wait For Event
		ASSERT(HandleCount > 0);
		n = WaitForMultipleObjects(HandleCount, HandleTab, FALSE, INFINITE);
		if ( n >= WAIT_OBJECT_0 && (n -= WAIT_OBJECT_0) < (DWORD)HandleCount ) {
			if ( HandleTab[n] == m_pSendEvent->m_hObject )
				bHaveSendData = TRUE;
			else if ( HandleTab[n] == TimerEvent.m_hObject ) {
				bHaveRecvData  = TRUE;
				bHaveSendReady = TRUE;
			}
		}
	}

ERRENDOF:
	if ( HaveError != 0 )
		AfxGetMainWnd()->PostMessage(WM_SOCKSEL, (WPARAM)m_hIn[0], WSAMAKESELECTREPLY(0, HaveError));

	if ( bReadOverLap || bWriteOverLap )
		CancelIo(m_hIn[0]);
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

	if ( (sz = GetEnvironmentVariable(_T("PATH"), _T(""), 0)) > 0 )
		GetEnvironmentVariable(_T("PATH"), tmp.GetBufferSetLength(sz), sz);
	dirs.GetString(tmp, ';');
	tmp.ReleaseBuffer();

	if ( (sz = GetEnvironmentVariable(_T("PATHEXT"), _T(""), 0)) > 0 )
		GetEnvironmentVariable(_T("PATHEXT"), tmp.GetBufferSetLength(sz), sz);
	exts.GetString(tmp, _T(';'));
	tmp.ReleaseBuffer();

	maps.RemoveAll();
	for ( i = 0 ; i < (int)dirs.GetSize() ; i++ ) {
		tmp.Format(_T("%s\\*.*"), dirs[i]);
		bWork = finder.FindFile(tmp);
		while ( bWork ) {
			bWork = finder.FindNextFile();
			if ( finder.IsDirectory() || finder.IsTemporary() )
				continue;
			tmp = finder.GetFileName();
			if ( (a = tmp.ReverseFind('.')) >= 0 && exts.FindNoCase(tmp.Mid(a)) >= 0 ) {
				wstr = (tmp.Mid(a).CompareNoCase(_T(".exe")) == 0 ? tmp.Left(a) : tmp);
				maps.Add(wstr);
			}
		}
		finder.Close();
	}
}
void CPipeSock::GetDirMaps(CStringMaps &maps, LPCTSTR dir, BOOL pf)
{
	CString tmp;
	CFileFind finder;
	BOOL bWork;
	CStringW wstr;

	maps.RemoveAll();
	tmp.Format(_T("%s\\*.*"), dir);
	bWork = finder.FindFile(tmp);
	while ( bWork ) {
		bWork = finder.FindNextFile();
		if ( finder.IsDots() || finder.IsHidden() || finder.IsTemporary() )
			continue;
		wstr = (pf ? finder.GetFilePath() : finder.GetFileName());
		if ( finder.IsDirectory() )
			wstr += L'\\';
		maps.Add(wstr);
	}
	finder.Close();
}