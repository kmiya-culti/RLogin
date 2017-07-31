// SFtp.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "Afxpriv.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "Ssh.h"
#include "SFtp.h"
#include "UpdateDlg.h"
#include "EditDlg.h"
#include "ChModDlg.h"
#include "AutoRenDlg.h"
#include "Data.h"

#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <shlobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static	int CALLBACK	ListCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CSFtp *pWnd = (CSFtp *)lParamSort;
	return pWnd->CompareNode((int)lParam1, (int)lParam2);
}

/////////////////////////////////////////////////////////////////////////////
// CFileNode
/////////////////////////////////////////////////////////////////////////////

CFileNode::CFileNode()
{
	m_flags = 0;
	m_size  = 0;
	m_uid   = 0;
	m_gid   = 0;
	m_attr  = 0;
	m_atime = 0;
	m_mtime = 0;
	m_icon  = 1;
	m_file  = _T("");
	m_sub   = _T("");
	m_path  = _T("");
	m_name  = _T("");
	m_pLeft = NULL;
	m_pRight = NULL;
	m_link  = FALSE;
}
CFileNode::~CFileNode()
{
	if ( m_pLeft != NULL )
		delete m_pLeft;
	if ( m_pRight != NULL )
		delete m_pRight;
}
void CFileNode::DecodeAttr(CBuffer *bp)
{
	int n, max;
	CBuffer tmp;

	m_flags = bp->Get32Bit();

	if ( m_flags & SSH2_FILEXFER_ATTR_SIZE )
		m_size = bp->Get64Bit();		// size

	if ( m_flags & SSH2_FILEXFER_ATTR_UIDGID ) {
		m_uid = bp->Get32Bit();			// uid
		m_gid = bp->Get32Bit();			// gid
	}

	if ( m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS )
		m_attr = bp->Get32Bit();		// permissions

	if ( m_flags & SSH2_FILEXFER_ATTR_ACMODTIME ) {
		m_atime = bp->Get32Bit();		// atime
		m_mtime = bp->Get32Bit();		// mtime
	}

	if ( m_flags & SSH2_FILEXFER_ATTR_EXTENDED ) {
		max = bp->Get32Bit();
		for ( n = 0 ; n < max ; n++ ) {
			bp->GetBuf(&tmp);		// type
			bp->GetBuf(&tmp);		// data
		}
	}
}
void CFileNode::EncodeAttr(CBuffer *bp)
{
	m_flags &= ~SSH2_FILEXFER_ATTR_EXTENDED;

	bp->Put32Bit(m_flags);

	if ( m_flags & SSH2_FILEXFER_ATTR_SIZE )
		bp->Put64Bit(m_size);

	if ( m_flags & SSH2_FILEXFER_ATTR_UIDGID ) {
		bp->Put32Bit(m_uid);
		bp->Put32Bit(m_gid);
	}

	if ( m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS )
		bp->Put32Bit(m_attr);

	if ( m_flags & SSH2_FILEXFER_ATTR_ACMODTIME ) {
		bp->Put32Bit((int)m_atime);
		bp->Put32Bit((int)m_mtime);
	}

}
const CFileNode & CFileNode::operator = (CFileNode &data)
{
	m_flags = data.m_flags;
	m_size  = data.m_size;
	m_uid   = data.m_uid;
	m_gid   = data.m_gid;
	m_attr  = data.m_attr;
	m_atime = data.m_atime;
	m_mtime = data.m_mtime;
	m_icon  = data.m_icon;
	m_file  = data.m_file;
	m_sub   = data.m_sub;
	m_path  = data.m_path;
	m_name  = data.m_name;
	m_link  = data.m_link;
	return *this;
}
int CFileNode::SetFileStat(LPCTSTR file)
{
	struct _utimbuf utm;

	utm.actime  = m_atime;
	utm.modtime = m_mtime;

	if ( _tutime(file, &utm) )
		return FALSE;

	if ( _tchmod(file, m_attr) )
		return FALSE;

	return TRUE;
}
int CFileNode::GetFileStat(LPCTSTR path, LPCTSTR file)
{
	struct _stati64 st;

	if ( _tstati64(path, &st) )
		return FALSE;

	m_flags = SSH2_FILEXFER_ATTR_SIZE | SSH2_FILEXFER_ATTR_PERMISSIONS | SSH2_FILEXFER_ATTR_ACMODTIME;
	m_size  = st.st_size;
	m_uid   = st.st_uid;
	m_gid   = st.st_gid;
	m_attr  = st.st_mode;
	m_atime = st.st_atime;
	m_mtime = st.st_mtime;
	m_file  = file;
	m_path  = path;
	m_name  = _T("");

	SetSubName();

	return TRUE;
}
LPCTSTR CFileNode::GetFileSize()
{
	if ( (m_flags & SSH2_FILEXFER_ATTR_SIZE) == 0 )
		return _T("");
	if ( (m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS) != 0 && (m_attr & _S_IFMT) == _S_IFDIR )
		return _T("DIR");
	m_name.Format(_T("%I64d"), m_size);
	return m_name;
}
LPCTSTR CFileNode::GetUserID()
{
	if ( (m_flags & SSH2_FILEXFER_ATTR_UIDGID) == 0 )
		return _T("");
	m_name.Format(_T("%d"), m_uid);
	return m_name;
}
LPCTSTR CFileNode::GetGroupID()
{
	if ( (m_flags & SSH2_FILEXFER_ATTR_UIDGID) == 0 )
		return _T("");
	m_name.Format(_T("%d"), m_gid);
	return m_name;
}
LPCTSTR CFileNode::GetAttr()
{
	if ( (m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS) == 0 )
		return _T("");

	m_name = ((m_link || (m_attr & _S_IFMT) == _S_IFLNK) ? _T("l") : ((m_attr & _S_IFMT) == _S_IFDIR ? _T("d") : _T("-")));
	for ( int b = 0400 ; b != 0 ; ) {
		m_name += ((m_attr & b) != 0 ? _T('r') : _T('-')); b >>= 1;
		m_name += ((m_attr & b) != 0 ? _T('w') : _T('-')); b >>= 1;
		m_name += ((m_attr & b) != 0 ? _T('x') : _T('-')); b >>= 1;
	}

	return m_name;
}
LPCTSTR CFileNode::GetAcsTime()
{
	if ( (m_flags & SSH2_FILEXFER_ATTR_ACMODTIME) == 0 )
		return _T("");
	CTime t(m_atime);
	m_name = t.Format(_T("%y/%m/%d %H:%M:%S"));
	return m_name;
}
LPCTSTR CFileNode::GetModTime()
{
	if ( (m_flags & SSH2_FILEXFER_ATTR_ACMODTIME) == 0 )
		return _T("");
	CTime t(m_mtime);
	m_name = t.Format(_T("%y/%m/%d %H:%M:%S"));
	return m_name;
}
void CFileNode::AutoRename(LPCTSTR p, CString &tmp, int mode)
{
	int n;
	CString work;
	LPCTSTR s;
	LPCTSTR ext = (mode == 0 ? _T("%00%") : _T("!"));

	static LPCTSTR renStr = _T("\\<>:\"/?*%");
	static LPCTSTR renTab[] = { _T("￥"), _T("＜"), _T("＞"), _T("："), _T("”"), _T("／"), _T("？"), _T("＊"), _T("%") };
	static LPCTSTR badTab[] = {
		_T("AUX"),		_T("CLOCK$"),	_T("COM1"),		_T("COM2"),		_T("COM3"),		// 5
		_T("COM4"),		_T("COM5"),		_T("COM6"),		_T("COM7"),		_T("COM8"),		// 10
		_T("COM9"),		_T("CON"),		_T("CONFIG$"),	_T("LPT1"),		_T("LPT2"),		// 15
		_T("LPT3"),		_T("LPT4"),		_T("LPT5"),		_T("LPT6"),		_T("LPT7"),		// 20
		_T("LPT8"),		_T("LPT9"),		_T("NUL"),		_T("PRN")						// 24
	};

	for ( ; *p != _T('\0') ; p++ ) {
#ifndef	_UNICODE
		if ( issjis1(p[0]) && issjis2(p[1]) ) {
			tmp += *(p++);
			tmp += *p;
		} else
#endif
		if ( (s = _tcschr(renStr, *p)) != NULL || *p < _T(' ') ) {
			if ( s != NULL && mode != 0 ) {
				tmp += renTab[(int)(s - renStr) / sizeof(TCHAR)];
			} else {
				work.Format(_T("%%%02x%%"), *p);
				tmp += work;
			}
		} else
			tmp += *p;
	}

	if ( (n = tmp.GetLength()) == 0 )
		return;

	if ( tmp[n - 1] == _T(' ') || tmp[n - 1] == _T('.') || BinaryFind((void *)(LPCTSTR)tmp, badTab, sizeof(char *), 24, (int (*)(const void *, const void *))_tcsicmp, NULL) )
		tmp += ext;
}
LPCTSTR CFileNode::GetLocalPath(LPCTSTR dir, class CSFtp *pWnd)
{
	m_name = dir;
	if ( m_name.Right(1).Compare(_T("\\")) != 0 )
		m_name += _T("\\");

	CString tmp[4];
	struct _stati64 st;

	AutoRename(m_file, tmp[0], 0);
	AutoRename(m_file, tmp[1], 1);

	if ( m_file.Compare(tmp[0]) != 0 ) {
		tmp[2] = m_name + tmp[0];
		tmp[3] = m_name + tmp[1];

		if ( !_tstati64(tmp[2], &st) )
			m_name += tmp[0];
		else if ( !_tstati64(tmp[3], &st) )
			m_name += tmp[1];
		else {
			CAutoRenDlg dlg(pWnd);

			dlg.m_Exec = pWnd->m_AutoRenMode & 7;
			dlg.m_Name[0] = tmp[0];
			dlg.m_Name[1] = tmp[1];
			dlg.m_Name[2] = m_file;
			dlg.m_NameOK  = _T("×");

			if ( (pWnd->m_AutoRenMode & 0x80) == 0 || pWnd->m_AutoRenMode == 0x82 ) {
				if ( dlg.DoModal() != IDOK )
					return NULL;
				pWnd->m_AutoRenMode = dlg.m_Exec;
			}

			switch(pWnd->m_AutoRenMode & 7) {
			case 0: m_name += tmp[0]; break;
			case 1: m_name += tmp[1]; break;
			case 2: m_name += dlg.m_Name[2]; break;
			case 3: return NULL;
			}
		}
	} else
		m_name += m_file;

	return m_name;
}

#define	htoc(c)		(isdigit(c) ? ((c) - '0') : ((c) >= 'a' ? ((c) - 'a' + 10) : ((c) - 'A') + 10))

LPCTSTR CFileNode::GetRemotePath(LPCTSTR dir, class CSFtp *pWnd)
{
	m_name = dir;
	if ( m_name.Right(1).Compare(_T("/")) != 0 )
		m_name += _T("/");

	LPCTSTR p = m_file;
	CString tmp;

	for ( ; *p != _T('\0') ; p++ ) {
#ifndef	_UNICODE
		if ( issjis1(p[0]) && issjis2(p[1]) ) {
			tmp += *(p++);
			tmp += *p;
		} else
#endif
		if ( p[0] == _T('%') && isxdigit(p[1]) && isxdigit(p[2]) && p[3] == _T('%') ) {
			tmp += (TCHAR)(htoc(p[1]) * 16 + htoc(p[2]));
			p += 3;
		} else
			tmp += *p;
	}

	if ( tmp.Compare(m_file) != 0 ) {
		if ( pWnd->m_AutoRenMode == 0x80 )
			m_name += tmp;
		else if ( (pWnd->m_AutoRenMode & 0x80) != 0 )
			m_name += m_file;
		else {
			if ( pWnd->MessageBox(CStringLoad(IDE_FILENAMEERROR), _T("QUESTION"), MB_YESNO) == IDYES ) {
				pWnd->m_AutoRenMode = 0x80;
				m_name += tmp;
			} else
				m_name += m_file;
		}
	} else
		m_name += m_file;

	return m_name;
}

static const struct _SubTab {
		LPCTSTR	name;
		int		icon;
	} tab[] = {
		{ _T("avi"),	5 },{ _T("bat"),	6 },{ _T("bmp"),	4 },{ _T("c"),  	2 },{ _T("c++"),	2 },	// 5
		{ _T("com"),	6 },{ _T("cpp"),	2 },{ _T("cs"), 	2 },{ _T("doc"),	2 },{ _T("exe"),	6 },	// 10
		{ _T("gif"),	4 },{ _T("h"),  	2 },{ _T("htm"),	3 },{ _T("html"),	3 },{ _T("ico"),	4 },	// 15
		{ _T("jpeg"),	4 },{ _T("jpg"),	4 },{ _T("log"),	2 },{ _T("mov"),	5 },{ _T("mp3"),	5 },	// 20
		{ _T("mpeg"),	5 },{ _T("mpg"),	5 },{ _T("php"),	3 },{ _T("txt"),	2 },{ _T("wav"),	5 },	// 25
		{ _T("wma"),	5 },{ _T("wmv"),	5 },{ _T("xml"),	3 }												// 28
	};

static int SubNameCmp(const void *src, const void *dis)
{
	return ((CString *)src)->CompareNoCase(((struct _SubTab *)dis)->name);
}
void CFileNode::SetSubName()
{
	int n;

	if ( (n = m_file.ReverseFind(_T('.'))) >= 0 )
		m_sub = m_file.Mid(n + 1);

	if ( IsDir() )
		m_icon = 0;
	else if ( BinaryFind((void *)&m_sub, (void *)tab, sizeof(struct _SubTab), 28, SubNameCmp, &n) )
		m_icon = tab[n].icon;
	else
		m_icon = 1;
}
int CFileNode::IsDir()
{
	return ((m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS) != 0 && (m_attr & _S_IFMT) == _S_IFDIR ? TRUE : FALSE);
}
int CFileNode::IsReg()
{
	return ((m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS) != 0 && (m_attr & _S_IFMT) == _S_IFREG ? TRUE : FALSE);
}
int CFileNode::IsLink()
{
	return ((m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS) != 0 && (m_attr & _S_IFMT) == _S_IFLNK ? TRUE : FALSE);
}
class CFileNode *CFileNode::Add(class CFileNode *pNode)
{
	int c;

