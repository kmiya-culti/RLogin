// MainFrm.cpp : CMainFrame クラスの実装
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ServerSelect.h"
#include "Script.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib,"winmm")

/////////////////////////////////////////////////////////////////////////////
// CPaneFrame

CPaneFrame::CPaneFrame(class CMainFrame *pMain, HWND hWnd, class CPaneFrame *pOwn)
{
	m_pLeft  = NULL;
	m_pRight = NULL;
	m_hWnd   = hWnd;
	m_pOwn   = pOwn;
	m_pMain  = pMain;

	m_Style = PANEFRAME_WINDOW;
	m_BoderSize = 1;
	m_bActive = FALSE;
	m_pServerEntry = NULL;

	if ( m_pOwn != NULL )
		m_Frame = m_pOwn->m_Frame;
	else
		m_pMain->GetFrameRect(m_Frame);
}
CPaneFrame::~CPaneFrame()
{
	if ( m_pServerEntry != NULL )
		delete m_pServerEntry;

	if ( m_NullWnd.m_hWnd != NULL )
		m_NullWnd.DestroyWindow();

	if ( m_pLeft != NULL )
		delete m_pLeft;

	if ( m_pRight != NULL )
		delete m_pRight;
}
void CPaneFrame::Attach(HWND hWnd)
{
	ASSERT(m_Style == PANEFRAME_WINDOW);
	m_hWnd = hWnd;
	m_bActive = FALSE;
	if ( m_NullWnd.m_hWnd != NULL )
		m_NullWnd.DestroyWindow();
}
void CPaneFrame::CreatePane(int Style, HWND hWnd)
{
	ASSERT(m_Style == PANEFRAME_WINDOW);
	ASSERT(m_pLeft == NULL);
	ASSERT(m_pRight == NULL);

	m_pLeft  = new CPaneFrame(m_pMain, m_hWnd, this);
	m_pRight = new CPaneFrame(m_pMain, hWnd, this);

	m_hWnd  = NULL;
	m_Style = Style;

	if ( m_NullWnd.m_hWnd != NULL )
		m_NullWnd.DestroyWindow();

	if ( m_bActive )
		m_pLeft->m_bActive = TRUE;
	m_bActive = FALSE;

	switch(m_Style) {
	case PANEFRAME_WIDTH:
		m_pLeft->m_Frame.right = m_pLeft->m_Frame.left + (m_Frame.Width() - m_BoderSize) / 2;
		m_pRight->m_Frame.left = m_pLeft->m_Frame.right + m_BoderSize;
		break;
	case PANEFRAME_HEIGHT:
		m_pLeft->m_Frame.bottom = m_pLeft->m_Frame.top + (m_Frame.Height() - m_BoderSize) / 2;
		m_pRight->m_Frame.top   = m_pLeft->m_Frame.bottom + m_BoderSize;
		break;
	case PANEFRAME_MAXIM:
		CPaneFrame *pPane = m_pLeft;
		m_pLeft = m_pRight;
		m_pRight = pPane;
		break;
	}

	m_pLeft->MoveFrame();
	m_pRight->MoveFrame();
}
class CPaneFrame *CPaneFrame::DeletePane(HWND hWnd)
{
	if ( m_Style == PANEFRAME_WINDOW ) {
		if ( m_hWnd == hWnd ) {
			delete this;
			return NULL;
		}
		return this;
	}

	ASSERT(m_pLeft  != NULL);
	ASSERT(m_pRight != NULL);

	if ( (m_pLeft = m_pLeft->DeletePane(hWnd)) == NULL ) {
		if ( m_pRight->m_Style == PANEFRAME_WINDOW ) {
			m_hWnd = m_pRight->m_hWnd;
			m_Style = m_pRight->m_Style;
			delete m_pRight;
			m_pRight = NULL;
			MoveFrame();
		} else {
			CPaneFrame *pPane = m_pRight;
			m_pLeft  = pPane->m_pLeft;  pPane->m_pLeft  = NULL;
			m_pRight = pPane->m_pRight; pPane->m_pRight = NULL;
			m_Style  = pPane->m_Style;
			CRect rect = m_Frame;
			m_Frame = pPane->m_Frame;
			m_pLeft->m_pOwn = this;
			m_pRight->m_pOwn = this;
			MoveParOwn(rect, PANEFRAME_NOCHNG);
			delete pPane;
		}

	} else if ( (m_pRight = m_pRight->DeletePane(hWnd)) == NULL ) {
		if ( m_pLeft->m_Style == PANEFRAME_WINDOW ) {
			m_hWnd = m_pLeft->m_hWnd;
			m_Style = m_pLeft->m_Style;
			delete m_pLeft;
			m_pLeft = NULL;
			MoveFrame();
		} else {
			CPaneFrame *pPane = m_pLeft;
			m_pLeft  = pPane->m_pLeft;  pPane->m_pLeft  = NULL;
			m_pRight = pPane->m_pRight; pPane->m_pRight = NULL;
			m_Style  = pPane->m_Style;
			CRect rect = m_Frame;
			m_Frame = pPane->m_Frame;
			m_pLeft->m_pOwn = this;
			m_pRight->m_pOwn = this;
			MoveParOwn(rect, PANEFRAME_NOCHNG);
			delete pPane;
		}
	}

	return this;
}
class CPaneFrame *CPaneFrame::GetPane(HWND hWnd)
{
	class CPaneFrame *pPane;

	if ( m_Style == PANEFRAME_WINDOW && m_hWnd == hWnd )
		return this;
	else if ( m_pLeft != NULL && (pPane = m_pLeft->GetPane(hWnd)) != NULL )
		return pPane;
	else if ( m_pRight != NULL && (pPane = m_pRight->GetPane(hWnd)) != NULL )
		return pPane;
	else
		return NULL;
}
class CPaneFrame *CPaneFrame::GetNull()
{
	CPaneFrame *pPane;

	if ( m_Style == PANEFRAME_WINDOW && m_hWnd == NULL && m_bActive )
		return this;
	else if ( m_pLeft != NULL && (pPane = m_pLeft->GetNull()) != NULL )
		return pPane;
	else if ( m_pRight != NULL && (pPane = m_pRight->GetNull()) != NULL )
		return pPane;
	else
		return NULL;
}
class CPaneFrame *CPaneFrame::GetEntry()
{
	CPaneFrame *pPane;

	if ( m_Style == PANEFRAME_WINDOW && m_hWnd == NULL && m_pServerEntry != NULL )
		return this;
	else if ( m_pLeft != NULL && (pPane = m_pLeft->GetEntry()) != NULL )
		return pPane;
	else if ( m_pRight != NULL && (pPane = m_pRight->GetEntry()) != NULL )
		return pPane;
	else
		return NULL;
}
class CPaneFrame *CPaneFrame::GetActive()
{
	CPaneFrame *pPane;
	CWnd *pTemp = m_pMain->MDIGetActive(NULL);

