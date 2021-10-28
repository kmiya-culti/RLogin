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
#include "OptDlg.h"
#include "DefParamDlg.h"
#include "FontParaDlg.h"
#include "StatusDlg.h"
#include "TraceDlg.h"
#include "ColParaDlg.h"

#include <iconv.h>
#include <imm.h>
#include <sys/types.h>
#include <sys/timeb.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#include "UniBlockTab.h"
#include "UniCharTab.h"
#include "UniNomalTab.h"
#include "BorderLine.h"

//////////////////////////////////////////////////////////////////////
// CMemMap

CMemMap::CMemMap(int cols_max, int line_max, int his_max)
{
	static int SeqNum = 0;
	CString mapname;
	SYSTEM_INFO SystemInfo;
	ULONGLONG size;

	mapname.Format(_T("RLoginTextRam_%08x_%d"), (unsigned)GetCurrentThreadId(), SeqNum++);

	for ( m_ColsMax = COLS_MAX ; m_ColsMax < cols_max ; )
		m_ColsMax <<= 1;
	for ( m_LineMax = LINE_MAX ; m_LineMax < line_max ; )
		m_LineMax <<= 1;

	GetSystemInfo(&SystemInfo);
	m_MapPage = (ULONGLONG)(((sizeof(CCharCell) * m_ColsMax * (m_LineMax / 2)) + SystemInfo.dwAllocationGranularity - 1) / SystemInfo.dwAllocationGranularity * SystemInfo.dwAllocationGranularity);
	m_MapSize = m_MapPage * 2;

	size = (((ULONGLONG)sizeof(CCharCell) * (ULONGLONG)m_ColsMax * (ULONGLONG)his_max + m_MapPage - 1) / m_MapPage + 1) * m_MapPage;

	m_hFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(size >> 32), (DWORD)size, mapname);

	if ( m_hFile == NULL || m_hFile == INVALID_HANDLE_VALUE )
		AfxThrowMemoryException();

	m_pMapTop = NULL;

	for ( int n = 0 ; n < MEMMAPCACHE ; n++ ) {
		m_MapData[n].bMap = FALSE;
		m_MapData[n].pNext = m_pMapTop;
		m_pMapTop = &(m_MapData[n]);
	}
}
CMemMap::~CMemMap()
{
	MEMMAPNODE *mp;

	for ( mp = m_pMapTop ;  mp != NULL ; mp = mp->pNext ) {
		if ( mp->bMap )
			UnmapViewOfFile(mp->pAddress);
	}

    CloseHandle(m_hFile);
}
CCharCell *CMemMap::GetMapRam(int x, int y)
{
	int diff;
	ULONGLONG pos, offset;
	MEMMAPNODE *pNext, *pBack;

	pNext = pBack = m_pMapTop;
	pos = (ULONGLONG)sizeof(CCharCell) * ((ULONGLONG)y * m_ColsMax + x);
	offset = (pos / m_MapPage) * m_MapPage;
	diff = (int)(pos - offset);

	ASSERT((m_MapSize - diff) >= (m_ColsMax * sizeof(CCharCell)));

	for ( ; ; ) {
		if ( pNext->bMap && pNext->Offset == offset )
			goto ENDOF;
		if ( pNext->pNext == NULL )
			break;
		pBack = pNext;
		pNext = pNext->pNext;
	}

	if ( pNext->bMap )
		UnmapViewOfFile(pNext->pAddress);

	pNext->bMap = TRUE;
	pNext->Offset = offset;
	pNext->pAddress = MapViewOfFile(m_hFile, FILE_MAP_WRITE, (DWORD)(offset >> 32), (DWORD)offset, (SIZE_T)m_MapSize);

	if ( pNext->pAddress == NULL )
		AfxThrowMemoryException();

	//TRACE("MemMap %d page\n", (int)(offset / m_MapPage));

ENDOF:
	if ( pNext != pBack ) {
		pBack->pNext = pNext->pNext;
		pNext->pNext = m_pMapTop;
		m_pMapTop = pNext;
	}
	return (CCharCell *)((BYTE *)(pNext->pAddress) + diff);
}

//////////////////////////////////////////////////////////////////////
// CCharCell

