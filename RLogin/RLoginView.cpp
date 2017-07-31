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
#include "SearchDlg.h"
#include "Script.h"
#include "AnyPastDlg.h"

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
	ON_MESSAGE(WM_IME_REQUEST, OnImeRequest)
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
	ON_WM_KEYUP()
	ON_COMMAND(IDM_SEARCH_REG, &CRLoginView::OnSearchReg)
	ON_COMMAND(IDM_SEARCH_NEXT, &CRLoginView::OnSearchNext)
	ON_COMMAND(IDM_SEARCH_BACK, &CRLoginView::OnSearchBack)
	ON_COMMAND(ID_GOZIVIEW, &CRLoginView::OnGoziview)
	ON_UPDATE_COMMAND_UI(ID_GOZIVIEW, &CRLoginView::OnUpdateGoziview)
	ON_COMMAND(ID_SPLIT_HEIGHT, &CRLoginView::OnSplitHeight)
	ON_COMMAND(ID_SPLIT_WIDTH, &CRLoginView::OnSplitWidth)
	ON_COMMAND(ID_SPLIT_OVER, &CRLoginView::OnSplitOver)
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
	m_GoziView  = FALSE;
	m_GoziStyle = (8 << 4) | 9;
	m_GoziCount = 4 + rand() % 28;
	m_GoziPos.SetPoint(0, 0);
	m_PastNoCheck = FALSE;

#ifdef	USE_DIRECTWRITE
	m_pRenderTarget = NULL;
#endif
}

CRLoginView::~CRLoginView()
{
	if ( m_pGhost != NULL )
		m_pGhost->DestroyWindow();

#ifdef	USE_DIRECTWRITE
	if ( m_pRenderTarget != NULL )
		m_pRenderTarget->Release();
#endif
}

BOOL CRLoginView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS;

	cs.lpszName = _T("RLoginView");
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW)); //, CreateSolidBrush(0xff000000));

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスの描画

void CRLoginView::OnDraw(CDC* pDC)
{
	CRLoginDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if ( (m_DispCaret & FGCARET_CREATE) != 0 )
		HideCaret();

	int sx = 0;
	int sy = 0;
	int ex = m_Cols  - 1;
	int ey = m_Lines - 1;

#ifdef	USE_DIRECTWRITE
	if ( m_pRenderTarget == NULL ) {
		CRect rect;
		D2D1_SIZE_U size;
		CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
        rtProps.usage =  D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

		GetClientRect(rect);
		size = D2D1::SizeU(rect.Width(), rect.Height());

		if ( pApp->m_pD2DFactory != NULL && pApp->m_pDWriteFactory != NULL ) {
			if ( SUCCEEDED(pApp->GetD2D1Factory()->CreateHwndRenderTarget(rtProps, D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_pRenderTarget)) ) {
	            m_pRenderTarget->QueryInterface(__uuidof(ID2D1GdiInteropRenderTarget), (void**)&m_pGDIRT);
			}
		}
	}
#endif

	if ( !pDC->IsPrinting() ) {
		CRect rect(((CPaintDC *)(pDC))->m_ps.rcPaint);
		sx = (rect.left + 1) * m_Cols / m_Width;
		ex = (rect.right + m_CharWidth) * m_Cols / m_Width;
		sy = (rect.top + 1) * m_Lines / m_Height;
		ey = (rect.bottom + m_CharHeight) * m_Lines / m_Height;

		if ( (sx * m_Width / m_Cols) > rect.left )
			sx--;
		if ( (ex * m_Width / m_Cols) < rect.right )
			ex++;
		if ( (sy * m_Height / m_Lines) > rect.top )
			sy--;
		if ( (ey * m_Height / m_Lines) < rect.bottom )
			ey++;

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
	else {
		CRect rect;

		if ( !pDC->IsPrinting() )
			rect = ((CPaintDC *)(pDC))->m_ps.rcPaint;
		else
			GetClientRect(rect);

		pDC->FillSolidRect(&((CPaintDC *)(pDC))->m_ps.rcPaint, GetSysColor(COLOR_APPWORKSPACE));
	}

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLTEKINWND) ) {
		CRect rect;
		GetClientRect(rect);
		pDoc->m_TextRam.TekDraw(pDC, rect);
	}

#ifdef	USE_DIRECTWRITE
	if ( m_pRenderTarget != NULL ) {
		m_pRenderTarget->BeginDraw();

		// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

		HRESULT hr = m_pRenderTarget->EndDraw();

		if ( SUCCEEDED(hr) && hr == D2DERR_RECREATE_TARGET ) {
			m_pRenderTarget->Release();
			m_pRenderTarget = NULL;
		}
	}
#endif

#ifdef	USE_GOZI
	if ( m_GoziView ) {
		CMainFrame *pMain = GetMainWnd();
		if ( pMain != NULL && pMain->m_ImageGozi.m_hImageList != NULL )
			pMain->m_ImageGozi.Draw(pDC, m_GoziStyle >> 4, m_GoziPos, ILD_NORMAL);
	}
#endif

	if ( (m_DispCaret & FGCARET_CREATE) != 0 )
		ShowCaret();

	pDoc->ClearActCount();

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