	if ( (pPane = GetNull()) != NULL )
		return pPane;
	else if ( pTemp != NULL && (pPane = GetPane(pTemp->m_hWnd)) != NULL )
		return pPane;
	else if ( (pPane = GetPane(NULL)) != NULL )
		return pPane;
	else
		return this;
}
int CPaneFrame::SetActive(HWND hWnd)
{
	if ( m_Style == PANEFRAME_WINDOW )
		return (m_hWnd == hWnd ? TRUE : FALSE);

	ASSERT(m_pLeft);
	ASSERT(m_pRight);

	if ( m_pLeft->SetActive(hWnd) )
		return TRUE;
	else if ( !m_pRight->SetActive(hWnd) )
		return FALSE;
	else if ( m_Style != PANEFRAME_MAXIM )
		return TRUE;

	CPaneFrame *pPane = m_pLeft;
	m_pLeft = m_pRight;
	m_pRight = pPane;

	return TRUE;
}
int CPaneFrame::IsOverLap(CPaneFrame *pPane)
{
	int n;
	CRect rect;

	if ( pPane == NULL || pPane == this )
		return (-1);

	if ( m_Style == PANEFRAME_WINDOW && rect.IntersectRect(m_Frame, pPane->m_Frame) )
		return 1;

	if ( m_pLeft != NULL && (n = m_pLeft->IsOverLap(pPane)) != 0 )
		return n;

	if ( m_pRight != NULL && (n = m_pRight->IsOverLap(pPane)) != 0 )
		return n;

	return 0;
}
void CPaneFrame::MoveFrame()
{
	ASSERT(m_Style == PANEFRAME_WINDOW);

	if ( m_hWnd != NULL ) {
		if ( m_NullWnd.m_hWnd != NULL )
			m_NullWnd.DestroyWindow();
		::SetWindowPos(m_hWnd, NULL, m_Frame.left - 1, m_Frame.top - 1, m_Frame.Width() - 1, m_Frame.Height() - 1,
			SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
	} else {
		CRect rect = m_Frame;
		m_pMain->AdjustRect(rect);
		if ( m_NullWnd.m_hWnd == NULL )
			m_NullWnd.Create(NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | (m_bActive ? SS_WHITEFRAME : SS_GRAYFRAME), rect, m_pMain);
		else {
			m_NullWnd.ModifyStyle(SS_WHITEFRAME | SS_GRAYFRAME, (m_bActive ? SS_WHITEFRAME : SS_GRAYFRAME), 0);
			m_NullWnd.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(),
				SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
	}
}
void CPaneFrame::MoveParOwn(CRect &rect, int Style)
{
	if ( m_Style == PANEFRAME_WINDOW ) {
		m_pMain->InvalidateRect(m_Frame);
		m_Frame = rect;
		MoveFrame();
	} else {
		ASSERT(m_pLeft);
		ASSERT(m_pRight);

		CRect left  = rect;
		CRect right = rect;
	
		switch(Style) {
		case PANEFRAME_NOCHNG:
		RECALC:
			if ( m_Style == PANEFRAME_WIDTH ) {
				left.right = left.left + m_pLeft->m_Frame.Width() * rect.Width() / m_Frame.Width();
				right.left = left.right + m_BoderSize;
			} else if ( m_Style == PANEFRAME_HEIGHT ) {
				left.bottom = left.top + m_pLeft->m_Frame.Height() * rect.Height() / m_Frame.Height();
				right.top   = left.bottom + m_BoderSize;
			}
			break;

		case PANEFRAME_MAXIM:
			m_Style = Style;
			break;

		case PANEFRAME_WIDTH:
			if ( m_Style != PANEFRAME_MAXIM )
				goto RECALC;
			m_Style = Style;
			left.right = left.left + (rect.Width() - m_BoderSize) / 2;
			right.left = left.right + m_BoderSize;
			break;

		case PANEFRAME_HEIGHT:
			if ( m_Style != PANEFRAME_MAXIM )
				goto RECALC;
			m_Style = Style;
			left.bottom = left.top + (rect.Height() - m_BoderSize) / 2;
			right.top   = left.bottom + m_BoderSize;
			break;
		}

		m_Frame = rect;
		m_pLeft->MoveParOwn(left, Style);
		m_pRight->MoveParOwn(right, Style);
	}
}
void CPaneFrame::HitActive(CPoint &po)
{
	if ( m_Style == PANEFRAME_WINDOW ) {
		if ( m_hWnd == NULL ) {
			BOOL bNew = (m_Frame.PtInRect(po) ? TRUE : FALSE);
			if ( m_bActive != bNew ) {
				m_bActive = bNew;
				MoveFrame();
			}
		} else
			m_bActive = FALSE;
	}

	if ( m_pLeft != NULL )
		m_pLeft->HitActive(po);

	if ( m_pRight != NULL )
		m_pRight->HitActive(po);
}
class CPaneFrame *CPaneFrame::HitTest(CPoint &po)
{
	switch(m_Style) {
	case PANEFRAME_WINDOW:
		if ( m_Frame.PtInRect(po) )
			return this;
		break;
	case PANEFRAME_WIDTH:
		if ( m_Frame.PtInRect(po) && po.x >= m_pLeft->m_Frame.right && po.x <= m_pRight->m_Frame.left )
			return this;
		break;
	case PANEFRAME_HEIGHT:
		if ( m_Frame.PtInRect(po) && po.y >= m_pLeft->m_Frame.bottom && po.y <= m_pRight->m_Frame.top )
			return this;
		break;
	}

	CPaneFrame *pPane;

	if ( m_pLeft != NULL && (pPane = m_pLeft->HitTest(po)) != NULL )
		return pPane;
	else if ( m_pRight != NULL && (pPane = m_pRight->HitTest(po)) != NULL )
		return pPane;
	else
		return NULL;
}
int CPaneFrame::BoderRect(CRect &rect)
{
	switch(m_Style) {
	case PANEFRAME_WIDTH:
		rect = m_Frame;
		rect.left  = m_pLeft->m_Frame.right - 1;
		rect.right = m_pRight->m_Frame.left + 1;
		break;
	case PANEFRAME_HEIGHT:
		rect = m_Frame;
		rect.top    = m_pLeft->m_Frame.bottom - 1;
		rect.bottom = m_pRight->m_Frame.top   + 1;
		break;
	default:
		return FALSE;
	}

	m_pMain->AdjustRect(rect);
	return TRUE;
}
void CPaneFrame::SetBuffer(CBuffer *buf)
{
	CStringA tmp;
	int sz = 0;
	CServerEntry *pEntry = NULL;

	if ( m_Style == PANEFRAME_WINDOW ) {
		tmp.Format("%d\t0\t", m_Style);
		if ( m_pServerEntry != NULL ) {
			delete m_pServerEntry;
			m_pServerEntry = NULL;
		}
		if ( m_hWnd != NULL ) {
			CChildFrame *pWnd = (CChildFrame *)(CWnd::FromHandlePermanent(m_hWnd));
			if ( pWnd != NULL ) {
				CRLoginDoc *pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument());
				if ( pDoc != NULL ) {
					pDoc->SetEntryProBuffer();
					pEntry = &(pDoc->m_ServerEntry);
					tmp.Format("%d\t0\t1\t", m_Style);
				}
			}
		}
		buf->PutStr(tmp);
		if ( pEntry != NULL )
			pEntry->SetBuffer(*buf);
		return;
	}

	ASSERT(m_pLeft);
	ASSERT(m_pRight);

	switch(m_Style) {
	case PANEFRAME_WIDTH:
		sz = m_pLeft->m_Frame.right * 1000 / m_Frame.Width();
		break;
	case PANEFRAME_HEIGHT:
		sz = m_pLeft->m_Frame.bottom * 1000 / m_Frame.Height();
		break;
	case PANEFRAME_MAXIM:
		sz = 100;
		break;
	}

	tmp.Format("%d\t%d\t", m_Style, sz);
	buf->PutStr(tmp);
	m_pLeft->SetBuffer(buf);
	m_pRight->SetBuffer(buf);
}
class CPaneFrame *CPaneFrame::GetBuffer(class CMainFrame *pMain, class CPaneFrame *pPane, class CPaneFrame *pOwn, CBuffer *buf)
{
	int Size;
	CStringA tmp;
	CStringArrayExt stra;

	if ( pPane == NULL )
		pPane = new CPaneFrame(pMain, NULL, pOwn);

	if ( buf->GetSize() < 4 )
		return pPane;
	buf->GetStr(tmp);
	if ( tmp.IsEmpty() )
		return pPane;
	stra.GetString(MbsToTstr(tmp));
	if ( stra.GetSize() < 2 )
		return pPane;
	pPane->m_Style = stra.GetVal(0);
	Size = stra.GetVal(1);

	if ( pPane->m_Style == PANEFRAME_WINDOW ) {
		if ( stra.GetSize() > 2 && stra.GetVal(2) == 1 ) {
			pPane->m_pServerEntry = new CServerEntry;
			pPane->m_pServerEntry->GetBuffer(*buf);
		}
		return pPane;
	}

	pPane->m_pLeft  = new CPaneFrame(pMain, NULL, pPane);
	pPane->m_pRight = new CPaneFrame(pMain, NULL, pPane);

	switch(pPane->m_Style) {
	case PANEFRAME_WIDTH:
		pPane->m_pLeft->m_Frame.right = pPane->m_Frame.Width() * Size / 1000;
		pPane->m_pRight->m_Frame.left = pPane->m_pLeft->m_Frame.right + pPane->m_BoderSize;
		break;
	case PANEFRAME_HEIGHT:
		pPane->m_pLeft->m_Frame.bottom = pPane->m_Frame.Height() * Size / 1000;
		pPane->m_pRight->m_Frame.top   = pPane->m_pLeft->m_Frame.bottom + pPane->m_BoderSize;
		break;
	}

	pPane->m_pLeft  = GetBuffer(pMain, pPane->m_pLeft,  pPane, buf);
	pPane->m_pRight = GetBuffer(pMain, pPane->m_pRight, pPane, buf);

	return pPane;
}

#ifdef	DEBUG
void CPaneFrame::Dump()
{
	static const char *style[] = { "NOCHNG", "MAXIM", "WIDTH", "HEIGHT", "WINDOW" };

	TRACE("hWnd=%x ", m_hWnd);
	TRACE("Style=%s ", style[m_Style]);
	TRACE("Frame=%d,%d,%d,%d ", m_Frame.left, m_Frame.top, m_Frame.right, m_Frame.bottom);
	TRACE("\n");

	if ( m_pLeft != NULL )
		m_pLeft->Dump();
	if ( m_pRight != NULL )
		m_pRight->Dump();
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CTimerObject

CTimerObject::CTimerObject()
{
	m_Id      = 0;
	m_Mode    = 0;
	m_pObject = NULL;
	m_pList   = NULL;
}
void CTimerObject::CallObject()
{
	ASSERT(m_pObject);

	switch(m_Mode & 007) {
	case TIMEREVENT_DOC:
		((CRLoginDoc *)(m_pObject))->OnDelayRecive((-1));
		break;
	case TIMEREVENT_SOCK:
		((CExtSocket *)(m_pObject))->OnTimer(m_Id);
		break;
	case TIMEREVENT_SCRIPT:
		((CScript *)(m_pObject))->OnTimer(m_Id);
		break;
	case TIMEREVENT_TEXTRAM:
		((CTextRam *)(m_pObject))->OnTimer(m_Id);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_COMMAND(ID_PANE_WSPLIT, OnPaneWsplit)
	ON_COMMAND(ID_PANE_HSPLIT, OnPaneHsplit)
	ON_COMMAND(ID_PANE_DELETE, OnPaneDelete)
	ON_COMMAND(ID_PANE_SAVE, OnPaneSave)
	ON_COMMAND(ID_WINDOW_CASCADE, OnWindowCascade)
	ON_COMMAND(ID_WINDOW_TILE_HORZ, OnWindowTileHorz)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_CASCADE, OnUpdateWindowCascade)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_TILE_HORZ, OnUpdateWindowTileHorz)
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_FILE_ALL_SAVE, OnFileAllSave)
	ON_MESSAGE(WM_SOCKSEL, OnWinSockSelect)
	ON_MESSAGE(WM_GETHOSTADDR, OnGetHostAddr)
	ON_MESSAGE(WM_ICONMSG, OnIConMsg)
	ON_MESSAGE(WM_THREADCMD, OnThreadMsg)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SOCK, OnUpdateIndicatorSock)
	ON_COMMAND(ID_FILE_ALL_LOAD, OnFileAllLoad)
	ON_WM_COPYDATA()
	ON_WM_ENTERMENULOOP()
	ON_WM_ACTIVATE()
	ON_COMMAND(ID_VIEW_SCROLLBAR, &CMainFrame::OnViewScrollbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SCROLLBAR, &CMainFrame::OnUpdateViewScrollbar)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ステータス ライン インジケータ
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
	ID_INDICATOR_SOCK,
};

