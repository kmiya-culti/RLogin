// SyncSock.cpp: CSyncSock クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "ExtSocket.h"
#include "SyncSock.h"
#include "ExtFileDialog.h"

#include <mbctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CSyncSock::CSyncSock(class CRLoginDoc *pDoc, CWnd *pWnd)
{
	m_pDoc = pDoc;
	m_pMainWnd = pWnd;
	m_pViewWnd = NULL;
	m_ProtoName = _T("None");
	m_DoCommand = 0;

	m_ThreadMode = THREAD_NONE;
	m_pSyncThread = NULL;

	m_RecvBuf.Clear();
	m_SendBuf.Clear();

	m_pProgDlg = NULL;
	m_AbortFlag = FALSE;

	m_ResvDoit = FALSE;
	m_MultiFile = FALSE;
	m_IsAscii = FALSE;
	m_bUseWrite = FALSE;
	m_ExtFileDlgMode = 0;
	m_bInitDone = FALSE;
	m_Param = 0;
	m_pLongLong[0] = m_pLongLong[1] = 0LL;
}

CSyncSock::~CSyncSock()
{
	DoAbort(TRUE);
}

//////////////////////////////////////////////////////////////////////

static UINT ProcThread(LPVOID pParam)
{
	CSyncSock *pThis = (CSyncSock *)pParam;

	pThis->OnProc(pThis->m_DoCommand);

	if ( pThis->m_RecvBuf.GetSize() > 0 ) {
		pThis->m_pDoc->m_pSock->SyncReceiveBack(pThis->m_RecvBuf.GetPtr(), pThis->m_RecvBuf.GetSize());
		pThis->m_RecvBuf.Clear();
	}

	pThis->m_ThreadMode = CSyncSock::THREAD_ENDOF;
	pThis->m_pMainWnd->PostMessage(WM_THREADCMD, THCMD_ENDOF, (LPARAM)pThis);
	pThis->m_ThreadEvent.SetEvent();

	return 0;
}
void  CSyncSock::StatusMsg(int ids)
{
	CString str;

	str.Format(CStringLoad(ids), m_ProtoName);
	((CMainFrame *)m_pMainWnd)->SetStatusText(str);
}
LPCSTR CSyncSock::PathNameToFileName(LPCTSTR pathName)
{
	LPCTSTR p;

	if ( (p = _tcsrchr(pathName, _T('\\'))) != NULL || (p = _tcsrchr(pathName, _T(':'))) != NULL )
		p += 1;
	else
		p = pathName;

	return m_pDoc->RemoteStr(p);
}
BOOL CSyncSock::FileDlgProc()
{
	LPCTSTR p;

	if ( (p = _tcsrchr(m_PathName, _T('.'))) == NULL )
		p = _T(".");

	CExtFileDialog dlg(((m_Param & CHKFILENAME_OPEN) ? TRUE : FALSE), p + 1, m_PathName,
		OFN_HIDEREADONLY | OFN_ENABLESIZING | ((m_Param & CHKFILENAME_MULTI) != 0 ? OFN_ALLOWMULTISELECT : 0), 
		CStringLoad(IDS_FILEDLGALLFILE), m_pMainWnd, 0, (m_ExtFileDlgMode == 0 ? TRUE : FALSE), m_ExtFileDlgMode, this);

	if ( DpiAwareDoModal(dlg, m_ExtFileDlgMode != 0 ? REQDPICONTEXT_SCALED : REQDPICONTEXT_AWAREV2) != IDOK )
		goto FILENOSELECT;

	if ( (m_Param & CHKFILENAME_MULTI) != 0 ) {
		m_ResvPath.RemoveAll();

		POSITION pos = dlg.GetStartPosition();
		while ( pos != NULL )
			m_ResvPath.AddTail(dlg.GetNextPathName(pos));

		if ( m_ResvPath.IsEmpty() )
			goto FILENOSELECT;

		m_PathName = m_ResvPath.RemoveHead();
		m_FileName = PathNameToFileName(m_PathName);

		m_MultiFile = (!m_ResvPath.IsEmpty() ? TRUE : FALSE);

	} else {
		m_PathName = dlg.GetPathName();
		m_FileName = m_pDoc->RemoteStr(dlg.GetFileName());
	}

	return TRUE;

FILENOSELECT:
	if ( !m_ResvPath.IsEmpty() )
		m_ResvPath.RemoveAll();
	m_MultiFile = FALSE;

	m_PathName.Empty();
	m_FileName.Empty();

	return FALSE;
}
void CSyncSock::ThreadCommand(int cmd)
{
	CString work;

	ASSERT(m_pMainWnd != NULL && m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	switch(cmd) {
	case THCMD_START:
		if ( m_ThreadMode != THREAD_NONE )
			break;
		m_pViewWnd = m_pDoc->GetAciveView();

		m_ThreadMode = THREAD_RUN;
		m_ThreadEvent.ResetEvent();
		m_pSyncThread = AfxBeginThread(ProcThread, this, THREAD_PRIORITY_NORMAL);

		m_pDoc->SetSleepReq(SLEEPSTAT_DISABLE);
		StatusMsg(IDS_FILETRANSMITSTART);
		break;
	case THCMD_ENDOF:
		if ( m_ThreadMode != THREAD_ENDOF )
			break;

		m_ThreadMode = THREAD_NONE;
		((CMainFrame *)m_pMainWnd)->DelThreadMsg(this);
		m_pDoc->m_pSock->SetRecvSyncMode(FALSE);

		if ( m_ResvDoit && !m_AbortFlag && !m_ResvPath.IsEmpty() )
			m_pDoc->DoDropFile();
		m_ResvDoit = FALSE;

		if ( m_pProgDlg != NULL )
			m_pProgDlg->DestroyWindow();
		m_pProgDlg = NULL;

		m_pDoc->SetSleepReq(SLEEPSTAT_ENABLE);
		StatusMsg(IDS_FILETRANSMITCLOSE);
		break;
	case THCMD_DLGOPEN:
		if ( m_pProgDlg != NULL )
			break;
		m_pProgDlg = new CProgDlg(m_pMainWnd);
		m_pProgDlg->m_pAbortFlag = &m_AbortFlag;
		m_pProgDlg->m_AutoDelete = TRUE;
		m_pProgDlg->m_pSock = m_pDoc->m_pSock;
		m_pProgDlg->Create(IDD_PROGDLG, m_pMainWnd);
		m_pProgDlg->SetWindowText(MbsToTstr(m_Message));
		m_pProgDlg->ShowWindow(SW_SHOW);
		if ( m_pViewWnd != NULL )
			m_pViewWnd->SetFocus();
		break;
	case THCMD_DLGCLOSE:
		if ( m_pProgDlg != NULL )
			m_pProgDlg->DestroyWindow();
		m_pProgDlg = NULL;
		break;
	case THCMD_DLGMESSAGE:
		if ( m_pProgDlg != NULL )
			m_pProgDlg->SetMessage(m_pDoc->LocalStr(m_Message));
		break;
	case THCMD_DLGRANGE:
		if ( m_pProgDlg == NULL )
			break;
		m_pProgDlg->SetFileName(m_PathName);
		m_pProgDlg->SetRange(m_pLongLong[0], m_pLongLong[1]);
		break;
	case THCMD_CHECKPATH:
		if ( !m_ResvPath.IsEmpty() ) {
			m_PathName = m_ResvPath.RemoveHead();
			// ファイルドロップ時は、m_MultiFileがFALSE
			// ファイルダイアログ拡張時は、毎回オプションを確認するようにした
			if ( !m_MultiFile && m_ExtFileDlgMode != 0 ) {
				m_Param &= ~CHKFILENAME_MULTI;
				m_ResvDoit = (FileDlgProc() ? TRUE : FALSE);
			} else {
				m_FileName = PathNameToFileName(m_PathName);
				m_ResvDoit = TRUE;
			}

			if ( m_ResvPath.IsEmpty() )
				m_MultiFile = FALSE;
		} else {
			m_MultiFile = FALSE;
			m_PathName = m_pDoc->LocalStr(m_FileName);
			m_ResvDoit = (FileDlgProc() ? TRUE : FALSE);
		}
		break;
	case THCMD_YESNO:
		work = m_pDoc->LocalStr(m_Message);
		if ( ::AfxMessageBox(work, MB_ICONQUESTION | MB_YESNO) == IDYES )
			m_Param = 'Y';
		break;
	case THCMD_XONXOFF:
		m_pDoc->m_pSock->SetXonXoff(m_Param);
		break;
	case THCMD_SENDSTR:
		m_pDoc->SendBuffer(m_SendBuf);
		break;
	case THCMD_SENDSCRIPT:
		m_pDoc->SendScript((LPCWSTR)m_SendBuf.GetPtr(), NULL);
		break;
	case TGCMD_MESSAGE:
		::AfxMessageBox(MbsToTstr(m_Message), MB_ICONINFORMATION);
		break;
	}
}

void CSyncSock::OnProc(int cmd)
{
}

void CSyncSock::DoProc(int cmd)
{
	ASSERT(m_pMainWnd != NULL && m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	if ( m_ThreadMode != THREAD_NONE )
		return;

	m_bInitDone = FALSE;
	m_AbortFlag = FALSE;

	m_DoCommand = cmd;
	m_pDoc->m_pSock->SetRecvSyncMode(TRUE);

	((CMainFrame *)m_pMainWnd)->AddThreadMsg(this);

	m_pMainWnd->PostMessage(WM_THREADCMD, THCMD_START, (LPARAM)this);
}
void CSyncSock::DoAbort(BOOL bClose)
{
	if ( m_ThreadMode == THREAD_NONE )
		return;

	if ( !bClose ) {
		m_AbortFlag = TRUE;
		ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);
		m_pDoc->m_pSock->SyncAbort();
		return;
	}

	m_ThreadMode = THREAD_DOEND;
	((CMainFrame *)m_pMainWnd)->DelThreadMsg(this);

	m_AbortFlag = TRUE;
	if ( m_pDoc != NULL && m_pDoc->m_pSock != NULL )
		m_pDoc->m_pSock->SyncAbort();

	WaitForEvent(m_ThreadEvent, _T("CSyncSock Thread"));
	m_ThreadMode = THREAD_NONE;

	if ( m_pDoc != NULL && m_pDoc->m_pSock != NULL )
		m_pDoc->m_pSock->SetRecvSyncMode(FALSE);

	if ( m_pProgDlg != NULL )
		m_pProgDlg->DestroyWindow();
	m_pProgDlg = NULL;

	if ( !m_ResvPath.IsEmpty() )
		m_ResvPath.RemoveAll();

	if ( m_pDoc != NULL )
		m_pDoc->SetSleepReq(SLEEPSTAT_ENABLE);
	StatusMsg(IDS_FILETRANSMITCLOSE);
}

//////////////////////////////////////////////////////////////////////

#ifdef	USE_DEBUGLOG
void CSyncSock::DebugDump(LPBYTE buf, int len)
{
	CBuffer *pBuffer;

	if ( m_pView != NULL ) {
		pBuffer = new CBuffer(len);
		pBuffer->Apend(buf, len);
		m_pView->PostMessage(WM_LOGWRITE, (WPARAM)1, (LPARAM)pBuffer);
	}
}
void CSyncSock::DebugMsg(LPCSTR fmt, ...)
{
	va_list arg;
	CBuffer *pBuffer;
	CStringA tmp;

	va_start(arg, fmt);
	tmp.FormatV(fmt, arg);
	va_end(arg);

	if ( m_pView != NULL ) {
		pBuffer = new CBuffer((LPCSTR)tmp);
		m_pView->PostMessage(WM_LOGWRITE, (WPARAM)0, (LPARAM)pBuffer);
	}
}
#endif

void CSyncSock::Bufferd_Send(int c)
{
	m_SendBuf.Put8Bit(c);
}
void CSyncSock::Bufferd_SendBuf(char *buf, int len)
{
	m_SendBuf.Apend((LPBYTE)buf, len);
}
void CSyncSock::Bufferd_Flush()
{
	DebugMsg("Bufferd_Flush %d", m_SendBuf.GetSize());
	DebugDump(m_SendBuf.GetPtr(), m_SendBuf.GetSize() < 16 ? m_SendBuf.GetSize() : 16);

	ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	if ( m_SendBuf.GetSize() > 0 )
		m_pDoc->m_pSock->SyncSend(m_SendBuf.GetPtr(), m_SendBuf.GetSize(), INFINITE, &m_AbortFlag);
	m_SendBuf.Clear();
}
void CSyncSock::Bufferd_Clear()
{
	ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	m_SendBuf.Clear();

	if ( m_bInitDone ) {
		m_RecvBuf.Clear();
		m_pDoc->m_pSock->SyncRecvClear();
	}
}
int CSyncSock::Bufferd_Receive(int sec, int msec)
{
	int n;
	BYTE tmp[2048];

	ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return (-2);

	if ( m_RecvBuf.GetSize() <= 0 ) {
		if ( (n = m_pDoc->m_pSock->SyncReceive(tmp, 2048, sec * 1000 + msec, &m_AbortFlag)) <= 0 )
			return (-2);	// TIME OUT
		m_RecvBuf.Apend(tmp, n);

		DebugMsg("Bufferd_Receive %d", n);
		DebugDump(tmp, n < 16 ? n : 16);
	}

	return m_RecvBuf.Get8Bit();
}
BOOL CSyncSock::Bufferd_ReceiveBuf(char *buf, int len, int sec, int msec)
{
	int n;

	ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return FALSE;

	if ( (n = m_RecvBuf.GetSize()) > 0 ) {
		if ( n > len )
			n = len;
		memcpy(buf, m_RecvBuf.GetPtr(), n);
		m_RecvBuf.Consume(n);
		buf += n;
		len -= n;
	}

	while ( len > 0 ) {
		if ( (n = m_pDoc->m_pSock->SyncReceive(buf, len, sec * 1000 + msec, &m_AbortFlag)) <= 0 )
			return FALSE;

		DebugMsg("Bufferd_ReceiveBuf %d", n);
		DebugDump((LPBYTE)buf, n < 16 ? n : 16);

		buf += n;
		len -= n;
	}

	return TRUE;
}
void CSyncSock::Bufferd_ReceiveBack(char *buf, int len)
{
	m_RecvBuf.Insert((LPBYTE)buf, len);
}
int CSyncSock::Bufferd_ReceiveSize()
{
	ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	return m_pDoc->m_pSock->GetRecvProcSize() + m_RecvBuf.GetSize();
}
void CSyncSock::SetXonXoff(int sw)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_Param = sw;
	m_pMainWnd->SendMessage(WM_THREADCMD, THCMD_XONXOFF, (LPARAM)this);
}
void CSyncSock::SendEcho(int ch)
{
	ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	BYTE c = (BYTE)ch;
	m_pDoc->m_pSock->SyncExtSend(&c, 1, INFINITE, &m_AbortFlag);
}
void CSyncSock::SendEchoBuffer(char *buf, int len)
{
	ASSERT(m_pDoc != NULL && m_pDoc->m_pSock != NULL);

	m_pDoc->m_pSock->SyncExtSend(buf,len, INFINITE, &m_AbortFlag);
}

