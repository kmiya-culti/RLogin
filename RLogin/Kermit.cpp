// Kermit.cpp: CKermit クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "Kermit.h"

#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CKermit::CKermit(class CRLoginDoc *pDoc, CWnd *pWnd) : CSyncSock(pDoc, pWnd)
{
}
CKermit::~CKermit(void)
{
}
void CKermit::OnProc(int cmd)
{
	switch(cmd) {
	case 0:
		DownLoad();
		break;
	case 1:
		UpLoad(FALSE);
		break;
	case 2:
		UpLoad(TRUE);
		break;
	case 3:
		SimpleUpLoad();
		break;
	case 4:
		SimpleDownLoad();
		break;
	}
}

//////////////////////////////////////////////////////////////////////

void CKermit::Init()
{
	m_Stage = 0;
	m_SendSeq = 0;
	m_RecvSeq = 0;

	m_MarkCh  = '\1';
	m_EolCh   = '\015';
	m_PadCh   = '\0';
	m_CtlCh   = '#';
	m_EbitCh  = '\0';	// 7bit '&'
	m_RepCh   = '~';
	m_TimeOut = 7;
	m_PadLen  = 0;
	m_WindMax = KMT_PKTQUE;
	m_PktMax  = 94;
	m_Caps    = KMT_CAP_LONGPKT | KMT_CAP_SLIDWIN | KMT_CAP_FILATTR;
	m_ChkType = 1;
	m_ChkReq  = 3;
	m_AttrFlag = 0;

	m_HisEbitCh  = 'Y';
	m_HisRepCh   = 'Y';

	m_MyEolCh   = '\015';
	m_MyCtlCh   = '#';
	m_MyPadCh   = '\0';
	m_MyPadLen  = 0;

	m_PktPos = 0;
	m_PktCnt = 0;
	m_Size   = 0;
	m_pData  = NULL;
	m_Work.Clear();

	m_FileType = FALSE;

	for ( int n = 0 ; n < KMT_PKTQUE ; n++ )
		m_OutPkt[n].Len = 0;
}

//////////////////////////////////////////////////////////////////////

int	CKermit::ChkSumType1(BYTE *p, int len)
{
	int sum = ChkSumType2(p, len);
    return ((((sum & 192) >> 6) + sum) & 63);
}
int	CKermit::ChkSumType2(BYTE *p, int len)
{
	int sum = 0;
	while ( len-- > 0 )
		sum = (sum + *(p++)) & 0377;
    return sum;
}
int	CKermit::ChkSumType3(BYTE *p, int len)
{
	int c;
	unsigned long q, crc = 0;
	while ( len-- > 0 ) {
		c   = (*p++);
		q   = (crc ^ c) & 017;
		crc = (crc >> 4) ^ (q * 010201);
		q   = (crc ^ (c >> 4)) & 017;
		crc = (crc >> 4) ^ (q * 010201);
	}
	return (int)(crc & 0xFFFF);
}

