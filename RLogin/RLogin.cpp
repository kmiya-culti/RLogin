// RLogin.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "RLogin.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"

#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CCommandLineInfoEx

CCommandLineInfoEx::CCommandLineInfoEx()
{
	m_PasStat = 0;
	m_Proto = (-1);
	m_Addr.Empty();
	m_Port.Empty();
	m_User.Empty();
	m_Pass.Empty();
	m_Term.Empty();
	m_InUse = FALSE;
}
void CCommandLineInfoEx::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	switch(m_PasStat) {
	case 0:
		if ( !bFlag )
			break;
		if ( _tcsicmp("ip", pszParam) == 0 )
			m_PasStat = 1;
		else if ( _tcsicmp("port", pszParam) == 0 )
			m_PasStat = 2;
		else if ( _tcsicmp("user", pszParam) == 0 )
			m_PasStat = 3;
		else if ( _tcsicmp("pass", pszParam) == 0 )
			m_PasStat = 4;
		else if ( _tcsicmp("term", pszParam) == 0 )
			m_PasStat = 5;
		else if ( _tcsicmp("direct", pszParam) == 0 )
			m_Proto = PROTO_DIRECT;
		else if ( _tcsicmp("login", pszParam) == 0 ) {
			m_Proto = PROTO_LOGIN;
			m_Port  = "login";
		} else if ( _tcsicmp("telnet", pszParam) == 0 ) {
			m_Proto = PROTO_TELNET;
			m_Port  = "telnet";
		} else if ( _tcsicmp("ssh", pszParam) == 0 ) {
			m_Proto = PROTO_SSH;
			m_Port  = "ssh";
		} else if ( _tcsicmp("inuse", pszParam) == 0 )
			m_InUse = TRUE;
		else
			break;
		ParseLast(bLast);
		return;
	case 1:
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Addr = pszParam;
		ParseLast(bLast);
		return;
	case 2:
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Port = pszParam;
		ParseLast(bLast);
		return;
	case 3:
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_User = pszParam;
		ParseLast(bLast);
		return;
	case 4:
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Pass = pszParam;
		ParseLast(bLast);
		return;
	case 5:
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Term = pszParam;
		ParseLast(bLast);
		return;
	}
	CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}
void CCommandLineInfoEx::GetString(CString &str)
{
	CStringArrayExt ary;

	ary.AddVal(m_Proto);
	ary.Add(m_Addr);
	ary.Add(m_Port);
	ary.Add(m_User);
	ary.Add(m_Pass);
	ary.Add(m_Term);
	ary.AddVal(m_InUse);
	ary.Add(m_strFileName);
	ary.AddVal(m_nShellCommand);

	ary.SetString(str);
}
void CCommandLineInfoEx::SetString(LPCSTR str)
{
	CStringArrayExt ary;

	ary.GetString(str);

	if ( ary.GetSize() <= 8 )
		return;

	m_Proto = ary.GetVal(0);
	m_Addr  = ary.GetAt(1);
	m_Port  = ary.GetAt(2);
	m_User  = ary.GetAt(3);
	m_Pass  = ary.GetAt(4);
	m_Term  = ary.GetAt(5);
	m_InUse = ary.GetVal(6);
	m_strFileName = ary.GetAt(7);
	switch(ary.GetVal(8)) {
	case FileNew:  m_nShellCommand = FileNew;  break;
	case FileOpen: m_nShellCommand = FileOpen; break;
	}
}

//////////////////////////////////////////////////////////////////////
// CRLoginApp

BEGIN_MESSAGE_MAP(CRLoginApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CRLoginApp::OnAppAbout)
	// 標準のファイル基本ドキュメント コマンド
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// CRLoginApp コンストラクション

CRLoginApp::CRLoginApp()
{
	// TODO: この位置に構築用コードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。
	m_NextSock = 0;
}


// 唯一の CRLoginApp オブジェクトです。

CRLoginApp theApp;

// CRLoginApp 初期化

