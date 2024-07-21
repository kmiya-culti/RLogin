//////////////////////////////////////////////////////////////////////
// PipeSock.cpp : 実装ファイル
//

#include "StdAfx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "PipeSock.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////
// CPipeSock

CFifoPipe::CFifoPipe(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoASync(pDoc, pSock)
{
	m_Type = FIFO_TYPE_PIPE;

	m_hIn[0] =  m_hIn[1]  = NULL;
	m_hOut[0] = m_hOut[1] = NULL;

	m_InThreadMode  = 0;
	m_OutThreadMode = 0;

	m_ProcThread = NULL;
	m_InThread = NULL;
	m_OutThread = NULL;

	memset(&m_proInfo, 0, sizeof(PROCESS_INFORMATION));
}
CFifoPipe::~CFifoPipe()
{
	Close();
}
void CFifoPipe::OnUnLinked(int nFd, BOOL bMid)
{
	Close();
	CFifoBase::OnUnLinked(nFd, bMid);
}
BOOL CFifoPipe::FlowCtrlCheck(int nFd)
{
	BOOL bRet = FALSE;
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( (nFd & 1) == 0 ) {		// STDIN/EXTIN
			if ( !pFifo->m_bEndOf && pFifo->GetSize() == 0 ) {
				GetEvent(nFd)->ResetEvent();
				bRet = TRUE;
			}
		} else {					// STDOUT/EXTOUT
			if ( !pFifo->m_bEndOf && pFifo->GetSize() >= FIFO_BUFUPPER ) {
				GetEvent(nFd)->ResetEvent();
				bRet = TRUE;
			}
		}
		RelFifo(pFifo);
	}

	return bRet;
}
BOOL CFifoPipe::WaitForFifo(int nFd, HANDLE hAbortEvent)
{
	DWORD n;
	HANDLE HandleTab[2];

	if ( !FlowCtrlCheck(nFd) )
		return TRUE;

	HandleTab[0] = GetEvent(nFd)->m_hObject;
	HandleTab[1] = hAbortEvent;

	if ( (n = WaitForMultipleObjects(2, HandleTab, FALSE, INFINITE)) == WAIT_FAILED )
		return FALSE;
	else if ( n == WAIT_OBJECT_0 )	// GetEvent(nFd)->m_hObject
		return TRUE;
	else
		return FALSE;
}

