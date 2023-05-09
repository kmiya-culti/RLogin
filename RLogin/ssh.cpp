//////////////////////////////////////////////////////////////////////
// ssh.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "PassDlg.h"
#include "EditDlg.h"
#include "IdkeySelDLg.h"
#include "IdKeyFileDlg.h"
#include "ssh.h"

#include "ssh1.h"
#include "ssh2.h"

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
// CFifoSsh

/*
CFifoSocket		CFifoSsh										CFifoStdio
	STDOUT		STDIN->OnRecvSocket	   ChannelCheck->EXTOUT		STDIN->OnRecvDocument->CDocument::OnSocketReceive
	STDINT		STDOUT<-SendSocket		ChannelCheck<-EXTIN		STDOUT<-SendProtocol<-CDocument::SocketSend
								      ChennelCheck->ChanOut		STDIN->SendMsgChannelData
								       ChennelCheck->ChanIn		STDOUT<-SSH2MsgChannelData 
*/

CFifoSsh::CFifoSsh(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoThread(pDoc, pSock)
{
}
int CFifoSsh::GetFreeFifo(int nFd)
{
	for ( ; nFd < GetFifoSize() ; nFd += 2 ) {
		if ( m_FifoBuf[nFd] == NULL && m_FifoBuf[nFd + 1] == NULL )
			return nFd;
	}

	return nFd;
}
void CFifoSsh::EndOfData(int nFd)
{
	SendFdCommand(FIFO_EXTOUT, FIFO_CMD_MSGQUEIN, FIFO_QCMD_ENDOFDATA, m_nLastError);
}
void CFifoSsh::OnRead(int nFd)
{
	int readlen;
	BYTE *buf = TempBuffer(m_nBufSize);

	ASSERT((nFd & 1) == 0);

	if ( nFd == FIFO_STDIN ) {
		while ( !m_bDestroy ) {
			if ( (readlen = Read(nFd, buf, m_nBufSize)) < 0 ) {
				// nFd End of data
				ResetFdEvents(nFd, FD_READ);
				EndOfData(nFd);
				break;
			} else if ( readlen == 0 ) {
				// nFd No data
				break;
			} else {
				// Call Process
				m_pSock->OnRecvSocket(buf, readlen, 0);
			}
		}
	} else
		((Cssh *)m_pSock)->ChannelCheck(nFd, FD_READ, NULL);
}
void CFifoSsh::OnWrite(int nFd)
{
	ASSERT((nFd & 1) != 0);

	if ( nFd == FIFO_STDOUT )
		ResetFdEvents(nFd, FD_WRITE);
	else
		((Cssh *)m_pSock)->ChannelCheck((nFd & ~1), FD_WRITE, NULL);	// EXTOUT->EXTIN 注意
}
void CFifoSsh::OnOob(int nFd, int len, BYTE *pBuffer)
{
	ResetFdEvents(nFd, FD_OOB);
}
void CFifoSsh::OnAccept(int nFd, SOCKET socket)
{
	if ( nFd == FIFO_STDIN )
		m_pSock->OnAccept(socket);
	else
		((Cssh *)m_pSock)->ChannelCheck(nFd, FD_ACCEPT, (void *)socket);
}
void CFifoSsh::OnConnect(int nFd)
{
	if ( nFd == FIFO_STDIN )
		m_pSock->OnConnect();
	else
		((Cssh *)m_pSock)->ChannelCheck(nFd, FD_CONNECT, NULL);
}
void CFifoSsh::OnClose(int nFd, int nLastError)
{
	if ( nFd == FIFO_STDIN )
		m_nLastError = nLastError;
	else
		((Cssh *)m_pSock)->ChannelCheck(nFd, FD_CLOSE, (void *)(UINT_PTR)nLastError);
}
void CFifoSsh::OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	if ( cmd != FIFO_CMD_MSGQUEIN )
		return;

	switch(param) {
	case FIFO_QCMD_SYNCRET:
		((Cssh *)m_pSock)->ChannelCheck(((Cssh *)m_pSock)->IdToFdOut(msg), FD_ACCEPT, buf);	// FdOut !!
		break;
	case FIFO_QCMD_RCPNEXT:
		if ( msg == TRUE )
			((Cssh *)m_pSock)->OpenRcpUpload(NULL);
		else
			((Cssh *)m_pSock)->m_FileList.RemoveAll();
		break;
	case FIFO_QCMD_SENDPACKET:
		((Cssh *)m_pSock)->FifoCmdSendPacket(msg, (CBuffer *)buf);
		break;
	case FIFO_QCMD_KEEPALIVE:
		((Cssh *)m_pSock)->SendMsgKeepAlive();
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// Cssh

Cssh::Cssh(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = ESCT_SSH_MAIN;
	m_SaveDh = NULL;
	m_EcdhClientKey = NULL;
	m_SSHVer = 0;
	m_InPackStat = 0;
	m_KeepAliveTiimerId = 0;
	m_KeepAliveSendCount = m_KeepAliveReplyCount = 0;
	m_KeepAliveRecvGlobalCount = m_KeepAliveRecvChannelCount = 0;
	m_CurveEvpKey = NULL;
	m_CurveClientPubkey.m_bZero = TRUE;
	m_RsaHostBlob.m_bZero = TRUE;
	m_RsaTranBlob.m_bZero = TRUE;
	m_RsaSharedKey.m_bZero = TRUE;
	m_RsaOaepSecret.m_bZero = TRUE;
	m_SessHash.m_bZero = TRUE;
	m_SntrupClientKey.m_bZero = TRUE;
	m_LastFreeId = 2;

	for ( int n = 0 ; n < 6 ; n++ )
		m_VKey[n] = NULL;

	m_pAgentMutex = NULL;
}
Cssh::~Cssh()
{
	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);

	for ( int n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
	}

	if ( m_KeepAliveTiimerId != 0 )
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this, m_KeepAliveTiimerId);

	if ( m_pAgentMutex != NULL )
		delete m_pAgentMutex;

	if ( m_CurveEvpKey != NULL )
		EVP_PKEY_free(m_CurveEvpKey);

	while ( m_FifoCannel.GetSize() > 0 ) {
		if ( m_FifoCannel[0] != NULL )
			delete (CFifoChannel *)m_FifoCannel[0];
		m_FifoCannel.RemoveAt(0);
	}
}

//////////////////////////////////////////////////////////////////////

CFifoBase *Cssh::FifoLinkMid()
{
	return new CFifoSsh(m_pDocument, this);
}
CFifoDocument *Cssh::FifoLinkRight()
{
	return new CFifoStdio(m_pDocument, this);
}
int Cssh::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
	try {
		int n, i;
		CString str;
		CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
		CIdKey IdKey, *pKey;
		CDWordArray list[2];

		m_SSHVer = 0;
		m_ServerVerStr = _T("");
		m_ClientVerStr = _T("");
		m_InPackStat = 0;
		m_PacketStat = 0;
		m_SSH2Status = 0;
		m_SendPackSeq = m_SendPackLen = 0;
		m_RecvPackSeq = m_RecvPackLen = 0;
		m_CipherMode = SSH_CIPHER_NONE;
		m_SessionIdLen = 0;
		m_AuthStat = AST_START;
		m_AuthMode = AUTH_MODE_NONE;
		m_bKeybIntrReq = FALSE;
		m_AuthMeta.Empty();
		m_AuthLog.Empty();
		m_HostName = m_pDocument->m_ServerEntry.m_HostName;
		m_HostPort = nHostPort;
		m_DhMode = DHMODE_GROUP_1;
		m_GlbReqMap.RemoveAll();
		m_ChnReqMap.RemoveAll();
		m_OpenChanCount = 0;
		m_Permit.RemoveAll();
		m_MyPeer.Clear();
		m_HisPeer.Clear();
		m_SessHash.Clear();
		m_Incom.Clear();
		m_InPackBuf.Clear();
		m_HostKey.Close();
		m_pIdKey = NULL;
		m_IdKeyTab.RemoveAll();
		m_IdKeyPos = 0;
		SetRecvBufSize(CHAN_SES_PACKET_DEFAULT);
		srand((UINT)time(NULL));
		m_bPfdConnect = 0;
		m_bExtInfo = FALSE;
		m_ExtInfo.RemoveAll();
		m_DhGexReqBits = 4096;
		m_bKnownHostUpdate = TRUE;
		m_bReqRsaSha1 = FALSE;

		m_KeepAliveSendCount = m_KeepAliveReplyCount = 0;
		m_KeepAliveRecvGlobalCount = m_KeepAliveRecvChannelCount = 0;
		m_bAuthAgentReqEnable = FALSE;
		m_PortFwdTable.RemoveAll();

		m_SSPI_phContext = NULL;
		ZeroMemory(&m_SSPI_hCredential, sizeof(m_SSPI_hCredential));
		ZeroMemory(&m_SSPI_hNewContext, sizeof(m_SSPI_hNewContext));
		ZeroMemory(&m_SSPI_tsExpiry, sizeof(m_SSPI_tsExpiry));

		if ( m_pAgentMutex != NULL ) {
			delete m_pAgentMutex;
			m_pAgentMutex = NULL;
		}

		if ( !m_pDocument->m_ServerEntry.m_IdkeyName.IsEmpty() ) {
			CString fileName(m_pDocument->m_ServerEntry.m_IdkeyName);
			m_pDocument->EntryText(fileName);
			if ( !IdKey.LoadPrivateKey(fileName, m_pDocument->m_ServerEntry.m_PassName) ) {
				CIdKeyFileDlg dlg;
				dlg.m_OpenMode  = IDKFDMODE_OPEN;
				dlg.m_Title.LoadString(IDS_SSH_PASS_TITLE);		// = _T("SSH鍵ファイルの読み込み");
				dlg.m_Message.LoadString(IDS_SSH_PASS_MSG);		// = _T("作成時に設定したパスフレーズを入力してください");
				if ( !IdKey.m_LoadMsg.IsEmpty() ) {
					dlg.m_Message += _T("\n\n");
					dlg.m_Message += IdKey.m_LoadMsg;
				}
				dlg.m_IdkeyFile = fileName;
				if ( dlg.DoModal() != IDOK )
					return FALSE;
				if ( !IdKey.LoadPrivateKey(dlg.m_IdkeyFile, dlg.m_PassName) ) {
					AfxMessageBox(CStringLoad(IDE_IDKEYLOADERROR));
					return FALSE;
				}
			}
			if ( IdKey.IsNotSupport() ) {
				AfxMessageBox(CStringLoad(IDE_IDKEYNOTSUPPORT));
				return FALSE;
			}
			IdKey.SetPass(m_pDocument->m_ServerEntry.m_PassName);
			m_IdKeyTab.Add(IdKey);
		}

		if ( pMain->AgeantInit() && !m_pDocument->m_ParamTab.m_bInitPageant && AfxMessageBox(CStringLoad(IDS_ADDPAGEANTENTRY), MB_ICONQUESTION | MB_YESNO) == IDYES ) {
			CIdkeySelDLg dlg;

			dlg.m_pParamTab = &(m_pDocument->m_ParamTab);
			dlg.m_pIdKeyTab = &(((CMainFrame *)AfxGetMainWnd())->m_IdKeyTab);
			dlg.m_IdKeyList = m_pDocument->m_ParamTab.m_IdKeyList;

			if ( dlg.DoModal() == IDOK ) {
				m_pDocument->m_ParamTab.m_IdKeyList = dlg.m_IdKeyList;
				m_pDocument->SetModifiedFlag(TRUE);
			}
		}

		for ( n = 0 ; n < m_pDocument->m_ParamTab.m_IdKeyList.GetSize() ; n++ ) {
			if ( (pKey = pMain->m_IdKeyTab.GetUid(m_pDocument->m_ParamTab.m_IdKeyList.GetVal(n))) == NULL || pKey->m_Type == IDKEY_NONE || (pKey->m_AgeantType != IDKEY_AGEANT_NONE && !pKey->m_bSecInit) )
				continue;
			else if ( pKey->m_Type == IDKEY_UNKNOWN && pKey->m_AgeantType == IDKEY_AGEANT_NONE )
				continue;

			list[pKey->CompPass(m_pDocument->m_ServerEntry.m_PassName) == 0 ? 0 : 1].Add(m_pDocument->m_ParamTab.m_IdKeyList.GetVal(n));
		}

		for ( n = 0 ; n < list[0].GetSize() ; n++ )
			m_IdKeyTab.Add(*(pMain->m_IdKeyTab.GetUid(list[0][n])));

		for ( n = 0 ; n < list[1].GetSize() ; n++ )
			m_IdKeyTab.Add(*(pMain->m_IdKeyTab.GetUid(list[1][n])));

		if ( m_pDocument->m_ParamTab.m_RsaExt != 0 ) {
			for ( n = 0 ; n < m_IdKeyTab.GetSize() ; n++ ) {
				if ( (m_IdKeyTab[n].m_Type & IDKEY_TYPE_MASK) != IDKEY_RSA2 || m_IdKeyTab[n].m_RsaNid != NID_sha1 )
					continue;

				// 追加せずに置き換えに変更 2021.11.05
				m_IdKeyTab[n].m_Type &= IDKEY_TYPE_MASK;
				m_IdKeyTab[n].m_Cert = 0;
				m_IdKeyTab[n].m_RsaNid = (m_pDocument->m_ParamTab.m_RsaExt == 1 ? NID_sha256 : NID_sha512);
			}
		}

		for ( n = 0 ; n <= AST_DONE ; n++ )
			m_AuthReqTab[n] = AST_DONE;
		i = AST_START;

		for ( n = 0 ; m_pDocument->m_ParamTab.GetPropNode(11, n, str) ; n++ ) {
			if ( str.CompareNoCase(_T("publickey")) == 0 ) {
				m_AuthReqTab[i] = AST_PUBKEY_TRY;
				i = AST_PUBKEY_TRY;
			} else if ( str.CompareNoCase(_T("hostbased")) == 0 ) {
				m_AuthReqTab[i] = AST_HOST_TRY;
				i = AST_HOST_TRY;
			} else if ( str.CompareNoCase(_T("password")) == 0 ) {
				m_AuthReqTab[i] = AST_PASS_TRY;
				i = AST_PASS_TRY;
			} else if ( str.CompareNoCase(_T("keyboard-interactive")) == 0 ) {
				m_AuthReqTab[i] = AST_KEYB_TRY;
				i = AST_KEYB_TRY;
			} else if ( str.CompareNoCase(_T("gssapi-with-mic")) == 0 ) {
				m_AuthReqTab[i] = AST_GSSAPI_TRY;
				i = AST_GSSAPI_TRY;
			}
		}

		m_EncCip.Init(_T("none"), MODE_ENC);
		m_DecCip.Init(_T("none"), MODE_DEC);
		m_EncMac.Init(_T("none"));
		m_DecMac.Init(_T("none"));
		m_EncCmp.Init(_T("none"), MODE_ENC);
		m_DecCmp.Init(_T("none"), MODE_DEC);

		m_CipTabMax = 0;
		for ( n = 0 ; n < 8 && m_pDocument->m_ParamTab.GetPropNode(0, n, str) ; n++ )
			m_CipTab[m_CipTabMax++] = m_EncCip.GetNum(str);

		n = m_pDocument->m_ParamTab.GetPropNode(2, 0, str);
		m_CompMode = (n == FALSE || str.Compare(_T("none")) == 0 ? FALSE : TRUE);

		if ( !CExtSocket::Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType, pAddrInfo) )
			return FALSE;

		return TRUE;

	} catch(LPCTSTR pMsg) {
		m_pDocument->m_ErrorPrompt.Format(_T("ssh Open Error '%s'"), pMsg);
		return FALSE;

	} catch(...) {
		m_pDocument->m_ErrorPrompt = _T("ssh Open Error 'Unknown'");
		return FALSE;
	}
}
void Cssh::Close()
{
	for ( int n = 0 ; n < m_FifoCannel.GetSize() ; n++ ) {
		CFifoChannel *pChan = (CFifoChannel *)m_FifoCannel[n];
		if ( pChan != NULL ) {
			pChan->m_Status = 0;
			ChannelClose(n);
		}
	}

	CExtSocket::Close();
}

//////////////////////////////////////////////////////////////////////

void Cssh::OnTimer(UINT_PTR nIDEvent)
{
	if ( nIDEvent == m_KeepAliveTiimerId )
		m_pFifoMid->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_KEEPALIVE);
	else
		CExtSocket::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////

void Cssh::OnRecvSocket(void* lpBuf, int nBufLen, int nFlags)
{
	CString msg;

	try {
		int crc, ch, rlen;
		CBuffer *bp, tmp, cmp, dec;
		CStringA str;

		if ( nFlags != 0 )
			return;

		m_Incom.Apend((LPBYTE)lpBuf, nBufLen);

		for ( ; ; ) {
			switch(m_InPackStat) {
			case 0:		// Fast Verstion String
				for ( ; ; ) {
					if ( m_Incom.GetSize() == 0 )
						return;
					ch = m_Incom.Get8Bit();
					if ( ch == '\n' ) {
						CString product, version;
						product = AfxGetApp()->GetProfileString(_T("MainFrame"), _T("ProductName"), _T("RLogin"));
						((CRLoginApp *)AfxGetApp())->GetVersion(version);

						m_ServerVerStr = LocalStr(m_ServerPrompt);

						if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSH1MODE) && (strncmp(m_ServerPrompt, "SSH-2", 5) == 0 || strncmp(m_ServerPrompt, "SSH-1.99", 8) == 0) ) {
							m_ClientVerStr.Format(_T("SSH-2.0-%s-%s"), (LPCTSTR)product, (LPCTSTR)version);
							m_InPackStat = 3;
							m_SSHVer = 2;
						} else if ( strncmp(m_ServerPrompt, "SSH-1", 5) == 0 ) {
							m_ClientVerStr.Format(_T("SSH-1.5-%s-%s"), (LPCTSTR)product, (LPCTSTR)version);
							m_InPackStat = 1;
							m_SSHVer = 1;
						} else {
							if ( !m_ServerPrompt.IsEmpty() ) {
								m_ServerPrompt += "\r\n";
								SendDocument((const void *)(LPCSTR)m_ServerPrompt, m_ServerPrompt.GetLength());
							}
							m_ServerPrompt.Empty();
							m_InPackStat = 0;
						}

						if ( m_InPackStat != 0 ) {
							if ( !m_pDocument->m_ParamTab.m_VerIdent.IsEmpty() ) {
								m_ClientVerStr += _T(' ');
								m_ClientVerStr += m_pDocument->m_ParamTab.m_VerIdent;
							}

							str.Format("%s\r\n", RemoteStr(m_ClientVerStr));
							SendSocket((LPCSTR)str, str.GetLength());

							DEBUGLOG("Receive Version %s", TstrToMbs(m_ServerVerStr));
							DEBUGLOG("Send Version %s", str);
						}

						break;
					} else if ( ch != '\r' )
						m_ServerPrompt += (char)ch;
				}
				break;

			case 1:		// SSH1 Packet Length
				if ( m_Incom.GetSize() < 4 )
					return;
				m_InPackLen = m_Incom.Get32Bit();
				m_InPackPad = 8 - (m_InPackLen % 8);
				if ( m_InPackLen < 4 || m_InPackLen > (256 * 1024) ) {	// Error Packet Length
					m_Incom.Clear();
					SendDisconnect("Packet length Error");
					throw _T("ssh1 packet length error");
				}
				m_InPackStat = 2;
				// break; Not use
			case 2:		// SSH1 Packet
				if ( m_Incom.GetSize() < (m_InPackLen + m_InPackPad) )
					return;
				tmp.Clear();
				tmp.Apend(m_Incom.GetPtr(), m_InPackLen + m_InPackPad);
				m_Incom.Consume(m_InPackLen + m_InPackPad);
				m_InPackStat = 1;
				bp = &tmp;
				dec.Clear();
				m_DecCip.Cipher(bp->GetPtr(), bp->GetSize(), &dec);
				bp = &dec;
				crc = bp->PTR32BIT(bp->GetPos(bp->GetSize() - 4));
				if ( crc != ssh_crc32(bp->GetPtr(), bp->GetSize() - 4) ) {
					SendDisconnect("Packet crc Error");
					throw _T("ssh1 packet crc error");
				}
				bp->Consume(m_InPackPad);
				bp->ConsumeEnd(4);
				cmp.Clear();
				m_DecCmp.UnCompress(bp->GetPtr(), bp->GetSize(), &cmp);
				bp = &cmp;
				ReceivePacket(bp);
				break;

			case 3:		// SSH2 Packet Lenght
				if ( m_DecCip.IsAEAD() ) {
					m_InPackStat = 5;
					break;
				} else if ( m_DecCip.IsPOLY() ) {
					m_InPackStat = 9;
					break;
				} else if ( m_DecMac.IsEtm() ) {
					m_InPackStat = 7;
					break;
				}
				if ( m_Incom.GetSize() < m_DecCip.GetBlockSize() )
					return;
				m_InPackBuf.Clear();
				m_DecCip.Cipher(m_Incom.GetPtr(), m_DecCip.GetBlockSize(), &m_InPackBuf);
				m_Incom.Consume(m_DecCip.GetBlockSize());
				m_InPackLen = m_InPackBuf.PTR32BIT(m_InPackBuf.GetPtr());
				if ( m_InPackLen < 5 || m_InPackLen > (256 * 1024) ) {
					m_Incom.Clear();
					SendDisconnect2(1, "Packet Len Error");
					throw _T("ssh2 packet length error");
				}
				m_InPackStat = 4;
				// break; Not use
			case 4:		// SSH2 Packet
				if ( m_Incom.GetSize() < (m_InPackLen - m_DecCip.GetBlockSize() + 4 + m_DecMac.GetBlockSize()) )
					return;
				m_InPackStat = 3;
				m_DecCip.Cipher(m_Incom.GetPtr(), m_InPackLen - m_DecCip.GetBlockSize() + 4, &m_InPackBuf);
				m_Incom.Consume(m_InPackLen - m_DecCip.GetBlockSize() + 4);
				bp = &m_InPackBuf;
				tmp.Clear();
				m_DecMac.Compute(m_RecvPackSeq, bp->GetPtr(), bp->GetSize(), &tmp);
				m_InPackLen = bp->Get32Bit();
				m_InPackPad = bp->Get8Bit();
				bp->ConsumeEnd(m_InPackPad);
				cmp.Clear();
				m_DecCmp.UnCompress(bp->GetPtr(), bp->GetSize(), &cmp);
				bp = &cmp;
				if ( m_DecMac.GetBlockSize() > 0 ) {
					if ( memcmp(tmp.GetPtr(), m_Incom.GetPtr(), m_DecMac.GetBlockSize()) != 0 ) {
						SendDisconnect2(1, "MAC Error");
						throw _T("ssh2 mac miss match error");
					}
					m_Incom.Consume(m_DecMac.GetBlockSize());
				}
				m_RecvPackSeq += 1;
				m_RecvPackLen += bp->GetSize();
				ReceivePacket2(bp);
				break;

			case 5:		// SSH2 AEAD Packet length
				if ( !m_DecCip.IsAEAD() ) {
					m_InPackStat = 3;
					break;
				}
				if ( m_Incom.GetSize() < 4 )
					return;
				m_DecCip.SetIvCounter(m_RecvPackSeq);
				m_InPackLen = m_DecCip.GeHeadData(m_Incom.GetPtr());
				if ( m_InPackLen < 5 || m_InPackLen > (256 * 1024) ) {
					m_Incom.Clear();
					SendDisconnect2(1, "Packet Len Error");
					throw _T("ssh2 AEAD packet length error");
				}
				m_InPackStat = 6;
				// break; Not use
			case 6:		// SSH2 AEAD Packet
				rlen = 4 + m_InPackLen + m_DecCip.GetTagSize();
				if ( m_Incom.GetSize() < rlen )
					return;
				m_InPackStat = 5;
				tmp.Clear();
				if ( !m_DecCip.Cipher(m_Incom.GetPtr(), rlen, &tmp) ) {
					m_Incom.Clear();
					SendDisconnect2(1, "MAC Error");
					throw _T("ssh2 AEAD mac miss match error");
				}
				m_Incom.Consume(rlen);
				m_InPackLen = tmp.Get32Bit();
				m_InPackPad = tmp.Get8Bit();
				tmp.ConsumeEnd(m_InPackPad);
				m_InPackBuf.Clear();
				m_DecCmp.UnCompress(tmp.GetPtr(), tmp.GetSize(), &m_InPackBuf);
				m_RecvPackSeq += 1;
				m_RecvPackLen += m_InPackBuf.GetSize();
				ReceivePacket2(&m_InPackBuf);
				break;

			case 7:		// SSH2 etm Packet length
				if ( !m_DecMac.IsEtm() ) {
					m_InPackStat = 3;
					break;
				}
				if ( m_Incom.GetSize() < 4 )
					return;
				m_DecCip.SetIvCounter(m_RecvPackSeq);
				m_InPackLen = m_DecCip.GeHeadData(m_Incom.GetPtr());
				if ( m_InPackLen < 5 || m_InPackLen > (256 * 1024) ) {
					m_Incom.Clear();
					SendDisconnect2(1, "Packet Len Error");
					throw _T("ssh2 etm packet length error");
				}
				m_InPackStat = 8;
				// break; Not use
			case 8:		// SSH2 etm Packet
				if ( m_Incom.GetSize() < (4 + m_InPackLen + m_DecMac.GetBlockSize()) )
					return;
				m_InPackStat = 7;
				dec.Clear();
				dec.Apend(m_Incom.GetPtr(), 4);
				m_DecCip.Cipher(m_Incom.GetPtr() + 4, m_InPackLen, &dec);
				tmp.Clear();
				m_DecMac.Compute(m_RecvPackSeq, m_Incom.GetPtr(), 4 + m_InPackLen, &tmp);
				m_Incom.Consume(4 + m_InPackLen);
				if ( m_DecMac.GetBlockSize() > 0 ) {
					if ( memcmp(tmp.GetPtr(), m_Incom.GetPtr(), m_DecMac.GetBlockSize()) != 0 ) {
						SendDisconnect2(1, "MAC Error");
						throw _T("ssh2 etm mac miss match error");
					}
					m_Incom.Consume(m_DecMac.GetBlockSize());
				}
				m_InPackLen = dec.Get32Bit();
				m_InPackPad = dec.Get8Bit();
				dec.ConsumeEnd(m_InPackPad);
				m_InPackBuf.Clear();
				m_DecCmp.UnCompress(dec.GetPtr(), dec.GetSize(), &m_InPackBuf);
				m_RecvPackSeq += 1;
				m_RecvPackLen += m_InPackBuf.GetSize();
				ReceivePacket2(&m_InPackBuf);
				break;

			case 9:		// SSH2 Poly1305 Packet length
				if ( !m_DecCip.IsPOLY() ) {
					m_InPackStat = 3;
					break;
				}
				if ( m_Incom.GetSize() < 4 )
					return;
				m_DecCip.SetIvCounter(m_RecvPackSeq);
				m_InPackLen = m_DecCip.GeHeadData(m_Incom.GetPtr());
				if ( m_InPackLen < 5 || m_InPackLen > (256 * 1024) ) {
					m_Incom.Clear();
					SendDisconnect2(1, "Packet Len Error");
					throw _T("ssh2 Poly1305 packet length error");
				}
				m_InPackStat = 10;
				// break; Not use
			case 10:		// SSH2 Poly1305 Packet
				rlen = 4 + m_InPackLen + m_DecCip.GetTagSize();
				if ( m_Incom.GetSize() < rlen )
					return;
				m_InPackStat = 9;
				tmp.Clear();
				if ( !m_DecCip.Cipher(m_Incom.GetPtr(), rlen, &tmp) ) {
					m_Incom.Clear();
					SendDisconnect2(1, "MAC Error");
					throw _T("ssh2 Poly1305 mac miss match error");
				}
				m_Incom.Consume(rlen);
				m_InPackLen = tmp.Get32Bit();
				m_InPackPad = tmp.Get8Bit();
				tmp.ConsumeEnd(m_InPackPad);
				m_InPackBuf.Clear();
				m_DecCmp.UnCompress(tmp.GetPtr(), tmp.GetSize(), &m_InPackBuf);
				m_RecvPackSeq += 1;
				m_RecvPackLen += m_InPackBuf.GetSize();
				ReceivePacket2(&m_InPackBuf);
				break;
			}
		}

	} catch(LPCWSTR pMsg) {
		msg.Format(_T("ssh Receive '%s'"), UniToTstr(pMsg));
	} catch(LPCSTR pMsg) {
		msg.Format(_T("ssh Receive '%s'"), MbsToTstr(pMsg));
	} catch(...) {
		msg.Format(_T("ssh Receive '%s'"), _T("unkown"));
	}

	int n;
	CString str;

	msg += _T("\n\nReceive Packet Dump\n");

	for ( n = 0 ; n < nBufLen && n < 16 ; n++ ) {
		str.Format(_T("%02x "), ((LPBYTE)lpBuf)[n]);
		msg += str;
	}

	if ( nBufLen > 16 )
		msg += _T("...");
	msg += _T('\n');

	for ( n = 0 ; n < nBufLen && n < 16 ; n++ ) {
		if ( ((LPBYTE)lpBuf)[n] < ' ' || ((LPBYTE)lpBuf)[n] >= 0x7F )
			msg += _T('.');
		else
			msg += ((CHAR *)lpBuf)[n];
	}

	if ( nBufLen > 16 )
		msg += _T("...");
	msg += _T('\n');

	AfxMessageBox(msg);
}

