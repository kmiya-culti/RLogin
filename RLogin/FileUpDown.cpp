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

	m_UpFp         = NULL;
	m_IConvEof     = FALSE;
	m_EncStart     = FALSE;
	m_EncodeEof    = FALSE;
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

/*
	�Z�����̕ϊ���f�R�[�h���s��Ȃ�
	�Z�����R�[�h�̕ϊ����s�� From [EUC/SJIS/UTF9...] To [EUC/SJIS/UTF9...]
	�Z�f�R�[�h�������s��							[uudecode/base64]
	���w�莞��(sec)��M��������ΏI������			[0123456789]
	�����s�R�[�h��ϊ�����							[CR/LF/CR+LF]
	����M����������\������
*/

void CFileUpDown::DownLoad()
{
	int ch, len;
	int last = (-1);
	int uudec = 0;
	FILE *fp = NULL;
	struct _stati64 st;
	BOOL bRewSize = TRUE;
	LONGLONG TranSize;
	CBuffer tmp, work, line;
	LPCSTR p;
	CStringA msg;
	CIsh ish;
	CStringBinary ishVolPath;

	if ( !CheckFileName(CHKFILENAME_SAVE, "", EXTFILEDLG_DOWNLOAD) )
		return;

	if ( m_bFileAppend && !_tstati64(m_PathName, &st) && (st.st_mode & _S_IFMT) == _S_IFREG ) {
		if ( (fp = _tfopen(m_PathName, _T("r+b"))) == NULL ) {
			::AfxMessageBox(IDE_FILEOPENERROR, MB_ICONERROR);
			return;
		}
		_fseeki64(fp, 0, SEEK_END);

	} else if ( (fp = _tfopen(m_PathName, _T("wb"))) == NULL ) {
		::AfxMessageBox(IDE_FILEOPENERROR, MB_ICONERROR);
		return;
	}

	UpDownOpen("Simple File Download");
	UpDownInit(0, 0);
	TranSize = 0;

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
			TranSize += tmp.GetSize();
			UpDownStat(TranSize);
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
			fwrite(tmp.GetPtr(), 1, tmp.GetSize(), fp);
			break;

		case DOWNMODE_ICONV:
			work.Clear();
			m_IConv.IConvSub(m_DownFrom, m_DownTo, &tmp, &work);
			fwrite(work.GetPtr(), 1, work.GetSize(), fp);
			break;

		case DOWNMODE_DECODE:
			while ( tmp.GetSize() > 0 ) {
				ch = tmp.GetByte();
				if ( ch == '\r' || ch == '\n' || ch == '\007' ) {
					if ( line.GetSize() <= 0 )
						continue;

					switch(m_DecMode) {
					case EDCODEMODE_UUENC:
						switch(uudec) {
						case 0:
							if ( strncmp((LPCSTR)line, "begin ", 6) == 0 )
								uudec = 1;
							else if( strncmp((LPCSTR)line, "begin-base64 ", 13) == 0 )
								uudec = 2;
							else
								break;

							p = line;
							// skip begin...
							while ( *p >  ' ' ) p++;
							while ( *p == ' ' ) p++;
							// skip 644..
							while ( *p >  ' ' ) p++;
							while ( *p == ' ' ) p++;

							if ( fp == NULL ) {
								if ( !CheckFileName(CHKFILENAME_SAVE, p) )
									goto ENDOFRET;
								if ( m_bFileAppend && !_tstati64(m_PathName, &st) && (st.st_mode & _S_IFMT) == _S_IFREG ) {
									if ( (fp = _tfopen(m_PathName, _T("r+b"))) == NULL ) {
										::AfxMessageBox(IDE_FILEOPENERROR, MB_ICONERROR);
										goto ENDOFRET;
									}
									_fseeki64(fp, 0, SEEK_END);
								} else if ( (fp = _tfopen(m_PathName, _T("wb"))) == NULL ) {
									::AfxMessageBox(IDE_FILEOPENERROR, MB_ICONERROR);
									goto ENDOFRET;
								}
								UpDownInit(0, 0);
							}

							msg.Format("uudecode start '%s'", p);
							UpDownMessage(msg);
							break;
						case 1:
							if ( strncmp((LPCSTR)line, "end", 3) == 0 ) {
								uudec = 0;
								if ( fp != NULL )
									fclose(fp);
								fp = NULL;
								UpDownMessage("uudecode end... closed");
							} else {
								work.UuDecode((LPCSTR)line);
								if ( work.GetSize() > 0 )
									fwrite(work.GetPtr(), 1, work.GetSize(), fp);
							}
							break;
						case 2:
							if ( strncmp((LPCSTR)line, "====", 4) == 0 ) {
								uudec = 0;
								if ( fp != NULL )
									fclose(fp);
								fp = NULL;
								UpDownMessage("uudecode end... closed");
							} else {
								work.Base64Decode((LPCSTR)line);
								if ( work.GetSize() > 0 )
									fwrite(work.GetPtr(), 1, work.GetSize(), fp);
							}
						}
						break;

					case EDCODEMODE_BASE64:
						work.Base64Decode((LPCSTR)line);
						if ( work.GetSize() > 0 )
							fwrite(work.GetPtr(), 1, work.GetSize(), fp);
						break;

					case EDCODEMODE_ISH:
						switch(ish.DecodeLine((LPCSTR)line, work)) {
						case ISH_RET_HEAD:
							if ( fp == NULL ) {
								if ( ish.m_VolSeq != 0 && ishVolPath.Find(ish.m_VolChkName) != NULL ) {
									m_PathName = ishVolPath[ish.m_VolChkName];
									fp = _tfopen(m_PathName, _T("r+b"));
								} else if ( !CheckFileName(CHKFILENAME_SAVE, ish.m_FileName) )
									goto ENDOFRET;
								else if ( m_bFileAppend && !_tstati64(m_PathName, &st) && (st.st_mode & _S_IFMT) == _S_IFREG ) {
									if ( (fp = _tfopen(m_PathName, _T("r+b"))) != NULL )
										_fseeki64(fp, 0, SEEK_END);
								} else
									fp = _tfopen(m_PathName, _T("wb"));

								if ( fp == NULL ) {
									::AfxMessageBox(IDE_FILEOPENERROR, MB_ICONERROR);
									goto ENDOFRET;
								}
							}
							if ( fp != NULL && ish.m_VolSeq != 0 ) {
								_fseeki64(fp, ish.m_FileSeek, SEEK_SET);
								ishVolPath[ish.m_VolChkName] = m_PathName;
							}
							UpDownInit(ish.m_FileSize);
							bRewSize = FALSE;
							if ( ish.m_VolSeq != 0 )
								msg.Format("ish start '%s' #%d", ish.m_FileName, ish.m_VolSeq);
							else
								msg.Format("ish start '%s'", ish.m_FileName);
							UpDownMessage(msg);
							break;
						case ISH_RET_DATA:
							if ( fp == NULL )
								break;
							if ( work.GetSize() > 0 )
								fwrite(work.GetPtr(), 1, work.GetSize(), fp);
							UpDownStat(ish.GetSize());
							break;
						case ISH_RET_ENDOF:
							if ( fp == NULL )
								break;
							if ( work.GetSize() > 0 )
								fwrite(work.GetPtr(), 1, work.GetSize(), fp);
							if ( ish.m_VolSeq != 0 )
								ishVolPath[ish.m_VolChkName][ish.m_VolSeq] = 1;
							UpDownStat(ish.m_FileSize);
							fclose(fp);
							fp = NULL;
							if ( ish.m_FileTime != 0 ) {
								struct _utimbuf utm;
								utm.actime  = ish.m_FileTime;
								utm.modtime = ish.m_FileTime;
								_tutime(m_PathName, &(utm));
							}
							if ( ish.m_VolSeq != 0 ) {
								msg.Format("ish end '%s' (%d/%d)", ish.m_FileName, ishVolPath[ish.m_VolChkName].GetSize(), ish.m_VolMax);
								UpDownMessage(msg);
							} else
								UpDownMessage("ish end... closed");
							break;
						case ISH_RET_ECCERR:
							if ( fp == NULL )
								break;
							fclose(fp);
							fp = NULL;
							UpDownMessage("ish ECC error... closed");
							break;
						case ISH_RET_CRCERR:
							if ( fp == NULL )
								break;
							fclose(fp);
							fp = NULL;
							UpDownMessage("ish CRC error... closed");
							break;
						}
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
	if ( fp != NULL )
		fclose(fp);
	UpDownClose();
}

//////////////////////////////////////////////////////////////////////

BOOL CFileUpDown::GetFile(GETPROCLIST *pProc)
{
	struct _stati64 st;
	int len;
	
	if ( AbortCheck() ) {
		UpDownClose();
		if ( m_UpFp != NULL ) {
			fclose(m_UpFp);
			m_UpFp = NULL;
		}
		return EOF;
	}

	if ( m_FileBuffer.GetSize() > 0 )
		return m_FileBuffer.GetByte();

	if ( m_UpFp == NULL ) {
		if ( m_PathName.IsEmpty() )
			return EOF;

		if ( _tstati64(m_PathName, &st) || (st.st_mode & _S_IFMT) != _S_IFREG )
			return EOF;

		if ( (m_UpFp = _tfopen(m_PathName, _T("rb"))) == NULL )
			return EOF;

		m_FileSize = st.st_size;
		m_TranSize = 0;
		m_FileBuffer.Clear();

		if ( (m_FileLen = (int)(m_FileSize / 100)) < 32 )
			m_FileLen = 32;
		else if ( m_FileLen > 4096 )
			m_FileLen = 4096;

		UpDownOpen("Simple File Upload");
		UpDownInit(m_FileSize);
	}

	if ( (len = (int)fread(m_FileBuffer.PutSpc(m_FileLen), 1, m_FileLen, m_UpFp)) <= 0 ) {
		UpDownClose();
		fclose(m_UpFp);
		m_UpFp = NULL;
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
		if ( m_UpFp != NULL ) {
			fclose(m_UpFp);
			m_UpFp = NULL;
		}
		return EOF;
	}

	if ( m_FileBuffer.GetSize() > 0 )
		return m_FileBuffer.GetByte();

	if ( m_UpFp == NULL ) {
		if ( m_PathName.IsEmpty() )
			return EOF;

		if ( _tstati64(m_PathName, &st) || (st.st_mode & _S_IFMT) != _S_IFREG )
			return EOF;

		if ( (m_UpFp = _tfopen(m_PathName, _T("rb"))) == NULL )
			return EOF;

		m_FileSize = st.st_size;
		m_TranSize = 0;
		m_FileBuffer.Clear();

		CheckShortName();

		if ( !m_Ish.EncodeHead(ISH_LEN_JIS7, m_UpFp, m_FileName, st.st_size, st.st_mtime, m_FileBuffer) ) {
			::AfxMessageBox(_T("ish file size limit error"), MB_ICONERROR);
			return EOF;
		}

		UpDownOpen("Ish File Upload");
		UpDownInit(m_FileSize);

	} else {
		if ( (len = m_Ish.EncodeBlock(m_UpFp, m_FileBuffer)) < 0 ) {
			UpDownClose();
			fclose(m_UpFp);
			m_UpFp = NULL;
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
	int ch;
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
int CFileUpDown::GetCrLf(GETPROCLIST *pProc)
{
	int ch;

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
	int len, ch;

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
	�Z�����R�[�h�̕ϊ���G���R�[�h���s��Ȃ�
	�Z�����R�[�h�̕ϊ����s�� From [EUC/SJIS/UTF9...] To [EUC/SJIS/UTF9...]
	�Z�G���R�[�h�������s��							[uuencode/base64]
	�����s�R�[�h��ϊ�����							[CR/LF/CR+LF]

	�Z�x�����������ő��M����
	�Z�w��T�C�Y(byte)�ő��M�x��(ms)				[1-1024KB]  [0-10000ms]
	�Z�s�P�ʂő��M�x�� �����P��(us) �s�P��(ms)		[0-10000us] [0-10000ms]
		����M����莞��(ms)�����ꍇ�Ɏ��𑗐M		[0-10000ms]
		�����s���m�F���w�莞��(ms)�҂��Ď��𑗐M	[0-10000ms]
	��XON/XOFF�����̎�M�Ńt���[���䂷��
	�����M���Ɏ�M����������\������
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

	if ( m_UpFp != NULL ) {
		fclose(m_UpFp);
		m_UpFp = NULL;
	}

	// m_MultiFile�̈����ɒ���
	if ( m_MultiFile && !AbortCheck() && !m_ResvPath.IsEmpty() )
		goto RECHECK;
}
