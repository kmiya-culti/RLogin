// ZModem.h: CZModem クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ZMODEM_H__6E023DFA_01A6_48B4_A6C8_9D9FFFF837EF__INCLUDED_)
#define AFX_ZMODEM_H__6E023DFA_01A6_48B4_A6C8_9D9FFFF837EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SyncSock.h"
#include "zm.h"

#define	ZM_PKTQUE	16

class CZModem : public CSyncSock  
{
public:
	void OnProc(int cmd);

	int CalCRC(char *ptr, int len);
	int Send_blk(FILE *fp, int bk, int len, int crcopt, int head);
	int Recive_blk(char *tmp, int block, int crcopt);
	void Recive_flush();

	int XUpLoad();
	int XDownLoad();

	int YUpLoad();
	int YDownLoad();

	int Zmodem;
	int Zrwindow;
	int Zctlesc;
	int Effbaud;
	int Rxtimeout;
	int Rxbuflen;
	int Rxcanfdx;

	void zperr(LPCSTR s);
	void zperr(LPCSTR s, int u) { CStringA str; str.Format(s, u); zperr(str); }
	void zperr(LPCSTR s, LPCSTR p) { CStringA str; str.Format(s, p); zperr(str); }

	int ZUpFile();
	int ZDownFile();
	int ZUpDown(int mode);

							/* Globals used by ZMODEM functions */
	int Rxframeind;			/* ZBIN ZBIN32, or ZHEX type of frame */
	int Rxtype;				/* Type of header received */
	int Rxhlen;				/* Length of header received */
	int Rxcount;			/* Count of data bytes received */
	char Rxhdr[ZMAXHLEN];	/* Received header */
	char Txhdr[ZMAXHLEN];	/* Transmitted header */
	LONGLONG Rxpos;				/* Received file position */
	LONGLONG Txpos;				/* Transmitted file position */
	int Txfcs32;			/* TURE means send binary frames with 32 bit FCS */
	int Crc32t;				/* Controls 32 bit CRC being sent */
							/* 1 == CRC32,  2 == CRC32 + RLE */
	int Crc32r;				/* Indicates/controls 32 bit CRC being received */
							/* 0 == CRC16,  1 == CRC32,  2 == CRC32 + RLE */
	int Usevhdrs;			/* Use variable length headers */
	int Znulls;				/* Number of nulls to send at beginning of ZDATA hdr */
	char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
	char *Altcan;			/* Alternate canit string */

	int lastsent;			/* Last char we sent */
	int Not8bit;			/* Seven bits seen on header */

	void zsbhdr(int len, int type, char *hdr);
	void zsbh32(int len, char *hdr, int type, int flavour);
	void zshhdr(int len, int type, char *hdr);
	void zsdata(char *buf, int length, int frameend);
	void zsda32(char *buf, int length, int frameend);
	int zrdata(char *buf, int length);
	int zrdat32(char *buf, int length);
	void garbitch();
	int zgethdr(char *hdr, int eflag);
	int zrbhdr(char *hdr);
	int zrbhd32(char *hdr);
	int zrhhdr(char *hdr);
	void zputhex(int c);
	void zsendline(int c);
	int zgethex();
	int zgeth1();
	int zdlread();
	int noxrd7();
	void stohdr(LONGLONG pos);
	LONGLONG rclhdr(register char *hdr);

	void zsdar32(char *buf, int length, int frameend);
	int zrdatr32(char *buf, int length);

	CZModem(class CRLoginDoc *pDoc, CWnd *pWnd);
	virtual ~CZModem();
};

#endif // !defined(AFX_ZMODEM_H__6E023DFA_01A6_48B4_A6C8_9D9FFFF837EF__INCLUDED_)
