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
	m_pWnd = pWnd;
	m_pDoc = pDoc;
	m_pView = NULL;
	m_ProtoName = _T("None");
	m_SendBuf.Clear();
	m_DoAbortFlag = FALSE;
	m_ThreadFlag = FALSE;
	m_ThreadMode = 0;
	m_pThreadEvent = new CEvent(FALSE, TRUE);
	m_pParamEvent  = new CEvent(FALSE, TRUE);
	m_ResvDoit = FALSE;
	m_MultiFile = FALSE;
	m_IsAscii = FALSE;
	m_bUseWrite = FALSE;
	m_ExtFileDlgMode = 0;
	m_EchoPostReq = FALSE;
	m_bInitDone = FALSE;
	m_bUpdateReq = FALSE;
}

CSyncSock::~CSyncSock()
{
	DoAbort();
	delete m_pThreadEvent;
	delete m_pParamEvent;
}

//////////////////////////////////////////////////////////////////////

static UINT ProcThread(LPVOID pParam)
{
	CSyncSock *pThis = (CSyncSock *)pParam;

	pThis->m_ThreadFlag = TRUE;
	pThis->OnProc(pThis->m_ThreadMode);
	for ( int n = 0 ; n < 10 && pThis->m_EchoPostReq ; n++ )
		Sleep(100);
	if ( pThis->m_RecvBuf.GetSize() > 0 ) {
		pThis->m_pDoc->m_pSock->SyncReceiveBack(pThis->m_RecvBuf.GetPtr(), pThis->m_RecvBuf.GetSize());
		pThis->m_RecvBuf.Clear();
	}
	pThis->m_ThreadFlag = FALSE;
	pThis->m_pThreadEvent->SetEvent();
	if ( !pThis->m_DoAbortFlag )
		pThis->m_pWnd->PostMessage(WM_THREADCMD, THCMD_ENDOF, (LPARAM)pThis);
	return 0;
}
void  CSyncSock::StatusMsg(int ids)
{
	CString str;

	str.Format(CStringLoad(ids), m_ProtoName);
	((CMainFrame *)AfxGetMainWnd())->SetStatusText(str);
}
void CSyncSock::ThreadCommand(int cmd)
{
	int n;
	LPCTSTR p;
	CString work;
	LONGLONG pos;

	switch(cmd) {
	case THCMD_START:
		if ( m_ThreadFlag )
			break;
		m_pView = (m_pDoc == NULL ? NULL : m_pDoc->GetAciveView());
		m_pThreadEvent->ResetEvent();
		AfxBeginThread(ProcThread, this, THREAD_PRIORITY_NORMAL);
		Sleep(100);
		if ( m_pDoc != NULL )
			m_pDoc->SetSleepReq(SLEEPSTAT_DISABLE);
		StatusMsg(IDS_FILETRANSMITSTART);
		break;
	case THCMD_ENDOF:
		if ( m_DoAbortFlag )
			break;
		WaitForSingleObject(m_pThreadEvent->m_hObject, INFINITE);
		if ( m_pDoc->m_pSock != NULL )
			m_pDoc->m_pSock->SetRecvSyncMode(FALSE);
		if ( m_ResvDoit && !m_ResvPath.IsEmpty() )
			m_pDoc->DoDropFile();
		m_ResvDoit = FALSE;
		if ( m_pDoc != NULL )
			m_pDoc->SetSleepReq(SLEEPSTAT_ENABLE);
		StatusMsg(IDS_FILETRANSMITCLOSE);
		break;
	case THCMD_DLGOPEN:
		if ( m_ProgDlg.m_hWnd != NULL )
			break;
		if ( m_pDoc != NULL )
			m_ProgDlg.m_pSock = m_pDoc->m_pSock;
		m_ProgDlg.Create(IDD_PROGDLG, m_pWnd);
		m_ProgDlg.SetWindowText(MbsToTstr(m_Message));
		m_ProgDlg.ShowWindow(SW_SHOW);
		m_pParamEvent->SetEvent();
		if ( m_pView != NULL )
			m_pView->SetFocus();
		break;
	case THCMD_DLGCLOSE:
		if ( m_ProgDlg.m_hWnd == NULL )
			break;
		m_ProgDlg.DestroyWindow();
		break;
	case THCMD_DLGMESSAGE:
		work = m_pDoc->LocalStr(m_Message);
		if ( m_ProgDlg.m_hWnd == NULL )
			m_pWnd->MessageBox(work);
		else
			m_ProgDlg.SetMessage(work);
		break;
	case THCMD_DLGRANGE:
		if ( m_ProgDlg.m_hWnd == NULL )
			break;
		m_ProgDlg.SetFileName(m_PathName);
		m_ProgDlg.SetRange(m_Size, m_RemSize);
		m_pParamEvent->SetEvent();
		break;
	case THCMD_DLGPOS:
		m_ProgSema.Lock();
		m_bUpdateReq = FALSE;
		pos = m_Size;
		m_ProgSema.Unlock();
		if ( m_ProgDlg.m_hWnd != NULL )
			m_ProgDlg.SetPos(pos);
		break;
	case THCMD_SENDBUF:
		m_SendSema.Lock();
		m_SwapBuf.Swap(m_SendBuf);
		m_SendBuf.Clear();
		m_SendSema.Unlock();
		if ( m_pDoc->m_pSock != NULL )
			m_pDoc->m_pSock->Send(m_SwapBuf.GetPtr(), m_SwapBuf.GetSize(), 0);
		break;
	case THCMD_CHECKPATH:
		RECHECK:
		if ( !m_ResvPath.IsEmpty() ) {
			m_PathName = m_ResvPath.RemoveHead();

			// ファイルダイアログ拡張時は、毎回オプションを確認するようにした
			if ( !m_MultiFile && m_ExtFileDlgMode != 0 ) {
				if ( (p = _tcsrchr(m_PathName, _T('.'))) == NULL )
					p = _T(".");

				// FileUploadのファイルドロップ時は、シングルに強制
				m_Param &= ~CHKFILENAME_MULTI;

				CExtFileDialog dlg(((m_Param & CHKFILENAME_OPEN) ? TRUE : FALSE), p + 1, m_PathName,
					OFN_HIDEREADONLY | OFN_ENABLESIZING | ((m_Param & CHKFILENAME_MULTI) ? OFN_ALLOWMULTISELECT : 0), 
					CStringLoad(IDS_FILEDLGALLFILE), m_pWnd, 0, (m_ExtFileDlgMode == 0 ? TRUE : FALSE), m_ExtFileDlgMode, this);

				if ( DpiAwareDoModal(dlg, m_ExtFileDlgMode != 0 ? REQDPICONTEXT_SCALED : REQDPICONTEXT_AWAREV2) == IDOK ) {
					m_PathName = dlg.GetPathName();
					m_FileName = m_pDoc->RemoteStr(dlg.GetFileName());
					m_ResvDoit = TRUE;

				} else {
					if ( !m_ResvPath.IsEmpty() )
						m_ResvPath.RemoveAll();
					m_PathName.Empty();
					m_FileName.Empty();
					m_ResvDoit = FALSE;
				}

			} else {
				if ( (p = _tcsrchr(m_PathName, _T('\\'))) != NULL || (p = _tcsrchr(m_PathName, _T(':'))) != NULL )
					m_FileName = m_pDoc->RemoteStr(p + 1);
				else
					m_FileName = m_pDoc->RemoteStr(m_PathName);

				m_ResvDoit = TRUE;
			}

			if ( m_ResvPath.IsEmpty() )
				m_MultiFile = FALSE;

		} else {
			m_MultiFile = FALSE;
			m_PathName = m_pDoc->LocalStr(m_FileName);
			if ( (p = _tcsrchr(m_PathName, _T('.'))) == NULL )
				p = _T(".");

			CExtFileDialog dlg(((m_Param & CHKFILENAME_OPEN) ? TRUE : FALSE), p + 1, m_PathName,
				OFN_HIDEREADONLY | OFN_ENABLESIZING | ((m_Param & CHKFILENAME_MULTI) ? OFN_ALLOWMULTISELECT : 0), 
				CStringLoad(IDS_FILEDLGALLFILE), m_pWnd, 0, (m_ExtFileDlgMode == 0 ? TRUE : FALSE), m_ExtFileDlgMode, this);

			if ( DpiAwareDoModal(dlg, m_ExtFileDlgMode != 0 ? REQDPICONTEXT_SCALED : REQDPICONTEXT_AWAREV2) == IDOK ) {
				if ( (m_Param & CHKFILENAME_MULTI) != 0 ) {
					POSITION pos = dlg.GetStartPosition();
					while ( pos != NULL )
						m_ResvPath.AddTail(dlg.GetNextPathName(pos));

					if ( !m_ResvPath.IsEmpty() ) {
						m_MultiFile = TRUE;
						goto RECHECK;
					}

					m_PathName.Empty();
					m_FileName.Empty();

				} else {
					m_PathName = dlg.GetPathName();
					m_FileName = m_pDoc->RemoteStr(dlg.GetFileName());
				}

			} else {
				m_PathName.Empty();
				m_FileName.Empty();
			}
		}
		m_pParamEvent->SetEvent();
		break;
	case THCMD_YESNO:
		work = m_pDoc->LocalStr(m_Message);
		if ( m_pWnd->MessageBox(work, _T("Question"), MB_ICONQUESTION | MB_YESNO) == IDYES )
			m_Param = 'Y';
		m_pParamEvent->SetEvent();
		break;
	case THCMD_XONXOFF:
		if ( m_pDoc->m_pSock != NULL )
			m_pDoc->m_pSock->SetXonXoff(m_Param);
		m_pParamEvent->SetEvent();
		break;
	case THCMD_SENDSTR:
		m_SendSema.Lock();
		m_pDoc->SendBuffer(m_SendBuf);
		m_SendBuf.Clear();
		m_SendSema.Unlock();
		m_pParamEvent->SetEvent();
		break;
	case THCMD_SENDSCRIPT:
		m_SendSema.Lock();
		m_pDoc->SendScript((LPCWSTR)m_SendBuf.GetPtr(), NULL);
		m_SendBuf.Clear();
		m_SendSema.Unlock();
		m_pParamEvent->SetEvent();
		break;
	case THCMD_SENDSYNC:
		m_SendSema.Lock();
		if ( m_pDoc->m_pSock != NULL ) {
			m_pDoc->m_pSock->m_pSyncEvent = m_pParamEvent;
			m_pDoc->m_pSock->Send(m_SendBuf.GetPtr(), m_SendBuf.GetSize(), 0);
		}
		m_SendBuf.Clear();
		m_SendSema.Unlock();
		//m_pParamEvent->SetEvent();
		break;
	case TGCMD_MESSAGE:
		m_pWnd->MessageBox(MbsToTstr(m_Message));
		m_pParamEvent->SetEvent();
		break;
	case TGCMD_NOWAITMESSAGE:
		m_pWnd->MessageBox(MbsToTstr(m_Message));
		break;
	case THCMD_ECHOBUFFER:
		m_SendSema.Lock();
		m_SwapBuf.Swap(m_EchoBuf);
		m_EchoBuf.Clear();
		m_EchoPostReq = FALSE;
		m_SendSema.Unlock();
		while ( m_SwapBuf.GetSize() > 0 ) {
			n = m_pDoc->OnSocketReceive(m_SwapBuf.GetPtr(), m_SwapBuf.GetSize(), 0);
			m_SwapBuf.Consume(n);
		}
		break;
	}
}

