//////////////////////////////////////////////////////////////////////
// CRLogin

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"       // メイン シンボル
#include "Data.h"
#include "ResDataBase.h"
#include "ExtFileDialog.h"

// CMainFrame
#define	WM_SOCKSEL			(WM_APP + 0)
#define WM_GETHOSTADDR		(WM_APP + 1)
#define	WM_ICONMSG			(WM_APP + 2)
#define WM_THREADCMD		(WM_APP + 3)
#define WM_AFTEROPEN		(WM_APP + 4)
#define WM_GETCLIPBOARD		(WM_APP + 5)

// CResTransDlg
#define WM_HTTPREQUEST		(WM_APP + 20)

// CRLoginView
#define WM_LOGWRITE			(WM_APP + 21)
#define	WM_SPEAKMSG			(WM_APP + 22)

// CSFtp
#define WM_RECIVEBUFFER		(WM_APP + 23)
#define WM_THREADENDOF		(WM_APP + 24)

// CFileThread
#define	WM_FILEWRITE		(WM_APP + 25)
#define	WM_FILEFLUSH		(WM_APP + 26)
#define	WM_FILESYNC			(WM_APP + 27)

// CCmdHisDlg
#define	WM_ADDCMDHIS		(WM_APP + 28)

#define	IDLEPROC_SOCKET		0
#define	IDLEPROC_ENCRYPT	1
#define	IDLEPROC_SCRIPT		2
#define	IDLEPROC_VIEW		3

#define	INUSE_NONE			0
#define	INUSE_ACTWIN		1
#define	INUSE_ALLWIN		2

#define	EMOJI_HASH			16
#define	EMOJI_LISTMAX		64

#define	SLEEPREQ_DISABLE	0
#define	SLEEPREQ_ENABLE		1
#define	SLEEPREQ_RESET		2

//////////////////////////////////////////////////////////////////////
// CCommandLineInfoEx

class CCommandLineInfoEx : public CCommandLineInfo
{
public:
	int m_PasStat;
	int m_Proto;
	CString m_Addr;
	CString m_Port;
	CString m_User;
	CString m_Pass;
	CString m_Term;
	CString m_Name;
	CString m_Idkey;
	CString m_Path;
	CString m_Cwd;
	int m_InUse;
	BOOL m_InPane;
	int m_AfterId;
	int m_ScreenX;
	int m_ScreenY;
	int m_ScreenW;
	int m_ScreenH;
	CString m_EventName;
	CString m_Title;
	CString m_Script;
	BOOL m_ReqDlg;

	CCommandLineInfoEx();
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
	BOOL ParseUrl(const TCHAR* pszParam);
	void GetString(CString &str);
	void SetString(LPCTSTR str);

	static LPCTSTR ShellEscape(LPCTSTR str);
};

//////////////////////////////////////////////////////////////////////
// CIdleProc

class CIdleProc : public CObject
{
public:
	class CIdleProc *m_pBack;
	class CIdleProc *m_pNext;
	int m_Type;
	void *m_pParam;
};

//////////////////////////////////////////////////////////////////////
// CRLoginApp

class CRLoginApp : public CWinApp
{
public:
	int m_IdleProcCount;
	CIdleProc *m_pIdleTop;
	CPtrArray m_SocketIdle;
	class CFontChache m_FontData;
	WSADATA wsaData;
	CString m_BaseDir;
	CString m_ExecDir;
	CString m_PathName;
	CCommandLineInfoEx *m_pCmdInfo;
	CServerEntry *m_pServerEntry;
	BOOL m_bLookCast;
	BOOL m_bOtherCast;
	CString m_LocalPass;
	BOOL m_bUseIdle;
	int m_TempSeqId;
	CString m_TempDirBase;
	CString m_TempDirPath;
	BOOL m_bRegistAppp;
	int m_MakeKeyMode;

	OSSL_PROVIDER *m_ProvDefault;
	OSSL_PROVIDER *m_ProvLegacy;
	OSSL_PROVIDER *m_ProvRlogin;

#ifdef	USE_KEYMACGLOBAL
	CKeyMacTab m_KeyMacGlobal;
#endif

#ifdef	USE_DIRECTWRITE
	ID2D1Factory *m_pD2DFactory;
	IDWriteFactory *m_pDWriteFactory;
	ID2D1DCRenderTarget *m_pDCRT;
	CEmojiImage *m_pEmojiList[EMOJI_HASH];
	CString m_EmojiFontName;
	CString m_EmojiImageDir;

	HDC GetEmojiImage(CEmojiImage *pEmoji);
	void SaveEmojiImage(CEmojiImage *pEmoji);
	HDC GetEmojiDrawText(CEmojiImage *pEmoji, COLORREF fc, int fh);
	BOOL DrawEmoji(CDC *pDC, CRect &rect, LPCTSTR str, COLORREF fc, COLORREF bc, BOOL bEraBack, int fh, int zm);
	void EmojiImageInit(LPCTSTR pFontName, LPCTSTR pImageDir);
#endif

#ifdef	USE_JUMPLIST
	IShellLink *MakeShellLink(LPCTSTR pEntryName);
	void CreateJumpList(CServerEntryTab *pEntry);
#endif

