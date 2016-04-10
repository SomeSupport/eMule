// Created: 04/02/2001 {mm/dm/yyyyy}
// Written by: Anish Mistry http://am-productions.yi.org/
/* This code is licensed under the GNU GPL.  See License.txt or (http://www.gnu.org/copyleft/gpl.html). */
#include "stdafx.h"
#include "MeterIcon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMeterIcon::CMeterIcon()
{
	m_nNumBars = 2;
	m_sDimensions.cx = 16;
	m_sDimensions.cy = 16;
	m_nMaxVal = 100;
	m_nSpacingWidth = 0;
	m_hFrame = NULL;
	m_bInit = false;
	m_crBorderColor = RGB(0,0,0);
	m_pLimits = NULL;
	m_pColors = NULL;
	m_nEntries = 0;
}

CMeterIcon::~CMeterIcon()
{
	// free color list memory
	delete[] m_pLimits;
	delete[] m_pColors;
}

COLORREF CMeterIcon::GetMeterColor(int nLevel) const
// it the nLevel is greater than the values defined in m_pLimits the last value in the array is used
{// begin GetMeterColor
	for(int i = 0;i < m_nEntries;i++)
	{
		if(nLevel <= m_pLimits[i])
		{
			return m_pColors[i];
		}
	}
	// default to the last entry
	return m_pColors[m_nEntries-1];
}// end GetMeterColor

HICON CMeterIcon::CreateMeterIcon(const int *pBarData)
// the returned icon must be cleaned up using DestroyIcon()
{// begin CreateMeterIcon
	ICONINFO iiNewIcon={0};
	iiNewIcon.fIcon = true;	// set that it is an icon

	// create DC's
	HDC hScreenDC = ::GetDC(HWND_DESKTOP);
	HDC hIconDC = CreateCompatibleDC(hScreenDC);
	HDC hMaskDC = CreateCompatibleDC(hScreenDC);

	if(hScreenDC == NULL)
	{// begin error check
		return NULL;
	}// end error check
	if(hIconDC == NULL)
	{// begin error check
		return NULL;
	}// end error check
	if(hMaskDC == NULL)
	{// begin error check
		return NULL;
	}// end error check
	
	// load bitmaps
	iiNewIcon.hbmColor = CreateCompatibleBitmap(hScreenDC,m_sDimensions.cx,m_sDimensions.cy);
	if(iiNewIcon.hbmColor == NULL)
	{// begin error check
		return NULL;
	}// end error check
	if(!::ReleaseDC(NULL,hScreenDC))	// release this ASAP
	{// begin DC not released
		return NULL;
	}// end DC not released
	iiNewIcon.hbmMask = CreateCompatibleBitmap(hMaskDC,m_sDimensions.cx,m_sDimensions.cy);
	if(iiNewIcon.hbmMask == NULL)
	{// begin error check
		return NULL;
	}// end error check
	HGDIOBJ hOldIconDC = ::SelectObject(hIconDC,iiNewIcon.hbmColor);
	if(hOldIconDC == NULL)
	{// begin error check
		return NULL;
	}// end error check
	HGDIOBJ hOldMaskDC = ::SelectObject(hMaskDC,iiNewIcon.hbmMask);
	if(hOldMaskDC == NULL)
	{// begin error check
		return NULL;
	}// end error check

	// initilize the bitmaps
	if(!BitBlt(hIconDC,0,0,m_sDimensions.cx,m_sDimensions.cy,NULL,0,0,BLACKNESS))
	{// begin BitBlt failed
		return NULL;
	}// end BitBlt failed
	if(!BitBlt(hMaskDC,0,0,m_sDimensions.cx,m_sDimensions.cy,NULL,0,0,WHITENESS))
	{// begin BitBlt failed
		return NULL;
	}// end BitBlt failed

	// draw the meters
	for(int i = 0;i < m_nNumBars;i++)
		if(DrawIconMeter(hIconDC,hMaskDC,pBarData[i],i) == false)
			return false;

	if(!DrawIconEx(hIconDC,0,0,m_hFrame,m_sDimensions.cx,m_sDimensions.cy,NULL,NULL,DI_NORMAL|DI_IMAGE))
	{// begin error check
		return NULL;
	}// end error check
	if(!DrawIconEx(hMaskDC,0,0,m_hFrame,m_sDimensions.cx,m_sDimensions.cy,NULL,NULL,DI_NORMAL|DI_MASK))
	{// begin error check
		return NULL;
	}// end error check

	// create icon
	::SelectObject(hIconDC,hOldIconDC);
	::SelectObject(hMaskDC,hOldMaskDC);
	HICON hNewIcon = CreateIconIndirect(&iiNewIcon);

	// cleanup
	DeleteObject(iiNewIcon.hbmColor);
	DeleteObject(iiNewIcon.hbmMask);
	DeleteDC(hMaskDC);
	DeleteDC(hIconDC);
	return hNewIcon;

}// end CreateMeterIcon

