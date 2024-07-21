//////////////////////////////////////////////////////////////////////
// Telnet.cpp : ŽÀ‘•ƒtƒ@ƒCƒ‹
//

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

///////////////////////////////////////////////////////
// CFifoTelnet

CFifoTelnet::CFifoTelnet(class CRLoginDoc *pDoc, class CExtSocket *pSock) : CFifoThread(pDoc, pSock)
{
}
void CFifoTelnet::OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult)
{
	switch(cmd) {
	case FIFO_CMD_MSGQUEIN:
		switch(param) {
		case FIFO_QCMD_SENDBREAK:
			((CTelnet *)m_pSock)->FifoSendBreak(msg);
			break;
		case FIFO_QCMD_SENDWINSIZE:
			((CTelnet *)m_pSock)->FifoSendWindSize();
			break;
		}
		break;
	}
}

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
	CStringA s;
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
	BIGNUM *b;

	b = BN_new();
	BN_add(b, bn, rm.bn);
	BN_copy(bn, b);
	BN_free(b);
}
void CMint::mult(CMint &rm)
{
	BIGNUM *b;
	BN_CTX *c;

	c = BN_CTX_new();
	b = BN_new();
    BN_mul(b, bn, rm.bn, c);
	BN_copy(bn, b);
    BN_free(b);
    BN_CTX_free(c);
}
void CMint::mdiv(CMint &rm)
{
	BIGNUM *q, *r;
	BN_CTX *c;

	c = BN_CTX_new();
	r = BN_new();
	q = BN_new();
	BN_div(q, r, bn, rm.bn, c);
	BN_copy(bn, q);
	BN_free(q);
	BN_free(r);
	BN_CTX_free(c);
}
void CMint::msup(CMint &rm)
{
	BIGNUM *q, *r;
	BN_CTX *c;

	c = BN_CTX_new();
	r = BN_new();
	q = BN_new();
	BN_div(q, r, bn, rm.bn, c);
	BN_copy(bn, r);
	BN_free(q);
	BN_free(r);
	BN_CTX_free(c);
}
void CMint::pow(CMint &bm, CMint &em, CMint &mm)
{
	BIGNUM *b;
	BN_CTX *c;

	c = BN_CTX_new();
	b = BN_new();
	BN_mod_exp(b, bm.bn, em.bn, mm.bn, c);
 	BN_copy(bn, b);
	BN_free(b);
	BN_CTX_free(c);
}
void CMint::sdiv(short d, short *ro)
{
	CMint dm(d);
	BIGNUM *q, *r;
	BN_CTX *c;

	c = BN_CTX_new();
	q = BN_new();
	r = BN_new();
	BN_div(q, r, bn, dm.bn, c);
 	BN_copy(bn, q);

	char *s = BN_bn2hex(r);
	*ro = (short)strtol(s, NULL, 16);
	OPENSSL_free(s);

	BN_free(r);
	BN_free(q);
	BN_CTX_free(c);
}
void CMint::mtox(CStringA &tmp)
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
        "ENCRYPT", "NEW-ENVIRON", "TN3270", "41",
		"CHARSET", "43", "COMPORT", "45", "46", "KERMIT",
		"Unknow", 0
	};
static const char *slc_list[] = { 
		"0", "SYNCH", "BRK", "IP", "AO", "AYT", "EOR",
		"ABORT", "EOF", "SUSP", "EC", "EL", "EW", "RP",
		"LNEXT", "XON", "XOFF", "FORW1", "FORW2",
		"MCL", "MCR", "MCWL", "MCWR", "MCBOL",
		"MCEOL", "INSRT", "OVER", "ECR", "EWR",
		"EBOL", "EEOL"
	};
static const char *slc_flag[] = {
		"NOSUPPORT", "CANTCHANGE", "VARIABLE", "DEFAULT"
	};

static const struct _slc_init {
	BYTE	code;
	BYTE	flag;
	BYTE	ch;
	BYTE	tc;
} slc_init[] = {
	{	SLC_SYNCH,	SLC_DEFAULT,								0,		0	},
	{	SLC_IP,		SLC_VARIABLE | SLC_FLUSHIN | SLC_FLUSHOUT,	3,		TELC_IP		},
	{	SLC_AO,		SLC_VARIABLE,								15,		TELC_AO		},
	{	SLC_AYT,	SLC_VARIABLE,								20,		TELC_AYT	},
	{	SLC_ABORT,	SLC_VARIABLE | SLC_FLUSHIN | SLC_FLUSHOUT,	28,		TELC_ABORT	},
	{	SLC_EOF,	SLC_VARIABLE,								4,		TELC_EOF	},
	{	SLC_SUSP,	SLC_VARIABLE | SLC_FLUSHIN,					26,		TELC_SUSP	},
	{	SLC_EC,		SLC_VARIABLE,								8,		TELC_EC		},
	{	SLC_EL,		SLC_VARIABLE,								21,		TELC_EL		},
	{	SLC_EW,		SLC_VARIABLE,								23,		0	},
	{	SLC_RP,		SLC_VARIABLE,								18,		0	},
	{	SLC_LNEXT,	SLC_VARIABLE,								22,		0	},
	{	SLC_XON,	SLC_VARIABLE,								17,		0	},
	{	SLC_XOFF,	SLC_VARIABLE,								19,		0	},
	{	SLC_FORW1,	SLC_NOSUPPORT,								255,	0	},
	{	SLC_FORW2,	SLC_NOSUPPORT,								255,	0	},

	{	SLC_CR,		SLC_VARIABLE | SLC_FLUSHOUT,				13,		0	},	// Add CR
	{	SLC_LF,		SLC_VARIABLE | SLC_FLUSHOUT,				10,		0	},	// Add LF
	{	0,			0,											0,		0	}
};

