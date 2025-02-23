#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "Data.h"
#include "ResDataBase.h"

//////////////////////////////////////////////////////////////////////
// CSizeStr

CSizeStr::CSizeStr()
{
	m_Type = 0x0000;	// 0xFFFF = Value
	m_Value = 0;
	m_String.Empty();
}
CSizeStr::~CSizeStr()
{
}
const CSizeStr & CSizeStr::operator = (CSizeStr &data)
{
	m_Type   = data.m_Type;
	m_Value  = data.m_Value;
	m_String = data.m_String;

	return *this;
}
BOOL CSizeStr::operator == (CSizeStr &data)
{
	if ( m_Type != data.m_Type )
		return FALSE;

	if ( m_Type == 0xFFFF )
		return (m_Value == data.m_Value ? TRUE : FALSE);
	else
		return (m_String.Compare(data.m_String) == 0 ? TRUE : FALSE);
}
void CSizeStr::SetIndex(int mode, class CStringIndex &index)
{
	if ( mode ) {	// Write
		if ( m_Type == 0xFFFF ) {
			index = (int)m_Value;
		} else {
			index = m_String;
		}
	} else {		// Read
		if ( index.m_bString ) {
			m_Type  = 0x0000;
			m_String = (LPCTSTR)index;
		} else {
			m_Type  = 0xFFFF;
			m_Value = (WORD)(int)index;
		}
	}
}
void CSizeStr::Serialize(int mode, class CBuffer &buf)
{
	if ( mode ) {	// Write
		if ( m_Type == 0xFFFF ) {
			buf.PutWord(m_Type);
			buf.PutWord(m_Value);
		} else {
			buf.PutText(TstrToUni(m_String));
		}
	} else {		// Read
		m_Type = buf.GetWord();
		if ( m_Type == 0xFFFF )
			m_Value = buf.GetWord();
		else {
			CStringW uni;
			WCHAR ch = (WCHAR)m_Type;
			while ( ch != L'\0' ) {
				uni += ch;
				ch = buf.GetWord();
			}
			m_Type = 0;
			m_String = UniToTstr(uni);
		}
	}
}
//////////////////////////////////////////////////////////////////////
// CDlgInitData

