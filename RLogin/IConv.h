// IConv.h: CIConv クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ICONV_H__3FCDBA02_3FD2_4416_ACF0_F8682073E833__INCLUDED_)
#define AFX_ICONV_H__3FCDBA02_3FD2_4416_ACF0_F8682073E833__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <iconv.h>
#include "Data.h"

class CIConv : public CObject  
{
public:
	int m_Mode;
    iconv_t m_Cd;
	CString m_From;
	CString m_To;
	CBuffer m_SaveBuf;
	class CIConv *m_Left;
	class CIConv *m_Right;
	DWORD m_Table[256];
	int m_ErrCount;
	int m_FromCodePage;
	int m_ToCodePage;
	CDwordIndex m_CodeIndex;
	const WORD *m_DecTab;

	CBuffer *ExtFromConv(CBuffer *in);
	void ExtToConv(CBuffer *out);

	void IConvClose();
	class CIConv *GetIConv(LPCTSTR from, LPCTSTR to);
	void IConvSub(LPCTSTR from, LPCTSTR to, CBuffer *in, CBuffer *out);
	BOOL IConvBuf(LPCTSTR from, LPCTSTR to, CBuffer *in, CBuffer *out);
	BOOL StrToRemote(LPCTSTR to, CBuffer *in, CBuffer *out);
	BOOL StrToRemote(LPCTSTR to, LPCTSTR in, CStringA &out);
	void RemoteToStr(LPCTSTR from, CBuffer *in, CBuffer *out);
	void RemoteToStr(LPCTSTR from, LPCSTR in, CString &out);
	DWORD IConvChar(LPCTSTR from, LPCTSTR to, DWORD ch);

	static BOOL IHaveCodePage(int cp);
	static void SetListArray(CStringArray &stra);
	static BOOL IsIconvList(LPCSTR name);

	CIConv();
	virtual ~CIConv();
};

#endif // !defined(AFX_ICONV_H__3FCDBA02_3FD2_4416_ACF0_F8682073E833__INCLUDED_)
