//////////////////////////////////////////////////////////////////////
// MidiData.cpp: CMidiData クラスのインプリメンテーション

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "TextRam.h"
#include "MidiData.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CMemBuffer
//////////////////////////////////////////////////////////////////////

CMemBuffer::CMemBuffer()
{
	m_Len = m_Max = 0;
	m_pData = NULL;
}

CMemBuffer::~CMemBuffer()
{
    if ( m_pData != NULL )
		delete [] m_pData;
}

void CMemBuffer::SizeCheck(int len)
{
	BYTE *pData;

	if ( (m_Len + len) < m_Max )
		return;
	while ( (m_Len + len) > m_Max )
		m_Max += (4 * 1024);
	pData = new BYTE[m_Max];
	if ( m_pData != NULL ) {
		memcpy(pData, m_pData, m_Len);
		delete [] m_pData;
	}
	m_pData = pData;
}

void CMemBuffer::AddByte(int data)
{
	SizeCheck(1);
	m_pData[m_Len++] = (BYTE)data;
}

void CMemBuffer::AddWord(int data)
{
	SizeCheck(2);
	m_pData[m_Len++] = (BYTE)(data >> 8);
	m_pData[m_Len++] = (BYTE)data;
}

void CMemBuffer::AddDWord(int data)
{
	SizeCheck(4);
	m_pData[m_Len++] = (BYTE)(data >> 24);
	m_pData[m_Len++] = (BYTE)(data >> 16);
	m_pData[m_Len++] = (BYTE)(data >> 8);
	m_pData[m_Len++] = (BYTE)data;
}

void CMemBuffer::AddBuff(BYTE *pData, int len)
{
	SizeCheck(len);
	memcpy(m_pData + m_Len, pData, len);
	m_Len += len;
}

void CMemBuffer::AddDelta(int data)
{
	int n = 8;
	BYTE tmp[8];

	while ( data > 127 ) {
		tmp[--n] = (BYTE)((data & 0x7F) | 0x80);
		data >>= 7;
	}
	tmp[--n] = (BYTE)((data & 0x7F) | 0x80);
	tmp[7] &= 0x7F;
	AddBuff(tmp + n, 8 - n);
}

void CMemBuffer::AddStr(LPCSTR str)
{
	AddBuff((BYTE *)str, (int)strlen(str));
}

void CMemBuffer::RemoveAll()
{
    if ( m_pData != NULL )
		delete [] m_pData;
	m_Len = m_Max = 0;
	m_pData = NULL;
}

//////////////////////////////////////////////////////////////////////
// CNode
//////////////////////////////////////////////////////////////////////

CNode::CNode()
{
	m_Next = m_Back = m_Link = NULL;
	m_Pos  = 0;
	m_Cmd  = 0;
	m_Pam.m_DWord = 0;
	m_Len  = 0;
	m_Data = NULL;

	m_Flag = FALSE;
	m_SubT = 0;
}

CNode::~CNode()
{
	if ( m_Data != NULL )
		delete [] m_Data;
}

int CNode::GetEventSize()
{
	int n = sizeof(DWORD) * 3;

	if ( (m_Cmd & 0xFF00) == 0xFF00 ) {
		switch(m_Cmd) {
		case 0xFF51:	// Tempo
			return n;
		case 0xFF80:	// Speek
			return (n + 4 + ((m_Len + (sizeof(DWORD) - 1)) / sizeof(DWORD)) * sizeof(DWORD));
		default:
			return 0;
		}
	}

	if ( m_Data != NULL )
		n += ((m_Len + 1 + (sizeof(DWORD) - 1)) / sizeof(DWORD)) * sizeof(DWORD);

	return n;
}

int CNode::SetEvent(DWORD pos, BYTE *pData)
{
	int n = 0;
	DWORD tmp;

	// dwDeltaTime
	tmp = m_Pos - pos;
	memcpy(pData + n, &tmp, sizeof(DWORD));
	n += sizeof(DWORD);

	// dwStreamID
	tmp = 0;
	memcpy(pData + n, &tmp, sizeof(DWORD));
	n += sizeof(DWORD);

	if ( (m_Cmd & 0xFF00) == 0xFF00 ) {
		switch(m_Cmd) {
		case 0xFF51:	// Tempo
			// dwEvent
			tmp = (MEVT_TEMPO << 24) | m_Pam.m_DWord;
			memcpy(pData + n, &tmp, sizeof(DWORD));
			n += sizeof(DWORD);
			return n;

		case 0xFF80:	// Speek
			tmp = ((MEVT_COMMENT << 24) | MEVT_F_CALLBACK | ((DWORD)sizeof(DWORD) + m_Len));
			memcpy(pData + n, &tmp, sizeof(DWORD));
			n += sizeof(DWORD);
			tmp = 0x0001;
			memcpy(pData + n, &tmp, sizeof(DWORD));
			n += sizeof(DWORD);
			memcpy(pData + n, m_Data, m_Len);
			n += ((m_Len + (sizeof(DWORD) - 1)) / sizeof(DWORD)) * sizeof(DWORD);
			return n;

		default:
			return 0;
		}

	}
	
	if ( m_Data == NULL ) {
		// dwEvent
		tmp = (MEVT_F_SHORT | m_Pam.m_DWord);
		memcpy(pData + n, &tmp, sizeof(DWORD));
		n += sizeof(DWORD);

#ifdef	_DEBUGXXX
		TRACE("%d: %08x\n", m_Pos, tmp);
#endif

	} else {
		// dwEvent
		tmp = (MEVT_F_LONG | (m_Len + 1));
		memcpy(pData + n, &tmp, sizeof(DWORD));
		n += sizeof(DWORD);

		// dwData
		pData[n] = (BYTE)m_Cmd;
		memcpy(pData + n + 1, m_Data, m_Len);

#ifdef	_DEBUGXXX
		TRACE("%d: ", m_Pos);
		for ( int i = 0 ; i < (m_Len + 1) ; i++ )
			TRACE("%02x ", pData[n + i]);
		TRACE("\n");
#endif

		n += ((m_Len + 1 + (sizeof(DWORD) - 1)) / sizeof(DWORD)) * sizeof(DWORD);

	}

	return n;
}

//////////////////////////////////////////////////////////////////////
// CNodeList
//////////////////////////////////////////////////////////////////////

CNodeList::CNodeList()
{
	m_Now = NULL;
	m_nTrack = 0;
	m_ActiveCh = 0;
}

CNodeList::~CNodeList()
{
	RemoveAll();
}

CNode *CNodeList::NewNode()
{
	return new CNode;
}

void CNodeList::FreeNode(CNode *pNode)
{
	delete pNode;
}

CNode *CNodeList::Link(CNode *pNode)
{
	if ( m_nTrack <= pNode->m_Track )
		m_nTrack = pNode->m_Track + 1;

	if ( (pNode->m_Cmd & 0xFF00) == 0 && (pNode->m_Cmd & 0xF0) != 0xF0 )
		m_ActiveCh |= (1 << (pNode->m_Cmd & 0x0F));

	if ( m_Now == NULL ) {
		pNode->m_Back = pNode->m_Next = NULL;
		m_Now = pNode;
	} else {
		while ( pNode->m_Pos < m_Now->m_Pos ) {
			if ( m_Now->m_Back == NULL ) {
				m_Now->m_Back = pNode;
				pNode->m_Back = NULL;
				pNode->m_Next = m_Now;
				m_Now = pNode;
				return m_Now;
			}
			m_Now = m_Now->m_Back;
		}
		while ( pNode->m_Pos >= m_Now->m_Pos ) {
			if ( m_Now->m_Next == NULL ) {
				m_Now->m_Next = pNode;
				pNode->m_Back = m_Now;
				pNode->m_Next = NULL;
				m_Now = pNode;
				return m_Now;
			}
			m_Now = m_Now->m_Next;
		}
		pNode->m_Back = m_Now->m_Back;
		m_Now->m_Back = pNode;
		pNode->m_Next = m_Now;
		if ( pNode->m_Back != NULL )
			pNode->m_Back->m_Next = pNode;
		m_Now = pNode;
	}

	return pNode;
}

void CNodeList::Unlink(CNode *pNode)
{
	if ( pNode->m_Back != NULL )
		pNode->m_Back->m_Next = pNode->m_Next;

	if ( pNode->m_Next != NULL )
		pNode->m_Next->m_Back = pNode->m_Back;

	if ( m_Now == pNode ) {
		if ( (m_Now = pNode->m_Next) == NULL )
			m_Now = pNode->m_Back;
	}
}

CNode *CNodeList::AddNode(DWORD pos, int track, int cmd, int note, int vol)
{
	CNode *pNode = NewNode();

	pNode->m_Pos     = pos;
	pNode->m_Track   = track;
	pNode->m_Cmd     = cmd;
	pNode->m_Pam.m_Byte[0]  = (BYTE)cmd;
	pNode->m_Pam.m_Byte[1]  = (BYTE)note;
	pNode->m_Pam.m_Byte[2]  = (BYTE)vol;
	pNode->m_Pam.m_Byte[3]  = 0;

	return Link(pNode);
}

CNode *CNodeList::AddNode(DWORD pos, int track, int cmd, int pam)
{
	CNode *pNode = NewNode();

	pNode->m_Pos     = pos;
	pNode->m_Track   = track;
	pNode->m_Cmd     = cmd;
	pNode->m_Pam.m_Byte[0]  = (BYTE)(pam);
	pNode->m_Pam.m_Byte[1]  = (BYTE)(pam >> 8);
	pNode->m_Pam.m_Byte[2]  = (BYTE)(pam >> 16);
	pNode->m_Pam.m_Byte[3]  = 0;

	return Link(pNode);
}

CNode *CNodeList::AddNode(DWORD pos, int track, int cmd, int len, BYTE *data)
{
	CNode *pNode = NewNode();

	pNode->m_Pos     = pos;
	pNode->m_Track   = track;
	pNode->m_Cmd     = cmd;
	pNode->m_Len     = len;
	pNode->m_Data    = new BYTE[len];
	memcpy(pNode->m_Data, data, len);

	return Link(pNode);
}

CNode *CNodeList::GetNode(DWORD pos)
{
	if ( m_Now == NULL )
		return NULL;

	while ( m_Now->m_Pos >= pos ) {
		if ( m_Now->m_Back == NULL )
			break;
		m_Now = m_Now->m_Back;
	}
	while ( m_Now->m_Pos < pos ) {
		if ( m_Now->m_Next == NULL )
			break;
		m_Now = m_Now->m_Next;
	}
	return m_Now;
}

CNode *CNodeList::TopNode(int nTrack)
{
	if ( m_Now == NULL )
		return NULL;

	while ( m_Now->m_Back != NULL )
		m_Now = m_Now->m_Back;

	if ( nTrack >= 0 ) {
		while ( m_Now->m_Track != nTrack ) {
			if ( m_Now->m_Next == NULL )
				return NULL;
			m_Now = m_Now->m_Next;
		}
	}

	return m_Now;
}

CNode *CNodeList::NextNode(int nTrack)
{
	if ( m_Now == NULL || m_Now->m_Next == NULL )
		return NULL;

	m_Now = m_Now->m_Next;

	if ( nTrack >= 0 ) {
		while ( m_Now->m_Track != nTrack ) {
			if ( m_Now->m_Next == NULL )
				return NULL;
			m_Now = m_Now->m_Next;
		}
	}

	return m_Now;
}

CNode *CNodeList::BackNode(int nTrack)
{
	if ( m_Now == NULL || m_Now->m_Back == NULL )
		return NULL;

	m_Now = m_Now->m_Back;

	if ( nTrack >= 0 ) {
		while ( m_Now->m_Track != nTrack ) {
			if ( m_Now->m_Back == NULL )
				return NULL;
			m_Now = m_Now->m_Back;
		}
	}

	return m_Now;
}

void CNodeList::RemoveAll()
{
	CNode *pNode;

	if ( m_Now == NULL )
		return;

	while ( m_Now->m_Back != NULL )
		m_Now = m_Now->m_Back;

	while ( (pNode = m_Now) != NULL ) {
		m_Now = pNode->m_Next;
		delete pNode;
	}
	m_nTrack = 0;
	m_ActiveCh = 0;
}

//////////////////////////////////////////////////////////////////////
// CMMLData
//////////////////////////////////////////////////////////////////////

