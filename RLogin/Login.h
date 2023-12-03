//////////////////////////////////////////////////////////////////////
// Login.h : インターフェイス
//

#pragma once

class CFifoLogin : public CFifoThread
{
public:
	CFifoLogin(class CRLoginDoc *pDoc, class CExtSocket *pSock);

	void SendStr(int nFd, LPCSTR mbs);
	void SendWindSize(int nFd);

	virtual void OnOob(int nFd, int len, BYTE *pBuffer);
	virtual void OnConnect(int nFd);
	virtual void OnCommand(int cmd, int param, int msg, int len, void *buf, CEvent *pEvent, BOOL *pResult);
};

class CLogin : public CExtSocket
{
public:
	CLogin(class CRLoginDoc *pDoc);

	virtual CFifoBase *FifoLinkMid();
	virtual BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM);

	virtual void SendWindSize();
};
