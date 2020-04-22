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
#include "GrapWnd.h"
#include "MsgWnd.h"

#include <imm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef	USE_OLE
/////////////////////////////////////////////////////////////////////////////
// CViewDropTarget

CViewDropTarget::CViewDropTarget()
{
}
CViewDropTarget::~CViewDropTarget()
{
}
DROPEFFECT CViewDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return OnDragOver(pWnd, pDataObject, dwKeyState, point);
}
DROPEFFECT CViewDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	if ( pDataObject->IsDataAvailable(CF_HDROP) || pDataObject->IsDataAvailable(CF_FILEDESCRIPTOR) )
		return DROPEFFECT_COPY;

	return DROPEFFECT_NONE;
}
BOOL CViewDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	HGLOBAL hData = NULL;

	if ( pDataObject->IsDataAvailable(CF_HDROP) ) {
		if ( (hData = pDataObject->GetGlobalData(CF_HDROP)) == NULL )
			return FALSE;

		pWnd->SendMessage(WM_DROPFILES, (WPARAM)hData);
		return TRUE;

	} else if ( pDataObject->IsDataAvailable(CF_FILEDESCRIPTOR) ) {
		if ( (hData = pDataObject->GetGlobalData(CF_FILEDESCRIPTOR)) == NULL )
			return FALSE;

		return DescToDrop(pWnd, pDataObject, hData);
	}

	return FALSE;
}
BOOL CViewDropTarget::DescToDrop(CWnd* pWnd, COleDataObject* pDataObject, HGLOBAL hDescInfo)
{
	int n, i;
	FILEGROUPDESCRIPTOR *pGroupDesc;
	FORMATETC FormatEtc;
	CString TempPath;
	LPCTSTR TempDir;
	CFile *pFile, TempFile;
	BYTE buff[4096];
	int dropCount = 0;
	DROPFILES drop;
	HDROP hDrop;
	CSharedFile sf(GMEM_MOVEABLE | GMEM_ZEROINIT);
	int doSub = 0;

	if ( (pGroupDesc = (FILEGROUPDESCRIPTOR *)::GlobalLock(hDescInfo)) == NULL )
		return FALSE;

	TempDir = ((CRLoginApp *)::AfxGetApp())->GetTempDir(TRUE);

	ZeroMemory(&FormatEtc, sizeof(FormatEtc));
	FormatEtc.cfFormat = CF_FILECONTENTS;
	FormatEtc.dwAspect = DVASPECT_CONTENT;
	FormatEtc.tymed = TYMED_FILE;

	ZeroMemory(&drop, sizeof(drop));
	drop.pFiles = sizeof(DROPFILES);
	drop.pt.x   = 0;
	drop.pt.y   = 0;
	drop.fNC    = FALSE;

#ifdef	_UNICODE
	drop.fWide  = TRUE;
#else
	drop.fWide  = FALSE;
#endif

	sf.Write(&drop, sizeof(DROPFILES));

	for ( n = 0 ; n < (int)pGroupDesc->cItems ; n++ ) {
		if ( (pGroupDesc->fgd[n].dwFlags & FD_ATTRIBUTES) != 0 && (pGroupDesc->fgd[n].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ) {

			if ( doSub == 0 )
				doSub = (AfxMessageBox(CStringLoad(IDS_DROPSUBDIRCHECK), MB_ICONQUESTION | MB_YESNO) == IDYES ? 2 : 1);

			if ( doSub == 1 )
				continue;

			TempPath.Format(_T("%s%s"), TempDir, pGroupDesc->fgd[n].cFileName);
			if ( !CreateDirectory(TempPath, NULL) ) {
				ThreadMessageBox(_T("GetFileData Error %s"), pGroupDesc->fgd[n].cFileName);
				break;
			}

		} else {
			if ( doSub != 2 && _tcschr(pGroupDesc->fgd[n].cFileName, _T('\\')) != NULL )
				continue;

			FormatEtc.lindex = n;
			if ( (pFile = pDataObject->GetFileData(CF_FILECONTENTS, &FormatEtc)) == NULL ) {
				ThreadMessageBox(_T("GetFileData Error %s"), pGroupDesc->fgd[n].cFileName);
				break;
			}

			TempPath.Format(_T("%s%s"), TempDir, pGroupDesc->fgd[n].cFileName);
			if ( !TempFile.Open(TempPath, CFile::modeCreate | CFile::modeWrite, NULL) ) {
				ThreadMessageBox(_T("OpenFile Error %s"), TempPath);
				break;
			}

			try {
				while ( (i = pFile->Read(buff, 4096)) > 0 )
					TempFile.Write(buff, i);
				TempFile.Close();

				// Release GetFileData Object
				pFile->Close();
				delete pFile;
			} catch(...) {
				ThreadMessageBox(_T("CopyFile Error %s"), TempPath);
				break;
			}

			dropCount++;
			sf.Write((LPCTSTR)TempPath, (TempPath.GetLength() + 1) * sizeof(TCHAR));
		}
	}

	::GlobalUnlock(hDescInfo);

	if ( dropCount > 0 ) {
		sf.Write(_T("\0"), sizeof(TCHAR));
		hDrop = (HDROP)sf.Detach();
		pWnd->SendMessage(WM_DROPFILES, (WPARAM)hDrop);
		::GlobalFree(hDrop);
	}

	return TRUE;
}

#endif	// USE_OLE

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
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_DROPFILES()
	ON_WM_SETCURSOR()
	ON_WM_VSCROLL()
	ON_WM_TIMER()

	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_XBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_DESTROY()

	ON_MESSAGE(WM_IME_NOTIFY, OnImeNotify)
	ON_MESSAGE(WM_IME_COMPOSITION, OnImeComposition)
	ON_MESSAGE(WM_IME_REQUEST, OnImeRequest)
	ON_MESSAGE(WM_LOGWRITE, OnLogWrite)

	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_COPY_ALL, &CRLoginView::OnEditCopyAll)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND_RANGE(IDM_CLIPBOARD_HIS1, IDM_CLIPBOARD_HIS10, ClipboardPaste)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_CLIPBOARD_HIS1, IDM_CLIPBOARD_HIS10, OnUpdateClipboardPaste)
	ON_COMMAND(IDM_CLIPBOARD_MENU, OnClipboardMenu)

	ON_COMMAND(IDM_IMAGEGRAPCOPY, OnImageGrapCopy)
	ON_COMMAND(IDM_IMAGEGRAPSAVE, OnImageGrapSave)
	ON_COMMAND(IDM_IMAGEGRAPHIST, OnImageGrapHistogram)
	ON_UPDATE_COMMAND_UI(IDM_IMAGEGRAPHIST, OnUpdateHistogram)

	ON_COMMAND(ID_MACRO_REC, OnMacroRec)
	ON_UPDATE_COMMAND_UI(ID_MACRO_REC, OnUpdateMacroRec)
	ON_COMMAND(ID_MACRO_PLAY, OnMacroPlay)
	ON_UPDATE_COMMAND_UI(ID_MACRO_PLAY, OnUpdateMacroPlay)
	ON_COMMAND_RANGE(ID_MACRO_HIS1, ID_MACRO_HIS5, OnMacroHis)
	ON_UPDATE_COMMAND_UI_RANGE(ID_MACRO_HIS1, ID_MACRO_HIS5, OnUpdateMacroHis)

	ON_COMMAND(ID_MOUSE_EVENT, &CRLoginView::OnMouseEvent)
	ON_UPDATE_COMMAND_UI(ID_MOUSE_EVENT, &CRLoginView::OnUpdateMouseEvent)

	ON_COMMAND(ID_PAGE_PRIOR, &CRLoginView::OnPagePrior)
	ON_COMMAND(ID_PAGE_NEXT, &CRLoginView::OnPageNext)

	ON_COMMAND(IDM_SEARCH_REG, &CRLoginView::OnSearchReg)
	ON_COMMAND(IDM_SEARCH_NEXT, &CRLoginView::OnSearchNext)
	ON_COMMAND(IDM_SEARCH_BACK, &CRLoginView::OnSearchBack)

	ON_COMMAND(ID_GOZIVIEW, &CRLoginView::OnGoziview)
	ON_UPDATE_COMMAND_UI(ID_GOZIVIEW, &CRLoginView::OnUpdateGoziview)

	ON_COMMAND(ID_SPLIT_HEIGHT, &CRLoginView::OnSplitHeight)
	ON_COMMAND(ID_SPLIT_WIDTH, &CRLoginView::OnSplitWidth)
	ON_COMMAND(ID_SPLIT_OVER, &CRLoginView::OnSplitOver)
	ON_COMMAND(IDM_SPLIT_HEIGHT_NEW, &CRLoginView::OnSplitHeightNew)
	ON_COMMAND(IDM_SPLIT_WIDTH_NEW, &CRLoginView::OnSplitWidthNew)

	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_COMMAND(IDM_EDIT_MARK, &CRLoginView::OnEditMark)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_MARK, &CRLoginView::OnUpdateEditMark)
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
	m_pBitmap = NULL;
	m_ClipFlag = 0;
	m_ClipStaPosSave = (-1);
	m_ClipTimer = 0;
	m_KeyMacFlag = FALSE;
	m_KeyMacSizeCheck = FALSE;
	m_ActiveFlag = TRUE;
	m_VisualBellFlag = FALSE;
	m_BlinkFlag = 0;
	m_ImageFlag = 0;
	m_MouseEventFlag = FALSE;
	m_WheelDelta = 0;
	m_WheelTimer = 0;
	m_WheelClock = 0;
	m_WheelzDelta = 0;
	m_pGhost = NULL;
	m_GoziView  = FALSE;
#if   USE_GOZI == 1 || USE_GOZI == 2
	m_GoziStyle = (8 << 4) | 9;
	m_GoziCount = 4 + rand() % 28;
	m_GoziPos.SetPoint(0, 0);
#elif USE_GOZI == 3 || USE_GOZI == 4
	m_GoziMax = 3;
	for ( int n = 0 ; n < 8 ; n++ ) {
		m_GoziData[n].m_GoziStyle = 5;
		m_GoziData[n].m_GoziCount = 4 + rand() % 28;
		m_GoziData[n].m_GoziPos.SetPoint(0, 0);
	}
#endif

	m_RDownStat = 0;
	m_RMovePixel = 0;
	m_SmoothTimer = 0;
	m_TopOffset = 0;

	m_SleepView = FALSE;
	m_SleepCount = 0;

	m_ScrollOut = FALSE;
	m_LastMouseFlags = 0;
	m_LastMouseClock = 0;
	m_bLButtonTrClk = FALSE;
	m_ClipUpdateLine = FALSE;
	m_ClipSavePoint.x = m_ClipSavePoint.y = 0;
	m_RclickTimer = 0;

	m_pCellSize = NULL;
	m_pSelectGrapWnd = NULL;

	m_CaretFlag = 0;
	m_CaretX = m_CaretY = 0;
	m_CaretSize.cx = m_CaretSize.cy = 1;
	m_CaretRect.SetRectEmpty();
	m_CaretMapSize.cx = m_CaretMapSize.cy = 0;
	m_CaretColor = 0;
	m_bCaretAllocCol = FALSE;
	m_CaretBaseClock = clock();
	m_CaretAnimeMax = 1;
	m_CaretAnimeClock = 100;

	m_bImmActive = FALSE;

	m_bDelayInvalThread = 0;
	m_DelayInvalWait = INFINITE;
	m_DelayInvalClock = (-1);

	m_MatrixCols.SetSize(COLS_MAX);

	m_PastNoCheck = FALSE;
	m_PastDelaySend = FALSE;
	m_PastCtrlView = FALSE;
}

CRLoginView::~CRLoginView()
{
	if ( m_pGhost != NULL )
		m_pGhost->DestroyWindow();

	if ( m_pCellSize != NULL )
		delete [] m_pCellSize;
}

BOOL CRLoginView::PreCreateWindow(CREATESTRUCT& cs)
{
	BOOL st;

	cs.style &= ~WS_BORDER;
	cs.style |= WS_CLIPSIBLINGS;

	cs.lpszName = _T("RLoginView");
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW)); //, CreateSolidBrush(0xff000000));

	st = CView::PreCreateWindow(cs);

	//TRACE("View Style ");
	//if ( (cs.style & WS_BORDER) != NULL ) TRACE("WS_BORDER ");
	//if ( (cs.style & WS_DLGFRAME) != NULL ) TRACE("WS_DLGFRAME ");
	//if ( (cs.style & WS_THICKFRAME) != NULL ) TRACE("WS_THICKFRAME ");
	//if ( (cs.dwExStyle & WS_EX_WINDOWEDGE) != NULL ) TRACE("WS_EX_WINDOWEDGE ");
	//if ( (cs.dwExStyle & WS_EX_CLIENTEDGE) != NULL ) TRACE("WS_EX_CLIENTEDGE ");
	//if ( (cs.dwExStyle & WS_EX_DLGMODALFRAME) != NULL ) TRACE("WS_EX_DLGMODALFRAME ");
	//if ( (cs.dwExStyle & WS_EX_TOOLWINDOW) != NULL ) TRACE("WS_EX_TOOLWINDOW ");
	//TRACE("\n");

	return st;
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスの描画

