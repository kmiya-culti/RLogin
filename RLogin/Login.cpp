// login.cpp: CLogin クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "Login.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CLogin::CLogin(class CRLoginDoc *pDoc) : CExtSocket(pDoc)
{
	m_Type = ESCT_RLOGIN;
	m_ConnectFlag = 0;
}

CLogin::~CLogin()
{
}

int CLogin::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
	int n;
	BOOL val = 1;

	for ( n = IPPORT_RESERVED - 1 ; ; n-- ) {
		if ( CExtSocket::Open(lpszHostAddress, nHostPort, n, SOCK_STREAM, pAddrInfo) )
			break;
		if ( WSAGetLastError() != 0 || n <= (IPPORT_RESERVED / 2) ) {
			if ( pAddrInfo != NULL )
				FreeAddrInfo((ADDRINFOT *)pAddrInfo);
			return FALSE;
		}
	}

	SetSockOpt(TCP_NODELAY, &val, sizeof(BOOL), SOL_SOCKET);
	m_ConnectFlag = 1;
	return TRUE;
}

void CLogin::OnConnect()
{
	CRLoginDoc *pDoc = GetDocument();
	SendStr("");
	SendStr(pDoc->RemoteStr(pDoc->m_ServerEntry.m_UserName));
	SendStr(pDoc->RemoteStr(pDoc->m_ServerEntry.m_UserName));
	SendStr(pDoc->RemoteStr(pDoc->m_ServerEntry.m_TermName));
	m_ConnectFlag = 2;
	CExtSocket::OnConnect();
}

void CLogin::SendStr(LPCSTR str)
{
	CExtSocket::Send(str, (int)strlen(str) + 1);		// With '\0'
}

void CLogin::SendWindSize()
{
	struct	winsize {
		unsigned short ws_row, ws_col;
		unsigned short ws_xpixel, ws_ypixel;
	} *wp;
	char obuf[4 + sizeof (struct winsize)];
	int cx = 0, cy = 0, sx = 0, sy = 0;

	if ( m_pDocument != NULL )
		m_pDocument->m_TextRam.GetScreenSize(&cx, &cy, &sx, &sy);

	if ( m_ConnectFlag < 2 )
		return;

	wp = (struct winsize *)(obuf + 4);
	obuf[0] = '\377';
	obuf[1] = '\377';
	obuf[2] = 's';
	obuf[3] = 's';
	wp->ws_row = htons(cy);
	wp->ws_col = htons(cx);
	wp->ws_xpixel = htons(sx);
	wp->ws_ypixel = htons(sy);

	CExtSocket::Send(obuf, sizeof(obuf));
}

void CLogin::OnReceiveCallBack(void *lpBuf, int nBufLen, int nFlags)
{
	if ( m_ConnectFlag == 3 && nFlags == 0 ) {
		CExtSocket::OnReceiveCallBack(lpBuf, nBufLen, 0);
		return;
	}

	BYTE *buf = (BYTE *)lpBuf;
	CRLoginDoc *pDoc = GetDocument();

#define TIOCPKT_FLUSHREAD	0x01	/* flush packet */
#define TIOCPKT_FLUSHWRITE	0x02	/* flush packet */
#define TIOCPKT_STOP		0x04	/* stop output */
#define TIOCPKT_START		0x08	/* start output */
#define TIOCPKT_NOSTOP		0x10	/* no more ^S, ^Q */
#define TIOCPKT_DOSTOP		0x20	/* now do ^S ^Q */
#define TIOCPKT_IOCTL		0x40	/* state change of pty driver */
#define TIOCPKT_WINDOW		0x80

	for ( int n = 0 ; n < nBufLen ; n++ ) {
		if ( pDoc != NULL && (buf[n] & 0x80) != 0 )
			SendWindSize();
		TRACE("Resv OOB %02x\n", buf[n]);
	}
	m_ConnectFlag = 3;
}
