// Data.cpp: CData クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Ssh.h"
#include "Data.h"
#include "IdKeyFileDlg.h"
#include "Script.h"

#include <ocidl.h>
#include <olectl.h>
#include <ole2.h>
#include <afxinet.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// lib

int	BinaryFind(void *ptr, void *tab, int size, int max, int (* func)(const void *, const void *), int *base)
{
	int n, c;
	int b = 0;
	int m = max - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = (*func)(ptr, (BYTE *)tab + size * n)) == 0 ) {
			if ( base != NULL )
				*base = n;
			return TRUE;
		} else if ( c > 0 )
			b = n + 1;
		else
			m = n - 1;
	}
	if ( base != NULL )
		*base = b;
	return FALSE;
}
BOOL IsZeroMemory(void *ptr, int len)
{
#if 0
	int n;
	BYTE *p = (BYTE *)ptr;

	for ( n = 0 ; n < len ; n++ ) {
		if ( *p != 0 )
			return FALSE;
		p++;
	}

	return TRUE;
#else
	BYTE *p = (BYTE *)ptr;

	if ( *p == 0 && memcmp(p, p + 1, len - 1) == 0 )
		return TRUE;

	return FALSE;
#endif
}

//////////////////////////////////////////////////////////////////////
// CBuffer

#ifdef	DEBUG
void CBuffer::Dump()
{
	int n;

	for ( n = 0 ; n < GetSize() ; n++ ) {
		TRACE("%02x ", m_Data[m_Ofs + n]);
		if ( (n % 16) == 15 )
			TRACE("\n");
	}
	if ( (n % 16) != 0 )
		TRACE("\n");
}
#endif

CBuffer::CBuffer(LPBYTE pData, int len)
{
	m_bZero = FALSE;
	m_Ofs   = 0;
	m_Len   = len;
	m_Max   = 0;
	m_Data  = pData;
}
CBuffer::CBuffer(int size)
{
	if ( size < 0 ) {
		m_bZero = TRUE;
		size = 32;
	} else
		m_bZero = FALSE;

	m_Ofs = m_Len = 0;
	m_Max = size;
	m_Data = new BYTE[m_Max];
}
CBuffer::CBuffer()
{
	m_bZero = FALSE;
	m_Ofs = m_Len = 0;
	m_Max = 32;
	m_Data = new BYTE[m_Max];
}
CBuffer::~CBuffer()
{
	if ( m_Max == 0 )
		return;

	if ( m_bZero )
		SecureZeroMemory(m_Data, m_Max);

	delete [] m_Data;
}
void CBuffer::RemoveAll()
{
	if ( m_Max == 0 )
		throw _T("RemoveAll : Read Only");

	if ( m_bZero )
		SecureZeroMemory(m_Data, m_Max);

	delete [] m_Data;

	m_Len = m_Ofs = 0;
	m_Max = 32;
	m_Data = new BYTE[m_Max];
}
void CBuffer::Move(CBuffer &data)
{
	if ( m_Max == 0 )
		throw _T("Move : Read Only");

	if ( m_bZero )
		SecureZeroMemory(m_Data, m_Max);

	delete [] m_Data;

	m_Len  = data.m_Len;
	m_Ofs  = data.m_Ofs;
	m_Max  = data.m_Max;
	m_Data = data.m_Data;

	data.m_Len = data.m_Ofs = 0;
	data.m_Max = 32;
	data.m_Data = new BYTE[data.m_Max];
}
void CBuffer::Consume(int len)
{
	m_Ofs += len;
	if ( m_Ofs >= m_Len )
		m_Len = m_Ofs = 0;
}
void CBuffer::ReAlloc(int len)
{
	if ( m_Max == 0 )
		throw _T("ReAlloc : Read Only");

	int oldMax = m_Max;

	if ( (len += m_Len) <= m_Max )
		return;
	
	if ( (len -= m_Ofs) <= m_Max ) {
		if ( (m_Len -= m_Ofs) > 0 )
			memcpy(m_Data, m_Data + m_Ofs, m_Len);
		m_Ofs = 0;
		return;
	}

	m_Max = (len * 2 + NIMALLOC) & ~(NIMALLOC - 1);
	LPBYTE tmp = new BYTE[m_Max];

	if ( (m_Len -= m_Ofs) > 0 )
		memcpy(tmp, m_Data + m_Ofs, m_Len);
	m_Ofs = 0;

	if ( m_bZero )
		SecureZeroMemory(m_Data, oldMax);

	delete [] m_Data;
	m_Data = tmp;
}
void CBuffer::Apend(LPBYTE buff, int len)
{
	if ( len <= 0 )
		return;
	ReAlloc(len);
	memcpy(m_Data + m_Len, buff, len);
	m_Len += len;
}
void CBuffer::RoundUp(int len)
{
	int n = len - (GetSize() % len);
	while ( n-- > 0 )
		Put8Bit(0);
}

void CBuffer::SetMbsStr(LPCTSTR str)
{
    CStringA tmp;

	while ( *str != _T('\0') ) {
		tmp = *(str++);
		*this += tmp;
	}
}
LPCSTR CBuffer::operator += (LPCSTR str)
{
	int len = (int)strlen(str);

	ReAlloc(len + sizeof(CHAR));
	memcpy(m_Data + m_Len, str, len);
	m_Len += len;
	return *this;
}
LPCWSTR CBuffer::operator += (LPCWSTR str)
{
	int len = (int)wcslen(str) * sizeof(WCHAR);

	ReAlloc(len + sizeof(WCHAR));
	memcpy(m_Data + m_Len, str, len);
	m_Len += len;
	return *this;
}
LPBYTE CBuffer::PutSpc(int len)
{
	ReAlloc(len);
	m_Len += len;
	return (m_Data + m_Len - len);
}
void CBuffer::Put8Bit(int val)
{
	ReAlloc(1);
	register LPBYTE p = m_Data + m_Len;
	*p = (BYTE)val;
	m_Len += 1;
}
void CBuffer::Put16Bit(int val)
{
	ReAlloc(2);
	register LPBYTE p = m_Data + m_Len;
	p[0] = (BYTE)(val >> 8);
	p[1] = (BYTE)(val >> 0);
	m_Len += 2;
}
void CBuffer::Put32Bit(LONG val)
{
	ReAlloc(4);
	register LPBYTE p = m_Data + m_Len;
	p[0] = (BYTE)(val >> 24);
	p[1] = (BYTE)(val >> 16);
	p[2] = (BYTE)(val >>  8);
	p[3] = (BYTE)(val >>  0);
	m_Len += 4;
}
void CBuffer::Put64Bit(LONGLONG val)
{
	ReAlloc(8);
	register LPBYTE p = m_Data + m_Len;
	p[0] = (BYTE)(val >> 56);
	p[1] = (BYTE)(val >> 48);
	p[2] = (BYTE)(val >> 40);
	p[3] = (BYTE)(val >> 32);
	p[4] = (BYTE)(val >> 24);
	p[5] = (BYTE)(val >> 16);
	p[6] = (BYTE)(val >>  8);
	p[7] = (BYTE)(val >>  0);
	m_Len += 8;
}
void CBuffer::PutBuf(LPBYTE buf, int len)
{
	Put32Bit(len);
	Apend(buf, len);
}
void CBuffer::PutStr(LPCSTR str)
{
	int len = (int)strlen(str);
	Put32Bit(len);
	Apend((LPBYTE)str, len);
}
void CBuffer::PutBIGNUM(BIGNUM *val)
{
	int bits = BN_num_bits(val);
	int bin_size = (bits + 7) / 8;
	LPBYTE tmp = new BYTE[bin_size];
	int len;
	if ( (len = BN_bn2bin(val, tmp)) != bin_size )
		throw _T("CBuffer PutBIGNUM");
	Put16Bit(bits);
	Apend(tmp, len);
	delete [] tmp;
}
void CBuffer::PutBIGNUM2(BIGNUM *val)
{
	int bytes;
	int hnoh;
	LPBYTE tmp;

	if (BN_is_zero(val)) {
		Put32Bit(0);
		return;
	}
	if (val->neg)
		throw _T("CBuffer PutBIGNUM2 neg");

	bytes = BN_num_bytes(val) + 1; /* extra padding byte */
    if ( bytes < 2 )
		throw _T("CBuffer PutBIGNUM2 size");

	if ( (tmp = new BYTE[bytes]) == NULL )
		throw _T("CBuffer PutBIGNUM2 new");

	tmp[0] = '\x00';

    if ( BN_bn2bin(val, tmp + 1) != (bytes - 1) )
		throw _T("CBuffer PutBIGNUM2 big");

	hnoh = (tmp[1] & 0x80) ? 0 : 1;
	Put32Bit(bytes - hnoh);
	Apend(tmp + hnoh, bytes - hnoh);
	delete [] tmp;
}
void CBuffer::PutEcPoint(const EC_GROUP *curve, const EC_POINT *point)
{
	int len;
	LPBYTE tmp;
	BN_CTX *bnctx;

	if ( (bnctx = BN_CTX_new()) == NULL )
		throw _T("CBuffer PutEcPoint mem");

	len = (int)EC_POINT_point2oct(curve, point, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, bnctx);

	if ( (tmp = new BYTE[len]) == NULL )
		throw _T("CBuffer PutEcPoint new");

	if ( EC_POINT_point2oct(curve, point, POINT_CONVERSION_UNCOMPRESSED, tmp, len, bnctx) != len )
		throw _T("CBuffer PutEcPoint len");

	PutBuf(tmp, len);

	BN_CTX_free(bnctx);
	delete [] tmp;
}
void CBuffer::PutDword(int val)
{
	ReAlloc(sizeof(DWORD));
	register LPDWORD p = (LPDWORD)(m_Data + m_Len);
	*p = (DWORD)val;
	m_Len += sizeof(DWORD);
}
void CBuffer::PutWord(int val)
{
	ReAlloc(sizeof(WORD));
	register LPWORD p = (LPWORD)(m_Data + m_Len);
	*p = (WORD)val;
	m_Len += sizeof(WORD);
}
void CBuffer::PutText(LPCWSTR str)
{
	int len = (int)wcslen(str) + 1;

	ReAlloc(len * sizeof(WCHAR));
	wcscpy((WCHAR *)(m_Data + m_Len), str);
	m_Len += (len * sizeof(WCHAR));
}
int CBuffer::Get8Bit()
{
	register LPBYTE p = m_Data + m_Ofs;
	if ( (m_Ofs += 1) > m_Len ) {
		m_Len = m_Ofs = 0;
		throw _T("CBuffer Get8Bit");
	}
	return *p;
}
int CBuffer::Get16Bit()
{
	register LPBYTE p = m_Data + m_Ofs;
	if ( (m_Ofs += 2) > m_Len ) {
		m_Len = m_Ofs = 0;
		throw _T("CBuffer Get16Bit");
	}
	return ((int)p[0] << 8) |
		   ((int)p[1] << 0);
}
LONG CBuffer::Get32Bit()
{
	register LPBYTE p = m_Data + m_Ofs;
	if ( (m_Ofs += 4) > m_Len ) {
		m_Len = m_Ofs = 0;
		throw _T("CBuffer Get32Bit");
	}
	return ((LONG)p[0] << 24) |
		   ((LONG)p[1] << 16) |
		   ((LONG)p[2] <<  8) |
		   ((LONG)p[3] <<  0);
}
LONGLONG CBuffer::Get64Bit()
{
	register LPBYTE p = m_Data + m_Ofs;
	if ( (m_Ofs += 8) > m_Len ) {
		m_Len = m_Ofs = 0;
		throw _T("CBuffer Get64Bit");
	}
	return ((LONGLONG)p[0] << 56) |
		   ((LONGLONG)p[1] << 48) |
		   ((LONGLONG)p[2] << 40) |
		   ((LONGLONG)p[3] << 32) |
		   ((LONGLONG)p[4] << 24) |
		   ((LONGLONG)p[5] << 16) |
		   ((LONGLONG)p[6] <<  8) |
		   ((LONGLONG)p[7] <<  0);
}
int CBuffer::GetStr(CStringA &str)
{
	int len = Get32Bit();
	if ( len < 0 || len > (256 * 1024) || (m_Len - m_Ofs) < len )
		throw _T("CBuffer GetStr");
	memcpy(str.GetBufferSetLength(len), m_Data + m_Ofs, len);
	Consume(len);
	return TRUE;
}
int CBuffer::GetBuf(CBuffer *buf)
{
	int len = Get32Bit();
	if ( len < 0 || len > (256 * 1024) || (m_Len - m_Ofs) < len )
		throw _T("CBuffer GetBuf");
	buf->Apend(GetPtr(), len);
	Consume(len);
	return TRUE;
}
int CBuffer::GetBIGNUM(BIGNUM *val)
{
	int bytes;
	int bits = Get16Bit();
	if ( (bytes = (bits + 7) / 8) > (8 * 1024) || bytes < 0 )
		throw _T("CBuffer GetBIGNUM size");
	if ( (m_Len - m_Ofs) < bytes )
		throw _T("CBuffer GetBIGNUM len");
    BN_bin2bn(m_Data + m_Ofs, bytes, val);
	Consume(bytes);
	return TRUE;
}
int CBuffer::GetBIGNUM2(BIGNUM *val)
{
	int bytes = Get32Bit();
	if ( bytes < 0 || bytes > (32 * 1024) || (m_Len - m_Ofs) < bytes )
		throw _T("CBuffer GetBIGNUM2 size");
    BN_bin2bn(m_Data + m_Ofs, bytes, val);
	Consume(bytes);
	return TRUE;
}
int CBuffer::GetBIGNUM_SecSh(BIGNUM *val)
{
	int bytes = (Get32Bit() + 7) / 8;
	if ( bytes < 0 || bytes > (32 * 1024) || (m_Len - m_Ofs) < bytes )
		throw _T("CBuffer GetBIGNUM_SecSh");
	if ( (m_Data[m_Ofs] & 0x80) != 0 ) {
		if ( m_Ofs > 0 ) {
			m_Data[m_Ofs - 1] = '\0';
		    BN_bin2bn(m_Data + m_Ofs - 1, bytes + 1, val);
		} else {
			LPBYTE tmp = new BYTE[bytes + 1];
			tmp[0] = '\0';
			memcpy(tmp + 1, m_Data + m_Ofs, bytes);
		    BN_bin2bn(tmp, bytes + 1, val);
			delete [] tmp;
		}
	} else
	    BN_bin2bn(m_Data + m_Ofs, bytes, val);
	Consume(bytes);
	return TRUE;
}
int CBuffer::GetEcPoint(const EC_GROUP *curve, EC_POINT *point)
{
	int ret;
	int len = Get32Bit();
	LPBYTE buf = GetPtr();
	BN_CTX *bnctx;

	if ( len <= 0 || len > (32 * 1024) || (m_Len - m_Ofs) < len )
		return FALSE;

	if ( buf[0] != POINT_CONVERSION_UNCOMPRESSED )
		return FALSE;

	if ( (bnctx = BN_CTX_new()) == NULL )
		throw _T("CBuffer GetEcPoint");

	ret = EC_POINT_oct2point(curve, point, buf, len, bnctx);

	BN_CTX_free(bnctx);
	Consume(len);

	return (ret == 1 ? TRUE : FALSE);
}
int CBuffer::GetDword()
{
	register LPDWORD p = (LPDWORD)(m_Data + m_Ofs);
	if ( (m_Ofs += sizeof(DWORD)) > m_Len ) {
		m_Len = m_Ofs = 0;
		throw _T("CBuffer GetDword");
	}
	return *p;
}
int CBuffer::GetWord()
{
	register LPWORD p = (LPWORD)(m_Data + m_Ofs);
	if ( (m_Ofs += sizeof(WORD)) > m_Len ) {
		m_Len = m_Ofs = 0;
		throw _T("CBuffer GetWord");
	}
	return *p;
}
int CBuffer::GetChar()
{
	register LPBYTE p = (LPBYTE)(m_Data + m_Ofs);
	if ( (m_Ofs += sizeof(BYTE)) > m_Len ) {
		m_Len = m_Ofs = 0;
		throw _T("CBuffer GetChar");
	}
	return *p;
}
void CBuffer::GetText(CString &str)
{
	WCHAR ch;
	CStringW tmp;

	while ( (ch = (WCHAR)GetWord()) != L'\0' )
		tmp += ch;

	str = UniToTstr(tmp);
}
void CBuffer::SET16BIT(LPBYTE pos, int val)
{
	pos[0] = (BYTE)(val >> 8);
	pos[1] = (BYTE)(val >> 0);
}
void CBuffer::SET32BIT(LPBYTE pos, int val)
{
	pos[0] = (BYTE)(val >> 24);
	pos[1] = (BYTE)(val >> 16);
	pos[2] = (BYTE)(val >>  8);
	pos[3] = (BYTE)(val >>  0);
}
void CBuffer::SET64BIT(LPBYTE pos, LONGLONG val)
{
	pos[0] = (BYTE)(val >> 56);
	pos[1] = (BYTE)(val >> 48);
	pos[2] = (BYTE)(val >> 40);
	pos[3] = (BYTE)(val >> 32);
	pos[4] = (BYTE)(val >> 24);
	pos[5] = (BYTE)(val >> 16);
	pos[6] = (BYTE)(val >>  8);
	pos[7] = (BYTE)(val >>  0);
}
int CBuffer::PTR16BIT(LPBYTE pos)
{
	return ((int)pos[0] << 8) |
		   ((int)pos[1] << 0);
}
LONG CBuffer::PTR32BIT(LPBYTE pos)
{
	return ((LONG)pos[0] << 24) |
		   ((LONG)pos[1] << 16) |
		   ((LONG)pos[2] <<  8) |
		   ((LONG)pos[3] <<  0);
}
LONGLONG CBuffer::PTR64BIT(LPBYTE pos)
{
	return ((LONGLONG)pos[0] << 56) |
		   ((LONGLONG)pos[1] << 48) |
		   ((LONGLONG)pos[2] << 40) |
		   ((LONGLONG)pos[3] << 32) |
		   ((LONGLONG)pos[4] << 24) |
		   ((LONGLONG)pos[5] << 16) |
		   ((LONGLONG)pos[6] <<  8) |
		   ((LONGLONG)pos[7] <<  0);
}

#ifdef	_UNICODE
  #define	PutTChar(c)		PutWord(c)
#else
  #define	PutTChar(c)		Put8Bit(c)
#endif

static const char *Base64EncTab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Base64DecTab[] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

LPCTSTR CBuffer::Base64Decode(LPCTSTR str)
{
	int n, c, o;

	Clear();
	for ( n = o = 0 ; *str != _T('\0') ; n++, str++ ) {
		while ( *str == _T('\r') || *str == _T('\n') )
			str++;
		if ( (c = Base64DecTab[(BYTE)(*str)]) < 0 )
			break;
		switch(n % 4) {
		case 0:
			o = c << 2;
			break;
		case 1:
			o |= (c >> 4);
			Put8Bit(o);
			o = c << 4;
			break;
		case 2:
			o |= (c >> 2);
			Put8Bit(o);
			o = c << 6;
			break;
		case 3:
			o |= c;
			Put8Bit(o);
			break;
		}
	}
	return str;
}
void CBuffer::Base64Encode(LPBYTE buf, int len)
{
	int n;
	Clear();
	for ( n = len ; n > 0 ; n -= 3, buf += 3 ) {
		if ( n >= 3 ) {
			PutTChar(Base64EncTab[(buf[0] >> 2) & 077]);
			PutTChar(Base64EncTab[((buf[0] << 4) | (buf[1] >> 4)) & 077]);
			PutTChar(Base64EncTab[((buf[1] << 2) | (buf[2] >> 6)) & 077]);
			PutTChar(Base64EncTab[buf[2] & 077]);
		} else if ( n >= 2 ) {
			PutTChar(Base64EncTab[(buf[0] >> 2) & 077]);
			PutTChar(Base64EncTab[((buf[0] << 4) | (buf[1] >> 4)) & 077]);
			PutTChar(Base64EncTab[(buf[1] << 2) & 077]);
			PutTChar('=');
		} else {
			PutTChar(Base64EncTab[(buf[0] >> 2) & 077]);
			PutTChar(Base64EncTab[(buf[0] << 4) & 077]);
			PutTChar('=');
			PutTChar('=');
		}
	}
}

static const char *QuotedEncTab = "0123456789abcdef";
static const char QuotedDecTab[] = {
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	 0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
	-1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1 };

LPCTSTR CBuffer::QuotedDecode(LPCTSTR str)
{
    int c1, c2;

	Clear();
	while ( *str != _T('\0') ) {
		if ( str[0] == _T('=') && (c1 = QuotedDecTab[(BYTE)str[1]]) >= 0 && (c2 = QuotedDecTab[(BYTE)str[2]]) >= 0 ) {
			Put8Bit((c1 << 4) | c2);
			str += 3;
		} else if ( str[0] == _T('=') && str[1] == _T('\n') ) {
		    str += 2;
		} else if ( str[0] == _T('=') && str[1] == _T('\0') ) {
		    break;
		} else {
			Put8Bit(*str);
			str += 1;
		}
    }
	return str;
}
void CBuffer::QuotedEncode(LPBYTE buf, int len)
{
	Clear();
    while ( len > 0 ) {
		if ( (*buf != '\n' && *buf != '\t' && *buf < ' ' ) || *buf >= 127 || *buf == '=' ) {
			PutTChar('=');
			PutTChar(QuotedEncTab[*buf >> 4]);
			PutTChar(QuotedEncTab[*buf & 15]);
		} else {
			PutTChar(*buf);
		}
		buf += 1;
		len -= 1;
    }
}

LPCTSTR CBuffer::Base16Decode(LPCTSTR str)
{
    int c1, c2;

	Clear();
	while ( *str != _T('\0') ) {
		if ( (c1 = QuotedDecTab[(BYTE)str[0]]) >= 0 && (c2 = QuotedDecTab[(BYTE)str[1]]) >= 0 ) {
			Put8Bit((c1 << 4) | c2);
			str += 2;
		} else
			break;
	}
	return str;
}
void CBuffer::Base16Encode(LPBYTE buf, int len)
{
	Clear();
    while ( len > 0 ) {
		PutTChar(QuotedEncTab[*buf >> 4]);
		PutTChar(QuotedEncTab[*buf & 15]);
		buf += 1;
		len -= 1;
    }
}

static const char BubBabVowels[] = { 'a', 'e', 'i', 'o', 'u', 'y' };
static const char BubBabConsonants[] = { 'b', 'c', 'd', 'f', 'g', 'h', 'k', 'l', 'm', 'n', 'p', 'r', 's', 't', 'v', 'z', 'x' };
static const char BubBabDecTab[] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 00
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 10
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 20
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 30
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 40
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 50
//			 a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
        -1,  0, 10, 11, 12,  1, 13, 14, 15,  2, -1, 16, 17, 18, 19,  3,		// 60
//		 p   q   r   s   t   u   v   w   x   y   z
        20, -1, 21, 22, 23,  4, 24, -1, 26,  5, 25, -1, -1, -1, -1, -1,		// 70
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 80
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 90
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// A0
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// B0
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// C0
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// D0
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// E0
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };	// F0

void CBuffer::BubbleBabble(LPBYTE buf, int len)
{
	u_int i, rounds, seed = 1;

	rounds = (len / 2) + 1;
	PutTChar('x');

	for ( i = 0 ; i < rounds ; i++ ) {
		u_int idx0, idx1, idx2, idx3, idx4;
		if ( (i + 1 < rounds) || (len % 2 != 0) ) {
			idx0 = (((((u_int)(buf[2 * i])) >> 6) & 3) + seed) % 6;
			idx1 = (((u_int)(buf[2 * i])) >> 2) & 15;
			idx2 = ((((u_int)(buf[2 * i])) & 3) + (seed / 6)) % 6;
			PutTChar(BubBabVowels[idx0]);
			PutTChar(BubBabConsonants[idx1]);
			PutTChar(BubBabVowels[idx2]);
			if ( (i + 1) < rounds ) {
				idx3 = (((u_int)(buf[(2 * i) + 1])) >> 4) & 15;
				idx4 = (((u_int)(buf[(2 * i) + 1]))) & 15;
				PutTChar(BubBabConsonants[idx3]);
				PutTChar('-');
				PutTChar(BubBabConsonants[idx4]);
				seed = ((seed * 5) + ((((u_int)(buf[2 * i])) * 7) + ((u_int)(buf[(2 * i) + 1])))) % 36;
			}
		} else {
			idx0 = seed % 6;
			idx1 = 16;
			idx2 = seed / 6;
			PutTChar(BubBabVowels[idx0]);
			PutTChar(BubBabConsonants[idx1]);
			PutTChar(BubBabVowels[idx2]);
		}
	}

	PutTChar('x');
	PutTChar('\0');
}
LPCTSTR CBuffer::BubBabDecode(LPCTSTR str)
{
	//  012345012345012345012345
	// xaaaa-aaaaa-aaaaa-aaaax
	int hi, mid, low, seed = 1;
	int byte[5], data[2];

	Clear();
	if ( *(str++) != _T('x') )
		return str;

	while ( *str != _T('\0') ) {
		if ( (byte[0] = BubBabDecTab[(BYTE)str[0]]) < 0 )
			break;
		if ( (byte[1] = BubBabDecTab[(BYTE)str[1]]) < 0 )
			break;
		if ( (byte[2] = BubBabDecTab[(BYTE)str[2]]) < 0 )
			break;
		if ( (byte[3] = BubBabDecTab[(BYTE)str[3]]) < 0 )
			break;

		if ( str[4] == _T('-') ) {
			if ( (byte[4] = BubBabDecTab[(BYTE)str[5]]) < 0 )
				break;

			if ( byte[0] >= 10 )		// Not Vowels
				break;
			if ( (byte[1] -= 10) < 0 )	// Not Consonants
				break;
			if ( byte[2] >= 10 )		// Not Vowels
				break;
			if ( (byte[3] -= 10) < 0 )	// Not Consonants
				break;
			if ( (byte[4] -= 10) < 0 )	// Not Consonants
				break;

			if ( (hi = (byte[0] - (seed % 6) + 6) % 6) >= 4 )
				break;
			if ( (mid = byte[1]) >= 16 )
				break;
			if ( (low = (byte[2] - (seed / 6 % 6) + 6) % 6) >= 4 )
				break;

			data[0] = (hi << 6) | (mid << 2) | low;

			if ( (hi = byte[3]) >= 16 )
				break;
			if ( (low = byte[4]) >= 16 )
				break;

			data[1] = (hi << 4) | low;

			Put8Bit(data[0]);
			Put8Bit(data[1]);

			seed = (seed * 5 + data[0] * 7 + data[1]) % 36;

			str += 6;

		} else if ( str[4] == _T('\0') ) {
			if ( str[3] == _T('x') ) {
				if ( byte[0] != (seed % 6) || byte[2] != (seed / 6) ) {
					Clear();	// SumError
					break;
				}
			} else {
				// Not Use ?
				if ( byte[0] >= 10 )		// Not Vowels
					break;
				if ( (byte[1] -= 10) < 0 )	// Not Consonants
					break;
				if ( byte[2] >= 10 )		// Not Vowels
					break;
				if ( (byte[3] -= 10) < 0 )	// Not Consonants
					break;
				if ( (byte[4] -= 10) < 0 )	// Not Consonants
					break;

				if ( (hi = (byte[0] - (seed % 6) + 6) % 6) >= 4 )
					break;
				if ( (mid = byte[1]) >= 16 )
					break;
				if ( (low = (byte[2] - (seed / 6 % 6) + 6) % 6) >= 4 )
					break;

				data[0] = (hi << 6) | (mid << 2) | low;

				Put8Bit(data[0]);
			}
			str += 4;
			break;
		} else
			break;
	}

	return str;
}

void CBuffer::md5(LPCTSTR str)
{
	unsigned int dlen;
	const EVP_MD *evp_md;
	u_char digest[EVP_MAX_MD_SIZE];
	EVP_MD_CTX md;
	CStringA tmp(str);

	evp_md = EVP_md5();
	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, (const void *)(LPCSTR)tmp, tmp.GetLength());
	EVP_DigestFinal(&md, digest, &dlen);

	Base16Encode(digest, dlen);
}
BOOL CBuffer::LoadFile(LPCTSTR filename)
{
	CFile file;
	ULONGLONG FileSize;

	if( !file.Open(filename, CFile::modeRead) )
		return FALSE;

	if ( (FileSize = file.GetLength()) > (512 * 1024 * 1024) )
		return FALSE;

	if ( m_bZero )
		SecureZeroMemory(m_Data, m_Max);

	delete [] m_Data;

	m_Len = m_Ofs = 0;
	m_Max = (int)FileSize;
	m_Data = new BYTE[m_Max];

	if ( file.Read(m_Data, m_Max) != m_Max )
		return FALSE;

	file.Close();

	m_Len = m_Max;

	return TRUE;
}
int CBuffer::KanjiCheck(int type)
{
	int n;
	CTextRam::KANCODEWORK work;

	if ( GetSize() >= 2 && m_Data[m_Ofs] == 0xFF && m_Data[m_Ofs + 1] == 0xFE ) {
		type = KANJI_UTF16LE;

	} else if ( GetSize() >= 2 && m_Data[m_Ofs] == 0xFE && m_Data[m_Ofs + 1] == 0xFF ) {
		type = KANJI_UTF16BE;

	} else {
		CTextRam::KanjiCodeInit(&work);

		for ( n = m_Ofs ; n < m_Len ; n++ )
			CTextRam::KanjiCodeCheck(m_Data[n], &work);

		if ( work.euc_rs > work.sjis_rs && work.euc_rs > work.utf8_rs )
			type = KANJI_EUC;
		else if ( work.sjis_rs > work.euc_rs && work.sjis_rs > work.utf8_rs )
			type = KANJI_SJIS;
		else if ( work.utf8_rs > work.euc_rs && work.utf8_rs > work.sjis_rs )
			type = KANJI_UTF8;
	}

	return type;
}
void CBuffer::KanjiConvert(int type)
{
	int n;
	TCHAR ch;
	CStringA mbs;
	CString str;
	CBuffer work;
	CIConv iconv;

	switch(type) {
	case KANJI_EUC:
		for ( n = m_Ofs ; n < m_Len ; n++ ) {
			mbs += (CHAR)m_Data[n];
			if ( m_Data[n] == '\n' ) {
				iconv.RemoteToStr(_T("EUCJP-MS"), mbs, str);
				work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
				mbs.Empty();
			}
		}
		if ( !mbs.IsEmpty() ) {
			iconv.RemoteToStr(_T("EUCJP-MS"), mbs, str);
			work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
		}
		Move(work);
		break;

	case KANJI_SJIS:
		for ( n = m_Ofs ; n < m_Len ; n++ ) {
			mbs += (CHAR)m_Data[n];
			if ( m_Data[n] == '\n' ) {
				str = mbs;
				work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
				mbs.Empty();
			}
		}
		if ( !mbs.IsEmpty() ) {
			str = mbs;
			work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
		}
		Move(work);
		break;

	case KANJI_UTF8:
		for ( n = m_Ofs ; n < m_Len ; n++ ) {
			mbs += (CHAR)m_Data[n];
			if ( m_Data[n] == '\n' ) {
				iconv.RemoteToStr(_T("UTF-8"), mbs, str);
				work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
				mbs.Empty();
			}
		}
		if ( !mbs.IsEmpty() ) {
			iconv.RemoteToStr(_T("UTF-8"), mbs, str);
			work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
		}
		Move(work);
		break;

	case KANJI_UTF16LE:
		for ( n = m_Ofs ; (n + 1) < m_Len ; n += 2 ) {
			ch = m_Data[n] | (m_Data[n + 1] << 8);
			str += ch;
			if ( ch == _T('\n') ) {
				work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
				str.Empty();
			}
		}
		if ( !str.IsEmpty() )
			work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
		Move(work);
		break;

	case KANJI_UTF16BE:
		for ( n = m_Ofs ; (n + 1) < m_Len ; n += 2 ) {
			ch = (m_Data[n] << 8) | m_Data[n + 1];
			str += ch;
			if ( ch == _T('\n') ) {
				work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
				str.Empty();
			}
		}
		if ( !str.IsEmpty() )
			work.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
		Move(work);
		break;
	}

	if ( GetSize() >= 2 && m_Data[m_Ofs] == 0xFF && m_Data[m_Ofs + 1] == 0xFE )
		Consume(2);
}

