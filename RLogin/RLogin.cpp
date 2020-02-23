// RLogin.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "RLogin.h"
#include <locale.h>
#include "EditDlg.h"

#ifdef	USE_DWMAPI
#include "DWMApi.h"
#endif

#ifdef	USE_JUMPLIST
#include <Shobjidl.h>
#include <Shlobj.h>
#include <propvarutil.h>
#include <propsys.h>
#include <propkey.h>
#endif

#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "Script.h"
#include "ResTransDlg.h"

#include <direct.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/conf.h>

#if	OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <internal/cryptlib.h>
#endif

#include "afxcmn.h"

#ifdef	USE_SAPI
#include <sphelper.h>
#endif

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
	m_Idkey.Empty();
	m_Path.Empty();
	m_InUse = INUSE_NONE;
	m_InPane = FALSE;
	m_AfterId = (-1);
	m_ScreenX = (-1);
	m_ScreenY = (-1);
	m_ScreenW = (-1);
	m_ScreenH = (-1);
	m_EventName.Empty();
	m_Title.Empty();
	m_Script.Empty();
	m_ReqDlg = FALSE;
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
		else if ( _tcsicmp(_T("after"), pszParam) == 0 )
			m_PasStat = 7;
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
			m_InUse = INUSE_ACTWIN;
		else if ( _tcsicmp(_T("inusea"), pszParam) == 0 )
			m_InUse = INUSE_ALLWIN;
		else if ( _tcsicmp(_T("inpane"), pszParam) == 0 )
			m_InPane = TRUE;
		else if ( _tcsicmp(_T("nothing"), pszParam) == 0 )
			m_nShellCommand = FileNothing;
		else if ( _tcsicmp(_T("sx"), pszParam) == 0 )
			m_PasStat = 8;
		else if ( _tcsicmp(_T("sy"), pszParam) == 0 )
			m_PasStat = 9;
		else if ( _tcsicmp(_T("cx"), pszParam) == 0 )
			m_PasStat = 10;
		else if ( _tcsicmp(_T("cy"), pszParam) == 0 )
			m_PasStat = 11;
		else if ( _tcsicmp(_T("event"), pszParam) == 0 )
			m_PasStat = 12;
		else if ( _tcsicmp(_T("title"), pszParam) == 0 )
			m_PasStat = 13;
		else if ( _tcsicmp(_T("script"), pszParam) == 0 )
			m_PasStat = 14;
		else if ( _tcsicmp(_T("req"), pszParam) == 0 )
			m_ReqDlg = TRUE;
		else if ( _tcsicmp(_T("idkey"), pszParam) == 0 )
			m_PasStat = 15;
		else if ( _tcsicmp(_T("file"), pszParam) == 0 )
			m_PasStat = 16;
		else
			break;
		ParseLast(bLast);
		return;
	case 1:			// ip
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Addr = pszParam;
		ParseLast(bLast);
		return;
	case 2:			// port
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Port = pszParam;
		ParseLast(bLast);
		return;
	case 3:			// user
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_User = pszParam;
		ParseLast(bLast);
		return;
	case 4:			// pass
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Pass = pszParam;
		ParseLast(bLast);
		return;
	case 5:			// term
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Term = pszParam;
		ParseLast(bLast);
		return;
	case 6:			// entry
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Name = pszParam;
		ParseLast(bLast);
		return;

	case 7:			// after
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_AfterId = _tstoi(pszParam);
		ParseLast(bLast);
		return;

	case 8:			// sx
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_ScreenX = _tstoi(pszParam);
		ParseLast(bLast);
		return;
	case 9:			// sy
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_ScreenY = _tstoi(pszParam);
		ParseLast(bLast);
		return;
	case 10:		// cx
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_ScreenW = _tstoi(pszParam);
		ParseLast(bLast);
		return;
	case 11:		// cy
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_ScreenH = _tstoi(pszParam);
		ParseLast(bLast);
		return;

	case 12:		// event
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_EventName = pszParam;
		ParseLast(bLast);
		return;
	case 13:		// title
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Title = pszParam;
		ParseLast(bLast);
		return;
	case 14:		// script
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Script = pszParam;
		ParseLast(bLast);
		return;
	case 15:		// idkey
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Idkey = pszParam;
		ParseLast(bLast);
		return;
	case 16:		// file
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Path = pszParam;
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
	int n, i;
	CString tmp, str, argv;
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
		str = tmp.Left(n);				// username:password
		tmp.Delete(0, n + 1);

		if ( (n = str.Find(_T(':'))) >= 0 ) {
			m_User = str.Left(n);
			m_Pass = str.Mid(n + 1);
		} else
			m_User = str;
	}

	if ( (n = tmp.Find(_T('#'))) >= 0 ) {
		str = (LPCTSTR)tmp + n + 1;		// anchor
		tmp.Delete(n, tmp.GetLength() - n);
	}
	if ( (n = tmp.Find(_T('?'))) >= 0 ) {
		argv = (LPCTSTR)tmp + n + 1;		// arg=value
		tmp.Delete(n, tmp.GetLength() - n);
	}
	if ( (n = tmp.Find(_T('/'))) >= 0 ) {
		m_Path = (LPCTSTR)tmp + n + 1;		// path
		tmp.Delete(n, tmp.GetLength() - n);
	}

	if ( (i = tmp.Find(_T('['))) >= 0 && (n = tmp.Find(_T(']'))) > i ) {
		m_Addr = tmp.Mid(i + 1, n - i - 1);
		tmp.Delete(0, n + 1);
		if ( (n = tmp.Find(_T(':'))) >= 0 )
			m_Port = tmp.Mid(n + 1);	// [::1]:port
	} else if ( (n = tmp.Find(_T(':'))) >= 0 ) {
		m_Addr = tmp.Left(n);			// hostname:port
		m_Port = tmp.Mid(n + 1);
	} else
		m_Addr = tmp;

	if ( !argv.IsEmpty() ) {
		CStringIndex index;
		index.GetQueryString(argv);
		for ( n = 0 ; n < index.GetSize() ; n++ ) {
			if ( index[n].IsEmpty() ) {
				ParseParam(index[n].GetIndex(), TRUE, FALSE);
			} else {
				ParseParam(index[n].GetIndex(), TRUE, FALSE);
				ParseParam(index[n], FALSE, FALSE);
			}
		}
	}

	return TRUE;
}
void CCommandLineInfoEx::GetString(CString &str)
{
	CString tmp;

	str.Empty();

	if ( !m_strFileName.IsEmpty() ) {
		tmp.Format(_T("\"%s\""), m_strFileName);
		str += tmp;
	}

	if ( !m_Name.IsEmpty() ) {
		tmp.Format(_T(" /entry \"%s\""), m_Name);
		str += tmp;
	}

	if ( m_Proto != (-1) ) {
		switch(m_Proto) {
		case PROTO_DIRECT:  str += _T(" /direct"); break;
		case PROTO_LOGIN:	str += _T(" /login"); break;
		case PROTO_TELNET:	str += _T(" /telnet"); break;
		case PROTO_SSH:		str += _T(" /ssh"); break;
		case PROTO_COMPORT:	str += _T(" /com"); break;
		case PROTO_PIPE:	str += _T(" /pipe"); break;
		}
	}

	if ( !m_Addr.IsEmpty() ) {
		tmp.Format(_T(" /ip %s"), ShellEscape(m_Addr));
		str += tmp;
	}

	if ( !m_Port.IsEmpty() ) {
		tmp.Format(_T(" /port %s"), ShellEscape(m_Port));
		str += tmp;
	}

	if ( !m_User.IsEmpty() ) {
		tmp.Format(_T(" /user %s"), ShellEscape(m_User));
		str += tmp;
	}

	if ( !m_Pass.IsEmpty() ) {
		tmp.Format(_T(" /pass %s"), ShellEscape(m_Pass));
		str += tmp;
	}

	if ( !m_Term.IsEmpty() ) {
		tmp.Format(_T(" /term %s"), ShellEscape(m_Term));
		str += tmp;
	}

	if ( !m_Title.IsEmpty() ) {
		tmp.Format(_T(" /title %s"), ShellEscape(m_Title));
		str += tmp;
	}

	if ( !m_Script.IsEmpty() ) {
		tmp.Format(_T(" /script %s"), ShellEscape(m_Script));
		str += tmp;
	}

	if ( m_AfterId != (-1) ) {
		tmp.Format(_T(" /after %d"), m_AfterId);
		str += tmp;
	}

	if ( m_ScreenX != (-1) ) {
		tmp.Format(_T(" /sx %d"), m_ScreenX);
		str += tmp;
	}

	if ( m_ScreenY != (-1) ) {
		tmp.Format(_T(" /sy %d"), m_ScreenY);
		str += tmp;
	}

	if ( m_ScreenW != (-1) ) {
		tmp.Format(_T(" /cx %d"), m_ScreenW);
		str += tmp;
	}

	if ( m_ScreenH != (-1) ) {
		tmp.Format(_T(" /cy %d"), m_ScreenH);
		str += tmp;
	}

	if ( m_ReqDlg )
		str += _T(" /req");

	//if ( m_InUse == INUSE_ACTWIN )
	//	str += _T(" /inuse");
	//else if ( m_InUse == INUSE_ALLWIN )
	//	str += _T(" /inusea");

	if ( m_InPane )
		str += _T(" /inpne");

	if ( !m_Idkey.IsEmpty() ) {
		tmp.Format(_T(" /idkey %s"), ShellEscape(m_Idkey));
		str += tmp;
	}
}
void CCommandLineInfoEx::SetString(LPCTSTR str)
{
	CStringArrayExt param;

	param.GetCmds(str);

	if ( param.GetSize() <= 0 )
		return;

	for ( int n = 0 ; n < param.GetSize() ; n++ ) {
		LPCTSTR pParam = param[n];
		BOOL bFlag = FALSE;
		BOOL bLast = ((n + 1) == param.GetSize());
		if ( pParam[0] == _T('-') || pParam[0] == _T('/') ) {
			bFlag = TRUE;
			pParam++;
		}
		ParseParam(pParam, bFlag, bLast);
	}
}
LPCTSTR CCommandLineInfoEx::ShellEscape(LPCTSTR str)
{
	static CString work;

	work = _T('"');
	while ( *str != _T('\0') ) {
		if ( *str == _T('"') )
			work += _T('\\');
		work += *(str++);
	}
	work += _T('"');

	return work;
}

//////////////////////////////////////////////////////////////////////
// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogExt
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

// 実装
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMClickSyslink(NMHDR *pNMHDR, LRESULT *pResult);
};

CAboutDlg::CAboutDlg() : CDialogExt(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
	CWnd *pWnd;
	CString form, text;

	CDialogExt::OnInitDialog();

	if ( (pWnd = GetDlgItem(IDC_VERSIONSTR)) != NULL ) {
		pWnd->GetWindowText(form);
#ifdef	_M_X64
		text.Format(form, _T("x64"));
#else
		text.Format(form, _T("x32"));
#endif
		pWnd->SetWindowText(text);
	}

	return TRUE;
}

void CAboutDlg::OnNMClickSyslink(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMLINK pNMLink = (PNMLINK)pNMHDR;
	ShellExecute(m_hWnd, NULL, pNMLink->item.szUrl, NULL, NULL, SW_NORMAL);
	*pResult = 0;
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogExt)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK1, &CAboutDlg::OnNMClickSyslink)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK2, &CAboutDlg::OnNMClickSyslink)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK3, &CAboutDlg::OnNMClickSyslink)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CRLoginApp

BEGIN_MESSAGE_MAP(CRLoginApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CRLoginApp::OnAppAbout)
	ON_COMMAND(ID_FILE_PRINT_SETUP, OnFilePrintSetup)
	// 標準のファイル基本ドキュメント コマンド
	ON_COMMAND(ID_FILE_NEW, &CRLoginApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CRLoginApp::OnFileOpen)
	ON_COMMAND(IDM_DISPWINIDX, &CRLoginApp::OnDispwinidx)
	ON_COMMAND(IDM_DIALOGFONT, &CRLoginApp::OnDialogfont)
	ON_COMMAND(IDM_LOOKCAST, &CRLoginApp::OnLookcast)
	ON_UPDATE_COMMAND_UI(IDM_LOOKCAST, &CRLoginApp::OnUpdateLookcast)
	ON_COMMAND(IDM_OTHERCAST, &CRLoginApp::OnOthercast)
	ON_UPDATE_COMMAND_UI(IDM_OTHERCAST, &CRLoginApp::OnUpdateOthercast)
	ON_COMMAND(IDM_PASSWORDLOCK, &CRLoginApp::OnPassLock)
	ON_UPDATE_COMMAND_UI(IDM_PASSWORDLOCK, &CRLoginApp::OnUpdatePassLock)
	ON_COMMAND(IDM_SAVERESFILE, &CRLoginApp::OnSaveresfile)
	ON_COMMAND(IDM_CREATEPROFILE, &CRLoginApp::OnCreateprofile)
	ON_UPDATE_COMMAND_UI(IDM_CREATEPROFILE, &CRLoginApp::OnUpdateCreateprofile)
	ON_COMMAND(IDM_SAVEREGFILE, &CRLoginApp::OnSaveregfile)
	ON_UPDATE_COMMAND_UI(IDM_SAVEREGFILE, &CRLoginApp::OnUpdateSaveregfile)
	ON_COMMAND(IDM_REGISTAPP, &CRLoginApp::OnRegistapp)
	ON_UPDATE_COMMAND_UI(IDM_REGISTAPP, &CRLoginApp::OnUpdateRegistapp)
END_MESSAGE_MAP()


// CRLoginApp コンストラクション

