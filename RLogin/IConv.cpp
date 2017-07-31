// IConv.cpp: CIConv クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

//#include <errno.h>
#include "stdafx.h"
#include "RLogin.h"
#include "IConv.h"
#include "TextRam.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static const WORD DecTCS[] = {
	0x25A1,	0x23B7,	0x250C,	0x2500,	0x2320,	0x2321,	0x2502,	0x23A1,	0x23A3,	0x23A4,	0x23A6,	0x239B,	0x239D,	0x239E,	0x23A0,	0x23A8,	// 02/00-02/15
	0x23AC,	0x25A1,	0x25A1,	0x2572,	0x2571,	0x2510,	0x2518,	0x25A1,	0x25A1,	0x25A1,	0x25A1,	0x25A1,	0x2264,	0x2260,	0x2265,	0x222B,	// 03/00-03/15
	0x2234,	0x221D,	0x221E,	0x00F7,	0x0394,	0x2207,	0x03A6,	0x0393,	0x223C,	0x2243,	0x0398,	0x00D7,	0x039B,	0x21D4,	0x21D2,	0x2261,	// 04/00-04/15
	0x03A0,	0x03A8,	0x25A1,	0x03A3,	0x25A1,	0x25A1,	0x221A,	0x03A9,	0x039E,	0x03A5,	0x2282,	0x2283,	0x2229,	0x222A,	0x2227,	0x2228,	// 05/00-05/15
	0x00AC,	0x03B1,	0x03B2,	0x03C7,	0x03B4,	0x03B5,	0x03C6,	0x03B3,	0x03B7,	0x03B9,	0x03B8,	0x03BA,	0x03BB,	0x25A1,	0x03BD,	0x2202,	// 06/00-06/15
	0x03C0,	0x03C8,	0x03C1,	0x03C3,	0x03C4,	0x25A1,	0x0192,	0x03C9,	0x03BE,	0x03C5,	0x03B6,	0x2190,	0x2191,	0x2192,	0x2193,	0x25A1,	// 07/00-07/15
};
static const WORD DecSGCS[] = {
	0x25C6,	0x2592,	0x2409,	0x240C,	0x240D,	0x240A,	0x00B0,	0x00B1,	0x2424,	0x240B,	0x2518,	0x2510,	0x250C,	0x2514,	0x253C,	0x23BA,	// 06/00-06/15
	0x23BB,	0x2500,	0x23BC,	0x23BD,	0x251C,	0x2524,	0x2534,	0x252C,	0x2502,	0x2264,	0x2265,	0x03C0,	0x2260,	0x00A3,	0x00B7,	0x007F,	// 07/00-07/15
};

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CIConv::CIConv()
{
	m_Mode = 0;
	m_Cd = NULL;
	m_Left = NULL;
	m_Right = NULL;
	ZeroMemory(m_Table, sizeof(m_Table));
	m_ErrCount = 0;
}

CIConv::~CIConv()
{
	if ( m_Left != NULL )
		delete m_Left;

	if ( m_Right != NULL )
		delete m_Right;

	if ( m_Cd != NULL && m_Cd != (iconv_t)(-1) )
	    iconv_close(m_Cd);
}
void CIConv::IConvClose()
{
	if ( m_Left != NULL )
		delete m_Left;
	m_Left = NULL;

	if ( m_Right != NULL )
		delete m_Right;
	m_Right = NULL;

	if ( m_Cd != NULL && m_Cd != (iconv_t)(-1) )
	    iconv_close(m_Cd);

	m_Cd = NULL;
	m_ErrCount = 0;
}

/******************
SJIS	JISX213-2
0xF040	0x2121		0xF09F	0x2821
0xF140	0x2321		0xF19F	0x2421
0xF240	0x2521		0xF29F	0x2C21
0xF340	0x2D21		0xF39F	0x2E21
0xF440	0x2F21		0xF49F	0x6E21
0xF540	0x6F21		0xF59F	0x7021
0xF640	0x7121		0xF69F	0x7221
0xF740	0x7321		0xF79F	0x7421
0xF840	0x7521		0xF89F	0x7621
0xF940	0x7721		0xF99F	0x7821
0xFA40	0x7921		0xFA9F	0x7A21
0xFB40	0x7B21		0xFB9F	0x7C21
0xFC40	0x7D21		0xFC9F	0x7E21
*******************/

