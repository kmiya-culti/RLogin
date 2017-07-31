// Telnet.h: CTelnet クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TELNET_H__ECB045AA_51FC_4815_A70B_04DA0D152E54__INCLUDED_)
#define AFX_TELNET_H__ECB045AA_51FC_4815_A70B_04DA0D152E54__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ExtSocket.h"
#include "Data.h"

#define	TELC_IAC	255		/* interpret as command: */
#define	TELC_DONT	254		/* you are not to use option */
#define	TELC_DO		253		/* please, you use option */
#define	TELC_WONT	252		/* I won't use option */
#define	TELC_WILL	251		/* I will use option */
#define	TELC_SB		250		/* interpret as subnegotiation */
#define	TELC_GA		249		/* you may reverse the line */
#define	TELC_EL		248		/* erase the current line */
#define	TELC_EC		247		/* erase the current character */
#define	TELC_AYT	246		/* are you there */
#define	TELC_AO		245		/* abort output--but let prog finish */
#define	TELC_IP		244		/* interrupt process--permanently */
#define	TELC_BREAK	243		/* break */
#define	TELC_DM		242		/* data mark--for connect. cleaning */
#define	TELC_NOP	241		/* nop */
#define	TELC_SE		240		/* end sub negotiation */
#define TELC_EOR 	239		/* end of record (transparent mode) */
#define	TELC_ABORT	238		/* Abort process */
#define	TELC_SUSP	237		/* Suspend process */
#define	TELC_EOF	236		/* End of file: EOF is already used... */

#define TELOPT_BINARY	0	/* 8-bit data path */
#define TELOPT_ECHO		1	/* echo */
#define	TELOPT_RCP		2	/* prepare to reconnect */
#define	TELOPT_SGA		3	/* suppress go ahead */
#define	TELOPT_NAMS		4	/* approximate message size */
#define	TELOPT_STATUS	5	/* give status */
#define	TELOPT_TM		6	/* timing mark */
#define	TELOPT_RCTE		7	/* remote controlled transmission and echo */
#define TELOPT_NAOL 	8	/* negotiate about output line width */
#define TELOPT_NAOP 	9	/* negotiate about output page size */
#define TELOPT_NAOCRD	10	/* negotiate about CR disposition */
#define TELOPT_NAOHTS	11	/* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD	12	/* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD	13	/* negotiate about formfeed disposition */
#define TELOPT_NAOVTS	14	/* negotiate about vertical tab stops */
#define TELOPT_NAOVTD	15	/* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD	16	/* negotiate about output LF disposition */
#define TELOPT_XASCII	17	/* extended ascic character set */
#define	TELOPT_LOGOUT	18	/* force logout */
#define	TELOPT_BM		19	/* byte macro */
#define	TELOPT_DET		20	/* data entry terminal */
#define	TELOPT_SUPDUP	21	/* supdup protocol */
#define	TELOPT_SUPDUPOUTPUT 22	/* supdup output */
#define	TELOPT_SNDLOC	23	/* send location */
#define	TELOPT_TTYPE	24	/* terminal type */
#define	TELOPT_EOR		25	/* end or record */
#define	TELOPT_TUID		26	/* TACACS user identification */
#define	TELOPT_OUTMRK	27	/* output marking */
#define	TELOPT_TTYLOC	28	/* terminal location number */
#define	TELOPT_3270REGIME 29	/* 3270 regime */
#define	TELOPT_X3PAD	30	/* X.3 PAD */
#define	TELOPT_NAWS		31	/* window size */
#define	TELOPT_TSPEED	32	/* terminal speed */
#define	TELOPT_LFLOW	33	/* remote flow control */
#define TELOPT_LINEMODE	34	/* Linemode option */
#define TELOPT_XDISPLOC	35	/* X Display Location */
#define TELOPT_OLD_ENVIRON 36	/* Old - Environment variables */
#define	TELOPT_AUTHENTICATION 37/* Authenticate */
#define	TELOPT_ENCRYPT	38	/* Encryption option */
#define TELOPT_NEW_ENVIRON 39	/* New - Environment variables */
#define TELOPT_NON		40
#define	TELOPT_EXOPL	255	/* extended-options-list */

/* sub-option qualifiers */
#define	TELQUAL_IS		0	/* option is... */
#define	TELQUAL_SEND	1	/* send option */
#define	TELQUAL_INFO	2	/* ENVIRON: informational version of IS */
#define	TELQUAL_REPLY	2	/* AUTHENTICATION: client version of IS */
#define	TELQUAL_NAME	3	/* AUTHENTICATION: client version of IS */

