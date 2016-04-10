///////////////////////////////////////////////////////////////////////////////
// class CProgressCtrlX
//
// Author:  Yury Goltsman
// email:   ygprg@go.to
// page:    http://go.to/ygprg
// Copyright © 2000, Yury Goltsman
//
// This code provided "AS IS," without warranty of any kind.
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// version : 1.1
// Added multi-color gradient
// Added filling with brush for background and bar(overrides color settings)
// Added borders attribute
// Added vertical text support
// Added snake mode
// Added reverse mode
// Added dual color for text
// Added text formatting
// Added tied mode for text and rubber bar mode
// Added support for vertical oriented control(PBS_VERTICAL)
// 
// version : 1.0
//
#include "stdafx.h"
#include "ProgressCtrlX.h"
#include "MemDC.h"
#include "DrawGdiX.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CProgressCtrlX

CProgressCtrlX::CProgressCtrlX()
	: m_rcBorders(0,0,0,0)
{
	// Init colors
	m_clrBk = ::GetSysColor(COLOR_3DFACE);
	m_clrTextOnBar = ::GetSysColor(COLOR_CAPTIONTEXT);
	m_clrTextOnBk = ::GetSysColor(COLOR_BTNTEXT);
	
	// set gradient colors
	COLORREF clrStart, clrEnd;
	clrStart = clrEnd = ::GetSysColor(COLOR_ACTIVECAPTION);
/*#if(WINVER >= 0x0500)
	BOOL fGradientCaption = FALSE;
	if(SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &fGradientCaption, 0) &&
	   fGradientCaption)
		clrEnd = ::GetSysColor(COLOR_GRADIENTACTIVECAPTION);
#endif / WINVER >= 0x0500  */
	SetGradientColors(clrStart, clrEnd);

	m_nStep = 10;	// according msdn
	m_nTail = 0;
	m_nTailSize = 40;
	m_pbrBk = m_pbrBar = NULL;
	m_bEmpty = false;
}

BEGIN_MESSAGE_MAP(CProgressCtrlX, CProgressCtrl)
	//{{AFX_MSG_MAP(CProgressCtrlX)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_MESSAGE(PBM_SETBARCOLOR, OnSetBarColor)
	ON_MESSAGE(PBM_SETBKCOLOR, OnSetBkColor)
	ON_MESSAGE(PBM_SETPOS, OnSetPos)
	ON_MESSAGE(PBM_DELTAPOS, OnDeltaPos)
	ON_MESSAGE(PBM_STEPIT, OnStepIt)
	ON_MESSAGE(PBM_SETSTEP, OnSetStep)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressCtrlX message handlers

BOOL CProgressCtrlX::OnEraseBkgnd(CDC* /*pDC*/) 
{
	// TODO: Add your message handler code here and/or call default
	return TRUE; // erase in OnPaint()
}

void CProgressCtrlX::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here
	CDrawInfo info;
	GetClientRect(&info.rcClient);

	// retrieve current position and range
	
	if (!m_bEmpty)
	{
		info.nCurPos = GetPos();
		GetRange(info.nLower, info.nUpper);
		info.dwStyle = GetStyle();
	}
	else
	{
		info.nLower = 0;
		info.nUpper = 0;
		info.nCurPos = 0;
		info.dwStyle = 0;
	}

	
	// Draw to memory DC
	CMemDC memDC(&dc);
	info.pDC = &memDC;
	
	// fill background 
	if(m_pbrBk)
		memDC.FillRect(&info.rcClient, m_pbrBk);
	else
		memDC.FillSolidRect(&info.rcClient, m_clrBk);

	// apply borders
	info.rcClient.DeflateRect(m_rcBorders);
		
	// if current pos is out of range return
	if (info.nCurPos < info.nLower || info.nCurPos > info.nUpper)
		return;

	BOOL fVert = info.dwStyle&PBS_VERTICAL;
	BOOL fSnake = info.dwStyle&PBS_SNAKE;
	BOOL fRubberBar = info.dwStyle&PBS_RUBBER_BAR;

	// calculate visible gradient width
	CRect rcBar(0,0,0,0);
	CRect rcMax(0,0,0,0);
	rcMax.right = fVert ? info.rcClient.Height() : info.rcClient.Width();
	rcBar.right = (int)((float)(info.nCurPos-info.nLower) * rcMax.right / ((info.nUpper-info.nLower == 0) ? 1 : info.nUpper-info.nLower));
	if(fSnake)
		rcBar.left = (int)((float)(m_nTail-info.nLower) * rcMax.right / ((info.nUpper-info.nLower == 0) ? 1 : info.nUpper-info.nLower));
	
	// draw bar
	if(m_pbrBar)
		memDC.FillRect(&ConvertToReal(info, rcBar), m_pbrBar);
	else
		DrawMultiGradient(info, fRubberBar ? rcBar : rcMax, rcBar);

	// Draw text
	DrawText(info, rcMax, rcBar);

	// Do not call CProgressCtrl::OnPaint() for painting messages
}

