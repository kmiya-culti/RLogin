// ExtSocket.cpp: CExtSocket �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "PassDlg.h"
#include "ssh.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef	USE_FIFOBUF
//////////////////////////////////////////////////////////////////////
// CSockBuffer

CSockBuffer::CSockBuffer()
{
	m_Type = 0;
	m_Ofs = m_Len = 0;
	m_Max = 4096;
	m_Data = new BYTE[m_Max];
	m_Left = m_Right = NULL;
}
CSockBuffer::~CSockBuffer()
{
	delete [] m_Data;
}
void CSockBuffer::AddAlloc(int len)
{
	if ( (len += m_Len) <= m_Max )
		return;
	else if ( (len -= m_Ofs) <= m_Max ) {
		if ( (m_Len -= m_Ofs) > 0 )
			memcpy(m_Data, m_Data + m_Ofs, m_Len);
		m_Ofs = 0;
		return;
	}

	m_Max = (len * 2 + 4096) & ~(4096 - 1);
	LPBYTE tmp = new BYTE[m_Max];

	if ( (m_Len -= m_Ofs) > 0 )
		memcpy(tmp, m_Data + m_Ofs, m_Len);
	m_Ofs = 0;

	delete [] m_Data;
	m_Data = tmp;
}
void CSockBuffer::Apend(LPBYTE buff, int len, int type)
{
	AddAlloc(len);
	memcpy(m_Data + m_Len, buff, len);
	m_Len += len;
	m_Type = type;
}
#endif

//////////////////////////////////////////////////////////////////////
// CExtSocket

static const int EventMask[2] = { FD_READ, FD_OOB };

CExtSocket::CExtSocket(class CRLoginDoc *pDoc)
{
	m_pDocument = pDoc;
	m_Type = ESCT_DIRECT;
	m_Fd = (-1);
	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;
	m_RecvSyncMode = SYNC_NONE;
	m_RecvBufSize = RECVDEFSIZ;

#ifndef	USE_FIFOBUF
	m_ListenMax = 0;
	m_OnRecvFlag = 0;
	m_DestroyFlag = FALSE;
	m_pRecvEvent = NULL;

	m_FreeHead = m_ProcHead = m_RecvHead = m_SendHead = NULL;
#endif

	m_RecvSize = m_RecvProcSize = 0;
	m_SendSize = 0;
	m_ProxyStatus = PRST_NONE;
	m_SSL_mode  = 0;
	m_SSL_pCtx  = NULL;
	m_SSL_pSock = NULL;
	m_SSL_keep  = FALSE;
	m_bConnect = FALSE;
	m_bCallConnect = TRUE;
	m_pSyncEvent = NULL;

#ifndef	USE_FIFOBUF
	m_AddrInfoTop = NULL;
	m_bUseProc = FALSE;

	m_TransmitLimit = 0;
	m_SendLimit.clock = m_RecvLimit.clock = clock();
	m_SendLimit.size  = m_RecvLimit.size  = 0;
	m_SendLimit.timer = m_RecvLimit.timer = 0;
#endif

#ifdef	USE_FIFOBUF
	m_pFifoLeft   = NULL;
	m_pFifoMid    = NULL;
	m_pFifoRight  = NULL;
	m_pFifoSync   = NULL;
	m_pFifoProxy  = NULL;
	m_pFifoListen = NULL;
#endif
}

CExtSocket::~CExtSocket()
{
#ifdef	USE_FIFOBUF
	Close();
#else
	if ( m_pRecvEvent != NULL )
		delete m_pRecvEvent;

	while ( m_ProcHead != NULL )
		m_ProcHead = RemoveHead(m_ProcHead);

	while ( m_RecvHead != NULL )
		m_RecvHead = RemoveHead(m_RecvHead);

	while ( m_SendHead != NULL )
		m_SendHead = RemoveHead(m_SendHead);

	CSockBuffer *sp;
	while ( (sp = m_FreeHead) != NULL ) {
		m_FreeHead = sp->m_Left;
		delete sp;
	}
#endif
}

void CExtSocket::ResetOption()
{
#ifdef	USE_FIFOBUF
	if ( m_pDocument == NULL || m_pFifoLeft == NULL )
		return;


	if ( m_pFifoLeft->m_Type == FIFO_TYPE_SOCKET ) {
		BOOL opval;
		m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_LIMITSIZE, m_pDocument->m_TextRam.IsOptEnable(TO_RLTRSLIMIT) ? (m_pDocument->m_ParamTab.m_TransmitLimit * 1024) : 0);
		opval = m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) ? TRUE : FALSE;
		m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, SO_KEEPALIVE, 0, (void *)&opval);
		opval = m_pDocument->m_TextRam.IsOptEnable(TO_RLNODELAY) ? TRUE : FALSE;
		m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, TCP_NODELAY, 0, (void *)&opval);
	}
#else
	BOOL opval;
	int oplen;

	if ( m_pDocument == NULL )
		return;

	m_TransmitLimit = m_pDocument->m_TextRam.IsOptEnable(TO_RLTRSLIMIT) ? (m_pDocument->m_ParamTab.m_TransmitLimit * 1024) : 0;

	opval = FALSE;
	oplen = sizeof(opval);
	::getsockopt(m_Fd, SOL_SOCKET, SO_KEEPALIVE, (char *)(&opval), (int *)&oplen);

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) && !opval ) {
		opval = TRUE;
		::setsockopt(m_Fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)(&opval), sizeof(opval));
	} else if ( !m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) && opval ) {
		opval = FALSE;
		::setsockopt(m_Fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)(&opval), sizeof(opval));
	}

	opval = FALSE;
	oplen = sizeof(opval);
	::getsockopt(m_Fd, SOL_SOCKET, TCP_NODELAY, (char *)(&opval), (int *)&oplen);

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLNODELAY) && !opval ) {
		opval = TRUE;
		::setsockopt(m_Fd, SOL_SOCKET, TCP_NODELAY, (const char *)(&opval), sizeof(opval));
	} else if ( !m_pDocument->m_TextRam.IsOptEnable(TO_RLNODELAY) && opval ) {
		opval = FALSE;
		::setsockopt(m_Fd, SOL_SOCKET, TCP_NODELAY, (const char *)(&opval), sizeof(opval));
	}
#endif
}
void CExtSocket::Destroy()
{
	Close();
#ifdef	USE_FIFOBUF
	delete this;
#else
	if ( (m_OnRecvFlag & RECV_ACTIVE) != 0 ) {
		if ( !m_DestroyFlag )
			((CRLoginApp *)AfxGetApp())->AddIdleProc(IDLEPROC_SOCKET, this);
		m_DestroyFlag = TRUE;
	} else {
		if ( m_DestroyFlag )
			((CRLoginApp *)AfxGetApp())->DelIdleProc(IDLEPROC_SOCKET, this);
		delete this;
	}
#endif
}

#ifndef	USE_FIFOBUF
CSockBuffer *CExtSocket::AddTail(CSockBuffer *sp, CSockBuffer *head)
{
	if ( head == NULL ) {
		sp->m_Left = sp;
		sp->m_Right = sp;
		return sp;
	} else {
		sp->m_Left = head->m_Left;
		sp->m_Right = head;
		head->m_Left = sp;
		sp->m_Left->m_Right = sp;
	}
	return head;
}
CSockBuffer *CExtSocket::AddHead(CSockBuffer *sp, CSockBuffer *head)
{
	if ( head == NULL ) {
		sp->m_Left = sp;
		sp->m_Right = sp;
	} else {
		sp->m_Left = head->m_Left;
		sp->m_Right = head;
		head->m_Left = sp;
		sp->m_Left->m_Right = sp;
	}
	return sp;
}
CSockBuffer *CExtSocket::DetachHead(CSockBuffer *head)
{
	CSockBuffer *sp;
	if ( (sp = head) == NULL )
		return NULL;
	else if ( sp == sp->m_Right )
		head = NULL;
	else {
		sp->m_Right->m_Left = sp->m_Left;
		sp->m_Left->m_Right = sp->m_Right;
		head = sp->m_Right;
	}
	return head;
}
CSockBuffer *CExtSocket::RemoveHead(CSockBuffer *head)
{
	CSockBuffer *sp;
	if ( (sp = head) == NULL )
		return NULL;
	else if ( sp == sp->m_Right )
		head = NULL;
	else {
		sp->m_Right->m_Left = sp->m_Left;
		sp->m_Left->m_Right = sp->m_Right;
		head = sp->m_Right;
	}
	FreeBuffer(sp);
	return head;
}
CSockBuffer *CExtSocket::AllocBuffer()
{
	CSockBuffer *sp;

	m_FreeSema.Lock();
	if ( m_FreeHead == NULL )
		m_FreeHead = new CSockBuffer;
	sp = m_FreeHead;
	m_FreeHead = sp->m_Left;
	m_FreeSema.Unlock();

	sp->Clear();
	return sp;
}
void CExtSocket::FreeBuffer(CSockBuffer *sp)
{
	m_FreeSema.Lock();
	sp->m_Left = m_FreeHead;
	m_FreeHead = sp;
	m_FreeSema.Unlock();
}
#endif

//////////////////////////////////////////////////////////////////////

