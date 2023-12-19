//////////////////////////////////////////////////////////////////////
// Script.h

#pragma once

#include "afx.h"
#include <afxtempl.h>
#include <afxmt.h>
#include "RegEx.h"
#include "ScriptDlg.h"

#define	VALTYPE_INT			0
#define	VALTYPE_DOUBLE		1
#define	VALTYPE_COMPLEX		2
#define	VALTYPE_INT64		3
#define	VALTYPE_STRING		4
#define	VALTYPE_WSTRING		5
#define	VALTYPE_DSTRING		6
#define	VALTYPE_LPBYTE		7
#define	VALTYPE_LPWCHAR		8
#define	VALTYPE_LPDCHAR		9
#define	VALTYPE_LPDDOUB		10
#define	VALTYPE_IDENT		11
#define	VALTYPE_PTR			12
#define	VALTYPE_EMPTY		13

#define	DATA_BUF_NONE		0
#define	DATA_BUF_BOTH		1
#define	DATA_BUF_HAVE		2
#define	DATA_BUF_LIMIT		(128 * 1024)

#define	LOOP_COUNT			300

#define	DOC_MODE_SAVE		0
#define	DOC_MODE_IDENT		1
#define	DOC_MODE_CALL		2

#define	FUNC_RET_NOMAL		0
#define	FUNC_RET_ABORT		1
#define	FUNC_RET_EXIT		2
#define	FUNC_RET_EVENT		3

#define	SCP_EVENT_TIMER		0001
#define	SCP_EVENT_CONS		0002
#define	SCP_EVENT_SOCK		0004
#define	SCP_EVENT_CONNECT	0010
#define	SCP_EVENT_CLOSE		0020
#define	SCP_EVENT_DIALOG	0040

#ifdef	_UNICODE
#define	VALTYPE_TSTRING	VALTYPE_WSTRING
#else
#define	VALTYPE_TSTRING	VALTYPE_STRING
#endif

#define	DOUBLE_ZERO			1e-20

#define	PTRTYPE_NONE		0
#define	PTRTYPE_FILE		1
#define	PTRTYPE_PIPE		2

typedef struct _ScriptCmdsDefs {
	LPCTSTR		name;
	int			cmds;
} ScriptCmdsDefs;

class CComplex : public CObject
{
public:
	double	re;
	double	im;

	inline const CComplex & operator = (double value) { re = value; im = 0.0; return *this; }
	inline const CComplex & operator = (CComplex &com) { re = com.re; im = com.im; return *this; }
	const CComplex & operator = (LPCTSTR str);

	inline const CComplex & Com(double rev, double imv) { re = rev; im = imv; return *this; }
	inline const double Abs() { return sqrt(pow(re, 2) + pow(im, 2)); }
	inline const double Arg() { return atan2(im, re); }

	void operator += (CComplex &com);
	void operator -= (CComplex &com);
	void operator *= (CComplex &com);
	void operator /= (CComplex &com);

	int Compare(CComplex &com);

	CComplex();
	CComplex(const CComplex &com) { re = com.re; im = com.im; }
	CComplex(double rev, double imv) { re = rev; im = imv; }
	~CComplex();
};

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
	CComplex m_Complex;
	int m_PtrType;
	CBuffer m_Buf;
	CBuffer m_Work;
	int m_ArrayPos;
	DWORD m_FuncPos;
	int m_SrcTextPos;
	int (CScript::*m_FuncExt)(int cmd, CScriptValue &local);
	CPtrArray m_Array;
	CScriptValue *m_Left;
	CScriptValue *m_Right;
	CScriptValue *m_Child;
	CScriptValue *m_Root;
	CScriptValue *m_Next;
	int m_DocCmds;
	BOOL m_bNoCase;

	const CScriptValue & operator = (CScriptValue &data);
	void operator = (class CScriptLex &lex);
	CScriptValue & operator [] (LPCTSTR str);
	CScriptValue & operator [] (int index);
	CScriptValue & GetAt(LPCTSTR str);
	CScriptValue & GetAt(int index);

	const int		operator = (int value);
	const double	operator = (double value);
	const CComplex &operator = (CComplex &com);
	const LONGLONG	operator = (LONGLONG value);
	const LPCSTR	operator = (LPCSTR str);
	const LPCWSTR	operator = (LPCWSTR str);
	const LPCDSTR	operator = (LPCDSTR str);
	const LPBYTE	operator = (LPBYTE value);
	const WCHAR *	operator = (WCHAR * value);
	const DCHAR *	operator = (DCHAR * value);
	const double *	operator = (double * value);
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
	operator CComplex & ();
	operator LONGLONG ();
	operator LPCSTR	();
	operator LPCWSTR ();
	operator LPCDSTR ();
	operator LPBYTE ();
	operator WCHAR * ();
	operator DCHAR * ();
	operator double * ();
	operator CScriptValue * ();
	operator void * ();

	void BufCopy(CScriptValue &data);
	int Find(LPCTSTR str);
	int FindAt(LPCTSTR str, int pos);
	int Add(CScriptValue &data);
	int Add(LPCTSTR str);
	int GetSize() { return (int)m_Array.GetSize(); }
	void ArrayCopy(CScriptValue &data);
	void RemoveAll();

	int GetType();
	int GetPtrType();
	CBuffer *GetBuf();
	CBuffer *GetWBuf();
	CBuffer *GetDBuf();
	void GetName(CStringA &name);
	int GetDocCmds();
	int Compare(int mode, CScriptValue *ptr);
	void Sort(int mode);

	void SetPtrType(int type);
	void SetArray(CStringArrayExt &ary, int mode);
	void SetStr(CString &str, int mode);
	void SetInt(int &val, int mode);
	void SetNodeStr(LPCTSTR node, CString &str, int mode);
	void SetNodeInt(LPCTSTR node, int &val, int mode);
	void SetIndex(CStringIndex &index);
	void GetIndex(CStringIndex &index);

	void Debug(CString &str);
	void Serialize(int mode, CBuffer *bp);
	void Empty() { m_Type = VALTYPE_EMPTY; }
	void *ThrowNullPtr(void *ptr);

	CScriptValue();
	~CScriptValue();
};

