// ssh.cpp: Cssh クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "PassDlg.h"
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
// Cssh 構築/消滅
//////////////////////////////////////////////////////////////////////

Cssh::Cssh(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = ESCT_SSH_MAIN;
	m_SaveDh = NULL;
	m_EcdhClientKey = NULL;
	m_SSHVer = 0;
	m_InPackStat = 0;
	m_pListFilter = NULL;

	for ( int n = 0 ; n < 6 ; n++ )
		m_VKey[n] = NULL;
}
Cssh::~Cssh()
{
	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);

	for ( int n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
	}

	((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);
}
int Cssh::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType, void *pAddrInfo)
{
  try {
	int n, i;
	CString str;
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CIdKey IdKey, *pKey;

	//CString tmp;
	//CCipher::BenchMark(tmp);
	//CExtSocket::OnReciveCallBack((void *)(LPCSTR)tmp, tmp.GetLength(), 0);

	//CString tmp;
	//CMacomp::BenchMark(tmp);
	//CExtSocket::OnReciveCallBack((void *)(LPCSTR)tmp, tmp.GetLength(), 0);

	m_SSHVer = 0;
	m_ServerVerStr = "";
	m_ClientVerStr = "";
	m_InPackStat = 0;
	m_PacketStat = 0;
	m_SSH2Status = 0;
	m_SendPackSeq = m_SendPackLen = 0;
	m_RecvPackSeq = m_RecvPackLen = 0;
	m_CipherMode = SSH_CIPHER_NONE;
	m_SessionIdLen = 0;
	m_StdChan = (-1);
	m_AuthStat = 0;
	m_AuthMode = AUTH_MODE_NONE;
	m_HostName = m_pDocument->m_ServerEntry.m_HostName;
	m_DhMode = DHMODE_GROUP_1;
	m_GlbReqMap.RemoveAll();
	m_ChnReqMap.RemoveAll();
	m_Chan.RemoveAll();
	m_ChanNext = m_ChanFree = m_ChanFree2 = (-1);
	m_OpenChanCount = 0;
	m_pListFilter = NULL;
	m_Permit.RemoveAll();
	m_MyPeer.Clear();
	m_HisPeer.Clear();
	m_Incom.Clear();
	m_InPackBuf.Clear();
	m_HostKey.Close();
	m_IdKey.Close();
	m_IdKeyTab.RemoveAll();
	m_IdKeyPos = 0;
	SetRecvBufSize(64 * 1024);

	srand((UINT)time(NULL));

	if ( !m_pDocument->m_ServerEntry.m_IdkeyName.IsEmpty() ) {
		if ( !IdKey.LoadPrivateKey(m_pDocument->m_ServerEntry.m_IdkeyName, m_pDocument->m_ServerEntry.m_PassName) ) {
			CIdKeyFileDlg dlg;
			dlg.m_OpenMode  = 1;
			dlg.m_Title   = _T("SSH鍵ファイルの読み込み");
			dlg.m_Message = _T("作成時に設定したパスフレーズを入力してください");
			dlg.m_IdkeyFile = m_pDocument->m_ServerEntry.m_IdkeyName;
			if ( dlg.DoModal() != IDOK ) {
				m_pDocument->m_ServerEntry.m_IdkeyName.Empty();
				return FALSE;
			}
			if ( !IdKey.LoadPrivateKey(dlg.m_IdkeyFile, dlg.m_PassName) ) {
				AfxMessageBox(IDE_IDKEYLOADERROR);
				return FALSE;
			}
		}
		IdKey.m_Pass = m_pDocument->m_ServerEntry.m_PassName;
		m_IdKeyTab.Add(IdKey);
	}

	for ( n = 0 ; n < m_pDocument->m_ParamTab.m_IdKeyList.GetSize() ; n++ ) {
		if ( (pKey = pMain->m_IdKeyTab.GetUid(m_pDocument->m_ParamTab.m_IdKeyList.GetVal(n))) != NULL ) {
			if ( pKey->m_Type != IDKEY_NONE && pKey->m_Pass.Compare(m_pDocument->m_ServerEntry.m_PassName) == 0 )
				m_IdKeyTab.Add(*pKey);
		}
	}

	for ( n = 0 ; n <= AST_DONE ; n++ )
		m_AuthReqTab[n] = AST_DONE;
	i = AST_START;

	for ( n = 0 ; m_pDocument->m_ParamTab.GetPropNode(11, n, str) ; n++ ) {
		if ( str.CompareNoCase("publickey") == 0 ) {
			m_AuthReqTab[i] = AST_PUBKEY_TRY;
			i = AST_PUBKEY_TRY;
		} else if ( str.CompareNoCase("hostbased") == 0 ) {
			m_AuthReqTab[i] = AST_HOST_TRY;
			i = AST_HOST_TRY;
		} else if ( str.CompareNoCase("password") == 0 ) {
			m_AuthReqTab[i] = AST_PASS_TRY;
			i = AST_PASS_TRY;
		} else if ( str.CompareNoCase("keyboard-interactive") == 0 ) {
			m_AuthReqTab[i] = AST_KEYB_TRY;
			i = AST_KEYB_TRY;
		}
	}

	m_EncCip.Init("none", MODE_ENC);
	m_DecCip.Init("none", MODE_DEC);
	m_EncMac.Init("none");
	m_DecMac.Init("none");
	m_EncCmp.Init("none", MODE_ENC);
	m_DecCmp.Init("none", MODE_DEC);

	m_CipTabMax = 0;
	for ( n = 0 ; n < 8 && m_pDocument->m_ParamTab.GetPropNode(0, n, str) ; n++ )
		m_CipTab[m_CipTabMax++] = m_EncCip.GetNum(str);

	n = m_pDocument->m_ParamTab.GetPropNode(2, 0, str);
	m_CompMode = (n == FALSE || str.Compare("none") == 0 ? FALSE : TRUE);

	if ( !CExtSocket::Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType, pAddrInfo) )
		return FALSE;

	return TRUE;
  } catch(...) {
	return FALSE;
  }
}
void Cssh::Close()
{
	m_Chan.RemoveAll();
	CExtSocket::Close();
}
int Cssh::Send(const void* lpBuf, int nBufLen, int nFlags)
{
  try {
	CBuffer tmp;
	switch(m_SSHVer) {
	case 1:
		tmp.Put8Bit(SSH_CMSG_STDIN_DATA);
		tmp.PutBuf((LPBYTE)lpBuf, nBufLen);
		SendPacket(&tmp);
		break;
	case 2:
		if ( m_StdChan == (-1) || m_StdInRes.GetSize() > 0 || !CHAN_OK(m_StdChan) )
			m_StdInRes.Apend((LPBYTE)lpBuf, nBufLen);
		else
			m_Chan[m_StdChan].m_pFilter->Send((LPBYTE)lpBuf, nBufLen);
		break;
	}
	return nBufLen;
  } catch(...) {
	return 0;
  }
}
void Cssh::OnReciveCallBack(void* lpBuf, int nBufLen, int nFlags)
{
  try {
	int crc, ch;
	CBuffer *bp, tmp, cmp, dec;
	CString str;

	if ( nFlags != 0 )
		return;

	m_Incom.Apend((LPBYTE)lpBuf, nBufLen);

	for ( ; ; ) {
		switch(m_InPackStat) {
		case 0:		// Fast Verstion String
			while ( m_Incom.GetSize() > 0 ) {
				ch = m_Incom.Get8Bit();
				if ( ch == '\n' ) {
					if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSH1MODE) &&
						   (m_ServerVerStr.Mid(0, 5).Compare("SSH-2") == 0 ||
							m_ServerVerStr.Mid(0, 8).Compare("SSH-1.99") == 0) ) {
						m_ClientVerStr = "SSH-2.0-OpenSSH-4.0 RLogin-1.10";
						m_InPackStat = 3;
						m_SSHVer = 2;
					} else {
						m_ClientVerStr = "SSH-1.5-OpenSSH-4.0 RLogin-1.10";
						m_InPackStat = 1;
						m_SSHVer = 1;
					}
					str.Format("%s\r\n", m_ClientVerStr);
					CExtSocket::Send((LPCSTR)str, str.GetLength());
					break;
				} else if ( ch != '\r' )
					m_ServerVerStr += (char)ch;
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
				throw "ssh1 packet length error";
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
				throw "ssh1 packet crc error";
			}
			bp->Consume(m_InPackPad);
			bp->ConsumeEnd(4);
			cmp.Clear();
			m_DecCmp.UnCompress(bp->GetPtr(), bp->GetSize(), &cmp);
			bp = &cmp;
			RecivePacket(bp);
			break;

		case 3:		// SSH2 Packet Lenght
			if ( m_Incom.GetSize() < m_DecCip.GetBlockSize() )
				return;
			m_InPackBuf.Clear();
			m_DecCip.Cipher(m_Incom.GetPtr(), m_DecCip.GetBlockSize(), &m_InPackBuf);
			m_Incom.Consume(m_DecCip.GetBlockSize());
			m_InPackLen = m_InPackBuf.PTR32BIT(m_InPackBuf.GetPtr());
			if ( m_InPackLen < 5 || m_InPackLen > (256 * 1024) ) {
				m_Incom.Clear();
				SendDisconnect2(1, "Packet Len Error");
				throw "ssh2 packet length error";
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
					throw "ssh2 mac miss match error";
				}
				m_Incom.Consume(m_DecMac.GetBlockSize());
			}
			m_RecvPackSeq++;
			m_RecvPackLen++;
			RecivePacket2(bp);
			break;
		}
	}
 } catch(...) {
 }
}
void Cssh::SendWindSize(int x, int y)
{
  try {
	CBuffer tmp;
	switch(m_SSHVer) {
	case 1:
		tmp.Put8Bit(SSH_CMSG_WINDOW_SIZE);
		tmp.Put32Bit(y);
		tmp.Put32Bit(x);
		tmp.Put32Bit(0);
		tmp.Put32Bit(0);
		SendPacket(&tmp);
		break;
	case 2:
		if ( m_StdChan == (-1) )
			break;
		tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
		tmp.Put32Bit(m_Chan[m_StdChan].m_RemoteID);
		tmp.PutStr("window-change");
		tmp.Put8Bit(0);
		tmp.Put32Bit(x);
		tmp.Put32Bit(y);
		tmp.Put32Bit(0);
		tmp.Put32Bit(0);
		SendPacket2(&tmp);
		break;
	}
 } catch(...) {
 }
}
void Cssh::OnTimer(UINT_PTR nIDEvent)
{
	SendMsgKeepAlive();
}
void Cssh::OnRecvEmpty()
{
	if ( m_StdChan != (-1) )
		ChannelPolling(m_StdChan);
}
void Cssh::OnSendEmpty()
{
	if ( m_StdChan != (-1) )
		ChannelPolling(m_StdChan);
	for ( CFilter *fp = m_pListFilter ; fp != NULL ; fp = fp->m_pNext )
		fp->OnSendEmpty();
}
void Cssh::GetStatus(CString &str)
{
	int n;
	CString tmp;

	CExtSocket::GetStatus(str);

	str += "\r\n";
	tmp.Format("SSH Server: %d\r\n", m_SSHVer);
	str += tmp;

	tmp.Format("Version: %s\r\n", m_ServerVerStr);
	str += tmp;
	tmp.Format("Kexs: %s\r\n", m_SProp[PROP_KEX_ALGS]);
	str += tmp;
	tmp.Format("Keys: %s\r\n", m_SProp[PROP_HOST_KEY_ALGS]);
	str += tmp;
	tmp.Format("Auth: %s\r\n", m_AuthMeta);
	str += tmp;
	tmp.Format("Cipher CtoS: %s\r\n", m_SProp[PROP_ENC_ALGS_CTOS]);
	str += tmp;
	tmp.Format("Cipher StoC: %s\r\n", m_SProp[PROP_ENC_ALGS_STOC]);
	str += tmp;
	tmp.Format("Mac CtoS: %s\r\n", m_SProp[PROP_MAC_ALGS_CTOS]);
	str += tmp;
	tmp.Format("Mac StoC: %s\r\n", m_SProp[PROP_MAC_ALGS_STOC]);
	str += tmp;
	tmp.Format("Compess CtoS: %s\r\n", m_SProp[PROP_COMP_ALGS_CTOS]);
	str += tmp;
	tmp.Format("Compess StoC: %s\r\n", m_SProp[PROP_COMP_ALGS_STOC]);
	str += tmp;
	tmp.Format("Language CtoS: %s\r\n", m_SProp[PROP_LANG_CTOS]);
	str += tmp;
	tmp.Format("Language StoC: %s\r\n", m_SProp[PROP_LANG_STOC]);
	str += tmp;

	str += "\r\n";
	tmp.Format("Kexs: %s\r\n", m_VProp[PROP_KEX_ALGS]);
	str += tmp;
	tmp.Format("Encode: %s + %s + %s\r\n", m_EncCmp.GetTitle(), m_EncCip.GetTitle(), m_EncMac.GetTitle());
	str += tmp;
	tmp.Format("Decode: %s + %s + %s\r\n", m_DecCmp.GetTitle(), m_DecCip.GetTitle(), m_DecMac.GetTitle());
	str += tmp;

	str += "UserAuth: ";
	switch(m_AuthMode) {
	case AUTH_MODE_NONE:		str += "none"; break;
	case AUTH_MODE_PUBLICKEY:	str += "publickey "; str += m_IdKey.GetName(); break;
	case AUTH_MODE_PASSWORD:	str += "password"; break;
	case AUTH_MODE_KEYBOARD:	str += "keyboard-interactive"; break;
	case AUTH_MODE_HOSTBASED:	str += "hostbased "; str += m_IdKey.GetName(); break;
	}
	str += "\r\n";

	m_HostKey.FingerPrint(tmp);
	str += "\r\n";
	str += tmp;

	str += "\r\n";
	tmp.Format("Open Channel: %d\r\n", m_OpenChanCount);
	str += tmp;

	for ( n = m_ChanNext ; n >= 0 ; n = m_Chan[n].m_Next ) {
		tmp.Format("Chanel: RemoteId=%d LocalId=%d Status=%x Type=%s Read=%lld Write=%lld\r\n",
			m_Chan[n].m_RemoteID, m_Chan[n].m_LocalID, m_Chan[n].m_Status, m_Chan[n].m_TypeName, m_Chan[n].m_WriteByte, m_Chan[n].m_ReadByte);
		str += tmp;
	}
}

