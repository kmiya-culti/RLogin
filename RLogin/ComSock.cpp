//////////////////////////////////////////////////////////////////////
// ComSock.cpp : 実装ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "ComSock.h"
#include "ComMoniDlg.h"
#include "ComInitDlg.h"

#include <winbase.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CFifoCom

CFifoCom::CFifoCom(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoASync(pDoc, pSock)
{
	m_Type = FIFO_TYPE_COM;
	m_nBufSize = COMBUFSIZE;

	m_hCom = INVALID_HANDLE_VALUE;

	m_ThreadMode = THREAD_NONE;
	m_pComThread = NULL;

	m_SendWait[0] = m_SendWait[1] = 0;
	m_SendCrLf = '\r';
}
CFifoCom::~CFifoCom()
{
	Close();
}
void CFifoCom::OnUnLinked(int nFd, BOOL bMid)
{
	Close();
	CFifoBase::OnUnLinked(nFd, bMid);
}
static UINT ComEventThread(LPVOID pParam)
{
	CFifoCom *pThis = (CFifoCom *)pParam;

	pThis->OnReadWriteProc();
	pThis->m_ThreadMode = CFifoCom::THREAD_NONE;
	return 0;
}
BOOL CFifoCom::Open(HANDLE hCom)
{
	if ( (m_hCom = hCom) == INVALID_HANDLE_VALUE )
		return FALSE;

	m_ThreadMode = THREAD_RUN;
	m_pComThread = AfxBeginThread(ComEventThread, this, THREAD_PRIORITY_NORMAL);

	return TRUE;
}
void CFifoCom::Close()
{
	if ( m_ThreadMode == THREAD_RUN && m_pComThread != NULL ) {
		m_ThreadMode = THREAD_ENDOF;
		m_AbortEvent.SetEvent();
		WaitForSingleObject(m_pComThread, INFINITE);
	}
	m_pComThread = NULL;
}
BOOL CFifoCom::FlowCtrlCheck(int nFd)
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
int CFifoCom::CalcReadByte()
{
	ASSERT(m_pSock != NULL && m_pSock->m_Type == ESCT_COMDEV);

	int ByteSec = ((CComSock *)m_pSock)->GetByteSec();

	ByteSec = (ByteSec * 50 / 1000);	// Byte per 50ms

	if ( ByteSec < 1 )					// < 300 baud
		ByteSec = 1;
	else if ( ByteSec > COMBUFSIZE )	// >= 230400 baud (1152 byte/50ms)
		ByteSec = COMBUFSIZE;

	return ByteSec;
}
void CFifoCom::OnReadWriteProc()
{
	int len, pos;

	DWORD dw;
	BOOL bRes;
	BYTE ReadBuf[COMBUFSIZE];
	BYTE WriteBuf[COMBUFSIZE];

	OVERLAPPED ReadOverLap;
	OVERLAPPED WriteOverLap;
	OVERLAPPED CommOverLap;

	int HandleCount;
	HANDLE HandleTab[10];

	int ReadByte = COMBUFSIZE;

	DWORD WriteDone;
	DWORD WriteByte;
	int WriteWaitMsec;
	clock_t WriteReqClock = clock();
	CEvent WriteTimerEvent;
	UINT WriteTimerId = 0;

	DWORD ComEvent = 0;
	DWORD LastModemStatus;

	BOOL bDsrSensitivity = FALSE;
	BOOL bOutxDsrFlow = FALSE;
	BOOL bOutxCtsFlow = FALSE;

	enum { READ_GETDATA, READ_OVERLAP, READ_OVERWAIT, READ_EVENTWAIT } ReadStat = READ_GETDATA;
	enum { WRITE_GETDATA, WRITE_HAVEDATA, WRITE_OVERLAP, WRITE_OVERWAIT, WRITE_EVENTWAIT, WRITE_TIMERWAIT, WRITE_FLOWCTRL } WriteStat = WRITE_GETDATA;
	enum { COM_GETDATA, COM_OVERLAP, COM_OVERWAIT } ComStat = COM_GETDATA;

	ASSERT(m_pSock != NULL && m_pSock->m_Type == ESCT_COMDEV);
	CComSock *pSock = (CComSock *)m_pSock;

	memset(&ReadOverLap, 0 , sizeof(ReadOverLap));
	memset(&WriteOverLap, 0 , sizeof(WriteOverLap));
	memset(&CommOverLap, 0 , sizeof(CommOverLap));

	ReadOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	WriteOverLap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	CommOverLap.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

	if ( pSock->m_pComConf != NULL ) {
		if ( pSock->m_pComConf->dcb.fDsrSensitivity != 0 )
			bDsrSensitivity = TRUE;
		if ( pSock->m_pComConf->dcb.fOutxDsrFlow != 0 )
			bOutxDsrFlow = TRUE;
		if ( pSock->m_pComConf->dcb.fOutxCtsFlow != 0 )
			bOutxCtsFlow = TRUE;

		ReadByte = CalcReadByte();
	}

	LastModemStatus = pSock->m_ModemStatus;

	while ( m_ThreadMode == THREAD_RUN ) {

		// Init HandleTab conter
		HandleCount = 0;

		// WaitCommEvent or CommOverLap
		// GETDATA->OVERLAP->OVERWAIT->GETDATA
		for ( ; ; ) {
			if ( ComStat == COM_GETDATA ) {
				bRes = WaitCommEvent(m_hCom, &ComEvent, &CommOverLap);
			} else if ( ComStat == COM_OVERLAP ) {
				bRes = GetOverlappedResult(m_hCom, &CommOverLap, &dw, FALSE);
			} else {
				// Com Event set
				if ( ComStat == COM_OVERWAIT )
					HandleTab[HandleCount++] = CommOverLap.hEvent;
				break;
			}

			if ( bRes ) {
				GetCommModemStatus(m_hCom, &pSock->m_ModemStatus);
				ClearCommError(m_hCom, &dw, &(pSock->m_ComStat)); pSock->m_CommError |= dw;

				if ( pSock->m_pComMoni != NULL && pSock->m_pComMoni->m_hWnd != NULL && pSock->m_pComMoni->m_bActive )
					pSock->m_pComMoni->PostMessage(WM_COMMAND, (WPARAM)ID_COMM_EVENT_MONI);

				if ( bDsrSensitivity && (ComEvent & EV_DSR) != 0 && (LastModemStatus & MS_DSR_ON) != 0 )
					goto ERRENDOF;

				if ( (ComEvent & (EV_CTS | EV_DSR)) != 0 && WriteStat == WRITE_FLOWCTRL )
					WriteStat = WRITE_GETDATA;

				LastModemStatus = pSock->m_ModemStatus;
				ComStat = COM_GETDATA;

			} else if ( (dw = ::GetLastError()) == ERROR_IO_INCOMPLETE || dw == ERROR_IO_PENDING ) {
				ComStat = COM_OVERWAIT;
			} else {
				m_nLastError = (int)dw;
				goto ERRENDOF;
			}
		}

		// ReadFile or ReadOverLap
		// GETDATA->OVERLAP->OVERWAIT->GETDATA
		for ( ; ; ) {
			if ( ReadStat == READ_GETDATA  ) {
				bRes = ReadFile(m_hCom, ReadBuf, ReadByte, &dw, &ReadOverLap);
			} else if ( ReadStat == READ_OVERLAP ) {
				bRes = GetOverlappedResult(m_hCom, &ReadOverLap, &dw, FALSE);
			} else {
				// Read Event set
				if ( ReadStat == READ_OVERWAIT )
					HandleTab[HandleCount++] = ReadOverLap.hEvent;
				else if ( ReadStat == READ_EVENTWAIT )
					HandleTab[HandleCount++] = GetEvent(FIFO_STDOUT)->m_hObject;
				break;
			}

			if ( bRes ) {
				// ReadFile done
				if ( dw == 0 ) {
					ReadStat = READ_GETDATA;
				} else if ( Write(FIFO_STDOUT, ReadBuf, (int)dw) < 0 ) {
					goto ERRENDOF;
				} else {
					// Write STDOUT Overflow check
					pSock->m_RecvByteSec += (int)dw;
					ReadStat = FlowCtrlCheck(FIFO_STDOUT) ? READ_EVENTWAIT : READ_GETDATA;
				}
			} else if ( (dw = ::GetLastError()) == ERROR_IO_INCOMPLETE || dw == ERROR_IO_PENDING ) {
				// ReadFile to overlap
				ReadStat = READ_OVERWAIT;
			} else {
				// ReadFile error
				m_nLastError = (int)dw;
				goto ERRENDOF;
			}
		}

		// WriteFile or WriteOverLap
		// GETDATA->HAVEDATA->OVERLAP->OVERWAIT->TIMERWAIT->GETDATA
		for ( ; ; ) {
			if ( WriteStat == WRITE_GETDATA ) {
				// Com DSR/CTS flow ctrl check
				if ( (bOutxDsrFlow && (pSock->m_ModemStatus & MS_DSR_ON) == 0) ||
					 (bOutxCtsFlow && (pSock->m_ModemStatus & MS_CTS_ON) == 0) ) {
					WriteStat = WRITE_FLOWCTRL;
					break;
				}

				// Read STDIN
				if ( (len = Peek(FIFO_STDIN, WriteBuf, COMBUFSIZE)) < 0 ) {
					goto ERRENDOF;
				} else if ( len == 0 ) {
					WriteStat = WRITE_EVENTWAIT;
				} else {
					// m_SendWait[0] = WC = msec/byte
					// m_SendWait[1] = WC = msec/byte
					if ( m_SendWait[1] != 0 ) {
						WriteWaitMsec = 0;
						for ( pos = 0 ; pos < len ; ) {
							if ( WriteBuf[pos++] == m_SendCrLf ) {
								WriteWaitMsec += m_SendWait[1];
								break;
							} else if ( m_SendWait[0] != 0 )
								WriteWaitMsec += m_SendWait[0];
						}
						len = pos;
					} else if ( m_SendWait[0] != 0 ) {
						WriteWaitMsec = len * m_SendWait[0];
					} else {
						WriteWaitMsec = 0;
					}

					Consume(FIFO_STDIN, len);
					pSock->m_SendByteSec += len;

					WriteDone = 0;
					WriteByte = (DWORD)len;
					WriteReqClock = clock();
					WriteStat = WRITE_HAVEDATA;
				}
			}

			if ( WriteStat == WRITE_HAVEDATA ) {
				// WriteFile start
				bRes = WriteFile(m_hCom, WriteBuf + WriteDone, WriteByte - WriteDone, &dw, &WriteOverLap);
			} else if ( WriteStat == WRITE_OVERLAP ) {
				bRes = GetOverlappedResult(m_hCom, &WriteOverLap, &dw, FALSE);
			} else {
				// Write Event set
				if ( WriteStat == WRITE_OVERWAIT )
					HandleTab[HandleCount++] = WriteOverLap.hEvent;
				else if ( WriteStat == WRITE_EVENTWAIT )
					HandleTab[HandleCount++] = GetEvent(FIFO_STDIN)->m_hObject;
				else if ( WriteStat == WRITE_TIMERWAIT ) {
					if ( WriteTimerId == 0 ) {
						if ( (len = (int)(WriteReqClock + (clock_t)WriteWaitMsec * CLOCKS_PER_SEC / 1000 - clock())) <= 0 ) {
							WriteStat = WRITE_GETDATA;
							continue;
						}
						WriteTimerId = timeSetEvent((UINT)(len * 1000 / CLOCKS_PER_SEC), 0, (LPTIMECALLBACK)(WriteTimerEvent.m_hObject), 0, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET | TIME_KILL_SYNCHRONOUS);
					}
					HandleTab[HandleCount++] = WriteTimerEvent;
				}
				// WRITE_FLOWCTRLはWaitComEventで監視
				break;
			}

			if ( bRes ) {
				// WriteFile done
				if ( (WriteDone += dw) >= WriteByte )
					WriteStat = (WriteWaitMsec > 0 ? WRITE_TIMERWAIT : WRITE_GETDATA);
				else
					WriteStat = WRITE_HAVEDATA;
			} else if ( (dw = ::GetLastError()) == ERROR_IO_INCOMPLETE || dw == ERROR_IO_PENDING ) {
				// WriteFile to overlap
				WriteStat = WRITE_OVERWAIT;
			} else {
				// WriteFile error
				m_nLastError = (int)dw;
				goto ERRENDOF;
			}
		}

		//TRACE("ComThredProc ReadStat=%d, WriteStat=%d, ComStat=%d, WaitFor=%d, CommEvent=%04x, CommError=%04x\n",
		//	ReadStat, WriteStat, ComStat, HandleCount, ComEvent, pSock->m_CommError);

		// Wait For Event
		ASSERT(HandleCount > 0);

		HandleTab[HandleCount++] = m_AbortEvent;
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
		else if ( HandleTab[dw] == WriteTimerEvent.m_hObject ) {
			WriteStat = WRITE_GETDATA;
			WriteTimerId = 0;

		} else if ( HandleTab[dw] == CommOverLap.hEvent )
			ComStat = COM_OVERLAP;

		else if ( HandleTab[dw] == m_AbortEvent )
			goto ERRENDOF;

		else if ( HandleTab[dw] == m_MsgEvent ) {
			// Fifo Command
			m_MsgSemaphore.Lock();
			while ( !m_MsgList.IsEmpty() ) {
				FifoMsg *pFifoMsg = m_MsgList.RemoveHead();
				m_MsgSemaphore.Unlock();

				switch(pFifoMsg->cmd) {
				case FIFO_CMD_FDEVENTS:
					switch(pFifoMsg->msg) {
					case FD_CLOSE:
						m_nLastError = (int)pFifoMsg->buffer;
						Read(FIFO_STDIN, NULL, 0);
						break;
					}
					break;

				case FIFO_CMD_MSGQUEIN:
					bRes = FALSE;
					switch(pFifoMsg->param) {
					case FIFO_QCMD_SENDBREAK:
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDBREAK, opt);
						bRes = SetCommBreak(m_hCom);
						Sleep(pFifoMsg->msg);
						ClearCommBreak(m_hCom);
						break;
					case FIFO_QCMD_COMMSTATE:
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_COMMSTATE);
						if ( pSock->m_pComConf != NULL ) {
							bRes = SetCommState(m_hCom, &(pSock->m_pComConf->dcb));
							if ( pSock->m_pComConf->dcb.fDtrControl == DTR_CONTROL_DISABLE )
								EscapeCommFunction(m_hCom, CLRDTR);
							else if ( pSock->m_pComConf->dcb.fDtrControl == DTR_CONTROL_ENABLE )
								EscapeCommFunction(m_hCom, SETDTR);
							if ( pSock->m_pComConf->dcb.fDtrControl == RTS_CONTROL_DISABLE )
								EscapeCommFunction(m_hCom, CLRRTS);
							else if ( pSock->m_pComConf->dcb.fDtrControl == RTS_CONTROL_ENABLE )
								EscapeCommFunction(m_hCom, SETRTS);
							m_SendWait[0] = pSock->m_SendWait[0];
							m_SendWait[1] = pSock->m_SendWait[1];
							m_SendCrLf    = pSock->m_SendCrLf;
							bDsrSensitivity = (pSock->m_pComConf->dcb.fDsrSensitivity != 0 ? TRUE : FALSE);
							bOutxDsrFlow = (pSock->m_pComConf->dcb.fOutxDsrFlow != 0 ? TRUE : FALSE);
							bOutxCtsFlow = (pSock->m_pComConf->dcb.fOutxCtsFlow != 0 ? TRUE : FALSE);
							ReadByte = CalcReadByte();
						}
						break;
					case FIFO_QCMD_SETDTRRTS:
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETDTRRTS, sw);
						bRes = EscapeCommFunction(m_hCom, (DWORD)pFifoMsg->msg);
						break;
					}
					if ( pFifoMsg->pResult != NULL )
						*(pFifoMsg->pResult) = bRes;
					break;
				}

				DeleteMsg(pFifoMsg);
				m_MsgSemaphore.Lock();
			}
			m_MsgSemaphore.Unlock();
		}
	}