//////////////////////////////////////////////////////////////////////

void Cssh::SendWindSize()
{
	try {
		CFifoChannel *pChan;
		CBuffer tmp;
		int cx = 0, cy = 0, sx = 0, sy = 0;

		if ( m_pDocument != NULL )
			m_pDocument->m_TextRam.GetScreenSize(&cx, &cy, &sx, &sy);

		switch(m_SSHVer) {
		case 1:
			tmp.Put8Bit(SSH_CMSG_WINDOW_SIZE);
			tmp.Put32Bit(cy);
			tmp.Put32Bit(cx);
			tmp.Put32Bit(sx);
			tmp.Put32Bit(sy);
			PostSendPacket(1, &tmp);
			break;
		case 2:
			if ( (pChan = GetFifoChannel(FdToId(FIFO_EXTIN))) == NULL || (pChan->m_Status & CHAN_OPEN_REMOTE) == 0 )
				break;
			tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
			tmp.Put32Bit(pChan->m_RemoteID);
			tmp.PutStr("window-change");
			tmp.Put8Bit(0);
			tmp.Put32Bit(cx);
			tmp.Put32Bit(cy);
			tmp.Put32Bit(sx);
			tmp.Put32Bit(sy);
			PostSendPacket(2, &tmp);
			break;
		}
	} catch(...) {
		AfxMessageBox(_T("ssh SendWindSize Error"));
	}
}
void Cssh::SendBreak(int opt)
{
	try {
		CFifoChannel *pChan;
		CBuffer tmp;

		switch(m_SSHVer) {
		case 2:
			if ( (pChan = GetFifoChannel(FdToId(FIFO_EXTIN))) == NULL || (pChan->m_Status & CHAN_OPEN_REMOTE) == 0 )
				break;
			tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
			tmp.Put32Bit(pChan->m_RemoteID);
			tmp.PutStr("break");
			tmp.Put8Bit(0);
			tmp.Put32Bit(opt != 0 ? 300 : 100);
			PostSendPacket(2, &tmp);
			break;
		}
	} catch(...) {
		AfxMessageBox(_T("ssh SendBreak Error"));
	}
}
int Cssh::GetRecvSize()
{
	return CExtSocket::GetRecvSocketSize();
}

//////////////////////////////////////////////////////////////////////

void Cssh::GetStatus(CString &str)
{
	CString tmp;

	CExtSocket::GetStatus(str);

	str += _T("\r\n");
	tmp.Format(_T("SSH Server: %d\r\n"), m_SSHVer);
	str += tmp;

	tmp.Format(_T("Version: %s\r\n"), (LPCTSTR)m_ServerVerStr);
	str += tmp;
	tmp.Format(_T("Kexs: %s\r\n"), (LPCTSTR)m_SProp[PROP_KEX_ALGS]);
	str += tmp;
	tmp.Format(_T("Keys: %s\r\n"), (LPCTSTR)m_SProp[PROP_HOST_KEY_ALGS]);
	str += tmp;
	tmp.Format(_T("Auth: %s\r\n"), (LPCTSTR)m_AuthMeta);
	str += tmp;
	tmp.Format(_T("Cipher CtoS: %s\r\n"), (LPCTSTR)m_SProp[PROP_ENC_ALGS_CTOS]);
	str += tmp;
	tmp.Format(_T("Cipher StoC: %s\r\n"), (LPCTSTR)m_SProp[PROP_ENC_ALGS_STOC]);
	str += tmp;
	tmp.Format(_T("Mac CtoS: %s\r\n"), (LPCTSTR)m_SProp[PROP_MAC_ALGS_CTOS]);
	str += tmp;
	tmp.Format(_T("Mac StoC: %s\r\n"), (LPCTSTR)m_SProp[PROP_MAC_ALGS_STOC]);
	str += tmp;
	tmp.Format(_T("Compess CtoS: %s\r\n"), (LPCTSTR)m_SProp[PROP_COMP_ALGS_CTOS]);
	str += tmp;
	tmp.Format(_T("Compess StoC: %s\r\n"), (LPCTSTR)m_SProp[PROP_COMP_ALGS_STOC]);
	str += tmp;
	tmp.Format(_T("Language CtoS: %s\r\n"), (LPCTSTR)m_SProp[PROP_LANG_CTOS]);
	str += tmp;
	tmp.Format(_T("Language StoC: %s\r\n"), (LPCTSTR)m_SProp[PROP_LANG_STOC]);
	str += tmp;

	if ( m_ExtInfo.GetSize() > 0 ) {
		str += _T("ext-info: ");
		for ( int n = 0 ; n < m_ExtInfo.GetSize() ; n++ ) {
			if ( n > 0 )
				str += _T(",");
			tmp.Format(_T("%s=\"%s\""), (LPCTSTR)m_ExtInfo[n].m_nIndex, (LPCTSTR)m_ExtInfo[n].m_String);
			str += tmp;
		}
		str += _T("\r\n");
	}

	str += _T("\r\n");
	tmp.Format(_T("Kexs: %s\r\n"), (LPCTSTR)m_VProp[PROP_KEX_ALGS]);
	str += tmp;

	if ( m_EncCip.IsAEAD() || m_EncCip.IsPOLY() )
		tmp.Format(_T("Encode: %s + %s\r\n"), m_EncCmp.GetTitle(), m_EncCip.GetTitle());
	else
		tmp.Format(_T("Encode: %s + %s + %s\r\n"), m_EncCmp.GetTitle(), m_EncCip.GetTitle(), m_EncMac.GetTitle());
	str += tmp;

	if ( m_DecCip.IsAEAD() || m_DecCip.IsPOLY() )
		tmp.Format(_T("Decode: %s + %s\r\n"), m_DecCmp.GetTitle(), m_DecCip.GetTitle());
	else
		tmp.Format(_T("Decode: %s + %s + %s\r\n"), m_DecCmp.GetTitle(), m_DecCip.GetTitle(), m_DecMac.GetTitle());
	str += tmp;

	str += _T("UserAuth: ");
	if ( !m_AuthLog.IsEmpty() )
		str += m_AuthLog;
	else
		str += _T("none");
	str += _T("\r\n");

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHKEEPAL) && m_pDocument->m_TextRam.m_SshKeepAlive > 0 && m_pDocument->m_ConnectTime != 0 ) {
		CTime now = CTime::GetCurrentTime();
		int sec = (int)(now.GetTime() - m_pDocument->m_ConnectTime);

		str += _T("KeepAlive CtoS: ");

		if ( sec >= 3600 )
			tmp.Format(_T("%d:%02d:%02d"), sec / 3600, (sec % 3600) / 60, sec % 60);
		else if ( sec >= 60 )
			tmp.Format(_T("%d:%02d"), sec / 60, sec % 60);
		else
			tmp.Format(_T("%d"), sec);
		str += tmp;

		tmp.Format(_T(" / %d = "), m_pDocument->m_TextRam.m_SshKeepAlive);
		str += tmp;
		tmp.Format(_T("%d, Send %d, Reply %d\r\n"), sec / m_pDocument->m_TextRam.m_SshKeepAlive, m_KeepAliveSendCount, m_KeepAliveReplyCount);
		str += tmp;
	}

	if ( m_KeepAliveRecvGlobalCount > 0 || m_KeepAliveRecvChannelCount > 0 ) {
		tmp.Format(_T("KeepAlive StoC: Global %d, Channel %d\r\n"), m_KeepAliveRecvGlobalCount, m_KeepAliveRecvChannelCount);
		str += tmp;
	}

	m_HostKey.FingerPrint(tmp);
	str += _T("\r\n");
	str += tmp;

	str += _T("\r\n");
	tmp.Format(_T("Open Channel: %d\r\n"), m_OpenChanCount);
	str += tmp;

	for ( int n = 0 ;  n < m_FifoCannel.GetSize() ; n++ ) {
		CFifoChannel *pChan = (CFifoChannel *)m_FifoCannel[n];
		if ( pChan != NULL ) {
			tmp.Format(_T("Chanel: RemoteId=%d LocalId=%d Name=%s Type=%d Status=%03o Stage=%d\r\n"),
				pChan->m_RemoteID, pChan->m_LocalID, MbsToTstr(pChan->m_TypeName), pChan->m_Type, pChan->m_Status, pChan->m_Stage);
			str += tmp;
		}
	}

	if ( m_Permit.GetSize() > 0 ) {
		str += _T("\r\n");
		tmp.Format(_T("Remote listened : %d\r\n"), m_Permit.GetSize());
		str += tmp;

		for ( int n = 0 ; n < m_Permit.GetSize() ; n++ ) {
			if ( m_Permit[n].m_Type == PFD_RSOCKS )
				tmp.Format(_T("Type=%d Listened=%s:%d\r\n"), m_Permit[n].m_Type, (LPCTSTR)m_Permit[n].m_rHost, m_Permit[n].m_rPort);
			else
				tmp.Format(_T("Type=%d Listened=%s:%d Connect=%s:%d\r\n"), m_Permit[n].m_Type, (LPCTSTR)m_Permit[n].m_rHost, m_Permit[n].m_rPort, (LPCTSTR)m_Permit[n].m_lHost, m_Permit[n].m_lPort);
			str += tmp;
		}
	}
}
void Cssh::ResetOption()
{
	CExtSocket::ResetOption();

	if ( m_KeepAliveTiimerId != 0 ) {
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this, m_KeepAliveTiimerId);
		m_KeepAliveTiimerId = 0;
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHKEEPAL) && m_pDocument->m_TextRam.m_SshKeepAlive > 0 )
		m_KeepAliveTiimerId = ((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(m_pDocument->m_TextRam.m_SshKeepAlive * 1000, TIMEREVENT_SOCK | TIMEREVENT_INTERVAL, this);

	if ( m_bAuthAgentReqEnable != m_pDocument->m_TextRam.IsOptEnable(TO_SSHAGENT) || m_PortFwdTable.Compare(m_pDocument->m_ParamTab.m_PortFwd) != 0 ) {
		AfxMessageBox(CStringLoad(IDS_SSHOPTIONCHECK), MB_ICONWARNING);

		m_bAuthAgentReqEnable = m_pDocument->m_TextRam.IsOptEnable(TO_SSHAGENT);
		m_PortFwdTable = m_pDocument->m_ParamTab.m_PortFwd;
	}
}

//////////////////////////////////////////////////////////////////////

void Cssh::FifoCmdSendPacket(int type, CBuffer *bp)
{
	if (  (m_SSH2Status & SSH2_STAT_SENTKEXINIT) != 0 ) {
		DelayedCheck *pDelayedCheck = new DelayedCheck;
		pDelayedCheck->type = type;
		pDelayedCheck->pParam = (void *)bp;
		pDelayedCheck->bDelBuf = TRUE;
		m_DelayedCheck.AddTail(pDelayedCheck);
	} else {
		switch(type) {
		case 1: SendPacket(bp); break;
		case 2: SendPacket2(bp); break;
		}
		delete bp;
	}
}
void Cssh::PostSendPacket(int type, CBuffer *bp)
{
	CBuffer *pWork = new CBuffer;
	pWork->Swap(*bp);
	m_pFifoMid->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDPACKET, type, 0, (void *)pWork);
}

//////////////////////////////////////////////////////////////////////
// SSH1 Function

void Cssh::SendDisconnect(LPCSTR str)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH_MSG_DISCONNECT);
	tmp.PutStr(str);
	SendPacket(&tmp);
}
void Cssh::SendPacket(CBuffer *bp)
{
	int len, pad, crc;
	CBuffer tmp, ext, dum;

	m_EncCmp.Compress(bp->GetPtr(), bp->GetSize(), &dum);
	bp = &dum;

	len = bp->GetSize() + 4;		// with CRC 4 Byte
	pad = 8 - (len % 8);

	tmp.Apend((LPBYTE)"\0\0\0\0\0\0\0\0", pad);
	tmp.Apend(bp->GetPtr(), bp->GetSize());
	crc = ssh_crc32(tmp.GetPtr(), tmp.GetSize());
	tmp.Put32Bit(crc);
	bp = &tmp;

	dum.Clear();
	m_EncCip.Cipher(bp->GetPtr(), bp->GetSize(), &dum);
	bp = &dum;

	ext.Put32Bit(len);
	ext.Apend(bp->GetPtr(), bp->GetSize());

	SendSocket(ext.GetPtr(), ext.GetSize());
}

int Cssh::SMsgPublicKey(CBuffer *bp)
{
	int n;
	RSA *host_key;
	RSA *server_key;
	BIGNUM *host_key_n, *host_key_e;
	BIGNUM *server_key_n, *server_key_e;
	BIGNUM *m_host_key_n, *m_host_key_e;
	CBuffer tmp;

	for ( n = 0 ; n < 8 ; n++ )
		m_Cookie[n] = bp->Get8Bit();

	server_key = RSA_new();
	n = bp->Get32Bit();
	server_key_e = bp->GetBIGNUM(NULL);
	server_key_n = bp->GetBIGNUM(NULL);
	RSA_set0_key(server_key, server_key_n, server_key_e, NULL);

	host_key = RSA_new();
	n = bp->Get32Bit();
	host_key_e = bp->GetBIGNUM(NULL);
	host_key_n = bp->GetBIGNUM(NULL);
	RSA_set0_key(host_key, host_key_n, host_key_e, NULL);

	m_HostKey.Create(IDKEY_RSA1);
	m_host_key_e = BN_dup(host_key_e);
	m_host_key_n = BN_dup(host_key_n);
	RSA_set0_key(m_HostKey.m_Rsa, m_host_key_n, m_host_key_e, NULL);

	if ( !m_HostKey.HostVerify(m_HostName, m_HostPort, this) )
		return FALSE;

	m_ServerFlag  = bp->Get32Bit();
	m_SupportCipher = bp->Get32Bit();
	m_SupportAuth = bp->Get32Bit();

	EVP_MD_CTX *md_ctx;
    BYTE nbuf[2048];
	BYTE obuf[EVP_MAX_MD_SIZE];
	int len;
	BIGNUM *key;

	if ( (md_ctx = EVP_MD_CTX_new()) == NULL )
		throw _T("SMsgPublicKey EVP_MD_CTX error");

	len = BN_num_bytes(host_key_n);
    BN_bn2bin(host_key_n, nbuf);

	if ( EVP_DigestInit(md_ctx, EVP_md5()) == 0 ||
	     EVP_DigestUpdate(md_ctx, nbuf, len) == 0 )
		throw _T("SMsgPublicKey host key error");

	len = BN_num_bytes(server_key_n);
    BN_bn2bin(server_key_n, nbuf);

	if ( EVP_DigestUpdate(md_ctx, nbuf, len) == 0 ||
	     EVP_DigestUpdate(md_ctx, m_Cookie, 8) == 0 ||
		 EVP_DigestFinal(md_ctx, obuf, NULL) == 0 )
		throw _T("SMsgPublicKey server key error");

	EVP_MD_CTX_free(md_ctx);
	memcpy(m_SessionId, obuf, 16);

	rand_buf(m_SessionKey, sizeof(m_SessionKey));

	key = BN_new();
	BN_set_word(key, 0);
	for ( n  = 0; n < 32 ; n++ ) {
		BN_lshift(key, key, 8);
		if ( n < 16 )
			BN_add_word(key, m_SessionKey[n] ^ m_SessionId[n]);
		else
			BN_add_word(key, m_SessionKey[n]);
    }

	if ( BN_cmp(server_key_n, host_key_n) < 0 ) {
		rsa_public_encrypt(key, key, server_key);
		rsa_public_encrypt(key, key, host_key);
	} else {
		rsa_public_encrypt(key, key, host_key);
		rsa_public_encrypt(key, key, server_key);
	}

	RSA_free(server_key);
	RSA_free(host_key);

	m_CipherMode = 0;
	for ( n = 0 ; n < m_CipTabMax ; n++ ) {
		if ( (m_SupportCipher & (1 << m_CipTab[n])) != 0 ) {
			m_CipherMode = m_CipTab[n];
			break;
		}
	}
	if ( m_CipherMode == 0 )
		return FALSE;

	tmp.Put8Bit(SSH_CMSG_SESSION_KEY);
	tmp.Put8Bit(m_CipherMode);
    for ( n = 0 ; n < 8 ; n++ )
		tmp.Put8Bit(m_Cookie[n]);
	tmp.PutBIGNUM(key);
	tmp.Put32Bit(SSH_PROTOFLAG_SCREEN_NUMBER | SSH_PROTOFLAG_HOST_IN_FWD_OPEN);
	SendPacket(&tmp);

	BN_clear_free(key);

	m_EncCip.Init(m_EncCip.GetName(m_CipherMode), MODE_ENC, m_SessionKey, 32);
	m_DecCip.Init(m_DecCip.GetName(m_CipherMode), MODE_DEC, m_SessionKey, 32);

	return TRUE;
}
CIdKey *Cssh::SelectIdKeyEntry()
{
	while ( m_IdKeyPos < m_IdKeyTab.GetSize() ) {
		m_pIdKey = &(m_IdKeyTab[m_IdKeyPos++]);
		if ( m_pIdKey->m_Type == IDKEY_RSA1 || m_pIdKey->m_Type == IDKEY_RSA2 )
			return m_pIdKey;
	}
	return NULL;
}
int Cssh::SMsgAuthRsaChallenge(CBuffer *bp)
{
	int n;
	int len;
	CBuffer tmp;
	BIGNUM *challenge;
	BYTE buf[32], res[EVP_MAX_MD_SIZE];
	CSpace inbuf, otbuf;

	if ( m_pIdKey == NULL )
		return FALSE;

	if ( !m_pIdKey->InitPass(m_pDocument->m_ServerEntry.m_PassName) )
		return FALSE;

	if ( (challenge = BN_new()) == NULL )
		return FALSE;

	bp->GetBIGNUM(challenge);
	inbuf.SetSize(BN_num_bytes(challenge));
	BN_bn2bin(challenge, inbuf.GetPtr());

	otbuf.SetSize(RSA_size(m_pIdKey->m_Rsa));

	if ( (len = RSA_private_decrypt(inbuf.GetSize(), inbuf.GetPtr(), otbuf.GetPtr(), m_pIdKey->m_Rsa, RSA_PKCS1_PADDING)) <= 0 ) {
		BN_free(challenge);
		return FALSE;
	}

	BN_bin2bn(otbuf.GetPtr(), len, challenge);

	if ( (len = BN_num_bytes(challenge)) <= 0 || len > 32 ) {
		BN_free(challenge);
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	BN_bn2bin(challenge, buf + sizeof(buf) - len);

	tmp.Apend(buf, 32);
	tmp.Apend(m_SessionId, 16);
	len = MD_digest(EVP_md5(), tmp.GetPtr(), tmp.GetSize(), res, sizeof(res));
	tmp.Clear();

	tmp.Put8Bit(SSH_CMSG_AUTH_RSA_RESPONSE);
	for ( n = 0 ; n < len ; n++ )
		tmp.Put8Bit(res[n]);
	SendPacket(&tmp);

	BN_free(challenge);
	return TRUE;
}
void Cssh::ReceivePacket(CBuffer *bp)
{
	int n;
	CStringA str;
	CBuffer tmp;
	int type = bp->Get8Bit();

	if ( type == SSH_MSG_DISCONNECT ) {
		bp->GetStr(str);
		SendTextMsg(str, str.GetLength());
		Close();
		return;
	} else if ( type == SSH_MSG_DEBUG ) {
		bp->GetStr(str);
		SendTextMsg(str, str.GetLength());
		return;
	}

	switch(m_PacketStat) {
	case 0:		// Session Key Excheng
		if ( type != SSH_SMSG_PUBLIC_KEY )
			goto DISCONNECT;
		if ( !SMsgPublicKey(bp) )
			goto DISCONNECT;
		m_PacketStat = 1;
		break;
	case 1:		// Session Key Exc Success & Send Login User
		if ( type != SSH_SMSG_SUCCESS )
			goto DISCONNECT;
		tmp.Put8Bit(SSH_CMSG_USER);
		tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_UserName));
		SendPacket(&tmp);
		m_PacketStat = 2;
		break;
	case 2:		// Login User Status
		m_PacketStat = 5;
		if ( type == SSH_SMSG_SUCCESS )
			break;
		if ( type != SSH_SMSG_FAILURE )
			goto DISCONNECT;
		if ( (m_SupportAuth & (1 << SSH_AUTH_RSA)) != 0 && SelectIdKeyEntry() != NULL && m_pIdKey->m_Rsa != NULL ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_RSA);
			{
				BIGNUM const *n = NULL;
				RSA_get0_key(m_pIdKey->m_Rsa, &n, NULL, NULL);
				tmp.PutBIGNUM(n);
			}
			SendPacket(&tmp);
			if ( !m_AuthLog.IsEmpty() )
				m_AuthLog += _T(",");
			m_AuthLog += _T("publickey");
			m_PacketStat = 10;
			break;
		}
	GO_AUTH_TIS:
		if ( (m_SupportAuth & (1 << SSH_AUTH_TIS)) != 0 ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_TIS);
			SendPacket(&tmp);
			if ( !m_AuthLog.IsEmpty() )
				m_AuthLog += _T(",");
			m_AuthLog += _T("tis");
			m_PacketStat = 3;
			break;
		}
	GO_AUTH_PASS:
		if ( (m_SupportAuth & (1 << SSH_AUTH_PASSWORD)) != 0 ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_PASSWORD);
			tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_PassName));
			SendPacket(&tmp);
			if ( !m_AuthLog.IsEmpty() )
				m_AuthLog += _T(",");
			m_AuthLog += _T("password");
			m_PacketStat = 4;
			break;
		}
		goto DISCONNECT;
	case 10:
		if ( type != SSH_SMSG_AUTH_RSA_CHALLENGE )
			goto GO_AUTH_TIS;
		if ( !SMsgAuthRsaChallenge(bp) )
			goto DISCONNECT;
		m_PacketStat = 11;
		break;
	case 3:		// TIS Auth
		if ( type != SSH_SMSG_AUTH_TIS_CHALLENGE )
			goto GO_AUTH_PASS;
		tmp.Put8Bit(SSH_CMSG_AUTH_TIS_RESPONSE);
		tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_PassName));
		SendPacket(&tmp);
		m_PacketStat = 4;
		break;
	case 11:
		if ( type != SSH_SMSG_SUCCESS )
			goto GO_AUTH_TIS;
	case 4:		// Password Auth Success
		if ( type != SSH_SMSG_SUCCESS )
			goto DISCONNECT;
		if ( m_CompMode == FALSE )
			goto NOCOMPRESS;
		tmp.Put8Bit(SSH_CMSG_REQUEST_COMPRESSION);
		tmp.Put32Bit(COMPLEVEL);
		SendPacket(&tmp);
		m_PacketStat = 5;
		break;
	case 5:		// Compression ?
		if ( type == SSH_SMSG_SUCCESS ) {
			m_EncCmp.Init(_T("zlib"), MODE_ENC, COMPLEVEL);
			m_DecCmp.Init(_T("zlib"), MODE_DEC, COMPLEVEL);
		} else if ( type == SSH_SMSG_FAILURE ) {
			m_EncCmp.Init(_T("node"), MODE_ENC, COMPLEVEL);
			m_DecCmp.Init(_T("none"), MODE_DEC, COMPLEVEL);
		} else
			goto DISCONNECT;
	NOCOMPRESS:
		tmp.Put8Bit(SSH_CMSG_REQUEST_PTY);
		tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_TermName));
		tmp.Put32Bit(m_pDocument->m_TextRam.m_Lines);
		tmp.Put32Bit(m_pDocument->m_TextRam.m_Cols);
		tmp.Put32Bit(0);
		tmp.Put32Bit(0);
		tmp.Put8Bit(0);		// TTY_OP_END
		SendPacket(&tmp);
		m_PacketStat = 6;
		break;
	case 6:		// TERM name & WinSz Success
		if ( type != SSH_SMSG_SUCCESS )
			goto DISCONNECT;
		tmp.Put8Bit(SSH_CMSG_EXEC_SHELL);
		SendPacket(&tmp);
		m_PacketStat = 7;
		m_bConnect = TRUE;
		m_pFifoMid->SendFdEvents(FIFO_EXTOUT, FD_CONNECT, NULL);
		break;

	case 7:		// Client loop
		switch(type) {
		case SSH_MSG_CHANNEL_DATA:
		case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
		case SSH_MSG_PORT_OPEN:
		case SSH_SMSG_AGENT_OPEN:
		case SSH_SMSG_X11_OPEN:
		case SSH_MSG_CHANNEL_CLOSE:
		case SSH_MSG_CHANNEL_CLOSE_CONFIRMATION:
			tmp.Put8Bit(SSH_SMSG_FAILURE);
			SendPacket(&tmp);
			break;

		case SSH_MSG_IGNORE:
			break;
		case SSH_SMSG_EXITSTATUS:
			SendDisconnect("Recv Exit Status");
			break;
		case SSH_SMSG_STDERR_DATA:
		case SSH_SMSG_STDOUT_DATA:
			n = bp->Get32Bit();
			SendTextMsg((LPCSTR)(bp->GetPtr()), n);
			break;

		default:
			SendDisconnect("Unkonw Type");
			break;
		}
		break;

	DISCONNECT:
		m_PacketStat = 99;
		SendDisconnect("What Type ?");
	case 99:
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// SSH2 Function