void CCharCell::operator = (DWORD ch)
{
	if ( IS_IMAGE(m_Vram.mode) )
		return;

	if ( (ch & 0xFFFF0000) != 0 ) {
		m_Data[0] = (WCHAR)(ch >> 16);
		m_Data[1] = (WCHAR)(ch);
		m_Data[2] = L'\0';
	} else {
		m_Data[0] = (WCHAR)(ch);
		m_Data[1] = L'\0';
	}
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

	if ( (ch & 0xFFFF0000) != 0 )
		m_Data[n++] = (WCHAR)(ch >> 16);

	if ( (ch & 0x0000FFFF) != 0 )
		m_Data[n++] = (WCHAR)ch;

	m_Data[n++] = L'\0';

	if ( n > MAXEXTSIZE && (m_Vram.attr & ATT_EXTVRAM) != 0 )
		m_Vram.attr &= ~ATT_EXTVRAM;
}
void CCharCell::operator = (LPCWSTR str)
{
	int n;
	int a = (int)wcslen(str) + 1;

	if ( IS_IMAGE(m_Vram.mode) )
		return;

	for ( n = 0 ; n < a ; n++ ) {
		if ( n >= (MAXCHARSIZE - 1) ) {
			TRACE("CCharCell over %d\n", a);
			m_Data[MAXCHARSIZE - 1] = L'\0';
			break;
		}
		m_Data[n] = *(str++);
	}

	if ( n > MAXEXTSIZE && (m_Vram.attr & ATT_EXTVRAM) != 0 )
		m_Vram.attr &= ~ATT_EXTVRAM;
}
void CCharCell::GetEXTVRAM(EXTVRAM &evram)
{
	if ( evram.eatt != 0 ) {
		m_Vram.attr |= ATT_EXTVRAM;
		SetEatt(evram.eatt);
		if ( (evram.eatt & EATT_BRGBCOL) != 0 )
			SetBrgb(evram.brgb);
		if ( (evram.eatt & EATT_FRGBCOL) != 0 )
			SetFrgb(evram.frgb);
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

	m_Vram.attr &= ~ATT_EXTVRAM;
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
	STDVRAM ram;
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

	m_Vram.attr &= ~ATT_EXTVRAM;
}
void CCharCell::Write(CFile &file)
{
	CBuffer tmp;

	SetBuffer(tmp);
	file.Write(tmp.GetPtr(), tmp.GetSize());
}
void CCharCell::Copy(CCharCell *dis, CCharCell *src, int size)
{
	memcpy(dis, src, sizeof(CCharCell) * size);
}
void CCharCell::Fill(CCharCell *dis, EXTVRAM &vram, int size)
{
	if ( size <= 0 )
		return;

	int a = 1;
	int b = 1;

	*dis = vram;

	while ( size > a ) {
		b *= 2;
		// a = 1,2,4,8,16... 二倍しながらコピーしてコピー回数を減らす
		memcpy(dis + a, dis, sizeof(CCharCell) * (size < b ? (size - a) : a));
		a = b;
	}
}

//////////////////////////////////////////////////////////////////////
// CFontNode

CFontNode::CFontNode()
{
	m_pTransColor = NULL;
	Init();
}
CFontNode::~CFontNode()
{
	if ( m_pTransColor != NULL ) {
		delete [] m_pTransColor;
		m_pTransColor = NULL;
	}
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
	for ( int n = 0 ; n < 16 ; n++ )
		m_FontName[n] = _T("");
	m_MapType     = 0;
	m_UniBlock    = _T("");
	m_OverZero    = _T("");
	m_JpSet       = (-1);

	m_Iso646Name[0] = _T("");
	m_Iso646Name[1] = _T("");

	// ISO646-US/JP				//	US->JP		JP->US
	m_Iso646Tab[0]  = 0x0023;
	m_Iso646Tab[1]  = 0x0024;
	m_Iso646Tab[2]  = 0x0040;
	m_Iso646Tab[3]  = 0x005B;
	m_Iso646Tab[4]  = 0x005C;	//	0x00A5		0x2216
	m_Iso646Tab[5]  = 0x005D;
	m_Iso646Tab[6]  = 0x005E;
	m_Iso646Tab[7]  = 0x0060;
	m_Iso646Tab[8]  = 0x007B;	
	m_Iso646Tab[9]  = 0x007C;
	m_Iso646Tab[10] = 0x007D;
	m_Iso646Tab[11] = 0x007E;	//	0x203E		0x223C

	if ( m_pTransColor != NULL ) {
		delete [] m_pTransColor;
		m_pTransColor = NULL;
	}
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
	stra.Add(m_UniBlock);
	stra.Add(m_Iso646Name[0]);
	stra.Add(m_Iso646Name[1]);
	for ( int n = 0 ; n < 12 ; n++ )
		stra.AddVal(m_Iso646Tab[n]);
	stra.Add(m_OverZero);
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

	m_EntryName   = stra.GetAt(5);
	m_IContName   = stra.GetAt(6);

	m_ZoomW		  = (stra.GetSize() > 7 ? stra.GetVal(7) : m_ZoomH);

	if ( m_IContName.Compare(_T("GB2312-80")) == 0 )	// Debug!!
		m_IContName = _T("GB_2312-80");

	m_JpSet       = CTextRam::JapanCharSet(m_IContName);

	for ( int n = 1 ; n < 16 ; n++ ) {				// 8 - 23
		if ( stra.GetSize() > (7 + n) )
			m_FontName[n] = stra.GetAt(7 + n);
	}

	if ( stra.GetSize() > (7 + 16) )
		m_Quality = stra.GetVal(7 + 16);

	if ( stra.GetSize() > (8 + 16) )
		m_IndexName = stra.GetAt(8 + 16);

	if ( stra.GetSize() > (9 + 16) )
		m_OffsetW = stra.GetVal(9 + 16);

	if ( stra.GetSize() > (10 + 16) )
		m_UniBlock = stra.GetAt(10 + 16);

	if ( stra.GetSize() > (11 + 16) )
		m_Iso646Name[0] = stra.GetAt(11 + 16);
	if ( stra.GetSize() > (12 + 16) )
		m_Iso646Name[1] = stra.GetAt(12 + 16);

	for ( int n = 0 ; n < 12 ; n++ ) {				// 29 - 41
		if ( stra.GetSize() > (13 + 16 + n) )
			m_Iso646Tab[n] = stra.GetVal(13 + 16 + n);
	}

	if ( stra.GetSize() > (13 + 16 + 12) )
		m_OverZero = stra.GetAt(13 + 16 + 12);
	
	m_Init = TRUE;
}
CFontChacheNode *CFontNode::GetFont(int Width, int Height, int Style, int FontNum, class CTextRam *pTextRam)
{
	LPCTSTR FontName;
	int CharSet = m_CharSet;

	if ( m_FontName[FontNum].IsEmpty() ) {
		if ( m_FontName[0].IsEmpty() ) {
			if ( pTextRam->m_DefFontName[FontNum].IsEmpty() )
				FontName = pTextRam->m_DefFontName[0];
			else
				FontName = pTextRam->m_DefFontName[FontNum];

			if ( pTextRam->IsOptEnable(TO_RLNOCHKFONT) )
				CharSet = DEFAULT_CHARSET;
		} else
			FontName = m_FontName[0];
	} else
		FontName = m_FontName[FontNum];
	
	return ((CRLoginApp *)AfxGetApp())->m_FontData.GetFont(FontName, Width * m_ZoomW / 100, Height * m_ZoomH / 100, CharSet, Style, m_Quality);
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
	m_UniBlock  = data.m_UniBlock;
	m_JpSet     = data.m_JpSet;

	for ( int n = 0 ; n < 16 ; n++ )
		m_FontName[n] = data.m_FontName[n];

	m_Iso646Name[0] = data.m_Iso646Name[0];
	m_Iso646Name[1] = data.m_Iso646Name[1];

	for ( int n = 0 ; n < 12 ; n++ )
		m_Iso646Tab[n] = data.m_Iso646Tab[n];

	m_OverZero   = data.m_OverZero;

	return *this;
}
int CFontNode::Compare(CFontNode &data)
{
	if ( m_Shift != data.m_Shift ||
		 m_ZoomW != data.m_ZoomW ||
		 m_ZoomH != data.m_ZoomH ||
		 m_OffsetW != data.m_OffsetW ||
		 m_OffsetH != data.m_OffsetH ||
		 m_CharSet != data.m_CharSet ||
		 m_EntryName.Compare(data.m_EntryName) != 0 ||
		 m_IContName.Compare(data.m_IContName) != 0 ||
		 m_IndexName.Compare(data.m_IndexName) != 0 ||
		 m_Quality != data.m_Quality ||
		 m_UniBlock.Compare(data.m_UniBlock) != 0 ||
		 m_OverZero.Compare(data.m_OverZero) != 0 )
		return 1;

	for ( int n = 0 ; n < 16 ; n++ ) {
		if ( m_FontName[n].Compare(data.m_FontName[n]) != 0 )
			return 1;
	}

	if ( m_Iso646Name[0].Compare(data.m_Iso646Name[0]) != 0 ||
		 m_Iso646Name[1].Compare(data.m_Iso646Name[1]) != 0 ||
		 memcmp(m_Iso646Tab, data.m_Iso646Tab, sizeof(m_Iso646Tab)) != 0 )
		return 1;

	return 0;
}
void CFontNode::SetUserBitmap(int code, int width, int height, CBitmap *pMap, int ofx, int ofy, int asx, int asy, COLORREF bc, COLORREF tc)
{
	CDC oDc, nDc;
	CBitmap *pOld[2];
	BITMAP info;

	if ( (code -= 0x20) < 0 )
		code = 0;
	else if ( code >= USFTCMAX )
		code = USFTCMAX - 1;

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
		nDc.FillSolidRect(0, 0, USFTWMAX * USFTCMAX, USFTHMAX, bc);
		memset(m_UserFontDef, 0, USFTCMAX / 8);
		if ( m_pTransColor != NULL )
			memset(m_pTransColor, 0xFF, sizeof(COLORREF) * USFTCMAX);

	} else
		pOld[1] = nDc.SelectObject(&m_UserFontMap);

	pOld[0] = oDc.SelectObject(pMap);

	if ( tc != (-1) ) {
		if ( m_pTransColor == NULL ) {
			m_pTransColor = new COLORREF[USFTCMAX];
			memset(m_pTransColor, 0xFF, sizeof(COLORREF) * USFTCMAX);
		}
		m_pTransColor[code] = tc;

		nDc.FillSolidRect(USFTWMAX * code, 0, (width < USFTWMAX ? width : USFTWMAX), (height < USFTHMAX ? height : USFTHMAX), tc);
		nDc.TransparentBlt(USFTWMAX * code, 0, (width < USFTWMAX ? width : USFTWMAX), (height < USFTHMAX ? height : USFTHMAX),
			&oDc, ofx * asx / ASP_DIV, ofy * asy / ASP_DIV, width * asx / ASP_DIV, height * asy / ASP_DIV, tc);

	} else {
		nDc.StretchBlt(USFTWMAX * code, 0, (width < USFTWMAX ? width : USFTWMAX), (height < USFTHMAX ? height : USFTHMAX),
			&oDc, ofx * asx / ASP_DIV, ofy * asy / ASP_DIV, width * asx / ASP_DIV, height * asy / ASP_DIV, SRCCOPY);
	}

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
	int x, y;
	CBitmap *pOld;

	if ( (code -= 0x20) < 0 )
		code = 0;
	else if ( code >= USFTCMAX )
		code = USFTCMAX - 1;

	if ( !dc.CreateCompatibleDC(NULL) )
		return;

	if ( m_MapType != FNT_BITMAP_MONO && m_UserFontMap.GetSafeHandle() != NULL )
		m_UserFontMap.DeleteObject();

	if ( m_UserFontMap.GetSafeHandle() == NULL ) {
		if ( !m_UserFontMap.CreateBitmap(USFTWMAX * USFTCMAX, USFTHMAX, 1, 1, NULL) ) {
			dc.DeleteDC();
			return;
		}
		m_MapType = FNT_BITMAP_MONO;
		pOld = dc.SelectObject(&m_UserFontMap);
		dc.FillSolidRect(0, 0, USFTWMAX * USFTCMAX, USFTHMAX, RGB(255, 255, 255));
		memset(m_UserFontDef, 0, USFTCMAX / 8);
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

	if ( m_pTransColor != NULL ) {
		delete [] m_pTransColor;
		m_pTransColor = NULL;
	}
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

		if ( !m_FontMap.CreateBitmap(width * USFTCMAX, height, info.bmPlanes, info.bmBitsPixel, NULL) )
			return FALSE;

		pOld[0] = oDc.SelectObject(&m_UserFontMap);
		pOld[1] = nDc.SelectObject(&m_FontMap);

		if ( m_pTransColor == NULL && (width > m_UserFontWidth || info.bmBitsPixel > 1) )
			nDc.SetStretchBltMode(HALFTONE);

		nDc.StretchBlt(0, 0, width * USFTCMAX, height, &oDc, 0, 0, USFTWMAX * USFTCMAX, USFTHMAX, SRCCOPY);

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
// CUniBlockTab

const CUniBlockTab & CUniBlockTab::operator = (CUniBlockTab &data)
{
	int n;

	m_Data.RemoveAll();
	m_Data.SetSize(data.m_Data.GetSize());

	for ( n = 0 ; n < data.m_Data.GetSize() ; n++ )
		m_Data[n] = data.m_Data[n];

	return *this;
}
int CUniBlockTab::Add(DWORD code, int index, LPCTSTR name)
{
	int n;
	int b = 0;
	int m = (int)m_Data.GetSize() - 1;
	UniBlockNode tmp;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( code == m_Data[n].code ) {
			m_Data[n].index = index;
			return n;
		} else if ( code > m_Data[n].code )
			b = n + 1;
		else
			m = n - 1;
	}

	tmp.code  = code;
	tmp.index = index;
	tmp.name  = name;
	m_Data.InsertAt(b, tmp);
	return b;
}
int CUniBlockTab::Find(DWORD code)
{
	int n;
	int b = 0;
	int m = (int)m_Data.GetSize() - 1;

	while ( b <= m ) {
		n = (b + m) / 2;
		if ( code == m_Data[n].code )
			return m_Data[n].index;
		else if ( code > m_Data[n].code )
			b = n + 1;
		else
			m = n - 1;
	}

	return (b <= 0 ? (-1) : m_Data[b - 1].index);
}
LPCTSTR CUniBlockTab::GetCode(LPCTSTR str, DWORD &code)
{
	DWORD tmp = 0;

	code = UNICODE_MAX;

	while ( *str == _T(' ') || *str == _T('\t') )
		str++;

	if ( _tcsncmp(str, _T("U+"), 2) == 0 || _tcsncmp(str, _T("u+"), 2) == 0 ||  _tcsncmp(str, _T("0x"), 2) == 0 ) {
		for ( str += 2 ; *str != _T('\0') ; str++ ) {
			if ( _tcschr(_T("0123456789"), *str) != NULL )
				tmp = tmp * 16 + (*str - _T('0'));
			else if ( _tcschr(_T("abcdef"), *str) != NULL )
				tmp = tmp * 16 + (*str - _T('a') + 10);
			else if ( _tcschr(_T("ABCDEF"), *str) != NULL )
				tmp = tmp * 16 + (*str - _T('A') + 10);
			else
				break;
		}
		code = tmp;
	}

	while ( *str == _T(' ') || *str == _T('\t') )
		str++;

	return str;
}
void CUniBlockTab::SetBlockCode(LPCTSTR str, int index)
{
	int n;
	int eidx;
	DWORD scode, ecode;

	// U+0100,U+0200-U+02FF
	for ( ; *str != _T('\0') ; str++ ) {
		str = CUniBlockTab::GetCode(str, scode);

		if ( scode == UNICODE_MAX )
			scode = 0;
		Add(scode, index);

		if ( *str == _T('-') ) {
			str = CUniBlockTab::GetCode(++str, ecode);
			ecode += 1;
			eidx = (-1);
			for ( n = 0 ; n < GetSize() ; n++ ) {
				if ( m_Data[n].code < scode )
					eidx = m_Data[n].index;
				else if ( m_Data[n].code > scode && m_Data[n].code < ecode )
					m_Data[n].index = index;
				else if ( m_Data[n].code >= ecode )
					break;
			}
			if ( ecode <= UNICODE_MAX && (n >= GetSize() || m_Data[n].code > ecode) )
				Add(ecode, eidx);

		} else if ( *str != _T(',') )
			break;
	}
}

//////////////////////////////////////////////////////////////////////
// CFontTab

CFontTab::CFontTab()
{
	m_Data = new CFontNode[CODE_MAX];
	m_pSection = _T("FontTab");
	Init();
}
CFontTab::~CFontTab()
{
	delete [] m_Data;
}
void CFontTab::Init()
{
	int n, i;
	CIConv iconv;
	static const CHAR Iso646Char[] = { 0x23, 0x24, 0x40, 0x5B, 0x5C, 0x5D, 0x5E, 0x60, 0x7B, 0x7C, 0x7D, 0x7E };
	static const struct _FontInitTab {
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
		{ _T("VT100-GRAPHIC"),			SET_94,		_T("0"),	'\x80', _T("DEC_SGCS-GR"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("IBM437-GR"),				SET_94,		_T("1"),	'\x80', _T("IBM437"),			DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("DEC_TCS-GR"),				SET_94,		_T("2"),	'\x80', _T("DEC_TCS-GR"),		DEFAULT_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },

		{ _T("ISO646-US"),				SET_94,		_T("@"),	'\x00', _T("ISO646-US"),		ANSI_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("ASCII(ANSI X3.4-1968)"),	SET_94,		_T("B"),	'\x00', _T("ANSI_X3.4-1968"),	ANSI_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0201-Kana"),		SET_94,		_T("I"),	'\x80', _T("JISX0201-1976"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("JIS X 0201-Roman"),		SET_94,		_T("J"),	'\x00', _T("JISX0201-1976"),	SHIFTJIS_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
		{ _T("GB 1988-80"),				SET_94,		_T("T"),	'\x00', _T("GB_1988-80"),		GB2312_CHARSET,		100,	100,	0,	0,	{ _T(""), _T("") } },

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
		{ _T("CYRILLIC (ISO8859-5)"),	SET_96,		_T("L"),	'\x80', _T("ISO8859-5"),		RUSSIAN_CHARSET,	100,	100,	0,	0,	{ _T(""), _T("") } },
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
		m_Data[i].m_JpSet       = CTextRam::JapanCharSet(m_Data[i].m_IContName);
		m_Data[i].m_IndexName   = FontInitTab[n].scs;
		m_Data[i].m_Init        = FALSE;

#if 0
		// libiconvを重視するならiso646tabを初期化したほうが良いが
		// Windowsフォントを重視するならユーザー設定に任せる
		if ( FontInitTab[n].mode <= SET_96 && FontInitTab[n].bank == 0 ) {
			for ( int c = 0 ; c < 12 ; c++ )
				m_Data[i].m_Iso646Tab[c] = iconv.IConvChar(FontInitTab[n].iset, _T("UTF-16BE"), (DWORD)(Iso646Char[c]));
		}
#endif
	}

	InitUniBlock();
}

void CFontTab::InitUniBlock()
{
	int n;

	m_UniBlockTab.RemoveAll();

	for ( n = 0 ; n < UNIBLOCKTABMAX ; n++ )
		m_UniBlockTab.Add(UniBlockTab[n].code, (-1));

	for ( n = 0 ; n < CODE_MAX ; n++ )
		m_UniBlockTab.SetBlockCode(m_Data[n].m_UniBlock, n);

	for ( n = 1 ; n < m_UniBlockTab.GetSize() ; ) {
		if ( m_UniBlockTab[n - 1].index == m_UniBlockTab[n].index )
			m_UniBlockTab.RemoveAt(n);
		else
			n++;
	}

	for ( n = 0 ; n < m_UniBlockTab.GetSize() ; n++ )
		m_UniBlockTab[n].code = CTextRam::UCS4toUCS2(m_UniBlockTab[n].code);
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

		if ( i == SET_UNICODE ) {
			m_Data[i].m_IContName = _T("UTF-16BE");
			m_Data[i].m_JpSet = (-1);
		}
	}

	InitUniBlock();
}
void CFontTab::DiffIndex(CFontTab &orig, CStringIndex &index)
{
	int n, i, a;
	CString str;
	CStringIndex *ip;

	for ( n = 0 ; n < CODE_MAX ; n++ ) {
		if ( m_Data[n].Compare(orig.m_Data[n]) == 0 )
			continue;

		if ( m_Data[n].m_EntryName.IsEmpty() ) {
			index[_T("Del")].Add(n);

		} else {
			str.Format(_T("%d"), n);
			ip = &(index[_T("Add")][str]);

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
			ip->Add(m_Data[n].m_UniBlock);

			ip->Add(m_Data[n].m_Iso646Name[0]);
			ip->Add(m_Data[n].m_Iso646Name[1]);

			a = ip->GetSize();
			ip->SetSize(a + 1);
			for ( i = 0 ; i < 12 ; i++ )
				(*ip)[a].Add(m_Data[n].m_Iso646Tab[i]);

			ip->Add(m_Data[n].m_OverZero);
		}
	}
}
void CFontTab::SetIndex(int mode, CStringIndex &index)
{
	int n, i, a, m;
	int code;
	CString str;
	CStringIndex *ip;
	static const LPCTSTR menbaName[] = { _T("Table"), _T("Add"), _T("Del"), NULL };

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
			ip->Add(m_Data[n].m_UniBlock);

			ip->Add(m_Data[n].m_Iso646Name[0]);
			ip->Add(m_Data[n].m_Iso646Name[1]);

			a = ip->GetSize();
			ip->SetSize(a + 1);
			for ( i = 0 ; i < 12 ; i++ )
				(*ip)[a].Add(m_Data[n].m_Iso646Tab[i]);

			ip->Add(m_Data[n].m_OverZero);
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
				if ( m == 2 ) {	// Del
					code = index[n][i];
					IndexRemove(code);

				} else {	// Table or Add
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
					m_Data[code].m_JpSet     = CTextRam::JapanCharSet(m_Data[code].m_IContName);

					for ( a = 0 ; a < 16 && a < index[n][i][10].GetSize() ; a++ )
						m_Data[code].m_FontName[a] = index[n][i][10][a];

					if ( index[n][i].GetSize() > 11 )
						m_Data[code].m_OffsetW = index[n][i][11];

					if ( index[n][i].GetSize() > 12 )
						m_Data[code].m_UniBlock = index[n][i][12];

					if ( index[n][i].GetSize() > 13 )
						m_Data[code].m_Iso646Name[0] = index[n][i][13];
					if ( index[n][i].GetSize() > 14 )
						m_Data[code].m_Iso646Name[0] = index[n][i][14];

					if ( index[n][i].GetSize() > 15 ) {
						for ( a = 0 ; a < 12 && a < index[n][i][15].GetSize() ; a++ )
							m_Data[code].m_Iso646Tab[a] = index[n][i][15][a];
					}

					if ( index[n][i].GetSize() > 16 )
						m_Data[code].m_OverZero = index[n][i][16];
				}
			}
		}

		InitUniBlock();
	}
}
const CFontTab & CFontTab::operator = (CFontTab &data)
{
	int n;

	for ( n = 0 ; n < CODE_MAX ; n++ )
		m_Data[n] = data.m_Data[n];

	m_UniBlockTab = data.m_UniBlockTab;

	return *this;
}
int CFontTab::Find(LPCTSTR entry)
{
	for ( int n = 0 ; n < CODE_MAX ; n++ ) {
		if ( _tcscmp(entry, m_Data[n].GetEntryName()) == 0 )
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

	if ( n >= (255 - 0x4F) && m_Data[i].m_IndexName.IsEmpty() )
		::AfxMessageBox(_T("Dscs Buffer Overflow Warning"), MB_ICONWARNING);

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
// CTextBitMap

CTextBitMap::CTextBitMap()
{
	m_pSection = _T("TextBitMap");
	Init();
}
CTextBitMap::~CTextBitMap()
{
}
const CTextBitMap & CTextBitMap::operator = (CTextBitMap &data)
{
	m_bEnable = data.m_bEnable;

	m_WidthAlign  = data.m_WidthAlign;
	m_HeightAlign = data.m_HeightAlign;

	m_Text      = data.m_Text;
	m_TextColor = data.m_TextColor;

	memcpy(&m_LogFont, &(data.m_LogFont), sizeof(m_LogFont));

	return *this;
}
const BOOL CTextBitMap::operator == (CTextBitMap &data)
{
	if ( m_bEnable != data.m_bEnable )
		return FALSE;

	if ( m_WidthAlign != data.m_WidthAlign || m_HeightAlign != data.m_HeightAlign )
		return FALSE;

	if ( m_Text.Compare(data.m_Text) != 0 || m_TextColor != data.m_TextColor )
		return FALSE;

	if ( m_LogFont.lfHeight != data.m_LogFont.lfHeight ||
		 m_LogFont.lfWeight != data.m_LogFont.lfWeight ||
		 m_LogFont.lfItalic != data.m_LogFont.lfItalic )
		return FALSE;

	if ( _tcscmp(m_LogFont.lfFaceName, data.m_LogFont.lfFaceName) != 0 )
		return FALSE;

	return TRUE;
}
void CTextBitMap::Init()
{
	m_bEnable = FALSE;

	m_WidthAlign  = DT_CENTER;		// DT_LEFT DT_CENTER DT_RIGHT
	m_HeightAlign = DT_VCENTER;		// DT_TOP DT_VCENTER DT_BOTTOM

	m_Text = _T("%E\\r\\n%S");
	m_TextColor = RGB(128, 128, 128);

	ZeroMemory(&m_LogFont, sizeof(m_LogFont));
	m_LogFont.lfHeight = 30;
}
void CTextBitMap::SetArray(CStringArrayExt &stra)
{
	stra.RemoveAll();

	stra.AddVal(m_bEnable);

	stra.AddVal(m_WidthAlign);
	stra.AddVal(m_HeightAlign);

	stra.Add(m_Text);
	stra.AddVal(m_TextColor);

	stra.AddVal(m_LogFont.lfHeight);
	stra.AddVal(m_LogFont.lfWeight);
	stra.AddVal(m_LogFont.lfItalic);
	stra.Add(m_LogFont.lfFaceName);
}
void CTextBitMap::GetArray(CStringArrayExt &stra)
{
	Init();

	//if ( stra.GetSize() < 9  )
	//	return;

	if ( stra.GetSize() > 0  )
		m_bEnable = stra.GetVal(0);

	if ( stra.GetSize() > 1  )
		m_WidthAlign = stra.GetVal(1);
	if ( stra.GetSize() > 2  )
		m_HeightAlign = stra.GetVal(2);

	if ( stra.GetSize() > 3  )
		m_Text = stra.GetAt(3);
	if ( stra.GetSize() > 4  )
		m_TextColor = stra.GetVal(4);

	if ( stra.GetSize() > 5 )
		m_LogFont.lfHeight = stra.GetVal(5);
	if ( stra.GetSize() > 6  )
		m_LogFont.lfWeight = stra.GetVal(6);
	if ( stra.GetSize() > 7  )
		m_LogFont.lfItalic = stra.GetVal(7);
	if ( stra.GetSize() > 8  )
		_tcsncpy(m_LogFont.lfFaceName, stra.GetAt(8), sizeof(m_LogFont.lfFaceName) / sizeof(TCHAR));
}
void CTextBitMap::DiffIndex(CTextBitMap &orig, CStringIndex &index)
{
	if ( m_bEnable != orig.m_bEnable )
		index[_T("Enable")]  = m_bEnable;

	if ( m_WidthAlign != orig.m_WidthAlign )
		index[_T("WidthAlign")]  = m_WidthAlign;
	if ( m_HeightAlign != orig.m_HeightAlign )
		index[_T("HeightAlign")] = m_HeightAlign;

	if ( m_Text.Compare(orig.m_Text) != 0 )
		index[_T("String")]   = m_Text;

	if ( m_TextColor != orig.m_TextColor ) {
		index[_T("Color")].Add(GetRValue(m_TextColor));
		index[_T("Color")].Add(GetGValue(m_TextColor));
		index[_T("Color")].Add(GetBValue(m_TextColor));
	}

	if ( m_LogFont.lfHeight != orig.m_LogFont.lfHeight )
		index[_T("Height")]   = m_LogFont.lfHeight;
	if ( m_LogFont.lfWeight != orig.m_LogFont.lfWeight )
		index[_T("Weight")]   = m_LogFont.lfWeight;
	if ( m_LogFont.lfItalic != orig.m_LogFont.lfItalic )
		index[_T("Italic")]   = m_LogFont.lfItalic;
	if ( _tcscmp(m_LogFont.lfFaceName, orig.m_LogFont.lfFaceName) != 0 )
		index[_T("FaceName")] = m_LogFont.lfFaceName;

}
void CTextBitMap::SetIndex(int mode, CStringIndex &index)
{
	int n;

	if ( mode ) {		// Write
		index[_T("Enable")]  = m_bEnable;

		index[_T("WidthAlign")]  = m_WidthAlign;
		index[_T("HeightAlign")] = m_HeightAlign;

		index[_T("String")]   = m_Text;
		index[_T("Color")].Add(GetRValue(m_TextColor));
		index[_T("Color")].Add(GetGValue(m_TextColor));
		index[_T("Color")].Add(GetBValue(m_TextColor));

		index[_T("Height")]   = m_LogFont.lfHeight;
		index[_T("Weight")]   = m_LogFont.lfWeight;
		index[_T("Italic")]   = m_LogFont.lfItalic;
		index[_T("FaceName")] = m_LogFont.lfFaceName;

	} else {			// Read
		if ( (n = index.Find(_T("Enable"))) >= 0 )
			m_bEnable = index[n];

		if ( (n = index.Find(_T("WidthAlign"))) >= 0 )
			m_WidthAlign = index[n];
		if ( (n = index.Find(_T("HeightAlign"))) >= 0 )
			m_HeightAlign = index[n];

		if ( (n = index.Find(_T("String"))) >= 0 )
			m_Text = index[n];
		if ( (n = index.Find(_T("Color"))) >= 0 && index[n].GetSize() >= 3 )
			m_TextColor = RGB(index[n][0], index[n][1], index[n][2]);

		if ( (n = index.Find(_T("Height"))) >= 0 )
			m_LogFont.lfHeight = index[n];
		if ( (n = index.Find(_T("Weight"))) >= 0 )
			m_LogFont.lfWeight = index[n];
		if ( (n = index.Find(_T("Italic"))) >= 0 )
			m_LogFont.lfItalic = index[n];
		if ( (n = index.Find(_T("FaceName"))) >= 0 )
			_tcsncpy(m_LogFont.lfFaceName, index[n], sizeof(m_LogFont.lfFaceName) / sizeof(TCHAR));
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
	m_TabMap = NULL;
	m_SaveParam.m_TabMap = NULL;
	m_StsParam.m_TabMap = NULL;
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

	if ( m_TabMap != NULL )
		delete [] m_TabMap;
	if ( m_SaveParam.m_TabMap != NULL )
		delete [] m_SaveParam.m_TabMap;
	if ( m_StsParam.m_TabMap != NULL )
		delete [] m_StsParam.m_TabMap;
}

//////////////////////////////////////////////////////////////////////
// CTabFlag

CTabFlag::CTabFlag()
{
	m_Param.Cols = COLS_MAX;
	m_Param.Line = LINE_MAX;
	m_DefTabSize = DEF_TAB;

	m_pData = m_pSingleColsFlag = m_pMultiColsFlag = m_pLineFlag = NULL;
}
CTabFlag::~CTabFlag()
{
	if ( m_pData != NULL )
		delete [] m_pData;
}
void CTabFlag::InitTab()
{
	m_Param.Element = (m_Param.Cols + 7) / 8;
	m_Param.Size = m_Param.Line + m_Param.Element + m_Param.Line * m_Param.Element;
	m_pData = new BYTE[m_Param.Size];

	m_pLineFlag = m_pData;
	m_pSingleColsFlag = m_pLineFlag + m_Param.Line;
	m_pMultiColsFlag = m_pSingleColsFlag + m_Param.Element;
	memset(m_pData, 0, m_Param.Size);

	memset(m_pLineFlag, 001, m_Param.Line);

	for ( int x = 0 ; x < m_Param.Cols ; x += m_DefTabSize )
		m_pSingleColsFlag[x / 8] |= (1 << (x % 8));

	for ( int y = 0 ; y < m_Param.Line ; y++ )
		memcpy(m_pMultiColsFlag + y * m_Param.Element, m_pSingleColsFlag, m_Param.Element);
}
void CTabFlag::ResetAll(int cols_max, int line_max, int def_tab)
{
	m_Param.Cols = cols_max;
	m_Param.Line = line_max;
	m_DefTabSize = def_tab;

	if ( m_pData != NULL )
		delete [] m_pData;

	m_pData = m_pSingleColsFlag = m_pMultiColsFlag = m_pLineFlag = NULL;
}
void CTabFlag::SizeCheck(int cols_max, int line_max, int def_tab)
{
	if ( m_pData == NULL ) {
		m_Param.Cols = cols_max;
		m_Param.Line = line_max;
		m_DefTabSize = def_tab;
		return;
	}

	if ( m_Param.Cols >= cols_max && m_Param.Line >= line_max )
		return;

	struct _TabFlagParam newParam;
	newParam.Cols = cols_max;
	newParam.Line = line_max;
	newParam.Element = (newParam.Cols + 7) / 8;
	newParam.Size = newParam.Line + newParam.Element + newParam.Line * newParam.Element;
	BYTE *pNewData = new BYTE[newParam.Size];
	BYTE *pNewLineFlag = pNewData;
	BYTE *pNewSingleColsFlag = pNewLineFlag + newParam.Line;
	BYTE *pNewMultiColsFlag = pNewSingleColsFlag + newParam.Element;

	memset(pNewData, 0, newParam.Size);

	memcpy(pNewLineFlag, m_pLineFlag, m_Param.Line);
	for ( int y = m_Param.Line ; y < newParam.Line ; y++ )
		pNewLineFlag[y] = 001;

	memcpy(pNewSingleColsFlag, m_pSingleColsFlag, m_Param.Element);
	for ( int x = (m_Param.Cols + def_tab - 1) / def_tab * def_tab ;  x < newParam.Cols ; x += def_tab )
		pNewSingleColsFlag[x / 8] |= (1 << (x % 8));

	for ( int y = 0 ; y < m_Param.Line ; y++ ) {
		memcpy(pNewMultiColsFlag + y * newParam.Element, m_pMultiColsFlag + y * m_Param.Element, m_Param.Element);
		for ( int x = (m_Param.Cols + def_tab - 1) / def_tab * def_tab ;  x < newParam.Cols ; x += def_tab )
			pNewMultiColsFlag[x / 8 + y * newParam.Element] |= (1 << (x % 8));
	}
	if ( m_Param.Line < newParam.Line ) {
		for ( int x = 0 ;  x < newParam.Cols ; x += def_tab )
			pNewMultiColsFlag[x / 8 + m_Param.Line * newParam.Element] |= (1 << (x % 8));
		for ( int y = m_Param.Line + 1 ; y < newParam.Line ; y++ )
			memcpy(pNewMultiColsFlag + y * newParam.Element, pNewMultiColsFlag + m_Param.Line * newParam.Element, newParam.Element);
	}

	delete [] m_pData;

	m_Param = newParam;
	m_pData = pNewData;
	m_pLineFlag = pNewLineFlag;
	m_pSingleColsFlag = pNewSingleColsFlag;
	m_pMultiColsFlag = pNewMultiColsFlag;
	m_DefTabSize = def_tab;
}

BOOL CTabFlag::IsSingleColsFlag(int x)
{
	if ( m_pSingleColsFlag != NULL )
		return ((m_pSingleColsFlag[x / 8] & (1 << (x % 8))) != 0 ? TRUE : FALSE);

	return ((x % m_DefTabSize) == 0 ? TRUE : FALSE);
}
BOOL CTabFlag::IsMultiColsFlag(int x, int y)
{
	if ( m_pMultiColsFlag != NULL )
		return ((m_pMultiColsFlag[x / 8 + y * m_Param.Element] & (1 << (x % 8))) != 0 ? TRUE : FALSE);

	return ((x % m_DefTabSize) == 0 ? TRUE : FALSE);
}
BOOL CTabFlag::IsLineFlag(int y)
{
	if ( m_pLineFlag != NULL )
		return m_pLineFlag[y];

	return TRUE;
}

void CTabFlag::SetSingleColsFlag(int x)
{
	if ( m_pSingleColsFlag == NULL )
		InitTab();

	m_pSingleColsFlag[x / 8] |= (1 << (x % 8));
}
void CTabFlag::ClrSingleColsFlag(int x)
{
	if ( m_pSingleColsFlag == NULL )
		InitTab();

	m_pSingleColsFlag[x / 8] &= ~(1 << (x % 8));
}
void CTabFlag::SetMultiColsFlag(int x, int y)
{
	if ( m_pMultiColsFlag == NULL )
		InitTab();

	m_pMultiColsFlag[x / 8 + y * m_Param.Element] |= (1 << (x % 8));
}
void CTabFlag::ClrMultiColsFlag(int x, int y)
{
	if ( m_pMultiColsFlag == NULL )
		InitTab();

	m_pMultiColsFlag[x / 8 + y * m_Param.Element] &= ~(1 << (x % 8));
}
void CTabFlag::SetLineflag(int y)
{
	if ( m_pLineFlag == NULL )
		InitTab();

	m_pLineFlag[y] = 001;
}
void CTabFlag::ClrLineflag(int y)
{
	if ( m_pLineFlag == NULL )
		InitTab();

	m_pLineFlag[y] = 000;
}

void CTabFlag::AllClrSingle()
{ 
	if ( m_pSingleColsFlag == NULL )
		InitTab();

	memset(m_pSingleColsFlag, 0, m_Param.Element);
}
void CTabFlag::AllClrMulti()
{
	if ( m_pMultiColsFlag == NULL )
		InitTab();

	memset(m_pMultiColsFlag, 0, m_Param.Line * m_Param.Element);
}
void CTabFlag::LineClrMulti(int y)
{ 
	if ( m_pMultiColsFlag == NULL )
		InitTab();

	memset(m_pMultiColsFlag + y * m_Param.Element, 0, m_Param.Element);
}
void CTabFlag::AllClrLine()
{
	if ( m_pLineFlag == NULL )
		InitTab();

	memset(m_pLineFlag, 0, m_Param.Line);
}
void CTabFlag::CopyLine(int dy, int sy)
{
	if ( m_pMultiColsFlag == NULL )
		InitTab();

	m_pLineFlag[dy] = m_pLineFlag[sy];
	memcpy(m_pMultiColsFlag + dy * m_Param.Element, m_pMultiColsFlag + sy * m_Param.Element, m_Param.Element);
}

void *CTabFlag::SaveFlag(void *pData)
{
	if ( m_pData == NULL )
		return NULL;

	if ( pData != NULL && memcmp(&m_Param, pData, sizeof(m_Param)) != 0 ) {
		delete [] pData;
		pData = NULL;
	}

	if ( pData == NULL ) {
		pData = (void *)(new BYTE[sizeof(m_Param) + m_Param.Size]);
		memcpy(pData, &m_Param, sizeof(m_Param));
	}

	memcpy((BYTE *)pData + sizeof(m_Param), m_pData, m_Param.Size);

	return pData;
}
void CTabFlag::LoadFlag(void *pData)
{
	if ( pData == NULL )
		return;

	if ( m_pData == NULL )
		InitTab();

	if ( memcmp(&m_Param, pData, sizeof(m_Param)) == 0 )
		memcpy(m_pData, (BYTE *)pData + sizeof(m_Param), m_Param.Size);

	else {
		struct _TabFlagParam *pParam = (struct _TabFlagParam *)pData;
		BYTE *pLineFlag = (BYTE *)pData + sizeof(struct _TabFlagParam);
		BYTE *pSingleColsFlag = pLineFlag + pParam->Line;
		BYTE *pMultiColsFlag = pSingleColsFlag + pParam->Element;

		memcpy(m_pLineFlag, pLineFlag, m_Param.Line < pParam->Line ? m_Param.Line : pParam->Line);
		memcpy(m_pSingleColsFlag, pSingleColsFlag, m_Param.Element < pParam->Element ? m_Param.Element : pParam->Element);

		for ( int y = 0 ; y < m_Param.Line && y < pParam->Line ; y++ )
			memcpy(m_pMultiColsFlag + y * m_Param.Element, pMultiColsFlag + y * pParam->Element, m_Param.Element < pParam->Element ? m_Param.Element : pParam->Element);
	}
}
void *CTabFlag::CopyFlag(void *pData)
{
	if ( pData == NULL )
		return NULL;

	struct _TabFlagParam *pParam = (struct _TabFlagParam *)pData;
	void *pTemp = (void *)(new BYTE[sizeof(struct _TabFlagParam) + pParam->Size]);

	memcpy(pTemp, pData, sizeof(struct _TabFlagParam) + pParam->Size);
	return pTemp;
}

//////////////////////////////////////////////////////////////////////
// CTextRam

CTextRam::CTextRam()
{
	m_pDocument = NULL;
	m_pServerEntry = NULL;

	m_ColTab = new COLORREF[EXTCOL_MAX + EXTCOL_MATRIX];
	m_Kan_Buf = new BYTE[KANBUFMAX];
	m_Macro = new CBuffer[MACROMAX];
	m_SaveParam.m_TabMap = NULL;
	m_StsParam.m_TabMap = NULL;

	m_bOpen = FALSE;
	m_pMemMap = NULL;
	m_Cols = m_ColsMax = 80;
	m_LineUpdate = 0;
	m_Lines = 25;
	m_Page = 0;
	m_TopY = 0;
	m_BtmY = m_Lines;
	m_LeftX  = 0;
	m_RightX = m_Cols;
	m_bReSize = FALSE;
	m_HisMax = 100;
	m_HisPos = 0;
	m_HisLen = 0;
	m_HisUse = 0;
	m_HisAbs = 0;
	m_pTextSave = m_pTextStack = NULL;
	m_AltIdx = 0;
	m_pAltSave[0] = m_pAltSave[1] = NULL;
	m_DispCaret = TRUE;
	m_DefTypeCaret = 1;
	m_TypeCaret = m_DefTypeCaret;
	m_DefCaretColor = RGB(255, 255, 255);
	m_ImeTypeCaret = 0;
	m_ImeCaretColor = RGB(31, 255, 127);
	m_CaretColor = m_DefCaretColor;
	m_UpdateRect.SetRectEmpty();
	m_UpdateFlag = FALSE;
	m_UpdateClock = clock();
	m_DelayUSecChar = DELAY_CHAR_USEC;
	m_DelayMSecLine = DELAY_LINE_MSEC;
	m_DelayMSecRecv = DELAY_RECV_MSEC;
	m_DelayMSecCrLf = 0;
	m_HisFile.Empty();
	m_LogFile.Empty();
	m_SshKeepAlive = 300;
	m_TelKeepAlive = 300;
	m_RecvCrLf = m_SendCrLf = 0;
	m_LastChar = 0;
	m_LastFlag = 0;
	m_LastPos.SetPoint(0, 0);
	m_LastCur.SetPoint(0, 0);
	m_LastSize = CM_ASCII;
	m_LastAttr = 0;
	m_LastStr.Empty();
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
	m_MediaCopyMode = MEDIACOPY_NONE;
	m_StsMode = 0;
	m_StsLine = 0;
	m_StsLed  = 0;
	m_DefTermPara[TERMPARA_VTLEVEL] = m_VtLevel = DEFVAL_VTLEVEL;
	m_DefTermPara[TERMPARA_FIRMVER] = m_FirmVer = DEFVAL_FIRMVER;
	m_DefTermPara[TERMPARA_TERMID]  = m_TermId  = DEFVAL_TERMID;
	m_DefTermPara[TERMPARA_UNITID]  = m_UnitId  = DEFVAL_UNITID;
	m_DefTermPara[TERMPARA_KEYBID]  = m_KeybId  = DEFVAL_KEYBID;
	m_LangMenu = 0;
	SetRetChar(FALSE);
	m_ClipFlag = 0;
	m_ClipCrLf = OSCCLIP_LF;
	m_ImageIndex = 1024;
	m_bOscActive = FALSE;
	m_bIntTimer = FALSE;
	m_bRtoL = FALSE;
	m_bJoint = FALSE;
	m_XtOptFlag = 0;
	m_XtMosPointMode = 0;
	m_TitleStack.RemoveAll();
	m_FrameCheck = FALSE;
	m_ScrnOffset.SetRect(0, 0, 0, 0);
	m_LogTimeFlag = TRUE;
	m_pCallPoint = &CTextRam::fc_FuncCall;
	m_TraceLogMode = 0;
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
	m_LogMode = LOGMOD_RAW;
	m_LogCurY = (-1);
	m_CharWidth = 8;
	m_CharHeight = 16;
	m_MarkColor = RGB(255, 255, 0);
	m_RtfMode = 0;
	m_iTerm2Mark = 0;
	m_pCmdHisWnd = NULL;

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
	m_pSixelColor = NULL;
	m_pSixelAlpha = NULL;

	m_FixVersion = 0;
	m_SleepMax = VIEW_SLEEP_MAX;

	for ( int n = 0 ; n < MODKEY_MAX ; n++ )
		m_DefModKey[n] = (-1);
	memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
	memset(m_ModKeyTab, 0xFF, sizeof(m_ModKeyTab));

	SaveParam(&m_SaveParam);

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

	if ( m_pMemMap != NULL )
		delete m_pMemMap;

	while ( (pSave = m_pTextSave) != NULL ) {
		m_pTextSave = pSave->m_pNext;
		delete pSave;
	}

	while ( (pSave = m_pTextStack) != NULL ) {
		m_pTextStack = pSave->m_pNext;
		delete pSave;
	}
	
	for ( int n = (int)m_PageTab.GetSize() - 1 ; n >= 0 ; n-- ) {
		if ( (pSave = (CTextSave *)m_PageTab[n]) != NULL ) {
			m_PageTab[n] = NULL;
			delete pSave;
		}
	}

	for ( int n = 0 ; n < 2 ; n++ ) {
		if ( m_pAltSave[n] != NULL )
			delete m_pAltSave[n];
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

	if ( m_pWorkGrapWnd != NULL )
		m_pWorkGrapWnd->DestroyWindow();

	while ( pGrapListType != NULL )
		pGrapListType->DestroyWindow();

	if ( m_pSixelColor != NULL )
		delete [] m_pSixelColor;

	if ( m_pSixelAlpha != NULL )
		delete [] m_pSixelAlpha;

	SetTraceLog(FALSE);

	delete [] m_ColTab;
	delete [] m_Kan_Buf;
	delete [] m_Macro;

	if ( m_SaveParam.m_TabMap != NULL )
		delete [] m_SaveParam.m_TabMap;
	if ( m_StsParam.m_TabMap != NULL )
		delete [] m_StsParam.m_TabMap;

	CMDHIS *pCmdHis;
	while ( !m_CommandHistory.IsEmpty() && (pCmdHis = m_CommandHistory.RemoveHead()) != NULL )
		delete pCmdHis;
}

//////////////////////////////////////////////////////////////////////
// Window Function
//////////////////////////////////////////////////////////////////////

void CTextRam::InitText(int Width, int Height)
{
	int n;
	int oldCurX, oldCurY;
	int oldCols, oldLines;
	int charWidth, charHeight;
	int newCols, newLines;
	int newColsMax, newHisMax;
	BOOL bHisInit = FALSE;

	if ( !m_bOpen )
		return;

	if ( IsOptEnable(TO_RLFONT) ) {
		if ( IsOptEnable(TO_RLHIDPIFSZ) )
			charHeight = MulDiv(m_DefFontSize, SCREEN_DPI_Y, DEFAULT_DPI_Y);
		else
			charHeight = m_DefFontSize;

		if ( (charWidth = charHeight * 10 / m_DefFontHw) <= 0 )
			charWidth = 1;

		newCols  = Width  / charWidth;
		newLines = Height / charHeight;

	} else {
		newCols = m_DefCols[0];

		if ( (charWidth = Width / newCols) <= 0 ) {
			charWidth = 1;
			newCols = Width;
		}

		if ( (charHeight = charWidth * m_DefFontHw / 10) <= 0 )
			charHeight = 1;

		if ( IsOptValue(TO_DECCOLM, 1) == 1 ) {
			newCols = m_DefCols[1];

			if ( (charWidth = Width / newCols) <= 0 ) {
				charWidth = 1;
				newCols = Width;
			}
		}

		newLines = Height / charHeight;

		if ( m_bReSize ) {
			if ( newCols == m_ReqCols && newLines == m_ReqLines )
				m_bReSize = FALSE;

			newCols  = m_ReqCols;
			newLines = m_ReqLines;

			charWidth  = Width  / newCols;
			charHeight = Height / newLines;
		}

		if ( charWidth <= 0 ) {
			charWidth = 1;
			newCols = Width / charWidth;
		}

		if ( charHeight <= 0 ) {
			charHeight = 1;
			newLines = Height / charHeight;
		}
	}

	if ( newCols < 4 )
		newCols = 4;

	if ( newLines < 2 )
		newLines = 2;

	if ( (newHisMax = m_DefHisMax)  < (newLines * 5) )
		newHisMax = newLines * 5;
	else if ( newHisMax > HIS_MAX )
		newHisMax = HIS_MAX;

	m_CharWidth  = charWidth;
	m_CharHeight = charHeight;

/************************************************************

						m_Cols		m_ColsMax		COLS_MAX
			0		0---+-----------+---------------+
			|    
			+-------+m_HisLen			0
			|		|					|
	m_HisPos+-------+-----------0-------+m_HisAbs
			|		|			|
	m_HisMax+-------0-----------+m_Lines

**************************************************************/

	if ( newHisMax == m_HisMax && m_pMemMap != NULL && m_pMemMap->m_ColsMax >= newCols && m_pMemMap->m_LineMax >= newLines ) {
		if ( newCols == m_Cols && newLines == m_Lines )
			return;

		oldCurX    = m_CurX;
		oldCurY    = m_CurY;
		oldCols    = m_Cols;
		oldLines   = m_Lines;
		newColsMax = (m_ColsMax >= newCols && m_LineUpdate < m_HisMax ? m_ColsMax : newCols);

		if ( newColsMax > m_ColsMax ) {
			for ( n = 1 ; n <= m_HisLen ; n++ )
				CCharCell::Fill(GETVRAM(m_ColsMax, oldLines - n), m_DefAtt, newColsMax - m_ColsMax);
		}

	} else {
		newColsMax = (m_ColsMax >= newCols && m_LineUpdate < m_HisMax ? m_ColsMax : newCols);
		CMemMap *pNewMap = new CMemMap(newColsMax, newLines, newHisMax);

		if ( m_pMemMap != NULL ) {
			oldCurX  = (m_ColsMax < newColsMax ? m_ColsMax : newColsMax);
			oldCurY  = (m_HisLen < newHisMax ? m_HisLen : newHisMax);
			oldCols  = m_Cols;
			oldLines = m_Lines;

			for ( n = 1 ; n <= oldCurY ; n++ ) {
				CCharCell::Copy(pNewMap->GetMapRam(0, newHisMax - n), GETVRAM(0, oldLines - n), oldCurX);
				if ( oldCurX < newColsMax )
					CCharCell::Fill(pNewMap->GetMapRam(oldCurX, newHisMax - n), m_DefAtt, newColsMax - oldCurX);
			}

			delete m_pMemMap;
			m_pMemMap = NULL;

			m_HisLen = oldCurY;
			oldCurX  = m_CurX;
			oldCurY  = m_CurY;

		} else {
			for ( n = 1 ; n <= newLines ; n++ )
				CCharCell::Fill(pNewMap->GetMapRam(0, newHisMax - n), m_DefAtt, newColsMax);

			oldCurX  = 0;
			oldCurY  = 0;
			oldCols  = newCols;
			oldLines = newLines;

			m_TopY   = 0;
			m_BtmY   = newLines;
			m_LeftX  = 0;
			m_RightX = newCols;

			m_HisLen = newLines;

			bHisInit = TRUE;
		}

		m_pMemMap = pNewMap;

		m_HisMax = newHisMax;
		m_HisPos = newHisMax - oldLines;	// 後で計算する場合に古い位置である必要あり
	}

	m_Cols       = newCols;
	m_Lines      = newLines;
	m_ColsMax    = newColsMax;
	m_LineUpdate = 0;

	m_TabFlag.SizeCheck(m_pMemMap->m_ColsMax, m_pMemMap->m_LineMax, m_DefTab);

	if ( oldLines < newLines ) {
		int add = 0;

		if ( (n = m_HisLen - oldLines) >= (newLines - oldLines) )
			n = newLines - oldLines;
		else
			add = (newLines - oldLines - n);

		oldCurY  += n;
		m_HisPos -= n;
		m_HisLen += add;

		for ( n = 1 ; n <= add ; n++ )
			CCharCell::Fill(GETVRAM(0, newLines - n), m_DefAtt, newColsMax);

	} else if ( oldLines > newLines ) {
		int top = 0;
		int bottom = 0;

		for ( n = oldLines ; n > 0 ; n-- ) {
			int x;
			CCharCell *pCe = GETVRAM(0, n - 1);
			for ( x = 0 ; x < oldCols ; x++ ) {
				if ( !pCe[x].IsEmpty() )
					break;
			}
			if ( x < oldCols )
				break;
		}
		// カーソルより上
		if ( n < oldCurY )
			n = oldCurY;

		// ブランクより上でカーソルより下
		if ( (n -= newLines) < 0 )
			n = 0;
		else if ( n > oldCurY )
			n = oldCurY;

		top = n;
		bottom = (oldLines - newLines - n);

		oldCurY  -= top;
		m_HisPos += top;
		m_HisLen -= bottom;
	}
		
	if ( m_HisLen < m_Lines )
		m_HisLen = m_Lines;
	else if ( m_HisLen >= m_HisMax )
		m_HisLen = m_HisMax - 1;

	RESET(RESET_CURSOR);

	if ( (m_CurX = oldCurX) >= m_Cols )
		m_CurX = m_Cols - 1;

	if ( (m_CurY = oldCurY) >= m_Lines )
		m_CurY = m_Lines - 1;

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

	if ( m_Cols != oldCols || m_Lines != oldLines ) {
		if ( m_pDocument != NULL && m_pDocument->m_pSock != NULL )
			m_pDocument->m_pSock->SendWindSize();
	}

	if ( bHisInit )
		InitHistory();

//	DISPUPDATE();
//	FLUSH();
}
void CTextRam::InitScreen(int cols, int lines)
{
	int cx = 0, cy = 0;
	int colsmax = (m_ColsMax > cols ? m_ColsMax : cols);
	CRect rect, box;
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CRLoginView *pView = (CRLoginView *)GetAciveView();
	CChildFrame *pChild;
	CPaneFrame *pPane;

	if ( !IsInitText() || pMain == NULL || pView == NULL || (pChild = pView->GetFrameWnd()) == NULL )
		return;

	if ( pMain->IsIconic() || pMain->IsZoomed() )
		pMain->ShowWindow(SW_SHOWNORMAL);

	if ( (pPane = pMain->GetPaneFromChild(pChild->m_hWnd)) != NULL )
		pPane->m_bReqSize = TRUE;

	if ( !IsOptEnable(TO_RLFONT) ) {
		m_bReSize = TRUE;
		m_ReqCols = cols;
		m_ReqLines = lines;
	}

	if ( IsOptEnable(TO_RLFONT) || IsOptEnable(TO_RLNORESZ) ) {
		cx = pView->m_CharWidth  * cols  + m_ScrnOffset.left + m_ScrnOffset.right  - pChild->m_Width;
		cy = pView->m_CharHeight * lines + m_ScrnOffset.top  + m_ScrnOffset.bottom - pChild->m_Height;

		pMain->GetWindowRect(rect);
		pMain->SetWindowPos(NULL, 0, 0, rect.Width() + cx, rect.Height() + cy, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);

	} else {
		m_pDocument->UpdateAllViews(NULL, UPDATE_RESIZE, NULL);
	}

	if ( pPane != NULL )
		pPane->m_bReqSize = FALSE;
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
	} else
		m_HisFhd.SeekToEnd();

	return TRUE;
}
void CTextRam::InitHistory()
{
	if ( !IsOptEnable(TO_RLHISFILE) || m_HisFile.IsEmpty() || !IsInitText() )
		return;

	if ( !OpenHisFile() )
		return;

	try {
		CSpace spc;
		DWORD mx, my, nx;
		char head[5];
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
			for ( int x = 0 ; x < (int)mx ; x++ ) {
				ram.Read(m_HisFhd, head[3] - '0');
				if ( x < m_ColsMax && m_HisLen < (m_HisMax - m_Lines - 1) )
					*(vp++) = ram;
			}
			if ( m_HisLen < (m_HisMax - m_Lines - 1) )
				m_HisLen++;
		}

		m_LineEditMapsInit = 0;
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
	} catch(CFileException* pEx) {
		pEx->Delete();
		return;
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
		int n, i;
		DWORD d;

		m_HisFhd.SeekToBegin();
		m_HisFhd.Write("RLH3", 4);
		n = m_Cols; m_HisFhd.Write(&n, sizeof(DWORD));
		n = m_HisLen; m_HisFhd.Write(&n, sizeof(DWORD));
		for ( n = 0 ; n < m_HisLen ; n++ ) {
			vp = GETVRAM(0, m_Lines - n - 1);
			for ( i = 0 ; i < m_Cols ; i++, vp++ )
				vp->Write(m_HisFhd);
		}

		d = (DWORD)m_LineEditHis.GetSize(); m_HisFhd.Write(&d, sizeof(DWORD));
		for ( i = 0 ; i < m_LineEditHis.GetSize() ; i++ ) {
			d = (DWORD)m_LineEditHis.GetAt(i).GetSize(); m_HisFhd.Write(&d, sizeof(DWORD));
			m_HisFhd.Write(m_LineEditHis.GetAt(i).GetPtr(), d);
		}

		m_HisFhd.Close();

	} catch(CFileException* pEx) {
		AfxMessageBox(CStringLoad(IDE_HISTORYBACKUPERROR));
		pEx->Delete();
		return;
	} catch(...) {
		AfxMessageBox(CStringLoad(IDE_HISTORYBACKUPERROR));
	}
}
void CTextRam::SaveLogFile()
{
	int x, y;
	int my = m_Lines;
	CCharCell *vp;

	if ( m_pDocument == NULL || m_pDocument->m_pLogFile == NULL || m_LogMode != LOGMOD_PAGE )
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
		CallReceiveLine(y);
}
void CTextRam::HisRegCheck(DWORD ch, DWORD pos)
{
	int i, a;
	CRegExRes res;

	if ( m_bSimpSerch ) {
		if ( m_SimpStr[m_SimpLen] == (DCHAR)ch ) {
			if ( m_SimpPos == 0xFFFFFFFF )
				m_SimpPos = pos;
			m_SimpLen++;
			if ( m_SimpStr[m_SimpLen] == _T('\0') ) {
				if ( m_SimpPos != 0xFFFFFFFF ) {
					for ( a = (int)m_SimpPos ; a <= (int)pos ; a++ )
						GETVRAM(a % m_ColsMax, a / m_ColsMax - m_HisPos)->m_Vram.attr |= ATT_SMARK;
				}
				m_SimpPos = 0xFFFFFFFF;
				m_SimpLen = 0;
			}
		} else {
			m_SimpPos = 0xFFFFFFFF;
			m_SimpLen = 0;
		}

	} else {
		if ( m_MarkReg.MatchChar(ch, pos, &res) ) {
			if ( res.GetSize() > 0 ) {
				i = 0;	// for ( i = 0 ; i < res.GetSize() ; i++ ) {
				for ( a = res[i].m_SPos ; a < res[i].m_EPos && a < res.m_Idx.GetSize() ; a++ ) {
					if ( res.m_Idx[a] != 0xFFFFFFFF )
						GETVRAM(res.m_Idx[a] % m_ColsMax, res.m_Idx[a] / m_ColsMax - m_HisPos)->m_Vram.attr |= ATT_SMARK;
				}
			}
		}
	}
}
void CTextRam::HisMarkClear()
{
	int n, x;
	CCharCell *vp;

	for ( n = 1 ; n <= m_HisLen ; n++ ) {
		vp = GETVRAM(0, m_Lines - n);
		for ( x = 0 ; x < m_Cols ; x++ )
			vp[x].m_Vram.attr &= ~ATT_SMARK;
	}
}
int CTextRam::HisRegMark(LPCTSTR str, BOOL bRegEx)
{
	m_MarkPos = m_HisPos + m_Lines - m_HisLen;

	while ( m_MarkPos < 0 )
		m_MarkPos += m_HisMax;

	while ( m_MarkPos >= m_HisMax )
		m_MarkPos -= m_HisMax;

	if ( str == NULL || *str == _T('\0') ) {
		HisMarkClear();
		return 0;
	}

	m_SimpStr = str;
	m_SimpPos = 0xFFFFFFFF;
	m_SimpLen = 0;
	m_bSimpSerch = TRUE;

	if ( bRegEx ) {
		if ( !m_MarkReg.Compile(str) ) {
			::AfxMessageBox(m_MarkReg.m_ErrMsg);
			return 0;
		}

		m_MarkReg.MatchCharInit();
		m_bSimpSerch = FALSE;
	}

	m_MarkLen = 0;
	m_MarkEol = TRUE;

	return m_HisLen;
}
int CTextRam::HisRegNext(int msec)
{
	int n, x, ex, mx;
	CCharCell *vp;
	CStringD str;
	LPCDSTR p;
	clock_t timeup = clock() + msec * 1000 / CLOCKS_PER_SEC;

	for ( mx = m_MarkLen + 2000 ; m_MarkLen < mx && m_MarkLen < m_HisLen ; m_MarkLen++ ) {
		vp = GETVRAM(0, m_MarkPos - m_HisPos);
		for ( ex = m_Cols - 1 ; ex >= 0 ; ex-- ) {
			if ( !vp[ex].IsEmpty() )
				break;
			vp[ex].m_Vram.attr &= ~ATT_SMARK;
		}

		if ( m_MarkEol )
			HisRegCheck(L'\n', m_ColsMax * m_MarkPos);

		for ( x = 0 ; x <= ex ; x += n ) {
			str.Empty();
			vp[x].m_Vram.attr &= ~ATT_SMARK;
			if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) ) { // && vp[x].Compare(vp[x + 1]) == 0 ) {
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
			HisRegCheck(L'\r', m_ColsMax * m_MarkPos + x);
			m_MarkEol = TRUE;
		} else
			m_MarkEol = FALSE;

		if ( ++m_MarkPos >= m_HisMax )
			m_MarkPos -= m_HisMax;

		if ( clock() >= timeup )
			break;
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
			if ( (vp->m_Vram.attr & ATT_SMARK) != 0 )
				return TRUE;
		}
	}
	return FALSE;
}

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
static const EXTVRAM TempAtt = {
//  et  bc  fc   ch  at  ft  md  dm  cm  em  fc  bc
	0,  0,  0, { 0,  0,  0,  0,  0,  0,  0,  7,  0 }
};
static const LPCTSTR DropCmdTab[] = {
	//	Non					BPlus				XModem					YModem
		_T(""),				_T("bp -d %s\\r"),	_T("rx %s\\r"),			_T("rb\\r"),
	//	ZModem				SCP					Kermit
		_T("rz\\r"),		_T("scp -t %s"),	_T("kermit -i -r\\r"),	_T(""),
	};
