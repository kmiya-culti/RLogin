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

#include <ocidl.h>
#include <olectl.h>
#include <ole2.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CBuffer

//#define	STATICBUFFER

#ifdef	STATICBUFFER

typedef struct _BufPtr {
	int				size;
	struct _BufPtr	*next;
} BUFPTR;

static	BOOL	BufPtrInit = FALSE;
static	BUFPTR	*BufPtrHash[256];

static	int	CalcHash(int size)
{
	int n = size >> 8;

	if ( n >= 16 )
		n = (n >> 2) + 16;
	if ( n >= 64 )
		n = (n >> 2) + 64;
	if ( n >= 256 )
		n = 255;
	return n;
}
static	LPBYTE	GetBufPtr(int size)
{
	int n;
	BUFPTR *bp, *tp;

	if ( !BufPtrInit ) {
		for ( n = 0 ; n < 256 ; n++ )
			BufPtrHash[n] = NULL;
		BufPtrInit = TRUE;
	}

	n = CalcHash(size);

	for ( bp = tp = BufPtrHash[n] ; bp != NULL ; ) {
		if ( bp->size >= size ) {
			if ( bp == tp )
				BufPtrHash[n] = bp->next;
			else
				tp->next = bp->next;
			return (LPBYTE)bp;
		}
		tp = bp;
		bp = bp->next;
	}

	return new BYTE[size];
}
static	void	SetBufPtr(int size, LPBYTE buf)
{
	int n;
	BUFPTR *bp;

	n = CalcHash(size);

	bp = (BUFPTR *)buf;
	bp->size = size;
	bp->next = BufPtrHash[n];
	BufPtrHash[n] = bp;
}
#endif
void	FreeBufPtr()
{
#ifdef	STATICBUFFER
	int n;
	BUFPTR *bp;

	if ( !BufPtrInit )
		return;

	for ( n = 0 ; n < 256 ; n++ ) {
		while ( (bp = BufPtrHash[n]) != NULL ) {
			BufPtrHash[n] = bp->next;
			delete (LPBYTE)bp;
		}
	}
#endif
}