	if ( (c = m_path.Compare(pNode->m_path)) == 0 )
		return this;
	else if ( c < 0 ) {
		if ( m_pLeft != NULL )
			return m_pLeft->Add(pNode);
		m_pLeft = new CFileNode;
		*m_pLeft = *pNode;
		return m_pLeft;
	} else {
		if ( m_pRight != NULL )
			return m_pRight->Add(pNode);
		m_pRight = new CFileNode;
		*m_pRight = *pNode;
		return m_pRight;
	}
}
class CFileNode *CFileNode::Find(LPCTSTR path)
{
	int c;

	if ( (c = m_path.Compare(path)) == 0 )
		return this;
	else if ( c < 0 ) {
		if ( m_pLeft != NULL )
			return m_pLeft->Find(path);
	} else {
		if ( m_pRight != NULL )
			return m_pRight->Find(path);
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CCmdQue
/////////////////////////////////////////////////////////////////////////////

CCmdQue::CCmdQue()
{
	m_Fd = (-1);
}

/////////////////////////////////////////////////////////////////////////////
// CSFtp
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CSFtp, CDialogExt)

CSFtp::CSFtp(CWnd* pParent /*=NULL*/)
	: CDialogExt(CSFtp::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSFtp)
	//}}AFX_DATA_INIT
	m_pSSh    = NULL;
	m_pChan   = NULL;
	m_VerId   = (-1);
	m_SeqId   = 0;
	m_LocalSortItem  = 0;
	m_RemoteSortItem = 0;
	m_HostKanjiSet   = _T("SJIS");
	m_UpDownCount = 0;
	m_bDragList = FALSE;
	m_DoAbort = FALSE;
	m_DoExec = 0;
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ACTIVE);
	m_TotalSize = 0;
	m_pCacheNode = NULL;
	m_UpdateCheckMode = 0;
	m_AutoRenMode = 0;
}
CSFtp::~CSFtp()
{
	CCmdQue *pQue;

	m_RemoteNode.RemoveAll();
	m_LocalNode.RemoveAll();

	while ( !m_CmdQue.IsEmpty() ) {
		pQue = m_CmdQue.RemoveHead();
		delete pQue;
	}
	while ( !m_WaitQue.IsEmpty() ) {
		pQue = m_WaitQue.RemoveHead();
		delete pQue;
	}
	if ( m_pCacheNode != NULL )
		delete m_pCacheNode;
}
void CSFtp::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REMOTE_LIST, m_RemoteList);
	DDX_Control(pDX, IDC_REMOTE_CWD, m_RemoteCwd);
	DDX_Control(pDX, IDC_LOCAL_LIST, m_LocalList);
	DDX_Control(pDX, IDC_LOCAL_CWD, m_LocalCwd);
	DDX_Control(pDX, IDC_PROGRESS1, m_UpDownProg);
	DDX_Control(pDX, IDC_STATUS1, m_UpDownStat[0]);
	DDX_Control(pDX, IDC_STATUS2, m_UpDownStat[1]);
	DDX_Control(pDX, IDC_STATUS3, m_UpDownStat[2]);
	DDX_Control(pDX, IDC_STATUS4, m_UpDownStat[3]);
}

/////////////////////////////////////////////////////////////////////////////

void CSFtp::Send(LPBYTE buf, int len)
{
	m_pChan->m_Output.Apend(buf, len);
	m_pChan->m_pSsh->SendMsgChannelData(m_pChan->m_LocalID);
}
int CSFtp::OnRecive(const void *lpBuf, int nBufLen)
{
	m_RecvBuf.Apend((LPBYTE)lpBuf, nBufLen);

	if ( (m_DoExec & 001) != 0 ) {
		m_DoExec |= 002;
		return nBufLen;
	}

	int n;
	int len = 0;
	CBuffer *buf = NULL;

	while ( m_RecvBuf.GetSize() >= 4 ) {
		n = m_RecvBuf.PTR32BIT(m_RecvBuf.GetPtr());
		if ( n > (256 * 1024) || n < 0 )
			throw "sftp packet length error";
		if ( m_RecvBuf.GetSize() < (n + 4) )
			break;

		if ( buf == NULL )
			buf = new CBuffer[10];
		buf[len++].Apend(m_RecvBuf.GetPtr() + 4, n);
		m_RecvBuf.Consume(n + 4);

		if ( len >= 10 ) {
			PostMessage(WM_RECIVEBUFFER, len, (LPARAM)buf);
			buf = NULL;
			len = 0;
		}
	}

	if ( buf != NULL )
		PostMessage(WM_RECIVEBUFFER, len, (LPARAM)buf);

	return nBufLen;
}
void CSFtp::OnConnect()
{
	CBuffer tmp;
	tmp.Put32Bit(0);
	tmp.Put8Bit(SSH2_FXP_INIT);
	tmp.Put32Bit(SSH2_FILEXFER_VERSION);
	SendBuffer(&tmp);
}
void CSFtp::Close()
{
	ASSERT(m_pChan != NULL);
	m_pChan->DoClose();
}

/////////////////////////////////////////////////////////////////////////////

void CSFtp::SendBuffer(CBuffer *bp)
{
	bp->SET32BIT(bp->GetPtr(), bp->GetSize() - 4);
	Send(bp->GetPtr(), bp->GetSize());
}
int CSFtp::ReciveBuffer(CBuffer *bp)
{
	CCmdQue *pQue;
	int type = bp->Get8Bit();
	int id   = bp->Get32Bit();

	if ( type == SSH2_FXP_VERSION ) {
		if ( (m_VerId = id) != SSH2_FILEXFER_VERSION ) {
			MessageBox(_T("SFTP Version Error"));
			Close();
			return FALSE;
		}
	} else {
		POSITION pos = m_CmdQue.GetHeadPosition();
		while ( pos != NULL ) {
			pQue = m_CmdQue.GetAt(pos);
			if ( pQue->m_ExtId == id ) {
				m_CmdQue.RemoveAt(pos);
				if ( (this->*(pQue->m_Func))(type, bp, pQue) )
					delete pQue;
				break;
			}
			m_CmdQue.GetNext(pos);
		}
		if ( pos == NULL )
			MessageBox(_T("Unkown sftp Message Recived"));
	}

	SendWaitQue();
	return TRUE;
}
void CSFtp::SendCommand(CCmdQue *pQue, int (CSFtp::*pFunc)(int type, CBuffer *bp, class CCmdQue *pQue), int mode)
{
	pQue->m_Func  = pFunc;
	pQue->m_ExtId = m_SeqId++;
	pQue->m_Msg.SET32BIT(pQue->m_Msg.GetPos(5), pQue->m_ExtId);

	if ( mode == SENDCMD_NOWAIT && m_VerId != SSH2_FILEXFER_VERSION )
		mode = SENDCMD_HEAD;

	switch(mode) {
	case SENDCMD_NOWAIT:
		SendBuffer(&(pQue->m_Msg));
		pQue->m_SendTime = CTime::GetCurrentTime();
		m_CmdQue.AddTail(pQue);
		break;
	case SENDCMD_HEAD:
		m_WaitQue.AddHead(pQue);
		break;
	case SENDCMD_TAIL:
		m_WaitQue.AddTail(pQue);
		break;
	}
}
void CSFtp::SendWaitQue()
{
	if ( m_VerId != SSH2_FILEXFER_VERSION || !m_CmdQue.IsEmpty() || m_WaitQue.IsEmpty() )
		return;

	CCmdQue *pQue = m_WaitQue.RemoveHead();
	SendBuffer(&(pQue->m_Msg));
	pQue->m_SendTime = CTime::GetCurrentTime();
	m_CmdQue.AddTail(pQue);
}
void CSFtp::RemoveWaitQue()
{
	CCmdQue *pQue;
	while ( !m_WaitQue.IsEmpty() ) {
		pQue = m_WaitQue.RemoveHead();
		delete pQue;
	}
}

/////////////////////////////////////////////////////////////////////////////

int CSFtp::RemoteMakePacket(class CCmdQue *pQue, int Type)
{
	CStringA tmp;

	pQue->m_Msg.Clear();
	pQue->m_Msg.Put32Bit(0);		// Packet Size
	pQue->m_Msg.Put8Bit(Type);
	pQue->m_Msg.Put32Bit(0);		// Packet ID

	switch(Type) {
	case SSH2_FXP_OPEN:
	case SSH2_FXP_STAT:
	case SSH2_FXP_SETSTAT:
	case SSH2_FXP_MKDIR:
	case SSH2_FXP_OPENDIR:
	case SSH2_FXP_REALPATH:
	case SSH2_FXP_REMOVE:
	case SSH2_FXP_RMDIR:
		KanjiConvToRemote(pQue->m_Path, tmp);
		pQue->m_Msg.PutStr(tmp);
		break;

	case SSH2_FXP_RENAME:
		KanjiConvToRemote(pQue->m_Path, tmp);
		pQue->m_Msg.PutStr(tmp);
		KanjiConvToRemote(pQue->m_CopyPath, tmp);
		pQue->m_Msg.PutStr(tmp);
		break;

	case SSH2_FXP_CLOSE:
	case SSH2_FXP_READDIR:
	case SSH2_FXP_FSETSTAT:
		pQue->m_Msg.PutBuf(pQue->m_Handle.GetPtr(), pQue->m_Handle.GetSize());
		break;

	case SSH2_FXP_READ:
	case SSH2_FXP_WRITE:
		pQue->m_Msg.PutBuf(pQue->m_Handle.GetPtr(), pQue->m_Handle.GetSize());
		pQue->m_Msg.Put64Bit(pQue->m_Offset);
		pQue->m_Msg.Put32Bit(pQue->m_Len);
		break;
	}

	return FALSE;
}
int CSFtp::RemoteLinkStatRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	CFileNode node;

	if ( type != SSH2_FXP_ATTRS ) {
//		DispErrMsg("Get Stat Attr Error", pQue->m_Path);
		return TRUE;
	}
	node.DecodeAttr(bp);

	if ( node.IsDir() || node.IsReg() ) {
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_size  = node.m_size;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_uid   = node.m_uid;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_gid   = node.m_gid;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_attr  = node.m_attr;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_atime = node.m_atime;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_mtime = node.m_mtime;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_flags = node.m_flags;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].m_link  = TRUE;
		pQue->m_pOwner->m_FileNode[pQue->m_Max].SetSubName();
	}

	return TRUE;
}
void CSFtp::RemoteLinkStat(LPCTSTR path, CCmdQue *pOwner, int index)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path   = path;
	pQue->m_Max    = index;
	pQue->m_pOwner = pOwner;

	RemoteMakePacket(pQue, SSH2_FXP_STAT);
	SendCommand(pQue, &CSFtp::RemoteLinkStatRes, SENDCMD_NOWAIT);
}
int CSFtp::RemoteMakeDirRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK )
		DispErrMsg(_T("Create Dir Error"), pQue->m_Path);
	else if ( pQue->m_Max )
		RemoteUpdateCwd(pQue->m_Path);
	return TRUE;
}
void CSFtp::RemoteMakeDir(LPCTSTR dir, int show)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = dir;
	pQue->m_Max  = show;

	RemoteMakePacket(pQue, SSH2_FXP_MKDIR);
	pQue->m_Msg.Put32Bit(SSH2_FILEXFER_ATTR_PERMISSIONS);
	pQue->m_Msg.Put32Bit(0755);
	SendCommand(pQue, &CSFtp::RemoteMakeDirRes, SENDCMD_TAIL);
}
int CSFtp::RemoteDeleteRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK )
		DispErrMsg(_T("Delete File/Dir Error"), pQue->m_Path);
	else if ( pQue->m_Max )
		RemoteUpdateCwd(pQue->m_Path);
	return TRUE;
}
void CSFtp::RemoteDelete(CFileNode *pNode, int show)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = pNode->m_path;
	pQue->m_Max  = show;

	if ( pNode->IsLink() || pNode->IsReg() || pNode->m_link ) {
		RemoteMakePacket(pQue, SSH2_FXP_REMOVE);
		SendCommand(pQue, &CSFtp::RemoteDeleteRes, SENDCMD_HEAD);
	} else if ( pNode->IsDir() ) {
		RemoteMakePacket(pQue, SSH2_FXP_RMDIR);
		SendCommand(pQue, &CSFtp::RemoteDeleteRes, SENDCMD_HEAD);
		RemoteDeleteDir(pQue->m_Path);
	}
}
int CSFtp::RemoteSetAttrRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK )
		DispErrMsg(_T("Set Attr Error"), pQue->m_Path);
	else if ( pQue->m_Max )
		RemoteUpdateCwd(pQue->m_Path);
	return TRUE;
}
void CSFtp::RemoteSetAttr(CFileNode *pNode, int attr, int show)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = pNode->m_path;
	pQue->m_Max  = show;

	RemoteMakePacket(pQue, SSH2_FXP_SETSTAT);
	pQue->m_Msg.Put32Bit(SSH2_FILEXFER_ATTR_PERMISSIONS);
	pQue->m_Msg.Put32Bit(attr);
	SendCommand(pQue, &CSFtp::RemoteSetAttrRes, SENDCMD_TAIL);
}
int CSFtp::RemoteRenameRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK )
		DispErrMsg(_T("Reanme Error"), pQue->m_Path);
	else if ( pQue->m_Max )
		RemoteUpdateCwd(pQue->m_Path);
	return TRUE;
}
void CSFtp::RemoteRename(CFileNode *pNode, LPCTSTR newname, int show)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = pNode->m_path;
	pQue->m_CopyPath = newname;
	pQue->m_Max  = show;

	RemoteMakePacket(pQue, SSH2_FXP_RENAME);
	SendCommand(pQue, &CSFtp::RemoteRenameRes, SENDCMD_TAIL);
}

/////////////////////////////////////////////////////////////////////////////