static UINT ProcessThread(LPVOID pParam)
{
	CFifoPipe *pThis = (CFifoPipe *)pParam;

	pThis->OnProcWait();
	pThis->m_ThreadEvent[0].SetEvent();
	return 0;
}
static UINT PipeInThread(LPVOID pParam)
{
	CFifoPipe *pThis = (CFifoPipe *)pParam;

	pThis->OnReadProc();
	pThis->m_InThreadMode = 0;
	pThis->m_ThreadEvent[1].SetEvent();
	return 0;
}
static UINT PipeOutThread(LPVOID pParam)
{
	CFifoPipe *pThis = (CFifoPipe *)pParam;

	pThis->OnWriteProc();
	pThis->m_OutThreadMode = 0;
	pThis->m_ThreadEvent[2].SetEvent();
	return 0;
}
static UINT PipeInOutThread(LPVOID pParam)
{
	CFifoPipe *pThis = (CFifoPipe *)pParam;

	pThis->OnReadWriteProc();
	pThis->m_OutThreadMode = 0;
	pThis->m_ThreadEvent[2].SetEvent();
	return 0;
}
BOOL CFifoPipe::IsPipeName(LPCTSTR path)
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
BOOL CFifoPipe::Open(LPCTSTR pCommand)
{
	if ( pCommand == NULL )
		pCommand = m_Command;
	else
		m_Command = pCommand;

	if ( IsPipeName(pCommand) ) {

		if ( (m_hIn[0] = CreateFile(pCommand, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE ) {
			m_nLastError = GetLastError();
			::ThreadMessageBox(_T("Pipe create error\n'%s'\n"), pCommand);
			return FALSE;
		}

		m_OutThreadMode = 1;
		m_ThreadEvent[2].ResetEvent();
		m_OutThread = AfxBeginThread(PipeInOutThread, this, THREAD_PRIORITY_BELOW_NORMAL);

	} else {

		SECURITY_ATTRIBUTES secAtt;
		STARTUPINFO startInfo;

		secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
		secAtt.lpSecurityDescriptor = NULL;
		secAtt.bInheritHandle = TRUE;

		if ( !CreatePipe(&(m_hIn[0]), &(m_hIn[1]), &secAtt, 0) ) {
			m_nLastError = GetLastError();
			::ThreadMessageBox(_T("Pipe stdin open error\n'%s'\n"), pCommand);
			return FALSE;
		}

		secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
		secAtt.lpSecurityDescriptor = NULL;
		secAtt.bInheritHandle = TRUE;

		if ( !CreatePipe(&(m_hOut[0]), &(m_hOut[1]), &secAtt, 0) ) {
			m_nLastError = GetLastError();
			::ThreadMessageBox(_T("Pipe stdout open error\n'%s'\n"), pCommand);
			return FALSE;
		}

		memset(&startInfo,0,sizeof(STARTUPINFO));
		startInfo.cb = sizeof(STARTUPINFO);
		startInfo.dwFlags = STARTF_USEFILLATTRIBUTE | STARTF_USECOUNTCHARS | STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		startInfo.wShowWindow = SW_HIDE;
		startInfo.hStdInput  = m_hOut[0];
		startInfo.hStdOutput = m_hIn[1];
		startInfo.hStdError  = m_hIn[1];

		if ( !CreateProcess(NULL, (LPTSTR)(LPCTSTR)pCommand, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startInfo, &m_proInfo) ) {
			m_nLastError = GetLastError();
			::ThreadMessageBox(_T("Pipe create process error\n'%s'\n"), pCommand);
			return FALSE;
		}

		m_ThreadEvent[0].ResetEvent();
		m_ProcThread = AfxBeginThread(ProcessThread, this, THREAD_PRIORITY_BELOW_NORMAL);

		m_InThreadMode = 1;
		m_ThreadEvent[1].ResetEvent();
		m_InThread = AfxBeginThread(PipeInThread, this, THREAD_PRIORITY_BELOW_NORMAL);

		m_OutThreadMode = 1;
		m_ThreadEvent[2].ResetEvent();
		m_OutThread = AfxBeginThread(PipeOutThread, this, THREAD_PRIORITY_BELOW_NORMAL);
	}

	SendFdEvents(FIFO_STDOUT, FD_CONNECT, NULL);

	return TRUE;
}
void CFifoPipe::Close()
{
	if ( m_proInfo.hProcess != NULL ) {
		m_AbortEvent[0].SetEvent();
		WaitForEvent(m_ThreadEvent[0], _T("CFifoPipe ProcThread"));

		TerminateProcess(m_proInfo.hProcess, 0);
		WaitForEvent(m_proInfo.hProcess, _T("CFifoPipe Process"));

		CloseHandle(m_proInfo.hProcess);
		CloseHandle(m_proInfo.hThread);
		m_proInfo.hProcess = NULL;

		if ( m_hIn[1] != NULL )
		   CloseHandle(m_hIn[1]);
		if ( m_hOut[0] != NULL )
		   CloseHandle(m_hOut[0]);

		m_hIn[1] = m_hOut[0] = NULL;
	}

	if ( m_InThreadMode != 0 ) {
		m_InThreadMode = 2;
		m_AbortEvent[1].SetEvent();
		// 別スレッドの場合はCancelIoExが有効のようだがWinXPには、無いので注意
		if ( ExCancelIoEx != NULL )
			ExCancelIoEx(m_hIn[0], NULL);
		else
			CancelIo(m_hIn[0]);
		WaitForEvent(m_ThreadEvent[1], _T("CFifoPipe InThread"));
	}

	if ( m_OutThreadMode != 0 ) {
		m_OutThreadMode = 2;
		m_AbortEvent[2].SetEvent();
		if ( ExCancelIoEx != NULL )
			ExCancelIoEx(m_hOut[1], NULL);
		else
			CancelIo(m_hOut[1]);
		WaitForEvent(m_ThreadEvent[2], _T("CFifoPipe OutThread"));
	}

	if ( m_hIn[0] != NULL )
	   CloseHandle(m_hIn[0]);
	if ( m_hOut[1] != NULL )
	   CloseHandle(m_hOut[1]);

	m_hIn[0] =  m_hIn[1]  = NULL;
	m_hOut[0] = m_hOut[1] = NULL;
}
void CFifoPipe::SendBreak(int opt)
{
	if ( m_proInfo.hProcess != NULL )
		GenerateConsoleCtrlEvent((opt ? CTRL_BREAK_EVENT : CTRL_C_EVENT), m_proInfo.dwProcessId);
}

void CFifoPipe::OnProcWait()
{
	DWORD n;
	HANDLE HandleTab[3];
	FifoMsg *pFifoMsg;

	for ( ; ; ) {
		HandleTab[0] = m_AbortEvent[0];
		HandleTab[1] = m_proInfo.hProcess;
		HandleTab[2] = m_MsgEvent;

		if ( (n = WaitForMultipleObjects(2, HandleTab, FALSE, INFINITE)) == WAIT_FAILED ) {
			break;
		} else if ( n == WAIT_OBJECT_0 ) {			// m_AbortEvent
			break;

		} else if ( n == (WAIT_OBJECT_0 + 1) ) {	// hProcess
			CloseHandle(m_proInfo.hProcess);
			CloseHandle(m_proInfo.hThread);
			m_proInfo.hProcess = NULL;

			if ( m_hIn[1] != NULL )
			   CloseHandle(m_hIn[1]);
			if ( m_hOut[0] != NULL )
			   CloseHandle(m_hOut[0]);

			m_hIn[1] = m_hOut[0] = NULL;
			
			if ( ExCancelIoEx != NULL ) {
				ExCancelIoEx(m_hIn[0], NULL);
				ExCancelIoEx(m_hOut[1], NULL);
			} else {
				CancelIo(m_hIn[0]);
				CancelIo(m_hOut[1]);
			}

			break;

		} else if ( n == (WAIT_OBJECT_0 + 2) ) {	// m_MsgEvent
			m_MsgSemaphore.Lock();
			while ( (pFifoMsg = MsgRemoveHead()) != NULL ) {
				m_MsgSemaphore.Unlock();

				switch(pFifoMsg->cmd) {
				case FIFO_CMD_FDEVENTS:
					switch(pFifoMsg->msg) {
					case FD_CLOSE:
						m_nLastError = (int)(UINT_PTR)pFifoMsg->buffer;
						Read(FIFO_STDIN, NULL, 0);
						break;
					}
					break;

				case FIFO_CMD_MSGQUEIN:
					switch(pFifoMsg->param) {
					case FIFO_QCMD_SENDBREAK:
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDBREAK, opt);
						SendBreak(pFifoMsg->msg);
						break;
					}
					break;
				}

				DeleteMsg(pFifoMsg);
				m_MsgSemaphore.Lock();
			}
			m_MsgSemaphore.Unlock();

		} else {
			break;
		}
	}
}
void CFifoPipe::OnReadProc()
{
	DWORD n;
	BYTE buff[1024];

	while ( m_InThreadMode == 1 ) {
		if ( !ReadFile(m_hIn[0], buff, 1024, &n, NULL) ) {
			SendFdEvents(FIFO_STDOUT, FD_CLOSE, NULL);
			Write(FIFO_STDOUT, NULL, 0);
			break;
		} else if ( n == 0 )
			continue;
		else if ( Write(FIFO_STDOUT, buff, (int)n) < 0 )
			break;
		else if ( !WaitForFifo(FIFO_STDOUT, m_AbortEvent[1]) )
			break;
	}
}
void CFifoPipe::OnWriteProc()
{
	int len;
	BYTE buf[1024];

	while ( m_OutThreadMode == 1 ) {
		if ( !WaitForFifo(FIFO_STDIN, m_AbortEvent[2]) )
			break;
		else if ( (len = Read(FIFO_STDIN, buf, 1024)) < 0 )
			break;

		while ( len > 0 ) {
			DWORD n = 0;
			if ( !WriteFile(m_hOut[1], buf, (DWORD)len, &n, NULL) )
				return;
			len -= (int)n;
		}
	}
}
void CFifoPipe::OnReadWriteProc()
{
	int len;
	FifoMsg *pFifoMsg;

	DWORD dw;
	BOOL bRes;
	BYTE ReadBuf[1024];
	BYTE WriteBuf[1024];

	OVERLAPPED ReadOverLap;
	OVERLAPPED WriteOverLap;

	int HandleCount;
	HANDLE HandleTab[10];

	DWORD WriteDone = 0;
	DWORD WriteByte = 0;

	enum { READ_GETDATA, READ_OVERLAP, READ_OVERWAIT, READ_EVENTWAIT } ReadStat = READ_GETDATA;
	enum { WRITE_GETDATA, WRITE_HAVEDATA, WRITE_OVERLAP, WRITE_OVERWAIT, WRITE_EVENTWAIT } WriteStat = WRITE_GETDATA;

	memset(&ReadOverLap, 0 , sizeof(ReadOverLap));
	memset(&WriteOverLap, 0 , sizeof(WriteOverLap));

	ReadOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	WriteOverLap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	while ( m_OutThreadMode == 1 ) {

		// Init HandleTab conter
		HandleCount = 0;

		// ReadFile or ReadOverLap
		for ( ; ; ) {
			if ( ReadStat == READ_GETDATA  ) {
				bRes = ReadFile(m_hIn[0], ReadBuf, 1024, &dw, &ReadOverLap);
			} else if ( ReadStat == READ_OVERLAP ) {
				bRes = GetOverlappedResult(m_hIn[0], &ReadOverLap, &dw, FALSE);
			} else {
				if ( ReadStat == READ_OVERWAIT )
					HandleTab[HandleCount++] = ReadOverLap.hEvent;
				else if ( ReadStat == READ_EVENTWAIT )
					HandleTab[HandleCount++] = GetEvent(FIFO_STDOUT)->m_hObject;
				break;
			}

			if ( bRes ) {
				if ( Write(FIFO_STDOUT, ReadBuf, (int)dw) < 0 )
					goto ERRENDOF;
				ReadStat = FlowCtrlCheck(FIFO_STDOUT) ? READ_EVENTWAIT : READ_GETDATA;
			} else if ( (dw = ::GetLastError()) == ERROR_IO_INCOMPLETE || dw == ERROR_IO_PENDING ) {
				ReadStat = READ_OVERWAIT;
			} else {
				m_nLastError = (int)dw;
				goto ERRENDOF;
			}
		}

		// WriteFile or WriteOverLap
		for ( ; ; ) {
			if ( WriteStat == WRITE_GETDATA ) {
				if ( (len = Peek(FIFO_STDIN, WriteBuf, 1024)) < 0 ) {
					goto ERRENDOF;
				} else if ( len == 0 ) {
					WriteStat = WRITE_EVENTWAIT;
				} else {
					Consume(FIFO_STDIN, len);

					WriteDone = 0;
					WriteByte = (DWORD)len;
					WriteStat = WRITE_HAVEDATA;
				}
			}

			if ( WriteStat == WRITE_HAVEDATA ) {
				bRes = WriteFile(m_hIn[0], WriteBuf + WriteDone, WriteByte - WriteDone, &dw, &WriteOverLap);
			} else if ( WriteStat == WRITE_OVERLAP ) {
				bRes = GetOverlappedResult(m_hIn[0], &WriteOverLap, &dw, FALSE);
			} else {
				if ( WriteStat == WRITE_OVERWAIT )
					HandleTab[HandleCount++] = WriteOverLap.hEvent;
				else if ( WriteStat == WRITE_EVENTWAIT )
					HandleTab[HandleCount++] = GetEvent(FIFO_STDIN)->m_hObject;
				break;
			}

			if ( bRes ) {
				if ( (WriteDone += dw) >= WriteByte )
					WriteStat = WRITE_GETDATA;
				else
					WriteStat = WRITE_HAVEDATA;
			} else if ( (dw = ::GetLastError()) == ERROR_IO_INCOMPLETE || dw == ERROR_IO_PENDING ) {
				WriteStat = WRITE_OVERWAIT;
			} else {
				m_nLastError = (int)dw;
				goto ERRENDOF;
			}
		}

		// Wait For Event
		ASSERT(HandleCount > 0);

		HandleTab[HandleCount++] = m_AbortEvent[2];
		HandleTab[HandleCount++] = m_MsgEvent;

		if ( (dw = WaitForMultipleObjects(HandleCount, HandleTab, FALSE, INFINITE)) == WAIT_FAILED ) {
			m_nLastError = GetLastError();
			goto ERRENDOF;
		} if ( dw < WAIT_OBJECT_0 || (dw -= WAIT_OBJECT_0) >= (DWORD)HandleCount )
			goto ERRENDOF;

		if ( HandleTab[dw] == GetEvent(FIFO_STDOUT)->m_hObject )
			ReadStat = READ_GETDATA;
		else if ( HandleTab[dw] ==  ReadOverLap.hEvent )
			ReadStat = READ_OVERLAP;

		else if ( HandleTab[dw] == GetEvent(FIFO_STDIN)->m_hObject )
			WriteStat = WRITE_GETDATA;
		else if ( HandleTab[dw] == WriteOverLap.hEvent )
			WriteStat = WRITE_OVERLAP;

		else if ( HandleTab[dw] == m_AbortEvent[2] )
			goto ERRENDOF;

		else if ( HandleTab[dw] == m_MsgEvent ) {
			m_MsgSemaphore.Lock();
			while ( (pFifoMsg = MsgRemoveHead()) != NULL ) {
				m_MsgSemaphore.Unlock();

				switch(pFifoMsg->cmd) {
				case FIFO_CMD_FDEVENTS:
					switch(pFifoMsg->msg) {
					case FD_CLOSE:
						m_nLastError = (int)(UINT_PTR)pFifoMsg->buffer;
						Read(FIFO_STDIN, NULL, 0);
						break;
					}
					break;
				}

				DeleteMsg(pFifoMsg);
				m_MsgSemaphore.Lock();
			}
			m_MsgSemaphore.Unlock();
		}
	}

ERRENDOF:
	SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)(UINT_PTR)m_nLastError);
	Write(FIFO_STDOUT, NULL, 0);

	if ( ReadOverLap.hEvent != NULL )
		CloseHandle(ReadOverLap.hEvent);

	if (WriteOverLap.hEvent != NULL )
		CloseHandle(WriteOverLap.hEvent);
}

///////////////////////////////////////////////////////
// CPipeSock

CPipeSock::CPipeSock(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = ESCT_PIPE;
}
CPipeSock::~CPipeSock(void)
{
}
CFifoBase *CPipeSock::FifoLinkLeft()
{
	return new CFifoPipe(m_pDocument, this);
}
void CPipeSock::SendBreak(int opt)
{
	ASSERT(m_pFifoLeft != NULL && m_pFifoLeft->m_Type == FIFO_TYPE_PIPE);

	m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDBREAK, opt);
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
		tmp.Format(_T("%s\\*.*"), (LPCTSTR)dirs[i]);
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