void CRLoginView::CalcPosRect(CRect &rect)
{
	int x, y;
	CRLoginDoc *pDoc = GetDocument();

	pDoc->m_TextRam.SetCalcPos(m_ClipStaPos, &x, &y);
	rect.left = x;
	rect.top = y;

	pDoc->m_TextRam.SetCalcPos(m_ClipEndPos, &x, &y);
	rect.right = x;
	rect.bottom = y;

	rect.NormalizeRect();

	rect.right  += 2;
	rect.bottom += 1;
}
void CRLoginView::InvalidateTextRect(CRect rect)
{
	CalcTextRect(rect);

	if ( rect.top >= m_Height || rect.bottom <= 0 )
		return;

	if ( rect.top < 0 )
		rect.top = 0;

	if ( rect.bottom > m_Height )
		rect.bottom = m_Height;

	InvalidateRect(rect, FALSE);

	if ( m_pGhost != NULL )
		m_pGhost->InvalidateRect(rect, FALSE);
}
void CRLoginView::CalcTextRect(CRect &rect)
{
	rect.left   = m_Width * rect.left  / m_Cols;
	rect.right  = m_Width * rect.right / m_Cols;
	rect.top    = m_Height * (rect.top    + m_HisOfs - m_HisMin) / m_Lines;
	rect.bottom = m_Height * (rect.bottom + m_HisOfs - m_HisMin) / m_Lines;
}
void CRLoginView::CalcGrapPoint(CPoint po, int *x, int *y)
{
	if      ( po.x < 0 )			po.x = 0;
	else if ( po.x >= m_Width )		po.x = m_Width -1;

	if      ( po.y < 0 )			po.y = 0;
	else if ( po.y >= m_Height )	po.y = m_Height - 1;

	*x = m_Cols  * po.x / m_Width;
	*y = m_Lines * po.y / m_Height - m_HisOfs + m_HisMin;
}
void CRLoginView::SendBroadCastMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CWinApp *pApp;
	CRLoginDoc *pThisDoc = GetDocument();
	static volatile BOOL inuse = FALSE;

	if ( inuse || (pApp = AfxGetApp()) == NULL )
		return;

	inuse = TRUE;
	POSITION pos = pApp->GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = pApp->GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
			POSITION vpos = pDoc->GetFirstViewPosition();
			while ( vpos != NULL ) {
				CRLoginView *pView = (CRLoginView *)pDoc->GetNextView(vpos);
				if ( pView != this )
					pView->OnMouseWheel(nFlags, zDelta, pt);
			}
		}
	}
	inuse = FALSE;
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
		if ( m_BroadCast ) {
			m_BroadCast = FALSE;
			SendBroadCast(buf);
			m_BroadCast = TRUE;
		}
	}

	if ( m_KeyMacFlag )
		m_KeyMacBuf.Apend(buf.GetPtr(), buf.GetSize());

	if ( pDoc->m_TextRam.IsOptEnable(TO_ANSIKAM) )
		return;

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

	pDoc->m_TextRam.m_IConv.StrToRemote(pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode], &buf, &tmp);
	pDoc->SocketSend(tmp.GetPtr(), tmp.GetSize());

	if ( !pDoc->m_TextRam.IsOptEnable(TO_ANSISRM) ) {
		pDoc->m_TextRam.PUTSTR(tmp.GetPtr(), tmp.GetSize());
		pDoc->m_TextRam.FLUSH();
	}
}
void CRLoginView::SetCaret()
{
	CPoint po(m_CaretX, m_CaretY);
	CRLoginDoc *pDoc = GetDocument();

	// 001 = CreateCaret Flag, 002 = CurSol ON/OFF, 004 = Focus Flag, 010 = Redraw Caret

	if ( (m_DispCaret & FGCARET_REDRAW) != 0 ) {
		m_DispCaret &= ~FGCARET_REDRAW;
		if ( (m_DispCaret & FGCARET_CREATE) != 0 ) {
			DestroyCaret();
			m_DispCaret &= ~FGCARET_CREATE;
		}
	}

	switch(m_DispCaret) {
	case FGCARET_FOCUS | FGCARET_ONOFF:					// 006
		switch(pDoc->m_TextRam.m_TypeCaret) {
		case 0: case 1: case 2:
			CreateSolidCaret(m_CharWidth, m_CharHeight);
			break;
		case 3: case 4:
			CreateSolidCaret(m_CharWidth, 1);
			break;
		case 5: case 6:
			CreateSolidCaret(1, m_CharHeight);
			break;
		}
		if ( pDoc->m_TextRam.m_TypeCaret == 3 || pDoc->m_TextRam.m_TypeCaret == 4 )
			po.y += m_CharHeight;
		SetCaretPos(po);
		ImmSetPos(m_CaretX, m_CaretY);
		ShowCaret();
		m_DispCaret |= FGCARET_CREATE;
		break;
	case FGCARET_FOCUS | FGCARET_ONOFF | FGCARET_CREATE:	//	007:
		if ( pDoc->m_TextRam.m_TypeCaret == 3 || pDoc->m_TextRam.m_TypeCaret == 4 )
			po.y += m_CharHeight;
		SetCaretPos(po);
		ImmSetPos(m_CaretX, m_CaretY);
		break;
	case FGCARET_FOCUS:									// 004:
		ImmSetPos(m_CaretX, m_CaretY);
		break;
	case FGCARET_FOCUS | FGCARET_CREATE:					// 005:
		ImmSetPos(m_CaretX, m_CaretY);
		// no break
	case FGCARET_CREATE:									// 001:
	case FGCARET_ONOFF | FGCARET_CREATE:					// 003:
		DestroyCaret();
		m_DispCaret &= ~FGCARET_CREATE;
		break;
	}
}
void CRLoginView::ImmSetPos(int x, int y)
{
	HIMC hIMC;
	COMPOSITIONFORM cpf;
	LOGFONT LogFont;

	if ( (hIMC = ImmGetContext(m_hWnd)) == NULL )
		return;

	CRLoginDoc *pDoc = GetDocument();
	LPCTSTR fontName = pDoc->m_TextRam.m_FontTab[SET_94x94 | '@'].m_FontName[0];

	if ( ImmGetCompositionFont(hIMC, &LogFont) && (LogFont.lfWidth != m_CharWidth || LogFont.lfHeight != m_CharHeight || _tcscmp(LogFont.lfFaceName, fontName) != 0) ) {
		LogFont.lfWidth  = m_CharWidth;
		LogFont.lfHeight = m_CharHeight;
		_tcsncpy(LogFont.lfFaceName, fontName, sizeof(LogFont.lfFaceName) / sizeof(TCHAR));
		ImmSetCompositionFont(hIMC, &LogFont);
	}

	cpf.dwStyle = CFS_POINT;
	cpf.ptCurrentPos.x = x;
	cpf.ptCurrentPos.y = y;
	ImmSetCompositionWindow(hIMC, &cpf);
	ImmReleaseContext(m_hWnd, hIMC);
}
int CRLoginView::ImmOpenCtrl(int sw)
{
	HIMC hIMC;
	int rt = (-1);

	if ( (hIMC = ImmGetContext(m_hWnd)) != NULL ) {
		rt = (ImmGetOpenStatus(hIMC) ? 1 : 0);
		if ( sw == 0 && rt == 1 ) {
			if ( ImmSetOpenStatus(hIMC, FALSE) )	// close
				rt = 0;
		} else if ( sw == 1 && rt == 0 ) {
			if ( ImmSetOpenStatus(hIMC, TRUE) )		// open
				rt = 1;
		}
	}
	return rt;
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
int CRLoginView::GetClipboad(CBuffer *bp)
{
	HGLOBAL hData;
	TCHAR *pData;
	CStringA buf;
	CRLoginDoc *pDoc = GetDocument();

	if ( !OpenClipboard() )
		return FALSE;

#ifdef	_UNICODE
	if ( (hData = GetClipboardData(CF_UNICODETEXT)) == NULL ) {
#else
	if ( (hData = GetClipboardData(CF_TEXT)) == NULL ) {
#endif
		CloseClipboard();
		return FALSE;
	}

	if ( (pData = (TCHAR *)GlobalLock(hData)) == NULL ) {
        CloseClipboard();
        return FALSE;
    }

	pDoc->m_TextRam.m_IConv.StrToRemote(pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode], pData, buf);
	bp->Apend((LPBYTE)(LPCSTR)buf, buf.GetLength());

	GlobalUnlock(hData);
	CloseClipboard();

	return TRUE;
}
int CRLoginView::SetClipboad(CBuffer *bp)
{
	HGLOBAL hClipData;
	TCHAR *pData;
	CString buf;
	CRLoginDoc *pDoc = GetDocument();

	pDoc->m_TextRam.m_IConv.RemoteToStr(pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode], (LPCSTR)*bp, buf);

	if ( (hClipData = GlobalAlloc(GMEM_MOVEABLE, (buf.GetLength() + 1) * sizeof(TCHAR))) == NULL )
		return FALSE;

	if ( (pData = (TCHAR *)GlobalLock(hClipData)) == NULL )
		goto ENDOF;

	_tcscpy(pData, buf);
	GlobalUnlock(hClipData);

	if ( !OpenClipboard() )
		goto ENDOF;

	if ( !EmptyClipboard() ) {
		CloseClipboard();
		goto ENDOF;
	}

#ifdef	_UNICODE
	SetClipboardData(CF_UNICODETEXT, hClipData);
#else
	SetClipboardData(CF_TEXT, hClipData);
#endif

	CloseClipboard();
	return TRUE;

ENDOF:
	GlobalFree(hClipData);
	return FALSE;
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

	if ( (m_Width  = cx) <= 0 ) m_Width  = 1;
	if ( (m_Height = cy) <= 0 ) m_Height = 1;

	pFrame->GetClientRect(rect);
	pFrame->m_Width  = m_Width;
	if ( (pFrame->m_Height = rect.Height() - 4) <= 0 )
		pFrame->m_Height = 1;

//	if ( m_ActiveFlag ) {
		pDoc->m_TextRam.InitText(pFrame->m_Width, pFrame->m_Height);
		pFrame->m_Cols  = pDoc->m_TextRam.m_Cols;
		pFrame->m_Lines = pDoc->m_TextRam.m_Lines;
//	}

	pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	tmp.Format(_T("%d x %d"), pDoc->m_TextRam.m_Cols, pDoc->m_TextRam.m_Lines);
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
	CRect rect, box;
	CRLoginDoc *pDoc = GetDocument();
	CChildFrame *pFrame = GetFrameWnd();
	ASSERT(pDoc);
	ASSERT(pFrame);

	if ( lHint == UPDATE_TEKFLUSH ) {
		if ( pDoc->m_TextRam.IsOptEnable(TO_RLTEKINWND) )
			Invalidate(FALSE);
		return;
	} else if ( lHint == UPDATE_VISUALBELL ) {
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
			::SetCursor(::LoadCursor(NULL, (pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ? IDC_IBEAM : IDC_ARROW)));
		return;
	} else if ( lHint == UPDATE_TYPECARET ) {
		m_DispCaret |= FGCARET_REDRAW;
		SetCaret();
		return;
	}

	if ( (m_DispCaret & FGCARET_FOCUS) != 0 && pSender != this &&
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
		if ( (m_Cols  = m_Width  * pFrame->m_Cols  / pFrame->m_Width) <= 0 )
			m_Cols = 1;
		if ( (m_Lines = m_Height * pFrame->m_Lines / pFrame->m_Height) <= 0 )
			m_Lines = 1;

		m_HisMin = pDoc->m_TextRam.m_Lines - m_Lines;

		m_CharWidth  = pFrame->m_Width  / pFrame->m_Cols;
		m_CharHeight = pFrame->m_Height / pFrame->m_Lines;
		
		if ( (m_DispCaret & FGCARET_CREATE) != 0 ) {
			m_DispCaret &= ~FGCARET_CREATE;
			DestroyCaret();
		}

		m_BmpFile.LoadFile(pDoc->m_TextRam.m_BitMapFile);
		m_pBitmap = m_BmpFile.GetBitmap(GetDC(), m_Width, m_Height);
		//pDoc->SetStatus(NULL);

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
		CalcPosRect(rect);
		if ( IsClipRectMode() ) {
			InvalidateTextRect(rect);
		} else if ( IsClipLineMode() ) {
			rect.left  = 0;
			rect.right = pDoc->m_TextRam.m_Cols;
			InvalidateTextRect(rect);
		} else if ( rect.Height() == 1 ) {
			InvalidateTextRect(rect);
		} else {
			box.left   = rect.left;
			box.right  = pDoc->m_TextRam.m_Cols;
			box.top    = rect.top;
			box.bottom = rect.top + 1;
			InvalidateTextRect(box);

			box.left   = 0;
			box.right  = pDoc->m_TextRam.m_Cols;
			box.top    = rect.top + 1;
			box.bottom = rect.bottom;
			if ( box.top < box.bottom )
				InvalidateTextRect(box);

			box.left   = 0;
			box.right  = rect.right;
			box.top    = rect.bottom;
			box.bottom = rect.bottom + 1;
			InvalidateTextRect(box);
		}
		break;

	case UPDATE_GOTOXY:
		break;
	}

	m_DispCaret &= ~FGCARET_ONOFF;
	m_CaretX = m_Width * pDoc->m_TextRam.m_CurX / m_Cols;
	m_CaretY = m_Height * (pDoc->m_TextRam.m_CurY + m_HisOfs - m_HisMin) / m_Lines;

	if ( m_CaretX < 0 || m_CaretX >= m_Width || m_CaretY < 0 || m_CaretY >= m_Height )
		m_CaretX = m_CaretY = 0;
	else if ( pDoc->m_pSock != NULL )
		m_DispCaret |= (pDoc->m_TextRam.m_DispCaret & FGCARET_ONOFF);

	SetCaret();
}