#define	ENV_VAR			0
#define	ENV_VALUE		1
#define	ENV_ESC			2
#define	ENV_USERVAR		3

#define	SUBOPTLEN	255
#define	TELOPT_MAX	(TELOPT_NEW_ENVIRON + 1)

/* option status */
#define	TELSTS_OFF	0
#define	TELSTS_ON	1
#define	TELSTS_WANT_OFF	2
#define	TELSTS_WANT_ON	3
#define	TELSTS_REVS_OFF	4
#define	TELSTS_REVS_ON	5

#define	TELFLAG_ACCEPT	001
#define	TELFLAG_ON		002

#define	RVST_DATA	0
#define	RVST_IAC	1
#define	RVST_DONT	2
#define	RVST_DO		3
#define	RVST_WONT	4
#define	RVST_WILL	5
#define	RVST_SB		6
#define	RVST_SE		7
#define	RVST_NON	8

#define	DO_OFF		0
#define	DO_ON		1
#define	SW_OFF		2
#define	SW_ON		3

#define	TELDONT(c)	(c)
#define	TELDO(c)	(c - 1)

#define AUTH_WHO_CLIENT         0       /* Client authenticating server */
#define AUTH_WHO_SERVER         1       /* Server authenticating client */

#define AUTH_HOW_ONE_WAY        0
#define AUTH_HOW_MUTUAL         2

#define AUTHTYPE_NULL           0
#define AUTHTYPE_KERBEROS_V4    1
#define AUTHTYPE_KERBEROS_V5    2
#define AUTHTYPE_SPX            3
#define AUTHTYPE_MINK           4
#define AUTHTYPE_NSA            5
#define AUTHTYPE_SRA            6
#define AUTHTYPE_CNT            7

#define SRA_KEY			0
#define SRA_USER		1
#define SRA_CONTINUE	2
#define SRA_PASS		3
#define SRA_ACCEPT		4
#define SRA_REJECT		5

#define	HEXKEYBYTES		48

#define NSA_KEY			0
#define NSA_USER		1
#define NSA_CONTINUE	2
#define NSA_PASS		3
#define NSA_ACCEPT		4
#define NSA_REJECT		5

#define	NSAKEYBYTES		8

#define ENCRYPT_IS              0       /* I pick encryption type ... */
#define ENCRYPT_SUPPORT         1       /* I support encryption types ... */
#define ENCRYPT_REPLY           2       /* Initial setup response */
#define ENCRYPT_START           3       /* Am starting to send encrypted */
#define ENCRYPT_END             4       /* Am ending encrypted */
#define ENCRYPT_REQSTART        5       /* Request you start encrypting */
#define ENCRYPT_REQEND          6       /* Request you end encrypting */
#define ENCRYPT_ENC_KEYID       7
#define ENCRYPT_DEC_KEYID       8

#define ENCTYPE_ANY             0
#define ENCTYPE_DES_CFB64       1
#define ENCTYPE_DES_OFB64       2

#define FB64_IV         1
#define FB64_IV_OK      2
#define FB64_IV_BAD     3

#define	DIR_DEC			0
#define	DIR_ENC			1

#define	STM_HAVE_KEYID	001
#define	STM_HAVE_IKEY	002
#define	STM_HAVE_IV		004
#define	STM_HAVE_OK		(STM_HAVE_KEYID | STM_HAVE_IKEY | STM_HAVE_IV)

#define LM_MODE			1
#define LM_FORWARDMASK	2
#define LM_SLC			3

#define SLC_SYNCH		1
#define SLC_BRK 		2
#define SLC_IP			3
#define SLC_AO			4
#define SLC_AYT			5
#define SLC_EOR 		6
#define SLC_ABORT		7
#define SLC_EOF 		8
#define SLC_SUSP		9
#define SLC_EC			10
#define SLC_EL			11
#define SLC_EW			12
#define SLC_RP			13
#define SLC_LNEXT		14
#define SLC_XON 		15
#define SLC_XOFF		16
#define SLC_FORW1		17
#define SLC_FORW2		18
#define SLC_MCL 		19
#define SLC_MCR			20
#define SLC_MCWL		21
#define SLC_MCWR		22
#define SLC_MCBOL		23
#define SLC_MCEOL		24
#define SLC_INSRT		25
#define SLC_OVER		26
#define SLC_ECR 		27
#define SLC_EWR 		28
#define SLC_EBOL		29
#define SLC_EEOL		30