CDlgInitData::CDlgInitData()
{
	m_ctrlID  = 0;
	m_Message = 0;
	m_String.Empty();
}
CDlgInitData::~CDlgInitData()
{
}
const CDlgInitData & CDlgInitData::operator = (CDlgInitData &data)
{
	m_ctrlID  = data.m_ctrlID;
	m_Message = data.m_Message;
	m_String  = data.m_String;

	return *this;
}
void CDlgInitData::SetIndex(int mode, class CStringIndex &index)
{
	if ( mode ) {	// Write
		index.RemoveAll();

		index.Add((int)m_ctrlID);
		index.Add((int)m_Message);
		index.Add(m_String);

	} else {		// Read
		if ( index.GetSize() > 2 ) {
			m_ctrlID  = (UINT)(int)index[0];
			m_Message = (UINT)(int)index[1];
			m_String  = index[2];
		}
	}
}
void CDlgInitData::Serialize(int mode, class CBuffer &buf)
{
	if ( mode ) {	// Write
		buf.PutWord(m_ctrlID);
		buf.PutWord(m_Message);

		CStringA mbs = TstrToMbs(m_String);
		int len = (mbs.GetLength() + 1) * sizeof(CHAR);

		buf.PutDword(len);
		buf.Apend((LPBYTE)(LPCSTR)mbs, len);

	} else {		// Read
		m_ctrlID  = buf.GetWord();

		if ( m_ctrlID == 0 )		// end of
			return;

		m_Message = buf.GetWord();
		int len = buf.GetDword();

		if ( len <= 0 )
			m_String.Empty();
		else {
			CStringA mbs;
			memcpy(mbs.GetBufferSetLength(len / sizeof(CHAR)), buf.GetPtr(), len);
			buf.Consume(len);
			m_String = MbsToTstr(mbs);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CDlgItemData

CDlgItemData::CDlgItemData()
{
	m_helpID       = 0;
	m_exStyle      = 0;
	m_style        = 0;
	m_x            = 0;
	m_y            = 0;
	m_cx           = 0;
	m_cy           = 0;
	m_id           = 0;
	m_windowClass.Empty();
	m_title.Empty();
	m_extraCount   = 0;
	m_extraData    = NULL;
}
CDlgItemData::~CDlgItemData()
{
	if ( m_extraData != NULL )
		delete [] m_extraData;
}
const CDlgItemData & CDlgItemData::operator = (CDlgItemData &data)
{
	m_helpID       = data.m_helpID;
	m_exStyle      = data.m_exStyle;
	m_style        = data.m_style;
	m_x            = data.m_x;
	m_y            = data.m_y;
	m_cx           = data.m_cx;
	m_cy           = data.m_cy;
	m_id           = data.m_id;
	m_windowClass  = data.m_windowClass;
	m_title        = data.m_title;
	m_extraCount   = data.m_extraCount;

	if ( m_extraData != NULL )
		delete [] m_extraData;
	m_extraData = data.m_extraData;
	data.m_extraData = NULL;

	return *this;
}
void CDlgItemData::SetIndex(int mode, class CStringIndex &index)
{
	CStringIndex work;

	if ( mode ) {	// Write
		index.RemoveAll();

		index.Add((int)m_helpID);
		index.Add((int)m_exStyle);
		index.Add((int)m_style);

		index.Add((int)m_x);
		index.Add((int)m_y);
		index.Add((int)m_cx);
		index.Add((int)m_cy);

		index.Add((int)m_id);

		m_windowClass.SetIndex(TRUE, work);
		index.Add(work);

		m_title.SetIndex(TRUE, work);
		index.Add(work);

		index.Add((int)m_extraCount);

		if ( m_extraData != NULL ) {
			work.RemoveAll();
			for ( int n = 0 ; n < m_extraCount ; n++ )
				work.Add((int)m_extraData[n]);
			index.m_Array.Add(work);
		}

	} else {		// Read

		if ( index.GetSize() > 10 ) {
			m_helpID  = (DWORD)(int)index[0];
			m_exStyle = (DWORD)(int)index[1];
			m_style   = (DWORD)(int)index[2];

			m_x  = (short)(int)index[3];
			m_y  = (short)(int)index[4];
			m_cx = (short)(int)index[5];
			m_cy = (short)(int)index[6];

			m_id = (DWORD)(int)index[7];

			m_windowClass.SetIndex(FALSE, index[8]);

			m_title.SetIndex(FALSE, index[9]);

			m_extraCount = (WORD)(int)index[10];
		}

		if ( m_extraCount > 0 && index.GetSize() > 11 && index[11].GetSize() > m_extraCount ) {
			if ( m_extraData != NULL )
				delete [] m_extraData;
			m_extraData = new BYTE[m_extraCount];
			for ( int n = 0 ; n < m_extraCount ; n++ )
				m_extraData[n] = (BYTE)(int)index[11][n];
		}
	}
}
void CDlgItemData::Serialize(int mode, class CBuffer &buf, BOOL bExt)
{
	//DLGITEMTEMPLATE;
	//	DWORD style;
	//	DWORD dwExtendedStyle;
	//	short x;
	//	short y;
	//	short cx;
	//	short cy;
	//	WORD id;
	//DLGITEMTEMPLATEEX
	//	DWORD     m_helpID;
	//	DWORD     m_exStyle;
	//	DWORD     m_style;
	//	short     m_x;
	//	short     m_y;
	//	short     m_cx;
	//	short     m_cy;
	//	DWORD     m_id;
	//	CSizeStr  m_windowClass;
	//	CSizeStr  m_title;
	//	WORD      m_extraCount;

	if ( mode ) {	// Write
		if ( bExt ) {
			buf.PutDword(m_helpID);
			buf.PutDword(m_exStyle);
			buf.PutDword(m_style);
		} else {
			buf.PutDword(m_style);
			buf.PutDword(m_exStyle);
		}

		buf.PutWord(m_x);
		buf.PutWord(m_y);
		buf.PutWord(m_cx);
		buf.PutWord(m_cy);

		if ( bExt )
			buf.PutDword(m_id);
		else
			buf.PutWord(m_id);

		m_windowClass.Serialize(TRUE, buf);
		m_title.Serialize(TRUE, buf);

		if ( m_extraData == NULL )
			buf.PutWord(0);
		else {
			buf.PutWord(m_extraCount);
			buf.Apend(m_extraData, m_extraCount);
		}

		// DWORD pading
		for ( int n = buf.GetSize() ; (n & 3) != 0 ; n++ )
			buf.PutByte(0);

	} else {		// Read
		int n = buf.GetSize();

		if ( bExt ) {
			m_helpID  = buf.GetDword();
			m_exStyle = buf.GetDword();
			m_style   = buf.GetDword();
		} else {
			m_helpID  = 0;
			m_style   = buf.GetDword();
			m_exStyle = buf.GetDword();
		}

		m_x  = buf.GetWord();
		m_y  = buf.GetWord();
		m_cx = buf.GetWord();
		m_cy = buf.GetWord();

		if ( bExt )
			m_id = buf.GetDword();
		else
			m_id = buf.GetWord();

		m_windowClass.Serialize(FALSE, buf);
		m_title.Serialize(FALSE, buf);

		m_extraCount = buf.GetWord();

		if ( m_extraData != NULL ) {
			delete [] m_extraData;
			m_extraData = NULL;
		}

		if ( m_extraCount != 0 ) {
			m_extraData = new BYTE[m_extraCount];
			memcpy(m_extraData, buf.GetPtr(), m_extraCount);
			buf.Consume(m_extraCount);
		}

		// DWORD pading
		for ( n -= buf.GetSize() ; buf.GetSize() > 0 && (n & 3) != 0 ; n++ )
			buf.GetByte();
	}
}

//////////////////////////////////////////////////////////////////////
// CDlgTempBase

CDlgTempBase::CDlgTempBase()
{
	m_ResId  = 0;

	m_dlgVer      = 0;
	m_signature   = 0;
	m_helpID      = 0;
	m_exStyle     = 0;
	m_style       = 0;
	m_cDlgItems   = 0;
	m_x           = 0;
	m_y           = 0;
	m_cx          = 0;
	m_cy          = 0;
	m_menu.Empty();
	m_windowClass.Empty();
	m_title.Empty();
	m_pointsize   = 0;
	m_weight      = 0;
	m_italic      = 0;
	m_charset     = 0;
	m_typeface.Empty();

	m_bInst       = FALSE;
	m_hTemplate   = NULL;
	m_hInitData   = NULL;
}
CDlgTempBase::~CDlgTempBase()
{
	if ( m_hTemplate != NULL )
		GlobalFree(m_hTemplate);

	if ( m_hInitData != NULL )
		GlobalFree(m_hInitData);
}
const CDlgTempBase & CDlgTempBase::operator = (CDlgTempBase &data)
{
	m_ResId  = data.m_ResId;

	m_dlgVer      = data.m_dlgVer;
	m_signature   = data.m_signature;
	m_helpID      = data.m_helpID;
	m_exStyle     = data.m_exStyle;
	m_style       = data.m_style;
	m_cDlgItems   = data.m_cDlgItems;
	m_x           = data.m_x;
	m_y           = data.m_y;
	m_cx          = data.m_cx;
	m_cy          = data.m_cy;
	m_menu        = data.m_menu;
	m_windowClass = data.m_windowClass;
	m_title       = data.m_title;
	m_pointsize   = data.m_pointsize;
	m_weight      = data.m_weight;
	m_italic      = data.m_italic;
	m_charset     = data.m_charset;
	m_typeface    = data.m_typeface;
	m_bInst		  = data.m_bInst;

	m_Item.SetSize(data.m_Item.GetSize());
	for ( int n = 0 ; n < data.m_Item.GetSize() ; n++ )
		m_Item[n] = data.m_Item[n];

	m_Init.SetSize(data.m_Init.GetSize());
	for ( int n = 0 ; n < data.m_Init.GetSize() ; n++ )
		m_Init[n] = data.m_Init[n];

	if ( m_hTemplate != NULL )
		GlobalFree(m_hTemplate);
	m_hTemplate = data.m_hTemplate;
	data.m_hTemplate = NULL;

	if ( m_hInitData != NULL )
		GlobalFree(m_hInitData);
	m_hInitData = data.m_hInitData;
	data.m_hInitData = NULL;

	return *this;
}
void CDlgTempBase::CopyData(CDlgTempBase &data)
{
	int n, i;

	if ( m_ResId != data.m_ResId || !(m_windowClass == data.m_windowClass) )
		return;

	m_title = data.m_title;
	m_bInst = FALSE;

	for ( n = 0 ; n < m_Item.GetSize() ; n++ ) {
		if ( n < data.m_Item.GetSize() && m_Item[n].m_id == data.m_Item[n].m_id && m_Item[n].m_windowClass == data.m_Item[n].m_windowClass )
			m_Item[n].m_title = data.m_Item[n].m_title;
		else
			break;
	}

	for ( ; n < m_Item.GetSize() ; n++ ) {
		if ( m_Item[n].m_id != (-1) ) {
			for ( i = 0 ; i < data.m_Item.GetSize() ; i++ ) {
				if ( m_Item[n].m_id == data.m_Item[i].m_id && m_Item[n].m_windowClass == data.m_Item[i].m_windowClass ) {
					m_Item[n].m_title = data.m_Item[i].m_title;
					break;
				}
			}
		}
	}
}
void CDlgTempBase::SetIndex(int mode, class CStringIndex &index)
{
	int n, i;
	CString str;
	CStringIndex work, *pIndex;

	if ( mode ) {	// Write
		pIndex = &(index[_T("Param")]);

		pIndex->RemoveAll();

		pIndex->Add((int)m_dlgVer);
		pIndex->Add((int)m_signature);

		pIndex->Add((int)m_helpID);
		pIndex->Add((int)m_exStyle);
		pIndex->Add((int)m_style);

		pIndex->Add((int)m_x);
		pIndex->Add((int)m_y);
		pIndex->Add((int)m_cx);
		pIndex->Add((int)m_cy);

		m_menu.SetIndex(TRUE, work);
		pIndex->Add(work);

		m_windowClass.SetIndex(TRUE, work);
		pIndex->Add(work);

		pIndex->Add((LPCTSTR)m_title);

		pIndex->Add((int)m_pointsize);
		pIndex->Add((int)m_weight);
		pIndex->Add((int)m_italic);

		if ( m_bInst ) {
			// 多言語化するならSHIFTJIS_CHARSETでは、問題有り
			pIndex->Add((int)DEFAULT_CHARSET);
			pIndex->Add((LPCTSTR)_T("MS Shell Dlg"));
		} else {
			pIndex->Add((int)m_charset);
			pIndex->Add((LPCTSTR)m_typeface);
		}

		for ( int n = 0 ; n < m_Item.GetSize() ; n++ ) {
			str.Format(_T("%d"), n);
			m_Item[n].SetIndex(TRUE, index[_T("Item")][str]);
		}

		for ( int n = 0 ; n < m_Init.GetSize() ; n++ ) {
			str.Format(_T("%d"), n);
			m_Init[n].SetIndex(TRUE, index[_T("Init")][str]);
		}

	} else {		// Read

		if ( (i = index.Find(_T("Param"))) >= 0 && index[i].GetSize() > 16 ) {
			m_dlgVer    = (WORD)(int)index[i][0];
			m_signature = (WORD)(int)index[i][1];

			m_helpID  = (DWORD)(int)index[i][2];
			m_exStyle = (DWORD)(int)index[i][3];
			m_style   = (DWORD)(int)index[i][4];

			m_x  = (short)(int)index[i][5];
			m_y  = (short)(int)index[i][6];
			m_cx = (short)(int)index[i][7];
			m_cy = (short)(int)index[i][8];

			m_menu.SetIndex(FALSE, index[i][9]);
			m_windowClass.SetIndex(FALSE, index[i][10]);

			m_title = (LPCTSTR)index[i][11];

			m_pointsize = (WORD)(int)index[i][12];
			m_weight    = (WORD)(int)index[i][13];

			m_italic  = (BYTE)(int)index[i][14];
			m_charset = (BYTE)(int)index[i][15];

			m_typeface = (LPCTSTR)index[i][16];

			m_bInst    = FALSE;
		}

		if ( (i = index.Find(_T("Item"))) >= 0 ) {
			m_cDlgItems = 0;
			m_Item.RemoveAll();
			m_Item.SetSize(index[i].GetSize());
			for ( n = 0 ; n < index[i].GetSize() ; n++ )
				m_Item[n].SetIndex(FALSE, index[i][n]);
			m_cDlgItems = (WORD)m_Item.GetSize();
		}

		if ( (i = index.Find(_T("Init"))) >= 0 ) {
			m_Init.RemoveAll();
			m_Init.SetSize(index[i].GetSize());
			for ( n = 0 ; n < index[i].GetSize() ; n++ )
				m_Init[n].SetIndex(FALSE, index[i][n]);
		}

		if ( m_hTemplate != NULL )
			GlobalFree(m_hTemplate);
		m_hTemplate = NULL;

		if ( m_hInitData != NULL )
			GlobalFree(m_hInitData);
		m_hInitData = NULL;
	}
}
void CDlgTempBase::Serialize(int mode, class CBuffer &buf)
{
	//DLGTEMPLATE
	//	DWORD style;
	//	DWORD dwExtendedStyle;
	//	WORD cdit;
	//	short x;
	//	short y;
	//	short cx;
	//	short cy;
	//
	//DLGTEMPLATEEX
	//	WORD dlgVer;
	//	WORD signature;
	//	DWORD helpID;
	//	DWORD exStyle;
	//	DWORD style;
	//	WORD cDlgItems;
	//	short x;
	//	short y;
	//	short cx;
	//	short cy;

	BOOL bExt = FALSE;

	if ( mode ) {	// Write
		bExt = (m_signature == 0xFFFF ? TRUE : FALSE);

		if ( bExt ) {
			buf.PutWord(m_dlgVer);
			buf.PutWord(m_signature);

			buf.PutDword(m_helpID);
			buf.PutDword(m_exStyle);
			buf.PutDword(m_style);

		} else {
			buf.PutDword(m_style);
			buf.PutDword(m_exStyle);
		}

		buf.PutWord(m_cDlgItems);

		buf.PutWord(m_x);
		buf.PutWord(m_y);
		buf.PutWord(m_cx);
		buf.PutWord(m_cy);

		m_menu.Serialize(TRUE, buf);
		m_windowClass.Serialize(TRUE, buf);

		buf.PutText(m_title);

		buf.PutWord(m_pointsize);
		buf.PutWord(m_weight);

		buf.PutByte(m_italic);
		buf.PutByte(m_charset);

		buf.PutText(m_typeface);

		// DWORD pading
		for ( int n = buf.GetSize() ; (n & 3) != 0 ; n++ )
			buf.PutByte(0);

		for ( int n = 0 ; n < m_Item.GetSize() ; n++ )
			m_Item[n].Serialize(TRUE, buf, bExt);

	} else {		// Read
		int n = buf.GetSize();

		m_dlgVer    = buf.GetWord();
		m_signature = buf.GetWord();

		if ( m_signature == 0xFFFF ) {
			// DLGTEMPLATEEX
			bExt = TRUE;

			m_helpID  = buf.GetDword();
			m_exStyle = buf.GetDword();
			m_style   = buf.GetDword();

		} else {
			// DLGTEMPLATE
			bExt = FALSE;

			m_style = ((DWORD)m_signature << 16) | (DWORD)m_dlgVer;
			m_dlgVer = 0;
			m_helpID   = 0;
			m_exStyle  = buf.GetDword();
		}

		m_cDlgItems = buf.GetWord();

		m_x  = buf.GetWord();
		m_y  = buf.GetWord();
		m_cx = buf.GetWord();
		m_cy = buf.GetWord();

		m_menu.Serialize(FALSE, buf);
		m_windowClass.Serialize(FALSE, buf);

		buf.GetText(m_title);

		m_pointsize = buf.GetWord();
		m_weight    = buf.GetWord();

		m_italic  = buf.GetByte();
		m_charset = buf.GetByte();

		buf.GetText(m_typeface);

		// DWORD pading
		for ( n -= buf.GetSize() ; buf.GetSize() > 0 && (n & 3) != 0 ; n++ )
			buf.GetByte();

		m_Item.SetSize(m_cDlgItems);

		for ( n = 0 ; n < m_cDlgItems ; n++ )
			m_Item[n].Serialize(FALSE, buf, bExt);

		if ( m_hTemplate != NULL )
			GlobalFree(m_hTemplate);
		m_hTemplate = NULL;

		if ( m_hInitData != NULL )
			GlobalFree(m_hInitData);
		m_hInitData = NULL;
	}
}
HGLOBAL CDlgTempBase::GetDialogHandle()
{
	CBuffer tmp;
	LPBYTE ptr;

	if ( m_hTemplate != NULL )
		return m_hTemplate;

	try {
		Serialize(TRUE, tmp);
	} catch(...) {
		return FALSE;
	}

	if ( (m_hTemplate = GlobalAlloc(GPTR, tmp.GetSize())) == NULL )
		return NULL;

	if ( (ptr = (LPBYTE)GlobalLock(m_hTemplate)) != NULL )
		memcpy(ptr, tmp.GetPtr(), tmp.GetSize());
	GlobalUnlock(m_hTemplate);

	return m_hTemplate;
}
HGLOBAL CDlgTempBase::GetInitDataHandle()
{
	int n;
	CBuffer tmp;
	LPBYTE ptr;

	if ( m_hInitData != NULL )
		return m_hInitData;

	if ( m_Init.GetSize() == 0 )
		return NULL;

	try {
		for ( n = 0 ; n < m_Init.GetSize() ; n++ )
			m_Init[n].Serialize(TRUE, tmp);
		tmp.PutWord(0);
	} catch(...) {
		return NULL;
	}

	if ( (m_hInitData = GlobalAlloc(GPTR, tmp.GetSize())) == NULL )
		return NULL;

	ptr = (LPBYTE)GlobalLock(m_hInitData);
	memcpy(ptr, tmp.GetPtr(), tmp.GetSize());
	GlobalUnlock(m_hInitData);

	return m_hInitData;
}
BOOL CDlgTempBase::LoadResource(LPCTSTR lpszName)
{
	ASSERT(IS_INTRESOURCE(lpszName));

	HINSTANCE hInst;
	HRSRC hResource;
	HGLOBAL hTemplate;
	LPBYTE lpDialogTemplate;
	CDlgInitData data;
	BOOL ret = FALSE;

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_DIALOG)) == NULL )
		return FALSE;

	if ( (hResource = ::FindResource(hInst, lpszName, RT_DIALOG)) == NULL )
		return FALSE;

	if ( (hTemplate = ::LoadResource(hInst, hResource)) == NULL )
		return FALSE;

	if ( (lpDialogTemplate = (LPBYTE)LockResource(hTemplate)) == NULL ) {
		FreeResource(hTemplate);
		return FALSE;
	}

	CBuffer DlgTmp(lpDialogTemplate, (int)SizeofResource(hInst, hResource));

	try {
		Serialize(FALSE, DlgTmp);
		ret = TRUE;

	} catch(...) {
		TRACE(_T("LoadResource RT_DIALOG Error\n"));
		ret = FALSE;
	}

	UnlockResource(hTemplate);
	FreeResource(hTemplate);

	if ( !ret )
		return ret;

	m_bInst = TRUE;
	m_ResId = LOWORD((DWORD_PTR)lpszName);
	m_Init.RemoveAll();

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_DLGINIT)) == NULL )
		return TRUE;	// Not DLGINIT

	if ( (hResource = ::FindResource(hInst, lpszName, RT_DLGINIT)) == NULL )
		return TRUE;	// Not DLGINIT

	if ( (hTemplate = ::LoadResource(hInst, hResource)) == NULL )
		return FALSE;

	if ( (lpDialogTemplate = (LPBYTE)LockResource(hTemplate)) == NULL ) {
		FreeResource(hTemplate);
		return FALSE;
	}

	CBuffer initTmp(lpDialogTemplate, (int)SizeofResource(hInst, hResource));

	try {
		while ( initTmp.GetSize() > 2 ) {
			data.Serialize(FALSE, initTmp);
			m_Init.Add(data);
		}
		ret = TRUE;

	} catch(...) {
		TRACE(_T("LoadResource RT_DLGINIT Error\n"));
		ret = FALSE;
	}

	UnlockResource(hTemplate);
	FreeResource(hTemplate);

	return ret;
}

