//////////////////////////////////////////////////////////////////////
// ExtSocket.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "PipeSock.h"
#include "ComSock.h"
#include "PassDlg.h"
#include "ssh.h"
#include "HttpCtx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CExtSocket

static const int EventMask[2] = { FD_READ, FD_OOB };

CExtSocket::CExtSocket(class CRLoginDoc *pDoc)
{
	m_pDocument = pDoc;
	m_Type = ESCT_DIRECT;

	m_Fd = (-1);
	m_RecvBufSize = RECVDEFSIZ;

	m_RealHostPort = 0;
	m_RealRemotePort = 0;
	m_RealSocketType = 0;

	m_ProxyStatus = PRST_NONE;
	m_ProxyLastError = 0;
	m_ProxyCode = 0;
	m_ProxyPort = 0;
	m_ProxyLength = 0;
	m_ProxyNum = 0;
	m_ProxyConnect = FALSE;
	m_ProxyCmdMode = FALSE;

	m_SSL_mode  = 0;
	m_SSL_pCtx  = NULL;
	m_SSL_pSock = NULL;
	m_SSL_keep  = FALSE;

	m_bConnect = FALSE;

	m_pFifoLeft   = NULL;
	m_pFifoMid    = NULL;
	m_pFifoRight  = NULL;
	m_pFifoSync   = NULL;
	m_pFifoProxy  = NULL;
	m_pFifoSSL    = NULL;

	m_pHttp2Ctx = NULL;
	m_pHttp3Ctx = NULL;
}
CExtSocket::~CExtSocket()
{
	Close();
}
void CExtSocket::Destroy()
{
	Close();
	delete this;
}

//////////////////////////////////////////////////////////////////////

CFifoBase *CExtSocket::FifoLinkLeft()
{
	if ( m_ProxyCmdMode )
		return new CFifoPipe(m_pDocument, this);
	else
		return new CFifoSocket(m_pDocument, this);
}
CFifoBase *CExtSocket::FifoLinkMid()
{
	return new CFifoWnd(m_pDocument, this);
}
CFifoDocument *CExtSocket::FifoLinkRight()
{
	return new CFifoDocument(m_pDocument, this);
}
void CExtSocket::FifoLink()
{
	FifoUnlink();

	ASSERT(m_pFifoLeft == NULL && m_pFifoMid == NULL && m_pFifoRight == NULL);

	m_pFifoLeft  = FifoLinkLeft();
	m_pFifoMid   = FifoLinkMid();
	m_pFifoRight = FifoLinkRight();

	CFifoBase::MidLink(m_pFifoLeft, FIFO_STDIN, m_pFifoMid, FIFO_STDIN, m_pFifoRight, FIFO_STDIN);
}
void CExtSocket::FifoUnlink()
{
	if ( m_pFifoSync != NULL ) {
		CFifoBase::UnLink(m_pFifoSync, FIFO_STDIN, TRUE);
		m_pFifoSync->Destroy();
		m_pFifoSync = NULL;
	}

	if ( m_pFifoProxy != NULL ) {
		CFifoBase::UnLink(m_pFifoProxy, FIFO_STDIN, TRUE);
		m_pFifoProxy->Destroy();
		m_pFifoProxy = NULL;
	}

	if ( m_pFifoSSL != NULL ) {
		CFifoBase::UnLink(m_pFifoSSL, FIFO_STDIN, TRUE);
		m_pFifoSSL->Destroy();
		m_pFifoSSL = NULL;
	}

	if ( m_pFifoLeft != NULL && m_pFifoMid != NULL && m_pFifoRight != NULL )
		CFifoBase::MidUnLink(m_pFifoLeft, FIFO_STDIN, m_pFifoMid, FIFO_STDIN, m_pFifoRight, FIFO_STDIN);

	if ( m_pFifoRight != NULL ) {
		m_pFifoRight->Destroy();
		m_pFifoRight = NULL;
	}
	if ( m_pFifoMid != NULL ) {
		m_pFifoMid->Destroy();
		m_pFifoMid = NULL;
	}
	if ( m_pFifoLeft != NULL ) {
		m_pFifoLeft->Destroy();
		m_pFifoLeft = NULL;
	}
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::PostClose(int nLastError)
{
	ASSERT(m_pFifoLeft != NULL);

	m_pFifoLeft->SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)(UINT_PTR)nLastError);
	m_pFifoLeft->Write(FIFO_STDOUT, NULL, 0);
}
int CExtSocket::GetFamily()
{
	if ( m_pDocument == NULL )
		return AF_UNSPEC;

	switch(m_pDocument->m_ParamTab.m_SelIPver) {
	case SEL_IPV6V4:
		return AF_UNSPEC;
	case SEL_IPV6:
		return AF_INET6;
	case SEL_IPV4:
		return AF_INET;
	}

	return AF_UNSPEC;
}
BOOL CExtSocket::ProxyOpen(int mode, BOOL keep, LPCTSTR ProxyHost, UINT ProxyPort, LPCTSTR ProxyUser, LPCTSTR ProxyPass, LPCTSTR RealHost, UINT RealPort)
{
	switch(mode & 7) {
	case 0: m_ProxyStatus = PRST_NONE;				break;	// Non
	case 1: m_ProxyStatus = PRST_HTTP_START;		break;	// HTTP
	case 2: m_ProxyStatus = PRST_SOCKS4_START;		break;	// SOCKS4
	case 3: m_ProxyStatus = PRST_SOCKS5_START;		break;	// SOCKS5
	case 4: m_ProxyStatus = PRST_HTTP_BASIC_START;	break;	// HTTP(Basic)
	case 6: m_ProxyStatus = PRST_HTTP2_START;		break;	// HTTP/2
	case 7: m_ProxyStatus = PRST_HTTP3_START;		break;	// HTTP/3
	}

	switch(mode >> 3) {
	case 0:  m_SSL_mode = 0; break;									// Non
	case 7:  m_SSL_mode = 2; m_RealSocketType = SOCK_DGRAM; break;	// QUIC
	default: m_SSL_mode = 1; break;									// SSL/TLS
	}

	m_SSL_keep = (m_ProxyStatus != PRST_NONE && m_SSL_mode == 1 ? keep : FALSE);
	m_ProxyLastError = 0;
	m_SSL_alpn.Empty();

	m_ProxyHostAddr = RealHost;
	PunyCodeAdress(RealHost, m_ProxyHost);

	m_ProxyPort = RealPort;
	m_ProxyUser = ProxyUser;
	m_ProxyPass = ProxyPass;

	if ( m_ProxyStatus == PRST_NONE ) {
		ProxyHost = RealHost;
		ProxyPort = RealPort;
	}

	return Open(ProxyHost, ProxyPort, 0, SOCK_STREAM);
}
BOOL CExtSocket::ProxyCommand(LPCTSTR pCommand)
{
	CString cmd = pCommand;

	m_ProxyCmdMode = TRUE;
	m_pDocument->EntryText(cmd);

	if ( cmd.IsEmpty() )
		return FALSE;

	return Open(cmd, 0);
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	CString host;

	m_RealHostAddr   = lpszHostAddress;
	m_RealHostPort   = nHostPort;
	m_RealRemotePort = nSocketPort;
	m_RealSocketType = nSocketType;

	FifoLink();

	if ( m_ProxyStatus != PRST_NONE ) {
		m_pFifoProxy = new CFifoProxy(m_pDocument, this);
		CFifoBase::Link(m_pFifoProxy, FIFO_STDIN, m_pFifoMid, FIFO_STDIN);
	}
	if ( m_SSL_mode != 0 ) {
		m_pFifoSSL = new CFifoSSL(m_pDocument, this);
		CFifoBase::Link(m_pFifoSSL, FIFO_STDIN, (m_pFifoProxy != NULL ? m_pFifoProxy : m_pFifoMid), FIFO_STDIN);
	}

	m_pFifoLeft->m_nBufSize = m_pFifoMid->m_nBufSize = m_RecvBufSize;

	switch(m_Type) {
	case ESCT_DIRECT:
	case ESCT_RLOGIN:
	case ESCT_TELNET:
	case ESCT_COMDEV:
	case ESCT_PIPE:
		m_pFifoMid->m_bFlowCtrl = TRUE;
		break;
	case ESCT_SSH_MAIN:
	case ESCT_SSH_CHAN:
		m_pFifoMid->m_bFlowCtrl = FALSE;
		break;
	}
	
	switch(m_pFifoLeft->m_Type) {
	case FIFO_TYPE_SOCKET:
		m_pFifoLeft->m_nLimitSize = m_pDocument->m_TextRam.IsOptEnable(TO_RLTRSLIMIT) ? (m_pDocument->m_ParamTab.m_TransmitLimit * 1024) : 0;
		PunyCodeAdress(m_RealHostAddr, host);
		return ((CFifoSocket *)m_pFifoLeft)->Open(host, m_RealHostPort, m_RealRemotePort, GetFamily(), m_RealSocketType);

	case FIFO_TYPE_PIPE:
		return ((CFifoPipe *)m_pFifoLeft)->Open(lpszHostAddress);
	}

	return FALSE;
}
void CExtSocket::Close()
{
	SSLClose();
	FifoUnlink();
	m_bConnect = FALSE;

	if ( m_pHttp2Ctx != NULL ) {
		delete m_pHttp2Ctx;
		m_pHttp2Ctx = NULL;
	}

	if ( m_pHttp3Ctx != NULL ) {
		delete m_pHttp3Ctx;
		m_pHttp3Ctx = NULL;
	}
}

//////////////////////////////////////////////////////////////////////

/*
CFifoSocket		CFifoMiddle									CFifoSync								CFifoDocument
	STDOUT		STDIN->OnRecvSocket	  SendDocument->EXTOUT	STDIN->SyncReceive	SyncExtSend->EXTOUT	STDIN->OnRecvDocument->CDocument::OnSocketReceive
	STDINT		STDOUT<-SendSocket	 OnRecvProtocol<-EXTIN	STDOUT<-SyncSend<-----------------EXTIN	STDOUT<-SendProtocol<-CDocument::SocketSend
*/

#define	SYNCABORTTIMEOUT	200