enum CScriptLexValue
{
		LEX_EOF = 0x10000,
		LEX_INT,	LEX_DOUBLE, LEX_INT64,	LEX_IMG,
		LEX_LABEL,	LEX_LABELA,	LEX_STRING,	LEX_WSTRING,
		LEX_ANDAND,	LEX_OROR,	LEX_INC,	LEX_DEC,
		LEX_ADDLET,	LEX_SUBLET,	LEX_MULLET,	LEX_DIVLET,	LEX_MODLET,
		LEX_ANDLET,	LEX_ORLET,	LEX_XORLET,	LEX_EQU,	LEX_UNEQU,	LEX_CATLET,
		LEX_LEFTLET,LEX_RIGHTLET,
		LEX_LEFT,	LEX_RIGHT,
		LEX_SML,	LEX_BIG,
		LEX_BREAK,	LEX_CASE,	LEX_CLASS,	LEX_CONTINUE,	LEX_DEFAULT,
		LEX_DO,		LEX_ELSE,	LEX_FOR,	LEX_FOREACH,	LEX_FUNCTION,
		LEX_IF,		LEX_IN,		LEX_RETURN,	LEX_SWITCH,		LEX_THIS,
		LEX_VAR,	LEX_WHILE,
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
	CComplex m_Complex;
	int m_Left;
	int m_Right;

	int operator = (int val) { return (m_Data.m_Int = val); }
	void operator = (LONGLONG val) { m_Data.m_Int64 = val; }
	void operator = (double val) { m_Data.m_Double = val; }
	void operator = (CComplex & com) { m_Complex = com; }

	operator int() { return m_Data.m_Int; }
	operator LONGLONG() { return m_Data.m_Int64; }
	operator double() { return m_Data.m_Double; }
	operator CComplex & () { return m_Complex; }
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
		CM_LOAD = 0,	CM_CLASS,		CM_LOCAL,	CM_SYSTEM,
		CM_VALUE,		CM_SAVE,		CM_IDENT,
		CM_IPUSH,		CM_PUSH,		CM_POP,
		CM_ADDARRAY,	CM_ARRAY,		CM_MENBA,
		CM_INT,			CM_STR,
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
		CM_FOREACH,		CM_EACHWITHKEY,
};

typedef struct _ScriptFunc
{
	LPCTSTR	name;
	int		cmd;
	int		(CScript::*proc)(int cmd, CScriptValue &local);
} ScriptFunc;

typedef struct _CodeStack {
	struct _CodeStack *next;
	CString name;
	CScriptValue *local;
	int mask;
	int pos;
	BOOL use;
} CodeStack;

class CScriptLoop : public CObject
{
public:
	int					m_StartPos;
	CDWordArray			m_BreakAddr;
	int					m_CaseAddr;
	class CScriptLoop	*m_Next;
};

typedef struct _ScriptDebug {
	DWORD	code;
	DWORD	spos, epos;
} ScriptDebug;

class CTempWnd : public CWnd
{
public:
	CTempWnd();
	virtual ~CTempWnd();

public:
	CScript *m_pScript;
	CScriptValue *m_pValue;
	CFont m_Font;

	void FuncCall(int nID);

protected:
	virtual void PostNcDestroy();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
};

class CScriptCmdUI : public CCmdUI
{
public:
	int m_Check;
	BOOL m_bEnable;
	BOOL m_bRadio;
	CString m_Text;

	CScriptCmdUI();

	virtual void Enable(BOOL bOn = TRUE);
	virtual void SetCheck(int nCheck = 1);
	virtual void SetRadio(BOOL bOn = TRUE);
	virtual void SetText(LPCTSTR lpszText);
};

class CScript : public CObject
{
public:
	int m_SavPos;
	int m_PosOld;
	int m_PosBtm;
	int m_BufOld;
	int m_BufBtm;
	int m_BufPos;
	int m_BufMax;
	LPCTSTR m_BufPtr;
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

	int m_CodePos;
	CScriptValue *m_Free;
	CScriptValue *m_Stack;
	CScriptValue *m_ExecLocal;
	CodeStack *m_CodeStack;
	class CScript *m_pNext;
	CString m_FormatStr;