void CExtSocket::PostMessage(WPARAM wParam, LPARAM lParam)
{
	::AfxGetMainWnd()->PostMessage(WM_SOCKSEL, wParam, lParam);
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
BOOL CExtSocket::AsyncGetHostByName(LPCTSTR pHostName)
{
	CString host;
	PunyCodeAdress(pHostName, host);
	return GetMainWnd()->SetAsyncHostAddr(ASYNC_GETADDR, host, this);
}
BOOL CExtSocket::AsyncCreate(LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress, int nSocketType)
{
#ifdef	USE_FIFOBUF
	return Create(lpszHostAddress, nHostPort, lpszRemoteAddress, nSocketType);
#else
	if ( lpszHostAddress == NULL )
		return Create(lpszHostAddress, nHostPort, lpszRemoteAddress, nSocketType);

	//m_RealHostAddr   = lpszHostAddress;
	PunyCodeAdress(lpszHostAddress, m_RealHostAddr);

	m_RealHostPort   = nHostPort;

	//m_RealRemoteAddr = (lpszRemoteAddress != NULL ? lpszRemoteAddress : _T(""));
	if ( lpszRemoteAddress != NULL )
		PunyCodeAdress(lpszRemoteAddress, m_RealRemoteAddr);
	else
		m_RealRemoteAddr.Empty();

	m_RealSocketType = nSocketType;

	ADDRINFOT hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = nSocketType;
	hints.ai_flags = AI_PASSIVE;

	return GetMainWnd()->SetAsyncAddrInfo(ASYNC_CREATEINFO, m_RealHostAddr, m_RealHostPort, &hints, this);
#endif
}
BOOL CExtSocket::AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
#ifdef	USE_FIFOBUF
	return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);
#else
	if ( lpszHostAddress == NULL || nHostPort == (-1) )
		return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);

	//m_RealHostAddr   = lpszHostAddress;
	PunyCodeAdress(lpszHostAddress, m_RealHostAddr);
	m_RealHostPort   = nHostPort;
	m_RealRemotePort = nSocketPort;
	m_RealSocketType = nSocketType;

	ADDRINFOT hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = GetFamily();
	hints.ai_socktype = nSocketType;

	return GetMainWnd()->SetAsyncAddrInfo(ASYNC_OPENINFO, m_RealHostAddr, m_RealHostPort, &hints, this);
#endif
}
BOOL CExtSocket::ProxyOpen(int mode, BOOL keep, LPCTSTR ProxyHost, UINT ProxyPort, LPCTSTR ProxyUser, LPCTSTR ProxyPass, LPCTSTR RealHost, UINT RealPort)
{
	//m_RealHostAddr   = ProxyHost;
	PunyCodeAdress(ProxyHost, m_RealHostAddr);
	m_RealHostPort   = ProxyPort;
	m_RealRemotePort = 0;
	m_RealSocketType = SOCK_STREAM;

	switch(mode & 7) {
	case 0: m_ProxyStatus = PRST_NONE;				break;	// Non
	case 1: m_ProxyStatus = PRST_HTTP_START;		break;	// HTTP
	case 2: m_ProxyStatus = PRST_SOCKS4_START;		break;	// SOCKS4
	case 3: m_ProxyStatus = PRST_SOCKS5_START;		break;	// SOCKS5
	case 4: m_ProxyStatus = PRST_HTTP_BASIC_START;	break;	// HTTP(Basic)
	}

	switch(mode >> 3) {
	case 0:  m_SSL_mode = 0; break;	// Non
	default: m_SSL_mode = 1; break;	// SSL/TLS
	}

	m_SSL_keep = keep;

	//m_ProxyHost = RealHost;
	PunyCodeAdress(RealHost, m_ProxyHost);
	m_ProxyPort = RealPort;
	m_ProxyUser = ProxyUser;
	m_ProxyPass = ProxyPass;

	if ( m_ProxyStatus == PRST_NONE ) {
		m_RealHostAddr   = m_ProxyHost;
		m_RealHostPort   = m_ProxyPort;
	}

#ifdef	USE_FIFOBUF
	return Open(m_RealHostAddr, m_RealHostPort, m_RealRemotePort, m_RealSocketType);
#else
	ADDRINFOT hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = GetFamily();
	hints.ai_socktype = m_RealSocketType;

	return GetMainWnd()->SetAsyncAddrInfo(ASYNC_OPENINFO, m_RealHostAddr, m_RealHostPort, &hints, this);
#endif
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::Create(LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress, int nSocketType, void *pAddrInfo)
{
#ifdef	USE_FIFOBUF
	FifoLink();
	m_pFifoListen = new CListenSocket(FIFO_STDIN, m_pFifoMid);
	return m_pFifoListen->Open(lpszHostAddress, nHostPort, GetFamily(), nSocketType, 128);
#else
	m_SocketEvent = FD_ACCEPT | FD_CLOSE;

	int opval;
	ADDRINFOT hints, *ai, *aitop;
	char *addr = NULL;
	CString str, host;
	SOCKET fd;

	if ( pAddrInfo != NULL )
		aitop = (ADDRINFOT *)pAddrInfo;
	else {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = nSocketType;
		hints.ai_flags = AI_PASSIVE;
		str.Format(_T("%d"), nHostPort);

		PunyCodeAdress(lpszHostAddress, host);

		if ( GetAddrInfo(host, str, &hints, &aitop) != 0)
			return FALSE;
	}

	m_Fd = (-1);
	m_ListenMax = 0;

	for ( ai = aitop ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) )
			continue;

		if ( !GetMainWnd()->SetAsyncSelect(fd, this, m_SocketEvent) ) {
			::closesocket(fd);
			continue;
		}

		opval = 1;
		if ( ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)(&opval), sizeof(opval)) != 0 ) {
			GetMainWnd()->DelAsyncSelect(fd, this);
			::closesocket(fd);
			continue;
		}

		if ( ::bind(fd, ai->ai_addr, (int)ai->ai_addrlen) < 0 || ::listen(fd, 128) == SOCKET_ERROR ) {
			GetMainWnd()->DelAsyncSelect(fd, this);
			::closesocket(fd);
			continue;
		}

		if ( m_ListenMax < LISTENSOCKS )
			m_FdTab[m_ListenMax++] = fd;
		else {
			GetMainWnd()->DelAsyncSelect(fd, this);
			::closesocket(fd);
		}
	}
	FreeAddrInfo(aitop);
	return (m_ListenMax == 0 ? FALSE : TRUE);
#endif
}

#ifndef	USE_FIFOBUF
BOOL CExtSocket::SyncOpenAddrInfo(UINT nSockPort)
{
	DWORD val;
	ADDRINFOT *ai;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
	struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	if ( m_AddrInfoTop == NULL )
		return FALSE;

	for ( ai = m_AddrInfoNext ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (m_Fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) )
			continue;

		if ( ai->ai_family == AF_INET ) {
			memset(&in, 0, sizeof(in));
			in.sin_family = AF_INET;
			in.sin_addr.s_addr = INADDR_ANY;
			in.sin_port = htons(nSockPort);
			if ( ::bind(m_Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR ) {
				::closesocket(m_Fd);
				m_Fd = (-1);
				continue;
			}
		} else {	// AF_INET6
			memset(&in6, 0, sizeof(in6));
			in6.sin6_family = AF_INET6;
			in6.sin6_addr = in6addr_any;
			in6.sin6_port = htons(nSockPort);
			if ( ::bind(m_Fd, (struct sockaddr *)&in6, sizeof(in6)) == SOCKET_ERROR ) {
				::closesocket(m_Fd);
				m_Fd = (-1);
				continue;
			}
		}

	    if ( ::connect(m_Fd, ai->ai_addr, (int)ai->ai_addrlen) == SOCKET_ERROR ) {
			::closesocket(m_Fd);
			m_Fd = (-1);
			continue;
		} else
			PostMessage(m_Fd, WSAMAKESELECTREPLY(FD_CONNECT, 0));

		if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) ) {
			::closesocket(m_Fd);
			m_Fd = (-1);
			continue;
		}

		val = 1;
		if ( ::ioctlsocket(m_Fd, FIONBIO, &val) != 0 ) {
			GetMainWnd()->DelAsyncSelect(m_Fd, this);
			::closesocket(m_Fd);
			m_Fd = (-1);
			continue;
		}

		break;
	}

	if ( ai == NULL ) {
		m_AddrInfoTop = m_AddrInfoNext = NULL;
		return FALSE;
	} else {
		m_AddrInfoNext = ai->ai_next;
		return TRUE;
	}
}
BOOL CExtSocket::OpenAddrInfo()
{
	DWORD val;
	ADDRINFOT *ai;

	if ( m_Fd != (-1) ) {
		GetMainWnd()->DelAsyncSelect(m_Fd, this);
		::shutdown(m_Fd, SD_BOTH);
		::closesocket(m_Fd);
		m_Fd = (-1);
	}

	if ( m_AddrInfoTop == NULL )
		return FALSE;

	for ( ai = m_AddrInfoNext ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (m_Fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) )
			continue;

		if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) ) {
			::closesocket(m_Fd);
			m_Fd = (-1);
			continue;
		}

		val = 1;
		if ( ::ioctlsocket(m_Fd, FIONBIO, &val) != 0 ) {
			GetMainWnd()->DelAsyncSelect(m_Fd, this);
			::closesocket(m_Fd);
			m_Fd = (-1);
			continue;
		}

	    if ( ::connect(m_Fd, ai->ai_addr, (int)ai->ai_addrlen) == SOCKET_ERROR ) {
			if ( WSAGetLastError() != WSAEWOULDBLOCK ) {
				GetMainWnd()->DelAsyncSelect(m_Fd, this);
				::closesocket(m_Fd);
				m_Fd = (-1);
				continue;
			}
		} else
			PostMessage(m_Fd, WSAMAKESELECTREPLY(FD_CONNECT, 0));

		break;
	}

	if ( ai == NULL ) {
		FreeAddrInfo(m_AddrInfoTop);
		m_AddrInfoTop = m_AddrInfoNext = NULL;
		return FALSE;
	} else {
		m_AddrInfoNext = ai->ai_next;
		return TRUE;
	}
}
#endif

