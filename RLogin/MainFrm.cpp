// MainFrm.cpp : CMainFrame �N���X�̎���
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
#include "richedit.h"
#include "TraceDlg.h"
#include "AnyPastDlg.h"
#include "TtyModeDlg.h"
#include "Fifo.h"
#include "Dbt.h"

#include <afxglobals.h>

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
	m_BoderSize = MulDiv(2, SCREEN_DPI_X, DEFAULT_DPI_X);
	m_bActive = FALSE;
	m_pServerEntry = NULL;
	m_bReqSize = FALSE;
	m_TabIndex = (-1);
	m_bFoucsReq = FALSE;

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
	m_Style = (Style == PANEFRAME_HEDLG ? PANEFRAME_HEIGHT : Style);

	if ( m_NullWnd.m_hWnd != NULL )
		m_NullWnd.DestroyWindow();

	if ( m_bActive )
		m_pLeft->m_bActive = TRUE;
	m_bActive = FALSE;

	switch(Style) {
	case PANEFRAME_WIDTH:
		m_pLeft->m_Frame.right = m_pLeft->m_Frame.left + (m_Frame.Width() - m_BoderSize) / 2;
		m_pRight->m_Frame.left = m_pLeft->m_Frame.right + m_BoderSize;
		break;
	case PANEFRAME_HEIGHT:
		m_pLeft->m_Frame.bottom = m_pLeft->m_Frame.top + (m_Frame.Height() - m_BoderSize) / 2;
		m_pRight->m_Frame.top   = m_pLeft->m_Frame.bottom + m_BoderSize;
		break;
	case PANEFRAME_HEDLG:
		m_pLeft->m_Frame.bottom = m_pLeft->m_Frame.top + (m_Frame.Height() - m_BoderSize) * 3 / 4;
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
		m_pMain->FrameToClient(&rect);
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

		case PANEFRAME_HEDLG:
			if ( m_Style != PANEFRAME_MAXIM )
				goto RECALC;
			m_Style = Style;
			left.bottom = left.top    + (rect.Height() - m_BoderSize) * 3 / 4;
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

	m_pMain->FrameToClient(&rect);
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
			CChildFrame *pActiveChild = (CChildFrame *)(((CMainFrame *)::AfxGetMainWnd())->MDIGetActive());
			CChildFrame *pWnd = (CChildFrame *)(CWnd::FromHandlePermanent(m_hWnd));
			if ( pWnd != NULL ) {
				CRLoginDoc *pDoc = (CRLoginDoc *)(pWnd->GetActiveDocument());
				if ( pDoc != NULL ) {
					// ���݂̃^�C�g����ۑ�
					if ( !pDoc->m_TitleName.IsEmpty() )
						pDoc->m_TextRam.m_TitleName = pDoc->m_TitleName;
					pDoc->SetEntryProBuffer();
					pEntry = &(pDoc->m_ServerEntry);
					pEntry->m_DocType = DOCTYPE_MULTIFILE;
					pDoc->SetModifiedFlag(FALSE);
#if	_MSC_VER >= _MSC_VER_VS10
					pDoc->ClearPathName();
#endif
					tmp.Format("%d\t0\t1\t%d\t%d\t", m_Style, 
						((CMainFrame *)::AfxGetMainWnd())->GetTabIndex(pWnd),
						(pActiveChild != NULL && pActiveChild->m_hWnd == pWnd->m_hWnd ? 1 : 0));
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
	m_pLeft->SetBuffer(buf, bEntry);
	m_pRight->SetBuffer(buf, bEntry);
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
			pPane->m_TabIndex = (stra.GetSize() > 3 ? stra.GetVal(3) : (-1));
			pPane->m_bFoucsReq = (stra.GetSize() > 4 ? (BOOL)stra.GetVal(4) : FALSE);
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
		((CRLoginDoc *)(m_pObject))->OnDelayReceive(m_Id);
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
	case TIMEREVENT_CLOSE:
		((CRLoginDoc *)(m_pObject))->OnSocketClose();
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SYSCOMMAND()
	ON_WM_TIMER()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_COPYDATA()
	ON_WM_ENTERMENULOOP()
	ON_WM_EXITMENULOOP()
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DRAWCLIPBOARD()
	ON_WM_CHANGECBCHAIN()
	ON_WM_CLIPBOARDUPDATE()
	ON_WM_MOVING()
	ON_WM_GETMINMAXINFO()
	ON_WM_INITMENU()
	ON_WM_SETTINGCHANGE()
	ON_WM_NCPAINT()
	ON_WM_NCACTIVATE()

	ON_MESSAGE(WM_ICONMSG, OnIConMsg)
	ON_MESSAGE(WM_THREADCMD, OnThreadMsg)
	ON_MESSAGE(WM_AFTEROPEN, OnAfterOpen)
	ON_MESSAGE(WM_GETCLIPBOARD, OnGetClipboard)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
	ON_MESSAGE(WM_SPEAKMSG, OnSpeakMsg)
	ON_MESSAGE(WM_FIFOMSG, OnFifoMsg)
	ON_MESSAGE(WM_DOCUMENTMSG, OnDocumentMsg)
	ON_MESSAGE(WM_DOCKBARDRAG, OnDockBarDrag)

	ON_COMMAND(ID_FILE_ALL_LOAD, OnFileAllLoad)
	ON_COMMAND(ID_FILE_ALL_SAVE, OnFileAllSave)
	ON_COMMAND(ID_FILE_ALL_LAST, OnFileAllLast)

	ON_COMMAND(ID_PANE_WSPLIT, OnPaneWsplit)
	ON_COMMAND(ID_PANE_HSPLIT, OnPaneHsplit)
	ON_COMMAND(ID_PANE_DELETE, OnPaneDelete)
	ON_COMMAND(ID_PANE_SAVE, OnPaneSave)

	ON_COMMAND(ID_VIEW_MENUBAR, &CMainFrame::OnViewMenubar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MENUBAR, &CMainFrame::OnUpdateViewMenubar)
	ON_COMMAND(ID_VIEW_QUICKBAR, &CMainFrame::OnViewQuickbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_QUICKBAR, &CMainFrame::OnUpdateViewQuickbar)
	ON_COMMAND(ID_VIEW_TABDLGBAR, &CMainFrame::OnViewTabDlgbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TABDLGBAR, &CMainFrame::OnUpdateViewTabDlgbar)
	ON_COMMAND(ID_VIEW_TABBAR, &CMainFrame::OnViewTabbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TABBAR, &CMainFrame::OnUpdateViewTabbar)
	ON_COMMAND(ID_VIEW_SCROLLBAR, &CMainFrame::OnViewScrollbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SCROLLBAR, &CMainFrame::OnUpdateViewScrollbar)
	ON_COMMAND(IDM_HISTORYDLG, &CMainFrame::OnViewHistoryDlg)
	ON_UPDATE_COMMAND_UI(IDM_HISTORYDLG, &CMainFrame::OnUpdateHistoryDlg)
	ON_COMMAND(ID_VIEW_VOICEBAR, &CMainFrame::OnViewVoicebar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VOICEBAR, &CMainFrame::OnUpdateViewVoicebar)
	ON_COMMAND(IDM_VIEW_SUBTOOL, &CMainFrame::OnViewSubToolbar)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_SUBTOOL, &CMainFrame::OnUpdateSubToolbar)

	ON_COMMAND(IDM_DOCKBARFIXED, &CMainFrame::OnDockBarFixed)
	ON_UPDATE_COMMAND_UI(IDM_DOCKBARFIXED, &CMainFrame::OnUpdateDockBarFixed)
	ON_COMMAND(IDM_DOCKBARINIT, &CMainFrame::OnDockBarInit)
	ON_UPDATE_COMMAND_UI(IDM_DOCKBARINIT, &CMainFrame::OnUpdateDockBarInit)

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

	ON_COMMAND(IDM_TABMULTILINE, &CMainFrame::OnTabmultiline)
	ON_UPDATE_COMMAND_UI(IDM_TABMULTILINE, &CMainFrame::OnUpdateTabmultiline)

	ON_COMMAND(IDM_QUICKCONNECT, &CMainFrame::OnQuickConnect)
	ON_UPDATE_COMMAND_UI(IDC_CONNECT, &CMainFrame::OnUpdateConnect)
	ON_BN_CLICKED(IDC_CONNECT, &CMainFrame::OnQuickConnect)

	ON_COMMAND(IDM_SPEAKALL, &CMainFrame::OnSpeakText)
	ON_UPDATE_COMMAND_UI(IDM_SPEAKALL, &CMainFrame::OnUpdateSpeakText)
	ON_BN_CLICKED(IDC_PLAYSTOP, &CMainFrame::OnSpeakText)

	ON_COMMAND(IDM_SPEAKBACK, &CMainFrame::OnSpeakBack)
	ON_UPDATE_COMMAND_UI(IDM_SPEAKBACK, &CMainFrame::OnUpdateSpeakText)
	ON_BN_CLICKED(IDC_BACKPOS, &CMainFrame::OnSpeakBack)

	ON_COMMAND(IDM_SPEAKNEXT, &CMainFrame::OnSpeakNext)
	ON_UPDATE_COMMAND_UI(IDM_SPEAKNEXT, &CMainFrame::OnUpdateSpeakText)
	ON_BN_CLICKED(IDC_NEXTPOS, &CMainFrame::OnSpeakNext)

	ON_COMMAND(IDM_APPCOLEDIT, &CMainFrame::OnAppcoledit)
	ON_COMMAND(IDM_KNOWNHOSTDEL, &CMainFrame::OnKnownhostdel)

	ON_COMMAND(IDM_CHARTOOLTIP, &CMainFrame::OnChartooltip)
	ON_UPDATE_COMMAND_UI(IDM_CHARTOOLTIP, &CMainFrame::OnUpdateChartooltip)

	ON_MESSAGE(WM_UAHDRAWMENU, OnUahDrawMenu)
	ON_MESSAGE(WM_UAHDRAWMENUITEM, OnUahDrawMenuItem)

	ON_MESSAGE(WM_DRAWEMOJI, OnUpdateEmoji)

	END_MESSAGE_MAP()

static const UINT indicators[] =
{
	ID_SEPARATOR,           // �X�e�[�^�X ���C�� �C���W�P�[�^
	ID_INDICATOR_SOCK,
	ID_INDICATOR_STAT,
	ID_INDICATOR_KMOD,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
//	ID_INDICATOR_SCRL,
//	ID_INDICATOR_KANA,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame �R���X�g���N�V����/�f�X�g���N�V����

CMainFrame::CMainFrame()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIconActive = AfxGetApp()->LoadIcon(IDI_ACTIVE);
	m_IconShow = FALSE;
	ZeroMemory(&m_IconData, sizeof(m_IconData));
	m_pTopPane = NULL;
	m_Frame.SetRectEmpty();
	m_pTrackPane = NULL;
	m_LastPaneFlag = FALSE;
	m_pLastPaneFoucs = NULL;
	m_TimerSeqId = TIMERID_TIMEREVENT;
	m_pTimerUsedId = NULL;
	m_pTimerFreeId = NULL;
	m_SleepStatus = 0;
	m_SleepTimer = 0;
	m_TransParValue = 255;
	m_TransParColor = RGB(0, 0, 0);
	m_SleepCount = 60;
	m_MidiTimer = 0;
	m_InfoThreadCount = 0;
	m_SplitType = PANEFRAME_WSPLIT;
	m_ExecCount = 0;
	m_StartMenuHand = NULL;
	m_bVersionCheck = FALSE;
	m_hNextClipWnd = NULL;
	m_ScrollBarFlag = FALSE;
	m_bBroadCast = FALSE;
	m_bMenuBarShow = FALSE;
	m_bTabBarShow = FALSE;
	m_bTabDlgBarShow = FALSE;
	m_bQuickConnect = FALSE;
	m_StatusTimer = 0;
	m_bAllowClipChain = TRUE;
	m_bClipEnable = FALSE;
	m_bClipChain = FALSE;
	m_ScreenDpiX = m_ScreenDpiY = 96;
	m_ScreenX = m_ScreenY = m_ScreenW = m_ScreenH = 0;
	m_bGlassStyle = FALSE;
	m_UseBitmapUpdate = FALSE;
	m_bClipThreadCount = 0;
	m_ClipTimer = 0;
	m_IdleTimer = 0;
	m_LastClipUpdate = clock();
	m_pMidiData = NULL;
	m_pServerSelect = NULL;
	m_pHistoryDlg = NULL;
	m_bVoiceEvent = FALSE;
	m_pAnyPastDlg = NULL;
	m_PastNoCheck = FALSE;
	m_pTaskbarList = NULL;
	m_bCharTooltip = FALSE;
	m_ImageSize.cx = m_ImageSize.cy = 16;
	m_bDarkMode = FALSE;
	m_bDockBarMode = FALSE;
	m_SpeakQueLen = m_SpeakQuePos = m_SpeakQueTop = 0;
	m_pSpeakView = NULL;
	m_pSpeakDoc = NULL;
	m_SpeakCols = m_SpeakLine = m_SpeakAbs = 0;
	m_SpeakActive[0] = m_SpeakActive[1] = m_SpeakActive[2] = 0;
	for ( int n = 0 ; n < SPEAKQUESIZE ; n++ ) {
		m_SpeakData[n].num = 0;
		m_SpeakData[n].skip = 0;
		m_SpeakData[n].abs = 0;
		m_SpeakData[n].line = 0;
	}
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

	if ( m_pMidiData != NULL )
		delete m_pMidiData;

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

/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK OwnerClientProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if ( uMsg == WM_ERASEBKGND ) {
		CWnd *pWnd = CWnd::FromHandle(hWnd);
		CDC *pDC = CDC::FromHandle((HDC)wParam);
		CRect rect;
		pWnd->GetClientRect(rect);
		pDC->FillSolidRect(rect, GetAppColor(COLOR_APPWORKSPACE));
		return TRUE;

	} else if ( uMsg == WM_NCPAINT || uMsg == WM_NCACTIVATE ) {
		CWnd *pWnd = CWnd::FromHandle(hWnd);
		CDC *pDC = pWnd->GetWindowDC();
		CRect rect;
		pWnd->GetWindowRect(rect);
		rect.SetRect(0, 0, rect.Width(), rect.Height());
		pDC->Draw3dRect(rect, GetAppColor(COLOR_BTNSHADOW), GetAppColor(COLOR_BTNHIGHLIGHT));
		rect.InflateRect(-AFX_CX_BORDER, -AFX_CY_BORDER);
		pDC->Draw3dRect(rect, GetAppColor(COLOR_WINDOWFRAME), GetAppColor(COLOR_BTNFACE));
		pWnd->ReleaseDC(pDC);
		return TRUE;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int n;
	CBitmapEx BitMap;
	CBuffer buf;
	CMenu *pMenu;
	UINT nID, nSt;
	BITMAP mapinfo;
	DWORD addStyle = 0;

	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if ( ExEnableNonClientDpiScaling != NULL )
		ExEnableNonClientDpiScaling(GetSafeHwnd());

	SetWindowSubclass(m_hWndMDIClient, OwnerClientProc, (UINT_PTR)NULL, (DWORD_PTR)this);

	// �L�����N�^�[�r�b�g�}�b�v�̓ǂݍ���
#if		USE_GOZI == 1 || USE_GOZI == 2
	((CRLoginApp *)::AfxGetApp())->LoadResBitmap(MAKEINTRESOURCE(IDB_BITMAP8), BitMap);
	m_ImageGozi.Create(32, 32, ILC_COLOR24 | ILC_MASK, 28, 10);
	m_ImageGozi.Add(&BitMap, RGB(192, 192, 192));
	BitMap.DeleteObject();
#elif	USE_GOZI == 3
	BitMap.LoadResBitmap(IDB_BITMAP8, SYSTEM_DPI_X, SYSTEM_DPI_Y, RGB(255, 255, 255));
	BitMap.GetBitmap(&mapinfo);
	m_ImageSize.cx = mapinfo.bmHeight;
	m_ImageSize.cy = mapinfo.bmHeight;
	m_ImageGozi.Create(m_ImageSize.cx, m_ImageSize.cy, ILC_COLOR24 | ILC_MASK, mapinfo.bmWidth / m_ImageSize.cx, 1);
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

	// ���j���[�摜���쐬
	InitMenuBitmap();

	if ( !(m_bDockBarMode = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("DockBarMode"), FALSE)) )
		addStyle |= CBRS_GRIPPER;

	// �c�[���E�X�e�[�^�X�E�^�u�@�o�[�̍쐬
	if ( !m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT,
			WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | addStyle | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_MAINFRAME), m_wndToolBar, this) ) {
		TRACE0("Failed to create toolbar\n");
		return -1;      // �쐬�Ɏ��s
	}

	if ( !m_wndSubToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT,
			WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | addStyle | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0), IDR_TOOLBAR2) ||
		!((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_TOOLBAR2), m_wndSubToolBar, this) ) {
		TRACE0("Failed to create subtoolbar\n");
		return -1;      // �쐬�Ɏ��s
	}

	if ( !m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)) ) {
		TRACE0("Failed to create status bar\n");
		return -1;      // �쐬�Ɏ��s
	}

	if ( !m_wndTabBar.Create(this, WS_VISIBLE| WS_CHILD | CBRS_ALIGN_TOP | addStyle, IDC_MDI_TAB_CTRL_BAR) ) {
		TRACE("Failed to create tabbar\n");
		return -1;      // fail to create
	}

	if ( !m_wndQuickBar.Create(this, IDD_QUICKBAR, WS_VISIBLE | WS_CHILD | CBRS_ALIGN_TOP | addStyle, IDD_QUICKBAR) ) {
		TRACE("Failed to create quickbar\n");
		return -1;      // fail to create
	}

	if ( !m_wndTabDlgBar.Create(this, WS_VISIBLE| WS_CHILD | CBRS_ALIGN_BOTTOM | addStyle, IDC_TABDLGBAR) ) {
		TRACE("Failed to create tabdlgbar\n");
		return -1;      // fail to create
	}

	if ( !m_wndVoiceBar.Create(this, IDD_VOICEBAR, WS_VISIBLE | WS_CHILD | CBRS_ALIGN_TOP | addStyle, IDD_VOICEBAR) ) {
		TRACE("Failed to create voicebar\n");
		return -1;      // fail to create
	}

	m_wndStatusBar.GetPaneInfo(0, nID, nSt, n);
	m_wndStatusBar.SetPaneInfo(0, nID, nSt, 160);

	CDockContextEx::EnableDocking(&m_wndToolBar, CBRS_ALIGN_ANY);
	CDockContextEx::EnableDocking(&m_wndSubToolBar, CBRS_ALIGN_ANY);
	CDockContextEx::EnableDocking(&m_wndTabBar, CBRS_ALIGN_ANY); // CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);
	CDockContextEx::EnableDocking(&m_wndQuickBar, CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);
	CDockContextEx::EnableDocking(&m_wndTabDlgBar, CBRS_ALIGN_ANY);
	CDockContextEx::EnableDocking(&m_wndVoiceBar, CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

	// CDockBarEx���ɓo�^
	for ( int i = 0 ; i < 4 ; i++ ) {
		CDockBarEx *pDock = new CDockBarEx;
		pDock->Create(this, WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE | dwDockBarMap[i][1], dwDockBarMap[i][0]);
	}

	EnableDocking(CBRS_ALIGN_ANY);
	m_pFloatingFrameClass = RUNTIME_CLASS(CMiniDockFrameWndEx);

#if 0
	// Theme�̃e�X�g
	m_wndToolBar.m_hReBarTheme = ExOpenThemeData(m_wndToolBar.GetSafeHwnd(), L"TOOLBAR");
	m_wndSubToolBar.m_hReBarTheme = ExOpenThemeData(m_wndSubToolBar.GetSafeHwnd(), L"TOOLBAR");
	m_wndStatusBar.m_hReBarTheme = ExOpenThemeData(m_wndStatusBar.GetSafeHwnd(), L"STATUS");
#endif

	DockControlBar(&m_wndToolBar);
	DockControlBar(&m_wndSubToolBar);
	DockControlBar(&m_wndQuickBar);
	DockControlBar(&m_wndVoiceBar);
	DockControlBar(&m_wndTabBar);
	DockControlBar(&m_wndTabDlgBar);

	// �o�[�̕\���ݒ�
	LoadBarState(_T("BarState"));

	m_bMenuBarShow = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("VMenu"), TRUE);

	if ( (AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("ToolBarStyle"), WS_VISIBLE) & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndToolBar, FALSE, FALSE);

	if ( (AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("SubToolBarStyle"), 0) & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndSubToolBar, FALSE, FALSE);

	if ( (AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("StatusBarStyle"), WS_VISIBLE) & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndStatusBar, FALSE, FALSE);

	if ( AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("QuickBarShow"), FALSE) == FALSE )
		ShowControlBar(&m_wndQuickBar, FALSE, FALSE);

	if ( AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("VoiceBarShow"), FALSE) == FALSE )
		ShowControlBar(&m_wndVoiceBar, FALSE, FALSE);

	m_bTabBarShow = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("TabBarShow"), FALSE);
	ShowControlBar(&m_wndTabBar, m_bTabBarShow, FALSE);

	m_bTabDlgBarShow = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("TabDlgBarShow"), FALSE);
	ShowControlBar(&m_wndTabDlgBar, FALSE, FALSE);

	// ������ʂ̐ݒ�
	m_TransParValue = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("LayeredWindow"), 255);
	m_TransParColor = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("LayeredColor"), RGB(0, 0 ,0));
	SetTransPar(m_TransParColor, m_TransParValue, LWA_ALPHA | LWA_COLORKEY);

	if ( (m_SleepCount = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("WakeUpSleep"), 0)) > 0 )
		m_SleepTimer = SetTimer(TIMERID_SLEEPMODE, 5000, NULL);

	m_ScrollBarFlag = AfxGetApp()->GetProfileInt(_T("ChildFrame"), _T("VScroll"), TRUE);
	m_bVersionCheck = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("VersionCheckFlag"), FALSE);

	m_bGlassStyle = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("GlassStyle"), FALSE);
	ExDwmEnableWindow(m_hWnd, m_bGlassStyle);

	m_bCharTooltip = AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("CharTooltip"), FALSE);

	// ��ʕ����𕜋A
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

	// ���j���[�̏�����
	if ( (pMenu = GetSystemMenu(FALSE)) != NULL ) {
		pMenu->InsertMenu(0, MF_BYPOSITION | MF_SEPARATOR);
		pMenu->InsertMenu(0, MF_BYPOSITION | MF_STRING, ID_VIEW_MENUBAR, CStringLoad(IDS_VIEW_MENUBAR));
	}

	if ( (pMenu = GetMenu()) != NULL ) {
		m_StartMenuHand = pMenu->GetSafeHmenu();
		SetMenuBitmap(pMenu);
	}

	// �N���b�v�{�[�h�`�F�C���̐ݒ�
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

	// �W���̐ݒ�̃L�[�ݒ��ǂݍ��݁E������
	m_DefKeyTab.Serialize(FALSE);
	m_DefKeyTab.CmdsInit();

	// �q�X�g���[�E�B���h�E�𕜋A
	if ( m_bTabDlgBarShow && AfxGetApp()->GetProfileInt(_T("MainFrame"), _T("HistoryDlg"), FALSE) )
		PostMessage(WM_COMMAND, IDM_HISTORYDLG);

	m_bDarkMode = ExDwmDarkMode(GetSafeHwnd());

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

	// ���j�^�[�̕\���͈̓`�F�b�N
	HMONITOR hMonitor;
    MONITORINFOEX  mi;
	CRect rc(cs.x, cs.y, cs.x + cs.cx, cs.y + cs.cy);

	hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &mi);

	// ���j�^�[����ɒ���
	if ( (mi.dwFlags & MONITORINFOF_PRIMARY) != 0 ) {
		CRect work;
		SystemParametersInfo(SPI_GETWORKAREA, 0, work, 0);
		mi.rcMonitor = work;
	}

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

	// ���j�^�[DPI���擾
	if ( ExGetDpiForMonitor != NULL )
		ExGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &m_ScreenDpiX, &m_ScreenDpiY);

	// ���j���[�����\�[�X�f�[�^�x�[�X�ɒu������
	if ( cs.hMenu != NULL ) {
		DestroyMenu(cs.hMenu);
		((CRLoginApp *)::AfxGetApp())->LoadResMenu(MAKEINTRESOURCE(IDR_MAINFRAME), cs.hMenu);
	}
	
	//cs.style &= ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME);
	//cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

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

