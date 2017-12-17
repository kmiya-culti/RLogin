#pragma once

#include "SyncSock.h"

#define	KMT_CMARK		0x01
#define	KMT_DATAMAX		4000
#define	KMT_PKTMAX		(KMT_DATAMAX + 32)
#define	KMT_PKTQUE		16

#define	KMT_TIMEOUT		't'
#define	KMT_CANCEL		'c'
#define	KMT_SUMERR		's'

#define	KMT_CAP_CONTINE	001		// 0000 0001
#define	KMT_CAP_LONGPKT	002		// 0000 0010	Long Packet capability
#define	KMT_CAP_SLIDWIN	004		// 0000 0100	Sliding Window capability
#define	KMT_CAP_FILATTR	010		// 0000 1000	Attribute capability
#define	KMT_CAP_RESEND	020		// 0001 0000	RESEND capability
#define	KMT_CAP_LOCKSFT	040		// 0010 0000	Locking Shift capability

#define	KMT_ATTR_TIME	001
#define	KMT_ATTR_MODE	002
#define	KMT_ATTR_SIZE	003
#define	KMT_ATTR_TYPE	004

class CKermit :	public CSyncSock
{
public:
	int	m_Stage;
	int m_SendSeq;
	int m_RecvSeq;

	BYTE m_MarkCh;		/* Packet Start character def='\1' */
	BYTE m_EolCh;		/* End-Of-Line character def='\015' */
	BYTE m_PadCh;		/* Padding character def=0 */
	BYTE m_CtlCh;		/* Control character prefix def='#' */
	BYTE m_EbitCh;		/* 8th bit character prefix def='&' */
	BYTE m_RepCh;		/* Repeat character prefix def='~' */
	BYTE m_HisEbitCh;	/* 8th bit character prefix def='&' */
	BYTE m_HisRepCh;	/* Repeat character prefix def='~' */

	BYTE m_MyEolCh;		/* End-Of-Line character def='\015' */
	BYTE m_MyPadCh;		/* Padding character def=0 */
	BYTE m_MyCtlCh;		/* Control character prefix def='#' */
	int m_MyPadLen;		/* How much padding def=0 */

	int m_TimeOut;		/* When I want to be timed out def=7 */
	int m_PadLen;		/* How much padding def=0 */
	int	m_WindMax;		/* Window size def=1 */
	int m_PktMax;		/* Maximum Packet Size def=94 */
	int m_Caps;			/* 2=LP 8=Attr */
	int m_ChkType;		/* Block check type def=3 */

	int m_ChkReq;
	int m_RepOld;
	int m_RepLen;

	BOOL m_FileType;
	int m_AttrFlag;
	time_t m_FileTime;
	int m_FileMode;
	LONGLONG m_FileSize;

	LONGLONG m_TranSize;

	int m_PktPos;
	int m_PktCnt;

	struct KmtPkt {
		int		Type;
		int		Seq;
		int		Len;
		BYTE	Buf[KMT_PKTMAX];
	} m_InPkt, m_OutPkt[KMT_PKTQUE];

	int m_Size;
	BYTE *m_pData;
	CBuffer	m_Work;

	void OnProc(int cmd);

	inline BYTE ToChar(int ch) { return (BYTE)(ch + '\040'); }
	inline int UnChar(BYTE ch) { return (ch - '\040'); }
	inline BYTE ToCtrl(int ch) { return (BYTE)(ch ^ 64); }
	inline int IncSeq(int seq) { return ((seq + 1) & 63); }

	void Init();
	void DownLoad();
	void UpLoad(BOOL ascii);

	void SimpleUpLoad();
	void SimpleDownLoad();

	int ChkSumType1(BYTE *p, int len);
	int ChkSumType2(BYTE *p, int len);
	int ChkSumType3(BYTE *p, int len);
	time_t TimeStamp(LPCSTR p);

	int ReadPacket();
	BOOL DecodePacket();

	int SizePacket();
	int MakePacket(int type, int seq, BYTE *buf, int len);
	int PktPos(int pos);
	void SendPacket(int pos);

	void EncodeChar(int ch);
	void EncodeInit();
	void EncodeData(int ch);
	void EncodeFinish();

	void SendInit(int type);
	void RecvInit(int type);
	void SendAttr();
	void RecvAttr();
	void SendStr(int type, LPCSTR msg);

	CKermit(class CRLoginDoc *pDoc, CWnd *pWnd);
	virtual ~CKermit();
};