static const LPCTSTR InitKeyList[] = {
	_T(""),
	_T("UP,DOWN,RIGHT,LEFT,PRIOR,NEXT,HOME,END,INSERT,DELETE"),
	_T("F1-F12"),
	_T("PAD0-PAD9,PADMUL,PADADD,PADSEP,PADSUB,PADDEC,PADDIV"),
	_T("ESCAPE,RETURN,BACK,TAB"),
	_T("SPACE,0-9,A-Z"),
};

void CTextRam::Init()
{
	m_DefCols[0]	= 80;
	m_DefCols[1]	= 132;
	m_Page          = 0;
	m_DefHisMax		= 2000;
	m_DefFontSize	= MulDiv(16, SYSTEM_DPI_Y, DEFAULT_DPI_Y);
	m_DefFontHw     = 20;
	m_KanjiMode		= EUC_SET;
	m_BankGL		= 0;
	m_BankGR		= 1;
	m_DefAtt		= TempAtt;
	m_DefCaretColor = RGB(255, 255, 255);
	m_RtfMode       = 0;

	memcpy(m_DefColTab, ColSetTab[DEFCOLTAB], sizeof(m_DefColTab));
	memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));
	memset(m_AnsiOpt, 0, sizeof(DWORD) * 16);
	memset(m_OptTab,  0, sizeof(DWORD) * 16);
	EnableOption(TO_ANSISRM);	//    12 SRM Set Send/Receive mode (Local echo off)
	EnableOption(TO_DECANM);	// ?   2 ANSI/VT52 Mode
	EnableOption(TO_DECAWM);	// ?   7 Autowrap mode
	EnableOption(TO_DECARM);	// ?   8 Autorepeat mode
	EnableOption(TO_DECTCEM);	// ?  25 Text Cursor Enable Mode
	EnableOption(TO_XTMCUS);	// ?  41 XTerm tab bug fix
//	EnableOption(TO_XTMRVW);	// ?  45 XTerm Reverse-wraparound mod
	EnableOption(TO_DECBKM);	// ?  67 Backarrow key mode (BS)
	EnableOption(TO_XTPRICOL);	// ?1070 Private Color Map
	EnableOption(TO_RLFONT);	// ?8404 フォントサイズから一行あたりの文字数を決定
	EnableOption(TO_RLUNIAWH);	// ?8428 UnicodeのAタイプの文字を半角として表示する
	EnableOption(TO_RLC1DIS);	// ?8454 C1制御を行わない
	EnableOption(TO_DRCSMMv1);	// ?8800 Unicode 16 Maping
	EnableOption(TO_RLNODELAY);	//  1486 TCPオプションのNoDelayを有効にする

	memcpy(m_DefAnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));
	memcpy(m_DefBankTab, DefBankTab, sizeof(m_DefBankTab));
	memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));

	m_SendCharSet[0] = _T("EUCJP-MS");
	m_SendCharSet[1] = _T("CP932");
	m_SendCharSet[2] = _T("ISO-2022-JP-MS");
	m_SendCharSet[3] = _T("UTF-8");
	m_SendCharSet[4] = _T("BIG-5");

	m_WheelSize      = 2;
	m_BitMapFile     = _T("");
	m_BitMapAlpha    = 255;
	m_BitMapBlend    = 128;
	m_BitMapStyle    = MAPING_FILL;
	m_DelayMSecLine = DELAY_LINE_MSEC;
	m_DelayMSecRecv = DELAY_RECV_MSEC;
	m_DelayMSecCrLf = 0;
	m_HisFile        = _T("");
	m_SshKeepAlive   = 300;
	m_TelKeepAlive   = 300;
	m_DropFileMode   = 0;
	m_DispCaret      = TRUE;
	m_TypeCaret      = m_DefTypeCaret;
	m_CaretColor     = m_DefCaretColor ;
	m_MarkColor      = RGB(255, 255, 0);
	m_ImeTypeCaret   = 0;
	m_ImeCaretColor  = RGB(31, 255, 127);

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

	m_DefTermPara[TERMPARA_VTLEVEL] = m_VtLevel = DEFVAL_VTLEVEL;
	m_DefTermPara[TERMPARA_FIRMVER] = m_FirmVer = DEFVAL_FIRMVER;
	m_DefTermPara[TERMPARA_TERMID]  = m_TermId  = DEFVAL_TERMID;
	m_DefTermPara[TERMPARA_UNITID]  = m_UnitId  = DEFVAL_UNITID;
	m_DefTermPara[TERMPARA_KEYBID]  = m_KeybId  = DEFVAL_KEYBID;

	m_LangMenu = 0;
	m_MediaCopyMode = MEDIACOPY_NONE;
	m_StsMode = 0;
	m_StsLine = 0;
	m_StsLed  = 0;
	SetRetChar(FALSE);

	for ( int n = 0 ; n < 64 ; n++ )
		m_Macro[n].Clear();

	m_ClipFlag = 0;
	m_ClipCrLf = OSCCLIP_LF;
	memset(m_MacroExecFlag, 0, sizeof(m_MacroExecFlag));
	m_ShellExec.GetString(_T("http://|https://|ftp://|mailto://"), _T('|'));
	m_InlineExt.GetString(_T("doc|txt|pdf|mp3|avi|mpg"), _T('|'));
	m_ScrnOffset.SetRect(0, 0, 0, 0);
	m_TimeFormat = _T("%H:%M:%S ");
	m_DefFontName[0] = _T("ＭＳ ゴシック");
	m_DefFontName[1] = _T("ＭＳ 明朝");

	m_TraceLogFile.Empty();
	m_TraceMaxCount = 1000;
	m_FixVersion = 0;
	m_SleepMax = VIEW_SLEEP_MAX;
	m_GroupCast.Empty();
	m_TitleName.Empty();

	for ( int n = 0 ; n < MODKEY_MAX ; n++ ) {
		m_DefModKey[n] = (-1);
		m_ModKeyList[n] = InitKeyList[n];
	}
	memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
	InitModKeyTab();

	m_ProcTab.Init();
//	m_FontTab.Init();
	m_TextBitMap.Init();

	m_SleepMode = SLEEPMODE_ACTIVE;

	RESET(RESET_ALL);

	// RESETでFALSEに設定される
	m_bInit = TRUE;
}

static const LPCTSTR setname[] = { _T("EUC"), _T("SJIS"), _T("ASCII"), _T("UTF8"), _T("BIG5") };

