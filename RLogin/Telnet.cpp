// Telnet.cpp: CTelnet クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "Telnet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CMint

#define BASEBITS (8 * sizeof(short) - 1)
#define BASE (1 << BASEBITS)
#define PROOT 3
#define HEXMODULUS "d4a0ba0250b6fd2ec626e7efd637df76c716e22d0944b88b"
#define KEYSIZE 192

CMint::CMint()
{
	bn = BN_new();
}
CMint::CMint(LPCSTR s)
{
	bn = BN_new();
	BN_hex2bn(&bn, s);
}
CMint::CMint(int n)
{
	CString s;
	s.Format("%x", n);
	bn = BN_new();
	BN_hex2bn(&bn, s);
}
CMint::~CMint()
{
	BN_free(bn);
}
void CMint::madd(CMint &rm)
{
	BIGNUM b;

	BN_init(&b);
	BN_add(&b, bn, rm.bn);
	BN_copy(bn, &b);
	BN_free(&b);
}
void CMint::mult(CMint &rm)
{
	BIGNUM b;
	BN_CTX *c;

	c = BN_CTX_new();
	BN_init(&b);
    BN_mul(&b, bn, rm.bn, c);
	BN_copy(bn, &b);
    BN_free(&b);
    BN_CTX_free(c);
}
void CMint::mdiv(CMint &rm)
{
	BIGNUM q, r;
	BN_CTX *c;

	c = BN_CTX_new();
	BN_init(&r);
	BN_init(&q);
	BN_div(&q, &r, bn, rm.bn, c);
	BN_copy(bn, &q);
	BN_free(&q);
	BN_free(&r);
	BN_CTX_free(c);
}
void CMint::msup(CMint &rm)
{
	BIGNUM q, r;
	BN_CTX *c;

	c = BN_CTX_new();
	BN_init(&r);
	BN_init(&q);
	BN_div(&q, &r, bn, rm.bn, c);
	BN_copy(bn, &r);
	BN_free(&q);
	BN_free(&r);
	BN_CTX_free(c);
}
void CMint::pow(CMint &bm, CMint &em, CMint &mm)
{
	BIGNUM b;
	BN_CTX *c;

	c = BN_CTX_new();
	BN_init(&b);
	BN_mod_exp(&b, bm.bn, em.bn, mm.bn, c);
 	BN_copy(bn, &b);
	BN_free(&b);
	BN_CTX_free(c);
}
void CMint::sdiv(short d, short *ro)
{
	CMint dm(d);
	BIGNUM q, r;
	BN_CTX *c;

	c = BN_CTX_new();
	BN_init(&q);
	BN_init(&r);
	BN_div(&q, &r, bn, dm.bn, c);
 	BN_copy(bn, &q);

	char *s = BN_bn2hex(&r);
	*ro = (short)strtol(s, NULL, 16);
	OPENSSL_free(s);

	BN_free(&r);
	BN_free(&q);
	BN_CTX_free(c);
}
void CMint::mtox(CString &tmp)
{
	char *s = BN_bn2hex(bn);
	tmp = s;
	while ( tmp.GetLength() < HEXKEYBYTES )
		tmp.Insert(0, "0");
	OPENSSL_free(s);
}
void CMint::Deskey(DesData *key)
{
	CMint a;
	int i;
	short r;
	short base = (1 << 8);
	unsigned char *p;

	a = *this;
	for ( i = 0 ; i < (((KEYSIZE - 64) / 2) / 8) ; i++ )
		a.sdiv(base, &r);
	p = (unsigned char *)key;
	for ( i = 0 ; i < 8 ; i++ ) {
		a.sdiv(base, &r);
		*(p++) = (unsigned char)r;
	}
}
void CMint::Ideakey(IdeaData *key)
{
	CMint a;
	int i;
	short r;
	short base = (1 << 8);
	unsigned char *p;

	a = *this;
	for ( i = 0 ; i < ((KEYSIZE - 128) / 8) ; i++ )
		a.sdiv(base, &r);
	p = (unsigned char *)key;
	for ( i = 0 ; i < 16 ; i++ ) {
		a.sdiv(base, &r);
		*(p++) = (unsigned char)r;
	}
}
void CMint::Debug()
{
	char *s = BN_bn2hex(bn);
	TRACE("%s\n", s);
	OPENSSL_free(s);
}

//////////////////////////////////////////////////////////////////////
// CTelnet

static const char *telopts[] = {
        "BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
        "STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
        "NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
        "NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
        "DATA ENTRY TERMINAL", "SUPDUP", "SUPDUP OUTPUT",
        "SEND LOCATION", "TERMINAL TYPE", "END OF RECORD",
        "TACACS UID", "OUTPUT MARKING", "TTYLOC",
        "3270 REGIME", "X.3 PAD", "NAWS", "TSPEED", "LFLOW",
        "LINEMODE", "XDISPLOC", "OLD-ENVIRON", "AUTHENTICATION",
        "ENCRYPT", "NEW-ENVIRON", "",
        0
	};

CTelnet::CTelnet(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = 2;
	ReciveStatus = RVST_NON;
}
CTelnet::~CTelnet()
{
}
BOOL CTelnet::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
    int n;

    for ( n = 0 ; n < TELOPT_MAX ; n++ ) {
		HisOpt[n].status = TELSTS_OFF;
		HisOpt[n].flags  = 0;
		MyOpt[n].status  = TELSTS_OFF;
		MyOpt[n].flags   = 0;
    }

	HisOpt[TELOPT_BINARY].flags		|= TELFLAG_ACCEPT;
	HisOpt[TELOPT_ECHO].flags		|= TELFLAG_ACCEPT;
