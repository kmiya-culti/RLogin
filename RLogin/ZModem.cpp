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

#define SOH			001
#define STX			002
#define EOT			004
#define ENQ			005
#define ACK			006
#define	LF			012
#define	CR			015
#define	DLE			020
#define XON			021
#define XOFF		023
#define NAK			025
#define CAN			030
#define	GS			035

#define WANTCRC		0103	// 'C'

#define	ERR			(-1)
#define TIMEOUT		(-2)
#define RCDO		(-3)

#define	ZBUFSIZE	(8 * 1024)
#define FRTYPES		22
#define	RXTIMEOUT	10

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
/*     The values must be right-shifted by eight bits by the "UPDC16"  */
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
#define UPDC16(b, c)	(crc16tab[((c >> 8) & 255)] ^ (c << 8) ^ b)

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CZModem::CZModem(class CRLoginDoc *pDoc, CWnd *pWnd) : CSyncSock(pDoc, pWnd)
{
	Zctlesc   = 0;
	Rxbuflen  = 1024;
	Rxcanfdx  = 1;

	Txfcs32 = 0;
	Crc32t = 1;
	Crc32r = 1;

	Rxpos = 0;
	Txpos = 0;

	Usevhdrs = 0;
	memset(Attn, 0, sizeof(Attn));

	Znulls = 0;
	Not8bit = 0;
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
int CZModem::Receive_blk(char *tmp, int block, int crcopt)
{
	int ch, bk, sum;
	int len = 128;

RECHK:
	if ( (ch = Bufferd_Receive(10)) < 0 )
		return ch;
	else if ( ch == EOT )
		return EOT;
	else if ( ch == CAN && Bufferd_Receive(1) == CAN )
		return CAN;
	else if ( ch == SOH )
		len = 128;
	else if ( ch == STX )
		len = 1024;
	else
		goto RECHK;

	if ( (bk = Bufferd_Receive(10)) < 0 )
		return bk;
	if ( (ch = Bufferd_Receive(10)) < 0 )
		return ch;
	if ( bk != (255 - ch) || bk != block )
		goto RECHK;

	if ( !Bufferd_ReceiveBuf(tmp, len, 10) )
		goto RECHK;

	if ( crcopt ) {
		sum = CalCRC(tmp, len);
		if ( (bk = Bufferd_Receive(10)) < 0 )
			return bk;
		if ( (ch = Bufferd_Receive(10)) < 0 )
			return ch;
		if ( sum != ((bk << 8) | ch) )
			goto RECHK;
	} else {
		for ( sum = ch = 0 ; ch < len ; ch++ )
			sum += tmp[ch];
		if ( (ch = Bufferd_Receive(10)) < 0 )
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

	if ( !CheckFileName(CHKFILENAME_OPEN, "") )
		goto CANRET;

    if ( (fp = _tfopen(m_PathName, _T("rb"))) == NULL )
		goto CANRET;

    _fseeki64(fp, 0L, SEEK_END);
    file_size = _ftelli64(fp);
    _fseeki64(fp, 0L, SEEK_SET);

    UpDownOpen("XModem File Upload");
    UpDownInit(file_size);

	while ( Bufferd_ReceiveSize() > 1 ) {
		ch = Bufferd_Receive(0);

		if ( ch == WANTCRC )
            crcopt = TRUE;
		else if ( ch == CAN && Bufferd_Receive(1) == CAN ) {
			Bufferd_Clear();
			goto ENDRET;
		}
	}

	while ( !AbortCheck() ) {

        ch = Bufferd_Receive(60);

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

        } else if ( ch == WANTCRC && Bufferd_ReceiveSize() == 0 ) {
            crcopt = TRUE;

		} else if ( ch == CAN && Bufferd_Receive(1) == CAN ) {
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

	if ( !CheckFileName(CHKFILENAME_SAVE, "") )
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

		ch = Receive_blk(tmp, bk, crcopt);

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
	BOOL sh = FALSE;

    SetXonXoff(FALSE);

	if ( !CheckFileName(CHKFILENAME_OPEN, "") )
		goto CANRET;

    if ( (fp = _tfopen(m_PathName, _T("rb"))) == NULL )
		goto CANRET;

    _fseeki64(fp, 0L, SEEK_END);
    file_size = _ftelli64(fp);
    _fseeki64(fp, 0L, SEEK_SET);

    UpDownOpen("YModem File Upload");
    UpDownInit(file_size);

	while ( !AbortCheck() ) {
		ch = Bufferd_Receive(60);

		if ( ch == WANTCRC ) {
			if ( sh == FALSE ) {
				sh = TRUE;
				Send_blk(fp, 0, block_len, TRUE, 1);
			}
		} else if ( ch == NAK ) {
            if ( ++nk >= 10 )
				goto CANRET;
			sh = FALSE;
			Send_blk(fp, 0, block_len, TRUE, 1);
		} else if ( ch == ACK ) {
            nk = 0;
			break;
		} else if ( ch == CAN && Bufferd_Receive(1) == CAN ) {
			Bufferd_Clear();
			goto ENDRET;
		} else if ( ch < 0 )
			goto CANRET;
		else
			SendEcho(ch);
	}

	while ( !AbortCheck() ) {
        ch = Bufferd_Receive(60);

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

        } else if ( ch == WANTCRC && Bufferd_ReceiveSize() == 0 ) {
            crcopt = TRUE;

		} else if ( ch == CAN && Bufferd_Receive(1) == CAN ) {
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
		ch = Bufferd_Receive(60);

		if ( ch == WANTCRC )
			Send_blk(fp, 0, block_len, TRUE, 2);
		else if ( ch == ACK )
			goto ENDRET;
		else if ( ch == CAN && Bufferd_Receive(1) == CAN ) {
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

		ch = Receive_blk(tmp, 0, TRUE);

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

	if ( !CheckFileName(CHKFILENAME_SAVE, tmp) )
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

		ch = Receive_blk(tmp, bk, TRUE);

        if ( ch == EOT ) {
            Bufferd_Send(ACK);
		    Bufferd_Flush();
            break;
		} else if ( ch == CAN ) {
			Bufferd_Clear();
            goto ENDRET;
        } else if ( ch == SOH ) {
			n = (file_size > 0 && (file_size - now_pos) < 128 ? (int)(file_size - now_pos) : 128);
			if ( fwrite(tmp, 1, n, fp) != n )
				goto CANRET;
			st = 2;
			now_pos += n;
		    UpDownStat(now_pos);
        } else if ( ch == STX ) {
			n = (file_size > 0 && (file_size - now_pos) < 1024 ? (int)(file_size - now_pos) : 1024);
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

	if ( !CheckFileName(CHKFILENAME_OPEN | CHKFILENAME_MULTI, "") )
		return ERR;

	if ( _tstati64(m_PathName, &st) )
		return ERR;

	if ( st.st_size > 0x7FFFFFFF ) {
		ZDispError("File Size 2GByte Over");
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
			ZSendHexHeader(4, ZFILE, Txhdr);

			opt.Format("%I64d %I64o %o 0 0 0", fileSize, st.st_mtime, st.st_mode);
			if ( (n = m_FileName.GetLength() + opt.GetLength() + 2) > ZBUFSIZE ) {
				char *tmp = new char[n + 2];
				strcpy(tmp, m_FileName);
				strcpy(tmp + m_FileName.GetLength() + 1, opt);
				ZSendData(tmp, n, ZCRCW);
				delete [] tmp;
			} else {
				strcpy(txbuf, m_FileName);
				strcpy(txbuf + m_FileName.GetLength() + 1, opt);
				ZSendData(txbuf, n, ZCRCW);
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
			ZSetLongHeader(crc);
			ZSendBinHeader(4, ZCRC, Txhdr);
			sendStat = 0;
			recvStat = 0;
			break;

		case 3:
			ZSetLongHeader(Txpos);
			ZSendBinHeader(4, ZDATA, Txhdr);
			// no break

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

			ZSendData(txbuf, n, e);
			//Bufferd_Flush();
			Bufferd_Sync();

			Txpos += n;
			break;

		case 5:
			ZSetLongHeader(Txpos);
			ZSendBinHeader(4, ZEOF, Txhdr);

			sendStat = 6;
			recvStat = 0;
			break;
		}

		if ( recvStat == 1 ) {
			while ( Bufferd_ReceiveSize() > 0 ) {
				if ( (c = Bufferd_Receive(1)) == CAN || c == ZPAD )
					goto HDRCHECK;
				else if ( c == XOFF || c == (XOFF|0200) )
					Bufferd_Receive(100);		// want XON
				
			}
			continue;
		}

		HDRCHECK:

		c = ZReceiveHeader(Rxhdr, 0x80);

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
			ZDispError("ZUpload Got NAK/SKIP");
			fclose(fp);
			UpDownClose();
			return FALSE;

		case ZACK:
			if ( --ackCount < 0 ) {
				ackCount = 0;
				break;
			}
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
			// no break

		case TIMEOUT:
			sendStat  = 3;
			if ( toutFlag && c == TIMEOUT )
				goto ERRRET;
			winCount = 1;
			ackCount = 0;
			toutFlag = TRUE;
			break;

		default:
			goto ERRRET;
		}
	}

ERRRET:
	ZSetLongHeader(0);
	ZSendHexHeader(4, ZABORT, Txhdr);
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
	int Rxcount;

	if ( ZReceiveData(buf, ZBUFSIZE, Rxcount) != GOTCRCW ) {
		ZSetLongHeader(0L);
		ZSendHexHeader(4, ZSKIP, Txhdr);
		return FALSE;
	}

	if ( !CheckFileName(CHKFILENAME_SAVE, buf) )
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

		ZSetLongHeader(Txpos);
		ZSendHexHeader(4, ZCRC, Txhdr);

		if ( ZReceiveHeader(Rxhdr, 0) != ZCRC || crc != Rxpos ) {
			fclose(fp);
			fp = NULL;
			Txpos = 0;
		} else if ( fileSize != 0 && fileSize == Txpos ) {
			fclose(fp);
			ZSetLongHeader(0);
			ZSendHexHeader(4, ZSKIP, Txhdr);
			return FALSE;
		}
	}
	
	if ( fp == NULL && (fp = _tfopen(m_PathName, _T("wb"))) == NULL )
		goto ERRRET;

	UpDownOpen("ZModem File Download");
	UpDownInit(fileSize, Txpos);

	for ( ; ; ) {
		ZSetLongHeader(Txpos);
		ZSendHexHeader(4, ZRPOS, Txhdr);

	READHDR:
		if ( AbortCheck() )
			goto ERRRET;

		c = ZReceiveHeader(Rxhdr, 0);

		switch(c) {
		case ZDATA:
			UpDownStat(Txpos);
			if ( (Txpos & 0xFFFFFFFFL) != ZGetLongHeader(Rxhdr) ) {
				ZReceiveData(buf, ZBUFSIZE, Rxcount);
				break;
			}
			for ( n = 0 ; (c = ZReceiveData(buf, ZBUFSIZE, Rxcount)) >= 0 ; n++ ) {
				haveData = TRUE;
				Txpos += Rxcount;
				if ( fwrite(buf, 1, Rxcount, fp) != (UINT)Rxcount )
					goto ERRRET;
				if ( c == GOTCRCQ || c == GOTCRCW ) {
					ZSetLongHeader(Txpos);
					ZSendHexHeader(4, ZACK, Txhdr);
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
			if ( (Txpos & 0xFFFFFFFFL) != ZGetLongHeader(Rxhdr) )
				break;
			if ( fileSize != 0 && fileSize != Txpos ) {
				ZDispError("File Size Error");
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
	ZSetLongHeader(0);
	ZSendHexHeader(4, ZABORT, Txhdr);
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
	int Rxcount;
	static const char canistr[] = {
		24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
	};

	Zctlesc   = 0;
	Rxbuflen  = 1024;
	Rxcanfdx  = 1;

	Txfcs32 = 0;
	Crc32t = 1;
	Crc32r = 1;

	Rxpos = 0;
	Txpos = 0;

	Usevhdrs = 0;
	memset(Attn, 0, sizeof(Attn));

	Znulls = 0;
	Not8bit = 0;

	switch(mode) {
	case 1:		// UpLoad
		ZSetLongHeader(0L);
		ZSendHexHeader(4, ZRQINIT, Txhdr);
		break;
	case 2:		// DownLoad
		ZSetLongHeader(ZBUFSIZE);
		Txhdr[ZF0] = (char)(CANFDX|CANOVIO|CANRLE);
		if ( Txfcs32 )
			Txhdr[ZF0] |= (char)CANFC32;
		if ( Zctlesc )
			Txhdr[ZF0] |= (char)TESCCTL;
		if ( Usevhdrs )
			Txhdr[ZF1] |= (char)CANVHDR;
		ZSendHexHeader(4, ZRINIT, Txhdr);
		break;
	}

	for ( ; ; ) {
		if ( AbortCheck() )
			goto CANRET;

		c = ZReceiveHeader(Rxhdr, (mode == 0 ? 0x82 : 2));
		mode = (-1);

		switch(c) {
		case ZRQINIT:
			Usevhdrs = (Rxhdr[ZF3] & 0x80) ? 1 : 0;

		SENDRINIT:
			ZSetLongHeader(ZBUFSIZE);
			Txhdr[ZF0] = (char)(CANFDX|CANOVIO|CANRLE);
			if ( Txfcs32 )
				Txhdr[ZF0] |= (char)CANFC32;
			if ( Zctlesc )
				Txhdr[ZF0] |= (char)TESCCTL;
			if ( Usevhdrs )
				Txhdr[ZF1] |= (char)CANVHDR;
			ZSendHexHeader(4, ZRINIT, Txhdr);
			break;

		case ZRINIT:
			Rxcanfdx = (Rxhdr[ZF0] & CANFDX)  ? 1 : 0;
			Txfcs32  = (Rxhdr[ZF0] & CANRLE)  ? 2 : ((Rxhdr[ZF0] & CANFC32) ? 1 : 0);
			Zctlesc  = (Rxhdr[ZF0] & TESCCTL) ? 1 : 0;
			Usevhdrs = (Rxhdr[ZF1] & CANVHDR) ? 1 : 0;

			if ( (Rxbuflen = (0377 & Rxhdr[ZP0]) + ((0377 & Rxhdr[ZP1]) << 8)) <= 0 )
				Rxbuflen = 1024;
			else if ( Rxbuflen > ZBUFSIZE )
				Rxbuflen = ZBUFSIZE;

			if ( Bufferd_ReceiveSize() > 2 )
				break;

			if ( !sendSinit && Zctlesc == 0 ) {
				sendSinit = TRUE;
				Zctlesc = 1;
				ZSetLongHeader(0L);
				Txhdr[ZF0] |= TESCCTL;
				ZSendHexHeader(4, ZSINIT, Txhdr);
				ZSendData("\003", 1, ZCRCW);
				Zctlesc = 0;
			} else if ( ZUpFile() ) {
				goto CANRET;
			} else {
				ZSetLongHeader(0);
				ZSendHexHeader(4, ZFIN, Txhdr);
				finMode = 1;
			}
			break;

		case ZSINIT:
			Zctlesc  = (Rxhdr[ZF0] & TESCCTL) ? 1 : 0;

			if ( ZReceiveData(Attn, ZATTNLEN, Rxcount) == GOTCRCW ) {
				ZSetLongHeader(1L);
				ZSendHexHeader(4, ZACK, Txhdr);
			} else
				ZSendHexHeader(4, ZNAK, Txhdr);
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
			ZSendHexHeader(4, ZACK, Rxhdr);
			break;

		case ZFREECNT:
			ZSetLongHeader(0L);
			ZSendHexHeader(4, ZACK, Txhdr);
			break;

		case ZACK:
		case ZNAK:
			ZSetLongHeader(0L);
			ZSendHexHeader(4, ZRQINIT, Txhdr);
			break;

		case ZFIN:
			if ( finMode ) {
				Bufferd_Send('O');
				Bufferd_Send('O');
				Bufferd_Flush();
			} else {
				ZSetLongHeader(0);
				ZSendBinHeader(4, ZFIN, Txhdr);
				if ( Bufferd_Receive(10) == 'O' )
					Bufferd_Receive(10);
			}
			goto ENDRET;

		case ZCAN:
			goto ENDRET;

		case ZSKIP:
		case ZABORT:
			ZSetLongHeader(0);
			ZSendHexHeader(4, ZFIN, Txhdr);
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
	Bufferd_SendBuf((char *)canistr, (int)strlen(canistr));
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

void CZModem::ZDispError(LPCSTR str, ...)
{
	CStringA tmp;
	va_list arg;
	va_start(arg, str);
	tmp.FormatV(str, arg);
	va_end(arg);

	UpDownMessage(tmp);
}
void CZModem::ZSendBinHeader(int len, int type, char *hdr)
{
	int c, n;
	unsigned short crc16 = 0;
	unsigned long crc32 = 0xFFFFFFFF;

	if ( type == ZDATA ) {
		for ( n = 0 ; n < Znulls ; n++ )
			Bufferd_Send(0);
	}

	Bufferd_Send(ZPAD);
	Bufferd_Send(ZDLE);

	Crc32t = Txfcs32;

	switch(Crc32t) {
	case 1:
	case 2:
		if ( Crc32t == 2 )
			Bufferd_Send(Usevhdrs ? ZVBINR32 : ZBINR32); 
		else
			Bufferd_Send(Usevhdrs ? ZVBIN32 : ZBIN32); 

		if ( Usevhdrs )
			ZSendEscByte(len);

		ZSendEscByte(type);
		crc32 = UPDC32(type, crc32);

		for ( n = 0 ; n < len ; n++ ) {
			c = hdr[n] & 0377;
			ZSendEscByte(c);
			crc32 = UPDC32(c, crc32);
		}

		crc32 = ~crc32;
		ZSendEscByte((int)crc32 & 0377); crc32 >>= 8;
		ZSendEscByte((int)crc32 & 0377); crc32 >>= 8;
		ZSendEscByte((int)crc32 & 0377); crc32 >>= 8;
		ZSendEscByte((int)crc32 & 0377);
		break;

	default:
		Bufferd_Send(Usevhdrs ? ZVBIN : ZBIN); 

		if ( Usevhdrs )
			ZSendEscByte(len);

		ZSendEscByte(type);
		crc16 = UPDC16(type, crc16);

		for ( n = 0 ; n < len ; n++ ) {
			c = hdr[n] & 0377;
			ZSendEscByte(c);
			crc16 = UPDC16(c, crc16);
		}

		crc16 = UPDC16(0, UPDC16(0, crc16));
		ZSendEscByte((crc16 >> 8) & 0377);
		ZSendEscByte(crc16 & 0377);
		break;
	}

	Bufferd_Flush();

	DebugMsg("SendBinHeader %d : %02x,%02x,%02x,%02x...", len, hdr[0] & 0377, hdr[1] & 0377, hdr[2] & 0377, hdr[3] & 0377);
}
void CZModem::ZSendHexHeader(int len, int type, char *hdr)
{
	int n, c;
	unsigned short crc;

	Bufferd_Send(ZPAD);
	Bufferd_Send(ZPAD);
	Bufferd_Send(ZDLE);

	if ( Usevhdrs ) {
		Bufferd_Send(ZVHEX);
		ZSendHexByte(len);
	} else
		Bufferd_Send(ZHEX);

	Crc32t = 0;

	ZSendHexByte(type);
	crc = UPDC16(type, 0);

	for ( n = 0 ; n < len ; n++ ) {
		c = hdr[n] & 0377;
		ZSendHexByte(c);
		crc = UPDC16(c, crc);
	}

	crc = UPDC16(0, UPDC16(0, crc));
	ZSendHexByte(crc >> 8);
	ZSendHexByte(crc);

	/* Make it printable on remote machine */
	Bufferd_Send(CR);
	Bufferd_Send(LF | 0200);	// 8Bit check ?

	/* Uncork the remote in case a fake XOFF has stopped data flow */
	if ( type != ZFIN && type != ZACK )
		Bufferd_Send(XON);

	Bufferd_Flush();

	DebugMsg("SendHexHeader %d : %02x,%02x,%02x,%02x...", len, hdr[0] & 0377, hdr[1] & 0377, hdr[2] & 0377, hdr[3] & 0377);
}
void CZModem::ZSendData(char *buf, int length, int frameend)
{
	int c, n;
	unsigned short crc16 = 0;
	unsigned long crc32 = 0xFFFFFFFF;

	DebugMsg("SendData %d", length);
	DebugDump((LPBYTE)buf, length < 16 ? length : 16);

	switch (Crc32t) {
	case 1:		// CRC 32 bit
		while ( length-- > 0 ) {
			c = *(buf++) & 0377;
			ZSendEscByte(c);
			crc32 = UPDC32(c, crc32);
		}

		Bufferd_Send(ZDLE);
		Bufferd_Send(frameend);
		crc32 = UPDC32(frameend, crc32);

		crc32 = ~crc32;
		ZSendEscByte((int)crc32); crc32 >>= 8;
		ZSendEscByte((int)crc32); crc32 >>= 8;
		ZSendEscByte((int)crc32); crc32 >>= 8;
		ZSendEscByte((int)crc32);
		break;

	case 2:		// CRC 32 bit width rep
		while ( length > 0 ) {
			for ( n = 1 ; n < length && n < 191 ; n++ ) {
				if ( *buf != buf[n] )
					break;
			}

			if ( n < 5 ) {
				n = 1;
				c = *buf & 0377;
				ZSendEscByte(c);
				crc32 = UPDC32(c, crc32);

				if ( c == ZRESC ) {
					ZSendEscByte(0100);
					crc32 = UPDC32(0100, crc32);
				}

			} else if ( *buf == 040 && n < 35 ) {
				ZSendEscByte(ZRESC);
				crc32 = UPDC32(ZRESC, crc32);

				c = n + 035;
				ZSendEscByte(c);
				crc32 = UPDC32(c, crc32);

			} else {
				ZSendEscByte(ZRESC);
				crc32 = UPDC32(ZRESC, crc32);

				c = n + 0100;
				ZSendEscByte(c);
				crc32 = UPDC32(c, crc32);

				c = *buf & 0377;
				ZSendEscByte(c);
				crc32 = UPDC32(c, crc32);
			}

			length -= n;
			buf += n;
		}

		Bufferd_Send(ZDLE);
		Bufferd_Send(frameend);
		crc32 = UPDC32(frameend, crc32);

		crc32 = ~crc32;
		ZSendEscByte((int)crc32); crc32 >>= 8;
		ZSendEscByte((int)crc32); crc32 >>= 8;
		ZSendEscByte((int)crc32); crc32 >>= 8;
		ZSendEscByte((int)crc32);
		break;

	default:	// CRC 16 bit
		while ( length-- > 0 ) {
			c = *(buf++) & 0377;
			ZSendEscByte(c);
			crc16 = UPDC16(c, crc16);
		}

		Bufferd_Send(ZDLE);
		Bufferd_Send(frameend);
		crc16 = UPDC16(frameend, crc16);

		crc16 = UPDC16(0, UPDC16(0, crc16));
		ZSendEscByte(crc16 >> 8);
		ZSendEscByte(crc16);
		break;
	}

	if ( frameend == ZCRCW ) {
		Bufferd_Send(XON);
		Bufferd_Flush();
	}
}
int CZModem::ZReceiveData(char *buf, int length, int &Rxcount)
{
	int c, d, n;
	unsigned short crc16 = 0;
	unsigned long crc32 = 0xFFFFFFFF;
	int rep = 0;
			
	Rxcount = 0;

	while ( (c = ZReceiveEscByte()) >= 0 ) {
		if ( c > 0377 ) {
			switch(c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				if ( Crc32r == 1 || Crc32r == 2 ) {
					crc32 = UPDC32(c & 0377, crc32);
					for ( n = 0 ; n < 4 ; n++ ) {
						if ( (d = ZReceiveEscByte()) < 0 )
							break;
						crc32 = UPDC32(d & 0377, crc32);
					}
					if ( n < 4 || crc32 != 0xDEBB20E3 ) {
						ZDispError("Bad CRC 32");
						return ERR;
					}
				} else {
					crc16 = UPDC16(c & 0377, crc16);
					for ( n = 0 ; n < 2 ; n++ ) {
						if ( (d = ZReceiveEscByte()) < 0 )
							break;
						crc16 = UPDC16(d & 0377, crc16);
					}
					if ( n < 2 || crc16 != 0 ) {
						ZDispError("Bad CRC 16");
						return ERR;
					}
				}

				DebugMsg("RecvData %d/%d", Rxcount, length);
				DebugDump((LPBYTE)buf, Rxcount < 16 ? Rxcount : 16);
				return c;

			case GOTCAN:
				ZDispError("Sender Canceled");
				return ZCAN;

			default:
				ZDispError("Garbled data subpacket");
				return c;
			}

		} else if ( Rxcount > length ) {
			ZDispError("Data subpacket too long");
			return ERR;

		} else if ( Crc32r == 1 ) {		// CRC 32
			buf[Rxcount++] = c;
			crc32 = UPDC32(c, crc32);

		} else if ( Crc32r == 2 ) {		// CRC 32 with rep
			crc32 = UPDC32(c, crc32);

			if ( rep == 0 ) {
				if ( c == ZRESC )
					rep = (-1);
				else
					buf[Rxcount++] = c;

			} else if ( rep < 0 ) {
				if ( c >= 040 && c < 0100) {
					for ( n = c - 035 ; n > 0 ; n-- ) {
						if ( Rxcount > length ) {
							ZDispError("Space Rep too long");
							return ERR;
						}
						buf[Rxcount++] = 040;
					}
				} else if ( c == 0100 ) {
					buf[Rxcount++] = ZRESC;
					rep = 0;
				} else
					rep = c;

			} else {
				for ( n = rep - 0100 ; n > 0 ; n-- ) {
					if ( Rxcount > length ) {
						ZDispError("Data Rep too long");
						return ERR;
					}
					buf[Rxcount++] = c;
				}
				rep = 0;
			}

		} else {						// CRC 16
			buf[Rxcount++] = c;
			crc16 = UPDC16(c, crc16);
		}
	}

	ZDispError("Data subpacket Read Error");
	return ERR;
}

enum ZStatus {
	WANT_PAD = 0,
	WANT_ZDLE,
	CHECK_ZDLE,
	WANT_FRAMEID,
	SIZE_BIN,
	SIZE_HEX,
	GET_PACKET,
};

int CZModem::ZReceiveHeader(char *hdr, int eflag)
{
	int c, n;
	int can = 0;
	int len = 0;
	int st = WANT_PAD;
	unsigned short crc16 = 0;
	unsigned long crc32 = 0xFFFFFFFF;
	int Rxtype = ERR;
	int Rxhlen = 4;
	int Rxframeind = 0;

	if ( (eflag & 0x80) != 0 ) {
		eflag &= 0x7F;
		st = WANT_ZDLE;
	}

	for ( ; ; ) {
		switch(st) {
		case WANT_PAD:
			if ( (c = Bufferd_Receive(RXTIMEOUT)) < 0 )
				return c;

			if ( c == CAN ) {			// CAN == ZDLE !!
				if ( ++can >= 5 )
					return ZCAN;
				break;
			} else
				can = 0;

			st = CHECK_ZDLE;
			break;

		case WANT_ZDLE:
			if ( (c = ZReceive7Bit()) < 0 )
				return c;

			st = CHECK_ZDLE;
			break;

		case CHECK_ZDLE:
			if ( c == ZPAD || c == (ZPAD | 0200) || c == XON )
				st = WANT_ZDLE;
			else if ( c == ZDLE )
				st = WANT_FRAMEID;
			else {
				if ( eflag > 1 )
					SendEcho(c);
				else if ( eflag && ((c &= 0177) & 0140) != 0 )
					SendEcho(c);

				if ( ++len >= ZBUFSIZE )
					return ERR;
				st = WANT_PAD;
			}
			break;

		case WANT_FRAMEID:
			if ( (c = ZReceive7Bit()) < 0 )
				return c;

			Rxframeind = c;

			switch(Rxframeind) {
			case ZVBIN32:
			case ZVBINR32:
			case ZVBIN:
				st = SIZE_BIN;
				break;
			case ZVHEX:
				st = SIZE_HEX;
				break;
			case ZBIN32:
			case ZBINR32:
			case ZBIN:
			case ZHEX:
				st = GET_PACKET;
				break;
			case ZPAD:
				st = CHECK_ZDLE;
				break;
			case ZDLE:
				st = WANT_FRAMEID;
				break;
			default:
				st = WANT_PAD;
				break;
			}
			break;

		case SIZE_BIN:
			if ( (c = Bufferd_Receive(RXTIMEOUT)) < 0 )
				return c;
			else if ( (Rxhlen = c) > ZMAXHLEN )
				st = CHECK_ZDLE;
			else
				st = GET_PACKET;
			break;

		case SIZE_HEX:
			if ( (c = ZReceiveHexByte()) < 0 )
				return c;
			else if ( (Rxhlen = c) > ZMAXHLEN )
				st = CHECK_ZDLE;
			else
				st = GET_PACKET;
			break;

		case GET_PACKET:
			switch(Rxframeind) {
			case ZVBIN32:
			case ZBIN32:
			case ZVBINR32:
			case ZBINR32:
				Crc32r = (Rxframeind == ZVBIN32 || Rxframeind == ZBIN32 ? 1 : 2);

				if ( (c = ZReceiveEscByte()) < 0 || (c & ~0377) != 0 )
					return c;

				Rxtype = c;
				crc32 = UPDC32(c, crc32);

				for ( n = 0 ; n < Rxhlen ; n++ ) {
					if ( (c = ZReceiveEscByte()) < 0 || (c & ~0377) != 0 )
						return c;
					hdr[n] = (char)c;
					crc32 = UPDC32(c, crc32);
				}

				for ( n = 0 ; n < 4 ; n++ ) {
					if ( (c = ZReceiveEscByte()) < 0 || (c & ~0377) != 0 )
						return c;
					crc32 = UPDC32(c, crc32);
				}

				if ( crc32 != 0xDEBB20E3 ) {
					ZDispError("Bad CRC");
					return ERR;
				}

				break;

			case ZVBIN:
			case ZBIN:
				Crc32r = 0;

				if ( (c = ZReceiveEscByte()) < 0 || (c & ~0377) != 0 )
					return c;

				Rxtype = c;
				crc16 = UPDC16(c, crc16);

				for ( n = 0 ; n < Rxhlen ; n++ ) {
					if ( (c = ZReceiveEscByte()) < 0 || (c & ~0377) != 0 )
						return c;
					hdr[n] = (char)c;
					crc16 = UPDC16(c, crc16);
				}

				for ( n = 0 ; n < 2 ; n++ ) {
					if ( (c = ZReceiveEscByte()) < 0 || (c & ~0377) != 0 )
						return c;
					crc16 = UPDC16(c, crc16);
				}

				if ( crc16 != 0 ) {
					ZDispError("Bad CRC");
					return ERR;
				}

				break;

			case ZVHEX:
			case ZHEX:
				Crc32r = 0;

				if ( (c = ZReceiveHexByte()) < 0 )
					return c;

				Rxtype = c;
				crc16 = UPDC16(c, crc16);

				for ( n = 0 ; n < Rxhlen ; n++ ) {
					if ( (c = ZReceiveHexByte() ) < 0 )
						return c;
					hdr[n] = (char)c;
					crc16 = UPDC16(c, crc16);
				}

				for ( n = 0 ; n < 2 ; n++ ) {
					if ( (c = ZReceiveHexByte()) < 0 )
						return c;
					crc16 = UPDC16(c, crc16);
				}

				if ( crc16 != 0 ) {
					ZDispError("Bad CRC");
					return ERR;
				}

				for ( n = 0 ; n < 2 && (c = Bufferd_Receive(1)) >= 0 ; n++ ) {
					if ( c == CR )
						continue;
					else if ( c == LF )
						Not8bit |= c;
					else if ( c == (LF | 0200) )
						Not8bit = c;
					break;
				}

				break;
			}

			Rxpos = hdr[ZP3] & 0377;
			Rxpos = (Rxpos << 8) + (hdr[ZP2] & 0377);
			Rxpos = (Rxpos << 8) + (hdr[ZP1] & 0377);
			Rxpos = (Rxpos << 8) + (hdr[ZP0] & 0377);

			if ( Rxtype >= 0 && Rxtype <= FRTYPES && (Rxframeind & 040) != 0 )
				Usevhdrs = 1;

			DebugMsg("RecvHeader %03o(%c) %d : %02x,%02x,%02x,%02x...",
				Rxtype, (Rxtype >= ' ' && Rxtype < 0x7F ? Rxtype : '?'), 
				Rxhlen, hdr[0] & 0377, hdr[1] & 0377, hdr[2] & 0377, hdr[3] & 0377);

			return Rxtype;
		}
	}
}
void CZModem::ZSendHexByte(int c)
{
	static const char	digits[]	= "0123456789abcdef";

	Bufferd_Send(digits[(c >> 4) & 0x0F]);
	Bufferd_Send(digits[c & 0x0F]);
}
void CZModem::ZSendEscByte(int c)
{
	if ( (c & 0140) != 0 ) {
		// 0x20-0x7F,0xA0-0xFF
		Bufferd_Send(c);
		return;
	}
		
	// 0x00-0x1F,0x80-0x9F

	if ( Zctlesc ) {
		Bufferd_Send(ZDLE);
		c ^= 0100;

	} else {
		switch(c & 0177) {
		case LF:	// 0x0A LF
		case CR:	// 0x0D CR
		case DLE:	// 0x10 DLE
		case XON:	// 0x11 XON
		case XOFF:	// 0x13 XOff
		case CAN:	// 0x18 CAN
		case GS:	// 0x1D GS
			Bufferd_Send(ZDLE);
			c ^= 0100;
			break;
		}
	}

	Bufferd_Send(c);
}
int CZModem::ZReceiveHexByte()
{
	int c, h, l;

	if ( (c = ZReceive7Bit()) < 0 )
		return c;
	else if ( c >= '0' && c <= '9' )
		h = c - '0';
	else if ( c >= 'a' && c <= 'f' )
		h = c - 'a' + 10;
	else if ( c >= 'A' && c <= 'F' )
		h = c - 'A' + 10;
	else
		return ERR;

	if ( (c = ZReceive7Bit()) < 0 )
		return c;
	else if ( c >= '0' && c <= '9' )
		l = c - '0';
	else if ( c >= 'a' && c <= 'f' )
		l = c - 'a' + 10;
	else if ( c >= 'A' && c <= 'F' )
		l = c - 'A' + 10;
	else
		return ERR;

	return ((h << 4) | l);
}
int CZModem::ZReceiveEscByte()
{
	int c;
	int cancel = 0;

	while ( (c = Bufferd_Receive(RXTIMEOUT)) >= 0 ) {
		if ( (c & 0140) != 0 )		// 0x20-0x7F
			return c;

		if ( c == ZDLE )
			break;
		else if ( c == XON || c == (XON | 0200) || c == XOFF || c == (XOFF | 0200) )
			continue;

		if ( Zctlesc )
			continue;

		return c;
	}

	// ZDLE ...
	while ( (c = Bufferd_Receive(RXTIMEOUT)) >= 0 ) {
		if ( (c & 0140) == 0100 )	// 0x40-0x5F
			return (c ^ 0100);		// 0x00-0x1F

		if ( c == CAN ) {	// CAN == ZDLE !!
			if ( ++cancel >= 4 )
				return GOTCAN;
		} else
			cancel = 0;

		if ( c == ZCRCE || c == ZCRCG || c == ZCRCQ || c == ZCRCW )
			return (c | GOTOR);
		else if ( c == ZRUB0 )
			return 0177;
		else if ( c == ZRUB1 )
			return 0377;
		else if ( c == 023 || c == 0223 || c == 021 || c == 0221 )	// XON or XOFF
			continue;

		if ( Zctlesc )
			continue;

		return ERR;
	}

	return c;
}
int CZModem::ZReceive7Bit()
{
	int c;

	while ( (c = Bufferd_Receive(RXTIMEOUT)) >= 0 ) {
		c &= 0177;					// 0xxx xxxx 7Bit mask

		if ( (c & 0140) != 0 )		// 0x20-0x7F 
			break;

		if ( c == CR || c == LF || c == ZDLE )
			break;
	}

	return c;
}
void CZModem::ZSetLongHeader(LONGLONG pos)
{
	Txhdr[ZP0] = (char)(pos);
	Txhdr[ZP1] = (char)(pos >> 8);
	Txhdr[ZP2] = (char)(pos >> 16);
	Txhdr[ZP3] = (char)(pos >> 24);
}
LONGLONG CZModem::ZGetLongHeader(register char *hdr)
{
	LONGLONG l;

	l = (hdr[ZP3] & 0377);
	l = (l << 8) | (hdr[ZP2] & 0377);
	l = (l << 8) | (hdr[ZP1] & 0377);
	l = (l << 8) | (hdr[ZP0] & 0377);
	return l;
}
