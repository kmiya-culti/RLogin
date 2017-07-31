// TextRam.cpp: CTextRam クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdarg.h>
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "PipeSock.h"
#include "GrapWnd.h"
#include "SCript.h"
#include "CancelDlg.h"
#include "OptDlg.h"
#include "DefParamDlg.h"
#include "StatusDlg.h"
#include "TraceDlg.h"

#include <iconv.h>
#include <imm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifdef	USE_FIXWCHAR

//////////////////////////////////////////////////////////////////////
// CMemMap

CMemMap::CMemMap()
{
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);
	m_MapSize = (ULONGLONG)((MEMMAPSIZE + SystemInfo.dwAllocationGranularity - 1) / SystemInfo.dwAllocationGranularity * SystemInfo.dwAllocationGranularity);

	m_pMapTop = NULL;

	for ( int n = 0 ; n < MEMMAPCACHE ; n++ ) {
		m_MapData[n].hFile = NULL;
		m_MapData[n].pNext = m_pMapTop;
		m_pMapTop = &(m_MapData[n]);
	}
}
CMemMap::~CMemMap()
{
}

HANDLE CMemMap::Create(ULONGLONG size)
{
	static int SeqNum = 0;
	CString mapname;
	HANDLE hFile;

	mapname.Format(_T("RLoginTextRam_%08x_%d"), (unsigned)GetCurrentThreadId(), SeqNum++);
	size = ((size + m_MapSize - 1) / m_MapSize) * m_MapSize;

	hFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(size >> 32), (DWORD)size, mapname);

	if ( hFile == NULL || hFile == INVALID_HANDLE_VALUE )
		AfxThrowMemoryException();

	return hFile;
}
void CMemMap::Close(HANDLE hFile)
{
	MEMMAPNODE *mp;

	for ( mp = m_pMapTop ;  mp != NULL ; mp = mp->pNext ) {
		if ( mp->hFile == hFile ) {
			UnmapViewOfFile(mp->pAddress);
			mp->hFile = NULL;
		}
	}

    CloseHandle(hFile);
}
void *CMemMap::MemMap(HANDLE hFile, ULONGLONG pos)
{
	int diff;
	ULONGLONG offset;
	MEMMAPNODE *pNext, *pBack;

	pNext = pBack = m_pMapTop;
	offset = (pos / m_MapSize) * m_MapSize;
	diff = (int)(pos - offset);

	for ( ; ; ) {
		if ( pNext->hFile == hFile && pNext->Offset == offset )
			goto ENDOF;
		if ( pNext->pNext == NULL )
			break;
		pBack = pNext;
		pNext = pNext->pNext;
	}

	if ( pNext->hFile != NULL )
		UnmapViewOfFile(pNext->pAddress);

	pNext->hFile = hFile;
	pNext->Offset = offset;
	pNext->pAddress = MapViewOfFile(hFile, FILE_MAP_WRITE, (DWORD)(offset >> 32), (DWORD)offset, m_MapSize);

	if ( pNext->pAddress == NULL )
		AfxThrowMemoryException();

	//TRACE("MemMap %d page\n", (int)(offset / m_MapSize));

ENDOF:
	if ( pNext != pBack ) {
		pBack->pNext = pNext->pNext;
		pNext->pNext = m_pMapTop;
		m_pMapTop = pNext;
	}
	return (void *)((BYTE *)(pNext->pAddress) + diff);
}

#else	// !USE_FIXWCHAR

//////////////////////////////////////////////////////////////////////
// WordAlloc
							//	 7     15    31
static void		*pMemFree[3] = { NULL, NULL, NULL };
static void		*pMemTop = NULL;

#define	ALLOCMAX	(128 * 1024)

WCHAR *WCharAlloc(int len)
{
	int n, a;
	int hs;
	BYTE *bp;
	WCHAR *wp;

	switch(len) {
	case 0: case 1:	case 2:	case 3:
	case 4: case 5:	case 6:	case 7:
		len = 7;
		hs  = 0;
		break;
	case  8: case  9: case 10: case 11:
	case 12: case 13: case 14: case 15:
		len = 15;
		hs  = 1;
		break;
	default:
		len = 31;
		hs  = 2;
		break;
	}

	if ( pMemFree[hs] == NULL ) {
		bp = new BYTE[ALLOCMAX];
		*((void **)bp) = pMemTop;
		pMemTop = (void *)bp;
		n = sizeof(void *);
		a = (1 + len) * sizeof(WCHAR);
		ASSERT(sizeof(void *) <= a);
		while ( (n + a) <= ALLOCMAX ) {
			wp = (WCHAR *)(bp + n);
			*((void **)wp) = pMemFree[hs];
			pMemFree[hs] = (void *)wp;
			n += a;
		}
	}

	wp = (WCHAR *)pMemFree[hs];
	pMemFree[hs] = *((void **)wp);
	*(wp++) = hs;
	return wp;
}
void WCharFree(WCHAR *ptr)
{
	int hs = *(--ptr);
	ASSERT(hs >= 0 && hs < 3);
	*((void **)ptr) = pMemFree[hs];
	pMemFree[hs] = (void *)ptr;
}
inline int WCharSize(WCHAR *ptr)
{
	static const int sizeTab[] = { 7, 15, 31 };
	return sizeTab[*(ptr - 1)];
}
void AllWCharAllocFree()
{
	void *ptr;

	while ( (ptr = pMemTop) != NULL ) {
		pMemTop = *((void **)ptr);
		delete ptr;
	}
}

#endif	// USE_FIXWCHAR

//////////////////////////////////////////////////////////////////////
// CCharCell

#ifndef	USE_FIXWCHAR
CCharCell::CCharCell()
{
	m_Data = m_Vram.pack.wcbuf;
//	ZeroMemory(&m_Vram, sizeof(VRAM));
}
CCharCell::~CCharCell()
{
	if ( m_Data != m_Vram.pack.wcbuf )
		WCharFree(m_Data);
}
void CCharCell::GetCCharCell(CCharCell &data)
{
	if ( data.m_Data == data.m_Vram.pack.wcbuf ) {
		if ( m_Data != m_Vram.pack.wcbuf ) {
			WCharFree(m_Data);
			m_Data = m_Vram.pack.wcbuf;
		}
	} else {
		if ( m_Data != m_Vram.pack.wcbuf )
			WCharFree(m_Data);
		m_Data = WCharAlloc(WCharSize(data.m_Data));
		memcpy(m_Data, data.m_Data, sizeof(WCHAR) * WCharSize(m_Data));
	}
	m_Vram = data.m_Vram;
}
#endif
void CCharCell::operator = (DWORD ch)
{
	if ( IS_IMAGE(m_Vram.mode) )
		return;

#ifndef	USE_FIXWCHAR
	if ( (ch & 0xFFFF0000) != 0 ) {
		if ( m_Data == m_Vram.pack.wcbuf )
			m_Data = WCharAlloc(3);
		//else if ( WCharSize(m_Data) < 3 ) {	not use min 7 word
		//	WCharFree(m_Data);
		//	m_Data = WCharAlloc(3);
		//}
		m_Data[0] = (WCHAR)(ch >> 16);
		m_Data[1] = (WCHAR)(ch);
		m_Data[2] = L'\0';
	} else {
		if ( m_Data != m_Vram.pack.wcbuf ) {
			WCharFree(m_Data);
			m_Data = m_Vram.pack.wcbuf;
		}
		m_Data[0] = (WCHAR)(ch);
		m_Data[1] = L'\0';
	}
#else
	if ( (ch & 0xFFFF0000) != 0 ) {
		m_Data[0] = (WCHAR)(ch >> 16);
		m_Data[1] = (WCHAR)(ch);
		m_Data[2] = L'\0';
	} else {
		m_Data[0] = (WCHAR)(ch);
		m_Data[1] = L'\0';
	}
#endif
}
void CCharCell::operator += (DWORD ch)
{
	int n, a, b;

	if ( ch == 0 || IS_IMAGE(m_Vram.mode) )
		return;

	for ( n = 0 ; m_Data[n] != 0 ; )
		n++;

	b = ((ch & 0xFFFF0000) != 0 ? 2 : 1);

	if ( (a = n + b + 1) > MAXCHARSIZE )
		return;

#ifndef	USE_FIXWCHAR
	if ( a > (m_Data == m_Vram.pack.wcbuf ? 2 : WCharSize(m_Data)) ) {
		WCHAR *nw = WCharAlloc(a);
		for ( n = 0 ; m_Data[n] != 0 ; n++ )
			nw[n] = m_Data[n];
		if ( m_Data != m_Vram.pack.wcbuf )
			WCharFree(m_Data);
		m_Data = nw;
	}
#endif

	if ( (ch & 0xFFFF0000) != 0 )
		m_Data[n++] = (WCHAR)(ch >> 16);

	if ( (ch & 0x0000FFFF) != 0 )
		m_Data[n++] = (WCHAR)ch;

	m_Data[n] = L'\0';
}
void CCharCell::operator = (LPCWSTR str)
{
	int n;
	int a = wcslen(str) + 1;

	if ( IS_IMAGE(m_Vram.mode) )
		return;

#ifndef	USE_FIXWCHAR
	if ( a <= 2 ) {
		if ( m_Data != m_Vram.pack.wcbuf ) {
			WCharFree(m_Data);
			m_Data = m_Vram.pack.wcbuf;
		}
	} else if ( m_Data == m_Vram.pack.wcbuf ) {
		m_Data = WCharAlloc(a);
	} else if ( a > WCharSize(m_Data) ) {
		WCharFree(m_Data);
		m_Data = WCharAlloc(a);
	}
#endif

	for ( n = 0 ; n < a ; n++ ) {
		if ( n >= (MAXCHARSIZE - 1) ) {
			TRACE("CCharCell over %d\n", a);
			m_Data[MAXCHARSIZE - 1] = L'\0';
			break;
		}
		m_Data[n] = *(str++);
	}
}
void CCharCell::SetVRAM(VRAM &ram)
{
	ram = m_Vram;

	if ( !IS_IMAGE(m_Vram.mode) ) {
		if ( m_Data[0] != 0 && m_Data[1] != 0 )
			ram.pack.dchar = (m_Data[0] << 16) | m_Data[1];
		else
			ram.pack.dchar = m_Data[0];
	}
}

void CCharCell::SetBuffer(CBuffer &buf)
{
	buf.Apend((LPBYTE)&m_Vram, sizeof(m_Vram));

	for ( LPCWSTR p = *this ; ; p++ ) {
		buf.PutWord(*p);
		if ( *p == L'\0' )
			break;
	}
}
void CCharCell::GetBuffer(CBuffer &buf)
{
	int c;

	if ( buf.GetSize() < sizeof(m_Vram) )
		return;

	memcpy(&m_Vram, buf.GetPtr(), sizeof(m_Vram));
	buf.Consume(sizeof(m_Vram));

	Empty();
	while ( (c = buf.GetWord()) != 0 )
		*this += (DWORD)c;
}

void CCharCell::SetString(CString &str)
{
	CBuffer tmp, work;

	SetBuffer(tmp);
	work.Base64Encode(tmp.GetPtr(), tmp.GetSize());
	str = (LPCTSTR)work;
}
void CCharCell::GetString(LPCTSTR str)
{
	CBuffer tmp;

	tmp.Base64Decode(str);
	if ( tmp.GetSize() < (sizeof(m_Vram) + 2) )
		return;
	GetBuffer(tmp);
}

void CCharCell::Read(CFile &file, int ver)
{
	WCHAR c;
	VRAM ram;
	BYTE tmp[8];

	switch(ver) {
	case 1:
		if ( file.Read(tmp, 8) != 8 )
			AfxThrowFileException(CFileException::endOfFile);

		ram.pack.dchar = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
		ram.bank = tmp[4] | (tmp[5] << 8);
		ram.eram = tmp[5] >> 2;
		ram.zoom = tmp[5] >> 4;
		ram.mode = tmp[5] >> 6;
		ram.attr = tmp[6];
		ram.fcol = tmp[7] & 0x0F;
		ram.bcol = tmp[7] >> 4;

		*this = ram;
		break;

	case 2:
		if ( file.Read(&ram, sizeof(ram)) != sizeof(ram) )
			AfxThrowFileException(CFileException::endOfFile);
		*this = ram;
		break;

	case 3:
		if ( file.Read(&m_Vram, sizeof(m_Vram)) != sizeof(m_Vram) )
			AfxThrowFileException(CFileException::endOfFile);
		Empty();
		do {
			if ( file.Read(&c, sizeof(WCHAR)) != sizeof(WCHAR) )
				AfxThrowFileException(CFileException::endOfFile);
			*this += (DWORD)c;
		} while ( c != L'\0' );
		break;
	}

	if ( IS_IMAGE(m_Vram.mode) ) {
		m_Vram.mode = CM_ASCII;
		Empty();
	}
}
void CCharCell::Write(CFile &file)
{
	CBuffer tmp;

	SetBuffer(tmp);
	file.Write(tmp.GetPtr(), tmp.GetSize());
}
void CCharCell::Copy(CCharCell *dis, CCharCell *src, int size)
{
#ifdef	USE_FIXWCHAR
	memcpy(dis, src, sizeof(CCharCell) * size);
#elif 0
	CCharCell *end;

	for ( end = dis + size ; dis < end ; )
		*(dis++) = *(src++);

#else
	register CCharCell *ptr;
	CCharCell *end;

	for ( ptr = dis, end = ptr + size ; ptr < end ; ptr++ ) {
		if ( ptr->m_Data != ptr->m_Vram.pack.wcbuf )
			WCharFree(ptr->m_Data);
	}

	memcpy(dis, src, sizeof(CCharCell) * size);

	for ( ptr = dis, end = ptr + size ; ptr < end ; ptr++, src++ ) {
		if ( src->m_Data == src->m_Vram.pack.wcbuf )
			ptr->m_Data = ptr->m_Vram.pack.wcbuf;
		else {
			ptr->m_Data = WCharAlloc(WCharSize(src->m_Data));
			memcpy(ptr->m_Data, src->m_Data, sizeof(WCHAR) * WCharSize(src->m_Data));
		}
	}
#endif
}
void CCharCell::Fill(CCharCell *dis, VRAM &vram, int size)
{
	register CCharCell *ptr;
	CCharCell *end;

	for ( ptr = dis, end = ptr + size ; ptr < end ; ptr++ ) {
#ifndef	USE_FIXWCHAR
		if ( ptr->m_Data != ptr->m_Vram.pack.wcbuf ) {
			WCharFree(ptr->m_Data);
			ptr->m_Data = ptr->m_Vram.pack.wcbuf;
		}
#endif
		ptr->m_Vram = vram;
		ptr->m_Data[0] = L'\0';
	}
}

//////////////////////////////////////////////////////////////////////
// CFontNode