#define AGENT_WSSH_PIPE_NAME	L"\\\\.\\pipe\\openssh-ssh-agent"
#define AGENT_PUTTY_PIPE_NAME	PagentPipeName()
#define AGENT_COPYDATA_ID		0x804e50ba   /* random goop */
#define AGENT_MAX_MSGLEN		(16 * 1024)

BOOL CMainFrame::WageantQuery(CBuffer *pInBuf, CBuffer *pOutBuf, LPCTSTR pipename)
{
	int n, len;
	HANDLE hPipe;
	BOOL bRet = FALSE;
	LPBYTE pBuffer;
	DWORD BufLen;
	DWORD writeByte;
	BYTE readBuffer[4096];
	DWORD readByte = 0;

	if ( (hPipe = CreateFile(pipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE )
		return bRet;

	pBuffer = pInBuf->GetPtr();
	BufLen  = pInBuf->GetSize();

	while ( BufLen > 0 ) {
		if ( !WriteFile(hPipe, pBuffer, BufLen, &writeByte, NULL) )
			goto ENDOFRET;
		pBuffer += writeByte;
		BufLen  -= writeByte;
	}

	pOutBuf->Clear();

	if ( ReadFile(hPipe, readBuffer, 4, &readByte, NULL) && readByte == 4 ) {
		len = (readBuffer[0] << 24) | (readBuffer[1] << 16) | (readBuffer[2] << 8) | (readBuffer[3]);
		if ( len > 0 && len < AGENT_MAX_MSGLEN ) {
			for ( n = 0 ; n < len ; ) {
				if ( !ReadFile(hPipe, readBuffer, 4096, &readByte, NULL) ) {
					pOutBuf->Clear();
					break;
				}
				pOutBuf->Apend(readBuffer, readByte);
				n += readByte;
			}
		}
	}

	if ( pOutBuf->GetSize() > 0 )
		bRet = TRUE;

ENDOFRET:
	CloseHandle(hPipe);
	SecureZeroMemory(readBuffer, sizeof(readBuffer));
	return bRet;
}

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

	mbs = TstrToMbs(mapname);
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

LPCTSTR CMainFrame::PagentPipeName()
{
	static BOOL bInit = FALSE;
	static CString pipename;

	if ( bInit )
		return pipename;

	HMODULE hCrypt32;
	BOOL (__stdcall *pCryptProtectMemory)(LPVOID pDataIn, DWORD cbDataIn, DWORD dwFlags);
	char data[CRYPTPROTECTMEMORY_BLOCK_SIZE];
	int len = CRYPTPROTECTMEMORY_BLOCK_SIZE;	//	"Pageant" len = 7, CRYPTPROTECTMEMORY_BLOCK_SIZE = 16, (7 + 1 + 15) / 16 * 16 = 16
	DWORD size;
	TCHAR username[MAX_COMPUTERNAME];
	int dlen;
	u_char digest[EVP_MAX_MD_SIZE];
	CBuffer tmp(-1);

	bInit = TRUE;

	size = MAX_COMPUTERNAME;
	ZeroMemory(username, sizeof(username));
	GetUserName(username, &size);

	ZeroMemory(data, CRYPTPROTECTMEMORY_BLOCK_SIZE);
	strcpy(data, "Pageant");

	if ( (hCrypt32 = LoadLibrary(_T("crypt32.dll"))) != NULL ) {
		if ( (pCryptProtectMemory = (BOOL (__stdcall *)(LPVOID pDataIn, DWORD cbDataIn, DWORD dwFlags))GetProcAddress(hCrypt32, "CryptProtectMemory")) != NULL )
			pCryptProtectMemory(data, len, CRYPTPROTECTMEMORY_CROSS_PROCESS);
		FreeLibrary(hCrypt32);
	}

	tmp.PutBuf((LPBYTE)data, len);
	dlen = MD_digest(EVP_sha256(), tmp.GetPtr(), tmp.GetSize(), digest, sizeof(digest));
	tmp.Base16Encode(digest, dlen);

	pipename.Format(_T("\\\\.\\pipe\\pageant.%s.%s"), username, (LPCTSTR)tmp);

	SecureZeroMemory(data, sizeof(data));
	SecureZeroMemory(digest, sizeof(digest));

	return pipename;
}

BOOL CMainFrame::AgeantInit()
{
	int n, i, mx;
	int ctype, type;
	int count = 0;
	CBuffer in, out;
	CIdKey key;
	CBuffer blob;
	CStringA name;

	for ( i = 0 ; i < m_IdKeyTab.GetSize() ; i++ ) {
		if ( m_IdKeyTab[i].m_AgeantType != IDKEY_AGEANT_NONE )
			m_IdKeyTab[i].m_bSecInit = FALSE;
	}

	in.Put32Bit(1);
	in.Put8Bit(SSH_AGENTC_REQUEST_IDENTITIES);

	for ( type = IDKEY_AGEANT_PUTTY ; type <= IDKEY_AGEANT_WINSSH ; type++ ) {

		if ( type == IDKEY_AGEANT_PUTTY ) {
			if ( PageantQuery(&in, &out) )
				ctype = IDKEY_AGEANT_PUTTY;
			else if ( WageantQuery(&in, &out, AGENT_PUTTY_PIPE_NAME) )
				ctype = IDKEY_AGEANT_PUTTYPIPE;
			else
				continue;
		} else if ( type == IDKEY_AGEANT_WINSSH ) {
			if ( !WageantQuery(&in, &out, AGENT_WSSH_PIPE_NAME) )
				continue;
			ctype = type;
		}

		if ( out.GetSize() < 5 || out.Get8Bit() != SSH_AGENT_IDENTITIES_ANSWER )
			continue;

		try {
			mx = out.Get32Bit();
			for ( n = 0 ; n < mx ; n++ ) {
				out.GetBuf(&blob);
				out.GetStr(name);
				key.m_Name = MbsToTstr(name);
				key.m_bSecInit = TRUE;
				key.m_AgeantType = ctype;
				if ( !key.GetBlob(&blob) )
					continue;

				for ( i = 0 ; i < m_IdKeyTab.GetSize() ; i++ ) {
					if ( m_IdKeyTab[i].m_AgeantType == ctype && m_IdKeyTab[i].ComperePublic(&key) == 0 ) {
						m_IdKeyTab[i].m_bSecInit = TRUE;
						count++;
						break;
					}
				}

				if ( i >= m_IdKeyTab.GetSize() ) {
					m_IdKeyTab.AddEntry(key, FALSE);
					count++;
				}
			}
#ifdef	DEBUG
		} catch(LPCTSTR msg) {
			TRACE(_T("AgeantInit Error %s '%s'\n"), MbsToTstr(name), msg);
#endif
		} catch(...) {
		}
	}

	return (count > 0 ? TRUE : FALSE);
}
BOOL CMainFrame::AgeantSign(int type, CBuffer *blob, CBuffer *sign, LPBYTE buf, int len, int flag)
{
	CBuffer in(-1), out(-1), work(-1);

	work.Put8Bit(SSH_AGENTC_SIGN_REQUEST);
	work.PutBuf(blob->GetPtr(), blob->GetSize());
	work.PutBuf(buf, len);
	work.Put32Bit(flag);
	
	in.PutBuf(work.GetPtr(), work.GetSize());

	if ( type == IDKEY_AGEANT_PUTTY ) {
		if ( !PageantQuery(&in, &out) )
			return FALSE;
	} else if ( type == IDKEY_AGEANT_WINSSH ) {
		if ( !WageantQuery(&in, &out, AGENT_WSSH_PIPE_NAME) )
			return FALSE;
	} else if ( type == IDKEY_AGEANT_PUTTYPIPE ) {
		if ( !WageantQuery(&in, &out, AGENT_PUTTY_PIPE_NAME) )
			return FALSE;
	} else
		return FALSE;

	if ( out.GetSize() < 5 || out.Get8Bit() != SSH_AGENT_SIGN_RESPONSE )
		return FALSE;

	sign->Clear();
	out.GetBuf(sign);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

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
void CMainFrame::DelTimerEvent(void *pParam, int Id)
{
	CTimerObject *tp, *bp;

	for ( tp = bp = m_pTimerUsedId ; tp != NULL ; ) {
		if ( tp->m_pObject == pParam && (Id == 0 || tp->m_Id == Id) ) {
			KillTimer(tp->m_Id);
			if ( tp == m_pTimerUsedId )
				m_pTimerUsedId = tp->m_pList;
			else
				bp->m_pList = tp->m_pList;
			FreeTimerEvent(tp);
			tp = bp->m_pList;
		} else {
			bp = tp;
			tp = tp->m_pList;
		}
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

void CMainFrame::SetMidiData(int nInit, int nPlay, LPCSTR mml)
{
	if ( m_pMidiData == NULL )
		m_pMidiData = new CMidiData(this);

	if ( m_pMidiData->m_hStream == NULL )
		return;

	m_pMidiData->LoadMML(mml, nInit);

	switch(nPlay) {
	case 0:
		m_pMidiData->Play();
		break;
	case 1:
		m_pMidiData->Stop();
		break;
	case 2:
		m_pMidiData->Pause();
		break;
	}
}
void CMainFrame::SetMidiEvent(int msec, DWORD msg)
{
	CMidiQue *qp;

	if ( m_pMidiData == NULL )
		m_pMidiData = new CMidiData(this);

	if ( m_pMidiData->m_hStream == NULL )
		return;

	if ( m_MidiQue.IsEmpty() && msec == 0 ) {
		midiOutShortMsg((HMIDIOUT)m_pMidiData->m_hStream, msg);
		return;
	}

	qp = new CMidiQue(msec, msg);

	m_MidiQue.AddTail(qp);
	qp = m_MidiQue.GetHead();

	if ( m_MidiTimer == 0 )
		m_MidiTimer = SetTimer(TIMERID_MIDIEVENT, qp->m_mSec, NULL);
}
void CMainFrame::SetIdleTimer(BOOL bSw)
{
	if ( bSw ) {
		if ( m_IdleTimer == 0 )
			m_IdleTimer = SetTimer(TIMERID_IDLETIMER, 100, NULL);

	} else if ( m_IdleTimer != 0 ) {
		KillTimer(m_IdleTimer);
		m_IdleTimer = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateServerEntry()
{
	m_ServerEntryTab.Serialize(FALSE);
	QuickBarInit();

	if ( m_pServerSelect != NULL )
		m_pServerSelect->PostMessage(WM_COMMAND, ID_EDIT_CHECK);
}
int CMainFrame::OpenServerEntry(CServerEntry &Entry)
{
	int n;
	INT_PTR retId;
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
		if ( pPane->m_bFoucsReq )
			m_pLastPaneFoucs = pTemp;
		if ( m_pTopPane->GetEntry() != NULL )
			PostMessage(WM_COMMAND, ID_FILE_NEW, 0);
		else {
			m_LastPaneFlag = FALSE;
			if ( m_pLastPaneFoucs != NULL )
				PostMessage(WM_COMMAND, ID_FILE_ALL_LAST, 0);
		}
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
				Entry.m_EntryName.Format(_T("%s:%s"), (pApp->m_pCmdInfo->m_Addr.IsEmpty() ? _T("unkown") : (LPCTSTR)pApp->m_pCmdInfo->m_Addr), (LPCTSTR)pApp->m_pCmdInfo->m_Port);
			Entry.m_DocType = DOCTYPE_SESSION;
			return TRUE;
		}

		m_pServerSelect = &dlg;
		retId = dlg.DoModal();
		m_pServerSelect = NULL;

		if ( retId != IDOK || dlg.m_EntryNum < 0 )
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

void CMainFrame::AddHistory(void *pCmdHis)
{
	if ( m_pHistoryDlg != NULL && m_pHistoryDlg->GetSafeHwnd() != NULL )
		m_pHistoryDlg->PostMessage(WM_ADDCMDHIS, (WPARAM)0, (LPARAM)pCmdHis);
}

void CMainFrame::AddTabDlg(CWnd *pWnd, int nImage, CPoint point)
{
	if ( !m_bTabDlgBarShow )
		return;

	CRect rect;
	m_wndTabDlgBar.GetWindowRect(rect);

	if ( rect.PtInRect(point) )
		AddTabDlg(pWnd, nImage);
}
void CMainFrame::AddTabDlg(CWnd *pWnd, int nImage)
{
	if ( !m_bTabDlgBarShow )
		return;

	m_wndTabDlgBar.Add(pWnd, nImage);

	if ( m_wndTabDlgBar.m_TabCtrl.GetItemCount() > 0 && (m_wndTabDlgBar.GetStyle() & WS_VISIBLE) == 0 )
		ShowControlBar(&m_wndTabDlgBar, TRUE, TRUE);

	SetFocus();
}
void CMainFrame::DelTabDlg(CWnd *pWnd)
{
	if ( !m_bTabDlgBarShow )
		return;

	m_wndTabDlgBar.Del(pWnd);

	if ( m_wndTabDlgBar.m_TabCtrl.GetItemCount() <=0 )
		ShowControlBar(&m_wndTabDlgBar, FALSE, TRUE);
}
void CMainFrame::SelTabDlg(CWnd *pWnd)
{
	if ( !m_bTabDlgBarShow )
		return;

	m_wndTabDlgBar.Sel(pWnd);
}
BOOL CMainFrame::IsInsideDlg(CWnd *pWnd)
{
	if ( !m_bTabDlgBarShow )
		return FALSE;

	return m_wndTabDlgBar.IsInside(pWnd);
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

	CPaneFrame *pPane;

	if ( m_pTopPane == NULL ) {
		pPane = m_pTopPane = new CPaneFrame(this, pWnd->m_hWnd, NULL);
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

	m_wndTabBar.Add(pWnd, pPane->m_TabIndex);
	pPane->m_TabIndex = (-1);
}
void CMainFrame::RemoveChild(CWnd *pWnd, BOOL bDelete)
{
	m_wndTabBar.Remove(pWnd);
	if ( m_wndTabBar.m_TabCtrl.GetItemCount() <= 1 )
		ShowControlBar(&m_wndTabBar, m_bTabBarShow, TRUE);

	if ( m_pTopPane == NULL )
		return;

	CPaneFrame *pPane = m_pTopPane->GetPane(pWnd->m_hWnd);

	if ( pPane == NULL )
		return;

	if ( bDelete || (pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM) ) {
		m_pTopPane = m_pTopPane->DeletePane(pWnd->m_hWnd);
	} else {
		pPane->m_hWnd = NULL;
		pPane->MoveFrame();
	}
}
void CMainFrame::ActiveChild(class CChildFrame *pWnd)
{
	if ( m_pTopPane == NULL )
		return;

	m_pTopPane->SetActive(pWnd->GetSafeHwnd());

	CRLoginView *pView;
	CRLoginDoc *pDoc;
	
	if ( m_bTabDlgBarShow && m_wndTabDlgBar.m_pShowWnd != NULL && m_wndTabDlgBar.m_pShowWnd != m_pHistoryDlg &&
			(pView = (CRLoginView *)pWnd->GetActiveView()) != NULL && (pDoc = pView->GetDocument()) != NULL ) {
		if ( m_wndTabDlgBar.m_pShowWnd != pDoc->m_TextRam.m_pCmdHisWnd && m_wndTabDlgBar.m_pShowWnd != pDoc->m_TextRam.m_pTraceWnd ) {
			if ( pDoc->m_TextRam.m_pCmdHisWnd != NULL )
				SelTabDlg(pDoc->m_TextRam.m_pCmdHisWnd);
			else if ( pDoc->m_TextRam.m_pTraceWnd != NULL )
				SelTabDlg(pDoc->m_TextRam.m_pTraceWnd);
		}
	}
}
CPaneFrame *CMainFrame::GetWindowPanePoint(CPoint point)
{
	if ( m_pTopPane == NULL )
		return NULL;

	ClientToFrame(&point);

	CPaneFrame *pPane = m_pTopPane->HitTest(point);

	if ( pPane == NULL || pPane->m_Style != PANEFRAME_WINDOW )
		return NULL;

	return pPane;
}
void CMainFrame::MoveChild(CWnd *pWnd, CPoint point)
{
	if ( m_pTopPane == NULL )
		return;

	ClientToFrame(&point);

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

void CMainFrame::GetCtrlBarRect(LPRECT rect, CControlBar *pCtrl)
{
	ShowControlBar(pCtrl, FALSE, TRUE);
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, rect);
	ShowControlBar(pCtrl, TRUE, TRUE);
}
void CMainFrame::GetFrameRect(CRect &frame)
{
	if ( m_Frame.left >= m_Frame.right && m_Frame.top >= m_Frame.bottom )
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);

	frame.SetRect(0, 0, m_Frame.Width(), m_Frame.Height());
}
void CMainFrame::FrameToClient(LPRECT lpRect)
{
	if ( m_Frame.left >= m_Frame.right && m_Frame.top >= m_Frame.bottom )
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);

	// FrameRect -> ClientRect
	lpRect->left   += m_Frame.left;
	lpRect->right  += m_Frame.left;
	lpRect->top    += m_Frame.top;
	lpRect->bottom += m_Frame.top;
}
void CMainFrame::FrameToClient(LPPOINT lpPoint)
{
	if ( m_Frame.left >= m_Frame.right && m_Frame.top >= m_Frame.bottom )
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);

	// FrameRect -> ClientRect
	lpPoint->x += m_Frame.left;
	lpPoint->y += m_Frame.top;
}
void CMainFrame::ClientToFrame(LPRECT lpRect)
{
	if ( m_Frame.left >= m_Frame.right && m_Frame.top >= m_Frame.bottom )
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);

	// ClientRect -> FrameRect
	lpRect->left   -= m_Frame.left;
	lpRect->right  -= m_Frame.left;
	lpRect->top    -= m_Frame.top;
	lpRect->bottom -= m_Frame.top;
}
void CMainFrame::ClientToFrame(LPPOINT lpPoint)
{
	if ( m_Frame.left >= m_Frame.right && m_Frame.top >= m_Frame.bottom )
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);

	// ClientRect -> FrameRect
	lpPoint->x -= m_Frame.left;
	lpPoint->y -= m_Frame.top;
}
void CMainFrame::RecalcLayout(BOOL bNotify) 
{
	CMDIFrameWnd::RecalcLayout(bNotify);

	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &m_Frame);

	if ( m_pTopPane != NULL ) {
		CRect rect(0, 0, m_Frame.Width(), m_Frame.Height());
		m_pTopPane->MoveParOwn(rect, PANEFRAME_NOCHNG);
	}
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
	ClientToFrame(&point);

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
			tmp += UniToTstr(L"\u2193");	// _T("��");
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

		tmp.Format(_T("%d %s"), (index++) % 10, (LPCTSTR)str);
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

		tmp.Format(_T("&%d %s"), (index++) % 10, (LPCTSTR)str);
		pMenu->AppendMenu(MF_STRING, nId++, tmp);
	}
}
BOOL CMainFrame::CopyClipboardData(CString &str)
{
	int len, max = 0;
	HGLOBAL hData;
	WCHAR *pData = NULL;
	BOOL ret = FALSE;

	// 10ms���b�N�o����܂ő҂�
	if ( m_OpenClipboardLock.IsLocked() || !m_OpenClipboardLock.Lock(10) )
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

	for ( len = 0 ; len < max && *pData != L'\0' && *pData != L'\x1A' ; len++ )
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
BOOL CMainFrame::SetClipboardText(LPCTSTR str, LPCSTR rtf)
{
	HGLOBAL hData = NULL;
	LPTSTR pData;
	BOOL bLock = FALSE;
	BOOL bOpen = FALSE;
	LPCTSTR pMsg = NULL;
	BOOL bRet = FALSE;

	// 500ms���b�N�o����܂ő҂�
	if ( !m_OpenClipboardLock.Lock(500) ) {
		pMsg = _T("Clipboard Busy...");
		goto ENDOF;
	}
	bLock = TRUE;

	if ( (hData = GlobalAlloc(GMEM_MOVEABLE, (_tcslen(str) + 1) * sizeof(TCHAR))) == NULL ) {
		pMsg = _T("Global Alloc Error");
		goto ENDOF;
	}

	if ( (pData = (TCHAR *)GlobalLock(hData)) == NULL ) {
		pMsg = _T("Global Lock Error");
		goto ENDOF;
	}

	_tcscpy(pData, str);
	GlobalUnlock(pData);

	// �N���b�v�{�[�h�`�F�C���̃`�F�b�N�ׂ̈̏���
	m_bClipEnable = FALSE;

	for ( int n = 0 ; !OpenClipboard() ; n++ ) {
		if ( n >= 10 ) {
			pMsg = _T("Clipboard Open Error");
			goto ENDOF;
		}
		Sleep(100);
	}
	bOpen = TRUE;

	if ( !EmptyClipboard() ) {
		pMsg = _T("Clipboard Empty Error");
		goto ENDOF;
	}

#ifdef	_UNICODE
	if ( SetClipboardData(CF_UNICODETEXT, hData) == NULL ) {
#else
	if ( SetClipboardData(CF_TEXT, hData) == NULL ) {
#endif
		pMsg = _T("Clipboard Set Data Error");
		goto ENDOF;
	}
	hData = NULL;
	bRet = TRUE;

	if ( rtf != NULL ) {
		if ( (hData = GlobalAlloc(GMEM_MOVEABLE, (strlen(rtf) + 1))) == NULL ) {
			pMsg = _T("Global Alloc Error");
			goto ENDOF;
		}

		if ( (pData = (TCHAR *)GlobalLock(hData)) == NULL ) {
			pMsg = _T("Global Lock Error");
			goto ENDOF;
		}

		strcpy((LPSTR)pData, rtf);
		GlobalUnlock(pData);

		if ( SetClipboardData(RegisterClipboardFormat(CF_RTF), hData) == NULL ) {
			pMsg = _T("Clipboard Set Data Error");
			goto ENDOF;
		}
		hData = NULL;
	}

ENDOF:
	if ( hData != NULL )
		GlobalFree(hData);
	if ( bOpen )
		CloseClipboard();
	if ( bLock )
		m_OpenClipboardLock.Unlock();
	if ( pMsg != NULL )
		::AfxMessageBox(pMsg, MB_ICONHAND);

	return bRet;
}
BOOL CMainFrame::GetClipboardText(CString &str)
{
	// �N���b�v�{�[�h�`�F�C���������Ȃ��ꍇ
	if ( !m_bClipEnable )
		SendMessage(WM_GETCLIPBOARD);

	if ( m_ClipBoard.IsEmpty() )
		return FALSE;

	str = m_ClipBoard.GetHead();

	return TRUE;
}

void CMainFrame::AnyPastDlgClose(int DocSeqNumber)
{
	if ( m_pAnyPastDlg != NULL && m_pAnyPastDlg->m_DocSeqNumber == DocSeqNumber )
		m_pAnyPastDlg->m_DocSeqNumber = 0;
}
void CMainFrame::AnyPastDlgOpen(LPCWSTR str, int DocSeqNumber)
{
	if ( m_pAnyPastDlg == NULL ) {
		m_pAnyPastDlg                 = new CAnyPastDlg;
		m_pAnyPastDlg->m_EditText     = str;
		m_pAnyPastDlg->m_DocSeqNumber = DocSeqNumber;
		m_pAnyPastDlg->m_pMain        = this;

		m_pAnyPastDlg->Create(IDD_ANYPASTDIG, this);
		m_pAnyPastDlg->ShowWindow(SW_SHOW);

	} else {
		m_pAnyPastDlg->SetEditText(str, DocSeqNumber);
		m_pAnyPastDlg->SetFocus();
	}
}

/////////////////////////////////////////////////////////////////////////////

static UINT VersionCheckThead(LPVOID pParam)
{
	CMainFrame *pWnd = (CMainFrame *)pParam;
	pWnd->VersionCheckProc();
	return 0;
}
int CMainFrame::VersionCheckProc()
{
	CBuffer buf;
	CHttpSession http;
	CHAR *p, *e;
	CString str;
	CStringArray pam;
	CStringLoad version;
	int rt = (-1);

	((CRLoginApp *)AfxGetApp())->GetVersion(version);
	str = AfxGetApp()->GetProfileString(_T("MainFrame"), _T("VersionNumber"), _T(""));
	if ( version.CompareDigit(str) < 0 )
		version = str;

	if ( !http.GetRequest(CStringLoad(IDS_VERSIONCHECKURL2), buf) )
		return rt;

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
		// RLogin 2.18.4 2015/05/20 http://nanno.bf1.jp/softlib/

		if ( pam.GetSize() >= 4 && pam[0].CompareNoCase(_T("RLogin")) == 0 ) {
			if ( version.CompareDigit(pam[1]) < 0 ) {
				AfxGetApp()->WriteProfileString(_T("MainFrame"), _T("VersionNumber"), pam[1]);
				m_VersionMessage.Format((LPCTSTR)CStringLoad(IDS_NEWVERSIONCHECK), (LPCTSTR)pam[1]);
				m_VersionPageUrl = pam[3];
				PostMessage(WM_COMMAND, IDM_NEWVERSIONFOUND);
				rt = 1;
			} else
				rt = 0;
			break;
		}
	}

	return rt;
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

void CMainFrame::SetTaskbarProgress(int state, int value)
{
	if ( m_pTaskbarList == NULL ) {
		if ( FAILED(::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, __uuidof(ITaskbarList3), reinterpret_cast<void**>(&m_pTaskbarList))) || FAILED(m_pTaskbarList->HrInit()) ) {
			m_pTaskbarList = NULL;
			return;
		}
	}

	switch(state) {
	default:
	case 0:
		m_pTaskbarList->SetProgressState(GetSafeHwnd(), TBPF_NOPROGRESS);
		break;
	case 1:		// When st is 1: set progress value to pr (number, 0-100). 
		m_pTaskbarList->SetProgressState(GetSafeHwnd(), TBPF_NORMAL);
		m_pTaskbarList->SetProgressValue(GetSafeHwnd(), value, 100);
		break;
	case 2:		// When st is 2: set error state in progress on Windows 7 taskbar, pr is optional. 
		m_pTaskbarList->SetProgressState(GetSafeHwnd(), TBPF_ERROR);
		m_pTaskbarList->SetProgressValue(GetSafeHwnd(), value, 100);
		break;
	case 3:		// When st is 3: set indeterminate state. 
		m_pTaskbarList->SetProgressState(GetSafeHwnd(), TBPF_INDETERMINATE);
		m_pTaskbarList->SetProgressValue(GetSafeHwnd(), value, 100);
		break;
	case 4:		// When st is 4: set paused state, pr is optional.
		m_pTaskbarList->SetProgressState(GetSafeHwnd(), TBPF_PAUSED);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame �f�f

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
// CMainFrame ���b�Z�[�W �n���h��

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

static int ThreadMsgComp(const void *src, const void *dis)
{
	return (int)((INT_PTR)src - (INT_PTR)*((void **)dis));
}
void CMainFrame::AddThreadMsg(void *pSyncSock)
{
	int n;
	if ( !BinaryFind(pSyncSock, m_ThreadMsg.GetData(), sizeof(void *), (int)m_ThreadMsg.GetSize(), ThreadMsgComp, &n) )
		m_ThreadMsg.InsertAt(n, pSyncSock);
}
void CMainFrame::DelThreadMsg(void *pSyncSock)
{
	int n;
	if ( BinaryFind(pSyncSock, m_ThreadMsg.GetData(), sizeof(void *), (int)m_ThreadMsg.GetSize(), ThreadMsgComp, &n) )
		m_ThreadMsg.RemoveAt(n);
}
LRESULT CMainFrame::OnThreadMsg(WPARAM wParam, LPARAM lParam)
{
	if ( BinaryFind((void *)lParam, m_ThreadMsg.GetData(), sizeof(void *), (int)m_ThreadMsg.GetSize(), ThreadMsgComp, NULL) )
		((CSyncSock *)lParam)->ThreadCommand((int)wParam);
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

	if ( !CServerSelect::IsJsonEntryText(*pStr) ) {

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
	}

	if ( m_pHistoryDlg != NULL )
		m_pHistoryDlg->Add(HISBOX_CLIP, *pStr);

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

	m_wndTabBar.FontSizeCheck();

	((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_MAINFRAME), m_wndToolBar, this);
	((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_TOOLBAR2), m_wndSubToolBar, this);

	m_wndQuickBar.DpiChanged();
	m_wndTabDlgBar.DpiChanged();
	m_wndVoiceBar.DpiChanged();
	m_wndStatusBar.DpiChanged();

	RecalcLayout(FALSE);

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

	if ( count > 0 && ::AfxMessageBox(CStringLoad(IDS_FILECLOSEQES), MB_ICONQUESTION | MB_YESNO) != IDYES )
		return;

	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("HistoryDlg"),	    m_bTabDlgBarShow && m_pHistoryDlg != NULL && m_wndTabDlgBar.IsInside(m_pHistoryDlg) ? TRUE : FALSE);
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("ToolBarStyle"),	m_wndToolBar.GetStyle());
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("SubToolBarStyle"),m_wndSubToolBar.GetStyle());
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("StatusBarStyle"), m_wndStatusBar.GetStyle());
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("QuickBarShow"),  (m_wndQuickBar.GetStyle() & WS_VISIBLE) != 0 ? TRUE : FALSE);
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("VoiceBarShow"),  (m_wndVoiceBar.GetStyle() & WS_VISIBLE) != 0 ? TRUE : FALSE);

	SaveBarState(_T("BarState"));

	if ( m_wndQuickBar.GetSafeHwnd() != NULL )
		m_wndQuickBar.SaveDialog();

	if ( m_pHistoryDlg != NULL )
		m_pHistoryDlg->SendMessage(WM_CLOSE);

	if ( m_pAnyPastDlg != NULL )
		m_pAnyPastDlg->SendMessage(WM_CLOSE);

	CMDIFrameWnd::OnClose();
}

