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
// CProgressWnd

IMPLEMENT_DYNAMIC(CProgressWnd, CWnd)

CProgressWnd::CProgressWnd()
{
	m_bNoRange = FALSE;
	m_RangeLower = 0;
	m_RangeUpper = 0;
	m_RangePos = 0;

	m_RateMax = 1;
	m_LastPos = 0;

	m_DataMax = 0;
	m_pDataTab = NULL;
}
CProgressWnd::~CProgressWnd()
{
	if ( m_pDataTab != NULL )
		delete [] m_pDataTab;
}

void CProgressWnd::InitDataTab(BOOL bInit)
{
	CRect rect;
	int DataMax;
	BYTE *pDataTab;

	GetClientRect(rect);

	if ( !bInit && m_pDataTab != NULL && m_DataMax == rect.Width() )
		return;

	DataMax = rect.Width();
	pDataTab = new BYTE[DataMax];

	if ( !bInit && m_pDataTab != NULL ) {
		for ( int n = 0 ; n < DataMax ; n++ )
			pDataTab[n] = m_pDataTab[n * m_DataMax / DataMax];
		if ( m_LastPos >= DataMax )
			m_LastPos = DataMax - 1;
	} else {
		ZeroMemory(pDataTab, sizeof(BYTE) * DataMax);
		m_RateMax = 1;
		m_LastPos = 0;
	}

	if ( m_pDataTab != NULL )
		delete [] m_pDataTab;

	m_DataMax = DataMax;
	m_pDataTab = pDataTab;
}