void CRLoginView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();

//	TRACE("OnChar %02x(%04x)\n", nChar, nFlags);

	CView::OnChar(nChar, nRepCnt, nFlags);

	if ( (nFlags & 0x2000) != 0 && pDoc->m_TextRam.IsOptEnable(TO_ANSISRM) )	// with Alt key
		tmp.PutWord(0x1B);

	tmp.PutWord(nChar);
	SendBuffer(tmp);
}
BOOL CRLoginView::ModifyKeys(UINT nChar, int nStat)
{
	int n;
	int num, mod, msk = 0;
	WCHAR esc, end, *p;
	CStringW pam, fmt;
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();

	if ( nChar > 255 || (num = pDoc->m_TextRam.m_ModKeyTab[nChar]) < 0 )
		return FALSE;

	if ( (mod = pDoc->m_TextRam.m_ModKey[num]) < 0 )
		return FALSE;

	if ( !pDoc->m_TextRam.IsOptEnable(TO_RLMODKEY) && pDoc->m_KeyTab.FindMaps(nChar, nStat, &tmp) && (n = CKeyNodeTab::GetCmdsKey((LPCWSTR)tmp)) > 0 ) {
		AfxGetMainWnd()->PostMessage(WM_COMMAND, (WPARAM)n);
		return TRUE;
	}

	if ( (nStat & MASK_SHIFT) != 0 ) msk |= 001;
	if ( (nStat & MASK_ALT)   != 0 ) msk |= 002;
	if ( (nStat & MASK_CTRL)  != 0 ) msk |= 004;

	if ( !pDoc->m_KeyTab.FindMaps(nChar, (nStat & ~(MASK_SHIFT | MASK_CTRL | MASK_ALT)), &tmp) )
		goto NOTDEF;

	n = tmp.GetSize() / sizeof(WCHAR);
	p = (WCHAR *)(tmp.GetPtr());

	// CSI or SS3 ?
	if ( !(n > 2 && p[0] == L'\033' && (p[1] == L'[' || p[1] == L'O')) )
		goto NOTDEF;
	esc = p[1];

	for ( p += 2, n -= 2 ; n > 0 && *p < L'@' ; p++, n-- )
		pam += *p;
	if ( n <= 0 )
		goto NOTDEF;
	end = *p;

	if ( mod >= 2 && pam.IsEmpty() )
		pam = L"1";

	fmt.Format(L"\033%c%s%s%s%d%c",
		mod >= 1 ? L'[' : esc,
		mod >= 3 ? L">" : L"",
		pam,
		pam.IsEmpty() ? L"" : L";", 
		msk + 1, end);

	goto SENDFMT;

NOTDEF:
	if ( num != MODKEY_OTHER && num != MODKEY_STRING )
		return FALSE;

	switch(mod) {
	case (-1):
	case 0:
		return FALSE;
	case 1:
		fmt.Format(L"\033[%d;%du", nChar, msk + 1);
		break;
	case 2:
		fmt.Format(L"\033[27;%d;%d~", msk + 1, nChar);
		break;
	case 3:
		fmt.Format(L"\033[>27;%d;%d~", msk + 1, nChar);
		break;
	}

SENDFMT:
	tmp.Clear();
	tmp.Apend((LPBYTE)(LPCWSTR)fmt, fmt.GetLength() * sizeof(WCHAR));
	SendBuffer(tmp);

	return TRUE;
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

	if ( nChar == VK_MENU )
		return TRUE;
	else if ( nChar == VK_SHIFT || nChar == VK_CONTROL ) {
		if ( m_ClipFlag > 0 && m_ClipFlag < 6 ) {
			OnUpdate(this, UPDATE_CLIPERA, NULL);
			switch(nChar) {
			case VK_SHIFT:	 m_ClipKeyFlags |= MK_SHIFT; break;
			case VK_CONTROL: m_ClipKeyFlags |= MK_CONTROL; break;
			}
			OnUpdate(this, UPDATE_CLIPERA, NULL);
		}
		return TRUE;
	}

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
	if ( pDoc->m_TextRam.IsOptEnable(TO_RLPNAM) )
		st |= MASK_APPL;

	if ( !pDoc->m_TextRam.IsOptEnable(TO_DECANM) )
		st |= MASK_VT52;
	else if ( pDoc->m_TextRam.IsOptEnable(TO_DECCKM) )
		st |= MASK_CKM;

	if ( nChar == VK_ESCAPE && !pDoc->m_TextRam.IsOptEnable(TO_RLCKMESC) )
		st &= ~MASK_CKM;

	//if ( (GetKeyState(VK_NUMLOCK) & 0x01) != 0 )
	//	st |= MASK_NUMLCK;
	//if ( (GetKeyState(VK_SCROLL) & 0x01) != 0 )
	//	st |= MASK_SCRLCK;
	//if ( (GetKeyState(VK_CAPITAL) & 0x01) != 0 )
	//	st |= MASK_CAPLCK;

	//TRACE("OnKey %02x(%02x)\n", nChar, st);

	if ( nChar >= VK_F1 && nChar <= VK_F24 && pDoc->m_TextRam.m_FuncKey[nChar - VK_F1].GetSize() > 0 ) {
		pDoc->SocketSend(pDoc->m_TextRam.m_FuncKey[nChar - VK_F1].GetPtr(), pDoc->m_TextRam.m_FuncKey[nChar - VK_F1].GetSize());
		return FALSE;
	}

	if ( (st & (MASK_SHIFT | MASK_CTRL | MASK_ALT)) != 0 && ModifyKeys(nChar, st) )
		return FALSE;

	if ( (GetKeyState(VK_NUMLOCK) & 0x01) != 0 && nChar >= VK_NUMPAD0 && nChar <= VK_DIVIDE )
		return TRUE;

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
void CRLoginView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_SHIFT || nChar == VK_CONTROL ) {
		if ( m_ClipFlag > 0 && m_ClipFlag < 6 ) {
			OnUpdate(this, UPDATE_CLIPERA, NULL);
			switch(nChar) {
			case VK_SHIFT:	 m_ClipKeyFlags &= ~MK_SHIFT; break;
			case VK_CONTROL: m_ClipKeyFlags &= ~MK_CONTROL; break;
			}
			OnUpdate(this, UPDATE_CLIPERA, NULL);
		}
	}
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CRLoginView::OnSetFocus(CWnd* pOldWnd) 
{
	CRLoginDoc *pDoc = GetDocument();

	CView::OnSetFocus(pOldWnd);

	m_DispCaret |= FGCARET_FOCUS;
	SetCaret();

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTFOCEVT) )
		pDoc->m_TextRam.UNGETSTR(_T("%sI"), pDoc->m_TextRam.m_RetChar[RC_CSI]);
}
void CRLoginView::OnKillFocus(CWnd* pNewWnd) 
{
	CRLoginDoc *pDoc = GetDocument();

	CView::OnKillFocus(pNewWnd);

	m_DispCaret &= ~FGCARET_FOCUS;
	SetCaret();

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTFOCEVT) )
		pDoc->m_TextRam.UNGETSTR(_T("%sO"), pDoc->m_TextRam.m_RetChar[RC_CSI]);
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
		pDoc->m_pScript->SetMenu(GetMainWnd());
		pDoc->m_TextRam.InitText(pFrame->m_Width, pFrame->m_Height);
		pFrame->m_Cols  = pDoc->m_TextRam.m_Cols;
		pFrame->m_Lines = pDoc->m_TextRam.m_Lines;
		pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
	} else if ( !bActivate && pDeactiveView->m_hWnd == m_hWnd )
		m_ActiveFlag = FALSE;
}