void Cssh::LogIt(LPCTSTR format, ...)
{
    va_list args;
	CString str;

	va_start(args, format);
	str.FormatV(format, args);
	va_end(args);

	if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) ) {
		m_pFifoMid->DodMsgStrPtr(DOCMSG_LOGIT, str);
		return;
	}

	CString tmp;
	CTime now = CTime::GetCurrentTime();
	DWORD size;
	TCHAR buf[MAX_COMPUTERNAME];

	tmp = now.Format(_T("%b %d %H:%M:%S "));
	size = MAX_COMPUTERNAME;
	if ( GetComputerName(buf, &size) )
		tmp += buf;
	tmp += _T(" : ");
	tmp += str;
	tmp += _T("\r\n");

	CStringA mbs(tmp);
	SendDocument((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
}
void Cssh::SendTextMsg(LPCSTR str, int len)
{
	CStringA mbs;

	while ( len-- > 0 ) {
		if ( str[0] == '\r' && str[1] == '\n' ) {
			mbs += "\r\n";
			str += 2;
		} else if ( *str == '\r' || *str == '\n' ) {
			mbs += "\r\n";
			str += 1;
		} else {
			mbs += *str;
			str += 1;
		}
	}

	SendDocument((void *)(LPCSTR)mbs, mbs.GetLength(), 0);
}
int Cssh::MatchList(LPCTSTR client, LPCTSTR server, CString &str)
{
	CString tmp;
	CStringBinary idx;

	while ( *server != _T('\0') ) {
		while ( *server != _T('\0') && _tcschr(_T(" ,"), *server) != NULL )
			server++;
		tmp = "";
		while ( *server != _T('\0') && _tcschr(_T(" ,"), *server) == NULL )
			tmp += *(server++);
		if ( !tmp.IsEmpty() )
			idx[tmp] = 1;
	}

	while ( *client != _T('\0') ) {
		while ( *client != _T('\0') && _tcschr(_T(" ,"), *client) != NULL )
			client++;
		tmp = "";
		while ( *client != _T('\0') && _tcschr(_T(" ,"), *client) == NULL )
			tmp += *(client++);
		//if ( tmp.CompareNoCase(_T("disuse")) == 0 )
		//	break;
		if ( !tmp.IsEmpty() && idx.Find(tmp) != NULL ) {
			str = tmp;
			return TRUE;
		}
	}

	return FALSE;
}
LPCTSTR Cssh::GetSigAlgs()
{
	int n;

	if ( (n = m_ExtInfo.Find(_T("server-sig-algs"))) < 0 )
		return NULL;

	return m_ExtInfo[n].m_String;
}
BOOL Cssh::IsServerVersion(LPCTSTR pattan)
{
	//	if (sscanf(server_version_string, "SSH-%d.%d-%[^\n]\n", &remote_major, &remote_minor, remote_version) != 3)
	LPCTSTR str = m_ServerVerStr;

	// SSH-
	if ( _tcsncmp(str, _T("SSH-"), 4) != 0 )
		return FALSE;
	str += 4;

	// major version
	while ( *str >= _T('0') && *str <= _T('9') )
		str++;
	if ( *str != _T('.') )
		return FALSE;
	str++;

	// minor version
	while ( *str >= _T('0') && *str <= _T('9') )
		str++;
	if ( *str != _T('-') )
		return FALSE;
	str++;

	if ( _tcsnicmp(str, pattan, _tcslen(pattan)) == 0 )
		return TRUE;

	return FALSE;
}
void Cssh::UserAuthNextState()
{
	m_IdKeyPos = 0;
	m_AuthStat = m_AuthReqTab[m_AuthStat];
	m_AuthMode = AUTH_MODE_NONE;
}
void Cssh::AddAuthLog(LPCTSTR str, ...)
{
	CString tmp;
	va_list arg;

	va_start(arg, str);
	tmp.FormatV(str, arg);
	va_end(arg);

	if ( !m_AuthLog.IsEmpty() )
		m_AuthLog += _T(", ");

	m_AuthLog += tmp;
}
BOOL Cssh::IsStriStr(LPCSTR str, LPCSTR ptn)
{
	for ( ; *str != '\0' ; str++ ) {
		if ( tolower(*str) == tolower(*ptn) ) {
			LPCSTR s = str + 1;
			LPCSTR p = ptn + 1;
			for ( ; ; ) {
				if ( *p == '\0' )
					return TRUE;
				if ( tolower(*s) != tolower(*p) )
					break;
				s++;
				p++;
			}
		}
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Channel funcsion

class CFifoChannel *Cssh::GetFifoChannel(int id)
{
	int nFd = IdToFdIn(id);
	CFifoChannel *pChan = NULL;

	if ( id < 0 ) {
		if ( id == (-2) ) {
			nFd = FIFO_EXTIN;
			id = FdToId(nFd);

			while ( id >= m_FifoCannel.GetSize() )
				m_FifoCannel.Add(NULL);

			m_FifoCannel[id] = pChan = new CFifoChannel;
			m_OpenChanCount++;

			pChan->m_Type = SSHFT_STDIO;
			pChan->m_Status = 0;
			pChan->m_Stage = 0;
			pChan->m_TypeName = _T("console");
			pChan->m_RemoteID = (-1);
			pChan->m_LocalID = id;
			pChan->m_LocalComs = 0;
			pChan->m_LocalWind = m_pDocument->m_ParamTab.m_StdIoBufSize * 1024;
			pChan->m_LocalPacks = pChan->m_LocalWind / 4;
			pChan->m_pFifoBase = NULL;
			pChan->m_bClosed = FALSE;

		} else {
			m_pFifoMid->Lock();
			for ( nFd = IdToFdIn(m_LastFreeId) ; ; nFd += 2 ) {
				nFd = ((CFifoSsh *)m_pFifoMid)->GetFreeFifo(nFd);
				id = FdToId(nFd);

				while ( id >= m_FifoCannel.GetSize() )
					m_FifoCannel.Add(NULL);

				if ( m_FifoCannel[id] == NULL )
					break;
			}
			m_LastFreeId = id + 1;
			m_FifoCannel[id] = pChan = new CFifoChannel;
			m_pFifoMid->Unlock();

			if ( ++m_OpenChanCount == CHAN_MAXCONNECT ) {
				if ( AfxMessageBox(CStringLoad(IDE_MANYCHANNEL), MB_ICONQUESTION | MB_YESNO) != IDYES ) {
					m_FifoCannel[id] = NULL;
					delete pChan;
					return NULL;
				}
			}

			pChan->m_Type = SSHFT_NONE;
			pChan->m_Status = 0;
			pChan->m_Stage = 0;
			pChan->m_RemoteID = (-1);
			pChan->m_LocalID = id;
			pChan->m_LocalComs = 0;
			pChan->m_LocalWind = CHAN_SES_WINDOW_DEFAULT;
			pChan->m_LocalPacks = CHAN_SES_PACKET_DEFAULT;
			pChan->m_pFifoBase = NULL;
			pChan->m_bClosed = FALSE;
		}

	} else if ( id < m_FifoCannel.GetSize() )
		pChan = (CFifoChannel *)m_FifoCannel[id];

	return pChan;
}

BOOL Cssh::ChannelCreate(int id, LPCTSTR lpszHostAddress, UINT nHostPort, LPCTSTR lpszRemoteAddress, UINT nRemotePort)
{
	int nFd = IdToFdIn(id);
	CFifoChannel *pChan = GetFifoChannel(id);

	ASSERT(pChan != NULL);

	pChan->m_lHost	= (lpszHostAddress != NULL ? lpszHostAddress : _T(""));
	pChan->m_lPort	= nHostPort;
	pChan->m_rHost	= (lpszRemoteAddress != NULL ? lpszRemoteAddress : _T(""));
	pChan->m_rPort	= nRemotePort;

	pChan->m_pFifoBase = new CFifoListen(m_pDocument, this);

	CFifoBase::Link(m_pFifoMid, nFd, pChan->m_pFifoBase, FIFO_STDIN);

	if ( !((CFifoListen *)pChan->m_pFifoBase)->Open(lpszHostAddress, nHostPort) )
		return FALSE;

	return TRUE;
}
BOOL Cssh::ChannelOpen(int id, LPCTSTR lpszHostAddress, UINT nHostPort)
{
	int nFd = IdToFdIn(id);
	CFifoChannel *pChan = GetFifoChannel(id);

	ASSERT(pChan != NULL);

	pChan->m_pFifoBase = new CFifoSocket(m_pDocument, this);

	CFifoBase::Link(m_pFifoMid, nFd, pChan->m_pFifoBase, FIFO_STDIN);

	if ( !((CFifoSocket *)pChan->m_pFifoBase)->Open(lpszHostAddress, nHostPort) )
		return FALSE;

	return TRUE;
}
/*
=================================
RecvEOF			CHAN_ENDOF_REMOTE	0
	SendEOF
	WriteBlock			0->1
RecvClose		CHAN_CLOSE_REMOTE	1
	SendClose
	IsOpen
		DoClose			1->2
	IsClosed
		ChannelClose	1->99
OnClose			CHAN_CLOSE_LOCAL	2
	ChannelClose		2->99
=================================
RecvEOF			CHAN_ENDOF_REMOTE	0
	SendEOF
	WriteBlock			0->1
OnClose			CHAN_CLOSE_LOCAL	1
	SendClose			1->3
RecvClose		CHAN_CLOSE_REMOTE	3
	ChannelClose		3->99
=================================
OnRead			CHAN_ENDOF_LOCAL	0
	SendEOF
	IsOpen
		DoClose			0->4
	IsClosed
		Stage			0->6
RecvClose		CHAN_CLOSE_REMOTE	4
	SendClose			4->5
OnClose			CHAN_CLOSE_LOCAL	5
	ChannelClose		5->99

RecvClose		CHAN_CLOSE_REMOTE	6
	SendClose
	ChannelClose		6->99
=================================
OnRead			CHAN_ENDOF_LOCAL	0
	SendEOF
	IsOpen
		DoClose			0->4
	IsClosed
		SendClose		0->5
OnClose			CHAN_CLOSE_LOCAL	4
	SendClose			4->5
RecvClose		CHAN_CLOSE_REMOTE	5
	ChannelClose		5->99
=================================
RecvClose		CHAN_CLOSE_REMOTE	0
	IsOpen
		DoClose			0->7
	IsClosed
		SendClose
		ChannelClose	0->99
OnClose			CHAN_CLOSE_LOCAL	7
	SendClose
	ChannelClose		7->99
=================================
OnClose			CHAN_CLOSE_LOCAL	0
	SendEOF				0->8
RecvClose		CHAN_CLOSE_REMOTE	8
	SendClose
	ChannelClose		8->99
=================================
*/
void Cssh::ChannelClose(int id, int nStat)
{
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	ASSERT(pChan != NULL);

#define	SendChanMsg(msg) { tmp.Clear(); tmp.Put8Bit(msg); tmp.Put32Bit(pChan->m_RemoteID); SendPacket2(&tmp); }

	//TRACE("ChannelClose #%d %d %d\n", id, nStat, pChan->m_Stage);

	ASSERT(nStat == 0 || pChan->m_RemoteID != (-1));

	switch(nStat) {
	case CHAN_ENDOF_LOCAL:		// OnRead
		if ( pChan->m_Stage == 0 ) {
			SendChanMsg(SSH2_MSG_CHANNEL_EOF);
			if ( !pChan->m_bClosed ) {
				m_pFifoMid->SendFdEvents(IdToFdOut(id), FD_CLOSE, NULL);
				m_pFifoMid->Read(IdToFdIn(id), NULL, 0);	// Channel Write Block
				pChan->m_Stage = 4;
			} else {
				pChan->m_Stage = 5;
			}
		}
		return;
	case CHAN_ENDOF_REMOTE:		// RecvEOF
		if ( pChan->m_Stage == 0 ) {
			pChan->m_Stage = 1;
			SendChanMsg(SSH2_MSG_CHANNEL_EOF);
			m_pFifoMid->Write(IdToFdOut(id), NULL, 0);	// Channel Read Endof
		}
		return;
	case CHAN_CLOSE_LOCAL:		// OnClose
		if ( pChan->m_Stage == 0 ) {
			pChan->m_Stage = 8;
			SendChanMsg(SSH2_MSG_CHANNEL_EOF);
		} else if ( pChan->m_Stage == 1 ) {
			pChan->m_Stage = 3;
			SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
		} else if ( pChan->m_Stage == 2 ) {
			pChan->m_Stage = 99;
			break;	// ChannelClose
		} else if ( pChan->m_Stage == 4 ) {
			pChan->m_Stage = 6;
			SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
		} else if ( pChan->m_Stage == 5 ) {
			pChan->m_Stage = 99;
			break;	// ChannelClose
		} else if ( pChan->m_Stage == 7 ) {
			pChan->m_Stage = 99;
			SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
			break;	// ChannelClose
		}
		return;
	case CHAN_CLOSE_REMOTE:		// RecvClose
		if ( pChan->m_Stage == 0 ) {
			if ( !pChan->m_bClosed ) {
				pChan->m_Stage = 7;
				m_pFifoMid->SendFdEvents(IdToFdOut(id), FD_CLOSE, NULL);
				m_pFifoMid->Read(IdToFdIn(id), NULL, 0);	// Channel Write Block
			} else {
				pChan->m_Stage = 99;
				SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
				break;	// ChannelClose
			}
		} else if ( pChan->m_Stage == 1 ) {
			SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
			if ( !pChan->m_bClosed ) {
				pChan->m_Stage = 2;
				m_pFifoMid->SendFdEvents(IdToFdOut(id), FD_CLOSE, NULL);
				m_pFifoMid->Read(IdToFdIn(id), NULL, 0);	// Channel Write Block
			} else {
				pChan->m_Stage = 99;
				break;	// ChannelClose
			}
		} else if ( pChan->m_Stage == 3 ) {
			pChan->m_Stage = 99;
			break;	// ChannelClose
		} else if ( pChan->m_Stage == 4 ) {
			pChan->m_Stage = 5;
			SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
		} else if ( pChan->m_Stage == 5 ) {
			pChan->m_Stage = 99;
			SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
			break;	// ChannelClose
		} else if ( pChan->m_Stage == 6 ) {
			pChan->m_Stage = 99;
			break;	// ChannelClose
		} else if ( pChan->m_Stage == 8 ) {
			pChan->m_Stage = 99;
			SendChanMsg(SSH2_MSG_CHANNEL_CLOSE);
			break;	// ChannelClose
		}
		return;
	}

	if ( pChan->m_pFifoBase != NULL ) {
		CFifoBase::UnLink(pChan->m_pFifoBase, FIFO_STDIN, FALSE);
		pChan->m_pFifoBase->Destroy();
	}

	switch(pChan->m_Type) {
	case SSHFT_NONE:
		LogIt(_T("Closed #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
		break;
	case SSHFT_STDIO:
		LogIt(_T("Closed #%d stdio"), id);
		break;
	case SSHFT_SFTP:
		LogIt(_T("Closed #%d sftp"), id);
		break;
	case SSHFT_AGENT:
		LogIt(_T("Closed #%d agent"), id);
		break;
	case SSHFT_RCP:
		LogIt(_T("Closed #%d rcp"), id);
		break;
	case SSHFT_X11:
		LogIt(_T("Closed #%d x11"), id);
		break;
	case SSHFT_LOCAL_LISTEN:
	case SSHFT_REMOTE_LISTEN:
		LogIt(_T("Listen Closed #%d %s:%d"), id, pChan->m_rHost, pChan->m_rPort);
		break;
	case SSHFT_LOCAL_SOCKS:
	case SSHFT_REMOTE_SOCKS:
		LogIt(_T("Sock Closed #%d %s:%d"), id, pChan->m_lHost, pChan->m_lPort);
		break;
	case SSHFT_SOCKET_LOCAL:
	case SSHFT_SOCKET_REMOTE:
		LogIt(_T("Closed #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
		break;
	}

	delete pChan;
	ASSERT(id < m_FifoCannel.GetSize());
	m_FifoCannel[id] = NULL;
	m_OpenChanCount--;

	if ( id < m_LastFreeId )
		m_LastFreeId = id;
	
	if ( id == FdToId(FIFO_EXTIN) && (m_SSH2Status & SSH2_STAT_HAVESTDIO) != 0 ) {
		m_SSH2Status &= ~SSH2_STAT_HAVESTDIO;

		if ( m_OpenChanCount > 0 && AfxMessageBox(CStringLoad(IDS_SSHCLOSEALL), MB_ICONQUESTION | MB_YESNO) == IDYES )
			PostClose(0);
	}

	if ( nStat != 0 && m_OpenChanCount == 0 && m_Permit.GetSize() == 0 )
		PostClose(0);
}
void Cssh::DelayedCheckExec()
{
	while ( !m_DelayedCheck.IsEmpty() ) {
		DelayedCheck *pDelayedCheck = (DelayedCheck *)m_DelayedCheck.RemoveHead();
		switch(pDelayedCheck->type) {
		case 0:
			ChannelCheck(pDelayedCheck->nFd, pDelayedCheck->fdEvent, pDelayedCheck->pParam);
			break;
		case 1:
			SendPacket((CBuffer *)pDelayedCheck->pParam);
			break;
		case 2:
			SendPacket2((CBuffer *)pDelayedCheck->pParam);
			break;
		}
		if ( pDelayedCheck->bDelBuf )
			delete (CBuffer *)pDelayedCheck->pParam;
		delete pDelayedCheck;
	}
}
void Cssh::ChannelCheck(int nFd, int fdEvent, void *pParam)
{
	int id = FdToId(nFd);
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp, *bp = (CBuffer *)pParam;

//	ASSERT(pChan != NULL);
	if ( pChan == NULL )
		return;

	if (  (m_SSH2Status & SSH2_STAT_SENTKEXINIT) != 0 ) {
		DelayedCheck *pDelayedCheck = new DelayedCheck;
		pDelayedCheck->type    = 0;
		pDelayedCheck->nFd     = nFd;
		pDelayedCheck->fdEvent = fdEvent;
		if ( (nFd & 1) != 0 && fdEvent == FD_READ && bp != NULL ) {	// EXTOUT SSH -> FifoChennel FD_READ pParam = CBuffer ptr
			CBuffer *pTemp = new CBuffer();
			pTemp->Swap(*bp);
			pDelayedCheck->pParam  = pTemp;
			pDelayedCheck->bDelBuf = TRUE;
		} else {
			pDelayedCheck->pParam  = pParam;
			pDelayedCheck->bDelBuf = FALSE;
		}
		m_DelayedCheck.AddTail(pDelayedCheck);
		return;
	}

	if ( (nFd & 1) == 0 ) {	
		// EXTIN  FifoChannel -> SSH
		switch(fdEvent) {
		case FD_READ:			// CFifoSsh::OnRead
			SendMsgChannelData(id);
			break;

		case FD_WRITE:			// CFifoSsh::OnWrite
			ChannelPolling(id);
			break;

		case FD_ACCEPT:			// CFifoSsh::OnAccept
			ChannelAccept(id, (SOCKET)pParam);
			break;

		case FD_CONNECT:		// CFifoSsh::OnConnect
			if ( (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
				SendMsgChannelOpenConfirmation(id);
			break;

		case FD_CLOSE:			// CFifoSsh::OnClose
			if ( (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
				SendMsgChannelOpenFailure(id);
			else if ( (pChan->m_Status & CHAN_OPEN_REMOTE) != 0 )
				ChannelClose(id, CHAN_CLOSE_LOCAL);
			else
				pChan->m_bClosed = TRUE;
			break;
		}

	} else {
		// EXTOUT SSH -> FifoChennel
		switch(fdEvent) {
		case FD_READ:			// SSH2MsgChannelData
			ASSERT(m_pFifoMid != NULL);
			m_pFifoMid->Write(IdToFdOut(id), bp->GetPtr(), bp->GetSize());	// FifoMid(Write)->FifoChannel
			break;

		case FD_WRITE:			// SSH2MsgChannelAdjust
			ASSERT(m_pFifoMid != NULL);
			m_pFifoMid->SetFdEvents(IdToFdIn(id), FD_READ);
			break;

		case FD_ACCEPT:			// CFifoSocks -> CFifoSsh::OnCommand(FIFO_QCMD_SYNCRET)
			SocksResult((CFifoSocks *)pParam);
			break;

		case FD_CONNECT:		// SSH2MsgChannelOpenReply
			ASSERT(m_pFifoMid != NULL);
			if ( !pChan->m_bClosed )
				m_pFifoMid->SendFdEvents(nFd, FD_CONNECT, NULL);
			else
				ChannelClose(id, CHAN_CLOSE_LOCAL);
			break;

		case FD_CLOSE:
			ChannelClose(id, CHAN_CLOSE_LOCAL);
			break;
		}
	}
}
void Cssh::ChannelPolling(int id)
{
	int len;
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	ASSERT(pChan != NULL);

	if ( (pChan->m_Status & (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE)) != (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE) )
		return;

	if ( (len = pChan->m_LocalComs - m_pFifoMid->GetDataSize(id)) >= (pChan->m_LocalWind * 3 / 4) ) {
		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_WINDOW_ADJUST);
		tmp.Put32Bit(pChan->m_RemoteID);
		tmp.Put32Bit(len);
		SendPacket2(&tmp);

		DEBUGLOG("WindowAdjust #%d %d - %d = %d (%d)", cp->m_LocalID, cp->m_LocalComs, cp->GetSendSize(), n, cp->m_LocalWind);

		pChan->m_LocalComs -= len;
	}
}
void Cssh::ChannelAccept(int id, SOCKET socket)
{
	int port;
	CString host[2];
	CFifoChannel *pChan = GetFifoChannel(id);
	CFifoChannel *pNewChan;

	ASSERT(pChan != NULL);

	if ( (pNewChan = GetFifoChannel(-1)) == NULL ) {
		::closesocket(socket);
		return;
	}
	
	CExtSocket::GetPeerName((SOCKET)socket, host[0], &port);

	if ( pChan->m_Type == SSHFT_LOCAL_LISTEN ) {
		pNewChan->m_Type = SSHFT_SOCKET_LOCAL;
		pNewChan->m_Status |= CHAN_OPEN_LOCAL;
		pNewChan->m_pFifoBase = new CFifoSocket(m_pDocument, this);

		CFifoBase::Link(m_pFifoMid, IdToFdIn(pNewChan->m_LocalID), pNewChan->m_pFifoBase, FIFO_STDIN);

		if ( !((CFifoSocket *)pNewChan->m_pFifoBase)->Attach(socket) ) {
			LogIt(_T("Local Listen attach error #%d %s:%d"), id, pChan->m_lHost, pChan->m_lPort);
			ChannelClose(pNewChan->m_LocalID);
			return;
		}

		host[1] = pChan->m_rHost;
		SendMsgChannelOpen(pNewChan->m_LocalID, "direct-tcpip", host[1], pChan->m_rPort, host[0], port);

	} else if ( pChan->m_Type == SSHFT_LOCAL_SOCKS ) {
		pNewChan->m_Type = SSHFT_SOCKET_LOCAL;
		pNewChan->m_Status |= CHAN_OPEN_LOCAL;
		pNewChan->m_lHost = host[0];
		pNewChan->m_lPort = port;
		pNewChan->m_pFifoBase = new CFifoSocket(m_pDocument, this);
		CFifoSocks *pFifoSocks = new CFifoSocks(m_pDocument, this, pNewChan);

		pFifoSocks->m_rFd = FIFO_EXTIN;
		pFifoSocks->m_wFd = FIFO_EXTOUT;

		CFifoBase::MidLink(m_pFifoMid, IdToFdIn(pNewChan->m_LocalID), pFifoSocks, FIFO_STDIN, pNewChan->m_pFifoBase, FIFO_STDIN);

		if ( !((CFifoSocket *)pNewChan->m_pFifoBase)->Attach(socket) )
			pFifoSocks->Abort();
	}
}
void Cssh::SocksResult(class CFifoSocks *pFifoSocks)
{
	CFifoChannel *pChan = GetFifoChannel(pFifoSocks->m_ChanId);
	CString host[2];

	ASSERT(pChan != NULL || pChan->m_LocalID != pFifoSocks->m_ChanId);

	if ( pChan->m_Type == SSHFT_REMOTE_SOCKS ) {
		if ( pFifoSocks->m_RetCode == 0 || (pFifoSocks->m_fdPostEvents[FIFO_STDIN] & FD_CLOSE) != 0 ) {	// STDIN = ssh channel
			LogIt(_T("Remote Socks Failure #%d %s:%d"), pChan->m_LocalID, pChan->m_lHost, pChan->m_lPort);
			pChan->m_pFifoBase->SendFdEvents(FIFO_STDOUT, FD_CLOSE, NULL);

		} else {
			CFifoSocket *pFifoSocket = new CFifoSocket(m_pDocument, this);

			pChan->m_rHost = LocalStr(pFifoSocks->m_HostName);
			pChan->m_rPort = pFifoSocks->m_HostPort;

			CFifoBase::Link(pFifoSocks, FIFO_EXTIN, pFifoSocket, FIFO_STDIN);
			CFifoBase::UnLink(pFifoSocks, FIFO_STDIN, TRUE);
			pFifoSocks->Destroy();

			pChan->m_Type = SSHFT_SOCKET_REMOTE;
			pChan->m_Status |= (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE);
			pChan->m_pFifoBase = pFifoSocket;

			if ( !pFifoSocket->Open(pChan->m_rHost, pChan->m_rPort) ) {
				LogIt(_T("Remote Socks Open Error #%d %s:%d"), pChan->m_LocalID, pChan->m_rHost, pChan->m_rPort);
				pChan->m_pFifoBase->SendFdEvents(FIFO_STDOUT, FD_CLOSE, NULL);
			}
		}

	} else if ( pChan->m_Type == SSHFT_SOCKET_LOCAL ) {
		if ( pFifoSocks->m_RetCode == 0 || (pFifoSocks->m_fdPostEvents[FIFO_EXTIN] & FD_CLOSE) != 0 ) {	// EXTIN = socket
			LogIt(_T("Local Socks %s #%d %s:%d"), 
				(pFifoSocks->m_fdPostEvents[FIFO_EXTIN] & FD_CLOSE) != 0 ? _T("Closed") : _T("Failure"),
				pChan->m_LocalID, pChan->m_lHost, pChan->m_lPort);

			CFifoBase::UnLink(pFifoSocks, FIFO_STDIN, TRUE);
			pFifoSocks->Destroy();

			pChan->m_rHost.Empty();
			pChan->m_rPort = 0;

			ChannelClose(pChan->m_LocalID);

		} else {
			host[0] = LocalStr(pFifoSocks->m_HostName);
			host[1] = pChan->m_lHost;
			SendMsgChannelOpen(pChan->m_LocalID, "direct-tcpip", host[0], pFifoSocks->m_HostPort, host[1], pChan->m_lPort);

			CFifoBase::UnLink(pFifoSocks, FIFO_STDIN, TRUE);
			pFifoSocks->Destroy();
		}
	}
}

void Cssh::PortForward()
{
	int n, i, a = 0;
	CString str;
	CStringArrayExt tmp;
	CFifoChannel *pChan;

	m_bPfdConnect = 0;
	m_PortFwdTable = m_pDocument->m_ParamTab.m_PortFwd;

	if ( m_pDocument->m_ServerEntry.m_ReEntryFlag && !m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) )
		return;

	for ( i = 0 ; i < m_pDocument->m_ParamTab.m_PortFwd.GetSize() ; i++ ) {
		m_pDocument->m_ParamTab.m_PortFwd.GetArray(i, tmp);
		if ( tmp.GetSize() <= 4 )
			continue;

		if ( tmp.GetSize() > 5 && tmp.GetVal(5) == 0 )
			continue;

		switch(tmp.GetVal(4)) {
		case PFD_LOCAL:
			if ( (pChan = GetFifoChannel(-1)) == NULL )
				break;
			pChan->m_Type = SSHFT_LOCAL_LISTEN;
			pChan->m_TypeName = _T("tcpip-listen");
			if ( !ChannelCreate(pChan->m_LocalID, tmp[0], GetPortNum(tmp[1]), tmp[2], GetPortNum(tmp[3])) ) {
				str.Format(_T("Port Forward Error %s:%s->%s:%s"), (LPCTSTR)tmp[0], (LPCTSTR)tmp[1], (LPCTSTR)tmp[2], (LPCTSTR)tmp[3]);
				AfxMessageBox(str);
				ChannelClose(pChan->m_LocalID);
			} else {
				LogIt(_T("Local Listen %s:%s"), (LPCTSTR)tmp[0], (LPCTSTR)tmp[1]);
				a++;
			}
			break;

		case PFD_SOCKS:
			if ( (pChan = GetFifoChannel(-1)) == NULL )
				break;
			pChan->m_Type = SSHFT_LOCAL_SOCKS;
			pChan->m_TypeName = _T("socks-listen");
			if ( !ChannelCreate(pChan->m_LocalID, tmp[0], GetPortNum(tmp[1]), tmp[2], GetPortNum(tmp[3])) ) {
				str.Format(_T("Socks Listen Error %s:%s->%s:%s"), (LPCTSTR)tmp[0], (LPCTSTR)tmp[1], (LPCTSTR)tmp[2], (LPCTSTR)tmp[3]);
				AfxMessageBox(str);
				ChannelClose(pChan->m_LocalID);
			} else {
				LogIt(_T("Local Socks %s:%s"), (LPCTSTR)tmp[0], (LPCTSTR)tmp[1]);
				a++;
			}
			break;

		case PFD_REMOTE:
		case PFD_RSOCKS:
			n = (int)m_Permit.GetSize();
			m_Permit.SetSize(n + 1);
			m_Permit[n].m_lHost = tmp[2];
			m_Permit[n].m_lPort = GetPortNum(tmp[3]);
			m_Permit[n].m_rHost = tmp[0];
			m_Permit[n].m_rPort = GetPortNum(tmp[1]);
			m_Permit[n].m_Type  = tmp.GetVal(4);
			SendMsgGlobalRequest(n, "tcpip-forward", tmp[0], GetPortNum(tmp[1]));
			LogIt(_T("Remote %s %s:%s"), (m_Permit[n].m_Type == PFD_REMOTE ? _T("Listen") : _T("Socks")), (LPCTSTR)tmp[0], (LPCTSTR)tmp[1]);
			m_bPfdConnect++;
			a++;
			break;
		}
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) ) {
		if ( a == 0 && !m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFTPORY) ) {
			AfxMessageBox(CStringLoad(IDE_PORTFWORDERROR));
			PostClose(0);
		}

		if ( m_bPfdConnect == 0 ) {
			m_bConnect = TRUE;
			m_pFifoMid->SendFdEvents(FIFO_EXTOUT, FD_CONNECT, NULL);
		}
	} else
		m_bPfdConnect = 0;
}

//////////////////////////////////////////////////////////////////////

void Cssh::OpenSFtpChannel()
{
	if ( (m_SSH2Status & SSH2_STAT_HAVELOGIN) == 0 )
		return;

	CFifoChannel *pChan = GetFifoChannel(-1);

	if ( pChan == NULL )
		return;

	pChan->m_Type = SSHFT_SFTP;
	pChan->m_pFifoBase = new CFifoSftp(m_pDocument, this, new CSFtp(NULL));

	((CFifoSftp *)pChan->m_pFifoBase)->m_pSFtp->m_pSSh = this;
	((CFifoSftp *)pChan->m_pFifoBase)->m_pSFtp->m_pFifoSftp = pChan->m_pFifoBase;
	((CFifoSftp *)pChan->m_pFifoBase)->m_pSFtp->m_HostKanjiSet = m_pDocument->m_TextRam.m_SendCharSet[m_pDocument->m_TextRam.m_KanjiMode];
	((CFifoSftp *)pChan->m_pFifoBase)->m_pSFtp->Create(IDD_SFTPDLG, CWnd::GetDesktopWindow());
	((CFifoSftp *)pChan->m_pFifoBase)->m_pSFtp->ShowWindow(SW_SHOW);

	CFifoBase::Link(m_pFifoMid, IdToFdIn(pChan->m_LocalID), pChan->m_pFifoBase, FIFO_STDIN);

	PostSendMsgChannelOpen(pChan->m_LocalID, "session");
}

static LPCTSTR GetFileName(LPCTSTR path)
{
	LPCTSTR p;
	return ((p = _tcsrchr(path, _T('\\'))) != NULL || (p = _tcsrchr(path, _T(':'))) != NULL ? (p + 1) : path);
}
static LPCTSTR ShellEscape(LPCTSTR str, CString &work)
{
	CString tmp;

	while ( *str != _T('\0') ) {
		if ( _tcschr(_T(" \"$@&'()^|[]{};*?<>`\\"), *str) != NULL ) {
			work += _T('\\');
			work += *(str++);
		} else if ( *str == _T('\t') ) {
			work += _T("\\t");
			str++;
		} else if ( *str == _T('\r') ) {
			work += _T("\\r");
			str++;
		} else if ( *str == _T('\n') ) {
			work += _T("\\n");
			str++;
		} else if ( *str < _T(' ') ) {
			tmp.Format(_T("\\%03o"), *str);
			work += tmp;
			str++;
		} else {
			work += *(str++);
		}
	}

	return work;
}
void Cssh::OpenRcpUpload(LPCTSTR file)
{
	if ( file == NULL ) {
		if ( !m_FileList.IsEmpty() )
			m_FileList.RemoveHead();
		if ( m_FileList.IsEmpty() )
			return;
		file = m_FileList.GetHead();
	} else {
		if ( !m_FileList.IsEmpty() ) {
			m_FileList.AddTail(file);
			return;
		}
		m_FileList.AddTail(file);
	}

	CFifoChannel *pChan = GetFifoChannel(-1);
	LPCTSTR last = GetFileName(file);
	CString work;

	if ( pChan == NULL )
		return;

	pChan->m_Type = SSHFT_RCP;
	pChan->m_rHost.Format(m_pDocument->m_TextRam.m_DropFileCmd[5], ShellEscape(last, work));
	pChan->m_pFifoBase = new CFifoRcp(m_pDocument, this, pChan, 0);	// rcp file upload = 0

	((CFifoRcp *)pChan->m_pFifoBase)->m_PathName = file;
	((CFifoRcp *)pChan->m_pFifoBase)->m_FileName = last;

	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg = new CProgDlg(NULL);
	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg->m_pAbortFlag = &(((CFifoRcp *)pChan->m_pFifoBase)->m_bAbortFlag);
	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg->Create(IDD_PROGDLG, ::AfxGetMainWnd());
	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg->SetWindowText(_T("scp File upload"));
	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg->ShowWindow(SW_SHOW);

	CFifoBase::Link(m_pFifoMid, IdToFdIn(pChan->m_LocalID), pChan->m_pFifoBase, FIFO_STDIN);

	PostSendMsgChannelOpen(pChan->m_LocalID, "session");
}
void Cssh::OpenRcpDownload(LPCTSTR file)
{
	CFifoChannel *pChan = GetFifoChannel(-1);
	CString work;

	if ( pChan == NULL )
		return;

	wchar_t *MyDownload;
	SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &MyDownload);

	pChan->m_Type = SSHFT_RCP;
	pChan->m_rHost.Format(_T("scp -p -r -f %s"), ShellEscape(file, work));
	pChan->m_pFifoBase = new CFifoRcp(m_pDocument, this, pChan, 1);	// rcp file download = 1

	((CFifoRcp *)pChan->m_pFifoBase)->m_PathName = UniToTstr(MyDownload);
	((CFifoRcp *)pChan->m_pFifoBase)->m_FileName = file;

	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg = new CProgDlg(NULL);
	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg->Create(IDD_PROGDLG, ::AfxGetMainWnd());
	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg->SetWindowText(_T("scp File download"));
	((CFifoRcp *)pChan->m_pFifoBase)->m_pProgDlg->ShowWindow(SW_SHOW);

	CFifoBase::Link(m_pFifoMid, IdToFdIn(pChan->m_LocalID), pChan->m_pFifoBase, FIFO_STDIN);

	PostSendMsgChannelOpen(pChan->m_LocalID, "session");
}

//////////////////////////////////////////////////////////////////////

void Cssh::SendMsgKexInit()
{
	CBuffer tmp(-1);

	if ( m_MyPeer.GetSize() == 0 || (m_SSH2Status & SSH2_STAT_SENTKEXINIT) != 0 )
		return;

	m_SSH2Status |= SSH2_STAT_SENTKEXINIT;

	tmp.Put8Bit(SSH2_MSG_KEXINIT);
	tmp.Apend(m_MyPeer.GetPtr(), m_MyPeer.GetSize());
	SendPacket2(&tmp);

	m_SendPackLen = 0;
}
void Cssh::SendMsgKexDhInit()
{
	CBuffer tmp(-1);

	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);

	if ( m_DhMode == DHMODE_GROUP_1 )
		m_SaveDh = dh_new_group1();
	else if ( m_DhMode == DHMODE_GROUP_14 )
		m_SaveDh = dh_new_group14();
	else if ( m_DhMode == DHMODE_GROUP_14_256 )
		m_SaveDh = dh_new_group14();
	else if ( m_DhMode == DHMODE_GROUP_15_512 )
		m_SaveDh = dh_new_group15();
	else if ( m_DhMode == DHMODE_GROUP_16_512 )
		m_SaveDh = dh_new_group16();
	else if ( m_DhMode == DHMODE_GROUP_17_512 )
		m_SaveDh = dh_new_group17();
	else if ( m_DhMode == DHMODE_GROUP_18_512 )
		m_SaveDh = dh_new_group18();

	dh_gen_key(m_SaveDh, m_NeedKeyLen * 8);

	BIGNUM const *pub_key = NULL;

	DH_get0_key(m_SaveDh, &pub_key, NULL);

	tmp.Put8Bit(SSH2_MSG_KEXDH_INIT);
	tmp.PutBIGNUM2(pub_key);

	SendPacket2(&tmp);
}
void Cssh::SendMsgKexDhGexRequest()
{
	CBuffer tmp(-1);
	
	m_DhGexReqBits = dh_estimate(m_NeedKeyLen * 8);

	if ( m_DhGexReqBits < DHGEX_MIN_BITS )
		m_DhGexReqBits = DHGEX_MIN_BITS;
	else if ( m_DhGexReqBits > DHGEX_MAX_BITS )
		m_DhGexReqBits = DHGEX_MAX_BITS;

	if ( m_DhGexReqBits > 4096 && IsServerVersion(_T("Cisco-1.")) )
		m_DhGexReqBits = 4096;

	tmp.Put8Bit(SSH2_MSG_KEX_DH_GEX_REQUEST);
	tmp.Put32Bit(DHGEX_MIN_BITS);
	tmp.Put32Bit(m_DhGexReqBits);
	tmp.Put32Bit(DHGEX_MAX_BITS);
	SendPacket2(&tmp);
}
int Cssh::SendMsgKexEcdhInit()
{
	CBuffer tmp(-1);

	switch(m_DhMode) {
	default:
	case DHMODE_ECDH_S2_N256:
		m_EcdhCurveNid = NID_X9_62_prime256v1;
		break;
	case DHMODE_ECDH_S2_N384:
		m_EcdhCurveNid = NID_secp384r1;
		break;
	case DHMODE_ECDH_S2_N521:
		m_EcdhCurveNid = NID_secp521r1;
		break;
	}

	if ( m_EcdhClientKey != NULL )
		EC_KEY_free(m_EcdhClientKey);

	if ( (m_EcdhClientKey = EC_KEY_new_by_curve_name(m_EcdhCurveNid)) == NULL )
		return FALSE;
	if ( EC_KEY_generate_key(m_EcdhClientKey) != 1 )
		return FALSE;
	if ( (m_EcdhGroup = EC_KEY_get0_group(m_EcdhClientKey)) == NULL )
		return FALSE;

	tmp.Put8Bit(SSH2_MSG_KEX_ECDH_INIT);
	tmp.PutEcPoint(m_EcdhGroup, EC_KEY_get0_public_key(m_EcdhClientKey));
	SendPacket2(&tmp);

	return TRUE;
}
void Cssh::SendMsgKexCurveInit()
{
	CBuffer tmp(-1);
	EVP_PKEY_CTX *ctx = NULL;
	size_t len = 0;
	int type = EVP_PKEY_X25519;

	m_CurveClientPubkey.Clear();

	switch(m_DhMode) {
	case DHMODE_CURVE25519:
		type = EVP_PKEY_X25519;
		break;
	case DHMODE_CURVE448:
		type = EVP_PKEY_X448;
		break;
	case DHMODE_SNT761X25519:
		type = EVP_PKEY_X25519;
		m_CurveClientPubkey.PutSpc(sntrup761_PUBLICKEYBYTES);
		m_SntrupClientKey.Clear();
		sntrup761_keypair(m_CurveClientPubkey.GetPtr(), m_SntrupClientKey.PutSpc(sntrup761_SECRETKEYBYTES));
		break;
	}

	if ( (ctx = EVP_PKEY_CTX_new_id(type, NULL)) == NULL )
		goto ENDOF;

	if ( EVP_PKEY_keygen_init(ctx) <= 0 )
		goto ENDOF;

	if ( m_CurveEvpKey != NULL )
		EVP_PKEY_free(m_CurveEvpKey);
	m_CurveEvpKey = NULL;

	if ( EVP_PKEY_keygen(ctx, &m_CurveEvpKey) <= 0 )
		goto ENDOF;

	if ( EVP_PKEY_get_raw_public_key(m_CurveEvpKey, NULL, &len) <= 0 )
		goto ENDOF;

	if ( EVP_PKEY_get_raw_public_key(m_CurveEvpKey, m_CurveClientPubkey.PutSpc((int)len), &len) <= 0 )
		goto ENDOF;

	tmp.Put8Bit(SSH2_MSG_KEX_ECDH_INIT);
	tmp.PutBuf(m_CurveClientPubkey.GetPtr(), m_CurveClientPubkey.GetSize());
	SendPacket2(&tmp);

ENDOF:
	if ( ctx != NULL )
		EVP_PKEY_CTX_free(ctx);
}
void Cssh::SendMsgNewKeys()
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_NEWKEYS);
	SendPacket2(&tmp);

	if ( !m_EncCip.Init(m_VProp[PROP_ENC_ALGS_CTOS], MODE_ENC, m_VKey[2], (-1), m_VKey[0]) ||
		 !m_EncMac.Init(m_VProp[PROP_MAC_ALGS_CTOS], m_VKey[4], (-1)) ||
		 !m_EncCmp.Init(m_VProp[PROP_COMP_ALGS_CTOS], MODE_ENC, COMPLEVEL) )
		Close();

	if ( m_bExtInfo ) {
		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_EXT_INFO);
		tmp.Put32Bit(1);
		tmp.PutStr("server-sig-algs");
		tmp.PutStr("rsa-sha2-256,rsa-sha2-512");
		SendPacket2(&tmp);
		// 最初だけ送れば良い？
		m_bExtInfo = FALSE;
	}
}

//////////////////////////////////////////////////////////////////////

#define	SSH2_GSS_OIDTYPE	0x06

static const struct _gssapi_kerberos_mech {
	BYTE	len;
	BYTE	*data;
} gssapi_kerberos_mech = { 9, (BYTE *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x02" };	// Kerberos : iso(1) member-body(2) United States(840) mit(113554) infosys(1) gssapi(2) krb5(2)

void Cssh::SendMsgServiceRequest(LPCSTR str)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_SERVICE_REQUEST);
	tmp.PutStr(str);
	SendPacket2(&tmp);
}

int Cssh::SendMsgUserAuthRequest(LPCSTR str)
{
	int skip, len;
	CBuffer tmp(-1), blob(-1), sig(-1);
	CString wrk;
	CPassDlg dlg;
	CStringA meta;

	tmp.PutBuf(m_SessHash.GetPtr(), m_SessHash.GetSize());
	skip = tmp.GetSize();
	tmp.Put8Bit(SSH2_MSG_USERAUTH_REQUEST);
	tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_UserName));
	tmp.PutStr("ssh-connection");

	if ( str == NULL ) {
		meta = m_AuthMeta;
		str = meta;
	} else if ( m_AuthMeta.IsEmpty() ) {
		m_AuthMeta = str;
	} else if ( m_AuthMeta.Compare(MbsToTstr(str)) != 0 ) {
		// 複数認証の場合、最初に戻す必要がある
		// Methodsが変化しない場合は、認証順位をユーザーが管理する必要あり
		m_AuthMeta = str;
		m_AuthStat = AST_START;
		m_bKeybIntrReq = FALSE;
		UserAuthNextState();
	}

	while ( m_AuthStat != AST_DONE ) {

		if ( m_AuthStat == AST_START ) {
			tmp.PutStr("none");
			m_AuthMeta.Empty();
			UserAuthNextState();

		} else if ( m_AuthStat == AST_PUBKEY_TRY ) {
			if ( strstr(str, "publickey") == NULL ) {
				UserAuthNextState();
				continue;
			}

			m_AuthMode = AUTH_MODE_NONE;

			while ( m_IdKeyPos < m_IdKeyTab.GetSize() ) {
				m_pIdKey = &(m_IdKeyTab[m_IdKeyPos++]);

				if ( m_pIdKey->m_Type == IDKEY_RSA1 )
					m_pIdKey->m_Type = IDKEY_RSA2;
				m_pIdKey->SetBlob(&blob);

				len = tmp.GetSize();
				tmp.PutStr("publickey");

				if ( m_pIdKey->Init(m_pDocument->m_ServerEntry.m_PassName) ) {
					tmp.Put8Bit(1);
					tmp.PutStr(TstrToMbs(m_pIdKey->GetName(TRUE, FALSE)));
					tmp.PutBuf(blob.GetPtr(), blob.GetSize());

					if ( !m_pIdKey->Sign(&sig, tmp.GetPtr(), tmp.GetSize(), GetSigAlgs()) ) {
						tmp.ConsumeEnd(tmp.GetSize() - len);
						continue;
					}

					tmp.PutBuf(sig.GetPtr(), sig.GetSize());

					AddAuthLog(_T("publickey(%s)"), m_pIdKey->GetName(TRUE, TRUE));
				} else {
					tmp.Put8Bit(0);
					tmp.PutStr(TstrToMbs(m_pIdKey->GetName(TRUE, FALSE)));
					tmp.PutBuf(blob.GetPtr(), blob.GetSize());

					AddAuthLog(_T("publickey:offered(%s)"), m_pIdKey->GetName(TRUE, TRUE));
				}

				if ( m_pIdKey->m_Type == IDKEY_RSA2 ) // && m_pIdKey->m_RsaNid != NID_sha1 )
					m_bReqRsaSha1 = TRUE;

				m_AuthMode = AUTH_MODE_PUBLICKEY;
				break;
			}

			if ( m_AuthMode != AUTH_MODE_PUBLICKEY ) {
				UserAuthNextState();
				continue;
			}

		} else if ( m_AuthStat == AST_HOST_TRY ) {
			if ( strstr(str, "hostbased") == NULL ) {
				UserAuthNextState();
				continue;
			}

			m_AuthMode = AUTH_MODE_NONE;

			while ( m_IdKeyPos < m_IdKeyTab.GetSize() ) {
				m_pIdKey = &(m_IdKeyTab[m_IdKeyPos++]);

				if ( !m_pIdKey->InitPass(m_pDocument->m_ServerEntry.m_PassName) )
					continue;
				m_pIdKey->SetBlob(&blob);

				len = tmp.GetSize();
				tmp.PutStr("hostbased");
				tmp.PutStr(TstrToMbs(m_pIdKey->GetName(TRUE, FALSE)));
				tmp.PutBuf(blob.GetPtr(), blob.GetSize());
				GetSockName(m_Fd, wrk, &len);
				tmp.PutStr(RemoteStr(wrk));			// client ip address
				m_pIdKey->GetUserHostName(wrk);
				tmp.PutStr(RemoteStr(wrk));			// client user name;

				if ( !m_pIdKey->Sign(&sig, tmp.GetPtr(), tmp.GetSize(), GetSigAlgs()) ) {
					tmp.ConsumeEnd(tmp.GetSize() - len);
					continue;
				}

				tmp.PutBuf(sig.GetPtr(), sig.GetSize());

				AddAuthLog(_T("hostbased(%s)"), m_pIdKey->GetName(TRUE, TRUE));

				m_AuthMode = AUTH_MODE_HOSTBASED;
				break;
			}

			if ( m_AuthMode != AUTH_MODE_HOSTBASED ) {
				UserAuthNextState();
				continue;
			}

		} else if ( m_AuthStat == AST_PASS_TRY ) {
			if ( strstr(str, "password") == NULL ) {
				UserAuthNextState();
				continue;
			}

			// ２度目以降のパスワードなら
			if ( m_IdKeyPos > 0 ) {
				dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
				dlg.m_PortName = m_pDocument->m_ServerEntry.m_PortName;
				dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
				dlg.m_PassName = m_pDocument->m_ServerEntry.m_PassName;
				dlg.m_Prompt   = _T("ssh Password");
				dlg.m_Enable   = PASSDLG_PASS;

				if ( dlg.DoModal() != IDOK ) {
					UserAuthNextState();
					continue;
				}

				m_pDocument->m_ServerEntry.m_PassName = dlg.m_PassName;
			}

			tmp.PutStr("password");
			tmp.Put8Bit(0);
			tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_PassName));

			AddAuthLog(_T("password(%s)"), m_pDocument->m_ServerEntry.m_UserName);

			m_IdKeyPos++;
			m_AuthMode = AUTH_MODE_PASSWORD;

		} else if ( m_AuthStat == AST_KEYB_TRY ) {
			if ( strstr(str, "keyboard-interactive") == NULL || m_bKeybIntrReq ) {
				UserAuthNextState();
				continue;
			}

			tmp.PutStr("keyboard-interactive");
			tmp.PutStr("");		// LANG
			tmp.PutStr("");		// DEV

			m_AuthMode = AUTH_MODE_KEYBOARD;
			// SSH2_MSG_USERAUTH_INFO_REQUESTでクリア
			m_bKeybIntrReq = TRUE;

		} else if ( m_AuthStat == AST_GSSAPI_TRY ) {
			UserAuthNextState();

			if ( strstr(str, "gssapi-with-mic") == NULL )
				continue;
			
			if ( (m_SSH2Status & SSH2_STAT_AUTHGSSAPI) != 0 )
				continue;

			if ( AcquireCredentialsHandle(NULL, _T("Kerberos"), SECPKG_CRED_OUTBOUND, NULL, NULL, NULL, NULL, &m_SSPI_hCredential, &m_SSPI_tsExpiry) != SEC_E_OK )
				continue;

			m_SSPI_phContext = NULL;
			m_SSPI_TargetName.Format(_T("host/%s"), (LPCTSTR)m_HostName);
			m_SSH2Status |= SSH2_STAT_AUTHGSSAPI;

			tmp.PutStr("gssapi-with-mic");
			tmp.Put32Bit(1);
			tmp.Put32Bit(gssapi_kerberos_mech.len + 2);
			tmp.Put8Bit(SSH2_GSS_OIDTYPE);
			tmp.Put8Bit(gssapi_kerberos_mech.len);
			tmp.Apend(gssapi_kerberos_mech.data, gssapi_kerberos_mech.len);

			AddAuthLog(_T("gssapi-with-mic(%s)"), m_HostName);

			m_AuthMode = AUTH_MODE_GSSAPI;
		}

		tmp.Consume(skip);
		SendPacket2(&tmp);
		return TRUE;
	}

	m_AuthStat = AST_DONE;
	m_AuthMode = AUTH_MODE_NONE;

	if ( m_AuthLog.IsEmpty() )
		m_AuthLog = _T("none");

	SendDisconnect2(SSH2_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT, "User Auth Failure");
	wrk.Format(_T("SSH2 User Auth Failure \"%s\" Status=%04o\nSend Disconnect Message...\n%s"), MbsToTstr(str), m_SSH2Status, m_AuthLog);
	if ( m_bReqRsaSha1 ) wrk += _T("\n\nYou may need to change the ssh-rsa key sign algorithm SHA1 to SHA2-256/512");
	AfxMessageBox(wrk, MB_ICONSTOP);
	
	return FALSE;
}

