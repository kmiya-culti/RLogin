// Data.h: CData クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATA_H__6A23DC3E_3DDC_47BD_A6FC_E0127564AE6E__INCLUDED_)
#define AFX_DATA_H__6A23DC3E_3DDC_47BD_A6FC_E0127564AE6E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
#include <afxmt.h>
#include "openssl/bn.h"
#include "openssl/ec.h"
#include "Regex.h"
#include "ChatStatDlg.h"

int	BinaryFind(void *ptr, void *tab, int size, int max, int (* func)(const void *, const void *), int *base);

#define	NIMALLOC	256

class CBuffer : public CObject
{
public:
	int m_Max;
	int m_Ofs;
	int m_Len;
	LPBYTE m_Data;

	void ReAlloc(int len);
	inline int GetSize() { return (m_Len - m_Ofs); }
	void Consume(int len);
	inline void ConsumeEnd(int len) { m_Len -= len; }
	void Apend(LPBYTE buff, int len);
	inline void Clear() { m_Len = m_Ofs = 0; }
	inline LPBYTE GetPtr() { return (m_Data + m_Ofs); }
	inline LPBYTE GetPos(int pos) { return (m_Data + m_Ofs + pos); }
	inline BYTE GetAt(int pos) { return m_Data[m_Ofs + pos]; }
	LPBYTE PutSpc(int len);
	void RoundUp(int len);

	void Put8Bit(int val);
	void Put16Bit(int val);
	void Put32Bit(int val);
	void Put64Bit(LONGLONG val);
	void PutBuf(LPBYTE buf, int len);
	void PutStr(LPCSTR str);
	void PutBIGNUM(BIGNUM *val);
	void PutBIGNUM2(BIGNUM *val);
	void PutEcPoint(const EC_GROUP *curve, const EC_POINT *point);
	inline void PutByte(int val) { Put8Bit(val); }
	void PutWord(int val);
	void PutDword(int val);

	int Get8Bit();
	int Get16Bit();
	int Get32Bit();
	LONGLONG Get64Bit();
	int GetStr(CStringA &str);
	int GetBuf(CBuffer *buf);
	int GetBIGNUM(BIGNUM *val);
	int GetBIGNUM2(BIGNUM *val);
	int GetBIGNUM_SecSh(BIGNUM *val);
	int GetEcPoint(const EC_GROUP *curve, EC_POINT *point);
	int GetDword();
	int GetWord();
	inline int GetByte() { return GetChar(); }
	int GetChar();

	inline void SET8BIT(LPBYTE pos, int val) { *pos = (BYTE)(val); }
	void SET16BIT(LPBYTE pos, int val);
	void SET32BIT(LPBYTE pos, int val);
	void SET64BIT(LPBYTE pos, LONGLONG val);

	inline int PTR8BIT(LPBYTE pos) { return (int)(*pos); }
	int PTR16BIT(LPBYTE pos);
	int PTR32BIT(LPBYTE pos);
	LONGLONG PTR64BIT(LPBYTE pos);

	LPCTSTR Base64Decode(LPCTSTR str);
	void Base64Encode(LPBYTE buf, int len);
	LPCTSTR Base16Decode(LPCTSTR str);
	void Base16Encode(LPBYTE buf, int len);
	LPCTSTR QuotedDecode(LPCTSTR str);
	void QuotedEncode(LPBYTE buf, int len);
	void md5(LPCTSTR str);

	const CBuffer & operator = (CBuffer &data) { Clear(); Apend(data.GetPtr(), data.GetSize()); return *this; }
	operator LPCSTR() { if ( m_Max <= m_Len) ReAlloc(1); m_Data[m_Len] = 0; return (LPCSTR)GetPtr(); }
	operator LPCWSTR() { if ( m_Max <= (m_Len + 1) ) ReAlloc(2); m_Data[m_Len] = 0; m_Data[m_Len + 1] = 0; return (LPCWSTR)GetPtr(); }
	operator const DWORD *() { if ( m_Max <= (m_Len + 1) ) ReAlloc(4); m_Data[m_Len] = 0; m_Data[m_Len + 1] = 0; m_Data[m_Len + 2] = 0; m_Data[m_Len + 3] = 0; return (const DWORD *)GetPtr(); }

#ifdef	_DEBUGXXX
	void RepSet();
	void Report();
	void Debug();
#endif