int CKermit::ReadPacket()
{
	int n;
	int ch, od, pos, sum;

RETRY:
	for ( m_InPkt.Len = pos = od = 0 ; ; od = ch ) {
		if ( AbortCheck() )
			return KMT_CANCEL;

		if ( (ch = Bufferd_Receive(10)) < 0 )
			return KMT_TIMEOUT;

		if ( ch == '\003' && od == ch )
			return KMT_CANCEL;
		else if ( ch == m_EolCh ) {
			if ( m_InPkt.Len != 0 ) {
				m_InPkt.Len = pos;
				break;
			}
			m_InPkt.Len = pos = 0;
		}

		if ( pos >= m_PktMax ) {
			goto RETRY;
		} else if ( ch == m_MarkCh ) {
			pos = 0;
			m_InPkt.Buf[pos++] = (BYTE)ch;
		} else if ( pos == 1 ) {
			m_InPkt.Buf[pos++] = (BYTE)ch;
			if ( (n = UnChar(ch)) > 1 )
				m_InPkt.Len = n + 2;
		} else if ( pos >= 2 ) {
			m_InPkt.Buf[pos++] = (BYTE)ch;
			if ( m_InPkt.Len == 0 && pos == 6 )
				m_InPkt.Len = UnChar(m_InPkt.Buf[4]) * 95 + UnChar(m_InPkt.Buf[5]) + 7;
		} else if ( pos == 0 && ch != m_EolCh )
			SendEcho(ch);

		if ( m_InPkt.Len != 0 && pos >= m_InPkt.Len )
			break;
	}

	if ( m_InPkt.Len < 4 )
		goto RETRY;

	/*
				0	1	2	3	4	5	6	7
		Nomal	Mrk	Len	Seq	Typ	.....
		Long	Mrk	Len	Seq	Typ	Hi	Lw	Sm	.....
	*/

	if ( UnChar(m_InPkt.Buf[1]) == 0 ) {	/* Long Pakcet */
		if ( m_InPkt.Len < 6 )
			goto RETRY;
		ch = m_InPkt.Buf[6];
		m_InPkt.Buf[6] = (BYTE)0;
		sum = ChkSumType1(m_InPkt.Buf + 1, 5);
		m_InPkt.Buf[6] = (BYTE)ch;
		if ( UnChar(ch) != (BYTE)sum )
			goto RETRY;
		n = 7;
	} else
		n = 4;

	m_Size  = m_InPkt.Len - n;
	m_pData = m_InPkt.Buf + n;

	switch(m_ChkType) {
	case 1:
		sum = ChkSumType1(m_InPkt.Buf + 1, m_InPkt.Len - 1 - 1);
		m_Size -= 1;
		ch = UnChar(m_pData[m_Size]);
		if ( ch != sum )
			return KMT_SUMERR;
		break;
	case 2:
		sum = ChkSumType2(m_InPkt.Buf + 1, m_InPkt.Len - 1 - 2);
		m_Size -= 2;
		ch  = (UnChar(m_pData[m_Size]) << 6);
		ch |=  UnChar(m_pData[m_Size + 1]);
		if ( ch != sum )
			return KMT_SUMERR;
		break;
	case 3:
		sum = ChkSumType3(m_InPkt.Buf + 1, m_InPkt.Len - 1 - 3);
		m_Size -= 3;
		ch  = (UnChar(m_pData[m_Size]) << 12);
		ch |= (UnChar(m_pData[m_Size + 1]) << 6);
		ch |=  UnChar(m_pData[m_Size + 2]);
		ch &= 0xFFFF;
		if ( ch != sum )
			return KMT_SUMERR;
		break;
	}

	m_pData[m_Size] = '\0';

	DebugMsg("Kermit %d recv: %d %c(%d)", m_RecvSeq, UnChar(m_InPkt.Buf[2]), m_InPkt.Buf[3], m_Size);
	DebugDump(m_InPkt.Buf, m_InPkt.Len < 16 ? m_InPkt.Len : 16);

	m_InPkt.Seq  = UnChar(m_InPkt.Buf[2]);
	m_InPkt.Type = m_InPkt.Buf[3];
	return m_InPkt.Type;
}
BOOL CKermit::DecodePacket()
{
	int b, b7, b8, rpt;
	BYTE *p = m_pData;
	BYTE *e = m_pData + m_Size;

	m_Work.Clear();

	while ( p < e ) {
		rpt = 1;
		b8 = 0;
		b = *(p++);

		if ( m_RepCh != 0 && b == m_RepCh ) {
			if ( (p + 2) > e )
				return FALSE;
			if ( (rpt = UnChar(*(p++))) <= 0 )
				rpt = 1;
			b = *(p++);
		}
		if ( m_EbitCh != 0 && b == m_EbitCh ) {
			if ( p >= e )
				return FALSE;
			b = *(p++);
			b8 = 128;
		}
		if ( m_CtlCh != 0 && b == m_CtlCh ) {
			if ( p >= e )
				return FALSE;
			b = *(p++);
			b7 = b & 127;
			if ( b7 != m_CtlCh && (m_EbitCh == 0 || b7 != m_EbitCh) && (m_RepCh == 0 || b7 != m_RepCh) )
				b = ToCtrl(b);
		}
		b |= b8;
		while ( rpt-- > 0 )
			m_Work.Put8Bit(b);
	}

    return TRUE;
}

