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
#include "emule.h"
#include "emuledlg.h"
#include "SearchDlg.h"
#include "SearchParamsWnd.h"
#include "SearchResultsWnd.h"
#include "SearchParams.h"
#include "OtherFunctions.h"
#include "CustomAutoComplete.h"
#include "HelpIDs.h"
#include "Opcodes.h"
#include "StringConversion.h"
#include <list>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	SEARCH_STRINGS_PROFILE	_T("AC_SearchStrings.dat")

IMPLEMENT_DYNAMIC(CSearchParamsWnd, CDialogBar);

BEGIN_MESSAGE_MAP(CSearchParamsWnd, CDialogBar)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_SYSCOLORCHANGE()
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
	ON_BN_CLICKED(IDC_STARTS, OnBnClickedStart)
	ON_BN_CLICKED(IDC_CANCELS, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_MORE, OnBnClickedMore)
	ON_EN_CHANGE(IDC_SEARCHNAME, OnEnChangeName)
	ON_CBN_SELCHANGE(IDC_TYPESEARCH, OnEnChangeName)
	ON_BN_CLICKED(IDC_SEARCH_RESET, OnBnClickedSearchReset)
	ON_BN_CLICKED(IDC_DD, OnDDClicked)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelChangeMethod)
	ON_CBN_SELENDOK(IDC_COMBO1, OnCbnSelEndOkMethod)
	ON_WM_SYSCOMMAND()
	ON_WM_SETCURSOR()
	ON_WM_HELPINFO()
END_MESSAGE_MAP()


CSearchParamsWnd::CSearchParamsWnd()
{
	m_szFloat.SetSize(0,0);
	m_szMRU.SetSize(0,0);

	// load default windows system cursor (a shared resource)
	m_hcurMove = ::LoadCursor(NULL, IDC_SIZEALL);

	m_searchdlg = NULL;
	m_pacSearchString = NULL;
	m_rcNameLbl.SetRectEmpty();
	m_rcName.SetRectEmpty();
	m_rcDropDownArrow.SetRectEmpty();
	m_rcFileTypeLbl.SetRectEmpty();
	m_rcFileType.SetRectEmpty();
	m_rcReset.SetRectEmpty();
	m_rcMethodLbl.SetRectEmpty();
	m_rcMethod.SetRectEmpty();
	m_rcOpts.SetRectEmpty();
	m_rcStart.SetRectEmpty();
	m_rcMore.SetRectEmpty();
	m_rcCancel.SetRectEmpty();
}

CSearchParamsWnd::~CSearchParamsWnd()
{
	if (m_pacSearchString)
	{
		m_pacSearchString->Unbind();
		m_pacSearchString->Release();
	}

	// 'DestroyCursor' is not to be used for a shared cursor (as returned with LoadCursor) (?!)
//	if (m_hcurMove)
//		VERIFY( DestroyCursor(m_hcurMove) );
}

void CSearchParamsWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ctlMethod);
	DDX_Control(pDX, IDC_TYPESEARCH, m_ctlFileType);
	DDX_Control(pDX, IDC_SEARCHNAME, m_ctlName);
	DDX_Control(pDX, IDC_SEARCH_OPTS, m_ctlOpts);
	DDX_Control(pDX, IDC_STARTS, m_ctlStart);
	DDX_Control(pDX, IDC_CANCELS, m_ctlCancel);
	DDX_Control(pDX, IDC_MORE, m_ctlMore);
}

LRESULT CSearchParamsWnd::OnInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	Default();
	InitWindowStyles(this);

	//(void)m_sizeDefault; // not yet set
	CRect sizeDefault;
	GetWindowRect(&sizeDefault);
	CRect rcBorders(4, 4, 4, 4);
	SetBorders(&rcBorders);
	m_szFloat.cx = sizeDefault.Width() + rcBorders.left + rcBorders.right + GetSystemMetrics(SM_CXEDGE) * 2;
	m_szFloat.cy = sizeDefault.Height() + rcBorders.top + rcBorders.bottom + GetSystemMetrics(SM_CYEDGE) * 2;
	m_szMRU = m_szFloat;

	UpdateData(FALSE);
	SetAllIcons();

	GetDlgItem(IDC_MSTATIC3)->GetWindowRect(&m_rcNameLbl);
	ScreenToClient(&m_rcNameLbl);

	m_ctlName.GetWindowRect(&m_rcName);
	ScreenToClient(&m_rcName);

	GetDlgItem(IDC_DD)->GetWindowRect(&m_rcDropDownArrow);
	ScreenToClient(&m_rcDropDownArrow);

	GetDlgItem(IDC_MSTATIC7)->GetWindowRect(&m_rcFileTypeLbl);
	ScreenToClient(&m_rcFileTypeLbl);

	m_ctlFileType.GetWindowRect(&m_rcFileType);
	ScreenToClient(&m_rcFileType);

	GetDlgItem(IDC_SEARCH_RESET)->GetWindowRect(&m_rcReset);
	ScreenToClient(&m_rcReset);

	GetDlgItem(IDC_METH)->GetWindowRect(&m_rcMethodLbl);
	ScreenToClient(&m_rcMethodLbl);

	m_ctlMethod.GetWindowRect(&m_rcMethod);
	ScreenToClient(&m_rcMethod);

	m_ctlOpts.GetWindowRect(&m_rcOpts);
	ScreenToClient(&m_rcOpts);

	m_ctlStart.GetWindowRect(&m_rcStart);
	ScreenToClient(&m_rcStart);
	
	m_ctlMore.GetWindowRect(&m_rcMore);
	ScreenToClient(&m_rcMore);
	
	m_ctlCancel.GetWindowRect(&m_rcCancel);
	ScreenToClient(&m_rcCancel);

	if (thePrefs.GetUseAutocompletion())
	{
		m_pacSearchString = new CCustomAutoComplete();
		m_pacSearchString->AddRef();
		if (m_pacSearchString->Bind(m_ctlName, ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST))
			m_pacSearchString->LoadList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + SEARCH_STRINGS_PROFILE);
		if (theApp.m_fontSymbol.m_hObject)
		{
			GetDlgItem(IDC_DD)->SetFont(&theApp.m_fontSymbol);
			GetDlgItem(IDC_DD)->SetWindowText(_T("6")); // show a down-arrow
		}
	}
	else
		GetDlgItem(IDC_DD)->ShowWindow(SW_HIDE);

	m_ctlName.LimitText(MAX_SEARCH_EXPRESSION_LEN); // max. length of search expression
	
	m_ctlMethod.ModifyStyle(0, WS_CLIPCHILDREN); // Reduce flickering during resize
	InitMethodsCtrl();
	if (m_ctlMethod.SetCurSel(thePrefs.GetSearchMethod()) == CB_ERR)
		m_ctlMethod.SetCurSel(SearchTypeEd2kServer);

	m_ctlFileType.ModifyStyle(0, WS_CLIPCHILDREN); // Reduce flickering during resize
	m_ctlFileType.GetComboBoxCtrl()->ModifyStyle(0, CBS_SORT);
	InitFileTypesCtrl();
	// Win98: "CComboBoxEx2::SelectString" fails under Win98!
	//if (!m_ctlFileType.SelectString(GetResString(IDS_SEARCH_ANY)))
	//	m_ctlFileType.SetCurSel(0);
	if (!m_ctlFileType.SelectItemDataStringA(ED2KFTSTR_ANY))
		m_ctlFileType.SetCurSel(0);

	m_ctlOpts.ModifyStyle(0, WS_CLIPCHILDREN); // Does not help, control keeps flickering like mad
	m_ctlOpts.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	m_ctlOpts.InsertColumn(0, GetResString(IDS_PARAMETER));
	m_ctlOpts.InsertColumn(1, GetResString(IDS_VALUE));

	m_ctlOpts.InsertItem(orMinSize, GetResString(IDS_SEARCHMINSIZE));
	m_ctlOpts.InsertItem(orMaxSize, GetResString(IDS_SEARCHMAXSIZE));
	m_ctlOpts.InsertItem(orAvailability, GetResString(IDS_SEARCHAVAIL));
	m_ctlOpts.InsertItem(orExtension, GetResString(IDS_SEARCHEXTENTION));
	m_ctlOpts.InsertItem(orCompleteSources, GetResString(IDS_COMPLSOURCES));
	m_ctlOpts.InsertItem(orCodec, GetResString(IDS_CODEC));
	m_ctlOpts.InsertItem(orBitrate, GetResString(IDS_MINBITRATE));
	m_ctlOpts.InsertItem(orLength, GetResString(IDS_MINLENGTH));
	m_ctlOpts.InsertItem(orTitle, GetResString(IDS_TITLE));
	m_ctlOpts.InsertItem(orAlbum, GetResString(IDS_ALBUM));
	m_ctlOpts.InsertItem(orArtist, GetResString(IDS_ARTIST));
	m_ctlOpts.SetColumnWidth(0, 100/*LVSCW_AUTOSIZE*/);
	m_ctlOpts.SetColumnWidth(1, 120);

	UpdateControls();
	OnEnChangeName();

	return TRUE;
}

