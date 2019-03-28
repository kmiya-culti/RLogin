// ResTransDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "afxdialogex.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ResTransDlg.h"

// CResTransDlg ダイアログ

IMPLEMENT_DYNAMIC(CResTransDlg, CDialogExt)

CResTransDlg::CResTransDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CResTransDlg::IDD, pParent)
{
}

CResTransDlg::~CResTransDlg()
{
}

void CResTransDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CLIENTID, m_ClientId);
	DDX_Text(pDX, IDC_CLIENTSECRET, m_ClientSecret);
	DDX_CBString(pDX, IDC_TRANSFROM, m_TransFrom);
	DDX_CBString(pDX, IDC_TRANSTO, m_TransTo);
	DDX_Control(pDX, IDC_TRANSPROGRES, m_TransProgress);
	DDX_Control(pDX, IDC_TRANSEXEC, m_TransExec);
}

BEGIN_MESSAGE_MAP(CResTransDlg, CDialogExt)
	ON_BN_CLICKED(IDC_TRANSEXEC, &CResTransDlg::OnBnClickedTransexec)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK1, &CResTransDlg::OnNMClickSyslink1)
	ON_WM_TIMER()
	ON_MESSAGE(WM_HTTPREQUEST, &CResTransDlg::OnHttpMessage)
END_MESSAGE_MAP()

