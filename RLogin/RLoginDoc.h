// RLoginDoc.h : CRLoginDoc クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RLOGINDOC_H__E9C04D5B_EA6E_4FFB_8827_C5263E3D30E6__INCLUDED_)
#define AFX_RLOGINDOC_H__E9C04D5B_EA6E_4FFB_8827_C5263E3D30E6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TextRam.h"
#include "ExtSocket.h"
#include "BPlus.h"
#include "ZModem.h"
#include "Kermit.h"
#include "Data.h"

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
#define	UPDATE_BLINKRECT	6
#define	UPDATE_INITSIZE		7
#define	UPDATE_SETCURSOR	8
#define	UPDATE_TYPECARET	9
#define	UPDATE_TEKFLUSH		10
#define	UPDATE_RESIZE		11
#define	UPDATE_CANCELBTN	12
#define	UPDATE_DISPINDEX	13
#define	UPDATE_WAKEUP		14

#define	CARET_MOVE			0
#define	CARET_CREATE		1
#define	CARET_DESTROY		2

#define	PROTO_DIRECT		0
#define	PROTO_LOGIN			1
#define	PROTO_TELNET		2
#define	PROTO_SSH			3
#define	PROTO_COMPORT		4
#define	PROTO_PIPE			5

#define	DELAY_ECHO_MSEC		1000

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
	int m_DelayFlag;
	CBuffer m_DelayBuf;
	int m_LoadMode;
	int m_ActCharCount;
	CMainFrame *m_pMainWnd;
	CString m_TitleString;
	CStrScript *m_pStrScript;
	CString m_ErrorPrompt;
	CString m_SearchStr;
	CStringA m_WorkMbs;
	CString m_WorkStr;
	class CScript *m_pScript;
	BOOL m_InPane;
	BOOL m_bUseIdle;
	int m_AfterId;

	void SetIndex(int mode, CStringIndex &index);

	void SetMenu(CMenu *pMenu, CKeyCmdsTab *pCmdsTab);
	BOOL EntryText(CString &name);

	void SendBuffer(CBuffer &buf);
	void SendScript(LPCWSTR str, LPCWSTR match);
	void OnReciveChar(DWORD ch, int pos);
	void OnSendBuffer(CBuffer &buf);

	int DelaySend();
	void OnDelayRecive(int ch);
	inline void ClearActCount() { m_ActCharCount = 0; }
	inline void IncActCount() { m_ActCharCount++; }
	inline void SetActCount() { m_ActCharCount = 2048; }
	inline BOOL IsActCount() { return (m_ActCharCount >= 2048 ? TRUE : FALSE); }

	int SocketOpen();
	void SocketClose();
	int SocketRecive(void *lpBuf, int nBufLen);
	void SocketSend(void *lpBuf, int nBufLen);
	void SocketSendWindSize(int x, int y);
	LPCSTR RemoteStr(LPCTSTR str);
	LPCTSTR LocalStr(LPCSTR str);

	void OnSocketConnect();
	void OnSocketError(int err);
	void OnSocketClose();
	int OnSocketRecive(LPBYTE lpBuf, int nBufLen, int nFlags);

	void SetStatus(LPCTSTR str);
	void SetEntryProBuffer();
	void SetCmdInfo(CCommandLineInfoEx *pCmdInfo);

	void DoDropFile();
	CWnd *GetAciveView();
	int GetViewCount();

	BOOL LogOpen(LPCTSTR filename);
	BOOL LogClose();

	void ScriptInit(int cmds, int shift, class CScriptValue &value);
	void ScriptValue(int cmds, class CScriptValue &value, int mode);

	void OnIdle();

//オーバーライド
protected:
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);

// インプリメンテーション
protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnFileClose();
	afx_msg void OnCancelBtn();
	afx_msg void OnSockIdle();

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

	afx_msg void OnTracedisp();
	afx_msg void OnLoadDefault();
	afx_msg void OnSaveDefault();
	afx_msg void OnSetOption();
	afx_msg void OnScreenReset(UINT nID);
	afx_msg void OnScriptMenu(UINT nID);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_RLOGINDOC_H__E9C04D5B_EA6E_4FFB_8827_C5263E3D30E6__INCLUDED_)
