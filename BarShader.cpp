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
#include "barshader.h"
#include "Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Why does _USE_MATH_DEFINES work in debug builds, but not in release builds??
#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#define HALF(X) (((X) + 1) / 2)

CBarShader::CBarShader(uint32 height, uint32 width) {
	m_iWidth = width;
	m_iHeight = height;
	m_uFileSize = (uint64)1;
	m_Spans.SetAt(0, 0);	// SLUGFILLER: speedBarShader
	m_Modifiers = NULL;
	m_bIsPreview=false;
}

CBarShader::~CBarShader(void) {
	delete[] m_Modifiers;
}

void CBarShader::Reset() {
	Fill(0);
}

void CBarShader::BuildModifiers() {
	delete[] m_Modifiers;
	m_Modifiers = NULL; // 'new' may throw an exception

	if (!m_bIsPreview) 
		m_used3dlevel=thePrefs.Get3DDepth();

	// Barry - New property page slider to control depth of gradient

	// Depth must be at least 2
	// 2 gives greatest depth, the higher the value, the flatter the appearance
	// m_Modifiers[count-1] will always be 1, m_Modifiers[0] depends on the value of depth
	
	int depth = (7-m_used3dlevel);
	int count = HALF(m_iHeight);
	double piOverDepth = M_PI/depth;
	double base = piOverDepth * ((depth / 2.0) - 1);
	double increment = piOverDepth / (count - 1);

	m_Modifiers = new float[count];
	for (int i = 0; i < count; i++)
		m_Modifiers[i] = (float)(sin(base + i * increment));
}

void CBarShader::SetWidth(int width) {
	if(m_iWidth != width) {
		m_iWidth = width;
		if (m_uFileSize > (uint64)0)
			m_dPixelsPerByte = (double)m_iWidth / (uint64)m_uFileSize;
		else
			m_dPixelsPerByte = 0.0;
		if (m_iWidth)
			m_dBytesPerPixel = (double)m_uFileSize / m_iWidth;
		else
			m_dBytesPerPixel = 0.0;
	}
}

void CBarShader::SetFileSize(EMFileSize fileSize) {
	if(m_uFileSize != fileSize) {
		m_uFileSize = fileSize;

		if (m_uFileSize > (uint64)0)
			m_dPixelsPerByte = (double)m_iWidth / (uint64)m_uFileSize;
		else
			m_dPixelsPerByte = 0.0;

		if (m_iWidth)
			m_dBytesPerPixel = (double)m_uFileSize / m_iWidth;
		else
			m_dBytesPerPixel = 0.0;
	}
}

void CBarShader::SetHeight(int height) {
	if(m_iHeight != height) {
		m_iHeight = height;

		BuildModifiers();
	}
}

void CBarShader::FillRange(uint64 start, uint64 end, COLORREF color) {
	if(end > m_uFileSize)
		end = m_uFileSize;

	if(start >= end)
		return;

	// SLUGFILLER: speedBarShader
	POSITION endpos = m_Spans.FindFirstKeyAfter(end+1);

	if (endpos)
		m_Spans.GetPrev(endpos);
	else
		endpos = m_Spans.GetTailPosition();

	ASSERT(endpos != NULL);

	COLORREF endcolor = m_Spans.GetValueAt(endpos);
	endpos = m_Spans.SetAt(end, endcolor);

	for (POSITION pos = m_Spans.FindFirstKeyAfter(start+1); pos != NULL && pos != endpos; ) {
		POSITION pos1 = pos;
		m_Spans.GetNext(pos);
		m_Spans.RemoveAt(pos1);
	}
	
	m_Spans.GetPrev(endpos);

	if (m_Spans.GetValueAt(endpos) != color)
		m_Spans.SetAt(start, color);
	// SLUGFILLER: speedBarShader
}

void CBarShader::Fill(COLORREF color) {
	// SLUGFILLER: speedBarShader
	m_Spans.RemoveAll();
	m_Spans.SetAt(0, color);
	m_Spans.SetAt(m_uFileSize, 0);
	// SLUGFILLER: speedBarShader
}