//////////////////////////////////////////////////////////////////////
// SSH1 Function
//////////////////////////////////////////////////////////////////////

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

	CExtSocket::Send(ext.GetPtr(), ext.GetSize());
}

int Cssh::SMsgPublicKey(CBuffer *bp)
{
	int n, i;
	RSA *host_key;
	RSA *server_key;
	CBuffer tmp;

	for ( n = 0 ; n < 8 ; n++ )
		m_Cookie[n] = bp->Get8Bit();

	server_key = RSA_new();
	server_key->n = BN_new();
	server_key->e = BN_new();
	n = bp->Get32Bit();
	bp->GetBIGNUM(server_key->e);
	bp->GetBIGNUM(server_key->n);

	host_key = RSA_new();
	host_key->n = BN_new();
	host_key->e = BN_new();
	n = bp->Get32Bit();
	bp->GetBIGNUM(host_key->e);
	bp->GetBIGNUM(host_key->n);

	m_HostKey.Create(IDKEY_RSA1);
	BN_copy(m_HostKey.m_Rsa->e, host_key->e);
	BN_copy(m_HostKey.m_Rsa->n, host_key->n);
	if ( !m_HostKey.HostVerify(m_HostName) )
		return FALSE;

	m_ServerFlag  = bp->Get32Bit();
	m_SupportCipher = bp->Get32Bit();
	m_SupportAuth = bp->Get32Bit();

	EVP_MD *evp_md = (EVP_MD *)EVP_md5();
	EVP_MD_CTX md;
    BYTE nbuf[2048];
	BYTE obuf[EVP_MAX_MD_SIZE];
	int len;
	BIGNUM *key;

    EVP_DigestInit(&md, evp_md);

	len = BN_num_bytes(host_key->n);
    BN_bn2bin(host_key->n, nbuf);
    EVP_DigestUpdate(&md, nbuf, len);

	len = BN_num_bytes(server_key->n);
    BN_bn2bin(server_key->n, nbuf);
    EVP_DigestUpdate(&md, nbuf, len);
    EVP_DigestUpdate(&md, m_Cookie, 8);
    EVP_DigestFinal(&md, obuf, NULL);
	memcpy(m_SessionId, obuf, 16);

	for ( n = 0 ; n < 32 ; n++ ) {
		if ( (n % 2) == 0 )
			i = rand();
		m_SessionKey[n] = (BYTE)(i);
		i >>= 8;
	}

	key = BN_new();
	BN_set_word(key, 0);
	for ( n  = 0; n < 32 ; n++ ) {
		BN_lshift(key, key, 8);
		if ( n < 16 )
			BN_add_word(key, m_SessionKey[n] ^ m_SessionId[n]);
		else
			BN_add_word(key, m_SessionKey[n]);
    }

	if ( BN_cmp(server_key->n, host_key->n) < 0 ) {
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
int Cssh::SMsgAuthRsaChallenge(CBuffer *bp)
{
	int n;
	int len;
	CBuffer tmp;
	BIGNUM *challenge;
	MD5_CTX md;
	BYTE buf[32], res[16];
	CSpace inbuf, otbuf;


	for ( ; ; ) {
		if ( m_IdKeyPos >= m_IdKeyTab.GetSize() )
			return FALSE;
		m_IdKey = m_IdKeyTab[m_IdKeyPos++];
		if ( m_IdKey.m_Type == IDKEY_RSA1 || m_IdKey.m_Type == IDKEY_RSA2 )
			break;
	}

	if ( (challenge = BN_new()) == NULL )
		return FALSE;

	bp->GetBIGNUM(challenge);
	inbuf.SetSize(BN_num_bytes(challenge));
	BN_bn2bin(challenge, inbuf.GetPtr());

	otbuf.SetSize(BN_num_bytes(m_IdKey.m_Rsa->n));

	if ( (len = RSA_private_decrypt(inbuf.GetSize(), inbuf.GetPtr(), otbuf.GetPtr(), m_IdKey.m_Rsa, RSA_PKCS1_PADDING)) <= 0 ) {
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
	MD5_Init(&md);
	MD5_Update(&md, buf, 32);
	MD5_Update(&md, m_SessionId, 16);
	MD5_Final(res, &md);	

	tmp.Put8Bit(SSH_CMSG_AUTH_RSA_RESPONSE);
	for ( n = 0 ; n < 16 ; n++ )
		tmp.Put8Bit(res[n]);
	SendPacket(&tmp);

	BN_free(challenge);
	return TRUE;
}
void Cssh::RecivePacket(CBuffer *bp)
{
	int n;
	CString str;
	CBuffer tmp;
	int type = bp->Get8Bit();

	if ( type == SSH_MSG_DISCONNECT ) {
		bp->GetStr(str);
		CExtSocket::OnReciveCallBack((void *)((LPCSTR)str), str.GetLength(), 0);
		Close();
		return;
	} else if ( type == SSH_MSG_DEBUG ) {
		bp->GetStr(str);
		CExtSocket::OnReciveCallBack((void *)((LPCSTR)str), str.GetLength(), 0);
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
		tmp.PutStr(m_pDocument->m_ServerEntry.m_UserName);
		SendPacket(&tmp);
		m_PacketStat = 2;
		break;
	case 2:		// Login User Status
		m_PacketStat = 5;
		if ( type == SSH_SMSG_SUCCESS )
			break;
		if ( type != SSH_SMSG_FAILURE )
			goto DISCONNECT;
		if ( (m_SupportAuth & (1 << SSH_AUTH_RSA)) != 0 && m_IdKey.m_Rsa != NULL ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_RSA);
			tmp.PutBIGNUM(m_IdKey.m_Rsa->n);
			SendPacket(&tmp);
			m_PacketStat = 10;
			break;
		}
	GO_AUTH_TIS:
		if ( (m_SupportAuth & (1 << SSH_AUTH_TIS)) != 0 ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_TIS);
			SendPacket(&tmp);
			m_PacketStat = 3;
			break;
		}
	GO_AUTH_PASS:
		if ( (m_SupportAuth & (1 << SSH_AUTH_PASSWORD)) != 0 ) {
			tmp.Put8Bit(SSH_CMSG_AUTH_PASSWORD);
			tmp.PutStr(m_pDocument->m_ServerEntry.m_PassName);
			SendPacket(&tmp);
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
		tmp.PutStr(m_pDocument->m_ServerEntry.m_PassName);
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
			m_EncCmp.Init("zlib", MODE_ENC, COMPLEVEL);
			m_DecCmp.Init("zlib", MODE_DEC, COMPLEVEL);
		} else if ( type == SSH_SMSG_FAILURE ) {
			m_EncCmp.Init("node", MODE_ENC, COMPLEVEL);
			m_DecCmp.Init("none", MODE_DEC, COMPLEVEL);
		} else
			goto DISCONNECT;
	NOCOMPRESS:
		tmp.Put8Bit(SSH_CMSG_REQUEST_PTY);
		tmp.PutStr(m_pDocument->m_ServerEntry.m_TermName);
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

		str.Format("%s+%s", m_EncCmp.GetTitle(), m_EncCip.GetTitle());
		m_pDocument->SetStatus(str);
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
			CExtSocket::OnReciveCallBack(bp->GetPtr(), n, 0);
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
//////////////////////////////////////////////////////////////////////

void Cssh::LogIt(LPCSTR format, ...)
{
    va_list args;
	CString str;

	va_start(args, format);
	str.FormatV(format, args);
	va_end(args);

	if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) ) {
		GetMainWnd()->SetMessageText(str);
		return;
	}

	CString tmp;
	CTime now = CTime::GetCurrentTime();
	DWORD size;
	TCHAR buf[MAX_COMPUTERNAME_LENGTH + 2];

	tmp = now.Format("%b %d %H:%M:%S ");
	size = MAX_COMPUTERNAME_LENGTH;
	if ( GetComputerName(buf, &size) )
		tmp += buf;
	tmp += " : ";
	tmp += str;
	tmp += "\r\n";

	CExtSocket::OnReciveCallBack((void *)(LPCSTR)tmp, tmp.GetLength(), 0);
}
int Cssh::ChannelOpen()
{
	int n, mx, id;

	if ( (id = m_ChanFree) == (-1) ) {
		for ( mx = 0 ; (n = m_ChanFree2) != (-1) ; mx++ ) {
			m_ChanFree2 = m_Chan[n].m_Next;
			m_Chan[n].m_Next = m_ChanFree;
			m_ChanFree = n;
		}
		if ( mx < 5 ) {
			n = (int)m_Chan.GetSize();
			mx = n + (10 - mx);
			m_Chan.SetSize(mx);
			for ( ; n < mx ; n++ ) {
				m_Chan[n].m_Next = m_ChanFree;
				m_ChanFree = n;
			}
		}
		id = m_ChanFree;
	}

	m_ChanFree = m_Chan[id].m_Next;
	m_Chan[id].m_Next = m_ChanNext;
	m_ChanNext = id;

	m_Chan[id].m_pDocument  = m_pDocument;
	m_Chan[id].m_pSsh       = this;

	m_Chan[id].m_Status     = 0;
	m_Chan[id].m_TypeName   = "";
	m_Chan[id].m_RemoteID   = (-1);
	m_Chan[id].m_LocalID    = id;
	m_Chan[id].m_RemoteWind = 0;
	m_Chan[id].m_RemoteMax  = 0;
	m_Chan[id].m_LocalComs  = 0;
	m_Chan[id].m_LocalWind  = CHAN_SES_WINDOW_DEFAULT;
	m_Chan[id].m_LocalPacks = CHAN_SES_PACKET_DEFAULT;
	m_Chan[id].m_Eof        = 0;
	m_Chan[id].m_Input.Clear();
	m_Chan[id].m_Output.Clear();

	m_Chan[id].m_lHost      = "";
	m_Chan[id].m_lPort      = 0;
	m_Chan[id].m_rHost      = "";
	m_Chan[id].m_rPort      = 0;
	m_Chan[id].m_ReadByte   = 0;
	m_Chan[id].m_WriteByte  = 0;
	m_Chan[id].m_ConnectTime= 0;

	((CRLoginApp *)AfxGetApp())->SetSocketIdle(&(m_Chan[id]));

	return id;
}
void Cssh::ChannelClose(int id)
{
	int n, nx;

	for ( n = nx = m_ChanNext ; n != (-1) ; ) {
		if ( n == id ) {
			if ( n == nx )
				m_ChanNext = m_Chan[n].m_Next;
			else
				m_Chan[nx].m_Next = m_Chan[n].m_Next;
			m_Chan[n].m_Next = m_ChanFree2;
			m_ChanFree2 = n;
			m_Chan[id].Close();
			break;
		}
		nx = n;
		n = m_Chan[n].m_Next;
	}

	if ( id == m_StdChan ) {
		m_StdChan = (-1);
		m_SSH2Status &= ~SSH2_STAT_HAVESTDIO;
	}

	if ( m_ChanNext == (-1) )
		SendDisconnect2(SSH2_DISCONNECT_CONNECTION_LOST, "End");
}
void Cssh::ChannelCheck(int n)
{
	CBuffer tmp;

	if ( !CHAN_OK(n) ) {
		if ( (m_Chan[n].m_Status & CHAN_LISTEN) && (m_Chan[n].m_Eof & CEOF_DEAD) ) {
			ChannelClose(n);
			LogIt("Listen Closed #%d %s:%d -> %s:%d", n, m_Chan[n].m_lHost, m_Chan[n].m_lPort, m_Chan[n].m_rHost, m_Chan[n].m_rPort);
		}
		return;
	}

	m_Chan[n].m_Eof &= ~(CEOF_IEMPY | CEOF_OEMPY);
	if ( m_Chan[n].GetSendSize() == 0 )
		m_Chan[n].m_Eof |= CEOF_IEMPY;
	if ( m_Chan[n].GetRecvSize() == 0 )
		m_Chan[n].m_Eof |= CEOF_OEMPY;

	if ( !(m_Chan[n].m_Eof & CEOF_SEOF) && !(m_Chan[n].m_Eof & CEOF_SCLOSE) ) {
		if ( (m_Chan[n].m_Eof & CEOF_ICLOSE) || ((m_Chan[n].m_Eof & CEOF_DEAD) && (m_Chan[n].m_Eof & CEOF_OEMPY)) ) {
			m_Chan[n].m_Output.Clear();
			m_Chan[n].m_Eof |= (CEOF_SEOF | CEOF_OEMPY);
			tmp.Clear();
			tmp.Put8Bit(SSH2_MSG_CHANNEL_EOF);
			tmp.Put32Bit(m_Chan[n].m_RemoteID);
			SendPacket2(&tmp);
			TRACE("Cannel #%d Send Eof\n", m_Chan[n].m_LocalID);
		}
	}

	if ( !(m_Chan[n].m_Eof & CEOF_SCLOSE) ) {
		if ( (m_Chan[n].m_Eof & CEOF_DEAD) || ((m_Chan[n].m_Eof & CEOF_IEOF) && (m_Chan[n].m_Eof & CEOF_IEMPY)) ) {
			m_Chan[n].m_Input.Clear();
			m_Chan[n].m_Eof |= (CEOF_SCLOSE | CEOF_IEMPY);
			tmp.Clear();
			tmp.Put8Bit(SSH2_MSG_CHANNEL_CLOSE);
			tmp.Put32Bit(m_Chan[n].m_RemoteID);
			SendPacket2(&tmp);
			TRACE("Cannel #%d Send Close\n", m_Chan[n].m_LocalID);
		}
	}

	if ( !(m_Chan[n].m_Eof & CEOF_ICLOSE) && !(m_Chan[n].m_Eof & CEOF_OEMPY) && m_Chan[n].m_Output.GetSize() > 0 )
		SendMsgChannelData(n);

	if ( (m_Chan[n].m_Eof & CEOF_ICLOSE) && (m_Chan[n].m_Eof & CEOF_SCLOSE) ) {
		if ( m_Chan[n].m_pFilter != NULL )
			LogIt("Closed #%d Filter", n);
		else
			LogIt("Closed #%d %s:%d -> %s:%d", n, m_Chan[n].m_lHost, m_Chan[n].m_lPort, m_Chan[n].m_rHost, m_Chan[n].m_rPort);
		m_OpenChanCount--;
		ChannelClose(n);
		TRACE("Cannel #%d Close\n", m_Chan[n].m_LocalID);
	}
}
void Cssh::ChannelAccept(int id, SOCKET hand)
{
	int i = ChannelOpen();
	CString host;
	int port;

	if ( !m_Chan[id].Accept(&(m_Chan[i]), hand) ) {
		ChannelClose(i);
		return;
	}

	CExtSocket::GetPeerName((int)m_Chan[i].m_Fd, host, &port);

	if ( (m_Chan[id].m_Status & CHAN_PROXY_SOCKS) != 0 ) {
		m_Chan[i].m_Status = CHAN_PROXY_SOCKS;
		m_Chan[i].m_lHost = host;
		m_Chan[i].m_lPort = port;
	} else {
		SendMsgChannelOpen(i, "direct-tcpip", m_Chan[id].m_rHost, m_Chan[id].m_rPort, host, port);
	}
}
void Cssh::ChannelPolling(int id)
{
	int n;
	CBuffer tmp;

	if ( (n = m_Chan[id].m_LocalComs - m_Chan[id].GetSendSize()) >= (m_Chan[id].m_LocalWind / 2) ) {
		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_WINDOW_ADJUST);
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.Put32Bit(n);
		SendPacket2(&tmp);
		m_Chan[id].m_LocalComs -= n;
		TRACE("Send Window Adjust %d\n", n);
	}

	if ( m_Chan[id].GetSendSize() <= 0 )
		ChannelCheck(id);
}
void Cssh::DecodeProxySocks(int id)
{
	int n;
	int len;
	DWORD dw;
	LPBYTE p;
	CString host[2];
	int port[2];
	struct {
		BYTE version;
		BYTE command;
		WORD dest_port;
		union {
			struct in_addr	in_addr;
			DWORD			dw_addr;
		} dest;
	} s4_req;
	BYTE tmp[257];
	struct sockaddr_in in;
#ifndef	NOIPV6
    struct sockaddr_in6 in6;
#endif

	if ( (len = m_Chan[id].m_Output.GetSize()) < 2 )
		return;

	p = m_Chan[id].m_Output.GetPtr();
	switch(p[0]) {
	case 4:
		if ( len < sizeof(s4_req) )
			break;
		for ( n = sizeof(s4_req) ; n < len ; n++ ) {
			if ( p[n] == '\0' )
				break;
		}
		if ( n++ >= len )
			break;
		s4_req.version   = m_Chan[id].m_Output.Get8Bit();
		s4_req.command   = m_Chan[id].m_Output.Get8Bit();
		s4_req.dest_port = *((WORD *)(m_Chan[id].m_Output.GetPtr())); m_Chan[id].m_Output.Consume(2);
		s4_req.dest.dw_addr = *((DWORD *)(m_Chan[id].m_Output.GetPtr())); m_Chan[id].m_Output.Consume(4);
		m_Chan[id].m_Output.Consume( n - sizeof(s4_req));

		m_Chan[id].m_Input.Put8Bit(0);
		m_Chan[id].m_Input.Put8Bit(90);
		m_Chan[id].m_Input.Put16Bit(0);
		m_Chan[id].m_Input.Put32Bit(INADDR_ANY);
		m_Chan[id].Send(m_Chan[id].m_Input.GetPtr(), m_Chan[id].m_Input.GetSize());
		m_Chan[id].m_Input.Clear();

		host[0] = inet_ntoa(s4_req.dest.in_addr);
		port[0] = ntohs(s4_req.dest_port);
		host[1] = m_Chan[id].m_lHost;
		port[1] = m_Chan[id].m_lPort;

		SendMsgChannelOpen(id, "direct-tcpip", host[0], port[0], host[1], port[1]);
		break;

	case 5:
		if ( (m_Chan[id].m_Status & CHAN_SOCKS5_AUTH) == 0 ) {
			if ( len < (p[1] + 2) )
				break;
			for ( n = 2 ; n < (p[1] + 2) ; n++ ) {
				if ( p[n] == '\0' )		// SOCKS5_NOAUTH
					break;
			}
			if ( n >= (p[1] + 2) ) {
				ChannelClose(id);
				break;
			}
			m_Chan[id].m_Output.Consume(p[1] + 2);
			len = m_Chan[id].m_Output.GetSize();

			m_Chan[id].m_Input.Put8Bit(5);
			m_Chan[id].m_Input.Put8Bit(0);
			m_Chan[id].Send(m_Chan[id].m_Input.GetPtr(), m_Chan[id].m_Input.GetSize());
			m_Chan[id].m_Input.Clear();

			m_Chan[id].m_Status |= CHAN_SOCKS5_AUTH;
		}

		if ( len < 4 )
			break;
		if ( p[1] != '\x01' || p[2] != '\0' ) {	// SOCKS5_CONNECT  ?
			ChannelClose(id);
			break;
		}
		if ( p[3] == '\x01' ) {			// SOCKS5_IPV4
			if ( len < (4 + 4 + 2) )
				break;
			memcpy(tmp, p + 4, 4);
			m_Chan[id].m_Output.Consume(4 + 4);
			memset(&in, 0, sizeof(in));
			in.sin_family = AF_INET;
			memcpy(&(in.sin_addr), tmp, 4);
			m_Chan[id].GetHostName((struct sockaddr *)&in, sizeof(in), host[0]);
		} else if ( p[3] == '\x03' ) {	// SOCKS5_DOMAIN
			if ( len < (4 + p[4] + 2) )
				break;
			memcpy(tmp, p + 5, p[4]);
			m_Chan[id].m_Output.Consume(5 + p[4]);
			tmp[p[4]] = '\0';
			host[0] = tmp;
#ifndef	NOIPV6
		} else if ( p[3] == '\x04' ) {	// SOCKS5_IPV6
			if ( len < (4 + 16 + 2) )
				break;
			memcpy(tmp, p + 4, 16);
			m_Chan[id].m_Output.Consume(4 + 16);
			memset(&in6, 0, sizeof(in6));
			in6.sin6_family = AF_INET6;
			memcpy(&(in6.sin6_addr), tmp, 16);
			m_Chan[id].GetHostName((struct sockaddr *)&in6, sizeof(in6), host[0]);
#endif
		} else {
			ChannelClose(id);
			break;
		}

		dw = *((WORD *)(m_Chan[id].m_Output.GetPtr()));
		m_Chan[id].m_Output.Consume(2);

		m_Chan[id].m_Input.Put8Bit(0x05);
		m_Chan[id].m_Input.Put8Bit(0x00);	// SOCKS5_SUCCESS
		m_Chan[id].m_Input.Put8Bit(0x00);
		m_Chan[id].m_Input.Put8Bit(0x01);	// SOCKS5_IPV4
		m_Chan[id].m_Input.Put32Bit(INADDR_ANY);
		m_Chan[id].m_Input.Put16Bit(dw);
		m_Chan[id].Send(m_Chan[id].m_Input.GetPtr(), m_Chan[id].m_Input.GetSize());
		m_Chan[id].m_Input.Clear();

		port[0] = ntohs((WORD)dw);
		host[1] = m_Chan[id].m_lHost;
		port[1] = m_Chan[id].m_lPort;

		SendMsgChannelOpen(id, "direct-tcpip", host[0], port[0], host[1], port[1]);
		break;

	default:
		ChannelClose(id);
		break;
	}
}
int Cssh::MatchList(LPCSTR client, LPCSTR server, CString &str)
{
	int n;
	CString tmp;
	CStringBinary idx;

	while ( *server != '\0' ) {
		while ( *server != '\0' && strchr(" ,", *server) != NULL )
			server++;
		tmp = "";
		while ( *server != '\0' && strchr(" ,", *server) == NULL )
			tmp += *(server++);
		if ( !tmp.IsEmpty() )
			idx.Add(tmp);
	}

	while ( *client != '\0' ) {
		while ( *client != '\0' && strchr(" ,", *client) != NULL )
			client++;
		tmp = "";
		while ( *client != '\0' && strchr(" ,", *client) == NULL )
			tmp += *(client++);
		if ( !tmp.IsEmpty() && idx.Find(tmp) != NULL ) {
			str = tmp;
			return TRUE;
		}
	}

	return FALSE;
}
void Cssh::PortForward()
{
	int n, i, a = 0;
	CString str;
	CStringArrayExt tmp;

	for ( i = 0 ; i < m_pDocument->m_ParamTab.m_PortFwd.GetSize() ; i++ ) {
		m_pDocument->m_ParamTab.m_PortFwd.GetArray(i, tmp);
		if ( tmp.GetSize() < 5 )
			continue;

		switch(tmp.GetVal(4)) {
		case PFD_LOCAL:
			n = ChannelOpen();
			m_Chan[n].m_Status = CHAN_LISTEN;
			m_Chan[n].m_TypeName = "tcpip-listen";
			if ( !m_Chan[n].CreateListen(tmp[0], GetPortNum(tmp[1]), tmp[2], GetPortNum(tmp[3])) ) {
				str.Format("Port Forward Error %s:%s->%s:%s", tmp[0], tmp[1], tmp[2], tmp[3]);
				AfxMessageBox(str);
				ChannelClose(n);
			} else {
				LogIt("Local Listen %s:%s", tmp[0], tmp[1]);
				a++;
			}
			break;

		case PFD_SOCKS:
			n = ChannelOpen();
			m_Chan[n].m_Status = CHAN_LISTEN | CHAN_PROXY_SOCKS;
			m_Chan[n].m_TypeName = "socks-listen";
			if ( !m_Chan[n].CreateListen(tmp[0], GetPortNum(tmp[1]), tmp[2], GetPortNum(tmp[3])) ) {
				str.Format("Socks Listen Error %s:%s->%s:%s", tmp[0], tmp[1], tmp[2], tmp[3]);
				AfxMessageBox(str);
				ChannelClose(n);
			} else {
				LogIt("Local Socks %s:%s", tmp[0], tmp[1]);
				a++;
			}
			break;

		case PFD_REMOTE:
			n = (int)m_Permit.GetSize();
			m_Permit.SetSize(n + 1);
			m_Permit[n].m_lHost = tmp[2];
			m_Permit[n].m_lPort = GetPortNum(tmp[3]);
			m_Permit[n].m_rHost = tmp[0];
			m_Permit[n].m_rPort = GetPortNum(tmp[1]);
			SendMsgGlobalRequest(n, "tcpip-forward", tmp[0], GetPortNum(tmp[1]));
			LogIt("Remote Listen %s:%s", tmp[0], tmp[1]);
			a++;
			break;
		}
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) && a == 0 )
		AfxMessageBox(IDE_PORTFWORDERROR);
}

