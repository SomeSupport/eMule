//***************************************************************************
//
// AUTHOR:  James White (feel free to remove or otherwise mangle any part)
//
//***************************************************************************
#include "stdafx.h"
#include "ColorButton.h"
#include "UserMsgs.h"

//***********************************************************************
//**                         MFC Debug Symbols                         **
//***********************************************************************
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//***********************************************************************
//**                            DDX Method                            **
//***********************************************************************

void AFXAPI DDX_ColorButton(CDataExchange *pDX, int nIDC, COLORREF& crColour)
{
    HWND hWndCtrl = pDX->PrepareCtrl(nIDC);
    ASSERT (hWndCtrl != NULL);                
    
    CColorButton* pColourButton = (CColorButton*) CWnd::FromHandle(hWndCtrl);
    if (pDX->m_bSaveAndValidate)
    {
		crColour = pColourButton->Color;
    }
    else // initializing
    {
		pColourButton->Color = crColour;
    }
}

//***********************************************************************
//**                             Constants                             **
//***********************************************************************
const int g_ciArrowSizeX = 4 ;
const int g_ciArrowSizeY = 2 ;

//***********************************************************************
//**                            MFC Macros                            **
//***********************************************************************
IMPLEMENT_DYNCREATE(CColorButton, CButton)

//***********************************************************************
// Method:	CColorButton::CColorButton(void)
// Notes:	Default Constructor.
//***********************************************************************
CColorButton::CColorButton(void):
	_Inherited(),
	m_Color(CLR_DEFAULT),
	m_DefaultColor(::GetSysColor(COLOR_APPWORKSPACE)),
	m_strDefaultText(_T("Automatic")),
	m_strCustomText(_T("More Colors...")),
	m_bPopupActive(FALSE),
	m_bTrackSelection(FALSE)
{
}

//***********************************************************************
// Method:	CColorButton::~CColorButton(void)
// Notes:	Destructor.
//***********************************************************************
CColorButton::~CColorButton(void)
{
}

//***********************************************************************
// Method:	CColorButton::GetColor()
// Notes:	None.
//***********************************************************************
COLORREF CColorButton::GetColor(void) const
{
	return m_Color;
}


//***********************************************************************
// Method:	CColorButton::SetColor()
// Notes:	None.
//***********************************************************************
void CColorButton::SetColor(COLORREF Color)
{
	m_Color = Color;

	if (::IsWindow(m_hWnd)) 
        RedrawWindow();
}


//***********************************************************************
// Method:	CColorButton::GetDefaultColor()
// Notes:	None.
//***********************************************************************
COLORREF CColorButton::GetDefaultColor(void) const
{
	return m_DefaultColor;
}

//***********************************************************************
// Method:	CColorButton::SetDefaultColor()
// Notes:	None.
//***********************************************************************
void CColorButton::SetDefaultColor(COLORREF Color)
{
	m_DefaultColor = Color;
}

//***********************************************************************
// Method:	CColorButton::SetCustomText()
// Notes:	None.
//***********************************************************************
void CColorButton::SetCustomText(LPCTSTR tszText)
{
	m_strCustomText = tszText;
}

//***********************************************************************
// Method:	CColorButton::SetDefaultText()
// Notes:	None.
//***********************************************************************
void CColorButton::SetDefaultText(LPCTSTR tszText)
{
	m_strDefaultText = tszText;
}


//***********************************************************************
// Method:	CColorButton::SetTrackSelection()
// Notes:	None.
//***********************************************************************
void CColorButton::SetTrackSelection(BOOL bTrack)
{
	m_bTrackSelection = bTrack;
}

//***********************************************************************
// Method:	CColorButton::GetTrackSelection()
// Notes:	None.
//***********************************************************************
BOOL CColorButton::GetTrackSelection(void) const
{
	return m_bTrackSelection;
}

//***********************************************************************
//**                         CButton Overrides                         **
//***********************************************************************
void CColorButton::PreSubclassWindow() 
{
    ModifyStyle(0, BS_OWNERDRAW);      

    _Inherited::PreSubclassWindow();
}

//***********************************************************************
//**                         Message Handlers                         **
//***********************************************************************
BEGIN_MESSAGE_MAP(CColorButton, CButton)
    //{{AFX_MSG_MAP(CColorButton)
    ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
    ON_MESSAGE(UM_CPN_SELENDOK,     OnSelEndOK)
    ON_MESSAGE(UM_CPN_SELENDCANCEL, OnSelEndCancel)
    ON_MESSAGE(UM_CPN_SELCHANGE,    OnSelChange)
