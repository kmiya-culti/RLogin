// RLogin.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "RLogin.h"
#include <locale.h>

#ifdef	USE_DWMAPI
#include "DWMApi.h"
#endif

#ifdef	USE_JUMPLIST
#include <Shobjidl.h>
#include <Shlobj.h>
#endif

#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"

#include <direct.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/conf.h>

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
	m_Name.Empty();
	m_InUse = FALSE;
	m_InPane = FALSE;
}
void CCommandLineInfoEx::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	switch(m_PasStat) {
	case 0:
		if ( !bFlag )
			break;
		if ( _tcsicmp(_T("ip"), pszParam) == 0 )
			m_PasStat = 1;
		else if ( _tcsicmp(_T("port"), pszParam) == 0 )
			m_PasStat = 2;
		else if ( _tcsicmp(_T("user"), pszParam) == 0 )
			m_PasStat = 3;
		else if ( _tcsicmp(_T("pass"), pszParam) == 0 )
			m_PasStat = 4;
		else if ( _tcsicmp(_T("term"), pszParam) == 0 )
			m_PasStat = 5;
		else if ( _tcsicmp(_T("entry"), pszParam) == 0 )
			m_PasStat = 6;
		else if ( _tcsicmp(_T("direct"), pszParam) == 0 )
			m_Proto = PROTO_DIRECT;
		else if ( _tcsicmp(_T("login"), pszParam) == 0 ) {
			m_Proto = PROTO_LOGIN;
			m_Port  = _T("login");
		} else if ( _tcsicmp(_T("telnet"), pszParam) == 0 ) {
			m_Proto = PROTO_TELNET;
			m_Port  = _T("telnet");
		} else if ( _tcsicmp(_T("ssh"), pszParam) == 0 ) {
			m_Proto = PROTO_SSH;
			m_Port  = _T("ssh");
		} else if ( _tcsicmp(_T("inuse"), pszParam) == 0 )
			m_InUse = TRUE;
		else if ( _tcsicmp(_T("inpane"), pszParam) == 0 )
			m_InPane = TRUE;
		else if ( _tcsicmp(_T("nothing"), pszParam) == 0 )
			m_nShellCommand = FileNothing;
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
	case 6:
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Name = pszParam;
		ParseLast(bLast);
		return;
	}
	if ( !bFlag && ParseUrl(pszParam) ) {
		ParseLast(bLast);
		return;
	}
	CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}
BOOL CCommandLineInfoEx::ParseUrl(const TCHAR* pszParam)
{
	// ssh://username:password@hostname:port/path?arg=value#anchor
	int n;
	CString tmp, str;
	static const struct {
		int		type;
		LPCTSTR	name;
		int		len;
	} proto[] = {
		{ PROTO_LOGIN,		_T("login"),	5	},
		{ PROTO_TELNET,		_T("telnet"),	6	},
		{ PROTO_SSH,		_T("ssh"),		3	},
		{ 0,				NULL,			0	},
	};

	for ( n = 0 ; proto[n].name != NULL ; n++ ) {
		if ( _tcsnicmp(proto[n].name, pszParam, proto[n].len) == 0 )
			break;
	}
	if ( proto[n].name == NULL )
		return FALSE;
	pszParam += proto[n].len;
	if ( _tcsnicmp(_T("://"), pszParam, 3) != 0 )
		return FALSE;
	pszParam += 3;

	m_Proto = proto[n].type;
	m_Port  = proto[n].name;

	tmp = pszParam;

	if ( (n = tmp.Find(_T('@'))) >= 0 ) {
		str = tmp.Left(n);
		tmp.Delete(0, n + 1);

		if ( (n = str.Find(_T(':'))) >= 0 ) {
			m_User = str.Left(n);
			m_Pass = str.Mid(n + 1);
		} else
			m_User = str;
	}

	str = tmp.SpanExcluding(_T("/?#"));

	if ( (n = str.Find(_T(':'))) >= 0 ) {
		m_Addr = str.Left(n);
		m_Port = str.Mid(n + 1);
	} else
		m_Addr = str;

	return TRUE;
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
	ary.Add(m_Name);

	ary.SetString(str);
}
void CCommandLineInfoEx::SetString(LPCTSTR str)
{
	CStringArrayExt ary;

	ary.GetString(str);

	if ( ary.GetSize() <= 9 )
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
	m_Name = ary.GetAt(9);
}