CTelnet::CTelnet(class CRLoginDoc *pDoc):CExtSocket(pDoc)
{
	m_Type = ESCT_TELNET;
	ReceiveStatus = RVST_NON;
	m_KeepAliveTiimerId = 0;

	ZeroMemory(HisOpt, sizeof(HisOpt));
	ZeroMemory(MyOpt, sizeof(MyOpt));
	SubOptLen = 0;
	ZeroMemory(SubOptBuf, sizeof(SubOptBuf));
	ReceiveStatus = 0;
	m_KeepAliveTiimerId = 0;

	CpcOpt.flowcontrol = FALSE;
	CpcOpt.baudrate = 0;
	CpcOpt.datasize = 0;
	CpcOpt.parity = 0;
	CpcOpt.stopsize = 0;
	CpcOpt.control = 0;
	CpcOpt.linestate = 0;
	CpcOpt.modemstate = 0;
	CpcOpt.linemask = 0;
	CpcOpt.modemmask = 0;

	ZeroMemory(pka, sizeof(pka));
	ZeroMemory(ska, sizeof(ska));
	ZeroMemory(pkb, sizeof(pkb));
	ZeroMemory(&ddk, sizeof(ddk));
	ZeroMemory(&idk, sizeof(idk));
	PassSendFlag = 0;

	ZeroMemory(feed, sizeof(feed));
	ZeroMemory(&krb_key, sizeof(krb_key));
	ZeroMemory(&krb_sched, sizeof(krb_sched));
	ZeroMemory(&temp_feed, sizeof(temp_feed));
	temp_feed_init = 0;
	ZeroMemory(stream, sizeof(stream));
	EncryptInputFlag = 0;
	EncryptOutputFlag = 0;

	ZeroMemory(slc_tab, sizeof(slc_tab));
	slc_mode = 0;
}
CTelnet::~CTelnet()
{
	if ( m_KeepAliveTiimerId != 0 )
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this, m_KeepAliveTiimerId);
}
CFifoBase *CTelnet::FifoLinkMid()
{
	return new CFifoTelnet(m_pDocument, this);
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
	HisOpt[TELOPT_SGA].flags		|= TELFLAG_ACCEPT;
//	HisOpt[TELOPT_LFLOW].flags		|= TELFLAG_ACCEPT;

	MyOpt[TELOPT_BINARY].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_ECHO].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_SGA].flags			|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_TTYPE].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_NAWS].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_NEW_ENVIRON].flags	|= TELFLAG_ACCEPT;