int CKermit::SizePacket()
{
	// 	0	1	2	3	4	5	6	7     n
	//	Mrk	Len	Seq	Typ	Hi	Lw	Sm	..... Sum...

	int len = 4;			// Mrk Len Seq Typ

	len += m_MyPadLen;

	if ( m_PktMax > 94 )
		len += 3;			// HI Lw Sm

	len += m_ChkType;		// Sum ... 1, 2, 3

	len += 5;				// m_RepCh + RepLen + m_EbitCh + m_MyCtlCh + ch = 5

	if ( m_MyEolCh != 0 )
		len += 1;

	return len;
}
int CKermit::MakePacket(int type, int seq, BYTE *buf, int len)
{
	int n, sum;
	BYTE *s, *p;

	m_PktPos = (m_PktPos + 1) & (KMT_PKTQUE - 1);
	p = m_OutPkt[m_PktPos].Buf;

	for ( n = 0 ; n < m_MyPadLen ; n++ )
		*(p++) = m_MyPadCh;

	*(p++) = m_MarkCh;
	s = p++;
	*(p++) = ToChar(seq);
	*(p++) = (BYTE)type;

	n = len + m_ChkType;
	if ( (n + 2) > 94 ) {
		*s = ToChar(0);
		*(p++) = ToChar(n / 95);
		*(p++) = ToChar(n % 95);
		*p = '\0';
		*(p++) = ToChar(ChkSumType1(s, 5));
	} else
		*s = ToChar(n + 2);

	memcpy(p, buf, len);
	p += len;
	n = (int)(p - s);

	switch(m_ChkType) {
	case 1:
		sum = ChkSumType1(s, n);
		*(p++) = ToChar(sum);
		break;
	case 2:
		sum = ChkSumType2(s, n);
		*(p++) = ToChar((sum >> 6) & 077);
		*(p++) = ToChar(sum & 077);
		break;
	case 3:
		sum = ChkSumType3(s, n);
		*(p++) = ToChar((sum >> 12) & 017);
		*(p++) = ToChar((sum >> 6) & 077);
		*(p++) = ToChar(sum & 077);
		break;
	}

	if ( m_MyEolCh != 0 )
		*(p++) = m_MyEolCh;

	m_OutPkt[m_PktPos].Type = type;
	m_OutPkt[m_PktPos].Seq  = seq;
	m_OutPkt[m_PktPos].Len  = (int)(p - m_OutPkt[m_PktPos].Buf);

	return m_PktPos;
}
int CKermit::PktPos(int pos)
{
	while ( pos < 0 )
		pos += KMT_PKTQUE;
	while ( pos >= KMT_PKTQUE )
		pos -= KMT_PKTQUE;
	return pos;
}
void CKermit::SendPacket(int pos)
{
	pos = PktPos(pos);

	if ( m_OutPkt[pos].Len <= 0 )
		return;

	Bufferd_SendBuf((char *)m_OutPkt[pos].Buf, m_OutPkt[pos].Len);
	Bufferd_Flush();

	DebugMsg("Kermit %d Send: %c(%d)", m_OutPkt[pos].Seq, m_OutPkt[pos].Type, m_OutPkt[pos].Len);
	DebugDump(m_OutPkt[pos].Buf, m_OutPkt[pos].Len < 16 ? m_OutPkt[pos].Len : 16);
}
void CKermit::EncodeChar(int ch)
{
	int b7 = ch & 127;
	if ( m_EbitCh != 0 && (ch & 128) != 0 ) {
		m_Work.Put8Bit(m_EbitCh);
		ch &= 127;
	}
	if ( m_MyCtlCh != 0 && (b7 < 32 || b7 == 127) ) {
		m_Work.Put8Bit(m_MyCtlCh);
		ch = ToCtrl(ch);
	} else if ( b7 == m_MyCtlCh )
		m_Work.Put8Bit(m_MyCtlCh);
	else if ( b7 == m_EbitCh )
		m_Work.Put8Bit(m_MyCtlCh);
	else if ( b7 == m_RepCh )
		m_Work.Put8Bit(m_MyCtlCh);

	m_Work.Put8Bit(ch);
}
void CKermit::EncodeInit()
{
	m_RepOld = (-1);
	m_RepLen = 0;
	m_Work.Clear();
}
void CKermit::EncodeData(int ch)
{
	if ( m_RepCh != 0 ) {
		if ( m_RepOld == ch ) {
			if ( ++m_RepLen > 63 ) {
				m_Work.Put8Bit(m_RepCh);
				m_Work.Put8Bit(ToChar(m_RepLen - 1));
				EncodeChar(m_RepOld);
				m_RepLen = 1;
			}
		} else {
			if ( m_RepOld != (-1) ) {
				if ( m_RepLen > 2 ) {
					m_Work.Put8Bit(m_RepCh);
					m_Work.Put8Bit(ToChar(m_RepLen));
				} else if ( m_RepLen > 1 )
					EncodeChar(m_RepOld);
				EncodeChar(m_RepOld);
			}
			m_RepOld = ch;
			m_RepLen = 1;
		}
	} else
		EncodeChar(ch);
}
void CKermit::EncodeFinish()
{
	if ( m_RepOld != (-1) ) {
		if ( m_RepLen > 2 ) {
			m_Work.Put8Bit(m_RepCh);
			m_Work.Put8Bit(ToChar(m_RepLen));
		} else if ( m_RepLen > 1 )
			EncodeChar(m_RepOld);
		EncodeChar(m_RepOld);
	}
	m_RepOld = (-1);
	m_RepLen = 0;
}

//////////////////////////////////////////////////////////////////////

