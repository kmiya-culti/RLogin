// ComMoniDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ComSock.h"
#include "ComMoniDlg.h"
#include "ComInitDlg.h"
#include "UserFlowDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CComMoniDlg ダイアログ

IMPLEMENT_DYNAMIC(CComMoniDlg, CDialogExt)

CComMoniDlg::CComMoniDlg(class CComSock *pSock, CWnd* pParent /*=NULL*/)
	: CDialogExt(CComMoniDlg::IDD, pParent)
{
	m_pSock = pSock;
	m_bActive = FALSE;
	m_DataBits = _T("8");
	m_ParityBit = _T("NOPARITY");
	m_StopBits = _T("1");
	m_FlowCtrl = 1;
	m_BaudRate = _T("9600");
	m_SendWait[0] = 0;
	m_SendWait[1] = 0;
	m_ModStsLast = 0;
	m_ModStsXor  = 0;
	m_ModStsDisp = 0;
	m_LastError = 0;

	for ( int n = 0 ; n < 9 ; n++ )
		m_PortColor[n] = 0;

	m_DataTop = 0;
	m_pData = new GRAPDATA[GRAPDATA_MAX];
	ZeroMemory(m_pData, sizeof(GRAPDATA) * GRAPDATA_MAX);
}

CComMoniDlg::~CComMoniDlg()
{
	delete [] m_pData;
}

void CComMoniDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_CBStringExact(pDX, IDC_BAUDRATE, m_BaudRate);
	DDX_CBStringExact(pDX, IDC_DATABITS, m_DataBits);
	DDX_CBStringExact(pDX, IDC_PARITYBIT, m_ParityBit);
	DDX_CBStringExact(pDX, IDC_STOPBITS, m_StopBits);
	DDX_CBIndex(pDX, IDC_FLOWCTRL, m_FlowCtrl);

	DDX_Text(pDX, IDC_SENDWAITC, m_SendWait[0]);
	DDX_Text(pDX, IDC_SENDWAITL, m_SendWait[1]);

	for ( int n = 0 ; n < 6 ; n++ )
		DDX_Control(pDX, IDC_CHECK1 + n, m_ErrBtn[n]);

	for ( int n = 0 ; n < 9 ; n++ )
		DDX_Control(pDX, IDC_COM_DCD + n, m_ColBox[n]);

	DDX_Control(pDX, IDC_COMGRAP, m_GrapBox);
	DDX_Control(pDX, IDC_COMCONFIG, m_ComConfig);
	DDX_Control(pDX, IDC_CTRL_DTR, m_ComCtrlDtr);
	DDX_Control(pDX, IDC_CTRL_RTS, m_ComCtrlRts);
}

BEGIN_MESSAGE_MAP(CComMoniDlg, CDialogExt)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_DRAWITEM()
	ON_WM_TIMER()
	ON_COMMAND(ID_COMM_EVENT_MONI, &CComMoniDlg::OnCommEvent)
	ON_BN_CLICKED(IDC_COMCONFIG, &CComMoniDlg::OnBnClickedComconfig)
	ON_BN_CLICKED(IDC_CLEARERROR, &CComMoniDlg::OnBnClickedClearerror)
	ON_BN_CLICKED(IDC_CTRL_DTR, &CComMoniDlg::OnBnClickedCtrlDtr)
	ON_BN_CLICKED(IDC_CTRL_RTS, &CComMoniDlg::OnBnClickedCtrlRts)
	ON_CBN_EDITCHANGE(IDC_BAUDRATE, &CComMoniDlg::OnCbnSelchange)
	ON_CBN_SELCHANGE(IDC_BAUDRATE, &CComMoniDlg::OnCbnSelchange)
	ON_CBN_SELCHANGE(IDC_DATABITS, &CComMoniDlg::OnCbnSelchange)
	ON_CBN_SELCHANGE(IDC_PARITYBIT, &CComMoniDlg::OnCbnSelchange)
	ON_CBN_SELCHANGE(IDC_STOPBITS, &CComMoniDlg::OnCbnSelchange)
	ON_CBN_SELCHANGE(IDC_FLOWCTRL, &CComMoniDlg::OnCbnSelchange)
	ON_EN_UPDATE(IDC_SENDWAITC, &CComMoniDlg::OnCbnSelchange)
	ON_EN_UPDATE(IDC_SENDWAITL, &CComMoniDlg::OnCbnSelchange)
	ON_BN_CLICKED(IDC_USERFLOW, &CComMoniDlg::OnBnClickedUserflow)
