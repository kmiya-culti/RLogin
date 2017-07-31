// ZModem.cpp: CZModem クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rlogin.h"
#include "ZModem.h"

#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define ENQ 005
#define CAN ('X'&037)
#define XOFF ('s'&037)
#define XON ('q'&037)
#define SOH 1
#define STX 2
#define EOT 4
#define ACK 6
#define NAK 025
#define CPMEOF 032
#define WANTCRC 0103    /* send C not NAK to get crc not checksum */
#define TIMEOUT (-2)
#define RCDO (-3)
#define GCOUNT (-4)
#define ERRORMAX 5
#define RETRYMAX 5
#define WCEOT (-10)
/* #define PATHLEN 257	/* ready for 4.2 bsd ? */
/* #define UNIXFILE 0xF000 /* The S_IFMT file mask bit for stat */

#define	ZBUFSIZE	(8 * 1024)

#define	DSZ
#define	UNSL		unsigned
#define TIMEOUT		(-2)
#define	ERR			(-1)

#define	xsendline(c)	Bufferd_Send(c)
#define	sendline(c)		Bufferd_Send(c)
#define	flushmo()		Bufferd_Flush()
#define	readline(s)		Bufferd_Recive(s)
#define	bttyout(c)		SendEcho(c)

/*
 * Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 * code or tables extracted from it, as desired without restriction.
 */

/* First, the polynomial itself and its table of feedback terms.  The  */
/* polynomial is                                                       */
/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
/* Note that we take it "backwards" and put the highest-order term in  */
/* the lowest-order bit.  The X^32 term is "implied"; the LSB is the   */
/* X^31 term, etc.  The X^0 term (usually shown as "+1") results in    */
/* the MSB being 1.                                                    */

/* Note that the usual hardware shift register implementation, which   */
/* is what we're using (we're merely optimizing it by doing eight-bit  */
/* chunks at a time) shifts bits into the lowest-order term.  In our   */
/* implementation, that means shifting towards the right.  Why do we   */
/* do it this way?  Because the calculated CRC must be transmitted in  */
/* order from highest-order term to lowest-order term.  UARTs transmit */
/* characters in order from LSB to MSB.  By storing the CRC this way,  */
/* we hand it to the UART in the order low-byte to high-byte; the UART */
/* sends each low-bit to hight-bit; and the result is transmission bit */
/* by bit from highest- to lowest-order term without requiring any bit */
/* shuffling on our part.  Reception works similarly.                  */

/* The feedback terms table consists of 256, 32-bit entries.  Notes:   */
/*                                                                     */
/*     The table can be generated at runtime if desired; code to do so */
/*     is shown later.  It might not be obvious, but the feedback      */
/*     terms simply represent the results of eight shift/xor opera-    */
/*     tions for all combinations of data and CRC register values.     */
/*                                                                     */
/*     The values must be right-shifted by eight bits by the "updcrc"  */
/*     logic; the shift must be unsigned (bring in zeroes).  On some   */
/*     hardware you could probably optimize the shift in assembler by  */
/*     using byte-swap instructions.                                   */

const unsigned long crc32tab[] = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#define UPDC32(b, c)	(crc32tab[((int)c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF))
#define updcrc(cp, crc) (crc16tab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CZModem::CZModem(class CRLoginDoc *pDoc, CWnd *pWnd) : CSyncSock(pDoc, pWnd)
{
	Zmodem    = 0;
	Zrwindow  = 2400;
	Effbaud   = 4800;
	Zctlesc   = 0;
	Rxtimeout = 10;
	Rxbuflen  = 1024;

	Crc32t = 1;
	Crc32r = 1;
	Txpos = 0;
	Txfcs32 = 0;
	Usevhdrs = 0;
	Znulls = 0;
}
CZModem::~CZModem()
{
}
void CZModem::OnProc(int cmd)
{
	switch(cmd) {
	case 1: XUpLoad(); break;
	case 2: YUpLoad(); break;
	case 3: ZUpDown(1); break;
	case 4: XDownLoad(); break;
	case 5: YDownLoad(); break;
	case 6: ZUpDown(2); break;
	case 7: ZUpDown(0); break;
	}
}

//////////////////////////////////////////////////////////////////////

