// RLoginView.cpp : CRLoginView クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "ExtSocket.h"
#include "Ssh.h"
#include "Data.h"

#include <imm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRLoginView

IMPLEMENT_DYNCREATE(CRLoginView, CView)

BEGIN_MESSAGE_MAP(CRLoginView, CView)
	//{{AFX_MSG_MAP(CRLoginView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
//	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_MESSAGE(WM_IME_NOTIFY, OnImeNotify)
	ON_MESSAGE(WM_IME_COMPOSITION, OnImeComposition)
	ON_WM_VSCROLL()
	ON_WM_TIMER()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_MACRO_REC, OnMacroRec)
	ON_UPDATE_COMMAND_UI(ID_MACRO_REC, OnUpdateMacroRec)
	ON_COMMAND(ID_MACRO_PLAY, OnMacroPlay)
	ON_UPDATE_COMMAND_UI(ID_MACRO_PLAY, OnUpdateMacroPlay)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_MACRO_HIS1, ID_MACRO_HIS5, OnMacroHis)
	ON_WM_DROPFILES()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_MOUSE_EVENT, &CRLoginView::OnMouseEvent)
	ON_UPDATE_COMMAND_UI(ID_MOUSE_EVENT, &CRLoginView::OnUpdateMouseEvent)
	ON_COMMAND(IDM_BROADCAST, &CRLoginView::OnBroadcast)
	ON_UPDATE_COMMAND_UI(IDM_BROADCAST, &CRLoginView::OnUpdateBroadcast)
	ON_COMMAND(ID_EDIT_COPY_ALL, &CRLoginView::OnEditCopyAll)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY_ALL, &CRLoginView::OnUpdateEditCopyAll)
	ON_COMMAND(ID_PAGE_PRIOR, &CRLoginView::OnPagePrior)
	ON_COMMAND(ID_PAGE_NEXT, &CRLoginView::OnPageNext)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスの構築/消滅

CRLoginView::CRLoginView()
{
	// TODO: この場所に構築用のコードを追加してください。
	m_Width  = 100;
	m_Height = 100;
	m_CharWidth  = 8;
	m_CharHeight = 16;
	m_Cols = 80;
	m_Lines = 25;
	m_HisMin = 0;
	m_HisOfs = 0;
	m_DispCaret = 0;
	m_CaretX = m_CaretY = 0;
	m_pBitmap = NULL;
	m_ClipFlag = 0;
	m_ClipTimer = 0;
	m_KeyMacFlag = FALSE;
	m_ActiveFlag = TRUE;
	m_VisualBellFlag = FALSE;
	m_BlinkFlag = 0;
	m_MouseEventFlag = FALSE;
	m_BroadCast = FALSE;
	m_WheelDelta = 0;
	m_WheelTimer = FALSE;
	m_pGhost = NULL;
}

CRLoginView::~CRLoginView()
{
	if ( m_pGhost != NULL )
		m_pGhost->DestroyWindow();
}

BOOL CRLoginView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS;

	cs.lpszName = "RLoginView";
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW)); //, CreateSolidBrush(0xff000000));

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスの描画

void CRLoginView::OnDraw(CDC* pDC)
{
	CRLoginDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if ( (m_DispCaret & 001) != 0 )
		HideCaret();

	int sx = 0;
	int sy = 0;
	int ex = m_Cols  - 1;
	int ey = m_Lines - 1;

	if ( !pDC->IsPrinting() ) {
		CRect rect(((CPaintDC *)(pDC))->m_ps.rcPaint);
		sx = rect.left   * m_Cols  / m_Width;
		ex = (rect.right + m_CharWidth - 1)  * m_Cols  / m_Width - 1;
		sy = rect.top    * m_Lines / m_Height;
		ey = (rect.bottom + m_CharHeight - 1) * m_Lines / m_Height - 1;

		if ( m_pBitmap != NULL ) {
			CDC TempDC;
			CBitmap *pOldBitMap;
			TempDC.CreateCompatibleDC(pDC);
			pOldBitMap = (CBitmap *)TempDC.SelectObject(m_pBitmap);
			pDC->BitBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, &TempDC, rect.left, rect.top, SRCCOPY);
			TempDC.SelectObject(pOldBitMap);
			pDC->SetBkMode(TRANSPARENT);
		}
	}

	if ( pDoc->m_TextRam.IsInitText() )
		pDoc->m_TextRam.DrawVram(pDC, sx, sy, ex, ey, this);

	if ( (m_DispCaret & 001) != 0 )
		ShowCaret();

//	TRACE("Draw %x(%d,%d,%d,%d)\n", m_hWnd, sx, sy, ex, ey);
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスの診断

#ifdef _DEBUG
void CRLoginView::AssertValid() const
{
	CView::AssertValid();
}