//	HisOpt[TELOPT_SGA].flags		|= TELFLAG_ACCEPT;
//	HisOpt[TELOPT_LFLOW].flags		|= TELFLAG_ACCEPT;

	MyOpt[TELOPT_BINARY].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_ECHO].flags		|= TELFLAG_ACCEPT;
//	MyOpt[TELOPT_SGA].flags			|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_TTYPE].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_NAWS].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_NEW_ENVIRON].flags	|= TELFLAG_ACCEPT;
//	MyOpt[TELOPT_LFLOW].flags		|= TELFLAG_ACCEPT;

    SubOptLen = 0;
    ReciveStatus = RVST_NON;

	srand((int)time(NULL));
	PassSendFlag = FALSE;

	memset(stream, 0, sizeof(stream));
	memset(&temp_feed, 0, sizeof(DesData));
	temp_feed_init = FALSE;
	EncryptInputFlag  = FALSE;
	EncryptOutputFlag = FALSE;

	if ( !CExtSocket::Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType) )
		return FALSE;

	if ( !m_pDocument->m_TextRam.IsOptEnable(TO_RLTENAT) &&
		 !m_pDocument->m_TextRam.IsOptEnable(TO_RLTENEC) )
		HisOpt[TELOPT_ENCRYPT].flags |= TELFLAG_ACCEPT;

	if ( !m_pDocument->m_TextRam.IsOptEnable(TO_RLTENAT) ) {
		MyOpt[TELOPT_AUTHENTICATION].flags |= TELFLAG_ACCEPT;
		if ( !m_pDocument->m_TextRam.IsOptEnable(TO_RLTENEC) )
			MyOpt[TELOPT_ENCRYPT].flags |= TELFLAG_ACCEPT;
	}

	SetRecvSize(16 * 1024);
	return TRUE;
}
void CTelnet::OnConnect()
{
	BOOL val = 1;
	CExtSocket::SetSockOpt(SO_OOBINLINE, &val, sizeof(BOOL), SOL_SOCKET);
    ReciveStatus = RVST_DATA;
	SendOpt(TELC_DO, TELOPT_BINARY);
	SendOpt(TELC_WILL, TELOPT_BINARY);
	CExtSocket::OnConnect();
}
void CTelnet::SockSend(char *buf, int len)
{
	if ( EncryptOutputFlag )
		EncryptEncode(buf, len);
	CExtSocket::Send(buf, len);
}
int CTelnet::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	int n = 0;
	int len = nBufLen;
	char *buf = (char *)lpBuf;
	char tmp[512 + 2];

   if ( ReciveStatus == RVST_NON )
	   return 0;

	if ( (MyOpt[TELOPT_ECHO].flags & TELFLAG_ON) != 0 )
		CExtSocket::OnReciveCallBack(buf, nBufLen, 0);

	while ( len-- > 0 ) {
		if ( n >= 512 ) {
			SockSend(tmp, n);
			n = 0;
		}
		if ( (tmp[n++] = *(buf++)) == (char)TELC_IAC )
			tmp[n++] = (char)TELC_IAC;
	}
	SockSend(tmp, n);
	return nBufLen;
}
void CTelnet::PrintOpt(int st, int ch, int opt)
{
#ifdef	_DEBUG
	char tmp[32];
	if ( opt < 0 || opt > TELOPT_MAX )
		opt = TELOPT_MAX;
	sprintf(tmp, "%s %s %s\r\n",
		(st == 0 ? "SEND":"RECV"),
		(ch == TELC_DO   ? "DO" :
		(ch == TELC_DONT ? "DONT" :
		(ch == TELC_WONT ? "WONT" :
		(ch == TELC_WILL ? "WILL" :
		(ch == TELC_SB   ? "SB" :
		(ch == TELC_DM   ? "DM" : "???")))))),
		telopts[opt]);
	TRACE(tmp);
#endif
}
void CTelnet::SendStr(LPCSTR str)
{
	int n = 0;
	char tmp[256];

	while ( *str != '\0' ) {
		if ( *str == (char)TELC_IAC )
			tmp[n++] = (char)TELC_IAC;
		tmp[n++] = *(str++);
		if ( n > 250 ) {
			SockSend(tmp, n);
			n = 0;
		}
	}
	if ( n > 0 )
		SockSend(tmp, n);
}
void CTelnet::SendOpt(int ch, int opt)
{
	char tmp[4];
	tmp[0] = (char)TELC_IAC;
	tmp[1] = ch;
	tmp[2] = opt;
	SockSend(tmp, 3);
	PrintOpt(0, ch, opt);
}
void CTelnet::SendWindSize(int x, int y)
{
	int n = 0;
	char tmp[32];

   if ( ReciveStatus == RVST_NON )
	   return;

	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SB;
	tmp[n++] = (char)TELOPT_NAWS;
	if ( (tmp[n++] = (char)(x >> 8)) == (char)TELC_IAC )
		tmp[n++] = (char)TELC_IAC;
	if ( (tmp[n++] = (char)(x)) == (char)TELC_IAC )
		tmp[n++] = (char)TELC_IAC;
	if ( (tmp[n++] = (char)(y >> 8)) == (char)TELC_IAC )
		tmp[n++] = (char)TELC_IAC;
	if ( (tmp[n++] = (char)(y)) == (char)TELC_IAC )
		tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SE;

	SockSend(tmp, n);
	PrintOpt(0, TELC_SB, TELOPT_NAWS);
}
void CTelnet::SetXonXoff(int sw)
{
	CExtSocket::SetXonXoff(sw);
}
void CTelnet::GetStatus(CString &str)
{
	int n;
	CString tmp;
	static const char *optsts[] = { "OFF", "ON", "WOFF", "WON", "ROFF", "RON" };

	CExtSocket::GetStatus(str);

	str += "\r\n";
	tmp.Format("Telnet Status: %d\r\n", ReciveStatus);
	str += tmp;

	str += "\r\n";
	tmp.Format("Server Option: ");
	str += tmp;

	for ( n = 0 ; n < TELOPT_MAX ; n++ ) {
		tmp.Format(" %s:%s", telopts[n], optsts[HisOpt[n].status]);
		str += tmp;
	}
	str += "\r\n";

	str += "\r\n";
	tmp.Format("Client Option: ");
	str += tmp;

	for ( n = 0 ; n < TELOPT_MAX ; n++ ) {
		tmp.Format(" %s:%s", telopts[n], optsts[MyOpt[n].status]);
		str += tmp;
	}
	str += "\r\n";

	if ( EncryptOutputFlag || EncryptInputFlag )
		str += "\r\n";

	if ( EncryptOutputFlag ) {
		tmp.Format("EncryptOut: %s\r\n", (stream[DIR_ENC].mode == ENCTYPE_DES_CFB64 ? "des-cfb" : "des-ofb"));
		str += tmp;
	}

	if ( EncryptInputFlag ) {
		tmp.Format("EncryptIn: %s\r\n", (stream[DIR_DEC].mode == ENCTYPE_DES_CFB64 ? "des-cfb" : "des-ofb"));
		str += tmp;
	}
}
void CTelnet::OptFunc(struct TelOptTab *tab, int opt, int sw, int ch)
{
	if ( opt < 0 || opt >= TELOPT_MAX ) {
		SendOpt(TELDONT(ch), opt);
		return;
	}

	switch(tab[opt].status) {
	case TELSTS_OFF:
		switch(sw) {
		case DO_ON:
			if ( (tab[opt].flags & TELFLAG_ACCEPT) != 0 ) {
				tab[opt].status = TELSTS_ON;
				SendOpt(TELDO(ch), opt);
			} else {
				tab[opt].status = TELSTS_OFF;
				SendOpt(TELDONT(ch), opt);
			}
			break;
		case SW_ON:
			tab[opt].status = TELSTS_WANT_ON;
			SendOpt(TELDO(ch), opt);
			break;
		}
		break;

	case TELSTS_ON:
		switch(sw) {
		case DO_OFF:
			tab[opt].status = TELSTS_OFF;
			SendOpt(TELDONT(ch), opt);
			break;
		case SW_OFF:
			tab[opt].status = TELSTS_WANT_OFF;
			SendOpt(TELDONT(ch), opt);
			break;
		}
		break;

	case TELSTS_WANT_OFF:
		switch(sw) {
		case DO_OFF:
			tab[opt].status = TELSTS_OFF;
			break;
		case DO_ON:
			tab[opt].status = TELSTS_ON;
			break;
		case SW_ON:
			tab[opt].status = TELSTS_REVS_OFF;
			break;
		}
		break;

	case TELSTS_WANT_ON:
		switch(sw) {
		case DO_OFF:
			tab[opt].status = TELSTS_OFF;
			break;
		case DO_ON:
			tab[opt].status = TELSTS_ON;
			break;
		case SW_OFF:
			tab[opt].status = TELSTS_REVS_ON;
			break;
		}
		break;

	case TELSTS_REVS_OFF:
		switch(sw) {
		case DO_OFF:
			tab[opt].status = TELSTS_WANT_ON;
			SendOpt(TELDO(ch), opt);
			break;
		case DO_ON:
			tab[opt].status = TELSTS_ON;
			break;
		case SW_OFF:
			tab[opt].status = TELSTS_WANT_OFF;
			break;
		}
		break;

	case TELSTS_REVS_ON:
		switch(sw) {
		case DO_OFF:
			tab[opt].status = TELSTS_OFF;
			break;
		case DO_ON:
			tab[opt].status = TELSTS_WANT_OFF;
			SendOpt(TELDONT(ch), opt);
			break;
		case SW_ON:
			tab[opt].status = TELSTS_WANT_ON;
			break;
		}
		break;
	}

	switch(sw) {
	case DO_OFF:
	case DO_ON:
		switch(tab[opt].status) {
		case TELSTS_OFF:
			tab[opt].flags &= ~TELFLAG_ON;
			break;
		case TELSTS_ON:
			tab[opt].flags |= TELFLAG_ON;
			if ( opt == TELOPT_NAWS )
				SendWindSize(m_pDocument->m_TextRam.m_Cols, m_pDocument->m_TextRam.m_Lines);
			else if ( opt == TELOPT_ENCRYPT )
				EncryptSendSupport();
			break;
		}
		break;
	}
}
void CTelnet::SubOptFunc(char *buf, int len)
{
	int n, i;
	int ptr = 0;
	char tmp[256];
	CStringIndex env;

#define	SB_GETC()	(ptr >= len ? EOF : (buf[ptr++] & 0xFF))

	PrintOpt(1, TELC_SB, buf[0] & 0xFF);

	switch(SB_GETC()) {
	case TELOPT_TTYPE:
        if ( (MyOpt[TELOPT_TTYPE].flags & TELFLAG_ON) == 0 )
            break;
        if ( SB_GETC() != TELQUAL_SEND )
			break;
		n = 0;
		tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)TELC_SB;
		tmp[n++] = (char)TELOPT_TTYPE;
		tmp[n++] = (char)TELQUAL_IS;
		SockSend(tmp, n);
		SendStr(m_pDocument->m_ServerEntry.m_TermName);
		n = 0;
		tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)TELC_SE;
		SockSend(tmp, n);
		PrintOpt(0, TELC_SB, TELOPT_TTYPE);
		break;

	case TELOPT_NEW_ENVIRON:
        if ( (MyOpt[TELOPT_NEW_ENVIRON].flags & TELFLAG_ON) == 0 )
            break;
        if ( SB_GETC() != TELQUAL_SEND )
			break;

		n = 0;
		tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)TELC_SB;
		tmp[n++] = (char)TELOPT_NEW_ENVIRON;
		tmp[n++] = (char)TELQUAL_IS;
		SockSend(tmp, n);

		env.GetString(m_pDocument->m_ParamTab.m_ExtEnvStr);
		env["USER"] = m_pDocument->m_ServerEntry.m_UserName;
		for ( i = 0 ; i < env.GetSize() ; i++ ) {
			if ( env[i].m_Value == 0 || env[i].m_nIndex.IsEmpty() || env[i].m_String.IsEmpty() )
				continue;
			n = 0;
			tmp[n++] = (char)ENV_VAR;
			SockSend(tmp, n);
			SendStr(env[i].m_nIndex);
			n = 0;
			tmp[n++] = (char)ENV_VALUE;
			SockSend(tmp, n);
			SendStr(env[i].m_String);
		}

		n = 0;
		tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)TELC_SE;
		SockSend(tmp, n);

		PrintOpt(0, TELC_SB, TELOPT_NEW_ENVIRON);
		break;

	case TELOPT_AUTHENTICATION:
		switch(SB_GETC()) {
		case TELQUAL_IS:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_OFF )
				return;
			TRACE("auth_is\n");
			break;
		case TELQUAL_SEND:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_WANT_OFF )
				return;
			AuthSend(buf + 2, len - 2);
			break;
		case TELQUAL_REPLY:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_WANT_OFF )
				return;
			AuthReply(buf + 2, len - 2);
			break;
		case TELQUAL_NAME:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_OFF )
				return;
			TRACE("auth_name\n");
			break;
		}
		break;

	case TELOPT_ENCRYPT:
		switch(SB_GETC()) {
		case ENCRYPT_START:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_OFF )
				return;
			EncryptStart(buf + 2, len - 2);
			break;
		case ENCRYPT_END:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_OFF )
				return;
			EncryptEnd();
			break;
		case ENCRYPT_SUPPORT:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_WANT_OFF )
				return;
			TRACE("encrypt_support\n");
			break;
		case ENCRYPT_REQSTART:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_WANT_OFF )
				return;
			EncryptRequestStart();
			break;
		case ENCRYPT_REQEND:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_WANT_OFF )
				return;
			EncryptRequestEnd();
			break;
		case ENCRYPT_IS:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_OFF )
				return;
			EncryptIs(buf + 2, len - 2);
			break;
		case ENCRYPT_REPLY:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_WANT_OFF )
				return;
			EncryptReply(buf + 2, len - 2);
			break;
		case ENCRYPT_ENC_KEYID:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_OFF )
				return;
			EncryptKeyid(DIR_DEC, buf + 2, len - 2);
			break;
		case ENCRYPT_DEC_KEYID:
			if ( MyOpt[TELOPT_AUTHENTICATION].status == TELSTS_WANT_OFF )
				return;
			EncryptKeyid(DIR_ENC, buf + 2, len - 2);
			break;
		}
		break;
	}
}
void CTelnet::OnReciveCallBack(void* lpBuf, int nBufLen, int nFlags)
{
    int c;
    int n = 0;
	int len = nBufLen;
    char *tmp, *buf;
	int DoDecode = FALSE;

	if ( nFlags != 0 || ReciveStatus == RVST_NON )
		return;

	tmp = buf = (char *)lpBuf;

    while ( len-- > 0 ) {
		if ( EncryptInputFlag && !DoDecode ) {
			EncryptDecode(buf, len + 1);
			DoDecode = TRUE;
		}
		c = (unsigned char)(*(buf++));
		switch(ReciveStatus) {
		case RVST_DATA:
			if ( c == TELC_IAC )
				ReciveStatus = RVST_IAC;
			else {
	    		tmp[n++] = c;
				while ( len > 0 ) {
					if ( *buf == (char)TELC_IAC )
						break;
					tmp[n++] = *(buf++);
					len--;
				}
			}
			break;

		case RVST_IAC:
		PROC_IAC:
			switch(c) {
			case TELC_IAC:
				tmp[n++] = (char)TELC_IAC;
				ReciveStatus = RVST_DATA;
				break;
			case TELC_DONT:
				ReciveStatus = RVST_DONT;
				break;
			case TELC_DO:
				ReciveStatus = RVST_DO;
				break;
			case TELC_WONT:
				ReciveStatus = RVST_WONT;
				break;
			case TELC_WILL:
				ReciveStatus = RVST_WILL;
				break;
		    case TELC_SB:
				ReciveStatus = RVST_SB;
				break;
			case TELC_DM:
				PrintOpt(1, c, TELOPT_NON);
				ReciveStatus = RVST_DATA;
				break;
			case TELC_NOP:
			case TELC_GA:
				PrintOpt(1, c, TELOPT_NON);
				ReciveStatus = RVST_DATA;
				break;
			default:
				ReciveStatus = RVST_DATA;
				break;
		    }
			break;

		case RVST_DONT:
			PrintOpt(1, TELC_DONT, c);
			OptFunc(MyOpt, c, DO_OFF, TELC_WONT);
			ReciveStatus = RVST_DATA;
			break;
		case RVST_DO:
			PrintOpt(1, TELC_DO, c);
		    OptFunc(MyOpt, c, DO_ON, TELC_WONT);
			ReciveStatus = RVST_DATA;
			break;
		case RVST_WONT:
			PrintOpt(1, TELC_WONT, c);
			OptFunc(HisOpt, c, DO_OFF, TELC_DONT);
			ReciveStatus = RVST_DATA;
			break;
		case RVST_WILL:
			PrintOpt(1, TELC_WILL, c);
			OptFunc(HisOpt, c, DO_ON, TELC_DONT);
			ReciveStatus = RVST_DATA;
			break;

		case RVST_SB:
		    if ( c == TELC_IAC )
				ReciveStatus = RVST_SE;
			else if ( SubOptLen < SUBOPTLEN )
				SubOptBuf[SubOptLen++] = c;
			break;
		case RVST_SE:
			switch(c) {
			case TELC_IAC:
	    		if ( SubOptLen < SUBOPTLEN )
					SubOptBuf[SubOptLen++] = c;
				ReciveStatus = RVST_SB;
				break;
			case TELC_SE:
				SubOptFunc(SubOptBuf, SubOptLen);
				SubOptLen = 0;
				ReciveStatus = RVST_DATA;
				break;
			default:
				SubOptFunc(SubOptBuf, SubOptLen);
				SubOptLen = 0;
				ReciveStatus = RVST_IAC;
				goto PROC_IAC;
			}
			break;
		}
    }

	CExtSocket::OnReciveCallBack(tmp, n, 0);
}
void CTelnet::SendOptData(int n1, int n2, int n3, void *buf, int len)
{
	int n;
	unsigned char *s = (unsigned char *)buf;
	char tmp[1024];

	if ( len < 0 )
		len = (int)strlen((const char *)s);

	tmp[0] = (char)TELC_IAC;
	tmp[1] = (char)TELC_SB;
	tmp[2] = (char)TELOPT_AUTHENTICATION;
	tmp[3] = (char)TELQUAL_IS;
	tmp[4] = (char)n1;		// ap->type
	tmp[5] = (char)n2;		// ap->way
	tmp[6] = (char)n3;		// type

	for ( n = 7 ; len > 0 && n < 1020 ; len-- ) {
		if ( (tmp[n++] = *(s++)) == (char)TELC_IAC )
			tmp[n++] = (char)TELC_IAC;
	}
	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SE;

	SockSend(tmp, n);
}