// CMainFrame コンストラクション/デストラクション

CMainFrame::CMainFrame()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIconActive = AfxGetApp()->LoadIcon(IDI_ACTIVE);
	m_IconShow = FALSE;
	m_pTopPane = NULL;
	m_Frame.SetRectEmpty();
	m_pTrackPane = NULL;
	m_StatusString = "";
	m_LastPaneFlag = FALSE;
	m_ModifiedFlag = FALSE;
	m_TimerSeqId = TIMERID_TIMEREVENT;
	m_pTimerUsedId = NULL;
	m_pTimerFreeId = NULL;
	m_SleepStatus = 0;
	m_SleepTimer = 0;
	m_TransParValue = 255;
	m_TransParColor = RGB(0, 0, 0);
	m_SleepCount = 60;
	m_MenuHand = NULL;
	m_hMidiOut = NULL;
	m_MidiTimer = 0;
	m_InfoThreadCount = 0;
}

CMainFrame::~CMainFrame()
{
	if ( m_pTopPane != NULL )
		delete m_pTopPane;

	while ( m_pTimerUsedId != NULL )
		DelTimerEvent(m_pTimerUsedId->m_pObject);

	CTimerObject *tp;
	while ( (tp = m_pTimerFreeId) != NULL ) {
		m_pTimerFreeId = tp->m_pList;
		delete tp;
	}

	if ( m_MidiTimer != 0 )
		KillTimer(m_MidiTimer);

	CMidiQue *mp;
	while ( !m_MidiQue.IsEmpty() && (mp = m_MidiQue.RemoveHead()) != NULL )
		delete mp;

	if ( m_hMidiOut != NULL ) {
		midiOutReset(m_hMidiOut);
		midiOutClose(m_hMidiOut);
	}

	CMenuBitMap *pMap;
	for ( int n = 0 ; n < m_MenuMap.GetSize() ; n++ ) {
		if ( (pMap = (CMenuBitMap *)m_MenuMap[n]) == NULL )
			continue;
		pMap->m_Bitmap.DeleteObject();
		delete pMap;
	}

#ifndef	NOIPV6
	for ( int n = 0 ; m_InfoThreadCount > 0 && n < 10 ; n++ )
		Sleep(300);
#endif
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

//	ExDwmEnableWindow(m_hWnd);

	if ( !m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT,
			WS_CHILD | WS_VISIBLE | CBRS_TOP | /*CBRS_GRIPPER | */CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME) ) {
		TRACE0("Failed to create toolbar\n");
		return -1;      // 作成に失敗
	}

	CBitmap BitMap;
	BitMap.LoadBitmap(IDB_BITMAP1);
	m_ImageList[0].Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 10);
	m_ImageList[0].Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	BitMap.LoadBitmap(IDB_BITMAP2);
	m_ImageList[1].Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 10);
	m_ImageList[1].Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	BitMap.LoadBitmap(IDB_BITMAP3);
	m_ImageList[2].Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 10);
	m_ImageList[2].Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	m_wndToolBar.SetSizes(CSize(16+7, 16+8), CSize(16, 16));
	m_wndToolBar.SendMessage(TB_SETIMAGELIST,			0, (LPARAM)(m_ImageList[0].m_hImageList));
	m_wndToolBar.SendMessage(TB_SETHOTIMAGELIST,		0, (LPARAM)(m_ImageList[1].m_hImageList));
	m_wndToolBar.SendMessage(TB_SETDISABLEDIMAGELIST,	0, (LPARAM)(m_ImageList[2].m_hImageList));

	BitMap.LoadBitmap(IDB_BITMAP8);
	m_ImageGozi.Create(32, 32, ILC_COLOR24 | ILC_MASK, 28, 10);
	m_ImageGozi.Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();

	int n, i, id, x, y;
	CMenuBitMap *pMap;
	CDC dc[2];
	CBitmap tmpMap;
	CBitmap *pOld[2];

	x = GetSystemMetrics(SM_CXMENUCHECK);
	y = GetSystemMetrics(SM_CYMENUCHECK);

	dc[0].CreateCompatibleDC(NULL);
	tmpMap.CreateBitmap(16, 16, dc[0].GetDeviceCaps(PLANES), dc[0].GetDeviceCaps(BITSPIXEL), NULL);
	pOld[0] = dc[0].SelectObject(&tmpMap);

	dc[1].CreateCompatibleDC(NULL);
	dc[1].SetStretchBltMode(HALFTONE);

	for ( n = i = 0 ; n < m_wndToolBar.GetCount() ; n++ ) {
		if ( (id = m_wndToolBar.GetItemID(n)) == ID_SEPARATOR )
			continue;

		if ( (pMap = new CMenuBitMap) == NULL )
			continue;
		m_MenuMap.Add(pMap);

		pMap->m_Id = id;
		pMap->m_Bitmap.CreateBitmap(x, y, dc[1].GetDeviceCaps(PLANES), dc[1].GetDeviceCaps(BITSPIXEL), NULL);
		pOld[1] = dc[1].SelectObject(&(pMap->m_Bitmap));

		m_ImageList[0].DrawEx(&(dc[0]), i++, CPoint(0, 0), CSize(16, 16), GetSysColor(COLOR_MENU), CLR_DEFAULT, ILD_NORMAL);
		dc[1].StretchBlt(0, 0, x, y, &(dc[0]), 0, 0, 16, 16, SRCCOPY);
		dc[1].SelectObject(pOld[1]);
	}

	dc[0].SelectObject(pOld[0]);
	dc[0].DeleteDC();
	dc[1].DeleteDC();

	if ( !m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)) ) {
		TRACE0("Failed to create status bar\n");
		return -1;      // 作成に失敗
	}

	if ( !m_wndTabBar.Create(this, WS_VISIBLE| WS_CHILD|CBRS_TOP|WS_EX_WINDOWEDGE, IDC_MDI_TAB_CTRL_BAR) ) {
		TRACE("Failed to create tabbar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndTabBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	DockControlBar(&m_wndTabBar);

	if ( (AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("ToolBarStyle"), WS_VISIBLE) & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndToolBar, FALSE, 0);
	if ( (AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("StatusBarStyle"), WS_VISIBLE) & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndStatusBar, FALSE, 0);
	ShowControlBar(&m_wndTabBar, FALSE, 0);

	m_TransParValue = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("LayeredWindow"), 255);
	m_TransParColor = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("LayeredColor"), RGB(0, 0 ,0));
	SetTransPar(m_TransParColor, m_TransParValue, LWA_ALPHA | LWA_COLORKEY);

	CBuffer buf;
	((CRLoginApp *)AfxGetApp())->GetProfileBuffer(_T("MainFrame"), _T("Pane"), buf);
	m_pTopPane = CPaneFrame::GetBuffer(this, NULL, NULL, &buf);

	if ( (m_SleepCount = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("WakeUpSleep"), 0)) > 0 )
		m_SleepTimer = SetTimer(TIMERID_SLEEPMODE, 5000, NULL);

	m_ScrollBarFlag = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("VScroll"), TRUE);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.x  = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("x"), cs.x);
	cs.y  = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("y"), cs.y);
	cs.cx = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("cx"), cs.cx);
	cs.cy = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("cy"), cs.cy);

	return TRUE;
}

