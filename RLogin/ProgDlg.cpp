// ProgDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "ProgDlg.h"
#include "ExtSocket.h"
#include "SyncSock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgDlg ダイアログ

IMPLEMENT_DYNAMIC(CProgDlg, CDialogExt)

CProgDlg::CProgDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CProgDlg::IDD, pParent)
{
	m_EndTime = _T("");
	m_TotalSize = _T("");
	m_TransRate = _T("");
	m_FileName = _T("");
	m_Message = _T("");
	m_Div = 1;
	m_AbortFlag = FALSE;
	m_pSock = NULL;
}

void CProgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_SIZEPROG, m_FileSize);
	DDX_Text(pDX, IDC_ENDTIME, m_EndTime);
	DDX_Text(pDX, IDC_TOTALSIZE, m_TotalSize);
	DDX_Text(pDX, IDC_TRANSRATE, m_TransRate);
	DDX_Text(pDX, IDC_FILENAME, m_FileName);
	DDX_Text(pDX, IDC_MESSAGE, m_Message);
}

BEGIN_MESSAGE_MAP(CProgDlg, CDialogExt)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgDlg メッセージ ハンドラ

void CProgDlg::OnCancel() 
{
	m_AbortFlag = TRUE;
	if ( m_pSock != NULL )
		m_pSock->SyncAbort();
//	CDialogExt::OnCancel();
}

BOOL CProgDlg::OnInitDialog() 
{
	CDialogExt::OnInitDialog();

	m_FileSize.SetRange(0, 0);
	m_FileSize.SetPos(0);
	m_EndTime = _T("");
	m_TotalSize = _T("");
	m_TransRate = _T("");
	m_Message   = _T("");
	m_AbortFlag = FALSE;
	UpdateData(FALSE);

	m_LastSize = 0;
	m_ResumeSize = 0;
	m_UpdatePos = m_ActivePos = 0;
	m_StartClock = m_UpdateClock = clock();

	SetTimer(1024, 300, NULL);

	int x, y;
	CRect rect, box;

	GetParent()->GetClientRect(box);
	GetWindowRect(rect);
	x = (box.Width() - rect.Width()) / 2;
	y = (box.Height() - rect.Height()) / 2;
	rect.left   += x;
	rect.right  += x;
	rect.top    += y;
	rect.bottom += y;
	MoveWindow(rect);

	return TRUE;
}

void CProgDlg::SetRange(LONGLONG max, LONGLONG rem)
{
	for ( m_Div = 1 ; (max / m_Div) >= 0x8000 ; m_Div *= 2 )
		;
	
	m_FileSize.SetRange(0, (short)(max / m_Div));
	m_FileSize.SetPos(0);
	m_UpdatePos = m_ActivePos = 0;

	UpdateData(TRUE);
	m_LastSize = max;
	m_ResumeSize = rem;
	m_StartClock = m_UpdateClock = clock();
	UpdateData(FALSE);
}
void CProgDlg::SetPos(LONGLONG pos)
{
	m_UpdatePos = pos;
	m_UpdateClock = clock();
}
void CProgDlg::UpdatePos(LONGLONG pos)
{
	int n;
	double d;

	UpdateData(TRUE);
	m_FileSize.SetPos((int)(pos / m_Div));

	if ( pos > 1000000 )
		m_TotalSize.Format(_T("%d.%03dM"), (int)(pos / 1000000), (int)((pos / 1000) % 1000));
	else if ( pos > 1000 )
		m_TotalSize.Format(_T("%dK"), (int)(pos / 1000));
	else
		m_TotalSize.Format(_T("%d"), (int)pos);

	if ( m_UpdateClock > m_StartClock ) {
		d = (double)(pos - m_ResumeSize) * CLOCKS_PER_SEC / (double)(m_UpdateClock - m_StartClock);

		if ( d > 0.0 && m_LastSize > 0 ) {
			n = (int)((double)(m_LastSize - pos) / d);
			if ( n >= 3600 )
				m_EndTime.Format(_T("%d:%02d:%02d"), n / 3600, (n % 3600) / 60, n % 60);
			else if ( n >= 60 )
				m_EndTime.Format(_T("%d:%02d"), n / 60, n % 60);
			else
				m_EndTime.Format(_T("%d"), n);
		} else
			m_EndTime = _T("");

		if ( d > 10048576.0 )
			m_TransRate.Format(_T("%dM"), (int)(d / 1048576.0));
		else if ( d > 10024.0 )
			m_TransRate.Format(_T("%dK"), (int)(d / 1024.0));
		else
			m_TransRate.Format(_T("%d"), (int)(d));
	}

	UpdateData(FALSE);
}
void CProgDlg::SetFileName(LPCTSTR file)
{
	UpdateData(TRUE);
	m_FileName = file;
	UpdateData(FALSE);
}
void CProgDlg::SetMessage(LPCTSTR msg)
{
	UpdateData(TRUE);
	m_Message = msg;
	UpdateData(FALSE);
}
void CProgDlg::OnTimer(UINT_PTR nIDEvent)
{
	if ( m_ActivePos != m_UpdatePos ) {
		m_ActivePos = m_UpdatePos;
		UpdatePos(m_UpdatePos);
	}
}