	CBuffer(int size);
	CBuffer();
	~CBuffer();
};

class CSpace : public CObject
{
public:
	int m_Len;
	LPBYTE m_Data;

	inline int GetSize() { return m_Len; }
	inline LPBYTE GetPtr() { return m_Data; }
	LPBYTE PutBuf(LPBYTE buf, int len)  { if ( m_Data != NULL ) delete m_Data; if ( len <= 0 ) m_Data = NULL; else { m_Data = new BYTE[len]; memcpy(m_Data, buf, len); } m_Len = len; return m_Data; }
	LPBYTE SetSize(int len) { if ( m_Data != NULL ) delete m_Data; if ( len <= 0 ) m_Data = NULL; else m_Data = new BYTE[len]; m_Len = len; return m_Data;  }

	const CSpace & operator = (CSpace &data) { PutBuf(data.GetPtr(), data.GetSize()); return *this; }

	CSpace() { m_Len = 0; m_Data = NULL; }
	~CSpace() { if ( m_Data != NULL ) delete m_Data; }
};

class CStringLoad : public CString
{
public:
	CStringLoad(int nID) { LoadString(nID); }
};

class CStringArrayExt : public CStringArray  
{
public:
	void AddVal(int value) { CString str; str.Format(_T("%d"), value); Add(str); }
	int GetVal(int index) { return _tstoi(GetAt(index)); }
	void AddBin(void *buf, int len);
	int GetBin(int index, void *buf, int len);
	void GetBuf(int index, CBuffer &buf);
	void AddArray(CStringArrayExt &stra);
	void GetArray(int index, CStringArrayExt &stra);
	void SetString(CString &str, int sep = '\t');
	void GetString(LPCTSTR str, int sep = '\t');
	void SetBuffer(CBuffer &buf);
	void GetBuffer(CBuffer &buf);
	const CStringArrayExt & operator = (CStringArrayExt &data);
	void Serialize(CArchive& ar);
	int Find(LPCTSTR str);
	int FindNoCase(LPCTSTR str);
	int Match(LPCTSTR str);
};

class CStringMaps : public CObject
{
public:
	CArray<CStringW, CStringW &> m_Data;

	int Add(LPCWSTR wstr);
	int Find(LPCWSTR wstr);
	int Next(LPCWSTR wstr, int pos);
	LPCWSTR GetAt(int n) { return m_Data[n]; }
	void RemoveAll() { m_Data.RemoveAll(); }
	int GetSize() { return (int)m_Data.GetSize(); }
	void AddWStrBuf(LPBYTE lpBuf, int nLen);

	CStringMaps();
	~CStringMaps();
};

class CStringIndex : public CObject
{
public:
	BOOL m_bNoCase;
	BOOL m_bNoSort;
	BOOL m_bString;
	BOOL m_bEmpty;
	int m_Value;
	CString m_nIndex;
	CString m_String;
	CArray<CStringIndex, CStringIndex &> m_Array;

	CStringIndex();
	~CStringIndex();

	const CStringIndex & operator = (CStringIndex &data);
	CStringIndex & operator [] (LPCTSTR str);
	CStringIndex & operator [] (int nIndex) { return m_Array[nIndex]; }
	const LPCTSTR operator = (LPCTSTR str) { m_bString = TRUE; m_bEmpty = FALSE; return (m_String = str); }
	operator LPCTSTR () const { return m_String; }
	const int operator = (int val) { m_bString = FALSE; m_bEmpty = FALSE; return (m_Value = val); }
	operator int () const { return (m_bString ? _tstoi(m_String) : m_Value); }

