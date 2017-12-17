// ComboBoxHis.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "ComboBoxHis.h"

// CComboBoxHis

IMPLEMENT_DYNAMIC(CComboBoxHis, CComboBox)

CComboBoxHis::CComboBoxHis()
{
}

CComboBoxHis::~CComboBoxHis()
{
}

BEGIN_MESSAGE_MAP(CComboBoxHis, CComboBox)
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
