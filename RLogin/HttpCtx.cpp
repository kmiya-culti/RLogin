///////////////////////////////////////////////////////
// HttpCtx.cpp : ŽÀ‘•ƒtƒ@ƒCƒ‹

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "Fifo.h"
#include "HttpCtx.h"

#include "qpack_table.h"

///////////////////////////////////////////////////////
// CStaticField

CStaticField::CStaticField()
{
	m_Name = NULL;
	m_Value = (-1);
}
CStaticField::~CStaticField()
{
}
const CStaticField & CStaticField::operator = (CStaticField &data)
{
	m_Name = data.m_Name;
	m_Value = data.m_Value;

	m_Array.RemoveAll();
	for ( int n = 0 ; n < data.m_Array.GetSize() ; n++ )
		m_Array.Add(data.m_Array[n]);

	return *this;
}
static int StaticFieldCmp(const void *src, const void *dis)
{
	return strcmp((LPCSTR)src, ((CStaticField *)dis)->m_Name);
}
int CStaticField::Add(LPCSTR str)
{
	int n;
	CStaticField tmp;

	if ( BinaryFind((void *)str, m_Array.GetData(), sizeof(CStaticField), (int)m_Array.GetSize(), StaticFieldCmp, &n) )
		return n;

	tmp.m_Name = str;
	m_Array.InsertAt(n, tmp);
	return n;
}
int CStaticField::Find(LPCSTR str)
{
	int n;

	if ( BinaryFind((void *)str, m_Array.GetData(), sizeof(CStaticField), (int)m_Array.GetSize(), StaticFieldCmp, &n) )
		return n;

	return (-1);
}

///////////////////////////////////////////////////////
// CDynamicField

CDynamicField::CDynamicField(LPCSTR name, LPCSTR value)
{
	m_Name = new char[strlen(name) + 1];
	strcpy(m_Name, name);

	m_Value = new char[strlen(value) + 1];
	strcpy(m_Value, value);
}
CDynamicField::~CDynamicField()
{
	delete m_Name;
	delete m_Value;
}

///////////////////////////////////////////////////////
// CHeadQue

CHeadQue::CHeadQue(DWORD sid, BYTE *pBuf, int len)
{
	m_sId = sid;
	m_Buffer.Apend(pBuf, len);
}
CHeadQue::~CHeadQue()
{
}

///////////////////////////////////////////////////////
// CHttp2Ctx
// RFC 7540 - Hypertext Transfer Protocol Version 2 (HTTP/2)
// RFC 7541 - HPACK: Header Compression for HTTP/2
// RFC 9113 - HTTP/2

CHttp2Ctx::CHttp2Ctx(class CFifoProxy *pFifoProxy)
{
	m_pFifoProxy = pFifoProxy;

	ZeroMemory(m_Settings, sizeof(m_Settings));
	m_Settings[HTTP2_SETTINGS_HEADER_TABLE_SIZE]		= 4096;
	m_Settings[HTTP2_SETTINGS_ENABLE_PUSH]				= 1;
	m_Settings[HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS]	= 0;
	m_Settings[HTTP2_SETTINGS_INITIAL_WINDOW_SIZE]		= 65535;
	m_Settings[HTTP2_SETTINGS_MAX_FRAME_SIZE]			= 16384;
	m_Settings[HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE]		= 0;
	m_Settings[HTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL]	= 0;

	m_ProxyStreamId = (-1);
	m_bEndofStream = FALSE;

	m_Http2WindowBytes[HTTP2_TOTAL_SERVER]  = HTTP2_DEFAULT_WINDOW_SIZE;
	m_Http2WindowBytes[HTTP2_TOTAL_CLIENT]  = HTTP2_DEFAULT_WINDOW_SIZE;
	m_Http2WindowBytes[HTTP2_STREAM_SERVER] = HTTP2_DEFAULT_WINDOW_SIZE;
	m_Http2WindowBytes[HTTP2_STREAM_CLIENT] = HTTP2_DEFAULT_WINDOW_SIZE;

	for ( int n = 0 ; n < HPACK_FIELD_MAX ; n++ ) {
		if ( m_StaticField.Find(hpack_field_tab[n].name) < 0 )
			m_StaticField[hpack_field_tab[n].name] = n + 1;
		if ( *hpack_field_tab[n].value != '\0' )
			m_StaticField[hpack_field_tab[n].name][hpack_field_tab[n].value] = n + 1;
	}
}
CHttp2Ctx::~CHttp2Ctx()
{
	for ( int n = 0 ; n < m_DyamicField.GetSize() ; n++ )
		delete (CDynamicField *)m_DyamicField[n];
	m_DyamicField.RemoveAll();

	while ( !m_SendQue.IsEmpty() ) {
		CBuffer *bp = (CBuffer *)m_SendQue.RemoveHead();
		delete bp;
	}

	for ( int n = 0 ; n < m_HeadQue.GetSize() ; n++ )
		delete (CHeadQue *)m_HeadQue[n];
	m_HeadQue.RemoveAll();
}