END_MESSAGE_MAP()

void CComMoniDlg::SetCommError()
{
	m_ErrBtn[0].SetCheck((m_pSock->m_CommError & CE_RXOVER)   != 0 ? BST_CHECKED : BST_UNCHECKED);
	m_ErrBtn[1].SetCheck((m_pSock->m_CommError & CE_OVERRUN)  != 0 ? BST_CHECKED : BST_UNCHECKED);
	m_ErrBtn[2].SetCheck((m_pSock->m_CommError & CE_RXPARITY) != 0 ? BST_CHECKED : BST_UNCHECKED);
	m_ErrBtn[3].SetCheck((m_pSock->m_CommError & CE_FRAME)    != 0 ? BST_CHECKED : BST_UNCHECKED);
	m_ErrBtn[4].SetCheck((m_pSock->m_CommError & CE_BREAK)    != 0 ? BST_CHECKED : BST_UNCHECKED);
	m_ErrBtn[5].SetCheck((m_pSock->m_CommError & CE_TXFULL)   != 0 ? BST_CHECKED : BST_UNCHECKED);
}
void CComMoniDlg::SetModemStatus()
{
	m_PortColor[PORTNUM_CTS] = (m_ModStsDisp & MS_CTS_ON)  != 0 ? PORTCOLOR_INON : PORTCOLOR_OFF;
	m_PortColor[PORTNUM_DSR] = (m_ModStsDisp & MS_DSR_ON)  != 0 ? PORTCOLOR_INON : PORTCOLOR_OFF;
	m_PortColor[PORTNUM_RI]  = (m_ModStsDisp & MS_RING_ON) != 0 ? PORTCOLOR_INON : PORTCOLOR_OFF;
	m_PortColor[PORTNUM_DCD] = (m_ModStsDisp & MS_RLSD_ON) != 0 ? PORTCOLOR_INON : PORTCOLOR_OFF;

	switch(m_pSock->m_pComConf->dcb.fDtrControl) {
	case DTR_CONTROL_DISABLE:   m_PortColor[PORTNUM_DTR] = PORTCOLOR_OFF; break;
	case DTR_CONTROL_ENABLE:	m_PortColor[PORTNUM_DTR] = PORTCOLOR_OUTON; break;
	case DTR_CONTROL_HANDSHAKE:	m_PortColor[PORTNUM_DTR] = PORTCOLOR_OUTCTRL; break;
	}

	switch(m_pSock->m_pComConf->dcb.fRtsControl) {
	case RTS_CONTROL_DISABLE:   m_PortColor[PORTNUM_RTS] = PORTCOLOR_OFF; break;
	case RTS_CONTROL_ENABLE:    m_PortColor[PORTNUM_RTS] = PORTCOLOR_OUTON; break;
	case RTS_CONTROL_HANDSHAKE: m_PortColor[PORTNUM_RTS] = PORTCOLOR_OUTCTRL; break;
	case RTS_CONTROL_TOGGLE:    m_PortColor[PORTNUM_RTS] = PORTCOLOR_TOGGLE; break;
	}

	m_PortColor[PORTNUM_RxD] = m_RecvByte > 0 ? PORTCOLOR_INON  : PORTCOLOR_OFF;
	m_PortColor[PORTNUM_TxD] = m_SendByte > 0 ? PORTCOLOR_OUTON : PORTCOLOR_OFF;

	for ( int n = 0 ; n < 9 ; n++ )
		m_ColBox[n].Invalidate(FALSE);

	m_ComCtrlDtr.EnableWindow(m_pSock->m_pComConf->dcb.fDtrControl <= DTR_CONTROL_ENABLE ? TRUE : FALSE);
	m_ComCtrlRts.EnableWindow(m_pSock->m_pComConf->dcb.fRtsControl <= RTS_CONTROL_ENABLE ? TRUE : FALSE);
}
void CComMoniDlg::DrawGrapData(CDC *pDC, CRect &rect)
{
	int a, n, i;
	int x, y;
	int height = rect.Height() - 7;
	int max = m_pSock->m_pComConf->dcb.BaudRate / 10;
	CPen PenLine[4];
	CPen *pOldPen;
	static DWORD ModStsBits[5] = { MS_RLSD_ON, MS_DSR_ON, MS_CTS_ON, MS_RING_ON, 0x8000 };
	static COLORREF ModStsCol[5] = { RGB(255, 255, 0), RGB(255, 0, 255), RGB(0, 255, 255), RGB(255, 255, 255), RGB(255, 0, 0) };

	pDC->FillSolidRect(rect, RGB(0, 0, 0));

	PenLine[0].CreatePen(PS_SOLID, 0, RGB(128, 255, 128));
	PenLine[1].CreatePen(PS_SOLID, 0, RGB(255, 128, 128));
	PenLine[2].CreatePen(PS_SOLID, 0, RGB( 64,  64, 128));
	PenLine[3].CreatePen(PS_SOLID, 0, RGB(255, 255,   0));

	pOldPen = pDC->SelectObject(&PenLine[2]);

	for ( n = 1 ; n <= 4 ; n++ ) {
		pDC->MoveTo(rect.left,  rect.bottom - height * n / 4);
		pDC->LineTo(rect.right, rect.bottom - height * n / 4);
	}

	for ( n = 1 ; n < 6 ; n++ ) {
		pDC->MoveTo(rect.left + rect.Width() * n / 6, rect.top);
		pDC->LineTo(rect.left + rect.Width() * n / 6, rect.bottom);
	}

	pDC->SelectObject(&PenLine[3]);
	i = m_DataTop;
	for ( n = 0 ; n < GRAPDATA_MAX ; n++ ) {
		if ( --i < 0 )
			i = GRAPDATA_MAX - 1;

		x = rect.right - n * rect.Width() / GRAPDATA_MAX;
		y = rect.top + 1;

		for ( a = 0 ; a < 5 ; a++ ) {
			if ( (m_pData[i].ModSts[1] & ModStsBits[a]) != 0 )
				pDC->SetPixelV(x, y + a, ModStsCol[a]);
		}
	}

	for ( a = 0 ; a < 2 ; a++ ) {
		pDC->SelectObject(&PenLine[a]);

		i = m_DataTop;
		for ( n = 0 ; n < GRAPDATA_MAX ; n++ ) {
			if ( --i < 0 )
				i = GRAPDATA_MAX - 1;

			x = rect.right - n * rect.Width() / GRAPDATA_MAX;
			if ( (y = m_pData[i].ByteSec[a] * height / max) > height )
				y = height;
			y = rect.bottom - y;

			if ( n == 0 )
				pDC->MoveTo(x, y);
			else
				pDC->LineTo(x, y);
		}
	}

	pDC->SelectObject(pOldPen);
}