void CTextRam::SetIndex(int mode, CStringIndex &index)
{
	int n, i;
	CString str, wrk;

	if ( mode ) {	// Write
		index[_T("Cols")][_T("Nomal")] = m_DefCols[0];
		index[_T("Cols")][_T("Wide")]  = m_DefCols[1];

		index[_T("History")]  = m_DefHisMax;
		index[_T("FontSize")] = m_DefFontSize;
		index[_T("CharSet")]  = m_KanjiMode;

		index[_T("GL")] = m_BankGL;
		index[_T("GR")] = m_BankGR;

		index[_T("Attribute")] = m_DefAtt.std.attr;
		index[_T("TextColor")] = m_DefAtt.std.fcol;
		index[_T("BackColor")] = m_DefAtt.std.bcol;

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
		index[_T("BitMapAlpha")]  = m_BitMapAlpha;
		index[_T("BitMapBlend")]  = m_BitMapBlend;
		index[_T("BitMapStyle")]  = m_BitMapStyle;

		index[_T("HisFile")]      = m_HisFile;
		index[_T("LogFile")]      = m_LogFile;

		index[_T("WheelSize")]    = m_WheelSize;
		index[_T("DelayUSecChar")] = m_DelayUSecChar;
		index[_T("DelayMSecLine")] = m_DelayMSecLine;
		index[_T("DelayMSecRecv")] = m_DelayMSecRecv;
		index[_T("DelayMSec")]     = m_DelayMSecCrLf;
		index[_T("KeepAliveSec")] = m_SshKeepAlive;
		index[_T("TelKeepAlive")] = m_TelKeepAlive;

		index[_T("WordStr")]      = m_WordStr;
		index[_T("DropFileMode")] = m_DropFileMode;

		for ( n = 0 ; n < 8 ; n++ )
			index[_T("DropFileCmd")].Add(m_DropFileCmd[n]);

		for ( n = 0 ; n < 4 ; n++ )
			index[_T("MouseMode")].Add(m_MouseMode[n]);

		index[_T("TitleMode")] = m_TitleMode;
		index[_T("ClipFlag")]  = m_ClipFlag;
		index[_T("ClipCrLf")]  = m_ClipCrLf;
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
			if ( IS_ENABLE(m_DefAnsiOpt, n) )
				index[_T("Option")].Add(IndexToOption(n));
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
		
		index[_T("Caret")] = m_DefTypeCaret;
		index[_T("BugFix")] = FIX_VERSION;
		index[_T("SleepTime")] = m_SleepMax;

		for ( n = 0 ; n < 5 ; n++ )
			index[_T("TermParaId")].Add(m_DefTermPara[n]);

		index[_T("LogMode")] = m_LogMode;

		index[_T("GroupCast")] = m_GroupCast;

		for ( n = 0 ; n < m_InlineExt.GetSize() ; n++ )
			index[_T("InlineExt")].Add(m_InlineExt[n]);

		index[_T("MarkColor")].Add(GetRValue(m_MarkColor));
		index[_T("MarkColor")].Add(GetGValue(m_MarkColor));
		index[_T("MarkColor")].Add(GetBValue(m_MarkColor));

		index[_T("CaretColor")].Add(GetRValue(m_DefCaretColor));
		index[_T("CaretColor")].Add(GetGValue(m_DefCaretColor));
		index[_T("CaretColor")].Add(GetBValue(m_DefCaretColor));

		index[_T("Title")] = m_TitleName;
		index[_T("RtfMode")] = m_RtfMode;
		index[_T("SleepMode")] = m_SleepMode;

		index[_T("ImeTypeCaret")] = m_ImeTypeCaret;
		index[_T("ImeCaretColor")].Add(GetRValue(m_ImeCaretColor));
		index[_T("ImeCaretColor")].Add(GetGValue(m_ImeCaretColor));
		index[_T("ImeCaretColor")].Add(GetBValue(m_ImeCaretColor));

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
			m_DefAtt.std.attr = index[n];
		if ( (n = index.Find(_T("TextColor"))) >= 0 ) {
			m_DefAtt.std.fcol = index[n];
			m_DefAtt.eatt &= ~EATT_FRGBCOL;
		}
		if ( (n = index.Find(_T("BackColor"))) >= 0 ) {
			m_DefAtt.std.bcol = index[n];
			m_DefAtt.eatt &= ~EATT_BRGBCOL;
		}

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
		if ( (n = index.Find(_T("BitMapAlpha"))) >= 0 )
			m_BitMapAlpha = index[n];
		if ( (n = index.Find(_T("BitMapBlend"))) >= 0 )
			m_BitMapBlend = index[n];
		if ( (n = index.Find(_T("BitMapStyle"))) >= 0 )
			m_BitMapStyle = index[n];

		if ( (n = index.Find(_T("HisFile"))) >= 0 )
			m_HisFile = index[n];
		if ( (n = index.Find(_T("LogFile"))) >= 0 )
			m_LogFile = index[n];

		if ( (n = index.Find(_T("WheelSize"))) >= 0 )
			m_WheelSize = index[n];
		if ( (n = index.Find(_T("DelayUSecChar"))) >= 0 )
			m_DelayUSecChar = index[n];
		if ( (n = index.Find(_T("DelayMSecLine"))) >= 0 )
			m_DelayMSecLine = index[n];
		if ( (n = index.Find(_T("DelayMSecRecv"))) >= 0 )
			m_DelayMSecRecv = index[n];
		if ( (n = index.Find(_T("DelayMSec"))) >= 0 )
			m_DelayMSecCrLf = index[n];

		if ( (n = index.Find(_T("KeepAliveSec"))) >= 0 )
			m_SshKeepAlive = index[n];
		if ( (n = index.Find(_T("TelKeepAlive"))) >= 0 )
			m_TelKeepAlive = index[n];

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
		if ( (n = index.Find(_T("ClipCrLf"))) >= 0 )
			m_ClipCrLf = index[n];
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
		if ( (n = index.Find(_T("MetaKeysAdd"))) >= 0 ) {
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int b = index[n][i];
				if ( b >= 0 && b <= 255 )
					m_MetaKeys[b / 32] |= (1 << (b % 32));
			}
		}
		if ( (n = index.Find(_T("MetaKeysDel"))) >= 0 ) {
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int b = index[n][i];
				if ( b >= 0 && b <= 255 )
					m_MetaKeys[b / 32] &= ~(1 << (b % 32));
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

		if ( (n = index.Find(_T("Caret"))) >= 0 ) {
			if ( (m_TypeCaret = m_DefTypeCaret = index[n]) <= 0 )
				m_TypeCaret = m_DefTypeCaret = 1;
		}

		if ( (n = index.Find(_T("BugFix"))) >= 0 )
			m_FixVersion = index[n];

		if ( (n = index.Find(_T("SleepTime"))) >= 0 )
			m_SleepMax = index[n];

		if ( (n = index.Find(_T("Option"))) >= 0 ) {
			memset(m_DefAnsiOpt, 0, sizeof(m_DefAnsiOpt));
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int a = OptionToIndex(index[n][i], TRUE);
				if ( a >= 0 && a <= 511 )
					m_DefAnsiOpt[a / 32] |= (1 << (a % 32));
			}
		}
		if ( (n = index.Find(_T("OptionAdd"))) >= 0 ) {
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int a = OptionToIndex(index[n][i], TRUE);
				if ( a >= 0 && a <= 511 )
					m_DefAnsiOpt[a / 32] |= (1 << (a % 32));
			}
		}
		if ( (n = index.Find(_T("OptionDel"))) >= 0 ) {
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int a = OptionToIndex(index[n][i], TRUE);
				if ( a >= 0 && a <= 511 )
					m_DefAnsiOpt[a / 32] &= ~(1 << (a % 32));
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
		if ( (n = index.Find(_T("SocketOptAdd"))) >= 0 ) {
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int b = index[n][i];
				if ( b >= 0 && b <= 511 )
					m_OptTab[b / 32] |= (1 << (b % 32));
			}
		}
		if ( (n = index.Find(_T("SocketOptDel"))) >= 0 ) {
			for ( i = 0 ; i < index[n].GetSize() ; i++ ) {
				int b = index[n][i];
				if ( b >= 0 && b <= 511 )
					m_OptTab[b / 32] &= ~(1 << (b % 32));
			}
		}

		if ( (n = index.Find(_T("TermParaId"))) >= 0 ) {
			for ( i = 0 ; i < 5 && i < index[n].GetSize() ; i++ )
				m_DefTermPara[i] = index[n][i];
			m_VtLevel = m_DefTermPara[TERMPARA_VTLEVEL];
			m_FirmVer = m_DefTermPara[TERMPARA_FIRMVER];
			m_TermId  = m_DefTermPara[TERMPARA_TERMID];
			m_UnitId  = m_DefTermPara[TERMPARA_UNITID];
			m_KeybId  = m_DefTermPara[TERMPARA_KEYBID];
		}

		if ( (n = index.Find(_T("LogMode"))) >= 0 )
			m_LogMode = index[n];

		if ( (n = index.Find(_T("GroupCast"))) >= 0 )
			m_GroupCast = index[n];

		if ( (n = index.Find(_T("InlineExt"))) >= 0 ) {
			m_InlineExt.RemoveAll();
			for ( i = 0 ; i < index[n].GetSize() ; i++ )
				m_InlineExt.Add(index[n][i]);
		}

		if ( (n = index.Find(_T("MarkColor"))) >= 0 && index[n].GetSize() >= 3 )
			m_MarkColor = RGB((int)index[n][0], (int)index[n][1], (int)index[n][2]);

		if ( (n = index.Find(_T("CaretColor"))) >= 0 && index[n].GetSize() >= 3 )
			m_DefCaretColor = m_CaretColor = RGB((int)index[n][0], (int)index[n][1], (int)index[n][2]);
		
		if ( (n = index.Find(_T("Title"))) >= 0 )
			m_TitleName = index[n];

		if ( (n = index.Find(_T("RtfMode"))) >= 0 )
			m_RtfMode = index[n];

		if ( (n = index.Find(_T("SleepMode"))) >= 0 )
			m_SleepMode = index[n];

		if ( (n = index.Find(_T("ImeTypeCaret"))) >= 0 )
			m_ImeTypeCaret = index[n];

		if ( (n = index.Find(_T("ImeCaretColor"))) >= 0 && index[n].GetSize() >= 3 )
			m_ImeCaretColor = RGB((int)index[n][0], (int)index[n][1], (int)index[n][2]);

		memcpy(m_ColTab, m_DefColTab, sizeof(m_DefColTab));
		memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));
		memcpy(m_BankTab, m_DefBankTab, sizeof(m_DefBankTab));

		RESET(RESET_ALL);
	}
}
void CTextRam::DiffIndex(CTextRam &orig, CStringIndex &index)
{
	int n, i;
	CString str, wrk;

	if ( m_DefCols[0] != orig.m_DefCols[0] )
		index[_T("Cols")][_T("Nomal")] = m_DefCols[0];
	if ( m_DefCols[1] != orig.m_DefCols[1] )
		index[_T("Cols")][_T("Wide")]  = m_DefCols[1];

	if ( m_DefHisMax != orig.m_DefHisMax )
		index[_T("History")]  = m_DefHisMax;
	if ( m_DefFontSize != orig.m_DefFontSize )
		index[_T("FontSize")] = m_DefFontSize;
	if ( m_KanjiMode != orig.m_KanjiMode )
		index[_T("CharSet")]  = m_KanjiMode;

	if ( m_BankGL != orig.m_BankGL )
		index[_T("GL")] = m_BankGL;
	if ( m_BankGR != orig.m_BankGR )
		index[_T("GR")] = m_BankGR;

	if ( m_DefAtt.std.attr != orig.m_DefAtt.std.attr )
		index[_T("Attribute")] = m_DefAtt.std.attr;
	if ( m_DefAtt.std.fcol != orig.m_DefAtt.std.fcol )
		index[_T("TextColor")] = m_DefAtt.std.fcol;
	if ( m_DefAtt.std.bcol != orig.m_DefAtt.std.bcol )
		index[_T("BackColor")] = m_DefAtt.std.bcol;

	if ( memcmp(m_DefColTab, orig.m_DefColTab, sizeof(m_DefColTab)) != 0 ) {
		index[_T("ColorTable")].SetSize(16);
		for ( n = 0 ; n < 16 ; n++ ) {
			index[_T("ColorTable")][n].Add(GetRValue(m_DefColTab[n]));
			index[_T("ColorTable")][n].Add(GetGValue(m_DefColTab[n]));
			index[_T("ColorTable")][n].Add(GetBValue(m_DefColTab[n]));
		}
	}

	for ( n = 0 ; n < 5 ; n++ ) {
		if ( memcmp(m_DefBankTab[n], orig.m_DefBankTab[n], sizeof(m_DefBankTab[n])) != 0 ) {
			index[_T("BankTable")][setname[n]].SetSize(4);
			for ( i = 0 ; i < 4 ; i++ ) {
				index[_T("BankTable")][setname[n]][i].Add(m_DefBankTab[n][i] >> 8);
				index[_T("BankTable")][setname[n]][i].Add(m_FontTab[m_DefBankTab[n][i]].m_IndexName);
			}
		}
	}

	for ( n = 0 ; n < 4 ; n++ ) {
		if ( m_SendCharSet[n].Compare(orig.m_SendCharSet[n]) != 0 ) {
			for ( n = 0 ; n < 4 ; n++ )
				index[_T("SendCharSet")].Add(m_SendCharSet[n]);
			break;
		}
	}

	if ( m_BitMapFile.Compare(orig.m_BitMapFile) != 0 )
		index[_T("BitMapFile")]   = m_BitMapFile;
	if ( m_BitMapAlpha != orig.m_BitMapAlpha )
		index[_T("BitMapAlpha")]  = m_BitMapAlpha;
	if ( m_BitMapBlend != orig.m_BitMapBlend )
		index[_T("BitMapBlend")]  = m_BitMapBlend;
	if ( m_BitMapStyle != orig.m_BitMapStyle )
		index[_T("BitMapStyle")]  = m_BitMapStyle;

	if ( m_HisFile.Compare(orig.m_HisFile) != 0 )
		index[_T("HisFile")]      = m_HisFile;
	if ( m_LogFile.Compare(orig.m_LogFile) != 0 )
		index[_T("LogFile")]      = m_LogFile;

	if ( m_WheelSize != orig.m_WheelSize )
		index[_T("WheelSize")]    = m_WheelSize;
	if ( m_DelayUSecChar != orig.m_DelayUSecChar )
		index[_T("DelayUSecChar")]    = m_DelayUSecChar;
	if ( m_DelayMSecLine != orig.m_DelayMSecLine )
		index[_T("DelayMSecLine")]    = m_DelayMSecLine;
	if ( m_DelayMSecRecv != orig.m_DelayMSecRecv )
		index[_T("DelayMSecRecv")]    = m_DelayMSecRecv;
	if ( m_DelayMSecCrLf != orig.m_DelayMSecCrLf )
		index[_T("DelayMSec")]    = m_DelayMSecCrLf;
	if ( m_SshKeepAlive != orig.m_SshKeepAlive )
		index[_T("KeepAliveSec")] = m_SshKeepAlive;
	if ( m_TelKeepAlive != orig.m_TelKeepAlive )
		index[_T("TelKeepAlive")] = m_TelKeepAlive;

	if ( m_WordStr.Compare(orig.m_WordStr) != 0 )
		index[_T("WordStr")]      = m_WordStr;
	if ( m_DropFileMode != orig.m_DropFileMode )
		index[_T("DropFileMode")] = m_DropFileMode;

	for ( n = 0 ; n < 8 ; n++ ) {
		if ( m_DropFileCmd[n].Compare(orig.m_DropFileCmd[n]) != 0 ) {
			for ( n = 0 ; n < 8 ; n++ )
				index[_T("DropFileCmd")].Add(m_DropFileCmd[n]);
			break;
		}
	}

	if ( memcmp(m_MouseMode, orig.m_MouseMode, sizeof(m_MouseMode)) != 0 ) {
		for ( n = 0 ; n < 4 ; n++ )
			index[_T("MouseMode")].Add(m_MouseMode[n]);
	}

	if ( m_TitleMode != orig.m_TitleMode )
		index[_T("TitleMode")] = m_TitleMode;
	if ( m_ClipFlag != orig.m_ClipFlag )
		index[_T("ClipFlag")]  = m_ClipFlag;
	if ( m_ClipCrLf != orig.m_ClipCrLf )
		index[_T("ClipCrLf")]  = m_ClipCrLf;
	if ( m_DefFontHw != orig.m_DefFontHw )
		index[_T("FontHw")]    = m_DefFontHw;

	if ( m_RecvCrLf != orig.m_RecvCrLf )
		index[_T("RecvCrLf")]  = m_RecvCrLf;
	if ( m_SendCrLf != orig.m_SendCrLf )
		index[_T("SendCrLf")]  = m_SendCrLf;

	m_ShellExec.SetString(str);
	orig.m_ShellExec.SetString(wrk);
	if ( str.Compare(wrk) != 0 ) {
		for ( n = 0 ; n < m_ShellExec.GetSize() ; n++ )
			index[_T("ShellExec")].Add(m_ShellExec[n]);
	}

	if ( memcmp(m_DefModKey, orig.m_DefModKey, sizeof(m_DefModKey)) != 0 ) {
		for ( n = 0 ; n < MODKEY_MAX ; n++ )
			index[_T("ModKey")].Add(m_DefModKey[n]);
	}
	for ( n = 0 ; n < MODKEY_MAX ; n++ ) {
		if ( m_ModKeyList[n].Compare(orig.m_ModKeyList[n]) != 0 ) {
			for ( n = 0 ; n < MODKEY_MAX ; n++ )
				index[_T("ModKeyList")].Add(m_ModKeyList[n]);
			break;
		}
	}

	if ( memcmp(m_MetaKeys, orig.m_MetaKeys, sizeof(m_MetaKeys)) != 0 ) {
		for ( n = 0 ; n < (32 * 8) ; n++ ) {
			if ( IS_ENABLE(m_MetaKeys, n) != IS_ENABLE(orig.m_MetaKeys, n) ) {
				if ( IS_ENABLE(m_MetaKeys, n) )
					index[_T("MetaKeysAdd")].Add(n);
				else
					index[_T("MetaKeysDel")].Add(n);
			}
		}
	}

	if ( memcmp(m_DefAnsiOpt, orig.m_DefAnsiOpt, sizeof(m_DefAnsiOpt)) != 0 ) {
		for ( n = 0 ; n < (32 * 16) ; n++ ) {
			i = IndexToOption(n);

			if ( IS_ENABLE(m_DefAnsiOpt, n) != IS_ENABLE(orig.m_DefAnsiOpt, n) ) {
				if ( IS_ENABLE(m_DefAnsiOpt, n) )
					index[_T("OptionAdd")].Add(i);
				else
					index[_T("OptionDel")].Add(i);
			}
		}
	}

	if ( memcmp(m_OptTab, orig.m_OptTab, sizeof(m_OptTab)) != 0 ) {
		for ( n = 0 ; n < (32 * 16) ; n++ ) {
			if ( IS_ENABLE(m_OptTab, n) != IS_ENABLE(orig.m_OptTab, n) ) {
				if ( IS_ENABLE(m_OptTab, n) )
					index[_T("SocketOptAdd")].Add(n);
				else
					index[_T("SocketOptDel")].Add(n);
			}
		}
	}

		// m_ProcTab.SetIndex(mode, index[_T("ProcTable")]);

	if ( memcmp(m_ScrnOffset, orig.m_ScrnOffset, sizeof(m_ScrnOffset)) != 0 ) {
		index[_T("ScreenOffset")].Add(m_ScrnOffset.left);
		index[_T("ScreenOffset")].Add(m_ScrnOffset.right);
		index[_T("ScreenOffset")].Add(m_ScrnOffset.top);
		index[_T("ScreenOffset")].Add(m_ScrnOffset.bottom);
	}

	if ( m_TimeFormat.Compare(orig.m_TimeFormat) != 0 )
		index[_T("TimeFormat")] = m_TimeFormat;

	for ( n = 0 ; n < 16 ; n++ ) {
		if ( m_DefFontName[n].Compare(orig.m_DefFontName[n]) != 0 ) {
			for ( n = 0 ; n < 16 ; n++ )
				index[_T("DefFontName")].Add(m_DefFontName[n]);
			break;
		}
	}

	if ( m_TraceLogFile.Compare(orig.m_TraceLogFile) != 0 )
		index[_T("TraceLogFile")]  = m_TraceLogFile;
	if ( m_TraceMaxCount != orig.m_TraceMaxCount )
		index[_T("TraceMaxCount")] = m_TraceMaxCount;
		
	if ( m_DefTypeCaret != orig.m_DefTypeCaret )
		index[_T("Caret")] = m_DefTypeCaret;
	if ( m_SleepMax != orig.m_SleepMax )
		index[_T("SleepTime")] = m_SleepMax;

	if ( memcmp(m_DefTermPara, orig.m_DefTermPara, sizeof(m_DefTermPara)) != 0 ) {
		for ( n = 0 ; n < 5 ; n++ )
			index[_T("TermParaId")].Add(m_DefTermPara[n]);
	}

	if ( m_LogMode != orig.m_LogMode )
		index[_T("LogMode")] = m_LogMode;

	if ( m_GroupCast.Compare(orig.m_GroupCast) != 0 )
		index[_T("GroupCast")] = m_GroupCast;

	m_InlineExt.SetString(str);
	orig.m_InlineExt.SetString(wrk);
	if ( str.Compare(wrk) != 0 ) {
		for ( n = 0 ; n < m_InlineExt.GetSize() ; n++ )
			index[_T("InlineExt")].Add(m_InlineExt[n]);
	}

	if ( m_MarkColor != orig.m_MarkColor ) {
		index[_T("MarkColor")].Add(GetRValue(m_MarkColor));
		index[_T("MarkColor")].Add(GetGValue(m_MarkColor));
		index[_T("MarkColor")].Add(GetBValue(m_MarkColor));
	}

	if ( m_DefCaretColor != orig.m_DefCaretColor ) {
		index[_T("CaretColor")].Add(GetRValue(m_DefCaretColor));
		index[_T("CaretColor")].Add(GetGValue(m_DefCaretColor));
		index[_T("CaretColor")].Add(GetBValue(m_DefCaretColor));
	}
	
	if ( m_TitleName.Compare(orig.m_TitleName) != 0 )
		index[_T("Title")] = m_TitleName;

	if ( m_RtfMode != orig.m_RtfMode )
		index[_T("RtfMode")] = m_RtfMode;

	if ( m_SleepMode != orig.m_SleepMode )
		index[_T("SleepMode")] = m_SleepMode;

	if ( m_ImeTypeCaret != orig.m_ImeTypeCaret )
		index[_T("ImeTypeCaret")] = m_ImeTypeCaret;

	if ( m_ImeCaretColor != orig.m_ImeCaretColor ) {
		index[_T("ImeCaretColor")].Add(GetRValue(m_ImeCaretColor));
		index[_T("ImeCaretColor")].Add(GetGValue(m_ImeCaretColor));
		index[_T("ImeCaretColor")].Add(GetBValue(m_ImeCaretColor));
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
	stra.AddBin(&m_DefAtt.std, sizeof(STDVRAM));
	stra.AddBin(m_DefColTab,  sizeof(m_DefColTab));
	stra.AddBin(m_DefAnsiOpt, sizeof(m_DefAnsiOpt));
	stra.AddBin(m_DefBankTab, sizeof(m_DefBankTab));
	stra.Add(m_SendCharSet[0]);
	stra.Add(m_SendCharSet[1]);
	stra.Add(m_SendCharSet[2]);
	stra.Add(m_SendCharSet[3]);
	stra.AddVal(m_WheelSize);
	stra.Add(m_BitMapFile);
	stra.AddVal(m_DelayMSecCrLf);
	stra.Add(m_HisFile);
	stra.AddVal(m_SshKeepAlive);
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

	stra.AddVal(FIX_VERSION);	// AnsiOpt Bugfix

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

	stra.AddVal(m_DefTypeCaret);
	stra.AddVal(m_SleepMax);
	stra.AddVal(m_LogMode);

	tmp.RemoveAll();
	for ( int n = 0 ; n < 5 ; n++ )
		tmp.AddVal(m_DefTermPara[n]);
	tmp.SetString(str, _T(';'));
	stra.Add(str);

	stra.Add(m_GroupCast);
	stra.AddVal(m_BitMapAlpha);

	m_TextBitMap.SetArray(tmp);
	tmp.SetString(str, _T('\x07'));
	stra.Add(str);

	stra.AddVal(m_BitMapBlend);

	m_InlineExt.SetString(str, _T('|'));
	stra.Add(str);

	stra.AddVal(m_MarkColor);
	stra.AddVal(m_BitMapStyle);
	stra.AddVal(m_DefCaretColor);

	stra.Add(m_TitleName);
	stra.AddVal(m_ClipCrLf);
	stra.AddVal(m_TelKeepAlive);

	stra.AddVal(m_RtfMode);

	stra.AddVal(m_DelayUSecChar);
	stra.AddVal(m_DelayMSecLine);
	stra.AddVal(m_DelayMSecRecv);

	stra.AddVal(m_SleepMode);

	stra.AddVal(m_ImeTypeCaret);
	stra.AddVal(m_ImeCaretColor);
}
void CTextRam::GetArray(CStringArrayExt &stra)
{
	int n;
	BYTE tmp[16];
	CStringArrayExt ext;

	if ( !m_bInit )
		Init();

	m_DefCols[0]  = stra.GetVal(0);
	m_DefHisMax   = stra.GetVal(1);
	m_DefFontSize = stra.GetVal(2);
	m_KanjiMode   = stra.GetVal(3);
	m_BankGL      = stra.GetVal(4);
	m_BankGR      = stra.GetVal(5);

	if ( (n = stra.GetBin(6, tmp, 16)) == sizeof(STDVRAM) )
		memcpy(&m_DefAtt.std, tmp, sizeof(STDVRAM));
	else if ( n == 8 ) {
		memset(&m_DefAtt.std, 0, sizeof(STDVRAM));
		m_DefAtt.std.attr = tmp[6];
		m_DefAtt.std.fcol = tmp[7] & 0x0F;
		m_DefAtt.std.bcol = tmp[7] >> 4;
	}
	m_DefAtt.eatt = 0;

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

	m_DelayMSecCrLf = (stra.GetSize() > 16 ? stra.GetVal(16) : 0);
	m_HisFile      = (stra.GetSize() > 17 ? stra.GetAt(17) : _T(""));
	m_SshKeepAlive = (stra.GetSize() > 18 ? stra.GetVal(18) : 0);
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

	m_FixVersion = (stra.GetSize() > 37 ? stra.GetVal(37) : 0);
	if ( m_FixVersion < 1 ) {
		ReversOption(TO_DECANM);
		EnableOption(TO_DECTCEM);	// ?25 Text Cursor Enable Mode
	}
	if ( m_FixVersion < 2 )
		EnableOption(TO_XTMCUS);	// ?41 XTerm tab bug fix
	if ( m_FixVersion < 3 ) {
		EnableOption(TO_DECBKM);	// ?67 Backarrow key mode (BS
		EnableOption(TO_ANSISRM);	//  12 SRM Set Send/Receive mode (Local echo off)
	}
	if ( m_FixVersion < 4 )
		EnableOption(TO_ANSISRM);	//  12 SRM Set Send/Receive mode (Local echo off)
	if ( m_FixVersion < 5 ) {					
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
	if ( m_FixVersion < 6 )
		EnableOption(TO_DRCSMMv1);	// ?8800 Unicode 16 Maping
	if ( m_FixVersion < 7 )
		EnableOption(TO_DECARM);	// ? 8 Autorepeat mode
	if ( m_FixVersion < 8 )
		EnableOption(TO_XTPRICOL);	// ?1070 Regis/Sixel Private Color Map

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

	if ( stra.GetSize() > 60 ) {
		if ( (m_TypeCaret = m_DefTypeCaret = stra.GetVal(60)) <= 0 )
			m_TypeCaret = m_DefTypeCaret = 1;
	}

	if ( stra.GetSize() > 61 )
		m_SleepMax = stra.GetVal(61);

	if ( stra.GetSize() > 62 )
		m_LogMode = stra.GetVal(62);
	else
		m_LogMode = IsOptValue(TO_RLLOGMODE, 2);

	if ( stra.GetSize() > 63 ) {
		ext.GetString(stra.GetAt(63), _T(';'));
		for ( n = 0 ; n < 5 && n < ext.GetSize() ; n++ )
			m_DefTermPara[n] = _tstoi(ext[n]);
		m_VtLevel = m_DefTermPara[TERMPARA_VTLEVEL];
		m_FirmVer = m_DefTermPara[TERMPARA_FIRMVER];
		m_TermId  = m_DefTermPara[TERMPARA_TERMID];
		m_UnitId  = m_DefTermPara[TERMPARA_UNITID];
		m_KeybId  = m_DefTermPara[TERMPARA_KEYBID];
	}

	if ( stra.GetSize() > 64 )
		m_GroupCast = stra.GetAt(64);

	if ( stra.GetSize() > 65 )
		m_BitMapAlpha = stra.GetVal(65);

	if ( stra.GetSize() > 66 ) {
		ext.GetString(stra.GetAt(66), _T('\x07'));
		m_TextBitMap.GetArray(ext);
	}
	if ( stra.GetSize() > 67 )
		m_BitMapBlend = stra.GetVal(67);

	if ( stra.GetSize() > 68 )
		m_InlineExt.GetString(stra.GetAt(68), _T('|'));

	if ( stra.GetSize() > 69 )
		m_MarkColor = stra.GetVal(69);

	if ( stra.GetSize() > 70 )
		m_BitMapStyle = stra.GetVal(70);

	if ( stra.GetSize() > 71 )
		m_DefCaretColor = m_CaretColor = stra.GetVal(71);

	if ( stra.GetSize() > 72 )
		m_TitleName = stra.GetAt(72);
	if ( stra.GetSize() > 73 )
		m_ClipCrLf = stra.GetVal(73);
	if ( stra.GetSize() > 74 )
		m_TelKeepAlive = stra.GetVal(74);

	if ( stra.GetSize() > 75 )
		m_RtfMode = stra.GetVal(75);

	if ( stra.GetSize() > 76 )
		m_DelayUSecChar = stra.GetVal(76);
	if ( stra.GetSize() > 77 )
		m_DelayMSecLine = stra.GetVal(77);
	if ( stra.GetSize() > 78 )
		m_DelayMSecRecv = stra.GetVal(78);

	if ( stra.GetSize() > 79 )
		m_SleepMode = stra.GetVal(79);

	if ( stra.GetSize() > 80 )
		m_ImeTypeCaret = stra.GetVal(80);
	if ( stra.GetSize() > 81 )
		m_ImeCaretColor = stra.GetVal(81);


	if ( m_FixVersion < 9 ) {
		if ( m_pDocument != NULL ) {
			if ( m_pDocument->m_ServerEntry.m_UserNameProvs.IsEmpty() || m_pDocument->m_ServerEntry.m_PassNameProvs.IsEmpty() )
				EnableOption(TO_RLUSEPASS);
			if ( (m_pDocument->m_ServerEntry.m_ProxyMode & 7) > 0 && (m_pDocument->m_ServerEntry.m_ProxyUserProvs.IsEmpty() || m_pDocument->m_ServerEntry.m_ProxyPassProvs.IsEmpty()) )
				EnableOption(TO_PROXPASS);
		} else if ( m_pServerEntry != NULL ) {
			if ( m_pServerEntry->m_UserNameProvs.IsEmpty() || m_pServerEntry->m_PassNameProvs.IsEmpty() )
				EnableOption(TO_RLUSEPASS);
			if ( (m_pServerEntry->m_ProxyMode & 7) > 0 && (m_pServerEntry->m_ProxyUserProvs.IsEmpty() || m_pServerEntry->m_ProxyPassProvs.IsEmpty()) )
				EnableOption(TO_PROXPASS);
		}
	}

	if ( m_FixVersion < 10 ) {
		if ( m_ModKeyList[4].Compare(_T("ESCAPE,RETURN,BACK,TAB,SPACE")) == 0 && m_ModKeyList[5].Compare(_T("0-9,A-Z")) == 0 ) {
			m_ModKeyList[4] = InitKeyList[4];
			m_ModKeyList[5] = InitKeyList[5];
		}
	}

	if ( m_FixVersion < 11 ) {
		if ( IsOptEnable(TO_RLDELAY) )
			EnableOption(TO_RLDELAYCR);
	}

	RESET(RESET_ALL);
}

static const ScriptCmdsDefs DocScrn[] = {
	{	"Cursol",		1	},
	{	"Size",			2	},
	{	"Style",		3	},
	{	"Color",		4	},
	{	"Mode",			10	},
	{	"ExtMode",		11	},
	{	"Sleep",		12	},
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
				if ( (opt = OptionToIndex(opt)) > 0 && opt < 512 )
					value = (int)(IsOptEnable(opt) ? 1 : 0);
				else
					value = (int)0;
			} else {
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
	case 12:				// Document.Screen.Sleep
		value.SetInt(m_SleepMax, mode);
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
		n = (m_DispCaret ? 1 : 0);
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
		n = m_AttNow.std.fcol;
		value.SetInt(n, mode);
		m_AttNow.std.fcol = (BYTE)n;
		m_AttNow.eatt &= ~EATT_FRGBCOL;
		break;
	case 27:				// Document.Screen.Style.BackColor
		n = m_AttNow.std.bcol;
		value.SetInt(n, mode);
		m_AttNow.std.bcol = (BYTE)n;
		m_AttNow.eatt &= ~EATT_BRGBCOL;
		break;
	case 28:				// Document.Screen.Style.Attribute
		n = m_AttNow.std.attr;
		value.SetInt(n, mode);
		m_AttNow.std.attr = (DWORD)n;
		break;
	case 29:				// Document.Screen.Style.FontNumber
		n = m_AttNow.std.font;
		value.SetInt(n, mode);
		m_AttNow.std.font = (DWORD)n;
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

	m_UpdateFlag = FALSE;

	for ( n = 0 ; n < nBufLen && !m_UpdateFlag ; n++ ) {
		fc_Call(lpBuf[n]);
		if ( m_RetSync ) {
			m_RetSync = FALSE;
			*sync = TRUE;
			break;
		}
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
		if ( vp[x].IsEmpty() ) {
			str.PutWord(' ');
			n = 1;
		} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
			for ( p = vp[x] ; *p != 0 ; p++ )
				str.PutWord(*p);
			n = 2;
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
	else if ( UnicodeWidth(ch) == 2 && iswalnum((TCHAR)ch) )
		return 4;
	else if ( iswalnum((TCHAR)ch) || m_WordStr.Find((WCHAR)ch) >= 0 )
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
void CTextRam::EditWordPos(CCurPos *sps, CCurPos *eps)
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
void CTextRam::MediaCopyStr(LPCWSTR str)
{
	if ( m_pDocument->m_pMediaCopyWnd == NULL ) {
		m_pDocument->m_pMediaCopyWnd = new CStatusDlg;
		m_pDocument->m_pMediaCopyWnd->m_bEdit = TRUE;
		m_pDocument->m_pMediaCopyWnd->m_OwnerType = 3;
		m_pDocument->m_pMediaCopyWnd->m_pValue = m_pDocument;
		m_pDocument->m_pMediaCopyWnd->m_Title = m_pDocument->m_ServerEntry.m_EntryName;
		m_pDocument->m_pMediaCopyWnd->Create(IDD_STATUS_DLG, CWnd::GetDesktopWindow());
		m_pDocument->m_pMediaCopyWnd->ShowWindow(SW_SHOW);
		::AfxGetMainWnd()->SetFocus();
	}

	m_pDocument->m_pMediaCopyWnd->AddStatusText(UniToTstr(str));
}
void CTextRam::MediaCopyDchar(DWORD ch)
{
	CStringW str;

	if ( (ch & 0xFFFF0000) != 0 )
		str = (WCHAR)(ch >> 16);
	str += (WCHAR)(ch & 0xFFFF);

	MediaCopyStr(str);
}
void CTextRam::MediaCopy(int y1, int y2)
{
	int x, y, sx, ex, tc;
	int len;
	LPCWSTR s;
	WCHAR tabc;
	CCharCell *vp;
	CStringW str;

	for ( y = y1 ; y <= y2 ; y++ ) {
		vp = GETVRAM(0, y);

		sx = 0;
		ex = m_Cols - 1;

		if ( vp->m_Vram.zoom != 0 ) {
			sx /= 2;
			ex /= 2;
		}

		while ( sx <= ex && vp[ex].IsEmpty() )
			ex--;

		if ( IS_2BYTE(vp[sx].m_Vram.mode) )
			sx++;

		tc = 0;
		for ( x = sx ; x <= ex ; x += len ) {
			if ( vp[x].IsEmpty() ) {
				s = L" ";
				len = 1;
			} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				s = vp[x];
				len = 2;
			} else {
				s = vp[x];
				len = 1;
			}

			if ( len == 1 && s[0] <= L' ' && s[1] == L'\0' ) {
				if ( tc++ == 0 )
					tabc = s[0];
				if ( (x % m_DefTab) == (m_DefTab - 1) && IsOptEnable(TO_RLSPCTAB) ) {
					str += (tc > 1 ? L'\t' : tabc);
					tc = 0;
				}
			} else {
				for ( ; tc > 0 ; tc-- )
					str += L' ';
				while ( *s != L'\0' )
					str += *(s++);
			}
		}
		for ( ; tc > 0 ; tc-- )
			str += L' ';

		str += L"\r\n";
	}

	MediaCopyStr(str);
}
void CTextRam::EditCopy(CCurPos sps, CCurPos eps, BOOL rectflag, BOOL lineflag)
{
	int i, x, y, sx, ex, tc;
	int x1, y1, x2, y2;
	int len;
	CCharCell *vp;
	CBuffer text, rtf, colrtf, fontrtf, mixrtf;
	LPCWSTR p, s;
	WCHAR tabc;
	int attr = 0;
	int fcol = 0;
	int bcol = 0;
	int fnum = 0;
	CDWordArray coltbl;
	CStringArrayExt fonttbl;
	CGrapWnd *pWnd;
	CRLoginView *pView = (CRLoginView *)GetAciveView();
	int fsize = 12;		// RTF default font size = 24 (1/2 point) = 12 point

	SetCalcPos(sps, &x1, &y1);
	SetCalcPos(eps, &x2, &y2);

	if ( rectflag && x1 > x2 ) {
		x = x1; x1 = x2; x2 = x;
	}

	if ( m_RtfMode != RTF_NONE ) {
		if ( m_RtfMode >= RTF_FONT ) {
			if ( pView != NULL )
				fsize = MulDiv(pView->m_CharHeight, 72, SCREEN_DPI_Y);

			fonttbl.Add(m_DefFontName[0]);
			fontrtf.AddFormat("{\\fonttbl{\\f0\\fmodern %s;}", TstrToMbs(m_DefFontName[0]));
		} else
			fontrtf = "{\\fonttbl\\f0;}\\f0";

		if ( m_RtfMode >= RTF_COLOR )
			colrtf = "{\\colortbl ;";

		rtf = "\r\n";
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

		if ( m_RtfMode >= RTF_JPEG ) {
			while ( sx <= ex && !IS_IMAGE(vp[ex].m_Vram.mode) && vp[ex].IsEmpty() )
				ex--;
		} else {
			while ( sx <= ex && vp[ex].IsEmpty() )
				ex--;
		}

		if ( IS_2BYTE(vp[sx].m_Vram.mode) )
			sx++;

		tc = 0;
		for ( x = sx ; x <= ex ; x += len ) {
			if ( vp[x].IsEmpty() ) {
				s = L" ";
				len = 1;
			} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				s = vp[x];
				len = 2;
			} else {
				s = vp[x];
				len = 1;
			}

			// TEXT Format
			if ( len == 1 && s[0] <= L' ' && s[1] == L'\0' ) {
				if ( tc++ == 0 )
					tabc = s[0];
				if ( (x % m_DefTab) == (m_DefTab - 1) && IsOptEnable(TO_RLSPCTAB) ) {
					text.PutWord(tc > 1 ? L'\t' : tabc);
					tc = 0;
				}
			} else {
				for ( ; tc > 0 ; tc-- )
					text.PutWord(L' ');
				for ( p = s ; *p != L'\0' ; p++ )
					text.PutWord(*p);
			}

			// RTF Format
			if ( m_RtfMode >= RTF_FONT ) {
				LPCTSTR fontname = NULL;

				if ( (i = vp[x].m_Vram.bank) >= 0 && i < CODE_MAX ) {
					if ( !m_FontTab[i].m_FontName[vp[x].m_Vram.font].IsEmpty() )
						fontname = m_FontTab[i].m_FontName[vp[x].m_Vram.font];
					else if ( !m_FontTab[i].m_FontName[0].IsEmpty() )
						fontname = m_FontTab[i].m_FontName[0];
					else if ( !m_DefFontName[vp[x].m_Vram.font].IsEmpty() )
						fontname = m_DefFontName[vp[x].m_Vram.font];
					else if ( !m_DefFontName[0].IsEmpty() )
						fontname = m_DefFontName[0];
				}

				if ( fontname != NULL ) {
					if ( (i = fonttbl.Find(fontname)) < 0 ) {
						i = (int)fonttbl.Add(fontname);
						fontrtf.AddFormat("{\\f%d\\fmodern %s;}", i, TstrToMbs(fontname));
					}
				} else
					i = 0;

				if ( i != fnum ) {
					fnum = i;
					rtf.AddFormat("\\f%d ", fnum);
				}
			}

			if ( m_RtfMode >= RTF_COLOR ) {
				int fcn, bcn;
				COLORREF frgb, brgb;

				fcn = vp[x].m_Vram.fcol;
				bcn = vp[x].m_Vram.bcol;

				if ( (vp[x].m_Vram.attr & ATT_EXTVRAM) != 0 && (vp->GetEatt() & EATT_FRGBCOL) != 0 )
					frgb = vp->GetFrgb();
				else
					frgb = m_ColTab[fcn];

				if ( (vp[x].m_Vram.attr & ATT_EXTVRAM) != 0 && (vp->GetEatt() & EATT_BRGBCOL) != 0 )
					brgb = vp->GetBrgb();
				else
					brgb = m_ColTab[bcn];

				if ( (vp[x].m_Vram.attr & ATT_REVS) != 0 ) {
					COLORREF trgb = frgb;
					frgb = brgb;
					brgb = trgb;

					i = fcn;
					fcn = bcn;
					bcn = i;
				}

				if ( fcn == m_DefAtt.std.fcol )
					i = 0;
				else {
					for ( i = 0 ; ; i++ ) {
						if ( i >= coltbl.GetSize() ) {
							i = (int)coltbl.Add((DWORD)frgb);
							colrtf.AddFormat("\\red%d\\green%d\\blue%d;", GetRValue(frgb), GetGValue(frgb), GetBValue(frgb));
							break;
						} else if ( coltbl[i] == (DWORD)frgb )
							break;
					}
					i++;	// 0=default color
				}

				if ( i != fcol ) {
					fcol = i;
					rtf.AddFormat("\\cf%d ", fcol);
				}

				if ( bcn == m_DefAtt.std.bcol )
					i = 0;
				else {
					for ( i = 0 ; ; i++ ) {
						if ( i >= coltbl.GetSize() ) {
							i = (int)coltbl.Add((DWORD)brgb);
							colrtf.AddFormat("\\red%d\\green%d\\blue%d;", GetRValue(brgb), GetGValue(brgb), GetBValue(brgb));
							break;
						} else if ( coltbl[i] == (DWORD)brgb )
							break;
					}
					i++;	// 0=default color
				}

				if ( i != bcol ) {
					bcol = i;
					rtf.AddFormat("\\highlight%d ", bcol);
				}
			}

			if ( m_RtfMode >= RTF_ATTR ) {
				if ( vp[x].m_Vram.attr != attr ) {
					i = vp[x].m_Vram.attr ^ attr;
					attr = vp[x].m_Vram.attr;
					if ( (i & ATT_BOLD) != 0 )
						rtf += ((attr & ATT_BOLD) != 0 ? "\\b " : "\\b0 ");
					if ( (i & ATT_ITALIC) != 0 )
						rtf += ((attr & ATT_ITALIC) != 0 ? "\\i " : "\\i0 ");
					if ( (i & ATT_LINE) != 0 )
						rtf += ((attr & ATT_LINE) != 0 ? "\\strike " : "\\strike0 ");
					if ( (i & ATT_STRESS) != 0 )
						rtf += ((attr & ATT_STRESS) != 0 ? "\\striked1 " : "\\striked0 ");

					if ( (i & (ATT_UNDER | ATT_DUNDER)) != 0 ) {
						if ( (attr & (ATT_UNDER | ATT_DUNDER)) == 0 )
							rtf += "\\ulnone ";
						else if ( (attr & ATT_DUNDER) != 0 )
							rtf += "\\uldb ";
						else if ( (attr & ATT_UNDER) != 0 )
							rtf += "\\ul ";
					}
				}
			}

			if ( m_RtfMode != RTF_NONE ) {
				if ( m_RtfMode >= RTF_JPEG && IS_IMAGE(vp[x].m_Vram.mode) && (pWnd = GetGrapWnd(vp[x].m_Vram.pack.image.id)) != NULL && pWnd->m_pActMap != NULL ) {
					for ( len = 1 ; (x + len) <= ex ; len++ ) {
						if ( !IS_IMAGE(vp[x + len].m_Vram.mode) )
							break;
						if ( vp[x].m_Vram.pack.image.id != vp[x + len].m_Vram.pack.image.id )
							break;
						if ( vp[x].m_Vram.pack.image.iy != vp[x + len].m_Vram.pack.image.iy )
							break;
						if ( (vp[x].m_Vram.pack.image.ix + len) != vp[x + len].m_Vram.pack.image.ix )
							break;
					}

					CBuffer buf;
					BITMAP map;
					HANDLE hBitmap = pWnd->GetBitmapBlock(RGB(255, 255, 255), //m_ColTab[m_DefAtt.std.bcol],
						vp[x].m_Vram.pack.image.ix, vp[x].m_Vram.pack.image.iy,
						vp[x].m_Vram.pack.image.ix + len, vp[x].m_Vram.pack.image.iy + 1,
						MulDiv(fsize, RTF_DPI, 72) * 10 / m_DefFontHw, MulDiv(fsize, RTF_DPI, 72),
						m_Cols, m_Lines, &map);

/*
								gdiplus		RTF			Word2010	WordPad		Taro2016
	Gdiplus::ImageFormatBMP		OK			\dibitmap	X			OK			OK			(BMPFILEHEADER Skip 14 Byte)
	Gdiplus::ImageFormatEMF		X			\emfblip
	Gdiplus::ImageFormatWMF		X			\wmetafileN
	Gdiplus::ImageFormatJPEG	OK			\jpegblip	OK			△			OK
	Gdiplus::ImageFormatPNG		OK			\pngblip	OK			△			OK
	Gdiplus::ImageFormatGIF		OK
	Gdiplus::ImageFormatTIFF	OK
	Gdiplus::ImageFormatEXIF	X
	Gdiplus::ImageFormatIcon	X

	GetBitmapBits							\wbitmapN	OK			OK			X			DDB
*/
					if ( m_RtfMode >= RTF_BITMAP ) {
						CBitmap Bitmap;
						Bitmap.Attach(hBitmap);
						int size = map.bmWidthBytes * map.bmHeight;
						Bitmap.GetBitmapBits(size, buf.PutSpc(size));
						Bitmap.Detach();

						rtf.AddFormat("{\\pict\\wbitmap%d\\picw%d\\pich%d\\picwgoal%d\\pichgoal%d\\wbmbitspixel%d\\wbmplanes%d\\wbmwidthbytes%d ",
								map.bmType, map.bmWidth, map.bmHeight,
								MulDiv(map.bmWidth, 72 * 20, RTF_DPI), MulDiv(map.bmHeight, 72 * 20, RTF_DPI),	// twips=1/20 point
								map.bmBitsPixel, map.bmPlanes, map.bmWidthBytes);

						rtf.PutHexBuf(buf.GetPtr(), buf.GetSize());
						rtf += '}';

					} else if ( pWnd->SaveImage(hBitmap, Gdiplus::ImageFormatJPEG, buf) ) {
						rtf.AddFormat("{\\pict\\jpegblip\\picw%d\\pich%d\\picwgoal%d\\pichgoal%d ",
							map.bmWidth, map.bmHeight,
							MulDiv(map.bmWidth, 72 * 20, RTF_DPI), MulDiv(map.bmHeight, 72 * 20, RTF_DPI));	// twips=1/20 point

						rtf.PutHexBuf(buf.GetPtr(), buf.GetSize());
						rtf += '}';

					} else {
						rtf += ' ';
					}

					if ( hBitmap != NULL )
						::DeleteObject(hBitmap);

				} else {
					for ( p = s ; *p != L'\0' ; p++ ) {
						if ( *p < 0x20 )
							rtf += ' ';
						else if ( _tcschr(_T("\\{}"), *p) != NULL )
							rtf.AddFormat("\\%c", *p);
						else if ( *p <= 0x7E )
							rtf += (CHAR)*p;
						else
							rtf.AddFormat("{\\uc\\u%d}", *p);
					}
				}
			}
		}

		for ( ; tc > 0 ; tc-- )
			text += L' ';

		if ( (y == y2 && lineflag) || (y < y2 && (x < m_Cols || (vp[0].m_Vram.attr & ATT_RETURN) != 0)) ) {
			text += L"\r\n";

			if ( m_RtfMode != RTF_NONE )
				rtf += "\\par\r\n";
		}
	}

	if ( m_RtfMode != RTF_NONE ) {
		rtf += '}';

		if ( m_RtfMode >= RTF_FONT )
			fontrtf.AddFormat("}\\f0\\fs%d\\sa0\\sl%d\\slmult1", fsize * 2, fsize * 20);		// \fs=1/2 point, sl=1/20 point

		if ( m_RtfMode >= RTF_COLOR )
			colrtf += '}';

		mixrtf = "{\\rtf1\\ansi\\deff0";
		mixrtf.Apend(fontrtf.GetPtr(), fontrtf.GetSize());
		mixrtf.Apend(colrtf.GetPtr(), colrtf.GetSize());
		mixrtf.Apend(rtf.GetPtr(), rtf.GetSize());
	}

	if ( IsOptEnable(TO_RLGAWL) != 0 ) {
		if ( m_ShellExec.Match(UniToTstr((LPCWSTR)text)) >= 0 )
			ShellExecuteW(AfxGetMainWnd()->GetSafeHwnd(), NULL, (LPCWSTR)text, NULL, NULL, SW_NORMAL);
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText((LPCWSTR)text, (m_RtfMode != RTF_NONE ? (LPCSTR)mixrtf : NULL));
}
void CTextRam::EditMark(CCurPos sps, CCurPos eps, BOOL rectflag, BOOL lineflag)
{
	int n, x, y, sx, ex;
	int x1, y1, x2, y2;
	CCharCell *vp;

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

		if ( IS_2BYTE(vp[sx].m_Vram.mode) )
			sx++;

		for ( x = sx ; x <= ex ; x += n ) {
			if ( vp[x].IsEmpty() ) {
				vp[x].m_Vram.attr |= ATT_SMARK;
				n = 1;
			} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				vp[x].m_Vram.attr |= ATT_SMARK;
				vp[x + 1].m_Vram.attr |= ATT_SMARK;
				n = 2;
			} else {
				vp[x].m_Vram.attr |= ATT_SMARK;
				n = 1;
			}
		}
	}
}
void CTextRam::GetLine(int sy, CString &str)
{
	int n, x, ex, mx, tc;
	WCHAR tabc;
	CCharCell *vp;
	LPCWSTR p;

	vp = GETVRAM(0, sy);
	mx = (vp->m_Vram.zoom != 0 ? (m_Cols / 2) : m_Cols);

	for ( ex = mx ; ex > 0 && vp[ex - 1].IsEmpty() ; )
		ex--;

	str.Empty();

	for ( x = tc = 0 ; x < ex ; x += n ) {
		if ( vp[x].IsEmpty() ) {
			p = L" ";
			n = 1;
		} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
			p = vp[x];
			n = 2;
		} else {
			p = vp[x];
			n = 1;
		}

		if ( n == 1 && p[0] <= L' ' && p[1] == L'\0' ) {
			if ( tc++ == 0 )
				tabc = p[0];
			if ( (x % m_DefTab) == (m_DefTab - 1) && IsOptEnable(TO_RLSPCTAB) ) {
				if ( tc > 1 )
					str += _T('\t');
				else
					str += tabc;
				tc = 0;
			}
		} else {
			for ( ; tc > 0 ; tc-- )
				str += _T(' ');
			while ( *p != L'\0' )
				str += *(p++);
		}
	}

	for ( ; tc > 0 ; tc-- )
		str += _T(' ');

//	if ( ex < mx || IsOptEnable(TO_RLLOGTIME) )
		str += _T("\r\n");
}
BOOL CTextRam::IsEmptyLine(int sy)
{
	int ex, mx;
	CCharCell *vp;

	vp = GETVRAM(0, sy);
	mx = (vp->m_Vram.zoom != 0 ? (m_Cols / 2) : m_Cols);

	for ( ex = mx ; ex > 0 && vp[ex - 1].IsEmpty() ; )
		ex--;

	return (ex <= 0 ? TRUE : FALSE);
}
void CTextRam::GetVram(int staX, int endX, int staY, int endY, CBuffer *pBuf)
{
	int n;
	int x, y, sx, ex;
	CCharCell *vp;
	LPCWSTR p;

	for ( y = staY ; y <= endY ; y++ ) {
		vp = GETVRAM(0, y);

		sx = (y == staY ? staX : 0);
		ex = (y == endY ? endX : (m_Cols - 1));

		if ( vp->m_Vram.zoom != 0 ) {
			sx = sx / 2;
			ex = ex / 2;
		}

		while ( sx <= ex && vp[ex].IsEmpty() )
			ex--;

		for ( x = sx ; x <= ex ; x += n ) {
			if ( vp[x].IsEmpty() ) {
				pBuf->PutWord(' ');
				n = 1;
			} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				for ( p = vp[x] ; *p != 0 ; p++ )
					pBuf->PutWord(*p);
				n = 2;
			} else {
				for ( p = vp[x] ; *p != 0 ; p++ )
					pBuf->PutWord(*p);
				n = 1;
			}
		}

		if ( y < endY && (x < m_Cols || (vp[0].m_Vram.attr & ATT_RETURN) != 0) ) {
			pBuf->PutWord(L'\r');
			pBuf->PutWord(L'\n');
		}
	}
}
BOOL CTextRam::SpeakLine(int cols, int line, CString &text, CArray<CCurPos, CCurPos &> &pos)
{
	int n, x, sx, ex;
	CCharCell *vp;
	LPCWSTR p;
	BOOL bContinue = FALSE;

	sx = cols;
	ex = m_Cols - 1;

	if ( sx > 0 && IS_2BYTE(GETVRAM(sx, line)->m_Vram.mode) )
		sx--;

	if ( sx > 0 ) {
		switch(IsWord(GetPos(sx, line))) {
		case 0:
		case 1:	// is alnum
			while ( IsWord(GetPos(sx - 1, line)) == 1 )
				DecPos(sx, line);
			break;
		case 2:	// ひらがな
			while ( IsWord(GetPos(sx - 1, line)) == 2 )
				DecPos(sx, line);
			while ( IsWord(GetPos(sx - 1, line)) == 4 )
				DecPos(sx, line);
			break;
		case 3:	// カタカナ
			while ( IsWord(GetPos(sx - 1, line)) == 3 )
				DecPos(sx, line);
			break;
		case 4:	// その他
			while ( IsWord(GetPos(sx - 1, line)) == 4 )
				DecPos(sx, line);
			break;
		}

		if ( IS_2BYTE(GETVRAM(sx, line)->m_Vram.mode) )
			DecPos(sx, line);
	}

	vp = GETVRAM(0, line);

	if ( vp->m_Vram.zoom != 0 )
		ex = ex / 2;

	while ( ex >= 0 && vp[ex].IsEmpty() )
		ex--;

	while ( sx <= ex && vp[sx].IsEmpty() )
		sx++;

	for ( x = sx ; x <= ex ; x += n ) {
		if ( vp[x].IsEmpty() ) {
			text += _T(' ');
			pos.Add(GetCalcPos(x, line));
			n = 1;
		} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
			for ( p = vp[x] ; *p != 0 ; p++ ) {
				text += *p;
				pos.Add(GetCalcPos(x, line));
			}
			n = 2;
		} else {
			for ( p = vp[x] ; *p != 0 ; p++ ) {
				text += *p;
				pos.Add(GetCalcPos(x, line));
			}
			n = 1;
		}
	}

	if ( (vp[0].m_Vram.attr & ATT_RETURN) == 0 )
		bContinue = TRUE;

	return bContinue;
}
BOOL CTextRam::SpeakCheck(CCurPos sPos, CCurPos ePos, LPCTSTR str)
{
	int n, x, y, tx, bx;
	int sx, sy, ex, ey;
	CCharCell *vp;
	LPCWSTR p;

	SetCalcPos(sPos, &sx, &sy);
	SetCalcPos(ePos, &ex, &ey);

	for ( y = sy ; y <= ey ; y++ ) {
		vp = GETVRAM(0, y);

		if ( y == ey )
			bx = ex;
		else {
			bx = m_Cols - 1;
			while ( bx >= 0 && vp[bx].IsEmpty() )
				bx--;
		}

		if ( y == sy )
			tx = sx;
		else {
			tx = 0;
			while ( tx <= bx && vp[tx].IsEmpty() )
				tx++;
		}


		for ( x = tx ; x <= bx ; x += n ) {
			if ( vp[x].IsEmpty() ) {
				if ( *(str++) != _T(' ') )
					return FALSE;
				n = 1;
			} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[x].m_Vram.mode) && IS_2BYTE(vp[x + 1].m_Vram.mode) && vp[x].Compare(vp[x + 1]) == 0 ) {
				for ( p = vp[x] ; *p != 0 ; p++ ) {
					if ( *(str++) != *p )
						return FALSE;
				}
				n = 2;
			} else {
				for ( p = vp[x] ; *p != 0 ; p++ ) {
					if ( *(str++) != *p )
						return FALSE;
				}
				n = 1;
			}
		}
	}

	return TRUE;
}

