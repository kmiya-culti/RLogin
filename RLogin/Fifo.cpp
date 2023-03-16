///////////////////////////////////////////////////////
// Fifo.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "Fifo.h"

#include <openssl/ssl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////
// CFifoBuffer

CFifoBuffer::CFifoBuffer()
{
	m_nBufTop = m_nBufBtm = 0;
	m_nBufMax = FIFO_BUFDEFMAX;
	m_pBuffer = new BYTE[m_nBufMax];

	m_nReadNumber = m_nWriteNumber = (-1);
	m_pReadBase = m_pWriteBase = NULL;
	m_bEndOf = FALSE;
}
CFifoBuffer::~CFifoBuffer()
{
	delete [] m_pBuffer;
}

int CFifoBuffer::PutBuffer(LPBYTE pBuffer, int nBufLen)
{
	if ( (m_nBufBtm + nBufLen) > m_nBufMax ) {
		if ( (m_nBufMax - GetSize()) >= nBufLen ) {
			memmove(m_pBuffer, GetTopPtr(), GetSize());
			m_nBufBtm -= m_nBufTop;
			m_nBufTop = 0;

		} else {
			int n;
			for ( n = m_nBufMax * 2 ; n < (m_nBufBtm - m_nBufTop + nBufLen) ; )
				n *= 2;
			BYTE *p = new BYTE[n];

			if ( m_nBufTop < m_nBufBtm ) {
				memcpy(p, GetTopPtr(), GetSize());
				m_nBufBtm = GetSize();
				m_nBufTop = 0;
			} else
				m_nBufTop = m_nBufBtm = 0;

			delete [] m_pBuffer;
			m_pBuffer = p;
			m_nBufMax = n;
		}
	}

	if ( pBuffer == NULL )
		m_bEndOf = TRUE;
	else {
		ASSERT((m_nBufBtm + nBufLen) <= m_nBufMax);

		memcpy(GetBtmPtr(), pBuffer, nBufLen);
		m_nBufBtm += nBufLen;
	}

	ASSERT(m_pReadBase != NULL && m_nReadNumber >= 0);
	m_pReadBase->FifoEvents(m_nReadNumber, this, FD_READ, NULL);

	return nBufLen;
}
int CFifoBuffer::GetBuffer(LPBYTE pBuffer, int nBufLen)
{
	int n;

	if ( pBuffer == NULL ) {
		m_bEndOf = TRUE;
		n = (-1);

	} else if ( (n = GetSize()) > 0 ) {
		if ( n > nBufLen )
			n = nBufLen;
		memcpy(pBuffer, m_pBuffer + m_nBufTop, n);
		m_nBufTop += n;
	}

	ASSERT(m_pWriteBase != NULL && m_nWriteNumber >= 0);
	m_pWriteBase->FifoEvents(m_nWriteNumber, this, FD_WRITE, NULL);

	return n;
}
int CFifoBuffer::ResBuffer(LPBYTE pBuffer, int nBufLen)
{
	int n;

	if ( (n = GetSize()) > 0 ) {
		if ( n > nBufLen )
			n = nBufLen;
		memcpy(pBuffer, m_pBuffer + m_nBufTop, n);
	}

	return n;
}
int CFifoBuffer::Consume(int nBufLen)
{
	int len;

	if ( nBufLen > GetSize() )
		nBufLen = GetSize();

	m_nBufTop += nBufLen;
	len = GetSize();

	ASSERT(m_pWriteBase != NULL && m_nWriteNumber >= 0);
	m_pWriteBase->FifoEvents(m_nWriteNumber, this, FD_WRITE, NULL);

	return (len == 0 && m_bEndOf ? (-1) : len);
}
int CFifoBuffer::BackBuffer(LPBYTE pBuffer, int nBufLen)
{
	if ( m_nBufTop < nBufLen ) {
		if ( (m_nBufMax - GetSize()) >= nBufLen ) {
			memmove(m_pBuffer + nBufLen, GetTopPtr(), GetSize());
			m_nBufBtm = m_nBufBtm - m_nBufTop + nBufLen;
			m_nBufTop = nBufLen;
		} else {
			int n;
			for ( n = m_nBufMax * 2 ; n < (m_nBufBtm - m_nBufTop + nBufLen) ; )
				n *= 2;
			BYTE *p = new BYTE[n];

			if ( m_nBufTop < m_nBufBtm ) {
				memcpy(p + nBufLen, GetTopPtr(), GetSize());
				m_nBufBtm = m_nBufBtm - m_nBufTop + nBufLen;
				m_nBufTop = nBufLen;
			} else
				m_nBufTop = m_nBufBtm = nBufLen;

			delete [] m_pBuffer;
			m_pBuffer = p;
			m_nBufMax = n;
		}
	}

	ASSERT(m_nBufTop >= nBufLen && m_nBufBtm <= m_nBufMax);

	m_nBufTop -= nBufLen;
	memcpy(GetTopPtr(), pBuffer, nBufLen);

	ASSERT(m_pReadBase != NULL && m_nReadNumber >= 0);
	m_pReadBase->FifoEvents(m_nReadNumber, this, FD_READ, NULL);

	return nBufLen;
}

///////////////////////////////////////////////////////
// CFifoBase

CFifoBase::CFifoBase(class CRLoginDoc *pDoc, class CExtSocket *pSock)
{
	m_Type = FIFO_TYPE_BASE;
	m_pDocument = pDoc;
	m_pSock = pSock;
	m_nLastError = 0;
	m_nBufSize = FIFO_BUFSIZE;
	m_nLimitSize = 0;
	m_bFlowCtrl = TRUE;
	m_nBufMax = 0;
	m_pBuffer = NULL;
	m_bMsgClosed = FALSE;
}
CFifoBase::~CFifoBase()
{
	int n;
	CFifoBuffer *pFifo;

	for ( n = 0 ; n < m_FifoBuf.GetSize() ; n++ ) {
		if ( (pFifo = (CFifoBuffer *)m_FifoBuf[n]) == NULL )
			continue;
		pFifo->Lock();
		if ( pFifo->m_pReadBase == this )
			pFifo->m_pWriteBase->SetFifo(pFifo->m_nWriteNumber, NULL);
		else if ( pFifo->m_pWriteBase == this )
			pFifo->m_pReadBase->SetFifo(pFifo->m_nReadNumber, NULL);
		pFifo->Unlock();

		delete pFifo;
	}
	m_FifoBuf.RemoveAll();

	for ( n = 0 ; n < m_EventTab.GetSize() ; n++ )
		delete (CEvent *)m_EventTab[n];
	m_EventTab.RemoveAll();

	RemoveAllMsg();

	if ( m_pBuffer != NULL )
		delete [] m_pBuffer;
}

///////////////////////////////////////////////////////
// CFifoBase	virtual

BOOL CFifoBase::IsOpen()
{
	return TRUE;
}
void CFifoBase::FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam)
{
}
void CFifoBase::SendCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	PostCommand(cmd, param, msg, len, buf, pEvent, pResult);
}
void CFifoBase::Destroy()
{
	delete this;
}
void CFifoBase::OnLinked(int nFd, BOOL bMid)
{
}
void CFifoBase::OnUnLinked(int nFd, BOOL bMid)
{
}

///////////////////////////////////////////////////////
// CFifoBase	public

void CFifoBase::ChkFifoSize(int nFd)
{
	while ( nFd >= (int)m_FifoBuf.GetSize() ) {
		m_FifoBuf.Add(NULL);
		m_EventTab.Add(new CEvent);
		m_fdAllowEvents.Add(FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
		m_fdPostEvents.Add(0);
	}
}

void CFifoBase::SetFifo(int nFd, CFifoBuffer *pFifo)
{
	ChkFifoSize(nFd);

	if ( (m_FifoBuf[nFd] = pFifo) == NULL ) {
		((CEvent *)m_EventTab[nFd])->ResetEvent();
		m_fdAllowEvents[nFd] = (FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
		m_fdPostEvents[nFd] = 0;
	}
}
CFifoBuffer *CFifoBase::GetFifo(int nFd)
{
	CFifoBuffer *pFifo;

	Lock();
	ChkFifoSize(nFd);
	if ( (pFifo = (CFifoBuffer *)m_FifoBuf[nFd]) == NULL ) {
		Unlock();
		return NULL;
	}
	pFifo->Lock();
	Unlock();

	return pFifo;
}
void CFifoBase::RelFifo(CFifoBuffer *pFifo)
{
	pFifo->Unlock();
}

void CFifoBase::SendFdEvents(int nFd, int msg, void *pParam)
{
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( pFifo->m_pReadBase != this )
			pFifo->m_pReadBase->FifoEvents(pFifo->m_nReadNumber, pFifo, msg, pParam);
		if ( pFifo->m_pWriteBase != this )
			pFifo->m_pWriteBase->FifoEvents(pFifo->m_nWriteNumber, pFifo, msg, pParam);
		RelFifo(pFifo);
	}
}
BOOL CFifoBase::PostCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	BOOL bRet;

	FifoMsg *pFifoMsg = new FifoMsg;

	pFifoMsg->cmd     = cmd;
	pFifoMsg->param   = param;
	pFifoMsg->msg     = msg;
	pFifoMsg->pEvent  = pEvent;
	pFifoMsg->pResult = pResult;

	if ( buf != NULL && len > 0 ) {
		pFifoMsg->len = len;
		pFifoMsg->buffer = new BYTE[len];
		memcpy(pFifoMsg->buffer, buf, len);
	} else {
		pFifoMsg->len = 0;
		pFifoMsg->buffer = (BYTE *)buf;
	}

	m_MsgSemaphore.Lock();
	if ( m_MsgList.IsEmpty() )
		m_MsgEvent.SetEvent();
	m_MsgList.AddTail(pFifoMsg);
	bRet = (m_bMsgClosed ? FALSE : TRUE);
	m_MsgSemaphore.Unlock();

	return bRet;
}
BOOL CFifoBase::SendCmdWait(int cmd, int param, int msg, int len, void *buf)
{
	CEvent waitEvent;
	BOOL Result = FALSE;

	if ( !IsOpen() )
		return FALSE;

	switch(m_Type) {
	case FIFO_TYPE_BASE:
	case FIFO_TYPE_WINDOW:
	case FIFO_TYPE_SYNC:
		SendCommand(cmd, param, msg, len, buf);
		break;

	case FIFO_TYPE_THREAD:
	case FIFO_TYPE_EVENT:
	case FIFO_TYPE_SOCKET:
	case FIFO_TYPE_LISTEN:
	case FIFO_TYPE_PIPE:
	case FIFO_TYPE_COM:
		SendCommand(cmd, param, msg, len, buf, &waitEvent, &Result);
		WaitForSingleObject(waitEvent, INFINITE);
		break;
	}

	return Result;
}
void CFifoBase::SendFdCommand(int nFd, int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( pFifo->m_pReadBase != this )
			pFifo->m_pReadBase->SendCommand(cmd, param, msg, len, buf, pEvent, pResult);
		if ( pFifo->m_pWriteBase != this )
			pFifo->m_pWriteBase->SendCommand(cmd, param, msg, len, buf, pEvent, pResult);
		RelFifo(pFifo);
	}
}
void CFifoBase::SetFdEvents(int nFd, DWORD fdEvent)
{
	CFifoBuffer *pFifo;

	if ( (fdEvent == FD_READ) != 0 && (pFifo = GetFifo(nFd)) != NULL ) {
		m_fdAllowEvents[nFd] |= FD_READ;
		if ( pFifo->GetSize() > 0 || pFifo->m_bEndOf )
			FifoEvents(nFd, pFifo, FD_READ, NULL);
		else
			pFifo->m_pWriteBase->FifoEvents(pFifo->m_nWriteNumber, pFifo, FD_WRITE, NULL);
		RelFifo(pFifo);
	}

	if ( (fdEvent == FD_WRITE) != 0 && (pFifo = GetFifo(nFd)) != NULL ) {
		m_fdAllowEvents[nFd] |= FD_WRITE;
		if ( pFifo->GetSize() <= FIFO_BUFLOWER )
			FifoEvents(nFd, pFifo, FD_WRITE, NULL);
		else
			pFifo->m_pReadBase->FifoEvents(pFifo->m_nReadNumber, pFifo, FD_READ, NULL);
		RelFifo(pFifo);
	}
}
void CFifoBase::ResetFdEvents(int nFd, DWORD fdEvent)
{
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		m_fdAllowEvents[nFd] &= ~fdEvent; 
		RelFifo(pFifo);
	}
}
BOOL CFifoBase::IsFdEvents(int nFd, DWORD fdEvent)
{
#if 1
	return ((m_fdAllowEvents[nFd] & fdEvent) != 0);
#else
	BOOL bAllow = FALSE;
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		bAllow = ((m_fdAllowEvents[nFd] & fdEvent) != 0 ? TRUE : FALSE);
		RelFifo(pFifo);
	}
	return bAllow;