CFontNode::CFontNode()
{
	Init();
}
CFontNode::~CFontNode()
{
}
void CFontNode::Init()
{
	m_Shift       = 0;
	m_ZoomW       = 100;
	m_ZoomH       = 100;
	m_OffsetW     = 0;
	m_OffsetH     = 0;
	m_CharSet     = 0;
	m_EntryName   = _T("");
	m_IContName   = _T("");
	m_IndexName   = _T("");
	m_Quality     = DEFAULT_QUALITY;
	m_Init        = FALSE;
	for ( int n = 0 ; n < 16 ; n++ ) {
		m_FontName[n] = _T("");
		m_Hash[n]     = 0;
	}
	m_MapType     = 0;
}
void CFontNode::SetArray(CStringArrayExt &stra)
{
	stra.RemoveAll();

	stra.AddVal(m_Shift);
	stra.AddVal(m_ZoomH);
	stra.AddVal(m_OffsetH);
	stra.AddVal(m_CharSet);
	stra.Add(m_FontName[0]);
	stra.Add(m_EntryName);
	stra.Add(m_IContName);
	stra.AddVal(m_ZoomW);
	for ( int n = 1 ; n < 16 ; n++ )
		stra.Add(m_FontName[n]);
	stra.AddVal(m_Quality);
	stra.Add(m_IndexName);
	stra.AddVal(m_OffsetW);
}
void CFontNode::GetArray(CStringArrayExt &stra)
{
	Init();

	if ( stra.GetSize() < 7 )
		return;

	m_Shift		  = stra.GetVal(0);
	m_ZoomH		  = stra.GetVal(1);
	m_OffsetH	  = stra.GetVal(2);
	m_CharSet	  = stra.GetVal(3);
	m_FontName[0] = stra.GetAt(4);
	SetHash(0);

	m_EntryName   = stra.GetAt(5);
	m_IContName   = stra.GetAt(6);
	m_ZoomW		  = (stra.GetSize() > 7 ? stra.GetVal(7) : m_ZoomH);

	if ( m_IContName.Compare(_T("GB2312-80")) == 0 )	// Debug!!
		m_IContName = _T("GB_2312-80");

	for ( int n = 1 ; n < 16 ; n++ ) {				// 8 - 23
		if ( stra.GetSize() > (7 + n) ) {
			m_FontName[n] = stra.GetAt(7 + n);
			SetHash(n);
		}
	}

	if ( stra.GetSize() > (7 + 16) )
		m_Quality = stra.GetVal(7 + 16);

	if ( stra.GetSize() > (8 + 16) )
		m_IndexName = stra.GetAt(8 + 16);

	if ( stra.GetSize() > (9 + 16) )
		m_OffsetW = stra.GetVal(9 + 16);

	m_Init = TRUE;
}
void CFontNode::SetHash(int num)
{
	m_Hash[num] = 0;
	LPCTSTR p = m_FontName[num];
	while ( *p != _T('\0') )
		m_Hash[num] = m_Hash[num] * 31 + *(p++);
}
CFontChacheNode *CFontNode::GetFont(int Width, int Height, int Style, int FontNum, LPCTSTR DefFontName)
{
	if ( m_FontName[FontNum].IsEmpty() )
		FontNum = 0;
	
	return ((CRLoginApp *)AfxGetApp())->m_FontData.GetFont((m_FontName[FontNum].IsEmpty() ? DefFontName : m_FontName[FontNum]),
					Width * m_ZoomW / 100, Height * m_ZoomH / 100, m_CharSet, Style, m_Quality, m_Hash[FontNum]);
}
const CFontNode & CFontNode::operator = (CFontNode &data)
{
	m_Shift     = data.m_Shift;
	m_ZoomW     = data.m_ZoomW;
	m_ZoomH     = data.m_ZoomH;
	m_OffsetW   = data.m_OffsetW;
	m_OffsetH   = data.m_OffsetH;
	m_CharSet   = data.m_CharSet;
	m_EntryName = data.m_EntryName;
	m_IContName = data.m_IContName;
	m_IndexName = data.m_IndexName;
	m_Quality   = data.m_Quality;
	m_Init      = data.m_Init;
	for ( int n = 0 ; n < 16 ; n++ ) {
		m_FontName[n] = data.m_FontName[n];
		m_Hash[n]     = data.m_Hash[n];
	}
	return *this;
}
void CFontNode::SetUserBitmap(int code, int width, int height, CBitmap *pMap, int ofx, int ofy)
{
	CDC oDc, nDc;
	CBitmap *pOld[2];
	BITMAP info;

	if ( (code -= 0x20) < 0 )
		code = 0;
	else if ( code > 95 )
		code = 95;

	if ( !pMap->GetBitmap(&info) )
		return;

	if ( !oDc.CreateCompatibleDC(NULL) || !nDc.CreateCompatibleDC(NULL) )
		return;

	if ( m_MapType != FNT_BITMAP_COLOR && m_UserFontMap.GetSafeHandle() != NULL )
		m_UserFontMap.DeleteObject();

	if ( m_UserFontMap.GetSafeHandle() == NULL ) {
		if ( !m_UserFontMap.CreateBitmap(USFTWMAX * 96, USFTHMAX, info.bmPlanes, info.bmBitsPixel, NULL) ) {
			oDc.DeleteDC();
			nDc.DeleteDC();
			return;
		}
		m_MapType = FNT_BITMAP_COLOR;
		pOld[1] = nDc.SelectObject(&m_UserFontMap);
		nDc.FillSolidRect(0, 0, USFTWMAX * 96, USFTHMAX, RGB(0, 0, 0));
		memset(m_UserFontDef, 0, 96 / 8);
	} else
		pOld[1] = nDc.SelectObject(&m_UserFontMap);

	pOld[0] = oDc.SelectObject(pMap);

//	nDc.BitBlt(USFTWMAX * code, 0, width, height, &oDc, ofx, ofy, SRCCOPY);
	nDc.BitBlt(USFTWMAX * code, 0, USFTWMAX, USFTHMAX, &oDc, ofx, ofy, SRCCOPY);

	oDc.SelectObject(pOld[0]);
	nDc.SelectObject(pOld[1]);

	oDc.DeleteDC();
	nDc.DeleteDC();

	m_UserFontWidth  = width;
	m_UserFontHeight = height;
	m_UserFontDef[code / 8] |= (1 << (code % 8));

	if (  m_FontMap.GetSafeHandle() != NULL )
		m_FontMap.DeleteObject();
}
void CFontNode::SetUserFont(int code, int width, int height, LPBYTE map)
{
	CDC dc;
	int n, x, y;
	CBitmap *pOld;

	if ( (code -= 0x20) < 0 )
		code = 0;
	else if ( code > 95 )
		code = 95;

	if ( !dc.CreateCompatibleDC(NULL) )
		return;

	if ( m_MapType != FNT_BITMAP_MONO && m_UserFontMap.GetSafeHandle() != NULL )
		m_UserFontMap.DeleteObject();

	if ( m_UserFontMap.GetSafeHandle() == NULL ) {
		if ( !m_UserFontMap.CreateBitmap(USFTWMAX * 96, USFTHMAX, 1, 1, NULL) ) {
			dc.DeleteDC();
			return;
		}
		m_MapType = FNT_BITMAP_MONO;
		pOld = dc.SelectObject(&m_UserFontMap);
		dc.FillSolidRect(0, 0, USFTWMAX * 96, USFTHMAX, RGB(255, 255, 255));
		memset(m_UserFontDef, 0, 96 / 8);
		m_UserFontDef[0] |= 0x01;	// ' '
	} else
		pOld = dc.SelectObject(&m_UserFontMap);

	for ( y = 0 ; y < USFTHMAX && y < height ; y++ ) {
		for ( x = 0 ; x < USFTWMAX && x < width ; x++ ) {
			if ( (map[x + USFTWMAX * (y / 6)] & (1 << (y % 6))) != 0 )
				dc.SetPixelV(code * USFTWMAX + x, y, RGB(0, 0, 0));
			else
				dc.SetPixelV(code * USFTWMAX + x, y, RGB(255, 255, 255));
		}
	}

	dc.SelectObject(pOld);
	dc.DeleteDC();

	m_UserFontWidth  = width;
	m_UserFontHeight = height;
	m_UserFontDef[code / 8] |= (1 << (code % 8));

	if (  m_FontMap.GetSafeHandle() != NULL )
		m_FontMap.DeleteObject();
}
BOOL CFontNode::SetFontImage(int width, int height)
{
	CDC oDc, nDc;
	CBitmap *pOld[2];
	BITMAP info;

	if ( m_UserFontMap.GetSafeHandle() == NULL )
		return FALSE;

	if ( !m_UserFontMap.GetBitmap(&info) )
		return FALSE;

	width  = width  * USFTWMAX / m_UserFontWidth;
	height = height * USFTHMAX / m_UserFontHeight;

	if ( m_FontMap.GetSafeHandle() == NULL || m_FontWidth != width || m_FontHeight != height ) {
		if ( m_FontMap.GetSafeHandle() != NULL )
			m_FontMap.DeleteObject();

		if ( !oDc.CreateCompatibleDC(NULL) || !nDc.CreateCompatibleDC(NULL) )
			return FALSE;

		if ( !m_FontMap.CreateBitmap(width * 96, height, info.bmPlanes, info.bmBitsPixel, NULL) )
			return FALSE;

		pOld[0] = oDc.SelectObject(&m_UserFontMap);
		pOld[1] = nDc.SelectObject(&m_FontMap);

		if ( width > m_UserFontWidth || info.bmBitsPixel > 1 )
			nDc.SetStretchBltMode(HALFTONE);

		nDc.StretchBlt(0, 0, width * 96, height, &oDc, 0, 0, USFTWMAX * 96, USFTHMAX, SRCCOPY);

		oDc.SelectObject(pOld[0]);
		nDc.SelectObject(pOld[1]);

		oDc.DeleteDC();
		nDc.DeleteDC();

		m_FontWidth = width;
		m_FontHeight = height;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CFontTab

CFontTab::CFontTab()
{
	m_pSection = _T("FontTab");
	Init();
}
void CFontTab::Init()
{
	int n, i;
	static const struct _FontInitTab	{
		LPCTSTR	name;
		WORD	mode;
		LPCTSTR	scs;
		char	bank;
		LPCTSTR	iset;
		WORD	cset;
		WORD	zoomw;
		WORD	zoomh;
		WORD	offsetw;
		WORD	offseth;
		LPCTSTR	font[2];
	} FontInitTab[] = {
		{ _T("VT100-GRAPHIC"),			SET_94,		_T("0"),	'\x00', _T(""),					SYMBOL_CHARSET,		100,	100,	0,	0,	{ _T("Tera Special"), _T("") } },
		{ _T("IBM437-GR"),				SET_94,		_T("1"),	'\x80', _T("IBM437"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },

		{ _T("ISO646-US"),				SET_94,		_T("@"),	'\x00', _T("ISO646-US"),		ANSI_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("ASCII(ANSI X3.4-1968)"),	SET_94,		_T("B"),	'\x00', _T("ANSI_X3.4-1968"),	ANSI_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0201-Kana"),		SET_94,		_T("I"),	'\x80', _T("JISX0201-1976"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0201-Roman"),		SET_94,		_T("J"),	'\x00', _T("JISX0201-1976"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("GB 1988-80"),				SET_94,		_T("T"),	'\x00', _T("GB_1988-80"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },

		{ _T("Russian"),				SET_94,		_T("&5"),	'\x00',	_T("CP866"),			RUSSIAN_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("Turkish"),				SET_94,		_T("%2"),	'\x00',	_T("CP1254"),			TURKISH_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },

		/*
		{ _T("ChineseBig5"),			SET_94x94,	_T("?"),	'\x00',	_T("CP950"),			CHINESEBIG5_CHARSET,100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("Johab"),					SET_94x94,	_T("?"),	'\x00',	_T("CP1361"),			JOHAB_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		*/

		{ _T("LATIN1 (ISO8859-1)"),		SET_96,		_T("A"),	'\x80', _T("LATIN1"),			EASTEUROPE_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("LATIN2 (ISO8859-2)"),		SET_96,		_T("B"),	'\x80', _T("LATIN2"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("LATIN3 (ISO8859-3)"),		SET_96,		_T("C"),	'\x80', _T("LATIN3"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("LATIN4 (ISO8859-4)"),		SET_96,		_T("D"),	'\x80', _T("LATIN4"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("CYRILLIC (ISO8859-5)"),	SET_96,		_T("L"),	'\x80', _T("ISO8859-5"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("ARABIC (ISO8859-6)"),		SET_96,		_T("G"),	'\x80', _T("ISO8859-6"),		ARABIC_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("GREEK (ISO8859-7)"),		SET_96,		_T("F"),	'\x80', _T("ISO8859-7"),		GREEK_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("HEBREW (ISO8859-8)"),		SET_96,		_T("H"),	'\x80', _T("ISO8859-8"),		HEBREW_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("LATIN5 (ISO8859-9)"),		SET_96,		_T("M"),	'\x80', _T("LATIN5"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("LATIN6 (ISO8859-10)"),	SET_96,		_T("V"),	'\x80', _T("LATIN6"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("THAI (ISO8859-11)"),		SET_96,		_T("T"),	'\x80', _T("ISO-8859-11"),		THAI_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("VIETNAMESE (ISO8859-12)"),SET_96,		_T("Z"),	'\x80', _T("CP1258"),			VIETNAMESE_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("BALTIC (ISO8859-13)"),	SET_96,		_T("Y"),	'\x80', _T("ISO-8859-13"),		BALTIC_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("CELTIC (ISO8859-14)"),	SET_96,		_T("_"),	'\x80', _T("ISO-8859-14"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("LATIN9 (ISO8859-15)"),	SET_96,		_T("b"),	'\x80', _T("ISO-8859-15"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("ROMANIAN (ISO8859-16)"),	SET_96,		_T("f"),	'\x80', _T("ISO-8859-16"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },

		{ _T("JIS X 0208-1978"),		SET_94x94,	_T("@"),	'\x00', _T("JIS_X0208-1983"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("GB2312-1980"),			SET_94x94,	_T("A"),	'\x00', _T("GB_2312-80"),		GB2312_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0208-1983"),		SET_94x94,	_T("B"),	'\x00', _T("JIS_X0208-1983"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("KSC5601-1987"),			SET_94x94,	_T("C"),	'\x00', _T("KS_C_5601-1987"),	HANGEUL_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0212-1990"),		SET_94x94,	_T("D"),	'\x00', _T("JIS_X0212-1990"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("ISO-IR-165"),				SET_94x94,	_T("E"),	'\x00', _T("ISO-IR-165"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0213-2000.1"),		SET_94x94,	_T("O"),	'\x00', _T("JIS_X0213-2000.1"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0213-2000.2"),		SET_94x94,	_T("P"),	'\x00', _T("JIS_X0213-2000.2"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0213-2004.1"),		SET_94x94,	_T("Q"),	'\x00', _T("JIS_X0213-2000.1"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },

		{ _T("UNICODE"),				SET_UNICODE,_T("*U"),	'\x00', _T("UTF-16BE"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },

		{ NULL, 0, 0x00, NULL },
	};

	for ( n = 0 ; n < CODE_MAX ; n++ )
		m_Data[n].Init();

	for ( n = 0 ; FontInitTab[n].name != NULL ; n++ ) {
		i = IndexFind(FontInitTab[n].mode, FontInitTab[n].scs);
		m_Data[i].m_Shift       = FontInitTab[n].bank;
		m_Data[i].m_ZoomW       = FontInitTab[n].zoomw;
		m_Data[i].m_ZoomH       = FontInitTab[n].zoomh;
		m_Data[i].m_OffsetW     = FontInitTab[n].offsetw;
		m_Data[i].m_OffsetH     = FontInitTab[n].offseth;
		m_Data[i].m_CharSet     = FontInitTab[n].cset;
		m_Data[i].m_FontName[0] = FontInitTab[n].font[0];
		m_Data[i].m_FontName[1] = FontInitTab[n].font[1];
		m_Data[i].m_EntryName   = FontInitTab[n].name;
		m_Data[i].m_IContName   = FontInitTab[n].iset;
		m_Data[i].m_IndexName   = FontInitTab[n].scs;
		m_Data[i].m_Init        = FALSE;
		m_Data[i].SetHash(0);
		m_Data[i].SetHash(1);
	}
}
void CFontTab::SetArray(CStringArrayExt &stra)
{
	int n;
	CString str;
	CStringArrayExt tmp;

	stra.RemoveAll();
	stra.Add(_T("-1"));
	for ( n = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_Data[n].m_EntryName.IsEmpty() )
			continue;
		m_Data[n].SetArray(tmp);
		str.Format(_T("%d"), n);
		tmp.InsertAt(0, str);
		tmp.SetString(str);
		stra.Add(str);
	}
}
void CFontTab::GetArray(CStringArrayExt &stra)
{
	int n, i;
	CStringArrayExt tmp;

	if ( stra.GetSize() == 0 || _tstoi(stra[0]) != (-1) )
		Init();
	else {
		for ( n = 0 ; n < CODE_MAX ; n++ )
			m_Data[n].Init();
	}

	for ( n = 0 ; n < stra.GetSize() ; n++ ) {
		tmp.GetString(stra[n]);
		if ( tmp.GetSize() < 1 )
			continue;
		if ( (i = tmp.GetVal(0)) < 0 || i >= CODE_MAX )
			continue;
		tmp.RemoveAt(0);
		m_Data[i].GetArray(tmp);
		if ( m_Data[i].m_IndexName.IsEmpty() ) {
			if ( i == SET_UNICODE )
				m_Data[i].m_IndexName = _T("*U");
			else
				m_Data[i].m_IndexName = (CHAR)(i & 0xFF);
		} else if ( m_Data[i].m_IndexName.Compare(_T("Unicode")) == 0 )
			m_Data[i].m_IndexName = _T("*U");
		if ( i == SET_UNICODE )
			m_Data[i].m_IContName = _T("UTF-16BE");
	}
}
void CFontTab::SetIndex(int mode, CStringIndex &index)
{
	int n, i, a, m;
	int code;
	CString str;
	CStringIndex *ip;
	static const LPCTSTR menbaName[] = { _T("Table"), _T("Add"), NULL };

	if ( mode ) {		// Write
		for ( n = 0 ; n < CODE_MAX ; n++ ) {
			if ( m_Data[n].m_EntryName.IsEmpty() )
				continue;

			str.Format(_T("%d"), n);
			ip = &(index[menbaName[0]][str]);

			ip->Add(m_Data[n].m_EntryName);
			ip->Add(n >> 8);
			ip->Add(m_Data[n].m_IndexName);
			ip->Add(m_Data[n].m_Shift);
			ip->Add(m_Data[n].m_ZoomW);
			ip->Add(m_Data[n].m_ZoomH);
			ip->Add(m_Data[n].m_OffsetH);
			ip->Add(m_Data[n].m_Quality);
			ip->Add(m_Data[n].m_CharSet);
			ip->Add(m_Data[n].m_IContName);

			a = ip->GetSize();
			ip->SetSize(a + 1);
			for ( i = 0 ; i < 16 ; i++ )
				(*ip)[a].Add(m_Data[n].m_FontName[i]);

			ip->Add(m_Data[n].m_OffsetW);
		}

	} else {			// Read
		for ( m = 0 ; menbaName[m] != NULL ; m++ ) {
			if ( (n = index.Find(menbaName[m])) < 0 )
				continue;

			if ( m == 0 ) {	// Table
				for ( i = 0 ; i < CODE_MAX ; i++ )
					m_Data[i].Init();
			}

			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				if ( index[n][i].GetSize() < 11 )
					continue;
				code = IndexFind(index[n][i][1] << 8, index[n][i][2]);

				m_Data[code].m_EntryName = index[n][i][0];
				m_Data[code].m_IndexName = index[n][i][2];
				m_Data[code].m_Shift     = index[n][i][3];
				m_Data[code].m_ZoomW     = index[n][i][4];
				m_Data[code].m_ZoomH     = index[n][i][5];
				m_Data[code].m_OffsetH   = index[n][i][6];
				m_Data[code].m_Quality   = index[n][i][7];
				m_Data[code].m_CharSet   = index[n][i][8];
				m_Data[code].m_IContName = index[n][i][9];

				for ( a = 0 ; a < 16 && a < index[n][i][10].GetSize() ; a++ )
					m_Data[code].m_FontName[a] = index[n][i][10][a];

				m_Data[code].m_OffsetW   = index[n][i][11];
			}
		}
	}
}
const CFontTab & CFontTab::operator = (CFontTab &data)
{
	for ( int n = 0 ; n < CODE_MAX ; n++ )
		m_Data[n] = data.m_Data[n];
	return *this;
}
int CFontTab::Find(LPCTSTR entry)
{
	for ( int n = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_Data[n].m_EntryName.Compare(entry) == 0 )
			return n;
	}
	return 0;
}
int CFontTab::IndexFind(int code, LPCTSTR name)
{
	int n, i;

	if ( name[1] == _T('\0') && name[0] >= _T('\x30') && name[0] <= _T('\x7E') ) {
		i = code | name[0];
		goto FOUND;
	} else if ( _tcscmp(name, _T("*U")) == 0 ) {
		i = SET_UNICODE;
		goto FOUND;
	}

	for ( n = i = 0 ;  n < (255 - 0x4F) ; n++ ) {
		i = code | (n + (n < 0x30 ? 0 : 0x4F));			// 0x00-0x2F or 0x7F-0xFF
		if ( m_Data[i].m_IndexName.IsEmpty() )
			break;
		else if ( m_Data[i].m_IndexName.Compare(name) == 0 )
			return i;
	}

FOUND:
	m_Data[i].m_IndexName = name;
	return i;
}
void CFontTab::IndexRemove(int idx)
{
	int n, i;
	int bank = idx & SET_MASK;
	int code = idx & 0xFF;

	m_Data[idx].Init();

	if ( code >= 0x30 && code <= 0x7E )
		return;

	while ( code < 255 ) {
		i = bank | code;
		if ( ++code == 0x30 )
			code = 0x7F;
		n = bank | code;
		if ( n == 255 || m_Data[n].m_IndexName.IsEmpty() ) {
			m_Data[i].Init();
			break;
		}
		m_Data[i] = m_Data[n];
	}
}

//////////////////////////////////////////////////////////////////////
// CTextSave

CTextSave::CTextSave()
{
	m_bAll = FALSE;
	m_pNext = NULL;
	m_pCharCell = NULL;
	m_pTextRam = NULL;
}
CTextSave::~CTextSave()
{
	int n;
	CGrapWnd *pWnd;

	if ( m_pCharCell != NULL )
		delete [] m_pCharCell;

	if ( m_pTextRam != NULL ) {
		for ( n = 0 ; n < m_GrapIdx.GetSize() ; n++ ) {
			if ( (pWnd = m_pTextRam->GetGrapWnd(m_GrapIdx[n])) != NULL )
				pWnd->m_Use--;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CTextRam

CTextRam::CTextRam()
{
	m_pDocument = NULL;
	m_bOpen = FALSE;
#ifdef	USE_FIXWCHAR
	m_hMap = NULL;
#else
	m_VRam = NULL;
#endif
	m_Cols = m_ColsMax = 80;
	m_LineUpdate = 0;
	m_Lines = 25;
	m_Page = 0;
	m_TopY = 0;
	m_BtmY = m_Lines;
	m_LeftX  = 0;
	m_RightX = m_Cols;
	m_HisMax = 100;
	m_HisPos = 0;
	m_HisLen = 0;
	m_HisUse = 0;
	m_pTextSave = m_pTextStack = NULL;
	m_DispCaret = FGCARET_ONOFF;
	m_TypeCaret = 0; 
	m_UpdateRect.SetRectEmpty();
	m_UpdateFlag = FALSE;
	m_DelayMSec = 0;
	m_HisFile.Empty();
	m_LogFile.Empty();
	m_KeepAliveSec = 0;
	m_RecvCrLf = m_SendCrLf = 0;
	m_LastChar = 0;
	m_LastFlag = 0;
	m_LastPos  = 0;
	m_DropFileMode = 0;
	m_WordStr.Empty();
	m_Exact = FALSE;
	m_MouseTrack = MOS_EVENT_NONE;
	m_MouseRect.SetRectEmpty();
	m_MouseOldSw    = (-1);
	m_MouseOldPos.x = (-1);
	m_MouseOldPos.y = (-1);
	m_Loc_Mode  = 0;
	memset(m_MetaKeys, 0, sizeof(m_MetaKeys));
	m_TitleMode = WTTL_ENTRY;
	m_StsFlag = FALSE;
	m_StsMode = 0;
	m_StsLed  = 0;
	m_VtLevel = 65;
	m_TermId  = 10;
	m_LangMenu = 0;
	SetRetChar(FALSE);
	m_ClipFlag = 0;
	m_FileSaveFlag = TRUE;
	m_ImageIndex = 1024;
	m_bOscActive = FALSE;
//	m_pCanDlg = NULL;
	m_bIntTimer = FALSE;
	m_bRtoL = FALSE;
	m_XtOptFlag = 0;
	m_XtMosPointMode = 0;
	m_TitleStack.RemoveAll();
	m_FrameCheck = FALSE;
	m_ScrnOffset.SetRect(0, 0, 0, 0);
	m_LogTimeFlag = TRUE;
	m_pCallPoint = &CTextRam::fc_FuncCall;
	m_pTraceWnd = NULL;
	m_pTraceTop = NULL;
	m_pTraceNow = NULL;
	m_pTraceProc = NULL;
	m_TraceSaveCount = 0;
	m_bTraceUpdate = FALSE;
	m_bTraceActive = FALSE;
	m_TraceMaxCount = 1000;
	m_pActGrapWnd = NULL;
	m_pWorkGrapWnd = NULL;
	m_GrapWndChkCLock = clock();
	m_GrapWndChkStat = 0;

	for ( int n = 0 ; n < 8 ; n++ )
		pGrapListIndex[n] = pGrapListImage[n] = NULL;
	pGrapListType = NULL;

	m_LineEditMode = FALSE;
	m_LineEditPos  = 0;
	m_LineEditBuff.Clear();
	m_LineEditMapsInit = 0;

	m_LogCharSet[0] = 1;
	m_LogCharSet[1] = 0;
	m_LogCharSet[2] = 3;
	m_LogCharSet[3] = 2;

	m_MouseMode[0] = 0;
	m_MouseMode[1] = 1;
	m_MouseMode[2] = 4;
	m_MouseMode[3] = 16;

	m_MousePos.x = m_MousePos.y = 0;

	m_Tek_Top = m_Tek_Free = NULL;
	m_pTekWnd = NULL;
	m_pImageWnd = NULL;
	m_bSixelColInit = FALSE;

	for ( int n = 0 ; n < MODKEY_MAX ; n++ )
		m_DefModKey[n] = (-1);
	memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
	memset(m_ModKeyTab, 0xFF, sizeof(m_ModKeyTab));

	fc_Init(EUC_SET);

	m_pSection = _T("TextRam");
	m_MinSize = 16;
	Init();
}
CTextRam::~CTextRam()
{
	CTextSave *pSave;

	if ( m_bIntTimer )
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);

#ifdef	USE_FIXWCHAR
	if ( m_hMap != NULL )
		m_MemMap.Close(m_hMap);
#else
	if ( m_VRam != NULL )
		delete [] m_VRam;
#endif

	while ( (pSave = m_pTextSave) != NULL ) {
		m_pTextSave = pSave->m_pNext;
		delete pSave;
	}
	while ( (pSave = m_pTextStack) != NULL ) {
		m_pTextStack = pSave->m_pNext;
		delete pSave;
	}

	TEKNODE *tp;
	while ( (tp = m_Tek_Top) != NULL ) {
		m_Tek_Top = tp->next;
		delete tp;
	}
	while ( (tp = m_Tek_Free) != NULL ) {
		m_Tek_Free = tp->next;
		delete tp;
	}

	if ( m_pTekWnd != NULL )
		m_pTekWnd->DestroyWindow();

	if ( m_pImageWnd != NULL )
		m_pImageWnd->DestroyWindow();

	//if ( m_pCanDlg != NULL )
	//	m_pCanDlg->DestroyWindow();

	if ( m_pWorkGrapWnd != NULL )
		m_pWorkGrapWnd->DestroyWindow();

	while ( pGrapListType != NULL )
		pGrapListType->DestroyWindow();

	SetTraceLog(FALSE);
}

//////////////////////////////////////////////////////////////////////
// Window Function
//////////////////////////////////////////////////////////////////////

void CTextRam::InitText(int Width, int Height)
{
	int n, x;
	CCharCell *tmp, *vp, *wp;
	int oldCurX, oldCurY;
	int oldCols, oldLines;
	int charWidth, charHeight;
	int newCols, newLines;
	int newColsMax, newHisMax;

	if ( !m_bOpen )
		return;

	if ( IsOptEnable(TO_RLFONT) ) {
		charWidth = m_DefFontSize;

		if ( (charHeight = charWidth * 10 / m_DefFontHw) <= 0 )
			charHeight = 1;

		if ( (newCols = Width / charHeight) < 10 )
			newCols = 10;
		else if ( newCols > COLS_MAX )
			newCols = COLS_MAX;

		if ( (newLines = Height / charWidth) <= 0 )
			newLines = 1;

	} else {
		if ( (newCols = m_DefCols[0]) > COLS_MAX )
			newCols = COLS_MAX;
		else if ( newCols < 20 )
			newCols = 20;

		if ( (charHeight  = Width / newCols) <= 0 )
			charHeight = 1;

		if ( (newLines = Height / (charHeight * m_DefFontHw / 10)) <= 0 )
			newLines = 1;

		if ( IsOptValue(TO_DECCOLM, 1) == 1 ) {
			if ( (newCols = m_DefCols[1]) > COLS_MAX )
				newCols = COLS_MAX;
			else if ( newCols < 20 )
				newCols = 20;

			if ( (charHeight  = Width / newCols) <= 0 )
				charHeight = 1;
		}
	}

	if ( newLines > LINE_MAX )
		newLines = LINE_MAX;

	if ( (newHisMax = m_DefHisMax)  < (newLines * 5) )
		newHisMax = newLines * 5;
	else if ( newHisMax > HIS_MAX )
		newHisMax = HIS_MAX;

#ifdef	USE_FIXWCHAR
	if ( newHisMax == m_HisMax && m_hMap != NULL ) {
		if ( newCols == m_Cols && newLines == m_Lines )
			return;

		oldCurX    = m_CurX;
		oldCurY    = m_CurY;
		oldCols    = m_Cols;
		oldLines   = m_Lines;
		newColsMax = (m_ColsMax >= newCols && m_LineUpdate < (m_Lines * 4) ? m_ColsMax : newCols);

		m_HisPos += (m_Lines - newLines);

		if ( newColsMax > m_ColsMax ) {
			for ( n = 0 ; n < m_HisLen ; n++ )
				CCharCell::Fill(GETVRAM(m_ColsMax, newLines - n), m_DefAtt, newColsMax - m_ColsMax);
		}

		if ( m_HisLen < newLines ) {
			for ( n = m_HisLen + 1 ; n <= newLines ; n++ )
				CCharCell::Fill(GETVRAM(0, newLines - n), m_DefAtt, newColsMax);
		}

	} else {
		newColsMax = (m_ColsMax >= newCols && m_LineUpdate < (m_Lines * 4) ? m_ColsMax : newCols);
		HANDLE hNewMap = m_MemMap.Create(ULONGLONG(sizeof(CCharCell) * COLS_MAX) * (ULONGLONG)newHisMax);

		if ( m_hMap != NULL ) {
			oldCurX = (m_ColsMax < newColsMax ? m_ColsMax : newColsMax);
			oldCurY = (m_HisLen < newHisMax ? m_HisLen : newHisMax);

			for ( n = 1 ; n <= oldCurY ; n++ ) {
				CCharCell::Copy(GETMAPRAM(hNewMap, 0, newHisMax - n), GETVRAM(0, m_Lines - n), oldCurX);
				if ( oldCurX < newColsMax )
					CCharCell::Fill(GETMAPRAM(hNewMap, oldCurX, newHisMax - n), m_DefAtt, newColsMax - oldCurX);
			}

			if ( oldCurY < newLines ) {
				for ( n = oldCurY + 1 ; n <= newLines ; n++ )
					CCharCell::Fill(GETMAPRAM(hNewMap, 0, newHisMax - n), m_DefAtt, newColsMax);
			}

			m_MemMap.Close(m_hMap);

			oldCurX  = m_CurX;
			oldCurY  = m_CurY;
			oldCols  = m_Cols;
			oldLines = m_Lines;


		} else {
			for ( n = 1 ; n <= newLines ; n++ )
				CCharCell::Fill(GETMAPRAM(hNewMap, 0, newHisMax - n), m_DefAtt, newColsMax);

			oldCurX  = 0;
			oldCurY  = 0;
			oldCols  = newCols;
			oldLines = newLines;

			m_TopY   = 0;
			m_BtmY   = newLines;
			m_LeftX  = 0;
			m_RightX = newCols;
		}

		m_hMap   = hNewMap;
		m_HisMax = newHisMax;
		m_HisPos = newHisMax - newLines;
	}
#else
	if ( newCols <= m_ColsMax && newHisMax == m_HisMax && m_VRam != NULL ) {
		if ( newCols == m_Cols && newLines == m_Lines )
			return;

		oldCurX    = m_CurX;
		oldCurY    = m_CurY;
		oldCols    = m_Cols;
		oldLines   = m_Lines;
		newColsMax = m_ColsMax;

		m_HisPos += (m_Lines - newLines);

		if ( m_HisLen < newLines ) {
			for ( n = m_HisLen + 1 ; n <= newLines ; n++ )
				CCharCell::Fill(GETVRAM(0, newLines - n), m_DefAtt, m_ColsMax);
		}

	} else {
		newColsMax = (m_ColsMax >= newCols && m_LineUpdate < (m_Lines * 4) ? m_ColsMax : newCols);
		tmp = new CCharCell[newColsMax * newHisMax];

		TRACE("Alloc CharCell Size %d\n", sizeof(CCharCell) * newColsMax * newHisMax);

		if ( m_VRam != NULL ) {
			oldCurX = (m_ColsMax < newColsMax ? m_ColsMax : newColsMax);
			oldCurY = (m_HisLen < newHisMax ? m_HisLen : newHisMax);

			for ( n = 1 ; n <= oldCurY ; n++ ) {
				CCharCell::Copy(tmp + (newHisMax - n) * newColsMax, GETVRAM(0, m_Lines - n), oldCurX);
				if ( oldCurX < newColsMax )
					CCharCell::Fill(tmp + (newHisMax - n) * newColsMax + oldCurX, m_DefAtt, newColsMax - oldCurX);
			}

			if ( oldCurY < newLines ) {
				for ( n = oldCurY + 1 ; n <= newLines ; n++ )
					CCharCell::Fill(tmp + (newHisMax - n) * newColsMax, m_DefAtt, newColsMax);
			}

			delete [] m_VRam;

			oldCurX  = m_CurX;
			oldCurY  = m_CurY;
			oldCols  = m_Cols;
			oldLines = m_Lines;


		} else {
			for ( n = 1 ; n <= newLines ; n++ )
				CCharCell::Fill(tmp + (newHisMax - n) * newColsMax, m_DefAtt, newColsMax);

			oldCurX  = 0;
			oldCurY  = 0;
			oldCols  = newCols;
			oldLines = newLines;

			m_TopY   = 0;
			m_BtmY   = newLines;
			m_LeftX  = 0;
			m_RightX = newCols;
		}

		m_VRam   = tmp;
		m_HisMax = newHisMax;
		m_HisPos = newHisMax - newLines;
	}
#endif

	m_Cols       = newCols;
	m_Lines      = newLines;
	m_ColsMax    = newColsMax;
	m_LineUpdate = 0;

	if ( m_HisLen <= oldLines && m_Lines < oldLines ) {
		for ( n = 0 ; n < oldLines ; n++ ) {
			tmp = GETVRAM(0, m_Lines - oldLines + n);
			for ( x = 0 ; x < m_Cols ; x++ ) {
				if ( !tmp[x].IsEmpty() )
					break;
			}
			if ( x < m_Cols )
				break;
			m_HisLen--;
		}
	}

	if ( m_HisLen < m_Lines )
		m_HisLen = m_Lines;
	else if ( m_HisLen >= m_HisMax )
		m_HisLen = m_HisMax - 1;

	RESET(RESET_CURSOR);

	if ( (m_CurX = oldCurX) >= m_Cols )
		m_CurX = m_Cols - 1;

	if ( (m_CurY = m_Lines - oldLines + oldCurY) < 0 ) {
		m_HisPos += m_CurY;
		if ( m_HisLen == m_Lines )
			m_HisLen += oldCurY;
		else
			m_HisLen += m_CurY;
		m_CurY = 0;
	//} else if ( m_CurY > oldCurY ) {
	//	for ( n = m_Lines - 1 ; n > oldCurY ; n-- ) {
	//		tmp = GETVRAM(0, n);
	//		for ( x = 0 ; x < m_Cols ; x++ ) {
	//			if ( !tmp[x].IsEmpty() )
	//				break;
	//		}
	//		if ( x < m_Cols )
	//			break;
	//	}
	//	if ( m_CurY > n )
	//		m_CurY = n;
	} else if ( m_CurY >= m_Lines ) {
		m_CurY = m_Lines - 1;
	}

	if ( m_TopY > 0 ) {
		if ( (m_TopY += (m_Lines - oldLines)) < 0 )
			m_TopY = 0;
		else if ( m_TopY >= m_Lines )
			m_TopY = m_Lines - 1;
	}

	if ( m_BtmY < oldLines ) {
		if ( (m_BtmY += (m_Lines - oldLines)) < 0 )
			m_BtmY = 0;
		else if ( m_BtmY > m_Lines )
			m_BtmY = m_Lines;
	} else
		m_BtmY = m_Lines;

	if ( m_LeftX >= m_Cols )
		m_LeftX = m_Cols - 1;

	if ( m_RightX == oldCols || m_RightX > m_Cols )
		m_RightX = m_Cols;

	if ( m_Cols != oldCols || m_Lines != oldLines )
		m_pDocument->SocketSendWindSize(m_Cols, m_Lines);

//	DISPUPDATE();
//	FLUSH();
}
void CTextRam::InitScreen(int cols, int lines)
{
	if ( !IsInitText() )
		return;

	int n, x, cx, cy;
	int colsmax = (m_ColsMax > cols ? m_ColsMax : cols);
	CCharCell *tmp, *vp, *wp;
	CRect rect, box;
	CWnd *pWnd = ::AfxGetMainWnd();

	if ( IsOptEnable(TO_RLFONT) ) {
		if ( pWnd->IsIconic() || pWnd->IsZoomed() )
			return;

		pWnd->GetWindowRect(rect);
		pWnd->GetClientRect(box);
		cx = (rect.Width()  - box.Width())  + box.Width()  * cols  / m_Cols;
		cy = (rect.Height() - box.Height()) + box.Height() * lines / m_Lines;
		pWnd->SetWindowPos(NULL, rect.left, rect.top, cx, cy, SWP_SHOWWINDOW);

	} else if ( pWnd->IsIconic() || pWnd->IsZoomed() || !IsOptEnable(TO_RLNORESZ) ) {

#ifdef	USE_FIXWCHAR
		m_HisPos += (m_Lines - lines);

		if ( colsmax > m_ColsMax ) {
			for ( n = 0 ; n < m_HisLen ; n++ )
				CCharCell::Fill(GETVRAM(m_ColsMax, lines - n), m_DefAtt, colsmax - m_ColsMax);
		}

		if ( m_HisLen < lines ) {
			for ( n = m_HisLen + 1 ; n <= lines ; n++ )
				CCharCell::Fill(GETVRAM(0, lines - n), m_DefAtt, colsmax);
		}

		m_Cols    = cols;
		m_ColsMax = colsmax;
		m_Lines   = lines;
		m_LineUpdate = 0;
#else
		tmp = new CCharCell[colsmax * m_HisMax];

		cx = (m_ColsMax < colsmax ? m_ColsMax : colsmax);
		for ( n = 1 ; n <= m_HisLen ; n++ ) {
			CCharCell::Copy(tmp + (m_HisMax - n) * colsmax, GETVRAM(0, m_Lines - n), cx);
			if ( cx < colsmax )
				CCharCell::Fill(tmp + (m_HisMax - n) * colsmax + cx, m_DefAtt, colsmax - cx);
		}

		if ( m_HisLen < lines ) {
			for ( n = m_HisLen + 1 ; n <= lines ; n++ )
				CCharCell::Fill(tmp + (m_HisMax - n) * colsmax, m_DefAtt, colsmax);
			m_HisLen = lines;
		}

		delete [] m_VRam;

		m_VRam    = tmp;

		m_HisPos  = m_HisMax - lines;
		m_Cols    = cols;
		m_ColsMax = colsmax;
		m_Lines   = lines;
		m_LineUpdate = 0;
#endif

		if ( m_CurX >= m_Cols )
			m_CurX = m_Cols - 1;

		if ( m_CurY >= m_Lines )
			m_CurY = m_Lines - 1;

		m_pDocument->SocketSendWindSize(m_Cols, m_Lines);
		m_pDocument->UpdateAllViews(NULL, UPDATE_INITSIZE, NULL);

	} else {
		pWnd->GetWindowRect(rect);
		pWnd->GetClientRect(box);
		cx = (rect.Width()  - box.Width())  + box.Width()  * cols  / m_Cols;
		cy = (rect.Height() - box.Height()) + box.Height() * lines / m_Lines;
		pWnd->SetWindowPos(NULL, rect.left, rect.top, cx, cy, SWP_SHOWWINDOW);
	}
}

BOOL CTextRam::OpenHisFile()
{
	int n;
	int num = 2;
	CString name = m_HisFile;
	CString base = name;
	
	if ( m_pDocument != NULL )
		m_pDocument->EntryText(name);

	if ( (n = name.ReverseFind(_T('.'))) >= 0 )
		base = name.Left(n);

	while ( num < 20 && !m_HisFhd.Open(name, CFile::modeReadWrite | CFile::modeCreate | CFile::modeNoTruncate | CFile::shareExclusive) )
		name.Format(_T("%s(%d).rlh"), base, num++);

	if ( m_HisFhd.m_hFile == CFile::hFileNull ) {
		name.Format(_T("History File Open Error '%s.rlh'"), base);
		::AfxMessageBox(name);
		return FALSE;
	}

	return TRUE;
}
void CTextRam::InitHistory()
{
	m_LineEditMapsInit = 0;

	if ( !IsOptEnable(TO_RLHISFILE) || m_HisFile.IsEmpty() || !IsInitText() )
		return;

	if ( !OpenHisFile() )
		return;

	try {
		CSpace spc;
		DWORD mx, my, nx;
		char head[5];
		BYTE tmp[8];
		CCharCell ram, *vp;

		m_HisFhd.SeekToBegin();
		if ( m_HisFhd.Read(head, 4) != 4 )
			AfxThrowFileException(CFileException::endOfFile);
		if ( strncmp(head, "RLH", 3) != 0 )
			AfxThrowFileException(CFileException::fileNotFound);
		if ( m_HisFhd.Read(&mx, sizeof(DWORD)) != sizeof(DWORD) )
			AfxThrowFileException(CFileException::endOfFile);
		if ( m_HisFhd.Read(&my, sizeof(DWORD)) != sizeof(DWORD) )
			AfxThrowFileException(CFileException::endOfFile);

		for ( DWORD n = 0 ; n < my ; n++ ) {
			vp = GETVRAM(0, (-1) - n);
			for ( int x = 0 ; x < mx ; x++ ) {
				ram.Read(m_HisFhd, head[3] - '0');
				if ( x < m_ColsMax && m_HisLen < (m_HisMax - m_Lines - 1) )
					*(vp++) = ram;
			}
			if ( m_HisLen < (m_HisMax - m_Lines - 1) )
				m_HisLen++;
		}

		m_LineEditMapsTab[2].RemoveAll();
		if ( m_HisFhd.Read(&mx, sizeof(DWORD)) == sizeof(DWORD) ) {
			for ( nx = 0 ; nx < mx ; nx++ ) {
				if ( m_HisFhd.Read(&my, sizeof(DWORD)) != sizeof(DWORD) || my < 0 || my > 1024 )
					break;
				spc.SetSize(my);
				if ( m_HisFhd.Read(spc.GetPtr(), my) != my )
					break;
				m_LineEditHis.Add(spc);
				m_LineEditMapsTab[2].AddWStrBuf(spc.GetPtr(), spc.GetSize());
			}
		}

	} catch(...) {
		return;
	}
}
void CTextRam::SaveHistory()
{
	if ( !IsOptEnable(TO_RLHISFILE) || m_HisFile.IsEmpty() || !IsInitText() )
		return;

	if ( m_HisFhd.m_hFile == CFile::hFileNull ) {
		if ( !OpenHisFile() )
			return;
	}

	try {
		CCharCell *vp;
		DWORD n, i;

		m_HisFhd.SeekToBegin();
		m_HisFhd.Write("RLH3", 4);
		n = m_Cols; m_HisFhd.Write(&n, sizeof(DWORD));
		n = m_HisLen; m_HisFhd.Write(&n, sizeof(DWORD));
		for ( n = 0 ; (int)n < m_HisLen ; n++ ) {
			vp = GETVRAM(0, m_Lines - n - 1);
			for ( i = 0 ; i < m_Cols ; i++, vp++ )
				vp->Write(m_HisFhd);
//			m_HisFhd.Write(vp, sizeof(VRAM) * m_Cols);
		}

		n = (DWORD)m_LineEditHis.GetSize(); m_HisFhd.Write(&n, sizeof(DWORD));
		for ( i = 0 ; i < (DWORD)m_LineEditHis.GetSize() ; i++ ) {
			n = (DWORD)m_LineEditHis.GetAt(i).GetSize(); m_HisFhd.Write(&n, sizeof(DWORD));
			m_HisFhd.Write(m_LineEditHis.GetAt(i).GetPtr(), n);
		}

	} catch(...) {
		AfxMessageBox(IDE_HISTORYBACKUPERROR);
	}
}
void CTextRam::HisRegCheck(DWORD ch, DWORD pos)
{
	int i, a;
	CRegExRes res;

	if ( m_MarkReg.MatchChar(ch, pos, &res) && (res.m_Status == REG_MATCH || res.m_Status == REG_MATCHOVER) ) {
		for ( i = 0 ; i < res.GetSize() ; i++ ) {
			for ( a = res[i].m_SPos ; a < res[i].m_EPos ; a++ ) {
				if ( res.m_Idx[a] != 0xFFFFFFFF )
					GETVRAM(res.m_Idx[a] % m_ColsMax, res.m_Idx[a] / m_ColsMax - m_HisPos)->m_Vram.attr |= ATT_MARK;
			}
		}
	}
}
int CTextRam::HisRegMark(LPCTSTR str)
{
	int n, x;
	CCharCell *vp;

	m_MarkPos = m_HisPos + m_Lines - m_HisLen;

	while ( m_MarkPos < 0 )
		m_MarkPos += m_HisMax;

	while ( m_MarkPos >= m_HisMax )
		m_MarkPos -= m_HisMax;

	if ( str == NULL || *str == '\0' ) {
		for ( n = 0 ; n < m_HisLen ; n++ ) {
			vp = GETVRAM(0, m_MarkPos - m_HisPos);
			for ( x = 0 ; x < m_Cols ; x++ )
				vp[x].m_Vram.attr &= ~ATT_MARK;
			while ( ++m_MarkPos >= m_HisMax )
				m_MarkPos -= m_HisMax;
		}
		return 0;
	}

	m_MarkReg.Compile(str);
	m_MarkReg.MatchCharInit();

	m_MarkLen = 0;
	m_MarkEol = TRUE;

	return m_HisLen;
}
int CTextRam::HisRegNext()
{
	int n, x, ex, mx;
	CCharCell *vp;
	CStringD str;
	LPCDSTR p;

	for ( mx = m_MarkLen + 100 ; m_MarkLen < mx && m_MarkLen < m_HisLen ; m_MarkLen++ ) {
		vp = GETVRAM(0, m_MarkPos - m_HisPos);
		for ( ex = m_Cols - 1 ; ex >= 0 ; ex-- ) {
			if ( !vp[ex].IsEmpty() )
				break;
			vp[ex].m_Vram.attr &= ~ATT_MARK;
		}

		if ( m_MarkEol )
			HisRegCheck(L'\n', 0xFFFFFFFF);

		for ( x = 0 ; x <= ex ; x += n ) {
			str.Empty();
			vp[x].m_Vram.attr &= ~ATT_MARK;
			if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				str += (LPCWSTR)vp[x];
				n = 2;
			} else if ( !IS_ASCII(vp[x].m_Vram.mode) || (DWORD)(vp[x]) == 0 ) {
				str += (DCHAR)' ';
				n = 1;
			} else {
				str += (LPCWSTR)vp[x];
				n = 1;
			}

			for ( p = str ; *p != 0 ; p++ )
				HisRegCheck(*p, m_ColsMax * m_MarkPos + x);
		}

		if ( x < m_Cols ) {
			HisRegCheck(L'\r', 0xFFFFFFFF);
			m_MarkEol = TRUE;
		} else
			m_MarkEol = FALSE;

		while ( ++m_MarkPos >= m_HisMax )
			m_MarkPos -= m_HisMax;
	}

	return m_MarkLen;
}
int CTextRam::HisMarkCheck(int top, int line, class CRLoginView *pView)
{
	int x, y;
	CCharCell *vp;

	for ( y = 0 ; y < line ; y++ ) {
		vp = GETVRAM(0, top + y);
		for ( x = 0 ; x < m_Cols ; x++, vp++ ) {
			if ( (vp->m_Vram.attr & ATT_MARK) != 0 )
				return TRUE;
		}
	}
	return FALSE;
}

static const COLORREF DefColTab[16] = {
		RGB(  0,   0,   0),	RGB(196,   0,   0),
		RGB(  0, 196,   0),	RGB(196, 196,   0),
		RGB(  0,   0, 196),	RGB(196,   0, 196),
		RGB(  0, 196, 196),	RGB(196, 196, 196),

		RGB(128, 128, 128),	RGB(255,   0,   0),
		RGB(  0, 255,   0),	RGB(255, 255,   0),
		RGB(  0,   0, 255),	RGB(255,   0, 255),
		RGB(  0, 255, 255),	RGB(255, 255, 255),
	};
static const WORD DefBankTab[5][4] = {
	/* EUC */
	{ { SET_94    | 'J' }, 	{ SET_94x94 | 'Q' },
	  { SET_94    | 'I' }, 	{ SET_94x94 | 'P' } },
	/* SJIS */
	{ { SET_94    | 'J' }, 	{ SET_94    | 'I' },
	  { SET_94x94 | 'Q' }, 	{ SET_94x94 | 'P' } },
	/* ASCII */
	{ { SET_94    | 'B' }, 	{ SET_94    | '1' },
	  { SET_94    | 'J' }, 	{ SET_94    | 'I' } },
	/* UTF-8 */
	{ { SET_94    | 'B' }, 	{ SET_96    | 'A' },
	  { SET_94    | 'J' }, 	{ SET_94    | 'I' } },
	/* BIG5 */
	{ { SET_94    | 'T' }, 	{ SET_94    | '1' },
	  { SET_94x94 | 'A' }, 	{ SET_94x94 | 'E' } },
	};
static const VRAM TempAtt = {
	//  ch  at  ft  md  dm  cm  em  fc  bc
		0,  0,  0,  0,  0,  0,  0,  7,  0
};
static const LPCTSTR DropCmdTab[] = {
	//	Non					BPlus				XModem					YModem
		_T(""),				_T("bp -d %s\\r"),	_T("rx %s\\r"),			_T("rb\\r"),
	//	ZModem				SCP					Kermit
		_T("rz\\r"),		_T("scp -t %s"),	_T("kermit -i -r"),		_T(""),
	};
static const LPCTSTR InitKeyList[] = {
	_T(""),
	_T("UP,DOWN,RIGHT,LEFT,PRIOR,NEXT,HOME,END,INSERT,DELETE"),
	_T("F1-F12"),
	_T("PAD0-PAD9,PADMUL,PADADD,PADSEP,PADSUB,PADDEC,PADDIV"),
	_T("ESCAPE,RETURN,BACK,TAB,SPACE"),
	_T("0-9,A-Z"),
};

void CTextRam::Init()
{
	m_DefCols[0]	= 80;
	m_DefCols[1]	= 132;
	m_Page          = 0;
	m_DefHisMax		= 2000;
	m_DefFontSize	= 16;
	m_DefFontHw     = 20;
	m_KanjiMode		= EUC_SET;
	m_BankGL		= 0;
	m_BankGR		= 1;
	m_DefAtt		= TempAtt;

	memcpy(m_DefColTab, DefColTab, sizeof(m_DefColTab));
	memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));
	memset(m_AnsiOpt, 0, sizeof(DWORD) * 16);
	memset(m_OptTab,  0, sizeof(DWORD) * 16);
	EnableOption(TO_ANSISRM);	//    12 SRM Set Send/Receive mode (Local echo off)
	EnableOption(TO_DECANM);	// ?   2 ANSI/VT52 Mode
	EnableOption(TO_DECAWM);	// ?   7 Autowrap mode
	EnableOption(TO_DECARM);	// ?   8 Autorepeat mode
	EnableOption(TO_DECTCEM);	// ?  25 Text Cursor Enable Mode
	EnableOption(TO_XTMCUS);	// ?  41 XTerm tab bug fix
	EnableOption(TO_XTMRVW);	// ?  45 XTerm Reverse-wraparound mod
	EnableOption(TO_DECBKM);	// ?  67 Backarrow key mode (BS)
	EnableOption(TO_XTPRICOL);	// ?1070 Private Color Map
	EnableOption(TO_RLFONT);	// ?8404 フォントサイズから一行あたりの文字数を決定
	EnableOption(TO_DRCSMMv1);	// ?8800 Unicode 16 Maping

	memcpy(m_DefAnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_DefBankTab, DefBankTab, sizeof(m_DefBankTab));
	memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));
	m_SendCharSet[0] = _T("EUCJP-MS");
	m_SendCharSet[1] = _T("SHIFT-JIS");
	m_SendCharSet[2] = _T("ISO-2022-JP");
	m_SendCharSet[3] = _T("UTF-8");
	m_SendCharSet[4] = _T("BIG-5");

	m_WheelSize      = 2;
	m_BitMapFile     = _T("");
	m_DelayMSec      = 0;
	m_HisFile        = _T("");
	m_KeepAliveSec   = 0;
	m_DropFileMode   = 0;

	for ( int n = 0 ; n < 8 ; n++ )
		m_DropFileCmd[n] = DropCmdTab[n];

	m_WordStr        = _T("\\/._");
	m_MouseTrack     = MOS_EVENT_NONE;
	m_MouseRect.SetRectEmpty();
	m_MouseMode[0]   = 0;
	m_MouseMode[1]   = 1;
	m_MouseMode[2]   = 4;
	m_MouseMode[3]   = 16;

	memset(m_MetaKeys, 0, sizeof(m_MetaKeys));
	m_TitleMode = WTTL_ENTRY;
	m_VtLevel = 65;
	m_TermId  = 10;
	m_LangMenu = 0;
	m_StsFlag = FALSE;
	m_StsMode = 0;
	m_StsLed  = 0;
	SetRetChar(FALSE);

	for ( int n = 0 ; n < 64 ; n++ )
		m_Macro[n].Clear();

	m_UnitId = 0;
	m_ClipFlag = 0;
	memset(m_MacroExecFlag, 0, sizeof(m_MacroExecFlag));
	m_ShellExec.GetString(_T("http://|https://|ftp://|mailto://"), _T('|'));
	m_ScrnOffset.SetRect(0, 0, 0, 0);
	m_TimeFormat = _T("%H:%M:%S ");
	m_DefFontName[0] = _T("ＭＳ ゴシック");
	m_DefFontName[1] = _T("ＭＳ 明朝");

	m_TraceLogFile.Empty();
	m_TraceMaxCount = 1000;

	for ( int n = 0 ; n < MODKEY_MAX ; n++ ) {
		m_DefModKey[n] = (-1);
		m_ModKeyList[n] = InitKeyList[n];
	}
	memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
	InitModKeyTab();

	m_ProcTab.Init();
//	m_FontTab.Init();

	RESET();
}
void CTextRam::SetIndex(int mode, CStringIndex &index)
{
	int n, i;
	CString str, wrk;
	static const LPCTSTR setname[] = { _T("EUC"), _T("SJIS"), _T("ASCII"), _T("UTF8"), _T("BIG5") };

	if ( mode ) {	// Write
		index[_T("Cols")][_T("Nomal")] = m_DefCols[0];
		index[_T("Cols")][_T("Wide")]  = m_DefCols[1];

		index[_T("History")]  = m_DefHisMax;
		index[_T("FontSize")] = m_DefFontSize;
		index[_T("CharSet")]  = m_KanjiMode;

		index[_T("GL")] = m_BankGL;
		index[_T("GR")] = m_BankGR;

		index[_T("Attribute")] = m_DefAtt.attr;
		index[_T("TextColor")] = m_DefAtt.fcol;
		index[_T("BackColor")] = m_DefAtt.bcol;

		index[_T("ColorTable")].SetSize(16);
		for ( n = 0 ; n < 16 ; n++ ) {
			index[_T("ColorTable")][n].Add(GetRValue(m_DefColTab[n]));
			index[_T("ColorTable")][n].Add(GetGValue(m_DefColTab[n]));
			index[_T("ColorTable")][n].Add(GetBValue(m_DefColTab[n]));
		}

		for ( n = 0 ; n < 5 ; n++ ) {
			index[_T("BankTable")][setname[n]].SetSize(4);
			for ( i = 0 ; i < 4 ; i++ ) {
				index[_T("BankTable")][setname[n]][i].Add(m_DefBankTab[n][i] >> 8);
				index[_T("BankTable")][setname[n]][i].Add(m_FontTab[m_DefBankTab[n][i]].m_IndexName);
			}
		}

		for ( n = 0 ; n < 4 ; n++ )
			index[_T("SendCharSet")].Add(m_SendCharSet[n]);

		index[_T("BitMapFile")]   = m_BitMapFile;
		index[_T("HisFile")]      = m_HisFile;
		index[_T("LogFile")]      = m_LogFile;

		index[_T("WheelSize")]    = m_WheelSize;
		index[_T("DelayMSec")]    = m_DelayMSec;
		index[_T("KeepAliveSec")] = m_KeepAliveSec;

		index[_T("WordStr")]      = m_WordStr;
		index[_T("DropFileMode")] = m_DropFileMode;

		for ( n = 0 ; n < 8 ; n++ )
			index[_T("DropFileCmd")].Add(m_DropFileCmd[n]);

		for ( n = 0 ; n < 4 ; n++ )
			index[_T("MouseMode")].Add(m_MouseMode[n]);

		index[_T("TitleMode")] = m_TitleMode;
		index[_T("ClipFlag")]  = m_ClipFlag;
		index[_T("FontHw")]    = m_DefFontHw;

		index[_T("RecvCrLf")]  = m_RecvCrLf;
		index[_T("SendCrLf")]  = m_SendCrLf;

		for ( n = 0 ; n < m_ShellExec.GetSize() ; n++ )
			index[_T("ShellExec")].Add(m_ShellExec[n]);

		for ( n = 0 ; n < MODKEY_MAX ; n++ ) {
			index[_T("ModKey")].Add(m_DefModKey[n]);
			index[_T("ModKeyList")].Add(m_ModKeyList[n]);
		}

		for ( n = 0 ; n < (32 * 8) ; n++ ) {
			if ( IS_ENABLE(m_MetaKeys, n) )
				index[_T("MetaKeys")].Add(n);
		}

		for ( n = 0 ; n < (32 * 16) ; n++ ) {
			if ( n >= 400 )				// RLogin Option		8400-8511(400-511)
				i = n + (8400 - 400);
			else if ( n >= 380 )		// XTerm Option 2		2000-2019(380-399)
				i = n + (2000 - 380);
			else if ( n >= 300 )		// XTerm Option			1000-1079(300-379)
				i = n + (1000 - 300);
			else if ( n >= 200 )		// ANSI Screen Option	200-299(200-299)
				i = n + (200 - 200);
			else						// DEC Terminal Option	0-199
				i = n;

			if ( IS_ENABLE(m_DefAnsiOpt, n) )
				index[_T("Option")].Add(i);
		}

		for ( n = 0 ; n < (32 * 16) ; n++ ) {
			if ( IS_ENABLE(m_OptTab, n) )
				index[_T("SocketOpt")].Add(n);
		}

		m_ProcTab.SetIndex(mode, index[_T("ProcTable")]);

		index[_T("ScreenOffset")].Add(m_ScrnOffset.left);
		index[_T("ScreenOffset")].Add(m_ScrnOffset.right);
		index[_T("ScreenOffset")].Add(m_ScrnOffset.top);
		index[_T("ScreenOffset")].Add(m_ScrnOffset.bottom);

		index[_T("TimeFormat")] = m_TimeFormat;

		for ( n = 0 ; n < 16 ; n++ )
			index[_T("DefFontName")].Add(m_DefFontName[n]);

		index[_T("TraceLogFile")]  = m_TraceLogFile;
		index[_T("TraceMaxCount")] = m_TraceMaxCount;

	} else {		// Read
		if ( (n = index.Find(_T("Cols"))) >= 0 ) {
			if ( (i = index[n].Find(_T("Nomal"))) >= 0 )
				m_DefCols[0] = index[n][i];
			if ( (i = index[n].Find(_T("Wide"))) >= 0 )
				m_DefCols[1] = index[n][i];
		}

		if ( (n = index.Find(_T("History"))) >= 0 )
			m_DefHisMax = index[n];
		if ( (n = index.Find(_T("FontSize"))) >= 0 )
			m_DefFontSize = index[n];
		if ( (n = index.Find(_T("CharSet"))) >= 0 )
			m_KanjiMode = index[n];

		if ( (n = index.Find(_T("GL"))) >= 0 )
			m_BankGL = index[n];
		if ( (n = index.Find(_T("GR"))) >= 0 )
			m_BankGR = index[n];

		if ( (n = index.Find(_T("Attribute"))) >= 0 )
			m_DefAtt.attr = index[n];
		if ( (n = index.Find(_T("TextColor"))) >= 0 )
			m_DefAtt.fcol = index[n];
		if ( (n = index.Find(_T("BackColor"))) >= 0 )
			m_DefAtt.bcol = index[n];

		if ( (n = index.Find(_T("ColorTable"))) >= 0 ) {
			for ( i = 0 ; i < 16 && i < index[n].GetSize() ; i++ ) {
				if ( index[n][i].GetSize() < 3 )
					continue;
				m_DefColTab[i] = RGB(index[n][i][0], index[n][i][1], index[n][i][2]);
			}
		}

		if ( (n = index.Find(_T("BankTable"))) >= 0 ) {
			for ( i = 0 ; i < 5 ; i++ ) {
				int a, b;
				if ( (a = index[n].Find(setname[i])) >= 0 ) {
					for ( b = 0 ; b < 4 && b < index[n][a].GetSize() ; b++ ) {
						if ( index[n][a].GetSize() < 2 )
							continue;
						m_DefBankTab[i][b] = m_FontTab.IndexFind(index[n][a][b][0] << 8, index[n][a][b][1]);
					}
				}
			}
		}

		if ( (n = index.Find(_T("SendCharSet"))) >= 0 ) {
			for ( i = 0 ; i < 4 && i < index[n].GetSize() ; i++ )
				m_SendCharSet[i] = index[n][i];
		}

		if ( (n = index.Find(_T("BitMapFile"))) >= 0 )
			m_BitMapFile = index[n];
		if ( (n = index.Find(_T("HisFile"))) >= 0 )
			m_HisFile = index[n];
		if ( (n = index.Find(_T("LogFile"))) >= 0 )
			m_LogFile = index[n];

		if ( (n = index.Find(_T("WheelSize"))) >= 0 )
			m_WheelSize = index[n];
		if ( (n = index.Find(_T("DelayMSec"))) >= 0 )
			m_DelayMSec = index[n];
		if ( (n = index.Find(_T("KeepAliveSec"))) >= 0 )
			m_KeepAliveSec = index[n];

		if ( (n = index.Find(_T("WordStr"))) >= 0 )
			m_WordStr = index[n];
		if ( (n = index.Find(_T("DropFileMode"))) >= 0 )
			m_DropFileMode = index[n];

		if ( (n = index.Find(_T("DropFileCmd"))) >= 0 ) {
			for ( i = 0 ; i < 8 && i < index[n].GetSize() ; i++ )
				m_DropFileCmd[i] = index[n][i];
		}

		if ( (n = index.Find(_T("MouseMode"))) >= 0 ) {
			for ( i = 0 ; i < 4 && i < index[n].GetSize() ; i++ )
				m_MouseMode[i] = index[n][i];
		}

		if ( (n = index.Find(_T("TitleMode"))) >= 0 )
			m_TitleMode = index[n];
		if ( (n = index.Find(_T("ClipFlag"))) >= 0 )
			m_ClipFlag = index[n];
		if ( (n = index.Find(_T("FontHw"))) >= 0 )
			m_DefFontHw = index[n];

		if ( (n = index.Find(_T("RecvCrLf"))) >= 0 )
			m_RecvCrLf = index[n];
		if ( (n = index.Find(_T("SendCrLf"))) >= 0 )
			m_SendCrLf = index[n];

		if ( (n = index.Find(_T("ShellExec"))) >= 0 ) {
			m_ShellExec.RemoveAll();
			for ( i = 0 ; i < index[n].GetSize() ; i++ )
				m_ShellExec.Add(index[n][i]);
		}

		if ( (n = index.Find(_T("ModKey"))) >= 0 ) {
			for ( i = 0 ; i < MODKEY_MAX && i < index[n].GetSize() ; i++ )
				m_DefModKey[i] = index[n][i];
			memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
		}

		if ( (n = index.Find(_T("ModKeyList"))) >= 0 ) {
			for ( i = 0 ; i < MODKEY_MAX && i < index[n].GetSize() ; i++ )
				m_ModKeyList[i] = index[n][i];
			InitModKeyTab();
		}

		m_ProcTab.SetIndex(mode, index[_T("ProcTable")]);

		if ( (n = index.Find(_T("MetaKeys"))) >= 0 ) {
			memset(m_MetaKeys, 0, sizeof(m_MetaKeys));
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int b = index[n][i];
				if ( b >= 0 && b <= 255 )
					m_MetaKeys[b / 32] |= (1 << (b % 32));
			}
		}

		if ( (n = index.Find(_T("ScreenOffset"))) >= 0 ) {
			if ( index[n].GetSize() > 0 )
				m_ScrnOffset.left   = (int)(index[_T("ScreenOffset")][0]);
			if ( index[n].GetSize() > 1 )
				m_ScrnOffset.right  = (int)(index[_T("ScreenOffset")][1]);
			if ( index[n].GetSize() > 2 )
				m_ScrnOffset.top    = (int)(index[_T("ScreenOffset")][2]);
			if ( index[n].GetSize() > 3 )
				m_ScrnOffset.bottom = (int)(index[_T("ScreenOffset")][3]);
		}

		if ( (n = index.Find(_T("TimeFormat"))) >= 0 )
			m_TimeFormat = index[n];

		if ( (n = index.Find(_T("DefFontName"))) >= 0 ) {
			for ( i = 0 ; i < 16 && i < index[n].GetSize() ; i++ )
				m_DefFontName[i] = index[n][i];
		}

		if ( (n = index.Find(_T("TraceLogFile"))) >= 0 )
			m_TraceLogFile = index[n];
		if ( (n = index.Find(_T("TraceMaxCount"))) >= 0 )
			m_TraceMaxCount = index[n];

		if ( (n = index.Find(_T("Option"))) >= 0 ) {
			memset(m_DefAnsiOpt, 0, sizeof(m_DefAnsiOpt));
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int a, b = index[n][i];

				if ( b >= 8400 )			// RLogin Option		8400-8511(400-511)
					a = b - (8400 - 400);
				else if ( b >= 2000 )		// XTerm Option 2		2000-2019(380-399)
					a = b - (2000 - 380);
				else if ( b >= 1000 )		// XTerm Option			1000-1079(300-379)
					a = b - (1000 - 300);
				else if ( b >= 200 )		// ANSI Screen Option	200-299(200-299)
					a = b - (200 - 200);
				else						// DEC Terminal Option	0-199
					a = b;

				if ( a >= 0 && a <= 511 )
					m_DefAnsiOpt[a / 32] |= (1 << (a % 32));
			}
		}

		if ( (n = index.Find(_T("SocketOpt"))) >= 0 ) {
			memset(m_OptTab, 0, sizeof(m_OptTab));
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int b = index[n][i];
				if ( b >= 0 && b <= 511 )
					m_OptTab[b / 32] |= (1 << (b % 32));
			}
		}

		memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));
		memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));
		memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));

		RESET();
	}
}
void CTextRam::SetArray(CStringArrayExt &stra)
{
	CString str;
	CStringArrayExt tmp;

	stra.RemoveAll();

	stra.AddVal(m_DefCols[0]);
	stra.AddVal(m_DefHisMax);
	stra.AddVal(m_DefFontSize);
	stra.AddVal(m_KanjiMode);
	stra.AddVal(m_BankGL);
	stra.AddVal(m_BankGR);
	stra.AddBin(&m_DefAtt, sizeof(VRAM));
	stra.AddBin(m_DefColTab,  sizeof(m_DefColTab));
	stra.AddBin(m_DefAnsiOpt, sizeof(m_DefAnsiOpt));
	stra.AddBin(m_DefBankTab, sizeof(m_DefBankTab));
	stra.Add(m_SendCharSet[0]);
	stra.Add(m_SendCharSet[1]);
	stra.Add(m_SendCharSet[2]);
	stra.Add(m_SendCharSet[3]);
	stra.AddVal(m_WheelSize);
	stra.Add(m_BitMapFile);
	stra.AddVal(m_DelayMSec);
	stra.Add(m_HisFile);
	stra.AddVal(m_KeepAliveSec);
	stra.Add(m_LogFile);
	stra.AddVal(m_DefCols[1]);
	stra.AddVal(m_DropFileMode);
	for ( int n = 0 ; n < 8 ; n++ )
		stra.Add(m_DropFileCmd[n]);
	stra.Add(m_WordStr);
	for ( int n = 0 ; n < 4 ; n++ )
		stra.AddVal(m_MouseMode[n]);
	stra.AddBin(m_MetaKeys,  sizeof(m_MetaKeys));

	m_ProcTab.SetArray(tmp);
	tmp.SetString(str, _T(';'));
	stra.Add(str);

	stra.AddVal(8);	// AnsiOpt Bugfix

	stra.AddVal(m_TitleMode);
	stra.Add(m_SendCharSet[4]);
	stra.AddVal(m_ClipFlag);
	stra.AddVal(m_DefFontHw);

	m_ShellExec.SetString(str, _T('|'));
	stra.Add(str);

	stra.AddVal(m_RecvCrLf);
	stra.AddVal(m_SendCrLf);
	stra.AddBin(m_OptTab, sizeof(m_OptTab));
	stra.AddBin(m_DefModKey, sizeof(m_DefModKey));

	stra.Add(m_ModKeyList[1]);
	stra.Add(m_ModKeyList[2]);
	stra.Add(m_ModKeyList[3]);
	stra.Add(m_ModKeyList[4]);
	stra.Add(m_ModKeyList[5]);

	stra.AddVal(m_ScrnOffset.left);
	stra.AddVal(m_ScrnOffset.right);
	stra.AddVal(m_ScrnOffset.top);
	stra.AddVal(m_ScrnOffset.bottom);

	stra.Add(m_TimeFormat);

	tmp.RemoveAll();
	for ( int n = 0 ; n < 16 ; n++ )
		tmp.Add(m_DefFontName[n]);
	tmp.SetString(str, _T(';'));
	stra.Add(str);

	stra.Add(m_TraceLogFile);
	stra.AddVal(m_TraceMaxCount);
}
void CTextRam::GetArray(CStringArrayExt &stra)
{
	int n, v;
	BYTE tmp[16];
	CStringArrayExt ext;

	Init();

	m_DefCols[0]  = stra.GetVal(0);
	m_DefHisMax   = stra.GetVal(1);
	m_DefFontSize = stra.GetVal(2);
	m_KanjiMode   = stra.GetVal(3);
	m_BankGL      = stra.GetVal(4);
	m_BankGR      = stra.GetVal(5);

	if ( (n = stra.GetBin(6, tmp, 16)) == sizeof(VRAM) )
		memcpy(&m_DefAtt, tmp, sizeof(VRAM));
	else if ( n == 8 ) {
		memset(&m_DefAtt, 0, sizeof(VRAM));
		m_DefAtt.attr = tmp[6];
		m_DefAtt.fcol = tmp[7] & 0x0F;
		m_DefAtt.bcol = tmp[7] >> 4;
	}

	stra.GetBin(7, m_DefColTab,  sizeof(m_DefColTab));
	memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));
	stra.GetBin(8, m_DefAnsiOpt, sizeof(m_DefAnsiOpt));
	memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));

	memcpy(m_DefBankTab, DefBankTab, sizeof(m_DefBankTab));
	stra.GetBin(9, m_DefBankTab, sizeof(m_DefBankTab));
	memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));

	m_SendCharSet[0] = stra.GetAt(10);
	m_SendCharSet[1] = stra.GetAt(11);
	m_SendCharSet[2] = stra.GetAt(12);
	m_SendCharSet[3] = stra.GetAt(13);

	m_WheelSize    = stra.GetVal(14);
	m_BitMapFile   = stra.GetAt(15);

	m_DelayMSec    = (stra.GetSize() > 16 ? stra.GetVal(16) : 0);
	m_HisFile      = (stra.GetSize() > 17 ? stra.GetAt(17) : _T(""));
	m_KeepAliveSec = (stra.GetSize() > 18 ? stra.GetVal(18) : 0);
	m_LogFile      = (stra.GetSize() > 19 ? stra.GetAt(19) : _T(""));
	m_DefCols[1]   = (stra.GetSize() > 20 ? stra.GetVal(20) : 132);

	m_DropFileMode = (stra.GetSize() > 21 ? stra.GetVal(21) : 0);
	for ( n = 0 ; n < 8 ; n++ )
		m_DropFileCmd[n]  = (stra.GetSize() > (22 + n) ? stra.GetAt(22 + n) : DropCmdTab[n]);

	m_WordStr      = (stra.GetSize() > 30 ? stra.GetAt(30) : _T("\\/._"));

	m_MouseMode[0] = (stra.GetSize() > 31 ? stra.GetVal(31) : 0);
	m_MouseMode[1] = (stra.GetSize() > 32 ? stra.GetVal(32) : 1);
	m_MouseMode[2] = (stra.GetSize() > 33 ? stra.GetVal(33) : 4);
	m_MouseMode[3] = (stra.GetSize() > 34 ? stra.GetVal(34) : 16);

	if ( stra.GetSize() > 35 )
		stra.GetBin(35, m_MetaKeys, sizeof(m_MetaKeys));

	if ( stra.GetSize() > 36 ) {
		ext.GetString(stra.GetAt(36), _T(';'));
		m_ProcTab.GetArray(ext);
	}

	v = (stra.GetSize() > 37 ? stra.GetVal(37) : 0);
	if ( v < 1 ) {
		ReversOption(TO_DECANM);
		EnableOption(TO_DECTCEM);	// ?25 Text Cursor Enable Mode
	}
	if ( v < 2 )
		EnableOption(TO_XTMCUS);	// ?41 XTerm tab bug fix
	if ( v < 3 ) {
		EnableOption(TO_DECBKM);	// ?67 Backarrow key mode (BS
		EnableOption(TO_ANSISRM);	//  12 SRM Set Send/Receive mode (Local echo off)
	}
	if ( v < 4 )
		EnableOption(TO_ANSISRM);	//  12 SRM Set Send/Receive mode (Local echo off)
	if ( v < 5 ) {					
		m_RecvCrLf = IsOptValue(426, 2);	// TO_RLRECVCR(426) to m_RecvCrLf
		m_SendCrLf = IsOptValue(418, 2);	// TO_RLECHOCR(418) to m_SendCrLf
		memcpy(m_OptTab, m_AnsiOpt, sizeof(m_AnsiOpt));
		for ( n = 0 ; n < 406 ; n++ ) DisableOption(1000 + n);
		for ( ; n < 416 ; n++ ) DisableOption(n);
		for ( ; n < 418 ; n++ ) DisableOption(1000 + n);
		for ( ; n < 428 ; n++ ) DisableOption(n);
		for ( ; n < 430 ; n++ ) DisableOption(1000 + n);
		for ( ; n < 435 ; n++ ) DisableOption(n);
		for ( ; n < 436 ; n++ ) DisableOption(1000 + n);
		for ( ; n < 437 ; n++ ) DisableOption(n);
		for ( ; n < 444 ; n++ ) DisableOption(1000 + n);
		for ( ; n < 445 ; n++ ) DisableOption(n);
		for ( ; n < 449 ; n++ ) DisableOption(1000 + n);
	}
	if ( v < 6 )
		EnableOption(TO_DRCSMMv1);	// ?8800 Unicode 16 Maping
	if ( v < 7 )
		EnableOption(TO_DECARM);	// ? 8 Autorepeat mode
	if ( v < 8 )
		EnableOption(TO_XTPRICOL);	// ?1070 Regis/Sixel Private Color Map
	
	DisableOption(TO_IMECTRL);
	memcpy(m_DefAnsiOpt, m_AnsiOpt, sizeof(m_DefAnsiOpt));

	if ( stra.GetSize() > 38 )
		m_TitleMode = stra.GetVal(38);

	if ( stra.GetSize() > 39 )
		m_SendCharSet[4] = stra.GetAt(39);

	if ( stra.GetSize() > 40 )
		m_ClipFlag = stra.GetVal(40);

	if ( stra.GetSize() > 41 )
		m_DefFontHw = stra.GetVal(41);

	if ( stra.GetSize() > 42 )
		m_ShellExec.GetString(stra.GetAt(42), _T('|'));

	if ( stra.GetSize() > 43 )
		m_RecvCrLf = stra.GetVal(43);

	if ( stra.GetSize() > 44 )
		m_SendCrLf = stra.GetVal(44);

	if ( stra.GetSize() > 45 )
		stra.GetBin(45, m_OptTab, sizeof(m_OptTab));

	if ( stra.GetSize() > 46 )
		stra.GetBin(46, m_DefModKey, sizeof(m_DefModKey));
	memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));

	if ( stra.GetSize() > 47 )
		m_ModKeyList[1] = stra.GetAt(47);
	if ( stra.GetSize() > 48 )
		m_ModKeyList[2] = stra.GetAt(48);
	if ( stra.GetSize() > 49 )
		m_ModKeyList[3] = stra.GetAt(49);
	if ( stra.GetSize() > 50 )
		m_ModKeyList[4] = stra.GetAt(50);
	if ( stra.GetSize() > 51 )
		m_ModKeyList[5] = stra.GetAt(51);
	InitModKeyTab();

	if ( stra.GetSize() > 52 )
		m_ScrnOffset.left = stra.GetVal(52);
	if ( stra.GetSize() > 53 )
		m_ScrnOffset.right = stra.GetVal(53);
	if ( stra.GetSize() > 54 )
		m_ScrnOffset.top = stra.GetVal(54);
	if ( stra.GetSize() > 55 )
		m_ScrnOffset.bottom = stra.GetVal(55);

	if ( stra.GetSize() > 56 )
		m_TimeFormat = stra.GetAt(56);

	if ( stra.GetSize() > 57 ) {
		ext.GetString(stra.GetAt(57), _T(';'));
		for ( n = 0 ; n < 16 && n < ext.GetSize() ; n++ )
			m_DefFontName[n] = ext[n];
	}

	if ( stra.GetSize() > 58 )
		m_TraceLogFile = stra.GetAt(58);
	if ( stra.GetSize() > 59 )
		m_TraceMaxCount = stra.GetVal(59);

	RESET();
}