BOOL CResTransDlg::TransErrMsg(CString *pMsg, LPCTSTR errMsg)
{
	CString msg;

	msg.Format(_T("Translate Error #%d"), m_ProcStat);

	if ( pMsg != NULL ) {
		if ( !pMsg->IsEmpty() ) {
			msg += _T('\n');
			msg += *pMsg;
		}
		delete pMsg;
	}

	if ( errMsg != NULL && *errMsg != _T('\0') ) {
		msg += _T('\n');
		msg += errMsg;
	}

	msg += _T("\nContinue Translating ?");

	if ( MessageBox(msg, _T("Error"), MB_ICONERROR | MB_YESNO) == IDYES )
		return TRUE;
	else
		return FALSE;
}
void CResTransDlg::TranslateProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	for ( ; ; ) {

		switch(m_ProcStat) {
		case TRANSPROC_START:
			UpdateData(TRUE);
			m_TransExec.EnableWindow(FALSE);
			m_ProcStat = (m_ClientId.IsEmpty() ? TRANSPROC_GOOGLEREQ : TRANSPROC_TOKENREQ);
			break;

		case TRANSPROC_TOKENREQ:
			{
				// POST https://datamarket.accesscontrol.windows.net/v2/OAuth2-13
				// content-type: application/x-www-form-urlencoded 
				// grant_type=client_credentials&client_id=id&client_secret=secret&scope=http://api.microsofttranslator.com
				//
				// { "access_token": "token" }

				if ( m_bProcAbort ) {
					m_ProcStat = TRANSPROC_ENDOF;
					break;
				}

				CStringA mbs;
				CStringIndex index(TRUE, TRUE);

				index[_T("client_id")] = m_ClientId;
				index[_T("client_secret")] = m_ClientSecret;
				index[_T("scope")] = _T("http://api.microsofttranslator.com");
				index[_T("grant_type")] = _T("client_credentials");
				index.SetQueryString(mbs, NULL, TRUE);

				CHttpThreadSession::Request(_T("https://datamarket.accesscontrol.windows.net/v2/OAuth2-13"), _T("Content-type: application/x-www-form-urlencoded;charset=UTF-8"), mbs, this);
				m_ProcStat = TANSPROC_TOKENRESULT;
				return;
			}

		case TANSPROC_TOKENRESULT:
			{
				if ( msg != WM_HTTPREQUEST || !(BOOL)wParam ) {
					m_ProcStat = TransErrMsg((CString *)lParam, NULL) ? TANSPROC_RETRY : TRANSPROC_ENDOF;
					break;
				}

				CBuffer body;
				CStringIndex index;

				if ( lParam != NULL ) {
					CIConv iconv;
					iconv.RemoteToStr(_T("UTF-8"), (CBuffer *)lParam, &body);
					delete (CBuffer *)lParam;
				}

				if ( !index.GetJsonFormat((LPCTSTR)body) || index.Find(_T("access_token")) < 0 ) {
					m_ProcStat = TransErrMsg(NULL, (LPCTSTR)body) ? TANSPROC_RETRY : TRANSPROC_ENDOF;
					break;
				}

				m_MsToken = index[_T("access_token")];

				m_ProcStat = TANSPROC_TRANSREQ;
				break;
			}

		case TANSPROC_TRANSREQ:
			{
				// GET http://api.microsofttranslator.com/V2/Ajax.svc/Translate?to=en&text=Hello%20World&oncomplete=translated
				// Authorization: Bearer $token
				//
				// GET http://api.microsofttranslator.com/V2/Ajax.svc/TranslateArray?to=en&texts=["Hello","Good"]
				// Authorization: Bearer $token

				if ( m_TransPos >= m_TransMax || m_bProcAbort ) {
					m_ProcStat = TRANSPROC_ENDOF;
					break;
				}

				int len = 0;
				CBuffer mbs;
				CStringIndex index(TRUE, TRUE);
				CString url, head;

				head.Format(_T("Authorization: Bearer %s"), m_MsToken);

				for ( m_TransNext = m_TransPos ; m_TransNext < m_TransMax && len < 1000 ; m_TransNext++ ) {
					len += m_TransStrTab[m_TransNext].m_SourceString.GetLength();
					index.Add(m_TransStrTab[m_TransNext].m_SourceString);
				}

				index.SetJsonFormat(mbs, 0, JSON_UTF8);

				url.Format(_T("http://api.microsofttranslator.com/V2/Ajax.svc/TranslateArray?from=%s&to=%s&texts="), m_TransFrom, m_TransTo);
				CHttpSession::QueryString(mbs, url);

				CHttpThreadSession::Request(url, head, NULL, this);

				m_ProcStat = TANSPROC_TRANSRESULT;
				return;
			}

		case TANSPROC_TRANSRESULT:
			{
				// [ {"From":"ja", "OriginalTextSentenceLengths":[5], "TranslatedText":"Hello", "TranslatedTextSentenceLengths":[5] },
				//   {"From":"ja", "OriginalTextSentenceLengths":[5], "TranslatedText":"Good evening", "TranslatedTextSentenceLengths":[12] } ]
				//
				// index[0]["From"] = "ja"
				// index[0]["OriginalTextSentenceLengths"][0] = "5"
				// index[0]["TranslatedText"] = "Hello"
				// index[0]["TranslatedTextSentenceLengths"][0] = "5"
				// index[1]["From"] = "ja"
				// index[1]["OriginalTextSentenceLengths"][0] = "5"
				// index[1]["TranslatedText"] = "Good evening"
				// index[1]["TranslatedTextSentenceLengths"][0] = "12"

				if ( msg != WM_HTTPREQUEST || !(BOOL)wParam ) {
					m_ProcStat = TransErrMsg((CString *)lParam, NULL) ? TANSPROC_RETRY : TRANSPROC_ENDOF;
					break;
				}

				int n;
				CBuffer body;
				CStringIndex index;

				if ( lParam != NULL ) {
					CIConv iconv;
					iconv.RemoteToStr(_T("UTF-8"), (CBuffer *)lParam, &body);
					delete (CBuffer *)lParam;
				}

				if ( !index.GetJsonFormat((LPCTSTR)body) || index.GetSize() <= 0 ) {
					m_ProcStat = TransErrMsg(NULL, (LPCTSTR)body) ? TANSPROC_RETRY : TRANSPROC_ENDOF;
					break;
				}

				for ( n = 0 ; (m_TransPos + n) < m_TransNext && n < index.GetSize() ; n++ )
					m_TransStrTab[m_TransPos + n].SetTargetString((LPCTSTR)index[n][_T("TranslatedText")]);

				m_TransPos += n;
				m_TransProgress.SetPos(m_TransPos);

				m_ProcStat = (m_bProcAbort ? TRANSPROC_ENDOF : TANSPROC_PAUSE);
				break;
			}

		case TRANSPROC_GOOGLEREQ:
			{
				// GET/POST https://translation.googleapis.com/language/translate/v2?
				// key=YOUR_API_KEY&
				// source=en&
				// target=de&
				// q=Hello%20world&
				// q=My%20name%20is%20Jeff

				if ( m_TransPos >= m_TransMax || m_bProcAbort ) {
					m_ProcStat = TRANSPROC_ENDOF;
					break;
				}

				int len = 0;
				CStringA mbs;
				CStringIndex index(TRUE, TRUE);
				CString query;
				CIConv iconv;

				query.Format(_T("source=%s&target=%s&key="), m_TransFrom, m_TransTo);
				iconv.StrToRemote(_T("UTF-8"), m_ClientSecret, mbs);
				CHttpSession::QueryString(mbs, query);

				for ( m_TransNext = m_TransPos ; m_TransNext < m_TransStrTab.GetSize() && len < 1000 ; m_TransNext++ ) {
					len += m_TransStrTab[m_TransNext].m_SourceString.GetLength();
					query += _T("&p=");
					iconv.StrToRemote(_T("UTF-8"), m_TransStrTab[m_TransNext].m_SourceString, mbs);
					CHttpSession::QueryString(mbs, query);
				}

				CHttpThreadSession::Request(_T("https://translation.googleapis.com/language/translate/v2"), _T("Content-type: application/x-www-form-urlencoded;charset=UTF-8"), TstrToMbs(query), this);

				m_ProcStat = TANSPROC_GOOGLERESULT;
				return;
			}

		case TANSPROC_GOOGLERESULT:
			{
				// { "data": { "translations": [ { "translatedText": "Hallo Welt" }, { "translatedText": "Mein Name ist Jeff" } ] } }
				//
				// index["data"]["translations"][0]["translatedText"] = "Hallo Welt"
				// index["data"]["translations"][1]["translatedText"] = "Mein Name ist Jeff"

				if ( msg != WM_HTTPREQUEST || !(BOOL)wParam ) {
					m_ProcStat = TransErrMsg((CString *)lParam, NULL) ? TANSPROC_RETRY : TRANSPROC_ENDOF;
					break;
				}

				int n;
				CBuffer body;
				CStringIndex index(TRUE, TRUE), *ip;

				if ( lParam != NULL ) {
					CIConv iconv;
					iconv.RemoteToStr(_T("UTF-8"), (CBuffer *)lParam, &body);
					delete (CBuffer *)lParam;
				}

				if ( !index.GetJsonFormat((LPCTSTR)body) || index.Find(_T("data")) < 0 ) {
					m_ProcStat = TransErrMsg(NULL, (LPCTSTR)body) ? TANSPROC_RETRY : TRANSPROC_ENDOF;
					break;
				}

				ip = &(index[_T("data")][_T("translations")]);

				for ( n = 0 ; (m_TransPos + n) < m_TransNext && n < index.GetSize() ; n++ )
					m_TransStrTab[m_TransPos + n].SetTargetString((LPCTSTR)(*ip)[n][_T("translatedText")]);

				m_TransPos += n;
				m_TransProgress.SetPos(m_TransPos);

				m_ProcStat = (m_bProcAbort ? TRANSPROC_ENDOF : TANSPROC_PAUSE);
				break;
			}
			break;

		case TANSPROC_PAUSE:
			SetTimer(1024, 100, NULL);
			m_ProcStat = (m_ClientId.IsEmpty() ? TRANSPROC_GOOGLEREQ : TANSPROC_TRANSREQ);
			return;

		case TANSPROC_RETRY:
			SetTimer(1024, 60000, NULL);
			m_ProcStat = TRANSPROC_WAIT;
			return;

		case TRANSPROC_WAIT:
			if ( m_bProcAbort ) {
				m_ProcStat = TRANSPROC_ENDOF;
				break;
			}
			m_ProcStat = (m_ClientId.IsEmpty() ? TRANSPROC_GOOGLEREQ : TRANSPROC_TOKENREQ);
			break;

		case TRANSPROC_ENDOF:
			m_TransExec.EnableWindow(TRUE);
			return;
		}
	}
}
BOOL CResTransDlg::IsTranslateProcExec()
{
	if ( m_ProcStat == TRANSPROC_ENDOF )
		return FALSE;

	if ( MessageBox(CStringLoad(IDS_TRANSLATEABORT), _T("Question"), MB_ICONQUESTION | MB_YESNO) == IDYES ) {
		m_bProcAbort = TRUE;

		if ( m_ProcStat == TRANSPROC_WAIT ) {
			KillTimer(1024);
			TranslateProc(WM_TIMER, 0, 0);
		}
	}

	return TRUE;
}