void CRLoginView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CRLoginDoc* CRLoginView::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRLoginDoc)));
	return (CRLoginDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRLoginView Lib

void CRLoginView::CalcTextRect(CRect &rect)
{
	rect.left   = m_Width  * rect.left  / m_Cols;
	rect.right  = m_Width  * rect.right / m_Cols;
	rect.top    = m_Height * (rect.top    + m_HisOfs - m_HisMin) / m_Lines;
	rect.bottom = m_Height * (rect.bottom + m_HisOfs - m_HisMin) / m_Lines;
}
void CRLoginView::CalcGrapPoint(CPoint po, int *x, int *y)
{
	*x = m_Cols  * po.x / m_Width;
	*y = m_Lines * po.y / m_Height - m_HisOfs + m_HisMin;
}
void CRLoginView::SendBroadCast(CBuffer &buf)
{
	CBuffer tmp;
	CWinApp *pApp;
	CRLoginDoc *pThisDoc = GetDocument();

	if ( (pApp = AfxGetApp()) == NULL )
		return;

	POSITION pos = pApp->GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = pApp->GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
			if ( pDoc == pThisDoc )
				continue;
			tmp = buf;
			pDoc->SendBuffer(tmp);
		}
	}
}
void CRLoginView::SendBuffer(CBuffer &buf, BOOL macflag)
{
	int n;
	WCHAR *p;
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	if ( macflag == FALSE ) {
		if ( pDoc->m_TextRam.IsOptEnable(TO_RLDSECHO) && !pDoc->m_TextRam.LineEdit(buf) )
			return;
		pDoc->OnSendBuffer(buf);
		if ( m_BroadCast )
			SendBroadCast(buf);
	}

	if ( m_KeyMacFlag )
		m_KeyMacBuf.Apend(buf.GetPtr(), buf.GetSize());

	// TO_RLECHOCR, TO_RLECHOLF  = 00(CR), 01(LF), 10(CR+LF)
	switch(pDoc->m_TextRam.IsOptEnable(TO_ANSILNM) ? 2 : pDoc->m_TextRam.m_SendCrLf) {
	case 0:	// CR
		break;
	case 1:	// LF
		n = buf.GetSize() / sizeof(WCHAR);
		p = (WCHAR *)buf.GetPtr();
		while ( n-- > 0 ) {
			if ( *p == 0x0D )
				*p = 0x0A;
			p++;
		}
		break;
	case 2:	// CR+LF
		n = buf.GetSize() / sizeof(WCHAR);
		p = (WCHAR *)buf.GetPtr();
		while ( n-- > 0 ) {
			if ( *p == 0x0D ) {
				tmp.PutWord(0x0D);
				tmp.PutWord(0x0A);
			} else
				tmp.PutWord(*p);
			p++;
		}
		buf = tmp;
		tmp.Clear();
		break;
	}

	CTextRam::MsToIconvUnicode((WCHAR *)(buf.GetPtr()), buf.GetSize() / sizeof(WCHAR), pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode]);
	pDoc->m_TextRam.m_IConv.IConvBuf("UCS-2LE", pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode], &buf, &tmp);
	pDoc->SocketSend(tmp.GetPtr(), tmp.GetSize());
}
void CRLoginView::SetCaret()
{
	CPoint po(m_CaretX, m_CaretY);

	// 001 = CreateCaret Flag, 002 = CurSol ON/OFF, 004 = Focus Flag
	switch(m_DispCaret) {
	case 006:
		CreateSolidCaret(m_CharWidth, m_CharHeight);
		SetCaretPos(po);
		ImmSetPos(po.x, po.y);
		ShowCaret();
		m_DispCaret |= 001;
		break;
	case 007:
		SetCaretPos(po);
		ImmSetPos(po.x, po.y);
		break;
	case 004:
		ImmSetPos(po.x, po.y);
		break;
	case 001:
	case 003:
	case 005:
		DestroyCaret();
		m_DispCaret &= ~001;
		break;
	}
}
void CRLoginView::ImmSetPos(int x, int y)
{
	HIMC hIMC;
	COMPOSITIONFORM cpf;

	if ( (hIMC = ImmGetContext(m_hWnd)) != NULL ) {
	    cpf.dwStyle = CFS_POINT;
		cpf.ptCurrentPos.x = x;
		cpf.ptCurrentPos.y = y;
		ImmSetCompositionWindow(hIMC, &cpf);
		ImmReleaseContext(m_hWnd, hIMC);
	}
}
void CRLoginView::SetGhostWnd(BOOL sw)
{
	if ( sw ) {		// Create
		if ( m_pGhost != NULL )
			return;
		CRect rect;
		GetWindowRect(rect);
		m_pGhost = new CGhostWnd();
		m_pGhost->m_pView = this;
		m_pGhost->m_pDoc  = GetDocument();
		m_pGhost->Create(NULL, m_pGhost->m_pDoc->GetTitle(), WS_TILED, rect, CWnd::GetDesktopWindow(), IDD_GHOSTWND);
//		m_pGhost->ShowWindow(SW_SHOWNOACTIVATE);
		m_pGhost->SetWindowPos(&wndTopMost, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW);

	} else {		// Destory
		if ( m_pGhost != NULL )
			m_pGhost->DestroyWindow();
		m_pGhost = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスのメッセージ ハンドラ

int CRLoginView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	DragAcceptFiles();

	return 0;
}

void CRLoginView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	if ( nType == SIZE_MINIMIZED )
		return;

	CRect rect;
	CString tmp;
	CRLoginDoc *pDoc = GetDocument();
	CChildFrame *pFrame = GetFrameWnd();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	ASSERT(pDoc);
	ASSERT(pFrame);

	m_Width  = cx;
	m_Height = cy;

	pFrame->GetClientRect(rect);
	pFrame->m_Width = cx;
	pFrame->m_Height = rect.Height() - 4;

//	if ( m_ActiveFlag ) {
		pDoc->m_TextRam.InitText(pFrame->m_Width, pFrame->m_Height);
		pFrame->m_Cols  = pDoc->m_TextRam.m_Cols;
		pFrame->m_Lines = pDoc->m_TextRam.m_Lines;
//	}

	pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	tmp.Format("%d x %d", pDoc->m_TextRam.m_Cols, pDoc->m_TextRam.m_Lines);
	pMain->SetMessageText(tmp);
}

void CRLoginView::OnMove(int x, int y) 
{
	CView::OnMove(x, y);
	OnUpdate(this, UPDATE_INVALIDATE, NULL);
}

