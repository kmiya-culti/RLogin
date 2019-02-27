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

IMPLEMENT_DYNAMIC(CComInitDlg, CDialogExt)

CComInitDlg::CComInitDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CComInitDlg::IDD, pParent)
{
	m_pSock = NULL;
	m_ComIndex = 0;
	m_DataBits = 3;
	m_ParityBit = 0;
	m_StopBits = 0;
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
	DDX_CBIndex(pDX, IDC_DATABITS, m_DataBits);
	DDX_CBIndex(pDX, IDC_PARITYBIT, m_ParityBit);
	DDX_CBIndex(pDX, IDC_STOPBITS, m_StopBits);
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

	return TRUE;
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
	m_DataBits		= m_pSock->m_pComConf->dcb.ByteSize - 5;
	m_ParityBit		= m_pSock->m_pComConf->dcb.Parity - NOPARITY;
	m_StopBits		= m_pSock->m_pComConf->dcb.StopBits - ONESTOPBIT;
	m_XoffLim		= m_pSock->m_pComConf->dcb.XoffLim;
	m_XonLim		= m_pSock->m_pComConf->dcb.XonLim;
	m_XonChar		= m_pSock->m_pComConf->dcb.XonChar;
	m_XoffChar		= m_pSock->m_pComConf->dcb.XoffChar;
	m_SendWait[0]	= m_pSock->m_SendWait[0];
	m_SendWait[1]	= m_pSock->m_SendWait[1];

	m_FlowCtrl		= CComSock::GetFlowCtrlMode(&(m_pSock->m_pComConf->dcb), m_UserDef);

	UpdateData(FALSE);

	return TRUE;
}
void CComInitDlg::OnOK()
{
	UpdateData(TRUE);

	m_pSock->m_ComPort = _tstoi(m_ComDevList[m_ComIndex].m_nIndex);

	m_pSock->m_pComConf->dcb.BaudRate	= _tstoi(m_BaudRate);
	m_pSock->m_pComConf->dcb.ByteSize	= m_DataBits  + 5;
	m_pSock->m_pComConf->dcb.Parity		= m_ParityBit + NOPARITY;
	m_pSock->m_pComConf->dcb.StopBits	= m_StopBits  + ONESTOPBIT;
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
	m_DataBits		= m_pSock->m_pComConf->dcb.ByteSize - 5;
	m_ParityBit		= m_pSock->m_pComConf->dcb.Parity - NOPARITY;
	m_StopBits		= m_pSock->m_pComConf->dcb.StopBits - ONESTOPBIT;
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
}