ERRENDOF:
	SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)m_nLastError);
	Write(FIFO_STDOUT, NULL, 0);

	CancelIo(m_hCom);

	CloseHandle(ReadOverLap.hEvent);
	CloseHandle(WriteOverLap.hEvent);
	CloseHandle(CommOverLap.hEvent);
}

//////////////////////////////////////////////////////////////////////
// CComSock

CComSock::CComSock(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type     = ESCT_COMDEV;

	m_hCom     = INVALID_HANDLE_VALUE;
	m_ComPort  = (-1);
	m_pComConf = NULL;
	m_pComMoni = NULL;

	m_SendWait[0] = 0;
	m_SendWait[1] = 0;

	m_ComEvent    = 0;
	m_CommError   = 0;
	m_ModemStatus = 0;

	m_RecvByteSec  = 0;
	m_SendByteSec  = 0;

	m_fInXOutXMode = 0;

	SetRecvBufSize(COMBUFSIZE);
}
CComSock::~CComSock()
{
	Close();

	if ( m_pComConf != NULL )
		delete m_pComConf;
}
void CComSock::FifoLink()
{
	FifoUnlink();

	ASSERT(m_pFifoLeft == NULL && m_pFifoMid == NULL && m_pFifoRight == NULL);

	m_pFifoLeft   = new CFifoCom(m_pDocument, this);
	m_pFifoMid    = new CFifoWnd(m_pDocument, this);
	m_pFifoRight  = new CFifoDocument(m_pDocument, this);

	CFifoBase::MidLink(m_pFifoLeft, FIFO_STDIN, m_pFifoMid, FIFO_STDIN, m_pFifoRight, FIFO_STDIN);
}
BOOL CComSock::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
	int n;
	COMMTIMEOUTS ComTime;

	if ( m_hCom != INVALID_HANDLE_VALUE )
		return FALSE;

	FifoLink();

	if ( !LoadComConf(lpszHostAddress, nHostPort, TRUE) ) {
		::AfxMessageBox(GetFormatErrorMessage(m_pDocument->m_ServerEntry.m_EntryName, lpszHostAddress, nHostPort, _T("GetCommConfig"), ::GetLastError()), MB_ICONSTOP);
		return FALSE;
	}

	if ( !SetCommState(m_hCom, &(m_pComConf->dcb)) ) {
		::AfxMessageBox(GetFormatErrorMessage(m_pDocument->m_ServerEntry.m_EntryName, lpszHostAddress, nHostPort, _T("SetCommState"), ::GetLastError()), MB_ICONSTOP);
		return FALSE;
	}

	SetupComm(m_hCom, COMQUEUESIZE, COMQUEUESIZE);
	PurgeComm(m_hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	if ( (n = 1000 * 2 / GetByteSec()) <= 0 )			// 1 Byte Read Time = msec * 2
		n = 1;

	GetCommTimeouts(m_hCom, &ComTime);
	ComTime.ReadIntervalTimeout         = (DWORD)n;
	ComTime.ReadTotalTimeoutMultiplier  = 0;
	ComTime.ReadTotalTimeoutConstant    = 0;
	ComTime.WriteTotalTimeoutMultiplier = 0;
	ComTime.WriteTotalTimeoutConstant   = 0;
	SetCommTimeouts(m_hCom, &ComTime);

	GetCommModemStatus(m_hCom, &m_ModemStatus);
	SetCommMask(m_hCom, EV_CTS | EV_DSR | EV_RLSD | EV_BREAK | EV_ERR | EV_RING);

	ASSERT(m_pFifoLeft != NULL && m_pFifoLeft->m_Type == FIFO_TYPE_COM);

	((CFifoCom *)m_pFifoLeft)->m_SendCrLf    = (m_pDocument->m_TextRam.m_SendCrLf == 0 ? '\r' : '\n');
	((CFifoCom *)m_pFifoLeft)->m_SendWait[0] = m_SendWait[0];
	((CFifoCom *)m_pFifoLeft)->m_SendWait[1] = m_SendWait[1];

	if ( m_pComConf->dcb.fDtrControl == DTR_CONTROL_DISABLE )
		EscapeCommFunction(m_hCom, CLRDTR);
	else if ( m_pComConf->dcb.fDtrControl == DTR_CONTROL_ENABLE )
		EscapeCommFunction(m_hCom, SETDTR);
	if ( m_pComConf->dcb.fDtrControl == RTS_CONTROL_DISABLE )
		EscapeCommFunction(m_hCom, CLRRTS);
	else if ( m_pComConf->dcb.fDtrControl == RTS_CONTROL_ENABLE )
		EscapeCommFunction(m_hCom, SETRTS);

	if ( !((CFifoCom *)m_pFifoLeft)->Open(m_hCom) )
		return FALSE;

	m_pFifoLeft->SendFdEvents(FIFO_STDOUT, FD_CONNECT, NULL);

	return TRUE;
}
void CComSock::Close()
{
	if ( m_hCom == INVALID_HANDLE_VALUE )
		return;

	if ( m_pFifoLeft != NULL && m_pFifoLeft->m_Type == FIFO_TYPE_COM )
		((CFifoCom *)m_pFifoLeft)->Close();

	SetCommMask(m_hCom, 0);
	PurgeComm(m_hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	EscapeCommFunction(m_hCom, CLRDTR);
	EscapeCommFunction(m_hCom, CLRRTS);
	CloseHandle(m_hCom);
	m_hCom = NULL;

	if ( m_pComMoni != NULL )
		m_pComMoni->DestroyWindow();

	CExtSocket::Close();
}
void CComSock::SendBreak(int opt)
{
	if ( m_hCom == INVALID_HANDLE_VALUE )
		return;

	if ( m_pFifoLeft != NULL && m_pFifoLeft->m_Type == FIFO_TYPE_COM )
		m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDBREAK, (opt != 0 ? 300 : 100));
}

