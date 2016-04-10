#pragma once

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl window

class CColorFrameCtrl : public CWnd
{
public:
	CColorFrameCtrl();
	virtual ~CColorFrameCtrl();

	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0);

	void SetFrameColor(COLORREF color);
	void SetBackgroundColor(COLORREF color);

	COLORREF m_crBackColor;        // background color
	COLORREF m_crFrameColor;       // frame color

protected:
	CRect  m_rectClient;
	CBrush m_brushBack;
	CBrush m_brushFrame;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy); 
};
