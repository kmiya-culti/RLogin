// ComInitDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ComSock.h"
#include "ComInitDlg.h"
#include "UserFlowDlg.h"

#include <setupapi.h>
#pragma	comment(lib,"setupapi.lib")

/////////////////////////////////////////////////////////////////////////////
// CComInitDlg ダイアログ

const LPCTSTR ParityBitsName[] = { _T("NOPARITY"), _T("ODDPARITY"), _T("EVENPARITY"), _T("MARKPARITY"), _T("SPACEPARITY"), NULL };
const LPCTSTR StopBitsName[] = { _T("1"), _T("1.5"), _T("2"), NULL };

IMPLEMENT_DYNAMIC(CComInitDlg, CDialogExt)

CComInitDlg::CComInitDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CComInitDlg::IDD, pParent)
{
	m_pSock = NULL;
	m_ComIndex = 0;
	m_DataBits = _T("8");
	m_ParityBit = ParityBitsName[0];
	m_StopBits = StopBitsName[0];
	m_FlowCtrl = 1;
	m_BaudRate = _T("9600");
	m_SendWait[0] = 0;
	m_SendWait[1] = 0;
}
CComInitDlg::~CComInitDlg()
{
}

void CComInitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);

	DDX_CBIndex(pDX, IDC_COMDEVLIST, m_ComIndex);

	DDX_CBString(pDX, IDC_BAUDRATE, m_BaudRate);
	DDX_CBString(pDX, IDC_DATABITS, m_DataBits);
	DDX_CBString(pDX, IDC_PARITYBIT, m_ParityBit);
	DDX_CBString(pDX, IDC_STOPBITS, m_StopBits);
	DDX_CBIndex(pDX, IDC_FLOWCTRL, m_FlowCtrl);

	DDX_Text(pDX, IDC_SENDWAITC, m_SendWait[0]);
	DDX_Text(pDX, IDC_SENDWAITL, m_SendWait[1]);
}
BOOL CComInitDlg::GetComDeviceList()
{
	SP_DEVINFO_DATA DeviceInfoData;
	HDEVINFO DeviceInfoSet;
	GUID ClassGuid[1];
	DWORD dwIndex = 0;
	TCHAR szFriendlyName[MAX_PATH];
	TCHAR szPortName[MAX_PATH];
	DWORD dwReqSize;
	DWORD dwPropType;
	DWORD dwType;
	HKEY hKey;
	CString portIndex;

	m_ComDevList.RemoveAll();

	if ( !SetupDiClassGuidsFromName(_T("PORTS"), ClassGuid, 1, &dwReqSize) )
		return FALSE;

	if ( (DeviceInfoSet = SetupDiGetClassDevs(ClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE)) == NULL )
		return FALSE;

	ZeroMemory(&DeviceInfoData, sizeof(DeviceInfoData));
	DeviceInfoData.cbSize = sizeof(DeviceInfoData);

	while ( SetupDiEnumDeviceInfo(DeviceInfoSet, dwIndex++, &DeviceInfoData) ) {

		szFriendlyName[0] = _T('\0');
		dwReqSize = 0;

		if ( !SetupDiGetDeviceRegistryProperty(DeviceInfoSet, &DeviceInfoData, SPDRP_FRIENDLYNAME, &dwPropType, (LPBYTE)szFriendlyName, sizeof(szFriendlyName), &dwReqSize) )
			continue;

		if ( (hKey = SetupDiOpenDevRegKey(DeviceInfoSet, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ)) == NULL )
			continue;

		szPortName[0] = _T('\0');
		dwReqSize = sizeof(szPortName);
		dwType = REG_SZ;

		if ( RegQueryValueEx(hKey, _T("PortName"), 0, &dwType, (LPBYTE)&szPortName, &dwReqSize) == ERROR_SUCCESS && _tcsnicmp(szPortName, _T("COM"), 3) == 0 ) {
			portIndex.Format(_T("%03d"), _tstoi(szPortName + 3));
			m_ComDevList[portIndex] = szFriendlyName;
		}

		RegCloseKey(hKey);
	}

	SetupDiDestroyDeviceInfoList(DeviceInfoSet);

	SubclassComboBox(IDC_BAUDRATE);

	return TRUE;
}
void CComInitDlg::CommPropCombo(int Type, CComboBox *pCombo, COMMPROP *pCommProp)
{
	int n;
	static struct _MaxBaud {
		DWORD bits;
		LPCTSTR name;
	} MaxBaudTab[] = {
		{ BAUD_075,			_T("75") },			{ BAUD_110,			_T("110") },		{ BAUD_150,			_T("150") },
		{ BAUD_300,			_T("300") },		{ BAUD_600,			_T("600") },		{ BAUD_1200,		_T("1200") },
		{ BAUD_1800,		_T("1800") },		{ BAUD_2400,		_T("2400") },		{ BAUD_4800,		_T("4800") },
		{ BAUD_7200,		_T("7200") },		{ BAUD_9600,		_T("9600") },		{ BAUD_14400,		_T("14400") },
		{ BAUD_19200,		_T("19200") },		{ BAUD_USER,		_T("28800") },		{ BAUD_38400,		_T("38400") },
		{ BAUD_56K,			_T("56000") },		{ BAUD_57600,		_T("57600") },		{ BAUD_115200,		_T("115200") },
		{ BAUD_128K,		_T("128000") },		{ BAUD_USER,		_T("230400") },		{ BAUD_USER,		_T("256000") },
		{ BAUD_USER,		_T("460800") },		{ BAUD_USER,		_T("512000") },		{ BAUD_USER,		_T("921600") },
		{ 0,			NULL },
	};
	static struct _SettableData {
		DWORD bits;
		LPCTSTR name;
	} SettableDataTab[] = {
		{ DATABITS_5,			_T("5") },				{ DATABITS_6,			_T("6") },
		{ DATABITS_7,			_T("7") },				{ DATABITS_8,			_T("8") },
		{ 0,			NULL },
	};
	static struct _SelParty {
		DWORD bits;
		LPCTSTR name;
	} SelPartyTab[] = {
		{ PARITY_NONE,			ParityBitsName[0] },	{ PARITY_ODD,			ParityBitsName[1] },
		{ PARITY_EVEN,			ParityBitsName[2] },	{ PARITY_MARK,			ParityBitsName[3] },
		{ PARITY_SPACE,			ParityBitsName[4] },
		{ 0,			NULL },
	};
	static struct _StopBits {
		DWORD bits;
		LPCTSTR name;
	} StopBitsTab[] = {
		{ STOPBITS_10,			StopBitsName[0] },		{ STOPBITS_15,			StopBitsName[1] },
		{ STOPBITS_20,			StopBitsName[2] },
		{ 0,			NULL },
	};

	while ( pCombo->GetCount() > 0 )
		pCombo->DeleteString(0);

	switch(Type) {
	case COMMPROP_TYPE_BAUD:
		if ( pCommProp->wPacketLength == 0 || (pCommProp->dwSettableParams & SP_BAUD) != 0 ) {
			pCombo->EnableWindow(TRUE);
			for ( n = 0 ; MaxBaudTab[n].name != NULL ; n++ ) {
				if ( pCommProp->wPacketLength == 0 || (pCommProp->dwMaxBaud & (BAUD_USER | MaxBaudTab[n].bits)) != 0 )
					pCombo->AddString(MaxBaudTab[n].name);
			}
		} else
			pCombo->EnableWindow(FALSE);
		break;

	case COMMPROP_TYPE_DATABITS:
		if ( pCommProp->wPacketLength == 0 || (pCommProp->dwSettableParams & SP_DATABITS) != 0 ) {
			pCombo->EnableWindow(TRUE);
			for ( n = 0 ; SettableDataTab[n].name != NULL ; n++ ) {
				if ( pCommProp->wPacketLength == 0 || (pCommProp->wSettableData & SettableDataTab[n].bits) != 0 )
					pCombo->AddString(SettableDataTab[n].name);
			}
		} else
			pCombo->EnableWindow(FALSE);
		break;

	case COMMPROP_TYPE_PARITY:
		if ( pCommProp->wPacketLength == 0 || (pCommProp->dwSettableParams & SP_PARITY) != 0 ) {
			pCombo->EnableWindow(TRUE);
			for ( n = 0 ; SelPartyTab[n].name != NULL ; n++ ) {
				if ( pCommProp->wPacketLength == 0 || (pCommProp->wSettableStopParity & SelPartyTab[n].bits) != 0 )
					pCombo->AddString(SelPartyTab[n].name);
			}
		} else
			pCombo->EnableWindow(FALSE);
		break;

	case COMMPROP_TYPE_STOPBITS:
		if ( pCommProp->wPacketLength == 0 || (pCommProp->dwSettableParams & SP_STOPBITS) != 0 ) {
			pCombo->EnableWindow(TRUE);
			for ( n = 0 ; StopBitsTab[n].name != NULL ; n++ ) {
				if ( pCommProp->wPacketLength == 0 || (pCommProp->wSettableStopParity & StopBitsTab[n].bits) != 0 )
					pCombo->AddString(StopBitsTab[n].name);
			}
		} else
			pCombo->EnableWindow(FALSE);
		break;
	}
}
void CComInitDlg::CommPropCheck()
{
	HANDLE hCom;
	CString str;
	COMMPROP CommProp;
	CComboBox *pCombo;

	memset(&CommProp, 0, sizeof(CommProp));

	if ( m_pSock->m_ComPort >= 10 )
		str.Format(_T("\\\\.\\COM%d"), m_pSock->m_ComPort);
	else
		str.Format(_T("COM%d"), m_pSock->m_ComPort);

	if ( (hCom = CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) != INVALID_HANDLE_VALUE ) {
		GetCommProperties(hCom, &CommProp);
		::CloseHandle(hCom);
	}

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_BAUDRATE)) != NULL )
		CommPropCombo(COMMPROP_TYPE_BAUD, pCombo, &CommProp);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_DATABITS)) != NULL )
		CommPropCombo(COMMPROP_TYPE_DATABITS, pCombo, &CommProp);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_PARITYBIT)) != NULL )
		CommPropCombo(COMMPROP_TYPE_PARITY, pCombo, &CommProp);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_STOPBITS)) != NULL )
		CommPropCombo(COMMPROP_TYPE_STOPBITS, pCombo, &CommProp);
}

