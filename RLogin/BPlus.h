// BPlus.h: CBPlus クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BPLUS_H__DE80D55A_8D83_4A28_B233_2C797C582F9C__INCLUDED_)
#define AFX_BPLUS_H__DE80D55A_8D83_4A28_B233_2C797C582F9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SyncSock.h"

#define	Def_BS			8				/* Buffer size 128 x n */
#define Max_SA         	8				/* Maximum number of waiting Packets */
#define Max_Buf_Size   	(Def_BS*128+8)	/* Largest data block we can handle */

#define	UNSIG	unsigned short

typedef struct _buf_type {
	int	seq;    		/* Packet's sequence number  */
	int	num;    		/* Number of bytes in Packet */
	char	buf[Max_Buf_Size]; 	/* Actual Packet data        */
} buf_type;

class CBPlus : public CSyncSock  
{
public:
	void OnProc(int cmd);

	int		BP_Special_Quoting;

	int		Seq_Num;
	UNSIG	CheckSum;
	UNSIG	crc_16;

	char   	His_WS,His_WR,His_BS,His_CM;
	char	His_QS[8];
	char	His_DR,His_UR,His_FI;
	char	Our_WS,Our_WR,Our_BS,Our_CM;
	char	Our_QS[8];
	char	Our_DR,Our_UR,Our_FI;
	int		B_Plus;
	int		Use_CRC;
	int		Buffer_Size;
	int		Mx_Bf_Sz;
	int		SA_Max;
	int		SA_Error_Count;
	char	Quote_Table[256];

	int     R_Size;
	int     Timed_Out,Packet_Received,masked;
	int     SA_Next_to_ACK;
	int     SA_Next_to_Fill;
	int     SA_Waiting;
	int    	Aborting;

	buf_type SA_Buf[Max_SA+1];
	char	R_buffer[Max_Buf_Size];

	int		e_ch;

	LONGLONG    S_File_Size;
	LONGLONG    R_File_Size;
	LONGLONG    S_Remaining;
	LONGLONG    R_Remaining;
	int		    Resume_Flag;

	time_t		F_Ctime;
	time_t		F_Mtime;
	CString		F_TrueName;

	void Clear_Quote_Table();
	void Update_Quote_Table (char *Quote_Set);

	int Init_CRC(int val);
	int	Upd_CRC(int value);
	void Do_CheckSum(int c);

	void Send_ACK();
	void Send_NAK();
	void Send_ENQ();
	void Send_Failure(char *Reason);
	void Send_Masked_Byte(int ch);
	int	ReSync();
	int Get_ACK();
	int Read_Byte();
	int Read_Masked_Byte();

	void Send_Data(int BuffeR_Number);
	int	Read_Packet(int Lead_in_Seen, int From_Send_Packet);
	int Send_Packet(int size);

	int Incr_Seq(int value);
	int	Incr_SA(int Old_Value);
	int	SA_Flush();
	void Do_Transport_Parameters();
	void Check_Keep(FILE *data_File, LPCSTR Name);

	LONGLONG Extract_Size(char **str);
	time_t Extract_Time(time_t zone, char **str);
	void Process_File_Information();

	int	Send_File(LPCSTR name);
	int	Receive_File(LPCSTR Name);

	void BP_Term_ENQ();
	int	BP_DLE_Seen();

	CBPlus(class CRLoginDoc *pDoc, CWnd *pWnd);
	virtual ~CBPlus();
};

#endif // !defined(AFX_BPLUS_H__DE80D55A_8D83_4A28_B233_2C797C582F9C__INCLUDED_)
