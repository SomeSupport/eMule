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
#include "KademliaWnd.h"
#include "KadContactListCtrl.h"
#include "KadContactHistogramCtrl.h"
#include "KadLookupGraph.h"
#include "KadSearchListCtrl.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/Kademlia/prefs.h"
#include "kademlia/utils/LookupHistory.h"
#include "Kademlia/net/kademliaudplistener.h"
#include "kademlia/kademlia/search.h"
#include "Ini2.h"
#include "CustomAutoComplete.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "clientlist.h"
#include "log.h"
#include "HttpDownloadDlg.h"
#include "Kademlia/routing/RoutingZone.h"
#include "HelpIDs.h"
#include ".\kademliawnd.h"
#include "DropDownButton.h"
#include "MenuCmds.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	ONBOOTSTRAP_STRINGS_PROFILE	_T("AC_BootstrapIPs.dat")

#define	DFLT_TOOLBAR_BTN_WIDTH	24

#define	WND1_BUTTON_XOFF	8
#define	WND1_BUTTON_WIDTH	250
#define	WND1_BUTTON_HEIGHT	22	// don't set the height to something different than 22 unless you know exactly what you are doing!
#define	WND1_NUM_BUTTONS	2


// KademliaWnd dialog

IMPLEMENT_DYNAMIC(CKademliaWnd, CDialog)

BEGIN_MESSAGE_MAP(CKademliaWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_BOOTSTRAPBUTTON, OnBnClickedBootstrapbutton)
	ON_BN_CLICKED(IDC_FIREWALLCHECKBUTTON, OnBnClickedFirewallcheckbutton)
	ON_BN_CLICKED(IDC_KADCONNECT, OnBnConnect)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CTLCOLOR()
	ON_EN_SETFOCUS(IDC_BOOTSTRAPIP, OnEnSetfocusBootstrapip)
	ON_EN_SETFOCUS(IDC_BOOTSTRAPURL, OnEnSetfocusBootstrapNodesdat)
	ON_EN_CHANGE(IDC_BOOTSTRAPIP, UpdateControlsState)
	ON_EN_CHANGE(IDC_BOOTSTRAPPORT, UpdateControlsState)
	ON_EN_CHANGE(IDC_BOOTSTRAPURL, UpdateControlsState)
	ON_BN_CLICKED(IDC_RADCLIENTS, UpdateControlsState)
	ON_BN_CLICKED(IDC_RADIP, UpdateControlsState)
	ON_BN_CLICKED(IDC_RADNODESURL, UpdateControlsState)
	ON_WM_HELPINFO()
	ON_NOTIFY(NM_DBLCLK, IDC_SEARCHLIST, OnNMDblclkSearchlist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SEARCHLIST, OnListModifiedSearchlist)
END_MESSAGE_MAP()

CKademliaWnd::CKademliaWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CKademliaWnd::IDD, pParent)
{
	m_contactListCtrl = new CKadContactListCtrl;
	m_contactHistogramCtrl = new CKadContactHistogramCtrl;
	m_kadLookupGraph = new CKadLookupGraph;
	searchList = new CKadSearchListCtrl;
	m_pacONBSIPs = NULL;
	m_pbtnWnd = new CDropDownButton;
	m_bBootstrapListMode = false;
	icon_kadsea=NULL;
}

CKademliaWnd::~CKademliaWnd()
{
	if (m_pacONBSIPs){
		m_pacONBSIPs->Unbind();
		m_pacONBSIPs->Release();
	}
	delete m_contactListCtrl;
	delete m_contactHistogramCtrl;
	delete searchList;
	delete m_kadLookupGraph;
	delete m_pbtnWnd;

	if (icon_kadsea)
		VERIFY( DestroyIcon(icon_kadsea) );
}

BOOL CKademliaWnd::SaveAllSettings()
{
	if (m_pacONBSIPs)
		m_pacONBSIPs->SaveList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + ONBOOTSTRAP_STRINGS_PROFILE);

	return TRUE;
}