void CBarShader::Draw(CDC* dc, int iLeft, int iTop, bool bFlat) {
	POSITION pos = m_Spans.GetHeadPosition();	// SLUGFILLER: speedBarShader
	RECT rectSpan;
	rectSpan.top = iTop;
	rectSpan.bottom = iTop + m_iHeight;
	rectSpan.right = iLeft;

	uint64 uBytesInOnePixel = (uint64)(m_dBytesPerPixel + 0.5f);
	uint64 start = 0;//bsCurrent->start;
	// SLUGFILLER: speedBarShader
	COLORREF color = m_Spans.GetValueAt(pos);
	m_Spans.GetNext(pos);
	// SLUGFILLER: speedBarShader
	while(pos != NULL && rectSpan.right < (iLeft + m_iWidth)) {	// SLUGFILLER: speedBarShader
		uint64 uSpan = m_Spans.GetKeyAt(pos) - start;	// SLUGFILLER: speedBarShader
		uint64 uPixels = (uint64)(uSpan * m_dPixelsPerByte + 0.5f);
		if (uPixels > 0) {
			rectSpan.left = rectSpan.right;
			rectSpan.right += (int)uPixels;
			FillRect(dc, &rectSpan, color, bFlat);	// SLUGFILLER: speedBarShader
			start += (uint64)(uPixels * m_dBytesPerPixel + 0.5f);
		} else {
			float fRed = 0;
			float fGreen = 0;
			float fBlue = 0;
			uint64 iEnd = start + uBytesInOnePixel;
			uint64 iLast = start;
			// SLUGFILLER: speedBarShader
			do {
				float fWeight = (float)((min(m_Spans.GetKeyAt(pos), iEnd) - iLast) * m_dPixelsPerByte);
				fRed   += GetRValue(color) * fWeight;
				fGreen += GetGValue(color) * fWeight;
				fBlue  += GetBValue(color) * fWeight;
				if(m_Spans.GetKeyAt(pos) > iEnd)
					break;
				iLast = m_Spans.GetKeyAt(pos);
				color = m_Spans.GetValueAt(pos);
				m_Spans.GetNext(pos);
			} while(pos != NULL);
			// SLUGFILLER: speedBarShader
			rectSpan.left = rectSpan.right;
			rectSpan.right++;
			if (g_bLowColorDesktop)
				FillRect(dc, &rectSpan, color, bFlat);
			else
				FillRect(dc, &rectSpan, fRed, fGreen, fBlue, bFlat);
			start += uBytesInOnePixel;
		}
		// SLUGFILLER: speedBarShader
		while(pos != NULL && m_Spans.GetKeyAt(pos) < start) {
			color = m_Spans.GetValueAt(pos);
			m_Spans.GetNext(pos);
		}
		// SLUGFILLER: speedBarShader
	}
}

void CBarShader::FillRect(CDC *dc, LPRECT rectSpan, COLORREF color, bool bFlat) {
	if(!color || bFlat)
		dc->FillRect(rectSpan, &CBrush(color));
	else
		FillRect(dc, rectSpan, GetRValue(color), GetGValue(color), GetBValue(color), false);
}

void CBarShader::FillRect(CDC *dc, LPRECT rectSpan, float fRed, float fGreen,
						  float fBlue, bool bFlat) {
	if(bFlat) {
		COLORREF color = RGB((int)(fRed + .5f), (int)(fGreen + .5f), (int)(fBlue + .5f));
		dc->FillRect(rectSpan, &CBrush(color));
	} else {
		if (m_Modifiers == NULL || (m_used3dlevel!=thePrefs.Get3DDepth() && !m_bIsPreview) )
			BuildModifiers();
		RECT rect = *rectSpan;
		int iTop = rect.top;
		int iBot = rect.bottom;
		int iMax = HALF(m_iHeight);
		for(int i = 0; i < iMax; i++) {
			CBrush cbNew(RGB((int)(fRed * m_Modifiers[i] + .5f), (int)(fGreen * m_Modifiers[i] + .5f), (int)(fBlue * m_Modifiers[i] + .5f)));
			
			rect.top = iTop + i;
			rect.bottom = iTop + i + 1;
			dc->FillRect(&rect, &cbNew);

			rect.top = iBot - i - 1;
			rect.bottom = iBot - i;
			dc->FillRect(&rect, &cbNew);
		}
	}
}

void CBarShader::DrawPreview(CDC* dc, int iLeft, int iTop, UINT previewLevel)		//Cax2 aqua bar
{
	m_bIsPreview=true;
	m_used3dlevel = previewLevel;
	BuildModifiers();
	Draw( dc, iLeft, iTop, (previewLevel==0));
	m_bIsPreview=false;
}