CRLoginApp::CRLoginApp()
{
	m_pServerEntry = NULL;
	m_bLookCast = FALSE;
	m_bOtherCast = FALSE;
	m_LocalPass.Empty();

#ifdef	USE_DIRECTWRITE
	m_pD2DFactory    = NULL;
	m_pDWriteFactory = NULL;
	m_pDCRT = NULL;

	for ( int n = 0 ; n < EMOJI_HASH ; n++ )
		m_pEmojiList[n] = NULL;
#endif

#ifdef	USE_SAPI
	m_pVoice = NULL;
#endif

	m_IdleProcCount = 0;
	m_pIdleTop = NULL;
	m_bUseIdle = FALSE;

	m_TempSeqId = 0;
	m_bRegistAppp = FALSE;
	m_MakeKeyMode = MAKEKEY_USERHOST;
}

//////////////////////////////////////////////////////////////////////
// 唯一の CRLoginApp オブジェクトです。

	CRLoginApp theApp;

	BOOL ExDwmEnable = FALSE;
#ifdef	USE_DWMAPI
	HMODULE ExDwmApi = NULL;
	HRESULT (__stdcall *ExDwmIsCompositionEnabled)(BOOL * pfEnabled) = NULL;
	HRESULT (__stdcall *ExDwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind) = NULL;
	HRESULT (__stdcall *ExDwmExtendFrameIntoClientArea)(HWND hWnd, const MARGINS* pMarInset) = NULL;
#endif
	
	HMODULE ExUserApi = NULL;
	BOOL (__stdcall *ExAddClipboardFormatListener)(HWND hwnd) = NULL;
	BOOL (__stdcall *ExRemoveClipboardFormatListener)(HWND hwnd) = NULL;
	UINT (__stdcall *ExGetDpiForSystem)() = NULL;
	HRESULT (__stdcall *ExSetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value) = NULL;
	BOOL (WINAPI *ExIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
	DPI_AWARENESS_CONTEXT (__stdcall *ExSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext) = NULL;
	BOOL (__stdcall *ExEnableNonClientDpiScaling)(HWND hwnd) = NULL;
	UINT GlobalSystemDpi = 96;

#ifdef	USE_RCDLL
	HMODULE ExRcDll = NULL;
#endif

	HMODULE ExShcoreApi = NULL;
	UINT (__stdcall *ExGetDpiForWindow)(HWND hwnd) = NULL;
	HRESULT (__stdcall *ExGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY) = NULL;

#ifdef	USE_DIRECTWRITE
	HMODULE ExD2D1Api = NULL;
    EXTERN_C HRESULT (WINAPI *ExD2D1CreateFactory)(__in D2D1_FACTORY_TYPE factoryType, __in REFIID riid, __in_opt CONST D2D1_FACTORY_OPTIONS *pFactoryOptions, __out void **ppIFactory) = NULL;
	HMODULE ExDWriteApi = NULL;
	EXTERN_C HRESULT (WINAPI *ExDWriteCreateFactory)(__in DWRITE_FACTORY_TYPE factoryType, __in REFIID iid, __out IUnknown **factory) = NULL;
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

#ifdef	USE_OLE
	CLIPFORMAT CF_FILEDESCRIPTOR = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
	CLIPFORMAT CF_FILECONTENTS = RegisterClipboardFormat(CFSTR_FILECONTENTS);
#endif

int ThreadMessageBox(LPCTSTR msg, ...)
{
	LPVOID lpMessageBuffer;
	CString tmp;
	va_list arg;
	DWORD err = ::GetLastError();

	va_start(arg, msg);
	tmp.FormatV(msg, arg);

	if ( err != 0 ) {
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMessageBuffer, 0, NULL);
		tmp += _T("\n\n");
		tmp += (LPTSTR)lpMessageBuffer;
		LocalFree(lpMessageBuffer);
	}

	return ::AfxMessageBox(tmp);
}
void DpiAwareSwitch(BOOL sw, int req)
{
	static BOOL bSwitch = FALSE;

	if ( sw ) {
		if ( ExIsValidDpiAwarenessContext != NULL && ExSetThreadDpiAwarenessContext != NULL ) {
			if ( (req & 001) != 0 && ExIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED) ) {
				ExSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
				bSwitch = TRUE;
			} else if ( (req & 002) != 0 && ExIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ) {
				ExSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
				bSwitch = TRUE;
			} else if ( (req & 004) != 0 && ExIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE) ) {
				ExSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
				bSwitch = TRUE;
			} else if ( (req & 010) != 0 && ExIsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE) ) {
				ExSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
				bSwitch = TRUE;
			}
		}
	} else if ( bSwitch ) {
		ExSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
		bSwitch = FALSE;
	}
}
INT_PTR DpiAwareDoModal(CCommonDialog &dlg, int req)
{
	INT_PTR result;

	// CCommonDialogがDPI_AWARENESS_CONTEXT_PER_MONITOR_AWARでは、DPIチェンジに対応出来ないので変更してみる
	// CFileDialogのbVistaStyleがFALSE(古いタイプ)は、V2でも駄目な模様・・・

	DpiAwareSwitch(TRUE, req);

	result = dlg.DoModal();

	DpiAwareSwitch(FALSE);

	return result;
}

//////////////////////////////////////////////////////////////////////

// CRLoginApp 初期化

#ifdef	USE_JUMPLIST
IShellLink *CRLoginApp::MakeShellLink(LPCTSTR pEntryName)
{
	IShellLink *pSheLink;
	TCHAR szExePath[MAX_PATH];
	CString param;
 
	GetModuleFileName(NULL, szExePath, _countof(szExePath));

	if ( FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSheLink))) )
		return NULL;

	param.Format(_T("/entry \"%s\" /inuse"), pEntryName);

	pSheLink->SetPath(szExePath);
	pSheLink->SetArguments(param);
	pSheLink->SetIconLocation(szExePath, 1);
	pSheLink->SetDescription(pEntryName);

	IPropertyStore *pProStore;
	PROPVARIANT pv;

	if ( FAILED(pSheLink->QueryInterface(IID_PPV_ARGS(&pProStore))) ) {
		pSheLink->Release();
		return NULL;
	}

	InitPropVariantFromString(pEntryName, &pv);
	pProStore->SetValue(PKEY_Title, pv);
	pProStore->Commit();

	PropVariantClear(&pv);
	pProStore->Release();

	return pSheLink;
}
void CRLoginApp::CreateJumpList(CServerEntryTab *pEntry)
{
	ICustomDestinationList *pJumpList;

	if ( FAILED(CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pJumpList))) )
		return;

	UINT uMaxSlots;
	IObjectArray *pRemovedList;

#ifdef	_DEBUG
	SetAppID(_T("Culti.RLogin.Debug"));
#else
	SetAppID(_T("Culti.RLogin.2"));
#endif

	//SetCurrentProcessExplicitAppUserModelID(m_pszAppID);
	pJumpList->SetAppID(m_pszAppID);

	pJumpList->DeleteList(m_pszAppID);
	//pJumpList->AppendKnownCategory(KDC_FREQUENT);
	pJumpList->AppendKnownCategory(KDC_RECENT);

	if ( FAILED(pJumpList->BeginList(&uMaxSlots, IID_PPV_ARGS(&pRemovedList))) ) {
		pJumpList->Release();
		return;
	}

	if ( FAILED(pRemovedList->GetCount(&uMaxSlots)) )
		uMaxSlots = 0;

	IObjectCollection *pObjCol;
	IObjectArray *pObjArray;
			
	if ( SUCCEEDED(CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pObjCol))) ) {
		IShellLink *pShellLink;

		for ( int n = 0 ; n < pEntry->GetSize() ; n++ ) {
			if ( (pShellLink = MakeShellLink(pEntry->GetAt(n).m_EntryName)) != NULL ) {
				pObjCol->AddObject(pShellLink);
				pShellLink->Release();
			}
		}

		if ( SUCCEEDED(pObjCol->QueryInterface(IID_PPV_ARGS(&pObjArray))) ) {
			pJumpList->AppendCategory(_T("Entry"), pObjArray);
			pObjArray->Release();
		}

		pObjCol->Release();
	}

	//if ( SUCCEEDED(CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pObjCol))) ) {
	//	if ( SUCCEEDED(pObjCol->QueryInterface(IID_PPV_ARGS(&pObjArray))) ) {
	//		pJumpList->AddUserTasks(pObjArray);
	//		pObjArray->Release();
	//	}
	//	pObjCol->Release();
	//}

	pJumpList->CommitList();

	pRemovedList->Release();
	pJumpList->Release();
}
#endif
	
#ifdef	USE_SAPI
void CRLoginApp::Speek(LPCTSTR str)
{
	if ( m_pVoice != NULL )
		m_pVoice->Speak(TstrToUni(str), SPF_ASYNC, NULL);
}
#endif

void CRLoginApp::SSL_Init()
{
	static BOOL bLoadAlgo = FALSE;

	if ( bLoadAlgo )
		return;
	bLoadAlgo = TRUE;

#if	OPENSSL_VERSION_NUMBER < 0x10100000L
	SSLeay_add_all_algorithms();
#else
	OPENSSL_init_ssl(0, NULL);
#endif
}

BOOL CRLoginApp::InitLocalPass()
{
	CIdKey key;
	CString passok, tmp;
	CEditDlg dlg;

	passok = GetProfileString(_T("RLoginApp"), _T("LocalPass"), _T(""));

	if ( passok.IsEmpty() )
		return TRUE;

	if ( IsOpenRLogin() )
		return TRUE;
		
	dlg.m_bPassword = TRUE;
	dlg.m_Edit    = _T("");
	dlg.m_WinText = _T("RLogin Password Lock");
	dlg.m_Title.LoadString(IDS_PASSLOCKOPENMSG);
	
	for ( int retry = 0 ; retry < 3 ; retry++ ) {
		m_LocalPass.Empty();

		if ( retry == 1 )
			dlg.m_Title += CStringLoad(IDS_PASSLOCKRETRYMSG);

		if ( dlg.DoModal() != IDOK ) {
			if ( ::AfxMessageBox(IDS_PASSLOCKDELMSG, MB_ICONQUESTION | MB_YESNO) == IDYES ) {
				WriteProfileString(_T("RLoginApp"), _T("LocalPass"), _T(""));
				return TRUE;
			}
			return FALSE;
		}

		key.Digest(m_LocalPass, dlg.m_Edit);
		key.DecryptStr(tmp, passok);

		if ( tmp.Compare(_T("01234567")) == 0 )
			return TRUE;
	}

	return FALSE;
}
BOOL CRLoginApp::IsWinVerCheck(int ver, int op)
{
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = (BYTE)(ver >> 8);
	osvi.dwMinorVersion = (BYTE)(ver);

    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);

    return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
}
BOOL CRLoginApp::GetExtFilePath(LPCTSTR ext, CString &path)
{
	path.Format(_T("%s\\%s%s"), m_BaseDir, m_pszAppName, ext);
	if ( _taccess_s(path, 06) == 0 )
		return TRUE;

	path.Format(_T("%s\\%s%s"), m_ExecDir, m_pszAppName, ext);
	if ( _taccess_s(path, 06) == 0 )
		return TRUE;

	path.Empty();
	return FALSE;
}
BOOL CRLoginApp::IsDirectory(LPCTSTR dir)
{
	CFileStatus st;

	if ( CFile::GetStatus(dir, st) && (st.m_attribute & CFile::directory) != 0 )
		return TRUE;

	return FALSE;
}
BOOL CRLoginApp::CreateDesktopShortcut(LPCTSTR entry)
{
	BOOL rt = FALSE;
	IShellLinkW  *pShellLink = NULL;
	IPersistFile *pPersistFile = NULL;
	WCHAR desktopPath[MAX_PATH];
	CStringW srcParam, linkPath;

#ifndef	USE_COMINIT
	if ( FAILED(CoInitialize(NULL) )
		return FALSE;
#endif
	
	if ( FAILED(SHGetSpecialFolderPath(NULL, desktopPath, CSIDL_DESKTOPDIRECTORY, FALSE)) )
		goto ENDOF;

	srcParam.Format(L"/entry \"%s\" /inuse", TstrToUni(entry));
	linkPath.Format(L"%s\\%s.lnk", desktopPath, TstrToUni(entry));
	
	if ( FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink))) )
		goto ENDOF;

	if ( FAILED(pShellLink->SetPath(TstrToUni(m_PathName))) )
		goto ENDOF;

	if ( FAILED(pShellLink->SetArguments(TstrToUni(srcParam))) )
		goto ENDOF;

	if ( FAILED(pShellLink->SetWorkingDirectory(TstrToUni(m_BaseDir))) )
		goto ENDOF;

	if ( FAILED(pShellLink->SetIconLocation(TstrToUni(m_PathName), 1)) )
		goto ENDOF;

	if ( FAILED(pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile))) )
		goto ENDOF;

	if ( SUCCEEDED(pPersistFile->Save(linkPath, TRUE)) )
		rt = TRUE;

ENDOF:
	if ( pPersistFile != NULL )
		pPersistFile->Release();

	if ( pShellLink != NULL )
		pShellLink->Release();

#ifndef	USE_COMINIT
	CoUninitialize();
#endif

	return rt;
}

//////////////////////////////////////////////////////////////////////

