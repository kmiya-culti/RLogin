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
#include "ssh2.h"
#include "ToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
	m_BoderSize = 2;
	m_bActive = FALSE;
	m_pServerEntry = NULL;
	m_bReqSize = FALSE;

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

	if ( m_pLeft->m_Frame.Width() < PANEMIN_WIDTH || m_pLeft->m_Frame.Height() < PANEMIN_HEIGHT || m_pRight->m_Frame.Width() < PANEMIN_WIDTH || m_pRight->m_Frame.Height() < PANEMIN_HEIGHT ) {
		m_Style = PANEFRAME_MAXIM;
		m_pLeft->m_Frame  = m_Frame;
		m_pRight->m_Frame = m_Frame;
	}

	m_pLeft->MoveFrame();
	m_pRight->MoveFrame();
}
CPaneFrame *CPaneFrame::InsertPane()
{
	CPaneFrame *pPane = new CPaneFrame(m_pMain, NULL, m_pOwn);

	pPane->m_Style  = PANEFRAME_MAXIM;
	pPane->m_Frame  = m_Frame;
	pPane->m_pLeft  = this;
	pPane->m_pRight = new CPaneFrame(m_pMain, NULL, pPane);

	if ( m_pOwn != NULL ) {
		if ( m_pOwn->m_pLeft == this )
			m_pOwn->m_pLeft = pPane;
		else if ( m_pOwn->m_pRight == this )
			m_pOwn->m_pRight = pPane;
	}
	m_pOwn = pPane;

	return pPane;
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

	ASSERT(m_pLeft != NULL && m_pRight != NULL);

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
BOOL CPaneFrame::IsTopLevel(CPaneFrame *pPane)
{
	CRect rect;

	if ( m_Style == PANEFRAME_WINDOW ) {
		if ( pPane->m_hWnd != NULL && m_hWnd != NULL && pPane->m_hWnd != m_hWnd && rect.IntersectRect(m_Frame, pPane->m_Frame) ) {
			HWND hWnd = pPane->m_hWnd;
			while ( (hWnd = ::GetWindow(hWnd, GW_HWNDPREV)) != NULL ) {
				if ( hWnd == m_hWnd )
					return FALSE;
			}
		}
		return TRUE;
	} else if ( m_pLeft != NULL && !m_pLeft->IsTopLevel(pPane) )
		return FALSE;
	else if ( m_pRight != NULL && !m_pRight->IsTopLevel(pPane) )
		return FALSE;
	else
		return TRUE;
}
int CPaneFrame::GetPaneCount(int count)
{
	if ( m_Style == PANEFRAME_WINDOW )
		count++;

	if ( m_pLeft != NULL )
		count = m_pLeft->GetPaneCount(count);

	if ( m_pRight != NULL )
		count = m_pRight->GetPaneCount(count);

	return count;
}
void CPaneFrame::MoveFrame()
{
	ASSERT(m_Style == PANEFRAME_WINDOW);

	if ( m_hWnd != NULL ) {
		if ( m_NullWnd.m_hWnd != NULL )
			m_NullWnd.DestroyWindow();
		::SetWindowPos(m_hWnd, NULL, m_Frame.left - 2, m_Frame.top - 2, m_Frame.Width(), m_Frame.Height(),
			SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS | SWP_DEFERERASE);
	} else {
		CRect rect = m_Frame;
		m_pMain->AdjustRect(rect);
		if ( m_NullWnd.m_hWnd == NULL )
			m_NullWnd.Create(NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | (m_bActive ? SS_WHITEFRAME : SS_GRAYFRAME), rect, m_pMain);
		else {
			m_NullWnd.ModifyStyle(SS_WHITEFRAME | SS_GRAYFRAME, (m_bActive ? SS_WHITEFRAME : SS_GRAYFRAME), 0);
			m_NullWnd.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(),
				SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_ASYNCWINDOWPOS);
		}
	}
}
void CPaneFrame::GetNextPane(int mode, class CPaneFrame *pThis, class CPaneFrame **ppPane)
{
	if ( m_Style == PANEFRAME_WINDOW ) {
		if ( m_hWnd != NULL && m_hWnd != pThis->m_hWnd ) {
			switch(mode) {
			case 0:	// Up
				if (  m_Frame.bottom > pThis->m_Frame.top )
					break;
				if ( *ppPane == NULL )
					*ppPane = this;
				else if ( m_Frame.bottom > (*ppPane)->m_Frame.bottom )
					*ppPane = this;
				else if ( m_Frame.bottom == (*ppPane)->m_Frame.bottom && abs(m_Frame.left - pThis->m_Frame.left) < abs((*ppPane)->m_Frame.left - pThis->m_Frame.left) )
					*ppPane = this;
				break;
			case 1:	// Down
				if (  m_Frame.top < pThis->m_Frame.bottom )
					break;
				if ( *ppPane == NULL )
					*ppPane = this;
				else if ( m_Frame.top < (*ppPane)->m_Frame.top )
					*ppPane = this;
				else if ( m_Frame.top == (*ppPane)->m_Frame.top && abs(m_Frame.left - pThis->m_Frame.left) < abs((*ppPane)->m_Frame.left - pThis->m_Frame.left) )
					*ppPane = this;
				break;
			case 2:	// Right
				if (  m_Frame.left < pThis->m_Frame.right )
					break;
				if ( *ppPane == NULL )
					*ppPane = this;
				else if ( m_Frame.left < (*ppPane)->m_Frame.left )
					*ppPane = this;
				else if ( m_Frame.left == (*ppPane)->m_Frame.left && abs(m_Frame.top - pThis->m_Frame.top) < abs((*ppPane)->m_Frame.top - pThis->m_Frame.top) )
					*ppPane = this;
				break;
			case 3:	// Left
				if (  m_Frame.right > pThis->m_Frame.left )
					break;
				if ( *ppPane == NULL )
					*ppPane = this;
				else if ( m_Frame.right > (*ppPane)->m_Frame.right )
					*ppPane = this;
				else if ( m_Frame.right == (*ppPane)->m_Frame.right && abs(m_Frame.top - pThis->m_Frame.top) < abs((*ppPane)->m_Frame.top - pThis->m_Frame.top) )
					*ppPane = this;
				break;
			}
		}
	} else {
		m_pLeft->GetNextPane(mode, pThis, ppPane);
		m_pRight->GetNextPane(mode, pThis, ppPane);
	}
}
BOOL CPaneFrame::IsReqSize()
{
	if ( m_Style == PANEFRAME_WINDOW )
		return m_bReqSize;
	else if ( m_pLeft->IsReqSize() )
		return TRUE;
	else if ( m_pRight->IsReqSize() )
		return TRUE;
	else
		return FALSE;
}
void CPaneFrame::MoveParOwn(CRect &rect, int Style)
{
	int n, l, r;

	if ( m_Style == PANEFRAME_WINDOW ) {
		m_pMain->InvalidateRect(m_Frame);
		m_Frame = rect;
		MoveFrame();
	} else {
		ASSERT(m_pLeft != NULL && m_pRight != NULL);

		CRect left  = rect;
		CRect right = rect;
	
		switch(Style) {
		case PANEFRAME_NOCHNG:
			if ( m_pLeft->IsReqSize() ) {
				if ( m_Style == PANEFRAME_WIDTH ) {
					left.right = left.left + rect.Width() - m_pRight->m_Frame.Width() - m_BoderSize;
					right.left = left.right + m_BoderSize;
				} else if ( m_Style == PANEFRAME_HEIGHT ) {
					left.bottom = left.top + rect.Height() - m_pRight->m_Frame.Height() - m_BoderSize;
					right.top   = left.bottom + m_BoderSize;
				}
				m_pLeft->m_bReqSize = FALSE;
				break;
			} else if ( m_pRight->IsReqSize() ) {
				if ( m_Style == PANEFRAME_WIDTH ) {
					left.right = left.left + m_pLeft->m_Frame.Width();
					right.left = left.right + m_BoderSize;
				} else if ( m_Style == PANEFRAME_HEIGHT ) {
					left.bottom = left.top + m_pLeft->m_Frame.Height();
					right.top   = left.bottom + m_BoderSize;
				}
				break;
			}
		RECALC:
			if ( m_Style == PANEFRAME_WIDTH ) {
				n = m_pLeft->m_Frame.Width() + m_pRight->m_Frame.Width() + m_BoderSize;

				if ( m_pLeft->m_Frame.Width() > m_pRight->m_Frame.Width() ) {
					left.right = left.left + m_pLeft->m_Frame.Width() * rect.Width() / n;
					right.left = left.right + m_BoderSize;
				} else {
					right.left = rect.right - m_pRight->m_Frame.Width() * rect.Width() / n;
					left.right = right.left - m_BoderSize;
				}
			} else if ( m_Style == PANEFRAME_HEIGHT ) {
				n = m_pLeft->m_Frame.Height() + m_pRight->m_Frame.Height() + m_BoderSize;

				if ( m_pLeft->m_Frame.Height() > m_pRight->m_Frame.Height() ) {
					left.bottom = left.top + m_pLeft->m_Frame.Height() * rect.Height() / n;
					right.top   = left.bottom + m_BoderSize;
				} else {
					right.top   = rect.bottom - m_pRight->m_Frame.Height() * rect.Height() / n;
					left.bottom = right.top - m_BoderSize;
				}
			}
			break;

		case PANEFRAME_MAXIM:
			m_Style = Style;
			break;

		case PANEFRAME_WIDTH:
			if ( m_Style != PANEFRAME_MAXIM )
				goto RECALC;
			m_Style = Style;
			left.right = left.left  + (rect.Width() - m_BoderSize) / 2;
			right.left = left.right + m_BoderSize;
			Style = PANEFRAME_NOCHNG;
			break;

		case PANEFRAME_HEIGHT:
			if ( m_Style != PANEFRAME_MAXIM )
				goto RECALC;
			m_Style = Style;
			left.bottom = left.top    + (rect.Height() - m_BoderSize) / 2;
			right.top   = left.bottom + m_BoderSize;
			Style = PANEFRAME_NOCHNG;
			break;

		case PANEFRAME_WSPLIT:
			m_Style = PANEFRAME_WIDTH;
			left.right = left.left  + (rect.Width() - m_BoderSize) / 2;
			right.left = left.right + m_BoderSize;
			Style = PANEFRAME_HSPLIT;
			break;

		case PANEFRAME_HSPLIT:
			m_Style = PANEFRAME_HEIGHT;
			left.bottom = left.top    + (rect.Height() - m_BoderSize) / 2;
			right.top   = left.bottom + m_BoderSize;
			Style = PANEFRAME_WSPLIT;
			break;

		case PANEFRAME_WEVEN:
			l = m_pLeft->GetPaneCount(0);
			r = m_pRight->GetPaneCount(0);
			if ( (l + r) == 0 ) {
				m_Style = PANEFRAME_MAXIM;
				break;
			}
			m_Style = PANEFRAME_WIDTH;
			left.right = left.left + rect.Width() * l / (l + r) - m_BoderSize / 2;
			right.left = left.right + m_BoderSize;
			break;

		case PANEFRAME_HEVEN:
			l = m_pLeft->GetPaneCount(0);
			r = m_pRight->GetPaneCount(0);
			if ( (l + r) == 0 ) {
				m_Style = PANEFRAME_MAXIM;
				break;
			}
			m_Style = PANEFRAME_HEIGHT;
			left.bottom = left.top + rect.Height() * l / (l + r) - m_BoderSize / 2;
			right.top   = left.bottom + m_BoderSize;
			break;
		}

		if ( left.Width() < PANEMIN_WIDTH || left.Height() < PANEMIN_HEIGHT || right.Width() < PANEMIN_WIDTH || right.Height() < PANEMIN_HEIGHT ) {
			Style = m_Style = PANEFRAME_MAXIM;
			left  = rect;
			right = rect;
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
		if ( m_Frame.PtInRect(po) && po.x >= (m_pLeft->m_Frame.right - 2) && po.x <= (m_pRight->m_Frame.left + 2) )
			return this;
		break;
	case PANEFRAME_HEIGHT:
		if ( m_Frame.PtInRect(po) && po.y >= (m_pLeft->m_Frame.bottom - 2) && po.y <= (m_pRight->m_Frame.top + 2) )
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
void CPaneFrame::SetBuffer(CBuffer *buf, BOOL bEntry)
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
		if ( bEntry && m_hWnd != NULL ) {
			CChildFrame *pWnd = (CChildFrame *)(CWnd::FromHandlePermanent(m_hWnd));
			if ( pWnd != NULL ) {
				CRLoginDoc *pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument());
				if ( pDoc != NULL ) {
					// 現在のタイトルを保存
					if ( !pDoc->m_TitleName.IsEmpty() )
						pDoc->m_TextRam.m_TitleName = pDoc->m_TitleName;
					pDoc->SetEntryProBuffer();
					pEntry = &(pDoc->m_ServerEntry);
					pEntry->m_DocType = DOCTYPE_MULTIFILE;
					pDoc->SetModifiedFlag(FALSE);
#if	_MSC_VER >= _MSC_VER_VS10
					pDoc->ClearPathName();
#endif
					tmp.Format("%d\t0\t1\t", m_Style);
				}
			}
		}
		buf->PutStr(tmp);
		if ( pEntry != NULL )
			pEntry->SetBuffer(*buf);
		return;
	}

	ASSERT(m_pLeft != NULL && m_pRight != NULL);

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

	if ( pPane->m_Style < PANEFRAME_MAXIM || pPane->m_Style > PANEFRAME_WINDOW ) {
		delete pPane;
		return NULL;
	}

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
		if ( pPane->m_pLeft->m_Frame.Width() < PANEMIN_WIDTH || pPane->m_pRight->m_Frame.Width() < PANEMIN_WIDTH ) {
			pPane->m_pLeft->m_Frame = pPane->m_Frame;
			pPane->m_pRight->m_Frame = pPane->m_Frame;
			pPane->m_Style = PANEFRAME_MAXIM;
		}
		break;
	case PANEFRAME_HEIGHT:
		pPane->m_pLeft->m_Frame.bottom = pPane->m_Frame.Height() * Size / 1000;
		pPane->m_pRight->m_Frame.top   = pPane->m_pLeft->m_Frame.bottom + pPane->m_BoderSize;
		if ( pPane->m_pLeft->m_Frame.Height() < PANEMIN_HEIGHT || pPane->m_pRight->m_Frame.Height() < PANEMIN_HEIGHT ) {
			pPane->m_pLeft->m_Frame = pPane->m_Frame;
			pPane->m_pRight->m_Frame = pPane->m_Frame;
			pPane->m_Style = PANEFRAME_MAXIM;
		}
		break;
	}

	pPane->m_pLeft  = GetBuffer(pMain, pPane->m_pLeft,  pPane, buf);
	pPane->m_pRight = GetBuffer(pMain, pPane->m_pRight, pPane, buf);

	if ( pPane->m_pLeft == NULL || pPane->m_pRight == NULL ) {
		delete pPane;
		return NULL;
	}

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
	ASSERT(m_pObject != NULL);

	switch(m_Mode & 007) {
	case TIMEREVENT_DOC:
		((CRLoginDoc *)(m_pObject))->OnDelayReceive((-1));
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
	ON_WM_ENTERIDLE()
	ON_WM_SYSCOMMAND()
	ON_WM_TIMER()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_COPYDATA()
	ON_WM_ENTERMENULOOP()
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DRAWCLIPBOARD()
	ON_WM_CHANGECBCHAIN()
	ON_WM_CLIPBOARDUPDATE()
	ON_WM_MOVING()
	ON_WM_GETMINMAXINFO()

	ON_MESSAGE(WM_SOCKSEL, OnWinSockSelect)
	ON_MESSAGE(WM_GETHOSTADDR, OnGetHostAddr)
	ON_MESSAGE(WM_ICONMSG, OnIConMsg)
	ON_MESSAGE(WM_THREADCMD, OnThreadMsg)
	ON_MESSAGE(WM_AFTEROPEN, OnAfterOpen)
	ON_MESSAGE(WM_GETCLIPBOARD, OnGetClipboard)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)

	ON_COMMAND(ID_FILE_ALL_LOAD, OnFileAllLoad)
	ON_COMMAND(ID_FILE_ALL_SAVE, OnFileAllSave)

	ON_COMMAND(ID_PANE_WSPLIT, OnPaneWsplit)
	ON_COMMAND(ID_PANE_HSPLIT, OnPaneHsplit)
	ON_COMMAND(ID_PANE_DELETE, OnPaneDelete)
	ON_COMMAND(ID_PANE_SAVE, OnPaneSave)

	ON_COMMAND(ID_VIEW_MENUBAR, &CMainFrame::OnViewMenubar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MENUBAR, &CMainFrame::OnUpdateViewMenubar)
	ON_COMMAND(ID_VIEW_TABBAR, &CMainFrame::OnViewTabbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TABBAR, &CMainFrame::OnUpdateViewTabbar)
	ON_COMMAND(ID_VIEW_SCROLLBAR, &CMainFrame::OnViewScrollbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SCROLLBAR, &CMainFrame::OnUpdateViewScrollbar)

	ON_COMMAND(ID_WINDOW_CASCADE, OnWindowCascade)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_CASCADE, OnUpdateWindowCascade)
	ON_COMMAND(ID_WINDOW_TILE_HORZ, OnWindowTileHorz)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_TILE_HORZ, OnUpdateWindowTileHorz)

	ON_COMMAND(ID_WINDOW_ROTATION, &CMainFrame::OnWindowRotation)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_ROTATION, &CMainFrame::OnUpdateWindowRotation)

	ON_COMMAND(IDM_WINDOW_PREV, &CMainFrame::OnWindowPrev)
	ON_UPDATE_COMMAND_UI(IDM_WINDOW_PREV, &CMainFrame::OnUpdateWindowPrev)
	ON_COMMAND(IDM_WINODW_NEXT, &CMainFrame::OnWinodwNext)
	ON_UPDATE_COMMAND_UI(IDM_WINODW_NEXT, &CMainFrame::OnUpdateWinodwNext)

	ON_COMMAND_RANGE(AFX_IDM_FIRST_MDICHILD, AFX_IDM_FIRST_MDICHILD + 255, &CMainFrame::OnWinodwSelect)
	ON_COMMAND_RANGE(IDM_MOVEPANE_UP, IDM_MOVEPANE_LEFT, &CMainFrame::OnActiveMove)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_MOVEPANE_UP, IDM_MOVEPANE_LEFT, &CMainFrame::OnUpdateActiveMove)

	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SOCK, OnUpdateIndicatorSock)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_STAT, OnUpdateIndicatorStat)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_KMOD, OnUpdateIndicatorKmod)
		
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)

	ON_COMMAND(IDM_VERSIONCHECK, &CMainFrame::OnVersioncheck)
	ON_UPDATE_COMMAND_UI(IDM_VERSIONCHECK, &CMainFrame::OnUpdateVersioncheck)
	ON_COMMAND(IDM_NEWVERSIONFOUND, &CMainFrame::OnNewVersionFound)

	ON_COMMAND(IDM_BROADCAST, &CMainFrame::OnBroadcast)
	ON_UPDATE_COMMAND_UI(IDM_BROADCAST, &CMainFrame::OnUpdateBroadcast)
	
	ON_COMMAND(IDM_TOOLCUST, &CMainFrame::OnToolcust)
	ON_COMMAND(IDM_CLIPCHAIN, &CMainFrame::OnClipchain)
	ON_UPDATE_COMMAND_UI(IDM_CLIPCHAIN, &CMainFrame::OnUpdateClipchain)
	ON_COMMAND(IDM_DELOLDENTRYTAB, OnDeleteOldEntry)

