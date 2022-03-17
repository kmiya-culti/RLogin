#include "StdAfx.h"
#include <stdarg.h>
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Regex.h"

/*******************************************************
	int n, i;
	CRegEx reg;
	CRegExRes res;
	CStringW str = L"0ABCABCDEAAAAAAZ9A0";

	reg.Compile("A+[B-Z]+");
	reg.MatchCharInit();

	for ( n = 0 ; n < str.GetLength() ; n++ ) {
		// 最大一致
		// 0ABCABCDEAAAAAAZ9A0
		//     ^ABC |      |
		//          ^ABCDE |
		//                 ^AAAAAAZ
		if ( reg.MatchChar(str[n], n, &res) && (res.m_Status == REG_MATCH || res.m_Status == REG_MATCHOVER) ) {
			for ( i = 0 ; i < res.GetSize() ; i++ )
				TRACE("%d %d:%s(%d)\n", n, i, res[i].m_Str, res.m_Status);
		}

		// 最小一致
		// 0ABCABCDEAAAAAAZ9A0
		//   ^AB|         |
		//      ^AB       |
		//                ^AAAAAAZ
		if ( reg.MatchChar(str[n], n, &res) ) {
			reg.MatchCharInit();
			for ( i = 0 ; i < res.GetSize() ; i++ )
				TRACE("%d %d:%s(%d)\n", n, i, res[i].m_Str, res.m_Status);
		}

		// 随時一致
		// 0ABCABCDEAAAAAAZ9A0
		//   ^AB          |
		//    ^ABC        |
		//      ^AB       |
		//       ^ABC     |
		//        ^ABCD   |
		//         ^ABCDE |
		//                ^AAAAAAZ
		if ( reg.MatchChar(str[n], n, &res) && res.m_Status != REG_MATCHOVER ) {
			for ( i = 0 ; i < res.GetSize() ; i++ )
				TRACE("%d %d:%s(%d)\n", n, i, res[i].m_Str, res.m_Status);
		}
	}
********************************************************/

//////////////////////////////////////////////////////////////////////
// CStringD

CStringD::CStringD()
{
	Empty();
}
CStringD::CStringD(const CStringD &data, int iFirst, int nCount)
{
	int n;
	int len = (int)data.m_Data.GetSize() - 1;

	Empty();

	if ( iFirst < 0 )
		iFirst = 0;

	if ( nCount < 0 )
		nCount = len - iFirst;

	for ( n = 0 ; n < nCount && iFirst < len ; n++, iFirst++ ) {
		m_Data.InsertAt(n, data.m_Data[iFirst]);
		m_Vs.InsertAt(n, data.m_Vs[iFirst]);
	}
}
CStringD::~CStringD()
{
	m_Data.RemoveAll();
	m_Vs.RemoveAll();
}
CStringD & CStringD::operator += (DCHAR ch)
{
	Insert(GetLength(), ch);
	return *this;
}
CStringD & CStringD::operator += (const CStringD &data)
{
	int n;
	int len = (int)data.m_Data.GetSize() - 1;
	int idx = GetLength();

	for ( n = 0 ; n < len ; n++, idx++ ) {
		m_Data.InsertAt(idx, data.m_Data[n]);
		m_Vs.InsertAt(idx, data.m_Vs[n]);
	}
	return *this;
}
CStringD &CStringD::operator += (LPCWSTR str)
{
	DCHAR ch;

	while ( *str != L'\0' ) {
		//           1101 10** **** ****	U+D800 - U+DBFF	上位サロゲート
		//	         1101 11xx xxxx xxxx	U+DC00 - U+DFFF	下位サロゲート
		// 000? **** **** **xx xxxx xxxx	U+010000 - U+10FFFF 21 bit
		if ( (str[0] & 0xFC00) == 0xD800 && (str[1] & 0xFC00) == 0xDC00 ) {
			ch = (((str[0] & 0x03FF) << 10) | (str[1] & 0x3FF)) + 0x10000L;
			str += 2;
		} else
			ch = *(str++);

		*this += ch;
	}
	return *this;
}
CStringD &CStringD::operator += (LPCDSTR str)
{
	while ( *str != 0 )
		*this += *(str++);
	return *this;
}
LPCWSTR CStringD::WStr(CStringW &str)
{
	int n;
	int len = GetLength();
	DCHAR ch;

	str.Empty();
	for ( n = 0 ; n < len ; n++ ) {
		ch = m_Data[n];

		if ( IsRegSpChar(ch) )
			continue;

		if ( (ch & 0xFFFF0000) != 0 ) {
			// 000? **** **** **xx xxxx xxxx	U+010000 - U+10FFFF 21 bit
			//           1101 10** **** ****	U+D800 - U+DBFF	上位サロゲート
			//	         1101 11xx xxxx xxxx	U+DC00 - U+DFFF	下位サロゲート
			ch -= 0x10000;
			str += (WCHAR)((ch >> 10)    | 0xD800);
			str += (WCHAR)((ch & 0x03FF) | 0xDC00);
		} else
			str += (WCHAR)(ch);

		ch = m_Vs[n];
		if ( (ch & 0xFFFF0000) != 0 ) {
			ch -= 0x10000;
			str += (WCHAR)((ch >> 10)    | 0xD800);
			str += (WCHAR)((ch & 0x03FF) | 0xDC00);
		} else if ( ch != 0 )
			str += (WCHAR)(ch);
	}
	return str;
}
LPCWSTR CStringD::GetAt(int idx)
{
	DCHAR ch;

	m_Work.Empty();

	if ( idx >= GetLength() )
		return m_Work;

	ch = m_Data[idx];

	if ( IsRegSpChar(ch) )
		return m_Work;

	if ( (ch & 0xFFFF0000) != 0 ) {
		// 000? **** **** **xx xxxx xxxx	U+010000 - U+10FFFF 21 bit
		//           1101 10** **** ****	U+D800 - U+DBFF	上位サロゲート
		//	         1101 11xx xxxx xxxx	U+DC00 - U+DFFF	下位サロゲート
		ch -= 0x10000;
		m_Work += (WCHAR)((ch >> 10)    | 0xD800);
		m_Work += (WCHAR)((ch & 0x03FF) | 0xDC00);
	} else
		m_Work += (WCHAR)(ch);

	ch = m_Vs[idx];
	if ( (ch & 0xFFFF0000) != 0 ) {
		ch -= 0x10000;
		m_Work += (WCHAR)((ch >> 10)    | 0xD800);
		m_Work += (WCHAR)((ch & 0x03FF) | 0xDC00);
	} else if ( ch != 0 )
		m_Work += (WCHAR)(ch);

	return m_Work;
}
int CStringD::ReverseFind(DCHAR ch)
{
	int len = GetLength();

	while ( --len >= 0 ) {
		if ( m_Data[len] == ch )
			return len;
	}
	return (-1);
}
int CStringD::Find(DCHAR ch, int iStart)
{
	int len = GetLength();

	while ( iStart < len ) {
		if ( m_Data[iStart] == ch )
			return iStart;
		iStart++;
	}
	return (-1);
}
int CStringD::Find(LPCDSTR str, int iStart)
{
	int n;
	LPCDSTR p, s;

	while ( (n = Find(*str, iStart)) >= 0 ) {
		p = m_Data.GetData() + n;
		for ( s = str ; *s != 0 ; ) {
			if ( *(s++) != *(p++) )
				break;
		}
		if ( *s == 0 )
			return n;
		iStart = n + 1;
	}
	return (-1);
}
int CStringD::Compare(LPCDSTR str)
{
	LPCDSTR p = m_Data.GetData();

	for ( ; ; ) {
		if ( *p != *str )
			return (*p > *str ? (1) : (-1));
		if ( *p == 0 )
			break;
		p++;
		str++;
	}
	return 0;
}
int CStringD::Insert(int iIndex, DCHAR ch)
{
	int len = GetLength();

	if ( iIndex < 0 )
		iIndex = 0;
	else if ( iIndex > len )
		iIndex = len;

	// VARIATION SELECTOR
	// U+180B - U+180D or U+FE00 - U+FE0F or U+E0100 - U+E01EF
	if ( (ch >= 0x180B && ch <= 0x180D) || (ch >= 0xFE00 && ch <= 0xFE0F) || (ch >= 0xE0100 && ch <= 0xE01EF) ) {
		if ( iIndex > 0 )
			m_Vs[iIndex - 1] = ch;
		return len;
	}

	m_Data.InsertAt(iIndex, ch);
	m_Vs.InsertAt(iIndex, (DWORD)0);

	return GetLength();
}
int CStringD::Insert(int iIndex, const CStringD &data)
{
	int n;
	int len = GetLength();

	if ( iIndex > len )
		iIndex = len;

	len = (int)data.m_Data.GetSize() - 1;
	for ( n = 0 ; n < len ; n++, iIndex++ ) {
		m_Data.InsertAt(iIndex, data.m_Data[n]);
		m_Vs.InsertAt(iIndex, data.m_Vs[n]);
	}

	return GetLength();
}
int CStringD::Delete(int iIndex, int nCount)
{
	int len = GetLength();

	if ( iIndex >= len )
		return len;
	if ( (iIndex + nCount) > len )
		nCount = len - iIndex;

	m_Data.RemoveAt(iIndex, nCount);
	m_Vs.RemoveAt(iIndex, nCount);

	return GetLength();
}
void CStringD::Format(LPCWSTR str, ...)
{
	CStringW tmp;
	va_list arg;
	va_start(arg, str);
	tmp.FormatV(str, arg);
	*this = tmp;
	va_end(arg);
}
void CStringD::Nomalize()
{
	int n;
	int len = GetLength();
	DWORD ch;

	for ( n = 0 ; n < (len - 1) ; ) {
		if ( (ch = CTextRam::UCS2toUCS4(CTextRam::UnicodeNomal(CTextRam::UCS4toUCS2(m_Data[n]), CTextRam::UCS4toUCS2(m_Data[n + 1])))) != 0 ) {
			m_Data[n] = ch;
			Delete(n + 1);
			len--;
		} else
			n++;
	}
}