//////////////////////////////////////////////////////////////////////

void Cssh::PostSendMsgChannelOpen(int id, LPCSTR type)
{
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	ASSERT(pChan != NULL);

	pChan->m_TypeName = type;
	pChan->m_Status |= CHAN_OPEN_LOCAL;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN);
	tmp.PutStr(type);
	tmp.Put32Bit(pChan->m_LocalID);
	tmp.Put32Bit(pChan->m_LocalWind);
	tmp.Put32Bit(pChan->m_LocalPacks);

	PostSendPacket(2, &tmp);
}
int Cssh::SendMsgChannelOpen(int id, LPCSTR type, LPCTSTR lhost, int lport, LPCTSTR rhost, int rport)
{
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	if ( pChan == NULL )
		return (-1);

	pChan->m_TypeName = type;
	pChan->m_Status |= CHAN_OPEN_LOCAL;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN);
	tmp.PutStr(type);
	tmp.Put32Bit(pChan->m_LocalID);
	tmp.Put32Bit(pChan->m_LocalWind);
	tmp.Put32Bit(pChan->m_LocalPacks);

	if ( lhost != NULL ) {
		tmp.PutStr(RemoteStr(lhost));
		tmp.Put32Bit(lport);
		tmp.PutStr(RemoteStr(rhost));
		tmp.Put32Bit(rport);
		pChan->m_lHost = lhost;
		pChan->m_lPort = lport;
		pChan->m_rHost = rhost;
		pChan->m_rPort = rport;
	}

	SendPacket2(&tmp);
	return pChan->m_LocalID;
}
void Cssh::SendMsgChannelOpenConfirmation(int id)
{
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	ASSERT(pChan != NULL);

	pChan->m_Status |= CHAN_OPEN_LOCAL;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_CONFIRMATION);
	tmp.Put32Bit(pChan->m_RemoteID);
	tmp.Put32Bit(pChan->m_LocalID);
	tmp.Put32Bit(pChan->m_LocalWind);
	tmp.Put32Bit(pChan->m_LocalPacks);
	SendPacket2(&tmp);

	if ( pChan->m_Type >= SSHFT_LOCAL_LISTEN )
		LogIt(_T("Open #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
	else
		LogIt(_T("Open #%d Filter %d"), id, pChan->m_Type);
}
void Cssh::SendMsgChannelOpenFailure(int id)
{
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	ASSERT(pChan != NULL);

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_FAILURE);
	tmp.Put32Bit(pChan->m_RemoteID);
	tmp.Put32Bit(SSH2_OPEN_CONNECT_FAILED);
	tmp.PutStr("open failed");
	tmp.PutStr("");
	SendPacket2(&tmp);

	ChannelClose(id);
}
void Cssh::SendMsgChannelData(int id)
{
	int len;
	CBuffer tmp(CHAN_SES_PACKET_DEFAULT + 16);
	CFifoChannel *pChan = GetFifoChannel(id);
	BYTE *buf;

	if ( pChan == NULL || (pChan->m_Status & (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE)) != (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE) )
		return;

	for ( ; ; ) {
		if ( (len = pChan->m_RemotePacks) > pChan->m_RemoteComs )
			len = pChan->m_RemoteComs;

		if ( len <= 0 ) {
			m_pFifoMid->ResetFdEvents(IdToFdIn(id), FD_READ);
			break;
		}

		buf = m_pFifoMid->TempBuffer(len);

		if ( (len = m_pFifoMid->Read(IdToFdIn(id), buf, len)) < 0 ) {
			// End of Data
			ChannelClose(id, CHAN_ENDOF_LOCAL);
			break;
		} else if ( len == 0 ) {
			// No data
			break;
		}

		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_DATA);
		tmp.Put32Bit(pChan->m_RemoteID);
		tmp.PutBuf(buf, len);
		SendPacket2(&tmp);

		pChan->m_RemoteComs -= len;
	}
}

