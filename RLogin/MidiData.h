#pragma once

#include <mmsystem.h>

//////////////////////////////////////////////////////////////////////
// CMidiData クラスのインターフェイス

#define	MIDIHDR_MAX			8
#define	MIDIDATA_MAX		(32 * 1024)

#define	MML_STEPMAX			384
#define	MML_NOVALUE			0x7FFF

class CMemBuffer : public CObject
{
public:
	int m_Max;
	int m_Len;
	BYTE *m_pData;

	void SizeCheck(int len);
	void AddByte(int data);
	void AddWord(int data);
	void AddDWord(int data);
	void AddBuff(BYTE *pData, int len);
	void AddDelta(int data);
	void AddStr(LPCSTR str);

	int GetSize() { return m_Len; }
	BYTE *GetData() { return m_pData; };
	void RemoveAll();

	CMemBuffer();
	~CMemBuffer();
};

class CNode : public CObject
{
public:
	class CNode *m_Next;
	class CNode *m_Back;
	class CNode *m_Link;

	DWORD	m_Pos;
	int		m_Cmd;
	union	{
		BYTE	m_Byte[4];
		WORD	m_Word[2];
		DWORD	m_DWord;
	} m_Pam;
	int		m_Len;
	BYTE	*m_Data;
	BYTE	m_Track;

	BOOL	m_Flag;
	int		m_SubT;

	int GetEventSize();
	int SetEvent(DWORD pos, BYTE *pData);

	CNode();
	~CNode();
};

class CNodeList : public CObject
{
public:
	class CNode *m_Now;
	int m_nTrack;
	DWORD m_ActiveCh;

	CNode *NewNode();
	void FreeNode(CNode *pNode);

	CNode *Link(CNode *pNode);
	void Unlink(CNode *pNode);

	CNode *AddNode(DWORD pos, int track, int cmd, int note, int vol);
	CNode *AddNode(DWORD pos, int track, int cmd, int pam);
	CNode *AddNode(DWORD pos, int track, int cmd, int len, BYTE *data);

	CNode *GetNode(DWORD pos);
	CNode *TopNode(int nTrack = (-1));
	CNode *NextNode(int nTrack = (-1));
	CNode *BackNode(int nTrack = (-1));

	void RemoveAll();

	CNodeList();
	~CNodeList();
};

class CMMLData : public CObject
{
public:
	int m_Cmd;
	int m_Track;
	int m_Note;
	int m_Step;
	int m_Gate;
	int m_Velo;
	int m_Time;
	int m_Octa;
	int m_Keys;
	BYTE *m_pData;
	class CMMLData *m_pLast;
	class CMMLData *m_pNext;
	class CMMLData *m_pLink;
	class CMMLData *m_pWork;
	BOOL m_bNest;
	BOOL m_bBase;
	BOOL m_bTai;
	int m_AddStep;
	int m_RandVol;

	CMMLData *Add(CMMLData *pData);
	const class CMMLData & operator = (class CMMLData &data);
	void RemoveAll();

	CMMLData();
	CMMLData(class CMMLData &data);
	~CMMLData();
};

class CMidiData : public CNodeList
{
public:
	int m_Pos;
	int m_Size;
	BYTE *m_pData;

	void SetBuffer(BYTE *pData, int len);
	int IsEmpty();
	int GetByte();
	int GetDelta();
	BYTE *GetData(int len);
	void UnGetByte() { m_Pos--; }

	BOOL SetSmfData(BYTE *pData, int len);
	BOOL LoadFile(LPCTSTR fileName);
	BOOL SaveFile(LPCTSTR fileName);

	int m_mmlValue;
	LPCSTR m_mmlStr;
	CWordArray m_mmlParam;
	int m_mmlPos[16];
	int m_mmlBase[16];
	BYTE m_mmlIds[4];
	BYTE m_mmlPitch[16];
	CMMLData m_mmlData; 

	int Velocity(int vol, int randum);
	int ParseChar();
	int ParseDigit(int ch);
	int ParseSingle(int ch);
	int ParseMulDiv(int ch);
	int ParseAddSub(int ch);
	int ParseParam(int ch);
	int ParseStep(int ch);
	int ParseNote(int ch);

	CMMLData *JointData(CMMLData *pLast, CMMLData *pData, int nTai);
	BOOL UpdateJoint(CMMLData *pLast, CMMLData *pNext, int nTai);
	void UpdateStep(CMMLData *pData);
	void UpdateData(CMMLData *pData);
	BOOL LoadMML(LPCSTR str, int nInit);

	int m_NowTrack;

	void SaveSingStep(int step, CStringA &mbs);
	void SaveNoteStep(LPCSTR cmd, int step, CStringA &mbs);
	void SaveNote(CFile *pFile, CMMLData *pData);
	int SaveStepPos(CNode *pNode, int stepPos, int ch);
	BOOL GetNoteData(CNode *pNode, int &note, int &velo, int &step, int &gate);
	BOOL SaveMML(LPCTSTR fileName);

	int m_PlayMode;
	CNode *m_pPlayNode;
	CNode *m_pDispNode;
	DWORD m_PlayPos;
	DWORD m_SeekPos;
	HMIDISTRM m_hStream;
	HANDLE m_hThread;
	DWORD m_dwThreadID;
	MIDIHDR m_MidiHdr[MIDIHDR_MAX];
	DWORD m_DivVal;
	CNode *m_pStep;
	DWORD m_LastClock;

	static DWORD WINAPI ThreadProc(void *lpParameter);
	BOOL StreamQueIn();

	void Play();
	void Pause();
	void Stop();

	int GetCurrentTime();
	DWORD GetCurrentSong();
	DWORD GetCurrentPosition();
	int GetCurrentTempo();
	void GetCurrentStep(DWORD pos, int &step, int &div);

	CMidiData();
	~CMidiData();
};