int CZModem::CalCRC(char *ptr, int len)
{
    int crc;

    crc = 0;
    while ( len-- > 0 )
        crc = crc16tab[((crc >> 8) ^ *(ptr++)) & 0xFF] ^ (crc << 8);
    return (crc & 0xFFFF);
}
int CZModem::Send_blk(FILE *fp, int bk, int len, int crcopt, int hd)
{
    int     n;
    int     sz;
    int     sum;
    char    head[3];
    char    tmp[1024];
	struct _stati64 st;
	CStringA str;

    head[0] = len >= 1024 ? STX:SOH;
    head[1] = bk;
    head[2] = 255-bk;

	if ( bk == 0 && len >= 1024 && hd > 0 ) {
		sz = 0;
		if ( hd == 1 ) {
			strncpy(tmp, m_FileName, 1023);
			while ( tmp[sz++] != '\0' ) ;
			if ( !_tstati64(m_PathName, &st) ) {
				str.Format("%I64d %I64o %o", st.st_size, st.st_mtime, st.st_mode);
				if ( (strlen(tmp) + strlen(str) + 2) < 1024 )
					strcpy(tmp + sz, str);
				while ( tmp[sz++] != '\0' ) ;
			}
		}
		if ( sz <= 128 ) {
			head[0] = SOH;
			len = 128;
		}
	} else {
	    if ( (sz = (int)fread(tmp, 1, len, fp)) <= 0 ) {
			Bufferd_Send(EOT);
			Bufferd_Flush();
			return sz;
		}
		if ( len >= 1024 && sz <= 896 ) {
			head[0] = SOH;
			len = 128;
			if ( sz > 128 ) {
				_fseeki64(fp, (LONGLONG)(0 - sz + 128), SEEK_CUR);
				sz = 128;
			}
		}
	}

    for ( n = sz ; n < len ; n++ )
        tmp[n] = '\0';

    Bufferd_SendBuf(head, 3);
    Bufferd_SendBuf(tmp, len);

    if ( crcopt ) {
        sum = CalCRC(tmp, len);
        Bufferd_Send(sum >> 8);
        Bufferd_Send(sum);

    } else {
        for ( sum = n = 0 ; n < len ; n++ )
            sum += tmp[n];
        Bufferd_Send(sum);
    }

    Bufferd_Flush();
    return len;
}
int CZModem::Recive_blk(char *tmp, int block, int crcopt)
{
	int ch, bk, sum;
	int len = 128;

RECHK:
	if ( (ch = Bufferd_Recive(10)) < 0 )
		return ch;
	else if ( ch == EOT )
		return EOT;
	else if ( ch == CAN && Bufferd_Recive(1) == CAN )
		return CAN;
	else if ( ch == SOH )
		len = 128;
	else if ( ch == STX )
		len = 1024;
	else
		goto RECHK;

	if ( (bk = Bufferd_Recive(10)) < 0 )
		return bk;
	if ( (ch = Bufferd_Recive(10)) < 0 )
		return ch;
	if ( bk != (255 - ch) || bk != block )
		goto RECHK;

	if ( Bufferd_ReciveBuf(tmp, len, 10) != FALSE )
		goto RECHK;

	if ( crcopt ) {
		sum = CalCRC(tmp, len);
		if ( (bk = Bufferd_Recive(10)) < 0 )
			return bk;
		if ( (ch = Bufferd_Recive(10)) < 0 )
			return ch;
		if ( sum != ((bk << 8) | ch) )
			goto RECHK;
	} else {
		for ( sum = ch = 0 ; ch < len ; ch++ )
			sum += tmp[ch];
		if ( (ch = Bufferd_Recive(10)) < 0 )
			return ch;
		if ( ch != (sum & 0xFF) )
			goto RECHK;
	}

	return (len == 128 ? SOH : STX);
}
int CZModem::XUpLoad()
{
    FILE *fp = NULL;
    int ch;
    int bk = 1;
    int nk = 0;
    int ef = FALSE;
	int crcopt = 0;
    int send_len = 0;
	int block_len = 128;
    LONGLONG file_size;
    LONGLONG now_pos = 0L;

    SetXonXoff(FALSE);

	CheckFileName(1, "");

	if ( m_PathName.IsEmpty() )
		goto CANRET;

    if ( (fp = _tfopen(m_PathName, _T("rb"))) == NULL )
		goto CANRET;

    _fseeki64(fp, 0L, SEEK_END);
    file_size = _ftelli64(fp);
    _fseeki64(fp, 0L, SEEK_SET);

    UpDownOpen("XModem File Upload");
    UpDownInit(file_size);

	while ( !AbortCheck() ) {

        ch = Bufferd_Recive(60);

        if ( ch == ACK ) {
			if ( ef != FALSE )
				goto ENDRET;

            nk = 0;
            bk = (bk + 1) & 0xFF;
            now_pos += send_len;
		    UpDownStat(now_pos);

        } else if ( ch == NAK ) {
            if ( ++nk >= 10 )
				break;

            _fseeki64(fp, now_pos, SEEK_SET);

        } else if ( ch == WANTCRC && Bufferd_ReciveSize() == 0 ) {
            crcopt = TRUE;

		} else if ( ch == CAN && Bufferd_Recive(1) == CAN ) {
			Bufferd_Clear();
			goto ENDRET;

		} else if ( ch < 0 ) {
			break;

        } else {
			SendEcho(ch);
            continue;
        }

		if ( (send_len = Send_blk(fp, bk, block_len, crcopt, 0)) == 0 ) {
            ef = TRUE;
		} else if ( send_len < 0 )
			break;
    }

CANRET:
	Bufferd_Clear();
	Bufferd_Send(CAN);
	Bufferd_Send(CAN);
	Bufferd_Flush();

ENDRET:
    SetXonXoff(TRUE);
	if ( fp != NULL )
	    fclose(fp);
    UpDownClose();
    return FALSE;
}
int CZModem::XDownLoad()
{
    FILE *fp = NULL;
    int ch;
    int bk = 1;
    int nk = 0;
	int st = 0;
	int crcopt = FALSE;
    char tmp[1024];
    LONGLONG file_size = 4096L;
    LONGLONG now_pos = 0L;

    SetXonXoff(FALSE);

	CheckFileName(0, "");

	if ( m_PathName.IsEmpty() )
		goto CANRET;

	if ( (fp = _tfopen(m_PathName, _T("wb"))) == NULL )
		goto CANRET;

    UpDownOpen("XModem File Download");
	UpDownInit(file_size);

	while ( !AbortCheck() ) {

		if ( st == 0 ) {
			Bufferd_Send(WANTCRC);
		    Bufferd_Flush();
			crcopt = TRUE;
		} else if ( st == 1 ) {
			if ( ++nk > 5 )
				break;
			Bufferd_Send(NAK);
		    Bufferd_Flush();
		} else {
			Bufferd_Send(ACK);
		    Bufferd_Flush();
			bk = (bk + 1) & 255;
			nk = 0;
			st = 1;
		}

		ch = Recive_blk(tmp, bk, crcopt);

        if ( ch == EOT ) {
            Bufferd_Send(ACK);
		    Bufferd_Flush();
            goto ENDRET;
		} else if ( ch == CAN ) {
			Bufferd_Clear();
            goto ENDRET;
        } else if ( ch == SOH ) {
			if ( fwrite(tmp, 1, 128, fp) != 128 )
				break;
			st = 2;
			now_pos += 128;
		    UpDownStat(now_pos);
        } else if ( ch == STX ) {
			if ( fwrite(tmp, 1, 1024, fp) != 1024 )
				break;
			st = 2;
			now_pos += 1024;
		    UpDownStat(now_pos);
		} else if ( st == 0 ) {
			st = 1;
			crcopt = FALSE;
		} else
			st = 1;

		if ( now_pos > (file_size * 3 / 4) ) {
			file_size = now_pos * 4;
			UpDownInit(file_size);
		}
    }

CANRET:
	Bufferd_Clear();
    Bufferd_Send(CAN);
    Bufferd_Send(CAN);
    Bufferd_Flush();

ENDRET:
    SetXonXoff(TRUE);
	if ( fp != NULL )
	    fclose(fp);
    UpDownClose();
    return FALSE;
}

//////////////////////////////////////////////////////////////////////

