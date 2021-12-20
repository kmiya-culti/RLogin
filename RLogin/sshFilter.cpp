// ssh.cpp: Cssh クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

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
// CFilter
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> filter
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CFilter::OnReceive
//	CChannel::GetSendSize
//		CFilter::GetSendSize (zero)
//
//	filter --> ssh server
//	CFilter::Send (m_Output.Apend)
//		Cssh::SendMsgChannelData (m_Output.Consume)
//	CChannel::GetRecvSize
//		CFilter::GetRecvSize (m_Output.GetSize)

CFilter::CFilter()
{
	m_Type = SSHFT_NONE;
	m_pChan = NULL;
	m_pNext = NULL;
}
CFilter::~CFilter()
{
}
void CFilter::Destroy()
{
	delete this;
}
void CFilter::Close()
{
	m_pChan->DoClose();
}
void CFilter::OnConnect()
{
}
void CFilter::Send(LPBYTE buf, int len)
{
	m_pChan->m_ReadByte += len;
	m_pChan->m_Output.Apend(buf, len);
	m_pChan->m_pSsh->SendMsgChannelData(m_pChan->m_LocalID);
}
void CFilter::OnReceive(const void *lpBuf, int nBufLen)
{
}
void CFilter::OnSendEmpty()
{
}
void CFilter::OnRecvEmpty()
{
}
int CFilter::GetSendSize()
{
	return 0;
}
int CFilter::GetRecvSize()
{
	return m_pChan->m_Output.GetSize();
}

//////////////////////////////////////////////////////////////////////
// CChannel
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> channel socket
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CExtSocket::Send
//				CChannel::OnSendEmpty();
//	CChannel::GetSendSize (CExtSocket::GetSendSize + m_Input.GetSize)
//
//	channel socket --> ssh server
//	CExtSocket::OnReceive
//		CChannel::OnReceiveCallBack
//			CExtSocket::OnReceiveCallBack
//				CChannel::OnReceiveProcBack (m_Output.Apend)
//					Cssh::SendMsgChannelData (m_Output.Consume)
//						CChannel::OnRecvEmpty();
//	CChannel::GetRecvSize (CExtSocket::GetRecvSize + m_Output.GetSize)