// CResTransDlg メッセージ ハンドラー

BOOL CResTransDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	m_ClientId     = pApp->GetProfileString(_T("ResTransDlg"), _T("ClientId"),     _T(""));
	m_ClientSecret = pApp->GetProfileString(_T("ResTransDlg"), _T("ClientSecret"), _T(""));
	m_TransFrom    = pApp->GetProfileString(_T("ResTransDlg"), _T("TransFrom"),    _T("ja"));
	m_TransTo      = pApp->GetProfileString(_T("ResTransDlg"), _T("TransTo"),      _T("en"));

	UpdateData(FALSE);

	m_TransPos = m_TransNext = 0;
	m_TransMax = m_ResDataBase.MakeTranslateTable(m_TransStrTab);
	m_MsToken.Empty();
	m_TransProgress.SetRange(0, m_TransMax);
	m_ProcStat = TRANSPROC_ENDOF;
	m_bProcAbort = FALSE;

	SubclassComboBox(IDC_TRANSFROM);
	SubclassComboBox(IDC_TRANSTO);

	return TRUE;
}
void CResTransDlg::OnOK()
{
	if ( IsTranslateProcExec() )
		return;

	UpdateData(TRUE);

	CFileDialog dlg(FALSE, _T("txt"), m_ResFileName, OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( dlg.DoModal() != IDOK )
		return;

	m_ResFileName = dlg.GetPathName();
	m_ResDataBase.SaveFile(m_ResFileName);

	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	pApp->WriteProfileString(_T("ResTransDlg"), _T("ClientId"),     m_ClientId);
	pApp->WriteProfileString(_T("ResTransDlg"), _T("ClientSecret"), m_ClientSecret);
	pApp->WriteProfileString(_T("ResTransDlg"), _T("TransFrom"),    m_TransFrom);
	pApp->WriteProfileString(_T("ResTransDlg"), _T("TransTo"),      m_TransTo);

	//CDialogExt::OnOK();
	DestroyWindow();
}
void CResTransDlg::OnCancel()
{
	if ( IsTranslateProcExec() )
		return;

	//CDialogExt::OnCancel();
	DestroyWindow();
}
void CResTransDlg::PostNcDestroy()
{
	CDialogExt::PostNcDestroy();
	delete this;
}

void CResTransDlg::OnBnClickedTransexec()
{
	m_ProcStat = TRANSPROC_START;
	m_bProcAbort = FALSE;
	TranslateProc(0, NULL, NULL);
}
void CResTransDlg::OnNMClickSyslink1(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMLINK pNMLink = (PNMLINK)pNMHDR;
	ShellExecute(m_hWnd, NULL, pNMLink->item.szUrl, NULL, NULL, SW_NORMAL);
	*pResult = 0;
}
void CResTransDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogExt::OnTimer(nIDEvent);

	KillTimer(nIDEvent);
	TranslateProc(WM_TIMER, NULL, NULL);
}
afx_msg LRESULT CResTransDlg::OnHttpMessage(WPARAM wParam, LPARAM lParam)
{
	TranslateProc(WM_HTTPREQUEST, wParam, lParam);
	return TRUE;
}