LRESULT CRLoginView::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
	CRLoginDoc *pDoc = GetDocument();

	switch(wParam) {
		/********
	case IMN_SETCOMPOSITIONWINDOW:
		HIMC hIMC;
		LOGFONT LogFont;
		if ( (hIMC = ImmGetContext(m_hWnd)) != NULL ) {
			if ( ImmGetCompositionFont(hIMC, &LogFont) ) {
				LogFont.lfWidth  = m_CharWidth;
				LogFont.lfHeight = m_CharHeight;
				_tcsncpy(LogFont.lfFaceName, pDoc->m_TextRam.m_FontTab[SET_94x94 | '@'].m_FontName[0], sizeof(LogFont.lfFaceName) / sizeof(TCHAR));
				ImmSetCompositionFont(hIMC, &LogFont);
			}
			ImmReleaseContext(m_hWnd, hIMC);
	    }
		return TRUE;
		*********/
    case IMN_SETSTATUSWINDOWPOS:
		ImmSetPos(m_CaretX, m_CaretY);
		return TRUE;
    default:
		return DefWindowProc(WM_IME_NOTIFY, wParam, lParam);
    }
}
LRESULT CRLoginView::OnImeComposition(WPARAM wParam, LPARAM lParam)
{
	HIMC hIMC;
	LONG len;
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();

	if ( (lParam & GCS_RESULTSTR) != 0 && (hIMC = ImmGetContext(m_hWnd)) != NULL ) {
		len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
		len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, tmp.PutSpc(len), len);
		ImmReleaseContext(m_hWnd, hIMC);
		SendBuffer(tmp);
		return TRUE;

	} else
		return DefWindowProc(WM_IME_COMPOSITION, wParam, lParam);
}
afx_msg LRESULT CRLoginView::OnImeRequest(WPARAM wParam, LPARAM lParam)
{
	int pos, len, cmp;
	HIMC hIMC;
	RECONVERTSTRING *pReConvStr = (RECONVERTSTRING*)lParam;
	CRLoginDoc *pDoc = GetDocument();
	CBuffer tmp;
	CString str;

	switch(wParam) {
	case IMR_DOCUMENTFEED:
#ifdef	_UNICODE
		pDoc->m_TextRam.GetVram(0, pDoc->m_TextRam.m_CurX - 1, pDoc->m_TextRam.m_CurY, pDoc->m_TextRam.m_CurY, &tmp);
		pos = tmp.GetSize();

		if ( (hIMC = ImmGetContext(m_hWnd)) != NULL ) {
			if ( (len = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0)) > 0 )
				ImmGetCompositionStringW(hIMC, GCS_COMPSTR, tmp.PutSpc(len), len);
			ImmReleaseContext(m_hWnd, hIMC);
			cmp = tmp.GetSize() - pos;
		} else
			cmp = 0;

		pDoc->m_TextRam.GetVram(pDoc->m_TextRam.m_CurX, pDoc->m_TextRam.m_Cols - 1, pDoc->m_TextRam.m_CurY, pDoc->m_TextRam.m_CurY, &tmp);
		len = tmp.GetSize();

		tmp.PutWord(0);

		if ( pReConvStr != NULL ) {
			pReConvStr->dwStrLen          = len / sizeof(WCHAR);
			pReConvStr->dwStrOffset       = sizeof(RECONVERTSTRING);

			pReConvStr->dwTargetStrLen    = cmp / sizeof(WCHAR);
			pReConvStr->dwTargetStrOffset = pos;

			pReConvStr->dwCompStrLen      = cmp / sizeof(WCHAR);
			pReConvStr->dwCompStrOffset   = pos;

			if ( pReConvStr->dwSize >= (sizeof(RECONVERTSTRING) + tmp.GetSize()) )
				memcpy((char *)pReConvStr + sizeof(RECONVERTSTRING), tmp.GetPtr(), tmp.GetSize());
		}

		return sizeof(RECONVERTSTRING) + tmp.GetSize();
#else
		pDoc->m_TextRam.GetVram(0, pDoc->m_TextRam.m_CurX - 1, pDoc->m_TextRam.m_CurY, pDoc->m_TextRam.m_CurY, &tmp);
		str = (LPCWSTR)tmp;
		pos = str.GetLength();

		if ( (hIMC = ImmGetContext(m_hWnd)) != NULL ) {
			tmp.Clear();
			if ( (len = ImmGetCompositionString(hIMC, GCS_COMPSTR, NULL, 0)) > 0 )
				ImmGetCompositionString(hIMC, GCS_COMPSTR, tmp.PutSpc(len), len);
			ImmReleaseContext(m_hWnd, hIMC);
			cmp = tmp.GetSize();
			str += (LPCSTR)tmp;
		} else
			cmp = 0;

		tmp.Clear();
		pDoc->m_TextRam.GetVram(pDoc->m_TextRam.m_CurX, pDoc->m_TextRam.m_Cols - 1, pDoc->m_TextRam.m_CurY, pDoc->m_TextRam.m_CurY, &tmp);
		str += (LPCWSTR)tmp;
		len = str.GetLength();

		if ( pReConvStr != NULL ) {
			pReConvStr->dwStrLen          = len;
			pReConvStr->dwStrOffset       = sizeof(RECONVERTSTRING);

			pReConvStr->dwTargetStrLen    = cmp;
			pReConvStr->dwTargetStrOffset = pos;

			pReConvStr->dwCompStrLen      = cmp;
			pReConvStr->dwCompStrOffset   = pos;

			if ( pReConvStr->dwSize >= (sizeof(RECONVERTSTRING) + str.GetLength() + 1) )
				strcpy((char *)pReConvStr + sizeof(RECONVERTSTRING), str);
		}

		return sizeof(RECONVERTSTRING) + (str.GetLength() + 1) * sizeof(CHAR);
#endif
	default:
		return DefWindowProc(WM_IME_REQUEST, wParam, lParam);
	}
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
	int style, anime, move;
	CRect rect;
	CRLoginDoc *pDoc = GetDocument();

	ASSERT(pDoc != NULL);
	ASSERT(m_hWnd != NULL);

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

	case 1028:		// Gozi Timer
		rect.SetRect(m_GoziPos.x, m_GoziPos.y, m_GoziPos.x + 32, m_GoziPos.y + 32);
		InvalidateRect(rect, FALSE);

		if ( !m_GoziView ) {
			KillTimer(1028);
			break;
		}

		move  = m_GoziStyle & 0x0F;
		style = m_GoziStyle >> 4;
		anime = style & 3;
		style &= ~3;

		if ( (move & 1) != 0 ) m_GoziPos.x += 8;
		if ( (move & 2) != 0 ) m_GoziPos.x -= 8;
		if ( (move & 4) != 0 ) m_GoziPos.y += 8;
		if ( (move & 8) != 0 ) m_GoziPos.y -= 8;

		if ( m_GoziPos.x < 0 ) { m_GoziPos.x = 0; move ^=  3; }
		if ( m_GoziPos.y < 0 ) { m_GoziPos.y = 0; move ^= 12; }

		GetClientRect(rect);
		if ( (m_GoziPos.x + 32) > rect.right  ) { m_GoziPos.x = rect.right  - 32; move ^= 3; }
		if ( (m_GoziPos.y + 32) > rect.bottom ) { m_GoziPos.y = rect.bottom - 32; move ^= 12; }

		if ( --m_GoziCount < 0 ) {
			m_GoziCount = 4 + rand() % 28;
			move = rand() % 16;
		}

		// 0-3=Down, 4-7=Up, 8-11=Right, 12-15=Left, 16-19=SRight, 20-23=SLeft, 24-27=SFire 
		switch(move) {
		case  0: style = 16; break; // -	-
		case  1: style =  8; break; // -	Right
		case  2: style = 12; break; // -	Left
		case  3: style = 16; break; // -	-
		case  4: style =  4; break; // Up	-
		case  5: style =  8; break; // Up	Right
		case  6: style = 12; break; // Up	Left
		case  7: style =  4; break; // Up	-
		case  8: style =  0; break; // Down -
		case  9: style =  8; break; // Down Right
		case 10: style = 12; break; // Down Left
		case 11: style =  0; break; // Down -
		case 12: style = 20; break; // -	-
		case 13: style =  8; break; // -	Right
		case 14: style = 12; break;	// -	Left
		case 15: style = 24; break;	// -	-
		}

		if ( style == 24 && pDoc != NULL && pDoc->m_TextRam.IsInitText() ) {
			int x, y;
			CVram *vp;
			CalcGrapPoint(m_GoziPos, &x, &y);
			x -= (rand() % 6);
			y += (rand() % 4);
			if ( x > 0 && x < m_Cols && y < m_Lines ) {
				pDoc->m_TextRam.GETVRAM(x, y )->pr.bc = 1 + (rand() % 3) * 2;	// 1,3,5
				CRect rect(x, y, x + 1, y + 1);
				InvalidateTextRect(rect);
			}
		}

		if ( ++anime > 3 ) anime = 0;
		m_GoziStyle = ((style + anime) << 4) | move;

		rect.SetRect(m_GoziPos.x, m_GoziPos.y, m_GoziPos.x + 32, m_GoziPos.y + 32);
		InvalidateRect(rect, FALSE);
		break;
	}
}