BOOL CExtSocket::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
#ifdef	USE_FIFOBUF
	FifoLink();

	if ( m_ProxyStatus != PRST_NONE ) {
		ASSERT(m_pFifoProxy == NULL);

		m_pFifoProxy = new CFifoProxy(m_pDocument, this);
		CFifoBase::Link(m_pFifoProxy, FIFO_STDIN, m_pFifoMid, FIFO_STDIN);
	}

	if ( (m_pFifoLeft->m_nBufSize = m_RecvBufSize) > FIFO_BUFDEFMAX )
		m_pFifoLeft->m_nBufSize = FIFO_BUFDEFMAX;

	switch(m_Type) {
	case ESCT_RLOGIN:
	case ESCT_TELNET:
		m_pFifoMid->m_bFlowCtrl = TRUE;
		break;
	case ESCT_SSH_MAIN:
	case ESCT_SSH_CHAN:
		m_pFifoMid->m_bFlowCtrl = FALSE;
		break;
	}
	
	m_pFifoLeft->m_nLimitSize = m_pDocument->m_TextRam.IsOptEnable(TO_RLTRSLIMIT) ? (m_pDocument->m_ParamTab.m_TransmitLimit * 1024) : 0;

	CString host;
	PunyCodeAdress(lpszHostAddress, host);

	return m_pFifoLeft->Open(host, nHostPort, nSocketPort, GetFamily(), nSocketType);
#else
	while ( m_ProcHead != NULL )
		m_ProcHead = RemoveHead(m_ProcHead);

	while ( m_RecvHead != NULL )
		m_RecvHead = RemoveHead(m_RecvHead);

	while ( m_SendHead != NULL )
		m_SendHead = RemoveHead(m_SendHead);

	m_TransmitLimit = m_pDocument->m_TextRam.IsOptEnable(TO_RLTRSLIMIT) ? (m_pDocument->m_ParamTab.m_TransmitLimit * 1024) : 0;

	m_RecvSize = m_RecvProcSize = 0;
	m_SendSize = 0;

	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;

	if ( m_SSL_mode != 0 )
		m_SocketEvent &= ~FD_READ;

	if ( pAddrInfo != NULL )
		m_AddrInfoTop = m_AddrInfoNext = (ADDRINFOT *)pAddrInfo;
	else {
		ADDRINFOT hints;
		CString str, host;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = GetFamily();
		hints.ai_socktype = nSocketType;
		str.Format(_T("%d"), nHostPort);

		PunyCodeAdress(lpszHostAddress, host);

		if ( GetAddrInfo(host, str, &hints, &m_AddrInfoNext) != 0 )
			return FALSE;

		m_AddrInfoTop = m_AddrInfoNext;
	}

	if ( nSocketPort != 0 ) {
		if ( !SyncOpenAddrInfo(nSocketPort) )
			return FALSE;
		FreeAddrInfo(m_AddrInfoTop);
		m_AddrInfoTop = NULL;

	} else if ( !OpenAddrInfo() )
		return FALSE;

	return TRUE;
#endif
}
void CExtSocket::Close()
{
#ifdef	USE_FIFOBUF
	SSLClose();
	FifoUnlink();
#else
	((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);

	SSLClose();

	if ( m_AddrInfoTop != NULL ) {
		FreeAddrInfo(m_AddrInfoTop);
		m_AddrInfoTop = NULL;
	}

	if ( m_Fd != (-1) ) {
		GetMainWnd()->DelAsyncSelect(m_Fd, this);
		::shutdown(m_Fd, SD_BOTH);
		::closesocket(m_Fd);
		m_Fd = (-1);
	}

	for ( int n = 0 ; n < m_ListenMax ; n++ ) {
		GetMainWnd()->DelAsyncSelect(m_FdTab[n], this);
		::shutdown(m_FdTab[n], SD_BOTH);
		::closesocket(m_FdTab[n]);
	}
	m_ListenMax = 0;
#endif
	m_bConnect = FALSE;
}
int CExtSocket::Accept(class CExtSocket *pSock, SOCKET hand)
{
#ifdef	USE_FIFOBUF
	SOCKET sock;
	struct sockaddr_storage sa;
	int len;

    memset(&sa, 0, sizeof(sa));
    len = sizeof(sa);
    if ( (sock = accept(hand, (struct sockaddr *)&sa, &len)) < 0 )
		return FALSE;

	if ( !pSock->Attach(m_pDocument, sock) ) {
		::closesocket(hand);
		return FALSE;
	}
	return TRUE;
#else
	unsigned long val = 1;
	SOCKET sock;
	struct sockaddr_storage sa;
	int len;

    memset(&sa, 0, sizeof(sa));
    len = sizeof(sa);
    if ( (sock = accept(hand, (struct sockaddr *)&sa, &len)) < 0 )
		return FALSE;

	if ( ::ioctlsocket(sock, FIONBIO, &val) != 0 ) {
		::closesocket(sock);
		return FALSE;
	}

	if ( !pSock->Attach(m_pDocument, sock) ) {
		::closesocket(sock);
		return FALSE;
	}

	return TRUE;
#endif
}
int CExtSocket::Attach(class CRLoginDoc *pDoc, SOCKET fd)
{
#ifdef	USE_FIFOBUF
	FifoLink();

	m_Fd = fd;
	GetMainWnd()->SetAsyncSelect(m_Fd, this, 0);

	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET )
		return FALSE;

	return ((CFifoSocket *)m_pFifoLeft)->Attach(fd);
#else
	Close();

	m_pDocument = pDoc;
	m_Fd = fd;

	while ( m_ProcHead != NULL )
		m_ProcHead = RemoveHead(m_ProcHead);

	while ( m_RecvHead != NULL )
		m_RecvHead = RemoveHead(m_RecvHead);

	while ( m_SendHead != NULL )
		m_SendHead = RemoveHead(m_SendHead);

	m_RecvSize = m_RecvProcSize = 0;
	m_SendSize = 0;

	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;

	if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) )
		return FALSE;

	return TRUE;
#endif
}

/*
CFifoSocket		CFifoMiddle														CFifoSync		CFifoDocument
	STDOUT		STDIN->OnReceiveCallBack  ExtSocket::OnReceiveCallBack->EXTOUT	STDIN EXTOUT    STDIN->OnReceiveProcBack
	STDINT		STDOUT<-ExtSocket::Send							   Send<-EXTIN	STDOUT EXTIN    STDOUT<-PreSend<-m_pDocument->SocketSend
*/
int CExtSocket::Receive(void* lpBuf, int nBufLen, int nFlags)
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::Receive\n");

	if ( m_pFifoSync == NULL )
		return 0;

	return m_pFifoSync->Read(FIFO_STDIN, (BYTE *)lpBuf, nBufLen);
#else
	int n;
	int len = 0;
	LPBYTE buf = (LPBYTE)lpBuf;

	m_RecvSema.Lock();
	while ( nBufLen > 0 && m_ProcHead != NULL && m_ProcHead->m_Type == nFlags ) {
		if ( (n = m_ProcHead->GetSize()) > nBufLen )
			n = nBufLen;
		memcpy(buf, m_ProcHead->GetPtr(), n);
		if ( m_ProcHead->Consume(n) <= 0 )
			m_ProcHead = RemoveHead(m_ProcHead);
		m_RecvProcSize -= n;
		if ( m_RecvProcSize <= RECVMINSIZ )
			PostMessage(m_Fd, WSAMAKESELECTREPLY(FD_RECVEMPTY, 0));
		len += n;
		buf += n;
		nBufLen -= n;
	}
	m_RecvSema.Unlock();
	return len;
#endif
}
int CExtSocket::SyncReceive(void* lpBuf, int nBufLen, int mSec, BOOL *pAbort)
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoSync == NULL || (pAbort != NULL && *pAbort) )
		return 0;

	return m_pFifoSync->Recv(FIFO_STDIN, (BYTE *)lpBuf, nBufLen, mSec);
#else
	int n;
	int len = 0;
	LPBYTE buf = (LPBYTE)lpBuf;
	clock_t st;

	st = clock();
	while ( !*pAbort ) {
		m_RecvSema.Lock();
		while ( nBufLen > 0 && m_ProcHead != NULL ) {
			if ( m_ProcHead->m_Type != 0 ) {
				m_RecvProcSize -= m_ProcHead->GetSize();
				m_ProcHead = RemoveHead(m_ProcHead);
			} else {
				if ( (n = m_ProcHead->GetSize()) > nBufLen )
					n = nBufLen;
				memcpy(buf, m_ProcHead->GetPtr(), n);
				if ( m_ProcHead->Consume(n) <= 0 )
					m_ProcHead = RemoveHead(m_ProcHead);
				m_RecvProcSize -= n;
				len += n;
				buf += n;
				nBufLen -= n;
			}
			if ( m_RecvProcSize <= RECVMINSIZ )
				PostMessage(m_Fd, WSAMAKESELECTREPLY(FD_RECVEMPTY, 0));
		}

		if ( len > 0 || nBufLen <= 0 ) {
			m_RecvSema.Unlock();
			break;
		} else if ( m_pRecvEvent != NULL ) {
			m_RecvSyncMode |= SYNC_EVENT;
			m_RecvSema.Unlock();
			if ( (n = (int)(clock() - st) * 1000 / CLOCKS_PER_SEC) >= mSec )
				break;
//			TRACE("SyncReceive wait %d\n", mSec - n);
			WaitForSingleObject(m_pRecvEvent->m_hObject, mSec - n);
		} else {
			m_RecvSema.Unlock();
			if ( (n = (int)(clock() - st) * 1000 / CLOCKS_PER_SEC) >= mSec )
				break;
			Sleep(mSec - n);
		}
	}
	return len;