#define	MIN_HORZ_WIDTH	500
#define	MIN_VERT_WIDTH	200

CSize CSearchParamsWnd::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	CFrameWnd* pFrm = GetDockingFrame();

	// This function is typically called with
	// CSize sizeHorz = m_pBar->CalcDynamicLayout(0, LM_HORZ | LM_HORZDOCK);
	// CSize sizeVert = m_pBar->CalcDynamicLayout(0, LM_VERTDOCK);
	// CSize sizeFloat = m_pBar->CalcDynamicLayout(0, LM_HORZ | LM_MRUWIDTH);

	CRect rcFrmClnt;
	pFrm->GetClientRect(&rcFrmClnt);
	CRect rcInside = rcFrmClnt;
	CalcInsideRect(rcInside, dwMode & LM_HORZDOCK);
	CRect rcBorders;
	rcBorders.left = rcInside.left - rcFrmClnt.left;
	rcBorders.top = rcInside.top - rcFrmClnt.top;
	rcBorders.bottom = rcFrmClnt.bottom - rcInside.bottom;
	rcBorders.right = rcFrmClnt.right - rcInside.right;

	if (dwMode & (LM_HORZDOCK | LM_VERTDOCK))
	{
		if (dwMode & LM_VERTDOCK)
		{
			CSize szFloat;
			szFloat.cx = MIN_VERT_WIDTH;
			szFloat.cy = rcFrmClnt.Height() + GetSystemMetrics(SM_CYEDGE) * 2;
			m_szFloat = szFloat;
			return szFloat;
		}
		else if (dwMode & LM_HORZDOCK)
		{
			CSize szFloat;
			szFloat.cx = rcFrmClnt.Width() + GetSystemMetrics(SM_CXEDGE) * 2;
			szFloat.cy = m_sizeDefault.cy + rcBorders.top + rcBorders.bottom;
			m_szFloat = szFloat;
			return szFloat;
		}
		return CDialogBar::CalcDynamicLayout(nLength, dwMode);
	}

	if (dwMode & LM_MRUWIDTH){
		return m_szMRU;
	}
	if (dwMode & LM_COMMIT){
		m_szMRU = m_szFloat;
		return m_szFloat;
	}

	CSize szFloat;
	if ((dwMode & LM_LENGTHY) == 0)
	{
		szFloat.cx = nLength;
		if (nLength < m_sizeDefault.cx + rcBorders.left + rcBorders.right)
		{
			szFloat.cx = MIN_VERT_WIDTH;
			szFloat.cy = MIN_HORZ_WIDTH;
		}
		else
		{
			szFloat.cy = m_sizeDefault.cy + rcBorders.top + rcBorders.bottom;
		}
	}
	else
	{
		szFloat.cy = nLength;
		if (nLength < MIN_HORZ_WIDTH)
		{
			szFloat.cx = m_sizeDefault.cx + rcBorders.left + rcBorders.right;
			szFloat.cy = m_sizeDefault.cy + rcBorders.top + rcBorders.bottom;
		}
		else
		{
			szFloat.cx = MIN_VERT_WIDTH;
		}
	}

	m_szFloat = szFloat;
	return szFloat;
}

BOOL CSearchParamsWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_hcurMove && ((m_dwStyle & (CBRS_GRIPPER|CBRS_FLOATING)) == CBRS_GRIPPER) && pWnd->GetSafeHwnd() == m_hWnd)
	{
		CPoint ptCursor;
		if (GetCursorPos(&ptCursor))
		{
			ScreenToClient(&ptCursor);
			CRect rcClnt;
			GetClientRect(&rcClnt);
			bool bMouseOverGripper;
			if (m_dwStyle & CBRS_ORIENT_HORZ)
				bMouseOverGripper = (rcClnt.PtInRect(ptCursor) && ptCursor.x <= 10);
			else
				bMouseOverGripper = (rcClnt.PtInRect(ptCursor) && ptCursor.y <= 10);
			if (bMouseOverGripper)
			{
				::SetCursor(m_hcurMove);
				return TRUE;
			}
		}
	}
	return CDialogBar::OnSetCursor(pWnd, nHitTest, message);
}