// Type 'S'(req) or 'Y'(ack)
void CKermit::SendInit(int type)
{
	BYTE param[20];

	param[0] = ToChar(m_PktMax < 94 ? m_PktMax : 94);
	param[1] = ToChar(7);
	param[2] = ToChar(m_MyPadLen);
	param[3] = ToCtrl(m_MyPadCh);
	param[4] = ToChar(m_MyEolCh);
	param[5] = m_MyCtlCh;
	param[7] = m_ChkReq + '0';

	if ( type == 'S' ) {	// Req
		param[6] = (m_EbitCh != 0 ? m_EbitCh : 'Y');
		param[8] = (m_RepCh  != 0 ? m_RepCh  : 'Y');

	} else {				// Ack
		if ( (m_HisEbitCh > 32 && m_HisEbitCh < 63) || (m_HisEbitCh > 95 && m_HisEbitCh < 127) )
			param[6] = m_EbitCh = m_HisEbitCh;
		else if ( m_HisEbitCh == 'Y' && m_EbitCh != 0 )
			param[6] = m_EbitCh;
		else {
			param[6] = 'N';
			m_EbitCh = '\0';
		}

		if ( (m_HisRepCh > 32 && m_HisRepCh < 63) || (m_HisRepCh > 95 && m_HisRepCh < 127) )
			param[8] = m_RepCh = m_HisRepCh;
		else if ( m_HisRepCh == 'Y' && m_RepCh != 0 )
			param[8] = m_RepCh;
		else {
			param[8] = 'N';
			m_RepCh = '\0';
		}
	}

	param[9]  = ToChar(m_Caps);
	param[10] = ToChar((m_Caps & KMT_CAP_SLIDWIN) != 0 ? m_WindMax : 1);

	if ( type == 'S' ) {	// Req
		param[11] = ToChar(KMT_DATAMAX / 95);
		param[12] = ToChar(KMT_DATAMAX % 95);
	} else {				// Ack
		param[11] = ToChar(m_PktMax / 95);
		param[12] = ToChar(m_PktMax % 95);
	}

	SendPacket(MakePacket(type, m_SendSeq, param, 13));

	DebugMsg("SendInit %c %c(%02x) %c(%02x)", type, param[6], m_EbitCh, param[8], m_RepCh);
}