void CTextRam::DrawBitmap(CDC *pDestDC, CRect &rect, CDC *pSrcDC, int width, int height, DWORD dwRop)
{
	if ( rect.Width() <= width && rect.Height() <= height ) {
		pDestDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), pSrcDC, 0, 0, dwRop);
		return;
	}

	for ( int y = 0 ; y < rect.Height() ; y += height ) {
		for ( int x = 0 ; x < rect.Width() ; x += width )
			pDestDC->BitBlt(rect.left + x, rect.top + y,
				(x + width) < rect.Width() ? width : rect.Width() - x,
				(y + height) < rect.Height() ? height : rect.Height() - y, pSrcDC, 0, 0, dwRop);
	}
}
void CTextRam::DrawLine(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, BOOL bEraBack, struct DrawWork &prop, class CRLoginView *pView)
{
	int n, i, a;
	int c, o;
	CPen cPen[5], *oPen;
	LOGBRUSH LogBrush;
	CPoint point[2];
	CRect box(rect), tmp;
	static const DWORD PenExtTab[3][4]  = {	{ 3, 1, 3, 1 }, { 2, 1, 2, 1 },	{ 1, 1, 1, 1 } };
	static int SinTab[] = { 0, 87, 174, 259, 342, 423, 500, 574, 643, 707, 766, 819, 866, 906, 940, 966, 985, 996, 1000 };

	cPen[0].CreatePen(PS_SOLID, BD_SIZE, fc);

	if ( pView->m_CharHeight < BD_HALFSIZE )
		cPen[4].CreatePen(PS_SOLID, BD_SIZE, RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2));

	oPen = pDC->SelectObject(&(cPen[0]));

	LogBrush.lbColor = fc;
	LogBrush.lbStyle = BS_SOLID;

	if ( prop.zoom == 2 )
		box.bottom += pView->m_CharHeight;
	else if ( prop.zoom ==  3 )
		box.top -= pView->m_CharHeight;

	if ( bEraBack )
		pDC->FillSolidRect(rect, bc);

	for ( n = i = a = 0 ; n < prop.size ; n++ ) {
		while ( prop.pSpace[i] == 0 )
			i++;
		box.right = box.left + prop.pSpace[i];

		if ( box.Width() < 2 || box.Height() < 2 ) {
			pDC->FillSolidRect(box, RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2));

		} else if ( prop.pText[a] >= 0x2500 && prop.pText[a] <= 0x257F ) {		// U+002500 Box Drawing
			const BYTE *tab = BorderTab[prop.pText[a] - 0x2500];
			CPoint center((box.left + box.right) / 2, (box.top + box.bottom) / 2);
			int r, t, k;

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
						cPen[1].CreatePen(PS_USERSTYLE, BD_SIZE, &LogBrush, 4, PenExtTab[0]);
					pDC->SelectObject(&(cPen[1]));
					break;
				case BD_LINE1 | BD_DOT3:
				case BD_LINE2 | BD_DOT3:
					if ( cPen[2].m_hObject == NULL )
						cPen[2].CreatePen(PS_USERSTYLE, BD_SIZE, &LogBrush, 4, PenExtTab[1]);
					pDC->SelectObject(&(cPen[2]));
					break;
				case BD_LINE1 | BD_DOT4:
				case BD_LINE2 | BD_DOT4:
					if ( cPen[3].m_hObject == NULL )
						cPen[3].CreatePen(PS_USERSTYLE, BD_SIZE, &LogBrush, 4, PenExtTab[2]);
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
						if ( (r = center.x - box.left) > (box.bottom - center.y) )
							r = box.bottom - center.y;
						if ( r >= 48 ) t = 1; else if ( r >= 24 ) t = 2; else if ( r >= 12 ) t = 3; else if ( r >= 4 ) t = 6; else t = 9;
						pDC->MoveTo(center.x, box.bottom);
						for ( k = 0 ; k <= 18 ; k += t )
							pDC->LineTo(center.x - r + r * SinTab[18 - k] / 1000, center.y + r - r * SinTab[k] / 1000);
						pDC->LineTo(box.left - 1, center.y);
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
						if ( (r = box.right - center.x) > (center.y - box.top) )
							r = center.y - box.top;
						if ( r >= 48 ) t = 1; else if ( r >= 24 ) t = 2; else if ( r >= 12 ) t = 3; else if ( r >= 4 ) t = 6; else t = 9;
						pDC->MoveTo(center.x, box.top);
						for ( k = 0 ; k <= 18 ; k += t )
							pDC->LineTo(center.x + r - r * SinTab[18 - k] / 1000, center.y - r + r * SinTab[k] / 1000);
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
						if ( (r = center.x - box.left) > (center.y - box.top) )
							r = center.y - box.top;
						if ( r >= 48 ) t = 1; else if ( r >= 24 ) t = 2; else if ( r >= 12 ) t = 3; else if ( r >= 4 ) t = 6; else t = 9;
						pDC->MoveTo(center.x, box.top);
						for ( k = 0 ; k <= 18 ; k += t )
							pDC->LineTo(center.x - r + r * SinTab[18 - k] / 1000, center.y - r + r * SinTab[k] / 1000);
						pDC->LineTo(box.left - 1, center.y);
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
						if ( (r = box.right - center.x) > (box.bottom - center.y) )
							r = box.bottom - center.y;
						if ( r >= 48 ) t = 1; else if ( r >= 24 ) t = 2; else if ( r >= 12 ) t = 3; else if ( r >= 4 ) t = 6; else t = 9;
						pDC->MoveTo(center.x, box.bottom);
						for ( k = 0 ; k <= 18 ; k += t )
							pDC->LineTo(center.x + r - r * SinTab[18 - k] / 1000, center.y + r - r * SinTab[k] / 1000);
						pDC->LineTo(box.right, center.y);
						break;
					case BD_LINE5:	// ╲
						pDC->MoveTo(box.left, box.top);
						pDC->LineTo(box.right - 1, box.bottom - 1);
						break;
					}
					break;
				}
			}

		} else if ( prop.pText[a] >= 0x2580 && prop.pText[a] <= 0x259F ) {		// U+002580 Block Elements
			CPoint center((box.left + box.right) / 2, (box.top + box.bottom) / 2);
			int line = (box.Width() < 8 ? 1 : box.Width() / 8);

			for ( c = 0 ; c < 2 ; c++ ) {
				int style = FillboxTab[prop.pText[a] - 0x2580][c];

				if ( style == FI_NONE )
					continue;

				switch(style & 07000) {		// left
				case FI_L_LEFT:	tmp.left = box.left; break;
				case FI_L_CENTER:	tmp.left = center.x; break;
				case FI_L_RIGHT:	tmp.left = box.right - line; break;
				}

				switch(style & 00700) {		// right
				case FI_R_LEFT:	tmp.right = box.left + line; break;
				case FI_R_CENTER:	tmp.right = center.x; break;
				case FI_R_RIGHT:	tmp.right = box.right; break;
				case FI_R_CALC:
					// U+2589 - U+258F
					if ( (line = box.Width() * (8 - (prop.pText[a] - 0x2588)) / 8) <= 0 ) line = 1;
					tmp.right = box.left + line;
					break;
				}

				switch(style & 00070) {		// top
				case FI_T_TOP:		tmp.top = box.top; break;
				case FI_T_CENTER:	tmp.top = center.y; break;
				case FI_T_BOTTOM:	tmp.top = box.bottom - line; break;
				case FI_T_CALC:
					// U+2581 - U+2587
					if ( (line = box.Height() * (prop.pText[a] - 0x2580) / 8) <= 0 ) line = 1;
					tmp.top = box.bottom - line;
					break;
				}

				switch(style & 00007) {		// bottom
				case FI_B_TOP:		tmp.bottom = box.top + line; break;
				case FI_B_CENTER:	tmp.bottom = center.y; break;
				case FI_B_BOTTOM:	tmp.bottom = box.bottom; break;
				}

				switch(prop.pText[a]) {
				case 0x2591:
				case 0x2592:
				case 0x2593:
					{
						CDC TempDC;
						CBitmap BitMap, *pOld;
						BITMAP MapInfo;

						TempDC.CreateCompatibleDC(pDC);
						((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(IDB_HATCH3 - (prop.pText[a] - 0x2591)), BitMap);
						BitMap.GetBitmap(&MapInfo);
						pOld = TempDC.SelectObject(&BitMap);

						if ( bEraBack ) {
							pDC->SetTextColor(fc);
							pDC->SetBkColor(bc);
							DrawBitmap(pDC, tmp, &TempDC, MapInfo.bmWidth, MapInfo.bmHeight, SRCCOPY);

						} else {
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(255, 255, 255));
							DrawBitmap(pDC, tmp, &TempDC, MapInfo.bmWidth, MapInfo.bmHeight, SRCAND);

							pDC->SetTextColor(fc);
							pDC->SetBkColor(RGB(0, 0, 0));
							DrawBitmap(pDC, tmp, &TempDC, MapInfo.bmWidth, MapInfo.bmHeight, SRCPAINT);

							pDC->SetBkColor(bc);
						}

						TempDC.SelectObject(pOld);
					}
					//pDC->FillSolidRect(tmp, RGB((GetRValue(fc) + GetRValue(bc) * 2) / 3, (GetGValue(fc) + GetGValue(bc) * 2) / 3, (GetBValue(fc) + GetBValue(bc) * 2) / 3));
					//pDC->FillSolidRect(tmp, RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2));
					//pDC->FillSolidRect(tmp, RGB((GetRValue(fc) * 2 + GetRValue(bc)) / 3, (GetGValue(fc) * 2 + GetGValue(bc)) / 3, (GetBValue(fc) * 2 + GetBValue(bc)) / 3));
					break;
				default:
					pDC->FillSolidRect(tmp, fc);
					break;
				}
			}
		}

		box.left += prop.pSpace[i];
		a = ++i;
	}

	pDC->SelectObject(oPen);
}
void CTextRam::DrawChar(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, BOOL bEraBack, struct DrawWork &prop, class CRLoginView *pView)
{
	int n, i, a, c;
	int x, y, nw, ow;
	int width, height, style = FONTSTYLE_NONE;
	int type = 0;
	UINT mode;
	CSize sz;
	CRect box(rect), frame(rect), save(rect);
	CStringW str;
	CFontChacheNode *pFontCache;
	CFontNode *pFontNode = &(m_FontTab[prop.bank]);
	CDC workDC, mirDC, *pSaveDC;
	CBitmap *pOldMap, *pOldMirMap, MirMap;

	mode = ETO_CLIPPED;
	if ( bEraBack )
		mode |= ETO_OPAQUE;

	width  = pView->m_CharWidth  * prop.csz * (prop.zoom == 0 ? 1 : 2);
	height = pView->m_CharHeight * (prop.zoom <= 1 ? 1 : 2);

	if ( prop.csz > 1 )
		style |= FONTSTYLE_FULLWIDTH;

	if ( (prop.attr & ATT_BOLD) != 0 && IsOptEnable(TO_RLBOLD) )
		style |= FONTSTYLE_BOLD;

	if ( (prop.attr & ATT_ITALIC) != 0 ) {
		style |= FONTSTYLE_ITALIC;
		width = width * 80 / 100;
	}

	if ( ATT_EXTYPE(prop.attr) == ATT_MIRROR ) {
		mirDC.CreateCompatibleDC(pDC);
		MirMap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
		pOldMirMap = mirDC.SelectObject(&MirMap);

		if ( !bEraBack )
			mirDC.StretchBlt(rect.Width() - 1, 0, 0 - rect.Width(), rect.Height(), pDC, rect.left, rect.top, rect.Width(), rect.Height(), SRCCOPY);
		mirDC.SetBkMode(TRANSPARENT);

		pSaveDC = pDC;
		pDC = &mirDC;

		rect.SetRect(0, 0, save.Width(), save.Height());
		box = frame = rect;
	}

	pFontCache = pFontNode->GetFont(width, height, style, prop.font, this);
	pDC->SelectObject(pFontCache->m_pFont);
	pDC->SetTextColor(fc);
	pDC->SetBkColor(bc);

	if ( (prop.attr & ATT_EMOJI) != 0 )
		type = 3;
	else if ( pFontCache->m_bFixed )
		type = 1;
	else if ( IsOptEnable(TO_RLWORDPP) )
		type = 2;

	if ( pFontNode->SetFontImage(width, height) ) {
		// Gaiji Mode
		// 文字単位で縮小、固定幅、センターリング
		workDC.CreateCompatibleDC(pDC);
		pOldMap = workDC.SelectObject(&(pFontNode->m_FontMap));

		for ( n = i = a = 0 ; n < prop.size ; n++ ) {
			while ( prop.pSpace[i] == 0 )
				i++;
			box.right = box.left + prop.pSpace[i];

			str.Empty();
			while ( a <= i )
				str += prop.pText[a++];

			if ( (c = str[0] - 0x20) >= 0 && c < USFTCMAX && (pFontNode->m_UserFontDef[c / 8] & (1 << (c % 8))) != 0 ) {
				if ( pFontNode->m_MapType == FNT_BITMAP_COLOR ) {
					if ( pFontNode->m_pTransColor != NULL && pFontNode->m_pTransColor[c] != (-1) ) {
						if ( bEraBack )
							pDC->FillSolidRect(box, bc);
						pDC->TransparentBlt(box.left, box.top, box.Width(), box.Height(), &workDC, pFontNode->m_FontWidth * c, (prop.zoom == 3 ? pView->m_CharHeight : 0), box.Width(), box.Height(), pFontNode->m_pTransColor[c]);
					} else
						pDC->BitBlt(box.left, box.top, box.Width(), box.Height(), &workDC, pFontNode->m_FontWidth * c, (prop.zoom == 3 ? pView->m_CharHeight : 0), SRCCOPY);
				} else if ( bEraBack ) {
					pDC->BitBlt(box.left, box.top, box.Width(), box.Height(), &workDC, pFontNode->m_FontWidth * c, (prop.zoom == 3 ? pView->m_CharHeight : 0), SRCCOPY);
				} else {
					pDC->SetTextColor(RGB(0, 0, 0));
					pDC->SetBkColor(RGB(255, 255, 255));
					pDC->BitBlt(box.left, box.top, box.Width(), box.Height(), &workDC, pFontNode->m_FontWidth * c, (prop.zoom == 3 ? pView->m_CharHeight : 0), SRCAND);
					pDC->SetTextColor(fc);
					pDC->SetBkColor(RGB(0, 0, 0));
					pDC->BitBlt(box.left, box.top, box.Width(), box.Height(), &workDC, pFontNode->m_FontWidth * c, (prop.zoom == 3 ? pView->m_CharHeight : 0), SRCPAINT);
					pDC->SetBkColor(bc);
				}

			} else {
				sz = pDC->GetTextExtent(str);
				if ( (nw = (box.Width() < sz.cx ? (width * box.Width() / sz.cx) : width)) <= 0 )
					nw = 1;

				if ( nw != width ) {
					pFontCache = pFontNode->GetFont(nw, height, style, prop.font, this);
					pDC->SelectObject(pFontCache->m_pFont);
					sz = pDC->GetTextExtent(str);
				}

				x = box.left + pView->m_CharWidth * pFontNode->m_OffsetW / 100 + (box.Width() - sz.cx) / 2;
				y = box.top - pView->m_CharHeight * pFontNode->m_OffsetH / 100 - (prop.zoom == 3 ? pView->m_CharHeight : 0);

				pDC->ExtTextOutW(x, y - pFontCache->m_Offset, mode, box, str, NULL);

				if ( nw != width ) {
					pFontCache = pFontNode->GetFont(width, height, style, prop.font, this);
					pDC->SelectObject(pFontCache->m_pFont);
				}
			}

			box.left += prop.pSpace[i++];
		}

		workDC.SelectObject(pOldMap);
		workDC.DeleteDC();

	} else if ( (prop.attr & ATT_RTOL) != 0 ) {
		// Right to Left
		// 文字列単位で縮小・拡大、プロポーショナル
		x = box.left + pView->m_CharWidth  * pFontNode->m_OffsetW / 100;
		y = box.top  - pView->m_CharHeight * pFontNode->m_OffsetH / 100 - (prop.zoom == 3 ? pView->m_CharHeight : 0);

		sz = pDC->GetTextExtent(prop.pText, prop.tlen);
		if ( sz.cx <= 0 ) sz.cx = 1;

		pFontCache = pFontNode->GetFont(width * box.Width() / sz.cx, height, style, prop.font, this);
		pDC->SelectObject(pFontCache->m_pFont);
		sz = pDC->GetTextExtent(prop.pText, prop.tlen);

		pDC->SetTextCharacterExtra((box.Width() - sz.cx) / prop.size);
		pDC->ExtTextOutW(x, y - pFontCache->m_Offset, mode, box, prop.pText, prop.tlen, NULL);
		pDC->SetTextCharacterExtra(0);

		pView->m_ClipUpdateLine = TRUE;

	} else if ( type == 1 ) {
		// Fixed Draw
		// オプションで縮小する必要があればCenter Drawに変更、固定幅
		x = box.left + pView->m_CharWidth  * pFontNode->m_OffsetW / 100;
		y = box.top  - pView->m_CharHeight * pFontNode->m_OffsetH / 100 - (prop.zoom == 3 ? pView->m_CharHeight : 0);

		if ( !IsOptEnable(TO_RLUNIAHF) ) {
			sz = pDC->GetTextExtent(prop.pText, prop.tlen);
			if ( box.Width() < sz.cx ) {
				if ( prop.csz == 2 && (box.Width() * 50 / sz.cx) == 25 ) {	// 50-51%
					// 全角のサイズ指定を半角で行う場合の救済処置
					pFontCache = pFontNode->GetFont(width * box.Width() / sz.cx, height, style, prop.font, this);
					pDC->SelectObject(pFontCache->m_pFont);
				} else {
					goto DRAWCHAR;
				}
			}
		}

		pDC->ExtTextOutW(x, y - pFontCache->m_Offset, mode, box, prop.pText, prop.tlen, prop.pSpace);

	} else if ( type == 2 ) {
		// Proportion Draw
		// 文字列単位で縮小、プロポーショナル、センターリング
		sz = pDC->GetTextExtent(prop.pText, prop.tlen);

		if ( box.Width() < sz.cx ) {
			pFontCache = pFontNode->GetFont(width * box.Width() / sz.cx, height, style, prop.font, this);
			pDC->SelectObject(pFontCache->m_pFont);
			sz = pDC->GetTextExtent(prop.pText, prop.tlen);
		}

		ow = 0;
		nw = sz.cx;

		for ( n = i = a = 0 ; n < prop.size ; n++ ) {
			while ( prop.pSpace[i] == 0 )
				i++;

			str.Empty();
			while ( a <= i )
				str += prop.pText[a++];

			sz = pDC->GetTextExtent(str);
			ow += sz.cx;

			// 文字幅(box.right)を計算
			if ( nw > 0 ) {
				x = ow * frame.Width() / nw;						// 相対幅
				y = rect.left + (n + 2) * rect.Width() / prop.size;	// 固定位置+2
				if ( (frame.left + x) >= y && nw > ow ) {
					nw -= ow;
					ow = 0;
					box.right = box.left + sz.cx;
					frame.left = box.right;
				} else
					box.right = frame.left + x;						// 均等割
			} else
				box.right = box.left + sz.cx;						// 表示幅

			x = box.left + pView->m_CharWidth  * pFontNode->m_OffsetW / 100 + (box.Width() - sz.cx) / 2;
			y = box.top  - pView->m_CharHeight * pFontNode->m_OffsetH / 100 - (prop.zoom == 3 ? pView->m_CharHeight : 0);

			pDC->ExtTextOutW(x, y - pFontCache->m_Offset, mode, box, str, NULL);

			for ( x = c = 0 ; x < prop.csz ; x++ ) {
				y = box.Width() * (x + 1) / prop.csz;
				pView->SetCellSize(prop.cols + (n * prop.csz) + x, prop.line, y - c);
				c = y;
			}

			prop.pSpace[i] = box.Width();
			box.left = box.right;
			i++;
		}

		pView->m_ClipUpdateLine = TRUE;

	} else {
		// Center Draw
		// 文字単位で縮小、固定幅、センターリング
		DRAWCHAR:
		for ( n = i = a = 0 ; n < prop.size ; n++ ) { 
			while ( prop.pSpace[i] == 0 )
				i++;
			box.right = box.left + prop.pSpace[i];

			str.Empty();
			while ( a <= i )
				str += prop.pText[a++];

#ifdef	USE_DIRECTWRITE
			// Try DirectWrite Color Emoji Draw
			if ( type != 3 || !((CRLoginApp *)::AfxGetApp())->DrawEmoji(pDC, box, str, fc, bc, bEraBack, height, prop.zoom == 3 ? TRUE : FALSE) ) {
#endif
				sz = pDC->GetTextExtent(str);
				if ( (nw = (box.Width() < sz.cx ? (width * box.Width() / sz.cx) : width)) <= 0 )
					nw = 1;

				if ( nw != width ) {
					pFontCache = pFontNode->GetFont(nw, height, style, prop.font, this);
					pDC->SelectObject(pFontCache->m_pFont);
					sz = pDC->GetTextExtent(str);
				}

				x = box.left + pView->m_CharWidth * pFontNode->m_OffsetW / 100 + (box.Width() - sz.cx) / 2;
				y = box.top - pView->m_CharHeight * pFontNode->m_OffsetH / 100 - (prop.zoom == 3 ? pView->m_CharHeight : 0);

				pDC->ExtTextOutW(x, y - pFontCache->m_Offset, mode, box, str, NULL);

				if ( nw != width ) {
					pFontCache = pFontNode->GetFont(width, height, style, prop.font, this);
					pDC->SelectObject(pFontCache->m_pFont);
				}
#ifdef	USE_DIRECTWRITE
			}
#endif

			box.left += prop.pSpace[i++];
		}
	}
	
	if ( ATT_EXTYPE(prop.attr) == ATT_MIRROR ) {
		rect = save;
		pSaveDC->StretchBlt(rect.left + rect.Width() - 1, rect.top, 0 - rect.Width(), rect.Height(), pDC, 0, 0, rect.Width(), rect.Height(), SRCCOPY);
		mirDC.SelectObject(pOldMirMap);
		mirDC.DeleteDC();
	}
}
void CTextRam::DrawHoriLine(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, struct DrawWork &prop, class CRLoginView *pView)
{
	COLORREF hc = RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2);
	CPen fPen(PS_SOLID, 1, fc);
	CPen cPen(PS_SOLID, 1, pView->m_CharHeight < BD_HALFSIZE ? hc : fc);
	CPen hPen(PS_SOLID, 1, hc);
	CPen *oPen = pDC->SelectObject(&cPen);
//	int OldRop2 = pDC->SetROP2(R2_MERGEPENNOT);
	POINT point[4];
	point[0].x = rect.left;
	point[1].x = rect.right;

	if ( (prop.attr & ATT_OVER) != 0 ) {
		point[0].y = rect.top;
		point[1].y = rect.top;
		pDC->Polyline(point, 2);
	} else if ( (prop.attr & ATT_DOVER) != 0 ) {
		point[0].y = rect.top;
		point[1].y = rect.top;
		pDC->Polyline(point, 2);
		point[2].x = ((prop.attr & ATT_LDLINE) != 0 ? (rect.left + 2) : rect.left);
		point[2].y = rect.top + 2;
		point[3].x = ((prop.attr & ATT_RDLINE) != 0 ? (rect.right - 3) : rect.right);
		point[3].y = rect.top + 2;
		pDC->SelectObject(&hPen);
		pDC->Polyline(&(point[2]), 2);
	}

	if ( (prop.attr & ATT_LINE) != 0 ) {
		point[0].y = (rect.top + rect.bottom) / 2;
		point[1].y = (rect.top + rect.bottom) / 2;
		pDC->SelectObject(&cPen);
		pDC->Polyline(point, 2);
	} else if ( (prop.attr & ATT_STRESS) != 0 ) {
		point[0].y = (rect.top + rect.bottom) / 2 - 1;
		point[1].y = (rect.top + rect.bottom) / 2 - 1;
		pDC->SelectObject(&cPen);
		pDC->Polyline(point, 2);
		point[0].y = (rect.top + rect.bottom) / 2 + 1;
		point[1].y = (rect.top + rect.bottom) / 2 + 1;
		pDC->Polyline(point, 2);
	}

	if ( (prop.attr & ATT_UNDER) != 0 ) {
		point[0].y = rect.bottom - 2;
		point[1].y = rect.bottom - 2;
		pDC->SelectObject(&fPen);
		pDC->Polyline(point, 2);
	} else if ( (prop.attr & ATT_SUNDER) != 0 ) {
		point[0].y = rect.bottom - 1;
		point[1].y = rect.bottom - 1;
		pDC->SelectObject(&cPen);
		pDC->Polyline(point, 2);
	} else if ( (prop.attr & ATT_DUNDER) != 0 ) {
		point[0].y = rect.bottom - 1;
		point[1].y = rect.bottom - 1;
		pDC->SelectObject(&cPen);
		pDC->Polyline(point, 2);
		point[2].x = ((prop.attr & ATT_LDLINE) != 0 ? (rect.left + 2) : rect.left);
		point[2].y = rect.bottom - 3;
		point[3].x = ((prop.attr & ATT_RDLINE) != 0 ? (rect.right - 3) : rect.right);
		point[3].y = rect.bottom - 3;
		pDC->SelectObject(&hPen);
		pDC->Polyline(&(point[2]), 2);
	}

//	pDC->SetROP2(OldRop2);
	pDC->SelectObject(oPen);
}
void CTextRam::DrawVertLine(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, struct DrawWork &prop, class CRLoginView *pView)
{
	int n, i;
	int x, w, c;
	COLORREF hc = RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2);
	CPen cPen(PS_SOLID, 1, pView->m_CharHeight < BD_HALFSIZE ? hc : fc);
	CPen hPen(PS_SOLID, 1, hc);
	CPen *oPen = pDC->SelectObject(&cPen);
//	int OldRop2 = pDC->SetROP2(R2_MERGEPENNOT);
	POINT point[9];

	x = rect.left;
	for ( n = i = 0 ; n < prop.size ; n++, i++ ) {
		while ( prop.pSpace[i] == 0 )
			i++;
		w = prop.pSpace[i];

		if ( (prop.attr & ATT_FRAME) != 0 ) {
			point[0].x = x; point[0].y = rect.top + 1;
			point[1].x = x + w - 2; point[1].y = rect.top + 1;
			point[2].x = x + w - 2; point[2].y = rect.bottom - 1;
			point[3].x = x; point[3].y = rect.bottom - 1;
			point[4].x = x; point[4].y = rect.top + 1;
			pDC->SelectObject(&cPen);
			pDC->Polyline(point, 5);
		} else if ( (prop.attr & ATT_CIRCLE) != 0 ) {
			c = w / 3;
			point[0].x = x + c; point[0].y = rect.top + 1;
			point[1].x = x + w - 2 - c; point[1].y = rect.top + 1;
			point[2].x = x + w - 2; point[2].y = rect.top + 1 + c;
			point[3].x = x + w - 2; point[3].y = rect.bottom - 1 - c;
			point[4].x = x + w - 2 - c; point[4].y = rect.bottom - 1;
			point[5].x = x + c; point[5].y = rect.bottom - 1;
			point[6].x = x; point[6].y = rect.bottom - 1 - c;
			point[7].x = x; point[7].y = rect.top + 1 + c;
			point[8].x = x + c; point[8].y = rect.top + 1;
			pDC->SelectObject(&cPen);
			pDC->Polyline(point, 9);
		}

		if ( (prop.attr & ATT_RSLINE) != 0 ) {
			point[0].x = x + w - 1; point[0].y = rect.top;
			point[1].x = x + w - 1; point[1].y = rect.bottom;
			pDC->SelectObject(&cPen);
			pDC->Polyline(point, 2);
		} else if ( (prop.attr & ATT_RDLINE) != 0 ) {
			point[0].x = x + w - 1; point[0].y = rect.top;
			point[1].x = x + w - 1; point[1].y = rect.bottom;
			pDC->SelectObject(&cPen);
			pDC->Polyline(point, 2);
			point[0].x = x + w - 3;
			point[0].y = ((prop.attr & ATT_DOVER) != 0 ? (rect.top + 2) : rect.top);
			point[1].x = x + w - 3;
			point[1].y = ((prop.attr & ATT_DUNDER) != 0 ? (rect.bottom - 2) : (rect.bottom));
			pDC->SelectObject(&hPen);
			pDC->Polyline(point, 2);
		}

		if ( (prop.attr & ATT_LSLINE) != 0 ) {
			point[0].x = x; point[0].y = rect.top;
			point[1].x = x; point[1].y = rect.bottom;
			pDC->SelectObject(&cPen);
			pDC->Polyline(point, 2);
		} else if ( (prop.attr & ATT_LDLINE) != 0 ) {
			point[0].x = x; point[0].y = rect.top;
			point[1].x = x; point[1].y = rect.bottom;
			pDC->SelectObject(&cPen);
			pDC->Polyline(point, 2);
			point[0].x = x + 2;
			point[0].y = ((prop.attr & ATT_DOVER) != 0 ? (rect.top + 2) : rect.top);
			point[1].x = x + 2;
			point[1].y = ((prop.attr & ATT_DUNDER) != 0 ? (rect.bottom - 2) : (rect.bottom));
			pDC->SelectObject(&hPen);
			pDC->Polyline(point, 2);
		}

		x += w;
	}