void Cssh::OpenSFtpChannel()
{
	int id = SendMsgChannelOpen((-1), "session");
	CSFtpFilter *pFilter = new CSFtpFilter;
	m_Chan[id].m_pFilter = pFilter;
	pFilter->m_pSFtp = new CSFtp(NULL);
	pFilter->m_pSFtp->m_pSSh = this;
	pFilter->m_pSFtp->m_pChan = &(m_Chan[id]);
	pFilter->m_pSFtp->m_HostKanjiSet = m_pDocument->m_TextRam.m_SendCharSet[m_pDocument->m_TextRam.m_KanjiMode];
	pFilter->m_pSFtp->Create(IDD_SFTPDLG, CWnd::GetDesktopWindow());
	pFilter->m_pSFtp->ShowWindow(SW_SHOW);
}
void Cssh::OpenRcpUpload(LPCSTR file)
{
	if ( file != NULL ) {
		m_RcpCmd.m_pSsh = this;
		m_RcpCmd.m_FileList.AddTail(file);
		m_RcpCmd.Destroy();
	} else {
		m_RcpCmd.m_pSsh = this;
		int id = SendMsgChannelOpen((-1), "session");
		m_Chan[id].m_pFilter = &m_RcpCmd;
		m_RcpCmd.m_pChan = &(m_Chan[id]);
		m_RcpCmd.m_pNext = m_pListFilter;
		m_pListFilter = &m_RcpCmd;
	}
}
void Cssh::SendMsgKexInit()
{
	CBuffer tmp;

	if ( m_MyPeer.GetSize() == 0 || (m_SSH2Status & SSH2_STAT_SENTKEXINIT) != 0 )
		return;
	tmp.Put8Bit(SSH2_MSG_KEXINIT);
	tmp.Apend(m_MyPeer.GetPtr(), m_MyPeer.GetSize());
	SendPacket2(&tmp);
	m_SSH2Status |= SSH2_STAT_SENTKEXINIT;
	m_SendPackLen = 0;
}
void Cssh::SendMsgKexDhInit()
{
	CBuffer tmp;

	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);
	m_SaveDh = (m_DhMode == DHMODE_GROUP_1 ? dh_new_group1() : dh_new_group14());
	dh_gen_key(m_SaveDh, m_NeedKeyLen * 8);

	tmp.Put8Bit(SSH2_MSG_KEXDH_INIT);
	tmp.PutBIGNUM2(m_SaveDh->pub_key);
	SendPacket2(&tmp);
}
void Cssh::SendMsgKexDhGexRequest()
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_KEX_DH_GEX_REQUEST);
	tmp.Put32Bit(1024);
	tmp.Put32Bit(dh_estimate(m_NeedKeyLen * 8));
	tmp.Put32Bit(8192);
	SendPacket2(&tmp);
}
int Cssh::SendMsgKexEcdhInit()
{
	CBuffer tmp;

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
void Cssh::SendMsgNewKeys()
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_NEWKEYS);
	SendPacket2(&tmp);

	if ( m_EncCip.Init(m_VProp[PROP_ENC_ALGS_CTOS], MODE_ENC, m_VKey[2], (-1), m_VKey[0]) ||
		 m_EncMac.Init(m_VProp[PROP_MAC_ALGS_CTOS], m_VKey[4], (-1)) ||
		 m_EncCmp.Init(m_VProp[PROP_COMP_ALGS_CTOS], MODE_ENC, COMPLEVEL) )
		Close();
}
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
	CBuffer tmp, blob, sig;
	CString wrk;

	tmp.PutBuf(m_SessionId, m_SessionIdLen);
	skip = tmp.GetSize();
	tmp.Put8Bit(SSH2_MSG_USERAUTH_REQUEST);
	tmp.PutStr(m_pDocument->m_ServerEntry.m_UserName);
	tmp.PutStr("ssh-connection");

	if ( str == NULL ) {
		m_AuthStat = AST_START;
		str = "none";
	}
	m_AuthMeta = str;

	while ( m_AuthStat != AST_DONE ) {
		if ( m_AuthStat == AST_START ) {
			tmp.PutStr("none");
			m_IdKeyPos = 0;
			m_AuthStat = m_AuthReqTab[AST_START];
			m_AuthMode = AUTH_MODE_NONE;

		} else if ( m_AuthStat == AST_PUBKEY_TRY ) {
			m_AuthMode = AUTH_MODE_NONE;
			while ( strstr(str, "publickey") != NULL && m_IdKeyPos < m_IdKeyTab.GetSize() ) {
				m_IdKey = m_IdKeyTab[m_IdKeyPos++];
				if ( m_IdKey.m_Type == IDKEY_RSA1 )
					m_IdKey.m_Type = IDKEY_RSA2;
				m_IdKey.SetBlob(&blob);
				len = tmp.GetSize();
				tmp.PutStr("publickey");
				tmp.Put8Bit(1);
				tmp.PutStr(m_IdKey.GetName());
				tmp.PutBuf(blob.GetPtr(), blob.GetSize());
				len = tmp.GetSize() - len;
				if ( m_IdKey.Sign(&sig, tmp.GetPtr(), tmp.GetSize()) ) {
					tmp.PutBuf(sig.GetPtr(), sig.GetSize());
					m_AuthMode = AUTH_MODE_PUBLICKEY;
					break;
				}
				tmp.ConsumeEnd(len);
			}
			if ( m_AuthMode == AUTH_MODE_NONE ) {
				m_IdKeyPos = 0;
				m_AuthStat = m_AuthReqTab[AST_PUBKEY_TRY];
				continue;
			}

		} else if ( m_AuthStat == AST_HOST_TRY ) {
			m_AuthMode = AUTH_MODE_NONE;
			while ( strstr(str, "hostbased") != NULL && m_IdKeyPos < m_IdKeyTab.GetSize() ) {
				m_IdKey = m_IdKeyTab[m_IdKeyPos++];
				m_IdKey.SetBlob(&blob);
				len = tmp.GetSize();
				tmp.PutStr("hostbased");
				tmp.PutStr(m_IdKey.GetName());
				tmp.PutBuf(blob.GetPtr(), blob.GetSize());
				GetSockName(m_Fd, wrk, &len);
				tmp.PutStr(wrk);		// client ip address
				m_IdKey.GetUserHostName(wrk);
				tmp.PutStr(wrk);		// client user name;
				if ( m_IdKey.Sign(&sig, tmp.GetPtr(), tmp.GetSize()) ) {
					tmp.PutBuf(sig.GetPtr(), sig.GetSize());
					m_AuthMode = AUTH_MODE_HOSTBASED;
					break;
				}
				tmp.ConsumeEnd(len);
			}
			if ( m_AuthMode == AUTH_MODE_NONE ) {
				m_IdKeyPos = 0;
				m_AuthStat = m_AuthReqTab[AST_HOST_TRY];
				continue;
			}

		} else if ( m_AuthStat == AST_PASS_TRY ) {
			m_IdKeyPos = 0;
			m_AuthStat = m_AuthReqTab[AST_PASS_TRY];
			if ( strstr(str, "password") == NULL || m_pDocument->m_ServerEntry.m_PassName.IsEmpty() )
				continue;
			tmp.PutStr("password");
			tmp.Put8Bit(0);
			tmp.PutStr(m_pDocument->m_ServerEntry.m_PassName);
			m_AuthMode = AUTH_MODE_PASSWORD;

		} else if ( m_AuthStat == AST_KEYB_TRY ) {
			m_IdKeyPos = 0;
			m_AuthStat = m_AuthReqTab[AST_KEYB_TRY];
			if ( strstr(str, "keyboard-interactive") == NULL )
				continue;
			tmp.PutStr("keyboard-interactive");
			tmp.PutStr("");		// LANG
			tmp.PutStr("");		// DEV
			m_AuthMode = AUTH_MODE_KEYBOARD;
		}

		tmp.Consume(skip);
		SendPacket2(&tmp);
		return TRUE;
	}

	m_AuthStat = AST_DONE;
	m_AuthMode = AUTH_MODE_NONE;
	return FALSE;
}
int Cssh::SendMsgChannelOpen(int n, LPCSTR type, LPCSTR lhost, int lport, LPCSTR rhost, int rport)
{
	CBuffer tmp;

	if ( n < 0 )
		n = ChannelOpen();

	m_Chan[n].m_Status     = CHAN_OPEN_LOCAL;
	m_Chan[n].m_TypeName   = type;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN);
	tmp.PutStr(type);
	tmp.Put32Bit(n);
	tmp.Put32Bit(m_Chan[n].m_LocalWind);
	tmp.Put32Bit(m_Chan[n].m_LocalPacks);
	if ( lhost != NULL ) {
		tmp.PutStr(lhost);
		tmp.Put32Bit(lport);
		tmp.PutStr(rhost);
		tmp.Put32Bit(rport);
		m_Chan[n].m_lHost = lhost;
		m_Chan[n].m_lPort = lport;
		m_Chan[n].m_rHost = rhost;
		m_Chan[n].m_rPort = rport;
	}
	SendPacket2(&tmp);

	TRACE("Channel Open %d(%d)\n", m_Chan[n].m_LocalWind, m_Chan[n].m_LocalPacks);

	return n;
}
void Cssh::SendMsgChannelOpenConfirmation(int id)
{
	CBuffer tmp;

	m_Chan[id].m_Status |= CHAN_OPEN_LOCAL;
	m_Chan[id].m_ConnectTime = CTime::GetCurrentTime();
	m_OpenChanCount++;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_CONFIRMATION);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.Put32Bit(m_Chan[id].m_LocalID);
	tmp.Put32Bit(m_Chan[id].m_LocalWind);
	tmp.Put32Bit(m_Chan[id].m_LocalPacks);
	SendPacket2(&tmp);

	TRACE("Channel OpenConf %d(%d)\n", m_Chan[id].m_LocalWind, m_Chan[id].m_LocalPacks);

	if ( m_Chan[id].m_pFilter != NULL )
		LogIt("Open Filter #%d", id);
	else
		LogIt("Open Connect #%d %s:%d -> %s:%d", id, m_Chan[id].m_lHost, m_Chan[id].m_lPort, m_Chan[id].m_rHost, m_Chan[id].m_rPort);
}
void Cssh::SendMsgChannelOpenFailure(int id)
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_FAILURE);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
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

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) || (m_SSH2Status & SSH2_STAT_SENTKEXINIT) != 0 )
		return;

	if ( m_SendPackLen >= MAX_PACKETLEN || m_RecvPackLen >= MAX_PACKETLEN ) {
		if ( GetSendSize() == 0 )
			SendMsgKexInit();
		return;
	}

	while ( (len = m_Chan[id].m_Output.GetSize()) > 0 ) {
		if ( len > m_Chan[id].m_RemoteWind )
			len = m_Chan[id].m_RemoteWind;

		if ( len > m_Chan[id].m_RemoteMax )
			len = m_Chan[id].m_RemoteMax;

		if ( len <= 0 )
			return;

		tmp.Clear();
		tmp.Put8Bit(SSH2_MSG_CHANNEL_DATA);
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.PutBuf(m_Chan[id].m_Output.GetPtr(), len);
		m_Chan[id].m_Output.Consume(len);
		m_Chan[id].m_RemoteWind -= len;
		SendPacket2(&tmp);
	}

	ChannelCheck(id);
}
void Cssh::SendMsgChannelRequesstShell(int id)
{
	int n;
	char *p;
	CBuffer tmp, tmode;
	CString str;
	CStringIndex env;
	static const struct _dummy_ttymode {
		BYTE	opcode;
		DWORD	param;
	} ttymode[] = {
	//	VINTR		VQUIT		VERASE		VKILL		VEOF		VEOL		VEOL2		VSTART
		{ 1,3 },	{ 2,28 },	{ 3,8 },	{ 4,21 },	{ 5,4 }, 	{ 6,255 },	{ 7,255 },	{ 8,17 },
	//	VSTOP		VSUSP		VDSUSP		VREPRINT	VWERASE		VLNEXT		VSTATUS		VDISCARD
		{ 9,19 },	{ 10,26 },	{ 11,25 },	{ 12,18 },	{ 13,23 },	{ 14,22 },	{ 17,20 },	{ 18,15 },
	//	IGNPAR		PARMRK		INPCK		ISTRIP		INLCR		IGNCR		ICRNL		IXON
		{ 30,0 },	{ 31,0 },	{ 32,0 },	{ 33,0 },	{ 34,0 },	{ 35,0 },	{ 36,1 },	{ 38,1 },
	//	IXANY		IXOFF		IMAXBEL
		{ 39,1 },	{ 40,0 },	{ 41,1 },
	//	ISIG		ICANON		ECHO		ECHOE		ECHOK		ECHONL		NOFLSH		TOSTOP
		{ 50,1 },	{ 51,1 },	{ 53,1 },	{ 54,1 },	{ 55,0 },	{ 56,0 },	{ 57,0 },	{ 58,0 },
	//	IEXTEN		ECHOCTL		ECHOKE		PENDIN
		{ 59,1 },	{ 60,1 },	{ 61,1 },	{ 62,1 },
	//	OPOST		OLCUC		OCRNL		ONOCR		ONLRET
		{ 70,1 },	{ 72,1 },	{ 73,0 },	{ 74,0 },	{ 75,0 },
	//	CS7			CS8			PARENB		PARODD
		{ 90,1 },	{ 91,1 },	{ 92,0 },	{ 93,0 },
	//	OSPEED			ISPEED
		{ 129,9600 },	{ 128,9600 },
		{ 0, 0 }
	};

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHAGENT) ) {
		tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.PutStr("auth-agent-req@openssh.com");
		tmp.Put8Bit(0);
		SendPacket2(&tmp);
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHX11PF) ) {
		if ( (p = getenv("DISPLAY")) == NULL ) {
			if ( m_pDocument->m_ParamTab.m_XDisplay.IsEmpty() )
				p = ":0";
			else
				p = (char *)(LPCSTR)m_pDocument->m_ParamTab.m_XDisplay;
		}

		while ( *p != '\0' && *p != ':' )
			str += *(p++);

		if ( str.IsEmpty() || str.CompareNoCase("unix") == 0 )
			str = "127.0.0.1";

		if ( *(p++) == ':' ) { // && CExtSocket::SokcetCheck(str, 6000 + atoi(p)) ) {
			n = 0;	// screen number
			while ( *p != '\0' && *p != '.' )
				p++;
			if ( *p == '.' )
				n = _tstoi(++p);
			str.Format("%04x%04x%04x%04x", rand(), rand(), rand(), rand());
			tmp.Clear();
			tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
			tmp.Put32Bit(m_Chan[id].m_RemoteID);
			tmp.PutStr("x11-req");
			tmp.Put8Bit(0);
			tmp.Put8Bit(0);		// XXX bool single connection
			tmp.PutStr("MIT-MAGIC-COOKIE-1");
			tmp.PutStr(str);
			tmp.Put32Bit(n);	// screen number
			SendPacket2(&tmp);
		}
	}

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("pty-req");
	tmp.Put8Bit(1);
	tmp.PutStr(m_pDocument->m_ServerEntry.m_TermName);
	tmp.Put32Bit(m_pDocument->m_TextRam.m_Cols);
	tmp.Put32Bit(m_pDocument->m_TextRam.m_Lines);
	tmp.Put32Bit(0);
	tmp.Put32Bit(0);

	tmode.Clear();
	for ( n = 0 ; ttymode[n].opcode != 0 ; n++ ) {
		tmode.Put8Bit(ttymode[n].opcode);
		tmode.Put32Bit(ttymode[n].param);
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
		tmp.Put32Bit(m_Chan[id].m_RemoteID);
		tmp.PutStr("env");
		tmp.Put8Bit(0);
		tmp.PutStr(env[n].m_nIndex);
		tmp.PutStr(env[n].m_String);
		SendPacket2(&tmp);
	}

	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("shell");
	tmp.Put8Bit(1);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(CHAN_REQ_SHELL);
}
void Cssh::SendMsgChannelRequesstSubSystem(int id)
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("subsystem");
	tmp.Put8Bit(1);
	tmp.PutStr("sftp");
	SendPacket2(&tmp);
	m_ChnReqMap.Add(CHAN_REQ_SFTP);
}
void Cssh::SendMsgChannelRequesstExec(int id)
{
	CBuffer tmp;

	tmp.Put8Bit(SSH2_MSG_CHANNEL_REQUEST);
	tmp.Put32Bit(m_Chan[id].m_RemoteID);
	tmp.PutStr("exec");
	tmp.Put8Bit(1);
	tmp.PutStr(m_Chan[id].m_pFilter->m_Cmd);
	SendPacket2(&tmp);
	m_ChnReqMap.Add(CHAN_REQ_EXEC);
}
void Cssh::SendMsgGlobalRequest(int num, LPCSTR str, LPCSTR rhost, int rport)
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_GLOBAL_REQUEST);
	tmp.PutStr(str);
	tmp.Put8Bit(1);
	if ( rhost != NULL ) {
		tmp.PutStr(rhost);
		tmp.Put32Bit(rport);
	}
	SendPacket2(&tmp);
	m_GlbReqMap.Add((WORD)num);
}
void Cssh::SendMsgKeepAlive()
{
	CBuffer tmp;
	tmp.Put8Bit(SSH2_MSG_GLOBAL_REQUEST);
	tmp.PutStr("keepalive@openssh.com");
	tmp.Put8Bit(1);
	SendPacket2(&tmp);
	m_GlbReqMap.Add(0xFFFF);
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
void Cssh::SendPacket2(CBuffer *bp)
{
	int n = bp->GetSize() + 128;
	int pad, i;
	CBuffer tmp(n);
	CBuffer enc(n);
	static int padflag = FALSE;
	static BYTE padimage[64];

	tmp.PutSpc(4 + 1);		// Size + Pad Era
	m_EncCmp.Compress(bp->GetPtr(), bp->GetSize(), &tmp);

	if ( (pad = m_EncCip.GetBlockSize() - (tmp.GetSize() % m_EncCip.GetBlockSize())) < 4 )
		pad += m_EncCip.GetBlockSize();

	tmp.SET32BIT(tmp.GetPos(0), tmp.GetSize() - 4 + pad);
	tmp.SET8BIT(tmp.GetPos(4), pad);

	if ( !padflag ) {
		for ( n = 0 ; n < 64 ; n += 2 ) {
			i = rand();
			padimage[n + 1] = (BYTE)(i >> 8);
			padimage[n + 0] = (BYTE)(i >> 0);
		}
		padflag = TRUE;
	}

	for ( n = 0 ; n < pad ; n += 64 )
		tmp.Apend(padimage, ((pad - n) >= 64 ? 64 : (pad - n)));

	m_EncCip.Cipher(tmp.GetPtr(), tmp.GetSize(), &enc);

	if ( m_EncMac.GetBlockSize() > 0 )
		m_EncMac.Compute(m_SendPackSeq, tmp.GetPtr(), tmp.GetSize(), &enc);

	CExtSocket::Send(enc.GetPtr(), enc.GetSize());

	m_SendPackSeq++;
	m_SendPackLen++;
}

int Cssh::SSH2MsgKexInit(CBuffer *bp)
{
	int n;
	static const struct {
		int	mode;
		char *name;
	} kextab[] = {
		{ DHMODE_ECDH_S2_N256,	"ecdh-sha2-nistp256",	},
		{ DHMODE_ECDH_S2_N384,	"ecdh-sha2-nistp384"	},
		{ DHMODE_ECDH_S2_N521,	"ecdh-sha2-nistp521"	},
		{ DHMODE_GROUP_GEX256,	"diffie-hellman-group-exchange-sha256",	},
		{ DHMODE_GROUP_GEX,		"diffie-hellman-group-exchange-sha1",	},
		{ DHMODE_GROUP_14,		"diffie-hellman-group14-sha1",			},
		{ DHMODE_GROUP_1,		"diffie-hellman-group1-sha1",			},
		{ 0,					NULL									},
	};

	m_HisPeer = *bp;
	for ( n = 0 ; n < 16 ; n++ )
		m_Cookie[n] = bp->Get8Bit();
	for ( n = 0 ; n < 10 ; n++ )
		bp->GetStr(m_SProp[n]);
	bp->Get8Bit();
	bp->Get32Bit();

	m_pDocument->m_ParamTab.GetProp(9,  m_CProp[PROP_KEX_ALGS]);			// PROPOSAL_KEX_ALGS
	m_pDocument->m_ParamTab.GetProp(10, m_CProp[PROP_HOST_KEY_ALGS]);		// PROPOSAL_SERVER_HOST_KEY_ALGS

	m_pDocument->m_ParamTab.GetProp(6,  m_CProp[PROP_ENC_ALGS_CTOS], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFENC));	// PROPOSAL_ENC_ALGS_CTOS
	m_pDocument->m_ParamTab.GetProp(3,  m_CProp[PROP_ENC_ALGS_STOC], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFENC));	// PROPOSAL_ENC_ALGS_STOC
	m_pDocument->m_ParamTab.GetProp(7,  m_CProp[PROP_MAC_ALGS_CTOS], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFMAC));	// PROPOSAL_MAC_ALGS_CTOS
	m_pDocument->m_ParamTab.GetProp(4,  m_CProp[PROP_MAC_ALGS_STOC], m_pDocument->m_TextRam.IsOptEnable(TO_SSHSFMAC));	// PROPOSAL_MAC_ALGS_STOC

	m_pDocument->m_ParamTab.GetProp(8,  m_CProp[PROP_COMP_ALGS_CTOS]);			// PROPOSAL_COMP_ALGS_CTOS
	m_pDocument->m_ParamTab.GetProp(5,  m_CProp[PROP_COMP_ALGS_STOC]);			// PROPOSAL_COMP_ALGS_STOC

	m_CProp[PROP_LANG_CTOS] = m_VProp[PROP_LANG_CTOS] = "";						// PROPOSAL_LANG_CTOS,
	m_CProp[PROP_LANG_STOC] = m_VProp[PROP_LANG_STOC] = "";						// PROPOSAL_LANG_STOC,

	for ( n = 0 ; n < 8 ; n++ ) {
		if ( !MatchList(m_CProp[n], m_SProp[n], m_VProp[n]) )
			return TRUE;
	}

	// PROPOSAL_KEX_ALGS
	for ( n = 0 ; kextab[n].name != NULL ; n++ ) {
		if ( m_VProp[PROP_KEX_ALGS].Compare(kextab[n].name) == 0 ) {
			m_DhMode = kextab[n].mode;
			break;
		}
	}
	if ( kextab[n].name == NULL )
		return TRUE;

	if ( (m_SSH2Status & SSH2_STAT_SENTKEXINIT) == 0 ) {
		m_MyPeer.Clear();
		for ( n = 0 ; n < 16 ; n += 2 )
			m_MyPeer.Put16Bit(rand());
		for ( n = 0 ; n < 10 ; n++ )
			m_MyPeer.PutStr(m_VProp[n]);
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
	return FALSE;
}
int Cssh::SSH2MsgKexDhReply(CBuffer *bp)
{
	int n;
	int ret = TRUE;
	CBuffer tmp, blob, sig;
	BIGNUM *spub = NULL, *ssec = NULL;
	LPBYTE p;
	int hashlen;
	BYTE hash[EVP_MAX_MD_SIZE];

	bp->GetBuf(&tmp);
	blob = tmp;

	if ( !m_HostKey.GetBlob(&tmp) )
		return TRUE;

	if ( !m_HostKey.HostVerify(m_HostName) )
		return TRUE;

	if ( (spub = BN_new()) == NULL || (ssec = BN_new()) == NULL )
		goto ENDRET;

	bp->GetBIGNUM2(spub);
	bp->GetBuf(&sig);

	if ( !dh_pub_is_valid(m_SaveDh, spub) )
		goto ENDRET;

	tmp.Clear();
	p = tmp.PutSpc(DH_size(m_SaveDh));
	n = DH_compute_key(p, spub, m_SaveDh);
	BN_bin2bn(p, n, ssec);

	hashlen = kex_dh_hash(hash,
		m_ClientVerStr, m_ServerVerStr,
		m_MyPeer.GetPtr(), m_MyPeer.GetSize(),
		m_HisPeer.GetPtr(), m_HisPeer.GetSize(),
		blob.GetPtr(), blob.GetSize(), m_SaveDh->pub_key,
		spub, ssec,
		EVP_sha1());

	if ( m_SessionIdLen == 0 ) {
		ASSERT(hashlen <= 64);
		m_SessionIdLen = hashlen;
		memcpy(m_SessionId, hash, hashlen);
	}

	if ( !m_HostKey.Verify(&sig, hash, hashlen) )
		goto ENDRET;

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
		m_VKey[n] = derive_key('A' + n, m_NeedKeyLen, hash, hashlen, ssec, m_SessionId, m_SessionIdLen, EVP_sha1());
	}

	ret = FALSE;

