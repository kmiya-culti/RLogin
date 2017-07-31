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
	int m_Table[256];
	int m_ErrCount;

	int JisToSJis(int cd);
	int SJisToJis(int cd);

	void IConvClose();
	class CIConv *GetIConv(LPCSTR from, LPCSTR to);
	void IConvSub(LPCSTR from, LPCSTR to, CBuffer *in, CBuffer *out);
	int IConvBuf(LPCSTR from, LPCSTR to, CBuffer *in, CBuffer *out);
	int IConvStr(LPCSTR from, LPCSTR to, LPCSTR in, CString &out);
	int IConvChar(LPCSTR from, LPCSTR to, int ch);

	static void SetListArray(CStringArray &array);

	CIConv();
	virtual ~CIConv();
};

#endif // !defined(AFX_ICONV_H__3FCDBA02_3FD2_4416_ACF0_F8682073E833__INCLUDED_)