#endif
}
void CExtSocket::SyncReceiveBack(void *lpBuf, int nBufLen)
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::SyncReceiveBack %d\n", nBufLen);

	if ( m_pFifoSync != NULL )
		m_pFifoSync->Insert(FIFO_STDIN, (BYTE *)lpBuf, nBufLen);
#else
	CSockBuffer *sp = AllocBuffer();
	m_RecvSema.Lock();
	sp->Apend((LPBYTE)lpBuf, nBufLen, 0);
	m_ProcHead = AddHead(sp, m_ProcHead);
	m_RecvProcSize += nBufLen;
	m_RecvSema.Unlock();
#endif
}
void CExtSocket::SyncAbort()
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoSync != NULL )
		m_pFifoSync->Abort();
#else
	if ( m_pRecvEvent == NULL )
		return;
	m_pRecvEvent->SetEvent();
#endif
}
void CExtSocket::SetRecvSyncMode(BOOL mode)
{
#ifdef	USE_FIFOBUF
	if ( mode ) {		// in Sync
		if ( m_pFifoSync != NULL )
			return;

		m_pFifoSync = new CFifoSync(m_pDocument);
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

	m_RecvSyncMode = (mode ? SYNC_ACTIVE : SYNC_NONE); 
#else
	if ( m_pRecvEvent == NULL )
		m_pRecvEvent = new CEvent(FALSE, FALSE);
	m_RecvSyncMode = (mode ? SYNC_ACTIVE : SYNC_NONE); 
#endif
}

#ifndef	USE_FIFOBUF
void CExtSocket::CheckLimit(int len, struct _LimitData *pLimit)
{
	int msec;
	clock_t now = clock();

	pLimit->size += len;

	if ( (msec = now - pLimit->clock) >= (CLOCKS_PER_SEC * 2) ) {
		pLimit->size = pLimit->size * CLOCKS_PER_SEC / msec;
		pLimit->clock = now - CLOCKS_PER_SEC;
		msec = CLOCKS_PER_SEC;
	}

	if ( msec > 0 && (int)(pLimit->size * CLOCKS_PER_SEC / msec) > m_TransmitLimit ) {
		if ( (msec = RECVBUFSIZ * 1000 / m_TransmitLimit) < 10 )
			msec = 10;
		else if ( msec > 500 )
			msec = 500;
		pLimit->timer = ((CMainFrame *)::AfxGetMainWnd())->SetTimerEvent(msec, TIMEREVENT_SOCK, this);
	}
}
#endif

#ifdef	USE_FIFOBUF
int CExtSocket::SyncSend(const void *lpBuf, int nBufLen, int nFlags)
{
	if ( m_pFifoSync == NULL ) {
		nBufLen = 0;
	} else {
		if ( nFlags == 0 )
			m_pFifoSync->Send(FIFO_STDOUT, (BYTE *)lpBuf, nBufLen);
		else if ( nFlags == MSG_OOB ) {
			WSABUF sbuf;
			sbuf.len = nBufLen;
			sbuf.buf = (CHAR *)lpBuf;
			m_pFifoSync->SendFdEvents(FIFO_STDOUT, FD_OOB, (void *)&sbuf);
		}
	}
	return nBufLen;
}
int CExtSocket::SyncExtSend(const void *lpBuf, int nBufLen, int nFlags)
{
	if ( m_pFifoSync == NULL ) {
		nBufLen = 0;
	} else {
		if ( nFlags == 0 )
			m_pFifoSync->Send(FIFO_EXTOUT, (BYTE *)lpBuf, nBufLen);
		else if ( nFlags == MSG_OOB ) {
			WSABUF sbuf;
			sbuf.len = nBufLen;
			sbuf.buf = (CHAR *)lpBuf;
			m_pFifoSync->SendFdEvents(FIFO_EXTOUT, FD_OOB, (void *)&sbuf);
		}
	}
	return nBufLen;
}
int CExtSocket::PreSend(const void* lpBuf, int nBufLen, int nFlags)
{
	if ( m_pFifoRight == NULL ) {
		nBufLen = 0;
	} else {
		if ( nFlags == 0 )
			m_pFifoRight->Write(FIFO_STDOUT, (BYTE *)lpBuf, nBufLen);
		else if ( nFlags == MSG_OOB ) {
			WSABUF sbuf;
			sbuf.len = nBufLen;
			sbuf.buf = (CHAR *)lpBuf;
			m_pFifoRight->SendFdEvents(FIFO_STDOUT, FD_OOB, (void *)&sbuf);
		}
	}
	return nBufLen;
}
#endif
int CExtSocket::Send(const void* lpBuf, int nBufLen, int nFlags)
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoMid == NULL )
		return 0;
	if ( nFlags == 0 )
		m_pFifoMid->Write(FIFO_STDOUT, (BYTE *)lpBuf, nBufLen);
	else if ( nFlags == MSG_OOB ) {
		WSABUF sbuf;
		sbuf.len = nBufLen;
		sbuf.buf = (CHAR *)lpBuf;
		m_pFifoMid->SendFdEvents(FIFO_STDOUT, FD_OOB, (void *)&sbuf);
	}
	return nBufLen;
#else
	int n;
	int len = nBufLen;
	CSockBuffer *sp;

	if ( nBufLen <= 0 )
		return len;

	while ( m_SendHead == NULL && m_SendLimit.timer == 0 ) {
		if ( m_SSL_mode != 0 )
			n = (m_SSL_pSock != NULL ? SSL_write(m_SSL_pSock, lpBuf, nBufLen) : 0);
		else
			n = (m_Fd != (-1) ? ::send(m_Fd, (char *)lpBuf, nBufLen, nFlags) : 0);

		//TRACE("Send %s %d\r\n", m_SSL_mode ? "SSL" : "", n);

		if ( n <= 0 )
			break;

		if ( m_TransmitLimit > 0 )
			CheckLimit(n, &m_SendLimit);

		if ( (nBufLen -= n) <= 0 ) {
			// Call OnSendEmpty
			PostMessage(m_Fd, WSAMAKESELECTREPLY(FD_WRITE, 0));
			return len;
		}

		lpBuf = (LPBYTE)lpBuf + n;
	}

	if ( m_Fd != (-1) && (m_SocketEvent & FD_WRITE) == 0 && m_SendLimit.timer == 0 ) {
		m_SocketEvent |= FD_WRITE;
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
		TRACE("Send SocketEvent FD_WRITE On %04x\n", m_SocketEvent);
	}

	if ( m_SendHead == NULL || (sp = m_SendHead->m_Left)->m_Type != nFlags || sp->GetSize() >= RECVBUFSIZ )
		sp = AllocBuffer();
	sp->Apend((LPBYTE)lpBuf, nBufLen, nFlags);
	if ( m_SendHead == NULL || sp != m_SendHead->m_Left )
		m_SendHead = AddTail(sp, m_SendHead);
	m_SendSize += nBufLen;

	//TRACE("Send Save %d\n", m_SendSize);
	
	return len;
#endif
}
void CExtSocket::SendFlash(int sec)
{
#ifdef	USE_FIFOBUF
	clock_t st;
	if ( m_pFifoMid == NULL )
		return;
	for ( st = clock() + sec * CLOCKS_PER_SEC ; m_pFifoMid->GetDataSize(FIFO_STDOUT) > 0 && st < clock() ; )
		Sleep(100);
#else
	clock_t st;

	for ( st = clock() + sec * CLOCKS_PER_SEC ; m_SendHead != NULL && st < clock() ; ) {
		OnSend();
		Sleep(100);
	}
#endif
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
#ifdef	USE_FIFOBUF
	if ( m_pFifoMid == NULL )
		return 0;
	return (m_pFifoMid->GetDataSize(FIFO_STDIN) + m_pFifoMid->GetDataSize(FIFO_EXTOUT));
#else
	return m_RecvSize + m_RecvProcSize;
#endif
}
int CExtSocket::GetRecvProcSize()
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoMid == NULL )
		return 0;
	return m_pFifoMid->GetDataSize(FIFO_STDIN);
#else
	return m_RecvProcSize;
#endif
}
int CExtSocket::GetSendSize()
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoMid == NULL )
		return 0;
	return m_pFifoMid->GetDataSize(FIFO_STDOUT);
#else
	return m_SendSize;