ENDRET:
	if ( spub != NULL )
		BN_clear_free(spub);
	if ( ssec != NULL )
		BN_clear_free(ssec);

	return ret;
}
int Cssh::SSH2MsgKexDhGexGroup(CBuffer *bp)
{
	CBuffer tmp;

	if ( m_SaveDh != NULL )
		DH_free(m_SaveDh);

	if ( (m_SaveDh = DH_new()) == NULL )
		return TRUE;
	if ( (m_SaveDh->p = BN_new()) == NULL )
		return TRUE;
	if ( (m_SaveDh->g = BN_new()) == NULL )
		return TRUE;

	if ( !bp->GetBIGNUM2(m_SaveDh->p) )
		return TRUE;
	if ( !bp->GetBIGNUM2(m_SaveDh->g) )
		return TRUE;

	dh_gen_key(m_SaveDh, m_NeedKeyLen * 8);

	tmp.Put8Bit(SSH2_MSG_KEX_DH_GEX_INIT);
	tmp.PutBIGNUM2(m_SaveDh->pub_key);
	SendPacket2(&tmp);

	return FALSE;
}
int Cssh::SSH2MsgKexDhGexReply(CBuffer *bp)
{
	int n;
	int ret = TRUE;
	CBuffer tmp, blob, sig;
	BIGNUM *spub = NULL, *ssec = NULL;
	LPBYTE p;
	const EVP_MD *evp_md;
	int hashlen;
	BYTE hash[EVP_MAX_MD_SIZE];

	bp->GetBuf(&tmp);
	blob = tmp;

	if ( !m_HostKey.GetBlob(&tmp) )
		return TRUE;

	if ( !m_HostKey.HostVerify(m_HostName) )
		return TRUE;

	if ( (spub = BN_new()) == NULL || (ssec = BN_new()) == NULL )
		goto ENDRET;

	bp->GetBIGNUM2(spub);
	bp->GetBuf(&sig);

	if ( !dh_pub_is_valid(m_SaveDh, spub) )
		goto ENDRET;

	tmp.Clear();
	p = tmp.PutSpc(DH_size(m_SaveDh));
	n = DH_compute_key(p, spub, m_SaveDh);
	BN_bin2bn(p, n, ssec);

	evp_md = (m_DhMode == DHMODE_GROUP_GEX ? EVP_sha1() : EVP_sha256());

	hashlen = kex_gex_hash(hash,
		m_ClientVerStr, m_ServerVerStr,
		m_MyPeer.GetPtr(), m_MyPeer.GetSize(),
		m_HisPeer.GetPtr(), m_HisPeer.GetSize(),
		blob.GetPtr(), blob.GetSize(),
		1024, dh_estimate(m_NeedKeyLen * 8), 8192,
		m_SaveDh->p, m_SaveDh->g, m_SaveDh->pub_key,
		spub, ssec, evp_md);

	if ( m_SessionIdLen == 0 ) {
		ASSERT(hashlen <= 64);
		m_SessionIdLen = hashlen;
		memcpy(m_SessionId, hash, hashlen);
	}

	if ( !m_HostKey.Verify(&sig, hash, hashlen) )
		goto ENDRET;

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
		m_VKey[n] = derive_key('A' + n, m_NeedKeyLen, hash, hashlen, ssec, m_SessionId, m_SessionIdLen, evp_md);
	}

	ret = FALSE;