	ISpVoice *m_pVoice;
	CPtrArray m_VoiceList;

	BOOL AddVoiceToken(const WCHAR * pszCategoryId, const WCHAR * pszReqAttribs, const WCHAR * pszOptAttribs);
	ISpVoice *VoiceInit();
	void VoiceFinis();
	void SetVoiceListCombo(CComboBox *pCombo);
	void SetVoice(LPCTSTR str, long rate);

	BOOL GetExtFilePath(LPCTSTR name, LPCTSTR ext, CString &path);
	BOOL IsDirectory(LPCTSTR dir);
	BOOL CreateDesktopShortcut(LPCTSTR entry);

	void AddIdleProc(int Type, void *pParam);
	void DelIdleProc(int Type, void *pParam);

	virtual BOOL GetProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE* ppData, UINT* pBytes);
	virtual BOOL WriteProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes);
	virtual CString GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL);
	void GetProfileData(LPCTSTR lpszSection, LPCTSTR lpszEntry, void *lpBuf, int nBufLen, void *lpDef = NULL);
	void GetProfileBuffer(LPCTSTR lpszSection, LPCTSTR lpszEntry, CBuffer &Buf);
	void GetProfileStringArray(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringArrayExt &stra);
	void WriteProfileStringArray(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringArrayExt &stra);
	void GetProfileStringIndex(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringIndex &index);
	void WriteProfileStringIndex(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringIndex &index);
	void GetProfileArray(LPCTSTR lpszSection, CStringArrayExt &stra);
	void WriteProfileArray(LPCTSTR lpszSection, CStringArrayExt &stra);
	int GetProfileSeqNum(LPCTSTR lpszSection, LPCTSTR lpszEntry);
	void GetProfileKeys(LPCTSTR lpszSection, CStringArrayExt &stra);
	void DelProfileEntry(LPCTSTR lpszSection, LPCTSTR lpszEntry);
	void DelProfileSection(LPCTSTR lpszSection);
	BOOL AliveProfileKeys(LPCTSTR lpszSection);

	void RegisterShellRemoveAll();
	void RegisterShellFileEntry();
	void RegisterShellProtocol(LPCTSTR pProtocol, LPCTSTR pOption);
	BOOL RegisterGetStr(HKEY hKey, LPCTSTR pSection, LPCTSTR pEntryName, CString &str);
	BOOL RegisterSetStr(HKEY hKey, LPCTSTR pSection, LPCTSTR pEntryName, LPCTSTR str, BOOL bCreate = TRUE);
	BOOL RegisterSetDword(HKEY hKey, LPCTSTR pSection, LPCTSTR pEntryName, DWORD dword, BOOL bCreate = TRUE);
	void RegisterDelete(HKEY hKey, LPCTSTR pSection, LPCTSTR pKey);
	void RegisterSave(HKEY hKey, LPCTSTR pSection, CBuffer &buf);
	void RegisterLoad(HKEY hKey, LPCTSTR pSection, CBuffer &buf);

	BOOL SavePrivateProfileKey(HKEY hKey, CFile *file, BOOL bRLoginApp = FALSE);
	BOOL SavePrivateProfile();

	void RegistryEscapeStr(LPCTSTR str, int len, CString &out);
	BOOL SaveRegistryKey(HKEY hKey, CFile *file, LPCTSTR base);
	int SaveRegistryFile();

	LANGID GetLangId();
	void GetVersion(CString &str);
	void SetDefaultPrinter();
	void SSL_Init();
	BOOL InitLocalPass();

	void OpenProcsCmd(CCommandLineInfoEx *pCmdInfo);
	void OpenCommandEntry(LPCTSTR entry);
	void OpenCommandLine(LPCTSTR str);
	BOOL CheckDocument(class CRLoginDoc *pDoc);
	BOOL OnEntryData(COPYDATASTRUCT *pCopyData);
	void OpenRLogin(class CRLoginDoc *pDoc, CPoint *pPoint);

	DWORD m_FindProcsId;
	HWND m_FindProcsHwnd;

	HWND FindProcsMainWnd(DWORD ProcsId);
	HWND NewProcsMainWnd(CPoint *pPoint, BOOL *pbOpen);

	BOOL OnInUseCheck(COPYDATASTRUCT *pCopyData, BOOL bIcon);
	BOOL InUseCheck(BOOL bIcon);
	BOOL OnIsOnlineEntry(COPYDATASTRUCT *pCopyData);
	BOOL IsOnlineEntry(LPCTSTR entry);
	BOOL OnIsOpenRLogin(COPYDATASTRUCT *pCopyData);
	BOOL IsOpenRLogin();
	void OnUpdateServerEntry(COPYDATASTRUCT *pCopyData);
	void UpdateServerEntry();
#ifdef	USE_KEYMACGLOBAL
	void OnUpdateKeyMac(COPYDATASTRUCT *pCopyData);
	void UpdateKeyMacGlobal();
