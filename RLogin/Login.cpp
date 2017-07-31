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

int CLogin::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	int n;
	BOOL val = 1;

	for ( n = IPPORT_RESERVED - 1 ; n > (IPPORT_RESERVED / 2) ; n-- ) {
		if ( CExtSocket::Open(lpszHostAddress, nHostPort, n, SOCK_STREAM) )
			break;
	}

	if ( n <= (IPPORT_RESERVED / 2) ) {
		CString errmsg;
		errmsg.Format("LoginSocket '%s' Port Error #%d", lpszHostAddress, IPPORT_RESERVED / 2);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
	}

	SetSockOpt(TCP_NODELAY, &val, sizeof(BOOL), SOL_SOCKET);
	SetRecvSize(16 * 1024);
	m_ConnectFlag = 1;
	return TRUE;
}

void CLogin::OnConnect()
{
	CRLoginDoc *pDoc = GetDocument();
	SendStr("");
	SendStr(pDoc->m_ServerEntry.m_UserName);
	SendStr(pDoc->m_ServerEntry.m_UserName);
	SendStr(pDoc->m_ServerEntry.m_TermName);
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

	for ( int n = 0 ; n < nBufLen ; n++ ) {
		if ( (buf[n] & 0x80) != 0 )
			SendWindSize(pDoc->m_TextRam.m_Cols, pDoc->m_TextRam.m_Lines);
	}
	m_ConnectFlag = 3;
}