void CSyncSock::OnProc(int cmd)
{
}

void CSyncSock::DoProc(int cmd)
{
	if ( m_ThreadFlag )
		return;
	m_DoAbortFlag = FALSE;
	m_ProgDlg.m_AbortFlag = FALSE;
	m_EchoPostReq = FALSE;
	m_bInitDone = FALSE;
	m_ThreadMode = cmd;
	m_pDoc->m_pSock->SetRecvSyncMode(TRUE);
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_START, (LPARAM)this);
}
void CSyncSock::DoAbort()
{
	if ( !m_ThreadFlag )
		return;
	m_DoAbortFlag = TRUE;
	m_ProgDlg.m_AbortFlag = TRUE;
	m_pParamEvent->SetEvent();
	if ( m_pDoc != NULL && m_pDoc->m_pSock != NULL )
		m_pDoc->m_pSock->SyncAbort();
	WaitForSingleObject(m_pThreadEvent->m_hObject, INFINITE);
	if ( m_pDoc != NULL && m_pDoc->m_pSock != NULL )
		m_pDoc->m_pSock->SetRecvSyncMode(FALSE);
	if ( m_pDoc != NULL )
		m_pDoc->SetSleepReq(SLEEPSTAT_ENABLE);
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
	m_SendSema.Lock();
	m_SendBuf.Put8Bit(c);
	m_SendSema.Unlock();
}
void CSyncSock::Bufferd_SendBuf(char *buf, int len)
{
	m_SendSema.Lock();
	m_SendBuf.Apend((LPBYTE)buf, len);
	m_SendSema.Unlock();
}
void CSyncSock::Bufferd_Flush()
{
	ASSERT(m_pWnd != NULL);

	DebugMsg("Bufferd_Flush %d", m_SendBuf.GetSize());
	//DebugDump(m_SendBuf.GetPtr(), m_SendBuf.GetSize() < 16 ? m_SendBuf.GetSize() : 16);

	m_pWnd->PostMessage(WM_THREADCMD, THCMD_SENDBUF, (LPARAM)this);
}
void CSyncSock::Bufferd_Clear()
{
	m_SendSema.Lock();
	m_SendBuf.Clear();
	m_SendSema.Unlock();

	if ( m_bInitDone && m_pDoc->m_pSock != NULL ) {
		m_RecvBuf.Clear();

		BOOL f = FALSE;
		BYTE tmp[256];
		while ( m_pDoc->m_pSock->SyncReceive(tmp, 256, 1000, &f) > 0 );
	}
}
void CSyncSock::Bufferd_Sync()
{
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_SENDSYNC, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
}
int CSyncSock::Bufferd_Receive(int sec, int msec)
{
	int n;
	BYTE tmp[1024];

	if ( m_pDoc->m_pSock == NULL || m_DoAbortFlag )
		return (-1);	// ERROR

	if ( m_RecvBuf.GetSize() <= 0 ) {
		if ( (n = m_pDoc->m_pSock->SyncReceive(tmp, 1024, sec * 1000 + msec, &m_ProgDlg.m_AbortFlag)) <= 0 )
			return (-2);	// TIME OUT
		m_RecvBuf.Apend(tmp, n);

		DebugMsg("Bufferd_Receive %d", n);
		//DebugDump(tmp, n < 16 ? n : 16);
	}

	return m_RecvBuf.Get8Bit();
}
BOOL CSyncSock::Bufferd_ReceiveBuf(char *buf, int len, int sec, int msec)
{
	int n;

	if ( m_pDoc->m_pSock == NULL || m_DoAbortFlag )
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
		if ( (n = m_pDoc->m_pSock->SyncReceive(buf, len, sec * 1000 + msec, &m_ProgDlg.m_AbortFlag)) <= 0 )
			return FALSE;

		DebugMsg("Bufferd_Receive %d", n);
		//DebugDump((LPBYTE)buf, n < 16 ? n : 16);

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
	if ( m_pDoc->m_pSock == NULL || m_DoAbortFlag )
		return 0;

	return m_pDoc->m_pSock->GetRecvProcSize() + m_RecvBuf.GetSize();
}
void CSyncSock::SetXonXoff(int sw)
{
	m_Param = sw;
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_XONXOFF, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
}