BOOL CRLoginApp::InitInstance()
{
	// LoadLibrary Search Path
	HMODULE hModule;
	if ((hModule = GetModuleHandle(_T("kernel32.dll"))) != NULL) {
		BOOL (WINAPI *pSetSearchPathMode)(DWORD);
		BOOL (WINAPI *pSetDefaultDllDirectories)(DWORD); 
		BOOL (WINAPI *pSetDllDirectoryA)(LPCSTR);

		if ( (pSetSearchPathMode = (BOOL (WINAPI *)(DWORD))GetProcAddress(hModule, "SetSearchPathMode")) != NULL )
			pSetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);
		else if ( (pSetDefaultDllDirectories = (BOOL (WINAPI *)(DWORD))GetProcAddress(hModule, "SetDefaultDllDirectories")) != NULL )
			pSetDefaultDllDirectories(0x00000800);	// LOAD_LIBRARY_SEARCH_SYSTEM32
		else if ( (pSetDllDirectoryA = (BOOL (WINAPI *)(LPCSTR))GetProcAddress(hModule, "SetDllDirectoryA")) != NULL )
			pSetDllDirectoryA("");
	}

	// デフォルトのロケールを設定 strftimeなどで必要
	setlocale(LC_ALL, "");

	//TODO: call AfxInitRichEdit2() to initialize richedit2 library.
	// アプリケーション マニフェストが visual スタイルを有効にするために、
	// ComCtl32.dll Version 6 以降の使用を指定する場合は、
	// Windows XP に InitCommonControlsEx() が必要です。さもなければ、ウィンドウ作成はすべて失敗します。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、これを設定します。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

#ifdef	USE_OLE
	//  OLEの初期化
	if ( !AfxOleInit() )
		return FALSE;
#endif

#ifdef	USE_COMINIT
	// COMライブラリ初期化
	if ( FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) ) {
		AfxMessageBox(_T("Com Library Init Error"));
		return FALSE;
	}
#endif

	// WINSOCK2.2の初期化
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD( 2, 2 );
	if ( WSAStartup( wVersionRequested, &wsaData ) != 0 ) {
		AfxMessageBox(IDS_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// 親のInitInstance呼び出し
	CWinApp::InitInstance();

	// 作業ディレクトリとプログラム名から実行ディレクトリを設定
	int n;
	TCHAR PathTemp[_MAX_PATH];
	CString iniFileName;
	BOOL bInitFile = FALSE;

	if ( _tgetcwd(PathTemp, _MAX_PATH) != NULL )
		m_BaseDir = PathTemp;

	GetModuleFileName(NULL, PathTemp, _MAX_PATH);
	m_PathName = PathTemp;

	if ( (n = m_PathName.ReverseFind(_T('\\'))) >= 0 )
		m_ExecDir = m_PathName.Left(n);

	if ( m_BaseDir.IsEmpty() )
		m_BaseDir = m_ExecDir;

	// ユーザープロファイルの検索
	if ( GetExtFilePath(_T(".ini"), iniFileName) ) {
		free((void*)m_pszProfileName);
		m_pszProfileName = _tcsdup(iniFileName);
		bInitFile = TRUE;
	}

	// 設定が格納されているレジストリ キーを変更します。
	if ( bInitFile == FALSE )
		SetRegistryKey(_T("Culti"));

	// 標準の INI ファイルのオプションをロードします (MRU を含む)
	LoadStdProfileSettings(4);

	// TempPathの設定
	GetTempPath(MAX_PATH, PathTemp);
	m_TempDirBase.Format(_T("%sRLogin%d\\"), PathTemp, GetCurrentThreadId());

#ifdef	USE_RCDLL
	// リソースDLL読み込み
	CString rcDllName;
	if ( GetExtFilePath(_T("_rc.dll"), rcDllName) && (ExRcDll = LoadLibrary(rcDllName)) != NULL )
		AfxSetResourceHandle(ExRcDll);
#endif

	// リソースデータベースの設定
	// m_ResDataBase.InitRessource();
	CString rcFileName;
	if ( GetExtFilePath(_T("_rc.txt"), rcFileName) ) {
		if ( !m_ResDataBase.LoadFile(rcFileName) )
			::AfxMessageBox(_T("Can't Load Resources File"));
	}

	// デフォルトツールバーイメージからBitmapリソースを作成
	InitToolBarBitmap(MAKEINTRESOURCE(IDR_MAINFRAME), IDB_BITMAP1);
	InitToolBarBitmap(MAKEINTRESOURCE(IDR_SFTPTOOL),  IDB_BITMAP5);

	// レジストリ保存のリソースデータベース読み込み
	CBuffer buf;
	GetProfileBuffer(_T("RLoginApp"), _T("ResDataBase"), buf);
	if ( buf.GetSize() > (sizeof(DWORD) * 5) )
		m_ResDataBase.Serialize(FALSE, buf);

#ifdef	USE_DWMAPI
	// ガラス効果のDLLを設定
	if ( (ExDwmApi = LoadLibrary(_T("dwmapi.dll"))) != NULL ) {
		ExDwmIsCompositionEnabled      = (HRESULT (__stdcall *)(BOOL* pfEnabled))GetProcAddress(ExDwmApi, "DwmIsCompositionEnabled");
		ExDwmEnableBlurBehindWindow    = (HRESULT (__stdcall *)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind))GetProcAddress(ExDwmApi, "DwmEnableBlurBehindWindow");
		ExDwmExtendFrameIntoClientArea = (HRESULT (__stdcall *)(HWND hWnd, const MARGINS* pMarInset))GetProcAddress(ExDwmApi, "DwmExtendFrameIntoClientArea");

		if ( (IsWinVerCheck(_WIN32_WINNT_VISTA) || IsWinVerCheck(_WIN32_WINNT_WIN7)) && ExDwmIsCompositionEnabled != NULL )
			ExDwmIsCompositionEnabled(&ExDwmEnable);
	}
#endif

	// クリップボードチェインライブラリを取得
	if ( (ExUserApi = LoadLibrary(_T("User32.dll"))) != NULL ) {
		ExAddClipboardFormatListener    = (BOOL (__stdcall *)(HWND hwnd))GetProcAddress(ExUserApi, "AddClipboardFormatListener");
		ExRemoveClipboardFormatListener = (BOOL (__stdcall *)(HWND hwnd))GetProcAddress(ExUserApi, "RemoveClipboardFormatListener");
		ExGetDpiForSystem               = (UINT (__stdcall *)())GetProcAddress(ExUserApi, "GetDpiForSystem");
		ExGetDpiForWindow               = (UINT (__stdcall *)(HWND hwnd))GetProcAddress(ExUserApi, "GetDpiForWindow");
		ExEnableNonClientDpiScaling     = (BOOL (__stdcall *)(HWND hwnd))GetProcAddress(ExUserApi, "EnableNonClientDpiScaling");
		ExIsValidDpiAwarenessContext    = (BOOL (__stdcall *)(DPI_AWARENESS_CONTEXT dpiContext))GetProcAddress(ExUserApi, "IsValidDpiAwarenessContext");
		ExSetThreadDpiAwarenessContext  = (DPI_AWARENESS_CONTEXT (__stdcall *)(DPI_AWARENESS_CONTEXT dpiContext))GetProcAddress(ExUserApi, "SetThreadDpiAwarenessContext");
	}

	// モニターDPIのライブラリを取得
	if ( (ExShcoreApi = LoadLibrary(_T("Shcore.dll"))) != NULL ) {
		ExGetDpiForMonitor             = (HRESULT (__stdcall *)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY))GetProcAddress(ExShcoreApi, "GetDpiForMonitor");
		ExSetProcessDpiAwareness       = (HRESULT (__stdcall *)(PROCESS_DPI_AWARENESS value))GetProcAddress(ExShcoreApi, "SetProcessDpiAwareness");
	}

	// System DPIの取得（念の為に０以下をチェック)
	if ( ExGetDpiForSystem != NULL ) {
		if ( (GlobalSystemDpi = ExGetDpiForSystem()) <= 0 )
			GlobalSystemDpi = 96;
	}

#ifdef	USE_DIRECTWRITE
	// DirectWriteを試す
	if ( IsWinVerCheck(_WIN32_WINNT_WINBLUE, VER_GREATER_EQUAL) ) {
		if ( (ExD2D1Api = LoadLibrary(_T("d2d1.dll"))) != NULL && (ExDWriteApi = LoadLibrary(_T("dwrite.dll"))) != NULL ) {
			ExD2D1CreateFactory = (HRESULT (WINAPI *)(__in D2D1_FACTORY_TYPE factoryType, __in REFIID riid, __in_opt CONST D2D1_FACTORY_OPTIONS *pFactoryOptions, __out void **ppIFactory))GetProcAddress(ExD2D1Api, "D2D1CreateFactory");
			ExDWriteCreateFactory = (HRESULT (WINAPI *)(__in DWRITE_FACTORY_TYPE factoryType, __in REFIID iid, __out IUnknown **factory))GetProcAddress(ExDWriteApi, "DWriteCreateFactory");

			if ( ExD2D1CreateFactory != NULL && ExDWriteCreateFactory != NULL && SUCCEEDED(ExD2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), NULL, reinterpret_cast<void **>(&m_pD2DFactory))) )
				ExDWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown **>(&m_pDWriteFactory));

			m_EmojiFontName = GetProfileString(_T("RLoginApp"), _T("EmojiFontName"), _T("Segoe UI emoji"));
		}
	}

	m_EmojiImageDir.Format(_T("%s\\emoji"), m_BaseDir);
	if ( !IsDirectory(m_EmojiImageDir) ) {
		m_EmojiImageDir.Format(_T("%s\\emoji"), m_ExecDir);
		if ( !IsDirectory(m_EmojiImageDir) ) {
			m_EmojiImageDir = GetProfileString(_T("RLoginApp"), _T("EmojiImageDir"), _T(""));
			if ( !m_EmojiImageDir.IsEmpty() ) {
				if ( !IsDirectory(m_EmojiImageDir) )
					m_EmojiImageDir.Empty();
			}
		}
	}
#endif

#ifdef	USE_SAPI
	// 音声合成の初期化
	if ( FAILED(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&m_pVoice)) )
		m_pVoice = NULL;
#endif

	// レジストリに保存するパスワードの暗号キーを選択
	m_MakeKeyMode = GetProfileInt(_T("RLoginApp"), _T("MakeKeyMode"), MAKEKEY_USERHOST);

	// アプリケーション用のドキュメント テンプレートを登録します。ドキュメント テンプレート
	//  はドキュメント、フレーム ウィンドウとビューを結合するために機能します。
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_RLOGINTYPE,
		RUNTIME_CLASS(CRLoginDoc),
		RUNTIME_CLASS(CChildFrame), // カスタム MDI 子フレーム
		RUNTIME_CLASS(CRLoginView));

	if ( !pDocTemplate )
		return FALSE;

	// メニューをリソースデータベースに置き換え
	DestroyMenu(pDocTemplate->m_hMenuShared);
	LoadResMenu(MAKEINTRESOURCE(IDR_RLOGINTYPE), pDocTemplate->m_hMenuShared);

	AddDocTemplate(pDocTemplate);

	// DDE、file open など標準のシェル コマンドのコマンド ラインを解析します。
	CCommandLineInfoEx cmdInfo;
	ParseCommandLine(cmdInfo);
	m_pCmdInfo = &cmdInfo;

	// inuseオプションで別プロセスを見つけたら終了
	if ( cmdInfo.m_InUse != INUSE_NONE && InUseCheck(cmdInfo.m_InUse == INUSE_ALLWIN ? TRUE : FALSE) )
		return FALSE;
	
	// メイン MDI フレーム ウィンドウを作成します。
	CMainFrame* pMainFrame = new CMainFrame;

	// コマンドライン指定のスクリーン座標
	pMainFrame->m_ScreenX = cmdInfo.m_ScreenX;
	pMainFrame->m_ScreenY = cmdInfo.m_ScreenY;
	pMainFrame->m_ScreenW = cmdInfo.m_ScreenW;
	pMainFrame->m_ScreenH = cmdInfo.m_ScreenH;

	// AfxGetMainWndがCMainFrameの作成中に使えるように
	m_pMainWnd = pMainFrame;

	if ( !pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME) ) {
		m_pMainWnd = NULL;
		delete pMainFrame;
		return FALSE;
	}

	// 接尾辞が存在する場合にのみ DragAcceptFiles を呼び出します。
	//  MDI アプリケーションでは、この呼び出しは、m_pMainWnd を設定した直後に発生しなければなりません。
	// ドラッグ/ドロップ オープンを許可します。
	m_pMainWnd->DragAcceptFiles(FALSE);

	// DDE Execute open を使用可能にします。
	EnableShellOpen();

	m_bRegistAppp = GetProfileInt(_T("RLoginApp"), _T("RegistAppp"),  FALSE);

	if ( m_bRegistAppp ) {
		// 管理者でないとレジストリを構築できないので削除
		//RegisterShellFileTypes(TRUE);
		RegisterShellFileEntry();
	}

	// デフォルトプリンタの設定
	SetDefaultPrinter();

	// コマンド ラインで指定されたディスパッチ コマンドです。アプリケーションが
	// /RegServer、/Register、/Unregserver または /Unregister で起動された場合、False を返します。
	if ( cmdInfo.m_nShellCommand != CCommandLineInfo::FileNew && cmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen && !ProcessShellCommand(cmdInfo) )
		return FALSE;

	// メイン ウィンドウが初期化されたので、表示と更新を行います。
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	// パスワードロックの確認
	if ( !InitLocalPass() )
		return FALSE;

	// プログラム識別子を設定
	::SetWindowLongPtr(m_pMainWnd->GetSafeHwnd(), GWLP_USERDATA, 0x524c4f47);

	// レジストリベースのサーバーエントリーの読み込み
	pMainFrame->m_ServerEntryTab.Serialize(FALSE);
	pMainFrame->QuickBarInit();

	// プログラムバージョンチェック
	pMainFrame->VersionCheck();

#ifdef	USE_JUMPLIST
	// ジャンプリスト初期化
	if ( IsWinVerCheck(_WIN32_WINNT_WIN7, VER_GREATER_EQUAL) )
		CreateJumpList(&(pMainFrame->m_ServerEntryTab));
#endif

	// レジストリからオプション復帰
	m_bLookCast  = GetProfileInt(_T("RLoginApp"), _T("LookCast"),  FALSE);
	m_bOtherCast = GetProfileInt(_T("RLoginApp"), _T("OtherCast"), FALSE);