CMMLData::CMMLData()
{
	m_Cmd   = 0;
	m_Track = 0;
	m_Note  = 60;		// N, CDEFGABC
	m_Step  = 96;		// L
	m_Gate  = 80;		// Q
	m_Velo  = 100;		// V
	m_Time  = 0;		// T
	m_Octa  = 5;		// O < >
	m_Keys  = 0;		// K

	m_pData = NULL;

	m_pNext = NULL;
	m_pLink = NULL;
	m_pLast = NULL;
	m_pWork = NULL;

	m_bNest = TRUE;
	m_bBase = FALSE;
	m_bTai  = TRUE;

	m_AddStep = 0;
	m_RandVol = 0;
}
CMMLData::CMMLData(class CMMLData &data)
{
	m_Cmd   = data.m_Cmd;
	m_Track = data.m_Track;
	m_Note  = data.m_Note;
	m_Step  = data.m_Step;
	m_Gate  = data.m_Gate;
	m_Velo  = data.m_Velo;
	m_Time  = data.m_Time;
	m_Octa  = data.m_Octa;
	m_Keys  = data.m_Keys;

	m_pData = NULL;

	m_pNext = NULL;
	m_pLink = NULL;
	m_pLast = NULL;
	m_pWork = NULL;

	m_bNest = TRUE;
	m_bBase = FALSE;
	m_bTai  = TRUE;

	m_AddStep = 0;
	m_RandVol = data.m_RandVol;
}
CMMLData::~CMMLData()
{
	RemoveAll();
}
void CMMLData::RemoveAll()
{
	if ( m_pData != NULL )
		delete [] m_pData;
	m_pData = NULL;

	if ( m_pLink != NULL )
		delete m_pLink;
	m_pLink = m_pLast = NULL;

	if ( m_pWork != NULL )
		delete m_pWork;
	m_pWork = NULL;

	// ネストが深い場合がある
	if ( m_pNext != NULL ) {
		if ( m_bNest ) {
			CMMLData *pData = m_pNext;
			for ( int n = 0 ; n < 64 ; n++ ) {
				pData->m_bNest = FALSE;
				if ( pData->m_pNext == NULL )
					break;
				pData = pData->m_pNext;
			}
			if ( pData->m_pNext != NULL ) {
				delete pData->m_pNext;
				pData->m_pNext = NULL;
			}
		}
		delete m_pNext;
	}
	m_pNext = NULL;
}
CMMLData *CMMLData::Add(CMMLData *pData)
{
	if ( m_pLink == NULL ) {
		m_pLink = m_pLast = pData;
	} else {
		m_pLast->m_pNext = pData;
		m_pLast = pData;
	}
//	pData->m_pNext = NULL;

	return pData;
}
const class CMMLData & CMMLData::operator = (class CMMLData &data)
{
	m_Cmd   = data.m_Cmd;
	m_Track = data.m_Track;
	m_Note  = data.m_Note;
	m_Step  = data.m_Step;
	m_Gate  = data.m_Gate;
	m_Velo  = data.m_Velo;
	m_Time  = data.m_Time;
	m_Octa  = data.m_Octa;
	m_Keys  = data.m_Keys;

	m_bBase = data.m_bBase;
	m_bTai  = data.m_bTai;

	m_AddStep = data.m_AddStep;
	m_RandVol = data.m_RandVol;

	if ( m_pData != NULL )
		delete [] m_pData;
	m_pData = data.m_pData;
	data.m_pData = NULL;

	if ( m_pLink != NULL )
		delete m_pLink;
	m_pLink = data.m_pLink;
	m_pLast = data.m_pLast;
	data.m_pLink = NULL;
	data.m_pLast = NULL;

	m_pWork = data.m_pWork;
	data.m_pWork = NULL;

	return *this;
}

//////////////////////////////////////////////////////////////////////
// CMidiData
//////////////////////////////////////////////////////////////////////

CMidiData::CMidiData()
{
	m_Size = m_Pos = 0;
	m_PlayMode = 0;
	m_DivVal = 96;
	m_pStep = NULL;
	m_LastClock = 0;
	m_pPlayNode = m_pDispNode = NULL;
	m_PlayPos = m_SeekPos = 0;
	m_hStream = NULL;

	ZeroMemory(m_mmlPos, sizeof(m_mmlPos));
	ZeroMemory(m_mmlBase, sizeof(m_mmlBase));
	memset(m_mmlPitch, 2, sizeof(m_mmlPitch));
	memcpy(m_mmlIds, "\x41\x10\x42\x12", 4);

	for ( int n = 0 ; n < MIDIHDR_MAX ;  n++ ) {
		memset(&(m_MidiHdr[n]), 0, sizeof(MIDIHDR));
		m_MidiHdr[n].lpData = new char[MIDIDATA_MAX];
		m_MidiHdr[n].dwBufferLength = MIDIDATA_MAX;
		m_MidiHdr[n].dwUser = n;
	}

	m_hThread = CreateThread(NULL, 0, CMidiData::ThreadProc, this, 0, &m_dwThreadID);
	while( !PostThreadMessage(m_dwThreadID, WM_APP, 0, 0) )
		Sleep(1);

	m_hStream = NULL;
	UINT dev = MIDI_MAPPER;

	if ( midiStreamOpen(&m_hStream, &dev, 1, m_dwThreadID, NULL, CALLBACK_THREAD) != MMSYSERR_NOERROR )
		m_hStream = NULL;

	if ( m_hStream != NULL ) {
		MIDIPROPTIMEDIV proptime;
		proptime.cbStruct  = sizeof(MIDIPROPTIMEDIV);
		proptime.dwTimeDiv = m_DivVal;
		midiStreamProperty(m_hStream, (BYTE*)&proptime, MIDIPROP_SET | MIDIPROP_TIMEDIV);
	}
}

CMidiData::~CMidiData()
{
	int n;

	if ( m_hStream != NULL ) {
		midiStreamStop(m_hStream);
		midiOutReset((HMIDIOUT)m_hStream);
		for ( n = 0 ; n < MIDIHDR_MAX ; n++ )
			midiOutUnprepareHeader((HMIDIOUT)m_hStream, &(m_MidiHdr[n]), sizeof(MIDIHDR));
		midiStreamClose(m_hStream);
	}

	if ( m_hThread != NULL )
		CloseHandle(m_hThread);

	for ( n = 0 ; n < MIDIHDR_MAX ; n++ )
		delete [] m_MidiHdr[n].lpData;
}

void CMidiData::SetBuffer(BYTE *pData, int len)
{
	m_Pos = 0;
	m_Size = len;
	m_pData = pData;
}

int CMidiData::IsEmpty()
{
	return (m_Pos >= m_Size ? TRUE : FALSE);
}

int CMidiData::GetByte()
{
	return (IsEmpty() ? (-1) : m_pData[m_Pos++]);
}

BYTE *CMidiData::GetData(int len)
{
	BYTE *p = (m_pData + m_Pos);
	if ( (m_Pos + len) > m_Size )
		return NULL;
	m_Pos += len;
	return p;
}

int CMidiData::GetDelta()
{
	int n, c;
	int delta = 0;

	for ( n = 0 ; ; n++ ) {
		if ( (c = GetByte()) < 0 || n >= 4 )
			return (-1);
		delta += (c & 0x7F);
		if ( (c & 0x80) == 0 )
			break;
		delta <<= 7;
	}
	return delta;
}

BOOL CMidiData::SetSmfData(BYTE *pData, int len)
{
	int n, c, s, d, v;
	int size, nTrack;
	DWORD delta, total;
	BYTE *p;
	CNode *pNode, *pTemp;

	RemoveAll();
	m_pPlayNode = m_pDispNode = NULL;
	m_PlayPos = m_SeekPos = 0;
	m_pStep = pTemp = NULL;
	m_LastClock = 0;

	if ( len < 18 )
		return FALSE;

	if ( strncmp((LPCSTR)(pData + 0), "MThd", 4) != 0 )
		return FALSE;

	n = (pData[4] << 24) + (pData[5] << 16) + (pData[6] << 8) + pData[7];
	if ( n != 6 )
		return FALSE;

	n = (pData[8] << 8) + pData[9];
	if ( n > 1 )
		return FALSE;

	nTrack   = (pData[10] << 8) + pData[11];
	m_DivVal = (pData[12] << 8) + pData[13];

	if ( m_hStream != NULL ) {
		MIDIPROPTIMEDIV proptime;
		proptime.cbStruct  = sizeof(MIDIPROPTIMEDIV);
		proptime.dwTimeDiv = m_DivVal;
		midiStreamProperty(m_hStream, (BYTE*)&proptime, MIDIPROP_SET | MIDIPROP_TIMEDIV);
	}

	pData += 14;
	len   -= 14;

	for ( n = 0 ; len > 8 && n < nTrack ; n++ ) {
		if ( strncmp((LPCSTR)pData, "MTrk", 4) != 0 )
			return FALSE;
		size = (pData[4] << 24) + (pData[5] << 16) + (pData[6] << 8) + pData[7];

		pData += 8;
		len   -= 8;
		if ( size > len )
			return FALSE;

		SetBuffer(pData, size);

		total = 0;
		s = 0x80;
		while ( !IsEmpty() ) {
			if ( (delta = GetDelta()) < 0 )
				break;
			total += delta;

			if ( (c = GetByte()) < 0 )
				break;
			if ( (c & 0x80) == 0 ) {
				c = s;
				UnGetByte();
			}
			s = c;

			if ( c == 0xFF ) {
				if ( (d = GetByte()) < 0 )
					break;
				c = (c << 8) | d;

				if ( (d = GetDelta()) < 0 )
					break;
				if ( (p = GetData(d)) == NULL )
					break;

				if ( c == 0xFF51 ) {	// Tempo Cheng
					if ( d >= 3 )
						AddNode(total, 0, c, (p[0] << 16) | (p[1] << 8) | p[2]);
				} else
					AddNode(total, n, c, d, p);

			} else if ( c == 0xF0 || c == 0xF7 ) {
				if ( (d = GetDelta()) < 0 )
					break;
				if ( (p = GetData(d)) == NULL )
					break;

				AddNode(total, n, c, d, p);

			} else if ( (c & 0xF0) == 0xF0 ) {
				AddNode(total, n, c, 0, 0);

			} else if ( (c & 0xF0) == 0xC0 || (c & 0xF0) == 0xD0 ) {
				if ( (d = GetByte()) < 0 )
					break;
				AddNode(total, n, c, d, 0);

			} else {
				if ( (d = GetByte()) < 0 )
					break;
				if ( (v = GetByte()) < 0 )
					break;
				if ( (c & 0xF0) == 0x90 && v == 0 )
					c &= 0xEF;		// 0x80
				AddNode(total, n, c, d, v);
			}
		}

		if ( m_LastClock < total )
			m_LastClock = total;

		pData += size;
		len   -= size;
	}


	// 拍子記号を抜き出し
	m_pStep = pTemp = NULL;
	for ( pNode = TopNode() ; pNode != NULL ; pNode = pNode->m_Next ) {
		if ( pNode->m_Cmd == 0xFF58 && pNode->m_Len >= 4 && pNode->m_Data != NULL && pNode->m_Data[0] != 0 ) {
			if ( pTemp == NULL )
				m_pStep = pNode;
			else
				pTemp->m_Link = pNode;
			pNode->m_Link = NULL;
			pTemp = pNode;
		}
	}

	return TRUE;
}
BOOL CMidiData::LoadFile(LPCTSTR fileName)
{
	CFile File;
	BYTE *pData;
	DWORD size;

	if ( !File.Open(fileName, CFile::modeRead) )
		return FALSE;

	size = (DWORD)File.GetLength();
	pData = new BYTE[size];

	if ( File.Read((LPSTR)pData, size) != size )
		return FALSE;

	File.Close();

	if ( !SetSmfData(pData, size) )
		return FALSE;

	delete [] pData;

	return TRUE;
}