void CRLoginView::OnDraw(CDC* pDC)
{
	int sx = 0;
	int sy = 0;
	int ex = m_Cols  - 1;
	int ey = m_Lines - 1;
	CDC workDC, *pSaveDC = NULL;
	CBitmap workMap, *pOldworkMap = NULL;
	CDC TempDC;
	CBitmap *pOldBitMap;
	CRLoginDoc* pDoc = GetDocument();
	CRect frame, drawbox, rect;

	GetClientRect(frame);
	drawbox = frame;
	m_HaveBack = FALSE;

	if ( !pDC->IsPrinting() ) {
		drawbox = ((CPaintDC *)(pDC))->m_ps.rcPaint;

		// 描画範囲をグラフィック座標からキャラクタ座標に変換
		sx = (drawbox.left + 1 - pDoc->m_TextRam.m_ScrnOffset.left) * m_Cols / m_Width;
		ex = (drawbox.right + m_CharWidth - pDoc->m_TextRam.m_ScrnOffset.left) * m_Cols / m_Width;
		sy = (drawbox.top + 1 - pDoc->m_TextRam.m_ScrnOffset.top - m_TopOffset) * m_Lines / m_Height;
		ey = (drawbox.bottom + m_CharHeight - pDoc->m_TextRam.m_ScrnOffset.top) * m_Lines / m_Height;

		if ( (sx * m_Width / m_Cols + pDoc->m_TextRam.m_ScrnOffset.left) > drawbox.left )
			sx--;
		if ( (ex * m_Width / m_Cols + pDoc->m_TextRam.m_ScrnOffset.left) < drawbox.right )
			ex++;
		if ( (sy * m_Height / m_Lines + pDoc->m_TextRam.m_ScrnOffset.top + m_TopOffset) > drawbox.top )
			sy--;
		if ( (ey * m_Height / m_Lines + pDoc->m_TextRam.m_ScrnOffset.top + m_TopOffset) < drawbox.bottom )
			ey++;

		if ( m_pBitmap != NULL ) {
			// 背景画像を描画 WorkDCに仮描画を準備
			workDC.CreateCompatibleDC(pDC);
			workMap.CreateCompatibleBitmap(pDC, frame.Width(), frame.Height());
			pOldworkMap = (CBitmap *)workDC.SelectObject(&workMap);

			pSaveDC = pDC;
			pDC = &workDC;

			TempDC.CreateCompatibleDC(pDC);
			pOldBitMap = (CBitmap *)TempDC.SelectObject(m_pBitmap);

			pDC->BitBlt(drawbox.left, drawbox.top, drawbox.Width(), drawbox.Height(), &TempDC, drawbox.left, drawbox.top, SRCCOPY);
			TempDC.SelectObject(pOldBitMap);
			pDC->SetBkMode(TRANSPARENT);
			m_HaveBack = TRUE;

		} else if ( !ExDwmEnable && ((CMainFrame *)::AfxGetMainWnd())->m_bGlassStyle ) {
			// 透過ウィンドウの背景描画
			PaintDesktop(pDC->GetSafeHdc());
			pDC->SetBkMode(TRANSPARENT);
			m_HaveBack = TRUE;
		}
	}

	// キャラクタVramの描画
	if ( pDoc->m_TextRam.IsInitText() )
		pDoc->m_TextRam.DrawVram(pDC, sx, sy, ex, ey, this, pDC->IsPrinting());
	else
		pDC->FillSolidRect(frame, GetSysColor(COLOR_APPWORKSPACE));

	// 仮描画であれば実際に描画
	if ( pSaveDC != NULL ) {
		pDC = pSaveDC;
		pDC->BitBlt(drawbox.left, drawbox.top, drawbox.Width(), drawbox.Height(), &workDC, drawbox.left, drawbox.top, SRCCOPY);
		workDC.SelectObject(pOldworkMap);
	}

	// Tekの重ね書き
	if ( pDoc->m_TextRam.IsOptEnable(TO_RLTEKINWND) ) {
		if ( m_TekBitmap.m_hObject != NULL ) {
			BITMAP mapinfo;
			m_TekBitmap.GetBitmap(&mapinfo);
			if ( mapinfo.bmWidth != frame.Width() || mapinfo.bmHeight != frame.Height() )
				m_TekBitmap.DeleteObject();
		}

		if ( TempDC.m_hDC == NULL )
			TempDC.CreateCompatibleDC(pDC);

		if ( m_TekBitmap.m_hObject == NULL ) {
			m_TekBitmap.CreateCompatibleBitmap(pDC, frame.Width(), frame.Height());
			pOldBitMap = (CBitmap *)TempDC.SelectObject(&m_TekBitmap);

			TempDC.FillSolidRect(frame, RGB(0, 0, 0));
			pDoc->m_TextRam.TekDraw(&TempDC, frame);

		} else
			pOldBitMap = (CBitmap *)TempDC.SelectObject(&m_TekBitmap);

		pDC->BitBlt(drawbox.left, drawbox.top, drawbox.Width(), drawbox.Height(), &TempDC, 0, 0, SRCINVERT);

		TempDC.SelectObject(pOldBitMap);
	}

#if		USE_GOZI == 1 || USE_GOZI == 2
	if ( m_GoziView ) {
		CMainFrame *pMain = GetMainWnd();
		if ( pMain != NULL && pMain->m_ImageGozi.m_hImageList != NULL )
			pMain->m_ImageGozi.Draw(pDC, m_GoziStyle >> 4, m_GoziPos, ILD_NORMAL);
	}
#elif	USE_GOZI == 3 || USE_GOZI == 4
	if ( m_GoziView ) {
		CMainFrame *pMain = GetMainWnd();
		for ( int n = 0 ; n < m_GoziMax ; n++ ) {
			if ( pMain != NULL && pMain->m_ImageGozi.m_hImageList != NULL )
				pMain->m_ImageGozi.Draw(pDC, m_GoziData[n].m_GoziStyle >> 4, m_GoziData[n].m_GoziPos, ILD_NORMAL);
		}
	}
#endif

	// プロポーション文字の場合にカレット＆IMEの位置再調整
	if ( (m_CaretFlag & FGCARET_CREATE) != 0 && m_ClipUpdateLine && m_pCellSize != NULL && m_CaretX != 0 ) {
		m_CaretX = GetGrapPos(pDoc->m_TextRam.m_CurX, (pDoc->m_TextRam.m_CurY + m_HisOfs - m_HisMin));
		m_CaretRect.left  = m_CaretX;
		m_CaretRect.right = m_CaretRect.left + m_CaretSize.cx;
		ImmSetPos(m_CaretX, m_CaretY);
	}

	// カレットを描画
	if ( !pDC->IsPrinting() && (m_CaretFlag & FGCARET_CREATE) != 0 && (m_CaretFlag & FGCARET_DRAW) != 0 && rect.IntersectRect(m_CaretRect, drawbox) ) {

		if ( TempDC.m_hDC == NULL )
			TempDC.CreateCompatibleDC(pDC);

		if ( m_CaretBitmap.m_hObject == NULL || m_CaretMapSize.cx != (m_CaretSize.cx * m_CaretAnimeMax) || m_CaretMapSize.cy != m_CaretSize.cy ) {

			// カレットビットマップを構築
			if ( m_CaretBitmap.m_hObject != NULL )
				m_CaretBitmap.DeleteObject();

			if ( m_CaretColor == 0 && (m_CaretColor = pDoc->m_TextRam.m_CaretColor) == 0 ) {
				m_bCaretAllocCol = TRUE;
				m_CaretColor = CaretColor();
			}

			m_CaretMapSize.cx = m_CaretSize.cx * m_CaretAnimeMax;
			m_CaretMapSize.cy = m_CaretSize.cy;
			m_CaretBitmap.CreateCompatibleBitmap(pDC, m_CaretMapSize.cx * m_CaretAnimeMax, m_CaretMapSize.cy);

			pOldBitMap = (CBitmap *)TempDC.SelectObject(&m_CaretBitmap);

			for ( int n = 0 ; n < m_CaretAnimeMax ; n++ ) {
				COLORREF col = (m_CaretAnimeMax <= 1 ? m_CaretColor :
									RGB(GetRValue(m_CaretColor) * (m_CaretAnimeMax - n - 1) / (m_CaretAnimeMax - 1),
									    GetGValue(m_CaretColor) * (m_CaretAnimeMax - n - 1) / (m_CaretAnimeMax - 1),
									    GetBValue(m_CaretColor) * (m_CaretAnimeMax - n - 1) / (m_CaretAnimeMax - 1)));

				TempDC.FillSolidRect(m_CaretMapSize.cx * n, 0, m_CaretMapSize.cx, m_CaretMapSize.cy, col);
			}

		} else
			pOldBitMap = (CBitmap *)TempDC.SelectObject(&m_CaretBitmap);

		if ( m_CaretAnimeMax > 1 ) {
			clock_t t = clock() - m_CaretBaseClock;
			int pos = (t / m_CaretAnimeClock) % m_CaretAnimeMax;

			pDC->BitBlt(m_CaretRect.left, m_CaretRect.top, m_CaretRect.Width(), m_CaretRect.Height(), &TempDC, m_CaretMapSize.cx * pos, 0, SRCINVERT);

			// プロポーショナルフォントの場合は全行更新
			if ( m_ClipUpdateLine && m_pCellSize != NULL ) {
				rect.SetRect(frame.left, m_CaretRect.top, frame.right, m_CaretRect.bottom);
				AddDeleyInval(((t + m_CaretBaseClock) / m_CaretAnimeClock + 1) * m_CaretAnimeClock, rect);
			} else
				AddDeleyInval(((t + m_CaretBaseClock) / m_CaretAnimeClock + 1) * m_CaretAnimeClock, m_CaretRect);

		} else
			pDC->BitBlt(m_CaretRect.left, m_CaretRect.top, m_CaretRect.Width(), m_CaretRect.Height(), &TempDC, 0, 0, SRCINVERT);

		TempDC.SelectObject(pOldBitMap);
	}

	SetDelayInvalTimer();
}
void CRLoginView::CreateGrapImage(int type)
{
	CDC MemDC;
	CBitmap *pBitmap;
	CBitmap *pOldBitMap;
	CString tmp;
	CRLoginDoc* pDoc = GetDocument();

	if ( !pDoc->m_TextRam.IsInitText() )
		return;

	tmp.Format(_T("Image - %s"), pDoc->GetTitle());

	if ( pDoc->m_TextRam.m_pImageWnd == NULL ) {
		pDoc->m_TextRam.m_pImageWnd = new CGrapWnd(&(pDoc->m_TextRam));
		pDoc->m_TextRam.m_pImageWnd->Create(NULL, tmp);
	} else
		pDoc->m_TextRam.m_pImageWnd->SetWindowText(tmp);

	MemDC.CreateCompatibleDC(NULL);
	pBitmap = pDoc->m_TextRam.m_pImageWnd->GetBitmap(m_Width, m_Height);
	pOldBitMap = (CBitmap *)MemDC.SelectObject(pBitmap);

	if ( m_pBitmap != NULL ) {
		CDC TempDC;
		CBitmap *pOldBitMap;
		TempDC.CreateCompatibleDC(&MemDC);
		pOldBitMap = (CBitmap *)TempDC.SelectObject(m_pBitmap);
		MemDC.BitBlt(0, 0, m_Width, m_Height, &TempDC, 0, 0, SRCCOPY);
		TempDC.SelectObject(pOldBitMap);
		MemDC.SetBkMode(TRANSPARENT);
	}

	pDoc->m_TextRam.DrawVram(&MemDC, 0, 0, m_Cols, m_Lines, this, FALSE);
	MemDC.SelectObject(pOldBitMap);

	pDoc->m_TextRam.m_pImageWnd->Invalidate(FALSE);

	if ( type > 0 )
		pDoc->m_TextRam.m_pImageWnd->SaveBitmap(type - 1);
	else if ( !pDoc->m_TextRam.m_pImageWnd->IsWindowVisible() )
		pDoc->m_TextRam.m_pImageWnd->ShowWindow(SW_SHOW);
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

void CRLoginView::ResetCellSize()
{
	if ( m_pCellSize == NULL )
		return;

	delete [] m_pCellSize;
	m_pCellSize = NULL;
}
void CRLoginView::SetCellSize(int x, int y, int sz)
{
	if ( sz <= 0 && m_pCellSize == NULL )
		return;

	if ( x < 0 || x >= m_Cols || y < 0 || y >= m_Lines )
		return;

	if ( m_pCellSize == NULL || m_CellCols != m_Cols || m_CellLines != m_Lines ) {
		if ( m_pCellSize != NULL )
			delete [] m_pCellSize;
		m_CellCols  = m_Cols;
		m_CellLines = m_Lines;
		m_pCellSize = new BYTE[m_CellCols * m_CellLines];
		ZeroMemory(m_pCellSize, sizeof(BYTE) * m_CellCols * m_CellLines);
	}

	m_pCellSize[x + m_CellCols * y] = sz;
}
int CRLoginView::GetGrapPos(int x, int y)
{
	int n, i;
	int pos;
	CRLoginDoc *pDoc = GetDocument();

	if ( m_pCellSize == NULL || y < 0 || y >= m_Lines || x <= 0 || x >= m_Cols || m_pCellSize[x + m_CellCols * y] == 0 )
		return (m_Width * x / m_Cols + pDoc->m_TextRam.m_ScrnOffset.left);

	pos = 0;
	for ( n = x - 1 ; n >= 0 ; n-- ) {
		if ( (i = m_pCellSize[n + m_CellCols * y]) == 0 ) {
			pos += m_Width * (n + 1) / m_Cols;
			break;
		} else
			pos += i;
	}

	pos += pDoc->m_TextRam.m_ScrnOffset.left;
	return pos;
}

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
void CRLoginView::CalcGrapPoint(CPoint po, int *x, int *y)
{
	CRLoginDoc *pDoc = GetDocument();

	if      ( (po.x -= pDoc->m_TextRam.m_ScrnOffset.left) < 0 )	po.x = 0;
	else if ( po.x >= m_Width )		po.x = m_Width -1;

	if      ( (po.y -= (pDoc->m_TextRam.m_ScrnOffset.top + m_TopOffset)) < 0 ) po.y = 0;
	else if ( po.y >= m_Height )	po.y = m_Height - 1;

	pDoc->m_TextRam.m_MousePos = po;

	*x = m_Cols  * po.x / m_Width;
	*y = m_Lines * po.y / m_Height;

	if ( m_pCellSize == NULL || *y < 0 || *y >= m_Lines )
		goto RETENDOF;

	ASSERT(*x >= 0 && *x < m_Cols);
	ASSERT(*y >= 0 && *y < m_Lines);

	int n, i;

	n = GetGrapPos(*x, *y);

	if ( n < po.x ) {
		while ( (*x + 1) < m_Cols ) {
			if ( (i = m_pCellSize[*x + *y * m_CellCols]) == 0 )
				n = m_Width * (*x + 1) / m_Cols;
			else
				n += i;

			if ( n > po.x )
				break;
			*x += 1;
		}
	} else {
		while ( *x > 0 ) {
			if ( (i = m_pCellSize[*x + *y * m_CellCols]) == 0 )
				n = m_Width * *x / m_Cols;
			else
				n -= i;

			*x -= 1;
			if ( n <= po.x )
				break;
		}
	}

RETENDOF:
	*y =  *y - m_HisOfs + m_HisMin;
	return;
}

void CRLoginView::InvalidateTextRect(CRect &rect)
{
	int height;
	CRLoginDoc *pDoc = GetDocument();

	rect.left   = m_Width  * rect.left  / m_Cols + (rect.left == 0 ? 0 : pDoc->m_TextRam.m_ScrnOffset.left);
	rect.right  = m_Width  * rect.right / m_Cols + pDoc->m_TextRam.m_ScrnOffset.left + (rect.right >= (pDoc->m_TextRam.m_Cols - 1) ? pDoc->m_TextRam.m_ScrnOffset.right : 0);

	rect.top    = m_Height * (rect.top    + m_HisOfs - m_HisMin) / m_Lines + m_TopOffset + (rect.top == 0 ? 0 : pDoc->m_TextRam.m_ScrnOffset.top);
	rect.bottom = m_Height * (rect.bottom + m_HisOfs - m_HisMin) / m_Lines + m_TopOffset + pDoc->m_TextRam.m_ScrnOffset.top + (rect.bottom >= (pDoc->m_TextRam.m_Lines - 1) ? pDoc->m_TextRam.m_ScrnOffset.bottom : 0);

	height = m_Height + pDoc->m_TextRam.m_ScrnOffset.top + pDoc->m_TextRam.m_ScrnOffset.bottom;

	if ( rect.top >= height || rect.bottom <= 0 )
		return;

	if ( rect.top < 0 )
		rect.top = 0;

	if ( rect.bottom > height )
		rect.bottom = height;

	InvalidateRect(rect, FALSE);

	if ( m_pGhost != NULL )
		m_pGhost->InvalidateRect(rect, FALSE);
}
void CRLoginView::InvalidateFullText()
{
	// 全描画なのでタイマーは、初期化
	m_DeleyInvalList.RemoveAll();

	Invalidate(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Delayed Invalidate

static UINT DelayInvalThreadProc(LPVOID pParam)
{
	return ((CRLoginView *)pParam)->DelayInvalThread();
}
UINT CRLoginView::DelayInvalThread()
{
	DWORD rt;
	DWORD dwMsec = INFINITE;

	m_pDelayInvalSync->SetEvent();

	while ( m_bDelayInvalThread ) {
		if ( (rt = WaitForSingleObject(m_pDelayInvalEvent->m_hObject, dwMsec)) == WAIT_TIMEOUT ) {
			PostMessage(WM_TIMER, (WPARAM)VTMID_INVALIDATE, NULL);
			dwMsec = INFINITE;

		} else if ( rt == WAIT_OBJECT_0 ) {
			dwMsec = m_DelayInvalWait;

		} else	// rt == WAIT_ABANDONED
			break;
	}

	m_pDelayInvalSync->SetEvent();
	return 0;
}
void CRLoginView::DelayInvalThreadStart()
{
	m_pDelayInvalEvent = new CEvent(FALSE, FALSE);
	m_pDelayInvalSync = new CEvent(FALSE, FALSE);

	m_bDelayInvalThread = TRUE;
	m_DelayInvalWait = INFINITE;
	m_DelayInvalClock = (-1);

	AfxBeginThread(DelayInvalThreadProc, this, THREAD_PRIORITY_NORMAL);
	WaitForSingleObject(m_pDelayInvalSync->m_hObject, INFINITE);
}
void CRLoginView::DelayInvalThreadEndof()
{
	if ( m_bDelayInvalThread ) {
		m_bDelayInvalThread = FALSE;
		m_pDelayInvalEvent->SetEvent();
		WaitForSingleObject(m_pDelayInvalSync->m_hObject, INFINITE);
	}

	delete m_pDelayInvalEvent;
	delete m_pDelayInvalSync;
}
void CRLoginView::DelayInvalThreadTimer(clock_t msec)
{
	m_DelayInvalWait = msec;
	m_pDelayInvalEvent->SetEvent();
}

void CRLoginView::SetDelayInvalTimer()
{
	if ( m_DeleyInvalList.IsEmpty() )
		return;

	DeleyInval *pIc = &m_DeleyInvalList.GetHead();

	if ( m_DelayInvalClock > 0 && m_DelayInvalClock <= pIc->m_Clock )
		return;

	clock_t tic;
	if ( (tic = pIc->m_Clock - clock()) < DELAYINVALMINCLOCK )
		tic = DELAYINVALMINCLOCK;

	// 次のタイマーをセット
	m_DelayInvalClock = pIc->m_Clock;
	DelayInvalThreadTimer(tic);
}
void CRLoginView::AddDeleyInval(clock_t ick, CRect &rect)
{
	DeleyInval tmp;
	POSITION pos = m_DeleyInvalList.GetHeadPosition();

	tmp.m_Clock = ick;
	tmp.m_Rect = rect;

	// 最近クロックでソート
	while ( pos != NULL ) {
		if ( m_DeleyInvalList.GetAt(pos).m_Clock >= ick ) {
			m_DeleyInvalList.InsertBefore(pos, tmp);
			return;
		}

		m_DeleyInvalList.GetNext(pos);
	}

	m_DeleyInvalList.AddTail(tmp);
}
void CRLoginView::PollingDeleyInval()
{
	clock_t tic, now = clock();
	DeleyInval *pIc;
	CRect *pRect = NULL;

	// タイマーをクリア
	m_DelayInvalClock = (-1);

	while ( !m_DeleyInvalList.IsEmpty() ) {
		pIc = &m_DeleyInvalList.GetHead();

		if ( (tic = pIc->m_Clock - now) < DELAYINVALMINCLOCK ) {
			if ( pRect == NULL || *pRect != pIc->m_Rect )
				InvalidateRect(pIc->m_Rect, FALSE);
			pRect = &(pIc->m_Rect);
			m_DeleyInvalList.RemoveHead();

		} else {
			// 次のタイマーをセット
			m_DelayInvalClock = pIc->m_Clock;
			DelayInvalThreadTimer(tic);
			break;
		}
	}
}
BOOL CRLoginView::OnIdle()
{
	// 最初のアイドル時に更新
	UpdateWindow();

	return FALSE;
}

int CRLoginView::HitTest(CPoint point)
{
	int mode = 0;
	CRect rect;
	CRLoginDoc *pDoc = GetDocument();

	GetClientRect(rect);

	if ( point.x < (pDoc->m_TextRam.m_ScrnOffset.left + m_CharWidth * 2) )
		mode |= 001;
	else if ( point.x > (rect.right - pDoc->m_TextRam.m_ScrnOffset.right - m_CharWidth * 2) )
		mode |= 003;
	else if ( point.x > ((rect.left + rect.right - m_CharWidth * 2) / 2) && point.x < ((rect.left + rect.right + m_CharWidth * 2) / 2) )
		mode |= 002;

	if ( point.y < (pDoc->m_TextRam.m_ScrnOffset.top + m_TopOffset + m_CharHeight) )
		mode |= 010;
	else if ( point.y > (rect.bottom - pDoc->m_TextRam.m_ScrnOffset.bottom - m_CharHeight) )
		mode |= 030;
	else if ( point.y > ((rect.top + rect.bottom - m_CharHeight) / 2) && point.y < ((rect.top + rect.bottom + m_CharHeight) / 2) )
		mode |= 020;

	//  011	010	012	010	013
	//  001	000	002	000	003
	//  021	020	022	020	023
	//  001	000	002	000	003
	//  031	030	032	030	033

	if ( (mode & 003) == 0 || (mode & 030) == 0 )
		mode = 0;

	return mode;
}

/////////////////////////////////////////////////////////////////////////////

void CRLoginView::SendBuffer(CBuffer &buf, BOOL macflag, BOOL delaySend)
{
	int n;
	WCHAR *p;
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();

	m_ScrollOut = FALSE;

	if ( macflag == FALSE ) {
		if ( pDoc->m_TextRam.IsOptEnable(TO_RLDSECHO) && !pDoc->m_TextRam.LineEdit(buf) )
			return;

		pDoc->OnSendBuffer(buf);

		if ( ((CMainFrame *)::AfxGetMainWnd())->m_bBroadCast && !pDoc->m_TextRam.IsOptEnable(TO_RLNTBCSEND) ) {

			((CRLoginApp *)::AfxGetApp())->SendBroadCast(buf, pDoc->m_TextRam.m_GroupCast);

			if ( pDoc->m_TextRam.m_GroupCast.IsEmpty() ) {
				// BroadCast
				if ( !pDoc->m_TextRam.IsOptEnable(TO_RLNTBCRECV) && !pDoc->m_TextRam.IsOptEnable(TO_RLGROUPCAST) )
					return;
			} else {
				// GroupCast
				if ( !pDoc->m_TextRam.IsOptEnable(TO_RLNTBCRECV) )
					return;
			}
		}
	}

	if ( m_KeyMacFlag ) {
		m_KeyMacBuf.Apend(buf.GetPtr(), buf.GetSize());

		if ( !m_KeyMacSizeCheck && m_KeyMacBuf.GetSize() >= KEYMACBUFMAX ) {
			m_KeyMacSizeCheck = TRUE;

			if ( MessageBox(CStringLoad(IDE_KEYMACTOOLONG), _T("Warning"), MB_ICONWARNING | MB_YESNO) != IDYES ) {
				m_KeyMacFlag = FALSE;
				m_KeyMacBuf.Clear();
			}
		}
	}

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
				tmp.PutWord(0x0A);
			else
				tmp.PutWord(*p);
			p++;
		}
		buf = tmp;
		tmp.Clear();
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
	pDoc->SocketSend(tmp.GetPtr(), tmp.GetSize(), delaySend);

	if ( !pDoc->m_TextRam.IsOptEnable(TO_ANSISRM) ) {
		pDoc->m_TextRam.PUTSTR(tmp.GetPtr(), tmp.GetSize());
		pDoc->m_TextRam.FLUSH();
	}
}

/////////////////////////////////////////////////////////////////////////////

void CRLoginView::InvalidateCaret()
{
	// プロポーショナルフォントの場合は全行更新
	if ( m_ClipUpdateLine ) {
		CRect rect;
		GetClientRect(rect);
		rect.top = m_CaretRect.top;
		rect.bottom = m_CaretRect.bottom;
		InvalidateRect(rect, FALSE);
	} else
		InvalidateRect(m_CaretRect, FALSE);
}
void CRLoginView::DispCaret(BOOL bShow)
{
	if ( (m_CaretFlag & FGCARET_CREATE) == 0 )
		return;

	if ( (bShow && (m_CaretFlag & FGCARET_DRAW) == 0) || (!bShow && (m_CaretFlag & FGCARET_DRAW) != 0) ) {
		m_CaretFlag ^= FGCARET_DRAW;
		InvalidateCaret();
	}
}
COLORREF CRLoginView::CaretColor()
{
	static int hue = 110, lum = 80, sat = 100;

	if ( (hue += 70) >= 360 )
		hue -= 360;

	return CGrapWnd::HLStoRGB(hue * HLSMAX / 360, lum * HLSMAX / 100, sat * HLSMAX / 100);
}
void CRLoginView::CaretPos(POINT point)
{
	if ( (m_CaretFlag & FGCARET_DRAW) != 0 )
		InvalidateCaret();

	m_CaretRect.left = point.x;
	m_CaretRect.top  = point.y;

	m_CaretRect.right  = m_CaretRect.left + m_CaretSize.cx;
	m_CaretRect.bottom = m_CaretRect.top  + m_CaretSize.cy;

	if ( (m_CaretFlag & FGCARET_DRAW) != 0 )
		InvalidateCaret();
}
void CRLoginView::CaretSize(int width, int height)
{
	m_CaretSize.cx = width;
	m_CaretSize.cy = height;
}
void CRLoginView::UpdateCaret()
{
	CRLoginDoc *pDoc = GetDocument();

	KillCaret();

	if ( m_CaretBitmap.m_hObject != NULL )
		m_CaretBitmap.DeleteObject();

	if ( m_bCaretAllocCol ) {
		if ( pDoc->m_TextRam.m_CaretColor != 0 ) {
			m_bCaretAllocCol = FALSE;
			m_CaretColor = 0;
		}
	} else
		m_CaretColor = 0;

	SetCaret();
}

void CRLoginView::KillCaret()
{
	if ( (m_CaretFlag & FGCARET_CREATE) != 0 ) {
		DispCaret(FALSE);
		m_CaretFlag &= ~FGCARET_CREATE;
	}
}
void CRLoginView::SetCaret()
{
	int n;
	CPoint po(m_CaretX, m_CaretY);
	CRLoginDoc *pDoc = GetDocument();

	// 001 = CreateCaret Flag, 002 = CurSol ON/OFF, 004 = Focus Flag, 010 = Redraw Caret
	// TRACE("SetCaret %02x\n", m_CaretFlag);

	switch(m_CaretFlag & FGCARET_MASK) {
	case FGCARET_FOCUS | FGCARET_ONOFF:

		switch(pDoc->m_TextRam.m_TypeCaret) {
		case 0: case 1: case 3: case 5:	// blink
			if ( (n = GetCaretBlinkTime() * 2) < 200 ) {
				m_CaretAnimeMax = 2;
				m_CaretAnimeClock = 100;
			} else if ( !pDoc->m_TextRam.IsOptEnable(TO_RLCARETANI) ) {
				m_CaretAnimeMax = 2;
				m_CaretAnimeClock = n / m_CaretAnimeMax;
			} else if ( n > 2000 ) {
				m_CaretAnimeMax = 20;
				m_CaretAnimeClock = n / m_CaretAnimeMax;
			} else {
				m_CaretAnimeClock = 50;
				m_CaretAnimeMax = n / m_CaretAnimeClock;
			}
			break;
		default:
			m_CaretAnimeMax = 1;
			m_CaretAnimeClock = 600;
			break;
		}

		CaretInitView();

		switch(pDoc->m_TextRam.m_TypeCaret) {
		case 0: case 1:	case 2:
			CaretSize(m_CharWidth, m_CharHeight);
			break;
		case 3: case 4:
			if ( (n = m_CharHeight / 9) < 1 )
				n = 1;
			po.y += (m_CharHeight - n);
			CaretSize(m_CharWidth, n);
			break;
		case 5: case 6:
			if ( (n = m_CharHeight / 9) < 1 )
				n = 1;
			CaretSize(n, m_CharHeight);
			break;
		}

		if ( (m_bCaretAllocCol && pDoc->m_TextRam.m_CaretColor != 0) || (!m_bCaretAllocCol && m_CaretColor != pDoc->m_TextRam.m_CaretColor) ) {
			if ( m_CaretBitmap.m_hObject != NULL )
				m_CaretBitmap.DeleteObject();
			m_bCaretAllocCol = FALSE;
			m_CaretColor = 0;
		}

		CaretPos(po);
		ImmSetPos(m_CaretX, m_CaretY);
		m_CaretFlag |= FGCARET_CREATE;
		DispCaret(TRUE);
		break;

	case FGCARET_FOCUS | FGCARET_ONOFF | FGCARET_CREATE:
		if ( pDoc->m_TextRam.m_TypeCaret == 3 || pDoc->m_TextRam.m_TypeCaret == 4 )
			po.y += (m_CharHeight - m_CaretSize.cy);
		CaretPos(po);
		ImmSetPos(m_CaretX, m_CaretY);
		DispCaret(TRUE);
		break;

	case FGCARET_FOCUS:
		ImmSetPos(m_CaretX, m_CaretY);
		break;

	case FGCARET_FOCUS | FGCARET_CREATE:
		ImmSetPos(m_CaretX, m_CaretY);
		// no break
	case FGCARET_CREATE:
	case FGCARET_ONOFF | FGCARET_CREATE:
		KillCaret();
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////

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
		_tcsncpy(LogFont.lfFaceName, fontName, LF_FACESIZE);
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
		ImmReleaseContext(m_hWnd, hIMC);
	}
	return rt;
}

/////////////////////////////////////////////////////////////////////////////

void CRLoginView::SetGhostWnd(BOOL sw)
{
	if ( sw ) {		// Create
		if ( m_pGhost != NULL )
			return;

		CRect rect;
		CRLoginDoc *pDoc = GetDocument();

		if ( pDoc->m_TextRam.IsOptEnable(TO_RLGWDIS) )
			return;

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
int CRLoginView::GetClipboard(CBuffer *bp)
{
	CStringA buf;
	CString text, tmp;
	CRLoginDoc *pDoc = GetDocument();

	if ( !((CMainFrame *)AfxGetMainWnd())->GetClipboardText(text) )
		return FALSE;

	for ( LPCTSTR p = text ; *p != _T('\0') ; ) {
		if ( p[0] == _T('\x0D') && p[1] == _T('\x0A') ) {
			pDoc->m_TextRam.AddOscClipCrLf(tmp);
			p += 2;
		} else if ( p[0] == _T('\x0A') && p[1] == _T('\x0D') ) {
			pDoc->m_TextRam.AddOscClipCrLf(tmp);
			p += 2;
		} else if ( p[0] == _T('\x0D') ) {
			pDoc->m_TextRam.AddOscClipCrLf(tmp);
			p += 1;
		} else if ( p[0] == _T('\x0A') ) {
			pDoc->m_TextRam.AddOscClipCrLf(tmp);
			p += 1;
		} else if ( p[0] == _T('\x1A') && p[1] == _T('\0') ) {
			p += 1;
		} else
			tmp += *(p++);
	}

	pDoc->m_TextRam.m_IConv.StrToRemote(pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode], tmp, buf);
	bp->Apend((LPBYTE)(LPCSTR)buf, buf.GetLength());

	return TRUE;
}
int CRLoginView::SetClipboard(CBuffer *bp)
{
	CBuffer buf, tmp;
	CRLoginDoc *pDoc = GetDocument();

	pDoc->m_TextRam.m_IConv.RemoteToStr(pDoc->m_TextRam.m_SendCharSet[pDoc->m_TextRam.m_KanjiMode], bp, &buf);
	
	for ( LPCTSTR p = buf ; *p != _T('\0') ; ) {
		if ( p[0] == _T('\x0D') && p[1] == _T('\x0A') ) {
			tmp += *(p++);
			tmp += *(p++);
		} else if ( p[0] == _T('\x0A') && p[1] == _T('\x0D') ) {
			tmp += _T('\x0D');
			tmp += _T('\x0A');
			p += 2;
		} else if ( p[0] == _T('\x0D') ) {
			tmp += *(p++);
			tmp += _T('\x0A');
		} else if ( p[0] == _T('\x0A') ) {
			tmp += _T('\x0D');
			tmp += *(p++);
		} else
			tmp += *(p++);
	}

	((CMainFrame *)AfxGetMainWnd())->SetClipboardText(tmp);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスのメッセージ ハンドラ

int CRLoginView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

#ifdef	USE_OLE
	m_DropTarget.Register(this);
#else
	DragAcceptFiles();
#endif

	m_ToolTip.Create(this, TTS_ALWAYSTIP | TTS_BALLOON);
	m_ToolTip.SetMaxTipWidth(512);
	m_ToolTip.Activate(TRUE);

	m_SleepView = FALSE;
	m_SleepCount = 0;
	SetTimer(VTMID_SLEEPTIMER, VIEW_SLEEP_MSEC, NULL);

	((CRLoginApp *)AfxGetApp())->AddIdleProc(IDLEPROC_VIEW, this);
	DelayInvalThreadStart();

	return 0;
}
void CRLoginView::OnDestroy()
{
	DelayInvalThreadEndof();
	((CRLoginApp *)AfxGetApp())->DelIdleProc(IDLEPROC_VIEW, this);

	CView::OnDestroy();
}

void CRLoginView::SetFrameRect(int cx, int cy)
{
	CRect rect;
	CRLoginDoc *pDoc = GetDocument();
	CChildFrame *pFrame = GetFrameWnd();

	if ( cx < 0 || cy < 0 ) {
		GetClientRect(rect);
		cx = rect.Width();
		cy = rect.Height();
	}

	if ( (m_Width  = cx - pDoc->m_TextRam.m_ScrnOffset.left - pDoc->m_TextRam.m_ScrnOffset.right)  <= 0 )
		m_Width  = 1;

	if ( (m_Height = cy - pDoc->m_TextRam.m_ScrnOffset.top - pDoc->m_TextRam.m_ScrnOffset.bottom) <= 0 )
		m_Height = 1;

	pFrame->GetClientRect(rect);
	pFrame->m_Width = m_Width;

	if ( (pFrame->m_Height = rect.Height() - 4) <= 0 )
		pFrame->m_Height = 1;

	pDoc->m_TextRam.InitText(pFrame->m_Width, pFrame->m_Height);

	pFrame->m_Cols  = pDoc->m_TextRam.m_Cols;
	pFrame->m_Lines = pDoc->m_TextRam.m_Lines;
}
void CRLoginView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

//	TRACE("CRLoginView::OnSize(%d,%d) %d\n", cx, cy, ((CChildFrame *)GetFrameWnd())->m_bInit);

	if ( nType == SIZE_MINIMIZED || !((CChildFrame *)GetFrameWnd())->m_bInit )
		return;

	CString tmp;
	CRLoginDoc *pDoc = GetDocument();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();

	SetFrameRect(cx, cy);
	pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);

	tmp.Format(_T("%d x %d"), pDoc->m_TextRam.m_Cols, pDoc->m_TextRam.m_Lines);

	if ( pDoc->m_TextRam.IsInitText() && !pDoc->m_TextRam.IsOptEnable(TO_RLMWDIS) )
		m_MsgWnd.Message(tmp, this, pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.bcol]);

	if ( m_BtnWnd.m_hWnd != NULL )
		m_BtnWnd.DoButton(this, NULL);
}