BOOL CKademliaWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	m_contactListCtrl->Init();
	m_kadLookupGraph->Init();
	searchList->Init();

    // Initalize Toolbar
	CRect rcBtn1;
	rcBtn1.top = 5;
	rcBtn1.left = WND1_BUTTON_XOFF;
	rcBtn1.right = rcBtn1.left + WND1_BUTTON_WIDTH + WND1_NUM_BUTTONS*DFLT_TOOLBAR_BTN_WIDTH;
	rcBtn1.bottom = rcBtn1.top + WND1_BUTTON_HEIGHT;
	m_pbtnWnd->Init(false);
	m_pbtnWnd->MoveWindow(&rcBtn1);
	SetAllIcons();

	// Vista: Remove the TBSTYLE_TRANSPARENT to avoid flickering (can be done only after the toolbar was initially created with TBSTYLE_TRANSPARENT !?)
	m_pbtnWnd->ModifyStyle((theApp.m_ullComCtrlVer >= MAKEDLLVERULL(6, 16, 0, 0)) ? TBSTYLE_TRANSPARENT : 0, TBSTYLE_TOOLTIPS);
	m_pbtnWnd->SetExtendedStyle(m_pbtnWnd->GetExtendedStyle() | TBSTYLE_EX_MIXEDBUTTONS);

	TBBUTTON atb1[1+WND1_NUM_BUTTONS] = {0};
	atb1[0].iBitmap = 0;
	atb1[0].idCommand = IDC_KADICO1;
	atb1[0].fsState = TBSTATE_ENABLED;
	atb1[0].fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
	atb1[0].iString = -1;

	atb1[1].iBitmap = 0;
	atb1[1].idCommand = MP_VIEW_KADCONTACTS;
	atb1[1].fsState = TBSTATE_ENABLED;
	atb1[1].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
	atb1[1].iString = -1;

	atb1[2].iBitmap = 1;
	atb1[2].idCommand = MP_VIEW_KADLOOKUP;
	atb1[2].fsState = TBSTATE_ENABLED;
	atb1[2].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
	atb1[2].iString = -1;
	m_pbtnWnd->AddButtons(_countof(atb1), atb1);

	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_SIZE | TBIF_BYINDEX;
	tbbi.cx = WND1_BUTTON_WIDTH;
	m_pbtnWnd->SetButtonInfo(0, &tbbi);

	// 'GetMaxSize' does not work properly under:
	//	- Win98SE with COMCTL32 v5.80
	//	- Win2000 with COMCTL32 v5.81 
	// The value returned by 'GetMaxSize' is just couple of pixels too small so that the 
	// last toolbar button is nearly not visible at all.
	// So, to circumvent such problems, the toolbar control should be created right with
	// the needed size so that we do not really need to call the 'GetMaxSize' function.
	// Although it would be better to call it to adapt for system metrics basically.
	if (theApp.m_ullComCtrlVer > MAKEDLLVERULL(5,81,0,0))
	{
		CSize size;
		m_pbtnWnd->GetMaxSize(&size);
		CRect rc;
		m_pbtnWnd->GetWindowRect(&rc);
		ScreenToClient(&rc);
		// the with of the toolbar should already match the needed size (see comment above)
		ASSERT( size.cx == rc.Width() );
		m_pbtnWnd->MoveWindow(rc.left, rc.top, size.cx, rc.Height());
	}

	Localize();

	AddAnchor(IDC_KADICO1, TOP_LEFT);
	AddAnchor(IDC_CONTACTLIST, TOP_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_KAD_LOOKUPGRAPH, TOP_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_KAD_HISTOGRAM, TOP_RIGHT, MIDDLE_RIGHT);
	AddAnchor(IDC_KADICO2, MIDDLE_LEFT);
	AddAnchor(IDC_SEARCHLIST, MIDDLE_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FIREWALLCHECKBUTTON, TOP_RIGHT);
	AddAnchor(IDC_KADCONNECT, TOP_RIGHT);
	AddAnchor(IDC_KADSEARCHLAB, MIDDLE_LEFT);
	AddAnchor(IDC_BSSTATIC, TOP_RIGHT);
	AddAnchor(IDC_BOOTSTRAPBUTTON, TOP_RIGHT);
	AddAnchor(IDC_BOOTSTRAPPORT, TOP_RIGHT);
	AddAnchor(IDC_BOOTSTRAPIP, TOP_RIGHT);
	AddAnchor(IDC_BOOTSTRAPURL, TOP_RIGHT);
	AddAnchor(IDC_SSTATIC4, TOP_RIGHT);
	AddAnchor(IDC_SSTATIC7, TOP_RIGHT);
	AddAnchor(IDC_NODESDATLABEL, TOP_RIGHT);
	AddAnchor(IDC_RADCLIENTS, TOP_RIGHT);
	AddAnchor(IDC_RADIP, TOP_RIGHT);
	AddAnchor(IDC_RADNODESURL, TOP_RIGHT);

	searchList->UpdateKadSearchCount();
	m_contactListCtrl->UpdateKadContactCount();

	if (thePrefs.GetUseAutocompletion()){
		m_pacONBSIPs = new CCustomAutoComplete();
		m_pacONBSIPs->AddRef();
		if (m_pacONBSIPs->Bind(::GetDlgItem(m_hWnd, IDC_BOOTSTRAPIP), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES ))
			m_pacONBSIPs->LoadList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + ONBOOTSTRAP_STRINGS_PROFILE);
	}

	CheckDlgButton(IDC_RADCLIENTS,1);
	ShowLookupGraph(false);
	
	return true;
}

void CKademliaWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONTACTLIST, *m_contactListCtrl);
	DDX_Control(pDX, IDC_KAD_HISTOGRAM, *m_contactHistogramCtrl);
	DDX_Control(pDX, IDC_KAD_LOOKUPGRAPH, *m_kadLookupGraph);
	DDX_Control(pDX, IDC_SEARCHLIST, *searchList);
	DDX_Control(pDX, IDC_BSSTATIC, m_ctrlBootstrap);
	DDX_Control(pDX, IDC_KADICO1, *m_pbtnWnd);
}

BOOL CKademliaWnd::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN)
	{
		// Don't handle Ctrl+Tab in this window. It will be handled by main window.
		if (pMsg->wParam == VK_TAB && GetAsyncKeyState(VK_CONTROL) < 0)
			return FALSE;
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CKademliaWnd::OnEnSetfocusBootstrapip()
{
	CheckRadioButton(IDC_RADIP, IDC_RADNODESURL, IDC_RADIP);
	UpdateControlsState();
}

void CKademliaWnd::OnEnSetfocusBootstrapNodesdat()
{
	CheckRadioButton(IDC_RADIP, IDC_RADNODESURL, IDC_RADNODESURL);
	UpdateControlsState();
}

void CKademliaWnd::OnBnClickedBootstrapbutton()
{
	CString strIP;
	uint16 nPort = 0;

	if (IsDlgButtonChecked(IDC_RADIP) != 0)
	{
		GetDlgItem(IDC_BOOTSTRAPIP)->GetWindowText(strIP);
		strIP.Trim();

		// auto-handle ip:port
		int iPos;
		if ((iPos = strIP.Find(_T(':'))) != -1)
		{
			GetDlgItem(IDC_BOOTSTRAPPORT)->SetWindowText(strIP.Mid(iPos+1));
			strIP = strIP.Left(iPos);
			GetDlgItem(IDC_BOOTSTRAPIP)->SetWindowText(strIP);
		}

		CString strPort;
		GetDlgItem(IDC_BOOTSTRAPPORT)->GetWindowText(strPort);
		strPort.Trim();
		nPort = (uint16)_ttoi(strPort);

		// invalid IP/Port
		if (strIP.GetLength()<7 || nPort==0)
			return;

		if (m_pacONBSIPs && m_pacONBSIPs->IsBound())
			m_pacONBSIPs->AddItem(strIP + _T(":") + strPort, 0);
		if( !Kademlia::CKademlia::IsRunning() )
		{
			Kademlia::CKademlia::Start();
			theApp.emuledlg->ShowConnectionState();
		}
		Kademlia::CKademlia::Bootstrap(strIP, nPort);
	}
	else if (IsDlgButtonChecked(IDC_RADNODESURL) != 0)
	{
		CString strURL;
		GetDlgItemText(IDC_BOOTSTRAPURL, strURL);
		if (strURL.IsEmpty() || (strURL.Find(_T("://")) == -1)) {
			// not a valid URL
			LogError(LOG_STATUSBAR, GetResString(IDS_INVALIDURL) );
			return;
		}
		UpdateNodesDatFromURL(strURL);
	}
	else
	{
		if( !Kademlia::CKademlia::IsRunning() )
		{
			Kademlia::CKademlia::Start();
			theApp.emuledlg->ShowConnectionState();
		}
	}
}

void CKademliaWnd::OnBnClickedFirewallcheckbutton()
{
	Kademlia::CKademlia::RecheckFirewalled();
}

void CKademliaWnd::OnBnConnect()
{
	if (Kademlia::CKademlia::IsConnected())
		Kademlia::CKademlia::Stop();
	else if (Kademlia::CKademlia::IsRunning())
		Kademlia::CKademlia::Stop();
	else
		Kademlia::CKademlia::Start();
	theApp.emuledlg->ShowConnectionState();
}

void CKademliaWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CKademliaWnd::SetAllIcons()
{
	// frames
	m_ctrlBootstrap.SetIcon(_T("KadBootstrap"));

	if (icon_kadsea)
		VERIFY( DestroyIcon(icon_kadsea) );
	icon_kadsea = theApp.LoadIcon(_T("KadCurrentSearches"), 16, 16);
	((CStatic*)GetDlgItem(IDC_KADICO2))->SetIcon(icon_kadsea);

	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 1, 1);
	iml.Add(CTempIconLoader(_T("KadContactList")));
	iml.Add(CTempIconLoader(_T("FriendSlot")));
	CImageList* pImlOld = m_pbtnWnd->SetImageList(&iml);
	iml.Detach();
	if (pImlOld)
		pImlOld->DeleteImageList();
}