static const ScriptCmdsDefs DocScrn[] = {
	{	"Cursol",		1	},
	{	"Size",			2	},
	{	"Style",		3	},
	{	"Color",		4	},
	{	"Mode",			10	},
	{	"ExtMode",		11	},
	{	NULL,			0	},
}, DocScrnCursol[] = {
	{	"x",			20	},
	{	"y",			21	},
	{	"Display",		22	},
	{	"Style",		23	},
	{	NULL,			0	},
}, DocScrnSize[] = {
	{	"x",			24	},
	{	"y",			25	},
	{	NULL,			0	},
}, DocScrnStyle[] = {
	{	"Color",		26	},
	{	"BackColor",	27	},
	{	"Attribute",	28	},
	{	"FontNumber",	29	},
	{	NULL,			0	},
};

void CTextRam::ScriptInit(int cmds, int shift, class CScriptValue &value)
{
	int n;

	value.m_DocCmds = cmds;

	for ( n = 0 ; DocScrn[n].name != NULL ; n++ )
		value[DocScrn[n].name].m_DocCmds = (DocScrn[n].cmds << shift) | cmds;

	for ( n = 0 ; DocScrnCursol[n].name != NULL ; n++ )
		value["Cursol"][DocScrnCursol[n].name].m_DocCmds = (DocScrnCursol[n].cmds << shift) | cmds;

	for ( n = 0 ; DocScrnSize[n].name != NULL ; n++ )
		value["Size"][DocScrnSize[n].name].m_DocCmds = (DocScrnSize[n].cmds << shift) | cmds;

	for ( n = 0 ; DocScrnStyle[n].name != NULL ; n++ )
		value["Style"][DocScrnStyle[n].name].m_DocCmds = (DocScrnStyle[n].cmds << shift) | cmds;
}
void CTextRam::ScriptTable(const struct _ScriptCmdsDefs *defs, class CScriptValue &value, int mode)
{
	int n, i;

	if ( mode == DOC_MODE_SAVE ) {
		for ( n = 0 ; defs[n].name != NULL ; n++ ) {
			if ( (i = value.Find(defs[n].name)) >= 0 )
				ScriptValue(defs[n].cmds, value[i], mode);
		}
	} else if ( mode == DOC_MODE_IDENT ) {
		for ( n = 0 ; defs[n].name != NULL ; n++ )
			ScriptValue(defs[n].cmds, value[defs[n].name], mode);
	}
}
void CTextRam::ScriptValue(int cmds, class CScriptValue &value, int mode)
{
	int n;
//	CString str;

	switch(cmds & 0xFF) {
	case 0:					// Document.Screen
		ScriptTable(DocScrn, value, mode);
		break;

	case 1:					// Document.Screen.Cursol
		ScriptTable(DocScrnCursol, value, mode);
		break;
	case 2:					// Document.Screen.Size
		ScriptTable(DocScrnSize, value, mode);
		break;
	case 3:					// Document.Screen.Style
		ScriptTable(DocScrnStyle, value, mode);
		break;

	case 4:					// Document.Screen.Color
		if ( mode == DOC_MODE_CALL ) {			// Document.Screen.Color(n, r, g, b)
			if ( value.GetSize() >= 4 && (n = value[0]) >= 0 && n < 256 ) {
				m_ColTab[n] = RGB((int)value[n][1], (int)value[n][2], (int)value[n][3]);
				if ( n < 16 )
					m_DefColTab[n] = m_ColTab[n];
				value = (int)0;
			} else
				value = (int)1;
		} else if ( mode == DOC_MODE_IDENT ) {
			value.RemoveAll();
			for ( n = 0 ; n < 256 ; n++ ) {
				value[n][0] = GetRValue(m_ColTab[n]);
				value[n][1] = GetGValue(m_ColTab[n]);
				value[n][2] = GetBValue(m_ColTab[n]);
			}
		} else if ( mode == DOC_MODE_SAVE ) {
			for ( n = 0 ; n < 256 && n < value.GetSize() ; n++ )
				m_ColTab[n] = RGB((int)value[n][0], (int)value[n][1], (int)value[n][2]);
			memcpy(m_DefColTab, m_ColTab, sizeof(m_DefColTab));
		}
		break;

	case 10:				// Document.Screen.Mode
		if ( mode == DOC_MODE_CALL ) {
			int opt = (int)value[0];
			if ( opt < 0 ) opt = 0;
			else if ( opt > 99 ) opt = 99;
			opt += 200;		// 200-299
			if ( value.GetSize() < 1 )
				break;
			else if ( value.GetSize() < 2 )
				value = (int)(IsOptEnable(opt) ? 1 : 0);
			else
				SetOption(opt, ((int)value[1] != 0 ? TRUE : FALSE));
		}
		break;
	case 11:				// Document.Screen.ExtMode
		if ( mode == DOC_MODE_CALL ) {
			int opt = (int)value[0];
			if ( value.GetSize() < 1 )
				break;
			else if ( value.GetSize() < 2 ) {
				if ( opt < 1 )
					opt = 0;
				else if ( opt >= 1000 && opt < 1080 )
					opt -= 700;
				else if ( opt >= 2000 && opt < 2020 )
					opt -= 1620;		// 380-399
				else if ( opt >= 8400 && opt < 8512 )
					opt -= 8000;		// 400-511
				else if ( opt == 7727 )
					opt = TO_RLCKMESC;	// 7727  -  Application Escape mode を有効にする。				Application Escape mode を無効にする。  
				else if ( opt == 7786 )
					opt = TO_RLMSWAPE;	// 7786  -  マウスホイール - カーソルキー変換を有効にする。		マウスホイール - カーソルキー変換を無効にする。  
				else if ( opt > 199 )
					opt = 199;
				value = (int)(IsOptEnable(opt) ? 1 : 0);
			} else {
				int n;
				CParaIndex save;
				save = m_AnsiPara;
				m_AnsiPara.RemoveAll();
				m_AnsiPara.Add(opt);
				fc_Push(STAGE_CSI);
				fc_DECSRET((int)value[1] != 0 ? 'h' : 'l');
				m_AnsiPara = save;
			}
		}
		break;

	case 20:				// Document.Screen.Cursol.x
		value.SetInt(m_CurX, mode);
		if ( mode == DOC_MODE_SAVE ) {
			LOCATE(m_CurX, m_CurY);
			FLUSH();
		}
		break;
	case 21:				// Document.Screen.Cursol.y
		value.SetInt(m_CurY, mode);
		if ( mode == DOC_MODE_SAVE ) {
			LOCATE(m_CurX, m_CurY);
			FLUSH();
		}
		break;
	case 22:				// Document.Screen.Cursol.Display
		n = (m_DispCaret & FGCARET_ONOFF) != 0 ? 1 : 0;
		value.SetInt(n, mode);
		if ( mode == DOC_MODE_SAVE ) {
			if ( n != 0 )
				CURON();
			else
				CUROFF();
			FLUSH();
		}
		break;
	case 23:				// Document.Screen.Cursol.Style
		value.SetInt(m_TypeCaret, mode);
		if ( mode == DOC_MODE_SAVE )
			FLUSH();
		break;

	case 24:				// Document.Screen.Size.x
		if ( mode == DOC_MODE_IDENT )
			value.SetInt(m_Cols, mode);
		break;
	case 25:				// Document.Screen.Size.y
		if ( mode == DOC_MODE_IDENT )
			value.SetInt(m_Lines, mode);
		break;

	case 26:				// Document.Screen.Style.Color
		n = m_AttNow.fcol;
		value.SetInt(n, mode);
		m_AttNow.fcol = (BYTE)n;
		break;
	case 27:				// Document.Screen.Style.BackColor
		n = m_AttNow.bcol;
		value.SetInt(n, mode);
		m_AttNow.bcol = (BYTE)n;
		break;
	case 28:				// Document.Screen.Style.Attribute
		n = m_AttNow.attr;
		value.SetInt(n, mode);
		m_AttNow.attr = (DWORD)n;
		break;
	case 29:				// Document.Screen.Style.FontNumber
		n = m_AttNow.font;
		value.SetInt(n, mode);
		m_AttNow.font = (DWORD)n;
		break;
	}
}
void CTextRam::Serialize(int mode)
{
	COptObject::Serialize(mode);
	m_FontTab.Serialize(mode);
}
void CTextRam::Serialize(int mode, CBuffer &buf)
{
	COptObject::Serialize(mode, buf);
	m_FontTab.Serialize(mode, buf);
}
void CTextRam::Serialize(CArchive& ar)
{
	COptObject::Serialize(ar);
	m_FontTab.Serialize(ar);
}
const CTextRam & CTextRam::operator = (CTextRam &data)
{
	m_DefCols[0]  = data.m_DefCols[0];
	m_DefCols[1]  = data.m_DefCols[1];
	m_DefHisMax   = data.m_DefHisMax;
	m_DefFontSize = data.m_DefFontSize;
	m_DefFontHw   = data.m_DefFontHw;
	m_KanjiMode   = data.m_KanjiMode;
	m_BankGL      = data.m_BankGL;
	m_BankGR      = data.m_BankGR;
	m_DefAtt	  = data.m_DefAtt;
	memcpy(m_DefColTab,  data.m_DefColTab,  sizeof(m_DefColTab));
	memcpy(m_ColTab,  data.m_ColTab,  sizeof(m_ColTab));
	memcpy(m_DefAnsiOpt, data.m_DefAnsiOpt, sizeof(m_DefAnsiOpt));
	memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_OptTab, data.m_OptTab, sizeof(m_OptTab));
	memcpy(m_DefBankTab, data.m_DefBankTab, sizeof(m_DefBankTab));
	memcpy(m_BankTab, m_BankTab, sizeof(m_BankTab));
	m_SendCharSet[0] = data.m_SendCharSet[0];
	m_SendCharSet[1] = data.m_SendCharSet[1];
	m_SendCharSet[2] = data.m_SendCharSet[2];
	m_SendCharSet[3] = data.m_SendCharSet[3];
	m_SendCharSet[4] = data.m_SendCharSet[4];
	m_WheelSize    = data.m_WheelSize;
	m_HisFile      = data.m_HisFile;
	m_DropFileMode = data.m_DropFileMode;
	for ( int n = 0 ; n < 8 ; n++ )
		m_DropFileCmd[n]  = data.m_DropFileCmd[n];
	m_WordStr      = data.m_WordStr;
	memcpy(m_MouseMode, data.m_MouseMode, sizeof(m_MouseMode));
	fc_Init(m_KanjiMode);
	memcpy(m_MetaKeys,  data.m_MetaKeys,  sizeof(m_MetaKeys));
	m_ProcTab = data.m_ProcTab;
	m_TitleMode = data.m_TitleMode;
	m_VtLevel = data.m_VtLevel;
	m_TermId  = data.m_TermId;
	m_LangMenu = data.m_LangMenu;
	m_UnitId = data.m_UnitId;
	m_ClipFlag  = data.m_ClipFlag;
	m_ShellExec = data.m_ShellExec;
	memcpy(m_DefModKey, data.m_DefModKey, sizeof(m_DefModKey));
	memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
	for ( int n = 0 ; n < MODKEY_MAX ; n++ )
		m_ModKeyList[n] = data.m_ModKeyList[n];
	InitModKeyTab();
	m_ScrnOffset = data.m_ScrnOffset;

	return *this;
}

