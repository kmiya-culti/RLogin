// RLogin.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "RLogin.h"
#include "locale.h"
#include "EditDlg.h"

#ifdef	USE_DWMAPI
#include "DWMApi.h"
#endif

#ifdef	USE_JUMPLIST
#include "Shobjidl.h"
#include "Shlobj.h"
#include "propvarutil.h"
#include "propsys.h"
#include "propkey.h"
#endif

#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "Script.h"
#include "ResTransDlg.h"
#include "MsgChkDlg.h"

#include "direct.h"

#include "openssl/ssl.h"
#include "openssl/engine.h"
#include "openssl/conf.h"
#include "internal/cryptlib.h"
#include <openssl/crypto.h>

#include <afxcmn.h>
#include <sphelper.h>
#include <mbctype.h>
#include <afxglobals.h>

#include "Fifo.h"

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
	m_Cwd.Empty();
	m_DarkOff = FALSE;
	m_Opt.Empty();
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
		else if ( _tcsicmp(_T("cwd"), pszParam) == 0 )
			m_PasStat = 17;
		else if ( _tcsicmp(_T("darkoff"), pszParam) == 0 )
			m_DarkOff = TRUE;
		else if ( _tcsicmp(_T("opt"), pszParam) == 0 )
			m_PasStat = 18;
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
	case 17:		// cwd
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Cwd = pszParam;
		ParseLast(bLast);
		return;
	case 18:		// opt
		m_PasStat = 0;
		if ( bFlag )
			break;
		m_Opt = pszParam;
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
		tmp.Format(_T("\"%s\""), (LPCTSTR)m_strFileName);
		str += tmp;
	}

	if ( !m_Name.IsEmpty() ) {
		tmp.Format(_T(" /entry \"%s\""), (LPCTSTR)m_Name);
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

	if ( !m_Cwd.IsEmpty() ) {
		tmp.Format(_T(" /cwd %s"), ShellEscape(m_Cwd));
		str += tmp;
	}

	if ( m_DarkOff == FALSE )
		str += _T(" /darkoff");

	if ( !m_Opt.IsEmpty() ) {
		tmp.Format(_T(" /opt %s"), ShellEscape(m_Opt));
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

	SetSaveProfile(_T("AboutDlg"));

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
// CSecPolicyDlg

class CSecPolicyDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CSecPolicyDlg)

public:
	CSecPolicyDlg();

// ダイアログ データ
	enum { IDD = IDD_SECPOLICYDLG };

public:
	CStatic m_MsgIcon;
	int m_MakeKeyMode;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CSecPolicyDlg, CDialogExt)

CSecPolicyDlg::CSecPolicyDlg() : CDialogExt(CSecPolicyDlg::IDD)
{
	m_MakeKeyMode = MAKEKEY_USERHOST;
}

void CSecPolicyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_MSGICON, m_MsgIcon);
	DDX_Radio(pDX, IDC_RADIO1, m_MakeKeyMode);
}

BOOL CSecPolicyDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	m_MsgIcon.SetIcon(::LoadIcon(NULL, IDI_WARNING));
	m_MakeKeyMode = AfxGetApp()->GetProfileInt(_T("RLoginApp"), _T("MakeKeyMode"), MAKEKEY_USERHOST);

	UpdateData(FALSE);

	SetSaveProfile(_T("SecPolicyDlg"));
	AddHelpButton(_T("#SECPOLICY"));

	return TRUE;
}

void CSecPolicyDlg::OnOK()
{
	UpdateData(TRUE);

	AfxGetApp()->WriteProfileInt(_T("RLoginApp"), _T("MakeKeyMode"), m_MakeKeyMode);

	CDialogExt::OnOK();
}


BEGIN_MESSAGE_MAP(CSecPolicyDlg, CDialogExt)
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
	ON_COMMAND(IDM_SECPORICY, &CRLoginApp::OnSecporicy)
END_MESSAGE_MAP()


// CRLoginApp コンストラクション

