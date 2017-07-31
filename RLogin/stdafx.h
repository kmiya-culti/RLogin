// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

#define	USE_DWMAPI
#define	USE_GOZI	3	// GOZI Version
#define	USE_NETTLE
#define	USE_SAPI
#define	USE_JUMPLIST
#define	USE_TEXTFRAME
//#define	USE_CLEFIA
//#define	USE_DIRECTWRITE
//#define	USE_DEBUGLOG
#define	USE_OLE
//#define	USE_RCDLL
#define	USE_KEYMACGLOBAL
//#define	USE_CLIENTKEY

// Windows バージョン

#define	_WIN32_WINNT_NT4		0x0400	// Windows NT 4.0
#define _WIN32_WINNT_WIN2K		0x0500	// Windows 2000
#define _WIN32_WINNT_WINXP		0x0501	// Windows XP
#define _WIN32_WINNT_WS03		0x0502	// Windows Server 2003/2008
#define _WIN32_WINNT_VISTA		0x0600	// Windows Vista
#define _WIN32_WINNT_WIN7		0x0601	// Windows 7
#define _WIN32_WINNT_WIN8		0x0602	// Windows 8
#define _WIN32_WINNT_WINBLUE	0x0603	// Windows 8.1
#define _WIN32_WINNT_WIN10		0x0A00	// Windows 10

#define	_WIN32_WINDOWS_W95		0x0400	// Windows 95
#define	_WIN32_WINDOWS_W98		0x0410	// Windows 98
#define	_WIN32_WINDOWS_WME		0x0500	// Windows Me

// VC/VS バージョン

#define	_MSC_VER_VC10			800		// VC++ 1.0
#define	_MSC_VER_VC20			900		// VC++ 2.0
#define	_MSC_VER_VC41			1000	// VC++ 4.1
#define	_MSC_VER_VC42			1010	// VC++ 4.2
#define	_MSC_VER_VC50			1100	// VC++ 5.0
#define	_MSC_VER_VC60			1200	// VC++ 6.0
#define	_MSC_VER_VS02			1300	// Visual Studio 2002
#define	_MSC_VER_VS03			1310	// Visual Studio 2003
#define	_MSC_VER_VS05			1400	// Visual Studio 2005
#define	_MSC_VER_VS08			1500	// Visual Studio 2008
#define	_MSC_VER_VS10			1600	// Visual Studio 2010
#define	_MSC_VER_VS12			1700	// Visual Studio 2012
#define	_MSC_VER_VS13			1800	// Visual Studio 2013
#define	_MSC_VER_VS15			1900	// Visual Studio 2015
#define	_MSC_VER_VS17			1910	// Visual Studio 2017

#if defined(USE_JUMPLIST) || defined(USE_SAPI)
  #define USE_COMINIT
#endif

#ifndef _SECURE_ATL
  #define _SECURE_ATL
#endif

#ifndef VC_EXTRALEAN
  #define VC_EXTRALEAN		// Windows ヘッダーから使用されていない部分を除外します。
#endif

// 下で指定された定義の前に対象プラットフォームを指定しなければならない場合、以下の定義を変更してください。
// 異なるプラットフォームに対応する値に関する最新情報については、MSDN を参照してください。

#ifdef	USE_JUMPLIST
  #define WINVER			_WIN32_WINNT_WIN7
  #define _WIN32_WINNT		_WIN32_WINNT_WIN7
#endif

#ifndef WINVER							// Windows XP 以降のバージョンに固有の機能の使用を許可します。
  #define WINVER			0x0502		// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_WINNT					// Windows XP 以降のバージョンに固有の機能の使用を許可します。                   
  #define _WIN32_WINNT		0x0502		// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif						

#ifndef _WIN32_WINDOWS					// Windows 98 以降のバージョンに固有の機能の使用を許可します。
  #define _WIN32_WINDOWS	0x0410		// これを Windows Me またはそれ以降のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_IE						// IE 6.0 以降のバージョンに固有の機能の使用を許可します。
  #define _WIN32_IE			0x0600		// これを IE の他のバージョン向けに適切な値に変更してください。
#endif

// 一般的で無視しても安全な MFC の警告メッセージの一部の非表示を解除します。
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 一部の CString コンストラクタは明示的です。
#define _AFX_ALL_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <afxwin.h>         // MFC のコアおよび標準コンポーネント
#include <afxext.h>         // MFC の拡張部分

#ifndef _AFX_NO_OLE_SUPPORT
  #include <afxdtctl.h>		// MFC の Internet Explorer 4 コモン コントロール サポート
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
  #include <afxcmn.h>			// MFC の Windows コモン コントロール サポート
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "winsock2.h"
#include "ws2tcpip.h"
#include <afx.h>
#include <afxdlgs.h>

#define	SECURITY_WIN32
#include <Security.h>

#pragma comment(lib,"winmm")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

#ifdef	USE_DIRECTWRITE
  #include <d2d1.h>
  #include <d2d1helper.h>
  #include <dwrite.h>
  #include <wincodec.h>

  #pragma comment(lib, "d2d1.lib")
  #pragma comment(lib, "dwrite.lib")
#endif

#ifdef	USE_SAPI
  #include <sapi.h>
#endif

#include <atlimage.h>
#include <atlbase.h>
#include <afxpriv.h>
#include <math.h>

#if	_MSC_VER >= _MSC_VER_VS10
  #include <afxcontrolbars.h>
#endif

#define	UniToMbs(s)				((LPCSTR)CStringA(s))
#define	MbsToUni(s)				((LPCWSTR)CStringW(s))

#ifdef	_UNICODE
  #define	TstrToMbs(s)		((LPCSTR)CStringA(s))
  #define	TstrToUni(s)		(s)
  #define	MbsToTstr(s)		((LPCTSTR)CStringW(s))
  #define	UniToTstr(s)		(s)
#else
  #define	TstrToMbs(s)		(s)
  #define	TstrToUni(s)		((LPCWSTR)CStringW(s))
  #define	MbsToTstr(s)		(s)
  #define	UniToTstr(s)		((LPCSTR)CStringA(s))
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