CFileNode *CSFtp::RemoteCacheAdd(CFileNode *pNode)
{
	if ( m_pCacheNode != NULL )
		return 	m_pCacheNode->Add(pNode);
	m_pCacheNode = new CFileNode;
	*m_pCacheNode = *pNode;
	return m_pCacheNode;
}
CFileNode *CSFtp::RemoteCacheFind(LPCTSTR path)
{
	if ( m_pCacheNode != NULL )
		return 	m_pCacheNode->Find(path);
	return NULL;
}
void CSFtp::RemoteCacheRemoveAll()
{
	if ( m_pCacheNode != NULL )
		delete m_pCacheNode;
	m_pCacheNode = NULL;
}
void CSFtp::RemoteCacheCwd()
{
	for ( int i = 0 ; i < m_RemoteNode.GetSize() ; i++ )
		RemoteCacheAdd(&(m_RemoteNode[i]));
}
int CSFtp::RemoteCacheDirRes(int st, class CCmdQue *pQue)
{
	if ( st != SSH2_FX_OK )
		return TRUE;			// No Error Displey

	for ( int i = 0 ; i < pQue->m_FileNode.GetSize() ; i++ )
		RemoteCacheAdd(&(pQue->m_FileNode[i]));

	return TRUE;
}
int CSFtp::RemoteMirrorUploadRes(int st, class CCmdQue *pQue)
{
	int n, i;

	if ( st != SSH2_FX_OK ) {
		DispErrMsg(_T("Remote Mirror Upload Error"), pQue->m_Path);
		return TRUE;
	}

	for ( n = 0 ; n < pQue->m_FileNode.GetSize() ; n++ ) {
		for ( i = 0 ; i < pQue->m_SaveNode.GetSize() ; i++ ) {
			if ( pQue->m_FileNode[n].m_file.CompareNoCase(pQue->m_SaveNode[i].m_file) == 0 )
				break;
		}
		if ( i >= pQue->m_SaveNode.GetSize() ) {
			if ( (m_DoUpdate & 0x20) == 0 ) {
				m_DoUpdate |= 0x20;
				if ( MessageBox(CStringLoad(IDS_LOCALFILEDELETE), _T("Question"), MB_YESNO | MB_ICONQUESTION) != IDYES ) {
					m_DoUpdate &= ~0x40;
					break;
				}
			}
			RemoteDelete(&(pQue->m_FileNode[n]));
		}
	}

	return TRUE;
}
int CSFtp::RemoteDeleteDirRes(int st, class CCmdQue *pQue)
{
	if ( st != SSH2_FX_OK ) {
		DispErrMsg(_T("Remote Directory Error"), pQue->m_Path);
		return TRUE;
	}

	for ( int n = 0 ; n < pQue->m_FileNode.GetSize() ; n++ )
		RemoteDelete(&(pQue->m_FileNode[n]));

	return TRUE;
}
int CSFtp::RemoteCopyDirRes(int st, class CCmdQue *pQue)
{
	int n, i;
	BOOL DoLoop;
	CString tmp;
    CFileFind Finder;
	CFileNode node;

	if ( st != SSH2_FX_OK ) {
		DispErrMsg(_T("Directory Copy Error"), pQue->m_Path);
		return TRUE;
	}

	if ( !CreateDirectory(pQue->m_CopyPath, NULL) ) {
		if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
			DispErrMsg(_T("Create Directory Error"), pQue->m_CopyPath);
			return TRUE;
		}
	} else
		LocalUpdateCwd(pQue->m_CopyPath);

	for ( n = 0 ; n < pQue->m_FileNode.GetSize() ; n++ ) {
		if ( pQue->m_FileNode[n].IsDir() || pQue->m_FileNode[n].IsReg() )
			DownLoadFile(&(pQue->m_FileNode[n]), pQue->m_FileNode[n].GetLocalPath(pQue->m_CopyPath, this));
	}

	if ( (m_DoUpdate & 0x40) == 0 )
		return TRUE;

	tmp.Format(_T("%s\\*.*"), pQue->m_CopyPath);
	pQue->m_SaveNode.RemoveAll();
	DoLoop = Finder.FindFile(tmp);
	while ( DoLoop != FALSE ) {
		DoLoop = Finder.FindNextFile();
		if ( Finder.IsSystem() || Finder.IsTemporary() || Finder.IsDots() )
			continue;
		if ( node.GetFileStat(Finder.GetFilePath(), Finder.GetFileName()) )
			pQue->m_SaveNode.Add(node);
	}
	Finder.Close();

	for ( n = 0 ; n < pQue->m_SaveNode.GetSize() ; n++ ) {
		for ( i = 0 ; i < pQue->m_FileNode.GetSize() ; i++ ) {
			if ( pQue->m_FileNode[i].m_file.CompareNoCase(pQue->m_SaveNode[n].m_file) == 0 )
				break;
		}
		if ( i >= pQue->m_FileNode.GetSize() ) {
			if ( (m_DoUpdate & 0x20) == 0 ) {
				m_DoUpdate |= 0x20;
				if ( MessageBox(CStringLoad(IDS_REMOTEFILEDELETE), _T("Question"), MB_YESNO | MB_ICONQUESTION) != IDYES ) {
					m_DoUpdate &= ~0x40;
					break;
				}
			}
			LocalDelete(pQue->m_SaveNode[n].m_path);
		}
	}
	return TRUE;
}
int CSFtp::RemoteSetListRes(int st, class CCmdQue *pQue)
{
	int n, i;

	if ( st != SSH2_FX_OK ) {
		DispErrMsg(_T("Set Remote Cwd Error"), pQue->m_Path);
		return TRUE;
	}

	m_RemoteCurDir = pQue->m_Path;
	m_RemoteCwd.SetWindowText(m_RemoteCurDir);
	if ( m_RemoteCwd.FindStringExact(0, m_RemoteCurDir) == CB_ERR )
		m_RemoteCwd.AddString(m_RemoteCurDir);
	SetRemoteCwdHis(m_RemoteCurDir);

	m_RemoteNode.RemoveAll();
	for ( n = 0 ; n < pQue->m_FileNode.GetSize() ; n++ ) {
		if ( pQue->m_FileNode[n].IsDir() || pQue->m_FileNode[n].IsReg() )
			m_RemoteNode.Add(pQue->m_FileNode[n]);
	}

	m_RemoteList.DeleteAllItems();
	m_RemoteList.SetItemCount((int)m_RemoteNode.GetSize());

	for ( n = 0 ; n < m_RemoteNode.GetSize() ; n++ ) {
		i = m_RemoteList.InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, n, m_RemoteNode[n].m_file, 0, 0, m_RemoteNode[n].GetIcon(), n);
		m_RemoteList.SetItemText(i, 1, m_RemoteNode[n].GetModTime());
		m_RemoteList.SetItemText(i, 2, m_RemoteNode[n].GetFileSize());
		m_RemoteList.SetItemText(i, 3, m_RemoteNode[n].GetAttr());
		m_RemoteList.SetItemText(i, 4, m_RemoteNode[n].GetUserID());
		m_RemoteList.SetItemText(i, 5, m_RemoteNode[n].GetGroupID());
	}
	ListSort(1, (-1));

	return TRUE;
}
int CSFtp::RemoteCloseDirRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	SetUpDownCount(-1);

	if ( pQue->m_Len == (-1) || type < 0 )
		return (this->*(pQue->m_EndFunc))(SSH2_FX_FAILURE, pQue);
	else if ( type == SSH2_FXP_STATUS )
		return (this->*(pQue->m_EndFunc))(bp->Get32Bit(), pQue);
	else
		return (this->*(pQue->m_EndFunc))(SSH2_FX_FAILURE, pQue);
}
int CSFtp::RemoteReadDirRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	int n, i, max;
	CString cwd;
	CStringA tmp;
	CFileNode node;

	if ( type != SSH2_FXP_NAME ) {
		pQue->m_Len = (type == SSH2_FXP_STATUS && bp->Get32Bit() == SSH2_FX_EOF ? 0 : (-1));
		RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
		SendCommand(pQue, &CSFtp::RemoteCloseDirRes, SENDCMD_HEAD);
		return FALSE;
	}
	
	cwd = pQue->m_Path;
	if ( cwd.Compare(_T("/")) == 0 )
		cwd = _T("");

	max = bp->Get32Bit();
	for ( n = 0 ; n < max ; n++ ) {
		bp->GetStr(tmp);
		KanjiConvToLocal(tmp, node.m_file);
		bp->GetStr(tmp);	// dummy strring read
		node.DecodeAttr(bp);
		node.m_path.Format(_T("%s/%s"), cwd, node.m_file);
		node.SetSubName();
		if ( !node.IsDot() ) {
			if ( node.IsDir() || node.IsReg() )
				pQue->m_FileNode.Add(node);
			else if ( node.IsLink() ) {
				i = (int)pQue->m_FileNode.Add(node);
				RemoteLinkStat(node.m_path, pQue, i);
			}
		}
	}

	RemoteMakePacket(pQue, SSH2_FXP_READDIR);
	SendCommand(pQue, &CSFtp::RemoteReadDirRes, SENDCMD_NOWAIT);
	return FALSE;
}
int CSFtp::RemoteOpenDirRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( type != SSH2_FXP_HANDLE )
		return RemoteCloseDirRes((-1), bp, pQue);

	bp->GetBuf(&pQue->m_Handle);

	RemoteMakePacket(pQue, SSH2_FXP_READDIR);
	SendCommand(pQue, &CSFtp::RemoteReadDirRes, SENDCMD_NOWAIT);
	return FALSE;
}
int CSFtp::RemoteSetCwdRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	CFileNode node;
	CStringA tmp;

	if ( type != SSH2_FXP_NAME || bp->Get32Bit() != 1 ) {
		DispErrMsg(_T("Get Real Path Error"), pQue->m_Path);
		return TRUE;
	}

	bp->GetStr(tmp); KanjiConvToLocal(tmp, pQue->m_Path);
	bp->GetStr(tmp); KanjiConvToLocal(tmp, pQue->m_CopyPath);
	node.DecodeAttr(bp);

	if ( pQue->m_Len != 0 && (node.m_flags & SSH2_FILEXFER_ATTR_ACMODTIME) != 0 && node.m_mtime <= m_RemoteCurTime )
		return TRUE;

	m_RemoteCurTime = (node.m_flags & SSH2_FILEXFER_ATTR_ACMODTIME ? node.m_mtime : time(NULL));
	pQue->m_EndFunc = &CSFtp::RemoteSetListRes;

	RemoteMakePacket(pQue, SSH2_FXP_OPENDIR);
	SendCommand(pQue, &CSFtp::RemoteOpenDirRes, SENDCMD_NOWAIT);
	SetUpDownCount(1);
	return FALSE;
}
void CSFtp::RemoteSetCwd(LPCTSTR path)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = path;
	pQue->m_Len  = 0;

	RemoteMakePacket(pQue, SSH2_FXP_REALPATH);
	SendCommand(pQue, &CSFtp::RemoteSetCwdRes, SENDCMD_NOWAIT);
}
int CSFtp::RemoteMtimeCwdRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	CFileNode node;

	if ( type != SSH2_FXP_ATTRS ) {
//		DispErrMsg("Get Stat Attr Error", pQue->m_Path);
		return TRUE;
	}
	node.DecodeAttr(bp);

	if ( (node.m_flags & SSH2_FILEXFER_ATTR_ACMODTIME) != 0 && node.m_mtime <= m_RemoteCurTime )
		return TRUE;

	m_RemoteCurTime = (node.m_flags & SSH2_FILEXFER_ATTR_ACMODTIME ? node.m_mtime : time(NULL));
	pQue->m_EndFunc = &CSFtp::RemoteSetListRes;

	RemoteMakePacket(pQue, SSH2_FXP_OPENDIR);
	SendCommand(pQue, &CSFtp::RemoteOpenDirRes, SENDCMD_NOWAIT);
	SetUpDownCount(1);
	return FALSE;
}
void CSFtp::RemoteMtimeCwd(LPCTSTR path)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = path;
	pQue->m_Len = 1;

	RemoteMakePacket(pQue, SSH2_FXP_STAT);
	SendCommand(pQue, &CSFtp::RemoteMtimeCwdRes, SENDCMD_NOWAIT);
}
void CSFtp::RemoteUpdateCwd(LPCTSTR path)
{
	int n;
	CString tmp = path;

	if ( (n = tmp.ReverseFind(_T('/'))) >= 0 )
		tmp = tmp.Left(n);

	if ( tmp.Compare(m_RemoteCurDir) != 0 )
		return;

	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = m_RemoteCurDir;
	pQue->m_EndFunc = &CSFtp::RemoteSetListRes;

	RemoteMakePacket(pQue, SSH2_FXP_OPENDIR);
	SendCommand(pQue, &CSFtp::RemoteOpenDirRes, SENDCMD_NOWAIT);
	SetUpDownCount(1);
}
void CSFtp::RemoteDeleteDir(LPCTSTR path)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = path;
	pQue->m_EndFunc = &CSFtp::RemoteDeleteDirRes;

	RemoteMakePacket(pQue, SSH2_FXP_OPENDIR);
	SendCommand(pQue, &CSFtp::RemoteOpenDirRes, SENDCMD_HEAD);
	SetUpDownCount(1);
}
void CSFtp::RemoteCacheDir(LPCTSTR path)
{
	CCmdQue *pQue = new CCmdQue;

	pQue->m_Path = path;
	pQue->m_EndFunc = &CSFtp::RemoteCacheDirRes;

	RemoteMakePacket(pQue, SSH2_FXP_OPENDIR);
	SendCommand(pQue, &CSFtp::RemoteOpenDirRes, SENDCMD_NOWAIT);
	SetUpDownCount(1);
}

/////////////////////////////////////////////////////////////////////////////