//////////////////////////////////////////////////////////////////////
// CStringArrayExt

CStringArrayExt::CStringArrayExt()
{
}
CStringArrayExt::CStringArrayExt(int nID)
{
	CStringLoad str(nID);
	GetString(str);
}
void CStringArrayExt::AddBin(void *buf, int len)
{
	CBuffer tmp;

	tmp.Base64Encode((LPBYTE)buf, len);
	Add((LPCTSTR)tmp);
}
void CStringArrayExt::GetBuf(int index, CBuffer &buf)
{
	buf.Base64Decode(GetAt(index));
}
int CStringArrayExt::GetBin(int index, void *buf, int len)
{
	CBuffer tmp;

	tmp.Base64Decode(GetAt(index));
	if ( len > tmp.GetSize() )
		len = tmp.GetSize();
	memcpy(buf, tmp.GetPtr(), len);
	return len;
}
void CStringArrayExt::SetString(CString &str, int sep)
{
	int n;

	str = "";
	for ( n = 0 ; n < GetSize() ; n++ ) {
		str += GetAt(n);
		str += (char)sep;
	}
}
void CStringArrayExt::GetString(LPCTSTR str, int sep)
{
	int i, a;
	CString work = str;

	RemoveAll();
	for ( i = 0 ; i < work.GetLength() ; i = a + 1 ) {
		if ( (a = work.Find(sep, i)) < 0 )
			a = work.GetLength();
		if ( (a - i) >= 0 )
			Add(work.Mid(i, a - i));
	}
}
void CStringArrayExt::AddArray(CStringArrayExt &stra)
{
	CString tmp;
	stra.SetString(tmp);
	Add(tmp);
}
void CStringArrayExt::GetArray(int index, CStringArrayExt &stra)
{
	stra.GetString(GetAt(index));
}
void CStringArrayExt::SetBuffer(CBuffer &buf)
{
	buf.Put32Bit((int)GetSize());
	for ( int n = 0 ; n < GetSize() ; n++ )
		buf.PutStr(TstrToMbs(GetAt(n)));
}
void CStringArrayExt::GetBuffer(CBuffer &buf)
{
	int n, mx;
	CStringA str;

	RemoveAll();
	if ( buf.GetSize() < 4 )
		return;
	mx = buf.Get32Bit();
	for ( n = 0 ; n < mx ; n++ ) {
		buf.GetStr(str);
		Add(MbsToTstr(str));
	}
}
void CStringArrayExt::Serialize(CArchive& ar)
{
#ifdef	_UNICODE
	int n;
	CStringA str;
	CHAR tmp[4];

	if ( ar.IsStoring() ) {
		// TODO: この位置に保存用のコードを追加してください。
		for ( n = 0 ; n < GetSize() ; n++ ) {
			str = GetAt(n);
			str += "\n";
			ar.Write((LPCSTR)str, str.GetLength());
		}
		ar.Write("ENDOF\n", 6);

	} else {
		// TODO: この位置に読み込み用のコードを追加してください。
		RemoveAll();
		str.Empty();
		while ( ar.Read(tmp, 1) == 1 ) {
			if ( tmp[0] == '\n' ) {
				if ( str.Compare("ENDOF") == 0 )
					break;
				Add(MbsToTstr(str));
				str.Empty();
			} else if ( tmp[0] != '\r' )
				str += tmp[0];
		}
	}

#else
	int n;
	CString str;

	if ( ar.IsStoring() ) {
		// TODO: この位置に保存用のコードを追加してください。
		for ( n = 0 ; n < GetSize() ; n++ ) {
			str = GetAt(n);
			str += _T("\n");
			ar.WriteString(str);
		}
		ar.WriteString(_T("ENDOF\n"));

	} else {
		// TODO: この位置に読み込み用のコードを追加してください。
		RemoveAll();
		while ( ar.ReadString(str) ) {
			str.Replace(_T("\n"), _T(""));
			if ( str.Compare(_T("ENDOF")) == 0 )
				break;
			Add(str);
		}
	}
#endif
}
const CStringArrayExt & CStringArrayExt::operator = (CStringArrayExt &data)
{
	RemoveAll();
	for ( int n = 0 ; n < data.GetSize() ; n++ )
		Add(data[n]);
	return *this;
}
int CStringArrayExt::Find(LPCTSTR str)
{
	for ( int n = 0 ; n < GetSize() ; n++ ) {
		if ( GetAt(n).Compare(str) == 0 )
			return n;
	}
	return (-1);
}
int CStringArrayExt::FindNoCase(LPCTSTR str)
{
	for ( int n = 0 ; n < GetSize() ; n++ ) {
		if ( GetAt(n).CompareNoCase(str) == 0 )
			return n;
	}
	return (-1);
}
int CStringArrayExt::Match(LPCTSTR str)
{
	for ( int n = 0 ; n < GetSize() ; n++ ) {
		if ( _tcsncmp(str, GetAt(n), GetAt(n).GetLength()) == 0 )
			return n;
	}
	return (-1);
}


CStrNode *CStringArrayExt::ParseWord(LPCTSTR *ptr)
{
	int type = 0;
	CString tmp;
	LPCTSTR str = *ptr;

	if ( *str == _T('"') ) {
		str++;
		while ( *str != _T('\0') ) {
			if ( *str == _T('"') ) {
				str++;
				break;
			}
			tmp += *(str++);
		}
		type = '"';

	} else if ( *str == _T('\'') ) {
		str++;
		while ( *str != _T('\0') ) {
			if ( *str == _T('\'') ) {
				str++;
				break;
			}
			tmp += *(str++);
		}
		type = '\'';

	} else if ( *str == _T('[') ) {
		tmp += *(str++);
		type = '[';

	} else if ( *str == _T(']') ) {
		tmp += *(str++);
		type = ']';

	} else if ( *str == _T(',') ) {
		tmp += *(str++);
		type = ',';

	} else if ( *str == _T('-') ) {
		tmp += *(str++);
		type = '-';

	} else if ( *str >= _T('0') &&*str <= _T('9') ) {
		while ( *str != _T('\0') ) {
			if ( *str >= _T('0') && *str <= _T('9') )
				tmp += *(str++);
			else
				break;
		}
		type = '0';

	} else if ( *str != _T('\0') ) {
		while ( *str != _T('\0') ) {
			if ( _tcschr(_T(",\"'[]-0123456789"), *str) != NULL )
				break;
			tmp += *(str++);
		}
		type = 'S';
	}

	*ptr = str;

	if ( type == 0 )
		return NULL;

	return new CStrNode(type, tmp);
}
CStrNode *CStringArrayExt::ParseLine(CStrNode *top, LPCTSTR *ptr, BOOL bNest)
{
	int st, ed;
	CString tmp;
	CStrNode *bp = new CStrNode('N', NULL);
	CStrNode *np, *dp, *tp;

	while ( (np = ParseWord(ptr)) != NULL ) {
		if ( np->m_Type == ',' )
			break;
		else if ( np->m_Type == '[' ) {
			delete np;
			np = ParseList(ptr, TRUE);
		} else if ( bNest && np->m_Type == ']' )
			break;

		bp->AddNext(np);
	}

	if ( !bNest )
		bp->AddNext(new CStrNode('E', NULL));
	else {
		for ( tp = bp->m_Next ; tp != NULL ; tp = tp->m_Next ) {
			if ( tp->m_Type == '0' && tp->m_Next != NULL && tp->m_Next->m_Type == '-' && tp->m_Next->m_Next != NULL && tp->m_Next->m_Next->m_Type == '0' ) {
				st = _tstoi(tp->m_Str);
				ed = _tstoi(tp->m_Next->m_Next->m_Str);

				tp->m_Type = 'L';
				dp = tp->m_Next;
				tp->m_Next = dp->m_Next->m_Next;
				dp->m_Next->m_Next = NULL;
				delete dp;

				while ( st <= ed ) {
					tmp.Format(_T("%d"), st++);
					dp = new CStrNode('N', NULL);
					dp->AddNext(new CStrNode('0', tmp));
					tp->AddList(dp);
				}
			}
		}
	}

	top->AddList(bp);

	return np;
}
CStrNode *CStringArrayExt::ParseList(LPCTSTR *ptr, BOOL bNest)
{
	BOOL bLoop = TRUE;
	CStrNode *np;
	CStrNode *top = new CStrNode('L', NULL);

	while ( bLoop && (np = ParseLine(top, ptr, bNest)) != NULL ) {
		if ( bNest && np->m_Type == ']' )
			bLoop = FALSE;
		delete np;
	}

	return top;
}
void CStringArrayExt::ParseNode(CStrNode *top, CStrNode *stack, CString &str)
{
	CStrNode *dp;
	CString tmp;

	while ( top != NULL ) {
		if ( top->m_Type == 'L' ) {
			if ( top->m_Next != NULL ) {
				top->m_Next->m_Stack = stack;
				stack = top->m_Next;
			}
			for ( dp = top->m_List ; dp != NULL ; dp = dp->m_List ) {
				tmp = str;
				ParseNode(dp, stack, tmp);
			}
			return;

		} else if ( top->m_Type == 'E' ) {
			if ( !str.IsEmpty() )
				Add(str);

		} else if ( top->m_Type != 'N' ) {
			str += top->m_Str;
		}

		top = top->m_Next;
	}

	if ( stack != NULL )
		ParseNode(stack, stack->m_Stack, str);
}
void CStringArrayExt::GetParam(LPCTSTR str)
{
	CString tmp;
	CStrNode *top;

	RemoveAll();
	top = ParseList(&str, FALSE);
	ParseNode(top, NULL, tmp);
	delete top;

	//for ( int n = 0 ; n < GetSize() ; n++ )
	//	TRACE("%s\n", TstrToMbs((*this)[n]));
}
void CStringArrayExt::GetCmds(LPCTSTR cmds)
{
	CString tmp;

	RemoveAll();

	while ( *cmds != _T('\0') ) {
		if ( *cmds == _T(' ') || *cmds == _T('\t') ) {
			cmds++;
			if ( !tmp.IsEmpty() ) {
				Add(tmp);
				tmp.Empty();
			}
		} else if ( *cmds == _T('"') ) {
			for ( cmds++ ; *cmds != _T('\0') && *cmds != _T('"') ; ) {
				if ( cmds[0] == _T('\\') && cmds[1] == _T('"') )
					cmds++;
				tmp += *(cmds++);
			}
			if ( *cmds != _T('\0') )
				cmds++;
		} else if ( *cmds == _T('\'') ) {
			for ( cmds++ ; *cmds != _T('\0') && *cmds != _T('\'') ; ) {
				if ( cmds[0] == _T('\\') && cmds[1] == _T('\'') )
					cmds++;
				tmp += *(cmds++);
			}
			if ( *cmds != _T('\0') )
				cmds++;
		} else
			tmp += *(cmds++);
	}

	if ( !tmp.IsEmpty() )
		Add(tmp);
}
void CStringArrayExt::AddSort(LPCTSTR str)
{
	int n, c;
	int b = 0;
	int m = (int)GetSize() - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = GetAt(n).Compare(str)) == 0 )
			return;
		else if ( c > 0 )
			b = n + 1;
		else
			m = n - 1;
	}

	InsertAt(b, str);
}
int CStringArrayExt::FindSort(LPCTSTR str)
{
	int n, c;
	int b = 0;
	int m = (int)GetSize() - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = GetAt(n).Compare(str)) == 0 )
			return n;
		else if ( c > 0 )
			b = n + 1;
		else
			m = n - 1;
	}

	return (-1);
}

//////////////////////////////////////////////////////////////////////
// CMenuLoad

BOOL CMenuLoad::LoadMenu(LPCTSTR lpszResourceName)
{
	HMENU hMenu;

	if ( !((CRLoginApp *)::AfxGetApp())->LoadResMenu(lpszResourceName, hMenu) )
		return FALSE;

	return Attach(hMenu);
}
BOOL CMenuLoad::UpdateMenuShortCut(CMenu *pMenu, CMenu *pUpdateMenu, LPCTSTR pShortCut)
{
	int n, a;
	CString str;
	CMenu *pSubMenu;

	for ( n = 0 ; n < pMenu->GetMenuItemCount() ; n++ ) {
		if ( (pSubMenu = pMenu->GetSubMenu(n)) == NULL )
			continue;

		if ( pSubMenu->GetSafeHmenu() == pUpdateMenu->GetSafeHmenu() ) {
			pMenu->GetMenuString(n, str, MF_BYPOSITION);
			if ( (a = str.Find(_T('\t'))) >= 0 )
				str.Truncate(a);
			str += _T("\t");
			str += pShortCut;
			pMenu->ModifyMenu(n, MF_BYPOSITION | MF_STRING, 0, str);
			return TRUE;
		}

		if ( UpdateMenuShortCut(pSubMenu, pUpdateMenu, pShortCut) )
			return TRUE;
	}

	return FALSE;
}
CMenu *CMenuLoad::GetItemSubMenu(UINT nId, CMenu *pMenu)
{
	int n;
	UINT item;
	CMenu *pSubMenu;

	for ( n = 0 ; n < pMenu->GetMenuItemCount() ; n++ ) {
		if ( (item = pMenu->GetMenuItemID(n)) == nId )
			return pMenu;

		if ( item == (UINT)(-1) && (pSubMenu = pMenu->GetSubMenu(n)) != NULL && (pSubMenu = GetItemSubMenu(nId, pSubMenu)) != NULL )
			return pSubMenu;
	}

	return NULL;
}
BOOL CMenuLoad::GetPopUpMenu(UINT nId, CMenu &PopUpMenu)
{
	int n;
	CMenu *pMenu;
	CMenuLoad DefMenu;
	CString str, tmp;

	if ( !PopUpMenu.CreatePopupMenu() )
		return FALSE;

	if ( !DefMenu.LoadMenu(nId) )
		return FALSE;

	// Move Top SubMenu
	for ( n = 0 ; n < DefMenu.GetMenuItemCount() ; n++ ) {
		DefMenu.GetMenuString(n, str, MF_BYPOSITION);
		if ( (pMenu = DefMenu.GetSubMenu(n)) != NULL ) {
			PopUpMenu.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)pMenu->GetSafeHmenu(), str);
			DefMenu.RemoveMenu(n, MF_BYPOSITION);
			n--;
		}
	}

	// Create Key History Menu
	if ( (pMenu = GetItemSubMenu(ID_MACRO_HIS1, &PopUpMenu)) != NULL ) {
		if ( pMenu->GetMenuString(ID_MACRO_HIS1, tmp, MF_BYCOMMAND) <= 0 )
			tmp = _T("Key History");
		pMenu->DeleteMenu(ID_MACRO_HIS1, MF_BYCOMMAND);
		for ( n = 0 ; n < 5 ; n++ ) {
			str.Format(_T("&%d %s"), n + 1, tmp);
			pMenu->AppendMenu(MF_STRING, ID_MACRO_HIS1 + n, str);
		}
	}

	// Create Script Menu
	if ( (pMenu = GetItemSubMenu(ID_CHARSCRIPT_END, &PopUpMenu)) != NULL ) {
		tmp.LoadString(IDS_SCRIPTMENUSTR);
		for ( n = 0 ; n < 5 ; n++ ) {
			str.Format(_T("&%d %s"), n + 1, tmp);
			pMenu->InsertMenu(ID_CHARSCRIPT_END, MF_BYCOMMAND, IDM_SCRIPT_MENU1 + n, str);
		}
	}

	// Create Clipboard Menu
	if ( (pMenu = GetItemSubMenu(IDM_CLIPBOARD_HIS1, &PopUpMenu)) != NULL ) {
		if ( pMenu->GetMenuString(IDM_CLIPBOARD_HIS1, tmp, MF_BYCOMMAND) <= 0 )
			tmp = _T("Clipboard History");
		pMenu->DeleteMenu(IDM_CLIPBOARD_HIS1, MF_BYCOMMAND);
		for ( n = 0 ; n < 10 ; n++ ) {
			str.Format(_T("&%d %s"), (n + 1) % 10, tmp);
			pMenu->AppendMenu(MF_STRING, IDM_CLIPBOARD_HIS1 + n, str);
		}
	}

	// Create Window Menu
	if ( (pMenu = GetItemSubMenu(IDM_FIRST_MDICHILD, &PopUpMenu)) != NULL ) {
		if ( pMenu->GetMenuString(IDM_FIRST_MDICHILD, tmp, MF_BYCOMMAND) <= 0 )
			tmp = _T("Window");
		pMenu->DeleteMenu(IDM_FIRST_MDICHILD, MF_BYCOMMAND);
		for ( n = 0 ; n < 10 ; n++ ) {
			str.Format(_T("&%d %s"), (n + 1) % 10, tmp);
			pMenu->AppendMenu(MF_STRING, AFX_IDM_FIRST_MDICHILD + n, str);
		}
	}

	// Set Menu Bitmap
	((CMainFrame *)::AfxGetMainWnd())->SetMenuBitmap(&PopUpMenu);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CStringLoad

CStringLoad::CStringLoad()
{
}

BOOL CStringLoad::LoadString(UINT nID)
{
	return ((CRLoginApp *)AfxGetApp())->LoadResString((LPCTSTR)nID, *this);
}
BOOL CStringLoad::IsDigit(LPCTSTR str)
{
	return (*str >= _T('0') && *str <= _T('9') ? TRUE : FALSE);
}
int CStringLoad::CompareDigit(LPCTSTR dis)
{
	int ns, nd;
	LPCTSTR src = *this;

	while ( *src != _T('\0') ) {
		if ( IsDigit(src) && IsDigit(dis) ) {
			for ( ns = 0 ; IsDigit(src) ; src++ )
				ns = ns * 10 + (*src - _T('0'));

			for ( nd = 0 ; IsDigit(dis) ; dis++ )
				nd = nd * 10 + (*dis - _T('0'));

			if ( ns != nd )
				return (ns > nd ? 1 : (-1));

		} else if ( *src == *dis ) {
			src++;
			dis++;

		} else
			return (*src > *dis ? 1 : (-1));
	}
	return (*dis == _T('\0') ? 0 : 1);
}
#ifdef	DEBUG
int CStringLoad::Compare(LPCTSTR dis)
{
	ASSERT(FALSE);
	return CString::Compare(dis);
}
#endif

//////////////////////////////////////////////////////////////////////
// CStrNode

CStrNode::CStrNode()
{
	m_Type = 0;
	m_Next = NULL;
	m_List = NULL;
	m_Stack = NULL;
}
CStrNode::CStrNode(int type, LPCTSTR str)
{
	m_Type = type;
	if ( str != NULL )
		m_Str = str;
	m_Next = NULL;
	m_List = NULL;
	m_Stack = NULL;
}
CStrNode::~CStrNode()
{
	if ( m_Next != NULL )
		delete m_Next;

	if ( m_List != NULL )
		delete m_List;
}
void CStrNode::AddNext(CStrNode *np)
{
	CStrNode *tp;

	if ( m_Next == NULL )
		m_Next = np;
	else {
		for ( tp = m_Next ; tp->m_Next != NULL ; )
			tp = tp->m_Next;
		tp->m_Next = np;
	}
}
void CStrNode::AddList(CStrNode *np)
{
	CStrNode *tp;

	if ( m_List == NULL )
		m_List = np;
	else {
		for ( tp = m_List ; tp->m_List != NULL ; )
			tp = tp->m_List;
		tp->m_List = np;
	}
}

//////////////////////////////////////////////////////////////////////
// CParaIndex

CParaIndex::CParaIndex()
{
	m_Str.Empty();
	m_Data = 0;
}
CParaIndex::CParaIndex(DWORD data)
{
	m_Str.Empty();
	m_Data = data;
}
CParaIndex::~CParaIndex()
{
}
const CParaIndex & CParaIndex::operator = (CParaIndex &data)
{
	m_Data = data.m_Data;
	m_Str  = data.m_Str;
	m_Array.SetSize(data.m_Array.GetSize());
	for ( int n = 0 ; n < data.m_Array.GetSize() ; n++ )
		m_Array[n] = data.m_Array[n];
	return *this;
}
BOOL CParaIndex::AddOpt(BYTE c, BOOL bAdd)
{
	int n = (int)GetSize() - 1;

	if ( n >= 0 ) {
		if ( m_Array[n].AddOpt(c, bAdd) && bAdd )
			Add(PARA_NOT);
		return FALSE;
	} else {
		m_Str += (CHAR)c;
		m_Data = PARA_OPT;
		return TRUE;
	}
}

//////////////////////////////////////////////////////////////////////
// CBmpFile

CBmpFile::CBmpFile()
{
	m_Width = m_Height = 0;
	m_BkColor = 0;
	m_Alpha = 255;
	m_pTextBitMap = new CTextBitMap;
	m_bkIndex = (-1);
}

CBmpFile::~CBmpFile()
{
	if ( m_pTextBitMap != NULL )
		delete m_pTextBitMap;
}

BOOL CBmpFile::LoadFile(LPCTSTR filename)
{
	CFile file;
	ULONGLONG FileSize;
	HGLOBAL hGLB = NULL;
	LPBYTE pBuf = NULL;
    LPSTREAM pStm = NULL;
	BOOL ret = FALSE;

	if ( m_FileName.Compare(filename) == 0 )
		return FALSE;

	m_FileName = filename;

	if ( (HBITMAP)m_Image != NULL )
		m_Image.Destroy();

	if ( m_Bitmap.m_hObject != NULL )
		m_Bitmap.DeleteObject();

	if ( m_FileName.IsEmpty() )
		return FALSE;

	if( !file.Open(filename, CFile::modeRead) )
		goto ERROF;

	if ( (FileSize = file.GetLength()) > (512 * 1024 * 1024) )
		goto ERROF;

	if ( (hGLB = ::GlobalAlloc(GMEM_MOVEABLE, (DWORD)FileSize)) == NULL )
		goto ERROF;

	if ( (pBuf = (LPBYTE)::GlobalLock(hGLB)) == NULL )
		goto ERROF;

	if ( file.Read(pBuf, (DWORD)FileSize) != (DWORD)FileSize )
		goto ERROF;

	m_bkIndex = GifTrnsIndex((LPBYTE)pBuf, (int)FileSize);

	::GlobalUnlock(hGLB);
	pBuf = NULL;

	if ( ::CreateStreamOnHGlobal(hGLB, TRUE, &pStm) != S_OK )
		goto ERROF;

	if ( FAILED(m_Image.Load(pStm)) )
		goto ERROF;

	if ( m_Image.GetBPP() == 32 ) {
		for ( int y = 0 ; y < m_Image.GetHeight() ; y++ ) {
			for ( int x = 0 ;  x < m_Image.GetWidth() ; x++ ) {
				LPBYTE p = (BYTE *)m_Image.GetPixelAddress(x, y);
				if ( p[3] == 0x00 )
					p[0] = p[1] = p[2] = 0x00;
				else if ( p[3] < 255 ) {
					p[0] = p[0] * p[3] / 255;
					p[1] = p[1] * p[3] / 255;
					p[2] = p[2] * p[3] / 255;
				}
			}
		}
	}

	if ( m_Bitmap.m_hObject != NULL )
		m_Bitmap.DeleteObject();

	ret = TRUE;

ERROF:
	if ( pStm != NULL )
		pStm->Release();

	if ( pBuf != NULL )
		::GlobalUnlock(hGLB);

	if ( hGLB != NULL )
		::GlobalFree(hGLB);

	return ret;
}