//	MyOpt[TELOPT_LFLOW].flags		|= TELFLAG_ACCEPT;
//	MyOpt[TELOPT_COMPORT].flags		|= TELFLAG_ACCEPT;
	MyOpt[TELOPT_TSPEED].flags		|= TELFLAG_ACCEPT;

    SubOptLen = 0;
    ReceiveStatus = RVST_NON;

	srand((int)time(NULL));
	PassSendFlag = FALSE;

	memset(stream, 0, sizeof(stream));
	memset(&temp_feed, 0, sizeof(DesData));
	temp_feed_init = FALSE;
	EncryptInputFlag  = FALSE;
	EncryptOutputFlag = FALSE;

	for ( n = 0 ; n < SLC_MAX ; n++ )
		slc_tab[n].flag = slc_tab[n].ch = slc_tab[n].tc = (BYTE)0;

	for ( n = 0 ; slc_init[n].code != 0 ; n++ ) {
		slc_tab[slc_init[n].code].flag = slc_init[n].flag | SLC_DEFINE;
		slc_tab[slc_init[n].code].ch   = slc_init[n].ch;
		slc_tab[slc_init[n].code].tc   = slc_init[n].tc;
	}

	slc_mode = 0;
	m_SendBuff.Clear();

	CpcOpt.signature   = _T("RLogin Telnet");
	CpcOpt.baudrate    = 9600;
	CpcOpt.datasize    = 8;	// 8 Bits
	CpcOpt.parity      = 1;	// Parity NONE
	CpcOpt.stopsize    = 1;	// 1 Stop Bits
	CpcOpt.control     = 3;	// HARDWARE Flow
	CpcOpt.linestate   = 0;
	CpcOpt.modemstate  = 0;
	CpcOpt.linemask    = 0xFF;
	CpcOpt.modemmask   = 0xFF;
	CpcOpt.flowcontrol = FALSE;

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

	if ( !m_pDocument->m_TextRam.IsOptEnable(TO_RLTENLM) )
		MyOpt[TELOPT_LINEMODE].flags |= TELFLAG_ACCEPT;

	return TRUE;
}
void CTelnet::OnConnect()
{
	BOOL val = 1;
	CExtSocket::SetSockOpt(SO_OOBINLINE, &val, sizeof(BOOL), SOL_SOCKET);
    ReceiveStatus = RVST_DATA;

	SendOpt(TELC_DO, TELOPT_BINARY);
	HisOpt[TELOPT_BINARY].status = TELSTS_WANT_ON;

	SendOpt(TELC_WILL, TELOPT_BINARY);
	MyOpt[TELOPT_BINARY].status = TELSTS_WANT_ON;

	m_KeepAliveTiimerId = m_pFifoMid->DocMsgKeepAlive(this);

	CExtSocket::OnConnect();
}
void CTelnet::SockSend(char *buf, int len)
{
	if ( EncryptOutputFlag )
		EncryptEncode(buf, len);

	SendSocket(buf, len);
}
void CTelnet::SendFlush()
{
	if ( m_SendBuff.GetSize() <= 0 )
		return;

	SockSend((char *)m_SendBuff.GetPtr(), m_SendBuff.GetSize());
	m_SendBuff.Clear();
}
void CTelnet::OnRecvProtocol(void* lpBuf, int nBufLen, int nFlags)
{
	int n, i;
	char *buf = (char *)lpBuf;
	char tmp[4];

   if ( ReceiveStatus == RVST_NON )
	   return;

	if ( (MyOpt[TELOPT_LINEMODE].flags & TELFLAG_ON) != 0 ) {
		for ( n = 0 ; n < nBufLen ; n++ ) {
			for ( i = SLC_SYNCH ; i <= SLC_LF ; i++ ) {
				if ( (slc_tab[i].flag & 3) == SLC_VARIABLE && slc_tab[i].ch == buf[n] ) {
					if ( slc_tab[i].tc == 0 ) {
						if ( buf[n] == (char)TELC_IAC )
							m_SendBuff.Put8Bit(TELC_IAC);
						m_SendBuff.Put8Bit(buf[n]);
					}
					if ( (slc_tab[i].flag & SLC_FLUSHOUT) != 0 )
						SendFlush();
					if ( slc_tab[i].tc != 0 ) {
						tmp[0] = (char)TELC_IAC;
						tmp[1] = (char)slc_tab[i].tc;
						SockSend(tmp, 2);
					}
					break;
				}
			}
			if ( i > SLC_LF ) {
				if ( buf[n] == (char)TELC_IAC )
					m_SendBuff.Put8Bit(TELC_IAC);
				m_SendBuff.Put8Bit(buf[n]);
			}
		}
	} else {
		for ( n = 0 ; n < nBufLen ; n++ ) {
			if ( buf[n] == (char)TELC_IAC )
				m_SendBuff.Put8Bit(TELC_IAC);
			m_SendBuff.Put8Bit(buf[n]);
		}
		SendFlush();
	}
}
void CTelnet::SendBreak(int opt)
{
	m_pFifoMid->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDBREAK);
}
void CTelnet::FifoSendBreak(int opt)
{
	char tmp[4];
	
	if ( (MyOpt[TELOPT_LINEMODE].flags & TELFLAG_ON) != 0 ) {
		switch(opt) {
		case 0:		// ^C
			SendProtocol("\x03", 1, 0);
			break;
		case 1:		// ^Z
			SendProtocol("\x1A", 1, 0);
			break;
		}
	} else {
		tmp[0] = (char)TELC_IAC;
		tmp[1] = (char)TELC_BREAK;
		SockSend(tmp, 2);
	}
}
void CTelnet::OnTimer(UINT_PTR nIDEvent)
{
	if ( nIDEvent == m_KeepAliveTiimerId ) {
		char tmp[4];
		tmp[0] = (char)TELC_IAC;
		tmp[1] = (char)TELC_NOP;
		SockSend(tmp, 2);
	} else
		CExtSocket::OnTimer(nIDEvent);
}
void CTelnet::ResetOption()
{
	CExtSocket::ResetOption();

	if ( m_KeepAliveTiimerId != 0 ) {
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this, m_KeepAliveTiimerId);
		m_KeepAliveTiimerId = 0;
	}

	if ( m_pDocument->m_TextRam.IsOptEnable(TO_TELKEEPAL) && m_pDocument->m_TextRam.m_TelKeepAlive > 0 )
		m_KeepAliveTiimerId = ((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(m_pDocument->m_TextRam.m_TelKeepAlive * 1000, TIMEREVENT_SOCK | TIMEREVENT_INTERVAL, this);
}

void CTelnet::PrintOpt(int st, int ch, int opt)
{
	if ( opt < 0 || opt > TELOPT_MAX )
		opt = TELOPT_MAX;

	DEBUGLOG("%s %s %s",
		(st == 0 ? "SEND":"RECV"),
		(ch == TELC_DO   ? "DO" :
		(ch == TELC_DONT ? "DONT" :
		(ch == TELC_WONT ? "WONT" :
		(ch == TELC_WILL ? "WILL" :
		(ch == TELC_SB   ? "SB" :
		(ch == TELC_DM   ? "DM" : "???")))))),
		telopts[opt]);
}
void CTelnet::SendStr(LPCSTR str)
{
	int n = 0;
	char tmp[256];
	LPCSTR p = str;

	while ( *p != '\0' ) {
		if ( *p == (char)TELC_IAC )
			tmp[n++] = (char)TELC_IAC;
		tmp[n++] = *(p++);
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
void CTelnet::SendSlcOpt()
{
	int n;
	int len = 0;
	char tmp[SLC_MAX * 4 + 6];

	tmp[len++] = (char)TELC_IAC;
	tmp[len++] = (char)TELC_SB;
	tmp[len++] = (char)TELOPT_LINEMODE;
	tmp[len++] = (char)LM_SLC;

	for ( n = SLC_SYNCH ; n <= SLC_EEOL ; n++ ) {
		if ( slc_tab[n].flag == 0 )
			continue;
		slc_tab[n].flag &= ~SLC_ACK;
		tmp[len++] = (char)n;
		tmp[len++] = (char)slc_tab[n].flag;
		if ( (tmp[len++] = slc_tab[n].ch) == (char)TELC_IAC )
			tmp[len++] = (char)TELC_IAC;
	}

	tmp[len++] = (char)TELC_IAC;
	tmp[len++] = (char)TELC_SE;

	SockSend(tmp, len);
}
void CTelnet::SendWindSize()
{
	m_pFifoMid->SendCommand(FIFO_CMD_MSGQUEIN, FIFO_QCMD_SENDWINSIZE);
}
void CTelnet::FifoSendWindSize()
{
	int n = 0;
	int screen[4];
	char tmp[32];

	if ( ReceiveStatus == RVST_NON )
		return;

	if ( (MyOpt[TELOPT_NAWS].flags & TELFLAG_ON) == 0 )
		return;

	ASSERT(m_pFifoLeft != NULL);
	m_pFifoLeft->DocMsgIntPtr(DOCMSG_SCREENSIZE, screen);

	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SB;
	tmp[n++] = (char)TELOPT_NAWS;
	if ( (tmp[n++] = (char)(screen[0] >> 8)) == (char)TELC_IAC )
		tmp[n++] = (char)TELC_IAC;
	if ( (tmp[n++] = (char)(screen[0])) == (char)TELC_IAC )
		tmp[n++] = (char)TELC_IAC;
	if ( (tmp[n++] = (char)(screen[1] >> 8)) == (char)TELC_IAC )
		tmp[n++] = (char)TELC_IAC;
	if ( (tmp[n++] = (char)(screen[1])) == (char)TELC_IAC )
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
	static LPCTSTR optsts[] = { _T("OFF"), _T("ON"), _T("WOFF"), _T("WON"), _T("ROFF"), _T("RON") };

	CExtSocket::GetStatus(str);

	str += _T("\r\n");
	tmp.Format(_T("Telnet Status: %d\r\n"), ReceiveStatus);
	str += tmp;

	str += _T("\r\n");
	tmp.Format(_T("Server Option: "));
	str += tmp;

	for ( n = 0 ; n < TELOPT_MAX ; n++ ) {
		tmp.Format(_T(" %s:%s"), MbsToTstr(telopts[n]), optsts[HisOpt[n].status]);
		str += tmp;
	}
	str += _T("\r\n");

	str += _T("\r\n");
	tmp.Format(_T("Client Option: "));
	str += tmp;

	for ( n = 0 ; n < TELOPT_MAX ; n++ ) {
		tmp.Format(_T(" %s:%s"), MbsToTstr(telopts[n]), optsts[MyOpt[n].status]);
		str += tmp;
	}
	str += _T("\r\n");

	str += _T("\r\n");
	str += _T("Linemode SLC: ");
	for ( n = SLC_SYNCH ; n <= SLC_EEOL ; n++ ) {
		tmp.Format(_T("%s %s%s%s%s %d; "), MbsToTstr(slc_list[n]), MbsToTstr(slc_flag[slc_tab[n].flag & 3]),
			((slc_tab[n].flag & SLC_ACK) != 0 ? _T("|ACK") : _T("")),
			((slc_tab[n].flag & SLC_FLUSHIN) != 0 ? _T("|FLUSHIN") : _T("")),
			((slc_tab[n].flag & SLC_FLUSHOUT) != 0 ? _T("|FLUSHOUT") : _T("")),
			slc_tab[n].ch);
		str += tmp;
	}
	str += _T("\r\n");

	str += _T("Linemode Mode: ");
	if ( (slc_mode & MODE_EDIT)     != 0 ) str += _T("EDIT ");
	if ( (slc_mode & MODE_TRAPSIG)  != 0 ) str += _T("TRAPSIG ");
	if ( (slc_mode & MODE_ACK)      != 0 ) str += _T("ACK ");
	if ( (slc_mode & MODE_SOFT_TAB) != 0 ) str += _T("SOFT_TAB ");
	if ( (slc_mode & MODE_LIT_ECHO) != 0 ) str += _T("LIT_ECHO ");
	str += _T("\r\n");

	if ( EncryptOutputFlag || EncryptInputFlag )
		str += _T("\r\n");

	if ( EncryptOutputFlag ) {
		tmp.Format(_T("EncryptOut: %s\r\n"), (stream[DIR_ENC].mode == ENCTYPE_DES_CFB64 ? _T("des-cfb") : _T("des-ofb")));
		str += tmp;
	}

	if ( EncryptInputFlag ) {
		tmp.Format(_T("EncryptIn: %s\r\n"), (stream[DIR_DEC].mode == ENCTYPE_DES_CFB64 ? _T("des-cfb") : _T("des-ofb")));
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
			if ( opt == TELOPT_LINEMODE ) {
				SendFlush();
				ASSERT(m_pFifoLeft != NULL);
				m_pFifoLeft->DocMsgLineMode(0);	// LineMode off
			}
			break;
		case TELSTS_ON:
			tab[opt].flags |= TELFLAG_ON;
			if ( opt == TELOPT_NAWS )
				FifoSendWindSize();
			else if ( opt == TELOPT_ENCRYPT )
				EncryptSendSupport();
			else if ( opt == TELOPT_LINEMODE ) {
				SendSlcOpt();
				ASSERT(m_pFifoLeft != NULL);
				m_pFifoLeft->DocMsgLineMode(1);	// LineMode on
			} else if ( opt == TELOPT_COMPORT ) {
				SendCpcValue(CPC_CTS_SET_BAUDRATE, CpcOpt.baudrate);
				SendCpcValue(CPC_CTS_SET_DATASIZE, CpcOpt.datasize);
				SendCpcValue(CPC_CTS_SET_PARITY,   CpcOpt.parity);
				SendCpcValue(CPC_CTS_SET_STOPSIZE, CpcOpt.stopsize);
				SendCpcValue(CPC_CTS_SET_CONTROL,  CpcOpt.control);
			}
			break;
		}
		break;
	}
}
void CTelnet::SubOptFunc(char *buf, int len)
{
	int n, i, v;
	int ptr = 0;
	char tmp[256];
	CStringIndex env;
	CStringA mbs;
	CString work;

	ASSERT(m_pFifoLeft != NULL);

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
		SendStr(m_pFifoLeft->DocMsgStrA(DOCMSG_TERMNAME, mbs));
		n = 0;
		tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)TELC_SE;
		SockSend(tmp, n);
		PrintOpt(0, TELC_SB, TELOPT_TTYPE);
		break;

	case TELOPT_TSPEED:
        if ( (MyOpt[TELOPT_TSPEED].flags & TELFLAG_ON) == 0 )
            break;
        if ( SB_GETC() != TELQUAL_SEND )
			break;
		n = 0;
		tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)TELC_SB;
		tmp[n++] = (char)TELOPT_TSPEED;
		tmp[n++] = (char)TELQUAL_IS;
		SockSend(tmp, n);
		{
			int ispeed = m_pFifoLeft->DocMsgTtyMode(128, 9600);	// ISPEED
			int ospeed = m_pFifoLeft->DocMsgTtyMode(129, 9600);	// OSPEED;
			CStringA str;

			str.Format("%d,%d", ispeed, ospeed);;
			SendStr(str);
		}
		n = 0;
		tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)TELC_SE;
		SockSend(tmp, n);
		PrintOpt(0, TELC_SB, TELOPT_TSPEED);
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

		env.GetString(m_pFifoLeft->DocMsgStr(DOCMSG_ENVSTR, work));
		env[_T("USER")] = m_pFifoLeft->DocMsgStr(DOCMSG_USERNAME, work);
		for ( i = 0 ; i < env.GetSize() ; i++ ) {
			if ( env[i].m_Value == 0 || env[i].m_nIndex.IsEmpty() || env[i].m_String.IsEmpty() )
				continue;
			n = 0;
			tmp[n++] = (char)ENV_VAR;
			SockSend(tmp, n);
			SendStr(m_pFifoLeft->DocMsgRemoteStr(env[i].m_nIndex, mbs));
			n = 0;
			tmp[n++] = (char)ENV_VALUE;
			SockSend(tmp, n);
			SendStr(m_pFifoLeft->DocMsgRemoteStr(env[i].m_String, mbs));
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

	case TELOPT_LINEMODE:
		switch(SB_GETC()) {
		case LM_SLC:
			while ( (n = SB_GETC()) != EOF ) {
				if ( n >= SLC_SYNCH && n <= SLC_EEOL && (i = SB_GETC()) != EOF && (v = SB_GETC()) != EOF ) {
					slc_tab[n].flag = i;
					slc_tab[n].ch   = v;
				}
			}
			break;
		case LM_MODE:
			if ( (n = SB_GETC()) != EOF )
				slc_mode = n;
			break;
		}
		break;

	case TELOPT_COMPORT:
		switch(SB_GETC()) {
		case CPC_STC_SIGNATURE:			// IAC SB COM-PORT-OPTION SIGNATURE <text> IAC SE
			// This command may be sent by either the client or the access
			// server to exchange signature information.
			CpcOpt.signature.Empty();
			while ( (n = SB_GETC()) != EOF )
				CpcOpt.signature += (CHAR)n;
			break;

		case CPC_STC_SET_BAUDRATE:		// IAC SB COM-PORT-OPTION SET-BAUD <value(4)> IAC SE
			// 4 Byte (network standard format) 0=current baudrate
			if ( (int)(ptr + sizeof(unsigned long)) < len )
				CpcOpt.baudrate = ntohl(*((unsigned long *)(buf + ptr)));
			break;

		case CPC_STC_SET_DATASIZE:		// IAC SB COM-PORT-OPTION SET-DATASIZE <value> IAC SE
			// Value	Data Bit Size
			// 0		Request Current Data Bit Size
			// 5		5
			// 6		6
			// 7		7
			// 8		8
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.datasize = (BYTE)n;
			 break;

		case CPC_STC_SET_PARITY:		// IAC SB COM-PORT-OPTION SET-PARITY <value> IAC SE
			// Value	Parity
			// 0		Request Current Data Size
			// 1		NONE
			// 2		ODD
			// 3		EVEN
			// 4		MARK
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.parity = (BYTE)n;
			break;

		case CPC_STC_SET_STOPSIZE:		// IAC SB COM-PORT-OPTION SET-STOPSIZE <value> IAC SE
			// Value	Stop Bit Size
			// 0		Request Current Data Size
			// 1		1
			// 2		2
			// 3		1.5
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.stopsize = (BYTE)n;
			 break;

		case CPC_STC_SET_CONTROL:		// IAC SB COM-PORT-OPTION SET-CONTROL <value> IAC SE
            //  1 Use No Flow Control (outbound/both)
            //  2 Use XON/XOFF Flow Control (outbound/both)
            //  3 Use HARDWARE Flow Control (outbound/both)
            //  4 Request BREAK State
            //  5 Set BREAK State ON
            //  6 Set BREAK State OFF
            //  7 Request DTR Signal State
            //  8 Set DTR Signal State ON
            //  9 Set DTR Signal State OFF
            // 10 Request RTS Signal State
            // 11 Set RTS Signal State ON
            // 12 Set RTS Signal State OFF
            // 13 Request Com Port Flow Control Setting (inbound)
            // 14 Use No Flow Control (inbound)
            // 15 Use XON/XOFF Flow Control (inbound)
            // 16 Use HARDWARE Flow Control (inbound)
            // 17 Use DCD Flow Control (outbound/both)
            // 18 Use DTR Flow Control (inbound)
            // 19 Use DSR Flow Control (outbound/both)
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.control = (BYTE)n;
			break;

		case CPC_STC_NOTIFY_LINESTATE:		// IAC SB COM-PORT-OPTION NOTIFY-LINESTATE <value> IAC SE
			// Bit Position
            // 7 Time-out Error
            // 6 Transfer Shift Register Empty
            // 5 Transfer Holding Register Empty
            // 4 Break-detect Error
            // 3 Framing Error
            // 2 Parity Error
            // 1 Overrun Error
            // 0 Data Ready
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.linestate = (BYTE)n;
			break;

		case CPC_STC_NOTIFY_MODEMSTATE:		// IAC SB COM-PORT-OPTION NOTIFY-MODEMSTATE <value> IAC SE
			// Bit Position
			// 7 Receive Line Signal Detect (also known as Carrier Detect)
			// 6 Ring Indicator
			// 5 Data-Set-Ready Signal State
			// 4 Clear-To-Send Signal State
			// 3 Delta Receive Line Signal Detect
			// 2 Trailing-edge Ring Detector
			// 1 Delta Data-Set-Ready
			// 0 Delta Clear-To-Send
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.modemstate = (BYTE)n;
			break;

		case CPC_STC_FLOWCONTROL_SUSPEND:	// IAC SB COM-PORT-OPTION FLOWCONTROL-SUSPEND IAC SE 
			// The sender of this command is requesting that the receiver
			// suspend transmission of both data and commands until the
			// FLOWCONTROL-RESUME is transmitted by the sender.
			CpcOpt.flowcontrol = TRUE;
			break;

		case CPC_STC_FLOWCONTROL_RESUME:	// IAC SB COM-PORT-OPTION FLOWCONTROL-RESUME IAC SE
			// The sender of this command is requesting that the receiver resume
			// transmission of both data and commands.
			CpcOpt.flowcontrol = FALSE;
			break;

		case CPC_STC_SET_LINESTATE_MASK:	// IAC SB COM-PORT-OPTION SET-LINESTATE-MASK <value> IAC SE
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.linemask = (BYTE)n;
			break;

		case CPC_STC_SET_MODEMSTATE_MASK:	// IAC SB COM-PORT-OPTION SET-MODEMSTATE-MASK <value> IAC SE
			if ( (n = SB_GETC()) != EOF )
				CpcOpt.modemmask = (BYTE)n;
			break;

		case CPC_STC_PURGE_DATA:			// IAC SB COM-PORT-OPTION PURGE-DATA <value> IAC SE
			// 1 Purge access server receive data buffer
			// 2 Purge access server transmit data buffer
			// 3 Purge both the access server receive data buffer and the access server transmit data buffer
			break;
		}
		break;
	}
}
void CTelnet::OnRecvSocket(void* lpBuf, int nBufLen, int nFlags)
{
    int c;
    int n = 0;
	int len = nBufLen;
    char *tmp, *buf;
	int DoDecode = FALSE;

	if ( nFlags != 0 || ReceiveStatus == RVST_NON )
		return;

	tmp = buf = (char *)lpBuf;

	while ( len-- > 0 ) {
		if ( EncryptInputFlag && !DoDecode ) {
			EncryptDecode(buf, len + 1);
			DoDecode = TRUE;
		}
		c = (unsigned char)(*(buf++));
		switch(ReceiveStatus) {
		case RVST_DATA:
			if ( c == TELC_IAC )
				ReceiveStatus = RVST_IAC;
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
				ReceiveStatus = RVST_DATA;
				break;
			case TELC_DONT:
				ReceiveStatus = RVST_DONT;
				break;
			case TELC_DO:
				ReceiveStatus = RVST_DO;
				break;
			case TELC_WONT:
				ReceiveStatus = RVST_WONT;
				break;
			case TELC_WILL:
				ReceiveStatus = RVST_WILL;
				break;
			case TELC_SB:
				ReceiveStatus = RVST_SB;
				break;
			case TELC_DM:
				PrintOpt(1, c, TELOPT_NON);
				ReceiveStatus = RVST_DATA;
				break;
			case TELC_NOP:
			case TELC_GA:
				PrintOpt(1, c, TELOPT_NON);
				ReceiveStatus = RVST_DATA;
				break;
			default:
				ReceiveStatus = RVST_DATA;
				break;
			}
			break;

		case RVST_DONT:
			PrintOpt(1, TELC_DONT, c);
			OptFunc(MyOpt, c, DO_OFF, TELC_WONT);
			ReceiveStatus = RVST_DATA;
			break;
		case RVST_DO:
			PrintOpt(1, TELC_DO, c);
			OptFunc(MyOpt, c, DO_ON, TELC_WONT);
			ReceiveStatus = RVST_DATA;
			break;
		case RVST_WONT:
			PrintOpt(1, TELC_WONT, c);
			OptFunc(HisOpt, c, DO_OFF, TELC_DONT);
			ReceiveStatus = RVST_DATA;
			break;
		case RVST_WILL:
			PrintOpt(1, TELC_WILL, c);
			OptFunc(HisOpt, c, DO_ON, TELC_DONT);
			ReceiveStatus = RVST_DATA;
			break;

		case RVST_SB:
			if ( c == TELC_IAC )
				ReceiveStatus = RVST_SE;
			else if ( SubOptLen < SUBOPTLEN )
				SubOptBuf[SubOptLen++] = c;
			break;
		case RVST_SE:
			switch(c) {
			case TELC_IAC:
	    		if ( SubOptLen < SUBOPTLEN )
					SubOptBuf[SubOptLen++] = c;
				ReceiveStatus = RVST_SB;
				break;
			case TELC_SE:
				SubOptFunc(SubOptBuf, SubOptLen);
				SubOptLen = 0;
				ReceiveStatus = RVST_DATA;
				break;
			default:
				SubOptFunc(SubOptBuf, SubOptLen);
				SubOptLen = 0;
				ReceiveStatus = RVST_IAC;
				goto PROC_IAC;
			}
			break;
		}
	}

	SendDocument(tmp, n, 0);
}