int CZModem::YUpLoad()
{
    FILE *fp = NULL;
    int ch;
    int bk = 1;
    int nk = 0;
    int ef = FALSE;
	int crcopt = 0;
    int send_len = 0;
	int block_len = 1024;
    LONGLONG file_size;
    LONGLONG now_pos = 0L;

    SetXonXoff(FALSE);

	CheckFileName(1, "");

	if ( m_PathName.IsEmpty() )
		goto CANRET;

    if ( (fp = _tfopen(m_PathName, _T("rb"))) == NULL )
		goto CANRET;

    _fseeki64(fp, 0L, SEEK_END);
    file_size = _ftelli64(fp);
    _fseeki64(fp, 0L, SEEK_SET);

    UpDownOpen("YModem File Upload");
    UpDownInit(file_size);

	while ( !AbortCheck() ) {
		ch = Bufferd_Recive(60);

		if ( ch == WANTCRC )
			Send_blk(fp, 0, block_len, TRUE, 1);
		else if ( ch == ACK )
			break;
		else if ( ch == CAN && Bufferd_Recive(1) == CAN ) {
			Bufferd_Clear();
			goto ENDRET;
		} else if ( ch < 0 )
			goto CANRET;
		else
			SendEcho(ch);
	}

	while ( !AbortCheck() ) {
        ch = Bufferd_Recive(60);

        if ( ch == ACK ) {
			if ( ef != FALSE )
				break;

            nk = 0;
            bk = (bk + 1) & 0xFF;
            now_pos += send_len;
		    UpDownStat(now_pos);

        } else if ( ch == NAK ) {
            if ( ++nk >= 10 )
				goto CANRET;

            _fseeki64(fp, now_pos, SEEK_SET);

        } else if ( ch == WANTCRC && Bufferd_ReciveSize() == 0 ) {
            crcopt = TRUE;

		} else if ( ch == CAN && Bufferd_Recive(1) == CAN ) {
			Bufferd_Clear();
			goto ENDRET;

		} else if ( ch < 0 ) {
			goto CANRET;

        } else {
			SendEcho(ch);
            continue;
        }

		if ( (send_len = Send_blk(fp, bk, block_len, crcopt, 0)) == 0 ) {
            ef = TRUE;
		} else if ( send_len < 0 )
			goto CANRET;
    }

	while ( !AbortCheck() ) {
		ch = Bufferd_Recive(60);

		if ( ch == WANTCRC )
			Send_blk(fp, 0, block_len, TRUE, 2);
		else if ( ch == ACK )
			goto ENDRET;
		else if ( ch == CAN && Bufferd_Recive(1) == CAN ) {
			Bufferd_Clear();
			goto ENDRET;
		} else if ( ch < 0 )
			goto CANRET;
		else
			SendEcho(ch);
	}

CANRET:
	Bufferd_Clear();
	Bufferd_Send(CAN);
	Bufferd_Send(CAN);
	Bufferd_Flush();

ENDRET:
    SetXonXoff(TRUE);
    fclose(fp);
    UpDownClose();
    return FALSE;
}
int CZModem::YDownLoad()
{
	int n;
	int ch;
	int bk = 1;
	int nk = 0;
	int st = 0;
	char *p;
    char tmp[1024];
    FILE *fp;
    LONGLONG file_size;
    LONGLONG now_pos;
	time_t file_time;
	struct _utimbuf utm;

    SetXonXoff(FALSE);

RELOAD:
	fp = NULL;
	file_size = now_pos = 0L;
	file_time = 0;

	nk = 0;

	for ( ; ; ) {
		if ( AbortCheck() )
			goto CANRET;

		Bufferd_Send(WANTCRC);
		Bufferd_Flush();

		ch = Recive_blk(tmp, 0, TRUE);

		if ( ch == CAN ) {
			Bufferd_Clear();
			goto ENDRET;
		} else if ( ch == SOH || ch == STX )
			break;

		if ( ++nk > 4 )
			goto CANRET;
	}

	Bufferd_Send(ACK);
    Bufferd_Flush();

	if ( tmp[0] == '\0' && tmp[1] == '\0' )
		goto ENDRET;

	p = tmp;
	while ( *(p++) != '\0' ) ;	// skip file name
	while ( *p >= '0' && *p <= '9' )
		file_size = file_size * 10 + (*(p++) - '0');

	while ( *p == ' ' ) p++;
	while ( *p >= '0' && *p <= '7' )
		file_time = file_time * 8 + (*(p++) - '0');

	CheckFileName(0, tmp);

	if ( m_PathName.IsEmpty() )
		goto CANRET;

	if ( (fp = _tfopen(m_PathName, _T("wb"))) == NULL )
		goto CANRET;

    UpDownOpen("YModem File Download");
	UpDownInit(file_size);

	st = 0;
	nk = 0;
	bk = 1;

	for ( ; ; ) {
		if ( AbortCheck() )
			goto CANRET;

		if ( st == 0 ) {
			Bufferd_Send(WANTCRC);
		    Bufferd_Flush();
			st = 1;
		} else if ( st == 1 ) {
			if ( ++nk > 5 )
				goto CANRET;
			Bufferd_Send(NAK);
		    Bufferd_Flush();
		} else {
			Bufferd_Send(ACK);
		    Bufferd_Flush();
			bk = (bk + 1) & 255;
			nk = 0;
			st = 1;
		}

		ch = Recive_blk(tmp, bk, TRUE);

        if ( ch == EOT ) {
            Bufferd_Send(ACK);
		    Bufferd_Flush();
            break;
		} else if ( ch == CAN ) {
			Bufferd_Clear();
            goto ENDRET;
        } else if ( ch == SOH ) {
			n = ((file_size - now_pos) < 128 ? (int)(file_size - now_pos) : 128);
			if ( fwrite(tmp, 1, n, fp) != n )
				goto CANRET;
			st = 2;
			now_pos += n;
		    UpDownStat(now_pos);
        } else if ( ch == STX ) {
			n = ((file_size - now_pos) < 1024 ? (int)(file_size - now_pos) : 1024);
			if ( fwrite(tmp, 1, n, fp) != n )
				goto CANRET;
			st = 2;
			now_pos += n;
		    UpDownStat(now_pos);
		}
	}

	fclose(fp);
    UpDownClose();

	if ( file_time != 0 ) {
		utm.actime  = (time_t)file_time;
		utm.modtime = (time_t)file_time;
		_tutime(m_PathName, &utm);
	}
	goto RELOAD;

CANRET:
	Bufferd_Clear();
	Bufferd_Send(CAN);
	Bufferd_Send(CAN);
	Bufferd_Flush();

ENDRET:
    SetXonXoff(TRUE);
	if ( fp != NULL )
	    fclose(fp);
    UpDownClose();
    return FALSE;
}

//////////////////////////////////////////////////////////////////////