CBitmap *CBmpFile::GetBitmap(CDC *pDC, int width, int height, COLORREF bkcolor, int Alpha)
{
	int x, y, cx, cy;
	CDC MemDC;
	CBitmap *pOldMemMap = NULL;
	CRect rect;
	CPoint po;

	if ( (HBITMAP)m_Image == NULL )
		return NULL;

	if ( m_Bitmap.m_hObject != NULL ) {
		if ( m_Width == width && m_Height == height && m_BkColor == bkcolor && m_Alpha == Alpha )
			return (&m_Bitmap);
		m_Bitmap.DeleteObject();
	}

	MemDC.CreateCompatibleDC(pDC);
	MemDC.SetMapMode(pDC->GetMapMode());

	m_Bitmap.CreateCompatibleBitmap(pDC, width, height);
	pOldMemMap = (CBitmap *)MemDC.SelectObject(&m_Bitmap);

	po = MemDC.GetBrushOrg();
	MemDC.SetStretchBltMode(HALFTONE);
	MemDC.FillSolidRect(0, 0, width, height, bkcolor);

	CSize size(m_Image.GetWidth(), m_Image.GetHeight());
	CSize offset(0, 0);

	cx = width  * 100 / size.cx;
	cy = height * 100 / size.cy;

	if ( cx < cy ) {
		cx = height * size.cx / size.cy;
		cy = height;
		offset.cx = ((cx - width) / 2) * size.cx / cx;
	} else {
		cx = width;
		cy = width  * size.cy / size.cx;
		offset.cy = ((cy - height) / 2) * size.cy / cy;
	}

	if ( m_bkIndex != (-1) && (m_Image.GetBPP() == 4 || m_Image.GetBPP() == 8) ) {
		RGBQUAD coltab[1];
		m_Image.GetColorTable(m_bkIndex, 1, coltab);
		coltab[0].rgbRed   = GetRValue(bkcolor);
		coltab[0].rgbGreen = GetGValue(bkcolor);
		coltab[0].rgbBlue  = GetBValue(bkcolor);
		coltab[0].rgbReserved = 0;
		m_Image.SetColorTable(m_bkIndex, 1, coltab);
	}

	m_Image.Draw(MemDC.GetSafeHdc(), 0, 0, cx, cy, offset.cx, offset.cy, size.cx, size.cy);

	if ( Alpha < 254 || 257 < Alpha ) {

		DWORD len;
		BYTE *lpBuf, *p;
		BITMAP mapinfo;
		BYTE ctab[256];
		COLORREF col;

		if ( Alpha < 255 ) {
			for ( x = 0 ; x < 256 ; x++ )
				ctab[x] = (x * Alpha / 255); 
		} else if ( Alpha >= 256 ) {
			for ( x = 0 ; x < 256 ; x++ )
				ctab[x] = (255 - ((255 - x) * (511 - Alpha) / 255)); 
		}

		m_Bitmap.GetBitmap(&mapinfo);

		switch(mapinfo.bmBitsPixel) {
		case 32:
			len = mapinfo.bmWidthBytes * mapinfo.bmHeight;
			lpBuf = new BYTE[len];
			m_Bitmap.GetBitmapBits(len, lpBuf);
			for ( y = 0 ; y < height ; y++ ) {
				p = lpBuf + mapinfo.bmWidthBytes * y;
				for ( x = 0 ; x < width ; x++ ) {
					*p = ctab[*p]; p++;
					*p = ctab[*p]; p++;
					*p = ctab[*p]; p++;
					p++;
				}
			}
			m_Bitmap.SetBitmapBits(len, lpBuf);
			delete [] lpBuf;
			break;

		case 24:
			len = mapinfo.bmWidthBytes * mapinfo.bmHeight;
			lpBuf = new BYTE[len];
			m_Bitmap.GetBitmapBits(len, lpBuf);
			for ( y = 0 ; y < height ; y++ ) {
				p = lpBuf + mapinfo.bmWidthBytes * y;
				for ( x = 0 ; x < width ; x++ ) {
					*p = ctab[*p]; p++;
					*p = ctab[*p]; p++;
					*p = ctab[*p]; p++;
				}
			}
			m_Bitmap.SetBitmapBits(len, lpBuf);
			delete [] lpBuf;
			break;

		case 16:
			len = mapinfo.bmWidthBytes * mapinfo.bmHeight;
			lpBuf = new BYTE[len];
			m_Bitmap.GetBitmapBits(len, lpBuf);
			for ( y = 0 ; y < height ; y++ ) {
				p = lpBuf + mapinfo.bmWidthBytes * y;
				for ( x = 0 ; x < width ; x++ ) {
					struct _rgb16 {
						WORD r:5;
						WORD g:6;
						WORD b:5;
					} *bits = (struct _rgb16 *)p;
					bits->r = ctab[bits->r << 3] >> 3;
					bits->g = ctab[bits->g << 2] >> 2;
					bits->b = ctab[bits->b << 3] >> 3;
					p += 2;
				}
			}
			m_Bitmap.SetBitmapBits(len, lpBuf);
			delete [] lpBuf;
			break;

		case 15:
			len = mapinfo.bmWidthBytes * mapinfo.bmHeight;
			lpBuf = new BYTE[len];
			m_Bitmap.GetBitmapBits(len, lpBuf);
			for ( y = 0 ; y < height ; y++ ) {
				p = lpBuf + mapinfo.bmWidthBytes * y;
				for ( x = 0 ; x < width ; x++ ) {
					struct _rgb15 {
						WORD r:5;
						WORD g:5;
						WORD b:5;
					} *bits = (struct _rgb15 *)p;
					bits->r = ctab[bits->r << 3] >> 3;
					bits->g = ctab[bits->g << 3] >> 3;
					bits->b = ctab[bits->b << 3] >> 3;
					p += 2;
				}
			}
			m_Bitmap.SetBitmapBits(len, lpBuf);
			delete [] lpBuf;
			break;

		default:
			for ( y = 0 ; y < height ; y++ ) {
				for ( x = 0 ; x < width ; x++ ) {
					col = MemDC.GetPixel(x, y);
					MemDC.SetPixelV(x, y, RGB(ctab[GetRValue(col)], ctab[GetGValue(col)], ctab[GetBValue(col)]));
				}
			}
			break;
		}
	}

	m_Width   = width;
	m_Height  = height;
	m_BkColor = bkcolor;
	m_Alpha   = Alpha;

	m_pTextBitMap->Init();
	m_Title.Empty();

	MemDC.SelectObject(pOldMemMap);
	MemDC.SetBrushOrg(po);

	return (&m_Bitmap);
}
CBitmap *CBmpFile::GetTextBitmap(CDC *pDC, int width, int height, COLORREF bkcolor, class CTextBitMap *pTextBitMap, LPCTSTR title, int Alpha)
{
	int n;
	int cr, cg, cb;
	CDC MemDC;
	CBitmap *pOldMemMap = NULL;
	CRect rect;
	CFont Font, *pOldFont;
	BOOL bEraBack = FALSE;

	if ( m_Bitmap.m_hObject != NULL && m_Width == width && m_Height == height && m_BkColor == bkcolor && m_Alpha == Alpha && (*m_pTextBitMap) == (*pTextBitMap) && m_Title.Compare(title) == 0 )
		return (&m_Bitmap);

	MemDC.CreateCompatibleDC(pDC);

	if ( GetBitmap(pDC, width, height, bkcolor, Alpha) == NULL ) {
		if ( m_Bitmap.m_hObject != NULL )
			m_Bitmap.DeleteObject();
		m_Bitmap.CreateCompatibleBitmap(pDC, width, height);
		bEraBack = TRUE;
	}

	pOldMemMap = (CBitmap *)MemDC.SelectObject(&m_Bitmap);

	Font.CreateFontIndirect(&(pTextBitMap->m_LogFont));
	pOldFont = (CFont *)MemDC.SelectObject(&Font);

	if ( bEraBack )
		MemDC.FillSolidRect(0, 0, width, height, bkcolor);

	cr = GetRValue(pTextBitMap->m_TextColor);
	cg = GetGValue(pTextBitMap->m_TextColor);
	cb = GetBValue(pTextBitMap->m_TextColor);

	if ( Alpha < 255 ) {
		cr = (cr * Alpha / 255); 
		cg = (cg * Alpha / 255); 
		cb = (cb * Alpha / 255); 
	} else if ( Alpha >= 256 ) {
		cr = (255 - ((255 - cr) * (511 - Alpha) / 255)); 
		cg = (255 - ((255 - cg) * (511 - Alpha) / 255)); 
		cb = (255 - ((255 - cb) * (511 - Alpha) / 255)); 
	}

	MemDC.SetTextColor(RGB(cr, cg, cb));
	MemDC.SetBkMode(TRANSPARENT);

	rect.left   = 0;
	rect.top    = 0;
	rect.right  = width;
	rect.bottom = height;

	switch(pTextBitMap->m_HeightAlign) {
	case DT_VCENTER:
		n = MemDC.DrawText(title, (int)_tcslen(title), rect, pTextBitMap->m_WidthAlign | DT_CALCRECT | DT_HIDEPREFIX);
		rect.left   = 0;
		rect.top    = (height - n) / 2;
		rect.right  = width;
		rect.bottom = height;
		break;
	case DT_BOTTOM:
		n = MemDC.DrawText(title, (int)_tcslen(title), rect, pTextBitMap->m_WidthAlign | DT_CALCRECT | DT_HIDEPREFIX);
		rect.left   = 0;
		rect.top    = height - n;
		rect.right  = width;
		rect.bottom = height;
		break;
	}

	MemDC.DrawText(title, (int)_tcslen(title), rect, pTextBitMap->m_WidthAlign | DT_HIDEPREFIX);

	m_Width   = width;
	m_Height  = height;
	m_BkColor = bkcolor;
	m_Alpha   = Alpha;

	(*m_pTextBitMap) = (*pTextBitMap);
	m_Title = title;

	MemDC.SelectObject(pOldMemMap);
	MemDC.SelectObject(pOldFont);

	return (&m_Bitmap);
}
int CBmpFile::GifTrnsIndex(LPBYTE lpBuf, int len)
{
#pragma pack(push, 1)
	struct _GifHead {
		BYTE	tag[6];	// GIF89a
		WORD	width;
		WORD	height;
			BYTE	SGCT:3;
			BYTE	SF:1;
			BYTE	CR:3;
			BYTE	GCTF:1;
		BYTE	bkindex;
		BYTE	Aspect;
	} *pGifHead;
	struct _GifCtrl {
		BYTE	id;		// 0x21
		BYTE	label;	// 0xf9
		BYTE	size;	// 0x04
			BYTE	trflag:1;
			BYTE	user:1;
			BYTE	motd:3;
			BYTE	resv:3;
		WORD	delay;
		BYTE	tridx;
		BYTE	term;	// 0x00
	} *pGifCtrl;
#pragma pack(pop)

	int pos = 0;

	pGifHead = (struct _GifHead *)(lpBuf + pos);
	if ( (pos += sizeof(struct _GifHead)) > len )
		return (-1);

	if ( memcmp(pGifHead->tag, "GIF89a", 6) != 0 )
		return (-1);

	if ( pGifHead->GCTF != 0 ) {
		if ( (pos += ((1 << (pGifHead->SGCT + 1)) * 3)) > len )
			return (-1);
	}

	while ( (pos + 3) < len ) {
		if ( lpBuf[pos] == 0x2c )	// Image Block
			return (-1);
		else if ( lpBuf[pos] != 0x21 )
			return (-1);

		pGifCtrl = (struct _GifCtrl *)(lpBuf + pos);

		if ( pGifCtrl->label == 0xf9 && pGifCtrl->size == 4 ) {
			if ( pGifCtrl->trflag == 0 )
				return (-1);
			return pGifCtrl->tridx;
		}

		pos += 3;	// id + label + size
		pos += pGifCtrl->size;

		// 0x01=Plain Text Extension or 0xFF=Application Extension
		if ( pGifCtrl->label == 0x01 || pGifCtrl->label == 0xFF )
			pos += lpBuf[pos - 1];

		pos += 1;	// term
	}

	return (-1);
}

//////////////////////////////////////////////////////////////////////
// CFontChacheNode

CFontChacheNode::CFontChacheNode()
{
	m_pFont = NULL;
	m_pNext = NULL;

	memset(&(m_LogFont), 0, sizeof(LOGFONT));
	m_LogFont.lfWidth          = 0;
	m_LogFont.lfHeight         = 0;
	m_LogFont.lfWeight         = FW_DONTCARE;
	m_LogFont.lfCharSet        = ANSI_CHARSET;
	m_LogFont.lfOutPrecision   = OUT_CHARACTER_PRECIS;
	m_LogFont.lfClipPrecision  = CLIP_CHARACTER_PRECIS;
	m_LogFont.lfQuality        = DEFAULT_QUALITY; // NONANTIALIASED_QUALITY, ANTIALIASED_QUALITY, CLEARTYPE_QUALITY
	m_LogFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

	m_Width   = 0;
	m_Height  = 0;
	m_CharSet = (-1);
	m_Style   = 0;
	m_Quality = 0;
	m_bFixed  = FALSE;
}
CFontChacheNode::~CFontChacheNode()
{
	if ( m_pFont != NULL )
		delete m_pFont;
}
CFont *CFontChacheNode::Open(LPCTSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality)
{
    _tcsncpy(m_LogFont.lfFaceName, pFontName, sizeof(m_LogFont.lfFaceName) / sizeof(TCHAR));
	m_LogFont.lfWidth       = Width;
	m_LogFont.lfHeight		= Height;
	m_LogFont.lfCharSet		= CharSet;
	m_LogFont.lfWeight      = ((Style & 1) != 0 ? FW_BOLD : FW_DONTCARE);
	m_LogFont.lfItalic      = ((Style & 2) != 0 ? TRUE : FALSE);
	m_LogFont.lfUnderline	= ((Style & 4) != 0 ? TRUE : FALSE);
	m_LogFont.lfQuality     = Quality;

	m_Width   = Width;
	m_Height  = Height;
	m_CharSet = CharSet;
	m_Style   = Style;
	m_Quality = Quality;
	m_bFixed  = FALSE;

	if ( m_pFont != NULL )
		delete m_pFont;
	m_pFont = new CFont;

	if ( !m_pFont->CreateFontIndirect(&m_LogFont) )
		m_pFont->Attach((HFONT)GetStockObject(SYSTEM_FONT));

	else {
		int n;
		CDC dc;
		CSize sz;
		CFont *pOld;

		dc.CreateCompatibleDC(NULL);
		pOld = dc.SelectObject(m_pFont);

		// Fixed Font Check 'W' == 'i'
		sz = dc.GetTextExtent(_T("W"), 1);
		if ( (n = sz.cx) <= 0 ) n = 1;
		sz = dc.GetTextExtent(_T("i"), 1);
		if ( (n = sz.cx * 100 / n) >= 80 )
			m_bFixed = TRUE;

		// AvgWidth Check Width > 'A'
		sz = dc.GetTextExtent(_T("ABC012abc"), 9);

		//		TEXTMETRIC metric;
		//		dc.GetTextMetrics(&metric);
		//
		//		---- ---+---+
		//				|	| tmInternalLeading	
		//		----	| --+
		//		 **		|
		//		*  *	| tmAsent
		//		 ***	|
		//		   * ---+
		//		 ** 	| tmDescent
		//		---- ---+
		//
		//						kozuka	source	meiryo	mei_ui	ms_got	ms_pgot
		//	         tmHeight	16		16		16		15		16		16
		//	          tmAsent	13		12		11		12		14		14
		//	        tmDescent	3		4		5		3		2		2
		//	tmInternalLeading	6		5		5		3		0		0

		dc.SelectObject(pOld);

		// Resize Width ?
		if ( sz.cx > 0 && (sz.cx * 100 / (m_Width * 9)) < 80 ) {
			m_LogFont.lfWidth  = m_LogFont.lfWidth * (m_Width * 9) / sz.cx;
			m_pFont->DeleteObject();
			if ( !m_pFont->CreateFontIndirect(&m_LogFont) )
				m_pFont->Attach((HFONT)GetStockObject(SYSTEM_FONT));
		}
	}

	return m_pFont;
}

//////////////////////////////////////////////////////////////////////
// CFontChache

CFontChache::CFontChache()
{
	int n, hs;

	for ( hs = 0 ; hs < 4 ; hs++ )
		m_pTop[hs] = NULL;
	for ( n = 0 ; n < FONTCACHEMAX ; n++ ) {
		hs = n % 4;
		m_Data[n].m_pNext = m_pTop[hs];
		m_pTop[hs] = &(m_Data[n]);
	}
}
CFontChacheNode *CFontChache::GetFont(LPCTSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality, int Hash)
{
	CFontChacheNode *pNext, *pBack;

	Hash = (Hash + Width + Height + Style) & 3;
	pNext = pBack = m_pTop[Hash];
	for ( ; ; ) {
		if ( pNext->m_pFont   != NULL &&
			 pNext->m_Width   == Width   &&
			 pNext->m_Height  == Height  &&
			 pNext->m_CharSet == CharSet &&
			 pNext->m_Style   == Style   &&
			 pNext->m_Quality == Quality &&
			 _tcscmp(pNext->m_LogFont.lfFaceName, pFontName) == 0 ) {
			if ( pNext != pBack ) {
				pBack->m_pNext = pNext->m_pNext;
				pNext->m_pNext = m_pTop[Hash];
				m_pTop[Hash] = pNext;
			}
			return pNext;
		}
		if ( pNext->m_pNext == NULL )
			break;
		pBack = pNext;
		pNext = pNext->m_pNext;
	}

	//TRACE(_T("CacheMiss %s(%d,%d,%d,%d)\n"), pFontName, CharSet, Width, Height, Hash);

	if ( pNext->Open(pFontName, Width, Height, CharSet, Style, Quality) == NULL )
		return NULL;

	pBack->m_pNext = pNext->m_pNext;
	pNext->m_pNext = m_pTop[Hash];
	m_pTop[Hash] = pNext;
	return pNext;
}

//////////////////////////////////////////////////////////////////////
// CMutexLock

CMutexLock::CMutexLock(LPCTSTR lpszName)
{
	m_pMutex = new CMutex(TRUE, lpszName);
	m_pLock  = new CSingleLock(m_pMutex, FALSE);
	m_pLock->Lock();
}
CMutexLock::~CMutexLock()
{
	m_pLock->Unlock();
	delete m_pLock;
	delete m_pMutex;
}

//////////////////////////////////////////////////////////////////////
// COptObject

COptObject::COptObject()
{
	m_pSection = _T("OptObject");
	m_MinSize = 1;
}
void COptObject::Init()
{
}
void COptObject::Serialize(int mode)
{
	CStringArrayExt stra;

	if ( mode ) {	// Write
		SetArray(stra);
		((CRLoginApp *)AfxGetApp())->WriteProfileArray(m_pSection, stra);
	} else {		// Read
		((CRLoginApp *)AfxGetApp())->GetProfileArray(m_pSection, stra);
		if ( stra.GetSize() < m_MinSize )
			Init();
		else
			GetArray(stra);
	}
}
void COptObject::Serialize(int mode, CBuffer &buf)
{
	CStringArrayExt stra;

	if ( mode ) {	// Write
		SetArray(stra);
		stra.SetBuffer(buf);
	} else {		// Read
		stra.GetBuffer(buf);
		if ( stra.GetSize() < m_MinSize )
			Serialize(FALSE);
		else
			GetArray(stra);
	}
}
void COptObject::Serialize(CArchive &ar)
{
	CStringArrayExt stra;

	if ( ar.IsStoring() ) {		// TODO: この位置に保存用のコードを追加してください。
		SetArray(stra);
		stra.Serialize(ar);
	} else {					// TODO: この位置に読み込み用のコードを追加してください。
		stra.Serialize(ar);
		if ( stra.GetSize() < m_MinSize )
			Serialize(FALSE);
		else
			GetArray(stra);
	}
}

//////////////////////////////////////////////////////////////////////
// CStrScriptNode

CStrScriptNode::CStrScriptNode()
{
	m_Left = m_Right = NULL;
}
CStrScriptNode::~CStrScriptNode()
{
	if ( m_Left != NULL )
		delete m_Left;
	if ( m_Right != NULL )
		delete m_Right;
}

//////////////////////////////////////////////////////////////////////
// CStrScript

CStrScript::CStrScript()
{
	m_Node = m_Exec = NULL;
	m_MakeChat = m_MakeFlag = FALSE;
}
CStrScript::~CStrScript()
{
	if ( m_Node != NULL )
		delete m_Node;
}
void CStrScript::Init()
{
	if ( m_Node != NULL )
		delete m_Node;

	m_Node = m_Exec = NULL;
	m_MakeChat = m_MakeFlag = FALSE;
}
CStrScriptNode *CStrScript::CopyNode(CStrScriptNode *np)
{
	CStrScriptNode *tp;

	if ( np == NULL )
		return NULL;

	tp = new CStrScriptNode;
	tp->m_RecvStr = np->m_RecvStr;
	tp->m_SendStr = np->m_SendStr;
	tp->m_Reg.Compile(tp->m_RecvStr);
	tp->m_Left  = CopyNode(np->m_Left);
	tp->m_Right = CopyNode(np->m_Right);

	return tp;
}
const CStrScript & CStrScript::operator = (CStrScript &data)
{
	if ( m_Node != NULL )
		delete m_Node;

	m_Node = CopyNode(data.m_Node);
	m_MakeChat = data.m_MakeChat;
	m_MakeFlag = FALSE;
	m_Exec = NULL;

	return *this;
}

void CStrScript::SetNode(CStrScriptNode *np, CBuffer &buf)
{
	if ( np == NULL )
		buf.Put8Bit(0);
	else {
		buf.Put8Bit(1);
		buf.PutStr(TstrToMbs(np->m_RecvStr));
		buf.PutStr(TstrToMbs(np->m_SendStr));
		SetNode(np->m_Left,  buf);
		SetNode(np->m_Right, buf);
	}
}
CStrScriptNode *CStrScript::GetNode(CBuffer &buf)
{
	CStringA mbs;
	CStrScriptNode *np;

	if ( buf.Get8Bit() == 0 )
		return NULL;

	np = new CStrScriptNode;
	buf.GetStr(mbs); np->m_RecvStr = mbs;
	buf.GetStr(mbs); np->m_SendStr = mbs;
	np->m_Reg.Compile(np->m_RecvStr);
	np->m_Left  = GetNode(buf);
	np->m_Right = GetNode(buf);

	return np;
}
void CStrScript::SetBuffer(CBuffer &buf)
{
	buf.Clear();
	buf.Put8Bit(m_MakeChat ? 1 : 0);
	SetNode(m_Node, buf);
}
int CStrScript::GetBuffer(CBuffer &buf)
{
	m_MakeChat = (buf.Get8Bit() != 0 ? TRUE : FALSE);

	if ( m_Node != NULL )
		delete m_Node;

	m_Node = GetNode(buf);

	m_MakeFlag = FALSE;
	m_Exec = NULL;

	return (m_Node != NULL ? TRUE : FALSE);
}
LPCTSTR CStrScript::QuoteStr(CString &tmp, LPCTSTR str)
{
#ifdef	_UNICODE
	tmp = _T("\"");
	while ( *str != _T('\0') ) {
		if ( *str == _T('"') ) {
			str++;
			tmp += _T("\\042");
		} else
			tmp += *(str++);
	}
	tmp += _T("\"");
	return tmp;
#else
	tmp = "\"";
	while ( *str != '\0' ) {
		if ( issjis1(str[0]) && issjis2(str[1]) ) {
			tmp += *(str++);
			tmp += *(str++);
		} else if ( *str == '"' ) {
			str++;
			tmp += "\\042";
		} else
			tmp += *(str++);
	}
	tmp += "\"";
	return tmp;
#endif
}
void CStrScript::SetNodeStr(CStrScriptNode *np, CString &str, int nst)
{
	int n;
	CString tmp;
	LPCTSTR cmd = _T("if ");

	if ( np == NULL )
		return;

	while ( np != NULL ) {
		for ( n = 0 ; n < nst ; n++ )
			str += _T("  ");

		str += cmd;
		str += QuoteStr(tmp, np->m_RecvStr);

		if ( !np->m_SendStr.IsEmpty() ) {
			str += _T(" then ");
			str += QuoteStr(tmp, np->m_SendStr);
		}
		str += _T("\r\n");

		SetNodeStr(np->m_Right, str, nst + 1);

		np = np->m_Left;
		cmd = _T("or ");
	}

	for ( n = 0 ; n < nst ; n++ )
		str += _T("  ");
	str += _T("fi\r\n");
}
int CStrScript::GetLex(LPCTSTR &str)
{
	m_LexTmp.Empty();

	while ( *str != _T('\0') && *str <= _T(' ') )
		str++;

	if ( *str == _T('\0') ) {
		return EOF;

	} else if ( *str == _T('"') ) {
		str++;
		while ( *str != _T('\0') && *str != _T('"') ) {
#ifndef	_UNICODE
			if ( issjis1(str[0]) && issjis2(str[1]) )
				m_LexTmp += *(str++);
#endif
			m_LexTmp += *(str++);
		}
		if ( *str != _T('\0') )
			str++;
		return 0;
	}

	while ( *str != _T('\0') && *str > _T(' ') ) {
#ifndef	_UNICODE
		if ( issjis1(str[0]) && issjis2(str[1]) )
			m_LexTmp += *(str++);
#endif
		m_LexTmp += *(str++);
	}

	if ( m_LexTmp.CollateNoCase(_T("if")) == 0 )
		return 1;
	else if ( m_LexTmp.CollateNoCase(_T("then")) == 0 )
		return 2;
	else if ( m_LexTmp.CollateNoCase(_T("or")) == 0 )
		return 3;
	else if ( m_LexTmp.CollateNoCase(_T("fi")) == 0 )
		return 4;
	else
		return 0;
}
CStrScriptNode *CStrScript::GetNodeStr(int &lex, LPCTSTR &str)
{
	int cmd = lex;
	CStrScriptNode *np;

	if ( lex != 1 && lex != 3 )	// if || or
		return NULL;

	if ( (lex = GetLex(str)) != 0 )	// str
		return NULL;

	np = new CStrScriptNode;
	np->m_RecvStr = m_LexTmp;
	np->m_SendStr.Empty();
	np->m_Reg.Compile(np->m_RecvStr);

	if ( (lex = GetLex(str)) == 2 ) {	// then
		if ( (lex = GetLex(str)) == 0 ) {	// str
			np->m_SendStr = m_LexTmp;
			lex = GetLex(str);
		}
	}

	if ( lex == 1 )	// if
		np->m_Right = GetNodeStr(lex, str);

	while ( lex == 3 )	// or
		np->m_Left  = GetNodeStr(lex, str);

	if ( cmd == 1 && lex == 4 )	// if ... fi
		lex = GetLex(str);

	return np;
}
void CStrScript::SetString(CString &str)
{
	str.Empty();
	SetNodeStr(m_Node, str, 0);
}
void CStrScript::GetString(LPCTSTR str)
{
	int lex;

	m_MakeChat = FALSE;

	if ( m_Node != NULL )
		delete m_Node;

	lex = GetLex(str);
	m_Node = GetNodeStr(lex, str);

	m_MakeFlag = FALSE;
	m_Exec = NULL;
}
void CStrScript::EscapeStr(LPCWSTR str, CString &buf, BOOL reg)
{
	int n;
	CString tmp;

	for ( buf.Empty() ; *str != L'\0' ; str++ ) {
		if ( *str <= L' ' || *str == L'\\' || *str == 0x7F ) {
			switch(*str) {
			case 0x07: buf += "\\a"; break;
			case 0x08: buf += "\\b"; break;
			case 0x09: buf += "\\t"; break;
			case 0x0A: buf += "\\n"; break;
			case 0x0B: buf += "\\v"; break;
			case 0x0C: buf += "\\f"; break;
			case 0x0D: buf += "\\r"; break;
			case L' ':  buf += " ";   break;
			case L'\\': buf += "\\\\"; break;
			default:
				tmp.Format(_T("\\%03o"), (CHAR)*str);
				buf += tmp;
				break;
			}
			if ( reg ) {
				for ( n = 1 ; str[0] == str[1] ; n++ )
					str++;
				if ( n > 1 )
					buf += _T('+');
			}
		} else if ( reg ) {
			switch(*str) {
			case L'0': case L'1': case L'2': case L'3': case L'4':
			case L'5': case L'6': case L'7': case L'8': case L'9':
				for ( n = 1 ; str[1] >= L'0' && str[1] <= L'9' ; n++ )
					str++;
				buf += (n > 1 ? _T("[0-9]+") : _T("[0-9]"));
				break;
			case L'[': case L'^': case L'$': case L'|':
			case L'+': case L'*': case L'.': case L'?':
			case L'{': case L'(':
				buf += _T("\\");
			default:
				buf += *str;
				for ( n = 1 ; str[0] == str[n] ; n++ );
				if ( n > 2 ) {
					buf += _T('+');
					str += (n - 1);
				}
				break;
			}
		} else {
			buf += *str;
		}
	}
}
void CStrScript::AddNode(LPCWSTR recv, LPCWSTR send)
{
	CStrScriptNode *np, *tp;

	if ( *recv == L'\0' && send == L'\0' )
		return;

	np = new CStrScriptNode;
	EscapeStr(recv, np->m_RecvStr, TRUE);
	EscapeStr(send, np->m_SendStr);
	np->m_Reg.Compile(np->m_RecvStr);

	if ( m_Node == NULL )
		m_Node = np;
	else {
		for ( tp = m_Node ; tp->m_Right != NULL ; )
			tp = tp->m_Right;
		tp->m_Right = np;
	}
}

int CStrScript::Status()
{
	if ( m_MakeFlag )
		return SCPSTAT_MAKE;
	else if ( m_Exec != NULL )
		return SCPSTAT_EXEC;
	else
		return SCPSTAT_NON;
}
void CStrScript::ExecNode(CStrScriptNode *np)
{
	for ( m_Exec = np ; np != NULL ; np = np->m_Left )
		np->m_Reg.MatchCharInit();
}
void CStrScript::ExecStop()
{
	m_MakeFlag = FALSE;
	m_Exec = NULL;
	if ( m_StatDlg.m_hWnd != NULL )
		m_StatDlg.DestroyWindow();
}
void CStrScript::ExecInit() 
{
	if ( m_MakeChat ) {
		m_Str.Empty();
		m_Line[0].Empty();
		m_Line[1].Empty();
		if ( m_Node != NULL )
			delete m_Node;
		m_Node = NULL;
		m_MakeChat = FALSE;
		m_MakeFlag = TRUE;
		m_Exec = FALSE;
	} else {
		m_MakeFlag = FALSE;
		ExecNode(m_Node);
	}

	if ( m_MakeFlag || m_Exec != NULL ) {
		if ( m_StatDlg.m_hWnd == NULL ) {
			m_StatDlg.Create(IDD_CHATSTAT, AfxGetMainWnd());
			m_StatDlg.ShowWindow(SW_SHOW);
		}
		m_StatDlg.m_Title.SetWindowText(m_MakeFlag ? _T("Makeing") : _T("Execute"));
		m_StatDlg.SetStaus(_T(""));
		AfxGetMainWnd()->SetFocus();
	}
}
LPCWSTR CStrScript::ExecChar(DWORD ch)
{
	CString tmp;
	CStrScriptNode *np;

	if ( m_MakeFlag ) {
		if ( ch != 0 )
			m_Line[0] += (WCHAR)ch;
		EscapeStr(m_Line[0], tmp);
		if ( m_StatDlg.m_hWnd != NULL )
			m_StatDlg.SetStaus(tmp);
		if ( ch == '\n' ) {
			m_Line[1] = m_Line[0];
			m_Line[0].Empty();
		}
	} else if ( m_Exec != NULL ) {
		for ( np = m_Exec ; np != NULL ; np = np->m_Left ) {
			if ( !tmp.IsEmpty() )
				tmp += _T(" or ");
			tmp += np->m_RecvStr;
			if ( np->m_Reg.MatchChar(CTextRam::UCS2toUCS4(ch), 0, &m_Res) ) { // && (m_Res.m_Status == REG_MATCH || m_Res.m_Status == REG_MATCHOVER) ) {
				ExecNode(np->m_Right);
				np->m_Reg.ConvertRes(TstrToUni(np->m_SendStr), m_Str, &m_Res);
				if ( m_StatDlg.m_hWnd != NULL )
					m_StatDlg.SetStaus(_T(""));
				return m_Str;
			}
		}
		if ( m_StatDlg.m_hWnd != NULL )
			m_StatDlg.SetStaus(tmp);
	}
	return NULL;
}
void CStrScript::SendStr(LPCWSTR str, int len, CServerEntry *ep)
{
	if ( !m_MakeFlag )
		return;

	for ( int n = 0 ; n < len ; n++, str++ ) {
		if ( m_Str.IsEmpty() ) {
			m_Line[2] = m_Line[0];
			if ( m_Line[2].IsEmpty() )
				m_Line[2] = m_Line[1];
		}
		if ( *str == L'\r' ) {
			if ( ep != NULL ) {
				if ( ep->m_EntryName.Compare(CString(m_Str)) == 0 )
					m_Str = L"%E";
				else if ( ep->m_UserName.Compare(CString(m_Str)) == 0 )
					m_Str = L"%U";
				else if ( ep->m_PassName.Compare(CString(m_Str)) == 0 )
					m_Str = L"%P";
				else if ( ep->m_TermName.Compare(CString(m_Str)) == 0 )
					m_Str = L"%T";
			}
			m_Str += *str;
			AddNode(m_Line[2], m_Str);
			m_Str.Empty();
		} else
			m_Str += *str;
	}
}

void CStrScript::SetTreeNode(CStrScriptNode *np, CTreeCtrl &tree, HTREEITEM own)
{
	HTREEITEM hti;
	CString tmp;

	if ( np == NULL )
		return;

	tmp.Format(_T("%s/%s"), np->m_RecvStr, np->m_SendStr);

	if ( (hti = tree.InsertItem(tmp, own, TVI_LAST)) == NULL )
		return;

	tree.SetItemData(hti, (DWORD_PTR)np);
	SetTreeNode(np->m_Left,  tree, own);
	SetTreeNode(np->m_Right, tree, hti);
	np->m_Left = np->m_Right = NULL;
	tree.Expand(hti, TVE_EXPAND);
}
void CStrScript::SetTreeCtrl(CTreeCtrl &tree)
{
	tree.DeleteAllItems();
	SetTreeNode(m_Node, tree, TVI_ROOT);
	m_Node = NULL;
}
CStrScriptNode *CStrScript::GetTreeNode(CTreeCtrl &tree, HTREEITEM hti)
{
	CStrScriptNode *np;

	if ( hti == NULL )
		return NULL;

	np = (CStrScriptNode *)tree.GetItemData(hti);
	tree.SetItemData(hti, (DWORD_PTR)NULL);
	np->m_Left  = GetTreeNode(tree, tree.GetNextItem(hti, TVGN_NEXT));
	np->m_Right = GetTreeNode(tree, tree.GetNextItem(hti, TVGN_CHILD));
	return np;
}
void CStrScript::GetTreeCtrl(CTreeCtrl &tree)
{
	m_Node = GetTreeNode(tree, tree.GetRootItem());
}

//////////////////////////////////////////////////////////////////////
// CServerEntry

