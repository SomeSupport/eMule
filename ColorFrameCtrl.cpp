#include "stdafx.h"
#include "ColorFrameCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl

BEGIN_MESSAGE_MAP(CColorFrameCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CColorFrameCtrl::CColorFrameCtrl()
{
	m_crBackColor = RGB(0, 0, 0);  // see also SetBackgroundColor
	m_crFrameColor = RGB(0, 255, 255);  // see also SetFrameColor

	m_brushBack.CreateSolidBrush(m_crBackColor);
	m_brushFrame.CreateSolidBrush(m_crFrameColor);
}

CColorFrameCtrl::~CColorFrameCtrl()
{
	m_brushFrame.DeleteObject();
	m_brushBack.DeleteObject();
}

BOOL CColorFrameCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) 
{
	BOOL result;
	static CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, AfxGetApp()->LoadStandardCursor(IDC_ARROW));

	result = CWnd::CreateEx( WS_EX_STATICEDGE, 
		                      className, NULL, dwStyle, 
		                      rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
		                      pParentWnd->GetSafeHwnd(), (HMENU)nID);
	if (result != 0)
		Invalidate();
	return result;
}

void CColorFrameCtrl::SetFrameColor( COLORREF color )
{
	m_crFrameColor = color;
	m_brushFrame.DeleteObject();
	m_brushFrame.CreateSolidBrush(m_crFrameColor);

	// clear out the existing garbage, re-start with a clean plot
	Invalidate();
}

void CColorFrameCtrl::SetBackgroundColor(COLORREF color)
{
	m_crBackColor = color;

	m_brushBack.DeleteObject();
	m_brushBack.CreateSolidBrush(m_crBackColor);

	// clear out the existing garbage, re-start with a clean plot
	Invalidate();
}

void CColorFrameCtrl::OnPaint() 
{
	CPaintDC dc(this);  // device context for painting

	dc.FillRect(m_rectClient, &m_brushBack);
	dc.FrameRect(m_rectClient, &m_brushFrame);
}

void CColorFrameCtrl::OnSize(UINT nType, int cx, int cy) 
{
	// NOTE: OnSize automatically gets called during the setup of the control
	CWnd::OnSize(nType, cx, cy);
	GetClientRect(m_rectClient);
}