BOOL CMidiData::SaveFile(LPCTSTR fileName)
{
	int n;
	CFile File;
	CMemBuffer mem;
	CMemBuffer tmp;
	CNode *pNode;
	DWORD total;

	if ( !File.Open(fileName, CFile::modeCreate | CFile::modeWrite) )
		return FALSE;

	mem.AddBuff((BYTE *)"MThd", 4);
	mem.AddDWord(6);
	mem.AddWord(1);
	mem.AddWord(m_nTrack);
	mem.AddWord(m_DivVal);

	for ( n = 0 ; n < m_nTrack ; n++ ) {
		tmp.RemoveAll();

		total = 0;
		for ( pNode = TopNode(n) ; pNode != NULL ; pNode = NextNode(n) ) {
			if ( pNode->m_Cmd == 0xFFFF || pNode->m_Cmd == 0xFFFE || pNode->m_Cmd == 0xFF2F || pNode->m_Cmd == 0xFF80 )
				continue;
			tmp.AddDelta(pNode->m_Pos - total);
			total = pNode->m_Pos;
			if ( pNode->m_Cmd == 0xFF51 ) {
				tmp.AddByte(pNode->m_Cmd >> 8);
				tmp.AddByte(pNode->m_Cmd);
				tmp.AddDelta(3);
				tmp.AddByte(pNode->m_Pam.m_Byte[2]);
				tmp.AddByte(pNode->m_Pam.m_Byte[1]);
				tmp.AddByte(pNode->m_Pam.m_Byte[0]);
			} else if ( (pNode->m_Cmd & 0xFF00) == 0xFF00 ) {
				tmp.AddByte(pNode->m_Cmd >> 8);
				tmp.AddByte(pNode->m_Cmd);
				tmp.AddDelta(pNode->m_Len);
				tmp.AddBuff(pNode->m_Data, pNode->m_Len);
			} else if ( pNode->m_Cmd == 0xF0 || pNode->m_Cmd == 0xF7 ) {
				tmp.AddByte(pNode->m_Cmd);
				tmp.AddDelta(pNode->m_Len);
				tmp.AddBuff(pNode->m_Data, pNode->m_Len);
			} else if ( (pNode->m_Cmd & 0xF0) == 0xC0 || (pNode->m_Cmd & 0xF0) == 0xD0 ) {
				tmp.AddByte(pNode->m_Pam.m_Byte[0]);
				tmp.AddByte(pNode->m_Pam.m_Byte[1]);
			} else {
				tmp.AddByte(pNode->m_Pam.m_Byte[0]);
				tmp.AddByte(pNode->m_Pam.m_Byte[1]);
				tmp.AddByte(pNode->m_Pam.m_Byte[2]);
			}
		}
		tmp.AddDelta(0);
		tmp.AddBuff((BYTE *)"\xFF\x2F\x00", 3);

		mem.AddBuff((BYTE *)"MTrk", 4);
		mem.AddDWord(tmp.GetSize());
		mem.AddBuff(tmp.GetData(), tmp.GetSize());
	}

	File.Write(mem.GetData(), mem.GetSize());
	File.Close();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////

					//		A	A#	B	C	C#	D	D#	E	F	F#	G	G#
static int NoteTab[] = {	9,		11,	0,		2,		4,	5,		7		};
static LPCSTR NoteName[] = { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b" };

int CMidiData::ParseChar()
{
	int ch;

	for ( ; ; ) {
		while ( *m_mmlStr <= ' ' && *m_mmlStr != '\0' )
			m_mmlStr++;

		if ( m_mmlStr[0] == '/' && m_mmlStr[1] == '/' ) {
			while ( *m_mmlStr != '\r' && *m_mmlStr != '\n' && *m_mmlStr != '\0' )
				m_mmlStr++;
		} else
			break;
	}

	if ( (ch = *m_mmlStr) == '\0' )
		return ch;
	m_mmlStr++;

	if ( ch >= 'a' && ch <= 'z' )
		ch -= 0x20;

	else if ( ch >= '0' && ch <= '9' ) {
		m_mmlValue = ch - '0';
		while ( *m_mmlStr >= '0' && *m_mmlStr <= '9' ) {
			if ( m_mmlValue < (MML_NOVALUE / 10) )
				m_mmlValue = m_mmlValue * 10 + (*(m_mmlStr++) - '0');
			else {
				m_mmlValue = MML_NOVALUE - 1;
				m_mmlStr++;
			}
		}
		ch = '0';

	} else if ( ch == '$' ) {
		m_mmlValue = 0;
		while ( *m_mmlStr != '\0' ) {
			if ( *m_mmlStr >= '0' && *m_mmlStr <= '9' )
				m_mmlValue = m_mmlValue * 16 + (*(m_mmlStr++) - '0');
			else if ( *m_mmlStr >= 'A' && *m_mmlStr <= 'F' )
				m_mmlValue = m_mmlValue * 16 + (*(m_mmlStr++) - 'A' + 10);
			else if ( *m_mmlStr >= 'a' && *m_mmlStr <= 'f' )
				m_mmlValue = m_mmlValue * 16 + (*(m_mmlStr++) - 'a' + 10);
			else
				break;
		}
		ch = '0';
	}

	return ch;
}

int CMidiData::ParseDigit(int ch)
{
	//	digit
	//	( expr )

	if ( ch == '(' ) {
		if ( (ch = ParseAddSub(ParseChar())) == ')' )
			ch = ParseChar();
	} else if ( ch == '0' )
		ch = ParseChar();
	else
		m_mmlValue = MML_NOVALUE;

	return ch;
}
int CMidiData::ParseSingle(int ch)
{
	//	+ expr
	//	- expr

	if ( ch == '+' ) {
		ch = ParseSingle(ParseChar());
	} else if ( ch == '-' ) {
		ch = ParseSingle(ParseChar());
		if ( m_mmlValue != MML_NOVALUE )
			m_mmlValue = 0 - m_mmlValue;
	} else
		ch = ParseDigit(ch);

	return ch;
}
int CMidiData::ParseMulDiv(int ch)
{
	// expr * expr
	// expr / expr
	// expr % expr

	ch = ParseSingle(ch);
	int a = m_mmlValue;

	for ( ; ; ) {
		if ( ch == '*' && a != MML_NOVALUE ) {
			ch = ParseSingle(ParseChar());
			if ( m_mmlValue != MML_NOVALUE )
				m_mmlValue = a * m_mmlValue;
		} else if ( ch == '/' && a != MML_NOVALUE ) {
			ch = ParseSingle(ParseChar());
			if ( m_mmlValue != MML_NOVALUE && m_mmlValue != 0 )
				m_mmlValue = a / m_mmlValue;
		} else if ( ch == '%' && a != MML_NOVALUE ) {
			ch = ParseSingle(ParseChar());
			if ( m_mmlValue != MML_NOVALUE && m_mmlValue != 0 )
				m_mmlValue = a % m_mmlValue;
		} else
			return ch;

		a = m_mmlValue;
	}
}
int CMidiData::ParseAddSub(int ch)
{
	// expr + expr
	// expr - expr

	ch = ParseMulDiv(ch);
	int a = m_mmlValue;

	for ( ; ; ) {
		if ( ch == '+' ) {
			ch = ParseMulDiv(ParseChar());
			if ( a != MML_NOVALUE && m_mmlValue != MML_NOVALUE )
				m_mmlValue = a + m_mmlValue;
		} else if ( ch == '-' ) {
			ch = ParseMulDiv(ParseChar());
			if ( a != MML_NOVALUE && m_mmlValue != MML_NOVALUE )
				m_mmlValue = a - m_mmlValue;
		} else
			return ch;

		a = m_mmlValue;
	}
}
int CMidiData::ParseParam(int ch)
{
	// expr[,expr]

	ch = ParseAddSub(ch);

	m_mmlParam.RemoveAll();
	m_mmlParam.Add(m_mmlValue);

	while ( ch == ',' ) {
		ch = ParseAddSub(ParseChar());
		m_mmlParam.Add(m_mmlValue);
	}

	return ch;
}
int CMidiData::ParseStep(int ch)
{
	// expr[.]

	BOOL bStep = FALSE;

	if ( ch == '%' ) {
		bStep = TRUE;
		ch = ParseChar();
	}

	ch = ParseAddSub(ch);

	if ( m_mmlValue == MML_NOVALUE )
		m_mmlValue = m_mmlData.m_Step;

	else if ( !bStep ) {
		if ( m_mmlValue == 0 )
			m_mmlValue = m_mmlData.m_Step;
		else
			m_mmlValue = MML_STEPMAX / m_mmlValue;
	}

	while ( ch == '.' ) {
		m_mmlValue = m_mmlValue + m_mmlValue / 2;
		ch = ParseChar();
	}

	return ch;
}
int CMidiData::Velocity(int vol, int randum)
{
	int max;

	if ( randum > 0 ) {
		if ( (max = vol * randum / 100) > 0 )
			vol += (max / 2 - (rand() % max));
	}
	
	if ( vol < 0 )
		vol = 0;
	else if ( vol > 127 )
		vol = 127;

	return vol;
}
int CMidiData::ParseNote(int ch)
{
	//	Ln[.]									音の長さをn分音符で指定(1-96初期値4)
	//	Qn										音の長さに対するゲートタイム(0-100初期値80%)
	//	Vn										ベロシティを指定(0-127初期値100)
	//	Sn										発音のタイミングをステップ単位で指定(初期値0)
	//	On										オクターブ(音階)を指定(0-10初期値5)
	//	<|>										オクターブダウン|アップ
	//	"|`										一時的オクターブダウン|アップ

	//	A-G[+-#][l][.][,q][,v][,s][,o]			音階(A-G)
	//	Nn[,l][.][,q][,v][,s][,o]				ノート番号(0-127)
	//	In[,l[.]][,q][,v][,s][,o]				前音のオフセット
	//	R[l][.]									休符
	//	[^|&]									スラー/タイ

	//	~n[,n]...								Ax,n,n	Polyphonic Key Pressure
	//	Yn,n[n,n]...							Bx,n,n	Control Change
	//	@n[,b]									Cx,n	Program Change
	//	!n[,n]...								Dx,p	Channel Pressure
	//	Pw[,w]...								Ex,L,M	Pitch Bend (P=-8191-8192:0)

	//	Mn[,n]									Bx,1,n	Modulation(0-127:0)
	//	Wn[,n]									Bx,10,n	Pan(0-127:64)
	//	Un[,n]									Bx,11,n	Expression(0-127:127)

	//	Zn[,n]									CH指定(1-16)
	//	Tn										テンポ(120)
	//	Xn[,n]...								システムエクスクルーシブ

	//	[...][+-#][n][.]						和音
	//	{...}[n][.]								連符

	int step, offs;
	CMMLData *pLast = NULL;
	CMMLData *pData = NULL;
	CMMLData save;
	int nTai = 0;
	int nOct = 0;
	int nVol = 0;
	CStringA mbs;

	for ( ; ; ) { 
		switch(ch) {
		case 'L':
			ch = ParseStep(ParseChar());
			m_mmlData.m_Step = m_mmlValue;
			break;

		case 'Q':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE )
				m_mmlData.m_Gate = m_mmlValue;
			break;

		case 'V':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE ) {
				if ( (m_mmlData.m_Velo = m_mmlValue) < 0 )
					m_mmlData.m_Velo = 0;
				else if ( m_mmlData.m_Velo > 127 )
					m_mmlData.m_Velo = 127;
			} else if ( ch == '?' ) {
				ch = ParseAddSub(ParseChar());
				if ( m_mmlValue != MML_NOVALUE )
					m_mmlData.m_RandVol = m_mmlValue;
				else
					m_mmlData.m_RandVol = 0;
			} else {
				for ( ; ; ) {
					if ( ch == '<' ) {
						ch = ParseChar();
						m_mmlData.m_Velo -= (m_mmlData.m_Velo / 15);
					} else if ( ch == '>' ) {
						ch = ParseChar();
						m_mmlData.m_Velo += (m_mmlData.m_Velo / 15);
					} else if ( ch == '"' ) {
						ch = ParseChar();
						nVol -= (m_mmlData.m_Velo / 15);
					} else if ( ch == '`' ) {
						ch = ParseChar();
						nVol += (m_mmlData.m_Velo / 15);
					} else
						break;
				}
			}
			break;

		case 'S':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE )
				m_mmlData.m_Time = m_mmlValue;
			break;

		case 'O':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE ) {
				if ( (m_mmlData.m_Octa = m_mmlValue) < 0 )
					m_mmlData.m_Octa = 0;
				else if ( m_mmlData.m_Octa > 10 )
					m_mmlData.m_Octa = 10;
			}
			break;

		case 'K':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE )
				m_mmlData.m_Keys = m_mmlValue;
			break;

		case '<':
			ch = ParseChar();
			if ( --m_mmlData.m_Octa < 0 )
				m_mmlData.m_Octa = 0;
			break;

		case '>':
			ch = ParseChar();
			if ( ++m_mmlData.m_Octa > 10 )
				m_mmlData.m_Octa = 10;
			break;

		case '"':
			ch = ParseChar();
			nOct--;
			break;

		case '`':
			ch = ParseChar();
			nOct++;
			break;

		case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F': case 'G':
			pData = new CMMLData(m_mmlData);

			pData->m_Note = NoteTab[ch - 'A'];
			for ( ; ; ) {
				ch = ParseChar();
				if ( ch == '+' || ch == '#' ) {
					pData->m_Note++;
				} else if ( ch == '-' ) {
					pData->m_Note--;
				} else
					break;
			}

			ch = ParseStep(ch);
			pData->m_Step = m_mmlValue;

			if ( ch == ',' ) {
				ch = ParseParam(ParseChar());
				if ( m_mmlParam.GetSize() > 0 && m_mmlParam[0] != MML_NOVALUE )
					pData->m_Gate = m_mmlParam[0];
				if ( m_mmlParam.GetSize() > 1 && m_mmlParam[1] != MML_NOVALUE )
					pData->m_Velo = m_mmlParam[1];
				if ( m_mmlParam.GetSize() > 2 && m_mmlParam[2] != MML_NOVALUE )
					pData->m_Time = m_mmlParam[2];
				if ( m_mmlParam.GetSize() > 3 && m_mmlParam[3] != MML_NOVALUE )
					pData->m_Octa = m_mmlParam[3];
			}

			if ( (pData->m_Note = pData->m_Note + (pData->m_Octa + nOct) * 12 + pData->m_Keys) < 0 )
				pData->m_Note = 0;
			else if ( pData->m_Note > 127 )
				pData->m_Note = 127;

			pData->m_Cmd = 0x80;
			pData->m_Velo = Velocity(pData->m_Velo + nVol, pData->m_RandVol);
			m_mmlData.m_Note = pData->m_Note;

			pLast = JointData(pLast, pData, nTai);
			nTai = 0;
			nOct = 0;
			nVol = 0;
			break;

		case 'N':
			pData = new CMMLData(m_mmlData);

			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE )
				pData->m_Note = m_mmlValue;

			if ( ch == ',' ) {
				ch = ParseStep(ParseChar());
				pData->m_Step = m_mmlValue;

				if ( ch  == ',' ) {
					ch = ParseParam(ParseChar());
					if ( m_mmlParam.GetSize() > 0 && m_mmlParam[0] != MML_NOVALUE )
						pData->m_Gate = m_mmlParam[0];
					if ( m_mmlParam.GetSize() > 1 && m_mmlParam[1] != MML_NOVALUE )
						pData->m_Velo = m_mmlParam[1];
					if ( m_mmlParam.GetSize() > 2 && m_mmlParam[2] != MML_NOVALUE )
						pData->m_Time = m_mmlParam[2];
					if ( m_mmlParam.GetSize() > 3 && m_mmlParam[3] != MML_NOVALUE )
						pData->m_Octa = m_mmlParam[3];
				}
			}

			pData->m_Cmd = 0x80;
			pData->m_Velo = Velocity(pData->m_Velo + nVol, pData->m_RandVol);
			m_mmlData.m_Note = pData->m_Note;

			pLast = JointData(pLast, pData, nTai);
			nTai = 0;
			nOct = 0;
			nVol = 0;
			break;

		case 'I':
			pData = new CMMLData(m_mmlData);

			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE )
				pData->m_Note += m_mmlValue;

			if ( ch == ',' ) {
				ch = ParseStep(ParseChar());
				pData->m_Step = m_mmlValue;

				if ( ch  == ',' ) {
					ch = ParseParam(ParseChar());
					if ( m_mmlParam.GetSize() > 0 && m_mmlParam[0] != MML_NOVALUE )
						pData->m_Gate = m_mmlParam[0];
					if ( m_mmlParam.GetSize() > 1 && m_mmlParam[1] != MML_NOVALUE )
						pData->m_Velo = m_mmlParam[1];
					if ( m_mmlParam.GetSize() > 2 && m_mmlParam[2] != MML_NOVALUE )
						pData->m_Time = m_mmlParam[2];
					if ( m_mmlParam.GetSize() > 3 && m_mmlParam[3] != MML_NOVALUE )
						pData->m_Octa = m_mmlParam[3];
				}
			}

			pData->m_Cmd = 0x80;
			pData->m_Velo = Velocity(pData->m_Velo + nVol, pData->m_RandVol);
			m_mmlData.m_Note = pData->m_Note;

			pLast = JointData(pLast, pData, nTai);
			nTai = 0;
			nOct = 0;
			nVol = 0;
			break;

		case 'R':
			pData = new CMMLData(m_mmlData);
			ch = ParseStep(ParseChar());
			pData->m_Cmd = 0x70;
			pData->m_Step = m_mmlValue;

			pLast = JointData(pLast, pData, nTai);
			nTai = 0;
			nOct = 0;
			nVol = 0;
			break;

		case '&':
			ch = ParseChar();
			nTai = 1;
			break;

		case '^':
			ch = ParseChar();
			nTai = 2;
			break;

		case '~':
			ch = ParseParam(ParseChar());
			step = (pLast != NULL ? pLast->m_Step : m_mmlData.m_Step);
			offs = (pLast != NULL ? pLast->m_Time : m_mmlData.m_Time);

			for ( int n = 0 ; n < m_mmlParam.GetSize() ; n++ ) {
				if ( m_mmlParam[n] != MML_NOVALUE ) {
					int w = (short)m_mmlParam[n];
					if ( w < 0 ) w = 0; else if ( w > 127 ) w = 127;
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xA0;
					pData->m_Note = (pLast != NULL ? pLast->m_Note : m_mmlData.m_Note);
					pData->m_Velo = w;
					pData->m_Time = offs + step * n / (int)m_mmlParam.GetSize();
					pData->m_Step = 0;
					pData->m_bBase = TRUE;
					m_mmlData.Add(pData);
				}
			}

			pData = new CMMLData(m_mmlData);
			pData->m_Cmd = 0xA0;
			pData->m_Note = (pLast != NULL ? pLast->m_Note : m_mmlData.m_Note);
			pData->m_Velo = 0;
			pData->m_Time = 0;
			pData->m_Step = 0;
			m_mmlData.Add(pData);
			break;

		case 'Y':
			if ( (ch = ParseChar()) == 'Y' ) {		// RPN/NRPN
				ch = ParseParam(ParseChar());
				if ( m_mmlParam.GetSize() > 0 && m_mmlParam[0] != MML_NOVALUE ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 101;
					pData->m_Velo = m_mmlParam[0];
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				}
				if ( m_mmlParam.GetSize() > 1 && m_mmlParam[1] != MML_NOVALUE ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 100;
					pData->m_Velo = m_mmlParam[1];
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				}
				if ( m_mmlParam.GetSize() > 2 && m_mmlParam[2] != MML_NOVALUE ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 6;
					pData->m_Velo = m_mmlParam[2];
					pData->m_Step = 0;
					m_mmlData.Add(pData);

					if ( m_mmlParam[0] == 0 && m_mmlParam[1] == 0 )
						m_mmlPitch[pData->m_Track] = (BYTE)m_mmlParam[2];		// Pitch Bend Sensitivity 
				}
				if ( m_mmlParam.GetSize() > 3 && m_mmlParam[3] != MML_NOVALUE ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 38;
					pData->m_Velo = m_mmlParam[3];
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				}

			} else {
				ch = ParseParam(ch);
				for ( int n = 0 ; n < (m_mmlParam.GetSize() - 1) ; n += 2 ) {
					if ( m_mmlParam[n] != MML_NOVALUE && m_mmlParam[n + 1] != MML_NOVALUE ) {
						int c = (short)m_mmlParam[n];
						if ( c < 0 ) c = 0; else if ( c > 127 ) c = 127;
						int w = (short)m_mmlParam[n + 1];
						if ( w < 0 ) w = 0; else if ( w > 127 ) w = 127;
						pData = new CMMLData(m_mmlData);
						pData->m_Cmd = 0xB0;
						pData->m_Note = c;
						pData->m_Velo = w;
						pData->m_Step = 0;
						m_mmlData.Add(pData);
					}
				}
			}
			break;

		case '@':
			ch = ParseParam(ParseChar());
			if ( m_mmlParam.GetSize() >= 2 && m_mmlParam[1] != MML_NOVALUE ) {
				int w = (short)m_mmlParam[1];
				if ( w < 0 ) w = 0; else if ( w > 127 ) w = 127;
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0xB0;
				pData->m_Note = 0;	// BankMSB Select
				pData->m_Velo = w;
				pData->m_Step = 0;
				m_mmlData.Add(pData);
			}
			if ( m_mmlParam.GetSize() >= 3 && m_mmlParam[2] != MML_NOVALUE ) {
				int w = (short)m_mmlParam[2];
				if ( w < 0 ) w = 0; else if ( w > 127 ) w = 127;
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0xB0;
				pData->m_Note = 32;	// BankLSB Select
				pData->m_Velo = w;
				pData->m_Step = 0;
				m_mmlData.Add(pData);
			}
			if ( m_mmlParam.GetSize() >= 1 && m_mmlParam[0] != MML_NOVALUE ) {
				int w = (short)m_mmlParam[0];
				if ( w < 1 ) w = 1; else if ( w > 128 ) w = 128;
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0xC0;
				pData->m_Note = w - 1;
				pData->m_Velo = 0;
				pData->m_Step = 0;
				m_mmlData.Add(pData);
			}
			break;

		case '!':
			ch = ParseParam(ParseChar());
			step = (pLast != NULL ? pLast->m_Step : m_mmlData.m_Step);
			offs = (pLast != NULL ? pLast->m_Time : m_mmlData.m_Time);

			for ( int n = 0 ; n < m_mmlParam.GetSize() ; n++ ) {
				if ( m_mmlParam[n] != MML_NOVALUE ) {
					int w = (short)m_mmlParam[n];
					if ( w < 0 ) w = 0; else if ( w > 127 ) w = 127;
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xD0;
					pData->m_Note = w;
					pData->m_Velo = 0;
					pData->m_Time = offs + step * n / (int)m_mmlParam.GetSize();
					pData->m_Step = 0;
					pData->m_bBase = TRUE;
					m_mmlData.Add(pData);
				}
			}

			pData = new CMMLData(m_mmlData);
			pData->m_Cmd = 0xD0;
			pData->m_Note = 0;
			pData->m_Velo = 0;
			pData->m_Time = 0;
			pData->m_Step = 0;
			m_mmlData.Add(pData);
			break;

		case 'P':
			if ( (ch = ParseChar()) == '<' || ch == '>' ) {
				int vect = (ch == '<' ? (-1) : 1);
				int tone = 2;
				step = (pLast != NULL ? pLast->m_Step : m_mmlData.m_Step);
				offs = (pLast != NULL ? pLast->m_Time : m_mmlData.m_Time);

				ch = ParseAddSub(ParseChar());
				if ( m_mmlValue != MML_NOVALUE && m_mmlValue > 0 )
					step = step / m_mmlValue;

				if ( ch == ',' ) {
					ch = ParseAddSub(ParseChar());
					if ( m_mmlValue != MML_NOVALUE )
						tone = m_mmlValue;
				}

				int w = 8192 * (tone * vect) / m_mmlPitch[m_mmlData.m_Track];
				if ( w < -8192 ) w = -8192;
				else if ( w > 8191 ) w = 8191;

				step = 6 * ((step + 3) / 6);
				for ( int n = step ; n >= 0 ; n -= 6 ) {
					pData = new CMMLData(m_mmlData);
					double d = (double)n / (double)step;
					int v = (int)((double)w * (d * d));
					pData->m_Cmd = 0xE0;
					pData->m_Note = (BYTE)((8192 - v) % 128);
					pData->m_Velo = (BYTE)((8192 - v) / 128);
					pData->m_Time = offs + step - n;
					pData->m_Step = 0;
					pData->m_bBase = TRUE;
					m_mmlData.Add(pData);
				}

			} else {
				ch = ParseParam(ch);
				if ( m_mmlParam.GetSize() == 1 && m_mmlParam[0] != MML_NOVALUE ) {
					int w= 8192 + (short)m_mmlParam[0];
					if ( w < 0 ) w = 0;
					else if ( w > 0x3FFF ) w = 0x3FFF;
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xE0;
					pData->m_Note = (BYTE)(w % 128);
					pData->m_Velo = (BYTE)(w / 128);
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				} else {
					step = (pLast != NULL ? pLast->m_Step : m_mmlData.m_Step);
					offs = (pLast != NULL ? pLast->m_Time : m_mmlData.m_Time);

					for ( int n = 0 ; n < m_mmlParam.GetSize() ; n++ ) {
						if ( m_mmlParam[n] != MML_NOVALUE ) {
							int w= 8192 + (short)m_mmlParam[n];
							if ( w < 0 ) w = 0;
							else if ( w > 0x3FFF ) w = 0x3FFF;
							pData = new CMMLData(m_mmlData);
							pData->m_Cmd = 0xE0;
							pData->m_Note = (BYTE)(w % 128);
							pData->m_Velo = (BYTE)(w / 128);
							pData->m_Time = offs + step * n / (int)m_mmlParam.GetSize();
							pData->m_Step = 0;
							pData->m_bBase = TRUE;
							m_mmlData.Add(pData);
						}
					}

					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xE0;
					pData->m_Note = 0x00;
					pData->m_Velo = 0x40;
					pData->m_Time = 0;
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				}
			}
			break;

		case 'M':
			ch = ParseParam(ParseChar());
			step = (pLast != NULL ? pLast->m_Step : m_mmlData.m_Step);
			offs = (pLast != NULL ? pLast->m_Time : m_mmlData.m_Time);

			for ( int n = 0 ; n < m_mmlParam.GetSize() ; n++ ) {
				if ( m_mmlParam[n] != MML_NOVALUE ) {
					int w = (short)m_mmlParam[n];
					if ( w < 0 ) w = 0; else if ( w > 127 ) w = 127;
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 1;
					pData->m_Velo = w;
					pData->m_Time = offs + step * n / (int)m_mmlParam.GetSize();
					pData->m_Step = 0;
					pData->m_bBase = TRUE;
					m_mmlData.Add(pData);
				}
			}

			pData = new CMMLData(m_mmlData);
			pData->m_Cmd = 0xB0;
			pData->m_Note = 1;
			pData->m_Velo = 0;
			pData->m_Time = 0;
			pData->m_Step = 0;
			m_mmlData.Add(pData);
			break;

		case 'W':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE && m_mmlValue >= -64 && m_mmlValue <= 63 ) {
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0xB0;
				pData->m_Note = 10;
				pData->m_Velo = 64 + m_mmlValue;
				pData->m_Step = 0;
				m_mmlData.Add(pData);
			}
			break;

		case 'U':
			if ( (ch = ParseChar()) == '<' || ch == '>' ) {
				BOOL vect = (ch == '<' ? TRUE : FALSE);
				step = (pLast != NULL ? pLast->m_Step : m_mmlData.m_Step);
				offs = (pLast != NULL ? pLast->m_Time : m_mmlData.m_Time);

				ch = ParseAddSub(ParseChar());
				if ( m_mmlValue != MML_NOVALUE && m_mmlValue > 0 )
					step = step / m_mmlValue;

				step = 6 * ((step + 3) / 6);
				for ( int n = 0 ; n <= step ; n += 6 ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 11;
					pData->m_Velo = (vect ? (127 - 127 * n / step) : (127 * n / step));
					pData->m_Time = offs + n;
					pData->m_Step = 0;
					pData->m_bBase = TRUE;
					m_mmlData.Add(pData);
				}
				if ( vect ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 11;
					pData->m_Velo = 127;
					pData->m_Time = 0;
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				}
			} else {
				ch = ParseParam(ch);
				if ( m_mmlParam.GetSize() == 1 && m_mmlParam[0] != MML_NOVALUE ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 11;
					pData->m_Velo = m_mmlValue;
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				} else {
					step = (pLast != NULL ? pLast->m_Step : m_mmlData.m_Step);
					offs = (pLast != NULL ? pLast->m_Time : m_mmlData.m_Time);

					for ( int n = 0 ; n < m_mmlParam.GetSize() ; n++ ) {
						if ( m_mmlParam[n] != MML_NOVALUE ) {
							pData = new CMMLData(m_mmlData);
							pData->m_Cmd = 0xB0;
							pData->m_Note = 11;
							pData->m_Velo = m_mmlParam[n];
							pData->m_Time = offs + step * n / (int)m_mmlParam.GetSize();
							pData->m_Step = 0;
							pData->m_bBase = TRUE;
							m_mmlData.Add(pData);
						}
					}

					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0xB0;
					pData->m_Note = 11;
					pData->m_Velo = 127;
					pData->m_Time = 0;
					pData->m_Step = 0;
					m_mmlData.Add(pData);
				}
			}
			break;

		case ';':
			ch = ParseChar();
			if ( ++m_mmlData.m_Track > 15 )
				m_mmlData.m_Track = 15;
			break;

		case 'Z':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE && m_mmlValue >= 1 && m_mmlValue <= 16 )
				m_mmlData.m_Track = m_mmlValue - 1;
			if ( ch == ',' ) {
				ch = ParseAddSub(ParseChar());
				if ( m_mmlValue != MML_NOVALUE ) {
					pData = new CMMLData(m_mmlData);
					pData->m_Cmd = 0x60;
					pData->m_Step = (m_mmlValue != MML_NOVALUE ? (MML_STEPMAX / 4 * m_mmlValue) : 0);
					m_mmlData.Add(pData);
				}
			}
			break;

		case 'T':
			ch = ParseAddSub(ParseChar());
			if ( m_mmlValue != MML_NOVALUE && m_mmlValue > 0 ) {
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0x50;
				pData->m_Note = 60000000 / m_mmlValue;		// 4分音符の長さをマイクロ秒単位
				pData->m_Step = 0;
				pData->m_Track = 0;
				m_mmlData.Add(pData);
			}
			break;

		case 'X':
			ch = ParseChar();
			if ( ch == 'M' || ch == 'R' ) {
				ch = ParseChar();
				// GM Reset
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0xF0;
				pData->m_pData = new BYTE[5];
				memcpy(pData->m_pData, "\x7E\x7F\x09\x01\xF7", 5);
				pData->m_Note = 5;
				pData->m_Step = 0;
				pData->m_Track = 0;
				m_mmlData.Add(pData);
				memset(m_mmlPitch, 2, sizeof(m_mmlPitch));	// Reset !!

			} else if ( ch == 'S' ) {
				ch = ParseChar();
				// GS Reset
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0xF0;
				pData->m_pData = new BYTE[10];
				memcpy(pData->m_pData, m_mmlIds, 4);
				memcpy(pData->m_pData + 4, "\x40\x00\x7F\x00\x41\xF7", 6);
				pData->m_Note = 10;
				pData->m_Step = 0;
				pData->m_Track = 0;
				m_mmlData.Add(pData);
				memset(m_mmlPitch, 2, sizeof(m_mmlPitch));	// Reset !!

			} else if ( ch == 'V' ) {
				ch = ParseParam(ParseChar());
				// Master Volume
				pData = new CMMLData(m_mmlData);
				pData->m_Cmd = 0xF0;
				pData->m_pData = new BYTE[7];
				memcpy(pData->m_pData, "\x7F\x7F\x04\x01\x7F\x7F\xF7", 7);
				if ( m_mmlParam.GetSize() > 0 && m_mmlParam[0] != MML_NOVALUE )
					pData->m_pData[5] = (BYTE)m_mmlParam[0];
				pData->m_Note = 7;
				pData->m_Step = 0;
				pData->m_Track = 0;
				m_mmlData.Add(pData);
				memset(m_mmlPitch, 2, sizeof(m_mmlPitch));	// Reset !!

			} else if ( ch == 'Z' ) {
				ch = ParseParam(ParseChar());
				// Midi MakerID DeviceID...
				for ( int n = 0 ; n < 4 && n < m_mmlParam.GetSize() ; n++ ) {
					if ( m_mmlParam[n] != MML_NOVALUE )
						m_mmlIds[n] = (BYTE)m_mmlParam[n];
				}

			} else if ( ch == 'X' ) {
				ch = ParseParam(ParseChar());
				//     +メーカーＩＤ
				//     |  +デバイスＩＤ
				//     |  |  +モデルＩＤ
				//     |  |  |  +コマンドＩＤ
				//     |  |  |  |    +アドレス
				//     |  |  |  |    |          +データ
				//     |  |  |  |    |          |   +SUM = 128 - [Addr + Data] % 128
				// F0 [41 10 42 12] [40 00 7F] [00] 41 F7	// GS Mode Set
				int len = (int)m_mmlParam.GetSize();
				int sum = 0;
				pData = new CMMLData(m_mmlData);
				pData->m_pData = new BYTE[4 + len + 2];
				memcpy(pData->m_pData, m_mmlIds, 4);
				ZeroMemory(pData->m_pData + 4, len);
				for ( int n = 0 ; n < len ; n++ ) {
					if ( m_mmlParam[n] != MML_NOVALUE )
						pData->m_pData[4 + n] = (BYTE)m_mmlParam[n];
					sum += pData->m_pData[4 + n];
				}
				pData->m_pData[4 + len] = (BYTE)(128 - (sum % 128));
				pData->m_pData[4 + len + 1] = '\xF7';
				pData->m_Cmd = 0xF0;
				pData->m_Note = 4 + len + 2;
				pData->m_Step = 0;
				pData->m_Track = 0;
				m_mmlData.Add(pData);

			} else {
				ch = ParseParam(ch);
				// システムエクスクルーシブ
				if ( m_mmlParam.GetSize() >= 2 && m_mmlParam[0] == 0xF0 && m_mmlParam[m_mmlParam.GetSize() - 1] == 0xF7 ) {
					pData = new CMMLData(m_mmlData);
					pData->m_pData = new BYTE[m_mmlParam.GetSize() - 1];
					for ( int n = 1 ; n < m_mmlParam.GetSize() ; n++ )
						pData->m_pData[n - 1] = (m_mmlParam[n] != MML_NOVALUE ? (m_mmlParam[n] & 0x7F) : 0);
					pData->m_Cmd = 0xF0;
					pData->m_Note = (int)m_mmlParam.GetSize() - 1;
					pData->m_Step = 0;
					pData->m_Track = 0;
					m_mmlData.Add(pData);
				}
			}
			break;

		case '[':
			pData = NULL;
			save = m_mmlData;

			m_mmlData.m_Octa += nOct; nOct = 0;
			m_mmlData.m_Velo += nVol; nVol = 0;

			if ( (ch = ParseNote(ParseChar())) == ']' ) {
				int ofs = 0;
				for ( ; ; ) {
					ch = ParseChar();
					if ( ch == '+' || ch == '#' ) {
						ofs++;
					} else if ( ch == '-' ) {
						ofs--;
					} else
						break;
				}
				ch = ParseStep(ch);
				pData = new CMMLData;
				*pData = m_mmlData;

				pData->m_Cmd = 0x10;
				pData->m_Step = m_mmlValue;
				pData->m_Note = ofs;
			}

			m_mmlData = save;

			if ( pData != NULL ) {
				pLast = JointData(pLast, pData, nTai);
				nTai = 0;
			}
			break;

		case '{':
			pData = NULL;
			save = m_mmlData;

			m_mmlData.m_Octa += nOct; nOct = 0;
			m_mmlData.m_Velo += nVol; nVol = 0;

			if ( (ch = ParseNote(ParseChar())) == '}' ) {
				ch = ParseStep(ParseChar());
				pData = new CMMLData;
				*pData = m_mmlData;

				pData->m_Cmd = 0x20;
				pData->m_Step = m_mmlValue;
			}

			m_mmlData = save;

			if ( pData != NULL ) {
				pLast = JointData(pLast, pData, nTai);
				nTai = 0;
			}
			break;

		case '=':
			ch = ParseAddSub(ParseChar());
			pData = new CMMLData(m_mmlData);
			pData->m_Cmd = 0x40;
			pData->m_Note = MML_STEPMAX / 4 * (m_mmlValue == MML_NOVALUE || m_mmlValue <= 0 ? 4 : m_mmlValue);
			pData->m_Step = 0;
			m_mmlData.Add(pData);
			break;

		case '\'':
			mbs.Empty();
			while ( *m_mmlStr != '\0' && *m_mmlStr != '\'' )
				mbs += *(m_mmlStr++);
			if ( *m_mmlStr == '\'' )
				m_mmlStr++;
			ch = ParseChar();

			pData = new CMMLData(m_mmlData);
			pData->m_Cmd = 0x0100;
			pData->m_Note = mbs.GetLength() + 1;
			pData->m_pData = new BYTE[pData->m_Note];
			memcpy(pData->m_pData, (LPCSTR)mbs, pData->m_Note);
			pData->m_Step = 0;
			m_mmlData.Add(pData);
			break;

		default:
			return ch;
		}
	}
}
CMMLData *CMidiData::JointData(CMMLData *pLast, CMMLData *pData, int nTai)
{
	if ( nTai == 0 || pLast == NULL || pLast->m_Track != pData->m_Track )
		return m_mmlData.Add(pData);

	if ( pLast->m_Cmd == 0x80 && pData->m_Cmd == 0x80 ) {
		if ( pLast->m_Note == pData->m_Note ) {
			pLast->m_Step += pData->m_Step;
			delete pData;
			return pLast;
		} else if ( nTai == 1 ) {
			pLast->m_Gate = 100;
			pLast = m_mmlData.Add(pData);
			return pLast;
		}
	}

	CMMLData *pTemp = new CMMLData;
	*pTemp = *pLast;

	pLast->m_Cmd = 0x30;
	pLast->m_Note = nTai;
	pLast->m_Step = pLast->m_Step + pData->m_Step;
	pLast->m_pLink = pTemp;
	pLast->m_pWork = pData;

	return pLast;
}
BOOL CMidiData::UpdateJoint(CMMLData *pLast, CMMLData *pNext, int nTai)
{
	CMMLData *pTemp;

	if ( pLast == NULL || pNext == NULL )
		return FALSE;

	if ( pLast->m_Cmd == 0x10 ) {			// 和音
		for ( pTemp = pLast->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext )
			UpdateJoint(pTemp, pNext, nTai);
		return FALSE;
	} else if ( pLast->m_Cmd == 0x00 || pLast->m_Cmd == 0x20 ) {	// 連符
		UpdateJoint(pLast->m_pLast, pNext, nTai);
		return FALSE;
	} else if ( pLast->m_Cmd ==  0x30 ) {	// スラー
		UpdateJoint(pLast->m_pLink, pNext, pLast->m_Note);
		UpdateJoint(pLast->m_pWork, pNext, pLast->m_Note);
		return FALSE;
	} else if ( pLast->m_Cmd != 0x80 || !pLast->m_bTai )	// Note
		return FALSE;

	if ( pNext->m_Cmd == 0x10 ) {			// 和音
		for ( pTemp = pNext->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext ) {
			if ( UpdateJoint(pLast, pTemp, nTai) )
				return TRUE;
		}
		pLast->m_Gate = 100;
		pLast->m_bTai = FALSE;
		return FALSE;
	} else if ( pNext->m_Cmd == 0x00 || pNext->m_Cmd == 0x20 ) {	// 連符
		if ( UpdateJoint(pLast, pNext->m_pLink, nTai) )
			return TRUE;
		pLast->m_Gate = 100;
		pLast->m_bTai = FALSE;
		return FALSE;
	} else if ( pNext->m_Cmd != 0x80 )		// Note
		return FALSE;

	if ( pLast->m_Track != pNext->m_Track )
		return FALSE;

	if ( pLast->m_Note == pNext->m_Note ) {
		pLast->m_AddStep += pNext->m_Step;
		pNext->m_Cmd = 0x90;	// Note -> Nop
		return TRUE;
	} else if ( nTai == 2 ) {	// スラー
		if (  (pNext->m_Note - pLast->m_Note) > m_mmlPitch[pLast->m_Track] )
			return FALSE;

		pLast->m_AddStep += pNext->m_Step;
		int w= 8192 + (pNext->m_Note - pLast->m_Note) * 8192 / m_mmlPitch[pLast->m_Track];
		if ( w < 0 ) w = 0;
		else if ( w > 0x3FFF ) w = 0x3FFF;

		CMMLData *pTemp[3];
		pTemp[0] = new CMMLData(*pLast);
		pTemp[1] = new CMMLData(*pLast);
		pTemp[2] = new CMMLData(*pLast);

		*pTemp[0] = *pNext;
		pTemp[0]->m_Cmd = 0x90;	// Note -> Nop

		pNext->m_Cmd = 0x00;	// Note -> List
		pNext->Add(pTemp[0]);
		pNext->Add(pTemp[1]);
		pNext->Add(pTemp[2]);
		pNext->m_Step = pLast->m_Step + pLast->m_AddStep;

		pTemp[1]->m_Cmd = 0xE0;
		pTemp[1]->m_Note = (BYTE)(w % 128);
		pTemp[1]->m_Velo = (BYTE)(w / 128);
		pTemp[1]->m_Time = 0;
		pTemp[1]->m_Step = 0;
		pTemp[1]->m_bBase = TRUE;

		pTemp[2]->m_Cmd = 0xE0;
		pTemp[2]->m_Note = 0x00;
		pTemp[2]->m_Velo = 0x40;
		pTemp[2]->m_Time = 0;
		pTemp[2]->m_Step = 0;

		return TRUE;
	}

	return FALSE;
}
void CMidiData::UpdateStep(CMMLData *pData)
{
	int total;
	CMMLData *pTemp;

	switch(pData->m_Cmd) {
	case 0x00:	// List
		pData->m_Step = 0;
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext ) {
			UpdateStep(pTemp);
			pData->m_Step += pTemp->m_Step;
		}
		break;
	case 0x10:	// 和音
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext ) {
			if ( pTemp->m_Step > 0 )
				pTemp->m_Step = pData->m_Step;
			UpdateStep(pTemp);
		}
		break;
	case 0x20:	// 連符
		total = 0;
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext )
			total += pTemp->m_Step;
		if ( total > 0 ) {
			for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext ) {
				if ( pTemp->m_Step > 0 )
					pTemp->m_Step = pTemp->m_Step * pData->m_Step / total;
				UpdateStep(pTemp);
			}
		}
		break;

	case 0x30:	// スラー
		total = pData->m_pLink->m_Step + pData->m_pWork->m_Step;
		pData->m_pLink->m_Step = pData->m_pLink->m_Step * pData->m_Step / total;
		pData->m_pWork->m_Step = pData->m_pWork->m_Step * pData->m_Step / total;
		UpdateStep(pData->m_pLink);
		UpdateStep(pData->m_pWork);
		UpdateJoint(pData->m_pLink, pData->m_pWork, pData->m_Note);
		break;
	}
}
void CMidiData::UpdateData(CMMLData *pData)
{
	int step[16];
	CNode *pNode, *pNext;
	CNode *pOnNode, *pOffNode;
	DWORD OnPos, OffPos;
	CMMLData *pTemp;

	switch(pData->m_Cmd) {
	case 0x00:	// List
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext )
			UpdateData(pTemp);
		break;
	case 0x10:	// 和音
		memcpy(step, m_mmlPos, sizeof(m_mmlPos));
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext ) {
			UpdateData(pTemp);
			memcpy(m_mmlPos, step, sizeof(m_mmlPos));
		}
		m_mmlPos[pData->m_Track] += pData->m_Step;
		break;
	case 0x20:	// 連符
		memcpy(step, m_mmlPos, sizeof(m_mmlPos));
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext )
			UpdateData(pTemp);
		memcpy(m_mmlPos, step, sizeof(m_mmlPos));
		m_mmlPos[pData->m_Track] += pData->m_Step;
		break;
	case 0x30:	// スラー
		UpdateData(pData->m_pLink);
		UpdateData(pData->m_pWork);
		break;

	case 0x40:	// 小節チェック
		//if ( (m_mmlPos[pData->m_Track] % pData->m_Note) != 0 ) {
		//	CString str;
		//	str.Format(_T("#%d %d ? %d\r\n"), pData->m_Track + 1, 
		//		m_mmlPos[pData->m_Track] / pData->m_Note + 1, m_mmlPos[pData->m_Track] % pData->m_Note);
		//	::AfxMessageBox(str);
		//}
		for ( int ch = 0 ; ch < 16 ; ch++ ) {
			if ( (m_mmlPos[ch] % pData->m_Note) != 0 ) {
				CString str;
				str.Format(_T("#%d %d ? %d\r\n"), ch + 1, 
					m_mmlPos[ch] / pData->m_Note + 1, m_mmlPos[ch] % pData->m_Note);
				::AfxMessageBox(str);
			}
		}
		m_SeekPos = m_mmlPos[pData->m_Track];
		break;

	case 0x50:	// Tempo
		AddNode(m_mmlPos[0] + pData->m_Time, 0, 0xFF51, pData->m_Note);
		break;
	case 0x60:	// ステップ位置リセット
		m_mmlPos[pData->m_Track] = pData->m_Step;
		break;
	case 0x70:	// 休符
		m_mmlPos[pData->m_Track] += pData->m_Step;
		break;

	case 0x80:	// Note
		m_mmlBase[pData->m_Track] = m_mmlPos[pData->m_Track];
		OnPos  = m_mmlPos[pData->m_Track] + pData->m_Time;
		OffPos = m_mmlPos[pData->m_Track] + pData->m_Time + (pData->m_Step + pData->m_AddStep) * pData->m_Gate / 100;
		m_mmlPos[pData->m_Track] += pData->m_Step;
		pOnNode = AddNode(OnPos, pData->m_Track, 0x90 | pData->m_Track, pData->m_Note, pData->m_Velo);
		pOffNode = AddNode(OffPos, pData->m_Track, 0x80 | pData->m_Track, pData->m_Note, 0);
		for ( pNode = pOnNode->m_Next ; pNode != NULL && pNode != pOffNode ; pNode = pNext ) {
			pNext = pNode->m_Next;
			if ( pNode->m_Pos >= pOffNode->m_Pos )
				break;
			// 同じノートオフを削除
			if ( pNode->m_Cmd == pOffNode->m_Cmd && pNode->m_Pam.m_DWord == pOffNode->m_Pam.m_DWord ) {
				Unlink(pNode);
				FreeNode(pNode);
			}
		}
		break;

	case 0x90:	// Nop
		m_mmlBase[pData->m_Track] = m_mmlPos[pData->m_Track];
		m_mmlPos[pData->m_Track] += pData->m_Step;
		break;

	case 0xA0:
	case 0xB0:
	case 0xC0:
	case 0xD0:
		AddNode((pData->m_bBase ? m_mmlBase[pData->m_Track] : m_mmlPos[pData->m_Track])	+ pData->m_Time, 
				pData->m_Track, pData->m_Cmd | pData->m_Track, pData->m_Note, pData->m_Velo);
		break;
	case 0xE0:
		pOnNode = AddNode((pData->m_bBase ? m_mmlBase[pData->m_Track] : m_mmlPos[pData->m_Track])	+ pData->m_Time, 
					pData->m_Track, pData->m_Cmd | pData->m_Track, pData->m_Note, pData->m_Velo);
		for ( pNode = pOnNode->m_Back ; pNode != NULL ; pNode = pNext ) {
			pNext = pNode->m_Back;
			// 同じ位置のピッチベンドを削除
			if ( pOnNode->m_Pos != pNode->m_Pos )
				break;
			if ( pOnNode->m_Cmd == pNode->m_Cmd ) {
				Unlink(pNode);
				FreeNode(pNode);
			}
		}
		break;

	case 0xF0:
		AddNode(m_mmlPos[0] + pData->m_Time, 0, pData->m_Cmd, pData->m_Note, pData->m_pData);
		break;
	case 0x100:
		AddNode(m_mmlPos[0] + pData->m_Time, 0, 0xFF80, pData->m_Note, pData->m_pData);
		break;
	}
}