void CHttp2Ctx::GetDynamicField(int index, CStringA &name, CStringA &value)
{
	if ( index < 0 || index >= m_DyamicField.GetSize() )
		return;
	CDynamicField *pField = (CDynamicField *)m_DyamicField[index];
	name = pField->m_Name;
	value = pField->m_Value;
}
void CHttp2Ctx::AddDynamicField(LPCSTR name, LPCSTR value)
{
	CDynamicField *pField = new CDynamicField(name, value);
	m_DyamicField.Add(pField);
}

// RFC 7541 - HPACK: Header Compression for HTTP/2
// 6.1. Indexed Header Field Representation
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 1 |        Index (7+)         |
//   +---+---------------------------+
// 6.2.1. Literal Header Field with Incremental Indexing
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 1 |      Index (6+)       |
//   +---+---+-----------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   | Value String (Length octets)  |
//   +-------------------------------+
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 1 |           0           |
//   +---+---+-----------------------+
//   | H |     Name Length (7+)      |
//   +---+---------------------------+
//   |  Name String (Length octets)  |
//   +---+---------------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   | Value String (Length octets)  |
//   +-------------------------------+
// 6.3. Dynamic Table Size Update
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 1 |   Max size (5+)   |
//   +---+---------------------------+
// 6.2.3. Literal Header Field Never Indexed
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 0 | 1 |  Index (4+)   |
//   +---+---+-----------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   | Value String (Length octets)  |
//   +-------------------------------+
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 0 | 1 |       0       |
//   +---+---+-----------------------+
//   | H |     Name Length (7+)      |
//   +---+---------------------------+
//   |  Name String (Length octets)  |
//   +---+---------------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   | Value String (Length octets)  |
//   +-------------------------------+
// 6.2.2. Literal Header Field without Indexing
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 0 | 0 |  Index (4+)   |
//   +---+---+-----------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   | Value String (Length octets)  |
//   +-------------------------------+
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 0 | 0 |       0       |
//   +---+---+-----------------------+
//   | H |     Name Length (7+)      |
//   +---+---------------------------+
//   |  Name String (Length octets)  |
//   +---+---------------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   | Value String (Length octets)  |
//   +-------------------------------+