//////////////////////////////////////////////////////////////////////
// CResMenuData

CResMenuData::CResMenuData()
{
	m_Option = 0;
	m_Id     = 0;
	m_String.Empty();
}
CResMenuData::~CResMenuData()
{
}
const CResMenuData & CResMenuData::operator = (CResMenuData &data)
{
	m_Option = data.m_Option;
	m_Id     = data.m_Id;
	m_String = data.m_String;

	return *this;
}

//////////////////////////////////////////////////////////////////////
// CResBaseNode

CResBaseNode::CResBaseNode()
{
	m_ResId = 0;
}

//////////////////////////////////////////////////////////////////////
// CResStringBase

CResStringBase::CResStringBase()
{
	m_ResId = 0;
	m_String.Empty();
}
CResStringBase::~CResStringBase()
{
}
const CResStringBase & CResStringBase::operator = (CResStringBase &data)
{
	m_ResId  = data.m_ResId;
	m_String = data.m_String;

	return *this;
}
void CResStringBase::Serialize(int mode, class CBuffer &buf)
{
	if ( mode ) {	// Write
		buf.PutText(TstrToUni(m_String));

	} else {		// Read
		buf.GetText(m_String);
	}
}

//////////////////////////////////////////////////////////////////////
// CResMenuBase

CResMenuBase::CResMenuBase()
{
	m_ResId   = 0;
	m_Version = 0;
	m_Offset  = 0;
}
CResMenuBase::~CResMenuBase()
{
}
const CResMenuBase & CResMenuBase::operator = (CResMenuBase &data)
{
	m_ResId   = data.m_ResId;
	m_Version = data.m_Version;
	m_Offset  = data.m_Offset;

	m_Data.SetSize(data.m_Data.GetSize());
	for ( int n = 0 ; n < data.m_Data.GetSize() ; n++ )
		m_Data[n] = data.m_Data[n];

	return *this;
}
void CResMenuBase::CopyData(CResMenuBase &data)
{
	int n, i;

	if ( m_ResId != data.m_ResId || m_Version != data.m_Version || m_Offset != data.m_Offset )
		return;

	for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
		if ( n < data.m_Data.GetSize() && m_Data[n].m_Option == data.m_Data[n].m_Option && m_Data[n].m_Id == data.m_Data[n].m_Id )
			m_Data[n].m_String = data.m_Data[n].m_String;
		else
			break;
	}

	for ( ; n < m_Data.GetSize() ; n++ ) {
		if ( m_Data[n].m_Id != 0 ) {
			for ( i = 0 ; i < data.m_Data.GetSize() ; i++ ) {
				if ( m_Data[n].m_Id == data.m_Data[i].m_Id ) {
					m_Data[n].m_String = data.m_Data[i].m_String;
					break;
				}
			}
		}
	}
}
void CResMenuBase::SetIndex(int mode, class CStringIndex &index)
{
	int i, n;
	CString str;

	if ( mode ) {	// Write
		index[_T("Param")].Add((int)m_Version);
		index[_T("Param")].Add((int)m_Offset);

		for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
			str.Format(_T("%d"), n);
			index[_T("Data")][str].Add((int)m_Data[n].m_Option);
			index[_T("Data")][str].Add((int)m_Data[n].m_Id);
			index[_T("Data")][str].Add(m_Data[n].m_String);
		}

	} else {		// Read
		if ( (i = index.Find(_T("Param"))) >= 0 && index[i].GetSize() > 2 ) {
			m_Version = (WORD)(int)index[i][0];
			m_Offset  = (WORD)(int)index[i][0];
		}

		if ( (i = index.Find(_T("Data"))) >= 0 ) {
			m_Data.RemoveAll();
			m_Data.SetSize(index[i].GetSize());
			for ( n = 0 ; n < index[i].GetSize() ; n++ ) {
				m_Data[n].m_Option = (WORD)(int)index[i][n][0];
				m_Data[n].m_Id     = (WORD)(int)index[i][n][1];
				m_Data[n].m_String = index[i][n][2];
			}
		}
	}
}
void CResMenuBase::Serialize(int mode, class CBuffer &buf)
{
	int n;
	WCHAR ch;
	CStringW str;
	LPCWSTR p;
	CResMenuData work;

	if ( mode ) {	// Write
		buf.PutWord(m_Version);
		buf.PutWord(m_Offset);

		for ( n = 0 ; n < m_Data.GetSize() ; n++ ) {
			buf.PutWord(m_Data[n].m_Option);
			if ( (m_Data[n].m_Option & MF_POPUP) == 0 )
				buf.PutWord(m_Data[n].m_Id);
			str = TstrToUni(m_Data[n].m_String);
			p = str;
			do {
				buf.PutWord(*p);
			} while ( *(p++) != L'\0' );
		}

	} else {		// Read

		m_Version = (WORD)buf.GetWord();
		m_Offset  = (WORD)buf.GetWord();

		m_Data.RemoveAll();

		while ( buf.GetSize() >= 4 ) {
			work.m_Option = buf.GetWord();
			if ( (work.m_Option & MF_POPUP) != 0 )
				work.m_Id  = 0;
			else
				work.m_Id  = buf.GetWord();
			work.m_String.Empty();
			while ( (ch = (WCHAR)buf.GetWord()) != L'\0' )
				work.m_String += ch;

			m_Data.Add(work);
		}
	}
}
HMENU CResMenuBase::GetMenuHandle()
{
	CBuffer tmp;

	try {
		Serialize(TRUE, tmp);
	} catch(...) {
		return NULL;
	}

	return LoadMenuIndirect((LPMENUTEMPLATE)tmp.GetPtr());
}