//////////////////////////////////////////////////////////////////////

void CTextRam::SetKanjiMode(int mode)
{
	m_KanjiMode = mode;
	fc_Init(mode);
}
int CTextRam::Write(LPBYTE lpBuf, int nBufLen, BOOL *sync)
{
	int n;

	if ( m_LineEditMode )
		LOADRAM();

	for ( n = 0 ; n < nBufLen && !m_UpdateFlag ; n++ ) {
		fc_Call(lpBuf[n]);
		if ( m_RetSync ) {
			m_RetSync = FALSE;
			*sync = TRUE;
			break;
		}
	}

	if ( m_UpdateFlag ) {
		m_UpdateFlag = FALSE;
		m_pDocument->SetActCount();
	}

	if ( m_LineEditMode || m_LineEditBuff.GetSize() > 0 ) {
		if ( IsOptEnable(TO_RLDSECHO) ) {
			SAVERAM();
			m_LineEditX = m_CurX;
			m_LineEditY = m_CurY;
			m_LineEditIndex = 0;
			m_LineEditMode = TRUE;
			LineEditEcho();
		} else {
			m_LineEditBuff.Clear();
			m_LineEditMode = FALSE;
		}
	}

	FLUSH();

	return n;
}
void CTextRam::LineEditCwd(int ex, int sy, CStringW &cwd)
{
	int n, x;
	CCharCell *vp;
	CBuffer str;
	LPCWSTR p;

	vp = GETVRAM(0, sy);
	for ( x = 0 ; x < ex ; x += n ) {
		if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
			for ( p = vp[x] ; *p != 0 ; p++ )
				str.PutWord(*p);
			n = 2;
		} else if ( !IS_ASCII(vp[x].m_Vram.mode) ) { //|| vp[x].ch == 0 ) {
			str.PutWord(' ');
			n = 1;
		} else {
			for ( p = vp[x] ; *p != 0 ; p++ )
				str.PutWord(*p);
			n = 1;
		}
	}
	str.PutWord(0);
	cwd = (LPWSTR)str.GetPtr();
}
void CTextRam::LineEditEcho()
{
	CBuffer tmp;
	CBuffer left, right;
	int cx, cy;
	BOOL cf, gf;

	left.Apend(m_LineEditBuff.GetPtr(), m_LineEditPos * sizeof(WCHAR));
	right.Apend(m_LineEditBuff.GetPos(m_LineEditPos * sizeof(WCHAR)), m_LineEditBuff.GetSize() - m_LineEditPos * sizeof(WCHAR));

	m_CurX = m_LineEditX;
	m_CurY = m_LineEditY;

	cf = IsOptEnable(TO_DECAWM) ? TRUE : FALSE;
	gf = IsOptEnable(TO_RLGNDW) ? TRUE : FALSE;

	EnableOption(TO_DECAWM);	// Auto Round Up
	EnableOption(TO_RLGNDW);	// No Do Warp

	tmp.Clear();
	m_IConv.StrToRemote(m_SendCharSet[m_KanjiMode], &left, &tmp);
	PUTSTR(tmp.GetPtr(), tmp.GetSize());
	cx = m_CurX; cy = m_CurY;

	tmp.Clear();
	m_IConv.StrToRemote(m_SendCharSet[m_KanjiMode], &right, &tmp);
	PUTSTR(tmp.GetPtr(), tmp.GetSize());
	ERABOX(m_CurX, m_CurY, m_Cols, m_CurY + 1);

	if ( m_LineEditIndex > 0 ) {
		m_LineEditY -= m_LineEditIndex;
		m_LineEditIndex = 0;
	}

	m_CurX = cx; m_CurY = cy;

	if ( !cf ) DisableOption(TO_DECAWM);
	if ( !gf ) DisableOption(TO_RLGNDW);
}
int CTextRam::LineEdit(CBuffer &buf)
{
	int n;
	int rt = FALSE;
	int ec = FALSE;
	int ds = FALSE;
	int sr = FALSE;
	int len;
	CBuffer tmp;
	CString dir;
	WCHAR *wp, *sp;
	CStringW wstr;

	if ( m_LineEditMode == FALSE ) {
		SAVERAM();
		m_LineEditMode = TRUE;
		m_LineEditPos = 0;
		m_LineEditX = m_CurX;
		m_LineEditY = m_CurY;
		m_LineEditHisPos = (-1);
		m_LineEditIndex = 0;
		m_LineEditMapsPos = (-1);
	}

	tmp = buf;
	len = tmp.GetSize() / sizeof(WCHAR);
	wp = (WCHAR *)tmp.GetPtr();
	buf.Clear();

	while ( len-- > 0 ) {
		switch(*wp) {
		case 'V'-'@':
			AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_EDIT_PASTE);
			break;

		case 'C'-'@':
			if ( m_pDocument != NULL && m_pDocument->m_pSock != NULL )
				m_pDocument->m_pSock->SendBreak(0);
			goto CLEARBUF;
		case 'Z'-'@':
			if ( m_pDocument != NULL && m_pDocument->m_pSock != NULL )
				m_pDocument->m_pSock->SendBreak(1);
			// goto CLEARBUF;
		case 'X'-'@':
		CLEARBUF:
			m_LineEditPos = 0;
			m_LineEditBuff.Clear();
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			rt = TRUE;
			break;

		case 'I'-'@':	// TAB
			if ( m_LineEditMapsPos < 0 ) {
				for ( m_LineEditMapsTop = m_LineEditPos ; m_LineEditMapsTop > 0 ; m_LineEditMapsTop-- ) {
					wp = (WCHAR *)m_LineEditBuff.GetPos((m_LineEditMapsTop - 1) * sizeof(WCHAR));
					if ( *wp == ' ' )
						break;
				}

				m_LineEditMapsDir.Empty();
				m_LineEditMapsStr.Empty();
				for ( n = m_LineEditMapsTop ; n < m_LineEditPos ; n++ ) {
					wp = (WCHAR *)m_LineEditBuff.GetPos(n * sizeof(WCHAR));
					m_LineEditMapsStr += *wp;
				}

				if ( m_pDocument != NULL && m_pDocument->m_pSock != NULL && m_pDocument->m_pSock->m_Type == ESCT_PIPE ) {
					if ( m_LineEditMapsTop == 0 ) {
						m_LineEditMapsIndex = 0;
						if ( (m_LineEditMapsInit & 001) == 0 ) {
							m_LineEditMapsInit |= 001;
							((CPipeSock *)(m_pDocument->m_pSock))->GetPathMaps(m_LineEditMapsTab[m_LineEditMapsIndex]);
						}
					} else {
						m_LineEditMapsIndex = 1;
						if ( m_LineEditMapsStr.GetAt(0) == 'L\\' || m_LineEditMapsStr.Find(L':') >= 0 ) {
							ds = TRUE;
							if ( (n = m_LineEditMapsStr.ReverseFind(L'\\')) >= 0 )
								dir = m_LineEditMapsStr.Left(n);
							else if ( (n = m_LineEditMapsStr.ReverseFind(L':')) >= 0 )
								dir = m_LineEditMapsStr.Left(n + 1);
							else
								dir = m_LineEditMapsStr;
						} else {
							LineEditCwd(m_LineEditX - 1, m_LineEditY, wstr);
							dir = wstr;
							if ( (n = m_LineEditMapsStr.ReverseFind(L'\\')) >= 0 ) {
								SetCurrentDirectory(dir);
								m_LineEditMapsDir = m_LineEditMapsStr.Left(n + 1);
								dir = m_LineEditMapsStr.Left(n);
								m_LineEditMapsStr = m_LineEditMapsStr.Mid(n + 1);
							}
						}
						((CPipeSock *)(m_pDocument->m_pSock))->GetDirMaps(m_LineEditMapsTab[m_LineEditMapsIndex], dir, ds);
					}
				} else
					m_LineEditMapsIndex = 2;

				if ( (m_LineEditMapsPos = m_LineEditMapsTab[m_LineEditMapsIndex].Find(m_LineEditMapsStr)) < 0 )
					break;
				wstr = m_LineEditMapsDir + m_LineEditMapsTab[m_LineEditMapsIndex].GetAt(m_LineEditMapsPos);

			} else if ( (m_LineEditMapsPos = m_LineEditMapsTab[m_LineEditMapsIndex].Next(m_LineEditMapsStr, m_LineEditMapsPos + 1)) >= 0 ) {
				wstr = m_LineEditMapsDir + m_LineEditMapsTab[m_LineEditMapsIndex].GetAt(m_LineEditMapsPos);
			} else {
				wstr = m_LineEditMapsDir + m_LineEditMapsStr;
				m_LineEditMapsPos = (-1);
				m_LineEditMapsIndex = 2;
			}

			if ( m_LineEditMapsIndex < 2 && wstr.Find(L' ') >= 0 ) {
				wstr.Insert(0, L'"');
				wstr += L'"';
			}

			tmp.Clear();
			tmp.Apend(m_LineEditBuff.GetPtr(), m_LineEditMapsTop * sizeof(WCHAR));
			tmp.Apend((LPBYTE)(LPCWSTR)wstr, wstr.GetLength() * sizeof(WCHAR));
			tmp.Apend(m_LineEditBuff.GetPos(m_LineEditPos * sizeof(WCHAR)), m_LineEditBuff.GetSize() - m_LineEditPos * sizeof(WCHAR));
			m_LineEditBuff = tmp;
			m_LineEditPos = m_LineEditMapsTop + wstr.GetLength();
			ec = TRUE;
			break;

		case 'H'-'@':	// BS
			if ( m_LineEditPos <= 0 )
				break;
			m_LineEditPos--;
		case 0x7F:	// DEL
			n = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			if ( m_LineEditPos >= n )
				break;
			sp = (WCHAR *)m_LineEditBuff.GetPos(m_LineEditPos * sizeof(WCHAR));
			memcpy(sp, sp + 1, (n - m_LineEditPos) * sizeof(WCHAR));
			m_LineEditBuff.ConsumeEnd(sizeof(WCHAR));
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			if ( m_LineEditBuff.GetSize() <= 0 )
				rt = TRUE;
			break;

		case 0x1C:	// LEFT
			if ( m_LineEditPos <= 0 )
				break;
			m_LineEditPos--;
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;
		case 0x1D:	// RIGHT
			if ( m_LineEditPos >= (int)(m_LineEditBuff.GetSize() / sizeof(WCHAR)) )
				break;
			m_LineEditPos++;
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;

		case 0x1E:	// UP
			if ( (m_LineEditHisPos + 1) >= m_LineEditHis.GetSize() )
				break;
			if ( ++m_LineEditHisPos <= 0 )
				m_LineEditNow.PutBuf(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
			m_LineEditBuff.Clear();
			m_LineEditBuff.Apend(m_LineEditHis.GetAt(m_LineEditHisPos).GetPtr(), m_LineEditHis.GetAt(m_LineEditHisPos).GetSize());
			m_LineEditPos = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;
		case 0x1F:	// DOWN
			if ( m_LineEditHisPos < 0 )
				break;
			if ( --m_LineEditHisPos < 0 ) {
				m_LineEditBuff.Clear();
				m_LineEditBuff.Apend(m_LineEditNow.GetPtr(), m_LineEditNow.GetSize());
				m_LineEditPos = m_LineEditBuff.GetSize() / sizeof(WCHAR);
				ec = TRUE;
				break;
			}
			m_LineEditBuff.Clear();
			m_LineEditBuff.Apend(m_LineEditHis.GetAt(m_LineEditHisPos).GetPtr(), m_LineEditHis.GetAt(m_LineEditHisPos).GetSize());
			m_LineEditPos = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			break;

		case 0x0D:
			if ( m_LineEditBuff.GetSize() > 0 ) {
				m_LineEditNow.PutBuf(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
				if ( m_LineEditHisPos >= 0 )
					m_LineEditHis.RemoveAt(m_LineEditHisPos);
				else {
					for ( n = 0 ; n < (int)m_LineEditHis.GetSize() ; n++ ) {
						if ( m_LineEditNow.GetSize() == m_LineEditHis.GetAt(n).GetSize() &&
							memcmp(m_LineEditNow.GetPtr(), m_LineEditHis.GetAt(n).GetPtr(), m_LineEditNow.GetSize()) == 0 ) {
							m_LineEditHis.RemoveAt(n);
							break;
						}
					}
				}
				m_LineEditHis.InsertAt(0, m_LineEditNow);
				if ( (n = (int)m_LineEditHis.GetSize()) > 200 )
					m_LineEditHis.RemoveAt(n - 1);
				m_LineEditMapsTab[2].AddWStrBuf(m_LineEditNow.GetPtr(), m_LineEditNow.GetSize());
			}
			m_LineEditBuff.PutWord(0x0D);
			buf.Apend(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
			m_LineEditBuff.Clear();
			m_LineEditPos = 0;
			m_LineEditHisPos = (-1);
			m_LineEditMapsPos = (-1);
			rt = TRUE;
			break;

		default:
			n = m_LineEditBuff.GetSize() / sizeof(WCHAR);
			if ( m_LineEditPos >= n )
				m_LineEditBuff.PutWord(*wp);
			else {
				m_LineEditBuff.PutWord(0x00);	// Dummy
				sp = (WCHAR *)m_LineEditBuff.GetPtr();
				for ( ; m_LineEditPos <= n ; n-- )
					sp[n] = sp[n - 1];
				sp[m_LineEditPos] = *wp;
			}
			m_LineEditPos++;
			m_LineEditMapsPos = (-1);
			ec = TRUE;
			if ( (*wp & 0xFF00) == 0 && *wp < L' ' )
				sr = TRUE;
			break;
		}
		wp++;
	}

	if ( sr ) {
		buf.Apend(m_LineEditBuff.GetPtr(), m_LineEditBuff.GetSize());
		m_LineEditBuff.Clear();
		m_LineEditPos = 0;
		m_LineEditHisPos = (-1);
		m_LineEditMapsPos = (-1);
		rt = TRUE;
		ec = FALSE;
	}

	if ( ec )
		LineEditEcho();

	if ( rt ) {
		LOADRAM();
		m_LineEditMode = FALSE;
	}

	if ( ec || rt )
		FLUSH();

	return rt;
}
int CTextRam::IsWord(DWORD ch)
{
	if ( ch < 0x20 )
		return 0;
	else if ( ch >= 0x3040 && ch <= 0x309F )		// 3040 - 309F ひらがな
		return 2;
	else if ( ch >= 0x30A0 && ch <= 0x30FF )		// 30A0 - 30FF カタカナ
		return 3;
	else if ( UnicodeWidth(ch) == 2 && iswalnum(ch) )
		return 4;
	else if ( iswalnum(ch) || m_WordStr.Find(ch) >= 0 )
		return 1;
	return 0;
}
int CTextRam::GetPos(int x, int y)
{
	CCharCell *vp;

	if ( x < 0 ) {
		if ( --y < (0 - m_HisLen + m_Lines) )
			return 0;
		x += m_Cols;
	} else if ( x >= m_Cols ) {
		if ( ++y >= m_Lines )
			return 0;
		x -= m_Cols;
	}
	vp = GETVRAM(x, y);
	if ( vp->IsEmpty() )
		return 0;
	else if ( IS_2BYTE(vp->m_Vram.mode) ) {
		if ( x <= 0 )
			return 0;
		vp--;
	}
	return *((LPCWSTR)(*vp));
}
BOOL CTextRam::IncPos(int &x, int &y)
{
	if ( ++x >= m_Cols ) {
		if ( ++y >= m_Lines ) {
			x = m_Cols - 1;
			y = m_Lines - 1;
			return TRUE;
		}
		x = 0;
	}
	return FALSE;
}
BOOL CTextRam::DecPos(int &x, int &y)
{
	if ( --x < 0 ) {
		if ( --y < (0 - m_HisLen + m_Lines) ) {
			x = 0;
			y = (0 - m_HisLen + m_Lines);
			return TRUE;
		}
		x = m_Cols - 1;
	}
	return FALSE;
}
void CTextRam::EditWordPos(int *sps, int *eps)
{
	int n, c;
	int tx, ty;
	int sx, sy;
	int ex, ey;
	CStringW tmp;

	SetCalcPos(*sps, &sx, &sy);

	if ( sx > 0 && IS_2BYTE(GETVRAM(sx, sy)->m_Vram.mode) )
		sx--;

	if ( IsOptEnable(TO_RLGAWL) != 0 ) {
		for ( tx = 0, ty = sy ; tx <= sx ; tx++ ) {
			tmp.Empty();
			for ( n = 0 ; n < 10 ; n++ )
				tmp += (WCHAR)GetPos(tx + n, ty);

			if ( m_ShellExec.Match((UniToTstr(tmp))) < 0 )
				continue;

			ex = tx;
			ey = ty;
			for ( n = 0 ; (c = GetPos(ex + 1, ey)) != 0 ; n++ ) {
				if ( !iswalnum(c) && wcschr(L"!\"#$%&'()=~|-^\\@[;:],./`{+*}<>?_", c) == NULL )
					break;
				IncPos(ex, ey);
			}
			if ( (tx + n) >= sx ) {
				sx = tx;
				sy = ty;
				goto SKIP;
			}
		}
	}

	ex = sx;
	ey = sy;

	switch(IsWord(GetPos(sx, sy))) {
	case 0:
	case 1:	// is alnum
		while ( IsWord(GetPos(sx - 1, sy)) == 1 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 1 )
			IncPos(ex, ey);
		break;
	case 2:	// ひらがな
		while ( IsWord(GetPos(sx - 1, sy)) == 2 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(sx - 1, sy)) == 4 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 2 )
			IncPos(ex, ey);
		break;
	case 3:	// カタカナ
		while ( IsWord(GetPos(sx - 1, sy)) == 3 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 3 )
			IncPos(ex, ey);
		break;
	case 4:	// その他
		while ( IsWord(GetPos(sx - 1, sy)) == 4 )
			DecPos(sx, sy);
		while ( IsWord(GetPos(ex + 1, ey)) == 4 )
			IncPos(ex, ey);
		while ( IsWord(GetPos(ex + 1, ey)) == 2 )
			IncPos(ex, ey);
		break;
	}

SKIP:
	if ( IS_2BYTE(GETVRAM(sx, sy)->m_Vram.mode) )
		DecPos(sx, sy);

	if ( IS_1BYTE(GETVRAM(ex, ey)->m_Vram.mode) )
		IncPos(ex, ey);

	*sps = GetCalcPos(sx, sy);
	*eps = GetCalcPos(ex, ey);
}
void CTextRam::EditCopy(int sps, int eps, BOOL rectflag, BOOL lineflag)
{
	int n, x, y, ch, sx, ex, tc;
	int x1, y1, x2, y2;
	HGLOBAL hClipData;
	WCHAR *pData;
	CCharCell *vp;
	CBuffer tmp, str;
	LPCWSTR p;

	SetCalcPos(sps, &x1, &y1);
	SetCalcPos(eps, &x2, &y2);

	if ( rectflag && x1 > x2 ) {
		x = x1; x1 = x2; x2 = x;
	}

	for ( y = y1 ; y <= y2 ; y++ ) {
		vp = GETVRAM(0, y);
		
		if ( rectflag ) {
			sx = x1;
			ex = x2;
		} else if ( lineflag ) {
			sx = 0;
			ex = m_Cols - 1;
		} else {
			sx = (y == y1 ? x1 : 0);
			ex = (y == y2 ? x2 : (m_Cols - 1));
		}

		if ( vp->m_Vram.zoom != 0 ) {
			sx /= 2;
			ex /= 2;
		}

		while ( sx <= ex && vp[ex].IsEmpty() )
			ex--;

		if ( IS_2BYTE(vp[sx].m_Vram.mode) )
			sx++;

		tc = 0;
		str.Clear();
		for ( x = sx ; x <= ex ; x += n ) {
			if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				p = vp[x]; ch = *p;
				while ( *p != 0 )
					str.PutWord(*(p++));
				n = 2;
			} else if ( vp[x].IsEmpty() ) {
				ch = ' ';
				str.PutWord(ch);
				n = 1;
			} else {
				p = vp[x]; ch = *p;
				while ( *p != 0 )
					str.PutWord(*(p++));
				n = 1;
			}

			if ( n == 1 && ch == ' ' && IsOptEnable(TO_RLSPCTAB) != 0 ) {
				tc++;
				if ( (x % m_DefTab) == (m_DefTab - 1) ) {
//					if ( tc > (m_DefTab / 2) ) {
					if ( tc > 1 ) {
						str.ConsumeEnd(tc * sizeof(WORD));
						str.PutWord('\t');
					}
					tc = 0;
				}
			} else
				tc = 0;
		}

		if ( (y < y2 && x < m_Cols) || (y == y2 && lineflag) ) {
			str.PutWord(L'\r');
			str.PutWord(L'\n');
		}

		tmp.Apend(str.GetPtr(), str.GetSize());
	}

	tmp.PutWord(0);

	if ( IsOptEnable(TO_RLGAWL) != 0 ) {
		if ( m_ShellExec.Match(UniToTstr((LPCWSTR)tmp)) >= 0 )
			ShellExecuteW(AfxGetMainWnd()->GetSafeHwnd(), NULL, (LPCWSTR)tmp, NULL, NULL, SW_NORMAL);
	}

	if ( (hClipData = GlobalAlloc(GMEM_MOVEABLE, tmp.GetSize())) == NULL )
		return;

	if ( (pData = (WCHAR *)GlobalLock(hClipData)) == NULL )
		goto ENDOF;

	memcpy(pData, tmp.GetPtr(), tmp.GetSize());
	GlobalUnlock(hClipData);

	if ( !AfxGetMainWnd()->OpenClipboard() )
		goto ENDOF;

	if ( !EmptyClipboard() ) {
		CloseClipboard();
		goto ENDOF;
	}

	SetClipboardData(CF_UNICODETEXT, hClipData);
	CloseClipboard();
	goto ENDOF2;

ENDOF:
	GlobalFree(hClipData);
ENDOF2:
	return;
}
void CTextRam::GetVram(int staX, int endX, int staY, int endY, CBuffer *pBuf)
{
	int n;
	int x, y, sx, ex;
	CCharCell *vp;
	LPCWSTR p;

	for ( y = staY ; y <= endY ; y++ ) {
		vp = GETVRAM(0, y);

		if ( vp->m_Vram.zoom != 0 ) {
			sx = staX / 2;
			ex = endX / 2;
		} else {
			sx = staX;
			ex = endX;
		}

		while ( sx <= ex && vp[ex].IsEmpty() )
			ex--;

		for ( x = sx ; x <= ex ; x += n ) {
			if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				for ( p = vp[x] ; *p != 0 ; p++ )
					pBuf->PutWord(*p);
				n = 2;
			} else if ( vp[x].IsEmpty() ) {
				pBuf->PutWord(' ');
				n = 1;
			} else {
				for ( p = vp[x] ; *p != 0 ; p++ )
					pBuf->PutWord(*p);
				n = 1;
			}
		}

		if ( y < endY && x < m_Cols ) {
			pBuf->PutWord(L'\r');
			pBuf->PutWord(L'\n');
		}
	}
}