void CTelnet::NsaGenkey(char *p)
{
	sprintf(p, "%08x", time(NULL));
}
void CTelnet::NsaEncode(char *p, int len, char *key)
{
    int n, i;

    for ( n = i = 0 ; n < len ; n++ ) {
        p[n] = (p[n] ^ key[i]);
        if ( ++i >= 8 )
            i = 0;
    }
}
void CTelnet::NsaDecode(char *p, int len, char *key)
{
    int n, i;

    for ( n = i = 0 ; n < len ; n++ ) {
        p[n] = (p[n] ^ key[i]);
        if ( ++i >= 8 )
            i = 0;
    }
}

void CTelnet::AuthSend(char *buf, int len)
{
	static char nosup[] = { (char)TELC_IAC, (char)TELC_SB, (char)TELOPT_AUTHENTICATION, (char)TELQUAL_IS, 
							(char)AUTHTYPE_NULL, (char)0, (char)TELC_IAC, (char)TELC_SE };

	while ( len >= 2 ) {
		switch(buf[0]) {
		case AUTHTYPE_SRA:
			SraGenkey(pka, ska);
			SendOptData(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_KEY, pka, HEXKEYBYTES);
			return;
/********************
		case AUTHTYPE_NSA:
			NsaGenkey(pka);
			SendOptData(AUTHTYPE_NSA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, NSA_KEY, pka, NSAKEYBYTES);
			return;
*********************/
		}
		len -= 2;
		buf += 2;
	}

	SockSend(nosup, sizeof(nosup));
}

