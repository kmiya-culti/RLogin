// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���̃C���N���[�h �t�@�C���A�܂���
// �Q�Ɖ񐔂������A�����܂�ύX����Ȃ��A�v���W�F�N�g��p�̃C���N���[�h �t�@�C��
// ���L�q���܂��B

#pragma once

#define	USE_GOZI			3	// GOZI Version
#define	USE_DWMAPI				// �K���X���ʂ�L���ɂ���
#define	USE_JUMPLIST			// Windows7�̃W�����v���X�g��L��
#define	USE_TEXTFRAME			// �͂ݕ�����L���ɂ���
#define	USE_DIRECTWRITE			// DirectWrite���g�p���ĊG�����\��
//#define	USE_DEBUGLOG		// �v���O�������x���Ńf�o�b�O���O���쐬
#define	USE_OLE					// OLE���g�p�����t�@�C�������L���ɂ���
//#define	USE_RCDLL			// rlogin_rc.dll�̓ǂݍ��݂�L��
#define	USE_KEYMACGLOBAL		// �L�[����̋L�^���O���[�o���ōs��
//#define	USE_CLIENTKEY		// �E�B���h�E�̋��ŃL�[�R�[�h�𔭐�����
//#define	USE_KEYSCRIPT		// �L�[�ݒ�ɃX�N���v�g�Ăяo���g�����s��
#define	USE_NETTLE				// nettle�̊e��Í���L���ɂ���
//#define	USE_CLEFIA			// CLEFIA�Í���L���ɂ���
//#define	USE_MACCTX			// EVP_MAC_CTX�̓��삪����(init(key=NULL�ɔ�Ή�)
//#define	USE_X509			// openssl��x509���J���ؖ���L��
//#define	USE_NOENDIAN		// �G���f�B�A���ɂƂ���Ȃ��R���p�C��
#define	USE_PARENTHESIS			// Unicode�̊��ʕ�����Ǝ��`��
//#define	USE_HOSTBOUND		// EXT_INFO��hostkeys-00@openssh.com��L��
#define	USE_EXTINFOINAUTH		// EXT_INFO��ext-info-in-auth@openssh.com��L��
//#define	USE_FIFOMONITER		// FifoBuffer�̃T�C�Y���f�o�b�O
#define	USE_CSTRINGLASTERR		// CStrinA/W���̕����ϊ���LastError���Z�b�g����̂ŏ��׍H

// Windows �o�[�W����

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

// VC/VS �o�[�W����

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
#define	_MSC_VER_VS19			1920	// Visual Studio 2019
#define	_MSC_VER_VS22			1930	// Visual Studio 2022

#ifndef _SECURE_ATL
  #define _SECURE_ATL
#endif

#ifndef VC_EXTRALEAN
  #define VC_EXTRALEAN		// Windows �w�b�_�[����g�p����Ă��Ȃ����������O���܂��B
#endif

// ���Ŏw�肳�ꂽ��`�̑O�ɑΏۃv���b�g�t�H�[�����w�肵�Ȃ���΂Ȃ�Ȃ��ꍇ�A�ȉ��̒�`��ύX���Ă��������B
// �قȂ�v���b�g�t�H�[���ɑΉ�����l�Ɋւ���ŐV���ɂ��ẮAMSDN ���Q�Ƃ��Ă��������B

#ifdef	USE_JUMPLIST
  #define WINVER			_WIN32_WINNT_WIN7
  #define _WIN32_WINNT		_WIN32_WINNT_WIN7
#endif

#ifndef WINVER							// Windows XP �ȍ~�̃o�[�W�����ɌŗL�̋@�\�̎g�p�������܂��B
  #define WINVER			0x0502		// ����� Windows �̑��̃o�[�W���������ɓK�؂Ȓl�ɕύX���Ă��������B
#endif

#ifndef _WIN32_WINNT					// Windows XP �ȍ~�̃o�[�W�����ɌŗL�̋@�\�̎g�p�������܂��B                   
  #define _WIN32_WINNT		0x0502		// ����� Windows �̑��̃o�[�W���������ɓK�؂Ȓl�ɕύX���Ă��������B
#endif						

#ifndef _WIN32_WINDOWS					// Windows 98 �ȍ~�̃o�[�W�����ɌŗL�̋@�\�̎g�p�������܂��B
  #define _WIN32_WINDOWS	0x0410		// ����� Windows Me �܂��͂���ȍ~�̃o�[�W���������ɓK�؂Ȓl�ɕύX���Ă��������B