CRLoginApp::CRLoginApp()
{
	m_pServerEntry = NULL;
	m_bLookCast = FALSE;
	m_bOtherCast = FALSE;
	m_LocalPass.Empty();

	m_ProvDefault = NULL;
	m_ProvLegacy = NULL;
	m_ProvRlogin = NULL;

#ifdef	USE_DIRECTWRITE
	m_pD2DFactory    = NULL;
	m_pDWriteFactory = NULL;
	m_pDCRT = NULL;

	for ( int n = 0 ; n < EMOJI_HASH ; n++ )
		m_pEmojiList[n] = NULL;

	m_pEmojiThreadQue = NULL;
	m_EmojiThreadMode = 0;
#endif

	m_pVoice = NULL;

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
	BOOL CompNameLenBugFix = TRUE;
	CString SystemIconv;

	BOOL ExDwmEnable = FALSE;
#ifdef	USE_DWMAPI
	HMODULE ExDwmApi = NULL;
	HRESULT (__stdcall *ExDwmIsCompositionEnabled)(BOOL * pfEnabled) = NULL;
	HRESULT (__stdcall *ExDwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind) = NULL;
	HRESULT (__stdcall *ExDwmExtendFrameIntoClientArea)(HWND hWnd, const MARGINS* pMarInset) = NULL;
	HRESULT (__stdcall *ExDwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, __in_bcount(cbAttribute) LPCVOID pvAttribute, DWORD cbAttribute) = NULL;
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

	HMODULE ExKernel32Api = NULL;
	BOOL (WINAPI *ExCancelIoEx)(HANDLE hFile, LPOVERLAPPED lpOverlapped) = NULL;

	HMODULE ExUxThemeDll = NULL;
	BOOL (_stdcall *AllowDarkModeForWindow)(HWND hwnd, BOOL allow) = NULL;	// #133;
	BOOL (__stdcall *AllowDarkModeForApp)(int mode) = NULL;					// #135
	HRESULT (__stdcall *ExSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) = NULL;
	HTHEME (__stdcall *ExOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
	HRESULT (__stdcall *ExCloseThemeData)(HTHEME hTheme);

	HMODULE ExNtDll = NULL;
	VOID (__stdcall *ExRtlGetNtVersionNumbers)(LPDWORD major, LPDWORD minor, LPDWORD build) = NULL;

	BOOL bDarkModeSupport = FALSE;
	BOOL bDarkModeEnable = TRUE;
	BOOL bUserAppColor = FALSE;
	BOOL bAppColBrushInit = FALSE;
	int AppColorBase = 0;
	COLORREF AppColorTable[2][APPCOL_MAX] = { { 0 }, { 0 } };
	HBRUSH  AppColorBrush[2][APPCOL_MAX] = { { 0 }, { 0 } };

#ifdef	USE_OLE
	CLIPFORMAT CF_FILEDESCRIPTOR = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
	CLIPFORMAT CF_FILECONTENTS = RegisterClipboardFormat(CFSTR_FILECONTENTS);
#endif
	
void InitAppColor()
{
	for ( int n = 0 ; n < APPCOL_SYSMAX ; n++ )
		AppColorTable[0][n] = GetSysColor(n);

	// 追加の色設定
	AppColorTable[0][APPCOL_MENUFACE]		= AppColorTable[0][COLOR_WINDOW];
	AppColorTable[0][APPCOL_MENUTEXT]		= AppColorTable[0][COLOR_WINDOWTEXT];
	AppColorTable[0][APPCOL_MENUHIGH]		= AppColorTable[0][COLOR_MENU];

	AppColorTable[0][APPCOL_DLGFACE]		= AppColorTable[0][COLOR_MENU];
	AppColorTable[0][APPCOL_DLGTEXT]		= AppColorTable[0][COLOR_MENUTEXT];
	AppColorTable[0][APPCOL_DLGOPTFACE]		= AppColorTable[0][COLOR_WINDOW];

	AppColorTable[0][APPCOL_BARBACK]		= AppColorTable[0][COLOR_WINDOW];
	AppColorTable[0][APPCOL_BARSHADOW]		= AppColorTable[0][COLOR_BTNSHADOW];
	AppColorTable[0][APPCOL_BARFACE]		= AppColorTable[0][COLOR_MENU];
	AppColorTable[0][APPCOL_BARHIGH]		= AppColorTable[0][COLOR_WINDOW];
	AppColorTable[0][APPCOL_BARBODER]		= AppColorTable[0][COLOR_BTNSHADOW];
	AppColorTable[0][APPCOL_BARTEXT]		= AppColorTable[0][COLOR_MENUTEXT];

	AppColorTable[0][APPCOL_TABFACE]		= AppColorTable[0][COLOR_MENU];
	AppColorTable[0][APPCOL_TABTEXT]		= AppColorTable[0][COLOR_MENUTEXT];
	AppColorTable[0][APPCOL_TABHIGH]		= AppColorTable[0][COLOR_WINDOW];
	AppColorTable[0][APPCOL_TABSHADOW]		= AppColorTable[0][COLOR_BTNSHADOW];

	AppColorTable[0][APPCOL_CTRLFACE]		= AppColorTable[0][COLOR_WINDOW];
	AppColorTable[0][APPCOL_CTRLTEXT]		= AppColorTable[0][COLOR_WINDOWTEXT];
	AppColorTable[0][APPCOL_CTRLHIGH]		= RGB(0, 120, 215);
	AppColorTable[0][APPCOL_CTRLHTEXT]		= RGB(255, 255, 255);
	AppColorTable[0][APPCOL_CTRLSHADOW]		= RGB(185, 209, 234);

	// ダークモードの色設定
	memcpy(AppColorTable[1], AppColorTable[0], sizeof(AppColorTable[1]));

	AppColorTable[1][COLOR_WINDOW]			= DARKMODE_BACKCOLOR;
	AppColorTable[1][COLOR_WINDOWTEXT]		= DARKMODE_TEXTCOLOR;
	AppColorTable[1][COLOR_WINDOWFRAME]		= DARKMODE_BACKCOLOR;

	AppColorTable[1][COLOR_MENU]			= DARKMODE_BACKCOLOR;
	AppColorTable[1][COLOR_MENUTEXT]		= AppColorTable[0][COLOR_MENU];
	AppColorTable[1][COLOR_MENUHILIGHT]		= AppColorTable[0][COLOR_MENUTEXT];
	AppColorTable[1][COLOR_MENUBAR]			= DARKMODE_BACKCOLOR;

	AppColorTable[1][COLOR_BTNFACE]			= DARKMODE_BACKCOLOR;
	AppColorTable[1][COLOR_BTNTEXT]			= DARKMODE_TEXTCOLOR;
	AppColorTable[1][COLOR_BTNSHADOW]		= AppColorTable[0][COLOR_WINDOWFRAME];
	AppColorTable[1][COLOR_BTNHIGHLIGHT]	= AppColorTable[0][COLOR_BTNSHADOW];

	AppColorTable[1][COLOR_3DDKSHADOW]	    = AppColorTable[0][COLOR_3DLIGHT];
	AppColorTable[1][COLOR_3DLIGHT]	        = AppColorTable[0][COLOR_3DDKSHADOW];

	// 追加のダークモードの色設定
	AppColorTable[1][APPCOL_MENUFACE]		= DARKMODE_BACKCOLOR;
	AppColorTable[1][APPCOL_MENUTEXT]		= DARKMODE_TEXTCOLOR;
	AppColorTable[1][APPCOL_MENUHIGH]		= AppColorTable[0][COLOR_MENUTEXT];

	AppColorTable[1][APPCOL_DLGFACE]		= DARKMODE_BACKCOLOR;
	AppColorTable[1][APPCOL_DLGTEXT]		= DARKMODE_TEXTCOLOR;
	AppColorTable[1][APPCOL_DLGOPTFACE]		= DARKMODE_BACKCOLOR;

	AppColorTable[1][APPCOL_BARBACK]		= DARKMODE_BACKCOLOR;
	AppColorTable[1][APPCOL_BARSHADOW]		= AppColorTable[0][COLOR_BTNSHADOW];
	AppColorTable[1][APPCOL_BARFACE]		= DARKMODE_BACKCOLOR;
	AppColorTable[1][APPCOL_BARHIGH]		= AppColorTable[0][COLOR_BACKGROUND];
	AppColorTable[1][APPCOL_BARBODER]		= AppColorTable[0][COLOR_BTNSHADOW];
	AppColorTable[1][APPCOL_BARTEXT]		= DARKMODE_TEXTCOLOR;

#ifdef	USE_DARKMODE
	AppColorTable[1][APPCOL_TABFACE]		= AppColorTable[1][COLOR_MENU];
	AppColorTable[1][APPCOL_TABTEXT]		= AppColorTable[1][COLOR_MENUTEXT];
	AppColorTable[1][APPCOL_TABHIGH]		= AppColorTable[1][COLOR_BACKGROUND];
	AppColorTable[1][APPCOL_TABSHADOW]		= AppColorTable[1][COLOR_BTNSHADOW];
#else
	AppColorTable[1][APPCOL_TABFACE]		= AppColorTable[0][COLOR_MENU];
	AppColorTable[1][APPCOL_TABTEXT]		= AppColorTable[0][COLOR_MENUTEXT];
	AppColorTable[1][APPCOL_TABHIGH]		= AppColorTable[0][COLOR_WINDOW];
	AppColorTable[1][APPCOL_TABSHADOW]		= AppColorTable[0][COLOR_BTNSHADOW];
#endif

	AppColorTable[1][APPCOL_CTRLFACE]		= AppColorTable[1][COLOR_WINDOW];
	AppColorTable[1][APPCOL_CTRLTEXT]		= AppColorTable[1][COLOR_WINDOWTEXT];
	AppColorTable[1][APPCOL_CTRLHIGH]		= RGB(0, 100, 190);
	AppColorTable[1][APPCOL_CTRLHTEXT]		= RGB(255, 255, 255);
	AppColorTable[1][APPCOL_CTRLSHADOW]		= RGB(56, 96, 124);

	// ブラシの初期化
	if ( !bAppColBrushInit ) {
		ZeroMemory(AppColorBrush, sizeof(AppColorBrush));
		bAppColBrushInit = TRUE;
	} else {
		for ( int n = 0 ; n < 2 ; n++ ) {
			for ( int i = 0 ; i < APPCOL_MAX ; i++ ) {
				if ( AppColorBrush[n][i] != NULL ) {
					DeleteObject(AppColorBrush[n][i]);
					AppColorBrush[n][i] = NULL;
				}
			}
		}
	}

	bUserAppColor = FALSE;
}
void LoadAppColor()
{
	int base, idx;
	CStringIndex index;
	CString BaseName, IndexName;

	bUserAppColor = FALSE;
	theApp.GetProfileStringIndex(_T("RLoginApp"), _T("AppColTab"), index);

	for ( int n = 0 ; n < 2 ; n++ ) {
		BaseName.Format(_T("%d"), n);
		if ( (base = index.Find(BaseName)) >= 0 ) { 
			for ( int i = 0 ; i < APPCOL_MAX ; i++ ) {
				IndexName.Format(_T("%d"), i);
				if ( (idx = index[base].Find(IndexName)) >= 0 ) {
					AppColorTable[n][i] = _tstoi(index[base][idx]);
					bUserAppColor = TRUE;
				}
			}
		}
	}
}
void SaveAppColor()
{
	CStringIndex index;
	CString BaseName, IndexName, ColorName;
	COLORREF SaveColTab[2][APPCOL_MAX];

	bUserAppColor = FALSE;
	memcpy(SaveColTab, AppColorTable, sizeof(SaveColTab));
	InitAppColor();

	for ( int n = 0 ; n < 2 ; n++ ) {
		BaseName.Format(_T("%d"), n);
		for ( int i = 0 ; i < APPCOL_MAX ; i++ ) {
			if ( AppColorTable[n][i] != SaveColTab[n][i] ) {
				IndexName.Format(_T("%d"), i);
				ColorName.Format(_T("%d"), SaveColTab[n][i]);
				index[BaseName][IndexName] = ColorName;
				bUserAppColor = TRUE;
			}
		}
	}
	
	memcpy(AppColorTable, SaveColTab, sizeof(AppColorTable));
	theApp.WriteProfileStringIndex(_T("RLoginApp"), _T("AppColTab"), index);
}
HBRUSH GetAppColorBrush(int nIndex)
{
	if ( AppColorBrush[AppColorBase][nIndex] == NULL )
		AppColorBrush[AppColorBase][nIndex] = CreateSolidBrush(AppColorTable[AppColorBase][nIndex]);

	return AppColorBrush[AppColorBase][nIndex];
}

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
BOOL ExDwmDarkMode(HWND hWnd)
{
#ifdef	USE_DWMAPI
	BOOL bSetAttr = FALSE;
	DWORD dwUseLight = 1;
	DWORD dwAttribute = 0;

	if ( ExDwmApi != NULL && ExDwmSetWindowAttribute != NULL ) {
		#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
			#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
		#endif
		#ifndef DWMWA_BORDER_COLOR
			#define	DWMWA_BORDER_COLOR	34
		#endif

		if ( bDarkModeEnable && theApp.RegisterGetDword(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), _T("AppsUseLightTheme"), &dwUseLight) && !dwUseLight )
			dwAttribute = 1;

		bSetAttr = TRUE;
	}

	if ( bDarkModeSupport && bSetAttr && hWnd != NULL ) {
		//COLORREF brc = (dwAttribute ? 0xFFFFFFFE : 0xFFFFFFFF);
		//ExDwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &brc, sizeof(brc));

		ExDwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dwAttribute, sizeof(dwAttribute));

		AppColorBase = (dwAttribute ? 1 : 0);
		return (dwAttribute != 0 ? TRUE : FALSE);
	}
#endif
	return FALSE;
}
void ExDarkModeEnable(BOOL bEnable)
{
	// メニューがダークモードになるが非公開関数
	if ( !bDarkModeSupport && AllowDarkModeForApp != NULL && ExRtlGetNtVersionNumbers != NULL ) {
		DWORD major = 0, minor = 0, build = 0;
		ExRtlGetNtVersionNumbers(&major, &minor, &build); build &= 0x0FFFFFFF;
		if ( major == 10 && minor == 0 && build >= 17763 )	// // Windows 10 1809 (10.0.17763)
			bDarkModeSupport = TRUE;
	}

	if ( bDarkModeSupport ) {
		AllowDarkModeForApp(bEnable ? 1 : 0);
		bDarkModeEnable = bEnable;
	}
}

static void AddErrorMessage(CString &str)
{
	LPVOID lpMessageBuffer;
	DWORD err = ::GetLastError();

	if ( err != 0 ) {
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMessageBuffer, 0, NULL);
		str += _T("\n\n");
		str += (LPTSTR)lpMessageBuffer;
		LocalFree(lpMessageBuffer);

	} else if ( errno != 0 ) {
		str += _T("\n\n");
		str += _tcserror(errno);
	}
}
LPCTSTR FormatErrorMessage(CString &str, LPCTSTR msg, ...)
{
	va_list arg;

	va_start(arg, msg);
	str.FormatV(msg, arg);

	AddErrorMessage(str);

	return str;
}
int ThreadMessageBox(LPCTSTR msg, ...)
{
	va_list arg;
	CString tmp;

	va_start(arg, msg);
	tmp.FormatV(msg, arg);

	AddErrorMessage(tmp);

	return ::AfxMessageBox(tmp, MB_ICONERROR);
}
int DoitMessageBox(LPCTSTR lpszPrompt, UINT nType, CWnd *pParent)
{
	if ( pParent != NULL && (pParent->IsIconic() || !pParent->IsWindowVisible()) )
		pParent->ShowWindow(SW_SHOWNORMAL);

	CMsgChkDlg dlg(pParent);

	dlg.m_MsgText = lpszPrompt;
	dlg.m_nType = nType;
	dlg.m_bNoChkEnable = FALSE;

	// エラーステータスをリセット
	::SetLastError(0);
	_set_errno(0);

	return (int)dlg.DoModal();
}