#endif
}
void CExtSocket::OnRecvEmpty()
{
#ifdef	USE_FIFOBUF
//	TRACE("OnRecvEmpty\r\n");
#else
	ReceiveFlowCheck();
#endif
}
void CExtSocket::OnSendEmpty()
{
//	TRACE("OnSendEmpty\r\n");

	if ( m_pSyncEvent != NULL ) {
		m_pSyncEvent->SetEvent();
		m_pSyncEvent = NULL;
	}
}
void CExtSocket::GetStatus(CString &str)
{
	CString tmp;
	CString name;
	int port;

#ifdef	USE_FIFOBUF
	tmp.Format(_T("Socket Type: %d\r\n"), m_Type);
	str += tmp;
	if ( m_pFifoMid != NULL ) {
		tmp.Format(_T("Receive Reserved: %d + %d Bytes\r\n"), m_pFifoMid->GetDataSize(FIFO_STDIN), m_pFifoMid->GetDataSize(FIFO_EXTOUT));
		str += tmp;
		tmp.Format(_T("Send Reserved: %d + %d Bytes\r\n"), m_pFifoMid->GetDataSize(FIFO_STDOUT), m_pFifoMid->GetDataSize(FIFO_EXTIN));
		str += tmp;
	}
#else
	tmp.Format(_T("Socket Type: %d\r\n"), m_Type);
	str += tmp;
	tmp.Format(_T("Receive Reserved: %d + %d Bytes\r\n"), m_RecvSize, m_RecvProcSize);
	str += tmp;
	tmp.Format(_T("Send Reserved: %d Bytes\r\n"), m_SendSize);
	str += tmp;
#endif

	if ( m_Fd != (-1) ) {
		GetSockName(m_Fd, name, &port);
		tmp.Format(_T("Connect %s#%d"), (LPCTSTR)name, port);
		str += tmp;
		GetPeerName(m_Fd, name, &port);
		tmp.Format(_T(" - %s#%d\r\n"), (LPCTSTR)name, port);
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

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::ProxyReadLine()
{

#ifdef	USE_FIFOBUF
	int c = (-1);
	BYTE buf[8];

	if ( m_pFifoProxy == NULL )
		return FALSE;

	while ( m_pFifoProxy->Read(FIFO_STDIN, buf, 1) > 0 ) {
		c = buf[0];
		if ( c == '\n' )
			break;
		else if ( c != '\r' )
			m_ProxyStr += (char)c;
	}

	return (c == '\n' ? TRUE : FALSE);
#else
	int c = 0;

	while ( m_RecvHead != NULL ) {
		if ( m_RecvHead->m_Type != 0 ) {
			m_RecvSize -= m_RecvHead->GetSize();
			m_RecvHead = RemoveHead(m_RecvHead);
		} else {
			while ( (c = m_RecvHead->GetByte()) != EOF ) {
				m_RecvSize -= 1;
				if ( c == '\n' )
					break;
				else if ( c != '\r' )
					m_ProxyStr += (char)c;
			}
			if ( m_RecvHead->GetSize() <= 0 )
				m_RecvHead = RemoveHead(m_RecvHead);
			if ( c == '\n' )
				break;
		}
	}

	return (c == '\n' ? TRUE : FALSE);
#endif
}
BOOL CExtSocket::ProxyReadBuff(int len)
{
#ifdef	USE_FIFOBUF
	int n;
	BYTE buf[FIFO_BUFDEFMAX];

	if ( m_pFifoProxy == NULL )
		return FALSE;

	while ( m_ProxyBuff.GetSize() < len ) {
		if ( (n = len - m_ProxyBuff.GetSize()) > FIFO_BUFDEFMAX )
			n = FIFO_BUFDEFMAX;
		if ( (n = m_pFifoProxy->Read(FIFO_STDIN, buf, n)) <= 0 )
			break;
		m_ProxyBuff.Apend(buf, n);
	}

	if ( m_ProxyBuff.GetSize() >= len )
		return TRUE;

	return FALSE;
#else
	int n;

	if ( m_ProxyBuff.GetSize() >= len )
		return TRUE;

	while ( m_RecvHead != NULL ) {
		if ( m_RecvHead->m_Type != 0 ) {
			m_RecvSize -= m_RecvHead->GetSize();
			m_RecvHead = RemoveHead(m_RecvHead);
		} else {
			if ( (n = len - m_ProxyBuff.GetSize()) <= 0 )
				return TRUE;
			if ( m_RecvHead->GetSize() < n )
				n = m_RecvHead->GetSize();
			m_ProxyBuff.Apend(m_RecvHead->GetPtr(), n);
			m_RecvHead->Consume(n);
			m_RecvSize -= n;
			if ( m_RecvHead->GetSize() <= 0 )
				m_RecvHead = RemoveHead(m_RecvHead);
			if ( m_ProxyBuff.GetSize() >= len )
				return TRUE;
		}
	}
	return FALSE;
#endif
}
#ifdef	USE_FIFOBUF
void CExtSocket::FifoLink()
{
	FifoUnlink();

	ASSERT(m_pFifoLeft == NULL && m_pFifoMid == NULL && m_pFifoRight == NULL && m_pFifoListen == NULL);

	m_pFifoLeft   = new CFifoSocket(m_pDocument);
	m_pFifoMid    = new CFifoMiddle(m_pDocument, this);
	m_pFifoRight  = new CFifoDocument(m_pDocument, this);

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

	if ( m_pFifoLeft != NULL && m_pFifoMid != NULL && m_pFifoRight != NULL )
		CFifoBase::MidUnLink(m_pFifoLeft, FIFO_STDIN, m_pFifoMid, FIFO_STDIN, m_pFifoRight, FIFO_STDIN);

	if ( m_pFifoListen != NULL ) {
		m_pFifoListen->Destroy();
		m_pFifoListen = NULL;
	}
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
BOOL CExtSocket::ProxyCheck()
{
	if ( m_pFifoProxy == NULL )
		return FALSE;

	if ( m_ProxyStatus != PRST_NONE && ProxyFunc() )
		return TRUE;

	CFifoBase::UnLink(m_pFifoProxy, FIFO_STDIN, TRUE);
	m_pFifoProxy->Destroy();
	m_pFifoProxy = NULL;

	return FALSE;
}
#endif
BOOL CExtSocket::ProxyFunc()
{
	int n, i;
	CBuffer buf;
	CString tmp;
	CBuffer A1, A2;
	CStringA mbs;
	u_char digest[EVP_MAX_MD_SIZE];

	while ( m_ProxyStatus != PRST_NONE ) {
		switch(m_ProxyStatus) {
		case PRST_HTTP_START:		// HTTP
			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("Connection: keep-alive\r\n")\
					   _T("\r\n"),
					   (LPCTSTR)m_ProxyHost, m_ProxyPort, (LPCTSTR)m_ProxyHost);
			mbs = tmp; CExtSocket::Send((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
			DEBUGLOG("ProxyFunc PRST_HTTP_START %s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			break;
		case PRST_HTTP_READLINE:
			if ( !ProxyReadLine() )
				return TRUE;
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
					tmp.Trim(_T(" \t\r\n"));
					if ( tmp.CompareNoCase(_T("Keep-Alive")) == 0 )
						m_ProxyConnect = TRUE;
				}
			}
			m_ProxyStatus = (m_ProxyLength > 0 ? PRST_HTTP_BODYREAD : PRST_HTTP_CODECHECK);
			m_ProxyBuff.Clear();
			break;
		case PRST_HTTP_BODYREAD:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return TRUE;
			m_ProxyStatus = PRST_HTTP_CODECHECK;
			break;
		case PRST_HTTP_CODECHECK:
			switch(m_ProxyCode) {
			case 200:
				if ( !m_SSL_keep )
					SSLClose();
				m_ProxyStatus = PRST_NONE;
				OnConnect();
				break;
			case 401:	// Authorization Required
			case 407:	// Proxy-Authorization Required
				if ( m_ProxyConnect ) {
					if ( m_ProxyAuth.Find(_T("basic")) >= 0 ) {
						m_ProxyStatus = PRST_HTTP_BASIC;
						break;
					} else if ( m_ProxyAuth.Find(_T("digest")) >= 0 ) {
						m_ProxyStatus = PRST_HTTP_DIGEST;
						break;
					}
				}
			default:
				SSLClose();
				m_pDocument->m_ErrorPrompt = (m_ProxyResult.GetSize() > 0 ? m_ProxyResult[0]: _T("HTTP Proxy Error"));
				PostMessage(m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
				m_ProxyStatus = PRST_NONE;
				return TRUE;
			}
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
			mbs = tmp; buf.Base64Encode((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("%sAuthorization: Basic %s\r\n")\
					   _T("\r\n"),
					   (LPCTSTR)m_ProxyHost, m_ProxyPort, (LPCTSTR)m_ProxyHost,
					   (m_ProxyCode == 407 ? _T("Proxy-") : _T("")),
					   (LPCTSTR)buf);
			mbs = tmp; CExtSocket::Send((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
			DEBUGLOG("ProxyFunc PRST_HTTP_BASIC %s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();

			SendFlash(3);
			if ( !m_SSL_keep )
				SSLClose();					// XXXXXXXX BUG???
			break;
		case PRST_HTTP_DIGEST:
			if (m_ProxyAuth.Find(_T("qop")) < 0 || m_ProxyAuth[_T("algorithm")].m_String.CompareNoCase(_T("md5")) != 0 || m_ProxyAuth.Find(_T("realm")) < 0 || m_ProxyAuth.Find(_T("nonce")) < 0 ) {
				m_pDocument->m_ErrorPrompt = _T("Authorization Paramter Error");
				PostMessage(m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
				m_ProxyStatus = PRST_NONE;
				return TRUE;
			}

			tmp.Format(_T("%s:%d"), (LPCTSTR)m_ProxyHost, m_ProxyPort);
			m_ProxyAuth[_T("uri")] = tmp;
			m_ProxyAuth[_T("qop")] = _T("auth");
			tmp.Format(_T("%08d"), 1);
			m_ProxyAuth[_T("nc")] = tmp;
			srand((int)time(NULL));
			tmp.Format(_T("%04x%04x%04x%04x"), rand(), rand(), rand(), rand());
			m_ProxyAuth[_T("cnonce")] = tmp;

			tmp.Format(_T("%s:%s:%s"), (LPCTSTR)m_ProxyUser, (LPCTSTR)m_ProxyAuth[_T("realm")], (LPCTSTR)m_ProxyPass);
			A1.md5(tmp);

			tmp.Format(_T("%s:%s"), _T("CONNECT"), (LPCTSTR)m_ProxyAuth[_T("uri")]);
			A2.md5(tmp);

			tmp.Format(_T("%s:%s:%s:%s:%s:%s"),
				(LPCTSTR)A1, (LPCTSTR)m_ProxyAuth[_T("nonce")], (LPCTSTR)m_ProxyAuth[_T("nc")],
				(LPCTSTR)m_ProxyAuth[_T("cnonce")], (LPCTSTR)m_ProxyAuth[_T("qop")], (LPCTSTR)A2);
			buf.md5(tmp);

			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("%sAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\",")\
					   _T(" algorithm=%s, response=\"%s\",")\
					   _T(" qop=%s, uri=\"%s\",")\
					   _T(" nc=%s, cnonce=\"%s\"")\
					   _T("\r\n")\
					   _T("\r\n"),
					   (LPCTSTR)m_ProxyHost, m_ProxyPort, (LPCTSTR)m_ProxyHost,
					   (m_ProxyCode == 407 ? _T("Proxy-") : _T("")),
					   (LPCTSTR)m_ProxyUser, (LPCTSTR)m_ProxyAuth[_T("realm")], (LPCTSTR)m_ProxyAuth[_T("nonce")],
					   (LPCTSTR)m_ProxyAuth[_T("algorithm")], (LPCTSTR)buf,
					   (LPCTSTR)m_ProxyAuth[_T("qop")], (LPCTSTR)m_ProxyAuth[_T("uri")],
					   (LPCTSTR)m_ProxyAuth[_T("nc")], (LPCTSTR)m_ProxyAuth[_T("cnonce")]);
			mbs = tmp; CExtSocket::Send((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
			DEBUGLOG("ProxyFunc PRST_HTTP_DIGEST %s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();

			SendFlash(3);
			if ( !m_SSL_keep )
				SSLClose();					// XXXXXXXX BUG???
			break;

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
			mbs = m_ProxyUser; buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength() + 1);
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = PRST_SOCKS4_READMSG;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS4_READMSG:
			if ( !ProxyReadBuff(8) )
				return TRUE;
			m_ProxyStatus = PRST_SOCKS4_CHECKMSG;
			break;
		case PRST_SOCKS4_CHECKMSG:
			if ( *(m_ProxyBuff.GetPos(1)) == 90 ) {
				m_ProxyStatus = PRST_NONE;
				OnPreConnect();
			} else
				m_ProxyStatus = PRST_SOCKS4_ENDOF;
			break;
		case PRST_SOCKS4_ENDOF:
			m_pDocument->m_ErrorPrompt = _T("SOCKS4 Proxy Conection Error");
			PostMessage(m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
			m_ProxyStatus = PRST_NONE;
			return TRUE;

		case PRST_SOCKS5_START:	// SOCKS5
			buf.Clear();
			buf.Put8Bit(5);
			buf.Put8Bit(3);
			buf.Put8Bit(0x00);		// SOCKS5_AUTH_NOAUTH
			buf.Put8Bit(0x02);		// SOCKS5_AUTH_USERPASS
			buf.Put8Bit(0x03);		// SOCKS5_AUTH_CHAP
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = PRST_SOCKS5_READMSG;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_READMSG:
			if ( !ProxyReadBuff(2) )
				return TRUE;
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
			mbs = m_ProxyUser;
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			mbs = m_ProxyPass;
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());

			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = PRST_SOCKS5_READAUTH;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_READAUTH:
			if ( !ProxyReadBuff(2) )
				return TRUE;
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
			mbs = m_ProxyHost;
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			buf.Put8Bit(m_ProxyPort >> 8);
			buf.Put8Bit(m_ProxyPort);
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = PRST_SOCKS5_READCONNECT;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_READCONNECT:
			if ( !ProxyReadBuff(4) )
				return TRUE;
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
				return TRUE;
			m_ProxyLength = 4 + 1 + *(m_ProxyBuff.GetPos(4)) + 2;
			m_ProxyStatus = PRST_SOCKS5_CONNECT;
			break;
		case PRST_SOCKS5_CONNECT:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return TRUE;
			m_ProxyStatus = PRST_NONE;
			OnPreConnect();
			break;
		case PRST_SOCKS5_ERROR:	// SOCKS5_ERROR
			m_pDocument->m_ErrorPrompt = _T("SOCKS5 Proxy Conection Error");
			PostMessage(m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
			m_ProxyStatus = PRST_NONE;
			return TRUE;

		case PRST_SOCKS5_CHAPSTART:	// SOCKS5 CHAP
			buf.Clear();
			buf.Put8Bit(0x01);
			buf.Put8Bit(0x02);
			buf.Put8Bit(0x11);
			buf.Put8Bit(0x85);		// HMAC-MD5
			buf.Put8Bit(0x02);
			mbs = m_ProxyUser;
			buf.Put8Bit(mbs.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyConnect = FALSE;
			m_ProxyStatus = PRST_SOCKS5_CHAPMSG;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_CHAPMSG:
			if ( !ProxyReadBuff(2) )
				return TRUE;
			if ( *(m_ProxyBuff.GetPos(0)) != 0x01 || (m_ProxyNum = *(m_ProxyBuff.GetPos(1))) == 0x00 )
				m_ProxyStatus = PRST_SOCKS5_ERROR;
			else 
				m_ProxyStatus = PRST_SOCKS5_CHAPCODE;
			m_ProxyBuff.Clear();
			break;
		case PRST_SOCKS5_CHAPCODE:
			if ( !ProxyReadBuff(2) )
				return TRUE;
			m_ProxyCode   = *(m_ProxyBuff.GetPos(0));
			m_ProxyLength = *(m_ProxyBuff.GetPos(1));
			m_ProxyBuff.Clear();
			m_ProxyStatus = PRST_SOCKS5_CHAPSTAT;
			break;
		case PRST_SOCKS5_CHAPSTAT:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return TRUE;
			m_ProxyStatus = PRST_SOCKS5_CHAPCHECK;
			switch(m_ProxyCode) {
			case 0x00:
				if ( *(m_ProxyBuff.GetPos(0)) == 0x00 )
					m_ProxyConnect = TRUE;
				else
					m_ProxyStatus = PRST_SOCKS5_ERROR;
				break;
			case 0x03:
				mbs = m_ProxyPass;
				n = HMAC_digest(EVP_md5(), (BYTE *)(LPCSTR)mbs, mbs.GetLength(), m_ProxyBuff.GetPtr(), m_ProxyBuff.GetSize(), digest, sizeof(digest));

				buf.Clear();
				buf.Put8Bit(0x01);
				buf.Put8Bit(0x01);
				buf.Put8Bit(0x04);
				buf.Put8Bit(16);
				buf.Apend(digest, n);
				CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
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
		}
	}
	return FALSE;
}
void CExtSocket::OnPreConnect()
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoLeft == NULL || !m_pFifoLeft->IsOpen() )
		return;

	m_Fd = m_pFifoLeft->m_hSocket;
	
	if ( m_SSL_mode != 0 && !SSLConnect() ) {
		OnError(WSAECONNREFUSED);
		return;
	}

	if ( m_pFifoProxy != NULL && m_ProxyStatus != PRST_NONE && ProxyFunc() )	// Proxy continue
		return;

	OnConnect();
#else
	if ( m_SSL_mode != 0 && !SSLConnect() ) {
		OnError(WSAECONNREFUSED);
		return;
	}

	if ( m_ProxyStatus != PRST_NONE ) {
		ProxyFunc();
		return;
	}

	OnConnect();
#endif
}
void CExtSocket::OnPreClose()
{
#ifdef	USE_FIFOBUF
	OnClose();
#else
	if ( CExtSocket::GetRecvSize() == 0 )
		OnClose();
	else
		m_OnRecvFlag |= RECV_DOCLOSE;
#endif
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::OnGetHostByName(LPCTSTR pHostName)
{
}
void CExtSocket::OnAsyncHostByName(int mode, LPCTSTR pHostName)
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::OnAsyncHostByName\n");
#else
	switch(mode) {
	case ASYNC_GETADDR:
		OnGetHostByName(pHostName);
		break;
	case ASYNC_CREATE:
		if ( !Create(pHostName, m_RealHostPort, m_RealRemoteAddr, m_RealSocketType) )
			OnError(WSAGetLastError());
		break;
	case ASYNC_OPEN:
		if ( !Open(pHostName, m_RealHostPort, m_RealRemotePort, m_RealSocketType) )
			OnError(WSAGetLastError());
		break;
	case ASYNC_CREATEINFO:
		if ( !Create(m_RealHostAddr, m_RealHostPort, m_RealRemoteAddr, m_RealSocketType, (void *)pHostName) )
			OnError(WSAGetLastError());
		break;
	case ASYNC_OPENINFO:
		if ( !Open(m_RealHostAddr, m_RealHostPort, m_RealRemotePort, m_RealSocketType, (void *)pHostName) )
			OnError(WSAGetLastError());
		break;
	}
#endif
}
void CExtSocket::OnError(int err)
{
#ifndef	USE_FIFOBUF
	if ( m_AddrInfoTop != NULL && OpenAddrInfo() )
		return;
#endif
	Close();

	if ( m_pDocument == NULL )
		return;

	m_pDocument->OnSocketError(err);
}
void CExtSocket::OnClose()
{
	Close();

	if ( m_pDocument == NULL )
		return;

	m_pDocument->OnSocketClose();
}
void CExtSocket::OnConnect()
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoLeft == NULL || !m_pFifoLeft->IsOpen() )
		return;

	GetMainWnd()->SetAsyncSelect(m_Fd, this, 0);

	if ( m_pFifoLeft->m_Type == FIFO_TYPE_SOCKET ) {
		if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) ) {
			BOOL opval = TRUE;
			m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, SO_KEEPALIVE, 0, (void *)&opval);
		}

		if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLNODELAY) ) {
			BOOL opval = TRUE;
			m_pFifoLeft->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, TCP_NODELAY, 0, (void *)&opval);
		}
	}
#else
	if ( m_AddrInfoTop != NULL ) {
		FreeAddrInfo(m_AddrInfoTop);
		m_AddrInfoTop = NULL;
	}

	if ( m_Fd != (-1) ) {
		m_SocketEvent &= ~FD_CONNECT;
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);

		if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) ) {
			BOOL opval = TRUE;
			if ( ::setsockopt(m_Fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)(&opval), sizeof(opval)) ) {
				OnError(WSAGetLastError());
				return;
			}
		}

		if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLNODELAY) ) {
			BOOL opval = TRUE;
			if ( ::setsockopt(m_Fd, SOL_SOCKET, TCP_NODELAY, (const char *)(&opval), sizeof(opval)) ) {
				OnError(WSAGetLastError());
				return;
			}
		}
	}