//////////////////////////////////////////////////////////////////////
// CRLoginApp

BEGIN_MESSAGE_MAP(CRLoginApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CRLoginApp::OnAppAbout)
	ON_COMMAND(ID_FILE_PRINT_SETUP, OnFilePrintSetup)
	// 標準のファイル基本ドキュメント コマンド
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	ON_COMMAND(IDM_DISPWINIDX, &CRLoginApp::OnDispwinidx)
END_MESSAGE_MAP()


// CRLoginApp コンストラクション

CRLoginApp::CRLoginApp()
{
	// TODO: この位置に構築用コードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。
	m_NextSock = 0;
	m_pServerEntry = NULL;
#ifdef	USE_DIRECTWRITE
	m_pD2DFactory    = NULL;
	m_pDWriteFactory = NULL;
#endif
}


// 唯一の CRLoginApp オブジェクトです。

CRLoginApp theApp;

#ifdef	USE_DWMAPI
	HMODULE ExDwmApi = NULL;
	BOOL ExDwmEnable = FALSE;
	HRESULT (__stdcall *ExDwmIsCompositionEnabled)(BOOL * pfEnabled) = NULL;
	HRESULT (__stdcall *ExDwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind) = NULL;
	HRESULT (__stdcall *ExDwmExtendFrameIntoClientArea)(HWND hWnd, const MARGINS* pMarInset) = NULL;
#endif

void ExDwmEnableWindow(HWND hWnd, BOOL bEnable)
{
#ifdef	USE_DWMAPI
	if ( ExDwmEnable && ExDwmApi != NULL && ExDwmEnableBlurBehindWindow != NULL && hWnd != NULL ) {
		//Create and populate the BlurBehind structre
		DWM_BLURBEHIND bb = {0};
		MARGINS margin = { -1 };

		//Enable Blur Behind and Blur Region;
		bb.dwFlags = DWM_BB_ENABLE;
		bb.fEnable = bEnable;
		bb.hRgnBlur = NULL;
		bb.fTransitionOnMaximized = false;
		//Enable Blur Behind
		ExDwmEnableBlurBehindWindow(hWnd, &bb);

		//margin.cxLeftWidth = margin.cxRightWidth = 0;
		//margin.cyTopHeight = 40;
		//margin.cyBottomHeight = 0;
		//ExDwmExtendFrameIntoClientArea(hWnd, &margin);
	}
#endif
}

// CRLoginApp 初期化