void CRLoginView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int x, y;
	CRect rect;
	CRLoginDoc *pDoc = GetDocument();
	CChildFrame *pFrame = GetFrameWnd();
	ASSERT(pDoc);
	ASSERT(pFrame);

	if ( lHint == UPDATE_VISUALBELL ) {
		if ( !m_VisualBellFlag ) {
			SetTimer(1025, 50, NULL);
			m_VisualBellFlag = TRUE;
			Invalidate(FALSE);
			if ( m_pGhost != NULL )
				m_pGhost->Invalidate(FALSE);
		}
		return;
	} else if ( lHint == UPDATE_SETCURSOR ) {
		CPoint point;
		GetClientRect(rect);
		ClientToScreen(rect);
		GetCursorPos(&point);
		if ( rect.PtInRect(point) )
			::SetCursor(::LoadCursor(NULL, (pDoc->m_TextRam.m_MouseTrack > 0 && !m_MouseEventFlag ? IDC_IBEAM : IDC_ARROW)));
		return;
	}

	if ( (m_DispCaret & 004) != 0 && pSender != this &&
			(lHint == UPDATE_INVALIDATE || lHint == UPDATE_TEXTRECT || lHint == UPDATE_GOTOXY) ) {
		y = pDoc->m_TextRam.m_CurY - m_HisMin + m_HisOfs;
		if ( y >= m_Lines ) {
			m_HisOfs = m_Lines - (pDoc->m_TextRam.m_CurY - m_HisMin) - 1;
			lHint = UPDATE_INVALIDATE;
		} else if ( y < 0 ) {
			m_HisOfs = m_HisMin - pDoc->m_TextRam.m_CurY;
			lHint = UPDATE_INVALIDATE;
		}
	}

	switch(lHint) {
	case UPDATE_INITSIZE:
		pFrame->m_Cols  = pDoc->m_TextRam.m_Cols;
		pFrame->m_Lines = pDoc->m_TextRam.m_Lines;
		// no break;
	case UPDATE_INITPARA:
		if ( (m_Cols  = m_Width  * pFrame->m_Cols / pFrame->m_Width) <= 0 )
			m_Cols = 1;
		if ( (m_Lines = m_Height * pFrame->m_Lines / pFrame->m_Height) <= 0 )
			m_Lines = 1;

		m_HisMin = pDoc->m_TextRam.m_Lines - m_Lines;

		m_CharWidth  = pFrame->m_Width  / pFrame->m_Cols;
		m_CharHeight = pFrame->m_Height / pFrame->m_Lines;
		//if ( (m_CharHeight = pFrame->m_Height / pFrame->m_Lines) > (m_CharWidth * 2) )
		//	m_CharHeight = m_CharWidth * 2;
		
		if ( (m_DispCaret & 001) != 0 ) {
			m_DispCaret &= ~001;
			DestroyCaret();
		}

		m_BmpFile.LoadFile(pDoc->m_TextRam.m_BitMapFile);
		m_pBitmap = m_BmpFile.GetBitmap(GetDC(), m_Width, m_Height);

		// No break
	case UPDATE_INVALIDATE:
		if ( m_HisOfs > 0 )
			m_HisOfs += pDoc->m_TextRam.m_HisUse;
		if ( m_HisOfs < 0 )
			m_HisOfs = 0;
		else if ( (m_Lines + m_HisOfs) > pDoc->m_TextRam.m_HisLen )
			m_HisOfs = pDoc->m_TextRam.m_HisLen - m_Lines;
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask  = SIF_PAGE | SIF_POS | SIF_RANGE;
		// Min=0, Max=99, Page=10, Pos = 90
		info.nMin   = 0;
		info.nMax   = pDoc->m_TextRam.m_HisLen - 1;
		info.nPage  = m_Lines;
		info.nPos   = pDoc->m_TextRam.m_HisLen - m_Lines - m_HisOfs;
		info.nTrackPos = 0;
		SetScrollInfo(SB_VERT, &info, TRUE);
		Invalidate(FALSE);
		if ( m_pGhost != NULL )
			m_pGhost->Invalidate(FALSE);
		break;

	case UPDATE_TEXTRECT:
	case UPDATE_BLINKRECT:
		rect = *((CRect *)pHint);
		CalcTextRect(rect);
		InvalidateRect(rect, FALSE);
		if ( m_pGhost != NULL )
			m_pGhost->InvalidateRect(rect, FALSE);
		break;

	case UPDATE_CLIPERA:
		if ( m_ClipStaPos <= m_ClipEndPos ) {
			pDoc->m_TextRam.SetCalcPos(m_ClipStaPos, &x, &y);
			rect.left = x; rect.top = y;
			pDoc->m_TextRam.SetCalcPos(m_ClipEndPos, &x, &y);
			rect.right = x + 1; rect.bottom = y + 1;
		} else {
			pDoc->m_TextRam.SetCalcPos(m_ClipEndPos, &x, &y);
			rect.left = x; rect.top = y;
			pDoc->m_TextRam.SetCalcPos(m_ClipStaPos, &x, &y);
			rect.right = x + 1; rect.bottom = y + 1;
		}
		if ( rect.Height() > 1 ) {
			rect.left = 0;
			rect.right = pDoc->m_TextRam.m_Cols;
		} else {
			if ( --rect.left < 0 )
				rect.left = 0;
			if ( ++rect.right > pDoc->m_TextRam.m_Cols )
				rect.right = pDoc->m_TextRam.m_Cols;
		}
		CalcTextRect(rect);
		InvalidateRect(rect, FALSE);
		if ( m_pGhost != NULL )
			m_pGhost->InvalidateRect(rect, FALSE);
		break;

	case UPDATE_GOTOXY:
		break;
	}

	m_DispCaret &= ~002;
	m_CaretX = m_Width  * pDoc->m_TextRam.m_CurX / m_Cols;
	m_CaretY = m_Height * (pDoc->m_TextRam.m_CurY + m_HisOfs - m_HisMin) / m_Lines;

	if ( m_CaretX < 0 || m_CaretX >= m_Width || m_CaretY < 0 || m_CaretY >= m_Height )
		m_CaretX = m_CaretY = 0;
	else if ( pDoc->m_pSock != NULL )
		m_DispCaret |= (pDoc->m_TextRam.m_DispCaret & 002);

	SetCaret();
}

void CRLoginView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CBuffer tmp;

//	TRACE("OnChar %02x(%04x)\n", nChar, nFlags);

	if ( (nFlags & 0x2000) != 0 )	// with Alt key
		tmp.PutWord(0x1B);

	CView::OnChar(nChar, nRepCnt, nFlags);

	tmp.PutWord(nChar);
	SendBuffer(tmp);
}
//void CRLoginView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
int CRLoginView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int n;
	int st = 0;
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	//CView::OnKeyDown(nChar, nRepCnt, nFlags);

