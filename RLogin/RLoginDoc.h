/////////////////////////////////////////////////////////////////////////////
// CRLoginDoc

#pragma once

#include "Data.h"
#include "TextRam.h"
#include "ExtSocket.h"
#include "BPlus.h"
#include "ZModem.h"
#include "Kermit.h"
#include "FileUpDown.h"

#define	DOCUMENT_MAX		200

#define	DOCTYPE_NONE		(-1)
#define	DOCTYPE_REGISTORY	0
#define	DOCTYPE_ENTRYFILE	1
#define	DOCTYPE_MULTIFILE	2
#define	DOCTYPE_SESSION		3

#define	UPDATE_INVALIDATE	0
#define	UPDATE_TEXTRECT		1
#define	UPDATE_GOTOXY		2
#define	UPDATE_CLIPERA		3
#define	UPDATE_INITPARA		4
#define	UPDATE_VISUALBELL	5
#define	UPDATE_INITSIZE		6
#define	UPDATE_SETCURSOR	7
#define	UPDATE_TYPECARET	8
#define	UPDATE_TEKFLUSH		9
#define	UPDATE_RESIZE		10
#define	UPDATE_CANCELBTN	11
#define	UPDATE_DISPINDEX	12
#define	UPDATE_WAKEUP		13
#define	UPDATE_SCROLLOUT	14
#define	UPDATE_UPDATEWINDOW	15
#define	UPDATE_CLIPCLAER	16
#define	UPDATE_DISPMSG		17

#define	CARET_MOVE			0
#define	CARET_CREATE		1
#define	CARET_DESTROY		2

#define	PROTO_DIRECT		0
#define	PROTO_LOGIN			1
#define	PROTO_TELNET		2
#define	PROTO_SSH			3
#define	PROTO_COMPORT		4
#define	PROTO_PIPE			5

#define	DELAY_CHAR_USEC		100
#define	DELAY_LINE_MSEC		500
#define	DELAY_RECV_MSEC		100
#define	DELAY_CLOSE_SOCKET	3000

#define	LOGDEBUG_NONE		0
#define	LOGDEBUG_RECV		1
#define	LOGDEBUG_SEND		2
#define	LOGDEBUG_INSIDE		3
#define	LOGDEBUG_FLASH		4

#define	SLEEPSTAT_CONNECT	0
#define	SLEEPSTAT_CLOSE		1
#define	SLEEPSTAT_DISABLE	2
#define	SLEEPSTAT_ENABLE	3
#define	SLEEPSTAT_RESET		4

class CRLoginDoc : public CDocument
{
	DECLARE_DYNCREATE(CRLoginDoc)

protected: // シリアライズ機能のみから作成します。
	CRLoginDoc();

public:
	virtual ~CRLoginDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// オペレーション
public:
	int m_DocSeqNumber;
	CExtSocket *m_pSock;
	CTextRam m_TextRam;
	CKeyNodeTab m_KeyTab;
	CKeyMacTab m_KeyMac;
	CParamTab m_ParamTab;
	CServerEntry m_ServerEntry;
	CFileExt *m_pLogFile;
	CBPlus *m_pBPlus;
	CZModem *m_pZModem;
	CKermit *m_pKermit;
	CFileUpDown *m_pFileUpDown;
	int m_DelayFlag;
	CBuffer m_DelayBuf;
	BOOL m_bDelayPast;
	int m_DelayLen;
	int m_TimerIdLine;
	int m_TimerIdRecv;
	int m_TimerIdCrLf;
	CMainFrame *m_pMainWnd;
	CString m_SockStatus;
	CStrScript *m_pStrScript;
	CString m_ErrorPrompt;
	CString m_SearchStr;
	CStringA m_WorkMbs;
	CString m_WorkStr;
	class CScript *m_pScript;
	BOOL m_InPane;
	int m_AfterId;
	CString m_CmdsPath;
	int m_LogSendRecv;
	CString m_ScriptFile;
	BOOL m_bReqDlg;
	CString m_CmdLine;
	CString m_TitleName;
	BOOL m_bCastLock;
	BOOL m_bExitPause;
	time_t m_ConnectTime;
	time_t m_CloseTime;
	class CStatusDlg *m_pStatusWnd;
	class CStatusDlg *m_pMediaCopyWnd;
	CWordArray m_OptFixCheck;
	BOOL m_bSleepDisable;

	static void LoadOption(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab);
	static void SaveOption(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab);
	static void LoadIndex(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab, CStringIndex &index);
	static void SaveIndex(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab, CStringIndex &index);
	static void DiffIndex(CServerEntry &ServerEntry, CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab, CServerEntry &OrigEntry, CStringIndex &index);
	static void LoadInitOption(CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab);
	static void LoadDefOption(CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab);
	static void SaveDefOption(CTextRam &TextRam, CKeyNodeTab &KeyTab, CKeyMacTab &KeyMac, CParamTab &ParamTab);