CChannel::CChannel():CExtSocket(NULL)
{
	m_Type = ESCT_SSH_CHAN;
	m_pSsh = NULL;
	m_Status = 0;
	m_pNext = NULL;
	m_pFilter = NULL;
}
CChannel::~CChannel()
{
	Close();
}
const CChannel & CChannel::operator = (CChannel &data)
{
	m_Fd			= data.m_Fd;
	m_pDocument     = data.m_pDocument;

	m_Status		= data.m_Status;
	m_RemoteID		= data.m_RemoteID;
	m_LocalID		= data.m_LocalID;
	m_LocalComs		= data.m_LocalComs;
	m_LocalWind		= data.m_LocalWind;
	m_RemoteWind	= data.m_RemoteWind;
	m_RemoteMax		= data.m_RemoteMax;
	m_Input			= data.m_Input;
	m_Output		= data.m_Output;
	m_TypeName		= data.m_TypeName;
	m_Eof			= data.m_Eof;
	m_pNext			= data.m_pNext;
	m_lHost			= data.m_lHost;
	m_lPort			= data.m_lPort;
	m_rHost			= data.m_rHost;
	m_rPort			= data.m_rPort;
	m_ReadByte		= data.m_ReadByte;
	m_WriteByte		= data.m_WriteByte;
	m_ConnectTime	= data.m_ConnectTime;

	return *this;
}
int CChannel::CreateListen(LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress, UINT nRemotePort)
{
	m_lHost	= (lpszHostAddress != NULL ? lpszHostAddress : _T(""));
	m_lPort	= nHostPort;
	m_rHost	= (lpszRemoteAddress != NULL ? lpszRemoteAddress : _T(""));
	m_rPort	= nRemotePort;

	return CExtSocket::Create(lpszHostAddress, nHostPort, lpszRemoteAddress);
}
void CChannel::Close()
{
	m_Status = 0;
	m_Input.Clear();
	m_Output.Clear();
	CExtSocket::Close();

	if ( m_pFilter != NULL ) {
		m_pFilter->m_pChan = NULL;
		m_pFilter->Destroy();
		m_pFilter = NULL;
	}

	((CRLoginApp *)AfxGetApp())->DelIdleProc(IDLEPROC_SOCKET, this);
}
int CChannel::Send(const void *lpBuf, int nBufLen, int nFlags)
{
	m_WriteByte += nBufLen;

	if ( (m_Status & CHAN_REMOTE_SOCKS) != 0 ) {
		m_Output.Apend((LPBYTE)lpBuf, nBufLen);
		m_pSsh->SendMsgChannelData(m_LocalID);
	} else if ( m_pFilter != NULL )
		m_pFilter->OnReceive(lpBuf, nBufLen);
	else
		CExtSocket::Send(lpBuf, nBufLen, nFlags);

	return nBufLen;
}
void CChannel::DoClose()
{
	switch(m_Status & (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE)) {
	case CHAN_OPEN_REMOTE:
		m_pSsh->SendMsgChannelOpenFailure(m_LocalID);
		break;
	case CHAN_OPEN_LOCAL:
		m_Eof |= CEOF_DEAD;
		break;
	case CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE:
		if ( (m_Eof & CEOF_DEAD) == 0 ) {
			m_Eof |= CEOF_DEAD;
			m_pSsh->ChannelCheck(m_LocalID);
		}
		break;
	}
}
void CChannel::OnError(int err)
{
	TRACE("Cannel OnError %d\n", err);
	DoClose();
}
void CChannel::OnClose()
{
	TRACE("Cannel OnClose\n");
	DoClose();
}
void CChannel::OnConnect()
{
	TRACE("Cannel OnConnect\n");
	if ( m_pFilter != NULL )
		m_pFilter->OnConnect();
	else if ( (m_Status & CHAN_OPEN_LOCAL) == 0 )
		m_pSsh->SendMsgChannelOpenConfirmation(m_LocalID);
}
void CChannel::OnAccept(SOCKET hand)
{
	m_pSsh->ChannelAccept(m_LocalID, hand);
}
void CChannel::OnReceiveCallBack(void *lpBuf, int nBufLen, int nFlags)
{
	if ( nFlags != 0 )
		return;

	if ( (m_Status & CHAN_PROXY_SOCKS) != 0 ) {
		m_Output.Apend((LPBYTE)lpBuf, nBufLen);
		m_pSsh->DecodeProxySocks(m_LocalID, m_Output);
	} else
		CExtSocket::OnReceiveCallBack(lpBuf, nBufLen, nFlags);
}
int CChannel::OnReceiveProcBack(void *lpBuf, int nBufLen, int nFlags)
{
	int n;

	if ( m_Status != (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE) )
		return 0;

	if ( (m_Eof & (CEOF_ICLOSE | CEOF_SEOF | CEOF_SCLOSE | CEOF_DEAD)) != 0 )
		return nBufLen;

	if ( (n = m_RemoteWind - m_Output.GetSize()) <= 0 ) {
		m_pSsh->SendMsgChannelData(m_LocalID);
		m_ReadByte += nBufLen;
		return 0;
	} else if ( nBufLen > n )
		nBufLen = n;

//	TRACE("Chanel #%d ProcBack %d\n", m_LocalID, nBufLen);

	m_Output.Apend((LPBYTE)lpBuf, nBufLen);
	m_pSsh->SendMsgChannelData(m_LocalID);
	m_ReadByte += nBufLen;

	return nBufLen;
}
int CChannel::GetSendSize()
{
	if ( m_pFilter != NULL )
		return m_pFilter->GetSendSize();
	else
		return (CExtSocket::GetSendSize() + m_Input.GetSize());
}
int CChannel::GetRecvSize()
{
	if ( m_pFilter != NULL )
		return m_pFilter->GetRecvSize();
	else
		return (CExtSocket::GetRecvSize() + m_Output.GetSize());
}
void CChannel::OnSendEmpty()
{
	// ソケットへのデータ送信が完了
	CExtSocket::OnSendEmpty();

	// チャンネルの受信ウインドウサイズ調整
	m_pSsh->ChannelPolling(m_LocalID);
}
void CChannel::OnRecvEmpty()
{
	// ソケットからの受信データが少なくなった時
	CExtSocket::OnRecvEmpty();
}

//////////////////////////////////////////////////////////////////////
// CStdIoFilter
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> stdio
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CStdIoFilter::OnReceive (m_Input.Apend)
//				CSsh::CExtSocket::OnReceiveCallBack (Display)
//					CSsh::OnRecvEmpty()
//						CStdIoFilter::OnRecvEmpty();
//	CChannel::GetSendSize
//  CSsh::GetRecvSize
//		CStdIoFilter::GetSendSize (CSsh::CExtSocket::GetRecvProcSize)
//
//	stdio --> ssh server
//	CFilter::Send (m_Output.Apend)
//		Cssh::SendMsgChannelData (m_Output.Consume)
//			CStdIoFilter::OnSendEmpty()
//	CChannel::GetRecvSize
//	CSsh::GetSendSize
//		CStdIoFilter::GetRecvSize (m_Output.GetSize)

CStdIoFilter::CStdIoFilter()
{
	m_Type = SSHFT_STDIO;
}
void CStdIoFilter::OnReceive(const void *lpBuf, int nBufLen)
{
	m_pChan->m_pSsh->CExtSocket::OnReceiveCallBack((void *)lpBuf, nBufLen, 0);
}
void CStdIoFilter::OnSendEmpty()
{
	m_pChan->m_pSsh->CExtSocket::OnSendEmpty();
}
void CStdIoFilter::OnRecvEmpty()
{
	m_pChan->m_pSsh->ChannelPolling(m_pChan->m_LocalID);
}
int CStdIoFilter::GetSendSize()
{
	return m_pChan->m_pSsh->CExtSocket::GetRecvProcSize();
}
int CStdIoFilter::GetRecvSize()
{
	return m_pChan->m_Output.GetSize();
}