void CSearchParamsWnd::OnSize(UINT nType, int cx, int cy)
{
	CDialogBar::OnSize(nType, cx, cy);

	if (m_ctlName.m_hWnd == NULL)
		return;
	if (cx >= MIN_HORZ_WIDTH)
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		CalcInsideRect(rcClient, TRUE);

		// resizing the name field instead the filter fields makes sense, because the filter input mostly stays small while a bigger
		// name filed allows longer search queries without scrolling.
		// however it doesn't looks nice at all, because of the asymmetry which is created by not resizing the method selectors
		// but resizing them wouldn't make any sense at all, so we need to find a better build up at some point, but for now
		// stay with the old method which is not perfect but looks fair enough
		/*
		HDWP hdwp = BeginDeferWindowPos(12);
		if (hdwp)
		{
			UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE;
			int iWidthOpts = (rcClient.Width() * 35) / 100;
			if (iWidthOpts < m_rcOpts.Width())
				iWidthOpts = m_rcOpts.Width();
			int iXPosOpts = rcClient.right - iWidthOpts;
			ASSERT( m_rcStart.Width() == m_rcMore.Width() && m_rcStart.Width() == m_rcCancel.Width() );
			int iXPosButtons = iXPosOpts - m_rcStart.Width() - 2;
			int iXPosDDArrow = iXPosButtons - 2 - m_rcDropDownArrow.Width();
			int iWidthName = iXPosDDArrow - (rcClient.left + m_rcName.left);

			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_MSTATIC3), NULL, rcClient.left + m_rcNameLbl.left, rcClient.top + m_rcNameLbl.top, m_rcNameLbl.Width(), m_rcNameLbl.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlName, NULL, rcClient.left + m_rcName.left, rcClient.top + m_rcName.top, iWidthName, m_rcName.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_DD), NULL, iXPosDDArrow, rcClient.top + m_rcDropDownArrow.top, m_rcDropDownArrow.Width(), m_rcDropDownArrow.Height(), uFlags);

			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_MSTATIC7), NULL, rcClient.left + m_rcFileTypeLbl.left, rcClient.top + m_rcFileTypeLbl.top, m_rcFileTypeLbl.Width(), m_rcFileTypeLbl.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlFileType, NULL, rcClient.left + m_rcFileType.left, rcClient.top + m_rcFileType.top, m_rcFileType.Width(), m_rcFileType.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_SEARCH_RESET), NULL, rcClient.left + m_rcReset.left, rcClient.top + m_rcReset.top, m_rcReset.Width(), m_rcReset.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_METH), NULL, rcClient.left + m_rcMethodLbl.left, rcClient.top + m_rcMethodLbl.top, m_rcMethodLbl.Width(), m_rcMethodLbl.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlMethod, NULL, rcClient.left + m_rcMethod.left, rcClient.top + m_rcMethod.top, m_rcMethod.Width(), m_rcMethod.Height(), uFlags);

			hdwp = DeferWindowPos(hdwp, m_ctlStart, NULL, iXPosButtons, rcClient.top + m_rcStart.top, m_rcStart.Width(), m_rcStart.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlMore, NULL, iXPosButtons, rcClient.top + m_rcMore.top, m_rcMore.Width(), m_rcMore.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlCancel, NULL, iXPosButtons, rcClient.top + m_rcCancel.top, m_rcCancel.Width(), m_rcCancel.Height(), uFlags);

   			hdwp = DeferWindowPos(hdwp, m_ctlOpts, NULL, iXPosOpts, rcClient.top + m_rcOpts.top, iWidthOpts, m_rcOpts.Height(), uFlags);
			VERIFY( EndDeferWindowPos(hdwp) );
		}
		*/

		int iWidthOpts = rcClient.right - (rcClient.left + m_rcOpts.left);
		HDWP hdwp = BeginDeferWindowPos(12);
		if (hdwp)
		{
			UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE;
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_MSTATIC3), NULL, rcClient.left + m_rcNameLbl.left, rcClient.top + m_rcNameLbl.top, m_rcNameLbl.Width(), m_rcNameLbl.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlName, NULL, rcClient.left + m_rcName.left, rcClient.top + m_rcName.top, m_rcName.Width(), m_rcName.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_DD), NULL, rcClient.left + m_rcDropDownArrow.left, rcClient.top + m_rcDropDownArrow.top, m_rcDropDownArrow.Width(), m_rcDropDownArrow.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_MSTATIC7), NULL, rcClient.left + m_rcFileTypeLbl.left, rcClient.top + m_rcFileTypeLbl.top, m_rcFileTypeLbl.Width(), m_rcFileTypeLbl.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlFileType, NULL, rcClient.left + m_rcFileType.left, rcClient.top + m_rcFileType.top, m_rcFileType.Width(), m_rcFileType.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_SEARCH_RESET), NULL, rcClient.left + m_rcReset.left, rcClient.top + m_rcReset.top, m_rcReset.Width(), m_rcReset.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, *GetDlgItem(IDC_METH), NULL, rcClient.left + m_rcMethodLbl.left, rcClient.top + m_rcMethodLbl.top, m_rcMethodLbl.Width(), m_rcMethodLbl.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlMethod, NULL, rcClient.left + m_rcMethod.left, rcClient.top + m_rcMethod.top, m_rcMethod.Width(), m_rcMethod.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlStart, NULL, rcClient.left + m_rcStart.left, rcClient.top + m_rcStart.top, m_rcStart.Width(), m_rcStart.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlMore, NULL, rcClient.left + m_rcMore.left, rcClient.top + m_rcMore.top, m_rcMore.Width(), m_rcMore.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlCancel, NULL, rcClient.left + m_rcCancel.left, rcClient.top + m_rcCancel.top, m_rcCancel.Width(), m_rcCancel.Height(), uFlags);
			hdwp = DeferWindowPos(hdwp, m_ctlOpts, NULL, rcClient.left + m_rcOpts.left, rcClient.top + m_rcOpts.top, iWidthOpts, m_rcOpts.Height(), uFlags);
			VERIFY( EndDeferWindowPos(hdwp) );
		}

		m_ctlOpts.ModifyStyle(0, LVS_NOCOLUMNHEADER);
		CRect rcOptsClnt;
		m_ctlOpts.GetClientRect(&rcOptsClnt);
		m_ctlOpts.SetColumnWidth(0, LVSCW_AUTOSIZE);
		// (**1) Adjust for Windows Classic Theme and Flat Style control
		int iCol2Width = rcOptsClnt.Width() - m_ctlOpts.GetColumnWidth(0) - 2/*(**1)*/;
		if (m_ctlOpts.GetColumnWidth(1) != iCol2Width)
			m_ctlOpts.SetColumnWidth(1, iCol2Width);
	}
	else if (cx < MIN_HORZ_WIDTH)
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		CalcInsideRect(rcClient, FALSE);

		int y = rcClient.top;

		CRect rcNameLbl;
		GetDlgItem(IDC_MSTATIC3)->GetWindowRect(&rcNameLbl);
		ScreenToClient(&rcNameLbl);
		GetDlgItem(IDC_MSTATIC3)->MoveWindow(rcClient.left, y, rcNameLbl.Width(), rcNameLbl.Height());
		y += rcNameLbl.Height() + 2;

		CRect rcName;
		m_ctlName.GetWindowRect(&rcName);
		ScreenToClient(&rcName);
		CRect rcDropDownArrow;
		GetDlgItem(IDC_DD)->GetWindowRect(&rcDropDownArrow);
		ScreenToClient(&rcDropDownArrow);
		int iNameWidth = rcClient.Width() - 4 - rcDropDownArrow.Width();
		m_ctlName.MoveWindow(rcClient.left, y, iNameWidth, rcName.Height());
		GetDlgItem(IDC_DD)->MoveWindow(rcClient.left + iNameWidth + 4, y, rcDropDownArrow.Width(), rcDropDownArrow.Height());
		y += rcName.Height() + 2;

		CRect rcFileTypeLbl;
		GetDlgItem(IDC_MSTATIC7)->GetWindowRect(&rcFileTypeLbl);
		ScreenToClient(&rcFileTypeLbl);
		GetDlgItem(IDC_MSTATIC7)->MoveWindow(rcClient.left, y, rcFileTypeLbl.Width(), rcFileTypeLbl.Height());
		y += rcFileTypeLbl.Height() + 2;

		CRect rcFileType;
		m_ctlFileType.GetWindowRect(&rcFileType);
		ScreenToClient(&rcFileType);
		m_ctlFileType.MoveWindow(rcClient.left, y, rcFileType.Width(), rcFileType.Height());

		CRect rcReset;
		GetDlgItem(IDC_SEARCH_RESET)->GetWindowRect(&rcReset);
		ScreenToClient(&rcReset);
		GetDlgItem(IDC_SEARCH_RESET)->MoveWindow(rcClient.left + rcFileType.Width() + 8, y, rcReset.Width(), rcReset.Height());

		y += rcFileType.Height() + 8;

		CRect rcMethodLbl;
		GetDlgItem(IDC_METH)->GetWindowRect(&rcMethodLbl);
		ScreenToClient(&rcMethodLbl);
		GetDlgItem(IDC_METH)->MoveWindow(rcClient.left, y, rcMethodLbl.Width(), rcMethodLbl.Height());
		y += rcMethodLbl.Height() + 2;

		CRect rcMethod;
		m_ctlMethod.GetWindowRect(&rcMethod);
		ScreenToClient(&rcMethod);
		m_ctlMethod.MoveWindow(rcClient.left, y, rcMethod.Width(), rcMethod.Height());
		y += rcMethod.Height() + 8;

		m_ctlStart.MoveWindow(rcClient.left, y, m_rcStart.Width(), m_rcStart.Height());
		m_ctlMore.MoveWindow(rcClient.left + m_rcStart.Width() + 4, y, m_rcMore.Width(), m_rcMore.Height());
		y += m_rcStart.Height() + 4;
		m_ctlCancel.MoveWindow(rcClient.left, y, m_rcCancel.Width(), m_rcCancel.Height());
		y += m_rcStart.Height() + 4;

		int iOptsHeight = rcClient.bottom - y - 2;
		m_ctlOpts.MoveWindow(rcClient.left, y, rcClient.Width(), iOptsHeight);
		CRect rcOptsClnt;
		m_ctlOpts.GetClientRect(&rcOptsClnt);
		m_ctlOpts.SetColumnWidth(1, rcOptsClnt.Width() - m_ctlOpts.GetColumnWidth(0));
		y += iOptsHeight + 4;

		m_ctlOpts.ModifyStyle(LVS_NOCOLUMNHEADER, 0);
	}
}

