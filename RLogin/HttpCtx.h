///////////////////////////////////////////////////////
// CHttp2Ctx コマンド ターゲット

#pragma once

#include "Data.h"

#define	HTTP2_DEFAULT_WINDOW_SIZE	65535
#define	HTTP2_CLIENT_WINDOW_SIZE	(4 * HTTP2_DEFAULT_WINDOW_SIZE)
#define	HTTP2_CLIENT_FLOWOFF_SIZE	4095

#define	HTTP2_TOTAL_SERVER		0
#define	HTTP2_TOTAL_CLIENT		1
#define	HTTP2_STREAM_SERVER		2
#define	HTTP2_STREAM_CLIENT		3

#define	HTTP2_TYPE_DATA				0x0
#define	HTTP2_TYPE_HEADERS			0x1
#define	HTTP2_TYPE_PRIORITY			0x2
#define	HTTP2_TYPE_RST_STREAM		0x3
#define	HTTP2_TYPE_SETTINGS			0x4
#define	HTTP2_TYPE_PUSH_PROMISE		0x5
#define	HTTP2_TYPE_PING				0x6
#define	HTTP2_TYPE_GOAWAY			0x7
#define	HTTP2_TYPE_WINDOW_UPDATE	0x8
#define	HTTP2_TYPE_CONTINUATION		0x9

#define	HTTP2_FLAG_END_STREAM		0x01
#define	HTTP2_FLAG_END_HEADERS		0x04
#define	HTTP2_FLAG_PADDED			0x08
#define	HTTP2_FLAG_PRIORITY			0x20
#define	HTTP2_FLAG_ACK				0x01

#define	HTTP2_SETTINGS_HEADER_TABLE_SIZE			0x1	//	4096
#define	HTTP2_SETTINGS_ENABLE_PUSH					0x2	//	1
#define	HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS		0x3	//	0		= 100
#define	HTTP2_SETTINGS_INITIAL_WINDOW_SIZE			0x4	//	65535	+= 0x7FFF0000
#define	HTTP2_SETTINGS_MAX_FRAME_SIZE				0x5	//	16384
#define	HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE			0x6	//	0
#define	HTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL		0x8	//	0		RFC 8441 - Bootstrapping WebSockets with HTTP/2

#define	HTTP2_NOW_STREAMID			(-1)
#define	HTTP2_NEW_STREAMID			(-2)

class CStaticField : public CObject
{
public:
	LPCSTR m_Name;
	int m_Value;
	CArray<CStaticField, CStaticField &> m_Array;

	CStaticField();
	~CStaticField();

	const CStaticField & operator = (CStaticField &data);

	int Add(LPCSTR str);
	int Find(LPCSTR str);

	inline CStaticField & operator [] (LPCSTR str) { return m_Array[Add(str)]; }
	inline CStaticField & operator [] (int index) { return m_Array[index]; }

	inline const int operator = (int val) { return (m_Value = val); }
	inline operator int () const { return m_Value; }
};

class CDynamicField : public CObject
{
public:
	LPSTR m_Name;
	LPSTR m_Value;

	CDynamicField(LPCSTR name, LPCSTR value);
	~CDynamicField();
};

class CHeadQue : public CObject
{
public:
	DWORD m_sId;
	CBuffer m_Buffer;

	CHeadQue(DWORD sid, BYTE *pBuf, int len);
	~CHeadQue();
};

class CHttp2Ctx : public CObject
{
public:
	class CFifoProxy *m_pFifoProxy;
	int m_Settings[10];
	CPtrArray m_DyamicField;
	int m_ProxyStreamId;
	BOOL m_bEndofStream;
	int m_Http2WindowBytes[4];
	CStaticField m_StaticField;
	CPtrList m_SendQue;
	CPtrArray m_HeadQue;

public:
	CHttp2Ctx(class CFifoProxy *pFifoProxy);
	virtual ~CHttp2Ctx();

	void GetDynamicField(int index, CStringA &name, CStringA &value);
	void AddDynamicField(LPCSTR name, LPCSTR value);

	BOOL GetHPackField(class CBuffer *bp, CStringA &name, CStringA &value);
	void PutHPackField(class CBuffer *bp, LPCSTR name, LPCSTR value);

	BOOL GetHPackFrame(class CBuffer *bp, int &length, int &type, int &flag, DWORD &sid);
	void SendHPackFrameQueBuffer();
	void SendHPackFrameBuffer(CBuffer *bp);
	void SendHPackFrame(int type, int flag, int sid, BYTE *pBuf, int len);
	inline void SetEndOfStream(int sid) { if ( sid == m_ProxyStreamId ) m_bEndofStream = TRUE; }
	inline BOOL IsEndOfStream() { return m_bEndofStream; }

	BOOL ReciveHeader(class CBuffer *bp, DWORD sid, BOOL bEndOf);
};

///////////////////////////////////////////////////////
// CHttp3Ctx

#define	HTTP3_TYPE_DATA				0x00
#define	HTTP3_TYPE_HEADERS			0x01
#define	HTTP3_TYPE_CANCEL_PUSH		0x03
#define	HTTP3_TYPE_SETTINGS			0x04
#define	HTTP3_TYPE_PUSH_PROMISE		0x05
#define	HTTP3_TYPE_GOAWAY			0x07
#define	HTTP3_TYPE_MAX_PUSH_ID		0x0d
#define	HTTP3_TYPE_ERROR			0xFF

class CHttp3Ctx : public CObject
{
public:
	class CFifoProxy *m_pFifoProxy;
	int m_InsertCount;
	int m_DeltaBase;
	CPtrArray m_DyamicField;
	CStaticField m_StaticField;

public:
	CHttp3Ctx(class CFifoProxy *pFifoProxy);
	virtual ~CHttp3Ctx();

	BOOL GetHeaderBase(CBuffer *bp);
	void PutHeaderBase(CBuffer *bp);

	void GetDynamicField(int index, CStringA &name, CStringA &value);
	void AddDynamicField(LPCSTR name, LPCSTR value);

	BOOL GetQPackField(class CBuffer *bp, CStringA &name, CStringA &value);
	void PutQPackField(class CBuffer *bp, LPCSTR name, LPCSTR value);

	BOOL GetQPackFrame(class CBuffer *bp, int &type, CBuffer *pData);
	void SendQPackFrame(int type, BYTE *pBuf, int len);
};