void CExtSocket::SetRecvSyncMode(BOOL mode)
{
	if ( mode ) {		// in Sync
		if ( m_pFifoSync != NULL )
			return;

		m_pFifoSync = new CFifoSync(m_pDocument, this);
		m_pFifoSync->m_nBufSize = FIFO_BUFSIZE * 2;
		m_pFifoSync->TunnelReadWrite(FIFO_EXTIN);

		CFifoBase::Link(m_pFifoSync, FIFO_STDIN, m_pFifoRight, FIFO_STDIN);

	} else {			// out Sync
		if ( m_pFifoSync == NULL )
			return;

		CFifoBase::UnLink(m_pFifoSync, FIFO_STDIN, TRUE);

		m_pFifoRight->SetFdEvents(FIFO_STDIN, FD_READ);
		m_pFifoRight->SetFdEvents(FIFO_STDOUT, FD_WRITE);

		m_pFifoSync->Destroy();
		m_pFifoSync = NULL;
	}
}
void CExtSocket::SyncRecvClear()
{
	int n;

	if ( m_pFifoSync == NULL )
		return;

	for ( ; ; ) {
		Sleep(SYNCABORTTIMEOUT);

		if ( (n = m_pFifoSync->GetDataSize(FIFO_STDIN)) <= 0 )
			break;
		else if ( n > 0 )
			m_pFifoSync->Consume(FIFO_STDIN, n);
	}
}
int CExtSocket::SyncReceive(void* lpBuf, int nBufLen, int mSec, BOOL *pAbort)
{
	if ( m_pFifoSync == NULL )
		return 0;

	if ( pAbort != NULL && *pAbort )
		mSec = SYNCABORTTIMEOUT;

	return m_pFifoSync->Recv(FIFO_STDIN, (BYTE *)lpBuf, nBufLen, mSec);
}
void CExtSocket::SyncReceiveBack(void *lpBuf, int nBufLen)
{
	if ( m_pFifoSync != NULL )
		m_pFifoSync->Insert(FIFO_STDIN, (BYTE *)lpBuf, nBufLen);
}
int CExtSocket::SyncSend(const void *lpBuf, int nBufLen, int mSec, BOOL *pAbort)
{
	if ( m_pFifoSync == NULL )
		return 0;

	if ( pAbort != NULL && *pAbort )
		mSec = SYNCABORTTIMEOUT;

	return m_pFifoSync->Send(FIFO_STDOUT, (BYTE *)lpBuf, nBufLen, mSec);
}
int CExtSocket::SyncExtSend(const void *lpBuf, int nBufLen, int mSec, BOOL *pAbort)
{
	if ( m_pFifoSync == NULL )
		return 0;

	if ( pAbort != NULL && *pAbort )
		mSec = SYNCABORTTIMEOUT;

	if ( (m_pFifoSync->GetDataSize(FIFO_EXTOUT) + nBufLen) >= (m_pFifoSync->m_nBufSize * 4) )
		return m_pFifoSync->Send(FIFO_EXTOUT, (BYTE *)lpBuf, nBufLen, mSec);
	else
		return m_pFifoSync->Write(FIFO_EXTOUT, (BYTE *)lpBuf, nBufLen);
}
void CExtSocket::SyncAbort()
{
	if ( m_pFifoSync != NULL )
		m_pFifoSync->Abort();
}

//////////////////////////////////////////////////////////////////////

/*
CFifoSocket		CFifoMiddle									CFifoDocument
	STDOUT		STDIN->OnRecvSocket	  SendDocument->EXTOUT	STDIN->OnRecvDocument->CDocument::OnSocketReceive
	STDINT		STDOUT<-SendSocket	 OnRecvProtocol<-EXTIN	STDOUT<-SendProtocol<-CDocument::SocketSend
*/

void CExtSocket::SendSocket(const void* lpBuf, int nBufLen, int nFlags)
{
	ASSERT(m_pFifoMid != NULL);

	if ( nFlags == 0 )
		m_pFifoMid->Write(FIFO_STDOUT, (BYTE *)lpBuf, nBufLen);
	else if ( nFlags == MSG_OOB ) {
		WSABUF sbuf;
		sbuf.len = nBufLen;
		sbuf.buf = (CHAR *)lpBuf;
		m_pFifoMid->SendFdEvents(FIFO_STDOUT, FD_OOB, (void *)&sbuf);
	}
}
void CExtSocket::OnRecvSocket(void *lpBuf, int nBufLen, int nFlags)
{
	SendDocument(lpBuf, nBufLen, nFlags);
}

void CExtSocket::SendProtocol(const void* lpBuf, int nBufLen, int nFlags)
{
	ASSERT(m_pFifoRight != NULL);

	if ( nFlags == 0 )
		m_pFifoRight->Write(FIFO_STDOUT, (BYTE *)lpBuf, nBufLen);
	else if ( nFlags == MSG_OOB ) {
		WSABUF sbuf;
		sbuf.len = nBufLen;
		sbuf.buf = (CHAR *)lpBuf;
		m_pFifoRight->SendFdEvents(FIFO_STDOUT, FD_OOB, (void *)&sbuf);
	}
}
void CExtSocket::OnRecvProtocol(void *lpBuf, int nBufLen, int nFlags)
{
	SendSocket(lpBuf, nBufLen, nFlags);
}

void CExtSocket::SendDocument(const void *lpBuf, int nBufLen, int nFlags)
{
	ASSERT(m_pFifoMid != NULL);

	if ( nFlags == 0 )
		m_pFifoMid->Write(FIFO_EXTOUT, (BYTE *)lpBuf, nBufLen);
	else if ( nFlags == MSG_OOB ) {
		WSABUF sbuf;
		sbuf.len = nBufLen;
		sbuf.buf = (CHAR *)lpBuf;
		m_pFifoMid->SendFdEvents(FIFO_EXTOUT, FD_OOB, (void *)&sbuf);
	}
}
int CExtSocket::OnRecvDocument(void *lpBuf, int nBufLen, int nFlags)
{
	ASSERT(m_pDocument != NULL);

	return m_pDocument->OnSocketReceive((LPBYTE)lpBuf, nBufLen, nFlags);
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::GetStatus(CString &str)
{
	CString tmp;
	CString name;
	int port;

	tmp.Format(_T("Socket Type: %d\r\n"), m_Type);
	str += tmp;
	if ( m_pFifoMid != NULL ) {
		tmp.Format(_T("Receive Reserved: %d + %d Bytes\r\n"), m_pFifoMid->GetDataSize(FIFO_STDIN), m_pFifoMid->GetDataSize(FIFO_EXTOUT));
		str += tmp;
		tmp.Format(_T("Send Reserved: %d + %d Bytes\r\n"), m_pFifoMid->GetDataSize(FIFO_STDOUT), m_pFifoMid->GetDataSize(FIFO_EXTIN));
		str += tmp;
	}

	if ( m_Fd != (-1) ) {
		GetSockName(m_Fd, name, &port);
		tmp.Format(_T("Connect: %s#%d"), (LPCTSTR)name, port);
		str += tmp;
		GetPeerName(m_Fd, name, &port);
		tmp.Format(_T(" - %s#%d\r\n"), (LPCTSTR)name, port);
		str += tmp;
	}

	if ( m_pFifoLeft != NULL && m_pFifoLeft->m_Type == FIFO_TYPE_SOCKET ) {
		if ( m_pFifoLeft->m_nLimitSize > 0 ) {
			tmp.Format(_T("Transmit Limits: %d Byte/Sec\r\n"), m_pFifoLeft->m_nLimitSize);
			str += tmp;
		}
		tmp.Format(_T("Read/Write Buffer size: %d Bytes\r\n"), m_pFifoLeft->m_nBufSize);
		str += tmp;
	}

	if ( m_pDocument != NULL && m_pDocument->m_ConnectTime != 0 ) {
		CTime tm = m_pDocument->m_ConnectTime;
		str += _T("Access Time: ");
		str += tm.Format(_T("%c"));
		int sec = (int)(time(NULL) - m_pDocument->m_ConnectTime);
		tmp.Format(_T(" (%02d:%02d:%02d)\r\n"), sec / 3600, (sec % 3600) / 60, sec % 60);
		str += tmp;
	}

	if ( !m_SSL_Msg.IsEmpty() ) {
		str += _T("\r\nOpenSSL: Cert Chain list\r\n");
		str += m_SSL_Msg;
	}
}
void CExtSocket::ResetOption()
{
	ASSERT(m_pDocument != NULL && m_pFifoLeft != NULL);

	if ( m_pFifoLeft->m_Type == FIFO_TYPE_SOCKET ) {
		BOOL opval;
		m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_LIMITSIZE, m_pDocument->m_TextRam.IsOptEnable(TO_RLTRSLIMIT) ? (m_pDocument->m_ParamTab.m_TransmitLimit * 1024) : 0);
		opval = m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) ? TRUE : FALSE;
		m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, SO_KEEPALIVE, 0, (void *)&opval);
		opval = m_pDocument->m_TextRam.IsOptEnable(TO_RLNODELAY) ? TRUE : FALSE;
		m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, TCP_NODELAY, 0, (void *)&opval);
	}
}

void CExtSocket::SendWindSize()
{
}
void CExtSocket::SendBreak(int opt)
{
}
void CExtSocket::SetXonXoff(int sw)
{
}

//////////////////////////////////////////////////////////////////////