//	pDC->SetROP2(OldRop2);
	pDC->SelectObject(oPen);
}
void CTextRam::DrawOverChar(CDC *pDC, CRect &rect, COLORREF fc, COLORREF bc, struct DrawWork &prop, class CRLoginView *pView)
{
	int n, i, a, x, y, w;
	CFontChacheNode *pFontCache;
	CFontNode *pFontNode = &(m_FontTab[prop.bank]);
	CString str;
	LPCTSTR over;
	int width  = pView->m_CharWidth  * (prop.zoom == 0 ? 1 : 2);
	int height = pView->m_CharHeight * (prop.zoom <= 1 ? 1 : 2);
	int style  = (prop.attr & ATT_BOLD) != 0 && IsOptEnable(TO_RLBOLD) ? FONTSTYLE_BOLD : FONTSTYLE_NONE;
	CSize sz;
	CRect box;

	if ( (prop.attr & ATT_ITALIC) != 0 ) {
		style |= FONTSTYLE_ITALIC;
		width = width * 80 / 100;
	}

	pFontCache = pFontNode->GetFont(width, height, style, prop.font, this);
	pDC->SelectObject(pFontCache->m_pFont);
	pDC->SetTextColor(fc);
	int OldBkMode = pDC->SetBkMode(TRANSPARENT);

	box.left = rect.left;
	box.top = rect.top;
	box.bottom = rect.bottom;

	for ( n = i = a = 0 ; n < prop.size ; n++, i++ ) {
		while ( prop.pSpace[i] == 0 )
			i++;
		w = prop.pSpace[i];

		str.Empty();
		while ( a <= i )
			str += prop.pText[a++];

		if ( (over = pFontNode->OverCharStr(str)) != NULL ) {
			sz = pDC->GetTextExtent(over);
			box.right = box.left + w;

			x = box.left + pView->m_CharWidth * pFontNode->m_OffsetW / 100 + (box.Width() - sz.cx) / 2;
			y = box.top - pView->m_CharHeight * pFontNode->m_OffsetH / 100 - (prop.zoom == 3 ? pView->m_CharHeight : 0);

			pDC->ExtTextOutW(x, y, ETO_CLIPPED, box, over, (int)_tcslen(over), NULL);
		}

		box.left += w;
	}

	pDC->SetBkMode(OldBkMode);
}
void CTextRam::DrawString(CDC *pDC, CRect &rect, struct DrawWork &prop, class CRLoginView *pView)
{
	BOOL bRevs = FALSE;
	BOOL bEraBack = FALSE;
	COLORREF fcol, bcol, tcol;
	CGrapWnd *pWnd;

	// Text Color & Back Color
	if ( (prop.attr & ATT_BOLD) != 0 && (!IsOptEnable(TO_RLBOLD) || IsOptEnable(TO_RLBOLDHC)) ) {
		if ( (prop.eatt & EATT_FRGBCOL) != 0 )
			fcol = RGB((GetRValue(prop.frgb) + 255) / 2, (GetGValue(prop.frgb) + 255) / 2, (GetBValue(prop.frgb) + 255) / 2);
		else if ( prop.fcol < 16 )
			fcol = m_ColTab[prop.fcol ^ 0x08];
		else
			fcol = RGB((GetRValue(m_ColTab[prop.fcol]) + 255) / 2, (GetGValue(m_ColTab[prop.fcol]) + 255) / 2, (GetBValue(m_ColTab[prop.fcol]) + 255) / 2);
	} else if ( (prop.eatt & EATT_FRGBCOL) != 0 )
		fcol = prop.frgb;
	else
		fcol = m_ColTab[prop.fcol];

	if ( (prop.eatt & EATT_BRGBCOL) != 0 )
		bcol = prop.brgb;
	else
		bcol = m_ColTab[prop.bcol];

	if ( (prop.attr & ATT_REVS) != 0 ) {
		tcol = fcol;
		fcol = bcol;
		bcol = tcol;
		bRevs = TRUE;
	}

	if ( (prop.attr & ATT_BLINK) != 0 ) {
		if ( !prop.print )
			pView->AddDeleyInval((prop.aclock / BLINK_TIME + 1) * BLINK_TIME, rect);

		if ( ((prop.aclock / BLINK_TIME) & 1) != 0 ) {
			tcol = fcol;
			fcol = bcol;
			bcol = tcol;
			bRevs = TRUE;
		}

	} else if ( (prop.attr & ATT_SBLINK) != 0 ) {
		if ( !prop.print )
			pView->AddDeleyInval((prop.aclock / SBLINK_TIME + 1) * SBLINK_TIME, rect);

		if ( ((prop.aclock / SBLINK_TIME) & 1) != 0 ) {
			tcol = fcol;
			fcol = bcol;
			bcol = tcol;
			bRevs = TRUE;
		}
	}

	if ( (prop.attr & ATT_SECRET) != 0 )
		fcol = bcol;

	if ( (prop.attr & ATT_HALF) != 0 )
		bcol = RGB((GetRValue(fcol) + GetRValue(bcol)) / 2, (GetGValue(fcol) + GetGValue(bcol)) / 2, (GetBValue(fcol) + GetBValue(bcol)) / 2);

	if ( (prop.attr & ATT_SMARK) != 0 ) {
		tcol = fcol;
		fcol = bcol;
		bcol = tcol;
		if ( bRevs )
			bcol = RGB((GetRValue(fcol) + GetRValue(bcol)) / 2, (GetGValue(fcol) + GetGValue(bcol)) / 2, (GetBValue(fcol) + GetBValue(bcol)) / 2);
		bcol = RGB((GetRValue(m_MarkColor) + GetRValue(bcol)) / 2, (GetGValue(m_MarkColor) + GetGValue(bcol)) / 2, (GetBValue(m_MarkColor) + GetBValue(bcol)) / 2);
		bRevs = TRUE;
	}

	if ( (prop.attr & ATT_CLIP) != 0 ) {
		tcol = fcol;
		fcol = bcol;
		bcol = tcol;
		if ( bRevs )
			bcol = RGB((GetRValue(fcol) + GetRValue(bcol)) / 2, (GetGValue(fcol) + GetGValue(bcol)) / 2, (GetBValue(fcol) + GetBValue(bcol)) / 2);
		bRevs = TRUE;
	}

	if ( IsOptEnable(TO_RLBACKHALF) && pView->GetFocus()->GetSafeHwnd() != pView->GetSafeHwnd() )
		fcol = RGB((GetRValue(fcol) * 2 + GetRValue(bcol)) / 3, (GetGValue(fcol) * 2 + GetGValue(bcol)) / 3, (GetBValue(fcol) * 2 + GetBValue(bcol)) / 3);

	if ( bRevs || prop.bcol != m_DefAtt.std.bcol )
		bEraBack = TRUE;

	if ( pView->m_HaveBack ) {	// pView->m_pBitmap != NULL ) {
		if ( bEraBack ) {
			CDC workDC;
			CBitmap workMap, *pOldMap;
			BLENDFUNCTION bf;

			workDC.CreateCompatibleDC(pDC);
			workMap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
			pOldMap = (CBitmap *)workDC.SelectObject(&workMap);
			workDC.FillSolidRect(0, 0, rect.Width(), rect.Height(), bcol);

			ZeroMemory(&bf, sizeof(bf));
			bf.BlendOp = AC_SRC_OVER;
			bf.SourceConstantAlpha = m_BitMapBlend;

			pDC->AlphaBlend(rect.left, rect.top, rect.Width(), rect.Height(), &workDC, 0, 0, rect.Width(), rect.Height(), bf);
			workDC.SelectObject(pOldMap);
		}
		bEraBack = FALSE;
	} else
		bEraBack = TRUE;

	if ( prop.idx != (-1) ) {
		// Image Draw
		if ( (pWnd = GetGrapWnd(prop.idx)) != NULL ) {
			pWnd->DrawBlock(pDC, rect, bcol, bEraBack, prop.stx, prop.sty, prop.edx, prop.sty + 1,
				pView->m_Width, pView->m_Height, pView->m_CharWidth, pView->m_CharHeight, pView->m_Cols, pView->m_Lines);
			if ( pWnd->IsGifAnime() && !prop.print )
				pView->AddDeleyInval(pWnd->m_GifAnimeClock, rect);
		} else if ( bEraBack )
			pDC->FillSolidRect(rect, bcol);

		if ( bRevs )
			pDC->InvertRect(rect);

	} else if ( prop.bank < 0 || prop.bank >= CODE_MAX ) {
		// Blank Draw
		if ( bEraBack )
			pDC->FillSolidRect(rect, bcol);

	} else if ( (prop.attr & ATT_BORDER) != 0 ) {
		// Line Draw
		DrawLine(pDC, rect, fcol, bcol, bEraBack, prop, pView);

	} else {
		// Text Draw
		DrawChar(pDC, rect, fcol, bcol, bEraBack, prop, pView);
	}

	if ( ATT_EXTYPE(prop.attr) == ATT_OVERCHAR )
		DrawOverChar(pDC, rect, fcol, bcol, prop, pView);

	if ( (prop.attr & (ATT_OVER | ATT_DOVER | ATT_LINE | ATT_UNDER | ATT_DUNDER | ATT_SUNDER | ATT_STRESS)) != 0 )
		DrawHoriLine(pDC, rect, fcol, bcol, prop, pView);

	if ( (prop.attr & (ATT_FRAME | ATT_CIRCLE | ATT_RSLINE | ATT_RDLINE | ATT_LSLINE | ATT_LDLINE)) != 0 )
		DrawVertLine(pDC, rect, fcol, bcol, prop, pView);

	// リバースチェック
	bRevs  = (IsOptEnable(TO_DECSCNM) ? 1 : 0);
	bRevs ^= (pView->m_VisualBellFlag ? 1 : 0);

	if ( bRevs )
		pDC->InvertRect(rect);
}
void CTextRam::DrawVram(CDC *pDC, int x1, int y1, int x2, int y2, class CRLoginView *pView, BOOL bPrint)
{
	int n;
	int x, y, sx, ex;
	int zoom, len;
	CCurPos cpos, spos, epos;
	int csx, cex, csy, cey;
	int isx, iex;
	CRect rect;
	CCharCell *top, *vp;
	struct DrawWork prop, work;
	CStringW str;
	CArray<WCHAR, const WCHAR &>text;
	CArray<INT, const INT &> space;
	CFont *pFontOld = pDC->SelectObject(CFont::FromHandle((HFONT)GetStockObject(SYSTEM_FONT)));

	text.SetSize(0, MAXCHARSIZE);
	space.SetSize(0, MAXCHARSIZE);

	ZeroMemory(&prop, sizeof(prop));
	prop.aclock = clock();
	prop.print  = bPrint;

	if ( pView->m_ClipFlag > 1 ) {
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
		prop.idx = (-1);
		prop.stx = 0;
		prop.edx = 0;
		prop.sty = 0;

		prop.bank = (-1);
		prop.font = 0;
		prop.csz  = 0;		// XXXXX

		prop.eatt = 0;
		prop.frgb = prop.brgb = 0;

		rect.top    = pView->CalcGrapY(y);
		rect.bottom = pView->CalcGrapY(y + 1);

		n = y - pView->m_HisOfs + pView->m_HisMin;
		if ( (m_HisLen + n) >= m_Lines && n < m_Lines ) {
			top = GETVRAM(0, n);
			prop.zoom = top->m_Vram.zoom;
		} else {
			top = NULL;
			prop.zoom = 0;
		}

		zoom = (prop.zoom != 0 ? 2 : 1);
		len = 0;
		isx = iex = 0;

		memcpy(&work, &prop, sizeof(work));

		sx = x1;
		ex = x2;

		if ( work.zoom != 0 ) {
			sx /= 2;
			ex /= 2;
		}

		if ( sx > 0 && top != NULL && (top[sx].m_Vram.attr & ATT_RTOL) != 0 ) {
			while ( sx > 0 && (top[sx - 1].m_Vram.attr & ATT_RTOL) != 0 )
				sx--;
		}
		if ( ex < m_Cols && top != NULL && (top[ex].m_Vram.attr & ATT_RTOL) != 0 ) {
			while ( (ex + 1) < m_Cols && (top[ex + 1].m_Vram.attr & ATT_RTOL) != 0 )
				ex++;
		}

		for ( x = sx ; x < ex ; x += work.csz ) {
			work.idx = (-1);
			work.stx = 0;
			work.edx = 0;
			work.sty = 0;

			work.bank = (-1);
			work.csz  = 1;
			str.Empty();
			vp = NULL;

			work.eatt = 0;
			work.frgb = work.brgb = 0;

			if ( top == NULL ) {
				work.attr = m_DefAtt.std.attr & ~ATT_EXTVRAM;
				work.font = m_DefAtt.std.font;
				work.fcol = m_DefAtt.std.fcol;
				work.bcol = m_DefAtt.std.bcol;

			} else if ( x < 0 ) {
				work.attr = top->m_Vram.attr & (ATT_REVS | ATT_SMARK | ATT_SBLINK | ATT_BLINK);
				work.font = top->m_Vram.font;
				work.fcol = top->m_Vram.fcol;
				work.bcol = top->m_Vram.bcol;

			} else if ( x >= m_Cols ) {
				vp = top + (m_Cols - 1);
				work.attr = vp->m_Vram.attr & (ATT_REVS | ATT_SMARK | ATT_SBLINK | ATT_BLINK);
				work.font = vp->m_Vram.font;
				work.fcol = vp->m_Vram.fcol;
				work.bcol = vp->m_Vram.bcol;

			} else {
				vp = top + x;
				if ( x > 0 && IS_2BYTE(vp[0].m_Vram.mode) && IS_1BYTE(vp[-1].m_Vram.mode) && vp[-1].Compare(vp[0]) == 0 ) {
					x--;
					vp--;
				}

				work.attr = vp->m_Vram.attr & ATT_MASKEXT;			// ATT_RETURN mask
				work.font = vp->m_Vram.font;
				work.fcol = vp->m_Vram.fcol;
				work.bcol = vp->m_Vram.bcol;

				if ( IS_IMAGE(vp->m_Vram.mode) ) {
					work.idx = vp->m_Vram.pack.image.id;
					work.stx = isx = vp->m_Vram.pack.image.ix;
					work.edx = iex = vp->m_Vram.pack.image.ix + 1;
					work.sty = vp->m_Vram.pack.image.iy;
					if ( prop.idx == work.idx && prop.sty == work.sty && prop.edx == work.stx ) {
						work.stx = prop.stx;
						work.edx = prop.edx;
					}
					pView->SetCellSize(x, y, 0);
				} else if ( x < (m_Cols - 1) && IS_1BYTE(vp[0].m_Vram.mode) && IS_2BYTE(vp[1].m_Vram.mode) && vp[0].Compare(vp[1]) == 0 ) {
					work.csz = 2;
					str = (LPCWSTR)*vp;
					if ( !str.IsEmpty() )
						work.bank = vp->m_Vram.bank & CODE_MASK;
					pView->SetCellSize(x, y, 0);
					pView->SetCellSize(x + 1, y, 0);
				} else if ( IS_ASCII(vp->m_Vram.mode) ) {
					str = (LPCWSTR)*vp;
					work.bank = vp->m_Vram.bank & CODE_MASK;
					if ( !str.IsEmpty() && str[0] > _T(' ') && str[1] == _T('\0') ) {
						if ( (work.bank & SET_MASK) <= SET_96 && m_FontTab[work.bank].m_Shift == 0 ) {
							switch(str[0]) {
							case 0x0023: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[0], str); break;
							case 0x0024: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[1], str); break;
							case 0x0040: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[2], str); break;
							case 0x005B: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[3], str); break;
							case 0x005C: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[4], str); break;
							case 0x005D: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[5], str); break;
							case 0x005E: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[6], str); break;
							case 0x0060: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[7], str); break;
							case 0x007B: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[8], str); break;
							case 0x007C: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[9], str); break;
							case 0x007D: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[10], str); break;
							case 0x007E: UCS4ToWStr(m_FontTab[work.bank].m_Iso646Tab[11], str); break;
							}
							if ( str[0] == 0xDBBF && str[1] == 0xDFFF ) {	// U+FFFFF BackSlash Mirror !!!
								str = _T('/');
								work.attr |= ATT_MIRROR;
							}
						}
					} else if ( str.IsEmpty() || str[0] < ' ' || m_FontTab[work.bank].m_UserFontMap.GetSafeHandle() == NULL )
						work.bank = (-1);
					pView->SetCellSize(x, y, 0);
				}

				if ( work.bank != (-1) && m_FontTab[work.bank].IsOverChar(str) )
					work.attr |= ATT_OVERCHAR;
			}

			// Matrox View
			if ( pView->m_SleepView ) {
				if ( work.bank == (-1) ) {
					work.attr &= ~ATT_MASK;
					work.bcol = EXTCOL_MAX + 25;		// back
					if ( work.idx != (-1) ) {
						if ( (n = pView->m_MatrixCols[x] - y) < 0 || n > 24)
							work.idx = (-1);
						else if ( n == 0 )
							work.attr |= ATT_REVS;
					}
				} else {
					work.attr &= ~ATT_MASK;
					work.bcol = EXTCOL_MAX + 25;		// back
					if ( (n = pView->m_MatrixCols[x] - y) < 0 || n > 24 )
						work.fcol = EXTCOL_MAX + 25;	// back
					else
						work.fcol = EXTCOL_MAX + 24 - n;
				}

			} else if ( vp != NULL && (work.attr & ATT_EXTVRAM) != 0 ) {
				work.eatt = vp->GetEatt();
				if ( (work.eatt & EATT_BRGBCOL) != 0 )
					work.brgb = vp->GetBrgb();
				if ( (work.eatt & EATT_FRGBCOL) != 0 )
					work.frgb = vp->GetFrgb();
			}

			// Frame View