/////////////////////////////////////////////////////////////////////////////
// CComMoniDlg メッセージ ハンドラー

BOOL CComMoniDlg::OnInitDialog()
{
	CComboBox *pCombo;
	COMMPROP CommProp;

	CDialogExt::OnInitDialog();

	ASSERT(m_pSock != NULL && m_pSock->m_pComConf != NULL);

	m_BaudRate.Format(_T("%d"), m_pSock->m_pComConf->dcb.BaudRate);
	m_DataBits.Format(_T("%d"), m_pSock->m_pComConf->dcb.ByteSize);
	m_ParityBit		= ParityBitsName[m_pSock->m_pComConf->dcb.Parity];
	m_StopBits		= StopBitsName[m_pSock->m_pComConf->dcb.StopBits];
	m_XoffLim		= m_pSock->m_pComConf->dcb.XoffLim;
	m_XonLim		= m_pSock->m_pComConf->dcb.XonLim;
	m_XonChar		= m_pSock->m_pComConf->dcb.XonChar;
	m_XoffChar		= m_pSock->m_pComConf->dcb.XoffChar;
	m_SendWait[0]	= m_pSock->m_SendWait[0];
	m_SendWait[1]	= m_pSock->m_SendWait[1];

	m_FlowCtrl		= CComSock::GetFlowCtrlMode(&(m_pSock->m_pComConf->dcb), m_UserDef);

	m_ComConfig.EnableWindow(FALSE);

	memset(&CommProp, 0, sizeof(CommProp));

	if ( m_pSock->m_hCom != INVALID_HANDLE_VALUE )
		GetCommProperties(m_pSock->m_hCom, &CommProp);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_BAUDRATE)) != NULL )
		CComInitDlg::CommPropCombo(COMMPROP_TYPE_BAUD, pCombo, &CommProp);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_DATABITS)) != NULL )
		CComInitDlg::CommPropCombo(COMMPROP_TYPE_DATABITS, pCombo, &CommProp);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_PARITYBIT)) != NULL )
		CComInitDlg::CommPropCombo(COMMPROP_TYPE_PARITY, pCombo, &CommProp);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_STOPBITS)) != NULL )
		CComInitDlg::CommPropCombo(COMMPROP_TYPE_STOPBITS, pCombo, &CommProp);

	UpdateData(FALSE);

	m_pSock->GetRecvSendByteSec(m_RecvByte, m_SendByte);
	m_RecvByte = m_SendByte = 0;

	m_LastError = m_pSock->m_CommError;
	m_ModStsLast = m_ModStsDisp = m_pSock->m_ModemStatus;
	m_ModStsXor = 0;

	SetCommError();
	SetModemStatus();
	::FormatErrorReset();

	m_bActive = TRUE;

	SetTimer(1024, 1000, NULL);

	SubclassComboBox(IDC_BAUDRATE);

	SetSaveProfile(_T("ComMoniDlg"));
	AddHelpButton(_T("#MONITOR"));

	return TRUE;
}
void CComMoniDlg::OnOK()
{
	//CDialogExt::OnOK();
}
void CComMoniDlg::OnCancel()
{
	//m_pSock->m_pComMoni = NULL;
	//CDialogExt::OnCancel();
}
void CComMoniDlg::OnClose()
{
	m_bActive = FALSE;
	m_pSock->m_pComMoni = NULL;
	CDialogExt::OnClose();
	DestroyWindow();
}
void CComMoniDlg::OnDestroy()
{
	m_bActive = FALSE;
	m_pSock->m_pComMoni = NULL;
	CDialogExt::OnDestroy();
}
void CComMoniDlg::PostNcDestroy()
{
	CDialogExt::PostNcDestroy();
	delete this;
}