ENDRET:
	if ( spub != NULL )
		BN_clear_free(spub);
	if ( ssec != NULL )
		BN_clear_free(ssec);

	return ret;
}
int Cssh::SSH2MsgKexEcdhReply(CBuffer *bp)
{
	int n;
	int ret = TRUE;
	CBuffer tmp, blob, sig;
	EC_POINT *server_public = NULL;
	BIGNUM *shared_secret = NULL;
	int klen;
	LPBYTE kbuf = NULL;
	const EVP_MD *evp_md;
	int hashlen;
	BYTE hash[EVP_MAX_MD_SIZE];

	bp->GetBuf(&tmp);
	blob = tmp;

	if ( !m_HostKey.GetBlob(&tmp) )
		return TRUE;

	if ( !m_HostKey.HostVerify(m_HostName) )
		return TRUE;

	if ( (server_public = EC_POINT_new(m_EcdhGroup)) == NULL )
		goto ENDRET;

	bp->GetEcPoint(m_EcdhGroup, server_public);
	bp->GetBuf(&sig);

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

	hashlen = kex_ecdh_hash(hash,
	    m_ClientVerStr, m_ServerVerStr,
		m_MyPeer.GetPtr(), m_MyPeer.GetSize(),
		m_HisPeer.GetPtr(), m_HisPeer.GetSize(),
		blob.GetPtr(), blob.GetSize(),
	    m_EcdhGroup, EC_KEY_get0_public_key(m_EcdhClientKey),
	    server_public, shared_secret,
		evp_md);

	if ( m_SessionIdLen == 0 ) {
		ASSERT(hashlen <= 64);
		m_SessionIdLen = hashlen;
		memcpy(m_SessionId, hash, hashlen);
	}

	if ( !m_HostKey.Verify(&sig, hash, hashlen) )
		goto ENDRET;

	for ( n = 0 ; n < 6 ; n++ ) {
		if ( m_VKey[n] != NULL )
			free(m_VKey[n]);
		m_VKey[n] = derive_key('A' + n, m_NeedKeyLen, hash, hashlen, shared_secret, m_SessionId, m_SessionIdLen, evp_md);
	}

	EC_KEY_free(m_EcdhClientKey);
	m_EcdhClientKey = NULL;

	ret = FALSE;

ENDRET:
	if ( kbuf != NULL )
		delete kbuf;
	if ( shared_secret != NULL )
		BN_clear_free(shared_secret);
	if ( server_public != NULL )
		EC_POINT_clear_free(server_public);

	return ret;
}
int Cssh::SSH2MsgNewKeys(CBuffer *bp)
{
	if ( m_DecCip.Init(m_VProp[PROP_ENC_ALGS_STOC], MODE_DEC, m_VKey[3], (-1), m_VKey[1]) ||
		 m_DecMac.Init(m_VProp[PROP_MAC_ALGS_STOC], m_VKey[5], (-1)) ||
		 m_DecCmp.Init(m_VProp[PROP_COMP_ALGS_STOC], MODE_DEC, COMPLEVEL) )
		return TRUE;

	CString str, ats[3];

	ats[0] = m_EncCmp.GetTitle();
	if ( ats[0].Compare(m_DecCmp.GetTitle()) != 0 ) {
		ats[0] += _T("/");
		ats[0] += m_DecCmp.GetTitle();
	}
	ats[1] = m_EncCip.GetTitle();
	if ( ats[1].Compare(m_DecCip.GetTitle()) != 0 ) {
		ats[1] += _T("/");
		ats[1] += m_DecCip.GetTitle();
	}
	ats[2] = m_EncMac.GetTitle();
	if ( ats[2].Compare(m_DecMac.GetTitle()) != 0 ) {
		ats[2] += _T("/");
		ats[2] += m_DecMac.GetTitle();
	}
	str.Format(_T("%s+%s+%s"), ats[0], ats[1], ats[2]);
	m_pDocument->SetStatus(str);

	return FALSE;
}
int Cssh::SSH2MsgUserAuthPkOk(CBuffer *bp)
{
	int n;
	CString name;
	CBuffer blob;
	CIdKey ReqKey;
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();

	bp->GetStr(name);
	bp->GetBuf(&blob);

	if ( !ReqKey.GetBlob(&blob) )
		return FALSE;

	for ( n = 0 ; n < m_IdKeyTab.GetSize() ; n++ ) {
		if ( m_IdKeyTab[n].m_Type != ReqKey.m_Type )
			continue;
		if ( ReqKey.Compere(&(m_IdKeyTab[n])) == 0 ) {
			m_IdKeyPos = n;
			m_AuthStat = AST_PUBKEY_TRY;
			return SendMsgUserAuthRequest("publickey");
		}
	}

	return FALSE;
}
int Cssh::SSH2MsgUserAuthPasswdChangeReq(CBuffer *bp)
{
	CBuffer tmp;
	CString info, lang, pass;
	CPassDlg dlg;

	bp->GetStr(info);
	bp->GetStr(lang);

	tmp.Put8Bit(SSH2_MSG_USERAUTH_REQUEST);
	tmp.PutStr(m_pDocument->m_ServerEntry.m_UserName);
	tmp.PutStr("ssh-connection");
	tmp.PutStr("password");
	tmp.Put8Bit(1);

	dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
	dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
	dlg.m_Prompt   = "Enter Old Password";
	dlg.m_PassName = "";
	if ( dlg.DoModal() != IDOK )
		return FALSE;
	tmp.PutStr(dlg.m_PassName);

	info = "Enter New Password";
	while ( pass.IsEmpty() ) {
		dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
		dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
		dlg.m_Prompt   = info;
		dlg.m_PassName = "";
		if ( dlg.DoModal() != IDOK )
			return FALSE;
		pass = dlg.m_PassName;

		dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
		dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
		dlg.m_Prompt   = "Retype New Password";
		dlg.m_PassName = "";
		if ( dlg.DoModal() != IDOK )
			return FALSE;
		if ( pass.Compare(dlg.m_PassName) != 0 ) {
			info = "Mismatch ReEntry New Password";
			pass.Empty();
		}
	}
	tmp.PutStr(pass);

	SendPacket2(&tmp);
	return TRUE;
}
int Cssh::SSH2MsgUserAuthInfoRequest(CBuffer *bp)
{
	int n, e, max;
	CBuffer tmp;
	CString name, inst, lang, prom;
	CPassDlg dlg;

	bp->GetStr(name);
	bp->GetStr(inst);
	bp->GetStr(lang);
	max = bp->Get32Bit();

	tmp.Put8Bit(SSH2_MSG_USERAUTH_INFO_RESPONSE);
	tmp.Put32Bit(max);

	for ( n = 0 ; n < max ; n++ ) {
		bp->GetStr(prom);
		e = bp->Get8Bit();
		dlg.m_HostAddr = m_pDocument->m_ServerEntry.m_HostName;
		dlg.m_UserName = m_pDocument->m_ServerEntry.m_UserName;
		dlg.m_Prompt   = prom;
		dlg.m_PassName = "";
		if ( dlg.DoModal() != IDOK )
			return FALSE;
		tmp.PutStr(dlg.m_PassName);
	}

	SendPacket2(&tmp);
	return TRUE;
}
int Cssh::SSH2MsgChannelOpen(CBuffer *bp)
{
	int n, i, id;
	CBuffer tmp;
	CString type;
	CString host[2];
	int port[2];
	CString str;
	char *p;

	bp->GetStr(type);
	id = bp->Get32Bit();

	n = ChannelOpen();
	m_Chan[n].m_TypeName   = type;
	m_Chan[n].m_RemoteID   = id;
	m_Chan[n].m_RemoteWind = bp->Get32Bit();
	m_Chan[n].m_RemoteMax  = bp->Get32Bit();
	m_Chan[n].m_LocalComs  = 0;

	TRACE("Channel Open Recive %d(%d)\n", m_Chan[n].m_RemoteWind, m_Chan[n].m_RemoteMax);

	if ( type.CompareNoCase("auth-agent@openssh.com") == 0 ) {
		m_Chan[n].m_pFilter = new CAgent;
		m_Chan[n].m_pFilter->m_pChan = &(m_Chan[n]);
		m_Chan[n].m_Status |= CHAN_OPEN_REMOTE;
		SendMsgChannelOpenConfirmation(n);
		SendMsgChannelData(n);
		return FALSE;

	} else if ( type.CompareNoCase("x11") == 0 ) {
		bp->GetStr(host[0]);
		port[0] = bp->Get32Bit();

		if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHX11PF) )
			goto FAILURE;

		if ( (p = getenv("DISPLAY")) == NULL ) {
			if ( m_pDocument->m_ParamTab.m_XDisplay.IsEmpty() )
				p = ":0";
			else
				p = (char *)(LPCSTR)m_pDocument->m_ParamTab.m_XDisplay;
		}

		host[1].Empty();
		while ( *p != '\0' && *p != ':' )
			host[1] += *(p++);
		if ( *(p++) != ':' )
			goto FAILURE;
		port[1] = _tstoi(p);

		if ( host[1].IsEmpty() || host[1].CompareNoCase("unix") == 0 )
			host[1] = "127.0.0.1";

		m_Chan[n].m_Status |= CHAN_OPEN_REMOTE;
		m_Chan[n].m_lHost = host[1];
		m_Chan[n].m_lPort = 6000 + port[1];
		m_Chan[n].m_rHost = host[0];
		m_Chan[n].m_rPort = port[0];

		if ( (i = m_Chan[n].Open(host[1], 6000 + port[1])) < 0 ) {
			str.Format("Channel Open Failure %s:%d->%s:%d", host[0], port[0], host[1], 6000 + port[1]);
			AfxMessageBox(str);
			goto FAILURE;
		}

		return FALSE;

	} else if ( type.CompareNoCase("forwarded-tcpip") == 0 ) {
		bp->GetStr(host[0]);
		port[0] = bp->Get32Bit();
		bp->GetStr(host[1]);
		port[1] = bp->Get32Bit();

		for ( i = 0 ; i < m_Permit.GetSize() ; i++ ) {
			if ( port[0] == m_Permit[i].m_rPort )
				break;
		}

		if ( i >=  m_Permit.GetSize() ) {
			str.Format("Channel Open Permit Failure %s:%d->%s:%d", host[0], port[0], host[1], port[1]);
			AfxMessageBox(str);
			goto FAILURE;
		}

		m_Chan[n].m_Status |= CHAN_OPEN_REMOTE;
		m_Chan[n].m_lHost = host[1];
		m_Chan[n].m_lPort = port[1];
		m_Chan[n].m_rHost = host[0];
		m_Chan[n].m_rPort = port[0];

		if ( (i = m_Chan[n].Open(m_Permit[i].m_lHost, m_Permit[i].m_lPort)) < 0 ) {
			str.Format("Channel Open Failure %s:%d->%s:%d", host[0], port[0], host[1], port[1]);
			AfxMessageBox(str);
			goto FAILURE;
		}

		return FALSE;
	}