//	TRACE("KeyDown %02X(%04X)\n", nChar, nFlags);

	if ( nChar == VK_SHIFT || nChar == VK_CONTROL || nChar == VK_MENU )
		return TRUE;

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLDSECHO) ) {
		switch(nChar) {
		case VK_LEFT:   tmp.PutWord(0x1C); SendBuffer(tmp); return FALSE;
		case VK_RIGHT:  tmp.PutWord(0x1D); SendBuffer(tmp); return FALSE;
		case VK_UP:     tmp.PutWord(0x1E); SendBuffer(tmp); return FALSE;
		case VK_DOWN:   tmp.PutWord(0x1F); SendBuffer(tmp); return FALSE;
		case VK_DELETE: tmp.PutWord(0x7F); SendBuffer(tmp); return FALSE;
		case VK_BACK:   tmp.PutWord(0x08); SendBuffer(tmp); return FALSE;
		case VK_TAB:    tmp.PutWord(0x09); SendBuffer(tmp); return FALSE;
		case VK_RETURN: tmp.PutWord(0x0D); SendBuffer(tmp); return FALSE;
		}
		return TRUE;
	}

	if ( (GetKeyState(VK_SHIFT) & 0x80) != 0 )
		st |= MASK_SHIFT;
	if ( (GetKeyState(VK_CONTROL) & 0x80) != 0 )
		st |= MASK_CTRL;
	if ( (GetKeyState(VK_MENU) & 0x80) != 0 )
		st |= MASK_ALT;
	if ( pDoc->m_TextRam.IsOptEnable(TO_DECCKM) )
		st |= MASK_APPL;

	//TRACE("OnKey %02x(%02x)\n", nChar, st);

	if ( pDoc->m_KeyTab.FindMaps(nChar, st, &tmp) ) {
		if ( (n = CKeyNodeTab::GetCmdsKey((LPCWSTR)tmp)) > 0 )
			AfxGetMainWnd()->PostMessage(WM_COMMAND, (WPARAM)n);
		else
			SendBuffer(tmp);
		return FALSE;
	}

	if ( (st & MASK_CTRL) != 0 ) {
		switch(nChar) {
		case VK_OEM_7:	// CTRL+^
			tmp.PutWord(0x1E);
			break;
		case VK_OEM_2:	// CTRL+?
			tmp.PutWord(0x1F);
			break;
		case VK_OEM_3:	// CTRL+@
		case VK_SPACE:	// CTRL+SPACE
			tmp.PutWord(0x00);
			break;
		}
		if ( tmp.GetSize() > 0 ) {
			SendBuffer(tmp);
			return FALSE;
		}
	}

	return TRUE;
}
void CRLoginView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	m_DispCaret |= 004;
	SetCaret();
}
void CRLoginView::OnKillFocus(CWnd* pNewWnd) 
{
	CView::OnKillFocus(pNewWnd);
	m_DispCaret &= ~004;
	SetCaret();
}
void CRLoginView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	if ( bActivate && pActivateView->m_hWnd == m_hWnd ) {
		CRLoginDoc *pDoc = GetDocument();
		CChildFrame *pFrame = GetFrameWnd();
		ASSERT(pDoc);
		ASSERT(pFrame);

		m_ActiveFlag = TRUE;
		pDoc->m_KeyMac.SetHisMenu(GetMainWnd());
		pDoc->m_TextRam.InitText(pFrame->m_Width, pFrame->m_Height);
		pFrame->m_Cols  = pDoc->m_TextRam.m_Cols;
		pFrame->m_Lines = pDoc->m_TextRam.m_Lines;
		pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
	} else if ( !bActivate && pDeactiveView->m_hWnd == m_hWnd )
		m_ActiveFlag = FALSE;
}

LRESULT CRLoginView::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
    switch(wParam) {
	case IMN_SETCOMPOSITIONWINDOW:
		HIMC hIMC;
		LOGFONT LogFont;
		if ( (hIMC = ImmGetContext(m_hWnd)) ) {
			if ( ImmGetCompositionFont(hIMC, &LogFont) ) {
				LogFont.lfWidth  = m_CharWidth;
				LogFont.lfHeight = m_CharHeight;
				ImmSetCompositionFont(hIMC, &LogFont);
			}
			ImmReleaseContext(m_hWnd, hIMC);
	    }
		return TRUE;
    case IMN_SETSTATUSWINDOWPOS:
		ImmSetPos(m_CaretX, m_CaretY);
		return TRUE;
    default:
		return DefWindowProc(WM_IME_NOTIFY, wParam, lParam);
    }
}
LRESULT CRLoginView::OnImeComposition(WPARAM wParam, LPARAM lParam)
{
	if ( (lParam & GCS_RESULTSTR) != 0 ) {
		HIMC hImc;
		LONG len;
		CBuffer tmp;

		hImc = ImmGetContext(m_hWnd);
		len = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, NULL, 0);
		len = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, tmp.PutSpc(len), len);
	    ImmReleaseContext(m_hWnd, hImc);

		SendBuffer(tmp);
		return TRUE;
	} else
		return DefWindowProc(WM_IME_COMPOSITION, wParam, lParam);
}

void CRLoginView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	int pos = m_HisOfs;
	int min = 0;
	int max = pDoc->m_TextRam.m_HisLen - m_Lines;

	SCROLLINFO info;
	ZeroMemory(&info, sizeof(SCROLLINFO));
	info.cbSize = sizeof(info);
	
	switch(nSBCode) {
	case SB_TOP:		// 一番上までスクロール。
		pos = max;
		break;
	case SB_BOTTOM:		// 一番下までスクロール。
		pos = min;
		break;
	case SB_ENDSCROLL:	// スクロール終了。
		break;
	case SB_LINEDOWN:	// 1 行下へスクロール。
		if ( (pos -= 1) < min )
			pos = min;
		break;
	case SB_LINEUP:		// 1 行上へスクロール。
		if ( (pos += 1) > max )
			pos = max;
		break;
	case SB_PAGEDOWN:	// 1 ページ下へスクロール。
		if ( (pos -= (m_Lines * 2 / 3)) < min )
			pos = min;
		break;
	case SB_PAGEUP:		// 1 ページ上へスクロール。
		if ( (pos += (m_Lines * 2 / 3)) > max )
			pos = max;
		break;
	case SB_THUMBPOSITION:	// 絶対位置へスクロール。現在位置は nPos で提供。
		info.fMask  = SIF_POS;
		GetScrollInfo(SB_VERT, &info, SIF_POS);
		pos = pDoc->m_TextRam.m_HisLen - m_Lines - info.nPos;
		break;
	case SB_THUMBTRACK:		// 指定位置へスクロール ボックスをドラッグ。現在位置は nPos で提供。
		info.fMask  = SIF_TRACKPOS;
		GetScrollInfo(SB_VERT, &info, SIF_TRACKPOS);
		pos = pDoc->m_TextRam.m_HisLen - m_Lines - info.nTrackPos;
		break;
	}

	if ( pos != m_HisOfs ) {
		m_HisOfs = pos;
		OnUpdate(this, UPDATE_INVALIDATE, NULL);
	}

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CRLoginView::OnTimer(UINT_PTR nIDEvent) 
{
	CRLoginDoc *pDoc = GetDocument();

	CView::OnTimer(nIDEvent);

	switch(nIDEvent) {
	case 1024:		// ClipTimer
		OnMouseMove(m_ClipKeyFlags, m_ClipSavePoint);
		break;
	case 1025:		// VisualBell
		KillTimer(nIDEvent);
		m_VisualBellFlag = FALSE;
		Invalidate(FALSE);
		break;
	case 1026:		// Blink Timer
		m_BlinkFlag = (++m_BlinkFlag & 3) | 4;
		if ( pDoc->m_TextRam.BLINKUPDATE(this) == 0 ) {
			KillTimer(nIDEvent);
			m_BlinkFlag = 0;
		}
		break;
	case 1027:		// Wheel Timer
		if ( (m_HisOfs += m_WheelDelta) < 0 ) {
			m_HisOfs = 0;
			m_WheelDelta = 0;
		} else if ( m_HisOfs > pDoc->m_TextRam.m_HisLen - m_Lines ) {
			m_HisOfs = pDoc->m_TextRam.m_HisLen - m_Lines;
			m_WheelDelta = 0;
		} else if ( m_WheelDelta < 0 )
			m_WheelDelta += 1;
		else
			m_WheelDelta -= 1;

		if ( m_WheelDelta == 0 ) {
			KillTimer(nIDEvent);
			m_WheelTimer = FALSE;
		}

		OnUpdate(this, UPDATE_INVALIDATE, NULL);
		break;
	}
}