	BOOL IsEmpty() { return m_bEmpty; }
	int GetSize() { return m_Array.GetSize(); }
	void SetSize(int nIndex) { m_Array.SetSize(nIndex); }
	LPCTSTR GetIndex() { return m_nIndex; }
	void RemoveAll() { m_Array.RemoveAll(); }
	int Add(LPCTSTR str) { CStringIndex tmp; tmp = str; return m_Array.Add(tmp); }
	int Add(int value) { CStringIndex tmp; tmp = value; return m_Array.Add(tmp); }
	void SetNoCase(BOOL b) { m_bNoCase = b; }
	void SetNoSort(BOOL b) { m_bNoSort = b; }

	int Find(LPCTSTR str);
	void SetArray(LPCTSTR str);

	void GetBuffer(CBuffer *bp);
	void SetBuffer(CBuffer *bp);
	void GetString(LPCTSTR str);
	void SetString(CString &str);

	void SetPackStr(CStringA &mbs);
	LPCTSTR GetPackStr(LPCTSTR str);
	BOOL ReadString(CArchive& ar, CString &str);
	void Serialize(CArchive& ar, LPCSTR base = NULL);
};

class CStringBinary : public CObject
{
public:
	CStringBinary *m_pLeft;
	CStringBinary *m_pRight;
	CString m_Index;
	CString m_String;
	int m_Value;

	CStringBinary();
	CStringBinary(LPCTSTR str);
	~CStringBinary();

	void RemoveAll();
	CStringBinary * Find(LPCTSTR str);
	CStringBinary * FindValue(int value);
	CStringBinary & operator [] (LPCTSTR str);
	CStringBinary & Add(LPCTSTR str) { return (*this)[str]; };
	const LPCTSTR operator = (LPCTSTR str) { m_Value = 0; return (m_String = str); }
	operator LPCTSTR () const { return m_String; }
	const int operator = (int val) { return (m_Value = val); }
	operator int () const { return m_Value; }
};

#define	PARA_NOT	0xFFFFFFFF
#define	PARA_MAX	0x7FFFFFFF

class CParaIndex : CObject
{
public:
	DWORD m_Data;
	CArray<CParaIndex, CParaIndex &> m_Array;

	CParaIndex();
	CParaIndex(DWORD data);
	~CParaIndex();

	const CParaIndex & operator = (CParaIndex &data);
	const DWORD operator = (DWORD data) { m_Data = data; return m_Data; }
	CParaIndex & operator [] (int nIndex) { return m_Array[nIndex]; }
	operator DWORD () const { return m_Data; }
	INT_PTR Add(DWORD data) { CParaIndex tmp(data); return m_Array.Add(tmp); }
	INT_PTR GetSize() { return m_Array.GetSize(); }
	void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1) { m_Array.SetSize(nNewSize, nGrowBy); }
	void RemoveAll() { m_Array.RemoveAll(); }
};

class CBmpFile : public CObject
{
public:
	IPicture *m_pPic;
	CBitmap m_Bitmap;
	int m_Width, m_Height;
	CString m_FileName;

	int LoadFile(LPCTSTR filename);
	CBitmap *GetBitmap(CDC *pDC, int width, int height);

	CBmpFile();
	virtual ~CBmpFile();
};

class CFontChacheNode : public CObject
{
public:
	CFont *m_pFont;
	LOGFONT m_LogFont;
	CFontChacheNode *m_pNext;
	int m_Style;
	int m_KanWidMul;

	CFont *Open(LPCTSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality);
	CFontChacheNode();
	~CFontChacheNode();
};

#define	FONTCACHEMAX	32

class CFontChache : public CObject
{
public:
	CFontChacheNode *m_pTop[4];
	CFontChacheNode m_Data[FONTCACHEMAX];