#endif
}

void CFifoBase::DeleteMsg(FifoMsg *pFifoMsg)
{
	if ( pFifoMsg->pEvent != NULL )
		pFifoMsg->pEvent->SetEvent();

	if ( pFifoMsg->len > 0 && pFifoMsg->buffer != NULL )
		delete [] pFifoMsg->buffer;

	delete pFifoMsg;
}
void CFifoBase::RemoveAllMsg()
{
	m_MsgSemaphore.Lock();
	m_bMsgClosed = TRUE;
	while ( !m_MsgList.IsEmpty() ) {
		FifoMsg *pFifoMsg = m_MsgList.RemoveHead();
		DeleteMsg(pFifoMsg);
	}
	m_MsgSemaphore.Unlock();
}

int CFifoBase::Consume(int nFd, int nBufLen)
{
	int len = (-1);
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		len = pFifo->Consume(nBufLen);
		RelFifo(pFifo);
	}

	return len;
}
int CFifoBase::Peek(int nFd, LPBYTE pBuffer, int nBufLen)
{
	int len = (-1);
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( (len = pFifo->ResBuffer(pBuffer, nBufLen)) == 0 && pFifo->m_bEndOf )
			len = (-1);
		RelFifo(pFifo);
	}

	return len;
}
int CFifoBase::Read(int nFd, LPBYTE pBuffer, int nBufLen)
{
	int len = (-1);
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( (len = pFifo->GetBuffer(pBuffer, nBufLen)) == 0 && pFifo->m_bEndOf )
			len = (-1);
		RelFifo(pFifo);
	}

	return len;
}
int CFifoBase::Write(int nFd, LPBYTE pBuffer, int nBufLen)
{
	int len = (-1);
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( !pFifo->m_bEndOf )
			len = pFifo->PutBuffer(pBuffer, nBufLen);
		RelFifo(pFifo);
	}

	return len;
}
int CFifoBase::Insert(int nFd, LPBYTE pBuffer, int nBufLen)
{
	int len = (-1);
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		len = pFifo->BackBuffer(pBuffer, nBufLen);
		RelFifo(pFifo);
	}

	return len;
}
BOOL CFifoBase::HaveData(int nFd)
{
	BOOL bActive = FALSE;
	CFifoBuffer *pFifo;
	
	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( pFifo->GetSize() > 0 || pFifo->m_bEndOf )
			bActive = TRUE;
		RelFifo(pFifo);
	}

	return bActive;
}
int CFifoBase::GetDataSize(int nFd)
{
	int len = (-1);
	CFifoBuffer *pFifo;
	
	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( (len = pFifo->GetSize()) == 0 && pFifo->m_bEndOf )
			len = (-1);
		RelFifo(pFifo);
	}

	return len;
}
int CFifoBase::GetXFd(int nFd)
{
	// STDIN->EXTOUT	STDOUT<-EXTIN	EXTIN->STDOUT	EXTOUT<-STDIN	...
	//	0	1	2	3	4	5	6	7	8	9	10	11	...
	//	3	2	1	0	7	6	5	4	11	10	9	8	...
	return ((nFd & ~3) + (3 - (nFd & 3)));
}
BYTE *CFifoBase::TempBuffer(int nSize)
{
	if ( nSize <= m_nBufMax )
		return m_pBuffer;

	if ( m_pBuffer != NULL )
		delete [] m_pBuffer;

	m_nBufMax = nSize;
	m_pBuffer = new BYTE[m_nBufMax];

	return m_pBuffer;
}
int CFifoBase::MoveBuffer(int nFd, class CBuffer *bp)
{
	int len = (-1);
	CFifoBuffer *pFifo;
	
	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( (len = pFifo->GetSize()) == 0 && pFifo->m_bEndOf )
			len = (-1);
		else if ( len > 0 ) {
			bp->Apend(pFifo->GetTopPtr(), len);
			pFifo->Consume(len);
		}
		RelFifo(pFifo);
	}

	return len;
}
void CFifoBase::Reset(int nFd)
{
	CFifoBuffer *pFifo;
	
	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		pFifo->m_bEndOf = FALSE;
		pFifo->m_nBufTop = pFifo->m_nBufBtm = 0;

		((CEvent *)(pFifo->m_pReadBase->m_EventTab[pFifo->m_nReadNumber]))->ResetEvent();
		pFifo->m_pReadBase->m_fdAllowEvents[pFifo->m_nReadNumber] = (FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
		pFifo->m_pReadBase->m_fdPostEvents[pFifo->m_nReadNumber] = 0;

		((CEvent *)(pFifo->m_pWriteBase->m_EventTab[pFifo->m_nWriteNumber]))->ResetEvent();
		pFifo->m_pWriteBase->m_fdAllowEvents[pFifo->m_nWriteNumber] = (FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
		pFifo->m_pWriteBase->m_fdPostEvents[pFifo->m_nWriteNumber] = 0;

		RelFifo(pFifo);
	}
}

///////////////////////////////////////////////////////

LPCSTR CFifoBase::DocMsgStrA(int msg, CStringA &str)
{
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_STRINGA;
	docMsg.pOut = (void *)&str;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, msg, (LPARAM)&docMsg);
	return str;
}
LPCWSTR CFifoBase::DocMsgStrW(int msg, CStringW &str)
{
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_STRINGW;
	docMsg.pOut = (void *)&str;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, msg, (LPARAM)&docMsg);
	return str;
}
void CFifoBase::DocMsgSize(int msg, CSize &size)
{
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_SIZE;
	docMsg.pOut = (void *)&size;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, msg, (LPARAM)&docMsg);
}
void CFifoBase::DocMsgRect(int msg, CRect &rect)
{
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_RECT;
	docMsg.pOut = (void *)&rect;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, msg, (LPARAM)&docMsg);
}
void CFifoBase::DocMsgIntPtr(int msg, int *pInt)
{
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_INTPTR;
	docMsg.pOut = (void *)pInt;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, msg, (LPARAM)&docMsg);
}
LPCSTR CFifoBase::DocMsgRemoteStr(LPCTSTR str, CStringA &mbs)
{
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_STRINGA;
	docMsg.pIn  = (void *)str;
	docMsg.pOut = (void *)&mbs;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, DOCMSG_REMOTESTR, (LPARAM)&docMsg);
	return mbs;
}
void CFifoBase::DocMsgLineMode(int sw)
{
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = sw;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, DOCMSG_LINEMODE, (LPARAM)&docMsg);
}
DWORD CFifoBase::DocMsgTtyMode(int mode, DWORD defVal)
{
	DWORD value = defVal;
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_DWORDPTR;
	docMsg.pIn  = (void *)mode;
	docMsg.pOut = (void *)&value;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, DOCMSG_TTYMODE, (LPARAM)&docMsg);
	return value;
}
DWORD CFifoBase::DocMsgKeepAlive(void *pThis)
{
	DWORD value = 0;
	DocMsg docMsg;

	docMsg.doc = m_pDocument;
	docMsg.type = DOCMSG_TYPE_DWORDPTR;
	docMsg.pIn  = pThis;
	docMsg.pOut = (void *)&value;

	::AfxGetMainWnd()->SendMessage(WM_DOCUMENTMSG, DOCMSG_KEEPALIVE, (LPARAM)&docMsg);
	return value;
}

///////////////////////////////////////////////////////
// CFifoBase	static

//	LeftBase STDIN(0)--Read---m_bReadBase--FifoBuffer--m_bWriteBase--Write--STDOUT(1)RightBase
//	LeftBase STDOUT(1)--Write--m_bWRiteBase---FifoBuffer--m_bReadBase--Read--STDIN(0)RightBase