void CKademliaWnd::Localize()
{
	m_ctrlBootstrap.SetWindowText(GetResString(IDS_BOOTSTRAP));
	GetDlgItem(IDC_BOOTSTRAPBUTTON)->SetWindowText(GetResString(IDS_BOOTSTRAP));
	GetDlgItem(IDC_SSTATIC4)->SetWindowText(GetResString(IDS_SV_ADDRESS) + _T(":"));
	GetDlgItem(IDC_SSTATIC7)->SetWindowText(GetResString(IDS_SV_PORT) + _T(":"));
	GetDlgItem(IDC_NODESDATLABEL)->SetWindowText(GetResString(IDS_BOOTSRAPNODESDAT));
	GetDlgItem(IDC_FIREWALLCHECKBUTTON)->SetWindowText(GetResString(IDS_KAD_RECHECKFW));
	
	SetDlgItemText(IDC_RADCLIENTS,GetResString(IDS_RADCLIENTS));

	UpdateControlsState();
	UpdateButtonTitle( m_pbtnWnd->IsButtonChecked(MP_VIEW_KADLOOKUP)==TRUE);
	m_contactHistogramCtrl->Localize();
	m_contactListCtrl->Localize();
	searchList->Localize();
	m_kadLookupGraph->Localize();

	m_pbtnWnd->SetBtnText(MP_VIEW_KADCONTACTS, GetResString(IDS_KADCONTACTLAB));
	m_pbtnWnd->SetBtnText(MP_VIEW_KADLOOKUP, GetResString(IDS_LOOKUPGRAPH));
}

void CKademliaWnd::UpdateControlsState()
{
	CString strLabel;
	if (Kademlia::CKademlia::IsConnected())
		strLabel = GetResString(IDS_MAIN_BTN_DISCONNECT);
	else if (Kademlia::CKademlia::IsRunning())
		strLabel = GetResString(IDS_MAIN_BTN_CANCEL);
	else
		strLabel = GetResString(IDS_MAIN_BTN_CONNECT);
	strLabel.Remove(_T('&'));
	GetDlgItem(IDC_KADCONNECT)->SetWindowText(strLabel);

	CString strBootstrapIP;
	GetDlgItemText(IDC_BOOTSTRAPIP, strBootstrapIP);
	CString strBootstrapPort;
	GetDlgItemText(IDC_BOOTSTRAPPORT, strBootstrapPort);
	CString strBootstrapUrl;
	GetDlgItemText(IDC_BOOTSTRAPURL, strBootstrapUrl);

	GetDlgItem(IDC_BOOTSTRAPBUTTON)->EnableWindow(
		!Kademlia::CKademlia::IsConnected()
		&& (  (IsDlgButtonChecked(IDC_RADIP)>0 && !strBootstrapIP.IsEmpty()
				&& (strBootstrapIP.Find(_T(':')) != -1 || !strBootstrapPort.IsEmpty()))
		    || IsDlgButtonChecked(IDC_RADCLIENTS) != 0
			|| (IsDlgButtonChecked(IDC_RADNODESURL) != 0 && !strBootstrapUrl.IsEmpty() ))
			);
}

