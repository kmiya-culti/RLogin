///////////////////////////////////////////////////////
// Login.cpp : ŽÀ‘•ƒtƒ@ƒCƒ‹
//

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

#define TIOCPKT_FLUSHREAD	0x01	/* flush packet */
#define TIOCPKT_FLUSHWRITE	0x02	/* flush packet */
#define TIOCPKT_STOP		0x04	/* stop output */
#define TIOCPKT_START		0x08	/* start output */
#define TIOCPKT_NOSTOP		0x10	/* no more ^S, ^Q */
#define TIOCPKT_DOSTOP		0x20	/* now do ^S ^Q */
#define TIOCPKT_IOCTL		0x40	/* state change of pty driver */
#define TIOCPKT_WINDOW		0x80

///////////////////////////////////////////////////////
// CFifoLogin

CFifoLogin::CFifoLogin(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoThread(pDoc, pSock)
{
}
void CFifoLogin::SendStr(int nFd, LPCSTR mbs)
{
	Write(nFd, (BYTE *)mbs, (int)strlen(mbs) + 1);
}
void CFifoLogin::SendWindSize(int nFd)
{
	struct	winsize {
		BYTE head[4];
		unsigned short row, col;
		unsigned short xpixel, ypixel;
	} winsize;
	int screen[4];

	DocMsgIntPtr(DOCMSG_SCREENSIZE, screen);

	winsize.head[0] = '\377';
	winsize.head[1] = '\377';
	winsize.head[2] = 's';
	winsize.head[3] = 's';
	winsize.row = htons(screen[1]);
	winsize.col = htons(screen[0]);
	winsize.xpixel = htons(screen[2]);
	winsize.ypixel = htons(screen[3]);

	Write(nFd, (BYTE *)&winsize, sizeof(winsize));
}
void CFifoLogin::OnOob(int nFd, int len, BYTE *pBuffer)
{
	ASSERT(nFd == FIFO_STDIN);

	for ( ; len > 0 ; len--, pBuffer++ ) {
		if ( (*pBuffer & TIOCPKT_WINDOW) != 0 )
			SendWindSize(FIFO_STDOUT);
	}
}
void CFifoLogin::OnConnect(int nFd)
{
	if ( nFd == FIFO_STDIN ) {
		CStringA str;
		SendStr(FIFO_STDOUT, "");
		SendStr(FIFO_STDOUT, DocMsgStrA(DOCMSG_USERNAME, str));
		SendStr(FIFO_STDOUT, str);
		SendStr(FIFO_STDOUT, DocMsgStrA(DOCMSG_TERMNAME, str));
	}
	CFifoThread::OnConnect(nFd);
}
void CFifoLogin::OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	switch(cmd) {
	case FIFO_CMD_MSGQUEIN:
		switch(param) {
		case FIFO_QCMD_SENDWINSIZE:
			SendWindSize(FIFO_STDOUT);
			break;
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// CLogin

CLogin::CLogin(class CRLoginDoc *pDoc) : CExtSocket(pDoc)
{
	m_Type = ESCT_RLOGIN;
}
CFifoBase *CLogin::FifoLinkMid()
{
	return new CFifoLogin(m_pDocument, this);
}
int CLogin::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	return CExtSocket::Open(lpszHostAddress, nHostPort, IPPORT_RESERVED - 1, nSocketType);
}
void CLogin::SendWindSize()
{
	m_pFifoMid->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDWINSIZE);
}
