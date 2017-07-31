#pragma once
#include "afx.h"

#define	MAPSIZE		512
#define	MAPBYTE		(MAPSIZE / 8)
#define	MAPARRAY	(MAPBYTE / sizeof(DWORD))
#define	MAPBITS		(8 * sizeof(DWORD))

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

	int		m_Type;
	int		m_SChar;
	int		m_EChar;
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
	CString		m_Str;

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
	CStringW	m_Str;
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
	CStringW			m_Str;
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

	int			m_Pos;
	CStringW	m_Patan;

	CRegEx(void);
	~CRegEx(void);

	void InitChar(LPCTSTR str);
	int GetChar();
	int GetNextChar();
	void UnGetChar();

	CRegExNode *AllocNode(int type);
	void FreeNode(CRegExNode *np);
	CRegExNode *CopyNodeSub(CRegExNode *np, CPtrArray &dups);
	CRegExNode *CopyNode(CRegExNode *np);
	void AddNode(CRegExNode *hp, int na, CPtrArray &ptr);

	int CompileEscape(int ch);
	CRegExNode *CompileRange();
	CRegExNode *CompileSub(int endc);
	BOOL Compile(LPCTSTR str);

	void AddQue(int sw, CRegExNode *np, CRegExWork *wp);
	CRegExQue *HeadQue(int sw);

	BOOL MatchStrSub(CStringW &str, int start, int end, CRegExRes *res);
	BOOL MatchStr(LPCTSTR str, CRegExRes *res);
	void ConvertRes(LPCWSTR pat, CStringW &buf, CRegExRes *res);
	BOOL ConvertStr(LPCTSTR str, LPCTSTR pat, CString &buf);
	int SplitStr(LPCTSTR str, CStringArray &data);

	void MatchCharInit();
	BOOL MatchChar(WCHAR ch, int idx, CRegExRes *res);
};