//////////////////////////////////////////////////////////////////////

void Cssh::SendMsgChannelRequesstShell(int id)
{
	int n;
	int cx = 0, cy = 0, sx = 0, sy = 0;
	LPCTSTR p;
	CBuffer tmp, tmode, dump;
	CString str;
	CStringIndex env;
	CFifoChannel *pChan = GetFifoChannel(id);

	//static const struct _dummy_ttymode {
	//	BYTE	opcode;
	//	DWORD	param;
	//} ttymode[] = {
	////	VINTR		VQUIT		VERASE		VKILL		VEOF		VEOL		VEOL2		VSTART
	//	{ 1,3 },	{ 2,28 },	{ 3,8 },	{ 4,21 },	{ 5,4 }, 	{ 6,255 },	{ 7,255 },	{ 8,17 },
	////	VSTOP		VSUSP		VDSUSP		VREPRINT	VWERASE		VLNEXT		VSTATUS		VDISCARD
	//	{ 9,19 },	{ 10,26 },	{ 11,25 },	{ 12,18 },	{ 13,23 },	{ 14,22 },	{ 17,20 },	{ 18,15 },
	////	IGNPAR		PARMRK		INPCK		ISTRIP		INLCR		IGNCR		ICRNL		IXON
	//	{ 30,0 },	{ 31,0 },	{ 32,0 },	{ 33,0 },	{ 34,0 },	{ 35,0 },	{ 36,1 },	{ 38,1 },
	////	IXANY		IXOFF		IMAXBEL
	//	{ 39,1 },	{ 40,0 },	{ 41,1 },
	////	ISIG		ICANON		ECHO		ECHOE		ECHOK		ECHONL		NOFLSH		TOSTOP
	//	{ 50,1 },	{ 51,1 },	{ 53,1 },	{ 54,1 },	{ 55,0 },	{ 56,0 },	{ 57,0 },	{ 58,0 },
	////	IEXTEN		ECHOCTL		ECHOKE		PENDIN
	//	{ 59,1 },	{ 60,1 },	{ 61,1 },	{ 62,1 },
	////	OPOST		ONLCR		OCRNL		ONOCR		ONLRET
	//	{ 70,1 },	{ 72,1 },	{ 73,0 },	{ 74,0 },	{ 75,0 },
	////	CS7			CS8			PARENB		PARODD
	//	{ 90,1 },	{ 91,1 },	{ 92,0 },	{ 93,0 },
	////	OSPEED			ISPEED
	//	{ 129,9600 },	{ 128,9600 },
	//	{ 0, 0 }
	//};

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHAGENT) ) {
		tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
		tmp.Put32Bit(pChan->m_RemoteID);
		tmp.PutStr("auth-agent-req@openssh.com");
		tmp.Put8Bit(0);
		SendPacket2(&tmp);
		m_bAuthAgentReqEnable = TRUE;
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHX11PF) ) {
		if ( (p = _tgetenv(_T("DISPLAY"))) == NULL ) {
			if ( m_pDocument->m_ParamTab.m_XDisplay.IsEmpty() )
				p = _T(":0");
			else
				p = m_pDocument->m_ParamTab.m_XDisplay;
		}

		while ( *p != _T('\0') && *p != _T(':') )
			str += *(p++);

		if ( str.IsEmpty() || str.CompareNoCase(_T("unix")) == 0 )
			str = _T("127.0.0.1");

		if ( *(p++) == _T(':') ) { // && CExtSocket::SokcetCheck(str, 6000 + atoi(p)) ) {
			n = 0;	// screen number
			while ( *p != _T('\0') && *p != _T('.') )
				p++;
			if ( *p == _T('.') )
				n = _tstoi(++p);

			m_x11AuthName = _T("MIT-MAGIC-COOKIE-1");
			rand_buf(m_x11AuthData.PutSpc(16), 16);
			dump.Base16Encode(m_x11AuthData.GetPtr(), m_x11AuthData.GetSize());

			m_x11LocalName = m_pDocument->m_ParamTab.m_x11AuthName;
			m_x11LocalData.Base64Decode(m_pDocument->m_ParamTab.m_x11AuthData);

			tmp.Clear();
			tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
			tmp.Put32Bit(pChan->m_RemoteID);
			tmp.PutStr("x11-req");
			tmp.Put8Bit(0);
			tmp.Put8Bit(0);		// XXX bool single connection
			tmp.PutStr(TstrToMbs(m_x11AuthName));
			tmp.PutStr(TstrToMbs((LPCTSTR)dump));
			tmp.Put32Bit(n);	// screen number
			SendPacket2(&tmp);
		}
	}

	if ( m_pDocument != NULL )
		m_pDocument->m_TextRam.GetScreenSize(&cx, &cy, &sx, &sy);

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(pChan->m_RemoteID);
	tmp.PutStr("pty-req");
	tmp.Put8Bit(1);
	tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_TermName));
	tmp.Put32Bit(cx);
	tmp.Put32Bit(cy);
	tmp.Put32Bit(sx);
	tmp.Put32Bit(sy);

	//tmode.Clear();
	//for ( n = 0 ; ttymode[n].opcode != 0 ; n++ ) {
	//	tmode.Put8Bit(ttymode[n].opcode);
	//	tmode.Put32Bit(ttymode[n].param);
	//}
	//tmode.Put8Bit(0);

	tmode.Clear();
	for ( n = 0 ; n < m_pDocument->m_ParamTab.m_TtyMode.GetSize() ; n++ ) {
		tmode.Put8Bit(m_pDocument->m_ParamTab.m_TtyMode[n].opcode);
		tmode.Put32Bit(m_pDocument->m_ParamTab.m_TtyMode[n].param);
	}
	tmode.Put8Bit(0);
	tmp.PutBuf(tmode.GetPtr(), tmode.GetSize());

	SendPacket2(&tmp);
	m_ChnReqMap.Add(CHAN_REQ_PTY);

	env.GetString(m_pDocument->m_ParamTab.m_ExtEnvStr);
	for ( n = 0 ; n < env.GetSize() ; n++ ) {
		if ( env[n].m_Value == 0 || env[n].m_nIndex.IsEmpty() || env[n].m_String.IsEmpty() )
			continue;
		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
		tmp.Put32Bit(pChan->m_RemoteID);
		tmp.PutStr("env");
		tmp.Put8Bit(0);
		tmp.PutStr(RemoteStr(env[n].m_nIndex));
		tmp.PutStr(RemoteStr(env[n].m_String));
		SendPacket2(&tmp);
	}

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(pChan->m_RemoteID);
	tmp.PutStr("shell");
	tmp.Put8Bit(1);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(CHAN_REQ_SHELL);
}
void Cssh::SendMsgChannelRequesstSubSystem(int id, LPCSTR cmds)
{
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(pChan->m_RemoteID);
	tmp.PutStr("subsystem");
	tmp.Put8Bit(1);
	tmp.PutStr(cmds);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(CHAN_REQ_SUBSYS);
}
void Cssh::SendMsgChannelRequesstExec(int id, LPCSTR cmds)
{
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(pChan->m_RemoteID);
	tmp.PutStr("exec");
	tmp.Put8Bit(1);
	tmp.PutStr(cmds);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(CHAN_REQ_EXEC);
}

//////////////////////////////////////////////////////////////////////

void Cssh::SendMsgGlobalRequest(int num, LPCSTR str, LPCTSTR rhost, int rport)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_GLOBAL_REQUEST);
	tmp.PutStr(str);
	tmp.Put8Bit(1);
	if ( rhost != NULL ) {
		tmp.PutStr(RemoteStr(rhost));
		tmp.Put32Bit(rport);
	}
	SendPacket2(&tmp);
	m_GlbReqMap.Add((WORD)num);
}
void Cssh::SendMsgKeepAlive()
{
	CBuffer tmp;
	BOOL bReply = ((m_KeepAliveSendCount - m_KeepAliveReplyCount) > 10 ? FALSE : TRUE);

	if ( (m_SSH2Status & SSH2_STAT_SENTKEXINIT) != 0 )
		return;

	tmp.Put8Bit(SSH2_MSG_GLOBAL_REQUEST);
	tmp.PutStr("keepalive@openssh.com");
	tmp.Put8Bit(bReply ? 1 : 0);
	SendPacket2(&tmp);

	if ( bReply )
		m_GlbReqMap.Add(0xFFFF);

	if ( ++m_KeepAliveSendCount < 0 )
		m_KeepAliveSendCount = 0;
}
void Cssh::SendMsgUnimplemented()
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_UNIMPLEMENTED);
	tmp.Put32Bit(m_RecvPackSeq);
	SendPacket2(&tmp);
}
void Cssh::SendDisconnect2(int st, LPCSTR str)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_DISCONNECT);
	tmp.Put32Bit(st);
	tmp.PutStr(str);
	SendPacket2(&tmp);
}

//////////////////////////////////////////////////////////////////////

void Cssh::SendPacket2(CBuffer *bp)
{
	int n = bp->GetSize() + 128;
	int pad, sz = bp->GetSize();
	CBuffer tmp(n);
	CBuffer enc(n);
	CBuffer mac;
	static int padflag = FALSE;
	static BYTE padimage[64];

	DEBUGLOG("SendPacket2 %d(%d) Seq=%d", bp->PTR8BIT(bp->GetPtr()), bp->GetSize(), m_SendPackSeq);
	DEBUGDUMP(bp->GetPtr(), bp->GetSize());

	tmp.PutSpc(4 + 1);		// Size + Pad Era
	m_EncCmp.Compress(bp->GetPtr(), bp->GetSize(), &tmp);

	n = tmp.GetSize();
	if ( m_EncMac.IsEtm() || m_EncCip.IsAEAD() || m_EncCip.IsPOLY() )
		n -= 4;
	if ( (pad = m_EncCip.GetBlockSize() - (n % m_EncCip.GetBlockSize())) < 4 )
		pad += m_EncCip.GetBlockSize();

	tmp.SET32BIT(tmp.GetPos(0), tmp.GetSize() - 4 + pad);
	tmp.SET8BIT(tmp.GetPos(4), pad);

	if ( !padflag ) {
		rand_buf(padimage, sizeof(padimage));
		padflag = TRUE;
	}

	for ( n = 0 ; n < pad ; n += 64 )
		tmp.Apend(padimage, ((pad - n) >= 64 ? 64 : (pad - n)));

	if ( m_EncCip.IsAEAD() || m_EncCip.IsPOLY() )
		m_EncCip.SetIvCounter(m_SendPackSeq);

	if ( m_EncMac.IsEtm() ) {
		enc.Apend(tmp.GetPtr(), 4);
		m_EncCip.Cipher(tmp.GetPtr() + 4, tmp.GetSize() - 4, &enc);

		if ( m_EncMac.GetBlockSize() > 0 ) {
			m_EncMac.Compute(m_SendPackSeq, enc.GetPtr(), enc.GetSize(), &mac);
			enc.Apend(mac.GetPtr(), mac.GetSize());
		}

	} else {
		m_EncCip.Cipher(tmp.GetPtr(), tmp.GetSize(), &enc);

		if ( m_EncMac.GetBlockSize() > 0 )
			m_EncMac.Compute(m_SendPackSeq, tmp.GetPtr(), tmp.GetSize(), &enc);
	}

	SendSocket(enc.GetPtr(), enc.GetSize());

	m_SendPackSeq += 1;
	m_SendPackLen += sz;

	if ( (m_SSH2Status & SSH2_STAT_SENTKEXINIT) == 0 && CExtSocket::GetRecvSize() == 0 && (m_SendPackLen >= MAX_PACKETLEN || m_RecvPackLen >= MAX_PACKETLEN) )
		SendMsgKexInit();
}