FAILURE:
	ChannelClose(n);
	tmp.Clear();
	tmp.Put8Bit(SSH2_MSG_CHANNEL_OPEN_FAILURE);
	tmp.Put32Bit(id);
	tmp.Put32Bit(SSH2_OPEN_CONNECT_FAILED);
	tmp.PutStr("open failed");
	tmp.PutStr("");
	SendPacket2(&tmp);
	return FALSE;
}
int Cssh::SSH2MsgChannelOpenReply(CBuffer *bp, int type)
{
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || m_Chan[id].m_Status != CHAN_OPEN_LOCAL )
		return TRUE;

	if ( type == SSH2_MSG_CHANNEL_OPEN_FAILURE ) {
		if ( m_Chan[id].m_pFilter != NULL )
			LogIt("Open Failure #%d Filter", id);
		else
			LogIt("Open Failure #%d %s:%d -> %s:%d", id, m_Chan[id].m_lHost, m_Chan[id].m_lPort, m_Chan[id].m_rHost, m_Chan[id].m_rPort);
		ChannelClose(id);
		return (id == m_StdChan ? TRUE : FALSE);
	}

	m_Chan[id].m_Status     |= CHAN_OPEN_REMOTE;
	m_Chan[id].m_RemoteID    = bp->Get32Bit();
	m_Chan[id].m_RemoteWind  = bp->Get32Bit();
	m_Chan[id].m_RemoteMax   = bp->Get32Bit();
	m_Chan[id].m_ConnectTime = CTime::GetCurrentTime();
	m_OpenChanCount++;

	TRACE("Channel OpenReply Recive %d(%d)\n", m_Chan[id].m_RemoteWind, m_Chan[id].m_RemoteMax);

	if ( m_Chan[id].m_pFilter != NULL ) {
		switch(m_Chan[id].m_pFilter->m_Type) {
		case SSHFT_STDIO:
			if ( (m_SSH2Status & SSH2_STAT_HAVESHELL) == 0 )
				SendMsgChannelRequesstShell(id);
			break;
		case SSHFT_SFTP:
			SendMsgChannelRequesstSubSystem(id);
			break;
		case SSHFT_RCP:
			SendMsgChannelRequesstExec(id);
			break;
		}
	} else
		LogIt("Open Confirmation #%d %s:%d -> %s:%d", id, m_Chan[id].m_lHost, m_Chan[id].m_lPort, m_Chan[id].m_rHost, m_Chan[id].m_rPort);

	SendMsgChannelData(id);
	return FALSE;
}
int Cssh::SSH2MsgChannelClose(CBuffer *bp)
{
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() )
		return TRUE;

	m_Chan[id].m_Eof |= CEOF_ICLOSE;
	ChannelCheck(id);

	return FALSE;
}
int Cssh::SSH2MsgChannelData(CBuffer *bp, int type)
{
	CBuffer tmp;
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;

	bp->GetBuf(&tmp);
	m_Chan[id].m_LocalComs += tmp.GetSize();

	if ( type == SSH2_MSG_CHANNEL_DATA )
		m_Chan[id].Send(tmp.GetPtr(), tmp.GetSize());

	ChannelPolling(id);
	return FALSE;
}
int Cssh::SSH2MsgChannelEof(CBuffer *bp)
{
	int id = bp->Get32Bit();
	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;
	m_Chan[id].m_Eof |= CEOF_IEOF;
	ChannelCheck(id);
	return FALSE;
}
int Cssh::SSH2MsgChannelRequesst(CBuffer *bp)
{
	int reply;
	CString str;
	CBuffer tmp;
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;

	bp->GetStr(str);
	reply = bp->Get8Bit();

	if ( str.Compare("exit-status") == 0 ) {
		bp->Get32Bit();
		if ( id == m_StdChan )
			m_SSH2Status &= ~SSH2_STAT_HAVESHELL;
	
	} else if ( str.Compare("exit-signal") == 0 ) {
		bp->GetStr(str);
		bp->Get8Bit();
		bp->GetStr(str);
		bp->GetStr(str);
		if ( id == m_StdChan )
			m_SSH2Status &= ~SSH2_STAT_HAVESHELL;
	}

	if ( reply ) {
		tmp.Put8Bit(SSH2_MSG_CHANNEL_SUCCESS);
		tmp.Put32Bit(id);
		SendPacket2(&tmp);
	}
	return FALSE;
}
int Cssh::SSH2MsgChannelRequestReply(CBuffer *bp, int type)
{
	int num;
	int id = bp->Get32Bit();

	if ( m_ChnReqMap.GetSize() <= 0 )
		return FALSE;
	num = m_ChnReqMap.GetAt(0);
	m_ChnReqMap.RemoveAt(0);

	switch(num) {
	case CHAN_REQ_PTY:		// pty-req
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			return TRUE;
		m_SSH2Status |= SSH2_STAT_HAVESTDIO;
		break;
	case CHAN_REQ_SHELL:	// shell
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			return TRUE;
		m_SSH2Status |= SSH2_STAT_HAVESHELL;
		if ( m_StdInRes.GetSize() > 0 && CHAN_OK(m_StdChan) ) {
			m_Chan[m_StdChan].m_pFilter->Send(m_StdInRes.GetPtr(), m_StdInRes.GetSize());
			m_StdInRes.Clear();
		}
		break;
	case CHAN_REQ_SFTP:		// subsystem
	case CHAN_REQ_EXEC:		// exec
		if ( type == SSH2_MSG_CHANNEL_FAILURE ) {
			m_Chan[id].m_Eof |= CEOF_DEAD;
			ChannelCheck(id);
		} else if ( m_Chan[id].m_pFilter != NULL )
			m_Chan[id].m_pFilter->OnConnect();
		break;
	case CHAN_REQ_X11:		// x11-req
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			AfxMessageBox("X11-req Failure");
		break;
	case CHAN_REQ_ENV:		// env
		if ( type == SSH2_MSG_CHANNEL_FAILURE )
			AfxMessageBox("env Failure");
		break;
	}
	return FALSE;
}
int Cssh::SSH2MsgChannelAdjust(CBuffer *bp)
{
	int len;
	int id = bp->Get32Bit();

	if ( id < 0 || id >= m_Chan.GetSize() || !CHAN_OK(id) )
		return TRUE;

	if ( (len = bp->Get32Bit()) < 0 )
		return TRUE;

	m_Chan[id].m_RemoteWind += len;

	TRACE("Recive Window Adjust %d\n", len);

	if ( m_Chan[id].m_Output.GetSize() > 0 )
		SendMsgChannelData(id);

	return FALSE;
}
int Cssh::SSH2MsgGlobalRequest(CBuffer *bp)
{
	CBuffer tmp;
	CString str, msg;
	BOOL success = FALSE;

	bp->GetStr(str);
	if ( !str.IsEmpty() ) {
		msg.Format("Get Msg Global Request '%s'", str);
		AfxMessageBox(msg);
	}
	if ( bp->Get8Bit() > 0 ) {
		tmp.Put8Bit(success ? SSH2_MSG_REQUEST_SUCCESS : SSH2_MSG_REQUEST_FAILURE);
		SendPacket2(&tmp);
	}
	return FALSE;
}
int Cssh::SSH2MsgGlobalRequestReply(CBuffer *bp, int type)
{
	int num;
	CString str;

	if ( m_GlbReqMap.GetSize() <= 0 ) {
		AfxMessageBox(IDE_SSHGLOBALREQERROR);
		return FALSE;
	}
	num = m_GlbReqMap.GetAt(0);
	m_GlbReqMap.RemoveAt(0);

	if ( (WORD)num == 0xFFFF )	// KeepAlive Reply
		return FALSE;

	if ( type == SSH2_MSG_REQUEST_FAILURE && num < m_Permit.GetSize() ) {
		str.Format("Global Request Failure %s:%d->%s:%d",
			m_Permit[num].m_lHost, m_Permit[num].m_lPort, m_Permit[num].m_rHost, m_Permit[num].m_rPort);
		AfxMessageBox(str);
		m_Permit.RemoveAt(num);
	}

	return FALSE;
}
void Cssh::RecivePacket2(CBuffer *bp)
{
	CString str;
	int type = bp->Get8Bit();

//	TRACE("SSH2 Type %d\n", type);

	switch(type) {
	case SSH2_MSG_KEXINIT:
		if ( SSH2MsgKexInit(bp) )
			goto DISCONNECT;
		m_SSH2Status |= SSH2_STAT_HAVEPROP;
		switch(m_DhMode) {
		case DHMODE_GROUP_1:
		case DHMODE_GROUP_14:
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
	case SSH2_MSG_NEWKEYS:
		if ( (m_SSH2Status & SSH2_STAT_HAVEKEYS) == 0 )
			goto DISCONNECT;
		if ( SSH2MsgNewKeys(bp) )
			goto DISCONNECT;
		m_SSH2Status &= ~SSH2_STAT_SENTKEXINIT;
		m_SSH2Status &= ~SSH2_STAT_HAVEKEYS;
		m_SSH2Status |= SSH2_STAT_HAVESESS;
		if ( (m_SSH2Status & SSH2_STAT_HAVELOGIN) == 0 ) {
			m_AuthStat = 0;
			SendMsgServiceRequest("ssh-userauth");
		}
		break;
	case SSH2_MSG_SERVICE_ACCEPT:
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		bp->GetStr(str);
		if ( str.Compare("ssh-userauth") == 0 ) {
			if ( !SendMsgUserAuthRequest(NULL) )
				goto DISCONNECT;
		} else
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
		if ( !m_pDocument->m_TextRam.IsOptEnable(TO_SSHPFORY) && (m_SSH2Status & SSH2_STAT_HAVESTDIO) == 0 ) {
			m_StdChan = ChannelOpen();
			m_Chan[m_StdChan].m_pFilter = new CStdIoFilter;
			m_Chan[m_StdChan].m_pFilter->m_pChan = &(m_Chan[m_StdChan]);
			m_Chan[m_StdChan].m_LocalPacks = 2 * 1024;
			m_Chan[m_StdChan].m_LocalWind = 16 * m_Chan[m_StdChan].m_LocalPacks;
			SendMsgChannelOpen(m_StdChan, "session");
		}
		if ( (m_SSH2Status & SSH2_STAT_HAVEPFWD) == 0 ) {
			m_SSH2Status |= SSH2_STAT_HAVEPFWD;
			PortForward();
		}
		if ( m_pDocument->m_TextRam.IsOptEnable(TO_SSHKEEPAL) && m_pDocument->m_TextRam.m_KeepAliveSec > 0 )
			((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(m_pDocument->m_TextRam.m_KeepAliveSec * 1000, TIMEREVENT_SOCK | TIMEREVENT_INTERVAL, this);
		break;
	case SSH2_MSG_USERAUTH_FAILURE:
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		bp->GetStr(str);
		if ( !SendMsgUserAuthRequest(str) )
			goto DISCONNECT;
		break;
	case SSH2_MSG_USERAUTH_BANNER:
		bp->GetStr(str);
		CExtSocket::OnReciveCallBack((void *)(LPCSTR)str, str.GetLength(), 0);
		break;

//	case SSH2_MSG_USERAUTH_PK_OK:				// 60	publickey
//	case SSH2_MSG_USERAUTH_PASSWD_CHANGEREQ:	// 60	password
	case SSH2_MSG_USERAUTH_INFO_REQUEST:		// 60	keyboard-interactive
		if ( (m_SSH2Status & SSH2_STAT_HAVESESS) == 0 )
			goto DISCONNECT;
		switch(m_AuthMode) {
		case AUTH_MODE_PUBLICKEY:
			if ( !SSH2MsgUserAuthPkOk(bp) )
				goto DISCONNECT;
			break;
		case AUTH_MODE_PASSWORD:
			if ( !SSH2MsgUserAuthPasswdChangeReq(bp) )
				goto DISCONNECT;
			break;
		case AUTH_MODE_KEYBOARD:
			if ( !SSH2MsgUserAuthInfoRequest(bp) )
				goto DISCONNECT;
			break;
		default:
			goto DISCONNECT;
		}
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
//		CExtSocket::OnReciveCallBack((void *)((LPCSTR)str), str.GetLength(), 0);
//		Close();
		AfxMessageBox(str);
		break;
	case SSH2_MSG_IGNORE:
	case SSH2_MSG_DEBUG:
	case SSH2_MSG_UNIMPLEMENTED:
		break;

	default:
		SendMsgUnimplemented();
		break;
	DISCONNECT:
		SendDisconnect2(SSH2_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT, "Packet Type Error");
		break;
	}
}