BOOL CRLoginView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	int pos, ofs;
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	ofs = zDelta * pDoc->m_TextRam.m_WheelSize / (WHEEL_DELTA / 2);

	if ( pDoc->m_TextRam.IsOptEnable(TO_DECCKM) || (pDoc->m_TextRam.m_MouseTrack > 0 && !m_MouseEventFlag) ) {
		if ( pDoc->m_KeyTab.FindMaps((ofs > 0 ? VK_UP : VK_DOWN), (pDoc->m_TextRam.IsOptEnable(TO_DECCKM) ? MASK_APPL : 0), &tmp) ) {
			for ( pos = (ofs < 0 ? (0 - ofs) : ofs) ; pos > 0 ; pos-- )
				SendBuffer(tmp);
		}
	} else {

		if ( m_WheelTimer ) {
			if ( m_WheelDelta > 0 ) {
				if ( ofs < 0 )
					m_WheelDelta = ofs;
				else
					m_WheelDelta += ofs;
			} else if ( m_WheelDelta < 0 ) {
				if ( ofs > 0 )
					m_WheelDelta = ofs;
				else
					m_WheelDelta += ofs;
			} else
				m_WheelDelta = ofs;
		} else {
			if ( !pDoc->m_TextRam.IsOptEnable(TO_RLMOSWHL) && (ofs > 3 || ofs < -3) ) {
				m_WheelDelta = ofs;
				SetTimer(1027, 100, NULL);
				m_WheelTimer = TRUE;
			} else {
				if ( (pos = m_HisOfs + ofs) < 0 )
					pos = 0;
				else if ( pos > (pDoc->m_TextRam.m_HisLen - m_Lines) )
					pos = pDoc->m_TextRam.m_HisLen - m_Lines;
				if ( pos != m_HisOfs ) {
					m_HisOfs = pos;
					OnUpdate(this, UPDATE_INVALIDATE, NULL);
				}
			}
		}
	}

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CRLoginView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CView::OnLButtonDblClk(nFlags, point);

	if ( m_ClipFlag == 6 ) {
		m_ClipFlag = 0;
		OnUpdate(this, UPDATE_CLIPERA, NULL);
	} else if ( m_ClipFlag != 0 )
		return;

	int x, y;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	CalcGrapPoint(point, &x, &y);
	m_ClipStaPos = m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
	pDoc->m_TextRam.EditWordPos(&m_ClipStaPos, &m_ClipEndPos);
	m_ClipFlag = 1;
	m_ClipTimer = 0;
	m_ClipKeyFlags = nFlags;
	SetCapture();
	OnUpdate(this, UPDATE_CLIPERA, NULL);
}
void CRLoginView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int x, y;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	CView::OnLButtonDown(nFlags, point);
	SetCapture();

	if ( m_ClipFlag == 6 ) {
		m_ClipFlag = 0;
		OnUpdate(this, UPDATE_CLIPERA, NULL);
	} else if ( m_ClipFlag != 0 )
		return;

	CalcGrapPoint(point, &x, &y);

	if ( pDoc->m_TextRam.m_MouseTrack > 0 && !m_MouseEventFlag ) {
		// Left			xxxx xx00	1
		// Right		xxxx xx01	2
		// Middle		xxxx xx10	3
		// Button 1		x1xx xx00	4
		// Button 2		x1xx xx01	5
		// Button 3		x1xx xx10	6
		// Release		xxxx xx11
		// shift		xxxx x1xx
		// meta			xxxx 1xxx
		// ctrl			xxx1 xxxx
		// motion		xx1x xxxx
		if ( x >= pDoc->m_TextRam.m_Cols  ) x = pDoc->m_TextRam.m_Cols  - 1; else if ( x < 0 ) x = 0;
		if ( y >= pDoc->m_TextRam.m_Lines ) y = pDoc->m_TextRam.m_Lines - 1; else if ( y < 0 ) y = 0;

		if ( pDoc->m_TextRam.m_MouseTrack == 6 )
			pDoc->m_TextRam.LocReport(1, 1, x, y);
		else if ( pDoc->m_TextRam.m_MouseTrack == 1 )
			pDoc->m_TextRam.UNGETSTR("\033[M%c%c%c", ' ' + (pDoc->m_TextRam.m_MouseMode[0] & 3), ' ' + x + 1, ' ' + y + 1);
		else
			pDoc->m_TextRam.UNGETSTR("\033[M%c%c%c", ' ' + pDoc->m_TextRam.m_MouseMode[0] + (nFlags & MK_SHIFT ? pDoc->m_TextRam.m_MouseMode[2] : 0) + (nFlags & MK_CONTROL ? pDoc->m_TextRam.m_MouseMode[3] : 0), ' ' + x + 1, ' ' + y + 1);

		if ( pDoc->m_TextRam.m_MouseTrack == 3 ) {
			m_ClipStaPos = m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
			m_ClipFlag     = 6;
			m_ClipKeyFlags = 0;
		}

		return;
	}

	m_ClipStaPos = m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
	m_ClipFlag     = 1;
	m_ClipTimer    = 0;
	m_ClipKeyFlags = nFlags;
}
void CRLoginView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	int x, y, pos;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	CView::OnLButtonUp(nFlags, point);
	ReleaseCapture();

	CalcGrapPoint(point, &x, &y);

	if ( m_ClipFlag == 0 || m_ClipFlag == 6 ) {
		if ( pDoc->m_TextRam.m_MouseTrack > 1 && !m_MouseEventFlag ) {
			if ( x >= pDoc->m_TextRam.m_Cols  ) x = pDoc->m_TextRam.m_Cols  - 1; else if ( x < 0 ) x = 0;
			if ( y >= pDoc->m_TextRam.m_Lines ) y = pDoc->m_TextRam.m_Lines - 1; else if ( y < 0 ) y = 0;

			if ( pDoc->m_TextRam.m_MouseTrack == 6 )
				pDoc->m_TextRam.LocReport(1, 0, x, y);
			else if ( pDoc->m_TextRam.m_MouseTrack == 3 ) {
				m_ClipFlag = 0;
				OnUpdate(this, UPDATE_CLIPERA, NULL);
				pDoc->m_TextRam.UNGETSTR("\033[t%c%c", ' ' + 1 + x, ' ' + 1 + y);
			} else
				pDoc->m_TextRam.UNGETSTR("\033[M%c%c%c", ' ' + 3 + (nFlags & MK_SHIFT ? pDoc->m_TextRam.m_MouseMode[2] : 0) + (nFlags & MK_CONTROL ? pDoc->m_TextRam.m_MouseMode[3] : 0), ' ' + x + 1, ' ' + y + 1);
		}
		return;
	}

	OnUpdate(this, UPDATE_CLIPERA, NULL);

