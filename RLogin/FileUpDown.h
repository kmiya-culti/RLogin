#pragma once

#include "syncsock.h"
#include "Ish.h"

#define	DOWNMODE_NONE		0
#define	DOWNMODE_ICONV		1
#define	DOWNMODE_DECODE		2

#define	UPMODE_NONE			0
#define	UPMODE_ICONV		1
#define	UPMODE_ENCODE		2

#define	SENDMODE_NONE		0
#define	SENDMODE_BLOCK		1
#define	SENDMODE_LINE		2

#define	EDCODEMODE_AUTO		(-1)
#define	EDCODEMODE_UUENC	0
#define	EDCODEMODE_BASE64	1
#define	EDCODEMODE_ISH		2
#define	EDCODEMODE_IHEX		3
#define	EDCODEMODE_SREC		4
#define	EDCODEMODE_THEX		5

#define	CRLFMODE_CR			0
#define	CRLFMODE_LF			1
#define	CRLFMODE_CRLF		2

class CFileUpDown :	public CSyncSock
{
public:
	int m_DownMode;
	CString m_DownFrom;
	CString m_DownTo;
	int m_DecMode;
	BOOL m_bDownWait;
	int m_DownSec;
	BOOL m_bDownCrLf;
	int m_DownCrLfMode;
	BOOL m_bWithEcho;
	BOOL m_bFileAppend;

	int m_UpMode;
	CString m_UpFrom;
	CString m_UpTo;
	int m_EncMode;
	BOOL m_bUpCrLf;
	int m_UpCrLfMode;

	int m_SendMode;
	int m_BlockSize;
	int m_BlockMsec;
	int m_CharUsec;
	int m_LineMsec;
	BOOL m_bRecvWait;
	int m_RecvMsec;
	BOOL m_bCrLfWait;
	int m_CrLfMsec;
	BOOL m_bXonXoff;
	BOOL m_bRecvEcho;

	void Serialize(int mode);
	void OnProc(int cmd);
	void CheckShortName();

	typedef struct _GETPROCLIST {
		int (CFileUpDown::*GetProc)(struct _GETPROCLIST *pProc);
		void (CFileUpDown::*UnGetProc)(int ch);
		struct _GETPROCLIST *pNext;
	} GETPROCLIST;

	FILE *m_FileHandle;
	int m_FileLen;
	LONGLONG m_FileSize, m_TranSize;
	LONGLONG m_TransOffs, m_TranSeek;
	BOOL m_bSizeLimit;
	CBuffer m_FileBuffer;
	CIsh m_Ish;
	int m_UuStat;
	CStringBinary m_ishVolPath;
	int m_AutoMode;
	LONGLONG m_CharSize;
	BOOL m_bRewSize;

	BOOL UuDecode(LPCSTR line);
	BOOL Base64Decode(LPCSTR line);
	BOOL IshDecode(LPCSTR line);
	BOOL IntelHexDecode(LPCSTR line);
	BOOL SRecordDecode(LPCSTR line);
	BOOL TekHexDecode(CBuffer &line);
	int DecodeCheck(CBuffer &line);

	void DownLoad();

	int GetFile(GETPROCLIST *pProc);
	int GetIshFile(GETPROCLIST *pProc);
	void UnGetFile(int ch);

	BOOL m_IConvEof;
	CBuffer m_IConvBuffer;
	int GetIConv(GETPROCLIST *pProc);
	void UnGetIConv(int ch);

	BOOL m_EncStart;
	BOOL m_EncodeEof;
	CBuffer m_EncodeBuffer;
	LONGLONG m_EncodeSize;
	WORD m_HighWordSize;
	int GetEncode(GETPROCLIST *pProc);
	int GetBase64(GETPROCLIST *pProc);
	int GetIntelHexEncode(GETPROCLIST *pProc);
	int GetSRecordEncode(GETPROCLIST *pProc);
	int GetTekHexEncode(GETPROCLIST *pProc);
	void UnGetEncode(int ch);

	BOOL m_CrLfEof;
	CBuffer m_CrLfBuffer;
	int GetCrLf(GETPROCLIST *pProc);
	void UnGetCrLf(int ch);

	BOOL m_LineEof;
	BOOL ReadLine(GETPROCLIST *pProc, CBuffer &out);

	BOOL m_SizeEof;
	BOOL ReadSize(GETPROCLIST *pProc, CBuffer &out, int size);

	void UpLoad();

	CFileUpDown(class CRLoginDoc *pDoc, CWnd *pWnd);
	virtual ~CFileUpDown();
};

