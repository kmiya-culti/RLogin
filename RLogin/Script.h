#pragma once
#include "afx.h"
#include <afxtempl.h>
#include <afxmt.h>
#include "SyncSock.h"

#define	VALTYPE_INT		0
#define	VALTYPE_DOUBLE	1
#define	VALTYPE_INT64	2
#define	VALTYPE_STRING	3
#define	VALTYPE_WSTRING	4
#define	VALTYPE_LPBYTE	5
#define	VALTYPE_LPWCHAR	6
#define	VALTYPE_IDENT	7
#define	VALTYPE_PTR		8

class CScriptValue : public CObject
{
public:
	CString m_Index;
	int m_Type;
	union {
		int			m_Int;
		double		m_Double;
		void		*m_Ptr;
		LONGLONG	m_Int64;
	} m_Value;
	CBuffer m_Buf;
	int m_ArrayPos;
	int m_FuncPos;
	int (*m_FuncExt)(int cmd, class CScript *owner, CScriptValue &local);
	CPtrArray m_Array;
	CScriptValue *m_Left;
	CScriptValue *m_Right;
	CScriptValue *m_Child;
	CScriptValue *m_Root;
	CScriptValue *m_Next;

	const CScriptValue & operator = (CScriptValue &data);
	void operator = (class CScriptLex &lex);
	CScriptValue & operator [] (LPCSTR str);
	CScriptValue & operator [] (int index);

	const int		operator = (int value);
	const double	operator = (double value);
	const LONGLONG	operator = (LONGLONG value);
	const LPCSTR	operator = (LPCSTR str);
	const LPCWSTR	operator = (LPCWSTR str);
	const LPBYTE	operator = (LPBYTE value);
	const WCHAR *	operator = (WCHAR * value);
	const CScriptValue * operator = (CScriptValue *ptr);
	const void * operator = (void *ptr);

	void operator *= (CScriptValue &data);
	void operator /= (CScriptValue &data);
	void operator %= (CScriptValue &data);
	void operator += (CScriptValue &data);
	void operator -= (CScriptValue &data);
	void AddStr(CScriptValue &data);

	operator int ();
	operator double ();
	operator LONGLONG ();
	operator LPCSTR	();
	operator LPCWSTR ();
	operator LPBYTE ();
	operator WCHAR * ();
	operator CScriptValue * ();
	operator void * ();

	void BufCopy(CScriptValue &data);
	int Find(LPCSTR str);
	int Add(CScriptValue &data);
	int Add(LPCSTR str);
	int GetSize() { return (int)m_Array.GetSize(); }
	void ArrayCopy(CScriptValue &data);
	void RemoveAll();

	int GetType();
	CBuffer *GetBuf();
	CBuffer *GetWBuf();
	int Compare(int mode, CScriptValue *ptr);
	void Sort(int mode);

#ifdef	_DEBUG
	void Debug();
#endif

	CScriptValue();
	~CScriptValue();
};

enum CScriptLexValue
{
		LEX_EOF = 0x10000,
		LEX_INT,	LEX_DOUBLE, LEX_INT64,
		LEX_LABEL,	LEX_STRING,	LEX_WSTRING,
		LEX_ANDAND,	LEX_OROR,	LEX_INC,	LEX_DEC,
		LEX_ADDLET,	LEX_SUBLET,	LEX_MULLET,	LEX_DIVLET,	LEX_MODLET,
		LEX_ANDLET,	LEX_ORLET,	LEX_XORLET,	LEX_EQU,	LEX_UNEQU,	LEX_CATLET,
		LEX_LEFTLET,LEX_RIGHTLET,
		LEX_LEFT,	LEX_RIGHT,
		LEX_SML,	LEX_BIG,
		LEX_BREAK,	LEX_CASE,	LEX_CLASS,	LEX_CONTINUE,	LEX_DEFAULT,
		LEX_DO,		LEX_ELSE,	LEX_FOR,	LEX_FUNCTION,	LEX_IF,
		LEX_RETURN,	LEX_SWITCH,	LEX_THIS,	LEX_VAR,		LEX_WHILE,
};

class CScriptLex : public CObject
{
public:
	int m_Type;
	union {
		int			m_Int;
		LONGLONG	m_Int64;
		double		m_Double;
	} m_Data;
	struct {
		int			m_Len;
		LPBYTE		m_Ptr;
	} m_Buf;
	int m_Left;
	int m_Right;

	int operator = (int val) { return (m_Data.m_Int = val); }
	void operator = (LONGLONG val) { m_Data.m_Int64 = val; }
	void operator = (double val) { m_Data.m_Double = val; }

	operator int() { return m_Data.m_Int; }
	operator LONGLONG() { return m_Data.m_Int64; }
	operator double() { return m_Data.m_Double; }
	operator LPCSTR() { return (LPCSTR)m_Buf.m_Ptr; }
	operator LPCWSTR() { return (LPCWSTR)m_Buf.m_Ptr; }

	void SetBuf(void *pBuf, int nBufLen);
	const CScriptLex & operator = (CScriptLex &data);
	int Compare(CScriptLex &data);

	CScriptLex();
	~CScriptLex();
};