void CSearchParamsWnd::OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL /*bDisableIfNoHndler*/)
{
	// Disable MFC's command routing by not passing the process flow to base class
}

void CSearchParamsWnd::UpdateControls()
{
	int iMethod = m_ctlMethod.GetCurSel();
	if (iMethod != CB_ERR)
	{
		if (iMethod != thePrefs.GetSearchMethod())
		{
			if (iMethod == SearchTypeKademlia)
				OnEnChangeName();
			else if (iMethod == SearchTypeEd2kServer && m_searchdlg->IsLocalEd2kSearchRunning())
				m_ctlStart.EnableWindow(FALSE);
			else if (iMethod == SearchTypeEd2kGlobal && m_searchdlg->IsGlobalEd2kSearchRunning())
				m_ctlStart.EnableWindow(FALSE);
			thePrefs.SetSearchMethod(iMethod);
		}
	}

	m_ctlOpts.SetItemData(orAvailability, (iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orExtension, (iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orCompleteSources, (iMethod==SearchTypeKademlia || iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orCodec, (iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orBitrate, (iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orLength, (iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orTitle, (iMethod==SearchTypeEd2kServer || iMethod==SearchTypeEd2kGlobal || iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orAlbum, (iMethod==SearchTypeEd2kServer || iMethod==SearchTypeEd2kGlobal || iMethod==SearchTypeContentDB) ? 1 : 0);
	m_ctlOpts.SetItemData(orArtist, (iMethod==SearchTypeEd2kServer || iMethod==SearchTypeEd2kGlobal || iMethod==SearchTypeContentDB) ? 1 : 0);
}

void CSearchParamsWnd::SetAllIcons()
{
	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("SearchMethod_KadServer")));
	iml.Add(CTempIconLoader(_T("SearchMethod_SERVER")));
	iml.Add(CTempIconLoader(_T("SearchMethod_GLOBAL")));
	iml.Add(CTempIconLoader(_T("SearchMethod_KADEMLIA")));
	iml.Add(CTempIconLoader(_T("SearchMethod_FILEDONKEY")));
	m_ctlMethod.SetImageList(&iml);
	m_imlSearchMethods.DeleteImageList();
	m_imlSearchMethods.Attach(iml.Detach());

	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("SearchFileType_Any")));
	iml.Add(CTempIconLoader(_T("SearchFileType_Archive")));
	iml.Add(CTempIconLoader(_T("SearchFileType_Audio")));
	iml.Add(CTempIconLoader(_T("SearchFileType_CDImage")));
	iml.Add(CTempIconLoader(_T("SearchFileType_Picture")));
	iml.Add(CTempIconLoader(_T("SearchFileType_Program")));
	iml.Add(CTempIconLoader(_T("SearchFileType_Video")));
	iml.Add(CTempIconLoader(_T("SearchFileType_Document")));
	iml.Add(CTempIconLoader(_T("AABCollectionFileType")));
	m_ctlFileType.SetImageList(&iml);
	m_imlFileType.DeleteImageList();
	m_imlFileType.Attach(iml.Detach());
}

void CSearchParamsWnd::OnDestroy()
{
	CDialogBar::OnDestroy();
	m_imlSearchMethods.DeleteImageList();
	m_imlFileType.DeleteImageList();
}

void CSearchParamsWnd::OnSysColorChange()
{
	CDialogBar::OnSysColorChange();
	SetAllIcons();
}

void CSearchParamsWnd::InitMethodsCtrl()
{
	int iMethod = m_ctlMethod.GetCurSel();
	m_ctlMethod.ResetContent();
	VERIFY( m_ctlMethod.AddItem(GetResString(IDS_AUTOMATIC), 0) == SearchTypeAutomatic );
	VERIFY( m_ctlMethod.AddItem(GetResString(IDS_SERVER), 1) == SearchTypeEd2kServer );
	VERIFY( m_ctlMethod.AddItem(GetResString(IDS_GLOBALSEARCH), 2) == SearchTypeEd2kGlobal );
	VERIFY( m_ctlMethod.AddItem(GetResString(IDS_KADEMLIA) + _T(" ") + GetResString(IDS_NETWORK), 3) == SearchTypeKademlia );
	VERIFY( m_ctlMethod.AddItem(_T("ContentDB (Web)"), 4) == SearchTypeContentDB );
	UpdateHorzExtent(m_ctlMethod, 16); // adjust dropped width to ensure all strings are fully visible
	m_ctlMethod.SetCurSel(iMethod != CB_ERR ? iMethod : SearchTypeAutomatic);
}

class SFileTypeCbEntry
{
public:
#if _MSC_VER>=1400
	SFileTypeCbEntry() {
	}
#endif
	SFileTypeCbEntry(const CString& strLabel, LPCSTR pszItemData, int iImage) {
		m_strLabel = strLabel;
		m_pszItemData = pszItemData;
		m_iImage = iImage;
	}

	bool operator<(const SFileTypeCbEntry& e) const {
		return (m_strLabel.Compare(e.m_strLabel) < 0);
	}

	CString m_strLabel;
	LPCSTR m_pszItemData;
	int m_iImage;
};

void CSearchParamsWnd::InitFileTypesCtrl()
{
	// get current selected entry by value (language independent)
	CStringA strCurSelFileType;
	int iItem = m_ctlFileType.GetCurSel();
	if (iItem != CB_ERR)
	{
		LPCSTR pszED2KFileType = (LPCSTR)m_ctlFileType.GetItemDataPtr(iItem);
		ASSERT( pszED2KFileType != NULL );
		strCurSelFileType = pszED2KFileType;
	}

	m_ctlFileType.ResetContent();

	// create temp. list of new entries (language dependent)
	std::list<SFileTypeCbEntry> lstFileTypeCbEntries;
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_ANY), ED2KFTSTR_ANY, 0));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_ARC), ED2KFTSTR_ARCHIVE, 1));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_AUDIO), ED2KFTSTR_AUDIO, 2));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_CDIMG), ED2KFTSTR_CDIMAGE, 3));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_PICS), ED2KFTSTR_IMAGE, 4));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_PRG), ED2KFTSTR_PROGRAM, 5));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_VIDEO), ED2KFTSTR_VIDEO, 6));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_DOC), ED2KFTSTR_DOCUMENT, 7));
	lstFileTypeCbEntries.push_back(SFileTypeCbEntry(GetResString(IDS_SEARCH_EMULECOLLECTION), ED2KFTSTR_EMULECOLLECTION, 8));

	// sort list with current language locale
	lstFileTypeCbEntries.sort();

	// fill combobox control with already sorted list
	std::list<SFileTypeCbEntry>::const_iterator it;
	for (it = lstFileTypeCbEntries.begin(); it != lstFileTypeCbEntries.end(); it++)
	{
		int iItem;
		if ((iItem = m_ctlFileType.AddItem((*it).m_strLabel, (*it).m_iImage)) != CB_ERR)
			m_ctlFileType.SetItemData(iItem, (DWORD_PTR)(*it).m_pszItemData);
	}

	UpdateHorzExtent(m_ctlFileType, 16); // adjust dropped width to ensure all strings are fully visible

	// restore previous selected entry by value (language independent)
	if (!m_ctlFileType.SelectItemDataStringA(strCurSelFileType))
	{
		// Win98: "CComboBoxEx2::SelectString" fails under Win98!
		//if (!m_ctlFileType.SelectString(GetResString(IDS_SEARCH_ANY)))
		//	m_ctlFileType.SetCurSel(0);
		if (!m_ctlFileType.SelectItemDataStringA(ED2KFTSTR_ANY))
			m_ctlFileType.SetCurSel(0);
	}
}

