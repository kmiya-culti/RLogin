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
			// no break

		case TRANSPROC_RESTART:
			if ( m_ClientId.IsEmpty() ) {
				if ( _tcsncmp(m_ClientSecret, _T("https://script.google.com/macros/"), 33) == 0 )
					m_ProcStat = TRANSPROC_GASCRIPTREQ;
				else
					m_ProcStat = TRANSPROC_GOOGLEREQ;
			} else
				m_ProcStat = TRANSPROC_TOKENREQ;
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

				for ( n = 0 ; (m_TransPos + n) < m_TransMax && n < index.GetSize() ; n++ )
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

				for ( n = 0 ; (m_TransPos + n) < m_TransMax && n < index.GetSize() ; n++ )
					m_TransStrTab[m_TransPos + n].SetTargetString((LPCTSTR)(*ip)[n][_T("translatedText")]);

				m_TransPos += n;
				m_TransProgress.SetPos(m_TransPos);

				m_ProcStat = (m_bProcAbort ? TRANSPROC_ENDOF : TANSPROC_PAUSE);
				break;
			}

		case TRANSPROC_GASCRIPTREQ:
			{
				/****
					Google Apps Script
					m_ClientSecret == https://script.google.com/macros/s/XXXXX/exec
					{ "source" : "jp", "target" : "en", "text" : [ "Hello", "World" ] }

					function doPost(e) {
						var p = e.parameter;
						var data = JSON.parse(e.postData.getDataAsString());
						var body;

						for ( var idx in data )
							body[idx] = LanguageApp.translate(data[idx], p.source, p.target);

						var response = ContentService.createTextOutput();
						response.setMimeType(ContentService.MimeType.JSON);
						response.setContent(JSON.stringify(body));

						return response;
					}
				****/

				if ( m_TransPos >= m_TransMax || m_bProcAbort ) {
					m_ProcStat = TRANSPROC_ENDOF;
					break;
				}

				CBuffer mbs;
				CStringIndex index(TRUE, TRUE), *ip;

				index[_T("source")] = m_TransFrom;
				index[_T("target")] = m_TransTo;
				ip = &(index[_T("text")]);

				for ( int n = 0, m_TransNext = m_TransPos ; n < 50 && m_TransNext < m_TransMax ; n++, m_TransNext++ )
					ip->Add(m_TransStrTab[m_TransNext].m_SourceString);

				index.SetJsonFormat(mbs, 0, JSON_UTF8);

				CHttpThreadSession::Request(m_ClientSecret, _T("Content-Type: application/json;charset=UTF-8"), mbs, this);

				m_ProcStat = TANSPROC_GASCRIPTRESULT;
				return;
			}

		case TANSPROC_GASCRIPTRESULT:
			{
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

				if ( !index.GetJsonFormat((LPCTSTR)body) || index.Find(_T("text")) < 0) {
					m_ProcStat = TransErrMsg(NULL, (LPCTSTR)body) ? TANSPROC_RETRY : TRANSPROC_ENDOF;
					break;
				}

				ip = &(index[_T("text")]);
				for ( n = 0 ; (m_TransPos + n) < m_TransMax && n < ip->GetSize() ; n++ )
					m_TransStrTab[m_TransPos + n].SetTargetString((LPCTSTR)(*ip)[n]);

				m_TransPos += n;
				m_TransProgress.SetPos(m_TransPos);

				m_ProcStat = (m_bProcAbort ? TRANSPROC_ENDOF : TANSPROC_PAUSE);
				break;
			}

		case TANSPROC_PAUSE:
			SetTimer(1024, 5000, NULL);
			m_ProcStat = TRANSPROC_WAIT;
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
			m_ProcStat = TRANSPROC_RESTART;
			break;

		case TRANSPROC_ENDOF:
			m_TransExec.EnableWindow(TRUE);
			if ( m_TransPos >= m_TransMax )
				m_bTranstate = TRUE;
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
//	m_TransFrom    = pApp->GetProfileString(_T("ResTransDlg"), _T("TransFrom"),    _T("ja"));
	m_TransFrom    = m_ResDataBase.m_Transrate;
	m_TransTo      = pApp->GetProfileString(_T("ResTransDlg"), _T("TransTo"),      _T("en"));

	UpdateData(FALSE);

	m_TransPos = m_TransNext = 0;
	m_TransMax = m_ResDataBase.MakeTranslateTable(m_TransStrTab);
	m_MsToken.Empty();
	m_TransProgress.SetRange(0, m_TransMax);
	m_ProcStat = TRANSPROC_ENDOF;
	m_bProcAbort = FALSE;
	m_bTranstate = FALSE;

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

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_ResFileName = dlg.GetPathName();

	if ( m_bTranstate ) {
#ifdef	_M_X64
		LCID lcid;
		TCHAR name[256];

		if ( (lcid = LocaleNameToLCID(m_TransTo, 0)) != 0 ) {
			m_ResDataBase.m_LangId = LANGIDFROMLCID(lcid);
			m_ResDataBase.m_Transrate = m_TransTo;
			GetLocaleInfo(lcid, LOCALE_SNATIVELANGNAME, name, sizeof(name));
			m_ResDataBase.m_Language = name;
		} else 
#endif
		{
			m_ResDataBase.m_LangId = 0;
			m_ResDataBase.m_Transrate = m_TransTo;
			m_ResDataBase.m_Language.Empty();
		}
	}

	m_ResDataBase.SaveFile(m_ResFileName);

	CRLoginApp *pApp = (CRLoginApp *)::AfxGetApp();

	pApp->WriteProfileString(_T("ResTransDlg"), _T("ClientId"),     m_ClientId);
	pApp->WriteProfileString(_T("ResTransDlg"), _T("ClientSecret"), m_ClientSecret);
//	pApp->WriteProfileString(_T("ResTransDlg"), _T("TransFrom"),    m_TransFrom);
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
#if 1
	m_ProcStat = TRANSPROC_START;
	m_bProcAbort = FALSE;
	m_bTranstate = FALSE;
	TranslateProc(0, NULL, NULL);
#elif 0
	int seq, num;
	CString file;
	CBuffer tmp, mbs;
	CIConv iconv;

	for ( seq = num = 0 ; num < m_TransStrTab.GetSize() ; seq++ ) {
		tmp.Clear();
		mbs.Clear();
		while ( num < m_TransStrTab.GetSize() ) {
			tmp += (LPCTSTR)m_TransStrTab[num].m_SourceString;
			tmp += _T("\r\n");
			num++;
			if ( (num % 500) == 0 )
				break;
		}
		iconv.StrToRemote(_T("UTF-8"), &tmp, &mbs);

		file.Format(_T("D:\\Temp\\Translate%02d.txt"), seq);
		if ( !mbs.SaveFile(file) )
			break;
	}
#else
	int seq, num;
	CString file, text;
	CBuffer tmp;
	CIConv iconv;

	UpdateData(TRUE);

	for ( seq = num = 0 ; num < m_TransStrTab.GetSize() ; seq++ ) {
		file.Format(_T("D:\\Temp\\%s\\Translate%02d.txt"), m_TransTo, seq);
		if ( !tmp.LoadFile(file) )
			break;
		tmp.KanjiConvert(tmp.KanjiCheck(KANJI_UTF8));

		while ( tmp.ReadString(text) && num < m_TransStrTab.GetSize() ) {
			text.TrimRight(_T("\r\n"));
			m_TransStrTab[num].SetTargetString(text);
			num++;
		}
	}

	m_bTranstate = TRUE;
#endif
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