int CComSock::GetByteSec()
{
	int bits;

	if ( m_pComConf == NULL )
		return 960;							// Dumy 9600bps

	bits = 10;								// Start bit 1 * 10

	bits += m_pComConf->dcb.ByteSize * 10;	// Data bits * 10

	if ( m_pComConf->dcb.Parity != 0 )		// !NonParty ? 1 * 10
		bits += 10;

	switch(m_pComConf->dcb.StopBits) {
	case 0: bits += 10; break;				// 1 Stop bits * 10
	case 1: bits += 15; break;				// 1.5 Stop bits * 10
	case 2: bits += 20; break;				// 2 Stop bits * 10
	}

	return (m_pComConf->dcb.BaudRate * 10 / bits);	// Byte/Sec
}
BOOL CComSock::SetComConf()
{
	if ( m_hCom == INVALID_HANDLE_VALUE || m_pComConf == NULL || m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_COM )
		return FALSE;

	m_SendCrLf = (m_pDocument->m_TextRam.m_SendCrLf == 0 ? '\r' : '\n');

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_COMMSTATE);
}
void CComSock::SetXonXoff(int sw)
{
	if ( m_hCom == INVALID_HANDLE_VALUE || m_pComConf == NULL || m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_COM )
		return;

	if ( sw ) {		// OFF->ON
		if ( m_pComConf->dcb.fInX != 0 || m_pComConf->dcb.fOutX != 0 || m_fInXOutXMode == 0 )
			return;
		m_pComConf->dcb.fInX  = m_fInXOutXMode & 1;
		m_pComConf->dcb.fOutX = (m_fInXOutXMode >> 4) & 1;
		m_fInXOutXMode = 0;
	} else {		// ON->OFF
		if ( m_pComConf->dcb.fInX == 0 && m_pComConf->dcb.fOutX == 0 )
			return;
		m_fInXOutXMode = m_pComConf->dcb.fInX | (m_pComConf->dcb.fOutX << 4);
		m_pComConf->dcb.fInX  = 0;
		m_pComConf->dcb.fOutX = 0;
	}

	SetComConf();
}
BOOL CComSock::SetComCtrl(DWORD ctrl)
{
	if ( m_hCom == INVALID_HANDLE_VALUE || m_pComConf == NULL || m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_COM )
		return FALSE;

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETDTRRTS, ctrl);
}
void CComSock::GetRecvSendByteSec(int &recvByte, int &sendByte)
{
	recvByte = m_RecvByteSec;
	sendByte = m_SendByteSec;

	m_RecvByteSec = m_SendByteSec = 0;
}