BOOL CRLoginView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	int pos, ofs;
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	if ( (nFlags & MK_SHIFT) != 0 )
		SendBroadCastMouseWheel(nFlags, zDelta, pt);

	ofs = zDelta * pDoc->m_TextRam.m_WheelSize / (WHEEL_DELTA / 2);

	if ( (!pDoc->m_TextRam.IsOptEnable(TO_RLMSWAPP) && pDoc->m_TextRam.IsOptEnable(TO_DECCKM)) ||
					(pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag) || (nFlags & MK_CONTROL) != 0 || pDoc->m_TextRam.IsOptEnable(TO_RLMSWAPE) ) {
		if ( pDoc->m_KeyTab.FindMaps((ofs > 0 ? VK_UP : VK_DOWN), (pDoc->m_TextRam.IsOptEnable(TO_DECCKM) ? MASK_CKM : 0), &tmp) ) {
			for ( pos = (ofs < 0 ? (0 - ofs) : ofs) ; pos > 0 ; pos-- )
				SendBuffer(tmp, TRUE);
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
			if ( !pDoc->m_TextRam.IsOptEnable(TO_RLMOSWHL) && (ofs > 4 || ofs < -4) ) {
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
	m_ClipStaOld = m_ClipStaPos;
	m_ClipEndOld = m_ClipEndPos;
	m_ClipFlag = 2;
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

	if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
		pDoc->m_TextRam.MouseReport(MOS_LOCA_LEDN, nFlags, x, y);

		if ( pDoc->m_TextRam.m_MouseTrack == MOS_EVENT_HILT ) {
			m_ClipStaPos = m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
			m_ClipFlag     = 6;
			m_ClipKeyFlags = 0;
		}

		return;
	}

	m_ClipStaPos   = m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
	m_ClipFlag     = 1;
	m_ClipTimer    = 0;
	m_ClipKeyFlags = nFlags;

	if ( IsClipLineMode() )
		OnUpdate(this, UPDATE_CLIPERA, NULL);
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
		if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
			pDoc->m_TextRam.MouseReport(MOS_LOCA_LEUP, nFlags, x, y);

			if ( pDoc->m_TextRam.m_MouseTrack == MOS_EVENT_HILT ) {
				m_ClipFlag = 0;
				OnUpdate(this, UPDATE_CLIPERA, NULL);
			}
		}
		return;
	}

	OnUpdate(this, UPDATE_CLIPERA, NULL);

	m_ClipKeyFlags = nFlags;

	if ( point.x < (0 - m_CharWidth * 3) || point.x > (m_Width + m_CharWidth * 3) )
		m_ClipKeyFlags |= 0x1000;
	else
		m_ClipKeyFlags &= 0xEFFF;

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

	if ( m_ClipFlag == 1 && m_ClipStaPos == m_ClipEndPos && !IsClipLineMode() ) {
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
	int x, y, pos, tos;
	CRLoginDoc *pDoc = GetDocument();
	ASSERT(pDoc);

	CView::OnMouseMove(nFlags, point);

	if ( m_ClipFlag == 0 || m_ClipFlag == 6 ) {
		if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
			CalcGrapPoint(point, &x, &y);
			pDoc->m_TextRam.MouseReport(MOS_LOCA_MOVE, nFlags, x, y);

			if ( pDoc->m_TextRam.m_MouseTrack == MOS_EVENT_HILT ) {
				OnUpdate(this, UPDATE_CLIPERA, NULL);
				m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
				OnUpdate(this, UPDATE_CLIPERA, NULL);
			}
		}
		return;
	}

	if ( m_ClipFlag != 1 )
		OnUpdate(this, UPDATE_CLIPERA, NULL);

	m_ClipKeyFlags = nFlags;

	if ( point.x < (0 - m_CharWidth * 3) || point.x > (m_Width + m_CharWidth * 3) )
		m_ClipKeyFlags |= 0x1000;
	else
		m_ClipKeyFlags &= 0xEFFF;

	if ( point.y < 0 || point.y > m_Height ) {
		m_ClipSavePoint = point;
		if ( m_ClipTimer == 0 )
			m_ClipTimer = (UINT)SetTimer(1024, 100, NULL);
		if ( point.y < (0 - m_CharHeight * 3) )
			OnVScroll(SB_PAGEUP, 0, NULL);
		else if ( point.y < 0 )
			OnVScroll(SB_LINEUP, 0, NULL);
		else if ( point.y < (m_Height + m_CharHeight * 3) )
			OnVScroll(SB_LINEDOWN, 0, NULL);
		else
			OnVScroll(SB_PAGEDOWN, 0, NULL);
	} else {
		if ( m_ClipTimer != 0 )
			KillTimer(m_ClipTimer);
		m_ClipTimer = 0;
	}

	CalcGrapPoint(point, &x, &y);
	pos = tos = pDoc->m_TextRam.GetCalcPos(x, y);

	switch(m_ClipFlag) {
	case 1:		// clip start
		if ( pos < m_ClipStaPos ) {
			m_ClipFlag = 4;
			m_ClipStaPos = pos;
		} else if ( pos > m_ClipEndPos ) {
			m_ClipFlag = 3;
			m_ClipEndPos = pos;
		}
		break;

	case 2:		// word clip start pos
		if ( pos < m_ClipStaPos ) {
			pDoc->m_TextRam.EditWordPos(&pos, &tos);
			m_ClipStaPos = pos;
		} else if ( pos > m_ClipEndPos ) {
			pDoc->m_TextRam.EditWordPos(&tos, &pos);
			m_ClipEndPos = pos;
			m_ClipStaPos = m_ClipStaOld;
			m_ClipFlag = 5;
		} else {
			pDoc->m_TextRam.EditWordPos(&pos, &tos);
			if ( m_ClipStaPos < pos )
				m_ClipStaPos = pos;
		}
		break;

	case 3:		// clip end pos 
		if ( pos < m_ClipStaPos ) {
			m_ClipFlag = 4;
			m_ClipEndPos = m_ClipStaPos;
			m_ClipStaPos = pos;
		} else
			m_ClipEndPos = pos;
		break;

	case 4:		// clip start pos
		if ( pos > m_ClipEndPos ) {
			m_ClipFlag = 3;
			m_ClipStaPos = m_ClipEndPos;
			m_ClipEndPos = pos;
		} else
			m_ClipStaPos = pos;
		break;

	case 5:		// word clip end pos
		if ( pos < m_ClipStaPos ) {
			pDoc->m_TextRam.EditWordPos(&pos, &tos);
			m_ClipStaPos = pos;
			m_ClipEndPos = m_ClipEndOld;
			m_ClipFlag = 2;
		} else if ( pos > m_ClipEndPos ) {
			pDoc->m_TextRam.EditWordPos(&tos, &pos);
			m_ClipEndPos = pos;
		} else {
			pDoc->m_TextRam.EditWordPos(&pos, &tos);
			if ( m_ClipEndPos > tos )
				m_ClipEndPos = tos;
		}
		break;
	}

	if ( m_ClipFlag != 1 )
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

	if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
		CalcGrapPoint(point, &x, &y);
		pDoc->m_TextRam.MouseReport(MOS_LOCA_RTDN, nFlags, x, y);
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

	if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
		CalcGrapPoint(point, &x, &y);
		pDoc->m_TextRam.MouseReport(MOS_LOCA_RTUP, nFlags, x, y);
	}
}
void CRLoginView::OnEditPaste() 
{
	HGLOBAL hData;
	WCHAR *pData, *pStr;
	CBuffer tmp;
	int cr = 0;
	int ct = 0;
	CRLoginDoc *pDoc = GetDocument();

	if ( !OpenClipboard() )
		return;

	if ( (hData = GetClipboardData(CF_UNICODETEXT)) == NULL ) {
		CloseClipboard();
		return;
	}

	if ( (pStr = (WCHAR *)GlobalLock(hData)) == NULL ) {
        CloseClipboard();
        return;
    }

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTBRPAMD) )
		tmp.Apend((LPBYTE)(L"\033[200~"), 6 * sizeof(WCHAR));

	for ( pData = pStr ; *pData != 0 ; pData++ ) {
		if ( *pData != L'\x0A' && *pData != L'\x1A' ) {
			tmp.Apend((LPBYTE)pData, sizeof(WCHAR));
			if ( *pData == L'\x0D' )
				cr++;
			else if ( *pData != L'\t' && *pData < L' ' )
				ct++;
		}
	}

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTBRPAMD) )
		tmp.Apend((LPBYTE)(L"\033[201~"), 6 * sizeof(WCHAR));

	GlobalUnlock(hData);
	CloseClipboard();

	//if ( (tmp.GetSize() / sizeof(WCHAR)) > 1000 || cr > 10 || ct > 10 ) {
	//	if ( MessageBox(CStringLoad(IDE_MESSAGE_ANYPASTE), _T("Question"), MB_ICONQUESTION | MB_YESNO) != IDYES )
	//		return;
	//}

	if ( m_PastNoCheck == FALSE && ((tmp.GetSize() / sizeof(WCHAR)) > 1000 || cr > 0 || ct > 0) ) {
		CAnyPastDlg dlg;
		dlg.m_TextBox = pStr;
		if ( dlg.DoModal() != IDOK )
			return;
		m_PastNoCheck = dlg.m_NoCheck;
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
		pDoc->m_TextRam.EditCopy(m_ClipStaPos, m_ClipEndPos, IsClipRectMode(), IsClipLineMode());
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
		pDoc->m_TextRam.EditCopy(m_ClipStaPos, m_ClipEndPos);
		m_ClipFlag = 0;
	}

	OnUpdate(this, UPDATE_CLIPERA, NULL);
}