//	CFifoBase::Link(pLeft, STDIN, pRight, STDIN);
//		pLeft(STDIN)<--pRight(STDOUT)
//		pLeft(STDOUT)-->pRight(STDIN)
//	CFifoBase::Link(pMid, STDIN, pRight, STDIN);
//		pLeft(STDIN)<--pMid(STDOUT)(EXTIN)<--pRight(STDOUT)
//		pLeft(STDOUT)-->pMid(STDIN)(EXTOUT)-->pRight(STDIN)
void CFifoBase::Link(CFifoBase *pLeft, int lFd, CFifoBase *pRight, int rFd)
{
	CFifoBuffer *pReadFifo = NULL;
	CFifoBuffer *pWriteFifo = NULL;
	CFifoBuffer *pExtReadFifo = NULL;
	CFifoBuffer *pExtWriteFifo = NULL;
	CFifoBase *pMid;

	if ( (rFd + 1) >= pRight->GetFifoSize() || (pReadFifo = (CFifoBuffer *)pRight->m_FifoBuf[rFd + 1]) == NULL )
		pReadFifo = new CFifoBuffer;
	else
		pExtReadFifo = new CFifoBuffer;

	if ( rFd >= pRight->GetFifoSize() || (pWriteFifo = (CFifoBuffer *)pRight->m_FifoBuf[rFd]) == NULL )
		pWriteFifo = new CFifoBuffer;
	else
		pExtWriteFifo = new CFifoBuffer;

	ASSERT((pExtReadFifo == NULL && pExtWriteFifo == NULL) || (pExtReadFifo != NULL && pExtWriteFifo != NULL));

	if ( pExtReadFifo == NULL && pExtWriteFifo == NULL ) {
		pLeft->Lock();
		pRight->Lock();

		pReadFifo->Lock();
		pWriteFifo->Lock();

		pReadFifo->m_pReadBase = pLeft;
		pReadFifo->m_pWriteBase = pRight;

		pWriteFifo->m_pWriteBase = pLeft;
		pWriteFifo->m_pReadBase = pRight;

		pLeft->SetFifo(lFd, pReadFifo);
		pLeft->SetFifo(lFd + 1, pWriteFifo);

		pReadFifo->m_nReadNumber = lFd;
		pWriteFifo->m_nWriteNumber = lFd + 1;

		pRight->SetFifo(rFd, pWriteFifo);
		pRight->SetFifo(rFd + 1, pReadFifo);

		pWriteFifo->m_nReadNumber = rFd;
		pReadFifo->m_nWriteNumber = rFd + 1;

		pReadFifo->Unlock();
		pWriteFifo->Unlock();

		pLeft->Unlock();
		pLeft->OnLinked(lFd, FALSE);

		pLeft->Lock();
		pRight->Unlock();
		pRight->OnLinked(rFd, FALSE);
		pLeft->Unlock();

	} else {
		pMid = pLeft;
		pLeft = pReadFifo->m_pReadBase;

		ASSERT(pLeft == pWriteFifo->m_pWriteBase);

		pLeft->Lock();
		pMid->Lock();
		pRight->Lock();

		pReadFifo->Lock();
		pWriteFifo->Lock();
		pExtReadFifo->Lock();
		pExtWriteFifo->Lock();

		//pReadFifo->m_pReadBase = pLeft;
		pReadFifo->m_pWriteBase = pMid;

		//pWriteFifo->m_pWriteBase = pLeft;
		pWriteFifo->m_pReadBase = pMid;

		pExtReadFifo->m_pReadBase = pMid;
		pExtReadFifo->m_pWriteBase = pRight;

		pExtWriteFifo->m_pWriteBase = pMid;
		pExtWriteFifo->m_pReadBase = pRight;

		pMid->SetFifo(lFd, pWriteFifo);
		pMid->SetFifo(lFd + 1, pReadFifo);
		pMid->SetFifo(lFd + 2, pExtReadFifo);
		pMid->SetFifo(lFd + 3, pExtWriteFifo);

		pWriteFifo->m_nReadNumber = lFd;
		pReadFifo->m_nWriteNumber = lFd + 1;
		pExtReadFifo->m_nReadNumber = lFd + 2;
		pExtWriteFifo->m_nWriteNumber = lFd + 3;

		for ( int n = 0 ; n < pRight->GetFifoSize() ; n++ ) {
			CFifoBuffer *pFifo = (CFifoBuffer *)pRight->m_FifoBuf[n];
			if ( pFifo == pWriteFifo ) {
				pRight->SetFifo(n, pExtWriteFifo);
				pExtWriteFifo->m_nReadNumber = n;
			}
			if ( pFifo == pReadFifo ) {
				pRight->SetFifo(n, pExtReadFifo);
				pExtReadFifo->m_nWriteNumber = n;
			}
		}
		
		pReadFifo->Unlock();
		pWriteFifo->Unlock();
		pExtReadFifo->Unlock();
		pExtWriteFifo->Unlock();

		pMid->Unlock();
		pMid->OnLinked(lFd, TRUE);

		pLeft->Unlock();
		pRight->Unlock();
	}
}
//		pLeft(STDIN)<--pMid(STDOUT)(EXTIN)<--pRight(STDOUT)
//		pLeft(STDOUT)-->pMid(STDIN)(EXTOUT)-->pRight(STDIN)
//	CFifoBase::Link(pMid, STDIN, TRUE);
//		pLeft(STDIN)<--pRight(STDOUT)
//		pLeft(STDOUT)-->pRight(STDIN)
//	CFifoBase::UnLink(pRight, STDIN, FALSE);
void CFifoBase::UnLink(CFifoBase *pMid, int nFd, BOOL bMid)
{
	CFifoBuffer *pReadFifo;
	CFifoBuffer *pWriteFifo;
	CFifoBuffer *pExtReadFifo;
	CFifoBuffer *pExtWriteFifo;
	CFifoBase *pLeft, *pRight;
	int lFd = FIFO_STDIN;

	ASSERT((nFd + 1) < pMid->GetFifoSize());
	pReadFifo = (CFifoBuffer *)pMid->m_FifoBuf[nFd];
	pWriteFifo = (CFifoBuffer *)pMid->m_FifoBuf[nFd + 1];
	ASSERT(pReadFifo != NULL && pWriteFifo != NULL);

	pLeft = pReadFifo->m_pWriteBase;
	lFd = pReadFifo->m_nWriteNumber - 1;
	ASSERT(pMid == pReadFifo->m_pReadBase);

	if ( !bMid ) {
		pLeft->OnUnLinked(lFd, FALSE);
		pMid->OnUnLinked(nFd, FALSE);

		pLeft->Lock();
		pMid->Lock();

		pReadFifo->Lock();
		pWriteFifo->Lock();

		for ( int n = 0 ; n < pLeft->GetFifoSize() ; n++ ) {
			CFifoBuffer *pFifo = (CFifoBuffer *)pLeft->m_FifoBuf[n];
			if ( pFifo == pWriteFifo )
				pLeft->SetFifo(n, NULL);
			if ( pFifo == pReadFifo )
				pLeft->SetFifo(n, NULL);
		}

		pMid->SetFifo(nFd, NULL);
		pMid->SetFifo(nFd + 1, NULL);

		pReadFifo->Unlock();
		pWriteFifo->Unlock();

		delete pReadFifo;
		delete pWriteFifo;

		pMid->Unlock();
		pLeft->Unlock();

	} else {
		ASSERT((nFd + 3) < pMid->GetFifoSize());
		pExtReadFifo = (CFifoBuffer *)pMid->m_FifoBuf[nFd + 2];
		pExtWriteFifo = (CFifoBuffer *)pMid->m_FifoBuf[nFd + 3];
		ASSERT(pExtReadFifo != NULL && pExtWriteFifo != NULL);

		pRight = pExtReadFifo->m_pWriteBase;
		ASSERT(pMid == pExtReadFifo->m_pReadBase);

		pMid->OnUnLinked(nFd, TRUE);

		pLeft->Lock();
		pMid->Lock();
		pRight->Lock();

		pReadFifo->Lock();
		pWriteFifo->Lock();
		pExtReadFifo->Lock();
		pExtWriteFifo->Lock();

		pReadFifo->m_pReadBase = pRight;
		pWriteFifo->m_pWriteBase = pRight;

		for ( int n = 0 ; n < pRight->GetFifoSize() ; n++ ) {
			CFifoBuffer *pFifo = (CFifoBuffer *)pRight->m_FifoBuf[n];
			if ( pFifo == pExtWriteFifo ) {
				pRight->SetFifo(n, pReadFifo);
				pReadFifo->m_nReadNumber = n;
			}
			if ( pFifo == pExtReadFifo ) {
				pRight->SetFifo(n, pWriteFifo);
				pWriteFifo->m_nWriteNumber = n;
			}
		}
		
		pMid->SetFifo(nFd, NULL);
		pMid->SetFifo(nFd + 1, NULL);
		pMid->SetFifo(nFd + 2, NULL);
		pMid->SetFifo(nFd + 3, NULL);

		pMid->Unlock();
		pLeft->Unlock();
		pRight->Unlock();

		// Left          Mid                            Right
		// STDIN  pRead  STDOUT pWrite EXTIN  pExtRead  STDOUT
		// STDOUT pWrite STDIN  pRead  EXTOUT pExtWrite STDIN

		// pExtRead->pWrite
		if ( pExtReadFifo->GetSize() > 0 )
			pWriteFifo->BackBuffer(pExtReadFifo->GetTopPtr(), pExtReadFifo->GetSize());
		// pExtWrite->pRead
		if ( pExtWriteFifo->GetSize() > 0 )
			pReadFifo->PutBuffer(pExtWriteFifo->GetTopPtr(), pExtWriteFifo->GetSize());

		pReadFifo->Unlock();
		pWriteFifo->Unlock();
		pExtReadFifo->Unlock();
		pExtWriteFifo->Unlock();

		delete pExtReadFifo;
		delete pExtWriteFifo;
	}
}

///////////////////////////////////////////////////////
// CFifoWnd

// CFifoBase::Left									CFifoWnd::Right
// Write->Send(FD_READ)->FifoEvents->PostMessage	OnFifoMsg->OnRead
// Read->Send(FD_WRITE)->FifoEvents->PostMessage	OnFifoMsg->OnWrite

CFifoWnd::CFifoWnd(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoBase(pDoc, pSock)
{
	m_Type = FIFO_TYPE_WINDOW;
	m_WndMsg = WM_FIFOMSG;
	m_pWnd = ::AfxGetMainWnd();
	m_bMsgPump = FALSE;
	m_bPostMsg = FALSE;
	m_bDestroy = FALSE;
}
CFifoWnd::~CFifoWnd()
{
}

void CFifoWnd::OnLinked(int nFd, BOOL bMid)
{
	if ( nFd == FIFO_STDIN )
		((CMainFrame *)::AfxGetMainWnd())->AddFifoActive(this);

	CFifoBase::OnLinked(nFd, bMid);
}
void CFifoWnd::OnUnLinked(int nFd, BOOL bMid)
{
	if ( nFd == FIFO_STDIN )
		((CMainFrame *)::AfxGetMainWnd())->DelFifoActive(this);

	CFifoBase::OnUnLinked(nFd, bMid);
}

// pFifoBufから直接呼出しLock中で別スレッドの場合あり
void CFifoWnd::PostMessage(int cmd, int param, int msg)
{
	m_MsgSemaphore.Lock();
	if ( !m_bDestroy && !m_bPostMsg && !m_MsgList.IsEmpty() ) {
		m_bPostMsg = TRUE;
		m_MsgSemaphore.Unlock();

		m_pWnd->PostMessage(m_WndMsg, MAKEWPARAM(cmd, param), (LPARAM)this);
	} else
		m_MsgSemaphore.Unlock();
}
void CFifoWnd::FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam)
{
	switch(fdEvent) {
	case FD_READ:	// Fifo.PutBuffer
	case FD_WRITE:	// Fifo.GetBuffer
		if ( (m_fdAllowEvents[nFd] & fdEvent) != 0 && (m_fdPostEvents[nFd] & fdEvent) == 0 ) {
			m_fdPostEvents[nFd] |= fdEvent;
			PostCommand(FIFO_CMD_FDEVENTS, nFd, fdEvent, 0, pParam);
		}
		break;

	case FD_OOB:
		if ( (m_fdAllowEvents[nFd] & fdEvent) != 0  ) {
			WSABUF *pSbuf = (WSABUF *)pParam;
			ASSERT(pSbuf != NULL && pSbuf->len > 0 && pSbuf->buf != NULL);
			PostCommand(FIFO_CMD_FDEVENTS, nFd, fdEvent, pSbuf->len, pSbuf->buf);
		}
		break;

	case FD_ACCEPT:
		if ( (m_fdAllowEvents[nFd] & fdEvent) != 0  )
			PostCommand(FIFO_CMD_FDEVENTS, nFd, fdEvent, 0, pParam);
		break;

	case FD_CONNECT:
	case FD_CLOSE:
		if ( (m_fdAllowEvents[nFd] & fdEvent) != 0  && (m_fdPostEvents[nFd] & fdEvent) == 0 ) {
			m_fdPostEvents[nFd] |= fdEvent;
			PostCommand(FIFO_CMD_FDEVENTS, nFd, fdEvent, 0, pParam);
		}
		break;
	}

	PostMessage(FIFO_CMD_FDEVENTS, nFd, fdEvent);
}
//	ON_MESSAGE(WM_FIFOMSG, OnFifoMsg)
// 	afx_msg LRESULT OnFifoMsg(WPARAM wParam, LPARAM lParam);
// 	LRESULT CMainFrame::OnFifoMsg(WPARAM wParam, LPARAM lParam) { ((CFifoWnd *)lParam)->MsgPump((int)wParam); }
void CFifoWnd::MsgPump(WPARAM wParam)
{
	int cmd = LOWORD(wParam);
	int param = HIWORD(wParam);
	FifoMsg *pFifoMsg;
	CFifoBuffer *pFifo;

	m_MsgSemaphore.Lock();
	ASSERT(m_bMsgPump == FALSE);
	m_bMsgPump = TRUE;

	while ( !m_MsgList.IsEmpty() ) {
		pFifoMsg = m_MsgList.RemoveHead();
		m_MsgSemaphore.Unlock();

		if ( !m_bDestroy ) {
			switch(pFifoMsg->cmd) {
			case FIFO_CMD_FDEVENTS:
				if ( (pFifoMsg->msg & (FD_READ | FD_WRITE)) != 0 && (pFifo = GetFifo(pFifoMsg->param)) != NULL ) {
					m_fdPostEvents[pFifoMsg->param] &= ~(pFifoMsg->msg);
					RelFifo(pFifo);
				}

				if ( (pFifoMsg->msg & FD_ACCEPT) != 0 )
					OnAccept(pFifoMsg->param, (SOCKET)pFifoMsg->buffer);
				if ( (pFifoMsg->msg & FD_CONNECT) != 0 )
					OnConnect(pFifoMsg->param);
				if ( (pFifoMsg->msg & FD_CLOSE) != 0 )
					OnClose(pFifoMsg->param, (int)pFifoMsg->buffer);
				if ( (pFifoMsg->msg & FD_OOB) != 0 )
					OnOob(pFifoMsg->param, pFifoMsg->len, pFifoMsg->buffer);
				if ( (pFifoMsg->msg & FD_READ) != 0 && IsFdEvents(pFifoMsg->param, FD_READ) )
					OnRead(pFifoMsg->param);
				if ( (pFifoMsg->msg & FD_WRITE) != 0 && IsFdEvents(pFifoMsg->param, FD_WRITE) )
					OnWrite(pFifoMsg->param);
				break;

			default:
				OnCommand(pFifoMsg->cmd, pFifoMsg->param, pFifoMsg->msg, pFifoMsg->len, pFifoMsg->buffer, pFifoMsg->pEvent, pFifoMsg->pResult);
				break;
			}
		}

		DeleteMsg(pFifoMsg);
		m_MsgSemaphore.Lock();
	}

	m_bMsgPump = m_bPostMsg = FALSE;
	m_MsgSemaphore.Unlock();

	if ( m_bDestroy )
		CFifoBase::Destroy();
}
void CFifoWnd::SendCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	if ( !m_bDestroy ) {
		PostCommand(cmd, param, msg, len, buf, pEvent, pResult);
		PostMessage(cmd, param, msg);
	}
}
void CFifoWnd::Destroy()
{
	// 以降のPostMessageをキャンセル
	((CMainFrame *)::AfxGetMainWnd())->DelFifoActive(this);

	// PostMessageを処理中ならMsgPumpでDestroy
	m_MsgSemaphore.Lock();
	if ( m_bMsgPump )
		m_bDestroy = TRUE;
	m_MsgSemaphore.Unlock();

	if ( !m_bDestroy )
		CFifoBase::Destroy();
}