// Type 'S'(req) or 'Y'(ack)
void CKermit::RecvInit(int type)
{
	int n, d, a, t;

	for ( n = 0 ; n < m_Size && n < 10 ; n++ ) {
		d = m_pData[n];
		switch(n) {
		case 0:
			if ( (m_PktMax = UnChar(d)) < 10 )
				m_PktMax = 80;								// XXX ???
			break;
		case 1:
			m_TimeOut = UnChar(d);
			break;
		case 2:
			m_PadLen = UnChar(d);
			break;
		case 3:
			m_PadCh = ToCtrl(d);
			break;
		case 4:
			m_EolCh = UnChar(d);
			break;
		case 5:
			m_CtlCh = d;
			break;
		case 6:		// 8th bit character prefix def='&'
			if ( type == 'S' )	// Req
				m_HisEbitCh = d;
			else if ( (d > 32 && d < 63) || (d > 95 && d < 127) )
				m_EbitCh = d;
			else if ( d != 'Y' )
				m_EbitCh = 0;
			break;
		case 7:
			if ( (a = d - '0') < 1 && a > 3 )
				a = 1;
			m_ChkReq = a;
			break;
		case 8:		// Repeat character prefix def='~'
			if ( type == 'S' )	// Req
				m_HisRepCh = d;
			if ( (d > 32 && d < 63) || (d > 95 && d < 127) )
				m_RepCh = d;
			else if ( d != 'Y' )
				m_RepCh = 0;
			break;
		case 9:
			a = 0;
			t = UnChar(d);
			if ( (t & m_Caps & KMT_CAP_LONGPKT) != 0 )
				a |= KMT_CAP_LONGPKT;
			if ( (t & m_Caps & KMT_CAP_SLIDWIN) != 0 )
				a |= KMT_CAP_SLIDWIN;
			if ( (t & m_Caps & KMT_CAP_FILATTR) != 0 )
				a |= KMT_CAP_FILATTR;
			m_Caps = a;
			break;
		}
	}

	for ( n = 9 ; n < m_Size && (m_pData[n] & KMT_CAP_CONTINE) != 0 ; n++ )
		;

	if ( (m_Caps & KMT_CAP_SLIDWIN) != 0 && (n + 1) < m_Size ) {
		m_WindMax = UnChar(m_pData[n + 1]);
		if ( m_WindMax <= 0 )
			m_WindMax = 1;
		else if ( m_WindMax > KMT_PKTQUE )
			m_WindMax = KMT_PKTQUE;
	} else
		m_WindMax = 1;

	if ( (m_Caps & KMT_CAP_LONGPKT) != 0 && (n + 3) < m_Size ) {
		d = UnChar(m_pData[n + 2]) * 95 + UnChar(m_pData[n + 3]);
		if ( d > m_PktMax && (m_PktMax = d) > KMT_DATAMAX )
			m_PktMax = KMT_DATAMAX;
	}

	DebugMsg("RecvInit %c %c(%02x) %c(%02x)", type, m_pData[6], m_EbitCh, m_pData[8], m_RepCh);
}
time_t CKermit::TimeStamp(LPCSTR p)
{
	struct tm tm;

	memset(&tm, 0, sizeof(tm));

	switch(strlen(p)) {
	case 17:	// yyyymmdd hh:mm:ss
		if ( sscanf(p, "%04d%02d%02d %02d:%02d:%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) < 6 )
			goto ERRFMT;
		break;
	case 15:	// yymmdd hh:mm:ss
		if ( sscanf(p, "%02d%02d%02d %02d:%02d:%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) < 6 )
			goto ERRFMT;
		break;
	case 14:	// yyyymmdd hh:mm
		if ( sscanf(p, "%04d%02d%02d %02d:%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min) < 5 )
			goto ERRFMT;
		break;
	case 12:	// yymmdd hh:mm
		if ( sscanf(p, "%02d%02d%02d %02d:%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min) < 5 )
			goto ERRFMT;
		break;
	case  8:	// yyyymmdd
		if ( sscanf(p, "%04d%02d%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) < 3 )
			goto ERRFMT;
		break;
	case  6:	// yymmdd
		if ( sscanf(p, "%02d%02d%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) < 3 )
			goto ERRFMT;
		break;
	default:
		goto ERRFMT;
	}

	if ( tm.tm_year < 0 )
		goto ERRFMT;
	else if ( tm.tm_year >= 1900 )
		tm.tm_year -= 1900;		// 1900 - 
	else if ( tm.tm_year < 50 )
		tm.tm_year += 100;		// 00 - 49 = 2000 - 2049 ... (+ 2000 - 1900) = 100)
	else if ( tm.tm_year < 100 )
		tm.tm_year += 0;		// 50 - 99 = 1950 - 1999 ... (+ 1900 - 1900) = 0)
	else
		goto ERRFMT;			// 100 - 1899 error

	if ( tm.tm_mon >= 1 && tm.tm_mon <= 12 )
		tm.tm_mon -= 1;			// 0 - 11
	else
		goto ERRFMT;

	if ( tm.tm_mday < 1 || tm.tm_mday > 31 )
		goto ERRFMT;

	if ( tm.tm_hour < 0 || tm.tm_hour >= 24 )
		goto ERRFMT;

	if ( tm.tm_min < 0 || tm.tm_min >= 60 )
		goto ERRFMT;

	if ( tm.tm_sec < 0 || tm.tm_sec >= 60 )
		goto ERRFMT;

	return mktime(&tm);

ERRFMT:
	return 0;
}
void CKermit::RecvAttr()
{
	int n, c;
	BYTE *p = m_pData;
	CStringA str;

	m_AttrFlag = 0;
	while ( (c = *(p++)) != '\0' ) {
		if ( *p == '\0' )
			break;
		str.Empty();
		n = UnChar(*(p++));
		while ( n-- > 0 && *p != '\0' )
			str += *(p++);

		switch(c) {
		case '.':	// System ID
		case '*':	// Encoding
		case '!':	// File length in K byte
		case '-':	// Generic "world" protection code
		case '/':	// Record format
		case '(':	// File Block Size
		case '+':	// Disposition
		case '0':	// System-dependent parameters
			break;
		case '"':	// File type
			m_AttrFlag |= KMT_ATTR_TYPE;
			m_FileType = (str[0] == 'A' ? TRUE : FALSE);
			break;
		case '#':	// File creation date "20100226 12:23:45"
			if ( (m_FileTime = TimeStamp(str)) != 0 )
				m_AttrFlag |= KMT_ATTR_TIME;
			break;
		case ',':	// File attribute "664"
			if ( sscanf(str, "%03o", &m_FileMode) == 1 && m_FileMode != 0 )
				m_AttrFlag |= KMT_ATTR_MODE;
			break;
		case '1':	// File length in bytes
			m_AttrFlag |= KMT_ATTR_SIZE;
			m_FileSize = _atoi64(str);
			break;
		}
	}
}
void CKermit::SendAttr()
{
	CStringA str, tmp;

	if ( (m_AttrFlag & KMT_ATTR_TYPE) != 0 ) {
		str += '"';
		str += (char)ToChar(2);
		str += (m_FileType ? "A" : "B");
		str += "8";
	}

	if ( (m_AttrFlag & KMT_ATTR_TIME) != 0 ) {
		CTime t(m_FileTime);
		tmp = t.Format(_T("%Y%m%d %H:%M:%S"));
		str += '#';
		str += (char)ToChar(tmp.GetLength());
		str += tmp;
	}
	if ( (m_AttrFlag & KMT_ATTR_MODE) != 0 ) {
		tmp.Format("%03o", m_FileMode & 0777);
		str += ',';
		str += (char)ToChar(tmp.GetLength());
		str += tmp;
	}
	if ( (m_AttrFlag & KMT_ATTR_SIZE) != 0 ) {
		tmp.Format("%I64d", m_FileSize);
		str += '1';
		str += (char)ToChar(tmp.GetLength());
		str += tmp;
	}

	SendPacket(MakePacket('A', m_SendSeq, (BYTE *)(LPCSTR)str, str.GetLength()));
}

void CKermit::SendStr(int type, LPCSTR msg)
{
	EncodeInit();
	while ( *msg != '\0' )
		EncodeData(*(msg++));
	EncodeFinish();

	SendPacket(MakePacket(type, m_SendSeq, m_Work.GetPtr(), m_Work.GetSize()));
}

//////////////////////////////////////////////////////////////////////

void CKermit::DownLoad()
{
	int n;
	int retry = 0;
	int stat = 0;
	FILE *fp = NULL;

	Init();
	MakePacket('N', m_SendSeq, (BYTE *)"", 0);	// Dummy NAK

	for ( ; ; ) {
		if ( (n = ReadPacket()) == KMT_CANCEL ) {
			SendStr('E', "Cancel");
			Bufferd_Clear();
			goto ENDOF;
		} else if ( n == 'E' ) {
			DecodePacket();
			UpDownMessage(m_Work);
			Bufferd_Clear();
			goto ENDOF;
		} else if ( n == KMT_TIMEOUT || n == KMT_SUMERR ) {
			if ( ++retry > 4 ) {
				SendStr('E', "Retry Over");
				Bufferd_Clear();
				goto ENDOF;
			}
			SendStr('N', "");
			DebugMsg("ReadPacket Timeout %d", retry);
			continue;
		}

		retry = 0;

		switch(stat) {
		case 0:
			if ( n == 'N' ) {			/* Receive NAK packet */
				UpLoad(FALSE);
				return;
			} else if ( n == 'S' ) {	/* Receive S packet */
				RecvInit('S');
				SendInit('Y');
				m_RecvSeq = m_SendSeq = IncSeq(m_SendSeq);
				m_ChkType = m_ChkReq;
				stat = 1;
			} else {
				SendStr('E', "S packet error");
				Bufferd_Clear();
				goto ENDOF;
			}
			break;

		case 1:
			if ( n == 'B' ) {			/* Receive EOT packet */
				SendStr('Y', "");
				m_RecvSeq = m_SendSeq = IncSeq(m_SendSeq);
				goto ENDOF;
			} else if ( n == 'F' ) {	/* Receive File header packet */
				DecodePacket();

				if ( CheckFileName(0, (LPCSTR)(m_Work)) == NULL ) {
					SendStr('E', "DownLoad Abort");
					goto ENDOF;
				}
				if ( (fp = FileOpen(m_PathName, "wb", FALSE)) == NULL ) {
					SendStr('E', "File Open Error");
					goto ENDOF;
				}
				UpDownOpen("Kermit File Download");
				m_TranSize = 0;

				SendStr('Y', "");
				m_RecvSeq = m_SendSeq = IncSeq(m_SendSeq);
				m_AttrFlag = 0;
				stat = 2;
			} else if ( n == 'N' ) {	/* Receive NAK packet */
				SendPacket(m_PktPos);
			} else {
				SendStr('E', "F or B packet error");
				Bufferd_Clear();
				goto ENDOF;
			}
			break;

		case 2:
			if ( n == 'A' ) {			/* Receive Attribute packet */
				RecvAttr();
				SendStr('Y', "\"");
				m_RecvSeq = m_SendSeq = IncSeq(m_SendSeq);

				if ( (m_AttrFlag & KMT_ATTR_SIZE) != 0 )
					UpDownInit(m_FileSize);
				if ( (m_AttrFlag & KMT_ATTR_TYPE) != 0 )
					SetFileType(m_FileType);
				break;
			}
			stat = 3;
			// no break;

		case 3:
			if ( n == 'Z' ) {			/* Receive EOF packet */
				SendStr('Y', "");
				m_RecvSeq = m_SendSeq = IncSeq(m_SendSeq);

				stat = 1;

				FileClose(fp);
				fp = NULL;
				UpDownClose();
				if ( (m_AttrFlag & KMT_ATTR_TIME) != 0 ) {
					struct _utimbuf utm;
					utm.actime  = m_FileTime;
					utm.modtime = m_FileTime;
					_tutime(m_PathName, &utm);
				}
			} else if ( n == 'D' ) {	/* Receive Data packet */
				if ( m_InPkt.Seq == m_RecvSeq ) {
					SendStr('Y', "");
					m_RecvSeq = m_SendSeq = IncSeq(m_SendSeq);

					DecodePacket();
					WriteFileFromHost((char *)m_Work.GetPtr(), m_Work.GetSize(), fp);
					m_TranSize += m_Work.GetSize();
					UpDownStat(m_TranSize);

				} else {
					CStringA msg;
					msg.Format("Recv D %d ? %d\r\n", m_InPkt.Seq, m_RecvSeq);
					UpDownMessage(msg);
				}

			} else if ( n == 'N' ) {	/* Receive NAK packet */
				SendEchoBuffer("Receive NAK\r\n", 12);
				SendPacket(m_PktPos);

			} else {
				SendStr('E', "A or D or Z packet error");
				Bufferd_Clear();
				goto ENDOF;
			}
			break;
		}
	}

ENDOF:
	if ( fp != NULL )
		FileClose(fp);
	UpDownClose();
}
void CKermit::UpLoad(BOOL ascii)
{
	int n, i, c;
	int retry = 0;
	int stat = 0;
	FILE *fp = NULL;
	struct _stati64 st;
	BOOL useAck = TRUE;
	int windSize = 1;
	int addSize = 3;

	Init();
	SendInit('S');
	m_SendSeq = IncSeq(m_SendSeq);
	m_PktCnt++;

	for ( ; ; ) {
		if ( useAck ) {
			if ( (n = ReadPacket()) == KMT_CANCEL ) {
				SendStr('E', "Cancel");
				Bufferd_Clear();
				goto ENDOF;
			} else if ( n == 'E' ) {
				DecodePacket();
				UpDownMessage(m_Work);
				Bufferd_Clear();
				goto ENDOF;
			} else if ( n == KMT_TIMEOUT || n == KMT_SUMERR ) {
				if ( ++retry > 4 ) {
					SendStr('E', "Retry Over");
					Bufferd_Clear();
					goto ENDOF;
				}
				for ( i = m_PktCnt - 1 ; i >= 0 ; i-- )
					SendPacket(m_PktPos - i);
				windSize = 1;
				continue;
			} else if ( n == 'N' ) {
				if ( ++retry > (m_PktCnt + 4) ) {
					SendStr('E', "Retry Nack Over");
					Bufferd_Clear();
					goto ENDOF;
				}
				for ( i = 0 ; i < m_PktCnt ; i++ ) {
					c = PktPos(m_PktPos - m_PktCnt + 1 + i);
					if ( m_InPkt.Seq == m_OutPkt[c].Seq ) {
						if ( stat == 0 && c == 0 && m_PktCnt == 1 && retry > 2 ) {
							Init();
							m_Caps    = 0;		// XXXX Cheap Kermit ?
							SendInit('S');
							m_SendSeq = IncSeq(m_SendSeq);
							m_PktCnt++;
						} else
							SendPacket(c);
						break;
					}
				}
				windSize = 1;
				continue;
			} else if ( n == 'Y' ) {
				for ( i = 0 ; i < m_PktCnt ; i++ ) {
					if ( m_InPkt.Seq == m_OutPkt[PktPos(m_PktPos - m_PktCnt + 1 + i)].Seq )
						break;
				}
				if ( i >= m_PktCnt ) {
					SendStr('E', "SeqNum Error");
					Bufferd_Clear();
					goto ENDOF;
				}
				retry = 0;
				m_PktCnt -= (i + 1);
				if ( windSize < m_WindMax )
					windSize++;
			} else {
				SendStr('E', "Unkown packet Error");
				Bufferd_Clear();
				goto ENDOF;
			}
		}

		switch(stat) {
		case 0:
			RecvInit('Y');
			m_ChkType = m_ChkReq;
			stat = 1;
			addSize = SizePacket();
			// no break

		case 1:
		NEXTSEND:
			if ( CheckFileName(3, "") == NULL || m_PathName.IsEmpty() || 
					_tstati64(m_PathName, &st) || (st.st_mode & _S_IFMT) != _S_IFREG ||
					(fp = FileOpen(m_PathName, "rb", ascii)) == NULL )
				goto SENDEOT;

			m_AttrFlag = KMT_ATTR_TIME | KMT_ATTR_MODE | KMT_ATTR_SIZE | KMT_ATTR_TYPE;
			m_FileType = ascii;
			m_FileTime = st.st_mtime;
			m_FileMode = st.st_mode;
			m_FileSize = st.st_size;
			m_TranSize = 0;

			UpDownOpen("Kermit File Upload");
			UpDownInit(m_FileSize);

			SendStr('F', m_FileName);	// Send File
			m_SendSeq = IncSeq(m_SendSeq);
			m_PktCnt++;
			windSize = 1;
			stat = ((m_Caps & KMT_CAP_FILATTR) != 0 ? 2 : 3);
			break;

		case 2:
			SendAttr();
			m_SendSeq = IncSeq(m_SendSeq);
			m_PktCnt++;
			windSize = 1;
			stat = 3;
			break;

		case 3:
			EncodeInit();
			while ( (c = ReadCharToHost(fp)) != EOF ) {
				m_TranSize++;
				EncodeData(c);
				if ( m_Work.GetSize() > (m_PktMax - addSize) )
					break;
			}
			EncodeFinish();

			if ( m_Work.GetSize() > 0 ) {
				SendPacket(MakePacket('D', m_SendSeq, m_Work.GetPtr(), m_Work.GetSize()));
				m_SendSeq = IncSeq(m_SendSeq);
				m_PktCnt++;
				UpDownStat(m_TranSize);
			} else {
				SendStr('Z', "");	// End of File
				m_SendSeq = IncSeq(m_SendSeq);
				m_PktCnt++;
				FileClose(fp);
				fp = NULL;
				UpDownClose();
				stat = 4;
			}
			useAck = (m_PktCnt < windSize ? FALSE : TRUE);
			break;

		case 4:
			useAck = TRUE;
			if ( m_PktCnt > 0 )
				break;
			stat = 5;
			// no break

		case 5:
			if ( !m_ResvPath.IsEmpty() )
				goto NEXTSEND;
			// no break

		SENDEOT:
			SendStr('B', "");	// EOT
			m_SendSeq = IncSeq(m_SendSeq);
			m_PktCnt++;
			stat = 6;
			break;

		case 6:
			goto ENDOF;
		}
	}

ENDOF:
	if ( fp != NULL )
		FileClose(fp);
	UpDownClose();
}

//////////////////////////////////////////////////////////////////////
// Simple File Up/Down Load

void CKermit::SimpleUpLoad()
{
	FILE *fp = NULL;
	struct _stati64 st;
	int len;
	char buf[1024];

	while ( !m_DoAbortFlag ) {
		if ( CheckFileName(3, "") == NULL || m_PathName.IsEmpty() )
			break;

		if ( _tstati64(m_PathName, &st) || (st.st_mode & _S_IFMT) != _S_IFREG )
			break;

		if ( (fp = FileOpen(m_PathName, "rb", FALSE)) == NULL )
			break;

		m_FileType = FALSE;
		m_FileTime = st.st_mtime;
		m_FileMode = st.st_mode;
		m_FileSize = st.st_size;
		m_TranSize = 0;

		UpDownOpen("Simple File Upload");
		UpDownInit(m_FileSize);

		while ( !m_DoAbortFlag && (len = ReadFileToHost(buf, 1024, fp)) > 0 ) {
			Bufferd_SendBuf(buf, len);
			Bufferd_Sync();

			m_TranSize += len;
			UpDownStat(m_TranSize);

			while ( (len = Bufferd_ReceiveSize()) > 0 ) {
				if ( len > 1024 )
					len = 1024;
				if ( !Bufferd_ReceiveBuf(buf, len, 0) )
					break;
				SendEchoBuffer(buf, len);
			}
		}

		FileClose(fp);
		UpDownClose();

		if (  m_ResvPath.IsEmpty() )
			break;
	}
}
void CKermit::SimpleDownLoad()
{
	int ch, len = 0;
	FILE *fp = NULL;
	char buf[1024];

	if ( CheckFileName(0, (LPCSTR)(m_Work)) == NULL )
		return;

	if ( (fp = FileOpen(m_PathName, "wb", FALSE)) == NULL )
		return;

	UpDownOpen("Simple File Download");
	m_TranSize = 0;

	while ( !m_DoAbortFlag ) {
		ch = Bufferd_Receive(len > 0 ? 2 : 60);

		if ( len <= 0 && ch < 0 )
			break;

		if ( ch >= 0 )
			buf[len++] = (char)ch;

		if ( len > 0 && (len >= 1024 || ch < 0) ) {
			WriteFileFromHost(buf, len, fp);

			m_TranSize += len;
			UpDownStat(m_TranSize);
			len = 0;
		}
	}

	FileClose(fp);
	UpDownClose();
}