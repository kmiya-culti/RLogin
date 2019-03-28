// PrnTextDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "PrnTextDlg.h"

// CPrnTextDlg ダイアログ

IMPLEMENT_DYNAMIC(CPrnTextDlg, CDialogExt)

CPrnTextDlg::CPrnTextDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CPrnTextDlg::IDD, pParent)
{
	m_FontName = _T("");
	m_FontSize = 10;
	m_FontLine = 10;

	m_Margin.left   = 10;
	m_Margin.right  = 10;
	m_Margin.top    = 10;
	m_Margin.bottom = 10;
}

CPrnTextDlg::~CPrnTextDlg()
{
}

void CPrnTextDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_CBString(pDX, IDC_COMBO1, m_FontName);
	DDX_Text(pDX, IDC_EDIT1, m_FontSize);
	DDX_Text(pDX, IDC_EDIT2, m_FontLine);

	DDX_Text(pDX, IDC_EDIT3, m_Margin.left);
	DDX_Text(pDX, IDC_EDIT4, m_Margin.right);
	DDX_Text(pDX, IDC_EDIT5, m_Margin.top);
	DDX_Text(pDX, IDC_EDIT6, m_Margin.bottom);
}

BEGIN_MESSAGE_MAP(CPrnTextDlg, CDialogExt)
END_MESSAGE_MAP()

static int CALLBACK EnumFontFamExComboAddStr(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam)
{
	CPrnTextDlg *pWnd = (CPrnTextDlg *)lParam;
	CComboBox *pCombo = (CComboBox *)pWnd->GetDlgItem(IDC_COMBO1);
	LPCTSTR name = lpelfe->elfLogFont.lfFaceName;

	if ( name[0] != _T('@') && pCombo->FindStringExact((-1), name) == CB_ERR )
		pCombo->AddString(name);

	return TRUE;
}

// CPrnTextDlg メッセージ ハンドラー

BOOL CPrnTextDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	CClientDC dc(this);
	LOGFONT logfont;

	ZeroMemory(&logfont, sizeof(LOGFONT)); 
	logfont.lfCharSet = DEFAULT_CHARSET;
	::EnumFontFamiliesEx(dc.m_hDC, &logfont, (FONTENUMPROC)EnumFontFamExComboAddStr, (LPARAM)this, 0);

	m_FontName = ::AfxGetApp()->GetProfileString(_T("PrintText"), _T("FontName"), PRINTFONTNAME);
	m_FontSize = ::AfxGetApp()->GetProfileInt(_T("PrintText"),    _T("FontSize"), PRINTFONTSIZE);
	m_FontLine = ::AfxGetApp()->GetProfileInt(_T("PrintText"),    _T("FontLine"), PRINTLINESIZE);

	m_Margin.left   = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginLeft"),   PRINTMARGINLEFT);
	m_Margin.right  = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginRight"),  PRINTMARGINRIGHT);
	m_Margin.top    = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginTop"),    PRINTMARGINTOP);
	m_Margin.bottom = ::AfxGetApp()->GetProfileInt(_T("PrintText"), _T("MarginBottom"), PRINTMARGINBOTTOM);

	UpdateData(FALSE);

	SubclassComboBox(IDC_COMBO1);

	return TRUE;
}

void CPrnTextDlg::OnOK()
{
	UpdateData(TRUE);

	::AfxGetApp()->WriteProfileString(_T("PrintText"), _T("FontName"), m_FontName);
	::AfxGetApp()->WriteProfileInt(_T("PrintText"),    _T("FontSize"), m_FontSize);
	::AfxGetApp()->WriteProfileInt(_T("PrintText"),    _T("FontLine"), m_FontLine);

	::AfxGetApp()->WriteProfileInt(_T("PrintText"), _T("MarginLeft"),   m_Margin.left);
	::AfxGetApp()->WriteProfileInt(_T("PrintText"), _T("MarginRight"),  m_Margin.right);
	::AfxGetApp()->WriteProfileInt(_T("PrintText"), _T("MarginTop"),    m_Margin.top);
	::AfxGetApp()->WriteProfileInt(_T("PrintText"), _T("MarginBottom"), m_Margin.bottom);

	CDialogExt::OnOK();
}