void CZModem::zperr(LPCSTR s)
{
	UpDownMessage((char *)s);
}
int CZModem::ZUpFile()
{
	int c, n, e;
	long sz;
	char txbuf[ZBUFSIZE + 4];
	LONGLONG fileSize  = 0;
	FILE *fp = NULL;
	long crc;
	int sendStat = 1;
	int recvStat = 0;
	int ackCount = 0;
	int winCount = 1;
	int nakCount = 0;
	int toutFlag = FALSE;
	struct _stati64 st;
	CStringA opt;

NEXTFILE:

	CheckFileName(3, "");

	if ( m_PathName.IsEmpty() )
		return ERR;

	if ( _tstati64(m_PathName, &st) )
		return ERR;

	if ( st.st_size > 0x7FFFFFFF ) {
		zperr("File Size 2GByte Over");
		return ERR;
	}

    if ( (fp = _tfopen(m_PathName, _T("rb"))) == NULL )
		return ERR;

	fileSize = st.st_size;
	Rxpos = Txpos = 0L;

	UpDownOpen("ZModem File Upload");
	UpDownInit(fileSize);

	for ( ; ; ) {
		if ( AbortCheck() )
			goto ERRRET;
		
		switch(sendStat) {
		case 1:
			Txhdr[ZF0] = (char)ZCBIN;				/* file conversion request */
			Txhdr[ZF1] = (char)ZMCLOB;				/* file management request */
			Txhdr[ZF2] = (char)ZTRLE;				/* file transport request */
			Txhdr[ZF3] = 0;
			zshhdr(4, ZFILE, Txhdr);

			opt.Format("%I64d %I64o %o 0 0 0", fileSize, st.st_mtime, st.st_mode);
			if ( (n = m_FileName.GetLength() + opt.GetLength() + 2) > ZBUFSIZE ) {
				char *tmp = new char[n + 2];
				strcpy(tmp, m_FileName);
				strcpy(tmp + m_FileName.GetLength() + 1, opt);
				zsdata(tmp, n, ZCRCW);
				delete tmp;
			} else {
				strcpy(txbuf, m_FileName);
				strcpy(txbuf + m_FileName.GetLength() + 1, opt);
				zsdata(txbuf, n, ZCRCW);
			}

			sendStat = 0;
			recvStat = 0;
			break;

		case 2:
			crc = 0xFFFFFFFFL;
			if ( _fseeki64(fp, 0L, 0) )
				goto ERRRET;
			if ( Rxpos == 0 )
				Rxpos = fileSize;
			for ( sz = 0 ; sz < Rxpos && (c = fgetc(fp)) != EOF ; sz++ )
				crc = UPDC32(c, crc);
			clearerr(fp);
			crc = ~crc;
			stohdr(crc);
			zsbhdr(4, ZCRC, Txhdr);
			sendStat = 0;
			recvStat = 0;
			break;

		case 3:
			stohdr(Txpos);
			zsbhdr(4, ZDATA, Txhdr);
			TRACE("ZDATA %d\n", Txpos);
		case 4:
			n = (int)fread(txbuf, 1, Rxbuflen, fp);

			if ( n < Rxbuflen ) {
				e = ZCRCE;
				sendStat = 5;
				recvStat = 1;
			} else {
				//e = ZCRCW;	// req ZRPOS
				e = ZCRCQ;		// req ACK
				//e = ZCRCG;	// not ACK
				sendStat = 4;
				ackCount++;
				recvStat = (Rxcanfdx == 0 || ackCount >= winCount ? 0 : 1);
			}

			zsdata(txbuf, n, e);
			flushmo();
			Txpos += n;
			TRACE("SDATA %d(%c)\n", n, e);
			break;

		case 5:
			stohdr(Txpos);
			zsbhdr(4, ZEOF, Txhdr);
			sendStat = 6;
			recvStat = 0;
			TRACE("ZEOF %d\n", Txhdr);
			break;
		}

		if ( recvStat == 1 ) {
			while ( Bufferd_ReciveSize() > 0 ) {
				if ( (c = readline(1)) == CAN || c == ZPAD )
					goto HDRCHECK;
				else if ( c == XOFF || c == (XOFF|0200) )
					readline(100);		// want XON
				
			}
			continue;
		}

	HDRCHECK:

		c = zgethdr(Rxhdr, 0);
		TRACE("zgethdr %02x\n", c);

		switch(c) {
		case ZRINIT:
			Rxcanfdx = (Rxhdr[ZF0] & CANFDX)  ? 1 : 0;
			Txfcs32  = (Rxhdr[ZF0] & CANRLE)  ? 2 : ((Rxhdr[ZF0] & CANFC32) ? 1 : 0);
			Zctlesc  = (Rxhdr[ZF0] & TESCCTL) ? 1 : 0;
			Usevhdrs = (Rxhdr[ZF1] & CANVHDR) ? 1 : 0;

			if ( Rxbuflen = (0377 & Rxhdr[ZP0]) + ((0377 & Rxhdr[ZP1]) << 8) <= 0 )
				Rxbuflen = 1024;
			else if ( Rxbuflen > ZBUFSIZE )
				Rxbuflen = ZBUFSIZE;

			if ( sendStat == 6 ) {
				fclose(fp);
				UpDownClose();
				if ( m_ResvPath.IsEmpty() )
					return FALSE;
				sendStat = 1;
				recvStat = 0;
				goto NEXTFILE;
			}

			winCount = 1;
			ackCount = 0;
			nakCount = 0;
			sendStat = 1;
			break;

		case ZNAK:
		case ZSKIP:
			zperr("ZUpload Got NAK/SKIP");
			fclose(fp);
			UpDownClose();
			return FALSE;

		case ZACK:
			if ( --ackCount < 0 )
				ackCount = 0;
			if ( winCount < ZM_PKTQUE )
				winCount++;
			toutFlag = FALSE;
			nakCount = 0;
			UpDownStat(Rxpos);
			break;

		case ZCRC:
			sendStat = 2;
			break;

		case ZRPOS:
			if ( ++nakCount > 4 )
				goto ERRRET;
			if ( _fseeki64(fp, Rxpos, 0) )
				goto ERRRET;
			Txpos = Rxpos;
		case TIMEOUT:
			sendStat  = 3;
			if ( toutFlag && c == TIMEOUT )
				goto ERRRET;
			winCount = 1;
			ackCount = 0;
			toutFlag = TRUE;
			TRACE("ZRPOS %d\n", Txpos);
			break;

		default:
			goto ERRRET;
		}
	}

ERRRET:
	stohdr(0);
	zshhdr(4, ZABORT, Txhdr);
	fclose(fp);
	UpDownClose();
	return ERR;
}
int CZModem::ZDownFile()
{
	int c, n;
	char buf[ZBUFSIZE + 4];
	LONGLONG fileSize  = 0;
	long fileMtime = 0;
	int fileMode = 0666;
	FILE *fp = NULL;
	BOOL haveData = FALSE;
	long crc;

	if ( zrdata(buf, ZBUFSIZE) != GOTCRCW ) {
		stohdr(0L);
		zshhdr(4, ZSKIP, Txhdr);
		return FALSE;
	}

	CheckFileName(0, buf);

	if ( m_PathName.IsEmpty() )
		goto ERRRET;

	char *p = buf + 1 + strlen(buf);
	if ( *p != '\0' )
		sscanf(p, "%I64d %lo %o", &fileSize, &fileMtime, &fileMode);

	Txpos = 0;

	if ( (fp = _tfopen(m_PathName, _T("rb+"))) != NULL ) {
		crc = 0xFFFFFFFFL;
		for ( ; (c = fgetc(fp)) != EOF ; Txpos++ )
			crc = UPDC32(c, crc);
		crc = ~crc;
		clearerr(fp);
		_fseeki64(fp, Txpos, SEEK_SET);

		stohdr(Txpos);
		zshhdr(4, ZCRC, Txhdr);

		if ( zgethdr(Rxhdr, 0) != ZCRC || crc != Rxpos ) {
			fclose(fp);
			fp = NULL;
			Txpos = 0;
		} else if ( fileSize != 0 && fileSize == Txpos ) {
			fclose(fp);
			stohdr(0);
			zshhdr(4, ZSKIP, Txhdr);
			return FALSE;
		}
	}
	
	if ( fp == NULL && (fp = _tfopen(m_PathName, _T("wb"))) == NULL )
		goto ERRRET;

	UpDownOpen("ZModem File Download");
	UpDownInit(fileSize, Txpos);

	for ( ; ; ) {
		stohdr(Txpos);
		zshhdr(4, ZRPOS, Txhdr);

	READHDR:
		if ( AbortCheck() )
			goto ERRRET;

		c = zgethdr(Rxhdr, 0);
		TRACE("zgethdr %02x\n", c);

		switch(c) {
		case ZDATA:
			UpDownStat(Txpos);
			if ( (Txpos & 0xFFFFFFFFL) != rclhdr(Rxhdr) ) {
				zrdata(buf, ZBUFSIZE);
				break;
			}
			for ( n = 0 ; (c = zrdata(buf, ZBUFSIZE)) >= 0 ; n++ ) {
				TRACE("ZDATA %c\n", c);
				haveData = TRUE;
				Txpos += Rxcount;
				if ( fwrite(buf, 1, Rxcount, fp) != (UINT)Rxcount )
					goto ERRRET;
				if ( c == GOTCRCQ || c == GOTCRCW ) {
					stohdr(Txpos);
					zshhdr(4, ZACK, Txhdr);
				}
				if ( c == GOTCRCE || c == GOTCRCW )
					break;
				if ( n > 10 ) {
					UpDownStat(Txpos);
					n = 0;
				}
			}
			goto READHDR;

		case ZEOF:
			UpDownStat(Txpos);
			if ( (Txpos & 0xFFFFFFFFL) != rclhdr(Rxhdr) )
				break;
			if ( fileSize != 0 && fileSize != Txpos ) {
				zperr("File Size Error");
				goto ERRRET;
			}
			fclose(fp);
			UpDownClose();
			if ( fileMtime != 0 ) {
				struct _utimbuf utm;
				utm.actime  = (time_t)fileMtime;
				utm.modtime = (time_t)fileMtime;
				_tutime(m_PathName, &utm);
			}
			return FALSE;

		case TIMEOUT:
			if ( !haveData )
				goto ERRRET;
			haveData = FALSE;
			break;

		default:
			goto ERRRET;
		}
	}

ERRRET:
	stohdr(0);
	zshhdr(4, ZABORT, Txhdr);
	if ( fp != NULL )
		fclose(fp);
	UpDownClose();
	return ERR;
}
int CZModem::ZUpDown(int mode)
{
	int c;
	int finMode = 0;
	BOOL sendSinit = TRUE;
	static const char canistr[] = {
		24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
	};

	Zmodem    = 0;
	Zrwindow  = 2400;
	Effbaud   = 4800;
	Zctlesc   = 0;
	Rxtimeout = 10;
	Rxbuflen  = 1024;
	Rxcanfdx  = 1;

	Crc32t = 1;
	Crc32r = 1;
	Txpos = 0;
	Txfcs32 = 0;
	Usevhdrs = 0;
	Znulls = 0;

	switch(mode) {
	case 1:		// UpLoad
		stohdr(0L);
		zshhdr(4, ZRQINIT, Txhdr);
		break;
	case 2:		// DownLoad
		stohdr(ZBUFSIZE);
		Txhdr[ZF0] = (char)(CANFDX|CANOVIO|CANRLE);
		if ( Txfcs32 )
			Txhdr[ZF0] |= (char)CANFC32;
		if ( Zctlesc )
			Txhdr[ZF0] |= (char)TESCCTL;
		if ( Usevhdrs )
			Txhdr[ZF1] |= (char)CANVHDR;
		zshhdr(4, ZRINIT, Txhdr);
		break;
	}

	for ( ; ; ) {
		if ( AbortCheck() )
			goto CANRET;

		c = zgethdr(Rxhdr, (mode == 0 ? 0x82 : 2));
		TRACE("zgethdr %02x\n", c);
		mode = (-1);

		switch(c) {
		case ZRQINIT:
			Usevhdrs = (Rxhdr[ZF3] & 0x80) ? 1 : 0;

		SENDRINIT:
			stohdr(ZBUFSIZE);
			Txhdr[ZF0] = (char)(CANFDX|CANOVIO|CANRLE);
			if ( Txfcs32 )
				Txhdr[ZF0] |= (char)CANFC32;
			if ( Zctlesc )
				Txhdr[ZF0] |= (char)TESCCTL;
			if ( Usevhdrs )
				Txhdr[ZF1] |= (char)CANVHDR;
			zshhdr(4, ZRINIT, Txhdr);
			break;

		case ZRINIT:
			Rxcanfdx = (Rxhdr[ZF0] & CANFDX)  ? 1 : 0;
			Txfcs32  = (Rxhdr[ZF0] & CANRLE)  ? 2 : ((Rxhdr[ZF0] & CANFC32) ? 1 : 0);
			Zctlesc  = (Rxhdr[ZF0] & TESCCTL) ? 1 : 0;
			Usevhdrs = (Rxhdr[ZF1] & CANVHDR) ? 1 : 0;

			if ( Rxbuflen = (0377 & Rxhdr[ZP0]) + ((0377 & Rxhdr[ZP1]) << 8) <= 0 )
				Rxbuflen = 1024;
			else if ( Rxbuflen > ZBUFSIZE )
				Rxbuflen = ZBUFSIZE;

			if ( Bufferd_ReciveSize() > 2 )
				break;

			if ( !sendSinit && Zctlesc == 0 ) {
				sendSinit = TRUE;
				Zctlesc = 1;
				stohdr(0L);
				Txhdr[ZF0] |= TESCCTL;
				zshhdr(4, ZSINIT, Txhdr);
				zsdata("\003", 1, ZCRCW);
				Zctlesc = 0;
			} else if ( ZUpFile() ) {
				goto CANRET;
			} else {
				stohdr(0);
				zshhdr(4, ZFIN, Txhdr);
				finMode = 1;
			}
			break;

		case ZSINIT:
			Zctlesc  = (Rxhdr[ZF0] & TESCCTL) ? 1 : 0;

			if ( zrdata(Attn, ZATTNLEN) == GOTCRCW ) {
				stohdr(1L);
				zshhdr(4, ZACK, Txhdr);
			} else
				zshhdr(4, ZNAK, Txhdr);
			break;

		case ZFILE:
			Usevhdrs = (Rxhdr[ZF3] & ZCANVHDR) ? 1 : 0;

			finMode = 0;
			if ( ZDownFile() )
				goto CANRET;
			else
				goto SENDRINIT;
			break;

		case ZCHALLENGE:
			zshhdr(4, ZACK, Rxhdr);
			break;

		case ZFREECNT:
			stohdr(0L);
			zshhdr(4, ZACK, Txhdr);
			break;

		case ZACK:
		case ZNAK:
			stohdr(0L);
			zshhdr(4, ZRQINIT, Txhdr);
			break;

		case ZFIN:
			if ( finMode ) {
				sendline('O');
				sendline('O');
				flushmo();
			} else {
				stohdr(0);
				zsbhdr(4, ZFIN, Txhdr);
				if ( readline(10) == 'O' )
					readline(10);
			}
			goto ENDRET;

		case ZCAN:
			goto ENDRET;

		case ZSKIP:
		case ZABORT:
			stohdr(0);
			zshhdr(4, ZFIN, Txhdr);
			finMode = 1;
			break;

		case TIMEOUT:
		default:
			goto CANRET;
		}

	}
ENDRET:
	return FALSE;

CANRET:
	Bufferd_Clear();
	Bufferd_SendBuf((char *)canistr, strlen(canistr));
	Bufferd_Flush();
	return ERR;
}