	CFontChacheNode *GetFont(LPCTSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality, int Hash);
	CFontChache();
};

class CMutexLock : public CObject
{
public:
	CMutex *m_pMutex;
	CSingleLock *m_pLock;
	CMutexLock(LPCTSTR lpszName = NULL); 
	~CMutexLock();
};

class COptObject : public CObject
{
public:
	LPCTSTR m_pSection;
	int m_MinSize;

	virtual void Init();
	virtual void SetArray(CStringArrayExt &stra) = 0;
	virtual void GetArray(CStringArrayExt &stra) = 0;

	virtual void Serialize(int mode);					// To Profile
	virtual void Serialize(int mode, CBuffer &buf);		// To CBuffer
	virtual void Serialize(CArchive &ar);				// To Archive

	COptObject();
};

class CStrScriptNode : public CObject
{
public:
	class CStrScriptNode	*m_Left;
	class CStrScriptNode	*m_Right;

	CRegEx		m_Reg;
	CString		m_RecvStr;
	CString		m_SendStr;

	CStrScriptNode();
	~CStrScriptNode();
};

#define	SCPSTAT_NON		0
#define	SCPSTAT_EXEC	1
#define	SCPSTAT_MAKE	2

class CStrScript : public CObject
{
public:
	CStrScriptNode	*m_Node;
	CStrScriptNode	*m_Exec;

	CStringW		m_Str;
	CRegExRes		m_Res;

	CChatStatDlg	m_StatDlg;
	BOOL			m_MakeChat;
	BOOL			m_MakeFlag;
	CStringW		m_Line[3];

	CString			m_LexTmp;

	void Init();
	CStrScriptNode *CopyNode(CStrScriptNode *np);

	void SetNode(CStrScriptNode *np, CBuffer &buf);
	CStrScriptNode *GetNode(CBuffer &buf);
	void SetBuffer(CBuffer &buf);
	int GetBuffer(CBuffer &buf);

	LPCTSTR QuoteStr(CString &tmp, LPCTSTR str);
	void SetNodeStr(CStrScriptNode *np, CString &str, int nst);
	int GetLex(LPCTSTR &str);
	CStrScriptNode *GetNodeStr(int &lex, LPCTSTR &str);
	void SetString(CString &str);
	void GetString(LPCTSTR str);

	void EscapeStr(LPCWSTR str, CString &buf, BOOL reg = FALSE);
	void AddNode(LPCWSTR recv, LPCWSTR send);

	int Status();
	void ExecInit();
	void ExecStop();

	void ExecNode(CStrScriptNode *np);
	LPCWSTR ExecChar(int ch);

	void SendStr(LPCWSTR str, int len, class CServerEntry *ep = NULL);

	void SetTreeNode(CStrScriptNode *np, CTreeCtrl &tree, HTREEITEM own);
	void SetTreeCtrl(CTreeCtrl &tree);

	CStrScriptNode *GetTreeNode(CTreeCtrl &tree, HTREEITEM hti);
	void GetTreeCtrl(CTreeCtrl &tree);

	const CStrScript & operator = (CStrScript &data);

	CStrScript();
	~CStrScript();
};

class CServerEntry : public CObject
{
public:
	CString m_EntryName;
	CString m_HostName;
	CString m_PortName;
	CString m_UserName;
	CString m_PassName;
	CString m_TermName;
	CString m_IdkeyName;
	int m_KanjiCode;
	int m_ProtoType;
	CBuffer m_ProBuffer;
	BOOL m_SaveFlag;
	BOOL m_CheckFlag;
	int m_Uid;
	CStrScript m_ChatScript;
	int m_ProxyMode;
	CString m_ProxyHost;
	CString m_ProxyPort;
	CString m_ProxyUser;
	CString m_ProxyPass;
	CString m_Memo;
	CString m_Group;
	CString m_ScriptFile;
	CString m_ScriptStr;
	CString m_HostNameProvs;
	CString m_UserNameProvs;
	CString m_PassNameProvs;
	CString m_ProxyHostProvs;
	CString m_ProxyUserProvs;
	CString m_ProxyPassProvs;

