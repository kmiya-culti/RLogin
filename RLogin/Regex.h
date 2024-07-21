#pragma once
#include "afx.h"

typedef DWORD DCHAR;
typedef DWORD * LPDSTR;
typedef const DWORD * LPCDSTR;

#define	IsWchar(ch)			(((ch) & 0xFFFF0000) == 0)
#define	IsRegSpChar(ch)		(((ch) & 0xFFFFFFF0) == 0xFFFFFFF0)

#define	REGSPCHAR_LINETOP	0xFFFFFFF0
#define	REGSPCHAR_LINEEND	0xFFFFFFF1

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

	inline int GetLength() { return (int)(m_Data.GetSize() - 1); }
	inline void Empty() { m_Data.RemoveAll(); m_Data.Add(0); m_Vs.RemoveAll(); m_Vs.Add(0); }
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

//////////////////////////////////////////////////////////////////////

#define	REG_NOMATCH		0
#define	REG_CONTINUE	1
#define	REG_MATCH		2
#define	REG_MATCHMORE	3
#define	REG_MATCHOVER	4

#define	REG_MAXREP		1024
#define	REG_MAXWORK		256

#define	REG_TABMEM_SIZE	32

#define	REG_MAPSIZE		512
#define	REG_MAPBYTE		(REG_MAPSIZE / 8)
#define	REG_MAPARRAY	(REG_MAPBYTE / sizeof(DWORD))
#define	REG_MAPBITS		(8 * sizeof(DWORD))

enum RegCmdType {
	RGE_GROUPSTART, RGE_GROUPEND,
	RGE_LOOPSTART, RGE_LOOPEND,	RGE_LOOPSHORT,
	RGE_GREEDSTART, RGE_GREEDEND,
	RGE_BRANCHES,
	RGE_LINETOP, RGE_NEGTOP, RGE_LINEBTM, RGE_NEGBTM,
	RGE_CHKMAP, RGE_NEGMAP,
	RGE_RANGECHAR, RGE_NEGCHAR,
	RGE_NOCASE, RGE_CASE,
	RGE_NONE,
};

#define	REG_BLOCK_STD		0
#define	REG_BLOCK_RIGHT		1
#define	REG_BLOCK_LEFT		2
#define	REG_BLOCK_NONE		3

#define	REG_NQFLAG_GREED	001
#define	REG_NQFLAG_LOOP		002
#define	REG_NQFLAG_NOCASE	004

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

	enum RegCmdType	m_Type;
	DCHAR			m_SChar;
	DCHAR			m_EChar;
	DWORD			*m_Map;

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
	int					m_Flag;

	CRegExQue(void);
	~CRegExQue(void);
};

class CRegExArg : public CObject
{
public:
	int			m_SPos;
	int			m_EPos;
	CStringD	m_Str;
	BOOL		m_Del;
	BOOL		m_Done;
	CList<int, int>	m_FrArg;
	CList<int, int>	m_BkArg;
	int			m_Flag;

	const CRegExArg & operator = (CRegExArg &data);
	CRegExArg(void);
};

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

	CRegExRes() { m_Status = 0; };
};

class CRegExWork : public CObject
{
public:
	class CRegExWork	*m_Next;
	class CRegExWork	*m_List;

	CRegExRes			m_Work;
	CRegExRes			m_Done;
	CDWordArray			m_Counter;
	CStringD			m_Str;
	int					m_Pos;
	int					m_Seq;
	BOOL				m_Die;
	CRegExIdx			m_Idx;
	BOOL				m_DupDone;
	int					m_Loop;

	CRegExWork(void);
	~CRegExWork(void);
};

#define	REG_FLAG_MODEMASK	0003

#define	REG_FLAG_SIMPLE		0000
#define	REG_FLAG_WILDCARD	0001
#define	REG_FLAG_REGEX		0002

#define	REG_FLAG_ESCCHAR	0004
#define	REG_FLAG_NOCASE		0010
#define	REG_FLAG_ESCPAR		0020

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

	int			m_Arg;
	int			m_Count;
	int			m_WorkSeq;
	int			m_WorkCount;
	CList<int, int>	m_ArgStack;

	typedef struct _CompStrBuf {
		struct _CompStrBuf *m_Next;
		DCHAR		m_SaveCh;
		int			m_Pos;
		CStringD	m_Patan;
	} COMPSTRBUF;

	COMPSTRBUF	*m_CompStrBuf;
	CString		m_ErrMsg;
	int			m_CompleFlag;

	CRegEx();
	CRegEx(int CompleFlag);
	~CRegEx();

	void Init(int CompleFlag);
	void InitChar(LPCTSTR str);
	void EndofChar();
	DCHAR GetChar();
	void UnGetChar(DCHAR ch);

	CRegExNode *AllocNode(enum RegCmdType type);
	void FreeNode(CRegExNode *np);
	void RemoveAllNode();

	DCHAR CompileEscape(DCHAR ch);
	CRegExNode *CompileExtChar(DCHAR ch, CRegExNode *rp);
	void CompileReverse(CRegExNode *rp);
	CRegExNode *CompileRange(CRegExNode *rp);
	CRegExNode *CompileSub(DCHAR endc);
	CRegExNode *CompleInline(CRegExNode *rp, LPCTSTR str);
	BOOL Compile(LPCTSTR str);
	BOOL IsSimple();

	void AddQue(int sw, CRegExNode *np, CRegExWork *wp, int Flag);
	CRegExQue *HeadQue(int sw);

	BOOL MatchStr(LPCTSTR str, CRegExRes *res);
	void ConvertRes(LPCWSTR pat, CStringW &buf, CRegExRes *res);
	BOOL ConvertStr(LPCTSTR str, LPCTSTR pat, CString &buf);
	int SplitStr(LPCTSTR str, CStringArray &data);

	DCHAR RevLowerUpeer(DCHAR ch);
	void MatchCharInit();
	BOOL MatchChar(DCHAR ch, int idx, CRegExRes *res);

	static LPCTSTR SimpleRegEx(LPCTSTR str, int flag);
};