int CExtSocket::GetRecvSize()
{
	if ( m_pFifoMid == NULL )
		return 0;

	return (m_pFifoMid->GetDataSize(FIFO_STDIN) + m_pFifoMid->GetDataSize(FIFO_EXTOUT));
}
int CExtSocket::GetSendSize()
{
	if ( m_pFifoMid == NULL )
		return 0;

	return (m_pFifoMid->GetDataSize(FIFO_STDOUT) + m_pFifoMid->GetDataSize(FIFO_EXTIN));
}
int CExtSocket::GetRecvProcSize()
{
	if ( m_pFifoMid == NULL )
		return 0;

	return m_pFifoMid->GetDataSize(FIFO_EXTOUT);
}
int CExtSocket::GetSendProcSize()
{
	if ( m_pFifoMid == NULL )
		return 0;

	return m_pFifoMid->GetDataSize(FIFO_EXTIN);
}
int CExtSocket::GetRecvSocketSize()
{
	if ( m_pFifoMid == NULL )
		return 0;

	return m_pFifoMid->GetDataSize(FIFO_STDIN);
}
int CExtSocket::GetSendSocketSize()
{
	if ( m_pFifoMid == NULL )
		return 0;

	return m_pFifoMid->GetDataSize(FIFO_STDOUT);
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::OnConnect()
{
	if ( m_pFifoLeft == NULL && m_pFifoMid == NULL && m_pFifoRight == NULL )
		FifoLink();

	if ( m_pFifoLeft != NULL && m_pFifoLeft->m_Type == FIFO_TYPE_SOCKET && m_pFifoLeft->IsOpen() ) {
		m_Fd = ((CFifoSocket *)m_pFifoLeft)->m_hSocket;

		if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) ) {
			BOOL opval = TRUE;
			m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, SO_KEEPALIVE, 0, (void *)&opval);
		}

		if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLNODELAY) ) {
			BOOL opval = TRUE;
			m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, TCP_NODELAY, 0, (void *)&opval);
		}
	}

	m_bConnect = TRUE;
}
void CExtSocket::OnClose()
{
	m_bConnect = FALSE;
}
void CExtSocket::OnError(int err)
{
	m_bConnect = FALSE;
}
void CExtSocket::OnAccept(SOCKET hand)
{
}
void CExtSocket::OnTimer(UINT_PTR nIDEvent)
{
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::IOCtl(long lCommand, DWORD *lpArgument)
{
	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET )
		return FALSE;

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_IOCtl, lCommand, 0, (void *)lpArgument);
}
BOOL CExtSocket::SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel)
{
	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET || nLevel != SOL_SOCKET || nOptionLen != sizeof(BOOL) )
		return FALSE;

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, nOptionName, 0, (void *)lpOptionValue);
}
BOOL CExtSocket::GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel)
{
	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET || nLevel != SOL_SOCKET || *lpOptionLen != sizeof(BOOL) )
		return FALSE;

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_GETSOCKOPT, nOptionName, 0, (void *)lpOptionValue);
}
BIO_ADDR *CExtSocket::GetPeerBioAddr(SOCKET fd)
{
	struct sockaddr_storage in;
	socklen_t inlen;
	BIO_ADDR *bio_addr = NULL;

	memset(&in, 0, sizeof(in));
	inlen = sizeof(in);
	getpeername(fd, (struct sockaddr *)&in, &inlen);

	switch(in.ss_family) {
	case AF_INET:
		if ( (bio_addr = BIO_ADDR_new()) != NULL ) {
			struct sockaddr_in *in4 = (struct sockaddr_in *)&in;
			BIO_ADDR_rawmake(bio_addr, in4->sin_family, (const void *)&(in4->sin_addr), sizeof(in4->sin_addr), in4->sin_port);
		}
		break;
	case AF_INET6:
		if ( (bio_addr = BIO_ADDR_new()) != NULL ) {
			struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&in;
			BIO_ADDR_rawmake(bio_addr, in6->sin6_family, (const void *)&(in6->sin6_addr), sizeof(in6->sin6_addr), in6->sin6_port);
		}
		break;
	}

	return bio_addr;
}
void CExtSocket::GetPeerName(SOCKET fd, CString &host, int *port)
{
	struct sockaddr_storage in;
	socklen_t inlen;
	char name[NI_MAXHOST], serv[NI_MAXSERV];

	memset(&in, 0, sizeof(in));
	memset(name, 0, sizeof(name));
	memset(serv, 0, sizeof(serv));
	inlen = sizeof(in);
	getpeername(fd, (struct sockaddr *)&in, &inlen);
	getnameinfo((struct sockaddr *)&in, inlen, name, sizeof(name), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
	host = name;
	*port = atoi(serv);
}
void CExtSocket::GetSockName(SOCKET fd, CString &host, int *port)
{
	struct sockaddr_storage in;
	socklen_t inlen;
	char name[NI_MAXHOST], serv[NI_MAXSERV];

	memset(&in, 0, sizeof(in));
	memset(name, 0, sizeof(name));
	memset(serv, 0, sizeof(serv));
	inlen = sizeof(in);
	getsockname(fd, (struct sockaddr *)&in, &inlen);
	getnameinfo((struct sockaddr *)&in, inlen, name, sizeof(name), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
	host = name;
	*port = atoi(serv);
}
void CExtSocket::GetHostName(struct sockaddr *addr, int addrlen, CString &host)
{
	char name[NI_MAXHOST], serv[NI_MAXSERV];

	memset(name, 0, sizeof(name));
	memset(serv, 0, sizeof(serv));
	getnameinfo(addr, (socklen_t)addrlen, name, sizeof(name), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
	host = name;
}
int CExtSocket::GetPortNum(LPCTSTR str)
{
	int n;
    struct servent *sp;

	if ( *str == 0 )
		return (-1);
	else if ( _tcsnicmp(str, _T("COM"), 3) == 0 )
		return (0 - _tstoi(str + 3));
	else if ( (n = _tstoi(str)) > 0 )
		return n;
	else if ( (sp = getservbyname(TstrToMbs(str), "tcp")) != NULL )
		return ntohs(sp->s_port);
	else if ( _tcscmp(str, _T("telnet")) == 0 )
		return 23;
	else if ( _tcscmp(str, _T("login")) == 0 )
		return 513;
	else if ( _tcscmp(str, _T("ssh")) == 0 )
		return 22;
	else if ( _tcscmp(str, _T("socks")) == 0 )
		return 1080;
	else if ( _tcscmp(str, _T("http")) == 0 )
		return 80;
	else if ( _tcscmp(str, _T("https")) == 0 )
		return 443;
	return 0;
}
BOOL CExtSocket::SokcetCheck(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	SOCKET Fd = INVALID_SOCKET;
	ADDRINFOT hints, *ai, *aitop;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
	CString str;
	struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = nSocketType;
	str.Format(_T("%d"), nHostPort);

	if ( GetAddrInfo(lpszHostAddress, str, &hints, &aitop) != 0)
		return FALSE;

	for ( ai = aitop ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (Fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == INVALID_SOCKET )
			continue;

		if ( nSocketPort != 0 ) {
			if ( ai->ai_family == AF_INET ) {
			    memset(&in, 0, sizeof(in));
			    in.sin_family = AF_INET;
			    in.sin_addr.s_addr = INADDR_ANY;
			    in.sin_port = htons(nSocketPort);
				if ( ::bind(Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR ) {
					::closesocket(Fd);
					continue;
				}
			} else {	// AF_INET6
			    memset(&in6, 0, sizeof(in6));
			    in6.sin6_family = AF_INET6;
			    in6.sin6_addr = in6addr_any;
			    in6.sin6_port = htons(nSocketPort);
				if ( ::bind(Fd, (struct sockaddr *)&in6, sizeof(in6)) == SOCKET_ERROR ) {
					::closesocket(Fd);
					continue;
				}
			}
		}

	    if ( ::connect(Fd, ai->ai_addr, (int)ai->ai_addrlen) == SOCKET_ERROR ) {
			::closesocket(Fd);
			continue;
		}

		break;
	}

	FreeAddrInfo(aitop);
	if ( Fd != INVALID_SOCKET )
		::closesocket(Fd);

	return (ai == NULL ? FALSE : TRUE);
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::PunyCodeEncode(LPCWSTR str, CString &out)
{
	int n, len, count, basic;
	unsigned long q, k, t, d;
	unsigned long min, max, delta, bias;

	out.Empty();

	for ( count = len = 0 ; str[len] != L'\0' ; len++ ) {
		if ( str[len] < 0x80 ) {
			out += str[len];
			count++;
		}
	}

	if ( count == len )
		return;

	out.Insert(0, _T("xn--"));

	if ( count > 0 )
		out += _T('-');

	min = 0x80;
	delta = 0;
	bias = 72;
	basic = count;

	while ( count < len ) {

		max = 0xFFFFFFFF;

		for ( n = 0 ; n < len ; n++ ) {
			if ( str[n] >= min && str[n] < max )
				max = str[n];
		}

	    delta += (max - min) * (count + 1);
		min = max;

		for ( n = 0 ; n < len ; n++ ) {
			if ( str[n] < min )
				delta++;

			if ( str[n] != min )
				continue;

			for ( q = delta, k = 36 ; ; k += 36 ) {
			  if ( k <= bias )
				  t = 1;
			  else if ( k > (bias + 26) )
				  t = 26;
			  else
				  t = k - bias;

			  if ( q < t )
				  break;

			  d = t + (q - t) % (36 - t);
			  out += (TCHAR)(d + 22 + 75 * (d < 26));

			  q = (q - t) / (36 - t);
			}

			out += (TCHAR)(q + 22 + 75 * (q < 26));

			if ( count == basic )
				delta = delta / 700;
			else
				delta = delta >> 1;

			delta += (delta / (count + 1));

			for ( k = 0 ; delta > ((36 - 1) * 26) / 2 ; k += 36 )
				delta /= (36 - 1);

			bias = k + (36 - 1 + 1) * delta / (delta + 38);

			delta = 0;
			count++;
		}

		delta++;
		min++;
	}
}
void CExtSocket::PunyCodeAdress(LPCTSTR str, CString &out)
{
	int n;
	CString tmp;
	CStringArrayExt param;
	LPCTSTR p;

	if ( str == NULL ) {
		out.Empty();
		return;
	}

	for ( p = str ; *p < 0x80 ; p++ ) {
		if ( *p == _T('\0') ) {
			out = str;
			return;
		}
	}

	out.Empty();
	param.GetString(str, _T('.'));

	for ( n = 0 ; n < param.GetSize() ; n++ ) {
		CExtSocket::PunyCodeEncode(TstrToUni(param[n]), tmp);
		if ( !out.IsEmpty() )
			out += _T('.');
		out += tmp;
	}
}

LPCTSTR CExtSocket::GetFormatErrorMessage(LPCTSTR entry, LPCTSTR host, int port, LPCTSTR type, int err)
{
	LPVOID lpMessageBuffer = NULL;
	CString tmp;
	static CString msg;

	msg.Format(_T("Server Entry '%s'\n"), entry == NULL ? _T("Unknown") : entry);

	if ( host != NULL ) {
		if ( port > 0 )
			tmp.Format(_T("Connection '%s:%d'\n"), host, port);
		else
			tmp.Format(_T("Connection '%s'\n"), host);
		msg += tmp;
	}

	if ( type != NULL ) {
		msg += type;
		msg += _T(" ");
	}

	tmp.Format(_T("Have Error #%d\n"), err);
	msg += tmp;

	if ( err > 0 && FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMessageBuffer, 0, NULL) != 0 && lpMessageBuffer != NULL ) {
		msg += _T("\n");
		msg += (LPTSTR)lpMessageBuffer;
		LocalFree(lpMessageBuffer);
	}

	return msg;
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::SSLInit()
{
	const SSL_METHOD *method = NULL;
	X509_STORE *pStore;
	X509 *pX509;
	int alpn_len;
	CBuffer alpn;

	theApp.SSL_Init();

	switch(m_SSL_mode) {
	case 1:
		method = TLS_client_method();
		break;
	case 2:
		method = OSSL_QUIC_client_method();
		break;
	}

	if ( (m_SSL_pCtx = SSL_CTX_new((SSL_METHOD *)method)) == NULL )
		return FALSE;

	if ( (m_SSL_pSock = SSL_new(m_SSL_pCtx)) == NULL )
		return FALSE;

	SSL_CTX_set_mode(m_SSL_pCtx, SSL_MODE_AUTO_RETRY);

	if ( (pStore = X509_STORE_new()) != NULL ) {
		// Windows Certificate Store からROOT証明書をOpenSSLに登録
		HCERTSTORE hStore;
		PCCERT_CONTEXT pContext = NULL;

		if ( (hStore = CertOpenSystemStore(0, _T("ROOT"))) != NULL ) {
			while ( (pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL ) {
				if ( (pX509 = d2i_X509(NULL, (const unsigned char **)(&pContext->pbCertEncoded), pContext->cbCertEncoded)) != NULL ) {
					X509_STORE_add_cert(pStore, pX509);
					X509_free(pX509);
				}
			}
			CertFreeCertificateContext(pContext);
			CertCloseStore(hStore, 0);

			SSL_CTX_set_cert_store(m_SSL_pCtx, pStore);
		} else
			X509_STORE_free(pStore);
	}

	m_SSL_alpn.Empty();

	switch(m_ProxyStatus) {
	case PRST_HTTP_START:				// HTTP
	case PRST_HTTP_BASIC_START:			// HTTP(Basic)
		m_SSL_alpn = "http/1.1";
		break;
	case PRST_HTTP2_START:				// HTTP/2
		m_SSL_alpn = "h2";
		break;
	case PRST_HTTP3_START:				// HTTP/3
		m_SSL_alpn = "h3";
		break;
	}

	if ( m_SSL_mode == 2 ) {
		if ( !SSL_set_default_stream_mode(m_SSL_pSock, SSL_DEFAULT_STREAM_MODE_NONE) )
			return FALSE;
	}

	if ( (alpn_len = m_SSL_alpn.GetLength()) > 0 ) {
		alpn.PutByte(alpn_len);
		alpn.Apend((BYTE *)(LPCSTR)m_SSL_alpn, alpn_len);

		if ( SSL_set_alpn_protos(m_SSL_pSock, alpn.GetPtr(), alpn.GetSize()) != 0 )
			return FALSE;
	}

	return TRUE;
}
int CExtSocket::SSLCheck()
{
	int n;
	long result = 0;
	X509 *pX509, *cert = NULL;
	STACK_OF(X509) *pStack;
	X509_NAME *pName;
	char name_buf[1024];
	CString tmp, subject, finger, dig;
	EVP_PKEY *pkey;
	CIdKey key;
	BOOL rf = FALSE;
	const char *errstr;

	if ( (n = SSL_connect(m_SSL_pSock)) < 0 )
		return SSL_get_error(m_SSL_pSock, n);

	cert   = SSL_get_peer_certificate(m_SSL_pSock);
	result = SSL_get_verify_result(m_SSL_pSock);

	m_SSL_Msg.Empty();

	if ( (pStack = SSL_get_peer_cert_chain(m_SSL_pSock)) != NULL ) {
		// 証明書のチェインをステータスに残す
		int n, num = sk_X509_num(pStack);

		for ( n = 0 ; n < num ; n++ ) {
			if ( (pX509 = sk_X509_value(pStack, n)) != NULL && (pName = X509_get_subject_name(pX509)) != NULL && X509_NAME_oneline(pName, name_buf, 1024) != NULL ) {
				if ( !m_SSL_Msg.IsEmpty() )
					m_SSL_Msg += _T("\r\n");

				tmp.Format(_T("Subject: %s\r\n"), MbsToTstr(name_buf));
				m_SSL_Msg += tmp;

				if ( (pName = X509_get_issuer_name(pX509)) != NULL && X509_NAME_oneline(pName, name_buf, 1024) != NULL ) {
					tmp.Format(_T("Issuer: %s\r\n"), MbsToTstr(name_buf));
					m_SSL_Msg += tmp;
				}

				if ( (pkey = X509_get_pubkey(pX509)) != NULL ) {
					if ( key.SetEvpPkey(pkey) ) {
						key.FingerPrint(finger, SSHFP_DIGEST_SHA256, SSHFP_FORMAT_SIMPLE);
						tmp.Format(_T("PublicKey: %s %dbits %s\r\n"), key.GetName(), key.GetSize(), (LPCTSTR)finger);
						m_SSL_Msg += tmp;
					} else
						m_SSL_Msg += _T("PublicKey: unkown key type\r\n");
					EVP_PKEY_free(pkey);
				}
			}
		}
	}

	if ( cert == NULL || result != X509_V_OK ) {
		if ( cert != NULL && (pName = X509_get_subject_name(cert)) != NULL && X509_NAME_oneline(pName, name_buf, 1024) != NULL )
			subject = MbsToTstr(name_buf);
		else
			subject = _T("unkown");

		if ( cert != NULL && (pkey = X509_get_pubkey(cert)) != NULL ) {
			if ( key.SetEvpPkey(pkey) ) {
				key.WritePublicKey(dig);
				tmp = AfxGetApp()->GetProfileString(_T("Certificate"), subject, _T(""));
				if ( !tmp.IsEmpty() && tmp.Compare(dig) == 0 )
					rf = TRUE;
			}
			EVP_PKEY_free(pkey);
		}

		if ( rf == FALSE ) {
			errstr = X509_verify_cert_error_string(result);
			tmp.Format(CStringLoad(IDS_CERTTRASTREQ), (m_SSL_Msg.IsEmpty() ? _T("Subject: unkown") : m_SSL_Msg), MbsToTstr(errstr));

			if ( ::AfxMessageBox(tmp, MB_ICONWARNING | MB_YESNO) != IDYES )
				goto ERRENDOF;

			if ( cert != NULL && !dig.IsEmpty() )
				AfxGetApp()->WriteProfileString(_T("Certificate"), subject, dig);
		}
	}

	if ( cert != NULL )
		X509_free(cert);

	m_SSL_Msg += _T("Version: ");
	m_SSL_Msg += MbsToTstr(SSL_get_version(m_SSL_pSock));
	m_SSL_Msg += _T("\r\n");

	if ( !m_SSL_alpn.IsEmpty() ) {
		const unsigned char* ret_alpn = NULL;
		unsigned int ret_alpn_len = 0; 
		SSL_get0_alpn_selected(m_SSL_pSock, &ret_alpn, &ret_alpn_len);

		m_SSL_alpn.Empty();
		while ( ret_alpn_len-- > 0 )
			m_SSL_alpn += *(ret_alpn++);

		if ( !m_SSL_alpn.IsEmpty() ) {
			tmp.Format(_T("TLS-ALPN: %s\r\n"), MbsToTstr(m_SSL_alpn));
			m_SSL_Msg += tmp;
		}
	}

	if ( m_SSL_mode == 2 && m_pFifoSSL != NULL ) {
		if ( (m_pFifoSSL->m_ssl = SSL_new_stream(m_SSL_pSock, SSL_STREAM_FLAG_NO_BLOCK | SSL_STREAM_FLAG_ADVANCE)) == NULL )
			goto ERRENDOF;

		if ( !SSL_set_blocking_mode(m_SSL_pSock, 0) )
			goto ERRENDOF;
	}

	return SSL_ERROR_NONE;

ERRENDOF:
	if ( cert != NULL )
		X509_free(cert);

	SSLClose();
	return SSL_ERROR_SSL;
}
void CExtSocket::SSLClose(BOOL bUnLink)
{
	if ( m_SSL_pSock != NULL ) {
		SSL *stram = NULL;

		if ( m_pFifoSSL != NULL ) {
			stram = m_pFifoSSL->m_ssl;
			m_pFifoSSL->Close();
		}

		if ( stram != NULL && stram != m_SSL_pSock ) {
			SSL_shutdown(stram);
			SSL_free(stram);
		}

		SSL_shutdown(m_SSL_pSock);
		SSL_free(m_SSL_pSock);
		m_SSL_pSock = NULL;
	}

	if ( m_SSL_pCtx != NULL ) {
		SSL_CTX_free(m_SSL_pCtx);
		m_SSL_pCtx  = NULL;
	}

	if ( bUnLink && m_pFifoSSL != NULL ) {
		CFifoBase::UnLink(m_pFifoSSL, FIFO_STDIN, TRUE);
		m_pFifoSSL->Destroy();
		m_pFifoSSL = NULL;
	}
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::ProxyCheck()
{
	ASSERT(m_pFifoProxy != NULL);

	if ( m_ProxyStatus != PRST_NONE && ProxyFunc() )
		return TRUE;

	if ( m_ProxyLastError == 0 )
		m_pFifoProxy->SendFdEvents(FIFO_EXTOUT, FD_CONNECT, NULL);
	else {
		m_pFifoProxy->SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)(UINT_PTR)m_ProxyLastError);
		m_pFifoProxy->SendFdEvents(FIFO_EXTOUT, FD_CLOSE, (void *)(UINT_PTR)m_ProxyLastError);
		m_pFifoProxy->Write(FIFO_EXTOUT, NULL, 0);
	}

	CFifoBase::UnLink(m_pFifoProxy, FIFO_STDIN, TRUE);
	m_pFifoProxy->Destroy();
	m_pFifoProxy = NULL;

	return FALSE;
}
BOOL CExtSocket::ProxyReadLine()
{
	int len, n;
	BYTE buf[256];

	ASSERT(m_pFifoProxy != NULL);

	for ( ; ; ) {
		if ( (len = m_pFifoProxy->Peek(FIFO_STDIN, buf, 256)) < 0 ) {
			m_ProxyLastError = (m_pFifoProxy->m_nLastError != 0 ? m_pFifoProxy->m_nLastError : WSAECONNRESET);
			m_ProxyStatus = PRST_NONE;
			return FALSE;
		} else if ( len == 0 )
			break;
		for ( n = 0 ; n < len ; n++ ) {
			if ( buf[n] == '\n' ) {
				m_pFifoProxy->Consume(FIFO_STDIN, n + 1);
				return TRUE;
			} else if ( buf[n] != '\r' )
				m_ProxyStr += (char)buf[n];
		}
		m_pFifoProxy->Consume(FIFO_STDIN, len);
	}
	return FALSE;
}
BOOL CExtSocket::ProxyReadBuff(int len)
{
	int n;
	BYTE buf[256];

	ASSERT(m_pFifoProxy != NULL);

	while ( m_ProxyBuff.GetSize() < len ) {
		if ( (n = len - m_ProxyBuff.GetSize()) > 256 )
			n = 256;
		if ( (n = m_pFifoProxy->Read(FIFO_STDIN, buf, n)) < 0 ) {
			m_ProxyLastError = (m_pFifoProxy->m_nLastError != 0 ? m_pFifoProxy->m_nLastError : WSAECONNRESET);
			m_ProxyStatus = PRST_NONE;
			return FALSE;
		} else if ( n == 0 )
			break;
		m_ProxyBuff.Apend(buf, n);
	}

	return (m_ProxyBuff.GetSize() >= len ? TRUE : FALSE);
}
BOOL CExtSocket::ProxyMakeDigest(CString &digest)
{
	CString tmp;
	CStringA mbs;
	LPCTSTR algo;
	CBuffer A1(-1), A2(-1), A3(-1);
	WORD cnonce[4];

	if (m_ProxyAuth.Find(_T("qop")) < 0 || m_ProxyAuth.Find(_T("realm")) < 0 || m_ProxyAuth.Find(_T("nonce")) < 0 ) {
		m_pDocument->m_ErrorPrompt = _T("Authorization Paramter Error");
		m_ProxyLastError = WSAECONNREFUSED;
		m_ProxyStatus = PRST_NONE;
		return FALSE;
	}

	tmp.Format(_T("%s:%d"), (LPCTSTR)m_ProxyHost, m_ProxyPort);
	m_ProxyAuth[_T("uri")] = tmp;

	m_ProxyAuth[_T("qop")] = _T("auth");

	tmp.Format(_T("%08d"), 1);
	m_ProxyAuth[_T("nc")] = tmp;

	rand_buf(cnonce, sizeof(cnonce));
	tmp.Format(_T("%04x%04x%04x%04x"), cnonce[0], cnonce[1], cnonce[2], cnonce[3]);
	m_ProxyAuth[_T("cnonce")] = tmp;

	algo = m_ProxyAuth[_T("algorithm")];
	tmp.Format(_T("%s:%s:%s"), 
		(LPCTSTR)m_ProxyUser, (LPCTSTR)m_ProxyAuth[_T("realm")], (LPCTSTR)m_ProxyPass);
	A1.Hash(algo, m_pFifoProxy->DocMsgRemoteStr(tmp, mbs));

	tmp.Format(_T("%s:%s"), 
		_T("CONNECT"), (LPCTSTR)m_ProxyAuth[_T("uri")]);
	A2.Hash(algo, m_pFifoProxy->DocMsgRemoteStr(tmp, mbs));

	tmp.Format(_T("%s:%s:%s:%s:%s:%s"),
		(LPCTSTR)A1, (LPCTSTR)m_ProxyAuth[_T("nonce")], (LPCTSTR)m_ProxyAuth[_T("nc")],
		(LPCTSTR)m_ProxyAuth[_T("cnonce")], (LPCTSTR)m_ProxyAuth[_T("qop")], (LPCTSTR)A2);
	A3.Hash(algo, m_pFifoProxy->DocMsgRemoteStr(tmp, mbs));
	m_ProxyAuth[_T("response")] = (LPCTSTR)A3;

	digest.Format(
		_T("digest username=\"%s\", realm=\"%s\", nonce=\"%s\",")\
		_T(" algorithm=%s, response=\"%s\",")\
		_T(" qop=%s, uri=\"%s\",")\
		_T(" nc=%s, cnonce=\"%s\""),
		(LPCTSTR)m_ProxyUser, (LPCTSTR)m_ProxyAuth[_T("realm")], (LPCTSTR)m_ProxyAuth[_T("nonce")],
		(LPCTSTR)m_ProxyAuth[_T("algorithm")], (LPCTSTR)m_ProxyAuth[_T("response")],
		(LPCTSTR)m_ProxyAuth[_T("qop")], (LPCTSTR)m_ProxyAuth[_T("uri")],
		(LPCTSTR)m_ProxyAuth[_T("nc")], (LPCTSTR)m_ProxyAuth[_T("cnonce")]);

	return TRUE;
}
BOOL CExtSocket::ProxyReOpen()
{
	BOOL rt = FALSE;

	ASSERT(m_pFifoLeft != NULL);

	SSLClose();

	switch(m_pFifoLeft->m_Type) {
	case FIFO_TYPE_SOCKET:
		((CFifoSocket *)m_pFifoLeft)->Close();
		((CFifoSocket *)m_pFifoLeft)->Reset(FIFO_STDIN);
		((CFifoSocket *)m_pFifoLeft)->Reset(FIFO_STDOUT);
		rt = ((CFifoSocket *)m_pFifoLeft)->Open();
		break;
	case FIFO_TYPE_PIPE:
		((CFifoPipe *)m_pFifoLeft)->Close();
		((CFifoPipe *)m_pFifoLeft)->Reset(FIFO_STDIN);
		((CFifoPipe *)m_pFifoLeft)->Reset(FIFO_STDOUT);
		rt = ((CFifoPipe *)m_pFifoLeft)->Open();
		break;
	case FIFO_TYPE_COM:
		((CFifoCom *)m_pFifoLeft)->Close();
		((CFifoCom *)m_pFifoLeft)->Reset(FIFO_STDIN);
		((CFifoCom *)m_pFifoLeft)->Reset(FIFO_STDOUT);
		rt = ((CFifoCom *)m_pFifoLeft)->Open();
		break;
	}

	return rt;
}
BOOL CExtSocket::ProxyFunc()
{
	int n, i;
	CBuffer buf;
	CString tmp, dig;
	CBuffer A1, A2;
	CStringA mbs;
	u_char digest[EVP_MAX_MD_SIZE];

	ASSERT(m_pFifoProxy != NULL);

	while ( m_ProxyStatus != PRST_NONE ) {
		switch(m_ProxyStatus) {
		case PRST_HTTP_START:		// HTTP
			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("Connection: keep-alive\r\n")\
					   _T("\r\n"),
					   (LPCTSTR)m_ProxyHost, m_ProxyPort, (LPCTSTR)m_ProxyHost);
			m_pFifoProxy->DocMsgRemoteStr(tmp, mbs);
			m_pFifoProxy->Write(FIFO_STDOUT, (BYTE *)(LPCSTR)mbs, mbs.GetLength());
			DEBUGLOG("ProxyFunc PRST_HTTP_START %s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			break;
		case PRST_HTTP_READLINE:
			if ( !ProxyReadLine() )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			DEBUGLOG("ProxyFunc PRST_HTTP_READLINE %s", TstrToMbs(m_ProxyStr));
			if ( m_ProxyStr.IsEmpty() ) {
				m_ProxyStatus = PRST_HTTP_HEADCHECK;
				break;
			} else if ( (m_ProxyStr[0] == _T('\t') || m_ProxyStr[0] == _T(' ')) && (n = (int)m_ProxyResult.GetSize()) > 0 )
				m_ProxyResult[n - 1] += m_ProxyStr;
			else
				m_ProxyResult.Add(m_ProxyStr);
			m_ProxyStr.Empty();
			break;
		case PRST_HTTP_HEADCHECK:
			m_ProxyCode = 0;
			m_ProxyLength = 0;
			m_ProxyConnect = FALSE;
			m_ProxyAuth.RemoveAll();
			for ( n = 0 ; n < m_ProxyResult.GetSize() ; n++ ) {
				if ( _tcsnicmp(m_ProxyResult[n], _T("HTTP"), 4) == 0 ) {
					if ( (i = m_ProxyResult[n].Find(_T(' '))) >= 0 )
						m_ProxyCode = _tstoi(m_ProxyResult[n].Mid(i + 1));
				} else if ( _tcsnicmp(m_ProxyResult[n], _T("Content-Length:"), 15) == 0 ) {
					if ( (i = m_ProxyResult[n].Find(_T(' '))) >= 0 )
						m_ProxyLength = _tstoi(m_ProxyResult[n].Mid(i + 1));
				} else if ( _tcsnicmp(m_ProxyResult[n], _T("WWW-Authenticate:"), 17) == 0 ) {
					m_ProxyAuth.SetArray(m_ProxyResult[n]);
				} else if ( _tcsnicmp(m_ProxyResult[n], _T("Proxy-Authenticate:"), 19) == 0 ) {
					m_ProxyAuth.SetArray(m_ProxyResult[n]);
				} else if ( _tcsnicmp(m_ProxyResult[n], _T("Connection:"), 11) == 0 ) {
					tmp = m_ProxyResult[n].Mid(12);
					tmp.MakeLower();
					if ( tmp.Find(_T("keep-alive")) >= 0 )
						m_ProxyConnect = TRUE;
				}
			}
			m_ProxyStatus = (m_ProxyLength > 0 ? PRST_HTTP_BODYREAD : PRST_HTTP_CODECHECK);
			m_ProxyBuff.Clear();
			break;
		case PRST_HTTP_BODYREAD:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			m_ProxyStatus = PRST_HTTP_CODECHECK;
			break;
		case PRST_HTTP_CODECHECK:
			switch(m_ProxyCode) {
			case 200:
				m_RealHostAddr = m_ProxyHostAddr;
				m_RealHostPort = m_ProxyPort;
				m_ProxyLastError = 0;
				m_ProxyStatus = PRST_NONE;
				break;
			case 401:	// Authorization Required
			case 407:	// Proxy-Authorization Required
				if ( m_ProxyAuth.Find(_T("basic")) >= 0 )
					m_ProxyStatus = PRST_HTTP_BASIC;
				else if ( m_ProxyAuth.Find(_T("digest")) >= 0 )
					m_ProxyStatus = PRST_HTTP_DIGEST;
				else {
					m_ProxyStatus = PRST_HTTP_ERROR;
					break;
				}
				if ( m_ProxyConnect )
					break;
				else if ( ProxyReOpen() )
					return TRUE;
				// no break;
			default:
				m_ProxyStatus = PRST_HTTP_ERROR;
				break;
			}
			break;
		case PRST_HTTP_ERROR:
			SSLClose();
			m_pDocument->m_ErrorPrompt = (m_ProxyResult.GetSize() > 0 ? m_ProxyResult[0]: _T("HTTP Proxy Error"));
			m_ProxyLastError = WSAECONNREFUSED;
			m_ProxyStatus = PRST_NONE;
			break;
		case PRST_HTTP_BASIC_START:
			m_ProxyStatus = PRST_HTTP_BASIC;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			m_ProxyCode = 407;
			m_ProxyLength = 0;
			m_ProxyConnect = FALSE;
			m_ProxyAuth.RemoveAll();
			break;
		case PRST_HTTP_BASIC:
			tmp.Format(_T("%s:%s"), (LPCTSTR)m_ProxyUser, (LPCTSTR)m_ProxyPass);
			m_pFifoProxy->DocMsgRemoteStr(tmp, mbs); 
			buf.Base64Encode((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("%sAuthorization: Basic %s\r\n")\
					   _T("\r\n"),
					   (LPCTSTR)m_ProxyHost, m_ProxyPort, (LPCTSTR)m_ProxyHost,
					   (m_ProxyCode == 407 ? _T("Proxy-") : _T("")),
					   (LPCTSTR)buf);
			m_pFifoProxy->DocMsgRemoteStr(tmp, mbs); 
			m_pFifoProxy->Write(FIFO_STDOUT, (BYTE *)(LPCSTR)mbs, mbs.GetLength());
			DEBUGLOG("ProxyFunc PRST_HTTP_BASIC %s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			if ( !m_SSL_keep )
				SSLClose(TRUE);					// XXXXXXXX BUG???
			break;
		case PRST_HTTP_DIGEST:
			if ( !ProxyMakeDigest(dig) )
				break;
			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("%sAuthorization: %s\r\n")\
					   _T("\r\n"),
					   (LPCTSTR)m_ProxyHost, m_ProxyPort, (LPCTSTR)m_ProxyHost,
					   (m_ProxyCode == 407 ? _T("Proxy-") : _T("")), (LPCTSTR)dig);
			m_pFifoProxy->DocMsgRemoteStr(tmp, mbs); 
			m_pFifoProxy->Write(FIFO_STDOUT, (BYTE *)(LPCSTR)mbs, mbs.GetLength());
			DEBUGLOG("ProxyFunc PRST_HTTP_DIGEST %s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			if ( !m_SSL_keep )
				SSLClose(TRUE);					// XXXXXXXX BUG???
			break;

		//////////////////////////////////////////////////////////////////////

		case PRST_SOCKS4_START:	// SOCKS4
			ULONG dw;
			struct hostent *hp;
			if ( (hp = ::gethostbyname(TstrToMbs(m_ProxyHost))) == NULL ) {
				m_ProxyStatus = PRST_SOCKS4_ENDOF;
				break;
			}
			dw = ((struct in_addr *)(hp->h_addr))->s_addr;
			buf.Clear();
			buf.Put8Bit(4);
			buf.Put8Bit(1);
			buf.Put8Bit(m_ProxyPort >> 8);
			buf.Put8Bit(m_ProxyPort);
			buf.Put8Bit(dw);
			buf.Put8Bit(dw >> 8);
			buf.Put8Bit(dw >> 16);
			buf.Put8Bit(dw >> 24);
			m_pFifoProxy->DocMsgRemoteStr(m_ProxyUser, mbs); 
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength() + 1);
			m_pFifoProxy->Write(FIFO_STDOUT, buf.GetPtr(), buf.GetSize());
			m_ProxyStatus = PRST_SOCKS4_READMSG;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS4_READMSG:
			if ( !ProxyReadBuff(8) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			m_ProxyStatus = PRST_SOCKS4_CHECKMSG;
			break;
		case PRST_SOCKS4_CHECKMSG:
			if ( *(m_ProxyBuff.GetPos(1)) == 90 ) {
				m_RealHostAddr = m_ProxyHostAddr;
				m_RealHostPort = m_ProxyPort;
				m_ProxyLastError = 0;
				m_ProxyStatus = PRST_NONE;
			} else
				m_ProxyStatus = PRST_SOCKS4_ENDOF;
			break;
		case PRST_SOCKS4_ENDOF:
			m_pDocument->m_ErrorPrompt = _T("SOCKS4 Proxy Conection Error");
			m_ProxyLastError = WSAECONNREFUSED;
			m_ProxyStatus = PRST_NONE;
			break;

		//////////////////////////////////////////////////////////////////////

		case PRST_SOCKS5_START:	// SOCKS5
			buf.Clear();
			buf.Put8Bit(5);
			buf.Put8Bit(3);
			buf.Put8Bit(0x00);		// SOCKS5_AUTH_NOAUTH
			buf.Put8Bit(0x02);		// SOCKS5_AUTH_USERPASS
			buf.Put8Bit(0x03);		// SOCKS5_AUTH_CHAP
			m_pFifoProxy->Write(FIFO_STDOUT, buf.GetPtr(), buf.GetSize());
			m_ProxyStatus = PRST_SOCKS5_READMSG;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_READMSG:
			if ( !ProxyReadBuff(2) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			if ( *(m_ProxyBuff.GetPos(0)) != 5 || (m_ProxyCode = *(m_ProxyBuff.GetPos(1))) == 0xFF )
				m_ProxyStatus = PRST_SOCKS5_ERROR;
			else if ( m_ProxyCode == 0x03 )	// SOCKS5_AUTH_CHAP
				m_ProxyStatus = PRST_SOCKS5_CHAPSTART;
			else if ( m_ProxyCode == 0x02 )	// SOCKS5_AUTH_USERPASS
				m_ProxyStatus = PRST_SOCKS5_SENDAUTH;
			else if ( m_ProxyCode == 0x00 )	// SOCKS5_AUTH_NOAUTH
				m_ProxyStatus = PRST_SOCKS5_SENDCONNECT;
			else
				m_ProxyStatus = PRST_SOCKS5_ERROR;
			break;
		case PRST_SOCKS5_SENDAUTH:	// SOCKS5_AUTH_USERPASS
			buf.Clear();
			buf.Put8Bit(1);
			m_pFifoProxy->DocMsgRemoteStr(m_ProxyUser, mbs);
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			m_pFifoProxy->DocMsgRemoteStr(m_ProxyPass, mbs);
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());

			m_pFifoProxy->Write(FIFO_STDOUT, buf.GetPtr(), buf.GetSize());
			m_ProxyStatus = PRST_SOCKS5_READAUTH;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_READAUTH:
			if ( !ProxyReadBuff(2) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			if ( *(m_ProxyBuff.GetPos(1)) != 0 )
				m_ProxyStatus = PRST_SOCKS5_ERROR;
			else
				m_ProxyStatus = PRST_SOCKS5_SENDCONNECT;
			break;
		case PRST_SOCKS5_SENDCONNECT:	// SOCKS5_CONNECT
			buf.Clear();
			buf.Put8Bit(5);		// SOCKS version 5
			buf.Put8Bit(1);		// CONNECT
			buf.Put8Bit(0);		// FLAG
			buf.Put8Bit(3);		// DOMAIN
			m_pFifoProxy->DocMsgRemoteStr(m_ProxyHost, mbs);
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			buf.Put8Bit(m_ProxyPort >> 8);
			buf.Put8Bit(m_ProxyPort);
			m_pFifoProxy->Write(FIFO_STDOUT, buf.GetPtr(), buf.GetSize());
			m_ProxyStatus = PRST_SOCKS5_READCONNECT;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_READCONNECT:
			if ( !ProxyReadBuff(4) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			if ( *(m_ProxyBuff.GetPos(0)) != 5 || *(m_ProxyBuff.GetPos(1)) != 0 )
				m_ProxyStatus = PRST_SOCKS5_ERROR;
			switch(*(m_ProxyBuff.GetPos(3))) {
			case 1:
				m_ProxyLength = 4 + 4 + 2;
				m_ProxyStatus = PRST_SOCKS5_CONNECT;
				break;
			case 3:
				m_ProxyLength = 4 + 1;
				m_ProxyStatus = PRST_SOCKS5_READSTAT;
				break;
			case 4:
				m_ProxyLength = 4 + 16 + 2;
				m_ProxyStatus = PRST_SOCKS5_CONNECT;
				break;
			default:
				m_ProxyStatus = PRST_SOCKS5_ERROR;
				break;
			}
			break;
		case PRST_SOCKS5_READSTAT:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			m_ProxyLength = 4 + 1 + *(m_ProxyBuff.GetPos(4)) + 2;
			m_ProxyStatus = PRST_SOCKS5_CONNECT;
			break;
		case PRST_SOCKS5_CONNECT:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			m_RealHostAddr = m_ProxyHostAddr;
			m_RealHostPort = m_ProxyPort;
			m_ProxyLastError = 0;
			m_ProxyStatus = PRST_NONE;
			break;
		case PRST_SOCKS5_ERROR:	// SOCKS5_ERROR
			m_pDocument->m_ErrorPrompt = _T("SOCKS5 Proxy Conection Error");
			m_ProxyLastError = WSAECONNREFUSED;
			m_ProxyStatus = PRST_NONE;
			return TRUE;

		case PRST_SOCKS5_CHAPSTART:	// SOCKS5 CHAP
			buf.Clear();
			buf.Put8Bit(0x01);
			buf.Put8Bit(0x02);
			buf.Put8Bit(0x11);
			buf.Put8Bit(0x85);		// HMAC-MD5
			buf.Put8Bit(0x02);
			m_pFifoProxy->DocMsgRemoteStr(m_ProxyUser, mbs);
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			m_pFifoProxy->Write(FIFO_STDOUT, buf.GetPtr(), buf.GetSize());
			m_ProxyConnect = FALSE;
			m_ProxyStatus = PRST_SOCKS5_CHAPMSG;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_CHAPMSG:
			if ( !ProxyReadBuff(2) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			if ( *(m_ProxyBuff.GetPos(0)) != 0x01 || (m_ProxyNum = *(m_ProxyBuff.GetPos(1))) == 0x00 )
				m_ProxyStatus = PRST_SOCKS5_ERROR;
			else 
				m_ProxyStatus = PRST_SOCKS5_CHAPCODE;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_CHAPCODE:
			if ( !ProxyReadBuff(2) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			m_ProxyCode   = *(m_ProxyBuff.GetPos(0));
			m_ProxyLength = *(m_ProxyBuff.GetPos(1));
			m_ProxyBuff.Clear();
			m_ProxyStatus = PRST_SOCKS5_CHAPSTAT;
			break;
		case PRST_SOCKS5_CHAPSTAT:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			m_ProxyStatus = PRST_SOCKS5_CHAPCHECK;
			switch(m_ProxyCode) {
			case 0x00:
				if ( *(m_ProxyBuff.GetPos(0)) == 0x00 )
					m_ProxyConnect = TRUE;
				else
					m_ProxyStatus = PRST_SOCKS5_ERROR;
				break;
			case 0x03:
				m_pFifoProxy->DocMsgRemoteStr(m_ProxyPass, mbs);
				n = HMAC_digest(EVP_md5(), (BYTE *)(LPCSTR)mbs, mbs.GetLength(), m_ProxyBuff.GetPtr(), m_ProxyBuff.GetSize(), digest, sizeof(digest));

				buf.Clear();
				buf.Put8Bit(0x01);
				buf.Put8Bit(0x01);
				buf.Put8Bit(0x04);
				buf.Put8Bit(16);
				buf.Apend(digest, n);
				m_pFifoProxy->Write(FIFO_STDOUT, buf.GetPtr(), buf.GetSize());
				break;
			case 0x11:
				if ( *(m_ProxyBuff.GetPos(0)) != 0x85 )
					m_ProxyStatus = PRST_SOCKS5_ERROR;
				break;
			}
			break;
		case PRST_SOCKS5_CHAPCHECK:
			if ( --m_ProxyNum > 0 )
				m_ProxyStatus = PRST_SOCKS5_CHAPCODE;
			else if ( m_ProxyConnect )
				m_ProxyStatus = PRST_SOCKS5_SENDCONNECT;
			else
				m_ProxyStatus = PRST_SOCKS5_CHAPMSG;
			m_ProxyBuff.Clear();
			break;

		//////////////////////////////////////////////////////////////////////

		case PRST_HTTP2_START:
			if ( m_SSL_alpn.CompareNoCase("h2") != 0 ) {	// SSL/QUIC onry
				m_ProxyStatus = PRST_HTTP_START;
				break;
			}

			if ( m_pHttp2Ctx != NULL )
				delete m_pHttp2Ctx;
			m_pHttp2Ctx = new CHttp2Ctx(m_pFifoProxy);

			A1.Clear();
			A1.Put16Bit(HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS);
			A1.Put32Bit(100);
			A1.Put16Bit(HTTP2_SETTINGS_INITIAL_WINDOW_SIZE);
			A1.Put32Bit(HTTP2_CLIENT_WINDOW_SIZE);
			m_pFifoProxy->Write(FIFO_STDOUT, (LPBYTE)"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24);
			m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_SETTINGS, 0, 0, A1.GetPtr(), A1.GetSize());

			m_ProxyBuff.Clear();
			m_ProxyStatus = PRST_HTTP2_REQUEST;
			break;
		case PRST_HTTP2_TUNNEL:
			{
				BYTE buf[4096];
				while ( (n = m_pFifoProxy->Peek(FIFO_EXTIN, buf, 4096)) > 0 ) {
					if ( m_pHttp2Ctx->IsEndOfStream() ) {
						m_ProxyLastError = WSAECONNABORTED;
						m_ProxyStatus = PRST_NONE;
						break;
					}
					if ( n > m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_STREAM_SERVER] )
						n = m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_STREAM_SERVER];
					if ( n > 0 ) {
						m_pFifoProxy->Consume(FIFO_EXTIN, n);
						m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_DATA, 0, HTTP2_NOW_STREAMID, buf, n);
						m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_STREAM_SERVER] -= n;
					} else
						break;
				}
			}
			// no break;
		case PRST_HTTP2_REQUEST:
		case PRST_HTTP2_CONNECT:
		case PRST_HTTP2_BASIC:
		case PRST_HTTP2_DIGEST:
			{
				int length, type, flag;
				DWORD sid;
				if ( (n = m_pFifoProxy->GetDataSize(FIFO_STDIN)) > 0 )
					ProxyReadBuff(m_ProxyBuff.GetSize() + n);
				if ( !m_pHttp2Ctx->GetHPackFrame(&m_ProxyBuff, length, type, flag, sid) )
					return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
				if ( type < 0 || type > HTTP2_TYPE_CONTINUATION || length < 0 || length > (128 * 1024) ) {	// SETTINGS_MAX_FRAME_SIZE ?
					m_pDocument->m_ErrorPrompt = _T("http/2 frame type or length error");
					m_ProxyLastError = WSAECONNREFUSED;
					m_ProxyStatus = PRST_NONE;
				}
				if ( m_ProxyBuff.GetSize() < (9 + length) )
					return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);

				m_ProxyBuff.Consume(9);
				buf.Clear();
				buf.Apend(m_ProxyBuff.GetPtr(), length);
				m_ProxyBuff.Consume(length);

				if ( (m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_TOTAL_CLIENT] -= (9 + length)) <= 0 ) {
					A1.Clear();
					A1.Put32Bit(HTTP2_CLIENT_WINDOW_SIZE);
					m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_WINDOW_UPDATE, 0, 0, A1.GetPtr(), A1.GetSize());
					m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_TOTAL_CLIENT] += HTTP2_CLIENT_WINDOW_SIZE;
				}

				TRACE("RecvHPackFrame %d, %02x, #%d, %d\n", type, flag, sid, length);

				switch(type) {
				case HTTP2_TYPE_DATA:
					if ( (flag & HTTP2_FLAG_PADDED) != 0 ) {
						n = buf.Get8Bit();
						buf.ConsumeEnd(n);
					}
					if ( (flag & HTTP2_FLAG_END_STREAM) != 0 ) {
						m_pHttp2Ctx->SetEndOfStream(sid);
						break;
					}

					if ( m_ProxyStatus == PRST_HTTP2_TUNNEL ) {
						if ( buf.GetSize() > 0 )
							m_pFifoProxy->Write(FIFO_EXTOUT, buf.GetPtr(), buf.GetSize());
						if ( (flag & HTTP2_FLAG_END_STREAM) != 0 )
							m_pHttp2Ctx->SetEndOfStream(sid);
					}

					if ( (m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_STREAM_CLIENT] -= length) <= HTTP2_CLIENT_FLOWOFF_SIZE ) {
						A1.Clear();
						A1.Put32Bit(HTTP2_DEFAULT_WINDOW_SIZE);
						m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_WINDOW_UPDATE, 0, HTTP2_NOW_STREAMID, A1.GetPtr(), A1.GetSize());
						m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_STREAM_CLIENT] += HTTP2_DEFAULT_WINDOW_SIZE;
					}
					break;
				case HTTP2_TYPE_HEADERS:
				case HTTP2_TYPE_CONTINUATION:
					if ( (flag & HTTP2_FLAG_PADDED) != 0 ) {
						n = buf.Get8Bit();
						buf.ConsumeEnd(n);
					}
					if ( (flag & HTTP2_FLAG_PRIORITY) != 0 ) {
						buf.Get32Bit();	// |E|Stream Dependency? (31)  
						buf.Get8Bit();	// Weight? (8)
					}
					if ( !m_pHttp2Ctx->ReciveHeader(&buf, sid, ((flag & HTTP2_FLAG_END_HEADERS) != 0 ? TRUE : FALSE)) )
						break;

					m_ProxyCode = 0;
					m_ProxyAuth.RemoveAll();

					while ( buf.GetSize() > 0 ) {
						CStringA name, value;
						if ( !m_pHttp2Ctx->GetHPackField(&buf, name, value) )
							break;
						if ( name.CompareNoCase(":status") == 0 )
							m_ProxyCode = atoi(value);
						else if ( name.CompareNoCase("authorization") == 0 )
							m_ProxyAuth.SetArray(LocalStr(value));
						else if ( name.CompareNoCase("WWW-Authenticate") == 0 )
							m_ProxyAuth.SetArray(LocalStr(value));
						else if ( name.CompareNoCase("Proxy-Authenticate") == 0 )
							m_ProxyAuth.SetArray(LocalStr(value));
					}

					switch(m_ProxyCode) {
					case 200:	// OK
						m_RealHostAddr = m_ProxyHostAddr;
						m_RealHostPort = m_ProxyPort;
						m_ProxyStatus = PRST_HTTP2_TUNNEL;
						m_pFifoProxy->SendFdEvents(FIFO_EXTOUT, FD_CONNECT, NULL);
						break;
					case 401:	// Authorization Required
					case 407:	// Proxy-Authorization Required
						if ( m_ProxyAuth.Find(_T("basic")) >= 0 ) {
							A1.Clear();
							m_pHttp2Ctx->PutHPackField(&A1, ":method", "CONNECT");
							mbs.Format("%s:%d", TstrToMbs(m_ProxyHost), m_ProxyPort);
							m_pHttp2Ctx->PutHPackField(&A1, ":authority", mbs);
							m_pHttp2Ctx->PutHPackField(&A1, "user-agent", "RLogin");
							tmp.Format(_T("%s:%s"), (LPCTSTR)m_ProxyUser, (LPCTSTR)m_ProxyPass);
							m_pFifoProxy->DocMsgRemoteStr(tmp, mbs); 
							buf.Base64Encode((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
							mbs.Format("basic %s", TstrToMbs((LPCTSTR)buf));
							m_pHttp2Ctx->PutHPackField(&A1, (m_ProxyCode == 407 ? "proxy-authorization" :"authorization"), mbs);

							m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_HEADERS, HTTP2_FLAG_END_HEADERS, HTTP2_NEW_STREAMID, A1.GetPtr(), A1.GetSize());
							m_ProxyStatus = PRST_HTTP2_BASIC;
							break;
						} else if ( m_ProxyAuth.Find(_T("digest")) >= 0 ) {
							if ( !ProxyMakeDigest(dig) )
								break;
							A1.Clear();
							m_pHttp2Ctx->PutHPackField(&A1, ":method", "CONNECT");
							mbs.Format("%s:%d", TstrToMbs(m_ProxyHost), m_ProxyPort);
							m_pHttp2Ctx->PutHPackField(&A1, ":authority", mbs);
							m_pHttp2Ctx->PutHPackField(&A1, "user-agent", "RLogin");
							m_pFifoProxy->DocMsgRemoteStr(dig, mbs);
							m_pHttp2Ctx->PutHPackField(&A1, (m_ProxyCode == 407 ? "proxy-authorization" :"authorization"), mbs);

							m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_HEADERS, HTTP2_FLAG_END_HEADERS, HTTP2_NEW_STREAMID, A1.GetPtr(), A1.GetSize());
							m_ProxyStatus = PRST_HTTP2_DIGEST;
							break;
						}
						// no break;
					default:
						m_pDocument->m_ErrorPrompt.Format(_T("http/2 status error %d"), m_ProxyCode);
						m_ProxyLastError = WSAECONNREFUSED;
						m_ProxyStatus = PRST_NONE;
						break;
					}
					break;
				case HTTP2_TYPE_PRIORITY:
					TRACE("PRIORITY\n");
					break;
				case HTTP2_TYPE_RST_STREAM:
					n = buf.Get32Bit();		// error code
					TRACE("RST_STREAM #%d %d\n", sid, n);
					m_pHttp2Ctx->ReciveHeader(NULL, sid, TRUE);
					m_pHttp2Ctx->SetEndOfStream(sid);
					break;
				case HTTP2_TYPE_SETTINGS:
					while ( buf.GetSize() >= (2 + 4) )	{	// Identifier (16) + Value (32)
						int ident, value;
						ident = buf.Get16Bit();
						value = buf.Get32Bit();
						switch(ident) {
						case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
						case HTTP2_SETTINGS_ENABLE_PUSH:
						case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
							break;
						case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
							m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_TOTAL_SERVER] = value;
							break;
						case HTTP2_SETTINGS_MAX_FRAME_SIZE:
						case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
							break;
						}
					}
					if ( (flag & HTTP2_FLAG_ACK) != 0 ) {
						if ( m_ProxyStatus == PRST_HTTP2_REQUEST ) {
							A1.Clear();
							m_pHttp2Ctx->PutHPackField(&A1, ":method", "CONNECT");
							mbs.Format("%s:%d", TstrToMbs(m_ProxyHost), m_ProxyPort);
							m_pHttp2Ctx->PutHPackField(&A1, ":authority", mbs);
							m_pHttp2Ctx->PutHPackField(&A1, "user-agent", "RLogin");

							m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_HEADERS, HTTP2_FLAG_END_HEADERS, HTTP2_NEW_STREAMID, A1.GetPtr(), A1.GetSize());
							m_ProxyStatus = PRST_HTTP2_CONNECT;
						}
					} else {
						m_pHttp2Ctx->SendHPackFrame(HTTP2_TYPE_SETTINGS, HTTP2_FLAG_ACK, 0, NULL, 0);
					}
					break;
				case HTTP2_TYPE_PUSH_PROMISE:
					// client not recive ?
					TRACE("PUSH_PROMISE\n");
					break;
				case HTTP2_TYPE_PING:
					TRACE("PING\n");
					break;
				case HTTP2_TYPE_GOAWAY:
					TRACE("GOAWAY\n");
					n = buf.Get32Bit();		// streame id
					n = buf.Get32Bit();		// error code
					m_pDocument->m_ErrorPrompt.Format(_T("http/2 goway #%d '%s'"), n, m_pFifoProxy->DocMsgLocalStr((LPCSTR)buf, tmp));
					m_ProxyLastError = WSAECONNABORTED;
					m_ProxyStatus = PRST_NONE;
					break;
				case HTTP2_TYPE_WINDOW_UPDATE:
					n = buf.Get32Bit() & 0x7FFFFFFF;
					if ( sid == 0 ) {
						m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_TOTAL_SERVER] += n;
						m_pHttp2Ctx->SendHPackFrameQueBuffer();
					} else if ( sid == m_pHttp2Ctx->m_ProxyStreamId )
						m_pHttp2Ctx->m_Http2WindowBytes[HTTP2_STREAM_SERVER] += n;
					break;
				default:
					m_pDocument->m_ErrorPrompt.Format(_T("http/2 type error %d"), type);
					m_ProxyLastError = WSAECONNREFUSED;
					m_ProxyStatus = PRST_NONE;
					break;
				}
			}
			break;

		//////////////////////////////////////////////////////////////////////

		case PRST_HTTP3_START:
			if ( m_SSL_alpn.CompareNoCase("h3") != 0 ) {	// SSL/QUIC onry
				m_ProxyStatus = PRST_HTTP_START;
				break;
			}
			if ( m_pHttp3Ctx != NULL )
				delete m_pHttp3Ctx;
			m_pHttp3Ctx = new CHttp3Ctx(m_pFifoProxy);

			A1.Clear();
			m_pHttp3Ctx->PutHeaderBase(&A1);
			m_pHttp3Ctx->PutQPackField(&A1, ":method", "CONNECT");
			mbs.Format("%s:%d", TstrToMbs(m_ProxyHost), m_ProxyPort);
			m_pHttp3Ctx->PutQPackField(&A1, ":authority", mbs);
			m_pHttp3Ctx->PutQPackField(&A1, "user-agent", "RLogin");
			m_pHttp3Ctx->SendQPackFrame(HTTP3_TYPE_HEADERS, A1.GetPtr(), A1.GetSize());

			m_ProxyBuff.Clear();
			m_ProxyStatus = PRST_HTTP3_CONNECT;
			break;
		case PRST_HTTP3_TUNNEL:
			{
				BYTE buf[4096];
				while ( (n = m_pFifoProxy->Peek(FIFO_EXTIN, buf, 4096)) > 0 ) {
					m_pHttp3Ctx->SendQPackFrame(HTTP3_TYPE_DATA, buf, n);
					m_pFifoProxy->Consume(FIFO_EXTIN, n);
				}
			}
			// no break;
		case PRST_HTTP3_CONNECT:
		case PRST_HTTP3_BASIC:
		case PRST_HTTP3_DIGEST:
			if ( (n = m_pFifoProxy->GetDataSize(FIFO_STDIN)) > 0 )
				ProxyReadBuff(m_ProxyBuff.GetSize() + n);
			if ( !m_pHttp3Ctx->GetQPackFrame(&m_ProxyBuff, n, &buf) )
				return (m_ProxyStatus != PRST_NONE ? TRUE : FALSE);
			switch(n) {
			case HTTP3_TYPE_DATA:
				if ( m_ProxyStatus == PRST_HTTP3_TUNNEL )
					m_pFifoProxy->Write(FIFO_EXTOUT, buf.GetPtr(), buf.GetSize());
				break;
			case HTTP3_TYPE_HEADERS:
				{
					CStringA name, value;

					if ( !m_pHttp3Ctx->GetHeaderBase(&buf) ) {
						m_ProxyLastError = WSAECONNREFUSED;
						m_ProxyStatus = PRST_NONE;
					}

					m_ProxyCode = 0;
					m_ProxyAuth.RemoveAll();

					while ( m_pHttp3Ctx->GetQPackField(&buf, name, value) ) {
						if ( name.Compare(":status") == 0 )
							m_ProxyCode = atoi(value);
						else if ( name.CompareNoCase("authorization") == 0 )
							m_ProxyAuth.SetArray(LocalStr(value));
						else if ( name.CompareNoCase("WWW-Authenticate") == 0 )
							m_ProxyAuth.SetArray(LocalStr(value));
						else if ( name.CompareNoCase("Proxy-Authenticate") == 0 )
							m_ProxyAuth.SetArray(LocalStr(value));
					}

					switch(m_ProxyCode) {
					case 200:	// OK
						m_RealHostAddr = m_ProxyHostAddr;
						m_RealHostPort = m_ProxyPort;
						m_ProxyStatus = PRST_HTTP3_TUNNEL;
						m_pFifoProxy->SendFdEvents(FIFO_EXTOUT, FD_CONNECT, NULL);
						break;
					case 401:	// Authorization Required
					case 407:	// Proxy-Authorization Required
						if ( m_ProxyAuth.Find(_T("basic")) >= 0 ) {
							A1.Clear();
							m_pHttp3Ctx->PutHeaderBase(&A1);
							m_pHttp3Ctx->PutQPackField(&A1, ":method", "CONNECT");
							mbs.Format("%s:%d", TstrToMbs(m_ProxyHost), m_ProxyPort);
							m_pHttp3Ctx->PutQPackField(&A1, ":authority", mbs);
							m_pHttp3Ctx->PutQPackField(&A1, "user-agent", "RLogin");
							tmp.Format(_T("%s:%s"), (LPCTSTR)m_ProxyUser, (LPCTSTR)m_ProxyPass);
							m_pFifoProxy->DocMsgRemoteStr(tmp, mbs); 
							buf.Base64Encode((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
							mbs.Format("basic %s", TstrToMbs((LPCTSTR)buf));
							m_pHttp3Ctx->PutQPackField(&A1, (m_ProxyCode == 407 ? "proxy-authorization" :"authorization"), mbs);

							m_pHttp3Ctx->SendQPackFrame(HTTP3_TYPE_HEADERS, A1.GetPtr(), A1.GetSize());
							m_ProxyStatus = PRST_HTTP3_BASIC;
							break;
						} else if ( m_ProxyAuth.Find(_T("digest")) >= 0 ) {
							if ( !ProxyMakeDigest(dig) )
								break;
							A1.Clear();
							m_pHttp3Ctx->PutHeaderBase(&A1);
							m_pHttp3Ctx->PutQPackField(&A1, ":method", "CONNECT");
							mbs.Format("%s:%d", TstrToMbs(m_ProxyHost), m_ProxyPort);
							m_pHttp3Ctx->PutQPackField(&A1, ":authority", mbs);
							m_pHttp3Ctx->PutQPackField(&A1, "user-agent", "RLogin");
							m_pFifoProxy->DocMsgRemoteStr(dig, mbs);
							m_pHttp3Ctx->PutQPackField(&A1, (m_ProxyCode == 407 ? "proxy-authorization" :"authorization"), mbs);

							m_pHttp3Ctx->SendQPackFrame(HTTP3_TYPE_HEADERS, A1.GetPtr(), A1.GetSize());
							m_ProxyStatus = PRST_HTTP3_DIGEST;
							break;
						}
						// no break;
					default:
						m_pDocument->m_ErrorPrompt.Format(_T("http/3 status error %d"), m_ProxyCode);
						m_ProxyLastError = WSAECONNREFUSED;
						m_ProxyStatus = PRST_NONE;
						break;
					}
				}
				break;
			case HTTP3_TYPE_CANCEL_PUSH:
			case HTTP3_TYPE_SETTINGS:
			case HTTP3_TYPE_PUSH_PROMISE:
			case HTTP3_TYPE_GOAWAY:
			case HTTP3_TYPE_MAX_PUSH_ID:
				break;
			default:
				m_pDocument->m_ErrorPrompt.Format(_T("http/3 type error %d"), n);
				m_ProxyLastError = WSAECONNREFUSED;
				m_ProxyStatus = PRST_NONE;
				break;
			}
			break;
		}
	}
	return FALSE;
}