int CSFtp::RemoteCloseReadRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( pQue->m_Fd != (-1) ) {
		_close(pQue->m_Fd);
		if ( !m_DoAbort )
			pQue->m_FileNode[0].SetFileStat(pQue->m_CopyPath);
	}

	if ( type >= 0 ) {
		if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK )
			DispErrMsg(_T("Close Read Error"), pQue->m_Path);
		else if ( pQue->m_Len == (-1) )
			DispErrMsg(_T("File Read Error"), pQue->m_Path);
		else if ( pQue->m_Len == (-2) )
			DispErrMsg(_T("Local File Create Error"), pQue->m_FileNode[0].m_path);
		else if ( pQue->m_Len == (-3) )
			DispErrMsg(_T("Local File Write Error"), pQue->m_FileNode[0].m_path);
	} else if ( type == (-1) )
		DispErrMsg(_T("Open Read Error"), pQue->m_Path);

	if ( pQue->m_Len != (-10) )
		LocalUpdateCwd(pQue->m_CopyPath);

	if ( m_DoAbort ) {
		RemoveWaitQue();
		m_UpDownCount = 0;
		m_TotalSize = 0;
		SetUpDownCount(0);
	} else
		SetUpDownCount(-1);
	SetRangeProg(NULL, 0, 0);

	return TRUE;
}
int CSFtp::RemoteDataReadRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	int n;
	CCmdQue *pOwner = (pQue->m_pOwner != NULL ? pQue->m_pOwner : pQue);

	if ( type != SSH2_FXP_DATA || m_DoAbort ) {
		if ( pQue->m_pOwner != NULL ) {
			if ( !m_DoAbort )
				DispErrMsg(_T("Remote Read Error"), pOwner->m_Path);
			return TRUE;
		}
		pQue->m_Len = (type == SSH2_FXP_STATUS && bp->Get32Bit() == SSH2_FX_EOF ? 0 : (m_DoAbort ? 0 : (-1)));
		RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
		SendCommand(pQue, &CSFtp::RemoteCloseReadRes, SENDCMD_NOWAIT);
		return FALSE;
	}

	if ( (n = bp->Get32Bit()) != pQue->m_Len || pOwner->m_Fd == (-1) ||
			_lseeki64(pOwner->m_Fd, pQue->m_Offset, SEEK_SET) != pQue->m_Offset || _write(pOwner->m_Fd, bp->GetPtr(), n) != n )
		goto WRITEERR;

	if ( pOwner == pQue )
		SetPosProg(pQue->m_Offset + n);

	pQue->m_Offset = pOwner->m_NextOfs;
	pQue->m_Len    = (pOwner->m_FileNode[0].HaveSize() && (pOwner->m_Size - pQue->m_Offset) <= SSH2_FX_TRANSBUFLEN ? (int)(pOwner->m_Size - pQue->m_Offset) : SSH2_FX_TRANSBUFLEN);
	n = ( m_UpDownRate > 0 ? (pOwner->m_Max * SSH2_FX_TRANSBUFLEN * 100 / m_UpDownRate) : SSH2_FX_TRANSTYPMSEC);

	if ( pOwner != pQue && (n > SSH2_FX_TRANSMAXMSEC || (pQue->m_Offset + pQue->m_Len) >= pOwner->m_Size) ) {
		pOwner->m_Max--;
		return TRUE;
	}

	RemoteMakePacket(pQue, SSH2_FXP_READ);
	SendCommand(pQue, &CSFtp::RemoteDataReadRes, SENDCMD_NOWAIT);
	pOwner->m_NextOfs += pQue->m_Len;

	if ( n < SSH2_FX_TRANSMINMSEC && pOwner->m_Max < SSH2_FX_MAXQUESIZE && (pOwner->m_NextOfs + SSH2_FX_TRANSBUFLEN) < pOwner->m_Size ) {
		CCmdQue *pDmy = new CCmdQue;
		pDmy->m_pOwner = pOwner;
		pDmy->m_Handle = pOwner->m_Handle;
		pDmy->m_Offset = pOwner->m_NextOfs;
		pDmy->m_Len    = SSH2_FX_TRANSBUFLEN;

		RemoteMakePacket(pDmy, SSH2_FXP_READ);
		SendCommand(pDmy, &CSFtp::RemoteDataReadRes, SENDCMD_NOWAIT);
		pOwner->m_NextOfs += pDmy->m_Len;
		pOwner->m_Max++;
//		TRACE("RemoteDataReadRes %d(%d)\n", pOwner->m_Max, n);
	}
	return FALSE;

WRITEERR:
	if ( pOwner == pQue ) {
		pQue->m_Len = (-3);
		RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
		SendCommand(pQue, &CSFtp::RemoteCloseReadRes, SENDCMD_NOWAIT);
		return FALSE;
	} else {
		if ( pOwner->m_Fd != (-1) )
			_close(pOwner->m_Fd);
		pOwner->m_Fd = (-1);
		pOwner->m_Max--;
		return TRUE;
	}
}
int CSFtp::RemoteOpenReadRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( type != SSH2_FXP_HANDLE )
		return RemoteCloseReadRes((-1), bp, pQue);
	bp->GetBuf(&pQue->m_Handle);

	if ( (pQue->m_Fd = _topen(pQue->m_CopyPath, (pQue->m_NextOfs != 0 ? _O_BINARY | _O_WRONLY : _O_BINARY | _O_WRONLY | _O_CREAT | _O_TRUNC), _S_IREAD | _S_IWRITE)) == (-1) ) {
		pQue->m_Len = (-2);
		RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
		SendCommand(pQue, &CSFtp::RemoteCloseReadRes, SENDCMD_NOWAIT);
		return FALSE;
	}

	pQue->m_pOwner = NULL;
	pQue->m_Max    = 1;
	pQue->m_Size   = (pQue->m_FileNode[0].HaveSize() ? pQue->m_FileNode[0].m_size : 0);
	SetRangeProg(pQue->m_CopyPath, pQue->m_Size, pQue->m_NextOfs);

	pQue->m_Offset  = pQue->m_NextOfs;
	pQue->m_Len     = (pQue->m_FileNode[0].HaveSize() && (pQue->m_Size - pQue->m_Offset) <= SSH2_FX_TRANSBUFLEN ? (int)(pQue->m_Size - pQue->m_Offset) : SSH2_FX_TRANSBUFLEN);

	RemoteMakePacket(pQue, SSH2_FXP_READ);
	SendCommand(pQue, &CSFtp::RemoteDataReadRes, SENDCMD_NOWAIT);
	pQue->m_NextOfs += pQue->m_Len;

	return FALSE;
}
void CSFtp::DownLoadFile(CFileNode *pNode, LPCTSTR file)
{
	CCmdQue *pQue;
	LONGLONG resume = 0;
	struct _stati64 st;

	if ( file == NULL )
		return;

	if ( pNode->IsDir() ) {
		pQue = new CCmdQue;
		pQue->m_Path     = pNode->m_path;
		pQue->m_CopyPath = file;
		pQue->m_EndFunc  = &CSFtp::RemoteCopyDirRes;

		RemoteMakePacket(pQue, SSH2_FXP_OPENDIR);
		SendCommand(pQue, &CSFtp::RemoteOpenDirRes, SENDCMD_HEAD);
		SetUpDownCount(1);
		return;
	}

	if ( !_tstati64(file, &st) ) {
		if ( (m_DoUpdate & 0x80) == 0 ) {
			CUpdateDlg dlg(this);
			dlg.m_FileName = file;
			dlg.m_DoResume = (st.st_size < pNode->m_size && st.st_mtime >= pNode->m_mtime ? TRUE : FALSE);
			m_DoExec |= 001;
			dlg.DoModal();
			m_DoUpdate = dlg.m_DoExec;
			m_DoExec &= ~001;
		}
		switch(m_DoUpdate & 0x0F) {
		case UDO_OVERWRITE:
			break;
		case UDO_MTIMECHECK:
			if ( st.st_mtime >= pNode->m_mtime )
				return;
			break;
		case UDO_UPDCHECK:
			if ( st.st_mtime == pNode->m_mtime && st.st_size == pNode->m_size )
				return;
			break;
		case UDO_RESUME:
			if ( st.st_size < pNode->m_size )
				resume = st.st_size;
			break;
		case UDO_ABORT:
			m_DoAbort = TRUE;
		default:
			return;
		}
	}

	pQue = new CCmdQue;
	pQue->m_Path = pNode->m_path;
	pQue->m_CopyPath = file;
	pQue->m_FileNode.RemoveAll();
	pQue->m_FileNode.Add(*pNode);
	pQue->m_NextOfs = resume;
	pQue->m_Fd = (-1);

	RemoteMakePacket(pQue, SSH2_FXP_OPEN);
	pQue->m_Msg.Put32Bit(SSH2_FXF_READ);
	pQue->m_FileNode[0].EncodeAttr(&(pQue->m_Msg));
	SendCommand(pQue, &CSFtp::RemoteOpenReadRes, SENDCMD_TAIL);
	SetUpDownCount(1);
}

/////////////////////////////////////////////////////////////////////////////

int CSFtp::RemoteCloseWriteRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	if ( pQue->m_Fd != (-1) )
		_close(pQue->m_Fd);

	if ( type >= 0 ) {
		if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK )
			DispErrMsg(_T("Close Write Error"), pQue->m_Path);
		else if ( pQue->m_Len == (-1) )
			DispErrMsg(_T("File Read Error"), pQue->m_FileNode[0].m_path);
		else if ( pQue->m_Len == (-2) )
			DispErrMsg(_T("Remote Write Error"), pQue->m_Path);
		else if ( pQue->m_Len == (-3) )
			DispErrMsg(_T("Set Attr Error"), pQue->m_Path);
	} else if ( type == (-1) )
		DispErrMsg(_T("File / Dir Write Error"), pQue->m_Path);
	else if ( type == (-2) )
		DispErrMsg(_T("Make Directory Error"), pQue->m_Path);
	else if ( type == (-3) )
		DispErrMsg(_T("Open Write Error"), pQue->m_Path);

	if ( m_DoAbort ) {
		RemoveWaitQue();
		m_UpDownCount = 0;
		m_TotalSize = 0;
		SetUpDownCount(0);
	} else
		SetUpDownCount(-1);
	SetRangeProg(NULL, 0, 0);

	if ( type != (-10) )
		RemoteUpdateCwd(pQue->m_Path);

	return TRUE;
}
int CSFtp::RemoteAttrWriteRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	pQue->m_Len = (type == SSH2_FXP_STATUS && bp->Get32Bit() == SSH2_FX_OK ? 0 : (-3));
	RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
	SendCommand(pQue, &CSFtp::RemoteCloseWriteRes, SENDCMD_NOWAIT);
	return FALSE;
}
int CSFtp::RemoteDataWriteRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	int n, r;
	LPBYTE buf;
	CCmdQue *pOwner = (pQue->m_pOwner != NULL ? pQue->m_pOwner : pQue);

	if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK || m_DoAbort ) {
		if ( pQue->m_pOwner != NULL ) {
			if ( !m_DoAbort )
				DispErrMsg(_T("Remote Write Error"), pOwner->m_Path);
			return TRUE;
		}
		pQue->m_Len = (m_DoAbort ? 0 : (-2));
		RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
		SendCommand(pQue, &CSFtp::RemoteCloseWriteRes, SENDCMD_NOWAIT);
		return FALSE;
	}

	if ( pOwner->m_Fd == (-1) )
		goto READERR;

	if ( pQue == pOwner ) {
		SetPosProg(pQue->m_Offset + pQue->m_Len);
		if ( (pQue->m_Offset + pQue->m_Len) >= pQue->m_Size ) {		// End of ?
			RemoteMakePacket(pQue, SSH2_FXP_FSETSTAT);
			pQue->m_FileNode[0].m_flags &= ~SSH2_FILEXFER_ATTR_PERMISSIONS;		// Not Copy Permissions XXXXXXXX
			pQue->m_FileNode[0].EncodeAttr(&(pQue->m_Msg));
			SendCommand(pQue, &CSFtp::RemoteAttrWriteRes, SENDCMD_NOWAIT);
			return FALSE;
		}
	}

	pQue->m_Offset = pOwner->m_NextOfs;
	pQue->m_Len    = (pOwner->m_FileNode[0].HaveSize() && (pOwner->m_Size - pQue->m_Offset) <= SSH2_FX_TRANSBUFLEN ? (int)(pOwner->m_Size - pQue->m_Offset) : SSH2_FX_TRANSBUFLEN);
	n = ( m_UpDownRate > 0 ? (pOwner->m_Max * SSH2_FX_TRANSBUFLEN * 100 / m_UpDownRate) : SSH2_FX_TRANSTYPMSEC);

	if ( pQue != pOwner && (n > SSH2_FX_TRANSMAXMSEC || (pQue->m_Offset + pQue->m_Len) >= pOwner->m_Size) ) {
		pOwner->m_Max--;
		return TRUE;
	}

	RemoteMakePacket(pQue, SSH2_FXP_WRITE);
	buf = pQue->m_Msg.PutSpc(pQue->m_Len);
	for ( r = 0 ; ; r++ ) {
		if ( _lseeki64(pOwner->m_Fd, pQue->m_Offset, SEEK_SET) == pQue->m_Offset && _read(pOwner->m_Fd, buf, pQue->m_Len) == pQue->m_Len )
			break;
		if ( r > 100 )
			goto READERR;
	}
	SendCommand(pQue, &CSFtp::RemoteDataWriteRes, SENDCMD_NOWAIT);
	pOwner->m_NextOfs += pQue->m_Len;

	if ( n < SSH2_FX_TRANSMINMSEC && pOwner->m_Max < SSH2_FX_MAXQUESIZE && (pOwner->m_NextOfs + SSH2_FX_TRANSBUFLEN) < pOwner->m_Size ) {
		CCmdQue *pDmy = new CCmdQue;
		pDmy->m_pOwner = pOwner;
		pDmy->m_Handle = pOwner->m_Handle;
		pDmy->m_Offset = pOwner->m_NextOfs;
		pDmy->m_Len    = SSH2_FX_TRANSBUFLEN;

		RemoteMakePacket(pDmy, SSH2_FXP_WRITE);
		buf = pDmy->m_Msg.PutSpc(pDmy->m_Len);
		for ( r = 0 ; ; r++ ) {
			if ( _lseeki64(pOwner->m_Fd, pDmy->m_Offset, SEEK_SET) == pDmy->m_Offset && _read(pOwner->m_Fd, buf, pDmy->m_Len) == pDmy->m_Len )
				break;
			if ( r > 100 ) {
				delete pDmy;
				goto READERR;
			}
		}
		SendCommand(pDmy, &CSFtp::RemoteDataWriteRes, SENDCMD_NOWAIT);
		pOwner->m_NextOfs += pDmy->m_Len;
		pOwner->m_Max++;
//		TRACE("RemoteDataWriteRes %d(%d)\n", pOwner->m_Max, n);
	}

	return FALSE;