void CComSock::GetStatus(CString &str)
{
	CString tmp;

	CExtSocket::GetStatus(str);

	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_COM )
		return;

	str += _T("\r\n");

	tmp.Format(_T("Receive Buffer: %d Bytes\r\n"), m_pFifoLeft->GetDataSize(FIFO_STDOUT));
	str += tmp;

	tmp.Format(_T("Send Buffer: %d Bytes\r\n"), m_pFifoLeft->GetDataSize(FIFO_STDIN));
	str += tmp;

	tmp.Format(_T("Thread Status: %d\r\n"), ((CFifoCom *)m_pFifoLeft)->m_ThreadMode);
	str += tmp;

	str += _T("\r\n");

	tmp.Format(_T("Open Device: COM%d\r\n"), m_ComPort);
	str += tmp;

	str += _T("Configuration: ");
	SaveComConf(tmp);
	str += tmp;
	str += _T("\r\n");

	tmp.Format(_T("Modem Status: CTS(%s) DSR(%s) RING(%s) RLSD(%s)\r\n"),
		((m_ModemStatus & MS_CTS_ON)  != 0 ? _T("ON") : _T("OFF")),
		((m_ModemStatus & MS_DSR_ON)  != 0 ? _T("ON") : _T("OFF")),
		((m_ModemStatus & MS_RING_ON) != 0 ? _T("ON") : _T("OFF")),
		((m_ModemStatus & MS_RLSD_ON) != 0 ? _T("ON") : _T("OFF")));
	str += tmp;

	str += _T("Error Status:");
	if ( m_CommError == 0 ) str += _T(" No Error");
	else {
		if ( (m_CommError & CE_BREAK)    != 0 ) str += _T(" Received Break,");
		if ( (m_CommError & CE_FRAME)    != 0 ) str += _T(" Frame Error,");
		if ( (m_CommError & CE_IOE)      != 0 ) str += _T(" I/O Error,");
		if ( (m_CommError & CE_OVERRUN)  != 0 ) str += _T(" Overrun Error,");
		if ( (m_CommError & CE_RXOVER)   != 0 ) str += _T(" Receive Overflow,");
		if ( (m_CommError & CE_RXPARITY) != 0 ) str += _T(" Parity Error,");
		if ( (m_CommError & CE_TXFULL)   != 0 ) str += _T(" Send Buffer Full,");
	}
	str += _T("\r\n");
}
void CComSock::ComMoniter()
{
	if ( m_pComMoni == NULL ) {
		m_pComMoni = new CComMoniDlg(this);
		m_pComMoni->Create(IDD_COMMONIDLG, CWnd::GetDesktopWindow());
		m_pComMoni->ShowWindow(SW_SHOW);
	} else
		m_pComMoni->DestroyWindow();
}