//////////////////////////////////////////////////////////////////////
// CResToolBarBase

CResToolBarBase::CResToolBarBase()
{
	m_ResId   = 0;
	m_Version = 0;
	m_Width   = 0;
	m_Height  = 0;
}
CResToolBarBase::~CResToolBarBase()
{
}
const CResToolBarBase & CResToolBarBase::operator = (CResToolBarBase &data)
{
	m_ResId   = data.m_ResId;
	m_Version = data.m_Version;
	m_Width   = data.m_Width;
	m_Height  = data.m_Height;

	m_Item.SetSize(data.m_Item.GetSize());
	for ( int n = 0 ; n < data.m_Item.GetSize() ; n++ )
		m_Item[n] = data.m_Item[n];

	return *this;
}
void CResToolBarBase::SetIndex(int mode, class CStringIndex &index)
{
	CStringIndex work;

	if ( mode ) {	// Write
		index.Add((int)m_Version);
		index.Add((int)m_Width);
		index.Add((int)m_Height);

		for ( int n = 0 ; n < m_Item.GetSize() ; n++ )
			work.Add((int)m_Item[n]);
		index.Add(work);

	} else {		// Read
		if ( index.GetSize() > 3 ) {
			m_Version = (WORD)(int)index[0];
			m_Width   = (WORD)(int)index[1];
			m_Height  = (WORD)(int)index[2];

			int count = index[3].GetSize();

			m_Item.SetSize(count);
			for ( int n = 0 ; n < count ; n++ )
				m_Item[n] = (WORD)(int)index[3][n];
		}
	}
}
void CResToolBarBase::Serialize(int mode, class CBuffer &buf)
{
	if ( mode ) {	// Write
		buf.PutWord(m_Version);
		buf.PutWord(m_Width);
		buf.PutWord(m_Height);

		buf.PutWord((int)m_Item.GetSize());
		for ( int n = 0 ; n < m_Item.GetSize() ; n++ )
			buf.PutWord(m_Item[n]);

	} else {		// Read
		m_Version = (WORD)buf.GetWord();
		m_Width   = (WORD)buf.GetWord();
		m_Height  = (WORD)buf.GetWord();

		int count = buf.GetWord();

		m_Item.SetSize(count);
		for ( int n = 0 ; n < count ; n++ ) {
			m_Item[n] = (WORD)buf.GetWord();
			if ( m_Item[n] == 5102 )			// IDM_NEWCONNECT remove !!
				m_Item[n] = ID_FILE_NEW;
		}
	}
}
BOOL CResToolBarBase::SetToolBar(class CToolBarEx &ToolBar, CWnd *pWnd)
{
	int n, i;
	int width, height;
	int TabCount = 0;
	UINT *pNewTab;
	CSize sz, dpi;

	ToolBar.RemoveAll();

	// Itemリストを作成 WORD != UINT ?
	pNewTab = new UINT[(int)m_Item.GetSize()];

	for ( n = 0 ; n < m_Item.GetSize() ; n++ ) {
		if ( m_Item[n] == 0 && (n + 2) < m_Item.GetSize() && m_Item[n + 1] == 0 && m_Item[n + 2] != 0 ) {
			// ２つの区切りが続いていれば次の区切りまでDropDownメニューとして処理
			n += 2;
			int DropCount = 0;
			for ( ; n < m_Item.GetSize() ; n++ ) {
				if ( m_Item[n] == 0 )
					break;
				ToolBar.AddItemImage((int)m_Item[n]);
				pNewTab[TabCount + DropCount++] = (UINT)m_Item[n];
			}

			if ( DropCount > 0 ) {
				int item = pNewTab[TabCount];
				CString key;
				key.Format(_T("%d-%d"), m_ResId, TabCount);
				item = ::AfxGetApp()->GetProfileInt(_T("ToolBarEx"), key, item);
				for ( i = 0 ; i <  DropCount ; i++ ) {
					if ( (TabCount + i) < (int)m_Item.GetSize() && pNewTab[TabCount + i] == (UINT)item )
						break;
				}
				if ( i >= DropCount )
					item = pNewTab[TabCount];
				ToolBar.SetDropItem(TabCount, item, DropCount, &(pNewTab[TabCount]));
				pNewTab[TabCount++] = (UINT)item;
			}

		} else {
			if ( m_Item[n] != 0 )
				ToolBar.AddItemImage((int)m_Item[n]);
			if ( TabCount < (int)m_Item.GetSize() )
				pNewTab[TabCount++] = (UINT)m_Item[n];
		}
	}

	if ( !ToolBar.SetButtons(pNewTab, TabCount) ) {
		delete [] pNewTab;
		return FALSE;
	}

	ToolBar.InitDropDown();

	// サイズを設定

	if ( ::AfxGetMainWnd() == NULL ) {
		width  = m_Width;
		height = m_Height;
	} else {
		CDialogExt::GetActiveDpi(dpi, pWnd, pWnd);
		width  = MulDiv(m_Width,  dpi.cx, DEFAULT_DPI_X);
		height = MulDiv(m_Height, dpi.cy, DEFAULT_DPI_Y);
	}

	ToolBar.SetSizes(CSize(width + 7, height + 8), CSize(width, height));
	ToolBar.CreateItemImage(width, height);

	delete [] pNewTab;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CResBitmapBase

CResBitmapBase::CResBitmapBase()
{
	m_hBitmap = NULL;
}
CResBitmapBase::~CResBitmapBase()
{
	if ( m_hBitmap != NULL )
		DeleteObject(m_hBitmap);
}
const CResBitmapBase & CResBitmapBase::operator = (CResBitmapBase &data)
{
	m_ResId    = data.m_ResId;
	m_FileName = data.m_FileName;

	if ( m_hBitmap != NULL )
		DeleteObject(m_hBitmap);
	m_hBitmap = NULL;

	if ( data.m_hBitmap != NULL )
		m_hBitmap = (HBITMAP)CopyImage(data.m_hBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

	return *this;
}
void CResBitmapBase::SetIndex(int mode, class CStringIndex &index)
{
	if ( mode ) {	// Write
		index = m_FileName;

	} else {		// Read
		if ( m_hBitmap != NULL )
			DeleteObject(m_hBitmap);
		m_hBitmap = NULL;

		m_FileName = index;

		if ( !m_FileName.IsEmpty() ) {
			CImage image;

			if ( SUCCEEDED(image.Load(m_FileName)) ) {
				m_hBitmap = (HBITMAP)image;
				image.Detach();
			}
		}
	}
}
void CResBitmapBase::Serialize(int mode, class CBuffer &buf)
{
	if ( mode ) {	// Write
		buf.PutText(TstrToUni(m_FileName));

	} else {		// Read
		buf.GetText(m_FileName);

		if ( !m_FileName.IsEmpty() ) {
			CImage image;

			if ( SUCCEEDED(image.Load(m_FileName)) ) {
				m_hBitmap = (HBITMAP)image;
				image.Detach();
			}
		}
	}
}
BOOL CResBitmapBase::SetBitmap(CBitmap &bitmap)
{ 
	HANDLE hCopyHandle;

	if ( m_hBitmap == NULL )
		return FALSE;

	if ( (hCopyHandle = CopyImage(m_hBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION)) == NULL )
		return FALSE;

	return bitmap.Attach(hCopyHandle);
}
BOOL CResBitmapBase::LoadBitmap(WORD id, LPCTSTR filename)
{
	CImage image;

	m_ResId = id;
	m_FileName = filename;

	if ( m_FileName.IsEmpty() )
		return FALSE;

	if ( FAILED(image.Load(m_FileName)) )
		return FALSE;

	m_hBitmap = (HBITMAP)image;
	image.Detach();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CResDataBase

static int ResIdCmp(const void *src, const void *dis)
{
	return ((int)LOWORD((DWORD_PTR)src) - (int)(((CResBaseNode *)dis)->m_ResId));
}
static BOOL CALLBACK ResNameProcCallBack(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	CResDataBase *pThis = (CResDataBase *)lParam;

	switch((INT_PTR)lpszType) {
	case RT_MENU:    return pThis->AddMenu(lpszName);
	case RT_DIALOG:  return pThis->AddDialog(lpszName);
	case RT_STRING:  return pThis->AddString(lpszName);
	case RT_TOOLBAR: return pThis->AddToolBar(lpszName);
	}
	return FALSE;
}

CResDataBase::CResDataBase()
{
#ifndef	USE_RCDLL
	m_LangId    = MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN);
	m_Transrate = _T("ja");
	m_Language  = UniToTstr(L"\u65E5\u672C\u8A9E");	// _T("日本語");
#else
	TCHAR name[256];

	m_LangId    = ((CRLoginApp *)AfxGetApp())->GetLangId();		// GetUserDefaultUILanguage();

	GetLocaleInfo(MAKELCID(m_LangId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, name, sizeof(name));
	m_Transrate = name;

	GetLocaleInfo(MAKELCID(m_LangId, SORT_DEFAULT), LOCALE_SNATIVELANGNAME, name, sizeof(name));
	m_Language = name;
#endif
}
CResDataBase::~CResDataBase()
{
}
const CResDataBase & CResDataBase::operator = (CResDataBase &data)
{
	int n;

	m_LangId    = data.m_LangId;
	m_Transrate = data.m_Transrate;
	m_Language  = data.m_Language;

	m_Dialog.RemoveAll();
	m_Dialog.SetSize(data.m_Dialog.GetSize());
	for ( n = 0 ; n < data.m_Dialog.GetSize() ; n++ )
		m_Dialog[n] = data.m_Dialog[n];
	
	m_String.RemoveAll();
	m_String.SetSize(data.m_String.GetSize());
	for ( n = 0 ; n < data.m_String.GetSize() ; n++ )
		m_String[n] = data.m_String[n];
	
	m_Menu.RemoveAll();
	m_Menu.SetSize(data.m_Menu.GetSize());
	for ( n = 0 ; n < data.m_Menu.GetSize() ; n++ )
		m_Menu[n] = data.m_Menu[n];
	
	m_ToolBar.RemoveAll();
	m_ToolBar.SetSize(data.m_ToolBar.GetSize());
	for ( n = 0 ; n < data.m_ToolBar.GetSize() ; n++ )
		m_ToolBar[n] = data.m_ToolBar[n];

	m_Bitmap.RemoveAll();
	m_Bitmap.SetSize(data.m_Bitmap.GetSize());
	for ( n = 0 ; n < data.m_Bitmap.GetSize() ; n++ )
		m_Bitmap[n] = data.m_Bitmap[n];

	return *this;
}
void CResDataBase::Serialize(int mode, class CBuffer &buf)
{
	int n, max;
	CBuffer tmp;

	if ( mode ) {	// Write
		buf.PutDword((int)m_Dialog.GetSize());
		for ( n = 0 ; n < m_Dialog.GetSize() ; n++ ) {
			tmp.Clear();
			m_Dialog[n].Serialize(TRUE, tmp);
			buf.PutWord(m_Dialog[n].m_ResId);
			buf.PutBuf(tmp.GetPtr(), tmp.GetSize());
		}

		buf.PutDword((int)m_String.GetSize());
		for ( n = 0 ; n < m_String.GetSize() ; n++ ) {
			tmp.Clear();
			m_String[n].Serialize(TRUE, tmp);
			buf.PutWord(m_String[n].m_ResId);
			buf.PutBuf(tmp.GetPtr(), tmp.GetSize());
		}

		buf.PutDword((int)m_Menu.GetSize());
		for ( n = 0 ; n < m_Menu.GetSize() ; n++ ) {
			tmp.Clear();
			m_Menu[n].Serialize(TRUE, tmp);
			buf.PutWord(m_Menu[n].m_ResId);
			buf.PutBuf(tmp.GetPtr(), tmp.GetSize());
		}

		buf.PutDword((int)m_ToolBar.GetSize());
		for ( n = 0 ; n < m_ToolBar.GetSize() ; n++ ) {
			tmp.Clear();
			m_ToolBar[n].Serialize(TRUE, tmp);
			buf.PutWord(m_ToolBar[n].m_ResId);
			buf.PutBuf(tmp.GetPtr(), tmp.GetSize());
		}

		buf.PutDword((int)m_Bitmap.GetSize());
		for ( n = 0 ; n < m_Bitmap.GetSize() ; n++ ) {
			tmp.Clear();
			m_Bitmap[n].Serialize(TRUE, tmp);
			buf.PutWord(m_Bitmap[n].m_ResId);
			buf.PutBuf(tmp.GetPtr(), tmp.GetSize());
		}

	} else {		// Read
		max = buf.GetDword();
		for ( n = 0 ; n < max ; n++ ) {
			CDlgTempBase work;
			tmp.Clear();
			work.m_ResId = buf.GetWord();
			buf.GetBuf(&tmp);
			work.Serialize(FALSE, tmp);
			Add(work.m_ResId, RT_DIALOG, &work);
		}

		max = buf.GetDword();
		for ( n = 0 ; n < max ; n++ ) {
			CResStringBase work;
			tmp.Clear();
			work.m_ResId = buf.GetWord();
			buf.GetBuf(&tmp);
			work.Serialize(FALSE, tmp);
			Add(work.m_ResId, RT_STRING, &work);
		}

		max = buf.GetDword();
		for ( n = 0 ; n < max ; n++ ) {
			CResMenuBase work;
			tmp.Clear();
			work.m_ResId = buf.GetWord();
			buf.GetBuf(&tmp);
			work.Serialize(FALSE, tmp);
			Add(work.m_ResId, RT_MENU, &work);
		}

		max = buf.GetDword();
		for ( n = 0 ; n < max ; n++ ) {
			CResToolBarBase work;
			tmp.Clear();
			work.m_ResId = buf.GetWord();
			buf.GetBuf(&tmp);
			work.Serialize(FALSE, tmp);
			Add(work.m_ResId, RT_TOOLBAR, &work);
		}

		max = buf.GetDword();
		for ( n = 0 ; n < max ; n++ ) {
			CResBitmapBase work;
			tmp.Clear();
			work.m_ResId = buf.GetWord();
			buf.GetBuf(&tmp);
			work.Serialize(FALSE, tmp);
			Add(work.m_ResId, RT_BITMAP, &work);
		}
	}
}
BOOL CResDataBase::LoadFile(LPCTSTR pFilename)
{
	int n, i, b;
	CFile File;
	CStringIndex index;
	BOOL ret = FALSE;
	BOOL bVerErr = TRUE;

	if ( !File.Open(pFilename, CFile::modeRead | CFile::shareDenyNone) )
		return FALSE;

	CWaitCursor wait;
	CArchive Archive(&File, CArchive::load);

	m_Dialog.RemoveAll();
	m_String.RemoveAll();
	m_Menu.RemoveAll();
	m_ToolBar.RemoveAll();
	m_Bitmap.RemoveAll();

	InitRessource();

	try {
		index.SetNoSort(TRUE);
		index.Serialize(Archive, NULL, RESFILE_TYPE);

		if ( (i = index.Find(_T("String"))) >= 0 ) {
			for ( n = 0 ; n < index[i].GetSize() ; n++ ) {
				CResStringBase work;
				work.m_ResId = (WORD)_tstoi(index[i][n].m_nIndex);
				work.m_String = (LPCTSTR)index[i][n];
				if ( BinaryFind((void *)(work.m_ResId), m_String.GetData(), sizeof(CResStringBase), (int)m_String.GetSize(), ResIdCmp, &b) )
					m_String[b].m_String = work.m_String;
				else
					m_String.InsertAt(b, work);
			}
		}

		if ( (i = index.Find(_T("Version"))) >= 0 ) {
			CStringLoad version;
			((CRLoginApp *)AfxGetApp())->GetVersion(version);
			if ( version.CompareDigit(index[i]) != 0 ) {
				if ( ::AfxMessageBox(CStringLoad(IDE_RESVERSIONMISMATCH), MB_ICONQUESTION | MB_YESNO) != IDYES )
					throw _T("Version mismatch");
				bVerErr = TRUE;
			} else
				bVerErr = FALSE;
		}

		if ( (i = index.Find(_T("Language"))) >= 0 && index[i].GetSize() >= 3 ) {
			m_LangId    = (LANGID)(int)index[i][0];
			m_Transrate = index[i][1];
			m_Language  = index[i][2];
		}

		if ( (i = index.Find(_T("Dialog"))) >= 0 ) {
			for ( n = 0 ; n < index[i].GetSize() ; n++ ) {
				CDlgTempBase work;
				work.m_ResId = (WORD)_tstoi(index[i][n].m_nIndex);
				if ( work.m_ResId == IDD_ABOUTBOX )		// バージョン情報ダイアログは、読み込まない
					continue;
				work.SetIndex(FALSE, index[i][n]);
				if ( BinaryFind((void *)(work.m_ResId), m_Dialog.GetData(), sizeof(CDlgTempBase), (int)m_Dialog.GetSize(), ResIdCmp, &b) ) {
					if ( bVerErr )
						m_Dialog[b].CopyData(work);
					else
						m_Dialog[b] = work;
				} else if ( !bVerErr )
					m_Dialog.InsertAt(b, work);
			}
		}

		if ( (i = index.Find(_T("Menu"))) >= 0 ) {
			for ( n = 0 ; n < index[i].GetSize() ; n++ ) {
				CResMenuBase work;
				work.m_ResId = (WORD)_tstoi(index[i][n].m_nIndex);
				work.SetIndex(FALSE, index[i][n]);
				if ( BinaryFind((void *)(work.m_ResId), m_Menu.GetData(), sizeof(CResMenuBase), (int)m_Menu.GetSize(), ResIdCmp, &b) ) {
					if ( bVerErr )
						m_Menu[b].CopyData(work);
					else
						m_Menu[b] = work;
				} else if ( !bVerErr )
					m_Menu.InsertAt(b, work);
			}
		}

		if ( (i = index.Find(_T("ToolBar"))) >= 0 ) {
			for ( n = 0 ; n < index[i].GetSize() ; n++ ) {
				CResToolBarBase work;
				work.m_ResId = (WORD)_tstoi(index[i][n].m_nIndex);
				work.SetIndex(FALSE, index[i][n]);
				if ( BinaryFind((void *)(work.m_ResId), m_ToolBar.GetData(), sizeof(CResToolBarBase), (int)m_ToolBar.GetSize(), ResIdCmp, &b) )
					m_ToolBar[b] = work;
				else
					m_ToolBar.InsertAt(b, work);
			}
		}

		if ( (i = index.Find(_T("Bitmap"))) >= 0 ) {
			for ( n = 0 ; n < index[i].GetSize() ; n++ ) {
				CResBitmapBase work;
				work.m_ResId = (WORD)_tstoi(index[i][n].m_nIndex);
				work.SetIndex(FALSE, index[i][n]);
				if ( BinaryFind((void *)(work.m_ResId), m_Bitmap.GetData(), sizeof(CResBitmapBase), (int)m_Bitmap.GetSize(), ResIdCmp, &b) )
					m_Bitmap[b] = work;
				else
					m_Bitmap.InsertAt(b, work);
			}
		}

		ret = TRUE;

#ifdef	DEBUG
	} catch(LPCTSTR msg) {
		TRACE(_T("CResDataBase::LoadFile '%s'\n"), msg);
		ret = FALSE;
#endif
	} catch(...) {
		ret = FALSE;
	}
	
	Archive.Close();
	File.Close();

	return ret;
}
BOOL CResDataBase::SaveFile(LPCTSTR pFilename)
{
	int n;
	CFile File;
	CStringIndex index;
	CString str;
	BOOL ret = FALSE;

	if ( !File.Open(pFilename, CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive) )
		return FALSE;

	CWaitCursor wait;
	CArchive Archive(&File, CArchive::store | CArchive::bNoFlushOnDelete);

	try {
		index.SetNoSort(TRUE);

		((CRLoginApp *)AfxGetApp())->GetVersion(str);
		index[_T("Version")] = str;

		index[_T("Language")].Add((int)m_LangId);
		index[_T("Language")].Add(m_Transrate);
		index[_T("Language")].Add(m_Language);

		for ( n = 0 ; n < m_Dialog.GetSize() ; n++ ) {
			str.Format(_T("%d"), m_Dialog[n].m_ResId);
			m_Dialog[n].SetIndex(TRUE, index[_T("Dialog")][str]);
		}

		for ( n = 0 ; n < m_String.GetSize() ; n++ ) {
			str.Format(_T("%d"), m_String[n].m_ResId);
			index[_T("String")][str] = m_String[n].m_String;
		}

		for ( n = 0 ; n < m_Menu.GetSize() ; n++ ) {
			str.Format(_T("%d"), m_Menu[n].m_ResId);
			m_Menu[n].SetIndex(TRUE, index[_T("Menu")][str]);
		}

		for ( n = 0 ; n < m_ToolBar.GetSize() ; n++ ) {
			str.Format(_T("%d"), m_ToolBar[n].m_ResId);
			m_ToolBar[n].SetIndex(TRUE, index[_T("ToolBar")][str]);
		}

		for ( n = 0 ; n < m_Bitmap.GetSize() ; n++ ) {
			if ( !m_Bitmap[n].IsUserMap() )
				continue;
			str.Format(_T("%d"), m_Bitmap[n].m_ResId);
			m_Bitmap[n].SetIndex(TRUE, index[_T("Bitmap")][str]);
		}

		index.Serialize(Archive, NULL, RESFILE_TYPE);

		ret = TRUE;
	} catch(...) {
		ret = FALSE;
	}

	Archive.Close();
	File.Close();

	return ret;
}
void CResDataBase::InitRessource()
{
	EnumResourceNames(NULL, RT_DIALOG,  ResNameProcCallBack, (LONG_PTR)this);
	EnumResourceNames(NULL, RT_STRING,  ResNameProcCallBack, (LONG_PTR)this);
	EnumResourceNames(NULL, RT_MENU,    ResNameProcCallBack, (LONG_PTR)this);
	EnumResourceNames(NULL, RT_TOOLBAR, ResNameProcCallBack, (LONG_PTR)this);
}
BOOL CResDataBase::InitToolBarBitmap(LPCTSTR lpszName, UINT ImageId)
{
	int n, i, idx;
	int mx, my;
	HINSTANCE hInst;
	HRSRC hResource;
	HGLOBAL hTemplate;
	LPBYTE lpStrPtr;
	CResToolBarBase work;
	CBitmap ImageMap[3], Bitmap;
	BITMAP MapInfo[3];
	CDC SrcDC, DisDC;
	CBitmap *pSrcOld, *pDisOld;

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_TOOLBAR)) == NULL )
		return FALSE;

	if ( (hResource = ::FindResource(hInst, lpszName, RT_TOOLBAR)) == NULL )
		return FALSE;

	if ( (hTemplate = ::LoadResource(hInst, hResource)) == NULL )
		return FALSE;

	if ( (lpStrPtr = (LPBYTE)LockResource(hTemplate)) == NULL ) {
		FreeResource(hTemplate);
		return FALSE;
	}

	CBuffer tmp(lpStrPtr, (int)SizeofResource(hInst, hResource));

	work.m_ResId = LOWORD((DWORD_PTR)lpszName);
	work.Serialize(FALSE, tmp);

	UnlockResource(hTemplate);
	FreeResource(hTemplate);

	if ( work.m_Item.GetSize() <= 0 )
		return FALSE;

	mx = work.m_Width;
	my = work.m_Height;

	for ( n = 0 ; n < 3 ; n++ ) {
		ImageMap[n].LoadBitmap(ImageId + n);

		if ( ImageMap[n].m_hObject == NULL || !ImageMap[n].GetBitmap(&MapInfo[n]) )
			continue;

		if ( my >= MapInfo[n].bmHeight )
			continue;

		mx = my = MapInfo[n].bmHeight;
	}

	// ツールバーイメージからBitmapリソースを作成
	SrcDC.CreateCompatibleDC(NULL);
	DisDC.CreateCompatibleDC(NULL);

	for ( n = idx = 0 ; n < work.m_Item.GetSize() ; n++ ) {
		if ( work.m_Item[n] == 0 )
			continue;

		for ( i = 0 ; i < 3 ; i++ ) {
			if ( ImageMap[i].m_hObject == NULL )
				continue;

			pSrcOld = SrcDC.SelectObject(&(ImageMap[i]));

			if ( Bitmap.m_hObject == NULL ) {
				Bitmap.CreateCompatibleBitmap(&SrcDC, mx * 3, my);
				pDisOld = DisDC.SelectObject(&Bitmap);
				DisDC.FillSolidRect(0, 0, mx * 3, my, RGB(192, 192, 192));
			}

			if ( mx == MapInfo[i].bmHeight )
				DisDC.BitBlt(mx * i, 0, mx, my, &SrcDC, mx * idx, 0, SRCCOPY);
			else
				::TransparentBlt(DisDC, mx * i, 0, mx, my, 
					SrcDC, MapInfo[i].bmHeight * idx, 0, MapInfo[i].bmHeight, MapInfo[i].bmHeight, RGB(192, 192, 192));

			SrcDC.SelectObject(pSrcOld);
		}

		if ( Bitmap.m_hObject != NULL ) {
			CResBitmapBase tmp;
			tmp.m_ResId = work.m_Item[n];
			tmp.m_hBitmap = (HBITMAP)Bitmap;

			if ( !BinaryFind((void *)(tmp.m_ResId), m_Bitmap.GetData(), sizeof(CResBitmapBase), (int)m_Bitmap.GetSize(), ResIdCmp, &i) )
				m_Bitmap.InsertAt(i, tmp);

			DisDC.SelectObject(pDisOld);
			Bitmap.Detach();
		}

		idx++;
	}

	return TRUE;
}
BOOL CResDataBase::AddDialog(LPCTSTR lpszName)
{
	if ( !IS_INTRESOURCE(lpszName) )
		return FALSE;	// break

	int n;
	CDlgTempBase work;

	if ( !work.LoadResource(lpszName) )
		return FALSE;

	if ( !BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_Dialog.GetData(), sizeof(CDlgTempBase), (int)m_Dialog.GetSize(), ResIdCmp, &n) )
		m_Dialog.InsertAt(n, work);

	return TRUE;	// continue
}
BOOL CResDataBase::AddString(LPCTSTR lpszName)
{
	if ( !IS_INTRESOURCE(lpszName) )
		return FALSE;	// break

	HINSTANCE hInst;
	HRSRC hResource;
	HGLOBAL hTemplate;
	LPBYTE lpStrPtr;

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_STRING)) == NULL )
		return FALSE;

	if ( (hResource = ::FindResource(hInst, lpszName, RT_STRING)) == NULL )
		return FALSE;

	if ( (hTemplate = ::LoadResource(hInst, hResource)) == NULL )
		return FALSE;

	if ( (lpStrPtr = (LPBYTE)LockResource(hTemplate)) == NULL ) {
		FreeResource(hTemplate);
		return FALSE;
	}

	int n;
	WCHAR ch;
	WORD BlockID = LOWORD((DWORD_PTR)lpszName);
	CBuffer tmp(lpStrPtr, (int)SizeofResource(hInst, hResource));
	CResStringBase work;
	BOOL ret = FALSE;

	try {
		/*************************
			RT_STRING
			BlockID = (ItemID >> 4) + 1;
			{
				WORD	len;
				WCHAR	str[len];
			} block[16];
		**************************/

		work.m_ResId = (BlockID - 1) << 4;
		while ( tmp.GetSize() >= 2 ) {
			work.m_String.Empty();
			for ( n = tmp.GetWord() ; n > 0 ; n-- ) {
				ch = (WCHAR)tmp.GetWord();
				work.m_String += ch;
			}

			if ( !work.m_String.IsEmpty() ) {
				if ( !BinaryFind((void *)(work.m_ResId), m_String.GetData(), sizeof(CResStringBase), (int)m_String.GetSize(), ResIdCmp, &n) )
					m_String.InsertAt(n, work);
			}

			work.m_ResId++;
		}

		ret = TRUE;		// continue

	} catch(...) {
		TRACE(_T("AddString Error\n"));
		ret = FALSE;	// break
	}

	UnlockResource(hTemplate);
	FreeResource(hTemplate);

	return ret;
}
BOOL CResDataBase::AddMenu(LPCTSTR lpszName)
{
	if ( !IS_INTRESOURCE(lpszName) )
		return FALSE;	// break

	HINSTANCE hInst;
	HRSRC hResource;
	HGLOBAL hTemplate;
	LPBYTE lpStrPtr;

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_MENU)) == NULL )
		return FALSE;

	if ( (hResource = ::FindResource(hInst, lpszName, RT_MENU)) == NULL )
		return FALSE;

	if ( (hTemplate = ::LoadResource(hInst, hResource)) == NULL )
		return FALSE;

	if ( (lpStrPtr = (LPBYTE)LockResource(hTemplate)) == NULL ) {
		FreeResource(hTemplate);
		return FALSE;
	}

	int n;
	CResMenuBase work;
	CBuffer tmp(lpStrPtr, (int)SizeofResource(hInst, hResource));
	BOOL ret = FALSE;

	try {
		work.m_ResId = LOWORD((DWORD_PTR)lpszName);
		work.Serialize(FALSE, tmp);

		if ( !BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_Menu.GetData(), sizeof(CResMenuBase), (int)m_Menu.GetSize(), ResIdCmp, &n) )
			m_Menu.InsertAt(n, work);

		ret = TRUE;		// continue

	} catch(...) {
		TRACE(_T("AddMenu Error\n"));
		ret = FALSE;	// break
	}

	UnlockResource(hTemplate);
	FreeResource(hTemplate);

	return ret;
}
BOOL CResDataBase::AddToolBar(LPCTSTR lpszName)
{
	if ( !IS_INTRESOURCE(lpszName) )
		return FALSE;	// break

	HINSTANCE hInst;
	HRSRC hResource;
	HGLOBAL hTemplate;
	LPBYTE lpStrPtr;

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_TOOLBAR)) == NULL )
		return FALSE;

	if ( (hResource = ::FindResource(hInst, lpszName, RT_TOOLBAR)) == NULL )
		return FALSE;

	if ( (hTemplate = ::LoadResource(hInst, hResource)) == NULL )
		return FALSE;

	if ( (lpStrPtr = (LPBYTE)LockResource(hTemplate)) == NULL ) {
		FreeResource(hTemplate);
		return FALSE;
	}

	int n;
	CBuffer tmp(lpStrPtr, (int)SizeofResource(hInst, hResource));
	CResToolBarBase work;
	BOOL ret = FALSE;

	try {
		work.m_ResId = LOWORD((DWORD_PTR)lpszName);
		work.Serialize(FALSE, tmp);

		if ( !BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_ToolBar.GetData(), sizeof(CResToolBarBase), (int)m_ToolBar.GetSize(), ResIdCmp, &n) )
			m_ToolBar.InsertAt(n, work);

		ret = TRUE;		// continue

	} catch(...) {
		TRACE(_T("AddToolBar Error\n"));
		ret = FALSE;	// break
	}

	UnlockResource(hTemplate);
	FreeResource(hTemplate);

	return ret;
}
BOOL CResDataBase::AddBitmap(LPCTSTR lpszName)
{
	if ( !IS_INTRESOURCE(lpszName) )
		return FALSE;	// break

	int i;
	CBitmap Bitmap;
	CResBitmapBase tmp;

	if ( !Bitmap.LoadBitmap(lpszName) )
		return FALSE;

	tmp.m_ResId = LOWORD((DWORD_PTR)lpszName);
	tmp.m_hBitmap = (HBITMAP)Bitmap;
	Bitmap.Detach();

	if ( !BinaryFind((void *)(tmp.m_ResId), m_Bitmap.GetData(), sizeof(CResBitmapBase), (int)m_Bitmap.GetSize(), ResIdCmp, &i) )
		m_Bitmap.InsertAt(i, tmp);

	return FALSE;
}
BOOL CResDataBase::LoadResDialog(LPCTSTR lpszName, HGLOBAL &hTemplate, HGLOBAL &hInitData)
{
	int n;
	HINSTANCE hInst;
	HRSRC hResource;

	hTemplate = NULL;
	hInitData = NULL;

	if ( IS_INTRESOURCE(lpszName) && m_Dialog.GetData() > 0 && BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_Dialog.GetData(), sizeof(CDlgTempBase), (int)m_Dialog.GetSize(), ResIdCmp, &n) ) {
		hTemplate = m_Dialog[n].GetDialogHandle();
		hInitData = m_Dialog[n].GetInitDataHandle();
		return TRUE;
	}

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_DIALOG)) == NULL )
		return FALSE;

	if ( (hResource = ::FindResource(hInst, lpszName, RT_DIALOG)) == NULL )
		return FALSE;

	if ( (hTemplate = ::LoadResource(hInst, hResource)) == NULL )
		return FALSE;

	if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_DLGINIT)) == NULL )
		return TRUE;	// Not DLGINIT

	if ( (hResource = ::FindResource(hInst, lpszName, RT_DLGINIT)) == NULL )
		return TRUE;	// Not DLGINIT

	if ( (hInitData = ::LoadResource(hInst, hResource)) == NULL ) {
		FreeResource(hTemplate);
		hTemplate = NULL;
		return FALSE;
	}

	return TRUE;
}
BOOL CResDataBase::LoadResMenu(LPCTSTR lpszName, HMENU &hMenu)
{
	int n;
	HINSTANCE hInst;

	hMenu = NULL;

	if ( IS_INTRESOURCE(lpszName) && m_Menu.GetData() > 0 && BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_Menu.GetData(), sizeof(CResMenuBase), (int)m_Menu.GetSize(), ResIdCmp, &n) ) {
		hMenu = m_Menu[n].GetMenuHandle();

	} else {
		if ( (hInst = ::AfxFindResourceHandle(lpszName, RT_MENU)) == NULL )
			return FALSE;

		hMenu = ::LoadMenu(hInst, lpszName);
	}

	return (hMenu != NULL ? TRUE : FALSE);
}