	int	InChar(int ch, CHAR *ptn);
	int	StrBin(int mx, const TCHAR *ptn[], LPCTSTR str);

	int GetChar();
	void UnGetChar(int ch);

	int LexDigit(int ch);
	int LexEscape(int ch, CStringW *save);
	CScriptLex *Lex();
	int LexAdd(CScriptLex *lex);

	void LoadIdent(CScriptValue *sp, int index, int cmd);
	int IdentChack(CScriptLex *lex);
	inline int CodePos() { return (int)m_Code.GetSize(); }
	inline int CodeAdd(int cmd) { return (int)m_Code.Add((DWORD)cmd); }
	inline int CodeAddPos(int pos) { return (int)m_Code.Add((DWORD)pos); }
	inline void CodeSet(int pos, int cmd) { m_Code[pos] = (DWORD)cmd; }
	inline void CodeSetPos(int pos) { m_Code[pos] = (DWORD)m_Code.GetSize(); }

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
	CScriptLex *Stage061(CScriptLex *lex);
	CScriptLex *Stage06(CScriptLex *lex);
	CScriptLex *Stage05(CScriptLex *lex);
	CScriptLex *Stage04(CScriptLex *lex);
	CScriptLex *Stage03(CScriptLex *lex);
	CScriptLex *Stage02(CScriptLex *lex);
	CScriptLex *Stage01(CScriptLex *lex);
	CScriptLex *Stage00(CScriptLex *lex);

	void StackPush();
	void StackPop();

	void CodeStackPush(BOOL use, LPCTSTR name = NULL);
	void CodeStackPop();
	int TekStyle(int idx, CScriptValue &local);

	LPCTSTR Format(CScriptValue &local, int base);

	int ComFunc(int cmd, CScriptValue &local);
	int TextWnd(int cmd, CScriptValue &local);
	int DataBs(int cmd, CScriptValue &local);
	int Dialog(int cmd, CScriptValue &local);
	int TekGrp(int cmd, CScriptValue &local);
	int Func10(int cmd, CScriptValue &local);
	int Func09(int cmd, CScriptValue &local);
	int Func08(int cmd, CScriptValue &local);
	int Func07(int cmd, CScriptValue &local);
	int Func06(int cmd, CScriptValue &local);
	int Func05(int cmd, CScriptValue &local);
	int Func04(int cmd, CScriptValue &local);
	int Func03(int cmd, CScriptValue &local);
	int Func02(int cmd, CScriptValue &local);
	int Func01(int cmd, CScriptValue &local);

	void FuncInit();

	int ParseBuff(LPCTSTR buf, int len);
	int LoadFile(LPCTSTR filename);
	int LoadStr(LPCTSTR str);

	int Exec();
	void Stop();
	void Abort();
	int ExecFile(LPCTSTR filename);
	int ExecStr(LPCTSTR addstr, int num = (-1));
	BOOL OnIdle();

	int Call(LPCTSTR func, CScriptValue *local);
	int Call(LPCTSTR func) { return Call(func, NULL); }

	class CRLoginDoc *m_pDoc;
	BOOL m_bOpenSock;
	int m_EventFlag;
	int m_EventMask;
	int m_EventMode;

	int SetEvent(int flag);
	void ResetEvent(int flag);
	void SetDocument(class CRLoginDoc *pDoc);

	struct {
		CString str;
		CString func;
		int pos;
	} m_MenuTab[10];

	void SetMenu(CMenu *pMenu);

	int m_ConsMode;
	int m_SockMode;
	CBuffer m_ConsBuff;
	CBuffer m_SockBuff;
	int m_RegMatch;
	CPtrArray m_RegData;
	int m_RegFlag;
	BOOL m_bConsOut;
	BOOL m_bAbort;
	volatile BOOL m_bInit;
	volatile BOOL m_bExec;

	BOOL IsConsOver();
	void SetConsBuff(LPBYTE buf, int len);
	BOOL IsSockOver();
	void SetSockBuff(LPBYTE buf, int len);
	void PutConsOut(LPBYTE buf, int len);

	void OnSockChar(int ch);
	void OnConnect();
	void OnClose();
	void OnTimer(UINT_PTR nIDEvent);
	inline BOOL IsExec() { return (m_CodePos >= 0 && m_CodePos < m_Code.GetSize() ? TRUE : FALSE); }
	inline BOOL IsConnect() { return (m_bAbort || m_pDoc->m_pSock == NULL || !m_pDoc->m_pSock->m_bConnect ? FALSE : TRUE); }

	CScriptDlg *m_pSrcDlg;
	DWORD m_SrcMax;
	LPTSTR m_pSrcText;
	ScriptDebug m_SrcWork;
	CArray<ScriptDebug, ScriptDebug &> m_SrcCode;

	void SetSrcMark(BOOL bBtm);
	void OpenDebug(LPCTSTR filename = NULL);

	CScript();
	virtual ~CScript(void);

	static void ScriptSystemInit(LPCTSTR path, LPCTSTR prog, LPCTSTR work);
};