void CRLoginView::OnMove(int x, int y) 
{
	CRLoginDoc *pDoc = GetDocument();

	CView::OnMove(x, y);

	if ( pDoc != NULL && pDoc->m_TextRam.IsInitText() )
		OnUpdate(this, UPDATE_INVALIDATE, NULL);
}

void CRLoginView::OnInitialUpdate()
{
	//CView::OnInitialUpdate();
}
void CRLoginView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int y;
	CPoint point;
	CRect rect, box;
	CString str;
	CRLoginDoc *pDoc = GetDocument();
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
	CChildFrame *pFrame = GetFrameWnd();

	ASSERT(pFrame != NULL);

	//TRACE("UpdateView %d\n", (int)lHint);

	switch(lHint) {
	case UPDATE_RESIZE:
		GetWindowRect(rect);
		SendMessage(WM_SIZE, SIZE_MAXSHOW, MAKELPARAM(rect.Width(), rect.Height()));
		return;

	case UPDATE_TEKFLUSH:
		if ( pDoc->m_TextRam.IsOptEnable(TO_RLTEKINWND) ) {
			if ( m_TekBitmap.m_hObject != NULL )
				m_TekBitmap.DeleteObject();
			InvalidateFullText();
		}
		return;

	case UPDATE_VISUALBELL:
		if ( !m_VisualBellFlag ) {
			SetTimer(VTMID_VISUALBELL, 50, NULL);
			m_VisualBellFlag = TRUE;
			InvalidateFullText();
			if ( m_pGhost != NULL )
				m_pGhost->Invalidate(FALSE);
		}
		return;

	case UPDATE_SETCURSOR:
		GetClientRect(rect);
		ClientToScreen(rect);
		GetCursorPos(&point);
		if ( rect.PtInRect(point) )
			OnSetCursor(NULL, 0, 0);
		return;

	case UPDATE_TYPECARET:
		UpdateCaret();
		return;

	case UPDATE_CANCELBTN:
		if ( pHint == NULL ) {
			if ( m_BtnWnd.m_hWnd != NULL ) {
				m_ToolTip.DelTool(&m_BtnWnd);
				m_BtnWnd.DoButton(NULL, NULL);
			}
		} else {
			m_BtnWnd.DoButton(this, &(pDoc->m_TextRam));
			m_ToolTip.AddTool(&m_BtnWnd, (LPCTSTR)pHint);
//			if ( (m_CaretFlag & FGCARET_FOCUS) != 0 || pDoc->GetViewCount() <= 1 )
				m_BtnWnd.ShowWindow(SW_SHOW);
		}
		return;

	case UPDATE_DISPINDEX:
		pMain->GetTabTitle(pFrame, str);
		m_MsgWnd.Message(str, this, pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.bcol]);
		return;

	case UPDATE_WAKEUP:
		m_SleepCount = 0;
		return;

	case UPDATE_SCROLLOUT:
		m_HisOfs = 0;
		lHint = UPDATE_INVALIDATE;
		KillScrollTimer();
		break;

	case UPDATE_UPDATEWINDOW:
		UpdateWindow();
		return;

	case UPDATE_DISPMSG:
		m_MsgWnd.Message((LPCTSTR)pHint, this, pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.bcol]);
		return;
	}

	if ( (m_CaretFlag & FGCARET_FOCUS) != 0 && pSender != this && m_ScrollOut == FALSE &&
			(lHint == UPDATE_INVALIDATE || lHint == UPDATE_TEXTRECT || lHint == UPDATE_GOTOXY) ) {
		y = pDoc->m_TextRam.m_CurY - m_HisMin + m_HisOfs;
		if ( y >= m_Lines ) {
			KillScrollTimer();
			m_HisOfs = m_Lines - (pDoc->m_TextRam.m_CurY - m_HisMin) - 1;
			lHint = UPDATE_INVALIDATE;
		} else if ( y < 0 ) {
			KillScrollTimer();
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

		if ( (m_CharWidth  = pFrame->m_Width  / pFrame->m_Cols) > pDoc->m_TextRam.m_CharWidth )
			m_CharWidth = pDoc->m_TextRam.m_CharWidth;
		if ( (m_CharHeight = pFrame->m_Height / pFrame->m_Lines) > pDoc->m_TextRam.m_CharHeight )
			m_CharHeight = pDoc->m_TextRam.m_CharHeight;
		
		KillCaret();

		if ( !pDoc->m_TextRam.m_BitMapFile.IsEmpty() || pDoc->m_TextRam.m_TextBitMap.m_bEnable ) {
			CDC *pDC = GetDC();
			GetClientRect(rect);
			str = pDoc->m_TextRam.m_BitMapFile;
			pDoc->EntryText(str);
			m_BmpFile.LoadFile(str);
			m_pBitmap = m_BmpFile.GetBitmap(pDC, rect.Width(), rect.Height(), pDoc->m_TextRam.GetBackColor(pDoc->m_TextRam.m_DefAtt), pDoc->m_TextRam.m_BitMapAlpha, pDoc->m_TextRam.m_BitMapStyle, this);
			if ( pDoc->m_TextRam.m_TextBitMap.m_bEnable && !pDoc->m_TextRam.m_TextBitMap.m_Text.IsEmpty() ) {
				str = pDoc->m_TextRam.m_TextBitMap.m_Text;
				pDoc->EntryText(str, L"");
				m_pBitmap = m_BmpFile.GetTextBitmap(pDC, rect.Width(), rect.Height(), pDoc->m_TextRam.GetBackColor(pDoc->m_TextRam.m_DefAtt), &(pDoc->m_TextRam.m_TextBitMap), str, pDoc->m_TextRam.m_BitMapAlpha, pDoc->m_TextRam.m_BitMapStyle, this);
			}
			if ( m_BmpFile.m_Style == MAPING_DESKTOP )
				((CMainFrame *)::AfxGetMainWnd())->m_UseBitmapUpdate = TRUE;
			ReleaseDC(pDC);
		} else
			m_pBitmap = NULL;

		if ( m_BtnWnd.m_hWnd == NULL && pDoc->m_TextRam.m_bOscActive && pDoc->m_TextRam.m_IntCounter >= 10 ) {
			m_BtnWnd.DoButton(this, &(pDoc->m_TextRam));
			m_ToolTip.AddTool(&m_BtnWnd, pDoc->m_TextRam.m_SeqMsg);
			m_BtnWnd.ShowWindow(SW_SHOW);
		}

		// No break

	case UPDATE_INVALIDATE:
		if ( m_HisOfs > 0 )
			m_HisOfs += pDoc->m_TextRam.m_HisUse;
		if ( m_HisOfs < 0 )
			m_HisOfs = 0;
		else if ( (m_Lines + m_HisOfs) > pDoc->m_TextRam.m_HisLen )
			m_HisOfs = pDoc->m_TextRam.m_HisLen - m_Lines;
		SCROLLINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		info.fMask  = SIF_PAGE | SIF_POS | SIF_RANGE;
		// Min=0, Max=99, Page=10, Pos = 90
		info.nMin   = 0;
		info.nMax   = pDoc->m_TextRam.m_HisLen - 1;
		info.nPage  = m_Lines;
		info.nPos   = pDoc->m_TextRam.m_HisLen - m_Lines - m_HisOfs;
		info.nTrackPos = 0;
		SetScrollInfo(SB_VERT, &info, TRUE);
		m_ClipUpdateLine = FALSE;
		ResetCellSize();
		InvalidateFullText();
		if ( m_pGhost != NULL )
			m_pGhost->Invalidate(FALSE);
		break;

	case UPDATE_TEXTRECT:
		rect = *((CRect *)pHint);
		if ( m_ClipUpdateLine ) {
			rect.left  = 0;
			rect.right = pDoc->m_TextRam.m_Cols;
		}
		InvalidateTextRect(rect);
		CaretInitView();
		break;

	case UPDATE_CLIPCLAER:
	case UPDATE_CLIPERA:
		CalcPosRect(rect);
		if ( IsClipLineMode() || m_ClipUpdateLine ) {
			rect.left  = 0;
			rect.right = pDoc->m_TextRam.m_Cols;
			InvalidateTextRect(rect);
		} else if ( IsClipRectMode() ) {
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
		CaretInitView();
		break;
	}

	m_CaretFlag &= ~FGCARET_ONOFF;
	m_CaretX = GetGrapPos(pDoc->m_TextRam.m_CurX, (pDoc->m_TextRam.m_CurY + m_HisOfs - m_HisMin));
	m_CaretY = m_Height * (pDoc->m_TextRam.m_CurY + m_HisOfs - m_HisMin) / m_Lines + pDoc->m_TextRam.m_ScrnOffset.top + m_TopOffset;

	if ( m_CaretX < 0 || m_CaretX >= (m_Width  + pDoc->m_TextRam.m_ScrnOffset.left) ||
		 m_CaretY < 0 || m_CaretY >= (m_Height + pDoc->m_TextRam.m_ScrnOffset.top) ) {
		m_CaretX = m_CaretY = 0;
		m_ScrollOut = TRUE;
	} else if ( pDoc->m_pSock != NULL && pDoc->m_pSock->m_bConnect && pDoc->m_TextRam.m_DispCaret )
		m_CaretFlag |= FGCARET_ONOFF;

	SetCaret();
}

void CRLoginView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();

	//TRACE("OnChar %02x[%d](%04x)\n", nChar, nRepCnt, nFlags);

	CView::OnChar(nChar, nRepCnt, nFlags);

	if ( (nFlags & 0x4000) != 0 && !pDoc->m_TextRam.IsOptEnable(TO_DECARM) )
		return;

	if ( (nFlags & 0x2000) != 0 )	// with Alt key
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
	CKeyNode *pNode;
	CRLoginDoc *pDoc = GetDocument();

	if ( nChar > 255 || (num = pDoc->m_TextRam.m_ModKeyTab[nChar]) < 0 )
		return FALSE;

	if ( (mod = pDoc->m_TextRam.m_ModKey[num]) < 0 )
		return FALSE;

	if ( !pDoc->m_TextRam.IsOptEnable(TO_RLMODKEY) && (pNode = pDoc->m_KeyTab.FindMaps(nChar, nStat)) != NULL && (n = CKeyNodeTab::GetCmdsKey((LPCWSTR)pNode->m_Maps)) > 0 ) {
		AfxGetMainWnd()->PostMessage(WM_COMMAND, (WPARAM)n);
		return TRUE;
	}

	if ( (nStat & MASK_SHIFT) != 0 ) msk |= 001;
	if ( (nStat & MASK_ALT)   != 0 ) msk |= 002;
	if ( (nStat & MASK_CTRL)  != 0 ) msk |= 004;

	if ( (pNode = pDoc->m_KeyTab.FindMaps(nChar, (nStat & ~(MASK_SHIFT | MASK_CTRL | MASK_ALT)))) == NULL )
		goto NOTDEF;

	n = pNode->m_Maps.GetSize() / sizeof(WCHAR);
	p = (WCHAR *)(pNode->m_Maps.GetPtr());

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
	CKeyNode *pNode;
	CRLoginDoc *pDoc = GetDocument();

	//CView::OnKeyDown(nChar, nRepCnt, nFlags);

	//TRACE("KeyDown %02X[%d](%04X)\n", nChar, nRepCnt, nFlags);

	if ( nChar == VK_MENU )
		return TRUE;
	else if ( nChar == VK_SHIFT || nChar == VK_CONTROL ) {
		if ( m_ClipFlag > 0 && m_ClipFlag < 6 ) {
			OnUpdate(this, UPDATE_CLIPCLAER, NULL);
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

	if ( (nFlags & 0x4000) != 0 && !pDoc->m_TextRam.IsOptEnable(TO_DECARM) )
		return TRUE;

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

	if ( (st & (MASK_SHIFT | MASK_CTRL | MASK_ALT)) == 0 && (GetKeyState(VK_NUMLOCK) & 0x01) != 0 && nChar >= VK_NUMPAD0 && nChar <= VK_DIVIDE )
		return TRUE;

	if ( (st & MASK_ALT) != 0 && nChar < 256 && (pDoc->m_TextRam.m_MetaKeys[nChar / 32] & (1 << (nChar % 32))) != 0 ) {
		if ( (st & MASK_CTRL) != 0 && nChar >= 'A' && nChar <= 'Z' ) {
			tmp.PutWord(L'\033');
			tmp.PutWord(nChar - L'@');
			SendBuffer(tmp);
			return FALSE;
		} else if ( (pNode = pDoc->m_KeyTab.FindMaps(nChar, st & ~MASK_ALT)) != NULL && pNode->m_Maps.GetSize() == sizeof(WCHAR) ) {
			n = *((WORD *)(pNode->m_Maps.GetPtr()));
			tmp.Clear();
			tmp.PutWord(L'\033');
			tmp.PutWord(n);
			SendBuffer(tmp);
			return FALSE;
		} else
			return TRUE;
	}

	if ( (pNode = pDoc->m_KeyTab.FindMaps(nChar, st)) != NULL ) {
		// ESCキーのCKM操作		TO_RLCKMESCによりCKM時にコード変換を抑制
		if ( (st & MASK_CKM) != 0 && wcscmp((LPCWSTR)pNode->m_Maps, L"\033O[") == 0 && !pDoc->m_TextRam.IsOptEnable(TO_RLCKMESC) ) {
			tmp = L"\033";
			SendBuffer(tmp);
		} else if ( (n = CKeyNodeTab::GetCmdsKey((LPCWSTR)pNode->m_Maps)) > 0 ) {
			AfxGetMainWnd()->PostMessage(WM_COMMAND, (WPARAM)n);
#ifdef	USE_KEYSCRIPT
		} else if ( ((LPCWSTR)pNode->m_Maps)[0] == L'%' ) {
			if ( pDoc->m_pScript != NULL )
				pNode->m_ScriptNumber = pDoc->m_pScript->ExecStr(UniToTstr((LPCWSTR)pNode->m_Maps + 1), pNode->m_ScriptNumber);
#endif
		} else
			SendBuffer(pNode->m_Maps);

		return FALSE;

	} else if ( nChar == VK_BACK ) {
		// If DECBKM is set, <x works as a backspace key. 
		// When you press <x , the terminal sends a BS(0x08) character to the host.
		// If DECBKM is reset, <x works as a delete key. 
		// When you press <x , the terminal sends a DEL(0x7F) character to the host.
		tmp = (pDoc->m_TextRam.IsOptEnable(TO_DECBKM) ? L"\x08" : L"\x7F");
		SendBuffer(tmp);
		return FALSE;
	}

	return TRUE;
}
void CRLoginView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_SHIFT || nChar == VK_CONTROL ) {
		if ( m_ClipFlag > 0 && m_ClipFlag < 6 ) {
			OnUpdate(this, UPDATE_CLIPCLAER, NULL);
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

	m_CaretFlag |= FGCARET_FOCUS;
	SetCaret();

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTFOCEVT) )
		pDoc->m_TextRam.UNGETSTR(_T("%sI"), pDoc->m_TextRam.m_RetChar[RC_CSI]);

	//if ( m_BtnWnd.m_hWnd != NULL )
	//	m_BtnWnd.ShowWindow(SW_SHOW);
}
void CRLoginView::OnKillFocus(CWnd* pNewWnd) 
{
	CRLoginDoc *pDoc = GetDocument();

	CView::OnKillFocus(pNewWnd);

	m_CaretFlag &= ~FGCARET_FOCUS;
	SetCaret();

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTFOCEVT) )
		pDoc->m_TextRam.UNGETSTR(_T("%sO"), pDoc->m_TextRam.m_RetChar[RC_CSI]);

	//if ( m_BtnWnd.m_hWnd != NULL )
	//	m_BtnWnd.ShowWindow(SW_HIDE);

	pDoc->UpdateAllViews(NULL, UPDATE_INVALIDATE, NULL);
}
void CRLoginView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	if ( bActivate && pActivateView->m_hWnd == m_hWnd ) {
		CRLoginDoc *pDoc = GetDocument();
		CChildFrame *pFrame = GetFrameWnd();

		ASSERT(pFrame != NULL);

		m_ActiveFlag = TRUE;
		SetFrameRect(-1, -1);
		pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
	} else if ( !bActivate && pDeactiveView->m_hWnd == m_hWnd )
		m_ActiveFlag = FALSE;
}