void CComMoniDlg::OnBnClickedUserflow()
{
	DCB dcb;
	CUserFlowDlg dlg(this);

	memcpy(&dcb, &(m_pSock->m_pComConf->dcb), sizeof(dcb));

	UpdateData(TRUE);

	CComSock::SetFlowCtrlMode(&dcb, m_FlowCtrl, m_UserDef);

	dcb.XoffLim  = m_XoffLim;
	dcb.XonLim   = m_XonLim;
	dcb.XonChar  = m_XonChar;
	dcb.XoffChar = m_XoffChar;

	dlg.m_pDCB = &dcb;

	if ( dlg.DoModal() != IDOK )
		return;

	m_XoffLim  = dcb.XoffLim;
	m_XonLim   = dcb.XonLim;
	m_XonChar  = dcb.XonChar;
	m_XoffChar = dcb.XoffChar;

	m_FlowCtrl = CComSock::GetFlowCtrlMode(&dcb, m_UserDef);

	UpdateData(FALSE);

	m_ComConfig.EnableWindow(TRUE);
}
void CComMoniDlg::OnBnClickedComconfig()
{
	UpdateData(TRUE);

	m_pSock->m_pComConf->dcb.BaudRate	= _tstoi(m_BaudRate);
	m_pSock->m_pComConf->dcb.ByteSize	= (BYTE)_tstoi(m_DataBits);

	m_pSock->m_pComConf->dcb.Parity = NOPARITY;
	for ( int n = 0 ; ParityBitsName[n] != NULL ; n++ ) {
		if ( _tcscmp(ParityBitsName[n], m_ParityBit) == 0 )
			m_pSock->m_pComConf->dcb.Parity	= n;

	}

	m_pSock->m_pComConf->dcb.StopBits	= ONESTOPBIT;
	for ( int n = 0 ; StopBitsName[n] != NULL ; n++ ) {
		if ( _tcscmp(StopBitsName[n], m_StopBits) == 0 )
			m_pSock->m_pComConf->dcb.StopBits	= n;

	}

	m_pSock->m_pComConf->dcb.XoffLim	= m_XoffLim;
	m_pSock->m_pComConf->dcb.XonLim		= m_XonLim;
	m_pSock->m_pComConf->dcb.XonChar	= m_XonChar;
	m_pSock->m_pComConf->dcb.XoffChar	= m_XoffChar;

	m_pSock->m_SendWait[0]	= m_SendWait[0];
	m_pSock->m_SendWait[1]	= m_SendWait[1];

	CComSock::SetFlowCtrlMode(&(m_pSock->m_pComConf->dcb), m_FlowCtrl, m_UserDef);

	if ( m_pSock->SetComConf() ) {
		m_pSock->SaveComConf(m_pSock->m_pDocument->m_ServerEntry.m_HostNameProvs);
		m_pSock->m_pDocument->SetModifiedFlag(TRUE);
	} else
		ThreadMessageBox(_T("SetCommState Error"));

	SetModemStatus();

	m_ComConfig.EnableWindow(FALSE);
}
void CComMoniDlg::OnBnClickedClearerror()
{
	m_pSock->m_CommError = 0;
	SetCommError();
}