//////////////////////////////////////////////////////////////////////

BOOL CSyncSock::CheckFileName(int mode, LPCSTR file, int extmode)
{
	if ( m_DoAbortFlag )
		return FALSE;
	m_Param = mode;
	m_FileName = file;
	m_ExtFileDlgMode = extmode;
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_CHECKPATH, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
	return (m_FileName.IsEmpty() || m_PathName.IsEmpty() ? FALSE : TRUE);
}
int CSyncSock::YesOrNo(LPCSTR msg)
{
	if ( m_DoAbortFlag )
		return 'N';
	m_Param = 'N';
	m_Message = msg;
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_YESNO, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
	return m_Param;
}
int CSyncSock::AbortCheck()
{
	if ( m_ProgDlg.m_hWnd == NULL )
		return FALSE;
	return m_ProgDlg.m_AbortFlag;
}
void CSyncSock::SendEcho(int ch)
{
	BOOL bPost = FALSE;

	m_SendSema.Lock();
	m_EchoBuf.Put8Bit(ch);
	if ( !m_EchoPostReq )
		m_EchoPostReq = bPost = TRUE;
	m_SendSema.Unlock();

	if ( bPost )
		m_pWnd->PostMessage(WM_THREADCMD, THCMD_ECHOBUFFER, (LPARAM)this);
}
void CSyncSock::SendEchoBuffer(char *buf, int len)
{
	BOOL bPost = FALSE;

	m_SendSema.Lock();
	m_EchoBuf.Apend((LPBYTE)buf, len);
	if ( !m_EchoPostReq )
		m_EchoPostReq = bPost = TRUE;
	m_SendSema.Unlock();

	if ( bPost )
		m_pWnd->PostMessage(WM_THREADCMD, THCMD_ECHOBUFFER, (LPARAM)this);
}
void CSyncSock::UpDownOpen(LPCSTR msg)
{
	m_Message = msg;
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_DLGOPEN, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
}
void CSyncSock::UpDownClose()
{
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_DLGCLOSE, (LPARAM)this);
}
void CSyncSock::UpDownMessage(LPCSTR msg)
{
	m_Message = msg;
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_DLGMESSAGE, (LPARAM)this);
}
void CSyncSock::UpDownInit(LONGLONG size, LONGLONG rems)
{
	if ( m_DoAbortFlag )
		return;
	m_Size = size;
	m_RemSize = rems;
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_DLGRANGE, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
}
void CSyncSock::UpDownStat(LONGLONG size)
{
	BOOL bPost = FALSE;

	m_ProgSema.Lock();
	if ( !m_bUpdateReq )
		m_bUpdateReq = bPost = TRUE;
	m_Size = size;
	m_ProgSema.Unlock();

	if ( bPost )
		m_pWnd->PostMessage(WM_THREADCMD, THCMD_DLGPOS, (LPARAM)this);
}
void CSyncSock::SendString(LPCWSTR str)
{
	if ( m_DoAbortFlag )
		return;
	m_SendSema.Lock();
	m_SendBuf.Clear();
	m_SendBuf.Apend((LPBYTE)str, (int)wcslen(str) * sizeof(WCHAR));
	m_SendSema.Unlock();
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_SENDSTR, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
}
void CSyncSock::SendScript(LPCWSTR str)
{
	if ( m_DoAbortFlag )
		return;
	m_SendSema.Lock();
	m_SendBuf.Clear();
	m_SendBuf.Apend((LPBYTE)str, (int)wcslen(str) * sizeof(WCHAR));
	m_SendSema.Unlock();
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, THCMD_SENDSCRIPT, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
}
void CSyncSock::Message(LPCSTR msg)
{
	m_Message = msg;
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, TGCMD_MESSAGE, (LPARAM)this);
	WaitForSingleObject(m_pParamEvent->m_hObject, INFINITE);
}
void CSyncSock::NoWaitMessage(LPCSTR msg)
{
	m_Message = msg;
	m_pParamEvent->ResetEvent();
	m_pWnd->PostMessage(WM_THREADCMD, TGCMD_NOWAITMESSAGE, (LPARAM)this);
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