	inline void SetIndex(int mode, CStringIndex &index) { 	if ( mode ) SaveIndex(m_ServerEntry, m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab, index); else LoadIndex(m_ServerEntry, m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab, index); }
	void SetMenu(CMenu *pMenu);
	BOOL InitDocument();

	static LPCTSTR GetSpecialFolder(LPCTSTR env);
	static void EnvironText(CString &env, CString &str);
	static void EnvironPath(CString &path);
	BOOL EntryText(CString &name, LPCWSTR match = NULL, BOOL bCtrl = FALSE);

	BOOL ScriptInit();
	void SendBuffer(CBuffer &buf, BOOL macflag = TRUE);
	void SendScript(LPCWSTR str, LPCWSTR match);
	void OnReceiveChar(DWORD ch, int pos);
	void OnSendBuffer(CBuffer &buf);

	int DelaySend();
	void OnDelayReceive(int id);

	void InitOptFixCheck(int Uid);
	BOOL SetOptFixEntry(LPCTSTR entryName);

	void SetSleepReq(int req);

	int SocketOpen();
	void SocketClose();
	int SocketReceive(void *lpBuf, int nBufLen);
	void SocketSend(void *lpBuf, int nBufLen, BOOL delaySend = FALSE);
	LPCSTR Utf8Str(LPCTSTR str);
	LPCSTR RemoteStr(LPCTSTR str);
	LPCTSTR LocalStr(LPCSTR str);

	void OnSocketConnect();
	void OnSocketError(int err);
	void OnSocketClose();
	int OnSocketReceive(LPBYTE lpBuf, int nBufLen, int nFlags);

	void SetDocTitle();
	inline void SetStatus(LPCTSTR str) { m_SockStatus = str; SetDocTitle(); }
	inline void SetEntryProBuffer() { SaveOption(m_ServerEntry, m_TextRam, m_KeyTab, m_KeyMac, m_ParamTab); }
	void SetCmdInfo(CCommandLineInfoEx *pCmdInfo);

	void DoDropFile();
	CWnd *GetAciveView();
	int GetViewCount();
	BOOL IsCanExit();

	BOOL LogOpen(LPCTSTR filename);
	BOOL LogClose();
	void LogWrite(LPBYTE lpBuf, int nBufLen, int SendRecv);
	void LogDebug(LPCSTR str, ...);
	void LogDump(LPBYTE lpBuf, int nBufLen);
	void LogInit();

	void ScriptInit(int cmds, int shift, class CScriptValue &value);
	void ScriptValue(int cmds, class CScriptValue &value, int mode);

	void OnIdle();

//オーバーライド
protected:
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);

public:
	virtual BOOL DoFileSave();

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnFileClose();
	afx_msg void OnCancelBtn();

	afx_msg void OnLogOpen();
	afx_msg void OnUpdateLogOpen(CCmdUI* pCmdUI);
	afx_msg void OnChatStop();
	afx_msg void OnUpdateChatStop(CCmdUI *pCmdUI);
	afx_msg void OnSftp();
	afx_msg void OnUpdateSftp(CCmdUI* pCmdUI);
	afx_msg void OnKanjiCodeSet(UINT nID);
	afx_msg void OnUpdateKanjiCodeSet(CCmdUI* pCmdUI);
	afx_msg void OnXYZModem(UINT nID);
	afx_msg void OnUpdateXYZModem(CCmdUI* pCmdUI);
	afx_msg void OnSendBreak();
	afx_msg void OnUpdateSendBreak(CCmdUI *pCmdUI);
	afx_msg void OnTekdisp();
	afx_msg void OnUpdateTekdisp(CCmdUI *pCmdUI);
	afx_msg void OnSocketstatus();
	afx_msg void OnUpdateSocketstatus(CCmdUI *pCmdUI);
	afx_msg void OnScript();
	afx_msg void OnUpdateScript(CCmdUI *pCmdUI);
	afx_msg void OnImagedisp();
	afx_msg void OnUpdateImagedisp(CCmdUI *pCmdUI);
	afx_msg void OnSockReOpen();

	afx_msg void OnTracedisp();
	afx_msg void OnUpdateTracedisp(CCmdUI *pCmdUI);
	afx_msg void OnCommoniter();
	afx_msg void OnUpdateCommoniter(CCmdUI *pCmdUI);
	afx_msg void OnCmdhis();
	afx_msg void OnUpdateCmdhis(CCmdUI *pCmdUI);

	afx_msg void OnLoadDefault();
	afx_msg void OnSaveDefault();
	afx_msg void OnSetOption();
	afx_msg void OnScreenReset(UINT nID);
	afx_msg void OnUpdateResetSize(CCmdUI *pCmdUI);
	afx_msg void OnScriptMenu(UINT nID);
	afx_msg void OnTitleedit();
};