	void Init();
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);
	void ScriptInit(int cmds, int shift, class CScriptValue &value);
	void ScriptValue(int cmds, class CScriptValue &value, int mode);
	void SetBuffer(CBuffer &buf);
	int GetBuffer(CBuffer &buf);
	void SetProfile(LPCTSTR pSection);
	int GetProfile(LPCTSTR pSection, int Uid);
	void DelProfile(LPCTSTR pSection);
	void Serialize(CArchive &ar);
	void SetIndex(int mode, CStringIndex &index);

	LPCTSTR GetKanjiCode();
	void SetKanjiCode(LPCTSTR str);
	LPCTSTR GetProtoName();
	int GetProtoType(LPCTSTR str);

	const CServerEntry & operator = (CServerEntry &data);
	CServerEntry();
};

class CServerEntryTab : public COptObject
{
public:
	CArray<CServerEntry, CServerEntry &> m_Data;

	void Init();
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);
	void Serialize(int mode);

	int AddEntry(CServerEntry &Entry);
	void UpdateAt(int nIndex);
	void RemoveAt(int nIndex);

	inline CServerEntry &GetAt(int nIndex) { return m_Data[nIndex]; }
	inline INT_PTR GetSize() { return m_Data.GetSize(); }

	CServerEntryTab();
};

#define	MASK_SHIFT	00001
#define	MASK_CTRL	00002
#define	MASK_ALT	00004
#define	MASK_APPL	00010
#define	MASK_CKM	00020
#define	MASK_VT52	00040
//#define	MASK_NUMLCK	00100
//#define	MASK_SCRLCK	00200
//#define	MASK_CAPLCK	00400

class CKeyNode : public CObject
{
public:
	int m_Code;
	int m_Mask;
	CBuffer m_Maps;
	CString m_Temp;

	void SetKeyMap(LPCTSTR str, int type, char map[256]);
	LPCTSTR GetMaps();
	void SetMaps(LPCTSTR str);
	LPCTSTR GetCode();
	void SetCode(LPCTSTR name);
	LPCTSTR GetMask();
	void CommandLine(LPCWSTR str, CStringW &cmd);
	void SetComboList(CComboBox *pCombo);

	const CKeyNode & operator = (CKeyNode &data);

	CKeyNode();
};

class CKeyCmds : public CObject
{
public:
	int m_Id;
	BOOL m_Flag;
	CString m_Menu;
	CString m_Text;

	void ResetMenu(CMenu *pMenu);
	const CKeyCmds & operator = (CKeyCmds &data);
};

class CKeyCmdsTab : public CObject
{
public:
	CArray<CKeyCmds, CKeyCmds &> m_Data;

	int Add(CKeyCmds &cmds);
	int Find(int id);
	void ResetMenuAll(CMenu *pMenu);

	inline CKeyCmds & operator [] (int nIndex) { return m_Data[nIndex]; }
	inline int GetSize() { return (int)m_Data.GetSize(); }
	inline void SetSize(int sz) { 	m_Data.SetSize(sz); }
	inline void RemoveAt(int pos) { m_Data.RemoveAt(pos); }
	inline void RemoveAll() { m_Data.RemoveAll(); }

	const CKeyCmdsTab & operator = (CKeyCmdsTab &data);
};

class CKeyNodeTab : public COptObject
{
public:
	BOOL m_CmdsInit;
	CKeyCmdsTab m_Cmds;
	CArray<CKeyNode, CKeyNode &> m_Node;

	void Init();
	void SetArray(CStringArrayExt &stra);
	void GetArray(CStringArrayExt &stra);
	void SetIndex(int mode, CStringIndex &index);
	void ScriptInit(int cmds, int shift, class CScriptValue &value);
	void ScriptValue(int cmds, class CScriptValue &value, int mode);