UINT CKademliaWnd::GetContactCount() const
{
	return m_contactListCtrl->GetItemCount();
}

void CKademliaWnd::UpdateKadContactCount()
{
	m_contactListCtrl->UpdateKadContactCount();
}

void CKademliaWnd::StartUpdateContacts()
{
	m_contactHistogramCtrl->SetRedraw(TRUE);
	m_contactHistogramCtrl->Invalidate();
	m_contactListCtrl->SetRedraw(TRUE);
}

void CKademliaWnd::StopUpdateContacts()
{
	m_contactHistogramCtrl->SetRedraw(FALSE);
	m_contactListCtrl->SetRedraw(FALSE);
}

bool CKademliaWnd::ContactAdd(const Kademlia::CContact* contact)
{
	if (contact->IsBootstrapContact() != m_bBootstrapListMode)
	{
		if (!contact->IsBootstrapContact())
		{
			// we have real contacts to add, remove all the bootstrap contacts and cancel the mode
			m_bBootstrapListMode = false;
			m_contactListCtrl->DeleteAllItems();
		}
		else
		{
			ASSERT(0);
			return false;
		}
	}
	if (!m_bBootstrapListMode)
		m_contactHistogramCtrl->ContactAdd(contact);
	return m_contactListCtrl->ContactAdd(contact);
}

void CKademliaWnd::ContactRem(const Kademlia::CContact* contact)
{
	if (contact->IsBootstrapContact() == m_bBootstrapListMode)
	{
		if (!m_bBootstrapListMode)
			m_contactHistogramCtrl->ContactRem(contact);
		m_contactListCtrl->ContactRem(contact);
	}
}

void CKademliaWnd::ContactRef(const Kademlia::CContact* contact)
{
	if (contact->IsBootstrapContact() == m_bBootstrapListMode)
		m_contactListCtrl->ContactRef(contact);
}

void CKademliaWnd::UpdateNodesDatFromURL(CString strURL){
	CString strTempFilename;
	strTempFilename.Format(_T("%stemp-%d-nodes.dat"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), ::GetTickCount());

	// try to download nodes.dat
	Log(GetResString(IDS_DOWNLOADING_NODESDAT_FROM), strURL);
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_strTitle = GetResString(IDS_DOWNLOADING_NODESDAT);
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK) {
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FAILEDDOWNLOADNODES), strURL);
		return;
	}

	if (!Kademlia::CKademlia::IsRunning()){
		Kademlia::CKademlia::Start();
		theApp.emuledlg->ShowConnectionState();
	}
	Kademlia::CKademlia::GetRoutingZone()->ReadFile(strTempFilename);
	(void)_tremove(strTempFilename);		
}

BOOL CKademliaWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	theApp.ShowHelp(eMule_FAQ_GUI_Kad);
	return TRUE;
}

HBRUSH CKademliaWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = theApp.emuledlg->GetCtlColor(pDC, pWnd, nCtlColor);
	if (hbr)
		return hbr;
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CKademliaWnd::UpdateSearchGraph(Kademlia::CLookupHistory* pLookupHistory)
{
	if (Kademlia::CKademlia::IsRunning())
	{
		m_kadLookupGraph->UpdateSearch(pLookupHistory);
		if (m_kadLookupGraph->GetAutoShowLookups() && !m_kadLookupGraph->HasActiveLookup())
		{
			bool bGraphVisible = m_pbtnWnd->IsButtonChecked(MP_VIEW_KADLOOKUP) == TRUE ? true : false;
			Kademlia::CLookupHistory* pActiveLookupHistory = searchList->FetchAndSelectActiveSearch(bGraphVisible);
			if (pActiveLookupHistory != NULL)
				SetSearchGraph(pActiveLookupHistory, false);
		}
	}
	else
	{
		// we could allow watching lookups while Kad is disconnected, but it feels cleaner and disconnecteder if we wipe the graph
		if (m_kadLookupGraph->HasLookup())
			SetSearchGraph(NULL, false);
	}
}