END_MESSAGE_MAP()

static const UINT indicators[] =
{
	ID_SEPARATOR,           // ステータス ライン インジケータ
	ID_INDICATOR_SOCK,
	ID_INDICATOR_STAT,
	ID_INDICATOR_KMOD,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
//	ID_INDICATOR_SCRL,
//	ID_INDICATOR_KANA,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame コンストラクション/デストラクション

CMainFrame::CMainFrame()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIconActive = AfxGetApp()->LoadIcon(IDI_ACTIVE);
	m_IconShow = FALSE;
	m_pTopPane = NULL;
	m_Frame.SetRectEmpty();
	m_pTrackPane = NULL;
	m_LastPaneFlag = FALSE;
	m_TimerSeqId = TIMERID_TIMEREVENT;
	m_pTimerUsedId = NULL;
	m_pTimerFreeId = NULL;
	m_SleepStatus = 0;
	m_SleepTimer = 0;
	m_TransParValue = 255;
	m_TransParColor = RGB(0, 0, 0);
	m_SleepCount = 60;
	m_hMidiOut = NULL;
	m_MidiTimer = 0;
	m_InfoThreadCount = 0;
	m_SplitType = PANEFRAME_WSPLIT;
	m_StartMenuHand = NULL;
	m_bVersionCheck = FALSE;
	m_hNextClipWnd = NULL;
	m_bBroadCast = FALSE;
	m_bTabBarShow = FALSE;
	m_StatusTimer = 0;
	m_bAllowClipChain = TRUE;
	m_bClipEnable = FALSE;
	m_bClipChain = FALSE;
	m_ScreenDpiX = m_ScreenDpiY = 96;
	m_bGlassStyle = FALSE;
	m_UseBitmapUpdate = FALSE;
	m_bClipThreadCount = 0;
	m_ClipTimer = 0;
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

	for ( int n = 0 ; m_InfoThreadCount > 0 && n < 10 ; n++ )
		Sleep(300);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int n;
	CBitmap BitMap;
	CBuffer buf;
	CMenu *pMenu;
	UINT nID, nSt;

	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// キャラクタービットマップの読み込み
#if		USE_GOZI == 1 || USE_GOZI == 2
	((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(IDB_BITMAP8), BitMap);
	m_ImageGozi.Create(32, 32, ILC_COLOR24 | ILC_MASK, 28, 10);
	m_ImageGozi.Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();
#elif	USE_GOZI == 3
	((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(IDB_BITMAP8), BitMap);
	m_ImageGozi.Create(16, 16, ILC_COLOR24 | ILC_MASK, 12, 10);
	m_ImageGozi.Add(&BitMap, RGB(255, 255, 255));
	BitMap.DeleteObject();
#elif	USE_GOZI == 4
	m_ImageGozi.Create(16, 16, ILC_COLOR24 | ILC_MASK, 12 * 13, 12);
	for ( n = 0 ; n < 13 ; n++ ) {
		((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(IDB_BITMAP10 + n), BitMap);
		m_ImageGozi.Add(&BitMap, RGB(255, 255, 255));
		BitMap.DeleteObject();
	}
#endif	// USE_GOZI

	// メニュー画像を作成
	InitMenuBitmap();

	// ツール・ステータス・タブ　バーの作成
	if ( !m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT,
			WS_CHILD | WS_VISIBLE | CBRS_TOP | /*CBRS_GRIPPER | */CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_MAINFRAME), m_wndToolBar) ) {
		TRACE0("Failed to create toolbar\n");
		return -1;      // 作成に失敗
	}

	if ( !m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)) ) {
		TRACE0("Failed to create status bar\n");
		return -1;      // 作成に失敗
	}

	if ( !m_wndTabBar.Create(this, WS_VISIBLE| WS_CHILD|CBRS_TOP|WS_EX_WINDOWEDGE, IDC_MDI_TAB_CTRL_BAR) ) {
		TRACE("Failed to create tabbar\n");
		return -1;      // fail to create
	}

	m_wndStatusBar.GetPaneInfo(0, nID, nSt, n);
	m_wndStatusBar.SetPaneInfo(0, nID, nSt, 160);

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndTabBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	DockControlBar(&m_wndTabBar);

	// バーの表示設定
	if ( (AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("ToolBarStyle"), WS_VISIBLE) & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndToolBar, FALSE, 0);
	if ( (AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("StatusBarStyle"), WS_VISIBLE) & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndStatusBar, FALSE, 0);

	m_bTabBarShow = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("TabBarShow"), FALSE);
	ShowControlBar(&m_wndTabBar, m_bTabBarShow, 0);

	// 特殊効果の設定
	m_TransParValue = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("LayeredWindow"), 255);
	m_TransParColor = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("LayeredColor"), RGB(0, 0 ,0));
	SetTransPar(m_TransParColor, m_TransParValue, LWA_ALPHA | LWA_COLORKEY);

	if ( (m_SleepCount = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("WakeUpSleep"), 0)) > 0 )
		m_SleepTimer = SetTimer(TIMERID_SLEEPMODE, 5000, NULL);

	m_ScrollBarFlag = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("VScroll"), TRUE);
	m_bVersionCheck = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("VersionCheckFlag"), FALSE);

	m_bGlassStyle = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("GlassStyle"), FALSE);
	ExDwmEnableWindow(m_hWnd, m_bGlassStyle);

	// 画面分割を復帰
	try {
		((CRLoginApp *)AfxGetApp())->GetProfileBuffer(_T("MainFrame"), _T("Pane"), buf);
		m_pTopPane = CPaneFrame::GetBuffer(this, NULL, NULL, &buf);
	} catch(LPCTSTR pMsg) {
		CString tmp;
		tmp.Format(_T("Pane Init Error '%s'"), pMsg);
		::AfxMessageBox(tmp);
	} catch(...) {
		CString tmp;
		tmp.Format(_T("Pane Init Error #%d"), ::GetLastError());
		::AfxMessageBox(tmp);
	}

	// メニューの初期化
	if ( (pMenu = GetSystemMenu(FALSE)) != NULL ) {
		pMenu->InsertMenu(0, MF_BYPOSITION | MF_SEPARATOR);
		pMenu->InsertMenu(0, MF_BYPOSITION | MF_STRING, ID_VIEW_MENUBAR, CStringLoad(IDS_VIEW_MENUBAR));
	}

	if ( (pMenu = GetMenu()) != NULL ) {
		m_StartMenuHand = pMenu->GetSafeHmenu();
		SetMenuBitmap(pMenu);
	}

	// クリップボードチェインの設定
	if ( (m_bAllowClipChain = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("ClipboardChain"), TRUE)) ) {
		if ( ExAddClipboardFormatListener != NULL && ExRemoveClipboardFormatListener != NULL ) {
			if ( ExAddClipboardFormatListener(m_hWnd) )
				PostMessage(WM_GETCLIPBOARD);
		} else {
			m_bClipChain = TRUE;
			m_hNextClipWnd = SetClipboardViewer();
			m_ClipTimer = SetTimer(TIMERID_CLIPUPDATE, 60000, NULL);
		}
	}

	// 標準の設定のキー設定を読み込み・初期化
	m_DefKeyTab.Serialize(FALSE);
	m_DefKeyTab.CmdsInit();

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	int n = GetExecCount();
	CString sect;

	if ( n == 0 ) {
		cs.x  = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("x"), cs.x);
		cs.y  = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("y"), cs.y);
		cs.cx = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("cx"), cs.cx);
		cs.cy = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("cy"), cs.cy);

	} else {
		sect.Format(_T("SecondFrame%02d"), n);

		if ( (n = AfxGetApp()->GetProfileInt(sect, _T("x"), (-1))) != (-1) )
			cs.x = n;

		if ( (n = AfxGetApp()->GetProfileInt(sect, _T("y"), (-1))) != (-1) )
			cs.y = n;

		if ( (n = AfxGetApp()->GetProfileInt(sect, _T("cx"), (-1))) != (-1) )
			cs.cx = n;
		else
			cs.cx = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("cx"), cs.cx);

		if ( (n = AfxGetApp()->GetProfileInt(sect, _T("cy"), (-1))) != (-1) )
			cs.cy = n;
		else
			cs.cy = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("cy"), cs.cy);
	}

	if (  m_ScreenW > 0 )
		cs.cx = m_ScreenW;
	if (  m_ScreenH > 0 )
		cs.cy = m_ScreenH;
	if ( m_ScreenX > 0 )
		cs.x = m_ScreenX - cs.cx / 2;
	if (  m_ScreenY > 0 )
		cs.y = m_ScreenY;

	// モニターの表示範囲チェック
	HMONITOR hMonitor;
    MONITORINFOEX  mi;
	CRect rc(cs.x, cs.y, cs.x + cs.cx, cs.y + cs.cy);

	hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &mi);