//	m_ClipKeyFlags = nFlags;

	pos = pDoc->m_TextRam.GetCalcPos(x, y);

	switch(m_ClipFlag) {
	case 3:
		if ( pos < m_ClipStaPos ) {
			m_ClipEndPos = m_ClipStaPos;
			m_ClipStaPos = pos;
		} else
			m_ClipEndPos = pos;
		break;
	case 4:
		if ( pos > m_ClipEndPos ) {
			m_ClipStaPos = m_ClipEndPos;
			m_ClipEndPos = pos;
		} else
			m_ClipStaPos = pos;
		break;
	}

	if ( m_ClipTimer != 0 )
		KillTimer(m_ClipTimer);

	if ( m_ClipStaPos == m_ClipEndPos ) {
		m_ClipFlag = 0;
		OnUpdate(this, UPDATE_CLIPERA, NULL);
		return;
	}

	m_ClipFlag = 6;
	if ( pDoc->m_TextRam.IsOptEnable(TO_RLCKCOPY) )
		OnEditCopy();
}
void CRLoginView::OnMouseMove(UINT nFlags, CPoint point) 
{
	int x, y, pos;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	CView::OnMouseMove(nFlags, point);

	if ( m_ClipFlag == 0 || m_ClipFlag == 6 ) {
		CalcGrapPoint(point, &x, &y);
		if ( (pDoc->m_TextRam.m_MouseTrack == 4 || pDoc->m_TextRam.m_MouseTrack == 5) && !m_MouseEventFlag ) {
			switch(nFlags & (MK_LBUTTON | MK_RBUTTON)) {
			case 0:
				if ( pDoc->m_TextRam.m_MouseTrack == 4 )
					return;
				pos = 3;
				break;
			case MK_LBUTTON:
				pos = pDoc->m_TextRam.m_MouseMode[0];
				break;
			case MK_RBUTTON:
			case MK_LBUTTON | MK_RBUTTON:
				pos = pDoc->m_TextRam.m_MouseMode[1];
				break;
			}
			if ( x >= pDoc->m_TextRam.m_Cols  ) x = pDoc->m_TextRam.m_Cols  - 1; else if ( x < 0 ) x = 0;
			if ( y >= pDoc->m_TextRam.m_Lines ) y = pDoc->m_TextRam.m_Lines - 1; else if ( y < 0 ) y = 0;

			pDoc->m_TextRam.UNGETSTR("\033[M%c%c%c", ' ' + pos + (nFlags & MK_SHIFT ? pDoc->m_TextRam.m_MouseMode[2] : 0) + (nFlags & MK_CONTROL ? pDoc->m_TextRam.m_MouseMode[3] : 0), ' ' + x + 1, ' ' + y + 1);

		} else if ( pDoc->m_TextRam.m_MouseTrack == 3 && !m_MouseEventFlag ) {
			OnUpdate(this, UPDATE_CLIPERA, NULL);
			m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
			OnUpdate(this, UPDATE_CLIPERA, NULL);
		}
		return;
	}

	OnUpdate(this, UPDATE_CLIPERA, NULL);

//	m_ClipKeyFlags = nFlags;

	if ( point.y < 0 || point.y > m_Height ) {
		m_ClipSavePoint = point;
		if ( m_ClipTimer == 0 )
			m_ClipTimer = (UINT)SetTimer(1024, 100, NULL);
		if ( point.y < (0 - m_CharHeight * 3) )
			OnVScroll(SB_PAGEUP, 0, NULL);
		else if ( point.y < 0 )
			OnVScroll(SB_LINEUP, 0, NULL);
		else if ( point.y < (m_CharHeight * 3) )
			OnVScroll(SB_LINEDOWN, 0, NULL);
		else
			OnVScroll(SB_PAGEDOWN, 0, NULL);
	} else {
		if ( m_ClipTimer != 0 )
			KillTimer(m_ClipTimer);
		m_ClipTimer = 0;
	}

	CalcGrapPoint(point, &x, &y);
	pos = pDoc->m_TextRam.GetCalcPos(x, y);

	switch(m_ClipFlag) {
	case 1:
	case 2:
		if ( pos < m_ClipStaPos ) {
			m_ClipFlag = 4;
			m_ClipStaPos = pos;
		} else if ( pos > m_ClipEndPos ) {
			m_ClipFlag = 3;
			m_ClipEndPos = pos;
		}
		break;
	case 3:
		if ( pos < m_ClipStaPos ) {
			m_ClipFlag = 4;
			m_ClipEndPos = m_ClipStaPos;
			m_ClipStaPos = pos;
		} else
			m_ClipEndPos = pos;
		break;
	case 4:
		if ( pos > m_ClipEndPos ) {
			m_ClipFlag = 3;
			m_ClipStaPos = m_ClipEndPos;
			m_ClipEndPos = pos;
		} else
			m_ClipStaPos = pos;
		break;
	}

	OnUpdate(this, UPDATE_CLIPERA, NULL);
}
void CRLoginView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLRCLICK) )
		OnEditPaste();

	CView::OnRButtonDblClk(nFlags, point);
}
void CRLoginView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int x, y;
	CCmdUI state;
	CMenu *pMenu, *pMain;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	CView::OnRButtonDown(nFlags, point);
	SetCapture();

	if ( pDoc->m_TextRam.m_MouseTrack > 0 && !m_MouseEventFlag ) {
		CalcGrapPoint(point, &x, &y);
		if ( x >= pDoc->m_TextRam.m_Cols  ) x = pDoc->m_TextRam.m_Cols  - 1; else if ( x < 0 ) x = 0;
		if ( y >= pDoc->m_TextRam.m_Lines ) y = pDoc->m_TextRam.m_Lines - 1; else if ( y < 0 ) y = 0;

		if ( pDoc->m_TextRam.m_MouseTrack == 6 )
			pDoc->m_TextRam.LocReport(1, 3, x, y);
		else if ( pDoc->m_TextRam.m_MouseTrack == 1 )
			pDoc->m_TextRam.UNGETSTR("\033[M%c%c%c", ' ' + (pDoc->m_TextRam.m_MouseMode[1] & 3), ' ' + x + 1, ' ' + y + 1);
		else
			pDoc->m_TextRam.UNGETSTR("\033[M%c%c%c", ' ' + pDoc->m_TextRam.m_MouseMode[1] + (nFlags & MK_SHIFT ? pDoc->m_TextRam.m_MouseMode[2] : 0) + (nFlags & MK_CONTROL ? pDoc->m_TextRam.m_MouseMode[3] : 0), ' ' + x + 1, ' ' + y + 1);
		return;
	}

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLRCLICK) )
		return;

	if ( (pMain = GetMainWnd()->GetMenu()) == NULL || (pMenu = pMain->GetSubMenu(1)) == NULL )
		return;

	state.m_pMenu = pMenu;
	state.m_nIndexMax = pMenu->GetMenuItemCount();
	for ( state.m_nIndex = 0 ; state.m_nIndex < state.m_nIndexMax ; state.m_nIndex++) {
		if ( (int)(state.m_nID = pMenu->GetMenuItemID(state.m_nIndex)) <= 0 )
			continue;
		state.m_pSubMenu = NULL;
		state.DoUpdate(this, TRUE);
	}

	ClientToScreen(&point);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CRLoginView::OnRButtonUp(UINT nFlags, CPoint point)
{
	int x, y;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	CView::OnRButtonUp(nFlags, point);
	ReleaseCapture();

	if ( pDoc->m_TextRam.m_MouseTrack > 1 && !m_MouseEventFlag ) {
		CalcGrapPoint(point, &x, &y);
		if ( x >= pDoc->m_TextRam.m_Cols  ) x = pDoc->m_TextRam.m_Cols  - 1; else if ( x < 0 ) x = 0;
		if ( y >= pDoc->m_TextRam.m_Lines ) y = pDoc->m_TextRam.m_Lines - 1; else if ( y < 0 ) y = 0;

		if ( pDoc->m_TextRam.m_MouseTrack == 6 )
			pDoc->m_TextRam.LocReport(1, 2, x, y);
		else
			pDoc->m_TextRam.UNGETSTR("\033[M%c%c%c", ' ' + 3 + (nFlags & MK_SHIFT ? pDoc->m_TextRam.m_MouseMode[2] : 0) + (nFlags & MK_CONTROL ? pDoc->m_TextRam.m_MouseMode[3] : 0), ' ' + x + 1, ' ' + y + 1);
	}
}

