// BlockDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "BlockDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CBlockDlg ダイアログ

IMPLEMENT_DYNAMIC(CBlockDlg, CDialogExt)

CBlockDlg::CBlockDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CBlockDlg::IDD, pParent)
{
	m_CodeSet = (-1);
	m_FontNum = 0;
	m_pFontNode = NULL;
	m_pFontTab = NULL;
	m_pTextRam = NULL;
	m_SelBlock = (-1);
	m_ScrollPos = 0;
	m_ScrollPage = 0;
	m_ScrollMax = 0;
	m_WheelDelta = 0;
	m_bSelCodeMode = FALSE;
	m_SelCodeSta = (-1);
	m_SelCodeEnd = (-1);
	m_LastSelBlock = (-1);
	m_DefFontName.Empty();
}
CBlockDlg::~CBlockDlg()
{
}

void CBlockDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BLOCKLIST, m_BlockList);
	DDX_Control(pDX, IDC_PREVIEWBOX, m_PreViewBox);
	DDX_Control(pDX, IDC_SCROLLBAR1, m_Scroll);
}

BEGIN_MESSAGE_MAP(CBlockDlg, CDialogExt)
	ON_WM_DRAWITEM()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_BLOCKLIST, &CBlockDlg::OnLvnItemchangedBlocklist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BLOCKLIST, &CBlockDlg::OnNMCustomdrawBlocklist)
	ON_COMMAND(ID_EDIT_COPY_ALL, &CBlockDlg::OnEditCopyAll)
	ON_COMMAND(ID_EDIT_PASTE_ALL, &CBlockDlg::OnEditPasteAll)
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

static const INITDLGTAB ItemTab[] = {
	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM },
	{ IDC_BLOCKLIST,	ITM_BTM_BTM },
	{ IDC_PREVIEWBOX,	ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ IDC_SCROLLBAR1,	ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT | ITM_BTM_BTM },
	{ 0,	0 },
};

