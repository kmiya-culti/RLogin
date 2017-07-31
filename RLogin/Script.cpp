#include "StdAfx.h"
#include <math.h>
#include <Mmsystem.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <share.h>
#include "RLogin.h"
#include "Data.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "ExtSocket.h"
#include "ScriptDlg.h"
#include "Script.h"
#include "RegEx.h"
#include "IConv.h"
#include "openssl/evp.h"


#pragma comment(lib,"winmm")

//////////////////////////////////////////////////////////////////////
// CScriptValue

CScriptValue::CScriptValue()
{
	m_Index.Empty();
	m_Type = VALTYPE_INT;
	m_Value.m_Int = 0;
	m_Buf.Clear();
	m_ArrayPos = 0;
	m_FuncPos = (-1);
	m_FuncExt = NULL;
	m_Array.RemoveAll();
	m_Left = m_Right = m_Child = m_Next = m_Root = NULL;
}
CScriptValue::~CScriptValue()
{
	RemoveAll();
}
int CScriptValue::GetType()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetType();
	return m_Type;
}
CBuffer *CScriptValue::GetBuf()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetBuf();
	(LPCSTR)(*this);
	return &m_Buf;
}
CBuffer *CScriptValue::GetWBuf()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetBuf();
	(LPCWSTR)(*this);
	return &m_Buf;
}
void CScriptValue::RemoveAll()
{
	for ( int n = 0 ; n < (int)m_Array.GetSize() ; n++ ) {
		if ( m_Array[n] != NULL )
			delete (CScriptValue *)(m_Array[n]);
	}
	m_Array.RemoveAll();
	m_Child = NULL;
}
void CScriptValue::ArrayCopy(CScriptValue &data)
{
	RemoveAll();
	for ( int n = 0 ; n < data.m_Array.GetSize() ; n++ )
		Add(data[n]);
}
void CScriptValue::BufCopy(CScriptValue &data)
{
	m_Type = data.m_Type;
	m_Buf  = data.m_Buf;
}
const CScriptValue & CScriptValue::operator = (CScriptValue &data)
{
	m_Index = data.m_Index;
	m_Type  = data.m_Type;
	m_Value = data.m_Value;
	m_Buf   = data.m_Buf;

	m_ArrayPos = data.m_ArrayPos;
	m_FuncPos  = data.m_FuncPos;
	m_FuncExt  = data.m_FuncExt;

	ArrayCopy(data);

	return *this;
}
void CScriptValue::operator = (class CScriptLex &lex)
{
	switch(lex.m_Type) {
	case LEX_INT:
		*this = (int)lex;
		break;
	case LEX_DOUBLE:
		*this = (double)lex;
		break;
	case LEX_INT64:
		*this = (LONGLONG)lex;
		break;
	case LEX_LABEL:
	case LEX_STRING:
		m_Type = VALTYPE_STRING;
		m_Buf.Clear();
		m_Buf.Apend(lex.m_Buf.m_Ptr, lex.m_Buf.m_Len);
		break;
	case LEX_WSTRING:
		m_Type = VALTYPE_WSTRING;
		m_Buf.Clear();
		m_Buf.Apend(lex.m_Buf.m_Ptr, lex.m_Buf.m_Len);
		break;
	}
}

//////////////////////////////////////////////////////////////////////

void CScriptValue::operator *= (CScriptValue &data)
{
	if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE )
		*this = (double)*this * (double)data;
	else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 )
		*this = (LONGLONG)*this * (LONGLONG)data;
	else
		*this = (int)*this * (int)data;
}
void CScriptValue::operator /= (CScriptValue &data)
{
	if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE ) {
		if ( (double)data != 0.0 )
			*this = (double)*this / (double)data;
		else
			*this = (double)0.0;
	} else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 ) {
		if ( (LONGLONG)data != 0L )
			*this = (LONGLONG)*this / (LONGLONG)data;
		else
			*this = (LONGLONG)0;
	} else {
		if ( (int)data != 0 )
			*this = (int)*this / (int)data;
		else
			*this = (int)0;
	}
}
void CScriptValue::operator %= (CScriptValue &data)
{
	if ( (int)data != 0 )
		*this = (int)*this % (int)data;
	else
		*this = (int)0;
}
void CScriptValue::operator += (CScriptValue &data)
{
	if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE )
		*this = (double)*this + (double)data;
	else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 )
		*this = (LONGLONG)*this + (LONGLONG)data;
	else
		*this = (int)*this + (int)data;
}
void CScriptValue::operator -= (CScriptValue &data)
{
	if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE )
		*this = (double)*this - (double)data;
	else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 )
		*this = (LONGLONG)*this - (LONGLONG)data;
	else
		*this = (int)*this - (int)data;
}
void CScriptValue::AddStr(CScriptValue &data)
{
	CBuffer *sp, *dp;
	if ( GetType() == VALTYPE_WSTRING || data.GetType() == VALTYPE_WSTRING ) {
		if ( m_Type == VALTYPE_IDENT ) {
			sp = GetWBuf();
			dp = data.GetWBuf();
			m_Type = VALTYPE_WSTRING;
			m_Buf.Clear();
			m_Buf.Apend(sp->GetPtr(), sp->GetSize());
			m_Buf.Apend(dp->GetPtr(), dp->GetSize());
		} else {
			(LPCWSTR)(*this);
			dp = data.GetWBuf();
			m_Buf.Apend(dp->GetPtr(), dp->GetSize());
		}
	} else {
		if ( m_Type == VALTYPE_IDENT ) {
			sp = GetBuf();
			dp = data.GetBuf();
			m_Type = VALTYPE_STRING;
			m_Buf.Clear();
			m_Buf.Apend(sp->GetPtr(), sp->GetSize());
			m_Buf.Apend(dp->GetPtr(), dp->GetSize());
		} else {
			(LPCSTR)(*this);
			dp = data.GetBuf();
			m_Buf.Apend(dp->GetPtr(), dp->GetSize());
		}
	}
}

//////////////////////////////////////////////////////////////////////

int CScriptValue::Add(CScriptValue &data)
{
	CScriptValue *tp;
	CScriptValue *sp;

	sp = new CScriptValue;
	*sp = data;

	if ( (tp = m_Child) == NULL )
		m_Child = sp;
	else {
		while ( tp != NULL ) {
			if ( tp->m_Index.Compare(sp->m_Index) >= 0 ) {
				if ( tp->m_Left == NULL ) {
					tp->m_Left = sp;
					break;
				}
				tp = tp->m_Left;
			} else {
				if ( tp->m_Right == NULL ) {
					tp->m_Right = sp;
					break;
				}
				tp = tp->m_Right;
			}
		}
	}

	sp->m_Root = this;
	sp->m_ArrayPos = (int)m_Array.Add(sp);
	return sp->m_ArrayPos;
}
int CScriptValue::Find(LPCSTR str)
{
	int c;
	CScriptValue *tp;

	for ( tp = m_Child ; tp != NULL ; ) {
		if ( (c = tp->m_Index.Compare(str)) == 0 )
			return tp->m_ArrayPos;
		else if ( c >= 0 )
			tp = tp->m_Left;
		else
			tp = tp->m_Right;
	}
	return (-1);
}
int CScriptValue::Add(LPCSTR str)
{
	int n;
	CScriptValue *tp;
	CScriptValue tmp;

	if ( str == NULL ) {
		tp = new CScriptValue;
		tp->m_Root = this;
		tp->m_ArrayPos = (int)m_Array.Add(tp);
		return tp->m_ArrayPos;
	} else if ( (n = Find(str)) != (-1) )
		return n;

	tmp.m_Index = str;
	return Add(tmp);
}
CScriptValue & CScriptValue::operator [] (LPCSTR str)
{
	return *(CScriptValue *)m_Array[Add(str)];
}
CScriptValue & CScriptValue::operator [] (int index)
{
	if ( index >= (int)m_Array.GetSize() )
		m_Array.SetSize(index + 1);
	if ( m_Array[index] == NULL ) {
		CScriptValue *sp = new CScriptValue;
		sp->m_Root = this;
		sp->m_ArrayPos = index;
		m_Array[index] = (void *)sp;
	}
	return *((CScriptValue *)m_Array[index]);
}

//////////////////////////////////////////////////////////////////////

const int CScriptValue::operator = (int value)
{
	m_Type = VALTYPE_INT;
	m_Value.m_Int = value;
	return value;
}
const double CScriptValue::operator = (double value)
{
	m_Type = VALTYPE_DOUBLE;
	m_Value.m_Double = value;
	return value;
}
const LONGLONG CScriptValue::operator = (LONGLONG value)
{
	m_Type = VALTYPE_INT64;
	m_Value.m_Int64 = value;
	return value;
}
const LPCSTR CScriptValue::operator = (LPCSTR str)
{
	m_Type = VALTYPE_STRING;
	m_Buf.Clear();
	m_Buf.Apend((LPBYTE)str, (int)strlen(str));
	return m_Buf;
}
const LPCWSTR CScriptValue::operator = (LPCWSTR str)
{
	m_Type = VALTYPE_WSTRING;
	m_Buf.Clear();
	m_Buf.Apend((LPBYTE)str, (int)wcslen(str) * sizeof(WCHAR));
	return m_Buf;
}
const LPBYTE CScriptValue::operator = (LPBYTE value)
{
	m_Type = VALTYPE_LPBYTE;
	m_Value.m_Ptr = value;
	return value;
}
const WCHAR * CScriptValue::operator = (WCHAR * value)
{
	m_Type = VALTYPE_LPWCHAR;
	m_Value.m_Ptr = value;
	return value;
}
const CScriptValue * CScriptValue::operator = (CScriptValue *ptr)
{
	m_Type = VALTYPE_IDENT;
	m_Value.m_Ptr = ptr;
	return ptr;
}
const void * CScriptValue::operator = (void *ptr)
{
	m_Type = VALTYPE_PTR;
	m_Value.m_Ptr = ptr;
	return ptr;
}

//////////////////////////////////////////////////////////////////////

CScriptValue::operator int ()
{
	switch(m_Type) {
	case VALTYPE_DOUBLE:
		return (int)(m_Value.m_Double);
	case VALTYPE_INT64:
		return (int)(m_Value.m_Int64);
	case VALTYPE_STRING:
		return atoi((LPCSTR)m_Buf);
	case VALTYPE_WSTRING:
		return _wtoi((LPCWSTR)m_Buf);
	case VALTYPE_LPBYTE:
		return (int)*((LPBYTE)(m_Value.m_Ptr));
	case VALTYPE_LPWCHAR:
		return (int)*((WCHAR *)(m_Value.m_Ptr));
	case VALTYPE_IDENT:
		return (int)*((CScriptValue *)(m_Value.m_Ptr));
	case VALTYPE_PTR:
		return (m_Value.m_Ptr == NULL ? 0 : 1);
	}
	return m_Value.m_Int;
}
CScriptValue::operator double ()
{
	switch(m_Type) {
	case VALTYPE_INT:
		return (double)(m_Value.m_Int);
	case VALTYPE_INT64:
		return (double)(m_Value.m_Int64);
	case VALTYPE_STRING:
		return atof((LPCSTR)m_Buf);
	case VALTYPE_WSTRING:
		return _wtof((LPCWSTR)m_Buf);
	case VALTYPE_LPBYTE:
		return (double)*((LPBYTE)(m_Value.m_Ptr));
	case VALTYPE_LPWCHAR:
		return (double)*((WCHAR *)(m_Value.m_Ptr));
	case VALTYPE_IDENT:
		return (double)*((CScriptValue *)(m_Value.m_Ptr));
	case VALTYPE_PTR:
		return 0.0;
	}
	return m_Value.m_Double;
}
CScriptValue::operator LONGLONG ()
{
	switch(m_Type) {
	case VALTYPE_INT:
		return (LONGLONG)(m_Value.m_Int);
	case VALTYPE_DOUBLE:
		return (LONGLONG)(m_Value.m_Double);
	case VALTYPE_STRING:
		return (LONGLONG)atof((LPCSTR)m_Buf);
	case VALTYPE_WSTRING:
		return (LONGLONG)_wtof((LPCWSTR)m_Buf);
	case VALTYPE_LPBYTE:
		return (LONGLONG)*((LPBYTE)(m_Value.m_Ptr));
	case VALTYPE_LPWCHAR:
		return (LONGLONG)*((WCHAR *)(m_Value.m_Ptr));
	case VALTYPE_IDENT:
		return (LONGLONG)*((CScriptValue *)(m_Value.m_Ptr));
	case VALTYPE_PTR:
		return (LONGLONG)0;
	}
	return m_Value.m_Int64;
}
CScriptValue::operator LPCSTR ()
{
	static CString tmp;
	switch(m_Type) {
	case VALTYPE_INT:
		tmp.Format("%d", m_Value.m_Int);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_DOUBLE:
		tmp.Format("%g", m_Value.m_Double);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_INT64:
		tmp.Format("%I64d", m_Value.m_Int64);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_WSTRING:
		tmp = (LPCWSTR)m_Buf;
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_LPBYTE:
		tmp.Format("%c", *((LPBYTE)(m_Value.m_Ptr)));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_LPWCHAR:
		tmp.Format("%C", *((WCHAR *)(m_Value.m_Ptr)));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_IDENT:
		return (LPCSTR)*((CScriptValue *)(m_Value.m_Ptr));
	case VALTYPE_PTR:
		m_Buf.Clear();
		break;
	}
	m_Type = VALTYPE_STRING;
	return (LPCSTR)m_Buf;
}
CScriptValue::operator LPCWSTR ()
{
	CStringW tmp;
	switch(m_Type) {
	case VALTYPE_INT:
		tmp.Format(L"%d", m_Value.m_Int);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_DOUBLE:
		tmp.Format(L"%g", m_Value.m_Double);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_INT64:
		tmp.Format(L"%I64d", m_Value.m_Int64);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_STRING:
		tmp = (LPCSTR)m_Buf;
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_LPBYTE:
		tmp.Format(L"%c", *((LPBYTE)(m_Value.m_Ptr)));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_LPWCHAR:
		tmp.Format(L"%C", *((WCHAR *)(m_Value.m_Ptr)));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_IDENT:
		return (LPCWSTR)*((CScriptValue *)(m_Value.m_Ptr));
	case VALTYPE_PTR:
		m_Buf.Clear();
		break;
	}
	m_Type = VALTYPE_WSTRING;
	return (LPCWSTR)m_Buf;
}
CScriptValue::operator LPBYTE ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_LPWCHAR:
	case VALTYPE_PTR:
		return NULL;
	case VALTYPE_IDENT:
		return (LPBYTE)*((CScriptValue *)(m_Value.m_Ptr));
	}
	return (LPBYTE)m_Value.m_Ptr;
}
CScriptValue::operator WCHAR * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_PTR:
		return NULL;
	case VALTYPE_IDENT:
		return (WCHAR *)*((CScriptValue *)(m_Value.m_Ptr));
	}
	return (WCHAR *)m_Value.m_Ptr;
}
CScriptValue::operator CScriptValue * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
	case VALTYPE_PTR:
		return NULL;
	}
	return (CScriptValue *)m_Value.m_Ptr;
}
CScriptValue::operator void * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
		return NULL;
	case VALTYPE_IDENT:
		return (void *)*((CScriptValue *)(m_Value.m_Ptr));
	}
	return m_Value.m_Ptr;
}