void CRLoginView::OnEditPaste() 
{
	HGLOBAL hData;
	WCHAR *pData;
	CBuffer tmp;
	int cr = 0;

	if ( !OpenClipboard() )
		return;

	if ( (hData = GetClipboardData(CF_UNICODETEXT)) == NULL ) {
		CloseClipboard();
		return;
	}

	if ( (pData = (WCHAR *)GlobalLock(hData)) == NULL ) {
        CloseClipboard();
        return;
    }

	for ( ; *pData != 0 ; pData++ ) {
		if ( *pData != L'\x0A' && *pData != L'\x1A' ) {
			tmp.Apend((LPBYTE)pData, sizeof(WCHAR));
			if ( *pData == L'\x0D' )
				cr++;
		}
	}

	GlobalUnlock(hData);
	CloseClipboard();

	if ( (tmp.GetSize() / sizeof(WCHAR)) > 4000 || cr > 50 ) {
		if ( MessageBox("多くの文字をペーストしようとしています\n送信しますか？", "Question", MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
	}

	SendBuffer(tmp);
}
void CRLoginView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(IsClipboardFormatAvailable(CF_TEXT) ? TRUE : FALSE);
}

void CRLoginView::OnMacroRec() 
{
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	if ( m_KeyMacFlag ) {
		CKeyMac tmp;
		tmp.SetBuf(m_KeyMacBuf.GetPtr(), m_KeyMacBuf.GetSize());
		pDoc->m_KeyMac.Add(tmp);
		pDoc->m_KeyMac.SetHisMenu(GetMainWnd());
		m_KeyMacFlag = FALSE;
	} else {
		m_KeyMacFlag = TRUE;
		m_KeyMacBuf.Clear();
	}
}
void CRLoginView::OnUpdateMacroRec(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_KeyMacFlag ? 1 : 0);
}
void CRLoginView::OnMacroPlay() 
{
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	pDoc->m_KeyMac.GetAt(0, tmp);
	SendBuffer(tmp, TRUE);
}
void CRLoginView::OnUpdateMacroPlay(CCmdUI* pCmdUI) 
{
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);
	pCmdUI->Enable(pDoc->m_KeyMac.m_Data.GetSize() > 0 ? TRUE : FALSE);
}
void CRLoginView::OnMacroHis(UINT nID) 
{
	CBuffer tmp;
	int n = nID - ID_MACRO_HIS1;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	pDoc->m_KeyMac.GetAt(n, tmp);
	SendBuffer(tmp, TRUE);

	if ( n > 0 ) {
		pDoc->m_KeyMac.Top(n);
		pDoc->m_KeyMac.SetHisMenu(GetMainWnd());
	}
}