/*
 *   Z M . C
 *    ZMODEM protocol primitives
 *    04-17-89  Chuck Forsberg Omen Technology Inc
 *
 * Entry point Functions:
 *	zsbhdr(type, hdr) send binary header
 *	zshhdr(type, hdr) send hex header
 *	zgethdr(hdr, eflag) receive header - binary or hex
 *	zsdata(buf, len, frameend) send data
 *	zrdata(buf, len) receive data
 *	stohdr(pos) store position data in Txhdr
 *	long rclhdr(hdr) recover position offset from header
 *
 *	This version implements ZMODEM Run Length Encoding, Comparision,
 *	and variable length headers.  These features were not funded
 *	by the original Telenet development contract.  This software,
 *	including these features, may be freely used for non
 *	commercial and educational purposes.  This software may also
 *	be freely used to support file transfer operations to or from
 *	licensed Omen Technology products.  Contact Omen Technology
 *	for licensing for other uses.  Any programs which use part or
 *	all of this software must be provided in source form with this
 *	notice intact except by written permission from Omen
 *	Technology Incorporated.
 *
 *		Omen Technology Inc		FAX: 503-621-3745
 *		Post Office Box 4681
 *		Portland OR 97208
 *
 *	Previous versions of this program (not containing the extensions
 *	listed above) remain in the public domain.
 *
 *	This code is made available in the hope it will be useful,
 *	BUT WITHOUT ANY WARRANTY OF ANY KIND OR LIABILITY FOR ANY
 *	DAMAGES OF ANY KIND.
 *
 */

static const char *frametypes[] = {
	"No Response to Error Correction Request",	/* -4 */
	"No Carrier Detect",		/* -3 */
	"TIMEOUT",		/* -2 */
	"ERROR",		/* -1 */
#define FTOFFSET 4
	"ZRQINIT",
	"ZRINIT",
	"ZSINIT",
	"ZACK",
	"ZFILE",
	"ZSKIP",
	"ZNAK",
	"ZABORT",
	"ZFIN",
	"ZRPOS",
	"ZDATA",
	"ZEOF",
	"ZFERR",
	"ZCRC",
	"ZCHALLENGE",
	"ZCOMPL",
	"ZCAN",
	"ZFREECNT",
	"ZCOMMAND",
	"ZSTDERR",
	"xxxxx"
#define FRTYPES 22	/* Total number of frame types in this array */
			/*  not including psuedo negative entries */
};