DWORD CIConv::JisToSJis(DWORD cd)
{
	DWORD hi,lo;

	hi = (cd >> 8) & 0xFF;
	lo = cd & 0xFF;

	if ( (hi & 1) != 0 )
		lo += (lo <= 0x5F ? 0x1F : 0x20);
	else
		lo += 0x7E;

	if ( (cd & 0xFF0000) == 0 ) {
		if ( hi <= 0x5E )
			hi = (hi + 0xE1) / 2;
		else
			hi = (hi + 0x161) / 2;
	} else {
		if ( hi >= 0x6E )
			hi = (hi + 0x17B) / 2;
		else if ( hi == 0x21 || (hi >= 0x23 && hi <= 0x25) )
			hi = (hi + 0x1BF) / 2;
		else if ( hi == 0x28 || (hi >= 0x2C && hi <= 0x2F) )
			hi = (hi + 0x1B9) / 2;
		else
			return 0x8140;
	}

	return (hi << 8 | lo);
}

DWORD CIConv::SJisToJis(DWORD cd)
{
	DWORD hi,lo;

	hi = (cd >> 8) & 0xFF;
	lo = cd & 0xFF;

	if ( hi <= 0x9F )
		hi = (hi - 0x71) * 2 + 1;
	else if ( hi <= 0xEF )
		hi = (hi - 0xB1) * 2 + 1;
	else
		hi = (hi - 0x60) * 2 + 1;

	if ( lo >= 0x9F ) {
		lo -= 0x7E;
		hi++;
	} else
		lo -= (lo >= 0x80 ? 0x20 : 0x1F);

	if ( hi >= 0x12A )
		hi += 0x44;
	else if ( hi >= 0x126 || hi == 0x122 )
		hi += 0x06;

	return (hi << 8 | lo);
}

class CIConv *CIConv::GetIConv(LPCTSTR from, LPCTSTR to)
{
	int n;

	if ( m_Cd == NULL && m_Mode == 0 ) {
		m_From = from;
		m_To   = to;
		m_Mode = 0;

		if ( _tcscmp(from, _T("EUC-JISX0213")) == 0 )
			m_Mode |= 001;
		else if ( _tcscmp(from, _T("SHIFT_JISX0213")) == 0 )
			m_Mode |= 002;
		else if ( _tcscmp(from, _T("JIS_X0213-2000.1")) == 0 )
			m_Mode |= 003;
		else if ( _tcscmp(from, _T("JIS_X0213-2000.2")) == 0 )
			m_Mode |= 004;
		else if ( _tcscmp(from, _T("DEC_TCS-GR")) == 0 )
			m_Mode |= 005;
		else if ( _tcscmp(from, _T("DEC_SGCS-GR")) == 0 )
			m_Mode |= 006;

		if ( _tcscmp(to, _T("EUC-JISX0213")) == 0 )
			m_Mode |= 010;
		else if ( _tcscmp(to, _T("SHIFT_JISX0213")) == 0 )
			m_Mode |= 020;
		else if ( _tcscmp(to, _T("JIS_X0213-2000.1")) == 0 )
			m_Mode |= 030;
		else if ( _tcscmp(to, _T("JIS_X0213-2000.2")) == 0 )
			m_Mode |= 040;

		if ( (m_Mode & 007) >= 005 )
			from = _T("UCS-4BE");
		else if ( (m_Mode & 007) >= 003 )
			from = _T("EUC-JISX0213");

		if ( (m_Mode & 070) >= 030 )
			to   = _T("EUC-JISX0213");

	    m_Cd = iconv_open(TstrToMbs(to), TstrToMbs(from));
		return this;
	}

	if ( (n = m_From.Compare(from)) == 0 && (n = m_To.Compare(to)) == 0 )
		return this;
	else if ( n < 0 ) {
		if ( m_Left == NULL )
			m_Left = new CIConv;
		return m_Left->GetIConv(from, to);
	} else {
		if ( m_Right == NULL )
			m_Right = new CIConv;
		return m_Right->GetIConv(from, to);
	}
}

