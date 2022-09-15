// Ish.cpp : ŽÀ‘•ƒtƒ@ƒCƒ‹
//

#include "stdafx.h"
#include "RLogin.h"
#include "Ish.h"

//////////////////////////////////////////////////////////////////////
// CIsh

CIsh::CIsh()
{
	m_Stat = 0;
	m_Type = ISH_TYPE_JIS7;
	m_Len  = ISH_LEN_JIS7;
	m_Size = 0;
	m_Buf  = NULL;
}
CIsh::~CIsh()
{
	if ( m_Buf != NULL )
		delete [] m_Buf;
}

//////////////////////////////////////////////////////////////////////

int CIsh::CalcCRC(LPBYTE buf, int len, unsigned short crc)
{
	while( len-- > 0 )
		crc = crc << 8 ^ crc16tab[crc >> 8 ^ *buf++];
	return crc;
}
BOOL CIsh::ChkCRC(CBuffer &buf)
{
	return (ISH_CRC_MATCH == CalcCRC(buf.GetPtr(), buf.GetSize()) ? TRUE : FALSE);
}
void CIsh::SetCRC(LPBYTE buf, int len)
{
	int crc = ~CalcCRC(buf, len);
	buf[len + 0] = (BYTE)(crc >> 8);
	buf[len + 1] = (BYTE)(crc);
}
BOOL CIsh::ErrCollect(int err, int e1, int e2)
{
	/*
	  "ecc.c"
	  Error collection routine for OS9 ish
	  By Gigo (original idea by M.ishizuka)
	*/

    BYTE sum;
    int i, j, k, x;

    if ( err == 1 ) {     /* 1 line error correct (easy!) */
		if ( *GetBuf(ISH_NUM_TATE, 0) != '\0' ) {		// Tate Sum
			for ( j = 1 ; j < ISH_COLS_MAX ; j++ ) {
				for( i = 1, sum = 0 ; i < ISH_NUM_HID ; )
					sum -= *GetBuf(i++, j);
				*GetBuf(e1, j) = sum;
			}
			return TRUE;
		}

		/* oh! checksum(V) error increase */
		e2 = ISH_NUM_TATE;
		*GetBuf(e2, 0) = e2;
		for( j = 1 ; j < ISH_COLS_MAX ; )
			*GetBuf(e2, j++) = 0;
    }

	if ( err > 2 )
		return FALSE;

    /* correct 2 line error */

    if ( *GetBuf(ISH_NUM_YOKO, 0) == '\0' )      /* no checksum(T) line, give up */
		return FALSE;

    /* calc hidden byte */

    switch(m_Type) {
    case ISH_TYPE_JIS7: sum = 0x9d; break;
    case ISH_TYPE_JIS8: sum = 0x1a; break;
    case ISH_TYPE_SJIS: sum = 0x04; break;
    }

    for( j = 1 ; j < ISH_COLS_MAX ; j++ )
		sum -= *GetBuf(ISH_NUM_YOKO, j);
	*GetBuf(ISH_NUM_YOKO, 0) = sum;

    /* clear & set */

    for( j = 0 ; j < m_Len ; j++ )
		*GetBuf(ISH_NUM_HID, j) = 0;

    x = 0;

    for( k = 0 ; k < ISH_COLS_LEN ; k++ ) {		/* search scanning pass */
		if ( x <= e1 )							/* use '?' oprater, clash !! Oops! */
		    j = ISH_NUM_HID - e1 + x;
		else
			j = x - e1 + 1;

		/* travase */

		for( i = 1, sum = 0 ; i <= m_Len ; j++, i++ ) {
			j = j % ISH_COLS_MAX;
			sum -= *GetBuf(i, j);
		}
		x = (x + e2 - e1) % ISH_COLS_MAX;
		*GetBuf(e2, x) = sum;

		/* vartical checksum */

		for( i = 1, sum = 0 ; i < ISH_NUM_HID ; i++ )
			sum -= *GetBuf(i, x);
		*GetBuf(e1, x) = sum;
    }

	return TRUE;
}
void CIsh::SetSum(int num, int len)
{
	int i;

	if ( num < ISH_NUM_TATE ) {
		// tatesum
		for ( i = 1 ; i < len ; i++ )
			*GetBuf(ISH_NUM_TATE, i) -= *GetBuf(num, i);
	}

    if ( num < ISH_NUM_YOKO ) {	
		// yokosum
		for( i = 0 ; i < len ; i++ )
			*GetBuf(ISH_NUM_YOKO, (len - (num - 1) + i) % len + 1) -= *GetBuf(num, i);
	}
}