//////////////////////////////////////////////////////////////////////

int CScriptValue::Compare(int mode, CScriptValue *ptr)
{
	int c;

	switch(mode & 2) {
	case 0:
		if ( GetType() == VALTYPE_WSTRING || ptr->GetType() == VALTYPE_WSTRING )
			c = wcscmp((LPCWSTR)*this, (LPCWSTR)*ptr);
		else if ( GetType() == VALTYPE_STRING || ptr->GetType() == VALTYPE_STRING )
			c = strcmp((LPCSTR)*this, (LPCSTR)*ptr);
		else if ( GetType() == VALTYPE_DOUBLE || ptr->GetType() == VALTYPE_DOUBLE )
			c = (((double)*this - (double)*ptr) == 0.0 ? 0 : ((double)*this > (double)*ptr ? 1 : (-1)));
		else if ( GetType() == VALTYPE_INT64 || ptr->GetType() == VALTYPE_INT64 )
			c = ((LONGLONG)*this == (LONGLONG)*ptr ? 0 : ((LONGLONG)*this > (LONGLONG)*ptr ? 1 : (-1)));
		else
			c = (int)*this - (int)*ptr;
		break;
	case 2:
		c = m_Index.Compare(ptr->m_Index);
		break;
	}
	if ( (mode & 1) != 0 )
		c = 0 - c;
	return c;
}
void CScriptValue::Sort(int mode)
{
    int i, j, h;
	void *tmp;

	for ( i = j = 0 ; i < (int)m_Array.GetSize() ; i++ ) {
		if ( m_Array[i] != NULL )
			m_Array[j++] = m_Array[i];
	}
	m_Array.SetSize(j);

	for ( h = 1 ; h <= (int)m_Array.GetSize() ; h = h * 3 + 1 )
		;

    while ( (h = h / 3) >= 1 ) {
        for ( i = h ; i < m_Array.GetSize() ; i++ ) {
			tmp = m_Array[i];
            for ( j = i - h ; j >= 0 ; j -= h ) {
				if ( ((CScriptValue *)tmp)->Compare(mode, (CScriptValue *)m_Array[j]) >= 0 )
					break;
               m_Array[j + h] = m_Array[j];
			}
            m_Array[j + h] = tmp;
        }
    }

	for ( i = 0 ; i < (int)m_Array.GetSize() ; i++ )
		((CScriptValue *)m_Array[i])->m_ArrayPos = i;
}
#ifdef	_DEBUG
void CScriptValue::Debug()
{
	int n;
	CScriptValue *sp;
	static char *typestr[] = { "int","double","int64","str","wstr","pbyte","pwchar","ident","ptr" };

	TRACE("%s:(%s)=", m_Index, typestr[m_Type]);
	switch(m_Type) {
	case VALTYPE_INT:
		TRACE("%d", m_Value.m_Int);
		break;
	case VALTYPE_DOUBLE:
		TRACE("%g", m_Value.m_Double);
		break;
	case VALTYPE_INT64:
		TRACE("%I64d", m_Value.m_Int64);
		break;
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
		TRACE("'%s'", (LPCSTR)*this);
		break;
	case VALTYPE_IDENT:
		TRACE("{");
		((CScriptValue *)(m_Value.m_Ptr))->Debug();
		TRACE("}");
		break;
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
	case VALTYPE_PTR:
		TRACE("'%p'", m_Value.m_Ptr);
		break;
	}

	if ( m_FuncPos != (-1) )
		TRACE(":F(%d)", m_FuncPos & 0x7FFF);

	for ( n = 0 ; n < m_Array.GetSize() ; n++ ) {
		if ( (sp = (CScriptValue *)m_Array[n]) != NULL ) {
			TRACE("[%d:", n);
			sp->Debug();
			TRACE("]");
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////
// CScriptLex

CScriptLex::CScriptLex()
{
	m_Type = 0;
	m_Data.m_Int64 = 0;
	m_Buf.m_Len = 0;
	m_Buf.m_Ptr = NULL;
	m_Left = m_Right = (-1);
}
CScriptLex::~CScriptLex()
{
	if ( m_Buf.m_Ptr != NULL )
		delete m_Buf.m_Ptr;
}
void CScriptLex::SetBuf(void *pBuf, int nBufLen)
{
	if ( m_Buf.m_Ptr != NULL ) {
		delete m_Buf.m_Ptr;
		m_Buf.m_Ptr = NULL;
	}
	if ( pBuf == NULL )
		return;
	m_Buf.m_Len = nBufLen;
	m_Buf.m_Ptr = new BYTE[nBufLen + 2];
	memcpy(m_Buf.m_Ptr, pBuf, nBufLen);
	m_Buf.m_Ptr[nBufLen] = 0;
	m_Buf.m_Ptr[nBufLen + 1] = 0;
}
const CScriptLex & CScriptLex::operator = (CScriptLex &data)
{
	m_Type  = data.m_Type;
	m_Data  = data.m_Data;
	SetBuf(data.m_Buf.m_Ptr, data.m_Buf.m_Len);
	m_Left  = data.m_Left;
	m_Right = data.m_Right;
	return *this;
}
int CScriptLex::Compare(CScriptLex &data)
{
	int c;

	if ( (c = m_Type - data.m_Type) != 0 )
		return c;

	switch(m_Type) {
	case LEX_INT:
		c = (int)*this - (int)data;
		break;
	case LEX_DOUBLE:
		c = ((double)*this == (double)data ? 0 : ((double)*this < (double)data ? (-1) : 1));
		break;
	case LEX_INT64:
		c = (int)((LONGLONG)*this - (LONGLONG)data);
		break;
	case LEX_LABEL:
	case LEX_STRING:
	case LEX_WSTRING:
		if ( (c = m_Buf.m_Len - data.m_Buf.m_Len) == 0 )
			c = memcmp(m_Buf.m_Ptr, data.m_Buf.m_Ptr, m_Buf.m_Len);
		break;
	}

	return c;
}

//////////////////////////////////////////////////////////////////////
// CScript

CScript::CScript(class CRLoginDoc *pDoc, CWnd *pWnd) : CSyncSock(pDoc, pWnd)
{
	m_BufPos = m_BufMax = 0;
	m_BufPtr = NULL;
	m_UnGetChar = 0;

	m_Local = m_Class = NULL;
	m_Free = NULL;
	m_Stack = new CScriptValue;
	m_Loops = NULL;

	m_SyncMode = FALSE;
	m_ConsOpen = FALSE;

	m_Data.RemoveAll();
	m_Code.RemoveAll();
	m_IncFile.RemoveAll();

	FuncInit();
}
CScript::~CScript(void)
{
	CScriptValue *sp;
	while ( (sp = m_Stack) != NULL ) {
		m_Stack = sp->m_Next;
		delete sp;
	}
	while ( (sp = m_Free) != NULL ) {
		m_Free = sp->m_Next;
		delete sp;
	}
}

//////////////////////////////////////////////////////////////////////

int	CScript::InChar(int ch, TCHAR *ptn)
{
    int n;
    for ( n = 0 ; ptn[n] != '\0' ; n++ ) {
		if ( ptn[n] == ch )
		    return n;
    }
    return (-1);
}
int	CScript::StrBin(int mx, TCHAR *ptn[], LPCTSTR str)
{
    int n, c;
    int bs = 0;

	mx--;
    while ( bs <= mx ) {
		n = (bs + mx) / 2;
		if ( (c = _tcscmp(ptn[n], str)) == 0 )
		    return n;
		else if ( c < 0 )
		    bs = n + 1;
		else
		    mx = n - 1;
    }
    return (-1);
}

#define issjis1(c)		(((unsigned char)(c) >= 0x81 && \
						  (unsigned char)(c) <= 0x9F) || \
						 ((unsigned char)(c) >= 0xE0 && \
						  (unsigned char)(c) <= 0xFC) )

#define issjis2(c)		(((unsigned char)(c) >= 0x40 && \
						  (unsigned char)(c) <= 0x7E) || \
						 ((unsigned char)(c) >= 0x80 && \
						  (unsigned char)(c) <= 0xFC) )

#define iskana(c)		((unsigned char)(c) >= 0xA0 && \
						 (unsigned char)(c) <= 0xDF)

int CScript::GetChar()
{
	int ch;
	CString tmp;

	if ( m_UnGetChar != 0 ) {
		ch = m_UnGetChar;
		m_UnGetChar = 0;
		return ch;
	}

	m_BufOld = m_BufPos;

	if ( m_BufPos >= m_BufMax )
		return EOF;

	if ( issjis1(m_BufPtr[m_BufPos]) && issjis2(m_BufPtr[m_BufPos + 1]) ) {
		tmp  = (char)m_BufPtr[m_BufPos++];
		tmp += (char)m_BufPtr[m_BufPos++];
	} else
		tmp  = (char)m_BufPtr[m_BufPos++];

	return (WCHAR)((CStringW)tmp)[0];
}
void CScript::UnGetChar(int ch)
{
	m_UnGetChar = ch;
}

