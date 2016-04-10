//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include <math.h>
#include "emule.h"
#include "OScopeCtrl.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// COScopeCtrl

CFont	COScopeCtrl::sm_fontAxis;
LOGFONT	COScopeCtrl::sm_logFontAxis;

BEGIN_MESSAGE_MAP(COScopeCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

COScopeCtrl::COScopeCtrl(int NTrends)
{
	int i;
	static const COLORREF PresetColor[16] = 
	{
		RGB(0xFF, 0x00, 0x00),
		RGB(0xFF, 0xC0, 0xC0),
		
		RGB(0xFF, 0xFF, 0x00),
		RGB(0xFF, 0xA0, 0x00),
		RGB(0xA0, 0x60, 0x00),
		
		RGB(0x00, 0xFF, 0x00),
		RGB(0x00, 0xA0, 0x00),
		
		RGB(0x00, 0x00, 0xFF),
		RGB(0x00, 0xA0, 0xFF),
		RGB(0x00, 0xFF, 0xFF),
		RGB(0x00, 0xA0, 0xA0),
		
		RGB(0xC0, 0xC0, 0xFF),
		RGB(0xFF, 0x00, 0xFF),
		RGB(0xA0, 0x00, 0xA0),
		
		RGB(0xFF, 0xFF, 0xFF),
		RGB(0x80, 0x80, 0x80)
	};

	//  *)	Using "Arial" or "MS Sans Serif" gives a more accurate small font,
	//		but does not work for Korean fonts.
	//	*)	Using "MS Shell Dlg" gives somewhat less accurate small fonts, but
	//		does work for all languages which are currently supported by eMule.
	// 8pt 'MS Shell Dlg' -- this shall be available on all Windows systems..
	if (sm_fontAxis.m_hObject == NULL) {
		if (CreatePointFont(sm_fontAxis, 8*10, theApp.GetDefaultFontFaceName()))
			sm_fontAxis.GetLogFont(&sm_logFontAxis);
		else if (sm_logFontAxis.lfHeight == 0) {
			memset(&sm_logFontAxis, 0, sizeof sm_logFontAxis);
			sm_logFontAxis.lfHeight = 10;
		}
	}

	// since plotting is based on a LineTo for each new point
	// we need a starting point (i.e. a "previous" point)
	// use 0.0 as the default first point.
	// these are public member variables, and can be changed outside
	// (after construction).  
	// G.Hayduk: NTrends is the number of trends that will be drawn on
	// the plot. First 15 plots have predefined colors, but others will
	// be drawn with white, unless you call SetPlotColor
	m_PlotData = new PlotData_t[NTrends];
	m_NTrends = NTrends;
	for (i = 0; i < m_NTrends; i++)
	{
		if (i < 15)
			m_PlotData[i].crPlotColor  = PresetColor[i];  // see also SetPlotColor
		else
			m_PlotData[i].crPlotColor  = RGB(255, 255, 255);  // see also SetPlotColor
		m_PlotData[i].penPlot.CreatePen(PS_SOLID, 0, m_PlotData[i].crPlotColor);
		m_PlotData[i].dPreviousPosition = 0.0;
		m_PlotData[i].nPrevY = -1;
		m_PlotData[i].dLowerLimit = -10.0;
		m_PlotData[i].dUpperLimit =  10.0;
		m_PlotData[i].dRange = m_PlotData[i].dUpperLimit - m_PlotData[i].dLowerLimit;
		m_PlotData[i].lstPoints.AddTail(0.0);
		// Initialize our new trend ratio variable to 1
		m_PlotData[i].iTrendRatio = 1;
		m_PlotData[i].LegendLabel.Format(_T("Legend %i"),i);
		m_PlotData[i].BarsPlot = false;
	}
	
	// public variable for the number of decimal places on the y axis
	// G.Hayduk: I've deleted the possibility of changing this parameter
	// in SetRange, so change it after constructing the plot
	m_nYDecimals = 1;
	
	// set some initial values for the scaling until "SetRange" is called.
	// these are protected varaibles and must be set with SetRange
	// in order to ensure that m_dRange is updated accordingly
	
	// m_nShiftPixels determines how much the plot shifts (in terms of pixels) 
	// with the addition of a new data point
	drawBars = false;
	autofitYscale = false;
	m_nShiftPixels = 1;
	m_nTrendPoints = 0;
	m_nMaxPointCnt = 1024;
	CustShift.m_nPointsToDo = 0;
	// G.Hayduk: actually I needed an OScopeCtrl to draw specific number of
	// data samples and stretch them on the plot ctrl. Now, OScopeCtrl has
	// two modes of operation: fixed Shift (when m_nTrendPoints=0, 
	// m_nShiftPixels is in use), or fixed number of Points in the plot width
	// (when m_nTrendPoints>0)
	// When m_nTrendPoints>0, CustShift structure is in use
	
	// background, grid and data colors
	// these are public variables and can be set directly
	m_crBackColor = RGB(0,   0,   0);  // see also SetBackgroundColor
	m_crGridColor = RGB(0, 255, 255);  // see also SetGridColor
	
	// public member variables, can be set directly 
	m_str.XUnits.Format(_T("Samples"));  // can also be set with SetXUnits
	m_str.YUnits.Format(_T("Y units"));  // can also be set with SetYUnits
	
	// G.Hayduk: configurable number of grids init
	// you are free to change those between contructing the object 
	// and calling Create
	m_nXGrids = 6;
	m_nYGrids = 5;
	m_nTrendPoints = -1;
	m_nXPartial = 0;

	m_bDoUpdate = true;
	m_nRedrawTimer = 0;
	m_uLastMouseFlags = 0;
	m_ptLastMousePos.x = -1;
	m_ptLastMousePos.y = -1;

	ready = false;
}

COScopeCtrl::~COScopeCtrl()
{
	if (m_bitmapOldGrid.m_hObject)
		m_dcGrid.SelectObject(m_bitmapOldGrid.Detach());
	if (m_bitmapOldPlot.m_hObject)
		m_dcPlot.SelectObject(m_bitmapOldPlot.Detach());
	delete[] m_PlotData;
}

BOOL COScopeCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) 
{
	BOOL result;
	static CString className = AfxRegisterWndClass(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	
	result = CWnd::CreateEx(/*WS_EX_CLIENTEDGE*/ // strong (default) border
							WS_EX_STATICEDGE,	// lightweight border
							className, NULL, dwStyle, 
							rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
							pParentWnd->GetSafeHwnd(), (HMENU)nID);
	if (result != 0)
		InvalidateCtrl();
	InitWindowStyles(this);
	
	ready = true;
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// Set Trend Ratio
//
// This allows us to set a ratio for a trend in our plot.  Basically, this
// trend will be divided by whatever the ratio was set to, so that we can have
// big numbers and little numbers in the same plot.  Wrote this especially for
// eMule.
//
// iTrend is an integer specifying which trend of this plot we should set the
// ratio for.
//
// iRatio is an integer defining what we should divide this trend's data by.
// For example, to have a 1:2 ratio of Y-Scale to this trend, iRatio would be 2.
// iRatio is 1 by default (No change in scale of data for this trend)
// This function now borrows a bit from eMule Plus v1
//
void COScopeCtrl::SetTrendRatio(int iTrend, UINT iRatio)
{
	ASSERT(iTrend < m_NTrends && iRatio > 0);	// iTrend must be a valid trend in this plot.

	if (iRatio != (UINT)m_PlotData[iTrend].iTrendRatio) {
		double dTrendModifier = (double)m_PlotData[iTrend].iTrendRatio / iRatio;
		m_PlotData[iTrend].iTrendRatio = iRatio;

		int iCnt = m_PlotData[iTrend].lstPoints.GetCount();
		for (int i = 0; i < iCnt; i++)
		{	
			POSITION pos = m_PlotData[iTrend].lstPoints.FindIndex(i);
			if (pos)
				m_PlotData[iTrend].lstPoints.SetAt(pos,m_PlotData[iTrend].lstPoints.GetAt(pos) * dTrendModifier);
		}
		InvalidateCtrl();
	}
}

void COScopeCtrl::SetLegendLabel(CString string, int iTrend)
{
	m_PlotData[iTrend].LegendLabel = string;
	InvalidateCtrl(false);
}

void COScopeCtrl::SetBarsPlot(bool BarsPlot, int iTrend)
{
	m_PlotData[iTrend].BarsPlot = BarsPlot;
	InvalidateCtrl(false);
}

void COScopeCtrl::SetRange(double dLower, double dUpper, int iTrend)
{
	ASSERT(dUpper > dLower);
	
	m_PlotData[iTrend].dLowerLimit     = dLower;
	m_PlotData[iTrend].dUpperLimit     = dUpper;
	m_PlotData[iTrend].dRange          = m_PlotData[iTrend].dUpperLimit - m_PlotData[iTrend].dLowerLimit;
	m_PlotData[iTrend].dVerticalFactor = (double)m_nPlotHeight / m_PlotData[iTrend].dRange; 
	InvalidateCtrl();
}

void COScopeCtrl::SetRanges(double dLower, double dUpper)
{
	ASSERT( dUpper > dLower );

	int iTrend;
	for (iTrend = 0; iTrend < m_NTrends; iTrend ++)
	{
		m_PlotData[iTrend].dLowerLimit     = dLower;
		m_PlotData[iTrend].dUpperLimit     = dUpper;
		m_PlotData[iTrend].dRange          = m_PlotData[iTrend].dUpperLimit - m_PlotData[iTrend].dLowerLimit;
		m_PlotData[iTrend].dVerticalFactor = (double)m_nPlotHeight / m_PlotData[iTrend].dRange; 
	}
	InvalidateCtrl();
}

void COScopeCtrl::SetXUnits(CString string, CString XMin, CString XMax)
{
	m_str.XUnits = string;
	m_str.XMin = XMin;
	m_str.XMax = XMax;
	InvalidateCtrl(false);
}

void COScopeCtrl::SetYUnits(CString string, CString YMin, CString YMax)
{
	m_str.YUnits = string;
	m_str.YMin = YMin;
	m_str.YMax = YMax;
	InvalidateCtrl();
}

void COScopeCtrl::SetGridColor(COLORREF color)
{
	m_crGridColor = color;
	InvalidateCtrl();
}

void COScopeCtrl::SetPlotColor(COLORREF color, int iTrend)
{
	m_PlotData[iTrend].crPlotColor = color;
	m_PlotData[iTrend].penPlot.DeleteObject();
	m_PlotData[iTrend].penPlot.CreatePen(PS_SOLID, 0, m_PlotData[iTrend].crPlotColor);
	//InvalidateCtrl();
}

COLORREF COScopeCtrl::GetPlotColor(int iTrend)
{
	return m_PlotData[iTrend].crPlotColor;
}

void COScopeCtrl::SetBackgroundColor(COLORREF color)
{
	m_crBackColor = color;
	InvalidateCtrl();
}

void COScopeCtrl::InvalidateCtrl(bool deleteGraph)
{
	int i, j, GridPos;
	int nCharacters;
	CPen *oldPen;
	CPen solidPen(PS_SOLID, 0, m_crGridColor);
	CFont yUnitFont, *oldFont;
	CString strTemp;
	
	CClientDC dc(this);  
	
	// if we don't have one yet, set up a memory dc for the grid
	if (m_dcGrid.GetSafeHdc() == NULL)
	{
		m_dcGrid.CreateCompatibleDC(&dc);
		m_bitmapGrid.DeleteObject();
		m_bitmapGrid.CreateCompatibleBitmap(&dc, m_nClientWidth, m_nClientHeight);
		m_bitmapOldGrid.Attach(SelectObject(m_dcGrid, m_bitmapGrid));
	}
	
	COLORREF crLabelBk;
	COLORREF crLabelFg;
	bool bStraightGraphs = true;
	if (bStraightGraphs) {
		// Get the background color from the parent window. This way the controls which are
		// embedded in a dialog window can get painted with the same background color as
		// the dialog window.
		HBRUSH hbr = (HBRUSH)GetParent()->SendMessage(WM_CTLCOLORSTATIC, (WPARAM)dc.m_hDC, (LPARAM)m_hWnd);
		if (hbr == GetSysColorBrush(COLOR_WINDOW))
			crLabelBk = GetSysColor(COLOR_WINDOW);
		else
			crLabelBk = GetSysColor(COLOR_BTNFACE);
		crLabelFg = GetSysColor(COLOR_WINDOWTEXT);
	}
	else {
		crLabelBk = m_crBackColor;
		crLabelFg = m_crGridColor;
	}

	// fill the grid background
	m_dcGrid.FillSolidRect(m_rectClient, crLabelBk);
	
	// draw the plot rectangle: determine how wide the y axis scaling values are
	double fAbsUpperLimit = fabs(m_PlotData[0].dUpperLimit);
	if (fAbsUpperLimit > 0.0)
		nCharacters = abs((int)log10(fAbsUpperLimit));
	else
		nCharacters = 0;

	double fAbsLowerLimit = fabs(m_PlotData[0].dLowerLimit);
	if (fAbsLowerLimit > 0.0)
		nCharacters = max(nCharacters, abs((int)log10(fAbsLowerLimit)));
	
	// add the units digit, decimal point and a minus sign, and an extra space
	// as well as the number of decimal places to display
	nCharacters = nCharacters + 4 + m_nYDecimals;  
	
	// adjust the plot rectangle dimensions
	// Changed this so that the Y-Units wouldn't overlap the Y-Scale.
	m_rectPlot.left = m_rectClient.left + 8*7+4;//(nCharacters) ;
	m_nPlotWidth    = m_rectPlot.Width();
	
	// draw the plot rectangle
	if (bStraightGraphs)
	{
		m_dcGrid.FillSolidRect(m_rectPlot.left, m_rectPlot.top, m_rectPlot.right - m_rectPlot.left + 1, m_rectPlot.bottom - m_rectPlot.top + 1, m_crBackColor);

		CRect rcPlot(m_rectPlot);
		rcPlot.left -= 1;
		rcPlot.top -= 1;
		rcPlot.right += 3;
		rcPlot.bottom += 3;
		m_dcGrid.DrawEdge(rcPlot, EDGE_SUNKEN, BF_RECT);
	}
	else
	{
		oldPen = m_dcGrid.SelectObject(&solidPen); 
		m_dcGrid.MoveTo(m_rectPlot.left, m_rectPlot.top);
		m_dcGrid.LineTo(m_rectPlot.right + 1, m_rectPlot.top);
		m_dcGrid.LineTo(m_rectPlot.right + 1, m_rectPlot.bottom + 1);
		m_dcGrid.LineTo(m_rectPlot.left, m_rectPlot.bottom + 1);
		m_dcGrid.LineTo(m_rectPlot.left, m_rectPlot.top);
		m_dcGrid.SelectObject(oldPen); 
	}

	// draw the dotted lines, 
	// use SetPixel instead of a dotted pen - this allows for a finer dotted line and a more "technical" look
	for (j = 1; j < m_nYGrids + 1; j++)
	{
		GridPos = m_rectPlot.Height() * j / (m_nYGrids + 1) + m_rectPlot.top;
		for (i = m_rectPlot.left; i < m_rectPlot.right; i += 4)
			m_dcGrid.SetPixel(i, GridPos, m_crGridColor);
	}

	if (thePrefs.m_bShowVerticalHourMarkers)
	{
		// Add vertical reference lines in the graphs. Each line indicates an elapsed hour from the current
		// time (extreme right of the graph).
		// Lines are always right aligned and the gap scales accordingly to the user horizontal scale.
		// Intervals of 10 hours are marked with slightly stronger lines that go beyond the bottom border.
		int hourSize, partialSize, surplus=0, extra=0;
		if (m_nXGrids > 0) {
			hourSize = (3600*m_rectPlot.Width())/(3600*m_nXGrids + m_nXPartial); // Size of an hour in pixels
			partialSize = m_rectPlot.Width() - hourSize*m_nXGrids;
			if (partialSize >= hourSize) {
				partialSize = (hourSize*m_nXPartial)/3600; // real partial size
				surplus = m_rectPlot.Width() - hourSize*m_nXGrids - partialSize; // Pixel surplus
			}

			GridPos = 0;
			for(j = 1; j <= m_nXGrids; j++) {
				extra = 0;
				if (surplus) {
					surplus--;
					extra=1;
				}
				GridPos += (hourSize+extra);
				if ((m_nXGrids - j + 1) % 10 == 0) {
					for(i = m_rectPlot.top; i < m_rectPlot.bottom; i += 2)
						m_dcGrid.SetPixel(m_rectPlot.left + GridPos - hourSize + partialSize, i, m_crGridColor);
				} else {
					for(i = m_rectPlot.top; i < m_rectPlot.bottom; i += 4)
						m_dcGrid.SetPixel(m_rectPlot.left + GridPos - hourSize + partialSize, i, m_crGridColor);
				}
			}
		}
	}

	if (afxIsWin95()) {
		// Win98: To get a rotated font it has to be specified as "Arial" ("MS Shell Dlg" 
		// and "MS Sans Serif" are not created with rotation)
		yUnitFont.CreateFont(FontPointSizeToLogUnits(8*10), 0, 900, 900, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET,
							 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Arial"));
	}
	else {
		yUnitFont.CreateFont(FontPointSizeToLogUnits(8*10), 0, 900, 900, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET,
							 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, theApp.GetDefaultFontFaceName());
	}

	// grab the horizontal font
	oldFont = m_dcGrid.SelectObject(&sm_fontAxis);
	
	// y max
	m_dcGrid.SetTextColor(crLabelFg);
	m_dcGrid.SetBkColor(crLabelBk);
	m_dcGrid.SetTextAlign(TA_RIGHT | TA_TOP);
	if (m_str.YMax.IsEmpty())
		strTemp.Format(_T("%.*lf"), m_nYDecimals, m_PlotData[0].dUpperLimit);
	else
		strTemp = m_str.YMax;
	m_dcGrid.TextOut(m_rectPlot.left - 4, m_rectPlot.top - 7, strTemp);
	
    if (m_rectPlot.Height() / (m_nYGrids + 1) >= 14) {
	    for (j = 1; j < (m_nYGrids + 1); j++) {
		    GridPos = m_rectPlot.Height() * j / (m_nYGrids + 1) + m_rectPlot.top;
    	    strTemp.Format(_T("%.*lf"), m_nYDecimals, m_PlotData[0].dUpperLimit * (m_nYGrids - j + 1) / (m_nYGrids + 1));
    	    m_dcGrid.TextOut(m_rectPlot.left - 4, GridPos - 7, strTemp);
        }
    } else {
	    strTemp.Format(_T("%.*lf"), m_nYDecimals, m_PlotData[0].dUpperLimit / 2);
	    m_dcGrid.TextOut(m_rectPlot.left - 2, m_rectPlot.bottom + ((m_rectPlot.top - m_rectPlot.bottom) / 2) - 7 , strTemp);
    }	
	
	// y min
	if (m_str.YMin.IsEmpty())
		strTemp.Format(_T("%.*lf"), m_nYDecimals, m_PlotData[0].dLowerLimit);
	else
		strTemp = m_str.YMin;
	m_dcGrid.TextOut(m_rectPlot.left - 4, m_rectPlot.bottom - 7, strTemp);

	// x units
	m_dcGrid.SetTextAlign(TA_RIGHT | TA_BOTTOM);
	m_dcGrid.TextOut(m_rectClient.right - 2, m_rectClient.bottom - 2, m_str.XUnits);

	// restore the font
	m_dcGrid.SelectObject(oldFont);
	
	// y units
	oldFont = m_dcGrid.SelectObject(&yUnitFont);
	m_dcGrid.SetTextAlign(TA_CENTER | TA_BASELINE);
	
	CRect rText(0,0,0,0);
	m_dcGrid.DrawText(m_str.YUnits, rText, DT_CALCRECT);
	m_dcGrid.TextOut((m_rectClient.left + m_rectPlot.left - 8) / 2 - rText.Height() / 2,
					 (m_rectPlot.bottom + m_rectPlot.top) / 2 - rText.Height() / 2,
					 m_str.YUnits );
	m_dcGrid.SelectObject(oldFont);

	oldFont = m_dcGrid.SelectObject(&sm_fontAxis);
	m_dcGrid.SetTextAlign(TA_LEFT | TA_TOP);

	int xpos = m_rectPlot.left + 2;
	int ypos = m_rectPlot.bottom + 3;
	for (i = 0; i < m_NTrends; i++)
	{
		CSize sizeLabel = m_dcGrid.GetTextExtent(m_PlotData[i].LegendLabel);
		if (xpos + 12 + sizeLabel.cx + 12 > m_rectPlot.right) {
			xpos = m_rectPlot.left + 2;
			ypos = m_rectPlot.bottom + sizeLabel.cy + 2;
		}

		if (bStraightGraphs)
		{
			const int iLegFrmD = 1;
			CPen penFrame(PS_SOLID, iLegFrmD, crLabelFg);
			oldPen = m_dcGrid.SelectObject(&penFrame);
			const int iLegBoxW = 9;
			const int iLegBoxH = 9;
			CRect rcLegendFrame;
			rcLegendFrame.left = xpos - iLegFrmD;
			rcLegendFrame.top = ypos + 2 - iLegFrmD;
			rcLegendFrame.right = rcLegendFrame.left + iLegBoxW + iLegFrmD;
			rcLegendFrame.bottom = rcLegendFrame.top + iLegBoxH + iLegFrmD;
			m_dcGrid.MoveTo(rcLegendFrame.left, rcLegendFrame.top);
			m_dcGrid.LineTo(rcLegendFrame.right, rcLegendFrame.top);
			m_dcGrid.LineTo(rcLegendFrame.right, rcLegendFrame.bottom);
			m_dcGrid.LineTo(rcLegendFrame.left, rcLegendFrame.bottom);
			m_dcGrid.LineTo(rcLegendFrame.left, rcLegendFrame.top);
			m_dcGrid.SelectObject(oldPen);
			m_dcGrid.FillSolidRect(xpos, ypos + 2, iLegBoxW, iLegBoxH, m_PlotData[i].crPlotColor);
			m_dcGrid.SetBkColor(crLabelBk);
		}
		else
		{
			CPen LegendPen(PS_SOLID, 3, m_PlotData[i].crPlotColor);
			oldPen = m_dcGrid.SelectObject(&LegendPen);
			m_dcGrid.MoveTo(xpos, ypos + 8);
			m_dcGrid.LineTo(xpos + 8, ypos + 4);
			m_dcGrid.SelectObject(oldPen);
		}

		m_dcGrid.TextOut(xpos + 12, ypos, m_PlotData[i].LegendLabel);
		xpos += 12 + sizeLabel.cx + 12;
	}

	m_dcGrid.SelectObject(oldFont);
	
	// if we don't have one yet, set up a memory dc for the plot
	if (m_dcPlot.GetSafeHdc() == NULL)
	{
		m_dcPlot.CreateCompatibleDC(&dc);
		m_bitmapPlot.DeleteObject();
		m_bitmapPlot.CreateCompatibleBitmap(&dc, m_nClientWidth, m_nClientHeight);
		m_bitmapOldPlot.Attach(SelectObject(m_dcPlot, m_bitmapPlot));
	}
	
	// make sure the plot bitmap is cleared
	if (deleteGraph)
		m_dcPlot.FillSolidRect(m_rectClient, m_crBackColor);

	int iNewSize = m_rectClient.Width() / m_nShiftPixels + 10;		// +10 just in case :)
	if (m_nMaxPointCnt < iNewSize)
		m_nMaxPointCnt = iNewSize;

	if (theApp.emuledlg->IsRunning()) 
	{
		if (!thePrefs.IsGraphRecreateDisabled())
		{
			// The timer will redraw the previous points in 200ms
			m_bDoUpdate = false;
			if (m_nRedrawTimer)
				KillTimer(m_nRedrawTimer);
			VERIFY( (m_nRedrawTimer = SetTimer(1612, 200, NULL)) != NULL ); // reduce flickering
		}
	}

	InvalidateRect(m_rectClient);
}

void COScopeCtrl::AppendPoints(double dNewPoint[], bool bInvalidate, bool bAdd2List, bool bUseTrendRatio)
{
	// append a data point to the plot
	int iTrend;
	for (iTrend = 0; iTrend < m_NTrends; iTrend ++)
	{
		// Changed this to support the new TrendRatio var
		if (bUseTrendRatio)
			m_PlotData[iTrend].dCurrentPosition = (double)dNewPoint[iTrend] / m_PlotData[iTrend].iTrendRatio;
		else
			m_PlotData[iTrend].dCurrentPosition = dNewPoint[iTrend];
		if (bAdd2List)
		{
			m_PlotData[iTrend].lstPoints.AddTail(m_PlotData[iTrend].dCurrentPosition);
			while (m_PlotData[iTrend].lstPoints.GetCount() > m_nMaxPointCnt)
				m_PlotData[iTrend].lstPoints.RemoveHead();
		}
	}
	
	// Sometime responsible for 'ghost' point on the left after a resize
	if (!m_bDoUpdate)
		return;

	if (m_nTrendPoints > 0)
	{
		if (CustShift.m_nPointsToDo == 0)
		{
			CustShift.m_nPointsToDo = m_nTrendPoints - 1;
			CustShift.m_nWidthToDo = m_nPlotWidth;
			CustShift.m_nRmndr = 0;
		}
		
		// a little bit tricky setting m_nShiftPixels in "fixed number of points through plot width" mode
		m_nShiftPixels = (CustShift.m_nWidthToDo + CustShift.m_nRmndr) / CustShift.m_nPointsToDo;
		CustShift.m_nRmndr = (CustShift.m_nWidthToDo + CustShift.m_nRmndr) % CustShift.m_nPointsToDo;
		if (CustShift.m_nPointsToDo == 1)
			m_nShiftPixels = CustShift.m_nWidthToDo;
		CustShift.m_nWidthToDo -= m_nShiftPixels;
		CustShift.m_nPointsToDo--;
	}
	DrawPoint();
	
	if (bInvalidate && ready && m_bDoUpdate)
		Invalidate();
	
	return;
}

/////////////////////////////////////////////////////////////////////////////
// G.Hayduk:
// AppendEmptyPoints adds a vector of data points, without drawing them
// (but shifting the plot), this way you can do a "hole" (space) in the plot
// i.e. indicating "no data here". When points are available, call AppendEmptyPoints
// for first valid vector of data points, and then call AppendPoints again and again 
// for valid points
//
void COScopeCtrl::AppendEmptyPoints(double dNewPoint[], bool bInvalidate, bool bAdd2List, bool bUseTrendRatio)
{
	int iTrend, currY;
	CRect ScrollRect, rectCleanUp;
	// append a data point to the plot
	// return the previous point
	for (iTrend = 0; iTrend < m_NTrends; iTrend ++)
	{
		if (bUseTrendRatio)
			m_PlotData[iTrend].dCurrentPosition = (double)dNewPoint[iTrend] / m_PlotData[iTrend].iTrendRatio;
		else
			m_PlotData[iTrend].dCurrentPosition = dNewPoint[iTrend];
		if (bAdd2List)
			m_PlotData[iTrend].lstPoints.AddTail(m_PlotData[iTrend].dCurrentPosition);
	}
	if (m_nTrendPoints > 0)
	{
		if (CustShift.m_nPointsToDo == 0)
		{
			CustShift.m_nPointsToDo = m_nTrendPoints - 1;
			CustShift.m_nWidthToDo = m_nPlotWidth;
			CustShift.m_nRmndr = 0;
		}
		m_nShiftPixels = (CustShift.m_nWidthToDo + CustShift.m_nRmndr) / CustShift.m_nPointsToDo;
		CustShift.m_nRmndr = (CustShift.m_nWidthToDo + CustShift.m_nRmndr) % CustShift.m_nPointsToDo;
		if (CustShift.m_nPointsToDo == 1)
			m_nShiftPixels = CustShift.m_nWidthToDo;
		CustShift.m_nWidthToDo -= m_nShiftPixels;
		CustShift.m_nPointsToDo--;
	}
	
	// DrawPoint's shift process
	if (m_dcPlot.GetSafeHdc() != NULL)
	{
		if (m_nShiftPixels > 0)
		{
			ScrollRect.left = m_rectPlot.left;
			ScrollRect.top = m_rectPlot.top + 1;
			ScrollRect.right = m_rectPlot.left + m_nPlotWidth;
			ScrollRect.bottom = m_rectPlot.top + 1 + m_nPlotHeight;
			ScrollRect = m_rectPlot;
			ScrollRect.right++;
			m_dcPlot.ScrollDC(-m_nShiftPixels, 0, ScrollRect, ScrollRect, NULL, NULL);
			
			// establish a rectangle over the right side of plot
			// which now needs to be cleaned up prior to adding the new point
			rectCleanUp = m_rectPlot;
			rectCleanUp.left = rectCleanUp.right - m_nShiftPixels + 1;
			rectCleanUp.right ++;
			m_dcPlot.FillSolidRect(rectCleanUp, m_crBackColor); // fill the cleanup area with the background
		}
		
		// draw the next line segement
		for (iTrend = 0; iTrend < m_NTrends; iTrend ++)
		{
			currY = m_rectPlot.bottom 
					- (long)((m_PlotData[iTrend].dCurrentPosition - m_PlotData[iTrend].dLowerLimit) 
					         * m_PlotData[iTrend].dVerticalFactor);
			m_PlotData[iTrend].nPrevY = currY;

			// store the current point for connection to the next point
			m_PlotData[iTrend].dPreviousPosition = m_PlotData[iTrend].dCurrentPosition;
		}
	}
	
	if (bInvalidate && m_bDoUpdate)
		Invalidate();
}
 
void COScopeCtrl::OnPaint() 
{
	CPaintDC dc(this);
	CDC memDC;
	CBitmap memBitmap;
	CBitmap* oldBitmap;
	
	memDC.CreateCompatibleDC(&dc);
	memBitmap.CreateCompatibleBitmap(&dc, m_nClientWidth, m_nClientHeight);
	oldBitmap = (CBitmap*)memDC.SelectObject(&memBitmap);
	
	if (memDC.GetSafeHdc() != NULL)
	{
		// first drop the grid on the memory dc
		memDC.BitBlt(0, 0, m_nClientWidth, m_nClientHeight,  &m_dcGrid, 0, 0, SRCCOPY);
		
		// now add the plot on top as a "pattern" via SRCPAINT. works well with dark background and a light plot
		memDC.BitBlt(0, 0, m_nClientWidth, m_nClientHeight, &m_dcPlot, 0, 0, SRCPAINT);

		// finally send the result to the display
		dc.BitBlt(0, 0, m_nClientWidth, m_nClientHeight, &memDC, 0, 0, SRCCOPY);
	}
	memDC.SelectObject(oldBitmap);
	memBitmap.DeleteObject();
}

void COScopeCtrl::DrawPoint()
{
	// this does the work of "scrolling" the plot to the left and appending a new data point all of the plotting is
    // directed to the memory based bitmap associated with m_dcPlot the will subsequently be BitBlt'd to the client
	// in OnPaint

	int currX, prevX, currY, prevY, iTrend;
	CPen *oldPen;
	CRect ScrollRect, rectCleanUp;

	if (m_dcPlot.GetSafeHdc() != NULL)
	{
		if (m_nShiftPixels > 0)
		{
			ScrollRect = m_rectPlot;
			ScrollRect.left++;
			ScrollRect.right++;
			ScrollRect.bottom++;
			m_dcPlot.ScrollDC(-m_nShiftPixels, 0, ScrollRect, ScrollRect, NULL, NULL);

			// establish a rectangle over the right side of plot
			// which now needs to be cleaned up prior to adding the new point
			rectCleanUp = m_rectPlot;
			rectCleanUp.left = rectCleanUp.right - m_nShiftPixels + 1;
			rectCleanUp.right++;
			rectCleanUp.bottom++;
			m_dcPlot.FillSolidRect(rectCleanUp, m_crBackColor); // fill the cleanup area with the background
		}
		
		// draw the next line segement
		for (iTrend = 0; iTrend < m_NTrends; iTrend ++)
		{
			// grab the plotting pen
			oldPen = m_dcPlot.SelectObject(&m_PlotData[iTrend].penPlot);
			
			// move to the previous point
			prevX = m_rectPlot.right - m_nShiftPixels;
			if (m_PlotData[iTrend].nPrevY > 0) {
				prevY = m_PlotData[iTrend].nPrevY;
			}
			else {
				prevY = m_rectPlot.bottom 
						- (long)((m_PlotData[iTrend].dPreviousPosition - m_PlotData[iTrend].dLowerLimit) 
						         * m_PlotData[iTrend].dVerticalFactor);
			}
			if (!m_PlotData[iTrend].BarsPlot)
				m_dcPlot.MoveTo(prevX, prevY);

			// draw to the current point
			currX = m_rectPlot.right;
			currY = m_rectPlot.bottom
				    - (long)((m_PlotData[iTrend].dCurrentPosition - m_PlotData[iTrend].dLowerLimit) 
					         * m_PlotData[iTrend].dVerticalFactor);
			m_PlotData[iTrend].nPrevY = currY;
			if (m_PlotData[iTrend].BarsPlot)
				m_dcPlot.MoveTo(currX, m_rectPlot.bottom);
			else {
				if (abs(prevX - currX) > abs(prevY - currY))
					currX += prevX - currX > 0 ? -1 : 1;
				else 
					currY += prevY - currY > 0 ? -1 : 1;
			}
			m_dcPlot.LineTo(currX, currY);
			m_dcPlot.SelectObject(oldPen);
			
			// if the data leaks over the upper or lower plot boundaries fill the upper and lower leakage with 
			// the background this will facilitate clipping on an as needed basis as opposed to always calling
			// IntersectClipRect
			if (prevY <= m_rectPlot.top || currY <= m_rectPlot.top)
				m_dcPlot.FillSolidRect(CRect(prevX - 1, m_rectClient.top, currX + 5, m_rectPlot.top + 1), m_crBackColor);
			if (prevY > m_rectPlot.bottom || currY > m_rectPlot.bottom)
				m_dcPlot.FillSolidRect(CRect(prevX - 1, m_rectPlot.bottom + 1, currX + 5, m_rectClient.bottom + 1), m_crBackColor);
			
			// store the current point for connection to the next point
			m_PlotData[iTrend].dPreviousPosition = m_PlotData[iTrend].dCurrentPosition;
		}
	}
}

void COScopeCtrl::OnSize(UINT nType, int cx, int cy)
{
	if (!cx && !cy)
		return;

	int iTrend;
	CWnd::OnSize(nType, cx, cy);
	
	// NOTE: OnSize automatically gets called during the setup of the control
	
	GetClientRect(m_rectClient);
	m_nClientHeight = m_rectClient.Height();
	m_nClientWidth = m_rectClient.Width();
	
	// the "left" coordinate and "width" will be modified in InvalidateCtrl to be based on the width 
	// of the y axis scaling
	m_rectPlot.left = 20;
	m_rectPlot.top = 10;
	m_rectPlot.right = m_rectClient.right - 10;
	m_rectPlot.bottom = m_rectClient.bottom - 3 - (abs(sm_logFontAxis.lfHeight) + 2)*2 - 3;
	
	m_nPlotHeight = m_rectPlot.Height();
	m_nPlotWidth = m_rectPlot.Width();
	
	// set the scaling factor for now, this can be adjusted in the SetRange functions
	for (iTrend = 0; iTrend < m_NTrends; iTrend ++)
		m_PlotData[iTrend].dVerticalFactor = (double)m_nPlotHeight / m_PlotData[iTrend].dRange;
	
	// destroy and recreate the grid bitmap
	CClientDC dc(this);
	if (m_bitmapOldGrid.m_hObject && m_bitmapGrid.GetSafeHandle() && m_dcGrid.GetSafeHdc())
	{
		m_dcGrid.SelectObject(m_bitmapOldGrid.Detach());
		m_bitmapGrid.DeleteObject();
		m_bitmapGrid.CreateCompatibleBitmap(&dc, m_nClientWidth, m_nClientHeight);
		m_bitmapOldGrid.Attach(SelectObject(m_dcGrid, m_bitmapGrid));
	}
	
	// destroy and recreate the plot bitmap
	if (m_bitmapOldPlot.m_hObject && m_bitmapPlot.GetSafeHandle() && m_dcPlot.GetSafeHdc())
	{
		m_dcPlot.SelectObject(m_bitmapOldPlot.Detach());
		m_bitmapPlot.DeleteObject();
		m_bitmapPlot.CreateCompatibleBitmap(&dc, m_nClientWidth, m_nClientHeight);
		m_bitmapOldPlot.Attach(SelectObject(m_dcPlot, m_bitmapPlot));
	}
	
	InvalidateCtrl();
}

void COScopeCtrl::Reset()
{
	// Clear all points
	for (int i = 0; i < m_NTrends; i++)
	{
		m_PlotData[i].dPreviousPosition = 0.0;
		m_PlotData[i].nPrevY = -1;

		for (int iTrend = 0; iTrend < m_NTrends; iTrend++)
			m_PlotData[iTrend].lstPoints.RemoveAll();
	}

	InvalidateCtrl();
}

int COScopeCtrl::ReCreateGraph(void)
{
	for (int i = 0; i < m_NTrends; i++)
	{
		m_PlotData[i].dPreviousPosition = 0.0;
		m_PlotData[i].nPrevY = -1;
	}
	
	double* pAddPoints = new double[m_NTrends];
	POSITION* pPosArray = new POSITION[m_NTrends];
	
	// Try to avoid to call the method AppendPoints() more than necessary
	// Remark: the default size of the list is 1024
	int pointToDraw = m_PlotData[0].lstPoints.GetCount();
	if (pointToDraw > m_nPlotWidth / m_nShiftPixels + 1)
		pointToDraw = m_nPlotWidth / m_nShiftPixels + 1;
	int startIndex = m_PlotData[0].lstPoints.GetCount() - pointToDraw;

	// Prepare to go through the elements on n lists in parallel
	for(int iTrend = 0; iTrend < m_NTrends; iTrend++)
		pPosArray[iTrend] = m_PlotData[iTrend].lstPoints.FindIndex(startIndex);
	
	// We will assume that each trends have the same among of points, so we test only the first iterator
	while (pPosArray[0] != 0)
	{
		for (int iTrend = 0; iTrend < m_NTrends; iTrend++)
			pAddPoints[iTrend] = m_PlotData[iTrend].lstPoints.GetNext(pPosArray[iTrend]);

		// Pass false for new bUseTrendRatio parameter so that graph is recreated correctly...
		AppendPoints(pAddPoints, false, false, false);
	}
	
	delete[] pAddPoints;
	delete[] pPosArray;

	Invalidate();
	return 0;
}

void COScopeCtrl::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == m_nRedrawTimer)
	{
		KillTimer(m_nRedrawTimer);
		m_nRedrawTimer = 0;
		m_bDoUpdate = true;
		ReCreateGraph();
	}

	CWnd::OnTimer(nIDEvent);
}

void COScopeCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDblClk(nFlags, point);

	CWnd* pwndParent = GetParent();
	if (pwndParent)
		pwndParent->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), STN_DBLCLK), (LPARAM)m_hWnd);
}

void COScopeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	CWnd::OnMouseMove(nFlags, point);

	if ((nFlags & MK_LBUTTON) == 0) {
		if (m_uLastMouseFlags & MK_LBUTTON) {
			// Mouse button was released -> explicitly clear the tooltip.
			CWnd* pwndParent = GetParent();
			if (pwndParent)
				pwndParent->SendMessage(UM_OSCOPEPOSITION, 0, (LPARAM)(LPCTSTR)_T(""));
		}
		m_uLastMouseFlags = nFlags;
		return;
	}
	m_uLastMouseFlags = nFlags;

	// If that check is not there, it may lead to 100% CPU usage because Windows (Vista?)
	// keeps sending mouse messages even if the mouse does not move but when the mouse
	// button stays pressed.
	if (point == m_ptLastMousePos)
		return;
	m_ptLastMousePos = point;

	CRect plotRect;
	GetPlotRect(plotRect);
	if (!plotRect.PtInRect(point))
		return;

	CWnd* pwndParent = GetParent();
	if (pwndParent)
	{
		int yValue = -1;
		int plotHeight = plotRect.Height();
		if (plotHeight > 0) {
			int yPixel = plotHeight - (point.y - plotRect.top);
			yValue = (int)(m_PlotData[0].dLowerLimit + yPixel * m_PlotData[0].dRange / plotHeight);
		}

		int mypos = (plotRect.Width() - point.x) + plotRect.left;
		int shownsecs = plotRect.Width() * thePrefs.GetTrafficOMeterInterval();
		float apixel = (float)shownsecs / (float)plotRect.Width();

		CString strInfo;
		DWORD dwTime = (DWORD)(mypos * apixel);
		time_t tNow = time(NULL) - dwTime;
		TCHAR szDate[128];
		_tcsftime(szDate, _countof(szDate), thePrefs.GetDateTimeFormat4Log(), localtime(&tNow));
		strInfo.Format(_T("%s: %u @ %s ") + GetResString(IDS_TIMEBEFORE), m_str.YUnits, yValue, szDate, CastSecondsToLngHM(dwTime));

		pwndParent->SendMessage(UM_OSCOPEPOSITION, 0, (LPARAM)(LPCTSTR)strInfo);
	}
}

void COScopeCtrl::OnSysColorChange()
{
	if (m_bitmapOldGrid.m_hObject)
		m_dcGrid.SelectObject(m_bitmapOldGrid.Detach());
	VERIFY( m_dcGrid.DeleteDC() );

	if (m_bitmapOldPlot.m_hObject)
		m_dcPlot.SelectObject(m_bitmapOldPlot.Detach());
	VERIFY( m_dcPlot.DeleteDC() );

	CWnd::OnSysColorChange();
	InvalidateCtrl(false);
}