#ifdef	USE_KEYMACGLOBAL
	m_KeyMacGlobal.m_bGlobal = TRUE;
	m_KeyMacGlobal.Serialize(FALSE);	// init
#endif

	CScript::ScriptSystemInit(m_PathName, m_pszAppName, m_BaseDir);

	// コマンドラインイベントをリセット
	if ( !cmdInfo.m_EventName.IsEmpty() ) {
		HANDLE hEvent;
		if ( (hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, cmdInfo.m_EventName)) != NULL ) {
			SetEvent(hEvent);
			CloseHandle(hEvent);
		}
	}

	// 新規 or ファイルで接続
	switch(cmdInfo.m_nShellCommand) {
	case CCommandLineInfo::FileNew:
		OnFileNew();
		break;
	case CCommandLineInfo::FileOpen:
		OpenDocumentFile(cmdInfo.m_strFileName);
		break;
	}
	m_pCmdInfo = NULL;

	return TRUE;
}

LPCTSTR CRLoginApp::GetTempDir(BOOL bSeqId)
{
	if ( GetFileAttributes(m_TempDirBase) == (-1) )
		CreateDirectory(m_TempDirBase, NULL);

	if ( !bSeqId )
		return m_TempDirBase;

	m_TempDirPath.Format(_T("%sTemp%04d\\"), m_TempDirBase, m_TempSeqId++);
	CreateDirectory(m_TempDirPath, NULL);

	return m_TempDirPath;
}

int CRLoginApp::ExitInstance() 
{
	// Free Handle or Library
	rand_buf(NULL, 0);

#if	OPENSSL_VERSION_NUMBER >= 0x1010100fL
	CONF_modules_finish();
	CONF_modules_unload(1);
	ERR_remove_state(0);
#elif	OPENSSL_VERSION_NUMBER >= 0x10100000L
	CONF_modules_finish();
	CONF_modules_unload(1);
	ERR_remove_state(0);
	crypto_cleanup_all_ex_data_int();
#else
	CONF_modules_finish();
	CONF_modules_unload(1);
	CONF_modules_free();
	EVP_cleanup();
	ENGINE_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();
#endif

	WSACleanup();

#ifdef	USE_DWMAPI
	if ( ExDwmApi != NULL )
		FreeLibrary(ExDwmApi);
#endif

	if ( ExShcoreApi != NULL )
		FreeLibrary(ExShcoreApi);

	if ( ExUserApi != NULL )
		FreeLibrary(ExUserApi);

#ifdef	USE_RCDLL
	if ( ExRcDll != NULL )
		FreeLibrary(ExRcDll);
#endif

#ifdef	USE_DIRECTWRITE
	if ( m_pDCRT != NULL )
		m_pDCRT->Release();

	if ( m_pDWriteFactory != NULL )
		m_pDWriteFactory->Release();

	if ( m_pD2DFactory != NULL )
		m_pD2DFactory->Release();

	for ( int n = 0 ; n < EMOJI_HASH ; n++ ) {
		CEmojiImage *pEmoji;
		while ( (pEmoji = m_pEmojiList[n]) != NULL ) {
			m_pEmojiList[n] = pEmoji->m_pNext;
			delete pEmoji;
		}
	}

	if ( ExD2D1Api != NULL )
		FreeLibrary(ExD2D1Api);

	if ( ExDWriteApi != NULL )
		FreeLibrary(ExDWriteApi);
#endif

#ifdef	USE_SAPI
	if ( m_pVoice != NULL )
		m_pVoice->Release();
#endif

#ifdef	USE_COMINIT
	CoUninitialize();
#endif

	CSFtp::LocalDelete(m_TempDirBase);

	return CWinApp::ExitInstance();
}

//////////////////////////////////////////////////////////////////////

void CRLoginApp::OpenProcsCmd(CCommandLineInfoEx *pCmdInfo)
{
	CCommandLineInfoEx *pOldCmdInfo = m_pCmdInfo;

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
	m_pCmdInfo = pOldCmdInfo;
}
void CRLoginApp::OpenCommandEntry(LPCTSTR entry)
{
	CCommandLineInfoEx cmds;

	cmds.ParseParam(_T("Entry"), TRUE, FALSE);
	cmds.ParseParam(entry, FALSE, TRUE);
	OpenProcsCmd(&cmds);
}
void CRLoginApp::OpenCommandLine(LPCTSTR str)
{
	CStringArrayExt param;
	CCommandLineInfoEx cmds;

	param.GetCmds(str);

	if ( param.GetSize() <= 0 )
		return;

	for ( int n = 0 ; n < param.GetSize() ; n++ ) {
		LPCTSTR pParam = param[n];
		BOOL bFlag = FALSE;
		BOOL bLast = ((n + 1) == param.GetSize());
		if ( pParam[0] == _T('-') || pParam[0] == _T('/') ) {
			bFlag = TRUE;
			pParam++;
		}
		cmds.ParseParam(pParam, bFlag, bLast);
	}

	OpenProcsCmd(&cmds);
}
BOOL CRLoginApp::CheckDocument(class CRLoginDoc *pDoc)
{
	POSITION pos = GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			if ( (CRLoginDoc *)pDocTemp->GetNextDoc(dpos) == pDoc )
				return TRUE;
		}
	}
	return FALSE;
}
BOOL CRLoginApp::IsRLoginWnd(HWND hWnd)
{
	TCHAR title[256];

	if ( hWnd == NULL || ::GetWindowLongPtr(hWnd, GWLP_USERDATA) != 0x524c4f47 )
		return FALSE;

	::GetWindowText(hWnd, title, 256);

	if ( _tcsncmp(title, _T("RLogin"), 6) != 0 )
		return FALSE;

	return TRUE;
}
HWND CRLoginApp::GetRLoginFromPoint(CPoint point)
{
	HWND hParent;
	HWND hWnd = ::WindowFromPoint(point);
	HWND hDeskTop = GetDesktopWindow();

	while ( hWnd != NULL && (hParent = ::GetParent(hWnd)) != NULL && hParent != hDeskTop )
		hWnd = hParent;

	if ( hWnd == NULL || hWnd == hDeskTop || !IsRLoginWnd(hWnd) )
		return NULL;

	return hWnd;
}
BOOL CRLoginApp::OnEntryData(COPYDATASTRUCT *pCopyData)
{
	int sx, sy;
	CBuffer tmp;
	CServerEntry entry;
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();

	tmp.Apend((LPBYTE)(pCopyData->lpData), pCopyData->cbData);

	sx = tmp.Get32Bit();
	sy = tmp.Get32Bit();

	if ( !entry.GetBuffer(tmp) )
		return FALSE;

	entry.m_DocType = DOCTYPE_ENTRYFILE;
	m_pServerEntry = &entry;

	pMain->SetActivePoint(CPoint(sx, sy));
	OnFileNew();

	return FALSE;	// not contine
}
void CRLoginApp::OpenRLogin(class CRLoginDoc *pDoc, CPoint *pPoint)
{
	CBuffer tmp;
	CString param;
	BOOL bOpen = FALSE;
	BOOL bCmdLine = TRUE;
	COPYDATASTRUCT copyData;
	HWND hWnd = GetRLoginFromPoint(*pPoint);

	switch(pDoc->m_ServerEntry.m_DocType) {
	case DOCTYPE_NONE:
		return;
	case DOCTYPE_REGISTORY:
	case DOCTYPE_SESSION:
		break;
	case DOCTYPE_ENTRYFILE:
	case DOCTYPE_MULTIFILE:
		if ( hWnd == NULL )
			hWnd = NewProcsMainWnd(pPoint, &bOpen);
		bCmdLine = FALSE;
		break;
	}

	if ( hWnd != NULL ) {
		// send other process

		if ( bCmdLine ) {
			// send cmdline

			param.Format(_T("%s /sx %d /sy %d /inuse"), pDoc->m_CmdLine, pPoint->x, pPoint->y);

			copyData.dwData = 0x524c4f31;
			copyData.cbData = (param.GetLength() + 1) * sizeof(TCHAR);
			copyData.lpData = param.GetBuffer();

		} else {
			// send pack data

			tmp.Put32Bit(pPoint->x);
			tmp.Put32Bit(pPoint->y);
			pDoc->SetEntryProBuffer();
			pDoc->m_ServerEntry.SetBuffer(tmp);

			copyData.dwData = 0x524c4f37;
			copyData.cbData = tmp.GetSize();
			copyData.lpData = tmp.GetPtr();
		}

		::SetForegroundWindow(hWnd);
		::SendMessage(hWnd, WM_COPYDATA, (WPARAM)(AfxGetMainWnd()->GetSafeHwnd()), (LPARAM)&copyData);

	} else if ( !bOpen ) {
		// open RLogin

		param.Format(_T("%s /sx %d /sy %d"), pDoc->m_CmdLine, pPoint->x, pPoint->y);
		ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), NULL, m_PathName, param, m_BaseDir, SW_NORMAL);
	}
}

//////////////////////////////////////////////////////////////////////

static BOOL CALLBACK RLoginEnumFindMainWnd(HWND hwnd, LPARAM lParam)
{
    DWORD pid;
	CRLoginApp *pThis = (CRLoginApp *)lParam;

    ::GetWindowThreadProcessId(hwnd, &pid);

	if ( pid != pThis->m_FindProcsId )
		return TRUE;

	if ( !CRLoginApp::IsRLoginWnd(hwnd) )
		return TRUE;

	pThis->m_FindProcsHwnd = hwnd;

	return FALSE;	// not continue
}
HWND CRLoginApp::FindProcsMainWnd(DWORD ProcsId)
{
	m_FindProcsId = ProcsId;
	m_FindProcsHwnd = NULL;

	::EnumWindows(RLoginEnumFindMainWnd, (LPARAM)this);

	return m_FindProcsHwnd;
}
HWND CRLoginApp::NewProcsMainWnd(CPoint *pPoint, BOOL *pbOpen)
{
	CString param, tmp;
	STARTUPINFO StatInfo;
	PROCESS_INFORMATION ProcInfo;
	HANDLE hEvent;

	ZeroMemory(&StatInfo, sizeof(StatInfo));
	ZeroMemory(&ProcInfo, sizeof(ProcInfo));

	tmp.Format(_T("RLogin%d"), GetCurrentThreadId());
	if ( (hEvent = CreateEvent(NULL, TRUE, FALSE, tmp)) == NULL )
		return NULL;

	param.Format(_T("%s /nothing /event %s"), m_pszAppName, tmp);

	if ( pPoint != NULL ) {
		tmp.Format(_T(" /sx %d /sy %d"), pPoint->x, pPoint->y);
		param += tmp;
	}

	if ( !CreateProcess(m_PathName, (LPTSTR)(LPCTSTR)param, NULL, NULL, FALSE, 0, NULL, m_BaseDir, &StatInfo, &ProcInfo) )
		return NULL;

	if ( pbOpen != NULL )
		*pbOpen = TRUE;

	// 最大5秒間メインウィンドウが開くのを待つ
	WaitForSingleObject(hEvent, 5000);
	CloseHandle(hEvent);

	return FindProcsMainWnd(ProcInfo.dwProcessId);
}

//////////////////////////////////////////////////////////////////////

static BOOL CALLBACK RLoginEnumFunc(HWND hwnd, LPARAM lParam)
{
	if ( CRLoginApp::IsRLoginWnd(hwnd) )
		return (BOOL)::SendMessage(hwnd, WM_COPYDATA, (WPARAM)(AfxGetMainWnd()->GetSafeHwnd()), lParam);

	return TRUE;
}

BOOL CRLoginApp::OnInUseCheck(COPYDATASTRUCT *pCopyData, BOOL bIcon)
{
	CCommandLineInfoEx cmdInfo;
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();

	if ( (!bIcon && pMain->IsIconic()) || pMain->m_IconShow )
		return TRUE;	// continue

	cmdInfo.SetString((LPCTSTR)(pCopyData->lpData));

	if ( cmdInfo.m_ScreenX != (-1) && cmdInfo.m_ScreenY != (-1) )
		pMain->SetActivePoint(CPoint(cmdInfo.m_ScreenX, cmdInfo.m_ScreenY));

	if (  pMain->IsIconic() )
		pMain->ShowWindow(SW_RESTORE);

	OpenProcsCmd(&cmdInfo);

	pMain->SetForegroundWindow();

	return FALSE;	// not continue
}
BOOL CRLoginApp::InUseCheck(BOOL bIcon)
{
	CString cmdLine;
	COPYDATASTRUCT copyData;

	if ( m_pCmdInfo == NULL )
		return FALSE;

	m_pCmdInfo->m_InUse = INUSE_NONE;
	m_pCmdInfo->GetString(cmdLine);

	copyData.dwData = bIcon ? 0x524c4f39 : 0x524c4f31;
	copyData.cbData = (cmdLine.GetLength() + 1) * sizeof(TCHAR);
	copyData.lpData = cmdLine.GetBuffer();

	AllowSetForegroundWindow(ASFW_ANY);

	return (::EnumWindows(RLoginEnumFunc, (LPARAM)&copyData) ? FALSE : TRUE);
}

BOOL CRLoginApp::OnIsOnlineEntry(COPYDATASTRUCT *pCopyData)
{
	POSITION pos = GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
			if ( pDoc->m_pSock != NULL && pDoc->m_TextRam.IsInitText() && pDoc->m_ServerEntry.m_EntryName.Compare((LPCTSTR)(pCopyData->lpData)) == 0 )
				return FALSE;	// not continue
		}
	}
	return TRUE;	// continue
}
BOOL CRLoginApp::IsOnlineEntry(LPCTSTR entry)
{
	COPYDATASTRUCT copyData;

	copyData.dwData = 0x524c4f32;
	copyData.cbData = (DWORD)(_tcslen(entry) + 1) * sizeof(TCHAR);
	copyData.lpData = (PVOID)entry;

	return (::EnumWindows(RLoginEnumFunc, (LPARAM)&copyData) ? FALSE : TRUE);
}

