// login.h: CLogin クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGIN_H__F26A2802_471E_11D2_ABC5_00C00C9003E0__INCLUDED_)
#define AFX_LOGIN_H__F26A2802_471E_11D2_ABC5_00C00C9003E0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CLogin : public CExtSocket
{
private:
	int m_ConnectFlag;
	void SendStr(LPCSTR str);
	
public:
	BOOL Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort = 0, int nSocketType = SOCK_STREAM, void *pAddrInfo = NULL);
	void OnConnect();
	void OnReceiveCallBack(void *lpBuf, int nBufLen, int nFlags);
	void SendWindSize();

	CLogin(class CRLoginDoc *pDoc);
	virtual ~CLogin();
};

#endif // !defined(AFX_LOGIN_H__F26A2802_471E_11D2_ABC5_00C00C9003E0__INCLUDED_)