void CSearchParamsWnd::Localize()
{
	SetWindowText(GetResString(IDS_SEARCHPARAMS));

	GetDlgItem(IDC_MSTATIC3)->SetWindowText(GetResString(IDS_SW_NAME));
	GetDlgItem(IDC_MSTATIC7)->SetWindowText(GetResString(IDS_TYPE));
	GetDlgItem(IDC_SEARCH_RESET)->SetWindowText(GetResString(IDS_PW_RESET));
	GetDlgItem(IDC_METH)->SetWindowText(GetResString(IDS_METHOD));

	m_ctlStart.SetWindowText(GetResString(IDS_SW_START));
	m_ctlCancel.SetWindowText(GetResString(IDS_CANCEL));
	m_ctlMore.SetWindowText(GetResString(IDS_MORE));

	SetWindowText(GetResString(IDS_SEARCHPARAMS));

	InitMethodsCtrl();
	InitFileTypesCtrl();

	m_ctlOpts.SetItemText(orMinSize, 0, GetResString(IDS_SEARCHMINSIZE));
	m_ctlOpts.SetItemText(orMaxSize, 0, GetResString(IDS_SEARCHMAXSIZE));
	m_ctlOpts.SetItemText(orAvailability, 0, GetResString(IDS_SEARCHAVAIL));
	m_ctlOpts.SetItemText(orExtension, 0, GetResString(IDS_SEARCHEXTENTION));
	m_ctlOpts.SetItemText(orCompleteSources, 0, GetResString(IDS_COMPLSOURCES));
	m_ctlOpts.SetItemText(orCodec, 0, GetResString(IDS_CODEC));
	m_ctlOpts.SetItemText(orBitrate, 0, GetResString(IDS_MINBITRATE));
	m_ctlOpts.SetItemText(orLength, 0, GetResString(IDS_MINLENGTH));
	m_ctlOpts.SetItemText(orTitle, 0, GetResString(IDS_TITLE));
	m_ctlOpts.SetItemText(orAlbum, 0, GetResString(IDS_ALBUM));
	m_ctlOpts.SetItemText(orArtist, 0, GetResString(IDS_ARTIST));

	CHeaderCtrl* pHeaderCtrl = m_ctlOpts.GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_PARAMETER);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(0, &hdi);

	strRes = GetResString(IDS_VALUE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(1, &hdi);
}