LRESULT CRLoginView::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
	HIMC hIMC;
	CRLoginDoc *pDoc = GetDocument();

	switch(wParam) {
	case IMN_SETOPENSTATUS:
		if ( (hIMC = ImmGetContext(m_hWnd)) != NULL ) {
			m_bImmActive = ImmGetOpenStatus(hIMC);
			ImmReleaseContext(m_hWnd, hIMC);
	    }
		if ( m_bImmActive )
			pDoc->m_TextRam.EnableOption(TO_IMECTRL);
		else
			pDoc->m_TextRam.DisableOption(TO_IMECTRL);
		break;
    case IMN_SETSTATUSWINDOWPOS:
		ImmSetPos(m_CaretX, m_CaretY);
		break;
    default:
		return DefWindowProc(WM_IME_NOTIFY, wParam, lParam);
    }

	return TRUE;
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
		((CMainFrame *)::AfxGetMainWnd())->SetIdleTimer(FALSE);
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
		((CMainFrame *)::AfxGetMainWnd())->SetIdleTimer(TRUE);
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
	int n, mx, my;
	int style, anime, move;
	CRect frame, rect;
	CRLoginDoc *pDoc = GetDocument();

	CView::OnTimer(nIDEvent);

	switch(nIDEvent) {
	case VTMID_MOUSEMOVE:		// ClipTimer
		OnMouseMove(m_ClipKeyFlags, m_ClipSavePoint);
		break;

	case VTMID_VISUALBELL:		// VisualBell
		KillTimer(nIDEvent);
		m_VisualBellFlag = FALSE;
		InvalidateFullText();
		break;

	case VTMID_WHEELMOVE:		// Wheel Timer
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
			m_WheelTimer = 0;
		}

		OnUpdate(this, UPDATE_INVALIDATE, NULL);
		break;

	case VTMID_SMOOTHSCR:
		if ( m_RMovePixel == 0 ) {
			KillScrollTimer();
			break;
		}

		if ( (clock() - m_RDownClock) > 10000 )
			m_RMovePixel = m_RMovePixel * 90 / 100;

		n = m_TopOffset + m_RMovePixel;

		m_HisOfs += n / m_CharHeight;
		m_TopOffset = n % m_CharHeight;

		if ( m_HisOfs <= 0 ) {
			m_HisOfs = 0;
			m_TopOffset = 0;
			KillTimer(m_SmoothTimer);
			m_SmoothTimer = 0;

		} else if ( m_HisOfs >= (pDoc->m_TextRam.m_HisLen - m_Lines) ) {
			m_HisOfs = pDoc->m_TextRam.m_HisLen - m_Lines;
			m_TopOffset = 0;
			KillTimer(m_SmoothTimer);
			m_SmoothTimer = 0;
		}

		OnUpdate(this, UPDATE_INVALIDATE, NULL);
		break;

	case VTMID_GOZIUPDATE:		// Gozi Timer