void CProgressCtrlX::DrawMultiGradient(const CDrawInfo& info, const CRect &rcGrad, const CRect &rcClip)
{
	int nSteps = m_ardwGradColors.GetSize()-1;
	float nWidthPerStep = (float)rcGrad.Width() / nSteps;
	CRect rcGradBand(rcGrad);
	for (int i = 0; i < nSteps; i++) 
	{
		rcGradBand.left = rcGrad.left + (int)(nWidthPerStep * i);
		rcGradBand.right = rcGrad.left + (int)(nWidthPerStep * (i+1));
		if(i == nSteps-1)	//last step (because of problems with float)
			rcGradBand.right = rcGrad.right;

		if(rcGradBand.right < rcClip.left)
			continue; // skip - band before cliping rect
		
		CRect rcClipBand(rcGradBand);
		if(rcClipBand.left < rcClip.left)
			rcClipBand.left = rcClip.left;
		if(rcClipBand.right > rcClip.right)
			rcClipBand.right = rcClip.right;

		DrawGradient(info, rcGradBand, rcClipBand, m_ardwGradColors[i], m_ardwGradColors[i+1]);

		if(rcClipBand.right == rcClip.right)
			break; // stop filling - next band is out of clipping rect
	}
}

void CProgressCtrlX::DrawGradient(const CDrawInfo& info, const CRect &rcGrad, const CRect &rcClip, COLORREF clrStart, COLORREF clrEnd)
{
	// Split colors to RGB chanels, find chanel with maximum difference 
	// between the start and end colors. This distance will determine 
	// number of steps of gradient
	int r = (GetRValue(clrEnd) - GetRValue(clrStart));
	int g = (GetGValue(clrEnd) - GetGValue(clrStart));
	int b = (GetBValue(clrEnd) - GetBValue(clrStart));
	int nSteps = max(abs(r), max(abs(g), abs(b)));
	// if number of pixels in gradient less than number of steps - 
	// use it as numberof steps
	int nPixels = rcGrad.Width();
	nSteps = min(nPixels, nSteps);
	if(nSteps == 0) nSteps = 1;

	float rStep = (float)r/nSteps;
	float gStep = (float)g/nSteps;
	float bStep = (float)b/nSteps;

	r = GetRValue(clrStart);
	g = GetGValue(clrStart);
	b = GetBValue(clrStart);

	BOOL fLowColor = info.pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE;
	if(!fLowColor && nSteps > 1)
		if(info.pDC->GetDeviceCaps(BITSPIXEL)*info.pDC->GetDeviceCaps(PLANES) < 8)
			nSteps = 1; // for 16 colors no gradient

	float nWidthPerStep = (float)rcGrad.Width() / nSteps;
	CRect rcFill(rcGrad);
	CBrush br;
	// Start filling
	for (int i = 0; i < nSteps; i++) 
	{
		rcFill.left = rcGrad.left + (int)(nWidthPerStep * i);
		rcFill.right = rcGrad.left + (int)(nWidthPerStep * (i+1));
		if(i == nSteps-1)	//last step (because of problems with float)
			rcFill.right = rcGrad.right;

		if(rcFill.right < rcClip.left)
			continue; // skip - band before cliping rect
		
		// clip it
		if(rcFill.left < rcClip.left)
			rcFill.left = rcClip.left;
		if(rcFill.right > rcClip.right)
			rcFill.right = rcClip.right;

		COLORREF clrFill = RGB(r + (int)(i * rStep),
		                       g + (int)(i * gStep),
		                       b + (int)(i * bStep));
		if(fLowColor)
		{
			br.CreateSolidBrush(clrFill);
			// CDC::FillSolidRect is faster, but it does not handle 8-bit color depth
			info.pDC->FillRect(&ConvertToReal(info, rcFill), &br);
			br.DeleteObject();
		}
		else
			info.pDC->FillSolidRect(&ConvertToReal(info, rcFill), clrFill);
		if(rcFill.right >= rcClip.right)
			break; // stop filling if we reach current position
	}
}