READERR:
	if ( pQue == pOwner ) {
		pQue->m_Len = (-1);
		RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
		SendCommand(pQue, &CSFtp::RemoteCloseWriteRes, SENDCMD_NOWAIT);
		return FALSE;
	} else {
		if ( pOwner->m_Fd != (-1) )
			_close(pOwner->m_Fd);
		pOwner->m_Fd = (-1);
		pOwner->m_Max--;
		return TRUE;
	}
}
int CSFtp::RemoteOpenWriteRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	LPBYTE buf;

	if ( type != SSH2_FXP_HANDLE )
		return RemoteCloseWriteRes((-3), bp, pQue);
	bp->GetBuf(&pQue->m_Handle);

	if ( (pQue->m_Fd = _topen(pQue->m_FileNode[0].m_path, _O_BINARY | _O_RDONLY)) == (-1) )
		goto CLOSEWRITE;

	pQue->m_pOwner  = NULL;
	pQue->m_Size = (pQue->m_FileNode[0].HaveSize() ? pQue->m_FileNode[0].m_size : 0);
	SetRangeProg(pQue->m_FileNode[0].m_path, pQue->m_Size, pQue->m_NextOfs);

	pQue->m_Max     = 1;
	pQue->m_Offset  = pQue->m_NextOfs;
	pQue->m_Len     = (pQue->m_FileNode[0].HaveSize() && (pQue->m_Size - pQue->m_Offset) <= SSH2_FX_TRANSBUFLEN ? (int)(pQue->m_Size - pQue->m_Offset) : SSH2_FX_TRANSBUFLEN);

	RemoteMakePacket(pQue, SSH2_FXP_WRITE);
	buf = pQue->m_Msg.PutSpc(pQue->m_Len);
	if ( _lseeki64(pQue->m_Fd, pQue->m_Offset, SEEK_SET) != pQue->m_Offset || _read(pQue->m_Fd, buf, pQue->m_Len) != pQue->m_Len )
		goto CLOSEWRITE;
	SendCommand(pQue, &CSFtp::RemoteDataWriteRes, SENDCMD_NOWAIT);
	pQue->m_NextOfs += pQue->m_Len;
	return FALSE;

CLOSEWRITE:
	pQue->m_Len = (-1);
	RemoteMakePacket(pQue, SSH2_FXP_CLOSE);
	SendCommand(pQue, &CSFtp::RemoteCloseReadRes, SENDCMD_NOWAIT);
	return FALSE;
}
int CSFtp::RemoteMkDirWriteRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	BOOL DoLoop;
	CString tmp;
    CFileFind Finder;
	CFileNode node;

	if ( type != (-1) ) {
		if ( type != SSH2_FXP_STATUS || bp->Get32Bit() != SSH2_FX_OK )
			return RemoteCloseWriteRes((-2), bp, pQue);
		else
			RemoteUpdateCwd(pQue->m_Path);
	}

	SetUpDownCount(-1);
	tmp.Format(_T("%s\\*.*"), pQue->m_FileNode[0].m_path);

	pQue->m_SaveNode.RemoveAll();
	DoLoop = Finder.FindFile(tmp);
	while ( DoLoop != FALSE ) {
		DoLoop = Finder.FindNextFile();
		if ( Finder.IsSystem() || Finder.IsTemporary() || Finder.IsDots() )
			continue;
		if ( node.GetFileStat(Finder.GetFilePath(), Finder.GetFileName()) ) {
			UpLoadFile(&node, node.GetRemotePath(pQue->m_Path, this));
			if ( (m_DoUpdate & 0x40) != 0 && type == (-1) )
				pQue->m_SaveNode.Add(node);
		}
	}
	Finder.Close();

	if ( (m_DoUpdate & 0x40) == 0 || type != (-1) )
		return TRUE;

	pQue->m_FileNode.RemoveAll();
	pQue->m_EndFunc  = &CSFtp::RemoteMirrorUploadRes;

	RemoteMakePacket(pQue, SSH2_FXP_OPENDIR);
	SendCommand(pQue, &CSFtp::RemoteOpenDirRes, SENDCMD_NOWAIT);
	SetUpDownCount(1);
	return FALSE;
}
int CSFtp::RemoteStatWriteRes(int type, CBuffer *bp, class CCmdQue *pQue)
{
	LONGLONG resume = 0;

	if ( type == SSH2_FXP_ATTRS ) {
		CFileNode node;
		node.DecodeAttr(bp);

		if ( node.IsDir() ) {
			if ( !pQue->m_FileNode[0].IsDir() )
				return RemoteCloseWriteRes((-1), bp, pQue);
			return RemoteMkDirWriteRes((-1), bp, pQue);
		} else if ( pQue->m_FileNode[0].IsDir() )
			return RemoteCloseWriteRes((-1), bp, pQue);

		if ( (m_DoUpdate & 0x80) == 0 ) {
			CUpdateDlg dlg(this);
			dlg.m_FileName = pQue->m_Path;
			dlg.m_DoResume = (node.m_size < pQue->m_FileNode[0].m_size && node.m_mtime >= pQue->m_FileNode[0].m_mtime ? TRUE : FALSE);
			m_DoExec |= 001;
			dlg.DoModal();
			m_DoUpdate = dlg.m_DoExec;
			m_DoExec &= ~001;
		}
		switch(m_DoUpdate & 0x0F) {
		case UDO_OVERWRITE:
			break;
		case UDO_MTIMECHECK:
			if ( node.m_mtime >= pQue->m_FileNode[0].m_mtime )
				return RemoteCloseWriteRes((-10), bp, pQue);
			break;
		case UDO_UPDCHECK:
			if ( node.m_mtime == pQue->m_FileNode[0].m_mtime && node.m_size == pQue->m_FileNode[0].m_size )
				return RemoteCloseWriteRes((-10), bp, pQue);
			break;
		case UDO_RESUME:
			if ( node.m_size < pQue->m_FileNode[0].m_size )
				resume = node.m_size;
			break;
		case UDO_ABORT:
			m_DoAbort = TRUE;
		default:
			return RemoteCloseWriteRes((-10), bp, pQue);
		}

		/********************
		if ( node.m_flags & SSH2_FILEXFER_ATTR_PERMISSIONS )		// Copy Permissions XXXXXXXX
			pQue->m_FileNode[0].m_attr = node.m_attr;
		*********************/
	}

	if ( pQue->m_FileNode[0].IsDir() ) {
		RemoteMakePacket(pQue, SSH2_FXP_MKDIR);
		pQue->m_Msg.Put32Bit(SSH2_FILEXFER_ATTR_PERMISSIONS);
		pQue->m_Msg.Put32Bit(0755);
		SendCommand(pQue, &CSFtp::RemoteMkDirWriteRes, SENDCMD_NOWAIT);
	} else {
		pQue->m_NextOfs = resume;
		pQue->m_Fd      = (-1);

		RemoteMakePacket(pQue, SSH2_FXP_OPEN);
		pQue->m_Msg.Put32Bit(pQue->m_NextOfs != 0 ? SSH2_FXF_WRITE : SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_TRUNC);
		pQue->m_FileNode[0].EncodeAttr(&(pQue->m_Msg));
		SendCommand(pQue, &CSFtp::RemoteOpenWriteRes, SENDCMD_TAIL);
	}
	return FALSE;
}
void CSFtp::UpLoadFile(CFileNode *pNode, LPCTSTR file)
{
	CFileNode *np;

	TRACE("%s:%s", pNode->m_path, file);

	if ( pNode->IsDir() ) {
		RemoteCacheDir(file);

	} else if ( (m_DoUpdate & 0x80) != 0 && (np = RemoteCacheFind(file)) != NULL ) {
		if ( ((m_DoUpdate & 0x0F) == UDO_MTIMECHECK && pNode->m_mtime <= np->m_mtime) ||
			((m_DoUpdate & 0x0F) == UDO_UPDCHECK   && pNode->m_mtime == np->m_mtime && pNode->m_size == np->m_size) ) {
			TRACE(" Cache\n");
			return;
		}
	}

	CCmdQue *pQue = new CCmdQue;
	pQue->m_Path = file;
	pQue->m_FileNode.RemoveAll();
	pQue->m_FileNode.Add(*pNode);

	RemoteMakePacket(pQue, SSH2_FXP_STAT);
	SendCommand(pQue, &CSFtp::RemoteStatWriteRes, SENDCMD_TAIL);
	SetUpDownCount(1);
TRACE(" Stat\n");
}

/////////////////////////////////////////////////////////////////////////////

int CSFtp::LocalDelete(LPCTSTR path)
{
	CString str;
	DWORD FileAttr;
    CFileFind Finder;
	BOOL DoLoop;

	if ( (FileAttr = GetFileAttributes(path)) == (-1) )
		return FALSE;

	if ( (FileAttr & FILE_ATTRIBUTE_READONLY) != 0 ) {
		FileAttr &= ~FILE_ATTRIBUTE_READONLY;
		if ( !SetFileAttributes(path, FileAttr) )
			return FALSE;
	}

	if ( (FileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0 ) {
		str.Format(_T("%s\\*.*"), path);
		DoLoop = Finder.FindFile(str);
		while ( DoLoop != FALSE ) {
			DoLoop = Finder.FindNextFile();
			if ( Finder.IsDots() )
				continue;
			if ( !LocalDelete(Finder.GetFilePath()) )
				return FALSE;
		}
		Finder.Close();

		if ( !RemoveDirectory(path) )
			return FALSE;

	} else if ( !DeleteFile(path) )
		return FALSE;

	return TRUE;
}
int CSFtp::LocalSetCwd(LPCTSTR path)
{
	int n, i;
	BOOL DoLoop;
	CString tmp;
    CFileFind Finder;
	TCHAR buf[_MAX_DIR];
	CFileNode node;
	struct _stati64 st;

	if ( _tchdir(path) )
		return FALSE;

	if ( _tgetcwd(buf, _MAX_DIR) == NULL )
		return FALSE;

	if ( !_tstati64(buf, &st) )
		m_LocalCurTime = st.st_mtime;

	m_LocalCwd.SetWindowText(buf);
	if ( m_LocalCwd.FindStringExact(0, buf) == CB_ERR )
		m_LocalCwd.AddString(buf);

	m_LocalCurDir = buf;
	SetLocalCwdHis(buf);

	m_LocalNode.RemoveAll();
	tmp.Format(_T("%s\\*.*"), buf);
	DoLoop = Finder.FindFile(tmp);

	while ( DoLoop != FALSE ) {
		DoLoop = Finder.FindNextFile();
		if ( Finder.IsSystem() || Finder.IsTemporary() || Finder.IsDots() )
			continue;
		if ( node.GetFileStat(Finder.GetFilePath(), Finder.GetFileName()) )
			m_LocalNode.Add(node);
	}
	Finder.Close();

	m_LocalList.DeleteAllItems();
	m_LocalList.SetItemCount((int)m_LocalNode.GetSize());

	for ( n = 0 ; n < m_LocalNode.GetSize() ; n++ ) {
		i = m_LocalList.InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, n, m_LocalNode[n].m_file, 0, 0, m_LocalNode[n].GetIcon(), n);
		m_LocalList.SetItemText(i, 1, m_LocalNode[n].GetModTime());
		m_LocalList.SetItemText(i, 2, m_LocalNode[n].GetFileSize());
		m_LocalList.SetItemText(i, 3, m_LocalNode[n].GetAttr());
	}
	ListSort(0, (-1));

	return TRUE;
}
void CSFtp::LocalUpdateCwd(LPCTSTR path)
{
	int n;
	CString tmp(path);

	if ( (n = tmp.ReverseFind(_T('\\'))) >= 0 )
		tmp = tmp.Left(n);

	if ( tmp.Compare(m_LocalCurDir) == 0 )
		LocalSetCwd(m_LocalCurDir);
}

/////////////////////////////////////////////////////////////////////////////

void CSFtp::SetLocalCwdHis(LPCTSTR cwd)
{
	int n;
	POSITION pos;
	CString tmp;

	if ( (pos = m_LocalCwdHis.Find(cwd)) != NULL )
		m_LocalCwdHis.RemoveAt(pos);
	m_LocalCwdHis.AddHead(cwd);

	pos = m_LocalCwdHis.GetHeadPosition();
	for ( n = 0 ; n < 10 && pos != NULL ; n++ ) {
		tmp.Format(_T("LoCurDir_%s_%d"), m_pSSh->m_HostName, n);
		AfxGetApp()->WriteProfileString(_T("CSFtp"), tmp, m_LocalCwdHis.GetNext(pos));
	}
}
void CSFtp::SetRemoteCwdHis(LPCTSTR cwd)
{
	int n;
	POSITION pos;
	CString tmp;

	if ( (pos = m_RemoteCwdHis.Find(cwd)) != NULL )
		m_RemoteCwdHis.RemoveAt(pos);
	m_RemoteCwdHis.AddHead(cwd);

	pos = m_RemoteCwdHis.GetHeadPosition();
	for ( n = 0 ; n < 10 && pos != NULL ; n++ ) {
		tmp.Format(_T("ReCurDir_%s_%d"), m_pSSh->m_HostName, n);
		AfxGetApp()->WriteProfileString(_T("CSFtp"), tmp, m_RemoteCwdHis.GetNext(pos));
	}
}