	BOOL Find(int code, int mask, int *base);
	int Add(CKeyNode &node);
	int Add(int code, int mask, LPCTSTR str);
	int Add(LPCTSTR code, int mask, LPCTSTR maps);
	BOOL FindKeys(int code, int mask, CBuffer *pBuf, int base, int bits);
	BOOL FindMaps(int code, int mask, CBuffer *pBuf);
	BOOL FindCapInfo(LPCTSTR name, CBuffer *pBuf);

	inline CKeyNode &GetAt(int pos) { return m_Node[pos]; }
	inline int GetSize() { return (int)m_Node.GetSize(); }
	inline void SetSize(int sz) { 	m_Node.SetSize(sz, 16); }
	inline void RemoveAt(int pos) { m_Node.RemoveAt(pos); m_CmdsInit = FALSE; }
	inline CKeyNode & operator[](int nIndex) { return m_Node[nIndex]; }

	const CKeyNodeTab & operator = (CKeyNodeTab &data);
	CKeyNodeTab();

	void BugFix(int fix);
	void CmdsInit();
	static int GetCmdsKey(LPCWSTR str);
	static void SetComboList(CComboBox *pCombo);
	static int GetDecKeyToCode(int code);
};

class CKeyMac : public CObject
{
public:
	int m_Len;
	LPBYTE m_Data;

	LPBYTE GetPtr() { return m_Data; }
	int GetSize() { return m_Len; }

	void GetMenuStr(CString &str);
	void GetStr(CString &str);
	void SetStr(LPCTSTR str);
	void SetBuf(LPBYTE buf, int len);

	BOOL operator == (CKeyMac &data);
	const CKeyMac & operator = (CKeyMac &data);
	CKeyMac();
	~CKeyMac();
};

class CKeyMacTab : public COptObject
{
public:
	CArray<CKeyMac, CKeyMac &> m_Data;

	void Init();
	void GetArray(CStringArrayExt &stra);
	void SetArray(CStringArrayExt &stra);
	void SetIndex(int mode, CStringIndex &index);
	void ScriptInit(int cmds, int shift, class CScriptValue &value);
	void ScriptValue(int cmds, class CScriptValue &value, int mode);

	void Top(int nIndex);
	void Add(CKeyMac &tmp);
	void GetAt(int nIndex, CBuffer &buf);
	void SetHisMenu(CWnd *pWnd);

	inline CKeyMac & operator[](int nIndex) { return m_Data[nIndex]; }
	const CKeyMacTab & operator = (CKeyMacTab &data);
	CKeyMacTab();
};

#define	PFD_LOCAL		0
#define	PFD_SOCKS		1
#define	PFD_REMOTE		2

#define	SEL_IPV6V4		0
#define	SEL_IPV6		1
#define	SEL_IPV4		2

class CParamTab : public COptObject
{
public:
	CString m_IdKeyStr[9];
	CStringArrayExt m_AlgoTab[12];
	CStringArrayExt m_PortFwd;
	CStringArrayExt m_IdKeyList;
	CString m_XDisplay;
	CString m_ExtEnvStr;
	DWORD m_OptTab[8];
	CString m_Reserve;
	int m_SelIPver;

	CParamTab();
	void Init();
	void GetArray(CStringArrayExt &stra);
	void SetArray(CStringArrayExt &stra);
	void SetIndex(int mode, CStringIndex &index);
	void ScriptInit(int cmds, int shift, class CScriptValue &value);
	void ScriptValue(int cmds, class CScriptValue &value, int mode);

	void GetProp(int num, CString &str, int shuffle = FALSE);
	int GetPropNode(int num, int node, CString &str);

	const CParamTab & operator = (CParamTab &data);
};

#endif // !defined(AFX_DATA_H__6A23DC3E_3DDC_47BD_A6FC_E0127564AE6E__INCLUDED_)