void CBlockDlg::InitList()
{
	int n;
	int num = 0;
	int block;
	CString str;

	block = m_BlockList.GetSelectMarkData();
	m_BlockList.DeleteAllItems();

	for ( n = 0 ; n < m_UniBlockTab.GetSize() ; n++ ) {
		str.Format(_T("U+%06X"), m_UniBlockTab[n].code);
		m_BlockList.InsertItem(LVIF_TEXT | LVIF_PARAM, n, str, 0, 0, 0, n);
		m_BlockList.SetItemText(n, 1, m_UniBlockTab[n].name);
		m_BlockList.SetItemData(n, n);
		m_BlockList.SetLVCheck(n, m_UniBlockTab[n].index == m_CodeSet ? TRUE : FALSE);
		if ( block == n )
			num = n;
	}

	m_BlockList.SetItemState(num, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_PreViewBox.Invalidate(FALSE);
}
void CBlockDlg::DrawPreView(HDC hDC)
{
	int x, y;
	int cols, line;
	int width, height;
	int block, cflag, cwid, bmd;
	DWORD code, next, ucs;
	CRect frame;
	CDC *pDC = CDC::FromHandle(hDC);
	COLORREF bc = RGB(0, 0, 0);
	COLORREF fc = RGB(128,128, 128);
	COLORREF hc = RGB(64, 64, 64);
	CString str;
	CSize sz;
	LOGFONT logfont;
	CFont font, *pOldFont;
	CPen pen, *pOldPen;
	WORD Indices[8];

	if ( m_pTextRam != NULL ) {
		bc = m_pTextRam->m_ColTab[m_pTextRam->m_DefAtt.std.bcol];
		fc = m_pTextRam->m_ColTab[m_pTextRam->m_DefAtt.std.fcol];
		hc = RGB((GetRValue(fc) + GetRValue(bc)) / 2, (GetGValue(fc) + GetGValue(bc)) / 2, (GetBValue(fc) + GetBValue(bc)) / 2);
	}

	m_PreViewBox.GetClientRect(frame);

	width  = height = frame.Width()  / 16 - 2;

	pDC->FillSolidRect(frame, bc);

	if ( (block = m_SelBlock) < 0 )
		return;

	code = m_UniBlockTab[block].code;
	next = ((block + 1) < m_UniBlockTab.GetSize() ? m_UniBlockTab[block + 1].code : (UNICODE_MAX + 1));

	m_ScrollMax  = (next - code) / 16;
	m_ScrollPage = frame.Height() / (height + 2);

	if ( m_ScrollPos < 0 )
		m_ScrollPos = 0;
	else if ( m_ScrollPos >= m_ScrollMax )
		m_ScrollPos = m_ScrollMax - 1;

	SCROLLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask  = SIF_PAGE | SIF_RANGE | SIF_POS;
	info.nMin   = 0;
	info.nMax   = m_ScrollMax;
	info.nPage  = m_ScrollPage;
	info.nPos   = m_ScrollPos;
	info.nTrackPos = 0;
	m_Scroll.SetScrollInfo(&info, TRUE);

	pDC->SetTextColor(fc);
	bmd = pDC->SetBkMode(TRANSPARENT);

	ZeroMemory(&logfont, sizeof(logfont));
	logfont.lfHeight	= height;
	logfont.lfCharSet	= m_pFontNode->m_CharSet;
	logfont.lfQuality   = m_pFontNode->m_Quality;

	if ( m_bSelCodeMode && !m_DefFontName.IsEmpty() )
		_tcsncpy(logfont.lfFaceName, m_DefFontName, sizeof(logfont.lfFaceName) / sizeof(TCHAR));
	else
		_tcsncpy(logfont.lfFaceName, (!m_pFontNode->m_FontName[m_FontNum].IsEmpty() ? m_pFontNode->m_FontName[m_FontNum] : (m_pTextRam == NULL ? _T("") : m_pTextRam->m_DefFontName[m_FontNum])), sizeof(logfont.lfFaceName) / sizeof(TCHAR));

	font.CreateFontIndirect(&logfont);
	pOldFont = pDC->SelectObject(&font);

	pen.CreatePen(PS_DOT, 1, hc);
	pOldPen = pDC->SelectObject(&pen);

	code += (m_ScrollPos * 16);

	for ( line = 0 ; line < m_ScrollPage && code < next ; line++ ) {
		for ( cols = 0 ; cols < 16 && code < next ; cols++, code++ ) {
			if ( m_pTextRam != NULL ) {
				cflag = m_pTextRam->UnicodeBaseFlag(CTextRam::UCS4toUCS2(code));
				if ( (cflag & UNI_WID) != 0 )
					cwid = 2;
				else if ( (cflag & UNI_AMB) != 0 )
					cwid = (m_pTextRam->IsOptEnable(TO_RLUNIAWH) ? 1 : 2);
				else
					cwid = 1;
			} else {
				cflag = 0;
				cwid  = 2;
			}

			if ( (cflag & UNI_AMB) != 0 ) {
				x = frame.Width()  * cols / 16 + (cwid == 2 ? (width / 4) : 0);
				y = frame.Height() * line / m_ScrollPage;
				pDC->MoveTo(x, y);
				pDC->LineTo(x, y + height - 1);
				pDC->LineTo(x + width - 1, y + height - 1);
				pDC->LineTo(x + width - 1, y);
				pDC->LineTo(x, y);
			}

			x = frame.Width()  * cols / 16 + (cwid == 1 ? (width / 4) : 0);
			y = frame.Height() * line / m_ScrollPage;
			pDC->Draw3dRect(x, y, width * cwid / 2, height, hc, hc);

			ucs = CTextRam::UCS4toUCS2(code);
			if ( (ucs & 0xFFFF0000) != 0 )
				str = (WCHAR)(ucs >> 16);
			else
				str.Empty();
			str += (WCHAR)(ucs);

			if ( GetGlyphIndices(pDC->GetSafeHdc(), str, str.GetLength(), Indices, GGI_MARK_NONEXISTING_GLYPHS) != GDI_ERROR && Indices[0] != 0xFFFF )
				pDC->SetTextColor(fc);
			else
				pDC->SetTextColor(hc);

			sz = pDC->GetTextExtent(str);
			pDC->TextOut(x + (width * cwid / 2 - sz.cx) / 2, y + (height - sz.cy) / 2, str);

			if ( m_bSelCodeMode && code >= m_SelCodeSta && code <= m_SelCodeEnd )
				pDC->InvertRect(CRect(frame.Width() * cols / 16, frame.Height() * line / m_ScrollPage,
					frame.Width() * cols / 16 + width, frame.Height() * line / m_ScrollPage + height));
		}
	}

	pDC->SetBkMode(bmd);
	pDC->SelectObject(pOldFont);
	pDC->SelectObject(pOldPen);
}

/////////////////////////////////////////////////////////////////////////////
// CBlockDlg メッセージ ハンドラー

static const LV_COLUMN InitListTab[2] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0,   80, _T("Block"), 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  200, _T("Name"),  0, 0 }, 
	};