int CSFtp::CompareNode(int src, int dis)
{
	int rc = 0;
	int item, revs;
	CFileNode *pSrc, *pDis;

	switch(m_SortNodeNum) {
	case 0:			// m_LocalNode
		pSrc = &(m_LocalNode[src]);
		pDis = &(m_LocalNode[dis]);
		item = m_LocalSortItem & 0x7F;
		revs = m_LocalSortItem & 0x80;
		break;
	case 1:			// m_RemoteNode
		pSrc = &(m_RemoteNode[src]);
		pDis = &(m_RemoteNode[dis]);
		item = m_RemoteSortItem & 0x7F;
		revs = m_RemoteSortItem & 0x80;
		break;
	}

	if ( pSrc->IsDir() != pDis->IsDir() )
		return (pSrc->IsDir() ? (-1) : 1);

	switch(item) {
	case 0:			// m_file
		rc = _tcscmp(pSrc->m_file, pDis->m_file);
		break;
	case 1:			// m_mtime
		rc = (int)(pSrc->m_mtime - pDis->m_mtime);
		break;
	case 2:			// m_size
		rc = (int)(pSrc->m_size - pDis->m_size);
		break;
	case 3:			// m_attr
		rc = _tcscmp(pSrc->GetAttr(), pDis->GetAttr());
		break;
	case 4:			// m_uid
		rc = pSrc->m_uid - pDis->m_uid;
		break;
	case 5:			// m_gid
		rc = pSrc->m_uid - pDis->m_uid;
		break;
	case 6:			// m_sub
		rc = _tcscmp(pSrc->m_sub, pDis->m_sub);
		break;
	}

	if ( rc == 0 && item != 0 )
		rc = _tcscmp(pSrc->m_file, pDis->m_file);

	return (revs ? (0 - rc) : rc);
}
void CSFtp::ListSort(int num, int item)
{
	CString tmp;
	m_SortNodeNum = num;
	switch(m_SortNodeNum) {
	case 0:			// m_LocalNode
		if ( item >= 0 ) {
			if ( (m_LocalSortItem & 0x7F) == item )
				m_LocalSortItem ^= 0x80;
			else
				m_LocalSortItem = item;
		}
		m_LocalList.SortItems(ListCompareFunc, (LPARAM)this);
		tmp.Format(_T("LocalSort_%s"), m_pSSh->m_HostName);
		AfxGetApp()->WriteProfileInt(_T("CSFtp"), tmp, m_LocalSortItem);
		break;
	case 1:			// m_RemoteNode
		if ( item >= 0 ) {
			if ( (m_RemoteSortItem & 0x7F) == item )
				m_RemoteSortItem ^= 0x80;
			else
				m_RemoteSortItem = item;
		}
		m_RemoteList.SortItems(ListCompareFunc, (LPARAM)this);
		tmp.Format(_T("RemoteSort_%s"), m_pSSh->m_HostName);
		AfxGetApp()->WriteProfileInt(_T("CSFtp"), tmp, m_RemoteSortItem);
		break;
	}
}

void CSFtp::SetUpDownCount(int count)
{
	CString tmp;
	m_UpDownCount += count;
	tmp.Format(_T("%d"), m_UpDownCount);
	m_UpDownStat[0].SetWindowText(tmp);
	m_UpDownStat[0].EnableWindow(m_UpDownCount > 0);
	tmp.Format((m_UpDownCount > 0 ? _T("SFtp(%d)") : _T("SFtp")), m_UpDownCount);
	SetWindowText(tmp);
}
void CSFtp::SetRangeProg(LPCTSTR file, LONGLONG size, LONGLONG ofs)
{
	if ( size <= 0 ) {
		m_ProgDiv = 0;

		m_UpDownProg.SetRange(0, 0);
		m_UpDownProg.SetPos(0);
		m_UpDownStat[1].SetWindowText(file == NULL ? _T("") : file);

		m_UpDownProg.EnableWindow(FALSE);
		m_UpDownStat[1].EnableWindow(FALSE);
		m_UpDownStat[2].EnableWindow(FALSE);
		m_UpDownStat[3].EnableWindow(FALSE);

	} else {
		if ( (m_ProgDiv = (int)(size / 4096)) <= 0 )
			m_ProgDiv = 1;
		m_ProgSize   = size;
		m_ProgOfs    =  ofs;
		m_ProgClock  = clock();
		m_UpDownRate = 0;

		m_UpDownProg.EnableWindow(TRUE);
		m_UpDownStat[1].EnableWindow(TRUE);
		m_UpDownStat[2].EnableWindow(TRUE);
		m_UpDownStat[3].EnableWindow(TRUE);

		m_UpDownProg.SetRange(0, (int)(size / m_ProgDiv));
		m_UpDownProg.SetPos(0);
		m_UpDownStat[1].SetWindowText(file == NULL ? _T("") : file);
		m_UpDownStat[2].SetWindowText(_T(""));
		m_UpDownStat[3].SetWindowText(_T(""));
	}
}
void CSFtp::SetPosProg(LONGLONG pos)
{
	int n;
	double d, e;
	CString tmp[2];

	if ( m_ProgDiv <= 0 )
		return;

	if ( clock() > m_ProgClock ) {
		d = (double)(pos - m_ProgOfs) * CLOCKS_PER_SEC / (double)(clock() - m_ProgClock);

		if ( d > 0.0 ) {
			e = (double)(m_ProgSize - pos) / d;
			n = (int)e;
			if ( n >= 3600 )
				tmp[1].Format(_T("%d:%02d:%02d"), n / 3600, (n % 3600) / 60, n % 60);
			else if ( n >= 60 )
				tmp[1].Format(_T("%d:%02d"), n / 60, n % 60);
			else
				tmp[1].Format(_T("%d"), n);
		}

		if ( d > 10048576.0 )
			tmp[0].Format(_T("%dMB/Sec"), (int)(d / 1048576.0));
		else if ( d > 10024.0 )
			tmp[0].Format(_T("%dKB/Sec"), (int)(d / 1024.0));
		else
			tmp[0].Format(_T("%dB/Sec"), (int)(d));

		m_UpDownRate = (int)d;
	}
	m_UpDownProg.SetPos((int)(pos / m_ProgDiv));
	m_UpDownStat[2].SetWindowText(tmp[0]);
	m_UpDownStat[3].SetWindowText(tmp[1]);
}
void CSFtp::DispErrMsg(LPCTSTR msg, LPCTSTR file)
{
	CString tmp;

	if ( file != NULL )
		tmp.Format(_T("%s\n%s"), file, msg);
	else
		tmp = msg;

	MessageBox(tmp);
}

BEGIN_MESSAGE_MAP(CSFtp, CDialogExt)
	//{{AFX_MSG_MAP(CSFtp)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_LOCAL_UP, OnLocalUp)
	ON_BN_CLICKED(IDC_REMOTE_UP, OnRemoteUp)
	ON_CBN_SELENDOK(IDC_LOCAL_CWD, OnSelendokLocalCwd)
	ON_CBN_SELENDOK(IDC_REMOTE_CWD, OnSelendokRemoteCwd)
	ON_CBN_KILLFOCUS(IDC_LOCAL_CWD, OnKillfocusLocalCwd)
	ON_CBN_KILLFOCUS(IDC_REMOTE_CWD, OnKillfocusRemoteCwd)
	ON_NOTIFY(NM_DBLCLK, IDC_LOCAL_LIST, OnDblclkLocalList)
	ON_NOTIFY(NM_DBLCLK, IDC_REMOTE_LIST, OnDblclkRemoteList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOCAL_LIST, OnColumnclickLocalList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_REMOTE_LIST, OnColumnclickRemoteList)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_BEGINDRAG, IDC_LOCAL_LIST, OnBegindragLocalList)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY(LVN_BEGINDRAG, IDC_REMOTE_LIST, OnBegindragRemoteList)
	ON_COMMAND(IDM_SFTP_CLOSE, OnSftpClose)
	ON_NOTIFY(NM_RCLICK, IDC_LOCAL_LIST, OnRclickLocalList)
	ON_NOTIFY(NM_RCLICK, IDC_REMOTE_LIST, OnRclickRemoteList)
	ON_COMMAND(IDM_SFTP_DELETE, OnSftpDelete)
	ON_COMMAND(IDM_SFTP_MKDIR, OnSftpMkdir)
	ON_COMMAND(IDM_SFTP_CHMOD, OnSftpChmod)
	ON_COMMAND(IDM_SFTP_DOWNLOAD, OnSftpDownload)
	ON_COMMAND(IDM_SFTP_UPLOAD, OnSftpUpload)
	ON_COMMAND(IDM_SFTP_ALLDOWNLOAD, OnSftpAlldownload)
	ON_COMMAND(IDM_SFTP_ALLUPLOAD, OnSftpAllupload)
	ON_COMMAND(IDM_SFTP_ABORT, OnSftpAbort)
	ON_COMMAND(IDM_SFTP_RENAME, OnSftpRename)
	ON_COMMAND(IDM_SFTP_REFLESH, OnSftpReflesh)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_MESSAGE(WM_RECIVEBUFFER, OnReciveBuffer)
END_MESSAGE_MAP()

#define	ITM_LEFT_HALF	0001
#define	ITM_LEFT_RIGHT	0002
#define	ITM_RIGHT_HALF	0004
#define	ITM_RIGHT_RIGHT	0010
#define	ITM_TOP_BTM		0020
#define	ITM_BTM_BTM		0040

static	int		ItemTabInit = FALSE;
static	struct	_SftpDlgItem	{
	UINT	id;
	int		mode;
	RECT	rect;
} ItemTab[] = {
	{ IDC_LOCAL_UP,		0 },
	{ IDC_LOCAL_CWD,	ITM_RIGHT_HALF },
	{ IDC_LOCAL_LIST,	ITM_RIGHT_HALF | ITM_BTM_BTM },

	{ IDC_REMOTE_UP,	ITM_LEFT_HALF | ITM_RIGHT_HALF },
	{ IDC_REMOTE_CWD,	ITM_LEFT_HALF | ITM_RIGHT_RIGHT },
	{ IDC_REMOTE_LIST,	ITM_LEFT_HALF | ITM_RIGHT_RIGHT | ITM_BTM_BTM },

	{ IDC_STATUS1,		ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_STATUS2,		ITM_RIGHT_HALF | ITM_TOP_BTM | ITM_BTM_BTM }, 
	{ IDC_PROGRESS1,	ITM_LEFT_HALF  | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_STATUS3,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_STATUS4,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_TOP_BTM | ITM_BTM_BTM },

	{ 0,	0 },
};

void CSFtp::InitItemOffset()
{
	int n;
	int cx, mx, cy;
	CRect rect;
	WINDOWPLACEMENT place;
	CWnd *pWnd;

	if ( ItemTabInit )
		return;
	ItemTabInit = TRUE;

	GetClientRect(rect);
	cx = rect.Width();
	mx = cx / 2;
	cy = rect.Height();

	for ( n = 0 ; ItemTab[n].id != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].id)) == NULL )
			continue;
		pWnd->GetWindowPlacement(&place);
		if ( ItemTab[n].mode & ITM_LEFT_HALF )
			ItemTab[n].rect.left = place.rcNormalPosition.left - mx;
		if ( ItemTab[n].mode & ITM_LEFT_RIGHT )
			ItemTab[n].rect.left = cx - place.rcNormalPosition.left;
		if ( ItemTab[n].mode & ITM_RIGHT_HALF )
			ItemTab[n].rect.right = place.rcNormalPosition.right - mx;
		if ( ItemTab[n].mode & ITM_RIGHT_RIGHT )
			ItemTab[n].rect.right = cx - place.rcNormalPosition.right;

		if ( ItemTab[n].mode & ITM_TOP_BTM )
			ItemTab[n].rect.top = cy - place.rcNormalPosition.top;
		else
			ItemTab[n].rect.top = place.rcNormalPosition.top;

		if ( ItemTab[n].mode & ITM_BTM_BTM )
			ItemTab[n].rect.bottom = cy - place.rcNormalPosition.bottom;
		else
			ItemTab[n].rect.bottom = place.rcNormalPosition.bottom;
	}
}
void CSFtp::SetItemOffset(int cx, int cy)
{
	int n;
	int mx = cx / 2;
	WINDOWPLACEMENT place;
	CWnd *pWnd;

	if ( !ItemTabInit )
		return;

	for ( n = 0 ; ItemTab[n].id != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].id)) == NULL )
			continue;
		pWnd->GetWindowPlacement(&place);
		if ( ItemTab[n].mode & ITM_LEFT_HALF )
			place.rcNormalPosition.left = mx + ItemTab[n].rect.left;
		if ( ItemTab[n].mode & ITM_LEFT_RIGHT )
			place.rcNormalPosition.left = cx - ItemTab[n].rect.left;
		if ( ItemTab[n].mode & ITM_RIGHT_HALF )
			place.rcNormalPosition.right = mx + ItemTab[n].rect.right;
		if ( ItemTab[n].mode & ITM_RIGHT_RIGHT )
			place.rcNormalPosition.right = cx - ItemTab[n].rect.right;

		if ( ItemTab[n].mode & ITM_TOP_BTM )
			place.rcNormalPosition.top = cy - ItemTab[n].rect.top;
		else
			place.rcNormalPosition.top = ItemTab[n].rect.top + m_ToolBarOfs;

		if ( ItemTab[n].mode & ITM_BTM_BTM )
			place.rcNormalPosition.bottom = cy - ItemTab[n].rect.bottom;
		else
			place.rcNormalPosition.bottom = ItemTab[n].rect.bottom + m_ToolBarOfs;

		pWnd->SetWindowPlacement(&place);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSFtp メッセージ ハンドラ

void CSFtp::OnClose() 
{
	if ( !IsIconic() ) {
		CRect rect;
		GetWindowRect(rect);
		AfxGetApp()->WriteProfileInt(_T("SFtpWnd"), _T("x"),  rect.left);
		AfxGetApp()->WriteProfileInt(_T("SFtpWnd"), _T("y"),  rect.top);
		AfxGetApp()->WriteProfileInt(_T("SFtpWnd"), _T("cx"), rect.right);
		AfxGetApp()->WriteProfileInt(_T("SFtpWnd"), _T("cy"), rect.bottom);
	}
	Close();
	CDialogExt::OnClose();
}

