//////////////////////////////////////////////////////////////////////
// sshFilter.cpp
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "EditDlg.h"
#include "IdKeyFileDlg.h"
#include "ssh.h"

#include "ssh1.h"
#include "ssh2.h"

#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CFifoStdio

CFifoStdio::CFifoStdio(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoDocument(pDoc, pSock)
{
}
void CFifoStdio::EndOfData(int nFd)
{
	SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)(UINT_PTR)m_nLastError);
}
void CFifoStdio::OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	if ( cmd != FIFO_CMD_MSGQUEIN )
		return;

	switch(param) {
	case FIFO_QCMD_ENDOFDATA:
		if ( msg == 0 ) {
			m_pSock->OnClose();
			m_pDocument->OnSocketClose();
		} else {
			m_pSock->OnError(msg);
			m_pDocument->OnSocketError(msg);
		}
		break;
	case FIFO_QCMD_RCPDOWNLOAD:
		((Cssh *)m_pSock)->OpenRcpDownload(m_pDocument->m_CmdsPath);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// CFifoSftp

CFifoSftp::CFifoSftp(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CSFtp *pSFtp) : CFifoBase(pDoc, pSock)
{
	m_pSFtp = pSFtp;
}
CFifoSftp::~CFifoSftp()
{
	if ( m_pSFtp->m_hWnd != NULL )
		m_pSFtp->PostMessage(WM_RECIVEBUFFER, (WPARAM)FD_CLOSE);
}
void CFifoSftp::FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam)
{
	switch(fdEvent) {
	case FD_READ:	// Fifo.PutBuffer
		if ( !m_pSFtp->m_bPostMsg && m_pSFtp->m_pFifoSftp != NULL ) {
			m_pSFtp->m_bPostMsg = TRUE;
			if ( !m_pSFtp->m_bBuzy )
				m_pSFtp->PostMessage(WM_RECIVEBUFFER, (WPARAM)FD_READ);
			else
				theApp.IdlePostMessage(m_pSFtp->GetSafeHwnd(), WM_RECIVEBUFFER, (WPARAM)FD_READ);
		}
		break;

	case FD_WRITE:	// Fifo.GetBuffer
	case FD_OOB:
	case FD_ACCEPT:
		break;

	case FD_CONNECT:
		m_pSFtp->PostMessage(WM_RECIVEBUFFER, (WPARAM)FD_CONNECT);
		break;

	case FD_CLOSE:
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// CFifoSocks

CFifoSocks::CFifoSocks(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan) : CFifoSync(pDoc, pSock)
{
	m_rFd = FIFO_STDIN;
	m_wFd = FIFO_STDOUT;
	m_ChanId = pChan->m_LocalID;

	m_RetCode = 0;
	m_HostName.Empty();
	m_HostPort = 0;
}
void CFifoSocks::OnLinked(int nFd, BOOL bMid)
{
	CFifoSync::OnLinked(nFd, bMid);

	if ( nFd == FIFO_STDIN )
		Open(DecodeSocksWorker, this);
}
void CFifoSocks::OnUnLinked(int nFd, BOOL bMid)
{
	if ( nFd == FIFO_STDIN )
		Close();

	CFifoSync::OnUnLinked(nFd, bMid);
}
void CFifoSocks::FifoEvents(int nFd, CFifoBuffer *pFifo, DWORD fdEvent, void *pParam)
{
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
	case FD_CONNECT:
		break;

	case FD_CLOSE:
		Abort();
		m_nLastError = (int)(UINT_PTR)pParam;
		m_fdPostEvents[nFd] |= fdEvent;
		break;
	}
}
UINT CFifoSocks::DecodeSocksWorker(void *pParam)
{
	CFifoSocks *pThis = (CFifoSocks *)pParam;

	pThis->DecodeSocks();
	pThis->RecvBaek(pThis->m_rFd);
	pThis->SendFdCommand(FIFO_STDOUT, FIFO_CMD_MSGQUEIN, FIFO_QCMD_SYNCRET, pThis->m_ChanId, 0, (void *)pThis);

	pThis->m_Threadtatus = FIFO_THREAD_NONE;
	pThis->m_ThreadEvent.SetEvent();
	return 0;
}
void CFifoSocks::DecodeSocks()
{
	int n, c;
	struct {
		BYTE version;
		BYTE command;
		WORD dest_port;
		union {
			struct in_addr	in_addr;
			DWORD			dw_addr;
			BYTE			bt_addr[4];
		} dest;
	} s4_req;
	CBuffer tmp;
	BOOL bS4a = FALSE;
	BYTE buf[32];
	struct sockaddr_in in;
    struct sockaddr_in6 in6;
	CString host;

#define	SOCKS_TIMEOUT	10000	// 10sec

	if ( RecvBuffer(m_rFd, (BYTE *)&s4_req, 2, SOCKS_TIMEOUT) != 2 )
		return;

	if ( s4_req.version == 4 ) {			// Socks ver4
		tmp.Clear();
		if ( RecvBuffer(m_rFd, tmp.PutSpc(2 + 4), 2 + 4, SOCKS_TIMEOUT) != (2 + 4) )
			return;

		s4_req.dest_port    = tmp.GetWord();	// LT Word
		s4_req.dest.dw_addr = tmp.GetDword();	// LT DWord

		if ( s4_req.command != 0x01 )
			return;

		if ( s4_req.dest.bt_addr[0] == 0 && s4_req.dest.bt_addr[1] == 0 && s4_req.dest.bt_addr[2] == 0 && s4_req.dest.bt_addr[3] != 0 )
			bS4a = TRUE;

		for ( ; ; ) {	// UserID
			if ( (c = RecvByte(m_rFd, SOCKS_TIMEOUT)) < 0 )
				return;
			else if ( c == '\0' )
				break;
		}

		if ( !bS4a ) {
			m_HostName = inet_ntoa(s4_req.dest.in_addr);
		} else {
			m_HostName.Empty();
			for ( ; ; ) {	// Domain Name
				if ( (c = RecvByte(m_rFd, SOCKS_TIMEOUT)) < 0 )
					return;
				else if ( c == '\0' )
					break;
				else
					m_HostName += (CHAR)c;
			}
		}
		m_HostPort = ntohs(s4_req.dest_port);

		tmp.Clear();
		tmp.Put8Bit(0);
		tmp.Put8Bit(90);
		tmp.Put16Bit(0);
		tmp.Put32Bit(INADDR_ANY);
		Write(m_wFd, tmp.GetPtr(), tmp.GetSize());

		m_RetCode = 1;

	} else if (s4_req.version == 5 ) {		// Socks ver5
		tmp.Clear();
		if ( RecvBuffer(m_rFd, tmp.PutSpc(s4_req.command), s4_req.command, SOCKS_TIMEOUT) != s4_req.command )
			return;

		for ( n = 0 ; n < s4_req.command ; n++ ) {
			if ( tmp.GetByte() == 0 )		// SOCKS5_NOAUTH
				break;
		}
		if ( n >= s4_req.command )
			return;

		tmp.Clear();
		tmp.Put8Bit(5);
		tmp.Put8Bit(0);
		Write(m_wFd, tmp.GetPtr(), tmp.GetSize());

		if ( RecvBuffer(m_rFd, buf, 4, SOCKS_TIMEOUT) != 4 )
			return;

		if ( buf[0] != 5 || buf[1] != 1 || buf[2] != 0 )
			return;

		if ( buf[3] == 1 ) {		// SOCKS5_IPV4
			if ( RecvBuffer(m_rFd, buf, (4 + 2), SOCKS_TIMEOUT) != (4 + 2) )
				return;

			memset(&in, 0, sizeof(in));
			in.sin_family = AF_INET;
			memcpy(&(in.sin_addr), buf, 4);
			CExtSocket::GetHostName((struct sockaddr *)&in, sizeof(in), host);
			m_HostName = TstrToMbs(host);
			m_HostPort = ntohs(*((WORD *)(buf + 4)));

		} else if ( buf[3] == 3 ) {	// SOCKS5_DOMAIN
			if ( RecvBuffer(m_rFd, buf + 4, 1, SOCKS_TIMEOUT) != 1 )
				return;

			tmp.Clear();
			if ( RecvBuffer(m_rFd, tmp.PutSpc((BYTE)buf[4]), (BYTE)buf[4], SOCKS_TIMEOUT) != (BYTE)buf[4] )
				return;
			if ( RecvBuffer(m_rFd, buf, 2, SOCKS_TIMEOUT) != 2 )
				return;

			m_HostName = (LPCSTR)tmp;
			m_HostPort = ntohs(*((WORD *)(buf + 0)));

		} else if ( buf[3] == 4 ) {	// SOCKS5_IPV6
			if ( RecvBuffer(m_rFd, buf, (16 + 2), SOCKS_TIMEOUT) != (16 + 2) )
				return;

			memset(&in6, 0, sizeof(in6));
			in6.sin6_family = AF_INET6;
			memcpy(&(in6.sin6_addr), buf, 16);
			CExtSocket::GetHostName((struct sockaddr *)&in6, sizeof(in6), host);
			m_HostName = TstrToMbs(host);
			m_HostPort = ntohs(*((WORD *)(buf + 16)));

		} else
			return;

		tmp.Clear();
		tmp.Put8Bit(0x05);
		tmp.Put8Bit(0x00);	// SOCKS5_SUCCESS
		tmp.Put8Bit(0x00);
		tmp.Put8Bit(0x01);	// SOCKS5_IPV4
		tmp.Put32Bit(INADDR_ANY);
		tmp.Put16Bit(m_HostPort);
		Write(m_wFd, tmp.GetPtr(), tmp.GetSize());

		m_RetCode = 1;

	} else if ( s4_req.version == 'C' && s4_req.command == 'O' ) {	// https CONNECT
		CStringA line, port;
		LPCSTR p;

		line += (CHAR)s4_req.version;
		line += (CHAR)s4_req.command;
		while ( (c = RecvByte(m_rFd, SOCKS_TIMEOUT)) >= 0 ) {
			if ( c == '\n' )
				break;
			else if ( c != '\0' && c != '\r' )
				line += (CHAR)c;
		}

		if ( strncmp(line, "CONNECT", 7) != 0 )
			return;
		for ( p = (LPCSTR)line + 7 ; *p == ' ' || *p == '\t' ; p++ );

		m_HostName.Empty();
		m_HostPort = 80;

		while ( *p != '\0' && *p != ' ' && *p != '\t' && *p != ':' )
			m_HostName += *(p++);
		if ( *p == ':' ) {
			p++;
			while ( *p != '\0' && *p != ' ' && *p != '\t' )
				port += *(p++);
			m_HostPort = CExtSocket::GetPortNum(MbsToTstr(port));
		}
		for ( ; *p == ' ' || *p == '\t' ; p++ );

		if ( strncmp(p, "HTTP/", 5) != 0 )
			return;

		line.Empty();
		for ( ; ; ) {
			if ( (c = RecvByte(m_rFd, SOCKS_TIMEOUT)) < 0 )
				return;
			if ( c == '\n' ) {
				if ( line.IsEmpty() )
					break;
				line.Empty();
			} else if ( c != '\0' && c != '\r' )
				line += (CHAR)c;
		}

		if ( m_HostName.IsEmpty() ) {
			Write(m_wFd, (LPBYTE)"HTTP/1.1 502 Bad Gateway\r\n\r\n", 28);
			m_RetCode = 0;
		} else {
			Write(m_wFd, (LPBYTE)"HTTP/1.1 200 OK\r\n\r\n", 19);
			m_RetCode = 1;
		}

	} else if ( (s4_req.version == 'G' && s4_req.command == 'E') ||		// http GET
			    (s4_req.version == 'H' && s4_req.command == 'E') ||		// http HEAD
			    (s4_req.version == 'P' && s4_req.command == 'O') ||		// http POST
			    (s4_req.version == 'O' && s4_req.command == 'P') ) {	// http OPTIONS
		CStringA line;
		CBuffer tmp;
		LPCSTR p;

		m_HostName.Empty();
		m_HostPort = 80;

		tmp.PutByte(s4_req.version);
		tmp.PutByte(s4_req.command);

		line += (CHAR)s4_req.version;
		line += (CHAR)s4_req.command;

		while ( (c = RecvByte(m_rFd, SOCKS_TIMEOUT)) >= 0 ) {
			tmp.PutByte(c);
			if ( c == '\n' )
				break;
			else if ( c != '\0' && c != '\r' )
				line += (CHAR)c;
		}

		if ( strncmp(line, "GET", 3) != 0 && strncmp(line, "HEAD", 4) != 0 && strncmp(line, "POST", 4) != 0 && strncmp(line, "OPTIONS", 7) != 0 )
			return;

		line.Empty();
		for ( ; ; ) {
			if ( (c = RecvByte(m_rFd, SOCKS_TIMEOUT)) < 0 )
				return;
			tmp.PutByte(c);
			if ( c == '\n' ) {
				p = line;
				if ( strnicmp(p, "Host:", 5) == 0 ) {
					for ( p = (LPCSTR)line + 5 ; *p == ' ' || *p == '\t' ; p++ );
					while ( *p != '\0' && *p != ':' )
						m_HostName += *(p++);
					if ( *p == ':' ) {
						p++;
						m_HostPort = CExtSocket::GetPortNum(MbsToTstr(p));
					}
				}
				if ( line.IsEmpty() )
					break;
				line.Empty();
			} else if ( c != '\0' && c != '\r' )
				line += (CHAR)c;
		}

		if ( m_HostName.IsEmpty() ) {
			Write(m_wFd, (LPBYTE)"HTTP/1.1 502 Bad Gateway\r\n\r\n", 28);
			m_RetCode = 0;
		} else {
			RecvBaek(m_rFd, tmp.GetPtr(), tmp.GetSize());
			m_RetCode = 1;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CFifoAgent

CFifoAgent::CFifoAgent(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan) : CFifoThread(pDoc, pSock)
{
	m_pChan = pChan;
}
void CFifoAgent::OnRead(int nFd)
{
	int len;
	CBuffer recv;

	for ( ; ; ) {
		if ( (len = MoveBuffer(FIFO_STDIN, &m_RecvBuffer)) < 0 ) {
			// End of Data
			ResetFdEvents(nFd, FD_READ);
			SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)(UINT_PTR)m_nLastError);
			break;
		} else if ( len == 0 ) {
			// No Data
			break;
		} else {
			while ( m_RecvBuffer.GetSize() >= 4 ) {
				len = m_RecvBuffer.PTR32BIT(m_RecvBuffer.GetPtr());
				if ( len > (256 * 1024) || len < 0 ) {
					// Packet Size error
					ResetFdEvents(nFd, FD_READ);
					SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)WSAEFAULT);
					break;
				}
				if ( m_RecvBuffer.GetSize() < (len + 4) )
					break;
				recv.Clear();
				recv.Apend(m_RecvBuffer.GetPtr() + 4, len);
				m_RecvBuffer.Consume(len + 4);

				ReceiveBuffer(&recv);
			}
		}
	}
}
void CFifoAgent::OnWrite(int nFd)
{
	ResetFdEvents(nFd, FD_WRITE);
}
void CFifoAgent::OnConnect(int nFd)
{
}
void CFifoAgent::OnClose(int nFd, int nLastError)
{
	if ( nFd == FIFO_STDIN ) {
		m_nLastError = nLastError;
	} else if ( nFd == FIFO_STDOUT ) {
		SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)(UINT_PTR)nLastError);
		Write(FIFO_STDOUT, NULL, 0);
	}
}

