#pragma once

#include "syncsock.h"

// CIsh コマンド ターゲット

/******** ish Header... LSB
    unsigned char sinc;			0
    unsigned char itype;		1
    unsigned char fsize[4];		2
    unsigned char line[2];		6	jis7=63, jis8=69, sjis=73
    unsigned char block[2];		8
    unsigned char byte;			10	jis7=16, jis8=8, sjis=16
    unsigned char ibyte;		11	jis7=13, jis8=8, sjis=15
    unsigned char iname[11];	12
    unsigned char janle;		23
    unsigned char os;			24	MS_DOS=0x00, CP_M=0x10, OS_9=0x20, UNIX=0x30
    unsigned char exttype;		25
    unsigned char tstamp;		26
    unsigned char time[4];		27
********/

#define	ISH_TYPE_JIS7	0
#define	ISH_TYPE_JIS8	1
#define	ISH_TYPE_SJIS	2

#define	ISH_LEN_JIS7	63
#define	ISH_LEN_JIS8	69
#define	ISH_LEN_SJIS	73

#define	ISH_RET_NONE	0
#define	ISH_RET_HEAD	1
#define	ISH_RET_DATA	2
#define	ISH_RET_ENDOF	3
#define	ISH_RET_ABORT	4

#define	ISH_COLS_LEN	(m_Len - 3)
#define	ISH_COLS_MAX	(m_Len - 2)
#define	ISH_LINE_LEN	(m_Len - 3)
#define	ISH_LINE_MAX	(m_Len - 2)

#define	ISH_NUM_TATE	(m_Len - 2)
#define	ISH_NUM_HID		(m_Len - 1)
#define	ISH_NUM_YOKO	(m_Len)

#define	ISH_CRC_INIT	0xFFFF
#define	ISH_CRC_MATCH	0x1D0F

class CIsh : public CObject
{
public:
	int m_Stat;
	int m_Type;
	int m_Len;
	LONGLONG m_Size;
	int m_AddSize;
	CBuffer m_AddBuf;
	int m_DecLine;
	int m_SeqNum;
	BYTE *m_Buf;
	int m_FileCRC;
	CStringA m_FileName;
	LONGLONG m_FileSize;

	int CalcCRC(LPBYTE buf, int len, unsigned short crc = ISH_CRC_INIT);
	BOOL ChkCRC(CBuffer &buf);
	void SetCRC(LPBYTE buf, int len);
	BOOL ErrCollect(int err, int e1, int e2);
	void SetSum(int num, int len);

	inline BYTE *GetBuf(int idx, int ofs) { return (m_Buf + (idx - 1) * m_Len + ofs); }

	int DecodeLine(LPCSTR str, CBuffer &out);

	void EncodeHead(LPCSTR filename, LONGLONG filesize, time_t mtime, CBuffer &out);
	int EncodeBlock(FILE *fp, CBuffer &out);

	CIsh();
	virtual ~CIsh();
};