/*******************************************************************************************************
		RFC 2119

		MUST(しなければならない)	REQUIRED(必須である)			SHALL(するものとする)
		SHOULD(すべきである)		RECOMMENDED(推奨される)
		MAY(してもよい)				OPTIONAL(任意である)

		SHOULD NOT(すべきではない)	NOT RECOMMENDED(推奨されない)
		MUST NOT(してはならない)									SHALL NOT(しないものとする)
********************************************************************************************************/

int Cssh::SSH2MsgKexInit(CBuffer *bp)
{
	int n;
	CStringA mbs;
	static const struct {
		int		mode;
		LPCTSTR	name;
	} kextab[] = {																							//  RFC9142
		{ DHMODE_ECDH_S2_N256,	_T("ecdh-sha2-nistp256")						},	// RFC5656	MAY				SHOULD
		{ DHMODE_ECDH_S2_N384,	_T("ecdh-sha2-nistp384")						},	// RFC5656	SHOULD			SHOULD
		{ DHMODE_ECDH_S2_N521,	_T("ecdh-sha2-nistp521")						},	// RFC5656	SHOULD			SHOULD
		{ DHMODE_GROUP_GEX256,	_T("diffie-hellman-group-exchange-sha256")		},	// RFC4419	MAY				MAY
		{ DHMODE_GROUP_GEX,		_T("diffie-hellman-group-exchange-sha1")		},	// RFC4419	SHOULD NOT		SHOULD NOT
		{ DHMODE_GROUP_14,		_T("diffie-hellman-group14-sha1")				},	// RFC4253	SHOULD			MAY
		{ DHMODE_GROUP_1,		_T("diffie-hellman-group1-sha1")				},	// RFC4253	SHOULD NOT		SHOULD NOT
		{ DHMODE_GROUP_14_256,	_T("diffie-hellman-group14-sha256")				},	// RFC8268	MAY				MUST
		{ DHMODE_GROUP_15_512,	_T("diffie-hellman-group15-sha512")				},	// RFC8268	MAY				MAY
		{ DHMODE_GROUP_16_512,	_T("diffie-hellman-group16-sha512")				},	// RFC8268	SHOULD			SHOULD
		{ DHMODE_GROUP_17_512,	_T("diffie-hellman-group17-sha512")				},	// RFC8268	MAY				MAY
		{ DHMODE_GROUP_18_512,	_T("diffie-hellman-group18-sha512")				},	// RFC8268	MAY				MAY
		{ DHMODE_CURVE25519,	_T("curve25519-sha256")							},	// RFC8731	MUST			SHOULD
		{ DHMODE_CURVE448,		_T("curve448-sha512")							},	// RFC8731	MAY				MAY
		{ DHMODE_SNT761X25519,	_T("sntrup761x25519-sha512@openssh.com")		},
		{ DHMODE_RSA1024SHA1,	_T("rsa1024-sha1")								},	// RFC4432	MUST NOT		MUST NOT
		{ DHMODE_RSA2048SHA2,	_T("rsa2048-sha256")							},	// RFC4432	MAY				MAY
		{ 0,					NULL											},
	};

	m_HisPeer = *bp;
	for ( n = 0 ; n < 16 ; n++ )
		m_Cookie[n] = bp->Get8Bit();
	for ( n = 0 ; n < 10 ; n++ ) {
		bp->GetStr(mbs);
		m_SProp[n] = mbs;
	}
	bp->Get8Bit();
	bp->Get32Bit();

	m_pDocument->m_ParamTab.GetProp(9,  m_CProp[PROP_KEX_ALGS]);			// PROPOSAL_KEX_ALGS
	m_pDocument->m_ParamTab.GetProp(10, m_CProp[PROP_HOST_KEY_ALGS]);		// PROPOSAL_SERVER_HOST_KEY_ALGS

	m_pDocument->m_ParamTab.GetProp(6,  m_CProp[PROP_ENC_ALGS_CTOS], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFENC));	// PROPOSAL_ENC_ALGS_CTOS
	m_pDocument->m_ParamTab.GetProp(3,  m_CProp[PROP_ENC_ALGS_STOC], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFENC));	// PROPOSAL_ENC_ALGS_STOC
	m_pDocument->m_ParamTab.GetProp(7,  m_CProp[PROP_MAC_ALGS_CTOS], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFMAC));	// PROPOSAL_MAC_ALGS_CTOS
	m_pDocument->m_ParamTab.GetProp(4,  m_CProp[PROP_MAC_ALGS_STOC], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFMAC));	// PROPOSAL_MAC_ALGS_STOC

	m_pDocument->m_ParamTab.GetProp(8,  m_CProp[PROP_COMP_ALGS_CTOS]);		// PROPOSAL_COMP_ALGS_CTOS
	m_pDocument->m_ParamTab.GetProp(5,  m_CProp[PROP_COMP_ALGS_STOC]);		// PROPOSAL_COMP_ALGS_STOC

	m_CProp[PROP_LANG_CTOS] = m_VProp[PROP_LANG_CTOS] = _T("");				// PROPOSAL_LANG_CTOS,
	m_CProp[PROP_LANG_STOC] = m_VProp[PROP_LANG_STOC] = _T("");				// PROPOSAL_LANG_STOC,

	for ( n = 0 ; n < 8 ; n++ ) {
		if ( !MatchList(m_CProp[n], m_SProp[n], m_VProp[n]) )
			return (-1);
	}

	if ( m_EncCip.IsAEAD(m_VProp[PROP_ENC_ALGS_CTOS]) || m_EncCip.IsPOLY(m_VProp[PROP_ENC_ALGS_CTOS]) )
		m_VProp[PROP_MAC_ALGS_CTOS] = m_VProp[PROP_ENC_ALGS_CTOS];

	if ( m_DecCip.IsAEAD(m_VProp[PROP_ENC_ALGS_STOC]) || m_DecCip.IsPOLY(m_VProp[PROP_ENC_ALGS_STOC]) )
		m_VProp[PROP_MAC_ALGS_STOC] = m_VProp[PROP_ENC_ALGS_STOC];

	// PROPOSAL_KEX_ALGS
	for ( n = 0 ; kextab[n].name != NULL ; n++ ) {
		if ( m_VProp[PROP_KEX_ALGS].Compare(kextab[n].name) == 0 ) {
			m_DhMode = kextab[n].mode;
			break;
		}
	}
	if ( kextab[n].name == NULL )
		return (-1);

	if ( _tcsstr(m_SProp[PROP_KEX_ALGS], _T("ext-info-s")) != NULL ) {
		// ext-info-sによりSSH2_MSG_NEWKEYSの後にSSH2_MSG_EXT_INFOを送る必要があるのか？
		// PROP_HOST_KEY_ALGSで"rsa-sha2-256/512"を指定するのだから不用なような気がする
		m_bExtInfo = TRUE;

		// ext-info-cは、常に送るのが正解？
		// これもrsa-sha2-256/512でサインを試せば良いだけのように思う
		m_VProp[PROP_KEX_ALGS] += _T(",ext-info-c");
	}

	if ( (m_SSH2Status & SSH2_STAT_SENTKEXINIT) == 0 ) {
		m_MyPeer.Clear();
		rand_buf(m_MyPeer.PutSpc(16), 16);
		for ( n = 0 ; n < 10 ; n++ )
			m_MyPeer.PutStr(TstrToMbs(m_VProp[n]));
		m_MyPeer.Put8Bit(0);
		m_MyPeer.Put32Bit(0);

		SendMsgKexInit();
	}

	m_NeedKeyLen = 0;
	if ( m_NeedKeyLen < m_EncCip.GetKeyLen(m_VProp[PROP_ENC_ALGS_CTOS]) )    m_NeedKeyLen = m_EncCip.GetKeyLen(m_VProp[PROP_ENC_ALGS_CTOS]);
	if ( m_NeedKeyLen < m_DecCip.GetKeyLen(m_VProp[PROP_ENC_ALGS_STOC]) )    m_NeedKeyLen = m_DecCip.GetKeyLen(m_VProp[PROP_ENC_ALGS_STOC]);
	if ( m_NeedKeyLen < m_EncCip.GetBlockSize(m_VProp[PROP_ENC_ALGS_CTOS]) ) m_NeedKeyLen = m_EncCip.GetBlockSize(m_VProp[PROP_ENC_ALGS_CTOS]);
	if ( m_NeedKeyLen < m_DecCip.GetBlockSize(m_VProp[PROP_ENC_ALGS_STOC]) ) m_NeedKeyLen = m_DecCip.GetBlockSize(m_VProp[PROP_ENC_ALGS_STOC]);
	if ( m_NeedKeyLen < m_EncMac.GetKeyLen(m_VProp[PROP_MAC_ALGS_CTOS]) )    m_NeedKeyLen = m_EncMac.GetKeyLen(m_VProp[PROP_MAC_ALGS_CTOS]);
	if ( m_NeedKeyLen < m_DecMac.GetKeyLen(m_VProp[PROP_MAC_ALGS_STOC]) )    m_NeedKeyLen = m_DecMac.GetKeyLen(m_VProp[PROP_MAC_ALGS_STOC]);

	m_RecvPackLen = 0;
	return 0;
}
int Cssh::HostVerifyKey(CBuffer *sign, CBuffer *addb, CBuffer *skey, const EVP_MD *evp_md)
{
	int n;
	int have, mdsz;
	char c;
	CBuffer b;
	EVP_MD_CTX *md_ctx;
	unsigned int hashlen;
	BYTE hash[EVP_MAX_MD_SIZE];

	b.PutStr(TstrToMbs(m_ClientVerStr));
	b.PutStr(TstrToMbs(m_ServerVerStr));

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	b.Put32Bit(m_MyPeer.GetSize()+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)m_MyPeer.GetPtr(), m_MyPeer.GetSize());

	b.Put32Bit(m_HisPeer.GetSize()+1);
	b.Put8Bit(SSH2_MSG_KEXINIT);
	b.Apend((LPBYTE)m_HisPeer.GetPtr(), m_HisPeer.GetSize());

	b.Apend(addb->GetPtr(), addb->GetSize());
	b.Apend(skey->GetPtr(), skey->GetSize());

	if ( (md_ctx = EVP_MD_CTX_new()) == NULL ||
		 EVP_DigestInit(md_ctx, evp_md) == 0 ||
		 EVP_DigestUpdate(md_ctx, b.GetPtr(), b.GetSize()) == 0 ||
		 EVP_DigestFinal(md_ctx, hash, &hashlen) == 0 )
		throw _T("HostVerifyKey hash Error");
	EVP_MD_CTX_free(md_ctx);

	if ( !m_HostKey.Verify(sign, hash, hashlen) )
		return TRUE;

	if ( m_SessHash.GetSize() == 0 )
		m_SessHash.Apend(hash, hashlen);

	if ( (md_ctx = EVP_MD_CTX_new()) == NULL )
		throw _T("HostVerifyKey md_ctx Error");
	mdsz = EVP_MD_size(evp_md);

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);

		if ( (m_VKey[n] = (u_char *)malloc(m_NeedKeyLen + (mdsz - (m_NeedKeyLen % mdsz)))) == NULL )
			throw _T("HostVerifyKey VKey malloc Error");
		c = (char)('A' + n);

		/* K1 = HASH(K || H || "A" || session_id) */
		if ( EVP_DigestInit(md_ctx, evp_md) == 0 ||
			 EVP_DigestUpdate(md_ctx, skey->GetPtr(), skey->GetSize()) == 0 ||
			 EVP_DigestUpdate(md_ctx, hash, hashlen) == 0 ||
			 EVP_DigestUpdate(md_ctx, &c, 1) == 0 ||
			 EVP_DigestUpdate(md_ctx, m_SessHash.GetPtr(), m_SessHash.GetSize()) == 0 ||
			 EVP_DigestFinal(md_ctx, m_VKey[n], NULL) == 0 )
			throw _T("HostVerifyKey kdf Error");

		/*
		 * expand key:
		 * Kn = HASH(K || H || K1 || K2 || ... || Kn-1)
		 * Key = K1 || K2 || ... || Kn
		 */
		for ( have = mdsz ; m_NeedKeyLen > have ; have += mdsz ) {
			if ( EVP_DigestInit(md_ctx, evp_md) == 0 ||
				 EVP_DigestUpdate(md_ctx, skey->GetPtr(), skey->GetSize()) == 0 ||
				 EVP_DigestUpdate(md_ctx, hash, hashlen) == 0 ||
				 EVP_DigestUpdate(md_ctx, m_VKey[n], have) == 0 ||
				 EVP_DigestFinal(md_ctx, m_VKey[n] + have, NULL) == 0 )
				throw _T("HostVerifyKey kdf expand Error");
		}
	}

	EVP_MD_CTX_free(md_ctx);

	return 0;
}
int Cssh::SSH2MsgKexDhReply(CBuffer *bp)
{
	int n;
	int ret = (-1);
	CBuffer tmp(-1), sign(-1), addb(-1), skey(-1);
	BIGNUM *spub = NULL, *ssec = NULL;
	LPBYTE p;
	const EVP_MD *evp_md;

	bp->GetBuf(&tmp);
	addb.PutBuf(tmp.GetPtr(), tmp.GetSize());

	if ( !m_HostKey.GetBlob(&tmp) )
		return (-1);

	if ( !m_HostKey.HostVerify(m_HostName, m_HostPort, this) )
		return (-1);

	if ( (spub = BN_new()) == NULL || (ssec = BN_new()) == NULL )
		goto ENDRET;

	bp->GetBIGNUM2(spub);
	bp->GetBuf(&sign);

	if ( !dh_pub_is_valid(m_SaveDh, spub) )
		goto ENDRET;

	switch(m_DhMode) {
	case DHMODE_GROUP_1:
	case DHMODE_GROUP_14:
		evp_md = EVP_sha1();
		break;
	case DHMODE_GROUP_14_256:
		evp_md = EVP_sha256();
		break;
	case DHMODE_GROUP_15_512:
	case DHMODE_GROUP_16_512:
	case DHMODE_GROUP_17_512:
	case DHMODE_GROUP_18_512:
		evp_md = EVP_sha512();
		break;
	}

	tmp.Clear();
	p = tmp.PutSpc(DH_size(m_SaveDh));
	n = DH_compute_key(p, spub, m_SaveDh);
	BN_bin2bn(p, n, ssec);

	BIGNUM const *pub_key = NULL;

	DH_get0_key(m_SaveDh, &pub_key, NULL);

	addb.PutBIGNUM2(pub_key);
	addb.PutBIGNUM2(spub);

	skey.PutBIGNUM2(ssec);

	if ( HostVerifyKey(&sign, &addb, &skey, evp_md) )
		goto ENDRET;

	ret = 0;

ENDRET:
	if ( spub != NULL )
		BN_clear_free(spub);
	if ( ssec != NULL )
		BN_clear_free(ssec);

	return ret;
}
int Cssh::SSH2MsgKexDhGexGroup(CBuffer *bp)
{
	CBuffer tmp(-1);

	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);

	if ( (m_SaveDh = DH_new()) == NULL )
		return (-1);

	BIGNUM *p = bp->GetBIGNUM2(NULL);
	BIGNUM *g = bp->GetBIGNUM2(NULL);

	DH_set0_pqg(m_SaveDh, p, NULL, g);

	int grp_bits = DH_bits(m_SaveDh);

	if ( DHGEX_MIN_BITS > grp_bits || grp_bits > DHGEX_MAX_BITS )
		return (-1);

	if ( dh_gen_key(m_SaveDh, m_NeedKeyLen * 8) )
		return (-1);

	BIGNUM const *pub_key = NULL;

	DH_get0_key(m_SaveDh, &pub_key, NULL);

	tmp.Put8Bit(SSH2_MSG_KEX_DH_GEX_INIT);
	tmp.PutBIGNUM2(pub_key);
	SendPacket2(&tmp);

	return 0;
}
int Cssh::SSH2MsgKexDhGexReply(CBuffer *bp)
{
	int n;
	LPBYTE p;
	int ret = (-1);
	CBuffer tmp(-1), sign(-1), addb(-1), skey(-1);
	BIGNUM *spub = NULL, *ssec = NULL;
	const EVP_MD *evp_md;

	bp->GetBuf(&tmp);
	addb.PutBuf(tmp.GetPtr(), tmp.GetSize());

	if ( !m_HostKey.GetBlob(&tmp) )
		return (-1);

	if ( !m_HostKey.HostVerify(m_HostName, m_HostPort, this) )
		return (-1);

	if ( (spub = BN_new()) == NULL || (ssec = BN_new()) == NULL )
		goto ENDRET;

	bp->GetBIGNUM2(spub);
	bp->GetBuf(&sign);

	if ( !dh_pub_is_valid(m_SaveDh, spub) )
		goto ENDRET;

	tmp.Clear();
	p = tmp.PutSpc(DH_size(m_SaveDh));
	n = DH_compute_key(p, spub, m_SaveDh);
	BN_bin2bn(p, n, ssec);

	evp_md = (m_DhMode == DHMODE_GROUP_GEX ? EVP_sha1() : EVP_sha256());

	BIGNUM const *dhp = NULL, *dhg = NULL, *dhpub_key = NULL;

	DH_get0_pqg(m_SaveDh, &dhp, NULL, &dhg);
	DH_get0_key(m_SaveDh, &dhpub_key, NULL);

	addb.Put32Bit(DHGEX_MIN_BITS);
	addb.Put32Bit(m_DhGexReqBits);
	addb.Put32Bit(DHGEX_MAX_BITS);

	addb.PutBIGNUM2((BIGNUM *)dhp);
	addb.PutBIGNUM2((BIGNUM *)dhg);

	addb.PutBIGNUM2((BIGNUM *)dhpub_key);
	addb.PutBIGNUM2(spub);

	skey.PutBIGNUM2(ssec);

	if ( HostVerifyKey(&sign, &addb, &skey, evp_md) )
		goto ENDRET;

	ret = 0;

ENDRET:
	if ( spub != NULL )
		BN_clear_free(spub);

	if ( ssec != NULL )
		BN_clear_free(ssec);

	return ret;
}
int Cssh::SSH2MsgKexEcdhReply(CBuffer *bp)
{
	int ret = (-1);
	CBuffer tmp(-1), sign(-1), addb(-1), skey(-1);
	EC_POINT *server_public = NULL;
	BIGNUM *shared_secret = NULL;
	int klen;
	LPBYTE kbuf = NULL;
	const EVP_MD *evp_md;

	bp->GetBuf(&tmp);
	addb.PutBuf(tmp.GetPtr(), tmp.GetSize());

	if ( !m_HostKey.GetBlob(&tmp) )
		return (-1);

	if ( !m_HostKey.HostVerify(m_HostName, m_HostPort, this) )
		return (-1);

	if ( (server_public = EC_POINT_new(m_EcdhGroup)) == NULL )
		goto ENDRET;

	bp->GetEcPoint(m_EcdhGroup, server_public);
	bp->GetBuf(&sign);

	if ( key_ec_validate_public(m_EcdhGroup, server_public) != 0 )
		goto ENDRET;

	klen = (EC_GROUP_get_degree(m_EcdhGroup) + 7) / 8;
	kbuf = new BYTE[klen];

	if ( ECDH_compute_key(kbuf, klen, server_public, m_EcdhClientKey, NULL) != (int)klen )
		goto ENDRET;
	if ( (shared_secret = BN_new()) == NULL )
		goto ENDRET;
	if ( BN_bin2bn(kbuf, klen, shared_secret) == NULL )
		goto ENDRET;

	addb.PutEcPoint(m_EcdhGroup, EC_KEY_get0_public_key(m_EcdhClientKey));
	addb.PutEcPoint(m_EcdhGroup, server_public);

	skey.PutBIGNUM2(shared_secret);

	switch(m_DhMode) {
	default:
	case DHMODE_ECDH_S2_N256:
		evp_md = EVP_sha256();
		break;
	case DHMODE_ECDH_S2_N384:
		evp_md = EVP_sha384();
		break;
	case DHMODE_ECDH_S2_N521:
		evp_md = EVP_sha512();
		break;
	}

	if ( HostVerifyKey(&sign, &addb, &skey, evp_md) )
		goto ENDRET;

	EC_KEY_free(m_EcdhClientKey);
	m_EcdhClientKey = NULL;

	ret = 0;

ENDRET:
	if ( kbuf != NULL )
		delete [] kbuf;

	if ( shared_secret != NULL )
		BN_clear_free(shared_secret);

	if ( server_public != NULL )
		EC_POINT_clear_free(server_public);

	return ret;
}
int Cssh::SSH2MsgKexCurveReply(CBuffer *bp)
{
	int n;
	int ret = (-1);
	CBuffer tmp(-1), sign(-1), addb(-1), skey(-1);
	CBuffer server_public(-1), shared_key(-1);
	BIGNUM *shared_secret = NULL;
	BYTE shared_digest[EVP_MAX_MD_SIZE];
	const EVP_MD *evp_md = EVP_sha256();
	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *pkey = NULL;
	size_t keylen;
	int type = EVP_PKEY_X25519;
	int ctsz = 0;

	bp->GetBuf(&tmp);
	addb.PutBuf(tmp.GetPtr(), tmp.GetSize());

	if ( !m_HostKey.GetBlob(&tmp) )
		return (-1);

	if ( !m_HostKey.HostVerify(m_HostName, m_HostPort, this) )
		return (-1);

	bp->GetBuf(&server_public);
	bp->GetBuf(&sign);

	switch(m_DhMode) {
	case DHMODE_CURVE25519:
		type = EVP_PKEY_X25519;
		evp_md = EVP_sha256();
		break;
	case DHMODE_CURVE448:
		type = EVP_PKEY_X448;
		evp_md = EVP_sha512();
		break;
	case DHMODE_SNT761X25519:
		type = EVP_PKEY_X25519;
		evp_md = EVP_sha512();
		ctsz = sntrup761_CIPHERTEXTBYTES;
		if ( server_public.GetSize() < ctsz )
			return (-1);
		if ( sntrup761_dec(shared_key.PutSpc(sntrup761_BYTES), server_public.GetPtr(), m_SntrupClientKey.GetPtr()) != 0 )
			return (-1);
		break;
	}

	if ( (pkey = EVP_PKEY_new_raw_public_key(type, NULL, server_public.GetPtr() + ctsz, server_public.GetSize() - ctsz)) == NULL )
		goto ENDRET;

	if ( (ctx = EVP_PKEY_CTX_new(m_CurveEvpKey, NULL)) == NULL )
		goto ENDRET;

	if ( EVP_PKEY_derive_init(ctx) <= 0 )
		goto ENDRET;

	if ( EVP_PKEY_derive_set_peer(ctx, pkey) <= 0 )
		goto ENDRET;

	if ( EVP_PKEY_derive(ctx, NULL, &keylen) <= 0 )
		goto ENDRET;

	if ( EVP_PKEY_derive(ctx, shared_key.PutSpc((int)keylen), &keylen) <= 0 )
		goto ENDRET;

	addb.PutBuf(m_CurveClientPubkey.GetPtr(), m_CurveClientPubkey.GetSize());
	addb.PutBuf(server_public.GetPtr(), server_public.GetSize());

	if ( m_DhMode == DHMODE_SNT761X25519 ) {
		n = MD_digest(evp_md, shared_key.GetPtr(), shared_key.GetSize(), shared_digest, sizeof(shared_digest));
		skey.PutBuf(shared_digest, n);

	} else {
		if ( (shared_secret = BN_new()) == NULL )
			goto ENDRET;

		if ( BN_bin2bn(shared_key.GetPtr(), shared_key.GetSize(), shared_secret) == NULL )
			goto ENDRET;

		skey.PutBIGNUM2(shared_secret);
	}

	if ( HostVerifyKey(&sign, &addb, &skey, evp_md) )
		goto ENDRET;

	ret = 0;

ENDRET:
	if ( pkey != NULL )
		EVP_PKEY_free(pkey);

	if ( ctx != NULL )
		EVP_PKEY_CTX_free(ctx);

	if ( m_CurveEvpKey != NULL )
		EVP_PKEY_free(m_CurveEvpKey);
	m_CurveEvpKey = NULL;

	if ( shared_secret != NULL )
		BN_clear_free(shared_secret);

	return ret;
}
int Cssh::SSH2MsgKexRsaPubkey(CBuffer *bp)
{
/*
       byte      SSH_MSG_KEXRSA_PUBKEY
       string    server public host key and certificates (K_S)
       string    K_T, transient RSA public key
*/
	int ret = (-1);
	CBuffer tmp(-1);
	const EVP_MD *evp_md = EVP_sha256();
	CIdKey TranKey;
	int bits;
	BIGNUM *shared_secret = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	size_t outlen;

	bp->GetBuf(&tmp);
	m_RsaHostBlob = tmp;

	if ( !m_HostKey.GetBlob(&tmp) )
		return (-1);

	if ( !m_HostKey.HostVerify(m_HostName, m_HostPort, this) )
		return (-1);

	bp->GetBuf(&tmp);
	m_RsaTranBlob = tmp;
		
	if ( !TranKey.GetBlob(&tmp) )
		return (-1);

	if ( TranKey.m_Type != IDKEY_RSA2 )
		return (-1);

	m_RsaSharedKey.Clear();
	m_RsaOaepSecret.Clear();

	if ( m_DhMode == DHMODE_RSA2048SHA2 )
		evp_md = EVP_sha512();

	bits = TranKey.GetSize() - 2 * EVP_MD_size(evp_md) * 8 - 49;

	if ( (shared_secret = BN_new()) == NULL )
		goto ENDRET;

	if ( BN_rand(shared_secret, bits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY) <= 0 )
		goto ENDRET;

	m_RsaSharedKey.Clear();
	m_RsaSharedKey.PutBIGNUM2(shared_secret);

	if ( (pkey = EVP_PKEY_new()) == NULL )
		goto ENDRET;

	if ( EVP_PKEY_set1_RSA(pkey, TranKey.m_Rsa) <= 0 )
		goto ENDRET;

	if ( (ctx = EVP_PKEY_CTX_new(pkey, NULL)) == NULL )
		goto ENDRET;

	if ( EVP_PKEY_encrypt_init(ctx) <= 0 )
		goto ENDRET;

	if ( EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 )
		goto ENDRET;

	if ( EVP_PKEY_CTX_set_rsa_oaep_md(ctx, evp_md) <= 0 )
		goto ENDRET;

	if ( EVP_PKEY_encrypt(ctx, NULL, &outlen, m_RsaSharedKey.GetPtr(), m_RsaSharedKey.GetSize()) <= 0 )
		goto ENDRET;

	m_RsaOaepSecret.PutSpc((int)outlen);

	if ( EVP_PKEY_encrypt(ctx, m_RsaOaepSecret.GetPtr(), &outlen, m_RsaSharedKey.GetPtr(), m_RsaSharedKey.GetSize()) <= 0 )
		goto ENDRET;

/*
       byte      SSH_MSG_KEXRSA_SECRET
       string    RSAES-OAEP-ENCRYPT(K_T, K)	// K = (KLEN - 2 * HLEN - 49)bits
*/
	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_KEXRSA_SECRET);
	tmp.PutBuf(m_RsaOaepSecret.GetPtr(), m_RsaOaepSecret.GetSize());
	SendPacket2(&tmp);

	ret = 0;