#endif

	if ( m_bCallConnect ) {
		m_bConnect = TRUE;
		m_pDocument->OnSocketConnect();
	}
}
void CExtSocket::OnAccept(SOCKET hand)
{
}
void CExtSocket::OnSend()
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::OnSend\n");
#else
	int n;

	while ( m_SendHead != NULL && m_SendLimit.timer == 0 ) {
		if ( m_SSL_mode != 0 )
			n = (m_SSL_pSock != NULL ? SSL_write(m_SSL_pSock, m_SendHead->GetPtr(), m_SendHead->GetSize()): 0);
		else
			n = (m_Fd != (-1) ? ::send(m_Fd, (char *)m_SendHead->GetPtr(), m_SendHead->GetSize(), m_SendHead->m_Type) : 0);

		//TRACE("OnSend %s %d\r\n", m_SSL_mode ? "SSL" : "", n);

		if ( n <= 0 ) {
			if ( (m_SocketEvent & FD_WRITE) == 0 ) {
				m_SocketEvent |= FD_WRITE;
				WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
			}
			return;
		}

		if ( m_SendHead->Consume(n) <= 0 )
			m_SendHead = RemoveHead(m_SendHead);

		m_SendSize -= n;

		if ( m_TransmitLimit > 0 )
			CheckLimit(n, &m_SendLimit);
	}

	if ( m_Fd != (-1) && (m_SocketEvent & FD_WRITE) != 0 ) {
		m_SocketEvent &= ~FD_WRITE;
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
		TRACE("OnSend SocketEvent FD_WRITE Off %04x\n", m_SocketEvent);
	}

	if ( m_SendHead == NULL )
		OnSendEmpty();