BOOL CHttp2Ctx::GetHPackField(class CBuffer *bp, CStringA &name, CStringA &value)
{
	int flag, index;
	ULONGLONG data;
	int saveOfs = bp->m_Ofs;
	BYTE *p = bp->GetPtr();

	name.Empty();
	value.Empty();

	if ( bp->GetSize() <= 0 )
		return FALSE;

	if ( (*p & 0x80) != 0 ) {
		// 6.1. Indexed Header Field Representation
		if ( !bp->GetPackInt(&data, &flag, 7) )
			goto CONTINUE;
		if ( (index = (int)data - 1) >= 0 && index < HPACK_FIELD_MAX ) {
			name = hpack_field_tab[index].name;
			value = hpack_field_tab[index].value;
		} else {
			GetDynamicField(index - HPACK_FIELD_MAX, name, value);
		}

	} else if ( (*p & 0x40) != 0 ) {
		// 6.2.1. Literal Header Field with Incremental Indexing
		if ( !bp->GetPackInt(&data, &flag, 6) )
			goto CONTINUE;

		if ( data == 0 ) {
			// Figure 7: Literal Header Field with Incremental Indexing -- New Name
			if ( !bp->GetPackStr(name, NULL, 7) )
				goto CONTINUE;
		} else if ( (index = (int)data - 1) >= 0 && index < HPACK_FIELD_MAX )
			// Figure 6: Literal Header Field with Incremental Indexing -- Indexed Name
			name = hpack_field_tab[index].name;
		 else 
			GetDynamicField(index - HPACK_FIELD_MAX, name, value);

		if ( !bp->GetPackStr(value, NULL, 7) )
			goto CONTINUE;

		AddDynamicField(name, value);

	} else if ( (*p & 0x20) != 0 ) {
		// 6.3. Dynamic Table Size Update
		if ( !bp->GetPackInt(&data, &flag, 5) )
			goto CONTINUE;

	} else if ( (*p & 0x10) != 0 ) {
		// 6.2.3. Literal Header Field Never Indexed
		if ( !bp->GetPackInt(&data, &flag, 4) )
			goto CONTINUE;

		if ( data == 0 ) {
			if ( !bp->GetPackStr(name, NULL, 7) )
				goto CONTINUE;
		} else if ( (index = (int)data - 1) >= 0 && index < HPACK_FIELD_MAX )
			name = hpack_field_tab[index].name;
		else
			GetDynamicField(index - HPACK_FIELD_MAX, name, value);

		if ( !bp->GetPackStr(value, NULL, 7) )
			goto CONTINUE;

	} else {
		// 6.2.2. Literal Header Field without Indexing
		if ( !bp->GetPackInt(&data, &flag, 4) )
			goto CONTINUE;

		if ( data == 0 ) {
			if ( !bp->GetPackStr(name, NULL, 7) )
				goto CONTINUE;
		} else if ( (index = (int)data - 1) >= 0 && index < HPACK_FIELD_MAX )
			name = hpack_field_tab[index].name;
		else
			GetDynamicField(index - HPACK_FIELD_MAX, name, value);

		if ( !bp->GetPackStr(value, NULL, 7) )
			goto CONTINUE;
	}

	return TRUE;

CONTINUE:
	bp->m_Ofs = saveOfs;
	return FALSE;
}
void CHttp2Ctx::PutHPackField(class CBuffer *bp, LPCSTR name, LPCSTR value)
{
	int n, i;

	if ( (n = m_StaticField.Find(name)) >= 0 ) {
		if ( (i = m_StaticField[n].Find(value)) >= 0 ) {
			// 6.1. Indexed Header Field Representation
			bp->PutPackInt((int)m_StaticField[n][i], 0x80, 7);
		} else {
			// 6.2.2. Literal Header Field without Indexing
			bp->PutPackInt((int)m_StaticField[n], 0x00, 4);
			bp->PutPackStr(value, 0x00, 7);
		}
	} else {
		// 6.2.2. Literal Header Field without Indexing
		bp->PutPackInt(0, 0x00, 4);
		bp->PutPackStr(name, 0x00, 7);
		bp->PutPackStr(value, 0x00, 7);
	}
}

// RFC 7540 - Hypertext Transfer Protocol Version 2 (HTTP/2)
// 4.1. Frame Format
//    +-----------------------------------------------+
//    |                 Length (24)                   |
//    +---------------+---------------+---------------+
//    |   Type (8)    |   Flags (8)   |
//    +-+-------------+---------------+-------------------------------+
//    |R|                 Stream Identifier (31)                      |
//    +=+=============================================================+
//    |                   Frame Payload (0...)                      ...
//    +---------------------------------------------------------------+

BOOL CHttp2Ctx::GetHPackFrame(class CBuffer *bp, int &length, int &type, int &flag, DWORD &sid)
{
	BYTE *p = bp->GetPtr();

	if ( bp->GetSize() < 9 )
		return FALSE;

	// network byte order (big endian)

	length  = *(p++) << 16;
	length |= *(p++) << 8;
	length |= *(p++);

	type = *(p++);
	flag = *(p++);

	sid  = *(p++) << 24;
	sid |= *(p++) << 16;
	sid |= *(p++) << 8;
	sid |= *(p++);

	return TRUE;
}
void CHttp2Ctx::SendHPackFrameQueBuffer()
{
	while ( !m_SendQue.IsEmpty() ) {
		CBuffer *bp = (CBuffer *)m_SendQue.RemoveHead();
		if ( m_Http2WindowBytes[HTTP2_TOTAL_SERVER] < (bp->GetSize() - 9) )
			break;
		m_pFifoProxy->Write(FIFO_STDOUT, bp->GetPtr(), bp->GetSize());
		m_Http2WindowBytes[HTTP2_TOTAL_SERVER] -= (bp->GetSize() - 9);
		delete bp;
	}
}
void CHttp2Ctx::SendHPackFrameBuffer(CBuffer *bp)
{
	if ( m_Http2WindowBytes[HTTP2_TOTAL_SERVER] < (bp->GetSize() - 9) ) {
		CBuffer *pQue = new CBuffer(bp->GetSize());
		pQue->Apend(bp->GetPtr(), bp->GetSize());
		m_SendQue.AddTail(pQue);

	} else {
		m_pFifoProxy->Write(FIFO_STDOUT, bp->GetPtr(), bp->GetSize());
		m_Http2WindowBytes[HTTP2_TOTAL_SERVER] -= (bp->GetSize() - 9);
	}
}
void CHttp2Ctx::SendHPackFrame(int type, int flag, int sid, BYTE *pBuf, int len)
{
	CBuffer tmp;

	if ( sid == HTTP2_NOW_STREAMID )
		sid = m_ProxyStreamId;
	else if ( sid == HTTP2_NEW_STREAMID ) {
		m_ProxyStreamId += 2;
		m_Http2WindowBytes[HTTP2_STREAM_CLIENT] = HTTP2_DEFAULT_WINDOW_SIZE;
		m_Http2WindowBytes[HTTP2_STREAM_SERVER] = HTTP2_DEFAULT_WINDOW_SIZE;
		sid = m_ProxyStreamId;
		m_bEndofStream = FALSE;
	}

	tmp.Put24Bit(len);		// Length (24)
	tmp.Put8Bit(type);		// Type (8)
	tmp.Put8Bit(flag);		// Flags (8)
	tmp.Put32Bit(sid);		// |R|Stream Identifier (31)

	if ( pBuf != NULL && len > 0 )
		tmp.Apend(pBuf, len);

	// TRACE("SendHPackFrame %d, %d, %d, %d\n", type, flag, sid, len);

	SendHPackFrameBuffer(&tmp);
}

