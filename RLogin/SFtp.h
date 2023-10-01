#pragma once

#include "DialogExt.h"
#include "ComboBoxHis.h"
#include "ListCtrlExt.h"
#include "ToolBarEx.h"
#include <Shobjidl.h>

/* version */
#define SSH2_FILEXFER_VERSION           3

/* client to server */
#define SSH2_FXP_INIT                   1
#define SSH2_FXP_OPEN                   3
#define SSH2_FXP_CLOSE                  4
#define SSH2_FXP_READ                   5
#define SSH2_FXP_WRITE                  6
#define SSH2_FXP_LSTAT                  7
#define SSH2_FXP_STAT_VERSION_0         7
#define SSH2_FXP_FSTAT                  8
#define SSH2_FXP_SETSTAT                9
#define SSH2_FXP_FSETSTAT               10
#define SSH2_FXP_OPENDIR                11
#define SSH2_FXP_READDIR                12
#define SSH2_FXP_REMOVE                 13
#define SSH2_FXP_MKDIR                  14
#define SSH2_FXP_RMDIR                  15
#define SSH2_FXP_REALPATH               16
#define SSH2_FXP_STAT                   17
#define SSH2_FXP_RENAME                 18
#define SSH2_FXP_READLINK               19
#define SSH2_FXP_SYMLINK                20

/* server to client */
#define SSH2_FXP_VERSION                2
#define SSH2_FXP_STATUS                 101
#define SSH2_FXP_HANDLE                 102
#define SSH2_FXP_DATA                   103
#define SSH2_FXP_NAME                   104
#define SSH2_FXP_ATTRS                  105

#define SSH2_FXP_EXTENDED               200
#define SSH2_FXP_EXTENDED_REPLY         201

/* attributes */
#define SSH2_FILEXFER_ATTR_SIZE         0x00000001
#define SSH2_FILEXFER_ATTR_UIDGID       0x00000002
#define SSH2_FILEXFER_ATTR_PERMISSIONS  0x00000004
#define SSH2_FILEXFER_ATTR_ACMODTIME    0x00000008
#define SSH2_FILEXFER_ATTR_EXTENDED     0x80000000

/* portable open modes */
#define SSH2_FXF_READ                   0x00000001
#define SSH2_FXF_WRITE                  0x00000002
#define SSH2_FXF_APPEND                 0x00000004
#define SSH2_FXF_CREAT                  0x00000008
#define SSH2_FXF_TRUNC                  0x00000010
#define SSH2_FXF_EXCL                   0x00000020

/* status messages */
#define SSH2_FX_OK                      0
#define SSH2_FX_EOF                     1
#define SSH2_FX_NO_SUCH_FILE            2
#define SSH2_FX_PERMISSION_DENIED       3
#define SSH2_FX_FAILURE                 4
#define SSH2_FX_BAD_MESSAGE             5
#define SSH2_FX_NO_CONNECTION           6
#define SSH2_FX_CONNECTION_LOST         7
#define SSH2_FX_OP_UNSUPPORTED          8

#define	SSH2_FX_TRANSBUFLEN				(31 * 1024)
#define	SSH2_FX_TRANSMINMSEC			20
#define	SSH2_FX_TRANSTYPMSEC			25
#define	SSH2_FX_TRANSMAXMSEC			30

#define	SSH2_FX_MAXQUESIZE				32

#define _S_IFLNK						0xA000		/* symbolic link */
#define	FILEIORETRY_MAX					10

#define	SENDCMD_NOWAIT					0
#define	SENDCMD_HEAD					1
#define	SENDCMD_TAIL					2

#define	THREADCMD_COPY					0
#define	THREADCMD_MOVE					1
#define	THREADCMD_MKDIR					2
#define	THREADCMD_RMDIR					3

#define	ENDCMD_NONE						0
#define	ENDCMD_PASSFILE					1
#define	ENDCMD_GROUPFILE				2
#define	ENDCMD_MOVEFILE					3