void CProgressCtrlX::DrawText(const CDrawInfo& info, const CRect &rcMax, const CRect &rcBar)
{
	if(!(info.dwStyle&PBS_TEXTMASK))
		return;
	BOOL fVert = info.dwStyle&PBS_VERTICAL;
	CDC *pDC = info.pDC;
	int nValue = 0;
	CString sFormat;
	GetWindowText(sFormat);
	switch(info.dwStyle&PBS_TEXTMASK)
	{
		case PBS_SHOW_PERCENT:
			if(sFormat.IsEmpty())
				sFormat = _T("%d%%");
			// retrieve current position and range
			nValue = (int)((float)(info.nCurPos-info.nLower) * 100 / ((info.nUpper-info.nLower == 0) ? 1 : info.nUpper-info.nLower));
			break;
		case PBS_SHOW_POSITION:
			if(sFormat.IsEmpty())
				sFormat = _T("%d");
			// retrieve current position
			nValue = info.nCurPos;
			break;
	}

	if (sFormat.IsEmpty())
		return;

	CFont* pFont = GetFont();
	CSelFont sf(pDC, pFont);
	CSelTextColor tc(pDC, m_clrTextOnBar);
	CSelBkMode bm(pDC, TRANSPARENT);
	CSelTextAlign	ta(pDC, TA_BOTTOM|TA_CENTER);
  CPoint ptOrg = pDC->GetWindowOrg();
	CString sText;
	sText.Format(sFormat, nValue);
	
	LONG grad = 0;
	if(pFont)
	{
		LOGFONT lf;
		pFont->GetLogFont(&lf);
		grad = lf.lfEscapement/10;
	}
	int x = 0, y = 0, dx = 0, dy = 0;
	CSize sizText = pDC->GetTextExtent(sText);
	if(grad == 0)         {	x = sizText.cx; y = sizText.cy; dx = 0; dy = sizText.cy;}
	else if(grad == 90)   {	x = sizText.cy; y = sizText.cx; dx = sizText.cy; dy = 0;}
	else if(grad == 180)  {	x = sizText.cx; y = sizText.cy; dx = 0; dy = -sizText.cy;}
	else if(grad == 270)  {	x = sizText.cy; y = sizText.cx; dx = -sizText.cy; dy = 0;}
	else ASSERT(0); // angle not supported
	CPoint pt = pDC->GetViewportOrg();
	if(info.dwStyle&PBS_TIED_TEXT)
	{
		CRect rcFill(ConvertToReal(info, rcBar));
		if((fVert ? y : x) <= rcBar.Width())
		{
			pDC->SetViewportOrg(rcFill.left + (rcFill.Width() + dx)/2, 
													rcFill.top + (rcFill.Height() + dy)/2);
			DrawClippedText(info, rcBar, sText, ptOrg);
		}
	}
	else
	{
		pDC->SetViewportOrg(info.rcClient.left + (info.rcClient.Width() + dx)/2, 
												info.rcClient.top + (info.rcClient.Height() + dy)/2);
		if(m_clrTextOnBar == m_clrTextOnBk)
			// if the same color for bar and background draw text once
			DrawClippedText(info, rcMax, sText, ptOrg);
		else
		{	
			// else, draw clipped parts of text
			
			// draw text on gradient
			if(rcBar.left != rcBar.right)
				DrawClippedText(info, rcBar, sText, ptOrg);

			// draw text out of gradient
			if(rcMax.right > rcBar.right)
			{
				tc.Select(m_clrTextOnBk);
				CRect rc(rcMax);
				rc.left = rcBar.right;
				DrawClippedText(info, rc, sText, ptOrg);
			}
			if(rcMax.left < rcBar.left)
			{
				tc.Select(m_clrTextOnBk);
				CRect rc(rcMax);
				rc.right = rcBar.left;
				DrawClippedText(info, rc, sText, ptOrg);
			}
		}
	}
	pDC->SetViewportOrg(pt);
}

void CProgressCtrlX::DrawClippedText(const CDrawInfo& info, const CRect& rcClip, CString& sText, const CPoint& ptWndOrg)
{
	CDC *pDC = info.pDC;
	CRgn rgn;
	CRect rc = ConvertToReal(info, rcClip);
	rc.OffsetRect(-ptWndOrg);
	rgn.CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
	pDC->SelectClipRgn(&rgn);
	pDC->TextOut (0, 0, sText);
	rgn.DeleteObject();
}

