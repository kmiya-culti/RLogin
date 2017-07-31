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

	for ( n = IPPORT_RESERVED - 1 ; n > (IPPORT_RESERVED / 2) ; n-- ) {
		if ( CExtSocket::Open(lpszHostAddress, nHostPort, n, SOCK_STREAM, pAddrInfo) )
			break;
	}

	if ( n <= (IPPORT_RESERVED / 2) ) {
#ifndef	NOIPV6
		if ( pAddrInfo != NULL )
			FreeAddrInfo((ADDRINFOT *)pAddrInfo);
#endif
		CString errmsg;
		errmsg.Format(_T("LoginSocket '%s' Port Error #%d"), lpszHostAddress, IPPORT_RESERVED / 2);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
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

void CLogin::SendWindSize(int x, int y)
{
	struct	winsize {
		unsigned short ws_row, ws_col;
		unsigned short ws_xpixel, ws_ypixel;
	} *wp;
	char obuf[4 + sizeof (struct winsize)];

	if ( m_ConnectFlag < 2 )
		return;

	wp = (struct winsize *)(obuf + 4);
	obuf[0] = '\377';
	obuf[1] = '\377';
	obuf[2] = 's';
	obuf[3] = 's';
	wp->ws_row = htons(y);
	wp->ws_col = htons(x);
	wp->ws_xpixel = htons(0);
	wp->ws_ypixel = htons(0);

	CExtSocket::Send(obuf, sizeof(obuf));
}

void CLogin::OnReciveCallBack(void *lpBuf, int nBufLen, int nFlags)
{
	if ( m_ConnectFlag >= 2 && nFlags == 0 ) {
		CExtSocket::OnReciveCallBack(lpBuf, nBufLen, 0);
		return;
	}

	char *buf = (char *)lpBuf;
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
		if ( (buf[n] & 0x80) != 0 )
			SendWindSize(pDoc->m_TextRam.m_Cols, pDoc->m_TextRam.m_Lines);
		TRACE("Resv OOB %02x\n", buf[n]);
	}
	m_ConnectFlag = 3;
}