int CMainFrame::SetAsyncSelect(SOCKET fd, CExtSocket *pSock, long lEvent)
{
	ASSERT(pSock->m_Type >= 0 && pSock->m_Type < 10);

	if ( lEvent != 0 && WSAAsyncSelect(fd, GetSafeHwnd(), WM_SOCKSEL, lEvent) != 0 )
		return FALSE;

	((CRLoginApp *)AfxGetApp())->SetSocketIdle(pSock);

	for ( int n = 0 ; n < m_SocketParam.GetSize() ; n += 2 ) {
		if ( m_SocketParam[n] == (void *)fd ) {
			m_SocketParam[n + 1] = (void *)pSock;
			return TRUE;
		}
	}
	m_SocketParam.Add((void *)fd);
	m_SocketParam.Add(pSock);

	return TRUE;
}

void CMainFrame::DelAsyncSelect(SOCKET fd, CExtSocket *pSock, BOOL useWsa)
{
	if ( useWsa )
		WSAAsyncSelect(fd, GetSafeHwnd(), 0, 0);

	((CRLoginApp *)AfxGetApp())->DelSocketIdle(pSock);

	for ( int n = 0 ; n < m_SocketParam.GetSize() ; n += 2 ) {
		if ( m_SocketParam[n] == (void *)fd ) {
			m_SocketParam.RemoveAt(n, 2);
			break;
		}
	}
}

int CMainFrame::SetAsyncHostAddr(int mode, LPCTSTR pHostName, CExtSocket *pSock)
{
	HANDLE hGetHostAddr;
	CString *pStr = new CString(pHostName);
	char *pData = new char[MAXGETHOSTSTRUCT];

	memset(pData, 0, MAXGETHOSTSTRUCT);
	if ( (hGetHostAddr = WSAAsyncGetHostByName(GetSafeHwnd(), WM_GETHOSTADDR, TstrToMbs(pHostName), pData, MAXGETHOSTSTRUCT)) == (HANDLE)0 ) {
		CString errmsg;
		errmsg.Format(_T("GetHostByName Error '%s'"), pHostName);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
	}

	m_HostAddrParam.Add(hGetHostAddr);
	m_HostAddrParam.Add(pSock);
	m_HostAddrParam.Add(pStr);
	m_HostAddrParam.Add(pData);
	m_HostAddrParam.Add((void *)mode);

	return TRUE;
}

#ifndef	NOIPV6
typedef struct _addrinfo_param {
	CMainFrame		*pWnd;
	int				mode;
	CString			name;
	CString			port;
	ADDRINFOT		hint;
	int				ret;
} addrinfo_param;

static UINT AddrInfoThread(LPVOID pParam)
{
	ADDRINFOT *ai;
	addrinfo_param *ap = (addrinfo_param *)pParam;

	ap->ret = GetAddrInfo(ap->name, ap->port, &(ap->hint), &ai);

	if ( ap->pWnd->m_InfoThreadCount-- > 0 && ap->pWnd->m_hWnd != NULL )
		ap->pWnd->PostMessage(WM_GETHOSTADDR, (WPARAM)ap, (LPARAM)ai);

	return 0;
}
#endif

int CMainFrame::SetAsyncAddrInfo(int mode, LPCTSTR pHostName, int PortNum, void *pHint, CExtSocket *pSock)
{
#ifndef	NOIPV6
	addrinfo_param *ap;

	ap = new addrinfo_param;
	ap->pWnd = this;
	ap->mode = mode;
	ap->name = pHostName;
	ap->port.Format(_T("%d"), PortNum);
	ap->ret  = 1;
	memcpy(&(ap->hint), pHint, sizeof(ADDRINFOT));

	m_InfoThreadCount++;
	AfxBeginThread(AddrInfoThread, ap, THREAD_PRIORITY_NORMAL);

	m_HostAddrParam.Add(ap);
	m_HostAddrParam.Add(pSock);
	m_HostAddrParam.Add(NULL);
	m_HostAddrParam.Add(NULL);
	m_HostAddrParam.Add((void *)mode);
#endif
	return TRUE;
}