//////////////////////////////////////////////////////////////////////
// CX11Filter
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> x11
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CX11Filter::OnReceive (m_Buffer.Apend)
//	CChannel::GetSendSize
//		CX11Filter::GetSendSize (m_Buffer.GetSize)
//
//	x11 --> ssh server
//	CX11Filter::Send (m_Output.Apend)
//		Cssh::SendMsgChannelData (m_Output.Consume)
//	CChannel::GetRecvSize
//		CX11Filter::GetRecvSize (CExtSocket::GetRecvSize() + m_Output.GetSize)

CX11Filter::CX11Filter()
{
	m_Type = SSHFT_X11;
}
void CX11Filter::OnConnect()
{
	m_pChan->m_pSsh->SendMsgChannelOpenConfirmation(m_pChan->m_LocalID);
}
int CX11Filter::GetSendSize()
{
	return m_Buffer.GetSize();
}
int CX11Filter::GetRecvSize()
{
	return (m_pChan->CExtSocket::GetRecvSize() + m_pChan->m_Output.GetSize());
}
void CX11Filter::OnReceive(const void *lpBuf, int nBufLen)
{
	int n;
	int name_len, data_len;
	int name_pad, data_pad;
	int over_pos, over_len;
	LPBYTE ptr;
	CBuffer tmp;

	m_Buffer.Apend((LPBYTE)lpBuf, nBufLen);

	if ( m_Buffer.GetSize() < 12 )
		return;

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
		m_pChan->m_pSsh->LogIt(_T("x11 #%d xauth unkown type '%02x'"), m_pChan->m_LocalID, ptr[0]);

		if ( !m_pChan->m_pSsh->m_pDocument->m_ParamTab.m_x11AuthFlag )
			Close();
		else {
			m_pChan->CExtSocket::Send(m_Buffer.GetPtr(), m_Buffer.GetSize(), 0);
			m_pChan->m_pFilter = NULL;
			delete this;
		}
		return;
	}
	
	name_pad = (name_len + 3) & ~0x3;
	data_pad = (data_len + 3) & ~0x3;

	if ( m_Buffer.GetSize() < (12 + name_pad + data_pad) )
		return;

	over_pos = 12 + name_pad + data_pad;
	over_len = m_Buffer.GetSize() - over_pos;

	if ( m_pChan->m_pSsh->m_x11AuthName.GetLength() != name_len || memcmp(TstrToMbs(m_pChan->m_pSsh->m_x11AuthName), ptr + 12, name_len) != 0 ||
		 m_pChan->m_pSsh->m_x11AuthData.GetSize() != data_len   || memcmp(m_pChan->m_pSsh->m_x11AuthData.GetPtr(), ptr + 12 + name_pad, data_len) != 0 ) {

		m_pChan->m_pSsh->LogIt(_T("x11 #%d xauth %s data not match"), m_pChan->m_LocalID, m_pChan->m_pSsh->m_x11AuthName);

		if ( !m_pChan->m_pSsh->m_pDocument->m_ParamTab.m_x11AuthFlag ) {
			Close();
			return;
		}
	}

	name_len = m_pChan->m_pSsh->m_x11LocalName.GetLength();
	data_len = m_pChan->m_pSsh->m_x11LocalData.GetSize();

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
	
	tmp.Apend((LPBYTE)(LPSTR)TstrToMbs(m_pChan->m_pSsh->m_x11LocalName), name_len);
	for ( n = name_len ; n < name_pad ; n++ )
		tmp.Put8Bit(0);

	tmp.Apend(m_pChan->m_pSsh->m_x11LocalData.GetPtr(), data_len);
	for ( n = data_len ; n < data_pad ; n++ )
		tmp.Put8Bit(0);

	if ( over_len > 0 )
		tmp.Apend(ptr + over_pos, over_len);

	m_pChan->CExtSocket::Send(tmp.GetPtr(), tmp.GetSize(), 0);

	m_pChan->m_pFilter = NULL;
	delete this;
}

//////////////////////////////////////////////////////////////////////
// CSFtpFilter
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> sftp
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CSFtpFilter::OnReceive
//				CSFtp::OnReceive (CSFtp::m_RecvBuf.Apend)
//	CChannel::GetSendSize
//		CSFtpFilter::GetSendSize (CSFtp::m_RecvBuf.GetSize)
//
//	sftp --> ssh server
//	CSFtp::Send
//		CFilter::Send (m_Output.Apend)
//			Cssh::SendMsgChannelData (m_Output.Consume)
//	CChannel::GetRecvSize
//		CFilter::GetRecvSize (m_Output.GetSize)

