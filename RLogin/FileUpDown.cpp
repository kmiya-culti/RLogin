//////////////////////////////////////////////////////////////////////
// CFileUpDown.cpp
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "FileUpDown.h"
#include "ExtFileDialog.h"

#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>

static LPCSTR CrLfStr[] = { "\r", "\n", "\r\n" };

CFileUpDown::CFileUpDown(class CRLoginDoc *pDoc, CWnd *pWnd) : CSyncSock(pDoc, pWnd)
{
	m_DownMode     = DOWNMODE_NONE;
	m_DownFrom     = pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode];
	m_DownTo       = SYSTEMICONV;
	m_DecMode      = EDCODEMODE_UUENC;
	m_bDownWait    = FALSE;
	m_DownSec      = 30;
	m_bDownCrLf    = FALSE;
	m_DownCrLfMode = CRLFMODE_CRLF;
	m_bWithEcho    = FALSE;
	m_bFileAppend  = FALSE;

	m_UpMode       = UPMODE_NONE;
	m_UpFrom       = SYSTEMICONV;
	m_UpTo         = pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode];
	m_EncMode      = EDCODEMODE_UUENC;
	m_bUpCrLf      = FALSE;
	m_UpCrLfMode   = CRLFMODE_LF;

	m_SendMode     = pDoc->m_TextRam.IsOptEnable(TO_RLDELAY) ? SENDMODE_LINE : SENDMODE_NONE;
	m_BlockSize    = 256;
	m_BlockMsec    = 100;
	m_CharUsec     = pDoc->m_TextRam.m_DelayUSecChar;
	m_LineMsec     = pDoc->m_TextRam.m_DelayMSecLine;
	m_bRecvWait    = pDoc->m_TextRam.IsOptEnable(TO_RLDELAYRV);
	m_RecvMsec     = pDoc->m_TextRam.m_DelayMSecRecv;
	m_bCrLfWait    = pDoc->m_TextRam.IsOptEnable(TO_RLDELAYCR);
	m_CrLfMsec     = pDoc->m_TextRam.m_DelayMSecCrLf;
	m_bXonXoff     = FALSE;
	m_bRecvEcho    = TRUE;

	m_FileHandle   = NULL;
	m_FileLen      = 0;
	m_FileSize     = 0LL;
	m_TranSize     = 0LL;
	m_TransOffs    = 0LL;
	m_TranSeek     = 0LL;
	m_bSizeLimit   = FALSE;

	m_UuStat       = 0;
	m_ishVolPath.RemoveAll();
	m_AutoMode     = EDCODEMODE_AUTO;

	m_IConvEof     = FALSE;
	m_EncStart     = FALSE;
	m_EncodeEof    = FALSE;
	m_EncodeSize   = 0LL;
	m_HighWordSize = 0;

	m_CrLfEof      = FALSE;
	m_LineEof      = FALSE;
	m_SizeEof      = FALSE;

	Serialize(FALSE);
}
CFileUpDown::~CFileUpDown()
{
	Serialize(TRUE);
}
void CFileUpDown::Serialize(int mode)
{
	CStringArrayExt stra;

	if ( mode ) {	// Write
		stra.AddVal(m_DownMode);
		stra.Add(m_DownFrom);
		stra.Add(m_DownTo);
		stra.AddVal(m_DecMode);
		stra.AddVal(m_bDownWait);
		stra.AddVal(m_DownSec);
		stra.AddVal(m_bDownCrLf);
		stra.AddVal(m_DownCrLfMode);
		stra.AddVal(m_bWithEcho);

		stra.AddVal(m_UpMode);
		stra.Add(m_UpFrom);
		stra.Add(m_UpTo);
		stra.AddVal(m_EncMode);
		stra.AddVal(m_bUpCrLf);
		stra.AddVal(m_UpCrLfMode);

		stra.AddVal(m_SendMode);
		stra.AddVal(m_BlockSize);
		stra.AddVal(m_BlockMsec);
		stra.AddVal(m_CharUsec);
		stra.AddVal(m_LineMsec);
		stra.AddVal(m_bRecvWait);
		stra.AddVal(m_RecvMsec);
		stra.AddVal(m_bWithEcho);
		stra.AddVal(m_bCrLfWait);
		stra.AddVal(m_CrLfMsec);
		stra.AddVal(m_bXonXoff);
		stra.AddVal(m_bRecvEcho);

		stra.AddVal(m_bFileAppend);

		((CRLoginApp *)AfxGetApp())->WriteProfileArray(_T("FileUpDown"), stra);

	} else {		// Read
		((CRLoginApp *)AfxGetApp())->GetProfileArray(_T("FileUpDown"), stra);

		if ( stra.GetSize() < 27 )
			return;

		m_DownMode     = stra.GetVal(0);
		m_DownFrom     = stra.GetAt(1);
		m_DownTo       = stra.GetAt(2);
		m_DecMode      = stra.GetVal(3);
		m_bDownWait    = stra.GetVal(4);
		m_DownSec      = stra.GetVal(5);
		m_bDownCrLf    = stra.GetVal(6);
		m_DownCrLfMode = stra.GetVal(7);
		m_bWithEcho    = stra.GetVal(8);

		m_UpMode       = stra.GetVal(9);
		m_UpFrom       = stra.GetAt(10);
		m_UpTo         = stra.GetAt(11);
		m_EncMode      = stra.GetVal(12);
		m_bUpCrLf      = stra.GetVal(13);
		m_UpCrLfMode   = stra.GetVal(14);

		m_SendMode     = stra.GetVal(15);
		m_BlockSize    = stra.GetVal(16);
		m_BlockMsec    = stra.GetVal(17);
		m_CharUsec     = stra.GetVal(18);
		m_LineMsec     = stra.GetVal(19);
		m_bRecvWait    = stra.GetVal(20);
		m_RecvMsec     = stra.GetVal(21);
		m_bWithEcho    = stra.GetVal(22);
		m_bCrLfWait    = stra.GetVal(23);
		m_CrLfMsec     = stra.GetVal(24);
		m_bXonXoff     = stra.GetVal(25);
		m_bRecvEcho    = stra.GetVal(26);

		if ( stra.GetSize() > 27 )
			m_bFileAppend = stra.GetVal(27);
	}
}
void CFileUpDown::OnProc(int cmd)
{
	switch(cmd) {
	case 0:
		DownLoad();
		break;
	case 1:
		UpLoad();
		break;
	}
}

void CFileUpDown::CheckShortName()		// check 8.3 file name
{
	int n;
	LPCTSTR s;
	CString name, sub;
	CStringA mbs;
	TCHAR shortname[MAX_PATH];

	if ( GetShortPathName(m_PathName, shortname, MAX_PATH) > 0 ) {
		if ( (s = _tcsrchr(shortname, _T('\\'))) != NULL || (s = _tcschr(shortname, _T(':'))) != NULL )
			name = s + 1;
		else
			name = shortname;
	} else
		name = m_pDoc->LocalStr(m_FileName);

	while ( (n = name.Find(_T(' '))) >= 0 )
		name.Delete(n);

	if ( (s = _tcsrchr(name, _T('.'))) != NULL )
		sub = ++s;

	if ( (n = name.Find(_T('.'))) >= 0 )
		name.Delete(n, name.GetLength() - n);

	for ( ; ; ) {
		m_FileName = m_pDoc->RemoteStr(name);
		if ( m_FileName.GetLength() <= 8 )
			break;
		name.Delete(name.GetLength() - 1);
	}

	if ( sub.IsEmpty() )
		return;

	for ( ; ; ) {
		mbs = m_pDoc->RemoteStr(sub);
		if ( mbs.GetLength() <= 3 )
			break;
		sub.Delete(sub.GetLength() - 1);
	}

	m_FileName += '.';
	m_FileName += mbs;
}

