#include "StdAfx.h"
#include <afxdb.h>
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
#include "SyncSock.h"
#include "Script.h"
#include "IConv.h"
#include "openssl/evp.h"
#include "EditDlg.h"
#include "PassDlg.h"
#include "StatusDlg.h"
#include "ComSock.h"

static CScriptValue SystemValue;

//////////////////////////////////////////////////////////////////////
// CComplex

CComplex::CComplex()
{
	re = im = 0.0;
}
CComplex::~CComplex()
{
}
const CComplex & CComplex::operator = (LPCTSTR str)
{
	// [+-][0-9]+[.][0-9]+([eE][+-][0-9]+)[+-][0-9]+[.][0-9]+([eE][+-][0-9]+)[iIjJ]
	LPTSTR p = (LPTSTR)str;

	re = wcstod(p, &p);
	if ( wcschr(_T("+-"), *p) != NULL ) {
		im = wcstod(p, &p);
		if ( wcschr(_T("iIjJ"), *p) == NULL )
			im = 0.0;
	} else
		im = 0.0;

	return *this;
}
void CComplex::operator += (CComplex &com)
{
	re = re + com.re;
	im = im + com.im;
}
void CComplex::operator -= (CComplex &com)
{
	re = re - com.re;
	im = im - com.im;
}
void CComplex::operator *= (CComplex &com)
{
	CComplex src(re, im);
	re = src.re * com.re - src.im * com.im;
	im = src.re * com.im + src.im * com.re;
}
void CComplex::operator /= (CComplex &com)
{
	CComplex src(re, im);
	double d = pow(com.re, 2) + pow(com.im, 2);
	re = (src.re * com.re + src.im * com.im) / d;
	im = (src.im * com.re - src.re * com.im) / d;
}
int CComplex::Compare(CComplex &com)
{
	// 実数 > 虚数として比較
	if ( fabs(re - com.re) < DOUBLE_ZERO ) {
		if ( fabs(im - com.im) < DOUBLE_ZERO )
			return 0;
		else if ( im > com.im )
			return 1;
		else
			return (-1);
	} else if ( re > com.re )
		return 1;
	else
		return (-1);
}

//////////////////////////////////////////////////////////////////////
// CScriptValue

CScriptValue::CScriptValue()
{
	m_Index.Empty();
	m_Type = VALTYPE_EMPTY;
	m_Value.m_Int = 0;
	m_Complex.re = m_Complex.im = 0.0;
	m_Buf.Clear();
	m_ArrayPos = 0;
	m_FuncPos = (-1);
	m_SrcTextPos = 0;
	m_FuncExt = NULL;
	m_Array.RemoveAll();
	m_Left = m_Right = m_Child = m_Next = m_Root = NULL;
	m_DocCmds = (-1);
	m_bNoCase = FALSE;
	m_PtrType = PTRTYPE_NONE;
}
CScriptValue::~CScriptValue()
{
	RemoveAll();
}
void *CScriptValue::ThrowNullPtr(void *ptr)
{
	if ( m_Value.m_Ptr == NULL )
		throw _T("CScriptValue null ptr");

	return ptr;
}
int CScriptValue::GetType()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetType();
	return m_Type;
}
int CScriptValue::GetPtrType()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetPtrType();
	if ( m_Type != VALTYPE_PTR )
		return PTRTYPE_NONE;
	return m_PtrType;
}
CBuffer *CScriptValue::GetBuf()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetBuf();
	if ( m_Type != VALTYPE_STRING )
		*this = (LPCSTR)(*this);
	return &m_Buf;
}
CBuffer *CScriptValue::GetWBuf()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetWBuf();
	if ( m_Type != VALTYPE_WSTRING )
		*this = (LPCWSTR)(*this);
	return &m_Buf;
}
CBuffer *CScriptValue::GetDBuf()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetDBuf();
	if ( m_Type != VALTYPE_DSTRING )
		*this = (LPCDSTR)(*this);
	return &m_Buf;
}
void CScriptValue::GetName(CStringA &name)
{
	if ( m_Type == VALTYPE_IDENT )
		((CScriptValue *)(m_Value.m_Ptr))->GetName(name);
	else if ( m_Root != NULL ) {
		if ( m_Root->m_Root != NULL ) {
			m_Root->GetName(name);
			name += ".";
		}
		if ( m_Index.IsEmpty() ) {
			CStringA tmp;
			tmp.Format("%d", m_ArrayPos); 
			name += tmp;
		} else
			name += m_Index;
	}
}
int CScriptValue::GetDocCmds()
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetDocCmds();
	return m_DocCmds;
}
CScriptValue & CScriptValue::GetAt(LPCSTR str)
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetAt(str);
	return (*this)[str];
}
CScriptValue & CScriptValue::GetAt(int index)
{
	if ( m_Type == VALTYPE_IDENT )
		return ((CScriptValue *)(m_Value.m_Ptr))->GetAt(index);
	return (*this)[index];
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
	CScriptValue *sp = &data;

	while ( sp->m_Type == VALTYPE_IDENT )
		sp = (CScriptValue *)(sp->m_Value.m_Ptr);

	m_Index = sp->m_Index;
	m_Type  = sp->m_Type;
	m_Value = sp->m_Value;
	m_Buf   = sp->m_Buf;

	m_Complex  = sp->m_Complex;
	m_ArrayPos = sp->m_ArrayPos;
	m_FuncPos  = sp->m_FuncPos;
	m_FuncExt  = sp->m_FuncExt;
	m_PtrType  = sp->m_PtrType;

	m_bNoCase = sp->m_bNoCase;

	ArrayCopy(*sp);

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
	case LEX_IMG:
		*this = (CComplex &)lex;
		break;
	case LEX_INT64:
		*this = (LONGLONG)lex;
		break;
	case LEX_LABEL:
	case LEX_LABELA:
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
	if ( GetType() == VALTYPE_COMPLEX || data.GetType() == VALTYPE_COMPLEX ) {
		CComplex d = (CComplex &)*this;
		d *= (CComplex &)data;
		*this = d;
	} else if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE )
		*this = (double)*this * (double)data;
	else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 )
		*this = (LONGLONG)*this * (LONGLONG)data;
	else if ( GetType() == VALTYPE_PTR || data.GetType() == VALTYPE_PTR )
		throw _T("ScriptValue ptr mull");
	else
		*this = (int)*this * (int)data;
}
void CScriptValue::operator /= (CScriptValue &data)
{
	if ( GetType() == VALTYPE_COMPLEX || data.GetType() == VALTYPE_COMPLEX ) {
		if ( ((CComplex &)data).re != 0.0 || ((CComplex &)data).im != 0.0  ) {
			CComplex d = (CComplex &)*this;
			d /= (CComplex &)data;
			*this = d;
		} else
			throw _T("ScriptValue complex div zero");
	} else if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE ) {
		if ( (double)data != 0.0 )
			*this = (double)*this / (double)data;
		else
			throw _T("ScriptValue double div zero");
	} else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 ) {
		if ( (LONGLONG)data != 0L )
			*this = (LONGLONG)*this / (LONGLONG)data;
		else
			throw _T("ScriptValue int64 div zero");
	} else if ( GetType() == VALTYPE_PTR || data.GetType() == VALTYPE_PTR ) {
		throw _T("ScriptValue ptr div");
	} else {
		if ( (int)data != 0 )
			*this = (int)*this / (int)data;
		else
			throw _T("ScriptValue int div zero");
	}
}
void CScriptValue::operator %= (CScriptValue &data)
{
	if ( (int)data != 0 )
		*this = (int)*this % (int)data;
	else
		throw _T("ScriptValue int mod zero");
}
void CScriptValue::operator += (CScriptValue &data)
{
	if ( GetType() == VALTYPE_COMPLEX || data.GetType() == VALTYPE_COMPLEX ) {
		CComplex d = (CComplex &)*this;
		d += (CComplex &)data;
		*this = d;
	} else if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE )
		*this = (double)*this + (double)data;
	else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 )
		*this = (LONGLONG)*this + (LONGLONG)data;
	else if ( GetType() == VALTYPE_PTR || data.GetType() == VALTYPE_PTR )
		throw _T("ScriptValue ptr add");
	else
		*this = (int)*this + (int)data;
}
void CScriptValue::operator -= (CScriptValue &data)
{
	if ( GetType() == VALTYPE_COMPLEX || data.GetType() == VALTYPE_COMPLEX ) {
		CComplex d = (CComplex &)*this;
		d -= (CComplex &)data;
		*this = d;
	} else if ( GetType() == VALTYPE_DOUBLE || data.GetType() == VALTYPE_DOUBLE )
		*this = (double)*this - (double)data;
	else if ( GetType() == VALTYPE_INT64 || data.GetType() == VALTYPE_INT64 )
		*this = (LONGLONG)*this - (LONGLONG)data;
	else if ( GetType() == VALTYPE_PTR || data.GetType() == VALTYPE_PTR )
		throw _T("ScriptValue ptr sub");
	else
		*this = (int)*this - (int)data;
}
void CScriptValue::AddStr(CScriptValue &data)
{
	if ( GetType() == VALTYPE_DSTRING || data.GetType() == VALTYPE_DSTRING ) {
		CStringD tmp;
		tmp = (LPCDSTR)(*this);
		tmp += (LPCDSTR)(data);
		*this = (LPCDSTR)tmp;
	} else if ( GetType() == VALTYPE_WSTRING || data.GetType() == VALTYPE_WSTRING ) {
		CStringW tmp;
		tmp = (LPCWSTR)(*this);
		tmp += (LPCWSTR)(data);
		*this = (LPCWSTR)tmp;
	} else {
		CStringA tmp;
		tmp = (LPCSTR)(*this);
		tmp += (LPCSTR)(data);
		*this = (LPCSTR)tmp;
	}
}

//////////////////////////////////////////////////////////////////////