CSFtpFilter::CSFtpFilter()
{
	m_Type = SSHFT_SFTP;
	m_pSFtp = NULL;
}
void CSFtpFilter::Destroy()
{
	if ( m_pSFtp != NULL ) {
		if ( m_pSFtp->m_hWnd != NULL )
			m_pSFtp->DestroyWindow();
		else
			delete m_pSFtp;
	}
	CFilter::Destroy();
}
void CSFtpFilter::OnConnect()
{
	if ( m_pSFtp != NULL )
		m_pSFtp->OnConnect();
}
void CSFtpFilter::OnReceive(const void *lpBuf, int nBufLen)
{
	if ( m_pSFtp != NULL )
		m_pSFtp->OnReceive(lpBuf, nBufLen);
}
int CSFtpFilter::GetSendSize()
{
	if ( m_pSFtp != NULL )
		return m_pSFtp->m_RecvBuf.GetSize();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// CAgent
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> agent
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CAgent::OnReceive (m_RecvBuf.Apend)
//	CChannel::GetSendSize
//		CAgent::GetSendSize (m_RecvBuf.GetSize)
//
//	agent --> ssh server
//	CFilter::Send (m_Output.Apend)
//		Cssh::SendMsgChannelData (m_Output.Consume)
//	CChannel::GetRecvSize
//		CFilter::GetRecvSize (m_Output.GetSize)

CAgent::CAgent()
{
	m_Type = SSHFT_AGENT;
}
CAgent::~CAgent()
{
}
int CAgent::GetSendSize()
{
	return m_RecvBuf.GetSize();
}
void CAgent::OnReceive(const void *lpBuf, int nBufLen)
{
	int n;
	CBuffer tmp;

	m_RecvBuf.Apend((LPBYTE)lpBuf, nBufLen);

	while ( m_RecvBuf.GetSize() >= 4 ) {
		n = m_RecvBuf.PTR32BIT(m_RecvBuf.GetPtr());
		if ( n > (256 * 1024) || n < 0 )
			throw _T("agent packet lenght error");
		if ( m_RecvBuf.GetSize() < (n + 4) )
			break;
		tmp.Clear();
		tmp.Apend(m_RecvBuf.GetPtr() + 4, n);
		m_RecvBuf.Consume(n + 4);
		ReceiveBuffer(&tmp);
	}
}
BOOL CAgent::IsLocked()
{
	if ( m_pChan->m_pSsh->m_pAgentMutex != NULL )
		return FALSE;	// Locked this

	CMutex Mutex(FALSE, _T("RLogin_SSH2_AGENT"), NULL);

	if ( GetLastError() == ERROR_ALREADY_EXISTS )
		return TRUE;

	return FALSE;
}
LPCTSTR CAgent::GetRsaSignAlg(CIdKey *key, int flag)
{
	if ( key == NULL || key->m_Type != IDKEY_RSA2 )
		return NULL;

	switch(flag & (SSH_AGENT_RSA_SHA2_256 | SSH_AGENT_RSA_SHA2_512)) {
	case SSH_AGENT_RSA_SHA2_256:
		return _T("rsa-sha2-256");
	case SSH_AGENT_RSA_SHA2_512:
		return _T("rsa-sha2-512");
	case SSH_AGENT_RSA_SHA2_256 | SSH_AGENT_RSA_SHA2_512:
		return (m_pChan->m_pDocument->m_ParamTab.m_RsaExt == 2 ? _T("rsa-sha2-512") : _T("rsa-sha2-256"));
	}

	return NULL;
}
CIdKey *CAgent::GetIdKey(CIdKey *key, LPCTSTR pass)
{
	int n;

	for ( n = 0 ; n < m_pChan->m_pSsh->m_IdKeyTab.GetSize() ; n++ ) {
		if ( m_pChan->m_pSsh->m_IdKeyTab[n].Compere(key) == 0 ) {
			if ( pass == NULL || m_pChan->m_pSsh->m_IdKeyTab[n].InitPass(pass) )
				return &(m_pChan->m_pSsh->m_IdKeyTab[n]);
		}
	}

	return NULL;
}
void CAgent::ReceiveBuffer(CBuffer *bp)
{
	CBuffer tmp;
	int type = bp->Get8Bit();

	try {
		switch(type) {
		case SSH_AGENTC_REQUEST_IDENTITIES:
			{
				int n, i = 0;
				CIdKey *pKey;
				CBuffer data, work;

				for ( n = 0 ; n < m_pChan->m_pSsh->m_IdKeyTab.GetSize() ; n++ ) {
					pKey = &(m_pChan->m_pSsh->m_IdKeyTab[n]);
					switch(pKey->m_Type) {
					case IDKEY_RSA1:
						{
							BIGNUM const *e = NULL, *n = NULL;

							RSA_get0_key(pKey->m_Rsa, &n, &e, NULL);

							data.Put32Bit(RSA_bits(pKey->m_Rsa));
							data.PutBIGNUM(e);
							data.PutBIGNUM(n);
							data.PutStr(TstrToMbs(pKey->m_Name));
						}
						i++;
						break;
					case IDKEY_RSA2:
					case IDKEY_DSA2:
					case IDKEY_ECDSA:
					case IDKEY_ED25519:
						work.Clear();
						pKey->SetBlob(&work);
						data.PutBuf(work.GetPtr(), work.GetSize());
						data.PutStr(TstrToMbs(pKey->m_Name));
						i++;
						break;
					}
				}
				work.Clear();
				work.Put8Bit(SSH_AGENT_IDENTITIES_ANSWER);
				work.Put32Bit(i);
				work.Apend(data.GetPtr(), data.GetSize());

				tmp.Put32Bit(work.GetSize());
				tmp.Apend(work.GetPtr(), work.GetSize());
				Send(tmp.GetPtr(), tmp.GetSize());
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
				if ( (pkey = GetIdKey(&key, m_pChan->m_pDocument->m_ServerEntry.m_PassName)) != NULL && pkey->Sign(&sig, data.GetPtr(), data.GetSize(), GetRsaSignAlg(pkey, flag)) ) {
					data.Clear();
					data.Put8Bit(SSH_AGENT_SIGN_RESPONSE);
					data.PutBuf(sig.GetPtr(), sig.GetSize());
					tmp.Put32Bit(data.GetSize());
					tmp.Apend(data.GetPtr(), data.GetSize());
					Send(tmp.GetPtr(), tmp.GetSize());
				} else {
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_FAILURE);
					Send(tmp.GetPtr(), tmp.GetSize());
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
					Send(tmp.GetPtr(), tmp.GetSize());

					if ( key.m_Cert != 0 ) {
						for ( n = 0 ; n < m_pChan->m_pSsh->m_IdKeyTab.GetSize() ; n++ ) {
							pkey = &(m_pChan->m_pSsh->m_IdKeyTab[n]);
							if ( pkey->m_Cert == 0 && pkey->ComperePublic(&key) == 0 ) {
								pkey->m_Cert = key.m_Cert;
								pkey->m_CertBlob = key.m_CertBlob;
								((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab.UpdateUid(pkey->m_Uid);
							}
						}
					} else {
						dlg.m_OpenMode = 3;
						dlg.m_Title.LoadString(IDS_IDKEYFILELOAD);;
						dlg.m_Message.LoadString(IDS_IDKEYCREATECOM);

						if ( dlg.DoModal() == IDOK ) {
							key.WritePrivateKey(key.m_SecBlob, dlg.m_PassName);
							if ( ((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab.AddEntry(key) )
								m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.AddVal(key.m_Uid);
						}
					}
				} else {
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_FAILURE);
					Send(tmp.GetPtr(), tmp.GetSize());
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
					for ( n = 0 ; n < m_pChan->m_pSsh->m_IdKeyTab.GetSize() ; n++ ) {
						if ( pKey->m_Uid == m_pChan->m_pSsh->m_IdKeyTab[n].m_Uid ) {
							m_pChan->m_pSsh->m_IdKeyTab.RemoveAt(n);
							break;
						}
					}
					for ( n = 0 ; n < m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.GetSize() ; n++ ) {
						if ( pKey->m_Uid == m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.GetVal(n) ) {
							m_pChan->m_pDocument->m_ParamTab.m_IdKeyList.RemoveAt(n);
							break;
						}
					}
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_SUCCESS);
					Send(tmp.GetPtr(), tmp.GetSize());
				} else {
					tmp.Put32Bit(1);
					tmp.Put8Bit(SSH_AGENT_FAILURE);
					Send(tmp.GetPtr(), tmp.GetSize());
				}
			}
			break;

		case SSH_AGENTC_LOCK:
			{
				CStringA mbs;
				bp->GetStr(mbs);

				tmp.Put32Bit(1);

				if ( m_pChan->m_pSsh->m_pAgentMutex != NULL ) {
					tmp.Put8Bit(m_pChan->m_pSsh->m_AgentPass.Compare(mbs) == 0 ? SSH_AGENT_SUCCESS : SSH_AGENT_FAILURE);
				} else {
					m_pChan->m_pSsh->m_pAgentMutex = new CMutex(TRUE, _T("RLogin_SSH2_AGENT"), NULL);
					if ( m_pChan->m_pSsh->m_pAgentMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS ) {
						if ( m_pChan->m_pSsh->m_pAgentMutex != NULL )
							delete m_pChan->m_pSsh->m_pAgentMutex;
						m_pChan->m_pSsh->m_pAgentMutex = NULL;
						tmp.Put8Bit(SSH_AGENT_FAILURE);
					 } else {
						tmp.Put8Bit(SSH_AGENT_SUCCESS);
						m_pChan->m_pSsh->m_AgentPass = mbs;
					 }
				}

				Send(tmp.GetPtr(), tmp.GetSize());
			}
			break;
		case SSH_AGENTC_UNLOCK:
			{
				CStringA mbs;
				bp->GetStr(mbs);

				tmp.Put32Bit(1);

				if ( m_pChan->m_pSsh->m_pAgentMutex == NULL || m_pChan->m_pSsh->m_AgentPass.Compare(mbs) != 0 ) {
					tmp.Put8Bit(SSH_AGENT_FAILURE);
				} else {
					delete m_pChan->m_pSsh->m_pAgentMutex;
					m_pChan->m_pSsh->m_pAgentMutex = NULL;
					tmp.Put8Bit(SSH_AGENT_SUCCESS);
				}

				Send(tmp.GetPtr(), tmp.GetSize());
			}
			break;

		default:
			tmp.Put32Bit(1);
			tmp.Put8Bit(SSH_AGENT_FAILURE);
			Send(tmp.GetPtr(), tmp.GetSize());
			break;
		}

	} catch(...) {
		tmp.Clear();
		tmp.Put32Bit(1);
		tmp.Put8Bit(SSH_AGENT_FAILURE);
		Send(tmp.GetPtr(), tmp.GetSize());
	}
}

//////////////////////////////////////////////////////////////////////
// CRcpUpload
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> rcp
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CRcpUpload::OnReceive
//	CChannel::GetSendSize
//		CFilter::GetSendSize (zero)
//
//	rcp --> ssh server
//	CFilter::Send (m_Output.Apend)
//		Cssh::SendMsgChannelData (m_Output.Consume)
//	Cssh::OnSendEmpty()
//		CRcpUpload::OnSendEmpty()
//	CChannel::GetRecvSize
//		CFilter::GetRecvSize (m_Output.GetSize)

CRcpUpload::CRcpUpload()
{
	m_Type = SSHFT_RCP;
	m_Fd   = NULL;
	m_Mode = 0;
}
CRcpUpload::~CRcpUpload()
{
}
void CRcpUpload::Close()
{
	if ( m_ProgDlg.m_hWnd != NULL )
		m_ProgDlg.DestroyWindow();

	if ( m_Fd != NULL )
		fclose(m_Fd);
	m_Fd = NULL;

	CFilter *fp;
	if ( (fp = m_pSsh->m_pListFilter) == this )
		m_pSsh->m_pListFilter = m_pNext;
	else {
		if ( fp->m_pNext == this )
			fp->m_pNext = m_pNext;
	}
	m_pNext = NULL;

	CFilter::Close();
}
void CRcpUpload::OnSendEmpty()
{
	int n;
	BYTE tmp[4096];

	if ( m_Mode != 3 || m_pChan == NULL )
		return;

	if ( (n = (int)(m_Stat.st_size - m_Size)) <= 0 ) {
		m_Mode = 5;
		tmp[0] = 0;
		Send(tmp, 1);
		return;
	}

	if ( m_ProgDlg.m_AbortFlag ) {
		Close();
		return;
	}

	while ( m_pChan->m_Output.GetSize() < (CHAN_SES_PACKET_DEFAULT * 4) ) {
		if ( (n = (int)fread(tmp, 1, 4096, m_Fd)) <= 0 )
			break;

		m_pChan->m_ReadByte += n;
		m_pChan->m_Output.Apend(tmp, n);
		m_Size += n;
	}
	m_pChan->m_pSsh->SendMsgChannelData(m_pChan->m_LocalID);

	m_ProgDlg.SetPos(m_Size);
}
void CRcpUpload::OnReceive(const void *lpBuf, int nBufLen)
{
	int c;
	LPBYTE p = (LPBYTE)lpBuf;
	LPBYTE e = (LPBYTE)lpBuf + nBufLen;
	CString str;
	CStringA tmp;

	while ( p < e ) {
		c = *(p++);
		switch(m_Mode) {
		case 0:
			if ( c != '\0' ) {
				Close();
				return;
			}
			tmp.Format("T%I64u 0 %I64u 0\n", m_Stat.st_mtime, m_Stat.st_atime);
			Send((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
			m_Mode = 1;
			break;
		case 1:
			if ( c != '\0' ) {
				Close();
				return;
			}
			str.Format(_T("C%04o %I64d %s\n"), (m_Stat.st_mode & 0777), m_Stat.st_size, (LPCTSTR)m_File);
			tmp = m_pSsh->m_pDocument->RemoteStr(str);
			Send((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
			m_Size = 0;
			m_Mode = 2;
			break;
		case 2:
			if ( c != '\0' ) {
				Close();
				return;
			}
			m_ProgDlg.Create(IDD_PROGDLG, AfxGetMainWnd());
			m_ProgDlg.ShowWindow(SW_SHOW);
			m_ProgDlg.SetFileName(m_Path);
			m_ProgDlg.SetRange(m_Stat.st_size, 0);

			m_Mode = 3;
			OnSendEmpty();
			break;
		case 5:
			m_ProgDlg.SetPos(m_Size);
			Close();
			return;
		}
	}
}
void CRcpUpload::Destroy()
{
	if ( m_FileList.IsEmpty() || m_pChan != NULL )
		return;

	LPCTSTR p;
	CKeyNode fmt;
	CStringW cmd;

	m_Mode = 0;
	m_Path = m_FileList.RemoveHead();
	if ( (p = _tcsrchr(m_Path, _T('\\'))) != NULL || (p = _tcsrchr(m_Path, _T(':'))) != NULL )
		m_File = p + 1;
	else
		m_File = m_Path;

	if ( _tstati64(m_Path, &m_Stat) || (m_Stat.st_mode & _S_IFMT) == _S_IFDIR )
		return;

	if ( (m_Fd = _tfopen(m_Path, _T("rb"))) == NULL )
		return;

	fmt.SetMaps(m_pSsh->m_pDocument->m_TextRam.m_DropFileCmd[5]);
	fmt.CommandLine(TstrToUni(m_File), cmd);
	m_Cmd = m_pSsh->m_pDocument->RemoteStr(UniToTstr(cmd));
	m_pSsh->OpenRcpUpload(NULL);
}

//////////////////////////////////////////////////////////////////////
// CRcpDownload
//////////////////////////////////////////////////////////////////////
//
//	ssh server --> rcp
//	SSH2MsgChannelData (SSH2_MSG_CHANNEL_DATA or SSH2_MSG_CHANNEL_EXTENDED_DATA)
//		CChannel::Send
//			CRcpDownload::OnReceive
//	CChannel::GetSendSize
//		CFilter::GetSendSize (zero)
//
//	rcp --> ssh server
//	CFilter::Send (m_Output.Apend)
//		Cssh::SendMsgChannelData (m_Output.Consume)
//	CChannel::GetRecvSize
//		CFilter::GetRecvSize (m_Output.GetSize)

CRcpDownload::CRcpDownload()
{
	m_Type = SSHFT_RCP;
	m_Mode = 0;
	m_Fd   = NULL;
	m_atime = m_mtime = 0;
}
CRcpDownload::~CRcpDownload()
{
	if ( m_Fd != NULL )
		fclose(m_Fd);
}
void CRcpDownload::DispMsg(LPCTSTR msg)
{
	LPCSTR mbs = m_pSsh->m_pDocument->RemoteStr(msg);
	m_pSsh->CExtSocket::OnReceiveCallBack((void *)mbs, (int)strlen(mbs), 0);
}
void CRcpDownload::DispInit()
{
	int n;
	int cx, cy, sx, sy;

	m_pSsh->m_pDocument->m_TextRam.GetScreenSize(&cx, &cy, &sx, &sy);

	if ( cx > 120 )
		cx -= 40;
	else if ( cx > 80 )
		cx -= 30;
	else if ( cx > 60 )
		cx -= 20;
	else if ( cx > 40 )
		cx -= 10;
	else if ( cx > 10 )
		cx -= 5;

	for ( n = 10 ; n > 2 && (cx / n) < 5 ; )
		n -= 2;

	m_DivCols = cx / n;
	m_MaxCols = m_DivCols * n - 1;
	m_StartClock = clock();

	DispPos();
}
void CRcpDownload::DispPos()
{
	int n, mx, msec;
	CString msg;
	CString left, mid, right;

	if ( m_Size <= 0 )
		mx = m_MaxCols;
	else
		mx = (int)(m_ReadSize * m_MaxCols / m_Size);

	for ( n = 0 ; n < m_MaxCols ; n++ )
		msg += (((n % m_DivCols) + 1) == m_DivCols ? _T('|') : _T('-'));

	if ( m_MaxCols > 10 && m_Size > 0 ) {
		if ( (msec = clock() - m_StartClock) > 100 ) {
			n = (int)((m_Size - m_ReadSize) / (m_ReadSize * CLOCKS_PER_SEC / (LONGLONG)msec));

			if ( n >= 3600 )
				mid.Format(_T("%d:%02d:%02d"), n / 3600, (n % 3600) / 60, n % 60);
			else if ( n >= 60 )
				mid.Format(_T("%d:%02d"), n / 60, n % 60);
			else if ( n > 0 )
				mid.Format(_T("%dsec"), n);

			else {
				n = (int)(m_ReadSize * CLOCKS_PER_SEC / (LONGLONG)msec);

				if ( n >= 10000000 )
					mid.Format(_T("%dMb/s"), n / 1000000);
				else if ( n >= 10000 )
					mid.Format(_T("%dKb/s"), n / 1000);
				else
					mid.Format(_T("%db/s"), n);
			}
		} else
			mid.Format(_T("%d%%"), (int)(m_ReadSize * 100 / m_Size));

		left = msg.Left((msg.GetLength() - mid.GetLength()) / 2);
		right = msg.Right(msg.GetLength() - left.GetLength() - mid.GetLength());
		msg = left;
		msg += mid;
		msg += right;
	}

	mid = _T("\r[\x1B[7m");
	mid += msg.Left(mx);
	mid += _T("\x1B[m");
	mid += msg.Right(msg.GetLength() - mx);
	mid += _T("]");

	DispMsg(mid);
}
void CRcpDownload::PutOkMsg()
{
	Send((LPBYTE)"", 1);
}
void CRcpDownload::PutErrorMsg(LPCSTR msg)
{
	Send((LPBYTE)(LPCSTR)msg, (int)strlen(msg));
}
void CRcpDownload::OnConnect()
{
	PutOkMsg();

	// ウィンドウをディレイクローズ
	m_pSsh->m_pDocument->m_bExitPause = TRUE;
}
void CRcpDownload::OnReceive(const void *lpBuf, int nBufLen)
{
	// read	"D%04o %d %.1024s\n"
	// write "\0" or "Error\n"

	//  read "T%llu 0 %llu 0\n" st_mtime, st_atime
	//	read "C%04o %lld %s\n"
	//	write "\0" or "Error\n"
	//	  read file data
	//	read "\0" or "Error\n"
	//	write "\0" or "Error\n"

	// read "E\n"
	// write "\0" or "Error\n"

	int n;
	CString work;
	LPCSTR p = (LPCSTR)lpBuf;
	LPCSTR e = (LPCSTR)lpBuf + nBufLen;

	while ( p < e ) {
		switch(m_Mode) {
		case 0:	// C
			m_Req = (int)*(p++);
			switch(m_Req) {
			case '\0':
				break;
			case 'C':
			case 'D':
				m_Flag = 0;
				m_Size = 0;
				m_StrMsg.Empty();
				m_Mode = 1;
				break;
			default:
				m_StrMsg.Empty();
				m_Mode = 3;
				break;
			}
			break;

		case 1:	// %04o
			if ( *p >= '0' && *p <= '7' ) {
				m_Flag = m_Flag * 8 + (*(p++) - '0');
				break;
			}
			p++;
			m_Mode = 2;
			break;
		case 2:	// %lld
			if ( *p >= '0' && *p <= '9' ) {
				m_Size = m_Size * 10 + (*(p++) - '0');
				break;
			}
			p++;
			m_Mode = 3;
			break;

		case 3:	// %s\n
			if ( *p != '\n' ) {
				m_StrMsg += (*p++);
				break;
			}
			p++;

			work = m_pSsh->m_pDocument->LocalStr(m_StrMsg);

			if ( m_Req == 'E' ) {
				if ( !m_DirPath.IsEmpty() )
					m_DirPath.RemoveHead();
				PutOkMsg();
				m_Mode = 0;
				break;
			} else if ( m_Req == 'T' ) {
				sscanf(m_StrMsg, "%I64u 0 %I64u 0", &m_mtime, &m_atime);
				PutOkMsg();
				m_Mode = 0;
				break;
			} else if ( m_Req == '\001' ) {
				work += _T("\r\n");
				DispMsg(work);
				PutErrorMsg("Error abort\n");
				m_Mode = 0;
				break;
			} else if ( m_Req != 'C' && m_Req != 'D' ) {
				CString tmp;
				tmp.Format(_T("%c%s: unkown command\r\n"), (m_Req < ' ' || m_Req >= 0x7e ? '?' : m_Req), (LPCTSTR)work);
				DispMsg(tmp);
				PutErrorMsg("Unkown command\n");
				m_Mode = 0;
				break;
			}

			if ( (n = work.Find(_T('/'))) >= 0 )
				work.Delete(0, n + 1);

			DispMsg(work);

			if ( m_DirPath.IsEmpty() ) {
				LPCTSTR ext;
				if ( (n = work.ReverseFind(_T('.'))) >= 0 )
					ext = (LPCTSTR)work + n;
				else
					ext = _T(".");
				CFileDialog dlg(FALSE, ext + 1, work, OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), ::AfxGetMainWnd());
				if ( DpiAwareDoModal(dlg) != IDOK ) {
					PutErrorMsg("Cancel file\n");
					DispMsg(_T(": cancel\r\n"));
					break;
				}
				m_PathName = dlg.GetPathName();
			} else {
				m_PathName.Format(_T("%s\\%s"), (LPCTSTR)(m_DirPath.GetHead()), (LPCTSTR)work);
			}
				
			if ( m_Req == 'C' ) {
				if ( (m_Fd = _tfopen(m_PathName, _T("wb"))) == NULL ) {
					PutErrorMsg("Can't open file\n");
					DispMsg(_T(": open error\r\n"));
					break;
				}
				m_ReadSize = 0;
				m_bHaveErr = FALSE;
				DispMsg(_T(": get\r\n"));
				DispInit();
				m_Mode = 4;

			} else if ( m_Req == 'D' ) {
				if ( !CreateDirectory(m_PathName, NULL) ) {
					PutErrorMsg("Can't Create directory\n");
					DispMsg(_T(": mkdir error\r\n"));
					break;
				}
				m_DirPath.AddHead(m_PathName);
				DispMsg(_T(": mkdir\r\n"));
				m_Mode = 0;
			}

			PutOkMsg();
			break;

		case 4:	// File Data
			if ( (n = (int)(e - p)) <= 0 )
				break;
			if ( (LONGLONG)n > (m_Size - m_ReadSize) )
				n = (int)(m_Size - m_ReadSize);

			if ( !m_bHaveErr && (int)fwrite(p, 1, n, m_Fd) != n )
				m_bHaveErr = TRUE;

			p += n;
			m_ReadSize += n;
			DispPos();
			if ( m_ReadSize < m_Size )
				break;

			m_StrMsg.Empty();
			if ( ferror(m_Fd) )
				m_bHaveErr = TRUE;
			fclose(m_Fd);
			m_Fd = NULL;
			m_Mode = 5;
			break;

		case 5:	// File I/O Status
			if ( !m_StrMsg.IsEmpty() ) {
				if ( *p != '\n' ) {
					m_StrMsg += *(p++);
					break;
				}
			} else if ( *p != '\0' ) {
				m_StrMsg += *(p++);
				break;
			}
			p++;

			if ( m_StrMsg.IsEmpty() && !m_bHaveErr ) {
				if ( m_atime != 0 && m_mtime != 0 ) {
					struct _utimbuf utm;
					utm.actime  = m_atime;
					utm.modtime = m_mtime;
					_tutime(m_PathName, &utm);
				}
				DispMsg(_T(" ok\r\n"));
				PutOkMsg();
			} else {
				if ( m_StrMsg.IsEmpty() )
					DispMsg(_T(" bad\r\n"));
				else {
					work = m_pSsh->m_pDocument->LocalStr(m_StrMsg);
					work += _T("\r\n");
					DispMsg(work);
				}
				PutErrorMsg("File Write Error\n");
			}

			m_atime = m_mtime = 0;
			m_Mode = 0;
			break;
		}
	}
}