#if USE_GOZI == 1
		rect.SetRect(m_GoziPos.x, m_GoziPos.y, m_GoziPos.x + 32, m_GoziPos.y + 32);
		InvalidateRect(rect, FALSE);

		if ( !m_GoziView ) {
			KillTimer(nIDEvent);
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
			CCharCell *vp;
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

#elif USE_GOZI == 2
		rect.SetRect(m_GoziPos.x, m_GoziPos.y, m_GoziPos.x + 32, m_GoziPos.y + 32);
		InvalidateRect(rect, FALSE);

		if ( !m_GoziView || pDoc == NULL || !pDoc->m_TextRam.IsInitText() ) {
			KillTimer(nIDEvent);
			break;
		}

		GetClientRect(rect);

		move  = m_GoziStyle & 0x0F;
		style = m_GoziStyle >> 4;
		anime = style & 3;
		style &= ~3;

		if ( m_GoziPos.x > rect.right )
			m_GoziPos.x = rect.right - 32;

		if ( m_GoziPos.y > rect.bottom )
			m_GoziPos.y = rect.bottom - 32;

		if ( ((mx = m_CaretX + m_CharWidth + 2) + 32) > rect.right )
			mx = m_CaretX - 32;
		else if ( mx < 0 )
			mx = 0;

		if ( ((my = m_CaretY + m_CharHeight - 32) + 32) > rect.bottom )
			my = rect.bottom - 32;
		else if ( my < 0 )
			my = 0;

		move = 0;

		if ( m_GoziPos.x < mx )
			move |= 1;
		else if ( m_GoziPos.x > mx )
			move |= 2;

		if ( m_GoziPos.y < my )
			move |= 4;
		else if ( m_GoziPos.y > my )
			move |= 8;

		// 0-3=Down, 4-7=Up, 8-11=Right, 12-15=Left, 16-19=SRight, 20-23=SLeft, 24-27=SFire 

		if ( move == 0 ) {
			if ( style == 16 || style == 20 ) {
				if ( --m_GoziCount < 0 )
					style = 24;
			} else if ( style != 24 ) {
				m_GoziCount = 16 + rand() % 24;
				style = (mx == (m_CaretX - 32) ? 16 : 20);
			}
		}

		if ( (move & 1) != 0 ) {
			if ( (m_GoziPos.x += 8) > mx )
				m_GoziPos.x = mx;
			style = 8;
		}
		if ( (move & 2) != 0 ) {
			if ( (m_GoziPos.x -= 8) < mx )
				m_GoziPos.x = mx;
			style = 12;
		}
		if ( (move & 4) != 0 ) {
			if ( (m_GoziPos.y += 8) > my )
				m_GoziPos.y = my;
			style = 4;
		}
		if ( (move & 8) != 0 ) {
			if ( (m_GoziPos.y -= 8) < my )
				m_GoziPos.y = my;
			style = 0;
		}

		if ( ++anime > 3 ) anime = 0;
		m_GoziStyle = ((style + anime) << 4) | move;

		rect.SetRect(m_GoziPos.x, m_GoziPos.y, m_GoziPos.x + 32, m_GoziPos.y + 32);
		InvalidateRect(rect, FALSE);

#elif USE_GOZI == 3 || USE_GOZI == 4

		for ( n = 0 ; n < m_GoziMax ; n++ ) {
			rect.SetRect(m_GoziData[n].m_GoziPos.x, m_GoziData[n].m_GoziPos.y, m_GoziData[n].m_GoziPos.x + 16, m_GoziData[n].m_GoziPos.y + 16);
			InvalidateRect(rect, FALSE);
		}

		if ( !m_GoziView || pDoc == NULL || !pDoc->m_TextRam.IsInitText() ) {
			KillTimer(nIDEvent);
			break;
		}

		GetClientRect(frame);

		for ( n = 0 ; n < m_GoziMax ; n++ ) {

			move  = m_GoziData[n].m_GoziStyle & 0x0F;
			style = m_GoziData[n].m_GoziStyle >> 4;
			anime = style % 6;
			style = (style / 12) * 12;

			if ( --m_GoziData[n].m_GoziCount < 0 ) {
				m_GoziData[n].m_GoziCount = 24 + rand() % 32;
				static int move_tab[] = { 1, 2, 4, 5, 6, 8, 9, 10, 0, 0, 0 };
				move = move_tab[rand() % 11];
				anime = rand() % 6;

				if ( move == 0 ) {
					if ( ((mx = m_CaretX + m_CharWidth + 2) + 16) > frame.right )
						mx = m_CaretX - 16;
					else if ( mx < 0 )
						mx = 0;

					if ( ((my = m_CaretY + m_CharHeight - 16) + 16) > frame.bottom )
						my = frame.bottom - 16;
					else if ( my < 0 )
						my = 0;

					if ( m_GoziData[n].m_GoziPos.x < mx )
						move |= 1;
					else if ( m_GoziData[n].m_GoziPos.x > mx )
						move |= 2;

					if ( m_GoziData[n].m_GoziPos.y < my )
						move |= 4;
					else if ( m_GoziData[n].m_GoziPos.y > my )
						move |= 8;
				}
			}

			if ( (move & 1) != 0 ) m_GoziData[n].m_GoziPos.x += 2;
			if ( (move & 2) != 0 ) m_GoziData[n].m_GoziPos.x -= 2;
			if ( (move & 4) != 0 ) m_GoziData[n].m_GoziPos.y += 2;
			if ( (move & 8) != 0 ) m_GoziData[n].m_GoziPos.y -= 2;

			if ( m_GoziData[n].m_GoziPos.x < 0 ) { m_GoziData[n].m_GoziPos.x = 0; move = (move & 12) | 1; }
			if ( m_GoziData[n].m_GoziPos.y < 0 ) { m_GoziData[n].m_GoziPos.y = 0; move = (move &  3) | 4; }

			if ( (m_GoziData[n].m_GoziPos.x + 16) > frame.right  ) { m_GoziData[n].m_GoziPos.x = frame.right  - 16; move = (move & 12) | 2; }
			if ( (m_GoziData[n].m_GoziPos.y + 16) > frame.bottom ) { m_GoziData[n].m_GoziPos.y = frame.bottom - 16; move = (move &  3) | 8; }

			style += ((move & 1) != 0 ? 0 : 6);

			if ( ++anime >= 6 ) anime = 0;
			m_GoziData[n].m_GoziStyle = ((style + anime) << 4) | move;

			rect.SetRect(m_GoziData[n].m_GoziPos.x, m_GoziData[n].m_GoziPos.y, m_GoziData[n].m_GoziPos.x + 16, m_GoziData[n].m_GoziPos.y + 16);
			InvalidateRect(rect, FALSE);
		}

#else	// !USE_GOZI
		KillTimer(nIDEvent);
#endif	// USE_GOZI
		break;

	case VTMID_SLEEPTIMER:		// Matrix Timer
		if ( m_SleepView == FALSE ) {
			if ( pDoc == NULL || !pDoc->m_TextRam.IsInitText() || !pDoc->m_TextRam.IsOptEnable(TO_SLEEPTIMER) )
				m_SleepCount = 0;
			else if ( ++m_SleepCount >= pDoc->m_TextRam.m_SleepMax ) {
				m_SleepView = TRUE;
				if ( m_MatrixCols.GetSize() < pDoc->m_TextRam.m_ColsMax )
					m_MatrixCols.SetSize(pDoc->m_TextRam.m_ColsMax);
				for ( int n = 0 ; n < m_Cols ; n++ )
					m_MatrixCols[n] = rand() % (m_Lines + 25);
				SetTimer(VTMID_SLEEPTIMER, 200, NULL);
			}
		} else {
			if ( m_SleepCount <= 0 ) {
				m_SleepView = FALSE;
				SetTimer(VTMID_SLEEPTIMER, VIEW_SLEEP_MSEC, NULL);
			} else {
				while ( m_MatrixCols.GetSize() < pDoc->m_TextRam.m_ColsMax )
					m_MatrixCols.Add(rand() % (m_Lines + 25));
				for ( mx = 0 ; mx < m_Cols ; mx++ ) {
					if ( ++m_MatrixCols[mx] > (m_Lines + 24) )
						m_MatrixCols[mx] = 0 - (rand() % 10);
				}
			}
			InvalidateFullText();
		}
		break;

	case VTMID_RCLICKCHECK:
		KillTimer(nIDEvent);
		PopUpMenu(m_RDownPoint);
		break;

	case VTMID_INVALIDATE:
		PollingDeleyInval();
		break;
	}
}