static const char badcrc[] = "Bad CRC";

/* Send ZMODEM binary header hdr of type type */
void CZModem::zsbhdr(int len, int type, char *hdr)
{
	register int n;
	register unsigned short crc;

#ifndef DSZ
	vfile("zsbhdr: %c %d %s %lx", Usevhdrs?'v':'f', len,
	  frametypes[type+FTOFFSET], rclhdr(hdr));
#endif
	if (type == ZDATA)
		for (n = Znulls; --n >=0; )
			xsendline(0);

	xsendline(ZPAD); xsendline(ZDLE);

	switch (Crc32t=Txfcs32) {
	case 2:
		zsbh32(len, hdr, type, Usevhdrs?ZVBINR32:ZBINR32);
		flushmo();  break;
	case 1:
		zsbh32(len, hdr, type, Usevhdrs?ZVBIN32:ZBIN32);  break;
	default:
		if (Usevhdrs) {
			xsendline(ZVBIN);
			zsendline(len);
		}
		else
			xsendline(ZBIN);
		zsendline(type);
		crc = updcrc(type, 0);

		for (n=len; --n >= 0; ++hdr) {
			zsendline(*hdr);
			crc = updcrc((0377& *hdr), crc);
		}
		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8);
		zsendline(crc);
	}
	if (type != ZDATA)
		flushmo();
}


/* Send ZMODEM binary header hdr of type type */
void CZModem::zsbh32(int len, char *hdr, int type, int flavour)
{
	register int n;
	register UNSL long crc;

	xsendline(flavour); 
	if (Usevhdrs) 
		zsendline(len);
	zsendline(type);
	crc = 0xFFFFFFFFL; crc = UPDC32(type, crc);

	for (n=len; --n >= 0; ++hdr) {
		crc = UPDC32((0377 & *hdr), crc);
		zsendline(*hdr);
	}
	crc = ~crc;
	for (n=4; --n >= 0;) {
		zsendline((int)crc);
		crc >>= 8;
	}
}

/* Send ZMODEM HEX header hdr of type type */
void CZModem::zshhdr(int len, int type, char *hdr)
{
	register int n;
	register unsigned short crc;

#ifndef DSZ
	vfile("zshhdr: %c %d %s %lx", Usevhdrs?'v':'f', len,
	  frametypes[type+FTOFFSET], rclhdr(hdr));
#endif
	sendline(ZPAD); sendline(ZPAD); sendline(ZDLE);
	if (Usevhdrs) {
		sendline(ZVHEX);
		zputhex(len);
	}
	else
		sendline(ZHEX);
	zputhex(type);
	Crc32t = 0;

	crc = updcrc(type, 0);
	for (n=len; --n >= 0; ++hdr) {
		zputhex(*hdr); crc = updcrc((0377 & *hdr), crc);
	}
	crc = updcrc(0,updcrc(0,crc));
	zputhex(crc>>8); zputhex(crc);

	/* Make it printable on remote machine */
	sendline(015); sendline(0212);
	/*
	 * Uncork the remote in case a fake XOFF has stopped data flow
	 */
	if (type != ZFIN && type != ZACK)
		sendline(021);
	flushmo();
}

/*
 * Send binary array buf of length length, with ending ZDLE sequence frameend
 */
static const char *Zendnames[] = { "ZCRCE", "ZCRCG", "ZCRCQ", "ZCRCW"};

void CZModem::zsdata(char *buf, int length, int frameend)
{
	register unsigned short crc;

#ifndef DSZ
	vfile("zsdata: %d %s", length, Zendnames[frameend-ZCRCE&3]);
#endif
	switch (Crc32t) {
	case 1:
		zsda32(buf, length, frameend);  break;
	case 2:
		zsdar32(buf, length, frameend);  break;
	default:
		crc = 0;
		for (;--length >= 0; ++buf) {
			zsendline(*buf); crc = updcrc((0377 & *buf), crc);
		}
		xsendline(ZDLE); xsendline(frameend);
		crc = updcrc(frameend, crc);

		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8); zsendline(crc);
	}
	if (frameend == ZCRCW) {
		xsendline(XON);  flushmo();
	}
}

void CZModem::zsda32(char *buf, int length, int frameend)
{
	register int c;
	register UNSL long crc;

	crc = 0xFFFFFFFFL;
	for (;--length >= 0; ++buf) {
		c = *buf & 0377;
		if (c & 0140)
			xsendline(lastsent = c);
		else
			zsendline(c);
		crc = UPDC32(c, crc);
	}
	xsendline(ZDLE); xsendline(frameend);
	crc = UPDC32(frameend, crc);

	crc = ~crc;
	for (c=4; --c >= 0;) {
		zsendline((int)crc);  crc >>= 8;
	}
}

/*
 * Receive array buf of max length with ending ZDLE sequence
 *  and CRC.  Returns the ending character or error code.
 *  NB: On errors may store length+1 bytes!
 */
int CZModem::zrdata(char *buf, int length)
{
	register int c;
	register unsigned short crc;
	register char *end;
	register int d;

	switch (Crc32r) {
	case 1:
		return zrdat32(buf, length);
	case 2:
		return zrdatr32(buf, length);
	}

	crc = Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				crc = updcrc((d=c)&0377, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if (crc & 0xFFFF) {
					zperr(badcrc);
					return ERR;
				}
				*buf = '\0';
				Rxcount = length - (int)(end - buf);
#ifndef DSZ
				vfile("zrdata: %d  %s", Rxcount,
				 Zendnames[d-GOTCRCE&3]);
#endif
				return d;
			case GOTCAN:
				zperr("Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT");
				return c;
			default:
				garbitch(); return c;
			}
		}
		*buf++ = c;
		crc = updcrc(c, crc);
	}
#ifdef DSZ
	garbitch(); 
#else
	zperr("Data subpacket too long");
#endif
	return ERR;
}

int CZModem::zrdat32(char *buf, int length)
{
	register int c;
	register UNSL long crc;
	register char *end;
	register int d;

	crc = 0xFFFFFFFFL;  Rxcount = 0;  end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				d = c;  c &= 0377;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if (crc != 0xDEBB20E3) {
					zperr(badcrc);
					return ERR;
				}
				*buf = '\0';
				Rxcount = length - (int)(end - buf);
#ifndef DSZ
				vfile("zrdat32: %d %s", Rxcount,
				 Zendnames[d-GOTCRCE&3]);
#endif
				return d;
			case GOTCAN:
				zperr("Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT");
				return c;
			default:
				garbitch(); return c;
			}
		}
		*buf++ = c;
		crc = UPDC32(c, crc);
	}
	zperr("Data subpacket too long");
	return ERR;
}

void CZModem::garbitch()
{
	zperr("Garbled data subpacket");
}

/*
 * Read a ZMODEM header to hdr, either binary or hex.
 *  eflag controls local display of non zmodem characters:
 *	0:  no display
 *	1:  display printing characters only
 *	2:  display all non ZMODEM characters
 *  On success, set Zmodem to 1, set Rxpos and return type of header.
 *   Otherwise return negative on error.
 *   Return ERROR instantly if ZCRCW sequence, for fast error recovery.
 */