void CIConv::IConvSub(LPCTSTR from, LPCTSTR to, CBuffer *in, CBuffer *out)
{
	int n = 0;
	CIConv *cp;
    char *inp, *otp;
    size_t ins, tns, xns, ots, len;
	char tmp[4096];

	cp = GetIConv(from, to);
	if ( cp->m_Cd == (iconv_t)(-1) )
		return;

RETRY:
	len = in->GetSize();

    while ( in->GetSize() > 0 && n != (-1) ) {
		inp = (char *)(in->GetPtr());
		ins = in->GetSize();
		otp = tmp;
		ots = 4096;
		xns = tns = (ins < 2048 ? ins : 2048);
        n = (int)iconv(cp->m_Cd, (char **)&inp, &tns, &otp, &ots);
		if ( ots < 4096 ) {
			out->Apend((LPBYTE)tmp, (int)(4096 - ots));
			ZeroMemory(tmp, (4096 - ots));
		}
		in->Consume((int)(xns - tns));
	}

	if ( len > 4 && len == in->GetSize() && n == (-1) ) {
		in->Consume(1);
		m_ErrCount++;
		goto RETRY;
	}
}
int CIConv::IConvBuf(LPCTSTR from, LPCTSTR to, CBuffer *in, CBuffer *out)
{
	int n = 0;
	CIConv *cp;
    char *inp, *otp;
    size_t ins, tns, xns, ots;
	char tmp[4096];

	cp = GetIConv(from, to);
	if ( cp->m_Cd == (iconv_t)(-1) )
		return 0;

	inp = (char *)(in->GetPtr());
	ins = in->GetSize();

    while ( ins > 0 && n != (-1) ) {
		otp = tmp;
		ots = 4096;
		xns = tns = (ins < 2048 ? ins : 2048);
        n = (int)iconv(cp->m_Cd, (char **)&inp, &tns, &otp, &ots);
		if ( ots < 4096 )
			out->Apend((LPBYTE)tmp, (int)(4096 - ots));
		ins -= (xns - tns);
	}

	otp = tmp;
	ots = 4096;
    iconv(cp->m_Cd, NULL, NULL, &otp, &ots);
	if ( ots < 4096 )
		out->Apend((LPBYTE)tmp, (int)(4096 - ots));

	return out->GetSize();
}
void CIConv::StrToRemote(LPCTSTR to, CBuffer *in, CBuffer *out)
{
	CTextRam::MsToIconvUniStr(to, (LPWSTR)(LPCWSTR)(*in), in->GetSize() / sizeof(WCHAR));
	IConvBuf(_T("UTF-16LE"), to, in, out);
}
void CIConv::StrToRemote(LPCTSTR to, LPCTSTR in, CStringA &out)
{
	CBuffer bIn, bOut;
#ifdef	_UNICODE
	bIn.Apend((LPBYTE)in, (int)_tcslen(in) * sizeof(WCHAR));
	CTextRam::MsToIconvUniStr(to, (LPWSTR)(LPCWSTR)bIn, bIn.GetSize() / sizeof(WCHAR));
	IConvBuf(_T("UTF-16LE"), to, &bIn, &bOut);
#else
	bIn.Apend((LPBYTE)in, (int)strlen(in));
	IConvBuf(_T("CP932"), to, &bIn, &bOut);
#endif
	out = (LPCSTR)bOut;
}
void CIConv::RemoteToStr(LPCTSTR from, CBuffer *in, CBuffer *out)
{
	CBuffer mid;
	IConvBuf(from, _T("UTF-16LE"), in, &mid);
	CTextRam::IconvToMsUniStr(from, (LPCWSTR)mid, mid.GetSize() / sizeof(WCHAR), *out);
}
void CIConv::RemoteToStr(LPCTSTR from, LPCSTR in, CString &out)
{
	CBuffer bIn, bMid, bOut;
	bIn.Apend((LPBYTE)in, (int)strlen(in));
	IConvBuf(from, _T("UTF-16LE"), &bIn, &bMid);
	CTextRam::IconvToMsUniStr(from, (LPCWSTR)bMid, bMid.GetSize() / sizeof(WCHAR), bOut);
	out = (LPCWSTR)bOut;
}