//////////////////////////////////////////////////////////////////////

int CIsh::DecodeLine(LPCSTR str, CBuffer &out)
{
	int num, line, len, err, bad[3];
	int ish_type[2];
	CBuffer work;
	int retcode = ISH_RET_NONE;

	out.Clear();

	switch(m_Stat) {
	case 0:		// Header check ! or #
		if ( *str == '!' )
			work.IshDecJis7(str);
		else if ( *str == '#' )
			work.IshDecJis8(str);
		else
			break;

		if ( work.GetSize() <= 60 || !ChkCRC(work) )
			break;

		m_Len = *(work.GetPos(6)) | (*(work.GetPos(7)) << 8);
		ish_type[0] = *(work.GetPos(10));
		ish_type[1] = *(work.GetPos(11));

		if (      ish_type[0] == 16 && ish_type[1] == 13 && m_Len == ISH_LEN_JIS7 )
			m_Type = ISH_TYPE_JIS7;
		else if ( ish_type[0] ==  8 && ish_type[1] ==  8 && m_Len == ISH_LEN_JIS8 )
			m_Type = ISH_TYPE_JIS8;
		else if ( ish_type[0] == 16 && ish_type[1] == 15 && m_Len == ISH_LEN_SJIS )
			m_Type = ISH_TYPE_SJIS;
		else
			break;

		m_AddSize = 2;
		m_AddBuf.Clear();
		m_FileCRC = ISH_CRC_INIT;
		m_FileSize = 0;

		m_Size = (LONGLONG)*(work.GetPos(2)) | ((LONGLONG)*(work.GetPos(3)) << 8) | ((LONGLONG)*(work.GetPos(4)) << 16) | ((LONGLONG)*(work.GetPos(5)) << 24);
		m_DecLine = (int)((m_Size + m_AddSize + ISH_COLS_LEN - 1) / (LONGLONG)ISH_COLS_LEN);

		{
			int n, m;
			LPCSTR p = (LPCSTR)(work.GetPos(12));

			m_FileName.Empty();
			for ( m = 8 ; m > 0 && p[m - 1] == ' ' ; )
				m--;
			for ( n = 0 ; n < m ; n++ )
				m_FileName += p[n];
			for ( m = 11 ; m > 8 && p[m - 1] == ' ' ; )
				m--;
			if ( m > 8 )
				m_FileName += '.';
			for ( n = 8 ; n < m ; n++ )
				m_FileName += p[n];
		}

		if ( m_Buf != NULL )
			delete [] m_Buf;

		m_Buf = new BYTE[m_Len * m_Len];
		ZeroMemory(m_Buf, m_Len * m_Len);
		m_SeqNum = 0;
		m_Stat = 1;
		retcode = ISH_RET_HEAD;
		break;

	case 1:
		switch(m_Type) {
		case ISH_TYPE_JIS7:
			work.IshDecJis7(str);
			break;
		case ISH_TYPE_JIS8:
			work.IshDecJis8(str);
			break;
		case ISH_TYPE_SJIS:
			work.IshDecSjis(str);
			break;
		}

		if ( work.GetSize() <= 0 || !ChkCRC(work) || work.GetSize() > m_Len )
			break;

		num = *(work.GetPos(0));
		if ( num <= 0 || num > m_Len )
			break;

		// YokoSum(Last Line) Missing ?
		if ( num < m_SeqNum )
			goto DECBLOCK;

		memcpy(GetBuf(num, 0), work.GetPtr(), work.GetSize());
		m_SeqNum = num;

		// num >= 1 && num <= (m_Len - 3)	Data Block
		// num == (m_Len - 2)				Tate Sum
		// num == (m_Len)					Yoko Sum (Last Line)

		if ( num < ISH_NUM_YOKO )
			break;

	DECBLOCK:
		line = (m_DecLine < ISH_LINE_LEN ? m_DecLine : ISH_LINE_LEN);

		if ( (m_DecLine -= line) <= 0 )
			m_Stat = 0;

		err = 0;
		for ( int n = 1 ; n <= line ; n++ ) {
			if ( *GetBuf(n, 0) == 0 && err < 3 ) {
				*GetBuf(n, 0) = (BYTE)n;
				bad[err++] = n;
			}
		}

		if ( err > 0 && !ErrCollect(err, bad[0], bad[1]) ) {
			m_Stat = 0;
			retcode = ISH_RET_ABORT;
			break;
		}

		for ( int n = 1 ; n <= line ; n++ ) {
			if ( m_Size > 0 ) {
				len = (int)(m_Size < (LONGLONG)ISH_COLS_LEN ? m_Size : (LONGLONG)ISH_COLS_LEN);
				m_Size -= len;
				out.Apend(GetBuf(n, 1), len);
				m_FileCRC = CalcCRC(GetBuf(n, 1), len, m_FileCRC);
				m_FileSize += len;
				if ( m_Size <= 0 && m_AddSize > 0 && len < ISH_COLS_LEN ) {
					int add = ISH_COLS_LEN - len;
					if ( add > m_AddSize )
						add = m_AddSize;
					m_AddSize -= add;
					m_AddBuf.Apend(GetBuf(n, 1 + len), add);
				}
			} else if ( m_AddSize > 0 ) {
				len = m_AddSize < ISH_COLS_LEN ? m_AddSize : ISH_COLS_LEN;
				m_AddSize -= len;
				m_AddBuf.Apend(GetBuf(n, 1), len);
			}
		}

		if ( m_DecLine <= 0 && m_AddBuf.GetSize() >= 2 ) {
			m_FileCRC = CalcCRC(m_AddBuf.GetPtr(), 2, m_FileCRC);
			if ( m_FileCRC != ISH_CRC_MATCH ) {
				m_Stat = 0;
				retcode = ISH_RET_ABORT;
				break;
			}
		}

		ZeroMemory(m_Buf, m_Len * m_Len);
		m_SeqNum = 0;

		retcode = (m_Stat == 0 ? ISH_RET_ENDOF : ISH_RET_DATA);
		break;
	}

	return retcode;
}