int CZModem::zgethdr(char *hdr, int eflag)
{
	register int c, n, cancount;

	n = Zrwindow + Effbaud;		/* Max bytes before start of frame */
	Rxframeind = Rxtype = 0;

	if ( (eflag & 0x80) != 0 ) {
		eflag &= 0x7F;
		goto havepad;
	}
startover:
	cancount = 5;
again:
	/* Return immediate ERROR if ZCRCW sequence seen */
	switch (c = readline(Rxtimeout)) {
	case RCDO:
	case TIMEOUT:
		goto fifi;
	case CAN:
gotcan:
		if (--cancount <= 0) {
			c = ZCAN; goto fifi;
		}
		switch (c = readline(1)) {
		case TIMEOUT:
			goto again;
		case ZCRCW:
			switch (readline(1)) {
			case TIMEOUT:
				c = ERR; goto fifi;
			case RCDO:
				goto fifi;
			default:
				goto agn2;
			}
		case RCDO:
			goto fifi;
		default:
			break;
		case CAN:
			if (--cancount <= 0) {
				c = ZCAN; goto fifi;
			}
			goto again;
		}
	/* **** FALL THRU TO **** */
	default:
agn2:
		if ( --n == 0) {
			c = GCOUNT;  goto fifi;
		}
		if (eflag > 1)
			bttyout(c);
		else if (eflag && ((c &= 0177) & 0140))
			bttyout(c);
#ifdef UNIX
		fflush(stderr);
#endif
		goto startover;
	case ZPAD|0200:		/* This is what we want. */
		Not8bit = c;
	case ZPAD:		/* This is what we want. */
		break;
	}
havepad:
	cancount = 5;
splat:
	switch (c = noxrd7()) {
	case ZPAD:
		goto splat;
	case RCDO:
	case TIMEOUT:
		goto fifi;
	default:
		goto agn2;
	case ZDLE:		/* This is what we want. */
		break;
	}


	Rxhlen = 4;		/* Set default length */
	Rxframeind = c = noxrd7();
	switch (c) {
	case ZVBIN32:
		if ((Rxhlen = c = readline(Rxtimeout)) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 1;  c = zrbhd32(hdr); break;
	case ZBIN32:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 1;  c = zrbhd32(hdr); break;
	case ZVBINR32:
		if ((Rxhlen = c = readline(Rxtimeout)) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 2;  c = zrbhd32(hdr); break;
	case ZBINR32:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 2;  c = zrbhd32(hdr); break;
	case RCDO:
	case TIMEOUT:
		goto fifi;
	case ZVBIN:
		if ((Rxhlen = c = readline(Rxtimeout)) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 0;  c = zrbhdr(hdr); break;
	case ZBIN:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 0;  c = zrbhdr(hdr); break;
	case ZVHEX:
		if ((Rxhlen = c = zgethex()) < 0)
			goto fifi;
		if (c > ZMAXHLEN)
			goto agn2;
		Crc32r = 0;  c = zrhhdr(hdr); break;
	case ZHEX:
		if (Usevhdrs)
			goto agn2;
		Crc32r = 0;  c = zrhhdr(hdr); break;
	case CAN:
		goto gotcan;
	default:
		goto agn2;
	}
	Rxpos = hdr[ZP3] & 0377;
	Rxpos = (Rxpos<<8) + (hdr[ZP2] & 0377);
	Rxpos = (Rxpos<<8) + (hdr[ZP1] & 0377);
	Rxpos = (Rxpos<<8) + (hdr[ZP0] & 0377);
fifi:
	switch (c) {
	case GOTCAN:
		c = ZCAN;
	/* **** FALL THRU TO **** */
	case ZNAK:
	case ZCAN:
	case ERR:
	case TIMEOUT:
	case RCDO:
	case GCOUNT:
		if (c >= -4 && c <= FRTYPES)
			zperr("Got %s", frametypes[c+FTOFFSET]);
		else
			zperr("Got #%d", c);
	/* **** FALL THRU TO **** */
#ifndef DSZ
	default:
		if (c >= -4 && c <= FRTYPES)
			vfile("zgethdr: %c %d %s %lx", Rxframeind, Rxhlen,
			  frametypes[c+FTOFFSET], Rxpos);
		else
			vfile("zgethdr: %d %lx", c, Rxpos);
#endif
	}
	/* Use variable length headers if we got one */
	if (c >= 0 && c <= FRTYPES && Rxframeind & 040)
		Usevhdrs = 1;
	return c;
}

/* Receive a binary style header (type and position) */
int CZModem::zrbhdr(char *hdr)
{
	register int c, n;
	register unsigned short crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=Rxhlen; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr(badcrc);
		return ERR;
	}
#ifdef ZMODEM
	Protocol = ZMODEM;
#endif
	Zmodem = 1;
	return Rxtype;
}

/* Receive a binary style header (type and position) with 32 bit FCS */
int CZModem::zrbhd32(char *hdr)
{
	register int c, n;
	register UNSL long crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = 0xFFFFFFFFL; crc = UPDC32(c, crc);
#ifdef DEBUGZ
	vfile("zrbhd32 c=%X  crc=%lX", c, crc);
#endif

	for (n=Rxhlen; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = UPDC32(c, crc);
		*hdr = c;
#ifdef DEBUGZ
		vfile("zrbhd32 c=%X  crc=%lX", c, crc);
#endif
	}
	for (n=4; --n >= 0;) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = UPDC32(c, crc);
#ifdef DEBUGZ
		vfile("zrbhd32 c=%X  crc=%lX", c, crc);
#endif
	}
	if (crc != 0xDEBB20E3) {
		zperr(badcrc);
		return ERR;
	}
#ifdef ZMODEM
	Protocol = ZMODEM;
#endif
	Zmodem = 1;
	return Rxtype;
}


/* Receive a hex style header (type and position) */
int CZModem::zrhhdr(char *hdr)
{
	register int c;
	register unsigned short crc;
	register int n;

	if ((c = zgethex()) < 0)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=Rxhlen; --n >= 0; ++hdr) {
		if ((c = zgethex()) < 0)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr(badcrc); return ERR;
	}
	switch ( c = readline(1)) {
	case 0215:
		Not8bit = c;
		/* **** FALL THRU TO **** */
	case 015:
	 	/* Throw away possible cr/lf */
		switch (c = readline(1)) {
		case 012:
			Not8bit |= c;
		}
	}
#ifdef ZMODEM
	Protocol = ZMODEM;
#endif
	Zmodem = 1; return Rxtype;
}

/* Send a byte as two hex digits */
void CZModem::zputhex(int c)
{
	static const char	digits[]	= "0123456789abcdef";

#ifdef DEBUGZ
	if (Verbose>8)
		vfile("zputhex: %02X", c);
#endif
	sendline(digits[(c&0xF0)>>4]);
	sendline(digits[(c)&0xF]);
}

/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telenet net escape)
 */
void CZModem::zsendline(int c)
{

	/* Quick check for non control characters */
	if (c & 0140)
		xsendline(lastsent = c);
	else {
		switch (c &= 0377) {
		case ZDLE:
			xsendline(ZDLE);
			xsendline (lastsent = (c ^= 0100));
			break;
		case 015:
		case 0215:
			if (!Zctlesc && (lastsent & 0177) != '@')
				goto sendit;
		/* **** FALL THRU TO **** */
		case 020:
		case 021:
		case 023:
		case 0220:
		case 0221:
		case 0223:
			xsendline(ZDLE);
			c ^= 0100;
	sendit:
			xsendline(lastsent = c);
			break;
		default:
			if (Zctlesc && ! (c & 0140)) {
				xsendline(ZDLE);
				c ^= 0100;
			}
			xsendline(lastsent = c);
		}
	}
}