int CMainFrame::SetTimerEvent(int msec, int mode, void *pParam)
{
	CTimerObject *tp;

	DelTimerEvent(pParam);

	if ( m_pTimerFreeId == NULL ) {
		for ( int n = 0 ; n < 16 ; n++ ) {
			tp = new CTimerObject;
			tp->m_Id = m_TimerSeqId++;
			tp->m_pList = m_pTimerFreeId;
			m_pTimerFreeId = tp;
		}
	}
	tp = m_pTimerFreeId;
	m_pTimerFreeId = tp->m_pList;
	tp->m_pList = m_pTimerUsedId;
	m_pTimerUsedId = tp;

	SetTimer(tp->m_Id, msec, NULL);
	tp->m_Mode = mode;
	tp->m_pObject = pParam;

	return tp->m_Id;
}
void CMainFrame::DelTimerEvent(void *pParam)
{
	CTimerObject *tp, *bp;

	for ( tp = bp = m_pTimerUsedId ; tp != NULL ; ) {
		if ( tp->m_pObject == pParam ) {
			KillTimer(bp->m_Id);
			if ( tp == m_pTimerUsedId )
				m_pTimerUsedId = tp->m_pList;
			else
				bp->m_pList = tp->m_pList;
			FreeTimerEvent(tp);
			break;
		}
		bp = tp;
		tp = tp->m_pList;
	}
}
void CMainFrame::RemoveTimerEvent(CTimerObject *pObject)
{
	CTimerObject *tp;

	KillTimer(pObject->m_Id);

	if ( (tp = m_pTimerUsedId) == pObject )
		m_pTimerUsedId = pObject->m_pList;
	else {
		while ( tp != NULL ) {
			if ( tp->m_pList == pObject ) {
				tp->m_pList = pObject->m_pList;
				break;
			}
			tp = tp->m_pList;
		}
	}
}
void CMainFrame::FreeTimerEvent(CTimerObject *pObject)
{
	pObject->m_Mode    = 0;
	pObject->m_pObject = NULL;
	pObject->m_pList   = m_pTimerFreeId;
	m_pTimerFreeId     = pObject;
}
void CMainFrame::SetMidiEvent(int msec, DWORD msg)
{
	CMidiQue *qp;

	if ( m_hMidiOut == NULL && midiOutOpen(&m_hMidiOut, MIDIMAPPER, NULL, 0, CALLBACK_NULL) != MMSYSERR_NOERROR )
		return;

	if ( m_MidiQue.IsEmpty() && msec == 0 ) {
		midiOutShortMsg(m_hMidiOut, msg);
		return;
	}

	qp = new CMidiQue;
	qp->m_mSec = msec;
	qp->m_Msg  = msg;

	m_MidiQue.AddTail(qp);
	qp = m_MidiQue.GetHead();

	if ( m_MidiTimer == 0 )
		m_MidiTimer = SetTimer(TIMERID_MIDIEVENT, qp->m_mSec, NULL);
}
int CMainFrame::OpenServerEntry(CServerEntry &Entry)
{
	int n;
	CServerSelect dlg;
	CWnd *pTemp = MDIGetActive(NULL);
	CPaneFrame *pPane = NULL;

	dlg.m_pData = &m_ServerEntryTab;
	dlg.m_EntryNum = (-1);

	if ( m_LastPaneFlag && pTemp != NULL && m_pTopPane != NULL &&
			(pPane = m_pTopPane->GetPane(pTemp->m_hWnd)) != NULL && pPane->m_pServerEntry != NULL ) {
		Entry = *(pPane->m_pServerEntry);
		Entry.m_SaveFlag = FALSE;
		delete pPane->m_pServerEntry;
		pPane->m_pServerEntry = NULL;
		if ( m_pTopPane->GetEntry() != NULL )
			PostMessage(WM_COMMAND, ID_FILE_NEW, 0);
		else
			m_LastPaneFlag = FALSE;
		return TRUE;
	}
	m_LastPaneFlag = FALSE;

	for ( n = 0 ; n< m_ServerEntryTab.m_Data.GetSize() ; n++ ) {
		if ( m_ServerEntryTab.m_Data[n].m_CheckFlag ) {
			dlg.m_EntryNum = n;
			break;
		}
	}

	if ( dlg.m_EntryNum < 0 ) {
		CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
		if ( pApp->m_pCmdInfo != NULL && !pApp->m_pCmdInfo->m_Name.IsEmpty() ) {
			for ( n = 0 ; n< m_ServerEntryTab.m_Data.GetSize() ; n++ ) {
				if ( pApp->m_pCmdInfo->m_Name.Compare(m_ServerEntryTab.m_Data[n].m_EntryName) == 0 ) {
					Entry = m_ServerEntryTab.m_Data[n];
					Entry.m_SaveFlag = FALSE;
					return TRUE;
				}
			}
		}
		if ( pApp->m_pCmdInfo != NULL && pApp->m_pCmdInfo->m_Proto != (-1) && !pApp->m_pCmdInfo->m_Addr.IsEmpty() && !pApp->m_pCmdInfo->m_Port.IsEmpty() ) {
			if ( Entry.m_EntryName.IsEmpty() )
				Entry.m_EntryName.Format(_T("%s:%s"), pApp->m_pCmdInfo->m_Addr, pApp->m_pCmdInfo->m_Port);
			Entry.m_SaveFlag = FALSE;
			return TRUE;
		}

		if ( dlg.DoModal() != IDOK || dlg.m_EntryNum < 0 )
			return FALSE;
	}

	m_ServerEntryTab.m_Data[dlg.m_EntryNum].m_CheckFlag = FALSE;
	Entry = m_ServerEntryTab.m_Data[dlg.m_EntryNum];
	Entry.m_SaveFlag = TRUE;

	for ( n = 0 ; n < m_ServerEntryTab.m_Data.GetSize() ; n++ ) {
		if ( m_ServerEntryTab.m_Data[n].m_CheckFlag ) {
			PostMessage(WM_COMMAND, ID_FILE_NEW, 0);
			break;
		}
	}
	return TRUE;
}
void CMainFrame::SetTransPar(COLORREF rgb, int value, DWORD flag)
{
	if ( (flag & LWA_COLORKEY) != 0 && rgb == 0 )
		flag &= ~LWA_COLORKEY;
	else if ( m_TransParColor != 0 ) {
		rgb = m_TransParColor;
		flag |= LWA_COLORKEY;
	}

	if ( (flag & LWA_ALPHA) != 0 && value == 255 )
		flag &= ~LWA_ALPHA;

	//rgb = RGB(1, 1, 1);
	//value = 196;
	//flag = LWA_ALPHA | LWA_COLORKEY;

	if ( flag == 0 )
		ModifyStyleEx(WS_EX_LAYERED, 0);
	else
		ModifyStyleEx(0, WS_EX_LAYERED);

	SetLayeredWindowAttributes(rgb, value, flag);

	Invalidate(TRUE);
}
void CMainFrame::SetWakeUpSleep(int sec)
{
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("WakeUpSleep"), sec);

	if ( sec > 0 && m_SleepTimer == 0 )
		m_SleepTimer = SetTimer(TIMERID_SLEEPMODE, 5000, NULL);
	else if ( sec == 0 && m_SleepTimer != 0 ) {
		KillTimer(m_SleepTimer);
		if ( m_SleepStatus >= sec )
			SetTransPar(0, m_TransParValue, LWA_ALPHA);
		m_SleepTimer = 0;
	}
	m_SleepStatus = 0;
	m_SleepCount = sec;
}
void CMainFrame::WakeUpSleep()
{
	if ( m_SleepStatus == 0 )
		return;
	else if ( m_SleepStatus >= m_SleepCount ) {
		SetTransPar(0, m_TransParValue, LWA_ALPHA);
		m_SleepTimer = SetTimer(TIMERID_SLEEPMODE, 5000, NULL);
	}
	m_SleepStatus = 0;
}

void CMainFrame::SetIconStyle()
{
	if ( m_IconShow ) {
		ShowWindow(SW_RESTORE);
		Shell_NotifyIcon(NIM_DELETE, &m_IconData);
		m_IconShow = FALSE;
		return;
	}

	ZeroMemory(&m_IconData, sizeof(NOTIFYICONDATA));
	m_IconData.cbSize = sizeof(NOTIFYICONDATA);

	m_IconData.hWnd   = m_hWnd;
	m_IconData.uID    = 1000;
	m_IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_IconData.uCallbackMessage = WM_ICONMSG;
	m_IconData.hIcon  = m_hIcon;
	_tcscpy(m_IconData.szTip, _T("RLogin"));

	if ( (m_IconShow = Shell_NotifyIcon(NIM_ADD, &m_IconData)) )
		ShowWindow(SW_HIDE);
}
void CMainFrame::SetIconData(HICON hIcon, LPCTSTR str)
{
	if ( m_IconShow == FALSE )
		return;
	if ( hIcon != NULL )
		m_IconData.hIcon  = hIcon;
	if ( str != NULL )
		_tcsncpy(m_IconData.szTip, str, sizeof(m_IconData.szTip) / sizeof(TCHAR));
	Shell_NotifyIcon(NIM_MODIFY, &m_IconData);
}

BOOL CMainFrame::IsConnectChild(CPaneFrame *pPane)
{
	CChildFrame *pWnd;
	CRLoginDoc *pDoc;

	if ( pPane == NULL )
		return FALSE;

	if ( IsConnectChild(pPane->m_pLeft) )
		return TRUE;

	if ( IsConnectChild(pPane->m_pRight) )
		return TRUE;

	if ( pPane->m_Style != PANEFRAME_WINDOW || pPane->m_hWnd == NULL )
		return FALSE;

	if ( (pWnd = (CChildFrame *)(CWnd::FromHandlePermanent(pPane->m_hWnd))) == NULL )
		return FALSE;

	if ( (pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument())) == NULL )
		return FALSE;

	if ( pDoc->m_pSock != NULL )
		return TRUE;

	return FALSE;
}
void CMainFrame::AddChild(CWnd *pWnd)
{
	if ( m_wndTabBar.m_TabCtrl.GetItemCount() >= 1 )
		ShowControlBar(&m_wndTabBar, TRUE, TRUE);
	m_wndTabBar.Add(pWnd);

	CPaneFrame *pPane;

	if ( m_pTopPane == NULL ) {
		m_pTopPane = new CPaneFrame(this, pWnd->m_hWnd, NULL);
		m_pTopPane->MoveFrame();
	} else if ( (pPane = m_pTopPane->GetNull()) != NULL ) {
		pPane->Attach(pWnd->m_hWnd);
		pPane->MoveFrame();
	} else if ( (pPane = m_pTopPane->GetEntry()) != NULL ) {
		pPane->Attach(pWnd->m_hWnd);
		pPane->MoveFrame();
	} else if ( (pPane = m_pTopPane->GetPane(NULL)) != NULL ) {
		pPane->Attach(pWnd->m_hWnd);
		pPane->MoveFrame();
	} else {
		CWnd *pTemp = MDIGetActive(NULL);
		pPane = m_pTopPane->GetPane(pTemp->m_hWnd);
		pPane->CreatePane(PANEFRAME_MAXIM, pWnd->m_hWnd);
	}
}
void CMainFrame::RemoveChild(CWnd *pWnd)
{
	m_wndTabBar.Remove(pWnd);
	if ( m_wndTabBar.m_TabCtrl.GetItemCount() <= 1 )
		ShowControlBar(&m_wndTabBar, FALSE, TRUE);

//	if ( m_pTopPane != NULL )
//		m_pTopPane = m_pTopPane->DeletePane(pWnd->m_hWnd);

	if ( m_pTopPane != NULL ) {
		CPaneFrame *pPane = m_pTopPane->GetPane(pWnd->m_hWnd);
		if ( pPane != NULL ) {
			if ( pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM ) {
				m_pTopPane = m_pTopPane->DeletePane(pWnd->m_hWnd);
			} else {
				pPane->m_hWnd = NULL;
				pPane->MoveFrame();
			}
		}
	}
}
void CMainFrame::ActiveChild(CWnd *pWnd)
{
	if ( m_pTopPane == NULL )
		return;
	m_pTopPane->SetActive(pWnd->m_hWnd);
#ifdef	DEBUG
	m_pTopPane->Dump();
#endif
}
void CMainFrame::MoveChild(CWnd *pWnd, CPoint point)
{
	if ( m_pTopPane == NULL )
		return;

	point.y -= m_Frame.top;
	CPaneFrame *pThis = m_pTopPane->GetPane(pWnd->m_hWnd);
	CPaneFrame *pPane = m_pTopPane->HitTest(point);

	if ( pThis == NULL || pPane == NULL || pPane->m_Style != PANEFRAME_WINDOW || pPane->m_hWnd == pWnd->m_hWnd )
		return;

	pThis->m_hWnd = pPane->m_hWnd;
	pThis->MoveFrame();
	pPane->m_hWnd = pWnd->m_hWnd;
	pPane->MoveFrame();
}