void CSFtp::OnDestroy() 
{
	m_pChan->m_pFilter = NULL;
	Close();
	CDialogExt::OnDestroy();	
}

void CSFtp::PostNcDestroy() 
{
	_tchdir(((CRLoginApp *)AfxGetApp())->m_BaseDir);
	CDialogExt::PostNcDestroy();
	delete this;
}

BOOL CSFtp::OnInitDialog() 
{
	int n;
	CRect rect;
	CBitmap BitMap;
	static const LV_COLUMN lvt[6] = {
		{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT,	180, _T("Name"),	0, 0 },
		{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,  110, _T("Date"),	0, 0 },
		{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,   70, _T("Size"),	0, 0 },
		{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,   80, _T("Attr"),	0, 0 },
		{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,   40, _T("uid"),		0, 0 },
		{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_RIGHT,   40, _T("gid"),		0, 0 },
	};

	CDialogExt::OnInitDialog();

	if ( !m_wndToolBar.CToolBar::CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
			!m_wndToolBar.LoadToolBar(IDR_SFTPTOOL) ) {
		TRACE0("Failed to create toolbar\n");
	}

	BitMap.LoadBitmap(IDB_BITMAP5);
	m_ImageList[0].Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 10);
	m_ImageList[0].Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	BitMap.LoadBitmap(IDB_BITMAP6);
	m_ImageList[1].Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 10);
	m_ImageList[1].Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	BitMap.LoadBitmap(IDB_BITMAP7);
	m_ImageList[2].Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 10);
	m_ImageList[2].Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	m_wndToolBar.SetSizes(CSize(16+7, 16+8), CSize(16, 16));
	m_wndToolBar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM)(m_ImageList[0].m_hImageList));
	m_wndToolBar.SendMessage(TB_SETHOTIMAGELIST, 0, (LPARAM)(m_ImageList[1].m_hImageList));
	m_wndToolBar.SendMessage(TB_SETDISABLEDIMAGELIST, 0, (LPARAM)(m_ImageList[2].m_hImageList));
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST,0, reposQuery, rect);
	m_ToolBarOfs = rect.top;
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

	BitMap.LoadBitmap(IDB_BITMAP4);
	m_ImageList[3].Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 4);
	m_ImageList[3].Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	for ( n = 0 ; n < 4 ; n++ )
		m_LocalList.InsertColumn(n, &lvt[n]);
	m_LocalList.SetImageList(&m_ImageList[3], LVSIL_SMALL);
	m_LocalList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

	for ( n = 0 ; n < 6 ; n++ )
		m_RemoteList.InsertColumn(n, &lvt[n]);
	m_RemoteList.SetImageList(&m_ImageList[3], LVSIL_SMALL);
	m_RemoteList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

	DWORD len = GetLogicalDriveStrings(0, NULL);
	LPTSTR buf = new TCHAR[len + 4];
	LPTSTR p;
	GetLogicalDriveStrings(len + 4, buf);
	for ( p = buf ; *p != '\0' ; ) {
		m_LocalCwd.AddString(p);
		while ( *(p++) != '\0' );
	}
	delete buf;

	CString tmp, work;
	m_LocalCwdHis.RemoveAll();
	m_RemoteCwdHis.RemoveAll();
	for ( n = 1 ; n < 10 ; n++ ) {
		tmp.Format(_T("LoCurDir_%s_%d"), m_pSSh->m_HostName, n);
		tmp = AfxGetApp()->GetProfileString(_T("CSFtp"), tmp, _T(""));
		if ( !tmp.IsEmpty() ) {
			m_LocalCwdHis.AddTail(tmp);
			m_LocalCwd.AddString(tmp);
		}
		tmp.Format(_T("ReCurDir_%s_%d"), m_pSSh->m_HostName, n);
		tmp = AfxGetApp()->GetProfileString(_T("CSFtp"), tmp, _T(""));
		if ( !tmp.IsEmpty() ) {
			m_RemoteCwdHis.AddTail(tmp);
			m_RemoteCwd.AddString(tmp);
		}
	}

	tmp.Format(_T("LocalSort_%s"), m_pSSh->m_HostName);
	m_LocalSortItem  = AfxGetApp()->GetProfileInt(_T("CSFtp"), tmp, 0);
	tmp.Format(_T("RemoteSort_%s"), m_pSSh->m_HostName);
	m_RemoteSortItem = AfxGetApp()->GetProfileInt(_T("CSFtp"), tmp, 0);

	tmp.Format(_T("LoCurDir_%s_%d"), m_pSSh->m_HostName, 0);
	work  = ::AfxGetApp()->GetProfileString(_T("CSFtp"), tmp, _T("."));
	LocalSetCwd(work);

	tmp.Format(_T("ReCurDir_%s_%d"), m_pSSh->m_HostName, 0);
	work = ::AfxGetApp()->GetProfileString(_T("CSFtp"), tmp, _T("."));
	RemoteSetCwd(work);
	SendWaitQue();

	InitItemOffset();
	m_LocalList.DragAcceptFiles(TRUE);
	m_RemoteList.DragAcceptFiles(TRUE);

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	GetWindowRect(rect);
	rect.left   = AfxGetApp()->GetProfileInt(_T("SFtpWnd"), _T("x"),  rect.left);
	rect.top    = AfxGetApp()->GetProfileInt(_T("SFtpWnd"), _T("y"),  rect.top);
	rect.right  = AfxGetApp()->GetProfileInt(_T("SFtpWnd"), _T("cx"), rect.right);
	rect.bottom = AfxGetApp()->GetProfileInt(_T("SFtpWnd"), _T("cy"), rect.bottom);
	MoveWindow(rect, FALSE);

	SetTimer(1120, 3000, NULL);

	return TRUE;
}

#define _AfxSetDlgCtrlID(hWnd, nID)     SetWindowLong(hWnd, GWL_ID, nID)
#define _AfxGetDlgCtrlID(hWnd)          ((UINT)(WORD)::GetDlgCtrlID(hWnd))

BOOL CSFtp::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	ENSURE_ARG(pNMHDR != NULL);
	ENSURE_ARG(pResult != NULL);
	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	// allow top level routing frame to handle the message
	if (GetRoutingFrame() != NULL)
		return FALSE;

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	TCHAR szFullText[256];
	CString strTipText;
	UINT_PTR nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = _AfxGetDlgCtrlID((HWND)nID);
	}

	if (nID != 0) // will be zero on a separator
	{
		// don't handle the message if no string resource found
		if (AfxLoadString((UINT)nID, szFullText) == 0)
			return FALSE;

		// this is the command id, not the button index
		AfxExtractSubString(strTipText, szFullText, 1, '\n');
	}
#ifndef _UNICODE
	if (pNMHDR->code == TTN_NEEDTEXTA)
		Checked::strncpy_s(pTTTA->szText, _countof(pTTTA->szText), strTipText, _TRUNCATE);
	else
		_mbstowcsz(pTTTW->szText, strTipText, _countof(pTTTW->szText));
#else
	if (pNMHDR->code == TTN_NEEDTEXTA)
		_wcstombsz(pTTTA->szText, strTipText, _countof(pTTTA->szText));
	else
		Checked::wcsncpy_s(pTTTW->szText, _countof(pTTTW->szText), strTipText, _TRUNCATE);
#endif
	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);


	return TRUE;    // message was handled
}

void CSFtp::OnLocalUp() 
{
	LocalSetCwd(_T(".."));
}

void CSFtp::OnRemoteUp() 
{
	int n;
	CString tmp;
	if ( (n = m_RemoteCurDir.ReverseFind(_T('/'))) >= 0 )
		tmp = m_RemoteCurDir.Left(n);
	if ( tmp.IsEmpty() )
		tmp = _T("/");
	if ( tmp.Compare(m_RemoteCurDir) != 0 )
		RemoteSetCwd(tmp);
	SendWaitQue();
}

void CSFtp::OnSelendokLocalCwd() 
{
	int n;
	CString tmp;
	m_LocalCwd.GetWindowText(tmp);
	if ( (n = m_LocalCwd.GetCurSel()) != CB_ERR )
		m_LocalCwd.GetLBText(n, tmp);
	LocalSetCwd(tmp);
}

void CSFtp::OnSelendokRemoteCwd() 
{
	int n;
	CString tmp;
	m_RemoteCwd.GetWindowText(tmp);
	if ( (n = m_RemoteCwd.GetCurSel()) != CB_ERR )
		m_RemoteCwd.GetLBText(n, tmp);
	if ( tmp.Compare(m_RemoteCurDir) != 0 )
		RemoteSetCwd(tmp);
	SendWaitQue();
}

void CSFtp::OnKillfocusLocalCwd() 
{
	CString tmp;
	m_LocalCwd.GetWindowText(tmp);
	if ( tmp.Compare(m_LocalCurDir) != 0 )
		LocalSetCwd(tmp);
}

void CSFtp::OnKillfocusRemoteCwd() 
{
	CString tmp;
	m_RemoteCwd.GetWindowText(tmp);
	if ( tmp.Compare(m_RemoteCurDir) != 0 )
		RemoteSetCwd(tmp);
	SendWaitQue();
}

void CSFtp::OnDblclkLocalList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n;
	if ( (n = m_LocalList.GetSelectionMark()) >= 0 ) {
		n = (int)m_LocalList.GetItemData(n);
		if ( n >= 0 && n < m_LocalNode.GetSize() && m_LocalNode[n].IsDir() )
			LocalSetCwd(m_LocalNode[n].m_path);
	}
	*pResult = 0;
}

void CSFtp::OnDblclkRemoteList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n;
	if ( (n = m_RemoteList.GetSelectionMark()) >= 0 ) {
		n = (int)m_RemoteList.GetItemData(n);
		if ( n >= 0 && n < m_RemoteNode.GetSize()&& m_RemoteNode[n].IsDir() )
			RemoteSetCwd(m_RemoteNode[n].m_path);
		SendWaitQue();
	}
	*pResult = 0;
}

void CSFtp::OnColumnclickLocalList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	ListSort(0, pNMListView->iSubItem);
	*pResult = 0;
}

void CSFtp::OnColumnclickRemoteList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	ListSort(1, pNMListView->iSubItem);
	*pResult = 0;
}

void CSFtp::OnSize(UINT nType, int cx, int cy) 
{
	if ( nType != SIZE_MINIMIZED )
		SetItemOffset(cx, cy);
	CDialogExt::OnSize(nType, cx, cy);
	Invalidate(TRUE);
}

BOOL CSFtp::PreTranslateMessage(MSG* pMsg) 
{
	if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN )
		pMsg->wParam = VK_TAB;
	else if ( pMsg->message == WM_DROPFILES )
		return DropFiles(pMsg->hwnd, (HDROP)(pMsg->wParam));
	return CDialogExt::PreTranslateMessage(pMsg);
}

int CSFtp::DropFiles(HWND hWnd, HDROP hDropInfo) 
{
	int n, i;
	int max = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	TCHAR path[_MAX_PATH + 4];
	CString tmp, file;

	for ( n = 0 ; n < max ; n++ ) {
		DragQueryFile(hDropInfo, n, path, _MAX_PATH);
		tmp = path;
		if ( (i = tmp.ReverseFind(_T('\\'))) > 0 )
			file = tmp.Mid(i + 1);
		else
			file = tmp;
		if ( hWnd == m_RemoteList.m_hWnd ) {
			CFileNode node;
			if ( node.GetFileStat(path, file) )
				UpLoadFile(&node, node.GetRemotePath(m_RemoteCurDir, this));
			SendWaitQue();
		} else if ( hWnd == m_LocalList.m_hWnd ) {
			tmp = m_LocalCurDir;
			if ( tmp.Right(1).Compare(_T("\\")) != 0 )
				tmp += _T("\\");
			tmp += file;
			CopyFile(path, tmp, TRUE);
			LocalSetCwd(m_LocalCurDir);
		}
	}
	DragFinish(hDropInfo);

	return TRUE;
}

void CSFtp::OnBegindragLocalList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n;
	CPoint po(12, 8);
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if ( (n = m_LocalList.GetSelectedCount()) < 0 )
		goto ENDRET;
	else if ( n > 1 )
		n = 7;
	else {
		if ( (n = m_LocalList.GetSelectionMark()) < 0 )
			goto ENDRET;
		if ( (n = (int)m_LocalList.GetItemData(n)) < 0 || n >= m_LocalNode.GetSize() )
			goto ENDRET;
		n = m_LocalNode[n].GetIcon();
	}

	m_hDragWnd = pNMHDR->hwndFrom;
	m_ImageList[3].BeginDrag(n, po);
	m_ImageList[3].DragEnter(GetDesktopWindow(), pNMListView->ptAction);

	m_bDragList = TRUE;
	SetCapture();

ENDRET:
	*pResult = 0;
}

void CSFtp::OnBegindragRemoteList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int n;
	CPoint po(12, 8);
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if ( (n = m_RemoteList.GetSelectedCount()) < 0 )
		goto ENDRET;
	else if ( n > 1 )
		n = 7;
	else {
		if ( (n = m_RemoteList.GetSelectionMark()) < 0 )
			goto ENDRET;
		if ( (n = (int)m_RemoteList.GetItemData(n)) < 0 || n >= m_RemoteNode.GetSize() )
			goto ENDRET;
		n = m_RemoteNode[n].GetIcon();
	}

	m_hDragWnd = pNMHDR->hwndFrom;
	m_ImageList[3].BeginDrag(n, po);
	m_ImageList[3].DragEnter(GetDesktopWindow(), pNMListView->ptAction);

	m_bDragList = TRUE;
	SetCapture();

ENDRET:
	*pResult = 0;
}

void CSFtp::OnMouseMove(UINT nFlags, CPoint point) 
{
	CPoint po(point);

	if ( m_bDragList ) {
		ClientToScreen(&po);
		m_ImageList[3].DragMove(po);
		m_ImageList[3].DragShowNolock(FALSE);
		m_ImageList[3].DragShowNolock(TRUE);
	}
	CDialogExt::OnMouseMove(nFlags, point);
}

