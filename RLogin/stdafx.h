// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

#define	USE_DWMAPI	1
#define	USE_GOZI	3
#define	USE_NETTLE	1
//#define	USE_CLEFIA	1
//#define	USE_JUMPLIST	1
//#define	USE_DIRECTWRITE	1
#define	USE_SAPI	1

//#define	WINSOCK11	1
//#define	NOIPV6		1
//#define	NOGDIPLUS	1

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Windows ヘッダーから使用されていない部分を除外します。
#endif

// 下で指定された定義の前に対象プラットフォームを指定しなければならない場合、以下の定義を変更してください。
// 異なるプラットフォームに対応する値に関する最新情報については、MSDN を参照してください。

#ifdef	USE_JUMPLIST
#define WINVER			0x0601		// Windows 7
#define _WIN32_WINNT	0x0601		// Windows 7
#define	_WIN32_WINDOWS	0x0601
#endif

#ifndef WINVER				// Windows XP 以降のバージョンに固有の機能の使用を許可します。
#define WINVER 0x0502		// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_WINNT		// Windows XP 以降のバージョンに固有の機能の使用を許可します。                   
#define _WIN32_WINNT 0x0502	// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif						

#ifndef _WIN32_WINDOWS		// Windows 98 以降のバージョンに固有の機能の使用を許可します。
#define _WIN32_WINDOWS 0x0410 // これを Windows Me またはそれ以降のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_IE			// IE 6.0 以降のバージョンに固有の機能の使用を許可します。
#define _WIN32_IE 0x0600	// これを IE の他のバージョン向けに適切な値に変更してください。
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 一部の CString コンストラクタは明示的です。

#define	_CRT_SECURE_NO_WARNINGS

// 一般的で無視しても安全な MFC の警告メッセージの一部の非表示を解除します。
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC のコアおよび標準コンポーネント
#include <afxext.h>         // MFC の拡張部分

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC の Internet Explorer 4 コモン コントロール サポート
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC の Windows コモン コントロール サポート
#endif // _AFX_NO_AFXCMN_SUPPORT

#ifdef	WINSOCK11
#include <afxsock.h>		// MFC のソケット拡張機能
#else
#include "winsock2.h"
#include "ws2tcpip.h"
#include <afx.h>
#include <afxdlgs.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Dnsapi.lib")
#endif

#ifdef	USE_DIRECTWRITE
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#endif

#ifndef	NOGDIPLUS
#include <atlimage.h>
#endif

#ifdef	USE_SAPI
#include <sapi.h>
#endif

#include <atlbase.h>
#include <afxpriv.h>
#include <math.h>
//#include <afxcontrolbars.h>

#define	UniToMbs(s)			(CStringA(s))
#define	MbsToUni(s)			(CStringW(s))

#ifdef	_UNICODE
#define	TstrToMbs(s)		(CStringA(s))
#define	TstrToUni(s)		(s)
#define	MbsToTstr(s)		(CStringW(s))
#define	UniToTstr(s)		(s)
#else
#define	TstrToMbs(s)		(s)
#define	TstrToUni(s)		(CStringW(s))
#define	MbsToTstr(s)		(s)
#define	UniToTstr(s)		(CStringA(s))
#endif

#if defined _UNICODE || defined _WIN64
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


