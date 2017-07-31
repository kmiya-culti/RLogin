// ExtSocket.cpp: CExtSocket クラスのインプリメンテーション
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
	delete m_Data;
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

	delete m_Data;
	m_Data = tmp;
}
void CSockBuffer::Apend(LPBYTE buff, int len, int type)
{
	AddAlloc(len);
	memcpy(m_Data + m_Len, buff, len);
	m_Len += len;
	m_Type = type;
}

//////////////////////////////////////////////////////////////////////
// CExtSocket

static const int EventMask[2] = { FD_READ, FD_OOB };

CExtSocket::CExtSocket(class CRLoginDoc *pDoc)
{
	m_pDocument = pDoc;
	m_Type = ESCT_DIRECT;
	m_Fd = (-1);
	m_ListenMax = 0;
	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;
	m_RecvSyncMode = SYNC_NONE;
	m_RecvBufSize = (4 * 1024);
	m_OnRecvFlag = m_OnSendFlag = 0;
	m_DestroyFlag = FALSE;
	m_pRecvEvent = NULL;
	m_FreeHead = m_ProcHead = m_RecvHead = m_SendHead = NULL;
	m_RecvSize = m_RecvProcSize = 0;
	m_SendSize = 0;
	m_ProxyStatus = PRST_NONE;
	m_SSL_mode  = 0;
	m_SSL_pCtx  = NULL;
	m_SSL_pSock = NULL;
	m_bConnect = FALSE;
#ifndef	NOIPV6
	m_AddrInfoTop = NULL;
#endif
}

CExtSocket::~CExtSocket()
{
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
}

#ifndef DEBUGLOG
void CExtSocket::DEBUGLOG(LPCSTR str, ...)
{
	LPCSTR p;
	CStringA tmp, hex, work;
	va_list arg;

	va_start(arg, str);
	tmp.FormatV(str, arg);
	va_end(arg);

	for ( p = tmp ; *p != '\0' ; p++ ) {
		if ( *p != '\r' && *p != '\n' && (*p < ' ' || *p > '\x7E') ) {
			hex.Format("?%02x", *p & 255);
			work += hex;
		} else
			work += *p;
	}

	OnReciveProcBack((void *)(LPCSTR)work, work.GetLength(), 0);
	TRACE("%s", work);
}
#endif

void CExtSocket::Destroy()
{
	Close();
	if ( (m_OnRecvFlag & RECV_ACTIVE) != 0 ) {
		if ( !m_DestroyFlag )
			((CRLoginApp *)AfxGetApp())->SetSocketIdle(this);
		m_DestroyFlag = TRUE;
	} else {
		if ( m_DestroyFlag )
			((CRLoginApp *)AfxGetApp())->DelSocketIdle(this);
		delete this;
	}
}
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
#ifndef	NOIPV6
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
#endif

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::AsyncGetHostByName(LPCTSTR pHostName)
{
	CString host;
	PunyCodeAdress(pHostName, host);
	return GetMainWnd()->SetAsyncHostAddr(ASYNC_GETADDR, host, this);
}
BOOL CExtSocket::AsyncCreate(LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress, int nSocketType)
{
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

#ifdef	NOIPV6
	return GetMainWnd()->SetAsyncHostAddr(ASYNC_CREATE, m_RealHostAddr, this);
#else
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
	if ( lpszHostAddress == NULL || nHostPort == (-1) )
		return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);

	//m_RealHostAddr   = lpszHostAddress;
	PunyCodeAdress(lpszHostAddress, m_RealHostAddr);
	m_RealHostPort   = nHostPort;
	m_RealRemotePort = nSocketPort;
	m_RealSocketType = nSocketType;

#ifdef	NOIPV6
	return GetMainWnd()->SetAsyncHostAddr(ASYNC_OPEN, m_RealHostAddr, this);
#else
	ADDRINFOT hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = GetFamily();
	hints.ai_socktype = nSocketType;

	return GetMainWnd()->SetAsyncAddrInfo(ASYNC_OPENINFO, m_RealHostAddr, m_RealHostPort, &hints, this);
#endif
}
BOOL CExtSocket::ProxyOpen(int mode, LPCTSTR ProxyHost, UINT ProxyPort, LPCTSTR ProxyUser, LPCTSTR ProxyPass, LPCTSTR RealHost, UINT RealPort)
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
	case 0: m_SSL_mode = 0; break;	// Non
	case 1: m_SSL_mode = 1; break;	// SSLv2
	case 2: m_SSL_mode = 2; break;	// SSLv3
	case 3: m_SSL_mode = 3; break;	// TLSv1
#if	OPENSSL_VERSION_NUMBER >= 0x10001000L
	case 4: m_SSL_mode = 4; break;	// TLSv1.1
	case 5: m_SSL_mode = 5; break;	// TLSv1.2