ENDRET:
	if ( shared_secret != NULL )
		BN_free(shared_secret);

	if ( ctx != NULL )
		EVP_PKEY_CTX_free(ctx);

	if ( pkey != NULL )
		EVP_PKEY_free(pkey);

	return ret;
}
int Cssh::SSH2MsgKexRsaDone(CBuffer *bp)
{
/*
       byte      SSH_MSG_KEXRSA_DONE
       string    signature of H with host key

       string    V_C, the client's identification string (CR and LF excluded)
       string    V_S, the server's identification string (CR and LF excluded)
       string    I_C, the payload of the client's SSH_MSG_KEXINIT
       string    I_S, the payload of the server's SSH_MSG_KEXINIT
       string    K_S, the host key
       string    K_T, the transient RSA key
       string    RSAES_OAEP_ENCRYPT(K_T, K), the encrypted secret
       mpint     K, the shared secret
*/
	CBuffer sign(-1), addb(-1);
	const EVP_MD *evp_md = EVP_sha256();

	bp->GetBuf(&sign);

	addb.PutBuf(m_RsaHostBlob.GetPtr(), m_RsaHostBlob.GetSize());
	addb.PutBuf(m_RsaTranBlob.GetPtr(), m_RsaTranBlob.GetSize());
	addb.PutBuf(m_RsaOaepSecret.GetPtr(), m_RsaOaepSecret.GetSize());

	if ( m_DhMode == DHMODE_RSA2048SHA2 )
		evp_md = EVP_sha512();

	if ( HostVerifyKey(&sign, &addb, &m_RsaSharedKey, evp_md) )
		return (-1);

	return 0;
}

//////////////////////////////////////////////////////////////////////

int Cssh::SSH2MsgNewKeys(CBuffer *bp)
{
	if ( !m_DecCip.Init(m_VProp[PROP_ENC_ALGS_STOC], MODE_DEC, m_VKey[3], (-1), m_VKey[1]) ||
		 !m_DecMac.Init(m_VProp[PROP_MAC_ALGS_STOC], m_VKey[5], (-1)) ||
		 !m_DecCmp.Init(m_VProp[PROP_COMP_ALGS_STOC], MODE_DEC, COMPLEVEL) )
		return (-1);

	return 0;
}
int Cssh::SSH2MsgExtInfo(CBuffer *bp)
{
	//	RFC 8308
	//	byte       SSH_MSG_EXT_INFO (value 7)
	//
	//	uint32     nr-extensions
	//	repeat the following 2 fields "nr-extensions" times:
	//		string   extension-name
	//		string   extension-value (binary)

	//	string      "server-sig-algs"
	//	name-list   signature-algorithms-accepted

	//	string         "delay-compression"
	//	string:
	//		name-list    compression_algorithms_client_to_server
	//		name-list    compression_algorithms_server_to_client
	//			"foo,bar" "bar,baz"
	//			00000016 00000007 666f6f2c626172 00000007 6261722c62617a

	//	string      "no-flow-control"
	//	string      choice of: "p" for preferred | "s" for supported

	//	string      "elevation"
	//	string      choice of: "y" | "n" | "d"

	int n, count;
	CStringA name, value;

	count = bp->Get32Bit();
	for ( n = 0 ; n < count ; n++ ) {
		bp->GetStr(name);
		if ( strcmp(name, "delay-compression") == 0 ) {
			CBuffer tmp;
			CStringA str;
			bp->GetBuf(&tmp);
			while ( tmp.GetSize() > 4 ) {
				tmp.GetStr(str);
				if ( !value.IsEmpty() )
					value += ';';
				value += str;
			}
		} else
			bp->GetStr(value);

		m_ExtInfo[MbsToTstr(name)] = MbsToTstr(value);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////

int Cssh::SSH2MsgUserAuthPkOk(CBuffer *bp)
{
	int n;
	CStringA name;
	CBuffer blob(-1);
	CIdKey ReqKey;

	bp->GetStr(name);
	bp->GetBuf(&blob);

	if ( !ReqKey.GetBlob(&blob) )
		return (-1);

	for ( n = 0 ; n < m_IdKeyTab.GetSize() ; n++ ) {
		if ( ReqKey.Compere(&(m_IdKeyTab[n])) != 0 )
			continue;

		if ( m_pIdKey->InitPass(m_pDocument->m_ServerEntry.m_PassName) ) {
			m_IdKeyPos = n;
			m_AuthStat = AST_PUBKEY_TRY;
			break;
		}
	}

	SendMsgUserAuthRequest(NULL);
	return 0;
}
int Cssh::SSH2MsgUserAuthPasswdChangeReq(CBuffer *bp)
{
	CBuffer tmp(-1);
	CStringA info, lang;
	CString pass;
	CPassDlg dlg;

	bp->GetStr(info);
	bp->GetStr(lang);

	tmp.Put8Bit(SSH2_MSG_USERAUTH_REQUEST);
	tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_UserName));
	tmp.PutStr("ssh-connection");
	tmp.PutStr("password");
	tmp.Put8Bit(1);

	dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
	dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
	dlg.m_Enable   = PASSDLG_PASS;
	dlg.m_Prompt   = "Enter Old Password";
	dlg.m_PassName = "";

	if ( dlg.DoModal() != IDOK )
		goto NEXTAUTH;

	tmp.PutStr(RemoteStr(dlg.m_PassName));

	info = "Enter New Password";
	while ( pass.IsEmpty() ) {
		dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
		dlg.m_PortName = m_pDocument->m_ServerEntry.m_PortName;
		dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
		dlg.m_Enable   = PASSDLG_PASS;
		dlg.m_Prompt   = info;
		dlg.m_PassName = "";

		if ( dlg.DoModal() != IDOK )
			goto NEXTAUTH;

		pass = dlg.m_PassName;

		dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
		dlg.m_PortName = m_pDocument->m_ServerEntry.m_PortName;
		dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
		dlg.m_Enable   = PASSDLG_PASS;
		dlg.m_Prompt   = "Retype New Password";
		dlg.m_PassName = "";

		if ( dlg.DoModal() != IDOK )
			goto NEXTAUTH;

		if ( pass.Compare(dlg.m_PassName) != 0 ) {
			info = "Mismatch ReEntry New Password";
			pass.Empty();
		}
	}

	tmp.PutStr(RemoteStr(pass));
	SendPacket2(&tmp);
	return 0;

NEXTAUTH:
	UserAuthNextState();
	SendMsgUserAuthRequest(NULL);
	return 0;
}
int Cssh::SSH2MsgUserAuthInfoRequest(CBuffer *bp)
{
	int n, echo, max;
	CBuffer tmp;
	CStringA name, inst, lang, prom;
	CEditDlg dlg;
	BOOL bAutoPass = FALSE;

	bp->GetStr(name);
	bp->GetStr(inst);
	bp->GetStr(lang);
	max = bp->Get32Bit();

	tmp.Put8Bit(SSH2_MSG_USERAUTH_INFO_RESPONSE);
	tmp.Put32Bit(max);

	for ( n = 0 ; n < max ; n++ ) {
		bp->GetStr(prom);
		echo = bp->Get8Bit();

		// 最初のトライでは、保存されたパスワードを送ってみる
		if ( m_IdKeyPos == 0 && n == 0 && max == 1 && !m_pDocument->m_ServerEntry.m_PassName.IsEmpty() && IsStriStr(prom, "password") ) {
			tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_PassName));
			bAutoPass = TRUE;

		} else {
			dlg.m_WinText.Format(_T("keyboard-interactive(%s@%s)"), (LPCTSTR)m_pDocument->m_ServerEntry.m_UserName, (LPCTSTR)m_pDocument->m_ServerEntry.m_HostName);
			dlg.m_Title.Empty();
			if ( !name.IsEmpty() ) {
				dlg.m_Title += LocalStr(name);
				dlg.m_Title += _T("\r\n");
			}
			if ( !inst.IsEmpty() ) {
				dlg.m_Title += LocalStr(inst);
				dlg.m_Title += _T("\r\n");
			}
			dlg.m_Title += LocalStr(prom);
			dlg.m_bPassword = (echo == 0 ? TRUE : FALSE);

			if ( dlg.DoModal() != IDOK ) {
				UserAuthNextState();
				SendMsgUserAuthRequest(NULL);
				return 0;
			}

			tmp.PutStr(RemoteStr(dlg.m_Edit));
		}
	}

	// 呼び出し回数をカウント
	m_IdKeyPos++;
	m_bKeybIntrReq = FALSE;

	if ( max > 0 )
		AddAuthLog(_T("keyboard-interactive(%s%s)"), m_pDocument->m_ServerEntry.m_UserName, (bAutoPass ? _T(":auto"): _T("")));

	SendPacket2(&tmp);
	return 0;
}
int Cssh::SSH2MsgUserAuthGssapiProcess(CBuffer *bp, int type)
{
	CBuffer tmp, sig;
	CString str;
	CStringA msg, lang;
	SecBuffer recv_token = { 0, SECBUFFER_TOKEN, NULL };
	SecBuffer send_token = { 0, SECBUFFER_TOKEN, NULL };
    SecBufferDesc input_desc  ={ SECBUFFER_VERSION, 1, &recv_token };
    SecBufferDesc output_desc ={ SECBUFFER_VERSION, 1, &send_token };
    unsigned long flags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_DELEGATE;
    unsigned long ret_flags = 0;
	SecBuffer sec_token[2];
	SecPkgContext_Sizes cont_size;
	long status;

	if ( (m_SSH2Status & SSH2_STAT_AUTHGSSAPI) == 0 )
		return (-1);

	switch(type) {
	case SSH2_MSG_USERAUTH_GSSAPI_RESPONSE:
		bp->GetBuf(&tmp);
		if ( tmp.GetSize() < (gssapi_kerberos_mech.len + 2) || *(tmp.GetPos(0)) != SSH2_GSS_OIDTYPE || *(tmp.GetPos(1)) != gssapi_kerberos_mech.len )
			goto RETRYAUTH;
		if ( memcmp(tmp.GetPos(2), gssapi_kerberos_mech.data, gssapi_kerberos_mech.len) != 0 )
			goto RETRYAUTH;
		tmp.Clear();
		break;

	case SSH2_MSG_USERAUTH_GSSAPI_TOKEN:
		bp->GetBuf(&tmp);
		break;

	case SSH2_MSG_USERAUTH_GSSAPI_ERRTOK:
		bp->GetBuf(&tmp);
		str.Format(_T("SSH2 Gssapi Receive ErrToken(%dbyte)"), tmp.GetSize());
		AfxMessageBox(str);
		goto RETRYAUTH;

	case SSH2_MSG_USERAUTH_GSSAPI_ERROR:
		bp->GetStr(msg);
		bp->GetStr(lang);
		str.Format(_T("SSH2 Gssapi Receive Error\n%s"), MbsToTstr(msg));
		AfxMessageBox(str);
		goto RETRYAUTH;
	}

	if ( tmp.GetSize() > 0 ) {
		recv_token.cbBuffer = tmp.GetSize();
		recv_token.pvBuffer = tmp.GetPtr();
	}

	status = InitializeSecurityContext(&m_SSPI_hCredential, m_SSPI_phContext, (LPTSTR)(LPCTSTR)m_SSPI_TargetName,
			flags, 0, SECURITY_NATIVE_DREP, &input_desc, 0, &m_SSPI_hNewContext, &output_desc, &ret_flags, &m_SSPI_tsExpiry);

	m_SSPI_phContext = &m_SSPI_hNewContext;

	if ( status == SEC_I_COMPLETE_AND_CONTINUE || status == SEC_I_COMPLETE_NEEDED ) {
		if ( status == SEC_I_COMPLETE_AND_CONTINUE )
			status = SEC_I_CONTINUE_NEEDED;
		else if ( status == SEC_I_COMPLETE_NEEDED )
			status = SEC_E_OK;

		if ( send_token.cbBuffer > 0 && send_token.pvBuffer != NULL && send_token.BufferType == SECBUFFER_TOKEN ) {
			tmp.Clear();
			tmp.Put8Bit((status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED) ? SSH2_MSG_USERAUTH_GSSAPI_TOKEN : SSH2_MSG_USERAUTH_GSSAPI_ERRTOK);
			tmp.PutBuf((LPBYTE)send_token.pvBuffer, send_token.cbBuffer);
			SendPacket2(&tmp);

			FreeContextBuffer(send_token.pvBuffer);
			send_token.cbBuffer = 0;
			send_token.pvBuffer = NULL;
		}

		if ( CompleteAuthToken(&m_SSPI_hCredential, &output_desc) != SEC_E_OK )
			goto RETRYAUTH;
	}

	if ( send_token.cbBuffer > 0 && send_token.pvBuffer != NULL && send_token.BufferType == SECBUFFER_TOKEN ) {
		tmp.Clear();
		tmp.Put8Bit((status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED) ? SSH2_MSG_USERAUTH_GSSAPI_TOKEN : SSH2_MSG_USERAUTH_GSSAPI_ERRTOK);
		tmp.PutBuf((LPBYTE)send_token.pvBuffer, send_token.cbBuffer);
		SendPacket2(&tmp);

		FreeContextBuffer(send_token.pvBuffer);
	}

	if ( status == SEC_I_CONTINUE_NEEDED )
		return 0;
	else if ( status != SEC_E_OK )
		goto RETRYAUTH;

	tmp.Clear();
	tmp.PutBuf(m_SessHash.GetPtr(), m_SessHash.GetSize());
	tmp.Put8Bit(SSH2_MSG_USERAUTH_REQUEST);
	tmp.PutStr(RemoteStr(m_pDocument->m_ServerEntry.m_UserName));
	tmp.PutStr("ssh-connection");
	tmp.PutStr("gssapi-with-mic");

	ZeroMemory(&cont_size, sizeof(cont_size));
	if ( QueryContextAttributes(m_SSPI_phContext, SECPKG_ATTR_SIZES, &cont_size) != SEC_E_OK )
		goto RETRYAUTH;

	input_desc.cBuffers     = 2;
    input_desc.pBuffers     = sec_token;
    input_desc.ulVersion    = SECBUFFER_VERSION;

    sec_token[0].BufferType = SECBUFFER_DATA;
	sec_token[0].cbBuffer   = tmp.GetSize();
    sec_token[0].pvBuffer   = tmp.GetPtr();

    sec_token[1].BufferType = SECBUFFER_TOKEN;
	sec_token[1].cbBuffer   = cont_size.cbMaxSignature;
    sec_token[1].pvBuffer   = sig.PutSpc(cont_size.cbMaxSignature);

	if ( MakeSignature(m_SSPI_phContext, 0, &input_desc, 0) != SEC_E_OK )
		goto RETRYAUTH;

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_USERAUTH_GSSAPI_MIC);
	tmp.PutBuf((LPBYTE)sec_token[1].pvBuffer, sec_token[1].cbBuffer);
	SendPacket2(&tmp);

	if ( m_SSPI_phContext != NULL ) {
		DeleteSecurityContext(m_SSPI_phContext);
		m_SSPI_phContext = NULL;
	}

	FreeCredentialsHandle(&m_SSPI_hCredential);
	m_SSH2Status &= ~SSH2_STAT_AUTHGSSAPI;

	return 0;

RETRYAUTH:
	if ( m_SSPI_phContext != NULL ) {
		DeleteSecurityContext(m_SSPI_phContext);
		m_SSPI_phContext = NULL;
	}

	FreeCredentialsHandle(&m_SSPI_hCredential);
	m_SSH2Status &= ~SSH2_STAT_AUTHGSSAPI;

	SendMsgUserAuthRequest(NULL);
	return 0;
}

//////////////////////////////////////////////////////////////////////