BEGIN_MESSAGE_MAP(CProgressWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
END_MESSAGE_MAP()

void CProgressWnd::SetRange(short nLower, short nUpper)
{
	SetRange32((int)nLower, (int)nUpper);
}
void CProgressWnd::SetRange32(int nLower, int nUpper)
{
	m_RangeLower = nLower;
	m_RangeUpper = nUpper;
	m_bNoRange = (m_RangeLower >= m_RangeUpper ? TRUE : FALSE);

	InitDataTab(TRUE);
}
void CProgressWnd::GetRange(int &nLower, int &nUpper)
{
	nLower = m_RangeLower;
	nUpper = m_RangeUpper;
}
int CProgressWnd::GetPos()
{
	return m_RangePos;
}
int CProgressWnd::SetPos(int nPos, int nRate)
{
	CRect rect;

	InitDataTab(FALSE);
	m_RangePos = nPos;

	if ( m_DataMax == 0 )
		return m_RangePos;

	if ( m_bNoRange ) {
		m_RangeLower = 0;
		m_RangeUpper = nPos = m_DataMax;
		for ( int n = 0 ; n < (m_DataMax - 1) ; n++ )
			m_pDataTab[n] = m_pDataTab[n + 1];
		Invalidate(FALSE);
	}

	if ( nPos < m_RangeLower ) nPos = m_RangeLower;
	if ( nPos > m_RangeUpper ) nPos = m_RangeUpper;

	if ( nRate > 0 && (int)((LONGLONG)m_RateMax * 255/ nRate) < 255 ) {
		for ( int n = 0 ; n < m_DataMax ; n++ )
			m_pDataTab[n] = (BYTE)((LONGLONG)m_pDataTab[n] * m_RateMax / nRate);
		m_RateMax = nRate;
		Invalidate(FALSE);
	}

	nPos = (int)((LONGLONG)(nPos - m_RangeLower) * (m_DataMax - 1) / (LONGLONG)(m_RangeUpper - m_RangeLower));
	nRate = (int)((LONGLONG)nRate * 255 / (LONGLONG)m_RateMax);

	if ( nPos < 0 ) nPos = 0;
	if ( nPos >= m_DataMax ) nPos = m_DataMax - 1;

	GetClientRect(rect);

	if ( m_LastPos > nPos ) {
		rect.right = rect.left + m_LastPos + 1;
		rect.left += nPos;
		InvalidateRect(rect, FALSE);

		for ( int n = nPos ; n <= m_LastPos ; n++ )
			m_pDataTab[n] = 0;
		m_LastPos = nPos;
		m_pDataTab[nPos] = nRate;

	} else {
		rect.right = rect.left + nPos + 1;
		rect.left += m_LastPos;
		InvalidateRect(rect, FALSE);

		while ( m_LastPos < nPos )
			m_pDataTab[m_LastPos++] = nRate;
		m_pDataTab[nPos] = nRate;
	}

	return m_RangePos;
}

void CProgressWnd::OnPaint()
{
	int n, x, y;
	int sx, ex;
	CRect rect;
	CPaintDC dc(this);
	CPen PenLine[3];
	CPen *pOldPen;

	PenLine[0].CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
	PenLine[1].CreatePen(PS_SOLID, 0, RGB(  0, 172,  64));
	PenLine[2].CreatePen(PS_SOLID, 0, RGB(215, 215, 215));

	pOldPen = dc.SelectObject(&PenLine[0]);

	InitDataTab(FALSE);
	GetClientRect(rect);

	if ( (sx = dc.m_ps.rcPaint.left - rect.left) < 0 )
		sx = 0;
	if ( (ex = dc.m_ps.rcPaint.right - rect.left) > m_DataMax )
		ex = m_DataMax;

	for ( x = sx ; x < ex ; x++ ) {
		y = m_pDataTab[x] * rect.Height() / 255;

		dc.SelectObject(&PenLine[1]);
		dc.MoveTo(rect.left + x, rect.bottom);
		dc.LineTo(rect.left + x, rect.bottom - y);

		dc.SelectObject(&PenLine[0]);
		dc.LineTo(rect.left + x, rect.top);
	}

	dc.SelectObject(&PenLine[2]);
	for ( n = 1 ; n < 4 ; n++ ) {
		y = rect.Height() * n / 4;
		dc.MoveTo(rect.left + sx, rect.bottom - y);
		dc.LineTo(rect.left + ex, rect.bottom - y);

		x = rect.Width() * n / 4;
		if ( sx <= x && x < ex ) { 
			dc.MoveTo(rect.left + x, rect.top);
			dc.LineTo(rect.left + x, rect.bottom);
		}
	}

	dc.SelectObject(pOldPen);
}

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
	m_pAbortFlag = NULL;
	m_pSock = NULL;
	m_UpdatePost = FALSE;
	m_AutoDelete = FALSE;
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
	ON_WM_NCDESTROY()
	ON_WM_TIMER()
	ON_MESSAGE(WM_PROGUPDATE, &CProgDlg::OnProgUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgDlg メッセージ ハンドラ

void CProgDlg::OnNcDestroy()
{
	CDialogExt::OnNcDestroy();

	if ( m_AutoDelete )
		delete this;
}
void CProgDlg::OnCancel() 
{
	if ( m_pAbortFlag != NULL )
		*m_pAbortFlag = TRUE;
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
	UpdateData(FALSE);

	m_LastSize = 0;
	m_ResumeSize = 0;
	m_UpdatePos = m_LastPos = m_ActivePos = 0;
	m_StartClock = m_LastClock = m_UpdateClock = clock();
	m_LastRate = 0;

	SetTimer(1130, 200, NULL);

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

	return FALSE;
}

void CProgDlg::SetRange(LONGLONG max, LONGLONG rem)
{
	for ( m_Div = 1 ; (max / m_Div) >= 0x8000 ; m_Div *= 2 )
		;
	
	m_FileSize.SetRange((short)(rem / m_Div), (short)(max / m_Div));
	m_FileSize.SetPos(0);
	m_UpdatePos = m_ActivePos = 0;
	m_LastRate = 0;

	UpdateData(TRUE);

	m_LastSize = max;
	m_ResumeSize = m_LastPos = rem;
	m_StartClock = m_LastClock = m_UpdateClock = clock();

	UpdateData(FALSE);
	m_UpdatePost = FALSE;
}
void CProgDlg::SetPos(LONGLONG pos)
{
	m_UpdatePos = pos;
	m_UpdateClock = clock();

	if ( (m_UpdateClock - m_LastClock) > 50 ) {
		m_LastRate = (int)((pos - m_LastPos) * CLOCKS_PER_SEC / (LONGLONG)(m_UpdateClock - m_LastClock));
		if ( (m_UpdateClock - m_LastClock) >= CLOCKS_PER_SEC ) {
			m_LastClock = (m_LastClock + m_UpdateClock) / 2;
			m_LastPos = (m_LastPos + pos) / 2;
		}
	}

	m_FileSize.SetPos((int)(pos / m_Div), m_LastRate);
}
void CProgDlg::UpdatePos(LONGLONG pos)
{
	int n;
	double d = 1.0;

	UpdateData(TRUE);
//	m_FileSize.SetPos((int)(pos / m_Div));

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
	m_UpdatePost = FALSE;
}
LRESULT CProgDlg::OnProgUpdate(WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pStr = (LPCTSTR)lParam;
	LONGLONG *pLongLong = (LONGLONG *)lParam;

	switch((int)wParam) {
	case PROG_UPDATE_RANGE:
		SetRange(pLongLong[0], pLongLong[1]);
		break;
	case PROG_UPDATE_POS:
		SetPos(pLongLong[0]);
		break;
	case PROG_UPDATE_FILENAME:
		SetFileName(pStr);
		break;
	case PROG_UPDATE_MESSAGE:
		SetMessage(pStr);
		break;
	case PROG_UPDATE_DESTORY:
		m_AutoDelete = TRUE;
		//if ( pStr != NULL && *pStr != _T('\0') )
		//	SetMessage(pStr);
		//else
			DestroyWindow();
		break;
	}

	return TRUE;
}
