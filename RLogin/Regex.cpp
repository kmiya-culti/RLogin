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
		// Å‘åˆê’v
		// 0ABCABCDEAAAAAAZ9A0
		//     ^ABC |      |
		//          ^ABCDE |
		//                 ^AAAAAAZ
		if ( reg.MatchChar(str[n], n, &res) && (res.m_Status == REG_MATCH || res.m_Status == REG_MATCHOVER) ) {
			for ( i = 0 ; i < res.GetSize() ; i++ )
				TRACE("%d %d:%s(%d)\n", n, i, res[i].m_Str, res.m_Status);
		}

		// Å¬ˆê’v
		// 0ABCABCDEAAAAAAZ9A0
		//   ^AB|         |
		//      ^AB       |
		//                ^AAAAAAZ
		if ( reg.MatchChar(str[n], n, &res) ) {
			reg.MatchCharInit();
			for ( i = 0 ; i < res.GetSize() ; i++ )
				TRACE("%d %d:%s(%d)\n", n, i, res[i].m_Str, res.m_Status);
		}

		// Žžˆê’v
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
	int len = data.m_Data.GetSize() - 1;

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
	int len = data.m_Data.GetSize() - 1;
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
		//           1101 10** **** ****	U+D800 - U+DBFF	ãˆÊƒTƒƒQ[ƒg
		//	         1101 11xx xxxx xxxx	U+DC00 - U+DFFF	‰ºˆÊƒTƒƒQ[ƒg
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
			//           1101 10** **** ****	U+D800 - U+DBFF	ãˆÊƒTƒƒQ[ƒg
			//	         1101 11xx xxxx xxxx	U+DC00 - U+DFFF	‰ºˆÊƒTƒƒQ[ƒg
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
		//           1101 10** **** ****	U+D800 - U+DBFF	ãˆÊƒTƒƒQ[ƒg
		//	         1101 11xx xxxx xxxx	U+DC00 - U+DFFF	‰ºˆÊƒTƒƒQ[ƒg
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

	len = data.m_Data.GetSize() - 1;
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
	m_List  = NULL;

	m_Type  = 0;
	m_SChar = 0;
	m_EChar = (-1);
	m_Map   = NULL;
}
CRegExNode::~CRegExNode(void)
{
	if ( m_List != NULL )
		delete [] m_List;
	if ( m_Map != NULL )
		delete m_Map;
}
const CRegExNode & CRegExNode::operator = (CRegExNode &data)
{
	m_Type  = data.m_Type;
	m_SChar = data.m_SChar;
	m_EChar = data.m_EChar;
	if ( m_Map != NULL )
		delete m_Map;
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
	m_List = m_Next = NULL;
}
CRegExQue::~CRegExQue(void)
{
	if ( m_List != NULL )
		delete [] m_List;
}

//////////////////////////////////////////////////////////////////////
// CRegExArg

