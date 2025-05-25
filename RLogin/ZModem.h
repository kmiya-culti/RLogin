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
	void DoProc(int cmd);
	void OnProc(int cmd);

	int CalCRC(char *ptr, int len);
	int Send_blk(FILE *fp, int bk, int len, int crcopt, int head);
	int Receive_blk(char *tmp, int block, int crcopt);
	void Receive_flush();

	int XUpLoad();
	int XDownLoad();

	int YUpLoad();
	int YDownLoad();

	int ZUpFile();
	int ZDownFile();
	int ZUpDown(int mode);

	int Zctlesc;
	int Rxbuflen;
	int Rxcanfdx;

	char Rxhdr[ZMAXHLEN];	/* Received header */
	char Txhdr[ZMAXHLEN];	/* Transmitted header */
	LONGLONG Rxpos;			/* Received file position */
	LONGLONG Txpos;			/* Transmitted file position */

	int Txfcs32;			/* TURE means send binary frames with 32 bit FCS */

	int Crc32t;				/* Controls 32 bit CRC being sent */
							/* 1 == CRC32,  2 == CRC32 + RLE */
	int Crc32r;				/* Indicates/controls 32 bit CRC being received */
							/* 0 == CRC16,  1 == CRC32,  2 == CRC32 + RLE */

	int Usevhdrs;			/* Use variable length headers */
	char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
	int Znulls;				/* Number of nulls to send at beginning of ZDATA hdr */
	int Not8bit;			/* Seven bits seen on header */

	void ZDispError(LPCSTR str, ...);
	void ZSendBinHeader(int len, int type, char *hdr);
	void ZSendHexHeader(int len, int type, char *hdr);
	void ZSendData(char *buf, int length, int frameend);
	int ZReceiveData(char *buf, int length, int &Rxcount);
	int ZReceiveHeader(char *hdr, int eflag);
	void ZSendHexByte(int c);
	void ZSendEscByte(int c);
	int ZReceiveHexByte();
	int ZReceiveEscByte();
	int ZReceive7Bit();
	void ZSetLongHeader(LONGLONG pos);
	LONGLONG ZGetLongHeader(char *hdr);

	CZModem(class CRLoginDoc *pDoc, CWnd *pWnd);
	virtual ~CZModem();
};

#endif // !defined(AFX_ZMODEM_H__6E023DFA_01A6_48B4_A6C8_9D9FFFF837EF__INCLUDED_)