void CTelnet::SendCpcValue(int cmd, int value)
{
	int n = 0;
	int i;
	char tmp[32], *p;

	if ( (MyOpt[TELOPT_COMPORT].flags & TELFLAG_ON) == 0 )
		return;

	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SB;
	tmp[n++] = (char)TELOPT_COMPORT;
	tmp[n++] = (char)cmd;

	if ( cmd == CPC_CTS_SET_BAUDRATE ) {
		value = htonl(value);
		p = (char *)(&value);
		for ( i = 0 ; i < sizeof(long) ; i++ ) {
			if ( *p == (char)TELC_IAC )
				tmp[n++] = (char)TELC_IAC;
			tmp[n++] = *(p++);
		}
	} else {
		if ( value == TELC_IAC )
			tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)value;
	}

	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SE;

	SockSend(tmp, n);
	PrintOpt(0, TELC_SB, TELOPT_COMPORT);
}
void CTelnet::SendCpcStr(int cmd, LPCTSTR str)
{
	int n = 0;
	char tmp[1024];

	if ( (MyOpt[TELOPT_COMPORT].flags & TELFLAG_ON) == 0 )
		return;

	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SB;
	tmp[n++] = (char)TELOPT_COMPORT;
	tmp[n++] = (char)cmd;

	while ( *str != '\0' && n < 1020 ) {
		if ( *str == (char)TELC_IAC )
			tmp[n++] = (char)TELC_IAC;
		tmp[n++] = (char)*(str++);
	}

	tmp[n++] = (char)TELC_IAC;
	tmp[n++] = (char)TELC_SE;

	SockSend(tmp, n);
	PrintOpt(0, TELC_SB, TELOPT_COMPORT);
}
void CTelnet::SendAuthOpt(int n1, int n2, int n3, void *buf, int len)
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