void CSFtp::OnLButtonUp(UINT nFlags, CPoint point) 
{
	int n, i, md = (-1);
	CWnd *pWnd;
	CPoint po(point);
	CListCtrl *pList;

	if ( m_bDragList == FALSE )
		goto ENDRET;

	::ReleaseCapture();
	m_bDragList = FALSE;
	m_ImageList[3].DragLeave(GetDesktopWindow());
	m_ImageList[3].EndDrag();

	ClientToScreen(&po);
	if ( (pWnd = WindowFromPoint(po)) == NULL )
		goto ENDRET;

	if ( m_hDragWnd == m_RemoteList.m_hWnd ) {
		pList = &m_RemoteList;
		if ( pWnd->m_hWnd == m_LocalList.m_hWnd )
			md = 0;
	} else if ( m_hDragWnd == m_LocalList.m_hWnd ) {
		pList = &m_LocalList;
		if ( pWnd->m_hWnd == m_RemoteList.m_hWnd )
			md = 2;
	}

	if ( md < 0 )
		goto ENDRET;

	if ( m_CmdQue.IsEmpty() && pList == &m_LocalList ) {
		RemoteCacheRemoveAll();
		RemoteCacheCwd();
	}
	m_DoUpdate = 0;
	m_DoAbort  = FALSE;

	for ( n = 0 ; n < pList->GetItemCount() && m_DoAbort == FALSE ; n++ ) {
		if ( pList->GetItemState(n, LVIS_SELECTED) != 0 ) {
			i = (int)pList->GetItemData(n);

			switch(md) {
			case 0:	// Remote -> Local
				DownLoadFile(&(m_RemoteNode[i]), m_RemoteNode[i].GetLocalPath(m_LocalCurDir, this));
				break;
			case 2:	// Local -> Remote
				UpLoadFile(&(m_LocalNode[i]), m_LocalNode[i].GetRemotePath(m_RemoteCurDir, this));
				break;
			}
		}
	}

	SendWaitQue();

ENDRET:
	CDialogExt::OnLButtonUp(nFlags, point);
}

void CSFtp::OnSftpClose() 
{
	Close();
}

void CSFtp::OnRclickLocalList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CMenu menu, *submenu;
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CPoint point = pNMListView->ptAction;

	menu.LoadMenu(IDR_SFTPMENU);
	m_LocalList.ClientToScreen(&point);
	if ( (submenu = menu.GetSubMenu(1)) != NULL )
		submenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, this);
	*pResult = 0;
}

void CSFtp::OnRclickRemoteList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CMenu menu, *submenu;
	NMLISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CPoint point = pNMListView->ptAction;

	menu.LoadMenu(IDR_SFTPMENU);
	m_RemoteList.ClientToScreen(&point);
	if ( (submenu = menu.GetSubMenu(1)) != NULL )
		submenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, this);
	*pResult = 0;
}

void CSFtp::OnSftpDelete() 
{
	int n, i;
	CListCtrl *pList = (CListCtrl *)GetFocus();
	int len = 0;
	CString tmp;

	if ( pList->m_hWnd == m_LocalList.m_hWnd ) {
		for ( n = 0 ; n < m_LocalList.GetItemCount() ; n++ ) {
			if ( m_LocalList.GetItemState(n, LVIS_SELECTED) != 0 ) {
				i = (int)m_LocalList.GetItemData(n);
				if ( len++ <= 0 )
					tmp = m_LocalNode[i].m_file;
				else if ( len < 3 ) {
					tmp += ",";
					tmp += m_LocalNode[i].m_file;
				} else
					tmp.Format(CStringLoad(IDS_FILECOUNTMSG), len);
			}
		}
		tmp += CStringLoad(IDS_FILEDELETEREQ);
		if ( MessageBox(tmp, _T("Question"), MB_YESNO | MB_ICONQUESTION) != IDYES )
			return;
		for ( n = 0 ; n < m_LocalList.GetItemCount() ; n++ ) {
			if ( m_LocalList.GetItemState(n, LVIS_SELECTED) != 0 ) {
				i = (int)m_LocalList.GetItemData(n);
				if ( !LocalDelete(m_LocalNode[i].m_path) )
					DispErrMsg(_T("Can't Delete Folder"), m_LocalNode[i].m_path);
			}
		}
		LocalSetCwd(m_LocalCurDir);

	} else if ( pList->m_hWnd == m_RemoteList.m_hWnd ) {
		for ( n = 0 ; n < m_RemoteList.GetItemCount() ; n++ ) {
			if ( m_RemoteList.GetItemState(n, LVIS_SELECTED) != 0 ) {
				i = (int)m_RemoteList.GetItemData(n);
				if ( len++ <= 0 )
					tmp = m_RemoteNode[i].m_file;
				else if ( len < 3 ) {
					tmp += ",";
					tmp += m_RemoteNode[i].m_file;
				} else
					tmp.Format(CStringLoad(IDS_FILECOUNTMSG), len);
			}
		}
		tmp += CStringLoad(IDS_FILEDELETEREQ);
		if ( MessageBox(tmp, _T("Question"), MB_YESNO | MB_ICONQUESTION) != IDYES )
			return;
		for ( n = 0 ; n < m_RemoteList.GetItemCount() ; n++ ) {
			if ( m_RemoteList.GetItemState(n, LVIS_SELECTED) != 0 ) {
				i = (int)m_RemoteList.GetItemData(n);
				RemoteDelete(&(m_RemoteNode[i]), TRUE);
			}
		}
		SendWaitQue();
	}
}

void CSFtp::OnSftpMkdir() 
{
	CEditDlg dlg(this);
	CListCtrl *pList = (CListCtrl *)GetFocus();

	dlg.m_Title.LoadString(IDS_CREATEDIRMSG);
	if ( pList->m_hWnd == m_LocalList.m_hWnd ) {
		dlg.m_Edit = m_LocalCurDir;
		if ( m_LocalCurDir.Right(1).Compare(_T("\\")) != 0 )
			dlg.m_Edit += _T("\\");
		if ( dlg.DoModal() != IDOK )
			return;
		if ( !CreateDirectory(dlg.m_Edit, NULL) )
			DispErrMsg(_T("Can't Create Directory"), dlg.m_Edit);
		LocalSetCwd(m_LocalCurDir);
	} else if ( pList->m_hWnd == m_RemoteList.m_hWnd ) {
		dlg.m_Edit = m_RemoteCurDir;
		if ( m_RemoteCurDir.Right(1).Compare(_T("/")) != 0 )
			dlg.m_Edit += _T("/");
		if ( dlg.DoModal() != IDOK )
			return;
		RemoteMakeDir(dlg.m_Edit, TRUE);
		SendWaitQue();
	}
}

void CSFtp::OnSftpRename() 
{
	int n;
	CEditDlg dlg(this);
	CString tmp;
	CListCtrl *pList = (CListCtrl *)GetFocus();

	if ( pList->m_hWnd != m_LocalList.m_hWnd && pList->m_hWnd != m_RemoteList.m_hWnd )
		return;

	if ( (n = pList->GetSelectionMark()) < 0 || (n = (int)pList->GetItemData(n)) < 0 )
		return;

	dlg.m_Title.LoadString(IDS_RENAMEMSG);
	if ( pList->m_hWnd == m_LocalList.m_hWnd ) {
		dlg.m_Edit = m_LocalNode[n].m_file;
		if ( dlg.DoModal() != IDOK )
			return;
		if ( _trename(m_LocalNode[n].m_file, dlg.m_Edit) )
			DispErrMsg(_T("Rename Error"), m_LocalNode[n].m_file);
		LocalSetCwd(m_LocalCurDir);
	} else if ( pList->m_hWnd == m_RemoteList.m_hWnd ) {
		dlg.m_Edit = m_RemoteNode[n].m_path;
		if ( dlg.DoModal() != IDOK )
			return;
		RemoteRename(&(m_RemoteNode[n]), dlg.m_Edit, TRUE);
		SendWaitQue();
	}
}

void CSFtp::OnSftpChmod() 
{
	int n, i;
	CChModDlg dlg(this);
	CListCtrl *pList = (CListCtrl *)GetFocus();

	if ( pList->m_hWnd != m_LocalList.m_hWnd && pList->m_hWnd != m_RemoteList.m_hWnd )
		return;

	if ( (n = pList->GetSelectionMark()) < 0 || (n = (int)pList->GetItemData(n)) < 0 )
		return;

	if ( pList->m_hWnd == m_LocalList.m_hWnd ) {
		dlg.m_Attr = m_LocalNode[n].m_attr;
		if ( dlg.DoModal() != IDOK )
			return;
		for ( n = 0 ; n < m_LocalList.GetItemCount() ; n++ ) {
			if ( m_LocalList.GetItemState(n, LVIS_SELECTED) != 0 ) {
				i = (int)m_LocalList.GetItemData(n);
				m_LocalNode[i].m_attr = dlg.m_Attr;
				m_LocalNode[i].SetFileStat(m_LocalNode[i].m_path);
			}
		}
		LocalSetCwd(m_LocalCurDir);
	} else {
		dlg.m_Attr = m_RemoteNode[n].m_attr;
		if ( dlg.DoModal() != IDOK )
			return;
		for ( n = 0 ; n < m_RemoteList.GetItemCount() ; n++ ) {
			if ( m_RemoteList.GetItemState(n, LVIS_SELECTED) != 0 ) {
				i = (int)m_RemoteList.GetItemData(n);
				RemoteSetAttr(&(m_RemoteNode[i]), dlg.m_Attr, TRUE);
			}
		}
		SendWaitQue();
	}
}

void CSFtp::OnSftpDownload() 
{
	int n, i;

	m_DoUpdate = 0;
	m_DoAbort  = FALSE;

	for ( n = 0 ; n < m_RemoteList.GetItemCount() && m_DoAbort == FALSE ; n++ ) {
		if ( m_RemoteList.GetItemState(n, LVIS_SELECTED) != 0 ) {
			i = (int)m_RemoteList.GetItemData(n);
			DownLoadFile(&(m_RemoteNode[i]), m_RemoteNode[i].GetLocalPath(m_LocalCurDir, this));
		}
	}
	SendWaitQue();
}

void CSFtp::OnSftpUpload() 
{
	int n, i;

	if ( m_CmdQue.IsEmpty() ) {
		RemoteCacheRemoveAll();
		RemoteCacheCwd();
	}
	m_DoUpdate = 0;
	m_DoAbort  = FALSE;

	for ( n = 0 ; n < m_LocalList.GetItemCount() && m_DoAbort == FALSE ; n++ ) {
		if ( m_LocalList.GetItemState(n, LVIS_SELECTED) != 0 ) {
			i = (int)m_LocalList.GetItemData(n);
			UpLoadFile(&(m_LocalNode[i]), m_LocalNode[i].GetRemotePath(m_RemoteCurDir, this));
		}
	}
	SendWaitQue();
}

void CSFtp::OnSftpAlldownload() 
{
	int n;
	CString tmp;
	CFileNode node;

	m_DoUpdate = UDO_UPDCHECK | 0xC0;
	m_DoAbort  = FALSE;

	tmp = m_RemoteCurDir;
	if ( (n = tmp.ReverseFind(_T('/'))) >= 0 )
		tmp = tmp.Mid(n + 1);

	node.m_file = tmp;
	node.m_path = m_RemoteCurDir;
	node.m_flags = SSH2_FILEXFER_ATTR_PERMISSIONS;
	node.m_attr = _S_IFDIR;

	DownLoadFile(&node, m_LocalCurDir);
	SendWaitQue();
}

void CSFtp::OnSftpAllupload() 
{
	int n;
	CString tmp;
	CFileNode node;

	if ( m_CmdQue.IsEmpty() ) {
		RemoteCacheRemoveAll();
		RemoteCacheCwd();
	}

	m_DoUpdate = UDO_UPDCHECK | 0xC0;
	m_DoAbort  = FALSE;

	tmp = m_LocalCurDir;
	if ( (n = tmp.ReverseFind(_T('\\'))) >= 0 )
		tmp = tmp.Mid(n + 1);

	node.m_file = tmp;
	node.m_path = m_LocalCurDir;
	node.m_flags = SSH2_FILEXFER_ATTR_PERMISSIONS;
	node.m_attr = _S_IFDIR;

	UpLoadFile(&node, m_RemoteCurDir);
	SendWaitQue();
}

void CSFtp::OnSftpReflesh() 
{
	LocalSetCwd(m_LocalCurDir);
	RemoteSetCwd(m_RemoteCurDir);
	SendWaitQue();
}

void CSFtp::OnSftpAbort() 
{
	m_DoAbort = TRUE;
}

void CSFtp::OnPaint() 
{
	if ( IsIconic() ) {
		CPaintDC dc(this);
		
		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	} else
		CDialogExt::OnPaint();
}

HCURSOR CSFtp::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CSFtp::OnTimer(UINT_PTR nIDEvent) 
{
	if ( (m_DoExec & 003) == 002 ) {
		m_DoExec &= ~002;
		OnRecive("", 0);
	}

	if ( m_UpdateCheckMode ) {	// Local
		struct _stati64 st;
		if ( !_tstati64(m_LocalCurDir, &st) && m_LocalCurTime < st.st_mtime && !m_DoExec )
			LocalSetCwd(m_LocalCurDir);
	} else {					// Remote
		if ( !m_RemoteCurDir.IsEmpty() && m_CmdQue.IsEmpty() && m_WaitQue.IsEmpty() && !m_DoExec )
			RemoteMtimeCwd(m_RemoteCurDir);
	}
	m_UpdateCheckMode ^= 1;
	CDialogExt::OnTimer(nIDEvent);
}

LRESULT CSFtp::OnReciveBuffer(WPARAM wParam, LPARAM lParam)
{
	int n;
	int len = (int)wParam;
	CBuffer *buf = (CBuffer *)lParam;

	for ( n = 0 ; n < len ; n++ )
		ReciveBuffer(&(buf[n]));

	delete [] buf;
	return TRUE;
}