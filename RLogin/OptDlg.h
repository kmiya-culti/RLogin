#if !defined(AFX_OPTDLG_H__62A00652_C3C5_4FF6_8FDF_89A5AFB8EDBE__INCLUDED_)
#define AFX_OPTDLG_H__62A00652_C3C5_4FF6_8FDF_89A5AFB8EDBE__INCLUDED_

#include "SerEntPage.h"
#include "TermPage.h"	// ClassView によって追加されました。
#include "ScrnPage.h"	// ClassView によって追加されました。
#include "ProtoPage.h"	// ClassView によって追加されました。
#include "CharSetPage.h"	// ClassView によって追加されました。
#include "ColParaDlg.h"	// ClassView によって追加されました。
#include "KeyPage.h"	// ClassView によって追加されました。
#include "HisPage.h"	// ClassView によって追加されました。

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptDlg.h : ヘッダー ファイル
//

/////////////////////////////////////////////////////////////////////////////
// COptDlg

class COptDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(COptDlg)

// コンストラクション
public:
	COptDlg(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	COptDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// アトリビュート
public:

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(COptDlg)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	CButton m_DoInit;
	CButton m_DoAppDmy;

	CSerEntPage m_SerEntPage;
	CKeyPage m_KeyPage;
	CColParaDlg m_ColorPage;
	CCharSetPage m_CharSetPage;
	CProtoPage m_ProtoPage;
	CTermPage m_TermPage;
	CScrnPage m_ScrnPage;
	CHisPage m_HisPage;

	CServerEntry *m_pEntry;
	CTextRam *m_pTextRam;
	CKeyNodeTab *m_pKeyTab;
	CParamTab *m_pParamTab;
	class CRLoginDoc *m_pDocument;

#define	UMOD_ENTRY		001
#define	UMOD_TEXTRAM	002
#define	UMOD_KEYTAB		004
#define	UMOD_PARAMTAB	010
	int m_ModFlag;

	virtual ~COptDlg();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(COptDlg)
	//}}AFX_MSG
	afx_msg void OnDoInit();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_OPTDLG_H__62A00652_C3C5_4FF6_8FDF_89A5AFB8EDBE__INCLUDED_)