#include "BorderLine.h"

void CTextRam::LineOut(CDC *pDC, CRect &box, COLORREF fc, COLORREF bc, int rv, struct DrawWork &prop, int len, LPCWSTR str, int *spc, class CRLoginView *pView)
{
	int n, c, o;
	LPCWSTR p = str;
	CPen cPen[5], *oPen;
	LOGBRUSH LogBrush;
	CPoint point[2];
	CRect era(box);
	static const DWORD PenExtTab[3][4]  = {	{ 3, 1, 3, 1 }, { 2, 1, 2, 1 },	{ 1, 1, 1, 1 } };

	cPen[0].CreatePen(PS_SOLID, 1, fc);

	if ( pView->m_CharHeight < BD_HALFSIZE )
		cPen[4].CreatePen(PS_SOLID, 1, RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2));

	oPen = pDC->SelectObject(&(cPen[0]));

	LogBrush.lbColor = fc;
	LogBrush.lbStyle = BS_SOLID;

	if ( prop.dmf ==  2 )
		box.bottom += pView->m_CharHeight;
	else if ( prop.dmf ==  3 )
		box.top -= pView->m_CharHeight;

	for ( n = 0 ; n < len ; n++, p++ ) {
		box.right = box.left + spc[n];
		era.right = era.left + spc[n];

		if ( pView->m_pBitmap == NULL || rv != FALSE )
			pDC->FillSolidRect(era, bc);

		if ( *p >= 0x2500 && *p <= 0x257F ) {
			const BYTE *tab = BorderTab[*p - 0x2500];
			CPoint center((box.left + box.right) / 2, (box.top + box.bottom) / 2);

			for ( c = 0 ; c < 4 ; c++ ) {
				if ( tab[c] == BD_NONE )
					continue;

				switch(tab[c]) {
				case BD_LINE1:
				case BD_LINE2:
				case BD_LINE3:
				case BD_LINE4:
				case BD_LINE5:
					pDC->SelectObject(&(cPen[0]));
					break;
				case BD_LINE1 | BD_DOT2:
				case BD_LINE2 | BD_DOT2:
					if ( cPen[1].m_hObject == NULL )
						cPen[1].CreatePen(PS_USERSTYLE, 1, &LogBrush, 4, PenExtTab[0]);
					pDC->SelectObject(&(cPen[1]));
					break;
				case BD_LINE1 | BD_DOT3:
				case BD_LINE2 | BD_DOT3:
					if ( cPen[2].m_hObject == NULL )
						cPen[2].CreatePen(PS_USERSTYLE, 1, &LogBrush, 4, PenExtTab[1]);
					pDC->SelectObject(&(cPen[2]));
					break;
				case BD_LINE1 | BD_DOT4:
				case BD_LINE2 | BD_DOT4:
					if ( cPen[3].m_hObject == NULL )
						cPen[3].CreatePen(PS_USERSTYLE, 1, &LogBrush, 4, PenExtTab[2]);
					pDC->SelectObject(&(cPen[3]));
					break;
				}

				point[0] = center;
				point[1] = center;

				switch(c) {
				case 0:		// Left
					point[0].x = box.left;
					point[1].x++;

					switch(tab[c] & 0x0F) {
					case BD_LINE1:
						pDC->Polyline(point, 2);
						break;
					case BD_LINE2:
						pDC->Polyline(point, 2);
						if ( pView->m_CharHeight < BD_HALFSIZE )
							pDC->SelectObject(&(cPen[4]));
					case BD_LINE3:
						pDC->MoveTo(point[0].x, point[0].y - BD_SIZE);
						o = (tab[2] == BD_LINE3 ? -BD_SIZE : (tab[2] == BD_NONE && tab[3] >= BD_LINE2 ? BD_SIZE : (tab[2] == BD_LINE1 ? -1 : 0)));
						pDC->LineTo(point[1].x + o, point[1].y - BD_SIZE);

						pDC->MoveTo(point[0].x, point[0].y + BD_SIZE);
						o = (tab[3] == BD_LINE3 ? -BD_SIZE : (tab[3] == BD_NONE && tab[2] >= BD_LINE2 ? BD_SIZE : (tab[3] == BD_LINE1 ? -1 : 0)));
						pDC->LineTo(point[1].x + o, point[1].y + BD_SIZE);
						break;
					case BD_LINE4:	// ╮
						pDC->MoveTo(box.left, center.y);
						pDC->LineTo(center.x - BD_CORNER, center.y);
						pDC->LineTo(center.x, center.y + BD_CORNER);
						pDC->LineTo(center.x, box.bottom);
						break;
					case BD_LINE5:	// ╱
						pDC->MoveTo(box.right - 1, box.top);
						pDC->LineTo(box.left, box.bottom - 1);
						break;
					}
					break;

				case 1:		// Right
					point[1].x = box.right;

					switch(tab[c] & 0x0F) {
					case BD_LINE1:
						pDC->Polyline(point, 2);
						break;
					case BD_LINE2:
						pDC->Polyline(point, 2);
						if ( pView->m_CharHeight < BD_HALFSIZE )
							pDC->SelectObject(&(cPen[4]));
					case BD_LINE3:
						o = (tab[2] == BD_LINE3 ? BD_SIZE : (tab[2] == BD_NONE && tab[3] >= BD_LINE2 ? -BD_SIZE : (tab[2] == BD_LINE1 ? 1 : 0)));
						pDC->MoveTo(point[0].x + o, point[0].y - BD_SIZE);
						pDC->LineTo(point[1].x, point[1].y - BD_SIZE);

						o = (tab[3] == BD_LINE3 ? BD_SIZE : (tab[3] == BD_NONE && tab[2] >= BD_LINE2 ? -BD_SIZE : (tab[3] == BD_LINE1 ? 1 : 0)));
						pDC->MoveTo(point[0].x + o, point[0].y + BD_SIZE);
						pDC->LineTo(point[1].x, point[1].y + BD_SIZE);
						break;
					case BD_LINE4:	// ╰
						pDC->MoveTo(center.x, box.top);
						pDC->LineTo(center.x, center.y - BD_CORNER);
						pDC->LineTo(center.x + BD_CORNER, center.y);
						pDC->LineTo(box.right, center.y);
						break;
					case BD_LINE5:	// ╱
						pDC->MoveTo(box.right - 1, box.top);
						pDC->LineTo(box.left, box.bottom - 1);
						break;
					}
					break;

				case 2:		// Up
					point[0].y = box.top;
					point[1].y++;

					switch(tab[c] & 0x0F) {
					case BD_LINE1:
						pDC->Polyline(point, 2);
						break;
					case BD_LINE2:
						pDC->Polyline(point, 2);
						if ( pView->m_CharHeight < BD_HALFSIZE )
							pDC->SelectObject(&(cPen[4]));
					case BD_LINE3:
						pDC->MoveTo(point[0].x - BD_SIZE, point[0].y);
						o = (tab[0] >= BD_LINE2 ? -BD_SIZE : (tab[0] == BD_NONE && tab[1] >= BD_LINE2 ? BD_SIZE : (tab[0] == BD_LINE1 ? -1 : 0)));
						pDC->LineTo(point[1].x - BD_SIZE, point[1].y + o);

						pDC->MoveTo(point[0].x + BD_SIZE, point[0].y);
						o = (tab[1] >= BD_LINE2 ? -BD_SIZE : (tab[1] == BD_NONE && tab[0] >= BD_LINE2 ? BD_SIZE : (tab[1] == BD_LINE1 ? -1 : 0)));
						pDC->LineTo(point[1].x + BD_SIZE, point[1].y + o);
						break;
					case BD_LINE4:	// ╯
						pDC->MoveTo(box.left, center.y);
						pDC->LineTo(center.x - BD_CORNER, center.y);
						pDC->LineTo(center.x, center.y - BD_CORNER);
						pDC->LineTo(center.x, box.top - 1);
						break;
					case BD_LINE5:	// ╲
						pDC->MoveTo(box.left, box.top);
						pDC->LineTo(box.right - 1, box.bottom - 1);
						break;
					}
					break;

				case 3:		// Down
					point[1].y = box.bottom;

					switch(tab[c] & 0x0F) {
					case BD_LINE1:
						pDC->Polyline(point, 2);
						break;
					case BD_LINE2:
						pDC->Polyline(point, 2);
						if ( pView->m_CharHeight < BD_HALFSIZE )
							pDC->SelectObject(&(cPen[4]));
					case BD_LINE3:
						o = (tab[0] >= BD_LINE2 ? BD_SIZE : (tab[0] == BD_NONE && tab[1] >= BD_LINE2 ? -BD_SIZE : (tab[0] == BD_LINE1 ? 1 : 0)));
						pDC->MoveTo(point[0].x - BD_SIZE, point[0].y + o);
						pDC->LineTo(point[1].x - BD_SIZE, point[1].y);

						o = (tab[1] >= BD_LINE2 ? BD_SIZE : (tab[1] == BD_NONE && tab[0] >= BD_LINE2 ? -BD_SIZE : (tab[1] == BD_LINE1 ? 1 : 0)));
						pDC->MoveTo(point[0].x + BD_SIZE, point[0].y + o);
						pDC->LineTo(point[1].x + BD_SIZE, point[1].y);
						break;
					case BD_LINE4:	// ╭
						pDC->MoveTo(box.right, center.y);
						pDC->LineTo(center.x + BD_CORNER, center.y);
						pDC->LineTo(center.x, center.y + BD_CORNER);
						pDC->LineTo(center.x, box.bottom);
						break;
					case BD_LINE5:	// ╲
						pDC->MoveTo(box.left, box.top);
						pDC->LineTo(box.right - 1, box.bottom - 1);
						break;
					}
					break;
				}
			}
		}

		box.left += spc[n];
		era.left += spc[n];
	}

	pDC->SelectObject(oPen);
}
void CTextRam::StrOut(CDC *pDC, CDC *pWdc, LPCRECT pRect, struct DrawWork &prop, int len, LPCWSTR str, int *spc, class CRLoginView *pView)
{
	int n, c, x, y, o;
	int at = prop.att;
	int rv = FALSE;
	int wd = (prop.dmf == 0 ? 1 : 2);
	int hd = (prop.dmf <= 1 ? 1 : 2);
	COLORREF fc, bc, tc;
	LPCWSTR p = str;
	CFontChacheNode *pFontCache;
	CFontNode *pFontNode = (prop.mod < 0 ? NULL : &(m_FontTab[prop.mod]));
	CBitmap *pOld;
	CRect box(*pRect);
	LPCTSTR DefFontName;

	if ( (at & ATT_BOLD) != 0 && !IsOptEnable(TO_RLBOLD) && prop.fcn < 16 )
		prop.fcn ^= 0x08;

	fc = m_ColTab[prop.fcn];
	bc = m_ColTab[prop.bcn];

	if ( (at & ATT_REVS) != 0 ) {
		tc = fc;
		fc = bc;
		bc = tc;
		rv = TRUE;
	}

	if ( (at & (ATT_SBLINK | ATT_BLINK)) != 0 ) {
		if ( (pView->m_BlinkFlag & 4) == 0 ) {
			pView->SetTimer(1026, 250, NULL);
			pView->m_BlinkFlag = 4;
		} else if ( ((at & ATT_BLINK) != 0 && (pView->m_BlinkFlag & 1) != 0) ||
				    ((at & ATT_BLINK) == 0 && (pView->m_BlinkFlag & 2) != 0) ) {
			tc = fc;
			fc = bc;
			bc = tc;
			rv = TRUE;
		}
	}

	if ( (at & ATT_SECRET) != 0 )
		fc = bc;

	if ( (at & ATT_CLIP) != 0 || (pView->m_ActiveFlag && (at & ATT_MARK) != 0) ) {
		tc = fc;
		fc = bc;
		bc = tc;
		if ( pView->m_pBitmap != NULL )
			pDC->InvertRect(pRect);
		if ( rv )
			bc = RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2);
		rv = TRUE;
	}

	if ( (at & ATT_HALF) != 0 )
		fc = RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2);

	if ( pFontNode == NULL ) {
		if ( prop.idx != (-1) ) {
			CGrapWnd *pWnd;
			if ( (pWnd = GetGrapWnd(prop.idx)) != NULL )
				pWnd->DrawBlock(pDC, pRect, bc, prop.stx, prop.sty, prop.edx, prop.sty + 1, pView);
			else
				pDC->FillSolidRect(pRect, bc);
			if ( rv )
				pDC->InvertRect(pRect);
		} else if ( pView->m_pBitmap == NULL )
			pDC->FillSolidRect(pRect, bc);

	} else if ( (at & ATT_BORDER) != 0 ) {
		LineOut(pDC, box, fc, bc, rv, prop, len, str, spc, pView);

	} else {
		x = pView->m_CharWidth  * wd;
		y = pView->m_CharHeight * hd;
		n = (at & ATT_ITALIC) != 0 ? 2 : 0;

		DefFontName = (m_DefFontName[prop.fnm].IsEmpty() ? m_DefFontName[0] : m_DefFontName[prop.fnm]);

		pFontCache = pFontNode->GetFont(x, y, n, prop.fnm, DefFontName);
		pDC->SelectObject(pFontCache != NULL ? pFontCache->m_pFont : CFont::FromHandle((HFONT)GetStockObject(SYSTEM_FONT)));

		pDC->SetTextColor(fc);
		pDC->SetBkColor(bc);

		if ( prop.csz == 1 && pFontCache != NULL && prop.mod == SET_UNICODE && !IsOptEnable(TO_RLUNIAHF) ) {
			CSize sz = pDC->GetTextExtent(str, len);
			if ( sz.cx > (pRect->right - pRect->left) ) {
				if ( (pFontCache = pFontNode->GetFont(x / 2, y, n, prop.fnm, DefFontName)) != NULL )
					pDC->SelectObject(pFontCache->m_pFont);
			}
		}

		if ( pFontNode->SetFontImage(pView->m_CharWidth  * wd, pView->m_CharHeight * hd) ) {
			if ( pWdc->GetSafeHdc() == NULL )
				pWdc->CreateCompatibleDC(pDC);

			pOld = pWdc->SelectObject(&(pFontNode->m_FontMap));

			x = pRect->left;
			y = pRect->bottom - pRect->top;
			o = (prop.dmf == 3 ? pView->m_CharHeight : 0);

			for ( n = 0 ; n < len ; n++, p++ ) {
				if ( (c = *p - 0x20) >= 0 && c < 96 && (pFontNode->m_UserFontDef[c / 8] & (1 << (c % 8))) != 0 )
					pDC->BitBlt(x, pRect->top, spc[n], y, pWdc, pFontNode->m_FontWidth * c, o, ((pView->m_pBitmap == NULL || rv != FALSE) ? SRCCOPY: SRCPAINT));
				else {
					box.left = x;
					box.right = x + spc[n];
					::ExtTextOutW(pDC->m_hDC, x, pRect->top - o, ((pView->m_pBitmap == NULL || rv != FALSE) ? (ETO_OPAQUE | ETO_CLIPPED) : ETO_CLIPPED), box, p, 1, &(spc[n]));
				}
				x += spc[n];
			}

			pWdc->SelectObject(pOld);

		} else {
			x = pRect->left + pView->m_CharWidth * pFontNode->m_OffsetW / 100;
			y = pRect->top - pView->m_CharHeight * pFontNode->m_OffsetH / 100 - (prop.dmf == 3 ? pView->m_CharHeight : 0);

			::ExtTextOutW(pDC->m_hDC, x, y, ((pView->m_pBitmap == NULL || rv != FALSE) ? (ETO_OPAQUE | ETO_CLIPPED) : ETO_CLIPPED), pRect, str, len, spc);

			if ( (at & ATT_BOLD) != 0 && (IsOptEnable(TO_RLBOLD) || prop.fcn >= 16) ) {
				n = pDC->SetBkMode(TRANSPARENT);
				::ExtTextOutW(pDC->m_hDC, x + 1, y, ETO_CLIPPED, pRect, str, len, spc);
				pDC->SetBkMode(n);
			}
		}
	}

	if ( (at & (ATT_OVER | ATT_DOVER | ATT_LINE | ATT_UNDER | ATT_DUNDER | ATT_STRESS)) != 0 ) {
		CPen cPen(PS_SOLID, 1, fc);
		CPen *oPen = pDC->SelectObject(&cPen);
		POINT point[4];
		point[0].x = pRect->left;
		point[1].x = pRect->right;

		if ( (at & ATT_OVER) != 0 ) {
			point[0].y = pRect->top;
			point[1].y = pRect->top;
			pDC->Polyline(point, 2);
		} else if ( (at & ATT_DOVER) != 0 ) {
			point[0].y = pRect->top;
			point[1].y = pRect->top;
			pDC->Polyline(point, 2);
			point[2].x = ((at & ATT_LDLINE) != 0 ? (pRect->left + 2) : pRect->left);
			point[2].y = pRect->top + 2;
			point[3].x = ((at & ATT_RDLINE) != 0 ? (pRect->right - 3) : pRect->right);
			point[3].y = pRect->top + 2;
			pDC->Polyline(&(point[2]), 2);
		}

		if ( (at & ATT_LINE) != 0 ) {
			point[0].y = (pRect->top + pRect->bottom) / 2;
			point[1].y = (pRect->top + pRect->bottom) / 2;
			pDC->Polyline(point, 2);
		} else if ( (at & ATT_STRESS) != 0 ) {
			point[0].y = (pRect->top + pRect->bottom) / 2 - 1;
			point[1].y = (pRect->top + pRect->bottom) / 2 - 1;
			pDC->Polyline(point, 2);
			point[0].y = (pRect->top + pRect->bottom) / 2 + 1;
			point[1].y = (pRect->top + pRect->bottom) / 2 + 1;
			pDC->Polyline(point, 2);
		}

		if ( (at & ATT_UNDER) != 0 ) {
			point[0].y = pRect->bottom - 2;
			point[1].y = pRect->bottom - 2;
			pDC->Polyline(point, 2);
		} else if ( (at & ATT_DUNDER) != 0 ) {
			point[0].y = pRect->bottom - 1;
			point[1].y = pRect->bottom - 1;
			pDC->Polyline(point, 2);
			point[2].x = ((at & ATT_LDLINE) != 0 ? (pRect->left + 2) : pRect->left);
			point[2].y = pRect->bottom - 3;
			point[3].x = ((at & ATT_RDLINE) != 0 ? (pRect->right - 3) : pRect->right);
			point[3].y = pRect->bottom - 3;
			pDC->Polyline(&(point[2]), 2);
		}

		pDC->SelectObject(oPen);
	}

	if ( (at & (ATT_FRAME | ATT_CIRCLE | ATT_RSLINE | ATT_RDLINE | ATT_LSLINE | ATT_LDLINE)) != 0 ) {
		CPen cPen(PS_SOLID, 1, fc);
		CPen *oPen = pDC->SelectObject(&cPen);
		POINT point[9];

		y = pRect->left;
		for ( x = 0 ; x < len ; x++ ) {
			if ( spc[x] == 0 )
				continue;

			if ( (at & ATT_FRAME) != 0 ) {
				point[0].x = y; point[0].y = pRect->top + 1;
				point[1].x = y + spc[x] - 2; point[1].y = pRect->top + 1;
				point[2].x = y + spc[x] - 2; point[2].y = pRect->bottom - 1;
				point[3].x = y; point[3].y = pRect->bottom - 1;
				point[4].x = y; point[4].y = pRect->top + 1;
				pDC->Polyline(point, 5);
			} else if ( (at & ATT_CIRCLE) != 0 ) {
				n = spc[x] / 3;
				point[0].x = y + n; point[0].y = pRect->top + 1;
				point[1].x = y + spc[x] - 2 - n; point[1].y = pRect->top + 1;
				point[2].x = y + spc[x] - 2; point[2].y = pRect->top + 1 + n;
				point[3].x = y + spc[x] - 2; point[3].y = pRect->bottom - 1 - n;
				point[4].x = y + spc[x] - 2 - n; point[4].y = pRect->bottom - 1;
				point[5].x = y + n; point[5].y = pRect->bottom - 1;
				point[6].x = y; point[6].y = pRect->bottom - 1 - n;
				point[7].x = y; point[7].y = pRect->top + 1 + n;
				point[8].x = y + n; point[8].y = pRect->top + 1;
				pDC->Polyline(point, 9);
			}

			if ( (at & ATT_RSLINE) != 0 ) {
				point[0].x = y + spc[x] - 1; point[0].y = pRect->top;
				point[1].x = y + spc[x] - 1; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
			} else if ( (at & ATT_RDLINE) != 0 ) {
				point[0].x = y + spc[x] - 1; point[0].y = pRect->top;
				point[1].x = y + spc[x] - 1; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
				point[0].x = y + spc[x] - 3;
				point[0].y = ((at & ATT_DOVER) != 0 ? (pRect->top + 2) : pRect->top);
				point[1].x = y + spc[x] - 3;
				point[1].y = ((at & ATT_DUNDER) != 0 ? (pRect->bottom - 3) : (pRect->bottom - 1));
				pDC->Polyline(point, 2);
			}

			if ( (at & ATT_LSLINE) != 0 ) {
				point[0].x = y; point[0].y = pRect->top;
				point[1].x = y; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
			} else if ( (at & ATT_LDLINE) != 0 ) {
				point[0].x = y; point[0].y = pRect->top;
				point[1].x = y; point[1].y = pRect->bottom - 1;
				pDC->Polyline(point, 2);
				point[0].x = y + 2;
				point[0].y = ((at & ATT_DOVER) != 0 ? (pRect->top + 2) : pRect->top);
				point[1].x = y + 2;
				point[1].y = ((at & ATT_DUNDER) != 0 ? (pRect->bottom - 3) : (pRect->bottom - 1));
				pDC->Polyline(point, 2);
			}

			y += spc[x];
		}

		pDC->SelectObject(oPen);
	}

	if ( IsOptEnable(TO_DECSCNM) )
		pDC->InvertRect(pRect);

	if ( pView->m_VisualBellFlag )
		pDC->InvertRect(pRect);
}
void CTextRam::DrawVram(CDC *pDC, int x1, int y1, int x2, int y2, class CRLoginView *pView)
{
	int n, x, y, ex;
	struct DrawWork work, prop;
	int pos, spos, epos;
	int csx, cex, csy, cey;
	int stx, edx;
	CCharCell *vp, *tp;
	int len, sln;
	CStringW str;
	LPCWSTR p;
	WCHAR tmp[COLS_MAX * MAXCHARSIZE];
	int spc[COLS_MAX * MAXCHARSIZE];
	CFont *pFontOld = pDC->SelectObject(CFont::FromHandle((HFONT)GetStockObject(SYSTEM_FONT)));
	RECT rect;
	CDC wDc;

	if ( pView->m_ClipFlag ) {
		if ( pView->m_ClipStaPos <= pView->m_ClipEndPos ) {
			spos = pView->m_ClipStaPos;
			epos = pView->m_ClipEndPos;
		} else {
			spos = pView->m_ClipEndPos;
			epos = pView->m_ClipStaPos;
		}
		if ( pView->IsClipRectMode() ) {
			SetCalcPos(spos, &csx, &csy);
			SetCalcPos(epos, &cex, &cey);
			if ( csx > cex ) {
				x = csx; csx = cex; cex = x;
				spos = GetCalcPos(csx, csy);
				epos = GetCalcPos(cex, cey);
			}
		} else if ( pView->IsClipLineMode() ) {
			SetCalcPos(spos, &csx, &csy);
			spos = GetCalcPos(0, csy);
			SetCalcPos(epos, &cex, &cey);
			epos = GetCalcPos(m_Cols - 1, cey);
		}
	}

	for ( y = y1 ; y < y2 ; y++ ) {
		len = sln = 0;
		memset(&prop, 0, sizeof(prop));
		prop.idx = (-1);
		stx = edx = 0;
		rect.top    = pView->CalcGrapY(y);
		rect.bottom = pView->CalcGrapY(y + 1);
		tp = GETVRAM(0, y - pView->m_HisOfs + pView->m_HisMin);
		work.dmf = tp->m_Vram.zoom;

		x = (work.dmf != 0 ? (x1 / 2) : x1);
		ex = x2;

		if ( x > 0 && (tp[x].m_Vram.attr & ATT_RTOL) != 0 ) {
			while ( x > 0 && (tp[x - 1].m_Vram.attr & ATT_RTOL) != 0 )
				x--;
		}
		if ( ex < m_Cols && (tp[ex].m_Vram.attr & ATT_RTOL) != 0 ) {
			while ( (ex + 1) < m_Cols && (tp[ex + 1].m_Vram.attr & ATT_RTOL) != 0 )
				ex++;
		}

		while ( x < ex ) {
			if ( work.dmf != 0 && x >= (m_Cols / 2) )
				break;

			work.idx = (-1);
			work.stx = 0;
			work.edx = 0;
			work.sty = 0;

			if ( x < 0 ) {
				work.att = tp->m_Vram.attr & (ATT_REVS | ATT_CLIP | ATT_MARK | ATT_SBLINK | ATT_BLINK);
				work.fnm = tp->m_Vram.font;
				work.fcn = tp->m_Vram.fcol;
				work.bcn = tp->m_Vram.bcol;
				work.mod = (-1);
				work.csz = 1;
				str.Empty();
			} else if ( x >= m_Cols ) {
				vp = tp + (m_Cols - 1);
				work.att = vp->m_Vram.attr & (ATT_REVS | ATT_CLIP | ATT_MARK | ATT_SBLINK | ATT_BLINK);
				work.fnm = vp->m_Vram.font;
				work.fcn = vp->m_Vram.fcol;
				work.bcn = vp->m_Vram.bcol;
				work.mod = (-1);
				work.csz = 1;
				str.Empty();
			} else {
				vp = tp + x;
				if ( x > 0 && IS_2BYTE(vp[0].m_Vram.mode) && IS_1BYTE(vp[-1].m_Vram.mode) && vp[-1].Compare(vp[0]) == 0 ) {
					x--;
					vp--;
				}
				work.att = vp->m_Vram.attr;
				work.fnm = vp->m_Vram.font;
				work.fcn = vp->m_Vram.fcol;
				work.bcn = vp->m_Vram.bcol;
				work.mod = vp->m_Vram.bank & CODE_MASK;
				work.csz = 1;
				str = (LPCWSTR)*vp;

				if ( x < (m_Cols - 1) && IS_1BYTE(vp[0].m_Vram.mode) && IS_2BYTE(vp[1].m_Vram.mode) && vp[0].Compare(vp[1]) == 0 ) {
					work.csz = 2;
				} else if ( IS_IMAGE(vp->m_Vram.mode) ) {
					work.mod = (-1);
					work.idx = vp->m_Vram.pack.image.id;
					work.stx = vp->m_Vram.pack.image.ix;
					work.edx = vp->m_Vram.pack.image.ix + 1;
					work.sty = vp->m_Vram.pack.image.iy;
					str.Empty();
					stx = work.stx;
					edx = work.edx;
					if ( prop.idx == work.idx && prop.sty == work.sty && prop.edx == work.stx ) {
						work.stx = prop.stx;
						work.edx = prop.edx;
					}
				} else {
					if ( !IS_ASCII(vp->m_Vram.mode) || str.IsEmpty() || str[0] < _T(' ') ) {
						str.Empty();
						work.mod = (-1);
					}
				}
			}

#ifdef	USE_TEXTFRAME
			if ( (work.att & ATT_FRAME) != 0 ) {
				n = (ATT_LSLINE | ATT_RSLINE | ATT_OVER | ATT_UNDER);
				if ( x > 0 && (vp[-1].m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_LSLINE;
				if ( x < (m_Cols - work.csz) && (vp[work.csz].m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_RSLINE;
				if ( y > 0 && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin - 1)->m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_OVER;
				if ( y < (m_Lines - 1) && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin + 1)->m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_UNDER;
				work.att &= ~(ATT_FRAME | ATT_CIRCLE);
				work.att |= n;
				m_FrameCheck = TRUE;

			} else if ( (work.att & ATT_CIRCLE) != 0 ) {
				n = (ATT_LDLINE | ATT_RDLINE | ATT_DOVER | ATT_DUNDER);
				if ( x > 0 && (vp[-1].m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_LDLINE;
				if ( x < (m_Cols - work.csz) && (vp[work.csz].m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_RDLINE;
				if ( y > 0 && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin - 1)->m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_DOVER;
				if ( y < (m_Lines - 1) && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin + 1)->m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_DUNDER;
				work.att &= ~(ATT_FRAME | ATT_CIRCLE);
				work.att |= n;
				m_FrameCheck = TRUE;
			}
#endif

			if ( pView->m_ClipFlag ) {
				pos = GetCalcPos((work.dmf != 0 ? (x * 2) : x), y - pView->m_HisOfs + pView->m_HisMin);
				if ( spos <= pos && pos <= epos ) {
					if ( pView->IsClipRectMode() ) {
						if ( work.dmf != 0 ) {
							if ( (x * 2) >= csx && (x * 2) <= cex )
								work.att |= ATT_CLIP;
						} else if ( x >= csx && x <= cex )
							work.att |= ATT_CLIP;
					} else
						work.att |= ATT_CLIP;
				}
			}

			if ( memcmp(&prop, &work, sizeof(work)) != 0 ) {
				if ( len > 0 ) {
					rect.right = pView->CalcGrapX(x) * (work.dmf ? 2 : 1);
					StrOut(pDC, &wDc, &rect, prop, len, tmp, spc, pView);
				}
				memcpy(&prop, &work, sizeof(work));
				prop.stx = stx;
				prop.edx = edx;
				len = sln = 0;
				rect.left = pView->CalcGrapX(x) * (work.dmf ? 2 : 1);
			} else
				prop.edx = edx;

			if ( work.mod != (-1) ) {
				for ( p = str ; *p != L'\0' ; )
					tmp[len++] = *(p++);
			} else
				tmp[len++] = L'\0';

			while ( sln < (len - 1) )
				spc[sln++] = 0;

			spc[sln++] = (pView->CalcGrapX(x + work.csz) - pView->CalcGrapX(x)) * (work.dmf ? 2 : 1);

			x += work.csz;
		}
		if ( len > 0 ) {
			rect.right = pView->CalcGrapX(x) * (work.dmf ? 2 : 1);
			StrOut(pDC, &wDc, &rect, prop, len, tmp, spc, pView);
		}
	}

	pDC->SelectObject(pFontOld);

	if ( wDc.GetSafeHdc() != NULL )
		wDc.DeleteDC();
}

CWnd *CTextRam::GetAciveView()
{
	return m_pDocument->GetAciveView();
}
void CTextRam::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	CWnd *pView = GetAciveView();
	if ( pView != NULL )
		pView->PostMessage(message, wParam, lParam);
}
void CTextRam::GetScreenSize(int *x, int *y)
{
	CRLoginView *pView;

	if ( (pView = (CRLoginView *)GetAciveView()) != NULL ) {
		*x = pView->m_CharWidth  * m_Cols;
		*y = pView->m_CharHeight * m_Lines;
	} else {
		*x = 6 * m_Cols;
		*y = (6 * m_DefFontHw / 10) * m_Lines;
	}
}

BOOL CTextRam::IsOptEnable(int opt)
{
	if ( opt >= 1000 && opt <= 1511 ) {
		opt -= 1000;
		return (IS_ENABLE(m_OptTab, opt) ? TRUE : FALSE);
	} else if ( opt >= 0 && opt <= 511 )
		return (IS_ENABLE(m_AnsiOpt, opt) ? TRUE : FALSE);
	else
		return FALSE;
}
void CTextRam::EnableOption(int opt)
{
	if ( opt >= 1000 && opt <= 1511 ) {
		opt -= 1000;
		m_OptTab[opt / 32]  |= (1 << (opt % 32));
	} else if ( opt >= 0 && opt <= 511 )
		m_AnsiOpt[opt / 32] |= (1 << (opt % 32));
}
void CTextRam::DisableOption(int opt)
{
	if ( opt >= 1000 && opt <= 1511 ) {
		opt -= 1000;
		m_OptTab[opt / 32]  &= ~(1 << (opt % 32));
	} else if ( opt >= 0 && opt <= 511 )
		m_AnsiOpt[opt / 32] &= ~(1 << (opt % 32));
}
void CTextRam::ReversOption(int opt)
{
	if ( opt >= 1000 && opt <= 1511 ) {
		opt -= 1000;
		m_OptTab[opt / 32]  ^= (1 << (opt % 32));
	} else if ( opt >= 0 && opt <= 511 )
		m_AnsiOpt[opt / 32] ^= (1 << (opt % 32));
}
int CTextRam::IsOptValue(int opt, int len)
{
	int n;
	int v = 0;
	int b = 1;
	for ( n = 0 ; n < len ; n++ ) {
		if ( IsOptEnable(opt) )
			v |= b;
		opt++;
		b <<= 1;
	}
	return v;
}
void CTextRam::SetOptValue(int opt, int len, int value)
{
	int n;
	for ( n = 0 ; n < len ; n++ ) {
		SetOption(opt, ((value & 1) != 0 ? TRUE : FALSE));
		opt++;
		value >>= 1;
	}
}
void CTextRam::InitDefParam(BOOL bCheck, int modFlag)
{
	CDefParamDlg dlg;

	if ( bCheck ) {
		dlg.m_InitFlag = modFlag;
		if ( dlg.DoModal() != IDOK )
			return;
		modFlag = dlg.m_InitFlag;
	}

	if ( (modFlag & UMOD_ANSIOPT) != 0 )
		memcpy(m_DefAnsiOpt, m_AnsiOpt, sizeof(m_DefAnsiOpt));

	if ( (modFlag & UMOD_MODKEY) != 0 )
		memcpy(m_DefModKey, m_ModKey, sizeof (m_DefModKey));

	if ( (modFlag & UMOD_COLTAB) != 0 )
		memcpy(m_DefColTab, m_ColTab, sizeof(m_DefColTab));

	if ( (modFlag & UMOD_BANKTAB) != 0 )
		memcpy(m_DefBankTab, m_BankTab, sizeof(m_DefBankTab));

	if ( (modFlag & UMOD_DEFATT) != 0 )
		m_DefAtt = m_AttNow;
}
void CTextRam::InitModKeyTab()
{
	int n;
	CKeyNode node;

	for ( n = 0 ; n < 256 ; n++ )
		m_ModKeyTab[n] = (-1);

	for ( n = 1 ; n < MODKEY_MAX ; n++ )
		node.SetKeyMap(m_ModKeyList[n], n, m_ModKeyTab);
}

void CTextRam::OnClose()
{
	int x, y;
	int my = m_Lines;
	CCharCell *vp;

	if ( !IsInitText() )
		return;

	SaveHistory();

	if ( m_pDocument == NULL || m_pDocument->m_pLogFile == NULL || IsOptValue(TO_RLLOGMODE, 2) != LOGMOD_LINE )
		return;

	while ( my > 0 ) {
		vp = GETVRAM(0, my - 1);
		for ( x = 0 ; x < m_Cols ; x++ ) {
			if ( !vp[x].IsEmpty() )
				break;
		}
		if ( x < m_Cols )
			break;
		my--;
	}

	for ( y = 0 ; y < my ; y++ )
		CallReciveLine(y);
}
void CTextRam::OnTimer(int id)
{
	m_IntCounter++;

	if ( m_bOscActive ) {
		if ( m_pDocument != NULL && m_IntCounter == 10 ) {
			m_SeqMsg.Format(CStringLoad(IDM_SEQCANCELSTR), m_OscName);
			m_pDocument->UpdateAllViews(NULL, UPDATE_CANCELBTN, (CObject *)(LPCTSTR)m_SeqMsg);
		}
		//if ( m_pCanDlg == NULL && m_IntCounter > 3 ) {
		//	m_pCanDlg = new CCancelDlg;
		//	m_pCanDlg->m_pTextRam = this;
		//	m_pCanDlg->m_PauseSec = 0;
		//	m_pCanDlg->m_WaitSec  = 180;
		//	m_pCanDlg->m_Name     = m_OscName;
		//	m_pCanDlg->Create(IDD_CANCELDLG, ::AfxGetMainWnd());
		//	m_pCanDlg->ShowWindow(SW_SHOW);
		//}
	} else if ( m_IntCounter > 180 ) {
		m_bIntTimer = FALSE;
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);
	}
}

void CTextRam::CallReciveLine(int y)
{
	int n, i;
	CStringW tmp, str;

	if ( m_pDocument == NULL || m_pDocument->m_pLogFile == NULL || IsOptValue(TO_RLLOGMODE, 2) != LOGMOD_LINE )
		return;

	LineEditCwd(m_Cols, y, tmp);
	n = tmp.GetLength();
	for ( i = 0 ; i < n && tmp[n - i - 1] == L' ' ; )
		i++;
	if ( i > 0 )
		tmp.Delete(n - i, i);
	tmp += L"\r\n";

	CBuffer in, out;
	CTime now = CTime::GetCurrentTime();

	if ( IsOptEnable(TO_RLLOGTIME) ) {
		str = now.Format(m_TimeFormat);
		in.Apend((LPBYTE)((LPCWSTR)str), str.GetLength() * sizeof(WCHAR));
	}

	in.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
	m_IConv.StrToRemote(m_SendCharSet[m_LogCharSet[IsOptValue(TO_RLLOGCODE, 2)]], &in, &out);
	m_pDocument->m_pLogFile->Write(out.GetPtr(), out.GetSize());
}
void CTextRam::CallReciveChar(DWORD ch)
{
	static const WCHAR *CtrlName[] = {
		L"NUL",	L"SOH",	L"STX",	L"ETX",	L"EOT",	L"ACK",	L"ENQ",	L"BEL",
		L"BS",	L"HT",	L"LF",	L"VT",	L"FF",	L"CR",	L"SO",	L"SI",
		L"DLE",	L"DC1",	L"DC2",	L"DC3",	L"DC4",	L"NAK",	L"SYN",	L"ETB",
		L"CAN",	L"EM",	L"SUB",	L"ESC",	L"FS",	L"GS",	L"RS",	L"US",
	};

	if ( (ch & 0xFFFF0000) == 0 && ch < ' ' ) {
//		m_LastChar = 0;
		m_LastFlag = 0;
		m_bRtoL = FALSE;
	}

	if ( m_pDocument == NULL || m_bTraceActive )
		return;

	int pos = m_CurY + m_HisPos;
	
	while ( pos < 0 )
		pos += m_HisMax;
	while ( pos >= m_HisMax )
		pos -= m_HisMax;

	m_pDocument->OnReciveChar(ch, m_CurX + pos * m_ColsMax);

	if ( m_pDocument->m_pLogFile == NULL )
		return;

	int md = IsOptValue(TO_RLLOGMODE, 2);

#if 0
	BOOL ct = ((ch & 0xFFFFFF00) != 0 || ch >= 0x20 || ch == 0x09 ? FALSE : TRUE);
	if ( m_LogStat == FALSE ) {
		if ( ct == FALSE )
			m_LogStat = TRUE;
		return;
	} else {
		if ( ct == FALSE )
			return;
		m_LogStat = FALSE;
	}
#endif

	if ( md == LOGMOD_RAW || md == LOGMOD_LINE )
		return;

	CStringW tmp, str;

	if ( ch >= 0 && ch < 0x20 && ch != 0x09 && ch != 0x0A && ch != 0x0D ) {
		if ( md == LOGMOD_CHAR && ch != 0x08 )
			return;
		tmp.Format(L"<%s>", CtrlName[ch]);
	} else {
		if ( (ch & 0xFFFF0000) != 0 )
			tmp = (WCHAR)(ch >> 16);
		tmp += (WCHAR)(ch & 0xFFFF);
	}

	CBuffer in, out;
	CTime now = CTime::GetCurrentTime();

	if ( m_LogTimeFlag && IsOptEnable(TO_RLLOGTIME) ) {
		str = now.Format(m_TimeFormat);
		in.Apend((LPBYTE)((LPCWSTR)str), str.GetLength() * sizeof(WCHAR));
		m_LogTimeFlag = FALSE;
	}

	if ( ch == 0x0A )
		m_LogTimeFlag = TRUE;

	in.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
	m_IConv.StrToRemote(m_SendCharSet[m_LogCharSet[IsOptValue(TO_RLLOGCODE, 2)]], &in, &out);
	m_pDocument->m_pLogFile->Write(out.GetPtr(), out.GetSize());
}

int CTextRam::UnicodeCharFlag(DWORD code)
{
	#include "UniCharTab.h"
	int n, b, m;

	b = 0;
	m = UNICHARTABMAX - 1;
	while ( b <= m ) {
		n = (b + m) / 2;
		if ( code == UniCharTab[n].code )
			return UniCharTab[n].flag;
		else if ( code > UniCharTab[n].code )
			b = n + 1;
		else
			m = n - 1;
	}
	return UniCharTab[b - 1].flag;
}
int CTextRam::UnicodeWidth(DWORD code)
{
	int cf = UnicodeCharFlag(code);

	if ( (cf & UNI_WID) != 0 )
		return 2;
	else if ( (cf & UNI_AMB) != 0 )
		return (IsOptEnable(TO_RLUNIAWH) ? 1 : 2);
	else
		return 1;
}

//////////////////////////////////////////////////////////////////////
// Static Lib
//////////////////////////////////////////////////////////////////////

void CTextRam::MsToIconvUniStr(LPCTSTR charset, LPWSTR str, int len)
{
	if ( _tcscmp(charset, _T("EUC-JISX0213")) == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
			case 0xFFE0: *str = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	} else if ( _tcscmp(charset, _T("SHIFT_JISX0213")) == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
//			case 0x005C: *str = 0x00A5; break;		/* \  0x5C          U+00A5 U+005C */
//			case 0x007E: *str = 0x203E; break;		/* ~  0x7E          U+203E U+007E */
			case 0xFFE0: *str = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	} else if ( _tcscmp(charset, _T("EUC-JP")) == 0 || _tcscmp(charset, _T("EUCJP")) == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
			case 0x2225: *str = 0x2016; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
			case 0xFF0D: *str = 0x2212; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
			case 0xFFE0: *str = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	} else if ( _tcscmp(charset, _T("SHIFT_JIS")) == 0 || _tcscmp(charset, _T("MS_KANJI")) == 0 ||
			    _tcscmp(charset, _T("SHIFT-JIS")) == 0 || _tcscmp(charset, _T("SJIS")) == 0 || _tcscmp(charset, _T("CSSHIFTJIS")) == 0 ) {
		for ( ; len-- > 0 ; str++ ) {
			switch(*str) {							/*                  iconv  MS     */
//			case 0x005C: *str = 0x00A5; break;		/* \  0x5C          U+00A5 U+005C */
//			case 0x007E: *str = 0x203E; break;		/* ~  0x7E          U+203E U+007E */
			case 0x2225: *str = 0x2016; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
			case 0xFF0D: *str = 0x2212; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
			case 0xFF5E: *str = 0x301C; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
			case 0xFFE0: *str = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
			case 0xFFE1: *str = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
			case 0xFFE2: *str = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
			}
		}
	}
}
DWORD CTextRam::IconvToMsUnicode(DWORD code)
{
	switch(code) {							/*                  iconv  MS     */
	case 0x00A2: code = 0xFFE0; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
	case 0x00A3: code = 0xFFE1; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
	case 0x00A5: code = 0x005C; break;		/* \  0x5C          U+00A5 U+005C */
	case 0x00AC: code = 0xFFE2; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
	case 0x2014: code = 0x2015; break;		/* ― 0x815C(01-29) U+2014 U+2015 */
	case 0x2016: code = 0x2225; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
	case 0x203E: code = 0x007E; break;		/* ~  0x7E          U+203E U+007E */
	case 0x2212: code = 0xFF0D; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
	case 0x301C: code = 0xFF5E; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
	}
	return code;
}
DWORD CTextRam::UCS2toUCS4(DWORD code)
{
	// 上位サロゲート      下位サロゲート
	// U+D800 - U+DBFF     U+DC00 - U+DFFF
	// 1101 10** **** **** 1101 11xx xxxx xxxx
	// 0000 0000 000? **** **** **xx xxxx xxxx	U+010000 - U+10FFFF 21 bit
	if ( (code & 0xFC00FC00L) == 0xD800DC00L )
		code = (((code & 0x03FF0000L) >> 6) | (code & 0x3FF)) + 0x10000L;
	return code;
}
DWORD CTextRam::UCS4toUCS2(DWORD code)
{
	// 上位サロゲート      下位サロゲート
	// U+D800 - U+DBFF     U+DC00 - U+DFFF
	// 1101 10** **** **** 1101 11xx xxxx xxxx
	// 0000 0000 000? **** **** **xx xxxx xxxx	U+010000 - U+10FFFF 21 bit
	if ( code >= 0x00010000L ) {
		code -= 0x10000L;
		code = (((code & 0xFFC00L) << 6) | (code & 0x3FF)) | 0xD800DC00L;
	}
	return code;
}
DWORD CTextRam::UnicodeNomal(DWORD code1, DWORD code2)
{
	#include "UniNomalTab.h"
	static BOOL UniNomInitFlag = FALSE;
	#define HASH_MAX 1024
	#define HASH_MASK (HASH_MAX - 1)
	static short HashTab[HASH_MAX];
	static short NodeTab[UNINOMALTABMAX];

	int n, hs;

	if ( !UniNomInitFlag ) {
		for ( n = 0 ; n < HASH_MAX ; n++ )
			HashTab[n] = (-1);
		for ( n = 0 ; n < UNINOMALTABMAX ; n++ ) {
			hs = (UniNomalTab[n].code[0] * 31 + UniNomalTab[n].code[1]) & HASH_MASK;
			NodeTab[n] = HashTab[hs];
			HashTab[hs] = n;
		}
		UniNomInitFlag = TRUE;
	}

	hs = (code1 * 31 + code2) & HASH_MASK;
	for ( n = HashTab[hs] ; n != (-1) ; n = NodeTab[n] ) {
		if ( UniNomalTab[n].code[0] == code1 && UniNomalTab[n].code[1] == code2 )
			return UniNomalTab[n].code[2];
	}
	return 0;
}
void CTextRam::IconvToMsUniStr(LPCWSTR p, int len, CBuffer &out)
{
	DWORD d1 = 0, d2, d3;

	while ( len-- > 0 ) {
		//  1101 10xx	U+D800 - U+DBFF	上位サロゲート	1101 11xx	U+DC00 - U+DFFF	下位サロゲート
		if ( (p[0] & 0xFC00) == 0xD800 && (p[1] & 0xFC00) == 0xDC00 ) {
			d2 = (p[0] << 16) | p[1];
			p += 2;
		} else
			d2 = *(p++);

		if ( d1 != 0 ) {
			if ( (d3 = UnicodeNomal(d1, d2)) != 0 )
				d2 = d3;
			else {
				d1 = IconvToMsUnicode(d1);
				if ( (d1 & 0xFFFF0000L) != 0 )
					out.PutWord((WORD)(d1 >> 16));
				out.PutWord((WORD)d1);
			}
		}
		d1 = d2;
	}
	if ( d1 != 0 ) {
		d1 = IconvToMsUnicode(d1);
		if ( (d1 & 0xFFFF0000L) != 0 )
			out.PutWord((WORD)(d1 >> 16));
		out.PutWord((WORD)d1);
	}
}

//////////////////////////////////////////////////////////////////////

void CTextRam::SetRetChar(BOOL f8)
{
	if ( f8 ) {
		m_RetChar[RC_DCS] = _T("\x90");
		m_RetChar[RC_SOS] = _T("\x98");
		m_RetChar[RC_CSI] = _T("\x9b");
		m_RetChar[RC_ST]  = _T("\x9c");
		m_RetChar[RC_OSC] = _T("\x9d");
		m_RetChar[RC_PM]  = _T("\x9e");
		m_RetChar[RC_APC] = _T("\x9f");
	} else {
		m_RetChar[RC_DCS] = _T("\033P");
		m_RetChar[RC_SOS] = _T("\033X");
		m_RetChar[RC_CSI] = _T("\033[");
		m_RetChar[RC_ST]  = _T("\033\\");
		m_RetChar[RC_OSC] = _T("\033]");
		m_RetChar[RC_PM]  = _T("\033^");
		m_RetChar[RC_APC] = _T("\033_");
	}
}
CGrapWnd *CTextRam::GetGrapWnd(int index)
{
	return CGrapWnd::ListFindIndex(pGrapListIndex[index & 7], index);
}
CGrapWnd *CTextRam::CmpGrapWnd(CGrapWnd *pWnd)
{
	return pWnd->ListFindImage(pGrapListImage[pWnd->m_Crc & 7]);
}
void *CTextRam::LastGrapWnd(int type)
{
	return CGrapWnd::ListFindType(pGrapListType, type);
}
void CTextRam::AddGrapWnd(CGrapWnd *pWnd)
{
	if ( pWnd->m_ImageIndex != (-1) ) {
		pWnd->ListAdd(&(pGrapListIndex[pWnd->m_ImageIndex & 7]), GRAPLIST_INDEX);
		pWnd->ListAdd(&(pGrapListImage[pWnd->m_Crc & 7]), GRAPLIST_IMAGE);
	}
	pWnd->ListAdd(&pGrapListType, GRAPLIST_TYPE);
}
void CTextRam::RemoveGrapWnd(CGrapWnd *pWnd)
{
	pWnd->ListRemove(&(pGrapListIndex[pWnd->m_ImageIndex & 7]), GRAPLIST_INDEX);
	pWnd->ListRemove(&(pGrapListImage[pWnd->m_Crc & 7]), GRAPLIST_IMAGE);
	pWnd->ListRemove(&pGrapListType, GRAPLIST_TYPE);
}
void CTextRam::ChkGrapWnd(int sec)
{
	int n, x, y;
	CCharCell *vp;
	CGrapWnd *pWnd, *pNext;

	for ( ; ; ) {
		switch(m_GrapWndChkStat) {
		case 0:
			if ( !IsInitText() || pGrapListType == NULL ) {
				m_GrapWndChkCLock = clock();
				return;
			} else if ( (m_GrapWndChkCLock + sec * CLOCKS_PER_SEC) > clock() )
				return;
			m_GrapWndChkStat = 1;
			m_GrapWndChkPos = 0 - m_HisLen + m_Lines;
			ZeroMemory(m_GrapWndUseMap, sizeof(m_GrapWndUseMap));
			break;

		case 1:
			for ( y = 0 ; y < 500 && m_GrapWndChkPos < m_Lines ; y++, m_GrapWndChkPos++ ) {
				vp = GETVRAM(0, m_GrapWndChkPos);
				for ( x = 0 ; x < m_ColsMax ; x++ ) {
					if ( IS_IMAGE(vp->m_Vram.mode) && (n = vp->m_Vram.pack.image.id) >= 0 && n < 4096 )
						m_GrapWndUseMap[n] = 1;
					vp++;
				}
			}
			if ( m_GrapWndChkPos < m_Lines )
				return;

			for ( pWnd = pGrapListType ; pWnd != NULL ; pWnd = pNext ) {
				pNext = pWnd->m_pList[GRAPLIST_TYPE];
				if ( pWnd->m_ImageIndex == (-1) )
					continue;
				else if ( pWnd->m_ImageIndex < 1024 ) {
					if ( m_GrapWndUseMap[pWnd->m_ImageIndex] != 0 ) {
						pWnd->m_Alive = 10;
						continue;
					}
					if ( ++pWnd->m_Alive < 10 )
						continue;
					pWnd->m_Alive = 10;
				}
				if ( m_GrapWndUseMap[pWnd->m_ImageIndex] == 0 && pWnd->m_Use <= 0 )
					pWnd->DestroyWindow();
			}

			m_GrapWndChkStat = 0;
			m_GrapWndChkCLock = clock();
			break;
		}
	}
}
void CTextRam::SizeGrapWnd(class CGrapWnd *pWnd)
{
	int x;
	int cx, cy, dx, dy;
	int fw, fh, sw, sh, dw, dh;
	CRLoginView *pView;

	GetMargin(MARCHK_NONE);

	if ( (cx = m_Margin.right - m_CurX) <= 0 )
		cx = m_Cols - m_CurX;

	if ( m_pDocument != NULL && (pView = (CRLoginView *)m_pDocument->GetAciveView()) != NULL ) {
		fw = pView->m_CharWidth;
		fh = pView->m_CharHeight;
	} else {
		fw = 6;
		fh = fw * m_DefFontHw / 10;
	}

	sw = fw * m_Cols;
	sh = fh * m_Lines;
	dw = m_Cols;
	dh = m_Lines;

	dx = pWnd->m_MaxX * pWnd->m_AspX / 100;
	dy = pWnd->m_MaxY * pWnd->m_AspY / 100;

	if ( (x = sw * cx / dw) < dx ) {
		pWnd->m_AspX = pWnd->m_AspX * x / dx;
		pWnd->m_AspY = pWnd->m_AspY * x / dx;
		dx = pWnd->m_MaxX * pWnd->m_AspX / 100;
		dy = pWnd->m_MaxY * pWnd->m_AspY / 100;
		cy = (dy * dh + sh - 1) / sh;
	} else {
		cx = (dx * dw + sw - 1) / sw;
		cy = (dy * dh + sh - 1) / sh;
	}

	pWnd->m_BlockX = cx;
	pWnd->m_BlockY = cy;
}

//////////////////////////////////////////////////////////////////////
// Low Level
//////////////////////////////////////////////////////////////////////

void CTextRam::RESET(int mode)
{
	if ( mode & RESET_PAGE ) {
		if ( IsInitText() )
			SETPAGE(0);
		else
			m_Page = 0;
	}

	if ( mode & RESET_CURSOR ) {
		m_CurX = 0;
		m_CurY = 0;
		m_DoWarp = FALSE;

		m_bRtoL = FALSE;
		m_Status = ST_NULL;
		m_LastChar = 0;
		m_LastFlag = 0;
		m_LastPos  = 0;
		m_Kan_Pos = 0;
		memset(m_Kan_Buf, 0, sizeof(m_Kan_Buf));
	}

	if ( mode & RESET_MARGIN ) {
		m_TopY = 0;
		m_BtmY = m_Lines;
		m_LeftX  = 0;
		m_RightX = m_Cols;
	}

	if ( mode & RESET_TABS ) {
		m_DefTab = DEF_TAB;
		TABSET(TAB_RESET);
	}

	if ( mode & RESET_BANK ) {
		m_BankGL = 0;
		m_BankGR = 1;
		m_BankSG = (-1);
		memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));
	}

	if ( mode & RESET_ATTR ) {
		m_DefAtt.pack.dchar = 0;
		m_DefAtt.bank = m_BankTab[m_KanjiMode][m_BankGL];
		m_DefAtt.eram = 0;
		m_DefAtt.zoom = 0;
		m_DefAtt.mode = 0;
//		m_DefAtt.attr = 0;
		m_AttSpc = m_DefAtt;
		m_AttNow = m_DefAtt;
	}

	if ( mode & RESET_COLOR ) {
		int n = 16;
		memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));

		// colors 16-231 are a 6x6x6 color cube
		for ( int r = 0 ; r < 6 ; r++ ) {
			for ( int g = 0 ; g < 6 ; g++ ) {
				for ( int b = 0 ; b < 6 ; b++ )
					m_ColTab[n++] = RGB(r * 51, g * 51, b * 51);
			}
		}
		// colors 232-255 are a grayscale ramp, intentionally leaving out
		for ( int g = 0 ; g < 24 ; g++ )
			m_ColTab[n++] = RGB(g * 11, g * 11, g * 11);
	}

	if ( mode & RESET_OPTION ) {
		memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));
		m_VtLevel = 65;
		m_TermId  = 10;
		m_LangMenu = 0;
		SetRetChar(FALSE);
		m_FileSaveFlag = TRUE;
		m_XtOptFlag = 0;
		m_XtMosPointMode = 0;
		m_TitleStack.RemoveAll();
		if ( m_pDocument != NULL )
			m_pDocument->SetStatus(NULL);
		memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
		InitModKeyTab();
	}

	if ( mode & RESET_CLS )
		ERABOX(0, 0, m_Cols, m_Lines);

	if ( mode & RESET_SAVE ) {
		m_Save_CurX = m_CurX;
		m_Save_CurY = m_CurY;
		m_Save_AttNow = m_AttNow;
		m_Save_AttSpc = m_AttSpc;
		m_Save_BankGL = m_BankGL;
		m_Save_BankGR = m_BankGR;
		m_Save_BankSG = m_BankSG;
		m_Save_DoWarp = m_DoWarp;
		m_Save_Decom  = (IsOptEnable(TO_DECOM) ? TRUE : FALSE);
		memcpy(m_Save_BankTab, m_BankTab, sizeof(m_BankTab));
		memcpy(m_Save_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
		memcpy(m_Save_TabMap, m_TabMap, sizeof(m_TabMap));
		for ( int n = 0 ; n < 64 ; n++ )
			m_Macro[n].Clear();
		memset(m_MacroExecFlag, 0, sizeof(m_MacroExecFlag));
	}

	if ( mode & RESET_MOUSE ) {
		m_MouseTrack = MOS_EVENT_NONE;
		m_MouseRect.SetRectEmpty();
		m_Loc_Mode  = 0;
	}

	if ( mode & RESET_CHAR ) {
		m_Exact = FALSE;
		m_StsFlag = FALSE;
		m_StsMode = 0;
		m_StsLed  = 0;
		fc_Init(m_KanjiMode);
	}

	if ( mode & RESET_TEK ) {
		m_Tek_Ver  = 4014;
		m_Tek_Mode = 0x1F;
		m_Tek_Pen  = 0;
		m_Tek_Stat = 0;
		m_Tek_Line = 0x08;
		m_Tek_Font = 0x08;
		m_Tek_Angle = 0;
		m_Tek_Base  = 0;
		m_Tek_Point = 0;
		m_Tek_Gin   = FALSE;
		m_Tek_Flush = FALSE;
		m_Tek_Int   = 0;
		m_Tek_cX = 0; m_Tek_cY = TEK_WIN_HEIGHT - 87;
		m_Tek_lX = 0; m_Tek_lY = m_Tek_cY;
		m_Tek_wX = 0; m_Tek_wY = m_Tek_cY;
		TekClear();
	}
}
CCharCell *CTextRam::GETVRAM(int cols, int lines)
{
	int pos = m_HisPos + lines;

	while ( pos < 0 )
		pos += m_HisMax;

	while ( pos >= m_HisMax )
		pos -= m_HisMax;

#ifdef	USE_FIXWCHAR
	return GETMAPRAM(m_hMap, cols, pos);
#else
	return (m_VRam + cols + m_ColsMax * pos);
#endif
}
void CTextRam::UNGETSTR(LPCTSTR str, ...)
{
	CString tmp, in;
	CStringA mbs, out;
	va_list arg;
	va_start(arg, str);
	tmp.FormatV(str, arg);
	if ( m_pDocument != NULL ) {
		for ( str = tmp ; *str != _T('\0') ; str++ ) {
			if ( *str > 0xFF ) {
				in = *str;
				m_pDocument->m_TextRam.m_IConv.StrToRemote(m_pDocument->m_TextRam.m_SendCharSet[m_pDocument->m_TextRam.m_KanjiMode], in, out);
				mbs += out;
			} else
				mbs += *str;
		}
		m_pDocument->SocketSend((void *)(LPCSTR)mbs, mbs.GetLength());

		if ( m_pCallPoint == &CTextRam::fc_TraceCall )
			m_TraceSendBuf.Apend((LPBYTE)(LPCSTR)mbs, mbs.GetLength());
	}
	va_end(arg);
}
void CTextRam::BEEP()
{
	if ( !IsOptEnable(TO_RLADBELL) )
		MessageBeep(MB_OK);
	if ( IsOptEnable(TO_RLVSBELL) )
		m_pDocument->UpdateAllViews(NULL, UPDATE_VISUALBELL, NULL);
}
void CTextRam::FLUSH()
{
	if ( m_UpdateRect == CRect(0, 0, m_Cols, m_Lines) )
		m_pDocument->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
	else if ( m_UpdateRect.left < m_UpdateRect.right && m_UpdateRect.top < m_UpdateRect.bottom )
		m_pDocument->UpdateAllViews(NULL, UPDATE_TEXTRECT, (CObject *)&m_UpdateRect);
	else
		m_pDocument->UpdateAllViews(NULL, UPDATE_GOTOXY, NULL);

	m_UpdateRect.left   = m_Cols;
	m_UpdateRect.top    = m_Lines;
	m_UpdateRect.right  = 0;
	m_UpdateRect.bottom = 0;

	m_HisUse = 0;

	if ( m_pTraceWnd != NULL && (m_pTraceTop != NULL || (m_pTraceNow != NULL && m_bTraceUpdate)) ) {
		m_pTraceWnd->m_List.LockWindowUpdate();

		while ( m_pTraceTop != NULL ) {
			CTraceNode *tp = m_pTraceTop->m_Next;
			tp->m_Next->m_Back = m_pTraceTop;
			m_pTraceTop->m_Next = tp->m_Next;
			if ( tp == m_pTraceTop )
				m_pTraceTop = NULL;
			m_pTraceWnd->AddTraceNode(tp);
		}

		if ( m_pTraceNow != NULL && m_bTraceUpdate ) {
			m_pTraceNow->m_CurX = m_CurX;
			m_pTraceNow->m_CurY = m_CurY;
			m_pTraceNow->m_Attr = m_AttNow.attr;
			m_pTraceNow->m_ForCol = m_AttNow.fcol;
			m_pTraceNow->m_BakCol = m_AttNow.bcol;
			m_pTraceWnd->AddTraceNode(m_pTraceNow);
			m_bTraceUpdate = FALSE;
		}

		m_pTraceWnd->m_List.EnsureVisible(m_pTraceWnd->m_List.GetItemCount() - 1, FALSE);
		m_pTraceWnd->m_List.UnlockWindowUpdate();
	}
}
void CTextRam::CUROFF()
{
	m_DispCaret &= ~FGCARET_ONOFF;
}	
void CTextRam::CURON()
{
	m_DispCaret |= FGCARET_ONOFF;
}
void CTextRam::DISPUPDATE()
{
	m_UpdateRect.left   = 0;
	m_UpdateRect.top    = 0;
	m_UpdateRect.right  = m_Cols;
	m_UpdateRect.bottom = m_Lines;
	m_FrameCheck = FALSE;

	if ( IsOptEnable(TO_DECSCLM) )
		m_UpdateFlag = TRUE;
	else
		m_pDocument->IncActCount();
}
void CTextRam::DISPVRAM(int sx, int sy, int w, int h)
{
	int ex = sx + w;
	int ey = sy + h;

#ifdef	USE_TEXTFRAME
	if ( m_FrameCheck ) {
		if ( sx > 0 ) sx--;
		if ( ex < m_Cols ) ex++;
		if ( sy > 0 ) sy--;
		if ( ey < m_Lines ) ey++;
	}
#endif

	if ( m_UpdateRect.left > sx )
		m_UpdateRect.left = sx;
	if ( m_UpdateRect.right < ex )
		m_UpdateRect.right = ex;
	if ( m_UpdateRect.top> sy )
		m_UpdateRect.top = sy;
	if ( m_UpdateRect.bottom < ey )
		m_UpdateRect.bottom = ey;

	if ( IsOptEnable(TO_DECSCLM) )
		m_UpdateFlag = TRUE;
	else
		m_pDocument->IncActCount();
}
void CTextRam::DISPRECT(int sx, int sy, int ex, int ey)
{
	int y;

	if ( sx > 0 ) {
		for ( y = sy ; y < ey ; y++ ) {
			if ( IS_2BYTE(GETVRAM(sx, y)->m_Vram.mode) ) {
				sx--;
				break;
			}
		}
	}
	if ( ex < m_Cols ) {
		for ( y = sy ; y < ey ; y++ ) {
			if ( IS_1BYTE(GETVRAM(ex, y)->m_Vram.mode) ) {
				ex++;
				break;
			}
		}
	}
	DISPVRAM(sx, sy, ex - sx, ey - sy);
}
int CTextRam::BLINKUPDATE(class CRLoginView *pView)
{
	int x, y, dm;
	CCharCell *vp;
	int rt = 0;
	int mk = ATT_BLINK;
	CRect rect;

	if ( (pView->m_BlinkFlag & 1) == 0 )
		mk |= ATT_SBLINK;

	for ( y = 0 ; y < m_Lines ; y++ ) {
		vp = GETVRAM(0, y - pView->m_HisOfs + pView->m_HisMin);
		dm = vp->m_Vram.zoom;
		rect.left   = m_Cols;
		rect.right  = 0;
		rect.top    = y - pView->m_HisOfs + pView->m_HisMin;
		rect.bottom = rect.top + 1;
		for ( x = 0 ; x < m_Cols ; x++ ) {
			if ( (vp->m_Vram.attr & mk) != 0 ) {
				if ( rect.left > x ) {
					rect.left  = x;
					rect.right = x + 1;
					rt |= 3;
				} else if ( rect.right == x ) {
					rect.right = x + 1;
					rt |= 3;
				} else {
					if ( rect.left < rect.right ) {
						if ( dm ) {
							rect.left  *= 2;
							rect.right *= 2;
						}
						m_pDocument->UpdateAllViews(NULL, UPDATE_BLINKRECT, (CObject *)&rect);
					}
					rect.left  = x;
					rect.right = x + 1;
					rt |= 3;
				}
			} else if ( (vp->m_Vram.attr & ATT_SBLINK) != 0 )
				rt |= 2;
			vp++;
		}
		if ( (rt & 1) != 0 ) {
			if ( dm ) {
				rect.left  *= 2;
				rect.right *= 2;
			}
			m_pDocument->UpdateAllViews(NULL, UPDATE_BLINKRECT, (CObject *)&rect);
			rt &= 0xFE;
		}
	}
	return rt;
}

//////////////////////////////////////////////////////////////////////
// Mid Level
//////////////////////////////////////////////////////////////////////

BOOL CTextRam::GetMargin(int bCheck)
{
	BOOL st = TRUE;

	if ( IsOptEnable(TO_DECLRMM) ) {
		m_Margin.left   = m_LeftX;
		m_Margin.right  = m_RightX;
	} else {
		m_Margin.left   = 0;
		m_Margin.right  = m_Cols;
	}

	m_Margin.top    = m_TopY;
	m_Margin.bottom = m_BtmY;

	if ( (bCheck & MARCHK_COLS) != 0 ) {
		if ( m_CurX < m_Margin.left )
			st = FALSE;
		else if ( m_CurX >= m_Margin.right )
			st = FALSE;
	}

	if ( (bCheck & MARCHK_LINES) != 0 ) {
		if ( m_CurY < m_Margin.top )
			st = FALSE;
		else if ( m_CurY >= m_Margin.bottom )
			st = FALSE;
	}

	return st;
}

int CTextRam::GetAnsiPara(int index, int defvalue, int minvalue, int maxvalue)
{
	int val;

	// CSI 1;2:3:4;5
	//
	// Para[0] = 1
	// Para[1] = 2	Para[1][0] = 3, Para[1][1] = 4
	// Para[2] = 5

	if ( index >= m_AnsiPara.GetSize() ) {
		val = defvalue;

	} else {
		val = m_AnsiPara[index];

		if ( val == PARA_NOT || val == PARA_OPT || val < minvalue )
			val = defvalue;

		else if ( maxvalue >= 0 && val > maxvalue )
			val = maxvalue;
	}

	return val;
}
void CTextRam::SetAnsiParaArea(int top)
{
	while ( m_AnsiPara.GetSize() < top )
		m_AnsiPara.Add(PARA_NOT);

	// Start Line
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(1);
	else if ( m_AnsiPara[top].IsEmpty() )
		m_AnsiPara[top] = 1;
	if ( IsOptEnable(TO_DECOM) )
		m_AnsiPara[top] = m_AnsiPara[top] + GetTopMargin();
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Lines )
		m_AnsiPara[top] = m_Lines - 1;
	top++;

	// Start Cols
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(1);
	else if ( m_AnsiPara[top].IsEmpty() )
		m_AnsiPara[top] = 1;
	if ( IsOptEnable(TO_DECOM) )
		m_AnsiPara[top] = m_AnsiPara[top] + GetLeftMargin();
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Cols )
		m_AnsiPara[top] = m_Cols - 1;
	top++;

	// End Line
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(m_Lines);
	else if ( m_AnsiPara[top].IsEmpty() )
		m_AnsiPara[top] = m_Lines;
	if ( IsOptEnable(TO_DECOM) )
		m_AnsiPara[top] = m_AnsiPara[top] + GetTopMargin();
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Lines )
		m_AnsiPara[top] = m_Lines - 1;
	top++;

	// End Cols
	if ( m_AnsiPara.GetSize() < (top + 1) )
		m_AnsiPara.Add(m_Cols);
	else if ( m_AnsiPara[top].IsEmpty() )
		m_AnsiPara[top] = m_Cols;
	if ( IsOptEnable(TO_DECOM) )
		m_AnsiPara[top] = m_AnsiPara[top] + GetLeftMargin();
	if ( (m_AnsiPara[top] = m_AnsiPara[top] - 1) < 0 )
		m_AnsiPara[top] = 0;
	else if ( m_AnsiPara[top] >= m_Cols )
		m_AnsiPara[top] = m_Cols - 1;
}
void CTextRam::LocReport(int md, int sw, int x, int y)
{
	// DECLRP(Locator report): CSI Pe ; Pb ; Pr ; Pc ; Pp & w
	// Pe is the event code 
	// Pb is the button code 
	// Pr is the row coordinate 
	// Pc is the column coordinate 
	// Pp is the third coordinate (page number) 
	//
	// Pe, the event code indicates what event caused this report to be generated.
	// The following event codes are defined:
	// 0 - request, the terminal received an explicit request for a locator report, but the locator is unavailable 
	// 1 - request, the terminal received an explicit request for a locator report 
	// 2 - left button down 
	// 3 - left button up 
	// 4 - middle button down 
	// 5 - middle button up 
	// 6 - right button down 
	// 7 - right button up 
	// 8 - fourth button down 
	// 9 - fourth button up 
	// 10 - locator outside filter rectangle 
	//
	// Pb is the button code, ASCII decimal 0-15 indicating which buttons are
	// down if any. The state of the four buttons on the locator correspond
	// to the low four bits of the decimal value, "1" means button depressed
	//
	// 0 - no buttons down 
	// 1 - right 
	// 2 - middle 
	// 4 - left 
	// 8 - fourth 

	int Pe = 0;
	int Pb = 0;

	if ( (m_Loc_Mode & LOC_MODE_PIXELS) != 0 ) {
		x = m_MousePos.x;
		y = m_MousePos.y;
	}

	if ( (sw & MK_RBUTTON) != 0 )
		Pb |= 1;

	if ( (sw & MK_LBUTTON) != 0 )
		Pb |= 4;

	if ( md == MOS_LOCA_INIT ) {			// Init Mode
		m_MouseTrack = ((m_Loc_Mode & LOC_MODE_ENABLE) != 0 ? MOS_EVENT_LOCA : MOS_EVENT_NONE);
		m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
		return;

	} else if ( md == MOS_LOCA_REQ ) {		// Request
		if ( (m_Loc_Mode & LOC_MODE_ENABLE) != 0 )
			Pe = 1;

	} else {					// Mouse Event
		switch(md) {
		case MOS_LOCA_LEDN:	// Left Down
			Pe = 2;
			break;
		case MOS_LOCA_LEUP:	// Left Up
			Pe = 3;
			break;
		case MOS_LOCA_RTDN:	// Right Down
			Pe = 6;
			break;
		case MOS_LOCA_RTUP:	// Right Up
			Pe = 7;
			break;
		case MOS_LOCA_MOVE:	// Mouse Move
			if ( (m_Loc_Mode & LOC_MODE_FILTER) == 0 )
				return;
			if ( m_Loc_Rect.IsRectEmpty() || m_Loc_Rect.PtInRect(CPoint(x, y)) )
				return;
			Pe = 10;
			break;
		}

		m_Loc_Mode &= ~LOC_MODE_FILTER;

		if ( (m_Loc_Mode & LOC_MODE_ONESHOT) == 0 ) {
			if ( (m_Loc_Mode & LOC_MODE_EVENT) == 0 )
				return;
			else if ( (md == MOS_LOCA_LEDN || md == MOS_LOCA_RTDN) && (m_Loc_Mode & LOC_MODE_DOWN) == 0 )
				return;
			else if ( (md == MOS_LOCA_LEUP || md == MOS_LOCA_RTUP) && (m_Loc_Mode & LOC_MODE_UP) == 0 )
				return;
		}
	}

	UNGETSTR(_T("%s%d;%d;%d;%d;%d&w"), m_RetChar[RC_CSI], Pe, Pb, y + 1, x + 1, 0);

	if ( (m_Loc_Mode & LOC_MODE_ONESHOT) != 0 ) {
		m_Loc_Mode &= ~(LOC_MODE_ENABLE | LOC_MODE_ONESHOT);
		m_MouseTrack = MOS_EVENT_NONE;
		m_pDocument->UpdateAllViews(NULL, UPDATE_SETCURSOR, NULL);
	}
}
void CTextRam::MouseReport(int md, int sw, int x, int y)
{
	if ( x < 0 )
		x = 0;
	else if ( x >= m_Cols  )
		x = m_Cols - 1;

	if ( y < 0 )
		y = 0;
	else if ( y >= m_Lines )
		y = m_Lines - 1;

	if ( m_MouseTrack == MOS_EVENT_LOCA ) {
		LocReport(md, sw, x, y);
		return;
	}

	// Button Code
	// Left			xxxx xx00	00
	// Right		xxxx xx01	01
	// Middle		xxxx xx10	02
	// Button 1		x1xx xx00	40
	// Button 2		x1xx xx01	41
	// Button 3		x1xx xx10	42
	// Release		xxxx xx11	03
	// shift		xxxx x1xx	04
	// meta			xxxx 1xxx	08
	// ctrl			xxx1 xxxx	10
	// motion		xx1x xxxx	20

	CStringA tmp, str;
	int stat = 0;
	int code = 'M';

	switch(md) {
	case MOS_LOCA_LEDN:
		stat = m_MouseMode[0];
		break;

	case MOS_LOCA_LEUP:
		if ( m_MouseTrack < MOS_EVENT_NORM )
			return;
		else if ( m_MouseTrack == MOS_EVENT_HILT ) {
			stat = 0x03;	// Release
			code = 't';
		} else if ( IsOptEnable(TO_XTSGRMOS) ) {
			stat = m_MouseMode[0];
			code = 'm';
		} else
			stat = 0x03;	// Release
		break;

	case MOS_LOCA_RTDN:
		if ( m_MouseTrack == MOS_EVENT_HILT )
			return;
		stat = m_MouseMode[1];
		break;

	case MOS_LOCA_RTUP:
		if ( m_MouseTrack < MOS_EVENT_NORM || m_MouseTrack == MOS_EVENT_HILT )
			return;
		if ( IsOptEnable(TO_XTSGRMOS) ) {
			stat = m_MouseMode[1];
			code = 'm';
		} else
			stat = 0x03;	// Release
		break;

	case MOS_LOCA_MOVE:
		if ( m_MouseTrack == MOS_EVENT_BTNE ) {
			if ( (sw & (MK_LBUTTON | MK_RBUTTON)) == 0 )
				return;
		} else if ( m_MouseTrack != MOS_EVENT_ANYE )
			return;
		if ( m_MouseOldSw == sw && m_MouseOldPos.x == x && m_MouseOldPos.y == y )
			return;
		if ( (sw & MK_LBUTTON) != 0 )
			stat = m_MouseMode[0];
		else if ( (sw & MK_RBUTTON) != 0 )
			stat = m_MouseMode[1];
		else
			stat = 0x03;	// Release
		stat |= 0x20;		// Motion
		break;

	default:
		return;
	}

	m_MouseOldSw    = sw;
	m_MouseOldPos.x = x; 
	m_MouseOldPos.y = y;

	if ( m_MouseTrack == MOS_EVENT_X10 ) {
		stat &= 0x03;
	} else {
		if ( (sw & MK_SHIFT) != 0 )
			stat |= m_MouseMode[2];
		if ( (sw & MK_CONTROL) != 0 )
			stat |= m_MouseMode[3];
	}

	tmp = m_RetChar[RC_CSI];

	if ( IsOptEnable(TO_XTSGRMOS) )
		tmp += '<';
	else if ( !IsOptEnable(TO_XTURXMOS) )
		tmp += (CHAR)code;

	if ( code != 't' ) {
		if ( IsOptEnable(TO_XTSGRMOS) ) {
			str.Format("%d;", stat);
			tmp += str;
		} else if ( IsOptEnable(TO_XTURXMOS) ) {
			str.Format("%d;", stat + ' ');
			tmp += str;
		} else if ( IsOptEnable(TO_XTEXTMOS) ) {
			stat += (' ');
			if ( stat < 128 )
				tmp += (CHAR)(stat);
			else {
				tmp += (CHAR)(0xC0 + (stat >> 6));
				tmp += (CHAR)(0x80 + (stat & 0x3F));
			}
		} else {
			tmp += (CHAR)(stat + ' ');
		}
	}

	if ( IsOptEnable(TO_XTSGRMOS) || IsOptEnable(TO_XTURXMOS) ) {
		str.Format("%d;%d", x + 1, y + 1);
		tmp += str;

	} else if ( IsOptEnable(TO_XTEXTMOS) ) {
		x += (' ' + 1);
		if ( x < 128 )
			tmp += (CHAR)(x);
		else {							// x < COLS_MAX !!!
		    tmp += (CHAR)(0xC0 + (x >> 6));
		    tmp += (CHAR)(0x80 + (x & 0x3F));
		}

		y += (' ' + 1);
		if ( y < 128 )
			tmp += (CHAR)(y);
		else {							// y < LINE_MAX !!!
		    tmp += (CHAR)(0xC0 + (y >> 6));
		    tmp += (CHAR)(0x80 + (y & 0x3F));
		}

	} else {
		x += (' ' + 1);
		if ( x < 256 )
			tmp += (CHAR)(x);
		else
			tmp += (CHAR)(255);

		y += (' ' + 1);
		if ( y < 256 )
			tmp += (CHAR)(y);
		else
			tmp += (CHAR)(255);
	}

	if ( IsOptEnable(TO_XTSGRMOS) || IsOptEnable(TO_XTURXMOS) )
		tmp += (CHAR)code;

	if ( m_pDocument != NULL )
		m_pDocument->SocketSend((void *)(LPCSTR)tmp, tmp.GetLength());
}
LPCTSTR CTextRam::GetHexPara(LPCTSTR str, CBuffer &buf)
{
	// ! Pn ; D...D ;		! is the repeat sequence introducer.

	int n;
	CBuffer tmp;

	buf.Clear();
	while ( *str != _T('\0') ) {
		if ( *str == _T(';') ) {
			str++;
			break;

		} else if ( *str == _T('!') ) {
			str++;
			for ( n = 0 ; isdigit(*str) ; )
				n = n * 10 + (*(str++) - _T('0'));
			if ( *str != _T(';') )
				break;
			str++;
			str = GetHexPara(str, tmp);
			while ( n-- > 0 )
				buf.Apend(tmp.GetPtr(), tmp.GetSize());

		} else if ( isxdigit(str[0]) && isxdigit(str[1]) ) {
			str = tmp.Base16Decode(str);
			buf.Apend(tmp.GetPtr(), tmp.GetSize());
		} else
			break;
	}
	return str;
}