CBuffer::CBuffer(int size)
{
	m_Ofs = m_Len = 0;
#ifdef	STATICBUFFER
	m_Max = (size + NIMALLOC) & ~(NIMALLOC - 1);
	m_Data = GetBufPtr(m_Max);
#else
	m_Max = size;
	m_Data = new BYTE[m_Max];
#endif
}
CBuffer::CBuffer()
{
	m_Ofs = m_Len = 0;
#ifdef	STATICBUFFER
	m_Max = NIMALLOC;
	m_Data = GetBufPtr(m_Max);
#else
	m_Max = 32;
	m_Data = new BYTE[m_Max];
#endif
}
#ifdef	_DEBUGXXX
static	int	report[32] = {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};
void CBuffer::RepSet()
{
	int n, b;
	b = 0x80000000;
	for ( n = 0 ; n < 32 ; n++ ) {
		if ( (m_Max & b) != 0 ) {
			report[n]++;
			break;
		}
		b >>= 1;
	}
}
void CBuffer::Report()
{
	int n, b;
	b = 0x00000001;
	for ( n = 31 ; n > 0 ; n-- ) {
		TRACE("%d:%d\n", b, report[n]);
		b <<= 1;
	}
}
void CBuffer::Debug()
{
	int n;
	for ( n = 0 ; n < m_Len ; n++ )
		TRACE("%02x ", m_Data[m_Ofs + n] & 0xFF);
	TRACE("\n");
}
#endif
CBuffer::~CBuffer()
{
#ifdef	_DEBUGXXX
	RepSet();
#endif

#ifdef	STATICBUFFER
	SetBufPtr(m_Max, m_Data);
#else
	delete m_Data;
#endif
}
void CBuffer::Consume(int len)
{
	m_Ofs += len;
	if ( m_Ofs >= m_Len )
		m_Len = m_Ofs = 0;
}
void CBuffer::ReAlloc(int len)
{
	if ( (len += m_Len) <= m_Max )
		return;
	else if ( (len -= m_Ofs) <= m_Max ) {
		if ( (m_Len -= m_Ofs) > 0 )
			memcpy(m_Data, m_Data + m_Ofs, m_Len);
		m_Ofs = 0;
		return;
	}

#ifdef	STATICBUFFER
	int old = m_Max;
	m_Max = (len * 2 + NIMALLOC) & ~(NIMALLOC - 1);
	LPBYTE tmp = GetBufPtr(m_Max);
#else
	m_Max = (len * 2 + NIMALLOC) & ~(NIMALLOC - 1);
	LPBYTE tmp = new BYTE[m_Max];
#endif

	if ( (m_Len -= m_Ofs) > 0 )
		memcpy(tmp, m_Data + m_Ofs, m_Len);
	m_Ofs = 0;

#ifdef	STATICBUFFER
	SetBufPtr(old, m_Data);
#else
	delete m_Data;
#endif

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
LPBYTE CBuffer::PutSpc(int len)
{
	ReAlloc(len);
	m_Len += len;
	return (m_Data + m_Len - len);
}
void CBuffer::Put8Bit(int val)
{
	ReAlloc(1);
	SET8BIT(m_Data + m_Len, val);
	m_Len += 1;
}
void CBuffer::Put16Bit(int val)
{
	ReAlloc(2);
	SET16BIT(m_Data + m_Len, val);
	m_Len += 2;
}
void CBuffer::Put32Bit(int val)
{
	ReAlloc(4);
	SET32BIT(m_Data + m_Len, val);
	m_Len += 4;
}
void CBuffer::Put64Bit(LONGLONG val)
{
	ReAlloc(8);
	SET64BIT(m_Data + m_Len, val);
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
		AfxThrowMemoryException();
	Put16Bit(bits);
	Apend(tmp, len);
	delete tmp;
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
		AfxThrowMemoryException();

	bytes = BN_num_bytes(val) + 1; /* extra padding byte */
    if ( bytes < 2 )
		AfxThrowMemoryException();

	if ( (tmp = new BYTE[bytes]) == NULL )
		AfxThrowMemoryException();

	tmp[0] = '\x00';

    if ( BN_bn2bin(val, tmp + 1) != (bytes - 1) )
		AfxThrowMemoryException();

	hnoh = (tmp[1] & 0x80) ? 0 : 1;
	Put32Bit(bytes - hnoh);
	Apend(tmp + hnoh, bytes - hnoh);
	delete tmp;
}
void CBuffer::PutWord(int val)
{
	WORD wd = (WORD)val;
	Apend((LPBYTE)(&wd), sizeof(WORD));
}
int CBuffer::Get8Bit()
{
	if ( GetSize() < 1 )
		AfxThrowUserException();
	int val = PTR8BIT(GetPtr());
	Consume(1);
	return val;
}
int CBuffer::Get16Bit()
{
	if ( GetSize() < 2 )
		AfxThrowUserException();
	int val = PTR16BIT(GetPtr());
	Consume(2);
	return val;
}
int CBuffer::Get32Bit()
{
	if ( GetSize() < 4 )
		AfxThrowUserException();
	int val = PTR32BIT(GetPtr());
	Consume(4);
	return val;
}
LONGLONG CBuffer::Get64Bit()
{
	if ( GetSize() < 8 )
		AfxThrowUserException();
	LONGLONG val = PTR64BIT(GetPtr());
	Consume(8);
	return val;
}
int CBuffer::GetStr(CString &str)
{
	int len = Get32Bit();
	if ( len < 0 || len > (256 * 1024) || (m_Len - m_Ofs) < len )
		AfxThrowUserException();
	memcpy(str.GetBufferSetLength(len), m_Data + m_Ofs, len);
	Consume(len);
	return TRUE;
}
int CBuffer::GetBuf(CBuffer *buf)
{
	int len = Get32Bit();
	if ( len < 0 || len > (256 * 1024) || (m_Len - m_Ofs) < len )
		AfxThrowUserException();
	buf->Apend(GetPtr(), len);
	Consume(len);
	return TRUE;
}
int CBuffer::GetBIGNUM(BIGNUM *val)
{
	int bytes;
	int bits = Get16Bit();
	if ( (bytes = (bits + 7) / 8) > (8 * 1024) )
		AfxThrowUserException();
	if ( (m_Len - m_Ofs) < bytes )
		AfxThrowUserException();
    BN_bin2bn(m_Data + m_Ofs, bytes, val);
	Consume(bytes);
	return TRUE;
}
int CBuffer::GetBIGNUM2(BIGNUM *val)
{
	int bytes = Get32Bit();
	if ( (m_Len - m_Ofs) < bytes )
		AfxThrowUserException();
    BN_bin2bn(m_Data + m_Ofs, bytes, val);
	Consume(bytes);
	return TRUE;
}
int CBuffer::GetWord()
{
	if ( GetSize() < sizeof(WORD) )
		AfxThrowUserException();
	WORD wd = *((WORD *)GetPtr());
	Consume(sizeof(WORD));
	return (int)wd;
}
void CBuffer::SET16BIT(LPBYTE pos, int val)
{
	register LPMEMSWAP p = (LPMEMSWAP)pos;
	register LPMEMSWAP w = (LPMEMSWAP)(&val);
	WORDSWAP(p, w);
}
void CBuffer::SET32BIT(LPBYTE pos, int val)
{
	register LPMEMSWAP p = (LPMEMSWAP)pos;
	register LPMEMSWAP w = (LPMEMSWAP)(&val);
	LONGSWAP(p, w);
}
void CBuffer::SET64BIT(LPBYTE pos, LONGLONG val)
{
	register LPMEMSWAP p = (LPMEMSWAP)pos;
	register LPMEMSWAP w = (LPMEMSWAP)(&val);
	LONGLONGSWAP(p, w);
}
int CBuffer::PTR16BIT(LPBYTE pos)
{
	int val = 0;
	register LPMEMSWAP p = (LPMEMSWAP)pos;
	register LPMEMSWAP w = (LPMEMSWAP)(&val);
	WORDSWAP(w, p);
	return val;
}
int CBuffer::PTR32BIT(LPBYTE pos)
{
	int val;
	register LPMEMSWAP p = (LPMEMSWAP)pos;
	register LPMEMSWAP w = (LPMEMSWAP)(&val);
	LONGSWAP(w, p);
	return val;
}
LONGLONG CBuffer::PTR64BIT(LPBYTE pos)
{
	LONGLONG val;
	register LPMEMSWAP p = (LPMEMSWAP)pos;
	register LPMEMSWAP w = (LPMEMSWAP)(&val);
	LONGLONGSWAP(w, p);
	return val;
}

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

void CBuffer::Base64Decode(LPCSTR str)
{
	int n, c, o;

	Clear();
	for ( n = o = 0 ; (c = Base64DecTab[(unsigned char)(*str)]) >= 0 ; n++, str++ ) {
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
}
void CBuffer::Base64Encode(LPBYTE buf, int len)
{
	int n;

	Clear();
	for ( n = len ; n > 0 ; n -= 3, buf += 3 ) {
		if ( n >= 3 ) {
			Put8Bit(Base64EncTab[(buf[0] >> 2) & 077]);
			Put8Bit(Base64EncTab[((buf[0] << 4) | (buf[1] >> 4)) & 077]);
			Put8Bit(Base64EncTab[((buf[1] << 2) | (buf[2] >> 6)) & 077]);
			Put8Bit(Base64EncTab[buf[2] & 077]);
		} else if ( n >= 2 ) {
			Put8Bit(Base64EncTab[(buf[0] >> 2) & 077]);
			Put8Bit(Base64EncTab[((buf[0] << 4) | (buf[1] >> 4)) & 077]);
			Put8Bit(Base64EncTab[(buf[1] << 2) & 077]);
			Put8Bit('=');
		} else {
			Put8Bit(Base64EncTab[(buf[0] >> 2) & 077]);
			Put8Bit(Base64EncTab[(buf[0] << 4) & 077]);
			Put8Bit('=');
			Put8Bit('=');
		}
	}
	Put8Bit('\0');
}

static const char QuotedEncTab[] = "0123456789abcdef";
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

void CBuffer::QuotedDecode(LPCSTR str)
{
    int c1, c2;

	Clear();
	while ( *str != '\0' ) {
		if ( str[0] == '=' && (c1 = QuotedDecTab[str[1]]) >= 0 && (c2 = QuotedDecTab[str[2]]) >= 0 ) {
			Put8Bit((c1 << 4) | c2);
			str += 3;
		} else if ( str[0] == '=' && str[1] == '\n' ) {
		    str += 2;
		} else if ( str[0] == '=' && str[1] == '\0' ) {
		    break;
		} else {
			Put8Bit(*str);
			str += 1;
		}
    }
}
void CBuffer::QuotedEncode(LPBYTE buf, int len)
{
	Clear();
    while ( len > 0 ) {
		if ( (*buf != '\n' && *buf != '\t' && *buf < ' ' ) || *buf >= 127 || *buf == '=' ) {
			Put8Bit('=');
			Put8Bit(QuotedEncTab[*buf >> 4]);
			Put8Bit(QuotedEncTab[*buf & 15]);
		} else {
			Put8Bit(*buf);
		}
		buf += 1;
		len -= 1;
    }
	Put8Bit('\0');
}

void CBuffer::Base16Decode(LPCSTR str)
{
    int c1, c2;

	Clear();
	while ( *str != '\0' ) {
		if ( (c1 = QuotedDecTab[str[0]]) >= 0 && (c2 = QuotedDecTab[str[1]]) >= 0 ) {
			Put8Bit((c1 << 4) | c2);
			str += 2;
		} else
			break;
	}
}
void CBuffer::Base16Encode(LPBYTE buf, int len)
{
	Clear();
    while ( len > 0 ) {
		Put8Bit(QuotedEncTab[*buf >> 4]);
		Put8Bit(QuotedEncTab[*buf & 15]);
		buf += 1;
		len -= 1;
    }
	Put8Bit('\0');
}
void CBuffer::md5(LPCSTR str)
{
	unsigned int dlen;
	const EVP_MD *evp_md;
	u_char digest[EVP_MAX_MD_SIZE];
	EVP_MD_CTX md;

	evp_md = EVP_md5();
	EVP_DigestInit(&md, evp_md);
	EVP_DigestUpdate(&md, (LPBYTE)str, (int)strlen(str));
	EVP_DigestFinal(&md, digest, &dlen);

	Base16Encode(digest, dlen);
}

//////////////////////////////////////////////////////////////////////
// CStringArrayExt

void CStringArrayExt::AddBin(void *buf, int len)
{
	CBuffer tmp;

	tmp.Base64Encode((LPBYTE)buf, len);
	Add((LPCSTR)tmp);
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
void CStringArrayExt::GetString(LPCSTR str, int sep)
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
void CStringArrayExt::AddArray(CStringArrayExt &array)
{
	CString tmp;
	array.SetString(tmp);
	Add(tmp);
}
void CStringArrayExt::GetArray(int index, CStringArrayExt &array)
{
	array.GetString(GetAt(index));
}
void CStringArrayExt::SetBuffer(CBuffer &buf)
{
	buf.Put32Bit((int)GetSize());
	for ( int n = 0 ; n < GetSize() ; n++ )
		buf.PutStr(GetAt(n));
}
void CStringArrayExt::GetBuffer(CBuffer &buf)
{
	int n, mx;
	CString str;

	RemoveAll();
	if ( buf.GetSize() < 4 )
		return;
	mx = buf.Get32Bit();
	for ( n = 0 ; n < mx ; n++ ) {
		buf.GetStr(str);
		Add(str);
	}
}
void CStringArrayExt::Serialize(CArchive& ar)
{
	int n;
	CString str;

	if ( ar.IsStoring() ) {
		// TODO: この位置に保存用のコードを追加してください。
		for ( n = 0 ; n < GetSize() ; n++ ) {
			str = GetAt(n);
			str += "\n";
			ar.WriteString(str);
		}
		ar.WriteString("ENDOF\n");

	} else {
		// TODO: この位置に読み込み用のコードを追加してください。
		RemoveAll();
		while ( ar.ReadString(str) ) {
			str.Replace("\n", "");
			if ( str.Compare("ENDOF") == 0 )
				break;
			Add(str);
		}
	}
}
const CStringArrayExt & CStringArrayExt::operator = (CStringArrayExt &data)
{
	RemoveAll();
	for ( int n = 0 ; n < data.GetSize() ; n++ )
		Add(data[n]);
	return *this;
}
int CStringArrayExt::Find(LPCSTR str)
{
	for ( int n = 0 ; n < GetSize() ; n++ ) {
		if ( GetAt(n).Compare(str) == 0 )
			return n;
	}
	return (-1);
}
int CStringArrayExt::FindNoCase(LPCSTR str)
{
	for ( int n = 0 ; n < GetSize() ; n++ ) {
		if ( GetAt(n).CompareNoCase(str) == 0 )
			return n;
	}
	return (-1);
}

//////////////////////////////////////////////////////////////////////
// CBmpFile

CBmpFile::CBmpFile()
{
	m_pPic = NULL;
}

CBmpFile::~CBmpFile()
{
	if ( m_pPic != NULL )
		m_pPic->Release();
}

int CBmpFile::LoadFile(LPCSTR filename)
{
	CFile file;
	HGLOBAL hGLB;
	void *pBuf;
	ULONGLONG FileSize;
    LPSTREAM pStm;

	if ( m_FileName.Compare(filename) == 0 )
		return FALSE;
	m_FileName = filename;

	if ( m_pPic != NULL )
		m_pPic->Release();
	m_pPic = NULL;

	if ( m_Bitmap.m_hObject != NULL )
		m_Bitmap.DeleteObject();

	if( !file.Open(filename, CFile::modeRead) )
		return FALSE;

	if ( (FileSize = file.GetLength()) > (256 * 1024 * 1024) )
		return FALSE;

	if ( (hGLB = ::GlobalAlloc(GMEM_MOVEABLE, (DWORD)FileSize)) == NULL )
		return FALSE;

	if ( (pBuf = ::GlobalLock(hGLB)) == NULL )
		goto ERROF;

	if ( file.Read(pBuf, (DWORD)FileSize) != (DWORD)FileSize )
		goto ERROF;

	::GlobalUnlock(hGLB);

	pStm = NULL;
	if ( ::CreateStreamOnHGlobal(hGLB, TRUE, &pStm) != S_OK )
		goto ERROF;

    if ( ::OleLoadPicture(pStm, (DWORD)FileSize, FALSE, IID_IPicture, (LPVOID *)&m_pPic) != S_OK )
		goto ERROF2;

	if ( m_Bitmap.m_hObject != NULL )
		m_Bitmap.DeleteObject();

	pStm->Release();
	::GlobalFree(hGLB);
    return TRUE;

ERROF2:
	pStm->Release();
ERROF:
	::GlobalFree(hGLB);
	return FALSE;
}

CBitmap *CBmpFile::GetBitmap(CDC *pDC, int width, int height)
{
	int x, y, cx, cy;
	CDC MemDC;
	CBitmap *pOldMemMap = NULL;
	CRect rect;
	CPoint po;
	OLE_XSIZE_HIMETRIC sx;
	OLE_YSIZE_HIMETRIC sy;

	if ( m_pPic == NULL )
		return NULL;

	if ( m_Bitmap.m_hObject != NULL ) {
		if ( m_Width == width && m_Height == height )
			return (&m_Bitmap);
		m_Bitmap.DeleteObject();
	}

	MemDC.CreateCompatibleDC(pDC);
	MemDC.SetMapMode(pDC->GetMapMode());

	m_Bitmap.CreateCompatibleBitmap(pDC, width, height);
	pOldMemMap = (CBitmap *)MemDC.SelectObject(&m_Bitmap);

	po = MemDC.GetBrushOrg();
	MemDC.SetStretchBltMode(HALFTONE);

	m_pPic->get_Width(&sx);
	m_pPic->get_Height(&sy);
	CSize size(sx, sy);
	pDC->HIMETRICtoDP(&size);

	cx = width  * 100 / size.cx;
	cy = height * 100 / size.cy;

	if ( cx > 100 && cy > 100 ) {
		cx = size.cx;
		cy = size.cy;
	} else if ( cx < cy ) {
		cx = width;
		cy = width  * size.cy / size.cx;
	} else {
		cx = height * size.cx / size.cy;
		cy = height;
	}

	m_pPic->Render(MemDC, 0, 0, cx, cy, 0, sy, sx, -sy, NULL);
	for ( y = 0 ; y < height ; y += cy ) {
		for ( x = 0 ; x < width ; x += cx ) {
			if ( x > 0 || y > 0 )
				MemDC.BitBlt(x, y, cx, cy, &MemDC, 0, 0, SRCCOPY);
		}
	}

	m_Width  = width;
	m_Height = height;

	MemDC.SelectObject(pOldMemMap);
	MemDC.SetBrushOrg(po);

	return (&m_Bitmap);
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
	m_Style = 0;
	m_KanWidMul = 100;
}
CFontChacheNode::~CFontChacheNode()
{
	if ( m_pFont != NULL )
		delete m_pFont;
}
CFont *CFontChacheNode::Open(LPCSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality)
{
    strcpy(m_LogFont.lfFaceName, pFontName);
	m_LogFont.lfWidth       = Width;
	m_LogFont.lfHeight		= Height;
	m_LogFont.lfCharSet		= CharSet;
	m_LogFont.lfWeight      = ((Style & 1) != 0 ? FW_BOLD : FW_DONTCARE);
	m_LogFont.lfItalic      = ((Style & 2) != 0 ? TRUE : FALSE);
	m_LogFont.lfUnderline	= ((Style & 4) != 0 ? TRUE : FALSE);
	m_LogFont.lfQuality     = Quality;
	m_Style = Style;

	if ( m_pFont != NULL )
		delete m_pFont;
	m_pFont = new CFont;

	if ( !m_pFont->CreateFontIndirect(&m_LogFont) ) {
		delete m_pFont;
		m_pFont = NULL;
		return NULL;
	}

	CDC dc;
	CSize sz;
	CFont *pOld;

	dc.CreateCompatibleDC(NULL);
	pOld = dc.SelectObject(m_pFont);
	sz = dc.GetTextExtent("亜", 2);
	dc.SelectObject(pOld);

	m_KanWidMul = Width * 200 / sz.cx;

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
CFontChacheNode *CFontChache::GetFont(LPCSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality, int Hash)
{
	CFontChacheNode *pNext, *pBack;

	Hash = (Hash + Width + Height + Style) & 3;
	pNext = pBack = m_pTop[Hash];
	for ( ; ; ) {
		if ( pNext->m_pFont != NULL &&
			 pNext->m_LogFont.lfCharSet == CharSet &&
			 pNext->m_LogFont.lfWidth   == Width   &&
			 pNext->m_LogFont.lfHeight  == Height  &&
			 pNext->m_LogFont.lfQuality == Quality &&
			 pNext->m_Style             == Style   &&
			 strcmp(pNext->m_LogFont.lfFaceName, pFontName) == 0 ) {
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

	TRACE("CacheMiss %s(%d,%d,%d,%d)\n", pFontName, CharSet, Height, Hash, Style);

	if ( pNext->Open(pFontName, Width, Height, CharSet, Style, Quality) == NULL )
		return NULL;

	pBack->m_pNext = pNext->m_pNext;
	pNext->m_pNext = m_pTop[Hash];
	m_pTop[Hash] = pNext;
	return pNext;
}

//////////////////////////////////////////////////////////////////////
// CMutexLock

CMutexLock::CMutexLock(LPCSTR lpszName)
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
	m_pSection = "OptObject";
	m_MinSize = 1;
}
void COptObject::Init()
{
}
void COptObject::Serialize(int mode)
{
	CStringArrayExt array;

	if ( mode ) {	// Write
		SetArray(array);
		((CRLoginApp *)AfxGetApp())->WriteProfileArray(m_pSection, array);
	} else {		// Read
		((CRLoginApp *)AfxGetApp())->GetProfileArray(m_pSection, array);
		if ( array.GetSize() < m_MinSize )
			Init();
		else
			GetArray(array);
	}
}
void COptObject::Serialize(int mode, CBuffer &buf)
{
	CStringArrayExt array;

	if ( mode ) {	// Write
		SetArray(array);
		array.SetBuffer(buf);
	} else {		// Read
		array.GetBuffer(buf);
		if ( array.GetSize() < m_MinSize )
			Serialize(FALSE);
		else
			GetArray(array);
	}
}
void COptObject::Serialize(CArchive &ar)
{
	CStringArrayExt array;

	if ( ar.IsStoring() ) {		// TODO: この位置に保存用のコードを追加してください。
		SetArray(array);
		array.Serialize(ar);
	} else {					// TODO: この位置に読み込み用のコードを追加してください。
		array.Serialize(ar);
		if ( array.GetSize() < m_MinSize )
			Serialize(FALSE);
		else
			GetArray(array);
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
		buf.PutStr(np->m_RecvStr);
		buf.PutStr(np->m_SendStr);
		SetNode(np->m_Left,  buf);
		SetNode(np->m_Right, buf);
	}
}
CStrScriptNode *CStrScript::GetNode(CBuffer &buf)
{
	CStrScriptNode *np;

	if ( buf.Get8Bit() == 0 )
		return NULL;

	np = new CStrScriptNode;
	buf.GetStr(np->m_RecvStr);
	buf.GetStr(np->m_SendStr);
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
LPCSTR CStrScript::QuoteStr(CString &tmp, LPCSTR str)
{
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
}
void CStrScript::SetNodeStr(CStrScriptNode *np, CString &str, int nst)
{
	int n;
	CString tmp;
	LPCSTR cmd = "if ";

	if ( np == NULL )
		return;

	while ( np != NULL ) {
		for ( n = 0 ; n < nst ; n++ )
			str += "  ";

		str += cmd;
		str += QuoteStr(tmp, np->m_RecvStr);

		if ( !np->m_SendStr.IsEmpty() ) {
			str += " then ";
			str += QuoteStr(tmp, np->m_SendStr);
		}
		str += "\r\n";

		SetNodeStr(np->m_Right, str, nst + 1);

		np = np->m_Left;
		cmd = "or ";
	}

	for ( n = 0 ; n < nst ; n++ )
		str += "  ";
	str += "fi\r\n";
}
int CStrScript::GetLex(LPCSTR &str)
{
	m_LexTmp.Empty();

	while ( *str != '\0' && *str <= ' ' )
		str++;

	if ( *str == '\0' ) {
		return EOF;

	} else if ( *str == '"' ) {
		str++;
		while ( *str != '\0' && *str != '"' ) {
			if ( issjis1(str[0]) && issjis2(str[1]) )
				m_LexTmp += *(str++);
			m_LexTmp += *(str++);
		}
		if ( *str != '\0' )
			str++;
		return 0;
	}

	while ( *str != '\0' && *str > ' ' ) {
		if ( issjis1(str[0]) && issjis2(str[1]) )
			m_LexTmp += *(str++);
		m_LexTmp += *(str++);
	}

	if ( m_LexTmp.CollateNoCase("if") == 0 )
		return 1;
	else if ( m_LexTmp.CollateNoCase("then") == 0 )
		return 2;
	else if ( m_LexTmp.CollateNoCase("or") == 0 )
		return 3;
	else if ( m_LexTmp.CollateNoCase("fi") == 0 )
		return 4;
	else
		return 0;
}
CStrScriptNode *CStrScript::GetNodeStr(int &lex, LPCSTR &str)
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
void CStrScript::GetString(LPCSTR str)
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

	for ( buf.Empty() ; *str != '\0' ; str++ ) {
		if ( *str <= ' ' || *str == '\\' || *str == 0x7F ) {
			switch(*str) {
			case 0x07: buf += "\\a"; break;
			case 0x08: buf += "\\b"; break;
			case 0x09: buf += "\\t"; break;
			case 0x0A: buf += "\\n"; break;
			case 0x0B: buf += "\\v"; break;
			case 0x0C: buf += "\\f"; break;
			case 0x0D: buf += "\\r"; break;
			case ' ':  buf += " ";   break;
			case '\\': buf += "\\\\"; break;
			default:
				tmp.Format("\\%03o", *str);
				buf += tmp;
				break;
			}
			if ( reg ) {
				for ( n = 1 ; str[0] == str[1] ; n++ )
					str++;
				if ( n > 1 )
					buf += '+';
			}
		} else if ( reg ) {
			switch(*str) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				for ( n = 1 ; str[1] >= '0' && str[1] <= '9' ; n++ )
					str++;
				buf += (n > 1 ? "[0-9]+" : "[0-9]");
				break;
			case '[': case '^': case '$': case '|':
			case '+': case '*': case '.': case '?':
			case '{': case '(':
				buf += "\\";
			default:
				buf += *str;
				for ( n = 1 ; str[0] == str[n] ; n++ );
				if ( n > 2 ) {
					buf += '+';
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

	if ( *recv == '\0' && send == '\0' )
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
		m_StatDlg.m_Title.SetWindowText(m_MakeFlag ? "Makeing" : "Execute");
		m_StatDlg.SetStaus("");
		AfxGetMainWnd()->SetFocus();
	}
}
LPCWSTR CStrScript::ExecChar(int ch)
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
				tmp += " or ";
			tmp += np->m_RecvStr;
			if ( np->m_Reg.MatchChar(ch, 0, &m_Res) && (m_Res.m_Status == REG_MATCH || m_Res.m_Status == REG_MATCHOVER) ) {
				ExecNode(np->m_Right);
				np->m_Reg.ConvertRes((CStringW)np->m_SendStr, m_Str, &m_Res);
				if ( m_StatDlg.m_hWnd != NULL )
					m_StatDlg.SetStaus("");
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
		if ( *str == '\r' ) {
			if ( ep != NULL ) {
				if ( ep->m_EntryName.Compare((CString)m_Str) == 0 )
					m_Str = "%E";
				else if ( ep->m_UserName.Compare((CString)m_Str) == 0 )
					m_Str = "%U";
				else if ( ep->m_PassName.Compare((CString)m_Str) == 0 )
					m_Str = "%P";
				else if ( ep->m_TermName.Compare((CString)m_Str) == 0 )
					m_Str = "%T";
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

	tmp.Format("%s/%s", np->m_RecvStr, np->m_SendStr);

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
	m_TermName.Empty();
	m_IdkeyName.Empty();
	m_KanjiCode = EUC_SET;
	m_ProtoType = PROTO_DIRECT;
	m_ProBuffer.Clear();
	m_HostReal.Empty();
	m_UserReal.Empty();
	m_PassReal.Empty();
	m_SaveFlag  = FALSE;
	m_CheckFlag = FALSE;
	m_Uid       = (-1);
	m_Script.Init();
	m_ProxyMode = 0;
	m_ProxyHost.Empty();
	m_ProxyPort.Empty();
	m_ProxyUser.Empty();
	m_ProxyPass.Empty();
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
	m_HostReal   = data.m_HostReal;
	m_UserReal   = data.m_UserReal;
	m_PassReal   = data.m_PassReal;
	m_SaveFlag   = data.m_SaveFlag;
	m_CheckFlag  = data.m_CheckFlag;
	m_Uid        = data.m_Uid;
	m_Script     = data.m_Script;
	m_ProxyMode  = data.m_ProxyMode;
	m_ProxyHost  = data.m_ProxyHost;
	m_ProxyPort  = data.m_ProxyPort;
	m_ProxyUser  = data.m_ProxyUser;
	m_ProxyPass  = data.m_ProxyPass;
	return *this;
}
void CServerEntry::GetArray(CStringArrayExt &array)
{
	CIdKey key;
	CBuffer buf;

	m_EntryName = array.GetAt(0);
	m_HostName  = array.GetAt(1);
	m_PortName  = array.GetAt(2);
	m_UserName  = array.GetAt(3);
	key.DecryptStr(m_PassName, array.GetAt(4));
	m_TermName  = array.GetAt(5);
	m_IdkeyName = array.GetAt(6);
	m_KanjiCode = array.GetVal(7);
	m_ProtoType = array.GetVal(8);

	m_Uid = (array.GetSize() > 9 ? array.GetVal(9):(-1));

	if ( array.GetSize() > 10 ) {
		array.GetBuf(10, buf);
		m_Script.GetBuffer(buf);
	} else
		m_Script.Init();

	if ( array.GetSize() > 15 ) {
		m_ProxyMode = array.GetVal(11);
		m_ProxyHost = array.GetAt(12);
		m_ProxyPort = array.GetAt(13);
		m_ProxyUser = array.GetAt(14);
		key.DecryptStr(m_ProxyPass, array.GetAt(15));
	}

	m_ProBuffer.Clear();
	m_HostReal = m_HostName;
	m_UserReal = m_UserName;
	m_PassReal = m_PassName;
	m_SaveFlag = FALSE;
}
void CServerEntry::SetArray(CStringArrayExt &array)
{
	CIdKey key;
	CString str;
	CBuffer buf;

	array.RemoveAll();
	array.Add(m_EntryName);
	array.Add(m_HostReal);
	array.Add(m_PortName);
	array.Add(m_UserReal);
	key.EncryptStr(str, m_PassReal);
	array.Add(str);
	array.Add(m_TermName);
	array.Add(m_IdkeyName);
	array.AddVal(m_KanjiCode);
	array.AddVal(m_ProtoType);
	array.AddVal(m_Uid);
	m_Script.SetBuffer(buf);
	array.AddBin(buf.GetPtr(), buf.GetSize());
	array.AddVal(m_ProxyMode);
	array.Add(m_ProxyHost);
	array.Add(m_ProxyPort);
	array.Add(m_ProxyUser);
	key.EncryptStr(str, m_ProxyPass);
	array.Add(str);
}
void CServerEntry::SetBuffer(CBuffer &buf)
{
	CStringArrayExt array;
	SetArray(array);
	array.SetBuffer(buf);
	buf.PutBuf(m_ProBuffer.GetPtr(), m_ProBuffer.GetSize());
}
int CServerEntry::GetBuffer(CBuffer &buf)
{
	CStringArrayExt array;
	array.GetBuffer(buf);
	if ( array.GetSize() < 9 )
		return FALSE; 
	GetArray(array);
	buf.GetBuf(&m_ProBuffer);
	return TRUE;
}
void CServerEntry::SetProfile(LPCSTR pSection)
{
	CBuffer buf;
	CString entry;
	entry.Format("List%02d", m_Uid);
	SetBuffer(buf);
	((CRLoginApp *)AfxGetApp())->WriteProfileBinary(pSection, entry, buf.GetPtr(), buf.GetSize());
}
int CServerEntry::GetProfile(LPCSTR pSection, int Uid)
{
	CBuffer buf;
	CString entry;
	entry.Format("List%02d", Uid);
	((CRLoginApp *)AfxGetApp())->GetProfileBuffer(pSection, entry, buf);
	return GetBuffer(buf);
}
void CServerEntry::DelProfile(LPCSTR pSection)
{
	CString entry;
	entry.Format("List%02d", m_Uid);
	((CRLoginApp *)AfxGetApp())->DelProfileEntry(pSection, entry);
}
void CServerEntry::Serialize(CArchive &ar)
{
	CStringArrayExt array;

	if ( ar.IsStoring() ) {		// TODO: この位置に保存用のコードを追加してください。
		SetArray(array);
		array.Serialize(ar);
	} else {					// TODO: この位置に読み込み用のコードを追加してください。
		array.Serialize(ar);
		if ( array.GetSize() < 9 )
			Init();
		else
			GetArray(array);
	}
}
LPCSTR CServerEntry::GetKanjiCode()
{
	switch(m_KanjiCode) {
	case SJIS_SET:	return "SJIS";
	case ASCII_SET:	return "ASCII";
	case UTF8_SET:	return "UTF8";
	default:		return "EUC";
	}
}
void CServerEntry::SetKanjiCode(LPCSTR str)
{
	if (      strcmp(str, "EUC") == 0 )		m_KanjiCode = EUC_SET;
	else if ( strcmp(str, "SJIS") == 0 )	m_KanjiCode = SJIS_SET;
	else if ( strcmp(str, "ASCII") == 0 )	m_KanjiCode = ASCII_SET;
	else if ( strcmp(str, "UTF8") == 0 )	m_KanjiCode = UTF8_SET;
	else									m_KanjiCode = EUC_SET;
}
int CServerEntry::GetProtoType(LPCSTR str)
{
	if ( strcmp(str, "serial") == 0 )
		return PROTO_COMPORT;
	else if ( strcmp(str, "telnet") == 0 )
		return PROTO_TELNET;
	else if ( strcmp(str, "login") == 0 )
		return PROTO_LOGIN;
	else if ( strcmp(str, "ssh") == 0 )
		return PROTO_SSH;
	else
		return PROTO_DIRECT;
}

//////////////////////////////////////////////////////////////////////
// CServerEntryTab

CServerEntryTab::CServerEntryTab()
{
	m_pSection = "ServerEntryTab";
	m_MinSize = 1;
	Serialize(FALSE);
}
void CServerEntryTab::Init()
{
	m_Data.RemoveAll();
}
void CServerEntryTab::SetArray(CStringArrayExt &array)
{
	CStringArrayExt tmp;

	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_Data[n].SetArray(tmp);
		array.AddArray(tmp);
	}
}
void CServerEntryTab::GetArray(CStringArrayExt &array)
{
	CStringArrayExt tmp;
	CServerEntry dum;

	m_Data.RemoveAll();
	for ( int n = 0 ; n < array.GetSize() ; n++ ) {
		array.GetArray(n, tmp);
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
			if ( entry[n].Left(4).Compare("List") != 0 || strchr("0123456789", entry[n][4]) == NULL )
				continue;
			uid = atoi(entry[n].Mid(4));
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
	Entry.m_Uid = ((CRLoginApp *)AfxGetApp())->GetProfileSeqNum(m_pSection, "ListMax");
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

//////////////////////////////////////////////////////////////////////
// CKeyNode

CKeyNode::CKeyNode()
{
	m_Code = (-1);
	m_Mask = 0;
	m_Maps.Clear();
}
LPCSTR CKeyNode::GetMaps()
{
	int n, c;
	LPCWSTR p;
	CString tmp;
	
	m_Temp = "";
	p = m_Maps;
	for ( n = 0 ; n < m_Maps.GetSize() ; n += 2, p++ ) {
		switch(*p) {
		case L'\b': m_Temp += "\\b"; break;
		case L'\t': m_Temp += "\\t"; break;
		case L'\n': m_Temp += "\\n"; break;
		case L'\a': m_Temp += "\\a"; break;
		case L'\f': m_Temp += "\\f"; break;
		case L'\r': m_Temp += "\\r"; break;
		case L'\\': m_Temp += "\\\\"; break;
		case L' ':  m_Temp += "\\s"; break;
		default:
			if ( *p == L'\x7F' || *p < L' ' ) {
				tmp.Format("\\%03o", *p);
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
void CKeyNode::SetMaps(LPCSTR str)
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
	_TCHAR	*name;
} KeyNameTab[] = {
	{ VK_F1,		"F1" },
	{ VK_F2,		"F2" },
	{ VK_F3,		"F3" },
	{ VK_F4,		"F4" },
	{ VK_F5,		"F5" },
	{ VK_F6,		"F6" },
	{ VK_F7,		"F7" },
	{ VK_F8,		"F8" },
	{ VK_F9,		"F9" },
	{ VK_F10,		"F10" },
	{ VK_F11,		"F11" },
	{ VK_F12,		"F12" },
	{ VK_F13,		"F13" },
	{ VK_F14,		"F14" },
	{ VK_F15,		"F15" },
	{ VK_F16,		"F16" },
	{ VK_F17,		"F17" },
	{ VK_F18,		"F18" },
	{ VK_F19,		"F19" },
	{ VK_F20,		"F20" },
	{ VK_F21,		"F21" },
	{ VK_F22,		"F22" },
	{ VK_F23,		"F23" },
	{ VK_F24,		"F24" },
	{ VK_UP,		"UP" },
	{ VK_DOWN,		"DOWN" },
	{ VK_RIGHT,		"RIGHT" },
	{ VK_LEFT,		"LEFT" },
	{ VK_PRIOR,		"PRIOR" },
	{ VK_NEXT,		"NEXT" },
	{ VK_INSERT,	"INSERT" },
	{ VK_DELETE,	"DELETE" },
	{ VK_HOME,		"HOME" },
	{ VK_END,		"END" },
	{ VK_PAUSE,		"PAUSE" },
	{ VK_CANCEL,	"BREAK" },
	{ VK_ESCAPE,	"ESCAPE" },
	{ VK_RETURN,	"RETURN" },
	{ VK_BACK,		"BACK" },
	{ VK_TAB,		"TAB" },
	{ VK_SPACE,		"SPACE" },
	{ VK_OEM_1,		"$BA(:)" },
	{ VK_OEM_PLUS,	"$BB(;)" },
	{ VK_OEM_COMMA,	"$BC(,)" },
	{ VK_OEM_MINUS,	"$BD(-)" },
	{ VK_OEM_PERIOD,"$BE(.)" },
	{ VK_OEM_2,		"$BF(/)" },
	{ VK_OEM_3,		"$C0(@)" },
	{ VK_OEM_4,		"$DB([)" },
	{ VK_OEM_5,		"$DC(\\)" },
	{ VK_OEM_6,		"$DD(])" },
	{ VK_OEM_7,		"$DE(^)" },
	{ VK_OEM_102,	"$E2(_)" },

	{ (-1),			NULL },
};

LPCSTR CKeyNode::GetCode()
{
	int n;
	if ( m_Code == (-1) )
		return "";
	for ( n = 0 ; KeyNameTab[n].name != NULL ; n++ ) {
		if ( KeyNameTab[n].code == m_Code )
			return KeyNameTab[n].name;
	}
	// * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
	// * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
	if ( m_Code >= 0x30 && m_Code <= 0x39 || m_Code >= 0x41 && m_Code <= 0x5A )
		m_Temp.Format("%c", m_Code);
	else
		m_Temp.Format("$%02X", m_Code);

	return m_Temp;
}
void CKeyNode::SetCode(LPCSTR name)
{
	int n;
	for ( n = 0 ; KeyNameTab[n].name != NULL ; n++ ) {
		if ( strcmp(KeyNameTab[n].name, name) == 0 ) {
			m_Code = KeyNameTab[n].code;
			return;
		}
	}
	if ( name[1] == '\0' && (*name >= 0x30 && *name >= 0x39 || *name >= 0x41 && *name <= 0x5A) ) {
		m_Code = *name;
	} else if ( name[0] == '$' && isxdigit(name[1]) && isxdigit(name[2]) ) {
		m_Code = 0;
		if ( isdigit(name[1]) )
			m_Code += ((name[1] - '0') * 16);
		else
			m_Code += ((toupper(name[1])- 'A') * 16);
		if ( isdigit(name[0]) )
			m_Code += (name[0] - '0');
		else
			m_Code += (toupper(name[0])- 'A');
	} else
		m_Code = atoi(name);
}
LPCSTR CKeyNode::GetMask()
{
	m_Temp = "";
	if ( (m_Mask & MASK_SHIFT) ) m_Temp += "Shift+";
	if ( (m_Mask & MASK_CTRL) )  m_Temp += "Ctrl+";
	if ( (m_Mask & MASK_ALT) )   m_Temp += "Alt+";
	if ( (m_Mask & MASK_APPL) )  m_Temp += "Appl+";

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
		str.Format("%c", n);
		pCombo->AddString(str);
	}
	for ( n = 0x41 ; n <= 0x5A ; n++ ) {
		str.Format("%c", n);
		pCombo->AddString(str);
	}
}

//////////////////////////////////////////////////////////////////////
// CKeyCmds

const CKeyCmds & CKeyCmds::operator = (CKeyCmds &data)
{
	m_Id = data.m_Id;
	m_Menu = data.m_Menu;

	return *this;
}

//////////////////////////////////////////////////////////////////////
// CKeyNodeTab

CKeyNodeTab::CKeyNodeTab()
{
	m_CmdsInit = FALSE;
	m_pSection = "KeyTab";
	Init();
}
void CKeyNodeTab::Init()
{
	int n;
	static const struct {
		int	code;
		int mask;
		_TCHAR *maps;
	} InitKeyTab[] = {
		{ VK_UP,	0,			"\\033[A" },
		{ VK_DOWN,	0,			"\\033[B" },
		{ VK_RIGHT,	0,			"\\033[C" },
		{ VK_LEFT,	0,			"\\033[D" },
		{ VK_END,	0,			"\\033[E" },
		{ VK_NEXT,	0,			"\\033[G" },
		{ VK_HOME,	0,			"\\033[H" },
		{ VK_PRIOR,	0,			"\\033[I" },
		{ VK_INSERT,0,			"\\033[L" },
		{ VK_F1,	0,			"\\033[M" },
		{ VK_F2,	0,			"\\033[N" },
		{ VK_F3,	0,			"\\033[O" },
		{ VK_F4,	0,			"\\033[P" },
		{ VK_F5,	0,			"\\033[Q" },
		{ VK_F6,	0,			"\\033[R" },
		{ VK_F7,	0,			"\\033[S" },
		{ VK_F8,	0,			"\\033[T" },
		{ VK_F9,	0,			"\\033[U" },
		{ VK_F10,	0,			"\\033[V" },
		{ VK_F11,	0,			"\\033[W" },
		{ VK_F12,	0,			"\\033[X" },
		{ VK_DELETE,0,			"\\177" },

		{ VK_HOME,	MASK_APPL,	"\\033[1~" },	//	kh	@0
		{ VK_INSERT,MASK_APPL,	"\\033[2~" },	//	kI
		{ VK_DELETE,MASK_APPL,	"\\033[3~" },	//	kD
		{ VK_END,	MASK_APPL,	"\\033[4~" },	//	@7	*6
		{ VK_PRIOR,	MASK_APPL,	"\\033[5~" },	//	kP
		{ VK_NEXT,	MASK_APPL,	"\\033[6~" },	//	kN

		{ VK_F1,	MASK_APPL,	"\\033[11~" },	//	k1
		{ VK_F2,	MASK_APPL,	"\\033[12~" },	//	k2
		{ VK_F3,	MASK_APPL,	"\\033[13~" },	//	k3
		{ VK_F4,	MASK_APPL,	"\\033[14~" },	//	k4
		{ VK_F5,	MASK_APPL,	"\\033[15~" },	//	k5
		{ VK_F6,	MASK_APPL,	"\\033[17~" },	//	k6
		{ VK_F7,	MASK_APPL,	"\\033[18~" },	//	k7
		{ VK_F8,	MASK_APPL,	"\\033[19~" },	//	k8
		{ VK_F9,	MASK_APPL,	"\\033[20~" },	//	k9
		{ VK_F10,	MASK_APPL,	"\\033[21~" },	//	k;
		{ VK_F11,	MASK_APPL,	"\\033[23~" },	//	F1
		{ VK_F12,	MASK_APPL,	"\\033[24~" },	//	F2
		{ VK_F13,	MASK_APPL,	"\\033[25~" },	//	F3
		{ VK_F14,	MASK_APPL,	"\\033[26~" },	//	F4
		{ VK_F15,	MASK_APPL,	"\\033[28~" },	//	F5
		{ VK_F16,	MASK_APPL,	"\\033[29~" },	//	F6
		{ VK_F17,	MASK_APPL,	"\\033[31~" },	//	F7
		{ VK_F18,	MASK_APPL,	"\\033[32~" },	//	F8
		{ VK_F19,	MASK_APPL,	"\\033[33~" },	//	F9
		{ VK_F20,	MASK_APPL,	"\\033[34~" },	//	FA

		{ VK_UP,	MASK_APPL,	"\\033OA" },	//	ku
		{ VK_DOWN,	MASK_APPL,	"\\033OB" },	//	kd
		{ VK_RIGHT,	MASK_APPL,	"\\033OC" },	//	kr
		{ VK_LEFT,	MASK_APPL,	"\\033OD" },	//	kl

		{ VK_PRIOR,	MASK_SHIFT,	"$PRIOR" },
		{ VK_NEXT,	MASK_SHIFT,	"$NEXT" },
		{ VK_PRIOR,	MASK_SHIFT | MASK_APPL,	"$PRIOR" },
		{ VK_NEXT,	MASK_SHIFT | MASK_APPL,	"$NEXT" },

		{ VK_CANCEL,0,			"$BREAK" },
//		{ VK_CANCEL,MASK_CTRL,	"$BREAK" },

		{ VK_OEM_7,	MASK_CTRL,	"\\036"	},
		{ VK_OEM_2,	MASK_CTRL,	"\\037"	},
		{ VK_OEM_3,	MASK_CTRL,	"\\000"	},
		{ VK_SPACE,	MASK_CTRL,	"\\000"	},

		{ (-1),		(-1),		NULL },
	};

	m_Node.RemoveAll();
	for ( n = 0 ; InitKeyTab[n].maps != NULL ; n++ )
		Add(InitKeyTab[n].code, InitKeyTab[n].mask, InitKeyTab[n].maps);
}
int CKeyNodeTab::Add(CKeyNode &node)
{
	int c;
	int n;
	int b = 0;
	int m = (int)m_Node.GetSize() - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = m_Node[n].m_Code - node.m_Code) == 0 && (c = m_Node[n].m_Mask - node.m_Mask) == 0 ) {
			m_Node[n] = node;
			return n;
		} else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}
	m_Node.InsertAt(b, node);
	m_CmdsInit = FALSE;
	return b;
}
int CKeyNodeTab::Add(LPCSTR code, int mask, LPCSTR maps)
{
	CKeyNode tmp;
	tmp.SetCode(code);
	tmp.m_Mask = mask;
	tmp.SetMaps(maps);
	return Add(tmp);
}
int CKeyNodeTab::Add(int code, int mask, LPCSTR str)
{
	CKeyNode tmp;
	tmp.m_Code = code;
	tmp.m_Mask = mask;
	tmp.SetMaps(str);
	return Add(tmp);
}
BOOL CKeyNodeTab::FindMaps(int code, int mask, CBuffer *pBuf)
{
	int c;
	int n;
	int b = 0;
	int m = (int)m_Node.GetSize() - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = m_Node[n].m_Code - code) == 0 && (c = m_Node[n].m_Mask - mask) == 0 ) {
			*pBuf = m_Node[n].m_Maps;
			return TRUE;
		} else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}

	if ( (mask & MASK_SHIFT) != 0 && FindMaps(code, mask & ~MASK_SHIFT, pBuf) )
		return TRUE;
	else if ( (mask & MASK_APPL) != 0 && FindMaps(code, mask & ~MASK_APPL, pBuf) )
		return TRUE;

	return FALSE;
}
void CKeyNodeTab::SetArray(CStringArrayExt &array)
{
	int n;
	CStringArrayExt tmp;

	array.RemoveAll();
	for ( n = 0 ; n < m_Node.GetSize() ; n++ ) {
		if ( m_Node[n].m_Code == (-1) )
			continue;
		tmp.RemoveAll();
		tmp.AddVal(m_Node[n].m_Code);
		tmp.AddVal(m_Node[n].m_Mask);
		tmp.Add(m_Node[n].GetMaps());
		array.AddArray(tmp);
	}
}
void CKeyNodeTab::GetArray(CStringArrayExt &array)
{
	int n;
	CStringArrayExt tmp;

	m_Node.RemoveAll();
	for ( n = 0 ; n < array.GetSize() ; n++ ) {
		array.GetArray(n, tmp);
		if ( tmp.GetSize() < 3 )
			continue;
		Add(tmp.GetVal(0), tmp.GetVal(1), tmp.GetAt(2));
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

static const struct {
	int	code;
	LPCWSTR name;
} CmdsKeyTab[] = {
#define	CMDSKEYTABMAX	54
	{	ID_APP_ABOUT,			L"$ABOUT"			},
	{	ID_SEND_BREAK,			L"$BREAK"			},
	{	IDM_BROADCAST,			L"$BROADCAST"		},
	{	ID_EDIT_COPY,			L"$EDIT_COPY"		},
	{	ID_EDIT_COPY_ALL,		L"$EDIT_COPYALL"	},
	{	ID_EDIT_PASTE,			L"$EDIT_PASTE"		},
	{	ID_APP_EXIT,			L"$EXIT"			},
	{	ID_FILE_ALL_SAVE,		L"$FILE_ALLSAVE"	},
	{	ID_FILE_CLOSE,			L"$FILE_CLOSE"		},
	{	ID_FILE_NEW,			L"$FILE_NEW"		},
	{	ID_FILE_OPEN,			L"$FILE_OPEN"		},
	{	ID_FILE_SAVE_AS,		L"$FILE_SAVE"		},
	{	IDM_KANJI_ASCII,		L"$KANJI_ASCII"		},
	{	IDM_KANJI_EUC,			L"$KANJI_EUC"		},
	{	IDM_KANJI_SJIS,			L"$KANJI_SJIS"		},
	{	IDM_KANJI_UTF8,			L"$KANJI_UTF8"		},
	{	ID_MACRO_HIS1,			L"$KEY_HIS1"		},
	{	ID_MACRO_HIS2,			L"$KEY_HIS2"		},
	{	ID_MACRO_HIS3,			L"$KEY_HIS3"		},
	{	ID_MACRO_HIS4,			L"$KEY_HIS4"		},
	{	ID_MACRO_HIS5,			L"$KEY_HIS5"		},
	{	ID_MACRO_PLAY,			L"$KEY_PLAY"		},
	{	ID_MACRO_REC,			L"$KEY_REC"			},
	{	ID_LOG_OPEN,			L"$LOG_OPEN"		},
	{	ID_MOUSE_EVENT,			L"$MOUSE_EVENT"		},
	{	ID_PAGE_NEXT,			L"$NEXT"			},
	{	IDC_LOADDEFAULT,		L"$OPTION_LOAD"		},
	{	ID_SETOPTION,			L"$OPTION_SET"		},
	{	ID_WINDOW_CASCADE,		L"$PANE_CASCADE"	},
	{	ID_PANE_DELETE,			L"$PANE_DELETE"		},
	{	ID_PANE_HSPLIT,			L"$PANE_HSPLIT"		},
	{	ID_WINDOW_TILE_HORZ,	L"$PANE_TILEHORZ"	},
	{	ID_PANE_WSPLIT,			L"$PANE_WSPLIT"		},
	{	ID_PAGE_PRIOR,			L"$PRIOR"			},
	{	IDM_RESET_ALL,			L"$RESET_ALL"		},
	{	IDM_RESET_ATTR,			L"$RESET_ATTR"		},
	{	IDM_RESET_BANK,			L"$RESET_BANK"		},
	{	IDM_RESET_ESC,			L"$RESET_ESC"		},
	{	IDM_RESET_MOUSE,		L"$RESET_MOUSE"		},
	{	IDM_RESET_TAB,			L"$RESET_TAB"		},
	{	IDM_RESET_TEK,			L"$RESET_TEK"		},
	{	ID_CHARSCRIPT_END,		L"$SCRIPT_END"		},
	{	IDM_SFTP,				L"$VIEW_SFTP"		},
	{	ID_VIEW_STATUS_BAR,		L"$VIEW_STATUSBAR"	},
	{	IDM_TEKDISP,			L"$VIEW_TEKDISP"	},
	{	ID_VIEW_TOOLBAR,		L"$VIEW_TOOLBAR"	},
	{	ID_WINDOW_CLOSE,		L"$WINDOW_CLOSE"	},
	{	ID_WINDOW_NEW,			L"$WINDOW_NEW"		},
	{	IDM_XMODEM_DOWNLOAD,	L"$XMODEM_DOWNLOAD"	},
	{	IDM_XMODEM_UPLOAD,		L"$XMODEM_UPLOAD"	},
	{	IDM_YMODEM_DOWNLOAD,	L"$YMODEM_DOWNLOAD"	},
	{	IDM_YMODEM_UPLOAD,		L"$YMODEM_UPLOAD"	},
	{	IDM_ZMODEM_DOWNLOAD,	L"$ZMODEM_DOWNLOAD"	},
	{	IDM_ZMODEM_UPLOAD,		L"$ZMODEM_UPLOAD"	},
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
		for ( i = 0 ; i < m_Cmds.GetSize() ; i++ ) {
			if ( id == m_Cmds[i].m_Id )
				break;
		}
		str = GetAt(n).GetMask();
		if ( !str.IsEmpty() )
			str += "+";
		str += GetAt(n).GetCode();
		if ( i < m_Cmds.GetSize() ) {
			tmp.m_Menu += ",";
			tmp.m_Menu += str;
		} else {
			tmp.m_Id = id;
			tmp.m_Menu = str;
			m_Cmds.Add(tmp);
		}
	}

	m_CmdsInit = TRUE;
}
int CKeyNodeTab::GetCmdsKey(LPCWSTR str)
{
	if ( str[0] != L'$' )
		return (-1);

	int c;
	int n;
	int b = 0;
	int m = CMDSKEYTABMAX - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = wcscmp(str, CmdsKeyTab[n].name)) == 0 )
			return CmdsKeyTab[n].code;
		else if ( c > 0 )
			b = n + 1;
		else
			m = n - 1;
	}
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
		delete m_Data;
}
void CKeyMac::GetMenuStr(CString &str)
{
	int n;
	CIConv iconv;
	CBuffer in, out;
	LPCSTR p;

	str = "";
	if ( m_Data == NULL )
		return;

	in.Apend(m_Data, m_Len);
	iconv.IConvBuf("UCS-2LE", "CP932", &in, &out);
	p = (LPCSTR)(out.GetPtr());

	for ( n = 0 ; n < out.GetSize() && n < 20 ; n++, p++ ) {
		if ( issjis1(p[0]) && issjis2(p[1]) ) {
			str += *(p++);
			n++;
			str += *p;
		} else if ( *p == '\r' )
			str += _T("↓");
		else if ( *p == '\x7F' || *p < ' ' )
			str += '.';
		else
			str += *p;
	}
	if ( n < out.GetSize() )
		str += "...";
}
void CKeyMac::GetStr(CString &str)
{
	int n;
	LPWSTR p = (LPWSTR)m_Data;
	CString tmp;

	str = "";
	if ( p == NULL )
		return;

	for ( n = 0 ; n < m_Len ; n += 2, p++ ) {
		if ( *p >= ' ' && *p < 0x7F )
			str += (char)(*p);
		else {
			tmp.Format(_T("\\x%04x"), *p);
			str += tmp;
		}
	}
}
void CKeyMac::SetStr(LPCSTR str)
{
	int c, n;
	CBuffer tmp;

	while ( *str != '\0' ) {
		if ( str[0] == '\\' && str[1] == 'x' ) {
			str += 2;
			for ( c = n = 0 ; n < 4 ; n++) {
				if ( *str >= '0' && *str <= '9' )
					c = c * 16 + (*(str++) - '0');
				else if ( *str >= 'A' && *str <= 'F' )
					c = c * 16 + (*(str++) - 'A' + 10);
				else if ( *str >= 'a' && *str <= 'f' )
					c = c * 16 + (*(str++) - 'a' + 10);
				else
					break;
			}
			tmp.PutWord(c);
		} else {
			tmp.PutWord(*(str++) & 0xFF);
		}
	}
	SetBuf(tmp.GetPtr(), tmp.GetSize());
}
void CKeyMac::SetBuf(LPBYTE buf, int len)
{
	if ( m_Data != NULL )
		delete m_Data;
	m_Len = len;
	m_Data = new BYTE[len];
	memcpy(m_Data, buf, len);
}
const CKeyMac & CKeyMac::operator = (CKeyMac &data)
{
	if ( data.m_Data == NULL ) {
		if ( m_Data != NULL )
			delete m_Data;
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
	m_pSection = "KeyMac";
}
void CKeyMacTab::Init()
{
	m_Data.RemoveAll();
}
void CKeyMacTab::SetArray(CStringArrayExt &array)
{
	CString tmp;

	array.RemoveAll();
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_Data[n].GetStr(tmp);
		array.Add(tmp);
	}
}
void CKeyMacTab::GetArray(CStringArrayExt &array)
{
	CKeyMac tmp;

	m_Data.RemoveAll();
	for ( int n = 0 ; n < array.GetSize() ; n++ ) {
		tmp.SetStr(array[n]);
		m_Data.Add(tmp);
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
void CKeyMacTab::SetHisMenu(CWnd *pWnd)
{
	int n;
	CString str, tmp;
	CMenu *pMenu, *pMain;

	if ( pWnd == NULL || (pMain = pWnd->GetMenu()) == NULL|| (pMenu = pMain->GetSubMenu(1)) == NULL )
		return;

	for ( n = 0 ; n < 5 ; n++ )
		pMenu->DeleteMenu(ID_MACRO_HIS1 + n, MF_BYCOMMAND);

	for ( n = 0 ; n < 5 && n < m_Data.GetSize() ; n++ ) {
		m_Data[n].GetMenuStr(tmp);
		str.Format("&%d %s", n + 1, tmp);
		pMenu->AppendMenu(MF_STRING, ID_MACRO_HIS1 + n, str);
	}
}

//////////////////////////////////////////////////////////////////////
// CParamTab

static const char *InitAlgo[9][13] = {
	{ "blowfish", "3des", "des", NULL },
	{ "crc32", NULL },
	{ "zlib", "none", NULL },

	{ "aes128-ctr", "aes192-ctr", "aes256-ctr", "arcfour", "arcfour128", "arcfour256", "blowfish-cbc", "3des-cbc", "cast128-cbc", "aes128-cbc", "aes192-cbc", "aes256-cbc", NULL },
	{ "hmac-sha1", "hmac-md5", "hmac-ripemd160", "hmac-md5-96", "hmac-sha1-96", NULL },
	{ "zlib@openssh.com", "zlib", "none", NULL },

	{ "aes128-ctr", "aes192-ctr", "aes256-ctr", "arcfour", "arcfour128", "arcfour256", "blowfish-cbc", "3des-cbc", "cast128-cbc", "aes128-cbc", "aes192-cbc", "aes256-cbc", NULL },
	{ "hmac-sha1", "hmac-md5", "hmac-ripemd160", "hmac-md5-96", "hmac-sha1-96", NULL },
	{ "zlib@openssh.com", "zlib", "none", NULL },
};

CParamTab::CParamTab()
{
	m_pSection = "ParamTab";
	m_MinSize = 18;
	Init();
}
void CParamTab::Init()
{
	int n, i;

	for ( n = 0 ; n < 9 ; n++ )
		m_IdKeyStr[n] = "";

	for ( n = 0 ; n < 9 ; n++ ) {
		m_AlgoTab[n].RemoveAll();
		for ( i = 0 ; InitAlgo[n][i] != NULL ; i++ )
			m_AlgoTab[n].Add(InitAlgo[n][i]);
	}

	m_PortFwd.RemoveAll();

	m_XDisplay  = ":0";
	m_ExtEnvStr = "";
}
void CParamTab::SetArray(CStringArrayExt &array)
{
	int n;
	CIdKey key;
	CString str;

	array.RemoveAll();

	for ( n = 0 ; n < 9 ; n++ )
		array.Add("");

	array.SetAt(0, "IdKeyList Entry");
	m_IdKeyList.SetString(array[1]);

	for ( n = 0 ; n < 9 ; n++ )
		array.AddArray(m_AlgoTab[n]);

	for ( n = 0 ; n < m_PortFwd.GetSize() ; n++ )
		array.Add(m_PortFwd[n]);
	array.Add("EndOf");

	array.Add(m_XDisplay);
	array.Add(m_ExtEnvStr);
}
void CParamTab::GetArray(CStringArrayExt &array)
{
	int n, a, i = 0;
	CIdKey key;
	CRLoginApp *pApp  = (CRLoginApp *)AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();

	for ( n = 0 ; n < 9 ; n++ ) {
		if ( (n % 3) == 2 )
			key.DecryptStr(m_IdKeyStr[n], array.GetAt(i++));
		else
			m_IdKeyStr[n] = array.GetAt(i++);
	}

	for ( n = 0 ; n < 9 ; n++ ) {
		array.GetArray(i++, m_AlgoTab[n]);
		for ( a = 0 ; InitAlgo[n][a] != NULL ; a++ ) {
			if ( m_AlgoTab[n].Find(InitAlgo[n][a]) < 0 )
				m_AlgoTab[n].Add(InitAlgo[n][a]);
		}
	}

	m_PortFwd.RemoveAll();
	while ( i < array.GetSize() ) {
		if ( array[i].Compare("EndOf") == 0 ) {
			i++;
			break;
		}
		m_PortFwd.Add(array[i++]);
	}

	m_XDisplay  = (array.GetSize() > i ? array.GetAt(i++) : ":0");
	m_ExtEnvStr = (array.GetSize() > i ? array.GetAt(i++) : "");

	if ( m_IdKeyStr[0].Compare("IdKeyList Entry") == 0 ) {
		m_IdKeyList.GetString(m_IdKeyStr[1]);
		for ( n = 0 ; n < 9 ; n++ )
			m_IdKeyStr[n].Empty();
	} else {
		m_IdKeyList.RemoveAll();
		for ( n = 0 ; n < 9 ; n += 3 ) {
			if ( !key.ReadPublicKey(m_IdKeyStr[n + 0]) ||
				 !key.ReadPrivateKey(m_IdKeyStr[n + 1], m_IdKeyStr[n + 2]) )
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
			key.m_Pass = m_IdKeyStr[n + 2];
			pMain->m_IdKeyTab.AddEntry(key);
			m_IdKeyList.AddVal(key.m_Uid);
		}
	}
}
void CParamTab::GetProp(int num, CString &str, int shuffle)
{
	str = "";
	if ( num < 0 || num >= 9 )
		return;

	if ( shuffle ) {
		int n, a, b, c, mx = (int)m_AlgoTab[num].GetSize();
		int *tab = new int[mx];
		for ( n = 0 ; n < mx ; n++ )
			tab[n] = n;
		for ( n = 0 ; n < (mx * 32) ; n++ ) {
			a = rand() % mx;
			b = rand() % mx;
			c = tab[a]; tab[a] = tab[b]; tab[b] = c;
		}
		for ( n = 0 ; n < mx ; n++ ) {
			if ( n > 0 )
				str += ",";
			str += m_AlgoTab[num][tab[n]];
		}
		delete tab;
	} else {
		for ( int i = 0 ; i < m_AlgoTab[num].GetSize() ; i++ ) {
			if ( i > 0 )
				str += ",";
			str += m_AlgoTab[num][i];
		}
	}
}
int CParamTab::GetPropNode(int num, int node, CString &str)
{
	str = "";
	if ( num < 0 || num >= 9 || node < 0 || node >= m_AlgoTab[num].GetSize() )
		return FALSE;
	str = m_AlgoTab[num][node];
	return TRUE;
}
const CParamTab & CParamTab::operator = (CParamTab &data)
{
	for ( int n = 0 ; n < 9 ; n++ ) {
		m_IdKeyStr[n] = data.m_IdKeyStr[n];
		m_AlgoTab[n]  = data.m_AlgoTab[n];
	}
	m_PortFwd   = data.m_PortFwd;
	m_IdKeyList = data.m_IdKeyList;
	m_XDisplay  = data.m_XDisplay;
	m_ExtEnvStr = data.m_ExtEnvStr;
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
int CStringMaps::Add(LPCWSTR wstr)
{
	int c;
	int n;
	int b = 0;
	int m = (int)m_Data.GetSize() - 1;
	CStringW tmp = wstr;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = m_Data[n].CompareNoCase(wstr)) == 0 )
			return n;
		else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}
	m_Data.InsertAt(b, tmp);
	return b;
}
int CStringMaps::Find(LPCWSTR wstr)
{
	int c;
	int n;
	int b = 0;
	int m = (int)m_Data.GetSize() - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( (c = m_Data[n].CompareNoCase(wstr)) == 0 )
			return n;
		else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}
	if ( b >= (int)m_Data.GetSize() )
		return (-1);
	return (m_Data[b].Left((int)wcslen(wstr)).CompareNoCase(wstr) == 0 ? b : (-1));
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
		if ( *wp == '"' ) {
			tmp += *(wp++);
			while ( nLen > 0 ) {
				tmp += *wp;
				nLen--;
				if ( *(wp++) == '"' )
					break;
			}
		} else if ( *wp == '\'' ) {
			tmp += *(wp++);
			while ( nLen > 0 ) {
				tmp += *wp;
				nLen--;
				if ( *(wp++) == '\'' )
					break;
			}
		} else if ( *wp == ' ' ) {
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
// CFifoBuffer

CFifoBuffer::CFifoBuffer()
{
	m_pEvent = new CEvent(FALSE, TRUE);
}
CFifoBuffer::~CFifoBuffer()
{
	delete m_pEvent;
}
int CFifoBuffer::GetData(LPBYTE lpBuf, int nBufLen, int sec)
{
	int n;
	int len = 0;
	time_t st, nt;

	time(&st);
	m_Sema.Lock();
	for ( ; ; ) {
		if ( (n = m_Data.GetSize()) > nBufLen )
			n = nBufLen;
		if ( n > 0 ) {
			memcpy(lpBuf, m_Data.GetPtr(), n);
			m_Data.Consume(n);
			len += n;
			lpBuf += n;
			if ( (nBufLen -= n) <= 0 )
				break;
		}
		time(&nt);
		if ( (nt - st) > sec )
			break;
		m_pEvent->ResetEvent();
		m_Sema.Unlock();
		WaitForSingleObject(m_pEvent->m_hObject, sec * 1000);
		m_Sema.Lock();
	}
	m_Sema.Unlock();
	return len;
}
void CFifoBuffer::SetData(LPBYTE lpBuf, int nBufLen)
{
	m_Sema.Lock();
	m_Data.Apend(lpBuf, nBufLen);
	if ( m_Data.GetSize() > (32 * 1024) )
		m_Data.Consume(m_Data.GetSize() - (32 * 1024));
	m_pEvent->SetEvent();
	m_Sema.Unlock();
}
WCHAR CFifoBuffer::GetWChar(int sec)
{
	WCHAR ch;
	if ( GetData((LPBYTE)&ch, sizeof(WCHAR), sec) < sizeof(WCHAR) )
		return EOF;
	return ch;
}
void CFifoBuffer::SetWChar(WCHAR ch)
{
	SetData((LPBYTE)&ch, sizeof(WCHAR));
}
int CFifoBuffer::WReadLine(CStringW &str, int sec)
{
	WCHAR ch;
	str.Empty();
	while ( (ch = GetWChar(sec)) != EOF ) {
		str += ch;
		if ( ch == '\n' )
			return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// CStringIndex

CStringIndex::CStringIndex()
{
	m_bNoCase = TRUE;
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
	m_Value   = data.m_Value;
	m_nIndex  = data.m_nIndex;
	m_String  = data.m_String;

	m_Array.RemoveAll();
	for ( int n = 0 ; n < data.m_Array.GetSize() ; n++ )
		m_Array.Add(data.m_Array[n]);

	return *this;
}
CStringIndex & CStringIndex::operator [] (LPCSTR str)
{
	int n, b, m, c;
	CStringIndex tmpData;

	b = 0;
	m = m_Array.GetSize() - 1;
	while ( b <= m ) {
		n = (b + m) / 2;
		c = (m_bNoCase ? m_Array[n].m_nIndex.CompareNoCase(str) : m_Array[n].m_nIndex.Compare(str));
		if ( c == 0 )
			return m_Array[n];
		else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}
	tmpData.SetNoCase(m_bNoCase);
	tmpData.m_nIndex = str;
	m_Array.InsertAt(b, tmpData);
	return m_Array[b];
}
int CStringIndex::Find(LPCSTR str)
{
	int n, b, m, c;

	b = 0;
	m = m_Array.GetSize() - 1;
	while ( b <= m ) {
		n = (b + m) / 2;
		c = (m_bNoCase ? m_Array[n].m_nIndex.CompareNoCase(str) : m_Array[n].m_nIndex.Compare(str));
		if ( c == 0 )
			return n;
		else if ( c < 0 )
			b = n + 1;
		else
			m = n - 1;
	}
	return (-1);
}
void CStringIndex::SetArray(LPCSTR str)
{
	int c, se;
	int st = 0;
	CString idx;
	CString val;

	m_Array.RemoveAll();
	while ( (c = *(str++)) != '\0' ) {
		switch(st) {
		case 0:
			if ( c == '=' ) {
				st = 1;
				idx.Trim(" \t\r\n");
				val.Empty();
			} else if ( c == ',' || c == ' ' || c == '\t' || c == '\r' || c == '\n' ) {
				idx.Trim(" \t\r\n");
				if ( !idx.IsEmpty() )
					(*this)[idx] = "";
				idx.Empty();
			} else
				idx += (char)c;
			break;
		case 1:
			if ( c == '"' || c == '\'' ) {
				se = c;
				st = 3;
				break;
			} else if ( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
				break;
			st = 2;
		case 2:
			if ( c == ',' ) {
				val.Trim(" \t\r\n");
				if ( !idx.IsEmpty() )
					(*this)[idx] = val;
				st = 0;
				idx.Empty();
			} else
				val += (char)c;
			break;
		case 3:
			if ( c == se ) {
				if ( !idx.IsEmpty() )
					(*this)[idx] = val;
				st = 4;
				idx.Empty();
			} else
				val += (char)c;
			break;
		case 4:
		    if ( c == ',' )
				st = 0;
		    break;
		}
	}

    switch(st) {
    case 1:
		if ( !idx.IsEmpty() )
		    (*this)[idx] = "";
		break;
    case 2:
		val.Trim(" \t\r\n");
    case 3:
		if ( !idx.IsEmpty() )
			(*this)[idx] = val;
		break;
    }
}

//////////////////////////////////////////////////////////////////////
// CStringEnv

CStringEnv::CStringEnv()
{
	m_Value = 0;
	m_nIndex.Empty();
	m_String.Empty();
	m_Array.RemoveAll();
}
CStringEnv::~CStringEnv()
{
}
const CStringEnv & CStringEnv::operator = (CStringEnv &data)
{
	m_Value   = data.m_Value;
	m_nIndex  = data.m_nIndex;
	m_String  = data.m_String;

	m_Array.RemoveAll();
	for ( int n = 0 ; n < data.m_Array.GetSize() ; n++ )
		m_Array.Add(data.m_Array[n]);

	return *this;
}
CStringEnv & CStringEnv::operator [] (LPCSTR str)
{
	int n;
	CStringEnv tmpData;

	for ( n = 0 ; n < GetSize() ; n++ ) {
		if ( m_Array[n].m_nIndex.CompareNoCase(str) == 0 )
			return m_Array[n];
	}

	tmpData.m_nIndex = str;
	n = m_Array.Add(tmpData);
	return m_Array[n];
}
int CStringEnv::Find(LPCSTR str)
{
	int n;

	for ( n = 0 ; n < GetSize() ; n++ ) {
		if ( m_Array[n].m_nIndex.CompareNoCase(str) == 0 )
			return n;
	}
	return (-1);
}
void CStringEnv::GetBuffer(CBuffer *bp)
{
	m_Value = bp->Get32Bit();
	bp->GetStr(m_nIndex);
	bp->GetStr(m_String);

	SetSize(bp->Get32Bit());

	for ( int n = 0 ; n < GetSize() ; n++ )
		m_Array[n].GetBuffer(bp);
}
void CStringEnv::SetBuffer(CBuffer *bp)
{
	bp->Put32Bit(m_Value);
	bp->PutStr(m_nIndex);
	bp->PutStr(m_String);

	bp->Put32Bit(GetSize());

	for ( int n = 0 ; n < GetSize() ; n++ )
		m_Array[n].SetBuffer(bp);
}
void CStringEnv::GetString(LPCSTR str)
{
	CBuffer tmp;
	if ( *str == '\0' )
		return;
	tmp.Base64Decode(str);
	if ( tmp.GetSize() < 16 )
		return;
	GetBuffer(&tmp);
}
void CStringEnv::SetString(CString &str)
{
	CBuffer tmp, work;
	SetBuffer(&tmp);
	work.Base64Encode(tmp.GetPtr(), tmp.GetSize());
	str = (LPCSTR)work;
}

//////////////////////////////////////////////////////////////////////
// CStringBinary

CStringBinary::CStringBinary()
{
	m_pLeft = m_pRight = NULL;
	m_Index = "a";
	m_Value = (-1);
}
CStringBinary::CStringBinary(LPCSTR str)
{
	m_pLeft = m_pRight = NULL;
	m_Index = str;
	m_Value = (-1);
}
CStringBinary::~CStringBinary()
{
	if ( m_pLeft != NULL )
		delete m_pLeft;

	if ( m_pRight != NULL )
		delete m_pRight;
}
CStringBinary & CStringBinary::operator [] (LPCSTR str)
{
	int c = m_Index.Compare(str);

	if ( c == 0 )
		return *this;
	else if ( c < 0 ) {
		if ( m_pLeft == NULL ) {
			m_pLeft = new CStringBinary(str);
			return *m_pLeft;
		} else
			return (*m_pLeft)[str];
	} else {
		if ( m_pRight == NULL ) {
			m_pRight = new CStringBinary(str);
			return *m_pRight;
		} else
			return (*m_pRight)[str];
	}
}
void CStringBinary::RemoveAll()
{
	if ( m_pLeft != NULL )
		delete m_pLeft;

	if ( m_pRight != NULL )
		delete m_pRight;

	m_pLeft = m_pRight = NULL;
	m_Index = "a";
	m_Value = (-1);
}
CStringBinary * CStringBinary::FindValue(int value)
{
	CStringBinary *bp;
	if ( m_Value == value )
		return this;
	if ( m_pLeft != NULL && (bp = m_pLeft->FindValue(value)) != NULL )
		return bp;
	else if ( m_pRight != NULL && (bp = m_pRight->FindValue(value)) != NULL )
		return bp;
	return NULL;
}