CServerEntry::CServerEntry()
{
	Init();
}
void CServerEntry::Init()
{
	m_EntryName.Empty();
	m_HostName.Empty();
	m_PortName.Empty();
	m_UserName.Empty();
	m_PassName.Empty();
	m_TermName = _T("xterm"); //.Empty();
	m_IdkeyName.Empty();
	m_KanjiCode = EUC_SET;
	m_ProtoType = PROTO_DIRECT;
	m_ProBuffer.Clear();
	m_CheckFlag = FALSE;
	m_Uid       = (-1);
	m_ChatScript.Init();
	m_ProxyMode = 0;
	m_ProxyHost.Empty();
	m_ProxyPort.Empty();
	m_ProxyUser.Empty();
	m_ProxyPass.Empty();
	m_Memo.Empty();
	m_Group.Empty();
	m_ScriptFile.Empty();
	m_ScriptStr.Empty();
	m_HostNameProvs.Empty();
	m_UserNameProvs.Empty();
	m_PassNameProvs.Empty();
	m_ProxyHostProvs.Empty();
	m_ProxyUserProvs.Empty();
	m_ProxyPassProvs.Empty();
	m_ProxySSLKeep = FALSE;
	m_BeforeEntry.Empty();
	m_ReEntryFlag = FALSE;
	m_DocType = (-1);
	m_IconName.Empty();
	m_bPassOk = TRUE;
	m_TitleName.Empty();
}
const CServerEntry & CServerEntry::operator = (CServerEntry &data)
{
	m_EntryName  = data.m_EntryName;
	m_HostName   = data.m_HostName;
	m_PortName   = data.m_PortName;
	m_UserName   = data.m_UserName;
	m_PassName   = data.m_PassName;
	m_TermName   = data.m_TermName;
	m_IdkeyName  = data.m_IdkeyName;
	m_KanjiCode  = data.m_KanjiCode;
	m_ProtoType  = data.m_ProtoType;
	m_ProBuffer  = data.m_ProBuffer;
	m_CheckFlag  = data.m_CheckFlag;
	m_Uid        = data.m_Uid;
	m_ChatScript = data.m_ChatScript;
	m_ProxyMode  = data.m_ProxyMode;
	m_ProxyHost  = data.m_ProxyHost;
	m_ProxyPort  = data.m_ProxyPort;
	m_ProxyUser  = data.m_ProxyUser;
	m_ProxyPass  = data.m_ProxyPass;
	m_Memo       = data.m_Memo;
	m_Group      = data.m_Group;
	m_ScriptFile = data.m_ScriptFile;
	m_ScriptStr  = data.m_ScriptStr;
	m_HostNameProvs  = data.m_HostNameProvs;
	m_UserNameProvs  = data.m_UserNameProvs;
	m_PassNameProvs  = data.m_PassNameProvs;
	m_ProxyHostProvs = data.m_ProxyHostProvs;
	m_ProxyUserProvs = data.m_ProxyUserProvs;
	m_ProxyPassProvs = data.m_ProxyPassProvs;
	m_ProxySSLKeep   = data.m_ProxySSLKeep;
	m_BeforeEntry    = data.m_BeforeEntry;
	m_ReEntryFlag    = data.m_ReEntryFlag;
	m_DocType        = data.m_DocType;
	m_IconName       = data.m_IconName;
	m_bPassOk        = data.m_bPassOk;
	m_TitleName      = data.m_TitleName;
	return *this;
}
void CServerEntry::GetArray(CStringArrayExt &stra)
{
	CIdKey key;
	CBuffer buf;
	CString str;

	m_EntryName = stra.GetAt(0);
	m_HostName  = stra.GetAt(1);
	m_PortName  = stra.GetAt(2);
	m_UserName  = stra.GetAt(3);
	key.DecryptStr(m_PassName, stra.GetAt(4), TRUE);
	m_TermName  = stra.GetAt(5);
	m_IdkeyName = stra.GetAt(6);
	m_KanjiCode = stra.GetVal(7);
	m_ProtoType = stra.GetVal(8);

	m_Uid = (stra.GetSize() > 9 ? stra.GetVal(9):(-1));

	if ( stra.GetSize() > 10 ) {
		stra.GetBuf(10, buf);
		m_ChatScript.GetBuffer(buf);
	} else
		m_ChatScript.Init();

	if ( stra.GetSize() > 15 ) {
		m_ProxyMode = stra.GetVal(11);
		m_ProxyHost = stra.GetAt(12);
		m_ProxyPort = stra.GetAt(13);
		m_ProxyUser = stra.GetAt(14);
		key.DecryptStr(m_ProxyPass, stra.GetAt(15), TRUE);
	}

	m_bPassOk = TRUE;

	if ( stra.GetSize() > 16 ) {
		key.DecryptStr(str, stra.GetAt(16), TRUE);
		if ( str.Compare(_T("12345678")) != 0 ) {
			m_PassName.Empty();
			m_ProxyPass.Empty();
			m_bPassOk = FALSE;
		}
	}

	m_Memo         = (stra.GetSize() > 17 ?  stra.GetAt(17) : _T(""));
	m_Group        = (stra.GetSize() > 18 ?  stra.GetAt(18) : _T(""));
	m_ScriptFile   = (stra.GetSize() > 19 ?  stra.GetAt(19) : _T(""));
	m_ScriptStr    = (stra.GetSize() > 20 ?  stra.GetAt(20) : _T(""));
	m_BeforeEntry  = (stra.GetSize() > 21 ?  stra.GetAt(21) : _T(""));
	m_ProxySSLKeep = (stra.GetSize() > 22 ?  stra.GetVal(22) : FALSE);;
	m_IconName     = (stra.GetSize() > 23 ?  stra.GetAt(23) : _T(""));

	m_TitleName.Empty();
	m_ProBuffer.Clear();

	m_HostNameProvs  = m_HostName;
	m_UserNameProvs  = m_UserName;
	m_PassNameProvs  = m_PassName;
	m_ProxyHostProvs = m_ProxyHost;
	m_ProxyUserProvs = m_ProxyUser;
	m_ProxyPassProvs = m_ProxyPass;
}
void CServerEntry::SetArray(CStringArrayExt &stra)
{
	CIdKey key;
	CString str;
	CBuffer buf;

	stra.RemoveAll();
	stra.Add(m_EntryName);
	stra.Add(m_HostNameProvs);
	stra.Add(m_PortName);
	stra.Add(m_UserNameProvs);
	key.EncryptStr(str, m_PassNameProvs, TRUE);
	stra.Add(str);
	stra.Add(m_TermName);
	stra.Add(m_IdkeyName);
	stra.AddVal(m_KanjiCode);
	stra.AddVal(m_ProtoType);
	stra.AddVal(m_Uid);
	m_ChatScript.SetBuffer(buf);
	stra.AddBin(buf.GetPtr(), buf.GetSize());
	stra.AddVal(m_ProxyMode);
	stra.Add(m_ProxyHostProvs);
	stra.Add(m_ProxyPort);
	stra.Add(m_ProxyUserProvs);
	key.EncryptStr(str, m_ProxyPassProvs, TRUE);
	stra.Add(str);
	key.EncryptStr(str, _T("12345678"), TRUE);
	stra.Add(str);
	stra.Add(m_Memo);
	stra.Add(m_Group);
	stra.Add(m_ScriptFile);
	stra.Add(m_ScriptStr);
	stra.Add(m_BeforeEntry);
	stra.AddVal(m_ProxySSLKeep);
	stra.Add(m_IconName);
}

static const ScriptCmdsDefs DocEntry[] = {
	{	"Host",			1	},
	{	"Port",			2	},
	{	"User",			3	},
	{	"Pass",			4	},
	{	"Term",			5	},
	{	"KeyFile",		6	},
	{	"Memo",			7	},
	{	"Group",		8	},
	{	"Script",		9	},
	{	"AddScript",	10	},
	{	"CodeSet",		11	},
	{	"Protocol",		12	},
	{	"Chat",			13	},
	{	"Proxy",		14	},
	{	"Before",		15	},
	{	"SSLKeep",		16	},
	{	NULL,			0	},
}, DocEntryProxy[] = {
	{	"Mode",			20	},
	{	"Host",			21	},
	{	"Port",			22	},
	{	"User",			23	},
	{	"Pass",			24	},
	{	NULL,			0	},
};

void CServerEntry::ScriptInit(int cmds, int shift, class CScriptValue &value)
{
	int n;

	value.m_DocCmds = cmds;

	for ( n = 0 ; DocEntry[n].name != NULL ; n++ )
		value[DocEntry[n].name].m_DocCmds = (DocEntry[n].cmds << shift) | cmds;

	for ( n = 0 ; DocEntryProxy[n].name != NULL ; n++ )
		value["Proxy"][DocEntryProxy[n].name].m_DocCmds = (DocEntryProxy[n].cmds << shift) | cmds;
}
void CServerEntry::ScriptValue(int cmds, class CScriptValue &value, int mode)
{
	int n, i;
	CString str;

	switch(cmds & 0xFF) {
	case 0:					// Document.Entry
		if ( mode == DOC_MODE_SAVE ) {
			for ( n = 0 ; DocEntry[n].name != NULL ; n++ ) {
				if ( (i = value.Find(DocEntry[n].name)) >= 0 )
					ScriptValue(DocEntry[n].cmds, value[i], mode);
			}
			m_EntryName = (LPCTSTR)value;
		} else if ( mode == DOC_MODE_IDENT ) {
			for ( n = 0 ; DocEntry[n].name != NULL ; n++ )
				ScriptValue(DocEntry[n].cmds, value[DocEntry[n].name], mode);
			 value = (LPCTSTR)m_EntryName;
		}
		break;

	case 1:					// Document.Entry.Host
		value.SetStr(m_HostName, mode);
		break;
	case 2:					// Document.Entry.Port
		value.SetStr(m_PortName, mode);
		break;
	case 3:					// Document.Entry.User
		value.SetStr(m_UserName, mode);
		break;
	case 4:					// Document.Entry.Pass
		value.SetStr(m_PassName, mode);
		break;
	case 5:					// Document.Entry.Term
		value.SetStr(m_TermName, mode);
		break;
	case 6:					// Document.Entry.KeyFile
		value.SetStr(m_IdkeyName, mode);
		break;
	case 7:					// Document.Entry.Memo
		value.SetStr(m_Memo, mode);
		break;
	case 8:					// Document.Entry.Group
		value.SetStr(m_Group, mode);
		break;
	case 9:					// Document.Entry.Script
		value.SetStr(m_ScriptFile, mode);
		break;
	case 10:				// Document.Entry.AddScript
		value.SetStr(m_ScriptStr, mode);
		break;

	case 11:				// Document.Entry.CodeSet
		str = GetKanjiCode();
		value.SetStr(str, mode);
		SetKanjiCode(str);
	case 12:				// Document.Entry.Protocol
		str = GetProtoName();
		value.SetStr(str, mode);
		m_ProtoType = GetProtoType(str);
	case 13:				// Document.Entry.Chat
		m_ChatScript.SetString(str);
		value.SetStr(str, mode);
		m_ChatScript.GetString(str);
		break;

	case 14:				// Document.Entry.Proxy
		if ( mode == DOC_MODE_SAVE ) {
			for ( n = 0 ; DocEntryProxy[n].name != NULL ; n++ ) {
				if ( (i = value.Find(DocEntryProxy[n].name)) >= 0 )
					ScriptValue(DocEntryProxy[n].cmds, value[i], mode);
			}
		} else if ( mode == DOC_MODE_IDENT ) {
			for ( n = 0 ; DocEntryProxy[n].name != NULL ; n++ )
				ScriptValue(DocEntryProxy[n].cmds, value[DocEntryProxy[n].name], mode);
		}
		break;

	case 15:				// Docyment.Entry.Before
		value.SetStr(m_BeforeEntry, mode);
		break;
	case 16:				// Docyment.Entry.SSLKeep
		value.SetInt(m_ProxySSLKeep, mode);
		break;

	case 20:				// Document.Entry.Proxy.Mode
		value.SetInt(m_ProxyMode, mode);
		break;
	case 21:				// Document.Entry.Proxy.Host
		value.SetStr(m_ProxyHost, mode);
		break;
	case 22:				// Document.Entry.Proxy.Port
		value.SetStr(m_ProxyPort, mode);
		break;
	case 23:				// Document.Entry.Proxy.User
		value.SetStr(m_ProxyUser, mode);
		break;
	case 24:				// Document.Entry.Proxy.Pass
		value.SetStr(m_ProxyPass, mode);
		break;
	}
}
void CServerEntry::SetBuffer(CBuffer &buf)
{
	CStringArrayExt stra;
	SetArray(stra);
	stra.SetBuffer(buf);
	buf.PutBuf(m_ProBuffer.GetPtr(), m_ProBuffer.GetSize());
}
int CServerEntry::GetBuffer(CBuffer &buf)
{
	CStringArrayExt stra;
	stra.GetBuffer(buf);
	if ( stra.GetSize() < 9 )
		return FALSE; 
	GetArray(stra);
	buf.GetBuf(&m_ProBuffer);
	return TRUE;
}
void CServerEntry::SetProfile(LPCTSTR pSection)
{
	CBuffer buf;
	CString entry;
	entry.Format(_T("List%02d"), m_Uid);
	SetBuffer(buf);
	((CRLoginApp *)AfxGetApp())->WriteProfileBinary(pSection, entry, buf.GetPtr(), buf.GetSize());
}
int CServerEntry::GetProfile(LPCTSTR pSection, int Uid)
{
	CBuffer buf;
	CString entry;
	entry.Format(_T("List%02d"), Uid);
	((CRLoginApp *)AfxGetApp())->GetProfileBuffer(pSection, entry, buf);
	return GetBuffer(buf);
}
void CServerEntry::DelProfile(LPCTSTR pSection)
{
	CString entry;
	entry.Format(_T("List%02d"), m_Uid);
	((CRLoginApp *)AfxGetApp())->DelProfileEntry(pSection, entry);
}
void CServerEntry::Serialize(CArchive &ar)
{
	CStringArrayExt stra;

	if ( ar.IsStoring() ) {		// TODO: この位置に保存用のコードを追加してください。
		SetArray(stra);
		stra.Serialize(ar);
	} else {					// TODO: この位置に読み込み用のコードを追加してください。
		stra.Serialize(ar);
		if ( stra.GetSize() < 9 )
			Init();
		else
			GetArray(stra);
	}
}
void CServerEntry::SetIndex(int mode, CStringIndex &index)
{
	int n, i;
	CIdKey key;
	CString str, pass;

	if ( mode ) {		// Write
		index[_T("Name")]  = m_EntryName;
		index[_T("Host")]  = m_HostNameProvs;
		index[_T("Port")]  = m_PortName;
		index[_T("User")]  = m_UserNameProvs;
		index[_T("Term")]  = m_TermName;
		index[_T("IdKey")] = m_IdkeyName;

		pass.Format(_T("TEST%s"), m_PassNameProvs);
		key.EncryptStr(str, pass, TRUE);
		index[_T("Pass")]  = str;

		index[_T("CharSet")]  = GetKanjiCode();
		index[_T("Protocol")] = GetProtoName();
		
		index[_T("Memo")]  = m_Memo;
		index[_T("Group")] = m_Group;

		index[_T("Proxy")][_T("Mode")] = m_ProxyMode;
		index[_T("Proxy")][_T("Host")] = m_ProxyHostProvs;
		index[_T("Proxy")][_T("Port")] = m_ProxyPort;
		index[_T("Proxy")][_T("User")] = m_ProxyUserProvs;

		pass.Format(_T("TEST%s"), m_ProxyPassProvs);
		key.EncryptStr(str, pass, TRUE);
		index[_T("Proxy")][_T("Pass")] = str;

		index[_T("Script")][_T("File")] = m_ScriptFile;
		index[_T("Script")][_T("Text")] = m_ScriptStr;

		m_ChatScript.SetString(str);
		index[_T("Chat")]  = str;

		index[_T("Before")] = m_BeforeEntry;
		index[_T("Icon")] = m_IconName;

	} else {			// Read
		if ( (n = index.Find(_T("Name"))) >= 0 )
			m_EntryName = index[n];
		if ( (n = index.Find(_T("Host"))) >= 0 )
			m_HostName = index[n];
		if ( (n = index.Find(_T("Port"))) >= 0 )
			m_PortName = index[n];
		if ( (n = index.Find(_T("User"))) >= 0 )
			m_UserName = index[n];
		if ( (n = index.Find(_T("Term"))) >= 0 )
			m_TermName = index[n];
		if ( (n = index.Find(_T("IdKey"))) >= 0 )
			m_IdkeyName = index[n];

		m_bPassOk = TRUE;

		if ( (n = index.Find(_T("Pass"))) >= 0 ) {
			key.DecryptStr(str, index[n], TRUE);
			if ( str.Left(4).Compare(_T("TEST")) == 0 )
				m_PassName = str.Mid(4);
			else
				m_bPassOk = FALSE;
		}

		if ( (n = index.Find(_T("CharSet"))) >= 0 )
			SetKanjiCode(index[n]);
		if ( (n = index.Find(_T("Protocol"))) >= 0 )
			m_ProtoType = GetProtoType(index[n]);

		if ( (n = index.Find(_T("Memo"))) >= 0 )
			m_Memo = index[n];
		if ( (n = index.Find(_T("Group"))) >= 0 )
			m_Group = index[n];
		
		if ( (n = index.Find(_T("Proxy"))) >= 0 ) {
			if ( (i = index[n].Find(_T("Mode"))) >= 0 )
				m_ProxyMode = index[n][i];
			if ( (i = index[n].Find(_T("Host"))) >= 0 )
				m_ProxyHost = index[n][i];
			if ( (i = index[n].Find(_T("Port"))) >= 0 )
				m_ProxyPort = index[n][i];
			if ( (i = index[n].Find(_T("User"))) >= 0 )
				m_ProxyUser = index[n][i];
			if ( (i = index[n].Find(_T("Pass"))) >= 0 ) {
				key.DecryptStr(str, index[n][i], TRUE);
				if ( str.Left(4).Compare(_T("TEST")) == 0 )
					m_ProxyPass = str.Mid(4);
				else
					m_bPassOk = FALSE;
			}
		}

		if ( (n = index.Find(_T("Script"))) >= 0 ) {
			if ( (i = index[n].Find(_T("File"))) >= 0 )
				m_ScriptFile = index[n][i];
			if ( (i = index[n].Find(_T("Text"))) >= 0 )
				m_ScriptStr = index[n][i];
		}

		if ( (n = index.Find(_T("Chat"))) >= 0 )
			m_ChatScript.GetString(index[n]);

		if ( (n = index.Find(_T("Before"))) >= 0 )
			m_BeforeEntry = index[n];

		if ( (n = index.Find(_T("Icon"))) >= 0 )
			m_IconName = index[n];

		m_ProBuffer.Clear();

		m_HostNameProvs  = m_HostName;
		m_UserNameProvs  = m_UserName;
		m_PassNameProvs  = m_PassName;
		m_ProxyHostProvs = m_ProxyHost;
		m_ProxyUserProvs = m_ProxyUser;
		m_ProxyPassProvs = m_ProxyPass;
	}
}
LPCTSTR CServerEntry::GetKanjiCode()
{
	switch(m_KanjiCode) {
	case SJIS_SET:	return _T("SJIS");
	case ASCII_SET:	return _T("ASCII");
	case UTF8_SET:	return _T("UTF8");
	case BIG5_SET:	return _T("BIG5");
	default:		return _T("EUC");
	}
}
void CServerEntry::SetKanjiCode(LPCTSTR str)
{
	if (      _tcscmp(str, _T("EUC")) == 0 )	m_KanjiCode = EUC_SET;
	else if ( _tcscmp(str, _T("SJIS")) == 0 )	m_KanjiCode = SJIS_SET;
	else if ( _tcscmp(str, _T("ASCII")) == 0 )	m_KanjiCode = ASCII_SET;
	else if ( _tcscmp(str, _T("UTF8")) == 0 )	m_KanjiCode = UTF8_SET;
	else if ( _tcscmp(str, _T("BIG5")) == 0 )	m_KanjiCode = BIG5_SET;
	else									    m_KanjiCode = EUC_SET;
}
LPCTSTR CServerEntry::GetProtoName()
{
	switch(m_ProtoType) {
	case PROTO_COMPORT:	return _T("serial");
	case PROTO_TELNET:	return _T("telnet");
	case PROTO_LOGIN:	return _T("login");
	case PROTO_SSH:		return _T("ssh");
	default:			return _T("direct");
	}
}
int CServerEntry::GetProtoType(LPCTSTR str)
{
	if ( _tcscmp(str, _T("serial")) == 0 )
		return PROTO_COMPORT;
	else if ( _tcscmp(str, _T("telnet")) == 0 )
		return PROTO_TELNET;
	else if ( _tcscmp(str, _T("login")) == 0 )
		return PROTO_LOGIN;
	else if ( _tcscmp(str, _T("ssh")) == 0 )
		return PROTO_SSH;
	else
		return PROTO_DIRECT;
}

//////////////////////////////////////////////////////////////////////
// CServerEntryTab

CServerEntryTab::CServerEntryTab()
{
	m_pSection = _T("ServerEntryTab");
	m_MinSize = 1;
}
void CServerEntryTab::Init()
{
	m_Data.RemoveAll();
}
void CServerEntryTab::SetArray(CStringArrayExt &stra)
{
	CStringArrayExt tmp;

	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_Data[n].SetArray(tmp);
		stra.AddArray(tmp);
	}
}
void CServerEntryTab::GetArray(CStringArrayExt &stra)
{
	CStringArrayExt tmp;
	CServerEntry dum;

	m_Data.RemoveAll();
	for ( int n = 0 ; n < stra.GetSize() ; n++ ) {
		stra.GetArray(n, tmp);
		dum.GetArray(tmp);
		m_Data.Add(dum);
	}
}
void CServerEntryTab::Serialize(int mode)
{
	int n;

	if ( mode ) {		// Write
		for ( n = 0 ; n < m_Data.GetSize() ; n++ )
			m_Data[n].SetProfile(m_pSection);
	} else {
		int uid;
		CServerEntry tmp;
		CStringArrayExt entry;
		CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();
		pApp->GetProfileKeys(m_pSection, entry);
		m_Data.RemoveAll();
		for ( n = 0 ; n < entry.GetSize() ; n++ ) {
			if ( entry[n].Left(4).Compare(_T("List")) != 0 || _tcschr(_T("0123456789"), entry[n][4]) == NULL )
				continue;
			uid = _tstoi(entry[n].Mid(4));
			if ( tmp.GetProfile(m_pSection, uid) ) {
				if ( tmp.m_Uid == (-1) )
					tmp.m_Uid = uid;
				m_Data.Add(tmp);
			}
		}
	}
}
int CServerEntryTab::AddEntry(CServerEntry &Entry)
{
	int n;

	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].m_Uid == Entry.m_Uid ) {
			Entry.SetProfile(m_pSection);
			m_Data[n] = Entry;
			return n;
		}
	}
	Entry.m_Uid = ((CRLoginApp *)AfxGetApp())->GetProfileSeqNum(m_pSection, _T("ListMax"));
	Entry.SetProfile(m_pSection);
	return (int)m_Data.Add(Entry);
}
void CServerEntryTab::UpdateAt(int nIndex)
{
	m_Data[nIndex].SetProfile(m_pSection);
}
void CServerEntryTab::RemoveAt(int nIndex)
{
	m_Data[nIndex].DelProfile(m_pSection);
	m_Data.RemoveAt(nIndex);
}
void CServerEntryTab::ReloadAt(int nIndex)
{
	if ( m_Data[nIndex].m_Uid != (-1) )
		m_Data[nIndex].GetProfile(m_pSection, m_Data[nIndex].m_Uid);
}

//////////////////////////////////////////////////////////////////////
// CKeyNode

CKeyNode::CKeyNode()
{
	m_Code = (-1);
	m_Mask = 0;
	m_Maps.Clear();
}
LPCTSTR CKeyNode::GetMaps()
{
	int n;
	LPCWSTR p;
	CString tmp;
	
	m_Temp = _T("");
	p = m_Maps;
	for ( n = 0 ; n < m_Maps.GetSize() ; n += 2, p++ ) {
		switch(*p) {
		case L'\b': m_Temp += _T("\\b"); break;
		case L'\t': m_Temp += _T("\\t"); break;
		case L'\n': m_Temp += _T("\\n"); break;
		case L'\a': m_Temp += _T("\\a"); break;
		case L'\f': m_Temp += _T("\\f"); break;
		case L'\r': m_Temp += _T("\\r"); break;
		case L'\\': m_Temp += _T("\\\\"); break;
		case L' ':  m_Temp += _T("\\s"); break;
		default:
			if ( *p == L'\x7F' || *p < L' ' ) {
				tmp.Format(_T("\\%03o"), *p);
				m_Temp += tmp;
			} else {
				tmp = *p;
				m_Temp += tmp;
			}
			break;
		}
	}
	return m_Temp;
}
void CKeyNode::SetMaps(LPCTSTR str)
{
	int c, n;
	CStringW tmp;
	LPCWSTR p;

	tmp = str;
	p = tmp;
	m_Maps.Clear();
	while ( *p != L'\0' ) {
		if ( *p == L'\\' ) {
			switch(*(++p)) {
			case L'b': m_Maps.PutWord(L'\b'); p++; break;
			case L't': m_Maps.PutWord(L'\t'); p++; break;
			case L'n': m_Maps.PutWord(L'\n'); p++; break;
			case L'a': m_Maps.PutWord(L'\a'); p++; break;
			case L'f': m_Maps.PutWord(L'\f'); p++; break;
			case L'r': m_Maps.PutWord(L'\r'); p++; break;
			case L's': m_Maps.PutWord(L' '); p++; break;
			case L'\\': m_Maps.PutWord(L'\\'); p++; break;
			case L'x': case L'X':
				p++;
				for ( c = n = 0 ; n < 2 ; n++) {
					if ( *p >= L'0' && *p <= L'9' )
						c = c * 16 + (*(p++) - L'0');
					else if ( *p >= L'A' && *p <= L'F' )
						c = c * 16 + (*(p++) - L'A' + 10);
					else if ( *p >= L'a' && *p <= L'f' )
						c = c * 16 + (*(p++) - L'a' + 10);
					else
						break;
				}
				m_Maps.PutWord(c);
				break;
			case L'0': case L'1': case L'2': case L'3':
			case L'4': case L'5': case L'6': case L'7':
				for ( c = n = 0 ; n < 3 &&
					*p >= L'0' && *p <= L'7' ; )
					c = c * 8 + (*(p++) - L'0');
				m_Maps.PutWord(c);
				break;
			default:
				m_Maps.PutWord(*(p++));
				break;
			}
		} else
			m_Maps.PutWord(*(p++));
	}
}

static const struct _KeyNameTab	{
	int		code;
	LPCTSTR	name;
} KeyNameTab[] = {
	{ VK_F1,		_T("F1")		},
	{ VK_F2,		_T("F2")		},
	{ VK_F3,		_T("F3")		},
	{ VK_F4,		_T("F4")		},
	{ VK_F5,		_T("F5")		},
	{ VK_F6,		_T("F6")		},
	{ VK_F7,		_T("F7")		},
	{ VK_F8,		_T("F8")		},
	{ VK_F9,		_T("F9")		},
	{ VK_F10,		_T("F10")		},
	{ VK_F11,		_T("F11")		},
	{ VK_F12,		_T("F12")		},
	{ VK_F13,		_T("F13")		},
	{ VK_F14,		_T("F14")		},
	{ VK_F15,		_T("F15")		},
	{ VK_F16,		_T("F16")		},
	{ VK_F17,		_T("F17")		},
	{ VK_F18,		_T("F18")		},
	{ VK_F19,		_T("F19")		},
	{ VK_F20,		_T("F20")		},
	{ VK_F21,		_T("F21")		},
	{ VK_F22,		_T("F22")		},
	{ VK_F23,		_T("F23")		},
	{ VK_F24,		_T("F24")		},

	{ VK_UP,		_T("UP")		},
	{ VK_DOWN,		_T("DOWN")		},
	{ VK_RIGHT,		_T("RIGHT")		},
	{ VK_LEFT,		_T("LEFT")		},

	{ VK_PRIOR,		_T("PRIOR")		},
	{ VK_NEXT,		_T("NEXT")		},
	{ VK_INSERT,	_T("INSERT")	},
	{ VK_DELETE,	_T("DELETE")	},
	{ VK_HOME,		_T("HOME")		},
	{ VK_END,		_T("END")		},

	{ VK_PAUSE,		_T("PAUSE")		},
	{ VK_CANCEL,	_T("BREAK")		},

	{ VK_ESCAPE,	_T("ESCAPE")	},
	{ VK_RETURN,	_T("RETURN")	},
	{ VK_BACK,		_T("BACK")		},
	{ VK_TAB,		_T("TAB")		},
	{ VK_SPACE,		_T("SPACE")		},

	{ VK_NUMPAD0,	_T("PAD0")		},
	{ VK_NUMPAD1,	_T("PAD1")		},
	{ VK_NUMPAD2,	_T("PAD2")		},
	{ VK_NUMPAD3 ,	_T("PAD3")		},
	{ VK_NUMPAD4,	_T("PAD4")		},
	{ VK_NUMPAD5,	_T("PAD5")		},
	{ VK_NUMPAD6,	_T("PAD6")		},
	{ VK_NUMPAD7,	_T("PAD7")		},
	{ VK_NUMPAD8,	_T("PAD8")		},
	{ VK_NUMPAD9,	_T("PAD9")		},
	{ VK_MULTIPLY,	_T("PADMUL")	},
	{ VK_ADD,		_T("PADADD")	},
	{ VK_SEPARATOR,	_T("PADSEP")	},
	{ VK_SUBTRACT,	_T("PADSUB")	},
	{ VK_DECIMAL,	_T("PADDEC")	},
	{ VK_DIVIDE,	_T("PADDIV")	},

	{ VK_OEM_1,		_T("$BA(:)")	},
	{ VK_OEM_PLUS,	_T("$BB(;)")	},
	{ VK_OEM_COMMA,	_T("$BC(,)")	},
	{ VK_OEM_MINUS,	_T("$BD(-)")	},
	{ VK_OEM_PERIOD,_T("$BE(.)")	},
	{ VK_OEM_2,		_T("$BF(/)")	},
	{ VK_OEM_3,		_T("$C0(@)")	},
	{ VK_OEM_4,		_T("$DB([)")	},
	{ VK_OEM_5,		_T("$DC(\\)")	},
	{ VK_OEM_6,		_T("$DD(])")	},
	{ VK_OEM_7,		_T("$DE(^)")	},
	{ VK_OEM_102,	_T("$E2(_)")	},

	{ VK_MBUTTON,	_T("MBTN")		},
	{ VK_XBUTTON1,	_T("XBTN1")		},
	{ VK_XBUTTON2,	_T("XBTN2")		},

	{ (-1),			NULL			},
};