#define	SFTP_TIMERID					1120

/////////////////////////////////////////////////////////////////////////////
// CSFtp ウィンドウ

class CFileNode : public CObject
{
public:
	int m_flags;
	LONGLONG m_size;
	WORD m_uid;
	WORD m_gid;
	WORD m_attr;
	time_t m_atime;
	time_t m_mtime;
	int m_icon;
	CString m_file;
	CString m_sub;
	CString m_path;
	CString m_name;
	class CFileNode *m_pLeft;
	class CFileNode *m_pRight;
	BOOL m_link;

	class CFileNode *Add(class CFileNode *pNode);
	class CFileNode *Find(LPCTSTR path);

	int GetIcon() { return m_icon; }
	LPCTSTR GetFileSize();
	LPCTSTR GetUserID(CStringBinary *pName = NULL);
	LPCTSTR GetGroupID(CStringBinary *pName = NULL);
	LPCTSTR GetAttr();
	LPCTSTR GetAcsTime();
	LPCTSTR GetModTime();
	void AutoRename(LPCTSTR p, CString &tmp, int mode);
	LPCTSTR GetLocalPath(LPCTSTR dir, class CSFtp *pWnd);
	LPCTSTR GetRemotePath(LPCTSTR dir, class CSFtp *pWnd);

	int IsDir();
	int IsReg();
	int IsLink();
	int IsDot() { return (m_file.Compare(_T(".")) == 0 || m_file.Compare(_T("..")) == 0 ? TRUE : FALSE); }

	int HaveSize() { return (m_flags & SSH2_FILEXFER_ATTR_SIZE); }
	void SetSubName();
	int SetFileStat(LPCTSTR file);
	int GetFileStat(LPCTSTR path, LPCTSTR file);
	void DecodeAttr(CBuffer *bp);
	void EncodeAttr(CBuffer *bp);
	const CFileNode & operator = (CFileNode &data);

	CFileNode();
	~CFileNode();
};

class CCmdQue : public CObject
{
public:
	DWORD m_ExtId;
	CTime m_SendTime;
	CBuffer m_Msg;
	int (CSFtp::*m_Func)(int type, CBuffer *bp, class CCmdQue *pQue);
	int (CSFtp::*m_EndFunc)(int st, CCmdQue *pQue);
	CBuffer m_Handle;
	LONGLONG m_Offset;
	LONGLONG m_Size;
	LONGLONG m_NextOfs;
	int m_Len;
	int m_Max;
	HANDLE m_hFile;
	CString m_Path;
	CString m_CopyPath;
	CArray<CFileNode, CFileNode &> m_FileNode;
	CArray<CFileNode, CFileNode &> m_SaveNode;
	class CCmdQue *m_pOwner;
	CBuffer m_MemData;
	int m_EndCmd;
	BOOL m_bThrad;
	class CSFtp *m_pSFtp;

	CCmdQue();
};

#ifdef	USE_OLE
class CSFtpDropTarget : public COleDropTarget
{
public:
	CSFtpDropTarget();
	~CSFtpDropTarget();

	virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
};
#endif	// USE_OLE