END_MESSAGE_MAP()


//***********************************************************************
// Method:	CColorButton::OnSelEndOK()
// Notes:	None.
//***********************************************************************
LONG CColorButton::OnSelEndOK(UINT lParam, LONG /*wParam*/)
{
	m_bPopupActive = FALSE;

    COLORREF OldColor = m_Color;
	
	Color = (COLORREF)lParam;

    CWnd *pParent = GetParent();

    if (pParent) 
	{
        pParent->SendMessage(UM_CPN_CLOSEUP, lParam, (WPARAM) GetDlgCtrlID());
        pParent->SendMessage(UM_CPN_SELENDOK, lParam, (WPARAM) GetDlgCtrlID());
    }

    if (OldColor != m_Color)
		if (pParent) pParent->SendMessage(UM_CPN_SELCHANGE, (m_Color!=CLR_DEFAULT)? m_Color:m_DefaultColor, (WPARAM) GetDlgCtrlID());

    return TRUE;
}


//***********************************************************************
// Method:	CColorButton::OnSelEndCancel()
// Notes:	None.
//***********************************************************************
LONG CColorButton::OnSelEndCancel(UINT lParam, LONG /*wParam*/)
{
	m_bPopupActive = FALSE;
	
	Color = (COLORREF)lParam;

    CWnd *pParent = GetParent();

    if (pParent) 
	{
        pParent->SendMessage(UM_CPN_CLOSEUP, lParam, (WPARAM) GetDlgCtrlID());
        pParent->SendMessage(UM_CPN_SELENDCANCEL, lParam, (WPARAM) GetDlgCtrlID());
    }

    return TRUE;
}


//***********************************************************************
// Method:	CColorButton::OnSelChange()
// Notes:	None.
//***********************************************************************
LONG CColorButton::OnSelChange(UINT lParam, LONG /*wParam*/)
{
    if (m_bTrackSelection) 
		Color = (COLORREF)lParam;

    CWnd *pParent = GetParent();

    if (pParent) pParent->SendMessage(UM_CPN_SELCHANGE, (m_Color!=CLR_DEFAULT)? m_Color:m_DefaultColor, (WPARAM) GetDlgCtrlID());	//Cax2 defaultcol fix

    return TRUE;
}

//***********************************************************************
// Method:	CColorButton::OnCreate()
// Notes:	None.
//***********************************************************************
int CColorButton::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CButton::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}

//***********************************************************************
// Method:	CColorButton::OnClicked()
// Notes:	None.
//***********************************************************************
BOOL CColorButton::OnClicked()
{
	m_bPopupActive = TRUE;

    CRect rDraw;
    GetWindowRect(rDraw);

    new CColourPopup(CPoint(rDraw.left, rDraw.bottom),		// Point to display popup
                     m_Color,								// Selected colour
                     this,									// parent
                     m_strDefaultText,						// "Default" text area
                     m_strCustomText);						// Custom Text

    CWnd *pParent = GetParent();

    if (pParent)
        pParent->SendMessage(UM_CPN_DROPDOWN, (LPARAM)m_Color, (WPARAM) GetDlgCtrlID());

    return TRUE;
}



