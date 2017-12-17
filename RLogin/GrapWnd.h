#pragma once

#include "TextRam.h"
#include <afxtempl.h>

// CGrapWnd ÉtÉåÅ[ÉÄ

#define	TYPE_SIXEL	1
#define	TYPE_REGIS	2
#define	TYPE_VRAM	3

#define	WRST_OVERLAY	0
#define	WRST_REPLACE	1
#define	WRST_COMPLEMENT	2
#define	WRST_ERASE		3

#define	VT_COLMAP_ID	5		// < VT220 Mono ?
#define	SIXEL_PALET		1024
#define	ASP_DIV			1000

#define	GRAPLIST_INDEX	0
#define	GRAPLIST_IMAGE	1
#define	GRAPLIST_TYPE	2
#define	GRAPLIST_MAX	3

#define	GRAPMAX_X		(32 * 1024)
#define	GRAPMAX_Y		(32 * 1024)

class CGifAnime : public CBitmap
{
public:
	clock_t		m_Delay;
	int			m_TransIndex;
	BOOL		m_bHaveAlpha;
	int			m_Motd;

	const CGifAnime & operator = (CGifAnime &data);

	CGifAnime(void);
	~CGifAnime();
};

class CGrapWnd : public CFrameWnd
{
	DECLARE_DYNAMIC(CGrapWnd)

public:
	class CTextRam *m_pTextRam;
	int m_Type;
	int m_MaxX;
	int m_MaxY;
	int m_AspX;
	int m_AspY;
	int m_Maps;
	CDC m_TempDC;
	CBitmap *m_pActMap;
	CBitmap m_Bitmap[3];
	CBitmap *m_pOldMap;
	int m_ImageIndex;
	int m_BlockX;
	int m_BlockY;
	int m_CellX;
	int m_CellY;
	DWORD m_Crc;
	int m_Use;
	int m_Alive;
	class CGrapWnd *m_pList[GRAPLIST_MAX];
	int m_TransIndex;
	BOOL m_bHaveAlpha;
	int m_GifAnimePos;
	clock_t m_GifAnimeClock;
	CArray<CGifAnime, CGifAnime &> m_GifAnime;

	void SaveBitmap(int type);
	BOOL SaveImage(HANDLE hBitmap, const GUID &guid, CBuffer &buf);
	HANDLE GetBitmapHandle();
	CBitmap *GetBitmap(int width, int height);
	int Compare(CGrapWnd *pWnd);
	void Copy(CGrapWnd *pWnd);
	void SetMapCrc();
	void InitColMap();
	void DrawBlock(CDC *pDC, LPCRECT pRect, COLORREF bc, BOOL bEraBack, int sx, int sy, int ex, int ey, int scW, int scH, int chW, int chH, int cols, int lines);
	HANDLE GetBitmapBlock(COLORREF bc, int sx, int sy, int ex, int ey, int chW, int chH, int cols, int lines, BITMAP *pMap = NULL);

	void ListAdd(class CGrapWnd **pTop, int idx);
	void ListRemove(class CGrapWnd **pTop, int idx);
	class CGrapWnd *ListFindImage(class CGrapWnd *pPos);
	static class CGrapWnd *ListFindIndex(class CGrapWnd *pPos, int index);
	static class CGrapWnd *ListFindType(class CGrapWnd *pPos, int type);

	// Sixel
	int m_SixelStat;
	int m_SixelWidth, m_SixelHeight;
	int m_SixelPointX, m_SixelPointY, m_SixelRepCount;
	int m_SixelColorIndex, m_SixelValue;
	BOOL m_SixelValueInit;
	CDWordArray m_SixelParam;
	CDC m_SixelTempDC;
	COLORREF m_SixelBackColor;
	COLORREF m_SixelTransColor;

	void SixelStart(int aspect, int mode, int grid, COLORREF bc = 0);
	void SixelResize();
	void SixelData(int ch);
	void SixelEndof();
	void SetSixel(int aspect, int mode, int grid, LPCSTR str, COLORREF bc = 0);