void CTelnet::AuthReply(char *buf, int len)
{
	int n, a, b;
	char tmp[256];

	a = buf[0];
	b = buf[2];
	len -= 3;
	buf += 3;

	if ( len < 0 )
		return;

	switch(a) {
	case AUTHTYPE_SRA:
		switch(b) {
		case SRA_KEY:
			if ( len < HEXKEYBYTES )
				break;
			memcpy(pkb, buf, HEXKEYBYTES);
			pkb[HEXKEYBYTES] = '\0';

			SraCommonKey(ska, pkb);

			if ( (n = m_pDocument->m_ServerEntry.m_UserName.GetLength()) > 250 )
				n = 250;
			strncpy(tmp, m_pDocument->m_ServerEntry.m_UserName, n);
			tmp[n] = '\0';

			n = SraEncode(tmp, n, &ddk);
			SendOptData(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_USER, tmp, n);
			PassSendFlag = FALSE;
			break;

		case SRA_CONTINUE:
			if ( PassSendFlag ) {
				SendOptData(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_REJECT, NULL, 0);
				break;
			}
			if ( (n = m_pDocument->m_ServerEntry.m_PassName.GetLength()) > 250 )
				n = 250;
			strncpy(tmp, m_pDocument->m_ServerEntry.m_PassName, n);
			tmp[n] = '\0';

			n = SraEncode(tmp, n, &ddk);
			SendOptData(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_PASS, tmp, n);
			PassSendFlag = TRUE;
			break;

		case SRA_ACCEPT:
			EncryptSessionKey();
			break;
		}
		break;
/**************************************
	case AUTHTYPE_NSA:
		switch(b) {
		case NSA_KEY:
			if ( len < NSAKEYBYTES )
				break;
			memcpy(pkb, buf, NSAKEYBYTES);
			pkb[NSAKEYBYTES] = '\0';

			if ( (n = pView->LoginName.GetLength()) > 250 )
				n = 250;
			strncpy(tmp, pView->LoginName, n);
			tmp[n] = '\0';

			NsaEncode(tmp, n, pkb);
			SendOptData(AUTHTYPE_NSA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, NSA_USER, tmp, n);
			PassSendFlag = FALSE;
			break;

		case NSA_CONTINUE:
			if ( PassSendFlag ) {
				SendOptData(AUTHTYPE_NSA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, NSA_REJECT, NULL, 0);
				break;
			}
			if ( (n = pView->PassName.GetLength()) > 250 )
				n = 250;
			strncpy(tmp, pView->PassName, n);
			tmp[n] = '\0';

			NsaEncode(tmp, n, pkb);
			SendOptData(AUTHTYPE_NSA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, NSA_PASS, tmp, n);
			PassSendFlag = TRUE;
			break;
		}
		break;
*****************************************/
	}
}
void CTelnet::SraGenkey(char *p, char *s)
{
	CMint pk(0);
	CMint sk(0);
	CMint base(BASE);
	CMint root(PROOT);
	CMint mod(HEXMODULUS);

	unsigned short seed[KEYSIZE / BASEBITS + 1];

	for ( int n = 0 ; n < (KEYSIZE / BASEBITS + 1) ; n++ ) {
		seed[n] = (unsigned short)rand();
		CMint tmp(seed[n] % BASE);
		sk.mult(base);
		sk.madd(tmp);
	}

	sk.msup(mod);
	pk.pow(root, sk, mod);

	CString tmp;

	sk.mtox(tmp);
	strcpy(s, tmp);
	pk.mtox(tmp);
	strcpy(p, tmp);
}
void CTelnet::SraCommonKey(char *s, char *p)
{
	CMint pub(p);
	CMint sec(s);
	CMint com(0);
	CMint mod(HEXMODULUS);

	com.pow(pub, sec, mod);
	com.Deskey(&ddk);
	com.Ideakey(&idk);
	des_set_odd_parity(&ddk);
}
int CTelnet::SraEncode(char *p, int len, DesData *key)
{
	char buf[256];
	DesData tk;
    des_key_schedule ks;
	int n, mx;
	static char hextab[17] = "0123456789ABCDEF";

	memset(buf, 0, 256);
	memset(tk, 0, 8);
	if ( (mx = ((len + 7) / 8) * 8) > 128 )
		mx = 128;
	des_key_sched(key, ks);
	des_cbc_encrypt((const unsigned char *)p, (unsigned char *)buf, mx, ks, &tk, DES_ENCRYPT);

	for ( n = 0 ; n < mx ; n++ ) {
		*(p++) = hextab[(buf[n] & 0xf0) >> 4];
		*(p++) = hextab[(buf[n] & 0x0f)];
	}
	*p = '\0';
	return (n * 2);
}
int CTelnet::SraDecode(char *p, int len, DesData *key)
{
	char buf[256];
	DesData i;
	des_key_schedule k;
    int n1,n2,op,l;

	memset(&i,0,sizeof(i));
	memset(buf,0,sizeof(buf));
	for ( l=0, op=0 ; l < len / 2 ; l++, op += 2 ) {
		if ( p[op] > '9')
			n1 = p[op] - 'A' + 10;
		else
			n1 = p[op] - '0';
		if ( p[op + 1] > '9')
			n2 = p[op+1] - 'A' + 10;
		else
			n2 = p[op+1] - '0';
		buf[l] = n1 * 16 + n2;
	}
	des_key_sched(key, k);
	des_cbc_encrypt((unsigned char *)buf, (unsigned char *)p, len / 2, k , &i, DES_DECRYPT);
	p[len / 2] = '\0';
	return (len / 2);
}