#endif
	}

	//m_ProxyHost = RealHost;
	PunyCodeAdress(RealHost, m_ProxyHost);
	m_ProxyPort = RealPort;
	m_ProxyUser = ProxyUser;
	m_ProxyPass = ProxyPass;

	if ( m_ProxyStatus == PRST_NONE ) {
		m_RealHostAddr   = m_ProxyHost;
		m_RealHostPort   = m_ProxyPort;
	}

#ifdef	NOIPV6
	return GetMainWnd()->SetAsyncHostAddr(ASYNC_OPEN, m_RealHostAddr, this);
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
	m_SocketEvent = FD_ACCEPT | FD_CLOSE;

#ifdef	NOIPV6
	int opval;
    struct hostent *hp;
    struct sockaddr_in in;
	CString host;

	if ( (m_Fd = ::socket(AF_INET, nSocketType, 0)) == (-1) )
		return FALSE;

	if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) )
		goto ERRRET2;

	opval = 1;
	if ( ::setsockopt(m_Fd, SOL_SOCKET, SO_REUSEADDR, (const char *)(&opval), sizeof(opval)) != 0 )
		goto ERRRET1;

    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = INADDR_ANY;
    in.sin_port = htons(nHostPort);

	if ( lpszRemoteAddress != NULL && (hp = ::gethostbyname(TstrToMbs(lpszRemoteAddress))) != NULL )
		in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

    if ( bind(m_Fd, (struct sockaddr *)&in, sizeof(in)) < 0 || listen(m_Fd, 128) == SOCKET_ERROR )
		goto ERRRET1;

	return TRUE;

ERRRET1:
	GetMainWnd()->DelAsyncSelect(m_Fd, this);
ERRRET2:
	::closesocket(m_Fd);
	m_Fd = (-1);
	return FALSE;
#else
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

#ifndef	NOIPV6
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
			GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, FD_CONNECT);

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
			GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, FD_CONNECT);

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
	while ( m_ProcHead != NULL )
		m_ProcHead = RemoveHead(m_ProcHead);

	while ( m_RecvHead != NULL )
		m_RecvHead = RemoveHead(m_RecvHead);

	while ( m_SendHead != NULL )
		m_SendHead = RemoveHead(m_SendHead);

	m_RecvSize = m_RecvProcSize = 0;
	m_SendSize = 0;

	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;

	if ( m_SSL_mode != 0 )
		m_SocketEvent &= ~FD_READ;