enum CScriptVCpuCode
{
		CM_LOAD = 0,	CM_CLASS,		CM_LOCAL,
		CM_VALUE,		CM_SAVE,		CM_IDENT,
		CM_PUSH,		CM_POP,
		CM_ADDARRAY,	CM_ARRAY,		CM_MENBA,
		CM_INC,			CM_DEC,
		CM_AINC,		CM_ADEC,
		CM_NEG,			CM_COM,			CM_NOT,
		CM_MUL,			CM_DIV,			CM_MOD,
		CM_ADD,			CM_SUB,			CM_CAT,
		CM_SFL,			CM_SFR,
		CM_BIG,			CM_SML,
		CM_BIGE,		CM_SMLE,
		CM_EQU,			CM_UNEQU,
		CM_AND,			CM_XOR,			CM_OR,
		CM_IF,			CM_IFN,			CM_CASE,
		CM_JMP,			CM_RET,			CM_CALL,
		CM_ADDINDEX,	CM_INDEX,
		CM_VALCLEAR,	CM_VALWITHKEY,	CM_VALARRAY,
		CM_EOF,			CM_DEBUG,
};

typedef struct _CScriptFunc
{
	char	*name;
	int		cmd;
	int		(*proc)(int cmd, class CScript *owner, CScriptValue &local);
} CScriptFunc;

class CScriptLoop : public CObject
{
public:
	int					m_StartPos;
	CDWordArray			m_BreakAddr;
	int					m_CaseAddr;
	class CScriptLoop	*m_Next;
};

class CScript : public CSyncSock
{
public:
	int m_PosOld;
	int m_SavPos;
	int m_BufOld;
	int m_BufPos;
	int m_BufMax;
	LPBYTE m_BufPtr;
	int m_UnGetChar;

	CString m_FileName;
	CScriptValue m_IncFile;
	CScriptLex m_LexTmp;
	CScriptValue *m_Local;
	CScriptValue *m_Class;
	CScriptLoop	*m_Loops;

	CArray<CScriptLex, CScriptLex &> m_LexData;
	CScriptValue m_Value;

	CScriptValue m_Data;
	CDWordArray m_Code;

	CScriptValue *m_Free;
	CScriptValue *m_Stack;

	BOOL m_ConsOpen;
	BOOL m_SyncMode;

	int	InChar(int ch, TCHAR *ptn);
	int	StrBin(int mx, TCHAR *ptn[], LPCTSTR str);

	int GetChar();
	void UnGetChar(int ch);

	int LexDigit(int ch);
	int LexEscape(int ch);
	CScriptLex *Lex();
	int LexAdd(CScriptLex *lex);

	void LoadIdent(CScriptValue *sp, int index);
	int IdentChack(CScriptLex *lex);
	int inline CodePos() { return (int)m_Code.GetSize(); }
	int inline CodeAdd(int cmd) { return (int)m_Code.Add((DWORD)cmd); }
	int inline CodeAddPos(int pos) { return (int)m_Code.Add((DWORD)pos); }
	void inline CodeSet(int pos, int cmd) { m_Code[pos] = (DWORD)cmd; }
	void inline CodeSetPos(int pos) { m_Code[pos] = (DWORD)m_Code.GetSize(); }

	void LoopPush();
	void LoopPop();
	void LoopAdd(int pos);

	CScriptLex *Stage18(CScriptLex *lex);
	CScriptLex *Stage17(CScriptLex *lex);
	CScriptLex *Stage16(CScriptLex *lex);
	CScriptLex *Stage15(CScriptLex *lex);
	CScriptLex *Stage14(CScriptLex *lex);
	CScriptLex *Stage13(CScriptLex *lex);
	CScriptLex *Stage12(CScriptLex *lex);
	CScriptLex *Stage11(CScriptLex *lex);
	CScriptLex *Stage10(CScriptLex *lex);
	CScriptLex *Stage09(CScriptLex *lex);
	CScriptLex *Stage08(CScriptLex *lex);
	CScriptLex *Stage07(CScriptLex *lex);
	CScriptLex *Stage06(CScriptLex *lex);
	CScriptLex *Stage05(CScriptLex *lex);
	CScriptLex *Stage04(CScriptLex *lex);
	CScriptLex *Stage03(CScriptLex *lex);
	CScriptLex *Stage02(CScriptLex *lex);
	CScriptLex *Stage01(CScriptLex *lex);
	CScriptLex *Stage00(CScriptLex *lex);

	void StackPush();
	void StackPop();

	static int Func08(int cmd, class CScript *owner, CScriptValue &local);
	static int Func07(int cmd, class CScript *owner, CScriptValue &local);
	static int Func06(int cmd, class CScript *owner, CScriptValue &local);
	static int Func05(int cmd, class CScript *owner, CScriptValue &local);
	static int Func04(int cmd, class CScript *owner, CScriptValue &local);
	static int Func03(int cmd, class CScript *owner, CScriptValue &local);
	static int Func02(int cmd, class CScript *owner, CScriptValue &local);
	static int Func01(int cmd, class CScript *owner, CScriptValue &local);

	void FuncInit();

	int ParseBuff(LPBYTE buf, int len);
	int LoadFile(LPCSTR filename);
	int LoadStr(LPCSTR str);

	int ExecSub(int pos, BOOL sub, CScriptValue *local);

	void OnProc(int cmd);

	CScript(class CRLoginDoc *pDoc, CWnd *pWnd);
	virtual ~CScript(void);
};