BOOL CBlockDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	int n;

	ASSERT(m_CodeSet >= 0 && m_CodeSet < CODE_MAX);
	ASSERT(m_pFontTab != NULL);
	ASSERT(m_pFontNode != NULL);

	InitItemOffset(ItemTab);

	for ( n = 0 ; n < UNIBLOCKTABMAX ; n++ )
		m_UniBlockTab.Add(UniBlockTab[n].code, (-1), UniBlockTab[n].name);

	for ( n = 0 ; n < CODE_MAX ; n++ )
		m_UniBlockTab.SetBlockCode((*m_pFontTab)[n].m_UniBlock, n);

	m_UniBlockNow = m_UniBlockTab;

	m_BlockList.SetExtendedStyle(LVS_EX_FULLROWSELECT | (m_bSelCodeMode ? 0 : LVS_EX_CHECKBOXES));
	m_BlockList.InitColumn(_T("BlockDlg"), InitListTab, 2);

	if ( !m_bSelCodeMode )
		m_BlockList.SetPopUpMenu(IDR_POPUPMENU, 4);

	InitList();

	if ( m_bSelCodeMode ) {
		if ( m_SelCodeSta != (-1) ) {
			for ( m_SelBlock = 0 ; m_SelBlock < m_UniBlockTab.GetSize() ; m_SelBlock++ ) {
				if ( m_SelCodeSta < m_UniBlockTab[m_SelBlock].code )
					break;
			}
			m_SelBlock--;
		} else if ( m_LastSelBlock != (-1) )
			m_SelBlock = m_LastSelBlock;

		if ( m_SelBlock >= 0 ) {
			if ( (n = m_BlockList.GetSelectionMark()) >= 0 )
				m_BlockList.SetItemState(n, 0, LVIS_FOCUSED | LVIS_SELECTED);
			m_BlockList.SetSelectionMark(m_SelBlock);
			m_BlockList.SetItemState(m_SelBlock, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			m_BlockList.EnsureVisible(m_SelBlock, FALSE);
			m_ScrollPos = 0;
			m_PreViewBox.Invalidate(FALSE);
		}
	}

	SetSaveProfile(_T("BlockDlg"));
	AddHelpButton(_T("#UNIBLOCK"));

	return TRUE;
}
void CBlockDlg::OnOK()
{
	int n, i;
	CString str;

	if ( !m_bSelCodeMode ) {
		m_pFontNode->m_UniBlock.Empty();

		for ( n = 0 ; n < m_BlockList.GetItemCount() ; n++ ) {
			if ( m_BlockList.GetLVCheck(n) && (i = (int)m_BlockList.GetItemData(n)) < (int)m_UniBlockTab.GetSize() && m_UniBlockTab[i].index == m_CodeSet ) {
				str.Format(_T("U+%06X"), m_UniBlockTab[i].code);
				if ( !m_pFontNode->m_UniBlock.IsEmpty() )
					m_pFontNode->m_UniBlock += _T(",");
				m_pFontNode->m_UniBlock += str;
			}
		}
	}

	CDialogExt::OnOK();
}

void CBlockDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if ( nIDCtl == IDC_PREVIEWBOX )
		DrawPreView(lpDrawItemStruct->hDC);
	else
		CDialogExt::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CBlockDlg::OnLvnItemchangedBlocklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( !m_bSelCodeMode && (pNMLV->uNewState & LVIS_STATEIMAGEMASK) != 0 ) {
		if ( m_BlockList.GetLVCheck(pNMLV->iItem) ) {
			// Check in
			m_UniBlockTab[(int)pNMLV->lParam].index = m_CodeSet;
		} else if ( (pNMLV->uOldState & LVIS_STATEIMAGEMASK) != 0 ) {
			// Check out
			if ( (m_UniBlockTab[(int)pNMLV->lParam].index = m_UniBlockNow[(int)pNMLV->lParam].index) == m_CodeSet )
				m_UniBlockTab[(int)pNMLV->lParam].index = (-1);
		}
	}

	if ( (pNMLV->uNewState & LVIS_SELECTED) != 0 ) {
		// Select item
		m_SelBlock = (int)pNMLV->lParam;
		m_ScrollPos = 0;
		m_PreViewBox.Invalidate(FALSE);
	}

	*pResult = 0;
}