BEGIN_MESSAGE_MAP(CComInitDlg, CDialogExt)
	ON_BN_CLICKED(IDC_USERFLOW, &CComInitDlg::OnBnClickedUserflow)
	ON_BN_CLICKED(IDC_DEFCONF, &CComInitDlg::OnBnClickedDefconf)
	ON_CBN_SELCHANGE(IDC_COMDEVLIST, &CComInitDlg::OnCbnSelchangeComdevlist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComInitDlg メッセージ ハンドラー

BOOL CComInitDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	CString portIndex, FriendlyName;
	CComboBox *pCombo;

	ASSERT(m_pSock != NULL && m_pSock->m_pComConf != NULL);

	GetComDeviceList();

	portIndex.Format(_T("%03d"), m_pSock->m_ComPort);
	if ( (m_ComIndex = m_ComDevList.Find(portIndex)) < 0 ) {
		FriendlyName.Format(_T("Unkown Device (COM%d)"), m_pSock->m_ComPort);
		m_ComDevList[portIndex] = FriendlyName;
		m_ComIndex = m_ComDevList.Find(portIndex);
	}

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_COMDEVLIST)) != NULL ) {
		for ( int n = 0 ; n < m_ComDevList.GetSize() ; n++ )
			pCombo->AddString(m_ComDevList[n]);
	}

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

	CommPropCheck();
	UpdateData(FALSE);

	return TRUE;
}
void CComInitDlg::OnOK()
{
	UpdateData(TRUE);

	m_pSock->m_ComPort = _tstoi(m_ComDevList[m_ComIndex].m_nIndex);

	m_pSock->m_pComConf->dcb.BaudRate	= _tstoi(m_BaudRate);
	m_pSock->m_pComConf->dcb.ByteSize	= (BYTE)_tstol(m_DataBits);

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

	CDialogExt::OnOK();
}

void CComInitDlg::OnBnClickedUserflow()
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
}
void CComInitDlg::OnBnClickedDefconf()
{
	m_pSock->LoadComConf(NULL, m_pSock->m_ComPort);

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

	UpdateData(FALSE);
}

void CComInitDlg::OnCbnSelchangeComdevlist()
{
	UpdateData(TRUE);
	m_pSock->m_ComPort = _tstoi(m_ComDevList[m_ComIndex].m_nIndex);
	CommPropCheck();
	UpdateData(FALSE);
}
