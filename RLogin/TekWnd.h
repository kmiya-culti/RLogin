#pragma once

#include "TextRam.h"
#include "GrapWnd.h"

// CTekWnd

class CTekWnd : public CFrameWndExt
{
	DECLARE_DYNAMIC(CTekWnd)

public:
	CWnd *m_pWnd;
	class CTextRam *m_pTextRam;

	void GinMouse(int ch, int x, int y);
	BOOL SaveDxf(LPCTSTR file);
	BOOL SaveTek(LPCTSTR file);
	void SaveSvgText(CBuffer &buf);
	BOOL SaveSvg(LPCTSTR file);

	double m_OfsX, m_OfsY;
	double m_Width, m_Height;
	double m_FontWidth, m_FontHeight;
	CStringIndex m_Defs;
	CStringIndex m_Style;

	typedef struct _TransFormParam {
		struct _TransFormParam *next;
		int	type;
		CArray<double, double> param;
	} TransFormParam;

	TransFormParam *m_pTransForm;

	LPCTSTR SvgTagStr(CStringIndex &xml, LPCTSTR name);
	LPCTSTR SvgParam(LPCTSTR p, CArray<double, double> &param, LPCTSTR pFormat = NULL);
	double SvgAngle(LPCTSTR p);
	double SvgLength(LPCTSTR p);

	void SvgTransFormClear();
	void SvgTransFromAngle(double &angle);
	void SvgTransFromSize(double &sx, double &sy);
	void SvgTransFromCalc(double &sx, double &sy);
	void SvgTransForm(CStringIndex *pXml);

	int SvgToTekColor(LPCTSTR p);
	int SvgToTekSt(CStringIndex &xml);
	void SvgToTekLine(double sx, double sy, double ex, double ey, int st);

	void SvgQuadBezier(double sx, double sy, double bx, double by, double ex, double ey, int st);
	void SvgCubicBezier(double sx, double sy, double bx1, double by1, double bx2, double by2, double ex, double ey, int st);

	void SvgLine(CStringIndex &xml);
	void SvgRect(CStringIndex &xml);
	void SvgPolyline(CStringIndex &xml);
	void SvgPolygon(CStringIndex &xml);
	void SvgPath(CStringIndex &xml);
	void SvgCircle(CStringIndex &xml);
	void SvgEllipse(CStringIndex &xml);

	void SvgSubText(double param[7], int st, int wmd, LPCTSTR p, CArray<double, double> list[5]);
	void SvgToTextSt(CStringIndex &xml, int &st, int &wmd, double &fw, double &fh);
	void SvgTspan(CStringIndex &xml, double param[7], CArray<double, double> list[5]);
	void SvgText(CStringIndex &xml);

	LPCTSTR SvgStyleSkip(LPCTSTR p);
	void SvgStyleProp(CStringIndex &xml, LPCTSTR p);
	void SvgStyleSet(CStringIndex &xml, LPCTSTR pBase, LPCTSTR pClass, LPCTSTR pPrefix = NULL);
	void SvgStyleCheck(CStringIndex &xml, LPCTSTR pBase);

	LPCTSTR SvgStyleParse(LPCTSTR p, BOOL bSkip = FALSE);
	void SvgStyle(CStringIndex &xml);

	BOOL SvgCmds(CStringIndex &xml);
	void SvgDefs(CStringIndex &xml);
	void SvgUse(CStringIndex &xml);
	void SvgGroup(CStringIndex &xml);
	void SvgRoot(CStringIndex &xml, BOOL bInit);

	BOOL LoadSvgText(LPCTSTR text);
	BOOL LoadSvg(LPCTSTR file);

	CTekWnd(class CTextRam *pTextRam, CWnd *pWnd);
	virtual ~CTekWnd();

protected:
	virtual void PostNcDestroy();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnTekClose();
	afx_msg void OnTekSave();
	afx_msg void OnTekClear();
	afx_msg void OnTekMode();
	afx_msg void OnUpdateTekMode(CCmdUI *pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnFilePrint();
};