LPCTSTR CKeyNode::GetCode()
{
	int n;
	if ( m_Code == (-1) )
		return _T("");
	for ( n = 0 ; KeyNameTab[n].name != NULL ; n++ ) {
		if ( KeyNameTab[n].code == m_Code )
			return KeyNameTab[n].name;
	}
	// * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
	// * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
	if ( m_Code >= 0x30 && m_Code <= 0x39 || m_Code >= 0x41 && m_Code <= 0x5A )
		m_Temp.Format(_T("%c"), m_Code);
	else
		m_Temp.Format(_T("$%02X"), m_Code);

	return m_Temp;
}
void CKeyNode::SetCode(LPCTSTR name)
{
	int n;
	for ( n = 0 ; KeyNameTab[n].name != NULL ; n++ ) {
		if ( _tcscmp(KeyNameTab[n].name, name) == 0 ) {
			m_Code = KeyNameTab[n].code;
			return;
		}
	}
	if ( name[1] == _T('\0') && (*name >= 0x30 && *name <= 0x39 || *name >= 0x41 && *name <= 0x5A) ) {
		m_Code = *name;
	} else if ( name[0] == _T('$') && isxdigit(name[1]) && isxdigit(name[2]) ) {
		m_Code = 0;
		if ( isdigit(name[1]) )
			m_Code += ((name[1] - _T('0')) * 16);
		else
			m_Code += ((toupper(name[1])- _T('A')) * 16);
		if ( isdigit(name[2]) )
			m_Code += (name[2] - _T('0'));
		else
			m_Code += (toupper(name[2])- _T('A'));
	} else
		m_Code = _tstoi(name);
}
void CKeyNode::SetKeyMap(LPCTSTR str, int type, char map[256])
{
	int n, i;
	int fCode, eCode;
	CString tmp;
	CStringArrayExt list;

	list.GetString(str, _T(','));

	for ( n = 0 ; n < list.GetSize() ; n++ ) {
		if ( (i = list[n].Find(_T('-'))) >= 0 ) {
			m_Code = (-1); SetCode(list[n].Left(i)); fCode = m_Code;
			m_Code = (-1); SetCode(list[n].Mid(i + 1)); eCode = m_Code;
			if ( fCode >= 0 ) {
				while ( fCode <= eCode )
					map[fCode++] = type;
			}
		} else {
			m_Code = (-1);
			SetCode(list[n]);
			if ( m_Code >= 0 && m_Code < 256 )
				map[m_Code] = type;
		}
	}
}
LPCTSTR CKeyNode::GetMask()
{
	m_Temp = _T("");
	if ( (m_Mask & MASK_CTRL) )  m_Temp += _T("Ctrl+");
	if ( (m_Mask & MASK_SHIFT) ) m_Temp += _T("Shift+");
	if ( (m_Mask & MASK_ALT) )   m_Temp += _T("Alt+");

	if ( (m_Mask & MASK_APPL) )  m_Temp += _T("App+");
	if ( (m_Mask & MASK_CKM) )   m_Temp += _T("Ckm+");
	if ( (m_Mask & MASK_VT52) )  m_Temp += _T("VT52+");

	//if ( (m_Mask & MASK_NUMLCK) ) m_Temp += _T("Num+");
	//if ( (m_Mask & MASK_SCRLCK) ) m_Temp += _T("Scr+");
	//if ( (m_Mask & MASK_CAPLCK) ) m_Temp += _T("Cap+");

	if ( !m_Temp.IsEmpty() )
		m_Temp.Delete(m_Temp.GetLength() - 1, 1);
	return m_Temp;
}
const CKeyNode & CKeyNode::operator = (CKeyNode &data)
{
	m_Code = data.m_Code;
	m_Mask = data.m_Mask;
	m_Maps = data.m_Maps;
	return *this;
}
void CKeyNode::CommandLine(LPCWSTR str, CStringW &cmd)
{
	CStringW tmp;
	while ( *str != L'\0' ) {
		if ( wcschr(L" '\"\\<>|&;$()", *str) != NULL ) {
			tmp += L'\\';
			tmp += *(str++);
		} else
			tmp += *(str++);
	}
	cmd.Format((LPCWSTR)m_Maps, tmp);
}
void CKeyNode::SetComboList(CComboBox *pCombo)
{
	int n;
	CString str;

	if ( pCombo == NULL )
		return;

	for ( n = pCombo->GetCount() - 1 ; n >= 0; n-- )
		pCombo->DeleteString(n);

	for ( n = 0 ; KeyNameTab[n].name != NULL ; n++ )
		pCombo->AddString(KeyNameTab[n].name);

	for ( n = 0x30 ; n <= 0x39 ; n++ ) {
		str.Format(_T("%c"), n);
		pCombo->AddString(str);
	}
	for ( n = 0x41 ; n <= 0x5A ; n++ ) {
		str.Format(_T("%c"), n);
		pCombo->AddString(str);
	}
}

//////////////////////////////////////////////////////////////////////
// CKeyCmds & CKeyCmdsTab

const CKeyCmds & CKeyCmds::operator = (CKeyCmds &data)
{
	m_Id = data.m_Id;
	m_Flag = data.m_Flag;
	m_Menu = data.m_Menu;
	m_Text = data.m_Text;

	return *this;
}
void CKeyCmds::SetMenu(CMenu *pMenu)
{
	CString str;

	str.Format(_T("%s\t%s"), m_Text, m_Menu);
	pMenu->ModifyMenu(m_Id, MF_BYCOMMAND | MF_STRING, m_Id, str);
}
void CKeyCmds::ResetMenu(CMenu *pMenu)
{
	int i;
	CString str;

	if ( pMenu->GetMenuString(m_Id, str, MF_BYCOMMAND) <= 0 )
		return;

	if ( (i = str.Find('\t')) < 0 )
		return;

	str.Truncate(i);
	pMenu->ModifyMenu(m_Id, MF_BYCOMMAND | MF_STRING, m_Id, str);
}
const CKeyCmdsTab & CKeyCmdsTab::operator = (CKeyCmdsTab &data)
{
	int n;

	SetSize(data.GetSize());
	for ( n = 0 ; n < data.GetSize() ; n++ )
		m_Data[n] = data[n];

	return *this;
}
static int KeyCmdsComp(const void *src, const void *dis)
{
	return (*((int *)src) - ((CKeyCmds *)dis)->m_Id);
}
int CKeyCmdsTab::Add(CKeyCmds &cmds)
{
	int n;

	if ( BinaryFind(&(cmds.m_Id), m_Data.GetData(), sizeof(CKeyCmds), (int)m_Data.GetSize(), KeyCmdsComp, &n) )
		return n;

	m_Data.InsertAt(n, cmds);
	return n;
}
int CKeyCmdsTab::Find(int id)
{
	int n;

	if ( BinaryFind(&id, m_Data.GetData(), sizeof(CKeyCmds), (int)m_Data.GetSize(), KeyCmdsComp, &n) )
		return n;

	return (-1);
}
void CKeyCmdsTab::ResetMenuAll(CMenu *pMenu)
{
	int n;

	for ( n = 0 ; n < GetSize() ; n++ )
		m_Data[n].ResetMenu(pMenu);
}
static int KeyCmdsTextCmp(const void *src, const void *dis)
{
	return ((CKeyCmds *)src)->m_Text.Compare(((CKeyCmds *)dis)->m_Text);
}
void CKeyCmdsTab::SortText()
{
	qsort(m_Data.GetData(), m_Data.GetSize(), sizeof(CKeyCmds), KeyCmdsTextCmp);
}

//////////////////////////////////////////////////////////////////////
// CKeyNodeTab

static const struct _InitKeyTab {
		int fix;
		int	code;
		int mask;
		LPCTSTR maps;
} InitKeyTab[] = {
		{ 0,	VK_UP,			0,						_T("\\033[A")		},
		{ 0,	VK_DOWN,		0,						_T("\\033[B")		},
		{ 0,	VK_RIGHT,		0,						_T("\\033[C")		},
		{ 0,	VK_LEFT,		0,						_T("\\033[D")		},

		{ 0,	VK_HOME,		0,						_T("\\033[H")		},	//	kh
		{ 0,	VK_INSERT,		0,						_T("\\033[2~")		},	//	kI
		{ 0,	VK_DELETE,		0,						_T("\\033[3~")		},	//	kD
		{ 0,	VK_END,			0,						_T("\\033[F")		},	//	@7
		{ 0,	VK_PRIOR,		0,						_T("\\033[5~")		},	//	kP
		{ 0,	VK_NEXT,		0,						_T("\\033[6~")		},	//	kN

		{ 0,	VK_UP,			MASK_CKM,				_T("\\033OA")		},	//	ku
		{ 0,	VK_DOWN,		MASK_CKM,				_T("\\033OB")		},	//	kd
		{ 0,	VK_RIGHT,		MASK_CKM,				_T("\\033OC")		},	//	kr
		{ 0,	VK_LEFT,		MASK_CKM,				_T("\\033OD")		},	//	kl

		{ 0,	VK_HOME,		MASK_CKM,				_T("\\033OH")		},
		{ 0,	VK_END,			MASK_CKM,				_T("\\033OF")		},
		{ 0,	VK_ESCAPE,		MASK_CKM,				_T("\\033O[")		},

		{ 0,	VK_UP,			MASK_VT52,				_T("\\033A")		},
		{ 0,	VK_DOWN,		MASK_VT52,				_T("\\033B")		},
		{ 0,	VK_RIGHT,		MASK_VT52,				_T("\\033C")		},
		{ 0,	VK_LEFT,		MASK_VT52,				_T("\\033D")		},

		{ 0,	VK_F1,			0,						_T("\\033OP")		},	//	k1
		{ 0,	VK_F2,			0,						_T("\\033OQ")		},	//	k2
		{ 0,	VK_F3,			0,						_T("\\033OR")		},	//	k3
		{ 0,	VK_F4,			0,						_T("\\033OS")		},	//	k4
		{ 0,	VK_F5,			0,						_T("\\033[15~")		},	//	k5
		{ 0,	VK_F6,			0,						_T("\\033[17~")		},	//	k6
		{ 0,	VK_F7,			0,						_T("\\033[18~")		},	//	k7
		{ 0,	VK_F8,			0,						_T("\\033[19~")		},	//	k8
		{ 0,	VK_F9,			0,						_T("\\033[20~")		},	//	k9
		{ 0,	VK_F10,			0,						_T("\\033[21~")		},	//	k;
		{ 0,	VK_F11,			0,						_T("\\033[23~")		},	//	F1
		{ 0,	VK_F12,			0,						_T("\\033[24~")		},	//	F2
		{ 0,	VK_F13,			0,						_T("\\033[25~")		},	//	F3
		{ 0,	VK_F14,			0,						_T("\\033[26~")		},	//	F4
		{ 0,	VK_F15,			0,						_T("\\033[28~")		},	//	F5
		{ 0,	VK_F16,			0,						_T("\\033[29~")		},	//	F6
		{ 0,	VK_F17,			0,						_T("\\033[31~")		},	//	F7
		{ 0,	VK_F18,			0,						_T("\\033[32~")		},	//	F8
		{ 0,	VK_F19,			0,						_T("\\033[33~")		},	//	F9
		{ 0,	VK_F20,			0,						_T("\\033[34~")		},	//	FA

		{ 0,	VK_TAB,			MASK_SHIFT,				_T("\\033[Z")		},

		{ 0,	VK_SEPARATOR,	MASK_APPL,				_T("\\033OX")		},	// = (equal)    | =	  | SS3 X
		{ 0,	VK_MULTIPLY,	MASK_APPL,				_T("\\033Oj")		},	// * (multiply) | *	  | SS3 j
		{ 0,	VK_ADD,			MASK_APPL,				_T("\\033On")		},	// + (add)      | +	  | SS3 k
		{ 0,	VK_SUBTRACT,	MASK_APPL,				_T("\\033Om")		},	// - (minus)    | -	  | SS3 m
		{ 0,	VK_DECIMAL,		MASK_APPL,				_T("\\033On")		},	// . (period)   | .	  | SS3 n
		{ 0,	VK_DIVIDE,		MASK_APPL,				_T("\\033Oo")		},	// / (divide)   | /	  | SS3 o
		{ 0,	VK_NUMPAD0,		MASK_APPL,				_T("\\033Op")		},	// 0	        | 0	  | SS3 p
		{ 0,	VK_NUMPAD1,		MASK_APPL,				_T("\\033Oq")		},	// 1	        | 1	  | SS3 q
		{ 0,	VK_NUMPAD2,		MASK_APPL,				_T("\\033Or")		},	// 2	        | 2	  | SS3 r
		{ 0,	VK_NUMPAD3,		MASK_APPL,				_T("\\033Os")		},	// 3	        | 3	  | SS3 s
		{ 0,	VK_NUMPAD4,		MASK_APPL,				_T("\\033Ot")		},	// 4	        | 4	  | SS3 t
		{ 0,	VK_NUMPAD5,		MASK_APPL,				_T("\\033Ou")		},	// 5	        | 5	  | SS3 u
		{ 0,	VK_NUMPAD6,		MASK_APPL,				_T("\\033Ov")		},	// 6	        | 6	  | SS3 v
		{ 0,	VK_NUMPAD7,		MASK_APPL,				_T("\\033Ow")		},	// 7	        | 7	  | SS3 w
		{ 0,	VK_NUMPAD8,		MASK_APPL,				_T("\\033Ox")		},	// 8	        | 8	  | SS3 x
		{ 0,	VK_NUMPAD9,		MASK_APPL,				_T("\\033Oy")		},	// 9	        | 9	  | SS3 y

		{ 0,	VK_PRIOR,		MASK_SHIFT,				_T("$PRIOR")		},
		{ 0,	VK_NEXT,		MASK_SHIFT,				_T("$NEXT")			},
		{ 0,	VK_PRIOR,		MASK_CTRL,				_T("$SEARCH_BACK")	},
		{ 0,	VK_NEXT,		MASK_CTRL,				_T("$SEARCH_NEXT")	},
		{ 0,	VK_CANCEL,		0,						_T("$BREAK")		},

		{ 0,	VK_UP,			MASK_CTRL,				_T("$PANE_ROTATION")},
		{ 0,	VK_DOWN,		MASK_CTRL,				_T("$SPLIT_HEIGHT")	},
		{ 0,	VK_RIGHT,		MASK_CTRL,				_T("$SPLIT_WIDTH")	},
		{ 0,	VK_LEFT,		MASK_CTRL,				_T("$SPLIT_OVER")	},

		{ 0,	VK_TAB,			MASK_CTRL,				_T("$PANE_NEXT")	},
		{ 0,	VK_TAB,			MASK_CTRL | MASK_SHIFT,	_T("$PANE_PREV")	},

		{ 6,	VK_UP,			MASK_CTRL | MASK_SHIFT,	_T("$PANE_TILEHORZ")},
		{ 6,	VK_LEFT,		MASK_CTRL | MASK_SHIFT,	_T("$PANE_CASCADE")	},

		{ 7,	VK_DOWN,		MASK_CTRL | MASK_SHIFT,	_T("$SPLIT_HEIGHTNEW")},
		{ 7,	VK_RIGHT,		MASK_CTRL | MASK_SHIFT,	_T("$SPLIT_WIDTHNEW")},

		{ 8,	VK_UP,			MASK_ALT,				_T("$PANE_UP")		},
		{ 8,	VK_DOWN,		MASK_ALT,				_T("$PANE_DOWN")	},
		{ 8,	VK_RIGHT,		MASK_ALT,				_T("$PANE_RIGHT")	},
		{ 8,	VK_LEFT,		MASK_ALT,				_T("$PANE_LEFT")	},

		{ 8,	'1',			MASK_CTRL,				_T("$PANE_SEL1")	},
		{ 8,	'2',			MASK_CTRL,				_T("$PANE_SEL2")	},
		{ 8,	'3',			MASK_CTRL,				_T("$PANE_SEL3")	},
		{ 8,	'4',			MASK_CTRL,				_T("$PANE_SEL4")	},
		{ 8,	'5',			MASK_CTRL,				_T("$PANE_SEL5")	},
		{ 8,	'6',			MASK_CTRL,				_T("$PANE_SEL6")	},
		{ 8,	'7',			MASK_CTRL,				_T("$PANE_SEL7")	},
		{ 8,	'8',			MASK_CTRL,				_T("$PANE_SEL8")	},
		{ 8,	'9',			MASK_CTRL,				_T("$PANE_SEL9")	},
		{ 8,	'0',			MASK_CTRL,				_T("$PANE_SEL10")	},

		{ 8,	'V',			MASK_CTRL,				_T("$EDIT_PASTE")	},
		{ 8,	'V',			MASK_ALT,				_T("$CLIPBOARD")	},

		{ 9,	'2',			MASK_CTRL | MASK_SHIFT,	_T("\\000")			},	// NUL
		{ 9,	'3',			MASK_CTRL | MASK_SHIFT,	_T("\\033")			},	// ESC
		{ 9,	'4',			MASK_CTRL | MASK_SHIFT,	_T("\\034")			},	// FS
		{ 9,	'5',			MASK_CTRL | MASK_SHIFT,	_T("\\035")			},	// GS
		{ 9,	'6',			MASK_CTRL | MASK_SHIFT,	_T("\\036")			},	// RS
		{ 9,	'7',			MASK_CTRL | MASK_SHIFT,	_T("\\037")			},	// US
		{ 9,	'8',			MASK_CTRL | MASK_SHIFT,	_T("\\177")			},	// DEL
		{ 9,	VK_DELETE,		MASK_CTRL,				_T("\\030")			},	// CAN

		{ 9,	VK_OEM_4,		MASK_CTRL,				_T("\\033")			},	// $DB([)
		{ 9,	VK_OEM_5,		MASK_CTRL,				_T("\\034")			},	// $DC(\)
		{ 9,	VK_OEM_6,		MASK_CTRL,				_T("\\035")			},	// $DD(])
		{ 9,	VK_OEM_7,		MASK_CTRL,				_T("\\036")			},	// $DE(^)
		{ 9,	VK_OEM_2,		MASK_CTRL,				_T("\\037")			},	// $BF(/)
		{ 9,	VK_OEM_3,		MASK_CTRL,				_T("\\000")			},	// $C0(@)
		{ 9,	VK_SPACE,		MASK_CTRL,				_T("\\000")			},	// SPACE

		{ 9,	'3',			MASK_CKM | MASK_CTRL | MASK_SHIFT,	_T("\\033O[")		},
		{ 9,	VK_OEM_4,		MASK_CKM | MASK_CTRL,				_T("\\033O[")		},	// $DB([)

		{ 0,	(-1),			(-1),					NULL },
	};

CKeyNodeTab::CKeyNodeTab()
{
	m_CmdsInit = FALSE;
	m_pSection = _T("KeyTab");
	Init();
}
void CKeyNodeTab::Init()
{
	int n;

	m_Node.RemoveAll();
	for ( n = 0 ; InitKeyTab[n].maps != NULL ; n++ )
		Add(InitKeyTab[n].code, InitKeyTab[n].mask, InitKeyTab[n].maps);
}
static int KeyNodeComp(const void *src, const void *dis)
{
	int c;

	if ( (c = (((CKeyNode *)src)->m_Code - ((CKeyNode *)dis)->m_Code)) == 0 )
		  c = (((CKeyNode *)src)->m_Mask - ((CKeyNode *)dis)->m_Mask);

	return c;
}
BOOL CKeyNodeTab::Find(int code, int mask, int *base)
{
	CKeyNode tmp;

	tmp.m_Code = code;
	tmp.m_Mask = mask;

	return BinaryFind(&tmp, m_Node.GetData(), sizeof(CKeyNode), (int)m_Node.GetSize(), KeyNodeComp, base);
}
int CKeyNodeTab::Add(CKeyNode &node)
{
	int n;

	if ( Find(node.m_Code, node.m_Mask, &n) )
		m_Node[n] = node;
	else
		m_Node.InsertAt(n, node);

	m_CmdsInit = FALSE;
	return n;
}
int CKeyNodeTab::Add(LPCTSTR code, int mask, LPCTSTR maps)
{
	CKeyNode tmp;
	tmp.SetCode(code);
	tmp.m_Mask = mask;
	tmp.SetMaps(maps);
	return Add(tmp);
}
int CKeyNodeTab::Add(int code, int mask, LPCTSTR str)
{
	CKeyNode tmp;
	tmp.m_Code = code;
	tmp.m_Mask = mask;
	tmp.SetMaps(str);
	return Add(tmp);
}
BOOL CKeyNodeTab::FindKeys(int code, int mask, CBuffer *pBuf, int base, int bits)
{
	if ( (mask & bits & MASK_VT52) != 0 ) {
		if ( FindKeys(code, mask, pBuf, base, bits & ~MASK_VT52) )
			return TRUE;
		if ( FindKeys(code, mask & ~MASK_VT52, pBuf, base, bits & ~MASK_VT52) )
			return TRUE;
	} else if ( (mask & bits & MASK_CKM) != 0 ) {
		if ( FindKeys(code, mask, pBuf, base, bits & ~MASK_CKM) )
			return TRUE;
		if ( FindKeys(code, mask & ~MASK_CKM, pBuf, base, bits & ~MASK_CKM) )
			return TRUE;
	} else if ( (mask & bits & MASK_APPL) != 0 ) {
		if ( FindKeys(code, mask, pBuf, base, bits & ~MASK_APPL) )
			return TRUE;
		if ( FindKeys(code, mask & ~MASK_APPL, pBuf, base, bits & ~MASK_APPL) )
			return TRUE;
	} else if ( (mask & bits & MASK_SHIFT) != 0 ) {
		if ( FindKeys(code, mask, pBuf, base, bits & ~MASK_SHIFT) )
			return TRUE;
		if ( FindKeys(code, mask & ~MASK_SHIFT, pBuf, base, bits & ~MASK_SHIFT) )
			return TRUE;
	} else {
		for ( int i = base ; i < m_Node.GetSize() && m_Node[i].m_Code == code ; i++ ) {
			if ( m_Node[i].m_Mask == mask ) {
				*pBuf = m_Node[i].m_Maps;
				return TRUE;
			}
		}
	}
	return FALSE;
}
BOOL CKeyNodeTab::FindMaps(int code, int mask, CBuffer *pBuf)
{
	int n;

	if ( Find(code, mask, &n) ) {
		*pBuf = m_Node[n].m_Maps;
		return TRUE;
	}

	while ( n > 0 && m_Node[n - 1].m_Code == code )
		n--;

	if ( n >= GetSize() || m_Node[n].m_Code != code )
		return FALSE;

	return FindKeys(code, mask, pBuf, n, MASK_VT52 | MASK_CKM | MASK_APPL | MASK_SHIFT);
}

#define	CAPINFOKEYMAX	76
static const struct _CapInfoKeyTab {
		LPCTSTR name;
		int	code;
		int mask;
	} CapInfoKeyTab[] = {
		{	_T("#2"),		VK_HOME,	MASK_SHIFT,				},
		{	_T("#4"),		VK_LEFT,	MASK_CKM | MASK_SHIFT,	},
		{	_T("%c"),		VK_NEXT,	MASK_SHIFT,				},
		{	_T("%e"),		VK_PRIOR,	MASK_SHIFT,				},
		{	_T("%i"),		VK_RIGHT,	MASK_CKM | MASK_SHIFT,	},
		{	_T("*7"),		VK_END, 	MASK_SHIFT,				},
		{	_T("@7"),		VK_END, 	0,						},
		{	_T("F1"),		VK_F11, 	0,						},
		{	_T("F2"),		VK_F12, 	0,						},
		{	_T("F3"),		VK_F13, 	0,						},
		{	_T("F4"),		VK_F14, 	0,						},
		{	_T("F5"),		VK_F15, 	0,						},
		{	_T("F6"),		VK_F16, 	0,						},
		{	_T("F7"),		VK_F17, 	0,						},
		{	_T("F8"),		VK_F18, 	0,						},
		{	_T("F9"),		VK_F19, 	0,						},
		{	_T("FA"),		VK_F20, 	0,						},
		{	_T("k1"),		VK_F1,		0,						},
		{	_T("k2"),		VK_F2,		0,						},
		{	_T("k3"),		VK_F3,		0,						},
		{	_T("k4"),		VK_F4,		0,						},
		{	_T("k5"),		VK_F5,		0,						},
		{	_T("k6"),		VK_F6,		0,						},
		{	_T("k7"),		VK_F7,		0,						},
		{	_T("k8"),		VK_F8,		0,						},
		{	_T("k9"),		VK_F9,		0,						},
		{	_T("k;"),		VK_F10, 	0,						},
		{	_T("kD"),		VK_DELETE,	0,						},
		{	_T("kEND"),		VK_END, 	MASK_SHIFT,				},
		{	_T("kF"),		VK_DOWN,	MASK_CKM | MASK_SHIFT,	},
		{	_T("kHOM"),		VK_HOME,	MASK_SHIFT,				},
		{	_T("kI"),		VK_INSERT,	0,						},
		{	_T("kLFT"),		VK_LEFT,	MASK_CKM | MASK_SHIFT,	},
		{	_T("kN"),		VK_NEXT,	0,						},
		{	_T("kNXT"),		VK_NEXT,	MASK_SHIFT,				},
		{	_T("kP"),		VK_PRIOR,	0,						},
		{	_T("kPRV"),		VK_PRIOR,	MASK_SHIFT,				},
		{	_T("kR"),		VK_UP,		MASK_CKM | MASK_SHIFT,	},
		{	_T("kRIT"),		VK_RIGHT,	MASK_CKM | MASK_SHIFT,	},
		{	_T("kcub1"),	VK_LEFT,	MASK_CKM,				},
		{	_T("kcud1"),	VK_DOWN,	MASK_CKM,				},
		{	_T("kcuf1"),	VK_RIGHT,	MASK_CKM,				},
		{	_T("kcuu1"),	VK_UP,		MASK_CKM,				},
		{	_T("kd"),		VK_DOWN,	MASK_CKM,				},
		{	_T("kdch1"),	VK_DELETE,	0,						},
		{	_T("kend"),		VK_END, 	0,						},
		{	_T("kf1"),		VK_F1,		0,						},
		{	_T("kf10"),		VK_F10, 	0,						},
		{	_T("kf11"),		VK_F11, 	0,						},
		{	_T("kf12"),		VK_F12, 	0,						},
		{	_T("kf13"),		VK_F13, 	0,						},
		{	_T("kf14"),		VK_F14, 	0,						},
		{	_T("kf15"),		VK_F15, 	0,						},
		{	_T("kf16"),		VK_F16, 	0,						},
		{	_T("kf17"),		VK_F17, 	0,						},
		{	_T("kf18"),		VK_F18, 	0,						},
		{	_T("kf19"),		VK_F19, 	0,						},
		{	_T("kf2"),		VK_F2,		0,						},
		{	_T("kf20"),		VK_F20, 	0,						},
		{	_T("kf3"),		VK_F3,		0,						},
		{	_T("kf4"),		VK_F4,		0,						},
		{	_T("kf5"),		VK_F5,		0,						},
		{	_T("kf6"),		VK_F6,		0,						},
		{	_T("kf7"),		VK_F7,		0,						},
		{	_T("kf8"),		VK_F8,		0,						},
		{	_T("kf9"),		VK_F9,		0,						},
		{	_T("kh"),		VK_HOME,	0,						},
		{	_T("khome"),	VK_HOME,	0,						},
		{	_T("kich1"),	VK_INSERT,	0,						},
		{	_T("kind"),		VK_DOWN,	MASK_CKM | MASK_SHIFT,	},
		{	_T("kl"),		VK_LEFT,	MASK_CKM,				},
		{	_T("knp"),		VK_NEXT,	0,						},
		{	_T("kpp"),		VK_PRIOR,	0,						},
		{	_T("kr"),		VK_RIGHT,	MASK_CKM,				},
		{	_T("kri"),		VK_UP,		MASK_CKM | MASK_SHIFT,	},
		{	_T("ku"),		VK_UP,		MASK_CKM,				},
	};

static int CapKeyCmp(const void *src, const void *dis)
{
	return _tcscmp((LPCTSTR)src, ((struct _CapInfoKeyTab *)dis)->name);
}
BOOL CKeyNodeTab::FindCapInfo(LPCTSTR name, CBuffer &buf)
{
	int n, b;

	if ( BinaryFind((void *)name, (void *)CapInfoKeyTab, sizeof(struct _CapInfoKeyTab), CAPINFOKEYMAX, CapKeyCmp, &n) ) {
		if ( !Find(CapInfoKeyTab[n].code, CapInfoKeyTab[n].mask, &b) )
			return FALSE;
		buf = m_Node[b].m_Maps;
		return (*((WCHAR *)(buf.GetPtr())) == L'$' ? FALSE : TRUE);
	}
	return FALSE;
}
void CKeyNodeTab::SetArray(CStringArrayExt &stra)
{
	int n;
	CStringArrayExt tmp;

	stra.RemoveAll();
	for ( n = 0 ; n < m_Node.GetSize() ; n++ ) {
		if ( m_Node[n].m_Code == (-1) )
			continue;
		tmp.RemoveAll();
		tmp.AddVal(m_Node[n].m_Code);
		tmp.AddVal(m_Node[n].m_Mask);
		tmp.Add(m_Node[n].GetMaps());
		stra.AddArray(tmp);
	}

	tmp.RemoveAll();
	tmp.AddVal(-1);
	tmp.AddVal(9);			// KeyCode Bug Fix
	stra.AddArray(tmp);
}
void CKeyNodeTab::GetArray(CStringArrayExt &stra)
{
	int n, fix = 0;
	CStringArrayExt tmp;

	m_Node.RemoveAll();
	for ( n = 0 ; n < stra.GetSize() ; n++ ) {
		stra.GetArray(n, tmp);
		if ( tmp.GetSize() < 3 ) {
			if ( tmp.GetVal(0) == (-1) )
				fix = tmp.GetVal(1);
			continue;
		}
		Add(tmp.GetVal(0), tmp.GetVal(1), tmp.GetAt(2));
	}
	BugFix(fix);
}
void CKeyNodeTab::SetIndex(int mode, CStringIndex &index)
{
	int n, i, m;
	CString str;
	CStringIndex *ip;
	static const LPCTSTR menbaName[] = { _T("Table"), _T("Add"), NULL };

	if ( mode ) {		// Write
		for ( n = 0 ; n < m_Node.GetSize() ; n++ ) {
			if ( m_Node[n].m_Code == (-1) )
				continue;
			str.Format(_T("%d"), n);
			ip = &(index[menbaName[0]][str]);

			ip->Add(m_Node[n].m_Code);
			ip->Add(m_Node[n].m_Mask);
			ip->Add(m_Node[n].GetMaps());
		}

	} else {			// Read
		for ( m = 0 ; menbaName[m] != NULL ; m++ ) {
			if ( (n = index.Find(menbaName[m])) < 0 )
				continue;

			if ( m == 0 )
				m_Node.RemoveAll();

			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				if ( index[n][i].GetSize() < 3 )
					continue;
				Add((int)index[n][i][0], (int)index[n][i][1], (LPCTSTR)index[n][i][2]);
			}
		}
	}
}
void CKeyNodeTab::ScriptInit(int cmds, int shift, class CScriptValue &value)
{
	value.m_DocCmds = cmds;

	value["Add" ].m_DocCmds = (1 << shift) | cmds;
	value["Find"].m_DocCmds = (2 << shift) | cmds;
}
void CKeyNodeTab::ScriptValue(int cmds, class CScriptValue &value, int mode)
{
	int n;

	if ( mode == DOC_MODE_CALL ) {
		switch(cmds) {
		case 1:				// Document.KeyCode.Add(code, mask, maps)
			if ( value.GetSize() >= 3 ) {
				value = (int)0;
				Add((int)value[0], (int)value[1], (LPCTSTR)value[2]);
			} else if ( value[0].Find("Code") >= 0 && value[0].Find("Mask") >= 0 && value[0].Find("Maps") >= 0 ) {
				value = (int)0;
				Add((int)value[0]["Code"], (int)value[0]["Mask"], (LPCTSTR)value[0]["Maps"]);
			} else
				value = (int)1;
			break;
		case 2:				// Document.KeyCode.Find(code, mask)
			if ( Find((int)value[0], (int)(value[1]), &n) ) {
				value = (int)0;
				value["Code"] = (int)(m_Node[n].m_Code);
				value["Mask"] = (int)(m_Node[n].m_Mask);
				value["Maps"] = (LPCTSTR)m_Node[n].GetMaps();
			} else
				value = (int)1;
			break;
		}

	} else if ( mode == DOC_MODE_IDENT && cmds == 0 ) {			// Document.KeyCode
		value.RemoveAll();
		for ( n = 0 ; n < m_Node.GetSize() ; n++ ) {
			value[n]["Code"] = (int)m_Node[n].m_Code;
			value[n]["Mask"] = (int)m_Node[n].m_Mask;
			value[n]["Maps"] = m_Node[n].GetMaps();
		}

	} else if ( mode == DOC_MODE_SAVE && cmds == 0 ) {			// Document.KeyCode
		for ( n = 0 ; n < value.GetSize() ; n++ )
			Add((int)value[n]["Code"], (int)value[n]["Mask"], (LPCTSTR)value[n]["Maps"]);
	}
}
const CKeyNodeTab & CKeyNodeTab::operator = (CKeyNodeTab &data)
{
	m_Node.RemoveAll();
	for ( int n = 0 ; n < data.m_Node.GetSize() ; n++ )
		m_Node.Add(data.m_Node[n]);
	m_CmdsInit = FALSE;
	return *this;
}