void CMainFrame::OnDestroy() 
{
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

	theApp.EmojiImageFinish();

	CMDIFrameWnd::OnDestroy();
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent) 
{
	int n;
	clock_t st;
	CTimerObject *tp;
	CMidiQue *mp;

	CMDIFrameWnd::OnTimer(nIDEvent);

	switch(nIDEvent) {
	case TIMERID_SLEEPMODE:
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
		break;

	case TIMERID_MIDIEVENT:
		KillTimer(nIDEvent);
		m_MidiTimer = 0;
		if ( !m_MidiQue.IsEmpty() && (mp = m_MidiQue.RemoveHead()) != NULL ) {
			if ( m_pMidiData != NULL && m_pMidiData->m_hStream != NULL )
				midiOutShortMsg((HMIDIOUT)m_pMidiData->m_hStream, mp->m_Msg);
			delete mp;
		}
		while ( !m_MidiQue.IsEmpty() && (mp = m_MidiQue.GetHead()) != NULL && mp->m_mSec == 0 ) {
			if ( m_pMidiData != NULL && m_pMidiData->m_hStream != NULL )
				midiOutShortMsg((HMIDIOUT)m_pMidiData->m_hStream, mp->m_Msg);
			m_MidiQue.RemoveHead();
			delete mp;
		}
		if ( !m_MidiQue.IsEmpty() && (mp = m_MidiQue.GetHead()) != NULL )
			m_MidiTimer = SetTimer(TIMERID_MIDIEVENT, mp->m_mSec, NULL);
		break;

	case TIMERID_STATUSCLR:
		KillTimer(m_StatusTimer);
		m_StatusTimer = 0;
		SetMessageText(AFX_IDS_IDLEMESSAGE);
		break;

	case TIMERID_CLIPUPDATE:
		if ( m_bClipChain ) {
			ChangeClipboardChain(m_hNextClipWnd);
			m_hNextClipWnd = SetClipboardViewer();
		} else {
			KillTimer(nIDEvent);
			m_ClipTimer = 0;
		}
		break;

	case TIMERID_IDLETIMER:
		// �ő�20��A100ms�ȉ��ɐ���
		st = clock() + 90;
		for ( n = 0 ; n < 20 && st > clock() ; n++ ) {
			if ( !((CRLoginApp *)AfxGetApp())->OnIdle((LONG)(-1)) )
				break;
		}
		//TRACE("TimerIdle %d(%d)\n", n, clock() - st + 90);
		break;

	default:
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
		break;
	}
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
void CMainFrame::SplitHeightPane(BOOL bDialog)
{
	if ( m_pTopPane == NULL )
		m_pTopPane = new CPaneFrame(this, NULL, NULL);

	CPaneFrame *pPane = m_pTopPane->GetActive();

	if ( pPane->m_pOwn == NULL || pPane->m_pOwn->m_Style != PANEFRAME_MAXIM )
		pPane->CreatePane((bDialog ? PANEFRAME_HEDLG : PANEFRAME_HEIGHT), NULL);
	else {
		while ( pPane->m_pOwn != NULL && pPane->m_pOwn->m_Style == PANEFRAME_MAXIM )
			pPane = pPane->m_pOwn;
		pPane = pPane->InsertPane();
		if ( pPane->m_pOwn == NULL )
			m_pTopPane = pPane;
		pPane->MoveParOwn(pPane->m_Frame, (bDialog ? PANEFRAME_HEDLG : PANEFRAME_HEIGHT));
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

	CPaneFrame *pPane, *pOwner, *pNext;

	if ( (pPane = m_pTopPane->GetActive()) == NULL )
		return;

	if ( pPane->m_Style == PANEFRAME_WINDOW && pPane->m_hWnd == NULL && (pOwner = pPane->m_pOwn) != NULL ) {
		pNext = (pOwner->m_pLeft == pPane ? pOwner->m_pRight : pOwner->m_pLeft);
		pPane->m_pLeft = pPane->m_pRight = NULL;
		delete pPane;

		pPane = pNext;
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
	HCURSOR hCursor = NULL;

	GetCursorPos(&point);
	ScreenToClient(&point);
	ClientToFrame(&point);

	if ( m_pTopPane != NULL && (pPane = m_pTopPane->HitTest(point)) != NULL && pPane->m_Style != PANEFRAME_WINDOW ) {
		LPCTSTR id = (pPane->m_Style == PANEFRAME_HEIGHT ? ATL_MAKEINTRESOURCE(AFX_IDC_VSPLITBAR) : ATL_MAKEINTRESOURCE(AFX_IDC_HSPLITBAR));
		HINSTANCE hInst = AfxFindResourceHandle(id, ATL_RT_GROUP_CURSOR);

		if ( hInst != NULL )
			hCursor = ::LoadCursorW(hInst, id);

		if ( hCursor == NULL )
			hCursor = AfxGetApp()->LoadStandardCursor(pPane->m_Style == PANEFRAME_HEIGHT ? IDC_SIZENS : IDC_SIZEWE);
	}

	if ( hCursor != NULL ) {
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
			CKeyNode *pNode;

			if ( (GetKeyState(VK_SHIFT) & 0x80) != 0 )
				st |= MASK_SHIFT;

			if ( (GetKeyState(VK_CONTROL) & 0x80) != 0 )
				st |= MASK_CTRL;

			if ( (GetKeyState(VK_MENU) & 0x80) != 0 )
				st |= MASK_ALT;

			if ( (pNode = m_DefKeyTab.FindMaps((int)pMsg->wParam, st)) != NULL &&  (id = CKeyNodeTab::GetCmdsKey((LPCWSTR)pNode->m_Maps)) > 0 ) {
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
	CRect rect;

	// m_TrackPoint�́AFrameRect�̍��W�Am_TrackRect�́AClientRect�̍��W�Ȃ̂Œ���
	ClientToFrame(&point);

	if ( m_pTrackPane->m_Style == PANEFRAME_WIDTH ) {
		rect = m_pTrackPane->m_Frame;
		FrameToClient(&rect);
		m_TrackRect = m_TrackBase + CPoint(point.x - m_TrackPoint.x, 0);
		int w = m_TrackRect.Width();
		if ( m_TrackRect.left < (rect.left + PANEMIN_WIDTH) ) {
			m_TrackRect.left = rect.left + PANEMIN_WIDTH;
			m_TrackRect.right = m_TrackRect.left + w;
		} else if ( m_TrackRect.right > (rect.right - PANEMIN_WIDTH) ) {
			m_TrackRect.right = rect.right - PANEMIN_WIDTH;
			m_TrackRect.left = m_TrackRect.right - w;
		}

	} else {
		rect = m_pTrackPane->m_Frame;
		FrameToClient(&rect);
		m_TrackRect = m_TrackBase + CPoint(0, point.y - m_TrackPoint.y);
		int h = m_TrackRect.Height();
		if ( m_TrackRect.top < (rect.top + PANEMIN_HEIGHT) ) {
			m_TrackRect.top = rect.top + PANEMIN_HEIGHT;
			m_TrackRect.bottom = m_TrackRect.top + h;
		} else if ( m_TrackRect.bottom > (rect.bottom - PANEMIN_HEIGHT) ) {
			m_TrackRect.bottom = rect.bottom - PANEMIN_HEIGHT;
			m_TrackRect.top = m_TrackRect.bottom - h;
		}
	}

	// �ʒu�̍X�V���`�F�b�N
	if ( m_TrackLast == m_TrackRect )
		return;
	m_TrackLast = m_TrackRect;

	if ( m_pTrackPane != NULL ) {
		ClientToFrame(m_TrackRect);

		if ( m_pTrackPane->m_Style == PANEFRAME_WIDTH ) {
			m_pTrackPane->m_pLeft->m_Frame.right = m_TrackRect.left  + 1;
			m_pTrackPane->m_pRight->m_Frame.left = m_TrackRect.right - 1;
		} else {
			m_pTrackPane->m_pLeft->m_Frame.bottom = m_TrackRect.top    + 1;
			m_pTrackPane->m_pRight->m_Frame.top   = m_TrackRect.bottom - 1;
		}

		m_pTrackPane->MoveParOwn(m_pTrackPane->m_Frame, PANEFRAME_NOCHNG);
	}
}
int CMainFrame::PreLButtonDown(UINT nFlags, CPoint point)
{
	CPaneFrame *pPane;

	ClientToFrame(&point);

	if ( m_pTrackPane != NULL || m_pTopPane == NULL || (pPane = m_pTopPane->HitTest(point)) == NULL )
		return FALSE;

	if ( pPane->m_Style == PANEFRAME_WINDOW ) {
		m_pTopPane->HitActive(point);
		return FALSE;
	}

	m_pTrackPane = pPane;
	m_pTrackPane->BoderRect(m_TrackRect);

	SetCapture();
	m_TrackPoint = point;
	m_TrackBase = m_TrackLast = m_TrackRect;

	return TRUE;
}
void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CMDIFrameWnd::OnLButtonUp(nFlags, point);

	if ( m_pTrackPane != NULL ) {
		OffsetTrack(point);
		ReleaseCapture();
		m_pTrackPane = NULL;
	}
}

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point) 
{
	CMDIFrameWnd::OnMouseMove(nFlags, point);

	MSG msg;

	// �\�������������܂ŏ������Ȃ�
	if ( m_pTrackPane != NULL && !::PeekMessage(&msg, NULL, NULL, NULL, PM_QS_PAINT | PM_NOREMOVE) )
		OffsetTrack(point);
}

void CMainFrame::OnUpdateIndicatorSock(CCmdUI* pCmdUI)
{
	int n = 7;
	CRLoginDoc *pDoc;
	static LPCTSTR ProtoName[] = { 
		_T("TCP"), _T("Login"), _T("Telnet"), _T("COM"), _T("PIPE"), _T("SSH"), _T("PFD"), _T("") };

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
		str += ( pDoc->m_TextRam.IsLineEditEnable()     ? _T('L') : _T(' '));
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

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_pTopPane->SetBuffer(&buf);

	if ( !file.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive) )
		return;

	file.Write("RLM100\n", 7);
	file.Write(buf.GetPtr(), buf.GetSize());
	file.Close();
}
void CMainFrame::OnFileAllLoad() 
{
	CPaneFrame *pPane;

	if ( IsConnectChild(m_pTopPane) ) {
		if ( ::AfxMessageBox(CStringLoad(IDE_ALLCLOSEREQ), MB_ICONQUESTION | MB_OKCANCEL) != IDOK )
			return;
	}
	
	try {
		if ( (pPane = CPaneFrame::GetBuffer(this, NULL, NULL, &m_AllFileBuf)) == NULL )
			return;
	} catch(...) {
		::AfxMessageBox(_T("File All Load Error"), MB_ICONERROR);
		return;
	}

	AfxGetApp()->CloseAllDocuments(FALSE);

	if ( m_pTopPane != NULL )
		delete m_pTopPane;
	m_pTopPane = pPane;

	if ( m_pTopPane->GetEntry() != NULL ) {
		m_LastPaneFlag = TRUE;
		m_pLastPaneFoucs = NULL;
		AfxGetApp()->AddToRecentFileList(m_AllFilePath);
		PostMessage(WM_COMMAND, ID_FILE_NEW, 0);
	}
}
void CMainFrame::OnFileAllLast()
{
	if ( m_pLastPaneFoucs != NULL )
		MDIActivate(m_pLastPaneFoucs);
	m_pLastPaneFoucs = NULL;
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if ( pCopyDataStruct->dwData == 0x524c4f31 ) {
		return ((CRLoginApp *)::AfxGetApp())->OnInUseCheck(pCopyDataStruct, FALSE);

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

	} else if ( pCopyDataStruct->dwData == 0x524c4f39 ) {
		return ((CRLoginApp *)::AfxGetApp())->OnInUseCheck(pCopyDataStruct, TRUE);

	} else if ( pCopyDataStruct->dwData == 0x524c4f3a ) {
		((CRLoginApp *)::AfxGetApp())->OnUpdateServerEntry(pCopyDataStruct);
		return TRUE;
	}

	return CMDIFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}

void CMainFrame::InitMenuBitmap()
{
#if 0
	int n, cx, cy;
	CDC dc[2];
	CBitmap BitMap;
	CBitmap *pOld[2];
	CBitmap *pBitmap;
	BITMAP mapinfo;
	CMenuBitMap *pMap;

	// ���\�[�X�f�[�^�x�[�X���烁�j���[�C���[�W���쐬
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
#else
	int n, cx, cy;
	CImage Image;
	CDC TmpDC;
	CBitmap BitMap;
	CBitmap *pOld;
	CBitmap *pBitmap;
	BITMAP mapinfo;
	CMenuBitMap *pMap;
	COLORREF menuCol = GetSysColor(COLOR_MENU);
	BOOL bGraterVista = CRLoginApp::IsWinVerCheck(_WIN32_WINNT_VISTA, VER_GREATER_EQUAL);

	// ���\�[�X�f�[�^�x�[�X���烁�j���[�C���[�W���쐬
	cx = GetSystemMetrics(SM_CXMENUCHECK);
	cy = GetSystemMetrics(SM_CYMENUCHECK);

	CResDataBase *pResData = &(((CRLoginApp *)::AfxGetApp())->m_ResDataBase);

	// Add Menu Image From Bitmap Resource
	for ( n = 0 ; n < 5 ; n++ )
		pResData->AddBitmap(MAKEINTRESOURCE(IDB_MENUMAP1 + n));

	// MenuMap RemoveAll
	for ( int n = 0 ; n < m_MenuMap.GetSize() ; n++ ) {
		if ( (pMap = (CMenuBitMap *)m_MenuMap[n]) == NULL )
			continue;
		pMap->m_Bitmap.DeleteObject();
		delete pMap;
	}
	m_MenuMap.RemoveAll();

	TmpDC.CreateCompatibleDC(NULL);

	for ( n = 0 ; n < pResData->m_Bitmap.GetSize() ; n++ ) {
		if ( pResData->m_Bitmap[n].m_hBitmap == NULL )
			continue;

		pBitmap = CBitmap::FromHandle(pResData->m_Bitmap[n].m_hBitmap);

		if ( pBitmap == NULL || !pBitmap->GetBitmap(&mapinfo) )
			continue;

		if ( (pMap = new CMenuBitMap) == NULL )
			continue;

		pOld = TmpDC.SelectObject(pBitmap);

		pMap->m_Id = pResData->m_Bitmap[n].m_ResId;
		Image.CreateEx(cx, cy, 32, BI_RGB, NULL, CImage::createAlphaChannel);

		for ( int y = 0 ; y < cx ; y++ ) {
			for ( int x = 0 ; x < cy ; x++ ) {
				BYTE *p = (BYTE *)Image.GetPixelAddress(x, y);
				p[0] = p[1] = p[2] = 192;
				p[3] = 255;
			}
		}

		::TransparentBlt(Image.GetDC(), 0, 0, cx,cy, TmpDC, 0, 0, (mapinfo.bmWidth <= mapinfo.bmHeight ? mapinfo.bmWidth : mapinfo.bmHeight), mapinfo.bmHeight, RGB(192, 192, 192));
		Image.ReleaseDC();

		TmpDC.SelectObject(pOld);

		if ( bGraterVista ) {
			for ( int y = 0 ; y < cx ; y++ ) {
				for ( int x = 0 ; x < cy ; x++ ) {
					BYTE *p = (BYTE *)Image.GetPixelAddress(x, y);
					if ( p[0] == 192 && p[1] == 192 && p[2] == 192 )
						p[0] = p[1] = p[2] = p[3] = 0;
					else
						p[3] = 255;
				}
			}
		} else {
			for ( int y = 0 ; y < cx ; y++ ) {
				for ( int x = 0 ; x < cy ; x++ ) {
					BYTE *p = (BYTE *)Image.GetPixelAddress(x, y);
					if ( p[0] == 192 && p[1] == 192 && p[2] == 192 )
						*((COLORREF *)p) = menuCol;
					else
						p[3] = 255;
				}
			}
		}

		pMap->m_Bitmap.Attach(Image);
		Image.Detach();

		m_MenuMap.Add(pMap);
	}

	TmpDC.DeleteDC();
#endif
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
BOOL CMainFrame::TrackPopupMenuIdle(CMenu *pMenu, UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect)
{
	BOOL rt = FALSE;

	// OnEnterMenuLoop
	SetIdleTimer(TRUE);

	rt = pMenu->TrackPopupMenu(nFlags, x, y, pWnd, lpRect);

	// OnExitMenuLoop
	SetIdleTimer(FALSE);

	return rt;
}

void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	int n, a;
	CMenu *pSubMenu;
	CRLoginDoc *pDoc;
	CString str;

	CMDIFrameWnd::OnInitMenu(pMenu);

	if ( (pDoc = GetMDIActiveDocument()) != NULL && pMenu != NULL && (pSubMenu = GetMenu()) != NULL && pSubMenu->GetSafeHmenu() == pMenu->GetSafeHmenu() )
		pDoc->SetMenu(pMenu);

	else if ( pMenu != NULL ) {
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
void CMainFrame::OnEnterMenuLoop(BOOL bIsTrackPopupMenu)
{
	SetIdleTimer(TRUE);
}
void CMainFrame::OnExitMenuLoop(BOOL bIsTrackPopupMenu)
{
	SetIdleTimer(FALSE);
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
	CWinApp *pApp = AfxGetApp();
	CChildFrame *pChild = (CChildFrame *)MDIGetActive();
	CMenu *pMenu = NULL;

	ASSERT(pApp != NULL);
	
	m_bMenuBarShow = (m_bMenuBarShow ? FALSE : TRUE);
	pApp->WriteProfileInt(_T("ChildFrame"), _T("VMenu"), m_bMenuBarShow);

	if ( pChild != NULL )
		pChild->OnUpdateFrameMenu(m_bMenuBarShow, pChild, NULL);

	if ( m_bMenuBarShow ) {
		if ( (pMenu = GetMenu()) == NULL && m_StartMenuHand != NULL )
			pMenu = CMenu::FromHandle(m_StartMenuHand);
	}

	SetMenu(pMenu);
}
void CMainFrame::OnUpdateViewMenubar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMenuBarShow);
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
void CMainFrame::OnViewQuickbar()
{
	ShowControlBar(&m_wndQuickBar, ((m_wndQuickBar.GetStyle() & WS_VISIBLE) != 0 ? FALSE : TRUE), FALSE);
}
void CMainFrame::OnUpdateViewQuickbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_wndQuickBar.GetStyle() & WS_VISIBLE) ? 1 : 0);
}
void CMainFrame::OnViewVoicebar()
{
	ShowControlBar(&m_wndVoiceBar, ((m_wndVoiceBar.GetStyle() & WS_VISIBLE) != 0 ? FALSE : TRUE), FALSE);
}
void CMainFrame::OnUpdateViewVoicebar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_wndVoiceBar.GetStyle() & WS_VISIBLE) ? 1 : 0);
}
void CMainFrame::OnViewTabDlgbar()
{
	CWinApp *pApp = AfxGetApp();

	if ( m_bTabDlgBarShow && (m_wndTabDlgBar.GetStyle() & WS_VISIBLE) == 0 && m_wndTabDlgBar.m_TabCtrl.GetItemCount() > 0 ) {
		ShowControlBar(&m_wndTabDlgBar, TRUE, FALSE);
		return;
	}
	
	m_bTabDlgBarShow = (m_bTabDlgBarShow? FALSE : TRUE);
	pApp->WriteProfileInt(_T("MainFrame"), _T("TabDlgBarShow"), m_bTabDlgBarShow);

	if ( m_bTabDlgBarShow ) {
		if ( m_pHistoryDlg != NULL )
			AddTabDlg(m_pHistoryDlg, 7);

		POSITION pos = pApp->GetFirstDocTemplatePosition();
		while ( pos != NULL ) {
			CDocTemplate *pDocTemp = pApp->GetNextDocTemplate(pos);
			POSITION dpos = pDocTemp->GetFirstDocPosition();
			while ( dpos != NULL ) {
				CRLoginDoc *pDoc = (CRLoginDoc *)pDocTemp->GetNextDoc(dpos);
				if ( pDoc == NULL )
					continue;
				if ( pDoc->m_TextRam.m_pCmdHisWnd != NULL )
					AddTabDlg(pDoc->m_TextRam.m_pCmdHisWnd, 2);
				if ( pDoc->m_TextRam.m_pTraceWnd != NULL )
					AddTabDlg(pDoc->m_TextRam.m_pTraceWnd, 6);
			}
		}
	} else {
		m_wndTabDlgBar.RemoveAll();
		ShowControlBar(&m_wndTabDlgBar, FALSE, FALSE);
	}
}
void CMainFrame::OnUpdateViewTabDlgbar(CCmdUI *pCmdUI)
{
	if ( m_bTabDlgBarShow && (m_wndTabDlgBar.GetStyle() & WS_VISIBLE) == 0 && m_wndTabDlgBar.m_TabCtrl.GetItemCount() > 0 )
		pCmdUI->SetCheck(0);
	else
		pCmdUI->SetCheck(m_bTabDlgBarShow);
}
void CMainFrame::OnViewTabbar()
{
	if ( (m_wndTabBar.GetStyle() & WS_VISIBLE) == 0 && (m_bTabBarShow || m_wndTabBar.m_TabCtrl.GetItemCount() > 1) ) {
		ShowControlBar(&m_wndTabBar, TRUE, FALSE);
		return;
	}

	m_bTabBarShow = (m_bTabBarShow ? FALSE : TRUE);
	::AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("TabBarShow"), m_bTabBarShow);

	if ( m_bTabBarShow )
		ShowControlBar(&m_wndTabBar, TRUE, FALSE);
	else if ( m_wndTabBar.m_TabCtrl.GetItemCount() <= 1 )
		ShowControlBar(&m_wndTabBar, FALSE, FALSE);
}
void CMainFrame::OnUpdateViewTabbar(CCmdUI *pCmdUI)
{
	if ( (m_wndTabBar.GetStyle() & WS_VISIBLE) == 0 && (m_bTabBarShow || m_wndTabBar.m_TabCtrl.GetItemCount() > 1) )
		pCmdUI->SetCheck(0);
	else
		pCmdUI->SetCheck(m_bTabBarShow);
}
void CMainFrame::OnViewSubToolbar()
{
	ShowControlBar(&m_wndSubToolBar, ((m_wndSubToolBar.GetStyle() & WS_VISIBLE) != 0 ? FALSE : TRUE), FALSE);
}
void CMainFrame::OnUpdateSubToolbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck((m_wndSubToolBar.GetStyle() & WS_VISIBLE) ? 1 : 0);
}