void CFifoWnd::EndOfData(int nFd)
{
	int eFd = GetXFd(nFd);
	SendFdEvents(eFd, FD_CLOSE, (void *)m_nLastError);
	Write(eFd, NULL, 0);

	if ( nFd == FIFO_STDIN ) {
		if ( m_nLastError == 0 )
			m_pSock->OnClose();
		else
			m_pSock->OnError(m_nLastError);
	}
}
void CFifoWnd::Recived(int nFd, BYTE *pBuffer, int nBufLen)
{
	if ( nFd == FIFO_STDIN )
		m_pSock->OnRecvSocket(pBuffer, nBufLen, 0);
	else if ( nFd == FIFO_EXTIN )
		m_pSock->OnRecvProtocol(pBuffer, nBufLen, 0);
}

// 接続相手がデータを書き込んだ場合(PutBuffer)に呼び出される
void CFifoWnd::OnRead(int nFd)
{
	int readlen;
	int eFd = GetXFd(nFd);
	BYTE *buf = TempBuffer(m_nBufSize);

	while ( !m_bDestroy ) {
		if ( m_bFlowCtrl && GetDataSize(eFd) >= FIFO_BUFUPPER ) {
			// eFd overflow
			ResetFdEvents(nFd, FD_READ);
			break;
		} else if ( (readlen = Read(nFd, buf, m_nBufSize)) < 0 ) {
			// nFd End of data
			ResetFdEvents(nFd, FD_READ);
			EndOfData(nFd);
			break;
		} else if ( readlen == 0 ) {
			// nFd No data
			break;
		} else {
			// Call Process
			Recived(nFd, buf, readlen);
		}
	}
}
// 接続相手がデータを読み込んだ場合(GetBuffer)に呼び出される
void CFifoWnd::OnWrite(int nFd)
{
	int eFd = GetXFd(nFd);
	int len = GetDataSize(nFd);

	if ( len >= 0 && len <= FIFO_BUFLOWER && !IsFdEvents(eFd, FD_READ) )
		SetFdEvents(eFd, FD_READ);
}
void CFifoWnd::OnOob(int nFd, int len, BYTE *pBuffer)
{
	m_pSock->OnRecvSocket(pBuffer, len, MSG_OOB);
}
void CFifoWnd::OnAccept(int nFd, SOCKET socket)
{
	m_pSock->OnAccept(socket);
}
void CFifoWnd::OnConnect(int nFd)
{
	m_pSock->OnConnect();
	SendFdEvents(GetXFd(nFd), FD_CONNECT, NULL);
}
void CFifoWnd::OnClose(int nFd, int nLastError)
{
	m_nLastError = nLastError;
}
void CFifoWnd::OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	// default そのまま下位に転送
	SendFdCommand(FIFO_STDOUT, cmd, param, msg, len, buf, pEvent, pResult);
}

///////////////////////////////////////////////////////
// CFifoWorkThread

IMPLEMENT_DYNCREATE(CFifoWorkThread, CWinThread)

BEGIN_MESSAGE_MAP(CFifoWorkThread, CWinThread)
	ON_THREAD_MESSAGE(WM_FIFOMSG, OnFifoMsg)
END_MESSAGE_MAP()

CFifoWorkThread::CFifoWorkThread()
{
	m_bDestroy = FALSE;
}
BOOL CFifoWorkThread::InitInstance()
{
	return TRUE;
}
int CFifoWorkThread::ExitInstance()
{
	// CFifoTelnetでopenssl関数を使用
	OPENSSL_thread_stop();

	return CWinThread::ExitInstance();
}
void CFifoWorkThread::OnFifoMsg(WPARAM wParam, LPARAM lParam)
{
	if ( !m_bDestroy )
		((CFifoThread *)lParam)->MsgPump(wParam);
}

///////////////////////////////////////////////////////
// CFifoThread

CFifoThread::CFifoThread(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoWnd(pDoc, pSock)
{
	m_Type = FIFO_TYPE_THREAD;
	m_pWinThread = NULL;
}
CFifoThread::~CFifoThread()
{
	ThreadClose();
}

BOOL CFifoThread::ThreadOpen()
{
	if ( m_pWinThread != NULL )
		return FALSE;

	m_pWinThread = new CFifoWorkThread;

	if ( !m_pWinThread->CreateThread() )
		goto ERRRET;

	if ( !m_pWinThread->SetThreadPriority(THREAD_PRIORITY_NORMAL) )
		goto ERRRET;

	return TRUE;

ERRRET:
	delete m_pWinThread;
	m_pWinThread = NULL;
	return FALSE;
}
BOOL CFifoThread::ThreadClose()
{
	if ( m_pWinThread == NULL )
		return FALSE;

	m_pWinThread->m_bDestroy = TRUE;
	m_pWinThread->PostThreadMessage(WM_QUIT, 0, 0);

	WaitForSingleObject(*m_pWinThread, INFINITE);
	m_pWinThread = NULL;

	return TRUE;
}

BOOL CFifoThread::IsOpen()
{
	return (m_pWinThread != NULL ? TRUE : FALSE);
}
void CFifoThread::Destroy()
{
	ThreadClose();
	CFifoWnd::Destroy();
}

void CFifoThread::OnLinked(int nFd, BOOL bMid)
{
	// skip CFifoWind::OnLinked
	CFifoBase::OnLinked(nFd, bMid);

	if ( nFd == FIFO_STDIN )
		ThreadOpen();
}
void CFifoThread::OnUnLinked(int nFd, BOOL bMid)
{
	if ( nFd == FIFO_STDIN )
		ThreadClose();

	// skip CFifoWind::OnLinked
	CFifoBase::OnUnLinked(nFd, bMid);
}
void CFifoThread::PostMessage(int cmd, int param, int msg)
{
	m_MsgSemaphore.Lock();
	if ( !m_bDestroy && !m_bPostMsg && !m_MsgList.IsEmpty() ) {
		m_bPostMsg = TRUE;
		m_MsgSemaphore.Unlock();

		m_pWinThread->PostThreadMessage(WM_FIFOMSG, MAKEWPARAM(cmd, param), (LPARAM)this);
	} else
		m_MsgSemaphore.Unlock();
}

///////////////////////////////////////////////////////
// CFifoSync

// CFifoBase::Left								CFifoSync::Right
// Write->Send(FD_READ)->FifoEvents->SetEvent	WaitEvent->ReadProc
// Read->Send(FD_WRITE)->FifoEvents->SetEvent	WaitEvent->WriteProc

CFifoSync::CFifoSync(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoBase(pDoc, pSock)
{
	m_Type = FIFO_TYPE_SYNC;
	m_Threadtatus = FIFO_THREAD_NONE;
	m_pWinThread = NULL;
	m_TunnelFd = (-1);
}
CFifoSync::~CFifoSync()
{
	Close();

	for ( int n = 0 ; n < m_nFdBuffer.GetSize() ; n++ ) {
		if ( m_nFdBuffer[n] != NULL )
			delete (CBuffer *)m_nFdBuffer[n];
	}
}
BOOL CFifoSync::IsOpen()
{
	return (m_Threadtatus == FIFO_THREAD_EXEC && m_pWinThread != NULL ? TRUE : FALSE);
}

void CFifoSync::FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam)
{
	int eFd;

	switch(fdEvent) {
	case FD_READ:	// Fifo.PutBuffer
		if ( pFifo != NULL && (pFifo->GetSize() > 0 || pFifo->m_bEndOf) )
			GetEvent(nFd)->SetEvent();		// Wait Form Recv
		break;

	case FD_WRITE:	// Fifo.GetBuffer
		if ( pFifo != NULL && (pFifo->GetSize() <= 0 || pFifo->m_bEndOf) )
			GetEvent(nFd)->SetEvent();		// Wait From Send
		break;

	case FD_OOB:
	case FD_ACCEPT:
		if ( (eFd = GetXFd(nFd)) < GetFifoSize() && m_FifoBuf[eFd] != NULL )
			SendFdEvents(eFd, fdEvent, pParam);
		break;

	case FD_CONNECT:
	case FD_CLOSE:
		if ( (m_fdPostEvents[nFd] & fdEvent) == 0 ) {
			m_fdPostEvents[nFd] |= fdEvent;
			if ( (eFd = GetXFd(nFd)) < GetFifoSize() && m_FifoBuf[eFd] != NULL )
				SendFdEvents(eFd, fdEvent, pParam);
		}
		break;
	}
}
void CFifoSync::SendCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent)
{
	SendFdCommand(FIFO_STDOUT, cmd, param, msg, len, buf, pEvent);
}
void CFifoSync::Destroy()
{
	Close();
	CFifoBase::Destroy();
}