#if 1
	// モニターを基準に調整
	if ( cs.x < mi.rcMonitor.left )
		cs.x = mi.rcMonitor.left;

	if ( cs.y < mi.rcMonitor.top )
		cs.y = mi.rcMonitor.top;

	if ( (cs.x + cs.cx) > mi.rcMonitor.right ) {
		if ( (cs.x = mi.rcMonitor.right - cs.cx) < mi.rcMonitor.left ) {
			cs.x  = mi.rcMonitor.left;
			cs.cx = mi.rcMonitor.right - mi.rcMonitor.left;
		}
	}

	if ( (cs.y + cs.cy) > mi.rcMonitor.bottom ) {
		if ( (cs.y = mi.rcMonitor.bottom - cs.cy) < mi.rcMonitor.top ) {
			cs.y  = mi.rcMonitor.top;
			cs.cy = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}
	}
#else
	// 仮想画面サイズを基準に調整
	if ( (cs.x + cs.cx) > mi.rcWork.right ) {
		if ( (cs.x = mi.rcWork.right - cs.cx) < mi.rcWork.left ) {
			cs.x  = mi.rcWork.left;
			cs.cx = mi.rcWork.right - mi.rcWork.left;
		}
	}

	if ( (cs.y + cs.cy) > mi.rcWork.bottom ) {
		if ( (cs.y = mi.rcWork.bottom - cs.cy) < mi.rcWork.top ) {
			cs.y  = mi.rcWork.top;
			cs.cy = mi.rcWork.bottom - mi.rcWork.top;
		}
	}
#endif

	// モニターDPIを取得
	if ( ExGetDpiForMonitor != NULL )
		ExGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &m_ScreenDpiX, &m_ScreenDpiY);

	// メニューをリソースデータベースに置き換え
	if ( cs.hMenu != NULL ) {
		DestroyMenu(cs.hMenu);
		((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_MAINFRAME), cs.hMenu);
	}

	//TRACE("Main Style ");
	//if ( (cs.style & WS_BORDER) != NULL ) TRACE("WS_BORDER ");
	//if ( (cs.style & WS_DLGFRAME) != NULL ) TRACE("WS_DLGFRAME ");
	//if ( (cs.style & WS_THICKFRAME) != NULL ) TRACE("WS_THICKFRAME ");
	//if ( (cs.dwExStyle & WS_EX_WINDOWEDGE) != NULL ) TRACE("WS_EX_WINDOWEDGE ");
	//if ( (cs.dwExStyle & WS_EX_CLIENTEDGE) != NULL ) TRACE("WS_EX_CLIENTEDGE ");
	//if ( (cs.dwExStyle & WS_EX_DLGMODALFRAME) != NULL ) TRACE("WS_EX_DLGMODALFRAME ");
	//if ( (cs.dwExStyle & WS_EX_TOOLWINDOW) != NULL ) TRACE("WS_EX_TOOLWINDOW ");
	//TRACE("\n");

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

#define AGENT_COPYDATA_ID 0x804e50ba   /* random goop */
#define AGENT_MAX_MSGLEN  8192