#define	CMDSKEYTABMAX	123
static const struct _CmdsKeyTab {
	int	code;
	LPCWSTR name;
} CmdsKeyTab[] = {
	{	ID_APP_ABOUT,				L"$ABOUT"			},
	{	ID_SEND_BREAK,				L"$BREAK"			},
	{	IDM_BROADCAST,				L"$BROADCAST"		},
	{	IDM_CLIPBOARD_MENU,			L"$CLIPBOARD"		},
	{	IDM_CLIPBOARD_HIS1,			L"$CLIPBOARD_HIS1"	},
	{	IDM_CLIPBOARD_HIS10,		L"$CLIPBOARD_HIS10"	},
	{	IDM_CLIPBOARD_HIS2,			L"$CLIPBOARD_HIS2"	},
	{	IDM_CLIPBOARD_HIS3,			L"$CLIPBOARD_HIS3"	},
	{	IDM_CLIPBOARD_HIS4,			L"$CLIPBOARD_HIS4"	},
	{	IDM_CLIPBOARD_HIS5,			L"$CLIPBOARD_HIS5"	},
	{	IDM_CLIPBOARD_HIS6,			L"$CLIPBOARD_HIS6"	},
	{	IDM_CLIPBOARD_HIS7,			L"$CLIPBOARD_HIS7"	},
	{	IDM_CLIPBOARD_HIS8,			L"$CLIPBOARD_HIS8"	},
	{	IDM_CLIPBOARD_HIS9,			L"$CLIPBOARD_HIS9"	},
	{	IDM_DIALOGFONT,				L"$DIALOG_FONT"		},
	{	ID_EDIT_COPY,				L"$EDIT_COPY"		},
	{	ID_EDIT_COPY_ALL,			L"$EDIT_COPYALL"	},
	{	ID_EDIT_PASTE,				L"$EDIT_PASTE"		},
	{	ID_APP_EXIT,				L"$EXIT"			},
	{	ID_FILE_ALL_SAVE,			L"$FILE_ALLSAVE"	},
	{	ID_FILE_CLOSE,				L"$FILE_CLOSE"		},
	{	ID_FILE_NEW,				L"$FILE_NEW"		},
	{	ID_FILE_OPEN,				L"$FILE_OPEN"		},
	{	ID_FILE_SAVE_AS,			L"$FILE_SAVE"		},
	{	IDM_KANJI_ASCII,			L"$KANJI_ASCII"		},
	{	IDM_KANJI_EUC,				L"$KANJI_EUC"		},
	{	IDM_KANJI_SJIS,				L"$KANJI_SJIS"		},
	{	IDM_KANJI_UTF8,				L"$KANJI_UTF8"		},
	{	IDM_KERMIT_DOWNLOAD,		L"$KERMIT_DOWNLOAD"	},
	{	IDM_KERMIT_UPLOAD,			L"$KERMIT_UPLOAD"	},
	{	ID_MACRO_HIS1,				L"$KEY_HIS1"		},
	{	ID_MACRO_HIS2,				L"$KEY_HIS2"		},
	{	ID_MACRO_HIS3,				L"$KEY_HIS3"		},
	{	ID_MACRO_HIS4,				L"$KEY_HIS4"		},
	{	ID_MACRO_HIS5,				L"$KEY_HIS5"		},
	{	ID_MACRO_PLAY,				L"$KEY_PLAY"		},
	{	ID_MACRO_REC,				L"$KEY_REC"			},
	{	ID_LOG_OPEN,				L"$LOG_OPEN"		},
	{	IDM_LOOKCAST,				L"$LOOKCAST"		},
	{	ID_MOUSE_EVENT,				L"$MOUSE_EVENT"		},
	{	ID_PAGE_NEXT,				L"$NEXT"			},
	{	IDC_LOADDEFAULT,			L"$OPTION_LOAD"		},
	{	ID_SETOPTION,				L"$OPTION_SET"		},
	{	IDM_OTHERCAST,				L"$OTHERCAST"		},
	{	ID_WINDOW_CASCADE,			L"$PANE_CASCADE"	},
	{	ID_PANE_DELETE,				L"$PANE_DELETE"		},
	{	IDM_MOVEPANE_DOWN,			L"$PANE_DOWN"		},
	{	ID_PANE_HSPLIT,				L"$PANE_HSPLIT"		},
	{	IDM_MOVEPANE_LEFT,			L"$PANE_LEFT"		},
	{	IDM_WINODW_NEXT,			L"$PANE_NEXT"		},
	{	IDM_WINDOW_PREV,			L"$PANE_PREV"		},
	{	IDM_MOVEPANE_RIGHT,			L"$PANE_RIGHT"		},
	{	ID_WINDOW_ROTATION,			L"$PANE_ROTATION"	},
	{	ID_PANE_SAVE,				L"$PANE_SAVE"		},
	{	AFX_IDM_FIRST_MDICHILD + 0,	L"$PANE_SEL1"		},
	{	AFX_IDM_FIRST_MDICHILD + 9,	L"$PANE_SEL10"		},
	{	AFX_IDM_FIRST_MDICHILD + 1,	L"$PANE_SEL2"		},
	{	AFX_IDM_FIRST_MDICHILD + 2,	L"$PANE_SEL3"		},
	{	AFX_IDM_FIRST_MDICHILD + 3,	L"$PANE_SEL4"		},
	{	AFX_IDM_FIRST_MDICHILD + 4,	L"$PANE_SEL5"		},
	{	AFX_IDM_FIRST_MDICHILD + 5,	L"$PANE_SEL6"		},
	{	AFX_IDM_FIRST_MDICHILD + 6,	L"$PANE_SEL7"		},
	{	AFX_IDM_FIRST_MDICHILD + 7,	L"$PANE_SEL8"		},
	{	AFX_IDM_FIRST_MDICHILD + 8,	L"$PANE_SEL9"		},
	{	ID_WINDOW_TILE_HORZ,		L"$PANE_TILEHORZ"	},
	{	IDM_MOVEPANE_UP,			L"$PANE_UP"			},
	{	ID_PANE_WSPLIT,				L"$PANE_WSPLIT"		},
	{	IDM_PASSWORDLOCK,			L"$PASSLOCK"		},
	{	ID_FILE_PRINT,				L"$PRINT_DIRECT"	},
	{	ID_FILE_PRINT_PREVIEW,		L"$PRINT_PREVIEW"	},
	{	ID_FILE_PRINT_SETUP,		L"$PRINT_SETUP"		},
	{	ID_PAGE_PRIOR,				L"$PRIOR"			},
	{	IDM_RESET_ALL,				L"$RESET_ALL"		},
	{	IDM_RESET_ATTR,				L"$RESET_ATTR"		},
	{	IDM_RESET_BANK,				L"$RESET_BANK"		},
	{	IDM_RESET_ESC,				L"$RESET_ESC"		},
	{	IDM_RESET_MOUSE,			L"$RESET_MOUSE"		},
	{	IDM_RESET_SCREEN,			L"$RESET_SCREEN"	},
	{	IDM_RESET_SIZE,				L"$RESET_SIZE"		},
	{	IDM_RESET_TAB,				L"$RESET_TAB"		},
	{	IDM_RESET_TEK,				L"$RESET_TEK"		},
	{	IDM_SAVERESFILE,			L"$RESOURCE_SAVE"	},
	{	ID_CHARSCRIPT_END,			L"$SCRIPT_END"		},
	{	IDM_SCRIPT,					L"$SCRIPT_EXEC"		},
	{	IDM_SCRIPT_MENU1,			L"$SCRIPT_MENU1"	},
	{	IDM_SCRIPT_MENU10,			L"$SCRIPT_MENU10"	},
	{	IDM_SCRIPT_MENU2,			L"$SCRIPT_MENU2"	},
	{	IDM_SCRIPT_MENU3,			L"$SCRIPT_MENU3"	},
	{	IDM_SCRIPT_MENU4,			L"$SCRIPT_MENU4"	},
	{	IDM_SCRIPT_MENU5,			L"$SCRIPT_MENU5"	},
	{	IDM_SCRIPT_MENU6,			L"$SCRIPT_MENU6"	},
	{	IDM_SCRIPT_MENU7,			L"$SCRIPT_MENU7"	},
	{	IDM_SCRIPT_MENU8,			L"$SCRIPT_MENU8"	},
	{	IDM_SCRIPT_MENU9,			L"$SCRIPT_MENU9"	},
	{	IDM_SEARCH_BACK,			L"$SEARCH_BACK"		},
	{	IDM_SEARCH_NEXT,			L"$SEARCH_NEXT"		},
	{	IDM_SEARCH_REG,				L"$SEARCH_REG"		},
	{	ID_SPLIT_HEIGHT,			L"$SPLIT_HEIGHT"	},
	{	IDM_SPLIT_HEIGHT_NEW,		L"$SPLIT_HEIGHTNEW"	},
	{	ID_SPLIT_OVER,				L"$SPLIT_OVER"		},
	{	ID_SPLIT_WIDTH,				L"$SPLIT_WIDTH"		},
	{	IDM_SPLIT_WIDTH_NEW,		L"$SPLIT_WIDTHNEW"	},
	{	IDM_VERSIONCHECK,			L"$VERSION_CHECK"	},
	{	IDM_IMAGEDISP,				L"$VIEW_IMAGEDISP"	},
	{	ID_GOZIVIEW,				L"$VIEW_JOKE"		},
	{	ID_VIEW_MENUBAR,			L"$VIEW_MENUBAR"	},
	{	ID_VIEW_SCROLLBAR,			L"$VIEW_SCROLLBAR"	},
	{	IDM_SFTP,					L"$VIEW_SFTP"		},
	{	IDM_SOCKETSTATUS,			L"$VIEW_SOCKET"		},
	{	ID_VIEW_STATUS_BAR,			L"$VIEW_STATUSBAR"	},
	{	ID_VIEW_TABBAR,				L"$VIEW_TABBAR"		},
	{	IDM_TEKDISP,				L"$VIEW_TEKDISP"	},
	{	ID_VIEW_TOOLBAR,			L"$VIEW_TOOLBAR"	},
	{	IDM_TRACEDISP,				L"$VIEW_TRACEDISP"	},
	{	ID_WINDOW_CLOSE,			L"$WINDOW_CLOSE"	},
	{	IDM_DISPWINIDX,				L"$WINDOW_INDEX"	},
	{	ID_WINDOW_NEW,				L"$WINDOW_NEW"		},
	{	IDM_XMODEM_DOWNLOAD,		L"$XMODEM_DOWNLOAD"	},
	{	IDM_XMODEM_UPLOAD,			L"$XMODEM_UPLOAD"	},
	{	IDM_YMODEM_DOWNLOAD,		L"$YMODEM_DOWNLOAD"	},
	{	IDM_YMODEM_UPLOAD,			L"$YMODEM_UPLOAD"	},
	{	IDM_ZMODEM_DOWNLOAD,		L"$ZMODEM_DOWNLOAD"	},
	{	IDM_ZMODEM_UPLOAD,			L"$ZMODEM_UPLOAD"	},
};

void CKeyNodeTab::CmdsInit()
{
	int n, i, id;
	CKeyCmds tmp;
	CString str;

	if ( m_CmdsInit )
		return;

	m_Cmds.RemoveAll();
	for ( n = 0 ; n < GetSize() ; n++ ) {
		if ( (id = GetCmdsKey(GetAt(n).m_Maps)) <= 0 || id == ID_PAGE_NEXT || id == ID_PAGE_PRIOR )
			continue;
		str = GetAt(n).GetMask();
		if ( !str.IsEmpty() )
			str += "+";
		str += GetAt(n).GetCode();
		if ( (i = m_Cmds.Find(id)) >= 0 ) {
			m_Cmds[i].m_Menu += _T(",");
			m_Cmds[i].m_Menu += str;
		} else {
			tmp.m_Id = id;
			tmp.m_Menu = str;
			tmp.m_Flag = FALSE;
			tmp.m_Text.Empty();
			m_Cmds.Add(tmp);
		}
	}

	m_CmdsInit = TRUE;
}
static int CmdsKeyCmp(const void *src, const void *dis)
{
	return wcscmp((const wchar_t *)src, ((struct _CmdsKeyTab *)dis)->name);
}
int CKeyNodeTab::GetCmdsKey(LPCWSTR str)
{
	int n;

	if ( str[0] != L'$' )
		return (-1);

	if ( BinaryFind((void *)str, (void *)CmdsKeyTab, sizeof(struct _CmdsKeyTab), CMDSKEYTABMAX, CmdsKeyCmp, &n) )
		return CmdsKeyTab[n].code;

	return (-1);
}
void CKeyNodeTab::SetComboList(CComboBox *pCombo)
{
	int n;
	CString str;

	if ( pCombo == NULL )
		return;

	for ( n = pCombo->GetCount() - 1 ; n >= 0; n-- )
		pCombo->DeleteString(n);

	for ( n = 0 ; n < CMDSKEYTABMAX ; n++ ) {
		str = CmdsKeyTab[n].name;
		pCombo->AddString(str);
	}
}
LPCTSTR CKeyNodeTab::GetCmdsStr(int code)
{
	int n;

	for ( n = 0 ; n < CMDSKEYTABMAX ; n++ ) {
		if ( CmdsKeyTab[n].code == code )
			return CmdsKeyTab[n].name;
	}
	return NULL;
}
void CKeyNodeTab::BugFix(int fix)
{
	int i, n;
	CStringBinary list;

	if ( fix < 1 ) {
		for ( n = 0 ; n < m_Node.GetSize() ; n++ ) {
			if ( m_Node[n].m_Code == VK_UP || m_Node[n].m_Code == VK_DOWN || m_Node[n].m_Code == VK_RIGHT || m_Node[n].m_Code == VK_LEFT ) {
				if ( m_Node[n].m_Mask == MASK_APPL )
					m_Node[n].m_Mask = MASK_CKM;
			} else if ( m_Node[n].m_Code == VK_END && m_Node[n].m_Mask == 0 )
				m_Node[n].SetMaps(_T("\\033[F"));
		}
	}

	if ( fix < 2 ) {
		for ( i = 0 ; InitKeyTab[i].maps != NULL ; i++ ) {
			if ( !Find(InitKeyTab[i].code, InitKeyTab[i].mask, &n) ) {
				for ( n = 0 ; n < m_Node.GetSize() ; n++ ) {
					if ( _tcscmp(m_Node[n].GetMaps(), InitKeyTab[i].maps) == 0 )
						break;
				}
				if ( n >= m_Node.GetSize() )
					Add(InitKeyTab[i].code, InitKeyTab[i].mask, InitKeyTab[i].maps);
			}
		}
	}

	if ( fix < 3 ) {
		for ( i = 0 ; InitKeyTab[i].maps != NULL ; i++ ) {
			if ( InitKeyTab[i].code < VK_NUMPAD0 || InitKeyTab[i].code > VK_DIVIDE )
				continue;
			if ( Find(InitKeyTab[i].code, InitKeyTab[i].mask, &n) )
				continue;
			Add(InitKeyTab[i].code, InitKeyTab[i].mask, InitKeyTab[i].maps);
		}
	}

	if ( fix < 4 ) {
		for ( i = 0 ; i < m_Node.GetSize() ; i++ ) {
			if ( GetCmdsKey((LPCWSTR)(m_Node[i].m_Maps)) <= 0 ) {
				m_Node.RemoveAt(i);
				i--;
			}
		}
		for ( i = 0 ; InitKeyTab[i].maps != NULL ; i++ ) {
			if ( Find(InitKeyTab[i].code, InitKeyTab[i].mask, &n) )
				continue;
			Add(InitKeyTab[i].code, InitKeyTab[i].mask, InitKeyTab[i].maps);
		}
	}

	if ( fix < 5 ) {
		for ( i = 0 ; InitKeyTab[i].maps != NULL ; i++ ) {
			if ( InitKeyTab[i].code != VK_TAB || (InitKeyTab[i].mask & MASK_CTRL) == 0 )
				continue;
			if ( Find(InitKeyTab[i].code, InitKeyTab[i].mask, &n) )
				continue;
			Add(InitKeyTab[i].code, InitKeyTab[i].mask, InitKeyTab[i].maps);
		}
	}

	//	$CLIPBORAD -> $CLIPBOARD
	if ( fix < 8 ) {
		for ( n = 0 ; n < GetSize() ; n++ ) {
			if ( wcsncmp(GetAt(n).m_Maps, L"$CLIPBORAD", 10) == 0 ) {
				LPWSTR p = (LPWSTR)GetAt(n).m_Maps.GetPtr();
				p[7] = L'A';
				p[8] = L'R';
			}
		}
	}

	for ( n = 0 ; n < GetSize() ; n++ )
		list[GetAt(n).GetMaps()] = 1;

	for ( i = 0 ; InitKeyTab[i].maps != NULL ; i++ ) {
		if ( fix >= InitKeyTab[i].fix  )
			continue;
		if ( Find(InitKeyTab[i].code, InitKeyTab[i].mask, &n) )
			continue;
		if ( InitKeyTab[i].maps[0] == _T('$') && list.Find(InitKeyTab[i].maps) != NULL )
			continue;
		Add(InitKeyTab[i].code, InitKeyTab[i].mask, InitKeyTab[i].maps);
	}
}
int CKeyNodeTab::GetDecKeyToCode(int code)
{
	static const BYTE DecKeyCode[] = {
		0,					// 0	
		VK_KANJI,			// 1	半角/全角
		'1',				// 2	1
		'2',				// 3	2
		'3',				// 4	3
		'4',				// 5	4
		'5',				// 6	5
		'6',				// 7	6
		'7',				// 8	7
		'8',				// 9	8
		'9',				// 10	9
		'0',				// 11	0
		VK_OEM_MINUS,		// 12	-
		VK_OEM_7,			// 13	^
		VK_OEM_5,			// 14	|
		VK_BACK,			// 15	BS
		VK_TAB,				// 16	TAB
		'Q',				// 17	Q
		'W',				// 18	W
		'E',				// 19	E
		'R',				// 20	R
		'T',				// 21	T
		'Y',				// 22	Y
		'U',				// 23	U
		'I',				// 24	I
		'O',				// 25	O
		'P',				// 26	P
		VK_OEM_3,			// 27	@
		VK_OEM_4,			// 28	[
		0,					// 29	
		0,					// 30	CAPSLOCK
		'A',				// 31	A
		'S',				// 32	S
		'D',				// 33	D
		'F',				// 34	F
		'G',				// 35	G
		'H',				// 36	H
		'J',				// 37	J
		'K',				// 38	K
		'L',				// 39	L
		VK_OEM_PLUS,		// 40	+
		VK_OEM_1,			// 41	*
		VK_OEM_6,			// 42	]
		VK_RETURN,			// 43	ENTER
		0,					// 44	SHIFT-L
		0,					// 45	
		'Z',				// 46	Z
		'X',				// 47	X
		'C',				// 48	C
		'V',				// 49	V
		'B',				// 50	B
		'N',				// 51	N
		'M',				// 52	M
		VK_OEM_COMMA,		// 53	,
		VK_OEM_PERIOD,		// 54	.
		VK_OEM_2,			// 55	/
		VK_OEM_102,			// 56	_
		0,					// 57	SHIFT-R
		0,					// 58	CTRL-L
		0,					// 59	
		0,					// 60	ALT-L
		VK_SPACE,			// 61	SPC
		0,					// 62	ALT-R
		0,					// 63	
		0,					// 64	CTRL-R
		VK_NONCONVERT,		// 65	無変換
		VK_CONVERT,			// 66	変換
		0,					// 67	カタカナ
		0,					// 68	
		0,					// 69	
		0,					// 70	
		0,					// 71	
		0,					// 72	
		0,					// 73	
		0,					// 74	
		VK_INSERT,			// 75	INS
		VK_DELETE,			// 76	DEL
		0,					// 77	
		0,					// 78	
		VK_LEFT,			// 79	LEFT
		VK_HOME,			// 80	HOME
		VK_END,				// 81	END
		0,					// 82	
		VK_UP,				// 83	UP
		VK_DOWN,			// 84	DOWN
		VK_PRIOR,			// 85	PAGEUP
		VK_NEXT,			// 86	PAGEDOWN
		0,					// 87	
		0,					// 88	
		VK_RIGHT,			// 89	RIGHT
		VK_NUMLOCK,			// 90	PAD NUMLOCK
		VK_NUMPAD7,			// 91	PAD 7
		VK_NUMPAD4,			// 92	PAD 4
		VK_NUMPAD1,			// 93	PAD 1
		VK_NUMPAD0,			// 94	PAD 0
		VK_DIVIDE,			// 95	PAD /
		VK_NUMPAD8,			// 96	PAD 8
		VK_NUMPAD5,			// 97	PAD 5
		VK_NUMPAD2,			// 98	PAD 2
		0,					// 99	
		VK_MULTIPLY,		// 100	PAD *
		VK_NUMPAD9,			// 101	PAD 9
		VK_NUMPAD6,			// 102	PAD 6
		VK_NUMPAD3,			// 103	PAD 3
		VK_DECIMAL,			// 104	PAD .
		VK_SUBTRACT,		// 105	PAD -
		VK_ADD,				// 106	PAD +
		0,					// 107	
		0,					// 108	PAD ENTER
		0,					// 109	
		VK_ESCAPE,			// 110	ESC
		0,					// 111	
		VK_F1,				// 112	F1
		VK_F2,				// 113	F2
		VK_F3,				// 114	F3
		VK_F4,				// 115	F4
		VK_F5,				// 116	F5
		VK_F6,				// 117	F6
		VK_F7,				// 118	F7
		VK_F8,				// 119	F8
		VK_F9,				// 120	F9
		VK_F10,				// 121	F10
		VK_F11,				// 122	F11
		VK_F12,				// 123	F12
		VK_PRINT,			// 124	PRINT
		VK_SCROLL,			// 125	SCROLL
		VK_PAUSE,			// 126	PAUSE
		0,					// 127	
		0,					// 128	
		0,					// 129	
	};

	return (code < 130 ? DecKeyCode[code] : 0);
}

//////////////////////////////////////////////////////////////////////
// CKeyMac

CKeyMac::CKeyMac()
{
	m_Len = 0;
	m_Data = NULL;
}
CKeyMac::~CKeyMac()
{
	if ( m_Data != NULL )
		delete [] m_Data;
}
void CKeyMac::GetMenuStr(CString &str)
{
	int n;
	int len;
	LPCWSTR p;

	str = _T("");
	if ( m_Data == NULL )
		return;

	p = (LPCWSTR)m_Data;
	len = m_Len / sizeof(WCHAR);

	for ( n = 0 ; n < 20 && n < len ; n++, p++ ) {
		if ( *p == L'\r' )
			str += _T("↓");
		else if ( *p == L'\x7F' || *p < L' ' || *p == L'&' || *str == L'\\' )
			str += _T('.');
		else
			str += (WCHAR)*p;
	}
	if ( n < len )
		str += _T("...");
}
void CKeyMac::GetStr(CString &str)
{
	int n;
	LPWSTR p = (LPWSTR)m_Data;
	CString tmp;

	str = _T("");
	if ( p == NULL )
		return;

	for ( n = 0 ; n < m_Len ; n += 2, p++ ) {
		if ( *p >= L' ' && *p < 0x7F )
			str += (CHAR)(*p);
		else {
			tmp.Format(_T("\\x%04x"), *p);
			str += tmp;
		}
	}
}
void CKeyMac::SetStr(LPCTSTR str)
{
	int c, n;
	CBuffer tmp;

	while ( *str != _T('\0') ) {
		if ( str[0] == _T('\\') && str[1] == _T('x') ) {
			str += 2;
			for ( c = n = 0 ; n < 4 ; n++) {
				if ( *str >= _T('0') && *str <= _T('9') )
					c = c * 16 + (*(str++) - _T('0'));
				else if ( *str >= _T('A') && *str <= _T('F') )
					c = c * 16 + (*(str++) - _T('A') + 10);
				else if ( *str >= 'a' && *str <= _T('f') )
					c = c * 16 + (*(str++) - _T('a') + 10);
				else
					break;
			}
			tmp.PutWord(c);
		} else {
			tmp.PutWord(*(str++));
		}
	}
	SetBuf(tmp.GetPtr(), tmp.GetSize());
}
void CKeyMac::SetBuf(LPBYTE buf, int len)
{
	if ( m_Data != NULL )
		delete [] m_Data;
	m_Len = len;
	m_Data = new BYTE[len];
	memcpy(m_Data, buf, len);
}
const CKeyMac & CKeyMac::operator = (CKeyMac &data)
{
	if ( data.m_Data == NULL ) {
		if ( m_Data != NULL )
			delete [] m_Data;
		m_Data = NULL;
		m_Len = 0;
	} else
		SetBuf(data.m_Data, data.m_Len);
	return *this;
}
BOOL CKeyMac::operator == (CKeyMac &data)
{
	if ( m_Data == NULL ) {
		if ( data.m_Data == NULL )
			return TRUE;
		return FALSE;
	} else if ( data.m_Data == NULL )
		return FALSE;

	if ( m_Len != data.m_Len )
		return FALSE;

	return (memcmp(m_Data, data.m_Data, m_Len) == 0 ? TRUE : FALSE);
}

//////////////////////////////////////////////////////////////////////
// CKeyMacTab

CKeyMacTab::CKeyMacTab()
{
	m_pSection = _T("KeyMac");
	Init();
}
void CKeyMacTab::Init()
{
	m_Data.RemoveAll();
#ifdef	USE_KEYMACGLOBAL
	m_bUpdate = FALSE;
#endif
}
void CKeyMacTab::SetArray(CStringArrayExt &stra)
{
	CString tmp;

	stra.RemoveAll();
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_Data[n].GetStr(tmp);
		stra.Add(tmp);
	}
}
void CKeyMacTab::GetArray(CStringArrayExt &stra)
{
	CKeyMac tmp;

	m_Data.RemoveAll();
	for ( int n = 0 ; n < stra.GetSize() ; n++ ) {
		tmp.SetStr(stra[n]);
		m_Data.Add(tmp);
	}
}
void CKeyMacTab::SetIndex(int mode, CStringIndex &index)
{
	int n;
	CString str;
	CKeyMac tmp;

	if ( mode ) {		// Write
		for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
			m_Data[n].GetStr(str);
			index.Add(str);
		}

	} else {			// Read
		if ( index.GetSize() > 0 )
			m_Data.RemoveAll();
		for ( n = 0 ; n < index.GetSize() ; n++ ) {
			tmp.SetStr(index[n]);
			m_Data.Add(tmp);
		}
	}
}
void CKeyMacTab::ScriptInit(int cmds, int shift, class CScriptValue &value)
{
	value.m_DocCmds = cmds;

	value["Add" ].m_DocCmds = (1 << shift) | cmds;
}
void CKeyMacTab::ScriptValue(int cmds, class CScriptValue &value, int mode)
{
	int n;
	CString str;
	CKeyMac tmp;

	if ( mode == DOC_MODE_CALL ) {
		switch(cmds) {
		case 1:					// Document.KeyMacro.Add(macro)
			tmp.SetStr((LPCTSTR)value[0]);
			if ( tmp.m_Len > 0 ) {
				Add(tmp);
				value = (int)0;
			} else
				value = (int)1;
			break;
		}

	} else if ( mode == DOC_MODE_SAVE && cmds == 0 ) {	// Document.KeyMacro
		for ( n = value.GetSize() - 1 ; n >= 0 ; n-- ) {
			tmp.SetStr((LPCTSTR)value[n]);
			Add(tmp);
		}

	} else if ( mode == DOC_MODE_IDENT && cmds == 0 ) {	// Document.KeyMacro
		value.RemoveAll();
		for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
			m_Data[n].GetStr(str);
			value[n] = (LPCTSTR)str;
		}
	}
}
const CKeyMacTab & CKeyMacTab::operator = (CKeyMacTab &data)
{
	m_Data.RemoveAll();
	for ( int n = 0 ; n < data.m_Data.GetSize() ; n++ )
		m_Data.Add(data.m_Data[n]);
	return *this;
}
void CKeyMacTab::Top(int nIndex)
{
	if ( nIndex == 0 || nIndex >= m_Data.GetSize() )
		return;
	CKeyMac tmp;
	tmp = m_Data[nIndex];
	m_Data.RemoveAt(nIndex);
	m_Data.InsertAt(0, tmp);
}
void CKeyMacTab::Add(CKeyMac &tmp)
{
	int n;
	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n] == tmp ) {
			m_Data.RemoveAt(n);
			break;
		}
	}
	if ( (n = (int)m_Data.GetSize()) > 10 )
		m_Data.RemoveAt(n - 1);

	m_Data.InsertAt(0, tmp);
}
void CKeyMacTab::GetAt(int nIndex, CBuffer &buf)
{
	buf.Clear();
	if ( nIndex < 0 || nIndex >= m_Data.GetSize() )
		return;
	buf.Apend(m_Data[nIndex].GetPtr(), m_Data[nIndex].GetSize());
}
void CKeyMacTab::SetHisMenu(CMenu *pMenu)
{
	int n;
	CString str, tmp;

	if ( m_Data.GetSize() <= 0 ) {
		pMenu->AppendMenu(MF_STRING, ID_MACRO_HIS1, CStringLoad(IDS_HISRECDEFAULT));
		pMenu->EnableMenuItem(ID_MACRO_HIS1, MF_BYCOMMAND | MF_GRAYED);
		return;
	}

	for ( n = 0 ; n < 5 && n < m_Data.GetSize() ; n++ ) {
		m_Data[n].GetMenuStr(tmp);
		str.Format(_T("&%d %s"), n + 1, tmp);
		pMenu->AppendMenu(MF_STRING, ID_MACRO_HIS1 + n, str);
	}
}