BOOL CFifoSync::WaitEvents(HANDLE hEvent, DWORD mSec)
{
	int nFd;
	DWORD ec;
	HANDLE hWaitEvents[4];
	DWORD Result;
	clock_t st;

	while ( mSec != 0 ) {
		ec = 0;
		if ( hEvent != NULL )
			hWaitEvents[ec++] = hEvent;
		if ( (nFd = m_TunnelFd) != (-1) && nFd < GetFifoSize() )
			hWaitEvents[ec++] = GetEvent(nFd)->m_hObject;
		hWaitEvents[ec++] = m_AbortEvent;

		st = clock();

		if ( (Result = WaitForMultipleObjects(ec, hWaitEvents, FALSE, mSec)) == WAIT_FAILED ) {
			m_nLastError = GetLastError();
			SendFdEvents(FIFO_EXTOUT, FD_CLOSE, (void *)m_nLastError);
			return FALSE;
		}

		if ( Result == WAIT_TIMEOUT ) {
			break;
		} else if ( Result < WAIT_OBJECT_0 || (Result -= WAIT_OBJECT_0) >= ec )
			return FALSE;

		if ( hWaitEvents[Result] == hEvent ) {
			break;
		} else if ( hWaitEvents[Result] == m_AbortEvent ) {
			return FALSE;
		} else if ( (nFd = m_TunnelFd) != (-1) && hWaitEvents[Result] == GetEvent(nFd)->m_hObject ) {
			int eFd = GetXFd(nFd);
			int len;
			BYTE *buf = TempBuffer(m_nBufSize);
			while ( (len = Read(nFd, buf, m_nBufSize)) > 0 )
				Write(eFd, buf, len);
		}

		if ( (Result = (DWORD)((clock() - st) * 1000 / CLOCKS_PER_SEC)) >= mSec ) {
			break;
		}
		mSec -= Result;
	}

	return TRUE;
}
int CFifoSync::Recv(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec)
{
	int len = (-1);
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		GetEvent(nFd)->ResetEvent();
		if ( (len = pFifo->GetBuffer(pBuffer, nBufLen)) == 0 ) {
			if ( pFifo->m_bEndOf )
				len = (-1);
			else if ( mSec != 0 ) {
				RelFifo(pFifo);

				if ( !WaitEvents(GetEvent(nFd)->m_hObject, mSec) )
					return (-1);

				if ( (pFifo = GetFifo(nFd)) == NULL )
					return (-1);
				if ( (len = pFifo->GetBuffer(pBuffer, nBufLen)) == 0 && pFifo->m_bEndOf )
					len = (-1);
			}
		}
		RelFifo(pFifo);
	}

	return len;
}
int CFifoSync::Send(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec)
{
	int len = (-1);
	CFifoBuffer *pFifo;

	if ( (pFifo = GetFifo(nFd)) != NULL ) {
		if ( !pFifo->m_bEndOf ) {
			GetEvent(nFd)->ResetEvent();
			len = pFifo->PutBuffer(pBuffer, nBufLen);
			if ( mSec != 0 && len > 0 ) {
				RelFifo(pFifo);

				if ( !WaitEvents(GetEvent(nFd)->m_hObject, mSec) )
					len = (-1);
			}
		} else
			RelFifo(pFifo);
	}

	return len;
}

CBuffer *CFifoSync::nFdBuffer(int nFd)
{
	while ( nFd >= m_nFdBuffer.GetSize() )
		m_nFdBuffer.Add(NULL);

	if ( m_nFdBuffer[nFd] == NULL )
		m_nFdBuffer[nFd] = new CBuffer;

	return (CBuffer *)m_nFdBuffer[nFd];
}
int CFifoSync::RecvByte(int nFd, DWORD mSec)
{
	CBuffer *bp = nFdBuffer(nFd);
	if ( bp->GetSize() <= 0 ) {
		int n;
		BYTE *pTemp = TempBuffer(m_nBufSize);
		if ( (n = Recv(nFd, pTemp, m_nBufSize, mSec)) <= 0 )
			return (-1);
		bp->Apend(pTemp, n);
	}
	return bp->GetByte();
}
int CFifoSync::RecvBuffer(int nFd, LPBYTE pBuffer, int nBufLen, DWORD mSec)
{
	CBuffer *bp = nFdBuffer(nFd);
	while ( bp->GetSize() < nBufLen ) {
		int n;
		BYTE *pTemp = TempBuffer(m_nBufSize);
		if ( (n = Recv(nFd, pTemp, m_nBufSize, mSec)) <= 0 )
			return (-1);
		bp->Apend(pTemp, n);
	}
	memcpy(pBuffer, bp->GetPtr(), nBufLen);
	bp->Consume(nBufLen);
	return nBufLen;
}
void CFifoSync::RecvBaek(int nFd, LPBYTE pBuffer, int nBufLen)
{
	CBuffer *bp = nFdBuffer(nFd);
	if ( pBuffer != NULL && nBufLen > 0 )
		bp->Apend(pBuffer, nBufLen);
	if ( bp->GetSize() > 0 )
		Insert(nFd, bp->GetPtr(), bp->GetSize());
	bp->Clear();
}
void CFifoSync::RecvClear(int nFd)
{
	nFdBuffer(nFd)->Clear();
	for ( int n = 0 ; ; ) {
		Sleep(200);
		if ( (n = GetDataSize(nFd)) > 0 )
			Consume(nFd, n);
		else
			break;
	}
}

BOOL CFifoSync::Open(AFX_THREADPROC pfnThreadProc, LPVOID pParam, int nPriority)
{
	Close();

	for ( int n = 0 ; n < m_nFdBuffer.GetSize() ; n++ ) {
		if ( m_nFdBuffer[n] != NULL )
			((CBuffer *)m_nFdBuffer[n])->Clear();
	}

	m_Threadtatus = FIFO_THREAD_EXEC;
	m_AbortEvent.ResetEvent();

	if ( (m_pWinThread = AfxBeginThread(pfnThreadProc, pParam, nPriority)) == NULL ) {
		m_Threadtatus = FIFO_THREAD_ERROR;
		return FALSE;
	}

	return TRUE;
}
void CFifoSync::Close()
{
	if ( m_pWinThread == NULL )
		return;

	if ( m_Threadtatus == FIFO_THREAD_EXEC ) {
		m_Threadtatus = FIFO_THREAD_ERROR;
		Abort();
		WaitForSingleObject(*m_pWinThread, INFINITE);
	}

	m_pWinThread = NULL;
	m_Threadtatus = FIFO_THREAD_NONE;
}
void CFifoSync::Abort()
{
	m_AbortEvent.SetEvent();
}

///////////////////////////////////////////////////////
// CFifoASync

// CFifoBase::Left								CFifoASync::Right
// Write->Send(FD_READ)->FifoEvents->SetEvent	WaitEvent->ReadProc
// Read->Send(FD_WRITE)->FifoEvents->SetEvent	WaitEvent->WriteProc

CFifoASync::CFifoASync(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoBase(pDoc, pSock)
{
	m_Type = FIFO_TYPE_EVENT;
}
CFifoASync::~CFifoASync()
{
}
void CFifoASync::FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam)
{
	switch(fdEvent) {
	case FD_READ:
	case FD_WRITE:
		GetEvent(nFd)->SetEvent();		// Wait Form Read or Write
		break;

	case FD_OOB:
		{
			WSABUF *pSbuf = (WSABUF *)pParam;
			ASSERT(pSbuf != NULL && pSbuf->len > 0 && pSbuf->buf != NULL);
			PostCommand(FIFO_CMD_FDEVENTS, nFd, FD_OOB, pSbuf->len, pSbuf->buf);
		}
		break;

	case FD_ACCEPT:
		PostCommand(FIFO_CMD_FDEVENTS, nFd, fdEvent, 0, pParam);
		break;

	case FD_CONNECT:
	case FD_CLOSE:
		if ( (m_fdPostEvents[nFd] & fdEvent) == 0 ) {
			m_fdPostEvents[nFd] |= fdEvent;
			PostCommand(FIFO_CMD_FDEVENTS, nFd, fdEvent, 0, pParam);
		}
		break;
	}
}
int CFifoASync::ReadWriteEvent()
{
	int HandleCount = 0;
	CFifoBuffer *pFifo;

	for ( int n = 0 ; n < GetFifoSize() ; n++ ) {
		if ( (pFifo = GetFifo(n)) == NULL )
			continue;

		if ( pFifo->m_pReadBase == this ) {					// FIFO_STDIN / FIFO_EXTIN
			if ( !pFifo->m_bEndOf ) {
				m_hWaitEvents.Add(GetEvent(n)->m_hObject);
				HandleCount++;
			}

		} else if ( pFifo->m_pWriteBase == this ) {			// FIFO_STDOUT / FIFO_EXTOUT
			if ( !pFifo->m_bEndOf || pFifo->GetSize() > 0 ) {
				m_hWaitEvents.Add(GetEvent(n)->m_hObject);
				HandleCount++;
			}
		}

		RelFifo(pFifo);
	}

	return HandleCount;
}

///////////////////////////////////////////////////////
// CFifoSocket