//////////////////////////////////////////////////////////////////////
// CRegExIdx

const CRegExIdx & CRegExIdx::operator = (CRegExIdx &data)
{
	RemoveAll();
	for ( int n = 0 ; n < data.GetSize() ; n++ )
		Add(data[n]);
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CRegExNode

CRegExNode::CRegExNode(void)
{
	m_Left  = NULL;
	m_Right = NULL;

	m_Type  = RGE_NONE;
	m_SChar = 0;
	m_EChar = (-1);
	m_Map   = NULL;
}
CRegExNode::~CRegExNode(void)
{
	if ( m_Map != NULL )
		delete [] m_Map;
}
const CRegExNode & CRegExNode::operator = (CRegExNode &data)
{
	m_Type  = data.m_Type;
	m_SChar = data.m_SChar;
	m_EChar = data.m_EChar;
	if ( m_Map != NULL )
		delete [] m_Map;
	m_Map = NULL;
	if ( data.m_Map != NULL ) {
		m_Map = new DWORD[REG_MAPARRAY];
		memcpy(m_Map, data.m_Map, REG_MAPBYTE);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CRegExQue

CRegExQue::CRegExQue(void)
{
	m_Next = NULL;
}
CRegExQue::~CRegExQue(void)
{
}

//////////////////////////////////////////////////////////////////////
// CRegExArg

CRegExArg::CRegExArg()
{
	m_SPos = 0;
	m_EPos = 0;
	m_Str.Empty();

	m_Del = TRUE;
	m_FrArg.RemoveAll();
	m_BkArg.RemoveAll();
}
const CRegExArg & CRegExArg::operator = (CRegExArg &data)
{
	m_SPos = data.m_SPos;
	m_EPos = data.m_EPos;
	m_Str  = data.m_Str;
	m_Del  = data.m_Del;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CRegExRes

const CRegExRes & CRegExRes::operator = (CRegExRes &data)
{
	m_Data.RemoveAll();
	for ( int n = 0 ; n < data.GetSize() ; n++ ) {
		if ( !data.m_Data[n].m_Del )
			m_Data.Add(data.m_Data[n]);
	}
	m_Idx = data.m_Idx;
	m_Str = data.m_Str;
	return *this;
}
void CRegExRes::Clear()
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ ) {
		m_Data[n].m_SPos = m_Data[n].m_EPos = 0;
		m_Data[n].m_Str.Empty();
		m_Data[n].m_Del = TRUE;
		m_Data[n].m_Done = FALSE;
		m_Data[n].m_FrArg.RemoveAll();
		m_Data[n].m_BkArg.RemoveAll();
	}
}

//////////////////////////////////////////////////////////////////////
// CRegExWork

CRegExWork::CRegExWork()
{
}
CRegExWork::~CRegExWork(void)
{
}

//////////////////////////////////////////////////////////////////////
// CRegEx

CRegEx::CRegEx()
{
	Init(REG_FLAG_REGEX | REG_FLAG_ESCCHAR);
}
CRegEx::CRegEx(int CompleFlag)
{
	Init(CompleFlag);
}
CRegEx::~CRegEx()
{
	CRegExNode *np;
	CRegExQue *qp;
	CRegExWork *wp;
	COMPSTRBUF *cp;

	while ( (np = m_NodeTop) != NULL ) {
		m_NodeTop = np->m_List;
		delete [] np;
	}

	while ( (qp = m_QueTop) != NULL ) {
		m_QueTop = qp->m_List;
		delete [] qp;
	}

	while ( (wp = m_WorkTop) != NULL ) {
		m_WorkTop = wp->m_List;
		delete [] wp;
	}

	while ( (cp = m_CompStrBuf) != NULL ) {
		m_CompStrBuf = cp->m_Next;
		delete cp;
	}
}

void CRegEx::Init(int CompleFlag)
{
	m_NodeTop = m_NodeFree = m_NodeHead = NULL;
	m_QueTop  = m_QueFree  = NULL;
	m_QueHead[0] = m_QueHead[1] = m_QueHead[2] = NULL;
	m_WorkTop = m_WorkFree = m_WorkHead = NULL;
	m_WorkSeq = 0;
	m_CompStrBuf = NULL;
	m_ErrMsg.Empty();
	m_CompleFlag = CompleFlag;
}
void CRegEx::InitChar(LPCTSTR str)
{
	COMPSTRBUF *cp = new COMPSTRBUF;

	cp->m_Next = m_CompStrBuf;
	m_CompStrBuf = cp;

	cp->m_Patan = TstrToUni(str);
	cp->m_SaveCh = 0;
	cp->m_Pos = 0;
}
void CRegEx::EndofChar()
{
	COMPSTRBUF *cp;

	if ( (cp = m_CompStrBuf) != NULL ) {
		m_CompStrBuf = cp->m_Next;
		delete cp;
	}
}
DCHAR CRegEx::GetChar()
{
	if ( m_CompStrBuf == NULL )
		return EOF;

	if ( m_CompStrBuf->m_SaveCh != 0 ) {
		DCHAR ch = m_CompStrBuf->m_SaveCh;
		m_CompStrBuf->m_SaveCh = 0;
		return ch;
	}

	if ( m_CompStrBuf->m_Pos >= m_CompStrBuf->m_Patan.GetLength() )
		return EOF;

	return m_CompStrBuf->m_Patan[m_CompStrBuf->m_Pos++];
}
void CRegEx::UnGetChar(DCHAR ch)
{
	if ( m_CompStrBuf != NULL )
		m_CompStrBuf->m_SaveCh = ch;
}

CRegExNode *CRegEx::AllocNode(enum RegCmdType type)
{
	int n;
	CRegExNode *np;

	if ( m_NodeFree == NULL ) {
		np = new CRegExNode[REG_TABMEM_SIZE];
		np->m_List = m_NodeTop;
		m_NodeTop = np;
		for ( n = 0 ; n < REG_TABMEM_SIZE ; n++ ) {
			np->m_Left = m_NodeFree;
			m_NodeFree = np++;
		}
	}

	np = m_NodeFree;
	m_NodeFree = np->m_Left;
	np->m_Type = type;
	np->m_Left = np->m_Right = NULL;
	return np;
}
void CRegEx::FreeNode(CRegExNode *np)
{
	np->m_Left = m_NodeFree;
	m_NodeFree = np;
}
void CRegEx::RemoveAllNode()
{
	CRegExNode *np;

	while ( (np = m_NodeTop) != NULL ) {
		m_NodeTop = np->m_List;
		delete [] np;
	}

	m_NodeFree = m_NodeHead = NULL;
}

DCHAR CRegEx::CompileEscape(DCHAR ch)
{
	int n;
	DCHAR val = 0;

	if ( ch == EOF )
		return ch;
	else if ( ch == L'u' || ch == L'U' ) {
		for ( n = 0 ; (ch = GetChar()) != EOF && n < 6 ; n++ ) {
			if ( ch >= L'0' && ch <= L'9' )
				val = val * 16 + (ch - L'0');
			else if ( ch >= L'a' && ch <= L'f' )
				val = val * 16 + (ch - L'a' + 10);
			else if ( ch >= L'A' && ch <= L'F' )
				val = val * 16 + (ch - L'A' + 10);
			else
				break;
		}
		UnGetChar(ch);
	} else if ( ch == L'x' || ch == L'X' ) {
		for ( n = 0 ; (ch = GetChar()) != EOF && n < 2 ; n++ ) {
			if ( ch >= L'0' && ch <= L'9' )
				val = val * 16 + (ch - L'0');
			else if ( ch >= L'a' && ch <= L'f' )
				val = val * 16 + (ch - L'a' + 10);
			else if ( ch >= L'A' && ch <= L'F' )
				val = val * 16 + (ch - L'A' + 10);
			else
				break;
		}
		UnGetChar(ch);
	} else if ( ch == L'o' || ch == L'O' ) {
		for ( n = 0 ; (ch = GetChar()) != EOF && n < 8 ; n++ ) {
			if ( ch >= L'0' && ch <= L'7' )
				val = val * 8 + (ch - L'0');
			else
				break;
		}
		UnGetChar(ch);
	} else if ( ch >= L'0' && ch <= L'7' ) {
		val = ch - L'0';
		for ( n = 0 ; (ch = GetChar()) != EOF && n < 2 ; n++ ) {
			if ( ch >= L'0' && ch <= L'7' )
				val = val * 8 + (ch - L'0');
			else
				break;
		}
		UnGetChar(ch);
	} else if ( ch == L'c' || ch == L'C' ) {
		if ( (ch = GetChar()) == L'-' )
			ch = GetChar();

		if ( ch >= L'\x20' && ch <= '\x3F' )
			val = ch - L'\x20';
		else if ( ch >= L'\x40' && ch <= '\x5F' )
			val = ch - L'\x40';
		else if ( ch >= L'\x60' && ch <= '\x7F' )
			val = ch - L'\x60';
		else 
			throw _T("RegEx syntax error\n\\c-? out of range character use \"@ABC...\"");
	} else {
		switch(ch) {
		case L'a': val = L'\x07'; break;
		case L'b': val = L'\x08'; break;
		case L't': val = L'\x09'; break;
		case L'n': val = L'\x0A'; break;
		case L'v': val = L'\x0B'; break;
		case L'f': val = L'\x0C'; break;
		case L'r': val = L'\x0D'; break;
		case L'e': val = L'\x1B'; break;
		default: val = ch; break;
		}
	}
	return val;
}
CRegExNode *CRegEx::CompileExtChar(DCHAR ch, CRegExNode *rp)
{
	DCHAR nx;

	switch(ch) {
	case L'd':
		rp->m_Left = AllocNode(RGE_RANGECHAR);
		rp = rp->m_Left;
		rp->m_SChar = L'0';
		rp->m_EChar = L'9';
		break;
	case L'D':
		rp->m_Left = AllocNode(RGE_NEGCHAR);
		rp = rp->m_Left;
		rp->m_SChar = L'0';
		rp->m_EChar = L'9';
		break;

	case L's':
		rp->m_Left = AllocNode(RGE_RANGECHAR);
		rp = rp->m_Left;
		rp->m_SChar = L'\0';
		rp->m_EChar = L' ';
		break;
	case L'S':
		rp->m_Left = AllocNode(RGE_NEGCHAR);
		rp = rp->m_Left;
		rp->m_SChar = L'\0';
		rp->m_EChar = L' ';
		break;

	case L'h':
		rp = CompleInline(rp, _T("[0-9a-fA-F]"));
		break;
	case L'H':
		rp = CompleInline(rp, _T("[^0-9a-fA-F]"));
		break;

	case L'w':
		rp = CompleInline(rp, _T("[A-Za-z0-9_]"));
		break;
	case L'W':
		rp = CompleInline(rp, _T("[^A-Za-z0-9_]"));
		break;

	case L'y':
		rp = CompleInline(rp, _T("^|\\s+"));
		break;
	case L'Y':
		rp = CompleInline(rp, _T("$|\\S+"));
		break;

	case L'A':
		rp = CompleInline(rp, _T("^|(?<=\\s+)"));
		break;
	case L'Z':
		rp = CompleInline(rp, _T("$|(?=\\s+)"));
		break;
	case L'z':
		rp = CompleInline(rp, _T("(?=\\s+)"));
		break;

	case L'u': case L'U':
	case L'x': case L'X':
	case L'o': case L'O':
		if ( (nx = GetChar()) == L'{' ) {
			for ( ; ; ) {
				nx = CompileEscape(ch);
				rp->m_Left = AllocNode(RGE_RANGECHAR);
				rp = rp->m_Left;
				rp->m_SChar = rp->m_EChar = nx;

				if ( (nx = GetChar()) == EOF )
					throw _T("RegEx syntax error\n{... no end to character list \"}\"");
				else if ( nx == L'}' )
					break;
				else if ( nx > L' ' )
					throw _T("RegEx syntax error\n{0...} character list overflow ?");
			}
		} else {
			UnGetChar(nx);
			return NULL;
		}
		break;

	default:
		return NULL;
	}

	return rp;
}
void CRegEx::CompileReverse(CRegExNode *rp)
{
	if ( rp == NULL )
		return;

	switch(rp->m_Type) {
	case RGE_BRANCHES:
		CompileReverse(rp->m_Right);
		break;

	case RGE_LINETOP:
		rp->m_Type = RGE_NEGTOP;
		break;
	case RGE_NEGTOP:
		rp->m_Type = RGE_LINETOP;
		break;
	case RGE_LINEBTM:
		rp->m_Type = RGE_NEGBTM;
		break;
	case RGE_NEGBTM:
		rp->m_Type = RGE_LINEBTM;
		break;

	case RGE_CHKMAP:
		rp->m_Type = RGE_NEGMAP;
		break;
	case RGE_NEGMAP:
		rp->m_Type = RGE_CHKMAP;
		break;
	case RGE_RANGECHAR:
		rp->m_Type = RGE_NEGCHAR;
		break;
	case RGE_NEGCHAR:
		rp->m_Type = RGE_RANGECHAR;
		break;
	}

	CompileReverse(rp->m_Left);
}
CRegExNode *CRegEx::CompileRange(CRegExNode *rp)
{
	DCHAR ch;
	enum RegCmdType type = RGE_RANGECHAR;
	int count = 0;
	CRegExNode tmp, *tp, *np, *ap, *bp = NULL;
	DCHAR nch = 0xFFFF;
	DCHAR mch = 0x0000;

	if ( (ch = GetChar()) == '^' )
		type = RGE_NEGCHAR;
	else
		UnGetChar(ch);

	tp = &tmp;
	tmp.m_Left  = NULL;
	tmp.m_Right = NULL;

	while ( (ch = GetChar()) != EOF && ch != L']' ) {
		if ( ch == L'\\' ) {
			if ( (ch = GetChar()) == EOF )
				break;
			// -(a)-[n]-+
			//   +--[n]-(b)-
			ap = AllocNode(RGE_BRANCHES);
			ap->m_Right = tmp.m_Left;
			if ( (np = CompileExtChar(ch, ap)) != NULL ) {
				if ( bp == NULL )
					bp = AllocNode(RGE_BRANCHES);
				np->m_Left = bp;
				tmp.m_Left = ap;
				if ( type == RGE_NEGCHAR )
					CompileReverse(ap->m_Left);
				continue;
			} else {
				FreeNode(ap);
				if ( (ch = CompileEscape(ch)) == EOF )
					break;
			}
		} else if ( ch == L'-' ) {
			UnGetChar(ch);
			ch = 0;
		}

		count++;
		tp->m_Right = AllocNode(type);
		tp = tp->m_Right;
		tp->m_SChar = tp->m_EChar = ch;

		if ( ch < nch )
			nch = ch;
		if ( ch > mch )
			mch = ch;

		if ( (ch = GetChar()) == L'-' ) {
			if ( (ch = GetChar()) != EOF && ch != L']' ) {
				if ( ch == L'\\' ) {
					if ( (ch = GetChar()) == EOF )
						break;
					if ( (np = CompileExtChar(ch, tp)) != NULL )
						throw _T("RegEx syntax error\n[-\\? only one character");
					else if ( (ch = CompileEscape(ch)) == EOF )
						break;
				}
			} else {
				UnGetChar(ch);
				ch = UNICODE_MAX;
			}

			if ( tp->m_SChar > ch )
				tp->m_SChar = ch;
			else
				tp->m_EChar = ch;

			if ( ch < nch )
				nch = ch;
			if ( ch > mch )
				mch = ch;

		} else
			UnGetChar(ch);
	}

	if ( ch != L']' )
		throw _T("RegEx syntax error\n[... no end to character list \"]\"");

	if ( tmp.m_Right != NULL && (mch - nch) < REG_MAPSIZE && count > 2 ) {
		np = AllocNode(type == RGE_RANGECHAR ? RGE_CHKMAP : RGE_NEGMAP);
		np->m_SChar = nch;
		np->m_EChar = mch;
		np->m_Map = new DWORD[REG_MAPARRAY];
		memset(np->m_Map, 0, REG_MAPBYTE);

		for ( tp = tmp.m_Right ; tp != NULL ; tp = tp->m_Right ) {
			for ( ch = tp->m_SChar ; ch <= tp->m_EChar ; ch++ )
				np->m_Map[(ch - nch) / REG_MAPBITS] |= (1 << ((ch - nch) % REG_MAPBITS));
		}

		while ( (tp = tmp.m_Right) != NULL ) {
			tmp.m_Right = tp->m_Right;
			FreeNode(tp);
		}

		tmp.m_Right = np;
	}

	if ( tmp.m_Left != NULL ) {
		// -(a)-[t]-+
		//   +--[n]-(b)-
		if ( tmp.m_Right != NULL ) {
			tmp.m_Left->m_Left = tmp.m_Right;
			tmp.m_Right->m_Left = bp;
		}
		rp->m_Left = tmp.m_Left;
		rp = bp;
	} else {
		rp->m_Left = tmp.m_Right;
		rp = rp->m_Left;
	}

	return rp;
}
CRegExNode *CRegEx::CompileSub(DCHAR endc)
{
	int n;
	int min, max;
	DCHAR ch, nx;
	CRegExNode tmp;
	CRegExNode *lp, *rp, *tp, *np;
	int noMore;

	rp = &tmp;
	lp = NULL;
	rp->m_Left = NULL;

	while ( (ch = GetChar()) != EOF && ch != endc ) {
		switch(ch) {
		case L'[':
			if ( (tp = CompileRange(rp)) == NULL )
				break;
			lp = rp;
			rp = tp;
			break;

		case L'\\':
			if ( (ch = GetChar()) == EOF )
				break;
			if ( (tp = CompileExtChar(ch, rp)) != NULL ) {
				lp = rp;
				rp = tp;
			} else {
				ch = CompileEscape(ch);
				goto CHARMATCH;
			}
			break;

		case L'.':
		    lp = rp;
			rp->m_Left = AllocNode(RGE_NEGCHAR);
			rp = rp->m_Left;
			rp->m_SChar = 0x0A;
			rp->m_EChar = 0x0A;
			rp->m_Right = AllocNode(RGE_NEGCHAR);
			rp->m_Right->m_SChar = 0x0D;
			rp->m_Right->m_EChar = 0x0D;
			break;

		case L'^':
			// -(s)-(^)-(e)-
		    lp = NULL;
			rp->m_Left = AllocNode(RGE_GROUPSTART);
			rp->m_Left->m_EChar = m_Arg++;
			rp->m_Left->m_Left = AllocNode(RGE_LINETOP);
			rp->m_Left->m_Left->m_Left = AllocNode(RGE_GROUPEND);
			rp->m_Left->m_Left->m_Left->m_EChar = REG_BLOCK_LEFT;
			rp->m_Left->m_Left->m_Left->m_SChar = m_ArgStack.GetHead();
			rp->m_Left->m_Right = rp->m_Left->m_Left->m_Left;
			rp->m_Left->m_Left->m_Left->m_Right = rp->m_Left;
			rp = rp->m_Left->m_Left->m_Left;
			break;
		case L'$':
			// -(s)-($)-(e)-
		    lp = NULL;
			rp->m_Left = AllocNode(RGE_GROUPSTART);
			rp->m_Left->m_EChar = m_Arg++;
			rp->m_Left->m_Left = AllocNode(RGE_LINEBTM);
			rp->m_Left->m_Left->m_Left = AllocNode(RGE_GROUPEND);
			rp->m_Left->m_Left->m_Left->m_EChar = REG_BLOCK_RIGHT;
			rp->m_Left->m_Left->m_Left->m_SChar = m_ArgStack.GetHead();
			rp->m_Left->m_Right = rp->m_Left->m_Left->m_Left;
			rp->m_Left->m_Left->m_Left->m_Right = rp->m_Left;
			rp = rp->m_Left->m_Left->m_Left;
			break;

		case L'*':
			if ( lp == NULL )
				goto CHARMATCH;
			min = 0;
			max = REG_MAXREP;
			goto REPENT;
		case L'+':
			if ( lp == NULL )
				goto CHARMATCH;
			min = 1;
			max = REG_MAXREP;
			goto REPENT;
		case L'?':
			if ( lp == NULL )
				goto CHARMATCH;
			min = 0;
			max = 1;
			goto REPENT;

		case L'{':
			if ( lp == NULL )
				goto CHARMATCH;
			min = 0;
			while ( (ch = GetChar()) >= L'0' && ch <= L'9' )
				min = min * 10 + (ch - L'0');
			max = min;
			if ( ch == L',' ) {
				if ( (ch = GetChar()) >= L'0' && ch <= L'9' ) {
					max = ch - L'0';
					while ( (ch = GetChar()) >= L'0' && ch <= L'9' )
						max = max * 10 + (ch - L'0');
				} else
					max = REG_MAXREP;
			}
			if ( ch != L'}' )
				throw _T("RegEx syntax error\n{... no end to character list \"}\"");

		REPENT:
			nx = GetChar();
			if ( nx == L'?' )
				noMore = 1;
			else if ( nx == L'+' )
				noMore = 2;
			else {
				UnGetChar(nx);
				noMore = 0;
			}

			if ( min > REG_MAXREP )
				min = REG_MAXREP;
			if ( max > REG_MAXREP )
				max = REG_MAXREP;

			if ( min > max ) {
				n = min; min = max; max = n;
				noMore = 2;
			}

			tp = AllocNode(RGE_LOOPSTART);
			np = AllocNode(RGE_BRANCHES);
			if ( noMore == 1 ) {
				//   +-------------->+
				// -(n)-(?)-[a]-(x)-(b)-
				//       +<------+
				tp->m_Left = AllocNode(RGE_LOOPSHORT);
				tp->m_Left->m_EChar = m_ArgStack.GetHead();
				tp->m_Left->m_Left = lp->m_Left;
				lp->m_Left = tp;
			} else if ( noMore == 2 ) {
				//   +------------------>+
				// -(n)-(+)-[a]-(+)-(x)-(b)-
				//       +<----------+
				tp->m_Left = AllocNode(RGE_GREEDSTART);
				tp->m_Left->m_Left = lp->m_Left;
				lp->m_Left = tp;
				rp->m_Left = AllocNode(RGE_GREEDEND);
				rp = rp->m_Left;
			} else {
				//   +---------->+
				// -(n)-[a]-(x)-(b)-
				//       +<--+
				tp->m_Left = lp->m_Left;
				lp->m_Left = tp;
			}
			tp->m_SChar = m_Count++;
			tp->m_EChar = min;
			tp->m_Right = np;
			rp->m_Left = AllocNode(RGE_LOOPEND);
			rp = rp->m_Left;
			rp->m_SChar = tp->m_SChar;
			rp->m_EChar = (min << 16) | max;
			rp->m_Left = np;
			rp->m_Right = tp->m_Left;
			rp = np;
			lp = NULL;
			break;

		case L'(':
			n = 0000;
			if ( (ch = GetChar()) == L'?' ) {
				if ( (ch = GetChar()) == L'i' ) {
					if ( (ch = GetChar()) == L')' ) {
						lp = NULL;
						rp->m_Left = AllocNode(RGE_NOCASE);
						rp = rp->m_Left;
						break;
					}
					n |= 0100;
				} else if ( ch == L'-' ) {
					if ( (ch = GetChar()) != L'i' )
						throw _T("RegEx syntax error\n((?-... this suppert character \"i\"");
					if ( (ch = GetChar()) == L')' ) {
						lp = NULL;
						rp->m_Left = AllocNode(RGE_CASE);
						rp = rp->m_Left;
						break;
					}
					n |= 0200;
				}

				if ( ch == L':' ) {
					n |= 020;
				} else if ( ch == L'>' ) {
					n |= 040;
				} else {
					if ( ch == L'<' ) {
						n |= 010;
						ch = GetChar();
					}
					if ( ch == L'=' )
						n |= 001;
					else if ( ch == L'!' )
						n |= 002;
					else {
						n &= ~010;
						UnGetChar(ch);
					}
				}
			} else if ( ch == L'#' ) {
				while ( (ch = GetChar()) != L')' ) {
					if ( ch == EOF )
						throw _T("RegEx syntax error\n(#... no end to character \")\"");
				}
				break;
			} else
				UnGetChar(ch);
			//   +<----->+
			// -(s)-[a]-(e)-
			lp = rp;
			rp->m_Left = AllocNode(RGE_GROUPSTART);
			rp->m_Left->m_EChar = m_Arg++;
			m_ArgStack.AddHead(rp->m_Left->m_EChar);
			switch(n & 0700) {
			case 0100:	// (?i...)
				rp->m_Left->m_Left = AllocNode(RGE_NOCASE);
				rp = rp->m_Left;
				break;
			case 0200:	// (?-i...)
				rp->m_Left->m_Left = AllocNode(RGE_CASE);
				rp = rp->m_Left;
				break;
			}
			rp->m_Left->m_Left = CompileSub(')');
			if ( (n & 002) != 0 )
				CompileReverse(rp->m_Left->m_Left);
		    while ( rp->m_Left != NULL )
				rp = rp->m_Left;
			m_ArgStack.RemoveHead();
			rp->m_Left = AllocNode(RGE_GROUPEND);
			switch(n & 0077) {
			case 001:	// ...(?=...)
			case 002:	// ...(?!...)
				rp->m_Left->m_EChar = REG_BLOCK_RIGHT;
				break;
			case 011:	// (?<=...)...
			case 012:	// (?<!...)...
				rp->m_Left->m_EChar = REG_BLOCK_LEFT;
				break;
			case 020:	// (?:...)
				rp->m_Left->m_EChar = REG_BLOCK_NONE;
				break;
			case 040:	// (?>...)
				//   +<------------->+
				// -(s)-(+)-[a]-(+)-(e)-
				rp->m_Left->m_EChar = REG_BLOCK_STD;
				tp = AllocNode(RGE_GREEDSTART);
				np = AllocNode(RGE_GREEDEND);
				tp->m_Left = lp->m_Left->m_Left;
				lp->m_Left->m_Left = tp;
				np->m_Left = rp->m_Left;
				rp->m_Left = np;
				rp = np;
				break;
			default:
				rp->m_Left->m_EChar = REG_BLOCK_STD;
				break;
			}
			rp = rp->m_Left;
			rp->m_SChar = m_ArgStack.GetHead();
			rp->m_Right = lp->m_Left;
			lp->m_Left->m_Right = rp;
			break;

		case L'|':
			// -(b)-[c]-(b)-
			//   +- [a] -+
			tp = AllocNode(RGE_BRANCHES);
			tp->m_Right = tmp.m_Left;
			tmp.m_Left = tp;
			tp->m_Left = CompileSub(EOF);
			while ( tp->m_Left != NULL )
				tp = tp->m_Left;
			tp->m_Left = rp->m_Left = AllocNode(RGE_BRANCHES);
			rp = rp->m_Left;
		    break;

		CHARMATCH:
		default:
		    lp = rp;
			rp->m_Left = AllocNode(RGE_RANGECHAR);
			rp = rp->m_Left;
		    rp->m_SChar = rp->m_EChar = ch;
			break;
		}
	}

	if ( ch != endc && endc != EOF )
		throw _T("RegEx syntax error\n(... no end to character list \")\"");

	return tmp.m_Left;
}
CRegExNode *CRegEx::CompleInline(CRegExNode *rp, LPCTSTR str)
{
	InitChar(str);

	rp->m_Left = CompileSub(EOF);

	while ( rp->m_Left != NULL )
		rp = rp->m_Left;

	EndofChar();

	return rp;
}
BOOL CRegEx::Compile(LPCTSTR str)
{
	CRegExNode *np;

	if ( m_CompleFlag != (REG_FLAG_REGEX | REG_FLAG_ESCCHAR) )
		str = SimpleRegEx(str, m_CompleFlag);

	m_Arg = 1;
	m_ArgStack.RemoveAll();
	m_Count = 0;
	RemoveAllNode();
	InitChar(str);

	try {
		//   +<----->+
		// -(s)-[a]-(e)-
		m_NodeHead = AllocNode(RGE_GROUPSTART);
		m_NodeHead->m_EChar = 0;
		m_NodeHead->m_SChar = 0;
		m_ArgStack.AddHead(0);
		if ( (m_NodeHead->m_Left = CompileSub(EOF)) == NULL )
			throw _T("RegEx Empty search string");
		for ( np = m_NodeHead ; np->m_Left != NULL ; )
			np = np->m_Left;
		np->m_Left = AllocNode(RGE_GROUPEND);
		np->m_Left->m_EChar = 0;
		m_ArgStack.RemoveHead();
		m_NodeHead->m_Right = np->m_Left;
		np->m_Left->m_Right = m_NodeHead;

	} catch(LPCTSTR msg) {
		m_ErrMsg = msg;
		RemoveAllNode();
	}

	EndofChar();

	return (m_NodeHead != NULL ? TRUE : FALSE);
}
BOOL CRegEx::IsSimple()
{
	CRegExNode *np = m_NodeHead;

	if ( np == NULL || np->m_Type != RGE_GROUPSTART )
		return FALSE;

	for ( np = np->m_Left ; np != NULL && np->m_Type == RGE_RANGECHAR ; np = np->m_Left ) {
		if ( np->m_SChar != np->m_EChar || np->m_Right != NULL )
			return FALSE;
	}

	if ( np == NULL || np->m_Type != RGE_GROUPEND || np->m_Left != NULL )
		return FALSE;

	return TRUE;
}
LPCTSTR CRegEx::SimpleRegEx(LPCTSTR str, int flag)
{
	LPCTSTR p;
	static CString tmp;

	tmp.Empty();

	switch(flag & REG_FLAG_MODEMASK) {
	case REG_FLAG_SIMPLE:
		if ( (flag & REG_FLAG_NOCASE) != 0 )
			tmp += _T("(?i)");
		for ( p = str ; *p != _T('\0') ; ) {
			if ( *p == _T('\\') ) {
				if ( (flag & REG_FLAG_ESCCHAR) == 0 )
					tmp += *p;
				tmp += *(p++);
				if ( *p != _T('\0') && (flag & REG_FLAG_ESCCHAR) != 0 )
					tmp += *(p++);
			} else if ( *p == _T('%') && (flag & REG_FLAG_ESCPAR) != 0 ) {
				if ( p[1] == _T('%') )
					tmp += *(p++);
				else
					tmp += _T('\\');
				p++;
			} else if ( _tcschr(_T("^$.+*?[({|"), *p) != NULL ) {
				tmp += _T('\\');
				tmp += *(p++);
			} else if ( *p < _T(' ') ) {
				tmp += _T('\\');
				tmp += (TCHAR)(_T('0') + ((*p >> 6) & 7));
				tmp += (TCHAR)(_T('0') + ((*p >> 3) & 7));
				tmp += (TCHAR)(_T('0') + (*(p++) & 7));
			} else
				tmp += *(p++);
		}
		break;

	case REG_FLAG_WILDCARD:
		if ( (flag & REG_FLAG_NOCASE) != 0 )
			tmp += _T("(?i)");
		for ( p = str ; *p != _T('\0') ; ) {
			if ( *p == _T('\\') ) {
				if ( (flag & REG_FLAG_ESCCHAR) == 0 )
					tmp += *p;
				tmp += *(p++);
				if ( *p != _T('\0') && (flag & REG_FLAG_ESCCHAR) != 0 )
					tmp += *(p++);
			} else if ( *p == _T('%') && (flag & REG_FLAG_ESCPAR) != 0 ) {
				if ( p[1] == _T('%') )
					tmp += *(p++);
				else
					tmp += _T('\\');
				p++;
			} else if ( *p == _T('*') ) {
				tmp += _T(".");
				tmp += *(p++);
				tmp += _T("?");
			} else if ( *p == _T('?') ) {
				tmp += _T(".");
				p++;
			} else if ( _tcschr(_T("^$.+[({|"), *p) != NULL ) {
				tmp += _T('\\');
				tmp += *(p++);
			} else if ( *p < _T(' ') ) {
				tmp += _T('\\');
				tmp += (TCHAR)(_T('0') + ((*p >> 6) & 7));
				tmp += (TCHAR)(_T('0') + ((*p >> 3) & 7));
				tmp += (TCHAR)(_T('0') + (*(p++) & 7));
			} else
				tmp += *(p++);
		}
		break;

	default:	// REG_FLAG_REGEX:
		if ( (flag & REG_FLAG_NOCASE) != 0 )
			tmp += _T("(?i)");
		for ( p = str ; *p != _T('\0') ; ) {
			if ( *p == _T('\\') ) {
				if ( (flag & REG_FLAG_ESCCHAR) == 0 )
					tmp += *p;
				tmp += *(p++);
				if ( *p != _T('\0') && (flag & REG_FLAG_ESCCHAR) != 0 )
					tmp += *(p++);
			} else if ( *p == _T('%') && (flag & REG_FLAG_ESCPAR) != 0 ) {
				if ( p[1] == _T('%') )
					tmp += *(p++);
				else
					tmp += _T('\\');
				p++;
			} else if ( *p < _T(' ') ) {
				tmp += _T('\\');
				tmp += (TCHAR)(_T('0') + ((*p >> 6) & 7));
				tmp += (TCHAR)(_T('0') + ((*p >> 3) & 7));
				tmp += (TCHAR)(_T('0') + (*(p++) & 7));
			} else
				tmp += *(p++);
		}
		break;
	}

	return tmp;
}

void CRegEx::AddQue(int sw, CRegExNode *np, CRegExWork *wp, int Flag)
{
	CRegExQue *qp;

	if ( np == NULL )
		return;

	for ( qp = m_QueHead[sw] ;  qp != NULL ; qp = qp->m_Next ) {
		if ( qp->m_Node == np ) {
			// 同じワークの同じノードの場合追加しない（リピート処理で起こりえる）
			if ( qp->m_Work->m_Seq == wp->m_Seq )
				return;
			// 条件付きで別ワークの同じノードを削除
			else {
				int qc = (qp->m_Flag & REG_NQFLAG_LOOP) == 0 ? 0 : qp->m_Work->m_Loop;
				int wc = (Flag & REG_NQFLAG_LOOP) == 0 ? 0 : wp->m_Loop;

				if ( qc == wc ) {
					if ( qp->m_Work->m_Seq > wp->m_Seq )
						qp->m_Work = wp;
					return;
				}
			}
		}
	}

	if ( m_QueFree == NULL ) {
		qp = new CRegExQue[REG_TABMEM_SIZE];
		qp->m_List = m_QueTop;
		m_QueTop = qp;
		for ( int n = 0 ; n < REG_TABMEM_SIZE ; n++ ) {
			qp->m_Next = m_QueFree;
			m_QueFree = qp++;
		}
	}
	qp = m_QueFree;
	m_QueFree = qp->m_Next;

	qp->m_Node = np;
	qp->m_Work = wp;
	qp->m_Flag = Flag;

	qp->m_Next = m_QueHead[sw];
	m_QueHead[sw] = qp;
}
CRegExQue *CRegEx::HeadQue(int sw)
{
	CRegExQue *qp;

	if ( (qp = m_QueHead[sw]) != NULL ) {
		m_QueHead[sw] = qp->m_Next;

		qp->m_Next = m_QueFree;
		m_QueFree = qp;
	}

	return qp;
}

BOOL CRegEx::MatchStr(LPCTSTR str, CRegExRes *res)
{
	int n, len;
	CStringD tmp;

	tmp = TstrToUni(str);
	tmp.Insert(0, REGSPCHAR_LINETOP);
	tmp += REGSPCHAR_LINEEND;

	len = tmp.GetLength();

	MatchCharInit();

	for ( n = 0 ; n < len ; n++ ) {
		if ( MatchChar(tmp[n], n, res) && (res->m_Status == REG_MATCH || res->m_Status == REG_MATCHOVER) )
			return TRUE;
	}

	return FALSE;
}
void CRegEx::ConvertRes(LPCWSTR pat, CStringW &buf, CRegExRes *res)
{
	int n;

	buf.Empty();
	while ( *pat != L'\0' ) {
		if ( pat[0] == L'%' && pat[1] == L'%' ) {
			buf += *(pat++);
			buf += *(pat++);
		} else if ( pat[0] == L'%' && (pat[1] >= L'0' && pat[1] <= L'9') ) {
			pat++;
			for ( n = 0 ; *pat >= L'0' && *pat <= L'9' ; pat++ )
					n = n * 10 + (*pat - L'0');
			if ( n < res->GetSize() )
				buf += res->GetAt(n).m_Str;
		} else
			buf += *(pat++);
	}
}
BOOL CRegEx::ConvertStr(LPCTSTR str, LPCTSTR pat, CString &buf)
{
	int n, a, len;
	CStringD tmp, wrk;
	CStringW wstr;
	CRegExRes res;
	BOOL rt = FALSE;

	tmp = TstrToUni(str);
	tmp.Insert(0, REGSPCHAR_LINETOP);
	tmp += REGSPCHAR_LINEEND;

	len = tmp.GetLength();

	MatchCharInit();

	for ( n = a = 0 ; n < len ; n++ ) {
		if ( MatchChar(tmp[n], n, &res) && (res.m_Status == REG_MATCH || res.m_Status == REG_MATCHOVER) ) {
			while ( a < (int)res.m_Idx[res[0].m_SPos] )
				wrk += tmp[a++];
			ConvertRes(TstrToUni(pat), wstr, &res);
			wrk += wstr;
			if ( res[0].m_EPos < res.m_Idx.GetSize() )
				a = res.m_Idx[res[0].m_EPos];
			else
				a = n + 1;
			rt = TRUE;
		}
	}

	while ( a < len )
		wrk += tmp[a++];

	buf = wrk;
	return rt;
}
int CRegEx::SplitStr(LPCTSTR str, CStringArray &data)
{
	int n, a, len;
	CStringD tmp;
	CRegExRes res;

	tmp = TstrToUni(str);
	tmp.Insert(0, REGSPCHAR_LINETOP);
	tmp += REGSPCHAR_LINEEND;

	len = tmp.GetLength();

	MatchCharInit();
	data.RemoveAll();

	for ( n = a = 0 ; n < len ; ) {
		if ( MatchChar(tmp[n], n, &res) && (res.m_Status == REG_MATCH || res.m_Status == REG_MATCHOVER) ) {
			data.Add(UniToTstr(tmp.Mid(a, res.m_Idx[res[0].m_SPos] - a)));
			if ( res[0].m_EPos < res.m_Idx.GetSize() )
				n = a = res.m_Idx[res[0].m_EPos];
			else
				n = a = n + 1;
		} else
			n++;
	}

	if ( a < (len - 1) )
		data.Add(UniToTstr(tmp.Mid(a, len - 1 - a)));

	return (int)data.GetSize();
}

void CRegEx::MatchCharInit()
{
	CRegExWork *wp;

	while ( HeadQue(0) != NULL );
	while ( HeadQue(1) != NULL );
	while ( HeadQue(2) != NULL );

	m_WorkSeq = 0;
	while ( (wp = m_WorkHead) != NULL ) {
		m_WorkHead = wp->m_Next;
		wp->m_Next = m_WorkFree;
		m_WorkFree = wp;
	}
}
DCHAR CRegEx::RevLowerUpeer(DCHAR ch)
{
	if ( IsWchar(ch) && iswascii((WCHAR)ch) ) {
		if ( iswlower((WCHAR)ch) )
			return (DCHAR)towupper((WCHAR)ch);
		else if ( iswupper((WCHAR)ch) )
			return (DCHAR)towlower((WCHAR)ch);
	}
	return 0;
}
BOOL CRegEx::MatchChar(DCHAR ch, int idx, CRegExRes *res)
{
	int i, Pos, Que;
	int min, max;
	CRegExNode *np, *ip;
	CRegExQue *qp, *qtp;
	CRegExWork *wp, *wtp;
	CRegExWork *whp = NULL;
	CRegExWork *wop = NULL;
	int Flag;
	DCHAR sch;

	if ( m_NodeHead == NULL ) {
		if ( res != NULL ) {
			res->Clear();
			res->m_Status = REG_MATCH;
		}
		return TRUE;
	} else if ( ch == 0 || (ch >= 0x180B && ch <= 0x180D) || (ch >= 0xFE00 && ch <= 0xFE0F) || (ch >= 0xE0100 && ch <= 0xE01EF) )
		return FALSE;

	if ( m_WorkFree == NULL ) {
		wp = new CRegExWork[REG_TABMEM_SIZE];
		wp->m_List = m_WorkTop;
		m_WorkTop = wp;
		for ( i = 0 ; i < REG_TABMEM_SIZE ; i++ ) {
			wp->m_Next = m_WorkFree;
			m_WorkFree = wp++;
		}
	}

	wp = m_WorkFree;
	m_WorkFree = wp->m_Next;

	wp->m_Next = m_WorkHead;
	m_WorkHead = wp;

	wp->m_Work.SetSize(m_Arg);
	wp->m_Done.SetSize(m_Arg);
	wp->m_Counter.SetSize(m_Count);
	wp->m_DupDone = FALSE;
	wp->m_Pos = 0;
	wp->m_Str.Empty();
	wp->m_Seq = m_WorkSeq++;
	wp->m_Idx.RemoveAll();
	AddQue(0, m_NodeHead, wp, 0);

	m_WorkCount = 0;
	for ( wp = m_WorkHead ; wp != NULL ; wp = wp->m_Next ) {
		wp->m_Die = TRUE;
		wp->m_Pos = wp->m_Str.GetLength();
		wp->m_Str += ch;
		wp->m_Idx.Add(idx);
		m_WorkCount++;
	}

	//TRACE("%d\n", m_WorkCount);

	for ( Que = 0 ; Que < 2 ; Que++ ) {
		while ( (qp = HeadQue(Que)) != NULL ) {
			np = qp->m_Node;
			wp = qp->m_Work;
			Flag = qp->m_Flag;

			//TRACE("%d(%d:%d) ", wp->m_Seq, np->m_Type, Que);

			switch(np->m_Type) {
			case RGE_GROUPSTART:
				i = np->m_EChar;

				Pos = wp->m_Pos + Que;
				if ( wp->m_Work[i].m_SPos >= wp->m_Work[i].m_EPos || wp->m_Work[i].m_EPos != Pos )
					wp->m_Work[i].m_SPos = wp->m_Work[i].m_EPos = Pos;

				wp->m_Work[i].m_FrArg.RemoveAll();
				wp->m_Work[i].m_BkArg.RemoveAll();
				wp->m_Work[i].m_Done = FALSE;
				wp->m_Work[i].m_Flag = Flag;

				AddQue(Que, np->m_Left, wp, Flag);
				break;
			case RGE_GROUPEND:
				i = np->m_Right->m_EChar;
				wp->m_Work[i].m_EPos = wp->m_Pos + Que;
				wp->m_Work[i].m_Done = TRUE;
				wp->m_Work[i].m_Del = (np->m_EChar == REG_BLOCK_STD ? FALSE : TRUE);
				Flag = wp->m_Work[i].m_Flag;

				while ( !wp->m_Work[i].m_FrArg.IsEmpty() ) {
					Pos = wp->m_Work[i].m_FrArg.RemoveHead();
					if ( wp->m_Work[i].m_SPos == wp->m_Work[Pos].m_SPos )
						wp->m_Work[i].m_SPos = wp->m_Work[Pos].m_SPos = wp->m_Work[Pos].m_EPos;
				}
				while ( !wp->m_Work[i].m_BkArg.IsEmpty() ) {
					Pos = wp->m_Work[i].m_BkArg.RemoveHead();
					if ( wp->m_Work[i].m_EPos == wp->m_Work[Pos].m_EPos )
						wp->m_Work[i].m_EPos = wp->m_Work[Pos].m_EPos = wp->m_Work[Pos].m_SPos;
				}

				wp->m_Work[i].m_Str = wp->m_Str.Mid(wp->m_Work[i].m_SPos, wp->m_Work[i].m_EPos - wp->m_Work[i].m_SPos);

				switch(np->m_EChar) {
				case REG_BLOCK_RIGHT:		// ...(?=...) or ...(?!...)
					wp->m_Work[np->m_SChar].m_BkArg.AddHead(i);
					break;
				case REG_BLOCK_LEFT:		// (?<=...)... or (?<!...)...
					wp->m_Work[np->m_SChar].m_FrArg.AddTail(i);
					break;
				}

				if ( i == 0 ) {
					wp->m_Done = wp->m_Work;
					if ( wp->m_Work[i].m_SPos < wp->m_Work[i].m_EPos ) {
						// 先のワークを優先して後ワークを削除
						if ( whp == NULL )
							whp = wp;
						else if ( whp->m_Seq > wp->m_Seq ) {
							whp->m_DupDone = TRUE;
							whp = wp;
						} else if ( whp->m_Seq < wp->m_Seq )
							wp->m_DupDone = TRUE;
					}
				}

				AddQue(Que, np->m_Left, wp, Flag);
				break;

			case RGE_LOOPSTART:
				wp->m_Counter[np->m_SChar] = 0;
				AddQue(Que, np->m_Left, wp, Flag);
				if ( np->m_EChar == 0 )
					AddQue(Que, np->m_Right, wp, Flag);
				break;
			case RGE_LOOPEND:
				i = ++(wp->m_Counter[np->m_SChar]);
				min = np->m_EChar >> 16;
				max = np->m_EChar & 0xFFFF;
				Flag &= ~REG_NQFLAG_LOOP;
				if ( i >= min )
					AddQue(Que, np->m_Left, wp, Flag);
				if ( i < max ) {
					wp->m_Loop = i;
					AddQue(Que, np->m_Right, wp, Flag | (max < REG_MAXREP ? REG_NQFLAG_LOOP : 0));
				}
				break;
			case RGE_LOOPSHORT:
				i = np->m_EChar;
				if ( wp->m_Work[i].m_Done )
					break;
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else
					AddQue(Que, np->m_Left, wp, Flag);
				break;

			case RGE_GREEDSTART:
				Flag |= REG_NQFLAG_GREED;
				AddQue(Que, np->m_Left, wp, Flag);
				break;
			case RGE_GREEDEND:
				Flag &= ~REG_NQFLAG_GREED;
				AddQue(Que, np->m_Left, wp, Flag);
				break;

			case RGE_BRANCHES:
				AddQue(Que, np->m_Left,  wp, Flag);
				AddQue(Que, np->m_Right, wp, Flag);
				break;

			case RGE_LINETOP:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else if ( ch == L'\n' || ch == REGSPCHAR_LINETOP )
					AddQue(1, np->m_Left, wp, Flag);
				break;
			case RGE_NEGTOP:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else if ( ch != L'\n' && ch != REGSPCHAR_LINETOP )
					AddQue(1, np->m_Left, wp, Flag);
				break;

			case RGE_LINEBTM:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else if ( ch == L'\r' || ch == REGSPCHAR_LINEEND )
					AddQue(1, np->m_Left, wp, Flag);
				break;
			case RGE_NEGBTM:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else if ( ch != L'\r' && ch != REGSPCHAR_LINEEND )
					AddQue(1, np->m_Left, wp, Flag);
				break;

			case RGE_CHKMAP:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else if ( ch >= np->m_SChar && ch <= np->m_EChar && (np->m_Map[(ch - np->m_SChar) / REG_MAPBITS] & (1 << ((ch - np->m_SChar) % REG_MAPBITS))) != 0 )
					AddQue(1, np->m_Left, wp, Flag);
				else if ( (Flag & REG_NQFLAG_NOCASE) != 0 && (sch = RevLowerUpeer(ch)) != 0 ) {
					if ( sch >= np->m_SChar && sch <= np->m_EChar && (np->m_Map[(sch - np->m_SChar) / REG_MAPBITS] & (1 << ((sch - np->m_SChar) % REG_MAPBITS))) != 0 )
						AddQue(1, np->m_Left, wp, Flag);
				}
				break;
			case RGE_NEGMAP:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else if ( ch < np->m_SChar || ch > np->m_EChar || (np->m_Map[(ch - np->m_SChar) / REG_MAPBITS] & (1 << ((ch - np->m_SChar) % REG_MAPBITS))) == 0 )
					AddQue(1, np->m_Left, wp, Flag);
				else if ( (Flag & REG_NQFLAG_NOCASE) != 0 && (sch = RevLowerUpeer(ch)) != 0 ) {
					if ( sch < np->m_SChar || sch > np->m_EChar || (np->m_Map[(sch - np->m_SChar) / REG_MAPBITS] & (1 << ((sch - np->m_SChar) % REG_MAPBITS))) == 0 )
						AddQue(1, np->m_Left, wp, Flag);
				}
				break;

			case RGE_RANGECHAR:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else {
					for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
						if ( ch >= ip->m_SChar && ch <= ip->m_EChar )
							break;
					}
					if ( ip != NULL )
						AddQue(1, np->m_Left, wp, Flag);
					else if ( (Flag & REG_NQFLAG_NOCASE) != 0 && (sch = RevLowerUpeer(ch)) != 0 ) {
						for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
							if ( sch >= ip->m_SChar && sch <= ip->m_EChar )
								break;
						}
						if ( ip != NULL )
							AddQue(1, np->m_Left, wp, Flag);
					}
				}
				break;
			case RGE_NEGCHAR:
				if ( Que > 0 )
					AddQue(2, np, wp, Flag);
				else {
					for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
						if ( ch >= ip->m_SChar && ch <= ip->m_EChar )
							break;
					}
					if ( ip == NULL )
						AddQue(1, np->m_Left, wp, Flag);
					else if ( (Flag & REG_NQFLAG_NOCASE) != 0 && (sch = RevLowerUpeer(ch)) != 0 ) {
						for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
							if ( sch >= ip->m_SChar && sch <= ip->m_EChar )
								break;
						}
						if ( ip == NULL )
							AddQue(1, np->m_Left, wp, Flag);
					}
				}
				break;

			case RGE_NOCASE:
				Flag |= REG_NQFLAG_NOCASE;
				AddQue(Que, np->m_Left, wp, Flag);
				break;
			case RGE_CASE:
				Flag &= ~REG_NQFLAG_NOCASE;
				AddQue(Que, np->m_Left, wp, Flag);
				break;
			}
		}

		//TRACE("\n");
	}

	// ノードキューのワークをチェックして再構築
	qtp = NULL;
	while ( (qp = m_QueHead[2]) != NULL ) {
		m_QueHead[2] = qp->m_Next;
		// ワークの文字数と重なった終了を基準に削除
		if ( qp->m_Work->m_Pos > REG_MAXREP || qp->m_Work->m_DupDone ) {
			qp->m_Next = m_QueFree;
			m_QueFree = qp;
		} else {
			if ( qp->m_Work == whp && (qp->m_Flag & REG_NQFLAG_GREED) != 0 )
				whp = NULL;
			qp->m_Work->m_Die = FALSE;
			qp->m_Next = qtp;
			qtp = qp;
		}
	}
	m_QueHead[0] = qtp;

	// ワークキューを再構築
	wtp = NULL;
	while ( (wp = m_WorkHead) != NULL ) {
		m_WorkHead = wp->m_Next;
		if ( wp->m_Die ) {
			wp->m_Next = m_WorkFree;
			m_WorkFree = wp;
			if ( wp->m_Work[0].m_SPos < wp->m_Work[0].m_EPos && (wop == NULL || wop->m_Seq > wp->m_Seq) )
				wop = wp;
		} else {
			wp->m_Next = wtp;
			wtp = wp;
		}
	}
	if ( (m_WorkHead = wtp) == NULL )
		m_WorkSeq = 0;

	if ( res != NULL ) {
		if ( whp != NULL ) {
			*res = whp->m_Done;
			res->m_Idx = whp->m_Idx;
			res->m_Str = whp->m_Str;
			res->m_Status = (whp->m_Die ? REG_MATCH : REG_MATCHMORE);
		} else if ( wop != NULL ) {
			*res = wop->m_Done;
			res->m_Idx = wop->m_Idx;
			res->m_Str = wop->m_Str;
			res->m_Status = REG_MATCHOVER;
		} else
			res->m_Status = (m_WorkHead != NULL ? REG_CONTINUE : REG_NOMATCH);
	}

	return (whp != NULL || wop != NULL ? TRUE : FALSE);
}