//////////////////////////////////////////////////////////////////////
// CParamTab

#if	OPENSSL_VERSION_NUMBER >= 0x10001000L
#define	META_AEAD_STRING	_T("AEAD_AES_256_GCM,AEAD_AES_128_GCM,AEAD_AES_256_CCM,AEAD_AES_128_CCM,") \
							_T("aes256-gcm@openssh.com,aes128-gcm@openssh.com,")
#else
#define	META_AEAD_STRING	_T("")
#endif

#ifdef	USE_NETTLE
#define	META_NETTLE_STRING	_T("twofish256-ctr,twofish192-ctr,twofish128-ctr,") \
							_T("serpent256-ctr,serpent192-ctr,serpent128-ctr,") \
							_T("twofish256-cbc,twofish192-cbc,twofish128-cbc,") \
							_T("serpent256-cbc,serpent192-cbc,serpent128-cbc,")
#else
#define	META_NETTLE_STRING	_T("")
#endif

#ifdef	USE_CLEFIA
#define	META_CLEFIA_STRING	_T("clefia256-ctr,clefia192-ctr,clefia128-ctr,") \
							_T("clefia256-cbc,clefia192-cbc,clefia128-cbc,")
#else
#define	META_CLEFIA_STRING	_T("")
#endif

static LPCTSTR InitAlgo[12]= {
	_T("blowfish,3des,des"),
	_T("crc32"),
	_T("zlib,none"),

	_T("chacha20-poly1305@openssh.com,") \
	_T("aes256-ctr,aes192-ctr,aes128-ctr,") \
	_T("camellia256-ctr,camellia192-ctr,camellia128-ctr,") \
	_T("blowfish-ctr,cast128-ctr,idea-ctr,") \
	_T("twofish-ctr,seed-ctr@ssh.com,3des-ctr,") \
	META_AEAD_STRING \
	_T("arcfour256,arcfour128,arcfour,") \
	_T("aes256-cbc,aes192-cbc,aes128-cbc,") \
	_T("camellia256-cbc,camellia192-cbc,camellia128-cbc,") \
	_T("blowfish-cbc,cast128-cbc,idea-cbc,") \
	_T("twofish-cbc,seed-cbc@ssh.com,3des-cbc,") \
	META_NETTLE_STRING \
	META_CLEFIA_STRING \
	_T("none"),

	_T("hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,") \
	_T("hmac-sha1-etm@openssh.com,hmac-md5-etm@openssh.com,") \
	_T("umac-64-etm@openssh.com,umac-128-etm@openssh.com,") \
	_T("hmac-sha2-256,hmac-sha2-256-96,hmac-sha2-512,hmac-sha2-512-96,") \
	_T("hmac-sha1,hmac-sha1-96,hmac-md5,hmac-md5-96,") \
	_T("hmac-ripemd160,hmac-whirlpool,") \
	_T("umac-64@openssh.com,umac-128@openssh.com,") \
	_T("umac-128,umac-96,umac-64,umac-32"),

	_T("zlib@openssh.com,zlib,none"),

	_T("chacha20-poly1305@openssh.com,") \
	_T("aes256-ctr,aes192-ctr,aes128-ctr,") \
	_T("camellia256-ctr,camellia192-ctr,camellia128-ctr,") \
	_T("blowfish-ctr,cast128-ctr,idea-ctr,") \
	_T("twofish-ctr,seed-ctr@ssh.com,3des-ctr,") \
	META_AEAD_STRING \
	_T("arcfour256,arcfour128,arcfour,") \
	_T("aes256-cbc,aes192-cbc,aes128-cbc,") \
	_T("camellia256-cbc,camellia192-cbc,camellia128-cbc,") \
	_T("blowfish-cbc,cast128-cbc,idea-cbc,") \
	_T("twofish-cbc,seed-cbc@ssh.com,3des-cbc,") \
	META_NETTLE_STRING \
	META_CLEFIA_STRING \
	_T("none"),

	_T("hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,") \
	_T("hmac-sha1-etm@openssh.com,hmac-md5-etm@openssh.com,") \
	_T("umac-64-etm@openssh.com,umac-128-etm@openssh.com,") \
	_T("hmac-sha2-256,hmac-sha2-256-96,hmac-sha2-512,hmac-sha2-512-96,") \
	_T("hmac-sha1,hmac-sha1-96,hmac-md5,hmac-md5-96,") \
	_T("hmac-ripemd160,hmac-whirlpool,") \
	_T("umac-64@openssh.com,umac-128@openssh.com,") \
	_T("umac-128,umac-96,umac-64,umac-32"),

	_T("zlib@openssh.com,zlib,none"),

	_T("curve25519-sha256,curve25519-sha256@libssh.org,") \
	_T("ecdh-sha2-nistp521,ecdh-sha2-nistp384,ecdh-sha2-nistp256,") \
	_T("diffie-hellman-group-exchange-sha256,diffie-hellman-group-exchange-sha1,") \
	_T("diffie-hellman-group16-sha512,diffie-hellman-group18-sha512,") \
	_T("diffie-hellman-group14-sha256,") \
	_T("diffie-hellman-group14-sha1,diffie-hellman-group1-sha1"),

	_T("ecdsa-sha2-nistp256-cert-v01@openssh.com,ecdsa-sha2-nistp384-cert-v01@openssh.com,ecdsa-sha2-nistp521-cert-v01@openssh.com,") \
	_T("ssh-ed25519-cert-v01@openssh.com,ssh-rsa-cert-v01@openssh.com,ssh-dss-cert-v01@openssh.com,") \
	_T("ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521,") \
	_T("ssh-ed25519,rsa-sha2-512,rsa-sha2-256,ssh-rsa,ssh-dss"),

	_T("publickey,hostbased,password,keyboard-interactive"),
};

static const ttymode_node def_ttymode[] = {
	//	VINTR		VQUIT		VERASE		VKILL		VEOF		VEOL		VEOL2		VSTART
		{ 1,3 },	{ 2,28 },	{ 3,8 },	{ 4,21 },	{ 5,4 }, 	{ 6,255 },	{ 7,255 },	{ 8,17 },
	//	VSTOP		VSUSP		VDSUSP		VREPRINT	VWERASE		VLNEXT		VSTATUS		VDISCARD
		{ 9,19 },	{ 10,26 },	{ 11,25 },	{ 12,18 },	{ 13,23 },	{ 14,22 },	{ 17,20 },	{ 18,15 },
	//	IGNPAR		PARMRK		INPCK		ISTRIP		INLCR		IGNCR		ICRNL		IXON
		{ 30,0 },	{ 31,0 },	{ 32,0 },	{ 33,0 },	{ 34,0 },	{ 35,0 },	{ 36,1 },	{ 38,1 },
	//	IXANY		IXOFF		IMAXBEL
		{ 39,1 },	{ 40,0 },	{ 41,1 },
	//	ISIG		ICANON		ECHO		ECHOE		ECHOK		ECHONL		NOFLSH		TOSTOP
		{ 50,1 },	{ 51,1 },	{ 53,1 },	{ 54,1 },	{ 55,0 },	{ 56,0 },	{ 57,0 },	{ 58,0 },
	//	IEXTEN		ECHOCTL		ECHOKE		PENDIN
		{ 59,1 },	{ 60,1 },	{ 61,1 },	{ 62,1 },
	//	OPOST		ONLCR		OCRNL		ONOCR		ONLRET
		{ 70,1 },	{ 72,1 },	{ 73,0 },	{ 74,0 },	{ 75,0 },
	//	CS7			CS8			PARENB		PARODD
		{ 90,1 },	{ 91,1 },	{ 92,0 },	{ 93,0 },
	//	OSPEED			ISPEED
		{ 129,9600 },	{ 128,9600 },
		{ 0, 0 }
	};

CParamTab::CParamTab()
{
	m_pSection = _T("ParamTab");
	m_MinSize = 18;
	Init();
}
void CParamTab::Init()
{
	int n;

	for ( n = 0 ; n < 9 ; n++ )
		m_IdKeyStr[n] = _T("");

	for ( n = 0 ; n < 12 ; n++ )
		m_AlgoTab[n].GetString(InitAlgo[n], _T(','));

	m_PortFwd.RemoveAll();

	m_XDisplay  = _T(":0");
	m_ExtEnvStr = _T("");
	memset(m_OptTab, 0, sizeof(m_OptTab));
	m_Reserve.Empty();
	m_SelIPver = SEL_IPV6V4;

	m_TtyMode.RemoveAll();
	for ( n = 0 ; def_ttymode[n].opcode != 0 ; n++ )
		m_TtyMode.Add(def_ttymode[n]);

	m_RsaExt = 0;
	m_VerIdent.Empty();
}
void CParamTab::SetArray(CStringArrayExt &stra)
{
	int n;
	CIdKey key;
	CString str, fmt;

	stra.RemoveAll();

	for ( n = 0 ; n < 9 ; n++ )
		stra.Add(_T(""));

	stra.SetAt(0, _T("IdKeyList Entry"));
	m_IdKeyList.SetString(stra[1]);

	for ( n = 0 ; n < 9 ; n++ )
		stra.AddArray(m_AlgoTab[n]);

	for ( n = 0 ; n < m_PortFwd.GetSize() ; n++ )
		stra.Add(m_PortFwd[n]);
	stra.Add(_T("EndOf"));

	stra.Add(m_XDisplay);
	stra.Add(m_ExtEnvStr);
	stra.AddBin(m_OptTab, sizeof(m_OptTab));
	stra.Add(m_Reserve);

	for ( n = 9 ; n < 12 ; n++ )
		stra.AddArray(m_AlgoTab[n]);

	stra.AddVal(m_SelIPver);

	str.Empty();
	for ( n = 0 ; n < m_TtyMode.GetSize() ; n++ ) {
		fmt.Format(_T("%d=%d,"), m_TtyMode[n].opcode, m_TtyMode[n].param);
		str += fmt;
	}
	stra.Add(str);

	stra.AddVal(m_RsaExt);
	stra.Add(m_VerIdent);
}
void CParamTab::GetArray(CStringArrayExt &stra)
{
	int n, a, i = 0;
	CIdKey key;
	CRLoginApp *pApp  = (CRLoginApp *)AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CStringBinary idx;
	CStringArrayExt list, node;
	ttymode_node ttymode;

	for ( n = 0 ; n < 9 ; n++ ) {
		if ( (n % 3) == 2 )
			key.DecryptStr(m_IdKeyStr[n], stra.GetAt(i++));
		else
			m_IdKeyStr[n] = stra.GetAt(i++);
	}


	for ( n = 0 ; n < 9 ; n++ ) {
		stra.GetArray(i++, m_AlgoTab[n]);

		idx.RemoveAll();
		list.GetString(InitAlgo[n], _T(','));
		for ( a = 0 ; a < list.GetSize() ; a++ ) {
			if ( m_AlgoTab[n].Find(list[a]) < 0 )
				m_AlgoTab[n].Add(list[a]);
			idx[list[a]] = a;
		}
		for ( a = 0 ; a < m_AlgoTab[n].GetSize() ; a++ ) {
			if ( idx[m_AlgoTab[n][a]] < 0 ) {
				m_AlgoTab[n].RemoveAt(a);
				a--;
			}
		}
	}

	m_PortFwd.RemoveAll();
	while ( i < stra.GetSize() ) {
		if ( stra[i].Compare(_T("EndOf")) == 0 ) {
			i++;
			break;
		}
		list.GetString(stra[i]);
		if ( list.GetSize() >= 4 ) {
			if ( list.GetSize() == 4 ) {
				if ( list[0].Compare(_T("localhost")) == 0 )
					a = PFD_LOCAL;
				else if ( list[0].Compare(_T("socks")) == 0 )
					a = PFD_SOCKS;
				else
					a = PFD_REMOTE;
				list.AddVal(a);
				list.SetString(stra[i]);
			}
			m_PortFwd.Add(stra[i]);
		}
		i++;
	}

	m_XDisplay  = (stra.GetSize() > i ? stra.GetAt(i++) : _T(":0"));
	m_ExtEnvStr = (stra.GetSize() > i ? stra.GetAt(i++) : _T(""));

	if ( stra.GetSize() > i )
		stra.GetBin(i++, m_OptTab, sizeof(m_OptTab));
	else
		memset(m_OptTab, 0, sizeof(m_OptTab));

	m_Reserve = (stra.GetSize() > i ? stra.GetAt(i++) : _T(""));

	for ( n = 9 ; n < 12 && stra.GetSize() > i ; n++ ) {
		stra.GetArray(i++, m_AlgoTab[n]);

		idx.RemoveAll();
		list.GetString(InitAlgo[n], _T(','));
		for ( a = 0 ; a < list.GetSize() ; a++ ) {
			if ( m_AlgoTab[n].Find(list[a]) < 0 )
				m_AlgoTab[n].Add(list[a]);
			idx[list[a]] = a;
		}
		for ( a = 0 ; a < m_AlgoTab[n].GetSize() ; a++ ) {
			if ( idx[m_AlgoTab[n][a]] < 0 ) {
				m_AlgoTab[n].RemoveAt(a);
				a--;
			}
		}
	}

	if ( stra.GetSize() > i )
		m_SelIPver = stra.GetVal(i++);
	else
		m_SelIPver = SEL_IPV6V4;

	if ( stra.GetSize() > i ) {
		list.GetString(stra.GetAt(i++), _T(','));
		if ( list.GetSize() > 0 ) {
			m_TtyMode.RemoveAll();
			for ( n = 0 ; n < list.GetSize() ; n++ ) {
				node.GetString(list[n], _T('='));
				if ( node.GetSize() >= 2 && node.GetVal(0) > 0 ) {
					ttymode.opcode = node.GetVal(0);
					ttymode.param  = node.GetVal(1);
					m_TtyMode.Add(ttymode);
				}
			}
		}
	}

	if ( stra.GetSize() > i )
		m_RsaExt = stra.GetVal(i++);

	if ( stra.GetSize() > i )
		m_VerIdent = stra.GetAt(i++);

	// Fix IdKeyList
	if ( m_IdKeyStr[0].Compare(_T("IdKeyList Entry")) == 0 ) {
		m_IdKeyList.GetString(m_IdKeyStr[1]);
		for ( n = 0 ; n < 9 ; n++ )
			m_IdKeyStr[n].Empty();
	} else {
		m_IdKeyList.RemoveAll();
		for ( n = 0 ; n < 9 ; n += 3 ) {
			if ( !key.ReadPublicKey(m_IdKeyStr[n + 0]) ||
				 !key.ReadPrivateKey(m_IdKeyStr[n + 1], m_IdKeyStr[n + 2], TRUE) )
				continue;
			for ( i = 0 ; i < pMain->m_IdKeyTab.GetSize() ; i++ ) {
				if ( pMain->m_IdKeyTab.GetAt(i).Compere(&key) == 0 )
					break;
			}
			if ( i < pMain->m_IdKeyTab.GetSize() ) {
				m_IdKeyList.AddVal(pMain->m_IdKeyTab.GetAt(i).m_Uid);
				continue;
			}
			key.m_Name = "";
			key.SetPass(m_IdKeyStr[n + 2]);
			pMain->m_IdKeyTab.AddEntry(key);
			m_IdKeyList.AddVal(key.m_Uid);
		}
	}
}
void CParamTab::SetIndex(int mode, CStringIndex &index)
{
	int n, i;
	CString str;
	CStringIndex tmp, env;
	ttymode_node node;

	if ( mode ) {		// Write
		for ( n = 0 ; n < 12 ; n++ ) {
			str.Format(_T("%d"), n);
			for ( i = 0 ; i < m_AlgoTab[n].GetSize() ; i++ )
				index[_T("Algo")][str].Add(m_AlgoTab[n][i]);
		}

		for ( n = 0 ; n < m_PortFwd.GetSize() ; n++ )
			index[_T("PortFwd")].Add(m_PortFwd[n]);

		index[_T("X11")] = m_XDisplay;
		index[_T("IPv")] = m_SelIPver;

		for ( n = 0 ; n < m_IdKeyList.GetSize() ; n++ )
			index[_T("IdKeyList")].Add(m_IdKeyList.GetVal(n));

		//for ( n = 0 ; n < (32 * 8) ; n++ ) {
		//	if ( IS_ENABLE(m_OptTab, n) )
		//		index[_T("Option")].Add(n);
		//}

		env.GetString(m_ExtEnvStr);
		for ( n = 0 ; n < env.GetSize() ; n++ ) {
			if ( env[n].m_nIndex.IsEmpty() || env[n].m_String.IsEmpty() )
				continue;
			index[_T("Env")][env[n].m_nIndex].Add(env[n].m_Value);
			index[_T("Env")][env[n].m_nIndex].Add(env[n].m_String);
		}

		index[_T("TtyMode")].SetSize((int)m_TtyMode.GetSize());
		for ( n = 0 ; n < m_TtyMode.GetSize() ; n++ ) {
			index[_T("TtyMode")][n].Add(m_TtyMode[n].opcode);
			index[_T("TtyMode")][n].Add(m_TtyMode[n].param);
		}

		index[_T("RsaExt")] = m_RsaExt;
		index[_T("VerIdent")] = m_VerIdent;

	} else {			// Read
		if ( (n = index.Find(_T("Algo"))) >= 0 ) {
			for ( int a = 0 ; a < 12 ; a++ ) {
				str.Format(_T("%d"), a);
				if ( (i = index[n].Find(str)) >= 0 ) {
					m_AlgoTab[a].RemoveAll();
					for ( int b = 0 ; b < index[n][i].GetSize() ; b++ )
						m_AlgoTab[a].Add(index[n][i][b]);
				}
			}
		}

		if ( (n = index.Find(_T("PortFwd"))) >= 0 ) {
			m_PortFwd.RemoveAll();
			for ( i = 0 ; i < index[n].GetSize() ; i++ )
				m_PortFwd.Add(index[n][i]);
		}

		if ( (n = index.Find(_T("X11"))) >= 0 )
			m_XDisplay = index[n];
		if ( (n = index.Find(_T("IPv"))) >= 0 )
			m_SelIPver = index[n];

		if ( (n = index.Find(_T("IdKeyList"))) >= 0 ) {
			m_IdKeyList.RemoveAll();
			for ( i = 0 ; i < index[n].GetSize() ; i++ )
				m_IdKeyList.AddVal(index[n][i]);
		}

		//if ( (n = index.Find(_T("Option"))) >= 0 ) {
		//	memset(m_OptTab, 0, sizeof(m_OptTab));
		//	for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
		//		int b = index[n][i];
		//		m_OptTab[b / 32] |= (1 << (b % 32));
		//	}
		//}

		if ( (n = index.Find(_T("Env"))) >= 0 ) {
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				if ( index[n][i].GetSize() < 2 )
					continue;
				tmp.m_Value  = index[n][i][0];
				tmp.m_String = index[n][i][1];
				tmp.m_nIndex = index[n][i].m_nIndex;
				env.m_Array.Add(tmp);
			}
			env.SetString(m_ExtEnvStr);
		}

		if ( (n = index.Find(_T("TtyMode"))) >= 0 && index[n].GetSize() > 0 ) {
			m_TtyMode.RemoveAll();
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				node.opcode = index[n][i][0];
				node.param  = index[n][i][1];
				if ( node.opcode > 0 )
					m_TtyMode.Add(node);
			}
		}

		if ( (n = index.Find(_T("RsaExt"))) >= 0 )
			m_RsaExt = index[n];

		if ( (n = index.Find(_T("VerIdent"))) >= 0 )
			m_VerIdent = index[n];
	}
}

static const ScriptCmdsDefs DocSsh[] = {
	{	"Protocol",		1	},
	{	"PortForward",	2	},
	{	"XDisplay",		3	},
	{	"Environ",		4	},
	{	"TtyMode",		17	},
	{	"RsaExt",		18	},
	{	"VerIdent",		19	},
	{	NULL,			0	},
}, DocSshProtocol[] = {
	{	"ssh1Cip",		5	},
	{	"ssh1Mac",		6	},
	{	"ssh1Comp",		7	},
	{	"InpCip",		8	},
	{	"InpMac",		9	},
	{	"InpComp",		10	},
	{	"OutCip",		11	},
	{	"OutMac",		12	},
	{	"OutComp",		13	},
	{	"Kexs",			14	},
	{	"HostKey",		15	},
	{	"UserAuth",		16	},
	{	NULL,			0	},
};