int Cssh::SSH2MsgChannelOpen(CBuffer *bp)
{
	int i, id;
	CBuffer tmp;
	CStringA type, mbs;
	CString host[2];
	int port[2];
	LPCTSTR p;
	CFifoChannel *pChan;

	bp->GetStr(type);
	id = bp->Get32Bit();

	if ( (pChan = GetFifoChannel(-1)) == NULL )
		goto FAILURE;

	pChan->m_TypeName    = type;
	pChan->m_RemoteID    = id;
	pChan->m_RemoteComs  = bp->Get32Bit();
	pChan->m_RemotePacks = bp->Get32Bit();
	pChan->m_LocalComs   = 0;

	if ( type.CompareNoCase("auth-agent@openssh.com") == 0 ) {
		pChan->m_Type = SSHFT_AGENT;
		pChan->m_Status |= (CHAN_OPEN_LOCAL | CHAN_OPEN_REMOTE);
		pChan->m_pFifoBase = new CFifoAgent(m_pDocument, this, pChan);

		CFifoBase::Link(m_pFifoMid, IdToFdIn(pChan->m_LocalID), pChan->m_pFifoBase, FIFO_STDIN);

		SendMsgChannelOpenConfirmation(pChan->m_LocalID);
		return 0;


	} else if ( type.CompareNoCase("x11") == 0 ) {
		bp->GetStr(mbs);
		host[0] = LocalStr(mbs);
		port[0] = bp->Get32Bit();

		if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHX11PF) )
			goto FAILURE;

		if ( (p = _tgetenv(_T("DISPLAY"))) == NULL ) {
			if ( m_pDocument->m_ParamTab.m_XDisplay.IsEmpty() )
				p = _T(":0");
			else
				p = m_pDocument->m_ParamTab.m_XDisplay;
		}

		host[1].Empty();
		while ( *p != _T('\0') && *p != _T(':') )
			host[1] += *(p++);
		if ( *(p++) != _T(':') )
			goto FAILURE;
		port[1] = _tstoi(p);

		if ( host[1].IsEmpty() || host[1].CompareNoCase(_T("unix")) == 0 )
			host[1] = _T("127.0.0.1");

		pChan->m_Type = SSHFT_X11;
		pChan->m_Status |= CHAN_OPEN_REMOTE;
		pChan->m_lHost = host[1];
		pChan->m_lPort = 6000 + port[1];
		pChan->m_rHost = host[0];
		pChan->m_rPort = port[0];

		pChan->m_pFifoBase = new CFifoSocket(m_pDocument, this);
		CFifoX11 *pFifoX11 = new CFifoX11(m_pDocument, this, pChan);

		CFifoBase::MidLink(m_pFifoMid, IdToFdIn(pChan->m_LocalID), pFifoX11, FIFO_STDIN, pChan->m_pFifoBase, FIFO_STDIN);

		if ( !((CFifoSocket *)pChan->m_pFifoBase)->Open(host[1], pChan->m_lPort) ) {
			LogIt(_T("x11 Open Failure #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
			CFifoBase::UnLink(pFifoX11, FIFO_STDIN, TRUE);
			goto FAILURE;
		}

		return 0;

	} else if ( type.CompareNoCase("forwarded-tcpip") == 0 ) {
		bp->GetStr(mbs);
		host[0] = LocalStr(mbs);
		port[0] = bp->Get32Bit();
		bp->GetStr(mbs);
		host[1] = LocalStr(mbs);
		port[1] = bp->Get32Bit();

		for ( i = 0 ; i < m_Permit.GetSize() ; i++ ) {
			if ( port[0] == m_Permit[i].m_rPort )
				break;
		}

		if ( i >=  m_Permit.GetSize() ) {
			LogIt(_T("forwarded-tcpip Permit Failure #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
			goto FAILURE;
		}
		
		pChan->m_Status |= CHAN_OPEN_REMOTE;
		pChan->m_lHost = host[1];
		pChan->m_lPort = port[1];
		pChan->m_rHost = host[0];
		pChan->m_rPort = port[0];

		if ( m_Permit[i].m_Type == PFD_RSOCKS ) {
			pChan->m_Type = SSHFT_REMOTE_SOCKS;
			pChan->m_pFifoBase = new CFifoSocks(m_pDocument, this, pChan);

			((CFifoSocks *)pChan->m_pFifoBase)->m_rFd = FIFO_STDIN;
			((CFifoSocks *)pChan->m_pFifoBase)->m_wFd = FIFO_STDOUT;

			CFifoBase::Link(m_pFifoMid, IdToFdIn(pChan->m_LocalID), pChan->m_pFifoBase, FIFO_STDIN);
			SendMsgChannelOpenConfirmation(pChan->m_LocalID);

		} else {
			pChan->m_Type = SSHFT_SOCKET_REMOTE;

			if ( !ChannelOpen(pChan->m_LocalID, m_Permit[i].m_lHost, m_Permit[i].m_lPort) ) {
				LogIt(_T("forwarded-tcpip Open Failure #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
				goto FAILURE;
			}
		}

		return 0;
	}

FAILURE:
	if ( pChan != NULL )
		ChannelClose(pChan->m_LocalID);

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_FAILURE);
	tmp.Put32Bit(id);
	tmp.Put32Bit(SSH2_OPEN_CONNECT_FAILED);
	tmp.PutStr("open failed");
	tmp.PutStr("");
	SendPacket2(&tmp);
	return 0;
}
int Cssh::SSH2MsgChannelOpenReply(CBuffer *bp, int type)
{
	int id = bp->Get32Bit();
	CFifoChannel *pChan;

	if ( id < 0 || (pChan = GetFifoChannel(id)) == NULL || (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
		return (-1);

	if ( type == SSH2_MSG_CHANNEL_OPEN_FAILURE ) {
		if ( pChan->m_Type != SSHFT_NONE )
			LogIt(_T("Open Failure #%d Filter"), id);
		else
			LogIt(_T("Open Failure #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
		ChannelClose(id);
		return (id == FdToId(FIFO_EXTIN) ? (-1) : 0);
	}

	pChan->m_Status     |= CHAN_OPEN_REMOTE;
	pChan->m_RemoteID    = bp->Get32Bit();
	pChan->m_RemoteComs  = bp->Get32Bit();
	pChan->m_RemotePacks = bp->Get32Bit();

	switch(pChan->m_Type) {
	case SSHFT_NONE:
		LogIt(_T("Connect #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
//		ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	case SSHFT_STDIO:
		LogIt(_T("Connect #%d stdio"), id);
		if ( (m_SSH2Status & SSH2_STAT_HAVESHELL) == 0 ) {
			SendMsgChannelRequesstShell(id);
		} else
			ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	case SSHFT_SFTP:
		LogIt(_T("Connect #%d sftp"), id);
		SendMsgChannelRequesstSubSystem(id, "sftp");
		break;
	case SSHFT_AGENT:
		LogIt(_T("Connect #%d agent"), id);
		ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	case SSHFT_RCP:
		LogIt(_T("Connect #%d rcp"), id);
		SendMsgChannelRequesstExec(id, RemoteStr(pChan->m_rHost));
		break;
	case SSHFT_X11:
		LogIt(_T("Connect #%d x11"), id);
		ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	case SSHFT_SOCKET_LOCAL:
	case SSHFT_SOCKET_REMOTE:
		LogIt(_T("Connect #%d %s:%d -> %s:%d"), id, pChan->m_lHost, pChan->m_lPort, pChan->m_rHost, pChan->m_rPort);
		ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	}

	return 0;
}
int Cssh::SSH2MsgChannelClose(CBuffer *bp)
{
	int id = bp->Get32Bit();
	CFifoChannel *pChan = GetFifoChannel(id);

	if ( pChan == NULL || (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
		return (-1);
	
	ChannelClose(id, CHAN_CLOSE_REMOTE);

	return 0;
}
int Cssh::SSH2MsgChannelData(CBuffer *bp, int type)
{
	int id = bp->Get32Bit();
	CFifoChannel *pChan = GetFifoChannel(id);
	CBuffer tmp;

	if ( pChan == NULL || (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
		return (-1);

	bp->GetBuf(&tmp);
	pChan->m_LocalComs += tmp.GetSize();

	ChannelCheck(IdToFdOut(id), FD_READ, (void *)&tmp);

	return 0;
}
int Cssh::SSH2MsgChannelEof(CBuffer *bp)
{
	int id = bp->Get32Bit();
	CFifoChannel *pChan = GetFifoChannel(id);

	if ( pChan == NULL || (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
		return (-1);
		
	ChannelClose(id, CHAN_ENDOF_REMOTE);

	return FALSE;
}
int Cssh::SSH2MsgChannelAdjust(CBuffer *bp)
{
	int len;
	int id = bp->Get32Bit();
	CFifoChannel *pChan = GetFifoChannel(id);

	if ( pChan == NULL || (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
		return (-1);

	if ( (len = bp->Get32Bit()) < 0 )
		return (-1);

	pChan->m_RemoteComs += len;

	ChannelCheck(IdToFdOut(id), FD_WRITE, NULL);

	return 0;
}

int Cssh::SSH2MsgChannelRequesst(CBuffer *bp)
{
	int id = bp->Get32Bit();
	CFifoChannel *pChan = GetFifoChannel(id);
	int reply;
	CStringA str;
	CBuffer tmp;
	BOOL success = FALSE;

	if ( pChan == NULL || (pChan->m_Status & CHAN_OPEN_LOCAL) == 0 )
		return (-1);

	bp->GetStr(str);
	reply = bp->Get8Bit();

	if ( str.Compare("exit-status") == 0 ) {
		bp->Get32Bit();
		if ( id == FdToId(FIFO_EXTIN) )
			m_SSH2Status &= ~SSH2_STAT_HAVESHELL;
		success = TRUE;
	
	} else if ( str.Compare("exit-signal") == 0 ) {
		bp->GetStr(str);
		bp->Get8Bit();
		bp->GetStr(str);
		bp->GetStr(str);
		if ( id == FdToId(FIFO_EXTIN) )
			m_SSH2Status &= ~SSH2_STAT_HAVESHELL;
		success = TRUE;

	} else if ( str.Compare("keepalive@openssh.com") == 0 ) {
		m_KeepAliveRecvChannelCount++;
		success = TRUE;
	}

	if ( reply ) {
		tmp.Put8Bit(success ? SSH2_MSG_CHANNEL_SUCCESS : SSH2_MSG_CHANNEL_FAILURE);
		tmp.Put32Bit(id);
		SendPacket2(&tmp);
	}
	return 0;
}
int Cssh::SSH2MsgChannelRequestReply(CBuffer *bp, int type)
{
	int num;
	int id = bp->Get32Bit();

	if ( m_ChnReqMap.GetSize() <= 0 )
		return 0;
	num = m_ChnReqMap.GetAt(0);
	m_ChnReqMap.RemoveAt(0);

	switch(num) {
	case CHAN_REQ_PTY:		// pty-req
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			return (-1);
		m_SSH2Status |= SSH2_STAT_HAVESTDIO;
		break;
	case CHAN_REQ_SHELL:	// shell
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			return (-1);
		m_SSH2Status |= SSH2_STAT_HAVESHELL;
		ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	case CHAN_REQ_SUBSYS:	// subsystem
	case CHAN_REQ_EXEC:		// exec
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			ChannelClose(id);
		else
			ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	case CHAN_REQ_X11:		// x11-req
		if ( type == SSH2_MSG_CHANNEL_FAILURE ) {
			AfxMessageBox(_T("X11-req Failure"));
			ChannelClose(id);
		} else
			ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	case CHAN_REQ_ENV:		// env
		if ( type == SSH2_MSG_CHANNEL_FAILURE ) {
			AfxMessageBox(_T("env Failure"));
			ChannelClose(id);
		} else
			ChannelCheck(IdToFdOut(id), FD_CONNECT, NULL);
		break;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////

int Cssh::SSH2MsgGlobalHostKeys(CBuffer *bp)
{
	int n, i;
	int useEntry = 0;
	int newEntry = 0;
	int delEntry = 0;
	int wrtFlag = 0;
	CIdKey key;
	CBuffer tmp;
	CString dig, kname;
	CStringArrayExt entry;
	CArray<CIdKey, CIdKey &> keyTab;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();

	if ( !m_bKnownHostUpdate )
		return FALSE;

	kname.Format(_T("%s:%d"), (LPCTSTR)m_HostName, m_HostPort);
	pApp->GetProfileStringArray(_T("KnownHosts"), kname, entry);

	if ( entry.GetSize() == 0 ) {
		// 古い形式(KnownHosts\host)を救済
		pApp->GetProfileStringArray(_T("KnownHosts"), m_HostName, entry);
		if ( entry.GetSize() != 0 ) {
			pApp->DelProfileEntry(_T("KnownHosts"), m_HostName);
			pApp->WriteProfileStringArray(_T("KnownHosts"), kname, entry);
		}
	}

	for ( n = 0 ; n < entry.GetSize() ; n++ ) {
		key.m_Uid = FALSE;
		if ( key.ReadPublicKey(entry[n]) ) {
			if ( key.ComperePublic(&m_HostKey) == 0 )
				key.m_Uid = TRUE;
			for ( i = 0 ; i < keyTab.GetSize() ; i++ ) {
				if ( key.ComperePublic(&(keyTab[i])) == 0 ) {
					wrtFlag |= 1;
					break;
				}
			}
			if ( i >= keyTab.GetSize() )
				keyTab.Add(key);
		}
	}

	while ( bp->GetSize() > 4 ) {
		bp->GetBuf(&tmp);
	
		if ( !key.GetBlob(&tmp) )
			return FALSE;

		for ( n = 0 ; n < keyTab.GetSize() ; n++ ) {
			if ( key.ComperePublic(&(keyTab[n])) == 0 ) {
				keyTab[n].m_Uid = TRUE;
				useEntry++;
				break;
			}
		}

		if ( n >= keyTab.GetSize() ) {
			key.m_Uid = TRUE;
			keyTab.Add(key);
			wrtFlag |= (key.ChkOldCertHosts(m_HostName) ? 1 : 3);
			newEntry++;
		}
	}

	for ( n = 0 ; n < keyTab.GetSize() ; n++ ) {
		if ( keyTab[n].m_Uid == FALSE ) {
			wrtFlag |= 3;
			delEntry++;
			break;
		}
	}

	if ( wrtFlag != 0 ) {
		if ( wrtFlag == 3 ) {
			dig.Format(CStringLoad(IDS_IDKEYHOSTKEYUPDATE), useEntry, newEntry, delEntry);
			if ( AfxMessageBox(dig, MB_ICONQUESTION | MB_YESNO) != IDYES )
				return TRUE;
		}

		entry.RemoveAll();
		for ( n = 0 ; n < keyTab.GetSize() ; n++ ) {
			if ( keyTab[n].m_Uid == TRUE ) {
				keyTab[n].WritePublicKey(dig, FALSE);
				entry.Add(dig);
			}
		}
		pApp->WriteProfileStringArray(_T("KnownHosts"), kname, entry);
	}

	return TRUE;
}
int Cssh::SSH2MsgGlobalRequest(CBuffer *bp)
{
	int reply;
	CBuffer tmp;
	CStringA str;
	CString msg;
	BOOL success = FALSE;

	bp->GetStr(str);
	reply = bp->Get8Bit();

	if ( str.Compare("keepalive@openssh.com") == 0 ) {
		m_KeepAliveRecvGlobalCount++;
		success = TRUE;

	} else if ( str.Compare("hostkeys-00@openssh.com") == 0 ) {
		success = SSH2MsgGlobalHostKeys(bp);

	} else if ( str.Compare("elevation") == 0 ) {	// RFC 8308
		success = TRUE;
		bp->Get8Bit();	// boolean     elevation performed

#ifdef	DEBUG
	} else if ( !str.IsEmpty() ) {
		msg.Format(_T("Get Msg Global Request '%s'"), LocalStr(str));
		AfxMessageBox(msg);
#endif
	}

	if ( reply > 0 ) {
		tmp.Put8Bit(success ? SSH2_MSG_REQUEST_SUCCESS : SSH2_MSG_REQUEST_FAILURE);
		SendPacket2(&tmp);
	}
	return 0;
}
int Cssh::SSH2MsgGlobalRequestReply(CBuffer *bp, int type)
{
	int num;
	CString str;

	if ( m_GlbReqMap.GetSize() <= 0 ) {
		AfxMessageBox(CStringLoad(IDE_SSHGLOBALREQERROR));
		return 0;
	}
	num = m_GlbReqMap.GetAt(0);
	m_GlbReqMap.RemoveAt(0);

	if ( (WORD)num == 0xFFFF ) {	// KeepAlive Reply
		if ( ++m_KeepAliveReplyCount < 0 )
			m_KeepAliveReplyCount = 0;
		return 0;
	}

	if ( m_bPfdConnect > 0 && --m_bPfdConnect <= 0 ) {
		m_bConnect = TRUE;
		m_pFifoMid->SendFdEvents(FIFO_EXTOUT, FD_CONNECT, NULL);
	}

	if ( type == SSH2_MSG_REQUEST_FAILURE && num < m_Permit.GetSize() ) {
		str.Format(_T("Global Request Failure %s:%d->%s:%d"),
			(LPCTSTR)m_Permit[num].m_lHost, m_Permit[num].m_lPort, (LPCTSTR)m_Permit[num].m_rHost, m_Permit[num].m_rPort);
		AfxMessageBox(str);
		m_Permit.RemoveAt(num);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////

void Cssh::ReceivePacket2(CBuffer *bp)
{
	DEBUGLOG("ReceivePacket2 %d(%d) Seq=%d", bp->PTR8BIT(bp->GetPtr()), bp->GetSize(), m_RecvPackSeq);
	DEBUGDUMP(bp->GetPtr(), bp->GetSize());

	CStringA str, tmp;
	int type = bp->Get8Bit();

	switch(type) {
	case SSH2_MSG_KEXINIT:
		if ( SSH2MsgKexInit(bp) )
			goto DISCONNECT;
		m_SSH2Status |= SSH2_STAT_HAVEPROP;
		switch(m_DhMode) {
		case DHMODE_GROUP_1:
		case DHMODE_GROUP_14:
		case DHMODE_GROUP_14_256:
		case DHMODE_GROUP_15_512:
		case DHMODE_GROUP_16_512:
		case DHMODE_GROUP_17_512:
		case DHMODE_GROUP_18_512:
			SendMsgKexDhInit();
			break;
		case DHMODE_GROUP_GEX:
		case DHMODE_GROUP_GEX256:
			SendMsgKexDhGexRequest();
			break;
		case DHMODE_ECDH_S2_N256:
		case DHMODE_ECDH_S2_N384:
		case DHMODE_ECDH_S2_N521:
			if ( !SendMsgKexEcdhInit() )
				goto DISCONNECT;
			break;
		case DHMODE_CURVE25519:
		case DHMODE_CURVE448:
		case DHMODE_SNT761X25519:
			SendMsgKexCurveInit();
			break;
		default:
			goto DISCONNECT;
		}
		break;
	case SSH2_MSG_KEXDH_REPLY:
//	case SSH2_MSG_KEX_DH_GEX_GROUP:
//	case SSH2_MSG_KEX_ECDH_REPLY:
		if ( (m_SSH2Status & SSH2_STAT_HAVEPROP) == 0 )
			goto DISCONNECT;
		switch(m_DhMode) {
		case DHMODE_GROUP_1:
		case DHMODE_GROUP_14:
		case DHMODE_GROUP_14_256:
		case DHMODE_GROUP_15_512:
		case DHMODE_GROUP_16_512:
		case DHMODE_GROUP_17_512:
		case DHMODE_GROUP_18_512:
			if ( SSH2MsgKexDhReply(bp) )
				goto DISCONNECT;
			m_SSH2Status &= ~SSH2_STAT_HAVEPROP;
			m_SSH2Status |= SSH2_STAT_HAVEKEYS;
			SendMsgNewKeys();
			break;
		case DHMODE_GROUP_GEX:
		case DHMODE_GROUP_GEX256:
			if ( SSH2MsgKexDhGexGroup(bp) )
				goto DISCONNECT;
			break;
		case DHMODE_ECDH_S2_N256:
		case DHMODE_ECDH_S2_N384:
		case DHMODE_ECDH_S2_N521:
			if ( SSH2MsgKexEcdhReply(bp) )
				goto DISCONNECT;
			m_SSH2Status &= ~SSH2_STAT_HAVEPROP;
			m_SSH2Status |= SSH2_STAT_HAVEKEYS;
			SendMsgNewKeys();
			break;
		case DHMODE_CURVE25519:
		case DHMODE_CURVE448:
		case DHMODE_SNT761X25519:
			if ( SSH2MsgKexCurveReply(bp) )
				goto DISCONNECT;
			m_SSH2Status &= ~SSH2_STAT_HAVEPROP;
			m_SSH2Status |= SSH2_STAT_HAVEKEYS;
			SendMsgNewKeys();
			break;
		default:
			goto DISCONNECT;
		}
		break;
	case SSH2_MSG_KEX_DH_GEX_REPLY:
		if ( (m_SSH2Status & SSH2_STAT_HAVEPROP) == 0 )
			goto DISCONNECT;
		if ( SSH2MsgKexDhGexReply(bp) )
			goto DISCONNECT;
		m_SSH2Status &= ~SSH2_STAT_HAVEPROP;
		m_SSH2Status |= SSH2_STAT_HAVEKEYS;
		SendMsgNewKeys();
		break;
	case SSH2_MSG_KEXRSA_PUBKEY:
		if ( (m_SSH2Status & SSH2_STAT_HAVEPROP) == 0 )
			goto DISCONNECT;
		if ( SSH2MsgKexRsaPubkey(bp) )
			goto DISCONNECT;
		m_SSH2Status |= SSH2_STAT_HAVERSAPUB;
		break;
	case SSH2_MSG_KEXRSA_DONE:
		if ( (m_SSH2Status & SSH2_STAT_HAVEPROP) == 0 || (m_SSH2Status & SSH2_STAT_HAVERSAPUB) == 0 )
			goto DISCONNECT;
		if ( SSH2MsgKexRsaDone(bp) )
			goto DISCONNECT;
		m_SSH2Status &= ~SSH2_STAT_HAVERSAPUB;
		m_SSH2Status &= ~SSH2_STAT_HAVEPROP;
		m_SSH2Status |= SSH2_STAT_HAVEKEYS;
		SendMsgNewKeys();
		break;

	case SSH2_MSG_NEWKEYS:
		if ( (m_SSH2Status & SSH2_STAT_HAVEKEYS) == 0 )
			goto DISCONNECT;
		if ( SSH2MsgNewKeys(bp) )
			goto DISCONNECT;
		m_SSH2Status &= ~SSH2_STAT_SENTKEXINIT;
		m_SSH2Status &= ~SSH2_STAT_HAVEKEYS;
		m_SSH2Status |= SSH2_STAT_HAVESESS;
		if ( (m_SSH2Status & SSH2_STAT_HAVELOGIN) == 0 ) {
			m_AuthStat = AST_START;
			SendMsgServiceRequest("ssh-userauth");
		} else {
			if ( m_EncCmp.m_Mode == 4 )
				m_EncCmp.Init(NULL, MODE_ENC, COMPLEVEL);
			if ( m_DecCmp.m_Mode == 4 )
				m_DecCmp.Init(NULL, MODE_DEC, COMPLEVEL);
			DelayedCheckExec();
		}
		break;
	case SSH2_MSG_SERVICE_ACCEPT:
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		bp->GetStr(str);
		if ( str.Compare("ssh-userauth") == 0 ) {
			if ( !SendMsgUserAuthRequest(NULL) )
				break;
		} else
			goto DISCONNECT;
		break;

	case SSH2_MSG_EXT_INFO:
		if ( SSH2MsgExtInfo(bp) )
			goto DISCONNECT;
		break;

	case SSH2_MSG_USERAUTH_SUCCESS:
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		if ( m_EncCmp.m_Mode == 4 )
			m_EncCmp.Init(NULL, MODE_ENC, COMPLEVEL);
		if ( m_DecCmp.m_Mode == 4 )
			m_DecCmp.Init(NULL, MODE_DEC, COMPLEVEL);
		m_SSH2Status |= SSH2_STAT_HAVELOGIN;
		if ( !m_pDocument->m_CmdsPath.IsEmpty() ) {
			m_pFifoMid->SendFdCommand(FIFO_EXTOUT, FIFO_CMD_MSGQUEIN, FIFO_QCMD_RCPDOWNLOAD);
			break;
		}
		if ( (m_SSH2Status & SSH2_STAT_HAVEPFWD) == 0 ) {
			m_SSH2Status |= SSH2_STAT_HAVEPFWD;
			PortForward();
		}
		if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) && (m_SSH2Status & SSH2_STAT_HAVESTDIO) == 0 ) {
			CFifoChannel *pChan = GetFifoChannel(-2);
			if ( pChan != NULL )
				SendMsgChannelOpen(pChan->m_LocalID, "session");
		} else
			GetFifoChannel(-2);
		if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHKEEPAL) && m_pDocument->m_TextRam.m_SshKeepAlive > 0 )
			m_KeepAliveTiimerId = m_pFifoMid->DocMsgSetTimer(m_pDocument->m_TextRam.m_SshKeepAlive * 1000, TIMEREVENT_SOCK | TIMEREVENT_INTERVAL, this);
		if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFTPORY) )
			m_pFifoMid->DocMsgCommand(IDM_SFTP);
		break;
	case SSH2_MSG_USERAUTH_FAILURE:
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		bp->GetStr(str);
		if ( !SendMsgUserAuthRequest(str) )
			break;
		break;
	case SSH2_MSG_USERAUTH_BANNER:
		bp->GetStr(str);
		SendTextMsg(str, str.GetLength());
		break;

//	case SSH2_MSG_USERAUTH_PK_OK:				// 60	publickey
//	case SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ:	// 60	password
//	case SSH2_MSG_USERAUTH_GSSAPI_RESPONSE:		// 60	gssapi
	case SSH2_MSG_USERAUTH_INFO_REQUEST:		// 60	keyboard-interactive
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		switch(m_AuthMode) {
		case AUTH_MODE_PUBLICKEY:
			if ( SSH2MsgUserAuthPkOk(bp) )
				goto DISCONNECT;
			break;
		case AUTH_MODE_PASSWORD:
			if ( SSH2MsgUserAuthPasswdChangeReq(bp) )
				goto DISCONNECT;
			break;
		case AUTH_MODE_KEYBOARD:
			if ( SSH2MsgUserAuthInfoRequest(bp) )
				goto DISCONNECT;
			break;
		case AUTH_MODE_GSSAPI:
			if ( SSH2MsgUserAuthGssapiProcess(bp, type) )
				goto DISCONNECT;
			break;
		default:
			goto DISCONNECT;
		}
		break;

	case SSH2_MSG_USERAUTH_GSSAPI_TOKEN:
	case SSH2_MSG_USERAUTH_GSSAPI_ERROR:
	case SSH2_MSG_USERAUTH_GSSAPI_ERRTOK:
		if ( SSH2MsgUserAuthGssapiProcess(bp, type) )
			goto DISCONNECT;
		break;

	case SSH2_MSG_CHANNEL_OPEN:
		if ( SSH2MsgChannelOpen(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_OPEN_FAILURE:
	case SSH2_MSG_CHANNEL_OPEN_CONFIRMATION:
		if ( SSH2MsgChannelOpenReply(bp, type) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_CLOSE:
		if ( SSH2MsgChannelClose(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_DATA:
	case SSH2_MSG_CHANNEL_EXTENDED_DATA:
		if ( SSH2MsgChannelData(bp, type) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_EOF:
		if ( SSH2MsgChannelEof(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_REQUEST:
		if ( SSH2MsgChannelRequesst(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_WINDOW_ADJUST:
		if ( SSH2MsgChannelAdjust(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_CHANNEL_SUCCESS:
	case SSH2_MSG_CHANNEL_FAILURE:
		if ( SSH2MsgChannelRequestReply(bp, type) )
			goto DISCONNECT;
		break;

	case SSH2_MSG_GLOBAL_REQUEST:
		if ( SSH2MsgGlobalRequest(bp) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_REQUEST_SUCCESS:
	case SSH2_MSG_REQUEST_FAILURE:
		if ( SSH2MsgGlobalRequestReply(bp, type) )
			goto DISCONNECT;
		break;

	case SSH2_MSG_DISCONNECT:
		bp->Get32Bit();
		bp->GetStr(str);
		tmp.Format("SSH2 Receive Disconnect Message\n%s", str);
		AfxMessageBox(LocalStr(tmp));
		break;

	case SSH2_MSG_IGNORE:
	case SSH2_MSG_UNIMPLEMENTED:
		break;
	case SSH2_MSG_DEBUG:
#ifdef	DEBUG_XXX
		bp->Get8Bit();
		bp->GetStr(str);
		tmp.Format("SSH2 Debug Message\n%s", str);
		AfxMessageBox(LocalStr(tmp), MB_ICONINFORMATION);
#endif
		break;

	default:
		SendMsgUnimplemented();
		break;

	DISCONNECT:
		str.Format("Packet Type %d Error", type);
		SendDisconnect2(SSH2_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT, str);
		str.Format("SSH2 Receive Packet Error Type=%d Status=%04o\nSend Disconnect Message...", type, m_SSH2Status);
		AfxMessageBox(MbsToTstr(str), MB_ICONSTOP);
		break;
	}
}