BOOL CMainFrame::PageantQuery(CBuffer *pInBuf, CBuffer *pOutBuf)
{
	int len;
	CWnd *pWnd;
	CString mapname;
	HANDLE filemap;
	BYTE *p;
	COPYDATASTRUCT cds;
	CStringA mbs;

	if ( (pWnd = FindWindow(_T("Pageant"), _T("Pageant"))) == NULL )
		return FALSE;

	mapname.Format(_T("PageantRequest%08x"), (unsigned)GetCurrentThreadId());
	filemap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,	0, AGENT_MAX_MSGLEN, mapname);

	if ( filemap == NULL || filemap == INVALID_HANDLE_VALUE )
		return FALSE;

	if ( (p = (BYTE *)MapViewOfFile(filemap, FILE_MAP_WRITE, 0, 0, 0)) == NULL ) {
		CloseHandle(filemap);
		return FALSE;
	}

	ASSERT(pInBuf->GetSize() < AGENT_MAX_MSGLEN);
	memcpy(p, pInBuf->GetPtr(), pInBuf->GetSize());

	mbs = mapname;
	cds.dwData = AGENT_COPYDATA_ID;
	cds.cbData = mbs.GetLength() + 1;
	cds.lpData = mbs.GetBuffer();

	pOutBuf->Clear();

	if ( pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&cds) > 0 ) {
		len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
		if ( len > 0 && (len + 4) < AGENT_MAX_MSGLEN )
			pOutBuf->Apend(p + 4, len);
	}

    UnmapViewOfFile(p);
    CloseHandle(filemap);

	return TRUE;
}
BOOL CMainFrame::PageantInit()
{
	int n, i;
	int count;
	CBuffer in, out;
	CIdKey key;
	CBuffer blob;
	CStringA name;

	for ( i = 0 ; i < m_IdKeyTab.GetSize() ; i++ ) {
		if ( m_IdKeyTab[i].m_bPageant )
			m_IdKeyTab[i].m_bSecInit = FALSE;
	}

	in.Put32Bit(1);
	in.Put8Bit(SSH_AGENTC_REQUEST_IDENTITIES);

	if ( !PageantQuery(&in, &out) )
		return FALSE;

	if ( out.GetSize() < 5 || out.Get8Bit() != SSH_AGENT_IDENTITIES_ANSWER )
		return FALSE;

	try {
		count = out.Get32Bit();
		for ( n = 0 ; n < count ; n++ ) {
			out.GetBuf(&blob);
			out.GetStr(name);
			key.m_Name = name;
			key.m_bPageant = TRUE;
			key.m_bSecInit = TRUE;
			if ( !key.GetBlob(&blob) )
				continue;

			for ( i = 0 ; i < m_IdKeyTab.GetSize() ; i++ ) {
				if ( m_IdKeyTab[i].m_bPageant && m_IdKeyTab[i].ComperePublic(&key) == 0 ) {
					m_IdKeyTab[i].m_bSecInit = TRUE;
					break;
				}
			}

			if ( i >= m_IdKeyTab.GetSize() )
				m_IdKeyTab.AddEntry(key, FALSE);
		}

#ifdef	DEBUG
	} catch(LPCTSTR msg) {
		TRACE(_T("PageantInit Error %s '%s'\n"), MbsToTstr(name), msg);
		return FALSE;
#endif
	} catch(...) {
		return FALSE;
	}

	return TRUE;
}
BOOL CMainFrame::PageantSign(CBuffer *blob, CBuffer *sign, LPBYTE buf, int len)
{
	CBuffer in, out, work;

	work.Put8Bit(SSH_AGENTC_SIGN_REQUEST);
	work.PutBuf(blob->GetPtr(), blob->GetSize());
	work.PutBuf(buf, len);
	work.Put32Bit(0);
	
	in.PutBuf(work.GetPtr(), work.GetSize());

	if ( !PageantQuery(&in, &out) )
		return FALSE;

	if ( out.GetSize() < 5 || out.Get8Bit() != SSH_AGENT_SIGN_RESPONSE )
		return FALSE;

	sign->Clear();
	out.GetBuf(sign);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

int CMainFrame::SetAsyncSelect(SOCKET fd, CExtSocket *pSock, long lEvent)
{
	ASSERT(pSock->m_Type >= 0 && pSock->m_Type < 10);

	if ( lEvent != 0 && WSAAsyncSelect(fd, GetSafeHwnd(), WM_SOCKSEL, lEvent) != 0 )
		return FALSE;

	((CRLoginApp *)AfxGetApp())->AddIdleProc(IDLEPROC_SOCKET, pSock);

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

	((CRLoginApp *)AfxGetApp())->DelIdleProc(IDLEPROC_SOCKET, pSock);

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
	m_HostAddrParam.Add((void *)(INT_PTR)mode);

	return TRUE;
}

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

int CMainFrame::SetAsyncAddrInfo(int mode, LPCTSTR pHostName, int PortNum, void *pHint, CExtSocket *pSock)
{
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
	m_HostAddrParam.Add((void *)(INT_PTR)mode);

	return TRUE;
}

int CMainFrame::SetAfterId(void *param)
{
	static int SeqId = 0;

	m_AfterIdParam.Add((void *)(INT_PTR)(++SeqId));
	m_AfterIdParam.Add(param);

	return SeqId;
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
			KillTimer(tp->m_Id);
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

/////////////////////////////////////////////////////////////////////////////

int CMainFrame::OpenServerEntry(CServerEntry &Entry)
{
	int n;
	CServerSelect dlg;
	CWnd *pTemp = MDIGetActive(NULL);
	CPaneFrame *pPane = NULL;
	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	dlg.m_pData = &m_ServerEntryTab;
	dlg.m_EntryNum = (-1);

	if ( m_LastPaneFlag && pTemp != NULL && m_pTopPane != NULL &&
			(pPane = m_pTopPane->GetPane(pTemp->m_hWnd)) != NULL && pPane->m_pServerEntry != NULL ) {
		Entry = *(pPane->m_pServerEntry);
		Entry.m_DocType = DOCTYPE_MULTIFILE;
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
		if ( pApp->m_pServerEntry != NULL ) {
			Entry = *(pApp->m_pServerEntry);
			pApp->m_pServerEntry = NULL;
			return TRUE;
		}
		if ( pApp->m_pCmdInfo != NULL && !pApp->m_pCmdInfo->m_Name.IsEmpty() ) {
			for ( n = 0 ; n< m_ServerEntryTab.m_Data.GetSize() ; n++ ) {
				if ( pApp->m_pCmdInfo->m_Name.Compare(m_ServerEntryTab.m_Data[n].m_EntryName) == 0 ) {
					Entry = m_ServerEntryTab.m_Data[n];
					Entry.m_DocType = DOCTYPE_REGISTORY;
					return TRUE;
				}
			}
		}
		if ( pApp->m_pCmdInfo != NULL && pApp->m_pCmdInfo->m_Proto != (-1) && !pApp->m_pCmdInfo->m_Port.IsEmpty() ) {
			if ( Entry.m_EntryName.IsEmpty() )
				Entry.m_EntryName.Format(_T("%s:%s"), (pApp->m_pCmdInfo->m_Addr.IsEmpty() ? _T("unkown") : pApp->m_pCmdInfo->m_Addr), pApp->m_pCmdInfo->m_Port);
			Entry.m_DocType = DOCTYPE_SESSION;
			return TRUE;
		}

		if ( dlg.DoModal() != IDOK || dlg.m_EntryNum < 0 )
			return FALSE;
	}

	m_ServerEntryTab.m_Data[dlg.m_EntryNum].m_CheckFlag = FALSE;
	Entry = m_ServerEntryTab.m_Data[dlg.m_EntryNum];
	Entry.m_DocType = DOCTYPE_REGISTORY;

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

	//rgb = RGB(0, 0, 0);
	//value = 255;
	//flag = LWA_COLORKEY;

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

/////////////////////////////////////////////////////////////////////////////

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
void CMainFrame::RemoveChild(CWnd *pWnd, BOOL bDelete)
{
	m_wndTabBar.Remove(pWnd);
	if ( m_wndTabBar.m_TabCtrl.GetItemCount() <= 1 )
		ShowControlBar(&m_wndTabBar, m_bTabBarShow, TRUE);

	if ( m_pTopPane != NULL ) {
		CPaneFrame *pPane = m_pTopPane->GetPane(pWnd->m_hWnd);
		if ( pPane != NULL ) {
			if ( bDelete || (pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM) ) {
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
#ifdef	DEBUG_XXX
	m_pTopPane->Dump();
#endif
}
BOOL CMainFrame::IsWindowPanePoint(CPoint point)
{
	if ( m_pTopPane == NULL )
		return FALSE;

	point.y -= m_Frame.top;

	CPaneFrame *pPane = m_pTopPane->HitTest(point);

	if ( pPane == NULL || pPane->m_Style != PANEFRAME_WINDOW )
		return FALSE;

	return TRUE;
}
void CMainFrame::MoveChild(CWnd *pWnd, CPoint point)
{
	if ( m_pTopPane == NULL )
		return;

	point.y -= m_Frame.top;

	HWND hLeft, hRight;
	CPaneFrame *pLeftPane  = m_pTopPane->GetPane(pWnd->m_hWnd);
	CPaneFrame *pRightPane = m_pTopPane->HitTest(point);

	if ( pLeftPane == NULL || pRightPane == NULL )
		return;

	if ( pLeftPane->m_Style != PANEFRAME_WINDOW || pRightPane->m_Style != PANEFRAME_WINDOW )
		return;

	hLeft = pLeftPane->m_hWnd;
	hRight = pRightPane->m_hWnd;

	if ( hLeft == NULL || hLeft == hRight )
		return;

	pLeftPane->m_hWnd = hRight;
	pRightPane->m_hWnd = hLeft;

	pLeftPane->MoveFrame();
	pRightPane->MoveFrame();
}
void CMainFrame::SwapChild(CWnd *pLeft, CWnd *pRight)
{
	if ( m_pTopPane == NULL || pLeft == NULL || pRight == NULL )
		return;

	HWND hLeft, hRight;
	CPaneFrame *pLeftPane  = m_pTopPane->GetPane(pLeft->m_hWnd);
	CPaneFrame *pRightPane = m_pTopPane->GetPane(pRight->m_hWnd);

	if ( pLeftPane == NULL || pRightPane == NULL )
		return;

	if ( pLeftPane->m_Style != PANEFRAME_WINDOW || pRightPane->m_Style != PANEFRAME_WINDOW )
		return;

	if ( (hLeft = pLeftPane->m_hWnd) == NULL || (hRight = pRightPane->m_hWnd) == NULL || hLeft == hRight )
		return;

	pLeftPane->m_hWnd = hRight;
	pRightPane->m_hWnd = hLeft;

	pLeftPane->MoveFrame();
	pRightPane->MoveFrame();
}
int CMainFrame::GetTabIndex(CWnd *pWnd)
{
	return m_wndTabBar.GetIndex(pWnd);
}
void CMainFrame::GetTabTitle(CWnd *pWnd, CString &title)
{
	int idx;
	
	if ( (idx = m_wndTabBar.GetIndex(pWnd)) >= 0 )
		m_wndTabBar.GetTitle(idx, title);
}
CWnd *CMainFrame::GetTabWnd(int idx)
{
	return m_wndTabBar.GetAt(idx);
}
int CMainFrame::GetTabCount()
{
	return m_wndTabBar.GetSize();
}
CRLoginDoc *CMainFrame::GetMDIActiveDocument()
{
	CChildFrame *pChild;
	CRLoginDoc *pDoc = NULL;

	if ( (pChild = (CChildFrame *)(MDIGetActive())) != NULL )
		pDoc = (CRLoginDoc *)(pChild->GetActiveDocument());

	return pDoc;
}

BOOL CMainFrame::IsOverLap(HWND hWnd)
{
	CPaneFrame *pPane;

	if ( m_pTopPane == NULL || (pPane = m_pTopPane->GetPane(hWnd)) == NULL )
		return FALSE;
	return (m_pTopPane->IsOverLap(pPane) == 1 ? TRUE : FALSE);
}
BOOL CMainFrame::IsTopLevelDoc(CRLoginDoc *pDoc)
{
	CRLoginView *pView;
	CChildFrame *pChild;
	CPaneFrame *pPane;
	
	if ( pDoc == NULL || (pView = (CRLoginView *)pDoc->GetAciveView()) == NULL || (pChild = pView->GetFrameWnd()) == NULL )
		return FALSE;

	if ( m_pTopPane == NULL || (pPane = m_pTopPane->GetPane(pChild->m_hWnd)) == NULL )
		return TRUE;

	if ( m_pTopPane->IsTopLevel(pPane) )
		return TRUE;

	return FALSE;
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

/////////////////////////////////////////////////////////////////////////////

static BOOL CALLBACK RLoginExecCountFunc(HWND hwnd, LPARAM lParam)
{
	CMainFrame *pMain = (CMainFrame *)lParam;

	if ( pMain->m_hWnd != hwnd && CRLoginApp::IsRLoginWnd(hwnd) )
		pMain->m_ExecCount++;

	return TRUE;
}
int CMainFrame::GetExecCount()
{
	m_ExecCount = 0;
	::EnumWindows(RLoginExecCountFunc, (LPARAM)this);
	return m_ExecCount;
}
void CMainFrame::SetActivePoint(CPoint point)
{
	CPaneFrame *pPane;

	ScreenToClient(&point);

	point.y -= m_Frame.top;
	if ( m_pTrackPane != NULL || m_pTopPane == NULL )
		return;

	if ( (pPane = m_pTopPane->HitTest(point)) == NULL )
		return;

	if ( pPane->m_Style == PANEFRAME_WINDOW ) {
		m_pTopPane->HitActive(point);
		if ( pPane->m_hWnd != NULL ) {
			CChildFrame *pWnd = (CChildFrame *)CWnd::FromHandle(pPane->m_hWnd);
			pWnd->MDIActivate();
		}
	}
}
void CMainFrame::SetStatusText(LPCTSTR message)
{
	if ( m_StatusTimer != 0 )
		KillTimer(m_StatusTimer);

	SetMessageText(message);

	m_StatusTimer = SetTimer(TIMERID_STATUSCLR, 30000, NULL);
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::ClipBoradStr(LPCWSTR str, CString &tmp)
{
	int n, i;

	tmp.Empty();

	for ( n = 0 ; n < 50 && *str != L'\0' ; n++, str++ ) {
		if ( *str == L'\n' )
			n--;
		else if ( *str == L'\r' )
			tmp += _T("↓");
		else if ( *str == L'\x7F' || *str < L' ' || *str == L'&' || *str == L'\\' )
			tmp += _T('.');
		else if ( *str >= 256 ) {
			n++;
			tmp += *str;
		} else
			tmp += *str;
		if ( n >= 40 && (i = (int)wcslen(str)) > 10 ) {
			tmp += _T(" ... ");
			str += (i - 11);
		}
	}
}
void CMainFrame::SetClipBoardComboBox(CComboBox *pCombo)
{
	int index = 1;
	POSITION pos;
	CString str, tmp;

	for ( pos = m_ClipBoard.GetHeadPosition() ; pos != NULL ; ) {
		ClipBoradStr((LPCWSTR)m_ClipBoard.GetNext(pos), str);

		tmp.Format(_T("%d %s"), (index++) % 10, str);
		pCombo->AddString(tmp);
	}
}
void CMainFrame::SetClipBoardMenu(UINT nId, CMenu *pMenu)
{
	int n;
	int index = 1;
	POSITION pos;
	CString str, tmp;

	for ( n = 0 ; n < 10 ; n++ )
		pMenu->DeleteMenu(nId + n, MF_BYCOMMAND);
	
	for ( pos = m_ClipBoard.GetHeadPosition() ; pos != NULL ; ) {
		ClipBoradStr((LPCWSTR)m_ClipBoard.GetNext(pos), str);

		tmp.Format(_T("&%d %s"), (index++) % 10, str);
		pMenu->AppendMenu(MF_STRING, nId++, tmp);
	}
}
BOOL CMainFrame::CopyClipboardData(CString &str)
{
	int len, max = 0;
	HGLOBAL hData;
	WCHAR *pData = NULL;
	BOOL ret = FALSE;

	// 10msロック出来るまで待つ
	if ( !m_OpenClipboardLock.Lock(10) )
		return FALSE;

	if ( !IsClipboardFormatAvailable(CF_UNICODETEXT) )
		goto UNLOCKRET;

	if ( !OpenClipboard() )
		goto UNLOCKRET;

	if ( (hData = GetClipboardData(CF_UNICODETEXT)) == NULL )
		goto CLOSERET;

	if ( (pData = (WCHAR *)GlobalLock(hData)) == NULL )
		goto CLOSERET;

	str.Empty();
	max = (int)GlobalSize(hData) / sizeof(WCHAR);

	for ( len = 0 ; len < max && len < (256 * 1024) && *pData != L'\0' && *pData != L'\x1A' ; len++ )
		str += *(pData++);

	GlobalUnlock(hData);
	ret = TRUE;

CLOSERET:
	CloseClipboard();

UNLOCKRET:
	m_OpenClipboardLock.Unlock();
	return ret;
}
static UINT CopyClipboardThead(LPVOID pParam)
{
	int n;
	CString *pStr = new CString;
	CMainFrame *pWnd = (CMainFrame *)pParam;

	for ( n = 0 ; ; n++ ) {
		if ( pWnd->CopyClipboardData(*pStr) ) {
			pWnd->PostMessage(WM_GETCLIPBOARD, NULL, (LPARAM)pStr);
			break;
		}

		if ( n >= 10 ) {
			delete pStr;
			break;
		}

		Sleep(100);
	}

	pWnd->m_bClipThreadCount--;
	return 0;
}
BOOL CMainFrame::SetClipboardText(LPCTSTR str)
{
	HGLOBAL hData;
	LPTSTR pData;

	// 500msロック出来るまで待つ
	if ( !m_OpenClipboardLock.Lock(500) ) {
		MessageBox(_T("Clipboard Busy..."));
		return FALSE;
	}

	if ( (hData = GlobalAlloc(GMEM_MOVEABLE, (_tcslen(str) + 1) * sizeof(TCHAR))) == NULL ) {
		m_OpenClipboardLock.Unlock();
		MessageBox(_T("Global Alloc Error"));
		return FALSE;
	}

	if ( (pData = (TCHAR *)GlobalLock(hData)) == NULL ) {
		GlobalFree(hData);
		m_OpenClipboardLock.Unlock();
		MessageBox(_T("Global Lock Error"));
		return FALSE;
	}

	_tcscpy(pData, str);
	GlobalUnlock(pData);

	// クリップボードチェインのチェックの為の処理
	m_bClipEnable = FALSE;

	for ( int n = 0 ; !OpenClipboard() ; n++ ) {
		if ( n >= 10 ) {
			GlobalFree(hData);
			m_OpenClipboardLock.Unlock();
			MessageBox(_T("Clipboard Open Error"));
			return FALSE;
		}
		Sleep(100);
	}

	if ( !EmptyClipboard() ) {
		GlobalFree(hData);
		CloseClipboard();
		m_OpenClipboardLock.Unlock();
		MessageBox(_T("Clipboard Empty Error"));
		return FALSE;
	}

#ifdef	_UNICODE
	if ( SetClipboardData(CF_UNICODETEXT, hData) == NULL ) {
#else
	if ( SetClipboardData(CF_TEXT, hData) == NULL ) {
#endif
		GlobalFree(hData);
		CloseClipboard();
		m_OpenClipboardLock.Unlock();
		MessageBox(_T("Clipboard Set Data Error"));
		return FALSE;
	}

	CloseClipboard();
	m_OpenClipboardLock.Unlock();
	return TRUE;
}
BOOL CMainFrame::GetClipboardText(CString &str)
{
	// クリップボードチェインが動かない場合
	if ( !m_bClipEnable )
		SendMessage(WM_GETCLIPBOARD);

	if ( m_ClipBoard.IsEmpty() )
		return FALSE;

	str = m_ClipBoard.GetHead();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

static UINT VersionCheckThead(LPVOID pParam)
{
	CMainFrame *pWnd = (CMainFrame *)pParam;
	pWnd->VersionCheckProc();
	return 0;
}
void CMainFrame::VersionCheckProc()
{
	CBuffer buf;
	CHttpSession http;
	CHAR *p, *e;
	CString str;
	CStringArray pam;
	CStringLoad version;

	((CRLoginApp *)AfxGetApp())->GetVersion(version);
	str = AfxGetApp()->GetProfileString(_T("MainFrame"), _T("VersionNumber"), _T(""));
	if ( version.CompareDigit(str) < 0 )
		version = str;

	if ( !http.GetRequest(CStringLoad(IDS_VERSIONCHECKURL), buf) )
		return;

	p = (CHAR *)buf.GetPtr();
	e = p + buf.GetSize();

	while ( p < e ) {
		str.Empty();
		pam.RemoveAll();

		for ( ; ; ) {
			if ( p >= e ) {
				pam.Add(str);
				break;
			} else if ( *p == '\n' ) {
				pam.Add(str);
				p++;
				break;
			} else if ( *p == '\r' ) {
				p++;
			} else if ( *p == '\t' || *p == ' ' ) {
				while ( *p == '\t' || *p == ' ' )
					p++;
				pam.Add(str);
				str.Empty();
			} else {
				str += *(p++);
			}
		}

		// 0      1      2          3
		// RLogin 2.18.4 2015/05/20 http://nanno.dip.jp/softlib/

		if ( pam.GetSize() >= 4 && pam[0].CompareNoCase(_T("RLogin")) == 0 && version.CompareDigit(pam[1]) < 0 ) {
			AfxGetApp()->WriteProfileString(_T("MainFrame"), _T("VersionNumber"), pam[1]);
			m_VersionMessage.Format(CStringLoad(IDS_NEWVERSIONCHECK), pam[1]);
			m_VersionPageUrl = pam[3];
			PostMessage(WM_COMMAND, IDM_NEWVERSIONFOUND);
			break;
		}
	}
}
void CMainFrame::VersionCheck()
{
	time_t now;
	int today, last;

	if ( !m_bVersionCheck )
		return;

	time(&now);
	today = (int)(now / (24 * 60 * 60));
	last = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("VersionCheck"), 0);

	if ( (last + 7) > today )
		return;

	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("VersionCheck"), today);

	AfxBeginThread(VersionCheckThead, this, THREAD_PRIORITY_LOWEST);
}

/////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////
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

	if ( (fs & FD_CLOSE) == 0 && WSAGETSELECTERROR(lParam) != 0 ) {
		pSock->OnError(WSAGETSELECTERROR(lParam));
		return TRUE;
	}

	if ( (fs & FD_CONNECT) != 0 )
		pSock->OnPreConnect();
	if ( (fs & FD_ACCEPT) != 0 )
		pSock->OnAccept((SOCKET)wParam);
	if ( (fs & FD_READ) != 0 )
		pSock->OnReceive(0);
	if ( (fs & FD_OOB) != 0 )
		pSock->OnReceive(MSG_OOB);
	if ( (fs & FD_WRITE) != 0 )
		pSock->OnSend();
	if ( (fs & FD_CLOSE) != 0 )
		pSock->OnPreClose();

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
		mode = (int)(INT_PTR)(m_HostAddrParam[n + 4]);
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

LRESULT CMainFrame::OnAfterOpen(WPARAM wParam, LPARAM lParam)
{
	int n;

	for ( n = 0 ; n < m_AfterIdParam.GetSize() ; n += 2 ) {
		if ( (INT_PTR)(m_AfterIdParam[n]) == (INT_PTR)(wParam) ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)m_AfterIdParam[n + 1];

			m_AfterIdParam.RemoveAt(n, 2);

			if ( !((CRLoginApp *)AfxGetApp())->CheckDocument(pDoc) )
				break;

			if ( (int)lParam != 0 ) {
				pDoc->OnSocketError((int)lParam);

			} else {
				pDoc->SocketOpen();

				CRLoginView *pView = (CRLoginView *)pDoc->GetAciveView();

				if ( pView != NULL )
					MDIActivate(pView->GetFrameWnd());
			}
			break;
		}
	}

	return TRUE;
}
LRESULT CMainFrame::OnGetClipboard(WPARAM wParam, LPARAM lParam)
{
	CString *pStr, tmp;

	if ( lParam != NULL )
		pStr = (CString *)lParam;
	else if ( CopyClipboardData(tmp) )
		pStr = &tmp;
	else
		return TRUE;

	POSITION pos = m_ClipBoard.GetHeadPosition();

	while ( pos != NULL ) {
		if ( m_ClipBoard.GetAt(pos).Compare(TstrToUni(*pStr)) == 0 ) {
			m_ClipBoard.RemoveAt(pos);
			break;
		}
		m_ClipBoard.GetNext(pos);
	}

	m_ClipBoard.AddHead(*pStr);

	while ( m_ClipBoard.GetSize() > 10 )
		m_ClipBoard.RemoveTail();

	if ( lParam != NULL )
		delete pStr;

	return TRUE;
}
LRESULT CMainFrame::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	// wParam
	//	The HIWORD of the wParam contains the Y-axis value of the new dpi of the window. 
	//	The LOWORD of the wParam contains the X-axis value of the new DPI of the window.
	//
	// lParam
	//	A pointer to a RECT structure that provides a suggested size and position of the 
	//	current window scaled for the new DPI. The expectation is that apps will reposition 
	//	and resize windows based on the suggestions provided by lParam when handling this message.

	m_ScreenDpiX = LOWORD(wParam);
	m_ScreenDpiY = HIWORD(wParam);

	TabBarFontCheck();

	MoveWindow((RECT *)lParam, TRUE);

	return TRUE;
}

void CMainFrame::OnClose()
{
	int count = 0;
	CWinApp *pApp = AfxGetApp();

	POSITION pos = pApp->GetFirstDocTemplatePosition();
	while ( pos != NULL ) {
		CDocTemplate *pDocTemp = pApp->GetNextDocTemplate(pos);
		POSITION dpos = pDocTemp->GetFirstDocPosition();
		while ( dpos != NULL ) {
			CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
			if ( pDoc != NULL && pDoc->m_pSock != NULL && pDoc->m_pSock->m_bConnect )
				count++;
		}
	}

	if ( count > 0 && AfxMessageBox(CStringLoad(IDS_FILECLOSEQES), MB_ICONQUESTION | MB_YESNO) != IDYES )
		return;

	CMDIFrameWnd::OnClose();
}

void CMainFrame::OnDestroy() 
{
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("ToolBarStyle"),	m_wndToolBar.GetStyle());
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("StatusBarStyle"), m_wndStatusBar.GetStyle());

	//AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("TabBarShow"), m_bTabBarShow);
	//AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("LayeredWindow"), m_TransParValue);
	//AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("LayeredColor"), m_TransParColor);
	//AfxGetApp()->WriteProfileInt(_T("ChildFrame"), _T("VScroll"), m_ScrollBarFlag);
	//AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("VersionCheckFlag"), m_bVersionCheck);

	if ( !IsIconic() && !IsZoomed() ) {
		int n = GetExecCount();
		CString sect;
		CRect rect;

		GetWindowRect(&rect);

		if ( n == 0 )
			sect = _T("MainFrame");
		else
			sect.Format(_T("SecondFrame%02d"), n);

		AfxGetApp()->WriteProfileInt(sect, _T("x"), rect.left);
		AfxGetApp()->WriteProfileInt(sect, _T("y"), rect.top);
		AfxGetApp()->WriteProfileInt(sect, _T("cx"), rect.Width());
		AfxGetApp()->WriteProfileInt(sect, _T("cy"), rect.Height());
	}

	if ( m_bClipChain ) {
		m_bClipChain = FALSE;
		ChangeClipboardChain(m_hNextClipWnd);

	} else if ( ExRemoveClipboardFormatListener != NULL )
		ExRemoveClipboardFormatListener(m_hWnd);

	if ( m_TempPath.GetSize() > 0 ) {
		int n, er;
		CString msg;
		do {
			msg.Format(CStringLoad(IDS_TEMPFILEDELETEMSG), m_TempPath.GetSize());
			for ( er = n = 0 ; n < m_TempPath.GetSize() ; n++ ) {
				if ( DeleteFile(m_TempPath[n]) ) {
					m_TempPath.RemoveAt(n);
					n--;
				} else if ( ++er < 3 ) {
					msg += _T("\n");
					msg += m_TempPath[n];
				}
			}
		} while ( er > 0 && MessageBox(msg, _T("Question"), MB_ICONQUESTION | MB_YESNO) == IDYES );
	}

	CMDIFrameWnd::OnDestroy();
}

void CMainFrame::OnEnterIdle(UINT nWhy, CWnd* pWho)
{
	CMDIFrameWnd::OnEnterIdle(nWhy, pWho);

	if ( nWhy == MSGF_MENU )
		((CRLoginApp *)AfxGetApp())->OnIdle(-1);
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

	} else if ( nIDEvent == m_StatusTimer ) {
		KillTimer(m_StatusTimer);
		m_StatusTimer = 0;
		SetMessageText(AFX_IDS_IDLEMESSAGE);

	} else if ( nIDEvent == TIMERID_CLIPUPDATE ) {
		if ( m_bClipChain ) {
			ChangeClipboardChain(m_hNextClipWnd);
			m_hNextClipWnd = SetClipboardViewer();
		} else {
			KillTimer(nIDEvent);
			m_ClipTimer = 0;
		}

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

void CMainFrame::SplitWidthPane()
{
	if ( m_pTopPane == NULL )
		m_pTopPane = new CPaneFrame(this, NULL, NULL);

	CPaneFrame *pPane = m_pTopPane->GetActive();

	if ( pPane->m_pOwn == NULL || pPane->m_pOwn->m_Style != PANEFRAME_MAXIM )
		pPane->CreatePane(PANEFRAME_WIDTH, NULL);
	else {
		while ( pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM )
			pPane = pPane->m_pOwn;
		pPane = pPane->InsertPane();
		if ( pPane->m_pOwn == NULL )
			m_pTopPane = pPane;
		pPane->MoveParOwn(pPane->m_Frame, PANEFRAME_WIDTH);
	}
}
void CMainFrame::SplitHeightPane()
{
	if ( m_pTopPane == NULL )
		m_pTopPane = new CPaneFrame(this, NULL, NULL);

	CPaneFrame *pPane = m_pTopPane->GetActive();

	if ( pPane->m_pOwn == NULL || pPane->m_pOwn->m_Style != PANEFRAME_MAXIM )
		pPane->CreatePane(PANEFRAME_HEIGHT, NULL);
	else {
		while ( pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM )
			pPane = pPane->m_pOwn;
		pPane = pPane->InsertPane();
		if ( pPane->m_pOwn == NULL )
			m_pTopPane = pPane;
		pPane->MoveParOwn(pPane->m_Frame, PANEFRAME_HEIGHT);
	}
}
CPaneFrame *CMainFrame::GetPaneFromChild(HWND hWnd)
{
	if ( m_pTopPane == NULL )
		return NULL;

	return m_pTopPane->GetPane(hWnd);
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

	CPaneFrame *pPane, *pOwner;

	if ( (pPane = m_pTopPane->GetActive()) == NULL )
		return;

	if ( pPane->m_Style == PANEFRAME_WINDOW && pPane->m_hWnd == NULL && (pOwner = pPane->m_pOwn) != NULL ) {
		pPane->m_pLeft = pPane->m_pRight = NULL;
		delete pPane;

		pPane = (pOwner->m_pLeft == pPane ? pOwner->m_pRight : pOwner->m_pLeft);
		pOwner->m_Style  = pPane->m_Style;
		pOwner->m_pLeft  = pPane->m_pLeft;
		pOwner->m_pRight = pPane->m_pRight;

		pOwner->m_hWnd   = pPane->m_hWnd;
		if ( pOwner->m_pServerEntry != NULL )
			delete pOwner->m_pServerEntry;
		pOwner->m_pServerEntry = pPane->m_pServerEntry;
		pPane->m_pServerEntry = NULL;

		if ( pOwner->m_pLeft != NULL )
			pOwner->m_pLeft->m_pOwn  = pOwner;
		if ( pOwner->m_pRight != NULL )
			pOwner->m_pRight->m_pOwn = pOwner;

		pPane->m_pLeft = pPane->m_pRight = NULL;
		delete pPane;

		pOwner->MoveParOwn(pOwner->m_Frame, PANEFRAME_NOCHNG);
		return;
	}

	for ( pOwner = pPane->m_pOwn ; pOwner != NULL ; pOwner = pOwner->m_pOwn ) {

		if ( pOwner->m_pLeft != NULL && pOwner->m_pLeft->m_hWnd == NULL && pOwner->m_pLeft->m_pLeft == NULL )
			pOwner->DeletePane(NULL);
		if ( pOwner->m_pRight != NULL && pOwner->m_pRight->m_hWnd == NULL && pOwner->m_pRight->m_pLeft == NULL )
			pOwner->DeletePane(NULL);

		if ( pOwner->m_Style != PANEFRAME_MAXIM ) {
			pOwner->MoveParOwn(pOwner->m_Frame, PANEFRAME_MAXIM);
			break;
		}
	}
}
void CMainFrame::OnPaneSave() 
{
	if ( m_pTopPane == NULL )
		return;

	CBuffer buf;
	m_pTopPane->SetBuffer(&buf, FALSE);
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
	m_pTopPane->MoveParOwn(rect, m_SplitType);

	switch(m_SplitType) {
	case PANEFRAME_WSPLIT: m_SplitType = PANEFRAME_HSPLIT; break;
	case PANEFRAME_HSPLIT: m_SplitType = PANEFRAME_WEVEN;  break;
	case PANEFRAME_WEVEN:  m_SplitType = PANEFRAME_HEVEN;  break;
	case PANEFRAME_HEVEN:  m_SplitType = PANEFRAME_WSPLIT; break;
	}
}
void CMainFrame::OnWindowRotation()
{
	int n, idx;
	HWND hWnd;
	CWnd *pWnd;
	CPaneFrame *pPane, *pNext;
	CRLoginDoc *pDoc;

	if ( m_pTopPane == NULL )
		return;

	if ( (pWnd = MDIGetActive()) == NULL )
		return;

	if ( (idx = m_wndTabBar.GetIndex(pWnd)) < 0 )
		return;

	if ( (pPane = m_pTopPane->GetPane(pWnd->GetSafeHwnd())) == NULL )
		return;

	for ( n = 1 ; n < m_wndTabBar.GetSize() ; n++ ) {
		if ( ++idx >= m_wndTabBar.GetSize() )
			idx = 0;

		if ( (pWnd = m_wndTabBar.GetAt(idx)) == NULL )
			break;

		if ( (pNext = m_pTopPane->GetPane(pWnd->GetSafeHwnd())) == NULL )
			break;

		hWnd = pPane->m_hWnd;
		pPane->m_hWnd = pNext->m_hWnd;
		pNext->m_hWnd = hWnd;

		pPane->MoveFrame();
		pPane = pNext;
	}
	pPane->MoveFrame();

	if ( (pDoc = GetMDIActiveDocument()) != NULL && pDoc->m_TextRam.IsOptEnable(TO_RLPAINWTAB) )
		m_wndTabBar.NextActive();

	PostMessage(WM_COMMAND, IDM_DISPWINIDX);
}
void CMainFrame::OnUpdateWindowCascade(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pTopPane != NULL && m_pTopPane->m_Style != PANEFRAME_WINDOW ? TRUE : FALSE);
}
void CMainFrame::OnUpdateWindowTileHorz(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pTopPane != NULL && m_pTopPane->m_Style != PANEFRAME_WINDOW ? TRUE : FALSE);
}
void CMainFrame::OnUpdateWindowRotation(CCmdUI *pCmdUI)
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
		LPCTSTR id = (pPane->m_Style == PANEFRAME_HEIGHT ? ATL_MAKEINTRESOURCE(AFX_IDC_VSPLITBAR) : ATL_MAKEINTRESOURCE(AFX_IDC_HSPLITBAR));
		HINSTANCE hInst = AfxFindResourceHandle(id, ATL_RT_GROUP_CURSOR);
		HCURSOR hCursor = NULL;

		if ( hInst != NULL )
			hCursor = ::LoadCursorW(hInst, id);

		if ( hCursor == NULL )
			hCursor = AfxGetApp()->LoadStandardCursor(pPane->m_Style == PANEFRAME_HEIGHT ? IDC_SIZENS : IDC_SIZEWE);

		if ( hCursor != NULL )
			::SetCursor(hCursor);

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

	} else if ( (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) ) {
		if ( MDIGetActive() == NULL ) {
			int id, st = 0;
			CBuffer tmp;

			if ( (GetKeyState(VK_SHIFT) & 0x80) != 0 )
				st |= MASK_SHIFT;

			if ( (GetKeyState(VK_CONTROL) & 0x80) != 0 )
				st |= MASK_CTRL;

			if ( (GetKeyState(VK_MENU) & 0x80) != 0 )
				st |= MASK_ALT;

			if ( m_DefKeyTab.FindMaps((int)pMsg->wParam, st, &tmp) &&  (id = CKeyNodeTab::GetCmdsKey((LPCWSTR)tmp)) > 0 ) {
				PostMessage(WM_COMMAND, (WPARAM)id);
				return TRUE;
			}
		}

		if ( (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6) && (GetKeyState(VK_CONTROL) & 0x80) != 0 )
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
		if ( m_TrackRect.left < (rect.left + PANEMIN_WIDTH) ) {
			m_TrackRect.left = rect.left + PANEMIN_WIDTH;
			m_TrackRect.right = m_TrackRect.left + w;
		} else if ( m_TrackRect.right > (rect.right - PANEMIN_WIDTH) ) {
			m_TrackRect.right = rect.right - PANEMIN_WIDTH;
			m_TrackRect.left = m_TrackRect.right - w;
		}
	} else {
		m_TrackRect += CPoint(0, point.y - m_TrackPoint.y);
		int h = m_TrackRect.Height();
		if ( m_TrackRect.top < (rect.top + PANEMIN_HEIGHT) ) {
			m_TrackRect.top = rect.top + PANEMIN_HEIGHT;
			m_TrackRect.bottom = m_TrackRect.top + h;
		} else if ( m_TrackRect.bottom > (rect.bottom - PANEMIN_HEIGHT) ) {
			m_TrackRect.bottom = rect.bottom - PANEMIN_HEIGHT;
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
	int n = 6;
	CRLoginDoc *pDoc;
	static LPCTSTR ProtoName[] = { _T("TCP"), _T("Login"), _T("Telnet"), _T("SSH"), _T("COM"), _T("PIPE"), _T("") };

	if ( (pDoc = GetMDIActiveDocument()) != NULL && pDoc->m_pSock != NULL )
		n = pDoc->m_pSock->m_Type;

	pCmdUI->SetText(ProtoName[n]);

	//	m_wndStatusBar.GetStatusBarCtrl().SetIcon(pCmdUI->m_nIndex, AfxGetApp()->LoadIcon(IDI_LOCKICON));
}
void CMainFrame::OnUpdateIndicatorStat(CCmdUI* pCmdUI)
{
	LPCTSTR str = _T("");
	CRLoginDoc *pDoc;

	if ( (pDoc = GetMDIActiveDocument()) != NULL && pDoc->m_pSock != NULL )
		str = pDoc->m_SockStatus;

	pCmdUI->SetText(str);
}
void CMainFrame::OnUpdateIndicatorKmod(CCmdUI* pCmdUI)
{
	CRLoginDoc *pDoc;
	CString str;

	if ( (pDoc = GetMDIActiveDocument()) != NULL && pDoc->m_pSock != NULL ) {
		str += ( pDoc->m_TextRam.IsOptEnable(TO_RLPNAM) ? _T('A') : _T(' '));
		str += ( pDoc->m_TextRam.IsOptEnable(TO_DECCKM) ? _T('C') : _T(' '));
		str += (!pDoc->m_TextRam.IsOptEnable(TO_DECANM) ? _T('V') : _T(' '));
	}

	pCmdUI->SetText(str);
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
}
void CMainFrame::OnFileAllLoad() 
{
	CPaneFrame *pPane;

	if ( IsConnectChild(m_pTopPane) ) {
		if ( MessageBox(CStringLoad(IDE_ALLCLOSEREQ), _T("Warning"), MB_ICONQUESTION | MB_OKCANCEL) != IDOK )
			return;
	}
	
	try {
		if ( (pPane = CPaneFrame::GetBuffer(this, NULL, NULL, &m_AllFileBuf)) == NULL )
			return;
	} catch(...) {
		::AfxMessageBox(_T("File All Load Error"));
		return;
	}

	AfxGetApp()->CloseAllDocuments(FALSE);

	if ( m_pTopPane != NULL )
		delete m_pTopPane;
	m_pTopPane = pPane;

	if ( m_pTopPane->GetEntry() != NULL ) {
		m_LastPaneFlag = TRUE;
		AfxGetApp()->AddToRecentFileList(m_AllFilePath);
		PostMessage(WM_COMMAND, ID_FILE_NEW, 0);
	}
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if ( pCopyDataStruct->dwData == 0x524c4f31 ) {
		return ((CRLoginApp *)::AfxGetApp())->OnInUseCheck(pCopyDataStruct);

	} else if ( pCopyDataStruct->dwData == 0x524c4f32 ) {
		return ((CRLoginApp *)::AfxGetApp())->OnIsOnlineEntry(pCopyDataStruct);

#ifdef	USE_KEYMACGLOBAL
	} else if ( pCopyDataStruct->dwData == 0x524c4f33 ) {
		((CRLoginApp *)::AfxGetApp())->OnUpdateKeyMac(pCopyDataStruct);
		return TRUE;
#endif

	} else if ( pCopyDataStruct->dwData == 0x524c4f34 ) {
		if ( m_bBroadCast )
			((CRLoginApp *)::AfxGetApp())->OnSendBroadCast(pCopyDataStruct);
		return TRUE;

	} else if ( pCopyDataStruct->dwData == 0x524c4f35 ) {
		((CRLoginApp *)::AfxGetApp())->OnSendBroadCastMouseWheel(pCopyDataStruct);
		return TRUE;

	} else if ( pCopyDataStruct->dwData == 0x524c4f36 ) {
		if ( m_bBroadCast )
			((CRLoginApp *)::AfxGetApp())->OnSendGroupCast(pCopyDataStruct);
		return TRUE;

	} else if ( pCopyDataStruct->dwData == 0x524c4f37 ) {
		return ((CRLoginApp *)::AfxGetApp())->OnEntryData(pCopyDataStruct);

	} else if ( pCopyDataStruct->dwData == 0x524c4f38 ) {
		return ((CRLoginApp *)::AfxGetApp())->OnIsOpenRLogin(pCopyDataStruct);
	}

	return CMDIFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}

void CMainFrame::InitMenuBitmap()
{
	int n, cx, cy;
	CDC dc[2];
	CBitmap BitMap;
	CBitmap *pOld[2];
	CBitmap *pBitmap;
	BITMAP mapinfo;
	CMenuBitMap *pMap;

	// リソースデータベースからメニューイメージを作成
	cx = GetSystemMetrics(SM_CXMENUCHECK);
	cy = GetSystemMetrics(SM_CYMENUCHECK);

	dc[0].CreateCompatibleDC(NULL);
	dc[1].CreateCompatibleDC(NULL);

	CResDataBase *pResData = &(((CRLoginApp *)::AfxGetApp())->m_ResDataBase);

	// Add Menu Image From Bitmap Resource
	for ( n = 0 ; n < 3 ; n++ )
		pResData->AddBitmap(MAKEINTRESOURCE(IDB_MENUMAP1 + n));

	// MenuMap RemoveAll
	for ( int n = 0 ; n < m_MenuMap.GetSize() ; n++ ) {
		if ( (pMap = (CMenuBitMap *)m_MenuMap[n]) == NULL )
			continue;
		pMap->m_Bitmap.DeleteObject();
		delete pMap;
	}
	m_MenuMap.RemoveAll();

	for ( n = 0 ; n < pResData->m_Bitmap.GetSize() ; n++ ) {
		if ( pResData->m_Bitmap[n].m_hBitmap == NULL )
			continue;

		pBitmap = CBitmap::FromHandle(pResData->m_Bitmap[n].m_hBitmap);

		if ( pBitmap == NULL || !pBitmap->GetBitmap(&mapinfo) )
			continue;

		if ( (pMap = new CMenuBitMap) == NULL )
			continue;

		pMap->m_Id = pResData->m_Bitmap[n].m_ResId;
		pMap->m_Bitmap.CreateBitmap(cx, cy, dc[1].GetDeviceCaps(PLANES), dc[1].GetDeviceCaps(BITSPIXEL), NULL);
		m_MenuMap.Add(pMap);

		pOld[0] = dc[0].SelectObject(pBitmap);
		pOld[1] = dc[1].SelectObject(&(pMap->m_Bitmap));

		dc[1].FillSolidRect(0, 0, cx, cy, GetSysColor(COLOR_MENU));
		dc[1].TransparentBlt(0, 0, cx, cy, &(dc[0]), 0, 0, (mapinfo.bmWidth <= mapinfo.bmHeight ? mapinfo.bmWidth : mapinfo.bmHeight), mapinfo.bmHeight, RGB(192, 192, 192));

		dc[0].SelectObject(pOld[0]);
		dc[1].SelectObject(pOld[1]);
	}

	dc[0].DeleteDC();
	dc[1].DeleteDC();
}
void CMainFrame::SetMenuBitmap(CMenu *pMenu)
{
	int n;
	CMenuBitMap *pMap;

	for ( n = 0 ; n < m_MenuMap.GetSize() ; n++ ) {
		if ( (pMap = (CMenuBitMap *)m_MenuMap[n]) != NULL )
			pMenu->SetMenuItemBitmaps(pMap->m_Id, MF_BYCOMMAND, &(pMap->m_Bitmap), NULL);
	}
}
CBitmap *CMainFrame::GetMenuBitmap(UINT nId)
{
	int n;
	CMenuBitMap *pMap;

	for ( n = 0 ; n < m_MenuMap.GetSize() ; n++ ) {
		if ( (pMap = (CMenuBitMap *)m_MenuMap[n]) != NULL && pMap->m_Id == nId )
			return &(pMap->m_Bitmap);
	}
	return NULL;
}

void CMainFrame::OnEnterMenuLoop(BOOL bIsTrackPopupMenu)
{
	int n, a;
	CMenu *pMenu, *pSubMenu;
	CRLoginDoc *pDoc;
	CString str;

	if ( (pMenu = GetMenu()) == NULL )
		return;

	if ( (pDoc = GetMDIActiveDocument()) != NULL )
		pDoc->SetMenu(pMenu);

	else {
		m_DefKeyTab.CmdsInit();

		for ( n = 0 ; n < m_DefKeyTab.m_Cmds.GetSize() ; n++ ) {
			if ( pMenu->GetMenuString(m_DefKeyTab.m_Cmds[n].m_Id, str, MF_BYCOMMAND) <= 0 )
				continue;

			if ( (a = str.Find(_T('\t'))) >= 0 )
				str.Truncate(a);

			m_DefKeyTab.m_Cmds[n].m_Text = str;
			m_DefKeyTab.m_Cmds[n].SetMenu(pMenu);
		}
	}
	
	// Add Old ServerEntryTab Delete Menu
	if ( (pSubMenu = CMenuLoad::GetItemSubMenu(IDM_PASSWORDLOCK, pMenu)) != NULL ) {
		pSubMenu->DeleteMenu(IDM_DELOLDENTRYTAB, MF_BYCOMMAND);

		if ( ((CRLoginApp *)AfxGetApp())->AliveProfileKeys(_T("ServerEntryTab")) )
			pSubMenu->AppendMenu(MF_STRING, IDM_DELOLDENTRYTAB, CStringLoad(IDS_DELOLDENTRYMENU));
	}

	SetMenuBitmap(pMenu);
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
	
	m_ScrollBarFlag = (m_ScrollBarFlag ? FALSE : TRUE);

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

void CMainFrame::OnViewMenubar()
{
	CWinApp *pApp;
	BOOL bMenu;
	CChildFrame *pChild = (CChildFrame *)MDIGetActive();

	if ( (pApp = AfxGetApp()) == NULL )
		return;
	
	bMenu = (pApp->GetProfileInt(_T("ChildFrame"), _T("VMenu"), TRUE) ? FALSE : TRUE);
	pApp->WriteProfileInt(_T("ChildFrame"), _T("VMenu"), bMenu);

	if ( pChild != NULL ) {
		if ( bMenu ) {
			pChild->OnUpdateFrameMenu(TRUE, pChild, NULL);
			SetMenu(GetMenu());
		} else
			SetMenu(NULL);
	}
}
void CMainFrame::OnUpdateViewMenubar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetMenu() != NULL ? TRUE : FALSE);
}
void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch(nID) {
	case ID_VIEW_MENUBAR:
		OnViewMenubar();
		break;
	default:
		CMDIFrameWnd::OnSysCommand(nID, lParam);
		break;
	}
}
void CMainFrame::OnViewTabbar()
{
	CWinApp *pApp = AfxGetApp();
	
	m_bTabBarShow = (m_bTabBarShow? FALSE : TRUE);
	pApp->WriteProfileInt(_T("MainFrame"), _T("TabBarShow"), m_bTabBarShow);

	if ( m_bTabBarShow )
		ShowControlBar(&m_wndTabBar, m_bTabBarShow, 0);
	else if ( m_wndTabBar.m_TabCtrl.GetItemCount() <= 1 )
		ShowControlBar(&m_wndTabBar, m_bTabBarShow, 0);
}
void CMainFrame::OnUpdateViewTabbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bTabBarShow);
}