BOOL CResDataBase::LoadResString(LPCTSTR lpszName, CString &str)
{
	int n;

	if ( IS_INTRESOURCE(lpszName) && m_String.GetData() > 0 && BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_String.GetData(), sizeof(CResStringBase), (int)m_String.GetSize(), ResIdCmp, &n) ) {
		str = m_String[n].m_String;
		return TRUE;
	}

	if ( IS_INTRESOURCE(lpszName) )
		return str.CString::LoadString(LOWORD((DWORD_PTR)lpszName));

	return FALSE;
}
BOOL CResDataBase::LoadResToolBar(LPCTSTR lpszName, CToolBarEx &ToolBar, CWnd *pWnd)
{
	int n;

	while ( !BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_ToolBar.GetData(), sizeof(CResToolBarBase), (int)m_ToolBar.GetSize(), ResIdCmp, &n) ) {
		if ( !AddToolBar(lpszName) )
			return FALSE;
	}

	return m_ToolBar[n].SetToolBar(ToolBar, pWnd);
}
BOOL CResDataBase::LoadResBitmap(LPCTSTR lpszName, CBitmap &Bitmap)
{
	int n;

	if ( IS_INTRESOURCE(lpszName) && m_Bitmap.GetData() > 0 && BinaryFind((void *)LOWORD((DWORD_PTR)lpszName), m_Bitmap.GetData(), sizeof(CResBitmapBase), (int)m_Bitmap.GetSize(), ResIdCmp, &n) )
		return m_Bitmap[n].SetBitmap(Bitmap);

	return Bitmap.LoadBitmap(lpszName);
}
void *CResDataBase::Find(WORD nId, LPCTSTR type)
{
	int n;

	switch(LOWORD((DWORD_PTR)type)) {
	case RT_BITMAP:
		if ( BinaryFind((void *)nId, m_Bitmap.GetData(), sizeof(CResBitmapBase), (int)m_Bitmap.GetSize(), ResIdCmp, &n) )
			return (void *)&(m_Bitmap[n]);
		break;
	case RT_MENU:
		if ( BinaryFind((void *)nId, m_Menu.GetData(), sizeof(CResMenuBase), (int)m_Menu.GetSize(), ResIdCmp, &n) )
			return (void *)&(m_Menu[n]);
		break;
	case RT_DIALOG:
		if ( BinaryFind((void *)nId, m_Dialog.GetData(), sizeof(CDlgTempBase), (int)m_Dialog.GetSize(), ResIdCmp, &n) )
			return (void *)&(m_Dialog[n]);
		break;
	case RT_STRING:
		if ( BinaryFind((void *)nId, m_String.GetData(), sizeof(CResStringBase), (int)m_String.GetSize(), ResIdCmp, &n) )
			return (void *)&(m_String[n]);
		break;
	case RT_TOOLBAR:
		if ( BinaryFind((void *)nId, m_ToolBar.GetData(), sizeof(CResToolBarBase), (int)m_ToolBar.GetSize(), ResIdCmp, &n) )
			return (void *)&(m_ToolBar[n]);
		break;
	}

	return NULL;
}
void CResDataBase::Add(WORD nId, LPCTSTR type, void *pBase)
{
	int n;

	if ( pBase == NULL )
		return;

	switch(LOWORD((DWORD_PTR)type)) {
	case RT_BITMAP:
		if ( BinaryFind((void *)nId, m_Bitmap.GetData(), sizeof(CResBitmapBase), (int)m_Bitmap.GetSize(), ResIdCmp, &n) )
			m_Bitmap[n] = *((CResBitmapBase *)pBase);
		else
			m_Bitmap.InsertAt(n, *((CResBitmapBase *)pBase));
		break;
	case RT_MENU:
		if ( BinaryFind((void *)nId, m_Menu.GetData(), sizeof(CResMenuBase), (int)m_Menu.GetSize(), ResIdCmp, &n) )
			m_Menu[n] = *((CResMenuBase *)pBase);
		else
			m_Menu.InsertAt(n, *((CResMenuBase *)pBase));
		break;
	case RT_DIALOG:
		if ( BinaryFind((void *)nId, m_Dialog.GetData(), sizeof(CDlgTempBase), (int)m_Dialog.GetSize(), ResIdCmp, &n) )
			m_Dialog[n] = *((CDlgTempBase *)pBase);
		else
			m_Dialog.InsertAt(n, *((CDlgTempBase *)pBase));
		break;
	case RT_STRING:
		if ( BinaryFind((void *)nId, m_String.GetData(), sizeof(CResStringBase), (int)m_String.GetSize(), ResIdCmp, &n) )
			m_String[n] = *((CResStringBase *)pBase);
		else
			m_String.InsertAt(n, *((CResStringBase *)pBase));
		break;
	case RT_TOOLBAR:
		if ( BinaryFind((void *)nId, m_ToolBar.GetData(), sizeof(CResToolBarBase), (int)m_ToolBar.GetSize(), ResIdCmp, &n) )
			m_ToolBar[n] = *((CResToolBarBase *)pBase);
		else
			m_ToolBar.InsertAt(n, *((CResToolBarBase *)pBase));
		break;
	}
}
BOOL CResDataBase::IsTranslateString(LPCTSTR str)
{
	int len = 0;

	while ( *str != _T('\0') ) {
		if ( *str > _T(' ') && !(*str >= _T('0') && *str <= _T('9')) && _tcschr(_T(",.()[]<>;:+-*/&%"), *str) == NULL )
			len++;
#ifdef	UNICODE
		if ( *str >= 0x100 )
			len++;
#endif
		str++;
		if ( len > 2 )
			return TRUE;
	}
	return FALSE;
}
int CResDataBase::MakeTranslateTable(CTranslateStringTab &strTab)
{
	int n, i;

	for ( n = 0 ; n < m_Dialog.GetSize() ; n++ ) {
		strTab.Add(&m_Dialog[n].m_title);
		for ( i = 0 ; i < m_Dialog[n].m_Item.GetSize() ; i++ ) {
			if ( IsTranslateString(m_Dialog[n].m_Item[i].m_title.m_String) )
				strTab.Add(&m_Dialog[n].m_Item[i].m_title.m_String);
		}
		//for ( i = 0 ; i < m_Dialog[n].m_Init.GetSize() ; i++ ) {
		//	if ( IsTranslateString(m_Dialog[n].m_Init[i].m_String) )
		//		strTab.Add(&m_Dialog[n].m_Init[i].m_String);
		//}
	}

	for ( n = 0 ; n < m_String.GetSize() ; n++ ) {
		if ( IsTranslateString(m_String[n].m_String) )
			strTab.Add(&m_String[n].m_String);
	}

	for ( n = 0 ; n < m_Menu.GetSize() ; n++ ) {
		for ( i = 0 ; i < m_Menu[n].m_Data.GetSize() ; i++ ) {
			if ( IsTranslateString(m_Menu[n].m_Data[i].m_String) )
				strTab.Add(&m_Menu[n].m_Data[i].m_String);
		}
	}

	return (int)strTab.GetSize();
}