CFifoSocket::CFifoSocket(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoASync(pDoc, pSock)
{
	m_Type = FIFO_TYPE_SOCKET;
	m_hSocket = INVALID_SOCKET;
	m_SockEvent = WSA_INVALID_EVENT;
	m_Threadtatus = FIFO_THREAD_NONE;
	m_pWinThread = NULL;
	m_bAbort = FALSE;
	m_SSL_pSock = NULL;
}
CFifoSocket::~CFifoSocket()
{
	Detach();
}
BOOL CFifoSocket::IsOpen()
{
	return (m_Threadtatus == FIFO_THREAD_EXEC && m_pWinThread != NULL ? TRUE : FALSE);
}
void CFifoSocket::Destroy()
{
	Close();
	CFifoASync::Destroy();
}
void CFifoSocket::OnUnLinked(int nFd, BOOL bMid)
{
	Close();
	CFifoBase::OnUnLinked(nFd, bMid);
}
static UINT FifoSocketOpenWorker(LPVOID pParam)
{
	int status;
	CFifoSocket *pThis = (CFifoSocket *)pParam;

	status = (pThis->AddInfoOpen() && pThis->SocketLoop() ? FIFO_THREAD_NONE : FIFO_THREAD_ERROR);
	pThis->ThreadEnd();
	pThis->m_Threadtatus = status;

	return 0;
}
void CFifoSocket::ThreadEnd()
{
	if ( !m_bAbort ) {
		SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)m_nLastError);
		Write(FIFO_STDOUT, NULL, 0);
	}

	RemoveAllMsg();
}
BOOL CFifoSocket::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nFamily, int nSocketType)
{
	if ( lpszHostAddress != NULL ) {
		m_HostAddress = lpszHostAddress;
		m_nHostPort   = nHostPort;
		m_nSocketPort = nSocketPort;
		m_nFamily     = nFamily;
		m_nSocketType = nSocketType;
	}

	if ( (m_SockEvent = WSACreateEvent()) == WSA_INVALID_EVENT )
		return FALSE;

	m_hSocket = INVALID_SOCKET;
	m_Threadtatus = FIFO_THREAD_EXEC;
	m_AbortEvent.ResetEvent();
	m_bAbort = FALSE;

	if ( (m_pWinThread = AfxBeginThread(FifoSocketOpenWorker, this, THREAD_PRIORITY_NORMAL)) == NULL ) {
		m_Threadtatus = FIFO_THREAD_ERROR;
		WSACloseEvent(m_SockEvent);
		return FALSE;
	}

	return TRUE;
}
BOOL CFifoSocket::Close()
{
	if ( m_pWinThread == NULL ) {
		WSASetLastError(WSAEFAULT);
		return FALSE;
	}

	if ( m_Threadtatus == FIFO_THREAD_EXEC ) {
		m_Threadtatus = FIFO_THREAD_ERROR;
		m_AbortEvent.SetEvent();
		WaitForSingleObject(*m_pWinThread, INFINITE);
	}

	if ( m_hSocket != INVALID_SOCKET )
		::closesocket(m_hSocket);

	if ( m_SockEvent != WSA_INVALID_EVENT )
	WSACloseEvent(m_SockEvent);

	m_hSocket = INVALID_SOCKET;
	m_SockEvent = WSA_INVALID_EVENT;
	m_pWinThread = NULL;

	if ( m_Threadtatus == FIFO_THREAD_NONE )
		return TRUE;

	WSASetLastError(m_nLastError);
	return FALSE;
}
BOOL CFifoSocket::ReOpen()
{
	Close();
	Reset(FIFO_STDIN);
	Reset(FIFO_STDOUT);
	return Open();
}
static UINT FifoSocketWorker(LPVOID pParam)
{
	int status;
	CFifoSocket *pThis = (CFifoSocket *)pParam;

	status = (pThis->SocketLoop() ? FIFO_THREAD_NONE : FIFO_THREAD_ERROR);
	pThis->ThreadEnd();
	pThis->m_Threadtatus = status;

	return 0;
}
BOOL CFifoSocket::Attach(SOCKET hSocket)
{
	Detach();

	if ( (m_hSocket = hSocket) == INVALID_SOCKET )
		return FALSE;

	DWORD val = 1;
	if ( ::ioctlsocket(m_hSocket, FIONBIO, &val) != 0 )
		return FALSE;

	if ( (m_SockEvent = WSACreateEvent()) == WSA_INVALID_EVENT )
		return FALSE;

	m_Threadtatus = FIFO_THREAD_EXEC;
	m_AbortEvent.ResetEvent();
	m_bAbort = FALSE;

	if ( (m_pWinThread = AfxBeginThread(FifoSocketWorker, this, THREAD_PRIORITY_NORMAL)) == NULL ) {
		m_Threadtatus = FIFO_THREAD_ERROR;
		WSACloseEvent(m_SockEvent);
		return FALSE;
	}

	return TRUE;
}
void CFifoSocket::Detach()
{
	if ( m_hSocket == INVALID_SOCKET || m_pWinThread == NULL )
		return;

	if ( m_Threadtatus == FIFO_THREAD_EXEC ) {
		m_Threadtatus = FIFO_THREAD_ERROR;
		m_AbortEvent.SetEvent();
		WaitForSingleObject(*m_pWinThread, INFINITE);
	}

	m_pWinThread = NULL;

	DWORD val = 0;
	::ioctlsocket(m_hSocket, FIONBIO, &val);

	WSACloseEvent(m_SockEvent);
	m_hSocket = INVALID_SOCKET;
}