//////////////////////////////////////////////////////////////////////

BOOL CFifoAgent::IsLocked()
{
	if ( ((Cssh *)m_pSock)->m_pAgentMutex != NULL )
		return FALSE;	// Locked this

	CMutex Mutex(FALSE, _T("RLogin_SSH2_AGENT"), NULL);

	if ( GetLastError() == ERROR_ALREADY_EXISTS )
		return TRUE;

	return FALSE;
}
LPCTSTR CFifoAgent::GetRsaSignAlg(CIdKey *key, int flag)
{
	if ( key == NULL || key->m_Type != IDKEY_RSA2 )
		return NULL;

	switch(flag & (SSH_AGENT_RSA_SHA2_256 | SSH_AGENT_RSA_SHA2_512)) {
	case SSH_AGENT_RSA_SHA2_256:
		return _T("rsa-sha2-256");
	case SSH_AGENT_RSA_SHA2_512:
		return _T("rsa-sha2-512");
	case SSH_AGENT_RSA_SHA2_256 | SSH_AGENT_RSA_SHA2_512:
		return (m_pDocument->m_ParamTab.m_RsaExt == 2 ? _T("rsa-sha2-512") : _T("rsa-sha2-256"));
	}

	return NULL;
}
CIdKey *CFifoAgent::GetIdKey(CIdKey *key, LPCTSTR pass)
{
	int n;

	for ( n = 0 ; n < ((Cssh *)m_pSock)->m_IdKeyTab.GetSize() ; n++ ) {
		if ( ((Cssh *)m_pSock)->m_IdKeyTab[n].Compere(key) == 0 ) {
			if ( pass == NULL || ((Cssh *)m_pSock)->m_IdKeyTab[n].InitPass(pass) )
				return &(((Cssh *)m_pSock)->m_IdKeyTab[n]);
		}
	}

	return NULL;
}
void CFifoAgent::ReceiveBuffer(CBuffer *bp)
{
	int rc = SSH_AGENT_FAILURE;
	CBuffer tmp;
	int type = bp->Get8Bit();

	try {
		switch(type) {
		case SSH_AGENTC_REQUEST_IDENTITIES:
			{
				int n, i = 0;
				CIdKey *pKey;
				CBuffer data, work;

				for ( n = 0 ; n < ((Cssh *)m_pSock)->m_IdKeyTab.GetSize() ; n++ ) {
					pKey = &(((Cssh *)m_pSock)->m_IdKeyTab[n]);
					if ( pKey->m_Type == IDKEY_RSA1 ) {
						BIGNUM const *e = NULL, *n = NULL;

						RSA_get0_key(pKey->m_Rsa, &n, &e, NULL);

						data.Put32Bit(RSA_bits(pKey->m_Rsa));
						data.PutBIGNUM(e);
						data.PutBIGNUM(n);
						data.PutStr(TstrToMbs(pKey->m_Name));
						i++;
					} else if ( pKey->m_Type != IDKEY_NONE ) {
						work.Clear();
						pKey->SetBlob(&work);
						data.PutBuf(work.GetPtr(), work.GetSize());
						data.PutStr(TstrToMbs(pKey->m_Name));
						i++;
					}
				}
				work.Clear();
				work.Put8Bit(SSH_AGENT_IDENTITIES_ANSWER);
				work.Put32Bit(i);
				work.Apend(data.GetPtr(), data.GetSize());

				tmp.Put32Bit(work.GetSize());
				tmp.Apend(work.GetPtr(), work.GetSize());
				Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
			}
			break;

		case SSH_AGENTC_SIGN_REQUEST:
			{
				int flag;
				CBuffer blob, data, sig;
				CIdKey key, *pkey;

				bp->GetBuf(&blob);
				bp->GetBuf(&data);
				flag = bp->Get32Bit();

				key.GetBlob(&blob);

				if ( (pkey = GetIdKey(&key, m_pDocument->m_ServerEntry.m_PassName)) != NULL && pkey->Sign(&sig, data.GetPtr(), data.GetSize(), GetRsaSignAlg(pkey, flag)) ) {
					data.Clear();
					data.Put8Bit(SSH_AGENT_SIGN_RESPONSE);
					data.PutBuf(sig.GetPtr(), sig.GetSize());
					tmp.Put32Bit(data.GetSize());
					tmp.Apend(data.GetPtr(), data.GetSize());
					Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
				} else {
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_FAILURE);
					Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
				}
			}
			break;

		case SSH_AGENTC_ADD_IDENTITY:
			//	byte                    SSH_AGENTC_ADD_IDENTITY
			//	string                  key type
			//	byte[]                  key contents
			//	string                  key comment
		case SSH_AGENTC_ADD_ID_CONSTRAINED:
			//	byte                    SSH_AGENTC_ADD_ID_CONSTRAINED
			//	string                  type
			//	byte[]                  contents
			//	string                  comment
			//	constraint[]            constraints
			{
				int n;
				CIdKey key, *pkey;
				CStringA work;
				CIdKeyFileDlg dlg;

				if ( !IsLocked() && key.GetPrivateBlob(bp) ) {
					bp->GetStr(work);
					key.m_Name = work;
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_SUCCESS);
					Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());

					if ( key.m_Cert != 0 ) {
						for ( n = 0 ; n < ((Cssh *)m_pSock)->m_IdKeyTab.GetSize() ; n++ ) {
							pkey = &(((Cssh *)m_pSock)->m_IdKeyTab[n]);
							if ( pkey->m_Cert == 0 && pkey->ComperePublic(&key) == 0 ) {
								pkey->m_Cert = key.m_Cert;
								pkey->m_CertBlob = key.m_CertBlob;
								((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab.UpdateUid(pkey->m_Uid);
							}
						}
					} else {
						dlg.m_OpenMode = IDKFDMODE_CREATE;
						dlg.m_Title.LoadString(IDS_IDKEYFILELOAD);;
						dlg.m_Message.LoadString(IDS_IDKEYCREATECOM);

						if ( dlg.DoModal() == IDOK ) {
							key.WritePrivateKey(key.m_SecBlob, dlg.m_PassName);
							if ( ((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab.AddEntry(key) )
								m_pDocument->m_ParamTab.m_IdKeyList.AddVal(key.m_Uid);
						}
					}
				} else {
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_FAILURE);
					Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
				}
			}
			break;

		case SSH_AGENTC_REMOVE_IDENTITY:
			{
				int n;
				CBuffer blob;
				CIdKey key, *pKey;

				bp->GetBuf(&blob);
				key.GetBlob(&blob);

				if ( !IsLocked() && (pKey = GetIdKey(&key, NULL)) != NULL ) {
					for ( n = 0 ; n < ((Cssh *)m_pSock)->m_IdKeyTab.GetSize() ; n++ ) {
						if ( pKey->m_Uid == ((Cssh *)m_pSock)->m_IdKeyTab[n].m_Uid ) {
							((Cssh *)m_pSock)->m_IdKeyTab.RemoveAt(n);
							break;
						}
					}
					for ( n = 0 ; n < m_pDocument->m_ParamTab.m_IdKeyList.GetSize() ; n++ ) {
						if ( pKey->m_Uid == m_pDocument->m_ParamTab.m_IdKeyList.GetVal(n) ) {
							m_pDocument->m_ParamTab.m_IdKeyList.RemoveAt(n);
							break;
						}
					}
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_SUCCESS);
					Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
				} else {
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_FAILURE);
					Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
				}
			}
			break;

		case SSH_AGENTC_LOCK:
			{
				CStringA mbs;
				bp->GetStr(mbs);

				tmp.Put32Bit(1);

				if ( ((Cssh *)m_pSock)->m_pAgentMutex != NULL ) {
					tmp.Put8Bit(((Cssh *)m_pSock)->m_AgentPass.Compare(mbs) == 0 ? SSH_AGENT_SUCCESS : SSH_AGENT_FAILURE);
				} else {
					((Cssh *)m_pSock)->m_pAgentMutex = new CMutex(TRUE, _T("RLogin_SSH2_AGENT"), NULL);
					if ( ((Cssh *)m_pSock)->m_pAgentMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS ) {
						if ( ((Cssh *)m_pSock)->m_pAgentMutex != NULL )
							delete ((Cssh *)m_pSock)->m_pAgentMutex;
						((Cssh *)m_pSock)->m_pAgentMutex = NULL;
						tmp.Put8Bit(SSH_AGENT_FAILURE);
					 } else {
						tmp.Put8Bit(SSH_AGENT_SUCCESS);
						((Cssh *)m_pSock)->m_AgentPass = mbs;
					 }
				}

				Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
			}
			break;
		case SSH_AGENTC_UNLOCK:
			{
				CStringA mbs;
				bp->GetStr(mbs);

				tmp.Put32Bit(1);

				if ( ((Cssh *)m_pSock)->m_pAgentMutex == NULL || ((Cssh *)m_pSock)->m_AgentPass.Compare(mbs) != 0 ) {
					tmp.Put8Bit(SSH_AGENT_FAILURE);
				} else {
					delete ((Cssh *)m_pSock)->m_pAgentMutex;
					((Cssh *)m_pSock)->m_pAgentMutex = NULL;
					tmp.Put8Bit(SSH_AGENT_SUCCESS);
				}

				Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
			}
			break;

		case SSH_AGENTC_EXTENSION:
			{
				CStringA mbs;
				bp->GetStr(mbs);
				if ( mbs.Compare("session-bind@openssh.com") == 0 ) {
					CIdKey key;
					CBuffer blob, sid, sig;
					int fwd;
					bp->GetBuf(&blob);
					bp->GetBuf(&sid);
					bp->GetBuf(&sig);
					fwd = bp->Get8Bit();

					key.GetBlob(&blob);

					CString kname, dig;
					CStringArrayExt entry;

					kname.Format(_T("%s:%d"), (LPCTSTR)((Cssh *)m_pSock)->m_HostName, ((Cssh *)m_pSock)->m_HostPort);
					((CRLoginApp *)AfxGetApp())->GetProfileStringArray(_T("KnownHosts"), kname, entry);

					key.m_Cert = 0;
					key.WritePublicKey(dig, FALSE);

					for ( int n = 0 ; n < entry.GetSize() ; n++ ) {
						if ( entry[n].Compare(dig) == 0 ) {
							if ( key.Verify(&sig, sid.GetPtr(), sid.GetSize()) )
								rc = SSH_AGENT_SUCCESS;
							break;
						}
					}
				}
			}
			tmp.Put32Bit(1);
			tmp.Put8Bit(rc);
			Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
			break;


		default:
			tmp.Put32Bit(1);
			tmp.Put8Bit(SSH_AGENT_FAILURE);
			Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
			break;
		}

	} catch(...) {
		tmp.Clear();
		tmp.Put32Bit(1);
		tmp.Put8Bit(SSH_AGENT_FAILURE);
		Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());
	}
}