void CBlockDlg::OnNMCustomdrawBlocklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	switch(pLVCD->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		if ( pLVCD->nmcd.lItemlParam >= 0 && pLVCD->nmcd.lItemlParam < m_UniBlockTab.GetSize() ) {
			if ( m_UniBlockTab[(int)pLVCD->nmcd.lItemlParam].index == m_CodeSet )
				pLVCD->clrText = RGB(255, 0, 0);
			else if ( m_UniBlockTab[(int)pLVCD->nmcd.lItemlParam].index != (-1) )
				pLVCD->clrText = RGB(127, 127, 127);
			else
				pLVCD->clrText = GetAppColor(APPCOL_CTRLTEXT);
		} else
			pLVCD->clrText = GetAppColor(APPCOL_CTRLTEXT);
        *pResult = CDRF_NEWFONT;
		break;
	default:
        *pResult = 0;
		break;
	}
}

void CBlockDlg::OnEditCopyAll()
{
	int n;
	CString tmp, str;

	for ( n = 0 ; n < m_UniBlockTab.GetSize() ; n++ ) {
		str.Format(_T("U+%06X\t"), m_UniBlockTab[n].code);
		if ( m_UniBlockTab[n].index >= 0 && m_UniBlockTab[n].index < CODE_MAX && !(*m_pFontTab)[m_UniBlockTab[n].index].m_EntryName.IsEmpty() )
			str += (*m_pFontTab)[m_UniBlockTab[n].index].m_EntryName;
		if ( m_UniBlockTab[n].name != NULL ) {
			str += _T("\t");
			str += m_UniBlockTab[n].name;
		}
		str += _T("\r\n");
		tmp += str;
	}

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(tmp);
}

void CBlockDlg::OnEditPasteAll()
{
	int n;
	CString str, tmp;
	CStringArrayExt line, pam;
	CStringBinary index;
	LPCTSTR p;

	if ( !((CMainFrame *)::AfxGetMainWnd())->CopyClipboardData(str) )
		return;


	for ( p = str ; *p != _T('\0') ; p++ ) {
		if ( *p == _T('\n') ) {
			if ( !tmp.IsEmpty() )
				line.Add(tmp);
			tmp.Empty();
		} else if ( *p != _T('\r') ) {
			tmp += *p;
		}
	}
	if ( !tmp.IsEmpty() )
		line.Add(tmp);

	for ( n = 0 ; n < CODE_MAX ; n++ ) {
		if ( !(*m_pFontTab)[n].m_EntryName.IsEmpty() )
			index[(*m_pFontTab)[n].m_EntryName] = n;
		(*m_pFontTab)[n].m_UniBlock.Empty();
	}

	m_UniBlockTab.RemoveAll();

	for ( n = 0 ; n < UNIBLOCKTABMAX ; n++ )
		m_UniBlockTab.Add(UniBlockTab[n].code, (-1), UniBlockTab[n].name);

	for ( n = 0 ; n < line.GetSize() ; n++ ) {
		pam.GetString(line[n]);
		if ( pam.GetSize() > 1 && !pam[0].IsEmpty() && !pam[1].IsEmpty() )
			m_UniBlockTab.SetBlockCode(pam[0], index[pam[1]]);
	}

	m_UniBlockNow = m_UniBlockTab;

	for ( n = 0 ; n < m_UniBlockTab.GetSize() ; n++ ) {
		if ( m_UniBlockTab[n].index > 0 && m_UniBlockTab[n].index < CODE_MAX ) {
			str.Format(_T("U+%06X"), m_UniBlockTab[n].code);
			if ( !(*m_pFontTab)[m_UniBlockTab[n].index].m_UniBlock.IsEmpty() )
				(*m_pFontTab)[m_UniBlockTab[n].index].m_UniBlock += _T(",");
			(*m_pFontTab)[m_UniBlockTab[n].index].m_UniBlock += str;
		}
	}
		
	InitList();
}

void CBlockDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch(nSBCode) {
	case SB_TOP:		// 一番上までスクロール。
		m_ScrollPos = 0;
		break;
	case SB_BOTTOM:		// 一番下までスクロール。
		m_ScrollPos = m_ScrollMax - 1;
		break;
	case SB_ENDSCROLL:	// スクロール終了。
		break;
	case SB_LINEDOWN:	// 1 行下へスクロール。
		m_ScrollPos += 1;
		break;
	case SB_LINEUP:		// 1 行上へスクロール。
		m_ScrollPos -= 1;
		break;
	case SB_PAGEDOWN:	// 1 ページ下へスクロール。
		m_ScrollPos += m_ScrollPage;
		break;
	case SB_PAGEUP:		// 1 ページ上へスクロール。
		m_ScrollPos -= m_ScrollPage;
		break;
	case SB_THUMBPOSITION:	// 絶対位置へスクロール。現在位置は nPos で提供。
		m_ScrollPos = nPos;
		break;
	case SB_THUMBTRACK:		// 指定位置へスクロール ボックスをドラッグ。現在位置は nPos で提供。
		m_ScrollPos = nPos;
		break;
	}

	m_PreViewBox.Invalidate(FALSE);
	CDialogExt::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CBlockDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	DWORD code;

	if ( !m_bSelCodeMode )
		return;

	m_PreViewBox.GetClientRect(rect);
	m_PreViewBox.ClientToScreen(rect);
	ScreenToClient(rect);

	if ( !rect.PtInRect(point) )
		return;
	
	if ( m_SelBlock < 0 )
		return;

	if ( ((point.y - rect.top) * m_ScrollPage / rect.Height() + m_ScrollPos) >= m_ScrollMax )
		return;

	code = m_UniBlockTab[m_SelBlock].code;
	code += (point.x - rect.left) * 16 / rect.Width();
	code += ((point.y - rect.top) * m_ScrollPage / rect.Height() + m_ScrollPos) * 16;

	m_SelCodeSta = code;

	if ( m_SelCodeEnd == (-1) )
		m_SelCodeEnd = code;

	if ( m_SelCodeSta > m_SelCodeEnd )
		m_SelCodeEnd = m_SelCodeSta;

	m_PreViewBox.Invalidate(FALSE);
}
void CBlockDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	DWORD code;

	if ( !m_bSelCodeMode )
		return;

	m_PreViewBox.GetClientRect(rect);
	m_PreViewBox.ClientToScreen(rect);
	ScreenToClient(rect);

	if ( !rect.PtInRect(point) )
		return;
	
	if ( m_SelBlock < 0 )
		return;

	if ( ((point.y - rect.top) * m_ScrollPage / rect.Height() + m_ScrollPos) >= m_ScrollMax )
		return;

	code = m_UniBlockTab[m_SelBlock].code;
	code += (point.x - rect.left) * 16 / rect.Width();
	code += ((point.y - rect.top) * m_ScrollPage / rect.Height() + m_ScrollPos) * 16;

	m_SelCodeEnd = code;

	if ( m_SelCodeSta == (-1) )
		m_SelCodeSta = code;

	if ( m_SelCodeEnd < m_SelCodeSta )
		m_SelCodeSta = m_SelCodeEnd;

	m_PreViewBox.Invalidate(FALSE);
}

BOOL CBlockDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	int n;

	m_WheelDelta += zDelta;

	if ( (m_WheelDelta / 40) == 0 )
		return CDialogExt::OnMouseWheel(nFlags, zDelta, pt);

	m_ScrollPos -= (m_WheelDelta / 40);
	m_WheelDelta %= 40;

	if ( m_ScrollPos < 0 ) {
		if ( --m_SelBlock < 0 )
			m_SelBlock = 0;

		CRect frame;
		m_PreViewBox.GetClientRect(frame);

		DWORD code = m_UniBlockTab[m_SelBlock].code;
		DWORD next = ((m_SelBlock + 1) < m_UniBlockTab.GetSize() ? m_UniBlockTab[m_SelBlock + 1].code : (UNICODE_MAX + 1));
		int height = frame.Width()  / 16 - 2;

		m_ScrollMax  = (next - code) / 16;
		m_ScrollPage = frame.Height() / (height + 2);
		if ( (m_ScrollPos = m_ScrollMax - m_ScrollPage) < 0 )
			m_ScrollPos = 0;

	} else if ( m_ScrollPos >= (m_ScrollMax - (m_ScrollPage / 2)) ) {
		if ( ++m_SelBlock >= (int)m_UniBlockTab.GetSize() )
			m_SelBlock = (int)m_UniBlockTab.GetSize() - 1;

		m_ScrollPos = 0;
	}

	if ( (n = m_BlockList.GetSelectionMark()) >= 0 && n != m_SelBlock ) {
		int savePos = m_ScrollPos;

		m_BlockList.SetItemState(n, 0, LVIS_FOCUSED | LVIS_SELECTED);

		m_BlockList.SetSelectionMark(m_SelBlock);
		m_BlockList.SetItemState(m_SelBlock, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_BlockList.EnsureVisible(m_SelBlock, FALSE);

		m_ScrollPos = savePos;
	}

	m_PreViewBox.Invalidate(FALSE);

	return CDialogExt::OnMouseWheel(nFlags, zDelta, pt);
}