void CRLoginView::OnEditCopy() 
{
	if ( m_ClipFlag == 6 ) {
		CRLoginDoc *pDoc = GetDocument();
		ASSERT(pDoc);
		pDoc->m_TextRam.EditCopy(m_ClipStaPos, m_ClipEndPos, IsClipRectMode());
		m_ClipFlag = 0;
		OnUpdate(this, UPDATE_CLIPERA, NULL);
	}
}
void CRLoginView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_ClipFlag == 6 ? TRUE : FALSE);
}
void CRLoginView::OnEditCopyAll()
{
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	m_ClipStaPos = pDoc->m_TextRam.GetCalcPos(0, 0 - pDoc->m_TextRam.m_HisLen + pDoc->m_TextRam.m_Lines);
//	m_ClipStaPos = pDoc->m_TextRam.GetCalcPos(0, 0);
	m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(pDoc->m_TextRam.m_Cols - 1, pDoc->m_TextRam.m_Lines - 1);
	m_ClipFlag     = 6;
	m_ClipTimer    = 0;
	m_ClipKeyFlags = 0;

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLCKCOPY) ) {
		pDoc->m_TextRam.EditCopy(m_ClipStaPos, m_ClipEndPos, IsClipRectMode());
		m_ClipFlag = 0;
	}

	OnUpdate(this, UPDATE_CLIPERA, NULL);
}

BOOL CRLoginView::PreTranslateMessage(MSG* pMsg)
{
	CRLoginDoc *pDoc = GetDocument();
	
	if ( pMsg->hwnd == m_hWnd ) {
		if ( pMsg->message == WM_KEYDOWN ) {
			if ( !OnKeyDown((UINT)pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam)) )
				return TRUE;
		} else if ( pMsg->message == WM_SYSKEYDOWN ) {
			if ( !OnKeyDown((UINT)pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam)) )
				return TRUE;
			if ( (UINT)pMsg->wParam < 256 && (pDoc->m_TextRam.m_MetaKeys[(UINT)pMsg->wParam / 32] & (1 << ((UINT)pMsg->wParam % 32))) != 0 )
				pMsg->message = WM_KEYDOWN;
		}
	}
	return CView::PreTranslateMessage(pMsg);
}

void CRLoginView::OnDropFiles(HDROP hDropInfo)
{
    int i;
	char FileName[512];
    int NameSize = sizeof(FileName);
    int FileNumber;
	CRLoginDoc *pDoc = GetDocument();
	CFileStatus st;
	BOOL doCmd = FALSE;

	if ( pDoc->m_TextRam.m_DropFileMode == 0 || pDoc->m_TextRam.m_DropFileCmd[pDoc->m_TextRam.m_DropFileMode].IsEmpty() ) {
		CView::OnDropFiles(hDropInfo);
		return;
	}

    FileNumber = DragQueryFile(hDropInfo, 0xffffffff, FileName, NameSize);

	for( i = 0 ; i < FileNumber ; i++ ) {
		DragQueryFile(hDropInfo, i, FileName, NameSize);

		if ( !CFile::GetStatus(FileName, st) || (st.m_attribute & CFile::directory) != 0 )
			continue;

		if ( pDoc->m_TextRam.m_DropFileMode == 1 ) {
			if ( pDoc->m_pBPlus == NULL )
				pDoc->m_pBPlus = new CBPlus(pDoc, AfxGetMainWnd());
			if ( pDoc->m_pBPlus->m_ResvPath.IsEmpty() && !pDoc->m_pBPlus->m_ThreadFlag )
				doCmd = TRUE;
			pDoc->m_pBPlus->m_ResvPath.AddTail(FileName);
		} else if ( pDoc->m_TextRam.m_DropFileMode == 5 ) {
			if ( pDoc->m_pSock != NULL && pDoc->m_pSock->m_Type == 3 && ((Cssh *)(pDoc->m_pSock))->m_SSHVer == 2 )
				((Cssh *)(pDoc->m_pSock))->OpenRcpUpload(FileName);
		} else if ( pDoc->m_TextRam.m_DropFileMode == 6 ) {
			if ( pDoc->m_pKermit == NULL )
				pDoc->m_pKermit = new CKermit(pDoc, AfxGetMainWnd());
			if ( pDoc->m_pKermit->m_ResvPath.IsEmpty() && !pDoc->m_pKermit->m_ThreadFlag )
				doCmd = TRUE;
			pDoc->m_pKermit->m_ResvPath.AddTail(FileName);
		} else if ( pDoc->m_TextRam.m_DropFileMode >= 2 ) {
			if ( pDoc->m_pZModem == NULL )
				pDoc->m_pZModem = new CZModem(pDoc, AfxGetMainWnd());
			if ( pDoc->m_pZModem->m_ResvPath.IsEmpty() && !pDoc->m_pZModem->m_ThreadFlag )
				doCmd = TRUE;
			pDoc->m_pZModem->m_ResvPath.AddTail(FileName);
		}
	}

	if ( doCmd )
		pDoc->DoDropFile();

	CView::OnDropFiles(hDropInfo);
}

BOOL CRLoginView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ( pWnd == this && nHitTest == HTCLIENT ) {
		CRLoginDoc *pDoc = GetDocument();
		::SetCursor(::LoadCursor(NULL, (pDoc->m_TextRam.m_MouseTrack > 0 && !m_MouseEventFlag ? IDC_IBEAM : (m_BroadCast ? IDC_SIZEALL : IDC_ARROW))));
		return TRUE;
	}
	return CView::OnSetCursor(pWnd, nHitTest, message);
}

void CRLoginView::OnMouseEvent()
{
	m_MouseEventFlag = (m_MouseEventFlag ? FALSE : TRUE);
	OnUpdate(NULL, UPDATE_SETCURSOR, NULL);
}
void CRLoginView::OnUpdateMouseEvent(CCmdUI *pCmdUI)
{
	CRLoginDoc *pDoc = GetDocument();
	pCmdUI->Enable(pDoc->m_TextRam.m_MouseTrack > 0 ? TRUE : FALSE);
	pCmdUI->SetCheck(m_MouseEventFlag ? TRUE : FALSE);
}

void CRLoginView::OnBroadcast()
{
	m_BroadCast = (m_BroadCast ? FALSE : TRUE);
}
void CRLoginView::OnUpdateBroadcast(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_BroadCast);
}

void CRLoginView::OnPagePrior()
{
	OnVScroll(SB_PAGEUP, 0, NULL);
}
void CRLoginView::OnPageNext()
{
	OnVScroll(SB_PAGEDOWN, 0, NULL);
}