// RFC 7540 - Hypertext Transfer Protocol Version 2 (HTTP/2)
// 11.2. Frame Type Registry
//   +---------------+------+--------------+
//   | Frame Type    | Code | Section      |
//   +---------------+------+--------------+
//   | DATA          | 0x0  | Section 6.1  |
//   | HEADERS       | 0x1  | Section 6.2  |
//   | PRIORITY      | 0x2  | Section 6.3  |
//   | RST_STREAM    | 0x3  | Section 6.4  |
//   | SETTINGS      | 0x4  | Section 6.5  |
//   | PUSH_PROMISE  | 0x5  | Section 6.6  |
//   | PING          | 0x6  | Section 6.7  |
//   | GOAWAY        | 0x7  | Section 6.8  |
//   | WINDOW_UPDATE | 0x8  | Section 6.9  |
//   | CONTINUATION  | 0x9  | Section 6.10 |
//   +---------------+------+--------------+

// 6.1. DATA
// DATA frames (type=0x0)
// END_STREAM (0x1): When set, bit 0 indicates that this frame is the last that the endpoint 
// PADDED (0x8): When set, bit 3 indicates that the Pad Length field and any padding that it describes are present.
//    +---------------+
//    |Pad Length? (8)|
//    +---------------+-----------------------------------------------+
//    |                            Data (*)                         ...
//    +---------------------------------------------------------------+
//    |                           Padding (*)                       ...
//    +---------------------------------------------------------------+

// 6.2. HEADERS
// HEADERS frame (type=0x1)
// END_STREAM (0x1): When set, bit 0 indicates that the header block
// END_HEADERS (0x4): When set, bit 2 indicates that this frame contains an entire header block
// PADDED (0x8): When set, bit 3 indicates that the Pad Length field and any padding that it describes are present.
// PRIORITY (0x20): When set, bit 5 indicates that the Exclusive Flag (E), Stream Dependency
//    +---------------+
//    |Pad Length? (8)|
//    +-+-------------+-----------------------------------------------+
//    |E|                 Stream Dependency? (31)                     |
//    +-+-------------+-----------------------------------------------+
//    |  Weight? (8)  |
//    +-+-------------+-----------------------------------------------+
//    |                   Header Block Fragment (*)                 ...
//    +---------------------------------------------------------------+
//    |                           Padding (*)                       ...
//    +---------------------------------------------------------------+
// E: A single-bit flag indicating that the stream dependency is exclusive (see Section 5.3). This field is only present if the PRIORITY flag is set.
// Stream Dependency: A 31-bit stream identifier for the stream that this stream depends on (see Section 5.3). This field is only present if the PRIORITY flag is set.
// Weight: An unsigned 8-bit integer representing a priority weight for the stream
// Header Block Fragment: A header block fragment

// 6.3. PRIORITY
//    +-+-------------------------------------------------------------+
//    |E|                  Stream Dependency (31)                     |
//    +-+-------------+-----------------------------------------------+
//    |   Weight (8)  |
//    +-+-------------+

// 6.4. RST_STREAM
//    +---------------------------------------------------------------+
//    |                        Error Code (32)                        |
//    +---------------------------------------------------------------+