//////////////////////////////////////////////////////////////////////
// CFifoX11

CFifoX11::CFifoX11(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan) : CFifoWnd(pDoc, pSock)
{
	m_pChan = pChan;
}

void CFifoX11::OnRead(int nFd)
{
	int readlen = (-1);
	BYTE *buf = TempBuffer(m_nBufSize);

	if ( nFd == FIFO_STDIN ) {
		// ssh server --> x11 socket
		while ( (readlen = Read(nFd, buf, m_nBufSize)) > 0 ) {
			if ( !OnReceive(buf, readlen) ) {
				CFifoBase::UnLink(this, FIFO_STDIN, TRUE);
				Destroy();
				return;
			}
		}

	} else {	// FIFO_EXTIN
		// x11 socket -> ssh server
		while ( (readlen = Read(nFd, buf, m_nBufSize)) > 0 )
			Write(GetXFd(nFd), buf, readlen);
	}

	if ( readlen < 0 )
		Write(GetXFd(nFd), NULL, 0);
}
void CFifoX11::OnWrite(int nFd)
{
	SendFdEvents(GetXFd(nFd), FD_WRITE, NULL);
}
void CFifoX11::OnOob(int nFd, int len, BYTE *pBuffer)
{
	ResetFdEvents(nFd, FD_OOB);
}
void CFifoX11::OnAccept(int nFd, SOCKET socket)
{
	ResetFdEvents(nFd, FD_ACCEPT);
}
void CFifoX11::OnConnect(int nFd)
{
	SendFdEvents(GetXFd(nFd), FD_CONNECT, NULL);
}
void CFifoX11::OnClose(int nFd, int nLastError)
{
	SendFdEvents(GetXFd(nFd), FD_CLOSE, (void *)(UINT_PTR)nLastError);

	CFifoBase::UnLink(this, FIFO_STDIN, TRUE);
	Destroy();
}