BOOL CRLoginApp::OnIsOpenRLogin(COPYDATASTRUCT *pCopyData)
{
	LPCTSTR name;
	HANDLE hHandle;
	TCHAR *pData;
	CIdKey key;
	CString tmp, str;

	if ( m_LocalPass.IsEmpty() )
		return TRUE;	// continue

	if ( pCopyData->cbData == 0 )
		return TRUE;	// continue

	if ( (name = ((LPCTSTR)(pCopyData->lpData))) == NULL )
		return TRUE;	// continue

	hHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, name);

	if ( hHandle == NULL || hHandle == INVALID_HANDLE_VALUE )
		return FALSE;

	if ( (pData = (TCHAR *)MapViewOfFile(hHandle, FILE_MAP_WRITE, 0, 0, 0)) == NULL ) {
		CloseHandle(hHandle);
		return TRUE;
	}

	str.Format(_T("TEST%s"), m_LocalPass);
	key.Encrypt(tmp, (LPBYTE)(str.GetBuffer()), ((str.GetLength() + 1) * sizeof(TCHAR)), m_PathName, MAKEKEY_USERHOST);
	_tcsncpy(pData, tmp, 1024);

	UnmapViewOfFile(pData);
	CloseHandle(hHandle);

	return FALSE;	// not continue;
}
BOOL CRLoginApp::IsOpenRLogin()
{
	CString name;
	HANDLE hHandle = NULL;
	TCHAR *pData = NULL;
	BOOL ret = FALSE;
	COPYDATASTRUCT copyData;
	CIdKey key;
	CBuffer tmp;

	name.Format(_T("RLoginDigest%08x"), (unsigned)GetCurrentThreadId());
	hHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,	0, 1024 * sizeof(TCHAR), name);

	if ( hHandle == NULL || hHandle == INVALID_HANDLE_VALUE )
		return FALSE;

	copyData.dwData = 0x524c4f38;
	copyData.cbData = (name.GetLength() + 1) * sizeof(TCHAR);
	copyData.lpData = name.GetBuffer();

	if ( ::EnumWindows(RLoginEnumFunc, (LPARAM)&copyData) )
		goto ENDOF;

	if ( (pData = (TCHAR *)MapViewOfFile(hHandle, FILE_MAP_READ, 0, 0, 0)) == NULL )
		goto ENDOF;

	key.Decrypt(&tmp, pData, m_PathName, MAKEKEY_USERHOST);
	if ( _tcsncmp((LPCTSTR)tmp, _T("TEST"), 4) != 0 )
		goto ENDOF;

	m_LocalPass = ((LPCTSTR)tmp) + 4;
	ret = TRUE;

ENDOF:
	if ( pData != NULL )
		UnmapViewOfFile(pData);

	if ( hHandle != NULL )
		CloseHandle(hHandle);

	return ret;
}

void CRLoginApp::OnUpdateServerEntry(COPYDATASTRUCT *pCopyData)
{
	if ( pCopyData->cbData == sizeof(DWORD) && *((DWORD *)(pCopyData->lpData)) == GetCurrentThreadId() )
		return;

	((CMainFrame *)AfxGetMainWnd())->UpdateServerEntry();
}
void CRLoginApp::UpdateServerEntry()
{
	COPYDATASTRUCT copyData;
	DWORD id = GetCurrentThreadId();

	copyData.dwData = 0x524c4f3a;
	copyData.cbData = sizeof(DWORD);
	copyData.lpData = (PVOID)&id;

	::EnumWindows(RLoginEnumFunc, (LPARAM)&copyData);
}

#ifdef	USE_KEYMACGLOBAL
void CRLoginApp::OnUpdateKeyMac(COPYDATASTRUCT *pCopyData)
{
	CKeyMac tmp;

	if ( pCopyData->cbData == sizeof(DWORD) && *((DWORD *)(pCopyData->lpData)) == GetCurrentThreadId() )
		return;

	if ( m_KeyMacGlobal.m_bUpdate && m_KeyMacGlobal.m_Data.GetSize() > 0 ) {
		tmp = m_KeyMacGlobal.m_Data[0];
		m_KeyMacGlobal.Serialize(FALSE);	// load
		if ( m_KeyMacGlobal.m_Data.GetSize() <= 0 || !(m_KeyMacGlobal.m_Data[0] == tmp) )
			m_KeyMacGlobal.m_bUpdate = FALSE;
	} else
		m_KeyMacGlobal.Serialize(FALSE);	// load
}
void CRLoginApp::UpdateKeyMacGlobal()
{
	COPYDATASTRUCT copyData;
	DWORD id = GetCurrentThreadId();

	m_KeyMacGlobal.Serialize(TRUE);		// save

	copyData.dwData = 0x524c4f33;
	copyData.cbData = sizeof(DWORD);
	copyData.lpData = (PVOID)&id;

	::EnumWindows(RLoginEnumFunc, (LPARAM)&copyData);
}
#endif

void CRLoginApp::OnSendBroadCast(COPYDATASTRUCT *pCopyData)
{
	CBuffer tmp;

	tmp.Apend((LPBYTE)(pCopyData->lpData), pCopyData->cbData);

	POSITION pos = GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);

			if ( pDoc != NULL && !pDoc->m_bCastLock && !pDoc->m_TextRam.IsOptEnable(TO_RLNTBCRECV) && !pDoc->m_TextRam.IsOptEnable(TO_RLGROUPCAST) ) {
				if ( !m_bLookCast || ((CMainFrame *)AfxGetMainWnd())->IsTopLevelDoc(pDoc) )
					pDoc->SendBuffer(tmp);
			}
		}
	}
}
void CRLoginApp::OnSendGroupCast(COPYDATASTRUCT *pCopyData)
{
	CString group;
	CBuffer tmp, buf;

	tmp.Apend((LPBYTE)(pCopyData->lpData), pCopyData->cbData);
	tmp.GetStrT(group);
	tmp.GetBuf(&buf);

	POSITION pos = GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);

			if ( pDoc != NULL && !pDoc->m_bCastLock && !pDoc->m_TextRam.IsOptEnable(TO_RLNTBCRECV) && group.Compare(pDoc->m_TextRam.m_GroupCast) == 0 ) {
				if ( !m_bLookCast || ((CMainFrame *)AfxGetMainWnd())->IsTopLevelDoc(pDoc) )
					pDoc->SendBuffer(buf);
			}
		}
	}
}
void CRLoginApp::SendBroadCast(CBuffer &buf, LPCTSTR pGroup)
{
	CBuffer tmp;
	COPYDATASTRUCT copyData;

	if ( pGroup == NULL || *pGroup == _T('\0') ) {
		copyData.dwData = 0x524c4f34;
		copyData.cbData = buf.GetSize();
		copyData.lpData = buf.GetPtr();
	} else {
		tmp.PutStrT(pGroup);
		tmp.PutBuf(buf.GetPtr(), buf.GetSize());
		copyData.dwData = 0x524c4f36;
		copyData.cbData = tmp.GetSize();
		copyData.lpData = tmp.GetPtr();
	}

	if ( m_bOtherCast )
		::EnumWindows(RLoginEnumFunc, (LPARAM)&copyData);
	else
		AfxGetMainWnd()->SendMessage(WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copyData);
}

void CRLoginApp::OnSendBroadCastMouseWheel(COPYDATASTRUCT *pCopyData)
{
	struct _WheelData {
		UINT nFlags;
		short zDelta;
		CPoint pt;
	} *pWheelData;

	if ( pCopyData->cbData != sizeof(struct _WheelData) )
		return;

	pWheelData = (struct _WheelData *)(pCopyData->lpData);

	POSITION pos = GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
			POSITION vpos = pDoc->GetFirstViewPosition();
			while ( vpos != NULL ) {
				CRLoginView *pView = (CRLoginView *)pDoc->GetNextView(vpos);
//				pView->OnMouseWheel(pWheelData->nFlags, pWheelData->zDelta, pWheelData->pt);
				pView->SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(pWheelData->nFlags, pWheelData->zDelta), MAKELPARAM(pWheelData->pt.x, pWheelData->pt.y)); 
			}
		}
	}
}
void CRLoginApp::SendBroadCastMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	COPYDATASTRUCT copyData;
	struct {
		UINT nFlags;
		short zDelta;
		CPoint pt;
	} WheelData;

	WheelData.nFlags = nFlags;
	WheelData.zDelta = zDelta;
	WheelData.pt = pt;

	copyData.dwData = 0x524c4f35;
	copyData.cbData = sizeof(WheelData);
	copyData.lpData = &WheelData;

	if ( (nFlags & MK_CONTROL) != 0 )
		::EnumWindows(RLoginEnumFunc, (LPARAM)&copyData);
	else
		OnSendBroadCastMouseWheel(&copyData);
}

//////////////////////////////////////////////////////////////////////