void CMainFrame::OnNewVersionFound()
{
	if ( MessageBox(m_VersionMessage, _T("New Version"), MB_ICONQUESTION | MB_YESNO) == IDYES )
		ShellExecute(m_hWnd, NULL, m_VersionPageUrl, NULL, NULL, SW_NORMAL);
}
void CMainFrame::OnVersioncheck()
{
	m_bVersionCheck = (m_bVersionCheck ? FALSE : TRUE);
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("VersionCheckFlag"), m_bVersionCheck);
	AfxGetApp()->WriteProfileString(_T("MainFrame"), _T("VersionNumber"), _T(""));

	if ( m_bVersionCheck )
		VersionCheckProc();
}
void CMainFrame::OnUpdateVersioncheck(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bVersionCheck);
}

void CMainFrame::OnWinodwNext()
{
	if ( m_wndTabBar.GetSize() <= 1 )
		return;
	m_wndTabBar.NextActive();
}
void CMainFrame::OnUpdateWinodwNext(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_wndTabBar.GetSize() > 1 ? TRUE : FALSE);
}
void CMainFrame::OnWindowPrev()
{
	if ( m_wndTabBar.GetSize() <= 1 )
		return;
	m_wndTabBar.PrevActive();
}
void CMainFrame::OnUpdateWindowPrev(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_wndTabBar.GetSize() > 1 ? TRUE : FALSE);
}
void CMainFrame::OnWinodwSelect(UINT nID)
{
	m_wndTabBar.SelectActive(nID - AFX_IDM_FIRST_MDICHILD);
}