BOOL CRLoginView::PreTranslateMessage(MSG* pMsg)
{
	CRLoginDoc *pDoc = GetDocument();
	
	if ( pMsg->hwnd == m_hWnd ) {
		if ( pMsg->message == WM_KEYDOWN ) {
			if ( pMsg->wParam == VK_BACK && !pDoc->m_TextRam.IsOptEnable(TO_DECBKM) )
				pMsg->wParam = VK_DELETE;
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
	TCHAR FileName[512];
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
			if ( pDoc->m_pSock != NULL && pDoc->m_pSock->m_Type == ESCT_SSH_MAIN && ((Cssh *)(pDoc->m_pSock))->m_SSHVer == 2 )
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
		::SetCursor(::LoadCursor(NULL, (pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ? IDC_IBEAM : (m_BroadCast ? IDC_SIZEALL : IDC_ARROW))));
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
	pCmdUI->Enable(pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE ? TRUE : FALSE);
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

void CRLoginView::OnSearchReg()
{
	CSearchDlg dlg;
	CRLoginDoc *pDoc = GetDocument();

	dlg.m_pDoc = pDoc;
	dlg.m_SearchStr = pDoc->m_SearchStr;

	if ( dlg.DoModal() != IDOK ) {
		pDoc->m_TextRam.HisRegMark(NULL);
		Invalidate(FALSE);
		return;
	}

	pDoc->m_SearchStr = dlg.m_SearchStr;

	if ( !pDoc->m_TextRam.HisMarkCheck(0 - m_HisOfs + m_HisMin, m_Lines, this) )
		OnSearchBack();

	Invalidate(FALSE);
}
void CRLoginView::OnSearchBack()
{
	int pos;
	CRLoginDoc *pDoc = GetDocument();

	for ( pos = m_HisOfs + 1 ; pos <= (pDoc->m_TextRam.m_HisLen - m_Lines) ; pos++ ) {
		if ( pDoc->m_TextRam.HisMarkCheck(0 - pos + m_HisMin, 1, this) ) {
			m_HisOfs = pos;
			OnUpdate(this, UPDATE_INVALIDATE, NULL);
			break;
		}
	}
}
void CRLoginView::OnSearchNext()
{
	int pos;
	CRLoginDoc *pDoc = GetDocument();

	for ( pos = m_HisOfs - 1 ; pos >= 0 ; pos-- ) {
		if ( pDoc->m_TextRam.HisMarkCheck(0 - pos + m_HisMin + m_Lines - 1, 1, this) ) {
			m_HisOfs = pos;
			OnUpdate(this, UPDATE_INVALIDATE, NULL);
			break;
		}
	}
}

void CRLoginView::OnGoziview()
{
	if ( m_GoziView ) {
		m_GoziView = FALSE;
	} else {
		m_GoziView = TRUE;
		SetTimer(1028, 400, NULL);
	}
}
void CRLoginView::OnUpdateGoziview(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_GoziView);
}

void CRLoginView::OnSplitHeight()
{
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( pMain == NULL )
		return;

	pMain->SendMessage(WM_COMMAND, ID_PANE_HSPLIT);
	OnSplitOver();
}
void CRLoginView::OnSplitWidth()
{
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( pMain == NULL )
		return;

	pMain->SendMessage(WM_COMMAND, ID_PANE_WSPLIT);
	OnSplitOver();
}
void CRLoginView::OnSplitOver()
{
	CCommandLineInfoEx cmds;
	CRLoginDoc *pDoc = GetDocument();
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	if ( pDoc == NULL || pApp == NULL )
		return;

	pDoc->m_InPane = TRUE;
	pDoc->SetEntryProBuffer();
	pApp->m_pServerEntry = &(pDoc->m_ServerEntry);
	cmds.ParseParam(_T("inpane"), TRUE, FALSE);
	((CRLoginApp *)::AfxGetApp())->OpenProcsCmd(&cmds);
}