BOOL WaitForEvent(HANDLE hHandle, LPCTSTR pMsg)
{
	DWORD rs = WaitForSingleObject(hHandle, 5000);

	switch(rs) {
	case WAIT_ABANDONED:
		ThreadMessageBox(_T("WaitForEvent abandoned '%s'"), pMsg);
		return FALSE;
	case WAIT_TIMEOUT:
		ThreadMessageBox(_T("WaitForEvent timeout '%s'"), pMsg);
		return FALSE;
	case WAIT_FAILED:
		ThreadMessageBox(_T("WaitForEvent failed '%s'"), pMsg);
		return FALSE;
	}
	return TRUE;
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

	if ( FAILED(pSheLink->SetPath(szExePath)) )
		goto ERRENDOF;

	if ( FAILED(pSheLink->SetArguments(param)) )
		goto ERRENDOF;

	if ( FAILED(pSheLink->SetIconLocation(szExePath, 1)) )
			goto ERRENDOF;

	if ( FAILED(pSheLink->SetDescription(pEntryName)) )
			goto ERRENDOF;

	IPropertyStore *pProStore;
	PROPVARIANT pv;

	if ( FAILED(pSheLink->QueryInterface(IID_PPV_ARGS(&pProStore))) )
		goto ERRENDOF;

	InitPropVariantFromString(pEntryName, &pv);
	pProStore->SetValue(PKEY_Title, pv);
	pProStore->Commit();

	PropVariantClear(&pv);
	pProStore->Release();

	return pSheLink;

ERRENDOF:
	pSheLink->Release();
	return NULL;
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
	
void CRLoginApp::SSL_Init()
{
	static BOOL bLoadAlgo = FALSE;

	if ( bLoadAlgo )
		return;
	bLoadAlgo = TRUE;

	OPENSSL_init_ssl(0, NULL);
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
			if ( ::AfxMessageBox(CStringLoad(IDS_PASSLOCKDELMSG), MB_ICONQUESTION | MB_YESNO) == IDYES ) {
				WriteProfileString(_T("RLoginApp"), _T("LocalPass"), _T(""));
				return TRUE;
			}
			return FALSE;
		}

		key.Digest(m_LocalPass, dlg.m_Edit);
		key.DecryptStr(tmp, passok);

		if ( tmp.Compare(_T("01234567")) == 0 ) {
			if ( CompNameLenBugFix == FALSE ) {
				// MAX_COMPUTERNAME_LENGTHのバグを修正
				CompNameLenBugFix = TRUE;
				key.EncryptStr(passok, _T("01234567"));
				WriteProfileString(_T("RLoginApp"), _T("LocalPass"), passok);
				CompNameLenBugFix = FALSE;	// 後でチェックする為に戻す
			}
			return TRUE;
		}
	}

	return FALSE;
}

#if	_MSC_VER < _MSC_VER_VS05
BOOL CRLoginApp::IsWinVerCheck(int ver, int op)
{
	if ( ExRtlGetNtVersionNumbers == NULL )
		return FALSE;

	DWORD major, minor, build;
    ExRtlGetNtVersionNumbers(&major, &minor, &build); build &= 0x0FFFFFFF;
	int osver = (major << 8) | (minor & 0xFF);

	switch(op) {
	case VER_EQUAL:				// ==
		return (osver == ver ? TRUE : FALSE);
		break;
	case VER_GREATER:			// >
		return (osver > ver ? TRUE : FALSE);
		break;
	case VER_GREATER_EQUAL:		// >=
		return (osver >= ver ? TRUE : FALSE);
		break;
	case VER_LESS:				// <
		return (osver < ver ? TRUE : FALSE);
		break;
	case VER_LESS_EQUAL:		// <=
		return (osver <= ver ? TRUE : FALSE);
		break;
	}

	return FALSE;
}
#elif _MSC_VER < _MSC_VER_VS13
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
#else

#include <VersionHelpers.h>