// 6.5. SETTINGS
// SETTINGS frame (type=0x4)
// ACK (0x1) When set, bit 0 indicates that this frame acknowledges receipt and application of the peer's SETTINGS frame. When this bit is set, the payload of the SETTINGS frame MUST be empty.
// stream identifier for a SETTINGS frame MUST be zero (0x0). 
//    +-------------------------------+
//    |       Identifier (16)         |
//    +-------------------------------+-------------------------------+
//    |                        Value (32)                             |
//    +---------------------------------------------------------------+
// 6.5.2. Defined SETTINGS Parameters
// SETTINGS_HEADER_TABLE_SIZE (0x1)				4096
// SETTINGS_ENABLE_PUSH (0x2)					1
// SETTINGS_MAX_CONCURRENT_STREAMS (0x3)		0		= 100
// SETTINGS_INITIAL_WINDOW_SIZE (0x4)			65535	+= 0x7FFF0000
// SETTINGS_MAX_FRAME_SIZE (0x5)				16384
// SETTINGS_MAX_HEADER_LIST_SIZE (0x6)			0
// SETTINGS_ENABLE_CONNECT_PROTOCOL (0x8)		0		RFC 8441 - Bootstrapping WebSockets with HTTP/2

// 6.6. PUSH_PROMISE
//    +---------------+
//    |Pad Length? (8)|
//    +-+-------------+-----------------------------------------------+
//    |R|                  Promised Stream ID (31)                    |
//    +-+-----------------------------+-------------------------------+
//    |                   Header Block Fragment (*)                 ...
//    +---------------------------------------------------------------+
//    |                           Padding (*)                       ...
//    +---------------------------------------------------------------+

// 6.7. PING
//    +---------------------------------------------------------------+
//    |                      Opaque Data (64)                         |
//    |                                                               |
//    +---------------------------------------------------------------+

// 6.8. GOAWAY
//    +-+-------------------------------------------------------------+
//    |R|                  Last-Stream-ID (31)                        |
//    +-+-------------------------------------------------------------+
//    |                      Error Code (32)                          |
//    +---------------------------------------------------------------+
//    |                  Additional Debug Data (*)                    |
//    +---------------------------------------------------------------+

// 6.9. WINDOW_UPDATE
//    +-+-------------------------------------------------------------+
//    |R|              Window Size Increment (31)                     |
//    +-+-------------------------------------------------------------+

// 6.10. CONTINUATION
// END_HEADERS (0x4): When set, bit 2 indicates that this frame ends a header block
//    +---------------------------------------------------------------+
//    |                   Header Block Fragment (*)                 ...
//    +---------------------------------------------------------------+

BOOL CHttp2Ctx::ReciveHeader(class CBuffer *bp, DWORD sid, BOOL bEndOf)
{
	INT_PTR index;
	CHeadQue *pHead = NULL;

	for ( index = 0 ; index < m_HeadQue.GetSize() ; index++ ) {
		if ( ((CHeadQue *)m_HeadQue[index])->m_sId == sid ) {
			pHead = (CHeadQue *)m_HeadQue[index];
			break;
		}
	}

	if ( bp == NULL ) {		// RST_STREAM Cannel
		if ( pHead != NULL ) {
			m_HeadQue.RemoveAt(index);
			delete pHead;
		}
		return FALSE;
	}

	if ( !bEndOf ) {
		if ( pHead == NULL ) {
			pHead = new CHeadQue(sid, bp->GetPtr(), bp->GetSize());
			index = m_HeadQue.Add(pHead);
		} else
			pHead->m_Buffer.Apend(bp->GetPtr(), bp->GetSize());

	} else if ( pHead != NULL ) {
		pHead->m_Buffer.Apend(bp->GetPtr(), bp->GetSize());
		bp->Swap(pHead->m_Buffer);
		m_HeadQue.RemoveAt(index);
		delete pHead;
	}

	return bEndOf;
}

///////////////////////////////////////////////////////
// CHttp3Ctx
// RFC 9000 - QUIC: A UDP-Based Multiplexed and Secure Transport
// RFC 9204 - QPACK: Field Compression for HTTP/3
// RFC 9114 - HTTP/3

CHttp3Ctx::CHttp3Ctx(class CFifoProxy *pFifoProxy)
{
	m_pFifoProxy = pFifoProxy;

	m_InsertCount = 0;
	m_DeltaBase = 0;

	for ( int n = 0 ; n < QPACK_FIELD_MAX ; n++ ) {
		if ( m_StaticField.Find(qpack_field_tab[n].name) < 0 )
			m_StaticField[qpack_field_tab[n].name] = n;
		if ( *qpack_field_tab[n].value != '\0' )
			m_StaticField[qpack_field_tab[n].name][qpack_field_tab[n].value] = n;
	}
}