void CRLoginView::GetMousePos(int *sw, int *x, int *y)
{
	CPoint po;
	
	if ( !GetCursorPos(&po) )
		return;

	ScreenToClient(&po);
	CalcGrapPoint(po, x, y);

#if 0
	*sw = 0;

	if ( (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 )
		*sw |= MK_LBUTTON;

	if ( (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0 )
		*sw |= MK_MBUTTON;

	if ( (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0 )
		*sw |= MK_RBUTTON;
#else
	*sw = m_LastMouseFlags;
#endif
}

void CRLoginView::PopUpMenu(CPoint point)
{
	CCmdUI state;
	CMenu *pMenu;
	CMenuLoad DefMenu;
	CRLoginDoc *pDoc = GetDocument();

	int x, y;
	CCharCell *vp;

	CalcGrapPoint(point, &x, &y);
	vp = pDoc->m_TextRam.GETVRAM(x, y);

	if ( IS_IMAGE(vp->m_Vram.mode) )
		m_pSelectGrapWnd = pDoc->m_TextRam.GetGrapWnd(vp->m_Vram.pack.image.id);
	else
		m_pSelectGrapWnd = NULL;

	if ( (pMenu = GetMainWnd()->GetMenu()) == NULL ) {
		if ( !DefMenu.LoadMenu(IDR_RLOGINTYPE) )
			return;
		pMenu = &DefMenu;
	}

	pDoc->SetMenu(pMenu);
	GetMainWnd()->SetMenuBitmap(pMenu);

	if ( (pMenu = CMenuLoad::GetItemSubMenu(ID_EDIT_COPY, pMenu)) == NULL )
		return;

	if ( m_pSelectGrapWnd != NULL ) {
		pMenu->InsertMenu(4, MF_BYPOSITION, IDM_IMAGEGRAPCOPY, CStringLoad(IDM_IMAGEGRAPCOPY));
		pMenu->InsertMenu(5, MF_BYPOSITION, IDM_IMAGEGRAPSAVE, CStringLoad(IDM_IMAGEGRAPSAVE));
		pMenu->InsertMenu(6, MF_BYPOSITION, IDM_IMAGEGRAPHIST, CStringLoad(IDM_IMAGEGRAPHIST));
		pMenu->InsertMenu(7, MF_BYPOSITION | MF_SEPARATOR);
	}

	state.m_pMenu = pMenu;
	state.m_nIndexMax = pMenu->GetMenuItemCount();
	for ( state.m_nIndex = 0 ; state.m_nIndex < state.m_nIndexMax ; state.m_nIndex++) {
		if ( (int)(state.m_nID = pMenu->GetMenuItemID(state.m_nIndex)) <= 0 )
			continue;
		state.m_pSubMenu = NULL;
		state.DoUpdate(this, TRUE);
	}

	ClientToScreen(&point);
	((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(pMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, this);

	if ( m_pSelectGrapWnd != NULL ) {
		pMenu->DeleteMenu(6, MF_BYPOSITION);
		pMenu->DeleteMenu(5, MF_BYPOSITION);
		pMenu->DeleteMenu(4, MF_BYPOSITION);
	}
}

BOOL CRLoginView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	int n;
	int pos, ofs;
	CRLoginDoc *pDoc = GetDocument();
	clock_t now, msec;
	CKeyNode *pNode;

	if ( (nFlags & MK_SHIFT) != 0 ) {
		((CRLoginApp *)::AfxGetApp())->SendBroadCastMouseWheel(nFlags & ~MK_SHIFT, zDelta, pt);
		return TRUE;
	}

	KillScrollTimer();

	now = clock();
	msec = (now - m_WheelClock) * 1000 / CLOCKS_PER_SEC;	// msec

	if ( m_WheelClock == 0 || msec > 1000 ) {				// > 1sec
		m_WheelClock = now;
		m_WheelzDelta = 0;

	} else if ( msec < 10 ) {								// < 10ms
		m_WheelzDelta += zDelta;

	} else {
		m_WheelzDelta += zDelta;
		n = m_WheelzDelta * 30 / msec;						// 30ms
		if ( abs(n) > abs(zDelta) )
			zDelta = n;
		if ( msec > 200 ) {									// > 200ms
			m_WheelClock = now;
			m_WheelzDelta = 0;
		}
	}

	ofs = zDelta * pDoc->m_TextRam.m_WheelSize / (WHEEL_DELTA / 2);

	//TRACE("OnMouseWheel %x, %d(%d), (%d,%d)\n", nFlags, zDelta, ofs, pt.x, pt.y);

	if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
		int x, y;
		ScreenToClient(&pt);
		CalcGrapPoint(pt, &x, &y);
		for ( pos = (ofs < 0 ? (0 - ofs) : ofs) ; pos > 0 ; pos-- )
			pDoc->m_TextRam.MouseReport((ofs > 0 ? MOS_LOCA_WHUP : MOS_LOCA_WHDOWN), nFlags, x, y);

	} else if ( (!pDoc->m_TextRam.IsOptEnable(TO_RLMSWAPP) && pDoc->m_TextRam.IsOptEnable(TO_DECCKM)) || 
			    (nFlags & MK_CONTROL) != 0 || pDoc->m_TextRam.IsOptEnable(TO_RLMSWAPE) ) {
		if ( (pNode = pDoc->m_KeyTab.FindMaps((ofs > 0 ? VK_UP : VK_DOWN), (pDoc->m_TextRam.IsOptEnable(TO_DECCKM) ? MASK_CKM : 0))) != NULL ) {
			for ( pos = (ofs < 0 ? (0 - ofs) : ofs) ; pos > 0 ; pos-- )
				SendBuffer(pNode->m_Maps, TRUE);
		}

	} else {

		if ( m_WheelTimer != 0 ) {
			//if ( m_WheelDelta > 0 ) {
			//	if ( ofs < 0 )
			//		m_WheelDelta = ofs;
			//	else
			//		m_WheelDelta += ofs;
			//} else if ( m_WheelDelta < 0 ) {
			//	if ( ofs > 0 )
			//		m_WheelDelta = ofs;
			//	else
			//		m_WheelDelta += ofs;
			//} else
				m_WheelDelta = ofs;
		} else {
			if ( !pDoc->m_TextRam.IsOptEnable(TO_RLMOSWHL) && abs(ofs) > 5 ) {
				m_WheelDelta = ofs;
				m_WheelTimer = SetTimer(VTMID_WHEELMOVE, 100, NULL);			// 100ms
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

void CRLoginView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int x, y;
	CRLoginDoc *pDoc = GetDocument();

	m_LastMouseFlags = nFlags;
	m_FirstMousePoint = point;

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

	if ( m_LastMouseClock != 0 && !pDoc->m_TextRam.IsOptEnable(TO_RLCKCOPY) && (UINT)((clock() - m_LastMouseClock) * 1000 / CLOCKS_PER_SEC) <= GetDoubleClickTime() )
		m_bLButtonTrClk = TRUE;
	else
		m_bLButtonTrClk = FALSE;

	m_LastMouseClock = 0;

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

	m_LastMouseFlags = nFlags;

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

	OnUpdate(this, UPDATE_CLIPCLAER, NULL);

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

	if ( m_ClipFlag == 1 && m_ClipStaPos == m_ClipEndPos ) { // && !IsClipLineMode() ) {
		if ( pDoc->m_TextRam.IsOptEnable(TO_RLSTAYCLIP) || !pDoc->m_TextRam.IsOptEnable(TO_RLCKCOPY) ) {
			if ( (m_ClipKeyFlags & MK_SHIFT) == 0 ) {
				m_ClipStaPosSave = m_ClipStaPos;
				m_ClipFlag = 0;

			} else if ( m_ClipStaPosSave != (-1) ) {
				if ( m_ClipStaPosSave > m_ClipStaPos )
					m_ClipEndPos = m_ClipStaPosSave;
				else
					m_ClipStaPos = m_ClipStaPosSave;

				m_ClipKeyFlags &= ~MK_SHIFT;
				m_ClipFlag = 6;
			}
			OnUpdate(this, UPDATE_CLIPERA, NULL);

		} else {
			m_ClipFlag = 0;
			OnUpdate(this, UPDATE_CLIPERA, NULL);

#ifdef	USE_CLIENTKEY
			switch(HitTest(point)) {
			case 011: OnKeyDown(VK_LMOUSE_LEFT_TOP,      0, nFlags); break;
			case 012: OnKeyDown(VK_LMOUSE_LEFT_CENTER,   0, nFlags); break;
			case 013: OnKeyDown(VK_LMOUSE_LEFT_BOTTOM,   0, nFlags); break;
			case 021: OnKeyDown(VK_LMOUSE_CENTER_TOP,    0, nFlags); break;
			case 022: OnKeyDown(VK_LMOUSE_CENTER_CENTER, 0, nFlags); break;
			case 023: OnKeyDown(VK_LMOUSE_CENTER_BOTTOM, 0, nFlags); break;
			case 031: OnKeyDown(VK_LMOUSE_RIGHT_TOP,     0, nFlags); break;
			case 032: OnKeyDown(VK_LMOUSE_RIGHT_CENTER,  0, nFlags); break;
			case 033: OnKeyDown(VK_LMOUSE_RIGHT_BOTTOM,  0, nFlags); break;
			}
#endif
			return;
		}

	} else {
		m_ClipStaPosSave = m_ClipStaPos;
		m_ClipFlag = 6;
	}

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLCKCOPY) )
		OnEditCopy();
}
void CRLoginView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int x, y;
	CRLoginDoc *pDoc = GetDocument();

	m_LastMouseFlags = nFlags;

	CView::OnLButtonDblClk(nFlags, point);

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

	m_ClipStaPos = m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
	pDoc->m_TextRam.EditWordPos(&m_ClipStaPos, &m_ClipEndPos);

	m_ClipStaOld = m_ClipStaPos;
	m_ClipEndOld = m_ClipEndPos;
	m_ClipFlag = 2;
	m_ClipTimer = 0;
	m_ClipKeyFlags = nFlags;

	SetCapture();
	m_LastMouseClock = clock();

	OnUpdate(this, UPDATE_CLIPERA, NULL);
}

void CRLoginView::OnMButtonDown(UINT nFlags, CPoint point)
{
	CView::OnMButtonDown(nFlags, point);

	OnKeyDown(VK_MBUTTON, 0, nFlags);
}
void CRLoginView::OnXButtonDown(UINT nFlags, UINT nButton, CPoint point)
{
	CView::OnXButtonDown(nFlags, nButton, point);

	switch(nButton) {
	case XBUTTON1:
		OnKeyDown(VK_XBUTTON1, 0, nFlags);
		break;
	case XBUTTON2:
		OnKeyDown(VK_XBUTTON2, 0, nFlags);
		break;
	}
}

void CRLoginView::OnMouseMove(UINT nFlags, CPoint point) 
{
	int x, y, pos, tos;
	CRLoginDoc *pDoc = GetDocument();

	m_LastMouseFlags = nFlags;

	CView::OnMouseMove(nFlags, point);

	if ( m_ClipFlag == 0 || m_ClipFlag == 6 ) {
		if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
			CalcGrapPoint(point, &x, &y);
			pDoc->m_TextRam.MouseReport(MOS_LOCA_MOVE, nFlags, x, y);

			if ( pDoc->m_TextRam.m_MouseTrack == MOS_EVENT_HILT ) {
				OnUpdate(this, UPDATE_CLIPCLAER, NULL);
				m_ClipEndPos = pDoc->m_TextRam.GetCalcPos(x, y);
				OnUpdate(this, UPDATE_CLIPERA, NULL);
			}

		} else if ( m_RDownStat == 2 || ((m_RDownStat == 1 || m_RDownStat == 3) && abs(point.y - m_RDownPoint.y) > m_CharHeight) ) {

			y = point.y - m_RDownPoint.y;
			x = y % m_CharHeight;

			if ( (y = m_RDownOfs + y / m_CharHeight) < 0 ) {
				y = 0;
				x = 0;
			} else if ( y > (pDoc->m_TextRam.m_HisLen - m_Lines) ) {
				y = pDoc->m_TextRam.m_HisLen - m_Lines;
				x = 0;
			}

			if ( y != m_HisOfs || x != m_TopOffset ) {
				m_TopOffset = x;
				m_HisOfs = y;
				OnUpdate(this, UPDATE_INVALIDATE, NULL);
			}

			if ( (x = clock() - m_RDownClock) > 10 ) {
				m_RMovePixel = (point.y - m_RMovePoint.y) * 100 / x;
				m_RDownClock += x;
				m_RMovePoint = point;
			}

			m_RDownStat = 2;
		}
		return;
	}

	if ( m_ClipFlag != 1 )
		OnUpdate(this, UPDATE_CLIPCLAER, NULL);

	m_ClipKeyFlags = nFlags;

	if ( point.x < (0 - m_CharWidth * 3) || point.x > (m_Width + m_CharWidth * 3) )
		m_ClipKeyFlags |= 0x1000;
	else
		m_ClipKeyFlags &= 0xEFFF;

	if ( point.y < 0 || point.y > m_Height ) {
		pos = m_HisOfs;
		
		if ( point.y < 0 && point.y <= m_ClipSavePoint.y )
			pos -= (point.y / m_CharHeight);
		else if ( point.y > m_Height && point.y >= m_ClipSavePoint.y )
			pos -= ((point.y - m_Height) / m_CharHeight);

		m_ClipSavePoint = point;

		if ( pos < 0 )
			pos = 0;
		else if ( pos > (pDoc->m_TextRam.m_HisLen - m_Lines) )
			pos = pDoc->m_TextRam.m_HisLen - m_Lines;

		if ( pos != m_HisOfs ) {
			m_HisOfs = pos;
			OnUpdate(this, UPDATE_INVALIDATE, NULL);
		}

	} else if ( m_ClipTimer != 0 ) {
		KillTimer(m_ClipTimer);
		m_ClipTimer = 0;
	}

	CalcGrapPoint(point, &x, &y);
	pos = tos = pDoc->m_TextRam.GetCalcPos(x, y);

	switch(m_ClipFlag) {
	case 1:		// clip start
		if ( abs(m_FirstMousePoint.x - point.x) < m_CharWidth && abs(m_FirstMousePoint.y - point.y) < m_CharHeight )
			break;

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
void CRLoginView::KillScrollTimer()
{
	CRLoginDoc *pDoc = GetDocument();

	if ( m_WheelTimer != 0 ) {
		KillTimer(m_WheelTimer);
		m_WheelTimer = 0;
	}

	if ( m_SmoothTimer != 0 ) {
		KillTimer(m_SmoothTimer);
		m_SmoothTimer = 0;

		m_HisOfs += (m_TopOffset > 0 ? 1 : (-1));

		if ( m_HisOfs < 0 )
			m_HisOfs = 0;
		else if ( m_HisOfs > (pDoc->m_TextRam.m_HisLen - m_Lines) )
			m_HisOfs = pDoc->m_TextRam.m_HisLen - m_Lines;

		m_TopOffset = 0;
		OnUpdate(this, UPDATE_INVALIDATE, NULL);
	}
}
void CRLoginView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int x, y;
	CRLoginDoc *pDoc = GetDocument();

	m_LastMouseFlags = nFlags;

	CView::OnRButtonDown(nFlags, point);
	SetCapture();

	if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
		CalcGrapPoint(point, &x, &y);
		pDoc->m_TextRam.MouseReport(MOS_LOCA_RTDN, nFlags, x, y);
		return;
	}

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLRBSCROLL) ) {
		m_RDownStat  = (m_SmoothTimer != 0 ? 3 : 1);
		m_RDownClock = clock();
		m_RDownPoint = point;
		m_RMovePoint = point;
		m_RDownOfs   = m_HisOfs;
		m_RMovePixel = 0;
		KillScrollTimer();
		return;
	}
	
	if ( pDoc->m_TextRam.IsOptEnable(TO_RLRSPAST) )
		OnEditPaste();
	else if ( !pDoc->m_TextRam.IsOptEnable(TO_RLRCLICK) )
		PopUpMenu(point);
	else {
		m_RDownClock = clock();
		m_RDownPoint = point;
		m_RclickTimer = SetTimer(VTMID_RCLICKCHECK, GetDoubleClickTime() * 3 / 2, NULL);
	}
}
void CRLoginView::OnRButtonUp(UINT nFlags, CPoint point)
{
	int x, y;
	CRLoginDoc *pDoc = GetDocument();

	m_LastMouseFlags = nFlags;

	CView::OnRButtonUp(nFlags, point);
	ReleaseCapture();

	if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
		CalcGrapPoint(point, &x, &y);
		pDoc->m_TextRam.MouseReport(MOS_LOCA_RTUP, nFlags, x, y);
		return;
	}

	if ( m_RDownStat == 2 ) {
		m_RDownStat = 0;

		y = point.y - m_RDownPoint.y;
		x = y % m_CharHeight;

		if ( (y = m_RDownOfs + y / m_CharHeight) < 0 ) {
			y = 0;
			x = 0;
		} else if ( y > (pDoc->m_TextRam.m_HisLen - m_Lines) ) {
			y = pDoc->m_TextRam.m_HisLen - m_Lines;
			x = 0;
		}

		if ( y != m_HisOfs || x != m_TopOffset ) {
			m_TopOffset = x;
			m_HisOfs = y;
			OnUpdate(NULL, UPDATE_INVALIDATE, NULL);
		}

		if ( (x = clock() - m_RDownClock) > 10 ) {
			m_RMovePixel = (point.y - m_RMovePoint.y) * 100 / x;
			m_RDownClock += x;
			m_RMovePoint = point;
		}

		if ( m_RMovePixel != 0 ) {
			m_SmoothTimer = SetTimer(VTMID_SMOOTHSCR, 100, NULL);

		} else if ( m_TopOffset != 0 ) {
			m_HisOfs += (m_TopOffset > 0 ? 1 : (-1));

			if ( m_HisOfs < 0 )
				m_HisOfs = 0;
			else if ( m_HisOfs > (pDoc->m_TextRam.m_HisLen - m_Lines) )
				m_HisOfs = pDoc->m_TextRam.m_HisLen - m_Lines;

			m_TopOffset = 0;
			OnUpdate(NULL, UPDATE_INVALIDATE, NULL);
		}

	} else if ( m_RDownStat == 1 ) {
		m_RDownStat = 0;

		if ( pDoc->m_TextRam.IsOptEnable(TO_RLRSPAST) )
			OnEditPaste();
		else if ( !pDoc->m_TextRam.IsOptEnable(TO_RLRCLICK) )
			PopUpMenu(point);
		else {
			m_RDownClock = clock();
			m_RDownPoint = point;
			m_RclickTimer = SetTimer(VTMID_RCLICKCHECK, GetDoubleClickTime() * 3 / 2, NULL);
		}

	} else {
		m_RDownStat = 0;
	}
}
void CRLoginView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	CRLoginDoc *pDoc = GetDocument();

	m_LastMouseFlags = nFlags;

	CView::OnRButtonDblClk(nFlags, point);

	if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag ) {
		int x, y;
		CalcGrapPoint(point, &x, &y);
		pDoc->m_TextRam.MouseReport(MOS_LOCA_RTDN, nFlags, x, y);
		return;
	}

	if ( m_RclickTimer != 0 ) {
		KillTimer(m_RclickTimer);
		m_RclickTimer = 0;
	}

	if ( !pDoc->m_TextRam.IsOptEnable(TO_RLRSPAST) && pDoc->m_TextRam.IsOptEnable(TO_RLRCLICK) )
		OnEditPaste();
}