#define SLC_CR			31		// XXX Add CR Key
#define SLC_LF			32		// XXX Add LF Key
#define	SLC_MAX			33

#define SLC_NOSUPPORT	0
#define SLC_CANTCHANGE	1
#define SLC_VARIABLE	2
#define SLC_DEFAULT		3

#define SLC_ACK 		0x80
#define SLC_FLUSHIN		0x40
#define SLC_FLUSHOUT	0x20

#define	SLC_DEFINE		0x100

#define MODE_EDIT		0x01
#define MODE_TRAPSIG	0x02
#define MODE_ACK		0x04
#define MODE_SOFT_TAB	0x08
#define MODE_LIT_ECHO	0x10

#include "openssl/des.h"
#include "openssl/bn.h"
#include "openssl/crypto.h"

typedef unsigned char DesData[8], IdeaData[16];

class CMint : public CObject
{
public:
	BIGNUM *bn;

	void madd(CMint &rm);
	void mult(CMint &rm);
	void mdiv(CMint &rm);
	void msup(CMint &rm);
	void pow(CMint &bm, CMint &em, CMint &mm);
	void sdiv(short d, short *ro);
	void mtox(CString &tmp);
	void Deskey(DesData *key);
	void Ideakey(IdeaData *key);
	void Debug();

	const CMint & operator = (CMint &rm) { BN_copy(bn, rm.bn); return *this; }

	CMint();
	CMint(int n);
	CMint(LPCSTR s);
	~CMint();
};

class CTelnet : public CExtSocket  
{
public:
	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	void OnConnect();
	void OnReciveCallBack(void *lpBuf, int nBufLen, int nFlags);
	int Send(const void *lpBuf, int nBufLen, int nFlags = 0);
	void SendBreak(int opt = 0);
	void SendWindSize(int x, int y);
	void SetXonXoff(int sw);
	void GetStatus(CString &str);

	CTelnet(class CRLoginDoc *pDoc);
	virtual ~CTelnet();

private:
	struct	TelOptTab {
		char	status;
		char	flags;
	} HisOpt[TELOPT_MAX], MyOpt[TELOPT_MAX];
	int SubOptLen;
	char SubOptBuf[SUBOPTLEN + 1];
	int ReciveStatus;

	void SockSend(char *buf, int len);
	void SendFlush();
	void SendStr(LPCTSTR str);
	void SendOpt(int ch, int opt);
	void SendSlcOpt();
	void OptFunc(struct TelOptTab *tab, int opt, int sw, int ch);
	void SubOptFunc(char *buf, int len);
	void PrintOpt(int st, int ch, int opt);

	char pka[128], ska[128], pkb[128];
	DesData ddk;
	IdeaData idk;
	int PassSendFlag;

	void SendOptData(int n1, int n2, int n3, void *buf, int len);
	void AuthSend(char *buf, int len);
	void AuthReply(char *buf, int len);

	void SraGenkey(char *p, char *s);
	void SraCommonKey(char *s, char *p);
	int SraEncode(char *p, int len, DesData *key);
	int SraDecode(char *p, int len, DesData *key);

	void NsaGenkey(char *p);
	void NsaEncode(char *p, int len, char *key);
	void NsaDecode(char *p, int len, char *key);

	char feed[128];
	DesData krb_key;
	des_key_schedule krb_sched;
	DesData temp_feed;
	int temp_feed_init;
	struct _stinfo {
		DesData		output;
		DesData		feed;
		DesData		iv;
		DesData		ikey;
		des_key_schedule sched;
		int		index;
		char	keyid[8];
		int		keylen;
		int		status;
		int		mode;
	} stream[2];
	int EncryptInputFlag;
	int EncryptOutputFlag;

	struct _slc {
		WORD	flag;
		BYTE	ch;
		BYTE	tc;
	} slc_tab[SLC_MAX];
	int slc_mode;

	void EncryptSendSupport();
	void EncryptSessionKey();
	void EncryptIs(char *p, int len);
	void EncryptReply(char *p, int len);
	void EncryptKeyid(int dir, char *p, int len);
	void EncryptSendRequestStart();
	void EncryptSendTempFeed();
	void EncryptSendStart();
	void EncryptStart(char *p, int len);
	void EncryptEnd();
	void EncryptRequestStart();
	void EncryptRequestEnd();
	void EncryptDecode(char *p, int len);
	void EncryptEncode(char *p, int len);
	void EncryptStatus();
};

#endif // !defined(AFX_TELNET_H__ECB045AA_51FC_4815_A70B_04DA0D152E54__INCLUDED_)