#endif
}
BOOL CExtSocket::ReceiveFlowCheck()
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::ReceiveFlowCheck\n");
	return FALSE;
#else
	int n;

	if ( m_Fd == (-1) )
		return FALSE;

	if ( (m_SocketEvent & (FD_READ | FD_OOB)) == 0 ) {
		if ( (n = GetRecvSize()) <= RECVMINSIZ && m_RecvLimit.timer == 0 ) {
			m_SocketEvent |= (FD_READ | FD_OOB);
			WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
			TRACE("OnReceive SocketEvent On %04x %d(%d:%d)\n", m_SocketEvent, n, m_RecvSize, m_RecvProcSize);
		}
	} else {
		if ( (n = GetRecvSize()) >= RECVMAXSIZ || m_RecvLimit.timer != 0 ) {
			m_SocketEvent &= ~(FD_READ | FD_OOB);
			WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
			TRACE("OnReceive SocketEvent Off %04x %d(%d:%d)\n", m_SocketEvent, n, m_RecvSize, m_RecvProcSize);
			return TRUE;
		}
	}

	return FALSE;
#endif
}
void CExtSocket::OnReceive(int nFlags)
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::OnReceive\n");
#else
	int n;
	CSockBuffer *sp = NULL;

	if ( m_Fd == (-1) )
		return;

	if ( ReceiveFlowCheck() )
		return;

	sp = AllocBuffer();
	sp->m_Type = nFlags;
	sp->AddAlloc(RECVBUFSIZ);

	if ( m_SSL_mode && m_SSL_pSock != NULL ) {
		if ( (n = SSL_read(m_SSL_pSock, (void *)sp->GetLast(), RECVBUFSIZ)) < 0 && SSL_get_error(m_SSL_pSock, n) != SSL_ERROR_WANT_READ ) {
			FreeBuffer(sp);
			OnError(WSAENOTCONN);
			return;
		}
	} else
		n = ::recv(m_Fd, (char *)sp->GetLast(), RECVBUFSIZ, nFlags);

	//TRACE("OnReceive(%d) %s %d\r\n", bWinSock, m_SSL_mode ? "SSL" : "", n);

	if ( n <= 0 )
		FreeBuffer(sp);
	else {
		sp->AddSize(n);
		m_RecvSema.Lock();
		m_RecvHead = AddTail(sp, m_RecvHead);
		m_RecvSize += sp->GetSize();
		m_RecvSema.Unlock();

		if ( m_TransmitLimit > 0 )
			CheckLimit(n, &m_RecvLimit);
	}
#endif
}

#ifndef	USE_FIFOBUF
BOOL CExtSocket::ReceiveCall()
{
	int n = 0;

	if ( m_ProxyStatus != PRST_NONE && ProxyFunc() )
		return TRUE;

	if ( m_RecvHead == NULL )
		return FALSE;

	if ( (m_OnRecvFlag & RECV_ACTIVE) != 0 )
		return TRUE;

	m_OnRecvFlag |= RECV_ACTIVE;

	//TRACE("ReceiveCall %d(%d:%d)\n", m_RecvHead->GetSize(), m_RecvSize - m_RecvProcSize, m_RecvProcSize);

	OnReceiveCallBack(m_RecvHead->GetPtr(), m_RecvHead->GetSize(), m_RecvHead->m_Type);

	n = m_RecvHead->m_Type;

	m_RecvSema.Lock();
	m_RecvSize -= m_RecvHead->GetSize();
	m_RecvHead = RemoveHead(m_RecvHead);
	m_RecvSema.Unlock();

	ReceiveFlowCheck();
	
	m_OnRecvFlag &= ~RECV_ACTIVE;

	return TRUE;
}
#endif

void CExtSocket::OnReceiveCallBack(void *lpBuf, int nBufLen, int nFlags)
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoMid != NULL )
		m_pFifoMid->Write(FIFO_EXTOUT, (BYTE *)lpBuf, nBufLen);
#else
	int n;
	CSockBuffer *sp;

	//TRACE("OnReceiveCallBack %08x %d (%d)\n", this, nBufLen, m_RecvSize);

	if ( nBufLen <= 0 )
		return;

	if ( (m_RecvSyncMode & SYNC_ACTIVE) == 0 && m_ProcHead == NULL ) {
		if ( (n = OnReceiveProcBack((LPBYTE)lpBuf, nBufLen, nFlags)) >= nBufLen ) {
			if ( m_RecvProcSize <= RECVMINSIZ )
				OnRecvEmpty();
			else
				ReceiveFlowCheck();
			return;
		}
		lpBuf = (LPBYTE)lpBuf + n;
		nBufLen -= n;
	}

	m_RecvSema.Lock();
	if ( m_ProcHead == NULL || (sp = m_ProcHead->m_Left)->m_Type != nFlags || sp->GetSize() >= RECVBUFSIZ )
		sp = AllocBuffer();
	sp->Apend((LPBYTE)lpBuf, nBufLen, nFlags);
	if ( m_ProcHead == NULL || sp != m_ProcHead->m_Left )
		m_ProcHead = AddTail(sp, m_ProcHead);
	m_RecvProcSize += nBufLen;
	if ( m_pRecvEvent != NULL && (m_RecvSyncMode & SYNC_EVENT) != 0 )
		m_pRecvEvent->SetEvent();
	m_RecvSyncMode &= ~SYNC_EVENT;
	m_RecvSema.Unlock();
#endif
}

#ifndef	USE_FIFOBUF
BOOL CExtSocket::ReceiveProc()
{
	int n, i;
	CSockBuffer *sp;
	BOOL ret = FALSE;

	//TRACE("ReceiveProc %08x (%d)\n", this, m_RecvSize);

	if ( (m_RecvSyncMode & SYNC_ACTIVE) != 0 )
		return FALSE;

	if ( (m_OnRecvFlag & RECV_INPROC) != 0 )
		return TRUE;

	m_OnRecvFlag |= RECV_INPROC;

	m_RecvSema.Lock();
	if ( (sp = m_ProcHead) != NULL ) {
		m_ProcHead = DetachHead(m_ProcHead);
		i = sp->m_Type;
		m_RecvSema.Unlock();
		if ( (n = OnReceiveProcBack(sp->GetPtr(), sp->GetSize(), sp->m_Type)) > 0 ) {
			m_RecvSema.Lock();
			m_RecvProcSize -= n;
			if ( sp->Consume(n) > 0 )
				m_ProcHead = AddHead(sp, m_ProcHead);
			else
				delete sp;
			m_RecvSema.Unlock();
		} else {
			m_RecvSema.Lock();
			m_ProcHead = AddHead(sp, m_ProcHead);
			m_RecvSema.Unlock();
		}
		if ( m_RecvProcSize <= RECVMINSIZ )
			OnRecvEmpty();
		else
			ReceiveFlowCheck();
		ret = TRUE;
	} else
		m_RecvSema.Unlock();

	m_OnRecvFlag &= ~RECV_INPROC;

	return ret;
}
#endif