//***********************************************************************
// Method:	CColorButton::DrawItem()
// Notes:	None.
//***********************************************************************
void CColorButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	ASSERT(lpDrawItemStruct);

	CDC*    pDC      = CDC::FromHandle(lpDrawItemStruct->hDC);
	UINT    state    = lpDrawItemStruct->itemState;
    CRect   rDraw    = lpDrawItemStruct->rcItem;
	CRect	rArrow;

	if (m_bPopupActive)
		state |= ODS_SELECTED|ODS_FOCUS;

	//******************************************************
	//**                  Draw Outer Edge
	//******************************************************
	UINT uFrameState = DFCS_BUTTONPUSH|DFCS_ADJUSTRECT;

	if (state & ODS_SELECTED)
		uFrameState |= DFCS_PUSHED;

	if (state & ODS_DISABLED)
		uFrameState |= DFCS_INACTIVE;
	
	pDC->DrawFrameControl(&rDraw,
						  DFC_BUTTON,
						  uFrameState);


	if (state & ODS_SELECTED)
		rDraw.OffsetRect(1,1);

	//******************************************************
	//**                     Draw Focus
	//******************************************************
	if (state & ODS_FOCUS) 
    {
		RECT rFocus = {rDraw.left,
					   rDraw.top,
					   rDraw.right - 1,
					   rDraw.bottom};
  
        pDC->DrawFocusRect(&rFocus);
    }

	rDraw.DeflateRect(::GetSystemMetrics(SM_CXEDGE),
					  ::GetSystemMetrics(SM_CYEDGE));

	//******************************************************
	//**                     Draw Arrow
	//******************************************************
	rArrow.left		= rDraw.right - g_ciArrowSizeX - ::GetSystemMetrics(SM_CXEDGE) /2;
	rArrow.right	= rArrow.left + g_ciArrowSizeX;
	rArrow.top		= (rDraw.bottom + rDraw.top)/2 - g_ciArrowSizeY / 2;
	rArrow.bottom	= (rDraw.bottom + rDraw.top)/2 + g_ciArrowSizeY / 2;

	DrawArrow(pDC,
			  &rArrow,
			  0,
			  (state & ODS_DISABLED) 
			  ? ::GetSysColor(COLOR_GRAYTEXT) 
			  : RGB(0,0,0));


	rDraw.right = rArrow.left - ::GetSystemMetrics(SM_CXEDGE)/2;

	//******************************************************
	//**                   Draw Separator
	//******************************************************
	pDC->DrawEdge(&rDraw,
				  EDGE_ETCHED,
				  BF_RIGHT);

	rDraw.right -= (::GetSystemMetrics(SM_CXEDGE) * 2) + 1 ;
				  
	//******************************************************
	//**                     Draw Color
	//******************************************************
	if ((state & ODS_DISABLED) == 0)
	{
		pDC->FillSolidRect(&rDraw,
						   (m_Color == CLR_DEFAULT)
						   ? m_DefaultColor
						   : m_Color);

		::FrameRect(pDC->m_hDC,
					&rDraw,
					(HBRUSH)::GetStockObject(BLACK_BRUSH));
	}
}


//***********************************************************************
//**                          Static Methods                          **
//***********************************************************************

//***********************************************************************
// Method:	CColorButton::DrawArrow()
// Notes:	None.
//***********************************************************************
void CColorButton::DrawArrow(CDC* pDC, 
							 RECT* pRect, 
							 int iDirection,
							 COLORREF clrArrow /*= RGB(0,0,0)*/)
{
	POINT ptsArrow[3];

	switch (iDirection)
	{
		case 0 : // Down
		{
			ptsArrow[0].x = pRect->left;
			ptsArrow[0].y = pRect->top;
			ptsArrow[1].x = pRect->right;
			ptsArrow[1].y = pRect->top;
			ptsArrow[2].x = (pRect->left + pRect->right)/2;
			ptsArrow[2].y = pRect->bottom;
			break;
		}

		case 1 : // Up
		{
			ptsArrow[0].x = pRect->left;
			ptsArrow[0].y = pRect->bottom;
			ptsArrow[1].x = pRect->right;
			ptsArrow[1].y = pRect->bottom;
			ptsArrow[2].x = (pRect->left + pRect->right)/2;
			ptsArrow[2].y = pRect->top;
			break;
		}

		case 2 : // Left
		{
			ptsArrow[0].x = pRect->right;
			ptsArrow[0].y = pRect->top;
			ptsArrow[1].x = pRect->right;
			ptsArrow[1].y = pRect->bottom;
			ptsArrow[2].x = pRect->left;
			ptsArrow[2].y = (pRect->top + pRect->bottom)/2;
			break;
		}

		case 3 : // Right
		{
			ptsArrow[0].x = pRect->left;
			ptsArrow[0].y = pRect->top;
			ptsArrow[1].x = pRect->left;
			ptsArrow[1].y = pRect->bottom;
			ptsArrow[2].x = pRect->right;
			ptsArrow[2].y = (pRect->top + pRect->bottom)/2;
			break;
		}
	}
	
	CBrush brsArrow(clrArrow);
	CPen penArrow(PS_SOLID, 1 , clrArrow);

	CBrush* pOldBrush = pDC->SelectObject(&brsArrow);
	CPen*   pOldPen   = pDC->SelectObject(&penArrow);
	
	pDC->SetPolyFillMode(WINDING);
	pDC->Polygon(ptsArrow, 3);

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}