BOOL CRLoginApp::InitInstance()
{
	setlocale(LC_ALL, "");

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
	
	int n;
	TCHAR tmp[_MAX_PATH];
	CString iniFileName;
	BOOL bInitFile = FALSE;

	if ( _tgetcwd(tmp, _MAX_PATH) != NULL )
		m_BaseDir = tmp;

	GetModuleFileName(NULL, tmp, _MAX_PATH);

	if ( m_BaseDir.IsEmpty() ) {
		m_BaseDir = tmp;
		if ( (n = m_BaseDir.ReverseFind(_T('\\'))) >= 0 )
			m_BaseDir = m_BaseDir.Left(n);
	}

	iniFileName.Format(_T("%s\\%s.ini"), m_BaseDir, m_pszAppName);
	if ( _taccess_s(iniFileName, 6) == 0 ) {
		m_pszProfileName = _tcsdup(iniFileName);
		bInitFile = TRUE;

	} else {
		iniFileName = tmp;
		if ( (n = iniFileName.ReverseFind(_T('.'))) >= 0 )
			iniFileName = iniFileName.Left(n);
		iniFileName += _T(".ini");
		if ( _taccess_s(iniFileName, 6) == 0 ) {
			m_pszProfileName = _tcsdup(iniFileName);
			bInitFile = TRUE;
		}
	}

	// 設定が格納されているレジストリ キーを変更します。
	// TODO: 会社名または組織名などの適切な文字列に
	// この文字列を変更してください。

	if ( bInitFile == FALSE )
		SetRegistryKey(_T("Culti"));

	LoadStdProfileSettings(4);  // 標準の INI ファイルのオプションをロードします (MRU を含む)


#ifdef	USE_DWMAPI
	if ( (ExDwmApi = LoadLibrary(_T("dwmapi.dll"))) != NULL ) {
		ExDwmIsCompositionEnabled      = (HRESULT (__stdcall *)(BOOL* pfEnabled))GetProcAddress(ExDwmApi, "DwmIsCompositionEnabled");
		ExDwmEnableBlurBehindWindow    = (HRESULT (__stdcall *)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind))GetProcAddress(ExDwmApi, "DwmEnableBlurBehindWindow");
		ExDwmExtendFrameIntoClientArea = (HRESULT (__stdcall *)(HWND hWnd, const MARGINS* pMarInset))GetProcAddress(ExDwmApi, "DwmExtendFrameIntoClientArea");

		OSVERSIONINFO VerInfo;
		memset(&VerInfo, 0, sizeof(VerInfo));
		VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);
		GetVersionEx(&VerInfo);

		if ( VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
			 VerInfo.dwMajorVersion == 6 &&						// Vista/Win7/Win8 ?
			 VerInfo.dwMinorVersion <= 1 &&						// Vista=0, Win7=1, Win8=2 ?
			 ExDwmIsCompositionEnabled != NULL )
			ExDwmIsCompositionEnabled(&ExDwmEnable);
	}
#endif

#ifdef	USE_DIRECTWRITE
	if ( SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory)) )
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown **>(&m_pDWriteFactory));
#endif

#ifdef	USE_JUMPLIST
	ICustomDestinationList *pJumpList = NULL;

	// COM の初期化
	if ( SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) ) {
		// インスタンスの作成
		if ( SUCCEEDED(CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pJumpList))) ) {
			// カテゴリの追加
			UINT uMaxSlots;
			IObjectCollection *pObjCol;
	        IShellLink *pSheLink;
			IObjectArray *pObjSlots, *pObjArray;

			if ( SUCCEEDED(pJumpList->BeginList(&uMaxSlots, IID_PPV_ARGS(&pObjSlots))) ) {

				//pJumpList->AppendKnownCategory(KDC_FREQUENT);		// よく使うファイルリスト
				//pJumpList->AppendKnownCategory(KDC_RECENT);		// 最近使ったファイル (Default)
			
				if ( SUCCEEDED(CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&pObjCol))) ) {

					if ( SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSheLink))) ) {
						pSheLink->SetArguments(_T("/inuse"));
						pSheLink->SetDescription(_T("inuse"));
						pSheLink->SetIconLocation(_T("Path"), 0);

						//IPropertyStore *pProStore;
						//if ( SUCCEEDED(pSheLink->QueryInterface(IID_IPropertyStore, (void **)&pProStore)) ) {
						//	pProStore->SetValue(&PKEY_Title, &pv);
						//	pProStore->Release();
						//}

						pObjCol->AddObject(pSheLink);
						pSheLink->Release();
					}

					if ( SUCCEEDED(pObjCol->QueryInterface(IID_PPV_ARGS(&pObjArray))) ) {
						pJumpList->AppendCategory(L"Category", pObjArray);
						pObjArray->Release();
					}
					pObjCol->Release();
				}

				if ( SUCCEEDED(CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&pObjCol))) ) {
					if ( SUCCEEDED(pObjCol->QueryInterface(IID_PPV_ARGS(&pObjArray))) ) {
						pJumpList->AddUserTasks(pObjArray);
						pObjArray->Release();
					}
					pObjCol->Release();
				}

				if ( SUCCEEDED(CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&pObjCol))) ) {
					if ( SUCCEEDED(pObjCol->QueryInterface(IID_PPV_ARGS(&pObjArray))) ) {
						pJumpList->AddUserTasks(pObjArray);
						pObjArray->Release();
					}
					pObjCol->Release();
				}

				// クリーンアップ
				pJumpList->CommitList();
				pObjSlots->Release();
			}

			pJumpList->Release();
		}

		CoUninitialize();
	}