#endif
	void OnSendBroadCast(COPYDATASTRUCT *pCopyData);
	void OnSendGroupCast(COPYDATASTRUCT *pCopyData);
	void SendBroadCast(CBuffer &buf, LPCTSTR pGroup);
	void OnSendBroadCastMouseWheel(COPYDATASTRUCT *pCopyData);
	void SendBroadCastMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	static BOOL IsRLoginWnd(HWND hWnd);
	static HWND GetRLoginFromPoint(CPoint point);
	static BOOL IsWinVerCheck(int ver, int op = VER_EQUAL);	// VER_GREATER_EQUAL
	static void SetSleepMode(int req);

	CResDataBase m_ResDataBase;

	inline BOOL LoadResDialog(LPCTSTR lpszName, HGLOBAL &hTemplate, HGLOBAL &hInitData) { return m_ResDataBase.LoadResDialog(lpszName, hTemplate, hInitData); }
	inline BOOL LoadResString(LPCTSTR lpszName, CString &str) { return m_ResDataBase.LoadResString(lpszName, str); }
	inline BOOL LoadResMenu(LPCTSTR lpszName, HMENU &hMenu) { return m_ResDataBase.LoadResMenu(lpszName, hMenu); }
	inline BOOL LoadResToolBar(LPCTSTR lpszName, CToolBar &ToolBar, CWnd *pWnd) { return m_ResDataBase.LoadResToolBar(lpszName, ToolBar, pWnd); }
	inline BOOL LoadResBitmap(LPCTSTR lpszName, CBitmap &Bitmap) { return m_ResDataBase.LoadResBitmap(lpszName, Bitmap); }
	inline BOOL InitToolBarBitmap(LPCTSTR lpszName, UINT ImageId) { return m_ResDataBase.InitToolBarBitmap(lpszName, ImageId); }

	LPCTSTR GetTempDir(BOOL bSeqId);

	CRLoginApp();

// オーバーライド
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual BOOL IsIdleMessage(MSG* pMsg);

// 実装
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnAppAbout();
	afx_msg void OnFilePrintSetup();
	afx_msg void OnDispwinidx();
	afx_msg void OnDialogfont();
	afx_msg void OnLookcast();
	afx_msg void OnUpdateLookcast(CCmdUI *pCmdUI);
	afx_msg void OnOthercast();
	afx_msg void OnUpdateOthercast(CCmdUI *pCmdUI);
	afx_msg void OnPassLock();
	afx_msg void OnUpdatePassLock(CCmdUI *pCmdUI);
	afx_msg void OnSaveresfile();
	afx_msg void OnCreateprofile();
	afx_msg void OnUpdateCreateprofile(CCmdUI *pCmdUI);
	afx_msg void OnSaveregfile();
	afx_msg void OnUpdateSaveregfile(CCmdUI *pCmdUI);
	afx_msg void OnRegistapp();
	afx_msg void OnUpdateRegistapp(CCmdUI *pCmdUI);
	afx_msg void OnSecporicy();
};

extern CRLoginApp theApp;

extern BOOL ExDwmEnable;
extern void ExDwmEnableWindow(HWND hWnd, BOOL bEnable);

extern BOOL (__stdcall *ExAddClipboardFormatListener)(HWND hwnd);
extern BOOL (__stdcall *ExRemoveClipboardFormatListener)(HWND hwnd);
extern UINT (__stdcall *ExGetDpiForSystem)();
extern BOOL (__stdcall *ExEnableNonClientDpiScaling)(HWND hwnd);
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((DPI_AWARENESS_CONTEXT)-5)
#if	_MSC_VER <= _MSC_VER_VS10
  DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#endif
extern BOOL (__stdcall *ExIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
extern DPI_AWARENESS_CONTEXT (__stdcall *ExSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
extern UINT GlobalSystemDpi;

#define	WM_DPICHANGED	0x02E0
extern UINT (__stdcall *ExGetDpiForWindow)(HWND hwnd);
typedef enum _MONITOR_DPI_TYPE { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
extern HRESULT (__stdcall *ExGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
typedef enum _PROCESS_DPI_AWARENESS { PROCESS_DPI_UNAWARE, PROCESS_SYSTEM_DPI_AWARE, PROCESS_PER_MONITOR_DPI_AWARE } PROCESS_DPI_AWARENESS;
extern HRESULT (__stdcall *ExSetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value);

extern int ThreadMessageBox(LPCTSTR msg, ...);

#define	REQDPICONTEXT_SCALED	015
#define	REQDPICONTEXT_AWAREV2	016

extern INT_PTR DpiAwareDoModal(CCommonDialog &dlg, int req = REQDPICONTEXT_AWAREV2);
extern void DpiAwareSwitch(BOOL sw, int req = REQDPICONTEXT_AWAREV2);

#ifdef	USE_OLE
extern CLIPFORMAT CF_FILEDESCRIPTOR;
extern CLIPFORMAT CF_FILECONTENTS;
#endif
