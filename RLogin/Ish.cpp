// Ish.cpp : 実装ファイル
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
	m_IshSize = 0;
	m_Buf  = NULL;
}
CIsh::~CIsh()
{
	if ( m_Buf != NULL )
		delete [] m_Buf;
}

//////////////////////////////////////////////////////////////////////

WORD CIsh::CalcCRC(LPBYTE buf, int len, WORD crc)
{
	while( len-- > 0 )
		crc = (crc << 8) ^ crc16tab[(BYTE)((crc >> 8) ^ *(buf++))];
	return crc;
}
DWORD CIsh::CalcCRC32(LPBYTE buf, int len, DWORD crc)
{
    while( len-- > 0 )
		crc = ((crc >> 8) & 0xffffff) ^ crc32tab[(BYTE)(crc ^ *(buf++))];
    return crc;
}
BOOL CIsh::ChkCRC(CBuffer &buf)
{
	return (ISH_CRC_MATCH == CalcCRC(buf.GetPtr(), buf.GetSize()) ? TRUE : FALSE);
}
void CIsh::SetCRC(LPBYTE buf, int len)
{
	WORD crc = ~CalcCRC(buf, len);
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
		*GetBuf(e2, 0) = (BYTE)e2;
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
void CIsh::GetFileCRC(FILE *fp)
{
	int n, len;
	WORD vcrc;
	LONGLONG vlen;
	BYTE tmp[1024];

	m_FileCRC = vcrc = ISH_CRC_INIT;
	m_FileCRC32 = ISH_CRC32_INIT;

	vlen = m_VolSize;
	m_VolCRCTab.RemoveAll();

	_fseeki64(fp, 0, SEEK_SET);

	while ( (len = (int)fread(tmp, 1, 1024, fp)) > 0 ) {
		m_FileCRC = CalcCRC(tmp, len, m_FileCRC);
		m_FileCRC32 = CalcCRC32(tmp, len, m_FileCRC32);

		n = (int)(vlen < len ? vlen : len);
		vcrc = CalcCRC(tmp, n, vcrc);

		if ( (vlen -= n) <= 0 ) {
			m_VolCRCTab.Add(~vcrc);
			vcrc = ISH_CRC_INIT;
			vlen = m_VolSize;
		}

		if ( n < len ) {
			vcrc = CalcCRC(tmp + n, len - n, vcrc);
			vlen -= (len - n);
		}
	}

	m_FileCRC   = ~m_FileCRC;
	m_FileCRC32 = ~m_FileCRC32;

	if ( vlen < m_VolSize )
		m_VolCRCTab.Add(~vcrc);

	_fseeki64(fp, 0, SEEK_SET);
}

//////////////////////////////////////////////////////////////////////

int CIsh::DecodeLine(LPCSTR str, CBuffer &out)
{
	int n, m;
	int num, line, len, err, bad[3];
	int type[2], block;
	CBuffer work;
	LPCSTR p;
	BYTE *pos;
	int st, dt;
	long tz;
	struct tm tm;
	WORD vcrc;
	DWORD fcrc;
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

		if ( work.GetSize() <= 60 || !ChkCRC(work) || *(work.GetPos(0)) != 0 )
			break;

		m_VolMax = 0;
		m_VolSeq = *(work.GetPos(1));
		m_VolSize = 0;
		m_VolChkName.Empty();
		m_bMultiVolume = FALSE;

		m_Len = *(work.GetPos(6)) | (*(work.GetPos(7)) << 8);
		block = *(work.GetPos(8)) | (*(work.GetPos(9)) << 8) + 1;
		type[0] = *(work.GetPos(10));
		type[1] = *(work.GetPos(11));

		if (      type[0] == 16 && type[1] == 13 && m_Len == ISH_LEN_JIS7 && block == ISH_LEN_JIS7 )
			m_Type = ISH_TYPE_JIS7;
		else if ( type[0] ==  8 && type[1] ==  8 && m_Len == ISH_LEN_JIS8 && block == ISH_LEN_JIS8 )
			m_Type = ISH_TYPE_JIS8;
		else if ( type[0] == 16 && type[1] == 15 && m_Len == ISH_LEN_SJIS && block == ISH_LEN_SJIS )
			m_Type = ISH_TYPE_SJIS;
		else if ( type[0] == 16 && type[1] == 14 && m_Len == ISH_LEN_NJIS && block == ISH_LEN_NJIS )
			m_Type = ISH_TYPE_NJIS;
		else
			break;

		m_AddSize = 2;
		m_AddBuf.Clear();
		m_WorkCRC = ISH_CRC_INIT;
		m_WorkSize = 0;
		m_FileName.Empty();
		m_FileSeek = 0;
		m_FileTime = 0;

		pos = work.GetPos(2);			// fsize[4]
		m_FileSize  =  (LONGLONG)*(pos++);
		m_FileSize |= ((LONGLONG)*(pos++) << 8);
		m_FileSize |= ((LONGLONG)*(pos++) << 16);
		m_FileSize |= ((LONGLONG)*(pos++) << 24);
		m_IshSize = m_FileSize;

		pos = work.GetPos(26);
		st = *(pos++);					// tstamp 01=time, 02=dtime, 04=fcrc16, 08=fcrc32, 10=vsize/vcrc16

		dt = 0;
		tz = 0;
		fcrc = 0;

		if ( (st & 0x01) != 0 ) {		// localtime
			dt  = *(pos++);
			dt |= *(pos++) << 8;
			dt |= *(pos++) << 16;
			dt |= *(pos++) << 24;
		}

		if ( (st & 0x02) != 0 ) {		// dtime
			tz |= *(pos++);
			tz  = *(pos++) << 8;
			tz |= (signed char)(*(pos++)) << 16;
		}

		if ( (st & 0x01) != 0 ) {
			memset(&tm, 0, sizeof(tm));

			// yyyy yyym mmmd dddd hhhh hmmm mmms ssss
			tm.tm_year =  (dt >> 25) + 80;
			tm.tm_mon  = ((dt >> 21) & 0x0F) - 1;
			tm.tm_mday =  (dt >> 16) & 0x1F;
			tm.tm_hour =  (dt >> 11) & 0x1F;
			tm.tm_min  =  (dt >>  5) & 0x3F;
			tm.tm_sec  =  (dt <<  1) & 0x3F;	// XXXX

			if ( (st & 0x02) != 0 ) {	// have dtime
				m_FileTime = _mkgmtime(&tm);
				m_FileTime += tz;
			} else
				m_FileTime = mktime(&tm);
		} else
			time(&m_FileTime);

		if ( (st & 0x04) != 0 ) { 		// fcrc16[2]
			fcrc  =  (DWORD)*(pos++);
			fcrc |= ((DWORD)*(pos++) << 8);
			m_FileCRC = (WORD)fcrc;
		}

		if ( (st & 0x08) != 0 ) {		// fcrc32[4]
			fcrc  =  (DWORD)*(pos++);
			fcrc |= ((DWORD)*(pos++) << 8);
			fcrc |= ((DWORD)*(pos++) << 16);
			fcrc |= ((DWORD)*(pos++) << 24);
			m_FileCRC32 = fcrc;
		}

		if ( (st & 0x10) != 0 ) {		// multi volume size & crc
			m_VolSize  =  (LONGLONG)*(pos++);
			m_VolSize |= ((LONGLONG)*(pos++) << 8);
			m_VolSize |= ((LONGLONG)*(pos++) << 16);
			m_VolSize |= ((LONGLONG)*(pos++) << 24);

			vcrc  =  (WORD)*(pos++);		// vcrc[2]
			vcrc |= ((WORD)*(pos++) << 8);

			while ( m_VolCRCTab.GetSize() <= m_VolSeq )
				m_VolCRCTab.Add(0);
			m_VolCRCTab[m_VolSeq - 1] = vcrc;
		}

		if ( m_VolSeq > 0 && m_VolSize > 0 && m_IshSize >= m_VolSize ) {
			m_VolMax = (int)((m_FileSize + m_VolSize - 1) / m_VolSize);
			if ( m_VolSeq >= m_VolMax )
				m_IshSize = m_FileSize - m_VolSize * (m_VolMax - 1);
			else
				m_IshSize = m_VolSize;

			m_FileSeek = (m_VolSeq - 1) * m_VolSize;
		} else
			m_VolSeq = 0;

		p = (LPCSTR)(work.GetPos(12));		// iname[11]

		for ( m = 8 ; m > 0 && p[m - 1] == ' ' ; )
			m--;
		for ( n = 0 ; n < m ; n++ )
			m_FileName += p[n];
		for ( m = 11 ; m > 8 && p[m - 1] == ' ' ; )
			m--;

		if ( m_VolSeq != 0 )
			m_VolChkName.Format(_T("%s_%04x"), MbsToTstr(m_FileName), fcrc);
		else {
			if ( m > 8 )
				m_FileName += '.';
			for ( n = 8 ; n < m ; n++ )
				m_FileName += p[n];
		}

		if ( m_Buf != NULL )
			delete [] m_Buf;

		m_Buf = new BYTE[m_Len * m_Len];
		ZeroMemory(m_Buf, m_Len * m_Len);
		m_DecLine = (int)((m_IshSize + m_AddSize + ISH_COLS_LEN - 1) / (LONGLONG)ISH_COLS_LEN);
		m_SeqNum = 0;
		m_Stat = 1;
		retcode = ISH_RET_HEAD;
		break;

	case 1:
		switch(m_Type) {
		case ISH_TYPE_JIS7: work.IshDecJis7(str); break;
		case ISH_TYPE_JIS8: work.IshDecJis8(str); break;
		case ISH_TYPE_SJIS: work.IshDecSjis(str); break;
		case ISH_TYPE_NJIS: work.IshDecNjis(str); break;
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
			retcode = ISH_RET_ECCERR;
			break;
		}

		for ( int n = 1 ; n <= line ; n++ ) {
			if ( m_IshSize > 0 ) {
				len = (int)(m_IshSize < (LONGLONG)ISH_COLS_LEN ? m_IshSize : (LONGLONG)ISH_COLS_LEN);
				m_IshSize -= len;
				out.Apend(GetBuf(n, 1), len);
				m_WorkCRC = CalcCRC(GetBuf(n, 1), len, m_WorkCRC);
				m_WorkSize += len;
				if ( m_IshSize <= 0 && m_AddSize > 0 && len < ISH_COLS_LEN ) {
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
			m_WorkCRC = CalcCRC(m_AddBuf.GetPtr(), 2, m_WorkCRC);
			if ( m_WorkCRC != ISH_CRC_MATCH ) {
				m_Stat = 0;
				retcode = ISH_RET_CRCERR;
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

void CIsh::SetHeader(CBuffer &out)
{
	int n;
	BYTE tmp[128];
	int block;
	LPCSTR s;
    struct tm *tm;
	long tz = 0;
	DWORD ftm;
	CBuffer work;
	CStringA mbs, seq;
	CString version;
	static const char *types[] = { "jis7", "jis8", "shift_jis", "non-kana" };

	block = m_Len - 1;
	tm = localtime(&m_FileTime);
	_get_timezone(&tz);

	// yyyy yyym mmmd dddd hhhh hmmm mmms ssss
	ftm  = (tm->tm_year - 80) << 25;
    ftm |= (tm->tm_mon  + 1)  << 21;
    ftm |=  tm->tm_mday       << 16;
    ftm |=  tm->tm_hour       << 11;
    ftm |=  tm->tm_min        << 5;
    ftm |=  tm->tm_sec        >> 1;

	ZeroMemory(tmp, 128);

	tmp[0] = 0;								// sinc

	if ( m_bMultiVolume ) {
		tmp[1] = (BYTE)m_VolSeq;			// itype
		tmp[2] = (BYTE)(m_FileSize);		// fsize[4]
		tmp[3] = (BYTE)(m_FileSize >> 8);
		tmp[4] = (BYTE)(m_FileSize >> 16);
		tmp[5] = (BYTE)(m_FileSize >> 24);
	} else {
		tmp[1] = 0;							// itype
		tmp[2] = (BYTE)(m_IshSize);			// fsize[4]
		tmp[3] = (BYTE)(m_IshSize >> 8);
		tmp[4] = (BYTE)(m_IshSize >> 16);
		tmp[5] = (BYTE)(m_IshSize >> 24);
	}

	tmp[6] = (BYTE)(m_Len);					// line[2]
	tmp[7] = (BYTE)(m_Len >> 8);
	tmp[8] = (BYTE)(block);					// block[2]
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
	case ISH_TYPE_NJIS:
		tmp[10] = 16;
		tmp[11] = 14;
		break;
	}

	s = m_FileName;
	for ( n = 0 ; n < 8 ; ) {
		if ( *s == '\0' || *s == '.' )
			break;
		else
			tmp[12 + n++] = *(s++);
	}
	for ( ; n < 8 ; n++ )
		tmp[12 + n] = ' ';

	if ( !m_bMultiVolume && m_VolSeq > 0 ) {
		tmp[12 +  8] = (BYTE)('0' + ((m_VolSeq / 100) % 10));
		tmp[12 +  9] = (BYTE)('0' + ((m_VolSeq / 10) % 10));
		tmp[12 + 10] = (BYTE)('0' +  (m_VolSeq % 10));
	} else if ( (s = strrchr(m_FileName, '.')) != NULL ) {
		s++;
		for ( n = 0 ; n < 3 ; ) {
			if ( *s == '\0' || *s == '.' )
				break;
			else
				tmp[12 + 8 + n++] = *(s++);
		}
		for ( ; n < 3 ; n++ )
			tmp[12 + 8 + n] = ' ';
	} else {
		tmp[12 +  8] = ' ';
		tmp[12 +  9] = ' ';
		tmp[12 + 10] = ' ';
	}

	tmp[23] = 0;							// janle
	tmp[24] = 0x00;							// os		MS_DOS=0x00, CP_M=0x10, OS_9=0x20, UNIX=0x30
	tmp[25] = 1;							// exttype
	tmp[26] = 0;							// tstamp	01=time, 02=dtime, 04=crc16, 08=crc32, 10=vsize+vcrc16

	tmp[26] |= 0x01;
	tmp[27] = (BYTE)(ftm);					// time[4]
	tmp[28] = (BYTE)(ftm >> 8);
	tmp[29] = (BYTE)(ftm >> 16);
	tmp[30] = (BYTE)(ftm >> 24);

	tmp[26] |= 0x02;
	tmp[31] = (BYTE)(tz);					// dtime[4]
	tmp[32] = (BYTE)(tz >> 8);
	tmp[33] = (BYTE)(tz >> 16);

	if ( m_bMultiVolume && m_VolSeq > 0 ) {
		tmp[26] |= 0x04;
		tmp[34] = (BYTE)(m_FileCRC >> 8);		// fcrc16[2] Hi Low
		tmp[35] = (BYTE)(m_FileCRC);

		tmp[26] |= 0x08;
		tmp[36] = (BYTE)(m_FileCRC32);			// fcrc32[4]
		tmp[37] = (BYTE)(m_FileCRC32 >> 8);
		tmp[38] = (BYTE)(m_FileCRC32 >> 16);
		tmp[39] = (BYTE)(m_FileCRC32 >> 24);

		tmp[26] |= 0x10;
		tmp[40] = (BYTE)(m_VolSize);			// vsize[4]
		tmp[41] = (BYTE)(m_VolSize >> 8);
		tmp[42] = (BYTE)(m_VolSize >> 16);
		tmp[43] = (BYTE)(m_VolSize >> 24);

		n = (m_VolCRCTab.GetSize() >= m_VolSeq ? m_VolCRCTab[m_VolSeq - 1] : 0);
		tmp[44] = (BYTE)(n >> 8);				// vcrc16[2] Hi Low
		tmp[45] = (BYTE)(n);
	}

	if ( m_Type == ISH_TYPE_JIS8 ) {
		SetCRC(tmp, ISH_LEN_JIS8 - 2);
		work.IshEncJis8(tmp, ISH_LEN_JIS8);
	} else {
		SetCRC(tmp, ISH_LEN_JIS7 - 2);
		work.IshEncJis7(tmp, ISH_LEN_JIS7);
	}
	work.PutByte('\n');

	n = (int)((m_IshSize + 2 + ISH_COLS_LEN - 1) / ISH_COLS_LEN);	// data line
	n =  n + (n / ISH_LINE_LEN + 1) * 2 + 3;						// tateyokosum(2) + head(3)
	((CRLoginApp *)AfxGetApp())->GetVersion(version);

	if ( m_VolSeq > 0 )
		seq.Format("(%03d)", m_VolSeq);

	mbs.Format("<<< %s%s for MS-DOS ( use %s ish ) [ %d lines ] RLogin ver%s >>>\n", m_FileName, seq, types[m_Type], n, TstrToMbs(version));
	out.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());

	out.Apend(work.GetPtr(), work.GetSize());
	out.Apend(work.GetPtr(), work.GetSize());
	out.Apend(work.GetPtr(), work.GetSize());
}
BOOL CIsh::EncodeHead(int type, FILE *fp, LPCSTR filename, LONGLONG filesize, time_t mtime, CBuffer &out)
{
	switch(type) {
	default:
	case ISH_TYPE_JIS7:
		m_Type = ISH_TYPE_JIS7;
		m_Len  = ISH_LEN_JIS7;
		break;
	case ISH_TYPE_JIS8:
		m_Type = ISH_TYPE_JIS8;
		m_Len  = ISH_LEN_JIS8;
		break;
	case ISH_TYPE_SJIS:
		m_Type = ISH_TYPE_SJIS;
		m_Len  = ISH_LEN_SJIS;
		break;
	case ISH_TYPE_NJIS:
		m_Type = ISH_TYPE_NJIS;
		m_Len  = ISH_LEN_NJIS;
		break;
	}

	m_FileName = filename;
	m_FileSize = filesize;
	m_FileTime = mtime;

	m_WorkCRC = ISH_CRC_INIT;
	m_IshSize = m_WorkSize = m_FileSize;

	// ファイルサイズが32bitsの制限があるのでISH_SPLIT_SIZE以上の場合に分割するようにした
	// マルチボリュームでもファイルサイズが32bitsなので単純な分割だけ行う
	// 最大分割数にも制限がある。マルチボリュームは、255で分割は、999が最大サイズ

	//#define	ISH_VOL_LINE		1000
	//#define	ISH_SPLIT_SIZE		(ISH_COLS_LEN * (ISH_VOL_LINE - ((ISH_VOL_LINE + ISH_LINE_LEN - 1 - 3) / ISH_LINE_LEN) * 2 - 3) - 2)

	#define	ISH_SPLIT_SIZE		0x3FFFFFFFL

	m_VolSeq = m_VolMax = 0;
	m_VolSize = ISH_SPLIT_SIZE;
	m_bMultiVolume = FALSE;

	if ( (m_VolMax = (int)((m_FileSize + m_VolSize - 1) / m_VolSize)) > 999 )
		return FALSE;
	else if ( m_VolMax > 1 ) {
		m_VolSeq = 1;
		m_IshSize = m_VolSize;
		m_WorkSize -= m_IshSize;
		// m_bMultiVolume = TRUE;
	}

	if ( m_bMultiVolume )
		GetFileCRC(fp);

	if ( m_Buf != NULL )
		delete [] m_Buf;
	m_Buf = new BYTE[m_Len * m_Len];

	m_Stat = 0;

	SetHeader(out);

	return TRUE;
}
int CIsh::EncodeBlock(FILE *fp, CBuffer &out)
{
	int n, num, len;
	int rsize = 0;
	BYTE *tmp;

	ASSERT(m_Buf != NULL);

	if ( m_Stat >= 2 ) {
		if ( m_VolSeq == 0 || m_WorkSize <= 0 )
			return (-1);

		m_WorkCRC = ISH_CRC_INIT;
		m_IshSize = (m_WorkSize < m_VolSize ? m_WorkSize : m_VolSize);

		m_WorkSize -= m_IshSize;
		m_VolSeq++;
		m_Stat = 0;

		SetHeader(out);
	}

	ZeroMemory(m_Buf, m_Len * m_Len);

	for ( num = 1 ; m_Stat < 2 && num <= ISH_LINE_LEN ; num++ ) {
		tmp = GetBuf(num, 0);

		if ( m_Stat == 0 ) {
			len = (m_IshSize < ISH_COLS_LEN ? (int)m_IshSize : ISH_COLS_LEN);

			if ( len > 0 && (len = (int)fread(tmp + 1, 1, len, fp)) > 0 )
				m_WorkCRC = CalcCRC(tmp + 1, len, m_WorkCRC);
			else
				len = 0;

			rsize += len;
			m_IshSize -= len;

			if ( (n = ISH_COLS_LEN - len) >= 2 ) {
				m_WorkCRC = ~m_WorkCRC;
				tmp[1 + len] = (BYTE)(m_WorkCRC >> 8);
				tmp[1 + len + 1] = (BYTE)(m_WorkCRC);
				len += 2;
				m_Stat = 2;
			} else if ( n >= 1 ) {
				m_WorkCRC = ~m_WorkCRC;
				tmp[1 + len] = (BYTE)(m_WorkCRC >> 8);
				len += 1;
				m_Stat = 1;
			}
		} else {	// m_Stat == 1
			tmp[1] = (BYTE)(m_WorkCRC);
			len = 1;
			m_Stat = 2;
		}

		if ( len < ISH_COLS_LEN )
			memset(tmp + 1 + len, 0, ISH_COLS_LEN - len);

		tmp[0] = (BYTE)num;
		SetCRC(tmp, ISH_COLS_MAX);
		SetSum(num, ISH_COLS_MAX);
	}

	// tatesum
	*GetBuf(ISH_NUM_TATE, 0) = (BYTE)ISH_NUM_TATE;
	SetSum(ISH_NUM_TATE, ISH_COLS_MAX);
	SetCRC(GetBuf(ISH_NUM_TATE, 0), ISH_COLS_MAX);

	// yokosum
	*GetBuf(ISH_NUM_YOKO, 0) = (BYTE)ISH_NUM_YOKO;
	SetCRC(GetBuf(ISH_NUM_YOKO, 0), ISH_COLS_MAX);

	for ( num = 1 ; num <= m_Len ; num++ ) {
		if ( *GetBuf(num, 0) == 0 )
			continue;

		switch(m_Type) {
		case ISH_TYPE_JIS7: out.IshEncJis7(GetBuf(num, 0), m_Len); break;
		case ISH_TYPE_JIS8: out.IshEncJis8(GetBuf(num, 0), m_Len); break;
		case ISH_TYPE_SJIS: out.IshEncSjis(GetBuf(num, 0), m_Len); break;
		case ISH_TYPE_NJIS: out.IshEncNjis(GetBuf(num, 0), m_Len); break;
		}
		out.PutByte('\n');
	}

	return rsize;
}