class CSFtp : public CDialogExt
{
	DECLARE_DYNAMIC(CSFtp)

// コンストラクション
public:
	CSFtp(CWnd* pParent = NULL);   // 標準のコンストラクタ
	virtual ~CSFtp();

// ダイアログ データ
	enum { IDD = IDD_SFTPDLG };

public:
	CListCtrlExt	m_RemoteList;
	CComboBoxExt	m_RemoteCwd;
	CListCtrlExt	m_LocalList;
	CComboBoxExt	m_LocalCwd;
	CProgressCtrl	m_UpDownProg;
	CStatic	m_UpDownStat[4];
	CString m_LastErrorMsg;
	class CFifoBase *m_pFifoSftp;

// クラスデータ
public:
	class CChannel *m_pChan;
	class Cssh *m_pSSh;
	int m_VerId;
	DWORD m_SeqId;
	CString m_LocalCurDir;
	CArray<CFileNode, CFileNode &> m_LocalNode;
	CString m_RemoteCurDir;
	CArray<CFileNode, CFileNode &> m_RemoteNode;
	CFileNode *m_pCacheNode;
	CImageList m_ImageList;
	CIConv m_IConv;
	CString m_HostKanjiSet;
	CToolBarEx m_wndToolBar;
	int m_ToolBarOfs;
	int m_UpdateCheckMode;
	time_t m_LocalCurTime;
	time_t m_RemoteCurTime;
	BOOL m_bUidGid;
	CStringBinary m_UserUid;
	CStringBinary m_GroupGid;

	BOOL m_bDragList;
	HWND m_hDragWnd;
	int m_DragImage;
	int m_DragAcvite;
	int m_DoUpdate;
	BOOL m_DoAbort;
	int m_DoExec;
	CBuffer m_RecvBuf;
	int m_AutoRenMode;
	HICON m_hIcon;
	int m_bShellExec[2];
	BOOL m_bPostMsg;
	BOOL m_bTheadExec;

#ifdef	USE_OLE
	CSFtpDropTarget m_DropTarget;
#endif

	void OnConnect();

	void Close();
	void Send(LPBYTE buf, int len);

	CList<class CCmdQue *, class CCmdQue *> m_CmdQue;
	CList<class CCmdQue *, class CCmdQue *> m_WaitQue;

	void SendBuffer(CBuffer *bp);
	int ReceiveBuffer(CBuffer *bp);
	void SendCommand(class CCmdQue *pQue, int (CSFtp::*pFunc)(int type, CBuffer *bp, class CCmdQue *pQue), int mode);
	void RemoveWaitQue();
	void SendWaitQue();

	void SetLastErrorMsg();
	BOOL SeekReadFile(HANDLE hFile, LPBYTE pBuffer, DWORD BufLen, LONGLONG SeekPos);
	BOOL SeekWriteFile(HANDLE hFile, LPBYTE pBuffer, DWORD BufLen, LONGLONG SeekPos);

	int RemoteMakePacket(class CCmdQue *pQue, int Type);

	int RemoteLinkStatRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteLinkStat(LPCTSTR path, CCmdQue *pOwner, int index);
	int RemoteMakeDirRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteMakeDir(LPCTSTR dir, int show);
	int RemoteDeleteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteDelete(CFileNode *pNode, int show = FALSE);
	int RemoteSetAttrRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteSetAttr(CFileNode *pNode, int attr, int show = TRUE);
	int RemoteRenameRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteRename(CFileNode *pNode, LPCTSTR newname, int show);

	CFileNode *RemoteCacheAdd(CFileNode *pNode);
	CFileNode *RemoteCacheFind(LPCTSTR path);
	void RemoteCacheRemoveAll();

	void RemoteCacheCwd();
	int RemoteCacheDirRes(int st, class CCmdQue *pQue);
	int RemoteMirrorUploadRes(int st, class CCmdQue *pQue);
	int RemoteDeleteDirRes(int st, class CCmdQue *pQue);
	int RemoteCopyDirRes(int st, class CCmdQue *pQue);
	int RemoteSetListRes(int st, class CCmdQue *pQue);

	int RemoteCloseDirRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteReadDirRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteOpenDirRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteSetCwdRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteMtimeCwdRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteSetCwd(LPCTSTR path, BOOL bNoWait = TRUE);
	void RemoteMtimeCwd(LPCTSTR path);
	void RemoteUpdateCwd(LPCTSTR path);
	void RemoteDeleteDir(LPCTSTR path);
	void RemoteCacheDir(LPCTSTR path);

	int RemoteEndofExec(int st, class CCmdQue *pQue);
	int RemoteCloseReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteDataReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteOpenReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void DownLoadFile(CFileNode *pNode, LPCTSTR file, BOOL bShellExec = FALSE);