/* Decode two lower case hex digits into an 8 bit byte value */
int CZModem::zgethex()
{
	register int c;

	c = zgeth1();
#ifdef DEBUGZ
	if (Verbose>8)
		vfile("zgethex: %02X", c);
#endif
	return c;
}
int CZModem::zgeth1()
{
	register int c, n;

	if ((c = noxrd7()) < 0)
		return c;
	n = c - '0';
	if (n > 9)
		n -= ('a' - ':');
	if (n & ~0xF)
		return ERR;
	if ((c = noxrd7()) < 0)
		return c;
	c -= '0';
	if (c > 9)
		c -= ('a' - ':');
	if (c & ~0xF)
		return ERR;
	c += (n<<4);
	return c;
}

/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */
int CZModem::zdlread()
{
	register int c;

again:
	/* Quick check for non control characters */
	if ((c = readline(Rxtimeout)) & 0140)
		return c;
	switch (c) {
	case ZDLE:
		break;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again;
	default:
		if (Zctlesc && !(c & 0140)) {
			goto again;
		}
		return c;
	}
again2:
	if ((c = readline(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(Rxtimeout)) < 0)
		return c;
	switch (c) {
	case CAN:
		return GOTCAN;
	case ZCRCE:
	case ZCRCG:
	case ZCRCQ:
	case ZCRCW:
		return (c | GOTOR);
	case ZRUB0:
		return 0177;
	case ZRUB1:
		return 0377;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again2;
	default:
		if (Zctlesc && ! (c & 0140)) {
			goto again2;
		}
		if ((c & 0140) ==  0100)
			return (c ^ 0100);
		break;
	}
#ifdef DEBUGZ
	if (Verbose>1)
		zperr("Bad escape sequence %x", c);
#endif
	return ERR;
}

/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
int CZModem::noxrd7()
{
	register int c;

	for (;;) {
		if ((c = readline(Rxtimeout)) < 0)
			return c;
		switch (c &= 0177) {
		case XON:
		case XOFF:
			continue;
		default:
			if (Zctlesc && !(c & 0140))
				continue;
		case '\r':
		case '\n':
		case ZDLE:
			return c;
		}
	}
}

/* Store long integer pos in Txhdr */
void CZModem::stohdr(LONGLONG pos)
{
	Txhdr[ZP0] = (char)(pos);
	Txhdr[ZP1] = (char)(pos>>8);
	Txhdr[ZP2] = (char)(pos>>16);
	Txhdr[ZP3] = (char)(pos>>24);
}

/* Recover a long integer from a header */
LONGLONG CZModem::rclhdr(register char *hdr)
{
	LONGLONG l;

	l = (hdr[ZP3] & 0377);
	l = (l << 8) | (hdr[ZP2] & 0377);
	l = (l << 8) | (hdr[ZP1] & 0377);
	l = (l << 8) | (hdr[ZP0] & 0377);
	return l;
}

/* End of zm.c */

/*
 * File: zmr.c 12-04-1988
 * Copyright 1988 Omen Technology Inc All Rights Reserved
 *
 *
 *	This code implements ZMODEM Run Length Encoding, not funded
 *	by the original Telenet development contract.  This software,
 *	including these features, may be freely used for non
 *	commercial and educational purposes.  This software may also
 *	be freely used to support file transfer operations to or from
 *	licensed Omen Technology products.  Contact Omen Technology
 *	for licensing for other uses.  Any programs which use part or
 *	all of this software must be provided in source form with this
 *	notice intact except by written permission from Omen
 *	Technology Incorporated.
 *
 *		Omen Technology Inc		FAX: 503-621-3745
 *		Post Office Box 4681
 *		Portland OR 97208
 *
 *	This code is made available in the hope it will be useful,
 *	BUT WITHOUT ANY WARRANTY OF ANY KIND OR LIABILITY FOR ANY
 *	DAMAGES OF ANY KIND.
 *	ZMODEM RLE compression and decompression functions
 */

/* Send data subpacket RLE encoded with 32 bit FCS */
void CZModem::zsdar32(char *buf, int length, int frameend)
{
	register int c, l, n;
	register UNSL long crc;

	crc = 0xFFFFFFFFL;  l = *buf++ & 0377;
	if (length == 1) {
		zsendline(l); crc = UPDC32(l, crc);
		if (l == ZRESC) {
			zsendline(1); crc = UPDC32(1, crc);
		}
	} else {
		for (n = 0; --length >= 0; ++buf) {
			if ((c = *buf & 0377) == l && n < 126 && length>0) {
				++n;  continue;
			}
			switch (n) {
			case 0:
				zsendline(l);
				crc = UPDC32(l, crc);
				if (l == ZRESC) {
					zsendline(0100); crc = UPDC32(0100, crc);
				}
				l = c; break;
			case 1:
				if (l != ZRESC) {
					zsendline(l); zsendline(l);
					crc = UPDC32(l, crc);
					crc = UPDC32(l, crc);
					n = 0; l = c; break;
				}
				/* **** FALL THRU TO **** */
			default:
				zsendline(ZRESC); crc = UPDC32(ZRESC, crc);
				if (l == 040 && n < 34) {
					n += 036;
					zsendline(n); crc = UPDC32(n, crc);
				}
				else {
					n += 0101;
					zsendline(n); crc = UPDC32(n, crc);
					zsendline(l); crc = UPDC32(l, crc);
				}
				n = 0; l = c; break;
			}
		}
	}
	xsendline(ZDLE); xsendline(frameend);
	crc = UPDC32(frameend, crc);

	crc = ~crc;
	for (length=4; --length >= 0;) {
		zsendline((int)crc);  crc >>= 8;
	}
}


/* Receive data subpacket RLE encoded with 32 bit FCS */
int CZModem::zrdatr32(char *buf, int length)
{
	register int c;
	register UNSL long crc;
	register char *end;
	register int d;

	crc = 0xFFFFFFFFL;  Rxcount = 0;  end = buf + length;
	d = 0;	/* Use for RLE decoder state */
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				d = c;  c &= 0377;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = UPDC32(c, crc);
				if (crc != 0xDEBB20E3) {
					zperr(badcrc);
					return ERR;
				}
				*buf = '\0';
				Rxcount = length - (int)(end - buf);
#ifndef DSZ
				vfile("zrdatr32: %d %s", Rxcount,
				  Zendnames[d-GOTCRCE&3]);
#endif
				return d;
			case GOTCAN:
				zperr("Sender Canceled");
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT");
				return c;
			default:
				zperr("Bad data subpacket");
				return c;
			}
		}
		crc = UPDC32(c, crc);
		switch (d) {
		case 0:
			if (c == ZRESC) {
				d = -1;  continue;
			}
			*buf++ = c;  continue;
		case -1:
			if (c >= 040 && c < 0100) {
				d = c - 035; c = 040;  goto spaces;
			}
			if (c == 0100) {
				d = 0;
				*buf++ = ZRESC;  continue;
			}
			d = c;  continue;
		default:
			d -= 0100;
			if (d < 1)
				goto badpkt;
spaces:
			if ((buf + d) > end)
				goto badpkt;
			while ( --d >= 0)
				*buf++ = c;
			d = 0;  continue;
		}
	}
badpkt:
	zperr("Data subpacket too long");
	return ERR;
}
