// BPlus.cpp: CBPlus クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "BPlus.h"

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

#define Def_WS          1           /* I can send 2 Packets ahead       */
#define Def_WR          1           /* I can receive single send-ahead  */
#define Def_CM          1           /* I can handle CRC                 */
#define Def_DQ          1           /* I can handle non-quoted NUL      */
                                    /* (including the `Tf' Packet       */
#define Def_UR          0           /* I can NOT handle Upload Recovery */
#define	Def_DR			2			/* I can Download Resume user's preference */
#define Def_FI          1           /* I can handle File Information */

#define max_Errors     	10

/* Receive States */

#define   R_Get_DLE       0
#define   R_Get_B         1
#define   R_Get_Seq       2
#define   R_Get_Data      3
#define   R_Get_Check     4
#define   R_Send_ACK      5
#define   R_Timed_Out     6
#define   R_Success       7

/* Send States */

#define   S_Get_DLE       1
#define   S_Get_Num       2
#define   S_Have_ACK      3
#define   S_Get_Packet    4
#define   S_Skip_Packet   5
#define   S_Timed_Out     6
#define   S_Send_NAK      7
#define   S_Send_ENQ      8
#define   S_Send_Data     9

/* Other Constants */

#define   dle   16
#define   etx   03
#define   nak   21
#define   enq   05

#define	READ_OP		"rb"
#define	WRITE_OP	"wb"
#define	UPDATE_OP	"rb+"

#define	ERR		(-1)

static const char BP_Special_Quote_Set[]={
			(char)0x14, (char)0x24, (char)0xd4, (char)0x14,		// 00-1F	03,05,0A,0D,10,11,13,15,1B,1D
			(char)0x00, (char)0x00, (char)0x00, (char)0x00 };	// 80-9F
static const char DQ_Full[]={
			(char)0xff, (char)0xff, (char)0xff, (char)0xff,
	   		(char)0xff, (char)0xff, (char)0xff, (char)0xff };
static const char DQ_Minimal[]={
        	(char)0x14, (char)0x00, (char)0xd4, (char)0x00,
        	(char)0x00, (char)0x00, (char)0x00, (char)0x00 };