void CComMoniDlg::OnBnClickedCtrlDtr()
{
	if ( ++m_pSock->m_pComConf->dcb.fDtrControl > DTR_CONTROL_ENABLE )
		m_pSock->m_pComConf->dcb.fDtrControl = DTR_CONTROL_DISABLE;

	m_pSock->SetComCtrl(m_pSock->m_pComConf->dcb.fDtrControl == DTR_CONTROL_DISABLE ? CLRDTR : SETDTR);
	SetModemStatus();

	m_ComConfig.EnableWindow(TRUE);
}
void CComMoniDlg::OnBnClickedCtrlRts()
{
	if ( ++m_pSock->m_pComConf->dcb.fRtsControl > RTS_CONTROL_ENABLE )
		m_pSock->m_pComConf->dcb.fRtsControl = RTS_CONTROL_DISABLE;

	m_pSock->SetComCtrl(m_pSock->m_pComConf->dcb.fRtsControl == RTS_CONTROL_DISABLE ? CLRRTS : SETRTS);
	SetModemStatus();

	m_ComConfig.EnableWindow(TRUE);
}

void CComMoniDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	CRect rect;
	static COLORREF ColTab[] = { RGB(0, 0, 0), RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255), RGB(128, 128, 128) };

	if ( nIDCtl >= IDC_COM_DCD && nIDCtl <= IDC_COM_RI ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_ColBox[nIDCtl - IDC_COM_DCD].GetClientRect(rect);
		dc.FillSolidRect(rect, ColTab[m_PortColor[nIDCtl - IDC_COM_DCD]]);
		dc.Detach();
		return;

	} else if ( nIDCtl == IDC_COMGRAP ) {
		dc.Attach(lpDrawItemStruct->hDC);
		m_GrapBox.GetClientRect(rect);
		DrawGrapData(&dc, rect);
		dc.Detach();
		return;
	}

	CDialogExt::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CComMoniDlg::OnCommEvent()
{
	if ( m_LastError != m_pSock->m_CommError )
		m_ModStsXor |= 0x8000;
	m_LastError = m_pSock->m_CommError;

	m_ModStsXor |= (m_ModStsLast ^ m_pSock->m_ModemStatus);
	m_ModStsLast = m_pSock->m_ModemStatus;

	SetCommError();
}

void CComMoniDlg::OnTimer(UINT_PTR nIDEvent)
{
	m_pSock->GetRecvSendByteSec(m_RecvByte, m_SendByte);

	// 変化したビットは、前状態から反転。変化しなかったビットは、現状態に設定
	m_ModStsDisp = (m_ModStsDisp & m_ModStsXor) | (m_pSock->m_ModemStatus & ~m_ModStsXor);
	m_ModStsDisp ^= m_ModStsXor;

	m_pData[m_DataTop].ByteSec[0] = m_RecvByte;
	m_pData[m_DataTop].ByteSec[1] = m_SendByte;
	m_pData[m_DataTop].ModSts[0] = m_ModStsDisp;
	m_pData[m_DataTop].ModSts[1] = m_ModStsXor;

	m_ModStsXor = 0;
	SetModemStatus();

	if ( ++m_DataTop >= GRAPDATA_MAX )
		m_DataTop = 0;

	m_GrapBox.Invalidate(FALSE);

	CDialogExt::OnTimer(nIDEvent);
}

void CComMoniDlg::OnCbnSelchange()
{
	m_ComConfig.EnableWindow(TRUE);
}

