#if !defined(AFX_SFTP_H__182BFCDB_1DB9_4B12_93A2_DEB89DD68159__INCLUDED_)
#define AFX_SFTP_H__182BFCDB_1DB9_4B12_93A2_DEB89DD68159__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SFtp.h : ヘッダー ファイル
//

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

#define	SSH2_FX_TRANSBUFLEN				(32 * 1024)
#define	SSH2_FX_TRANSMINMSEC			80
#define	SSH2_FX_TRANSTYPMSEC			100
#define	SSH2_FX_TRANSMAXMSEC			120

#define	SSH2_FX_MAXQUESIZE				128

#define _S_IFLNK  0xA000		/* symbolic link */

#define WM_RECIVEBUFFER		(WM_USER + 4)

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
	class CFileNode *Find(LPCSTR path);

	int GetIcon() { return m_icon; }
	LPCSTR GetFileSize();
	LPCSTR GetUserID();
	LPCSTR GetGroupID();
	LPCSTR GetAttr();
	LPCSTR GetAcsTime();
	LPCSTR GetModTime();
	void AutoRename(LPCSTR p, CString &tmp, int mode);
	LPCSTR GetLocalPath(LPCSTR dir, class CSFtp *pWnd);
	LPCSTR GetRemotePath(LPCSTR dir, class CSFtp *pWnd);

	int IsDir();
	int IsReg();
	int IsLink();
	int IsDot() { return (m_file.Compare(".") == 0 || m_file.Compare("..") == 0 ? TRUE : FALSE); }

	int HaveSize() { return (m_flags & SSH2_FILEXFER_ATTR_SIZE); }
	void SetSubName();
	int SetFileStat(LPCSTR file);
	int GetFileStat(LPCSTR path, LPCSTR file);
	void DecodeAttr(CBuffer *bp);
	void EncodeAttr(CBuffer *bp);
	const CFileNode & operator = (CFileNode &data);

	CFileNode();
	~CFileNode();
};

class CSFtp : public CDialog
{
// コンストラクション
public:
	CSFtp(CWnd* pParent = NULL);   // 標準のコンストラクタ
	~CSFtp();

// ダイアログ データ
	//{{AFX_DATA(CSFtp)
	enum { IDD = IDD_SFTPDLG };
	CListCtrl	m_RemoteList;
	CComboBox	m_RemoteCwd;
	CListCtrl	m_LocalList;
	CComboBox	m_LocalCwd;
	//}}AFX_DATA
	CProgressCtrl	m_UpDownProg;
	CStatic	m_UpDownStat[4];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CSFtp)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	class CChannel *m_pChan;
	class Cssh *m_pSSh;
	int m_VerId;
	int m_SeqId;
	CString m_LocalCurDir;
	CArray<CFileNode, CFileNode &> m_LocalNode;
	CString m_RemoteCurDir;
	CArray<CFileNode, CFileNode &> m_RemoteNode;
	CFileNode *m_pCacheNode;
	CImageList m_ImageList[4];
	CIConv m_IConv;
	CString m_HostKanjiSet;
	CToolBar m_wndToolBar;
	int m_ToolBarOfs;
	int m_UpdateCheckMode;
	time_t m_LocalCurTime;
	time_t m_RemoteCurTime;

	BOOL m_bDragList;
	HWND m_hDragWnd;
	int m_DoUpdate;
	BOOL m_DoAbort;
	int m_DoExec;
	CBuffer m_RecvBuf;
	int m_AutoRenMode;

	void OnConnect();
	int OnRecive(const void *lpBuf, int nBufLen);

	void Close();
	void Send(LPBYTE buf, int len);

	CList<class CCmdQue *, class CCmdQue *> m_CmdQue;
	CList<class CCmdQue *, class CCmdQue *> m_WaitQue;

	void SendBuffer(CBuffer *bp);
	int ReciveBuffer(CBuffer *bp);
	void SendCommand(class CCmdQue *pQue, int (CSFtp::*pFunc)(int type, CBuffer *bp, class CCmdQue *pQue), int mode);
#define	SENDCMD_NOWAIT	0
#define	SENDCMD_HEAD	1
#define	SENDCMD_TAIL	2
	void RemoveWaitQue();
	void SendWaitQue();

	int RemoteMakePacket(class CCmdQue *pQue, int Type);

	int RemoteLinkStatRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteLinkStat(LPCSTR path, CCmdQue *pOwner, int index);
	int RemoteMakeDirRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteMakeDir(LPCSTR dir, int show);
	int RemoteDeleteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteDelete(CFileNode *pNode, int show = FALSE);
	int RemoteSetAttrRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteSetAttr(CFileNode *pNode, int attr, int show = TRUE);
	int RemoteRenameRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void RemoteRename(CFileNode *pNode, LPCSTR newname, int show);

	CFileNode *RemoteCacheAdd(CFileNode *pNode);
	CFileNode *RemoteCacheFind(LPCSTR path);
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
	void RemoteSetCwd(LPCSTR path);
	void RemoteMtimeCwd(LPCSTR path);
	void RemoteUpdateCwd(LPCSTR path);
	void RemoteDeleteDir(LPCSTR path);
	void RemoteCacheDir(LPCSTR path);

	int RemoteCloseReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteDataReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteOpenReadRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void DownLoadFile(CFileNode *pNode, LPCSTR file);

	int RemoteCloseWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteAttrWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteDataWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteOpenWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteMkDirWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	int RemoteStatWriteRes(int type, CBuffer *bp, class CCmdQue *pQue);
	void UpLoadFile(CFileNode *pNode, LPCSTR file);

	int LocalDelete(LPCSTR path);
	int LocalSetCwd(LPCSTR path);
	void LocalUpdateCwd(LPCSTR path);

	int DropFiles(HWND hWnd, HDROP hDropInfo);

	CStringList m_LocalCwdHis;
	CStringList m_RemoteCwdHis;

	void SetLocalCwdHis(LPCSTR cwd);
	void SetRemoteCwdHis(LPCSTR cwd);

	void InitItemOffset();
	void SetItemOffset(int cx, int cy);

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
	LONGLONG m_TotalSize;

	void SetUpDownCount(int count);
	void SetRangeProg(LPCSTR file, LONGLONG size, LONGLONG ofs);
	void SetPosProg(LONGLONG pos);
	void DispErrMsg(LPCSTR msg, LPCSTR file);
	void KanjiConvToLocal(LPCSTR in, CString &out);
	void KanjiConvToRemote(LPCSTR in, CString &out);

	// 生成されたメッセージ マップ関数
protected:
	HICON m_hIcon;

	//{{AFX_MSG(CSFtp)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnLocalUp();
	afx_msg void OnRemoteUp();
	afx_msg void OnSelendokLocalCwd();
	afx_msg void OnSelendokRemoteCwd();
	afx_msg void OnKillfocusLocalCwd();
	afx_msg void OnKillfocusRemoteCwd();
	afx_msg void OnDblclkLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBegindragLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBegindragRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSftpClose();
	afx_msg void OnRclickLocalList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickRemoteList(NMHDR* pNMHDR, LRESULT* pResult);
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
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnReciveBuffer(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

class CCmdQue : public CObject
{
public:
	int m_ExtId;
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
	int m_Fd;
	CString m_Path;
	CString m_CopyPath;
	CArray<CFileNode, CFileNode &> m_FileNode;
	CArray<CFileNode, CFileNode &> m_SaveNode;
	class CCmdQue *m_pOwner;

	CCmdQue();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_SFTP_H__182BFCDB_1DB9_4B12_93A2_DEB89DD68159__INCLUDED_)