void CTelnet::EncryptSendSupport()
{
	TRACE("EncryptSendSupport\n");

	EncryptSendRequestStart();

	int n = 0;
	feed[n++] = (char)TELC_IAC;
	feed[n++] = (char)TELC_SB;
	feed[n++] = (char)TELOPT_ENCRYPT;
	feed[n++] = (char)ENCRYPT_SUPPORT;
	feed[n++] = (char)ENCTYPE_DES_CFB64;
	feed[n++] = (char)ENCTYPE_DES_OFB64;
	feed[n++] = (char)TELC_IAC;
	feed[n++] = (char)TELC_SE;

	SockSend(feed, n);
}
void CTelnet::EncryptSessionKey()
{
	TRACE("EncryptSessionKey\n");

	memcpy(&krb_key, &ddk, sizeof(DesData));

	memcpy(&(stream[DIR_ENC].ikey), &krb_key, sizeof(DesData));
	des_key_sched(&krb_key, stream[DIR_ENC].sched);
	memcpy(&(stream[DIR_ENC].output), &(stream[DIR_ENC].iv), sizeof(DesData));
	stream[DIR_ENC].index = sizeof(DesData);
	stream[DIR_ENC].status |= STM_HAVE_IKEY;

	memcpy(&(stream[DIR_DEC].ikey), &krb_key, sizeof(DesData));
	des_key_sched(&krb_key, stream[DIR_DEC].sched);
	memcpy(&(stream[DIR_DEC].output), &(stream[DIR_DEC].iv), sizeof(DesData));
	stream[DIR_DEC].index = sizeof(DesData);
	stream[DIR_DEC].status |= STM_HAVE_IKEY;

	des_key_sched(&krb_key, krb_sched);
}
void CTelnet::EncryptIs(char *p, int len)
{
	int n = 0;

	TRACE("EncryptIs %x\n", stream[DIR_DEC].status);

	if ( len < 1 )
		return;

	switch(p[0]) {
	case ENCTYPE_DES_CFB64: 
	case ENCTYPE_DES_OFB64:
		stream[DIR_DEC].mode = p[0];
		break;
	default:
		return;
	}

	feed[n++] = (char)TELC_IAC;
	feed[n++] = (char)TELC_SB;
	feed[n++] = (char)TELOPT_ENCRYPT;
	feed[n++] = (char)ENCRYPT_REPLY;
	feed[n++] = (char)stream[DIR_DEC].mode;
	feed[n++] = (char)FB64_IV_OK;
	feed[n++] = (char)TELC_IAC;
	feed[n++] = (char)TELC_SE;

	if ( len < 10 || p[1] != FB64_IV || (stream[DIR_DEC].status & STM_HAVE_IKEY) == 0 )
		feed[5] = (char)FB64_IV_BAD;
	else {
		memcpy(&(stream[DIR_DEC].iv), p + 2, sizeof(DesData));
		memcpy(&(stream[DIR_DEC].output), p + 2, sizeof(DesData));
		des_key_sched(&(stream[DIR_DEC].ikey), stream[DIR_DEC].sched);
		stream[DIR_DEC].status |= STM_HAVE_IV;
	}
	SockSend(feed, n);

//	EncryptSendRequestStart();
}
void CTelnet::EncryptReply(char *p, int len)
{
	int n, i = 0;

	TRACE("EncryptReply %x\n", stream[DIR_ENC].status);

	if ( len < 2 || p[1] != (char)FB64_IV_OK )
		return;

	switch(p[0]) {
	case ENCTYPE_DES_CFB64: 
	case ENCTYPE_DES_OFB64:
		stream[DIR_ENC].mode = p[0];
		break;
	default:
		return;
	}

	if ( (stream[DIR_ENC].status & STM_HAVE_IKEY) == 0 || temp_feed_init == FALSE )
		return;

	memcpy(&(stream[DIR_ENC].iv), &temp_feed, sizeof(DesData));
	memcpy(&(stream[DIR_ENC].output), &temp_feed, sizeof(DesData));
	des_key_sched(&(stream[DIR_ENC].ikey), stream[DIR_ENC].sched);
	stream[DIR_ENC].index = sizeof(DesData);
	stream[DIR_ENC].status |= STM_HAVE_IV;

	stream[DIR_ENC].keylen = 1;
	stream[DIR_ENC].keyid[0] = '\0';
	stream[DIR_ENC].status |= STM_HAVE_KEYID;

	TRACE("Send Keyid %d = %d %d\n", DIR_ENC, stream[DIR_ENC].keylen, stream[DIR_ENC].keyid[0]);

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SB;
	feed[i++] = (char)TELOPT_ENCRYPT;
	feed[i++] = (char)ENCRYPT_ENC_KEYID;

	for ( n = 0 ; n < stream[DIR_ENC].keylen && i < 60 ; n++ ) {
		if ( (feed[i++] = stream[DIR_ENC].keyid[n]) == (char)TELC_IAC )
			feed[i++] = (char)TELC_IAC;
	}

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SE;

	SockSend(feed, i);
}
void CTelnet::EncryptKeyid(int dir, char *p, int len)
{
	int n, i = 0;

	TRACE("EncryptKeyid %d = %d %d ? %d %d\n", dir, len, *p, stream[dir].keylen, stream[dir].keyid[0]);

	if ( len == 0 ) {
		if ( stream[dir].keylen == 0 )
			return;
		stream[dir].keylen = 0;
		stream[dir].status &= ~STM_HAVE_KEYID;
	} else if ( len != stream[dir].keylen || memcmp(p, stream[dir].keyid, len) != 0 ) {
		if ( len > 8 )
			len = 8;
		stream[dir].keylen = len;
		memcpy(stream[dir].keyid, p, len);
		if ( len != 1 || *p != '\0' ) {
			stream[dir].keylen = 0;
			stream[dir].status &= ~STM_HAVE_KEYID;
		} else
			stream[dir].status |= STM_HAVE_KEYID;
	} else {
		if ( len != 1 || *p != '\0' ) {
			stream[dir].keylen = 0;
			stream[dir].status &= ~STM_HAVE_KEYID;
		} else {
			stream[dir].status |= STM_HAVE_KEYID;
			if ( dir == DIR_ENC )		// ENC
				EncryptSendStart();
		}
		return;
	}

	TRACE("Send Keyid %d = %d %d\n", dir, stream[dir].keylen, stream[dir].keyid[0]);

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SB;
	feed[i++] = (char)TELOPT_ENCRYPT;
	feed[i++] = (char)(dir == DIR_ENC ? ENCRYPT_ENC_KEYID : ENCRYPT_DEC_KEYID);

	for ( n = 0 ; n < stream[dir].keylen && i < 60 ; n++ ) {
		if ( (feed[i++] = stream[dir].keyid[n]) == (char)TELC_IAC )
			feed[i++] = (char)TELC_IAC;
	}

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SE;

	SockSend(feed, i);
}
void CTelnet::EncryptSendRequestStart()
{
	int n, i = 0;

	TRACE("EncryptSendRequestStart %x\n", stream[DIR_DEC].status);

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SB;
	feed[i++] = (char)TELOPT_ENCRYPT;
	feed[i++] = (char)ENCRYPT_REQSTART;

	for ( n = 0 ; n < stream[DIR_DEC].keylen && i < 60 ; n++ ) {
		if ( (feed[i++] = stream[DIR_DEC].keyid[n]) == (char)TELC_IAC )
			feed[i++] = (char)TELC_IAC;
	}

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SE;

	SockSend(feed, i);
}
void CTelnet::EncryptSendTempFeed()
{
	int n, i = 0;

	TRACE("EncryptSendTempFeed %x %x\n",stream[DIR_DEC].status, stream[DIR_ENC].status);

	des_random_key(&temp_feed);
	des_ecb_encrypt(&temp_feed, &temp_feed, krb_sched, 1);
	temp_feed_init = TRUE;

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SB;
	feed[i++] = (char)TELOPT_ENCRYPT;
	feed[i++] = (char)ENCRYPT_IS;
	feed[i++] = (char)stream[DIR_DEC].mode;
	feed[i++] = (char)FB64_IV;

	for ( n = 0 ; n < 8 ; n++ ) {
		if ( (feed[i++] = temp_feed[n]) == (char)TELC_IAC )
			feed[i++] = (char)TELC_IAC;
	}

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SE;

	SockSend(feed, i);
}
void CTelnet::EncryptSendStart()
{
	int n, i = 0;

	TRACE("EncryptOutputStart %x\n", stream[DIR_ENC].status);

	if ( stream[DIR_ENC].status != STM_HAVE_OK )
		return;

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SB;
	feed[i++] = (char)TELOPT_ENCRYPT;
	feed[i++] = (char)ENCRYPT_START;

	for ( n = 0 ; n < stream[DIR_ENC].keylen && i < 60 ; n++ ) {
		if ( (feed[i++] = stream[DIR_ENC].keyid[n]) == (char)TELC_IAC )
			feed[i++] = (char)TELC_IAC;
	}

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SE;

	SockSend(feed, i);

	EncryptOutputFlag = TRUE;
	EncryptStatus();
}
void CTelnet::EncryptStart(char *p, int len)
{
	TRACE("EncryptInputStart %x\n", stream[DIR_DEC].status);

	if ( stream[DIR_DEC].status == STM_HAVE_OK )
		EncryptInputFlag = TRUE;

	EncryptStatus();
	EncryptSendTempFeed();
}
void CTelnet::EncryptEnd()
{
	TRACE("EncryptEnd\n");
	EncryptInputFlag = FALSE;
	EncryptStatus();
}
void CTelnet::EncryptRequestStart()
{
	TRACE("EncryptRequestStart\n");
	EncryptSendStart();
}
void CTelnet::EncryptRequestEnd()
{
	int i = 0;

	TRACE("EncryptRequestEnd\n");

	if ( EncryptOutputFlag == FALSE )
		return;

	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SB;
	feed[i++] = (char)TELOPT_ENCRYPT;
	feed[i++] = (char)ENCRYPT_END;;
	feed[i++] = (char)TELC_IAC;
	feed[i++] = (char)TELC_SE;

	SockSend(feed, i);

	EncryptOutputFlag = FALSE;
	EncryptStatus();
}
void CTelnet::EncryptDecode(char *p, int len)
{
	int idx = stream[DIR_DEC].index;
	switch(stream[DIR_DEC].mode) {
	case  ENCTYPE_DES_CFB64:
		while ( len-- > 0 ) {
			if ( idx >= sizeof(DesData) ) {
				DesData b;
				des_ecb_encrypt(&(stream[DIR_DEC].output), &b, stream[DIR_DEC].sched, 1);
				memcpy(&(stream[DIR_DEC].feed), &b, sizeof(DesData));
				idx = 0;
			}
			stream[DIR_DEC].output[idx] = *p;
			*(p++) ^= stream[DIR_DEC].feed[idx];
			idx++;
		}
		stream[DIR_DEC].index = idx;
		break;
	case ENCTYPE_DES_OFB64:
		while ( len-- > 0 ) {
			if ( idx >= sizeof(DesData) ) {
				DesData b;
				des_ecb_encrypt(&(stream[DIR_DEC].feed), &b, stream[DIR_DEC].sched, 1);
				memcpy(&(stream[DIR_DEC].feed), &b, sizeof(DesData));
				idx = 0;
			}
			*(p++) ^= stream[DIR_DEC].feed[idx];
			idx++;
		}
		stream[DIR_DEC].index = idx;
		break;
	}
}
void CTelnet::EncryptEncode(char *p, int len)
{
	int idx = stream[DIR_ENC].index;
	switch(stream[DIR_ENC].mode) {
	case  ENCTYPE_DES_CFB64:
		while ( len-- > 0 ) {
			if ( idx >= sizeof(DesData) ) {
				DesData b;
				des_ecb_encrypt(&(stream[DIR_ENC].output), &b, stream[DIR_ENC].sched, 1);
				memcpy(&(stream[DIR_ENC].feed), &b, sizeof(DesData));
				idx = 0;
			}
			*p = stream[DIR_ENC].output[idx] = stream[DIR_ENC].feed[idx] ^ *p;
			p++;
			idx++;
		}
		stream[DIR_ENC].index = idx;
		break;
	case ENCTYPE_DES_OFB64:
		while ( len-- > 0 ) {
			if ( idx >= sizeof(DesData) ) {
				DesData b;
				des_ecb_encrypt(&(stream[DIR_ENC].feed), &b, stream[DIR_ENC].sched, 1);
				memcpy(&(stream[DIR_ENC].feed), &b, sizeof(DesData));
				idx = 0;
			}
			*(p++) ^= stream[DIR_ENC].feed[idx];
			idx++;
		}
		stream[DIR_ENC].index = idx;
		break;
	}
}
void CTelnet::EncryptStatus()
{
	CString tmp;

	if ( EncryptOutputFlag || EncryptInputFlag ) {
		if ( EncryptOutputFlag )
			tmp += (stream[DIR_ENC].mode == ENCTYPE_DES_CFB64 ? "cfb+" : "ofb+");
		else
			tmp += "none+";

		if ( EncryptInputFlag )
			tmp += (stream[DIR_DEC].mode == ENCTYPE_DES_CFB64 ? "cfb" : "ofb");
		else
			tmp += "none";
	}
	m_pDocument->SetStatus(tmp);
}