void CParamTab::ScriptInit(int cmds, int shift, class CScriptValue &value)
{
	int n;

	value.m_DocCmds = cmds;

	for ( n = 0 ; DocSsh[n].name != NULL ; n++ )
		value[DocSsh[n].name].m_DocCmds = (DocSsh[n].cmds << shift) | cmds;

	for ( n = 0 ; DocSshProtocol[n].name != NULL ; n++ )
		value["Protocol"][DocSshProtocol[n].name].m_DocCmds = (DocSshProtocol[n].cmds << shift) | cmds;
}
void CParamTab::ScriptValue(int cmds, class CScriptValue &value, int mode)
{
	int n, i;
	CString str;
	CStringArrayExt tmp;
	CStringIndex env;

	switch(cmds & 0xFF) {
	case 0:					// Document.ssh
		if ( mode == DOC_MODE_SAVE ) {
			for ( n = 0 ; DocSsh[n].name != NULL ; n++ ) {
				if ( (i = value.Find(DocSsh[n].name)) >= 0 )
					ScriptValue(DocSsh[n].cmds, value[i], mode);
			}
		} else if ( mode == DOC_MODE_IDENT ) {
			for ( n = 0 ; DocSsh[n].name != NULL ; n++ )
				ScriptValue(DocSsh[n].cmds, value[DocSsh[n].name], mode);
		}
		break;

	case 1:					// Document.ssh.Protocol
		if ( mode == DOC_MODE_SAVE ) {
			for ( n = 0 ; DocSshProtocol[n].name != NULL ; n++ ) {
				if ( (i = value.Find(DocSshProtocol[n].name)) >= 0 )
					ScriptValue(DocSshProtocol[n].cmds, value[i], mode);
			}
		} else if ( mode == DOC_MODE_IDENT ) {
			for ( n = 0 ; DocSshProtocol[n].name != NULL ; n++ )
				ScriptValue(DocSshProtocol[n].cmds, value[DocSshProtocol[n].name], mode);
		}
		break;

	case 2:					// Document.ssh.PortForward
		if ( mode == DOC_MODE_SAVE ) {
			m_PortFwd.RemoveAll();
			for ( n = 0 ; n < value.GetSize() ; n++ ) {
				tmp.SetSize(5);
				value[n].SetNodeStr("LocalHost", tmp[0], mode);
				value[n].SetNodeStr("LocalPort", tmp[1], mode);
				value[n].SetNodeStr("RemoteHost", tmp[2], mode);
				value[n].SetNodeStr("RemotePort", tmp[3], mode);
				tmp[4].Format(_T("%d"), (int)value[n]);
				tmp.SetString(str);
				if ( !tmp[0].IsEmpty() && !tmp[1].IsEmpty() && !tmp[2].IsEmpty() && !tmp[3].IsEmpty() )
					m_PortFwd.Add(str);
			}
		} else if ( mode == DOC_MODE_IDENT ) {
			value.RemoveAll();
			for ( n = 0 ; n < m_PortFwd.GetSize() ; n++ ) {
				tmp.GetString(m_PortFwd[n]);
				if ( tmp.GetSize() < 5 )
					continue;
				value[n]["LocalHost"]  = (LPCTSTR)tmp[0];
				value[n]["LocalPort"]  = (LPCTSTR)tmp[1];
				value[n]["RemoteHost"] = (LPCTSTR)tmp[2];
				value[n]["RemotePort"] = (LPCTSTR)tmp[3];
				value[n]               = _tstoi(tmp[4]);
			}
		}
		break;
	case 3:					// Document.ssh.XDisplay
		value.SetStr(m_XDisplay, mode);
		break;
	case 4:					// Document.ssh.Environ
		if ( mode == DOC_MODE_SAVE ) {
			env.RemoveAll();
			for ( n = 0 ; n < value.GetSize() ; n++ )
				env[MbsToTstr(value[n].m_Index)].m_String = (LPCTSTR)value[n];
			env.SetString(m_ExtEnvStr);
		} else if ( mode == DOC_MODE_IDENT ) {
			env.GetString(m_ExtEnvStr);
			value.RemoveAll();
			for ( n = 0 ; n < env.GetSize() ; n++ )
				value[TstrToMbs(env[n].m_nIndex)] = (LPCTSTR)env[n].m_String;
		}
		break;

	case 5:					// Document.ssh.Protocol.ssh1Cip
	case 6:					// Document.ssh.Protocol.ssh1Mac
	case 7:					// Document.ssh.Protocol.ssh1Comp
	case 8:					// Document.ssh.Protocol.InpCip
	case 9:					// Document.ssh.Protocol.InpMac
	case 10:				// Document.ssh.Protocol.InpComp
	case 11:				// Document.ssh.Protocol.OutCip
	case 12:				// Document.ssh.Protocol.OutMac
	case 13:				// Document.ssh.Protocol.OutComp
	case 14:				// Document.ssh.Protocol.Kexs
	case 15:				// Document.ssh.Protocol.HostKey
	case 16:				// Document.ssh.Protocol.UserAuth
		value.SetArray(m_AlgoTab[cmds - 5], mode);
		break;

	case 17:				// Document.ssh.TtyMode
		if ( mode == DOC_MODE_SAVE ) {
			m_TtyMode.SetSize(value.GetSize());
			for ( n = 0 ; n < value.GetSize() ; n++ ) {
				m_TtyMode[n].opcode = (int)value[n][0];
				m_TtyMode[n].param  = (int)value[n][1];
			}
		} else if ( mode == DOC_MODE_IDENT ) {
			value.RemoveAll();
			for ( n = 0 ; n < m_TtyMode.GetSize() ; n++ ) {
				value[n][0] = (int)m_TtyMode[n].opcode;
				value[n][1] = (int)m_TtyMode[n].param;
			}
		}
		break;

	case 18:				// Document.ssh.RsaExt
		value.SetInt(m_RsaExt, mode);
		break;
	case 19:				// Document.ssh.VerIdent
		value.SetStr(m_VerIdent, mode);
		break;
	}
}
void CParamTab::GetProp(int num, CString &str, int shuffle)
{
	int n, a, b, c, mx = 0;
	int *tab;

	str = _T("");
	if ( num < 0 || num >= 12 )
		return;

	if ( shuffle ) {
		for ( mx = 0 ; mx < (int)m_AlgoTab[num].GetSize() ; mx++ ) {
			if ( m_AlgoTab[num][mx].Compare(_T("none")) == 0 )
				break;
		}
	}

	if ( mx > 1 ) {
		tab = new int[mx];
		for ( n = 0 ; n < mx ; n++ )
			tab[n] = n;
		for ( n = 0 ; n < (mx * 8) ; n++ ) {
			a = rand() % mx;
			b = rand() % mx;
			c = tab[a]; tab[a] = tab[b]; tab[b] = c;
		}
		for ( n = 0 ; n < mx ; n++ ) {
			if ( n > 0 )
				str += _T(",");
			str += m_AlgoTab[num][tab[n]];
		}
		delete [] tab;

	} else {
		for ( int i = 0 ; i < m_AlgoTab[num].GetSize() ; i++ ) {
			if ( i > 0 )
				str += _T(",");
			str += m_AlgoTab[num][i];
		}
	}
}
int CParamTab::GetPropNode(int num, int node, CString &str)
{
	str = _T("");
	if ( num < 0 || num >= 12 || node < 0 || node >= m_AlgoTab[num].GetSize() )
		return FALSE;
	str = m_AlgoTab[num][node];
	return TRUE;
}
const CParamTab & CParamTab::operator = (CParamTab &data)
{
	int n;

	for ( n = 0 ; n < 9 ; n++ )
		m_IdKeyStr[n] = data.m_IdKeyStr[n];
	for ( n = 0 ; n < 12 ; n++ )
		m_AlgoTab[n]  = data.m_AlgoTab[n];
	m_PortFwd   = data.m_PortFwd;
	m_IdKeyList = data.m_IdKeyList;
	m_XDisplay  = data.m_XDisplay;
	m_ExtEnvStr = data.m_ExtEnvStr;
	memcpy(m_OptTab, data.m_OptTab, sizeof(m_OptTab));
	m_Reserve   = data.m_Reserve;
	m_SelIPver  = data.m_SelIPver;
	m_TtyMode.SetSize(data.m_TtyMode.GetSize());
	for ( n = 0 ; n < data.m_TtyMode.GetSize() ; n++ )
		m_TtyMode[n] = data.m_TtyMode[n];
	m_RsaExt = data.m_RsaExt;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CStringMaps

CStringMaps::CStringMaps()
{
}
CStringMaps::~CStringMaps()
{
	m_Data.RemoveAll();
}
static int StrMapsCmp(const void *src, const void *dis)
{
	return (0 - ((CStringW *)dis)->CompareNoCase((LPCWSTR)src));
}
int CStringMaps::Add(LPCWSTR wstr)
{
	int n;
	CStringW tmp(wstr);

	if ( BinaryFind((void *)wstr, m_Data.GetData(), sizeof(CStringW), (int)m_Data.GetSize(), StrMapsCmp, &n) )
		return n;

	m_Data.InsertAt(n, tmp);
	return n;
}
int CStringMaps::Find(LPCWSTR wstr)
{
	int n;

	if ( BinaryFind((void *)wstr, m_Data.GetData(), sizeof(CStringW), (int)m_Data.GetSize(), StrMapsCmp, &n) )
		return n;

	if ( n >= (int)m_Data.GetSize() )
		return (-1);

	return (m_Data[n].Left((int)wcslen(wstr)).CompareNoCase(wstr) == 0 ? n : (-1));
}
int CStringMaps::Next(LPCWSTR wstr, int pos)
{
	if ( pos >= GetSize() )
		return (-1);

	return (m_Data[pos].Left((int)wcslen(wstr)).CompareNoCase(wstr) == 0 ? pos : (-1));
}
void CStringMaps::AddWStrBuf(LPBYTE lpBuf, int nLen)
{
	CStringW tmp;
	WCHAR *wp;

	wp = (WCHAR *)lpBuf;
	nLen /= sizeof(WCHAR);

	while ( nLen-- > 0 ) {
		if ( *wp == L'"' ) {
			tmp += *(wp++);
			while ( nLen > 0 ) {
				tmp += *wp;
				nLen--;
				if ( *(wp++) == L'"' )
					break;
			}
		} else if ( *wp == L'\'' ) {
			tmp += *(wp++);
			while ( nLen > 0 ) {
				tmp += *wp;
				nLen--;
				if ( *(wp++) == L'\'' )
					break;
			}
		} else if ( *wp == L' ' ) {
			wp++;
			if ( !tmp.IsEmpty() ) {
				Add(tmp);
				tmp.Empty();
			}
		} else
			tmp += *(wp++);
	}
	if ( !tmp.IsEmpty() )
		Add(tmp);
}

//////////////////////////////////////////////////////////////////////
// CStringIndex

CStringIndex::CStringIndex()
{
	m_bNoCase = TRUE;
	m_bNoSort = FALSE;
	m_bString = TRUE;
	m_bEmpty  = TRUE;
	m_Value = 0;
	m_nIndex.Empty();
	m_String.Empty();
	m_Array.RemoveAll();
}
CStringIndex::~CStringIndex()
{
}
const CStringIndex & CStringIndex::operator = (CStringIndex &data)
{
	m_bNoCase = data.m_bNoCase;
	m_bNoSort = data.m_bNoSort;
	m_bString = data.m_bString;
	m_Value   = data.m_Value;
	m_nIndex  = data.m_nIndex;
	m_String  = data.m_String;
	m_bEmpty  = data.m_bEmpty;

	m_Array.RemoveAll();
	for ( int n = 0 ; n < data.m_Array.GetSize() ; n++ )
		m_Array.Add(data.m_Array[n]);

	return *this;
}
static int StrIdxCmp(const void *src, const void *dis)
{
	return (0 - ((CStringIndex *)dis)->m_nIndex.Compare((LPCTSTR)src));
}
static int StrIdxCmpNoCase(const void *src, const void *dis)
{
	return (0 - ((CStringIndex *)dis)->m_nIndex.CompareNoCase((LPCTSTR)src));
}
CStringIndex & CStringIndex::operator [] (LPCTSTR str)
{
	int n;
	CStringIndex tmpData;

	if ( m_bNoSort ) {
		for ( n = 0 ; n < m_Array.GetSize() ; n++ ) {
			if ( (m_bNoCase ? m_Array[n].m_nIndex.CompareNoCase(str) : m_Array[n].m_nIndex.Compare(str)) == 0 )
				return m_Array[n];
		}
	} else {
		if ( BinaryFind((void *)str, m_Array.GetData(), sizeof(CStringIndex), (int)m_Array.GetSize(), m_bNoCase ? StrIdxCmpNoCase : StrIdxCmp, &n) )
			return m_Array[n];
	}

	tmpData.m_nIndex = str;
	tmpData.SetNoCase(m_bNoCase);
	tmpData.SetNoSort(m_bNoSort);
	m_Array.InsertAt(n, tmpData);
	return m_Array[n];
}
int CStringIndex::Find(LPCTSTR str)
{
	int n;

	if ( m_bNoSort ) {
		for ( n = 0 ; n < m_Array.GetSize() ; n++ ) {
			if ( (m_bNoCase ? m_Array[n].m_nIndex.CompareNoCase(str) : m_Array[n].m_nIndex.Compare(str)) == 0 )
				return n;
		}
	} else {
		if ( BinaryFind((void *)str, m_Array.GetData(), sizeof(CStringIndex), (int)m_Array.GetSize(), m_bNoCase ? StrIdxCmpNoCase : StrIdxCmp, &n) )
			return n;
	}

	return (-1);
}
void CStringIndex::SetArray(LPCTSTR str)
{
	TCHAR c, se;
	int st = 0;
	CString idx;
	CString val;

	m_Array.RemoveAll();
	while ( (c = *(str++)) != _T('\0') ) {
		switch(st) {
		case 0:
			if ( c == _T('=') ) {
				st = 1;
				idx.Trim(_T(" \t\r\n"));
				val.Empty();
			} else if ( c == _T(',') || c == _T(' ') || c == _T('\t') || c == _T('\r') || c == _T('\n') ) {
				idx.Trim(_T(" \t\r\n"));
				if ( !idx.IsEmpty() )
					(*this)[idx] = _T("");
				idx.Empty();
			} else
				idx += (TCHAR)c;
			break;
		case 1:
			if ( c == _T('"') || c == _T('\'') ) {
				se = c;
				st = 3;
				break;
			} else if ( c == _T(' ') || c == _T('\t') || c == _T('\r') || c == _T('\n') )
				break;
			st = 2;
		case 2:
			if ( c == _T(',') ) {
				val.Trim(_T(" \t\r\n"));
				if ( !idx.IsEmpty() )
					(*this)[idx] = val;
				st = 0;
				idx.Empty();
			} else
				val += c;
			break;
		case 3:
			if ( c == se ) {
				if ( !idx.IsEmpty() )
					(*this)[idx] = val;
				st = 4;
				idx.Empty();
			} else
				val += c;
			break;
		case 4:
		    if ( c == _T(',') )
				st = 0;
		    break;
		}
	}

    switch(st) {
    case 1:
		if ( !idx.IsEmpty() )
		    (*this)[idx] = _T("");
		break;
    case 2:
		val.Trim(_T(" \t\r\n"));
    case 3:
		if ( !idx.IsEmpty() )
			(*this)[idx] = val;
		break;
    }
}
void CStringIndex::SetValue(LPCSTR &str)
{
	m_bEmpty = FALSE;
	m_bString = FALSE;
	m_String.Empty();
	m_Value = 0;

	while ( *str != '\0' && *str != ';' && *str != ':' ) {
		if ( *str < '0' || *str > '9' )
			m_bString = TRUE;
		m_String += *(str++);
	}

	if ( !m_bString ) {
		m_Value = _tstoi(m_String);
	} else if ( m_String.CompareNoCase(_T("true")) == 0 ) {
		m_Value = 1;
		m_bString = FALSE;
	} else if ( m_String.CompareNoCase(_T("false")) == 0 ) {
		m_Value = 0;
		m_bString = FALSE;
	}
}
void CStringIndex::SetParam(LPCSTR &str)
{
	CString value;

	while ( *str != '\0' ) {
		if ( *str == '=' ) {
			(*this)[value].SetValue(++str);
			value.Empty();
			if ( *str == ';' ) {
				str++;
			} else if ( *str == ':' ) {
				str++;
				while ( *str != '\0' )
					value += *(str++);
				break;
			}
		} else
			value += *(str++);
	}

	if ( !value.IsEmpty() )
		*this = value;
}
void CStringIndex::SetKey(LPCSTR &str)
{
	CString value;

	while ( *str != '\0' ) {
		if ( *str == '=' ) {
			(*this)[value].SetParam(++str);
			value.Empty();
		} else
			value += *(str++);
	}
}
void CStringIndex::GetBuffer(CBuffer *bp)
{
	CStringA mbs;

	m_Value = bp->Get32Bit();
	bp->GetStr(mbs); m_nIndex = mbs;
	bp->GetStr(mbs); m_String = mbs;

	SetSize(bp->Get32Bit());

	for ( int n = 0 ; n < GetSize() ; n++ )
		m_Array[n].GetBuffer(bp);
}
void CStringIndex::SetBuffer(CBuffer *bp)
{
	bp->Put32Bit(m_Value);
	bp->PutStr(TstrToMbs(m_nIndex));
	bp->PutStr(TstrToMbs(m_String));

	bp->Put32Bit(GetSize());

	for ( int n = 0 ; n < GetSize() ; n++ )
		m_Array[n].SetBuffer(bp);
}
void CStringIndex::GetString(LPCTSTR str)
{
	CBuffer tmp;
	if ( *str == _T('\0') )
		return;
	tmp.Base64Decode(str);
	if ( tmp.GetSize() < 16 )
		return;
	GetBuffer(&tmp);
}
void CStringIndex::SetString(CString &str)
{
	CBuffer tmp, work;
	SetBuffer(&tmp);
	work.Base64Encode(tmp.GetPtr(), tmp.GetSize());
	str = (LPCTSTR)work;
}

void CStringIndex::SetPackStr(CStringA &mbs)
{
	if ( m_bEmpty ) {
		if ( GetSize() > 0 && m_Array[0].m_nIndex.IsEmpty() ) {
			mbs = '[';
			for ( int n = 0 ; n < GetSize() ; n++ ) {
				CStringA tmp;
				m_Array[n].SetPackStr(tmp);
				if ( n > 0 )
					mbs += ',';
				mbs += tmp;
			}
			mbs += ']';
		}

	} else if ( m_bString ) {
		CStringA tmp;
		LPCTSTR p = m_String;

		mbs = '"';
		while ( *p != _T('\0') ) {
			if ( *p == _T('\\') || *p == _T('"') ) {
				mbs += '\\';
				mbs += *(p++);
			} else if ( (*p & 0xFF00) != 0 ) {
				tmp = *p;
				if ( tmp.IsEmpty() || tmp.Compare("?") == 0 )
					tmp.Format("\\u%04X", *p);
				mbs += tmp;
				p++;
			} else if ( *p < _T('\x20') || *p > _T('\x7E') ) {
				switch(*p) {
				case _T('\b'): mbs += "\\b"; break;
				case _T('\t'): mbs += "\\t"; break;
				case _T('\n'): mbs += "\\n"; break;
				case _T('\a'): mbs += "\\a"; break;
				case _T('\f'): mbs += "\\f"; break;
				case _T('\r'): mbs += "\\r"; break;
				default:
					tmp.Format("\\x%02X", *p);
					mbs += tmp;
					break;
				}
				p++;
			} else
				mbs += *(p++);
		}
		mbs += '"';

	} else {
		mbs.Format("%d", m_Value);
	}
}
LPCTSTR CStringIndex::GetPackStr(LPCTSTR str)
{
	int n, m;
	DWORD c;

	m_bString = TRUE;
	m_bEmpty  = TRUE;
	m_String.Empty();
	m_Value = 0;
	RemoveAll();

	while ( *str != _T('\0') ) {

		while ( *str <= _T(' ') )
			str++;

		if ( *str == _T('[') ) {
			for ( str++ ; *str != _T('\0') ; ) {
				n = GetSize();
				SetSize(n + 1);
				str = m_Array[n].GetPackStr(str);

				while ( *str <= _T(' ') )
					str++;

				if ( *str == _T(']') ) {
					str++;
					break;
				} else if ( *str != _T(',') )
					break;
				str++;
			}

		} else if ( *str == _T('"') ) {
			str++;
			m_String.Empty();
			while ( *str != _T('\0') && *str != _T('"') ) {
				if ( *str == _T('\\') ) {
					switch(*(++str)) {
					case _T('0'):
						break;

					case _T('x'): case _T('X'):
					case _T('u'): case _T('U'):
						m = (*str == _T('x') || *str == _T('X') ? 2 : 4);
						str++;
						c = 0;
						for ( n = 0 ; n < m ; n++ ) {
							if ( *str >= _T('0') && *str <= _T('9') )
								c = c * 16 + (*(str++) - _T('0'));
							else if ( *str >= _T('A') && *str <= _T('F') )
								c = c * 16 + (*(str++) - _T('A') + 10);
							else if ( *str >= _T('a') && *str <= _T('f') )
								c = c * 16 + (*(str++) - _T('a') + 10);
							else
								break;
						}
						if ( c >= 0x10000 ) {
							c = CTextRam::UCS4toUCS2(c);
							m_String += (WCHAR)(c >> 16);
							m_String += (WCHAR)c;
						} else
							m_String += (WCHAR)c;
						break;

					case _T('b'): m_String += _T("\b"); str++; break;
					case _T('t'): m_String += _T("\t"); str++; break;
					case _T('n'): m_String += _T("\n"); str++; break;
					case _T('a'): m_String += _T("\a"); str++; break;
					case _T('f'): m_String += _T("\f"); str++; break;
					case _T('r'): m_String += _T("\r"); str++; break;

					default:
						m_String += *(str++);
						break;
					}
				} else
					m_String += *(str++);
			}
			if ( *str != _T('"') )
				break;
			str++;

			m_bString = TRUE;
			m_bEmpty  = FALSE;
			break;

		} else if ( (*str >= _T('0') && *str <= _T('9')) || *str == _T('-') ) {
			if ( *str == _T('0') ) {
				str++;
				if ( *str == _T('x') || *str == _T('X') || *str == _T('u') || *str == _T('U') ) {
					str++;
					for ( c = 0 ; ; str++ ) {
						if ( *str >= _T('0') && *str <= _T('9') )
							c = c * 16 + (*str - _T('0'));
						else if ( *str >= _T('A') && *str <= _T('F') )
							c = c * 16 + (*str - _T('A') + 10);
						else if ( *str >= _T('a') && *str <= _T('f') )
							c = c * 16 + (*str - _T('a') + 10);
						else
							break;
					}
				} else {
					for ( c = 0 ; *str >= _T('0') && *str <= _T('7') ; str++ )
							c = c * 8 + (*str - _T('0'));
				}
				m_Value = c;
			} else if ( *str == _T('-') ) {
				str++;
				for ( m_Value = 0 ; *str >= _T('0') && *str <= _T('9') ; str++ )
					m_Value = m_Value * 10 + (*str - _T('0'));
				m_Value = 0 - m_Value;
			} else {
				for ( m_Value = 0 ; *str >= _T('0') && *str <= _T('9') ; str++ )
					m_Value = m_Value * 10 + (*str - _T('0'));
			}

			m_bString = FALSE;
			m_bEmpty  = FALSE;
			break;

		} else
			break;
	}

	return str;
}
BOOL CStringIndex::ReadString(CArchive& ar, CString &str)
{
	CHAR c;
	CStringA mbs;

	while ( ar.Read(&c, 1) == 1 ) {
		mbs += c;
		if ( c == '\n' )
			break;
	}

	if ( mbs.IsEmpty() )
		return FALSE;

	str = mbs.Trim(" \t\r\n");

	return TRUE;
}
void CStringIndex::Serialize(CArchive& ar, LPCSTR base)
{
	if ( ar.IsStoring() ) {		// Write
		CStringA mbs, tmp;

		if ( base != NULL )
			mbs = base;
		if ( !mbs.IsEmpty() )
			mbs += '.';
		mbs += m_nIndex;

		if ( GetSize() > 0 && !m_Array[0].m_nIndex.IsEmpty() ) {
			for ( int n = 0 ; n < GetSize() ; n++ )
				m_Array[n].Serialize(ar, mbs);
		} else {
			SetPackStr(tmp);
			mbs += '=';
			mbs += tmp;
			mbs += ";\r\n";
			ar.Write((LPCSTR)mbs, sizeof(CHAR) * mbs.GetLength());
		}

	} else {					// Read
		CString str;
		CString base, data, index;
		LPCTSTR p;
		CStringIndex *ip;

		while ( ReadString(ar, str) ) {
			if ( str.Compare(_T("ENDOF")) == 0 )
				break;

			p = str;
			base.Empty();
			data.Empty();

			while ( *p != _T('\0') ) {
				if ( *p == _T('#') )
					break;
				else if ( *p == _T('=') )
					break;
				base += *(p++);
			}
			base.Trim(_T(" \t\r\n"));

			if ( *p == _T('=') ) {
				data = ++p;
				data.Trim(_T(" \t\r\n"));
			}

			if ( base.IsEmpty() )
				continue;

			ip = this;
			p = base;
			index.Empty();
			while ( *p != _T('\0') ) {
				if ( *p == _T('.') ) {
					ip = &((*ip)[index]);
					index.Empty();
					p++;
				} else
					index += *(p++);
			}
			if ( !index.IsEmpty() )
				ip = &((*ip)[index]);

			ip->GetPackStr(data);
		}
	}
}
#ifdef	DEBUG
void CStringIndex::Dump(int nest)
{
	for ( int n = 0 ; n < nest ; n++ )
		TRACE(_T(" "));

	if ( m_bString )
		TRACE(_T("%s = \"%s\"\n"), m_nIndex, m_String);
	else
		TRACE(_T("%s = %d\n"), m_nIndex, m_Value);

	for ( int n = 0 ; n < m_Array.GetSize() ; n++ )
		m_Array[n].Dump(nest + 1);
}
#endif

//////////////////////////////////////////////////////////////////////
// CStringBinary Root

CStringBinary::CStringBinary()
{
	m_pRoot = m_pLeft = m_pRight = NULL;
	m_Value = (-1);
	m_Index.Empty();
	m_Balance = 0;
}
CStringBinary::~CStringBinary()
{
	if ( m_pRoot != NULL )
		delete m_pRoot;

	if ( m_pLeft != NULL )
		delete m_pLeft;

	if ( m_pRight != NULL )
		delete m_pRight;
}
CStringBinary & CStringBinary::operator [] (LPCTSTR str)
{
	BOOL bNew = FALSE;

	if ( m_pRoot == NULL ) {
		m_pRoot = new CStringBinary(str);
		return *m_pRoot;
	} else
		return *(m_pRoot->Add(&m_pRoot, str, bNew));
}
CStringBinary & CStringBinary::operator [] (int number)
{
	CString index;

	index.Format(_T("%d"), number);
	return (*this)[index];
}
void CStringBinary::RemoveAll()
{
	if ( m_pRoot != NULL )
		delete m_pRoot;

	m_pRoot = NULL;
}
CStringBinary * CStringBinary::Find(LPCTSTR str)
{
	if ( m_pRoot == NULL )
		return NULL;

	return m_pRoot->FindNode(str);
}
CStringBinary * CStringBinary::FindValue(int value)
{
	if ( m_pRoot == NULL )
		return NULL;

	return m_pRoot->FindNodeValue(value);
}

//////////////////////////////////////////////////////////////////////
// CStringBinary Node

CStringBinary::CStringBinary(LPCTSTR str)
{
	m_pRoot = m_pLeft = m_pRight = NULL;
	m_Value = (-1);
	m_Index = str;
	m_Balance = 0;
}
CStringBinary * CStringBinary::Add(CStringBinary **ppTop, LPCTSTR str, BOOL &bNew)
{
	int c = m_Index.Compare(str);
	CStringBinary *np, *tp, *sp;

	if ( c == 0 )
		return this;

	else if ( c < 0 ) {
		if ( m_pLeft == NULL ) {
			m_pLeft = new CStringBinary(str);
			bNew = TRUE;
			np = m_pLeft;
		} else
			np = m_pLeft->Add(&m_pLeft, str, bNew);

		if ( bNew ) {
			if ( --m_Balance == 0 ) {
				bNew = FALSE;
			} else if ( m_Balance < (-1) ) {
				tp = m_pLeft;
				if ( tp->m_Balance == (-1) || tp->m_pRight == NULL ) {
					m_pLeft      = tp->m_pRight;
					tp->m_pRight = this;

					m_Balance     = 0;
					tp->m_Balance = 0;
					*ppTop = tp;

				} else {
					sp           = tp->m_pRight;
					tp->m_pRight = sp->m_pLeft;
					sp->m_pLeft  = tp;
						m_pLeft  = sp->m_pRight;
					sp->m_pRight = this;

					m_Balance     = (sp->m_Balance == (-1) ?   1  : 0);
					tp->m_Balance = (sp->m_Balance ==   1  ? (-1) : 0);
					sp->m_Balance = 0;
					*ppTop = sp;
				}
				bNew = FALSE;
			}
		}

	} else {
		if ( m_pRight == NULL ) {
			m_pRight = new CStringBinary(str);
			bNew = TRUE;
			np = m_pRight;
		} else
			np = m_pRight->Add(&m_pRight, str, bNew);

		if ( bNew ) {
			if ( ++m_Balance == 0 ) {
				bNew = FALSE;
			} else if ( m_Balance > 1 ) {
				tp = m_pRight;
				if ( tp->m_Balance == 1 || tp->m_pLeft == NULL ) {
					m_pRight     = tp->m_pLeft;
					tp->m_pLeft  = this;

					m_Balance     = 0;
					tp->m_Balance = 0;
					*ppTop = tp;

				} else {
					sp           = tp->m_pLeft;
					tp->m_pLeft  = sp->m_pRight;
					sp->m_pRight = tp;
						m_pRight = sp->m_pLeft;
					sp->m_pLeft  = this;

					m_Balance     = (sp->m_Balance ==   1  ? (-1) : 0);
					tp->m_Balance = (sp->m_Balance == (-1) ?   1  : 0);
					sp->m_Balance = 0;
					*ppTop = sp;
				}
				bNew = FALSE;
			}
		}
	}

	return np;
}
CStringBinary * CStringBinary::FindNode(LPCTSTR str)
{
	int c = m_Index.Compare(str);

	if ( c == 0 )
		return this;
	else if ( c < 0 ) {
		if ( m_pLeft == NULL )
			return NULL;
		return m_pLeft->FindNode(str);
	} else {
		if ( m_pRight == NULL )
			return NULL;
		return m_pRight->FindNode(str);
	}
}
CStringBinary * CStringBinary::FindNodeValue(int value)
{
	CStringBinary *bp;

	if ( m_Value == value )
		return this;

	if ( m_pLeft != NULL && (bp = m_pLeft->FindNodeValue(value)) != NULL )
		return bp;
	else if ( m_pRight != NULL && (bp = m_pRight->FindNodeValue(value)) != NULL )
		return bp;

	return NULL;
}

#ifdef	DEBUG
void CStringBinary::Tree(int nest)
{
	if ( m_pRoot != NULL )
		m_pRoot->TreeNode(nest);
}
void CStringBinary::TreeNode(int nest)
{
	if ( m_pLeft != NULL )
		m_pLeft->TreeNode(nest + 1);

	TRACE(_T("%d %s(%d)\n"), nest, m_Index, m_Balance);

	if ( m_pRight != NULL )
		m_pRight->TreeNode(nest + 1);
}
#endif

//////////////////////////////////////////////////////////////////////
// CFileExt
	
void CFileExt::Init()
{
	m_WriteLen = 0;
	m_pWriteBuffer = new BYTE[FEXT_BUF_MAX];

	m_ReadLen = m_ReadPos = 0;
	m_pReadBuffer = new BYTE[FEXT_BUF_MAX];
}
CFileExt::CFileExt()
{
	Init();
	CFile::CFile();
}
#if	_MSC_VER >= _MSC_VER_VS10
CFileExt::CFileExt(CAtlTransactionManager* pTM)
{
	Init();
	CFile::CFile(pTM);
}
#endif
CFileExt::CFileExt(HANDLE hFile)
{
	Init();
	CFile::CFile(hFile);
}
CFileExt::CFileExt(LPCTSTR lpszFileName, UINT nOpenFlags)
{
	Init();
	CFile::CFile(lpszFileName, nOpenFlags);
}

CFileExt::~CFileExt()
{
	if ( m_hFile != INVALID_HANDLE_VALUE )
		Close();

	delete [] m_pWriteBuffer;
	delete [] m_pReadBuffer;
}

BOOL CFileExt::Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
	m_WriteLen = 0;
	m_ReadLen = m_ReadPos = 0;

	return CFile::Open(lpszFileName, nOpenFlags, pError);
}
#if	_MSC_VER >= _MSC_VER_VS10
BOOL CFileExt::Open(LPCTSTR lpszFileName, UINT nOpenFlags, CAtlTransactionManager* pTM, CFileException* pError)
{
	m_WriteLen = 0;
	m_ReadLen = m_ReadPos = 0;

	return CFile::Open(lpszFileName, nOpenFlags, pTM, pError);
}
#endif

ULONGLONG CFileExt::GetPosition( ) const
{
	ULONGLONG lOff = CFile::GetPosition();

	if ( m_ReadLen > m_ReadPos )
		lOff -= (LONGLONG)(m_ReadLen - m_ReadPos);

	if ( m_WriteLen > 0 )
		lOff += (LONGLONG)(m_WriteLen);

	return lOff;
}
ULONGLONG CFileExt::Seek(LONGLONG lOff, UINT nFrom)
{
	if ( nFrom == CFile::current && m_ReadLen > m_ReadPos )
		lOff -= (LONGLONG)(m_ReadLen - m_ReadPos);

	Flush();
	m_ReadLen = m_ReadPos = 0;

	return CFile::Seek(lOff, nFrom);
}
void CFileExt::Write(const void* lpBuf, UINT nCount)
{
	UINT n;
	LPBYTE p = (LPBYTE)lpBuf;

	while ( nCount > 0 ) {
		if ( (n = FEXT_BUF_MAX - m_WriteLen) > nCount )
			n = nCount;
		memcpy(m_pWriteBuffer + m_WriteLen, p, n);
		m_WriteLen += n;
		p += n;
		nCount -= n;
		if ( m_WriteLen >= FEXT_BUF_MAX )
			Flush();
	}
}
void CFileExt::Flush()
{
	if ( m_WriteLen > 0 ) {
		CFile::Write(m_pWriteBuffer, m_WriteLen);
		m_WriteLen = 0;
	}
	CFile::Flush();
}
void CFileExt::Close()
{
	Flush();
	CFile::Close();
}
UINT CFileExt::Read(void* lpBuf, UINT nCount)
{
	UINT n;
	UINT len = 0;
	BYTE *ptr = (BYTE *)lpBuf;

	while ( nCount > 0 ) {
		if ( (n = m_ReadLen - m_ReadPos) > 0 ) {
			if ( n > nCount )
				n = nCount;
			memcpy(ptr, m_pReadBuffer + m_ReadPos, n);
			m_ReadPos += n;
			ptr += n;
			len += n;
			nCount -= n;
		}

		if ( nCount == 0 )
			break;

		m_ReadLen = m_ReadPos = 0;
		m_ReadLen = CFile::Read(m_pReadBuffer, FEXT_BUF_MAX);

		if ( m_ReadLen == 0 )
			break;
	}

	return len;
}

//////////////////////////////////////////////////////////////////////
// CHttpSession

CHttpSession::CHttpSession()
{
	m_Protocl    = _T("http");
	m_UserName   = _T("");
	m_Password   = _T("");
	m_HostName   = _T("");
	m_PortNumber = _T("");
	m_Param      = _T("");
}
CHttpSession::~CHttpSession()
{
}
void CHttpSession::ParseUrl(LPCTSTR url)
{
	// ssh://username:password@hostname:port/path?arg=value#anchor

	int n;
	LPCTSTR p;
	CString tmp;

	tmp.Empty();
	for ( p = url ; *p != _T('\0') ; p++ ) {
		if ( _tcsncmp(p, _T("://"), 3) == 0 ) {
			m_Protocl = tmp;
			url = p + 3;
			break;
		} else
			tmp += *p;
	}

	tmp.Empty();
	for ( p = url ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('@') ) {
			if ( (n = tmp.Find(_T(':'))) >= 0 ) {
				m_UserName = tmp.Left(n);
				m_Password = tmp.Mid(n + 1);
			} else
				m_UserName = tmp;
			url = p + 1;
			break;
		} else
			tmp += *p;
	}

	tmp.Empty();
	for ( p = url ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('/') || *p == _T('?') || *p == _T('#') ) {
			url = p;
			break;
		} else
			tmp += *p;
	}

	if ( (n = tmp.Find(_T(':'))) >= 0 ) {
		m_HostName  = tmp.Left(n);
		m_PortNumber = tmp.Mid(n + 1);
	} else
		m_HostName = tmp;

	m_Param = url;
}
BOOL CHttpSession::GetRequest(LPCTSTR url, CBuffer &buf)
{
	int n;
	DWORD dwStatus;
	CInternetSession InetSess;
	CHttpConnection *pHttpConn;
	CHttpFile *pHttpFile;
	BYTE tmp[BUFSIZ];

	if ( InternetAttemptConnect(0) != ERROR_SUCCESS )
		return FALSE;

	ParseUrl(url);

	if ( m_Protocl.Compare(_T("http")) != 0 )
		return FALSE;

	try {
		 if ( (pHttpConn = InetSess.GetHttpConnection(m_HostName,
							(m_PortNumber.IsEmpty() ? INTERNET_INVALID_PORT_NUMBER : _tstoi(m_PortNumber)), 
							(m_UserName.IsEmpty()   ? NULL : (LPCTSTR)m_UserName), 
							(m_Password.IsEmpty()   ? NULL : (LPCTSTR)m_Password))) == NULL )
			return FALSE;

		if ( (pHttpFile	= pHttpConn->OpenRequest(CHttpConnection::HTTP_VERB_GET, m_Param, NULL, 1, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE)) == NULL )
			return FALSE;

		if ( !pHttpFile->SendRequest() )
			return FALSE;

		if ( !pHttpFile->QueryInfoStatusCode(dwStatus) )
			return FALSE;

		if ( dwStatus != HTTP_STATUS_OK )
			return FALSE;

		while ( (n = pHttpFile->Read(tmp, BUFSIZ)) > 0 )
			buf.Apend(tmp, n);

		// buf.KanjiConvert(buf.KanjiCheck());

		pHttpFile->Close();
		pHttpConn->Close();
		InetSess.Close();

		delete pHttpFile;
		delete pHttpConn;

		return TRUE;

	} catch (...) {
		return FALSE;
	}
}