BOOL CSearchParamsWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
	   		return FALSE;

		if (pMsg->wParam == VK_DELETE && m_pacSearchString && m_pacSearchString->IsBound() && pMsg->hwnd == m_ctlName.m_hWnd)
		{
			if (GetAsyncKeyState(VK_MENU)<0 || GetAsyncKeyState(VK_CONTROL)<0)
				m_pacSearchString->Clear();
			else
				m_pacSearchString->RemoveSelectedItem();
		}

		if (pMsg->wParam == VK_RETURN && m_ctlStart.IsWindowEnabled())
		{
			if (m_pacSearchString && m_pacSearchString->IsBound() && pMsg->hwnd == m_ctlName.m_hWnd)
			{
				CString strText;
				m_ctlName.GetWindowText(strText);
				if (!strText.IsEmpty())
				{
					m_ctlName.SetWindowText(_T("")); // this seems to be the only chance to let the dropdown list to disapear
					m_ctlName.SetWindowText(strText);
					m_ctlName.SetSel(strText.GetLength(), strText.GetLength());
				}
			}
		}
	}

	return CDialogBar::PreTranslateMessage(pMsg);
}

void CSearchParamsWnd::OnBnClickedStart()
{
	m_ctlMore.EnableWindow(FALSE);
	if (m_ctlOpts.GetEditCtrl()->GetSafeHwnd())
		m_ctlOpts.CommitEditCtrl();

	SSearchParams* pParams = GetParameters();
	if (pParams)
	{
		if (!pParams->strExpression.IsEmpty())
		{
			if (m_pacSearchString && m_pacSearchString->IsBound())
				m_pacSearchString->AddItem(pParams->strExpression, 0);
			m_searchdlg->StartSearch(pParams);
		}
		else
			delete pParams;
	}
}

void CSearchParamsWnd::OnBnClickedMore()
{
	CWnd* pWndFocus = GetFocus();
	m_ctlMore.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_ctlMore.m_hWnd)
		m_ctlStart.SetFocus();

	if (m_searchdlg->SearchMore())
	{
		pWndFocus = GetFocus();
		m_ctlStart.EnableWindow(FALSE);
		if (pWndFocus && pWndFocus->m_hWnd == m_ctlStart.m_hWnd)
			m_ctlName.SetFocus();
		m_ctlCancel.EnableWindow(TRUE);
	}
}

void CSearchParamsWnd::OnBnClickedCancel()
{
	m_searchdlg->CancelSearch();

	CWnd* pWndFocus = GetFocus();
	m_ctlCancel.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_ctlCancel.m_hWnd)
		m_ctlName.SetFocus();
	m_ctlStart.EnableWindow(TRUE);
}

void CSearchParamsWnd::ResetHistory()
{
	if (m_pacSearchString == NULL)
		return;
	m_ctlName.SendMessage(WM_KEYDOWN, VK_ESCAPE, 0x00510001);
	m_pacSearchString->Clear(); 
}

void CSearchParamsWnd::OnCbnSelChangeMethod()
{
	UpdateControls();
	OnEnChangeName();
}

void CSearchParamsWnd::OnCbnSelEndOkMethod()
{
	UpdateControls();
}

void CSearchParamsWnd::OnDDClicked()
{
	m_ctlName.SetFocus();
	m_ctlName.SetWindowText(_T(""));
	m_ctlName.SendMessage(WM_KEYDOWN, VK_DOWN, 0x00510001);
}

BOOL CSearchParamsWnd::SaveSearchStrings()
{
	if (m_pacSearchString == NULL)
		return FALSE;
	return m_pacSearchString->SaveList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + SEARCH_STRINGS_PROFILE);
}

void CSearchParamsWnd::SaveSettings()
{
	SaveSearchStrings();
}

void CSearchParamsWnd::OnEnChangeName()
{
	m_ctlStart.EnableWindow(m_ctlName.GetWindowTextLength() > 0);
}

void CSearchParamsWnd::OnBnClickedSearchReset()
{
	m_ctlName.SetWindowText(_T(""));

	// Win98: "CComboBoxEx2::SelectString" fails under Win98!
	//if (!m_ctlFileType.SelectString(GetResString(IDS_SEARCH_ANY)))
	//	m_ctlFileType.SetCurSel(0);
	if (!m_ctlFileType.SelectItemDataStringA(ED2KFTSTR_ANY))
		m_ctlFileType.SetCurSel(0);

	for (int i = 0; i < m_ctlOpts.GetItemCount(); i++)
		m_ctlOpts.SetItemText(i, 1, _T(""));

	OnEnChangeName();
}

void CSearchParamsWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID == SC_KEYMENU)
	{
		if (lParam == EMULE_HOTMENU_ACCEL)
			theApp.emuledlg->SendMessage(WM_COMMAND, IDC_HOTMENU);
		else
			theApp.emuledlg->SendMessage(WM_SYSCOMMAND, nID, lParam);
		return;
	}
	CDialogBar::OnSysCommand(nID, lParam);
}