//////////////////////////////////////////////////////////////////////

BOOL CSyncSock::CheckFileName(int mode, LPCSTR file, int extmode)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return FALSE;

	m_Param = mode;
	m_FileName = file;
	m_ExtFileDlgMode = extmode;
	m_pMainWnd->SendMessage(WM_THREADCMD, THCMD_CHECKPATH, (LPARAM)this);
	return (m_FileName.IsEmpty() || m_PathName.IsEmpty() ? FALSE : TRUE);
}
int CSyncSock::YesOrNo(LPCSTR msg)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return 'N';

	m_Param = 'N';
	m_Message = msg;
	m_pMainWnd->SendMessage(WM_THREADCMD, THCMD_YESNO, (LPARAM)this);
	return m_Param;
}
void CSyncSock::UpDownOpen(LPCSTR msg)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_Message = msg;
	m_pMainWnd->SendMessage(WM_THREADCMD, THCMD_DLGOPEN, (LPARAM)this);
}
void CSyncSock::UpDownClose()
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_pMainWnd->PostMessage(WM_THREADCMD, THCMD_DLGCLOSE, (LPARAM)this);
}
void CSyncSock::UpDownMessage(LPCSTR msg)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_Message = msg;
	m_pMainWnd->PostMessage(WM_THREADCMD, THCMD_DLGMESSAGE, (LPARAM)this);
}
void CSyncSock::UpDownInit(LONGLONG size, LONGLONG rems)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_pLongLong[0] = size;
	m_pLongLong[1] = rems;
	m_pMainWnd->SendMessage(WM_THREADCMD, THCMD_DLGRANGE, (LPARAM)this);
}
void CSyncSock::UpDownStat(LONGLONG size)
{
	if ( m_pProgDlg != NULL && (!m_pProgDlg->m_UpdatePost || m_pProgDlg->m_LastSize == size) ) {
		m_pProgDlg->m_UpdatePost = TRUE;
		m_pLongLong[0] = size;
		m_pProgDlg->PostMessage(WM_PROGUPDATE, PROG_UPDATE_POS, (LPARAM)m_pLongLong);
	}
}
void CSyncSock::SendString(LPCWSTR str)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_SendBuf.Clear();
	m_SendBuf.Apend((LPBYTE)str, (int)wcslen(str) * sizeof(WCHAR));
	m_pMainWnd->SendMessage(WM_THREADCMD, THCMD_SENDSTR, (LPARAM)this);
}
void CSyncSock::SendScript(LPCWSTR str)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_SendBuf.Clear();
	m_SendBuf.Apend((LPBYTE)str, (int)wcslen(str) * sizeof(WCHAR));
	m_pMainWnd->SendMessage(WM_THREADCMD, THCMD_SENDSCRIPT, (LPARAM)this);
}
void CSyncSock::Message(LPCSTR msg)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_Message = msg;
	m_pMainWnd->SendMessage(WM_THREADCMD, TGCMD_MESSAGE, (LPARAM)this);
}
void CSyncSock::NoWaitMessage(LPCSTR msg)
{
	ASSERT(m_pMainWnd != NULL);

	if ( m_ThreadMode >= THREAD_DOEND )
		return;

	m_Message = msg;
	m_pMainWnd->PostMessage(WM_THREADCMD, TGCMD_MESSAGE, (LPARAM)this);
}