void CMainFrame::OnActiveMove(UINT nID)
{
	CWnd *pTemp;
	CPaneFrame *pActive;
	CChildFrame *pWnd;
	CPaneFrame *pPane = NULL;

	if ( m_pTopPane == NULL || (pTemp  = MDIGetActive(NULL)) == NULL || (pActive = m_pTopPane->GetPane(pTemp->m_hWnd)) == NULL )
		return;

	m_pTopPane->GetNextPane(nID - IDM_MOVEPANE_UP, pActive, &pPane);

	if ( pPane != NULL && (pWnd = (CChildFrame *)CWnd::FromHandle(pPane->m_hWnd)) != NULL )
		pWnd->MDIActivate();
}
void CMainFrame::OnUpdateActiveMove(CCmdUI *pCmdUI)
{
	CWnd *pTemp;
	CPaneFrame *pActive;
	CPaneFrame *pPane = NULL;

	if ( m_pTopPane == NULL || m_wndTabBar.GetSize() <= 1 || (pTemp  = MDIGetActive(NULL)) == NULL || (pActive = m_pTopPane->GetPane(pTemp->m_hWnd)) == NULL )
		pCmdUI->Enable(FALSE);
	else {
		m_pTopPane->GetNextPane(pCmdUI->m_nID - IDM_MOVEPANE_UP, pActive, &pPane);
		pCmdUI->Enable(pPane != NULL ? TRUE : FALSE);
	}
}