static struct _ComTab {
		LPCTSTR	name;
		int		mode;
		int		value;
	} ComTab[] = {
		{	_T("75"),		1,	75			},
		{	_T("110"),		1,	CBR_110		},
		{	_T("150"),		1,	150		},
		{	_T("300"),		1,	CBR_300		},
		{	_T("600"),		1,	CBR_600		},
		{	_T("1200"),		1,	CBR_1200	},
		{	_T("2400"),		1,	CBR_2400	},
		{	_T("4800"),		1,	CBR_4800	},
		{	_T("9600"),		1,	CBR_9600	},
		{	_T("14400"),	1,	CBR_14400	},
		{	_T("19200"),	1,	CBR_19200	},
		{	_T("28800"),	1,	28800		},
		{	_T("38400"),	1,	CBR_38400	},
		{	_T("56000"),	1,	CBR_56000	},
		{	_T("57600"),	1,	CBR_57600	},
		{	_T("115200"),	1,	CBR_115200	},
		{	_T("128000"),	1,	CBR_128000	},
		{	_T("230400"),	1,	230400		},
		{	_T("256000"),	1,	CBR_256000	},
		{	_T("460800"),	1,	460800		},
		{	_T("512000"),	1,	512000		},
		{	_T("921600"),	1,	921600		},

		{	_T("5"),		2,	5			},
		{	_T("6"),		2,	6			},
		{	_T("7"),		2,	7			},
		{	_T("8"),		2,	8			},

		{	_T("NOP"),		3,	NOPARITY	},
		{	_T("ODD"),		3,	ODDPARITY	},
		{	_T("EVEN"),		3,	EVENPARITY	},
		{	_T("MARK"),		3,	MARKPARITY	},
		{	_T("SPC"),		3,	SPACEPARITY	},

		{	_T("1"),		4,	ONESTOPBIT		},
		{	_T("1.5"),		4,	ONE5STOPBITS	},
		{	_T("2"),		4,	TWOSTOPBITS		},

		{	_T("NOC"),		5,	COMFLOW_NONE	},
		{	_T("CTS"),		5,	COMFLOW_RTSCTS	},
		{	_T("XON"),		5,	COMFLOW_XONXOFF	},

		{	NULL,			0,	0			},
	};