int CScript::LexDigit(int ch)
{
	LONGLONG val = 0;

	if ( ch == '0' ) {
		if ( (ch = GetChar()) == '.' )
			goto DOUBLEFLOAT;
		else if ( ch == 'x' || ch == 'X' ) {
			while ( (ch = GetChar()) != EOF ) {
				if ( ch >= '0' && ch <= '9' )
					val = val * 16 + (ch - '0');
				else if ( ch >= 'A' && ch <= 'F' )
					val = val * 16 + (ch - 'A' + 10);
				else if ( ch >= 'a' && ch <= 'f' )
					val = val * 16 + (ch - 'a' + 10);
				else
					break;
			}
		} else {
			while ( ch >= '0' && ch <= '7' ) {
				val = val * 8 + (ch - '0');
				ch = GetChar();
			}
		}
		if ( ch == 'L' ) {
			m_LexTmp = (LONGLONG)val;
			return LEX_INT64;
		} else {
			UnGetChar(ch);
			if ( val > 0xFFFFFFFF ) {
				m_LexTmp = (LONGLONG)val;
				return LEX_INT64;
			} else {
				m_LexTmp = (int)val;
				return LEX_INT;
			}
		}
	}

	while ( ch >= '0' && ch <= '9' ) {
		val = val * 10 + (ch - '0');
		ch = GetChar();
	}

	if ( ch == '.' )
		goto DOUBLEFLOAT;
	else if ( ch == 'L' ) {
		m_LexTmp = (LONGLONG)val;
		return LEX_INT64;
	} else {
		UnGetChar(ch);
		if ( val > 0xFFFFFFFF ) {
			m_LexTmp = (LONGLONG)val;
			return LEX_INT64;
		} else {
			m_LexTmp = (int)val;
			return LEX_INT;
		}
	}

DOUBLEFLOAT:

	double a = (double)val;
	double d = 0.1;

	while ( (ch = GetChar()) != EOF ) {
		if ( ch >= '0' && ch <= '9' )
			a = a + (double)(ch - '0') * d;
		else
			break;
		d = d * 0.1;
	}
	UnGetChar(ch);
	m_LexTmp = (double)a;
	return LEX_DOUBLE;
}
int CScript::LexEscape(int ch)
{
	int n;
	int val = 0;

	if ( ch == 'x' || ch == 'X' ) {
		for ( n = 0 ; (ch = GetChar()) != EOF && n < 2 ; n++ ) {
			if ( ch >= '0' && ch <= '9' )
				val = val * 16 + (ch - '0');
			else if ( ch >= 'a' && ch <= 'f' )
				val = val * 16 + (ch - 'a' + 10);
			else if ( ch >= 'A' && ch <= 'F' )
				val = val * 16 + (ch - 'A' + 10);
			else
				break;
		}
		UnGetChar(ch);
	} else if ( ch >= '0' && ch <= '7' ) {
		val = val - '0';
		for ( n = 0 ; (ch = GetChar()) != EOF && n < 2 ; n++ ) {
			if ( ch >= '0' && ch <= '7' )
				val = val * 8 + (ch - '0');
			else
				break;
		}
		UnGetChar(ch);
	} else {
		switch(ch) {
		case 'a': val = '\x07'; break;
		case 'b': val = '\x08'; break;
		case 't': val = '\x09'; break;
		case 'n': val = '\x0A'; break;
		case 'v': val = '\x0B'; break;
		case 'f': val = '\x0C'; break;
		case 'r': val = '\x0D'; break;
		default: val = ch; break;
		}
	}
	return val;
}
CScriptLex *CScript::Lex()
{
#define IDTR_MAX   	(5 * 3)
    static TCHAR *func[] = {
		"break",	"case", 	"class",	"continue",	"default",
		"do",		"else", 	"for",		"function",	"if",
		"return",	"switch",	"this",		"var",		"while"
    };
	static int adr_tbl[]={
		LEX_ANDAND, LEX_OROR, LEX_INC, LEX_DEC
    };
    static int let_tbl[]={
		LEX_ADDLET, LEX_SUBLET, LEX_MULLET, LEX_DIVLET, LEX_MODLET,
		LEX_ANDLET, LEX_ORLET,  LEX_XORLET, LEX_EQU,    LEX_UNEQU,	LEX_CATLET
    };
    static int sfl_tbl[]={
		LEX_LEFTLET, LEX_RIGHTLET
    };
    static int sft_tbl[]={
		LEX_LEFT, LEX_RIGHT
    };
    static int bse_tbl[]={
		LEX_SML, LEX_BIG
    };

	int n;
	int ch, nx;
	CString tmp;
	CBuffer buf;

	m_LexTmp.SetBuf(NULL, 0);

LOOP:
	while ( (ch = GetChar()) != EOF ) {
		if ( ch > ' ' && ch != 0x7F && ch != '\\' )
			break;
	}

	if ( ch == '/' ) {
		if ( (nx = GetChar()) == '*' ) {
			ch = nx;
			nx = GetChar();
			while ( !(ch == '*' && nx == '/') ) {
				ch = nx;
				if ( (nx = GetChar()) == EOF )
					goto ENDOF;
			}
			goto LOOP;
		} else if ( nx == '/' ) {
			while ( (ch = GetChar()) != '\n' ) {
				if ( ch == EOF )
					goto ENDOF;
			}
			goto LOOP;
		} else
			UnGetChar(nx);

	}

	m_PosOld = m_BufOld;

	if ( ch == EOF ) {
		goto ENDOF;
	
	} else if ( ch >= '0' && ch <= '9' ) {
		ch = LexDigit(ch);

	} if ( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' ) {
		nx = GetChar();
		if ( (ch == 'L' || ch == 'W') && nx == '"' ) {	// WSTRING
			while ( (ch = GetChar()) != EOF ) {
				if ( ch == '"' )
					break;
				else if ( ch == '\\' )
					buf.PutWord(LexEscape(GetChar()));
				else if ( ch == EOF )
					goto ENDOF;
				else
					buf.PutWord(ch);
			}
			m_LexTmp.SetBuf(buf.GetPtr(), buf.GetSize());
			ch = LEX_WSTRING;

		} else {
			UnGetChar(nx);

			do {
				tmp += (WCHAR)ch;
				ch = GetChar();
			} while ( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' );
		    UnGetChar(ch);

			if ( (n = StrBin(IDTR_MAX, func, tmp)) >= 0 )
				ch = LEX_BREAK + n;
			else {
				m_LexTmp.SetBuf((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
				ch = LEX_LABEL;
			}
		}

	} else if ( (n = InChar(ch, _T("+-*/%&|^=!#"))) >= 0 ) {
		if ( (nx = GetChar()) == '=' )
		    ch = let_tbl[n];
		else if ( (n = InChar(ch, _T("&|+-"))) >= 0 && ch == nx )
		    ch = adr_tbl[n];
		else
		    UnGetChar(nx);

    } else if ( (n = InChar(ch,"<>")) >= 0 ) {
		if ( (nx = GetChar()) == ch ) {
		    if ( (nx = GetChar()) == '=' )
				ch = sfl_tbl[n];
			else {
				UnGetChar(nx);
				ch = sft_tbl[n];
			}
		} else if ( nx == '=' )
		    ch = bse_tbl[n];
		else
		    UnGetChar(nx);

	} else if ( ch == '\'' ) {
		if ( (ch = GetChar()) == '\\' )
			ch = LexEscape(GetChar());
		if ( (m_LexTmp = ch) == EOF )
			goto ENDOF;
		if ( (ch = GetChar()) != '\'' )
			UnGetChar(ch);
		ch = LEX_INT;

	} else if ( ch == '"' ) {
		while ( (ch = GetChar()) != EOF ) {
			if ( ch == '"' )
				break;
			else if ( ch == '\\' )
				buf.Put8Bit(LexEscape(GetChar()));
			else if ( ch == EOF )
				goto ENDOF;
			else {
				tmp = (WCHAR)ch;
				buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
			}
		}
		m_LexTmp.SetBuf(buf.GetPtr(), buf.GetSize());
		ch = LEX_STRING;
	}

//	TRACE("Lex %d(%c)\n", ch, (ch < 256 ? ch : ' '));

	m_LexTmp.m_Type = ch;
	return &m_LexTmp;

ENDOF:
	m_LexTmp.m_Type = LEX_EOF;
	return &m_LexTmp;
}
int CScript::LexAdd(CScriptLex *lex)
{
	int c, n;

	lex->m_Left = lex->m_Right = (-1);

	if ( m_LexData.GetSize() <= 0 )
		return m_LexData.Add(*lex);
	else {
		for ( n = 0 ; ; ) {
			if ( (c = m_LexData[n].Compare(*lex)) == 0 )
				return n;
			else if ( c < 0 ) {
				if ( m_LexData[n].m_Left == (-1) ) {
					m_LexData[n].m_Left = m_LexData.Add(*lex);
					return m_LexData[n].m_Left;
				} else
					n = m_LexData[n].m_Left;
			} else {
				if ( m_LexData[n].m_Right == (-1) ) {
					m_LexData[n].m_Right = m_LexData.Add(*lex);
					return m_LexData[n].m_Right;
				} else
					n = m_LexData[n].m_Right;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

void CScript::LoopPush()
{
	CScriptLoop *lp = new CScriptLoop;
	lp->m_StartPos = CodePos();
	lp->m_BreakAddr.RemoveAll();
	lp->m_CaseAddr = (-1);
	lp->m_Next = m_Loops;
	m_Loops = lp;
}
void CScript::LoopPop()
{
	int n;
	CScriptLoop *lp = m_Loops;
	m_Loops = lp->m_Next;
	for ( n = 0 ; n < (int)lp->m_BreakAddr.GetSize() ; n++ )
		CodeSetPos(lp->m_BreakAddr[n]);
	delete lp;
}
void CScript::LoopAdd(int pos)
{
	m_Loops->m_BreakAddr.Add(pos);
}

int CScript::IdentChack(CScriptLex *lex)
{
	int n;
	if ( (n = (int)m_Code.GetSize() - 1) < 0 )
		THROW(lex);
	if ( m_Code[n] != CM_IDENT )
		THROW(lex);
	return n;
}
void CScript::LoadIdent(CScriptValue *sp, int index)
{
	if ( sp == NULL )
		return;
	LoadIdent(sp->m_Next, sp->m_ArrayPos);
	if ( sp->m_Next == NULL ) {
		CodeAdd(CM_LOAD);
		CodeAddPos(index);
	} else {
		CScriptLex lex;
		lex.SetBuf((void *)(LPCSTR)(*sp)[index].m_Index, (*sp)[index].m_Index.GetLength());
		lex.m_Type = LEX_LABEL;
		CodeAdd(CM_PUSH);
		CodeAdd(CM_VALUE);
		CodeAddPos(LexAdd(&lex));
		CodeAdd(CM_ARRAY);
	}
}


//////////////////////////////////////////////////////////////////////

CScriptLex *CScript::Stage18(CScriptLex *lex)
{
/****
expr	:	LABEL
		|	LABEL '.' LABEL
		|	STRING
		|	INT
		|	DOUBLE
		|	'(' expr ')'
		|	'[' expr ',' expr ... ']'
****/
	int n;
	int nf = FALSE;

	switch(lex->m_Type) {
	case LEX_VAR:
		nf = TRUE;
		lex = Lex();
		if ( lex->m_Type != LEX_LABEL )
			THROW(lex);
		// no break
	case LEX_LABEL:
		if ( m_Local != NULL && (n = m_Local->Find((LPCSTR)*lex)) != (-1) ) {
			CodeAdd(CM_LOCAL);
			CodeAddPos(n);
		} else if ( nf ) {
			LoadIdent(m_Class, m_Class->Add((LPCSTR)*lex));
		} else {
			CScriptValue *sp = NULL;
			for ( sp = m_Class ; sp != NULL ; sp = sp->m_Next ) {
				if ( (n = sp->Find((LPCSTR)*lex)) != (-1) ) {
					LoadIdent(sp, n);
					break;
				}
			}
			if ( sp == NULL )
				LoadIdent(m_Class, m_Class->Add((LPCSTR)*lex));
		}
		n = CodeAdd(CM_IDENT);
		lex = Lex();
		while ( lex->m_Type == '.' ) {
			lex = Lex();
			if ( lex->m_Type != LEX_LABEL )
				THROW(lex);
			CodeSet(n, CM_PUSH);
			n = LexAdd(lex);
			CodeAdd(CM_VALUE);
			CodeAddPos(n);
			CodeAdd(CM_ARRAY);
			n = CodeAdd(CM_IDENT);
			lex = Lex();
		}
		break;
	case LEX_INT:
	case LEX_DOUBLE:
	case LEX_INT64:
	case LEX_STRING:
	case LEX_WSTRING:
		CodeAdd(CM_VALUE);
		CodeAddPos(LexAdd(lex));
		lex = Lex();
		break;
	case '(':
		lex = Stage04(Lex());
		if ( lex->m_Type != ')' )
			THROW(lex);
		lex = Lex();
		break;
	case '[':
		CodeAdd(CM_VALCLEAR);
		CodeAdd(CM_PUSH);
		for ( ; ; ) {
			lex = Stage04(Lex());
			if ( lex->m_Type == ':' ) {
				CodeAdd(CM_PUSH);
				lex = Stage04(Lex());
				CodeAdd(CM_VALWITHKEY);
			} else
				CodeAdd(CM_VALARRAY);

			if ( lex->m_Type == ']' )
				break;
			else if ( lex->m_Type != ',' )
				THROW(lex);
		}
		CodeAdd(CM_POP);
		lex = Lex();
		break;
	}
	return lex;
}
CScriptLex *CScript::Stage17(CScriptLex *lex)
{
/****
	|	expr '[' expr ']'
****/
	int n;
	lex = Stage18(lex);
	while ( lex->m_Type == '[' ) {
		n = IdentChack(lex);
		CodeSet(n, CM_PUSH);
		lex = Lex();
		if ( lex->m_Type == ']' ) {	/* index empty */
			CodeAdd(CM_ADDARRAY);
		} else {
			lex = Stage04(lex);
			if ( lex->m_Type != ']' )
				THROW(lex);
			CodeAdd(CM_ARRAY);
		}
		CodeAdd(CM_IDENT);
		lex = Lex();
	}
	return lex;
}
CScriptLex *CScript::Stage16(CScriptLex *lex)
{
/****
	|	expr '(' exprs ')'
****/
	int n;
	lex = Stage17(lex);
	if ( lex->m_Type == '(' ) {
		n = IdentChack(lex);
		CodeSet(n, CM_PUSH);
		lex = Lex();
		n = 0;
		while ( lex->m_Type != ')' ) {
			lex = Stage04(lex);
			CodeAdd(CM_PUSH);
			n++;
			if ( lex->m_Type != ',' )
				break;
			lex = Lex();
		}
		if ( lex->m_Type != ')' )
			THROW(lex);
		CodeAdd(CM_CALL);
		CodeAddPos(n);
		lex = Lex();
	}
	return lex;
}
CScriptLex *CScript::Stage15(CScriptLex *lex)
{
/****
	|	expr '{' expr '}'
****/
	int n;
	lex = Stage16(lex);
	if ( lex->m_Type == '{' ) {
		n = IdentChack(lex);
		CodeSet(n, CM_PUSH);
		lex = Lex();
		if ( lex->m_Type == '}' ) {	/* index empty */
			CodeAdd(CM_ADDINDEX);
		} else {
			lex = Stage04(lex);
			if ( lex->m_Type != '}' )
				THROW(lex);
			CodeAdd(CM_INDEX);
		}
		CodeAdd(CM_IDENT);
		lex = Lex();
	}
	return lex;
}
CScriptLex *CScript::Stage14(CScriptLex *lex)
{
/****
	|	'++' expr		# 右から左
	|	'--' expr
	|	expr '++'
	|	expr '--'
****/
	int n;
	if ( lex->m_Type == LEX_INC ) {
		lex = Stage15(Lex());
		n = IdentChack(lex);
		CodeSet(n, CM_INC);
	} else if ( lex->m_Type == LEX_DEC ) {
		lex = Stage15(Lex());
		n = IdentChack(lex);
		CodeSet(n, CM_DEC);
	} else {
		lex = Stage15(lex);
		if ( lex->m_Type == LEX_INC ) {
			n = IdentChack(lex);
			CodeSet(n, CM_AINC);
			lex = Lex();
		} else if ( lex->m_Type == LEX_DEC ) {
			n = IdentChack(lex);
			CodeSet(n, CM_ADEC);
			lex = Lex();
		}
	}
	return lex;
}
CScriptLex *CScript::Stage13(CScriptLex *lex)
{
/****
	|	'-' expr
	|	'+' expr
	|	'~' expr
	|	'!' expr
****/
	for ( ; ; ) {
		if ( lex->m_Type == '-' ) {
			lex = Stage14(Lex());
			CodeAdd(CM_NEG);
		} else if ( lex->m_Type == '+' ) {
			lex = Stage14(Lex());
		} else if ( lex->m_Type == '~' ) {
			lex = Stage14(Lex());
			CodeAdd(CM_COM);
		} else if ( lex->m_Type == '!' ) {
			lex = Stage14(Lex());
			CodeAdd(CM_NOT);
		} else
			return Stage14(lex);
	}
}
CScriptLex *CScript::Stage12(CScriptLex *lex)
{
/****
	|	expr '*' expr
	|	expr '/' expr
	|	expr '%' expr
****/
	lex = Stage13(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case '*':
			CodeAdd(CM_PUSH);
			lex = Stage13(Lex());
			CodeAdd(CM_MUL);
			break;
		case '/':
			CodeAdd(CM_PUSH);
			lex = Stage13(Lex());
			CodeAdd(CM_DIV);
			break;
		case '%':
			CodeAdd(CM_PUSH);
			lex = Stage13(Lex());
			CodeAdd(CM_MOD);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage11(CScriptLex *lex)
{
/****
	|	expr '+' expr
	|	expr '-' expr
	|	expr '#' expr
****/
	lex = Stage12(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case '+':
			CodeAdd(CM_PUSH);
			lex = Stage12(Lex());
			CodeAdd(CM_ADD);
			break;
		case '-':
			CodeAdd(CM_PUSH);
			lex = Stage12(Lex());
			CodeAdd(CM_SUB);
			break;
		case '#':
			CodeAdd(CM_PUSH);
			lex = Stage12(Lex());
			CodeAdd(CM_CAT);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage10(CScriptLex *lex)
{
/****
	|	expr '<<' expr
	|	expr '>>' expr
****/
	lex = Stage11(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case LEX_LEFT:
			CodeAdd(CM_PUSH);
			lex = Stage11(Lex());
			CodeAdd(CM_SFL);
			break;
		case LEX_RIGHT:
			CodeAdd(CM_PUSH);
			lex = Stage11(Lex());
			CodeAdd(CM_SFR);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage09(CScriptLex *lex)
{
/****
	|	expr '<' expr
	|	expr '>' expr
	|	expr '<=' expr
	|	expr '>=' expr
****/
	lex = Stage10(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case '>':
			CodeAdd(CM_PUSH);
			lex = Stage10(Lex());
			CodeAdd(CM_BIG);
			break;
		case '<':
			CodeAdd(CM_PUSH);
			lex = Stage10(Lex());
			CodeAdd(CM_SML);
			break;
		case LEX_BIG:
			CodeAdd(CM_PUSH);
			lex = Stage10(Lex());
			CodeAdd(CM_BIGE);
			break;
		case LEX_SML:
			CodeAdd(CM_PUSH);
			lex = Stage10(Lex());
			CodeAdd(CM_SMLE);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage08(CScriptLex *lex)
{
/****
	|	expr '==' expr
	|	expr '!=' expr
****/
	lex = Stage09(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case LEX_EQU:
			CodeAdd(CM_PUSH);
			lex = Stage09(Lex());
			CodeAdd(CM_EQU);
			break;
		case LEX_UNEQU:
			CodeAdd(CM_PUSH);
			lex = Stage09(Lex());
			CodeAdd(CM_UNEQU);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage07(CScriptLex *lex)
{
/****
	|	expr '&' expr
	|	expr '^' expr
	|	expr '|' expr
****/
	lex = Stage08(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case '&':
			CodeAdd(CM_PUSH);
			lex = Stage08(Lex());
			CodeAdd(CM_AND);
			break;
		case '^':
			CodeAdd(CM_PUSH);
			lex = Stage08(Lex());
			CodeAdd(CM_XOR);
			break;
		case '|':
			CodeAdd(CM_PUSH);
			lex = Stage08(Lex());
			CodeAdd(CM_OR);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage06(CScriptLex *lex)
{
/****
	|	expr '&&' expr
	|	expr '||' expr
****/
	int n;
	lex = Stage07(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case LEX_ANDAND:
			CodeAdd(CM_IFN);
			n = CodeAddPos(0);
			lex = Stage07(Lex());
			CodeSetPos(n);
			break;
		case LEX_OROR:
			CodeAdd(CM_IF);
			n = CodeAddPos(0);
			lex = Stage07(Lex());
			CodeSetPos(n);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage05(CScriptLex *lex)
{
/****
	|	expr '?' expr ':' expr
****/
	int a, b;
	lex = Stage06(lex);
	for ( ; ; ) {
		if ( lex->m_Type == '?' ) {
			CodeAdd(CM_IFN);
			a = CodeAddPos(0);
			lex = Stage06(Lex());
			CodeAdd(CM_JMP);
			b = CodeAddPos(0);
			if ( lex->m_Type != ':' )
				THROW(lex);
			CodeSetPos(a);
			lex = Stage06(Lex());
			CodeSetPos(b);
		} else
			return lex;
	}
}
CScriptLex *CScript::Stage04(CScriptLex *lex)
{
/****
	|	expr '=' expr			# 右から左
	|	expr '*=' expr
	|	expr '/=' expr
	|	expr '%=' expr
	|	expr '+=' expr
	|	expr '-=' expr
	|	expr '>>=' expr
	|	expr '<<=' expr
	|	expr '&=' expr
	|	expr '^=' expr
	|	expr '|=' expr
	|	expr '#=' expr
****/
	int n;
	lex = Stage05(lex);
	for ( ; ; ) {
		switch(lex->m_Type) {
		case '=':
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_SAVE);
			break;
		case LEX_MULLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_MUL);
			CodeAdd(CM_SAVE);
			break;
		case LEX_DIVLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_DIV);
			CodeAdd(CM_SAVE);
			break;
		case LEX_MODLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_MOD);
			CodeAdd(CM_SAVE);
			break;
		case LEX_ADDLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_ADD);
			CodeAdd(CM_SAVE);
			break;
		case LEX_SUBLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_SUB);
			CodeAdd(CM_SAVE);
			break;
		case LEX_LEFTLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_SFL);
			CodeAdd(CM_SAVE);
			break;
		case LEX_RIGHTLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_SFR);
			CodeAdd(CM_SAVE);
			break;
		case LEX_ANDLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_AND);
			CodeAdd(CM_SAVE);
			break;
		case LEX_XORLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_XOR);
			CodeAdd(CM_SAVE);
			break;
		case LEX_ORLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_OR);
			CodeAdd(CM_SAVE);
			break;
		case LEX_CATLET:
			n = IdentChack(lex);
			CodeSet(n, CM_PUSH);
			CodeAdd(CM_IDENT);
			CodeAdd(CM_PUSH);
			lex = Stage04(Lex());
			CodeAdd(CM_CAT);
			CodeAdd(CM_SAVE);
			break;
		default:
			return lex;
		}
	}
}
CScriptLex *CScript::Stage03(CScriptLex *lex)
{
/****
	|	expr ',' expr
****/
	lex = Stage04(lex);
	for ( ; ; ) {
		if ( lex->m_Type == ',' )
			lex = Stage04(Lex());
		else
			return lex;
	}
}
CScriptLex *CScript::Stage02(CScriptLex *lex)
{
/****
	|	expr ';'
	|	IF '(' expr ')' stmt
	|	IF '(' expr ')' stmt ELSE stmt
	|	WHILE '(' expr ')' stmt
	|	DO stmts WHILE '(' expr ')' ';'
	|	FOR '(' expr ';' expr ';' expr')' stmt
	|	BREAK ';'
	| 	CONTINUE ';'
	|	RETURN expr ';'
	|	SWITCH '(' expr ')' stmt
	|	CASE expr ':'
	|	DEFAULT ':'
	|	FUNCTION label '(' expr ')' stmt
	|	VAR expr
	|	CLASS label stmt
****/
	int a, b, c, d, e;

	switch(lex->m_Type) {
	case LEX_IF:
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		if ( lex->m_Type != ')' )
			THROW(lex);
		CodeAdd(CM_IFN);
		a = CodeAddPos(0);
		lex = Stage01(Lex());
		if ( lex->m_Type == LEX_ELSE ) {
			CodeAdd(CM_JMP);
			b = CodeAddPos(0);
			CodeSetPos(a);
			lex = Stage01(Lex());
			CodeSetPos(b);
		} else
			CodeSetPos(a);
		break;

	case LEX_WHILE:
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		LoopPush();
		lex = Stage03(Lex());
		if ( lex->m_Type != ')' )
			THROW(lex);
		CodeAdd(CM_IFN);
		LoopAdd(CodeAddPos(0));
		lex = Stage01(Lex());
		CodeAdd(CM_JMP);
		CodeAddPos(m_Loops->m_StartPos);
		LoopPop();
		break;

	case LEX_DO:
		LoopPush();
		lex = Stage01(Lex());
		if ( lex->m_Type != LEX_WHILE )
			THROW(lex);
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		if ( lex->m_Type != ')' )
			THROW(lex);
		lex = Lex();
		if ( lex->m_Type != ';' )
			THROW(lex);
		lex = Lex();
		CodeAdd(CM_IF);
		CodeAddPos(m_Loops->m_StartPos);
		LoopPop();
		break;

	case LEX_FOR:
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		if ( lex->m_Type != ';' )
			THROW(lex);

		LoopPush();
		lex = Stage03(Lex());
		if ( lex->m_Type != ';' )
			THROW(lex);
		CodeAdd(CM_IFN);
		LoopAdd(CodeAddPos(0));

		CodeAdd(CM_JMP);
		a = CodeAddPos(0);

		b = CodePos();
		lex = Stage03(Lex());
		if ( lex->m_Type != ')' )
			THROW(lex);
		CodeAdd(CM_JMP);
		CodeAddPos(m_Loops->m_StartPos);

		CodeSetPos(a);
		lex = Stage01(Lex());
		CodeAdd(CM_JMP);
		CodeAddPos(b);
		LoopPop();
		break;

	case LEX_RETURN:
		lex = Stage03(Lex());
		if ( lex->m_Type != ';' )
			THROW(lex);
		CodeAdd(CM_RET);
		lex = Lex();
		break;

	case LEX_BREAK:
		lex = Lex();
		if ( lex->m_Type != ';' )
			THROW(lex);
		if ( m_Loops == NULL )
			THROW(lex);
		CodeAdd(CM_JMP);
		LoopAdd(CodeAddPos(0));
		lex = Lex();
		break;
	case LEX_CONTINUE:
		lex = Lex();
		if ( lex->m_Type != ';' )
			THROW(lex);
		if ( m_Loops == NULL || m_Loops->m_StartPos == (-1) )
			THROW(lex);
		CodeAdd(CM_JMP);
		CodeAddPos(m_Loops->m_StartPos);
		lex = Lex();
		break;

	case LEX_SWITCH:
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		if ( lex->m_Type != ')' )
			THROW(lex);
		LoopPush();
		m_Loops->m_StartPos = (-1);
		CodeAdd(CM_PUSH);
		CodeAdd(CM_JMP);
		m_Loops->m_CaseAddr = CodeAddPos(0);
		lex = Stage01(Lex());
		if ( m_Loops->m_CaseAddr != (-1) )
			CodeSetPos(m_Loops->m_CaseAddr);
		LoopPop();
		CodeAdd(CM_POP);
		break;
	case LEX_CASE:
		if ( m_Loops == NULL )
			THROW(lex);
		CodeAdd(CM_JMP);
		a = CodeAddPos(0);
		if ( m_Loops->m_CaseAddr != (-1) ) {
			CodeSetPos(m_Loops->m_CaseAddr);
			m_Loops->m_CaseAddr = (-1);
		}
		lex = Stage04(Lex());
		if ( lex->m_Type != ':' )
			THROW(lex);
		lex = Lex();
		CodeAdd(CM_CASE);
		m_Loops->m_CaseAddr = CodeAddPos(0);
		CodeSetPos(a);
		break;
	case LEX_DEFAULT:
		if ( m_Loops == NULL )
			THROW(lex);
		if ( m_Loops->m_CaseAddr != (-1) ) {
			CodeSetPos(m_Loops->m_CaseAddr);
			m_Loops->m_CaseAddr = (-1);
		}
		lex = Lex();
		if ( lex->m_Type != ':' )
			THROW(lex);
		lex = Lex();
		break;

	case LEX_FUNCTION:
		{
			CScriptValue *sp;
			CScriptValue local;

			lex = Lex();
			if ( lex->m_Type != LEX_LABEL )
				THROW(lex);
			b = m_Class->Add((LPCSTR)*lex);
			sp = &((*m_Class)[b]);
			lex = Lex();

			while ( lex->m_Type == '.' ) {
				lex = Lex();
				if ( lex->m_Type != LEX_LABEL )
					THROW(lex);
				b = sp->Add((LPCSTR)*lex);
				sp = &((*sp)[b]);
				lex = Lex();
			}

			if ( lex->m_Type != '(' )
				THROW(lex);
			lex = Lex();
			while ( lex->m_Type != ')' ) {
				if ( lex->m_Type != LEX_LABEL )
					THROW(lex);
				local[(LPCSTR)*lex];
				lex = Lex();
				if ( lex->m_Type == ',' )
					lex = Lex();
			}

			CodeAdd(CM_JMP);
			a = CodeAddPos(0);
			sp->m_FuncPos = CodePos();
			sp->m_Next = m_Class;
			m_Class = sp;
			local.m_Next = m_Local;
			m_Local = &local;

			lex = Stage01(Lex());

			m_Local = m_Local->m_Next;
			m_Class = m_Class->m_Next;
			CodeAdd(CM_RET);
			CodeSetPos(a);
		}
		break;

	case LEX_CLASS:
		{
			lex = Lex();
			if ( lex->m_Type != LEX_LABEL )
				THROW(lex);
			CScriptValue *sp = &((*m_Class)[(LPCSTR)*lex]);
			sp->m_Next = m_Class;
			m_Class = sp;
			lex = Stage01(Lex());
			m_Class = sp->m_Next;
		}
		break;

	default:
		lex = Stage03(lex);
		if ( lex->m_Type != ';' )
			THROW(lex);
		lex = Lex();
	}
	return lex;
}
CScriptLex *CScript::Stage01(CScriptLex *lex)
{
/****
	|	stmt
	|	'{' stmts '}'
****/
	if ( lex->m_Type == '{' ) {
		lex = Lex();
		while ( lex->m_Type != '}' ) {
			if ( lex->m_Type == LEX_EOF )
				THROW(lex);
			lex = Stage01(lex);
		}
		lex = Lex();
	} else {
		m_SavPos = m_PosOld;
		lex = Stage02(lex);
	}

	return lex;
}
CScriptLex *CScript::Stage00(CScriptLex *lex)
{
/****
	|	stmts stmt
****/
	while ( lex->m_Type != LEX_EOF )
		lex = Stage01(lex);
	CodeAdd(CM_EOF);
	return lex;
}

int CScript::ParseBuff(LPBYTE buf, int len)
{
	INT_PTR pos = m_Code.GetSize();

	m_PosOld = m_SavPos = m_BufPos = 0;
	m_BufMax = len;
	m_BufPtr = buf;
	m_UnGetChar = 0;

	m_Class = &m_Data;
	m_Local = NULL;
	m_Loops = NULL;

	try {
		Stage00(Lex());
		return (int)pos;
	} catch (CScriptLex *lex) {
		m_Code.SetSize(pos);

		CScriptDlg dlg;
		dlg.m_FileName.Format("Syntax Error in '%s'", m_FileName);
		dlg.m_pSrc = m_BufPtr;
		dlg.m_Max  = m_BufMax;
		dlg.m_SPos = m_SavPos;
		dlg.m_EPos = m_BufPos;
		dlg.DoModal();
		return (-1);
	}
}
int CScript::LoadFile(LPCSTR filename)
{
	int n;
	int pos;
	CFile file;
	BYTE *buf;
	ULONGLONG size;
	CFileStatus stat;

	if ( !CFile::GetStatus(filename, stat) )
		return (-1);

	if ( (n = m_IncFile.Find(stat.m_szFullName)) >= 0 )
		return n;

	if( !file.Open(filename, CFile::modeRead) )
		return (-1);

	if ( (size = file.GetLength()) > (256 * 1024 * 1024) )
		return (-1);

	buf = new BYTE[(size_t)size + 1];
	buf[(int)size] = '\0';

	if ( file.Read(buf, (DWORD)size) != (DWORD)size )
		return (-1);

	file.Close();

	m_FileName = filename;
	if ( (pos = ParseBuff(buf, (int)size)) < 0 ) {
		delete buf;
		return (-1);
	}

	n = m_IncFile.Add((LPCSTR)stat.m_szFullName);
	m_IncFile[n] = (int)0;
	m_IncFile[n].m_FuncPos = pos;

	delete buf;
	return n;
}
int CScript::LoadStr(LPCSTR str)
{
	int n;
	int pos;
	CString tmp;

	tmp.Format("#%s$", str);

	if ( (n = m_IncFile.Find(tmp)) >= 0 )
		return n;

	m_FileName = "...";
	if ( (pos = ParseBuff((LPBYTE)str, (int)strlen(str))) < 0 )
		return (-1);

	n = m_IncFile.Add(tmp);
	m_IncFile[n] = (int)0;
	m_IncFile[n].m_FuncPos = pos;

	return n;
}

//////////////////////////////////////////////////////////////////////

void CScript::StackPush()
{
	CScriptValue *sp;
	
	if ( m_Free == NULL )
		m_Free = new CScriptValue;

	sp = m_Free;
	m_Free = sp->m_Next;

	sp->m_Next = m_Stack;
	m_Stack = sp;
}
void CScript::StackPop()
{
	CScriptValue *sp;

	if ( (sp = m_Stack) == NULL )
		return;
	m_Stack = sp->m_Next;

	sp->m_Next = m_Free;
	m_Free = sp;

	*sp = (int)0;
	sp->m_Buf.Clear();
	sp->RemoveAll();
}

int CScript::ExecSub(int pos, BOOL sub, CScriptValue *local)
{
	int n;
	int spos = pos;
	CScriptValue *sp, *dp;

#ifdef	_DEBUGXXX
	static struct { char *name; int val; } code_tab[] = {
		{ "LOAD",	1 },	{ "CLASS",	1 },	{ "LOCAL",	1 },
		{ "VALUE",	1 },	{ "SAVE",	0 },	{ "IDENT",	0 },
		{ "PUSH",	0 },	{ "POP",	0 },
		{ "ADDAR",	0 },	{ "ARRAY",	0 },	{ "MENBA",	1 },
		{ "INC",	0 },	{ "DEC",	0 },
		{ "AINC",	0 },	{ "ADEC",	0 },
		{ "NEG",	0 },	{ "COM",	0 },	{ "NOT",	0 },
		{ "MUL",	0 },	{ "DIV",	0 },	{ "MOD",	0 },
		{ "ADD",	0 },	{ "SUB",	0 },	{ "CAT",	0 },
		{ "SFL",	0 },	{ "SFR",	0 },
		{ "BIG",	0 },	{ "SML",	0 },
		{ "BIGE",	0 },	{ "SMLE",	0 },
		{ "EQU",	0 },	{ "UNEQU",	0 },
		{ "AND",	0 },	{ "XOR",	0 },	{ "OR",		0 },
		{ "IF",		1 },	{ "IFN",	1 },	{ "CASE",	1 },
		{ "JMP",	1 },	{ "RET",	0 },	{ "CALL",	1 },
		{ "ADDIDX",	0 },	{ "INDEX",	0 },
		{ "EOF",	0 },	{ "DEBUG",	1 },
	};

	while ( pos < (int)m_Code.GetSize() ) {
		TRACE("%s", code_tab[m_Code[pos]].name);
		for ( n = 0 ; n < code_tab[m_Code[pos]].val ; n++ )
			TRACE(" %d", m_Code[pos + n]);
		TRACE("\n");

		TRACE("Stack");
		for ( sp = m_Stack ; sp != NULL ; sp = sp->m_Next )
			TRACE("['%s':%d:'%s']", sp->m_Index, sp->m_Type, (LPCSTR)sp->m_Buf);
		TRACE("\n");
#else
	while ( pos < (int)m_Code.GetSize() && m_DoAbortFlag == FALSE ) {
#endif

		sp = m_Stack->m_Next;

		switch(m_Code[pos++]) {
		case CM_LOAD:
			n = m_Code[pos++];
			*m_Stack = &(m_Data[n]);
			break;
		case CM_CLASS:
			n = m_Code[pos++];
			*m_Stack = &((*((CScriptValue *)(m_Stack->m_Value.m_Ptr)))[n]);
			break;
		case CM_LOCAL:
			n = m_Code[pos++];
			*m_Stack = &((*local)[n]);
			break;
		case CM_VALUE:
			n = m_Code[pos++];
			*m_Stack = m_LexData[n];
			break;
		case CM_SAVE:
			if ( sp->m_Type == VALTYPE_LPBYTE )
				*((LPBYTE)(*sp)) = (BYTE)((int)(*m_Stack));
			else if ( sp->m_Type == VALTYPE_LPWCHAR )
				*((WCHAR *)(*sp)) = (WCHAR)((int)(*m_Stack));
			else if ( sp->m_Type == VALTYPE_IDENT ) {
				sp = (CScriptValue *)*sp;
				for ( dp = m_Stack ; dp->m_Type == VALTYPE_IDENT ; )
					dp = (CScriptValue *)(*dp);
				sp->m_Type    = dp->m_Type;
				sp->m_Value   = dp->m_Value;
				sp->m_Buf     = dp->m_Buf;
				sp->m_FuncPos = dp->m_FuncPos;
				sp->m_FuncExt = dp->m_FuncExt;
				sp->ArrayCopy(*dp);
			}
			StackPop();
			break;
		case CM_IDENT:
			if ( m_Stack->m_Type == VALTYPE_LPBYTE )
				*m_Stack = (int)(*((LPBYTE)*m_Stack));
			else if ( m_Stack->m_Type == VALTYPE_LPWCHAR )
				*m_Stack = (int)(*((WCHAR *)*m_Stack));
			//else if ( m_Stack->m_Type == VALTYPE_IDENT )
			//	*m_Stack = *((CScriptValue *)*m_Stack);
			break;
		case CM_PUSH:
			StackPush();
			break;
		case CM_POP:
			StackPop();
			break;

		case CM_ADDARRAY:
			if ( sp->m_Type == VALTYPE_IDENT )
				*(m_Stack->m_Next) = &((*((CScriptValue *)(*sp)))[(LPCSTR)NULL]);
			else
				*(m_Stack->m_Next) = (int)0;
			StackPop();
			break;

		case CM_ARRAY:
			if ( sp->m_Type == VALTYPE_IDENT ) {
				sp = (CScriptValue *)*sp;
				switch(m_Stack->GetType()) {
				case VALTYPE_INT:
				case VALTYPE_INT64:
					*(m_Stack->m_Next) = &((*sp)[(int)*m_Stack]);
					break;
				default:
					*(m_Stack->m_Next) = &((*sp)[(LPCSTR)*m_Stack]);
					break;
				}
			} else
				*(m_Stack->m_Next) = (int)0;
			StackPop();
			break;
		case CM_MENBA:
			if ( m_Stack->m_Type == VALTYPE_IDENT ) {
				sp = (CScriptValue *)(*m_Stack);
				n = m_Code[pos++];
				*m_Stack = &((*sp)[n]);
			}
			break;

		case CM_INC:
			if ( m_Stack->m_Type == VALTYPE_LPBYTE ) {
				*((LPBYTE)*m_Stack) = *((LPBYTE)*m_Stack) + 1;
				*m_Stack = (int)(*((LPBYTE)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_LPWCHAR ) {
				*((WCHAR *)*m_Stack) = *((WCHAR *)*m_Stack) + 1;
				*m_Stack = (int)(*((WCHAR *)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_IDENT ) {
				*((CScriptValue *)*m_Stack) = (int)*((CScriptValue *)*m_Stack) + 1;
				*m_Stack = (int)*((CScriptValue *)*m_Stack);
			} else
				*m_Stack = (int)0;
			break;
		case CM_DEC:
			if ( m_Stack->m_Type == VALTYPE_LPBYTE ) {
				*((LPBYTE)*m_Stack) = *((LPBYTE)*m_Stack) - 1;
				*m_Stack = (int)(*((LPBYTE)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_LPWCHAR ) {
				*((WCHAR *)*m_Stack) = *((WCHAR *)*m_Stack) - 1;
				*m_Stack = (int)(*((WCHAR *)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_IDENT ) {
				*((CScriptValue *)*m_Stack) = (int)*((CScriptValue *)*m_Stack) - 1;
				*m_Stack = (int)*((CScriptValue *)*m_Stack);
			} else
				*m_Stack = (int)0;
			break;
		case CM_AINC:
			if ( m_Stack->m_Type == VALTYPE_LPBYTE ) {
				n = (int)(*((LPBYTE)*m_Stack));
				*((LPBYTE)*m_Stack) = *((LPBYTE)*m_Stack) + 1;
				*m_Stack = n;
			} else if ( m_Stack->m_Type == VALTYPE_LPWCHAR ) {
				n = (int)(*((WCHAR *)*m_Stack));
				*((WCHAR *)*m_Stack) = *((WCHAR *)*m_Stack) + 1;
				*m_Stack = n;
			} else if ( m_Stack->m_Type == VALTYPE_IDENT ) {
				n = (int)*((CScriptValue *)*m_Stack);
				*((CScriptValue *)*m_Stack) = (int)*((CScriptValue *)*m_Stack) + 1;
				*m_Stack = n;
			} else
				*m_Stack = (int)0;
			break;
		case CM_ADEC:
			if ( m_Stack->m_Type == VALTYPE_LPBYTE ) {
				n = (int)(*((LPBYTE)*m_Stack));
				*((LPBYTE)*m_Stack) = *((LPBYTE)*m_Stack) - 1;
				*m_Stack = n;
			} else if ( m_Stack->m_Type == VALTYPE_LPWCHAR ) {
				n = (int)(*((WCHAR *)*m_Stack));
				*((WCHAR *)*m_Stack) = *((WCHAR *)*m_Stack) - 1;
				*m_Stack = n;
			} else if ( m_Stack->m_Type == VALTYPE_IDENT ) {
				n = (int)*((CScriptValue *)*m_Stack);
				*((CScriptValue *)*m_Stack) = (int)*((CScriptValue *)*m_Stack) - 1;
				*m_Stack = n;
			} else
				*m_Stack = (int)0;
			break;

		case CM_NEG:
			if ( m_Stack->GetType() == VALTYPE_DOUBLE )
				*m_Stack = 0.0 - (double)*m_Stack;
			else if ( m_Stack->GetType() == VALTYPE_INT64 )
				*m_Stack = 0 - (LONGLONG)*m_Stack;
			else
				*m_Stack = 0 - (int)*m_Stack;
			break;
		case CM_COM:
			*m_Stack = ~(int)*m_Stack;
			break;
		case CM_NOT:
			if ( m_Stack->GetType() == VALTYPE_STRING )
				*m_Stack = (int)(*((LPCSTR)*m_Stack) == '\0' ? 1 : 0);
			else if ( m_Stack->GetType() == VALTYPE_WSTRING )
				*m_Stack = (int)(*((LPCWSTR)*m_Stack) == L'\0' ? 1 : 0);
			else if ( m_Stack->GetType() == VALTYPE_INT64 )
				*m_Stack = (int)((LONGLONG)*m_Stack == 0 ? 1 : 0);
			else
				*m_Stack = (int)((int)*m_Stack == 0 ? 1 : 0);
			break;

		case CM_MUL:
			*sp *= *m_Stack;
			StackPop();
			break;
		case CM_DIV:
			*sp /= *m_Stack;
			StackPop();
			break;
		case CM_MOD:
			*sp %= *m_Stack;
			StackPop();
			break;
		case CM_ADD:
			*sp += *m_Stack;
			StackPop();
			break;
		case CM_SUB:
			*sp -= *m_Stack;
			StackPop();
			break;
		case CM_CAT:
			sp->AddStr(*m_Stack);
			StackPop();
			break;

		case CM_SFL:
			*sp = (int)*sp << (int)*m_Stack;
			StackPop();
			break;
		case CM_SFR:
			*sp = (int)*sp >> (int)*m_Stack;
			StackPop();
			break;

		case CM_BIG:
			*sp = (sp->Compare(0, m_Stack) > 0 ? 1 : 0);
			StackPop();
			break;
		case CM_SML:
			*sp = (sp->Compare(0, m_Stack) < 0 ? 1 : 0);
			StackPop();
			break;
		case CM_BIGE:
			*sp = (sp->Compare(0, m_Stack) >= 0 ? 1 : 0);
			StackPop();
			break;
		case CM_SMLE:
			*sp = (sp->Compare(0, m_Stack) <= 0 ? 1 : 0);
			StackPop();
			break;
		case CM_EQU:
			*sp = (sp->Compare(0, m_Stack) == 0 ? 1 : 0);
			StackPop();
			break;
		case CM_UNEQU:
			*sp = (sp->Compare(0, m_Stack) != 0 ? 1 : 0);
			StackPop();
			break;

		case CM_AND:
			*sp = (int)*sp & (int)*m_Stack;
			StackPop();
			break;
		case CM_XOR:
			*sp = (int)*sp ^ (int)*m_Stack;
			StackPop();
			break;
		case CM_OR:
			*sp = (int)*sp | (int)*m_Stack;
			StackPop();
			break;

		case CM_IF:
			if ( m_Stack->GetType() == VALTYPE_STRING ) {
				if ( *((LPCSTR)*m_Stack) != '\0' )
					pos = m_Code[pos];
				else
					pos++;
			} else if ( m_Stack->GetType() == VALTYPE_WSTRING ) {
				if ( *((LPCWSTR)*m_Stack) != L'\0' )
					pos = m_Code[pos];
				else
					pos++;
			} else if ( (int)*m_Stack != 0 )
				pos = m_Code[pos];
			else
				pos++;
			break;
		case CM_IFN:
			if ( m_Stack->GetType() == VALTYPE_STRING ) {
				if ( *((LPCSTR)*m_Stack) == '\0' )
					pos = m_Code[pos];
				else
					pos++;
			} else if ( m_Stack->GetType() == VALTYPE_WSTRING ) {
				if ( *((LPCWSTR)*m_Stack) == L'\0' )
					pos = m_Code[pos];
				else
					pos++;
			} else if ( (int)*m_Stack == 0 )
				pos = m_Code[pos];
			else
				pos++;
			break;
		case CM_CASE:
			if ( sp->Compare(0, m_Stack) != 0 )
				pos = m_Code[pos];
			else
				pos++;
			break;

		case CM_JMP:
			pos = m_Code[pos];
			break;
		case CM_RET:
			return TRUE;
		case CM_CALL:
			{
				int rt = FALSE;
				int st = m_Code[pos++];
				CScriptValue *fp;

				m_Stack->RemoveAll();
				m_Stack->m_Array.SetSize(st);
				for ( n = st - 1 ; n >= 0 ; n-- ) {
					m_Stack->m_Array[n] = sp;
					sp = sp->m_Next;
				}
				*m_Stack = (CScriptValue *)sp;		// acc

				if ( sp->m_Type == VALTYPE_IDENT ) {
					fp = (CScriptValue *)*sp;
					*sp = (int)0;
					if ( (n = fp->m_FuncPos) != (-1) ) {
						if ( (n & 0x8000) == 0 ) {
							fp = m_Stack;
							m_Stack = sp;
							ExecSub(n, FALSE, fp);
							m_Stack = fp;
						} else if (fp->m_FuncExt != NULL )
							rt = (*(fp->m_FuncExt))(n & 0x7FFF, this, *m_Stack);
					}
				}

				for ( n = 0 ; n < st ; n++ )
					m_Stack->m_Array[n] = NULL;

				for ( n = 0 ; n <= st ; n++ )
					StackPop();

				if ( rt )
					return TRUE;
			}
			break;

		case CM_ADDINDEX:
			if ( sp->m_Type == VALTYPE_IDENT ) {
				BYTE *p;
				sp = (CScriptValue *)(*sp);
				if ( sp->m_Type == VALTYPE_WSTRING ) {
					p = sp->m_Buf.PutSpc(sizeof(WCHAR));
					p[0] = p[1] = '\0';
					*(m_Stack->m_Next) = (WCHAR *)(p);
				} else {
					(LPCSTR)*sp;
					sp->m_Type = VALTYPE_STRING;
					p = sp->m_Buf.PutSpc(sizeof(char));
					*p = '\0';
					*(m_Stack->m_Next) = (LPBYTE)(p);
				}
			}
			StackPop();
			break;
		case CM_INDEX:
			if ( sp->m_Type == VALTYPE_IDENT ) {
				int n, i;
				BYTE *p;
				n = (int)*m_Stack;
				sp = (CScriptValue *)(*(m_Stack->m_Next));
				if ( sp->m_Type == VALTYPE_WSTRING ) {
					n *= sizeof(WCHAR);
					if ( (i = n - sp->m_Buf.GetSize() + 2) > 0 ) {
						p = sp->m_Buf.PutSpc(i);
						memset(p, 0, i);
					}
					*(m_Stack->m_Next) = (WCHAR *)(sp->m_Buf.GetPos(n));
				} else {
					(LPCSTR)*sp;
					sp->m_Type = VALTYPE_STRING;
					if ( (i = n - sp->m_Buf.GetSize() + 1) > 0 ) {
						p = sp->m_Buf.PutSpc(i);
						memset(p, 0, i);
					}
					*(m_Stack->m_Next) = (LPBYTE)(sp->m_Buf.GetPos(n));
				}
			}
			StackPop();
			break;

		case CM_VALCLEAR:
			*m_Stack = (int)0;
			m_Stack->RemoveAll();
			break;
		case CM_VALWITHKEY:
			if ( m_Stack->m_Next->GetType() == VALTYPE_INT )
				sp = &((*(m_Stack->m_Next->m_Next))[(int)(*(m_Stack->m_Next))]);
			else
				sp = &((*(m_Stack->m_Next->m_Next))[(LPCSTR)(*(m_Stack->m_Next))]);
			*sp = *m_Stack;
			StackPop();
			break;
		case CM_VALARRAY:
			sp = &((*(m_Stack->m_Next))[(LPCSTR)NULL]);
			*sp = *m_Stack;
			break;

		case CM_EOF:
			return FALSE;

		case CM_DEBUG:
			break;
		}
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////

int CScript::Func08(int cmd, class CScript *owner, CScriptValue &local)
{
	int n;
	CRegEx *rp;
	CRegExRes res;
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:		// regopen(r)
		rp = new CRegEx;
		rp->Compile((LPCSTR)local[0]);
		rp->MatchCharInit();
		(*acc) = (void *)rp;
		break;
	case 1:		// regclose(a)
		if ( local[0].GetType() != VALTYPE_PTR )
			break;
		rp = (CRegEx *)(void *)local[0];
		delete rp;
		break;
	case 2:		// regchar(c, a)
		if ( local[1].GetType() != VALTYPE_PTR )
			break;
		rp = (CRegEx *)(void *)local[1];
		if ( rp->MatchChar((int)local[0], 0, &res) && (res.m_Status == REG_MATCH || res.m_Status == REG_MATCHOVER) ) {
			(*acc) = (int)res.m_Data.GetSize();
			acc->RemoveAll();
			for ( n = 0 ; n < res.GetSize() ; n++ )
				(*acc)[n] = (LPCSTR)(res.m_Data[n].m_Str);
		} else
			(*acc) = (int)0;
		break;
	case 3:		// regstr(s, a)
		if ( local[1].GetType() != VALTYPE_PTR )
			break;
		rp = (CRegEx *)(void *)local[1];
		if ( rp->MatchStr((LPCSTR)local[0], &res) ) {
			(*acc) = (int)res.m_Data.GetSize();
			acc->RemoveAll();
			for ( n = 0 ; n < (int)res.m_Data.GetSize() ; n++ )
				(*acc)[n] = (LPCSTR)(res.m_Data[n].m_Str);
		} else
			(*acc) = (int)0;
		break;
	}
	return FALSE;
}
int CScript::Func07(int cmd, class CScript *owner, CScriptValue &local)
{
	if ( owner->m_pWnd == NULL || owner->m_pDoc == NULL )
		return FALSE;

	CBuffer *bp;
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:		// cgetc(s)
		if ( !owner->m_ConsOpen )
			(*acc) = (int)EOF;
		//else
		//	(*acc) = (int)owner->m_SockFifo.GetWChar((int)local[0]);
		break;
	case 1:		// cgets(s)
		if ( !owner->m_ConsOpen )
			(*acc) = L"";
		else {
			CStringW tmp;
			//owner->m_SockFifo.WReadLine(tmp, (int)local[0]);
			(*acc) = (LPCWSTR)tmp;
		}
		break;
	case 2:		// cputc(c)
		{
			CStringW tmp;
			tmp += (WCHAR)(int)local[0];
			owner->SendString(tmp);
		}
		break;
	case 3:		// cputs(c)
		owner->SendScript((LPCWSTR)local[0]);
		break;

	case 4:		// sgetc(s);
		owner->Bufferd_Recive((int)local[0]);
		break;
	case 5:		// sgets(s);
		{
			int ch, sec;
			acc->m_Type = VALTYPE_STRING;
			acc->m_Buf.Clear();
			sec = (int)local[0];
			while ( (ch = owner->Bufferd_Recive(sec)) != EOF ) {
				acc->m_Buf.Put8Bit(ch);
				if ( ch == '\n' )
					break;
			}
		}
		break;
	case 6:		// sputc(s);
		owner->Bufferd_Send((int)local[0]);
		owner->Bufferd_Flush();
		break;
	case 7:		// sputs(s);
		bp = local[0].GetBuf();
		owner->Bufferd_SendBuf((char *)bp->GetPtr(), bp->GetSize());
		owner->Bufferd_Flush();
		break;
	case 8:		// sread(n, s);
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.Clear();
		owner->Bufferd_ReciveBuf((char *)acc->m_Buf.PutSpc((int)local[0]), (int)local[0], (int)local[1]);
		break;
	case 9:		// swrite(s);
		bp = local[0].GetBuf();
		owner->Bufferd_SendBuf((char *)bp->GetPtr(), bp->GetSize());
		owner->Bufferd_Flush();
		break;
	case 10:	// sync(m)
		owner->m_SyncMode = (int)local[0];
		if ( owner->m_pDoc != NULL && owner->m_pDoc->m_pSock != NULL )
			owner->m_pDoc->m_pSock->SetRecvSyncMode(owner->m_SyncMode);
		break;
	case 11:	// xonxoff(s)
		owner->SetXonXoff((int)local[0]);
		break;

	case 12:	// filecheck(m, f)
		(*acc) = (LPCSTR)owner->CheckFileName((int)local[0], (LPCSTR)local[1]);
		break;
	case 13:	// yesno(m)
		(*acc) = (int)owner->YesOrNo((LPCSTR)local[0]);
		break;
	case 14:	// abort()
		(*acc) = (int)owner->AbortCheck();
		break;
	case 15:	// updownopen()
		owner->UpDownOpen((LPCSTR)local[0]);
		break;
	case 16:	// updownclose()
		owner->UpDownClose();
		break;
	case 17:	// updownmsg(s)
		owner->UpDownMessage((char *)(LPCSTR)local[0]);
		break;
	case 18:	// updownrange(m)
		owner->UpDownInit((int)local[0]);
		break;
	case 19:	// updownpos()
		owner->UpDownStat((int)local[0]);
		break;
	}
	return FALSE;
}
int CScript::Func06(int cmd, class CScript *owner, CScriptValue &local)
{
	CBuffer *bp;
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:	// iconv(f, t, s)
		{
			CIConv iconv;
			acc->m_Type = VALTYPE_STRING;
			iconv.IConvBuf((LPCSTR)local[0], (LPCSTR)local[1], local[2].GetBuf(), &(acc->m_Buf));
		}
		break;

	case 1:	// md5(s, f)
	case 2:	// sha1(s, f)
		{
			unsigned int dlen;
			const EVP_MD *evp_md;
			u_char digest[EVP_MAX_MD_SIZE];
			EVP_MD_CTX md;

			evp_md = (cmd == 1 ? EVP_md5() : EVP_sha1());

			bp = local[0].GetBuf();
			EVP_DigestInit(&md, evp_md);
			EVP_DigestUpdate(&md, (LPBYTE)bp->GetPtr(), bp->GetSize());
			EVP_DigestFinal(&md, digest, &dlen);

			acc->m_Type = VALTYPE_STRING;
			acc->m_Buf.Clear();
			if ( (int)local[1] == 0 )
				acc->m_Buf.Base16Encode(digest, dlen);
			else
				acc->m_Buf.Apend((LPBYTE)digest, (int)dlen);
		}
		break;

	case 3:	// base64e(s)
		bp = local[0].GetBuf();
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.Base64Encode(bp->GetPtr(), bp->GetSize());
		break;
	case 4:	// base64d(s)
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.Base64Decode((LPCSTR)local[0]);
		break;
	case 5:	// quotede(s)
		bp = local[0].GetBuf();
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.QuotedEncode(bp->GetPtr(), bp->GetSize());
		break;
	case 6:	// quotedd(s)
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.QuotedDecode((LPCSTR)local[0]);
		break;
	case 7:	// base16e(s)
		bp = local[0].GetBuf();
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.Base16Encode(bp->GetPtr(), bp->GetSize());
		break;
	case 8:	// base16d(s)
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.Base16Decode((LPCSTR)local[0]);
		break;

	case 9:	// crc16(s, c);
		{
			int c, v;
			c = (int)local[0] & 0xFFFF;
			v = (int)local[1] & 0x00FF;
			extern const unsigned short CRCTable[];
			(*acc) = (int)(CRCTable[((c >> 8) ^ v) & 0xFF] ^ (c << 8));
		}
		break;
	case 10:// crc32(s, c);
		{
			int c, v;
			c = (int)local[0] & 0xFFFFFFFF;
			v = (int)local[1] & 0x000000FF;
			extern long c32tab[];
			(*acc) = (int)(c32tab[((c >> 8) ^ v) & 0xFF] ^ ((c << 8) & 0x00FFFFFF));
		}
		break;
    }
	return FALSE;
}
int CScript::Func05(int cmd, class CScript *owner, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:	// imadd(a, b)
		(*acc)    = (double)local[0]    + (double)local[1];
		(*acc)[0] = (double)local[0][0] + (double)local[1][0];
		break;
	case 1:	// imsub(a, b)
		(*acc)    = (double)local[0]    - (double)local[1];
		(*acc)[0] = (double)local[0][0] - (double)local[1][0];
		break;
	case 2:	// immul(a, b)
		(*acc)    = (double)local[0] * (double)local[1]    - (double)local[0][0] * (double)local[1][0];
		(*acc)[0] = (double)local[0] * (double)local[1][0] + (double)local[0][0] * (double)local[1];
		break;
	case 3:	// imdiv(a, b)
		{
			double d = pow((double)local[1], 2) + pow((double)local[1][0], 2);
			(*acc)    = ((double)local[0]    * (double)local[1] + (double)local[0][0] * (double)local[1][0]) / d;
			(*acc)[0] = ((double)local[0][0] * (double)local[1] - (double)local[0]    * (double)local[1][0]) / d;
		}
		break;
	case 4:	// impow(a, b)
		{
			double r = sqrt(pow((double)local[0], 2) + pow((double)local[0][0], 2));
			double s = atan2((double)local[0][0], (double)local[0]);
			(*acc)    = pow(r, (double)local[1]) * cos((double)local[1] * s);
			(*acc)[0] = pow(r, (double)local[1]) * sin((double)local[1] * s);
		}
		break;
	case 5:	// imsqrt(a)
		{
			double r = sqrt(pow((double)local[0], 2) + pow((double)local[0][0], 2));
			double s = atan2((double)local[0][0], (double)local[0]);
			(*acc)    = sqrt(r) * cos(s / 2);
			(*acc)[0] = sqrt(r) * sin(s / 2);
		}
		break;
	case 6:	// imsin(a)
		(*acc)    = sin((double)local[0]) * cosh((double)local[0][0]);
		(*acc)[0] = cos((double)local[0]) * sinh((double)local[0][0]);
		break;
	case 7:	// imcos(a)
		(*acc)    = cos((double)local[0]) * cosh((double)local[0][0]);
		(*acc)[0] = 0.0 - sin((double)local[0]) * sinh((double)local[0][0]);
		break;
	case 8:	// imtan(a)
		{
			CScriptValue tmp;
			tmp = acc;
			Func05(6, owner, local);	// imsin
			tmp[0] = *acc;
			Func05(7, owner, local);	// imcos
			tmp[1] = *acc;
			Func05(3, owner, tmp);		// imdiv
		}
		break;
	case 9:	// imlog(a)
		{
			double r = sqrt(pow((double)local[0], 2) + pow((double)local[0][0], 2));
			double s = atan2((double)local[0][0], (double)local[0]);
			(*acc)    = log(r);
			(*acc)[0] = s;
		}
		break;
	case 10:	// imexp(a)
		(*acc)    = exp((double)local[0]) * cos((double)local[0][0]);
		(*acc)[0] = exp((double)local[0]) * sin((double)local[0][0]);
		break;
	case 11:	// imabs(a)
		(*acc) = sqrt(pow((double)local[0], 2) + pow((double)local[0][0], 2));
		break;
	case 12:	// imarg(a)
		(*acc) = atan2((double)local[0][0], (double)local[0]);
		break;
	}
	return FALSE;
}
int CScript::Func04(int cmd, class CScript *owner, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:		// rand
		(*acc) = (int)rand();
		break;
	case 1:		// srand
		srand((int)local[0]);
		break;
	case 2:	// floor
		(*acc) = floor((double)local[0]);
		break;
	case 3:	// asin
		(*acc) = asin((double)local[0]);
		break;
	case 4:	// sqrt
		(*acc) = sqrt((double)local[0]);
		break;
	case 5:	// tan
		(*acc) = tan((double)local[0]);
		break;
	case 6:	// acos
		(*acc) = acos((double)local[0]);
		break;
	case 7:	// abs
		if ( local[0].GetType() == VALTYPE_INT )
			(*acc) = abs((int)local[0]);
		else
			(*acc) = abs((double)local[0]);
		break;
	case 8:	// cos
		(*acc) = cos((double)local[0]);
		break;
	case 9:	// exp
		(*acc) = exp((double)local[0]);
		break;
	case 10:	// fmod
		(*acc) = fmod((double)local[0], (double)local[1]);
		break;
	case 11:	// atan
		(*acc) = atan((double)local[0]);
		break;
	case 12:	// sin
		(*acc) = sin((double)local[0]);
		break;
	case 13:	// pow
		(*acc) = pow((double)local[0], (double)local[1]);
		break;
	case 14:	// ceil
		(*acc) = ceil((double)local[0]);
		break;
	case 15:	// sinh
		(*acc) = sinh((double)local[0]);
		break;
	case 16:	// cosh
		(*acc) = cosh((double)local[0]);
		break;
	case 17:	// tanh
		(*acc) = tanh((double)local[0]);
		break;
	case 18:	// log
		(*acc) = log((double)local[0]);
		break;
	case 19:	// log10
		(*acc) = log10((double)local[0]);
		break;
	case 20:	// atan2
		(*acc) = atan2((double)local[0], (double)local[1]);
		break;
	}
	return FALSE;
}
int CScript::Func03(int cmd, class CScript *owner, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:		// f=fopen(n,m,s)
		if ( local.GetSize() > 2 ) {
			int n = _SH_DENYRW;
			switch((int)local[2]) {
			case 0: n = _SH_DENYNO; break;		// 読み出しアクセスおよび書き込みアクセスを許可します
			case 1: n = _SH_DENYRD; break;		// ファイルの読み出しを禁止します
			case 2: n = _SH_DENYWR; break;		// ファイルの書き込みを禁止します
			case 3: n = _SH_DENYRW; break;		// ファイルの読み出しと書き込みを禁止します
			}
	 		(*acc) = (void *)_fsopen((LPCSTR)local[0], (LPCSTR)local[1], n);
		} else
	 		(*acc) = (void *)fopen((LPCSTR)local[0], (LPCSTR)local[1]);
		break;
	case 1:		// fclose(f)
		fclose((FILE *)(void *)local[0]);
		break;
	case 2:		// fread(s, f)
		{
			int n;
			CBuffer tmp;
			n = (int)local[0];
			n = (int)fread(tmp.PutSpc(n), 1, n, (FILE *)(void *)local[1]);
			acc->m_Type = VALTYPE_STRING;
			acc->m_Buf.Clear();
			acc->m_Buf.Apend(tmp.GetPtr(), n);
		}
		break;
	case 3:		// fwrite(b, f)
		{
			CBuffer *bp = local[0].GetBuf();
			(*acc) = (int)fwrite(bp->GetPtr(), 1, bp->GetSize(), (FILE *)(void *)local[1]);
		}
		break;
	case 4:		// fgets(f)
		{
			char *p;
			char tmp[4096];
			if ( (p = fgets(tmp, 4096, (FILE *)(void *)local[0])) != NULL )
				(*acc) = (LPCSTR)p;
			else
				(*acc) = (int)0;
		}
		break;
	case 5:		// fputs(s, f)
		(*acc) = (int)fputs((LPCSTR)local[0], (FILE *)(void *)local[1]);
		break;
	case 6:		// fgetc(f)
		(*acc) = (int)fgetc((FILE *)(void *)local[0]);
		break;
	case 7:		// fputc(c, f)
		(*acc) = (int)fputc((int)local[0], (FILE *)(void *)local[1]);
		break;
	case 8:		// feof
		(*acc) = (int)feof((FILE *)(void *)local[0]);
	case 9:		// ferror
		(*acc) = (int)ferror((FILE *)(void *)local[0]);
		break;
	case 10:	// fflush
		(*acc) = (int)fflush((FILE *)(void *)local[0]);
		break;
	case 11:	// ftell
		(*acc) = (LONGLONG)_ftelli64((FILE *)(void *)local[0]);
		break;
	case 12:	// fseek(f, o, s);
		// 0 = SEEK_SET		ファイルの先頭。
		// 1 = SEEK_CUR		ファイル ポインタの現在位置。
		// 2 = SEEK_END		ファイルの終端。
		(*acc) = (LONGLONG)_fseeki64((FILE *)(void *)local[0], (LONGLONG)local[1], (int)local[2]);
		break;

	case 13:	// file(f)
		{
			int n;
			FILE *fp;
			char tmp[4096];
			acc->RemoveAll();
			if ( (fp = fopen((LPCSTR)local[0], "r")) != NULL ) {
				for ( n = 0 ; fgets(tmp, 4096, fp) != NULL; n++ )
					(*acc)[(LPCSTR)NULL] = (LPCSTR)tmp;
				(*acc) = (int)n;
				fclose(fp);
			} else
				(*acc) = (int)0;
		}
		break;
	case 14:	// stat(f)
		{
			struct _stat32i64 st;
			if ( _stat32i64((LPCSTR)local[0], &st) == 0 ) {
				(*acc) = (int)1;
				acc->RemoveAll();
				(*acc)["dev"]   = (int)st.st_dev;
				(*acc)["ino"]   = (int)st.st_ino;
				(*acc)["mode"]  = (int)st.st_mode;
				(*acc)["nlink"] = (int)st.st_nlink;
				(*acc)["uid"]   = (int)st.st_uid;
				(*acc)["gid"]   = (int)st.st_gid;
				(*acc)["rdev"]  = (int)st.st_rdev;
				(*acc)["size"]  = (LONGLONG)st.st_size;
				(*acc)["atime"] = (int)st.st_atime;
				(*acc)["mtime"] = (int)st.st_mtime;
				(*acc)["ctime"] = (int)st.st_ctime;
			} else
				(*acc) = (int)0;
		}
		break;

	case 15:	// basename(f)
		{
			int n;
			CString tmp;
			tmp = (LPCSTR)local[0];
			if ( (n = tmp.ReverseFind('\\')) >= 0 || (n = tmp.ReverseFind(':')) >= 0 || (n = tmp.ReverseFind('/')) >= 0 )
				(*acc) = (LPCSTR)tmp.Mid(n + 1);
			else
				(*acc) = (LPCSTR)tmp;
		}
		break;
	case 16:	// dirname(f)
		{
			int n;
			CString tmp;
			tmp = (LPCSTR)local[0];
			if ( (n = tmp.ReverseFind('\\')) >= 0 || (n = tmp.ReverseFind(':')) >= 0 || (n = tmp.ReverseFind('/')) >= 0 )
				(*acc) = (LPCSTR)tmp.Left(n);
			else
				(*acc) = (LPCSTR)"";
		}
		break;

	case 17:	// copy(s, t)
		(*acc) = (int)CopyFile((LPCSTR)local[0], (LPCSTR)local[1], TRUE);
		break;
	case 18:	// rename(s, t)
		(*acc) = (int)MoveFile((LPCSTR)local[0], (LPCSTR)local[1]);
		break;
	case 19:	// delete(s)
		(*acc) = (int)DeleteFile((LPCSTR)local[0]);
		break;
	case 20:	// getcwd()
		{
			CHAR tmp[_MAX_DIR];
			if ( GetCurrentDirectory(_MAX_DIR, tmp) == 0 )
				tmp[0] = '\0';
			(*acc) = (LPCSTR)tmp;
		}
		break;
	case 21:	// chdir(d)
		(*acc) = (int)SetCurrentDirectory((LPCSTR)local[0]);
		break;

	case 22:	// play(s)
		PlaySound((LPCSTR)local[0], NULL, SND_ASYNC | SND_FILENAME);
		break;
	}
	return FALSE;
}
int CScript::Func02(int cmd, class CScript *owner, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:		// substr(s, m, n)
		{
			if ( local[0].GetType() == VALTYPE_WSTRING ) {
				CStringW tmp;
				tmp = (LPCWSTR)local[0];
				(*acc) = (LPCWSTR)tmp.Mid((int)local[1], (int)local[2]);
			} else {
				CString tmp;
				tmp = (LPCSTR)local[0];
				(*acc) = (LPCSTR)tmp.Mid((int)local[1], (int)local[2]);
			}
		}
		break;
	case 1:		// strstr(s, t)
		{
			if ( local[0].GetType() == VALTYPE_WSTRING ) {
				CStringW tmp;
				tmp = (LPCWSTR)local[0];
				(*acc) = (int)tmp.Find((LPCWSTR)local[1]);
			} else {
				CString tmp;
				tmp = (LPCSTR)local[0];
				(*acc) = (int)tmp.Find((LPCSTR)local[1]);
			}
		}
		break;

	case 2:		// ereg(r, s)
		{
			int n;
			CRegEx reg;
			CRegExRes res;
			reg.Compile((LPCSTR)local[0]);
			if ( reg.MatchStr((LPCSTR)local[1], &res) ) {
				(*acc) = (int)res.m_Data.GetSize();
				acc->RemoveAll();
				for ( n = 0 ; n < (int)res.m_Data.GetSize() ; n++ )
					(*acc)[n] = (LPCSTR)(res.m_Data[n].m_Str);
			} else
				(*acc) = (int)0;
		}
		break;
	case 3:		// replace(r, t, s)
		{
			CString tmp;
			CRegEx reg;
			reg.Compile((LPCSTR)local[0]);
			if ( reg.ConvertStr((LPCSTR)local[2], (LPCSTR)local[1], tmp) )
				(*acc) = (LPCSTR)tmp;
		}
		break;
	case 4:	// split(r, s)
		{
			int n;
			CRegEx reg;
			CStringArray tmp;
			reg.Compile((LPCSTR)local[0]);
			if ( reg.SplitStr((LPCSTR)local[1], tmp) ) {
				(*acc) = (int)tmp.GetSize();
				acc->RemoveAll();
				for ( n = 0 ; n < (int)tmp.GetSize() ; n++ )
					(*acc)[n] = (LPCSTR)tmp[n];
			} else
				(*acc) = (int)0;
		}
		break;

	case 5:	// sprintf(fmt, expr, ... )
		{
			int n = 0;
			BOOL ll;
			CString tmp[3];
			LPCSTR str;
			str = (LPCSTR)local[n++];
			while ( *str != '\0' ) {
				if ( *str == '%' ) {
					tmp[1].Empty();
					tmp[2].Empty();
					tmp[1] += *(str++);
					ll = FALSE;

					if ( *str == '-' || *str == '+' || *str == '0' || *str == ' ' || *str == '#' )
						tmp[1] += *(str++);

					while ( (*str >= '0' && *str <= '9') || *str == '.' || *str == '*' )
						tmp[1] += *(str++);

					if ( *str == 'h' )
						tmp[1] += *(str++);
					else if ( str[0] == 'l' && str[1] == 'l' ) {
						tmp[1] += *(str++);
						tmp[1] += *(str++);
						ll = TRUE;
					} else if ( *str == 'l' )
						tmp[1] += *(str++);
					else if ( str[0] == 'I' && str[1] == '3' && str[2] == '2' ) {
						tmp[1] += *(str++);
						tmp[1] += *(str++);
						tmp[1] += *(str++);
					} else if ( str[0] == 'I' && str[1] == '6' && str[2] == '4' ) {
						tmp[1] += *(str++);
						tmp[1] += *(str++);
						tmp[1] += *(str++);
						ll = TRUE;
					} else if ( *str == 'I' )
						tmp[1] += *(str++);

					tmp[1] += *str;
					switch(*(str++)) {
					case 'c': case 'C':
						tmp[2].Format(tmp[1], (int)local[n++]);
						break;
					case 'd': case 'i': case 'o': case 'u':
					case 'x': case 'X':
						if ( ll )
							tmp[2].Format(tmp[1], (LONGLONG)local[n++]);
						else if ( local[n].GetType() == VALTYPE_INT64 ) {
							tmp[1].Insert(tmp[1].GetLength() - 1, "I64");
							tmp[2].Format(tmp[1], (LONGLONG)local[n++]);
						} else
							tmp[2].Format(tmp[1], (int)local[n++]);
						break;
					case 'e': case 'E':
					case 'f':
					case 'g': case 'G':
					case 'a': case 'A':
						tmp[2].Format(tmp[1], (double)local[n++]);
						break;
					case 's': case 'S':
						tmp[2].Format(tmp[1], (LPCSTR)local[n++]);
						break;

					case '%':
						tmp[2] += '%';
						break;
					default:
						tmp[2] += tmp[1];
						break;
					}
					tmp[0] += tmp[2];
				} else
					tmp[0] += *(str++);
			}
			(*acc) = (LPCSTR)tmp[0];
		}
		break;

	case 6:		// time
		(*acc) = (int)(time(NULL));
		break;
	case 7:	// strftime(f, t)
		{
			time_t t;
			struct tm *tm;
			char tmp[1024];
			if ( local.GetSize() < 2 || (t = (time_t)(int)local[1]) == 0 )
				time(&t);
			tm = localtime(&t);
			strftime(tmp, 1024, (LPCSTR)local[0], tm);
			(*acc) = (LPCSTR)tmp;
		}
		break;
	case 8:	// getdate(t)
		{
			time_t t;
			struct tm *tm;
			if ( local.GetSize() < 1 || (t = (time_t)(int)local[0]) == 0 )
				time(&t);
			tm = localtime(&t);
			acc->RemoveAll();
			(*acc) = (int)t;
			(*acc)["seconds"] = (int)tm->tm_sec;
			(*acc)["minutes"] = (int)tm->tm_min;
			(*acc)["hours"]   = (int)tm->tm_hour;
			(*acc)["mday"]    = (int)tm->tm_mday;
			(*acc)["wday"]    = (int)tm->tm_wday;
			(*acc)["mon"]     = (int)tm->tm_mon  + 1;
			(*acc)["year"]    = (int)tm->tm_year + 1900;
			(*acc)["yday"]    = (int)tm->tm_yday;
			(*acc)["sec"]     = (int)tm->tm_sec;
			(*acc)["min"]     = (int)tm->tm_min;
			(*acc)["hour"]    = (int)tm->tm_hour;
		}
		break;
	case 9:	// mktime(hour, minute, second, month, day, year, is_dst)
		{
			time_t t;
			struct tm *tm;
			time(&t);
			tm = localtime(&t);
			if ( local.GetSize() >= 6 )
				tm->tm_isdst = (int)local[6];
			if ( local.GetSize() >= 5 )
				tm->tm_year = (int)local[5] - 1900;
			if ( local.GetSize() >= 4 )
				tm->tm_mday = (int)local[4];
			if ( local.GetSize() >= 3 )
				tm->tm_mon = (int)local[3] - 1;
			if ( local.GetSize() >= 2 )
				tm->tm_sec = (int)local[2];
			if ( local.GetSize() >= 1 )
				tm->tm_min = (int)local[1];
			if ( local.GetSize() >= 0 )
				tm->tm_hour = (int)local[0];
			(*acc) = (int)mktime(tm);
		}
		break;
	}
	return FALSE;
}
int CScript::Func01(int cmd, class CScript *owner, CScriptValue &local)
{
	CScriptValue *sp;
	CScriptValue *acc = (CScriptValue *)local;

	switch(cmd) {
	case 0:		// echo
		{
//			TRACE("echo(");
			for ( int n = 0 ; n < (int)local.GetSize() ; n++ )
				TRACE((LPCSTR)local[n]);
//			TRACE(")\n");
		}
		break;
	case 1:		// include(f)
		{
			int n, rt;
			if ( (n = owner->LoadFile((LPCSTR)local[0])) >= 0 && (int)owner->m_IncFile[n] == 0 ) {
				owner->m_IncFile[n] = (int)(-1);
				rt = owner->ExecSub(owner->m_IncFile[n].m_FuncPos, FALSE, &local);
				owner->m_IncFile[n] = (int)0;
				if ( rt )
					return TRUE;
			}
		}
		break;
	case 2:		// exit()
		return TRUE;

	case 3:		// int
		(*acc) = (int)local[0];
		break;
	case 4:		// int64
		(*acc) = (LONGLONG)local[0];
		break;
	case 5:		// str(s)
		(*acc) = (LPCSTR)local[0];
		break;
	case 6:		// wstr(s)
		(*acc) = (LPCWSTR)local[0];
		break;
	case 7:		// length
		sp = &(local[0]);
		while ( sp->m_Type == VALTYPE_IDENT )
			sp = (CScriptValue *)(*sp);
		if ( sp->m_Type == VALTYPE_STRING )
			(*acc) = (int)sp->m_Buf.GetSize();
		else if ( sp->m_Type == VALTYPE_WSTRING )
			(*acc) = (int)(sp->m_Buf.GetSize() / sizeof(WCHAR));
		else
			(*acc) = (int)strlen((LPCSTR)(*sp));
		break;

	case 8:		// sort
	case 9:		// rsort
	case 10:	// ksort
	case 11:	// krsort
		sp = &(local[0]);
		while ( sp->m_Type == VALTYPE_IDENT )
			sp = (CScriptValue *)(*sp);
		sp->Sort(cmd - 8);
		(*acc) = sp;
		break;
	case 12:	// count
		sp = &(local[0]);
		while ( sp->m_Type == VALTYPE_IDENT )
			sp = (CScriptValue *)(*sp);
		(*acc) = sp->GetSize();
		break;
	case 13:	// key
		sp = &(local[0]);
		while ( sp->m_Type == VALTYPE_IDENT )
			sp = (CScriptValue *)(*sp);
		(*acc) = (LPCSTR)(sp->m_Index);
		break;
	case 14:	// reset(s)
		sp = &(local[0]);
		while ( sp->m_Type == VALTYPE_IDENT )
			sp = (CScriptValue *)(*sp);
		sp->RemoveAll();
		*sp = (int)0;
		(*acc) = sp;
		break;

	case 15:	// sleep(ms)
		Sleep((int)local[0]);
		break;

	case 16:	// debug(s)
#ifdef	_DEBUG
		local[0].Debug();
		TRACE("\n");
#endif
		break;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////

void CScript::FuncInit()
{
	static const CScriptFunc funcs[] = {
		{ "echo",		0,	Func01 },
		{ "include",	1,	Func01 },	{ "exit",		2,	Func01 },
		{ "int",		3,	Func01 },	{ "int64",		4,	Func01 },	
		{ "str",		5,	Func01 },	{ "wstr",		6,	Func01 },
		{ "length",		7,	Func01 },	{ "sort",		8,	Func01 },
		{ "rsort",		9,	Func01 },	{ "ksort",		10,	Func01 },
		{ "krsort",		11,	Func01 },	{ "count",		12,	Func01 },
		{ "key",		13,	Func01 },	{ "reset",		14, Func01 },
		{ "sleep",		15,	Func01 },	{ "debug",		16, Func01 },

		{ "substr",		0,	Func02 },	{ "strstr",		1,	Func02 },
		{ "ereg",		2,	Func02 },	{ "replace",	3,	Func02 },
		{ "split",		4,	Func02 },	{ "sprintf",	5,	Func02 },
		{ "time",		6,	Func02 },	{ "strftime",	7,	Func02 },
		{ "getdate",	8,	Func02 },	{ "mktime",		9,	Func02 },

		{ "fopen",		0,	Func03 },	{ "fclose",		1,	Func03 },
		{ "fread",		2,	Func03 },	{ "fwrite",		3,	Func03 },
		{ "fgets",		4,	Func03 },	{ "fputs",		5,	Func03 },
		{ "fgetc",		6,	Func03 },	{ "fputc",		7,	Func03 },
		{ "feof",		8,	Func03 },	{ "ferror",		9,	Func03 },
		{ "fflush",		10,	Func03 },	{ "ftell",		11,	Func03 },
		{ "fseek",		12,	Func03 },	{ "file",		13,	Func03 },
		{ "stat",		14,	Func03 },	{ "basename",	15,	Func03 },
		{ "dirname",	16,	Func03 },	{ "copy",		17,	Func03 },
		{ "rename",		18,	Func03 },	{ "delete",		19,	Func03 },
		{ "getcwd",		20,	Func03 },	{ "chdir",		21,	Func03 },
		{ "play",		22,	Func03 },

		{ "rand",		0,	Func04 },	{ "srand",		1,	Func04 },
		{ "floor",		2,	Func04 },	{ "asin",		3,	Func04 },
		{ "sqrt",		4,	Func04 },	{ "tan",		5,	Func04 },
		{ "acos",		6,	Func04 },	{ "abs",		7,	Func04 },
		{ "cos",		8,	Func04 },	{ "exp",		9,	Func04 },
		{ "fmod",		10,	Func04 },	{ "atan",		11,	Func04 },
		{ "sin",		12,	Func04 },	{ "pow",		13,	Func04 },
		{ "ceil",		14,	Func04 },	{ "sinh",		15,	Func04 },
		{ "cosh",		16,	Func04 },	{ "tanh",		17,	Func04 },
		{ "log",		18,	Func04 },	{ "log10",		19,	Func04 },
		{ "atan2",		20,	Func04 },

		{ "imadd",		0,	Func05 },	{ "imsub",		1,	Func05 },
		{ "immul",		2,	Func05 },	{ "imdiv",		3,	Func05 },
		{ "impow",		4,	Func05 },	{ "imsqrt",		5,	Func05 },
		{ "imsin",		6,	Func05 },	{ "imcos",		7,	Func05 },
		{ "imtab",		8,	Func05 },	{ "imlog",		9,	Func05 },
		{ "imexp",		10,	Func05 },	{ "imabs",		11,	Func05 },
		{ "imarg",		12,	Func05 },

		{ "iconv",		0,	Func06 },
		{ "md5",		1,	Func06 },	{ "sha1",		2,	Func06 },
		{ "base64e",	3,	Func06 },	{ "base64d",	4,	Func06 },
		{ "quotede",	5,	Func06 },	{ "quotedd",	6,	Func06 },
		{ "base16e",	7,	Func06 },	{ "base16d",	8,	Func06 },
		{ "crc16",		9,	Func06 },	{ "crc32",		10,	Func06 },

		{ "cgetc",		0,	Func07 },	{ "cgets",		1,	Func07 },
		{ "cputc",		2,	Func07 },	{ "cputs",		3,	Func07 },
		{ "sgetc",		4,	Func07 },	{ "sgets",		5,	Func07 },
		{ "sputc",		6,	Func07 },	{ "sputs",		7,	Func07 },
		{ "sread",		8,	Func07 },	{ "swrite",		9,	Func07 },
		{ "sync",		10,	Func07 },	{ "xonxoff",	11,	Func07 },
		{ "filecheck",	12,	Func07 },	{ "yesno",		13,	Func07 },
		{ "abort",		14,	Func07 },	{ "updownopen",	15,	Func07 },
		{ "updownclose",16,	Func07 },	{ "updownmsg",	17,	Func07 },
		{ "updownrange",18,	Func07 },	{ "updownpos",	19,	Func07 },

		{ "regopen",	0,	Func08 },	{ "regclose",	1,	Func08 },
		{ "regchar",	2,	Func08 },	{ "regstr",		3,	Func08 },

		{ NULL,			0,	NULL }
	};
	int n;

	for ( n = 0 ; funcs[n].name != NULL ; n++ ) {
		m_Data[(LPCSTR)funcs[n].name].m_FuncPos = (funcs[n].cmd | 0x8000);
		m_Data[(LPCSTR)funcs[n].name].m_FuncExt = funcs[n].proc;
	}

	m_Data["E"]  = 2.71828182845904523536;
	m_Data["PI"] = 3.14159265358979323846;

	m_Data["SH_DENYNO"] = (int)0;
	m_Data["SH_DENYRD"] = (int)1;
	m_Data["SH_DENYWR"] = (int)2;
	m_Data["SH_DENYRW"] = (int)3;

	m_Data["SEEK_SET"] = (int)0;
	m_Data["SEEK_CUR"] = (int)1;
	m_Data["SEEK_END"] = (int)2;
}

//////////////////////////////////////////////////////////////////////

void CScript::OnProc(int cmd)
{
	CScriptValue *sp;

	if ( m_pDoc != NULL ) {
		sp = &(m_Data["SERVER"]);
		sp->RemoveAll();
		(*sp)["NAME"] = (LPCSTR)m_pDoc->m_ServerEntry.m_EntryName;
		(*sp)["HOST"] = (LPCSTR)m_pDoc->m_ServerEntry.m_HostName;
		(*sp)["USER"] = (LPCSTR)m_pDoc->m_ServerEntry.m_UserName;
		(*sp)["PASS"] = (LPCSTR)m_pDoc->m_ServerEntry.m_PassName;
		(*sp)["TERM"] = (LPCSTR)m_pDoc->m_ServerEntry.m_TermName;
		(*sp)["PORT"] = (LPCSTR)m_pDoc->m_ServerEntry.m_PortName;
		(*sp)["CODE"] = (int)m_pDoc->m_ServerEntry.m_KanjiCode;
	}

	ExecSub(m_IncFile[cmd].m_FuncPos, FALSE, NULL);
}