BOOL CFifoX11::OnReceive(const void *lpBuf, int nBufLen)
{
	int n;
	int name_len, data_len;
	int name_pad, data_pad;
	int over_pos, over_len;
	LPBYTE ptr;
	CBuffer tmp;

	m_Buffer.Apend((LPBYTE)lpBuf, nBufLen);

	if ( m_Buffer.GetSize() < 12 )
		return TRUE;

	ptr = m_Buffer.GetPtr();

	switch(ptr[0]) {
	case 0x42:		// MSB first
		name_len = (ptr[6] << 8) | ptr[7];
		data_len = (ptr[8] << 8) | ptr[9];
		break;
	case 0x6c:		// LSB first
		name_len = (ptr[7] << 8) | ptr[6];
		data_len = (ptr[9] << 8) | ptr[8];
		break;
	default:
		((Cssh *)m_pSock)->LogIt(_T("x11 #%d xauth unkown type '%02x'"), m_pChan->m_LocalID, ptr[0]);

		if ( !m_pDocument->m_ParamTab.m_x11AuthFlag )
			SendFdEvents(FIFO_EXTOUT, FD_CLOSE, (void *)WSAEFAULT);
		else
			Write(FIFO_EXTOUT, m_Buffer.GetPtr(), m_Buffer.GetSize());

		return FALSE;
	}
	
	name_pad = (name_len + 3) & ~0x3;
	data_pad = (data_len + 3) & ~0x3;

	if ( m_Buffer.GetSize() < (12 + name_pad + data_pad) )
		return TRUE;

	over_pos = 12 + name_pad + data_pad;
	over_len = m_Buffer.GetSize() - over_pos;

	if ( ((Cssh *)m_pSock)->m_x11AuthName.GetLength() != name_len || memcmp(TstrToMbs(((Cssh *)m_pSock)->m_x11AuthName), ptr + 12, name_len) != 0 ||
		 ((Cssh *)m_pSock)->m_x11AuthData.GetSize() != data_len   || memcmp(((Cssh *)m_pSock)->m_x11AuthData.GetPtr(), ptr + 12 + name_pad, data_len) != 0 ) {

		((Cssh *)m_pSock)->LogIt(_T("x11 #%d xauth %s data not match"), m_pChan->m_LocalID, ((Cssh *)m_pSock)->m_x11AuthName);

		if ( !m_pDocument->m_ParamTab.m_x11AuthFlag )
			SendFdEvents(FIFO_EXTOUT, FD_CLOSE, (void *)WSAEFAULT);
		else
			Write(FIFO_EXTOUT, m_Buffer.GetPtr(), m_Buffer.GetSize());

		return FALSE;
	}

	name_len = ((Cssh *)m_pSock)->m_x11LocalName.GetLength();
	data_len = ((Cssh *)m_pSock)->m_x11LocalData.GetSize();

	name_pad = (name_len + 3) & ~0x3;
	data_pad = (data_len + 3) & ~0x3;

	tmp.Apend(ptr, 6);	// 6c 00 0b 00 00 00
	switch(ptr[0]) {
	case 0x42:		// MSB first
		tmp.Put8Bit(name_len >> 8);
		tmp.Put8Bit(name_len & 0xFF);
		tmp.Put8Bit(data_len >> 8);
		tmp.Put8Bit(data_len & 0xFF);
		break;
	case 0x6c:		// LSB first
		tmp.Put8Bit(name_len & 0xFF);
		tmp.Put8Bit(name_len >> 8);
		tmp.Put8Bit(data_len & 0xFF);
		tmp.Put8Bit(data_len >> 8);
		break;
	}

	for ( n = tmp.GetSize() ; n < 12 ; n++ )
		tmp.Put8Bit(0);
	
	tmp.Apend((LPBYTE)(LPSTR)TstrToMbs(((Cssh *)m_pSock)->m_x11LocalName), name_len);
	for ( n = name_len ; n < name_pad ; n++ )
		tmp.Put8Bit(0);

	tmp.Apend(((Cssh *)m_pSock)->m_x11LocalData.GetPtr(), data_len);
	for ( n = data_len ; n < data_pad ; n++ )
		tmp.Put8Bit(0);

	if ( over_len > 0 )
		tmp.Apend(ptr + over_pos, over_len);

	Write(FIFO_EXTOUT, tmp.GetPtr(), tmp.GetSize());

	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// CFifoRcp

#define	FIFORCP_TIMEOUT		2000

CFifoRcp::CFifoRcp(class CRLoginDoc *pDoc, class CExtSocket *pSock, class CFifoChannel *pChan, int Mode) : CFifoSync(pDoc, pSock)
{
	m_pChan = pChan;
	m_Mode = Mode;
	m_pProgDlg = NULL;
	m_nBufSize = (8 * 1024);
	m_bAbortFlag = FALSE;
}
void CFifoRcp::OnLinked(int nFd, BOOL bMid)
{
	CFifoSync::OnLinked(nFd, bMid);

	if ( nFd == FIFO_STDIN )
		Open(RcpUpDowndWorker, this);
}
void CFifoRcp::OnUnLinked(int nFd, BOOL bMid)
{
	if ( nFd == FIFO_STDIN )
		Close();

	CFifoSync::OnUnLinked(nFd, bMid);
}
UINT CFifoRcp::RcpUpDowndWorker(void *pParam)
{
	CFifoRcp *pThis = (CFifoRcp *)pParam;

	switch(pThis->m_Mode) {
	case 0:
		pThis->RcpUpLoad();
		break;
	case 1:
		pThis->RcpDownLoad();
		break;
	}

	pThis->SendFdEvents(FIFO_STDOUT, FD_CLOSE, (void *)(UINT_PTR)pThis->m_nLastError);
	pThis->m_Threadtatus = FIFO_THREAD_NONE;
	pThis->m_ThreadEvent.SetEvent();

	return 0;
}
BOOL CFifoRcp::ReadLine(int nFd, CStringA &str)
{
	int c;

	str.Empty();
	for ( ; ; ) {
		if ( (c = RecvByte(nFd, FIFORCP_TIMEOUT)) < 0 ) {
			str.Format("read time over %dmsec", FIFORCP_TIMEOUT);
			return FALSE;
		}
		else if ( c == 0 || c == '\n' )
			break;
		else
			str += (CHAR)c;
	}
	return TRUE;
}
LPCTSTR CFifoRcp::MakePath(LPCTSTR base, LPCSTR name, CString &path)
{
	CString tmp = ((Cssh *)m_pSock)->LocalStr(name);
	LPCTSTR p = tmp;

	path = base;
	path += _T('\\');

	for ( p = tmp ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('/') )
			path += _T('\\');
		else if ( *p == _T('\\') )
			path += _T('_');
		else
			path += *p;
	}

	return path;
}
void CFifoRcp::RcpUpLoad()
{
/*
	read "\0" or "Error\n"
	write "T%I64u 0 %I64u 0\n"	st_mtime st_atime

	read "\0" or "Error\n"
	write "C%04o %I64d %s\n"	(st_mode & 0777) st_size, filename

	read "\0" or "Error\n"
	write File Data....st_size
	write "\0" or "Error\n"
*/
	int len;
	CStringA line;
	FILE *fp = NULL;
	struct _stati64 stat;
	LONGLONG size;
	BYTE *pTemp = TempBuffer(m_nBufSize);
	LONGLONG param[2];

	if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL )
		m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_FILENAME, (LPARAM)(LPCTSTR)m_PathName);

	if ( _tstati64(m_PathName, &stat) || (fp = _tfopen(m_PathName, _T("rb"))) == NULL ) {
		line.Format("file open error %d", GetLastError());
		goto ERRRET;
	}
	
	m_nLastError = WSAEFAULT;

	if ( (stat.st_mode & _S_IFMT) != _S_IFREG ) {
		line.Format("file mode error %d", (stat.st_mode & _S_IFMT));
		goto ERRRET;
	}

	if ( !ReadLine(FIFO_STDIN, line) || !line.IsEmpty() )
		goto ERRRET;

	line.Format("T%I64u 0 %I64u 0\n", stat.st_mtime, stat.st_atime);
	if ( Send(FIFO_STDOUT, (LPBYTE)(LPCSTR)line, line.GetLength(), FIFORCP_TIMEOUT) < 0 )
		goto ERRRET;

	if ( !ReadLine(FIFO_STDIN, line) || !line.IsEmpty() )
		goto ERRRET;

	line.Format("C%04o %I64d %s\n", stat.st_mode & 0777, stat.st_size, m_FileName);
	if ( Send(FIFO_STDOUT, (LPBYTE)(LPCSTR)line, line.GetLength(), FIFORCP_TIMEOUT) < 0 )
		goto ERRRET;

	if ( !ReadLine(FIFO_STDIN, line) || !line.IsEmpty() )
		goto ERRRET;

	if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL ) {
		m_pProgDlg->m_UpdatePost = TRUE;
		param[0] = stat.st_size;
		param[1] = 0;
		m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_RANGE, (LPARAM)param);
	}

	for ( size = stat.st_size ; size > 0 ; size -= len ) {
		len = (size < (LONGLONG)m_nBufSize ? (int)size : m_nBufSize);

		if ( fread(pTemp, 1, len, fp) != len ) {
			line.Format("file read error %d", GetLastError());
			goto ERRRET;
		}

		if ( Send(FIFO_STDOUT, pTemp, len, FIFORCP_TIMEOUT) < 0 ) {
			line.Format("send time over %dmsec", FIFORCP_TIMEOUT);
			goto ERRRET;
		}

		if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL ) {
			if ( m_bAbortFlag ) {
				line.Format("file upload abort");
				goto ERRRET;
			}
			if ( !m_pProgDlg->m_UpdatePost ) {
				m_pProgDlg->m_UpdatePost = TRUE;
				param[0] = stat.st_size - size;
				m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_POS, (LPARAM)param);
			}
		}
	}

	pTemp[0] = 0;
	if ( Send(FIFO_STDOUT, pTemp, 1, FIFORCP_TIMEOUT) < 0 ) {
		line.Format("send time over %dmsec", FIFORCP_TIMEOUT);
		goto ERRRET;
	}

	line.Empty();
	m_nLastError = 0;