DWORD CIConv::IConvChar(LPCTSTR from, LPCTSTR to, DWORD ch)
{
	int n = 0;
	DWORD od = ch;
	CIConv *cp;
    char *inp, *otp;
    size_t ins, ots;
    unsigned char itmp[32], otmp[32];

	cp = GetIConv(from, to);
	if ( cp->m_Cd == (iconv_t)(-1) )
		return 0x0000;

	if ( (ch & 0xFFFFFF00) == 0 && cp->m_Table[ch] != 0 )
		return cp->m_Table[ch];

	switch(cp->m_Mode) {
	case 011:		// EUC-JISX0213 > EUC-JISX0213
		goto ENDOF;
	case 021:		// EUC-JISX0213 > SHIFT_JISX0213
		ch = JisToSJis(ch & 0x17F7F);
		goto ENDOF;
	case 031:		// EUC-JISX0213 > JISX0213.1
		if ( ch >= 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;
	case 041:		// EUC-JISX0213 > JISX0213.2
		if ( ch < 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;

	case 012:		// SHIFT_JISX0213 > EUC-JISX0213
		ch = SJisToJis(ch);
		if ( ch >= 0x10000 )
			ch |= 0x8F0000;
		ch |= 0x8080;
		goto ENDOF;
	case 022:		// SHIFT_JISX0213 > SHIFT_JISX0213
		goto ENDOF;
	case 032:		// SHIFT_JISX0213 > JISX0213.1
		ch = SJisToJis(ch);
		if ( ch >= 0x10000 )
			ch = 0x0000;
		goto ENDOF;
	case 042:		// SHIFT_JISX0213 > JISX0213.2
		ch = SJisToJis(ch);
		if ( ch < 0x10000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;

	case 003:		// JISX0213.1 > ???
		ch |= 0x8080;
		break;
	case 013:		// JISX0213.1 > EUC-JISX0213
		ch |= 0x8080;
		goto ENDOF;
	case 023:		// JISX0213.1 > SHIFT_JISX0213
		ch = JisToSJis(ch);
		goto ENDOF;
	case 033:		// JISX0213.1 > JISX0213.1
		goto ENDOF;
	case 043:		// JISX0213.1 > JISX0213.2
		ch = 0x0000;
		goto ENDOF;

	case 004:		// JISX0213.2 > ???
		ch |= 0x8F8080;
		break;
	case 014:		// JISX0213.2 > EUC-JISX0213
		ch |= 0x8F8080;
		goto ENDOF;
	case 024:		// JISX0213.2 > SHIFT_JISX0213
		ch = JisToSJis(ch | 0x10000);
		goto ENDOF;
	case 034:		// JISX0213.2 > JISX0213.1
		ch = 0x0000;
		goto ENDOF;
	case 044:		// JISX0213.2 > JISX0213.2
		goto ENDOF;

	case 005:		// DEC_TCS-GR > ???
		ch &= 0x7F;
		if ( ch >= 0x0020 && ch <= 0x007F )
			ch = DecTCS[ch - 0x20];
		from = _T("UCS-4BE");
		break;
	case 006:		// DEC_SGCS-GR > ???
		ch &= 0x7F;
		if ( ch >= 0x0060 && ch <= 0x007F )
			ch = DecSGCS[ch - 0x60];
		from = _T("UCS-4BE");
		break;
	}

	if ( _tcscmp(from, _T("UCS-4BE")) == 0 ) {
		itmp[n++] = (unsigned char)(ch >> 24);
		itmp[n++] = (unsigned char)(ch >> 16);
		itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	} else if ( _tcscmp(from, _T("UTF-16BE")) == 0 ) {
		if ( (ch & 0xFFFF0000) != 0 ) {
			itmp[n++] = (unsigned char)(ch >> 24);
			itmp[n++] = (unsigned char)(ch >> 16);
		}
		itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	} else {
		if ( (ch & 0xFF000000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 24);
		if ( (ch & 0xFFFF0000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 16);
		if ( (ch & 0xFFFFFF00) != 0 )
			itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	}

    inp = (char *)itmp;
    ins = n;
    otp = (char *)otmp;
    ots = 32;

    while ( ins > 0 ) {
        if ( iconv(cp->m_Cd, (char **)&inp, &ins, &otp, &ots) == (size_t)(-1) )
	        return 0x0000;
    }
	iconv(cp->m_Cd, NULL, NULL, &otp, &ots);

	ch = 0;
	otp = (char *)otmp;
	for ( n = (int)(32 - ots) ; n > 0 ;  n-- )
		ch = (ch << 8) | (unsigned char)(*(otp++));

	switch(cp->m_Mode & 070) {
	case 030:		// JISX0213.1
		if ( ch >= 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		break;
	case 040:	// JISX0213.2
		if ( ch < 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		break;
	}

ENDOF:
	if ( (od & 0xFFFFFF00) == 0 )
		cp->m_Table[od] = ch;
	return ch;
}

static int GetCharSet(unsigned int namescount, const char * const * names, void* data)
{
	unsigned int i;
	CStringArray *pArray = (CStringArray *)data;

	for ( i = 0 ; i < namescount ; i++ )
		pArray->Add(MbsToTstr(names[i]));
	return 0;
}
void CIConv::SetListArray(CStringArray &stra)
{
	stra.RemoveAll();
	iconvlist(GetCharSet, &stra);
	stra.Add(_T("JIS_X0213-2000.1"));
	stra.Add(_T("JIS_X0213-2000.2"));
	stra.Add(_T("DEC_TCS-GR"));
	stra.Add(_T("DEC_SGCS-GR"));
}