//////////////////////////////////////////////////////////////////////

FILE *CSyncSock::FileOpen(LPCTSTR filename, LPCSTR mode, BOOL ascii)
{
	m_IsAscii = ascii;
	m_InBuf.Clear();
	m_OutBuf.Clear();
	m_IConv.IConvClose();
	m_HostCode = m_pDoc->m_TextRam.m_SendCharSet[m_pDoc->m_TextRam.m_KanjiMode];
	m_bUseWrite = FALSE;
	return _tfopen(filename, MbsToTstr(mode));
}
void CSyncSock::FileClose(FILE *fp)
{
	int n;
	CStringA mbs;
	CBuffer work;

	if ( m_bUseWrite && m_InBuf.GetSize() > 0 ) {
		m_IConv.IConvSub(m_HostCode, _T("UTF-16LE"), &m_InBuf, &work);

		mbs = (LPCWSTR)work;
		m_OutBuf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());

		if ( (n = (int)fwrite(m_OutBuf.GetPtr(), 1, m_OutBuf.GetSize(), fp)) > 0 )
			m_OutBuf.Consume(n);
	}

	if ( m_IConv.m_ErrCount > 0 || m_InBuf.GetSize() > 0 || m_OutBuf.GetSize() > 0 )
		NoWaitMessage(TstrToMbs(CStringLoad(IDE_KANJICONVERROR)));

	fclose(fp);
}
int CSyncSock::ReadCharToHost(FILE *fp)
{
	BYTE tmp[4];

	if ( !m_IsAscii )
		return fgetc(fp);

	if ( ReadFileToHost((char *)tmp, 1, fp) != 1 )
		return EOF;

	return tmp[0];
}
int CSyncSock::ReadFileToHost(char *buf, int len, FILE *fp)
{
	int n, i;
	char tmp[4096];
	CBuffer work;
	CStringW str;

	if ( !m_IsAscii )
		return (int)fread(buf, 1, len, fp);

	while ( m_OutBuf.GetSize() < len && !feof(fp) ) {
		if ( (n = (int)fread(tmp, 1, 4096, fp)) <= 0 )
			break;

		for ( i = 0 ; i < n ; i++ ) {
			if ( tmp[i] == '\n' ) {
				m_InBuf.Put8Bit(tmp[i]);

				str = (LPCSTR)m_InBuf;
				m_InBuf.Clear();

				work.Apend((LPBYTE)(LPCWSTR)str, str.GetLength() * sizeof(WCHAR));
				m_IConv.IConvSub(_T("UTF-16LE"), m_HostCode, &work, &m_OutBuf);

			} else if ( tmp[i] != '\r' )
				m_InBuf.Put8Bit(tmp[i]);
		}
	}

	if ( feof(fp) && m_InBuf.GetSize() > 0 ) {
		str = (LPCSTR)m_InBuf;
		m_InBuf.Clear();

		work.Apend((LPBYTE)(LPCWSTR)str, str.GetLength() * sizeof(WCHAR));
		m_IConv.IConvSub(_T("UTF-16LE"), m_HostCode, &work, &m_OutBuf);
	}

	if ( (n = (m_OutBuf.GetSize() < len ? m_OutBuf.GetSize() : len)) > 0 ) {
		memcpy(buf, m_OutBuf.GetPtr(), n);
		m_OutBuf.Consume(n);
	}

	return n;
}
int CSyncSock::WriteFileFromHost(char *buf, int len, FILE *fp)
{
	int n;
	CBuffer work;
	CStringA mbs;

	if ( !m_IsAscii )
		return (int)fwrite(buf, 1, len, fp);

	for ( n = 0 ; n < len ; n++ ) {
		if ( buf[n] == '\n' ) {
			m_InBuf.Put8Bit('\r');
			m_InBuf.Put8Bit('\n');

			work.Clear();
			m_IConv.IConvSub(m_HostCode, _T("UTF-16LE"), &m_InBuf, &work);

			mbs = (LPCWSTR)work;
			m_OutBuf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());

		} else if ( buf[n] != '\r' )
			m_InBuf.Put8Bit(buf[n]);

		m_bUseWrite = TRUE;
	}

	if ( (n = (int)fwrite(m_OutBuf.GetPtr(), 1, m_OutBuf.GetSize(), fp)) > 0 )
		m_OutBuf.Consume(n);

	return len;
}