void CTextRam::LOCATE(int x, int y)
{
	m_DoWarp = FALSE;
	if ( (m_CurX = x) >= m_Cols )
		m_CurX = m_Cols - 1;
	else if ( m_CurX < 0 )
		m_CurX = 0;
	if ( (m_CurY = y) >= m_Lines )
		m_CurY = m_Lines - 1;
	else if ( m_CurY < 0 )
		m_CurY = 0;
	if ( m_CurX >= (m_Cols / 2) && GetDm(m_CurY) != 0 )
		m_CurX = (m_Cols / 2) - 1;
}
void CTextRam::ERABOX(int sx, int sy, int ex, int ey, int df)
{
	int x, y, dm;
	CCharCell *vp, *tp, spc;

	m_DoWarp = FALSE;

	if ( sx < 0 ) sx = 0; else if ( sx >= m_Cols  ) return;
	if ( sy < 0 ) sy = 0; else if ( sy >= m_Lines ) return;
	if ( ex < sx ) return; else if ( ex > m_Cols  ) ex = m_Cols;
	if ( ey < sy ) return; else if ( ey > m_Lines ) ey = m_Lines;

	if ( (df & ERM_ISOPRO) != 0 && IsOptEnable(TO_ANSIERM) )
		df &= ~ERM_ISOPRO;

	spc = m_AttSpc;
	for ( y = sy ; y < ey ; y++ ) {
		tp = GETVRAM(0, y);
		dm = tp->m_Vram.zoom;
		vp = tp + sx;

		switch(df & (ERM_ISOPRO | ERM_DECPRO)) {
		case 0:		// clear em
			CCharCell::Fill(vp, m_AttSpc, ex - sx);
			//for ( x = sx ; x < ex ; x++ )
			//	*(vp++) = spc;
			break;
		case ERM_ISOPRO:
			for ( x = sx ; x < ex ; x++, vp++ ) {
				if ( (vp->m_Vram.eram & EM_ISOPROTECT) == 0 )
					*vp = spc;
			}
			break;
		case ERM_DECPRO:
			for ( x = sx ; x < ex ; x++, vp++ ) {
				if ( (vp->m_Vram.eram & EM_DECPROTECT) == 0 )
					*vp = spc;
			}
			break;
		}

		if ( (df & ERM_SAVEDM) == 0 ) {
			tp->m_Vram.zoom = 0;
			DISPVRAM(sx, y, ex - sx, 1);
		} else {
			if ( (tp->m_Vram.zoom = dm) != 0 )
				DISPVRAM(sx * 2, y, (ex - sx) * 2, 1);
			else
				DISPVRAM(sx, y, ex - sx, 1);
		}
	}
}
void CTextRam::FORSCROLL(int sx, int sy, int ex, int ey)
{
	ASSERT(sx >= 0 && sx < m_Cols);
	ASSERT(sy >= 0 && sy < m_Lines);
	ASSERT(ex > 0 && ex <= m_Cols);
	ASSERT(ey > 0 && ey <= m_Lines);

	m_DoWarp = FALSE;

	if ( sy == 0 && ey == m_Lines && sx == 0 && ex == m_Cols && !m_LineEditMode && !m_bTraceActive ) {
		CallReciveLine(0);
		m_HisUse++;

		if ( (m_HisPos += 1) >= m_HisMax )
			m_HisPos -= m_HisMax;
		if ( m_HisLen < (m_HisMax - 1) )
			m_HisLen += 1;
		if ( m_LineUpdate < m_Lines )
			m_LineUpdate++;

		CCharCell::Fill(GETVRAM(0, m_Lines - 1), m_AttSpc, m_ColsMax);

	} else {
		for ( int y = sy + 1; y < ey ; y++ )
			CCharCell::Copy(GETVRAM(sx, y - 1), GETVRAM(sx, y), ex - sx);

		ERABOX(sx, ey - 1, ex, ey);
	}

	DISPRECT(sx, sy, ex, ey);
}
void CTextRam::BAKSCROLL(int sx, int sy, int ex, int ey)
{
	ASSERT(sx >= 0 && sx < m_Cols);
	ASSERT(sy >= 0 && sy < m_Lines);
	ASSERT(ex > 0 && ex <= m_Cols);
	ASSERT(ey > 0 && ey <= m_Lines);

	int x, y;
	CCharCell *vp, *wp;

	m_DoWarp = FALSE;

	for ( y = ey - 1; y > sy ; y-- )
		CCharCell::Copy(GETVRAM(sx, y), GETVRAM(sx, y - 1), ex - sx);

	ERABOX(sx, sy, ex, sy + 1);
	DISPRECT(sx, sy, ex, ey);
}
void CTextRam::LEFTSCROLL()
{
	int n, i, dm;
	CCharCell *vp;

	GetMargin(MARCHK_NONE);

	for ( n = m_Margin.top ; n < m_Margin.bottom ; n++ ) {
		vp = GETVRAM(0, n);
		dm = vp->m_Vram.zoom;
		for ( i = m_Margin.left + 1 ; i < m_Margin.right ; i++ )
			vp[i - 1] = vp[i];
		vp[m_Margin.right - 1] = m_AttSpc;
		vp->m_Vram.zoom = dm;
	}


	DISPRECT(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
}
void CTextRam::RIGHTSCROLL()
{
	int n, i, dm;
	CCharCell *vp;

	GetMargin(MARCHK_NONE);

	for ( n = m_Margin.top ; n < m_Margin.bottom ; n++ ) {
		vp = GETVRAM(0, n);
		dm = vp->m_Vram.zoom;
		for ( i = m_Margin.right - 1 ; i > m_Margin.left ; i-- )
			vp[i] = vp[i - 1];
		vp[m_Margin.left] = m_AttSpc;
		vp->m_Vram.zoom = dm;
	}

	DISPRECT(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
}
void CTextRam::DOWARP()
{
	if ( !m_DoWarp )
		return;

	GetMargin(MARCHK_NONE);

	m_DoWarp = FALSE;
	m_LastPos -= COLS_MAX;
	m_CurX = m_Margin.left;

	ONEINDEX();
}
void CTextRam::INSCHAR(BOOL bMargin)
{
	int n, dm;
	CCharCell *vp;
	BOOL st;

	m_DoWarp = FALSE;

	st = GetMargin(MARCHK_COLS);

	if ( !bMargin ) {
		if ( m_CurX >= m_Margin.right )
			m_Margin.right  = m_Cols;
		st = TRUE;
	}

	if ( st ) {
		vp = GETVRAM(0, m_CurY);
		dm = vp->m_Vram.zoom;

		for ( n = m_Margin.right - 1 ; n > m_CurX ; n-- )
			vp[n] = vp[n - 1];

		vp[n] = m_AttSpc;
		vp->m_Vram.zoom = dm;

		if ( dm != 0 )
			DISPVRAM(0, m_CurY, m_Cols, 1);
		else
			DISPVRAM(m_CurX, m_CurY, m_Margin.right - m_CurX, 1);
	}
}
void CTextRam::DELCHAR()
{
	int n, dm;
	CCharCell *vp;

	m_DoWarp = FALSE;

	if ( GetMargin(MARCHK_COLS) ) {
		vp = GETVRAM(0, m_CurY);
		dm = vp->m_Vram.zoom;

		for ( n = m_CurX + 1; n < m_Margin.right ; n++ )
			vp[n - 1] = vp[n];

		vp[n - 1] = m_AttSpc;
		vp->m_Vram.zoom = dm;

		if ( dm != 0 )
			DISPVRAM(0, m_CurY, m_Cols, 1);
		else
			DISPVRAM(m_CurX, m_CurY, m_Margin.right - m_CurX, 1);
	}
}
void CTextRam::FILLCHAR(int ch)
{
	int x, y, dm;
	CCharCell *vp, tmp;

	tmp = m_AttNow;
	tmp.m_Vram.bank = m_BankTab[m_KanjiMode][0];
	tmp = (DWORD)ch;

	for ( y = 0 ; y < m_Lines ; y++ ) {
		vp = GETVRAM(0, y);
		dm = vp->m_Vram.zoom;
		for ( x = 0 ; x < m_Cols ; x++ )
			*(vp++) = tmp;
		vp->m_Vram.zoom = dm;
	}

	DISPVRAM(0, 0, m_Cols, m_Lines);
}
void CTextRam::ONEINDEX()
{
	m_DoWarp = FALSE;

	if ( GetMargin(MARCHK_LINES) ) {
		if ( ++m_CurY >= m_Margin.bottom ) {
			m_CurY = m_Margin.bottom - 1;
			FORSCROLL(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
			m_LineEditIndex++;
		}
	} else {
		if ( ++m_CurY >= m_Lines )
			m_CurY = m_Lines - 1;
	}
}
void CTextRam::REVINDEX()
{
	m_DoWarp = FALSE;

	if ( GetMargin(MARCHK_LINES) ) {
		if ( --m_CurY < m_Margin.top ) {
			m_CurY = m_Margin.top;
			BAKSCROLL(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
		}
	} else {
		if ( --m_CurY < 0 )
			m_CurY = 0;
	}
}
void CTextRam::PUT1BYTE(DWORD ch, int md, int at)
{
	if ( m_StsFlag ) {
		md &= CODE_MASK;
		ch |= m_FontTab[md].m_Shift;
		ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, _T("UTF-16BE"), ch);			// Char変換ではUTF-16BEを使用！
		ch = IconvToMsUnicode(ch);
		m_LastChar = ch;
		m_LastFlag = 0;
		m_LastPos  = 0;
		if ( (ch & 0xFFFF0000) != 0 )
			m_StsPara += (WCHAR)(ch >> 16);
		m_StsPara += (WCHAR)ch;
		((CMainFrame *)AfxGetMainWnd())->SetMessageText(CString(m_StsPara));
		return;
	}

	DOWARP();

	CCharCell *vp;
	int dm = GetDm(m_CurY);

	if ( !GetMargin(MARCHK_COLS) ) {
		m_Margin.left  = 0;
		m_Margin.right = m_Cols;
	}

	if ( dm != 0 ) {
		m_Margin.left  /= 2;
		m_Margin.right /= 2;
	}

	if ( m_bRtoL ) {
		md = SET_UNICODE;
		at = ATT_RTOL;
	}

	vp = GETVRAM(m_CurX, m_CurY);

	vp->m_Vram.bank = (WORD)md;
	vp->m_Vram.eram = m_AttNow.eram;
//	vp->m_Vram.zoom = m_AttNow.dm;	no Init
	vp->m_Vram.mode = CM_ASCII;
	vp->m_Vram.attr = m_AttNow.attr | at;
	vp->m_Vram.font = m_AttNow.font;
	vp->m_Vram.fcol = m_AttNow.fcol;
	vp->m_Vram.bcol = m_AttNow.bcol;

	md &= CODE_MASK;
	ch |= m_FontTab[md].m_Shift;
	ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, _T("UTF-16BE"), ch);			// Char変換ではUTF-16BEを使用！
	ch = IconvToMsUnicode(ch);

	if ( ch >= 0x2500 && ch <= 0x257F && !IsOptEnable(TO_RLDRWLINE) )		// Border Char
		vp->m_Vram.attr |= ATT_BORDER;

	*vp = (DWORD)ch;

	if ( dm != 0 )
		DISPVRAM(m_CurX * 2, m_CurY, 2, 1);
	else
		DISPVRAM(m_CurX, m_CurY, 1, 1);

	m_LastChar = ch;
	m_LastFlag = 0;
	m_LastPos  = m_CurX + m_CurY * COLS_MAX;

	if ( ++m_CurX >= m_Margin.right ) {
		if ( IsOptEnable(TO_DECAWM) != 0 ) {
			if ( IsOptEnable(TO_RLGNDW) != 0 ) {
				m_CurX = m_Margin.left;
				m_LastPos -= COLS_MAX;
				ONEINDEX();
			} else {
				m_CurX = m_Margin.right - 1;
				m_DoWarp = TRUE;
			}
		} else 
			m_CurX = m_Margin.right - 1;
	}

	if ( ch != 0 )
		CallReciveChar(ch);
}
void CTextRam::PUT2BYTE(DWORD ch, int md, int at)
{
	if ( m_StsFlag ) {
		md &= CODE_MASK;
		ch |= m_FontTab[md].m_Shift;
		ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, _T("UTF-16BE"), ch);			// Char変換ではUTF-16BEを使用！
		ch = IconvToMsUnicode(ch);
		m_LastChar = ch;
		m_LastFlag = 0;
		m_LastPos  = 0;
		if ( (ch & 0xFFFF0000) != 0 )
			m_StsPara += (WCHAR)(ch >> 16);
		m_StsPara += (WCHAR)ch;
		((CMainFrame *)AfxGetMainWnd())->SetMessageText(CString(m_StsPara));
		return;
	}

	DOWARP();

	CCharCell *vp;
	int dm = GetDm(m_CurY);

	if ( !GetMargin(MARCHK_COLS) ) {
		m_Margin.left  = 0;
		m_Margin.right = m_Cols;
	}

	if ( dm != 0 ) {
		m_Margin.left  /= 2;
		m_Margin.right /= 2;
	}

	if ( (m_CurX + 1) >= m_Margin.right ) {
		PUT1BYTE(0, md);

		dm = GetDm(m_CurY);

		if ( !GetMargin(MARCHK_COLS) ) {
			m_Margin.left  = 0;
			m_Margin.right = m_Cols;
		}

		if ( dm != 0 ) {
			m_Margin.left  /= 2;
			m_Margin.right /= 2;
		}
	}

	vp = GETVRAM(m_CurX, m_CurY);

	vp[0].m_Vram.bank = vp[1].m_Vram.bank = (WORD)md;
	vp[0].m_Vram.eram = vp[1].m_Vram.eram = m_AttNow.eram;
//	vp[0].dm = vp[1].dm = m_AttNow.dm;	no Init
	vp[0].m_Vram.mode = CM_1BYTE;
	vp[1].m_Vram.mode = CM_2BYTE;
	vp[0].m_Vram.attr = vp[1].m_Vram.attr = m_AttNow.attr | at;
	vp[0].m_Vram.font = vp[1].m_Vram.font = m_AttNow.font;
	vp[0].m_Vram.fcol = vp[1].m_Vram.fcol = m_AttNow.fcol;
	vp[0].m_Vram.bcol = vp[1].m_Vram.bcol = m_AttNow.bcol;

	md &= CODE_MASK;
	ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, _T("UTF-16BE"), ch);			// Char変換ではUTF-16BEを使用！
	ch = IconvToMsUnicode(ch);

	if ( ch >= 0x2500 && ch <= 0x257F&& !IsOptEnable(TO_RLDRWLINE) )		// Border Char
		vp[0].m_Vram.attr |= ATT_BORDER;

	vp[0] = (DWORD)ch;
	vp[1] = (DWORD)ch;

	if ( dm != 0 )
		DISPVRAM(m_CurX * 2, m_CurY, 4, 1);
	else
		DISPVRAM(m_CurX, m_CurY, 2, 1);

	m_LastChar = ch;
	m_LastFlag = 0;
	m_LastPos  = m_CurX + m_CurY * COLS_MAX;

	if ( (m_CurX += 2) >= m_Margin.right ) {
		if ( IsOptEnable(TO_DECAWM) != 0 ) {
			if ( IsOptEnable(TO_RLGNDW) != 0 ) {
				m_CurX = m_Margin.left;
				m_LastPos -= COLS_MAX;
				ONEINDEX();
			} else {
				m_CurX = m_Margin.right - 1;
				m_DoWarp = TRUE;
			}
		} else 
			m_CurX = m_Margin.right - 1;
	}

	if ( ch != 0 )
		CallReciveChar(ch);
}
void CTextRam::PUTADD(int x, int y, DWORD ch, int cf)
{
	int n, i;
	CCharCell *vp;

	if ( x < 0 || x >= m_Cols )
		return;
	if ( y >= m_Lines )		// y < 0 OK ?
		return;

	vp = GETVRAM(x, y);
	n = (x < (m_Cols - 1) && IS_1BYTE(vp[0].m_Vram.mode) && IS_2BYTE(vp[1].m_Vram.mode)) ? 2 : 1;

	if ( (cf & UNI_WID) != 0 )
		i = 2;
	else if ( (cf & UNI_AMB) != 0 )
		i = (IsOptEnable(TO_RLUNIAWH) ? 1 : 2);
	else
		i = 1;

	if ( n == 2 || i == n ) {
		*vp += (DWORD)ch;
//		if ( IS_1BYTE(vp->m_Vram.mode) && IS_2BYTE(vp[1].m_Vram.mode) )	not use
			vp[1] = (LPCWSTR)(*vp);
		DISPVRAM(x, y, IS_1BYTE(vp->m_Vram.mode) ? 2 : 1, 1);
	} else {
		CStringW str = *vp;
		LPCWSTR p = str;
		LOCATE(x, y);
		PUT2BYTE(0, SET_UNICODE);
		vp = GETVRAM(m_LastPos % COLS_MAX, m_LastPos / COLS_MAX);
		vp->Empty();
		while ( *p != L'\0' )
			*vp += (DWORD)(*(p++));
		*vp += (DWORD)ch;
		vp[1] = (LPCWSTR)(*vp);
	}
}
void CTextRam::ANSIOPT(int opt, int bit)
{
	if ( bit < 0 || bit > 511 )
		return;

	switch(opt) {
	case 'h':
		EnableOption(bit);
		break;
	case 'l':
		DisableOption(bit);
		break;
	case 'r':
		DisableOption(bit);
		if ( IS_ENABLE(m_Save_AnsiOpt, bit) )
			EnableOption(bit);
		break;
	case 's':
		m_Save_AnsiOpt[bit / 32] &= ~(1 << (bit % 32));
		if ( IsOptEnable(bit) )
			m_Save_AnsiOpt[bit / 32] |= (1 << (bit % 32));
		break;
	}
}
void CTextRam::INSMDCK(int len)
{
	if ( IsOptEnable(TO_ANSIIRM) == 0 )
		return;

	while ( len-- > 0 )
		INSCHAR(FALSE);
}
CTextSave *CTextRam::GETSAVERAM(BOOL bAll)
{
	int n, i, x;
	CCharCell *vp, *wp;
	CTextSave *pNext;
	CTextSave *pSave = new CTextSave;
	CGrapWnd *pWnd;
	
	pSave->m_pCharCell = new CCharCell[m_Cols * m_Lines];

	for ( n = 0 ; n < m_Lines ; n++ ) {
		vp = GETVRAM(0, n);
		wp = pSave->m_pCharCell + n * m_Cols;
		for ( x = 0 ; x < m_Cols ; x++ ) {
			if ( IS_IMAGE(vp->m_Vram.mode) && vp->m_Vram.pack.image.id >= 0 ) {
				for ( i = 0 ; i < pSave->m_GrapIdx.GetSize() ; i++ ) {
					if ( pSave->m_GrapIdx[i] == vp->m_Vram.pack.image.id )
						break;
				}
				if ( i >= pSave->m_GrapIdx.GetSize() )
					pSave->m_GrapIdx.Add(vp->m_Vram.pack.image.id);
			}
			*(wp++) = *(vp++);
		}
	}

	pSave->m_pTextRam = this;

	for ( i = 0 ; i < pSave->m_GrapIdx.GetSize() ; i++ ) {
		if ( (pWnd = GetGrapWnd(pSave->m_GrapIdx[i])) != NULL )
			pWnd->m_Use++;
	}

	pSave->m_bAll = bAll;

	pSave->m_AttNow = m_AttNow;
	pSave->m_AttSpc = m_AttSpc;
	pSave->m_Cols   = m_Cols;
	pSave->m_Lines  = m_Lines;
	pSave->m_CurX   = m_CurX;
	pSave->m_CurY   = m_CurY;

	pSave->m_TopY   = m_TopY;
	pSave->m_BtmY   = m_BtmY;
	pSave->m_LeftX  = m_LeftX;
	pSave->m_RightX = m_RightX;

	pSave->m_DoWarp   = m_DoWarp;
	pSave->m_Exact    = m_Exact;
	pSave->m_LastChar = m_LastChar;
	pSave->m_LastFlag = m_LastFlag;
	pSave->m_LastPos  = m_LastPos;

	memcpy(pSave->m_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));

	pSave->m_BankGL = m_BankGL;
	pSave->m_BankGR = m_BankGR;
	pSave->m_BankSG = m_BankSG;
	memcpy(pSave->m_BankTab, m_BankTab, sizeof(m_BankTab));
	memcpy(pSave->m_TabMap, m_TabMap, sizeof(m_TabMap));

	pSave->m_Save_CurX   = m_Save_CurX;
	pSave->m_Save_CurY   = m_Save_CurY;
	pSave->m_Save_AttNow = m_Save_AttNow;
	pSave->m_Save_AttSpc = m_Save_AttSpc;
	pSave->m_Save_BankGL = m_Save_BankGL;
	pSave->m_Save_BankGR = m_Save_BankGR;
	pSave->m_Save_BankSG = m_Save_BankSG;
	pSave->m_Save_DoWarp = m_Save_DoWarp;
	pSave->m_Save_Decom  = m_Save_Decom;
	memcpy(pSave->m_Save_BankTab, m_Save_BankTab, sizeof(m_Save_BankTab));
	memcpy(pSave->m_Save_AnsiOpt, m_Save_AnsiOpt, sizeof(m_Save_AnsiOpt));
	memcpy(pSave->m_Save_TabMap, m_Save_TabMap, sizeof(m_Save_TabMap));

	if ( pSave->m_bAll ) {
		pSave->m_XtMosPointMode = m_XtMosPointMode;
		pSave->m_XtOptFlag = m_XtOptFlag;

		pSave->m_DispCaret = m_DispCaret;
		pSave->m_TypeCaret = m_TypeCaret;

		//m_TitleStack
		//m_Macro[MACROMAX]
		//m_ModKey[MODKEY_MAX];
		//m_ColTab[EXTCOL_MAX];
		//m_VtLevel
		//m_TermId
		//m_UnitId
		//m_LangMenu
		//m_StsFlag
		//m_StsMode
		//m_StsLed
		//m_StsPara
		//m_KanjiMode SetKanjiMode(mode) ???
	}

	return pSave;
}
void CTextRam::SETSAVERAM(CTextSave *pSave)
{
	int n, x, cx;
	CCharCell *vp, *wp;

	if ( pSave->m_bAll && (pSave->m_Cols != m_Cols || pSave->m_Lines != m_Lines) )
		InitScreen(pSave->m_Cols, pSave->m_Lines);

	m_AttNow = pSave->m_AttNow;
	m_AttSpc = pSave->m_AttSpc;
	if ( (m_CurX = pSave->m_CurX) >= m_Cols )  m_CurX = m_Cols - 1;
	if ( (m_CurY = pSave->m_CurY) >= m_Lines ) m_CurY = m_Lines - 1;
	if ( m_Cols == pSave->m_Cols && m_Lines == pSave->m_Lines ) {
		if ( (m_TopY = pSave->m_TopY) >= m_Lines ) m_TopY = m_Lines - 1;
		if ( (m_BtmY = pSave->m_BtmY) > m_Lines )  m_BtmY = m_Lines;
		if ( (m_LeftX  = pSave->m_LeftX) >= m_Cols ) m_LeftX  = m_Lines - 1;
		if ( (m_RightX = pSave->m_RightX) > m_Cols ) m_RightX = m_Cols;
	}

	m_DoWarp   = pSave->m_DoWarp;
	m_Exact    = pSave->m_Exact;
	m_LastChar = pSave->m_LastChar;
	m_LastFlag = pSave->m_LastFlag;
	m_LastPos  = pSave->m_LastPos;

	memcpy(m_AnsiOpt, pSave->m_AnsiOpt, sizeof(DWORD) * 12);	// 0 - 384

	m_BankGL = pSave->m_BankGL;
	m_BankGR = pSave->m_BankGR;
	m_BankSG = pSave->m_BankSG;
	memcpy(m_BankTab, pSave->m_BankTab, sizeof(m_BankTab));
	memcpy(m_TabMap, pSave->m_TabMap, sizeof(m_TabMap));

	m_Save_CurX   = pSave->m_Save_CurX;
	m_Save_CurY   = pSave->m_Save_CurY;
	m_Save_AttNow = pSave->m_Save_AttNow;
	m_Save_AttSpc = pSave->m_Save_AttSpc;
	m_Save_BankGL = pSave->m_Save_BankGL;
	m_Save_BankGR = pSave->m_Save_BankGR;
	m_Save_BankSG = pSave->m_Save_BankSG;
	m_Save_DoWarp = pSave->m_Save_DoWarp;
	m_Save_Decom  = pSave->m_Save_Decom;
	memcpy(m_Save_BankTab, pSave->m_Save_BankTab, sizeof(m_BankTab));
	memcpy(m_Save_AnsiOpt, pSave->m_Save_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_Save_TabMap, pSave->m_Save_TabMap, sizeof(m_TabMap));

	if ( pSave->m_bAll ) {
		m_XtMosPointMode = pSave->m_XtMosPointMode;
		m_XtOptFlag = pSave->m_XtOptFlag;

		m_DispCaret = pSave->m_DispCaret;
		m_TypeCaret = pSave->m_TypeCaret;

		//m_TitleStack
		//m_Macro[MACROMAX]
		//m_ModKey[MODKEY_MAX];
		//m_ColTab[EXTCOL_MAX];
		//m_VtLevel
		//m_TermId
		//m_UnitId
		//m_LangMenu
		//m_StsFlag
		//m_StsMode
		//m_StsLed
		//m_StsPara
		//m_KanjiMode SetKanjiMode(mode) ???
	}

	cx = (pSave->m_Cols < m_Cols ? pSave->m_Cols : m_Cols);
	for ( n = 0 ; n < pSave->m_Lines && n < m_Lines ; n++ ) {
		vp = GETVRAM(0, n);
		wp = pSave->m_pCharCell + n * pSave->m_Cols;
		for ( x = 0 ; x < cx ; x++ )
			*(vp++) = *(wp++);
	}

	DISPVRAM(0, 0, m_Cols, m_Lines);
}
void CTextRam::SAVERAM()
{
	int n, x;
	CCharCell *vp, *wp;
	CTextSave *pNext;
	CTextSave *pSave;

	if ( m_bTraceActive )
		return;

	pSave = GETSAVERAM(FALSE);

	pSave->m_pNext = m_pTextSave;
	m_pTextSave = pNext = pSave;

	for ( n = 0 ; pSave->m_pNext != NULL ; n++ ) {
		pNext = pSave;
		pSave = pSave->m_pNext;
	}
	if ( n > 16 ) {
		pNext->m_pNext = pSave->m_pNext;
		delete pSave;
	}
}
void CTextRam::LOADRAM()
{
	CTextSave *pSave;

	if ( m_bTraceActive )
		return;

	if ( (pSave = m_pTextSave) == NULL )
		return;

	m_pTextSave = pSave->m_pNext;

	SETSAVERAM(pSave);

	delete pSave;

	m_TraceSaveCount = 0;
}
void CTextRam::PUSHRAM()
{
	CTextSave *pSave;

	if ( m_bTraceActive )
		return;

	SAVERAM();

	if ( (pSave = m_pTextStack) != NULL ) {
		m_pTextStack = pSave->m_pNext;
		pSave->m_pNext = m_pTextSave;
		m_pTextSave = pSave;
		LOADRAM();
	} else {
		ERABOX(0, 0, m_Cols, m_Lines);
		LOCATE(0, 0);
	}

	m_TraceSaveCount = 0;
}
void CTextRam::POPRAM()
{
	CTextSave *pSave;

	if ( m_bTraceActive )
		return;

	SAVERAM();

	if ( (pSave = m_pTextSave) != NULL ) {
		m_pTextSave = pSave->m_pNext;
		pSave->m_pNext = m_pTextStack;
		m_pTextStack = pSave;
	}

	if ( m_pTextSave != NULL ) {
		LOADRAM();
	} else {
		ERABOX(0, 0, m_Cols, m_Lines);
		LOCATE(0, 0);
	}

	m_TraceSaveCount = 0;
}
void CTextRam::SETPAGE(int page)
{
	CTextSave *pSave;

	if ( page < 0 )
		page = 0;
	else if ( page > 100 )
		page = 100;

	if ( m_PageTab.GetSize() <= m_Page )
		m_PageTab.SetSize(m_Page + 1);

	if ( m_PageTab.GetSize() <= page )
		m_PageTab.SetSize(page + 1);

	if ( (pSave = (CTextSave *)m_PageTab[m_Page]) != NULL ) {
		delete pSave;
		m_PageTab[m_Page] = NULL;
	}

	SAVERAM();

	if ( (pSave = m_pTextSave) != NULL ) {
		m_pTextSave = pSave->m_pNext;
		m_PageTab[m_Page] = pSave;
	}

	if ( (pSave = (CTextSave *)m_PageTab[page]) != NULL ) {
		m_PageTab[page] = NULL;
		pSave->m_pNext = m_pTextSave;
		m_pTextSave = pSave;
		LOADRAM();
	} else {
		ERABOX(0, 0, m_Cols, m_Lines);
		LOCATE(0, 0);
	}

	m_Page = page;
}
CTextSave *CTextRam::GETPAGE(int page)
{
	CTextSave *pSave;

	if ( page < 0 )
		page = 0;
	else if ( page > 100 )
		page = 100;

	if ( m_PageTab.GetSize() <= page )
		m_PageTab.SetSize(page + 1);

	if ( (pSave = (CTextSave *)m_PageTab[page]) != NULL )
		return pSave;

	SAVERAM();

	if ( (pSave = m_pTextSave) == NULL )
		return NULL;

	m_pTextSave = pSave->m_pNext;
	m_PageTab[page] = pSave;

	int x, y;
	CCharCell *vp = pSave->m_pCharCell;

	for ( y = 0 ; y < pSave->m_Lines ; y++ ) {
		for ( x = 0 ; x < pSave->m_Cols ; x++ )
			*(vp++) = m_AttSpc;
	}
	pSave->m_CurX = 0;
	pSave->m_CurY = 0;

	return pSave;
}