#ifdef	NOIPV6
	DWORD val;
    struct hostent *hp;
    struct sockaddr_in in;
	CString host;

	if ( (m_Fd = ::socket(AF_INET, nSocketType, 0)) == (-1) )
		return FALSE;

	if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) )
		goto ERRRET2;

	if ( nSocketPort != 0 ) {
	    memset(&in, 0, sizeof(in));
	    in.sin_family = AF_INET;
	    in.sin_addr.s_addr = INADDR_ANY;
	    in.sin_port = htons(nSocketPort);
		if ( ::bind(m_Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR )
			goto ERRRET1;
	}

	PunyCodeAdress(lpszHostAddress, host);

	if ( (hp = ::gethostbyname(TstrToMbs(host))) == NULL )
		goto ERRRET1;

	in.sin_family = AF_INET;
	in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	in.sin_port = htons(nHostPort);

	val = 1;
	if ( ::ioctlsocket(m_Fd, FIONBIO, &val) != 0 )
		goto ERRRET1;

    if ( ::connect(m_Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR ) {
		if ( WSAGetLastError() != WSAEWOULDBLOCK )
			goto ERRRET1;
	} else
		GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, FD_CONNECT);

	return TRUE;

ERRRET1:
	GetMainWnd()->DelAsyncSelect(m_Fd, this);
ERRRET2:
	::closesocket(m_Fd);
	m_Fd = (-1);
	return FALSE;

#else
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
	SSLClose();

#ifndef	NOIPV6
	if ( m_AddrInfoTop != NULL ) {
		FreeAddrInfo(m_AddrInfoTop);
		m_AddrInfoTop = NULL;
	}
#endif
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
	m_bConnect = FALSE;
}
int CExtSocket::Accept(class CExtSocket *pSock, SOCKET hand)
{
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
}
int CExtSocket::Attach(class CRLoginDoc *pDoc, SOCKET fd)
{
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
}
int CExtSocket::Recive(void* lpBuf, int nBufLen, int nFlags)
{
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
		m_RecvSize -= n;
		m_RecvProcSize -= n;
		if ( m_RecvProcSize < RECVMINSIZ )
			m_RecvSyncMode |= SYNC_EMPTY;
		len += n;
		buf += n;
		nBufLen -= n;
	}
	m_RecvSema.Unlock();
	if ( m_Fd != (-1) && (m_SocketEvent & FD_READ) == 0 && (m_RecvSyncMode & SYNC_EMPTY) != 0 )
		GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, FD_READ);
	return len;
}
int CExtSocket::SyncRecive(void* lpBuf, int nBufLen, int nSec, BOOL *pAbort)
{
	int n;
	int len = 0;
	LPBYTE buf = (LPBYTE)lpBuf;
	time_t st, nt;

	time(&st);
	while ( !*pAbort ) {
		m_RecvSema.Lock();
		while ( nBufLen > 0 && m_ProcHead != NULL ) {
			if ( m_ProcHead->m_Type != 0 ) {
				m_RecvSize -= m_ProcHead->GetSize();
				m_RecvProcSize -= m_ProcHead->GetSize();
				m_ProcHead = RemoveHead(m_ProcHead);
			} else {
				if ( (n = m_ProcHead->GetSize()) > nBufLen )
					n = nBufLen;
				memcpy(buf, m_ProcHead->GetPtr(), n);
				if ( m_ProcHead->Consume(n) <= 0 )
					m_ProcHead = RemoveHead(m_ProcHead);
				m_RecvSize -= n;
				m_RecvProcSize -= n;
				len += n;
				buf += n;
				nBufLen -= n;
			}
			if ( m_RecvProcSize < RECVMINSIZ )
				m_RecvSyncMode |= SYNC_EMPTY;
		}

		if ( len > 0 || nBufLen <= 0 ) {
			m_RecvSema.Unlock();
			break;
		} else if ( m_pRecvEvent != NULL ) {
			m_pRecvEvent->ResetEvent();
			m_RecvSyncMode |= SYNC_EVENT;
			m_RecvSema.Unlock();
			time(&nt);
			if ( (n = (int)(nt - st)) >= nSec )
				break;
//			TRACE("SyncRecive wait %d\n", nSec - n);
			WaitForSingleObject(m_pRecvEvent->m_hObject, (nSec - n) * 1000);
		} else {
			m_RecvSema.Unlock();
			time(&nt);
			if ( (n = (int)(nt - st)) >= nSec )
				break;
			Sleep(100);
		}
	}
	if ( m_Fd != (-1) && (m_SocketEvent & FD_READ) == 0 && (m_RecvSyncMode & SYNC_EMPTY) != 0 )
		GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, FD_READ);
	return len;
}
void CExtSocket::SyncReciveBack(void *lpBuf, int nBufLen)
{
	CSockBuffer *sp = AllocBuffer();
	m_RecvSema.Lock();
	sp->Apend((LPBYTE)lpBuf, nBufLen, 0);
	m_ProcHead = AddHead(sp, m_ProcHead);
	m_RecvSize += nBufLen;
	m_RecvProcSize += nBufLen;
	m_RecvSyncMode &= ~SYNC_EMPTY;
	m_RecvSema.Unlock();
}
void CExtSocket::SyncAbort()
{
	if ( m_pRecvEvent == NULL )
		return;
	m_RecvSema.Lock();
	m_pRecvEvent->SetEvent();
	m_RecvSema.Unlock();
}
void CExtSocket::SetRecvSyncMode(BOOL mode)
{
	if ( m_pRecvEvent == NULL )
		m_pRecvEvent = new CEvent(FALSE, TRUE);
	m_RecvSyncMode = (mode ? SYNC_ACTIVE : SYNC_NONE); 
}
void CExtSocket::SendFlash(int sec)
{
	clock_t st;

	for ( st = clock() + sec * CLOCKS_PER_SEC ; m_SendHead != NULL && st < clock() ; ) {
		OnSend();
		Sleep(100);
	}
}
int CExtSocket::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	int n;
	int len = nBufLen;
	CSockBuffer *sp;

	if ( nBufLen <= 0 )
		return 0;

	if ( m_SendHead == NULL ) {
		n = (m_SSL_mode ? SSL_write(m_SSL_pSock, lpBuf, nBufLen):
						  ::send(m_Fd, (char *)lpBuf, nBufLen, nFlags));
		if ( n > 0 ) {
			lpBuf = (LPBYTE)lpBuf + n;
			if ( (nBufLen -= n) <= 0 ) {
				m_OnSendFlag |= SEND_EMPTY;
				goto RETOF;
			}
		}
	}

	if ( m_SendHead == NULL || (sp = m_SendHead->m_Left)->m_Type != nFlags || sp->GetSize() > RECVBUFSIZ )
		sp = AllocBuffer();
	sp->Apend((LPBYTE)lpBuf, nBufLen, nFlags);
	if ( m_SendHead == NULL || sp != m_SendHead->m_Left )
		m_SendHead = AddTail(sp, m_SendHead);
	m_SendSize += nBufLen;

	//TRACE("Send Save %d\n", m_SendSize);
	
	if ( (m_SocketEvent & FD_WRITE) == 0 ) {
		m_SocketEvent |= FD_WRITE;
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
	}