CHttp3Ctx::~CHttp3Ctx()
{
	for ( int n = 0 ; n < m_DyamicField.GetSize() ; n++ )
		delete (CDynamicField *)m_DyamicField[n];
	m_DyamicField.RemoveAll();
}

// RFC 9204 - QPACK: Field Compression for HTTP/3
// 4.5.1. Encoded Field Section Prefix
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   |   Required Insert Count (8+)  |
//   +---+---------------------------+
//   | S |      Delta Base (7+)      |
//   +---+---------------------------+
//   |      Encoded Field Lines    ...
//   +-------------------------------+

BOOL CHttp3Ctx::GetHeaderBase(CBuffer *bp)
{
	int flag;
	ULONGLONG count, base;

	if ( !bp->GetPackInt(&count, NULL, 8) )		// Required Insert Count
		return FALSE;
	if ( !bp->GetPackInt(&base, &flag, 7) )		// Delta Base
		return FALSE;

	m_InsertCount = (int)count;
	m_DeltaBase = (int)base;

	if ( (flag & 0x80) != 0 )		// |S|Delta Base (7+)|
		m_DeltaBase = 0 - m_DeltaBase;

	return TRUE;
}
void CHttp3Ctx::PutHeaderBase(CBuffer *bp)
{
	bp->PutPackInt(0, 0x00, 8);			// Required Insert Count
	bp->PutPackInt(0, 0x00, 7);			// Delta Base
}

void CHttp3Ctx::GetDynamicField(int index, CStringA &name, CStringA &value)
{
	if ( index < 0 || index >= m_DyamicField.GetSize() )
		return;
	CDynamicField *pField = (CDynamicField *)m_DyamicField[index];
	name = pField->m_Name;
	value = pField->m_Value;
}
void CHttp3Ctx::AddDynamicField(LPCSTR name, LPCSTR value)
{
	CDynamicField *pField = new CDynamicField(name, value);
	m_DyamicField.Add(pField);
}

// RFC 9204 - QPACK: Field Compression for HTTP/3 (RFC 9204) 

// 4.3. Encoder Instructions
// 4.3.2. Insert with Name Reference
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 1 | T |    Name Index (6+)    |
//   +---+---+-----------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   |  Value String (Length bytes)  |
//   +-------------------------------+
// 4.3.3. Insert with Literal Name
//    0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 1 | H | Name Length (5+)  |
//   +---+---+---+-------------------+
//   |  Name String (Length bytes)   |
//   +---+---------------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   |  Value String (Length bytes)  |
//   +-------------------------------+
// 4.3.1. Set Dynamic Table Capacity
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 1 |   Capacity (5+)   |
//   +---+---+---+-------------------+
// 4.3.4. Duplicate
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 0 |    Index (5+)     |
//   +---+---+---+-------------------+

// 4.4. Decoder Instructions
// 4.4.1. Section Acknowledgment
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 1 |      Stream ID (7+)       |
//   +---+---------------------------+
// 4.4.2. Stream Cancellation
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 1 |     Stream ID (6+)    |
//   +---+---+-----------------------+
// 4.4.3. Insert Count Increment
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 |     Increment (6+)    |
//   +---+---+-----------------------+


// 4.5. Field Line Representations
// 4.5.2. Indexed Field Line
//   When T=1, the number represents the static table index;
//   when T=0, the number is the relative index of the entry in the dynamic table.
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 1 | T |      Index (6+)       |
//   +---+---+-----------------------+
// 4.5.4. Literal Field Line with Name Reference
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 1 | N | T |Name Index (4+)|
//   +---+---+---+---+---------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   |  Value String (Length bytes)  |
//   +-------------------------------+
// 4.5.6. Literal Field Line with Literal Name
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 1 | N | H |NameLen(3+)|
//   +---+---+---+---+---+-----------+
//   |  Name String (Length bytes)   |
//   +---+---------------------------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   |  Value String (Length bytes)  |
//   +-------------------------------+
// 4.5.3. Indexed Field Line with Post-Base Index
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 0 | 1 |  Index (4+)   |
//   +---+---+---+---+---------------+
// 4.5.5. Literal Field Line with Post-Base Name Reference
//     0   1   2   3   4   5   6   7
//   +---+---+---+---+---+---+---+---+
//   | 0 | 0 | 0 | 0 | N |NameIdx(3+)|
//   +---+---+---+---+---+-----------+
//   | H |     Value Length (7+)     |
//   +---+---------------------------+
//   |  Value String (Length bytes)  |
//   +-------------------------------+