int CExtSocket::OnReceiveProcBack(void *lpBuf, int nBufLen, int nFlags)
{
#ifdef	USE_FIFOBUF
	if ( m_pDocument == NULL )
		return 0;

	return m_pDocument->OnSocketReceive((LPBYTE)lpBuf, nBufLen, nFlags);
#else
	//TRACE("OnReceiveProcBack %d/%d\n", nBufLen, m_RecvSize);
	
	if ( (m_OnRecvFlag & RECV_PROCBACK) != 0 )
		return 0;

	m_OnRecvFlag |= RECV_PROCBACK;

	if ( m_pDocument != NULL )
		nBufLen = m_pDocument->OnSocketReceive((LPBYTE)lpBuf, nBufLen, nFlags);

	m_OnRecvFlag &= ~RECV_PROCBACK;

	return nBufLen;
#endif
}
int CExtSocket::OnIdle()
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::OnIdle\n");
	return FALSE;
#else
	if ( m_DestroyFlag ) {
		Destroy();
		return TRUE;
	}

	if ( m_bUseProc ) {
		if ( ReceiveProc() )
			return TRUE;
		m_bUseProc = FALSE;
	}

	if ( ReceiveCall() ) {
		m_bUseProc = TRUE;
		return TRUE;
	}

	if ( ReceiveProc() ) {
		m_bUseProc = TRUE;
		return TRUE;
	}

	if ( (m_OnRecvFlag & RECV_DOCLOSE) != 0 && GetRecvSize() == 0 ) {
		m_OnRecvFlag &= ~RECV_DOCLOSE;
		OnClose();
		return TRUE;
	}

	return FALSE;
#endif
}
void CExtSocket::OnTimer(UINT_PTR nIDEvent)
{
#ifdef	USE_FIFOBUF
	TRACE("CExtSocket::OnTimer\n");
#else
	if ( m_SendLimit.timer == (int)nIDEvent ) {
		m_SendLimit.timer = 0;
		OnSend();
	} else if ( m_RecvLimit.timer == (int)nIDEvent ) {
		m_RecvLimit.timer = 0;
		OnReceive(0);
	} else
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this, (int)nIDEvent);
#endif
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::IOCtl(long lCommand, DWORD *lpArgument)
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET )
		return FALSE;

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_IOCtl, lCommand, 0, (void *)lpArgument);
#else
	if ( m_Fd == (-1) || ::ioctlsocket(m_Fd, lCommand, lpArgument) != 0 )
		return FALSE;
	return TRUE;
#endif
}
BOOL CExtSocket::SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel)
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET || nLevel != SOL_SOCKET || nOptionLen != sizeof(BOOL) )
		return FALSE;

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SETSOCKOPT, nOptionName, 0, (void *)lpOptionValue);
#else
	if ( m_Fd == (-1) || ::setsockopt(m_Fd, nLevel, nOptionName, (const char *)lpOptionValue, nOptionLen) != 0 )
		return FALSE;
	return TRUE;
#endif
}
BOOL CExtSocket::GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel)
{
#ifdef	USE_FIFOBUF
	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET || nLevel != SOL_SOCKET || *lpOptionLen != sizeof(BOOL) )
		return FALSE;

	return m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_GETSOCKOPT, nOptionName, 0, (void *)lpOptionValue);
#else
	if ( m_Fd == (-1) || ::getsockopt(m_Fd, nLevel, nOptionName, (char *)lpOptionValue, lpOptionLen) != 0 )
		return FALSE;
	return TRUE;
#endif
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
	SOCKET Fd;
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

		if ( (Fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) )
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

int CExtSocket::SSLConnect()
{
	DWORD val;
	const SSL_METHOD *method;
	long result = 0;
	X509 *pX509, *cert = NULL;
	const char *errstr;
	CString tmp, subject, finger, dig;
	CIdKey key;
	BOOL rf = FALSE;
	X509_STORE *pStore;
	X509_NAME *pName;
	char name_buf[1024];
	STACK_OF(X509) *pStack;
	EVP_PKEY *pkey;

#ifdef	USE_FIFOBUF
	void *pauseParam[2];
	CEvent waitEvent;

	pauseParam[0] = (void *)(HANDLE)waitEvent;
	pauseParam[1] = NULL;

	if ( m_pFifoLeft == NULL || m_pFifoLeft->m_Type != FIFO_TYPE_SOCKET )
		return FALSE;

	if ( !m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SSLCONNECT, 0, 0, (void *)pauseParam) )
		return FALSE;

#else
	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), 0, 0);
#endif

	val = 0;
	::ioctlsocket(m_Fd, FIONBIO, &val);

	GetApp()->SSL_Init();

	method = TLS_client_method();

	if ( (m_SSL_pCtx = SSL_CTX_new((SSL_METHOD *)method)) == NULL )
		goto ERRENDOF;

	if ( (m_SSL_pSock = SSL_new(m_SSL_pCtx)) == NULL )
		goto ERRENDOF;

	SSL_CTX_set_mode(m_SSL_pCtx, SSL_MODE_AUTO_RETRY);
	SSL_set_fd(m_SSL_pSock, (int)m_Fd);

	if ( (pStore = X509_STORE_new()) != NULL ) {
		// Windows Certificate Store ����ROOT�ؖ�����OpenSSL�ɓo�^
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

	if ( SSL_connect(m_SSL_pSock) < 0 )
		goto ERRENDOF;

	cert   = SSL_get_peer_certificate(m_SSL_pSock);
	result = SSL_get_verify_result(m_SSL_pSock);

	m_SSL_Msg.Empty();

	if ( (pStack = SSL_get_peer_cert_chain(m_SSL_pSock)) != NULL ) {
		// �ؖ����̃`�F�C�����X�e�[�^�X�Ɏc��
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

				if ( (pkey = X509_get_pubkey(pX509)) != NULL && key.SetEvpPkey(pkey) ) {
					key.FingerPrint(finger, SSHFP_DIGEST_SHA256, SSHFP_FORMAT_SIMPLE);
					tmp.Format(_T("PublicKey: %s %dbits %s\r\n"), key.GetName(), key.GetSize(), (LPCTSTR)finger);
					m_SSL_Msg += tmp;
				}
			}
		}
	}

	if ( cert == NULL || result != X509_V_OK ) {
		if ( cert != NULL && (pName = X509_get_subject_name(cert)) != NULL && X509_NAME_oneline(pName, name_buf, 1024) != NULL )
			subject = MbsToTstr(name_buf);
		else
			subject = _T("unkown");

		if ( cert != NULL && (pkey = X509_get_pubkey(cert)) != NULL && key.SetEvpPkey(pkey) ) {
			key.WritePublicKey(dig);
			tmp = AfxGetApp()->GetProfileString(_T("Certificate"), subject, _T(""));
			if ( !tmp.IsEmpty() && tmp.Compare(dig) == 0 )
				rf = TRUE;
		}

		if ( rf == FALSE ) {
			errstr = X509_verify_cert_error_string(result);
			tmp.Format(CStringLoad(IDS_CERTTRASTREQ), (m_SSL_Msg.IsEmpty() ? _T("Subject: unkown") : m_SSL_Msg), MbsToTstr(errstr));

			if ( AfxMessageBox(tmp, MB_ICONWARNING | MB_YESNO) != IDYES )
				goto ERRENDOF;

			if ( cert != NULL && !dig.IsEmpty() )
				AfxGetApp()->WriteProfileString(_T("Certificate"), subject, dig);
		}
	}

	if ( cert != NULL )
		X509_free(cert);

	val = 1;
	::ioctlsocket(m_Fd, FIONBIO, &val);

#ifdef	USE_FIFOBUF
	pauseParam[1] = (void *)m_SSL_pSock;
	waitEvent.SetEvent();
#else
	m_SocketEvent |= FD_READ;
	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
#endif

	return TRUE;

ERRENDOF:
#ifdef	USE_FIFOBUF
	pauseParam[1] = NULL;
	waitEvent.SetEvent();
#endif

	if ( cert != NULL )
		X509_free(cert);
	SSLClose();
	return FALSE;
}
void CExtSocket::SSLClose()
{
#ifdef	USE_FIFOBUF
	void *pauseParam[2];

	pauseParam[0] = NULL;
	pauseParam[1] = NULL;

	if ( m_SSL_pSock != NULL && m_pFifoLeft != NULL && m_pFifoLeft->m_Type == FIFO_TYPE_SOCKET )
		m_pFifoLeft->SendCmdWait(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SSLCONNECT, 0, 0, pauseParam);
#endif

	if ( m_SSL_pSock != NULL )
		SSL_free(m_SSL_pSock);

	if ( m_SSL_pCtx != NULL )
		SSL_CTX_free(m_SSL_pCtx);

	m_SSL_mode  = 0;
	m_SSL_pCtx  = NULL;
	m_SSL_pSock = NULL;
}