LRESULT CProgressCtrlX::OnSetBarColor(WPARAM clrEnd, LPARAM clrStart)
{
	SetGradientColors(clrStart, clrEnd ? clrEnd : clrStart);

	return CLR_DEFAULT;
}

LRESULT CProgressCtrlX::OnSetBkColor(WPARAM, LPARAM clrBk)
{
	m_clrBk = clrBk;
	return CLR_DEFAULT;
}

LRESULT CProgressCtrlX::OnSetStep(WPARAM nStepInc, LPARAM)
{
	m_nStep = nStepInc;
	return Default();
}

LRESULT CProgressCtrlX::OnSetPos(WPARAM newPos, LPARAM)
{
	int nOldPos;
	if(SetSnakePos(nOldPos, newPos))
		return nOldPos;

	return Default();
}

LRESULT CProgressCtrlX::OnDeltaPos(WPARAM nIncrement, LPARAM)
{
	int nOldPos;
	if(SetSnakePos(nOldPos, nIncrement, TRUE))
		return nOldPos;

	return Default();
}

LRESULT CProgressCtrlX::OnStepIt(WPARAM, LPARAM)
{
	int nOldPos;
	if(SetSnakePos(nOldPos, m_nStep, TRUE))
		return nOldPos;

	return Default();
}

/////////////////////////////////////////////////////////////////////////////
// CProgressCtrlX implementation

BOOL CProgressCtrlX::SetSnakePos(int& nOldPos, int nNewPos, BOOL fIncrement)
{
	DWORD dwStyle = GetStyle();
	if(!(dwStyle&PBS_SNAKE))
		return FALSE;
	
	int nLower, nUpper;
	GetRange(nLower, nUpper);
	if(fIncrement)
	{
		int nCurPos = GetPos();
		if(nCurPos == nUpper && nCurPos - m_nTail < m_nTailSize )
			nCurPos = m_nTail + m_nTailSize;
		nNewPos = nCurPos + abs(nNewPos);
	}
	if(nNewPos > nUpper+m_nTailSize)
	{
		nNewPos -= nUpper-nLower + m_nTailSize;
		if(nNewPos > nUpper + m_nTailSize)
		{
			ASSERT(0); // too far - reset
			nNewPos = nUpper + m_nTailSize;
		}
		if(dwStyle&PBS_REVERSE)
			ModifyStyle(PBS_REVERSE, 0);
		else
			ModifyStyle(0, PBS_REVERSE);
	}
	else if(nNewPos >= nUpper)
		Invalidate();
	
	m_nTail = nNewPos - m_nTailSize;
	if(m_nTail < nLower)
		m_nTail = nLower;

	nOldPos = DefWindowProc(PBM_SETPOS, nNewPos, 0);
	return TRUE;
}

void CProgressCtrlX::SetTextFormat(LPCTSTR szFormat, DWORD ffFormat)
{
	ASSERT(::IsWindow(m_hWnd));
	
	if(!szFormat || !szFormat[0] || !ffFormat)
	{
		ModifyStyle(PBS_TEXTMASK, 0);
		SetWindowText(_T(""));
	}
	else
	{
		ModifyStyle(PBS_TEXTMASK, ffFormat);
		SetWindowText(szFormat);
	}
}

CRect CProgressCtrlX::ConvertToReal(const CDrawInfo& info, const CRect& rcVirt)
{
	BOOL fReverse = info.dwStyle&PBS_REVERSE;
	BOOL fVert = info.dwStyle&PBS_VERTICAL;

	CRect rc(info.rcClient);
	if(fVert)
	{
		rc.top = info.rcClient.top + 
		         (fReverse ? rcVirt.left : (info.rcClient.Height() - rcVirt.right));
		rc.bottom = rc.top + rcVirt.Width();
	}
	else
	{
		rc.left = info.rcClient.left + 
		          (fReverse ? (info.rcClient.Width() - rcVirt.right) : rcVirt.left);
		rc.right = rc.left + rcVirt.Width();
	}
	return rc;
}

void CProgressCtrlX::SetGradientColorsX(int nCount, COLORREF clrFirst, COLORREF clrNext, ...)
{ 
	m_ardwGradColors.SetSize(nCount); 
	
	m_ardwGradColors.SetAt(0, clrFirst); 
	m_ardwGradColors.SetAt(1, clrNext);  

  va_list pArgs;
  va_start(pArgs, clrNext);
	for(int i = 2; i < nCount; i++)
		m_ardwGradColors.SetAt(i, va_arg(pArgs, COLORREF));
	va_end( pArgs );
}