void CTextRam::COPY(int sp, int sx, int sy, int ex, int ey, int dp, int dx, int dy)
{
	WORD dm;
	int nx, ny, tx, ty;
	CCharCell spc, *np, *tp;
	CTextSave *pSrc, *pDis;

	if ( IsOptEnable(TO_DECOM) ) {
		sx += GetLeftMargin();
		sy += m_TopY;
		ex += GetLeftMargin();
		ey += m_TopY;
		dx += GetLeftMargin();
		dy += m_TopY;
	}

	if      ( sp < 0 )   sp = 0;
	else if ( sp > 100 ) sp = 100;
	if      ( dp < 0 )   dp = 0;
	else if ( dp > 100 ) dp = 100;

	if ( sx < 0 )  sx = 0;
	if ( ex < sx ) ex = sx;
	if ( sy < 0 )  sy = 0;
	if ( ey < sy ) ey = sy;
	if ( dx < 0 )  dx = 0;
	if ( dy < 0 )  dy = 0;

	if ( sp == dp ) {
		if ( sp == m_Page ) {	// Vram to Vram
			spc = m_AttSpc;
			if ( sy > dy || (sy == dy && sx >= dx) ) {
				spc = m_AttSpc;
				for ( ty = sy, ny = dy ; ty <= ey && ny < m_Lines ; ty++, ny++ ) {
					for ( tx = sx, nx = dx ; tx <= ex && nx < m_Cols ; tx++, nx++ ) {
						if ( tx >= m_Cols || ty >= m_Lines )
							tp = &spc;
						else
							tp = GETVRAM(tx, ty);
						np = GETVRAM(nx, ny);
						if ( nx == 0 ) {
							dm = np->m_Vram.zoom;
							*np = *tp;
							np->m_Vram.zoom = dm;
						} else
							*np = *tp;
					}
				}
			} else {
				for ( ty = ey, ny = dy + ey - sy ; ty >= sy && ny >= 0 ; ty--, ny-- ) {
					if ( ny >= m_Lines )
						continue;
					for ( tx = ex, nx = dx + ex - sx ; tx >= sx && nx >= 0 ; tx--, nx-- ) {
						if ( nx >= m_Cols )
							continue;
						if ( tx >= m_Cols || ty >= m_Lines )
							tp = &spc;
						else
							tp = GETVRAM(tx, ty);
						np = GETVRAM(nx, ny);
						if ( nx == 0 ) {
							dm = np->m_Vram.zoom;
							*np = *tp;
							np->m_Vram.zoom = dm;
						} else
							*np = *tp;
					}
				}
			}
//			DISPVRAM(dx, dy, ex - sx + 1, ey - sy + 1);
			DISPRECT(dx, dy, dx + ex - sx + 1, dy + ey - sy + 1);


		} else {				// Page to dup Page
			pSrc = GETPAGE(sp);
			spc = pSrc->m_AttSpc;
			if ( sy > dy || (sy == dy && sx >= dx) ) {
				for ( ty = sy, ny = dy ; ty <= ey && ny < pSrc->m_Lines ; ty++, ny++ ) {
					for ( tx = sx, nx = dx ; tx <= ex && nx < pSrc->m_Cols ; tx++, nx++ ) {
						if ( tx >= pSrc->m_Cols || ty >= pSrc->m_Lines )
							tp = &spc;
						else
							tp = pSrc->m_pCharCell + tx + pSrc->m_Cols * ty;
						np = pSrc->m_pCharCell + nx + pSrc->m_Cols * ny;
						if ( nx == 0 ) {
							dm = np->m_Vram.zoom;
							*np = *tp;
							np->m_Vram.zoom = dm;
						} else
							*np = *tp;
					}
				}
			} else {
				for ( ty = ey, ny = dy + ey - sy ; ty >= sy && ny >= 0 ; ty--, ny-- ) {
					if ( ny >= pSrc->m_Lines )
						continue;
					for ( tx = ex, nx = dx + ex - sx ; tx >= sx && nx >= 0 ; tx--, nx-- ) {
						if ( nx >= pSrc->m_Cols )
							continue;
						if ( tx >= pSrc->m_Cols || ty >= pSrc->m_Lines )
							tp = &spc;
						else
							tp = pSrc->m_pCharCell + tx + pSrc->m_Cols * ty;
						np = pSrc->m_pCharCell + nx + pSrc->m_Cols * ny;
						if ( nx == 0 ) {
							dm = np->m_Vram.zoom;
							*np = *tp;
							np->m_Vram.zoom = dm;
						} else
							*np = *tp;
					}
				}
			}
		}

	} else {
		if ( sp == m_Page )	{			// Vram to Page
			pDis = GETPAGE(dp);
			spc = m_AttSpc;
			for ( ty = sy, ny = dy ; ty <= ey && ny < pDis->m_Lines ; ty++, ny++ ) {
				for ( tx = sx, nx = dx ; tx <= ex && nx < pDis->m_Cols ; tx++, nx++ ) {
					if ( tx >= m_Cols || ty >= m_Lines )
						tp = &spc;
					else
						tp = GETVRAM(tx, ty);
					np = pDis->m_pCharCell + nx + pDis->m_Cols * ny;
					if ( nx == 0 ) {
						dm = np->m_Vram.zoom;
						*np = *tp;
						np->m_Vram.zoom = dm;
					} else
						*np = *tp;
				}
			}

		} else if ( dp == m_Page ) {	// Page to Vram
			pSrc = GETPAGE(sp);
			spc = pSrc->m_AttSpc;
			for ( ty = sy, ny = dy ; ty <= ey && ny < m_Lines ; ty++, ny++ ) {
				for ( tx = sx, nx = dx ; tx <= ex && nx < m_Cols ; tx++, nx++ ) {
					if ( tx >= pSrc->m_Cols || ty >= pSrc->m_Lines )
						tp = &spc;
					else
						tp = pSrc->m_pCharCell + tx + pSrc->m_Cols * ty;
					np = GETVRAM(nx, ny);
					if ( nx == 0 ) {
						dm = np->m_Vram.zoom;
						*np = *tp;
						np->m_Vram.zoom = dm;
					} else
						*np = *tp;
				}
			}
//			DISPVRAM(dx, dy, ex - sx + 1, ey - sy + 1);
			DISPRECT(dx, dy, dx + ex - sx + 1, dy + ey - sy + 1);

		} else {						// Page to diff Page
			pSrc = GETPAGE(sp);
			pDis = GETPAGE(dp);
			spc = pSrc->m_AttSpc;
			for ( ty = sy, ny = dy ; ty <= ey && ny < pDis->m_Lines ; ty++, ny++ ) {
				for ( tx = sx, nx = dx ; tx <= ex && nx < pDis->m_Cols ; tx++, nx++ ) {
					if ( tx >= pSrc->m_Cols || ty >= pSrc->m_Lines )
						tp = &spc;
					else
						tp = pSrc->m_pCharCell + tx + pSrc->m_Cols * ty;
					np = pDis->m_pCharCell + nx + pDis->m_Cols * ny;
					if ( nx == 0 ) {
						dm = np->m_Vram.zoom;
						*np = *tp;
						np->m_Vram.zoom = dm;
					} else
						*np = *tp;
				}
			}
		}
	}
}
void CTextRam::TABSET(int sw)
{
	int n, i;

	switch(sw) {
	case TAB_COLSSET:		// Cols Set
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			m_TabMap[m_CurY + 1][m_CurX / 8 + 1] |= (0x80 >> (m_CurX % 8));
		else							// SINGLE
			m_TabMap[0][m_CurX / 8 + 1] |= (0x80 >> (m_CurX % 8));
		break;
	case TAB_COLSCLR:		// Cols Clear
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			m_TabMap[m_CurY + 1][m_CurX / 8 + 1] &= ~(0x80 >> (m_CurX % 8));
		else							// SINGLE
			m_TabMap[0][m_CurX / 8 + 1] &= ~(0x80 >> (m_CurX % 8));
		break;
	case TAB_COLSALLCLR:	// Cols All Claer
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			memset(&(m_TabMap[m_CurY + 1][1]), 0, COLS_MAX / 8);
		else							// SINGLE
			memset(&(m_TabMap[0][1]), 0, COLS_MAX / 8);
		break;

	case TAB_COLSALLCLRACT:	// Cols All Clear if Active Line
		if ( (m_TabMap[m_CurY + 1][0] & 001) != 0 )
			memset(&(m_TabMap[m_CurY + 1][1]), 0, COLS_MAX / 8);
		break;

	case TAB_LINESET:		// Line Set
		m_TabMap[m_CurY + 1][0] |= 001;
		break;
	case TAB_LINECLR:		// Line Clear
		m_TabMap[m_CurY + 1][0] &= ~001;
		break;
	case TAB_LINEALLCLR:	// Line All Clear
		for ( n = 0 ; n < m_Lines ; n++ )
			m_TabMap[n + 1][0] &= ~001;
		break;

	case TAB_RESET:			// Reset
		m_TabMap[0][0] = 001;
		memset(&(m_TabMap[0][1]), 0, COLS_MAX / 8);
		for ( i = 0 ; i < LINE_MAX ; i += m_DefTab )
			m_TabMap[0][i / 8 + 1] |= (0x80 >> (i % 8));
		for ( n = 1 ; n <= m_Lines ; n++ )
			memcpy(&(m_TabMap[n][0]), &(m_TabMap[0][0]), COLS_MAX / 8 + 1);
		break;

	case TAB_DELINE:		// Delete Line
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			i = GetBottomMargin();
			for ( n = m_CurY + 1 ; n < i ; n++ )
				memcpy(m_TabMap[n], m_TabMap[n + 1], COLS_MAX / 8 + 1);
			//m_TabMap[m_Lines][0] = 0;
			//memset(&(m_TabMap[m_Lines][1]), 0, COLS_MAX / 8);
		}
		break;
	case TAB_INSLINE:		// Insert Line
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			i = GetBottomMargin();
			for ( n = i - 1 ; n > m_CurY ; n-- )
				memcpy(m_TabMap[n + 1], m_TabMap[n], COLS_MAX / 8 + 1);
			//m_TabMap[m_CurY + 1][0] = 0;
			//memset(&(m_TabMap[m_CurY + 1][1]), 0, COLS_MAX / 8);
		}
		break;

	case TAB_ALLCLR:		// Cols Line All Clear
		for ( n = 0 ; n <= m_Lines ; n++ )
			memset(&(m_TabMap[n][0]), 0, COLS_MAX / 8 + 1);
		break;

	case TAB_COLSNEXT:		// Cols Tab Stop
		if ( IsOptEnable(TO_XTMCUS) )
			DOWARP();
		i = (IsOptEnable(TO_ANSITSM) ? (m_CurY + 1) : 0);
		GetMargin(MARCHK_NONE);
		if ( m_CurX >= m_Margin.right )
			m_Margin.right = m_Cols;
		for ( n = m_CurX + 1 ; n < m_Cols ; n++ ) {
			if ( (m_TabMap[i][n / 8 + 1] & (0x80 >> (n % 8))) != 0 && n >= m_Margin.left )
				break;
		}
		if ( n >= m_Margin.right )
			n = m_Margin.right - 1;
		LOCATE(n, m_CurY);
		break;
	case TAB_COLSBACK:		// Cols Back Tab Stop
		i = (IsOptEnable(TO_ANSITSM) ? (m_CurY + 1) : 0);
		GetMargin(MARCHK_NONE);
		if ( m_CurX < m_Margin.left )
			m_Margin.left = 0;
		for ( n = m_CurX - 1 ; n > 0 ; n-- ) {
			if ( (m_TabMap[i][n / 8 + 1] & (0x80 >> (n % 8))) != 0 && n < m_Margin.right )
				break;
		}
		if ( n < m_Margin.left )
			n = m_Margin.left;
		LOCATE(n, m_CurY);
		break;

	case TAB_LINENEXT:		// Line Tab Stop
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_CurY + 1 ; n < m_Lines ; n++ ) {
				if ( (m_TabMap[n + 1][0] & 001) != 0 && n >= m_TopY )
					break;
			}
		} else
			n = m_CurY + 1;
		if ( n >= m_BtmY )
			n = m_BtmY - 1;
		LOCATE(m_CurX, n);
		break;
	case TAB_LINEBACK:		// Line Back Tab Stop
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_CurY - 1 ; n > 0 ; n-- ) {
				if ( (m_TabMap[n + 1][0] & 001) != 0 && n < m_BtmY )
					break;
			}
		} else
			n = m_CurY - 1;
		if ( n < m_TopY )
			n = m_TopY;
		LOCATE(m_CurX, n);
		break;
	}
}
void CTextRam::PUTSTR(LPBYTE lpBuf, int nBufLen)
{
	CBuffer tmp(nBufLen);

	tmp.Apend(lpBuf, nBufLen);
	lpBuf = tmp.GetPtr();

	while ( nBufLen-- > 0 )
		fc_FuncCall(*(lpBuf++));

	m_RetSync = FALSE;
}
int CTextRam::GETCOLIDX(int red, int green, int blue)
{
	int i;
	int idx = 0;
	DWORD min = 0xFFFFFF;	// 255 * 255 * 3 + 255 * 255 * 9 + 255 * 255 = 845325 = 0x000CE60D

	if ( red   > 255 ) red   = 255;
	if ( green > 255 ) green = 255;
	if ( blue  > 255 ) blue  = 255;

	for ( i = 0 ; i < 256 ; i++ ) {
		int r = GetRValue(m_ColTab[i]) - red;
		int g = GetGValue(m_ColTab[i]) - green;
		int b = GetBValue(m_ColTab[i]) - blue;
		DWORD d = r * r * 3 + g * g * 9 + b * b;
		if ( min > d ) {
			idx = i;
			min = d;
		}
	}
	return idx;
}