BOOL CRLoginApp::IsWinVerCheck(int ver, int op)
{
	switch(op) {
	case VER_EQUAL:				// ==
		switch(ver) {
		case _WIN32_WINNT_WINXP:
			return (IsWindowsXPOrGreater() && !IsWindowsVistaOrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_VISTA:
			return (IsWindowsVistaOrGreater() && !IsWindows7OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN7:
			return (IsWindows7OrGreater() && !IsWindows8OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN8:
			return (IsWindows8OrGreater() && !IsWindows10OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WINBLUE:
			return (IsWindows8Point1OrGreater() && !IsWindows10OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN10:
			return (IsWindows10OrGreater() ? TRUE : FALSE);
		}
		break;
	case VER_GREATER:			// >
		switch(ver) {
		case _WIN32_WINNT_WINXP:
			return (IsWindowsVistaOrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_VISTA:
			return (IsWindows7OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN7:
			return (IsWindows8OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN8:
			return (IsWindows8Point1OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WINBLUE:
			return (IsWindows10OrGreater() ? TRUE : FALSE);
		}
		break;
	case VER_GREATER_EQUAL:		// >=
		switch(ver) {
		case _WIN32_WINNT_WINXP:
			return (IsWindowsXPOrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_VISTA:
			return (IsWindowsVistaOrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN7:
			return (IsWindows7OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN8:
			return (IsWindows8OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WINBLUE:
			return (IsWindows8Point1OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN10:
			return (IsWindows10OrGreater() ? TRUE : FALSE);
		}
		break;
	case VER_LESS:				// <
		switch(ver) {
		case _WIN32_WINNT_WINXP:
			return (!IsWindowsXPOrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_VISTA:
			return (!IsWindowsVistaOrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN7:
			return (!IsWindows7OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN8:
			return (!IsWindows8OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WINBLUE:
			return (!IsWindows8Point1OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN10:
			return (!IsWindows10OrGreater() ? TRUE : FALSE);
		}
		break;
	case VER_LESS_EQUAL:		// <=
		switch(ver) {
		case _WIN32_WINNT_WINXP:
			return (!IsWindowsVistaOrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_VISTA:
			return (!IsWindows7OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN7:
			return (!IsWindows8OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WIN8:
			return (!IsWindows8Point1OrGreater() ? TRUE : FALSE);
		case _WIN32_WINNT_WINBLUE:
			return (!IsWindows10OrGreater() ? TRUE : FALSE);
		}
		break;
	}

	return FALSE;
}
#endif

BOOL CRLoginApp::GetExtFilePath(LPCTSTR name, LPCTSTR ext, CString &path)
{
	if ( name == NULL )
		name = m_pszAppName;

	path.Format(_T("%s\\%s%s"), (LPCTSTR)m_BaseDir, name, ext);
	if ( _taccess_s(path, 06) == 0 )
		return TRUE;

	path.Format(_T("%s\\%s%s"), (LPCTSTR)m_ExecDir, name, ext);
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

	return rt;
}

//////////////////////////////////////////////////////////////////////

#ifdef	OPENSSL_DEBUG
void *Debug_Malloc(size_t num, const char *file, int line)
{
	void *adr = _malloc_dbg(num, _NORMAL_BLOCK, file, line); // malloc(num);
	return adr;
}
void OpenSSL_Memory_Test()
{
	CRYPTO_malloc_fn malloc_fn;
	CRYPTO_realloc_fn realloc_fn;
	CRYPTO_free_fn free_fn;

	CRYPTO_get_mem_functions(&malloc_fn, &realloc_fn, &free_fn);
	CRYPTO_set_mem_functions(Debug_Malloc, realloc_fn, free_fn);
}
#endif

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

	// OS設定のSystem CodePageをiconvでの名前で保存
	LPCTSTR p = CIConv::GetCodePageName(_getmbcp());
	SystemIconv = (p != NULL ? p : _T("UTF-8"));

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
	if ( !AfxOleInit() ) {
		::AfxMessageBox(_T("OLE Init Error"), MB_ICONERROR);
		return FALSE;
	}
#endif

	// COMライブラリ初期化
	if ( FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) ) {
		::AfxMessageBox(_T("Com Library Init Error"), MB_ICONERROR);
		return FALSE;
	}

	// WINSOCK2.2の初期化
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD( 2, 2 );
	if ( WSAStartup( wVersionRequested, &wsaData ) != 0 ) {
		::AfxMessageBox(CStringLoad(IDS_SOCKETS_INIT_FAILED), MB_ICONERROR);
		return FALSE;
	}

	// opensslの初期化
	m_ProvDefault = OSSL_PROVIDER_load(NULL, "default");
	m_ProvLegacy  = OSSL_PROVIDER_load(NULL, "legacy");

	OSSL_PROVIDER_add_builtin(NULL, "rlogin", RLOGIN_provider_init);
	m_ProvRlogin = OSSL_PROVIDER_load(NULL, "rlogin");

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
	if ( GetExtFilePath(NULL, _T(".ini"), iniFileName) ) {
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

	// アプリカラーテーブルの初期化
	InitAppColor();
	LoadAppColor();

#ifdef	USE_RCDLL
	// リソースDLL読み込み
	CString rcDllName;
	if ( GetExtFilePath(NULL, _T("_rc.dll"), rcDllName) && (ExRcDll = LoadLibrary(rcDllName)) != NULL )
		AfxSetResourceHandle(ExRcDll);
#endif

	// リソースデータベースの設定
	// m_ResDataBase.InitRessource();
	CString rcFileName;
	if ( GetExtFilePath(NULL, _T("_rc.txt"), rcFileName) ) {
		if ( !m_ResDataBase.LoadFile(rcFileName) )
			::AfxMessageBox(_T("Can't Load Resources File"), MB_ICONWARNING);
	} else {
		TCHAR locale[256];
		LANGID id = GetUserDefaultUILanguage();

		if ( GetLocaleInfo(MAKELCID(id, SORT_DEFAULT), LOCALE_SISO639LANGNAME, locale, sizeof(locale)) != 0 ) {
			CString base, ext;
			base.Format(_T("lang\\%s"), m_pszAppName);
			ext.Format(_T("_%s.txt"), locale);
			if ( GetExtFilePath(base, ext, rcFileName) ) {
				if ( !m_ResDataBase.LoadFile(rcFileName) )
					::AfxMessageBox(_T("Can't Load Locale Resources File"), MB_ICONWARNING);
			}
		}
	}

	// デフォルトツールバーイメージからBitmapリソースを作成
	InitToolBarBitmap(MAKEINTRESOURCE(IDR_MAINFRAME), IDB_BITMAP1);
	InitToolBarBitmap(MAKEINTRESOURCE(IDR_TOOLBAR2),  IDB_BITMAP10);
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
		ExDwmSetWindowAttribute        = (HRESULT (__stdcall *)(HWND hwnd, DWORD dwAttribute, __in_bcount(cbAttribute) LPCVOID pvAttribute, DWORD cbAttribute))GetProcAddress(ExDwmApi, "DwmSetWindowAttribute");

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

	// CancelIoExライブラリを取得
	if ( (ExKernel32Api = LoadLibrary(_T("Kernel32.dll"))) != NULL )
		ExCancelIoEx = (BOOL (WINAPI *)(HANDLE hFile, LPOVERLAPPED lpOverlapped))GetProcAddress(ExKernel32Api, "CancelIoEx");

	// ダークモード関連の関数
	if ( (ExUxThemeDll = LoadLibrary(_T("uxtheme.dll"))) != NULL ) {
		AllowDarkModeForWindow = (BOOL (__stdcall *)(HWND hwnd, BOOL allow))GetProcAddress(ExUxThemeDll, MAKEINTRESOURCEA(133));
		AllowDarkModeForApp	= (BOOL (__stdcall *)(int mode))GetProcAddress(ExUxThemeDll, MAKEINTRESOURCEA(135));
		ExSetWindowTheme = (HRESULT (__stdcall *)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList))GetProcAddress(ExUxThemeDll, "SetWindowTheme");
		ExOpenThemeData = (HTHEME (__stdcall *)(HWND hwnd, LPCWSTR pszClassList))GetProcAddress(ExUxThemeDll, "OpenThemeData");
		ExCloseThemeData = (HRESULT (__stdcall *)(HTHEME hTheme))GetProcAddress(ExUxThemeDll, "CloseThemeData");
	}

	// NTのバージョン取得関数
	if ( (ExNtDll = LoadLibrary(_T("ntdll.dll"))) != NULL ) {
		ExRtlGetNtVersionNumbers = (VOID (__stdcall *)(LPDWORD major, LPDWORD minor, LPDWORD build))GetProcAddress(ExNtDll, "RtlGetNtVersionNumbers");
	}

#ifdef	USE_DIRECTWRITE
	// DirectWriteを試す
	if ( IsWinVerCheck(_WIN32_WINNT_WINBLUE, VER_GREATER_EQUAL) ) {
		if ( (ExD2D1Api = LoadLibrary(_T("d2d1.dll"))) != NULL && (ExDWriteApi = LoadLibrary(_T("dwrite.dll"))) != NULL ) {
			ExD2D1CreateFactory = (HRESULT (WINAPI *)(__in D2D1_FACTORY_TYPE factoryType, __in REFIID riid, __in_opt CONST D2D1_FACTORY_OPTIONS *pFactoryOptions, __out void **ppIFactory))GetProcAddress(ExD2D1Api, "D2D1CreateFactory");
			ExDWriteCreateFactory = (HRESULT (WINAPI *)(__in DWRITE_FACTORY_TYPE factoryType, __in REFIID iid, __out IUnknown **factory))GetProcAddress(ExDWriteApi, "DWriteCreateFactory");

			if ( ExD2D1CreateFactory != NULL && ExDWriteCreateFactory != NULL && SUCCEEDED(ExD2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), NULL, reinterpret_cast<void **>(&m_pD2DFactory))) )
				ExDWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown **>(&m_pDWriteFactory));
		}
	}

	m_EmojiFontName = GetProfileString(_T("RLoginApp"), _T("EmojiFontName"), _T("Segoe UI emoji"));
	m_EmojiImageDir = GetProfileString(_T("RLoginApp"), _T("EmojiImageDir"), _T(""));

	if ( m_EmojiImageDir.IsEmpty() || !IsDirectory(m_EmojiImageDir) ) {
		m_EmojiImageDir.Format(_T("%s\\emoji"), (LPCTSTR)m_BaseDir);
		if ( !IsDirectory(m_EmojiImageDir) ) {
			m_EmojiImageDir.Format(_T("%s\\emoji"), (LPCTSTR)m_ExecDir);
			if ( !IsDirectory(m_EmojiImageDir) )
				m_EmojiImageDir.Empty();
		}
	}
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

	if ( !pDocTemplate ) {
		::AfxMessageBox(_T("DocTemplate Create Error"), MB_ICONERROR);
		return FALSE;
	}

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

	if ( !cmdInfo.m_Cwd.IsEmpty() ) {
		if ( SetCurrentDirectory(cmdInfo.m_Cwd) )
			m_BaseDir = cmdInfo.m_Cwd;
	}
	
	// ダークモードのサポート
	ExDarkModeEnable(cmdInfo.m_DarkOff ? FALSE : GetProfileInt(_T("RLoginApp"), _T("DarkMode"), TRUE));

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
		//AfxMessageBox(_T("MainFrame Create Error"));
		//delete pMainFrame;
		m_pMainWnd = NULL;
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

	// MAX_COMPUTERNAME_LENGTHのバグをチェック
	if ( (CompNameLenBugFix = GetProfileInt(_T("RLoginApp"), _T("CompNameLenBugFix"), FALSE)) == FALSE ) {
		DWORD size;
		TCHAR buf[MAX_COMPUTERNAME_LENGTH + 2];

		CompNameLenBugFix = TRUE;

		size = MAX_COMPUTERNAME_LENGTH;
		if ( !GetUserName(buf, &size) )
			CompNameLenBugFix = FALSE;
		size = MAX_COMPUTERNAME_LENGTH;
		if ( !GetComputerName(buf, &size) )
			CompNameLenBugFix = FALSE;

		WriteProfileInt(_T("RLoginApp"), _T("CompNameLenBugFix"), TRUE);
	}

	// パスワードロックの確認
	if ( !InitLocalPass() )
		return FALSE;

	// プログラム識別子を設定
	::SetWindowLongPtr(m_pMainWnd->GetSafeHwnd(), GWLP_USERDATA, 0x524c4f47);

	// レジストリベースのサーバーエントリーの読み込み
	pMainFrame->m_ServerEntryTab.Serialize(FALSE);

	if ( CompNameLenBugFix == FALSE ) {
		// MAX_COMPUTERNAME_LENGTHのバグを修正
		CompNameLenBugFix = TRUE;
		if ( pMainFrame->m_ServerEntryTab.GetSize() > 0 ) {
			pMainFrame->m_ServerEntryTab.Serialize(TRUE);
			::AfxMessageBox(IDS_COMPNAMELENBUGFIX, MB_ICONINFORMATION | MB_OK);
		}
	}

	// ユーザープロファイルの更新を確認
	if ( m_pszRegistryKey == NULL && GetProfileInt(_T("RLoginApp"), _T("CompressPrivateProfile"), 0) == 0 ) {
		if ( ::AfxMessageBox(IDS_COMPPRIVATEPROFILE, MB_ICONWARNING | MB_YESNO) != IDYES )
			return FALSE;
		WriteProfileInt(_T("RLoginApp"), _T("CompressPrivateProfile"), 1);
		pMainFrame->m_ServerEntryTab.Serialize(TRUE);
	}

	// クイックバーの初期化
	pMainFrame->QuickBarInit();

	// プログラムバージョンチェック
	pMainFrame->VersionCheck();

#ifdef	USE_JUMPLIST
	// ジャンプリスト初期化
	if ( IsWinVerCheck(_WIN32_WINNT_WIN7, VER_GREATER_EQUAL) && IsWinVerCheck(_WIN32_WINNT_WINBLUE, VER_LESS_EQUAL) )
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

	m_TempDirPath.Format(_T("%sTemp%04d\\"), (LPCTSTR)m_TempDirBase, m_TempSeqId++);
	CreateDirectory(m_TempDirPath, NULL);

	return m_TempDirPath;
}

int CRLoginApp::ExitInstance() 
{
	// Free Handle or Library
	rand_buf(NULL, 0);

	RLOGIN_provider_finish();

	if ( m_ProvRlogin != NULL )
		OSSL_PROVIDER_unload(m_ProvRlogin);
	if ( m_ProvLegacy != NULL )
		OSSL_PROVIDER_unload(m_ProvLegacy);
	if ( m_ProvDefault != NULL )
		OSSL_PROVIDER_unload(m_ProvDefault);

	CONF_modules_finish();
	CONF_modules_unload(1);

	OPENSSL_cleanup();

	WSACleanup();

#ifdef	USE_DWMAPI
	if ( ExDwmApi != NULL )
		FreeLibrary(ExDwmApi);
#endif

	if ( ExShcoreApi != NULL )
		FreeLibrary(ExShcoreApi);

	if ( ExUserApi != NULL )
		FreeLibrary(ExUserApi);

	if ( ExKernel32Api != NULL )
		FreeLibrary(ExKernel32Api);

	if ( ExUxThemeDll != NULL )
		FreeLibrary(ExUxThemeDll);

	if ( ExNtDll != NULL )
		FreeLibrary(ExNtDll);

#ifdef	USE_RCDLL
	if ( ExRcDll != NULL )
		FreeLibrary(ExRcDll);
#endif

#ifdef	USE_DIRECTWRITE
	EmojiImageFinish();

	// WM_DRAWEMOJIが処理されずに終了する場合の処置
	if ( m_EmojiPostStat == EMOJIPOSTSTAT_POST && m_pEmojiPostDocPos != NULL ) {
		CEmojiDocPos *pDocPos;
		while ( (pDocPos = CEmojiDocPos::RemoveHead(&m_pEmojiPostDocPos)) != NULL )
			delete pDocPos;
	}

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

	VoiceFinis();
	CoUninitialize();

	CSFtp::LocalDelete(m_TempDirBase);

	while ( !m_PostMsgQue.IsEmpty() ) {
		PostMsgQue *pQue = m_PostMsgQue.RemoveHead();
		delete pQue;
	}

	SaveAppColor();

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

			param.Format(_T("%s /sx %d /sy %d /inuse"), (LPCTSTR)pDoc->m_CmdLine, pPoint->x, pPoint->y);

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

		param.Format(_T("%s /sx %d /sy %d"), (LPCTSTR)pDoc->m_CmdLine, pPoint->x, pPoint->y);
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

	param.Format(_T("%s /nothing /event %s"), m_pszAppName, (LPCTSTR)tmp);

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

	str.Format(_T("TEST%s"), (LPCTSTR)m_LocalPass);
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

BOOL CRLoginApp::GetProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE* ppData, UINT* pBytes)
{
	if ( m_pszRegistryKey != NULL )
		return CWinApp::GetProfileBinary(lpszSection, lpszEntry, ppData, pBytes);

	else {	// private profile
		LPCTSTR p;
		CString compEntry, origEntry;

		for ( p = lpszEntry ; *p != _T('\0') ; p++ );
		if ( (p -= 2) >= lpszEntry && p[0] == _T('_') && p[1] == _T('#') ) {
			while ( lpszEntry < p )
				origEntry += *(lpszEntry++);
			lpszEntry = origEntry;
		}

		if ( CWinApp::GetProfileBinary(lpszSection, lpszEntry, ppData, pBytes) && *ppData != NULL )
			return TRUE;

		compEntry.Format(_T("%s_#"), lpszEntry);

		if ( !CWinApp::GetProfileBinary(lpszSection, compEntry, ppData, pBytes) || *ppData == NULL )
			return FALSE;

		if ( *pBytes < sizeof(UINT) )	// ???
			return TRUE;

		uLong compSize = *((UINT *)(*ppData));
		BYTE *compBuff = new BYTE[compSize];

		if ( uncompress(compBuff, &compSize, *ppData + sizeof(UINT), *pBytes - sizeof(UINT)) != Z_OK ) {
			delete [] compBuff;
			delete *ppData;
			*ppData = NULL;
			*pBytes = 0;
			return FALSE;
		}

		delete *ppData;
		*ppData = compBuff;
		*pBytes = compSize;
		return TRUE;
	}
}
BOOL CRLoginApp::WriteProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes)
{
	if ( m_pszRegistryKey != NULL )
		return CWinApp::WriteProfileBinary(lpszSection, lpszEntry, pData, nBytes);

	else {	// private profile
		LPCTSTR p;
		uLong compSize = compressBound(nBytes);
		BYTE *compBuff = new BYTE[compSize + sizeof(UINT)];
		CString compEntry, origEntry;
		BOOL bRet;

		for ( p = lpszEntry ; *p != _T('\0') ; p++ );
		if ( (p -= 2) >= lpszEntry && p[0] == _T('_') && p[1] == _T('#') ) {
			while ( lpszEntry < p )
				origEntry += *(lpszEntry++);
			lpszEntry = origEntry;
		}

		*((UINT *)compBuff) = nBytes;
		compEntry.Format(_T("%s_#"), lpszEntry);

		if ( nBytes <= 32 || compress2(compBuff + sizeof(UINT), &compSize, pData, nBytes, 9) != Z_OK || nBytes <= (compSize + sizeof(UINT)) ) {
			bRet = CWinApp::WriteProfileBinary(lpszSection, lpszEntry, pData, nBytes);
			WritePrivateProfileString(lpszSection, compEntry, NULL, m_pszProfileName);
		} else {
			bRet = CWinApp::WriteProfileBinary(lpszSection, compEntry, compBuff, compSize + sizeof(UINT));
			WritePrivateProfileString(lpszSection, lpszEntry, NULL, m_pszProfileName);
		}

		delete [] compBuff;
		return bRet;
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

	if ( GetProfileBinary(lpszSection, lpszEntry, &pData, &len) && pData != NULL && len > 0 ) {
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

	if ( GetProfileBinary(lpszSection, lpszEntry, &pData, &len) && pData != NULL && len > 0 )
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
			if ( key.Right(2).Compare(_T("_#")) == 0 )
				key.Delete(key.GetLength() - 2, 2);
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

		CString compEntry;
		compEntry.Format(_T("%s_#"), lpszEntry);
		WritePrivateProfileString(lpszSection, compEntry, NULL, m_pszProfileName);
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
		::AfxMessageBox(CStringLoad(IDE_REGISTRYDELETEERROR));
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

	strTemp.Format(_T("%s,%d"), (LPCTSTR)strPathName, 1);
	RegisterSetStr(HKEY_CURRENT_USER, _T("Software\\Classes\\RLogin.Document\\DefaultIcon"), _T(""), strTemp);

	strTemp.Format(_T("%s /inuse \"%%1\""), (LPCTSTR)strPathName);
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
		if ( ::AfxMessageBox(CStringLoad(IDS_REGISTAPPPROTOCOL), MB_ICONQUESTION | MB_YESNO) != IDYES )
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
		strTemp.Format(_T("%s,%d"), (LPCTSTR)strPathName, 1);
		RegisterSetStr(HKEY_CURRENT_USER, strSection, _T(""), strTemp);

		strSection.Format(_T("Software\\Classes\\%s\\shell\\open\\command"), pSection);
		strTemp.Format(_T("%s %s \"%%1\""), (LPCTSTR)strPathName, pOption);
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
BOOL CRLoginApp::RegisterGetDword(HKEY hKey, LPCTSTR pSection, LPCTSTR pEntryName, DWORD *pDword)
{
	HKEY hSubKey;
	DWORD type, size;
	BOOL ret = FALSE;

	if ( RegOpenKeyEx(hKey, pSection, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS ) {
		if ( RegQueryValueEx(hSubKey, pEntryName, NULL, &type, NULL, &size) == ERROR_SUCCESS && type == REG_DWORD && size == sizeof(DWORD) ) {
			if ( RegQueryValueEx(hSubKey, pEntryName, NULL, NULL, (LPBYTE)pDword, &size) == ERROR_SUCCESS )
				ret = TRUE;
		}
		RegCloseKey(hSubKey);
	}

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

BOOL CRLoginApp::SavePrivateProfileKey(HKEY hKey, CFile *file, BOOL bRLoginApp)
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
	BOOL bCompCheck = FALSE;

	for ( n = 0 ; ; n++ ) {
		len = 1024;
		if ( RegEnumKeyEx(hKey, n, name, &len, NULL, NULL, NULL, &last) != ERROR_SUCCESS )
			break;

		if ( RegOpenKeyEx(hKey, name, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS )
			return FALSE;

		mbs.Format("[%s]\r\n", TstrToMbs(name));
		file->Write((LPCSTR)mbs, mbs.GetLength() * sizeof(CHAR));

		if ( !SavePrivateProfileKey(hSubKey, file, _tcscmp(name, _T("RLoginApp")) == 0 ? TRUE : FALSE) ) {
			RegCloseKey(hSubKey);
			return FALSE;
		}

		RegCloseKey(hSubKey);
	}

	for ( n = 0 ; ; n++ ) {
		len = 1024;
		if ( RegEnumValue(hKey, n, name, &len, NULL, &type, NULL, &size) != ERROR_SUCCESS )
			break;

		if ( pData == NULL || max < (size + 2) ) {
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
			if ( bRLoginApp && _tcscmp(name, _T("CompressPrivateProfile")) == 0 )
				bCompCheck = TRUE;
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
			if ( size > 32 ) {
				uLong compSize = compressBound(size);
				BYTE *compBuff = new BYTE[compSize + sizeof(UINT)];
				*((UINT *)compBuff) = size;
				if ( compress2(compBuff + sizeof(UINT), &compSize, pData, size, 9) == Z_OK && size > (compSize + sizeof(UINT)) ) {
					mbs.Format("%s_#=", TstrToMbs(name));
					size = compSize + sizeof(UINT);
					memcpy(pData, compBuff, size);
				}
				delete [] compBuff;
			}
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

	if ( bRLoginApp && !bCompCheck ) {
		mbs = "CompressPrivateProfile=1\r\n";
		file->Write((LPCSTR)mbs, mbs.GetLength() * sizeof(CHAR));
	}

	return TRUE;
}
BOOL CRLoginApp::SavePrivateProfile()
{
	CFile file;
	CString filename;
	HKEY hAppKey;
	BOOL ret = FALSE;

	filename.Format(_T("%s\\RLogin.ini"), (LPCTSTR)m_BaseDir);
	CFileDialog dlg(FALSE, _T("ini"), filename, OFN_OVERWRITEPROMPT, _T("Private Profile (*.ini)|*.ini|All Files (*.*)|*.*||"), AfxGetMainWnd());

	if ( DpiAwareDoModal(dlg) != IDOK )
		return FALSE;

	CWaitCursor wait;

	if ( !file.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive) ) {
		::AfxMessageBox(_T("Cann't Create Private Profile"));
		return FALSE;
	}

	if ( (hAppKey = GetAppRegistryKey()) != NULL ) {
		if ( SavePrivateProfileKey(hAppKey, &file) )
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
			str.Format(L"%s=\"%s\"\r\n", (LPCWSTR)key, TstrToUni(out));
			break;

		case REG_DWORD:
			str.Format(L"%s=dword:%08x\r\n", (LPCWSTR)key, size < sizeof(DWORD) ? 0 : *((DWORD *)pData));
			break;

		default:
			if ( type == REG_BINARY )
				str.Format(L"%s=hex:", (LPCWSTR)key);
			else
				str.Format(L"%s=hex(%x):", (LPCWSTR)key, type);

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
int CRLoginApp::SaveRegistryFile()
{
	CString key;
	CFile file;
	CString filename;
	CTime tm = CTime::GetCurrentTime();
	HKEY hAppKey;
	WCHAR bom = 0xFEFF;
	int ret = 2;
	static LPCWSTR head = L"Windows Registry Editor Version 5.00\r\n\r\n";

	filename.Format(_T("%s\\RLogin-%s.reg"), (LPCTSTR)m_BaseDir, (LPCTSTR)tm.Format(_T("%y%m%d")));
	CFileDialog dlg(FALSE, _T("reg"), filename, OFN_OVERWRITEPROMPT, _T("Registry file (*.reg)|*.reg|All Files (*.*)|*.*||"), AfxGetMainWnd());

	if ( DpiAwareDoModal(dlg) != IDOK )
		return 0;

	CWaitCursor wait;

	if ( !file.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive) )
		return 3;

	file.Write(&bom, sizeof(WCHAR)); 
	file.Write(head, (UINT)_tcslen(head) * sizeof(WCHAR));

	ASSERT(m_pszRegistryKey != NULL);
	ASSERT(m_pszProfileName != NULL);

	key.Format(_T("HKEY_CURRENT_USER\\Software\\%s\\%s"), m_pszRegistryKey, m_pszProfileName);

	if ( (hAppKey = GetAppRegistryKey()) != NULL ) {
		if ( SaveRegistryKey(hAppKey, &file, key) )
			ret = 1;
		RegCloseKey(hAppKey);
	}

	file.Close();
	return ret;
}

//////////////////////////////////////////////////////////////////////

LANGID CRLoginApp::GetLangId()
{
	HRSRC hRsrc;
	HGLOBAL hGlobal;
	void *pData;
	UINT dwLength;
	void *pValue;
	LANGID LangId = 0;

	if ( (hRsrc = FindResource(NULL, (LPCTSTR)VS_VERSION_INFO, RT_VERSION)) == NULL )
		return 0;

	if ( (hGlobal = LoadResource(NULL, hRsrc)) == NULL )
		return 0;

	if ( (pData = (void *)LockResource(hGlobal)) == NULL )
		return 0;

	if ( VerQueryValue(pData, _T("\\VarFileInfo\\Translation"), &pValue, &dwLength) ) {
		switch(dwLength) {
		case 1:	// BYTE
			LangId = (LANGID)*((BYTE *)pValue);
			break;
		case 2:	// WORD
			LangId = (LANGID)*((WORD *)pValue);
			break;
		case 4:	// DWORD
			LangId = (LANGID)*((DWORD *)pValue);
			break;
		}
	}

	FreeResource(hGlobal);

	return LangId;
}
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
	
	ASSERT(m_nThreadID == GetCurrentThreadId());

	if ( lCount >= 0 && CWinApp::OnIdle(lCount) )
		return TRUE;

	// PostMessageが連続しないようにここでポストする
	m_PostMsgLock.Lock();
	while ( !m_PostMsgQue.IsEmpty() ) {
		PostMsgQue *pQue = m_PostMsgQue.RemoveHead();
		::PostMessage(pQue->hWnd, pQue->Msg, pQue->wParam, pQue->lParam);
		delete pQue;
	}
	m_PostMsgLock.Unlock();

	for ( n = 0 ; !rt && n < m_IdleProcCount && (pProc = m_pIdleTop) != NULL ; n++ ) {
		m_pIdleTop = pProc->m_pNext;

		switch(pProc->m_Type) {
		case IDLEPROC_ENCRYPT:
			rt = mt_proc(pProc->m_pParam);
			break;
		case IDLEPROC_SCRIPT:
			rt = ((CScript *)(pProc->m_pParam))->OnIdle();
			break;
		case IDLEPROC_VIEW:
			rt = ((CRLoginView *)(pProc->m_pParam))->OnIdle();
			break;
		case IDLEPROC_FIFODOC:
			rt = ((CFifoDocument *)(pProc->m_pParam))->OnIdle();
			break;
		}

		// pProcの呼び出し後の利用不可(DelIdleProc後の可能性あり)
		// m_IdleProcCountも変化する場合有り
	}

	// メッセージ処理後に必ずOnIdleを呼ぶように・・・
	if ( rt )
		m_bUseIdle = TRUE;

	//TRACE("OnIdle(%d) %d\n", lCount, rt);

	// カラー絵文字メッセージ待ちをクリア
	if ( m_EmojiPostStat == EMOJIPOSTSTAT_WAIT )
		m_EmojiPostStat = EMOJIPOSTSTAT_DONE;

	return rt;
}
BOOL CRLoginApp::IsIdleMessage(MSG* pMsg)
{
	if ( m_bUseIdle ) {
		m_bUseIdle = FALSE;
		return TRUE;
	}

	if ( pMsg->message == WM_PAINT || pMsg->message == WM_DRAWEMOJI || pMsg->message == WM_FIFOMSG )
		return TRUE;

	if ( CWinApp::IsIdleMessage(pMsg) )
		return TRUE;

	return FALSE;
}
int CRLoginApp::DoMessageBox(LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt)
{
	if ( (nType & MB_ICONMASK) == 0 ) {
		switch (nType & MB_TYPEMASK) {
		case MB_OK:
		case MB_OKCANCEL:
			nType |= MB_ICONEXCLAMATION;
			break;

		case MB_YESNO:
		case MB_YESNOCANCEL:
			nType |= MB_ICONQUESTION;
			break;

		case MB_ABORTRETRYIGNORE:
		case MB_RETRYCANCEL:
			break;
		}
	}

	if ( theApp.m_pMainWnd == NULL )
		return CWinApp::DoMessageBox(lpszPrompt, nType, nIDPrompt);

	else if ( m_nThreadID == GetCurrentThreadId() )
		return ::DoitMessageBox(lpszPrompt, nType, ::AfxGetMainWnd());

	else {
		DocMsg docMsg;
		docMsg.doc = NULL;
		docMsg.pIn = (void *)lpszPrompt;
		docMsg.pOut = (void *)NULL;
		docMsg.type = nType;
		theApp.m_pMainWnd->SendMessage(WM_DOCUMENTMSG, DOCMSG_MESSAGE, (LPARAM)&docMsg);
		return docMsg.type;
	}
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
void CRLoginApp::SetSleepMode(int req)
{
	static int disCount = 0;
	static clock_t lastReset = 0;

	switch(req) {
	case SLEEPREQ_DISABLE:
		if ( disCount++ == 0 )
			SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
		break;
	case SLEEPREQ_ENABLE:
		if ( --disCount <= 0 ) {
			SetThreadExecutionState(ES_CONTINUOUS);
			lastReset = clock() + CLOCKS_PER_SEC * 20;
			disCount = 0;
		}
		break;
	case SLEEPREQ_RESET:
		if ( disCount == 0 ) {
			clock_t now = clock();
			if ( lastReset == 0 || lastReset <= now ) {
				SetThreadExecutionState(ES_CONTINUOUS);
				lastReset = now + CLOCKS_PER_SEC * 20;
			}
		}
		break;
	}
}
void CRLoginApp::IdlePostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PostMsgQue tmp = { hWnd, Msg, wParam, lParam };

	// 別スレッドからの場合あり
	m_PostMsgLock.Lock();
	POSITION pos = m_PostMsgQue.GetHeadPosition();

	while ( pos != NULL ) {
		PostMsgQue *pQue = m_PostMsgQue.GetNext(pos);
		if ( pQue->hWnd == hWnd && pQue->Msg == Msg && pQue->wParam == wParam && pQue->lParam == lParam ) {
			m_PostMsgLock.Unlock();
			return;
		}
	}

	m_PostMsgQue.AddTail(new PostMsgQue(tmp));
	m_PostMsgLock.Unlock();
}

//////////////////////////////////////////////////////////////////////
// CRLoginApp メッセージ ハンドラ

void CRLoginApp::OnFileNew()
{
	if ( ((CMainFrame *)::AfxGetMainWnd())->GetTabCount() >= DOCUMENT_MAX ) {
		::AfxMessageBox(_T("Too many new documents"));
		return;
	}

	try {
		CWinApp::OnFileNew();
	} catch(LPCTSTR pMsg) {
		CString msg;
		msg.Format(_T("FileNew Catch Error '%s'"), pMsg);
		::AfxMessageBox(msg);
	} catch(...) {
		::AfxMessageBox(_T("FileNew Catch Error"));
	}
}
void CRLoginApp::OnFileOpen()
{
	if ( ((CMainFrame *)::AfxGetMainWnd())->GetTabCount() >= DOCUMENT_MAX ) {
		::AfxMessageBox(_T("Too many open documents"));
		return;
	}

	try {
		CWinApp::OnFileOpen();
	} catch(LPCTSTR pMsg) {
		CString msg;
		msg.Format(_T("FileOpen Catch Error '%s'"), pMsg);
		::AfxMessageBox(msg);
	} catch(...) {
		::AfxMessageBox(_T("FileOpen Catch Error"));
	}
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
	CString FontName, tmp;
	int FontSize;
	LOGFONT LogFont;
	CDC dc;

	dc.CreateCompatibleDC(NULL);

	FontName = GetProfileString(_T("Dialog"), _T("FontName"), _T(""));
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

	if ( FontName.IsEmpty() && RegisterGetStr(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes"), _T("MS Shell Dlg"), tmp) )
	    _tcsncpy(LogFont.lfFaceName, tmp, LF_FACESIZE);
	else
	    _tcsncpy(LogFont.lfFaceName, FontName, LF_FACESIZE);

	CFontDialog font(&LogFont, CF_NOVERTFONTS | CF_SCREENFONTS | CF_INACTIVEFONTS /*| CF_SELECTSCRIPT */, NULL, ::AfxGetMainWnd());

	if ( DpiAwareDoModal(font) == IDOK ) {
		FontName = LogFont.lfFaceName;
		FontSize = 0 - MulDiv(LogFont.lfHeight, 72, dc.GetDeviceCaps(LOGPIXELSY));

		WriteProfileString(_T("Dialog"), _T("FontName"), FontName);
		WriteProfileInt(_T("Dialog"), _T("FontSize"), FontSize);

	} else if ( !FontName.IsEmpty() && ::AfxMessageBox(CStringLoad(IDS_DLGFONTDELMSG), MB_ICONQUESTION | MB_YESNO) == IDYES ) {
		DelProfileEntry(_T("Dialog"), _T("FontName"));
		DelProfileEntry(_T("Dialog"), _T("FontSize"));
	}

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
		if ( ::AfxMessageBox(CStringLoad(IDS_PASSLOCKDELMSG), MB_ICONQUESTION | MB_YESNO) != IDYES )
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
	pDlg->m_ResFileName.Format(_T("%s\\%s_rc.txt"), (LPCTSTR)m_BaseDir, m_pszAppName);

	pDlg->Create(IDD_RESTRANSDLG); //, CWnd::GetDesktopWindow());
	pDlg->ShowWindow(SW_SHOW);
}

void CRLoginApp::OnCreateprofile()
{
	if ( SavePrivateProfile() )
		::AfxMessageBox(CStringLoad(IDS_CREATEPROFILE), MB_ICONERROR);
}
void CRLoginApp::OnUpdateCreateprofile(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pszRegistryKey != NULL ? TRUE : FALSE);
}

void CRLoginApp::OnSaveregfile()
{
	switch(SaveRegistryFile()) {
	case 0:		// no save
		break;
	case 1:
		::AfxMessageBox(_T("Successfully saved all registries to a file"), MB_ICONINFORMATION);
		break;
	case 2:
		::AfxMessageBox(_T("Can't Save Registry File"), MB_ICONWARNING);
		break;
	case 3:
		::AfxMessageBox(_T("Cann't Create Registry file"), MB_ICONWARNING);
		break;
	}
}
void CRLoginApp::OnUpdateSaveregfile(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pszRegistryKey != NULL ? TRUE : FALSE);
}

void CRLoginApp::OnRegistapp()
{
	m_bRegistAppp = GetProfileInt(_T("RLoginApp"), _T("RegistAppp"),  FALSE);

	if ( m_bRegistAppp ) {
		if ( ::AfxMessageBox(CStringLoad(IDS_REGAPPDELMSG), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
		m_bRegistAppp = FALSE;
		RegisterShellRemoveAll();
	} else {
		if ( ::AfxMessageBox(CStringLoad(IDS_REGAPPSETMSG), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
		m_bRegistAppp = TRUE;
		RegisterShellFileEntry();
	}

	WriteProfileInt(_T("RLoginApp"), _T("RegistAppp"), m_bRegistAppp);
}
void CRLoginApp::OnUpdateRegistapp(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bRegistAppp);
}

void CRLoginApp::OnSecporicy()
{
	CSecPolicyDlg dlg;

	dlg.DoModal();
}

#ifdef	USE_DIRECTWRITE

HDC CRLoginApp::GetEmojiImage(class CEmojiImage *pEmoji)
{
	if ( m_EmojiImageDir.IsEmpty() )
		return NULL;

	ASSERT(pEmoji != NULL);

	int n;
	CString path, base, name, fmt;
	CStringD dstr = pEmoji->m_String;
	LPCDSTR p = dstr;

	base.Format(_T("%s\\%05X\\"), (LPCTSTR)m_EmojiImageDir, *p & 0xFFFF00);

	for ( ; *p != 0 ; p++ ) {
		fmt.Format(_T("%05X_"), *p);
		name += fmt;
	}
	name.Delete(name.GetLength() - 1, 1);

	for ( ; ; ) {
		path.Format(_T("%s%s.png"), (LPCTSTR)base, (LPCTSTR)name);

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

	path.Format(_T("%s\\emoji\\%05X"), (LPCTSTR)m_BaseDir, *p & 0xFFFF00);
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

	ASSERT(pEmoji != NULL);

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
			(FLOAT)fh * 96 / SYSTEM_DPI_Y, _T(""), &pTextFormat)) )
		goto ENDOF;

	if ( FAILED(m_pDWriteFactory->CreateTextLayout(str, len, pTextFormat, 1024, 256, &pTextLayout)) )
		goto ENDOF;
                        
	pTextLayout->GetMetrics(&tTextMetrics);

	frame.left   = frame.top = 0;
	frame.right  = (LONG)tTextMetrics.width;
	frame.bottom = (LONG)tTextMetrics.height;

	pEmoji->m_String = str;

	if ( !pEmoji->m_Image.CreateEx(frame.Width(), frame.Height(), 32, BI_RGB, NULL, CImage::createAlphaChannel) )
		goto ENDOF;

	hDC = pEmoji->m_Image.GetDC();

	if ( FAILED(m_pDCRT->BindDC(hDC, &frame)) )
		goto ENDOF;

	m_pDCRT->BeginDraw();
	m_pDCRT->SetTransform(D2D1::Matrix3x2F::Identity());
	m_pDCRT->Clear(oBKColor);

#define	D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT	((D2D1_DRAW_TEXT_OPTIONS)0x00000004)

	m_pDCRT->DrawText(str, len, pTextFormat,
		&D2D1::RectF((FLOAT)frame.left, (FLOAT)frame.top, (FLOAT)frame.right, (FLOAT)frame.bottom), 
		pBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);

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

void CRLoginApp::OnUpdateEmoji(WPARAM wParam, LPARAM lParam)
{
	CEmojiDocPos *pDocPos, *pTop = (CEmojiDocPos *)lParam;

	while ( (pDocPos = CEmojiDocPos::RemoveHead(&pTop)) != NULL ) {
		POSITION pos = GetFirstDocTemplatePosition();
		while ( pos != NULL ) {
			CDocTemplate *pDocTemp = GetNextDocTemplate(pos);

			POSITION dpos = pDocTemp->GetFirstDocPosition();
			while ( dpos != NULL ) {
				CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);

				if ( pDocPos->m_pDoc == pDoc && pDocPos->m_Seq == pDoc->m_DocSeqNumber ) {
					if ( pDoc->m_TextRam.m_HisAbs < pDocPos->m_Abs )
						pDocPos->m_Abs -= pDoc->m_TextRam.m_HisMax;

					POSITION vpos = pDoc->GetFirstViewPosition();
					while ( vpos != NULL ) {
						CRLoginView *pView = (CRLoginView *)pDoc->GetNextView(vpos);

						CRect rect(pDocPos->m_Pos);
						rect.top    = rect.top    + pDocPos->m_Abs - pDoc->m_TextRam.m_HisAbs;
						rect.bottom = rect.bottom + pDocPos->m_Abs - pDoc->m_TextRam.m_HisAbs;
						pView->InvalidateTextRect(rect);
					}
				}
			}
		}
		delete pDocPos;
	}

	// OnIdleが呼ばれるまで次のポストを抑制
	m_pEmojiPostDocPos = NULL;
	m_EmojiPostStat = EMOJIPOSTSTAT_WAIT;
}
static UINT EmojiThreadProc(LPVOID pParam)
{
	HDC hDC;
	CEmojiImage *pEmoji;
	CRLoginApp *pApp = (CRLoginApp *)pParam;
	CEmojiDocPos *pTop, *pDocPos;
	int count;
	clock_t limit;

	while ( pApp->m_EmojiThreadMode == 1 ) {
		count = 0;
		limit = clock() + (300 * CLOCKS_PER_SEC / 1000);
		pTop = NULL;

		pApp->m_EmojiThreadSemaphore.Lock();
		while ( pApp->m_EmojiThreadMode == 1 && (pEmoji = CEmojiImage::RemoveHead(&(pApp->m_pEmojiThreadQue))) != NULL ) {
			pEmoji->m_Status = EMOJIIMGSTAT_LOAD;
			pApp->m_EmojiThreadSemaphore.Unlock();

			if ( (hDC = pApp->GetEmojiImage(pEmoji)) == NULL )
				hDC = pApp->GetEmojiDrawText(pEmoji, RGB(255, 255, 255), MulDiv(48, SYSTEM_DPI_Y, DEFAULT_DPI_Y));

			if ( hDC != NULL )
				pEmoji->m_Image.ReleaseDC();

			pApp->m_EmojiThreadSemaphore.Lock();
			pEmoji->m_Status = EMOJIIMGSTAT_DONE;
			if ( (pDocPos = pEmoji->m_pDocPos) != NULL )
				pEmoji->m_pDocPos = NULL;
			pApp->m_EmojiThreadSemaphore.Unlock();

			if ( pDocPos != NULL )
				pTop = pDocPos->AddList(pTop);

			if ( pTop != NULL && (++count >= 8 || limit < clock()) && pApp->m_EmojiThreadMode == 1 && pApp->m_EmojiPostStat == EMOJIPOSTSTAT_DONE ) {
				ASSERT(pApp->m_pMainWnd != NULL && pApp->m_pMainWnd->GetSafeHwnd() != NULL);
				pApp->m_pEmojiPostDocPos = pTop;
				pApp->m_EmojiPostStat = EMOJIPOSTSTAT_POST;
				pApp->m_pMainWnd->PostMessage(WM_DRAWEMOJI, NULL, (LPARAM)pTop);
				count = 0;
				limit = clock() + (300 * CLOCKS_PER_SEC / 1000);
				pTop = NULL;
			}

			pApp->m_EmojiThreadSemaphore.Lock();
		}
		pApp->m_EmojiThreadSemaphore.Unlock();

		// すでにスレッド終了なら・・・
		if ( pApp->m_EmojiThreadMode != 1 ) {
			while ( (pDocPos = CEmojiDocPos::RemoveHead(&pTop)) != NULL )
				delete pDocPos;
		} else if ( pTop != NULL ) {
			ASSERT(pApp->m_pMainWnd != NULL && pApp->m_pMainWnd->GetSafeHwnd() != NULL);
			pApp->m_pEmojiPostDocPos = pTop;
			pApp->m_EmojiPostStat = EMOJIPOSTSTAT_POST;
			pApp->m_pMainWnd->PostMessage(WM_DRAWEMOJI, NULL, (LPARAM)pTop);
		}

		// 次のイベント待ち
		WaitForSingleObject(pApp->m_EmojiReqEvent, INFINITE);
	}

	pApp->m_EmojiThreadEvent.SetEvent();
	return 0;
}

BOOL CRLoginApp::DrawEmoji(CDC *pDC, CRect &rect, LPCTSTR str, COLORREF fc, COLORREF bc, BOOL bEraBack, void *pParam)
{
	HDC hDC = NULL;
	int hash = 0;
	int count = 0;
	CEmojiImage *pBack, *pEmoji;
	int width, height;
	CTextRam::DrawWork *pProp = (CTextRam::DrawWork *)pParam;
 	static BLENDFUNCTION bf = { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA };

	for ( int n = 0 ; str[n] != _T('\0') ; n++ )
		hash = hash + (int)str[n];
	hash &= (EMOJI_HASH -1);

	for ( pBack = pEmoji = m_pEmojiList[hash] ; pEmoji != NULL ; ) {
		if ( pEmoji->m_String.Compare(str) == 0 ) {
			if ( pEmoji == pBack )
				m_pEmojiList[hash] = pEmoji->m_pNext;
			else
				pBack->m_pNext = pEmoji->m_pNext;

			pEmoji->m_pNext = m_pEmojiList[hash];
			m_pEmojiList[hash] = pEmoji;

			m_EmojiThreadSemaphore.Lock();
			if ( pEmoji->m_Status != EMOJIIMGSTAT_DONE ) {
				pEmoji->Add(pProp->pDoc, pProp->pos);
				if ( pEmoji->m_Status == EMOJIIMGSTAT_WAIT ) {
					// 再表示なら優先順位を変更・・・ランダムな更新が微妙
					m_pEmojiThreadQue = pEmoji->RemoveAt(m_pEmojiThreadQue);
					m_pEmojiThreadQue = pEmoji->AddHead(m_pEmojiThreadQue);
				}
				m_EmojiThreadSemaphore.Unlock();

				if ( pProp->emode == EMOJIMODE_COLALT )
					return FALSE;
				if ( bEraBack )
					pDC->FillSolidRect(rect, bc);
				return TRUE;
			}
			m_EmojiThreadSemaphore.Unlock();

			if ( (HBITMAP)(pEmoji->m_Image) == NULL || (hDC = pEmoji->m_Image.GetDC()) == NULL )
				return FALSE;
			break;
		}

		if ( ++count > EMOJI_LISTMAX && pEmoji->m_pNext == NULL && pEmoji->m_Status == EMOJIIMGSTAT_DONE ) {
			pBack->m_pNext = NULL;
			pEmoji->m_Image.Destroy();
			break;
		}

		pBack = pEmoji;
		pEmoji = pEmoji->m_pNext;
	}

	if ( hDC == NULL ) {
		if ( pEmoji == NULL )
			pEmoji = new CEmojiImage;

		pEmoji->m_String = str;
		pEmoji->m_pNext = m_pEmojiList[hash];
		m_pEmojiList[hash] = pEmoji;

		switch(pProp->emode) {
		case EMOJIMODE_COLFILL:
		case EMOJIMODE_COLALT:
			pEmoji->m_Status = EMOJIIMGSTAT_WAIT;
			pEmoji->Add(pProp->pDoc, pProp->pos);

			if ( m_EmojiThreadMode == 0 ) {
				m_EmojiPostStat = 0;
				m_pEmojiPostDocPos = NULL;
				m_EmojiThreadMode = 1;
				AfxBeginThread(EmojiThreadProc, this, THREAD_PRIORITY_NORMAL);
			}

			m_EmojiThreadSemaphore.Lock();
			m_pEmojiThreadQue = pEmoji->AddHead(m_pEmojiThreadQue);
			m_EmojiReqEvent.SetEvent();
			m_EmojiThreadSemaphore.Unlock();

			if ( pProp->emode == EMOJIMODE_COLALT )
				return FALSE;
			if ( bEraBack )
				pDC->FillSolidRect(rect, bc);
			return TRUE;

		case EMOJIMODE_COLOLD:
			pEmoji->m_Status = EMOJIIMGSTAT_DONE;

			if ( (hDC = GetEmojiImage(pEmoji)) == NULL && (hDC = GetEmojiDrawText(pEmoji, fc, MulDiv(48, SYSTEM_DPI_Y, DEFAULT_DPI_Y))) == NULL )
				return FALSE;
			break;
		}
	}

	width  = pEmoji->m_Image.GetWidth();
	height = pEmoji->m_Image.GetHeight();

	if ( bEraBack )
		pDC->FillSolidRect(rect, bc);

	::AlphaBlend(pDC->GetSafeHdc(), rect.left, rect.top, rect.Width(), rect.Height(),
		hDC, 0, pProp->zoom == 3 ? (height / 2) : 0, width, pProp->zoom >= 2 ? (height / 2) : height, bf);

	// CImage.GetDC()してあるので注意
	pEmoji->m_Image.ReleaseDC();
	return TRUE;
}
void CRLoginApp::EmojiImageInit(LPCTSTR pFontName, LPCTSTR pImageDir)
{
	int n;
	CEmojiImage *pEmoji;
	CString UserImageDir;

	UserImageDir = GetProfileString(_T("RLoginApp"), _T("EmojiImageDir"), _T(""));

	if ( m_EmojiFontName.Compare(pFontName) == 0 && UserImageDir.Compare(pImageDir) == 0 )
		return;

	WriteProfileString(_T("RLoginApp"), _T("EmojiFontName"), pFontName);
	WriteProfileString(_T("RLoginApp"), _T("EmojiImageDir"), pImageDir);

	m_EmojiFontName = pFontName;
	m_EmojiImageDir = pImageDir;

	if ( m_EmojiImageDir.IsEmpty() || !IsDirectory(m_EmojiImageDir) ) {
		m_EmojiImageDir.Format(_T("%s\\emoji"), (LPCTSTR)m_BaseDir);
		if ( !IsDirectory(m_EmojiImageDir) ) {
			m_EmojiImageDir.Format(_T("%s\\emoji"), (LPCTSTR)m_ExecDir);
			if ( !IsDirectory(m_EmojiImageDir) )
				m_EmojiImageDir.Empty();
		}
	}

	EmojiImageFinish();

	for ( n = 0 ; n < EMOJI_HASH ; n++ ) {
		while ( (pEmoji = m_pEmojiList[n]) != NULL ) {
			m_pEmojiList[n] = pEmoji->m_pNext;
			delete pEmoji;
		}
	}

	::AfxGetMainWnd()->RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE);
}
void CRLoginApp::EmojiImageFinish()
{
	if ( m_EmojiThreadMode != 0 ) {
		m_EmojiThreadMode = 0;
		m_EmojiReqEvent.SetEvent();
		WaitForEvent(m_EmojiThreadEvent, _T("Emoji Thread"));
	}

	m_pEmojiThreadQue = NULL;
}

#endif

#define	SPCAT_DEF_VOICES	SPCAT_VOICES	// L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Voices"
#define	SPCAT_V10_VOICES	L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech Server\\v10.0\\Voices"
#define	SPCAT_V11_VOICES	L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech Server\\v11.0\\Voices"
#define	SPCAT_ONE_VOICES	L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices"
	
#define	REQ_ATTR_LANG_JP		 L"Language = 411"
#define	REQ_ATTR_LANG_EN		 L"Language = 409"

BOOL CRLoginApp::AddVoiceToken(const WCHAR * pszCategoryId, const WCHAR * pszReqAttribs, const WCHAR * pszOptAttribs)
{
	BOOL rt = FALSE;
	IEnumSpObjectTokens *pEnum = NULL;
	ULONG ulCount;
	ISpObjectToken *pToken;

	if ( FAILED(SpEnumTokens(pszCategoryId, pszReqAttribs, pszOptAttribs, &pEnum)) )
		goto ERRRET;

	if ( FAILED(pEnum->GetCount(&ulCount)) )
		goto ERRRET;

	while ( ulCount-- > 0 ) {
		if ( FAILED(pEnum->Next(1, &pToken, NULL)) )
			goto ERRRET;

		m_VoiceList.Add(pToken);
	}

	rt = TRUE;

ERRRET:
	if ( pEnum != NULL )
		pEnum->Release();

	return rt;
}
ISpVoice *CRLoginApp::VoiceInit()
{
	int n;
	ISpObjectToken *pToken;
	WCHAR *desc;
	CString VoiceDesc;
	long rate;
	static BOOL bInit = FALSE;

	if ( bInit )
		return m_pVoice;

	bInit = TRUE;

	m_VoiceList.RemoveAll();

	if ( FAILED(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&m_pVoice)) )
		return NULL;

	if ( SUCCEEDED(m_pVoice->GetVoice(&pToken)) && SUCCEEDED(SpGetDescription(pToken, &desc)) )
		VoiceDesc = desc;

	VoiceDesc = GetProfileString(_T("RLoginApp"), _T("VoiceDesc"), VoiceDesc);

	m_pVoice->GetRate(&rate);
	rate = GetProfileInt(_T("RLoginApp"), _T("VoiceRate"), rate);
	if ( rate < -10 ) rate = -10; else if ( rate > 10 ) rate = 10;

	AddVoiceToken(SPCAT_DEF_VOICES, NULL, NULL);
//	AddVoiceToken(SPCAT_V10_VOICES, NULL, NULL);
//	AddVoiceToken(SPCAT_V11_VOICES, NULL, NULL);
	AddVoiceToken(SPCAT_ONE_VOICES, NULL, NULL);

	for ( n = 0 ; n < m_VoiceList.GetSize() ; n++ ) {
		pToken = (ISpObjectToken *)m_VoiceList[n];

		if ( SUCCEEDED(SpGetDescription(pToken, &desc)) && VoiceDesc.Compare(UniToTstr(desc)) == 0 ) {
			m_pVoice->SetVoice(pToken);
			break;
		}
	}

	m_pVoice->SetRate(rate);	//  -10 to 10 

	return m_pVoice;
}
void CRLoginApp::VoiceFinis()
{
	int n;

	for ( n = 0 ; n < m_VoiceList.GetSize() ; n++ ) {
		ISpObjectToken *pToken = (ISpObjectToken *)m_VoiceList[n];
		pToken->Release();
	}

	m_VoiceList.RemoveAll();

	if ( m_pVoice != NULL )
		m_pVoice->Release();

	m_pVoice = NULL;
}
void CRLoginApp::SetVoiceListCombo(CComboBox *pCombo)
{
	int n;
	WCHAR *desc;
	ISpObjectToken *pToken;
	CString VoiceDesc;

	if ( VoiceInit() == NULL )
		return;

	if ( SUCCEEDED(m_pVoice->GetVoice(&pToken)) && SUCCEEDED(SpGetDescription(pToken, &desc)) )
		VoiceDesc = desc;

	for ( n = pCombo->GetCount() - 1 ; n >= 0 ; n-- )
		pCombo->DeleteString(n);

	for ( n = 0 ; n < m_VoiceList.GetSize() ; n++ ) {
		if ( SUCCEEDED(SpGetDescription((ISpObjectToken *)m_VoiceList[n], &desc)) ) {
			if ( pCombo->FindStringExact((-1), UniToTstr(desc)) == CB_ERR )
				pCombo->AddString(UniToTstr(desc));

			if ( VoiceDesc.Compare(desc) == 0 )
				pCombo->SetCurSel(n);
		}
	}
}
void CRLoginApp::SetVoice(LPCTSTR str, long rate)
{
	int n;
	WCHAR *desc;
	ISpObjectToken *pToken;
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( VoiceInit() == NULL )
		return;

	m_pVoice->Pause();

	for ( n = 0 ; n < m_VoiceList.GetSize() ; n++ ) {
		pToken = (ISpObjectToken *)m_VoiceList[n];

		if ( SUCCEEDED(SpGetDescription(pToken, &desc)) && _tcscmp(str, UniToTstr(desc)) == 0 ) {
			m_pVoice->SetVoice(pToken);
			break;
		}
	}

	if ( rate < -10 ) rate = -10; else if ( rate > 10 ) rate = 10;
	m_pVoice->SetRate(rate);	//  -10 to 10 
	
	WriteProfileString(_T("RLoginApp"), _T("VoiceDesc"), str);
	WriteProfileInt(_T("RLoginApp"), _T("VoiceRate"), rate);

	m_pVoice->Resume();
		
	pMain->SpeakUpdate(pMain->m_SpeakActive[2], pMain->m_SpeakActive[1] - pMain->m_SpeakActive[0]);
}