void CComSock::SetDcdParam(CStringArrayExt &param)
{
	int n, i;

	for ( n = 0 ; n < param.GetSize() ; n++ ) {
		for ( i = 0 ; ComTab[i].name != NULL ; i++ ) {
			if ( param[n].Compare(ComTab[i].name) == 0 ) {
				switch(ComTab[i].mode) {
				case 1: m_pComConf->dcb.BaudRate = ComTab[i].value; break;
				case 2: m_pComConf->dcb.ByteSize = ComTab[i].value; break;
				case 3: m_pComConf->dcb.Parity   = ComTab[i].value; break;
				case 4: m_pComConf->dcb.StopBits = ComTab[i].value; break;
				case 5: SetFlowCtrlMode(&(m_pComConf->dcb), ComTab[i].value, NULL); break;
				}
				break;
			}
		}
		if ( ComTab[i].name != NULL )
			continue;

		if ( _tcsncmp(param[n], _T("USER="), 5) == 0 ) {
			SetFlowCtrlMode(&(m_pComConf->dcb), COMFLOW_USERDEF, (LPCTSTR)param[n] + 5);

		} else if ( _tcsncmp(param[n], _T("XOFFL="), 6) == 0 ) {
			m_pComConf->dcb.XoffLim = COMQUEUESIZE - COMQUEUESIZE * _tstoi((LPCTSTR)param[n] + 6) / 100;
			if ( m_pComConf->dcb.XoffLim < (COMQUEUESIZE - COMQUEUESIZE * 95 / 100) )
				m_pComConf->dcb.XoffLim = (COMQUEUESIZE - COMQUEUESIZE * 95 / 100);
			else if ( m_pComConf->dcb.XoffLim > (COMQUEUESIZE - COMQUEUESIZE * 50 / 100) )
				m_pComConf->dcb.XoffLim = (COMQUEUESIZE - COMQUEUESIZE * 50 / 100);

		} else if ( _tcsncmp(param[n], _T("XONL="), 5) == 0 ) {
			m_pComConf->dcb.XonLim = COMQUEUESIZE * _tstoi((LPCTSTR)param[n] + 5) / 100;
			if ( m_pComConf->dcb.XonLim > (COMQUEUESIZE * 50 / 100) )
				m_pComConf->dcb.XonLim = (COMQUEUESIZE * 50 / 100);
			else if ( m_pComConf->dcb.XonLim < (COMQUEUESIZE * 5 / 100) )
				m_pComConf->dcb.XonLim = (COMQUEUESIZE * 5 / 100);

		} else if ( _tcsncmp(param[n], _T("XONC="), 5) == 0 ) {
			m_pComConf->dcb.XonChar = (char)_tstoi((LPCTSTR)param[n] + 5);
		} else if ( _tcsncmp(param[n], _T("XOFFC="), 6) == 0 ) {
			m_pComConf->dcb.XoffChar = (char)_tstoi((LPCTSTR)param[n] + 6);

		} else if ( _tcsncmp(param[n], _T("WC="), 3) == 0 ) {
			m_SendWait[0] = _tstoi((LPCTSTR)param[n] + 3);
		} else if ( _tcsncmp(param[n], _T("WL="), 3) == 0 ) {
			m_SendWait[1] = _tstoi((LPCTSTR)param[n] + 3);

		} else if ( (i = _tstoi(param[n])) >= 100 ) {
			m_pComConf->dcb.BaudRate = i;
		}
	}
}
BOOL CComSock::LoadComConf(LPCTSTR ComSetStr, int ComPort, BOOL bOpen)
{
	int n;
	DWORD sz = sizeof(COMMCONFIG);
	BYTE *temp = new BYTE[sz];
	CStringArrayExt param;
	HANDLE hCom = INVALID_HANDLE_VALUE;
	CString ComName;

	ZeroMemory(temp, sz);

	// ComSetStrをNULLでデバイスの規定値を取得
	if ( ComSetStr != NULL )
		param.GetString(ComSetStr, _T(';'));

	// ComSetStr内のCOM設定を優先（古いバージョン互換の為）
	for ( n = 0 ; n < param.GetSize() ; n++ ) {
		if ( _tcsnicmp(param[n], _T("COM"), 3) == 0 ) {
			ComPort = _tstoi((LPCTSTR)param[n] + 3);
			break;
		}
	}

	// CExtSocket::GetPortNum()では、マイナス値を返す！
	if ( ComPort < 0 )
		ComPort = 0 - ComPort;
	else if ( ComPort == 0 ) {
		::SetLastError(ERROR_PATH_NOT_FOUND);
		return FALSE;
	}
	m_ComPort = ComPort;

	// GetDefaultCommConfigは、\\.\COMnを使用しない
	ComName.Format(_T("COM%d"), ComPort);
	if ( !GetDefaultCommConfig(ComName, (LPCOMMCONFIG)temp, &sz) ) {
		if ( ComPort >= 10 )
			ComName.Format(_T("\\\\.\\COM%d"), ComPort);

		if ( (hCom = CreateFile(ComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE )
			return FALSE;

		if ( !GetCommConfig(hCom, (LPCOMMCONFIG)temp, &sz) ) {
			::CloseHandle(hCom);
			return FALSE;
		}
	}

	// ReSize COMMCONFIG ?
	if ( sz > sizeof(COMMCONFIG) ) {
		delete [] temp;
		temp = new BYTE[sz];
		ZeroMemory(temp, sz);

		if ( hCom == INVALID_HANDLE_VALUE ) {
			if ( !GetDefaultCommConfig(ComName, (LPCOMMCONFIG)temp, &sz) )
				return FALSE;
		} else  if ( !GetCommConfig(hCom, (LPCOMMCONFIG)temp, &sz) ) {
			::CloseHandle(hCom);
			return FALSE;
		}
	}

	if ( bOpen ) { 
		if ( hCom == INVALID_HANDLE_VALUE ) {
			if ( ComPort >= 10 )
				ComName.Format(_T("\\\\.\\COM%d"), ComPort);
			if ( (m_hCom = CreateFile(ComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE )
				return FALSE;
		} else
			m_hCom = hCom;
	} else if ( hCom != INVALID_HANDLE_VALUE )
		::CloseHandle(hCom);

	if ( m_pComConf != NULL )
		delete m_pComConf;
	m_pComConf = (COMMCONFIG *)temp;

	// Default DCB Parameter

	//	XoffLim	受信バッファの空き容量が XoffLim バイト未満になると 
	//			XoffChar が送信され、 受信不可能であることを示します。 
	m_pComConf->dcb.XoffLim  = COMQUEUESIZE - COMQUEUESIZE * 75 / 100;

	//	XonLim	受信バッファのデータが XonLim バイト以下になると 
	//			XonChar が送信され、 受信可能であることを示します。  
	m_pComConf->dcb.XonLim   = COMQUEUESIZE * 25 / 100;

	m_pComConf->dcb.fAbortOnError = 0;	/* Abort all reads and writes on Error */
	m_pComConf->dcb.XonChar  = 0x11;	/* Tx and Rx X-ON character        */
	m_pComConf->dcb.XoffChar = 0x13;	/* Tx and Rx X-OFF character       */
	m_pComConf->dcb.ErrorChar = 0;		/* Error replacement char          */
    m_pComConf->dcb.EofChar = 0;		/* End of Input character          */
    m_pComConf->dcb.EvtChar = 0;		/* Received Event character        */

	m_SendWait[0] = 0;
	m_SendWait[1] = 0;

	SetDcdParam(param);

	return TRUE;
}
BOOL CComSock::SaveComConf(CString &str)
{
	int n, r;
	int b = 0;
	CString tmp;

	str.Empty();

	if ( m_pComConf == NULL )
		return FALSE;

	for ( n = 0 ; ComTab[n].name != NULL ; n++ ) {
		switch(ComTab[n].mode) {
		case 1: r = (m_pComConf->dcb.BaudRate == ComTab[n].value ? TRUE : FALSE); break;
		case 2: r = (m_pComConf->dcb.ByteSize == ComTab[n].value ? TRUE : FALSE); break;
		case 3: r = (m_pComConf->dcb.Parity   == ComTab[n].value ? TRUE : FALSE); break;
		case 4: r = (m_pComConf->dcb.StopBits == ComTab[n].value ? TRUE : FALSE); break;
		}
		if ( r == TRUE ) {
			str += ComTab[n].name;
			str += _T(";");
			b |= (1 << ComTab[n].mode);
		}
	}

	if ( (b & (1 << 1)) == 0 ) {
		tmp.Format(_T("%d;"), m_pComConf->dcb.BaudRate);
		str += tmp;
	}

	n = GetFlowCtrlMode(&(m_pComConf->dcb), tmp);
	switch(n) {
	case COMFLOW_NONE:		str += _T("NOC;"); break;
	case COMFLOW_RTSCTS:	str += _T("CTS;"); break;
	case COMFLOW_XONXOFF:	str += _T("XON;"); break;
	case COMFLOW_USERDEF:	str += _T("USER="); str += tmp; str += _T(";"); break;
	}

	if ( m_pComConf->dcb.XoffLim != (COMQUEUESIZE - COMQUEUESIZE * 75 / 100) ) {
		tmp.Format(_T("XOFFL=%d;"), 100 - 100 * m_pComConf->dcb.XoffLim / COMQUEUESIZE);
		str += tmp;
	}
	if ( m_pComConf->dcb.XonLim != (COMQUEUESIZE * 25 / 100) ) {
		tmp.Format(_T("XONL=%d;"), m_pComConf->dcb.XonLim * 100 / COMQUEUESIZE);
		str += tmp;
	}
	if ( m_pComConf->dcb.XonChar != 0x11 ) {
		tmp.Format(_T("XONC=%d;"), m_pComConf->dcb.XonChar);
		str += tmp;
	}
	if ( m_pComConf->dcb.XoffChar != 0x13 ) {
		tmp.Format(_T("XOFFC=%d;"), m_pComConf->dcb.XoffChar);
		str += tmp;
	}

	if ( m_SendWait[0] != 0 ) {
		tmp.Format(_T("WC=%d;"), m_SendWait[0]);
		str += tmp;
	}
	if ( m_SendWait[1] != 0 ) {
		tmp.Format(_T("WL=%d;"), m_SendWait[1]);
		str += tmp;
	}

	return TRUE;
}
BOOL CComSock::SetupComConf(CString &ComSetStr, int &ComPort, CWnd *pOwner)
{
	CComInitDlg dlg(pOwner);

	if ( !LoadComConf(ComSetStr, ComPort) ) {
		ThreadMessageBox(_T("COM%d Load Config Error '%s'"), ComPort, ComSetStr);
		return FALSE;
	}

	dlg.m_pSock = this;

	if ( dlg.DoModal() != IDOK )
		return FALSE;

	if ( !SaveComConf(ComSetStr) )
		return FALSE;

	if ( ComPort != m_ComPort )
		ComPort = m_ComPort;

	return TRUE;
}

void CComSock::SetFlowCtrlMode(DCB *pDCB, int FlowCtrl, LPCTSTR UserDef)
{
	switch(FlowCtrl) {
	case COMFLOW_NONE:
		pDCB->fDtrControl		= DTR_CONTROL_ENABLE;
		pDCB->fOutxDsrFlow		= FALSE;
		pDCB->fDsrSensitivity	= FALSE;
		pDCB->fTXContinueOnXoff	= FALSE;
		pDCB->fRtsControl		= RTS_CONTROL_ENABLE;
		pDCB->fOutxCtsFlow		= FALSE;
		pDCB->fInX				= FALSE;
		pDCB->fOutX				= FALSE;
		break;
	case COMFLOW_RTSCTS:
		pDCB->fDtrControl		= DTR_CONTROL_ENABLE;
		pDCB->fOutxDsrFlow		= FALSE;
		pDCB->fDsrSensitivity	= FALSE;
		pDCB->fTXContinueOnXoff	= FALSE;
		pDCB->fRtsControl		= RTS_CONTROL_HANDSHAKE;
		pDCB->fOutxCtsFlow		= TRUE;
		pDCB->fInX				= FALSE;
		pDCB->fOutX				= FALSE;
		break;
	case COMFLOW_XONXOFF:
		pDCB->fDtrControl		= DTR_CONTROL_ENABLE;
		pDCB->fOutxDsrFlow		= FALSE;
		pDCB->fDsrSensitivity	= FALSE;
		pDCB->fTXContinueOnXoff	= FALSE;
		pDCB->fRtsControl		= RTS_CONTROL_ENABLE;
		pDCB->fOutxCtsFlow		= FALSE;
		pDCB->fInX				= TRUE;
		pDCB->fOutX				= TRUE;
		break;
	case COMFLOW_USERDEF:
		if ( UserDef != NULL ) {
			LPCTSTR p = UserDef;
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fDtrControl			= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fOutxDsrFlow		= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fDsrSensitivity		= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fTXContinueOnXoff	= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fRtsControl			= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fOutxCtsFlow		= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fInX				= *(p++) - _T('0');
			if ( *p >= _T('0') && *p <= _T('9') ) pDCB->fOutX				= *(p++) - _T('0');
		}
		break;
	}

}
int CComSock::GetFlowCtrlMode(DCB *pDCB, CString &UserDef)
{
	if (	pDCB->fDtrControl		== DTR_CONTROL_ENABLE &&
			pDCB->fOutxDsrFlow		== FALSE &&
			pDCB->fDsrSensitivity	== FALSE &&
			pDCB->fTXContinueOnXoff	== FALSE &&
			pDCB->fRtsControl		== RTS_CONTROL_ENABLE &&
			pDCB->fOutxCtsFlow		== FALSE &&
			pDCB->fInX				== FALSE &&
			pDCB->fOutX				== FALSE )
		return COMFLOW_NONE;
	else if (
			pDCB->fDtrControl		== DTR_CONTROL_ENABLE &&
			pDCB->fOutxDsrFlow		== FALSE &&
			pDCB->fDsrSensitivity	== FALSE &&
			pDCB->fTXContinueOnXoff	== FALSE &&
			pDCB->fRtsControl		== RTS_CONTROL_HANDSHAKE &&
			pDCB->fOutxCtsFlow		== TRUE &&
			pDCB->fInX				== FALSE &&
			pDCB->fOutX				== FALSE )
		return COMFLOW_RTSCTS;
	else if (
			pDCB->fDtrControl		== DTR_CONTROL_ENABLE &&
			pDCB->fOutxDsrFlow		== FALSE &&
			pDCB->fDsrSensitivity	== FALSE &&
			pDCB->fTXContinueOnXoff	== FALSE &&
			pDCB->fRtsControl		== RTS_CONTROL_ENABLE &&
			pDCB->fOutxCtsFlow		== FALSE &&
			pDCB->fInX				== TRUE &&
			pDCB->fOutX				== TRUE )
		return COMFLOW_XONXOFF;
	else {
		UserDef.Format(_T("%c%c%c%c%c%c%c%c"),
			pDCB->fDtrControl		+ _T('0'),
			pDCB->fOutxDsrFlow		+ _T('0'),
			pDCB->fDsrSensitivity	+ _T('0'),
			pDCB->fTXContinueOnXoff	+ _T('0'),
			pDCB->fRtsControl		+ _T('0'),
			pDCB->fOutxCtsFlow		+ _T('0'),
			pDCB->fInX				+ _T('0'),
			pDCB->fOutX				+ _T('0'));
		return COMFLOW_USERDEF;
	}
}
void CComSock::AliveComPort(BYTE pComAliveBits[COMALIVEBYTE])
{
	int n;
	TCHAR *p;
	int ReTry = 4;	// 64,128,256...
	int nBufLen = (64 * 1024);
	TCHAR *pDevBuf = new TCHAR[nBufLen];
	COMMCONFIG conf;
	CString name;

	ZeroMemory(pComAliveBits, COMALIVEBYTE);

RETRY:
	if ( QueryDosDevice(NULL, pDevBuf, nBufLen) > 0 ) {
		for ( p = pDevBuf ; *p != _T('\0') ; ) {
			if ( _tcsnicmp(p, _T("COM"), 3) == 0 && (n = _tstoi(p + 3)) > 0 && n < COMALIVEBITS )
				pComAliveBits[n / 8] |= (1 << (n % 8));
			while ( *(p++) != _T('\0') );
		}

	} else if ( --ReTry > 0 && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
		nBufLen *= 2;
		delete pDevBuf;
		pDevBuf = new TCHAR[nBufLen];
		goto RETRY;

	} else {
		for ( n = 1 ; n < 10 ; n++ ) {
			name.Format(_T("COM%d"), n);
			DWORD sz = sizeof(COMMCONFIG);
			if ( GetDefaultCommConfig(name, &conf, &sz) && n < COMALIVEBITS )
				pComAliveBits[n / 8] |= (1 << (n % 8));
		}
	}

	delete pDevBuf;
}