RETOF:
	return len;
}
void CExtSocket::SendWindSize(int x, int y)
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
	return m_RecvSize;
}
int CExtSocket::GetRecvProcSize()
{
	return m_RecvProcSize;
}
int CExtSocket::GetSendSize()
{
	return m_SendSize;
}
void CExtSocket::OnRecvEmpty()
{
}
void CExtSocket::OnSendEmpty()
{
}
void CExtSocket::GetStatus(CString &str)
{
	CString tmp;
	CString name;
	int port;

	tmp.Format(_T("Socket Type: %d\r\n"), m_Type);
	str += tmp;
	tmp.Format(_T("Recive Reserved: %d Bytes\r\n"), m_RecvSize);
	str += tmp;
	tmp.Format(_T("Send Reserved: %d Bytes\r\n"), m_SendSize);
	str += tmp;

	if ( m_Fd != (-1) ) {
		GetSockName(m_Fd, name, &port);
		tmp.Format(_T("Connect %s#%d"), name, port);
		str += tmp;
		GetPeerName(m_Fd, name, &port);
		tmp.Format(_T(" - %s#%d\r\n"), name, port);
		str += tmp;
	}
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::ProxyReadLine()
{
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
}
BOOL CExtSocket::ProxyReadBuff(int len)
{
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
}
int CExtSocket::ProxyFunc()
{
	int n, i;
	CBuffer buf;
	CString tmp;
	CBuffer A1, A2;
	CStringA mbs;

	while ( m_ProxyStatus != PRST_NONE ) {
		switch(m_ProxyStatus) {
		case PRST_HTTP_START:		// HTTP
			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("Connection: keep-alive\r\n")\
					   _T("\r\n"),
					   m_ProxyHost, m_ProxyPort, m_RealHostAddr);
			mbs = tmp; CExtSocket::Send((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
			DEBUGLOG("%s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			break;
		case PRST_HTTP_READLINE:
			if ( !ProxyReadLine() )
				return TRUE;
			DEBUGLOG("%s\r\n", TstrToMbs(m_ProxyStr));
			if ( m_ProxyStr.IsEmpty() ) {
				m_ProxyStatus = PRST_HTTP_HEADCHECK;
				break;
			} else if ( (m_ProxyStr[0] == _T('\t') || m_ProxyStr[0] == _T(' ')) && (n = m_ProxyResult.GetSize()) > 0 )
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
			//DEBUGLOG("%s", (LPCSTR)m_ProxyBuff);
			m_ProxyStatus = PRST_HTTP_CODECHECK;
			break;
		case PRST_HTTP_CODECHECK:
			switch(m_ProxyCode) {
			case 200:
				SSLClose();
				m_ProxyStatus = PRST_NONE;
				OnPreConnect();
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
				GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
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
			tmp.Format(_T("%s:%s"), m_ProxyUser, m_ProxyPass);
			mbs = tmp; buf.Base64Encode((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
			tmp.Format(_T("CONNECT %s:%d HTTP/1.1\r\n")\
					   _T("Host: %s\r\n")\
					   _T("%sAuthorization: Basic %s\r\n")\
					   _T("\r\n"),
					   m_ProxyHost, m_ProxyPort, m_RealHostAddr,
					   (m_ProxyCode == 407 ? _T("Proxy-") : _T("")),
					   (LPCTSTR)buf);
			mbs = tmp; CExtSocket::Send((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
			DEBUGLOG("%s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();

			SendFlash(3);
			SSLClose();					// XXXXXXXX BUG???
			break;
		case PRST_HTTP_DIGEST:
			if (m_ProxyAuth.Find(_T("qop")) < 0 || m_ProxyAuth[_T("algorithm")].m_String.CompareNoCase(_T("md5")) != 0 || m_ProxyAuth.Find(_T("realm")) < 0 || m_ProxyAuth.Find(_T("nonce")) < 0 ) {
				m_pDocument->m_ErrorPrompt = _T("Authorization Paramter Error");
				GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
				m_ProxyStatus = PRST_NONE;
				return TRUE;
			}

			tmp.Format(_T("%s:%d"), m_ProxyHost, m_ProxyPort);
			m_ProxyAuth[_T("uri")] = tmp;
			m_ProxyAuth[_T("qop")] = _T("auth");
			tmp.Format(_T("%08d"), 1);
			m_ProxyAuth[_T("nc")] = tmp;
			srand((int)time(NULL));
			tmp.Format(_T("%04x%04x%04x%04x"), rand(), rand(), rand(), rand());
			m_ProxyAuth[_T("cnonce")] = tmp;

			tmp.Format(_T("%s:%s:%s"), m_ProxyUser, (LPCTSTR)m_ProxyAuth[_T("realm")], m_ProxyPass);
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
					   m_ProxyHost, m_ProxyPort, m_RealHostAddr,
					   (m_ProxyCode == 407 ? _T("Proxy-") : _T("")),
					   m_ProxyUser, (LPCTSTR)m_ProxyAuth[_T("realm")], (LPCTSTR)m_ProxyAuth[_T("nonce")],
					   (LPCTSTR)m_ProxyAuth[_T("algorithm")], (LPCTSTR)buf,
					   (LPCTSTR)m_ProxyAuth[_T("qop")], (LPCTSTR)m_ProxyAuth[_T("uri")],
					   (LPCTSTR)m_ProxyAuth[_T("nc")], (LPCTSTR)m_ProxyAuth[_T("cnonce")]);
			mbs = tmp; CExtSocket::Send((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
			DEBUGLOG("%s", mbs);
			m_ProxyStatus = PRST_HTTP_READLINE;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();

			SendFlash(3);
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
			GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
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
			GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
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
				{
					HMAC_CTX c;
					u_char hmac[EVP_MAX_MD_SIZE];

					mbs = m_ProxyPass;
					HMAC_Init(&c, (LPCSTR)mbs, mbs.GetLength(), EVP_md5());
					HMAC_Update(&c, m_ProxyBuff.GetPtr(), m_ProxyBuff.GetSize());
					HMAC_Final(&c, hmac, NULL);
					HMAC_cleanup(&c);

					buf.Clear();
					buf.Put8Bit(0x01);
					buf.Put8Bit(0x01);
					buf.Put8Bit(0x04);
					buf.Put8Bit(16);
					buf.Apend(hmac, 16);
					CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
				}
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
	if ( m_SSL_mode != 0 ) {
		if ( !SSLConnect() ) {
			OnError(WSAECONNREFUSED);
			return;
		}
	}

	if ( m_ProxyStatus != PRST_NONE ) {
		ProxyFunc();
		return;
	}

	OnConnect();
}
void CExtSocket::OnPreClose()
{
	if ( CExtSocket::GetRecvSize() == 0 )
		OnClose();
	else
		m_OnRecvFlag |= RECV_DOCLOSE;
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::OnGetHostByName(LPCTSTR pHostName)
{
}
void CExtSocket::OnAsyncHostByName(int mode, LPCTSTR pHostName)
{
	switch(mode) {
	case ASYNC_GETADDR:
		OnGetHostByName(pHostName);
		break;
	case ASYNC_CREATE:
		if ( !Create(pHostName, m_RealHostPort, m_RealRemoteAddr, m_RealSocketType) )
			OnError(WSAENOTCONN);
		break;
	case ASYNC_OPEN:
		if ( !Open(pHostName, m_RealHostPort, m_RealRemotePort, m_RealSocketType) )
			OnError(WSAENOTCONN);
		break;
	case ASYNC_CREATEINFO:
		if ( !Create(m_RealHostAddr, m_RealHostPort, m_RealRemoteAddr, m_RealSocketType, (void *)pHostName) )
			OnError(WSAENOTCONN);
		break;
	case ASYNC_OPENINFO:
		if ( !Open(m_RealHostAddr, m_RealHostPort, m_RealRemotePort, m_RealSocketType, (void *)pHostName) )
			OnError(WSAENOTCONN);
		break;
	}
}
void CExtSocket::OnError(int err)
{
#ifndef	NOIPV6
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
#ifndef	NOIPV6
	if ( m_AddrInfoTop != NULL ) {
		FreeAddrInfo(m_AddrInfoTop);
		m_AddrInfoTop = NULL;
	}
#endif

	if ( m_Fd != (-1) ) {
		m_SocketEvent &= ~FD_CONNECT;
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);

		if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) ) {
			BOOL opval = TRUE;
			::setsockopt(m_Fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)(&opval), sizeof(opval));
		}
	}

	m_bConnect = TRUE;

	if ( m_pDocument != NULL )
		m_pDocument->OnSocketConnect();
}
void CExtSocket::OnAccept(SOCKET hand)
{
}
void CExtSocket::OnSend()
{
	int n;

	while ( m_SendHead != NULL ) {
		n = (m_SSL_mode ? SSL_write(m_SSL_pSock, m_SendHead->GetPtr(), m_SendHead->GetSize()):
						  ::send(m_Fd, (char *)m_SendHead->GetPtr(), m_SendHead->GetSize(), m_SendHead->m_Type));

		//TRACE("OnSend %s %d\r\n", m_SSL_mode ? "SSL" : "", n);

		if ( n <= 0 ) {
			m_SocketEvent |= FD_WRITE;
			WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
			return;
		}
		m_SendSize -= n;
		if ( m_SendHead->Consume(n) <= 0 )
			m_SendHead = RemoveHead(m_SendHead);
	}
	m_SocketEvent &= ~FD_WRITE;
	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
	m_OnSendFlag |= SEND_EMPTY;
}
void CExtSocket::OnRecive(int nFlags)
{
	int n, i;
	CSockBuffer *sp;

	if ( (m_SocketEvent & EventMask[nFlags]) == 0 ) {
		if ( m_RecvSize >= RECVMINSIZ )
			return;
		m_SocketEvent |= (FD_READ | FD_OOB);
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
		TRACE("OnRecive SocketEvent On %04x (%d)\n", m_SocketEvent, m_RecvSize);
	}

	for ( i = 0 ; i < 2 ; i++ ) {
		sp = AllocBuffer();
		sp->m_Type = nFlags;
		sp->AddAlloc(RECVBUFSIZ);

		n = (m_SSL_mode ? SSL_read(m_SSL_pSock, (void *)sp->GetLast(), RECVBUFSIZ):
						  ::recv(m_Fd, (char *)sp->GetLast(), RECVBUFSIZ, nFlags));

		//DEBUGLOG("OnRecive %s %d\r\n", m_SSL_mode ? "SSL" : "", n);

		if ( n > 0 )
			sp->AddSize(n);
		else if ( n < 0 && m_SSL_mode && SSL_get_error(m_SSL_pSock, n) != SSL_ERROR_WANT_READ ) {
			FreeBuffer(sp);
			OnError(WSAENOTCONN);
			return;
		}

		if ( sp->GetSize() <= 0 ) {
			FreeBuffer(sp);
			break;
		}

		m_RecvSema.Lock();
		m_RecvHead = AddTail(sp, m_RecvHead);
		m_RecvSize += sp->GetSize();
		m_RecvSema.Unlock();

		if ( sp->GetSize() < RECVBUFSIZ )
			break;
	}

	if ( nFlags == 0 && (m_SocketEvent & EventMask[nFlags]) != 0 && m_RecvSize >= RECVMAXSIZ ) {
		m_SocketEvent &= ~EventMask[nFlags];
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
		TRACE("OnRecive SocketEvent Off %04x (%d)\n", m_SocketEvent, m_RecvSize);
	}

	ReciveCall();
}
int CExtSocket::ReciveCall()
{
//	TRACE("ReciveCall %08x (%d)\n", this, m_RecvSize);

	if ( m_RecvHead != NULL && m_ProxyStatus != PRST_NONE && ProxyFunc() )
		return m_RecvSize;

	if ( m_RecvHead == NULL )
		return 0;

	if ( (m_OnRecvFlag & RECV_ACTIVE) != 0 )
		return 0;

	m_OnRecvFlag |= RECV_ACTIVE;

	OnReciveCallBack(m_RecvHead->GetPtr(), m_RecvHead->GetSize(), m_RecvHead->m_Type);

	int n = m_RecvHead->m_Type;

	m_RecvSema.Lock();
	m_RecvSize -= m_RecvHead->GetSize();
	m_RecvHead = RemoveHead(m_RecvHead);
	m_RecvSema.Unlock();

	if ( (m_SocketEvent & EventMask[n]) == 0 && m_RecvSize < RECVMINSIZ )
		OnRecive(n);

	m_OnRecvFlag &= ~RECV_ACTIVE;

	return m_RecvSize;
}
void CExtSocket::OnReciveCallBack(void *lpBuf, int nBufLen, int nFlags)
{
	int n;
	CSockBuffer *sp;

//	TRACE("OnReciveCallBack %08x %d (%d)\n", this, nBufLen, m_RecvSize);

	if ( nBufLen <= 0 )
		return;

	if ( (m_RecvSyncMode & SYNC_ACTIVE) == 0 && m_ProcHead == NULL ) {
		if ( (n = OnReciveProcBack((LPBYTE)lpBuf, nBufLen, nFlags)) >= nBufLen ) {
//			OnRecvEmpty();
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
	m_RecvSize += nBufLen;
	m_RecvProcSize += nBufLen;
	if ( m_pRecvEvent != NULL && (m_RecvSyncMode & SYNC_EVENT) != 0 )
		m_pRecvEvent->SetEvent();
	m_RecvSyncMode &= ~(SYNC_EMPTY | SYNC_EVENT);
	m_RecvSema.Unlock();

	if ( m_Fd != (-1) && nFlags == 0 && (m_SocketEvent & EventMask[nFlags]) != 0 && m_RecvSize >= RECVMAXSIZ ) {
		m_SocketEvent &= ~EventMask[nFlags];
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
		TRACE("OnReciveCallBack SocketEvent Off %04x (%d)\n", m_SocketEvent, m_RecvSize);
	}
}
int CExtSocket::ReciveProc()
{
	int n, i;
	CSockBuffer *sp;

//	TRACE("ReciveProc %08x (%d)\n", this, m_RecvSize);

	if ( (m_OnRecvFlag & RECV_INPROC) != 0 )
		return FALSE;

	m_OnRecvFlag |= RECV_INPROC;

	if ( (m_RecvSyncMode & SYNC_ACTIVE) == 0 ) {
		m_RecvSema.Lock();
		if ( (sp = m_ProcHead) != NULL ) {
			m_ProcHead = DetachHead(m_ProcHead);
			i = sp->m_Type;
			m_RecvSema.Unlock();
			if ( (n = OnReciveProcBack(sp->GetPtr(), sp->GetSize(), sp->m_Type)) > 0 ) {
				m_RecvSema.Lock();
				m_RecvSize -= n;
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
			if ( m_RecvProcSize < RECVMINSIZ )
				OnRecvEmpty();
			if ( m_RecvSize < RECVMINSIZ && m_Fd != (-1) && (m_SocketEvent & EventMask[i]) == 0 )
				OnRecive(i);

			m_OnRecvFlag &= ~RECV_INPROC;
			return TRUE;
		} else
			m_RecvSema.Unlock();

	} else if ( (m_RecvSyncMode & SYNC_EMPTY) != 0 ) {
		m_RecvSyncMode &= ~SYNC_EMPTY;
		OnRecvEmpty();
		if ( m_Fd != (-1) ) {
			if ( (m_SocketEvent & EventMask[0]) == 0 )
				OnRecive(0);
			if ( (m_SocketEvent & EventMask[1]) == 0 )
				OnRecive(1);
		}
	}

	m_OnRecvFlag &= ~RECV_INPROC;
	return FALSE;
}
int CExtSocket::OnReciveProcBack(void *lpBuf, int nBufLen, int nFlags)
{
//	TRACE("OnReciveProcBack %d/%d\n", nBufLen, m_RecvSize);
	
	if ( (m_OnRecvFlag & RECV_PROCBACK) != 0 )
		return 0;

	m_OnRecvFlag |= RECV_PROCBACK;

	if ( m_pDocument != NULL )
		nBufLen = m_pDocument->OnSocketRecive((LPBYTE)lpBuf, nBufLen, nFlags);

	m_OnRecvFlag &= ~RECV_PROCBACK;

	return nBufLen;
}
int CExtSocket::OnIdle()
{
	if ( m_DestroyFlag ) {
		Destroy();
		return TRUE;
	}

	if ( (m_OnSendFlag & SEND_EMPTY) != 0 ) {
		m_OnSendFlag &= ~SEND_EMPTY;
		OnSendEmpty();
	}

	if ( ReciveCall() )
		return TRUE;

	if ( ReciveProc() )
		return TRUE;

	if ( (m_OnRecvFlag & RECV_DOCLOSE) != 0 && GetRecvSize() == 0 ) {
		m_OnRecvFlag &= ~RECV_DOCLOSE;
		OnClose();
	}

	return FALSE;
}
void CExtSocket::OnTimer(UINT_PTR nIDEvent)
{
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::IOCtl(long lCommand, DWORD* lpArgument)
{
	if ( m_Fd == (-1) || ::ioctlsocket(m_Fd, lCommand, lpArgument) != 0 )
		return FALSE;
	return TRUE;
}
BOOL CExtSocket::SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel)
{
	if ( m_Fd == (-1) || ::setsockopt(m_Fd, nLevel, nOptionName, (const char *)lpOptionValue, nOptionLen) != 0 )
		return FALSE;
	return TRUE;
}
BOOL CExtSocket::GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel)
{
	if ( m_Fd == (-1) || ::getsockopt(m_Fd, nLevel, nOptionName, (char *)lpOptionValue, lpOptionLen) != 0 )
		return FALSE;
	return TRUE;
}
void CExtSocket::GetPeerName(int fd, CString &host, int *port)
{
#ifdef	NOIPV6
	int inlen;
	struct sockaddr_in in;

	memset(&in, 0, sizeof(in));
	inlen = sizeof(in);
	getpeername(fd, (struct sockaddr *)&in, &inlen);
	host = inet_ntoa(in.sin_addr);
	*port = ntohs(in.sin_port);
#else
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
#endif
}
void CExtSocket::GetSockName(int fd, CString &host, int *port)
{
#ifdef	NOIPV6
	int inlen;
	struct sockaddr_in in;

	memset(&in, 0, sizeof(in));
	inlen = sizeof(in);
	getsockname(fd, (struct sockaddr *)&in, &inlen);
	host = inet_ntoa(in.sin_addr);
	*port = ntohs(in.sin_port);
#else
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
#endif
}
void CExtSocket::GetHostName(struct sockaddr *addr, int addrlen, CString &host)
{
#ifdef	NOIPV6
	struct sockaddr_in *in = (struct sockaddr_in *)addr;

	host = inet_ntoa(in->sin_addr);
#else
	char name[NI_MAXHOST], serv[NI_MAXSERV];

	memset(name, 0, sizeof(name));
	memset(serv, 0, sizeof(serv));
	getnameinfo(addr, (socklen_t)addrlen, name, sizeof(name), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
	host = name;
#endif
}
int CExtSocket::GetPortNum(LPCTSTR str)
{
	int n;
    struct servent *sp;

	if ( *str == 0 )
		return (-1);
	else if ( _tcsncmp(str, _T("COM"), 3) == 0 )
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
#ifdef	NOIPV6
	SOCKET Fd;
    struct hostent *hp;
    struct sockaddr_in in;

	if ( (Fd = ::socket(AF_INET, nSocketType, 0)) == (-1) )
		return FALSE;

	if ( nSocketPort != 0 ) {
	    memset(&in, 0, sizeof(in));
	    in.sin_family = AF_INET;
	    in.sin_addr.s_addr = INADDR_ANY;
	    in.sin_port = htons(nSocketPort);
		if ( ::bind(Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR )
			goto ERRRET;
	}

	if ( (hp = ::gethostbyname(TstrToMbs(lpszHostAddress))) == NULL )
		goto ERRRET;

	in.sin_family = AF_INET;
	in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	in.sin_port = htons(nHostPort);

    if ( ::connect(Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR )
		goto ERRRET;

	::closesocket(Fd);
	return TRUE;

ERRRET:
	::closesocket(Fd);
	return FALSE;

#else
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
#endif
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::PunyCodeEncode(LPCWSTR str, CString &out)
{
	int n, i, len, count, basic;
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

//////////////////////////////////////////////////////////////////////

int CExtSocket::SSLConnect()
{
	DWORD val;
	const SSL_METHOD *method;
	long result = 0;
	X509 *cert = NULL;
	const char *errstr;
	CString tmp, dig;
	CIdKey key;
	BOOL rf = FALSE;

	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), 0, 0);

	val = 0;
	::ioctlsocket(m_Fd, FIONBIO, &val);

	GetApp()->SSL_Init();

	switch(m_SSL_mode) {
	case 1:
		method = SSLv2_client_method();
		break;
	case 2:
		method = SSLv3_client_method();
		break;
	case 3:
		method = TLSv1_client_method();
		break;
#if	OPENSSL_VERSION_NUMBER >= 0x10001000L
	case 4:
		method = TLSv1_1_client_method();
		break;
	case 5:
		method = TLSv1_2_client_method();
		break;
#endif
	}

	if ( (m_SSL_pCtx = SSL_CTX_new((SSL_METHOD *)method)) == NULL )
		goto ERRENDOF;

	if ( (m_SSL_pSock = SSL_new(m_SSL_pCtx)) == NULL )
		goto ERRENDOF;

	SSL_CTX_set_mode(m_SSL_pCtx, SSL_MODE_AUTO_RETRY);
	SSL_set_fd(m_SSL_pSock, m_Fd);

	if ( SSL_connect(m_SSL_pSock) < 0 )
		goto ERRENDOF;

	cert   = SSL_get_peer_certificate(m_SSL_pSock);
	result = SSL_get_verify_result(m_SSL_pSock);

	if ( cert == NULL || result != 0 ) {
		if ( cert != NULL && cert->name != NULL && cert->cert_info != NULL && cert->cert_info->key != NULL && cert->cert_info->key->pkey != NULL ) {
			key.SetEvpPkey(cert->cert_info->key->pkey);
			key.WritePublicKey(dig);
			tmp = AfxGetApp()->GetProfileString(_T("Certificate"), MbsToTstr(cert->name), _T(""));
			if ( !tmp.IsEmpty() && tmp.Compare(dig) == 0 )
				rf = TRUE;
		}
		if ( rf == FALSE ) {
			errstr = X509_verify_cert_error_string(result);
			tmp.Format(CStringLoad(IDS_CERTTRASTREQ), (cert != NULL && cert->name != NULL ? MbsToTstr(cert->name) : _T("unknown")),  MbsToTstr(errstr));
			if ( AfxMessageBox(tmp, MB_ICONQUESTION | MB_YESNO) != IDYES )
				goto ERRENDOF;
			if ( cert != NULL && cert->name != NULL && !dig.IsEmpty() )
				AfxGetApp()->WriteProfileString(_T("Certificate"), MbsToTstr(cert->name), dig);
		}
	}

	if ( cert != NULL )
		X509_free(cert);

	val = 1;
	::ioctlsocket(m_Fd, FIONBIO, &val);

	m_SocketEvent |= FD_READ;
	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);

	return TRUE;

ERRENDOF:
	if ( cert != NULL )
		X509_free(cert);
	SSLClose();
	return FALSE;
}
void CExtSocket::SSLClose()
{
	if ( m_SSL_pSock != NULL )
		SSL_free(m_SSL_pSock);

	if ( m_SSL_pCtx != NULL )
		SSL_CTX_free(m_SSL_pCtx);

	m_SSL_mode  = 0;
	m_SSL_pCtx  = NULL;
	m_SSL_pSock = NULL;
}
