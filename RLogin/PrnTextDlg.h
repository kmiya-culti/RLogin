#pragma once

#include "DialogExt.h"

// CPrnTextDlg ダイアログ

#define	PRINTFONTNAME		_T("ＭＳ ゴシック")
#define	PRINTFONTSIZE		10		// Point
#define	PRINTLINESIZE		15		// Point

#define	PRINTMARGINLEFT		10		// mm
#define	PRINTMARGINRIGHT	10		// mm
#define	PRINTMARGINTOP		10		// mm
#define	PRINTMARGINBOTTOM	10		// mm

class CPrnTextDlg : public CDialogExt
{
	DECLARE_DYNAMIC(CPrnTextDlg)

public:
	CPrnTextDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CPrnTextDlg();

// ダイアログ データ
	enum { IDD = IDD_PRNTEXTDLG };

public:
	CString m_FontName;
	int m_FontSize;
	int m_FontLine;
	CRect m_Margin;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	DECLARE_MESSAGE_MAP()
};