BOOL CMidiData::LoadMML(LPCSTR str, int nInit)
{
	// nInit	0	Reset Param Data
	//			1	Reset Param
	//			2	Not Reset
	//			3	Base64 SMF

	if ( nInit == 3 ) {
		CBuffer tmp;
		Stop();
		tmp.Base64Param(str);
		return SetSmfData(tmp.GetPtr(), tmp.GetSize());

	} else if ( nInit == 4 ) {
		CFileDialog dlg(FALSE, _T("mid"), _T("*.mid"), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), ::AfxGetMainWnd());
		if ( dlg.DoModal() != IDOK )
			return FALSE;
		return SaveFile(dlg.GetPathName());

	} else if ( nInit == 5 ) {
		CFileDialog dlg(FALSE, _T("mml"), _T("*.mml"), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), ::AfxGetMainWnd());
		if ( dlg.DoModal() != IDOK )
			return FALSE;
		return SaveMML(dlg.GetPathName());

	} else if ( nInit == 0 || nInit == 1 ) {
		Stop();

		if ( nInit == 0 )
			RemoveAll();

		m_PlayMode = 0;
		m_pPlayNode = m_pDispNode = NULL;
		m_PlayPos = m_SeekPos = 0;
		m_DivVal = 96;
		m_pStep = NULL;
		m_LastClock = 0;

		m_mmlData.m_Cmd   = 0x00;
		m_mmlData.m_Track = 0;
		m_mmlData.m_Note  = 60;		// N, CDEFGABC
		m_mmlData.m_Step  = 96;		// L
		m_mmlData.m_Gate  = 80;		// Q
		m_mmlData.m_Velo  = 100;	// V
		m_mmlData.m_Time  = 0;		// T
		m_mmlData.m_Octa  = 5;		// O < >
		m_mmlData.m_Keys  = 0;		// K
		m_mmlData.RemoveAll();

		ZeroMemory(m_mmlPos, sizeof(m_mmlPos));
		ZeroMemory(m_mmlBase, sizeof(m_mmlBase));
		// memset(m_mmlPitch, 2, sizeof(m_mmlPitch));		// MIDIの初期化に注意
		memcpy(m_mmlIds, "\x41\x10\x42\x12", 4);			// MIDI DeviceID...

		if ( m_hStream != NULL ) {
			MIDIPROPTIMEDIV proptime;
			proptime.cbStruct  = sizeof(MIDIPROPTIMEDIV);
			proptime.dwTimeDiv = m_DivVal;
			midiStreamProperty(m_hStream, (BYTE*)&proptime, MIDIPROP_SET | MIDIPROP_TIMEDIV);

			MIDIPROPTEMPO tempo;
			tempo.cbStruct = sizeof(MIDIPROPTEMPO);
			tempo.dwTempo  = 60000000 / 120;
			midiStreamProperty(m_hStream, (BYTE*)&tempo, MIDIPROP_SET | MIDIPROP_TEMPO);
		}
	}

	m_mmlStr = str;
	ParseNote(ParseChar());

	UpdateStep(&m_mmlData);
	UpdateData(&m_mmlData);
	m_mmlData.RemoveAll();

	return TRUE;
}