void CMainFrame::OnBroadcast()
{
	m_bBroadCast = (m_bBroadCast ? FALSE : TRUE);
}
void CMainFrame::OnUpdateBroadcast(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bBroadCast);
}

void CMainFrame::OnDrawClipboard()
{
	CMDIFrameWnd::OnDrawClipboard();

	if ( m_hNextClipWnd && m_hNextClipWnd != m_hWnd )
		::SendMessage(m_hNextClipWnd, WM_DRAWCLIPBOARD, NULL, NULL);

	m_bClipEnable = TRUE;	// クリップボードチェインが有効？

	if ( m_bClipThreadCount < CLIPOPENTHREADMAX  ) {
		m_bClipThreadCount++;
		AfxBeginThread(CopyClipboardThead, this, THREAD_PRIORITY_NORMAL);
	}
}
void CMainFrame::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter)
{
	CMDIFrameWnd::OnChangeCbChain(hWndRemove, hWndAfter);

	if ( hWndRemove == m_hNextClipWnd )
		m_hNextClipWnd = hWndAfter;
	else if ( m_hNextClipWnd && m_hNextClipWnd != m_hWnd )
		::SendMessage(m_hNextClipWnd, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndAfter); 
}
void CMainFrame::OnClipboardUpdate()
{
	m_bClipEnable = TRUE;	// クリップボードチェインが有効？

	// 本来ならここでOpenClipboardなどのクリップボードにアクセスすれば良いと思うのだが
	// リモートディスクトップをRLogin上のポートフォワードで実行するとGetClipboardDataで
	// デッドロックが起こってしまう。

	// その対応で別スレッドでクリップボードのアクセスを行っているがExcel2010などでクリ
	// ップボードのコピーを多数行った場合などにこのOnClipboardUpdateがかなりの頻度で送
	// られるようになりスレッドが重複して起動する

	// OpenClipboardでは、同じウィンドウでのオープンをブロックしないようで妙な動作が確
	// 認できた（Open->Open->Close->Closeで先のCloseで解放され、後のCloseは無視される)
	// GlobalLockしているメモリハンドルがUnlock前に解放される症状が出た

	// メインウィンドウでのアクセスはCMutexLockをOpenClipbardの前に行うよう
	// にした。別スレッドのクリップボードアクセスは、問題が多いと思う

	// かなりややこしい動作なのでここにメモを残す

	if ( m_bClipThreadCount < CLIPOPENTHREADMAX  ) {
		m_bClipThreadCount++;
		AfxBeginThread(CopyClipboardThead, this, THREAD_PRIORITY_NORMAL);
	}
}