bool CMeterIcon::DrawIconMeter(HDC hDestDC, HDC hDestDCMask, int nLevel, int nPos)
{// begin DrawIconMeter
	// draw meter
	HBRUSH hBrush = CreateSolidBrush(GetMeterColor(nLevel));
	if(hBrush == NULL)
	{// begin error check
		return false;
	}// end error check
	HGDIOBJ hOldBrush = SelectObject(hDestDC,hBrush);
	if(hOldBrush == NULL)
	{// begin error check
		return false;
	}// end error check
	HPEN hPen = CreatePen(PS_SOLID,1,m_crBorderColor);
	if(hPen == NULL)
		return false;
	HGDIOBJ hOldPen = SelectObject(hDestDC,hPen);
	if(hOldPen == NULL)
		return false;
	if(!Rectangle(hDestDC,((m_sDimensions.cx-1)/m_nNumBars)*nPos+m_nSpacingWidth,m_sDimensions.cy-((nLevel*(m_sDimensions.cy-1)/m_nMaxVal)+1),((m_sDimensions.cx-1)/m_nNumBars)*(nPos+1)+1,m_sDimensions.cy))
	{// begin error check
		return false;
	}// end error check
	if(!DeleteObject(SelectObject(hDestDC,hOldPen)))
	{// begin error check
		return false;
	}// end error check
	if(!DeleteObject(SelectObject(hDestDC,hOldBrush)))
	{// begin error check
		return false;
	}// end error check

	// draw meter mask
	HBRUSH hDestDCMaskBrush = CreateSolidBrush(RGB(0,0,0));
	if(hDestDCMaskBrush == NULL)
	{// begin error check
		return false;
	}// end error check
	HGDIOBJ hOldDestDCMaskBrush = SelectObject(hDestDCMask,hDestDCMaskBrush);
	if(hOldDestDCMaskBrush == NULL)
	{// begin error check
		return false;
	}// end error check
	HPEN hMaskPen = CreatePen(PS_SOLID,1,RGB(0,0,0));
	if(hMaskPen == NULL)
		return false;
	HGDIOBJ hOldMaskPen = SelectObject(hDestDCMask,hMaskPen);
	if(hOldMaskPen == NULL)
		return false;

	if (nLevel>0) 
	if(!Rectangle(hDestDCMask,
		m_sDimensions.cx-2,
		m_sDimensions.cy-((nLevel*(m_sDimensions.cy-1)/m_nMaxVal)+1),
		m_sDimensions.cx,
		m_sDimensions.cy))
	{// begin error check
		return false;
	}// end error check
	if(!DeleteObject(SelectObject(hDestDCMask,hOldMaskPen)))
	{// begin error check
		return false;
	}// end error check
	if(!DeleteObject(SelectObject(hDestDCMask,hOldDestDCMaskBrush)))
	{// begin error check
		return false;
	}// end error check

	return true;
}// end DrawIconMeter


HICON CMeterIcon::SetFrame(HICON hIcon)
// return the old frame icon
{// begin SetFrame
	HICON hOld = m_hFrame;
	m_hFrame = hIcon;
	return hOld;
}// end SetFrame

HICON CMeterIcon::Create(const int *pBarData)
// must call init once before calling
{// begin Create
	if(!m_bInit)
		return NULL;
	return CreateMeterIcon(pBarData);
}// end Create

bool CMeterIcon::Init(HICON hFrame, int nMaxVal, int nNumBars, int nSpacingWidth, int nWidth, int nHeight, COLORREF crColor)
// nWidth & nHeight are the dimensions of the icon that you want created
// nSpacingWidth is the space between the bars
// hFrame is the overlay for the bars
// crColor is the outline color for the bars
{// begin Init
	SetFrame(hFrame);
	SetWidth(nSpacingWidth);
	SetMaxValue(nMaxVal);
	SetDimensions(nWidth,nHeight);
	SetNumBars(nNumBars);
	SetBorderColor(crColor);
	m_bInit = true;
	return m_bInit;
}// end Init

SIZE CMeterIcon::SetDimensions(int nWidth, int nHeight)
// return the previous dimension
{// begin SetDimensions
	SIZE sOld = m_sDimensions;
	m_sDimensions.cx = nWidth;
	m_sDimensions.cy = nHeight;
	return sOld;
}// end SetDimensions

int CMeterIcon::SetNumBars(int nNum)
{// begin SetNumBars
	int nOld = m_nNumBars;
	m_nNumBars = nNum;
	return nOld;
}// end SetNumBars

int CMeterIcon::SetWidth(int nWidth)
{// begin SetWidth
	int nOld = m_nSpacingWidth;
	m_nSpacingWidth = nWidth;
	return nOld;
}// end SetWidth

int CMeterIcon::SetMaxValue(int nVal)
{// begin SetMaxValue
	int nOld = m_nMaxVal;
	m_nMaxVal = nVal;
	return nOld;
}// end SetMaxValue

COLORREF CMeterIcon::SetBorderColor(COLORREF crColor)
{// begin SetBorderColor
	COLORREF crOld = m_crBorderColor;
	m_crBorderColor = crColor;
	return crOld;
}// end SetBorderColor

bool CMeterIcon::SetColorLevels(const int* pLimits, const COLORREF* pColors, int nEntries)
// pLimits is an array of int that contain the upper limit for the corresponding color
{// begin SetColorLevels
	// free exsisting memory
	delete[] m_pLimits;
	m_pLimits = NULL; // 'new' may throw an exception
	
	delete[] m_pColors;
	m_pColors = NULL; // 'new' may throw an exception

	// allocate new memory
	m_pLimits = new int[nEntries];
	m_pColors = new COLORREF[nEntries];
	// copy values
	for(int i = 0;i < nEntries;i++)
	{// begin copy
		m_pLimits[i] = pLimits[i];
		m_pColors[i] = pColors[i];
	}// end copy
	m_nEntries = nEntries;
	return true;
}// end SetColorLevels