BOOL CHttp3Ctx::GetQPackField(class CBuffer *bp, CStringA &name, CStringA &value)
{
	int flag;
	ULONGLONG data;
	int saveOfs = bp->m_Ofs;
	BYTE *p = bp->GetPtr();

	name.Empty();
	value.Empty();

	if ( bp->GetSize() <= 0 )
		return FALSE;

	if ( (*p & 0x80) != 0 ) {
		// 4.5.2. Indexed Field Line
		//   | 1 | T |      Index (6+)       |
		if ( !bp->GetPackInt(&data, &flag, 6) )
			goto CONTINUE;
		if ( (flag & 0x40) != 0 && data < QPACK_FIELD_MAX ) {			// T == 1 static
			name = qpack_field_tab[(int)data].name;
			value = qpack_field_tab[(int)data].value;
		} else if ( (flag & 0x40) == 0 )								// T == 0 dynamic 
			GetDynamicField(m_DeltaBase + (int)data, name, value);

	} else if ( (*p & 0x40) != 0 ) {
		// 4.5.4. Literal Field Line with Name Reference
		//   | 0 | 1 | N | T |Name Index (4+)|
		if ( !bp->GetPackInt(&data, &flag, 4) )
			goto CONTINUE;
		if ( !bp->GetPackStr(value, NULL, 7) )
			goto CONTINUE;
		if ( (flag & 0x10) != 0 && data < QPACK_FIELD_MAX )				// T == 1 static
			name = qpack_field_tab[(int)data].name;
		else if ( (flag & 0x10) == 0 )									// T == 0 dynamic 
			GetDynamicField(m_DeltaBase + (int)data, name, value);

		if ( (flag & 0x20) == 0 )										// N == 0
			AddDynamicField(name, value);
	
	} else if ( (*p & 0x20) != 0 ) {
		// 4.5.6. Literal Field Line with Literal Name
		//   | 0 | 0 | 1 | N | H |NameLen(3+)|
		if ( !bp->GetPackStr(name, &flag, 3) )
			goto CONTINUE;
		if ( !bp->GetPackStr(value, NULL, 7) )
			goto CONTINUE;

		if ( (flag & 0x10) == 0 )										// N == 0
			AddDynamicField(name, value);

	} else if ( (*p & 0x10) != 0 ) {
		// 4.5.3. Indexed Field Line with Post-Base Index
		//   | 0 | 0 | 0 | 1 |  Index (4+)   |
		if ( !bp->GetPackInt(&data, &flag, 4) )
			goto CONTINUE;

		GetDynamicField(m_DeltaBase - (int)data - 1, name, value);

	} else {
		// 4.5.5. Literal Field Line with Post-Base Name Reference
		//   | 0 | 0 | 0 | 0 | N |NameIdx(3+)|
		if ( !bp->GetPackInt(&data, &flag, 3) )
			goto CONTINUE;

		GetDynamicField(m_DeltaBase - (int)data - 1, name, value);
		value.Empty();

		if ( !bp->GetPackStr(value, NULL, 7) )
			goto CONTINUE;

		if ( (flag & 0x08) == 0 )										// N == 0
			AddDynamicField(name, value);
	}

	return TRUE;

CONTINUE:
	bp->m_Ofs = saveOfs;
	return FALSE;
}
void CHttp3Ctx::PutQPackField(class CBuffer *bp, LPCSTR name, LPCSTR value)
{
	int n, i;

	if ( (n = m_StaticField.Find(name)) >= 0 ) {
		if ( (i = m_StaticField[n].Find(value)) >= 0 ) {
			// 4.5.2. Indexed Field Line
			bp->PutPackInt((int)m_StaticField[n][i], 0xC0, 6);		// 1T, T=1
		} else {
			// 4.5.4. Literal Field Line with Name Reference
			bp->PutPackInt((int)m_StaticField[n], 0x70, 4);			// 01NT, N=1, T=1
			bp->PutPackStr(value, 0x00, 7);
		}
	} else {
		// 4.5.6. Literal Field Line with Literal Name
		bp->PutPackStr(name, 0x30, 3);									// 001N, N=1
		bp->PutPackStr(value, 0x00, 7);
	}
}