// Thread process...
BOOL CFifoSocket::AddInfoOpen()
{
	int n;
	ADDRINFOT hints;
	ADDRINFOT *ai, *aitop;
	DWORD val;
	CString ServicsName;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
	struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	UINT nSocketPort = m_nSocketPort;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = m_nFamily;
	hints.ai_socktype = m_nSocketType;
	ServicsName.Format(_T("%d"), m_nHostPort);

	if ( GetAddrInfo(m_HostAddress, ServicsName, &hints, &aitop) != 0 ) {
		m_nLastError = WSAGetLastError();
		return FALSE;
	}

	for ( ; ; ) {
		for ( ai = aitop ; ai != NULL ; ai = ai->ai_next ) {
			if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
				continue;

			if ( (m_hSocket = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) ) {
				m_nLastError = WSAGetLastError();
				continue;
			}

			if ( nSocketPort != 0 ) {
				if ( ai->ai_family == AF_INET ) {
					memset(&in, 0, sizeof(in));
					in.sin_family = AF_INET;
					in.sin_addr.s_addr = INADDR_ANY;
					in.sin_port = htons(nSocketPort);
					if ( ::bind(m_hSocket, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR ) {
						m_nLastError = WSAGetLastError();
						::closesocket(m_hSocket);
						continue;
					}
				} else {	// AF_INET6
					memset(&in6, 0, sizeof(in6));
					in6.sin6_family = AF_INET6;
					in6.sin6_addr = in6addr_any;
					in6.sin6_port = htons(nSocketPort);
					if ( ::bind(m_hSocket, (struct sockaddr *)&in6, sizeof(in6)) == SOCKET_ERROR ) {
						m_nLastError = WSAGetLastError();
						::closesocket(m_hSocket);
						continue;
					}
				}
			}

			if ( ::connect(m_hSocket, ai->ai_addr, (int)ai->ai_addrlen) == SOCKET_ERROR ) {
				if ( (n = WSAGetLastError()) != WSAEWOULDBLOCK ) {
					m_nLastError = n;
					::closesocket(m_hSocket);
					continue;
				}
			} else
				SendFdEvents(FIFO_STDOUT, FD_CONNECT, NULL);

			val = 1;
			if ( ::ioctlsocket(m_hSocket, FIONBIO, &val) != 0 ) {
				m_nLastError = WSAGetLastError();
				::closesocket(m_hSocket);
				continue;
			}

			// socket open success
			m_nLastError = 0;
			FreeAddrInfo(aitop);
			return TRUE;
		}

		if ( nSocketPort-- == 0 )
			break;
	}

	FreeAddrInfo(aitop);
	m_hSocket = INVALID_SOCKET;
	return FALSE;
}
BOOL CFifoSocket::SocketLoop()
{
	int n;
	int len;
	int sslErr = 0;
	int nDefBufSize = m_nBufSize;
	DWORD Event;
	WSANETWORKEVENTS NetworkEvents;
	long fdEvents = FD_READ | FD_WRITE | FD_OOB | FD_CONNECT | FD_CLOSE;
	long oldEvents = 0;
	int bClose = 000;
	BYTE *buffer;
	CList<FifoMsg *, FifoMsg *> OobList;
	LONGLONG RecvSize = 0;
	LONGLONG SendSize = 0;
	clock_t now;
	clock_t RecvClock = clock();
	clock_t SendClock = clock();
	DWORD RecvTimeOut = 0;
	DWORD SendTimeOut = 0;
	DWORD TimeOut;
	HANDLE hPauseEvent = NULL;
	void **pParam = NULL;
	BOOL bReadWrite = FALSE;

	if ( m_nLimitSize > 0 && (m_nBufSize = m_nLimitSize * 50 / 1000) < 32 )		// Byte per 50ms
		m_nBufSize = 32;

	buffer = TempBuffer(m_nBufSize);

	for ( ; ; ) {
		if ( hPauseEvent != NULL ) {
			m_hWaitEvents.RemoveAll();
			m_hWaitEvents.Add(hPauseEvent);
			m_hWaitEvents.Add(m_AbortEvent);

			if ( (Event = WSAWaitForMultipleEvents((DWORD)m_hWaitEvents.GetSize(), (WSAEVENT *)m_hWaitEvents.GetData(), FALSE, WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED ) {
				m_nLastError = WSAGetLastError();
				return FALSE;
			} else if ( Event >= WSA_WAIT_EVENT_0 && (Event -= WSA_WAIT_EVENT_0) < (DWORD)m_hWaitEvents.GetSize() && m_hWaitEvents[Event] == m_AbortEvent ) {
				m_bAbort = TRUE;
				return TRUE;
			}

			if ( pParam != NULL ) {
				m_SSL_pSock = (SSL *)pParam[1];
				fdEvents |= FD_READ;
			}
			hPauseEvent = NULL;
		}

		m_hWaitEvents.RemoveAll();

		if ( ReadWriteEvent() <= 0 && (bClose & 002) != 0 )
			break;

		if ( (bClose & 010) == 0 ) {
			if ( fdEvents != oldEvents ) {
				if ( WSAEventSelect(m_hSocket, m_SockEvent, fdEvents) == SOCKET_ERROR ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				}
				oldEvents = fdEvents;
			}
			m_hWaitEvents.Add((HANDLE)m_SockEvent);
		}

		m_hWaitEvents.Add(m_MsgEvent);
		m_hWaitEvents.Add(m_AbortEvent);

		if ( RecvTimeOut != 0 ) {
			if ( SendTimeOut != 0 && SendTimeOut < RecvTimeOut )
				TimeOut = SendTimeOut;
			else
				TimeOut = RecvTimeOut;
		} else if ( SendTimeOut != 0 )
			TimeOut = SendTimeOut;
		else
			TimeOut = WSA_INFINITE;

		if ( (Event = WSAWaitForMultipleEvents((DWORD)m_hWaitEvents.GetSize(), (WSAEVENT *)m_hWaitEvents.GetData(), FALSE, TimeOut, FALSE)) == WSA_WAIT_FAILED ) {
			m_nLastError = WSAGetLastError();
			return FALSE;
		}

		if ( Event == WSA_WAIT_TIMEOUT ) {
			RecvTimeOut = SendTimeOut = 0;
			NetworkEvents.lNetworkEvents = 0;
			if ( (fdEvents & FD_WRITE) == 0 ) {
				NetworkEvents.lNetworkEvents |= FD_WRITE;
				NetworkEvents.iErrorCode[FD_WRITE_BIT] = 0;
			}
			if ( (fdEvents & FD_READ) == 0 ) {
				NetworkEvents.lNetworkEvents |= FD_READ;
				NetworkEvents.iErrorCode[FD_READ_BIT] = 0;
			}
			if ( NetworkEvents.lNetworkEvents != 0 )
				goto SOCKETEVENT;

		} else if ( Event < WSA_WAIT_EVENT_0 || (Event -= WSA_WAIT_EVENT_0) >= (DWORD)m_hWaitEvents.GetSize() )
			continue;

		if ( m_hWaitEvents[Event] == m_AbortEvent ) {
			m_bAbort = TRUE;
			break;

		} else if ( m_hWaitEvents[Event] == m_SockEvent ) {
			if ( WSAEnumNetworkEvents(m_hSocket, m_SockEvent, &NetworkEvents) == SOCKET_ERROR ) {
				m_nLastError = WSAGetLastError();
				return FALSE;
			}

			if ( (NetworkEvents.lNetworkEvents & FD_CONNECT) != 0 ) {
				if ( NetworkEvents.iErrorCode[FD_CONNECT_BIT] != 0 ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				}  else
					SendFdEvents(FIFO_STDOUT, FD_CONNECT, NULL);
			}

			SOCKETEVENT:

			if ( (NetworkEvents.lNetworkEvents & FD_READ) != 0 ) {
				if ( NetworkEvents.iErrorCode[FD_READ_BIT] != 0 ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				} else {
					if ( sslErr == (0x200 | SSL_ERROR_WANT_READ) )	// SSL_write is SSL_ERROR_WANT_READ
						goto SSLWRITERETRY;
					SSLREADRETRY:
					sslErr = 0;

					for ( ; ; ) {
						if ( m_nLimitSize > 0 ) {
							n = (int)((now = clock()) - RecvClock);
							RecvClock = now;
							if ( n >= 0 && n < (CLOCKS_PER_SEC * 3) && (RecvSize -= ((LONGLONG)m_nLimitSize * n / CLOCKS_PER_SEC)) > 0 && (RecvTimeOut = (int)(RecvSize * 1000 / m_nLimitSize)) > 0 ) {
								fdEvents &= ~FD_READ;
								break;
							} else {
								RecvSize = 0;
								RecvTimeOut = 0;
							}
						}

						if ( GetDataSize(FIFO_STDOUT) >= FIFO_BUFUPPER ) {
							fdEvents &= ~FD_READ;
							break;
						} else {
							if ( m_SSL_pSock != NULL ) {
								if ( (len = SSL_read(m_SSL_pSock, (void *)buffer, m_nBufSize)) < 0 ) {
									if ( (sslErr = SSL_get_error(m_SSL_pSock, len)) != SSL_ERROR_WANT_READ && sslErr != SSL_ERROR_WANT_WRITE ) {
										m_nLastError = WSAECONNABORTED;
										return FALSE;
									}
									fdEvents |= (sslErr == SSL_ERROR_WANT_READ ? FD_READ : FD_WRITE);
									sslErr |= 0x100;	// SSL_read mark
									break;
								}
							} else
								len = ::recv(m_hSocket, (char *)buffer, m_nBufSize, 0);

							if ( len <= 0 ) {
								fdEvents |= FD_READ;
								break;
							} else if ( Write(FIFO_STDOUT, buffer, len) < 0 ) {
								fdEvents &= ~FD_READ;
								bClose |= 002;
								break;
							} else
								RecvSize += len;
						}
					}
				}
			}

			if ( (NetworkEvents.lNetworkEvents & FD_WRITE) != 0 ) {
				if ( NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0 ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				} else {
					if ( sslErr == (0x100 | SSL_ERROR_WANT_WRITE) )	// SSL_read is SSL_ERROR_WANT_WRITE
						goto SSLREADRETRY;
					SSLWRITERETRY:
					sslErr = 0;

					for ( ; ; ) {
						if ( m_nLimitSize > 0 ) {
							n = (int)((now = clock()) - SendClock);
							SendClock = now;
							if ( n >= 0 && n < (CLOCKS_PER_SEC * 3) && (SendSize -= ((LONGLONG)m_nLimitSize * n / CLOCKS_PER_SEC)) > 0 && (SendTimeOut = (int)(SendSize * 1000 / m_nLimitSize)) > 0 ) {
								fdEvents &= ~FD_WRITE;
								break;
							} else {
								SendSize = 0;
								SendTimeOut = 0;
							}
						}

						if ( !OobList.IsEmpty() ) {
							FifoMsg *pFifoMsg = OobList.RemoveHead();
							if ( (len = ::send(m_hSocket, (const char *)pFifoMsg->buffer, pFifoMsg->len, MSG_OOB)) <= 0 ) {
								fdEvents |= FD_WRITE;
								break;
							} else if ( (pFifoMsg->len -= len) > 0 ) {
								memmove(pFifoMsg->buffer, pFifoMsg->buffer + len, pFifoMsg->len);
								OobList.AddHead(pFifoMsg);
								SendSize += len;
							} else {
								DeleteMsg(pFifoMsg);
								SendSize += len;
							}
						} else if ( (n = Peek(FIFO_STDIN, buffer, m_nBufSize)) == 0 ) {
							fdEvents &= ~FD_WRITE;
							break;
						} else if ( n < 0 ) {
							fdEvents &= ~FD_WRITE;
							bClose |= 001;
							break;
						} else {
							if ( m_SSL_pSock != NULL ) {
								if ( (len = SSL_write(m_SSL_pSock, (const char *)buffer, n)) < 0 ) {
									if ( (sslErr = SSL_get_error(m_SSL_pSock, len)) != SSL_ERROR_WANT_WRITE && sslErr != SSL_ERROR_WANT_READ ) {
										m_nLastError = WSAECONNABORTED;
										return FALSE;
									}
									fdEvents |= (sslErr == SSL_ERROR_WANT_READ ? FD_READ : FD_WRITE);
									sslErr |= 0x200;	// SSL_write mark
									break;
								}
							} else
								len = ::send(m_hSocket, (const char *)buffer, n, 0);

							if ( len <= 0 ) {
								fdEvents |= FD_WRITE;
								break;
							} else {
								Consume(FIFO_STDIN, len);
								SendSize += len;
							}
						}
					}
				}
			}

			if ( (NetworkEvents.lNetworkEvents & FD_OOB) != 0 ) {
				if ( NetworkEvents.iErrorCode[FD_OOB_BIT] != 0 ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				} else if ( (len = ::recv(m_hSocket, (CHAR *)buffer, m_nBufSize, MSG_OOB)) <= 0 ) {
					fdEvents |= FD_OOB;
				} else {
					WSABUF sbuf;
					sbuf.len = len;
					sbuf.buf = (CHAR *)buffer;
					SendFdEvents(FIFO_STDOUT, FD_OOB, &sbuf);
				}
			}

			if ( (NetworkEvents.lNetworkEvents & FD_CLOSE) != 0 ) {
				if ( NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0 ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				} else {
					Read(FIFO_STDIN, NULL, 0);			// Endof Set
					Write(FIFO_STDOUT, NULL, 0);		// Endof Set
					bClose |= 003;
				}
			}

		} else if ( m_hWaitEvents[Event] == GetEvent(FIFO_STDIN)->m_hObject ) {
			if ( (fdEvents & FD_WRITE) == 0 ) {
				NetworkEvents.lNetworkEvents = FD_WRITE;
				NetworkEvents.iErrorCode[FD_WRITE_BIT] = 0;
				goto SOCKETEVENT;
			}

		} else if ( m_hWaitEvents[Event] == GetEvent(FIFO_STDOUT)->m_hObject ) {
			if ( (fdEvents & FD_READ) == 0 && GetDataSize(FIFO_STDOUT) <= FIFO_BUFLOWER ) {
				NetworkEvents.lNetworkEvents = FD_READ;
				NetworkEvents.iErrorCode[FD_READ_BIT] = 0;
				goto SOCKETEVENT;
			}

		} else if ( m_hWaitEvents[Event] == m_MsgEvent ) {
			m_MsgSemaphore.Lock();
			while ( !m_MsgList.IsEmpty() ) {
				FifoMsg *pFifoMsg = m_MsgList.RemoveHead();
				m_MsgSemaphore.Unlock();

				switch(pFifoMsg->cmd) {
				case FIFO_CMD_FDEVENTS:
					switch(pFifoMsg->msg) {
					case FD_OOB:
						OobList.AddTail(pFifoMsg);
						pFifoMsg = NULL;
						break;
					case FD_CONNECT:
						SendFdEvents(FIFO_STDIN, FD_WRITE, NULL);
						SendFdEvents(FIFO_STDOUT, FD_READ, NULL);
						bReadWrite = TRUE;
						break;
					case FD_CLOSE:
						m_nLastError = (int)pFifoMsg->buffer;
						Read(FIFO_STDIN, NULL, 0);
						Write(FIFO_STDOUT, NULL, 0);
						bClose |= 003;
						break;
					}
					break;

				case FIFO_CMD_MSGQUEIN:
					switch(pFifoMsg->param) {
					case FIFO_QCMD_LIMITSIZE:
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_LIMITSIZE, LimitByteSec);
						if ( (m_nLimitSize = pFifoMsg->msg) > 0 ) {
							if ( (m_nBufSize = m_nLimitSize / 16) < 32 )
								m_nBufSize = 32;
						} else
							m_nBufSize = nDefBufSize;
						buffer = TempBuffer(m_nBufSize);
						break;

					case FIFO_QCMD_GETSOCKOPT:
						// BOOL opval;
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_GETSOCKOPT, SO_KEEPALIVE, 0, (void *)&opval);
						n = sizeof(BOOL);
						if ( ::getsockopt(m_hSocket, SOL_SOCKET, pFifoMsg->msg, (char *)(pFifoMsg->buffer), (int *)&n) == 0 ) {
							if ( pFifoMsg->pResult != NULL )
								*(pFifoMsg->pResult) = TRUE;
						}
						break;
					case FIFO_QCMD_SETSOCKOPT:
						// BOOL opval;
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, SO_KEEPALIVE, 0, (void *)&opval);
						if ( ::setsockopt(m_hSocket, SOL_SOCKET, pFifoMsg->msg, (const char *)(pFifoMsg->buffer), sizeof(BOOL)) == 0 ) {
							if ( pFifoMsg->pResult != NULL )
								*(pFifoMsg->pResult) = TRUE;
						}
						break;
					case FIFO_QCMD_IOCtl:
						// SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_IOCtl, lCommand, 0, (void *)lpArgument);
						if ( ::ioctlsocket(m_hSocket, (long)pFifoMsg->msg, (u_long *)pFifoMsg->buffer) == 0 ) {
							if ( pFifoMsg->pResult != NULL )
								*(pFifoMsg->pResult) = TRUE;
						}
						break;
					case FIFO_QCMD_SSLCONNECT:
						// SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SSLCONNECT, 0, 0, (void *)param);
						pParam = (void **)(pFifoMsg->buffer);
						hPauseEvent = (HANDLE)pParam[0];
						m_SSL_pSock = (SSL *)pParam[1];
						if ( hPauseEvent != NULL ) {
							//　WSAEventSelectを閉じて待たないとSSL_connectが失敗する
							oldEvents = 0;
							WSAEventSelect(m_hSocket, NULL, 0);
						}
						if ( pFifoMsg->pResult != NULL )
							*(pFifoMsg->pResult) = TRUE;
						break;
					}
					break;
				}

				if ( pFifoMsg != NULL )
					DeleteMsg(pFifoMsg);
				m_MsgSemaphore.Lock();
			}
			m_MsgSemaphore.Unlock();

			if ( bReadWrite ) {
				bReadWrite = FALSE;
				NetworkEvents.lNetworkEvents = FD_READ | FD_WRITE;
				NetworkEvents.iErrorCode[FD_READ_BIT] = 0;
				NetworkEvents.iErrorCode[FD_WRITE_BIT] = 0;
				goto SOCKETEVENT;

			} else if ( !OobList.IsEmpty() && (fdEvents & FD_WRITE) == 0 ) {
				NetworkEvents.lNetworkEvents = FD_WRITE;
				NetworkEvents.iErrorCode[FD_WRITE_BIT] = 0;
				goto SOCKETEVENT;
			}
		}
	}

	return TRUE;
}

///////////////////////////////////////////////////////
// CFifoListen