//////////////////////////////////////////////////////////////////////

BOOL CFileUpDown::UuDecode(LPCSTR line)
{
	CStringA msg;
	CBuffer work;

	switch(m_UuStat) {
	case 0:
		if ( strncmp(line, "begin ", 6) == 0 )
			m_UuStat = 1;
		else if( strncmp(line, "begin-base64 ", 13) == 0 )
			m_UuStat = 2;
		else
			break;

		// skip begin...
		while ( *line >  ' ' ) line++;
		while ( *line == ' ' ) line++;
		// skip 644..
		while ( *line >  ' ' ) line++;
		while ( *line == ' ' ) line++;

		if ( m_FileHandle == NULL ) {
			if ( !CheckFileName(CHKFILENAME_SAVE, line) )
				return FALSE;
			if ( (m_FileHandle = _tfopen(m_PathName, _T("wb"))) == NULL ) {
				::ThreadMessageBox(_T("uudecode '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
				return FALSE;
			}
			UpDownInit(0, 0);
		}

		msg.Format("uudecode start '%s'", line);
		UpDownMessage(msg);
		break;
	case 1:
		if ( strncmp(line, "end", 3) == 0 ) {
			m_UuStat = 0;
			m_AutoMode = m_DecMode;
			if ( m_FileHandle != NULL )
				fclose(m_FileHandle);
			m_FileHandle = NULL;
			UpDownMessage("uudecode end... closed");
		} else {
			work.UuDecode(line);
			if ( work.GetSize() > 0 ) {
				if ( fwrite(work.GetPtr(), 1, work.GetSize(), m_FileHandle) != work.GetSize() ) {
					::ThreadMessageBox(_T("uudecode fwrite error '%s'\n"), m_PathName);
					return FALSE;
				}
			}
		}
		break;
	case 2:
		if ( strncmp(line, "====", 4) == 0 ) {
			m_UuStat = 0;
			m_AutoMode = m_DecMode;
			if ( m_FileHandle != NULL ) {
				fclose(m_FileHandle);
				m_FileHandle = NULL;
				UpDownMessage("uudecode end... closed");
			}
		} else {
			work.Base64Decode(line);
			if ( work.GetSize() > 0 ) {
				if ( fwrite(work.GetPtr(), 1, work.GetSize(), m_FileHandle) != work.GetSize() ) {
					::ThreadMessageBox(_T("uudecode fwrite error '%s'\n"), m_PathName);
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}
BOOL CFileUpDown::Base64Decode(LPCSTR line)
{
	CBuffer work;

	work.Base64Decode(line);

	if ( work.GetSize() > 0 ) {
		if ( m_FileHandle == NULL ) {
			if ( !CheckFileName(CHKFILENAME_SAVE, "") )
				return FALSE;
			if ( (m_FileHandle = _tfopen(m_PathName, _T("wb"))) == NULL ) {
				::ThreadMessageBox(_T("base64 '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
				return FALSE;
			}
			UpDownInit(0, 0);
		}

		if ( fwrite(work.GetPtr(), 1, work.GetSize(), m_FileHandle) != work.GetSize() ) {
			::ThreadMessageBox(_T("base64 fwrite error '%s'\n"), m_PathName);
			return FALSE;
		}
	}

	return TRUE;
}
BOOL CFileUpDown::IshDecode(LPCSTR line, BOOL &bRewSize)
{
	CStringA msg;
	CBuffer work;

	switch(m_Ish.DecodeLine(line, work)) {
	case ISH_RET_HEAD:
		if ( m_FileHandle == NULL ) {
			if ( m_Ish.m_VolSeq != 0 && m_ishVolPath.Find(m_Ish.m_VolChkName) != NULL ) {
				m_PathName = m_ishVolPath[m_Ish.m_VolChkName];
				m_FileHandle = _tfopen(m_PathName, _T("r+b"));
			} else if ( !CheckFileName(CHKFILENAME_SAVE, m_Ish.m_FileName) )
				return FALSE;
			else
				m_FileHandle = _tfopen(m_PathName, _T("wb"));

			if ( m_FileHandle == NULL ) {
				::ThreadMessageBox(_T("ish decode '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
				return FALSE;
			}
		}
		if ( m_FileHandle != NULL && m_Ish.m_VolSeq != 0 ) {
			_fseeki64(m_FileHandle, m_Ish.m_FileSeek, SEEK_SET);
			m_ishVolPath[m_Ish.m_VolChkName] = m_PathName;
		}
		UpDownInit(m_Ish.m_FileSize);
		bRewSize = FALSE;
		if ( m_Ish.m_VolSeq != 0 )
			msg.Format("ish start '%s' #%d", (LPCSTR)m_Ish.m_FileName, m_Ish.m_VolSeq);
		else
			msg.Format("ish start '%s'", (LPCSTR)m_Ish.m_FileName);
		UpDownMessage(msg);
		break;
	case ISH_RET_DATA:
		if ( m_FileHandle == NULL )
			break;
		if ( work.GetSize() > 0 ) {
			if ( fwrite(work.GetPtr(), 1, work.GetSize(), m_FileHandle) != work.GetSize() ) {
				::ThreadMessageBox(_T("ish fwrite error '%s'\n"), m_PathName);
				return FALSE;
			}
		}
		UpDownStat(m_Ish.GetSize());
		break;
	case ISH_RET_ENDOF:
		m_AutoMode = m_DecMode;
		if ( m_FileHandle == NULL )
			break;
		if ( work.GetSize() > 0 )
			fwrite(work.GetPtr(), 1, work.GetSize(), m_FileHandle);
		if ( m_Ish.m_VolSeq != 0 )
			m_ishVolPath[m_Ish.m_VolChkName][m_Ish.m_VolSeq] = 1;
		UpDownStat(m_Ish.m_FileSize);
		fclose(m_FileHandle);
		m_FileHandle = NULL;
		if ( m_Ish.m_FileTime != 0 ) {
			struct _utimbuf utm;
			utm.actime  = m_Ish.m_FileTime;
			utm.modtime = m_Ish.m_FileTime;
			_tutime(m_PathName, &(utm));
		}
		if ( m_Ish.m_VolSeq != 0 ) {
			msg.Format("ish end '%s' (%d/%d)", (LPCSTR)m_Ish.m_FileName, m_ishVolPath[m_Ish.m_VolChkName].GetSize(), m_Ish.m_VolMax);
			UpDownMessage(msg);
		} else
			UpDownMessage("ish end... closed");
		break;
	case ISH_RET_ECCERR:
		m_AutoMode = m_DecMode;
		if ( m_FileHandle == NULL )
			break;
		fclose(m_FileHandle);
		m_FileHandle = NULL;
		UpDownMessage("ish ECC error... closed");
		break;
	case ISH_RET_CRCERR:
		m_AutoMode = m_DecMode;
		if ( m_FileHandle == NULL )
			break;
		fclose(m_FileHandle);
		m_FileHandle = NULL;
		UpDownMessage("ish CRC error... closed");
		break;
	}

	return TRUE;
}
BOOL CFileUpDown::IntelHexDecode(LPCSTR line)
{
	CBuffer work;
	BYTE sum = 0;
	BYTE *s;
	int len;
	WORD addr;
	int type;
	WORD HighWord;

	if ( *line != ':' )
		return TRUE;

	work.IntelHexDecode(++line);
	if ( work.GetSize() >= 5 ) {
		s = work.GetPtr();
		for ( int n = 0 ; n < work.GetSize() ; n++ )
			sum += *(s++);
		if ( sum != 0 )
			return TRUE;

		len = work.Get8Bit();
		addr = (WORD)work.Get16Bit();
		type = work.Get8Bit();

		if ( len != (work.GetSize() - 1) )
			return TRUE;

		switch(type) {
		case 0:		// data
			if ( m_FileHandle == NULL ) {
				if ( !CheckFileName(CHKFILENAME_SAVE, "") )
					return FALSE;
				if ( (m_FileHandle = _tfopen(m_PathName, _T("wb"))) == NULL ) {
					::ThreadMessageBox(_T("intel hex '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
					return FALSE;
				}
				UpDownMessage("intel hex new file");
				UpDownInit(0, 0);
			}
			m_TranSize = m_TransOffs + (LONGLONG)addr;
			if ( m_TranSeek != m_TranSize && _fseeki64(m_FileHandle, m_TranSize, SEEK_SET) != 0 ) {
				::ThreadMessageBox(_T("intel hex fseek error '%s'\n"), m_PathName);
				return FALSE;
			}
			if ( fwrite(work.GetPtr(), 1, len, m_FileHandle) != len ) {
				::ThreadMessageBox(_T("intel hex fwrite error '%s'\n"), m_PathName);
				return FALSE;
			}
			m_TranSeek = m_TranSize + len;
			break;
		case 1:		// endof
			m_TransOffs = m_TranSeek = 0LL;
			m_AutoMode = m_DecMode;
			if ( m_FileHandle != NULL ) {
				fclose(m_FileHandle);
				m_FileHandle = NULL;
				UpDownMessage("intel hex end of data... closed");
			}
			break;
		case 2:		// segment
			HighWord = work.Get16Bit();
			m_TransOffs = (LONGLONG)HighWord << 4;
			break;
		case 4:		// high word address
			HighWord = work.Get16Bit();
			m_TransOffs = (LONGLONG)HighWord << 16;
			break;
		}
	}

	return TRUE;
}
BOOL CFileUpDown::SRecordDecode(LPCSTR line)
{
	CBuffer work;
	int len, alen;

	if ( line[0] != 'S' )
		return TRUE;

	if ( line[1] == '0' )
		alen = 2;				// len = 1, addr = 2, sum = 1	4 byte, addr == 0
	else if ( line[1] == '1' )
		alen = 2;				// len = 1, addr = 2, sum = 1	4 byte
	else if ( line[1] == '2' )
		alen = 3;				// len = 1, addr = 3, sum = 1	5 byte
	else if ( line[1] == '3' )
		alen = 4;				// len = 1, addr = 4, sum = 1	6 byte
	else if ( line[1] == '7' || line[1] == '8' || line[1] == '9'  ) {
		m_TransOffs = m_TranSeek = m_TransOffs = 0LL;
		m_AutoMode = m_DecMode;
		if ( m_FileHandle != NULL ) {
			fclose(m_FileHandle);
			m_FileHandle = NULL;
			UpDownMessage("s-record end of data... closed");
		}
		return TRUE;
	} else
		return TRUE;

	work.IntelHexDecode(line + 2);
	if ( work.GetSize() < (alen + 2) )
		return TRUE;

	BYTE sum = 0;
	BYTE *s = work.GetPtr();
	for ( int n = 0 ; n < work.GetSize() ; n++ )
		sum += *(s++);
	if ( sum != 0xFF )
		return TRUE;

	len = work.Get8Bit();
	if ( len > work.GetSize() )
		return TRUE;
	len -= (alen + 1);		// addr + sum

	m_TranSize = 0LL;
	for ( int n = 0 ; n < alen ; n++ )
		m_TranSize = (m_TranSize << 8) | (LONGLONG)work.Get8Bit();

	if ( line[1] == '0' ) {
		CStringA msg;
		work.ConsumeEnd(1);	// remove sum
		msg.Format("s-record '%s'", (LPCSTR)work);
		UpDownMessage((LPCSTR)msg);

	} else {
		if ( m_FileHandle == NULL ) {
			if ( !CheckFileName(CHKFILENAME_SAVE, "") )
				return FALSE;
			if ( (m_FileHandle = _tfopen(m_PathName, _T("wb"))) == NULL ) {
				::ThreadMessageBox(_T("s-record '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
				return FALSE;
			}
			UpDownMessage("s-record new file");
			UpDownInit(0, 0);
		}

		if ( m_TranSeek != m_TranSize && _fseeki64(m_FileHandle, m_TranSize, SEEK_SET) != 0 ) {
			::ThreadMessageBox(_T("s-record fseek error '%s'\n"), m_PathName);
			return FALSE;
		}
		if ( fwrite(work.GetPtr(), 1, len, m_FileHandle) != len ) {
			::ThreadMessageBox(_T("s-record fwrite error '%s'\n"), m_PathName);
			return FALSE;
		}
		m_TranSeek = m_TranSize + len;
	}

	return TRUE;
}
BOOL CFileUpDown::TekHexDecode(CBuffer &line)
{
	int n, i;
	BYTE sum = 0;
	int len, alen, ch;
	int type;
	CBuffer work;

	if ( *((LPCSTR)line) != '%' )
		return TRUE;

	if ( (len = line.TekHexDecode(1, 2)) < 7 )
		return TRUE;

	for ( n = 0 ; n < len ; n++ ) {
		if ( (ch = line.TekHexDecode(1 + n, 1)) == (-1) )
			break;
		if ( n != 3 && n != 4 )		// sum
			sum += (BYTE)ch;
	}
	if ( n < len || sum != line.TekHexDecode(4, 2) )
		return TRUE;

	type = line.TekHexDecode(3, 1);
	if ( type == 8 ) {
		m_TransOffs = m_TranSeek = m_TransOffs = 0LL;
		m_AutoMode = m_DecMode;
		if ( m_FileHandle != NULL ) {
			fclose(m_FileHandle);
			m_FileHandle = NULL;
			UpDownMessage("tek hex end of data... closed");
		}
		return TRUE;
	} else if ( type != 6 )
		return TRUE;

	alen = line.TekHexDecode(6, 1);
	if ( alen < 1 || alen > 8 )
		return TRUE;
	m_TranSize = (LONGLONG)(DWORD)line.TekHexDecode(7, alen);

	work.Clear();
	i = 7 + alen;
	len = len - 6 - alen;
	for ( n = 0 ; n < len ; n++ ) {
		if ( (ch = line.TekHexDecode(i++, 1)) == (-1) )
			break;
		if ( (n & 1) == 0 )
			sum = (BYTE)ch;
		else {
			sum = (sum << 4) | ch;
			work.PutByte(sum);
		}
	}
	if ( n < len )
		return TRUE;

	len = work.GetSize();

	if ( m_FileHandle == NULL ) {
		if ( !CheckFileName(CHKFILENAME_SAVE, "") )
			return FALSE;
		if ( (m_FileHandle = _tfopen(m_PathName, _T("wb"))) == NULL ) {
			::ThreadMessageBox(_T("tek hex '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
			return FALSE;
		}
		m_TranSeek = 0LL;
		UpDownMessage("tek hex new file");
		UpDownInit(0, 0);
	}

	if ( m_TranSeek != m_TranSize && _fseeki64(m_FileHandle, m_TranSize, SEEK_SET) != 0 ) {
		::ThreadMessageBox(_T("tek hex fseek error '%s'\n"), m_PathName);
		return FALSE;
	}
	if ( fwrite(work.GetPtr(), 1, len, m_FileHandle) != len ) {
		::ThreadMessageBox(_T("tek hex fwrite error '%s'\n"), m_PathName);
		return FALSE;
	}
	m_TranSeek = m_TranSize + len;

	return TRUE;
}
int CFileUpDown::DecodeCheck(CBuffer &line)
{
	LPCSTR p = line;
	CBuffer work;
	BYTE sum = 0;

	// uudecode
	if ( strncmp(p, "begin ", 6) == 0 || strncmp(p, "begin-base64 ", 13) == 0 )
		return EDCODEMODE_UUENC;

	// ish header
	if ( *p == '!' ) {
		work.IshDecJis7(p);
		if ( work.GetSize() > 60 && m_Ish.ChkCRC(work) && *(work.GetPos(0)) == 0 )
			return EDCODEMODE_ISH;
	} else if ( *p == '#' ) {
		work.IshDecJis8(p);
		if ( work.GetSize() > 60 && m_Ish.ChkCRC(work) && *(work.GetPos(0)) == 0 )
			return EDCODEMODE_ISH;
	}

	// intel hex
	if ( *p == ':' ) {
		work.IntelHexDecode(p + 1);
		if ( work.GetSize() >= 5 ) {
			BYTE *s = work.GetPtr();
			for ( int n = 0 ; n < work.GetSize() ; n++ )
				sum += *(s++);
			if ( sum == 0 ) {
				int len = work.Get8Bit();
				if ( len == (work.GetSize() - (1 + 2 + 1)) )	// type=1 + addr=2 + sum=1
					return EDCODEMODE_IHEX;
			}
		}
	}

	// s-record
	if ( p[0] == 'S' && strchr("012345789", p[1]) != NULL ) {
		work.IntelHexDecode(p + 2);
		BYTE *s = work.GetPtr();
		for ( int n = 0 ; n < work.GetSize() ; n++ )
			sum += *(s++);
		if ( sum == 0xFF ) {
			int len = work.Get8Bit();
			if ( len <= work.GetSize() )
				return EDCODEMODE_SREC;
		}
	}

	// tek hex
	if ( *p == '%' ) {
		int len = line.TekHexDecode(1, 2);
		if ( len >= 7 ) {
			int n, ch;
			for ( n = 0 ; n < len ; n++ ) {
				if ( (ch = line.TekHexDecode(1 + n, 1)) == (-1) )
					break;
				if ( n != 3 && n != 4 )		// sum
					sum += (BYTE)ch;
			}
			if ( n >= len && sum == line.TekHexDecode(4, 2) )
				return EDCODEMODE_THEX;
		}
	}

	// base64は、判別が曖昧なので注意
	//p = work.Base64Decode(p);
	//if ( (*p == '\0' || *p == '=') && work.GetSize() > 0 )
	//	return EDCODEMODE_BASE64;

	return EDCODEMODE_AUTO;
}

/*
	〇文字の変換やデコードを行わない
	〇文字コードの変換を行う From [EUC/SJIS/UTF9...] To [EUC/SJIS/UTF9...]
	〇デコード処理を行う							[uudecode/base64]
	□指定時間(sec)受信が無ければ終了する			[0123456789]
	□改行コードを変換する							[CR/LF/CR+LF]
	□受信した文字を表示する
*/

void CFileUpDown::DownLoad()
{
	int ch, len;
	int last = (-1);
	struct _stati64 st;
	BOOL bRewSize = TRUE;
	LONGLONG CharSize;
	CBuffer tmp, work, line;

	if ( !CheckFileName(CHKFILENAME_SAVE, "", EXTFILEDLG_DOWNLOAD) )
		return;

	if ( m_bFileAppend && m_DownMode != DOWNMODE_DECODE && !_tstati64(m_PathName, &st) && (st.st_mode & _S_IFMT) == _S_IFREG ) {
		if ( (m_FileHandle = _tfopen(m_PathName, _T("r+b"))) == NULL ) {
			::ThreadMessageBox(_T("DownLoad '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
			return;
		}
		_fseeki64(m_FileHandle, 0, SEEK_END);

	} else if ( (m_FileHandle = _tfopen(m_PathName, _T("wb"))) == NULL ) {
		::ThreadMessageBox(_T("DownLoad '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
		return;
	}

	UpDownOpen("Simple File Download");
	UpDownInit(0, 0);

	CharSize = 0LL;
	m_TranSize = m_TranSeek = m_TransOffs = 0LL;
	m_UuStat = 0;
	m_Ish.Init();
	m_ishVolPath.RemoveAll();
	m_AutoMode = m_DecMode;

	while ( !AbortCheck() ) {
		tmp.Clear();
		if ( (ch = Bufferd_Receive(m_bDownWait ? m_DownSec : 300)) < 0 )
			break;
		tmp.PutByte(ch);

		if ( (len = Bufferd_ReceiveSize()) > 0 ) {
			if ( len > 1024 )
				len = 1024;
			if ( !Bufferd_ReceiveBuf((char *)tmp.PutSpc(len), len, 0) )
				break;
		}

		if ( m_bWithEcho )
			SendEchoBuffer((char *)tmp.GetPtr(), tmp.GetSize());

		if ( bRewSize ) {
			CharSize += tmp.GetSize();
			UpDownStat(CharSize);
		}

		if ( m_bDownCrLf ) {
			work.Clear();
			while ( tmp.GetSize() > 0 ) {
				ch = tmp.GetByte();
				if ( ch == '\n' )
					work += CrLfStr[m_DownCrLfMode];
				else if ( ch != '\r' ) {
					if ( last == '\r' )
						work += CrLfStr[m_DownCrLfMode];
					work.PutByte(ch);
				}
				last = ch;
			}
			tmp.Swap(work);
		}

		switch(m_DownMode) {
		case DOWNMODE_NONE:
			if ( fwrite(tmp.GetPtr(), 1, tmp.GetSize(), m_FileHandle) != tmp.GetSize() ) {
				::ThreadMessageBox(_T("file fwrite error '%s'\n"), m_PathName);
				goto ENDOFRET;
			}
			break;

		case DOWNMODE_ICONV:
			work.Clear();
			m_IConv.IConvSub(m_DownFrom, m_DownTo, &tmp, &work);
			if ( fwrite(work.GetPtr(), 1, work.GetSize(), m_FileHandle) != work.GetSize() ) {
				::ThreadMessageBox(_T("iconv fwrite error '%s'\n"), m_PathName);
				goto ENDOFRET;
			}
			break;

		case DOWNMODE_DECODE:
			while ( tmp.GetSize() > 0 ) {
				ch = tmp.GetByte();
				if ( ch == '\r' || ch == '\n' || ch == '\007' ) {
					if ( line.GetSize() <= 0 )
						continue;

					if ( m_AutoMode == EDCODEMODE_AUTO )
						m_AutoMode = DecodeCheck(line);

					switch(m_AutoMode) {
					case EDCODEMODE_AUTO:
						break;
					case EDCODEMODE_UUENC:
						if ( !UuDecode(line) )
							goto ENDOFRET;
						break;
					case EDCODEMODE_BASE64:
						if ( !Base64Decode(line) )
							goto ENDOFRET;
						break;
					case EDCODEMODE_ISH:
						if ( !IshDecode(line, bRewSize) )
							goto ENDOFRET;
						break;
					case EDCODEMODE_IHEX:
						if ( !IntelHexDecode(line) )
							goto ENDOFRET;
						break;
					case EDCODEMODE_SREC:
						if ( !SRecordDecode(line) )
							goto ENDOFRET;
						break;
					case EDCODEMODE_THEX:
						if ( !TekHexDecode(line) )
							goto ENDOFRET;
						break;
					}

					line.Clear();
				} else
					line.PutByte(ch);
			}
			break;
		}
	}

ENDOFRET:
	if ( m_FileHandle != NULL )
		fclose(m_FileHandle);
	UpDownClose();
}

//////////////////////////////////////////////////////////////////////

BOOL CFileUpDown::GetFile(GETPROCLIST *pProc)
{
	struct _stati64 st;
	int len;
	
	if ( AbortCheck() ) {
		UpDownClose();
		if ( m_FileHandle != NULL ) {
			fclose(m_FileHandle);
			m_FileHandle = NULL;
		}
		return EOF;
	}

	if ( m_FileBuffer.GetSize() > 0 )
		return m_FileBuffer.GetByte();

	if ( m_FileHandle == NULL ) {
		if ( m_PathName.IsEmpty() )
			return EOF;

		if ( _tstati64(m_PathName, &st) || (st.st_mode & _S_IFMT) != _S_IFREG ) {
			::ThreadMessageBox(_T("Upload '%s'\n%s"), (LPCTSTR)m_PathName, CStringLoad(IDE_FILEOPENERROR));
			return EOF;
		}

		if ( (m_FileHandle = _tfopen(m_PathName, _T("rb"))) == NULL ) {
			::ThreadMessageBox(_T("Upload '%s'\n%s"), (LPCTSTR)m_PathName, CStringLoad(IDE_FILEOPENERROR));
			return EOF;
		}

		m_FileSize = st.st_size;
		m_TranSize = 0;
		m_FileBuffer.Clear();

		if ( (m_FileLen = (int)(m_FileSize / 100)) < 32 )
			m_FileLen = 32;
		else if ( m_FileLen > 4096 )
			m_FileLen = 4096;

		if ( m_bSizeLimit && m_FileSize > 0xFFFFFFFFLL ) {
			::ThreadMessageBox(_T("Upload '%s'\n%s"), (LPCTSTR)m_PathName, _T("File Size 4GByte Over"));
			return EOF;
		}

		UpDownOpen("Simple File Upload");
		UpDownInit(m_FileSize);
	}

	if ( (len = (int)fread(m_FileBuffer.PutSpc(m_FileLen), 1, m_FileLen, m_FileHandle)) <= 0 ) {
		UpDownClose();
		fclose(m_FileHandle);
		m_FileHandle = NULL;
		return EOF;
	}

	m_FileBuffer.ConsumeEnd(m_FileLen - len);

	m_TranSize += len;
	UpDownStat(m_TranSize);

	return m_FileBuffer.GetByte();
}
BOOL CFileUpDown::GetIshFile(GETPROCLIST *pProc)
{
	struct _stati64 st;
	int len;
	
	if ( AbortCheck() ) {
		UpDownClose();
		if ( m_FileHandle != NULL ) {
			fclose(m_FileHandle);
			m_FileHandle = NULL;
		}
		return EOF;
	}

	if ( m_FileBuffer.GetSize() > 0 )
		return m_FileBuffer.GetByte();

	if ( m_FileHandle == NULL ) {
		if ( m_PathName.IsEmpty() )
			return EOF;

		if ( _tstati64(m_PathName, &st) || (st.st_mode & _S_IFMT) != _S_IFREG ) {
			::ThreadMessageBox(_T("ish Upload '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
			return EOF;
		}

		if ( (m_FileHandle = _tfopen(m_PathName, _T("rb"))) == NULL ) {
			::ThreadMessageBox(_T("ish Upload '%s'\n%s"), m_PathName, CStringLoad(IDE_FILEOPENERROR));
			return EOF;
		}

		m_FileSize = st.st_size;
		m_TranSize = 0;
		m_FileBuffer.Clear();

		CheckShortName();

		if ( !m_Ish.EncodeHead(ISH_LEN_JIS7, m_FileHandle, m_FileName, st.st_size, st.st_mtime, m_FileBuffer) ) {
			::AfxMessageBox(_T("ish file size limit error"), MB_ICONERROR);
			return EOF;
		}

		UpDownOpen("Ish File Upload");
		UpDownInit(m_FileSize);

	} else {
		if ( (len = m_Ish.EncodeBlock(m_FileHandle, m_FileBuffer)) < 0 ) {
			UpDownClose();
			fclose(m_FileHandle);
			m_FileHandle = NULL;
			return EOF;
		}

		m_TranSize += len;
		UpDownStat(m_TranSize);
	}

	return m_FileBuffer.GetByte();
}
void CFileUpDown::UnGetFile(int ch)
{
	BYTE tmp[4];

	if ( ch == EOF )
		return;

	tmp[0] = (BYTE)ch;
	m_FileBuffer.Insert(tmp, 1);
}
int CFileUpDown::GetIConv(GETPROCLIST *pProc)
{
	int ch = EOF;
	CBuffer tmp;

	for ( ; ; ) {
		if ( m_IConvBuffer.GetSize() > 0 )
			return m_IConvBuffer.GetByte();

		if ( m_IConvEof ) {
			m_IConvEof = FALSE;
			return EOF;
		}

		while ( tmp.GetSize() < 1024 && (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ) {
			tmp.PutByte(ch);
			if ( ch == '\n' ) {
				if ( tmp.GetSize() > m_FileLen )
					break;
			} else if ( ch == '\r' ) {
				ch = (this->*pProc->GetProc)(pProc->pNext);
				if ( ch == '\n' )
					tmp.PutByte(ch);
				else
					(this->*pProc->UnGetProc)(ch);
				if ( tmp.GetSize() > m_FileLen )
					break;
			}
		}

		if ( ch == EOF )
			m_IConvEof = TRUE;

		if ( tmp.GetSize() > 0 ) {
			m_IConv.IConvSub(m_UpFrom, m_UpTo, &tmp, &m_IConvBuffer);
			tmp.Clear();
		}
	}
}
void CFileUpDown::UnGetIConv(int ch)
{
	BYTE tmp[4];

	if ( ch == EOF )
		return;

	tmp[0] = (BYTE)ch;
	m_IConvBuffer.Insert(tmp, 1);
}
int CFileUpDown::GetEncode(GETPROCLIST *pProc)
{
	int len, ch;
	BYTE tmp[128];

	for ( ; ; ) {
		if ( m_EncodeBuffer.GetSize() > 0 )
			return m_EncodeBuffer.GetByte();

		if ( m_EncodeEof ) {
			m_EncStart = FALSE;
			m_EncodeEof = FALSE;
			return EOF;
		}

		for ( len = 0 ; len < 45 && (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ; len++ )
			tmp[len] = (BYTE)ch;

		if ( ch == EOF )
			m_EncodeEof = TRUE;

		if ( !m_EncStart ) {
			m_EncStart = TRUE;
			m_EncodeBuffer.Format("begin 644 %s\n", (LPCSTR)m_FileName);
		}

		if ( len > 0 ) {
			m_EncodeBuffer.UuEncode(tmp, len);
			m_EncodeBuffer.PutByte('\n');
		}

		if ( m_EncodeEof )
			m_EncodeBuffer += "`\nend\n";
	}
}
void CFileUpDown::UnGetEncode(int ch)
{
	BYTE tmp[4];

	if ( ch == EOF )
		return;

	tmp[0] = (BYTE)ch;
	m_EncodeBuffer.Insert(tmp, 1);
}
int CFileUpDown::GetBase64(GETPROCLIST *pProc)
{
	int len, ch;
	BYTE tmp[128];
	CBuffer work;

	for ( ; ; ) {
		if ( m_EncodeBuffer.GetSize() > 0 )
			return m_EncodeBuffer.GetByte();

		if ( m_EncodeEof ) {
			m_EncodeEof = FALSE;
			return EOF;
		}

		for ( len = 0 ; len < 54 && (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ; len++ )
			tmp[len] = (BYTE)ch;

		if ( ch == EOF )
			m_EncodeEof = TRUE;

		if ( len > 0 ) {
			work.Base64Encode(tmp, len);
			m_EncodeBuffer += TstrToMbs((LPCTSTR)work);
			m_EncodeBuffer.PutByte('\n');
		}
	}
}
int CFileUpDown::GetIntelHexEncode(GETPROCLIST *pProc)
{
	int len, ch;
	BYTE sum;
	BYTE head[8];
	BYTE tmp[34];

	for ( ; ; ) {
		if ( m_EncodeBuffer.GetSize() > 0 )
			return m_EncodeBuffer.GetByte();

		if ( m_EncodeEof ) {
			m_EncStart = FALSE;
			m_EncodeEof = FALSE;
			return EOF;
		}

		for ( len = 0 ; len < 32 && (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ; len++ )
			tmp[len] = (BYTE)ch;

		if ( ch == EOF )
			m_EncodeEof = TRUE;

		if ( !m_EncStart ) {
			m_EncStart = TRUE;
			m_EncodeSize = 0LL;
			m_HighWordSize = 0;
		}

		if ( len > 0 ) {
			if ( m_HighWordSize != (WORD)(m_EncodeSize >> 16) ) {
				m_HighWordSize = (WORD)(m_EncodeSize >> 16);

				if ( m_FileSize <= 0xFFFFFLL ) {
					head[0] = 0x02;
					head[1] = 0x00;
					head[2] = 0x00;
					head[3] = 0x02;		// 拡張セグメントアドレス
					head[4] = (BYTE)(m_HighWordSize << (12 - 8));
					head[5] = (BYTE)(m_HighWordSize << 12);
				} else {
					head[0] = 0x02;
					head[1] = 0x00;
					head[2] = 0x00;
					head[3] = 0x04;		// 拡張リニアアドレス
					head[4] = (BYTE)(m_HighWordSize >> 8);
					head[5] = (BYTE)(m_HighWordSize);
				}

				sum = 0;
				for ( int n = 0 ; n < 6 ; n++ )
					sum += head[n];
				head[6] = (BYTE)(0 - sum);

				m_EncodeBuffer.PutByte(':');
				m_EncodeBuffer.IntelHexEncode(head, 7);
				m_EncodeBuffer.PutByte('\n');
			}

			head[0] = (BYTE)len;
			head[1] = (BYTE)(m_EncodeSize >> 8);
			head[2] = (BYTE)(m_EncodeSize);
			head[3] = 0x00;		// データ

			// データがHighWord境界を超えない
			ASSERT(m_HighWordSize == (WORD)((m_EncodeSize + len - 1) >> 16));
			m_EncodeSize += len;

			sum = 0;
			for ( int n = 0 ; n < 4 ; n++ )
				sum += head[n];
			for ( int n = 0 ; n < len ; n++ )
				sum += tmp[n];
			tmp[len] = (BYTE)(0 - sum);

			m_EncodeBuffer.PutByte(':');
			m_EncodeBuffer.IntelHexEncode(head, 4);
			m_EncodeBuffer.IntelHexEncode(tmp, len + 1);
			m_EncodeBuffer.PutByte('\n');
		}

		if ( m_EncodeEof )
			m_EncodeBuffer += ":00000001FF\n";
	}
}
int CFileUpDown::GetSRecordEncode(GETPROCLIST *pProc)
{
	int len, ch, ext;
	BYTE sum, type;
	BYTE head[8];
	BYTE tmp[34];

	for ( ; ; ) {
		if ( m_EncodeBuffer.GetSize() > 0 )
			return m_EncodeBuffer.GetByte();

		if ( m_EncodeEof ) {
			m_EncStart = FALSE;
			m_EncodeEof = FALSE;
			return EOF;
		}

		if ( !m_EncStart ) {
			LPCSTR p = m_FileName;
			for ( len = 0 ; len < 31 && *p != '\0' ; len++ )
				tmp[len] = *(p++);
			tmp[len++] = '\0';

			// S0 recode
			type = '0';
			ext = 0;
			head[ext++] = (BYTE)(len + 2 + 1);
			head[ext++] = 0x00;
			head[ext++] = 0x00;

			sum = 0;
			for ( int n = 0 ; n < ext ; n++ )
				sum += head[n];
			for ( int n = 0 ; n < len ; n++ )
				sum += tmp[n];
			tmp[len] = (BYTE)(~sum);

			m_EncodeBuffer.PutByte('S');
			m_EncodeBuffer.PutByte(type);
			m_EncodeBuffer.IntelHexEncode(head, ext);
			m_EncodeBuffer.IntelHexEncode(tmp, len + 1);
			m_EncodeBuffer.PutByte('\n');

			m_EncStart = TRUE;
			m_EncodeSize = 0LL;
		}

		for ( len = 0 ; len < 32 && (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ; len++ )
			tmp[len] = (BYTE)ch;

		if ( ch == EOF )
			m_EncodeEof = TRUE;

		if ( len > 0 ) {
			ext = 0;
			if ( m_FileSize <= 0xFFFFLL ) {
				// S1 recode
				type = '1';
				head[ext++] = (BYTE)(len + 2 + 1);
				head[ext++] = (BYTE)(m_EncodeSize >> 8);
				head[ext++] = (BYTE)(m_EncodeSize);
			} else if ( m_FileSize <= 0xFFFFFFLL ) {
				// S2 recode
				type = '2';
				head[ext++] = (BYTE)(len + 3 + 1);
				head[ext++] = (BYTE)(m_EncodeSize >> 16);
				head[ext++] = (BYTE)(m_EncodeSize >> 8);
				head[ext++] = (BYTE)(m_EncodeSize);
			} else {
				// S3 recode
				type = '3';
				head[ext++] = (BYTE)(len + 4 + 1);
				head[ext++] = (BYTE)(m_EncodeSize >> 24);
				head[ext++] = (BYTE)(m_EncodeSize >> 16);
				head[ext++] = (BYTE)(m_EncodeSize >> 8);
				head[ext++] = (BYTE)(m_EncodeSize);
			}

			m_EncodeSize += len;

			sum = 0;
			for ( int n = 0 ; n < ext ; n++ )
				sum += head[n];
			for ( int n = 0 ; n < len ; n++ )
				sum += tmp[n];
			tmp[len] = (BYTE)(~sum);

			m_EncodeBuffer.PutByte('S');
			m_EncodeBuffer.PutByte(type);
			m_EncodeBuffer.IntelHexEncode(head, ext);
			m_EncodeBuffer.IntelHexEncode(tmp, len + 1);
			m_EncodeBuffer.PutByte('\n');
		}

		if ( m_EncodeEof ) {
			if ( m_FileSize <= 0xFFFFLL )
				m_EncodeBuffer += "S9030000FC\n";		// S9 recode
			else if ( m_FileSize <= 0xFFFFFFLL )
				m_EncodeBuffer += "S804000000FB\n";		// S8 recode
			else
				m_EncodeBuffer += "S70500000000FA\n";	// S7 recode
		}
	}
}
int CFileUpDown::GetTekHexEncode(GETPROCLIST *pProc)
{
	int len, ch;
	int blen, type, alen;
	BYTE sum;
	BYTE tmp[34];
	LONGLONG asz;

	for ( ; ; ) {
		if ( m_EncodeBuffer.GetSize() > 0 )
			return m_EncodeBuffer.GetByte();

		if ( m_EncodeEof ) {
			m_EncStart = FALSE;
			m_EncodeEof = FALSE;
			return EOF;
		}

		for ( len = 0 ; len < 32 && (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ; len++ )
			tmp[len] = (BYTE)ch;

		if ( ch == EOF )
			m_EncodeEof = TRUE;

		if ( !m_EncStart ) {
			m_EncStart = TRUE;
			m_EncodeSize = 0LL;
		}

		if ( len > 0 ) {
			if ( (m_EncodeSize & 0xFFFF0000LL) != 0 ) {
				if ( (m_EncodeSize & 0xF0000000LL) != 0 )
					alen = 8;
				else if ( (m_EncodeSize & 0x0F000000LL) != 0 )
					alen = 7;
				else if ( (m_EncodeSize & 0x00F00000LL) != 0 )
					alen = 6;
				else
					alen = 5;
			}else {
				if ( (m_EncodeSize & 0xF000LL) != 0 )
					alen = 4;
				else if ( (m_EncodeSize & 0x0F00LL) != 0 )
					alen = 3;
				else if ( (m_EncodeSize & 0x00F0LL) != 0 )
					alen = 2;
				else
					alen = 1;
			}

			type = 6;
			blen = 2 + 1 + 2 + 1 + alen + len * 2;

			sum =  (BYTE)((blen >> 4) & 15);
			sum += (BYTE)(blen & 15);
			sum += (BYTE)type;
			sum += (BYTE)alen;

			asz = m_EncodeSize;
			for ( int n = 0 ; n < alen ; n++ ) {
				sum += (BYTE)(asz & 15);
				asz >>= 4;
			}

			for ( int n = 0 ; n < len ; n++ ) {
				sum += (BYTE)((tmp[n] >> 4) & 15);
				sum += (BYTE)(tmp[n] & 15);
			}

			m_EncodeBuffer.PutByte('%');
			m_EncodeBuffer.TekHexEncode(blen, 2);
			m_EncodeBuffer.TekHexEncode(type, 1);
			m_EncodeBuffer.TekHexEncode(sum, 2);
			m_EncodeBuffer.TekHexEncode(alen, 1);
			m_EncodeBuffer.TekHexEncode((int)m_EncodeSize, alen);
			for ( int n = 0 ; n < len ; n++ )
				m_EncodeBuffer.TekHexEncode(tmp[n], 2);
			m_EncodeBuffer.PutByte('\n');

			m_EncodeSize += len;
		}

		if ( m_EncodeEof )
			m_EncodeBuffer += "%0781010\n";
	}
}
int CFileUpDown::GetCrLf(GETPROCLIST *pProc)
{
	int ch = EOF;

	for ( ; ; ) {
		if ( m_CrLfBuffer.GetSize() > 0 )
			return m_CrLfBuffer.GetByte();
		
		if ( m_CrLfEof ) {
			m_CrLfEof = FALSE;
			return EOF;
		}
		
		while ( m_CrLfBuffer.GetSize() < 1024 && (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ) {
			if ( ch == '\n' ) {
				m_CrLfBuffer += CrLfStr[m_UpCrLfMode];
				if ( m_CrLfBuffer.GetSize() > m_FileLen )
					break;
			} else if ( ch == '\r' ) {
				ch = (this->*pProc->GetProc)(pProc->pNext);
				if ( ch == '\n' )
					m_CrLfBuffer += CrLfStr[m_UpCrLfMode];
				else {
					m_CrLfBuffer += CrLfStr[m_UpCrLfMode];
					(this->*pProc->UnGetProc)(ch);
				}
				if ( m_CrLfBuffer.GetSize() > m_FileLen )
					break;
			} else
				m_CrLfBuffer.PutByte(ch);
		}

		if ( ch == EOF )
			m_CrLfEof = TRUE;
	}
}
void CFileUpDown::UnGetCrLf(int ch)
{
	BYTE tmp[4];

	if ( ch == EOF )
		return;

	tmp[0] = (BYTE)ch;
	m_CrLfBuffer.Insert(tmp, 1);
}

BOOL CFileUpDown::ReadLine(GETPROCLIST *pProc, CBuffer &out)
{
	int ch, last = (-1);

	out.Clear();

	if ( m_LineEof ) {
		m_LineEof = FALSE;
		return FALSE;
	}

	while ( (ch = (this->*pProc->GetProc)(pProc->pNext)) != EOF ) {
		out.PutByte(ch);
		if ( ch == '\n' )
			break;
		else if ( last == '\r' ) {
			(this->*pProc->UnGetProc)(ch);
			break;
		}
		last = ch;
	}

	if ( ch == EOF ) {
		if ( out.GetSize() <= 0 )
			return FALSE;
		m_LineEof = TRUE;
	}

	return TRUE;
}
BOOL CFileUpDown::ReadSize(GETPROCLIST *pProc, CBuffer &out, int size)
{
	int len, ch = EOF;

	out.Clear();

	if ( m_SizeEof ) {
		m_SizeEof = FALSE;
		return FALSE;
	}

	for ( len = 0 ; len < size && (ch =  (this->*pProc->GetProc)(pProc->pNext)) != EOF ; len++ )
		out.PutByte(ch);

	if ( ch == EOF ) {
		if ( out.GetSize() <= 0 )
			return FALSE;
		m_SizeEof = TRUE;
	}

	return TRUE;
}

/*
	〇文字コードの変換やエンコードを行わない
	〇文字コードの変換を行う From [EUC/SJIS/UTF9...] To [EUC/SJIS/UTF9...]
	〇エンコード処理を行う							[uuencode/base64]
	□改行コードを変換する							[CR/LF/CR+LF]

	〇遅延処理無しで送信する
	〇指定サイズ(byte)で送信遅延(ms)				[1-1024KB]  [0-10000ms]
	〇行単位で送信遅延 文字単位(us) 行単位(ms)		[0-10000us] [0-10000ms]
		□受信が一定時間(ms)無い場合に次を送信		[0-10000ms]
		□改行を確認し指定時間(ms)待って次を送信	[0-10000ms]
	□XON/XOFF文字の受信でフロー制御する
	□送信中に受信した文字を表示する
*/
void CFileUpDown::UpLoad()
{
	int n, len, ch;
	GETPROCLIST *pProc, procTab[4];
	CBuffer tmp;
	BOOL bHaveData;
	int nWaitCrLf;
	int maxMsec, minMsec, reqMsec, watMsec;
	clock_t st;
	char buf[1024];

RECHECK:
	if ( !CheckFileName(CHKFILENAME_OPEN | CHKFILENAME_MULTI, "", EXTFILEDLG_UPLOAD) )
		return;

	m_FileBuffer.Clear();
	m_IConvEof = FALSE;
	m_IConvBuffer.Clear();
	m_EncStart = FALSE;
	m_EncodeEof = FALSE;
	m_EncodeBuffer.Clear();
	m_CrLfEof = FALSE;
	m_CrLfBuffer.Clear();
	m_LineEof = FALSE;
	m_SizeEof = FALSE;

	n = 0;
	procTab[n].pNext = NULL;
	pProc = &(procTab[n++]);
	pProc->GetProc = &CFileUpDown::GetFile;
	pProc->UnGetProc = &CFileUpDown::UnGetFile;

	switch(m_UpMode) {
	case UPMODE_ICONV:
		procTab[n].pNext = pProc;
		pProc = &(procTab[n++]);
		pProc->GetProc = &CFileUpDown::GetIConv;
		pProc->UnGetProc = &CFileUpDown::UnGetIConv;
		break;
	case UPMODE_ENCODE:
		switch(m_EncMode) {
		case EDCODEMODE_UUENC:
			procTab[n].pNext = pProc;
			pProc = &(procTab[n++]);
			pProc->GetProc = &CFileUpDown::GetEncode;
			pProc->UnGetProc = &CFileUpDown::UnGetEncode;
			break;
		case EDCODEMODE_BASE64:
			procTab[n].pNext = pProc;
			pProc = &(procTab[n++]);
			pProc->GetProc = &CFileUpDown::GetBase64;
			pProc->UnGetProc = &CFileUpDown::UnGetEncode;
			break;
		case EDCODEMODE_ISH:
			n = 0;
			procTab[n].pNext = NULL;
			pProc = &(procTab[n++]);
			pProc->GetProc = &CFileUpDown::GetIshFile;
			pProc->UnGetProc = &CFileUpDown::UnGetFile;
			break;
		case EDCODEMODE_IHEX:
			procTab[n].pNext = pProc;
			pProc = &(procTab[n++]);
			pProc->GetProc = &CFileUpDown::GetIntelHexEncode;
			pProc->UnGetProc = &CFileUpDown::UnGetEncode;
			m_bSizeLimit = TRUE;
			break;
		case EDCODEMODE_SREC:
			procTab[n].pNext = pProc;
			pProc = &(procTab[n++]);
			pProc->GetProc = &CFileUpDown::GetSRecordEncode;
			pProc->UnGetProc = &CFileUpDown::UnGetEncode;
			m_bSizeLimit = TRUE;
			break;
		case EDCODEMODE_THEX:
			procTab[n].pNext = pProc;
			pProc = &(procTab[n++]);
			pProc->GetProc = &CFileUpDown::GetTekHexEncode;
			pProc->UnGetProc = &CFileUpDown::UnGetEncode;
			m_bSizeLimit = TRUE;
			break;
		}
		break;
	}

	if ( m_bUpCrLf ) {
		procTab[n].pNext = pProc;
		pProc = &(procTab[n++]);
		pProc->GetProc = &CFileUpDown::GetCrLf;
		pProc->UnGetProc = &CFileUpDown::UnGetCrLf;
	}

	bHaveData = TRUE;
	nWaitCrLf = 0;
	maxMsec = minMsec = reqMsec = watMsec = 0;

	while ( !AbortCheck() ) {
		switch(m_SendMode) {
		case SENDMODE_NONE:
			bHaveData = ReadSize(pProc, tmp, 1024);
			break;
		case SENDMODE_BLOCK:
			bHaveData = ReadSize(pProc, tmp, m_BlockSize);
			maxMsec = m_BlockMsec * tmp.GetSize() / m_BlockSize;
			break;
		case SENDMODE_LINE:
			bHaveData = ReadLine(pProc, tmp);
			maxMsec = m_CharUsec * tmp.GetSize() / 1000 + m_LineMsec;
			if ( m_bRecvWait )
				reqMsec = m_RecvMsec;
			if ( m_bCrLfWait ) {
				watMsec = m_CrLfMsec;
				nWaitCrLf = 1;
			}
			break;
		}

		if ( !bHaveData )
			break;

		Bufferd_SendBuf((char *)tmp.GetPtr(), tmp.GetSize());
		Bufferd_Flush();

		st = clock();
		do {
			n = maxMsec - ((clock() - st) * 1000 / CLOCKS_PER_SEC);

			if ( minMsec != 0 && minMsec < n )
				n = minMsec;

			if ( n > 0 ) {
				if ( (ch = Bufferd_Receive(0, n)) < 0 )
					break;
				buf[0] = (char)ch;
				Bufferd_ReceiveBack(buf, 1);
			}

			while ( (len = Bufferd_ReceiveSize()) > 0 ) {
				if ( len > 1024 )
					len = 1024;
				if ( !Bufferd_ReceiveBuf(buf, len, 0) )
					break;

				if ( m_bRecvEcho )
					SendEchoBuffer(buf, len);

				if ( m_bXonXoff ) {
					// XOFF 19=CTRL+S(1/3)
					// XON	17=CTRL+Q(1/1)
					for ( n = 0 ; n < len ; n++ ) {
						if ( buf[n] == 19 )
							maxMsec = 10000;
						else if ( buf[n] == 17 )
							maxMsec = 0;
						else if ( nWaitCrLf == 1 && (buf[n] == '\r' || buf[n] == '\n') ) {
							st = clock();
							maxMsec = watMsec;
							nWaitCrLf = 2;
						}
					}
				} else if ( nWaitCrLf == 1  ) {
					for ( n = 0 ; n < len ; n++ ) {
						if ( buf[n] == '\r' || buf[n] == '\n' ) {
							st = clock();
							maxMsec = watMsec;
							nWaitCrLf = 2;
							break;
						}
					}
				}

				if ( minMsec == 0 && reqMsec != 0 )
					minMsec = reqMsec;
			}

		} while ( ((clock() - st) * 1000 / CLOCKS_PER_SEC) < maxMsec );
	}

	if ( m_FileHandle != NULL ) {
		fclose(m_FileHandle);
		m_FileHandle = NULL;
	}

	// m_MultiFileの扱いに注意
	if ( m_MultiFile && !AbortCheck() && !m_ResvPath.IsEmpty() )
		goto RECHECK;
}