CRegExArg::CRegExArg()
{
	m_SPos = 0;
	m_EPos = 0;
	m_Str.Empty();
}
const CRegExArg & CRegExArg::operator = (CRegExArg &data)
{
	m_SPos = data.m_SPos;
	m_EPos = data.m_EPos;
	m_Str  = data.m_Str;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// CRegExRes

const CRegExRes & CRegExRes::operator = (CRegExRes &data)
{
	SetSize(data.GetSize());
	for ( int n = 0 ; n < data.GetSize() ; n++ )
		m_Data[n] = data.m_Data[n];
	m_Idx = data.m_Idx;
	m_Str = data.m_Str;
	return *this;
}
void CRegExRes::Clear()
{
	for ( int n = 0 ; n < m_Data.GetSize() ; n++ )
		m_Data[n].m_SPos = m_Data[n].m_EPos = 0;
}

//////////////////////////////////////////////////////////////////////
// CRegExWork

CRegExWork::CRegExWork()
{
	m_List = NULL;
}
CRegExWork::~CRegExWork(void)
{
	if ( m_List != NULL )
		delete [] m_List;
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
	m_Pos = 0;
	m_QueSw = 0;
}

CRegEx::~CRegEx(void)
{
	if ( m_NodeTop != NULL )
		delete [] m_NodeTop;
	if ( m_QueTop != NULL )
		delete [] m_QueTop;
	if ( m_WorkTop != NULL )
		delete [] m_WorkTop;
}

void CRegEx::InitChar(LPCTSTR str)
{
	m_Patan = TstrToUni(str);
	m_SaveCh = 0;
	m_Pos = 0;
}
DCHAR CRegEx::GetChar()
{
	if ( m_SaveCh != 0 ) {
		DCHAR ch = m_SaveCh;
		m_SaveCh = 0;
		return ch;
	}

	if ( m_Pos >= m_Patan.GetLength() )
		return EOF;

	return m_Patan[m_Pos++];
}
void CRegEx::UnGetChar(DCHAR ch)
{
	m_SaveCh = ch;
}

CRegExNode *CRegEx::AllocNode(int type)
{
	int n;
	CRegExNode *np;

	if ( m_NodeFree == NULL ) {
		np = new CRegExNode[16];
		np->m_List = m_NodeTop;
		m_NodeTop = np;
		for ( n = 0 ; n < 16 ; n++ ) {
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
CRegExNode *CRegEx::CopyNodeSub(CRegExNode *np, CPtrArray &dups)
{
	int n;
	CRegExNode *tp = NULL;
	if ( np == NULL )
		return NULL;
	for ( n = 0 ; n < dups.GetSize() ; n += 2 ) {
		if ( dups[n] == np )
			return (CRegExNode *)dups[n + 1];
	}
	tp = AllocNode(np->m_Type);
	*tp = *np;
	tp->m_Left  = CopyNodeSub(np->m_Left,  dups);
	tp->m_Right = CopyNodeSub(np->m_Right, dups);
	dups.Add(np);
	dups.Add(tp);
	return tp;
}
CRegExNode *CRegEx::CopyNode(CRegExNode *np)
{
	CPtrArray dups;
	return CopyNodeSub(np, dups);
}

DCHAR CRegEx::CompileEscape(DCHAR ch)
{
	int n;
	DCHAR val = 0;

	if ( ch == EOF )
		return ch;
	else if ( ch == L'x' || ch == L'X' ) {
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
	} else if ( ch >= L'0' && ch <= L'7' ) {
		val = val - L'0';
		for ( n = 0 ; (ch = GetChar()) != EOF && n < 2 ; n++ ) {
			if ( ch >= L'0' && ch <= L'7' )
				val = val * 8 + (ch - L'0');
			else
				break;
		}
		UnGetChar(ch);
	} else {
		switch(ch) {
		case L'a': val = L'\x07'; break;
		case L'b': val = L'\x08'; break;
		case L't': val = L'\x09'; break;
		case L'n': val = L'\x0A'; break;
		case L'v': val = L'\x0B'; break;
		case L'f': val = L'\x0C'; break;
		case L'r': val = L'\x0D'; break;
		default: val = ch; break;
		}
	}
	return val;
}
CRegExNode *CRegEx::CompileRange()
{
	DCHAR ch;
	int type = 'c';
	int count = 0;
	CRegExNode tmp;
	CRegExNode *tp;
	DCHAR nch = 0xFFFF;
	DCHAR mch = 0x0000;

	tp = &tmp;
	tp->m_Right = NULL;

	if ( (ch = GetChar()) == '^' )
		type = 'C';
	else
		UnGetChar(ch);

	while ( (ch = GetChar()) != EOF && ch != L']' ) {
		if ( ch == L'\\' && (ch = CompileEscape(GetChar())) == EOF )
			break;
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
				if ( ch == L'\\' && (ch = CompileEscape(GetChar())) == EOF )
					break;
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
		} else
			UnGetChar(ch);
	}

	if ( (mch - nch) < MAPSIZE && count > 2 ) {
		tmp.m_Left = AllocNode('m');
		tmp.m_Left->m_SChar = nch;
		tmp.m_Left->m_EChar = mch;
		tmp.m_Left->m_Map = new DWORD[MAPARRAY];
		memset(tmp.m_Left->m_Map, 0, MAPBYTE);
		for ( tp = tmp.m_Right ; tp != NULL ; tp = tp->m_Right ) {
			for ( ch = tp->m_SChar ; ch <= tp->m_EChar ; ch++ )
				tmp.m_Left->m_Map[(ch - nch) / MAPBITS] |= (1 << ((ch - nch) % MAPBITS));
		}
		if ( type == 'C' ) {
			for ( ch = 0 ; ch < MAPARRAY ; ch++ )
				tmp.m_Left->m_Map[ch] = ~tmp.m_Left->m_Map[ch];
		}
		while ( (tp = tmp.m_Right) != NULL ) {
			tmp.m_Right = tp->m_Right;
			FreeNode(tp);
		}
		tmp.m_Right = tmp.m_Left;
	}

	return tmp.m_Right;
}
CRegExNode *CRegEx::CompileSub(DCHAR endc)
{
	int n;
	int min, max;
	DCHAR ch, nx;
	CRegExNode tmp;
	CRegExNode *lp, *rp, *ep, *tp, *np;

	rp = &tmp;
	lp = NULL;
	rp->m_Left = NULL;

	while ( (ch = GetChar()) != EOF && ch != endc ) {
		switch(ch) {
		case L'[':
			lp = rp;
			if ( (rp->m_Left = CompileRange()) == NULL )
				break;
			while ( rp->m_Left != NULL )
				rp = rp->m_Left;
			break;

		case L'\\':
			if ( (ch = GetChar()) == EOF )
				break;
			switch(ch) {
			case L'd':
			    lp = rp;
				rp->m_Left = AllocNode('c');
				rp = rp->m_Left;
				rp->m_SChar = L'0';
				rp->m_EChar = L'9';
				break;
			case L'D':
			    lp = rp;
				rp->m_Left = AllocNode('C');
				rp = rp->m_Left;
				rp->m_SChar = L'0';
				rp->m_EChar = L'9';
				break;

			case L's':
			    lp = rp;
				rp->m_Left = AllocNode('c');
				rp = rp->m_Left;
				rp->m_SChar = L'\0';
				rp->m_EChar = L' ';
				break;
			case L'S':
			    lp = rp;
				rp->m_Left = AllocNode('C');
				rp = rp->m_Left;
				rp->m_SChar = L'\0';
				rp->m_EChar = L' ';
				break;

			case L'w':
				// [A-Za-z0-9_]
			    lp = rp;
				rp->m_Left = AllocNode('c');
				rp = rp->m_Left;
				rp->m_SChar = L'A';
				rp->m_EChar = L'Z';
				rp->m_Right = AllocNode('c');
				rp->m_Right->m_SChar = L'a';
				rp->m_Right->m_EChar = L'z';
				rp->m_Right->m_Right = AllocNode('c');
				rp->m_Right->m_Right->m_SChar = L'0';
				rp->m_Right->m_Right->m_EChar = L'9';
				rp->m_Right->m_Right->m_Right = AllocNode('c');
				rp->m_Right->m_Right->m_Right->m_SChar = L'_';
				rp->m_Right->m_Right->m_Right->m_EChar = L'_';
				break;
			case L'W':
				// [^A-Za-z0-9_]
			    lp = rp;
				rp->m_Left = AllocNode('C');
				rp = rp->m_Left;
				rp->m_SChar = L'A';
				rp->m_EChar = L'Z';
				rp->m_Right = AllocNode('C');
				rp->m_Right->m_SChar = L'a';
				rp->m_Right->m_EChar = L'z';
				rp->m_Right->m_Right = AllocNode('C');
				rp->m_Right->m_Right->m_SChar = L'0';
				rp->m_Right->m_Right->m_EChar = L'9';
				rp->m_Right->m_Right->m_Right = AllocNode('C');
				rp->m_Right->m_Right->m_Right->m_SChar = L'_';
				rp->m_Right->m_Right->m_Right->m_EChar = L'_';
				break;

			case L'y':
				// "^|\s+"
				//   +--(^)----->+
				// -(b)-(c)-(b)-(b)-
				//       +<--+
				lp = rp;
				rp->m_Left = AllocNode('b');
				rp = rp->m_Left;
				rp->m_Left  = AllocNode('c');
				rp->m_Left->m_SChar = L'\0';
				rp->m_Left->m_EChar = L' ';
				rp->m_Left->m_Left = AllocNode('b');
				rp->m_Left->m_Left->m_Left = AllocNode('b');
				rp->m_Left->m_Left->m_Right = rp->m_Left;
				rp->m_Right = AllocNode('^');
				rp->m_Right->m_Left = rp->m_Left->m_Left->m_Left;
				rp = rp->m_Left->m_Left->m_Left;
				break;
			case L'Y':
				// "$|\s+"
				//   +--($)----->+
				// -(b)-(c)-(b)-(b)-
				//       +<--+
				lp = rp;
				rp->m_Left = AllocNode('b');
				rp = rp->m_Left;
				rp->m_Left  = AllocNode('c');
				rp->m_Left->m_SChar = L'\0';
				rp->m_Left->m_EChar = L' ';
				rp->m_Left->m_Left = AllocNode('b');
				rp->m_Left->m_Left->m_Left = AllocNode('b');
				rp->m_Left->m_Left->m_Right = rp->m_Left;
				rp->m_Right = AllocNode('$');
				rp->m_Right->m_Left = rp->m_Left->m_Left->m_Left;
				rp = rp->m_Left->m_Left->m_Left;
				break;

			default:
				ch = CompileEscape(ch);
				goto CHARMATCH;
			}
			break;

		case L'.':
		    lp = rp;
			rp->m_Left = AllocNode('C');
			rp = rp->m_Left;
			rp->m_SChar = 0x00;
			rp->m_EChar = 0x19;
			break;

		case L'^':
			if ( tmp.m_Left != NULL )
				goto CHARMATCH;
		    lp = NULL;
			rp->m_Left = AllocNode('^');
			rp = rp->m_Left;
			break;

		case L'$':
			if ( tmp.m_Left == NULL )
				goto CHARMATCH;
			nx = GetChar(); UnGetChar(nx);
			if ( nx != EOF && nx != '|' && nx != endc )
				goto CHARMATCH;
		    lp = NULL;
			rp->m_Left = AllocNode('$');
			rp = rp->m_Left;
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
			if ( min > 256 )
				min = 256;
			max = min;
			if ( ch == L',' ) {
				max = 0;
				while ( (ch = GetChar()) >= L'0' && ch <= L'9' )
					max = max * 10 + (ch - L'0');
				if ( max > 256 )
					max = 256;
			}
			if ( ch != L'}' )
				UnGetChar(ch);
		REPENT:
			if ( min == 0 ) {
				tp = AllocNode('b');
				np = AllocNode('b');
				if ( max == 0 ) {
					//   +---------->+
					// -(b)-[a]-(b)-(b)-
					//       +<--+
					tp->m_Left = lp->m_Left;
					lp->m_Left = tp;
					tp->m_Right = np;
					rp->m_Left = AllocNode('b');
					rp = rp->m_Left;
					rp->m_Left = np;
					rp->m_Right = tp->m_Left;
					rp = np;
				} else {
					//   +---------------------->+
					//   |       +-------------->+
					//   |       |       +------>+
					// -(b)-[a]-(b)-[a]-(b)-[a]-(b)-
					tp->m_Left = lp->m_Left;
					lp->m_Left = tp;
					tp->m_Right = np;
					for ( n = 1 ; n < max ; n++ ) {
						ep = AllocNode('b');
						ep->m_Left = CopyNode(tp->m_Left);
						ep->m_Right = np;
						tp = ep;
						rp->m_Left = ep;
						while ( rp->m_Left != NULL )
							rp = rp->m_Left;
					}
					rp->m_Left = np;
					rp = np;
				}
			} else {
				// -[a]-[a]-
				tp = lp;
				for ( n = 1 ; n < min ; n++ ) {
					rp->m_Left = CopyNode(tp->m_Left);
					tp = rp;
					while ( rp->m_Left != NULL )
						rp = rp->m_Left;
				}
				if ( min > max ) {
					// -[a]-[a]-(b)-
					//       +<--+
					rp->m_Left = AllocNode('b');
					rp = rp->m_Left;
					rp->m_Right = tp->m_Left;
				} else if ( min < max ) {
					//           +-------------->+
					//           |       +------>+
					// -[a]-[a]-(b)-[a]-(b)-[a]-(b)-
					np = AllocNode('b');
					for ( n = min ; n < max ; n++ ) {
						ep = AllocNode('b');
						ep->m_Left = CopyNode(tp->m_Left);
						ep->m_Right = np;
						tp = ep;
						rp->m_Left = ep;
						while ( rp->m_Left != NULL )
							rp = rp->m_Left;
					}
					rp->m_Left = np;
					rp = np;
				}
			}
			lp = NULL;
			break;

		case L'(':
			//   +<----->+
			// -(s)-[a]-(e)-
			lp = rp;
			rp->m_Left = AllocNode('s');
			lp->m_Left->m_EChar = m_Arg++;
			rp->m_Left->m_Left = CompileSub(')');
		    while ( rp->m_Left != NULL )
				rp = rp->m_Left;
			rp->m_Left = AllocNode('e');
			rp = rp->m_Left;
			rp->m_Right = lp->m_Left;
			lp->m_Left->m_Right = rp;
			break;

		case L'|':
			//   +--[a]--+
			// -(b)-[a]-(b)-
			nx = GetChar(); UnGetChar(nx);
			if ( lp == NULL || nx == EOF || nx == endc )
				goto CHARMATCH;
			tp = AllocNode('b');
			np = AllocNode('b');
			tp->m_Left = tmp.m_Left;
			tmp.m_Left = tp;
			rp->m_Left = np;
			if ( (rp = tp->m_Right = CompileSub(endc)) == NULL )
				tp->m_Right = np;
			else {
				while ( rp->m_Left != NULL )
					rp = rp->m_Left;
				rp->m_Left = np;
			}
			lp = NULL;
			rp = np;
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

	return tmp.m_Left;
}
BOOL CRegEx::Compile(LPCTSTR str)
{
	CRegExNode *np;

	if ( m_NodeTop != NULL ) {
		delete [] m_NodeTop;
		m_NodeTop = m_NodeFree = NULL;
	}
	m_Arg = 1;
	m_NodeHead = NULL;
	InitChar(str);

	if ( *str != '\0' ) {
		//   +<----->+
		// -(s)-[a]-(e)-
		m_NodeHead = AllocNode('s');
		m_NodeHead->m_EChar = 0;
		m_NodeHead->m_Left = CompileSub(EOF);
		for ( np = m_NodeHead ; np->m_Left != NULL ; )
			np = np->m_Left;
		np->m_Left = AllocNode('e');
		m_NodeHead->m_Right = np->m_Left;
		np->m_Left->m_Right = m_NodeHead;
	}

	m_WorkTmp.m_Work.SetSize(m_Arg);
	m_WorkTmp.m_Done.SetSize(m_Arg);
	m_WorkTmp.m_Seq = 0;

	return (m_NodeHead != NULL ? TRUE : FALSE);
}
void CRegEx::AddQue(int sw, CRegExNode *np, CRegExWork *wp)
{
	int n;
	CRegExQue *qp;

	if ( np == NULL )
		return;
	for ( qp = m_QueHead[sw] ; qp != NULL ; qp = qp->m_Next ) {
		if ( qp->m_Node == np ) {
			if ( qp->m_Work != NULL && wp != NULL && qp->m_Work->m_Seq > wp->m_Seq )
				qp->m_Work = wp;
			return;
		}
	}
	if ( m_QueFree == NULL ) {
		qp = new CRegExQue[16];
		qp->m_List = m_QueTop;
		m_QueTop = qp;
		for ( n = 0 ; n < 16 ; n++ ) {
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
	int n, i;
	CRegExNode *np, *ip;
	CRegExQue *qp;
	CRegExWork *wp;

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
				AddQue(m_QueSw, np->m_Left, wp);
				break;
			case 'e':
				i = np->m_Right->m_EChar;
				wp->m_Work[i].m_EPos = n;
				if ( i == 0 )
					wp->m_Done = wp->m_Work;
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
			case '$':
				if ( n >= end )
					AddQue(m_QueSw, np->m_Left, wp);
				else if ( str[n] == L'\r' )
					AddQue(m_QueSw, np->m_Left, wp);
				break;
			case 'm':
				if ( n >= end )
					break;
				if ( str[n] >= np->m_SChar && str[n] <= np->m_EChar && (np->m_Map[(str[n] - np->m_SChar) / MAPBITS] & (1 << ((str[n] - np->m_SChar) % MAPBITS))) != 0 )
					AddQue(m_QueSw^1, np->m_Left, wp);
				break;
			case 'c':
			case 'C':
				if ( n >= end )
					break;
				for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
					if ( str[n] >= ip->m_SChar && str[n] <= ip->m_EChar )
						break;
				}
				if ( (np->m_Type == 'c' && ip != NULL) || (np->m_Type == 'C' && ip == NULL ) )
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

	if ( res != NULL ) {
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
	while ( *pat != '\0' ) {
		if ( pat[0] == '%' && (pat[1] >= '0' && pat[1] <= '9') ) {
			pat++;
			for ( n = 0 ; *pat >= '0' && *pat <= '9' ; pat++ )
					n = n * 10 + (*(pat++) - '0');
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
	int i;
	int nc = FALSE;
	CRegExNode *np, *ip;
	CRegExQue *qp;
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
		wp = new CRegExWork[16];
		wp->m_List = m_WorkTop;
		m_WorkTop = wp;
		for ( i = 0 ; i < 16 ; i++ ) {
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
	wp->m_Pos = 0;
	wp->m_Str.Empty();
	wp->m_Seq = m_WorkSeq++;
	wp->m_Idx.RemoveAll();
	AddQue(m_QueSw, m_NodeHead, wp);

	for ( wp = m_WorkHead ; wp != NULL ; wp = wp->m_Next ) {
		wp->m_Die = TRUE;
		wp->m_Pos = wp->m_Str.GetLength();
		wp->m_Str += ch;
		wp->m_Idx.Add(idx);
	}

	do {
		while ( (qp = HeadQue(m_QueSw)) != NULL ) {
			np = qp->m_Node;
			wp = qp->m_Work;

			switch(np->m_Type) {
			case 's':
				i = np->m_EChar;
				if ( wp->m_Work[i].m_SPos >= wp->m_Work[i].m_EPos || wp->m_Work[i].m_EPos != wp->m_Pos ) {
					wp->m_Work[i].m_SPos = wp->m_Pos;
					wp->m_Work[i].m_EPos = wp->m_Pos;
				}
				AddQue(m_QueSw, np->m_Left, wp);
				break;
			case 'e':
				i = np->m_Right->m_EChar;
				wp->m_Work[i].m_EPos = wp->m_Pos;
				wp->m_Work[i].m_Str = wp->m_Str.Mid(wp->m_Work[i].m_SPos, wp->m_Work[i].m_EPos - wp->m_Work[i].m_SPos);
				if ( i == 0 ) {
					wp->m_Done = wp->m_Work;
					if ( wp->m_Done[i].m_SPos < wp->m_Done[i].m_EPos && (whp == NULL || whp->m_Seq >= wp->m_Seq) )
						whp  = wp;
				}
				AddQue(m_QueSw, np->m_Left, wp);
				break;
			case 'b':
				AddQue(m_QueSw, np->m_Left,  wp);
				AddQue(m_QueSw, np->m_Right, wp);
				break;
			case '^':
				if ( nc )
					AddQue(m_QueSw^1, np, wp);
				else if ( ch == L'\n' )
					AddQue(2, np->m_Left, wp);
				break;
			case '$':
				if ( nc )
					AddQue(m_QueSw^1, np, wp);
				else if ( ch == L'\r' )
					AddQue(m_QueSw, np->m_Left, wp);
				break;
			case 'm':
				if ( nc )
					AddQue(m_QueSw^1, np, wp);
				else if ( ch >= np->m_SChar && ch <= np->m_EChar && (np->m_Map[(ch - np->m_SChar) / MAPBITS] & (1 << ((ch - np->m_SChar) % MAPBITS))) != 0 )
					AddQue(2, np->m_Left, wp);
				break;
			case 'c':
			case 'C':
				if ( nc )
					AddQue(m_QueSw^1, np, wp);
				else {
					for ( ip = np ; ip != NULL ; ip = ip->m_Right ) {
						if ( ch >= ip->m_SChar && ch <= ip->m_EChar )
							break;
					}
					if ( (np->m_Type == 'c' && ip != NULL) || (np->m_Type == 'C' && ip == NULL ) )
						AddQue(2, np->m_Left, wp);
				}
				break;
			}
		}

		if ( m_QueHead[2] == NULL )
			break;

		if ( !nc ) {
			for ( wp = m_WorkHead ; wp != NULL ; wp = wp->m_Next )
				wp->m_Pos++;
			nc = TRUE;
		}
		while ( (qp = HeadQue(2)) != NULL ) {
			if ( qp->m_Work->m_Pos < 512 )
				AddQue(m_QueSw, qp->m_Node, qp->m_Work);
		}

	} while ( m_QueHead[m_QueSw] != NULL );

	m_QueSw ^= 1;

	for ( qp = m_QueHead[m_QueSw] ; qp != NULL ; qp = qp->m_Next )
		wp->m_Die = FALSE;

	wtp = NULL;
	while ( (wp = m_WorkHead) != NULL ) {
		m_WorkHead = wp->m_Next;
		if ( wp->m_Die ) {
			wp->m_Next = m_WorkFree;
			m_WorkFree = wp;
			if ( wp->m_Done[0].m_SPos < wp->m_Done[0].m_EPos && (wop == NULL || wop->m_Seq > wp->m_Seq) )
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