BOOL CRLoginView::SendPasteText(LPCWSTR wstr)
{
	int ct = 0;
	int len = 0;
	LPCWSTR p;
	CBuffer tmp;
	BOOL rt = FALSE;
	CAnyPastDlg dlg;
	CRLoginDoc *pDoc = GetDocument();
	CStringW wrk;

	for ( p = wstr ; *p != 0 ; ) {
		if ( p[0] == L'\x0D' && p[1] == L'\x0A' ) {
			wrk += *(p++);
			wrk += *(p++);
			ct++;
		} else if ( p[0] == L'\x0A' && p[1] == L'\x0D' ) {
			wrk += L'\x0D';
			wrk += L'\x0A';
			p += 2;
			ct++;
		} else if ( p[0] == L'\x0D' ) {
			wrk += *(p++);
			wrk += L'\x0A';
			ct++;
		} else if ( p[0] == L'\x0A' ) {
			wrk += L'\x0D';
			wrk += *(p++);
			ct++;
		} else if ( p[0] == L'\x1A' && p[1] == L'\0' ) {
			p++;
		} else if ( p[0] < L' ' ) {
			wrk += *(p++);
			ct++;
		} else {
			wrk += *(p++);
			len++;
		}
	}

	if ( ct > 0 && pDoc->m_TextRam.IsOptEnable(TO_RLDELCRLF) && wrk.GetLength() >= 2 && wrk.Right(2).Compare(L"\x0D\x0A") == 0 ) {
		wrk.Delete(wrk.GetLength() - 2, 2);
		ct--;
	}

	wstr = wrk;

	if ( pDoc->m_TextRam.IsOptEnable(TO_RLEDITPAST) || (m_PastNoCheck == FALSE && (len > 500 || ct > 0)) ) {
		dlg.m_NoCheck     = m_PastNoCheck;
		dlg.m_EditText    = wstr;
		dlg.m_bDelayPast  = pDoc->m_TextRam.IsOptEnable(TO_RLDELYPAST) || m_PastDelaySend ? TRUE : FALSE;
		dlg.m_bUpdateText = pDoc->m_TextRam.IsOptEnable(TO_RLUPDPAST) ? TRUE : FALSE;
		dlg.m_bCtrlView   = m_PastCtrlView;
		dlg.m_pView       = this;

		if ( dlg.DoModal() != IDOK ) {
			m_PastDelaySend = dlg.m_bDelayPast;
			m_PastCtrlView  = dlg.m_bCtrlView;
			return FALSE;
		}

		m_PastNoCheck   = dlg.m_NoCheck;
		m_PastDelaySend = dlg.m_bDelayPast;
		m_PastCtrlView  = dlg.m_bCtrlView;

		wstr = TstrToUni(dlg.m_EditText);

		if ( dlg.m_bUpdateText != pDoc->m_TextRam.IsOptEnable(TO_RLUPDPAST) ) {
			pDoc->m_TextRam.SetOption(TO_RLUPDPAST,  dlg.m_bUpdateText);
			pDoc->SetModifiedFlag(TRUE);
		}

		if ( dlg.m_bUpdateText ) {
			((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(dlg.m_EditText);
			rt = TRUE;
		}
	}

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTBRPAMD) )
		tmp.Apend((LPBYTE)(L"\033[200~"), 6 * sizeof(WCHAR));

	for ( p = wstr ; *p != 0 ; p++ ) {
		if ( *p != L'\x0A' )
			tmp.Apend((LPBYTE)p, sizeof(WCHAR));
	}

	if ( pDoc->m_TextRam.IsOptEnable(TO_XTBRPAMD) )
		tmp.Apend((LPBYTE)(L"\033[201~"), 6 * sizeof(WCHAR));

	SendBuffer(tmp, FALSE, m_PastDelaySend);
	return rt;
}
void CRLoginView::OnEditPaste() 
{
	CString text;
	HDROP hData;
	CRLoginDoc *pDoc = GetDocument();

	if ( IsClipboardFormatAvailable(CF_HDROP) ) {
		if ( pDoc->m_TextRam.m_DropFileMode == 0 || pDoc->m_TextRam.m_DropFileCmd[pDoc->m_TextRam.m_DropFileMode].IsEmpty() )
			return;
		if ( MessageBox(CStringLoad(IDS_CLIPBOARDDROP), _T("Question"), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
		if ( !OpenClipboard() )
			return;
		if ( (hData = (HDROP)GetClipboardData(CF_HDROP)) != NULL )
			OnDropFiles(hData);
		CloseClipboard();

	} else if ( ((CMainFrame *)AfxGetMainWnd())->GetClipboardText(text) )
		SendPasteText(TstrToUni(text));
}
void CRLoginView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(IsClipboardFormatAvailable(CF_UNICODETEXT) || IsClipboardFormatAvailable(CF_HDROP) ? TRUE : FALSE);
}
void CRLoginView::ClipboardPaste(UINT nID)
{
	int n;
	int index = nID - IDM_CLIPBOARD_HIS1;
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();
	CRLoginDoc *pDoc = GetDocument();

	if ( pMain == NULL || index >= pMain->m_ClipBoard.GetSize() )
		return;

	if ( index > 0 ) {
		POSITION pos = pMain->m_ClipBoard.GetHeadPosition();
		for ( n = 0 ; n < index && pos != NULL ; n++ )
			pMain->m_ClipBoard.GetNext(pos);

		// Global Clipboard Update
		if ( pos != NULL ) {
			if ( !SendPasteText(pMain->m_ClipBoard.GetAt(pos)) )
				((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(pMain->m_ClipBoard.GetAt(pos));
		}

		// Local Clipboard Update
		//if ( pos != NULL ) {
		//	SendPasteText(pMain->m_ClipBoard.GetAt(pos));
		//	CStringW save = pMain->m_ClipBoard.GetAt(pos);
		//	pMain->m_ClipBoard.RemoveAt(pos);
		//	pMain->m_ClipBoard.AddHead(save);
		//}

	} else
		SendPasteText(pMain->m_ClipBoard.GetHead());
}
void CRLoginView::OnUpdateClipboardPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CMainFrame *)::AfxGetMainWnd())->m_ClipBoard.GetSize() > (int)(pCmdUI->m_nID - IDM_CLIPBOARD_HIS1) ? TRUE : FALSE);
}
void CRLoginView::OnClipboardMenu()
{
	CPoint point;
	CMenu *pMenu;
	CMenuLoad DefMenu;
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( pMain == NULL )
		return;

	if ( (pMenu = pMain->GetMenu()) == NULL ) {
		if ( !DefMenu.LoadMenu(IDR_RLOGINTYPE) )
			return;
		pMenu = &DefMenu;
	}
	
	if ( (pMenu = CMenuLoad::GetItemSubMenu(IDM_CLIPBOARD_HIS1, pMenu)) == NULL )
		return;

	pMain->SetClipBoardMenu(IDM_CLIPBOARD_HIS1, pMenu);

	point.x = m_CaretX;
	point.y = m_CaretY + m_CharHeight;
	ClientToScreen(&point);
	((CMainFrame *)::AfxGetMainWnd())->TrackPopupMenuIdle(pMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CRLoginView::OnMacroRec() 
{
#ifdef	USE_KEYMACGLOBAL
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	if ( m_KeyMacFlag ) {
		if ( m_KeyMacBuf.GetSize() > 0 ) {
			CKeyMac tmp;
			tmp.SetBuf(m_KeyMacBuf.GetPtr(), m_KeyMacBuf.GetSize());
			pApp->m_KeyMacGlobal.m_bUpdate = TRUE;
			pApp->m_KeyMacGlobal.Add(tmp);
			pApp->UpdateKeyMacGlobal();
		}
		m_KeyMacFlag = FALSE;
	} else {
		m_KeyMacFlag = TRUE;
		m_KeyMacSizeCheck = FALSE;
		m_KeyMacBuf.Clear();
	}
#else
	CRLoginDoc *pDoc = GetDocument();

	if ( m_KeyMacFlag ) {
		if ( m_KeyMacBuf.GetSize() > 0 ) {
			CKeyMac tmp;
			tmp.SetBuf(m_KeyMacBuf.GetPtr(), m_KeyMacBuf.GetSize());
			pDoc->m_KeyMac.Add(tmp);
			pDoc->SetModifiedFlag(TRUE);
		}
		m_KeyMacFlag = FALSE;
	} else {
		m_KeyMacFlag = TRUE;
		m_KeyMacSizeCheck = FALSE;
		m_KeyMacBuf.Clear();
	}
#endif
}
void CRLoginView::OnUpdateMacroRec(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_KeyMacFlag ? 1 : 0);
}
void CRLoginView::OnMacroPlay() 
{
#ifdef	USE_KEYMACGLOBAL
	CBuffer tmp;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	if ( pApp->m_KeyMacGlobal.m_Data.GetSize() <= 0 || !pApp->m_KeyMacGlobal.m_bUpdate )
		return;

	pApp->m_KeyMacGlobal.GetAt(0, tmp);
	pApp->UpdateKeyMacGlobal();

	SendBuffer(tmp, FALSE);
#else
	CBuffer tmp;
	CRLoginDoc *pDoc = GetDocument();

	if ( pDoc->m_KeyMac.m_Data.GetSize() <= 0 )
		return;

	pDoc->m_KeyMac.GetAt(0, tmp);

	SendBuffer(tmp, FALSE);
#endif
}
void CRLoginView::OnUpdateMacroPlay(CCmdUI* pCmdUI) 
{
#ifdef	USE_KEYMACGLOBAL
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	pCmdUI->Enable(!m_KeyMacFlag && pApp->m_KeyMacGlobal.m_bUpdate ? TRUE : FALSE);
#else
	CRLoginDoc *pDoc = GetDocument();
	pCmdUI->Enable(!m_KeyMacFlag && pDoc->m_KeyMac.GetSize() > 0 ? TRUE : FALSE);
#endif
}
void CRLoginView::OnMacroHis(UINT nID) 
{
#ifdef	USE_KEYMACGLOBAL
	CBuffer tmp;
	int n = nID - ID_MACRO_HIS1;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	if ( pApp->m_KeyMacGlobal.GetSize() <= n )
		return;

	pApp->m_KeyMacGlobal.m_bUpdate = TRUE;
	pApp->m_KeyMacGlobal.GetAt(n, tmp);
	pApp->m_KeyMacGlobal.Top(n);
	pApp->UpdateKeyMacGlobal();

	SendBuffer(tmp, FALSE);
#else
	CBuffer tmp;
	int n = nID - ID_MACRO_HIS1;
	CRLoginDoc *pDoc = GetDocument();

	if ( pDoc->m_KeyMac.GetSize() <= n )
		return;

	pDoc->m_KeyMac.GetAt(n, tmp);
	pDoc->m_KeyMac.Top(n);
	pDoc->SetModifiedFlag(TRUE);

	SendBuffer(tmp, FALSE);
#endif
}
void CRLoginView::OnUpdateMacroHis(CCmdUI* pCmdUI)
{
#ifdef	USE_KEYMACGLOBAL
	int n = pCmdUI->m_nID - ID_MACRO_HIS1;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	pCmdUI->Enable(n >= 0 && n < pApp->m_KeyMacGlobal.GetSize() ? TRUE : FALSE);
#else
	int n = nID - ID_MACRO_HIS1;
	CRLoginDoc *pDoc = GetDocument();
	pCmdUI->Enable(n >= 0 && n < pDoc->m_KeyMac.GetSize() ? TRUE : FALSE);
#endif
}

void CRLoginView::OnEditCopy() 
{
	if ( m_ClipFlag == 6 ) {
		CRLoginDoc *pDoc = GetDocument();
		pDoc->m_TextRam.EditCopy(m_ClipStaPos, m_ClipEndPos, IsClipRectMode(), IsClipLineMode());

		if ( !pDoc->m_TextRam.IsOptEnable(TO_RLSTAYCLIP) ) {
			m_ClipFlag = 0;
			OnUpdate(this, UPDATE_CLIPERA, NULL);
		}
	}
}
void CRLoginView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_ClipFlag == 6 ? TRUE : FALSE);
}