#ifdef	USE_TEXTFRAME
			if ( (work.attr & ATT_FRAME) != 0 ) {
				n = (ATT_LSLINE | ATT_RSLINE | ATT_OVER | ATT_SUNDER);
				if ( vp != NULL && x > 0 && (vp[-1].m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_LSLINE;
				if ( vp != NULL && x < (m_Cols - work.csz) && (vp[work.csz].m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_RSLINE;
				if ( y > 0 && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin - 1)->m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_OVER;
				if ( y < (m_Lines - 1) && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin + 1)->m_Vram.attr & ATT_FRAME) != 0 )
					n &= ~ATT_SUNDER;
				work.attr &= ~(ATT_FRAME | ATT_CIRCLE);
				work.attr |= n;
				m_FrameCheck = TRUE;

			} else if ( (work.attr & ATT_CIRCLE) != 0 ) {
				n = (ATT_LDLINE | ATT_RDLINE | ATT_DOVER | ATT_DUNDER);
				if ( vp != NULL && x > 0 && (vp[-1].m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_LDLINE;
				if ( vp != NULL && x < (m_Cols - work.csz) && (vp[work.csz].m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_RDLINE;
				if ( y > 0 && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin - 1)->m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_DOVER;
				if ( y < (m_Lines - 1) && (GETVRAM(x, y - pView->m_HisOfs + pView->m_HisMin + 1)->m_Vram.attr & ATT_CIRCLE) != 0 )
					n &= ~ATT_DUNDER;
				work.attr &= ~(ATT_FRAME | ATT_CIRCLE);
				work.attr |= n;
				m_FrameCheck = TRUE;
			}
#endif

			// Clip View
			if ( pView->m_ClipFlag > 1 && x >= 0 && x < m_Cols && top != NULL ) {
				cpos = GetCalcPos((work.zoom != 0 ? (x * 2) : x), y - pView->m_HisOfs + pView->m_HisMin);
				if ( spos <= cpos && cpos <= epos ) {
					if ( pView->IsClipRectMode() ) {
						if ( work.zoom != 0 ) {
							if ( (x * 2) >= csx && (x * 2) <= cex )
								work.attr |= ATT_CLIP;
						} else if ( x >= csx && x <= cex )
							work.attr |= ATT_CLIP;
					} else
						work.attr |= ATT_CLIP;
				}
			}

			// Speak Text
			if ( pView->m_bSpeakDispText && x >= 0 && x < m_Cols && top != NULL ) {
				cpos = GetCalcPos(x, y - pView->m_HisOfs + pView->m_HisMin);
				if ( pView->m_SpeakStaPos <= cpos && cpos <= pView->m_SpeakEndPos )
					work.attr ^= ATT_UNDER;
			}

			// Text Draw
			if ( memcmp(&prop, &work, sizeof(work)) != 0 ) {
				if ( len > 0 ) {
					rect.right = pView->CalcGrapX(x * zoom);
					prop.size   = len;
					prop.tlen   = (int)text.GetSize();
					prop.pText  = text.GetData();
					prop.pSpace = space.GetData();
					DrawString(pDC, rect, prop, pView);
				}
				work.cols = x;
				work.line = y;
				memcpy(&prop, &work, sizeof(prop));
				text.RemoveAll();
				space.RemoveAll();
				rect.left = pView->CalcGrapX(x * zoom);
				len = 0;
				prop.stx = isx;
				prop.edx = iex;
			} else
				prop.edx = iex;

			if ( work.bank != (-1) ) {
				for ( LPCWSTR p = str ; *p != L'\0' ; )
					text.Add(*(p++));
			} else
				text.Add(L'\0');

			while ( space.GetSize() < (text.GetSize() - 1) )
				space.Add(0);

			space.Add(pView->CalcGrapX((x + work.csz) * zoom) - pView->CalcGrapX(x * zoom));

			len++;
		}

		if ( len > 0 ) {
			rect.right = pView->CalcGrapX(x * zoom);
			prop.size   = len;
			prop.tlen   = (int)text.GetSize();
			prop.pText  = text.GetData();
			prop.pSpace = space.GetData();
			DrawString(pDC, rect, prop, pView);
		}
	}

	pDC->SelectObject(pFontOld);
}

CWnd *CTextRam::GetAciveView()
{
	if ( m_pDocument == NULL )
		return NULL;

	return m_pDocument->GetAciveView();
}
void CTextRam::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	CWnd *pView;
	
	if ( (pView = GetAciveView()) != NULL )
		pView->PostMessage(message, wParam, lParam);
}
void CTextRam::GetCellSize(int *x, int *y)
{
	CRLoginView *pView;

	if ( (pView = (CRLoginView *)GetAciveView()) != NULL ) {
		*x = pView->m_CharWidth;
		*y = pView->m_CharHeight;
	} else {
		*x = 8;
		*y = *x * m_DefFontHw / 10;
	}
}
void CTextRam::GetScreenSize(int *pCx, int *pCy, int *pSx, int *pSy)
{
	*pCx = m_Cols;
	*pCy = GetLineSize();

	GetCellSize(pSx, pSy);

	*pSx = *pSx * *pCx;
	*pSy = *pSy * *pCy;
}

int CTextRam::OptionToIndex(int value, BOOL bAnsi)
{
	if ( value > 0 && value <= 199 )
		value += 0;					// DEC Terminal Option	0-199 -> 0-199
	else if ( bAnsi && value >= 200 && value <= 299 )
		value += 0;					// ANSI Screen Option	200-299 -> 200-299(ANSI 0-199を割り当て)
	else if ( value >= 1000 && value <= 1079 )
		value -= 700;				// XTerm Option			1000-1079 -> 300-379
	else if ( value >= 2000 && value <= 2019 )
		value -= 1620;				// XTerm Option 2		2000-2019 -> 380-399
	else if ( value >= 8400 && value <= 8512 )
		value -= 8000;				// RLogin Option		8400-8511 -> 400-511

	else if ( value == 7727 )
		value = TO_RLCKMESC;		// 7727 - Application Escape mode を有効にする。				Application Escape mode を無効にする。  
	else if ( value == 7786 )
		value = TO_RLMSWAPE;		// 7786 - マウスホイール - カーソルキー変換を有効にする。		マウスホイール - カーソルキー変換を無効にする。 
	else if ( value == 8200 )
		value = TO_TTCTH;			// 8200 - 画面クリア(ED 2)時にカーソルを左上に移動する。		移動しない
	else if ( value == 8800 )
		value = TO_DRCSMMv1;		// 8800 - Unicodeマッピング有効									Unicodeマッピング無効 
	else if ( value == 8840 )
		value = TO_RLUNIAWH;		// 8840 - TNAMB Aタイプをダブル幅の文字にする					シングル幅にする
	else
		value = (-1);

	return value;
}
int CTextRam::IndexToOption(int value)
{
	if ( value >= 400 )				// RLogin Option		400-511 -> 8400-8511
		value += (8400 - 400);
	else if ( value >= 380 )		// XTerm Option 2		380-399 -> 2000-2019
		value += (2000 - 380);
	else if ( value >= 300 )		// XTerm Option			300-379 -> 1000-1079
		value += (1000 - 300);
	else if ( value >= 200 )		// ANSI Screen Option	200-299 -> 200-299(ANSI 0-199を割り当て)
		value += (200 - 200);
									// DEC Terminal Option	0-199 -> 0-199
	return value;
}
void CTextRam::OptionString(int value, CString &str)
{
	if ( value >= 400 )				// RLogin Option		400-511 -> ?8400-8511
		str.Format(_T("?%d"), value + (8400 - 400));
	else if ( value >= 380 )		// XTerm Option 2		380-399 -> ?2000-2019
		str.Format(_T("?%d"), value + (2000 - 380));
	else if ( value >= 300 )		// XTerm Option			300-379 -> ?1000-1079
		str.Format(_T("?%d"), value	+ (1000 - 300));
	else if ( value >= 200 )		// ANSI Screen Option	200-299 -> 0-99
		str.Format(_T("%d"), value + (0 - 200));
	else							// DEC Terminal Option	0-199 -> ?0-199
		str.Format(_T("?%d"), value);
}
void CTextRam::IncDscs(int &Pcss, CString &str)
{
	// I I F        
	// Generic Dscs.
	// A Dscs can consist of 0 to 2 intermediates (I) and a final (F).
	// Intermediates are in the range 2/0 to 2/15.
	// Finals are in the range 3/0 to 7/14. 

	LPCTSTR p = str;
	TCHAR scs[3];

	scs[0] = scs[1] = scs[2] = 0;

	while ( *p != _T('\0') ) {
		if ( *p >= _T('\x20') && *p <= _T('\x2F') ) {
			if ( scs[0] == 0 )
				scs[0] = *(p++);
			else if ( scs[1] == 0 )
				scs[1] = *(p++);
			else {
				// shift
				scs[0] = scs[1];
				scs[1] = *(p++);
			}
		} else if ( *p >= _T('\x30') && *p <= _T('\x7E') ) {
			scs[2] = *(p++);
			break;
		} else
			p++;
	}

	scs[2] += 1;
	if ( scs[2] < _T('\x30') )
		scs[2] = _T('\x30');
	else if ( scs[2] > _T('\x7E') ) {
		scs[2] = _T('\x30');

#if 1
		// IIF のFのみインクリメント
		// オーバー時は、文字セットを変更
		Pcss = (Pcss == 0 ? 1 : 0);
#else
		scs[1] += 1;
		if ( scs[1] < _T('\x20') )
			scs[1] = _T('\x20');
		else if ( scs[1] > _T('\x2F') ) {
			scs[1] = _T('\x20');

			scs[0] += 1;
			if ( scs[0] < _T('\x20') )
				scs[0] = _T('\x20');
			else if ( scs[0] > _T('\x2F') ) {
				scs[0] = _T('\x20');
				// Over Flow !!!
			}
		}
#endif
	}

	str.Empty();
	if ( scs[0] != 0 )
		str += scs[0];
	if ( scs[1] != 0 )
		str += scs[1];
	if ( scs[2] != 0 )
		str += scs[2];
}

#define	CMDMAPBITSIZE	(8 * sizeof(DWORD))

static BOOL InitCmdMap = FALSE;
static DWORD CmdMap[128 / CMDMAPBITSIZE];

typedef struct _STRSTACK {
	struct _STRSTACK	*next;
	LPCTSTR				str;
} STRSTACK;

static void DateTimeFormatCheck(LPCTSTR fmt, CString &tmp, struct _timeb &tb, STRSTACK *top)
{
	int n;
	CString env, wrk;
	TCHAR cmd;
	BOOL bExt;
	STRSTACK stack, *sp;

	while ( *fmt != _T('\0') ) {
		if ( fmt[0] == _T('%') && fmt[1] == _T('{') && _tcschr(fmt, _T('}')) != NULL ) {
			env.Empty();
			wrk.Empty();
			for ( fmt += 2 ; *fmt != _T('\0') ; ) {
				if ( *fmt == _T('}') ) {
					fmt++;
					break;
				}
				env += *(fmt++);
			}
			for ( sp = top ; sp != NULL ; sp = sp->next ) {
				if ( _tcscmp(env, sp->str) == 0 )
					break;
			}
			if ( sp == NULL ) {
				CRLoginDoc::EnvironText(env, wrk);
				stack.next = top;
				stack.str  = env;
				DateTimeFormatCheck(wrk, tmp, tb, &stack);
			}

		} else  if ( *fmt == _T('%') ) {
			if ( (cmd = fmt[1]) == _T('#') ) {
				bExt = TRUE;
				cmd = fmt[2];
			} else
				bExt = FALSE;

			if ( cmd == _T('\0') )
				break;
			else if ( cmd < 128 && (CmdMap[cmd / CMDMAPBITSIZE] & (1 << (cmd % CMDMAPBITSIZE))) != 0 ) {
				tmp += (bExt ? _T("%#") : _T("%"));
				tmp += cmd;
			} else if ( cmd == _T('L') ) {
				if ( bExt ) {
					if ( tb.millitm >= 100 ) {
						tmp += (TCHAR)(_T('0') + (tb.millitm / 100) % 10);
						tmp += (TCHAR)(_T('0') + (tb.millitm / 10)  % 10);
					} else if ( tb.millitm >= 10 )
						tmp += (TCHAR)(_T('0') + (tb.millitm / 10)  % 10);
				} else {
					tmp += (TCHAR)(_T('0') + (tb.millitm / 100) % 10);
					tmp += (TCHAR)(_T('0') + (tb.millitm / 10)  % 10);
				}
				tmp += (TCHAR)(_T('0') +  tb.millitm % 10);
			}

			fmt += (bExt ? 3 : 2);

		} else if ( *fmt == _T('\\') ) {
			switch(fmt[1]) {
			case _T('a'): tmp += _T('\x07'); fmt += 2; break;
			case _T('b'): tmp += _T('\x08'); fmt += 2; break;
			case _T('t'): tmp += _T('\x09'); fmt += 2; break;
			case _T('n'): tmp += _T('\x0A'); fmt += 2; break;
			case _T('v'): tmp += _T('\x0B'); fmt += 2; break;
			case _T('f'): tmp += _T('\x0C'); fmt += 2; break;
			case _T('r'): tmp += _T('\x0D'); fmt += 2; break;
			case _T('\\'): tmp += _T('\\');  fmt += 2; break;

			case _T('x'): case _T('X'):
				fmt += 2;
				for ( n = cmd = 0 ; n < 2 ; n++ ) {
					if ( *fmt >= _T('0') && *fmt <= _T('9') )
						cmd = cmd * 16 + (*(fmt++) - _T('0'));
					else if ( *fmt >= _T('A') && *fmt <= _T('F') )
						cmd = cmd * 16 + (*(fmt++) - _T('A') + 10);
					else if ( *fmt >= _T('a') && *fmt <= _T('f') )
						cmd = cmd * 16 + (*(fmt++) - _T('a') + 10);
					else
						break;
				}
				if ( cmd == _T('%') )
					tmp += _T('%');
				tmp += cmd;
				break;

			case _T('0'): case _T('1'): case _T('2'): case _T('3'):
			case _T('4'): case _T('5'): case _T('6'): case _T('7'):
				fmt += 1;
				for ( n = cmd = 0 ; n < 3 ; n++ ) {
					if ( *fmt >= _T('0') && *fmt <= _T('7') )
						cmd = cmd * 8 + (*(fmt++) - _T('0'));
					else
						break;
				}
				if ( cmd == _T('%') )
					tmp += _T('%');
				tmp += cmd;
				break;

			default:
				fmt += 1;
				break;
			}

		} else
			tmp += *(fmt++);
	}
}

void CTextRam::GetCurrentTimeFormat(LPCTSTR fmt, CString &str)
{
	struct _timeb tb;
	CTime now;
	CString tmp;

	/*
		VS2010	_T("aAbBcdHIjmMpSUwWxXyYzZ%")
				_T("aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%")
	*/

	if ( !InitCmdMap ) {
		ZeroMemory(CmdMap, sizeof(CmdMap));
		for ( LPCTSTR p = _T("aAbBcdHIjmMpSUwWxXyYzZ%") ; *p != _T('\0') ; p++ )
			CmdMap[*p / CMDMAPBITSIZE] |= (1 << (*p % CMDMAPBITSIZE));
		InitCmdMap = TRUE;
	}

	_ftime(&tb);
	now = tb.time;

	DateTimeFormatCheck(fmt, tmp, tb, NULL);

	if ( tmp.GetLength() > 120 )
		str = _T("Date/Time Format to long... ");
	else
		str = now.Format(tmp);
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
int CTextRam::InitDefParam(BOOL bCheck, int modFlag)
{
	CDefParamDlg dlg;

	if ( bCheck ) {
		dlg.m_InitFlag = modFlag;
		dlg.m_pTextRam = this;
		if ( dlg.DoModal() != IDOK )
			return (modFlag & ~(UMOD_ANSIOPT | UMOD_MODKEY | UMOD_COLTAB | UMOD_BANKTAB | UMOD_DEFATT | UMOD_CARET));
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

	if ( (modFlag & UMOD_CARET) != 0 ) {
		m_DefTypeCaret  = m_TypeCaret;
		m_DefCaretColor = m_CaretColor;
	}
	
	return modFlag;
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
	if ( !IsInitText() )
		return;

	SaveHistory();

	if ( m_pDocument != NULL )
		m_pDocument->LogClose();
}
void CTextRam::OnTimer(int id)
{
	m_IntCounter++;

	if ( m_bOscActive ) {
		if ( m_pDocument != NULL && m_IntCounter == 10 ) {
			m_SeqMsg.Format(CStringLoad(IDM_SEQCANCELSTR), m_OscName);
			m_pDocument->UpdateAllViews(NULL, UPDATE_CANCELBTN, (CObject *)(LPCTSTR)m_SeqMsg);

			CString str;
			str.Format(CStringLoad(IDS_OSCBUFFERINGMSG), m_OscName);
			((CMainFrame *)AfxGetMainWnd())->SetStatusText(str);
		}
	} else if ( m_IntCounter > 180 ) {
		m_bIntTimer = FALSE;
		((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);
	}
}

void CTextRam::CallReceiveLine(int y)
{
	if ( m_pDocument == NULL || m_pDocument->m_pLogFile == NULL )
		return;

	CStringW tmp, str;
	CBuffer in, out;

	GetLine(y, tmp);

	if ( IsOptEnable(TO_RLLOGTIME) ) {
		GetCurrentTimeFormat(m_TimeFormat, str);
		in.Apend((LPBYTE)((LPCWSTR)str), str.GetLength() * sizeof(WCHAR));
	}

	in.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
	m_IConv.StrToRemote(m_SendCharSet[m_LogCharSet[IsOptValue(TO_RLLOGCODE, 2)]], &in, &out);
	m_pDocument->m_pLogFile->Write(out.GetPtr(), out.GetSize());
}
BOOL CTextRam::CallReceiveChar(DWORD ch, LPCTSTR name)
{
	// UTF-16LE x 2 or BS(08) HT(09) LF(0A) VT(0B) FF(0C) CR(0D) ESC(1B)
	//
	// ch = 0x0000 0000
	//		  UCS2 UCS2

	if ( (ch & 0xFFFF0000) == 0 && ch < ' ' ) {
		m_LastFlag = 0;
		m_bRtoL = FALSE;
		m_bJoint = FALSE;
	}

	if ( m_pDocument == NULL || m_bTraceActive )
		return FALSE;

	if ( m_StsMode == (STSMODE_ENABLE | STSMODE_INDICATOR) ) {
		if ( (ch & 0xFFFF0000) != 0 )
			m_StsText += (WCHAR)(ch >> 16);
		else if ( ch == L'\r' || ch == L'\n' )
			m_StsText.Empty();
		else if ( ch >= L' ' )
			m_StsText += (WCHAR)ch;
		((CMainFrame *)AfxGetMainWnd())->SetStatusText(UniToTstr(m_StsText));
		return TRUE;
	}

	if ( name == NULL && m_MediaCopyMode != MEDIACOPY_NONE )
		MediaCopyDchar(ch);

	int pos = m_CurY + m_HisPos;
	
	while ( pos < 0 )
		pos += m_HisMax;
	while ( pos >= m_HisMax )
		pos -= m_HisMax;

	m_pDocument->OnReceiveChar(ch, m_CurX + pos * m_ColsMax);

	if ( m_pDocument->m_pLogFile == NULL )
		return (m_MediaCopyMode == MEDIACOPY_RELAY ? TRUE : FALSE);

	if ( m_LogMode == LOGMOD_LINE ) {
		if ( pos != m_LogCurY ) { // && ch >= 0x20 ) {
			if ( m_LogCurY != (-1) )
				CallReceiveLine(m_LogCurY - m_HisPos);
			m_LogCurY = pos;
		}

	} else if ( m_LogMode == LOGMOD_CTRL || m_LogMode == LOGMOD_CHAR ) {
		CBuffer in, out;
		CStringW tmp, str;

		switch(ch) {
		case 0x08:
			tmp = L"<BS>";
			break;
		case 0x0B:
			tmp = L"<VT>";
			break;
		case 0x0C:
			tmp = L"<FF>";
			break;
		case 0x1B:
			ASSERT(name != NULL);
			tmp.Format(L"<%s>", TstrToUni(name));
			break;
		case 0x09:	// TAB
		case 0x0A:	// LF
		case 0x0D:	// CR
			tmp = (WCHAR)ch;
			break;
		default:
			if ( (ch & 0xFFFF0000) != 0 )
				tmp = (WCHAR)(ch >> 16);
			tmp += (WCHAR)(ch & 0xFFFF);
			break;
		}

		if ( m_LogTimeFlag && IsOptEnable(TO_RLLOGTIME) ) {
			GetCurrentTimeFormat(m_TimeFormat, str);
			in.Apend((LPBYTE)((LPCWSTR)str), str.GetLength() * sizeof(WCHAR));
			m_LogTimeFlag = FALSE;
		}

		if ( ch == 0x0A )
			m_LogTimeFlag = TRUE;

		in.Apend((LPBYTE)((LPCWSTR)tmp), tmp.GetLength() * sizeof(WCHAR));
		m_IConv.StrToRemote(m_SendCharSet[m_LogCharSet[IsOptValue(TO_RLLOGCODE, 2)]], &in, &out);
		m_pDocument->m_pLogFile->Write(out.GetPtr(), out.GetSize());
	}

	return (m_MediaCopyMode == MEDIACOPY_RELAY ? TRUE : FALSE);
}

int CTextRam::UnicodeCharFlag(DWORD code)
{
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

#define	JPSET_X0208		0
#define	JPSET_X0213		1
#define	JPSET_SJIS		2
#define	JPSET_SJISX		3

typedef struct _JpSetList {
	LPCTSTR name;
	int     type;
} JpSetList;

static int JpsetNameComp(const void *src, const void *dis)
{
	return _tcscmp((LPCTSTR)src, ((JpSetList *)dis)->name);
}
int CTextRam::JapanCharSet(LPCTSTR name)
{
	static const JpSetList jpsettab[30] = {
		{ _T("CSEUCPKDFMTJAPANESE"),							JPSET_X0208 },	// EUC-JP
		{ _T("CSISO2022JP"),								    JPSET_X0208 },	// ISO-2022-JP
		{ _T("CSISO2022JP2"),								    JPSET_X0208 },	// ISO-2022-JP-2
		{ _T("CSISO87JISX0208"),								JPSET_X0208 },	// JIS_X0208
		{ _T("CSSHIFTJIS"),									    JPSET_SJIS },	// SHIFT-JIS
		{ _T("EUC-JIS-2004"),									JPSET_X0213 },	// JISX0213
		{ _T("EUC-JISX0213"),									JPSET_X0213 },	// JISX0213
		{ _T("EUC-JP"),										    JPSET_X0208 },	// EUC-JP
		{ _T("EUCJP"),										    JPSET_X0208 },	// EUC-JP
		{ _T("EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE"),	JPSET_X0208 },	// EUC-JP

		{ _T("ISO-2022-JP"),								    JPSET_X0208 },	// ISO-2022-JP
		{ _T("ISO-2022-JP-1"),								    JPSET_X0208 },	// ISO-2022-JP-1
		{ _T("ISO-2022-JP-2"),								    JPSET_X0208 },	// ISO-2022-JP-2
		{ _T("ISO-2022-JP-2004"),								JPSET_X0213 },	// ISO-2022-JP-2004
		{ _T("ISO-2022-JP-3"),									JPSET_X0213 },	// ISO-2022-JP-2004
		{ _T("ISO-IR-87"),									    JPSET_X0208 },	// JIS_X0208
		{ _T("JIS0208"),									    JPSET_X0208 },	// JIS_X0208
		{ _T("JIS_C6226-1983"),									JPSET_X0208 },	// JIS_X0208
		{ _T("JIS_X0208"),									    JPSET_X0208 },	// JIS_X0208
		{ _T("JIS_X0208-1983"),									JPSET_X0208 },	// JIS_X0208

		{ _T("JIS_X0208-1990"),									JPSET_X0208 },	// JIS_X0208
		{ _T("JIS_X0213-2000.1"),								JPSET_X0213 },	// JISX0213
		{ _T("JIS_X0213-2000.2"),								JPSET_X0213 },	// JISX0213
		{ _T("MS_KANJI"),									    JPSET_SJIS },	// SHIFT-JIS
		{ _T("SHIFT-JIS"),									    JPSET_SJIS },	// SHIFT-JIS
		{ _T("SHIFT_JIS"),									    JPSET_SJIS },	// SHIFT-JIS
		{ _T("SHIFT_JIS-2004"),									JPSET_SJISX },	// JISX0213
		{ _T("SHIFT_JISX0213"),									JPSET_SJISX },	// JISX0213
		{ _T("SJIS"),										    JPSET_SJIS },	// SHIFT-JIS
		{ _T("X0208"),										    JPSET_X0208 },	// JIS_X0208
	};
	int n;

	if ( BinaryFind((void *)name, (void *)jpsettab, sizeof(JpSetList), 30, JpsetNameComp, &n) )
		return jpsettab[n].type;

	return (-1);
}
/***********************************

	libiconv-1.16

	SJIS	EUC		SHIFT_JIS	S-JISX0213	EUC-JP		E-JISX0213	JIS_X0208	JISX0213	CP932
	005c	005c	00a5		00a5														005c
	007e	007e	203e		203e														007e
	815c	a1bd				2014					2014					2014		2015
	8160	a1c1	301c		301c		301c		301c		301c		301c		ff5e
	8161	a1c2	2016		2016		2016		2016		2016		2016		2225
	817c	a1dd	2212		2212		2212		2012		2212		2212		ff0d
	8191	a1f1	00a2		00a2		00a2		00a2		00a2		00a2		ffe0
	8192	a1f2	00a3		00a3		00a3		00a3		00a3		00a3		ffe1
	81ca	a2cc	00ac		00ac		00ac		00ac		00ac		00ac		ffe2

***************************/

DWORD CTextRam::MsToIconvUnicode(int jpset, DWORD code)
{
	switch(jpset) {
	case JPSET_X0208:
		switch(code) {							/*                  iconv  MS     */
		case 0x2225: code = 0x2016; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
		case 0xFF0D: code = 0x2212; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
		case 0xFF5E: code = 0x301C; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
		case 0xFFE0: code = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
		case 0xFFE1: code = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
		case 0xFFE2: code = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
		}
		break;
	case JPSET_X0213:
		switch(code) {							/*                  iconv  MS     */
		case 0x2015: code = 0x2014; break;		/* ― 0x815C(01-29) U+2014 U+2015 */
		case 0x2225: code = 0x2016; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
		case 0xFF0D: code = 0x2212; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
		case 0xFF5E: code = 0x301C; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
		case 0xFFE0: code = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
		case 0xFFE1: code = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
		case 0xFFE2: code = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
		}
		break;
	case JPSET_SJIS:
		switch(code) {							/*                  iconv  MS     */
		case 0x005C: code = 0x00A5; break;		/* \  0x5C          U+00A5 U+005C */
		case 0x007E: code = 0x203E; break;		/* ~  0x7E          U+203E U+007E */
		case 0x2225: code = 0x2016; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
		case 0xFF0D: code = 0x2212; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
		case 0xFF5E: code = 0x301C; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
		case 0xFFE0: code = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
		case 0xFFE1: code = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
		case 0xFFE2: code = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
		}
		break;
	case JPSET_SJISX:
		switch(code) {							/*                  iconv  MS     */
		case 0x005C: code = 0x00A5; break;		/* \  0x5C          U+00A5 U+005C */
		case 0x007E: code = 0x203E; break;		/* ~  0x7E          U+203E U+007E */
		case 0x2015: code = 0x2014; break;		/* ― 0x815C(01-29) U+2014 U+2015 */
		case 0x2225: code = 0x2016; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
		case 0xFF0D: code = 0x2212; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
		case 0xFF5E: code = 0x301C; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
		case 0xFFE0: code = 0x00A2; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
		case 0xFFE1: code = 0x00A3; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
		case 0xFFE2: code = 0x00AC; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
		}
		break;
	}

	return code;
}
DWORD CTextRam::IconvToMsUnicode(int jpset, DWORD code)
{
	switch(jpset) {
	case JPSET_X0208:
		switch(code) {							/*                  iconv  MS     */
		case 0x00A2: code = 0xFFE0; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
		case 0x00A3: code = 0xFFE1; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
		case 0x00AC: code = 0xFFE2; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
		case 0x2016: code = 0x2225; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
		case 0x2212: code = 0xFF0D; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
		case 0x301C: code = 0xFF5E; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
		}
		break;
	case JPSET_X0213:
		switch(code) {							/*                  iconv  MS     */
		case 0x00A2: code = 0xFFE0; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
		case 0x00A3: code = 0xFFE1; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
		case 0x00AC: code = 0xFFE2; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
		case 0x2014: code = 0x2015; break;		/* ― 0x815C(01-29) U+2014 U+2015 */
		case 0x2016: code = 0x2225; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
		case 0x2212: code = 0xFF0D; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
		case 0x301C: code = 0xFF5E; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
		}
		break;
	case JPSET_SJIS:
		switch(code) {							/*                  iconv  MS     */
		case 0x00A2: code = 0xFFE0; break;		/* ￠ 0x8191(01-81) U+00A2 U+FFE0 */
		case 0x00A3: code = 0xFFE1; break;		/* ￡ 0x8192(01-82) U+00A3 U+FFE1 */
		case 0x00A5: code = 0x005C; break;		/* \  0x5C          U+00A5 U+005C */
		case 0x00AC: code = 0xFFE2; break;		/* ￢ 0x81CA(02-44) U+00AC U+FFE2 */
		case 0x2016: code = 0x2225; break;		/* ∥ 0x8161(01-34) U+2016 U+2225 */
		case 0x203E: code = 0x007E; break;		/* ~  0x7E          U+203E U+007E */
		case 0x2212: code = 0xFF0D; break;		/* － 0x817C(01-61) U+2212 U+FF0D */
		case 0x301C: code = 0xFF5E; break;		/* ～ 0x8160(01-33) U+301C U+FF5E */
		}
		break;
	case JPSET_SJISX:
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
		break;
	}

	return code;
}
void CTextRam::MsToIconvUniStr(LPCTSTR charset, LPWSTR str, int len)
{
	int jpset = JapanCharSet(charset);

	for ( ; len-- > 0 ; str++ )
		*str = (WCHAR)MsToIconvUnicode(jpset, *str);
}
void CTextRam::IconvToMsUniStr(LPCTSTR charset, LPWSTR str, int len)
{
	int jpset = JapanCharSet(charset);

	for ( ; len-- > 0 ; str++ )
		*str = (WCHAR)IconvToMsUnicode(jpset, *str);
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
void CTextRam::UCS4ToWStr(DWORD code, CStringW &str)
{
	if ( code >= 0x00010000L ) {
		code -= 0x10000L;
		code = (((code & 0xFFC00L) << 6) | (code & 0x3FF)) | 0xD800DC00L;
		str = (WCHAR)(code >> 16);
		str += (WCHAR)code;
	} else
		str = (WCHAR)code;
}
DWORD CTextRam::UnicodeNomal(DWORD code1, DWORD code2)
{
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
void CTextRam::UnicodeNomalStr(LPCWSTR p, int len, CBuffer &out)
{
	DWORD d1 = 0, d2, d3;

	while ( len-- > 0 ) {
		//  1101 10xx	U+D800 - U+DBFF	上位サロゲート	1101 11xx	U+DC00 - U+DFFF	下位サロゲート
		if ( len > 0 && (p[0] & 0xFC00) == 0xD800 && (p[1] & 0xFC00) == 0xDC00 ) {
			d2 = (p[0] << 16) | p[1];
			p += 2;
			len--;
		} else
			d2 = *(p++);

		if ( d1 != 0 && !(d1 == 0xFFFE || d1 == 0xFEFF) ) {
			if ( (d3 = UnicodeNomal(d1, d2)) != 0 )
				d2 = d3;
			else {
				if ( (d1 & 0xFFFF0000L) != 0 )
					out.PutWord((WORD)(d1 >> 16));
				out.PutWord((WORD)d1);
			}
		}
		d1 = d2;
	}
	if ( d1 != 0 && !(d1 == 0xFFFE || d1 == 0xFEFF) ) {
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
void CTextRam::SizeGrapWnd(class CGrapWnd *pWnd, int cx, int cy, BOOL bAspect)
{
	int n, t;
	int dx, dy;
	int fw, fh;
	int wax, way;
	int hax, hay;

	GetCellSize(&fw, &fh);

	// disp image size
	if ( (dx = pWnd->m_MaxX * pWnd->m_AspX / ASP_DIV) <= 0 ) dx = 1;
	if ( (dy = pWnd->m_MaxY * pWnd->m_AspY / ASP_DIV) <= 0 ) dy = 1;

	// width zoom calc
	if ( cx <= 0 ) {	// auto width
		GetMargin(MARCHK_NONE);

		if ( (t = m_Margin.right - m_CurX) <= 0 )
			t = m_Lines - m_CurX;

		if ( (n = fw * t) < dx ) {
			// zoom out
			wax = pWnd->m_AspX * n / dx;
			way = pWnd->m_AspY * n / dx;
		} else {
			// no zoom
			wax = pWnd->m_AspX;
			way = pWnd->m_AspY;
		}
	} else {			// cell width
		// zoom in/out
		n = fw * cx;
		wax = pWnd->m_AspX * n / dx;
		way = pWnd->m_AspY * n / dx;
	}

	// height zoom calc
	if ( cy <= 0 ) {	// auto height
		// no zoom
		hax = pWnd->m_AspX;
		hay = pWnd->m_AspY;
	} else {			// cell height
		// zoom in/out
		n = fh * cy;
		hax = pWnd->m_AspX * n / dy;
		hay = pWnd->m_AspY * n / dy;
	}

	if ( bAspect ) {
		// Preserve Aspect Ratio
		if ( wax < hax || way < hay ) {
			pWnd->m_AspX = wax;
			pWnd->m_AspY = way;
		} else {
			pWnd->m_AspX = hax;
			pWnd->m_AspY = hay;
		}
	} else {
		pWnd->m_AspX = wax;
		pWnd->m_AspY = hay;
	}

	if ( pWnd->m_AspX <= 0 )
		pWnd->m_AspX = 1;
	if ( pWnd->m_AspY <= 0 )
		pWnd->m_AspY = 1;

	// disp image size recalc
	if ( (dx = pWnd->m_MaxX * pWnd->m_AspX / ASP_DIV) <= 0 ) dx = 1;
	if ( (dy = pWnd->m_MaxY * pWnd->m_AspY / ASP_DIV) <= 0 ) dy = 1;

	// auto cell size calc
	if ( cx <= 0 )		// auto
		cx = (dx + fw - 1) / fw;
	if ( cy <= 0 )		// auto
		cy = (dy + fh - 1) / fh;

	pWnd->m_BlockX = cx;
	pWnd->m_BlockY = cy;

	pWnd->m_CellX = fw;
	pWnd->m_CellY = fh;
}
void CTextRam::DispGrapWnd(class CGrapWnd *pGrapWnd, BOOL bNextCols)
{
	DISPVRAM(m_CurX, m_CurY, pGrapWnd->m_BlockX, pGrapWnd->m_BlockY);
	m_UpdateFlag = TRUE;	// 表示の機会を与える

	for ( int y = 0 ; y < pGrapWnd->m_BlockY ; y++ ) {
		CCharCell *vp = GETVRAM(m_CurX, m_CurY);
		for ( int x = 0 ; x < pGrapWnd->m_BlockX && (m_CurX + x) < m_Margin.right ; x++ ) {
			vp->m_Vram.pack.image.id = pGrapWnd->m_ImageIndex;
			vp->m_Vram.pack.image.ix = x;
			vp->m_Vram.pack.image.iy = y;
			vp->m_Vram.bank = SET_96 | 'A';
			vp->m_Vram.eram = m_AttNow.std.eram;
			vp->m_Vram.mode = CM_IMAGE;
			vp->m_Vram.attr = m_AttNow.std.attr;
			vp->m_Vram.font = m_AttNow.std.font;
			vp->m_Vram.fcol = m_AttNow.std.fcol;
			vp->m_Vram.bcol = m_AttNow.std.bcol;

			// 拡張frgbはpack.imageと重なるので注意！！
			if ( (m_AttNow.eatt & EATT_BRGBCOL) != 0 ) {
				vp->m_Vram.attr |= ATT_EXTVRAM;
				vp->SetEatt(EATT_BRGBCOL);
				vp->SetBrgb(m_AttNow.brgb);
			}

			vp++;
		}
		if ( bNextCols ) {
			if ( (y + 1) < pGrapWnd->m_BlockY )
				ONEINDEX();
		} else
			ONEINDEX();
	}

	if ( bNextCols ) {
		if ( (m_CurX += pGrapWnd->m_BlockX) >= m_Margin.right ) {
			if ( IsOptEnable(TO_DECAWM) ) {
				if ( IsOptEnable(TO_RLGNDW) ) {
					LOCATE(m_Margin.left, m_CurY);
					ONEINDEX();
				} else {
					LOCATE(m_Margin.right - 1, m_CurY);
					m_DoWarp = TRUE;
				}
			} else 
				LOCATE(m_Margin.right - 1, m_CurY);
		} else
			LOCATE(m_CurX, m_CurY);
	}
}

//////////////////////////////////////////////////////////////////////
// Low Level
//////////////////////////////////////////////////////////////////////

void CTextRam::RESET(int mode)
{
	BOOL bSendSize = FALSE;

	if ( mode & RESET_SIZE && m_pDocument != NULL ) {
		m_bReSize = FALSE;
		m_pDocument->UpdateAllViews(NULL, UPDATE_RESIZE, NULL);
	}

	if ( mode & RESET_PAGE ) {
		if ( IsInitText() ) {
			SETPAGE(0);
			ALTRAM(0);
		} else {
			m_Page = 0;
			m_AltIdx = 0;
		}
	}

	if ( mode & RESET_CURSOR ) {
		m_CurX = 0;
		m_CurY = 0;
		m_DoWarp = FALSE;

		m_bRtoL = FALSE;
		m_bJoint = FALSE;
		m_Status = ST_NULL;
		m_LastChar = 0;
		m_LastFlag = 0;
		m_LastPos.SetPoint(0, 0);
		m_LastCur.SetPoint(0, 0);
		m_LastSize = CM_ASCII;
		m_LastAttr = 0;
		m_LastStr.Empty();
		m_Kan_Pos = 0;
		memset(m_Kan_Buf, 0, sizeof(m_Kan_Buf));

		memset(m_iTerm2MaekPos, 0, sizeof(CPoint) * 4);
	}
	
	if ( mode & RESET_CARET ) {
		m_TypeCaret = m_DefTypeCaret;
		m_CaretColor = m_DefCaretColor;
	}

	if ( mode & RESET_STATUS ) {
		if ( (m_StsMode & STSMODE_MASK) == STSMODE_INSCREEN )
			bSendSize = TRUE;
		m_StsMode = 0;
		m_StsLine = 0;
	}

	if ( mode & RESET_MARGIN ) {
		if ( m_StsMode == STSMODE_INSCREEN ) {
			m_TopY = 0;
			m_BtmY = m_Lines - m_StsLine;
		} else if ( m_StsMode == (STSMODE_ENABLE | STSMODE_INSCREEN) ) {
			m_TopY = m_Lines - m_StsLine;
			m_BtmY = m_Lines;
		} else {
			m_TopY = 0;
			m_BtmY = m_Lines;
		}
	}
	if ( mode &	RESET_RLMARGIN ) {
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
		fc_Init(m_KanjiMode);
	}

	if ( mode & RESET_ATTR ) {
		m_DefAtt.std.pack.dchar = 0;
		m_DefAtt.std.bank = m_BankTab[m_KanjiMode][m_BankGL];
		m_DefAtt.std.eram = 0;
		m_DefAtt.std.zoom = 0;
		m_DefAtt.std.mode = 0;
		m_DefAtt.eatt = 0;
		m_DefAtt.frgb = 0;
		m_DefAtt.brgb = 0;
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

#if 0
		// ext color 256-265
		m_ColTab[EXTCOL_VT_TEXT_FORE] = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_VT_TEXT_BACK] = m_ColTab[m_DefAtt.bcol];
		m_ColTab[EXTCOL_TEXT_CURSOR]  = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_MOUSE_FORE]   = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_MOUDE_BACK]   = m_ColTab[m_DefAtt.bcol];
		m_ColTab[EXTCOL_TEK_FORE]     = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_TEK_BACK]     = m_ColTab[m_DefAtt.bcol];
		m_ColTab[EXTCOL_HILIGHT_BACK] = m_ColTab[m_DefAtt.bcol];
		m_ColTab[EXTCOL_TEK_CURSOR]   = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_HILIGHT_FORE] = m_ColTab[m_DefAtt.fcol];

		// sp color 266-269
		m_ColTab[EXTCOL_TEXT_BOLD]    = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_TEXT_UNDER]   = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_TEXT_BLINK]   = m_ColTab[m_DefAtt.fcol];
		m_ColTab[EXTCOL_TEXT_REVERSE] = m_ColTab[m_DefAtt.fcol];
#endif

		// matrix color
		n = EXTCOL_MAX;
		for ( int g = 0 ; g < 24 ; g++ )
			m_ColTab[n++] = RGB(0, g * 11, 0);
		m_ColTab[n++] = RGB(255, 255, 255);
		m_ColTab[n++] = RGB(0, 0, 0);

		if ( m_bSixelColInit ) {
			delete [] m_pSixelColor;
			delete [] m_pSixelAlpha;
			m_bSixelColInit = 0;
			m_pSixelColor = NULL;
			m_pSixelAlpha = NULL;
		}
	}

	if ( mode & RESET_OPTION ) {
		memcpy(m_AnsiOpt, m_DefAnsiOpt, sizeof(m_AnsiOpt));
		SetRetChar(FALSE);

		if ( m_StsMode == (STSMODE_ENABLE | STSMODE_INSCREEN) )
			EnableOption(TO_DECOM);
	}

	if ( mode & RESET_XTOPT ) {
		m_XtOptFlag = 0;
		m_XtMosPointMode = 0;
		m_TitleStack.RemoveAll();
	}

	if ( mode & RESET_MODKEY ) {
		memcpy(m_ModKey, m_DefModKey, sizeof(m_ModKey));
		InitModKeyTab();
	}

	if ( mode & RESET_IDS ) {
		m_VtLevel = m_DefTermPara[TERMPARA_VTLEVEL];
		m_FirmVer = m_DefTermPara[TERMPARA_FIRMVER];
		m_TermId  = m_DefTermPara[TERMPARA_TERMID];
		m_UnitId  = m_DefTermPara[TERMPARA_UNITID];
		m_KeybId  = m_DefTermPara[TERMPARA_KEYBID];
		m_LangMenu = 0;
	}

	if ( mode & RESET_CLS )
		ERABOX(0, 0, m_Cols, m_Lines);

	if ( mode & RESET_HISTORY )
		m_HisLen = m_Lines;

	if ( mode & RESET_MOUSE ) {
		m_MouseTrack = MOS_EVENT_NONE;
		m_MouseRect.SetRectEmpty();
		m_Loc_Mode  = 0;
	}

	if ( mode & RESET_CHAR ) {
		m_MediaCopyMode = MEDIACOPY_NONE;
		m_Exact = FALSE;
		m_StsLed  = 0;
	}

	if ( mode & RESET_SAVE ) {
		SaveParam(&m_SaveParam);
		for ( int n = 0 ; n < 64 ; n++ )
			m_Macro[n].Clear();
		memset(m_MacroExecFlag, 0, sizeof(m_MacroExecFlag));
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
		TekClear(m_pTekWnd != NULL ? TRUE : FALSE);
	}

	if ( bSendSize && m_pDocument != NULL && m_pDocument->m_pSock != NULL )
		m_pDocument->m_pSock->SendWindSize();

	m_bInit = FALSE;
}
CCharCell *CTextRam::GETVRAM(int cols, int lines)
{
	int pos = m_HisPos + lines;

	while ( pos < 0 )
		pos += m_HisMax;

	while ( pos >= m_HisMax )
		pos -= m_HisMax;

	return m_pMemMap->GetMapRam(cols, pos);
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

		if ( m_pCallPoint == &CTextRam::fc_TraceCall && (m_TraceLogMode & 001) != 0 )
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
			m_pTraceNow->m_Attr = m_AttNow.std.attr;
			m_pTraceNow->m_ForCol = m_AttNow.std.fcol;
			m_pTraceNow->m_BakCol = m_AttNow.std.bcol;
			m_pTraceWnd->AddTraceNode(m_pTraceNow);
			m_bTraceUpdate = FALSE;
		}

		m_pTraceWnd->m_List.EnsureVisible(m_pTraceWnd->m_List.GetItemCount() - 1, FALSE);
		m_pTraceWnd->m_List.UnlockWindowUpdate();
	}
}
void CTextRam::CUROFF()
{
	m_DispCaret = FALSE;
}	
void CTextRam::CURON()
{
	m_DispCaret = TRUE;
}
void CTextRam::DISPUPDATE()
{
	m_UpdateRect.left   = 0;
	m_UpdateRect.top    = 0;
	m_UpdateRect.right  = m_Cols;
	m_UpdateRect.bottom = m_Lines;
	m_FrameCheck = FALSE;
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
	if ( m_UpdateRect.top > sy )
		m_UpdateRect.top = sy;
	if ( m_UpdateRect.bottom < ey )
		m_UpdateRect.bottom = ey;
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
	else if ( (int)m_AnsiPara[top] >= m_Lines )
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
	else if ( (int)m_AnsiPara[top] >= m_Cols )
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
	else if ( (int)m_AnsiPara[top] >= m_Lines )
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
	else if ( (int)m_AnsiPara[top] >= m_Cols )
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
			if ( (m_Loc_Rect.left >= m_Loc_Rect.right && m_Loc_Rect.top >= m_Loc_Rect.bottom) || m_Loc_Rect.PtInRect(CPoint(x, y)) )
				return;
			Pe = 10;
			break;
		default:
			return;
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
	TRACE("MouseReport %d,%d\n", md, sw);

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

	case MOS_LOCA_WHUP:
		stat = 0x40;
		break;
	case MOS_LOCA_WHDOWN:
		stat = 0x41;
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
		else if ( x < 4096 ) {
		    tmp += (CHAR)(0xC0 + (x >> 6));
		    tmp += (CHAR)(0x80 + (x & 0x3F));
		} else {
			tmp += "\xFF\xFF";
		}

		y += (' ' + 1);
		if ( y < 128 )
			tmp += (CHAR)(y);
		else if ( y < 4096 ) {
		    tmp += (CHAR)(0xC0 + (y >> 6));
		    tmp += (CHAR)(0x80 + (y & 0x3F));
		} else {
			tmp += "\xFF\xFF";
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
void CTextRam::AddOscClipCrLf(CString &text)
{
	switch(m_ClipCrLf) {
	case OSCCLIP_LF:
		text += _T('\x0A');
		break;
	case OSCCLIP_CR:
		text += _T('\x0D');
		break;
	case OSCCLIP_CRLF:
		text += _T("\x0D\x0A");
		break;
	}
}
void CTextRam::SetStatusMode(int fMode, int nLine)
{
	BOOL bSendSize = FALSE;

	switch(fMode) {
	case STSMODE_HIDE:
	case STSMODE_INDICATOR:
		if ( (m_StsMode & STSMODE_MASK) == STSMODE_INSCREEN ) {
			// In Screen Status Hide
			if ( (m_StsMode & STSMODE_ENABLE) != 0 )
				LoadParam(&m_StsParam, TRUE);
			if ( m_BtmY == (m_Lines - m_StsLine) )
				m_BtmY = m_Lines;
			m_StsMode = (m_StsMode & STSMODE_ENABLE) | fMode;
			ERABOX(0, m_Lines - m_StsLine, m_Cols, m_Lines);
			m_StsLine = 0;
			bSendSize = TRUE;
		} else
			m_StsMode = (m_StsMode & STSMODE_ENABLE) | fMode;
		break;
	case STSMODE_INSCREEN:
		if ( (m_StsMode & STSMODE_MASK) != STSMODE_INSCREEN ) {
			// In Screen Status Show
			if ( (m_StsLine = nLine) >= m_Lines )
				m_StsLine = m_Lines - 1;
			if ( m_StsLine <= 0 )
				break;
			while ( m_CurY >= (m_Lines - m_StsLine) ) {
				FORSCROLL(0, 0, m_Cols, m_Lines);
				m_CurY--;
			}
			while ( m_BtmY > (m_Lines - m_StsLine) )
				m_BtmY--;
			ERABOX(0, m_Lines - m_StsLine, m_Cols, m_Lines); 
			bSendSize = TRUE;

			SaveParam(&m_StsParam);
			m_StsParam.m_CurX = 0;
			m_StsParam.m_CurY = m_Lines - m_StsLine;
			m_StsParam.m_TopY = m_Lines - m_StsLine;
			m_StsParam.m_BtmY = m_Lines;
			m_StsParam.m_Decom = TRUE;

			if ( (m_StsMode & STSMODE_ENABLE) != 0 )
				SwapParam(&m_StsParam, TRUE);
		}
		m_StsMode = (m_StsMode & STSMODE_ENABLE) | STSMODE_INSCREEN;
		break;

	case STSMODE_DISABLE:
		switch(m_StsMode) {
		case STSMODE_HIDE:
		case STSMODE_INDICATOR:
		case STSMODE_INSCREEN:
			break;
		case STSMODE_ENABLE | STSMODE_HIDE:
		case STSMODE_ENABLE | STSMODE_INDICATOR:
			m_StsMode &= ~STSMODE_ENABLE;
			break;
		case STSMODE_ENABLE | STSMODE_INSCREEN:
			m_StsMode = STSMODE_INSCREEN;
			// Move Main Screen
			SwapParam(&m_StsParam, TRUE);
			break;
		}
		break;
	case STSMODE_ENABLE:
		switch(m_StsMode) {
		case STSMODE_HIDE:
			break;
		case STSMODE_INDICATOR:
			m_StsMode = STSMODE_ENABLE | STSMODE_INDICATOR;
			break;
		case STSMODE_INSCREEN:
			m_StsMode = STSMODE_ENABLE | STSMODE_INSCREEN;
			// Move Status Line
			SwapParam(&m_StsParam, TRUE);
			break;
		case STSMODE_ENABLE | STSMODE_HIDE:
			m_StsMode = STSMODE_HIDE;
			break;
		case STSMODE_ENABLE | STSMODE_INDICATOR:
		case STSMODE_ENABLE | STSMODE_INSCREEN:
			break;
		}
		break;
	}

	if ( bSendSize && m_pDocument != NULL && m_pDocument->m_pSock != NULL )
		m_pDocument->m_pSock->SendWindSize();
}
void CTextRam::SaveParam(SAVEPARAM *pSave)
{
	pSave->m_CurX     = m_CurX;
	pSave->m_CurY     = m_CurY;

	pSave->m_TopY     = m_TopY;
	pSave->m_BtmY     = m_BtmY;
	pSave->m_LeftX    = m_LeftX;
	pSave->m_RightX   = m_RightX;

	pSave->m_AttNow   = m_AttNow;
	pSave->m_AttSpc   = m_AttSpc;

	pSave->m_BankGL   = m_BankGL;
	pSave->m_BankGR   = m_BankGR;
	pSave->m_BankSG   = m_BankSG;
	memcpy(pSave->m_BankTab, m_BankTab, sizeof(m_BankTab));

	pSave->m_DoWarp   = m_DoWarp;
	pSave->m_Exact    = m_Exact;
	pSave->m_LastChar = m_LastChar;
	pSave->m_LastFlag = m_LastFlag;
	pSave->m_LastPos  = m_LastPos;
	pSave->m_LastSize = m_LastSize;
	pSave->m_LastAttr = m_LastAttr;
	wcscpy_s(pSave->m_LastStr, MAXCHARSIZE, m_LastStr);

	pSave->m_bRtoL    = m_bRtoL;
	pSave->m_bJoint   = m_bJoint;

	pSave->m_Decom    = (IsOptEnable(TO_DECOM)  ? TRUE : FALSE);
	pSave->m_Decawm   = (IsOptEnable(TO_DECAWM) ? TRUE : FALSE);
	memcpy(pSave->m_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));

	pSave->m_TabMap = m_TabFlag.SaveFlag(pSave->m_TabMap);
}
void CTextRam::LoadParam(SAVEPARAM *pSave, BOOL bAll)
{
	m_CurX     = pSave->m_CurX;
	m_CurY     = pSave->m_CurY;

	if ( m_CurX < 0 ) m_CurX = 0; else if ( m_CurX >= m_Cols ) m_CurX = m_Cols - 1;
	if ( m_CurY < 0 ) m_CurY = 0; else if ( m_CurY >= m_Lines ) m_CurY = m_Lines - 1;

	m_AttNow   = pSave->m_AttNow;
	m_AttSpc   = pSave->m_AttSpc;

	m_BankGL   = pSave->m_BankGL;
	m_BankGR   = pSave->m_BankGR;
	m_BankSG   = pSave->m_BankSG;
	memcpy(m_BankTab, pSave->m_BankTab, sizeof(m_BankTab));

	m_DoWarp   = pSave->m_DoWarp;
	m_Exact    = pSave->m_Exact;
	m_LastChar = pSave->m_LastChar;
	m_LastFlag = pSave->m_LastFlag;
	m_LastPos  = pSave->m_LastPos;
	m_LastSize = pSave->m_LastSize;
	m_LastAttr = pSave->m_LastAttr;
	m_LastStr  = pSave->m_LastStr;
	m_bRtoL    = pSave->m_bRtoL;
	m_bJoint   = pSave->m_bJoint;

	if ( bAll ) {
		m_TopY     = pSave->m_TopY;
		m_BtmY     = pSave->m_BtmY;
		m_LeftX    = pSave->m_LeftX;
		m_RightX   = pSave->m_RightX;

		if ( m_TopY < 0 ) m_TopY = 0; else if ( m_TopY >= m_Lines ) m_TopY = m_Lines - 1;
		if ( m_BtmY < 0 ) m_BtmY = 0; else if ( m_BtmY > m_Lines ) m_BtmY = m_Lines;

		if ( m_LeftX < 0 ) m_LeftX = 0; else if ( m_LeftX >= m_Cols ) m_LeftX = m_Cols - 1;
		if ( m_RightX < 0 ) m_RightX = 0; else if ( m_RightX > m_Cols ) m_RightX = m_Cols;

		memcpy(m_AnsiOpt, pSave->m_AnsiOpt, sizeof(m_AnsiOpt));
		m_TabFlag.LoadFlag(pSave->m_TabMap);
	}

	SetOption(TO_DECOM,  pSave->m_Decom);
	SetOption(TO_DECAWM, pSave->m_Decawm);
}
void CTextRam::SwapParam(SAVEPARAM *pSave, BOOL bAll)
{
	SAVEPARAM temp;

	SaveParam(&temp);
	LoadParam(pSave, bAll);

	if ( pSave->m_TabMap != NULL )
		delete [] pSave->m_TabMap;

	memcpy(pSave, &temp, sizeof(SAVEPARAM));
}
void CTextRam::CopyParam(SAVEPARAM *pDis, SAVEPARAM *pSrc)
{
	if ( pDis->m_TabMap != NULL )
		delete [] pDis->m_TabMap;

	memcpy(pDis, pSrc, sizeof(SAVEPARAM));
	pDis->m_TabMap = m_TabFlag.CopyFlag(pSrc->m_TabMap);
}

//////////////////////////////////////////////////////////////////////
// Low Level

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

	if ( m_StsMode == STSMODE_INSCREEN && m_CurY >= GetLineSize() )
		m_CurY = GetLineSize() - 1;
	else if ( m_StsMode == (STSMODE_ENABLE | STSMODE_INSCREEN) && m_CurY < GetLineSize() )
		m_CurY = GetLineSize();

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

	if ( m_StsMode == STSMODE_INSCREEN ) {
		if ( sy >= GetLineSize() ) return;
		if ( ey > GetLineSize() ) ey = GetLineSize();
		if ( ey <= sy ) return;
	} else if ( m_StsMode == (STSMODE_ENABLE | STSMODE_INSCREEN) ) {
		if ( ey < GetLineSize() ) return;
		if ( sy < GetLineSize() ) sy = GetLineSize();
		if ( ey <= sy ) return;
	}

	if ( sx == 0 && ex == m_Cols && sy == 0 && GetLineSize() > 1 && IsOptEnable(TO_RLCLSBACK) ) {
		for ( x = ey ; x > 0 ; x-- ) {
			if ( !IsEmptyLine(x - 1) )
				break;
		}
		for ( y = 0 ; y < x ; y++)
			FORSCROLL(0, 0, m_Cols, ey);

		m_pDocument->UpdateAllViews(NULL, UPDATE_SCROLLOUT, NULL);
		return;
	}

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

	if ( sy == 0 && sx == 0 && ex == m_Cols && !m_LineEditMode && !m_bTraceActive ) {
		if ( m_LogMode == LOGMOD_PAGE )
			CallReceiveLine(0);

		m_HisUse++;

		if ( ++m_HisAbs >= (m_HisMax * 2) ) {
			POSITION pos = m_CommandHistory.GetHeadPosition();
			while ( pos != NULL ) {
				CMDHIS *pCmdHis = m_CommandHistory.GetNext(pos);
				pCmdHis->habs -= m_HisMax;	// habs = m_HisAbs + y...
			}

			m_HisAbs -= m_HisMax;
		}

		if ( (m_HisPos += 1) >= m_HisMax )
			m_HisPos -= m_HisMax;
		if ( m_HisLen < (m_HisMax - 1) )
			m_HisLen += 1;
		if ( m_LineUpdate < m_HisMax )
			m_LineUpdate++;

		if ( ey == m_Lines )
			CCharCell::Fill(GETVRAM(0, m_Lines - 1), m_AttSpc, m_ColsMax);
		else {
			for ( int y = m_Lines - 1; y >= ey ; y-- )
				CCharCell::Copy(GETVRAM(0, y), GETVRAM(0, y - 1), m_Cols);

			CCharCell::Fill(GETVRAM(0, ey - 1), m_AttSpc, m_ColsMax);
		}

	} else {
		for ( int y = sy + 1 ; y < ey ; y++ )
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

	m_DoWarp = FALSE;

	for ( int y = ey - 1 ; y > sy ; y-- )
		CCharCell::Copy(GETVRAM(sx, y), GETVRAM(sx, y - 1), ex - sx);

	ERABOX(sx, sy, ex, sy + 1);
	DISPRECT(sx, sy, ex, ey);
}
void CTextRam::LEFTSCROLL()
{
	int n, i, dm;
	CCharCell *vp;

	m_DoWarp = FALSE;

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

	m_DoWarp = FALSE;

	GetMargin(MARCHK_NONE);

	for ( n = m_Margin.top ; n < m_Margin.bottom ; n++ ) {
		vp = GETVRAM(0, n);
		dm = vp->m_Vram.zoom;
		for ( i = m_Margin.right - 1 ; i > m_Margin.left ; i-- )
			vp[i] = vp[i - 1];
		vp[m_Margin.left] = m_AttSpc;
		//vp[m_Margin.left] = L' ';
		vp->m_Vram.zoom = dm;
	}

	DISPRECT(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
}
void CTextRam::DOWARP()
{
	if ( !m_DoWarp )
		return;

	m_DoWarp = FALSE;
	m_CurX = GetLeftMargin();

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
		//vp[n] = L' ';
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
			if ( m_CurX >= m_Margin.left && m_CurX < m_Margin.right ) {
				FORSCROLL(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
				m_LineEditIndex++;

				if ( IsOptEnable(TO_DECSCLM) && ((m_pDocument != NULL && m_pDocument->m_pSock != NULL && m_pDocument->m_pSock->GetRecvProcSize() < RECVDEFSIZ) || (clock() - m_UpdateClock) > 20) ) {
					m_UpdateClock = clock();
					m_UpdateFlag = TRUE;
				}

				if ( m_iTerm2Mark != 0 ) {
					m_iTerm2MaekPos[0].y--;
					m_iTerm2MaekPos[1].y--;
					m_iTerm2MaekPos[2].y--;
					m_iTerm2MaekPos[3].y--;
				}
			}
		}
	} else {
		if ( ++m_CurY >= m_Lines )
			m_CurY = m_Lines - 1;
	}
	LOCATE(m_CurX, m_CurY);
}
void CTextRam::REVINDEX()
{
	m_DoWarp = FALSE;

	if ( GetMargin(MARCHK_LINES) ) {
		if ( --m_CurY < m_Margin.top ) {
			m_CurY = m_Margin.top;
			if ( m_CurX >= m_Margin.left && m_CurX < m_Margin.right )
				BAKSCROLL(m_Margin.left, m_Margin.top, m_Margin.right, m_Margin.bottom);
		}
	} else {
		if ( --m_CurY < 0 )
			m_CurY = 0;
	}
	LOCATE(m_CurX, m_CurY);
}
void CTextRam::PUT1BYTE(DWORD ch, int md, int at, LPCWSTR str)
{
	int block;
	CCharCell *vp;
	int dm = GetDm(m_CurY);
	WORD bank = (WORD)md;

	DOWARP();

	md &= CODE_MASK;

#if 0
	// ISO946 0x23, 0x24, 0x40, 0x5B, 0x5C, 0x5D, 0x5E, 0x60, 0x7B, 0x7C, 0x7D, 0x7E,
	static const DWORD iso646map[4] = { 0x00000000, 0x00000018, 0x78000001, 0x78000001 };

	if ( str == NULL && !(ch < 128 && (md & SET_MASK) <= SET_96 && m_FontTab[md].m_Shift == 0) && (iso646map[ch / 32] & (1 << (ch % 32))) != 0) {
#else
	if ( str == NULL && !(ch < 128 && (md & SET_MASK) <= SET_96 && m_FontTab[md].m_Shift == 0) ) {
#endif
		ch |= m_FontTab[md].m_Shift;
		ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, _T("UTF-16BE"), ch);			// Char変換ではUTF-16BEを使用！
		ch = IconvToMsUnicode(m_FontTab[md].m_JpSet, ch);
	}

	m_LastChar = ch;
	m_LastFlag = 0;
	m_LastPos.SetPoint(m_CurX, m_CurY + m_HisPos);
	m_LastCur = m_LastPos;
	m_LastSize = CM_ASCII;
	m_LastAttr = at;

	if ( str == NULL && ch != 0 && CallReceiveChar(ch) )
		return;

	if ( !GetMargin(MARCHK_COLS) ) {
		m_Margin.left  = 0;
		m_Margin.right = m_Cols;
	}

	if ( dm != 0 ) {
		m_Margin.left  /= 2;
		m_Margin.right /= 2;
	}

	if ( m_bRtoL )
		at |= ATT_RTOL;

	vp = GETVRAM(m_CurX, m_CurY);

	vp->m_Vram.bank = bank;
	vp->m_Vram.eram = m_AttNow.std.eram;
//	vp->m_Vram.zoom = m_AttNow.dm;	no Init
	vp->m_Vram.mode = CM_ASCII;
	vp->m_Vram.attr = m_AttNow.std.attr | at;
	vp->m_Vram.font = m_AttNow.std.font;
	vp->m_Vram.fcol = m_AttNow.std.fcol;
	vp->m_Vram.bcol = m_AttNow.std.bcol;
	vp->GetEXTVRAM(m_AttNow);

	if ( m_bRtoL && md != SET_UNICODE ) {
		if ( (block = m_FontTab.m_UniBlockTab.Find(0x0600)) != (-1) )
			vp->m_Vram.bank = (WORD)block;
		else
			vp->m_Vram.bank = (WORD)SET_UNICODE;
	} else if ( md == SET_UNICODE && (block = m_FontTab.m_UniBlockTab.Find(ch)) != (-1) )
		vp->m_Vram.bank = (WORD)block;

	if ( ch >= 0x2500 && ch <= 0x259F && !IsOptEnable(TO_RLDRWLINE) )		// Border Char
		vp->m_Vram.attr |= ATT_BORDER;

	if ( str != NULL ) {
		*vp = str;
	} else {
		*vp = (DWORD)ch;
		m_LastStr = (LPCWSTR)(*vp);
	}

	if ( dm != 0 )
		DISPVRAM(m_CurX * 2, m_CurY, 2, 1);
	else
		DISPVRAM(m_CurX, m_CurY, 1, 1);

	if ( ++m_CurX >= m_Margin.right ) {
		if ( IsOptEnable(TO_DECAWM) != 0 ) {
			if ( IsOptEnable(TO_RLGNDW) != 0 ) {
				m_CurX = m_Margin.left;
				ONEINDEX();
			} else {
				m_CurX = m_Margin.right - 1;
				m_DoWarp = TRUE;
			}
		} else 
			m_CurX = m_Margin.right - 1;
	}

	m_LastCur.SetPoint(m_CurX, m_CurY);
}
void CTextRam::PUT2BYTE(DWORD ch, int md, int at, LPCWSTR str)
{
	int block;
	CCharCell *vp;
	int dm = GetDm(m_CurY);
	WORD bank = (WORD)md;

	DOWARP();

	md &= CODE_MASK;

	if ( str == NULL ) {
		ch = m_IConv.IConvChar(m_FontTab[md].m_IContName, _T("UTF-16BE"), ch);			// Char変換ではUTF-16BEを使用！
		ch = IconvToMsUnicode(m_FontTab[md].m_JpSet, ch);
	}

	m_LastChar = ch;
	m_LastFlag = 0;
	m_LastPos.SetPoint(m_CurX, m_CurY + m_HisPos);
	m_LastCur = m_LastPos;
	m_LastSize = CM_1BYTE;
	m_LastAttr = at;

	if ( str == NULL && ch != 0 && CallReceiveChar(ch) )
		return;

	if ( !GetMargin(MARCHK_COLS) ) {
		m_Margin.left  = 0;
		m_Margin.right = m_Cols;
	}

	if ( dm != 0 ) {
		m_Margin.left  /= 2;
		m_Margin.right /= 2;
	}

	if ( (m_CurX + 1) >= m_Margin.right ) {
		m_DoWarp = TRUE;
		DOWARP();

		m_LastPos.SetPoint(m_CurX, m_CurY + m_HisPos);		// Reset

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

	vp[0].m_Vram.bank = vp[1].m_Vram.bank = bank;
	vp[0].m_Vram.eram = vp[1].m_Vram.eram = m_AttNow.std.eram;
//	vp[0].dm = vp[1].dm = m_AttNow.dm;	no Init
	vp[0].m_Vram.mode = CM_1BYTE;
	vp[1].m_Vram.mode = CM_2BYTE;
	vp[0].m_Vram.attr = vp[1].m_Vram.attr = m_AttNow.std.attr | at;
	vp[0].m_Vram.font = vp[1].m_Vram.font = m_AttNow.std.font;
	vp[0].m_Vram.fcol = vp[1].m_Vram.fcol = m_AttNow.std.fcol;
	vp[0].m_Vram.bcol = vp[1].m_Vram.bcol = m_AttNow.std.bcol;
	vp[0].GetEXTVRAM(m_AttNow); vp[1].GetEXTVRAM(m_AttNow);

	if ( md == SET_UNICODE && (block = m_FontTab.m_UniBlockTab.Find(ch)) != (-1) )
		vp[0].m_Vram.bank = vp[1].m_Vram.bank = (WORD)block;

	if ( ch >= 0x2500 && ch <= 0x259F && !IsOptEnable(TO_RLDRWLINE) )		// Border Char
		vp[0].m_Vram.attr |= ATT_BORDER;

	if ( str != NULL ) {
		vp[0] = str;
		vp[1] = str;
	} else {
		vp[0] = (DWORD)ch;
		vp[1] = (DWORD)ch;
		m_LastStr = (LPCWSTR)(*vp);
	}

	if ( dm != 0 )
		DISPVRAM(m_CurX * 2, m_CurY, 4, 1);
	else
		DISPVRAM(m_CurX, m_CurY, 2, 1);

	if ( (m_CurX += 2) >= m_Margin.right ) {
		if ( IsOptEnable(TO_DECAWM) != 0 ) {
			if ( IsOptEnable(TO_RLGNDW) != 0 ) {
				m_CurX = m_Margin.left;
				ONEINDEX();
			} else {
				m_CurX = m_Margin.right - 1;
				m_DoWarp = TRUE;
			}
		} else 
			m_CurX = m_Margin.right - 1;
	}

	m_LastCur.SetPoint(m_CurX, m_CurY);
}
void CTextRam::PUTADD(int x, int y, DWORD ch)
{
	CCharCell *vp;

	if ( ch != 0 && CallReceiveChar(ch) )
		return;

	if ( x < 0 || x >= m_Cols )
		return;

	if ( y >= m_Lines )		// y < 0 OK ?
		return;

	vp = GETVRAM(x, y);
	*vp += (DWORD)ch;

	m_LastStr = *vp;

	if ( x < (m_Cols - 1) && IS_1BYTE(vp[0].m_Vram.mode) && IS_2BYTE(vp[1].m_Vram.mode) ) {
		vp[1] = (LPCWSTR)(*vp);
		DISPVRAM(x, y, 2, 1);
	} else {
		DISPVRAM(x, y, 1, 1);
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
		if ( IS_ENABLE(m_SaveParam.m_AnsiOpt, bit) )
			EnableOption(bit);
		break;
	case 's':
		m_SaveParam.m_AnsiOpt[bit / 32] &= ~(1 << (bit % 32));
		if ( IsOptEnable(bit) )
			m_SaveParam.m_AnsiOpt[bit / 32] |= (1 << (bit % 32));
		break;
	}
}
void CTextRam::INSMDCK(int len)
{
	if ( IsOptEnable(TO_ANSIIRM) == 0 )
		return;

	if ( m_DoWarp )
		return;

	while ( len-- > 0 )
		INSCHAR(FALSE);
}
CTextSave *CTextRam::GETSAVERAM(int fMode)
{
	int n, i, x;
	CCharCell *vp, *wp;
	CTextSave *pSave = new CTextSave;
	CGrapWnd *pWnd;
	
	pSave->m_pTextRam = this;
	pSave->m_bAll = (fMode == SAVEMODE_ALL ? TRUE : FALSE);

	if ( fMode >= SAVEMODE_VRAM ) {
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
	}

	for ( i = 0 ; i < pSave->m_GrapIdx.GetSize() ; i++ ) {
		if ( (pWnd = GetGrapWnd(pSave->m_GrapIdx[i])) != NULL )
			pWnd->m_Use++;
	}

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
	pSave->m_LastCur  = m_LastCur;
	pSave->m_LastSize = m_LastSize;
	pSave->m_LastAttr = m_LastAttr;
	wcscpy_s(pSave->m_LastStr, MAXCHARSIZE, m_LastStr);
	pSave->m_bRtoL    = m_bRtoL;
	pSave->m_bJoint   = m_bJoint;

	memcpy(pSave->m_AnsiOpt, m_AnsiOpt, sizeof(m_AnsiOpt));

	pSave->m_BankGL = m_BankGL;
	pSave->m_BankGR = m_BankGR;
	pSave->m_BankSG = m_BankSG;
	memcpy(pSave->m_BankTab, m_BankTab, sizeof(m_BankTab));
	
	pSave->m_TabMap = m_TabFlag.SaveFlag(pSave->m_TabMap);

	CopyParam(&pSave->m_SaveParam, &m_SaveParam);

	pSave->m_StsMode = m_StsMode;
	pSave->m_StsLine = m_StsLine;

	CopyParam(&pSave->m_StsParam, &m_StsParam);

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
		//m_StsLed
		//m_KanjiMode SetKanjiMode(mode) ???
	}

	return pSave;
}
void CTextRam::SETSAVERAM(CTextSave *pSave)
{
	if ( pSave->m_bAll && (pSave->m_Cols != m_Cols || pSave->m_Lines != m_Lines) )
		InitScreen(pSave->m_Cols, pSave->m_Lines);

	m_AttNow = pSave->m_AttNow;
	m_AttSpc = pSave->m_AttSpc;

	if ( (m_CurX = pSave->m_CurX) >= m_Cols )  m_CurX = m_Cols - 1;
	if ( (m_CurY = pSave->m_CurY) >= m_Lines ) m_CurY = m_Lines - 1;

	if ( (m_TopY = pSave->m_TopY) >= m_Lines ) m_TopY = m_Lines - 1;
	if ( (m_BtmY = pSave->m_BtmY) > m_Lines || pSave->m_BtmY == pSave->m_Lines )  m_BtmY = m_Lines;

	if ( (m_LeftX  = pSave->m_LeftX) >= m_Cols ) m_LeftX  = m_Lines - 1;
	if ( (m_RightX = pSave->m_RightX) > m_Cols ) m_RightX = m_Cols;

	m_DoWarp   = pSave->m_DoWarp;
	m_Exact    = pSave->m_Exact;
	m_LastChar = pSave->m_LastChar;
	m_LastFlag = pSave->m_LastFlag;
	m_LastPos  = pSave->m_LastPos;
	m_LastCur  = pSave->m_LastCur;
	m_LastSize = pSave->m_LastSize;
	m_LastAttr = pSave->m_LastAttr;
	m_LastStr  = pSave->m_LastStr;

	memcpy(m_AnsiOpt, pSave->m_AnsiOpt, sizeof(DWORD) * 12);	// 0 - 384

	m_BankGL = pSave->m_BankGL;
	m_BankGR = pSave->m_BankGR;
	m_BankSG = pSave->m_BankSG;
	memcpy(m_BankTab, pSave->m_BankTab, sizeof(m_BankTab));

	m_TabFlag.LoadFlag(pSave->m_TabMap);

	CopyParam(&m_SaveParam, &pSave->m_SaveParam);

	m_StsMode = pSave->m_StsMode;
	m_StsLine = pSave->m_StsLine;

	CopyParam(&m_StsParam, &pSave->m_StsParam);

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
		//m_StsLed
		//m_KanjiMode SetKanjiMode(mode) ???
	}

	if ( pSave->m_pCharCell != NULL ) {
		int n, x, cx;
		CCharCell *vp, *wp;

		cx = (pSave->m_Cols < m_Cols ? pSave->m_Cols : m_Cols);
		for ( n = 0 ; n < pSave->m_Lines && n < m_Lines ; n++ ) {
			vp = GETVRAM(0, n);
			wp = pSave->m_pCharCell + n * pSave->m_Cols;
			for ( x = 0 ; x < cx ; x++ )
				*(vp++) = *(wp++);
		}

		DISPVRAM(0, 0, m_Cols, m_Lines);
	}
}
void CTextRam::SAVERAM()
{
	int n;
	CTextSave *pNext;
	CTextSave *pSave;

	if ( m_bTraceActive )
		return;

	pSave = GETSAVERAM(SAVEMODE_VRAM);

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
void CTextRam::ALTRAM(int idx)
{
	if ( idx < 0 )
		idx = 0;
	else if ( idx > 1 )
		idx = 1;

	if ( idx == m_AltIdx )
		return;

	SAVERAM();

	if ( m_pAltSave[m_AltIdx] != NULL )
		delete m_pAltSave[m_AltIdx];

	m_pAltSave[m_AltIdx] = m_pTextSave;
	m_pTextSave = m_pTextSave->m_pNext;

	if ( m_pAltSave[idx] != NULL ) {
		m_pAltSave[idx]->m_pNext = m_pTextSave;
		m_pTextSave = m_pAltSave[idx];
		m_pAltSave[idx] = NULL;
		LOADRAM();
	} else {
		ERABOX(0, 0, m_Cols, m_Lines);
		LOCATE(0, 0);
	}

	m_AltIdx = idx;
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
		sy += GetTopMargin();
		ex += GetLeftMargin();
		ey += GetTopMargin();
		dx += GetLeftMargin();
		dy += GetTopMargin();
	}

	if      ( sp < 0 )   sp = 0;
	else if ( sp > 100 ) sp = 100;
	if      ( dp < 0 )   dp = 0;
	else if ( dp > 100 ) dp = 100;

	if ( sx < 0 )  sx = 0;
	if ( ex < sx ) return;	// ex = sx;
	if ( sy < 0 )  sy = 0;
	if ( ey < sy ) return;	//ey = sy;
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
							break;	//tp = &spc;
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
							continue;	//tp = &spc;
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
							break;	//tp = &spc;
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
							continue;	//tp = &spc;
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
						break;	//tp = &spc;
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
						break;	//tp = &spc;
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
						break;	//tp = &spc;
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
			m_TabFlag.SetMultiColsFlag(m_CurX, m_CurY);
		else							// SINGLE
			m_TabFlag.SetSingleColsFlag(m_CurX);
		break;
	case TAB_COLSCLR:		// Cols Clear
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			m_TabFlag.ClrMultiColsFlag(m_CurX, m_CurY);
		else							// SINGLE
			m_TabFlag.ClrSingleColsFlag(m_CurX);
		break;
	case TAB_COLSALLCLR:	// Cols All Claer
		if ( IsOptEnable(TO_ANSITSM) )	// MULTIPLE
			m_TabFlag.AllClrMulti();
		else							// SINGLE
			m_TabFlag.AllClrSingle();
		break;

	case TAB_COLSALLCLRACT:	// Cols All Clear if Active Line
		m_TabFlag.LineClrMulti(m_CurY);
		break;

	case TAB_LINESET:		// Line Set
		m_TabFlag.SetLineflag(m_CurY);
		break;
	case TAB_LINECLR:		// Line Clear
		m_TabFlag.ClrLineflag(m_CurY);
		break;
	case TAB_LINEALLCLR:	// Line All Clear
		m_TabFlag.AllClrLine();
		break;

	case TAB_RESET:			// Reset
		if ( m_pMemMap != NULL )
			m_TabFlag.ResetAll(m_pMemMap->m_ColsMax, m_pMemMap->m_LineMax, m_DefTab);
		else
			m_TabFlag.ResetAll(m_ColsMax, m_Lines, m_DefTab);
		break;

	case TAB_DELINE:		// Delete Line
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			i = GetBottomMargin() - 1;
			for ( n = m_CurY ; n < i ; n++ )
				m_TabFlag.CopyLine(n, n + 1);
		}
		break;
	case TAB_INSLINE:		// Insert Line
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			i = GetBottomMargin() - 1;
			for ( n = i ; n > m_CurY ; n-- )
				m_TabFlag.CopyLine(n + 1, n);
		}
		break;

	case TAB_ALLCLR:		// Cols Line All Clear
		m_TabFlag.AllClrLine();
		m_TabFlag.AllClrSingle();
		m_TabFlag.AllClrMulti();
		break;

	case TAB_COLSNEXT:		// Cols Tab Stop
		if ( IsOptEnable(TO_XTMCUS) )
			DOWARP();
		i = IsOptEnable(TO_ANSITSM);
		GetMargin(MARCHK_NONE);
		if ( m_CurX >= m_Margin.right )
			m_Margin.right = m_Cols;
		for ( n = m_CurX + 1 ; n < m_Cols ; n++ ) {
			if ( m_TabFlag.IsColsFlag(n, m_CurY, i) )
				break;
		}
		if ( n >= m_Margin.right )
			n = m_Margin.right - 1;
		if ( n > m_CurX ) {
			CCharCell *vp = GETVRAM(m_CurX, m_CurY);
			if ( !IS_IMAGE(vp->m_Vram.mode) && (DWORD)(*vp) <= L' ' ) {
				*vp = (DWORD)'\t';
				vp->m_Vram.bank = (WORD)m_BankTab[m_KanjiMode][0];
				vp->m_Vram.eram = m_AttNow.std.eram;
				vp->m_Vram.mode = CM_ASCII;
				vp->m_Vram.attr = m_AttNow.std.attr;
				vp->m_Vram.font = m_AttNow.std.font;
				vp->m_Vram.fcol = m_AttNow.std.fcol;
				vp->m_Vram.bcol = m_AttNow.std.bcol;
				vp->GetEXTVRAM(m_AttNow);
			}
		}
		LOCATE(n, m_CurY);
		break;
	case TAB_COLSBACK:		// Cols Back Tab Stop
		GetMargin(MARCHK_NONE);
		i = IsOptEnable(TO_ANSITSM);
		if ( m_CurX < m_Margin.left )
			m_Margin.left = 0;
		for ( n = m_CurX - 1 ; n > 0 ; n-- ) {
			if ( m_TabFlag.IsColsFlag(n, m_CurY, i) )
				break;
		}
		if ( n < m_Margin.left )
			n = m_Margin.left;
		LOCATE(n, m_CurY);
		break;
	case TAB_CHT:		// Cols Tab Stop
		if ( IsOptEnable(TO_XTMCUS) )
			DOWARP();
		i = IsOptEnable(TO_ANSITSM);
		for ( n = m_CurX + 1 ; n < m_Cols ; n++ ) {
			if ( m_TabFlag.IsColsFlag(n, m_CurY, i) )
				break;
		}
		if ( n > m_CurX ) {
			CCharCell *vp = GETVRAM(m_CurX, m_CurY);
			if ( !IS_IMAGE(vp->m_Vram.mode) && (DWORD)(*vp) <= L' ' ) {
				*vp = (DWORD)'\t';
				vp->m_Vram.bank = (WORD)m_BankTab[m_KanjiMode][0];
				vp->m_Vram.eram = m_AttNow.std.eram;
				vp->m_Vram.mode = CM_ASCII;
				vp->m_Vram.attr = m_AttNow.std.attr;
				vp->m_Vram.font = m_AttNow.std.font;
				vp->m_Vram.fcol = m_AttNow.std.fcol;
				vp->m_Vram.bcol = m_AttNow.std.bcol;
				vp->GetEXTVRAM(m_AttNow);
			}
		}
		LOCATE(n, m_CurY);
		break;
	case TAB_CBT:		// Cols Back Tab Stop
		i = IsOptEnable(TO_ANSITSM);
		for ( n = m_CurX - 1 ; n > 0 ; n-- ) {
			if ( m_TabFlag.IsColsFlag(n, m_CurY, i) )
				break;
		}
		LOCATE(n, m_CurY);
		break;

	case TAB_LINENEXT:		// Line Tab Stop
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_CurY + 1 ; n < m_Lines ; n++ ) {
				if ( m_TabFlag.IsLineFlag(n) && n >= m_TopY )
					break;
			}
			if ( n >= m_BtmY )
				n = m_BtmY - 1;
			LOCATE(m_CurX, n);
		} else {
			ONEINDEX();
		}
		break;
	case TAB_LINEBACK:		// Line Back Tab Stop
		if ( IsOptEnable(TO_ANSITSM) ) {	// MULTIPLE
			for ( n = m_CurY - 1 ; n > 0 ; n-- ) {
				if ( m_TabFlag.IsLineFlag(n) && n < m_BtmY )
					break;
			}
			if ( n < m_TopY )
				n = m_TopY;
			LOCATE(m_CurX, n);
		} else {
			REVINDEX();
		}
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
	DWORD min = 0xFFFFFF;	// 255 * 255 * 2 + 255 * 255 * 6 + 255 * 255 = 585225 = 0x0008EE09

	if ( red   > 255 ) red   = 255;
	if ( green > 255 ) green = 255;
	if ( blue  > 255 ) blue  = 255;

	for ( i = 0 ; i < 256 ; i++ ) {
		int r = GetRValue(m_ColTab[i]) - red;
		int g = GetGValue(m_ColTab[i]) - green;
		int b = GetBValue(m_ColTab[i]) - blue;
		DWORD d = r * r * 2 + g * g * 6 + b * b;
		if ( min > d ) {
			idx = i;
			min = d;
		}
	}
	return idx;
}