BOOL CMainFrame::IsOverLap(HWND hWnd)
{
	CPaneFrame *pPane;

	if ( m_pTopPane == NULL || (pPane = m_pTopPane->GetPane(hWnd)) == NULL )
		return FALSE;
	return (m_pTopPane->IsOverLap(pPane) == 1 ? TRUE : FALSE);
}
void CMainFrame::GetFrameRect(CRect &frame)
{
	if ( m_Frame.IsRectEmpty() )
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);
	frame.SetRect(0, 0, m_Frame.Width(), m_Frame.Height());
}
void CMainFrame::AdjustRect(CRect &rect)
{
	if ( m_Frame.IsRectEmpty() )
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);
	rect.top    += m_Frame.top;
	rect.bottom += m_Frame.top;
}

// CMainFrame 診断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame メッセージ ハンドラ

LRESULT CMainFrame::OnWinSockSelect(WPARAM wParam, LPARAM lParam)
{
	int	fs = WSAGETSELECTEVENT(lParam);
	CExtSocket *pSock = NULL;

	for ( int n = 0 ; n < m_SocketParam.GetSize() ; n += 2 ) {
		if ( m_SocketParam[n] == (void *)wParam ) {
			pSock = (CExtSocket *)(m_SocketParam[n + 1]);
			break;
		}
	}

	if ( pSock == NULL )
		return TRUE;

	ASSERT(pSock->m_Type >= 0 && pSock->m_Type < 10 );

	if( WSAGETSELECTERROR(lParam) != 0 ) {
		pSock->OnError(WSAGETSELECTERROR(lParam));
		return TRUE;
	}

	if ( (fs & FD_CONNECT) != 0 )
		pSock->OnPreConnect();
	if ( (fs & FD_ACCEPT) != 0 )
		pSock->OnAccept((SOCKET)wParam);
	if ( (fs & FD_READ) != 0 )
		pSock->OnRecive(0);
	if ( (fs & FD_OOB) != 0 )
		pSock->OnRecive(MSG_OOB);
	if ( (fs & FD_WRITE) != 0 )
		pSock->OnSend();
	if ( (fs & FD_CLOSE) != 0 )
		pSock->OnPreClose();

	//if ( (fs & FD_RECIVE_EMPTY) != 0 )
	//	pSock->OnRecvEmpty();
	//if ( (fs & FD_SEND_EMPTY) != 0 )
	//	pSock->OnSendEmpty();

	return TRUE;
}

LRESULT CMainFrame::OnGetHostAddr(WPARAM wParam, LPARAM lParam)
{
	int n;
	CExtSocket *pSock;
	CString *pStr;
	struct hostent *hp;
	int mode;
	struct sockaddr_in in;
	int errcode = WSAGETASYNCERROR(lParam);
	int buflen  = WSAGETASYNCBUFLEN(lParam);

	for ( n = 0 ; n < m_HostAddrParam.GetSize() ; n += 5 ) {
		mode = (int)m_HostAddrParam[n + 4];
		if ( (mode & 030) == 0 && m_HostAddrParam[n] == (void *)wParam ) {
			pSock = (CExtSocket *)m_HostAddrParam[n + 1];
			pStr = (CString *)m_HostAddrParam[n + 2];
			hp = (struct hostent *)m_HostAddrParam[n + 3];

			if ( errcode == 0 ) {
				memset(&in, 0, sizeof(in));
				in.sin_family = hp->h_addrtype;
				memcpy(&(in.sin_addr), hp->h_addr, (hp->h_length < sizeof(in.sin_addr) ? hp->h_length : sizeof(in.sin_addr)));
				pSock->GetHostName((struct sockaddr *)&in, sizeof(in), *pStr);
			}

			pSock->OnAsyncHostByName(mode, *pStr);

			m_HostAddrParam.RemoveAt(n, 5);
			delete pStr;
			delete hp;
			break;
#ifndef	NOIPV6
		} else if ( (mode & 030) == 010 && m_HostAddrParam[n] == (void *)wParam ) {
			addrinfo_param *ap = (addrinfo_param *)wParam;
			ADDRINFOT *info = (ADDRINFOT *)lParam;
			pSock = (CExtSocket *)m_HostAddrParam[n + 1];

			if ( ap->ret == 0 )
				pSock->OnAsyncHostByName(mode, (LPCTSTR)info);
			else
				pSock->OnAsyncHostByName(mode & 003, ap->name);

			m_HostAddrParam.RemoveAt(n, 5);
			delete ap;
			break;
#endif
		}
	}
	return TRUE;
}

LRESULT CMainFrame::OnIConMsg(WPARAM wParam, LPARAM lParam)
{
	switch(lParam) {
	case WM_LBUTTONDBLCLK:
		ShowWindow(SW_RESTORE);
		Shell_NotifyIcon(NIM_DELETE, &m_IconData);
		m_IconShow = FALSE;
		break;
	}
	return FALSE;
}

LRESULT CMainFrame::OnThreadMsg(WPARAM wParam, LPARAM lParam)
{
	CSyncSock *pSp = (CSyncSock *)lParam;
	pSp->ThreadCommand((int)wParam);
	return TRUE;
}

void CMainFrame::OnDestroy() 
{
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("ToolBarStyle"),	m_wndToolBar.GetStyle());
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("StatusBarStyle"), m_wndStatusBar.GetStyle());

	if ( !IsIconic() && !IsZoomed() ) {
		CRect rect;
		GetWindowRect(&rect);
		AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("x"), rect.left);
		AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("y"), rect.top);
		AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("cx"), rect.Width());
		AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("cy"), rect.Height());
	}

	CMDIFrameWnd::OnDestroy();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent) 
{
	CTimerObject *tp;
	CMidiQue *mp;

	if ( nIDEvent == m_SleepTimer ) {
		if ( m_SleepStatus < m_SleepCount ) {
			m_SleepStatus += 5;
		} else if ( m_SleepStatus == m_SleepCount ) {
			m_SleepStatus++;
			m_SleepTimer = SetTimer(TIMERID_SLEEPMODE, 100, NULL);
		} else if ( m_SleepStatus < (m_SleepCount + 18) ) {
			m_SleepStatus++;
			SetTransPar(0, m_TransParValue * (m_SleepCount + 20 - m_SleepStatus) / 20, LWA_ALPHA);
		} else if ( m_SleepStatus == (m_SleepCount + 18) ) {
			m_SleepStatus++;
			KillTimer(nIDEvent);
			m_SleepTimer = 0;
		}
	} else if ( nIDEvent == m_MidiTimer ) {
		KillTimer(nIDEvent);
		m_MidiTimer = 0;
		if ( !m_MidiQue.IsEmpty() && (mp = m_MidiQue.RemoveHead()) != NULL ) {
			if ( m_hMidiOut != NULL )
				midiOutShortMsg(m_hMidiOut, mp->m_Msg);
			delete mp;
		}
		while ( !m_MidiQue.IsEmpty() && (mp = m_MidiQue.GetHead()) != NULL && mp->m_mSec == 0 ) {
			if ( m_hMidiOut != NULL )
				midiOutShortMsg(m_hMidiOut, mp->m_Msg);
			m_MidiQue.RemoveHead();
			delete mp;
		}
		if ( !m_MidiQue.IsEmpty() && (mp = m_MidiQue.GetHead()) != NULL )
			m_MidiTimer = SetTimer(TIMERID_MIDIEVENT, mp->m_mSec, NULL);

	} else {
		for ( tp = m_pTimerUsedId ; tp != NULL ; tp = tp->m_pList ) {
			if ( tp->m_Id == (int)nIDEvent ) {
				if ( (tp->m_Mode & 030) == 000 ) {
					RemoveTimerEvent(tp);
					tp->CallObject();
					FreeTimerEvent(tp);
				} else
					tp->CallObject();
				break;
			}
		}
		if ( tp == NULL )
			KillTimer(nIDEvent);
	}
	CMDIFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::RecalcLayout(BOOL bNotify) 
{
	CMDIFrameWnd::RecalcLayout(bNotify);
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);
	if ( m_pTopPane == NULL )
		return;
	CRect rect;
	GetFrameRect(rect);
	m_pTopPane->MoveParOwn(rect, PANEFRAME_NOCHNG);
}