void CSearchParamsWnd::SetParameters(const SSearchParams* pParams)
{
	if (!pParams->bClientSharedFiles)
	{
		m_ctlName.SetWindowText(pParams->strExpression);
		m_ctlFileType.SelectItemDataStringA(pParams->strFileType);

		m_ctlOpts.SetItemText(orMinSize, 1, pParams->strMinSize);
		m_ctlOpts.SetItemText(orMaxSize, 1, pParams->strMaxSize);
		m_ctlOpts.SetItemText(orExtension, 1, pParams->strExtension);
		CString strBuff;
		if (pParams->uAvailability > 0)
			strBuff.Format(_T("%u"), pParams->uAvailability);
		else
			strBuff.Empty();
		m_ctlOpts.SetItemText(orAvailability, 1, strBuff);

		if (pParams->uComplete > 0)
			strBuff.Format(_T("%u"), pParams->uComplete);
		else
			strBuff.Empty();
		m_ctlOpts.SetItemText(orCompleteSources, 1, strBuff);

		m_ctlOpts.SetItemText(orCodec, 1, pParams->strCodec);

		if (pParams->ulMinBitrate > 0)
			strBuff.Format(_T("%u"), pParams->ulMinBitrate);
		else
			strBuff.Empty();
		m_ctlOpts.SetItemText(orBitrate, 1, strBuff);

		if (pParams->ulMinLength > 0)
			SecToTimeLength(pParams->ulMinLength, strBuff);
		else
			strBuff.Empty();
		m_ctlOpts.SetItemText(orLength, 1, strBuff);
		m_ctlOpts.SetItemText(orTitle, 1, pParams->strTitle);
		m_ctlOpts.SetItemText(orAlbum, 1, pParams->strAlbum);
		m_ctlOpts.SetItemText(orArtist, 1, pParams->strArtist);
	}
}

uint64 CSearchParamsWnd::GetSearchAttrSize(const CString& rstrExpr)
{
	CString strExpr(rstrExpr);
	strExpr.Trim();
	LPTSTR endptr = NULL;
	double dbl = _tcstod(strExpr, &endptr);
	if (endptr && *endptr != _T('\0'))
	{
		while (*endptr == _T(' '))
			endptr++;

		TCHAR chModifier = _totlower((_TUCHAR)*endptr);
		if (chModifier == _T('b'))
			return (uint64)(dbl + 0.5);
		else if (chModifier == _T('k'))
			return (uint64)(dbl*1024 + 0.5);
		else if (chModifier == _T('\0') || chModifier == _T('m'))
			;
		else if (chModifier == _T('g'))
			return (uint64)(dbl*1024*1024*1024 + 0.5);
		else 
			return (uint64)-1;
	}
	return (uint64)(dbl*1024*1024 + 0.5); // Default = MBytes
}

ULONG CSearchParamsWnd::GetSearchAttrNumber(const CString& rstrExpr)
{
	CString strExpr(rstrExpr);
	strExpr.Trim();
	LPTSTR endptr = NULL;
	double dbl = _tcstod(strExpr, &endptr);
	if (endptr && *endptr != _T('\0'))
	{
		while (*endptr == _T(' '))
			endptr++;

		TCHAR chModifier = _totlower((_TUCHAR)*endptr);
		if (chModifier == _T('\0'))
			;
		else if (chModifier == _T('k'))
			return (ULONG)(dbl*1000 + 0.5);
		else if (chModifier == _T('m'))
			return (ULONG)(dbl*1000*1000 + 0.5);
		else if (chModifier == _T('g'))
			return (ULONG)(dbl*1000*1000*1000 + 0.5);
		else
			return (ULONG)-1;
	}
	return (ULONG)(dbl + 0.5);
}

ULONG CSearchParamsWnd::GetSearchAttrLength(const CString& rstrExpr)
{
	CString strExpr(rstrExpr);
	strExpr.Trim();

	UINT hour = 0, min = 0, sec = 0;
	if (_stscanf(strExpr, _T("%u : %u : %u"), &hour, &min, &sec) == 3)
		return hour * 3600 + min * 60 + sec;
	if (_stscanf(strExpr, _T("%u : %u"), &min, &sec) == 2)
		return min * 60 + sec;

	LPTSTR endptr = NULL;
	double dbl = _tcstod(strExpr, &endptr);
	if (endptr && *endptr != _T('\0'))
	{
		while (*endptr == _T(' '))
			endptr++;

		TCHAR chModifier = _totlower((_TUCHAR)*endptr);
		if (chModifier == _T('\0') || chModifier == _T('s'))
			;
		else if (chModifier == _T('m'))
			return (ULONG)(dbl*60 + 0.5);
		else if (chModifier == _T('h'))
			return (ULONG)(dbl*60*60 + 0.5);
		else
			return (ULONG)-1;
	}
	return (ULONG)(dbl + 0.5);
}