BOOL CRLoginApp::InitInstance()
{
	//TODO: call AfxInitRichEdit2() to initialize richedit2 library.
	// アプリケーション マニフェストが visual スタイルを有効にするために、
	// ComCtl32.dll Version 6 以降の使用を指定する場合は、
	// Windows XP に InitCommonControlsEx() が必要です。さもなければ、ウィンドウ作成はすべて失敗します。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
	// これを設定します。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

#ifdef	WINSOCK11
	if ( !AfxSocketInit() ) {
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
#else
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD( 2, 2 );
	if ( WSAStartup( wVersionRequested, &wsaData ) != 0 ) {
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
#endif

	// 標準初期化
	// これらの機能を使わずに最終的な実行可能ファイルの
	// サイズを縮小したい場合は、以下から不要な初期化
	// ルーチンを削除してください。
	// 設定が格納されているレジストリ キーを変更します。
	// TODO: 会社名または組織名などの適切な文字列に
	// この文字列を変更してください。
	SetRegistryKey(_T("Culti"));
	LoadStdProfileSettings(4);  // 標準の INI ファイルのオプションをロードします (MRU を含む)

	// アプリケーション用のドキュメント テンプレートを登録します。ドキュメント テンプレート
	//  はドキュメント、フレーム ウィンドウとビューを結合するために機能します。
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_RLOGINTYPE,
		RUNTIME_CLASS(CRLoginDoc),
		RUNTIME_CLASS(CChildFrame), // カスタム MDI 子フレーム
		RUNTIME_CLASS(CRLoginView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// メイン MDI フレーム ウィンドウを作成します。
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	// 接尾辞が存在する場合にのみ DragAcceptFiles を呼び出します。
	//  MDI アプリケーションでは、この呼び出しは、m_pMainWnd を設定した直後に発生しなければなりません。
	// ドラッグ/ドロップ オープンを許可します。
	m_pMainWnd->DragAcceptFiles(FALSE);

	// DDE Execute open を使用可能にします。

	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// DDE、file open など標準のシェル コマンドのコマンド ラインを解析します。
	CCommandLineInfoEx cmdInfo;
	ParseCommandLine(cmdInfo);
	m_pCmdInfo = &cmdInfo;

	if ( cmdInfo.m_InUse && InUseCheck() )
		return FALSE;

	::SetWindowLongPtr(m_pMainWnd->GetSafeHwnd(), GWLP_USERDATA, 0x524c4f47);

	// コマンド ラインで指定されたディスパッチ コマンドです。アプリケーションが
	// /RegServer、/Register、/Unregserver または /Unregister で起動された場合、False を返します。
	// if (!ProcessShellCommand(cmdInfo))
	//	return FALSE;

	char tmp[_MAX_DIR];
	if ( _getcwd(tmp, _MAX_DIR) != NULL )
		m_BaseDir = tmp;
	else {
		int n;
		m_BaseDir = _pgmptr;
		if ( (n = m_BaseDir.ReverseFind('\\')) >= 0 )
			m_BaseDir = m_BaseDir.Left(n);
	}

	// メイン ウィンドウが初期化されたので、表示と更新を行います。
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	switch(cmdInfo.m_nShellCommand) {
	case CCommandLineInfo::FileNew:
		OnFileNew();
		break;
	case CCommandLineInfo::FileOpen:
		if ( OpenDocumentFile(cmdInfo.m_strFileName) == NULL )
			OnFileNew();
		break;
	}
	m_pCmdInfo = NULL;

	return TRUE;
}

void CRLoginApp::OpenProcsCmd(CCommandLineInfoEx *pCmdInfo)
{
	m_pCmdInfo = pCmdInfo;
	switch(pCmdInfo->m_nShellCommand) {
	case CCommandLineInfo::FileNew:
		OnFileNew();
		break;
	case CCommandLineInfo::FileOpen:
		if ( OpenDocumentFile(pCmdInfo->m_strFileName) == NULL )
			OnFileNew();
		break;
	}
	m_pCmdInfo = NULL;
}
BOOL CALLBACK RLoginEnumFunc(HWND hwnd, LPARAM lParam)
{
	CWnd wnd;
	CRLoginApp *pApp = (CRLoginApp *)lParam;
	TCHAR title[1024];
	CString cmdLine;

	::GetWindowText(hwnd, title, 1024);

	if ( _tcsncmp(title, "RLogin", 6) == 0 && ::GetWindowLongPtr(hwnd, GWLP_USERDATA) == 0x524c4f47 ) {
		pApp->m_pCmdInfo->m_InUse = FALSE;
		pApp->m_pCmdInfo->GetString(cmdLine);
		COPYDATASTRUCT copyData;
		copyData.dwData = 0x524c4f31;
		copyData.cbData = cmdLine.GetAllocLength();
		copyData.lpData = cmdLine.GetBuffer();
		::SendMessage(hwnd, WM_COPYDATA, (WPARAM)(pApp->m_pMainWnd->GetSafeHwnd()), (LPARAM)&copyData);
		return FALSE;
	}

	return TRUE;
}
BOOL CRLoginApp::InUseCheck()
{
	::EnumWindows(RLoginEnumFunc, (LPARAM)this);
	return (m_pCmdInfo == NULL || m_pCmdInfo->m_InUse ? FALSE : TRUE);
}
void CRLoginApp::SetSocketIdle(class CExtSocket *pSock)
{
	for ( int n = 0 ; n < m_SocketIdle.GetSize() ; n++ ) {
		if ( m_SocketIdle[n] == (void *)pSock )
			return;
	}
	m_SocketIdle.Add((void *)pSock);
}
void CRLoginApp::DelSocketIdle(class CExtSocket *pSock)
{
	for ( int n = 0 ; n < m_SocketIdle.GetSize() ; n++ ) {
		if ( m_SocketIdle[n] == (void *)pSock ) {
			m_SocketIdle.RemoveAt(n);
			break;
		}
	}
}
void CRLoginApp::GetProfileData(LPCTSTR lpszSection, LPCTSTR lpszEntry, void *lpBuf, int nBufLen, void *lpDef)
{
	LPBYTE pData;
	UINT len;

	if ( GetProfileBinary(lpszSection, lpszEntry, &pData, &len) ) {
		if ( len == (UINT)nBufLen )
			memcpy(lpBuf, pData, nBufLen);
		else
			memcpy(lpBuf, lpDef, nBufLen);
		delete pData;
	} else if ( lpDef != NULL )
		memcpy(lpBuf, lpDef, nBufLen);
}
void CRLoginApp::GetProfileBuffer(LPCTSTR lpszSection, LPCTSTR lpszEntry, CBuffer &Buf)
{
	LPBYTE pData;
	UINT len;

	Buf.Clear();
	if ( GetProfileBinary(lpszSection, lpszEntry, &pData, &len) ) {
		Buf.Apend(pData, len);
		delete pData;
	}
}
void CRLoginApp::GetProfileArray(LPCTSTR lpszSection, CStringArrayExt &array)
{
	int n, mx;
	CString entry;
	
	array.RemoveAll();
	mx = GetProfileInt(lpszSection, "ListMax", 0);
	for ( n = 0 ; n < mx ; n++ ) {
		entry.Format("List%02d", n);
		array.Add(GetProfileString(lpszSection, entry, ""));
	}
}
void CRLoginApp::WriteProfileArray(LPCTSTR lpszSection, CStringArrayExt &array)
{
	int n;
	CString entry;

	for ( n = 0 ; n < array.GetSize() ; n++ ) {
		entry.Format("List%02d", n);
		WriteProfileString(lpszSection, entry, array.GetAt(n));
	}
	WriteProfileInt(lpszSection, "ListMax", n);
}
int CRLoginApp::GetProfileSeqNum(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	CMutexLock Mutex(lpszEntry);
	int num = GetProfileInt(lpszSection, lpszEntry, 0) ;
	WriteProfileInt(lpszSection, lpszEntry, num + 1);
	return num;
}
void CRLoginApp::GetProfileKeys(LPCTSTR lpszSection, CStringArrayExt &array)
{
	array.RemoveAll();
	HKEY hAppKey;
	if ( (hAppKey = GetAppRegistryKey()) == NULL )
		return;
	HKEY hSecKey;
	if ( RegOpenKeyEx(hAppKey, lpszSection, 0, KEY_READ, &hSecKey) == ERROR_SUCCESS && hSecKey != NULL ) {
		TCHAR name[1024];
		DWORD len = 1024;
		for ( int n = 0 ; RegEnumValue(hSecKey, n, name, &len, NULL, NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS ; n++, len = 1024 )
			array.Add(name);
		RegCloseKey(hSecKey);
	}
	RegCloseKey(hAppKey);
}
void CRLoginApp::DelProfileEntry(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	HKEY hSecKey;
	if ( (hSecKey = GetSectionKey(lpszSection)) == NULL )
		return;
	RegDeleteValue(hSecKey, lpszEntry);
	RegCloseKey(hSecKey);
}



// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// ダイアログを実行するためのアプリケーション コマンド
void CRLoginApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CRLoginApp メッセージ ハンドラ

BOOL CRLoginApp::OnIdle(LONG lCount) 
{
	if ( CWinApp::OnIdle(lCount) )
		return TRUE;

	if ( m_NextSock >= m_SocketIdle.GetSize() )
		m_NextSock = 0;

	for ( int n = 0 ; n < m_SocketIdle.GetSize() ; n++ ) {
		CExtSocket *pSock = (CExtSocket *)(m_SocketIdle[m_NextSock]);
		if ( ++m_NextSock >= m_SocketIdle.GetSize() )
			m_NextSock = 0;
		if ( pSock->OnIdle() )
			return TRUE;
	}

	return FALSE;
}

int CRLoginApp::ExitInstance() 
{
	CRYPTO_cleanup_all_ex_data();
#ifndef	WINSOCK11
	WSACleanup();
#endif

#ifdef	_DEBUGXXX
	CBuffer tmp;
	tmp.Report();
#endif
	return CWinApp::ExitInstance();
}

BOOL CRLoginApp::SaveAllModified() 
{
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	if ( pMain != NULL && !pMain->SaveModified() )
		return FALSE;

	return CWinApp::SaveAllModified();
}
void CRLoginApp::SSL_Init()
{
	static BOOL bLoadAlgo = FALSE;

	if ( bLoadAlgo )
		return;
	bLoadAlgo = TRUE;

	SSLeay_add_all_algorithms();
}