#pragma once

#include <afxtempl.h>

//////////////////////////////////////////////////////////////////////
// CSizeStr

class CSizeStr : CObject
{
public:
	WORD m_Type;
	WORD m_Value;
	CString m_String;

	void SetIndex(int mode, class CStringIndex &index);
	void Serialize(int mode, class CBuffer &buf);

	inline void Empty() { m_Type = m_Value = 0; m_String.Empty(); }
	inline BOOL IsEmpty() { return (m_Type == 0xFFFF || m_String.IsEmpty() ? TRUE : FALSE); }

	const CSizeStr & operator = (CSizeStr &data);
	BOOL operator == (CSizeStr &data);

	CSizeStr();
	~CSizeStr();
};

//////////////////////////////////////////////////////////////////////
// CDlgInitData

class CDlgInitData : CObject
{
public:
	UINT	m_ctrlID;
	UINT	m_Message;
	CString m_String;

	void SetIndex(int mode, class CStringIndex &index);
	void Serialize(int mode, class CBuffer &buf);

	const CDlgInitData & operator = (CDlgInitData &data);

	CDlgInitData();
	~CDlgInitData();
};

//////////////////////////////////////////////////////////////////////
// CDlgItemData

class CDlgItemData : CObject
{
public:
	DWORD     m_helpID;
	DWORD     m_exStyle;
	DWORD     m_style;
	short     m_x;
	short     m_y;
	short     m_cx;
	short     m_cy;
	DWORD     m_id;
	CSizeStr  m_windowClass;
	CSizeStr  m_title;
	WORD      m_extraCount;
	BYTE      *m_extraData;

	void SetIndex(int mode, class CStringIndex &index);
	void Serialize(int mode, class CBuffer &buf, BOOL bExt);

	const CDlgItemData & operator = (CDlgItemData &data);

	CDlgItemData();
	~CDlgItemData();
};

//////////////////////////////////////////////////////////////////////
// CResMenuData

class CResMenuData : CObject
{
public:
	WORD m_Option;
	WORD m_Id;
	CString	m_String;

	const CResMenuData & operator = (CResMenuData &data);

	CResMenuData();
	~CResMenuData();
};

//////////////////////////////////////////////////////////////////////
// CResBaseNode
// CResStringBase
// CResMenuBase
// CResToolBarBase
// CDlgTempBase

class CResBaseNode : CObject
{
public:
	WORD m_ResId;

	CResBaseNode();
};

class CResStringBase : public CResBaseNode
{
public:
	CString	m_String;

	void Serialize(int mode, class CBuffer &buf);

	const CResStringBase & operator = (CResStringBase &data);

	CResStringBase();
	~CResStringBase();
};

class CResMenuBase : public CResBaseNode
{
public:
	WORD m_Version;
	WORD m_Offset;
	CArray<CResMenuData, CResMenuData &> m_Data;

	void SetIndex(int mode, class CStringIndex &index);
	void Serialize(int mode, class CBuffer &buf);
	void CopyData(CResMenuBase &data);

	HMENU GetMenuHandle();

	const CResMenuBase & operator = (CResMenuBase &data);

	CResMenuBase();
	~CResMenuBase();
};

class CResToolBarBase : public CResBaseNode
{
public:
	WORD m_Version;
	WORD m_Width;
	WORD m_Height;
	CWordArray m_Item;

	void SetIndex(int mode, class CStringIndex &index);
	void Serialize(int mode, class CBuffer &buf);

	BOOL SetToolBar(class CToolBarEx &ToolBar, CWnd *pWnd);

	const CResToolBarBase & operator = (CResToolBarBase &data);

	CResToolBarBase();
	~CResToolBarBase();
};

class CDlgTempBase : public CResBaseNode
{
public:
	WORD      m_dlgVer;
	WORD      m_signature;
	DWORD     m_helpID;
	DWORD     m_exStyle;
	DWORD     m_style;
	WORD      m_cDlgItems;
	short     m_x;
	short     m_y;
	short     m_cx;
	short     m_cy;
	CSizeStr  m_menu;
	CSizeStr  m_windowClass;
	CString	  m_title;
	WORD      m_pointsize;
	WORD      m_weight;
	BYTE      m_italic;
	BYTE      m_charset;
	CString   m_typeface;

	BOOL	m_bInst;
	HGLOBAL m_hTemplate;
	HGLOBAL m_hInitData;
	CArray<CDlgItemData, CDlgItemData &> m_Item;
	CArray<CDlgInitData, CDlgInitData &> m_Init;

	void SetIndex(int mode, class CStringIndex &index);
	void Serialize(int mode, class CBuffer &buf);
	void CopyData(CDlgTempBase &data);

	HGLOBAL GetDialogHandle();
	HGLOBAL GetInitDataHandle();
	BOOL LoadResource(LPCTSTR lpszName);

	const CDlgTempBase & operator = (CDlgTempBase &data);

	CDlgTempBase();
	~CDlgTempBase();
};

class CResBitmapBase : public CResBaseNode
{
public:
	HBITMAP m_hBitmap;
	CString m_FileName;

	void SetIndex(int mode, class CStringIndex &index);
	void Serialize(int mode, class CBuffer &buf);

	BOOL SetBitmap(CBitmap &bitmap);
	BOOL LoadBitmap(WORD id, LPCTSTR filename);

	inline BOOL IsUserMap() { return (!m_FileName.IsEmpty() ? TRUE : FALSE); }

	const CResBitmapBase & operator = (CResBitmapBase &data);

	CResBitmapBase();
	~CResBitmapBase();
};

//////////////////////////////////////////////////////////////////////
// CResDataBase

#define	RESFILE_TYPE	5	// 4 = OEM, 5 = UTF-8

class CResDataBase : CObject
{
public:
	LANGID m_LangId;
	CString m_Transrate;
	CString m_Language;
	CArray<CDlgTempBase, CDlgTempBase &> m_Dialog;
	CArray<CResStringBase, CResStringBase &> m_String;
	CArray<CResMenuBase, CResMenuBase &> m_Menu;
	CArray<CResToolBarBase, CResToolBarBase &> m_ToolBar;
	CArray<CResBitmapBase, CResBitmapBase &> m_Bitmap;

	void Serialize(int mode, class CBuffer &buf);

	BOOL LoadFile(LPCTSTR pFilename);
	BOOL SaveFile(LPCTSTR pFilename);

	void InitRessource();
	BOOL InitToolBarBitmap(LPCTSTR lpszName, UINT ImageId);

	BOOL AddDialog(LPCTSTR lpszName);
	BOOL AddString(LPCTSTR lpszName);
	BOOL AddMenu(LPCTSTR lpszName);
	BOOL AddToolBar(LPCTSTR lpszName);
	BOOL AddBitmap(LPCTSTR lpszName);

	BOOL LoadResDialog(LPCTSTR lpszName, HGLOBAL &hTemplate, HGLOBAL &hInitData);
	BOOL LoadResMenu(LPCTSTR lpszName, HMENU &hMenu);

	BOOL LoadResString(LPCTSTR lpszName, CString &str);
	BOOL LoadResToolBar(LPCTSTR lpszName, class CToolBarEx &ToolBar, CWnd *pWnd);
	BOOL LoadResBitmap(LPCTSTR lpszName, CBitmap &Bitmap);

	void *Find(WORD nId, LPCTSTR type);
	void Add(WORD nId, LPCTSTR type, void *pBase);

	BOOL IsTranslateString(LPCTSTR str);
	int MakeTranslateTable(CTranslateStringTab &strTab);

	const CResDataBase & operator = (CResDataBase &data);

	CResDataBase();
	~CResDataBase();
};