SSearchParams* CSearchParamsWnd::GetParameters()
{
	CString strExpression;
	m_ctlName.GetWindowText(strExpression);
	strExpression.Trim();
	if (!IsValidEd2kString(strExpression)){
		AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + GetResString(IDS_SEARCH_INVALIDCHAR), MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
		return NULL;
	}

	CStringA strFileType;
	int iItem = m_ctlFileType.GetCurSel();
	if (iItem != CB_ERR)
	{
		LPCSTR pszED2KFileType = (LPCSTR)m_ctlFileType.GetItemDataPtr(iItem);
		ASSERT( pszED2KFileType != NULL );
		strFileType = pszED2KFileType;
	}

	CString strMinSize = m_ctlOpts.GetItemText(orMinSize, 1);
	uint64 ullMinSize = GetSearchAttrSize(strMinSize);
	if (ullMinSize == (uint64)-1) {
		CString strError;
		strError.Format(GetResString(IDS_SEARCH_ATTRERR), m_ctlOpts.GetItemText(orMinSize, 0));
		AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
		return NULL;
	}

	CString strMaxSize = m_ctlOpts.GetItemText(orMaxSize, 1);
	uint64 ullMaxSize = GetSearchAttrSize(strMaxSize);
	if (ullMaxSize == (uint64)-1) {
		CString strError;
		strError.Format(GetResString(IDS_SEARCH_ATTRERR), m_ctlOpts.GetItemText(orMaxSize, 0));
		AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
		return NULL;
	}
	
	if (ullMaxSize < ullMinSize){
		ullMaxSize = 0; // TODO: Create a message box for that
		m_ctlOpts.SetItemText(orMaxSize, 1, _T(""));
	}

	CString strExtension;
	if ((m_ctlOpts.GetItemData(orExtension) & 1) == 0)
	{
		strExtension = m_ctlOpts.GetItemText(orExtension, 1);
		strExtension.Trim();
		if (!strExtension.IsEmpty() && strExtension[0] == _T('.'))
		{
			strExtension = strExtension.Mid(1);
			m_ctlOpts.SetItemText(orExtension, 1, strExtension);
		}
	}

	UINT uAvailability = 0;
	if ((m_ctlOpts.GetItemData(orAvailability) & 1) == 0)
	{
		CString strAvailability = m_ctlOpts.GetItemText(orAvailability, 1);
		uAvailability = GetSearchAttrNumber(strAvailability);
		if (uAvailability == (UINT)-1)
		{
			CString strError;
			strError.Format(GetResString(IDS_SEARCH_ATTRERR), m_ctlOpts.GetItemText(orAvailability, 0));
			AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
			return NULL;
		}
		else if (uAvailability > 1000000)
		{
			uAvailability = 1000000;
			strAvailability.Format(_T("%u"), uAvailability);
			m_ctlOpts.SetItemText(orAvailability, 1, strAvailability);
		}
	}

	UINT uComplete = 0;
	if ((m_ctlOpts.GetItemData(orCompleteSources) & 1) == 0)
	{
		CString strComplete = m_ctlOpts.GetItemText(orCompleteSources, 1);
		uComplete = GetSearchAttrNumber(strComplete);
		if (uComplete == (UINT)-1)
		{
			CString strError;
			strError.Format(GetResString(IDS_SEARCH_ATTRERR), m_ctlOpts.GetItemText(orCompleteSources, 0));
			AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
			return NULL;
		}
		else if (uComplete > 1000000)
		{
			uComplete = 1000000;
			strComplete.Format(_T("%u"), uComplete);
			m_ctlOpts.SetItemText(orCompleteSources, 1, strComplete);
		}
	}

	CString strCodec;
	if ((m_ctlOpts.GetItemData(orCodec) & 1) == 0)
		strCodec = m_ctlOpts.GetItemText(orCodec, 1);
	strCodec.Trim();

	ULONG ulMinBitrate = 0;
	if ((m_ctlOpts.GetItemData(orBitrate) & 1) == 0)
	{
		CString strMinBitrate = m_ctlOpts.GetItemText(orBitrate, 1);
		ulMinBitrate = GetSearchAttrNumber(strMinBitrate);
		if (ulMinBitrate == (ULONG)-1)
		{
			CString strError;
			strError.Format(GetResString(IDS_SEARCH_ATTRERR), m_ctlOpts.GetItemText(orBitrate, 0));
			AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
			return NULL;
		}
		else if (ulMinBitrate > 1000000)
		{
			ulMinBitrate = 1000000;
			strMinBitrate.Format(_T("%u"), ulMinBitrate);
			m_ctlOpts.SetItemText(orBitrate, 1, strMinBitrate);
		}
	}

	ULONG ulMinLength = 0;
	if ((m_ctlOpts.GetItemData(orLength) & 1) == 0)
	{
		CString strMinLength = m_ctlOpts.GetItemText(orLength, 1);
		ulMinLength = GetSearchAttrLength(strMinLength);
		if (ulMinLength == (ULONG)-1)
		{
			CString strError;
			strError.Format(GetResString(IDS_SEARCH_ATTRERR), m_ctlOpts.GetItemText(orLength, 0));
			AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
			return NULL;
		}
		else if (ulMinLength > 3600*24)
		{
			ulMinLength = 3600*24;
			CString strValue;
			SecToTimeLength(ulMinLength, strValue);
			m_ctlOpts.SetItemText(orLength, 1, strValue);
		}
	}

	SSearchParams* pParams = new SSearchParams;
	pParams->strExpression = strExpression;
	pParams->eType = (ESearchType)m_ctlMethod.GetCurSel();
	pParams->strFileType = strFileType;
	pParams->strMinSize = strMinSize;
	pParams->ullMinSize = ullMinSize;
	pParams->strMaxSize = strMaxSize;
	pParams->ullMaxSize = ullMaxSize;
	pParams->uAvailability = uAvailability;
	pParams->strExtension = strExtension;
	//pParams->bMatchKeywords = IsDlgButtonChecked(IDC_MATCH_KEYWORDS)!=0;
	pParams->uComplete = uComplete;
	pParams->strCodec = strCodec;
	pParams->ulMinBitrate = ulMinBitrate;
	pParams->ulMinLength = ulMinLength;
	if ((m_ctlOpts.GetItemData(orTitle) & 1) == 0)
	{
		pParams->strTitle = m_ctlOpts.GetItemText(orTitle, 1);
		pParams->strTitle.Trim();
	}
	if ((m_ctlOpts.GetItemData(orAlbum) & 1) == 0)
	{
		pParams->strAlbum = m_ctlOpts.GetItemText(orAlbum, 1);
		pParams->strAlbum.Trim();
	}
	if ((m_ctlOpts.GetItemData(orArtist) & 1) == 0)
	{
		pParams->strArtist = m_ctlOpts.GetItemText(orArtist, 1);
		pParams->strArtist.Trim();
	}

	return pParams;
}

BOOL CSearchParamsWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	theApp.ShowHelp(eMule_FAQ_GUI_Search);
	return TRUE;
}

void CSearchParamsWnd::ProcessEd2kSearchLinkRequest(CString strSearchTerm)
{
	// We have a ed2k search link, asking us to search for stuff
	// For now we handle this very basic: Put the search term into the params window (so the users can also see it)
	// And if we are connected, start the search otherwise not.
	if (strSearchTerm.IsEmpty())
	{
		ASSERT( false );
		return;
	}
	OnBnClickedSearchReset();
	m_ctlName.SetWindowText(strSearchTerm);
	if (theApp.IsConnected())
	{
		if ((!theApp.IsConnected(true, false) && (ESearchType)m_ctlMethod.GetCurSel() == SearchTypeKademlia)
			|| (!theApp.IsConnected(false, true) && ((ESearchType)m_ctlMethod.GetCurSel() == SearchTypeEd2kServer || (ESearchType)m_ctlMethod.GetCurSel() == SearchTypeEd2kGlobal)))
		{
			m_ctlMethod.SetCurSel(0);
		}
		OnBnClickedStart();
	}
}