#endif

#ifndef _WIN32_IE						// IE 6.0 �ȍ~�̃o�[�W�����ɌŗL�̋@�\�̎g�p�������܂��B
  #define _WIN32_IE			0x0600		// ����� IE �̑��̃o�[�W���������ɓK�؂Ȓl�ɕύX���Ă��������B
#endif

// ��ʓI�Ŗ������Ă����S�� MFC �̌x�����b�Z�[�W�̈ꕔ�̔�\�����������܂��B
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// �ꕔ�� CString �R���X�g���N�^�͖����I�ł��B
#define _AFX_ALL_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <afxwin.h>         // MFC �̃R�A����ѕW���R���|�[�l���g
#include <afxext.h>         // MFC �̊g������

#ifndef _AFX_NO_OLE_SUPPORT
  #include <afxdtctl.h>		// MFC �� Internet Explorer 4 �R���� �R���g���[�� �T�|�[�g
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
  #include <afxcmn.h>			// MFC �� Windows �R���� �R���g���[�� �T�|�[�g
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "winsock2.h"
#include "ws2tcpip.h"
#include <afx.h>
#include <afxdlgs.h>

#define	SECURITY_WIN32
#include <Security.h>
#include <wincrypt.h>
#include <bcrypt.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "windowscodecs.lib")

#pragma warning(disable : 4996)		// openssl legacy warning������

#ifdef	USE_DIRECTWRITE
  #include <d2d1.h>
  #include <d2d1helper.h>
  #include <dwrite.h>
  #include <wincodec.h>
#endif

#include <sapi.h>
#include <atlimage.h>
#include <atlbase.h>
#include <afxpriv.h>
#include <math.h>

#if	_MSC_VER >= _MSC_VER_VS10
  #include <afxcontrolbars.h>
#endif

#ifdef	USE_CSTRINGLASTERR
	extern BOOL bBreak;
	class CStringMbs : public CStringA
	{
	public:
		CStringMbs(LPCWSTR wstr) {
			DWORD err = GetLastError();
	#ifdef	_DEBUG
			bBreak = FALSE;
			*(CStringA *)this = wstr;
			bBreak = TRUE;
	#else
			*(CStringA *)this = wstr;
	#endif
			SetLastError(err);
		}
	};
	class CStringUni : public CStringW
	{
	public:
		CStringUni(LPCSTR str) {
			DWORD err = GetLastError();
	#ifdef	_DEBUG
			bBreak = FALSE;
			*(CStringW *)this = str;
			bBreak = TRUE;
	#else
			*(CStringW *)this = str;
	#endif
			SetLastError(err);
		}
	};

	#define	UniToMbs(s)				((LPCSTR)CStringMbs(s))
	#define	MbsToUni(s)				((LPCWSTR)CStringUni(s))

	#ifdef	_UNICODE
	  #define	TstrToMbs(s)		((LPCSTR)CStringMbs(s))
	  #define	TstrToUni(s)		((LPCWSTR)(s))
	  #define	MbsToTstr(s)		((LPCTSTR)CStringUni(s))
	  #define	UniToTstr(s)		((LPCTSTR)(s))
	#else
	  #define	TstrToMbs(s)		((LPCSTR)(s))
	  #define	TstrToUni(s)		((LPCWSTR)CStringUni(s))
	  #define	MbsToTstr(s)		((LPCTSTR)(s))
	  #define	UniToTstr(s)		((LPCTSTR)CStringMbs(s))
	#endif
#else
	#define	UniToMbs(s)				((LPCSTR)CStringA(s))
	#define	MbsToUni(s)				((LPCWSTR)CStringW(s))

	#ifdef	_UNICODE
	  #define	TstrToMbs(s)		((LPCSTR)CStringA(s))
	  #define	TstrToUni(s)		((LPCWSTR)(s))
	  #define	MbsToTstr(s)		((LPCTSTR)CStringW(s))
	  #define	UniToTstr(s)		((LPCTSTR)(s))
	#else
	  #define	TstrToMbs(s)		((LPCSTR)(s))
	  #define	TstrToUni(s)		((LPCWSTR)CStringW(s))
	  #define	MbsToTstr(s)		((LPCTSTR)(s))
	  #define	UniToTstr(s)		((LPCTSTR)CStringA(s))
	#endif
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