// RFC 9114 - HTTP/3
// 7. HTTP Framing Layer
// +==============+================+================+========+=========+
// | Frame        | Control Stream | Request        | Push   | Section |
// |              |                | Stream         | Stream |         |
// +==============+================+================+========+=========+
// | DATA         | No             | Yes            | Yes    | Section |
// |              |                |                |        | 7.2.1   |
// +--------------+----------------+----------------+--------+---------+
// | HEADERS      | No             | Yes            | Yes    | Section |
// |              |                |                |        | 7.2.2   |
// +--------------+----------------+----------------+--------+---------+
// | CANCEL_PUSH  | Yes            | No             | No     | Section |
// |              |                |                |        | 7.2.3   |
// +--------------+----------------+----------------+--------+---------+
// | SETTINGS     | Yes (1)        | No             | No     | Section |
// |              |                |                |        | 7.2.4   |
// +--------------+----------------+----------------+--------+---------+
// | PUSH_PROMISE | No             | Yes            | No     | Section |
// |              |                |                |        | 7.2.5   |
// +--------------+----------------+----------------+--------+---------+
// | GOAWAY       | Yes            | No             | No     | Section |
// |              |                |                |        | 7.2.6   |
// +--------------+----------------+----------------+--------+---------+
// | MAX_PUSH_ID  | Yes            | No             | No     | Section |
// |              |                |                |        | 7.2.7   |
// +--------------+----------------+----------------+--------+---------+
// | Reserved     | Yes            | Yes            | Yes    | Section |
// |              |                |                |        | 7.2.8   |
// +--------------+----------------+----------------+--------+---------+
// 11.2.1. Frame Types
// +==============+=======+===============+
// | Frame Type   | Value | Specification |
// +==============+=======+===============+
// | DATA         |  0x00 | Section 7.2.1 |
// +--------------+-------+---------------+
// | HEADERS      |  0x01 | Section 7.2.2 |
// +--------------+-------+---------------+
// | Reserved     |  0x02 | This document |
// +--------------+-------+---------------+
// | CANCEL_PUSH  |  0x03 | Section 7.2.3 |
// +--------------+-------+---------------+
// | SETTINGS     |  0x04 | Section 7.2.4 |
// +--------------+-------+---------------+
// | PUSH_PROMISE |  0x05 | Section 7.2.5 |
// +--------------+-------+---------------+
// | Reserved     |  0x06 | This document |
// +--------------+-------+---------------+
// | GOAWAY       |  0x07 | Section 7.2.6 |
// +--------------+-------+---------------+
// | Reserved     |  0x08 | This document |
// +--------------+-------+---------------+
// | Reserved     |  0x09 | This document |
// +--------------+-------+---------------+
// | MAX_PUSH_ID  |  0x0d | Section 7.2.7 |
// +--------------+-------+---------------+
//
// DATA Frame { Type (i) = 0x00, Length (i), Data (..), }
// HEADERS Frame { Type (i) = 0x01, Length (i), Encoded Field Section (..), }
// CANCEL_PUSH Frame { Type (i) = 0x03, Length (i), Push ID (i), }
// SETTINGS Frame { Type (i) = 0x04, Length (i), Setting { Identifier (i), Value (i) } (..) ..., }
// PUSH_PROMISE Frame { Type (i) = 0x05, Length (i), Push ID (i), Encoded Field Section (..), }
// GOAWAY Frame { Type (i) = 0x07, Length (i), Stream ID/Push ID (i), }
// MAX_PUSH_ID Frame { Type (i) = 0x0d, Length (i), Push ID (i), }

BOOL CHttp3Ctx::GetQPackFrame(class CBuffer *bp, int &type, CBuffer *pData)
{
	int lenght;
	int saveOfs = bp->m_Ofs;

	if ( (type = (int)bp->GetVarInt()) < 0 )
		return FALSE;

	if ( (lenght = (int)bp->GetVarInt()) < 0 )
		goto CONTINUE;

	if ( type > HTTP3_TYPE_MAX_PUSH_ID || lenght > (128 * 1024) ) {
		type = HTTP3_TYPE_ERROR;
		return TRUE;
	}
	
	if ( bp->GetSize() < lenght )
		goto CONTINUE;

	pData->Clear();
	pData->Apend(bp->GetPtr(), lenght);
	bp->Consume(lenght);

	TRACE("GetQPackFrame %d, %d\n", type, lenght);

	return TRUE;

CONTINUE:
	bp->m_Ofs = saveOfs;
	return FALSE;
}
void CHttp3Ctx::SendQPackFrame(int type, BYTE *pBuf, int len)
{
	CBuffer tmp;

	tmp.PutVarInt(0x01);
	tmp.PutVarInt(len);
	tmp.Apend(pBuf, len);

	m_pFifoProxy->Write(FIFO_STDOUT, tmp.GetPtr(), tmp.GetSize());

	TRACE("SendQPackFrame %d, %d\n", type, len);
}