	// Grap Image Load
	void SetImage(CImage &image, BOOL bCheckAlpha);
	BOOL LoadMemory(LPBYTE lpBuf, int len, CImage &image);
	void AddGifAnime(CImage &image, int width, int height, CRect &rect, int bkidx, COLORREF bkcol, int delay, int motd, int trsidx);
	BOOL LoadGifAnime(LPBYTE lpBuf, int len);
	BOOL LoadPicture(LPBYTE lpBuf, int len);
	inline BOOL IsGifAnime() { return (m_GifAnime.GetSize() > 1 ? TRUE : FALSE); }

	// ReGUIS
	typedef struct _ReGISPROC{
		int ch;
		int (CGrapWnd::*proc[5])(LPCSTR &p, int cmd, int num);
	} ReGISPROC;

	int GetChar(LPCSTR &p);
	int GetDigit(LPCSTR &p, int val);
	int GetBacket(LPCSTR &p, int &x, int &y);
	int GetColor(LPCSTR &p, COLORREF &rgb);
	int GetHexTab(LPCSTR &p, int max, BYTE *tab);
	int ParseCmds(LPCSTR &p, int cmd, const CGrapWnd::ReGISPROC *pCmd, const CGrapWnd::ReGISPROC cmdTab[]);
	void PushWparam();
	void PopWparam();

	BYTE m_BitPen;
	int m_MulPen;
	COLORREF m_ColPen;
	int m_LwPen;
	CPen m_ActPen;

	BYTE m_BitBrush;
	int m_MulBrush;
	COLORREF m_ColBrush;
	int m_CharBrush;
	int m_HatBrush;
	CBrush m_ActBrush;
	BOOL m_MapBrush;

	void ReSizeScreen();
	void SelectPage(int page);
	void CreatePen();
	void CreateBrush();
	void LineTo(int x, int y);
	void ArcTo(int x, int y);
	void UserFontDraw(int x, int y, COLORREF fc, COLORREF bc, BYTE *ptn);
	void TextTo(LPCSTR str);
	void SmoothLine(BOOL closed);
	void FillOpen();
	void FillClose();
	void TransparentOpen();
	void TransparentClose();

	// Screen Option
	int m_ActMap;
	COLORREF *m_ColMap;
	COLORREF m_PriColMap[SIXEL_PALET];
	COLORREF m_BakCol;
	BYTE m_PtnMap[10];

	// Write Option
	struct _ReGISWOPT {
		int Pvalue;
		COLORREF IntCol;
		BYTE BitPtn;
		BYTE NegPtn;
		int MulPtn;
		BOOL ShaCtl;
		int ShaChar;
		CPoint ShaPos;
		int LineWidth;
		int HatchIdx;
		int WriteStyle;
	} m_Wopt, m_SaveWopt;
	BOOL m_LocPush;

	BOOL m_UseXPos;
	CPoint m_Pos;
	CList<CPoint, CPoint &> m_PosStack;

	// Text Option
	struct _ReGISTOPT {
		CPoint DispSize;
		CPoint UnitSize;
		CPoint SpaceSize;
		CPoint OffsetSize;
		int FontItalic;
		int FontAngle;
		int FontSelect;
	} m_Topt, m_SaveTopt;

	int m_ActFont;
	CStringA m_UserFontName[4];
	BYTE m_UserFont[4][96][10];

	int m_ArcFlag;
	int m_ArcDegs;
	CArray<CPoint, CPoint &> m_ArcPoint;

	CStringA m_Macro[26];
	BOOL m_FilFlag;

	void ParseInit(int cmd);
	int ParseNumber(LPCSTR &p, int cmd, int num);
	int ParseVector(LPCSTR &p, int cmd, int num);
	int ParseBacket(LPCSTR &p, int cmd, int num);
	int ParseOption(LPCSTR &p, int cmd, int num);
	int ParseString(LPCSTR &p, int cmd, int num);
	int ParseSingel(LPCSTR &p, int cmd, int num);

	void SetReGIS(int mode, LPCSTR p);

	static COLORREF RGBtoHLS(COLORREF rgb);
	static COLORREF HLStoRGB(int hue, int lum, int sat);

	CGrapWnd(class CTextRam *pTextRam);
	virtual ~CGrapWnd();

protected:
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnGrapClose();
	afx_msg void OnGrapSave();
	afx_msg void OnGrapClear();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnUpdateGrapSave(CCmdUI *pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
};