void CMainFrame::OnPaneWsplit() 
{
	if ( m_pTopPane == NULL )
		m_pTopPane = new CPaneFrame(this, NULL, NULL);

	CPaneFrame *pPane = m_pTopPane->GetActive();

	if ( pPane->m_pOwn == NULL || pPane->m_pOwn->m_Style != PANEFRAME_MAXIM )
		pPane->CreatePane(PANEFRAME_WIDTH, NULL);
	else {
		while ( pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM )
			pPane = pPane->m_pOwn;
		pPane->MoveParOwn(pPane->m_Frame, PANEFRAME_WIDTH);
	}
}
void CMainFrame::OnPaneHsplit() 
{
	if ( m_pTopPane == NULL )
		m_pTopPane = new CPaneFrame(this, NULL, NULL);

	CPaneFrame *pPane = m_pTopPane->GetActive();

	if ( pPane->m_pOwn == NULL || pPane->m_pOwn->m_Style != PANEFRAME_MAXIM )
		pPane->CreatePane(PANEFRAME_HEIGHT, NULL);
	else {
		while ( pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM )
			pPane = pPane->m_pOwn;
		pPane->MoveParOwn(pPane->m_Frame, PANEFRAME_HEIGHT);
	}
}
void CMainFrame::OnPaneDelete() 
{
	if ( m_pTopPane == NULL )
		return;

	CPaneFrame *pPane = m_pTopPane->GetActive();

	while ( pPane->m_pOwn != NULL ) {
		pPane = pPane->m_pOwn;
		if ( pPane->m_pLeft != NULL && pPane->m_pLeft->m_hWnd == NULL && pPane->m_pLeft->m_pLeft == NULL )
			pPane->DeletePane(NULL);
		if ( pPane->m_pRight != NULL && pPane->m_pRight->m_hWnd == NULL && pPane->m_pRight->m_pLeft == NULL )
			pPane->DeletePane(NULL);
		if ( pPane->m_Style != PANEFRAME_MAXIM ) {
			pPane->MoveParOwn(pPane->m_Frame, PANEFRAME_MAXIM);
			break;
		}
	}
}
void CMainFrame::OnPaneSave() 
{
	if ( m_pTopPane == NULL )
		return;

	CBuffer buf;
	m_pTopPane->SetBuffer(&buf);
	((CRLoginApp *)AfxGetApp())->WriteProfileBinary(_T("MainFrame"), _T("Pane"), buf.GetPtr(), buf.GetSize());
}

void CMainFrame::OnWindowCascade() 
{
	while ( m_pTopPane != NULL && m_pTopPane->GetPane(NULL) != NULL )
		m_pTopPane = m_pTopPane->DeletePane(NULL);

	if ( m_pTopPane == NULL )
		return;

	CRect rect;
	GetFrameRect(rect);
	m_pTopPane->MoveParOwn(rect, PANEFRAME_MAXIM);
}
void CMainFrame::OnWindowTileHorz() 
{
	while ( m_pTopPane != NULL && m_pTopPane->GetPane(NULL) != NULL )
		m_pTopPane = m_pTopPane->DeletePane(NULL);

	if ( m_pTopPane == NULL )
		return;

	CRect rect;
	GetFrameRect(rect);
	m_pTopPane->MoveParOwn(rect, PANEFRAME_HEIGHT);
}
void CMainFrame::OnUpdateWindowCascade(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pTopPane != NULL && m_pTopPane->m_Style != PANEFRAME_WINDOW ? TRUE : FALSE);
}
void CMainFrame::OnUpdateWindowTileHorz(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pTopPane != NULL && m_pTopPane->m_Style != PANEFRAME_WINDOW ? TRUE : FALSE);
}

BOOL CMainFrame::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	CPoint point;
	CPaneFrame *pPane;

	GetCursorPos(&point);
	ScreenToClient(&point);
	point.y -= m_Frame.top;

	if ( m_pTopPane != NULL && (pPane = m_pTopPane->HitTest(point)) != NULL && pPane->m_Style != PANEFRAME_WINDOW ) {
		::SetCursor(::LoadCursor(NULL, (pPane->m_Style == PANEFRAME_HEIGHT ? IDC_SIZENS : IDC_SIZEWE)));
		return TRUE;
	}
	return CMDIFrameWnd::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	if ( pMsg->message == WM_LBUTTONDOWN ) {
		CPoint point(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
		::ClientToScreen(pMsg->hwnd, &point);
		ScreenToClient(&point);
		if ( PreLButtonDown((UINT)pMsg->wParam, point) )
			return TRUE;
	}
	return CMDIFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OffsetTrack(CPoint point)
{
	CRect rect = m_pTrackPane->m_Frame;
	AdjustRect(rect);

	point.y -= m_Frame.top;

	if ( m_pTrackPane->m_Style == PANEFRAME_WIDTH ) {
		m_TrackRect += CPoint(point.x - m_TrackPoint.x, 0);
		int w = m_TrackRect.Width();
		if ( m_TrackRect.left < (rect.left + 16) ) {
			m_TrackRect.left = rect.left + 16;
			m_TrackRect.right = m_TrackRect.left + w;
		} else if ( m_TrackRect.right > (rect.right - 32) ) {
			m_TrackRect.right = rect.right - 32;
			m_TrackRect.left = m_TrackRect.right - w;
		}
	} else {
		m_TrackRect += CPoint(0, point.y - m_TrackPoint.y);
		int h = m_TrackRect.Height();
		if ( m_TrackRect.top < (rect.top + 16) ) {
			m_TrackRect.top = rect.top + 16;
			m_TrackRect.bottom = m_TrackRect.top + h;
		} else if ( m_TrackRect.bottom > (rect.bottom - 16) ) {
			m_TrackRect.bottom = rect.bottom - 16;
			m_TrackRect.top = m_TrackRect.bottom - h;
		}
	}

	m_TrackPoint = point;
}
void CMainFrame::InvertTracker(CRect &rect)
{
	CDC* pDC = GetDC();
	CBrush* pBrush = CDC::GetHalftoneBrush();
	HBRUSH hOldBrush = NULL;

	if (pBrush != NULL)
		hOldBrush = (HBRUSH)SelectObject(pDC->m_hDC, pBrush->m_hObject);

	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);

	if (hOldBrush != NULL)
		SelectObject(pDC->m_hDC, hOldBrush);

	ReleaseDC(pDC);
}
int CMainFrame::PreLButtonDown(UINT nFlags, CPoint point)
{
	CPaneFrame *pPane;

	point.y -= m_Frame.top;
	if ( m_pTrackPane != NULL || m_pTopPane == NULL )
		return FALSE;

	if ( (pPane = m_pTopPane->HitTest(point)) == NULL )
		return FALSE;

	if ( pPane->m_Style == PANEFRAME_WINDOW ) {
		m_pTopPane->HitActive(point);
		return FALSE;
	}

	SetCapture();
	m_pTrackPane = pPane;
	m_pTrackPane->BoderRect(m_TrackRect);
	InvertTracker(m_TrackRect);
	m_TrackPoint = point;
	return TRUE;
}
void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CMDIFrameWnd::OnLButtonUp(nFlags, point);

	if ( m_pTrackPane == NULL )
		return;
	InvertTracker(m_TrackRect);
	OffsetTrack(point);
	ReleaseCapture();

	m_TrackRect.top    -= m_Frame.top;
	m_TrackRect.bottom -= m_Frame.top;

	if ( m_pTrackPane->m_Style == PANEFRAME_WIDTH ) {
		m_pTrackPane->m_pLeft->m_Frame.right = m_TrackRect.left  + 1;
		m_pTrackPane->m_pRight->m_Frame.left = m_TrackRect.right - 1;
	} else {
		m_pTrackPane->m_pLeft->m_Frame.bottom = m_TrackRect.top    + 1;
		m_pTrackPane->m_pRight->m_Frame.top   = m_TrackRect.bottom - 1;
	}

	m_pTrackPane->MoveParOwn(m_pTrackPane->m_Frame, PANEFRAME_NOCHNG);
	m_pTrackPane = NULL;
}

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point) 
{
	CMDIFrameWnd::OnMouseMove(nFlags, point);

	if ( m_pTrackPane == NULL )
		return;
	InvertTracker(m_TrackRect);
	OffsetTrack(point);
	InvertTracker(m_TrackRect);
}