void CMidiData::SaveSingStep(int step, CStringA &mbs)
{
	int n, i;
	int max = (int)m_DivVal * 4;

	for ( n = 1 ; n <= 64 ; n *= 2 ) {
		i = max / n;
		if ( i == step ) {
			mbs.Format("%d", n);
			return;
		}
		i += i / 2;
		if ( i == step ) {
			mbs.Format("%d.", n);
			return;
		}
	}

	if ( (n = max / step) > 0 && step == (max / n) ) {
		mbs.Format("%d", n);
		return;
	}

	mbs.Format("%%%d", step * 96 / m_DivVal);
}
void CMidiData::SaveNoteStep(LPCSTR cmd, int step, CStringA &mbs)
{
	int n;
	int div = 1;
	int max = (int)m_DivVal * 4;
	CStringA fmt;

	while ( step > 0 ) {
		while ( div <= 64 && step < (max / div) )
			div *= 2;

		if ( div > 64 ) {
			if ( (div = max / step) <= 0 )
				div = 1;
			if ( step < (max / div) )
				div *= 2;
		}

		fmt.Format("%s%d", cmd, div);
		mbs += fmt;

		n = max / div;
		if ( step == (n + n / 2) ) {
			mbs += '.';
			break;
		}

		if ( (step -= n) > 0 )
			mbs += (*cmd == 'r' ? ' ' : '&');
	}
}
void CMidiData::SaveNote(CFile *pFile, CMMLData *pData)
{
	int n, step, gate;
	CMMLData *pTemp, save;
	CStringA mbs, fmt;

	switch(pData->m_Cmd) {
	case 0x00:	// List
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext )
			SaveNote(pFile, pTemp);
		break;
	case 0x10:	// 和音
		save.m_Gate = m_mmlData.m_Gate;
		save.m_Velo = m_mmlData.m_Velo;
		save.m_Time = m_mmlData.m_Time;
		save.m_Octa = m_mmlData.m_Octa;
		pFile->Write("[", 1);
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext )
			SaveNote(pFile, pTemp);
		SaveSingStep(pData->m_Step, fmt);
		mbs.Format("]%s ", fmt);
		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		m_mmlData.m_Gate = save.m_Gate;
		m_mmlData.m_Velo = save.m_Velo;
		m_mmlData.m_Time = save.m_Time;
		m_mmlData.m_Octa = save.m_Octa;
		break;
	case 0x20:	// 連符
		for ( pTemp = pData->m_pLink ; pTemp != NULL ; pTemp = pTemp->m_pNext )
			SaveNote(pFile, pTemp);
		break;

	case 0x30:	// スラー
	case 0x40:	// 小節チェック
		break;

	case 0x50:	// Tempo
		fmt.Format("t%d ", 60000000 / pData->m_Note);
		pFile->Write((LPCSTR)fmt, fmt.GetLength());
		break;

	case 0x60:	// ステップ位置リセット
		fmt.Format("\r\nz%d,%d ", pData->m_Track + 1, pData->m_Step / m_DivVal);
		mbs = fmt;

		if ( (n = pData->m_Step % m_DivVal) > 0 ) {
			SaveNoteStep("r", n, mbs);
			mbs += ' ';
		}

		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		break;

	case 0x70:	// 休符
		SaveNoteStep("r", pData->m_Step, mbs);
		mbs += ' ';
		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		break;

	case 0x80:	// Note
		step = pData->m_Step;
		gate = pData->m_Gate * 100 / step;

		if ( m_mmlData.m_Gate != gate ) {
			m_mmlData.m_Gate = gate;
			fmt.Format("q%d", gate);
			mbs += fmt;
		}
		if ( m_mmlData.m_Velo != pData->m_Velo ) {
			m_mmlData.m_Velo = pData->m_Velo;
			fmt.Format("v%d", pData->m_Velo);
			mbs += fmt;
		}
		n = pData->m_Time * (MML_STEPMAX / 4) / m_DivVal;
		if ( m_mmlData.m_Time != n ) {
			m_mmlData.m_Time = n;
			fmt.Format("s%d", n);
			mbs += fmt;
		}

		n = pData->m_Note / 12;
		for ( ; n > m_mmlData.m_Octa ; m_mmlData.m_Octa++ )
			mbs += '>';
		for ( ; n < m_mmlData.m_Octa ; m_mmlData.m_Octa-- )
			mbs += '<';

		if ( pData->m_Track == 9 )
			fmt.Format("n%d,", pData->m_Note);
		else
			fmt.Format("%s", NoteName[pData->m_Note % 12]);

		SaveNoteStep(fmt, step, mbs);
		mbs += ' ';

		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		break;

	case 0x90:	// Nop
		break;

	case 0xA0:	// Polyphonic Key Pressure
		break;
	case 0xB0:	// Control Change
		n = pData->m_Time * (MML_STEPMAX / 4) / m_DivVal;
		if ( m_mmlData.m_Time != n ) {
			m_mmlData.m_Time = n;
			fmt.Format("s%d", n);
			mbs += fmt;
		}
		fmt.Format("y%d,%d ", pData->m_Note, pData->m_Velo);
		mbs += fmt;
		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		break;
	case 0xC0:	// Program Change
		n = pData->m_Time * (MML_STEPMAX / 4) / m_DivVal;
		if ( m_mmlData.m_Time != n ) {
			m_mmlData.m_Time = n;
			fmt.Format("s%d", n);
			mbs += fmt;
		}
		fmt.Format("@%d ", pData->m_Note + 1);
		mbs += fmt;
		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		break;
	case 0xD0:	// Channel Pressure
		break;

	case 0xE0:	// Pitch Bend
		n = pData->m_Time * (MML_STEPMAX / 4) / m_DivVal;
		if ( m_mmlData.m_Time != n ) {
			m_mmlData.m_Time = n;
			fmt.Format("s%d", n);
			mbs += fmt;
		}
		n = (pData->m_Velo * 128 + pData->m_Note) - 8192;
		fmt.Format("p%d ", n);
		mbs += fmt;
		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		break;

	case 0xF0:
		n = pData->m_Time * (MML_STEPMAX / 4) / m_DivVal;
		if ( m_mmlData.m_Time != n ) {
			m_mmlData.m_Time = n;
			fmt.Format("s%d", n);
			mbs += fmt;
		}
		mbs = "x$f0";
		for ( n = 0 ; n < pData->m_Note ; n++ ) {
			fmt.Format(",$%02x", pData->m_pData[n]);
			mbs += fmt;
		}
		mbs += ' ';
		pFile->Write((LPCSTR)mbs, mbs.GetLength());
		break;
	}
}
int CMidiData::SaveStepPos(CNode *pNode, int stepPos, int ch)
{
	int n;
	int div;
	CMMLData *pData;

	div = m_DivVal / 8;		// 4分音符 / 8 = 32分音符
	n = (pNode->m_Pos / div) * div;

	if ( m_NowTrack != ch || stepPos > n || (n - stepPos) > ((int)m_DivVal * 4 * 2) ) {
		pData = new CMMLData;
		pData->m_Cmd = 0x60;
		pData->m_Step = n;
		pData->m_Track = ch;
		m_mmlData.Add(pData);
		m_NowTrack = ch;

	} else if ( stepPos < n ) {
		pData = new CMMLData;
		pData->m_Cmd = 0x70;
		pData->m_Step = n - stepPos;
		pData->m_Track = ch;
		m_mmlData.Add(pData);
	}

	return n;
}
BOOL CMidiData::GetNoteData(CNode *pNode, int &note, int &velo, int &step, int &gate)
{
	note = pNode->m_Pam.m_Byte[1];
	velo = pNode->m_Pam.m_Byte[2];

	int ch = pNode->m_Pam.m_Byte[0] & 0x0F;
	CNode *pNext = pNode->m_Next;

	// ノートオフを探す
	while ( pNext != NULL ) {
		if ( (pNext->m_Pam.m_Byte[0] == (0x90 | ch) && pNext->m_Pam.m_Byte[1] == note) ||
			 (pNext->m_Pam.m_Byte[0] == (0x80 | ch) && pNext->m_Pam.m_Byte[1] == note) )
			break;
		pNext = pNext->m_Next;
	}

	if ( pNext == NULL )
		return FALSE;

	step = gate = pNext->m_Pos - pNode->m_Pos;

	// 次のノートオンを探す
	if ( pNext->m_Pam.m_Byte[0] == (0x80 | ch) ) {
		pNext = pNext->m_Next;
		while ( pNext != NULL ) {
			if ( pNext->m_Pam.m_Byte[0] == (0x90 | ch) )
				break;
			pNext = pNext->m_Next;
		}
		if ( pNext != NULL )
			step = pNext->m_Pos - pNode->m_Pos;
	}

	while ( step > 0 && (gate * 100 / step) < 50 )
		step /= 2;

	return (step <= 0 || gate <= 0 ? FALSE : TRUE);
}
BOOL CMidiData::SaveMML(LPCTSTR fileName)
{
	int ch, note, velo, step, gate;
	CNode *pNode;
	DWORD stepPos;
	CMMLData *pData, *pTemp, *pWork;
	CFile file;

	if ( !file.Open(fileName, CFile::modeCreate | CFile::modeWrite) )
		return FALSE;

	m_mmlData.m_Cmd   = 0x00;
	m_mmlData.m_Track = 0;
	m_mmlData.m_Note  = 60;		// N, CDEFGABC
	m_mmlData.m_Step  = 96;		// L
	m_mmlData.m_Gate  = 80;		// Q
	m_mmlData.m_Velo  = 100;	// V
	m_mmlData.m_Time  = 0;		// T
	m_mmlData.m_Octa  = 5;		// O < >
	m_mmlData.m_Keys  = 0;		// K
	m_mmlData.RemoveAll();

	ZeroMemory(m_mmlPos, sizeof(m_mmlPos));

	m_NowTrack = (-1);

	// ノードのチェックフラグクリア
	pNode = TopNode();
	while ( pNode != NULL ) {
		pNode->m_Flag = FALSE;
		pNode->m_SubT = 0;
		pNode = NextNode();
	}

	for ( ch = 0 ; ch < 16 ; ch++ ) {
		if ( (m_ActiveCh & (1 << ch)) == 0 )
			continue;

		stepPos = 0;
		for ( pNode = TopNode() ; pNode != NULL ; pNode = NextNode() ) {
			if ( pNode->m_Flag )
				continue;

			if ( pNode->m_Cmd == (0x90 | ch) ) {
				if ( GetNoteData(pNode, note, velo, step, gate) ) {
					pNode->m_Flag = TRUE;
					stepPos = SaveStepPos(pNode, stepPos, ch);

					pData = new CMMLData;
					pData->m_Cmd = 0x80;
					pData->m_Note = note;
					pData->m_Velo = velo;
					pData->m_Step = step;
					pData->m_Time = pNode->m_Pos - stepPos;
					pData->m_Gate = gate;
					pData->m_Track = ch;

					//TRACE("c=%d n=%d v=%d g=%d s=%d\n", ch, note, velo, gate, step);

					// 和音を探す
					for ( CNode *pNext = pNode->m_Next ; pNext != NULL ; pNext = pNext->m_Next ) {
						if ( pNext->m_Pos >= (pNode->m_Pos + pData->m_Step) )
							break;

						if ( pNext->m_SubT != pNode->m_SubT )
							continue;

						if ( pNext->m_Pam.m_Byte[0] == (0x90 | ch) ) {
							if ( pNext->m_Pos == pNode->m_Pos && GetNoteData(pNext, note, velo, step, gate) && pData->m_Gate == gate ) {
								pNext->m_Flag = TRUE;

								pTemp = new CMMLData;
								pTemp->m_Cmd = 0x80;
								pTemp->m_Note = note;
								pTemp->m_Velo = velo;
								pTemp->m_Step = pData->m_Step;
								pTemp->m_Gate = gate;
								pTemp->m_Time = pNext->m_Pos - stepPos;
								pTemp->m_Track = ch;

								//TRACE("  c=%d n=%d v=%d g=%d s=%d\n", ch, note, velo, gate, step);

								if ( pData->m_Cmd != 0x10 ) {	// 和音
									pWork = new CMMLData;
									*pWork = *pData;
									pData->m_Cmd = 0x10;
									pData->Add(pWork);
								}
								pData->Add(pTemp);

							} else
								pNode->m_SubT++;
						}
					}

					m_mmlData.Add(pData);
					stepPos += pData->m_Step;
				}

			} else if ( pNode->m_Cmd == 0xFF51 ) {
				pNode->m_Flag = TRUE;
				stepPos = SaveStepPos(pNode, stepPos, ch);

				pData = new CMMLData;
				pData->m_Cmd = 0x50;
				pData->m_Note = pNode->m_Pam.m_DWord;
				pData->m_Step = 0;
				pData->m_Time = pNode->m_Pos - stepPos;
				m_mmlData.Add(pData);

			} else if ( pNode->m_Cmd == 0xF0 ) {
				pNode->m_Flag = TRUE;
				stepPos = SaveStepPos(pNode, stepPos, ch);

				pData = new CMMLData;
				pData->m_Cmd = 0xF0;
				pData->m_Note = pNode->m_Len;
				pData->m_pData = new BYTE[pNode->m_Len];
				memcpy(pData->m_pData, pNode->m_Data, pNode->m_Len);
				pData->m_Step = 0;
				pData->m_Time = pNode->m_Pos - stepPos;
				m_mmlData.Add(pData);

			} else if ( (pNode->m_Cmd & 0xFF0F) == ch && (pNode->m_Cmd & 0xF0) != 0xF0 && (pNode->m_Cmd & 0xF0) != 0x80 ) {
				pNode->m_Flag = TRUE;
				stepPos = SaveStepPos(pNode, stepPos, ch);

				pData = new CMMLData;
				pData->m_Cmd = pNode->m_Cmd & 0xF0;
				pData->m_Note = pNode->m_Pam.m_Byte[1];
				pData->m_Velo = pNode->m_Pam.m_Byte[2];
				pData->m_Step = 0;
				pData->m_Time = pNode->m_Pos - stepPos;
				pData->m_Track = ch;
				m_mmlData.Add(pData);
			}
		}

		SaveNote(&file, &m_mmlData);
		m_mmlData.RemoveAll();
	}

	file.Close();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////

DWORD WINAPI CMidiData::ThreadProc(void *lpParameter)
{
	MSG msg;
	MIDIHDR *pMidiHdr;
	MIDIEVENT *pEvent;
	CMidiData *pMidi = (CMidiData *)lpParameter;

	while( GetMessage(&msg, NULL, 0, 0) > 0 ) {
		switch(msg.message) {
		case MM_MOM_POSITIONCB:
			pMidiHdr = (MIDIHDR*)msg.lParam;
			pEvent = (MIDIEVENT*)&(pMidiHdr->lpData[pMidiHdr->dwOffset]);
			if ( (pEvent->dwEvent & 0xFF000000) == ((MEVT_COMMENT << 24) | MEVT_F_CALLBACK) ) {
				switch(*((DWORD *)(pEvent->dwParms))) {
				case 0x0000:	// Next Stream
					pMidiHdr->dwFlags = 0;
					if ( !pMidi->StreamQueIn() )
						pMidi->Stop();
					break;
				case 0x0001:	// Speek
					((CRLoginApp *)::AfxGetApp())->Speek(MbsToTstr((LPCSTR)((BYTE *)(pEvent->dwParms) + sizeof(DWORD))));
					break;
				}
			}
			break;
		}
	}

	return 0;
}

BOOL CMidiData::StreamQueIn()
{
	int rt = FALSE;
	int n, i, a;
	MIDIEVENT *pEvent;

	for ( n = 0 ; m_pPlayNode != NULL && n < MIDIHDR_MAX ;  n++ ) {
		if ( m_MidiHdr[n].dwFlags != 0 )
			continue;

		for ( i = 0 ; m_pPlayNode != NULL ; ) {
			if ( (a = m_pPlayNode->GetEventSize()) <= 0 ) {
				m_pPlayNode = m_pPlayNode->m_Next;
				continue;
			}

			if ( m_SeekPos > m_pPlayNode->m_Pos ) {
				if ( (m_pPlayNode->m_Cmd & 0xF0) == 0x80 || (m_pPlayNode->m_Cmd & 0xF0) == 0x90 || (m_pPlayNode->m_Cmd & 0xF0) == 0xE0 ) {
					m_PlayPos = m_pPlayNode->m_Pos;
					m_pPlayNode = m_pPlayNode->m_Next;
					continue;
				}
			}

			if ( (i + a + 24) > MIDIDATA_MAX )
				break;

			if ( m_SeekPos > m_pPlayNode->m_Pos )
				m_PlayPos = m_pPlayNode->m_Pos;

			pEvent = (MIDIEVENT *)(m_MidiHdr[n].lpData + i);
			i += m_pPlayNode->SetEvent(m_PlayPos, (BYTE *)(m_MidiHdr[n].lpData + i));

			m_PlayPos = m_pPlayNode->m_Pos;
			m_pPlayNode = m_pPlayNode->m_Next;
		}
		if ( i == 0 )
			break;

		pEvent = (MIDIEVENT *)(m_MidiHdr[n].lpData + i);
		pEvent->dwDeltaTime = (m_pPlayNode == NULL ? m_DivVal : 0);
		pEvent->dwStreamID = 0;
		pEvent->dwEvent = ((MEVT_COMMENT << 24) | MEVT_F_CALLBACK | (DWORD)sizeof(DWORD));
		*((DWORD *)(pEvent->dwParms)) = 0x0000;
		i += (sizeof(DWORD) * 4);

		m_MidiHdr[n].dwFlags = 0;
		m_MidiHdr[n].dwOffset = 0;
		memset(m_MidiHdr[n].dwReserved, 0, sizeof(m_MidiHdr[n].dwReserved));
		m_MidiHdr[n].lpNext = 0;
		m_MidiHdr[n].reserved = 0;
		m_MidiHdr[n].dwBytesRecorded = i;

		midiOutPrepareHeader((HMIDIOUT)m_hStream, &(m_MidiHdr[n]), sizeof(MIDIHDR));
		midiStreamOut(m_hStream, &(m_MidiHdr[n]), sizeof(MIDIHDR));

		rt = TRUE;
	}

	if ( rt || m_pPlayNode != NULL )
		return TRUE;

	for ( n = 0 ; n < MIDIHDR_MAX ;  n++ ) {
		if ( m_MidiHdr[n].dwFlags != 0 )
			break;
	}
	return (n < MIDIHDR_MAX ? TRUE : FALSE);
}

void CMidiData::Play()
{
	if ( m_hStream == NULL )
		return;

	switch(m_PlayMode) {
	case 0:		// Stop
		m_PlayPos = 0;
		m_pPlayNode = m_pDispNode = TopNode();
		if ( !StreamQueIn() )
			break;
	case 1:
		midiStreamRestart(m_hStream);
		m_PlayMode = 2;
		break;
	}
}

void CMidiData::Pause()
{
	if ( m_hStream == NULL )
		return;

	switch(m_PlayMode) {
	case 1:
		midiStreamRestart(m_hStream);
		m_PlayMode = 2;
		break;
	case 2:
		midiStreamPause(m_hStream);
		m_PlayMode = 1;
		break;
	}
}

void CMidiData::Stop()
{
	if ( m_hStream == NULL )
		return;

	m_PlayPos = m_SeekPos = 0;
	m_pPlayNode = m_pDispNode = NULL;
	m_PlayMode = 0;

	midiStreamStop(m_hStream);
	midiOutReset((HMIDIOUT)m_hStream);

	for ( int n = 0 ; n < MIDIHDR_MAX ; n++ ) {
		midiOutUnprepareHeader((HMIDIOUT)m_hStream, &(m_MidiHdr[n]), sizeof(MIDIHDR));
		m_MidiHdr[n].dwFlags = 0;
	}
}

void CMidiData::GetCurrentStep(DWORD pos, int &step, int &div)
{
	int n;
	int deno, mole;
	CNode *pNode = m_pStep;
	DWORD tick, ofs;

	step = div = 0;
	mole = m_DivVal * 4 / 4;
	deno = mole * 4;
	tick = pos;
	ofs = 0;
	while ( pNode != NULL ) {
		if ( pNode->m_Pos > pos )
			break;

		step += ((pNode->m_Pos - ofs) / deno);
		tick -= (pNode->m_Pos - ofs);
		ofs = pNode->m_Pos;

		for ( n = 0, mole = 1 ; n < pNode->m_Data[1] ; n++ )
			mole *= 2;
		mole = m_DivVal * 4 / mole;
		deno = mole * pNode->m_Data[0];

		pNode = pNode->m_Link;
	}
	step += (tick / deno);
	div  =  (tick % deno) / mole;
}

int CMidiData::GetCurrentTime()
{
	if ( m_hStream == NULL )
		return 0;

	MMTIME mmt;
	mmt.wType = TIME_MS;
	midiStreamPosition(m_hStream, &mmt, sizeof(MMTIME));
	return (mmt.u.ms / 1000);
}

DWORD CMidiData::GetCurrentSong()
{
	if ( m_hStream == NULL )
		return 0;

	MMTIME mmt;
	mmt.wType = TIME_MIDI;
	midiStreamPosition(m_hStream, &mmt, sizeof(MMTIME));
	return (mmt.u.midi.songptrpos);
}

DWORD CMidiData::GetCurrentPosition()
{
	if ( m_hStream == NULL )
		return 0;

	MMTIME mmt;
	mmt.wType = TIME_TICKS;
	mmt.u.ticks = 0;
	midiStreamPosition(m_hStream, &mmt, sizeof(MMTIME));
	return (mmt.u.ticks);
}

int CMidiData::GetCurrentTempo()
{
	if ( m_hStream == NULL )
		return 0;

	MIDIPROPTEMPO tempo;
	tempo.cbStruct = sizeof(MIDIPROPTEMPO);
	tempo.dwTempo  = 0;
	if ( midiStreamProperty(m_hStream, (BYTE*)&tempo, MIDIPROP_GET | MIDIPROP_TEMPO) != MMSYSERR_NOERROR )
		return 0;
	return (tempo.dwTempo == 0 ? 0 : (60000000 / tempo.dwTempo));
}