const unsigned short crc16tab[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CBPlus::CBPlus(class CRLoginDoc *pDoc, CWnd *pWnd) : CSyncSock(pDoc, pWnd)
{
	BP_Special_Quoting = 1;
	F_FileType = FALSE;
}

CBPlus::~CBPlus()
{
}

void CBPlus::OnProc(int cmd)
{
	switch(cmd) {
	case 0x05: BP_Term_ENQ(); break;
	case 0x10: BP_DLE_Seen(); break;
	}
}

//////////////////////////////////////////////////////////////////////

void CBPlus::Clear_Quote_Table()
{
    memset(Quote_Table,0,256);
}
void CBPlus::Update_Quote_Table (char *Quote_Set)
{
    int	   i, j, k;
    char   b, c;

    c = 0x40;
    k = 0x00;

    for ( i = 0 ; i < 8 ; i++ ) {
		if ( i == 4 ) {
		    c = 0x60;
			k = 0x80;
		}

		b = Quote_Set[i];

		for ( j = 0 ; j < 8 ; j++ ) {
			if ( (b & 0x80) != 0)
				Quote_Table[k] = c;

			b <<= 1;
			c++;
			k++;
		}
    }
}
void CBPlus::BP_Term_ENQ()
{
    int     i;

    if ( !Read_Byte() || e_ch != enq )
		return;

    Seq_Num = 0;
    Mx_Bf_Sz = Buffer_Size = 512;               /* Set up defaults */
    Our_WS = 0;
    Our_WR = 0;
    Our_BS = 4;
    Our_CM = 0;
    Our_DR = 0;
    Our_UR = 0;
    Our_FI = 0;

    B_Plus      = FALSE;             /* Not B Plus Protocol */
    Use_CRC     = FALSE;             /* Not CRC_16      */
    SA_Max      = 1;                 /* Single Packet send */
    SA_Error_Count = 0;              /* No Upload errors yet */
	F_FileType  = FALSE;

    for( i = 0 ; i < sizeof(DQ_Minimal) ; i++ )
		Our_QS[i] = DQ_Minimal[i];

    Clear_Quote_Table();
    Update_Quote_Table(Our_QS);

    Bufferd_Send(dle);
    Bufferd_Send('+');
    Bufferd_Send('+');
    Bufferd_Send(dle);
    Bufferd_Send('0');
    Bufferd_Flush();
}

int CBPlus::Init_CRC(int val)
{
    return (crc_16 = (UNSIG)val);
}
int	CBPlus::Upd_CRC(int value)
{
    value &= 0xFF;
    crc_16 = crc16tab[((crc_16 >> 8) ^ ((UNSIG)value)) & 0xff] ^ (crc_16 << 8);
    return( crc_16 );
}
void CBPlus::Do_CheckSum(BYTE c)
{
    c &= 0xFF;
    if ( B_Plus && Use_CRC )
    	CheckSum = Upd_CRC (c);
    else {
       	CheckSum <<= 1;
		if (CheckSum > 255)
   		    CheckSum = (CheckSum & 0xff) + 1;
		CheckSum += c;
		if (CheckSum > 255)
    	    CheckSum = (CheckSum & 0xff) + 1;
    }
}

void CBPlus::Send_Masked_Byte(BYTE c)
{
    if (Quote_Table [c] != 0) {
    	Bufferd_Send(dle);
		Bufferd_Send(Quote_Table[c]);
    } else
		Bufferd_Send(c);
}
void CBPlus::Send_ACK()
{
    Bufferd_Send(dle);
    Bufferd_Send(Seq_Num + '0');
    Bufferd_Flush();
}
void CBPlus::Send_NAK()
{
    Bufferd_Send(nak);
    Bufferd_Flush();
}
void CBPlus::Send_ENQ()
{
    Bufferd_Send(enq);
    Bufferd_Flush();
}
int CBPlus::Read_Byte()
{
    if ( (e_ch = Bufferd_Receive(Aborting ? 5 : 10)) < 0 ) {
		Timed_Out = TRUE;
		return( FALSE );
    } else {
		Timed_Out = FALSE;
		return(TRUE);
    }
}
int CBPlus::Read_Masked_Byte()
{
    masked = FALSE;

    if ( Read_Byte() == FALSE )
    	return(FALSE);

    if ( e_ch == dle ) {
		if (Read_Byte() == FALSE)
    	    return(FALSE);
		if ( e_ch < 0x60 )
			e_ch &= 0x1f;
		else
			e_ch = (e_ch & 0x1F) | 0x80;
		masked = TRUE;
    }
    return(TRUE);
}
int CBPlus::Incr_Seq(int value)
{
    if ( value == 9 )
		return( 0);
    else
		return(value + 1);
}

int	CBPlus::Read_Packet(int Lead_in_Seen, int From_Send_Packet)
{
    int	    State, next_seq, block_num, errors;
    UNSIG   new_cks;
    int	    i;
    int	    NAK_Sent;

    if ( Packet_Received ) {
		Packet_Received = FALSE;
		return( TRUE);
    }

    NAK_Sent = FALSE;
    memset(R_buffer,0,Buffer_Size);
    next_seq = (Seq_Num +  1) % 10;
    errors = 0;
    i = 0;

    if ( Lead_in_Seen )
		State = R_Get_Seq;
    else
		State = R_Get_DLE;

    while( TRUE ) {
		switch ( State ) {
		case R_Get_DLE :
		    if ( AbortCheck() && !Aborting ) {
			    Send_Failure ("AAborted by user");
				return(FALSE);
			}
			if ( !Read_Byte() )
				State = R_Timed_Out;
			else if ((e_ch & 0x7F) == dle )
				State = R_Get_B;
			else if ((e_ch & 0x7F) == enq )
				State = R_Send_ACK;
			break;

		case R_Get_B :
		    if ( !Read_Byte() )
				State = R_Timed_Out;
			else if ((e_ch & 0x7F) == 'B')
				State = R_Get_Seq;
			else if (e_ch == enq)
				State = R_Send_ACK;
			else if (e_ch == ';')
				State = R_Get_DLE;
			else
				State = R_Get_DLE;
		    break;

		case R_Get_Seq :
		    if ( !Read_Byte() )
				State = R_Timed_Out;
			else if ( e_ch == enq)
				State = R_Send_ACK;
			else {
				if (B_Plus && Use_CRC)
					CheckSum = Init_CRC(0xffff);
				else
					CheckSum = 0;
				block_num = e_ch - '0';
				Do_CheckSum (e_ch);
				i = 0;
				State = R_Get_Data;
			}
			break;

		case R_Get_Data :
			if ( !Read_Masked_Byte() )
				State = R_Timed_Out;
			else if ( (e_ch == etx) && !masked ) {
				Do_CheckSum (etx);
				State = R_Get_Check;
			} else {
				if ( i < Max_Buf_Size ) {
					R_buffer[i] = e_ch;
					i++;
					Do_CheckSum (e_ch);
				}
			}
			break;

		case R_Get_Check :
			if ( !Read_Masked_Byte() )
				State = R_Timed_Out;
			else {
				if ( B_Plus && Use_CRC ) {
					CheckSum = Upd_CRC(e_ch);
					if ( !Read_Masked_Byte() )
						new_cks = CheckSum ^ 0xff;
					else {
						CheckSum = Upd_CRC(e_ch);
						new_cks = 0;
	    			}
				} else
					new_cks = e_ch;

				if (new_cks != CheckSum)
				    State = R_Timed_Out;
				else if (R_buffer[0] == 'F') /* Watch for Failure Packet */
					State = R_Success;	 /* which is accepted regardless */
				else if (block_num == Seq_Num) /* Watch for duplicate block */
					State = R_Send_ACK; /* Simply ACK it */
				else if (block_num != next_seq)
					State = R_Timed_Out; /* Bad seq num */
				else
					State = R_Success;
			}
			break;

		case R_Timed_Out :
			errors++;
			if ( (errors > max_Errors) || (From_Send_Packet) )
				return( FALSE );
			if ( !NAK_Sent || !B_Plus ) {
				NAK_Sent = TRUE;
				Send_NAK();
			}
			State = R_Get_DLE;
			break;

		case R_Send_ACK :
			if ( !Aborting )
				Send_ACK();
			State = R_Get_DLE;        /* wait for the next block */
    	    break;

		case R_Success :
			if ( !Aborting )
				Seq_Num = block_num;
			R_Size = i;
			return(TRUE);
		}
    }
}

void CBPlus::Send_Data(int BuffeR_Number)
{
    int	    i;
    buf_type *p;

    p = &SA_Buf[BuffeR_Number];

    if (B_Plus && Use_CRC)
		CheckSum = Init_CRC (0xffff);
    else
		CheckSum = 0;

    Bufferd_Send (dle);
    Bufferd_Send ('B');
    Bufferd_Send (p->seq + '0');
    Do_CheckSum (p->seq + '0');

    for (i = 0; i <= p->num; i++ ) {
		Send_Masked_Byte (p->buf [i]);
		Do_CheckSum (p->buf[i]);
    }

    Bufferd_Send (etx);
    Do_CheckSum (etx);

    if (B_Plus && Use_CRC)
		Send_Masked_Byte ((BYTE)(CheckSum >> 8));

    Send_Masked_Byte ((BYTE)CheckSum);

    Bufferd_Flush();
}
int	CBPlus::Incr_SA(int Old_Value)
{
    if (Old_Value == Max_SA)
		return(0);
    else
		return(Old_Value + 1);
}

#define	  Get_First_DLE     1
#define	  Get_First_Digit   2
#define	  Get_Second_DLE    3
#define	  Get_Second_Digit  4

int	CBPlus::ReSync()
{
    int	    State,Digit_1;

    Bufferd_Send (enq);     /* Send <ENQ><ENQ> */
    Bufferd_Send (enq);
    Bufferd_Flush();

    State = Get_First_DLE;

    while(1) {
		switch (State) {
		case Get_First_DLE :
		    if ( !Read_Byte() )
				return(-1);
			if ( e_ch == dle )
				State = Get_First_Digit;
			break;

		case Get_First_Digit :
			if ( !Read_Byte() )
				return(-1);
			if ( (e_ch >= '0') && (e_ch <= '9') ) {
				Digit_1 = e_ch;
				State = Get_Second_DLE;
			} else if (e_ch == 'B')
				return( e_ch );
			break;

		case Get_Second_DLE :
			if ( !Read_Byte() )
				return(-1);
			if ( e_ch == dle )
				State = Get_Second_Digit;
			break;

		case Get_Second_Digit :
			if ( !Read_Byte() )
				return(-1);
			if ( (e_ch >= '0') && (e_ch <= '9') ) {
				if (Digit_1 == e_ch  )
					return(e_ch);
				else if (e_ch == 'B')
					return( e_ch );
				else {
					Digit_1 = e_ch;
					State = Get_Second_DLE;
	 			}
			} else
				State = Get_Second_DLE;
			break;
		}
	}
}

int	CBPlus::Get_ACK()
{
    int State, errors, block_num, i;
    int	Sent_ENQ;
    int	SA_Index;

    Packet_Received = FALSE;
    errors = 0;
    Sent_ENQ = FALSE;
    State = S_Get_DLE;

    while ( 1 ) {
		switch (State) {
		case S_Get_DLE :
			if ( AbortCheck() && !Aborting ) {
				Send_Failure ("AAborted by user");
				return(FALSE);
			}
			if ( !Read_Byte() )
				State = S_Timed_Out;
			else {
				if ( e_ch == dle )
					State = S_Get_Num;
				else if (e_ch == nak )
					State = S_Send_ENQ;
				else if (e_ch == etx )
					State = S_Send_NAK;
			}
			break;

		case S_Get_Num :
			if (!Read_Byte() )
				State = S_Timed_Out;
			else if ( (e_ch >= '0') && (e_ch <= '9') )
				State = S_Have_ACK;           /* Received ACK */
			else if ( e_ch == 'B' ) {
				if (!Aborting)
					State = S_Get_Packet; /* Try to receive a Packet */
				else
					State = S_Skip_Packet;   /* Try to skip a Packet */
			} else if (e_ch == nak)
				State = S_Send_ENQ;
			else if (e_ch == ';')
				State = S_Get_DLE;
			else
				State = S_Timed_Out;
			break;

		case S_Get_Packet :
			if (Read_Packet (TRUE, TRUE) ) {
				Packet_Received = TRUE;
				if (R_buffer [0] == 'F') {
					Send_ACK();
					return(FALSE);
				}
				State = S_Get_DLE;     /* Stay here to find the ACK */
			} else
				State = S_Get_DLE;
			break;

		case S_Skip_Packet :    /* Skip an incoming Packet */
			if (!Read_Byte())
				State = S_Timed_Out;
			else if (e_ch == etx ) {
				if (!Read_Masked_Byte())
					State = S_Timed_Out;
				else if (!Use_CRC)
					State = S_Get_DLE;
				else if (!Read_Masked_Byte())
					State = S_Timed_Out;
				else
					State = S_Get_DLE;
			}
			break;

		case S_Have_ACK :
			block_num = e_ch - '0';
		    if ( SA_Buf[SA_Next_to_ACK].seq == block_num  ) {
				SA_Next_to_ACK = Incr_SA (SA_Next_to_ACK);
				SA_Waiting = SA_Waiting - 1;
				if ( SA_Error_Count > 0 )
    	    	    SA_Error_Count--;
				return(TRUE);
			} else if ( (SA_Buf[Incr_SA(SA_Next_to_ACK)].seq == block_num) && SA_Waiting == 2) {
				SA_Next_to_ACK = Incr_SA (SA_Next_to_ACK);
				SA_Next_to_ACK = Incr_SA (SA_Next_to_ACK);
				SA_Waiting = SA_Waiting - 2;
				if ( SA_Error_Count > 0 )
					SA_Error_Count--;
 				return(TRUE);
		    } else if ( SA_Buf[SA_Next_to_ACK].seq == Incr_Seq(block_num) ) {
				if ( Sent_ENQ )
					State = S_Send_Data; /* Remote missed first block */
				else
					State = S_Get_DLE;       /* Duplicate ACK */
			} else {
				if ( !Aborting )        /* While aborting, ignore any */
					State = S_Timed_Out; /* ACKs that have been sent   */
				else
					State = S_Get_DLE;	/* which are not for the failure */
			}       	                            /* Packet. */
			Sent_ENQ = FALSE;
			break;

		case S_Timed_Out :
			State = S_Send_ENQ;
			break;

		case S_Send_NAK :
			errors++;
			if ( errors > max_Errors )
				return(FALSE);
			Send_NAK();
			State = S_Get_DLE;
			break;

		case S_Send_ENQ :
			errors++;
			if ( (errors > max_Errors) || (Aborting && (errors > 3)) )
				return(FALSE);
			e_ch = ReSync();
			if ( e_ch == (-1) )
				State = S_Get_DLE;
			else if (e_ch == 'B') {
				if( !Aborting )
					State = S_Get_Packet;   /* Try to receive a Packet */
			else
				State = S_Skip_Packet;  /* Try to skip a Packet */
			} else
				State = S_Have_ACK;
			Sent_ENQ   = TRUE;
			break;

		case S_Send_Data :
			SA_Error_Count += 3;
			if ( SA_Error_Count >= 12 ) {/* Stop Upload Send Ahead if too many */
				SA_Max = 1;           /* errors have occured */
				Buffer_Size = Mx_Bf_Sz;
			}
			if ( SA_Max > 1 && Buffer_Size > 128 )
				Buffer_Size -= 128;
			SA_Index = SA_Next_to_ACK;
			for ( i = 1 ; i <= SA_Waiting ; i++ ) {
				Send_Data (SA_Index);
       			SA_Index = Incr_SA (SA_Index);
			}
			State = S_Get_DLE;
			Sent_ENQ = FALSE;
			break;
		}
	}
}

int	CBPlus::Send_Packet(int size)
{
    while ( SA_Waiting >= SA_Max ) {
		if ( !Get_ACK() )
		    return(FALSE);
    }

    Seq_Num = Incr_Seq (Seq_Num);
    SA_Buf [SA_Next_to_Fill].seq = Seq_Num;
    SA_Buf [SA_Next_to_Fill].num = size;
    Send_Data (SA_Next_to_Fill);
    SA_Next_to_Fill = Incr_SA (SA_Next_to_Fill);
    SA_Waiting = SA_Waiting + 1;
    return(TRUE);
}

int	CBPlus::SA_Flush()
{
    while ( SA_Waiting > 0 ) {
		if ( !Get_ACK() )
			return(FALSE);
    }
    return( TRUE );
}

void CBPlus::Send_Failure(char *Reason)
{
    int	    i;
    buf_type *p;

    SA_Next_to_ACK = 0;
    SA_Next_to_Fill = 0;
    SA_Waiting = 0;
    Aborting   = TRUE;          /* Inform Get_ACK we're aborting ]*/

    p = &SA_Buf [0];
    p->buf [0] = 'F';
    for ( i = 1 ; i <= (int)strlen(Reason) ; i++ )
		p->buf [i] = (Reason [i]);

    if ( Send_Packet ((int)strlen(Reason)) )
		SA_Flush();   /* Gotta wait for the Initiator to ACK it */

    if ( Reason[0] != 'A' )
		UpDownMessage(Reason + 1);

	Bufferd_Clear();
}

void CBPlus::Do_Transport_Parameters()
{
    int	    Quote_Set_Present;
    int     i;
    buf_type *p;

    if (BP_Special_Quoting) {
		for ( i = 0 ; i < 8 ; i++ )
		    Our_QS[i] = BP_Special_Quote_Set[i];
    } else {
		for ( i = 0 ; i < 8 ; i++ )
			Our_QS[i] = DQ_Minimal[i];
    }

    for ( i = R_Size + 1; i<=512; i++ )
		R_buffer [i] = 0;

    His_WS = R_buffer [1];     /* Pick out Initiator's parameters */
    His_WR = R_buffer [2];
    His_BS = R_buffer [3];
    His_CM = R_buffer [4];

    His_QS [0] = R_buffer [7];
    His_QS [1] = R_buffer [8];
    His_QS [2] = R_buffer [9];
    His_QS [3] = R_buffer [10];
    His_QS [4] = R_buffer [11];
    His_QS [5] = R_buffer [12];
    His_QS [6] = R_buffer [13];
    His_QS [7] = R_buffer [14];

    His_DR = R_buffer [15];
    His_UR = R_buffer [16];
    His_FI = R_buffer [17];

    Quote_Set_Present = (R_Size >= 14 ? TRUE:FALSE);

    p = &(SA_Buf[SA_Next_to_Fill]);
    p->buf [0] = '+';  /* Prepare to return Our own parameters */
    p->buf [1] = Def_WS;
    p->buf [2] = Def_WR;
    p->buf [3] = Def_BS;
    p->buf [4] = Def_CM;
    p->buf [5] = Def_DQ;
    p->buf [6] = 0;          /* No transport layer here */

    for ( i = 0 ; i <= 7 ; i++ )
		p->buf[i + 7] = Our_QS[i];

    p->buf [15] = Def_DR;
    p->buf [16] = Def_UR;
    p->buf [17] = Def_FI;

    Update_Quote_Table((char *)DQ_Full);   /* Send the + Packet w/ full quoting */

    if ( !Send_Packet(17) )
		return;

    if ( SA_Flush() ) {         /* Wait for host's ACK on Our Packet */
		Our_WR = His_WS < Def_WR ? His_WS:Def_WR;
		Our_WS = His_WR < Def_WS ? His_WR:Def_WS;
		Our_BS = His_BS < Def_BS ? His_BS:Def_BS;
		Our_CM = His_CM < Def_CM ? His_CM:Def_CM;
		Our_DR = His_DR < Def_DR ? His_DR:Def_DR;
		Our_UR = His_UR < Def_UR ? His_UR:Def_UR;
		Our_FI = His_FI < Def_FI ? His_FI:Def_FI;
		if (Our_BS == 0)
		    Our_BS = 4;    /* Default */

		Mx_Bf_Sz = Buffer_Size = Our_BS * 128;
		B_Plus = TRUE;

		if (Our_CM == 1)
		    Use_CRC = TRUE;

		if (Our_WS != 0)
			SA_Max = Max_SA;
    }

    Clear_Quote_Table();            /* Restore Our Quoting Set */
    Update_Quote_Table (Our_QS);

    if ( Quote_Set_Present )     /* Insert Initiator's Quote Set */
		Update_Quote_Table (His_QS);
}

void CBPlus::Check_Keep(FILE *data_File, LPCTSTR Path, LPCSTR Name)
{
    char yn;
	CStringA str;

    fclose (data_File);

    if ( (!B_Plus) || (Our_DR == 0) ) {
		str = "Do you wish to retain the partial ";
		str += Name;
		str += "?";
		yn = YesOrNo(str);
    } else
		yn = 'Y';

    if ( yn == 'N' || yn == 'n' )
		_tunlink(Path);
}

LONGLONG CBPlus::Extract_Size(char **str)
{
	int flag = 1;
	LONGLONG l = 0;
	char *p = *str;

	while ( *p == ' ' )
		p++;

	if ( *p == '-' ) {
		flag = (-1);
		p++;
	}

	while ( *p >= '0' && *p <= '9' )
		l = l * 10 + (*(p++) - '0');

	while ( *p == ' ' )
		p++;

	*str = p;
	return (l * flag);
}
time_t CBPlus::Extract_Time(time_t zone, char **str)
{
	int n, i;
	LONGLONG l;
	int year, month, mday;
	time_t t, d;
	char *p = *str;
    static const int MonthDays[2][13] = {
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    };

	l = Extract_Size(&p);
	mday  = (int)(l % 100); l /= 100;
	month = (int)(l % 100); l /= 100;
	year  = (int)l;

	t = (time_t)Extract_Size(&p);

	if ( year < 1993 || month < 1 || month > 12 || mday < 1 || mday > 31 )
		t = 0;
	else {
		i = (year % 4) == 0 && (year % 100 != 0 || year % 400 == 0 ? 1 : 0);
		d = (year - 1970) * 365 + (year - 1969) / 4;	// BUG !!!
		for ( n = 1 ; n < month ; n++ )
			d += MonthDays[i][n];
		d += (mday - 1);
		d *= 86400L;
		t += d;
		t += zone;
	}

	*str = p;
	return t;
}
void CBPlus::Process_File_Information()
{
/*
	<Lead-in>
	<Sequence>
	T I							<- R_buffer
	<data type>					データの種類を表す。A = ASCII, B = Binary. 
	<compression>				データ圧縮の有無。将来のために予約されている。0 = データ圧縮なし 
	<file length> *				送信するデータの長さ。データが圧縮される場合には、圧縮前の値を用いる。 
	<zone> *					タイムゾーン (分) 
	<creation date>	*			ファイル作成日で、yyyymmddの形式。 
	<creation time>	*			ファイル作成時、00:00:00 からの秒数 
	<modification date>	*		ファイル変更日、'0' の場合には無視される 
	<modification time>	*		ファイル変更時、'0' の場合には無視される 
	<true name length>			<true name> の長さを1バイトで表わす 
	<true name>					ファイル名。ディレクトリ・デバイス等の情報は含めない。 
	<Trailer>

	* ASCII の数字による表現。区切りは１つ以上の空白による。
*/
	R_buffer[R_Size - 1] = '\0';
	char *p = R_buffer + 4;
	R_Remaining = Extract_Size(&p);
	time_t zone = (time_t)(Extract_Size(&p) * 60);
	F_Ctime = Extract_Time(zone, &p);
	F_Mtime = Extract_Time(zone, &p);
	char trueName[258];
	strncpy(trueName, p + 1, *p & 255);
	trueName[*p & 255] = '\0';
	F_TrueName = trueName;

    UpDownInit(R_Remaining, R_File_Size);
}

int	CBPlus::Receive_File(LPCTSTR filePath, LPCSTR fileName)
{
    FILE    *Data_File;
    int	    status;
    LONGLONG File_Length=0L;      /* For download resumption */
    int	    Packet_Len;
    int	    i, n;
    char    yn;
    char    Dow_Type;
    buf_type *p;

    Dow_Type = 'D';         /* Assume normal downloading */

    Data_File = FileOpen( filePath, UPDATE_OP, F_FileType );	/* open for r/w first */

    if ( Data_File != NULL && F_FileType == FALSE ) {
		if ( Our_DR > 1 )
		    Dow_Type = 'R';  /* Remote supports `Tf', let's try */
		else if ( Our_DR > 0 ) {
			yn = YesOrNo("Do you wish to resume downloading ?");
			if ( yn == 'Y' || yn == 'y' )
				Dow_Type = 'R';
		}
    }

    switch( Dow_Type ) {
	case 'D':
		if( Data_File != NULL )
			FileClose( Data_File );    /* close the read/write file */
		Data_File = FileOpen ( filePath, WRITE_OP, F_FileType );	    /* open for write */
		if (Data_File == NULL) {
			Send_Failure ("CCannot create file");
			return(FALSE);
		}
		R_File_Size = 0L;
		Send_ACK();
		break;

    case 'R' :                     /* Resume download */
		p = &SA_Buf [SA_Next_to_Fill];
		CheckSum = Init_CRC (0xffff);
		do {
			n = (int)fread (&p->buf [0], 1, Buffer_Size, Data_File);
			for ( i = 0 ; i < n ; i++ )
				CheckSum = Upd_CRC(p->buf[i]);
		} while( n > 0 );

		p->buf [0] = 'T';
		p->buf [1] = 'r';

		Packet_Len = 2;
		File_Length = _ftelli64(Data_File);

		sprintf(&(p->buf[Packet_Len]), "%I64d %d ", File_Length, CheckSum);

		while ( p->buf[Packet_Len] != '\0' )
			Packet_Len++;

		if ( !Send_Packet(Packet_Len) ) {
			FileClose (Data_File);
			return(FALSE);
		}
		if ( !SA_Flush() ) {
			FileClose (Data_File);
			return(FALSE);
		}

		R_File_Size = File_Length;
		UpDownInit(R_File_Size, R_File_Size);
		Resume_Flag = TRUE;
		break;
    }
/*
  Process each incoming Packet until 'TC' Packet received or failure
*/
    R_Remaining = 0;
    UpDownStat(R_Remaining);

    while ( TRUE ) {
		if ( Read_Packet (FALSE, FALSE) ) {
			switch (R_buffer[0]) {
			case 'N' :
				if ( Resume_Flag )
					Resume_Flag = FALSE;

				status = (int)WriteFileFromHost( &R_buffer[1], R_Size - 1, Data_File );

				if ( (status != (R_Size - 1)) ) {
					Send_Failure ("EWrite failure");
					Check_Keep (Data_File, filePath, fileName);
					return(FALSE);
				}

				R_File_Size += status;
				UpDownStat(R_File_Size);

				Send_ACK();
				break;

			case 'T' :
				if (R_buffer[1] == 'C') {
					FileClose (Data_File);
					Send_ACK();
					if ( F_Ctime != 0 ) {
						struct _utimbuf utm;
						if ( F_Mtime != 0 ) {
							utm.actime  = F_Mtime;
							utm.modtime = F_Mtime;
						} else {
							utm.actime  = F_Ctime;
							utm.modtime = F_Ctime;
						}
						_tutime(filePath, &utm);
					}
					return(TRUE);
				} else if ( R_buffer [1] == 'I' ) {
					Send_ACK();
					Process_File_Information();
				} else if ( R_buffer [1] == 'f' ) {
					FileClose (Data_File);       /* So...replace the file */
					Data_File = FileOpen ( filePath, WRITE_OP, F_FileType );
					if ( Data_File == NULL ) {
						Send_Failure ("CCannot create file");
						return(FALSE);
					}

					R_File_Size = File_Length = 0;
					UpDownInit(R_Remaining, R_File_Size);
					UpDownStat(R_File_Size);

					Resume_Flag = FALSE;
					Send_ACK();
				} else {
			        Send_Failure ("NInvalid T Packet");
				    Check_Keep (Data_File, filePath, fileName);
					return(FALSE);
				}
				break;
			case 'F' :
				Send_ACK();
				Check_Keep (Data_File, filePath, fileName);
				return(FALSE);
		    }
		} else {
			Check_Keep (Data_File, filePath, fileName);
			return(FALSE);
		}
	}
}

int	CBPlus::Send_File(LPCTSTR filePath, LPCSTR fileName)
{
    int	n;
    FILE *data_File;
    buf_type *p;
	char *s;
	struct _stati64 st;
	struct tm *tm;

	if ( _tstati64(filePath, &st) || (st.st_mode & _S_IFMT) != _S_IFREG ) {
		Send_Failure ("MFile not found");
		return(FALSE);
	}

    if ( (data_File = FileOpen(filePath, READ_OP, F_FileType)) == NULL ) {
		Send_Failure ("MFile open error");
		return(FALSE);
    }

	F_Ctime = st.st_ctime;
	F_Mtime = st.st_mtime;
	S_Remaining = st.st_size;

    UpDownInit(S_Remaining);

	if ( Our_FI ) {
		p = &SA_Buf[SA_Next_to_Fill];
		p->buf[0] = 'T';
		p->buf[1] = 'I';
		p->buf[2] = 'B';
		p->buf[3] = 0;
		s = &(p->buf[4]);

		sprintf(s, "%I64d 0 ", S_Remaining);
		while ( *s != '\0' ) s++;

		tm = gmtime(&F_Ctime);
		sprintf(s, "%04d%02d%02d %d ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour * (60 * 60) + tm->tm_min * 60 + tm->tm_sec);
		while ( *s != '\0' ) s++;

		tm = gmtime(&F_Mtime);
		sprintf(s, "%04d%02d%02d %d ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour * (60 * 60) + tm->tm_min * 60 + tm->tm_sec);
		while ( *s != '\0' ) s++;

		*(s++) = (char)strlen(fileName);
		strcpy(s, fileName);
		while ( *s != '\0' ) s++;

		Send_Packet((int)(s - p->buf));
	}

    do {
		p = &SA_Buf [SA_Next_to_Fill];
		p->buf [0] = 'N';
		n = (int)ReadFileToHost (&p->buf[1], Buffer_Size, data_File);

		if ( n > 0 ) {
			if (Send_Packet (n) == FALSE) {
				fclose(data_File);
				return(FALSE);
			}

			S_File_Size = S_File_Size + n;
			UpDownStat(S_File_Size);
		}
    } while( n > 0 );

    if ( n < 0 ) {
		Send_Failure ("EFile read failure");
		FileClose(data_File);
		return(FALSE);
    }

    p = &SA_Buf [SA_Next_to_Fill];
    p->buf [0] = 'T';
    p->buf [1] = 'C';

    if ( Send_Packet (2) == FALSE ) {
		FileClose (data_File);
		return(FALSE);
    } else {
		FileClose (data_File);
		if (!SA_Flush())
		    return(FALSE);
		return(TRUE);
    }
}

int	CBPlus::BP_DLE_Seen()
{
    int i;
	CStringA fileName;

    if ( !Read_Byte() || e_ch != dle )
		return 0;

    if ( !Read_Byte() || e_ch != 'B' )
		return 0;

    SA_Next_to_ACK  = 0;    /* Initialize Send-ahead variables */
    SA_Next_to_Fill = 0;
    SA_Waiting      = 0;
    Aborting        = FALSE;
    Packet_Received = FALSE;

    S_File_Size = 0;
    R_File_Size = 0;
    Resume_Flag = FALSE;

	F_Ctime = F_Mtime = 0;
	F_TrueName.Empty();

    if (Read_Packet (TRUE, FALSE)) {
		switch (R_buffer[0]) {
		case 'T':                      /* File Transfer Application */

			switch (R_buffer[1]) {
			case 'D' :
				break;
		    case 'U' :
				break;
		    default:
				Send_Failure ("NUnimplemented Transfer function");
				return 0;
			}

			switch (R_buffer[2]) {
			case 'A':
				F_FileType = TRUE;
				break;
			case 'B':
				F_FileType = FALSE;
				break;
			default:
				Send_Failure ("NUnimplemented file type");
				return 0;
			}

			fileName.Empty();
			for ( i = 3 ; R_buffer[i] != '\0' && i < R_Size ; i++ )
				fileName += R_buffer[i];

			if ( CheckFileName((R_buffer[1] == 'U' ? 1 : 0), fileName) == NULL || m_PathName.IsEmpty() ) {
				Send_Failure ("CCancell Transfer file");
				return 0;
			}

			if (R_buffer[1] == 'U') {
			    UpDownOpen("BPlus File Upload");
				Send_File (m_PathName, m_FileName);
			} else {
			    UpDownOpen("BPlus File Download");
				Receive_File (m_PathName, m_FileName);
			}

		    UpDownClose();
		    return 1;

		case '+':          /* Received Transport Parameters Packet */
			Do_Transport_Parameters();
			break;

		default:	/* Unknown Packet; tell the host we don't know */
			Send_Failure ("NUnknown Packet Type");
            break;

		}
    }

    return 0;
}