int CScriptValue::Add(CScriptValue &data)
{
	CScriptValue *tp;
	CScriptValue *sp;

	sp = new CScriptValue;
	*sp = data;

	if ( (sp->m_bNoCase = m_bNoCase) )
		sp->m_Index.MakeLower();

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
	sp->m_bNoCase = m_bNoCase;
	return sp->m_ArrayPos;
}
int CScriptValue::Find(LPCSTR str)
{
	int c;
	CScriptValue *tp;
	CStringA tmp;

	if ( m_bNoCase ) {
		tmp = str;
		tmp.MakeLower();
		str = tmp;
	}
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
		tp->m_bNoCase = m_bNoCase;
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
	if ( index < 0 )
		throw _T("ScriptValue negative index");

	if ( index >= (int)m_Array.GetSize() )
		m_Array.SetSize(index + 1);

	if ( m_Array[index] == NULL ) {
		CScriptValue *sp = new CScriptValue;
		sp->m_Root = this;
		sp->m_ArrayPos = index;
		sp->m_bNoCase = m_bNoCase;
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
const CComplex & CScriptValue::operator = (CComplex &com)
{
	m_Type = VALTYPE_COMPLEX;
	m_Complex = com;
	return m_Complex;
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
const LPCDSTR CScriptValue::operator = (LPCDSTR str)
{
	m_Type = VALTYPE_DSTRING;
	m_Buf.Clear();
	m_Buf.Apend((LPBYTE)str, (int)CStringD(str).GetLength() * sizeof(DCHAR));
	return (LPCDSTR)m_Buf;
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
const DCHAR * CScriptValue::operator = (DCHAR * value)
{
	m_Type = VALTYPE_LPDCHAR;
	m_Value.m_Ptr = value;
	return value;
}
const double * CScriptValue::operator = (double * value)
{
	m_Type = VALTYPE_LPDDOUB;
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
	m_PtrType = PTRTYPE_NONE;
	return ptr;
}

//////////////////////////////////////////////////////////////////////

CScriptValue::operator int ()
{
	if ( this == NULL )
		throw _T("CScriptValue Null int");
	switch(m_Type) {
	case VALTYPE_INT:
		return m_Value.m_Int;
	case VALTYPE_DOUBLE:
		return (int)(m_Value.m_Double);
	case VALTYPE_COMPLEX:
		return (int)(m_Complex.re);
	case VALTYPE_INT64:
		return (int)(m_Value.m_Int64);
	case VALTYPE_STRING:
		return atoi((LPCSTR)m_Buf);
	case VALTYPE_WSTRING:
		return _wtoi((LPCWSTR)m_Buf);
	case VALTYPE_DSTRING:
		return _wtoi(CStringD((LPCDSTR)m_Buf));
	case VALTYPE_LPBYTE:
		return (int)*((LPBYTE)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPWCHAR:
		return (int)*((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPDCHAR:
		return (int)*((DCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPDDOUB:
		return (int)*((DCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_IDENT:
		return (int)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_PTR:
		return (m_Value.m_Ptr == NULL ? 0 : 1);
	}
	return 0;
}
CScriptValue::operator double ()
{
	switch(m_Type) {
	case VALTYPE_INT:
		return (double)(m_Value.m_Int);
	case VALTYPE_DOUBLE:
		return m_Value.m_Double;
	case VALTYPE_COMPLEX:
		return (double)m_Complex.re;
	case VALTYPE_INT64:
		return (double)(m_Value.m_Int64);
	case VALTYPE_STRING:
		return atof((LPCSTR)m_Buf);
	case VALTYPE_WSTRING:
		return _wtof((LPCWSTR)m_Buf);
	case VALTYPE_DSTRING:
		return _wtof(CStringD((LPCDSTR)m_Buf));
	case VALTYPE_LPBYTE:
		return (double)*((LPBYTE)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPWCHAR:
		return (double)*((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPDCHAR:
		return (double)*((DCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPDDOUB:
		return *((double *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_IDENT:
		return (double)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return 0.0;
}
CScriptValue::operator CComplex & ()
{
	switch(m_Type) {
	case VALTYPE_INT:
		m_Complex = (double)(m_Value.m_Int);
		break;
	case VALTYPE_DOUBLE:
		m_Complex = m_Value.m_Double;
		break;
	case VALTYPE_INT64:
		m_Complex = (double)(m_Value.m_Int64);
		break;
	case VALTYPE_STRING:
		m_Complex = MbsToTstr((LPCSTR)m_Buf);
		break;
	case VALTYPE_WSTRING:
		m_Complex = UniToTstr((LPCWSTR)m_Buf);
		break;
	case VALTYPE_DSTRING:
		m_Complex = UniToTstr(CStringD((LPCDSTR)m_Buf));
		break;
	case VALTYPE_LPBYTE:
		m_Complex = (double)*((LPBYTE)(ThrowNullPtr(m_Value.m_Ptr)));
		break;
	case VALTYPE_LPWCHAR:
		m_Complex = (double)*((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
		break;
	case VALTYPE_LPDCHAR:
		m_Complex = (double)*((DCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
		break;
	case VALTYPE_LPDDOUB:
		m_Complex = *((double *)(ThrowNullPtr(m_Value.m_Ptr)));
		break;
	case VALTYPE_IDENT:
		return (CComplex &)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return m_Complex;
}
CScriptValue::operator LONGLONG ()
{
	switch(m_Type) {
	case VALTYPE_INT:
		return (LONGLONG)(m_Value.m_Int);
	case VALTYPE_DOUBLE:
		return (LONGLONG)(m_Value.m_Double);
	case VALTYPE_COMPLEX:
		return (LONGLONG)(m_Complex.re);
	case VALTYPE_INT64:
		return m_Value.m_Int64;
	case VALTYPE_STRING:
		return (LONGLONG)atof((LPCSTR)m_Buf);
	case VALTYPE_WSTRING:
		return (LONGLONG)_wtof((LPCWSTR)m_Buf);
	case VALTYPE_DSTRING:
		return (LONGLONG)_wtof(CStringD((LPCDSTR)m_Buf));
	case VALTYPE_LPBYTE:
		return (LONGLONG)*((LPBYTE)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPWCHAR:
		return (LONGLONG)*((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPDCHAR:
		return (LONGLONG)*((DCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_LPDDOUB:
		return (LONGLONG)*((double *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_IDENT:
		return (LONGLONG)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return (LONGLONG)0;
}
CScriptValue::operator LPCSTR ()
{
	CStringA tmp;
	CStringD work;

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
	case VALTYPE_COMPLEX:
		tmp.Format("%g+%gi", m_Complex.re, m_Complex.im);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_INT64:
		tmp.Format("%I64d", m_Value.m_Int64);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_STRING:
		return (LPCSTR)m_Buf;
	case VALTYPE_WSTRING:
		tmp = (LPCWSTR)m_Buf;
		m_Work.Clear();
		m_Work.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		return (LPCSTR)m_Work;
	case VALTYPE_DSTRING:
		tmp = CStringD((LPCDSTR)m_Buf);
		m_Work.Clear();
		m_Work.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		return (LPCSTR)m_Work;
	case VALTYPE_LPBYTE:
		tmp.Format("%c", *((LPBYTE)(ThrowNullPtr(m_Value.m_Ptr))));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_LPWCHAR:
		tmp.Format("%C", *((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr))));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_LPDCHAR:
		work = *((DCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
		tmp = work;
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_LPDDOUB:
		tmp.Format("%g", *((double *)(ThrowNullPtr(m_Value.m_Ptr))));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_IDENT:
		return (LPCSTR)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		m_Buf.Clear();
		break;
	}
	return (LPCSTR)m_Buf;
}
CScriptValue::operator LPCWSTR ()
{
	CStringW tmp;
	CStringD work;

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
	case VALTYPE_COMPLEX:
		tmp.Format(L"%g+%gi", m_Complex.re, m_Complex.im);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_INT64:
		tmp.Format(L"%I64d", m_Value.m_Int64);
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength());
		break;
	case VALTYPE_STRING:
		tmp = (LPCSTR)m_Buf;
		m_Work.Clear();
		m_Work.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		return (LPCWSTR)m_Work;
	case VALTYPE_WSTRING:
		return (LPCWSTR)m_Buf;
	case VALTYPE_DSTRING:
		tmp = CStringD((LPCDSTR)m_Buf);
		m_Work.Clear();
		m_Work.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		return (LPCWSTR)m_Work;
	case VALTYPE_LPBYTE:
		tmp.Format(L"%c", *((LPBYTE)(ThrowNullPtr(m_Value.m_Ptr))));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_LPWCHAR:
		tmp.Format(L"%C", *((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr))));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_LPDCHAR:
		work = *((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
		tmp = work;
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_LPDDOUB:
		tmp.Format(L"%g", ((double *)(ThrowNullPtr(m_Value.m_Ptr))));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCWSTR)tmp, tmp.GetLength() * sizeof(WCHAR));
		break;
	case VALTYPE_IDENT:
		return (LPCWSTR)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		m_Buf.Clear();
		break;
	}
	return (LPCWSTR)m_Buf;
}
CScriptValue::operator LPCDSTR ()
{
	CStringW tmp;
	CStringD work;

	switch(m_Type) {
	case VALTYPE_INT:
		tmp.Format(L"%d", m_Value.m_Int);
		break;
	case VALTYPE_DOUBLE:
		tmp.Format(L"%g", m_Value.m_Double);
		break;
	case VALTYPE_COMPLEX:
		tmp.Format(L"%g+%gi", m_Complex.re, m_Complex.im);
		break;
	case VALTYPE_INT64:
		tmp.Format(L"%I64d", m_Value.m_Int64);
		break;
	case VALTYPE_STRING:
		tmp = (LPCSTR)m_Buf;
		break;
	case VALTYPE_WSTRING:
		tmp = (LPCWSTR)m_Buf;
		break;
	case VALTYPE_DSTRING:
		return (LPCDSTR)m_Buf;
	case VALTYPE_LPBYTE:
		tmp.Format(L"%c", *((LPBYTE)(ThrowNullPtr(m_Value.m_Ptr))));
		break;
	case VALTYPE_LPWCHAR:
		tmp.Format(L"%C", *((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr))));
		break;
	case VALTYPE_LPDCHAR:
		work = *((WCHAR *)(ThrowNullPtr(m_Value.m_Ptr)));
		m_Buf.Clear();
		m_Buf.Apend((LPBYTE)(LPCDSTR)work, work.GetLength() * sizeof(DCHAR));
		return (LPCDSTR)m_Buf;
	case VALTYPE_LPDDOUB:
		tmp.Format(L"%g", ((double *)(ThrowNullPtr(m_Value.m_Ptr))));
		break;
	case VALTYPE_IDENT:
		return (LPCDSTR)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		break;
	}
	work = tmp;
	m_Work.Clear();
	m_Work.Apend((LPBYTE)(LPCDSTR)work, work.GetLength() * sizeof(DCHAR));
	return (LPCDSTR)m_Work;
}
CScriptValue::operator LPBYTE ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_COMPLEX:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_DSTRING:
	case VALTYPE_LPWCHAR:
	case VALTYPE_LPDCHAR:
	case VALTYPE_LPDDOUB:
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		return NULL;
	case VALTYPE_IDENT:
		return (LPBYTE)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return (LPBYTE)m_Value.m_Ptr;
}
CScriptValue::operator WCHAR * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_COMPLEX:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_LPDCHAR:
	case VALTYPE_LPDDOUB:
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		return NULL;
	case VALTYPE_IDENT:
		return (WCHAR *)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return (WCHAR *)m_Value.m_Ptr;
}
CScriptValue::operator DCHAR * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_COMPLEX:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
	case VALTYPE_LPDDOUB:
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		return NULL;
	case VALTYPE_IDENT:
		return (DCHAR *)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return (DCHAR *)m_Value.m_Ptr;
}
CScriptValue::operator double * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_COMPLEX:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
	case VALTYPE_LPDCHAR:
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		return NULL;
	case VALTYPE_IDENT:
		return (double *)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return (double *)m_Value.m_Ptr;
}
CScriptValue::operator CScriptValue * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_COMPLEX:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_DSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
	case VALTYPE_LPDCHAR:
	case VALTYPE_LPDDOUB:
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		return NULL;
	}
	return (CScriptValue *)m_Value.m_Ptr;
}
CScriptValue::operator void * ()
{
	switch(m_Type) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_COMPLEX:
	case VALTYPE_INT64:
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_DSTRING:
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
	case VALTYPE_LPDCHAR:
	case VALTYPE_LPDDOUB:
	case VALTYPE_EMPTY:
		return NULL;
	case VALTYPE_IDENT:
		return (void *)*((CScriptValue *)(ThrowNullPtr(m_Value.m_Ptr)));
	}
	return m_Value.m_Ptr;
}

//////////////////////////////////////////////////////////////////////

int CScriptValue::Compare(int mode, CScriptValue *ptr)
{
	int c;

	switch(mode & 2) {
	case 0:
		if ( GetType() == VALTYPE_DSTRING || ptr->GetType() == VALTYPE_DSTRING ) {
			CStringD tmp((LPCDSTR)*this);
			c =tmp.Compare((LPCDSTR)*ptr);
		} else if ( GetType() == VALTYPE_WSTRING || ptr->GetType() == VALTYPE_WSTRING )
			c = wcscmp((LPCWSTR)*this, (LPCWSTR)*ptr);
		else if ( GetType() == VALTYPE_STRING || ptr->GetType() == VALTYPE_STRING )
			c = strcmp((LPCSTR)*this, (LPCSTR)*ptr);
		else if ( GetType() == VALTYPE_DOUBLE || ptr->GetType() == VALTYPE_DOUBLE )
			c = ( fabs((double)*this - (double)*ptr) < DOUBLE_ZERO ? 0 : ((double)*this > (double)*ptr ? 1 : (-1)));
		else if ( GetType() == VALTYPE_COMPLEX || ptr->GetType() == VALTYPE_COMPLEX )
			c = ((CComplex &)*this).Compare((CComplex &)*ptr);
		else if ( GetType() == VALTYPE_INT64 || ptr->GetType() == VALTYPE_INT64 )
			c = ((LONGLONG)*this == (LONGLONG)*ptr ? 0 : ((LONGLONG)*this > (LONGLONG)*ptr ? 1 : (-1)));
		else if ( GetType() == VALTYPE_PTR || ptr->GetType() == VALTYPE_PTR )
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
void CScriptValue::SetPtrType(int type)
{
	if ( m_Type == VALTYPE_IDENT )
		((CScriptValue *)(m_Value.m_Ptr))->SetPtrType(type);
	else if ( m_Type == VALTYPE_PTR )
		m_PtrType = type;
}
void CScriptValue::SetArray(CStringArrayExt &ary, int mode)
{
	int n;

	switch(mode) {
	case DOC_MODE_SAVE:
		ary.RemoveAll();
		for ( n = 0 ; n < GetSize() ; n++ )
			ary.Add((LPCTSTR)(*this)[n]);
		break;
	case DOC_MODE_IDENT:
		RemoveAll();
		for ( n = 0 ; n < ary.GetSize() ; n++ )
			(*this)[n] = (LPCTSTR)ary[n];
		break;
	}
}
void CScriptValue::SetStr(CString &str, int mode)
{
	switch(mode) {
	case DOC_MODE_SAVE:
		str = (LPCTSTR)(*this);
		break;
	case DOC_MODE_IDENT:
		*this = (LPCTSTR)str;
		break;
	}
}
void CScriptValue::SetInt(int &val, int mode)
{
	switch(mode) {
	case DOC_MODE_SAVE:
		val = (int)(*this);
		break;
	case DOC_MODE_IDENT:
		*this = (int)val;
		break;
	}
}
void CScriptValue::SetNodeStr(LPCSTR node, CString &str, int mode)
{
	int n;

	switch(mode) {
	case DOC_MODE_SAVE:
		if ( (n = Find(node)) >= 0 )
			str = (LPCTSTR)(*this)[n];
		break;
	case DOC_MODE_IDENT:
		(*this)[node] = (LPCTSTR)str;
		break;
	}
}
void CScriptValue::SetNodeInt(LPCSTR node, int &val, int mode)
{
	int n;

	switch(mode) {
	case DOC_MODE_SAVE:
		if ( (n = Find(node)) >= 0 )
			val = (int)(*this)[n];
		break;
	case DOC_MODE_IDENT:
		(*this)[node] = (int)val;
		break;
	}
}
void CScriptValue::Serialize(int mode, CBuffer *bp)
{
	if ( m_Type == VALTYPE_IDENT ) {
		((CScriptValue *)(m_Value.m_Ptr))->Serialize(mode, bp);
		return;
	}

	int n, len;
	CBuffer tmp;

	if ( mode ) {	// Write
		bp->PutStr(m_Index);
		bp->Put8Bit(m_Type);
		bp->Put8Bit(m_bNoCase);

		switch(m_Type) {
		case VALTYPE_INT:
		case VALTYPE_DOUBLE:
		case VALTYPE_INT64:
			bp->PutBuf((LPBYTE)&m_Value, sizeof(m_Value));
			break;
		case VALTYPE_COMPLEX:
			bp->PutBuf((LPBYTE)&m_Complex, sizeof(m_Complex));
			break;
		case VALTYPE_STRING:
		case VALTYPE_WSTRING:
		case VALTYPE_DSTRING:
			bp->PutBuf(m_Buf.GetPtr(), m_Buf.GetSize());
			break;
		case VALTYPE_IDENT:
		case VALTYPE_LPBYTE:
		case VALTYPE_LPWCHAR:
		case VALTYPE_LPDCHAR:
		case VALTYPE_LPDDOUB:
		case VALTYPE_PTR:
		case VALTYPE_EMPTY:
			break;
		}

		bp->Put32Bit((int)m_Array.GetSize());

		for ( n = 0 ; n < m_Array.GetSize() ; n++ ) {
			if ( m_Array[n] != NULL )
				bp->Put32Bit(0);
			else {
				((CScriptValue *)m_Array[n])->Serialize(mode, &tmp);
				bp->PutBuf(tmp.GetPtr(), tmp.GetSize());
			}
		}

	} else {		// Read
		bp->GetStr(m_Index);
		m_Type = bp->Get8Bit();
		m_bNoCase = bp->Get8Bit();

		switch(m_Type) {
		case VALTYPE_INT:
		case VALTYPE_DOUBLE:
		case VALTYPE_INT64:
			tmp.Clear();
			bp->GetBuf(&tmp);
			if ( tmp.GetSize() == sizeof(m_Value) )
				memcpy((LPBYTE)&m_Value, tmp.GetPtr(), sizeof(m_Value));
			break;
		case VALTYPE_COMPLEX:
			tmp.Clear();
			bp->GetBuf(&tmp);
			if ( tmp.GetSize() == sizeof(m_Complex) )
				memcpy((LPBYTE)&m_Complex, tmp.GetPtr(), sizeof(m_Complex));
			break;
		case VALTYPE_STRING:
		case VALTYPE_WSTRING:
		case VALTYPE_DSTRING:
			m_Buf.Clear();
			bp->GetBuf(&m_Buf);
			break;
		case VALTYPE_IDENT:
		case VALTYPE_LPBYTE:
		case VALTYPE_LPWCHAR:
		case VALTYPE_LPDCHAR:
		case VALTYPE_LPDDOUB:
		case VALTYPE_PTR:
		case VALTYPE_EMPTY:
			break;
		}

		len = bp->Get32Bit();
		RemoveAll();

		for ( n = 0 ; n < len ; n++ ) {
			tmp.Clear();
			bp->GetBuf(&tmp);
			if ( tmp.GetSize() == 0 ) {
				m_Array.Add(NULL);
			} else {
				CScriptValue node;
				node.Serialize(mode, &tmp);
				Add(node);
			}
		}
	}
}
void CScriptValue::SetIndex(CStringIndex &index)
{
	// CScriptValue -> CScringIndex
	int n;

	if ( m_Type == VALTYPE_IDENT ) {
		((CScriptValue *)(m_Value.m_Ptr))->SetIndex(index);
		return;
	}

	index.SetNoSort(TRUE);
	index.m_nIndex = m_Index;

	switch(GetType()) {
	case VALTYPE_INT:
	case VALTYPE_DOUBLE:
	case VALTYPE_COMPLEX:
	case VALTYPE_INT64:
		index = (int)(*this);
		break;
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_DSTRING:
		index = (LPCTSTR)(*this);
		break;
	case VALTYPE_LPBYTE:
		index = (int)*((BYTE *)(*this));
		break;
	case VALTYPE_LPWCHAR:
		index = (int)*((WCHAR *)(*this));
		break;
	case VALTYPE_LPDCHAR:
		index = (int)*((DCHAR *)(*this));
		break;
	case VALTYPE_LPDDOUB:
		index = (int)*((double *)(*this));
		break;
	case VALTYPE_IDENT:
	case VALTYPE_PTR:
	case VALTYPE_EMPTY:
		index.m_bEmpty = TRUE;
		break;
	}

	index.RemoveAll();
	index.SetSize(GetSize());

	for ( n = 0 ; n < m_Array.GetSize() ; n++ ) {
		if ( m_Array[n] != NULL )
			GetAt(n).SetIndex(index[n]);
	}
}
void CScriptValue::GetIndex(CStringIndex &index)
{
	// CScriptValue <- CScringIndex
	int n, i;

	if ( m_Type == VALTYPE_IDENT ) {
		((CScriptValue *)(m_Value.m_Ptr))->GetIndex(index);
		return;
	}

	if ( index.IsEmpty() )
		Empty();
	else if ( index.m_bString )
		(*this) = (LPCTSTR)index;
	else
		(*this) = (int)index;

	RemoveAll();
	for ( n = 0 ; n < index.GetSize() ; n++ ) {
		if ( index[n].m_nIndex.IsEmpty() )
			i = Add(NULL);
		else
			i = Add(TstrToMbs(index[n].m_nIndex));
		GetAt(i).GetIndex(index[n]);
	}
}
void CScriptValue::Debug(CString &str)
{
	int n;
	CScriptValue *sp;
	CString tmp;
	static LPCTSTR typestr[] = {
		_T("int"),		_T("double"),	_T("complex"),	_T("int64"),	_T("str"),
		_T("wstr"),		_T("dstr"),		_T("pbyte"),	_T("pwchar"),	_T("pdchar"),
		_T("pdouble"),	_T("ident"),	_T("ptr"),		_T("empty") };

	str.Format(_T("%s:(%s)="), MbsToTstr(m_Index), typestr[m_Type]);
	switch(m_Type) {
	case VALTYPE_INT:
		tmp.Format(_T("%d"), m_Value.m_Int);
		break;
	case VALTYPE_DOUBLE:
		tmp.Format(_T("%f"), m_Value.m_Double);
		break;
	case VALTYPE_COMPLEX:
		tmp.Format(_T("%f+%fi"), m_Complex.re, m_Complex.im);
		break;
	case VALTYPE_INT64:
		tmp.Format(_T("%I64d"), m_Value.m_Int64);
		break;
	case VALTYPE_STRING:
	case VALTYPE_WSTRING:
	case VALTYPE_DSTRING:
		tmp.Format(_T("'%s'"), (LPCTSTR)*this);
		break;
	case VALTYPE_IDENT:
		((CScriptValue *)(m_Value.m_Ptr))->Debug(tmp);
		tmp.Insert(0, _T('{'));
		tmp += _T('}');
		break;
	case VALTYPE_LPBYTE:
	case VALTYPE_LPWCHAR:
	case VALTYPE_LPDCHAR:
	case VALTYPE_LPDDOUB:
	case VALTYPE_PTR:
		tmp.Format(_T("'%p'"), m_Value.m_Ptr);
		break;
	case VALTYPE_EMPTY:
		tmp = "empty";
		break;
	}
	str += tmp;

	if ( m_FuncPos != (-1) ) {
		tmp.Format(_T(":F(%d)"), m_FuncPos & 0x7FFFFFFFL);
		str += tmp;
	}

	for ( n = 0 ; n < m_Array.GetSize() ; n++ ) {
		if ( (sp = (CScriptValue *)m_Array[n]) != NULL ) {
			tmp.Format(_T("[%d:"), n); str += tmp;
			sp->Debug(tmp); str += tmp;
			str += _T("]");
		}
	}
}

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
		delete [] m_Buf.m_Ptr;
}
void CScriptLex::SetBuf(void *pBuf, int nBufLen)
{
	if ( m_Buf.m_Ptr != NULL ) {
		delete [] m_Buf.m_Ptr;
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
	m_Complex = data.m_Complex;
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
	case LEX_IMG:
		c = m_Complex.Compare(data.m_Complex);
		break;
	case LEX_INT64:
		c = (int)((LONGLONG)*this - (LONGLONG)data);
		break;
	case LEX_LABEL:
	case LEX_LABELA:
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

CScript::CScript()
{
	m_BufPos = m_BufMax = 0;
	m_BufPtr = NULL;
	m_UnGetChar = 0;

	m_Local = m_Class = NULL;
	m_Free = NULL;
	m_Loops = NULL;

	m_Stack = NULL;
	m_ExecLocal = NULL;
	m_CodeStack = NULL;
	m_CodePos = (-1);

	m_pDoc = NULL;
	m_bOpenSock = FALSE;
	m_EventFlag = 0;
	m_EventMask = 0;
	m_EventMode = 0;
	m_RegMatch = 0;
	m_bAbort = FALSE;
	m_bInit = FALSE;
	m_bExec = FALSE;

	m_Data.RemoveAll();
	m_Code.RemoveAll();
	m_IncFile.RemoveAll();

	m_ConsMode = DATA_BUF_NONE;
	m_SockMode = DATA_BUF_NONE;
	m_bConsOut = FALSE;

	m_SrcMax = 0;
	m_pSrcText = NULL;
	m_pSrcDlg = NULL;

	FuncInit();

	((CRLoginApp *)AfxGetApp())->AddIdleProc(IDLEPROC_SCRIPT, this);
}
CScript::~CScript(void)
{
	CScriptValue *sp;

	((CRLoginApp *)AfxGetApp())->DelIdleProc(IDLEPROC_SCRIPT, this);

	((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);

	while ( m_CodeStack != NULL )
		CodeStackPop();

	while ( (sp = m_Stack) != NULL ) {
		m_Stack = sp->m_Next;
		delete sp;
	}
	while ( (sp = m_Free) != NULL ) {
		m_Free = sp->m_Next;
		delete sp;
	}

	if ( m_pSrcText != NULL )
		delete [] m_pSrcText;

	if ( m_pSrcDlg != NULL ) {
		m_pSrcDlg->m_pScript = NULL;
		m_pSrcDlg->DestroyWindow();
	}
}

//////////////////////////////////////////////////////////////////////

int	CScript::InChar(int ch, CHAR *ptn)
{
    int n;
    for ( n = 0 ; ptn[n] != '\0' ; n++ ) {
		if ( ptn[n] == ch )
		    return n;
    }
    return (-1);
}
int	CScript::StrBin(int mx, const CHAR *ptn[], LPCSTR str)
{
    int n, c;
    int bs = 0;

	mx--;
    while ( bs <= mx ) {
		n = (bs + mx) / 2;
		if ( (c = _stricmp(ptn[n], str)) == 0 )
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

	if ( m_UnGetChar != 0 ) {
		ch = m_UnGetChar;
		m_UnGetChar = 0;
		return ch;
	}

	m_BufOld = m_BufPos;

	if ( m_BufPos >= m_BufMax )
		return EOF;

#ifdef	_UNICODE
	ch = m_BufPtr[m_BufPos++];
#else
	CStringA tmp;
	if ( issjis1(m_BufPtr[m_BufPos]) && issjis2(m_BufPtr[m_BufPos + 1]) ) {
		tmp  = (char)m_BufPtr[m_BufPos++];
		tmp += (char)m_BufPtr[m_BufPos++];
	} else
		tmp  = (char)m_BufPtr[m_BufPos++];
	ch = (WCHAR)(CStringW(tmp))[0];
#endif

	return ch;
}
void CScript::UnGetChar(int ch)
{
	m_UnGetChar = ch;
}

int CScript::LexDigit(int ch)
{
	LONGLONG val = 0;

	if ( ch == '0' ) {
		if ( (ch = GetChar()) == '.' || ch == 'e' || ch == 'E' || ch == 'i' || ch == 'j' )
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

	if ( ch == '.' || ch == 'e' || ch == 'E' || ch == 'i' || ch == 'j' )
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

	if ( ch == '.' ) {
		double d = 0.1;
		while ( (ch = GetChar()) != EOF ) {
			if ( ch >= '0' && ch <= '9' )
				a = a + (double)(ch - '0') * d;
			else
				break;
			d = d * 0.1;
		}
	}

	if ( ch == 'e' || ch == 'E' ) {
		int n = 0, f = 1;

		ch = GetChar();
		if ( ch == '-' )
			f = (-1);
		else if ( ch == '+' )
			f = 1;
		else
			THROW(NULL);

		while ( (ch = GetChar()) != EOF ) {
			if ( ch >= '0' && ch <= '9' )
				n = n * 10 + (ch - '0');
			else
				break;
		}
		a = a * pow(10.0, n * f);
	}

	if ( ch == 'i' || ch == 'j' ) {
		m_LexTmp.m_Complex.re = 0.0;
		m_LexTmp.m_Complex.im = a;
		return LEX_IMG;
	} else {
		UnGetChar(ch);
		m_LexTmp = (double)a;
		return LEX_DOUBLE;
	}
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
#define IDTR_MAX   	(5 * 3 + 2)
    static const CHAR *func[] = {
		"break",	"case", 	"class",	"continue",	"default",
		"do",		"else", 	"for",		"foreach",	"function",
		"if",		"in",		"return",	"switch",	"this",	
		"var",		"while",
    };
	static const int adr_tbl[]={
		LEX_ANDAND, LEX_OROR, LEX_INC, LEX_DEC
    };
    static const int let_tbl[]={
		LEX_ADDLET, LEX_SUBLET, LEX_MULLET, LEX_DIVLET, LEX_MODLET,
		LEX_ANDLET, LEX_ORLET,  LEX_XORLET, LEX_EQU,    LEX_UNEQU,	LEX_CATLET
    };
    static const int sfl_tbl[]={
		LEX_LEFTLET, LEX_RIGHTLET
    };
    static const int sft_tbl[]={
		LEX_LEFT, LEX_RIGHT
    };
    static const int bse_tbl[]={
		LEX_SML, LEX_BIG
    };

	int n;
	int ch, nx;
	CStringA tmp;
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
	m_PosBtm = m_BufBtm;

	if ( ch == EOF ) {
		goto ENDOF;
	
	} else if ( ch >= '0' && ch <= '9' ) {
		ch = LexDigit(ch);

	} else if ( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' ) {
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
				if ( ch == '.' ) {
					GetChar();	// skip '.'
					ch = LEX_LABELA;
				} else
					ch = LEX_LABEL;
			}
		}

	} else if ( (n = InChar(ch, "+-*/%&|^=!.")) >= 0 ) {
		if ( (nx = GetChar()) == '=' )
		    ch = let_tbl[n];
		else if ( (n = InChar(ch, "&|+-")) >= 0 && ch == nx )
		    ch = adr_tbl[n];
		else
		    UnGetChar(nx);

    } else if ( (n = InChar(ch, "<>")) >= 0 ) {
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
	m_BufBtm = m_BufPos;
	m_LexTmp.m_Type = ch;
	return &m_LexTmp;

ENDOF:
	m_BufBtm = m_BufPos;
	m_LexTmp.m_Type = LEX_EOF;
	return &m_LexTmp;
}
int CScript::LexAdd(CScriptLex *lex)
{
	int c, n;

	lex->m_Left = lex->m_Right = (-1);

	if ( m_LexData.GetSize() <= 0 )
		return (int)m_LexData.Add(*lex);
	else {
		for ( n = 0 ; ; ) {
			if ( (c = m_LexData[n].Compare(*lex)) == 0 )
				return n;
			else if ( c < 0 ) {
				if ( m_LexData[n].m_Left == (-1) ) {
					m_LexData[n].m_Left = (int)m_LexData.Add(*lex);
					return m_LexData[n].m_Left;
				} else
					n = m_LexData[n].m_Left;
			} else {
				if ( m_LexData[n].m_Right == (-1) ) {
					m_LexData[n].m_Right = (int)m_LexData.Add(*lex);
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
void CScript::LoadIdent(CScriptValue *sp, int index, int cmd)
{
	if ( sp == NULL )
		return;
	LoadIdent(sp->m_Next, sp->m_ArrayPos, cmd);
	if ( sp->m_Next == NULL ) {
		CodeAdd(cmd);
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
	case LEX_LABELA:
		if ( m_Local != NULL && (n = m_Local->Find((LPCSTR)*lex)) != (-1) ) {
			CodeAdd(CM_LOCAL);
			CodeAddPos(n);
		} else if ( nf ) {
			LoadIdent(m_Class, m_Class->Add((LPCSTR)*lex), CM_LOAD);
		} else {
			CScriptValue *sp = NULL;
			for ( sp = m_Class ; sp != NULL ; sp = sp->m_Next ) {
				if ( (n = sp->Find((LPCSTR)*lex)) != (-1) ) {
					LoadIdent(sp, n, CM_LOAD);
					break;
				}
			}
			if ( sp == NULL ) {
				if ( strcmp((LPCSTR)*lex, "System") == 0 )
					LoadIdent(&SystemValue, SystemValue.Add((LPCSTR)*lex), CM_SYSTEM);
				else
					LoadIdent(&m_Data, m_Data.Add((LPCSTR)*lex), CM_LOAD);
			}
		}
		n = CodeAdd(CM_IDENT);
		while ( lex->m_Type == LEX_LABELA ) {
			lex = Lex();
			if ( lex->m_Type != LEX_LABEL && lex->m_Type != LEX_LABELA )
				THROW(lex);
			CodeSet(n, CM_PUSH);
			n = LexAdd(lex);
			CodeAdd(CM_VALUE);
			CodeAddPos(n);
			CodeAdd(CM_ARRAY);
			n = CodeAdd(CM_IDENT);
		}
		lex = Lex();
		break;
	case LEX_INT:
	case LEX_DOUBLE:
	case LEX_IMG:
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
			lex = Lex();
			if ( lex->m_Type == ']' )
				break;

			lex = Stage04(lex);
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
	|	'#' expr
	|	'$' expr
****/

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
	} else if ( lex->m_Type == '#' ) {
		lex = Stage14(Lex());
		CodeAdd(CM_INT);
	} else if ( lex->m_Type == '$' ) {
		lex = Stage14(Lex());
		CodeAdd(CM_STR);
	} else
		lex = Stage14(lex);

	return lex;
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
	|	expr '.' expr
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
		case '.':
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
CScriptLex *CScript::Stage061(CScriptLex *lex)
{
/****
	|	expr '&&' expr
****/
	int n;
	lex = Stage07(lex);
	while ( lex->m_Type == LEX_ANDAND ) {
		CodeAdd(CM_IFN);
		n = CodeAddPos(0);
		lex = Stage07(Lex());
		CodeSetPos(n);
	}
	return lex;
}
CScriptLex *CScript::Stage06(CScriptLex *lex)
{
/****
	|	expr '||' expr
****/
	int n;
	lex = Stage061(lex);
	while ( lex->m_Type == LEX_OROR ) {
		CodeAdd(CM_IF);
		n = CodeAddPos(0);
		lex = Stage061(Lex());
		CodeSetPos(n);
	}
	return lex;
}
CScriptLex *CScript::Stage05(CScriptLex *lex)
{
/****
	|	expr '?' expr ':' expr
****/
	int a, b;
	lex = Stage06(lex);
	if ( lex->m_Type == '?' ) {
		CodeAdd(CM_IFN);
		a = CodeAddPos(0);
		lex = Stage05(Lex());
		CodeAdd(CM_JMP);
		b = CodeAddPos(0);
		if ( lex->m_Type != ':' )
			THROW(lex);
		CodeSetPos(a);
		lex = Stage05(Lex());
		CodeSetPos(b);
	}
	return lex;
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
	switch(lex->m_Type) {
	case '=':
		n = IdentChack(lex);
		CodeSet(n, CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_SAVE);
		break;
	case LEX_MULLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_MUL);
		CodeAdd(CM_SAVE);
		break;
	case LEX_DIVLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_DIV);
		CodeAdd(CM_SAVE);
		break;
	case LEX_MODLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_MOD);
		CodeAdd(CM_SAVE);
		break;
	case LEX_ADDLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_ADD);
		CodeAdd(CM_SAVE);
		break;
	case LEX_SUBLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_SUB);
		CodeAdd(CM_SAVE);
		break;
	case LEX_LEFTLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_SFL);
		CodeAdd(CM_SAVE);
		break;
	case LEX_RIGHTLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_SFR);
		CodeAdd(CM_SAVE);
		break;
	case LEX_ANDLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_AND);
		CodeAdd(CM_SAVE);
		break;
	case LEX_XORLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_XOR);
		CodeAdd(CM_SAVE);
		break;
	case LEX_ORLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_OR);
		CodeAdd(CM_SAVE);
		break;
	case LEX_CATLET:
		n = IdentChack(lex);
		CodeSet(n, CM_IPUSH);
		CodeAdd(CM_IDENT);
		CodeAdd(CM_PUSH);
		lex = Stage04(Lex());
		CodeAdd(CM_CAT);
		CodeAdd(CM_SAVE);
		break;
	}
	return lex;
}
CScriptLex *CScript::Stage03(CScriptLex *lex)
{
/****
	|	expr ',' expr
****/
	for ( ; ; ) {
		lex = Stage04(lex);
		if ( lex->m_Type != ',' )
			break;
		lex = Lex();
	}

	return lex;
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
	|	FOREACH '(' expr ':' expr IN expr ')' stmt
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
	int a, b;
	switch(lex->m_Type) {
	case LEX_IF:
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		SetSrcMark(TRUE);
		if ( lex->m_Type != ')' )
			THROW(lex);
		CodeAdd(CM_IFN);
		a = CodeAddPos(0);
		lex = Stage01(Lex());
		SetSrcMark(TRUE);
		if ( lex->m_Type == LEX_ELSE ) {
			CodeAdd(CM_JMP);
			b = CodeAddPos(0);
			CodeSetPos(a);
			lex = Stage01(Lex());
			CodeSetPos(b);
			SetSrcMark(TRUE);
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
		SetSrcMark(TRUE);
		CodeAdd(CM_IFN);
		LoopAdd(CodeAddPos(0));
		lex = Stage01(Lex());
		SetSrcMark(TRUE);
		CodeAdd(CM_JMP);
		CodeAddPos(m_Loops->m_StartPos);
		LoopPop();
		break;

	case LEX_DO:
		LoopPush();
		lex = Stage01(Lex());
		SetSrcMark(TRUE);
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
		SetSrcMark(TRUE);
		CodeAdd(CM_IF);
		CodeAddPos(m_Loops->m_StartPos);
		LoopPop();
		break;

	case LEX_FOR:
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		SetSrcMark(TRUE);
		if ( lex->m_Type != ';' )
			THROW(lex);

		LoopPush();
		lex = Stage03(Lex());
		SetSrcMark(TRUE);
		if ( lex->m_Type != ';' )
			THROW(lex);

		if ( m_Loops->m_StartPos != CodePos() ) {
			CodeAdd(CM_IFN);
			LoopAdd(CodeAddPos(0));
			SetSrcMark(TRUE);
		}

		CodeAdd(CM_JMP);
		a = CodeAddPos(0);

		SetSrcMark(TRUE);
		b = CodePos();
		lex = Stage03(Lex());
		SetSrcMark(TRUE);
		if ( lex->m_Type != ')' )
			THROW(lex);

		CodeAdd(CM_JMP);
		CodeAddPos(m_Loops->m_StartPos);

		CodeSetPos(a);
		lex = Stage01(Lex());

		SetSrcMark(TRUE);
		CodeAdd(CM_JMP);
		CodeAddPos(b);
		LoopPop();
		break;

	case LEX_FOREACH:
		b = 0;
		lex->m_Type = LEX_INT;
		lex->m_Data.m_Int = 0;
		CodeAdd(CM_VALUE);
		CodeAddPos(LexAdd(lex));
		CodeAdd(CM_PUSH);

		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		a = IdentChack(lex);
		CodeSet(a, CM_PUSH);

		if ( lex->m_Type == ':' ) {
			lex = Stage03(Lex());
			a = IdentChack(lex);
			CodeSet(a, CM_PUSH);
			b = 1;
		}

		if ( lex->m_Type != LEX_IN )
			THROW(lex);
		lex = Stage03(Lex());
		CodeAdd(CM_PUSH);

		if ( lex->m_Type != ')' )
			THROW(lex);

		SetSrcMark(TRUE);
		LoopPush();
		CodeAdd(b == 0 ? CM_FOREACH : CM_EACHWITHKEY);
		LoopAdd(CodeAddPos(0));
		
		lex = Stage01(Lex());
		SetSrcMark(TRUE);
		CodeAdd(CM_JMP);
		CodeAddPos(m_Loops->m_StartPos);
		LoopPop();
		break;

	case LEX_RETURN:
		lex = Stage03(Lex());
		if ( lex->m_Type != ';' )
			THROW(lex);
		SetSrcMark(TRUE);
		CodeAdd(CM_RET);
		lex = Lex();
		break;

	case LEX_BREAK:
		lex = Lex();
		if ( lex->m_Type != ';' )
			THROW(lex);
		if ( m_Loops == NULL )
			THROW(lex);
		SetSrcMark(TRUE);
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
		SetSrcMark(TRUE);
		CodeAdd(CM_JMP);
		CodeAddPos(m_Loops->m_StartPos);
		lex = Lex();
		break;

	case LEX_SWITCH:
		// loop		PUSH
		//			JMP		case1
		//			...
		// break	POP
		lex = Lex();
		if ( lex->m_Type != '(' )
			THROW(lex);
		lex = Stage03(Lex());
		SetSrcMark(TRUE);
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
		//			JMP		next1
		// case1	value...
		//			CASE	case2
		// next1	...
		if ( m_Loops == NULL )
			THROW(lex);
		SetSrcMark(TRUE);
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
		SetSrcMark(TRUE);
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
			if ( lex->m_Type != LEX_LABEL && lex->m_Type != LEX_LABELA )
				THROW(lex);
			b = m_Class->Add((LPCSTR)*lex);
			sp = &((*m_Class)[b]);

			while ( lex->m_Type == LEX_LABELA ) {
				lex = Lex();
				if ( lex->m_Type != LEX_LABEL && lex->m_Type != LEX_LABELA )
					THROW(lex);
				b = sp->Add((LPCSTR)*lex);
				sp = &((*sp)[b]);
			}
			lex = Lex();

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

			SetSrcMark(TRUE);
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
		break;
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
		SetSrcMark(TRUE);
		lex = Lex();
	} else {
		m_SavPos = m_PosOld;
		SetSrcMark(FALSE);
		lex = Stage02(lex);
		SetSrcMark(TRUE);
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
	CodeAdd(CM_RET);
	return lex;
}

int CScript::ParseBuff(LPCTSTR buf, int len)
{
	INT_PTR pos = m_Code.GetSize();
	INT_PTR src = m_SrcCode.GetSize();
	INT_PTR lex = m_LexData.GetSize();

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
	} catch (...) {
		m_Code.SetSize(pos);
		m_SrcCode.SetSize(src);
		//m_LexData.SetSize(lex);

		CScriptDlg *pDlg = new CScriptDlg();

		pDlg->m_FileName.Format(_T("Syntax Error in '%s'"), (LPCTSTR)m_FileName);
		pDlg->m_pSrc = m_BufPtr;
		pDlg->m_Max  = m_BufMax;
		pDlg->m_SPos = m_SavPos;
		pDlg->m_EPos = m_BufPos;

		pDlg->Create(IDD_SCRIPTDLG, CWnd::GetDesktopWindow());
		pDlg->ShowWindow(SW_SHOW);

		return (-1);
	}
}
void CScript::SetSrcMark(BOOL bBtm)
{
	if ( m_pSrcText == NULL )
		return;

	if ( bBtm ) {
		if ( (m_SrcWork.epos = m_SrcMax + m_PosBtm) <= m_SrcWork.spos ) {
			m_SrcWork.epos = m_SrcWork.spos;
			m_SrcWork.spos = m_SrcMax + m_PosBtm;
		}
		if ( m_SrcWork.code != CodePos() ) {
			m_SrcCode.Add(m_SrcWork);
		}
	}

	m_SrcWork.code = CodePos();
	m_SrcWork.spos = m_SrcMax + m_PosOld;
}
static int ScrDebCmp(const void *src, const void *dis)
{
	ScriptDebug *pSrc = (ScriptDebug *)src;
	ScriptDebug *pDis = (ScriptDebug *)dis;

	if ( pSrc->code > pDis->code )
		return 1;
	else if ( pSrc->code < pDis->code )
		return (-1);
	else
		return 0;
}
int CScript::LoadFile(LPCTSTR filename)
{
	int n;
	//FILE *fp;
	//char tmp[BUFSIZ];
	//CString str;
	CBuffer buf;
	int size;
	LPTSTR ptr;
	int pos;

	if ( (n = m_IncFile.Find(TstrToMbs(filename))) >= 0 )
		return n;
	
	//if ( (fp = _tfopen(filename, _T("rb"))) == NULL )
	//	return (-1);

	//while ( fgets(tmp, BUFSIZ, fp) != NULL ) {
	//	str = tmp;
	//	buf.Apend((LPBYTE)(LPCTSTR)str, str.GetLength() * sizeof(TCHAR));
	//}

	//fclose(fp);

	if ( !buf.LoadFile(filename) ) {
		ThreadMessageBox(_T("Script File load error '%s'"), filename);
		return (-1);
	}

	buf.KanjiConvert(buf.KanjiCheck());

	size = buf.GetSize() / sizeof(TCHAR);

	if ( m_pSrcText == NULL ) {
		m_pSrcText = new TCHAR[size + 1];
		ptr = m_pSrcText;
	} else {
		ptr = new TCHAR[m_SrcMax + size + 1];
		memcpy(ptr, m_pSrcText, m_SrcMax * sizeof(TCHAR));
		delete [] m_pSrcText;
		m_pSrcText = ptr;
		ptr = m_pSrcText + m_SrcMax;
	}

	memcpy(ptr, buf.GetPtr(), buf.GetSize());
	ptr[size] = _T('\0');

	m_FileName = filename;
	if ( (pos = ParseBuff(buf, size)) < 0 )
		return (-1);

	n = m_IncFile.Add(TstrToMbs(filename));
	m_IncFile[n] = (int)0;
	m_IncFile[n].m_FuncPos = pos;
	m_IncFile[n].m_SrcTextPos = m_SrcMax;

	qsort(m_SrcCode.GetData(), m_SrcCode.GetSize(), sizeof(ScriptDebug), ScrDebCmp);
	m_SrcMax += size;

	return n;
}
int CScript::LoadStr(LPCTSTR str)
{
	int n;
	int pos;
	int size;
	LPTSTR ptr;

	if ( str == NULL || *str == _T('\0') )
		return (-1);

	size = (int)_tcslen(str);

	if ( m_pSrcText == NULL ) {
		m_pSrcText = new TCHAR[size + 1];
		ptr = m_pSrcText;
	} else {
		ptr = new TCHAR[m_SrcMax + size + 1];
		memcpy(ptr, m_pSrcText, m_SrcMax * sizeof(TCHAR));
		delete [] m_pSrcText;
		m_pSrcText = ptr;
		ptr = m_pSrcText + m_SrcMax;
	}

	memcpy(ptr, str, size * sizeof(TCHAR));
	ptr[size] = _T('\0');

	m_FileName = _T("NULL");
	if ( (pos = ParseBuff(ptr, size)) < 0 )
		return (-1);

	n = m_IncFile.Add((LPCSTR)NULL);
	m_IncFile[n] = (int)0;
	m_IncFile[n].m_FuncPos = pos;
	m_IncFile[n].m_SrcTextPos = m_SrcMax;

	qsort(m_SrcCode.GetData(), m_SrcCode.GetSize(), sizeof(ScriptDebug), ScrDebCmp);
	m_SrcMax += size;

	return n;
}

//////////////////////////////////////////////////////////////////////

void CScript::StackPush()
{
	CScriptValue *sp;
	
	//if ( m_Free == NULL )
	//	m_Free = new CScriptValue;

	//sp = m_Free;
	//m_Free = sp->m_Next;

	sp = new CScriptValue;

	sp->m_Next = m_Stack;
	m_Stack = sp;
}
void CScript::StackPop()
{
	CScriptValue *sp;

	ASSERT(m_Stack != NULL);

	if ( m_Stack == NULL )
		return;

	sp = m_Stack;
	m_Stack = sp->m_Next;

	delete sp;

	//sp->m_Next = m_Free;
	//m_Free = sp;

	//*sp = (int)0;
	//sp->m_Buf.Clear();
	//sp->RemoveAll();
}
void CScript::CodeStackPush(BOOL use, LPCTSTR name)
{
	CodeStack *cp;

	cp          = new CodeStack;
	cp->next    = m_CodeStack;
	m_CodeStack = cp;

	cp->local   = m_ExecLocal;
	cp->pos     = m_CodePos;
	cp->mask    = m_EventMask;
	m_EventMask = 0;

	if ( name != NULL )
		cp->name = name;

	if ( (cp->use = use) != FALSE )
		StackPush();
}
void CScript::CodeStackPop()
{
	CodeStack *cp;

	if ( (cp = m_CodeStack) == NULL )
		return;
	m_CodeStack = cp->next;

	if ( m_ExecLocal != NULL )
		delete m_ExecLocal;

	m_ExecLocal = cp->local;
	m_CodePos   = cp->pos;
	m_EventMask = cp->mask;

	if ( cp->use != FALSE )
		StackPop();

	delete cp;
}
static int SrcCodeCmp(const void *src, const void *dis)
{
	if ( (DWORD_PTR)src > (DWORD_PTR)((ScriptDebug *)dis)->code )
		return 1;
	else if ( (DWORD_PTR)src < (DWORD_PTR)((ScriptDebug *)dis)->code )
		return (-1);
	else
		return 0;
}
void CScript::Abort()
{
	while ( m_CodeStack != NULL )
		CodeStackPop();

	while ( m_Stack != NULL )
		StackPop();

	m_CodePos = (-1);
}
void CScript::Stop()
{
	int n;

	for ( n = 0 ; m_CodePos >= 0 && m_CodePos < (int)m_Code.GetSize() && n < 1000 ; n++ ) {
		OnClose();
		Exec();
	}

	Abort();
}

// return TRUE		use continue
//		  FALSE		end of code
int CScript::Exec()
{
	int n, len, count = 0;
	CScriptValue *sp, *dp;
	int dsrc = (-1);

	if ( m_EventMask != 0 ) {
		if ( m_EventFlag == m_EventMask )
			return FALSE;
		m_EventMode = m_EventFlag ^ m_EventMask;
		m_EventMask &= ~m_EventMode;
	}

	while ( m_CodePos >= 0 && m_CodePos < (int)m_Code.GetSize() ) {

		if ( m_pSrcDlg != NULL ) {
			int sc = m_SrcMax, ec = m_SrcMax;
			BinaryFind((void *)(INT_PTR)m_CodePos, m_SrcCode.GetData(), sizeof(ScriptDebug), (int)m_SrcCode.GetSize(), SrcCodeCmp, &n);
			if ( n < m_SrcCode.GetSize() ) {
				while ( n > 0 && m_CodePos < (int)m_SrcCode[n].code )
					n--;
//				TRACE("%d %d:%d:%d >%d\n", m_CodePos, m_SrcCode[n].code, m_SrcCode[n].spos, m_SrcCode[n].epos, m_SrcCode[n+1].code);
				sc = m_SrcCode[n].spos;
				ec = m_SrcCode[n].epos;
			}
			if ( sc == ec )
				ec++;
			if ( dsrc != (-1) && dsrc != sc ) {
				m_pSrcDlg->UpdatePos(sc, ec);
				return TRUE;
			}
			dsrc = sc;
		}

		if ( ++count >= LOOP_COUNT )
			return TRUE;

		if ( m_Stack == NULL )
			throw _T("Stack Null");

		sp = m_Stack->m_Next;

		switch(m_Code[m_CodePos++]) {
		case CM_LOAD:
			n = m_Code[m_CodePos++];
			*m_Stack = &(m_Data[n]);
			break;
		case CM_CLASS:
			n = m_Code[m_CodePos++];
			*m_Stack = &((*((CScriptValue *)(m_Stack->m_Value.m_Ptr)))[n]);
			break;
		case CM_LOCAL:
			ASSERT(m_ExecLocal != NULL);
			n = m_Code[m_CodePos++];
			*m_Stack = &((*m_ExecLocal)[n]);
			break;
		case CM_SYSTEM:
			n = m_Code[m_CodePos++];
			*m_Stack = &(SystemValue[n]);
			break;

		case CM_VALUE:
			n = m_Code[m_CodePos++];
			*m_Stack = m_LexData[n];
			break;
		case CM_SAVE:
			if ( sp == NULL )
				throw _T("Stack Over");
			if ( sp->m_Type == VALTYPE_LPBYTE )
				*((LPBYTE)(*sp)) = (BYTE)((int)(*m_Stack));
			else if ( sp->m_Type == VALTYPE_LPWCHAR )
				*((WCHAR *)(*sp)) = (WCHAR)((int)(*m_Stack));
			else if ( sp->m_Type == VALTYPE_LPDCHAR )
				*((DCHAR *)(*sp)) = (DCHAR)((int)(*m_Stack));
			else if ( sp->m_Type == VALTYPE_LPDDOUB )
				*((double *)(*sp)) = (double)(*m_Stack);
			else if ( sp->m_Type == VALTYPE_IDENT ) {
				while ( sp->m_Type == VALTYPE_IDENT )
					sp = (CScriptValue *)*sp;
				for ( dp = m_Stack ; dp->m_Type == VALTYPE_IDENT ; )
					dp = (CScriptValue *)(*dp);
				if ( m_pDoc != NULL && (n = sp->GetDocCmds()) != (-1) ) {
					// switch(n & 0x03)
					m_pDoc->ScriptValue(n >> 2, *dp, DOC_MODE_SAVE);
					*(m_Stack->m_Next) = *dp;
				} else {
					sp->m_Type    = dp->m_Type;
					sp->m_Value   = dp->m_Value;
					sp->m_Buf     = dp->m_Buf;
					sp->m_Complex = dp->m_Complex;
					sp->m_FuncPos = dp->m_FuncPos;
					sp->m_FuncExt = dp->m_FuncExt;
					sp->m_PtrType = dp->m_PtrType;
					sp->ArrayCopy(*dp);
				}
			}
			StackPop();
			break;
		case CM_IDENT:
			if ( m_Stack->m_Type == VALTYPE_LPBYTE )
				*m_Stack = (int)(*((LPBYTE)*m_Stack));
			else if ( m_Stack->m_Type == VALTYPE_LPWCHAR )
				*m_Stack = (int)(*((WCHAR *)*m_Stack));
			else if ( m_Stack->m_Type == VALTYPE_LPDCHAR )
				*m_Stack = (int)(*((DCHAR *)*m_Stack));
			else if ( m_Stack->m_Type == VALTYPE_LPDDOUB )
				*m_Stack = *((double *)*m_Stack);
			else if ( m_Stack->m_Type == VALTYPE_IDENT && m_pDoc != NULL && (n = m_Stack->GetDocCmds()) != (-1) ) {
				// switch(n & 0x03)
				m_pDoc->ScriptValue(n >> 2, *m_Stack, DOC_MODE_IDENT);
			}
			//else if ( m_Stack->m_Type == VALTYPE_IDENT )
			//	*m_Stack = *((CScriptValue *)*m_Stack);
			break;

		case CM_IPUSH:
			StackPush();
			*m_Stack = *(m_Stack->m_Next);
			break;
		case CM_PUSH:
			StackPush();
			break;
		case CM_POP:
			StackPop();
			break;

		case CM_ADDARRAY:
			if ( sp == NULL )
				throw _T("Stack AddArray Over");
			if ( sp->m_Type == VALTYPE_IDENT )
				*(m_Stack->m_Next) = &((*((CScriptValue *)(*sp)))[(LPCSTR)NULL]);
			else
				*(m_Stack->m_Next) = (int)0;
			StackPop();
			break;

		case CM_ARRAY:
			if ( sp == NULL )
				throw _T("Stack Array Over");
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
				n = m_Code[m_CodePos++];
				*m_Stack = &((*sp)[n]);
			}
			break;

		case CM_INT:
			*m_Stack = (int)(*m_Stack);
			break;

		case CM_STR:
			if ( m_Stack->m_Type == VALTYPE_IDENT )
				*m_Stack = *((CScriptValue *)*m_Stack);
			(LPCSTR)(*m_Stack);
			break;

		case CM_INC:
			if ( m_Stack->m_Type == VALTYPE_LPBYTE ) {
				*((LPBYTE)*m_Stack) = *((LPBYTE)*m_Stack) + 1;
				*m_Stack = (int)(*((LPBYTE)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_LPWCHAR ) {
				*((WCHAR *)*m_Stack) = *((WCHAR *)*m_Stack) + 1;
				*m_Stack = (int)(*((WCHAR *)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_LPDCHAR ) {
				*((DCHAR *)*m_Stack) = *((DCHAR *)*m_Stack) + 1;
				*m_Stack = (int)(*((DCHAR *)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_LPDDOUB ) {
				*((double *)*m_Stack) = *((double *)*m_Stack) + 1.0;
				*m_Stack = *((double *)*m_Stack);
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
			} else if ( m_Stack->m_Type == VALTYPE_LPDCHAR ) {
				*((DCHAR *)*m_Stack) = *((DCHAR *)*m_Stack) - 1;
				*m_Stack = (int)(*((DCHAR *)*m_Stack));
			} else if ( m_Stack->m_Type == VALTYPE_LPDDOUB ) {
				*((double *)*m_Stack) = *((double *)*m_Stack) - 1.0;
				*m_Stack = *((double *)*m_Stack);
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
			} else if ( m_Stack->m_Type == VALTYPE_LPDCHAR ) {
				n = (int)(*((DCHAR *)*m_Stack));
				*((DCHAR *)*m_Stack) = *((DCHAR *)*m_Stack) + 1;
				*m_Stack = n;
			} else if ( m_Stack->m_Type == VALTYPE_LPDDOUB ) {
				double d = *((double *)*m_Stack);
				*((double *)*m_Stack) = d + 1.0;
				*m_Stack = d;
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
			} else if ( m_Stack->m_Type == VALTYPE_LPDCHAR ) {
				n = (int)(*((DCHAR *)*m_Stack));
				*((DCHAR *)*m_Stack) = *((DCHAR *)*m_Stack) - 1;
				*m_Stack = n;
			} else if ( m_Stack->m_Type == VALTYPE_LPDDOUB ) {
				double d = *((double *)*m_Stack);
				*((double *)*m_Stack) = d - 1.0;
				*m_Stack = d;
			} else if ( m_Stack->m_Type == VALTYPE_IDENT ) {
				n = (int)*((CScriptValue *)*m_Stack);
				*((CScriptValue *)*m_Stack) = (int)*((CScriptValue *)*m_Stack) - 1;
				*m_Stack = n;
			} else
				*m_Stack = (int)0;
			break;

		case CM_NEG:
			if ( m_Stack->GetType() == VALTYPE_COMPLEX ) {
				CComplex com(0.0, 0.0);
				com -= (CComplex &)*m_Stack;
				*m_Stack = com;
			} else if ( m_Stack->GetType() == VALTYPE_DOUBLE )
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
			else if ( m_Stack->GetType() == VALTYPE_DSTRING )
				*m_Stack = (int)(*((LPCDSTR)*m_Stack) == 0 ? 1 : 0);
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
					m_CodePos = m_Code[m_CodePos];
				else
					m_CodePos++;
			} else if ( m_Stack->GetType() == VALTYPE_WSTRING ) {
				if ( *((LPCWSTR)*m_Stack) != L'\0' )
					m_CodePos = m_Code[m_CodePos];
				else
					m_CodePos++;
			} else if ( m_Stack->GetType() == VALTYPE_DSTRING ) {
				if ( *((LPCDSTR)*m_Stack) != 0 )
					m_CodePos = m_Code[m_CodePos];
				else
					m_CodePos++;
			} else if ( (int)*m_Stack != 0 )
				m_CodePos = m_Code[m_CodePos];
			else
				m_CodePos++;
			break;
		case CM_IFN:
			if ( m_Stack->GetType() == VALTYPE_STRING ) {
				if ( *((LPCSTR)*m_Stack) == '\0' )
					m_CodePos = m_Code[m_CodePos];
				else
					m_CodePos++;
			} else if ( m_Stack->GetType() == VALTYPE_WSTRING ) {
				if ( *((LPCWSTR)*m_Stack) == L'\0' )
					m_CodePos = m_Code[m_CodePos];
				else
					m_CodePos++;
			} else if ( m_Stack->GetType() == VALTYPE_DSTRING ) {
				if ( *((LPCDSTR)*m_Stack) == 0 )
					m_CodePos = m_Code[m_CodePos];
				else
					m_CodePos++;
			} else if ( (int)*m_Stack == 0 )
				m_CodePos = m_Code[m_CodePos];
			else
				m_CodePos++;
			break;
		case CM_CASE:
			if ( sp->Compare(0, m_Stack) != 0 )
				m_CodePos = m_Code[m_CodePos];
			else
				m_CodePos++;
			break;

		case CM_JMP:
			m_CodePos = m_Code[m_CodePos];
			break;

		case CM_RET:
			CodeStackPop();
			break;
		case CM_CALL:
			// xyz(abc, def)
			// acc			def				abc				xyz
			// m_Stack		->m_Next(sp)	->m_Next		->m_Next
			// acc          xyz
			len = m_Code[m_CodePos++];			// 2
			m_Stack->RemoveAll();
			m_Stack->m_Array.SetSize(len);
			for ( n = len - 1 ; n >= 0 ; n-- ) {
				if ( sp == NULL )
					throw _T("Stack Under");
				m_Stack->m_Array[n] = sp;		// [1]=def [0]=abc
				sp = sp->m_Next;
			}									// sp = xyz
			m_Stack->m_Next = sp;				// acc->m_Next -> sp (remove def abc)
			*m_Stack = (CScriptValue *)sp;		// acc = xyz

			ASSERT(sp->m_Type == VALTYPE_IDENT);

			if ( sp->m_Type != VALTYPE_IDENT ) {
				StackPop();
				break;
			}

			dp = (CScriptValue *)*sp;			// dp = *xyz
//			*sp = (int)0;						// sp = xyz = 0
			if ( (n = dp->m_FuncPos) != (-1) ) {
				if ( (n & 0x80000000L) == 0 ) {
					CStringA name;
					dp->GetName(name);
					CodeStackPush(FALSE, MbsToTstr(name));
					m_ExecLocal = m_Stack;
					m_Stack     = m_Stack->m_Next;	// StackPop
					m_CodePos   = n;
					*sp = (int)0;				// sp = xyz = 0
				} else if ( dp->m_FuncExt != NULL ) {
					if ( (n = (this->*(dp->m_FuncExt))(n & 0x7FFFFFFFL, *m_Stack)) == FUNC_RET_EXIT ) {
						Abort();
						return FALSE;
					} else if ( n == FUNC_RET_ABORT ) {
						StackPop();
						return FALSE;
					} else if ( n == FUNC_RET_EVENT ) {
						*sp = (CScriptValue *)dp;	// sp = &xyz
						sp = m_Stack;
						m_Stack = sp->m_Next;		// pop acc
						for ( n = 0 ; n < len ; n++ ) {
							((CScriptValue *)sp->m_Array[n])->m_Next = m_Stack;
							m_Stack = (CScriptValue *)sp->m_Array[n];	// [0]=abc [1]=def
							sp->m_Array[n] = NULL;
						}
						sp->m_Next = m_Stack;
						m_Stack = sp;			// push acc
						*sp = (int)0;
						m_CodePos -= 2;
						return FALSE;
					} else	// FUNC_RET_NOMAL
						StackPop();
					m_EventMode = 0;
				}
			} else if ( m_pDoc != NULL && (n = dp->GetDocCmds()) != (-1) ) {
				// switch(n & 0x03)
				m_pDoc->ScriptValue(n >> 2, *m_Stack, DOC_MODE_CALL);
				StackPop();
			} else
				StackPop();
			break;

		case CM_ADDINDEX:
			if ( sp->m_Type == VALTYPE_IDENT ) {
				BYTE *p;
				sp = (CScriptValue *)(*sp);
				if ( sp->m_Type == VALTYPE_DSTRING ) {
					p = sp->m_Buf.PutSpc(sizeof(DCHAR));
					p[0] = p[1] = p[2] = p[3] = '\0';
					*(m_Stack->m_Next) = (WCHAR *)(p);
				} else if ( sp->m_Type == VALTYPE_WSTRING ) {
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
				if ( sp->m_Type == VALTYPE_COMPLEX ) {
					if ( n <= 0 )
						*(m_Stack->m_Next) = (double *)(&(sp->m_Complex.re));
					else if ( n >= 1 )
						*(m_Stack->m_Next) = (double *)(&(sp->m_Complex.im));
				} else if ( sp->m_Type == VALTYPE_DSTRING ) {
					n *= sizeof(DCHAR);
					if ( (i = n - sp->m_Buf.GetSize() + 4) > 0 ) {
						p = sp->m_Buf.PutSpc(i);
						memset(p, 0, i);
					}
					*(m_Stack->m_Next) = (DCHAR *)(sp->m_Buf.GetPos(n));
				} else if ( sp->m_Type == VALTYPE_WSTRING ) {
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
			{
				if ( m_Stack->m_Next->GetType() == VALTYPE_INT )
					sp = &((*(m_Stack->m_Next->m_Next))[(int)(*(m_Stack->m_Next))]);
				else
					sp = &((*(m_Stack->m_Next->m_Next))[(LPCSTR)(*(m_Stack->m_Next))]);
				CStringA index = sp->m_Index;
				*sp = *m_Stack;
				sp->m_Index = index;
				StackPop();
			}
			break;
		case CM_VALARRAY:
			sp = &((*(m_Stack->m_Next))[(LPCSTR)NULL]);
			*sp = *m_Stack;
			sp->m_Index.Empty();
			break;

		case CM_FOREACH:
			{
				CScriptValue *ap = m_Stack->m_Next;
				CScriptValue *vp = ap->m_Next;
				CScriptValue *cp = vp->m_Next;

				while ( ap->m_Type == VALTYPE_IDENT )
					ap = (CScriptValue *)*ap;
				while ( vp->m_Type == VALTYPE_IDENT )
					vp = (CScriptValue *)*vp;

				n = (int)(*cp);
				if ( n < ap->GetSize() ) {
					*vp = (*ap)[n];
					*cp = (int)(n + 1);
				} else {
					StackPop();		// array
					StackPop();		// value
					StackPop();		// counter
					m_CodePos = m_Code[m_CodePos];
				}
			}
			break;

		case CM_EACHWITHKEY:
			{
				CScriptValue *ap = m_Stack->m_Next;
				CScriptValue *vp = ap->m_Next;
				CScriptValue *kp = vp->m_Next;
				CScriptValue *cp = kp->m_Next;

				while ( ap->m_Type == VALTYPE_IDENT )
					ap = (CScriptValue *)*ap;
				while ( kp->m_Type == VALTYPE_IDENT )
					kp = (CScriptValue *)*kp;
				while ( vp->m_Type == VALTYPE_IDENT )
					vp = (CScriptValue *)*vp;

				n = (int)(*cp);
				if ( n < ap->GetSize() ) {
					*kp = (*ap)[n].m_Index;
					*vp = (*ap)[n];
					*cp = (int)(n + 1);
				} else {
					StackPop();		// array
					StackPop();		// value
					StackPop();		// key
					StackPop();		// counter
					m_CodePos = m_Code[m_CodePos];
				}
			}
			break;
		}
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// ComSock Func

int CScript::ComFunc(int cmd, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	if ( m_pDoc == NULL || m_pDoc->m_pSock == NULL || m_pDoc->m_pSock->m_Type != ESCT_COMDEV )
		return FUNC_RET_NOMAL;

	CComSock *pComSock = (CComSock *)m_pDoc->m_pSock;

	if ( pComSock->m_hCom == INVALID_HANDLE_VALUE || pComSock->m_pComConf == NULL || pComSock->m_ThreadMode != CComSock::THREAD_RUN )
		return FUNC_RET_NOMAL;

	switch(cmd) {
	case 0:		// comset(str)
		pComSock->SetDcdStr((LPCTSTR)local[0]);
		(*acc) = pComSock->SetComConf();
		break;

	case 1:		// comdtr(sw)
		if ( pComSock->m_pComConf->dcb.fDtrControl == DTR_CONTROL_DISABLE && (int)local[0] != 0 ) {
			pComSock->m_pComConf->dcb.fDtrControl = DTR_CONTROL_ENABLE;
			pComSock->SetComCtrl(SETDTR);
		} else if ( pComSock->m_pComConf->dcb.fDtrControl == DTR_CONTROL_ENABLE && (int)local[0] == 0 ) {
			pComSock->m_pComConf->dcb.fDtrControl = DTR_CONTROL_DISABLE;
			pComSock->SetComCtrl(CLRDTR);
		}
		break;

	case 2:		// comrts(sw)
		if ( pComSock->m_pComConf->dcb.fRtsControl == RTS_CONTROL_DISABLE && (int)local[0] != 0 ) {
			pComSock->m_pComConf->dcb.fRtsControl = RTS_CONTROL_ENABLE;
			pComSock->SetComCtrl(SETRTS);
		} else if ( pComSock->m_pComConf->dcb.fRtsControl == RTS_CONTROL_ENABLE && (int)local[0] == 0 ) {
			pComSock->m_pComConf->dcb.fRtsControl = RTS_CONTROL_DISABLE;
			pComSock->SetComCtrl(CLRRTS);
		}
		break;

	case 3:		// combreak(sw)
		pComSock->SendBreak((int)local[0]);
		break;
	}

	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// TextWnd Func

int CScript::TextWnd(int cmd, CScriptValue &local)
{
	static const ScriptFunc textwndfuncs[] = {
		{ "open",		1,	&CScript::TextWnd },	{ "close",		2,	&CScript::TextWnd },
		{ "gettext",	3,	&CScript::TextWnd },	{ "settext",	4,	&CScript::TextWnd },
		{ "addtext",	5,	&CScript::TextWnd },

		{ NULL,			0,	NULL }
	};

	CStatusDlg *pWnd;
	CString text;
	CScriptValue *acc = (CScriptValue *)local;
	CScriptValue *base = (CScriptValue *)*acc;
	acc->Empty();

	if ( base != NULL && base->m_Root != NULL )
		base = base->m_Root;

	switch(cmd) {
	case 0:		// textwnd(title, text)
		(*acc)["pWnd"] = (void *)NULL;
		(*acc)["TitleText"] = (LPCTSTR)local[0];
		(*acc)["StatusText"] = (LPCTSTR)local[1];
		for ( int n = 0 ; textwndfuncs[n].name != NULL ; n++ ) {
			(*acc)[(LPCSTR)textwndfuncs[n].name].m_FuncPos = (textwndfuncs[n].cmd | 0x80000000L);
			(*acc)[(LPCSTR)textwndfuncs[n].name].m_FuncExt = textwndfuncs[n].proc;
		}
		break;
	case 1:		// textwnd.open()
		if ( base == NULL )
			break;
		if ( (pWnd = (CStatusDlg *)((void *)(base->GetAt("pWnd")))) != NULL )
			break;
		pWnd = new CStatusDlg(NULL);
		pWnd->m_Title = (LPCTSTR)(base->GetAt("TitleText"));
		pWnd->m_OwnerType = 1;
		pWnd->m_pValue = (void *)base;
		base->GetAt("pWnd") = (void *)pWnd;
		if ( pWnd->Create(IDD_STATUS_DLG, CWnd::GetDesktopWindow()) ) {
			pWnd->ShowWindow(SW_SHOW);
			pWnd->SetStatusText((LPCTSTR)(base->GetAt("StatusText")));
		}
		break;
	case 2:		// textwnd.close()
		if ( base == NULL || (pWnd = (CStatusDlg *)((void *)(base->GetAt("pWnd")))) == NULL )
			break;
		pWnd->DestroyWindow();
		base->GetAt("pWnd") = (void *)NULL;
		break;
	case 3:		// textwnd.gettext()
		if ( base == NULL || (pWnd = (CStatusDlg *)((void *)(base->GetAt("pWnd")))) == NULL )
			break;
		pWnd->GetStatusText(text);
		(*acc) = (LPCTSTR)text;
		break;
	case 4:		// textwnd.settext()
		if ( base == NULL || (pWnd = (CStatusDlg *)((void *)(base->GetAt("pWnd")))) == NULL )
			break;
		pWnd->SetStatusText((LPCTSTR)local[0]);
		break;
	case 5:		// textwnd.addtext()
		if ( base == NULL || (pWnd = (CStatusDlg *)((void *)(base->GetAt("pWnd")))) == NULL )
			break;
		pWnd->AddStatusText((LPCTSTR)local[0]);
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// DataBase Func

int CScript::DataBs(int cmd, CScriptValue &local)
{
	static const ScriptFunc databsfuncs[] = {
		{ "open",		1,	&CScript::DataBs },	{ "close",		2,	&CScript::DataBs },
		{ "sql",		3,	&CScript::DataBs },	{ "count",		4,	&CScript::DataBs },
		{ "fetch",		5,	&CScript::DataBs },	{ "move",		6,	&CScript::DataBs },

		{ NULL,			0,	NULL }
	};

	CScriptValue *acc = (CScriptValue *)local;
	CScriptValue *base = (CScriptValue *)*acc;
	acc->Empty();

	if ( base != NULL && base->m_Root != NULL )
		base = base->m_Root;

	switch(cmd) {
	case 0:		// database()
		{
			CDatabase *pDb = new CDatabase;
			(*acc)["pDatabase"]   = (void *)pDb;
			(*acc)["pRecordset "] = (void *)NULL;
			for ( int n = 0 ; databsfuncs[n].name != NULL ; n++ ) {
				(*acc)[(LPCSTR)databsfuncs[n].name].m_FuncPos = (databsfuncs[n].cmd | 0x80000000L);
				(*acc)[(LPCSTR)databsfuncs[n].name].m_FuncExt = databsfuncs[n].proc;
			}
		}
		break;
	case 1:		// database.open(name, mode)
		if ( base != NULL ) {
			CDatabase *pDb = (CDatabase *)(void *)base->GetAt("pDatabase");
			if ( pDb != NULL ) {
				try {
					(*acc) = (int)pDb->OpenEx((LPCTSTR)local[0], (DWORD)(int)local[1]);
				} catch(...) {
					(*acc) = (int)0;
				}
			} else
				(*acc) = (int)0;
		} else
			(*acc) = (int)0;
		break;
	case 2:		// database.close()
		if ( base != NULL ) {
			CRecordset *pRs = (CRecordset *)(void *)base->GetAt("pRecordset");
			CDatabase *pDb = (CDatabase *)(void *)base->GetAt("pDatabase");
			if ( pRs != NULL ) {
				pRs->Close();
				delete pRs;
				base->GetAt("pRecordset") = (void *)NULL;
			}
			if ( pDb != NULL ) {
				pDb->Close();
				delete pDb;
				base->GetAt("pDatabase") = (void *)NULL;
			}
		}
		break;
	case 3:		// database.sql(sql)
		if ( base != NULL ) {
			CDatabase *pDb = (CDatabase *)(void *)base->GetAt("pDatabase");
			CRecordset *pRs = (CRecordset *)(void *)base->GetAt("pRecordset");
			if ( pDb == NULL )
				break;
			if ( pRs != NULL )
				pRs->Close();
			else {
				pRs = new CRecordset(pDb);
				base->GetAt("pRecordset") = (void *)pRs;
			}
			try {
				(*acc) = (int)pRs->Open(AFX_DB_USE_DEFAULT_TYPE, (LPCTSTR)local[0]);
			} catch(...) {
				(*acc) = (int)0;
			}
		} else
			(*acc) = (int)0;
		break;
	case 4:		// database.count()
		if ( base != NULL ) {
			CRecordset *pRs = (CRecordset *)(void *)base->GetAt("pRecordset");
			if ( pRs != NULL ) {
				try {
					int len = 0;
					pRs->MoveFirst();
					while ( !pRs->IsEOF() ) {
						len++;
						pRs->MoveNext();
					}
					pRs->MoveFirst();
					(*acc) = len;
				} catch(...) {
					(*acc) = (int)0;
				}
			} else
				(*acc) = (int)0;
		} else
			(*acc) = (int)0;
		break;
	case 5:		// database.fetch()
		(*acc).RemoveAll();
		(*acc).Empty();
		if ( base != NULL ) {
			CRecordset *pRs = (CRecordset *)(void *)base->GetAt("pRecordset");
			if ( pRs == NULL || pRs->IsEOF() )
				break;
			try {
				int n;
				int len = pRs->GetODBCFieldCount();
				CODBCFieldInfo fieldInfo;
				CStringA name;
				CDBVariant val;

				for ( n = 0 ; n < len ; n++ ) {
					pRs->GetODBCFieldInfo(n, fieldInfo);
					name = fieldInfo.m_strName;
					pRs->GetFieldValue(n, val);

					switch (val.m_dwType) {
					case DBVT_NULL:
						(*acc)[name].Empty();
						break;
					case DBVT_BOOL:
						(*acc)[name] = (int)val.m_boolVal;
						break;
					case DBVT_UCHAR:
						(*acc)[name] = (int)val.m_chVal;
						break;
					case DBVT_SHORT:
						(*acc)[name] = (int)val.m_iVal;
						break;
					case DBVT_LONG:
						(*acc)[name] = (int)val.m_lVal;
						break;
					case DBVT_SINGLE:
						(*acc)[name] = (double)val.m_fltVal;
						break;
					case DBVT_DOUBLE:
						(*acc)[name] = (double)val.m_dblVal;
						break;
					case DBVT_DATE:
						{
							time_t t;
							struct tm *tm;
							time(&t);
							tm = localtime(&t);
							val.m_pdate;
							tm->tm_year = val.m_pdate->year - 1900;
							tm->tm_mday = val.m_pdate->day;
							tm->tm_mon  = val.m_pdate->month - 1;
							tm->tm_sec  = val.m_pdate->second;
							tm->tm_min  = val.m_pdate->minute;
							tm->tm_hour = val.m_pdate->hour;
							(*acc)[name] = (int)mktime(tm);
						}
						break;
					case DBVT_STRING:
						(*acc)[name] = (LPCTSTR)*(val.m_pstring);
						break;
					case DBVT_BINARY:
						{
							LPBYTE pByte;
							CBuffer *bp = (*acc)[name].GetBuf();
							if ( (pByte = (LPBYTE)GlobalLock(val.m_pbinary->m_hData)) == NULL )
								break;
							bp->Apend(pByte, (int)val.m_pbinary->m_dwDataLength);
							GlobalUnlock(pByte);
						}
						break;
					case DBVT_ASTRING:
						(*acc)[name] = (LPCSTR)*(val.m_pstringA);
						break;
					case DBVT_WSTRING:
						(*acc)[name] = (LPCWSTR)*(val.m_pstringW);
						break;
					default:
						(*acc)[name].Empty();
						break;
					}
				}
				(*acc) = (int)len;
				pRs->MoveNext();

			} catch(...) {
				;
			}
		}
		break;
	case 6:			// database.move(pos)
		if ( base != NULL ) {
			CRecordset *pRs = (CRecordset *)(void *)base->GetAt("pRecordset");
			if ( pRs == NULL )
				break;
			try {
				int n = (int)local[0];
				if ( n == 0 )
					pRs->MoveFirst();
				else if ( n < 0 )
					pRs->MoveLast();
				else
					pRs->SetAbsolutePosition(n + 1);
			} catch(...) {
				;
			}
		}
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// Dialog Func

int CScript::Dialog(int cmd, CScriptValue &local)
{
	static const ScriptFunc dialogfuncs[] = {
		{ "add",		1,	&CScript::Dialog },	{ "open",		2,	&CScript::Dialog },
		{ "close",		3,	&CScript::Dialog },	{ "center",		4,	&CScript::Dialog },
		{ "show",		5,	&CScript::Dialog },	{ "wait",		6,	&CScript::Dialog },
		{ "gettext",	7,	&CScript::Dialog },	{ "getcheck",	8,	&CScript::Dialog },
		{ "getradio",	9,	&CScript::Dialog },	{ "getselect",	10,	&CScript::Dialog },
		{ "settext",	11,	&CScript::Dialog },	{ "setcheck",	12,	&CScript::Dialog },
		{ "setradio",	13,	&CScript::Dialog },	{ "setselect",	14,	&CScript::Dialog },
		{ "setlist",	15,	&CScript::Dialog },	{ "setrange",	16,	&CScript::Dialog },
		{ "setpos",		17,	&CScript::Dialog },	{ "modstyle",	18,	&CScript::Dialog },
		{ "sendmsg",	19,	&CScript::Dialog },

		{ NULL,			0,	NULL }
	};

	CScriptValue *acc = (CScriptValue *)local;
	CScriptValue *base = (CScriptValue *)*acc;
	acc->Empty();

	if ( base != NULL && base->m_Root != NULL )
		base = base->m_Root;

	switch(cmd) {
	case 0:	// dialog(name, size, child)
		{
			int n;
			(*acc)["pWnd"] = (void *)NULL;
			(*acc)["WindowText"] = (LPCTSTR)local[0];
			(*acc)["Size"]["sx"] = (int)(local[1].GetAt(0));
			(*acc)["Size"]["sy"] = (int)(local[1].GetAt(1));
			(*acc)["Size"]["cx"] = (int)(local[1].GetAt(2));
			(*acc)["Size"]["cy"] = (int)(local[1].GetAt(3));
			for ( n = 0 ; n < local[2].GetSize() ; n++ ) {
				(*acc)["Child"][n]["pWnd"]       = (void *)NULL;
				(*acc)["Child"][n]["Type"]       = (LPCSTR)(local[2].GetAt(n).GetAt(0));
				(*acc)["Child"][n]["Size"]["sx"] = (int)(local[2].GetAt(n).GetAt(1).GetAt(0));
				(*acc)["Child"][n]["Size"]["sy"] = (int)(local[2].GetAt(n).GetAt(1).GetAt(1));
				(*acc)["Child"][n]["Size"]["cx"] = (int)(local[2].GetAt(n).GetAt(1).GetAt(2));
				(*acc)["Child"][n]["Size"]["cy"] = (int)(local[2].GetAt(n).GetAt(1).GetAt(3));
				(*acc)["Child"][n]["Text"]       = (LPCTSTR)(local[2].GetAt(n).GetAt(2));
				(*acc)["Child"][n]["Check"]	     = (int)(local[2].GetAt(n).GetAt(3));
				(*acc)["Child"][n]["Func"]	     = (LPCSTR)(local[2].GetAt(n).GetAt(4));
				(*acc)["Child"][n]["Value"]	     = (int)(-1);
				(*acc)["Child"][n]["Group"]	     = (int)(-1);
			}
			(*acc)["FontPoint"] = (int)local[3];
			(*acc)["FontName"]  = (LPCTSTR)local[4];
			for ( n = 0 ; dialogfuncs[n].name != NULL ; n++ ) {
				(*acc)[(LPCSTR)dialogfuncs[n].name].m_FuncPos = (dialogfuncs[n].cmd | 0x80000000L);
				(*acc)[(LPCSTR)dialogfuncs[n].name].m_FuncExt = dialogfuncs[n].proc;
			}
		}
		break;
	case 1:	// dialog.add(type, size, text, state, func)
		if ( base != NULL ) {
			int n = base->GetAt("Child").GetSize();
			base->GetAt("Child").GetAt(n).GetAt("pWnd")       = (void *)NULL;
			base->GetAt("Child").GetAt(n).GetAt("Type")       = (LPCSTR)(local[0]);
			base->GetAt("Child").GetAt(n).GetAt("Size")["sx"] = (int)(local[1].GetAt(0));
			base->GetAt("Child").GetAt(n).GetAt("Size")["sy"] = (int)(local[1].GetAt(1));
			base->GetAt("Child").GetAt(n).GetAt("Size")["cx"] = (int)(local[1].GetAt(2));
			base->GetAt("Child").GetAt(n).GetAt("Size")["cy"] = (int)(local[1].GetAt(3));
			base->GetAt("Child").GetAt(n).GetAt("Text")       = (LPCTSTR)(local[2]);
			base->GetAt("Child").GetAt(n).GetAt("Check")     = (int)(local[3]);
			base->GetAt("Child").GetAt(n).GetAt("Func")	     = (LPCSTR)(local[4]);
			base->GetAt("Child").GetAt(n).GetAt("Value")     = (int)(-1);
			base->GetAt("Child").GetAt(n).GetAt("Group")     = (int)(-1);
			(*acc) = (int)n;
		} else
			(*acc) = (int)(-1);
		break;

	case 2:	// dialog.open()
		if ( base != NULL ) {
			CTempWnd *pWnd;
			CRect rect((int)base->GetAt("Size")["sx"], (int)base->GetAt("Size")["sy"],
					   (int)base->GetAt("Size")["cx"], (int)base->GetAt("Size")["cy"]);
			rect.right += rect.left; rect.bottom += rect.top;

			pWnd = (CTempWnd *)(void *)base->GetAt("pWnd");
			if ( pWnd != NULL ) pWnd->DestroyWindow();

			pWnd = new CTempWnd;
			base->GetAt("pWnd") = (void *)pWnd;
			pWnd->m_pScript = this;
			pWnd->m_pValue  = base;

			if ( pWnd->CreateEx(WS_EX_WINDOWEDGE,
					AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
						AfxGetApp()->LoadCursor(IDC_ARROW), (HBRUSH)(COLOR_WINDOW)),
					(LPCTSTR)base->GetAt("WindowText"),
					WS_POPUP  | WS_CAPTION | WS_SYSMENU | WS_BORDER,	// WS_VISIBLE
					rect, NULL, NULL, NULL) )
				(*acc) = (int)1;
		}
		break;
	case 3:	// dialog.close()
		if ( base != NULL ) {
			CTempWnd *pWnd = (CTempWnd *)((void *)(base->GetAt("pWnd")));
			if ( pWnd != NULL )
				pWnd->DestroyWindow();
		}
		break;

	case 4:	// dialog.center()
		if ( base != NULL ) {
			CWnd *pWnd;
			CRect frame;
			CRect rect((int)base->GetAt("Size")["sx"], (int)base->GetAt("Size")["sy"],
					   (int)base->GetAt("Size")["cx"], (int)base->GetAt("Size")["cy"]);
			rect.right += rect.left; rect.bottom += rect.top;

			int cx = rect.Width();
			int cy = rect.Height();

			if ( (pWnd = AfxGetMainWnd()) == NULL )
				break;

			pWnd->GetWindowRect(frame);

			rect.left = frame.left + (frame.Width()  - cx) / 2;
			rect.top  = frame.top  + (frame.Height() - cy) / 2;
			rect.right  = rect.left + cx;
			rect.bottom = rect.top  + cy;

			if ( (pWnd = (CWnd *)(void *)base->GetAt("pWnd")) != NULL )
				pWnd->SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;
	case 5:	// dialog.show(f)
		if ( base != NULL ) {
			CTempWnd *pWnd = (CTempWnd *)(void *)base->GetAt("pWnd");
			if ( pWnd != NULL )
				pWnd->ShowWindow((int)local[0] == 0 ? SW_HIDE : SW_SHOW);
		}
		break;
	case 6:	// dialog.wait()
		if ( base != NULL ) {
			CTempWnd *pWnd = (CTempWnd *)(void *)base->GetAt("pWnd");
			if ( pWnd != NULL )
				return SetEvent(SCP_EVENT_DIALOG);
		}
		break;

	case 7:	// dialog.gettext(id)
		if ( base != NULL )
			(*acc) = (LPCTSTR)base->GetAt("Child").GetAt((int)local[0]).GetAt("Text");
		else
			(*acc) = (LPCSTR)"";
		break;
	case 8:	// dialog.getcheck(id)
		if ( base != NULL )
			(*acc) = (int)base->GetAt("Child").GetAt((int)local[0]).GetAt("Check");
		else
			(*acc) = (int)0;
		break;
	case 9:	// dialog.getradio(id)
		if ( base != NULL )
			(*acc) = (int)base->GetAt("Child").GetAt((int)local[0]).GetAt("Value");
		else
			(*acc) = (int)(-1);
		break;
	case 10:	// dialog.getselect(id)
		if ( base != NULL )
			(*acc) = (int)base->GetAt("Child").GetAt((int)local[0]).GetAt("Value");
		else
			(*acc) = (int)(-1);
		break;

	case 11:	// dialog.settext(id, str)
		if ( base != NULL ) {
			CWnd *pWnd = (CWnd *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			pWnd->SetWindowText((LPCTSTR)local[1]);
			base->GetAt("Child").GetAt((int)local[0]).GetAt("Text") = (LPCTSTR)local[1];
		}
		break;
	case 12:	// dialog.setcheck(id, state)
		if ( base != NULL ) {
			CButton *pWnd = (CButton *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			pWnd->SetCheck((int)local[1]);
			base->GetAt("Child").GetAt((int)local[0]).GetAt("Check") = (int)local[1];
		}
		break;
	case 13:	// dialog.setradio(id, vallue)
		if ( base != NULL ) {
			int id = (int)base->GetAt("Child").GetAt((int)local[0]).GetAt("Group");
			if ( id == (-1) )
				break;
			int nd = id + (int)local[1];
			for ( int n = id ; n < base->GetAt("Child").GetSize() ; n++ ) {
				if ( (int)base->GetAt("Child").GetAt(n).GetAt("Group") != id )
					break;
				CButton *pWnd = (CButton *)(void *)base->GetAt("Child").GetAt(n).GetAt("pWnd");
				if ( pWnd == NULL )
					continue;
				if ( n == nd ) {
					pWnd->SetCheck(BST_CHECKED);
					base->GetAt("Child").GetAt(id).GetAt("Value") = (int)(n - id);
				} else
					pWnd->SetCheck(BST_UNCHECKED);
			}
		}
		break;
	case 14:	// setselect(id, num)
		if ( base != NULL ) {
			CComboBox *pWnd = (CComboBox *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			pWnd->SetCurSel((int)local[1]);
			base->GetAt("Child").GetAt((int)local[0]).GetAt("Value") = (int)local[1];
		}
		break;
	case 15:	// setlistt(id, list)
		if ( base != NULL ) {
			CComboBox *pWnd = (CComboBox *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			for ( int n = 0 ; n < local[1].GetSize() ; n++ )
				pWnd->AddString((LPCTSTR)(local[1][n]));
		}
		break;

	case 16:	// setrange(id, min, max)
		if ( base != NULL ) {
			CProgressCtrl *pWnd = (CProgressCtrl *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			pWnd->SetRange32((int)local[1], (int)local[2]);
		}
		break;
	case 17:	// setpos(id, pos)
		if ( base != NULL ) {
			CProgressCtrl *pWnd = (CProgressCtrl *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			pWnd->SetPos((int)local[1]);
		}
		break;

	case 18:	// modstyle(id, del, add, delex, addex)
		if ( base != NULL ) {
			CWnd *pWnd = (CWnd *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			pWnd->ModifyStyle((int)local[1], (int)local[2]);
			if ( local.GetSize() > 3 )
				pWnd->ModifyStyleEx((int)local[3], (int)local[4], SWP_FRAMECHANGED);
		}
		break;
	case 19:	// sendmsg(id, message, rparam, lparam)
		if ( base != NULL ) {
			CWnd *pWnd = (CWnd *)(void *)base->GetAt("Child").GetAt((int)local[0]).GetAt("pWnd");
			if ( pWnd == NULL )
				break;
			if ( local.GetSize() > 3 ) {
				if ( local[3].GetType() == VALTYPE_WSTRING )
					(*acc) = (int)pWnd->SendMessage((int)local[1], (int)local[2], (LPARAM)(local[3].GetWBuf()->GetPtr()));
				else
					(*acc) = (int)pWnd->SendMessage((int)local[1], (int)local[2], (LPARAM)(local[3].GetBuf()->GetPtr()));
			} else
				(*acc) = (int)pWnd->SendMessageW((int)local[1], (int)local[2]);
		}
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// TekGrap Func

int CScript::TekStyle(int idx, CScriptValue &local)
{
	int st = 0;
	int color, style, size;

	if ( local.GetSize() > idx )
		color = m_Data["tek"]["color"] = (int)local[idx];
	else
		color = m_Data["tek"]["color"];

	if ( local.GetSize() > (idx + 1) )
		style = m_Data["tek"]["style"] = (int)local[idx + 1];
	else
		style = m_Data["tek"]["style"];

	if ( local.GetSize() > (idx + 2) )
		size = m_Data["tek"]["size"] = (int)local[idx + 2];
	else
		size = m_Data["tek"]["size"];

	if ( local.GetSize() > (idx + 3) )
		m_Data["tek"]["flush"] = (int)local[idx + 3];

	if ( size < 1 )
		size = 1;
	else if ( size > 2 )
		size = 2;

	st |= ((color & 15) << 3);
	st |= (style & 7);
	st |= (((size - 1) & 1) << 7);

	return st;
}

#define	GAUSS_MAX	10

static void gauss(double a[GAUSS_MAX][GAUSS_MAX], int n, double xx[GAUSS_MAX])
{
    int i, j, k, l, piv;
    double p, q, m, b[1][GAUSS_MAX];

    for ( i = 0 ; i < n ; i++ ) {
		m = 0.0;
		piv = i;

		for ( l = i ; l < n ; l++ ) {
			if ( fabs(a[l][i]) > m ) {
				m = fabs(a[l][i]);
				piv = l;
			}
		}

		if ( piv != i ) {
			for ( j = 0 ; j < (n + 1) ; j++ ) {
				b[0][j] = a[i][j];
				a[i][j] = a[piv][j];
				a[piv][j] = b[0][j];
			}
		}
    }

    for ( k = 0 ; k < n ; k++ ) {
		p = a[k][k];
		a[k][k] = 1.0;

		for ( j = k + 1 ; j < (n + 1) ; j++ ) {
			if ( p == 0 )
	    		a[k][j] = 0.0;
			else
	    		a[k][j] = a[k][j] / p;
		}


		for ( i = k + 1 ; i < n ; i++ ) {
			q = a[i][k];

			for ( j = k + 1 ; j < (n + 1) ; j++ )
				a[i][j] = a[i][j] - q * a[k][j];

			a[i][k] = 0.0;
		}
    }

    for ( i = n - 1 ; i >= 0 ; i-- ) {
		xx[i] = a[i][n];

		for ( j = n - 1 ; j > i ; j-- )
			xx[i] = xx[i] - a[i][j] * xx[j];
    }
}
static void sai2pow(double x[], double y[], int m, int n, double xx[GAUSS_MAX])
{
    int i, j, k;
    double a[GAUSS_MAX][GAUSS_MAX];

    if ( ++n >= GAUSS_MAX )
		n = GAUSS_MAX - 1;

    for ( i = 0 ; i < n ; i++ ) {
		for ( j = 0 ; j < (n + 1) ; j++ )
			a[i][j] = 0.0;
    }
    
    for ( i = 0 ; i < n ; i++ ) {
		for ( j = 0 ; j < n ; j++ ) {
			for ( k = 0 ; k < m ; k++ )
			a[i][j] = a[i][j] + pow(x[k], i + j);
		}
    }

    for ( i = 0 ; i < n ; i++ ) {
		for ( k = 0 ; k < m ; k++ )
			a[i][n] = a[i][n] + pow(x[k], i) * y[k];
    }

    gauss(a, n, xx);
}
static double sai2y(double x, int n, double xx[GAUSS_MAX])
{
	int i;
	double y = 0;

	for ( i = 0 ; i < n ; i++ )
		y = y + xx[i] * pow(x, i);

	return y;
}
static void spline_init(double x[], double y[], int n, double xx[GAUSS_MAX])
{
	int i, j;
	double f;
    double a[GAUSS_MAX][GAUSS_MAX];

	for ( i = 0 ; i < n ; i++ ) {
		a[i][0] = 1.0;
		f = x[i];
		for ( j = 1 ; j < n ; j++ ) {
			a[i][j] = f;
			f = f * f;
		}
		a[i][n] = y[i];
	}

	gauss(a, n, xx);
}
static double spline_calc(double x, int n, double xx[GAUSS_MAX])
{
    int i;
    double y = xx[0];

    for ( i = 1 ; i < n ; i++ ) {
		y = y + xx[i] * x;
		x = x * x;
    }
    return y;
}
static double lagrange_calc(double x1, double *x, double *y, int n)
{
    double p = 0;
	for ( int i = 0 ; i < n ; i++ ) {
		double c = y[i];
		for ( int j = 0 ; j < n ; j++ ){
			if ( j != i)
				c *= (x1 - x[j]) / (x[i] - x[j]);
		}
		p += c;
	}
	return p;
}

int CScript::TekGrp(int cmd, CScriptValue &local)
{
	int n;
	int st = 0;
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	if ( m_pDoc == NULL )
		return FUNC_RET_NOMAL;

	switch(cmd) {
	case 0:		// tekopen(s)
		m_pDoc->m_TextRam.TekInit(4105);
		if ( (int)local[0] == 0 )
			m_pDoc->m_TextRam.TekInit(0);
		else
			m_pDoc->m_TextRam.EnableOption(TO_RLTEKINWND);
		break;
	case 1:		// tekclose
		m_pDoc->m_TextRam.TekClose();
		break;
	case 2:		// tekclear(f)
		m_Data["tek"]["flush"] = (int)local[0];
		m_pDoc->m_TextRam.TekClear((int)local[0] == 0 ? TRUE : FALSE);
		break;
	case 3:		// tekmark(no, sx, sy, color, style, size, flush)
		st = TekStyle(3, local);
		m_pDoc->m_TextRam.TekMark(st, (int)local[0], (int)local[1], (int)local[2]);
		if ( (int)m_Data["tek"]["flush"] == 0 )
			m_pDoc->m_TextRam.TekFlush();
		break;
	case 4:		// tekline(sx, sy, ex, ey, color, style, size, flush)	st = wccccsss
		st = TekStyle(4, local);
		m_pDoc->m_TextRam.TekLine(st, (int)local[0], (int)local[1], (int)local[2], (int)local[3]);
		if ( (int)m_Data["tek"]["flush"] == 0 )
			m_pDoc->m_TextRam.TekFlush();
		break;
	case 5:		// tektext(sx, sy, str, color, size, angle, flush)		st = 0ccccsss
		{
			int n, ch, col = 0, fsz = 0, ang = 0;
			LPCWSTR p = (LPCWSTR)local[2];
			if ( local.GetSize() > 3 )
				col = m_Data["tek"]["color"] = (int)local[3];
			else
				col = m_Data["tek"]["color"];
			if ( local.GetSize() > 4 )
				fsz = m_Data["tek"]["font"] = (int)local[4];
			else
				fsz = m_Data["tek"]["font"];
			if ( local.GetSize() > 5 )
				ang = m_Data["tek"]["angle"] = (int)local[5];
			else
				ang = m_Data["tek"]["angle"];
			if ( local.GetSize() > 6 )
				m_Data["tek"]["flush"] = (int)local[6];
			st |= ((col & 15) << 3);
			st |= (fsz & 7);
			m_pDoc->m_TextRam.m_Tek_cX = (int)local[0];
			m_pDoc->m_TextRam.m_Tek_cY = (int)local[1];
			m_pDoc->m_TextRam.m_Tek_Angle = ang % 360;
			while ( *p != L'\0' ) {
				if ( (p[0] & 0xFC00) == 0xD800 && (p[1] & 0xFC00) == 0xDC00 ) {
					ch = (p[0] << 16) | p[1];
					p += 2;
				} else
					ch = *(p++);
				m_pDoc->m_TextRam.TekText(st, m_pDoc->m_TextRam.m_Tek_cX, m_pDoc->m_TextRam.m_Tek_cY, ch);
				for ( n = m_pDoc->m_TextRam.UnicodeWidth(ch) ; n > 0 ; n-- )
					m_pDoc->m_TextRam.fc_TEK_RIGHT(0x09);
			}
			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 6:		// tekbox(sx, sy, ex, ey, color, style, size, flush)	st = wccccsss
		{
			st = TekStyle(4, local);
			int sx = (int)local[0];
			int sy = (int)local[1];
			int ex = (int)local[2];
			int ey = (int)local[3];
			m_pDoc->m_TextRam.TekLine(st, sx, sy, ex, sy);
			m_pDoc->m_TextRam.TekLine(st, ex, sy, ex, ey);
			m_pDoc->m_TextRam.TekLine(st, ex, ey, sx, ey);
			m_pDoc->m_TextRam.TekLine(st, sx, ey, sx, sy);
			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 7:	// tekarc(x, y, r, color, style, size, flush)
		{
			int x = (int)local[0];
			int y = (int)local[1];
			int r = (int)local[2];
			int m = r / 5;
			int sx = x + (int)(r * cos(0.0));
			int sy = y + (int)(r * sin(0.0));
			st = TekStyle(3, local);
			if ( m == 0 )
				m_pDoc->m_TextRam.TekLine(st, x, y, x, y);
			else {
				for ( n =  0 ; n <= m ; n++ ) {
					int ex = x + (int)(r * cos(3.14159265358979323846 * 2 * n / m));
					int ey = y + (int)(r * sin(3.14159265358979323846 * 2 * n / m));
					m_pDoc->m_TextRam.TekLine(st, sx, sy, ex, ey);
					sx = ex;
					sy = ey;
				}
			}
			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 8:		// tekpoly(xy, color, style, size, flush)
		{
			int sx, sy, ex, ey;
			CScriptValue *sp = (CScriptValue *)local[0];
			st = TekStyle(1, local);
			if ( sp->GetSize() < 2 )
				break;
			for ( n = 0 ; n < sp->GetSize() ; n++ ) {
				if ( (*sp)[n].GetType() == VALTYPE_COMPLEX ) {
					ex = (int)((CComplex &)((*sp)[n])).re;
					ey = (int)((CComplex &)((*sp)[n])).im;
				} else {
					ex = (*sp)[n].GetAt("x");
					ey = (*sp)[n].GetAt("y");
				}
				if ( n > 0 )
					m_pDoc->m_TextRam.TekLine(st, sx, sy, ex, ey);
				sx = ex;
				sy = ey;
			}
			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 9:		// tekbezier(xy, color, style, size, flush)
		{
			CPoint tmp;
			CArray<CPoint, CPoint &> po, da;
			CScriptValue *sp = (CScriptValue *)local[0];
			st = TekStyle(1, local);
			if ( sp->GetSize() < 4 )
				break;
			for ( n = 0 ; n < sp->GetSize() ; n++ ) {
				if ( (*sp)[n].GetType() == VALTYPE_COMPLEX )
					po.Add(CPoint((int)(((CComplex &)((*sp)[n])).re), (int)(((CComplex &)((*sp)[n])).im)));
				else
					po.Add(CPoint((int)((*sp)[n].GetAt("x")), (int)((*sp)[n].GetAt("y"))));
			}
			for ( n = 0 ; n < (po.GetSize() - 3) ; n += 3 ) {
				int m = (int)(sqrt(pow((double)po[n].x - (double)po[n + 3].x, 2) + pow((double)po[n].y - (double)po[n + 3].y, 2)) / 10.0);
				if ( m < 1 ) m = 1;
				for ( int c = 0 ; c <= m ; c++ ) {
					double t = (double)c / m;
					tmp.x = (int)(pow(1 - t, 3) * po[n].x + 3 * pow(1 - t, 2) * t * po[n + 1].x + 3 * (1 - t) * pow(t, 2) * po[n + 2].x + pow(t, 3) * po[n + 3].x);
					tmp.y = (int)(pow(1 - t, 3) * po[n].y + 3 * pow(1 - t, 2) * t * po[n + 1].y + 3 * (1 - t) * pow(t, 2) * po[n + 2].y + pow(t, 3) * po[n + 3].y);
					da.Add(tmp);
				}
			}
			for ( n = 1 ; n < da.GetSize() ; n++ )
				m_pDoc->m_TextRam.TekLine(st, da[n - 1].x, da[n - 1].y, da[n].x, da[n].y);

			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 10:	// teksmooth(xy, color, style, size, flush)
		{
			CPoint tmp;
			BOOL closed = FALSE;
			CArray<CPoint, CPoint &> da, po;
			CScriptValue *sp = (CScriptValue *)local[0];
			st = TekStyle(1, local);
			if ( sp->GetSize() < 2 )
				break;
			for ( n = 0 ; n < sp->GetSize() ; n++ ) {
				if ( (*sp)[n].GetType() == VALTYPE_COMPLEX )
					da.Add(CPoint((int)(((CComplex &)((*sp)[n])).re), (int)(((CComplex &)((*sp)[n])).im)));
				else
					da.Add(CPoint((int)((*sp)[n].GetAt("x")), (int)((*sp)[n].GetAt("y"))));
			}
			int max = (int)da.GetSize();
			if ( max < 3 ) {
				m_pDoc->m_TextRam.TekLine(st, da[0].x, da[0].y, da[1].x, da[1].y);
				break;
			}

			if ( da[0] == da[max - 1] )
				closed = TRUE;
			for ( n = 0 ; n < (max - 1) ; n++ ) {
				double a, b, q, r, k = 0.5522847 * 0.5;

				if ( n == 0 ) {
					po.Add(da[n]);
					if ( closed ) {
						a = (double)(da[n+1].x - da[max-2].x);
						b = (double)(da[n+1].y - da[max-2].y);
					} else {
						a = 0.0;
						b = 0.0;
					}
				} else {
					a = (double)(da[n+1].x - da[n-1].x);
					b = (double)(da[n+1].y - da[n-1].y);
				}


				q = atan2(b, a);
				r = sqrt(a * a + b * b);
				tmp.x = da[n].x + (int)(r * k * cos(q));
				tmp.y = da[n].y + (int)(r * k * sin(q));
				po.Add(tmp);

				if ( (n + 2) < max ) {
					a = (double)(da[n].x - da[n+2].x);
					b = (double)(da[n].y - da[n+2].y);
				} else if ( closed ) {
					a = (double)(da[n].x - da[1].x);
					b = (double)(da[n].y - da[1].y);
				} else {
					a = 0.0;
					b = 0.0;
				}

				q = atan2(b, a);
				r = sqrt(a * a + b * b);
				tmp.x = da[n+1].x + (int)(r * k * cos(q));
				tmp.y = da[n+1].y + (int)(r * k * sin(q));
				po.Add(tmp);

				po.Add(da[n+1]);
			}

			da.RemoveAll();
			for ( n = 0 ; n < (po.GetSize() - 3) ; n += 3 ) {
				int m = (int)(sqrt(pow((double)po[n].x - (double)po[n + 3].x, 2) + pow((double)po[n].y - (double)po[n + 3].y, 2)) / 10.0);
				if ( m < 1 ) m = 1;
				for ( int c = 0 ; c <= m ; c++ ) {
					double t = (double)c / m;
					tmp.x = (int)(pow(1 - t, 3) * po[n].x + 3 * pow(1 - t, 2) * t * po[n + 1].x + 3 * (1 - t) * pow(t, 2) * po[n + 2].x + pow(t, 3) * po[n + 3].x);
					tmp.y = (int)(pow(1 - t, 3) * po[n].y + 3 * pow(1 - t, 2) * t * po[n + 1].y + 3 * (1 - t) * pow(t, 2) * po[n + 2].y + pow(t, 3) * po[n + 3].y);
					da.Add(tmp);
				}
			}

			for ( n = 1 ; n < da.GetSize() ; n++ )
				m_pDoc->m_TextRam.TekLine(st, da[n - 1].x, da[n - 1].y, da[n].x, da[n].y);

			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 11:		// tekspline(xy, color, style, size, flush)
		{
			CPoint tmp[4], tp;
			BOOL closed = FALSE;
			CArray<CPoint, CPoint &> da, po;
			CScriptValue *sp = (CScriptValue *)local[0];
			st = TekStyle(1, local);
			if ( sp->GetSize() < 2 )
				break;
			for ( n = 0 ; n < sp->GetSize() ; n++ ) {
				if ( (*sp)[n].GetType() == VALTYPE_COMPLEX )
					da.Add(CPoint((int)(((CComplex &)((*sp)[n])).re), (int)(((CComplex &)((*sp)[n])).im)));
				else
					da.Add(CPoint((int)((*sp)[n].GetAt("x")), (int)((*sp)[n].GetAt("y"))));
			}
			int max = (int)da.GetSize();
			if ( da[0] == da[max - 1] ) {
				max -= 1;
				closed = TRUE;
			}
			if ( max < 3 ) {
				m_pDoc->m_TextRam.TekLine(st, da[0].x, da[0].y, da[1].x, da[1].y);
				break;
			}
			int len = (closed ? max : (max - 1));
			for ( n = 0 ; n < len ; n++ ) {
				int a, i, r = 0;
				double d;
				double td[4], tx[4], ty[4];
				double dx[GAUSS_MAX];
				double dy[GAUSS_MAX];

				if ( n > 0 )
					tmp[r++] = da[n - 1];
				else if ( closed )
					tmp[r++] = da[max - 1];
				a = r;
				for ( i = n ; r < 4 && i < max ; )
					tmp[r++] = da[i++];
				if ( r < 4 && closed )
					tmp[r++] = da[0];
				if ( r < 4 && closed )
					tmp[r++] = da[1];

				if ( r < 2 ) {
					for ( i = 0 ; i < r ; i++ )
						po.Add(tmp[i]);
					continue;
				}

				td[0] = 0.0;
				for ( i = 1 ; i < r ; i++ )
					td[i] = td[i - 1] + sqrt(pow((double)(tmp[i].x - tmp[i - 1].x), 2) + pow((double)(tmp[i].y - tmp[i - 1].y), 2));

				for ( i = 0 ; i < r ; i++ ) {
					tx[i] = (double)(tmp[i].x);
					ty[i] = (double)(tmp[i].y);
				}

				spline_init(td, tx, r, dx);
				spline_init(td, ty, r, dy);

				for ( d = td[a] ; d < td[a + 1] ; d += 2.0 ) {
					tp.x = (LONG)spline_calc(d, r, dx);
					tp.y = (LONG)spline_calc(d, r, dy);
					po.Add(tp);
				}

				/*
				for ( d = td[a] ; d < td[a + 1] ; d += 2.0 ) {
					tp.x = lagrange_calc(d, td, tx, r);
					tp.y = lagrange_calc(d, td, ty, r);
					po.Add(tp);
				}
				*/
			}

			for ( n = 1 ; n < po.GetSize() ; n++ )
				m_pDoc->m_TextRam.TekLine(st, po[n - 1].x, po[n - 1].y, po[n].x, po[n].y);

			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 12:		// teklesq(xy, n, color, style, size, flush)
		{
			CArray<double, double> x, y;
			double xx[GAUSS_MAX];
			CArray<CPoint, CPoint &> po;
			CScriptValue *sp = (CScriptValue *)local[0];
			int nn = (int)local[1];
			st = TekStyle(2, local);
			if ( sp->GetSize() < 2 || nn < 1 || nn > GAUSS_MAX )
				break;
			for ( n = 0 ; n < sp->GetSize() ; n++ ) {
				if ( (*sp)[n].GetType() == VALTYPE_COMPLEX ) {
					x.Add(((CComplex &)((*sp)[n])).re);
					y.Add(((CComplex &)((*sp)[n])).im);
				} else {
					x.Add((double)(*sp)[n].GetAt("x"));
					y.Add((double)(*sp)[n].GetAt("y"));
				}
			}
			sai2pow(x.GetData(), y.GetData(), (int)x.GetSize(), nn, xx);

			double nx, mx;
			nx = mx = x[0];
			for ( n = 1 ; n < x.GetSize() ; n++ ) {
				if ( nx > x[n] )
					nx = x[n];
				if ( mx < x[n] )
					mx = x[n];
			}

			int m = (int)((mx - nx) / 10.0);
			for ( n = 0 ; n <= m ; n++ ) {
				double x = nx + n * (mx - nx) / m;
				double y = sai2y(x, nn + 1, xx);
				po.Add(CPoint((int)x, (int)y));
			}

			for ( n = 1 ; n < po.GetSize() ; n++ )
				m_pDoc->m_TextRam.TekLine(st, po[n - 1].x, po[n - 1].y, po[n].x, po[n].y);

			if ( (int)m_Data["tek"]["flush"] == 0 )
				m_pDoc->m_TextRam.TekFlush();
		}
		break;
	case 13:		// tekflush(f)
		m_Data["tek"]["flush"] = (int)local[0];
		if ( (int)m_Data["tek"]["flush"] == 0 )
			m_pDoc->m_TextRam.TekFlush();
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// Message Dlg Func...

int CScript::Func10(int cmd, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	switch(cmd) {
	case 0:		// msgdlg(m)
		AfxMessageBox((LPCTSTR)local[0]);
		break;
	case 1:		// yesnodlg(m)
		if ( AfxMessageBox((LPCTSTR)local[0], MB_ICONQUESTION | MB_YESNO) == IDYES )
			(*acc) = (int)1;
		else
			(*acc) = (int)0;
		break;
	case 2:		// inputdlg(ttl, msg, edit);
		{
			CEditDlg dlg;
			dlg.m_WinText = (LPCTSTR)local[0];
			dlg.m_Title   = (LPCTSTR)local[1];
			dlg.m_Edit    = (LPCTSTR)local[2];
			if ( dlg.DoModal() == IDOK )
				(*acc) = (LPCTSTR)dlg.m_Edit;
			else
				(*acc) = (LPCSTR)"";
		}
		break;
	case 3:		// filedlg(rw, ext, file, filter);
		{
			if ( local.GetSize() <= 3 )
				local[3] = CStringLoad(IDS_FILEDLGALLFILE);
			CFileDialog dlg((int)local[0], (LPCTSTR)local[1], (LPCTSTR)local[2], OFN_HIDEREADONLY, (LPCTSTR)local[3], ::AfxGetMainWnd());
			if ( DpiAwareDoModal(dlg) == IDOK )
				(*acc) = (LPCTSTR)dlg.GetPathName();
			else
				(*acc) = (LPCSTR)"";
		}
		break;

	case 4:		// command(arg...)
		{
			static BOOL bNest = FALSE;
			CCommandLineInfoEx cmds;
			(*acc) = (int)1;
			if ( bNest ) break;
			bNest = TRUE;
			for ( int n = 0 ; n < local.GetSize() ; n++ ) {
				LPCTSTR pParam = (LPCTSTR)local[n];
				BOOL bFlag = FALSE;
				BOOL bLast = ((n + 1) == local.GetSize());
				if ( pParam[0] == _T('-') || pParam[0] == _T('/') ) {
					bFlag = TRUE;
					pParam++;
				}
				cmds.ParseParam(pParam, bFlag, bLast);
			}
			((CRLoginApp *)::AfxGetApp())->OpenProcsCmd(&cmds);
			bNest = FALSE;
			(*acc) = (int)0;
		}
		break;
	case 5:		// shellexec(path, param, dir)
		(*acc) = (int)(INT_PTR)ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), NULL, (LPCTSTR)local[0], (LPCTSTR)local[1], (LPCTSTR)local[2], SW_NORMAL);
		break;

	case 6:		// passdlg(host, port, user, pass, prompt, title, sec)
		{
			CPassDlg dlg;

			if ( local.GetSize() > 0 )	dlg.m_HostAddr = (LPCTSTR)local[0];
			if ( local.GetSize() > 1 )	dlg.m_PortName = (LPCTSTR)local[1];
			if ( local.GetSize() > 2 )	dlg.m_UserName = (LPCTSTR)local[2];
			if ( local.GetSize() > 3 )	dlg.m_PassName = (LPCTSTR)local[3];
			if ( local.GetSize() > 4 )	dlg.m_Prompt   = (LPCTSTR)local[4];
			if ( local.GetSize() > 5 )	dlg.m_Title    = (LPCTSTR)local[5];
			if ( local.GetSize() > 6 )	dlg.m_MaxTime  = (int)local[6];

			if ( dlg.DoModal() != IDOK ) {
				(*acc) = (int)0;
				break;
			}
			(*acc)[0] = (LPCTSTR)dlg.m_HostAddr;
			(*acc)[1] = (LPCTSTR)dlg.m_PortName;
			(*acc)[2] = (LPCTSTR)dlg.m_UserName;
			(*acc)[3] = (LPCTSTR)dlg.m_PassName;
			(*acc) = (int)1;
		}
		break;

	case 7:		// dump(s)
		{
			int n;
			CStringA str, tmp;
			CBuffer *bp;
			switch(local[0].GetType()) {
			case VALTYPE_STRING:
				{
					bp = local[0].GetBuf();
					LPBYTE p = (LPBYTE)bp->GetPtr();
					for ( n = 0 ; n < bp->GetSize() ; n++ ) {
						if ( n > 0 && (n % 16) == 0 )
							str += "\r\n";
						tmp.Format("%02X ", *(p++));
						str += tmp;
					}
					str += "\r\n";
				}
				(*acc) = (LPCSTR)str;
				break;
			case VALTYPE_WSTRING:
				{
					bp = local[0].GetWBuf();
					LPWORD p = (LPWORD)bp->GetPtr();
					for ( n = 0 ; n < bp->GetSize() ; n += sizeof(WORD) ) {
						if ( n > 0 && (n % 16) == 0 )
							str += "\r\n";
						tmp.Format("%04X ", *(p++));
						str += tmp;
					}
					str += "\r\n";
				}
				(*acc) = (LPCSTR)str;
				break;
			case VALTYPE_DSTRING:
				{
					bp = local[0].GetDBuf();
					LPDWORD p = (LPDWORD)bp->GetPtr();
					for ( n = 0 ; n < bp->GetSize() ; n += sizeof(DWORD) ) {
						if ( n > 0 && (n % 16) == 0 )
							str += "\r\n";
						tmp.Format("%08X ", *(p++));
						str += tmp;
					}
					str += "\r\n";
				}
				(*acc) = (LPCSTR)str;
				break;

			default:
				(*acc) = (LPCSTR)"no string";
				break;
			}
		}
		break;

	case 8:		// menu(id, str, func, pos)
		{
			CStringA mbs;
			int id = (int)local[0];
			if ( id < 0 || id >= 10 )
				break;
			m_MenuTab[id].str  = (LPCTSTR)local[1];
			if ( !m_MenuTab[id].str.IsEmpty() ) {
				m_MenuTab[id].func = (LPCTSTR)local[2];
				m_MenuTab[id].pos  = CKeyNodeTab::GetCmdsKey((LPCWSTR)local[3]);
			}
//			SetMenu(AfxGetMainWnd()->GetMenu());
		}
		break;
	case 9:		// iconstyle(str)
		((CMainFrame *)AfxGetMainWnd())->SetIconStyle();
		if ( local.GetSize() > 0 && ((CMainFrame *)AfxGetMainWnd())->m_IconShow )
			((CMainFrame *)AfxGetMainWnd())->SetIconData(NULL, (LPCTSTR)local[0]);
		break;

	case 10:	// getproint(pro, def)
		(*acc) = (int)AfxGetApp()->GetProfileInt(_T("ScriptUser"), (LPCTSTR)local[0], (int)local[1]);
		break;
	case 11:	// putproint(pro, val)
		AfxGetApp()->WriteProfileInt(_T("ScriptUser"), (LPCTSTR)local[0], (int)local[1]);
		break;
	case 12:	// getprostr(pro, def)
		(*acc) = (LPCTSTR)AfxGetApp()->GetProfileString(_T("ScriptUser"), (LPCTSTR)local[0], (LPCTSTR)local[1]);
		break;
	case 13:	// putprostr(pro, str)
		AfxGetApp()->WriteProfileString(_T("ScriptUser"), (LPCTSTR)local[0], (LPCTSTR)local[1]);
		break;

	case 14:	// menucall(menu)
		{
			int cmd = CKeyNodeTab::GetCmdsKey((LPCWSTR)local[0]);
			if ( cmd > 0 )
				AfxGetMainWnd()->SendMessage(WM_COMMAND, (WPARAM)cmd);
		}
		break;
	case 15:	// menucheck(menu)
		{
			CScriptCmdUI cmdui;
			cmdui.m_nID = CKeyNodeTab::GetCmdsKey((LPCWSTR)local[0]);
			if ( cmdui.m_nID > 0 ) {
				cmdui.DoUpdate(::AfxGetMainWnd(), FALSE);
				(*acc) = cmdui.m_Check;
			} else
				(*acc) = (-1);
		}
		break;
	case 16:	// menuenable(menu)
		{
			CScriptCmdUI cmdui;
			cmdui.m_nID = CKeyNodeTab::GetCmdsKey((LPCWSTR)local[0]);
			if ( cmdui.m_nID > 0 ) {
				cmdui.DoUpdate(::AfxGetMainWnd(), FALSE);
				(*acc) = (int)cmdui.m_bEnable;
			} else
				(*acc) = (-1);
		}
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// Socket Func

int CScript::Func09(int cmd, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	if ( m_pDoc == NULL )
		return FUNC_RET_EXIT;

	switch(cmd) {
	case 0:		// sgetc(ms)
		ResetEvent(SCP_EVENT_SOCK | SCP_EVENT_TIMER);
		if ( m_SockMode != DATA_BUF_NONE ) {
			int ms = (int)local[0];
			if ( m_SockBuff.GetSize() > 0 ) {
				(*acc) = (int)(m_SockBuff.Get8Bit());
			} else if ( !IsConnect() || (m_EventMode & SCP_EVENT_TIMER) != 0 || ms < 0 ) {
				(*acc) = (int)(-1);
			} else if ( ms > 0 ) {
				((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(ms, TIMEREVENT_SCRIPT, this);
					SetEvent(SCP_EVENT_TIMER);
				return SetEvent(SCP_EVENT_SOCK | SCP_EVENT_TIMER);
			} else
				return SetEvent(SCP_EVENT_SOCK);
		} else {
			(*acc) = (int)(-1);
			return FUNC_RET_ABORT;
		}
		break;
	case 1:		// sgets(ms)
		ResetEvent(SCP_EVENT_SOCK | SCP_EVENT_TIMER);
		if ( m_SockMode != DATA_BUF_NONE ) {
			int ms = (int)local[0];
			int n = 0, ch = 0;
			int len = m_SockBuff.GetSize();
			LPBYTE p = m_SockBuff.GetPtr();
			while ( n < len ) {
				if ( (ch = p[n++]) == '\n' )
					break;
			}
			if ( ch == '\n' || (len > 0 && !IsConnect()) ) {
				CBuffer *bp = (*acc).GetBuf();
				bp->Clear();
				bp->Apend(p, n);
				m_SockBuff.Consume(n);
			} else if ( !IsConnect() || (m_EventMode & SCP_EVENT_TIMER) != 0 || ms < 0 ) {
				(*acc) = (LPCSTR)"";
			} else if ( ms > 0 ) {
				((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(ms, TIMEREVENT_SCRIPT, this);
				return SetEvent(SCP_EVENT_SOCK | SCP_EVENT_TIMER);
			} else
				return SetEvent(SCP_EVENT_SOCK);
		} else {
			(*acc) = (LPCSTR)"";
			return FUNC_RET_ABORT;
		}
		break;
	case 2:		// sread(n, ms)
		ResetEvent(SCP_EVENT_SOCK | SCP_EVENT_TIMER);
		if ( m_SockMode != DATA_BUF_NONE ) {
			int ms = (int)local[1];
			int n = (int)local[0];
			int len = m_SockBuff.GetSize();
			CBuffer *bp = (*acc).GetBuf();
			if ( n <= len ) {
				bp->Clear();
				bp->Apend(m_SockBuff.GetPtr(), n);
				m_SockBuff.Consume(n);
			} else if ( !IsConnect() || (m_EventMode & SCP_EVENT_TIMER) != 0 || ms < 0 ) {
				if ( len > 0 ) {
					if ( n > len )
						n = len;
					bp->Clear();
					bp->Apend(m_SockBuff.GetPtr(), n);
					m_SockBuff.Consume(n);
				} else
					(*acc) = (LPCSTR)"";
			} else if ( ms > 0 ) {
				((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(ms, TIMEREVENT_SCRIPT, this);
				return SetEvent(SCP_EVENT_SOCK | SCP_EVENT_TIMER);
			} else
				return SetEvent(SCP_EVENT_SOCK);
		} else {
			(*acc) = (LPCSTR)"";
			return FUNC_RET_ABORT;
		}
		break;

	case 3:		// sputc(s);
		if ( m_pDoc->m_pSock != NULL ) {
			CStringD str;
			str = (DCHAR)(int)local[0];
			LPCSTR mbs = m_pDoc->RemoteStr(str);
			m_pDoc->m_pSock->Send((LPCSTR)mbs, (int)strlen(mbs));
		}
		break;
	case 4:		// sputs(s);
		if ( m_pDoc->m_pSock != NULL ) {
			LPCSTR mbs = m_pDoc->RemoteStr((LPCWSTR)local[0]);
			m_pDoc->m_pSock->Send((LPCSTR)mbs, (int)strlen(mbs));
		}
		break;
	case 5:		// swrite(s);
		if ( m_pDoc->m_pSock != NULL ) {
			CBuffer *bp = local[0].GetBuf();
			m_pDoc->m_pSock->Send(bp->GetPtr(), bp->GetSize());
		}
		break;

	case 6:		// sopen(n)
		m_SockBuff.Clear();
		m_SockMode = (int)local[0] + DATA_BUF_BOTH;
		break;
	case 7:		// sclose()
		m_SockMode = DATA_BUF_NONE;
		m_SockBuff.Clear();
		break;

	case 8:		// remotestr(s)
		(*acc) = (LPCSTR)m_pDoc->RemoteStr((LPCTSTR)local[0]);
		break;
	case 9:		// localstr(s)
		(*acc) = (LPCTSTR)m_pDoc->LocalStr((LPCSTR)local[0]);
		break;

	case 10:	// wait(f)
		if ( m_EventMode != 0 ) {
			if ( (m_EventMode & (SCP_EVENT_CONNECT | SCP_EVENT_CLOSE)) != 0 )
				break;
			else if ( (m_EventFlag & SCP_EVENT_CONNECT) != 0 )
				return SetEvent(SCP_EVENT_CONNECT);
			else if ( (m_EventFlag & SCP_EVENT_CLOSE) != 0 )
				return SetEvent(SCP_EVENT_CLOSE);
		} else if ( (int)local[0] == 0 ) {
			if ( !IsConnect() )
				return SetEvent(SCP_EVENT_CONNECT);
		} else {
			if ( IsConnect() )
				return SetEvent(SCP_EVENT_CLOSE);
		}
		break;

	case 11:		// sendstr(str)
		{
			CBuffer tmp((LPCWSTR)local[0]);
			m_pDoc->SendBuffer(tmp, FALSE);
		}
		break;
	case 12:		// broadcast(str, group)
		{
			CBuffer tmp((LPCWSTR)local[0]);
			((CRLoginApp *)::AfxGetApp())->SendBroadCast(tmp, (LPCTSTR)local[1]);
		}
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// Console Func

int CScript::Func08(int cmd, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	if ( m_pDoc == NULL )
		return FUNC_RET_EXIT;

	switch(cmd) {
	case 0:		// cgetc(ms)
		ResetEvent(SCP_EVENT_CONS | SCP_EVENT_TIMER);
		if ( m_ConsMode != DATA_BUF_NONE ) {
			int ms = (int)local[0];
			if ( m_ConsBuff.GetSize() >= (2 * sizeof(DWORD)) ) {
				(*acc)    = (int)CTextRam::UCS2toUCS4(m_ConsBuff.GetDword());
				(*acc)["pos"] = (int)m_ConsBuff.GetDword();
			} else if ( !IsConnect() || (m_EventMode & SCP_EVENT_TIMER) != 0 || ms < 0 ) {
				(*acc) = (int)(-1);
			} else if ( ms > 0 ) {
				((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(ms, TIMEREVENT_SCRIPT, this);
				return SetEvent(SCP_EVENT_CONS | SCP_EVENT_TIMER);
			} else
				return SetEvent(SCP_EVENT_CONS);
		} else {
			(*acc) = (int)(-1);
			return FUNC_RET_ABORT;
		}
		break;
	case 1:		// cgets(ms)
		ResetEvent(SCP_EVENT_CONS | SCP_EVENT_TIMER);
		if ( m_ConsMode != DATA_BUF_NONE ) {
			int ms = (int)local[0];
			int n, ch, pos = (-1);
			int len = m_ConsBuff.GetSize() / sizeof(DWORD);
			LPDWORD p = (LPDWORD)m_ConsBuff.GetPtr();
			for ( n = 0 ; n < len ; n += 2 ) {
				if ( (ch = p[n]) == '\n' ) {
					n += 2;
					break;
				}
			}
			if ( ch == '\n' || (len > 0 && !IsConnect()) ) {
				CBuffer *bp = (*acc).GetWBuf();
				bp->Clear();
				while ( n > 0 ) {
					ch = m_ConsBuff.GetDword();
					if ( (ch & 0xFFFF0000) != 0 )
						bp->PutWord(ch >> 16);
					bp->PutWord(ch);
					if ( pos == (-1) )
						pos = m_ConsBuff.GetDword();
					else
						m_ConsBuff.Consume(sizeof(DWORD));
					n -= 2;
				}
				(*acc)["pos"] = (int)pos;
			} else if ( !IsConnect() || (m_EventMode & SCP_EVENT_TIMER) != 0 || ms < 0 ) {
				(*acc) = (LPCSTR)"";
			} else if ( ms > 0 ) {
				((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(ms, TIMEREVENT_SCRIPT, this);
				return SetEvent(SCP_EVENT_CONS | SCP_EVENT_TIMER);
			} else
				return SetEvent(SCP_EVENT_CONS);
		} else {
			(*acc) = (LPCSTR)"";
			return FUNC_RET_ABORT;
		}
		break;
	case 2:		// cread(n)
		break;

	case 3:		// cputc(c)
		{
			CStringD str;
			str = (DCHAR)(int)local[0];
			LPCSTR mbs = m_pDoc->RemoteStr(str);
			PutConsOut((LPBYTE)mbs, (int)strlen(mbs));
		}
		break;
	case 4:		// cputs(s)
		{
			LPCSTR mbs = m_pDoc->RemoteStr((LPCTSTR)local[0]);
			PutConsOut((LPBYTE)mbs, (int)strlen(mbs));
		}
		break;
	case 5:		// cwrite(s)
		{
			CBuffer *bp = local[0].GetBuf();
			PutConsOut((LPBYTE)bp->GetPtr(), bp->GetSize());
		}
		break;

	case 6:		// copen()
		m_ConsBuff.Clear();
		m_ConsMode = DATA_BUF_BOTH;
		break;
	case 7:		// cclose()
		m_ConsMode = DATA_BUF_NONE;
		m_ConsBuff.Clear();
		break;

	case 8:		// cwait(t, s...)
		ResetEvent(SCP_EVENT_CONS | SCP_EVENT_TIMER);
		if ( m_EventMode != 0 ) {
			if ( m_RegMatch >= 0 || (m_EventMode & SCP_EVENT_TIMER) != 0 || !IsConnect() ) {
				((CMainFrame *)AfxGetMainWnd())->DelTimerEvent(this);
				(*acc) = (int)m_RegMatch;
				m_RegMatch = 0;
				for ( int n = 0 ; n < m_RegData.GetSize() ; n++ )
					delete (CRegEx *)m_RegData[n];
				m_RegData.RemoveAll();
			} else
				return SetEvent(SCP_EVENT_CONS);
		} else if ( !IsConnect() ) {
			(*acc) = (int)(-1);
		} else {
			int n;
			for ( n = 0 ; n < m_RegData.GetSize() ; n++ )
				delete (CRegEx *)m_RegData[n];
			m_RegData.RemoveAll();
			for ( n = 1 ; n < local.GetSize() ; n++ ) {
				CRegEx *pReg = new CRegEx;
				if ( !pReg->Compile((LPCTSTR)local[n]) ) {
					delete pReg;
					continue;
				}
				pReg->MatchCharInit();
				m_RegData.Add(pReg);
			}
			if ( m_RegData.GetSize() > 0 ) {
				m_RegMatch = (-1);
				if ( (n = (int)local[0]) > 0 ) {
					((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(n * 1000, TIMEREVENT_SCRIPT, this);
					return SetEvent(SCP_EVENT_TIMER | SCP_EVENT_CONS);
				} else
					return SetEvent(SCP_EVENT_CONS);
			}
		}
		break;

	case 9:		// getchar(pos)
		{
			int pos = (int)local[0];
			int x = pos % m_pDoc->m_TextRam.m_ColsMax;
			int y = pos / m_pDoc->m_TextRam.m_ColsMax;
			if ( x < 0 || x >= m_pDoc->m_TextRam.m_Cols || y < 0 || y >= m_pDoc->m_TextRam.m_HisMax ) {
				(*acc) = (int)(-1);
				break;
			}

			CCharCell *vp = m_pDoc->m_TextRam.GETVRAM(x, y - m_pDoc->m_TextRam.m_HisPos);

			(*acc)       = (int)CTextRam::UCS2toUCS4((DWORD)(*vp));
			(*acc)["fc"] = (int)vp->m_Vram.fcol;
			(*acc)["bc"] = (int)vp->m_Vram.bcol;
			(*acc)["at"] = (int)vp->m_Vram.attr;
			(*acc)["ft"] = (int)vp->m_Vram.font;
		}
		break;
	case 10:	// setchar(ch, pos)
		{
			int pos = (int)local[1];
			int x = pos % m_pDoc->m_TextRam.m_ColsMax;
			int y = pos / m_pDoc->m_TextRam.m_ColsMax;
			if ( x < 0 || x >= m_pDoc->m_TextRam.m_Cols || y < 0 || y >= m_pDoc->m_TextRam.m_HisMax ) {
				(*acc) = (int)(-1);
				break;
			}

			CCharCell *vp = m_pDoc->m_TextRam.GETVRAM(x, y - m_pDoc->m_TextRam.m_HisPos);

			*vp = CTextRam::UCS4toUCS2((int)local[0]);
			vp->m_Vram.fcol = (int)local[0].GetAt("fc");
			vp->m_Vram.bcol = (int)local[0].GetAt("bc");
			vp->m_Vram.attr = (int)local[0].GetAt("at");
			vp->m_Vram.font = (int)local[0].GetAt("ft");

			y -= m_pDoc->m_TextRam.m_HisPos;
			while ( y < 0 )
				y += m_pDoc->m_TextRam.m_HisMax;
			while ( y >= m_pDoc->m_TextRam.m_HisMax )
				y -= m_pDoc->m_TextRam.m_HisMax;

			if ( y < m_pDoc->m_TextRam.m_Lines ) {
				m_pDoc->m_TextRam.DISPVRAM(x, y, 1, 1);
				m_pDoc->m_TextRam.FLUSH();
			}
		}
		break;

	case 11:		// locate(x, y)
		m_pDoc->m_TextRam.LOCATE((int)local[0], (int)local[1]);
		m_pDoc->m_TextRam.FLUSH();
		// no break;
	case 12:		// getpos()
		{
			int pos = m_pDoc->m_TextRam.m_CurY + m_pDoc->m_TextRam.m_HisPos;
			while ( pos < 0 )
				pos += m_pDoc->m_TextRam.m_HisMax;
			while ( pos >= m_pDoc->m_TextRam.m_HisMax )
				pos -= m_pDoc->m_TextRam.m_HisMax;
			(*acc) = (m_pDoc->m_TextRam.m_CurX + pos * m_pDoc->m_TextRam.m_ColsMax);
		}
		break;
	case 13:		// xypos(x, y)
		{
			int x = (int)local[0];
			int y = (int)local[1];
			if ( x < 0 ) x = 0;
			else if ( x >= m_pDoc->m_TextRam.m_Cols )
				x = m_pDoc->m_TextRam.m_Cols - 1;
			if ( y < 0 ) y = 0;
			else if ( y >= m_pDoc->m_TextRam.m_Lines )
				y = m_pDoc->m_TextRam.m_Lines - 1;
			int pos = y + m_pDoc->m_TextRam.m_HisPos;
			while ( pos < 0 )
				pos += m_pDoc->m_TextRam.m_HisMax;
			while ( pos >= m_pDoc->m_TextRam.m_HisMax )
				pos -= m_pDoc->m_TextRam.m_HisMax;
			(*acc) = (x + pos * m_pDoc->m_TextRam.m_ColsMax);
		}
		break;
	case 14:		// getstr(spos, epos)
		{
			int sx = (int)local[0] % m_pDoc->m_TextRam.m_ColsMax;
			int sy = (int)local[0] / m_pDoc->m_TextRam.m_ColsMax - m_pDoc->m_TextRam.m_HisPos;
			int ex = (int)local[1] % m_pDoc->m_TextRam.m_ColsMax;
			int ey = (int)local[1] / m_pDoc->m_TextRam.m_ColsMax - m_pDoc->m_TextRam.m_HisPos;
			CBuffer *bp = acc->GetWBuf();
			m_pDoc->m_TextRam.GetVram(sx, ex, sy, ey, bp);
		}
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// RegEx Func

int CScript::Func07(int cmd, CScriptValue &local)
{
	int n;
	CRegEx *rp;
	CRegExRes res;
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	switch(cmd) {
	case 0:		// regopen(r)
		rp = new CRegEx;
		rp->Compile((LPCTSTR)local[0]);
		rp->MatchCharInit();
		(*acc) = (void *)rp;
		break;
	case 1:		// regclose(a)
		if ( local[0].GetType() != VALTYPE_PTR )
			break;
		if ( (rp = (CRegEx *)(void *)local[0]) == NULL )
			break;
		if ( local[0].m_Type == VALTYPE_IDENT )
			*((CScriptValue *)local[0]) = (int)0;
		delete rp;
		break;
	case 2:		// regchar(c, a)
		if ( local[1].GetType() != VALTYPE_PTR )
			break;
		if ( (rp = (CRegEx *)(void *)local[1]) == NULL )
			break;
		if ( rp->MatchChar((int)local[0], (int)(local[0].GetAt(0)), &res) ) {
			(*acc) = (int)res.m_Data.GetSize();
			acc->RemoveAll();
			for ( n = 0 ; n < res.GetSize() ; n++ ) {
				(*acc)[n] = (LPCTSTR)(res.m_Data[n].m_Str);
				(*acc)[n]["pos"] = (int)res.m_Idx[n];
			}
		} else {
			(*acc) = (int)0;
			(*acc)[0] = (int)res.m_Status;
		}
		break;
	case 3:		// regstr(s, a)
		if ( local[1].GetType() != VALTYPE_PTR )
			break;
		if ( (rp = (CRegEx *)(void *)local[1]) == NULL )
			break;
		if ( rp->MatchStr((LPCTSTR)local[0], &res) ) {
			(*acc) = (int)res.m_Data.GetSize();
			acc->RemoveAll();
			for ( n = 0 ; n < (int)res.m_Data.GetSize() ; n++ )
				(*acc)[n] = (LPCTSTR)(res.m_Data[n].m_Str);
		} else {
			(*acc) = (int)0;
			(*acc)[0] = (int)res.m_Status;
		}
		break;

	case 4:		// cipopen(type, key, ec)
		{
			unsigned int dlen;
			const EVP_MD *evp_md = EVP_sha1();
			u_char key[EVP_MAX_MD_SIZE], iv[EVP_MAX_MD_SIZE];
			EVP_MD_CTX *md_ctx;
			CCipher *cp = new CCipher;
			CBuffer *bp = local[1].GetBuf();

			md_ctx = EVP_MD_CTX_new();
			EVP_DigestInit(md_ctx, evp_md);
			EVP_DigestUpdate(md_ctx, (LPBYTE)bp->GetPtr(), bp->GetSize());
			EVP_DigestFinal(md_ctx, key, &dlen);
			EVP_MD_CTX_free(md_ctx);

			md_ctx = EVP_MD_CTX_new();
			EVP_DigestInit(md_ctx, evp_md);
			EVP_DigestUpdate(md_ctx, (LPBYTE)bp->GetPtr(), bp->GetSize());
			EVP_DigestUpdate(md_ctx, key, 16);
			EVP_DigestFinal(md_ctx, key + 16, &dlen);
			EVP_MD_CTX_free(md_ctx);

			memset(iv, 0, sizeof(iv));

			if ( cp->Init((LPCTSTR)local[0], (int)local[2], key, (-1), iv) )
				(*acc).Empty();
			else
				(*acc) = (void *)cp;
		}
		break;
	case 5:		// cipclose(a)
		{
			CCipher *cp = (CCipher *)(void *)local[0];
			if ( cp != NULL )
				delete cp;
			if ( local[0].m_Type == VALTYPE_IDENT )
				*((CScriptValue *)local[0]) = (int)0;
		}
		break;
	case 6:		// cipstr(s, a)
		{
			CCipher *cp = (CCipher *)(void *)local[1];
			(*acc).Empty();
			(*acc).GetBuf()->Clear();
			if ( cp != NULL ) {
				CBuffer tmp, out;
				CBuffer *bp = local[0].GetBuf();

				if ( cp->m_Mode == MODE_ENC ) {
					tmp.PutBuf(bp->GetPtr(), bp->GetSize());
					while ( (tmp.GetSize() % cp->GetBlockSize()) != 0 )
						tmp.Put8Bit(0);
					cp->Cipher(tmp.GetPtr(), tmp.GetSize(), (*acc).GetBuf());

				} else if ( cp->m_Mode == MODE_DEC ) {
					try {
						tmp = *bp;
						while ( (tmp.GetSize() % cp->GetBlockSize()) != 0 )
							tmp.Put8Bit(0);
						cp->Cipher(tmp.GetPtr(), tmp.GetSize(), &out);
						bp = (*acc).GetBuf();
						bp->Clear();
						out.GetBuf(bp);
					} catch(...) {
						;
					}
				}
			}
		}
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// String Func

int CScript::Func06(int cmd, CScriptValue &local)
{
	CBuffer *bp;
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	switch(cmd) {
	case 0:	// iconv(f, t, s)
		{
			CIConv iconv;
			switch(local[2].GetType()) {
			case VALTYPE_DSTRING:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetDBuf(), acc->GetBuf());
				break;
			case VALTYPE_WSTRING:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetWBuf(), acc->GetBuf());
				break;
			case VALTYPE_STRING:
			default:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetBuf(), acc->GetBuf());
				break;
			}
		}
		break;
	case 1:	// iconvw(f, t, s)
		{
			CIConv iconv;
			switch(local[2].GetType()) {
			case VALTYPE_DSTRING:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetDBuf(), acc->GetWBuf());
				break;
			case VALTYPE_WSTRING:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetWBuf(), acc->GetWBuf());
				break;
			case VALTYPE_STRING:
			default:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetBuf(), acc->GetWBuf());
				break;
			}
		}
		break;
	case 2:	// iconvd(f, t, s)
		{
			CIConv iconv;
			switch(local[2].GetType()) {
			case VALTYPE_DSTRING:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetDBuf(), acc->GetDBuf());
				break;
			case VALTYPE_WSTRING:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetWBuf(), acc->GetDBuf());
				break;
			default:
				iconv.IConvBuf((LPCTSTR)local[0], (LPCTSTR)local[1], local[2].GetBuf(), acc->GetDBuf());
				break;
			}
		}
		break;

	case 3:	// md5(s, f)
	case 4:	// sha1(s, f)
		{
			unsigned int dlen;
			const EVP_MD *evp_md;
			u_char digest[EVP_MAX_MD_SIZE];
			EVP_MD_CTX *md_ctx;

			evp_md = (cmd == 3 ? EVP_md5() : EVP_sha1());

			bp = local[0].GetBuf();
			md_ctx = EVP_MD_CTX_new();
			EVP_DigestInit(md_ctx, evp_md);
			EVP_DigestUpdate(md_ctx, (LPBYTE)bp->GetPtr(), bp->GetSize());
			EVP_DigestFinal(md_ctx, digest, &dlen);
			EVP_MD_CTX_free(md_ctx);

			acc->m_Type = VALTYPE_TSTRING;
			acc->m_Buf.Clear();
			if ( (int)local[1] == 0 )
				acc->m_Buf.Base16Encode(digest, dlen);
			else
				acc->m_Buf.Apend((LPBYTE)digest, (int)dlen);
		}
		break;

	case 5:	// base64e(s)
		bp = local[0].GetBuf();
		acc->m_Type = VALTYPE_TSTRING;
		acc->m_Buf.Base64Encode(bp->GetPtr(), bp->GetSize());
		break;
	case 6:	// base64d(s)
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.Base64Decode((LPCTSTR)local[0]);
		break;
	case 7:	// quotede(s)
		bp = local[0].GetBuf();
		acc->m_Type = VALTYPE_TSTRING;
		acc->m_Buf.QuotedEncode(bp->GetPtr(), bp->GetSize());
		break;
	case 8:	// quotedd(s)
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.QuotedDecode((LPCTSTR)local[0]);
		break;
	case 9:	// base16e(s)
		bp = local[0].GetBuf();
		acc->m_Type = VALTYPE_TSTRING;
		acc->m_Buf.Base16Encode(bp->GetPtr(), bp->GetSize());
		break;
	case 10:	// base16d(s)
		acc->m_Type = VALTYPE_STRING;
		acc->m_Buf.Base16Decode((LPCTSTR)local[0]);
		break;

	case 11:	// crc16(s, c);
		{
			int c, v;
			c = (int)local[0] & 0xFFFF;
			v = (int)local[1] & 0x00FF;
			extern const unsigned short crc16tab[];
			(*acc) = (int)((crc16tab[((c >> 8) ^ v) & 0xFF] ^ (c << 8)) & 0xFFFF);
		}
		break;
	case 12:	// crc32(s, c);
		{
			int c, v;
			c = (int)local[0] & 0xFFFFFFFF;
			v = (int)local[1] & 0x000000FF;
			extern const unsigned long crc32tab[];
			(*acc) = (int)((crc32tab[((c >> 8) ^ v) & 0xFF] ^ ((c << 8) & 0x00FFFFFF)) & 0xFFFFFFFF);
		}
		break;
	case 13:	// escstr(s)
		{
			int n;
			CBuffer *bp = local[0].GetWBuf();
			LPCWSTR p = (LPCWSTR)(bp->GetPtr());
			int len = bp->GetSize() / sizeof(WCHAR);
			CStringW str;
			for ( n = 0 ; n < len ; n++, p++ ) {
				// \x00, \n, \r, \, ', ", \x1a 
				if ( *p == _T('\0') )
					str += _T("\\0");
				else if ( *p == _T('\n') )
					str += _T("\\n");
				else if ( *p == _T('\r') )
					str += _T("\\r");
				else if ( *p == _T('\\') )
					str += _T("\\\\");
				else if ( *p == _T('\'') )
					str += _T("\\'");
				else if ( *p == _T('"') )
					str += _T("\\\"");
				else if ( *p == _T('\x1a') )
					str += _T("\\x1a");
				else
					str += *p;
			}
			(*acc) = str;
		}
		break;
    }
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// Complex Func

int CScript::Func05(int cmd, CScriptValue &local)
{
	CComplex a, b;
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	switch(cmd) {
	case 0:	// cpow(a, b)
		{
			double r = sqrt(pow(((CComplex &)local[0]).re, 2) + pow(((CComplex &)local[0]).im, 2));
			double s = atan2(((CComplex &)local[0]).im, ((CComplex &)local[0]).re);
			a.Com(pow(r, ((CComplex &)local[1]).re) * cos(((CComplex &)local[1]).re * s),
				  pow(r, ((CComplex &)local[1]).re) * sin(((CComplex &)local[1]).re * s));
			(*acc) = a;
		}
		break;
	case 1:	// csqrt(a)
		{
			double r = sqrt(pow(((CComplex &)local[0]).re, 2) + pow(((CComplex &)local[0]).im, 2));
			double s = atan2(((CComplex &)local[0]).im, ((CComplex &)local[0]).re);
			a.Com(sqrt(r) * cos(s / 2),
				  sqrt(r) * sin(s / 2));
			(*acc) = a;
		}
		break;
	case 2:	// csin(a)
		a.Com(sin(((CComplex &)local[0]).re) * cosh(((CComplex &)local[0]).im),
			  cos(((CComplex &)local[0]).re) * sinh(((CComplex &)local[0]).im));
		(*acc) = a;
		break;
	case 3:	// ccos(a)
		a.Com(cos(((CComplex &)local[0]).re) * cosh(((CComplex &)local[0]).im),
			  0.0 - sin(((CComplex &)local[0]).re) * sinh(((CComplex &)local[0]).im));
		(*acc) = a;
		break;
	case 4:	// ctan(a)
		{
			a.Com(sin(((CComplex &)local[0]).re) * cosh(((CComplex &)local[0]).im),
						     cos(((CComplex &)local[0]).re) * sinh(((CComplex &)local[0]).im));
			b.Com(cos(((CComplex &)local[0]).re) * cosh(((CComplex &)local[0]).im),
							 0.0 - sin(((CComplex &)local[0]).re) * sinh(((CComplex &)local[0]).im));
			a /= b;
			(*acc) = a;
		}
		break;
	case 5:	// clog(a)
		{
			double r = sqrt(pow(((CComplex &)local[0]).re, 2) + pow(((CComplex &)local[0]).im, 2));
			double s = atan2(((CComplex &)local[0]).im, ((CComplex &)local[0]).re);
			a.Com(log(r), s);
			(*acc) = a;
		}
		break;
	case 6:	// cexp(a)
		a.Com(exp(((CComplex &)local[0]).re) * cos(((CComplex &)local[0]).im),
			  exp(((CComplex &)local[0]).re) * sin(((CComplex &)local[0]).im));
		(*acc) = a;
		break;

	case 7:	// cabs(a)
		(*acc) = ((CComplex &)local[0]).Abs();
		break;
	case 8:	// carg(a)
		(*acc) = ((CComplex &)local[0]).Arg();
		break;

	case 9:	// complex(r, i)
		a.Com((double)local[0], (double)local[1]);
		(*acc) = a;
		break;

	case 10: // creal(a)
		(*acc) = ((CComplex &)local[0]).re;
		break;
	case 11: // cimg(a)
		(*acc) = ((CComplex &)local[0]).im;
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// Math Func

int CScript::Func04(int cmd, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

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
			(*acc) = fabs((double)local[0]);
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
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// File Func

int CScript::Func03(int cmd, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

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
	 		if ( ((*acc) = (void *)_tfsopen((LPCTSTR)local[0], (LPCTSTR)local[1], n)) != NULL )
				acc->m_PtrType = PTRTYPE_FILE;
		} else {
	 		if ( ((*acc) = (void *)_tfopen((LPCTSTR)local[0], (LPCTSTR)local[1])) != NULL )
				acc->m_PtrType = PTRTYPE_FILE;
		}
		break;
	case 1:		// fclose(f)
		if ( local[0].GetPtrType() == PTRTYPE_FILE )
			(*acc) = (int)fclose((FILE *)(void *)local[0]);
		else if ( local[0].GetPtrType() == PTRTYPE_PIPE )
			(*acc) = (int)_pclose((FILE *)(void *)local[0]);
		else
			throw _T("fclose not fopen ptr");
		local[0].SetPtrType(PTRTYPE_NONE);
		break;
	case 2:		// fread(s, f)
		if ( local[1].GetPtrType() != PTRTYPE_NONE ) {
			int n;
			CBuffer tmp;
			n = (int)local[0];
			n = (int)fread(tmp.PutSpc(n), 1, n, (FILE *)(void *)local[1]);
			acc->m_Type = VALTYPE_STRING;
			acc->m_Buf.Clear();
			acc->m_Buf.Apend(tmp.GetPtr(), n);
		} else
			throw _T("fread not (f|p)open ptr");
		break;
	case 3:		// fwrite(b, f)
		if ( local[1].GetPtrType() != PTRTYPE_NONE ) {
			CBuffer *bp = local[0].GetBuf();
			(*acc) = (int)fwrite(bp->GetPtr(), 1, bp->GetSize(), (FILE *)(void *)local[1]);
		} else
			throw _T("fwrite not (f|p)open ptr");
		break;
	case 4:		// fgets(f)
		if ( local[0].GetPtrType() != PTRTYPE_NONE ) {
			char *p;
			char tmp[4096];
			if ( (p = fgets(tmp, 4096, (FILE *)(void *)local[0])) != NULL )
				(*acc) = (LPCSTR)p;
			else
				(*acc) = (int)0;
		} else
			throw _T("fgets not (f|p)open ptr");
		break;
	case 5:		// fputs(s, f)
		if ( local[1].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (int)fputs((LPCSTR)local[0], (FILE *)(void *)local[1]);
		else
			throw _T("fputs not (f|p)open ptr");
		break;
	case 6:		// fgetc(f)
		if ( local[0].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (int)fgetc((FILE *)(void *)local[0]);
		else
			throw _T("fgetc not (f|p)open ptr");
		break;
	case 7:		// fputc(c, f)
		if ( local[1].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (int)fputc((int)local[0], (FILE *)(void *)local[1]);
		else
			throw _T("fputc not (f|p)open ptr");
		break;
	case 8:		// feof
		if ( local[0].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (int)feof((FILE *)(void *)local[0]);
		else
			throw _T("feof not (f|p)open ptr");
		break;
	case 9:		// ferror
		if ( local[0].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (int)ferror((FILE *)(void *)local[0]);
		else
			throw _T("ferror not (f|p)open ptr");
		break;
	case 10:	// fflush
		if ( local[0].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (int)fflush((FILE *)(void *)local[0]);
		else
			throw _T("fflush not (f|p)open ptr");
		break;
	case 11:	// ftell
		if ( local[0].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (LONGLONG)_ftelli64((FILE *)(void *)local[0]);
		else
			throw _T("ftell not (f|p)open ptr");
		break;
	case 12:	// fseek(f, o, s);
		// 0 = SEEK_SET		ファイルの先頭。
		// 1 = SEEK_CUR		ファイル ポインタの現在位置。
		// 2 = SEEK_END		ファイルの終端。
		if ( local[0].GetPtrType() != PTRTYPE_NONE )
			(*acc) = (LONGLONG)_fseeki64((FILE *)(void *)local[0], (LONGLONG)local[1], (int)local[2]);
		else
			throw _T("fseek not (f|p)open ptr");
		break;

	case 13:	// file(f)
		{
			int n;
			FILE *fp;
			char tmp[4096];
			acc->RemoveAll();
			if ( (fp = _tfopen((LPCTSTR)local[0], _T("r"))) != NULL ) {
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
				(*acc) = (LPCTSTR)tmp.Mid(n + 1);
			else
				(*acc) = (LPCTSTR)tmp;
		}
		break;
	case 16:	// dirname(f)
		{
			int n;
			CString tmp;
			tmp = (LPCSTR)local[0];
			if ( (n = tmp.ReverseFind('\\')) >= 0 || (n = tmp.ReverseFind(':')) >= 0 || (n = tmp.ReverseFind('/')) >= 0 )
				(*acc) = (LPCTSTR)tmp.Left(n);
			else
				(*acc) = (LPCTSTR)"";
		}
		break;

	case 17:	// copy(s, t)
		(*acc) = (int)CopyFile((LPCTSTR)local[0], (LPCTSTR)local[1], TRUE);
		break;
	case 18:	// rename(s, t)
		(*acc) = (int)MoveFile((LPCTSTR)local[0], (LPCTSTR)local[1]);
		break;
	case 19:	// delete(s)
		(*acc) = (int)DeleteFile((LPCTSTR)local[0]);
		break;
	case 20:	// getcwd()
		{
			TCHAR tmp[_MAX_DIR];
			if ( GetCurrentDirectory(_MAX_DIR, tmp) == 0 )
				tmp[0] = '\0';
			(*acc) = (LPCTSTR)tmp;
		}
		break;
	case 21:	// chdir(d)
		(*acc) = (int)SetCurrentDirectory((LPCTSTR)local[0]);
		break;

	case 22:	// play(s)
		PlaySound((LPCTSTR)local[0], NULL, SND_ASYNC | SND_FILENAME);
		break;

	case 23:	// speak(s)
		((CMainFrame *)::AfxGetMainWnd())->Speak((LPCTSTR)local[0]);
		break;

	case 24:	// popen(c, m)
 		if ( ((*acc) = (void *)_tpopen((LPCTSTR)local[0], (LPCTSTR)local[1])) != NULL )
			acc->m_PtrType = PTRTYPE_PIPE;
		break;
	case 25:	// pclose(f)
		if ( local[0].GetPtrType() == PTRTYPE_FILE )
			(*acc) = (int)fclose((FILE *)(void *)local[0]);
		else if ( local[0].GetPtrType() == PTRTYPE_PIPE )
			(*acc) = (int)_pclose((FILE *)(void *)local[0]);
		else
			throw _T("pclose not popen ptr");
		local[0].SetPtrType(PTRTYPE_NONE);
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// StdString Func

int CScript::Func02(int cmd, CScriptValue &local)
{
	CScriptValue *acc = (CScriptValue *)local;
	acc->Empty();

	switch(cmd) {
	case 0:		// substr(s, m, n)
		{
			if ( local[0].GetType() == VALTYPE_DSTRING ) {
				CStringD tmp;
				tmp = (LPCDSTR)local[0];
				(*acc) = (LPCDSTR)tmp.Mid((int)local[1], (int)local[2]);
			} else if ( local[0].GetType() == VALTYPE_WSTRING ) {
				CStringW tmp;
				tmp = (LPCWSTR)local[0];
				(*acc) = (LPCWSTR)tmp.Mid((int)local[1], (int)local[2]);
			} else {
				CStringA tmp;
				tmp = (LPCSTR)local[0];
				(*acc) = (LPCSTR)tmp.Mid((int)local[1], (int)local[2]);
			}
		}
		break;
	case 1:		// strstr(s, t)
		{
			if ( local[0].GetType() == VALTYPE_DSTRING ) {
				CStringD tmp;
				tmp = (LPCDSTR)local[0];
				(*acc) = (int)tmp.Find((LPCDSTR)local[1]);
			} else if ( local[0].GetType() == VALTYPE_WSTRING ) {
				CStringW tmp;
				tmp = (LPCWSTR)local[0];
				(*acc) = (int)tmp.Find((LPCWSTR)local[1]);
			} else {
				CStringA tmp;
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
			reg.Compile((LPCTSTR)local[0]);
			if ( reg.MatchStr((LPCTSTR)local[1], &res) ) {
				(*acc) = (int)res.m_Data.GetSize();
				acc->RemoveAll();
				for ( n = 0 ; n < (int)res.m_Data.GetSize() ; n++ )
					(*acc)[n] = (LPCTSTR)(res.m_Data[n].m_Str);
			} else
				(*acc) = (int)0;
		}
		break;
	case 3:		// replace(r, t, s)
		{
			CString tmp;
			CRegEx reg;
			reg.Compile((LPCTSTR)local[0]);
			if ( reg.ConvertStr((LPCTSTR)local[2], (LPCTSTR)local[1], tmp) )
				(*acc) = (LPCTSTR)tmp;
		}
		break;
	case 4:	// split(r, s)
		{
			int n;
			CRegEx reg;
			CStringArray tmp;
			reg.Compile((LPCTSTR)local[0]);
			if ( reg.SplitStr((LPCTSTR)local[1], tmp) ) {
				(*acc) = (int)tmp.GetSize();
				acc->RemoveAll();
				for ( n = 0 ; n < (int)tmp.GetSize() ; n++ )
					(*acc)[n] = (LPCTSTR)tmp[n];
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

					while ( (*str >= '0' && *str <= '9') || *str == '.' || *str == '*' ) {
						if ( *str == '*' ) {
							str++;
							tmp[2].Format(_T("%d"), (int)local[n++]);
							tmp[1] += tmp[2];
							tmp[2].Empty();
						} else
							tmp[1] += *(str++);
					}

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
							tmp[1].Insert(tmp[1].GetLength() - 1, _T("I64"));
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
						tmp[2].Format(tmp[1], (LPCTSTR)local[n++]);
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
			(*acc) = (LPCTSTR)tmp[0];
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

	case 10:	// trim(s)
		{
			CStringW str = (LPCWSTR)local[0];
			str.Trim((LPCWSTR)local[1]);
			(*acc) = (LPCWSTR)str;
		}
		break;
	case 11:	// trimleft(s)
		{
			CStringW str = (LPCWSTR)local[0];
			str.TrimLeft((LPCWSTR)local[1]);
			(*acc) = (LPCWSTR)str;
		}
		break;
	case 12:	// trimright(s)
		{
			CStringW str = (LPCWSTR)local[0];
			str.TrimRight((LPCWSTR)local[1]);
			(*acc) = (LPCWSTR)str;
		}
		break;

	case 13:	// ctos(DCHAR)
		{
			CStringW str;
			DCHAR ch = (int)local[0];
			if ( (ch & 0xFFFF0000) != 0 ) {
				// 000? **** **** **xx xxxx xxxx	U+010000 - U+10FFFF 21 bit
				//           1101 10** **** ****	U+D800 - U+DBFF	上位サロゲート
				//	         1101 11xx xxxx xxxx	U+DC00 - U+DFFF	下位サロゲート
				ch -= 0x10000;
				str += (WCHAR)((ch >> 10)    | 0xD800);
				str += (WCHAR)((ch & 0x03FF) | 0xDC00);
			} else
				str += (WCHAR)(ch);
			(*acc) = (LPCWSTR)str;
		}
		break;

	case 14:	// escshell(s)
		{
			CStringW str, tmp;
			LPCWSTR p = (LPCWSTR)local[0];
			while ( *p != L'\0' ) {
				if ( wcschr(L" \"$@&'()^|[]{};*?<>`\\", *p) != NULL ) {
					str += L'\\';
					str += *(p++);
				} else if ( *p == L'\t' ) {
					str += L"\\t";
					p++;
				} else if ( *p == L'\r' ) {
					str += L"\\r";
					p++;
				} else if ( *p == L'\n' ) {
					str += L"\\n";
					p++;
				} else if ( *p < L' ' ) {
					tmp.Format(L"\\%03o", *p);
					str += tmp;
					p++;
				} else {
					str += *(p++);
				}
			}
			(*acc) = (LPCWSTR)str;
		}
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////
// StdLib Func

int CScript::Func01(int cmd, CScriptValue &local)
{
	CScriptValue *sp;
	CScriptValue *acc = (CScriptValue *)local;
	CStringIndex index;
	acc->Empty();

	switch(cmd) {
	case 0:		// echo
#ifdef _DEBUGXX
		for ( int n = 0 ; n < (int)local.GetSize() ; n++ )
			TRACE("%s", (LPCSTR)local[n]);
#else
		if ( m_pDoc != NULL ) {
			for ( int n = 0 ; n < (int)local.GetSize() ; n++ ) {
				CStringA str = m_pDoc->RemoteStr(local[n]);
				str.Replace("\n", "\r\n");
				PutConsOut((LPBYTE)(LPCSTR)str, str.GetLength());
			}
		}
#endif
		break;
	case 1:		// include(f)
		ExecFile((LPCTSTR)local[0]);
		break;
	case 2:		// exit()
		return FUNC_RET_EXIT;

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
		else if ( sp->m_Type == VALTYPE_DSTRING )
			(*acc) = (int)(sp->m_Buf.GetSize() / sizeof(DCHAR));
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
		sp->Empty();
		(*acc) = sp;
		break;

	case 15:	// sleep(ms)
		if ( m_EventMode != 0 ) {
			if ( (m_EventMode & SCP_EVENT_TIMER) == 0 )
				return SetEvent(SCP_EVENT_TIMER);
		} else if ( (int)local[0] > 0 ) {
			((CMainFrame *)AfxGetMainWnd())->SetTimerEvent((int)local[0], TIMEREVENT_SCRIPT, this);
			return SetEvent(SCP_EVENT_TIMER);
		}
		break;

	case 16:	// debug(s)
		{
			CString str;
			local[0].Debug(str);
			(*acc) = (LPCTSTR)str;
		}
		break;

	case 17:	// dstr(s)
		(*acc) = (LPCDSTR)local[0];
		break;
	case 18:	// beep()
		if ( m_pDoc != NULL )
			m_pDoc->m_TextRam.BEEP();
		break;

	case 19:	// pack(s)
		{
			CBuffer tmp, base;
			local[0].Serialize(TRUE, &tmp);
			base.Base64Encode(tmp.GetPtr(), tmp.GetSize());
			(*acc) = (LPCTSTR)base;
		}
		break;
	case 20:	// unpack()
		{
			CBuffer tmp;
			tmp.Base64Decode((LPCTSTR)local[0]);
			(*acc).Serialize(FALSE, &tmp);
		}
		break;

	case 21:	// execstr(s)
		ExecStr((LPCTSTR)local[0]);
		break;

	case 22:	// inport(index)	ScriptValue -> Document
		if ( m_pDoc != NULL ) {
			local[0].SetIndex(index);
			m_pDoc->SetIndex(FALSE, index);
		}
		break;
	case 23:	// export()			ScriptValue <- Document
		if ( m_pDoc != NULL ) {
			m_pDoc->SetIndex(TRUE, index);
			(*acc).GetIndex(index);
		} else
			(*acc).Empty();
		break;
	}
	return FUNC_RET_NOMAL;
}

//////////////////////////////////////////////////////////////////////

void CScript::FuncInit()
{
	static const ScriptFunc funcs[] = {
		{ "echo",		0,	&CScript::Func01 },
		{ "include",	1,	&CScript::Func01 },	{ "exit",		2,	&CScript::Func01 },
		{ "int",		3,	&CScript::Func01 },	{ "int64",		4,	&CScript::Func01 },	
		{ "str",		5,	&CScript::Func01 },	{ "wstr",		6,	&CScript::Func01 },
		{ "length",		7,	&CScript::Func01 },	{ "sort",		8,	&CScript::Func01 },
		{ "rsort",		9,	&CScript::Func01 },	{ "ksort",		10,	&CScript::Func01 },
		{ "krsort",		11,	&CScript::Func01 },	{ "count",		12,	&CScript::Func01 },
		{ "key",		13,	&CScript::Func01 },	{ "reset",		14, &CScript::Func01 },
		{ "sleep",		15,	&CScript::Func01 },	{ "debug",		16, &CScript::Func01 },
		{ "dstr",		17, &CScript::Func01 },	{ "beep",		18,	&CScript::Func01 },
		{ "pack",		19,	&CScript::Func01 },	{ "unpack",		20,	&CScript::Func01 },
		{ "execstr",	21,	&CScript::Func01 },	{ "inport",		22,	&CScript::Func01 },
		{ "export",		23,	&CScript::Func01 },

		{ "substr",		0,	&CScript::Func02 },	{ "strstr",		1,	&CScript::Func02 },
		{ "ereg",		2,	&CScript::Func02 },	{ "replace",	3,	&CScript::Func02 },
		{ "split",		4,	&CScript::Func02 },	{ "sprintf",	5,	&CScript::Func02 },
		{ "time",		6,	&CScript::Func02 },	{ "strftime",	7,	&CScript::Func02 },
		{ "getdate",	8,	&CScript::Func02 },	{ "mktime",		9,	&CScript::Func02 },
		{ "trim",		10,	&CScript::Func02 },	{ "trimleft",	11,	&CScript::Func02 },
		{ "trimright",	12,	&CScript::Func02 },	{ "ctos",		13,	&CScript::Func02 },
		{ "escshell",	14,	&CScript::Func02 },

		{ "fopen",		0,	&CScript::Func03 },	{ "fclose",		1,	&CScript::Func03 },
		{ "fread",		2,	&CScript::Func03 },	{ "fwrite",		3,	&CScript::Func03 },
		{ "fgets",		4,	&CScript::Func03 },	{ "fputs",		5,	&CScript::Func03 },
		{ "fgetc",		6,	&CScript::Func03 },	{ "fputc",		7,	&CScript::Func03 },
		{ "feof",		8,	&CScript::Func03 },	{ "ferror",		9,	&CScript::Func03 },
		{ "fflush",		10,	&CScript::Func03 },	{ "ftell",		11,	&CScript::Func03 },
		{ "fseek",		12,	&CScript::Func03 },	{ "file",		13,	&CScript::Func03 },
		{ "stat",		14,	&CScript::Func03 },	{ "basename",	15,	&CScript::Func03 },
		{ "dirname",	16,	&CScript::Func03 },	{ "copy",		17,	&CScript::Func03 },
		{ "rename",		18,	&CScript::Func03 },	{ "delete",		19,	&CScript::Func03 },
		{ "getcwd",		20,	&CScript::Func03 },	{ "chdir",		21,	&CScript::Func03 },
		{ "play",		22,	&CScript::Func03 },	{ "speak",		23, &CScript::Func03 },
		{ "popen",		24,	&CScript::Func03 },	{ "pclose",		25, &CScript::Func03 },

		{ "rand",		0,	&CScript::Func04 },	{ "srand",		1,	&CScript::Func04 },
		{ "floor",		2,	&CScript::Func04 },	{ "asin",		3,	&CScript::Func04 },
		{ "sqrt",		4,	&CScript::Func04 },	{ "tan",		5,	&CScript::Func04 },
		{ "acos",		6,	&CScript::Func04 },	{ "abs",		7,	&CScript::Func04 },
		{ "cos",		8,	&CScript::Func04 },	{ "exp",		9,	&CScript::Func04 },
		{ "fmod",		10,	&CScript::Func04 },	{ "atan",		11,	&CScript::Func04 },
		{ "sin",		12,	&CScript::Func04 },	{ "pow",		13,	&CScript::Func04 },
		{ "ceil",		14,	&CScript::Func04 },	{ "sinh",		15,	&CScript::Func04 },
		{ "cosh",		16,	&CScript::Func04 },	{ "tanh",		17,	&CScript::Func04 },
		{ "log",		18,	&CScript::Func04 },	{ "log10",		19,	&CScript::Func04 },
		{ "atan2",		20,	&CScript::Func04 },

		{ "cpow",		0,	&CScript::Func05 },	{ "csqrt",		1,	&CScript::Func05 },
		{ "csin",		2,	&CScript::Func05 },	{ "ccos",		3,	&CScript::Func05 },
		{ "ctan",		4,	&CScript::Func05 },	{ "clog",		5,	&CScript::Func05 },
		{ "cexp",		6,	&CScript::Func05 },	{ "cabs",		7,	&CScript::Func05 },
		{ "carg",		8,	&CScript::Func05 },	{ "complex",	9,	&CScript::Func05 },
		{ "creal",		10,	&CScript::Func05 },	{ "cimg",		11,	&CScript::Func05 },

		{ "iconv",		0,	&CScript::Func06 },	{ "iconvw",		1,	&CScript::Func06 },
		{ "iconvd",		2,	&CScript::Func06 },	{ "md5",		3,	&CScript::Func06 },
		{ "sha1",		4,	&CScript::Func06 },	{ "base64e",	5,	&CScript::Func06 },
		{ "base64d",	6,	&CScript::Func06 },	{ "quotede",	7,	&CScript::Func06 },
		{ "quotedd",	8,	&CScript::Func06 },	{ "base16e",	9,	&CScript::Func06 },
		{ "base16d",	10,	&CScript::Func06 },	{ "crc16",		11,	&CScript::Func06 },
		{ "crc32",		12,	&CScript::Func06 },	{ "escstr",		13,	&CScript::Func06 },

		{ "regopen",	0,	&CScript::Func07 },	{ "regclose",	1,	&CScript::Func07 },
		{ "regchar",	2,	&CScript::Func07 },	{ "regstr",		3,	&CScript::Func07 },
		{ "cipopen",	4,	&CScript::Func07 },	{ "cipclose",	5,	&CScript::Func07 },
		{ "cipstr",		6,	&CScript::Func07 },

		{ "cgetc",		0,	&CScript::Func08 },	{ "cgets",		1,	&CScript::Func08 },
		{ "cread",		2,	&CScript::Func08 },	{ "cputc",		3,	&CScript::Func08 },
		{ "cputs",		4,	&CScript::Func08 },	{ "cwrite",		5,	&CScript::Func08 },
		{ "copen",		6,	&CScript::Func08 },	{ "cclose",		7,	&CScript::Func08 },
		{ "swait",		8,	&CScript::Func08 },	{ "getchar",	9,	&CScript::Func08 },
		{ "setchar",	10,	&CScript::Func08 },	{ "locate",		11,	&CScript::Func08 },
		{ "getpos",		12,	&CScript::Func08 },	{ "xypos",		13,	&CScript::Func08 },
		{ "getstr",		14,	&CScript::Func08 },

		{ "sgetc",		0,	&CScript::Func09 },	{ "sgets",		1,	&CScript::Func09 },
		{ "sread",		2,	&CScript::Func09 },	{ "sputc",		3,	&CScript::Func09 },
		{ "sputs",		4,	&CScript::Func09 },	{ "swrite",		5,	&CScript::Func09 },
		{ "sopen",		6,	&CScript::Func09 },	{ "sclose",		7,	&CScript::Func09 },
		{ "remotestr",	8,	&CScript::Func09 },	{ "localstr",	9,	&CScript::Func09 },
		{ "wait",		10,	&CScript::Func09 },	{ "sendstr",	11,	&CScript::Func09 },
		{ "broadcast",	12,	&CScript::Func09 },

		{ "msgdlg",		0,	&CScript::Func10 },	{ "yesnodlg",	1,	&CScript::Func10 },
		{ "inputdlg",	2,	&CScript::Func10 },	{ "filedlg",	3,	&CScript::Func10 },
		{ "command",	4,	&CScript::Func10 },	{ "shellexec",	5,	&CScript::Func10 },
		{ "passdlg",	6,	&CScript::Func10 },	{ "dump",		7,	&CScript::Func10 },
		{ "menu",		8,	&CScript::Func10 },	{ "iconstyle",	9,	&CScript::Func10 },
		{ "getproint",	10,	&CScript::Func10 },	{ "putproint",	11,	&CScript::Func10 },
		{ "getprostr",	12,	&CScript::Func10 },	{ "putprostr",	13,	&CScript::Func10 },
		{ "menucall",	14,	&CScript::Func10 },	{ "menucheck",	15,	&CScript::Func10 },
		{ "menuenable",	16,	&CScript::Func10 },

		{ "tekopen",	0,	&CScript::TekGrp },	{ "tekclose",	1,	&CScript::TekGrp },
		{ "tekclear",	2,	&CScript::TekGrp },	{ "tekmark",	3,	&CScript::TekGrp },
		{ "tekline",	4,	&CScript::TekGrp },	{ "tektext",	5,	&CScript::TekGrp },
		{ "tekbox",		6,	&CScript::TekGrp },	{ "tekarc",		7,	&CScript::TekGrp },
		{ "tekpoly",	8,	&CScript::TekGrp },	{ "tekbezier",	9,	&CScript::TekGrp },
		{ "teksmooth",	10,	&CScript::TekGrp },	{ "tekspline",	11,	&CScript::TekGrp },
		{ "teklesq",	12,	&CScript::TekGrp },	{ "tekflush",	13,	&CScript::TekGrp },

		{ "dialog",		0,	&CScript::Dialog },	{ "database",	0,	&CScript::DataBs },
		{ "textwnd",	0,	&CScript::TextWnd },

		{ "comset",		0,	&CScript::ComFunc },{ "comdtr",		1,	&CScript::ComFunc },
		{ "comrts",		2,	&CScript::ComFunc },{ "combreak",	3,	&CScript::ComFunc },

		{ NULL,			0,	NULL }
	};
	int n;

	for ( n = 0 ; funcs[n].name != NULL ; n++ ) {
		m_Data[(LPCSTR)funcs[n].name].m_FuncPos = (funcs[n].cmd | 0x80000000L);
		m_Data[(LPCSTR)funcs[n].name].m_FuncExt = funcs[n].proc;
	}

	m_Data["TRUE"] = 1;
	m_Data["FALSE"] = 0;
	m_Data["EOF"] = (-1);

	m_Data["E"]  = 2.71828182845904523536;
	m_Data["PI"] = 3.14159265358979323846;

	m_Data["OPEN_LOOK"]  = (int)0;
	m_Data["OPEN_CATCH"] = (int)1;

	m_Data["CONNECT"] = (int)0;
	m_Data["CLOSE"]   = (int)1;

	m_Data["SH_DENYNO"] = (int)0;
	m_Data["SH_DENYRD"] = (int)1;
	m_Data["SH_DENYWR"] = (int)2;
	m_Data["SH_DENYRW"] = (int)3;

	m_Data["SEEK_SET"] = (int)0;
	m_Data["SEEK_CUR"] = (int)1;
	m_Data["SEEK_END"] = (int)2;
}

//////////////////////////////////////////////////////////////////////

int CScript::SetEvent(int flag)
{
	m_EventFlag |= flag;
	m_EventMask |= flag;
	return FUNC_RET_EVENT;
}
void CScript::ResetEvent(int flag)
{
	m_EventFlag &= ~flag;
	m_EventMask &= ~flag;
}
void CScript::SetDocument(class CRLoginDoc *pDoc)
{
	m_pDoc = pDoc;
	m_Data["Document"].m_bNoCase = TRUE;
	m_pDoc->ScriptInit(0, 2, m_Data["Document"]);
	m_bOpenSock = TRUE;
}
void CScript::SetMenu(CMenu *pMenu)
{
	int n, id;

	if ( pMenu == NULL )
		return;

	for ( n = 0 ; n < 10 ; n++ ) {
		if ( m_MenuTab[n].str.IsEmpty() || m_MenuTab[n].func.IsEmpty() )
			continue;
		if ( pMenu->GetMenuState(IDM_SCRIPT_MENU1 + n, MF_BYCOMMAND) != 0xFFFFFFFF )
			continue;
		if ( (id = m_MenuTab[n].pos) <= 0 )
			id = ID_CHARSCRIPT_END;
		pMenu->InsertMenu(id, MF_BYCOMMAND, IDM_SCRIPT_MENU1 + n, m_MenuTab[n].str);
	}
}
BOOL CScript::IsConsOver()
{
	return (m_ConsBuff.GetSize() >= DATA_BUF_LIMIT ? TRUE : FALSE);
}
void CScript::SetConsBuff(LPBYTE buf, int len)
{
	if ( m_ConsMode == DATA_BUF_NONE )
		return;

	m_ConsBuff.Apend(buf, len);
	m_EventFlag &= ~SCP_EVENT_CONS;
}
BOOL CScript::IsSockOver()
{
	return (m_SockBuff.GetSize() >= DATA_BUF_LIMIT || m_ConsBuff.GetSize() >= DATA_BUF_LIMIT ? TRUE : FALSE);
}
void CScript::SetSockBuff(LPBYTE buf, int len)
{
	if ( m_SockMode == DATA_BUF_NONE )
		return;

	m_SockBuff.Apend(buf, len);
	m_EventFlag &= ~SCP_EVENT_SOCK;
}
void CScript::PutConsOut(LPBYTE buf, int len)
{
	ASSERT(m_pDoc != NULL );
	m_bConsOut = TRUE;
	m_pDoc->m_TextRam.PUTSTR(buf, len);
	m_bConsOut = FALSE;
	m_pDoc->m_TextRam.FLUSH();
}

//////////////////////////////////////////////////////////////////////

void CScript::OnTimer(UINT_PTR nIDEvent)
{
	m_EventFlag &= ~SCP_EVENT_TIMER;
}
void CScript::OnSockChar(int ch)
{
	int n;
	CRegEx *pReg;
	CRegExRes res;

	m_RegMatch = (-1);
	for ( n = 0 ; n < m_RegData.GetSize() ; n++ ) {
		pReg = (CRegEx *)m_RegData[n];
		if ( pReg->MatchChar(ch, 0, &res) ) {
			m_RegMatch = n;
			m_EventFlag &= ~SCP_EVENT_CONS;
			break;
		}
	}
	if ( m_RegMatch >= 0 ) {
		for ( n = 0 ; n < m_RegData.GetSize() ; n++ )
			delete (CRegEx *)m_RegData[n];
		m_RegData.RemoveAll();
	}
}
void CScript::OnConnect()
{
	m_EventFlag &= ~SCP_EVENT_CONNECT;
	m_bAbort = FALSE;
}
void CScript::OnClose()
{
	m_EventFlag = 0;
	m_bAbort = TRUE;
}
BOOL CScript::OnIdle()
{
	BOOL rt = FALSE;

	if ( !m_bInit )
		return FALSE;

	if ( m_pSrcDlg == NULL && !m_bExec ) {
		m_bExec = TRUE;
		try {
			if ( Exec() )
				rt = TRUE;
		} catch(LPCTSTR pMsg) {
			CString tmp;
			tmp.Format(_T("Script Error '%s'"), pMsg);
			AfxMessageBox(tmp);
			m_bOpenSock = FALSE;
			Abort();
		} catch(...) {
			AfxMessageBox(_T("Script Unkown Error"));
			m_bOpenSock = FALSE;
			Abort();
		}
		m_bExec = FALSE;
	}

	if ( m_bOpenSock && m_CodePos == (-1) ) {
		m_bOpenSock = FALSE;
		if ( m_pDoc != NULL && m_pDoc->m_TextRam.m_bOpen && m_pDoc->m_pSock == NULL )
			m_pDoc->SocketOpen();
		rt = TRUE;
	}

	return rt;
}

//////////////////////////////////////////////////////////////////////

int CScript::ExecFile(LPCTSTR filename)
{
	int n;
	LPCTSTR p;

	if ( filename == NULL || *filename == _T('\0') )
		return (-1);

	if ( (n = LoadFile(filename)) < 0 )
		return (-1);

	if ( (p = _tcsrchr(filename, _T('\\'))) != NULL || (p = _tcsrchr(filename, _T(':'))) != NULL )
		filename = p + 1;

	CodeStackPush(TRUE, filename);
	m_ExecLocal = NULL;
	m_CodePos   = m_IncFile[n].m_FuncPos;
	m_bAbort    = FALSE;

	return n;
}
int CScript::ExecStr(LPCTSTR str, int num)
{
	if ( num < 0 && (num = LoadStr(str)) < 0 )
		return (-1);

	CodeStackPush(TRUE, _T("ExecStr"));
	m_ExecLocal = NULL;
	m_CodePos   = m_IncFile[num].m_FuncPos;
	m_bAbort    = FALSE;

	return num;
}
void CScript::OpenDebug(LPCTSTR filename)
{
	if ( m_pSrcDlg != NULL || m_pSrcText == NULL )
		return;

	m_pSrcDlg = new CScriptDlg();

	m_pSrcDlg->m_pScript = this;
	m_pSrcDlg->m_FileName.Format(_T("Debug in '%s'"), (filename != NULL ? filename : _T("all")));
	m_pSrcDlg->m_pSrc = m_pSrcText;
	m_pSrcDlg->m_Max  = m_SrcMax;

	if ( m_SrcCode.GetSize() > 0 && m_CodePos != (-1) ) {
		int n = 0;
		BinaryFind((void *)(INT_PTR)m_CodePos, m_SrcCode.GetData(), sizeof(ScriptDebug), (int)m_SrcCode.GetSize(), SrcCodeCmp, &n);
		while ( n > 0 && n < m_SrcCode.GetSize() && m_CodePos < (int)m_SrcCode[n].code )
			n--;
		m_pSrcDlg->m_SPos = m_SrcCode[n].spos;
		m_pSrcDlg->m_EPos = m_SrcCode[n].epos;
	}

	m_pSrcDlg->Create(IDD_SCRIPTDLG, CWnd::GetDesktopWindow());
	m_pSrcDlg->ShowWindow(SW_SHOW);
}

int CScript::Call(LPCTSTR func, CScriptValue *local)
{
	int n, i;
	CStringArrayExt node;
	CScriptValue *sp = &m_Data;

	node.GetString(func, '.');

	if ( node.GetSize() == 0 )
		return FALSE;

	for ( n = 0 ; n < node.GetSize() ; n++ ) {
		if ( node[n].IsEmpty() )
			continue;
		if ( (i = sp->Find(TstrToMbs(node[n]))) < 0 )
			return FALSE;
		sp = &((*sp)[i]);
	}

	if ( sp->m_FuncPos == (-1) || (sp->m_FuncPos & 0x80000000L) != 0 )
		return FALSE;

	CodeStackPush(TRUE, func);
	m_ExecLocal = local;
	m_CodePos   = sp->m_FuncPos;
	m_bAbort    = FALSE;

	return TRUE;
}
void CScript::ScriptSystemInit(LPCTSTR path, LPCTSTR prog, LPCTSTR work)
{
	SystemValue["System"]["FullPath"] = path;
	SystemValue["System"]["AppName"]  = prog;
	SystemValue["System"]["WorkDir"]  = work;
}

//////////////////////////////////////////////////////////////////////
// CTempWnd

CTempWnd::CTempWnd()
{
	m_pScript = NULL;
	m_pValue  = NULL;
}
CTempWnd::~CTempWnd()
{
}

BEGIN_MESSAGE_MAP(CTempWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CTempWnd::PostNcDestroy()
{
	if ( m_pValue != NULL ) {
		m_pValue->GetAt("pWnd") = (void *)NULL;

		if ( m_pScript != NULL )
			m_pScript->m_EventFlag &= ~SCP_EVENT_DIALOG;

		int n;
		CWnd *pWnd;
		CScriptValue *dp;
		CScriptValue *sp = &(m_pValue->GetAt("Child"));

		for ( n = 0 ; n < sp->GetSize() ; n++ ) {
			dp = &(sp->GetAt(n));
			if ( (pWnd = (CWnd *)(void *)dp->GetAt("pWnd")) == NULL )
				continue;
			dp->GetAt("pWnd") = (void *)NULL;
			delete pWnd;
		}
	}

	delete this;
}
int CTempWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if ( m_pValue == NULL )
		return 0;

	int n;
	int group = (-1);
	CScriptValue *sp = &(m_pValue->GetAt("Child"));
	CScriptValue *dp;

	if ( (n = (int)m_pValue->GetAt("FontPoint")) < 10 )
		n = 90;
	m_Font.CreatePointFont(MulDiv(n, SCREEN_DPI_Y, SYSTEM_DPI_Y), (LPCTSTR)(m_pValue->GetAt("FontName")));
	SetFont(&m_Font, FALSE);

	for ( n = 0 ; n < sp->GetSize() ; n++ ) {
		dp = &(sp->GetAt(n));
		if ( (void *)dp->GetAt("pWnd") != NULL )
			continue;

		LPCSTR type = dp->GetAt("Type");
		CRect rect((int)dp->GetAt("Size")["sx"], (int)dp->GetAt("Size")["sy"],
				   (int)dp->GetAt("Size")["cx"], (int)dp->GetAt("Size")["cy"]);
		rect.right += rect.left; rect.bottom += rect.top;
		LPCTSTR text = dp->GetAt("Text");
		int style = WS_CHILD | WS_VISIBLE;
		int state = dp->GetAt("Check");

		if ( _stricmp(type, "static") == 0 ) {
			CStatic *pWnd = new CStatic;
			pWnd->Create(text, style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			dp->GetAt("pWnd") = (void *)pWnd;
		} else if ( _stricmp(type, "edit") == 0 ) {
			CEdit *pWnd = new CEdit;
			pWnd->Create(ES_AUTOHSCROLL | WS_TABSTOP | style, rect, this, 3000 + n);
			pWnd->ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);
			pWnd->SetFont(&m_Font, FALSE);
			pWnd->SetWindowText(text);
			dp->GetAt("pWnd") = (void *)pWnd;

		} else if ( _stricmp(type, "check") == 0 ) {
			CButton *pWnd = new CButton;
			pWnd->Create(text, BS_AUTOCHECKBOX | WS_TABSTOP | style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			pWnd->SetCheck(state);
			dp->GetAt("pWnd") = (void *)pWnd;

		} else if ( _stricmp(type, "radiotop") == 0 ) {
			CButton *pWnd = new CButton;
			pWnd->Create(text, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP | style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			pWnd->SetCheck(state);
			dp->GetAt("pWnd") = (void *)pWnd;
			dp->GetAt("Group") = group = n;
			if ( (state == BST_CHECKED) )
				sp->GetAt("Child").GetAt(group).GetAt("Value") = (int)(n - group);
		} else if ( _stricmp(type, "radio") == 0 ) {
			CButton *pWnd = new CButton;
			pWnd->Create(text, BS_AUTORADIOBUTTON | WS_TABSTOP | style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			pWnd->SetCheck(state);
			dp->GetAt("pWnd") = (void *)pWnd;
			dp->GetAt("Group") = group;
			if ( group >= 0 && (state == BST_CHECKED) )
				sp->GetAt("Child").GetAt(group).GetAt("Value") = (int)(n - group);

		} else if ( _stricmp(type, "group") == 0 ) {
			CButton *pWnd = new CButton;
			pWnd->Create(text, BS_GROUPBOX | style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			dp->GetAt("pWnd") = (void *)pWnd;
		} else if ( _stricmp(type, "button") == 0 ) {
			CButton *pWnd = new CButton;
			pWnd->Create(text, WS_TABSTOP | style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			dp->GetAt("pWnd") = (void *)pWnd;

		} else if ( _stricmp(type, "combo") == 0 ) {
			CComboBox *pWnd = new CComboBox;
			pWnd->Create(CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP | style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			pWnd->SetWindowText(text);
			dp->GetAt("pWnd") = (void *)pWnd;
		} else if ( _stricmp(type, "list") == 0 ) {
			CComboBox *pWnd = new CComboBox;
			pWnd->Create(CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP | style, rect, this, 3000 + n);
			pWnd->SetFont(&m_Font, FALSE);
			pWnd->SetWindowText(text);
			dp->GetAt("pWnd") = (void *)pWnd;

		} else if ( _stricmp(type, "progress") == 0 ) {
			CProgressCtrl *pWnd = new CProgressCtrl;
			pWnd->Create(style, rect, this, 3000 + n);
			dp->GetAt("pWnd") = (void *)pWnd;
		}
	}

	return 0;
}
void CTempWnd::OnClose()
{
	CWnd::OnClose();
}
void CTempWnd::FuncCall(int nID)
{
	LPCTSTR func = (LPCTSTR)m_pValue->GetAt("Child").GetAt(nID - 3000).GetAt("Func");

	if ( *func == _T('\0') )
		return;

	CScriptValue *local = new CScriptValue;
	(*local)[0] = (int)(nID - 3000);
	(*local)[1] = (CScriptValue *)m_pValue;

	if ( !m_pScript->Call(func, local) )
		delete local;
}
BOOL CTempWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if ( nID < 3000 || nID > 5000 )
		goto SKIP;

	switch(nCode) {
	case BN_CLICKED:
		{
			int n;
			CScriptValue *sp = &(m_pValue->GetAt("Child").GetAt(nID - 3000));
			CButton *pWnd = (CButton *)(void *)(sp->GetAt("pWnd"));
			if ( pWnd == NULL )
				break;

			sp->GetAt("Check") = (int)pWnd->GetCheck();

			if ( (n = sp->GetAt("Group")) >= 0 )
				m_pValue->GetAt("Child").GetAt(n).GetAt("Value") = (int)(nID - 3000 - n);

			FuncCall(nID);
		}
		break;

	case CBN_SELCHANGE:
		{
			int n;
			CString text;
			CScriptValue *sp = &(m_pValue->GetAt("Child").GetAt(nID - 3000));
			CComboBox *pWnd = (CComboBox *)(void *)(sp->GetAt("pWnd"));

			if ( pWnd == NULL )
				break;

			if ( (n = pWnd->GetCurSel()) >= 0 )
				pWnd->GetLBText(n, text);
			else
				pWnd->GetWindowText(text);

			sp->GetAt("Value") = n;
			sp->GetAt("Text") = text;

			FuncCall(nID);
		}
		break;

	case CBN_EDITUPDATE:
		{
			CString text;
			CScriptValue *sp = &(m_pValue->GetAt("Child").GetAt(nID - 3000));
			CComboBox *pWnd = (CComboBox *)(void *)(sp->GetAt("pWnd"));

			if ( pWnd == NULL )
				break;

			pWnd->GetWindowText(text);
			sp->GetAt("Value") = (-1);
			sp->GetAt("Text") = text;

			FuncCall(nID);
		}
		break;

	case EN_UPDATE:
		{
			CString text;
			CScriptValue *sp = &(m_pValue->GetAt("Child").GetAt(nID - 3000));
			CEdit *pWnd = (CEdit *)(void *)(sp->GetAt("pWnd"));

			if ( pWnd == NULL )
				break;

			pWnd->GetWindowText(text);
			sp->GetAt("Text") = text;

			FuncCall(nID);
		}
		break;
	}

SKIP:
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
BOOL CTempWnd::PreTranslateMessage(MSG* pMsg)
{
    if( IsDialogMessage(pMsg) )
        return TRUE;
    else
		return CWnd::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////
// CScriptCmdUI

CScriptCmdUI::CScriptCmdUI()
{
	m_bEnable = FALSE;
	m_Check   = 0;
	m_bRadio  = FALSE;
}
void CScriptCmdUI::Enable(BOOL bOn)
{
	m_bEnable = bOn;
}
void CScriptCmdUI::SetCheck(int nCheck)
{
	m_Check = nCheck;
}
void CScriptCmdUI::SetRadio(BOOL bOn)
{
	m_bRadio = bOn;
}
void CScriptCmdUI::SetText(LPCTSTR lpszText)
{
	m_Text = lpszText;
}