#endif

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

	// デフォルトプリンタの設定
	SetDefaultPrinter();

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
void CRLoginApp::OpenProcsEntry(LPCTSTR entry)
{
	CCommandLineInfoEx cmds;

	if ( entry != NULL && *entry != _T('\0') ) {
		cmds.ParseParam(_T("entry"), TRUE, FALSE);
		cmds.ParseParam(entry, FALSE, FALSE);
	}
	cmds.ParseParam(_T("inpane"), TRUE, FALSE);
	OpenProcsCmd(&cmds);
}
BOOL CALLBACK RLoginEnumFunc(HWND hwnd, LPARAM lParam)
{
	CRLoginApp *pApp = (CRLoginApp *)lParam;
	TCHAR title[1024];
	CString cmdLine;

	::GetWindowText(hwnd, title, 1024);

	if ( _tcsncmp(title, _T("RLogin"), 6) == 0 && ::GetWindowLongPtr(hwnd, GWLP_USERDATA) == 0x524c4f47 ) {
		pApp->m_pCmdInfo->m_InUse = FALSE;
		pApp->m_pCmdInfo->GetString(cmdLine);
		COPYDATASTRUCT copyData;
		copyData.dwData = 0x524c4f31;
		copyData.cbData = cmdLine.GetLength() * sizeof(TCHAR);
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
	ASSERT(pSock->m_Type >= 0 && pSock->m_Type < 10);

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
CString CRLoginApp::GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault)
{
	if ( m_pszRegistryKey != NULL )
		return CWinApp::GetProfileString(lpszSection, lpszEntry, lpszDefault);
	else {
		ASSERT(m_pszProfileName != NULL);
		if (lpszDefault == NULL)
			lpszDefault = _T("");	// don't pass in NULL

		DWORD dw;
		CBuffer work(64 * 1024);
		for ( ; ; ) {
			dw = ::GetPrivateProfileString(lpszSection, lpszEntry, lpszDefault, (LPWSTR)(work.GetPtr()), work.m_Max / sizeof(TCHAR), m_pszProfileName);
			if ( dw < (work.m_Max / sizeof(TCHAR) - 1) ) {
				work.m_Len = dw * sizeof(TCHAR);
				break;
			}
			work.ReAlloc(work.m_Max * 2);
		}
		return (LPCTSTR)work;
	}
}
void CRLoginApp::GetProfileData(LPCTSTR lpszSection, LPCTSTR lpszEntry, void *lpBuf, int nBufLen, void *lpDef)
{
	LPBYTE pData;
	UINT len;

	if ( GetProfileBinary(lpszSection, lpszEntry, &pData, &len) ) {
		if ( len == (UINT)nBufLen )
			memcpy(lpBuf, pData, nBufLen);
		else if ( lpDef != NULL )
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
void CRLoginApp::GetProfileArray(LPCTSTR lpszSection, CStringArrayExt &stra)
{
	int n, mx;
	CString entry;
	
	stra.RemoveAll();
	mx = GetProfileInt(lpszSection, _T("ListMax"), 0);
	for ( n = 0 ; n < mx ; n++ ) {
		entry.Format(_T("List%02d"), n);
		stra.Add(GetProfileString(lpszSection, entry, _T("")));
	}
}
void CRLoginApp::WriteProfileArray(LPCTSTR lpszSection, CStringArrayExt &stra)
{
	int n;
	CString entry;

	for ( n = 0 ; n < stra.GetSize() ; n++ ) {
		entry.Format(_T("List%02d"), n);
		WriteProfileString(lpszSection, entry, stra.GetAt(n));
	}
	WriteProfileInt(lpszSection, _T("ListMax"), n);
}
int CRLoginApp::GetProfileSeqNum(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	CMutexLock Mutex(lpszEntry);
	int num = GetProfileInt(lpszSection, lpszEntry, 0) ;
	WriteProfileInt(lpszSection, lpszEntry, num + 1);
	return num;
}
void CRLoginApp::GetProfileKeys(LPCTSTR lpszSection, CStringArrayExt &stra)
{
	stra.RemoveAll();
	if ( m_pszRegistryKey != NULL ) {
		HKEY hAppKey;
		if ( (hAppKey = GetAppRegistryKey()) == NULL )
			return;
		HKEY hSecKey;
		if ( RegOpenKeyEx(hAppKey, lpszSection, 0, KEY_READ, &hSecKey) == ERROR_SUCCESS && hSecKey != NULL ) {
			TCHAR name[1024];
			DWORD len = 1024;
			for ( int n = 0 ; RegEnumValue(hSecKey, n, name, &len, NULL, NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS ; n++, len = 1024 )
				stra.Add(name);
			RegCloseKey(hSecKey);
		}
		RegCloseKey(hAppKey);

	} else {
		DWORD dw, sz = 64 * 1024;
		CString key;
		TCHAR *p, *work = new TCHAR[sz];
		for ( ; ; ) {
			dw = GetPrivateProfileSection(lpszSection, work, sz, m_pszProfileName);
			if ( dw < (sz - 2) )
				break;
			sz *= 2;
			delete work;
			work = new TCHAR[sz];
		}
		for ( p = work ; *p != _T('\0') ; ) {
			key.Empty();
			while ( *p != _T('\0') && *p != _T('=') )
				key += *(p++);
			if ( !key.IsEmpty() )
				stra.Add(key);
			while ( *(p++) != _T('\0') )
				;
		}
		delete work;
	}
}
void CRLoginApp::DelProfileEntry(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	if ( m_pszRegistryKey != NULL ) {
		HKEY hSecKey;
		if ( (hSecKey = GetSectionKey(lpszSection)) == NULL )
			return;
		RegDeleteValue(hSecKey, lpszEntry);
		RegCloseKey(hSecKey);

	} else {
		WritePrivateProfileString(lpszSection, lpszEntry, NULL, m_pszProfileName);
	}
}

void CRLoginApp::RegisterShellProtocol(LPCTSTR pSection, LPCTSTR pOption)
{
	HKEY hKey[4];
	DWORD val;
	CString strTemp;
	CString strPathName;
	CString oldDefine;
	CBuffer buf;
	CRegKey reg;

	AfxGetModuleShortFileName(AfxGetInstanceHandle(), strPathName);

	CString strOpenCommandLine;
	CString strDefaultIconCommandLine;

	strDefaultIconCommandLine.Format(_T("%s,%d"), strPathName, 1);
	strOpenCommandLine.Format(_T("%s %s %%1"), strPathName, pOption);

	//	HKEY_CLASSES_ROOT or HKEY_CURRENT_USER\Software\Classes
	//
	//	[HKEY_CLASSES_ROOT\ssh]
	//	@ = "URL: ssh Protocol"
	//	BrowserFlags = dword:00000008
	//	EditFlags = dword:00000002
	//	URL Protocol = ""

	//	[HKEY_CLASSES_ROOT\ssh\DefaultIcon]
	//	@ = "RLogin.exe,1"

	//	[HKEY_CLASSES_ROOT\ssh\shell]
	//	[HKEY_CLASSES_ROOT\ssh\shell\open]
	//	[HKEY_CLASSES_ROOT\ssh\shell\open\command]
	//	@ = "RLogin.exe /term xterm /inuse %1"

	strTemp.Format(_T("Software\\Classes\\%s"), pSection);

	if ( reg.Open(HKEY_CURRENT_USER, strTemp) == ERROR_SUCCESS ) {
		ULONG len = 0;
		if ( reg.QueryBinaryValue(_T("OldDefine"), NULL, &len) == ERROR_SUCCESS )
			reg.QueryBinaryValue(_T("OldDefine"), buf.PutSpc(len), &len);
		else
			RegisterSave(HKEY_CURRENT_USER, strTemp, buf);
		buf.Clear();
		reg.Close();
	}

	if( AfxRegCreateKey(HKEY_CURRENT_USER, strTemp, &(hKey[0])) == ERROR_SUCCESS ) {

		strTemp.Format(_T("URL: %s Protocol"), pSection);
		RegSetValueEx(hKey[0], _T(""), 0, REG_SZ, (const LPBYTE)(LPCTSTR)strTemp, (strTemp.GetLength() + 1) * sizeof(TCHAR));
		val = 8;
		RegSetValueEx(hKey[0], _T("BrowserFlags"), 0, REG_DWORD, (const LPBYTE)(&val), sizeof(val));
		val = 2;
		RegSetValueEx(hKey[0], _T("EditFlags"), 0, REG_DWORD, (const LPBYTE)(&val), sizeof(val));
		strTemp = "";
		RegSetValueEx(hKey[0], _T("URL Protocol"), 0, REG_SZ, (const LPBYTE)(LPCTSTR)strTemp, (strTemp.GetLength() + 1) * sizeof(TCHAR));

		RegSetValueEx(hKey[0], _T("OldDefine"), 0, REG_BINARY, (const LPBYTE)buf.GetPtr(), buf.GetSize());

		if( AfxRegCreateKey(hKey[0], _T("DefaultIcon"), &(hKey[1])) == ERROR_SUCCESS ) {
			RegSetValueEx(hKey[1], _T(""), 0, REG_SZ, (const LPBYTE)(LPCTSTR)strDefaultIconCommandLine, (strDefaultIconCommandLine.GetLength() + 1) * sizeof(TCHAR));
			RegCloseKey(hKey[1]);
		}

		if( AfxRegCreateKey(hKey[0], _T("shell"), &(hKey[1])) == ERROR_SUCCESS ) {
			if( AfxRegCreateKey(hKey[1], _T("open"), &(hKey[2])) == ERROR_SUCCESS ) {
				if( AfxRegCreateKey(hKey[2], _T("command"), &(hKey[3])) == ERROR_SUCCESS ) {
					RegSetValueEx(hKey[3], _T(""), 0, REG_SZ, (const LPBYTE)(LPCTSTR)strOpenCommandLine, (strOpenCommandLine.GetLength() + 1) * sizeof(TCHAR));
					RegCloseKey(hKey[3]);
				}
				RegCloseKey(hKey[2]);
			}
			RegCloseKey(hKey[1]);
		}

		RegCloseKey(hKey[0]);
	}
}
void CRLoginApp::RegisterDelete(HKEY hKey, LPCTSTR pSection, LPCTSTR pKey)
{
	CRegKey reg;

	if ( reg.Open(hKey, pSection) != ERROR_SUCCESS )
		return;

	reg.RecurseDeleteKey(pKey);
	reg.Close();
}
void CRLoginApp::RegisterSave(HKEY hKey, LPCTSTR pSection, CBuffer &buf)
{
	int n;
	CRegKey reg;
	DWORD type, len;
	TCHAR *work;
	CStringArray menba;

	if ( reg.Open(hKey, pSection) != ERROR_SUCCESS )
		return;

	len = 0;
	reg.QueryValue(_T(""), &type, NULL, &len);
	if ( len < 1024 )
		len = 1024;
	work = new TCHAR[len];
	reg.QueryValue(_T(""), &type, work, &len);

	buf.Put32Bit(type);
	buf.PutBuf((LPBYTE)work, len);

	for ( n = 0 ; ; n++ ) {
		len = 1020;
		if ( reg.EnumKey(n, work, &len) != ERROR_SUCCESS )
			break;
		work[len] = _T('\0');
		menba.Add(work);
	}

	delete work;

	buf.Put32Bit(menba.GetSize());
	for ( n = 0 ; n < menba.GetSize() ; n++ ) {
		buf.PutStr(TstrToMbs(menba[n]));
		RegisterSave(reg.m_hKey, menba[n], buf);
	}

	reg.Close();
}
void CRLoginApp::RegisterLoad(HKEY hKey, LPCTSTR pSection, CBuffer &buf)
{
	int n;
	CRegKey reg;
	DWORD type, len;
	CString name;
	CBuffer work;
	CStringA mbs;

	if ( buf.GetSize() < 4 )
		return;

	if ( reg.Create(hKey, pSection) != ERROR_SUCCESS )
		return;

	type = buf.Get32Bit();
	buf.GetBuf(&work);
	reg.SetValue(_T(""), type, work.GetPtr(), work.GetSize());

	len = buf.Get32Bit();
	for ( n = 0 ; n < len ; n++ ) {
		buf.GetStr(mbs);
		name = mbs;
		RegisterLoad(reg.m_hKey, name, buf);
	}

	reg.Close();
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

int mt_idle();
int ScriptIdle();

BOOL CRLoginApp::OnIdle(LONG lCount) 
{
	BOOL rt = FALSE;

	if ( CWinApp::OnIdle(lCount) )
		return TRUE;

	for ( int n = 0 ; n < m_SocketIdle.GetSize() ; n++ ) {
		//if ( m_NextSock >= m_SocketIdle.GetSize() )
		//	m_NextSock = 0;
		//CExtSocket *pSock = (CExtSocket *)(m_SocketIdle[m_NextSock]);

		//if ( ++m_NextSock >= m_SocketIdle.GetSize() )
		//	m_NextSock = 0;

		CExtSocket *pSock = (CExtSocket *)(m_SocketIdle[n]);

		ASSERT(pSock->m_Type >= 0 && pSock->m_Type < 10 );

		if ( pSock->OnIdle() )
			rt = TRUE;
	}

	if ( mt_idle() )
		rt = TRUE;

	if ( ScriptIdle() )
		rt = TRUE;

	return rt;
}

int CRLoginApp::ExitInstance() 
{
	CONF_modules_finish();
	CONF_modules_unload(1);
	CONF_modules_free();
	EVP_cleanup();
	ENGINE_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();

#ifndef	WINSOCK11
	WSACleanup();
#endif

#ifdef	USE_DWMAPI
	if ( ExDwmApi != NULL )
		FreeLibrary(ExDwmApi);
#endif

#ifdef	USE_DIRECTWRITE
	if ( m_pDWriteFactory != NULL )
		m_pDWriteFactory->Release();
	if ( m_pD2DFactory != NULL )
		m_pD2DFactory->Release();
#endif

#ifndef	FIXWCHAR
	AllWCharAllocFree();
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
	SSLeay_add_ssl_algorithms();
}

void CRLoginApp::SetDefaultPrinter()
{
	int n;
	int wDefault;
	CString Driver, Device, Output;
	LPDEVNAMES lpDevNames;
	LPDEVMODE lpDevMode;
	HANDLE hDevNames;
	HANDLE hDevMode;
	LPTSTR ptr;
	UINT sz;
	LPBYTE pByte;

	if ( !GetProfileBinary(_T("PrintSetup"), _T("DevMode"), &pByte, &sz) )
		return;

	if ( sz < sizeof(DEVMODE) )
		goto ERRRET;

	lpDevMode = (LPDEVMODE)pByte;

	if ( sz < (UINT)(lpDevMode->dmSize + lpDevMode->dmDriverExtra) )
		goto ERRRET;

	Driver = GetProfileString(_T("PrintSetup"), _T("Driver"), _T(""));
	Device = GetProfileString(_T("PrintSetup"), _T("Device"), _T(""));
	Output = GetProfileString(_T("PrintSetup"), _T("Output"), _T(""));
	wDefault = GetProfileInt(_T("PrintSetup"),  _T("Default"), 0);

	if ( Driver.IsEmpty() || Device.IsEmpty() || Output.IsEmpty() )
		goto ERRRET;

	if ( (hDevNames = ::GlobalAlloc(GHND, sizeof(DEVNAMES) + (Driver.GetLength() + Device.GetLength() + Output.GetLength() + 3) * sizeof(TCHAR))) == NULL )
		goto ERRRET;

	if ( (hDevMode = ::GlobalAlloc(GHND, sz)) == NULL ) {
		::GlobalFree(hDevNames);
		goto ERRRET;
	}

	if ( (lpDevNames = (LPDEVNAMES)::GlobalLock(hDevNames)) == NULL ) {
		::GlobalFree(hDevNames);
		::GlobalFree(hDevMode);
		goto ERRRET;
	}

	if ( (lpDevMode = (LPDEVMODE)::GlobalLock(hDevMode)) == NULL ) {
		::GlobalUnlock(hDevNames);
		::GlobalFree(hDevNames);
		::GlobalFree(hDevMode);
		goto ERRRET;
	}

	lpDevNames->wDefault = wDefault;
	n   = sizeof(DEVNAMES) / sizeof(TCHAR);
	ptr = (LPTSTR)lpDevNames;

	lpDevNames->wDriverOffset = n;
	_tcscpy(ptr + n, Driver);
	n += (Driver.GetLength() + 1);

	lpDevNames->wDeviceOffset = n;
	_tcscpy(ptr + n, Device);
	n += (Device.GetLength() + 1);

	lpDevNames->wOutputOffset = n;
	_tcscpy(ptr + n, Output);

	memcpy(lpDevMode, pByte, sz);

	::GlobalUnlock(hDevMode);
	::GlobalUnlock(hDevNames);

	SelectPrinter(hDevNames, hDevMode);

ERRRET:
	delete pByte;
}

void CRLoginApp::OnFilePrintSetup()
{
	CPrintDialog pd(TRUE);

	if ( DoPrintDialog(&pd) != IDOK )
		return;

	LPDEVNAMES lpDevNames;
	LPDEVMODE lpDevMode;

	if ( m_hDevNames == NULL || (lpDevNames = (LPDEVNAMES)::GlobalLock(m_hDevNames)) == NULL )
		return;

	if ( m_hDevMode == NULL || (lpDevMode = (LPDEVMODE)::GlobalLock(m_hDevMode)) == NULL ) {
		::GlobalUnlock(m_hDevNames);
		return;
	}

	WriteProfileString(_T("PrintSetup"), _T("Driver"), (LPCTSTR)lpDevNames + lpDevNames->wDriverOffset);
	WriteProfileString(_T("PrintSetup"), _T("Device"), (LPCTSTR)lpDevNames + lpDevNames->wDeviceOffset);
	WriteProfileString(_T("PrintSetup"), _T("Output"), (LPCTSTR)lpDevNames + lpDevNames->wOutputOffset);
	WriteProfileInt(_T("PrintSetup"), _T("Default"), lpDevNames->wDefault);
	WriteProfileBinary(_T("PrintSetup"), _T("DevMode"), (LPBYTE)lpDevMode, lpDevMode->dmSize + lpDevMode->dmDriverExtra);

	::GlobalUnlock(m_hDevMode);
	::GlobalUnlock(m_hDevNames);
}

void CRLoginApp::OnDispwinidx()
{
	POSITION pos = GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
			pDoc->UpdateAllViews(NULL, UPDATE_DISPINDEX, NULL);
		}
	}
}