void CMainFrame::OnDockBarFixed()
{
	m_bDockBarMode = (m_bDockBarMode ? FALSE : TRUE);
	::AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("DockBarMode"), m_bDockBarMode);

	DWORD dwDel = 0, dwAdd = CBRS_GRIPPER;

	if ( m_bDockBarMode ) {
		m_wndToolBar.m_dwStyle    &= ~CBRS_GRIPPER;
		m_wndSubToolBar.m_dwStyle &= ~CBRS_GRIPPER;
		m_wndQuickBar.m_dwStyle   &= ~CBRS_GRIPPER;
		m_wndVoiceBar.m_dwStyle   &= ~CBRS_GRIPPER;
		m_wndTabBar.m_dwStyle     &= ~CBRS_GRIPPER;
		m_wndTabDlgBar.m_dwStyle  &= ~CBRS_GRIPPER;
	} else {
		m_wndToolBar.m_dwStyle    |= CBRS_GRIPPER;
		m_wndSubToolBar.m_dwStyle |= CBRS_GRIPPER;
		m_wndQuickBar.m_dwStyle   |= CBRS_GRIPPER;
		m_wndVoiceBar.m_dwStyle   |= CBRS_GRIPPER;
		m_wndTabBar.m_dwStyle     |= CBRS_GRIPPER;
		m_wndTabDlgBar.m_dwStyle  |= CBRS_GRIPPER;
	}

	RecalcLayout(TRUE);
}
void CMainFrame::OnUpdateDockBarFixed(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bDockBarMode ? 1 : 0);
}
void CMainFrame::OnDockBarInit()
{
	CControlBar *pBar;
	struct {
		UINT nID;
		DWORD Style;
		CControlBar *pBar;
	} DocBarTab[] = {
		{ AFX_IDW_DOCKBAR_BOTTOM,	CBRS_ALIGN_BOTTOM,	&m_wndTabDlgBar		},
		{ AFX_IDW_DOCKBAR_TOP,		CBRS_ALIGN_TOP,		&m_wndTabBar		},
		{ AFX_IDW_DOCKBAR_TOP,		CBRS_ALIGN_TOP,		&m_wndVoiceBar		},
		{ AFX_IDW_DOCKBAR_TOP,		CBRS_ALIGN_TOP,		&m_wndQuickBar		},
		{ AFX_IDW_DOCKBAR_TOP,		CBRS_ALIGN_TOP,		&m_wndSubToolBar	},
		{ AFX_IDW_DOCKBAR_TOP,		CBRS_ALIGN_TOP,		&m_wndToolBar		},
		{ 0,						0,					NULL				},
	};

	for ( int n = 0 ; (pBar = DocBarTab[n].pBar) != NULL ; n++ ) {
		if ( DocBarTab[n].Style != (pBar->m_dwStyle & CBRS_ALIGN_ANY) && !pBar->IsFloating() ) {
			pBar->m_pDockBar->RemoveControlBar(pBar);
			pBar->m_pDockBar = NULL;
		}

		DockControlBar(pBar, DocBarTab[n].nID, CRect(0, 0, 0, 0));
	}
}
void CMainFrame::OnUpdateDockBarInit(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnNewVersionFound()
{
	if ( ::AfxMessageBox(m_VersionMessage, MB_ICONQUESTION | MB_YESNO) == IDYES )
		ShellExecute(m_hWnd, NULL, m_VersionPageUrl, NULL, NULL, SW_NORMAL);
}
void CMainFrame::OnVersioncheck()
{
	int rt;

	m_bVersionCheck = (m_bVersionCheck ? FALSE : TRUE);
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("VersionCheckFlag"), m_bVersionCheck);
	AfxGetApp()->WriteProfileString(_T("MainFrame"), _T("VersionNumber"), _T(""));

	if ( m_bVersionCheck ) {
		if ( ::AfxMessageBox(CStringLoad(IDS_VERCHKENABLE), MB_ICONQUESTION | MB_YESNO) == IDYES ) {
			if ( (rt = VersionCheckProc()) == 0 )
				::AfxMessageBox(CStringLoad(IDS_VERCHKLATEST), MB_ICONINFORMATION);
			else if ( rt < 0 )
				::AfxMessageBox(CStringLoad(IDE_VERCHKERROR), MB_ICONERROR);
		}
	} else
		::AfxMessageBox(CStringLoad(IDS_VERCHKDISABLE), MB_ICONINFORMATION);
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

	OnClipboardUpdate();
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
	// �{���Ȃ炱����OpenClipboard�Ȃǂ̃N���b�v�{�[�h�ɃA�N�Z�X����Ηǂ��Ǝv���̂���
	// �����[�g�f�B�X�N�g�b�v��RLogin��̃|�[�g�t�H���[�h�Ŏ��s�����GetClipboardData��
	// �f�b�h���b�N���N�����Ă��܂��B

	// ���̑Ή��ŕʃX���b�h�ŃN���b�v�{�[�h�̃A�N�Z�X���s���Ă��邪Excel2010�ȂǂŃN��
	// �b�v�{�[�h�̃R�s�[�𑽐��s�����ꍇ�Ȃǂɂ���OnClipboardUpdate�����Ȃ�̕p�x�ő�
	// ����悤�ɂȂ�X���b�h���d�����ċN������

	// OpenClipboard�ł́A�����E�B���h�E�ł̃I�[�v�����u���b�N���Ȃ��悤�Ŗ��ȓ��삪�m
	// �F�ł����iOpen->Open->Close->Close�Ő��Close�ŉ������A���Close�͖��������)
	// GlobalLock���Ă��郁�����n���h����Unlock�O�ɉ�������Ǐ󂪏o��

	// ���C���E�B���h�E�ł̃A�N�Z�X��CMutexLock��OpenClipbard�̑O�ɍs���悤
	// �ɂ����B�ʃX���b�h�̃N���b�v�{�[�h�A�N�Z�X�́A��肪�����Ǝv��

	// ���Ȃ��₱��������Ȃ̂ł����Ƀ������c��

	clock_t now = clock();
	int msec = (int)(now - m_LastClipUpdate) * 1000 / CLOCKS_PER_SEC;
	m_LastClipUpdate = now;

	if ( msec > 0 && msec < CLIPOPENLASTMSEC )
		return;

	//TRACE("OnClipboardUpdate %d\n", msec);

	m_bClipEnable = TRUE;	// �N���b�v�{�[�h�`�F�C�����L���H

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

	((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_MAINFRAME), m_wndToolBar, this);
	((CRLoginApp *)::AfxGetApp())->LoadResToolBar(MAKEINTRESOURCE(IDR_TOOLBAR2), m_wndSubToolBar, this);

	// �c�[���o�[�̍ĕ\��
	RecalcLayout(FALSE);
}