void CMainFrame::OnToolcust()
{
	CToolDlg dlg;

	if ( dlg.DoModal() != IDOK )
		return;

	((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_MAINFRAME), m_wndToolBar);

	// ツールバーの再表示
	RecalcLayout(FALSE);
}

void CMainFrame::OnClipchain()
{
	if ( m_bAllowClipChain ) {
		// Do Disable
		m_bAllowClipChain = FALSE;

		if ( m_bClipChain == FALSE ) {
			if ( ExRemoveClipboardFormatListener != NULL )
				ExRemoveClipboardFormatListener(m_hWnd);
		} else {
			if ( m_ClipTimer != 0 ) {
				KillTimer(m_ClipTimer);
				m_ClipTimer = 0;
			}
			ChangeClipboardChain(m_hNextClipWnd);
		}

	} else {
		// Do Enable
		m_bAllowClipChain = TRUE;

		if ( ExAddClipboardFormatListener != NULL && ExRemoveClipboardFormatListener != NULL ) {
			if ( ExAddClipboardFormatListener(m_hWnd) )
				PostMessage(WM_GETCLIPBOARD);
			m_bClipChain = FALSE;
		} else {
			m_hNextClipWnd = SetClipboardViewer();
			m_ClipTimer = SetTimer(TIMERID_CLIPUPDATE, 60000, NULL);
			m_bClipChain = TRUE;
		}
	}

	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("ClipboardChain"), m_bAllowClipChain);
}
void CMainFrame::OnUpdateClipchain(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bAllowClipChain);
}

void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
	CMDIFrameWnd::OnMoving(fwSide, pRect);

	if ( !ExDwmEnable && m_bGlassStyle )
		Invalidate(FALSE);

	if ( m_UseBitmapUpdate ) {
		m_UseBitmapUpdate = FALSE;

		CWinApp *pApp = AfxGetApp();
		POSITION pos = pApp->GetFirstDocTemplatePosition();

		while ( pos != NULL ) {
			CDocTemplate *pDocTemp = pApp->GetNextDocTemplate(pos);
			POSITION dpos = pDocTemp->GetFirstDocPosition();
			while ( dpos != NULL ) {
				CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
				if ( pDoc != NULL && pDoc->m_TextRam.m_BitMapStyle == MAPING_DESKTOP )
					pDoc->UpdateAllViews(NULL, UPDATE_INITPARA, NULL);
			}
		}
	}
}
void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	CRect rect;
	int cx = 200, cy = 200;

	if ( m_hWnd != NULL ) {
		GetWindowRect(rect);

		if ( !m_Frame.IsRectEmpty() && !rect.IsRectEmpty() ) {
			cx = rect.Width()  - m_Frame.Width()  + PANEMIN_WIDTH  * 4;
			cy = rect.Height() - m_Frame.Height() + PANEMIN_HEIGHT * 2;
		}
	}

	lpMMI->ptMinTrackSize.x = cx;
	lpMMI->ptMinTrackSize.y = cy;

	CMDIFrameWnd::OnGetMinMaxInfo(lpMMI);
}
void CMainFrame::OnDeleteOldEntry()
{
	if ( ::AfxMessageBox(IDS_DELOLDENTRYMSG, MB_ICONQUESTION | MB_YESNO) == IDYES )
		((CRLoginApp *)AfxGetApp())->DelProfileSection(_T("ServerEntryTab"));
}
LRESULT CMainFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
	if ( wParam == 0 )
		return CMDIFrameWnd::OnSetMessageString(wParam, lParam);

	int n;
	CStringLoad msg((UINT)wParam);

	if ( (n = msg.Find(_T("\n"))) >= 0 )
		msg.Truncate(n);

	return CMDIFrameWnd::OnSetMessageString(0, (LPARAM)(LPCTSTR)msg);
}

#define _AfxGetDlgCtrlID(hWnd)          ((UINT)(WORD)::GetDlgCtrlID(hWnd))

BOOL CMainFrame::OnToolTipText(UINT nId, NMHDR* pNMHDR, LRESULT* pResult)
{
//	return CMDIFrameWnd::OnToolTipText(nId, pNMHDR, pResult);

	ENSURE_ARG(pNMHDR != NULL);
	ENSURE_ARG(pResult != NULL);
	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
//	TCHAR szFullText[256];
	CStringLoad szFullText;
	CString strTipText;
	UINT_PTR nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = _AfxGetDlgCtrlID((HWND)nID);
	}

	if (nID != 0) // will be zero on a separator
	{
		// don't handle the message if no string resource found
//		if (AfxLoadString((UINT)nID, szFullText) == 0)
		if (szFullText.LoadString((UINT)nID) == 0)
			return FALSE;

		// this is the command id, not the button index
		AfxExtractSubString(strTipText, szFullText, 1, '\n');
	}
#ifndef _UNICODE
	if (pNMHDR->code == TTN_NEEDTEXTA)
		Checked::strncpy_s(pTTTA->szText, _countof(pTTTA->szText), strTipText, _TRUNCATE);
	else
		_mbstowcsz(pTTTW->szText, strTipText, _countof(pTTTW->szText));
#else
	if (pNMHDR->code == TTN_NEEDTEXTA)
		_wcstombsz(pTTTA->szText, strTipText, _countof(pTTTA->szText));
	else
		Checked::wcsncpy_s(pTTTW->szText, _countof(pTTTW->szText), strTipText, _TRUNCATE);
#endif
	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);

	return TRUE;    // message was handled
}