	int RemoteCloseMemReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteDataMemReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteOpenMemReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void MemLoadFile(LPCTSTR file, int cmd);

	int RemoteDownEndof(int st, class CCmdQue *pQue);
	int RemoteCloseWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteAttrWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteDataWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteOpenWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteMkDirWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteStatWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void UpLoadFile(CFileNode *pNode, LPCTSTR file, BOOL bMove = FALSE);

	void ThreadCmdExec(int id, int type, LPCTSTR src, LPCTSTR dis, LONGLONG size);
	BOOL ThreadCmdStart(CCmdQue *pQue);
	void ThreadCmdAdd(int type, LPCTSTR src, LPCTSTR dis, LONGLONG size = 0);

	int LocalCopy(LPCTSTR src, LPCTSTR dis, BOOL bMove);
	static BOOL LocalDelete(LPCTSTR path);
	int LocalSetCwd(LPCTSTR path);
	void LocalUpdateCwd(LPCTSTR path);
	HDROP LocalDropInfo();
	int LocalSelectCount();

	BOOL DropFiles(HWND hWnd, HDROP hDropInfo, DROPEFFECT dropEffect);
	BOOL DescriptorFiles(HWND hWnd, HGLOBAL hDescInfo, DROPEFFECT dropEffect, COleDataObject *pDataObject);

	CStringList m_LocalCwdHis;
	CStringList m_RemoteCwdHis;

	void SetLocalCwdHis(LPCTSTR cwd);
	void SetRemoteCwdHis(LPCTSTR cwd);

	int m_LocalSortItem;
	int m_RemoteSortItem;
	int m_SortNodeNum;
	void ListSort(int num, int item);
	int CompareNode(int src, int dis);

	int m_UpDownCount;
	int m_UpDownRate;
	int m_ProgDiv;
	LONGLONG m_ProgSize;
	LONGLONG m_ProgOfs;
	clock_t m_ProgClock;
	LONGLONG m_TotalPos;
	LONGLONG m_TotalSize;
	LONGLONG m_TransmitSize;
	clock_t m_TransmitClock;
	CComPtr<ITaskbarList3> m_pTaskbarList;

	void SetUpDownCount(int count);
	void AddTotalRange(LONGLONG size);
	void SetRangeProg(LPCTSTR file, LONGLONG size, LONGLONG ofs);
	void SetPosProg(LONGLONG pos);
	void DispErrMsg(LPCTSTR msg, LPCTSTR file);
	LPCTSTR JointPath(LPCTSTR dir, LPCTSTR file, CString &path);
	inline void KanjiConvToLocal(LPCSTR in, CString &out) { m_IConv.RemoteToStr(m_HostKanjiSet, in, out); }
	inline void KanjiConvToRemote(LPCTSTR in, CStringA &out) {	m_IConv.StrToRemote(m_HostKanjiSet, in, out); }

// オーバーライド
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnOK();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnReceiveBuffer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnThreadEndof(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLocalUp();
	afx_msg void OnRemoteUp();
	afx_msg void OnSftpDelete();
	afx_msg void OnSftpMkdir();
	afx_msg void OnSftpChmod();
	afx_msg void OnSftpDownload();
	afx_msg void OnSftpUpload();
	afx_msg void OnSftpAlldownload();
	afx_msg void OnSftpAllupload();
	afx_msg void OnSftpAbort();
	afx_msg void OnSftpRename();
	afx_msg void OnSftpReflesh();
	afx_msg void OnSftpClose();
	afx_msg void OnSftpUidgid();
	afx_msg void OnSelendokLocalCwd();
	afx_msg void OnSelendokRemoteCwd();
	afx_msg void OnKillfocusLocalCwd();
	afx_msg void OnKillfocusRemoteCwd();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnDblclkLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindragLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindragRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
};

