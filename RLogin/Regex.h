#pragma once
#include "afx.h"

#define	MAPSIZE		512
#define	MAPBYTE		(MAPSIZE / 8)
#define	MAPARRAY	(MAPBYTE / sizeof(DWORD))
#define	MAPBITS		(8 * sizeof(DWORD))

typedef DWORD DCHAR;
typedef DWORD * LPDSTR;
typedef const DWORD * LPCDSTR;

class CStringD : public CObject
{
public:
	CStringW m_Work;
	CDWordArray m_Data;
	CDWordArray m_Vs;

	CStringD();
	CStringD(LPCWSTR str) { *this = str; }
	CStringD(LPCDSTR str) { *this = str; }
	CStringD(const CStringD &data) { *this = data; }
	CStringD(const CStringD &data, int iFirst, int nCount);
	~CStringD();

	inline int GetLength() { return (m_Data.GetSize() - 1); }
	inline void Empty() { m_Data.RemoveAll(); m_Data.Add(0); }
	void Format(LPCWSTR str, ...);
	void Nomalize();

	int ReverseFind(DCHAR ch);
	int Find(DCHAR ch, int iStart = 0);
	int Find(LPCDSTR str, int iStart = 0);
	inline int Find(LPCWSTR str, int iStart = 0) { return Find((LPCDSTR)CStringD(str), iStart); }

	int Compare(LPCDSTR str);
	inline int Compare(LPCWSTR str) { return Compare((LPCDSTR)CStringD(str)); }

	int Insert(int iIndex, DCHAR ch);
	int Insert(int iIndex, const CStringD &data);
	inline int Insert(int iIndex, LPCWSTR str) { return Insert(iIndex, CStringD(str)); }
	int Delete(int iIndex, int nCount = 1);

	inline CStringD Mid(int iFirst, int nCount = (-1)) { return CStringD(*this, iFirst, nCount); }
	inline CStringD Left(int nCount) { return Mid(0, nCount); }
	inline CStringD Right(int nCount) { return Mid(GetLength() - nCount); }

	LPCWSTR WStr(CStringW &str);
	LPCWSTR GetAt(int idx);

	CStringD & operator += (DCHAR ch);
	CStringD & operator += (LPCWSTR str);
	CStringD & operator += (LPCDSTR str);
	CStringD & operator += (const CStringD &data);

	inline CStringD & operator = (DCHAR ch) { Empty(); return (*this += ch); }
	inline CStringD & operator = (LPCWSTR str) { Empty(); return (*this += str); }
	inline CStringD & operator = (LPCDSTR str) { Empty(); return (*this += str); }
	inline CStringD & operator = (const CStringD &data) { Empty(); return (*this += data); }

	inline DCHAR operator [] (int idx) { return m_Data[idx]; }
	inline operator LPCDSTR() { return m_Data.GetData(); }
	inline operator LPCWSTR() { return WStr(m_Work); }
};

class CRegExIdx : public CDWordArray
{
public:
	const CRegExIdx & operator = (CRegExIdx &data);
};

class CRegExNode : public CObject
{
public:
	class CRegExNode *m_Left;
	class CRegExNode *m_Right;
	class CRegExNode *m_List;

	int 	m_Type;
	DCHAR	m_SChar;
	DCHAR	m_EChar;
	DWORD	*m_Map;

	const CRegExNode & operator = (CRegExNode &data);

	CRegExNode(void);
	~CRegExNode(void);
};

class CRegExQue : public CObject
{
public:
	class CRegExQue	*m_Next;
	class CRegExQue	*m_List;

	class CRegExNode	*m_Node;
	class CRegExWork	*m_Work;

	CRegExQue(void);
	~CRegExQue(void);
};

class CRegExArg : public CObject
{
public:
	int			m_SPos;
	int			m_EPos;
	CStringD	m_Str;

	const CRegExArg & operator = (CRegExArg &data);
	CRegExArg(void);
};

#define	REG_NOMATCH		0
#define	REG_CONTINUE	1
#define	REG_MATCH		2
#define	REG_MATCHMORE	3
#define	REG_MATCHOVER	4

class CRegExRes : public CObject
{
public:
	int			m_Status;
	CStringD	m_Str;
	CRegExIdx	m_Idx;
	CArray<CRegExArg, CRegExArg &> m_Data;

	void Clear();
	inline void SetSize(INT_PTR size) { m_Data.SetSize(size); Clear(); }
	inline INT_PTR GetSize() { return m_Data.GetSize(); }
	inline CRegExArg & GetAt(int index) { return m_Data[index]; }
	inline CRegExArg & operator[] (int index) { return m_Data[index]; }
	const CRegExRes & operator = (CRegExRes &data);
};

class CRegExWork : public CObject
{
public:
	class CRegExWork	*m_Next;
	class CRegExWork	*m_List;

	CRegExRes			m_Work;
	CRegExRes			m_Done;
	CStringD			m_Str;
	int					m_Pos;
	int					m_Seq;
	BOOL				m_Die;
	CRegExIdx			m_Idx;

	CRegExWork(void);
	~CRegExWork(void);
};

class CRegEx : public CObject
{
public:
	CRegExNode *m_NodeTop;
	CRegExNode *m_NodeFree;
	CRegExNode *m_NodeHead;

	CRegExQue	*m_QueTop;
	CRegExQue	*m_QueFree;
	CRegExQue	*m_QueHead[3];

	CRegExWork	*m_WorkTop;
	CRegExWork	*m_WorkFree;
	CRegExWork	*m_WorkHead;

	int			m_QueSw;
	int			m_Arg;
	int			m_WorkSeq;
	CRegExWork	m_WorkTmp;

	DCHAR		m_SaveCh;
	int			m_Pos;
	CStringD	m_Patan;

	CRegEx(void);
	~CRegEx(void);

	void InitChar(LPCTSTR str);
	DCHAR GetChar();
	void UnGetChar(DCHAR ch);

	CRegExNode *AllocNode(int type);
	void FreeNode(CRegExNode *np);
	CRegExNode *CopyNodeSub(CRegExNode *np, CPtrArray &dups);
	CRegExNode *CopyNode(CRegExNode *np);
	void AddNode(CRegExNode *hp, int na, CPtrArray &ptr);

	DCHAR CompileEscape(DCHAR ch);
	CRegExNode *CompileRange();
	CRegExNode *CompileSub(DCHAR endc);
	BOOL Compile(LPCTSTR str);

	void AddQue(int sw, CRegExNode *np, CRegExWork *wp);
	CRegExQue *HeadQue(int sw);

	BOOL MatchStrSub(CStringD &str, int start, int end, CRegExRes *res);
	BOOL MatchStr(LPCTSTR str, CRegExRes *res);
	void ConvertRes(LPCWSTR pat, CStringW &buf, CRegExRes *res);
	BOOL ConvertStr(LPCTSTR str, LPCTSTR pat, CString &buf);
	int SplitStr(LPCTSTR str, CStringArray &data);

	void MatchCharInit();
	BOOL MatchChar(DCHAR ch, int idx, CRegExRes *res);
};