void CMainFrame::OnUpdateIndicatorSock(CCmdUI* pCmdUI)
{
//	m_wndStatusBar.GetStatusBarCtrl().SetIcon(pCmdUI->m_nIndex, AfxGetApp()->LoadIcon(IDI_LOCKICON));
	pCmdUI->SetText(m_StatusString);
}

void CMainFrame::OnFileAllSave() 
{
	if ( m_pTopPane == NULL || MDIGetActive(NULL) == NULL )
		return;

	CFile file;
	CBuffer buf;
	CFileDialog dlg(FALSE, _T("rlg"), m_AllFilePath, OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGRLOGIN), this);

	if ( dlg.DoModal() != IDOK )
		return;

	m_pTopPane->SetBuffer(&buf);

	if ( !file.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite) )
		return;

	file.Write("RLM100\n", 7);
	file.Write(buf.GetPtr(), buf.GetSize());
	file.Close();
	m_ModifiedFlag = FALSE;
}
void CMainFrame::OnFileAllLoad() 
{
	CPaneFrame *pPane;

	if ( IsConnectChild(m_pTopPane) ) {
		if ( MessageBox(CStringLoad(IDE_ALLCLOSEREQ), _T("Warning"), MB_ICONQUESTION | MB_OKCANCEL) != IDOK )
			return;
	}
	
	if ( (pPane = CPaneFrame::GetBuffer(this, NULL, NULL, &m_AllFileBuf)) == NULL )
		return;

	AfxGetApp()->CloseAllDocuments(FALSE);

	if ( m_pTopPane != NULL )
		delete m_pTopPane;
	m_pTopPane = pPane;

	if ( m_pTopPane->GetEntry() != NULL ) {
		m_LastPaneFlag = TRUE;
		m_ModifiedFlag = FALSE;
		AfxGetApp()->AddToRecentFileList(m_AllFilePath);
		PostMessage(WM_COMMAND, ID_FILE_NEW, 0);
	}
}
BOOL CMainFrame::SaveModified( )
{
	if ( !m_ModifiedFlag || m_AllFilePath.IsEmpty() )
		return TRUE;

	CFile file;
	CBuffer buf;
	CString prompt;

	AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, m_AllFilePath);

	switch(AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE)) {
	case IDCANCEL:
		break;
	case IDYES:
		m_pTopPane->SetBuffer(&buf);
		if ( !file.Open(m_AllFilePath, CFile::modeCreate | CFile::modeWrite) )
			return FALSE;
		file.Write("RLM100\n", 7);
		file.Write(buf.GetPtr(), buf.GetSize());
		file.Close();
		m_ModifiedFlag = FALSE;
		break;
	case IDNO:
		return FALSE;
	}

	return TRUE;
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if ( pCopyDataStruct->dwData == 0x524c4f31 ) {
		CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
		CCommandLineInfoEx cmdInfo;
		cmdInfo.SetString((LPCTSTR)(pCopyDataStruct->lpData));
		pApp->OpenProcsCmd(&cmdInfo);
		return TRUE;
	}
	return CMDIFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}
void CMainFrame::OnEnterMenuLoop(BOOL bIsTrackPopupMenu)
{
	int n, i;
	CMenu *pMenu, Save;
	CChildFrame *pWnd;
	CRLoginDoc *pDoc;
	CString str, tmp;
	CKeyCmds *pCmds;
	CMenuBitMap *pMap;

	if ( (pMenu = GetMenu()) == NULL )
		return;

	if ( m_MenuHand != pMenu->GetSafeHmenu() ) {
		if ( m_MenuHand != NULL ) {
			Save.Attach(m_MenuHand);
			m_MenuTab.ResetMenuAll(&Save);
			Save.Detach();
		}
		m_MenuTab.RemoveAll();
		m_MenuHand = pMenu->GetSafeHmenu();
	}

	if ( (pWnd = (CChildFrame *)(MDIGetActive())) == NULL || (pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument())) == NULL ) {
		m_MenuTab.ResetMenuAll(pMenu);
		m_MenuTab.RemoveAll();
		return;
	}

	for ( n = 0 ; n < m_MenuTab.GetSize() ; n++ )
		m_MenuTab[n].m_Flag = FALSE;

	pDoc->m_KeyTab.CmdsInit();
	for ( n = 0 ; n < pDoc->m_KeyTab.m_Cmds.GetSize() ; n++ ) {
		pCmds = &(pDoc->m_KeyTab.m_Cmds[n]);
		if ( (i = m_MenuTab.Find(pCmds->m_Id)) >= 0 ) {
			m_MenuTab[i].m_Flag = TRUE;
			if ( pCmds->m_Id >= ID_MACRO_HIS1 && pCmds->m_Id <= ID_MACRO_HIS5 ) {
				if ( pMenu->GetMenuString(pCmds->m_Id, tmp, MF_BYCOMMAND) <= 0 )
					continue;
				str.Format(_T("%s\t%s"), m_MenuTab[i].m_Text, pCmds->m_Menu);
				if ( str.Compare(tmp) == 0 )
					continue;
				m_MenuTab[i].m_Text = tmp;
			} else if ( m_MenuTab[i].m_Menu.Compare(pCmds->m_Menu) == 0 )
				continue;
			m_MenuTab[i].m_Menu = pCmds->m_Menu;
		} else {
			if ( pMenu->GetMenuString(pCmds->m_Id, str, MF_BYCOMMAND) <= 0 )
				continue;
			if ( (i = str.Find(_T('\t'))) >= 0 )
				str.Truncate(i);
			pCmds->m_Flag = TRUE;
			pCmds->m_Text = str;
			i = m_MenuTab.Add(*pCmds);
		}
		str.Format(_T("%s\t%s"), m_MenuTab[i].m_Text, m_MenuTab[i].m_Menu);
		pMenu->ModifyMenu(pCmds->m_Id, MF_BYCOMMAND | MF_STRING, pCmds->m_Id, str);
	}

	for ( n = 0 ; n < m_MenuTab.GetSize() ; n++ ) {
		if ( m_MenuTab[n].m_Flag )
			continue;
		m_MenuTab[n].ResetMenu(pMenu);
		m_MenuTab.RemoveAt(n);
		n--;
	}

	for ( n = 0 ; n < m_MenuMap.GetSize() ; n++ ) {
		if ( (pMap = (CMenuBitMap *)m_MenuMap[n]) != NULL )
			pMenu->SetMenuItemBitmaps(pMap->m_Id, MF_BYCOMMAND, &(pMap->m_Bitmap), NULL);
	}
}

void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CMDIFrameWnd::OnActivate(nState, pWndOther, bMinimized);

	if ( nState == WA_INACTIVE )
		m_wndTabBar.SetGhostWnd(FALSE);
}

void CMainFrame::OnViewScrollbar()
{
	CWinApp *pApp;

	if ( (pApp = AfxGetApp()) == NULL )
		return;
	
	m_ScrollBarFlag = (pApp->GetProfileInt(_T("ChildFrame"), _T("VScroll"), TRUE) ? FALSE : TRUE);

	POSITION pos = pApp->GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = pApp->GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
			POSITION vpos = pDoc->GetFirstViewPosition();
			while ( vpos != NULL ) {
				CRLoginView *pView = (CRLoginView *)pDoc->GetNextView(vpos);
				if ( pView != NULL ) {
					CChildFrame *pChild = pView->GetFrameWnd();
					if ( pChild != NULL )
						pChild->SetScrollBar(m_ScrollBarFlag);
				}
			}
		}
	}

	pApp->WriteProfileInt(_T("ChildFrame"), _T("VScroll"), m_ScrollBarFlag);
}

void CMainFrame::OnUpdateViewScrollbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_ScrollBarFlag);
}