void CTelnet::AuthSend(char *buf, int len)
{
	static const char nosup[] = { (char)TELC_IAC, (char)TELC_SB, (char)TELOPT_AUTHENTICATION, (char)TELQUAL_IS, 
							(char)AUTHTYPE_NULL, (char)0, (char)TELC_IAC, (char)TELC_SE };

	while ( len >= 2 ) {
		switch(buf[0]) {
		case AUTHTYPE_SRA:
			SraGenkey(pka, ska);
			SendAuthOpt(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_KEY, pka, HEXKEYBYTES);
			return;
		}
		len -= 2;
		buf += 2;
	}

	SockSend((char *)nosup, sizeof(nosup));
}

void CTelnet::AuthReply(char *buf, int len)
{
	int n, a, b;
	char tmp[256];
	CStringA work;

	a = buf[0];
	b = buf[2];
	len -= 3;
	buf += 3;

	if ( len < 0 )
		return;

	ASSERT(m_pFifoLeft != NULL);

	switch(a) {
	case AUTHTYPE_SRA:
		switch(b) {
		case SRA_KEY:
			if ( len < HEXKEYBYTES )
				break;
			memcpy(pkb, buf, HEXKEYBYTES);
			pkb[HEXKEYBYTES] = '\0';

			SraCommonKey(ska, pkb);

			m_pFifoLeft->DocMsgStrA(DOCMSG_USERNAME, work);
			if ( (n = work.GetLength()) > 250 )
				n = 250;
			strncpy(tmp, work, n);
			tmp[n] = '\0';

			n = SraEncode(tmp, n, &ddk);
			SendAuthOpt(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_USER, tmp, n);
			PassSendFlag = FALSE;
			break;

		case SRA_CONTINUE:
			if ( PassSendFlag ) {
				SendAuthOpt(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_REJECT, NULL, 0);
				break;
			}

			m_pFifoLeft->DocMsgStrA(DOCMSG_PASSNAME, work);
			if ( (n = work.GetLength()) > 250 )
				n = 250;
			strncpy(tmp, work, n);
			tmp[n] = '\0';

			n = SraEncode(tmp, n, &ddk);
			SendAuthOpt(AUTHTYPE_SRA, AUTH_WHO_CLIENT | AUTH_HOW_ONE_WAY, SRA_PASS, tmp, n);
			PassSendFlag = TRUE;
			break;

		case SRA_ACCEPT:
			EncryptSessionKey();
			break;
		}
		break;
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

	CStringA tmp;

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
	static const char hextab[17] = "0123456789ABCDEF";

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
}
void CTelnet::EncryptStart(char *p, int len)
{
	if ( stream[DIR_DEC].status == STM_HAVE_OK )
		EncryptInputFlag = TRUE;

	EncryptSendTempFeed();
}
void CTelnet::EncryptEnd()
{
	EncryptInputFlag = FALSE;
}
void CTelnet::EncryptRequestStart()
{
	EncryptSendStart();
}
void CTelnet::EncryptRequestEnd()
{
	int i = 0;

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