void CMainFrame::OnClipchain()
{
	if ( m_bAllowClipChain ) {
		// Do Disable
		m_bAllowClipChain = FALSE;
		m_bClipEnable = FALSE;

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
		m_bClipEnable = TRUE;

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
void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	if ( bDarkModeSupport && lpszSection != NULL && _tcscmp(lpszSection, _T("ImmersiveColorSet")) == 0 ) {
		m_bDarkMode = ExDwmDarkMode(GetSafeHwnd());
		RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
	} else {
		InitAppColor();
		LoadAppColor();
	}

	CMDIFrameWnd::OnSettingChange(uFlags, lpszSection);
}
void CMainFrame::DrawSystemBar()
{
	CRect window, client;
	CDC *pDC = GetWindowDC();

	GetWindowRect(window);
	GetClientRect(client);
	ClientToScreen(client);

	// ���j���[�o�[���̐���w�i�Ɠ����ɓh��
	pDC->FillSolidRect(0, client.top - window.top - 1, window.Width(), 1, GetAppColor(APPCOL_MENUFACE));

	ReleaseDC(pDC);
}
void CMainFrame::OnNcPaint()
{
	Default();

	if ( bDarkModeSupport )
		DrawSystemBar();
}
BOOL CMainFrame::OnNcActivate(BOOL bActive)
{
	BOOL ret = CMDIFrameWnd::OnNcActivate(bActive);

	if ( bDarkModeSupport )
		DrawSystemBar();

	return ret;
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	CRect rect;
	int cx = 200, cy = 200;

	if ( m_hWnd != NULL ) {
		GetWindowRect(rect);

		if ( !(m_Frame.left >= m_Frame.right && m_Frame.top >= m_Frame.bottom) && !(rect.left >= rect.right && rect.top >= rect.bottom) ) {
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
	if ( ::AfxMessageBox(CStringLoad(IDS_DELOLDENTRYMSG), MB_ICONQUESTION | MB_YESNO) == IDYES )
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

BOOL CMainFrame::SpeakQueIn()
{
	if ( m_SpeakAbs > m_pSpeakDoc->m_TextRam.m_HisAbs ) {
		m_SpeakLine -= m_SpeakAbs;
		m_SpeakAbs = m_pSpeakDoc->m_TextRam.m_HisAbs;
		m_SpeakLine += m_SpeakAbs;
	}

	BOOL bContinue;
	int line = m_SpeakLine - m_pSpeakDoc->m_TextRam.m_HisAbs;
	ISpVoice *pVoice = ((CRLoginApp *)::AfxGetApp())->VoiceInit();

	if ( line < (0 - m_pSpeakDoc->m_TextRam.m_HisLen + m_pSpeakDoc->m_TextRam.m_Lines) ) {
		line = 0 - m_pSpeakDoc->m_TextRam.m_HisLen + m_pSpeakDoc->m_TextRam.m_Lines + 1;
		m_SpeakAbs = m_pSpeakDoc->m_TextRam.m_HisAbs;
		m_SpeakLine = line + m_SpeakAbs;
	}

	while ( m_SpeakQueLen < SPEAKQUESIZE ) {
		if ( line >= m_pSpeakDoc->m_TextRam.m_Lines )
			return FALSE;

		m_SpeakData[m_SpeakQuePos].text.Empty();
		m_SpeakData[m_SpeakQuePos].pos.RemoveAll();
		m_SpeakData[m_SpeakQuePos].skip = 0;
		m_SpeakData[m_SpeakQuePos].abs  = m_SpeakAbs;
		m_SpeakData[m_SpeakQuePos].line = m_SpeakLine;

		for ( int n = 0 ; n < 3 && line < m_pSpeakDoc->m_TextRam.m_Lines ; n++ ) {
			bContinue = m_pSpeakDoc->m_TextRam.SpeakLine(m_SpeakCols, line++, m_SpeakData[m_SpeakQuePos].text, m_SpeakData[m_SpeakQuePos].pos);
	  		m_SpeakLine++;
			m_SpeakCols = 0;

			if ( !bContinue )
				break;
		}

		if ( m_SpeakData[m_SpeakQuePos].text.IsEmpty() || m_SpeakData[m_SpeakQuePos].pos.GetSize() == 0 )
			continue;

		pVoice->Speak(m_SpeakData[m_SpeakQuePos].text, SPF_ASYNC | SPF_IS_NOT_XML, &(m_SpeakData[m_SpeakQuePos].num));

		if ( ++m_SpeakQuePos >= SPEAKQUESIZE )
			m_SpeakQuePos = 0;
		m_SpeakQueLen++;
	}

	return TRUE;
}
void CMainFrame::Speak(LPCTSTR str)
{
	ISpVoice *pVoice = ((CRLoginApp *)::AfxGetApp())->VoiceInit();

	if ( pVoice == NULL )
		return;

	if ( m_bVoiceEvent ) {
		m_bVoiceEvent = FALSE;
		pVoice->SetInterest(0, 0);
		pVoice->Speak(L"", SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_NOT_XML, NULL);
		m_pSpeakView->SpeakTextPos(FALSE, NULL, NULL);
	}

	pVoice->Speak(TstrToUni(str), SPF_ASYNC, NULL);
}
void CMainFrame::SpeakUpdate(int x, int y)
{
	int pos;

	TRACE("UPDATE %d,%d\n", x, y);

	if ( !m_bVoiceEvent )
		return;

	pos = m_SpeakQueTop;
	for ( int n = 0 ; n < m_SpeakQueLen ; n++ ) {
		m_SpeakData[pos].skip = 1;
		if ( ++pos >= SPEAKQUESIZE )
			pos = 0;
	}

	m_SpeakAbs  = m_SpeakActive[0] = m_pSpeakDoc->m_TextRam.m_HisAbs;
	m_SpeakLine = m_SpeakActive[1] = m_SpeakAbs + y;
	m_SpeakCols = m_SpeakActive[2] = x;
}
LRESULT CMainFrame::OnSpeakMsg(WPARAM wParam, LPARAM lParam)
{
	SPEVENT eventItem;
	ISpVoice *pVoice = ((CRLoginApp *)::AfxGetApp())->VoiceInit();
	SPVOICESTATUS status;
	CCurPos spos, epos;
	ULONG skipd;

	memset(&eventItem, 0, sizeof(SPEVENT));

	while( pVoice->GetEvents(1, &eventItem, NULL ) == S_OK ) {
		if ( !m_bVoiceEvent )
			continue;

		switch(eventItem.eEventId) {
        case SPEI_WORD_BOUNDARY:
			if ( m_SpeakQueLen <= 0 )
				break;
			if ( eventItem.ulStreamNum != m_SpeakData[m_SpeakQueTop].num )
				break;
			pVoice->GetStatus(&status, NULL);
			if ( status.dwRunningState != SPRS_IS_SPEAKING )
				break;
			if ( status.ulCurrentStream != m_SpeakData[m_SpeakQueTop].num )
				break;
			if ( status.ulInputWordLen <= 0 )
				break;
			if ( (ULONG)m_SpeakData[m_SpeakQueTop].pos.GetSize() < (status.ulInputWordPos + status.ulInputWordLen - 1) )
				break;

			spos = m_SpeakData[m_SpeakQueTop].pos[status.ulInputWordPos];
			epos = m_SpeakData[m_SpeakQueTop].pos[status.ulInputWordPos + status.ulInputWordLen - 1];

			if ( m_SpeakData[m_SpeakQueTop].skip != 0 ) {
				pVoice->Skip(L"SENTENCE", 1, &skipd);
//				m_SpeakData[m_SpeakQueTop].skip = 0;
			} else {
				m_SpeakActive[0] = m_pSpeakDoc->m_TextRam.m_HisAbs;
				m_SpeakActive[1] = spos.cy - m_pSpeakDoc->m_TextRam.m_HisPos - m_pSpeakDoc->m_TextRam.m_HisMax + m_pSpeakDoc->m_TextRam.m_HisAbs;
				m_SpeakActive[2] = spos.cx;
			}

//			TRACE("SPEI_WORD %d,%d\n", spos.cx, spos.cy);

			if ( !m_pSpeakDoc->m_TextRam.SpeakCheck(spos, epos, (LPCTSTR)m_SpeakData[m_SpeakQueTop].text + status.ulInputWordPos) ) {
				pVoice->Skip(L"SENTENCE", 1, &skipd);
				m_SpeakAbs  = m_SpeakData[m_SpeakQueTop].abs;
				m_SpeakLine = m_SpeakData[m_SpeakQueTop].line;
				m_pSpeakView->SpeakTextPos(FALSE, NULL, NULL);
			} else
				m_pSpeakView->SpeakTextPos(TRUE, &spos, &epos);
			break;

		case SPEI_SENTENCE_BOUNDARY:
//		case SPEI_START_INPUT_STREAM:
			if ( m_SpeakQueLen <= 0 )
				break;
			if ( eventItem.ulStreamNum != m_SpeakData[m_SpeakQueTop].num )
				break;
			if ( m_SpeakData[m_SpeakQueTop].skip != 0 ) {
				pVoice->Skip(L"SENTENCE", 100, &skipd);
				m_SpeakData[m_SpeakQueTop].skip = 0;
			}
			break;

		case SPEI_END_INPUT_STREAM:
			if ( m_SpeakQueLen <= 0 )
				break;
			if ( eventItem.ulStreamNum != m_SpeakData[m_SpeakQueTop].num )
				break;

			m_SpeakData[m_SpeakQueTop].text.Empty();
			m_SpeakData[m_SpeakQueTop].pos.RemoveAll();

			if ( ++m_SpeakQueTop >= SPEAKQUESIZE )
				m_SpeakQueTop = 0;
			m_SpeakQueLen--;

			if ( !SpeakQueIn() && m_SpeakQueLen <= 0 ) {
				pVoice->SetInterest(0, 0);
				m_bVoiceEvent = FALSE;
			}

			m_pSpeakView->SpeakTextPos(FALSE, NULL, NULL);
			break;
		}
	}

	return TRUE;
}
void CMainFrame::OnSpeakText()
{
	ISpVoice *pVoice = ((CRLoginApp *)::AfxGetApp())->VoiceInit();
	CChildFrame *pChild = (CChildFrame *)MDIGetActive();
	CRLoginView *pView;
	CRLoginDoc *pDoc;

	if ( pVoice == NULL )
		return;

	if ( !m_bVoiceEvent ) {
		if ( pChild == NULL || (pView = (CRLoginView *)pChild->GetActiveView()) == NULL || (pDoc = pView->GetDocument()) == NULL )
			return;

		ULONGLONG ev = SPFEI(SPEI_WORD_BOUNDARY) | SPFEI(SPEI_SENTENCE_BOUNDARY) | SPFEI(SPEI_END_INPUT_STREAM);
		pVoice->SetInterest(ev, ev);
		pVoice->SetNotifyWindowMessage(GetSafeHwnd(), WM_SPEAKMSG, 0, 0);

		m_pSpeakView = pView;
		m_pSpeakDoc  = pDoc;

		m_SpeakQueLen = m_SpeakQuePos = m_SpeakQueTop = 0;
		m_SpeakAbs  = m_SpeakActive[0] = m_pSpeakDoc->m_TextRam.m_HisAbs;
		m_SpeakLine = m_SpeakActive[1] = m_SpeakAbs - m_pSpeakView->m_HisOfs;
		m_SpeakCols = m_SpeakActive[2] = 0;

		SpeakQueIn();

		m_bVoiceEvent = TRUE;

	} else {
		m_bVoiceEvent = FALSE;
		pVoice->SetInterest(0, 0);
		pVoice->Speak(L"", SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_NOT_XML, NULL);
		m_pSpeakView->SpeakTextPos(FALSE, NULL, NULL);
	}
}
void CMainFrame::OnSpeakBack()
{
	SpeakUpdate(0, m_SpeakActive[1] - m_SpeakActive[0] - 1);
}
void CMainFrame::OnSpeakNext()
{
	SpeakUpdate(0, m_SpeakActive[1] - m_SpeakActive[0] + 1);
}
void CMainFrame::OnUpdateSpeakText(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(((CRLoginApp *)::AfxGetApp())->VoiceInit() != NULL ? TRUE : FALSE);
	pCmdUI->SetCheck(m_bVoiceEvent ? 1 : 0);
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

void CMainFrame::OnTabmultiline()
{
	m_wndTabBar.MultiLine();
}
void CMainFrame::OnUpdateTabmultiline(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndTabBar.m_bMultiLine ? 1 : 0);
}

void CMainFrame::OnQuickConnect()
{
	CString cmds;

	if ( !m_bQuickConnect )
		return;

	m_wndQuickBar.SetComdLine(cmds);
	((CRLoginApp *)::AfxGetApp())->OpenCommandLine(cmds);
}
void CMainFrame::OnUpdateConnect(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bQuickConnect);
}
void CMainFrame::OnViewHistoryDlg()
{
	if ( m_pHistoryDlg == NULL ) {
		m_pHistoryDlg = new CHistoryDlg(NULL);
		m_pHistoryDlg->Create(IDD_HISTORYDLG, CWnd::GetDesktopWindow());
		AddTabDlg(m_pHistoryDlg, 7);
		m_pHistoryDlg->ShowWindow(SW_SHOW);
	} else
		m_pHistoryDlg->SendMessage(WM_CLOSE);
}
afx_msg void CMainFrame::OnUpdateHistoryDlg(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pHistoryDlg != NULL ? 1 : 0);
}

void CMainFrame::OnKnownhostdel()
{
	CKnownHostsDlg dlg;

	dlg.DoModal();
}

void CMainFrame::OnChartooltip()
{
	m_bCharTooltip = m_bCharTooltip ? FALSE : TRUE;
	AfxGetApp()->WriteProfileInt(_T("MainFrame"), _T("CharTooltip"), m_bCharTooltip);
}
void CMainFrame::OnUpdateChartooltip(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bCharTooltip);
}
void CMainFrame::OnAppcoledit()
{
	CAppColDlg dlg;

	dlg.DoModal();
}

static int FifoBaseComp(const void *src, const void *dis)
{
	return (int)((INT_PTR)src - (INT_PTR)*((void **)dis));
}
void CMainFrame::AddFifoActive(void *pFifoBase)
{
	int n;
	if ( !BinaryFind(pFifoBase, m_FifoActive.GetData(), sizeof(void *), (int)m_FifoActive.GetSize(), FifoBaseComp, &n) )
		m_FifoActive.InsertAt(n, pFifoBase);
}
void CMainFrame::DelFifoActive(void *pFifoBase)
{
	int n;
	if ( BinaryFind(pFifoBase, m_FifoActive.GetData(), sizeof(void *), (int)m_FifoActive.GetSize(), FifoBaseComp, &n) )
		m_FifoActive.RemoveAt(n);
}
LRESULT CMainFrame::OnFifoMsg(WPARAM wParam, LPARAM lParam)
{	
	if ( BinaryFind((void *)lParam, m_FifoActive.GetData(), sizeof(void *), (int)m_FifoActive.GetSize(), FifoBaseComp, NULL) )
		((CFifoWnd *)lParam)->MsgPump(wParam);
	return TRUE;
}
LRESULT CMainFrame::OnDocumentMsg(WPARAM wParam, LPARAM lParam)
{
	DocMsg *pDocMsg = (DocMsg *)lParam;

	ASSERT(pDocMsg != NULL);

	switch((int)wParam) {
	case DOCMSG_REMOTESTR:
		ASSERT(pDocMsg->type == DOCMSG_TYPE_STRINGA && pDocMsg->doc != NULL);
		*((CStringA *)pDocMsg->pOut) = pDocMsg->doc->RemoteStr((LPCTSTR)pDocMsg->pIn);
		break;
	case DOCMSG_LOCALSTR:
		ASSERT(pDocMsg->type == DOCMSG_TYPE_STRINGT && pDocMsg->doc != NULL);
		*((CString *)pDocMsg->pOut) = pDocMsg->doc->LocalStr((LPCSTR)pDocMsg->pIn);
		break;

	case DOCMSG_USERNAME:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == DOCMSG_TYPE_STRINGA )
			*((CStringA *)pDocMsg->pOut) = pDocMsg->doc->RemoteStr(pDocMsg->doc->m_ServerEntry.m_UserName);
		else if ( pDocMsg->type == DOCMSG_TYPE_STRINGW )
			*((CStringW *)pDocMsg->pOut) = TstrToUni(pDocMsg->doc->m_ServerEntry.m_UserName);
		break;
	case DOCMSG_PASSNAME:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == DOCMSG_TYPE_STRINGA )
			*((CStringA *)pDocMsg->pOut) = pDocMsg->doc->RemoteStr(pDocMsg->doc->m_ServerEntry.m_PassName);
		else if ( pDocMsg->type == DOCMSG_TYPE_STRINGW )
			*((CStringW *)pDocMsg->pOut) = TstrToUni(pDocMsg->doc->m_ServerEntry.m_PassName);
		break;
	case DOCMSG_TERMNAME:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == DOCMSG_TYPE_STRINGA )
			*((CStringA *)pDocMsg->pOut) = pDocMsg->doc->RemoteStr(pDocMsg->doc->m_ServerEntry.m_TermName);
		else if ( pDocMsg->type == DOCMSG_TYPE_STRINGW )
			*((CStringW *)pDocMsg->pOut) = TstrToUni(pDocMsg->doc->m_ServerEntry.m_TermName);
		break;
	case DOCMSG_ENVSTR:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == DOCMSG_TYPE_STRINGA )
			*((CStringA *)pDocMsg->pOut) = pDocMsg->doc->RemoteStr(pDocMsg->doc->m_ParamTab.m_ExtEnvStr);
		else if ( pDocMsg->type == DOCMSG_TYPE_STRINGW )
			*((CStringW *)pDocMsg->pOut) = TstrToUni(pDocMsg->doc->m_ParamTab.m_ExtEnvStr);
		break;

	case DOCMSG_SCREENSIZE:
		{
			ASSERT(pDocMsg->type == DOCMSG_TYPE_INTPTR && pDocMsg->doc != NULL);
			int *pInt = (int *)pDocMsg->pOut;
			pDocMsg->doc->m_TextRam.GetScreenSize(pInt + 0, pInt + 1, pInt + 2, pInt + 3);
		}
		break;
	case DOCMSG_LINEMODE:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == 0 ) {
			if ( pDocMsg->doc->m_TextRam.IsLineEditEnable() )
				pDocMsg->doc->m_TextRam.LineEditSwitch();
		} else {
			if ( !pDocMsg->doc->m_TextRam.IsLineEditEnable() )
				pDocMsg->doc->m_TextRam.LineEditSwitch();
		}
		break;
	case DOCMSG_TTYMODE:
		ASSERT(pDocMsg->doc != NULL);
		for ( int n = 0 ; n < pDocMsg->doc->m_ParamTab.m_TtyMode.GetSize() ; n++ ) {
			if ( pDocMsg->doc->m_ParamTab.m_TtyMode[n].opcode == (BYTE)(UINT_PTR)pDocMsg->pIn ) {
				*((DWORD *)(pDocMsg->pOut)) = pDocMsg->doc->m_ParamTab.m_TtyMode[n].param;
				break;
			}
		}
		break;

	case DOCMSG_KEEPALIVE:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->doc->m_TextRam.IsOptEnable(TO_TELKEEPAL) && pDocMsg->doc->m_TextRam.m_TelKeepAlive > 0 )
			*((DWORD *)(pDocMsg->pOut)) = ((CMainFrame *)AfxGetMainWnd())->SetTimerEvent(pDocMsg->doc->m_TextRam.m_TelKeepAlive * 1000, TIMEREVENT_SOCK | TIMEREVENT_INTERVAL, pDocMsg->pIn);
		break;

	case DOCMSG_ENTRYTEXT:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == DOCMSG_TYPE_STRINGT )
			pDocMsg->doc->EntryText(*((CString *)pDocMsg->pOut));
		break;
	case DOCMSG_LOGIT:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == DOCMSG_TYPE_STRPTR ) {
			pDocMsg->doc->LogDebug("%s", TstrToMbs(((LPCTSTR)(pDocMsg->pIn))));
			SetStatusText((LPCTSTR)(pDocMsg->pIn));
		}
		break;
	case DOCMSG_COMMAND:
		ASSERT(pDocMsg->doc != NULL);
		if ( pDocMsg->type == DOCMSG_TYPE_INT ) {
			CWnd *pWnd = pDocMsg->doc->GetAciveView();
			if ( pWnd != NULL )
				pWnd->PostMessage(WM_COMMAND, (WPARAM)(int)(UINT_PTR)(pDocMsg->pIn), NULL);
		}
		break;
	case DOCMSG_SETTIMER:
		pDocMsg->type = SetTimerEvent((int)(pDocMsg->type), (int)(UINT_PTR)(pDocMsg->pIn), pDocMsg->pOut);
		break;

	case DOCMSG_MESSAGE:
		pDocMsg->type = ::DoitMessageBox((LPCTSTR)(pDocMsg->pIn), (UINT)(pDocMsg->type), this);
		break;
	}

	return TRUE;
}
LRESULT CMainFrame::OnDockBarDrag(WPARAM wParam, LPARAM lParam)
{
	CControlBar *pBar = (CControlBar *)wParam;
	CDockContextEx *pCont = (CDockContextEx *)lParam;
	ASSERT(pBar != NULL && pCont != NULL);
	pCont->TrackLoop();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnUahDrawMenu(WPARAM wParam, LPARAM lParam)
{
	if ( !bDarkModeSupport )
		return Default();

	UAHMENU *pUahMenu = (UAHMENU *)lParam;
	CDC *pDC = CDC::FromHandle(pUahMenu->hdc);
	MENUBARINFO mbi;
	CRect rect, rcWindow;

	ASSERT(pUahMenu != NULL && pDC != NULL);

	ZeroMemory(&mbi, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	GetMenuBarInfo(OBJID_MENU, 0, &mbi);

	GetWindowRect(rcWindow);
	rect = mbi.rcBar;
    rect.OffsetRect(-rcWindow.left, -rcWindow.top);

	pDC->FillSolidRect(rect, GetAppColor(APPCOL_MENUFACE));

	return TRUE;
}
LRESULT CMainFrame::OnUahDrawMenuItem(WPARAM wParam, LPARAM lParam)
{
	if ( !bDarkModeSupport )
		return Default();

	UAHDRAWMENUITEM *pUahDrawMenuItem = (UAHDRAWMENUITEM *)lParam;
	CMenu *pMenu = CMenu::FromHandle(pUahDrawMenuItem->um.hmenu);
	int npos = pUahDrawMenuItem->umi.iPosition;
	UINT state = pUahDrawMenuItem->dis.itemState;
	CDC *pDC = CDC::FromHandle(pUahDrawMenuItem->dis.hDC);
	CRect rect = pUahDrawMenuItem->dis.rcItem;
	DWORD dwFlags = DT_SINGLELINE | DT_VCENTER | DT_CENTER;
	CString title;
	COLORREF TextColor = GetAppColor(APPCOL_MENUTEXT);
	COLORREF BackColor = GetAppColor(APPCOL_MENUFACE);
	int OldBkMode = pDC->SetBkMode(TRANSPARENT);

	ASSERT(pUahDrawMenuItem != NULL && pDC != NULL && pMenu != NULL);
	
	pMenu->GetMenuString(npos, title, MF_BYPOSITION);

	if ( (state & ODS_NOACCEL) != 0 )
        dwFlags |= DT_HIDEPREFIX;

	if ( (state & (ODS_INACTIVE | ODS_GRAYED | ODS_DISABLED)) != 0 )
		TextColor = GetAppColor(COLOR_GRAYTEXT);

	if ( (state & (ODS_HOTLIGHT | ODS_SELECTED)) != 0 )
		BackColor = GetAppColor(APPCOL_MENUHIGH);

	TextColor = pDC->SetTextColor(TextColor);
	pDC->FillSolidRect(rect, BackColor);
	pDC->DrawText(title, rect, dwFlags);

	pDC->SetTextColor(TextColor);
	pDC->SetBkMode(OldBkMode);

	return TRUE;
}

LRESULT CMainFrame::OnUpdateEmoji(WPARAM wParam, LPARAM lParam)
{
	theApp.OnUpdateEmoji(wParam, lParam);
	return TRUE;
}
