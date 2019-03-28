// ComboBoxHis.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "ComboBoxHis.h"

// CComboBoxExt

/*
	CComboBoxに対してSetFont/MoveWindowを実行すると全範囲が選択されるバグ？
*/

IMPLEMENT_DYNAMIC(CComboBoxExt, CComboBox)

CComboBoxExt::CComboBoxExt()
{
}

CComboBoxExt::~CComboBoxExt()
{
}

BEGIN_MESSAGE_MAP(CComboBoxExt, CComboBox)
	ON_WM_WINDOWPOSCHANGED()
	ON_MESSAGE(WM_SETFONT, OnSetFont)
END_MESSAGE_MAP()

void CComboBoxExt::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	DWORD pos = GetEditSel();

	CComboBox::OnWindowPosChanged(lpwndpos);

	if ( pos != CB_ERR )
		SetEditSel(LOWORD(pos), HIWORD(pos));
}
LRESULT CComboBoxExt::OnSetFont(WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	DWORD pos = GetEditSel();

	result = DefWindowProc(WM_SETFONT, wParam, lParam);

	if ( pos != CB_ERR )
		SetEditSel(LOWORD(pos), HIWORD(pos));

	return result;
}

// CComboBoxHis

IMPLEMENT_DYNAMIC(CComboBoxHis, CComboBoxExt)

CComboBoxHis::CComboBoxHis()
{
}

CComboBoxHis::~CComboBoxHis()
{
}

BEGIN_MESSAGE_MAP(CComboBoxHis, CComboBoxExt)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CComboBoxHis メッセージ ハンドラー

void CComboBoxHis::LoadHis(LPCTSTR lpszSection)
{
	int n;
	CStringArrayExt aray;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();

	pApp->GetProfileArray(lpszSection, aray);
	for ( n = 0 ; n < aray.GetSize() ; n++ )
		AddString(aray[n]);
	m_Section = lpszSection;
}
void CComboBoxHis::SaveHis(LPCTSTR lpszSection)
{
	int n;
	CString str;
	CStringArrayExt aray;
	CRLoginApp *pApp = (CRLoginApp *)AfxGetApp();

	for ( n = 0 ; n < GetCount() && n < 20 ; n++ ) {
		GetLBText(n, str);
		aray.Add(str);
	}
	pApp->WriteProfileArray(lpszSection, aray);
}
void CComboBoxHis::AddHis(LPCTSTR str)
{
	int n;

	if ( (n = FindStringExact(0, str)) != CB_ERR )
		DeleteString(n);

	InsertString(0, str);
}
BOOL CComboBoxHis::PreTranslateMessage(MSG* pMsg)
{
	int idx;
	CString tmp[2];

	if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE && GetDroppedState() && (idx = GetCurSel()) >= 0 ) {
		GetWindowText(tmp[0]);
		GetLBText(idx, tmp[1]);
		ShowDropDown(FALSE);
		DeleteString(idx);
		ShowDropDown(TRUE);
		if ( tmp[0].Compare(tmp[1]) != 0 )
			SetWindowText(tmp[0]);
		return TRUE;
	}

	return CComboBox::PreTranslateMessage(pMsg);
}
void CComboBoxHis::OnDestroy()
{
	if ( !m_Section.IsEmpty() )
		SaveHis(m_Section);

	CComboBox::OnDestroy();
}