ERRRET:
	if ( fp != NULL )
		fclose(fp);

	if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL )
		m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_DESTORY, (LPARAM)(LPCTSTR)MbsToTstr(line));

	SendFdCommand(FIFO_STDOUT, FIFO_CMD_MSGQUEIN, FIFO_QCMD_RCPNEXT, (line.IsEmpty() ? TRUE : FALSE));
}
void CFifoRcp::RcpDownLoad()
{
/*
	write "\0" or "Error\n"

	read "T%llu 0 %llu 0\n"		st_mtime, st_atime
	write "\0" or "Error\n"

	read "D%04o %d %s\n"		st_mode st_size dirname
	write "\0" or "Error\n"

		read "T%llu 0 %llu 0\n"		st_mtime, st_atime
		write "\0" or "Error\n"

		read "C%04o %lld %s\n"		st_mode st_size filename
		write "\0" or "Error\n"
			read file data
		read "\0" or "Error\n"
		write "\0" or "Error\n"

	read "E\n"					??
	write "\0" or "Error\n"

	read "\0"					End
	write "\0" or "Error\n"

	read "\01"					Error
	write "\0" or "Error\n"
*/
	int len;
	CStringA line;
	int st_mode = 0;
	LONGLONG st_size = 0;
	time_t st_mtime = 0, st_atime = 0;
	char name[MAX_PATH];
	CString path;
	FILE *fp = NULL;
	LONGLONG size;
	BYTE *pTemp = TempBuffer(m_nBufSize);
	LONGLONG param[2];
	CStringList PathList;

	m_nLastError = WSAEFAULT;
	PathList.AddHead(m_PathName);

	Write(FIFO_STDOUT, (LPBYTE)"\0", 1);

	for ( ; ; ) {
		if ( !ReadLine(FIFO_STDIN, line) )
			break;

		if ( line.IsEmpty() ) {
			Write(FIFO_STDOUT, (LPBYTE)"\0", 1);
			break;
		}

		switch(line[0]) {
		case '\001':
			Write(FIFO_STDOUT, (LPBYTE)"\0", 1);
			break;
		case 'D':
			Write(FIFO_STDOUT, (LPBYTE)"\0", 1);
			if ( line.GetLength() > sizeof(name) || sscanf(line, "D%o %I64d %s", &st_mode, &st_size, name) != 3 )
				goto ERRRET;
			MakePath(PathList.GetHead(), name, path);
			if ( !CreateDirectory(path, NULL) ) {
				line.Format("create directory error %d", GetLastError());
				goto ERRRET;
			}
			PathList.AddHead(path);
			break;
		case 'E':
			Write(FIFO_STDOUT, (LPBYTE)"\0", 1);
			PathList.RemoveHead();
			break;
		case 'T':
			Write(FIFO_STDOUT, (LPBYTE)"\0", 1);
			if ( sscanf(line, "T%I64d 0 %I64d 0", &st_mtime, &st_atime) != 2 )
				goto ERRRET;
			break;
		case 'C':
			Write(FIFO_STDOUT, (LPBYTE)"\0", 1);
			if ( line.GetLength() > sizeof(name) || sscanf(line, "C%o %I64d %s", &st_mode, &st_size, name) != 3 )
				goto ERRRET;
			MakePath(PathList.GetHead(), name, path);

			if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL )
				m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_FILENAME, (LPARAM)(LPCTSTR)path);

			if ( (fp = _tfopen(path, _T("wb"))) == NULL ) {
				line.Format("file open error %d", GetLastError());
				goto ERRRET;
			}

			if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL ) {
				m_pProgDlg->m_UpdatePost = TRUE;
				param[0] = st_size;
				param[1] = 0;
				m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_RANGE, (LPARAM)param);
			}

			for ( size = st_size ; size > 0 ; size -= len ) {
				len = (size < (LONGLONG)m_nBufSize ? (int)size : m_nBufSize);

				if ( (len = Recv(FIFO_STDIN, pTemp, len, FIFORCP_TIMEOUT)) < 0 ) {
					line.Format("read time over %dmsec", FIFORCP_TIMEOUT);
					goto ERRRET;
				} else if ( len == 0 )
					continue;

				if ( fwrite(pTemp, 1, len, fp) != len ) {
					line.Format("file write error %d", GetLastError());
					goto ERRRET;
				}

				if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL ) {
					if ( m_bAbortFlag ) {
						line.Format("file download abort");
						goto ERRRET;
					}
					if ( !m_pProgDlg->m_UpdatePost ) {
						m_pProgDlg->m_UpdatePost = TRUE;
						param[0] = st_size - size;
						m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_POS, (LPARAM)param);
					}
				}
			}
			fclose(fp);
			fp = NULL;

			if ( !ReadLine(FIFO_STDIN, line) )
				goto ERRRET;
			if ( !line.IsEmpty() )
				goto ERRRET;
			Write(FIFO_STDOUT, (LPBYTE)"\0", 1);

			if ( st_atime != 0 && st_mtime != 0 ) {
				struct _utimbuf utm;
				utm.actime  = st_atime;
				utm.modtime = st_mtime;
				_tutime(path, &utm);
			}

			st_mtime = st_atime = 0;
			st_size = 0;
			break;
		}
	}

	m_nLastError = 0;

ERRRET:
	if ( fp != NULL )
		fclose(fp);

	if ( m_pProgDlg != NULL && m_pProgDlg->m_hWnd != NULL )
		m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_DESTORY, (LPARAM)(LPCTSTR)MbsToTstr(line));
}