//////////////////////////////////////////////////////////////////////

void CIsh::EncodeHead(LPCSTR filename, LONGLONG filesize, time_t mtime, CBuffer &out)
{
	int n;
	BYTE tmp[128];
	int block;
	LPCSTR s;
    struct tm *tm;
	DWORD ftm = 0;
	CBuffer work;
	CStringA mbs;
	CString version;
	static const char *types[] = { "jis7", "jis8", "shift_jis" };

	m_Stat = 0;
	m_Type = ISH_TYPE_JIS7;
	m_Len  = ISH_LEN_JIS7;
	m_Size = filesize;
	m_FileCRC = ISH_CRC_INIT;

	block = m_Len - 1;
	tm = localtime(&mtime);

	// yyyy yyym mmmd dddd hhhh hmmm mmms ssss
	ftm  = (tm->tm_year - 80) << 25;
    ftm |= (tm->tm_mon  + 1)  << 21;
    ftm |=  tm->tm_mday       << 16;
    ftm |=  tm->tm_hour       << 11;
    ftm |=  tm->tm_min        << 5;
    ftm |=  tm->tm_sec        >> 1;

	if ( m_Buf != NULL )
		delete [] m_Buf;
	m_Buf = new BYTE[m_Len * m_Len];

	ASSERT(m_Len < 128);
	ZeroMemory(tmp, m_Len);

	tmp[0] = 0;						// sinc
	tmp[1] = 0;						// itype
	tmp[2] = (BYTE)(m_Size);		// fsize[4]
	tmp[3] = (BYTE)(m_Size >> 8);
	tmp[4] = (BYTE)(m_Size >> 16);
	tmp[5] = (BYTE)(m_Size >> 24);
	tmp[6] = (BYTE)(m_Len);			// line[2]
	tmp[7] = (BYTE)(m_Len >> 8);
	tmp[8] = (BYTE)(block);			// block[2]
	tmp[9] = (BYTE)(block >> 8);

	switch(m_Type) {
	case ISH_TYPE_JIS7:
		tmp[10] = 16;
		tmp[11] = 13;
		break;
	case ISH_TYPE_JIS8:
		tmp[10] = 8;
		tmp[11] = 8;
		break;
	case ISH_TYPE_SJIS:
		tmp[10] = 16;
		tmp[11] = 15;
		break;
	}

	s = filename;
	for ( n = 0 ; n < 11 ; n++ ) {
		if ( *s == '\0' )
			tmp[12 + n] = ' ';		// iname[11]
		else if ( *s == '.' ) {
			for ( ; n < 8 ; n++ )
				tmp[12 + n] = ' ';
			s++;
			n--;
		} else
			tmp[12 + n] = *(s++);
	}

	tmp[23] = 0;					// janle
	tmp[24] = 0x00;					// os		MS_DOS=0x00, CP_M=0x10, OS_9=0x20, UNIX=0x30
	tmp[25] = 1;					// exttype
	tmp[26] = 1;					// tstamp

	tmp[27] = (BYTE)(ftm);			// time[4]
	tmp[28] = (BYTE)(ftm >> 8);
	tmp[29] = (BYTE)(ftm >> 16);
	tmp[30] = (BYTE)(ftm >> 24);

	SetCRC(tmp, m_Len - 2);

	if ( m_Type == ISH_TYPE_JIS8 )
		work.IshEncJis8(tmp, m_Len);
	else
		work.IshEncJis7(tmp, m_Len);
	work.PutByte('\n');

	n = (int)((filesize + 2 + ISH_COLS_LEN - 1) / ISH_COLS_LEN);	// data line
	n =  n + (n / ISH_LINE_LEN + 1) * 2 + 3;						// tateyokosum(2) + head(3)
	((CRLoginApp *)AfxGetApp())->GetVersion(version);

	mbs.Format("<<< %s for MS-DOS ( use %s ish ) [ %d lines ] RLogin ver%s >>>\n", filename, types[m_Type], n, TstrToMbs(version));
	out.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());

	out.Apend(work.GetPtr(), work.GetSize());
	out.Apend(work.GetPtr(), work.GetSize());
	out.Apend(work.GetPtr(), work.GetSize());
}
int CIsh::EncodeBlock(FILE *fp, CBuffer &out)
{
	int n, num, len;
	int rsize = 0;
	BYTE *tmp;

	ASSERT(m_Buf != NULL);

	if ( m_Stat >= 2 )
		return rsize;

	ZeroMemory(m_Buf, m_Len * m_Len);

	for ( num = 1 ; m_Stat < 2 && num <= ISH_LINE_LEN ; num++ ) {
		tmp = GetBuf(num, 0);

		if ( m_Stat == 0 ) {
			len = (m_Size < ISH_COLS_LEN ? (int)m_Size : ISH_COLS_LEN);

			if ( len > 0 && (len = (int)fread(tmp + 1, 1, len, fp)) > 0 )
				m_FileCRC = CalcCRC(tmp + 1, len, m_FileCRC);
			else
				len = 0;

			rsize += len;
			m_Size -= len;

			if ( (n = ISH_COLS_LEN - len) >= 2 ) {
				m_FileCRC = ~m_FileCRC;
				tmp[1 + len] = (BYTE)(m_FileCRC >> 8);
				tmp[1 + len + 1] = (BYTE)(m_FileCRC);
				len += 2;
				m_Stat = 2;
			} else if ( n >= 1 ) {
				m_FileCRC = ~m_FileCRC;
				tmp[1 + len] = (BYTE)(m_FileCRC >> 8);
				len += 1;
				m_Stat = 1;
			}
		} else {	// m_Stat == 1
			tmp[1] = (BYTE)(m_FileCRC);
			len = 1;
			m_Stat = 2;
		}

		if ( len < ISH_COLS_LEN )
			memset(tmp + 1 + len, 0, ISH_COLS_LEN - len);

		tmp[0] = num;
		SetCRC(tmp, ISH_COLS_MAX);
		SetSum(num, ISH_COLS_MAX);
	}

	// tatesum
	*GetBuf(ISH_NUM_TATE, 0) = ISH_NUM_TATE;
	SetSum(ISH_NUM_TATE, ISH_COLS_MAX);
	SetCRC(GetBuf(ISH_NUM_TATE, 0), ISH_COLS_MAX);

	// yokosum
	*GetBuf(ISH_NUM_YOKO, 0) = ISH_NUM_YOKO;
	SetCRC(GetBuf(ISH_NUM_YOKO, 0), ISH_COLS_MAX);

	for ( num = 1 ; num <= m_Len ; num++ ) {
		if ( *GetBuf(num, 0) == 0 )
			continue;

		switch(m_Type) {
		case ISH_TYPE_JIS7: out.IshEncJis7(GetBuf(num, 0), m_Len); break;
		case ISH_TYPE_JIS8: out.IshEncJis8(GetBuf(num, 0), m_Len); break;
		case ISH_TYPE_SJIS: out.IshEncSjis(GetBuf(num, 0), m_Len); break;
		}
		out.PutByte('\n');
	}

	return rsize;
}