CFifoListen::CFifoListen(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoASync(pDoc, pSock)
{
	m_Threadtatus = 0;
	m_pWinThread = NULL;
}
CFifoListen::~CFifoListen()
{
	Close();
}
void CFifoListen::Destroy()
{
	Close();
	CFifoASync::Destroy();
}
static UINT ListenWorker(LPVOID pParam)
{
	CFifoListen *pThis = (CFifoListen *)pParam;

	pThis->m_Threadtatus = (pThis->AddInfoOpen() && pThis->ListenProc() ? 0 : 1);

	if ( !pThis->m_bAbort )
		pThis->SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)pThis->m_nLastError);

	pThis->RemoveAllMsg();

	return 0;
}
BOOL CFifoListen::Open(LPCTSTR lpszHostAddress, UINT nHostPort, int nFamily, int nSocketType, int nBacklog)
{
	Close();

	m_HostAddress = lpszHostAddress;
	m_nHostPort   = nHostPort;
	m_nFamily     = nFamily;
	m_nSocketType = nSocketType;
	m_nBacklog    = nBacklog;

	m_Threadtatus = (-1);

	if ( (m_pWinThread = AfxBeginThread(ListenWorker, this, THREAD_PRIORITY_NORMAL)) == NULL )
		return FALSE;

	return TRUE;
}
BOOL CFifoListen::Close()
{
	if ( m_pWinThread != NULL ) {
		m_AbortEvent.SetEvent();
		WaitForSingleObject(*m_pWinThread, INFINITE);
		m_pWinThread = NULL;
	}

	for ( int n = 0 ; n < m_SocketEvent.GetSize() ; n++ )
		WSACloseEvent(m_SocketEvent[n]);
	m_SocketEvent.RemoveAll();

	for ( int n = 0 ; n < m_Socket.GetSize() ; n++ ) {
		if ( m_Socket[n] != INVALID_SOCKET )
			::closesocket(m_Socket[n]);
	}
	m_Socket.RemoveAll();

	return TRUE;
}
BOOL CFifoListen::AddInfoOpen()
{
	ADDRINFOT hints;
	ADDRINFOT *ai, *aitop;
	CString ServicsName;
	SOCKET socket;
	WSAEVENT sevent;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = m_nFamily;
	hints.ai_socktype = m_nSocketType;
	hints.ai_flags = AI_PASSIVE;
	ServicsName.Format(_T("%d"), m_nHostPort);

	if ( GetAddrInfo(m_HostAddress, ServicsName, &hints, &aitop) != 0 ) {
		m_nLastError = WSAGetLastError();
		return FALSE;
	}

	for ( ai = aitop ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (socket = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) ) {
			m_nLastError = WSAGetLastError();
			continue;
		}

		if ( ::bind(socket, ai->ai_addr, (int)ai->ai_addrlen) < 0 || ::listen(socket, m_nBacklog) == SOCKET_ERROR ) {
			m_nLastError = WSAGetLastError();
			::closesocket(socket);
			continue;
		}

		if ( (sevent = WSACreateEvent()) == WSA_INVALID_EVENT ) {
			m_nLastError = WSAGetLastError();
			::closesocket(socket);
			continue;
		}

		if ( WSAEventSelect(socket, sevent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR ) {
			m_nLastError = WSAGetLastError();
			::closesocket(socket);
			continue;
		}

		m_SocketEvent.Add(sevent);
		m_Socket.Add(socket);
	}

	FreeAddrInfo(aitop);
	return (m_Socket.GetSize() <= 0 ? FALSE : TRUE);
}
BOOL CFifoListen::ListenProc()
{
	int idx;
	DWORD Event;
	WSANETWORKEVENTS NetworkEvents;
	CArray<HANDLE, HANDLE> WaitEvent;

	for ( ; ; ) {
		WaitEvent.RemoveAll();

		for ( idx = 0 ; idx < m_SocketEvent.GetSize() ; idx++ ) {
			if ( m_Socket[idx] != INVALID_SOCKET )
				WaitEvent.Add(m_SocketEvent[idx]);
		}

		if ( WaitEvent.GetSize() <= 0 )
			break;

		WaitEvent.Add(m_MsgEvent);
		WaitEvent.Add(m_AbortEvent);

		if ( (Event = WSAWaitForMultipleEvents((DWORD)WaitEvent.GetSize(), (WSAEVENT *)WaitEvent.GetData(), FALSE, WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED ) {
			m_nLastError = WSAGetLastError();
			return FALSE;
		}

		if ( Event < WSA_WAIT_EVENT_0 || (Event -= WSA_WAIT_EVENT_0) >= (DWORD)WaitEvent.GetSize() )
			continue;

		if ( WaitEvent[Event] == m_AbortEvent ) {
			m_bAbort = TRUE;
			break;

		} else if ( WaitEvent[Event] == m_MsgEvent ) {
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
				}

				if ( pFifoMsg != NULL )
					DeleteMsg(pFifoMsg);
				m_MsgSemaphore.Lock();
			}
			m_MsgSemaphore.Unlock();

		} else {
			for ( idx = 0 ; idx < m_SocketEvent.GetSize() ; idx++ ) {
				if ( WaitEvent[Event] == m_SocketEvent[idx] )
					break;
			}
			if ( idx >= m_SocketEvent.GetSize() )
				continue;

			if ( WSAEnumNetworkEvents(m_Socket[idx], m_SocketEvent[idx], &NetworkEvents) == SOCKET_ERROR ) {
				m_nLastError = WSAGetLastError();
				return FALSE;
			}

			if ( (NetworkEvents.lNetworkEvents & FD_ACCEPT) != 0 ) {
				if ( NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0 ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				}
				int len;
				struct sockaddr_storage sa;
				SOCKET scoket;

				memset(&sa, 0, sizeof(sa));
				len = sizeof(sa);
			    if ( (scoket = ::accept(m_Socket[idx], (struct sockaddr *)&sa, &len)) != INVALID_SOCKET )
					SendFdEvents(FIFO_STDOUT, FD_ACCEPT, (void *)scoket);
			}

			if ( (NetworkEvents.lNetworkEvents & FD_CLOSE) != 0 ) {
				if ( NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0 ) {
					m_nLastError = WSAGetLastError();
					return FALSE;
				}

				::closesocket(m_Socket[idx]);
				m_Socket[idx] = INVALID_SOCKET;
			}
		}
	}

	return TRUE;
}

///////////////////////////////////////////////////////
// CFifoProxy

/*
CFifoSocket		CFifoProxy					CFifoMiddle									CFifoDocument
	STDOUT		STDIN->ProxyCheck	EXTOUT	STDIN->OnRecvSocket  SendDocument->EXTOUT	STDIN->OnRecvDocument->CDocument::OnSocketReceive
	STDINT		STDOUT<-ProxyCheck	EXTIN	STDOUT<-SendSocket	OnRecvProtocol<-EXTIN	STDOUT<-SendProtocol<-CDocument::SocketSend
*/
CFifoProxy::CFifoProxy(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoWnd(pDoc, pSock)
{
}
CFifoProxy::~CFifoProxy()
{
}
void CFifoProxy::OnRead(int nFd)
{
	// ProxyCheck内でCFifoProxyをUnLink/Destroyするので注意
	if ( nFd == FIFO_STDIN )
		m_pSock->ProxyCheck();
}
void CFifoProxy::OnWrite(int nFd)
{
	ResetFdEvents(nFd, FD_WRITE);
}
void CFifoProxy::OnConnect(int nFd)
{
	if ( nFd != FIFO_STDIN )
		return;

	if ( m_pSock->m_SSL_mode != 0 && !m_pSock->SSLConnect() ) {
		m_pSock->m_ProxyStatus = PRST_NONE;
		m_pSock->m_ProxyLastError = WSAECONNABORTED;
		SendFdEvents(FIFO_EXTOUT, FD_CLOSE, (void *)m_pSock->m_ProxyLastError);
	}
	m_pSock->ProxyCheck();
}
void CFifoProxy::OnClose(int nFd, int nLastError)
{
	m_nLastError = nLastError;

	if ( nFd == FIFO_EXTIN )
		SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)nLastError);
}

///////////////////////////////////////////////////////
// CFifoDocument
/*
CFifoSocket		CFifoMiddle									CFifoDocument
	STDOUT		STDIN->OnRecvSocket	  SendDocument->EXTOUT	STDIN->OnRecvDocument->CDocument::OnSocketReceive
	STDINT		STDOUT<-SendSocket	 OnRecvProtocol<-EXTIN	STDOUT<-SendProtocol<-CDocument::SocketSend
*/

CFifoDocument::CFifoDocument(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoWnd(pDoc, pSock)
{
	m_RecvStart = 0;
	m_bContinue = FALSE;
	m_bClosed = FALSE;
	((CRLoginApp *)AfxGetApp())->AddIdleProc(IDLEPROC_FIFODOC, this);
}
CFifoDocument::~CFifoDocument()
{
	((CRLoginApp *)AfxGetApp())->DelIdleProc(IDLEPROC_FIFODOC, this);
}
void CFifoDocument::EndOfData(int nFd)
{
	if ( m_nLastError == 0 )
		m_pDocument->OnSocketClose();
	else
		m_pDocument->OnSocketError(m_nLastError);
}
void CFifoDocument::OnRead(int nFd)
{
	int readlen, complet, remain;
	int line = 0, remline = (-1);
	BYTE *buffer = TempBuffer(m_nBufSize);

	ASSERT(nFd == FIFO_STDIN);

	// タイマーをスタート、OnIdleでリセットする
	if ( m_RecvStart == 0 )
		m_RecvStart = clock();

	while ( !m_bDestroy ) {
		if ( (readlen = Peek(nFd, buffer, m_nBufSize)) < 0 ) {
			// End of data
			ResetFdEvents(nFd, FD_READ);
			if ( m_bClosed )
				EndOfData(nFd);
			else
				m_bContinue = m_bClosed = TRUE;
			break;
		} else if ( readlen == 0  ) {
			// No data
			break;
		}

		complet = m_pSock->OnRecvDocument((void *)buffer, readlen, 0);
		remain = Consume(nFd, complet);

		if ( m_pDocument->SocketSyncMode() ) {
			// Socket Sync mode chenge
			break;
		} else if ( remain < 0 ) {
			// End of data
			continue;
		} else if ( remain == 0 ) {
			// No data
			break;
		} else if ( (clock() - m_RecvStart) >= (50 * 1000 / CLOCKS_PER_SEC) ) {
			// 50ms over
			m_bContinue = TRUE;
			ResetFdEvents(nFd, FD_READ);
			break;
		} else if ( complet < readlen ) {
			// OnReceiveProcBack interrupted. smooth scroll mode
			if ( remline < 0 )
				remline = remain * 10 / m_pDocument->m_TextRam.m_Cols / 12;	// remain / (cols * 6 / 10) / 2
			if ( ++line < remline )
				continue;
			m_bContinue = TRUE;
			ResetFdEvents(nFd, FD_READ);
			break;
		}
	}
}
void CFifoDocument::OnWrite(int nFd)
{
	ResetFdEvents(nFd, FD_WRITE);
}
void CFifoDocument::OnConnect(int nFd)
{
	ASSERT(nFd == FIFO_STDIN);
	m_pDocument->OnSocketConnect();
}
void CFifoDocument::OnClose(int nFd, int nLastError)
{
	if ( nFd == FIFO_STDIN ) {
		m_nLastError = nLastError;
	} else if ( nFd == FIFO_STDOUT ) {
		SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)nLastError);
		Write(FIFO_STDOUT, NULL, 0);
	}
}
BOOL CFifoDocument::OnIdle()
{
	// タイマーリセット
	m_RecvStart = 0;

	if ( m_bContinue ) {
		m_bContinue = FALSE;
		SetFdEvents(FIFO_STDIN, FD_READ);
	}

	return FALSE;
}