void CKademliaWnd::SetSearchGraph(Kademlia::CLookupHistory* pLookupHistory, bool bMakeVisible)
{
	m_kadLookupGraph->SetSearch(pLookupHistory);
	if (bMakeVisible)
		ShowLookupGraph(true);
	else
	{
		bool bGraphVisible = m_pbtnWnd->IsButtonChecked(MP_VIEW_KADLOOKUP) == TRUE ? true : false;
		UpdateButtonTitle(bGraphVisible);
	}
}

void CKademliaWnd::OnNMDblclkSearchlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pItemInfo = (LPNMITEMACTIVATE)pNMHDR;
	if (pItemInfo->iItem >= 0)
	{
		Kademlia::CSearch* pSearch = (Kademlia::CSearch*)searchList->GetItemData(pItemInfo->iItem);
		if (pSearch != NULL)
		{
			SetSearchGraph(pSearch->GetLookupHistory(), true);
			thePrefs.SetAutoShowLookups(false);
		}
	}
	*pResult = 0;
}

void CKademliaWnd::OnListModifiedSearchlist(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	POSITION pos = searchList->GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		Kademlia::CSearch* pSearch = (Kademlia::CSearch*)searchList->GetItemData(searchList->GetNextSelectedItem(pos));
		if (pSearch != NULL)
			SetSearchGraph(pSearch->GetLookupHistory(), false);
	}
	*pResult = 0;
}

void CKademliaWnd::ShowLookupGraph(bool bShow)
{
	int iIcon;
	CString strText;
	if (bShow)
	{
		iIcon = 1;
		m_pbtnWnd->CheckButton(MP_VIEW_KADLOOKUP);
	}
	else
	{
		iIcon = 0;
		m_pbtnWnd->CheckButton(MP_VIEW_KADCONTACTS);
	}
	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_IMAGE;
	tbbi.iImage = iIcon;
	m_pbtnWnd->SetButtonInfo(GetWindowLong(*m_pbtnWnd, GWL_ID), &tbbi);
	m_kadLookupGraph->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
	m_contactListCtrl->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	UpdateButtonTitle(bShow);
}

void CKademliaWnd::UpdateButtonTitle(bool bLookupGraph)
{
	CString strText;
	if (bLookupGraph)
	{
		if (m_kadLookupGraph->HasLookup())
			strText.Format(_T("%s (%s)"), GetResString(IDS_LOOKUPGRAPH), m_kadLookupGraph->GetCurrentLookupTitle());
		else
			strText =  GetResString(IDS_LOOKUPGRAPH);
	}
	else
		strText.Format(_T("%s (%i)"), GetResString(IDS_KADCONTACTLAB), m_contactListCtrl->GetItemCount());
	m_pbtnWnd->SetWindowText(strText);
}

void CKademliaWnd::UpdateContactCount()
{
	if (m_pbtnWnd->IsButtonChecked(MP_VIEW_KADCONTACTS))
		UpdateButtonTitle(false);
}

BOOL CKademliaWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{ 
		case IDC_KADICO1:
			ShowLookupGraph(m_contactListCtrl->IsWindowVisible() == TRUE ? true : false);
			break;
		case MP_VIEW_KADCONTACTS:
			ShowLookupGraph(false);
			break;
		case MP_VIEW_KADLOOKUP:
			ShowLookupGraph(true);
			break;
		default:
			return CWnd::OnCommand(wParam, lParam);
	}
	return TRUE;
}

void CKademliaWnd::SetBootstrapListMode()
{
	// rather than normal contacts we show contacts only used to bootstrap in this mode
	// once the first "normal" contact will be added, the mode is cancelled and all bootstrap contacts removed
	if (m_contactListCtrl->GetItemCount() == 0)
		m_bBootstrapListMode = true;
	else
		ASSERT(0);
}