CString CRLoginApp::GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault)
{
	if ( m_pszRegistryKey != NULL )
		return CWinApp::GetProfileString(lpszSection, lpszEntry, lpszDefault);
	else {
		ASSERT(m_pszProfileName != NULL);
		if (lpszDefault == NULL)
			lpszDefault = _T("");	// don't pass in NULL

		DWORD dw;
		CBuffer work(64 * 1024 * sizeof(TCHAR));
		for ( ; ; ) {
			dw = ::GetPrivateProfileString(lpszSection, lpszEntry, lpszDefault, (LPTSTR)work.m_Data, work.m_Max / sizeof(TCHAR), m_pszProfileName);
			if ( dw < (work.m_Max  / sizeof(TCHAR) - 1) )
				break;
			work.ReAlloc(work.m_Max * 2);
		}
		return (LPCTSTR)work.m_Data;
	}
}
void CRLoginApp::GetProfileData(LPCTSTR lpszSection, LPCTSTR lpszEntry, void *lpBuf, int nBufLen, void *lpDef)
{
	LPBYTE pData = NULL;
	UINT len = 0;

	if ( GetProfileBinary(lpszSection, lpszEntry, &pData, &len) && pData != NULL && len > 0 && len < (512 * 1024) ) {
		if ( len == (UINT)nBufLen )
			memcpy(lpBuf, pData, nBufLen);
		else if ( lpDef != NULL )
			memcpy(lpBuf, lpDef, nBufLen);
	} else if ( lpDef != NULL )
		memcpy(lpBuf, lpDef, nBufLen);

	if ( pData != NULL )
		delete [] pData;
}
void CRLoginApp::GetProfileBuffer(LPCTSTR lpszSection, LPCTSTR lpszEntry, CBuffer &Buf)
{
	LPBYTE pData = NULL;
	UINT len = 0;

	Buf.Clear();

	if ( GetProfileBinary(lpszSection, lpszEntry, &pData, &len) && pData != NULL && len > 0 && len < (512 * 1024) )
		Buf.Apend(pData, len);

	if ( pData != NULL )
		delete [] pData;
}
void CRLoginApp::GetProfileStringArray(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringArrayExt &stra)
{
	CBuffer buf;
	CString str;

	GetProfileBuffer(lpszSection, lpszEntry, buf);

	stra.RemoveAll();
	while ( buf.GetSize() > 4 ) {
		buf.GetStrT(str);
		stra.Add(str);
	}
}
void CRLoginApp::WriteProfileStringArray(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringArrayExt &stra)
{
	int n;
	CBuffer buf;

	for ( n = 0 ; n < stra.GetSize() ; n++ )
		buf.PutStrT(stra[n]);

	WriteProfileBinary(lpszSection, lpszEntry, buf.GetPtr(), buf.GetSize());
}
void CRLoginApp::GetProfileStringIndex(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringIndex &index)
{
	CBuffer buf;

	GetProfileBuffer(lpszSection, lpszEntry, buf);
	index.GetBuffer(&buf);
}
void CRLoginApp::WriteProfileStringIndex(LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringIndex &index)
{
	CBuffer buf;

	index.SetBuffer(&buf);
	WriteProfileBinary(lpszSection, lpszEntry, buf.GetPtr(), buf.GetSize());
}
void CRLoginApp::GetProfileArray(LPCTSTR lpszSection, CStringArrayExt &stra)
{
	int n, mx;
	CString entry;
	CMutexLock Mutex(lpszSection);
	
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
	CMutexLock Mutex(lpszSection);

	for ( n = 0 ; n < stra.GetSize() ; n++ ) {
		entry.Format(_T("List%02d"), n);
		WriteProfileString(lpszSection, entry, stra.GetAt(n));
	}
	WriteProfileInt(lpszSection, _T("ListMax"), n);
}
int CRLoginApp::GetProfileSeqNum(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	CMutexLock Mutex(lpszSection);
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
			delete [] work;
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
		delete [] work;
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
void CRLoginApp::DelProfileSection(LPCTSTR lpszSection)
{
	if ( m_pszRegistryKey != NULL ) {
		HKEY hAppKey;
		if ( (hAppKey = GetAppRegistryKey()) == NULL )
			return;
		RegDeleteKey(hAppKey, lpszSection);
		RegCloseKey(hAppKey);

	} else {
		WritePrivateProfileString(lpszSection, NULL, NULL, m_pszProfileName);
	}
}
BOOL CRLoginApp::AliveProfileKeys(LPCTSTR lpszSection)
{
	BOOL rt = FALSE;

	if ( m_pszRegistryKey != NULL ) {
		HKEY hAppKey;
		if ( (hAppKey = GetAppRegistryKey()) != NULL ) {
			HKEY hSecKey;
			if ( RegOpenKeyEx(hAppKey, lpszSection, 0, KEY_READ, &hSecKey) == ERROR_SUCCESS && hSecKey != NULL ) {
				rt = TRUE;
				RegCloseKey(hSecKey);
			}
			RegCloseKey(hAppKey);
		}

	} else {
		TCHAR tmp[256];
		if ( GetPrivateProfileSection(lpszSection, tmp, 256, m_pszProfileName) != 0 )
			rt = TRUE;
	}

	return rt;
}

void CRLoginApp::RegisterShellRemoveAll()
{
	int n;
	CString strTemp;
	static LPCTSTR protoName[] = { _T("ssh"), _T("telnet"), _T("login"), NULL };

	RegisterDelete(HKEY_CURRENT_USER, _T("Software\\RegisteredApplications"), _T("RLogin"));

	RegisterDelete(HKEY_CURRENT_USER, _T("Software\\Culti\\RLogin\\Capabilities\\FileAssociations"), _T(".rlg"));
	RegisterDelete(HKEY_CURRENT_USER, _T("Software\\Classes"), _T(".rlg"));
	RegisterDelete(HKEY_CURRENT_USER, _T("Software\\Classes"), _T("RLogin.Document"));

	RegisterDelete(HKEY_LOCAL_MACHINE, _T("Software\\Classes"), _T(".rlg"));
	RegisterDelete(HKEY_LOCAL_MACHINE, _T("Software\\Classes"), _T("RLogin.Document"));

	for ( n = 0 ; protoName[n] != NULL ; n++ ) {
		strTemp.Format(_T("RLogin.%s"), protoName[n]);
		RegisterDelete(HKEY_CURRENT_USER, _T("Software\\Culti\\RLogin\\Capabilities\\UrlAssociations"), protoName[n]);
		RegisterDelete(HKEY_CURRENT_USER, _T("Software\\Classes"), strTemp);

		strTemp.Format(_T("Software\\Classes\\%s\\shell\\open\\command"), protoName[n]);
		if ( RegisterGetStr(HKEY_CURRENT_USER, strTemp, _T(""), strTemp) && strTemp.Find(_T("RLogin")) >= 0 ) {

			strTemp.Format(_T("Software\\Classes\\%s"), protoName[n]);
			RegisterDelete(HKEY_CURRENT_USER, strTemp, _T("BrowserFlags"));
			RegisterDelete(HKEY_CURRENT_USER, strTemp, _T("EditFlags"));
			RegisterDelete(HKEY_CURRENT_USER, strTemp, _T("DefaultIcon"));
			RegisterDelete(HKEY_CURRENT_USER, strTemp, _T("shell"));
		}
	}

	if ( RegisterGetStr(HKEY_CLASSES_ROOT, _T("RLogin.Document\\shell\\open\\ddeexec"), _T(""), strTemp) )
		::AfxMessageBox(IDE_REGISTRYDELETEERROR);
}
void CRLoginApp::RegisterShellFileEntry()
{
	CString strTemp;
	CString strPathName;

	if ( RegisterGetStr(HKEY_CURRENT_USER, _T("Software\\Classes\\RLogin.Document\\shell\\open\\command"), _T(""), strTemp) 
				&& _tcsncmp(strPathName, strTemp, strPathName.GetLength()) == 0 )
		return;

	// HKEY_CURRENT_USER\Software\Culti\RLogin\Capabilities
	//   ApplicationDescription = "RLogin Terminal Software"
	// HKEY_CURRENT_USER\Software\Culti\RLogin\Capabilities\FileAssociations
	//   .rlg   = "RLogin.Document"
	//
	// HKEY_CURRENT_USER\Software\RegisteredApplications
	//	 RLogin = "Software\Culti\RLogin\Capabilities"

	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Culti\\RLogin\\Capabilities"), _T("ApplicationDescription"), _T("RLogin Terminal Software"));
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Culti\\RLogin\\Capabilities\\FileAssociations"), _T(".rlg"), _T("RLogin.Document"));
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\RegisteredApplications"), _T("RLogin"), _T("Software\\Culti\\RLogin\\Capabilities"), FALSE);

	// HKEY_CURRENT_USER\Software\Classes\.rlg = RLogin.Document
	// HKEY_CURRENT_USER\Software\Classes\RLogin.Document = "RLogin Document"
	// HKEY_CURRENT_USER\Software\Classes\RLogin.Document\DefaultIcon = "RLogin.exe,1"
	// HKEY_CURRENT_USER\Software\Classes\RLogin.Document\shell\open\command = "RLogin.exe /inuse %1"

	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Classes\\.rlg"), _T(""), _T("RLogin.Document"));
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Classes\\RLogin.Document"), _T(""), _T("RLogin Document"));

	AfxGetModuleShortFileName(AfxGetInstanceHandle(), strPathName);

	strTemp.Format(_T("%s,%d"), strPathName, 1);
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Classes\\RLogin.Document\\DefaultIcon"), _T(""), strTemp);

	strTemp.Format(_T("%s /inuse \"%%1\""), strPathName);
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Classes\\RLogin.Document\\shell\\open\\command"), _T(""), strTemp);
}
void CRLoginApp::RegisterShellProtocol(LPCTSTR pProtocol, LPCTSTR pOption)
{
	CString strTemp;
	CString strEntry;
	CString strSection;
	CString strPathName;
	LPCTSTR pSection = pProtocol;

	if ( !m_bRegistAppp ) {
		if ( ::AfxMessageBox(IDS_REGISTAPPPROTOCOL, MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
		m_bRegistAppp = TRUE;
		WriteProfileInt(_T("RLoginApp"), _T("RegistAppp"), m_bRegistAppp);
	}

	// HKEY_CURRENT_USER\Software\Culti\RLogin\Capabilities
	//   ApplicationDescription = "RLogin Terminal Software"
	// HKEY_CURRENT_USER\Software\Culti\RLogin\Capabilities\UrlAssociations
	//   login  = "RLogin.login"
	//   telnet = "RLogin.telnet"
	//   ssh    = "RLogin.ssh"
	//
	// HKEY_CURRENT_USER\Software\RegisteredApplications
	//	 RLogin = "Software\Culti\RLogin\Capabilities"
	//
	// HKEY_CURRENT_USER\Software\Microsoft\Windows\Shell\Associations\UrlAssociations\SSH
	//    UserChoice = "RLogin.ssh"

	strEntry.Format(_T("RLogin.%s"), pProtocol);

	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Culti\\RLogin\\Capabilities"), _T("ApplicationDescription"), _T("RLogin Terminal Software"));
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Culti\\RLogin\\Capabilities\\UrlAssociations"), pProtocol, strEntry);
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\RegisteredApplications"), _T("RLogin"), _T("Software\\Culti\\RLogin\\Capabilities"), FALSE);

	// HKEY_CURRENT_USER\Software\Classes\RLogin.ssh = "URL: ssh Protocol"
	//	 BrowserFlags = dword:00000008
	//	 EditFlags = dword:00000002
	//	 URL Protocol = ""
	// HKEY_CURRENT_USER\Software\Classes\RLogin.ssh\DefaultIcon = "RLogin.exe,1"
	// HKEY_CURRENT_USER\Software\Classes\RLogin.ssh\shell\open\command = "RLogin.exe /inuse %1"
	//
	// HKEY_CURRENT_USER\Software\Classes\ssh = "URL: ssh Protocol"
	//	 BrowserFlags = dword:00000008
	//	 EditFlags = dword:00000002
	//	 URL Protocol = ""
	// HKEY_CURRENT_USER\Software\Classes\ssh\DefaultIcon = "RLogin.exe,1"
	// HKEY_CURRENT_USER\Software\Classes\ssh\shell\open\command = "RLogin.exe /inuse %1"

	AfxGetModuleShortFileName(AfxGetInstanceHandle(), strPathName);

	for ( int n = 0 ; n < 2 ; n++ ) {
		strSection.Format(_T("Software\\Classes\\%s"), pSection);
		strTemp.Format(_T("URL: %s Protocol"), pProtocol);
		RegisterSetStr(HKEY_CURRENT_USER, strSection, _T(""), strTemp);
		RegisterSetDword(HKEY_CURRENT_USER, strSection, _T("BrowserFlags"), 8);
		RegisterSetDword(HKEY_CURRENT_USER, strSection, _T("EditFlags"), 2);
		RegisterSetStr(HKEY_CURRENT_USER, strSection, _T("URL Protocol"), _T(""));

		strSection.Format(_T("Software\\Classes\\%s\\DefaultIcon"), pSection);
		strTemp.Format(_T("%s,%d"), strPathName, 1);
		RegisterSetStr(HKEY_CURRENT_USER, strSection, _T(""), strTemp);

		strSection.Format(_T("Software\\Classes\\%s\\shell\\open\\command"), pSection);
		strTemp.Format(_T("%s %s \"%%1\""), strPathName, pOption);
		RegisterSetStr(HKEY_CURRENT_USER, strSection, _T(""), strTemp);

		// "RLogin.ssh"と"ssh"に同じ内容を設定
		pSection = strEntry;
	}
}
BOOL CRLoginApp::RegisterGetStr(HKEY hKey, LPCTSTR pSection, LPCTSTR pEntryName, CString &str)
{
	HKEY hSubKey = NULL;
	DWORD type, size;
	BOOL ret = FALSE;

	if ( RegOpenKeyEx(hKey, pSection, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS ) {
		if ( RegQueryValueEx(hSubKey, pEntryName, NULL, &type, NULL, &size) == ERROR_SUCCESS && type == REG_SZ ) {
			if ( RegQueryValueEx(hSubKey, pEntryName, NULL, &type, LPBYTE(str.GetBufferSetLength(size)), &size) == ERROR_SUCCESS )
				ret = TRUE;
		}
		RegCloseKey(hSubKey);
	}

	return ret;
}
BOOL CRLoginApp::RegisterSetStr(HKEY hKey, LPCTSTR pSection, LPCTSTR pEntryName, LPCTSTR str, BOOL bCreate)
{
	HKEY hSubKey;
	BOOL ret = FALSE;
	DWORD dw = 0;

	if ( RegOpenKeyEx(hKey, pSection, 0, KEY_WRITE, &hSubKey) != ERROR_SUCCESS ) {
		if ( !bCreate || RegCreateKeyEx(hKey, pSection, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hSubKey, &dw) != ERROR_SUCCESS )
			return FALSE;
	}

	if ( RegSetValueEx(hSubKey, pEntryName, 0, REG_SZ, (const LPBYTE)str, (DWORD)(_tcslen(str) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS )
		ret = TRUE;

	RegCloseKey(hSubKey);

	return ret;
}
BOOL CRLoginApp::RegisterSetDword(HKEY hKey, LPCTSTR pSection, LPCTSTR pEntryName, DWORD dword, BOOL bCreate)
{
	HKEY hSubKey;
	BOOL ret = FALSE;
	DWORD dw = 0;

	if ( RegOpenKeyEx(hKey, pSection, 0, KEY_WRITE, &hSubKey) != ERROR_SUCCESS ) {
		if ( !bCreate || RegCreateKeyEx(hKey, pSection, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hSubKey, &dw) != ERROR_SUCCESS )
			return FALSE;
	}

	if ( RegSetValueEx(hSubKey, pEntryName, 0, REG_DWORD, (const LPBYTE)&dword, (DWORD)sizeof(DWORD)) == ERROR_SUCCESS )
		ret = TRUE;

	RegCloseKey(hSubKey);

	return ret;
}
void CRLoginApp::RegisterDelete(HKEY hKey, LPCTSTR pSection, LPCTSTR pKey)
{
	CRegKey reg;

	if ( reg.Open(hKey, pSection) != ERROR_SUCCESS )
		return;

	if ( reg.DeleteValue(pKey) != ERROR_SUCCESS )
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

	delete [] work;

	buf.Put32Bit((int)menba.GetSize());
	for ( n = 0 ; n < menba.GetSize() ; n++ ) {
		buf.PutStrT(menba[n]);
		RegisterSave(reg.m_hKey, menba[n], buf);
	}

	reg.Close();
}
void CRLoginApp::RegisterLoad(HKEY hKey, LPCTSTR pSection, CBuffer &buf)
{
	int n, len;
	CRegKey reg;
	DWORD type;
	CString name;
	CBuffer work;

	if ( buf.GetSize() < 4 )
		return;

	if ( reg.Create(hKey, pSection) != ERROR_SUCCESS )
		return;

	type = buf.Get32Bit();
	buf.GetBuf(&work);
	reg.SetValue(_T(""), type, work.GetPtr(), work.GetSize());

	len = buf.Get32Bit();
	for ( n = 0 ; n < len ; n++ ) {
		buf.GetStrT(name);
		RegisterLoad(reg.m_hKey, name, buf);
	}

	reg.Close();
}

BOOL CRLoginApp::SavePrivateKey(HKEY hKey, CFile *file)
{
	int n;
	TCHAR name[1024];
	DWORD len;
	DWORD type;
	DWORD max = 0;
	BYTE *pData = NULL;
	DWORD size;
	FILETIME last;
	HKEY hSubKey;
	CStringA mbs;

	for ( n = 0 ; ; n++ ) {
		len = 1024;
		if ( RegEnumKeyEx(hKey, n, name, &len, NULL, NULL, NULL, &last) != ERROR_SUCCESS )
			break;

		if ( RegOpenKeyEx(hKey, name, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS )
			return FALSE;

		mbs.Format("[%s]\r\n", TstrToMbs(name));
		file->Write((LPCSTR)mbs, mbs.GetLength() * sizeof(CHAR));

		if ( !SavePrivateKey(hSubKey, file) ) {
			RegCloseKey(hSubKey);
			return FALSE;
		}

		RegCloseKey(hSubKey);
	}

	for ( n = 0 ; ; n++ ) {
		len = 1024;
		if ( RegEnumValue(hKey, n, name, &len, NULL, &type, NULL, &size) != ERROR_SUCCESS )
			break;

		if ( max < (size + 2) ) {
			if ( pData != NULL )
				delete [] pData;
			max = size + 2;
			pData = new BYTE[max];
		}

		if ( RegQueryValueEx(hKey, name, NULL, &type, pData, &size) != ERROR_SUCCESS )
			continue;

		switch(type) {
		case REG_DWORD:
			if ( size == 0 )
				mbs.Format("%s=\r\n", TstrToMbs(name));
			else
				mbs.Format("%s=%d\r\n", TstrToMbs(name), *((DWORD *)pData));
			break;
		case REG_SZ:
			if ( size == 0 )
				mbs.Format("%s=\r\n", TstrToMbs(name));
			else {
				*((LPTSTR)(pData + size)) = L'\0';
				mbs.Format("%s=%s\r\n", TstrToMbs(name), TstrToMbs((LPCTSTR)pData));
			}
			break;
		case REG_BINARY:
			mbs.Format("%s=", TstrToMbs(name));
			for ( len = 0 ; len < size ; len++ ) {
				mbs += (CHAR)('A' + (pData[len] & 0x0F));
				mbs += (CHAR)('A' + ((pData[len] >> 4) & 0x0F));
			}
			mbs += "\r\n";
			break;
		default:
			// XXX Error
			mbs.Empty();
			break;
		}

		if ( !mbs.IsEmpty() )
			file->Write((LPCSTR)mbs, mbs.GetLength() * sizeof(CHAR));
	}

	if ( pData != NULL )
		delete [] pData;

	return TRUE;
}
BOOL CRLoginApp::SavePrivateProfile()
{
	CFile file;
	CString filename;
	HKEY hAppKey;
	BOOL ret = FALSE;

	filename.Format(_T("%s\\RLogin.ini"), m_BaseDir);
	CFileDialog dlg(FALSE, _T("ini"), filename, OFN_OVERWRITEPROMPT, _T("Private Profile (*.ini)|*.ini|All Files (*.*)|*.*||"), AfxGetMainWnd());

	if ( DpiAwareDoModal(dlg) != IDOK )
		return FALSE;

	CWaitCursor wait;

	if ( !file.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite) ) {
		::AfxMessageBox(_T("Cann't Create Private Profile"));
		return FALSE;
	}

	if ( (hAppKey = GetAppRegistryKey()) != NULL ) {
		if ( SavePrivateKey(hAppKey, &file) )
			ret = TRUE;
		RegCloseKey(hAppKey);
	}

	file.Close();
	return ret;
}

void CRLoginApp::RegistryEscapeStr(LPCTSTR str, int len, CString &out)
{
	out.Empty();

	for ( ; len > 0 ; len--, str++ ) {
		switch(*str) {
		case _T('\n'):
			out += _T("\r\n");
			break;
		case _T('"'):
			out += _T("\\\"");
			break;
		case _T('\\'):
			out += _T("\\\\");
			break;
		default:
			out += *str;
			break;
		}
	}
}
BOOL CRLoginApp::SaveRegistryKey(HKEY hKey, CFile *file, LPCTSTR base)
{
	int n;
	TCHAR name[1024];
	DWORD len;
	DWORD type;
	DWORD max = 0;
	BYTE *pData = NULL;
	DWORD size;
	FILETIME last;
	HKEY hSubKey;
	CString keypath, out;
	CStringW key, str, hex;

	str.Format(L"[%s]\r\n", TstrToUni(base));
	file->Write((LPCWSTR)str, str.GetLength() * sizeof(WCHAR));

	for ( n = 0 ; ; n++ ) {
		len = 1024;
		if ( RegEnumValue(hKey, n, name, &len, NULL, &type, NULL, &size) != ERROR_SUCCESS )
			break;

		if ( max < size ) {
			if ( pData != NULL )
				delete [] pData;
			max = size;
			pData = new BYTE[max];
		}

		if ( RegQueryValueEx(hKey, name, NULL, &type, pData, &size) != ERROR_SUCCESS )
			continue;

		if ( _tcslen(name) > 0 )
			key.Format(L"\"%s\"", name);
		else
			key = L"@";

		switch(type) {
		case REG_SZ:
			RegistryEscapeStr((LPCTSTR)pData, size / sizeof(TCHAR), out);
			str.Format(L"%s=\"%s\"\r\n", key, TstrToUni(out));
			break;

		case REG_DWORD:
			str.Format(L"%s=dword:%08x\r\n", key, size < sizeof(DWORD) ? 0 : *((DWORD *)pData));
			break;

		default:
			if ( type == REG_BINARY )
				str.Format(L"%s=hex:", key);
			else
				str.Format(L"%s=hex(%x):", key, type);

			for ( len = 0 ; len < size ; len++ ) {
				hex.Format(L"%02x", pData[len]);
				str += hex;
				if ( (len + 1) < size ) {
					str += L',';
					if ( str.GetLength() > 76 ) {
						str += L"\\\r\n";
						file->Write((LPCWSTR)str, str.GetLength() * sizeof(WCHAR));
						str = L"  ";
					}
				}
			}
			str += L"\r\n";
			break;
		}

		file->Write((LPCWSTR)str, str.GetLength() * sizeof(WCHAR));
	}

	if ( pData != NULL )
		delete [] pData;

	file->Write(L"\r\n", 2 * sizeof(WCHAR));

	for ( n = 0 ; ; n++ ) {
		len = 1024;
		if ( RegEnumKeyEx(hKey, n, name, &len, NULL, NULL, NULL, &last) != ERROR_SUCCESS )
			break;

		if ( RegOpenKeyEx(hKey, name, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS )
			continue;

		keypath.Format(_T("%s\\%s"), base, name);

		if ( !SaveRegistryKey(hSubKey, file, keypath) ) {
			RegCloseKey(hSubKey);
			return FALSE;
		}

		RegCloseKey(hSubKey);
	}

	return TRUE;
}
BOOL CRLoginApp::SaveRegistryFile()
{
	CString key;
	CFile file;
	CString filename;
	CTime tm = CTime::GetCurrentTime();
	HKEY hAppKey;
	WCHAR bom = 0xFEFF;
	BOOL ret = FALSE;
	static LPCWSTR head = L"Windows Registry Editor Version 5.00\r\n\r\n";

	filename.Format(_T("%s\\RLogin-%s.reg"), m_BaseDir, tm.Format(_T("%y%m%d")));
	CFileDialog dlg(FALSE, _T("reg"), filename, OFN_OVERWRITEPROMPT, _T("Registry file (*.reg)|*.reg|All Files (*.*)|*.*||"), AfxGetMainWnd());

	if ( DpiAwareDoModal(dlg) != IDOK )
		return FALSE;

	CWaitCursor wait;

	if ( !file.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite) ) {
		::AfxMessageBox(_T("Cann't Create Registry file"));
		return FALSE;
	}

	file.Write(&bom, sizeof(WCHAR)); 
	file.Write(head, (UINT)_tcslen(head) * sizeof(WCHAR));

	ASSERT(m_pszRegistryKey != NULL);
	ASSERT(m_pszProfileName != NULL);

	key.Format(_T("HKEY_CURRENT_USER\\Software\\%s\\%s"), m_pszRegistryKey, m_pszProfileName);

	if ( (hAppKey = GetAppRegistryKey()) != NULL ) {
		if ( SaveRegistryKey(hAppKey, &file, key) )
			ret = TRUE;
		RegCloseKey(hAppKey);
	}

	file.Close();
	return ret;
}

//////////////////////////////////////////////////////////////////////

void CRLoginApp::GetVersion(CString &str)
{
	HRSRC hRsrc;
	HGLOBAL hGlobal;
	struct _VS_VERSIONINFO {
		WORD  wLength;
		WORD  wValueLength;
		WORD  wType;
		WCHAR szKey[16];
		WORD  Padding1[1];
		VS_FIXEDFILEINFO Value;
	} *pData;

	str = _T("1.1.0");

	if ( (hRsrc = FindResource(NULL, (LPCTSTR)VS_VERSION_INFO, RT_VERSION)) == NULL )
		return;

	if ( (hGlobal = LoadResource(NULL, hRsrc)) == NULL )
		return;

	if ( (pData = (struct _VS_VERSIONINFO *)LockResource(hGlobal)) != NULL )
		str.Format(_T("%d.%d.%d"), HIWORD(pData->Value.dwFileVersionMS), LOWORD(pData->Value.dwFileVersionMS), HIWORD(pData->Value.dwFileVersionLS));

	FreeResource(hGlobal);
}

void CRLoginApp::SetDefaultPrinter()
{
	int n, max;
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

	max = sizeof(DEVNAMES) + (Driver.GetLength() + Device.GetLength() + Output.GetLength() + 3) * sizeof(TCHAR);

	if ( (hDevNames = ::GlobalAlloc(GHND, max)) == NULL )
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
	_tcsncpy(ptr + n, Driver, (max / sizeof(TCHAR)) - n);
	n += (Driver.GetLength() + 1);

	lpDevNames->wDeviceOffset = n;
	_tcsncpy(ptr + n, Device, (max / sizeof(TCHAR)) - n);
	n += (Device.GetLength() + 1);

	lpDevNames->wOutputOffset = n;
	_tcsncpy(ptr + n, Output, (max / sizeof(TCHAR)) - n);

	memcpy(lpDevMode, pByte, sz);

	::GlobalUnlock(hDevMode);
	::GlobalUnlock(hDevNames);

	SelectPrinter(hDevNames, hDevMode);

ERRRET:
	delete pByte;
}

//////////////////////////////////////////////////////////////////////

BOOL mt_proc(LPVOID pParam);

BOOL CRLoginApp::OnIdle(LONG lCount) 
{
	int n;
	CIdleProc *pProc;
	BOOL rt = FALSE;

	if ( lCount >= 0 && CWinApp::OnIdle(lCount) )
		return TRUE;

	for ( n = 0 ; !rt && n < m_IdleProcCount && (pProc = m_pIdleTop) != NULL ; n++ ) {
		m_pIdleTop = pProc->m_pNext;

		switch(pProc->m_Type) {
		case IDLEPROC_SOCKET:
			rt = ((CExtSocket *)(pProc->m_pParam))->OnIdle();
			break;
		case IDLEPROC_ENCRYPT:
			rt = mt_proc(pProc->m_pParam);
			break;
		case IDLEPROC_SCRIPT:
			rt = ((CScript *)(pProc->m_pParam))->OnIdle();
			break;
		case IDLEPROC_VIEW:
			rt = ((CRLoginView *)(pProc->m_pParam))->OnIdle();
			break;
		}

		// pProcの呼び出し後の利用不可(DelIdleProc後の可能性あり)
		// m_IdleProcCountも変化する場合有り
	}

	// メッセージ処理後に必ずOnIdleを呼ぶように・・・
	if ( rt )
		m_bUseIdle = TRUE;

	//TRACE("OnIdle(%d) %d\n", lCount, rt);

	return rt;
}
BOOL CRLoginApp::IsIdleMessage(MSG* pMsg)
{
	if ( m_bUseIdle ) {
		m_bUseIdle = FALSE;
		return TRUE;
	}

	if ( pMsg->message == WM_SOCKSEL || pMsg->message == WM_NULL || pMsg->message == WM_PAINT )
		return TRUE;

	if ( CWinApp::IsIdleMessage(pMsg) )
		return TRUE;

	return FALSE;
}

void CRLoginApp::AddIdleProc(int Type, void *pParam)
{
	CIdleProc *pProc = m_pIdleTop;
	
	while ( pProc != NULL ) {
		if ( pProc->m_Type == Type && pProc->m_pParam == pParam )
			return;
		if ( (pProc = pProc->m_pNext) == m_pIdleTop )
			break;
	}

	m_IdleProcCount++;
	pProc = new CIdleProc;
	pProc->m_Type = Type;
	pProc->m_pParam = pParam;

	if ( m_pIdleTop == NULL ) {
		pProc->m_pBack = pProc->m_pNext = pProc;
		m_pIdleTop = pProc;
	} else {
		pProc->m_pBack = m_pIdleTop->m_pBack;
		m_pIdleTop->m_pBack->m_pNext = pProc;
		pProc->m_pNext = m_pIdleTop;
		m_pIdleTop->m_pBack = pProc;
		m_pIdleTop = pProc;
	}
}
void CRLoginApp::DelIdleProc(int Type, void *pParam)
{
	CIdleProc *pProc = m_pIdleTop;

	while ( pProc != NULL ) {
		if ( pProc->m_Type == Type && pProc->m_pParam == pParam ) {
			if ( pProc->m_pNext == pProc )
				m_pIdleTop = NULL;
			else {
				pProc->m_pBack->m_pNext = pProc->m_pNext;
				pProc->m_pNext->m_pBack = pProc->m_pBack;
				if ( pProc == m_pIdleTop )
					m_pIdleTop = pProc->m_pNext;
			}
			delete pProc;
			m_IdleProcCount--;
			break;
		}

		if ( (pProc = pProc->m_pNext) == m_pIdleTop )
			break;
	}
}

//////////////////////////////////////////////////////////////////////
// CRLoginApp メッセージ ハンドラ

void CRLoginApp::OnFileNew()
{
	if ( ((CMainFrame *)::AfxGetMainWnd())->GetTabCount() < DOCUMENT_MAX )
		CWinApp::OnFileNew();
}
void CRLoginApp::OnFileOpen()
{
	if ( ((CMainFrame *)::AfxGetMainWnd())->GetTabCount() < DOCUMENT_MAX )
		CWinApp::OnFileOpen();
}
void CRLoginApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CRLoginApp::OnFilePrintSetup()
{
	INT_PTR res;
	CPrintDialog pd(TRUE);

	DpiAwareSwitch(TRUE);
	res = DoPrintDialog(&pd);
	DpiAwareSwitch(FALSE);

	if ( res != IDOK )
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

void CRLoginApp::OnDialogfont()
{
	CString FontName;
	int FontSize;
	LOGFONT LogFont;
	CDC dc;

	dc.CreateCompatibleDC(NULL);

	FontName = GetProfileString(_T("Dialog"), _T("FontName"), _T("MS UI Gothic"));
	FontSize = GetProfileInt(_T("Dialog"), _T("FontSize"), 9);

	memset(&(LogFont), 0, sizeof(LOGFONT));
	LogFont.lfWidth          = 0;
	LogFont.lfHeight         = 0 - MulDiv(FontSize, dc.GetDeviceCaps(LOGPIXELSY), 72);
	LogFont.lfWeight         = FW_DONTCARE;
	LogFont.lfCharSet        = DEFAULT_CHARSET;
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DEFAULT_QUALITY;
	LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    _tcsncpy(LogFont.lfFaceName, FontName, LF_FACESIZE);

	CFontDialog font(&LogFont, CF_NOVERTFONTS | CF_SCREENFONTS | CF_SELECTSCRIPT, NULL, ::AfxGetMainWnd());

	if ( DpiAwareDoModal(font) != IDOK )
		return;

    FontName = LogFont.lfFaceName;
	FontSize = 0 - MulDiv(LogFont.lfHeight, 72, dc.GetDeviceCaps(LOGPIXELSY));

	WriteProfileString(_T("Dialog"), _T("FontName"), FontName);
	WriteProfileInt(_T("Dialog"), _T("FontSize"), FontSize);

	((CMainFrame *)AfxGetMainWnd())->BarFontCheck();
}

void CRLoginApp::OnLookcast()
{
	m_bLookCast = m_bLookCast ? FALSE : TRUE;
	WriteProfileInt(_T("RLoginApp"), _T("LookCast"), m_bLookCast);
}
void CRLoginApp::OnUpdateLookcast(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bLookCast);
}
void CRLoginApp::OnOthercast()
{
	m_bOtherCast = m_bOtherCast ? FALSE : TRUE;
	WriteProfileInt(_T("RLoginApp"), _T("OtherCast"), m_bOtherCast);
}
void CRLoginApp::OnUpdateOthercast(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bOtherCast);
}

void CRLoginApp::OnPassLock()
{
	int n;
	CIdKey key;
	CEditDlg dlg;
	CString passok, tmp;
	CServerEntryTab *pTab = &(((CMainFrame *)::AfxGetMainWnd())->m_ServerEntryTab);

	if ( m_LocalPass.IsEmpty() ) {	// パスワードロック設定
		dlg.m_bPassword = TRUE;
		dlg.m_Edit    = _T("");
		dlg.m_WinText = _T("RLogin Password Lock");
		dlg.m_Title.LoadString(IDS_PASSLOCKINITMSG);

		if ( dlg.DoModal() != IDOK || dlg.m_Edit.IsEmpty() )
			return;

		key.Digest(m_LocalPass, dlg.m_Edit);
		key.EncryptStr(passok, _T("01234567"));

		WriteProfileString(_T("RLoginApp"), _T("LocalPass"), passok);

		for ( n = 0 ; n < pTab->GetSize() ; n++ ) {
			if ( pTab->GetAt(n).m_bPassOk )
				pTab->UpdateAt(n);
			else
				pTab->ReloadAt(n);
		}

	} else {						// パスワードロック解除
		if ( ::AfxMessageBox(IDS_PASSLOCKDELMSG, MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;

		WriteProfileString(_T("RLoginApp"), _T("LocalPass"), _T(""));
		m_LocalPass.Empty();

		for ( n = 0 ; n < pTab->GetSize() ; n++ ) {
			if ( pTab->GetAt(n).m_bPassOk )
				pTab->UpdateAt(n);
		}
	}
}
void CRLoginApp::OnUpdatePassLock(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_LocalPass.IsEmpty() ? FALSE : TRUE);
}
void CRLoginApp::OnSaveresfile()
{
	CResTransDlg *pDlg = new CResTransDlg;

	pDlg->m_ResDataBase = m_ResDataBase;
	pDlg->m_ResDataBase.InitRessource();
	pDlg->m_ResFileName.Format(_T("%s\\%s_rc.txt"), m_BaseDir, m_pszAppName);

	pDlg->Create(IDD_RESTRANSDLG); //, CWnd::GetDesktopWindow());
	pDlg->ShowWindow(SW_SHOW);
}

void CRLoginApp::OnCreateprofile()
{
	if ( SavePrivateProfile() )
		::AfxMessageBox(IDS_CREATEPROFILE);
}
void CRLoginApp::OnUpdateCreateprofile(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pszRegistryKey != NULL ? TRUE : FALSE);
}

void CRLoginApp::OnSaveregfile()
{
	if ( !SaveRegistryFile() )
		::AfxMessageBox(_T("Can't Save Registry File"));
}
void CRLoginApp::OnUpdateSaveregfile(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pszRegistryKey != NULL ? TRUE : FALSE);
}

void CRLoginApp::OnRegistapp()
{
	m_bRegistAppp = GetProfileInt(_T("RLoginApp"), _T("RegistAppp"),  FALSE);

	if ( m_bRegistAppp ) {
		m_bRegistAppp = FALSE;
		RegisterShellRemoveAll();
	} else {
		m_bRegistAppp = TRUE;
		RegisterShellFileEntry();
	}

	WriteProfileInt(_T("RLoginApp"), _T("RegistAppp"), m_bRegistAppp);
}
void CRLoginApp::OnUpdateRegistapp(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bRegistAppp);
}

#ifdef	USE_DIRECTWRITE

HDC CRLoginApp::GetEmojiImage(class CEmojiImage *pEmoji)
{
	if ( m_EmojiImageDir.IsEmpty() )
		return NULL;

	int n;
	CString path, base, name, fmt;
	CStringD dstr = pEmoji->m_String;
	LPCDSTR p = dstr;

	base.Format(_T("%s\\%05X\\"), m_EmojiImageDir, *p & 0xFFFF00);

	for ( ; *p != 0 ; p++ ) {
		fmt.Format(_T("%05X_"), *p);
		name += fmt;
	}
	name.Delete(name.GetLength() - 1, 1);


	for ( ; ; ) {
		path.Format(_T("%s%s.png"), base, name);

		if ( _taccess_s(path, 04) == 0 ) {
			if ( FAILED(pEmoji->m_Image.Load(path)) )
				break;
			pEmoji->m_bFileImage = TRUE;
			return pEmoji->m_Image.GetDC();
		}

		if ( (n = name.ReverseFind(_T('_'))) < 0 )
			break;

		name.Delete(n, name.GetLength() - n);
	}

	return NULL;
}
void CRLoginApp::SaveEmojiImage(class CEmojiImage *pEmoji)
{
	CString path, fmt;
	CStringD dstr = pEmoji->m_String;
	LPCDSTR p = dstr;

	path.Format(_T("%s\\emoji\\%05X"), m_BaseDir, *p & 0xFFFF00);
	CreateDirectory(path, NULL);
	path += _T('\\');

	for ( ; *p != 0 ; p++ ) {
		fmt.Format(_T("%05X_"), *p);
		path += fmt;
	}

	path.Delete(path.GetLength() - 1, 1);
	path += _T(".png");

	pEmoji->m_Image.Save(path);
}
HDC CRLoginApp::GetEmojiDrawText(class CEmojiImage *pEmoji, COLORREF fc, int fh)
{
	if ( m_pD2DFactory == NULL || m_pDWriteFactory == NULL )
		return NULL;

	BOOL rc = FALSE;
	ID2D1SolidColorBrush *pBrush = NULL;
	IDWriteTextFormat *pTextFormat = NULL;
	IDWriteTextLayout* pTextLayout = NULL;
	DWRITE_TEXT_METRICS tTextMetrics;
	CRect frame;
	HDC hDC = NULL;
	LPCTSTR str = pEmoji->m_String;
	int len = pEmoji->m_String.GetLength();
	static D2D1_COLOR_F oBKColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	if ( m_pDCRT == NULL ) {
		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

		if ( FAILED(m_pD2DFactory->CreateDCRenderTarget(&props, &m_pDCRT)) )
			goto ENDOF;
	}

	if ( FAILED(m_pDCRT->CreateSolidColorBrush(D2D1::ColorF(fc), &pBrush)) )
		goto ENDOF;

	if ( FAILED(m_pDWriteFactory->CreateTextFormat(m_EmojiFontName, NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			(FLOAT)fh, _T(""), &pTextFormat)) )
		goto ENDOF;

	if ( FAILED(m_pDWriteFactory->CreateTextLayout(str, len, pTextFormat, 1024, 256, &pTextLayout)) )
		goto ENDOF;
                        
	pTextLayout->GetMetrics(&tTextMetrics);

	frame.left   = frame.top = 0;
	frame.right  = (LONG)tTextMetrics.width;
	frame.bottom = (LONG)tTextMetrics.height;

	if ( pEmoji == NULL )
		pEmoji = new CEmojiImage;
	pEmoji->m_String = str;

	if ( !pEmoji->m_Image.CreateEx(frame.Width(), frame.Height(), 32, BI_RGB, NULL, CImage::createAlphaChannel) )
		goto ENDOF;

	hDC = pEmoji->m_Image.GetDC();

	if ( FAILED(m_pDCRT->BindDC(hDC, &frame)) )
		goto ENDOF;

	m_pDCRT->BeginDraw();
	m_pDCRT->SetTransform(D2D1::Matrix3x2F::Identity());
	m_pDCRT->Clear(oBKColor);

#define	D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT	0x00000004

	m_pDCRT->DrawText(str, len, pTextFormat,
		&D2D1::RectF((FLOAT)frame.left, (FLOAT)frame.top, (FLOAT)frame.right, (FLOAT)frame.bottom), 
		pBrush, (D2D1_DRAW_TEXT_OPTIONS)D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);

	if ( FAILED(m_pDCRT->EndDraw()) ) {
		m_pDCRT->Release();
		m_pDCRT = NULL;
		goto ENDOF;
	}

	rc = TRUE;
	pEmoji->m_bFileImage = FALSE;

	// emoji\xxx00\xxxxx.pngを作成
	//SaveEmojiImage(pEmoji);

ENDOF:
	if ( !rc && hDC != NULL ) {
		pEmoji->m_Image.ReleaseDC();
		hDC = NULL;
	}

	if ( pTextLayout != NULL )
		pTextLayout->Release();

	if ( pTextFormat != NULL )
		pTextFormat->Release();

	if ( pBrush != NULL )
		pBrush->Release();

	return hDC;
}

BOOL CRLoginApp::DrawEmoji(CDC *pDC, CRect &rect, LPCTSTR str, COLORREF fc, COLORREF bc, BOOL bEraBack, int fh, int zm)
{
	BOOL rc = FALSE;
	HDC hDC = NULL;
	int hash = 0;
	int count = 0;
	BOOL bLast = FALSE;
	CEmojiImage *pBack, *pMid, *pEmoji;
	int width, height;
 	static BLENDFUNCTION bf = { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA};

	for ( int n = 0 ; str[n] != _T('\0') ; n++ )
		hash = hash + (int)str[n];
	hash &= (EMOJI_HASH -1);

	for ( pBack = pEmoji = m_pEmojiList[hash] ; pEmoji != NULL ; ) {
		if ( pEmoji->m_String.Compare(str) == 0 ) {
			if ( pEmoji == pBack )
				m_pEmojiList[hash] = pEmoji->m_pNext;
			else
				pBack->m_pNext = pEmoji->m_pNext;

			if ( (HBITMAP)(pEmoji->m_Image) == NULL )
				goto ENDOF;

			// キャッシュしているサイズが小さい場合は、再構築
			if ( !pEmoji->m_bFileImage && pEmoji->m_Image.GetHeight() < fh )
				pEmoji->m_Image.Destroy();
			else
				hDC = pEmoji->m_Image.GetDC();
			break;
		}

		if ( ++count > EMOJI_LISTMAX && pEmoji->m_pNext == NULL ) {
			// 再利用時には、リストの中間に挿入。キャッシュミスが連続する場合には有効？(ARCのつもり)
			bLast = TRUE;
			pBack->m_pNext = NULL;
			pEmoji->m_Image.Destroy();
			break;
		} else if ( count == (EMOJI_LISTMAX / 2) )
			pMid = pEmoji;

		pBack = pEmoji;
		pEmoji = pEmoji->m_pNext;
	}

	if ( hDC == NULL ) {
		if ( pEmoji == NULL )
			pEmoji = new CEmojiImage;
		pEmoji->m_String = str;

		// ビットマップ取得できない情報もキャッシュに登録
		if (  (hDC = GetEmojiImage(pEmoji)) == NULL && (hDC = GetEmojiDrawText(pEmoji, fc, fh)) == NULL )
			goto ENDOF;
	}

	width  = pEmoji->m_Image.GetWidth();
	height = pEmoji->m_Image.GetHeight();

	if ( bEraBack )
		pDC->FillSolidRect(rect, bc);

	::AlphaBlend(pDC->GetSafeHdc(), rect.left, rect.top, rect.Width(), rect.Height(),
		hDC, 0, zm == 3 ? (height / 2) : 0, width, zm >= 2 ? (height / 2) : height, bf);

	// CImage.GetDC()してあるので注意
	pEmoji->m_Image.ReleaseDC();
	rc = TRUE;

ENDOF:
	if ( bLast ) {
		// Insert Middle
		pEmoji->m_pNext = pMid->m_pNext;
		pMid->m_pNext = pEmoji;
	} else {
		// Insert Top
		pEmoji->m_pNext = m_pEmojiList[hash];
		m_pEmojiList[hash] = pEmoji;
	}

	return rc;
}
#endif