void CRLoginView::OnEditMark()
{
	if ( m_ClipFlag == 6 ) {
		CRLoginDoc *pDoc = GetDocument();
		pDoc->m_TextRam.EditMark(m_ClipStaPos, m_ClipEndPos, IsClipRectMode(), IsClipLineMode());

		m_ClipFlag = 0;
		OnUpdate(this, UPDATE_CLIPERA, NULL);
	}
}
void CRLoginView::OnUpdateEditMark(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_ClipFlag == 6 ? TRUE : FALSE);
}

void CRLoginView::OnEditCopyAll()
{
	CRLoginDoc *pDoc = GetDocument();

	m_ClipStaPos = pDoc->m_TextRam.GetCalcPos(0, 0 - pDoc->m_TextRam.m_HisLen + pDoc->m_TextRam.m_Lines);
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


void CRLoginView::OnImageGrapCopy()
{
	HANDLE hBitmap;

	if ( m_pSelectGrapWnd == NULL || (hBitmap = m_pSelectGrapWnd->GetBitmapHandle()) == NULL )
		return;

	if ( OpenClipboard() ) {
		if ( EmptyClipboard() )
			SetClipboardData(CF_BITMAP, hBitmap);
		else
			DeleteObject(hBitmap);

		CloseClipboard();

	} else
		DeleteObject(hBitmap);
}
void CRLoginView::OnImageGrapSave()
{
	if ( m_pSelectGrapWnd == NULL || m_pSelectGrapWnd->m_pActMap == NULL )
		return;

	m_pSelectGrapWnd->SaveBitmap(1);
}
void CRLoginView::OnImageGrapHistogram()
{
	if ( m_pSelectGrapWnd == NULL || m_pSelectGrapWnd->m_pActMap == NULL )
		return;

	m_pSelectGrapWnd->PostMessage(WM_COMMAND, IDM_HISTOGRAM);
}
void CRLoginView::OnUpdateHistogram(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pSelectGrapWnd != NULL && m_pSelectGrapWnd->m_pActMap != NULL && m_pSelectGrapWnd->m_pHistogram == NULL ? TRUE : FALSE);
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
			else if ( pMsg->wParam == VK_MENU && !IsZeroMemory(pDoc->m_TextRam.m_MetaKeys, sizeof(pDoc->m_TextRam.m_MetaKeys)) ) {
				pMsg->message = WM_KEYDOWN;
			}
		}
	} else
		m_ToolTip.RelayEvent(pMsg);

	return CView::PreTranslateMessage(pMsg);
}

BOOL CRLoginView::SetDropFile(LPCTSTR FileName, BOOL &doCmd, BOOL &doSub)
{
	CFileStatus st;
	CRLoginDoc *pDoc = GetDocument();

	if ( !CFile::GetStatus(FileName, st) || (st.m_attribute & CFile::directory) != 0 ) {
		CString str;
		BOOL DoLoop;
	    CFileFind Finder;

		if ( !doSub && MessageBox(CStringLoad(IDS_DROPSUBDIRCHECK), _T("Question"), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return FALSE;

		doSub = TRUE;
		str.Format(_T("%s\\*.*"), FileName);
		DoLoop = Finder.FindFile(str);
		while ( DoLoop != FALSE ) {
			DoLoop = Finder.FindNextFile();
			if ( Finder.IsDots() )
				continue;
			if ( !SetDropFile(Finder.GetFilePath(), doCmd, doSub) )
				return FALSE;
		}
		Finder.Close();

		return TRUE;
	}

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
	} else if ( pDoc->m_TextRam.m_DropFileMode == 7 ) {
		if ( pDoc->m_pFileUpDown == NULL )
			pDoc->m_pFileUpDown = new CFileUpDown(pDoc, AfxGetMainWnd());
		if ( pDoc->m_pFileUpDown->m_ResvPath.IsEmpty() && !pDoc->m_pFileUpDown->m_ThreadFlag )
			doCmd = TRUE;
		pDoc->m_pFileUpDown->m_ResvPath.AddTail(FileName);
	} else if ( pDoc->m_TextRam.m_DropFileMode >= 2 ) {
		if ( pDoc->m_pZModem == NULL )
			pDoc->m_pZModem = new CZModem(pDoc, AfxGetMainWnd());
		if ( pDoc->m_pZModem->m_ResvPath.IsEmpty() && !pDoc->m_pZModem->m_ThreadFlag )
			doCmd = TRUE;
		pDoc->m_pZModem->m_ResvPath.AddTail(FileName);
	}

	return TRUE;
}
void CRLoginView::OnDropFiles(HDROP hDropInfo)
{
    int i;
	TCHAR FileName[MAX_PATH * 2];
    int FileCount;
	CRLoginDoc *pDoc = GetDocument();
	BOOL doCmd = FALSE;
	BOOL doSub = FALSE;

	if ( pDoc->m_TextRam.m_DropFileMode == 0 )
		return;

    FileCount = DragQueryFile(hDropInfo, 0xffffffff, FileName, sizeof(FileName));

	for( i = 0 ; i < FileCount ; i++ ) {
		DragQueryFile(hDropInfo, i, FileName, sizeof(FileName));
		SetDropFile(FileName, doCmd, doSub);
	}

	if ( doCmd )
		pDoc->DoDropFile();
}

BOOL CRLoginView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ( pWnd == NULL || (pWnd == this && nHitTest == HTCLIENT) ) {
		int mode;
		CRLoginDoc *pDoc = GetDocument();
		HCURSOR hCursor = NULL;

		if ( ((CMainFrame *)::AfxGetMainWnd())->m_bBroadCast ) {

			mode = 0;

			if ( !pDoc->m_TextRam.IsOptEnable(TO_RLNTBCSEND) )
				mode |= 001;
			if ( !pDoc->m_TextRam.IsOptEnable(TO_RLNTBCRECV) )
				mode |= 002;

			switch(mode) {
			case 1:
				hCursor = AfxGetApp()->LoadStandardCursor(IDC_SIZENWSE);
				break;
			case 2:
				hCursor = AfxGetApp()->LoadStandardCursor(IDC_SIZENESW);
				break;
			case 3:
				hCursor = AfxGetApp()->LoadStandardCursor(IDC_SIZEALL);
				break;
			}
		}

#ifdef	USE_CLIENTKEY
		if ( hCursor == NULL ) {
			CPoint point;
			GetCursorPos(&point);
			ScreenToClient(&point);

			// クライアント座標でコマンド実行のテスト
			if ( HitTest(point) != 0 )
				hCursor = AfxGetApp()->LoadStandardCursor(IDC_HELP);
		}
#endif

		if ( hCursor == NULL ) {

			mode = pDoc->m_TextRam.m_XtMosPointMode;

			if ( pDoc->m_TextRam.m_MouseTrack != MOS_EVENT_NONE && !m_MouseEventFlag )
				mode |= 4;

			if ( pDoc->m_TextRam.IsOptEnable(TO_RLCURIMD) )
				mode |= 8;

			switch(mode) {	//	Xt	Ev	Rv
			case 0:			//	0	Off	Off
			case 4:			//	0	On	Off
			case 5:			//	1	On	Off
				hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
				break;

			case 8:			//	0	Off	On
			case 12:		//	0	On	On
			case 13:		//	1	On	On
				hCursor = AfxGetApp()->LoadStandardCursor(IDC_IBEAM);
				break;

			//case 1:		//	1	Off	Off
			//case 2:		//	2	Off	Off
			//case 3:		//	3	Off	Off
			//case 6:		//	2	On	Off
			//case 7:		//	3	On	Off
			//case 9:		//	1	Off	On
			//case 10:		//	2	Off	On
			//case 11:		//	3	Off	On
			//case 14:		//	2	On	On
			//case 15:		//	3	On	On
			//	break;
			}
		}

		if ( hCursor != NULL ) {
			::SetCursor(hCursor);
			return TRUE;
		}
	}

	if ( pWnd != NULL  )
		return CView::OnSetCursor(pWnd, nHitTest, message);
	else
		return FALSE;
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
		pDoc->m_TextRam.HisRegMark(NULL, FALSE);
		InvalidateFullText();
		return;
	}

	pDoc->m_SearchStr = dlg.m_SearchStr;

	if ( !pDoc->m_TextRam.HisMarkCheck(0 - m_HisOfs + m_HisMin, m_Lines, this) )
		OnSearchBack();

	InvalidateFullText();
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

#if USE_GOZI == 1 || USE_GOZI == 2
		SetTimer(VTMID_GOZIUPDATE, 400, NULL);
#elif USE_GOZI == 3
		m_GoziMax = 3 + (rand() % 6);
		SetTimer(VTMID_GOZIUPDATE, 200, NULL);
#elif USE_GOZI == 4
		m_GoziMax = 3 + (rand() % 6);
		for ( int n = 0 ; n < m_GoziMax ; n++ ) {
			m_GoziData[n].m_GoziStyle = (((rand() % 13) * 12) << 4) | 5;
			m_GoziData[n].m_GoziPos.SetPoint(0, 0);
		}
		SetTimer(VTMID_GOZIUPDATE, 200, NULL);
#endif

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

	pMain->SplitHeightPane();
	OnSplitOver();
}
void CRLoginView::OnSplitWidth()
{
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( pMain == NULL )
		return;

	pMain->SplitWidthPane();
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
	pDoc->m_ServerEntry.m_ReEntryFlag = TRUE;
	pApp->m_pServerEntry = &(pDoc->m_ServerEntry);
	cmds.ParseParam(_T("inpane"), TRUE, TRUE);
	pApp->OpenProcsCmd(&cmds);
}
void CRLoginView::OnSplitHeightNew()
{
	CCommandLineInfoEx cmds;
	CRLoginDoc *pDoc = GetDocument();
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( pMain == NULL )
		return;

	pDoc->m_InPane = TRUE;
	pMain->SplitHeightPane();
	cmds.ParseParam(_T("inpane"), TRUE, TRUE);
	pApp->OpenProcsCmd(&cmds);
}
void CRLoginView::OnSplitWidthNew()
{
	CCommandLineInfoEx cmds;
	CRLoginDoc *pDoc = GetDocument();
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();
	CMainFrame *pMain = (CMainFrame *)::AfxGetMainWnd();

	if ( pMain == NULL )
		return;

	pDoc->m_InPane = TRUE;
	pMain->SplitWidthPane();
	cmds.ParseParam(_T("inpane"), TRUE, TRUE);
	pApp->OpenProcsCmd(&cmds);
}
LRESULT CRLoginView::OnLogWrite(WPARAM wParam, LPARAM lParam)
{
	CRLoginDoc *pDoc = GetDocument();
	CBuffer *pBuffer = (CBuffer *)lParam;

	if ( wParam == 0 ) {
		pDoc->LogWrite(pBuffer->GetPtr(), pBuffer->GetSize(), LOGDEBUG_INSIDE);
		pDoc->LogWrite(NULL, 0, LOGDEBUG_FLASH);
	} else
		pDoc->LogDump(pBuffer->GetPtr(), pBuffer->GetSize());

	delete pBuffer;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRLoginView クラスの印刷

BOOL CRLoginView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// デフォルトの印刷準備
	pInfo->SetMaxPage(1);
	return DoPreparePrinting(pInfo);
}
void CRLoginView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	CView::OnBeginPrinting(pDC, pInfo);
}
void CRLoginView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	CView::OnEndPrinting(pDC, pInfo);
}
void CRLoginView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	int n;
	CRect rect, box;
	CRLoginDoc* pDoc = GetDocument();
	CFont font, *pOldFont;
	CTime tm = CTime::GetCurrentTime();
	CString str;
	CSize sz;
	int save_param[11];

	if ( !pDoc->m_TextRam.IsInitText() )
		return;

	save_param[0] = m_Width;
	save_param[1] = m_Height;

	save_param[2] = m_CharWidth;
	save_param[3] = m_CharHeight;

	save_param[4] = pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.fcol];
	save_param[5] = pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.bcol];

	save_param[6] = pDoc->m_TextRam.m_ScrnOffset.left;
	save_param[7] = pDoc->m_TextRam.m_ScrnOffset.right;
	save_param[8] = pDoc->m_TextRam.m_ScrnOffset.top;
	save_param[9] = pDoc->m_TextRam.m_ScrnOffset.bottom;

	save_param[10] = m_TopOffset;

	rect.left   = 100 * pDC->GetDeviceCaps(LOGPIXELSX) / 254;		// 左右印刷マージン 10mm
	rect.top    = 100 * pDC->GetDeviceCaps(LOGPIXELSY) / 254;		// 上下印刷マージン 10mm
	rect.right  = pDC->GetDeviceCaps(HORZRES) - rect.left;
	rect.bottom = pDC->GetDeviceCaps(VERTRES) - rect.top;

	box = rect;

	if ( box.Height() > (n = m_Height * box.Width() / m_Width) ) {
		box.top += (box.Height() - n) / 2;
		box.bottom = box.top + n;
	} else {
		n = m_Width * box.Height() / m_Height;
		box.left += (box.Width() - n) / 2;
		box.right = box.left + n;
	}

	m_Width  = box.Width();
	m_Height = box.Height();

	m_CharWidth  = m_Width  / m_Cols;
	m_CharHeight = m_Height / m_Lines;
	m_TopOffset = 0;

	pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.fcol] = RGB(0, 0, 0);
	pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.bcol] = RGB(255, 255, 255);

	pDoc->m_TextRam.m_ScrnOffset.SetRect(box.left, box.top, 0, 0);

	pDoc->m_TextRam.DrawVram(pDC, 0, 0, m_Cols, m_Lines, this, TRUE);

	font.CreateFont(72, 0, 0, 0, 0, FALSE, 0, 0, ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));
	pOldFont = pDC->SelectObject(&font);

	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->SetBkColor(RGB(255, 255, 255));

	str = tm.Format(_T("%c"));
	sz = pDC->GetTextExtent(str);
	pDC->TextOut(rect.right - sz.cx, rect.bottom + sz.cy, str);

	str = pDoc->m_ServerEntry.m_EntryName;
	sz = pDC->GetTextExtent(str);
	pDC->TextOut(rect.left, rect.bottom + sz.cy, str);

	pDC->SelectObject(pOldFont);

	m_Width      = save_param[0];
	m_Height     = save_param[1];

	m_CharWidth  = save_param[2];
	m_CharHeight = save_param[3];

	pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.fcol] = save_param[4];
	pDoc->m_TextRam.m_ColTab[pDoc->m_TextRam.m_DefAtt.std.bcol] = save_param[5];

	pDoc->m_TextRam.m_ScrnOffset.left   = save_param[6];
	pDoc->m_TextRam.m_ScrnOffset.right  = save_param[7];
	pDoc->m_TextRam.m_ScrnOffset.top    = save_param[8];
	pDoc->m_TextRam.m_ScrnOffset.bottom = save_param[9];

	m_TopOffset = save_param[10];
}

