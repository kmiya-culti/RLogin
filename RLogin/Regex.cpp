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

	m_Type  = 0;
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
		m_Map = new DWORD[MAPARRAY];
		memcpy(m_Map, data.m_Map, MAPBYTE);
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

CRegEx::CRegEx(void)
{
	m_NodeTop = m_NodeFree = m_NodeHead = NULL;
	m_QueTop  = m_QueFree  = NULL;
	m_QueHead[0] = m_QueHead[1] = m_QueHead[2] = NULL;
	m_WorkTop = m_WorkFree = m_WorkHead = NULL;
	m_WorkSeq = 0;
	m_QueSw = 0;
	m_CompStrBuf = NULL;
	m_ErrMsg.Empty();
}

CRegEx::~CRegEx(void)
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

CRegExNode *CRegEx::AllocNode(int type)
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
		rp->m_Left = AllocNode('c');
		rp = rp->m_Left;
		rp->m_SChar = L'0';
		rp->m_EChar = L'9';
		break;
	case L'D':
		rp->m_Left = AllocNode('C');
		rp = rp->m_Left;
		rp->m_SChar = L'0';
		rp->m_EChar = L'9';
		break;

	case L's':
		rp->m_Left = AllocNode('c');
		rp = rp->m_Left;
		rp->m_SChar = L'\0';
		rp->m_EChar = L' ';
		break;
	case L'S':
		rp->m_Left = AllocNode('C');
		rp = rp->m_Left;
		rp->m_SChar = L'\0';
		rp->m_EChar = L' ';
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
				rp->m_Left = AllocNode('c');
				rp = rp->m_Left;
				rp->m_SChar = rp->m_EChar = nx;

				if ( (nx = GetChar()) == EOF )
					throw _T("RegEx syntax error\n{...  no end to character list \"}\"");
				else if ( nx == L'}' )
					break;
				else if ( nx > L' ' )
					throw _T("RegEx syntax error\n{0...}  character list overflow ?");
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
	case 'c':
		rp->m_Type = 'C';
		break;
	case 'C':
		rp->m_Type = 'c';
		break;
	case 'm':
		rp->m_Type = 'M';
		break;
	case 'M':
		rp->m_Type = 'm';
		break;
	case '^':
		rp->m_Type = '1';
		break;
	case '$':
		rp->m_Type = '2';
		break;
	case '1':
		rp->m_Type = '^';
		break;
	case '2':
		rp->m_Type = '$';
		break;
	case 'b':
		CompileReverse(rp->m_Right);
		break;
	}

	CompileReverse(rp->m_Left);
}
CRegExNode *CRegEx::CompileRange(CRegExNode *rp)
{
	DCHAR ch;
	int type = 'c';
	int count = 0;
	CRegExNode tmp, *tp, *np, *ap, *bp = NULL;
	DCHAR nch = 0xFFFF;
	DCHAR mch = 0x0000;

	if ( (ch = GetChar()) == '^' )
		type = 'C';
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
			ap = AllocNode('b');
			ap->m_Right = tmp.m_Left;
			if ( (np = CompileExtChar(ch, ap)) != NULL ) {
				if ( bp == NULL )
					bp = AllocNode('b');
				np->m_Left = bp;
				tmp.m_Left = ap;
				if ( type == 'C' )
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
				ch = 0x0010FFFF;
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
		throw _T("RegEx syntax error\n[...  no end to character list \"]\"");

	if ( tmp.m_Right != NULL && (mch - nch) < MAPSIZE && count > 2 ) {
		np = AllocNode(type == 'c' ? 'm' : 'M');
		np->m_SChar = nch;
		np->m_EChar = mch;
		np->m_Map = new DWORD[MAPARRAY];
		memset(np->m_Map, 0, MAPBYTE);

		for ( tp = tmp.m_Right ; tp != NULL ; tp = tp->m_Right ) {
			for ( ch = tp->m_SChar ; ch <= tp->m_EChar ; ch++ )
				np->m_Map[(ch - nch) / MAPBITS] |= (1 << ((ch - nch) % MAPBITS));
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
	CRegExNode *pLast = NULL;
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
			rp->m_Left = AllocNode('C');
			rp = rp->m_Left;
			rp->m_SChar = 0x0A;
			rp->m_EChar = 0x0A;
			rp->m_Right = AllocNode('C');
			rp->m_Right->m_SChar = 0x0D;
			rp->m_Right->m_EChar = 0x0D;
			break;

		case L'^':
			// -(s)-(^)-(e)-
		    lp = NULL;
			rp->m_Left = AllocNode('s');
			rp->m_Left->m_EChar = m_Arg++;
			rp->m_Left->m_Left = AllocNode('^');
			rp->m_Left->m_Left->m_Left = AllocNode('e');
			rp->m_Left->m_Left->m_Left->m_EChar = 2;
			rp->m_Left->m_Left->m_Left->m_SChar = m_ArgStack.GetHead();
			rp->m_Left->m_Right = rp->m_Left->m_Left->m_Left;
			rp->m_Left->m_Left->m_Left->m_Right = rp->m_Left;
			rp = rp->m_Left->m_Left->m_Left;
			break;
		case L'$':
			// -(s)-($)-(e)-
		    lp = NULL;
			rp->m_Left = AllocNode('s');
			rp->m_Left->m_EChar = m_Arg++;
			rp->m_Left->m_Left = AllocNode('$');
			rp->m_Left->m_Left->m_Left = AllocNode('e');
			rp->m_Left->m_Left->m_Left->m_EChar = 1;
			rp->m_Left->m_Left->m_Left->m_SChar = m_ArgStack.GetHead();
			rp->m_Left->m_Right = rp->m_Left->m_Left->m_Left;
			rp->m_Left->m_Left->m_Left->m_Right = rp->m_Left;
			rp = rp->m_Left->m_Left->m_Left;
			break;

		case L'*':
			if ( lp == NULL )
				goto CHARMATCH;
			min = 0;
			max = 0;
			goto REPENT;
		case L'+':
			if ( lp == NULL )
				goto CHARMATCH;
			min = 1;
			max = 0;
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
				max = 0;
				while ( (ch = GetChar()) >= L'0' && ch <= L'9' )
					max = max * 10 + (ch - L'0');
			}
			if ( ch != L'}' )
				throw _T("RegEx syntax error\n{...  no end to character list \"}\"");

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

			//   +-------------->+
			// -(n)-(?)-[a]-(x)-(b)-
			//       +<------+
			tp = AllocNode('n');
			np = AllocNode('b');
			if ( noMore != 0 ) {
				tp->m_Left = AllocNode('?');
				tp->m_Left->m_EChar = m_ArgStack.GetHead();
				tp->m_Left->m_Left = lp->m_Left;
				lp->m_Left = tp;
			} else {
				tp->m_Left = lp->m_Left;
				lp->m_Left = tp;
			}
			tp->m_SChar = m_Count++;
			tp->m_EChar = min;
			tp->m_Right = np;
			rp->m_Left = AllocNode('x');
			rp = rp->m_Left;
			rp->m_SChar = tp->m_SChar;
			rp->m_EChar = (min << 16) | max;
			rp->m_Left = np;
			rp->m_Right = tp->m_Left;
			rp = np;
			lp = NULL;
			break;

		case L'(':
			n = 000;
			if ( (ch = GetChar()) == L'?' ) {
				if ( (ch = GetChar()) == L':' ) {
					n = 020;
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
						UnGetChar(ch);
						n = 000;
					}
				}
			} else
				UnGetChar(ch);
			//   +<----->+
			// -(s)-[a]-(e)-
			lp = rp;
			rp->m_Left = AllocNode('s');
			rp->m_Left->m_EChar = m_Arg++;
			m_ArgStack.AddHead(rp->m_Left->m_EChar);
			rp->m_Left->m_Left = CompileSub(')');
			if ( (n & 002) != 0 )
				CompileReverse(rp->m_Left->m_Left);
		    while ( rp->m_Left != NULL )
				rp = rp->m_Left;
			rp->m_Left = AllocNode('e');
			rp = rp->m_Left;
			switch(n) {
			case 001:	// ...(?=...)
			case 002:	// ...(?!...)
				rp->m_EChar = 1;
				break;
			case 011:	// (?<=...)...
			case 012:	// (?<!...)...
				rp->m_EChar = 2;
				break;
			case 020:	// (?:...)
				rp->m_EChar = 3;
				break;
			default:
				rp->m_EChar = 0;
				break;
			}
			m_ArgStack.RemoveHead();
			rp->m_SChar = m_ArgStack.GetHead();
			rp->m_Right = lp->m_Left;
			lp->m_Left->m_Right = rp;
			break;

		case L'|':
			// -(b)-   -(b)-
			//   +- [a]--+
			tp = AllocNode('b');
			tp->m_Right = tmp.m_Left;
			tmp.m_Left = tp;
			if ( pLast == NULL )
				pLast = AllocNode('b');
			rp->m_Left = pLast;
			lp = NULL;
			rp = tp;
		    break;

		CHARMATCH:
		default:
		    lp = rp;
			rp->m_Left = AllocNode('c');
			rp = rp->m_Left;
		    rp->m_SChar = rp->m_EChar = ch;
			break;
		}
	}

	if ( ch != endc && endc != EOF )
		throw _T("RegEx syntax error\n(...  no end to character list \")\"");

	if ( pLast != NULL )
		rp->m_Left = pLast;

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

	m_Arg = 1;
	m_ArgStack.RemoveAll();
	m_Count = 0;
	RemoveAllNode();
	InitChar(str);

	try {
		//   +<----->+
		// -(s)-[a]-(e)-
		m_NodeHead = AllocNode('s');
		m_NodeHead->m_EChar = 0;
		m_NodeHead->m_SChar = 0;
		m_ArgStack.AddHead(0);
		m_NodeHead->m_Left = CompileSub(EOF);
		for ( np = m_NodeHead ; np->m_Left != NULL ; )
			np = np->m_Left;
		np->m_Left = AllocNode('e');
		np->m_Left->m_EChar = 0;
		m_ArgStack.RemoveHead();
		m_NodeHead->m_Right = np->m_Left;
		np->m_Left->m_Right = m_NodeHead;

	} catch(LPCTSTR msg) {
		m_ErrMsg = msg;
		RemoveAllNode();
	}

	EndofChar();

	m_WorkTmp.m_Work.SetSize(m_Arg);
	m_WorkTmp.m_Done.SetSize(m_Arg);
	m_WorkTmp.m_Counter.SetSize(m_Count);
	m_WorkTmp.m_Seq = 0;

	return (m_NodeHead != NULL ? TRUE : FALSE);
}
BOOL CRegEx::IsSimple()
{
	CRegExNode *np = m_NodeHead;

	if ( np == NULL || np->m_Type != 's' )
		return FALSE;

	for ( np = np->m_Left ; np != NULL && np->m_Type == 'c' ; np = np->m_Left ) {
		if ( np->m_SChar != np->m_EChar || np->m_Right != NULL )
			return FALSE;
	}

	if ( np == NULL || np->m_Type != 'e' || np->m_Left != NULL )
		return FALSE;

	return TRUE;
}
void CRegEx::AddQue(int sw, CRegExNode *np, CRegExWork *wp)
{
	int n;
	CRegExQue *qp;

	if ( np == NULL )
		return;

	for ( qp = m_QueHead[sw] ;  qp != NULL ; qp = qp->m_Next ) {
		if ( qp->m_Node == np ) {
			if ( qp->m_Work->m_Seq > wp->m_Seq )
				qp->m_Work = wp;
			return;
		}
	}

	if ( m_QueFree == NULL ) {
		qp = new CRegExQue[REG_TABMEM_SIZE];
		qp->m_List = m_QueTop;
		m_QueTop = qp;
		for ( n = 0 ; n < REG_TABMEM_SIZE ; n++ ) {
			qp->m_Next = m_QueFree;
			m_QueFree = qp++;
		}
	}
	qp = m_QueFree;
	m_QueFree = qp->m_Next;

	qp->m_Node = np;
	qp->m_Work = wp;

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
BOOL CRegEx::MatchStrSub(CStringD &str, int start, int end, CRegExRes *res)
{
	int n, i, Pos;
	int min, max;
	CRegExNode *np, *ip;
	CRegExQue *qp;
	CRegExWork *wp = NULL;

	m_QueSw = 0;
	while ( HeadQue(0) != NULL );
	while ( HeadQue(1) != NULL );

	m_WorkTmp.m_Work.Clear();
	m_WorkTmp.m_Done.Clear();

	AddQue(m_QueSw, m_NodeHead, &m_WorkTmp);

	for ( n = start ; n <= end ; n++ ) {

		while ( (qp = HeadQue(m_QueSw)) != NULL ) {
			np = qp->m_Node;
			wp = qp->m_Work;

			switch(np->m_Type) {
			case 's':
				i = np->m_EChar;

				if ( wp->m_Work[i].m_SPos >= wp->m_Work[i].m_EPos || wp->m_Work[i].m_EPos != wp->m_Pos ) {
					wp->m_Work[i].m_SPos = n;
					wp->m_Work[i].m_EPos = n;
				}

				wp->m_Work[i].m_FrArg.RemoveAll();
				wp->m_Work[i].m_BkArg.RemoveAll();
				wp->m_Work[i].m_Done = FALSE;

				AddQue(m_QueSw, np->m_Left, wp);
				break;
			case 'e':
				i = np->m_Right->m_EChar;
				wp->m_Work[i].m_EPos = n;
				wp->m_Work[i].m_Done = TRUE;
				wp->m_Work[i].m_Del = (np->m_EChar == 0 ? FALSE : TRUE);

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
				case 1:		// ...(?=...) or ...(?!...)
					wp->m_Work[np->m_SChar].m_BkArg.AddHead(i);
					break;
				case 2:		// (?<=...)... or (?<!...)...
					wp->m_Work[np->m_SChar].m_FrArg.AddTail(i);
					break;
				}

				if ( i == 0 )
					wp->m_Done = wp->m_Work;

				AddQue(m_QueSw, np->m_Left, wp);
				break;

			case 'n':
				wp->m_Counter[np->m_SChar] = 0;
				AddQue(m_QueSw, np->m_Left, wp);
				if ( np->m_EChar == 0 )
					AddQue(m_QueSw, np->m_Right, wp);
				break;
			case 'x':
				i = ++(wp->m_Counter[np->m_SChar]);
				min = np->m_EChar >> 16;
				max = np->m_EChar & 0xFFFF;
				if ( i >= min )
					AddQue(m_QueSw, np->m_Left, wp);
				if ( max == 0 || min > max || i < max )
					AddQue(m_QueSw, np->m_Right, wp);
				break;
			case '?':
				i = np->m_EChar;
				if ( wp->m_Work[i].m_Done )
					break;
				AddQue(m_QueSw, np->m_Left, wp);
				break;

			case 'b':
				AddQue(m_QueSw, np->m_Left,  wp);
				AddQue(m_QueSw, np->m_Right, wp);
				break;

			case '^':
				if ( n == 0 )
					AddQue(m_QueSw, np->m_Left, wp);
				else if ( n < end && str[n] == L'\n' )
					AddQue(m_QueSw^1, np->m_Left, wp);
				break;
			case '1':
				if ( n != 0 )
					AddQue(m_QueSw, np->m_Left, wp);
				else if ( n < end && str[n] != L'\n' )
					AddQue(m_QueSw^1, np->m_Left, wp);
				break;

			case '$':
				if ( n >= end )
					AddQue(m_QueSw, np->m_Left, wp);
				else if ( str[n] == L'\r' )
					AddQue(m_QueSw, np->m_Left, wp);
				break;
			case '2':
				if ( n < end )
					AddQue(m_QueSw, np->m_Left, wp);
				else if ( str[n] != L'\r' )
					AddQue(m_QueSw, np->m_Left, wp);
				break;

			case 'm':
				if ( n >= end )
					break;
				if ( str[n] >= np->m_SChar && str[n] <= np->m_EChar && (np->m_Map[(str[n] - np->m_SChar) / MAPBITS] & (1 << ((str[n] - np->m_SChar) % MAPBITS))) != 0 )
					AddQue(m_QueSw^1, np->m_Left, wp);
				break;
			case 'M':
				if ( n >= end )
					break;
				if ( str[n] < np->m_SChar || str[n] > np->m_EChar || (np->m_Map[(str[n] - np->m_SChar) / MAPBITS] & (1 << ((str[n] - np->m_SChar) % MAPBITS))) == 0 )
					AddQue(m_QueSw^1, np->m_Left, wp);
				break;

			case 'c':
				if ( n >= end )
					break;
				for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
					if ( str[n] >= ip->m_SChar && str[n] <= ip->m_EChar )
						break;
				}
				if ( ip != NULL )
					AddQue(m_QueSw^1, np->m_Left, wp);
				break;
			case 'C':
				if ( n >= end )
					break;
				for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
					if ( str[n] >= ip->m_SChar && str[n] <= ip->m_EChar )
						break;
				}
				if ( ip == NULL )
					AddQue(m_QueSw^1, np->m_Left, wp);
				break;
			}
		}
		m_QueSw ^= 1;

		if ( m_QueHead[m_QueSw] == NULL )
			break;
	}

	if ( m_WorkTmp.m_Done[0].m_SPos >= m_WorkTmp.m_Done[0].m_EPos ) {
		if ( res != NULL )
			res->m_Status = (m_QueHead[m_QueSw] != NULL ? REG_CONTINUE : REG_NOMATCH);
		return FALSE;
	}

	if ( res != NULL && wp != NULL ) {
		*res = wp->m_Done;
		res->m_Status = (m_QueHead[m_QueSw] != NULL ? REG_MATCHMORE : REG_MATCH);
		for ( n = 0 ; n < m_Arg ; n++ )
			(*res)[n].m_Str = str.Mid((*res)[n].m_SPos, (*res)[n].m_EPos - (*res)[n].m_SPos);
	}

	return TRUE;
}
BOOL CRegEx::MatchStr(LPCTSTR str, CRegExRes *res)
{
	int n, len;
	CStringD tmp;

	tmp = TstrToUni(str);
	len = tmp.GetLength();

	for ( n = 0 ; n < len ; n++ ) {
		if ( MatchStrSub(tmp, n, len, res) )
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
	int n, len;
	BOOL rt = FALSE;
	CStringD wrk;
	CStringW tmp[4];
	CRegExRes res;

	wrk = TstrToUni(str);
	tmp[1] = pat;
	len = wrk.GetLength();

	for ( n = 0 ; n < len ; ) {
		if ( MatchStrSub(wrk, n, len, &res) ) {
			ConvertRes(tmp[1], tmp[3], &res);
			tmp[2] += tmp[3];
			n = res[0].m_EPos;
			rt = TRUE;
		} else {
			tmp[2] += wrk.GetAt(n++);
		}
	}
	buf = tmp[2];
	return rt;
}
int CRegEx::SplitStr(LPCTSTR str, CStringArray &data)
{
	int n, a, len;
	CStringD tmp;
	CRegExRes res;

	tmp = TstrToUni(str);
	len = tmp.GetLength();

	data.RemoveAll();
	for ( n = a = 0 ; n < len ; ) {
		if ( MatchStrSub(tmp, n, len, &res) ) {
			data.Add(UniToTstr(tmp.Mid(a, res[0].m_SPos - a)));
			n = a = res[0].m_EPos;
		} else
			n++;
	}
	if ( a < n )
		data.Add(UniToTstr(tmp.Mid(a, n - a)));
	return (int)data.GetSize();
}

void CRegEx::MatchCharInit()
{
	CRegExWork *wp;

	m_QueSw = 0;
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
BOOL CRegEx::MatchChar(DCHAR ch, int idx, CRegExRes *res)
{
	int i, Pos, Que;
	int min, max;
	CRegExNode *np, *ip;
	CRegExQue *qp, *qtp;
	CRegExWork *wp, *wtp;
	CRegExWork *whp = NULL;
	CRegExWork *wop = NULL;

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
	wp->m_Pos = 0;
	wp->m_Str.Empty();
	wp->m_Seq = m_WorkSeq++;
	wp->m_Idx.RemoveAll();
	AddQue(0, m_NodeHead, wp);

	for ( wp = m_WorkHead ; wp != NULL ; wp = wp->m_Next ) {
		wp->m_Die = TRUE;
		wp->m_Pos = wp->m_Str.GetLength();
		wp->m_Str += ch;
		wp->m_Idx.Add(idx);
	}

	for ( Que = 0 ; Que < 2 ; Que++ ) {
		while ( (qp = HeadQue(Que)) != NULL ) {
			np = qp->m_Node;
			wp = qp->m_Work;

			//TRACE("%d(%c:%d) ", wp->m_Seq, np->m_Type, Que);

			switch(np->m_Type) {
			case 's':
				i = np->m_EChar;

				Pos = wp->m_Pos + Que;
				if ( wp->m_Work[i].m_SPos >= wp->m_Work[i].m_EPos || wp->m_Work[i].m_EPos != Pos )
					wp->m_Work[i].m_SPos = wp->m_Work[i].m_EPos = Pos;

				wp->m_Work[i].m_FrArg.RemoveAll();
				wp->m_Work[i].m_BkArg.RemoveAll();
				wp->m_Work[i].m_Done = FALSE;

				AddQue(Que, np->m_Left, wp);
				break;
			case 'e':
				i = np->m_Right->m_EChar;
				wp->m_Work[i].m_EPos = wp->m_Pos + Que;
				wp->m_Work[i].m_Done = TRUE;
				wp->m_Work[i].m_Del = (np->m_EChar == 0 ? FALSE : TRUE);

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
				case 1:		// ...(?=...) or ...(?!...)
					wp->m_Work[np->m_SChar].m_BkArg.AddHead(i);
					break;
				case 2:		// (?<=...)... or (?<!...)...
					wp->m_Work[np->m_SChar].m_FrArg.AddTail(i);
					break;
				}

				if ( i == 0 ) {
					wp->m_Done = wp->m_Work;
					if ( wp->m_Work[i].m_SPos < wp->m_Work[i].m_EPos && (whp == NULL || whp->m_Seq >= wp->m_Seq) )
						whp  = wp;
				}

				AddQue(Que, np->m_Left, wp);
				break;

			case 'n':
				wp->m_Counter[np->m_SChar] = 0;
				AddQue(Que, np->m_Left, wp);
				if ( np->m_EChar == 0 )
					AddQue(Que, np->m_Right, wp);
				break;
			case 'x':
				i = ++(wp->m_Counter[np->m_SChar]);
				min = np->m_EChar >> 16;
				max = np->m_EChar & 0xFFFF;
				if ( i >= min )
					AddQue(Que, np->m_Left, wp);
				if ( max == 0 || min > max || i < max )
					AddQue(Que, np->m_Right, wp);
				break;
			case '?':
				i = np->m_EChar;
				if ( wp->m_Work[i].m_Done )
					break;
				if ( Que > 0 )
					AddQue(2, np, wp);
				else
					AddQue(Que, np->m_Left, wp);
				break;

			case 'b':
				AddQue(Que, np->m_Left,  wp);
				AddQue(Que, np->m_Right, wp);
				break;

			case '^':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else if ( ch == L'\n' )
					AddQue(1, np->m_Left, wp);
				break;
			case '1':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else if ( ch != L'\n' )
					AddQue(1, np->m_Left, wp);
				break;

			case '$':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else if ( ch == L'\r' )
					AddQue(1, np->m_Left, wp);
				break;
			case '2':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else if ( ch != L'\r' )
					AddQue(1, np->m_Left, wp);
				break;

			case 'm':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else if ( ch >= np->m_SChar && ch <= np->m_EChar && (np->m_Map[(ch - np->m_SChar) / MAPBITS] & (1 << ((ch - np->m_SChar) % MAPBITS))) != 0 )
					AddQue(1, np->m_Left, wp);
				break;
			case 'M':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else if ( ch < np->m_SChar || ch > np->m_EChar || (np->m_Map[(ch - np->m_SChar) / MAPBITS] & (1 << ((ch - np->m_SChar) % MAPBITS))) == 0 )
					AddQue(1, np->m_Left, wp);
				break;

			case 'c':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else {
					for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
						if ( ch >= ip->m_SChar && ch <= ip->m_EChar )
							break;
					}
					if ( ip != NULL )
						AddQue(1, np->m_Left, wp);
				}
				break;
			case 'C':
				if ( Que > 0 )
					AddQue(2, np, wp);
				else {
					for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
						if ( ch >= ip->m_SChar && ch <= ip->m_EChar )
							break;
					}
					if ( ip == NULL )
						AddQue(1, np->m_Left, wp);
				}
				break;
			}
		}

		//TRACE("\n");
	}

	// ノードキューのワークをチェックして再構築
	qtp = NULL;
	while ( (qp = m_QueHead[2]) != NULL ) {
		m_QueHead[2] = qp->m_Next;
		if ( qp->m_Work->m_Pos > REG_MAXREP ) {
			qp->m_Next = m_QueFree;
			m_QueFree = qp;
		} else {
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
