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
#include "SearchDlg.h"
#include "SearchResultsWnd.h"
#include "SearchParamsWnd.h"
#include "SearchParams.h"
#include "Packets.h"
#include "OtherFunctions.h"
#include "SearchFile.h"
#include "SearchList.h"
#include "Sockets.h"
#include "ServerList.h"
#include "Server.h"
#include "SafeFile.h"
#include "DownloadQueue.h"
#include "Statistics.h"
#include "emuledlg.h"
#include "opcodes.h"
#include "ED2KLink.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/search.h"
#include "SearchExpr.h"
#define USE_FLEX
#include "Parser.hpp"
#include "Scanner.h"
#include "HelpIDs.h"
#include "Exceptions.h"
#include "StringConversion.h"
#include "UserMsgs.h"
#include "Log.h"
#include "MenuCmds.h"
#include "DropDownButton.h"
#include "ButtonsTabCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int yyparse();
extern int yyerror(const char* errstr);
extern int yyerror(LPCTSTR errstr);
extern LPCTSTR g_aszInvKadKeywordChars;

enum ESearchTimerID
{
	TimerServerTimeout = 1,
	TimerGlobalSearch
};

enum ESearchResultImage
{
	sriServerActive,
	sriGlobalActive,
	sriKadActice,
	sriClient,
	sriServer,
	sriGlobal,
	sriKad
};

#define	SEARCH_LIST_MENU_BUTTON_XOFF	8
#define	SEARCH_LIST_MENU_BUTTON_WIDTH	170
#define	SEARCH_LIST_MENU_BUTTON_HEIGHT	22	// don't set the height do something different than 22 unless you know exactly what you are doing!

// CSearchResultsWnd dialog

IMPLEMENT_DYNCREATE(CSearchResultsWnd, CResizableFormView)

BEGIN_MESSAGE_MAP(CSearchResultsWnd, CResizableFormView)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_SDOWNLOAD, OnBnClickedDownloadSelected)
	ON_BN_CLICKED(IDC_CLEARALL, OnBnClickedClearAll)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelChangeTab)
	ON_MESSAGE(UM_CLOSETAB, OnCloseTab)
	ON_MESSAGE(UM_DBLCLICKTAB, OnDblClickTab)
	ON_WM_DESTROY()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_WM_HELPINFO()
	ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
	ON_BN_CLICKED(IDC_OPEN_PARAMS_WND, OnBnClickedOpenParamsWnd)
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(UM_DELAYED_EVALUATE, OnChangeFilter)
	ON_NOTIFY(TBN_DROPDOWN, IDC_SEARCHLST_ICO, OnSearchListMenuBtnDropDown)
END_MESSAGE_MAP()

CSearchResultsWnd::CSearchResultsWnd(CWnd* /*pParent*/)
	: CResizableFormView(CSearchResultsWnd::IDD)
{
	m_nEd2kSearchID = 0x80000000;
	global_search_timer = 0;
	searchpacket = NULL;
	m_b64BitSearchPacket = false;
	canceld = false;
	servercount = 0;
	globsearch = false;
	m_uTimerLocalServer = 0;
	m_iSentMoreReq = 0;
	searchselect.m_bCloseable = true;
	m_btnSearchListMenu = new CDropDownButton;
	m_nFilterColumn = 0;
	m_cattabs = new CButtonsTabCtrl;
}

CSearchResultsWnd::~CSearchResultsWnd()
{
	delete m_cattabs;
	m_ctlSearchListHeader.Detach();
	delete m_btnSearchListMenu;
	if (globsearch)
		delete searchpacket;
	if (m_uTimerLocalServer)
		VERIFY( KillTimer(m_uTimerLocalServer) );
}

void CSearchResultsWnd::OnInitialUpdate()
{
	CResizableFormView::OnInitialUpdate();
	InitWindowStyles(this);
	theApp.searchlist->SetOutputWnd(&searchlistctrl);
	m_ctlSearchListHeader.Attach(searchlistctrl.GetHeaderCtrl()->Detach());
	searchlistctrl.Init(theApp.searchlist);
	searchlistctrl.SetPrefsKey(_T("SearchListCtrl"));

	CRect rc;
	rc.top = 2;
	rc.left = SEARCH_LIST_MENU_BUTTON_XOFF;
	rc.right = rc.left + SEARCH_LIST_MENU_BUTTON_WIDTH;
	rc.bottom = rc.top + SEARCH_LIST_MENU_BUTTON_HEIGHT;
	m_btnSearchListMenu->Init(true, true);
	m_btnSearchListMenu->MoveWindow(&rc);
	m_btnSearchListMenu->AddBtnStyle(IDC_SEARCHLST_ICO, TBSTYLE_AUTOSIZE);
	// Vista: Remove the TBSTYLE_TRANSPARENT to avoid flickering (can be done only after the toolbar was initially created with TBSTYLE_TRANSPARENT !?)
	m_btnSearchListMenu->ModifyStyle(TBSTYLE_TOOLTIPS | ((theApp.m_ullComCtrlVer >= MAKEDLLVERULL(6, 16, 0, 0)) ? TBSTYLE_TRANSPARENT : 0), 0);
	m_btnSearchListMenu->SetExtendedStyle(m_btnSearchListMenu->GetExtendedStyle() & ~TBSTYLE_EX_MIXEDBUTTONS);
	m_btnSearchListMenu->RecalcLayout(true);

	m_ctlFilter.OnInit(&m_ctlSearchListHeader);

	SetAllIcons();
	Localize();
	searchprogress.SetStep(1);
	global_search_timer = 0;
	globsearch = false;

	AddAnchor(*m_btnSearchListMenu, TOP_LEFT);
	AddAnchor(IDC_FILTER, TOP_RIGHT);
	AddAnchor(IDC_SDOWNLOAD, BOTTOM_LEFT);
	AddAnchor(IDC_SEARCHLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS1, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CLEARALL, BOTTOM_RIGHT);
	AddAnchor(IDC_OPEN_PARAMS_WND, TOP_RIGHT);
	AddAnchor(searchselect.m_hWnd, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC_DLTOof, BOTTOM_LEFT);
	AddAnchor(*m_cattabs, BOTTOM_LEFT, BOTTOM_RIGHT);

	ShowSearchSelector(false);

	if (theApp.m_fontSymbol.m_hObject){
		GetDlgItem(IDC_STATIC_DLTOof)->SetFont(&theApp.m_fontSymbol);
		GetDlgItem(IDC_STATIC_DLTOof)->SetWindowText(GetExStyle() & WS_EX_LAYOUTRTL ? _T("3") : _T("4")); // show a right-arrow
	}
}

void CSearchResultsWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SEARCHLIST, searchlistctrl);
	DDX_Control(pDX, IDC_PROGRESS1, searchprogress);
	DDX_Control(pDX, IDC_TAB1, searchselect);
	DDX_Control(pDX, IDC_CATTAB2, *m_cattabs);
	DDX_Control(pDX, IDC_FILTER, m_ctlFilter);
	DDX_Control(pDX, IDC_OPEN_PARAMS_WND, m_ctlOpenParamsWnd);
	DDX_Control(pDX, IDC_SEARCHLST_ICO, *m_btnSearchListMenu);
}

void CSearchResultsWnd::StartSearch(SSearchParams* pParams)
{
	switch (pParams->eType)
	{
		case SearchTypeAutomatic:
		case SearchTypeEd2kServer:
		case SearchTypeEd2kGlobal:
		case SearchTypeKademlia:
			StartNewSearch(pParams);
			break;

		case SearchTypeContentDB:
			ShellOpenFile(CreateWebQuery(pParams));
			delete pParams;
			return;

		default:
			ASSERT(0);
			delete pParams;
	}
}

void CSearchResultsWnd::OnTimer(UINT nIDEvent)
{
	CResizableFormView::OnTimer(nIDEvent);

	if (m_uTimerLocalServer != 0 && nIDEvent == m_uTimerLocalServer)
	{
		if (thePrefs.GetDebugServerSearchesLevel() > 0)
			Debug(_T("Timeout waiting on search results of local server\n"));
		// the local server did not answer within the timeout
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;

		// start the global search
		if (globsearch)
		{
			if (global_search_timer == 0)
				VERIFY( (global_search_timer = SetTimer(TimerGlobalSearch, 750, 0)) != NULL );
		}
		else
			CancelEd2kSearch();
	}
	else if (nIDEvent == global_search_timer)
	{
	    if (theApp.serverconnect->IsConnected())
		{
			CServer* pConnectedServer = theApp.serverconnect->GetCurrentServer();
			if (pConnectedServer)
				pConnectedServer = theApp.serverlist->GetServerByAddress(pConnectedServer->GetAddress(), pConnectedServer->GetPort());

			CServer* toask = NULL;
			while (servercount < theApp.serverlist->GetServerCount()-1)
			{
				servercount++;
				searchprogress.StepIt();

				toask = theApp.serverlist->GetNextSearchServer();
				if (toask == NULL)
					break;
				if (toask == pConnectedServer) {
					toask = NULL;
					continue;
				}
				if (toask->GetFailedCount() >= thePrefs.GetDeadServerRetries()) {
					toask = NULL;
					continue;
				}
				break;
			}

			if (toask)
			{
				bool bRequestSent = false;
				if (toask->SupportsLargeFilesUDP() && (toask->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES))
				{
					CSafeMemFile data(50);
					uint32 nTagCount = 1;
					data.WriteUInt32(nTagCount);
					CTag tagFlags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
					tagFlags.WriteNewEd2kTag(&data);
					Packet* pExtSearchPacket = new Packet(OP_GLOBSEARCHREQ3, searchpacket->size + (uint32)data.GetLength());
					data.SeekToBegin();
					data.Read(pExtSearchPacket->pBuffer, (uint32)data.GetLength());
					memcpy(pExtSearchPacket->pBuffer+(uint32)data.GetLength(), searchpacket->pBuffer, searchpacket->size);
					theStats.AddUpDataOverheadServer(pExtSearchPacket->size);
					theApp.serverconnect->SendUDPPacket(pExtSearchPacket, toask, true);
					bRequestSent = true;
					if (thePrefs.GetDebugServerUDPLevel() > 0)
						Debug(_T(">>> Sending %s  to server %-21s (%3u of %3u)\n"),  _T("OP__GlobSearchReq3"), ipstr(toask->GetAddress(), toask->GetPort()), servercount, theApp.serverlist->GetServerCount());

				}
				else if (toask->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)
				{
					if (!m_b64BitSearchPacket || toask->SupportsLargeFilesUDP()){
						searchpacket->opcode = OP_GLOBSEARCHREQ2;
						if (thePrefs.GetDebugServerUDPLevel() > 0)
							Debug(_T(">>> Sending %s  to server %-21s (%3u of %3u)\n"), _T("OP__GlobSearchReq2"), ipstr(toask->GetAddress(), toask->GetPort()), servercount, theApp.serverlist->GetServerCount());
						theStats.AddUpDataOverheadServer(searchpacket->size);
						theApp.serverconnect->SendUDPPacket(searchpacket, toask, false);
						bRequestSent = true;
					}
					else{
						if (thePrefs.GetDebugServerUDPLevel() > 0)
							Debug(_T(">>> Skipped UDP search on server %-21s (%3u of %3u): No large file support\n"), ipstr(toask->GetAddress(), toask->GetPort()), servercount, theApp.serverlist->GetServerCount());
					}
				}
				else
				{
					if (!m_b64BitSearchPacket || toask->SupportsLargeFilesUDP()){
						searchpacket->opcode = OP_GLOBSEARCHREQ;
						if (thePrefs.GetDebugServerUDPLevel() > 0)
							Debug(_T(">>> Sending %s  to server %-21s (%3u of %3u)\n"), _T("OP__GlobSearchReq1"), ipstr(toask->GetAddress(), toask->GetPort()), servercount, theApp.serverlist->GetServerCount());
						theStats.AddUpDataOverheadServer(searchpacket->size);
						theApp.serverconnect->SendUDPPacket(searchpacket, toask, false);
						bRequestSent = true;
					}
					else{
						if (thePrefs.GetDebugServerUDPLevel() > 0)
							Debug(_T(">>> Skipped UDP search on server %-21s (%3u of %3u): No large file support\n"), ipstr(toask->GetAddress(), toask->GetPort()), servercount, theApp.serverlist->GetServerCount());
					}
				}
				if (bRequestSent)
					theApp.searchlist->SentUDPRequestNotification(m_nEd2kSearchID, toask->GetIP());
			}
			else
				CancelEd2kSearch();
	    }
	    else
			CancelEd2kSearch();
    }
	else
		ASSERT( 0 );
}

void CSearchResultsWnd::SetSearchResultsIcon(UINT uSearchID, int iImage)
{
    int iTabItems = searchselect.GetItemCount();
    for (int i = 0; i < iTabItems; i++)
	{
        TCITEM tci;
        tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL && ((const SSearchParams*)tci.lParam)->dwSearchID == uSearchID)
		{
			tci.mask = TCIF_IMAGE;
			tci.iImage = iImage;
			searchselect.SetItem(i, &tci);
			break;
		}
    }
}

void CSearchResultsWnd::SetActiveSearchResultsIcon(UINT uSearchID)
{
	SSearchParams* pParams = GetSearchResultsParams(uSearchID);
	if (pParams)
	{
		int iImage;
		if (pParams->eType == SearchTypeKademlia)
			iImage = sriKadActice;
		else if (pParams->eType == SearchTypeEd2kGlobal)
			iImage = sriGlobalActive;
		else
			iImage = sriServerActive;
		SetSearchResultsIcon(uSearchID, iImage);
	}
}

void CSearchResultsWnd::SetInactiveSearchResultsIcon(UINT uSearchID)
{
	SSearchParams* pParams = GetSearchResultsParams(uSearchID);
	if (pParams)
	{
		int iImage;
		if (pParams->eType == SearchTypeKademlia)
			iImage = sriKad;
		else if (pParams->eType == SearchTypeEd2kGlobal)
			iImage = sriGlobal;
		else
			iImage = sriServer;
		SetSearchResultsIcon(uSearchID, iImage);
	}
}

SSearchParams* CSearchResultsWnd::GetSearchResultsParams(UINT uSearchID) const
{
    int iTabItems = searchselect.GetItemCount();
    for (int i = 0; i < iTabItems; i++)
	{
        TCITEM tci;
        tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL && ((const SSearchParams*)tci.lParam)->dwSearchID == uSearchID)
			return (SSearchParams*)tci.lParam;
    }
	return NULL;
}

void CSearchResultsWnd::CancelSearch(UINT uSearchID)
{
	if (uSearchID == 0)
	{
		int iCurSel = searchselect.GetCurSel();
		if (iCurSel == -1)
			return;
		TCITEM item;
		item.mask = TCIF_PARAM;
		if (searchselect.GetItem(iCurSel, &item) && item.lParam != NULL)
			uSearchID = ((const SSearchParams*)item.lParam)->dwSearchID;
		if (uSearchID == 0)
			return;
	}

	SSearchParams* pParams = GetSearchResultsParams(uSearchID);
	if (pParams == NULL)
		return;

	if (pParams->eType == SearchTypeEd2kServer || pParams->eType == SearchTypeEd2kGlobal)
		CancelEd2kSearch();
	else if (pParams->eType == SearchTypeKademlia)
	{
		Kademlia::CSearchManager::StopSearch(pParams->dwSearchID, false);
		CancelKadSearch(pParams->dwSearchID);
	}
}

void CSearchResultsWnd::CancelEd2kSearch()
{
	SetInactiveSearchResultsIcon(m_nEd2kSearchID);

	canceld = true;

	// delete any global search timer
	if (globsearch){
		delete searchpacket;
		searchpacket = NULL;
		m_b64BitSearchPacket = false;
	}
	globsearch = false;
	if (global_search_timer){
		VERIFY( KillTimer(global_search_timer) );
		global_search_timer = 0;
		searchprogress.SetPos(0);
	}

	// delete local server timeout timer
	if (m_uTimerLocalServer){
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;
	}

	SearchCanceled(m_nEd2kSearchID);
}

void CSearchResultsWnd::CancelKadSearch(UINT uSearchID)
{
	SearchCanceled(uSearchID);
}

void CSearchResultsWnd::SearchStarted()
{
	CWnd* pWndFocus = GetFocus();
	m_pwndParams->m_ctlStart.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd  == m_pwndParams->m_ctlStart.m_hWnd)
		m_pwndParams->m_ctlName.SetFocus();
	m_pwndParams->m_ctlCancel.EnableWindow(TRUE);
}

void CSearchResultsWnd::SearchCanceled(UINT uSearchID)
{
	SetInactiveSearchResultsIcon(uSearchID);

	int iCurSel = searchselect.GetCurSel();
	if (iCurSel != -1)
	{
		TCITEM item;
		item.mask = TCIF_PARAM;
		if (searchselect.GetItem(iCurSel, &item) && item.lParam != NULL && uSearchID == ((const SSearchParams*)item.lParam)->dwSearchID)
		{
			CWnd* pWndFocus = GetFocus();
			m_pwndParams->m_ctlCancel.EnableWindow(FALSE);
			if (pWndFocus && pWndFocus->m_hWnd == m_pwndParams->m_ctlCancel.m_hWnd)
				m_pwndParams->m_ctlName.SetFocus();
			m_pwndParams->m_ctlStart.EnableWindow(m_pwndParams->m_ctlName.GetWindowTextLength() > 0);
		}
	}
}

void CSearchResultsWnd::LocalEd2kSearchEnd(UINT count, bool bMoreResultsAvailable)
{
	// local server has answered, kill the timeout timer
	if (m_uTimerLocalServer) {
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;
	}

	if (!canceld && count > MAX_RESULTS)
		CancelEd2kSearch();
	if (!canceld) {
		if (!globsearch)
			SearchCanceled(m_nEd2kSearchID);
		else
			VERIFY( (global_search_timer = SetTimer(TimerGlobalSearch, 750, 0)) != NULL );
	}
	m_pwndParams->m_ctlMore.EnableWindow(bMoreResultsAvailable && m_iSentMoreReq < MAX_MORE_SEARCH_REQ);
}

void CSearchResultsWnd::AddGlobalEd2kSearchResults(UINT count)
{
	if (!canceld && count > MAX_RESULTS)
		CancelEd2kSearch();
}

void CSearchResultsWnd::OnBnClickedDownloadSelected()
{
	//start download(s)
	DownloadSelected();
}

void CSearchResultsWnd::OnDblClkSearchList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	OnBnClickedDownloadSelected();
	*pResult = 0;
}

CString	CSearchResultsWnd::CreateWebQuery(SSearchParams* pParams)
{
	CString query;
	switch (pParams->eType)
	{
	case SearchTypeContentDB:
		query = _T("http://contentdb.emule-project.net/search.php?");
		query += _T("s=") + EncodeURLQueryParam(pParams->strExpression);
		if (pParams->strFileType == ED2KFTSTR_AUDIO)
			query += _T("&cat=2");
		else if (pParams->strFileType == ED2KFTSTR_VIDEO)
			query += _T("&cat=3");
		else if (pParams->strFileType == ED2KFTSTR_PROGRAM)
			query += _T("&cat=1");
		else
			query += _T("&cat=all");
		query += _T("&rel=1&search_option=simple&network=edonkey&go=Search");
		break;
	default:
		return _T("");
	}
	return query;
}

void CSearchResultsWnd::DownloadSelected()
{
	DownloadSelected(thePrefs.AddNewFilesPaused());
}

void CSearchResultsWnd::DownloadSelected(bool bPaused)
{
	CWaitCursor curWait;
	POSITION pos = searchlistctrl.GetFirstSelectedItemPosition();
	while (pos != NULL)
	{
		int iIndex = searchlistctrl.GetNextSelectedItem(pos);
		if (iIndex >= 0)
		{
			// get selected listview item (may be a child item from an expanded search result)
			const CSearchFile* sel_file = (CSearchFile*)searchlistctrl.GetItemData(iIndex);

			// get parent
			const CSearchFile* parent;
			if (sel_file->GetListParent() != NULL)
				parent = sel_file->GetListParent();
			else
				parent = sel_file;

			if (parent->IsComplete() == 0 && parent->GetSourceCount() >= 50)
			{
				CString strMsg;
				strMsg.Format(GetResString(IDS_ASKDLINCOMPLETE), sel_file->GetFileName());
				int iAnswer = AfxMessageBox(strMsg, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
				if (iAnswer != IDYES)
					continue;
			}

			// create new DL-queue entry with all properties of parent (e.g. already received sources!)
			// but with the filename of the selected listview item.
			CSearchFile tempFile(parent);
			tempFile.SetFileName(sel_file->GetFileName());
			tempFile.SetStrTagValue(FT_FILENAME, sel_file->GetFileName());
			theApp.downloadqueue->AddSearchToDownload(&tempFile, bPaused, GetSelectedCat());

			// update parent and all childs
			searchlistctrl.UpdateSources(parent);
		}
	}
}

void CSearchResultsWnd::OnSysColorChange()
{
	CResizableFormView::OnSysColorChange();
	SetAllIcons();
	searchlistctrl.CreateMenues();
}

void CSearchResultsWnd::SetAllIcons()
{
	m_btnSearchListMenu->SetIcon(_T("SearchResults"));

	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("SearchMethod_ServerActive")));
	iml.Add(CTempIconLoader(_T("SearchMethod_GlobalActive")));
	iml.Add(CTempIconLoader(_T("SearchMethod_KademliaActive")));
	iml.Add(CTempIconLoader(_T("StatsClients")));
	iml.Add(CTempIconLoader(_T("SearchMethod_SERVER")));
	iml.Add(CTempIconLoader(_T("SearchMethod_GLOBAL")));
	iml.Add(CTempIconLoader(_T("SearchMethod_KADEMLIA")));
	searchselect.SetImageList(&iml);
	m_imlSearchResults.DeleteImageList();
	m_imlSearchResults.Attach(iml.Detach());
	searchselect.SetPadding(CSize(12, 3));
}

void CSearchResultsWnd::Localize()
{
	searchlistctrl.Localize();
	m_ctlFilter.ShowColumnText(true);
	UpdateCatTabs();

    GetDlgItem(IDC_CLEARALL)->SetWindowText(GetResString(IDS_REMOVEALLSEARCH));
	m_btnSearchListMenu->SetWindowText(GetResString(IDS_SW_RESULT));
    GetDlgItem(IDC_SDOWNLOAD)->SetWindowText(GetResString(IDS_SW_DOWNLOAD));
	m_ctlOpenParamsWnd.SetWindowText(GetResString(IDS_SEARCHPARAMS)+_T("..."));
}

void CSearchResultsWnd::OnBnClickedClearAll()
{
	DeleteAllSearches();
}

CString DbgGetFileMetaTagName(UINT uMetaTagID)
{
	switch (uMetaTagID)
	{
		case FT_FILENAME:			return _T("@Name");
		case FT_FILESIZE:			return _T("@Size");
		case FT_FILESIZE_HI:		return _T("@SizeHI");
		case FT_FILETYPE:			return _T("@Type");
		case FT_FILEFORMAT:			return _T("@Format");
		case FT_LASTSEENCOMPLETE:	return _T("@LastSeenComplete");
		case FT_SOURCES:			return _T("@Sources");
		case FT_COMPLETE_SOURCES:	return _T("@Complete");
		case FT_MEDIA_ARTIST:		return _T("@Artist");
		case FT_MEDIA_ALBUM:		return _T("@Album");
		case FT_MEDIA_TITLE:		return _T("@Title");
		case FT_MEDIA_LENGTH:		return _T("@Length");
		case FT_MEDIA_BITRATE:		return _T("@Bitrate");
		case FT_MEDIA_CODEC:		return _T("@Codec");
		case FT_FILECOMMENT:		return _T("@Comment");
		case FT_FILERATING:			return _T("@Rating");
		case FT_FILEHASH:			return _T("@Filehash");
	}

	CString buffer;
	buffer.Format(_T("Tag0x%02X"), uMetaTagID);
	return buffer;
}

CString DbgGetFileMetaTagName(LPCSTR pszMetaTagID)
{
	if (strlen(pszMetaTagID) == 1)
		return DbgGetFileMetaTagName(((BYTE*)pszMetaTagID)[0]);
	CString strName;
	strName.Format(_T("\"%hs\""), pszMetaTagID);
	return strName;
}

CString DbgGetSearchOperatorName(UINT uOperator)
{
	static const LPCTSTR _aszEd2kOps[] = 
	{
		_T("="),
		_T(">"),
		_T("<"),
		_T(">="),
		_T("<="),
		_T("<>"),
	};

	if (uOperator >= ARRSIZE(_aszEd2kOps)){
		ASSERT(0);
		return _T("*UnkOp*");
	}
	return _aszEd2kOps[uOperator];
}

static CStringA s_strCurKadKeywordA;
static CSearchExpr s_SearchExpr;
CStringArray g_astrParserErrors;

static TCHAR s_chLastChar = 0;
static CString s_strSearchTree;

bool DumpSearchTree(int& iExpr, const CSearchExpr& rSearchExpr, int iLevel, bool bFlat)
{
	if (iExpr >= rSearchExpr.m_aExpr.GetCount())
		return false;
	if (!bFlat)
		s_strSearchTree += _T('\n') + CString(_T(' '), iLevel);
	const CSearchAttr& rSearchAttr = rSearchExpr.m_aExpr[iExpr++];
	CStringA strTok = rSearchAttr.m_str;
	if (strTok == SEARCHOPTOK_AND || strTok == SEARCHOPTOK_OR || strTok == SEARCHOPTOK_NOT)
	{
		if (bFlat) {
			if (s_chLastChar != _T('(') && s_chLastChar != _T('\0'))
				s_strSearchTree.AppendFormat(_T(" "));
		}
		s_strSearchTree.AppendFormat(_T("(%hs "), strTok.Mid(1));
		s_chLastChar = _T('(');
		DumpSearchTree(iExpr, rSearchExpr, iLevel + 4, bFlat);
		DumpSearchTree(iExpr, rSearchExpr, iLevel + 4, bFlat);
		s_strSearchTree.AppendFormat(_T(")"));
		s_chLastChar = _T(')');
	}
	else
	{
		if (bFlat) {
			if (s_chLastChar != _T('(') && s_chLastChar != _T('\0'))
				s_strSearchTree.AppendFormat(_T(" "));
		}
		s_strSearchTree += rSearchAttr.DbgGetAttr();
		s_chLastChar = _T('\1');
	}
	return true;
}

bool DumpSearchTree(const CSearchExpr& rSearchExpr, bool bFlat)
{
	s_chLastChar = _T('\0');
	int iExpr = 0;
	int iLevel = 0;
	return DumpSearchTree(iExpr, rSearchExpr, iLevel, bFlat);
}

void ParsedSearchExpression(const CSearchExpr* pexpr)
{
	int iOpAnd = 0;
	int iOpOr = 0;
	int iOpNot = 0;
	int iNonDefTags = 0;
	//CStringA strDbg;
	for (int i = 0; i < pexpr->m_aExpr.GetCount(); i++)
	{
		const CSearchAttr& rSearchAttr = pexpr->m_aExpr[i];
		const CStringA& rstr = rSearchAttr.m_str;
		if (rstr == SEARCHOPTOK_AND)
		{
			iOpAnd++;
			//strDbg.AppendFormat("%s ", rstr.Mid(1));
		}
		else if (rstr == SEARCHOPTOK_OR)
		{
			iOpOr++;
			//strDbg.AppendFormat("%s ", rstr.Mid(1));
		}
		else if (rstr == SEARCHOPTOK_NOT)
		{
			iOpNot++;
			//strDbg.AppendFormat("%s ", rstr.Mid(1));
		}
		else
		{
			if (rSearchAttr.m_iTag != FT_FILENAME)
				iNonDefTags++;
			//strDbg += rSearchAttr.DbgGetAttr() + " ";
		}
	}
	//if (thePrefs.GetDebugServerSearchesLevel() > 0)
	//	Debug(_T("Search Expr: %hs\n"), strDbg);

	// this limit (+ the additional operators which will be added later) has to match the limit in 'CreateSearchExpressionTree'
	//	+1 Type (Audio, Video)
	//	+1 MinSize
	//	+1 MaxSize
	//	+1 Avail
	//	+1 Extension
	//	+1 Complete sources
	//	+1 Codec
	//	+1 Bitrate
	//	+1 Length
	//	+1 Title
	//	+1 Album
	//	+1 Artist
	// ---------------
	//  12
	if (iOpAnd + iOpOr + iOpNot > 10)
		yyerror(GetResString(IDS_SEARCH_TOOCOMPLEX));
	
	// FIXME: When searching on Kad the keyword may not be included into the OR operator in anyway (or into the not part of NAND)
	// Currently we do not check this properly for all cases but only for the most common ones and more important we
	// do not try to rearrange keywords, which could make a search valid
	if (!s_strCurKadKeywordA.IsEmpty() && iOpOr > 0)
	{
		if (iOpAnd + iOpNot > 0)
		{
			if (pexpr->m_aExpr.GetCount() > 2)
			{
				if (pexpr->m_aExpr[0].m_str == SEARCHOPTOK_OR && pexpr->m_aExpr[1].m_str == s_strCurKadKeywordA)
					yyerror(GetResString(IDS_SEARCH_BADOPERATORCOMBINATION));
			}
		}
		else // if we habe only OR its not going to work out for sure
			yyerror(GetResString(IDS_SEARCH_BADOPERATORCOMBINATION));
	}

	s_SearchExpr.m_aExpr.RemoveAll();
	// optimize search expression, if no OR nor NOT specified
	if (iOpAnd > 0 && iOpOr == 0 && iOpNot == 0 && iNonDefTags == 0)
	{
		
		// figure out if we can use a better keyword than the one the user selected
		// for example most user will search like this "The oxymoronaccelerator 2", which would ask the node which indexes "the"
		// This causes higher traffic for such nodes and makes them a viable target to attackers, while the kad result should be
		// the same or even better if we ask the node which indexes the rare keyword "oxymoronaccelerator", so we try to rearrenge
		// keywords and generally assume that the longer keywords are rarer
		if (thePrefs.GetRearrangeKadSearchKeywords() && !s_strCurKadKeywordA.IsEmpty())
		{
			for (int i = 0; i < pexpr->m_aExpr.GetCount(); i++)
			{
				if (pexpr->m_aExpr[i].m_str != SEARCHOPTOK_AND)
				{
					if (pexpr->m_aExpr[i].m_str != s_strCurKadKeywordA 
						&& pexpr->m_aExpr[i].m_str.FindOneOf(g_aszInvKadKeywordCharsA) == (-1)
						&& pexpr->m_aExpr[i].m_str.Find('"') != 0 // no quoted expressions as keyword
						&& pexpr->m_aExpr[i].m_str.GetLength() >= 3
						&& s_strCurKadKeywordA.GetLength() < pexpr->m_aExpr[i].m_str.GetLength())
					{
						s_strCurKadKeywordA = pexpr->m_aExpr[i].m_str;
					}
				}
			}
		}

		CStringA strAndTerms;
		for (int i = 0; i < pexpr->m_aExpr.GetCount(); i++)
		{
			if (pexpr->m_aExpr[i].m_str != SEARCHOPTOK_AND)
			{
				ASSERT( pexpr->m_aExpr[i].m_iTag == FT_FILENAME );
				// Minor optimization: Because we added the Kad keyword to the boolean search expression,
				// we remove it here (and only here) again because we know that the entire search expression
				// does only contain (implicit) ANDed strings.
				if (pexpr->m_aExpr[i].m_str != s_strCurKadKeywordA)
				{
					if (!strAndTerms.IsEmpty())
						strAndTerms += ' ';
					strAndTerms += pexpr->m_aExpr[i].m_str;
				}
			}
		}
		ASSERT( s_SearchExpr.m_aExpr.GetCount() == 0);
		s_SearchExpr.m_aExpr.Add(CSearchAttr(strAndTerms));
	}
	else
	{
		if (pexpr->m_aExpr.GetCount() != 1
			|| !(pexpr->m_aExpr[0].m_iTag == FT_FILENAME && pexpr->m_aExpr[0].m_str == s_strCurKadKeywordA))
			s_SearchExpr.m_aExpr.Append(pexpr->m_aExpr);
	}
}

class CSearchExprTarget
{
public:
	CSearchExprTarget(CSafeMemFile* pData, EUtf8Str eStrEncode, bool bSupports64Bit, bool* pbPacketUsing64Bit)
	{
		m_data = pData;
		m_eStrEncode = eStrEncode;
		m_bSupports64Bit = bSupports64Bit;
		m_pbPacketUsing64Bit = pbPacketUsing64Bit;
		if (m_pbPacketUsing64Bit)
			*m_pbPacketUsing64Bit = false;
	}

	const CString& GetDebugString() const
	{
		return m_strDbg;
	}

	void WriteBooleanAND()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x00);			// "AND"
		m_strDbg.AppendFormat(_T("AND "));
	}

	void WriteBooleanOR()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x01);			// "OR"
		m_strDbg.AppendFormat(_T("OR "));
	}

	void WriteBooleanNOT()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x02);			// "NOT"
		m_strDbg.AppendFormat(_T("NOT "));
	}

	void WriteMetaDataSearchParam(const CString& rstrValue)
	{
		m_data->WriteUInt8(1);						// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_strDbg.AppendFormat(_T("\"%s\" "), rstrValue);
	}

	void WriteMetaDataSearchParam(UINT uMetaTagID, const CString& rstrValue)
	{
		m_data->WriteUInt8(2);						// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteUInt16(sizeof uint8);			// meta tag ID length
		m_data->WriteUInt8((uint8)uMetaTagID);		// meta tag ID name
		m_strDbg.AppendFormat(_T("%s=\"%s\" "), DbgGetFileMetaTagName(uMetaTagID), rstrValue);
	}

	void WriteMetaDataSearchParamA(UINT uMetaTagID, const CStringA& rstrValueA)
	{
		m_data->WriteUInt8(2);						// string parameter type
		m_data->WriteString(rstrValueA);			// string value
		m_data->WriteUInt16(sizeof uint8);			// meta tag ID length
		m_data->WriteUInt8((uint8)uMetaTagID);		// meta tag ID name
		m_strDbg.AppendFormat(_T("%s=\"%hs\" "), DbgGetFileMetaTagName(uMetaTagID), rstrValueA);
	}

	void WriteMetaDataSearchParam(LPCSTR pszMetaTagID, const CString& rstrValue)
	{
		m_data->WriteUInt8(2);						// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteString(pszMetaTagID);			// meta tag ID
		m_strDbg.AppendFormat(_T("%s=\"%s\" "), DbgGetFileMetaTagName(pszMetaTagID), rstrValue);
	}

	void WriteMetaDataSearchParam(UINT uMetaTagID, UINT uOperator, uint64 ullValue)
	{
		bool b64BitValue = ullValue > 0xFFFFFFFFui64;
		if (b64BitValue && m_bSupports64Bit) {
			if (m_pbPacketUsing64Bit)
				*m_pbPacketUsing64Bit = true;
			m_data->WriteUInt8(8);					// numeric parameter type (int64)
			m_data->WriteUInt64(ullValue);			// numeric value
		}
		else {
			if (b64BitValue)
				ullValue = 0xFFFFFFFFU;
			m_data->WriteUInt8(3);					// numeric parameter type (int32)
			m_data->WriteUInt32((uint32)ullValue);	// numeric value
		}
		m_data->WriteUInt8((uint8)uOperator);	// comparison operator
		m_data->WriteUInt16(sizeof uint8);		// meta tag ID length
		m_data->WriteUInt8((uint8)uMetaTagID);	// meta tag ID name
		m_strDbg.AppendFormat(_T("%s%s%I64u "), DbgGetFileMetaTagName(uMetaTagID), DbgGetSearchOperatorName(uOperator), ullValue);
	}

	void WriteMetaDataSearchParam(LPCSTR pszMetaTagID, UINT uOperator, uint64 ullValue)
	{
		bool b64BitValue = ullValue > 0xFFFFFFFFui64;
		if (b64BitValue && m_bSupports64Bit) {
			if (m_pbPacketUsing64Bit)
				*m_pbPacketUsing64Bit = true;
			m_data->WriteUInt8(8);					// numeric parameter type (int64)
			m_data->WriteUInt64(ullValue);			// numeric value
		}
		else {
			if (b64BitValue)
				ullValue = 0xFFFFFFFFU;
			m_data->WriteUInt8(3);					// numeric parameter type (int32)
			m_data->WriteUInt32((uint32)ullValue);	// numeric value
		}
		m_data->WriteUInt8((uint8)uOperator);	// comparison operator
		m_data->WriteString(pszMetaTagID);		// meta tag ID
		m_strDbg.AppendFormat(_T("%s%s%I64u "), DbgGetFileMetaTagName(pszMetaTagID), DbgGetSearchOperatorName(uOperator), ullValue);
	}

protected:
	CSafeMemFile* m_data;
	CString m_strDbg;
	EUtf8Str m_eStrEncode;
	bool m_bSupports64Bit;
	bool* m_pbPacketUsing64Bit;
};

static CSearchExpr s_SearchExpr2;

static void AddAndAttr(UINT uTag, const CString& rstr)
{
	s_SearchExpr2.m_aExpr.InsertAt(0, CSearchAttr(uTag, StrToUtf8(rstr)));
	if (s_SearchExpr2.m_aExpr.GetCount() > 1)
		s_SearchExpr2.m_aExpr.InsertAt(0, CSearchAttr(SEARCHOPTOK_AND));
}

static void AddAndAttr(UINT uTag, UINT uOpr, uint64 ullVal)
{
	s_SearchExpr2.m_aExpr.InsertAt(0, CSearchAttr(uTag, uOpr, ullVal));
	if (s_SearchExpr2.m_aExpr.GetCount() > 1)
		s_SearchExpr2.m_aExpr.InsertAt(0, CSearchAttr(SEARCHOPTOK_AND));
}

bool GetSearchPacket(CSafeMemFile* pData, SSearchParams* pParams, bool bTargetSupports64Bit, bool* pbPacketUsing64Bit)
{
	CStringA strFileType;
	if (pParams->strFileType == ED2KFTSTR_ARCHIVE){
		// eDonkeyHybrid 0.48 uses type "Pro" for archives files
		// www.filedonkey.com uses type "Pro" for archives files
		strFileType = ED2KFTSTR_PROGRAM;
	}
	else if (pParams->strFileType == ED2KFTSTR_CDIMAGE){
		// eDonkeyHybrid 0.48 uses *no* type for iso/nrg/cue/img files
		// www.filedonkey.com uses type "Pro" for CD-image files
		strFileType = ED2KFTSTR_PROGRAM;
	}
	else{
		//TODO: Support "Doc" types
		strFileType = pParams->strFileType;
	}

	s_strCurKadKeywordA.Empty();
	ASSERT( !pParams->strExpression.IsEmpty() );
	if (pParams->eType == SearchTypeKademlia)
	{
		ASSERT( !pParams->strKeyword.IsEmpty() );
		s_strCurKadKeywordA = StrToUtf8(pParams->strKeyword);
	}
	if (pParams->strBooleanExpr.IsEmpty())
		pParams->strBooleanExpr = pParams->strExpression;
	if (pParams->strBooleanExpr.IsEmpty())
		return false;

	//TRACE(_T("Raw search expr:\n"));
	//TRACE(_T("%s"), pParams->strBooleanExpr);
	//TRACE(_T("  %s\n"), DbgGetHexDump((uchar*)(LPCTSTR)pParams->strBooleanExpr, pParams->strBooleanExpr.GetLength()*sizeof(TCHAR)));
	g_astrParserErrors.RemoveAll();
	s_SearchExpr.m_aExpr.RemoveAll();
	if (!pParams->strBooleanExpr.IsEmpty())
	{
	    LexInit(pParams->strBooleanExpr, true);
	    int iParseResult = yyparse();
	    LexFree();
	    if (g_astrParserErrors.GetSize() > 0)
		{
		    s_SearchExpr.m_aExpr.RemoveAll();
			CString strError(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + g_astrParserErrors[g_astrParserErrors.GetSize() - 1]);
		    throw new CMsgBoxException(strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	    }
	    else if (iParseResult != 0)
		{
		    s_SearchExpr.m_aExpr.RemoveAll();
			CString strError(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + GetResString(IDS_SEARCH_GENERALERROR));
		    throw new CMsgBoxException(strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	    }
		if (pParams->eType == SearchTypeKademlia && s_strCurKadKeywordA != StrToUtf8(pParams->strKeyword))
		{
			DebugLog(_T("KadSearch: Keyword was rearranged, using %s instead of %s"), OptUtf8ToStr(s_strCurKadKeywordA), pParams->strKeyword);	
			pParams->strKeyword = OptUtf8ToStr(s_strCurKadKeywordA);
		}
	}
	//TRACE(_T("Parsed search expr:\n"));
	//for (int i = 0; i < s_SearchExpr.m_aExpr.GetCount(); i++){
	//	TRACE(_T("%hs"), s_SearchExpr.m_aExpr[i]);
	//	TRACE(_T("  %s\n"), DbgGetHexDump((uchar*)(LPCSTR)s_SearchExpr.m_aExpr[i], s_SearchExpr.m_aExpr[i].GetLength()*sizeof(CHAR)));
	//}

	// create ed2k search expression
	CSearchExprTarget target(pData, utf8strRaw, bTargetSupports64Bit, pbPacketUsing64Bit);

	s_SearchExpr2.m_aExpr.RemoveAll();

	if (!pParams->strExtension.IsEmpty())
		AddAndAttr(FT_FILEFORMAT, pParams->strExtension);

	if (pParams->uAvailability > 0)
		AddAndAttr(FT_SOURCES, ED2K_SEARCH_OP_GREATER_EQUAL, pParams->uAvailability);
	
	if (pParams->ullMaxSize > 0)
		AddAndAttr(FT_FILESIZE, ED2K_SEARCH_OP_LESS_EQUAL, pParams->ullMaxSize);
    
	if (pParams->ullMinSize > 0)
		AddAndAttr(FT_FILESIZE, ED2K_SEARCH_OP_GREATER_EQUAL, pParams->ullMinSize);
    
	if (!strFileType.IsEmpty())
		AddAndAttr(FT_FILETYPE, CString(strFileType));
    
	if (pParams->uComplete > 0)
		AddAndAttr(FT_COMPLETE_SOURCES, ED2K_SEARCH_OP_GREATER_EQUAL, pParams->uComplete);

	if (pParams->ulMinBitrate > 0)
		AddAndAttr(FT_MEDIA_BITRATE, ED2K_SEARCH_OP_GREATER_EQUAL, pParams->ulMinBitrate);

	if (pParams->ulMinLength > 0)
		AddAndAttr(FT_MEDIA_LENGTH, ED2K_SEARCH_OP_GREATER_EQUAL, pParams->ulMinLength);

	if (!pParams->strCodec.IsEmpty())
		AddAndAttr(FT_MEDIA_CODEC, pParams->strCodec);

	if (!pParams->strTitle.IsEmpty())
		AddAndAttr(FT_MEDIA_TITLE, pParams->strTitle);

	if (!pParams->strAlbum.IsEmpty())
		AddAndAttr(FT_MEDIA_ALBUM, pParams->strAlbum);

	if (!pParams->strArtist.IsEmpty())
		AddAndAttr(FT_MEDIA_ARTIST, pParams->strArtist);

	if (s_SearchExpr2.m_aExpr.GetCount() > 0)
	{
		if (s_SearchExpr.m_aExpr.GetCount() > 0)
			s_SearchExpr.m_aExpr.InsertAt(0, CSearchAttr(SEARCHOPTOK_AND));
		s_SearchExpr.Add(&s_SearchExpr2);
	}

	if (thePrefs.GetVerbose())
	{
		s_strSearchTree.Empty();
		DumpSearchTree(s_SearchExpr, true);
		DebugLog(_T("Search Expr: %s"), s_strSearchTree);
	}

	for (int j = 0; j < s_SearchExpr.m_aExpr.GetCount(); j++)
	{
		const CSearchAttr& rSearchAttr = s_SearchExpr.m_aExpr[j];
		const CStringA& rstrA = rSearchAttr.m_str;
		if (rstrA == SEARCHOPTOK_AND)
		{
			target.WriteBooleanAND();
		}
		else if (rstrA == SEARCHOPTOK_OR)
		{
			target.WriteBooleanOR();
		}
		else if (rstrA == SEARCHOPTOK_NOT)
		{
			target.WriteBooleanNOT();
		}
		else if (rSearchAttr.m_iTag == FT_FILESIZE			||
				 rSearchAttr.m_iTag == FT_SOURCES			||
				 rSearchAttr.m_iTag == FT_COMPLETE_SOURCES	||
				 rSearchAttr.m_iTag == FT_FILERATING		||
				 rSearchAttr.m_iTag == FT_MEDIA_BITRATE		||
				 rSearchAttr.m_iTag == FT_MEDIA_LENGTH)
		{
			// 11-Sep-2005 []: Kad comparison operators where changed to match the ED2K operators. For backward
			// compatibility with old Kad nodes, we map ">=val" and "<=val" to ">val-1" and "<val+1". This way,
			// the older Kad nodes will perform a ">=val" and "<=val".
			//
			// TODO: This should be removed in couple of months!
			if (rSearchAttr.m_uIntegerOperator == ED2K_SEARCH_OP_GREATER_EQUAL)
				target.WriteMetaDataSearchParam(rSearchAttr.m_iTag, ED2K_SEARCH_OP_GREATER, rSearchAttr.m_nNum - 1);
			else if (rSearchAttr.m_uIntegerOperator == ED2K_SEARCH_OP_LESS_EQUAL)
				target.WriteMetaDataSearchParam(rSearchAttr.m_iTag, ED2K_SEARCH_OP_LESS, rSearchAttr.m_nNum + 1);
			else
				target.WriteMetaDataSearchParam(rSearchAttr.m_iTag, rSearchAttr.m_uIntegerOperator, rSearchAttr.m_nNum);
		}
		else if (rSearchAttr.m_iTag == FT_FILETYPE			||
				 rSearchAttr.m_iTag == FT_FILEFORMAT		||
				 rSearchAttr.m_iTag == FT_MEDIA_CODEC		||
				 rSearchAttr.m_iTag == FT_MEDIA_TITLE		|| 
				 rSearchAttr.m_iTag == FT_MEDIA_ALBUM		|| 
				 rSearchAttr.m_iTag == FT_MEDIA_ARTIST)
		{
			ASSERT( rSearchAttr.m_uIntegerOperator == ED2K_SEARCH_OP_EQUAL );
			target.WriteMetaDataSearchParam(rSearchAttr.m_iTag, OptUtf8ToStr(rSearchAttr.m_str));
		}
		else
		{
			ASSERT( rSearchAttr.m_iTag == FT_FILENAME );
			ASSERT( rSearchAttr.m_uIntegerOperator == ED2K_SEARCH_OP_EQUAL );
			target.WriteMetaDataSearchParam(OptUtf8ToStr(rstrA));
		}
	}

	if (thePrefs.GetDebugServerSearchesLevel() > 0)
		Debug(_T("Search Data: %s\n"), target.GetDebugString());
	s_SearchExpr.m_aExpr.RemoveAll();
	s_SearchExpr2.m_aExpr.RemoveAll();
	return true;
}

bool CSearchResultsWnd::StartNewSearch(SSearchParams* pParams)
{
	
	if (pParams->eType == SearchTypeAutomatic){
		// select between kad and server
		// its easy if we are connected to one network only anyway
		if (!theApp.serverconnect->IsConnected() && Kademlia::CKademlia::IsRunning() && Kademlia::CKademlia::IsConnected())
			pParams->eType = SearchTypeKademlia;
		else if (theApp.serverconnect->IsConnected() && (!Kademlia::CKademlia::IsRunning() || !Kademlia::CKademlia::IsConnected()))
			pParams->eType = SearchTypeEd2kServer;
		else if (!theApp.serverconnect->IsConnected() && (!Kademlia::CKademlia::IsRunning() || !Kademlia::CKademlia::IsConnected())){
			AfxMessageBox(GetResString(IDS_NOTCONNECTEDANY), MB_ICONWARNING);
			delete pParams;
			return false;
		}
		else {
			// connected to both
			// We choose Kad, except 
			// - if we are connected to a static server 
			// - or a server with more than 40k and less than 2mio users connected, more than 5 mio files and if our serverlist contains less than
			// 40 servers (otherwise we have assume that its polluted with fake server and we might just as well be connected to one) 
			// might be further optmized in the future
			if (theApp.serverconnect->IsConnected() && theApp.serverconnect->GetCurrentServer() != NULL 
				&& (theApp.serverconnect->GetCurrentServer()->IsStaticMember()
				|| (theApp.serverconnect->GetCurrentServer()->GetUsers() > 40000 && theApp.serverlist->GetServerCount() < 40
					&& theApp.serverconnect->GetCurrentServer()->GetUsers() < 5000000 
					&& theApp.serverconnect->GetCurrentServer()->GetFiles() > 5000000)))
			{
				pParams->eType = SearchTypeEd2kServer;
			}
			else
				pParams->eType = SearchTypeKademlia;
		}
	}

	ESearchType eSearchType = pParams->eType;
	if (eSearchType == SearchTypeEd2kServer || eSearchType == SearchTypeEd2kGlobal)
	{
		if (!theApp.serverconnect->IsConnected()) {
			AfxMessageBox(GetResString(IDS_ERR_NOTCONNECTED), MB_ICONWARNING);
			delete pParams;
			//if (!theApp.serverconnect->IsConnecting() && !theApp.serverconnect->IsConnected())
			//	theApp.serverconnect->ConnectToAnyServer();
			return false;
		}

		try
		{
			if (!DoNewEd2kSearch(pParams)) {
				delete pParams;
				return false;
			}
		}
		catch (CMsgBoxException* ex)
		{
			AfxMessageBox(ex->m_strMsg, ex->m_uType, ex->m_uHelpID);
			ex->Delete();
			delete pParams;
			return false;
		}

		SearchStarted();
		return true;
	}

	if (eSearchType == SearchTypeKademlia)
	{
		if (!Kademlia::CKademlia::IsRunning() || !Kademlia::CKademlia::IsConnected()) {
			AfxMessageBox(GetResString(IDS_ERR_NOTCONNECTEDKAD), MB_ICONWARNING);
			delete pParams;
			//if (!Kademlia::CKademlia::IsRunning())
			//	Kademlia::CKademlia::Start();
			return false;
		}
		
		try
		{
			if (!DoNewKadSearch(pParams)) {
				delete pParams;
				return false;
			}
		}
		catch (CMsgBoxException* ex)
		{
			AfxMessageBox(ex->m_strMsg, ex->m_uType, ex->m_uHelpID);
			ex->Delete();
			delete pParams;
			return false;
		}

		SearchStarted();
		return true;
	}

	ASSERT(0);
	delete pParams;
	return false;
}

bool CSearchResultsWnd::DoNewEd2kSearch(SSearchParams* pParams)
{
	if (!theApp.serverconnect->IsConnected())
		return false;

	bool bServerSupports64Bit = theApp.serverconnect->GetCurrentServer() != NULL
								&& (theApp.serverconnect->GetCurrentServer()->GetTCPFlags() & SRV_TCPFLG_LARGEFILES);
	bool bPacketUsing64Bit = false;
	CSafeMemFile data(100);
	if (!GetSearchPacket(&data, pParams, bServerSupports64Bit, &bPacketUsing64Bit) || data.GetLength() == 0)
		return false;

	CancelEd2kSearch();

	CStringA strResultType = pParams->strFileType;
	if (strResultType == ED2KFTSTR_PROGRAM)
		strResultType.Empty();
	m_nEd2kSearchID++;
	pParams->dwSearchID = m_nEd2kSearchID;
	theApp.searchlist->NewSearch(&searchlistctrl, strResultType, m_nEd2kSearchID, pParams->eType, pParams->strExpression);
	canceld = false;

	if (m_uTimerLocalServer){
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;
	}

	// once we've sent a new search request, any previously received 'More' gets invalid.
	CWnd* pWndFocus = GetFocus();
	m_pwndParams->m_ctlMore.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_pwndParams->m_ctlMore.m_hWnd)
		m_pwndParams->m_ctlCancel.SetFocus();
	m_iSentMoreReq = 0;

	Packet* packet = new Packet(&data);
	packet->opcode = OP_SEARCHREQUEST;
	if (thePrefs.GetDebugServerTCPLevel() > 0)
		Debug(_T(">>> Sending OP__SearchRequest\n"));
	theStats.AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet,false);

	if (pParams->eType == SearchTypeEd2kGlobal && theApp.serverconnect->IsUDPSocketAvailable())
	{
		// set timeout timer for local server
		m_uTimerLocalServer = SetTimer(TimerServerTimeout, 50000, NULL);

		if (thePrefs.GetUseServerPriorities())
			theApp.serverlist->ResetSearchServerPos();

		if (globsearch){
			delete searchpacket;
			searchpacket = NULL;
			m_b64BitSearchPacket = false;
		}
		searchpacket = packet;
		searchpacket->opcode = OP_GLOBSEARCHREQ; // will be changed later when actually sending the packet!!
		m_b64BitSearchPacket = bPacketUsing64Bit;
		servercount = 0;
		searchprogress.SetRange32(0, theApp.serverlist->GetServerCount() - 1);
		globsearch = true;
	}
	else{
		globsearch = false;
		delete packet;
	}
	CreateNewTab(pParams);
	return true;
}
	
bool CSearchResultsWnd::SearchMore()
{
	if (!theApp.serverconnect->IsConnected())
		return false;

	SetActiveSearchResultsIcon(m_nEd2kSearchID);
	canceld = false;

	Packet* packet = new Packet();
	packet->opcode = OP_QUERY_MORE_RESULT;
	if (thePrefs.GetDebugServerTCPLevel() > 0)
		Debug(_T(">>> Sending OP__QueryMoreResults\n"));
	theStats.AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet);
	m_iSentMoreReq++;
	return true;
}

bool CSearchResultsWnd::DoNewKadSearch(SSearchParams* pParams)
{
	if (!Kademlia::CKademlia::IsConnected())
		return false;

	int iPos = 0;
	pParams->strKeyword = pParams->strExpression.Tokenize(_T(" "), iPos);
	if (!pParams->strKeyword.IsEmpty() && pParams->strKeyword.GetAt(0) == _T('\"'))
	{
		// remove leading and possibly trailing quotes, if they terminate properly (otherwise the keyword is later handled as invalid)
		// (quotes are still kept in search expr and matched against the result, so everything is fine)
		if (pParams->strKeyword.GetLength() > 1 && pParams->strKeyword.Right(1).Compare(_T("\"")) == 0)
			pParams->strKeyword = pParams->strKeyword.Mid(1, pParams->strKeyword.GetLength() - 2);
		else if (pParams->strExpression.Find(_T('\"'), 1) > pParams->strKeyword.GetLength())
			pParams->strKeyword = pParams->strKeyword.Mid(1, pParams->strKeyword.GetLength() - 1);
	}
	pParams->strKeyword.Trim();

	CSafeMemFile data(100);
	if (!GetSearchPacket(&data, pParams, true, NULL)/* || (!pParams->strBooleanExpr.IsEmpty() && data.GetLength() == 0)*/)
		return false;

	if (pParams->strKeyword.IsEmpty() || pParams->strKeyword.FindOneOf(g_aszInvKadKeywordChars) != -1){
		CString strError;
		strError.Format(GetResString(IDS_KAD_SEARCH_KEYWORD_INVALID), g_aszInvKadKeywordChars);
		throw new CMsgBoxException(strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	}

	LPBYTE pSearchTermsData = NULL;
	UINT uSearchTermsSize = (UINT)data.GetLength();
	if (uSearchTermsSize){
		pSearchTermsData = new BYTE[uSearchTermsSize];
		data.SeekToBegin();
		data.Read(pSearchTermsData, uSearchTermsSize);
	}

	Kademlia::CSearch* pSearch = NULL;
	try
	{
		pSearch = Kademlia::CSearchManager::PrepareFindKeywords(pParams->strKeyword, uSearchTermsSize, pSearchTermsData);
		delete[] pSearchTermsData;
		if (!pSearch){
			ASSERT(0);
			return false;
		}
	}
	catch (CString strException)
	{
		delete[] pSearchTermsData;
		throw new CMsgBoxException(strException, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	}
	pParams->dwSearchID = pSearch->GetSearchID();
	CStringA strResultType = pParams->strFileType;
	if (strResultType == ED2KFTSTR_PROGRAM)
		strResultType.Empty();
	theApp.searchlist->NewSearch(&searchlistctrl, strResultType, pParams->dwSearchID, pParams->eType, pParams->strExpression);
	CreateNewTab(pParams);
	return true;
}

bool CSearchResultsWnd::CreateNewTab(SSearchParams* pParams, bool bActiveIcon)
{
    int iTabItems = searchselect.GetItemCount();
    for (int i = 0; i < iTabItems; i++)
	{
        TCITEM tci;
        tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL && ((const SSearchParams*)tci.lParam)->dwSearchID == pParams->dwSearchID)
			return false;
    }

	// add new tab
	TCITEM newitem;
	if (pParams->strExpression.IsEmpty())
		pParams->strExpression = _T("-");
	newitem.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	newitem.lParam = (LPARAM)pParams;
	pParams->strSearchTitle = (pParams->strSpecialTitle.IsEmpty() ? pParams->strExpression : pParams->strSpecialTitle);
	CString strTcLabel(pParams->strSearchTitle);
	strTcLabel.Replace(_T("&"), _T("&&"));
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)strTcLabel);
	newitem.cchTextMax = 0;
	if (pParams->bClientSharedFiles)
		newitem.iImage = sriClient;
	else if (pParams->eType == SearchTypeKademlia)
		newitem.iImage = bActiveIcon ? sriKadActice : sriKad;
	else if (pParams->eType == SearchTypeEd2kGlobal)
		newitem.iImage = bActiveIcon ? sriGlobalActive : sriGlobal;
	else{
		ASSERT( pParams->eType == SearchTypeEd2kServer );
		newitem.iImage = bActiveIcon ? sriServerActive : sriServer;
	}
	int itemnr = searchselect.InsertItem(INT_MAX, &newitem);
	if (!searchselect.IsWindowVisible())
		ShowSearchSelector(true);
	searchselect.SetCurSel(itemnr);
	searchlistctrl.ShowResults(pParams->dwSearchID);
	return true;
}

void CSearchResultsWnd::DeleteSelectedSearch()
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItemCount() > 0 && searchselect.GetCurFocus() != (-1) && searchselect.GetItem(searchselect.GetCurFocus(), &item)
		&& item.lParam != NULL)
	{
		DeleteSearch(((const SSearchParams*)item.lParam)->dwSearchID);
	}
}

bool CSearchResultsWnd::CanDeleteSearch(uint32 /*nSearchID*/) const
{
	return (searchselect.GetItemCount() > 0);
}

void CSearchResultsWnd::DeleteSearch(uint32 nSearchID)
{
	Kademlia::CSearchManager::StopSearch(nSearchID, false);

	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int i;
	for (i = 0; i < searchselect.GetItemCount(); i++) {
		if (searchselect.GetItem(i, &item) && item.lParam != -1 && item.lParam != NULL && ((const SSearchParams*)item.lParam)->dwSearchID == nSearchID)
			break;
	}
	if (item.lParam == -1 || item.lParam == NULL || ((const SSearchParams*)item.lParam)->dwSearchID != nSearchID)
		return;

	// delete search results
	if (!canceld && nSearchID == m_nEd2kSearchID)
		CancelEd2kSearch();
	if (nSearchID == m_nEd2kSearchID)
		m_pwndParams->m_ctlMore.EnableWindow(FALSE);
	theApp.searchlist->RemoveResults(nSearchID);
	
	// clean up stored states (scrollingpos etc) for this search
	searchlistctrl.ClearResultViewState(nSearchID);
	
	// delete search tab
	int iCurSel = searchselect.GetCurSel();
	searchselect.DeleteItem(i);
	delete (SSearchParams*)item.lParam;

	int iTabItems = searchselect.GetItemCount();
	if (iTabItems > 0){
		// select next search tab
		if (iCurSel == CB_ERR)
			iCurSel = 0;
		else if (iCurSel >= iTabItems)
			iCurSel = iTabItems - 1;
		(void)searchselect.SetCurSel(iCurSel);	// returns CB_ERR if error or no prev. selection(!)
		iCurSel = searchselect.GetCurSel();		// get the real current selection
		if (iCurSel == CB_ERR)					// if still error
			iCurSel = searchselect.SetCurSel(0);
		if (iCurSel != CB_ERR){
			item.mask = TCIF_PARAM;
			item.lParam = NULL;
			if (searchselect.GetItem(iCurSel, &item) && item.lParam != NULL){
				searchselect.HighlightItem(iCurSel, FALSE);
				ShowResults((const SSearchParams*)item.lParam);
			}
		}
	}
	else{
		theApp.searchlist->Clear();
		searchlistctrl.DeleteAllItems();
		ShowSearchSelector(false);
		searchlistctrl.NoTabs();

		CWnd* pWndFocus = GetFocus();
		m_pwndParams->m_ctlMore.EnableWindow(FALSE);
		m_pwndParams->m_ctlCancel.EnableWindow(FALSE);
		m_pwndParams->m_ctlStart.EnableWindow(m_pwndParams->m_ctlName.GetWindowTextLength() > 0);
		if (pWndFocus) {
			if (pWndFocus->m_hWnd == m_pwndParams->m_ctlMore.m_hWnd || pWndFocus->m_hWnd == m_pwndParams->m_ctlCancel.m_hWnd) {
				if (m_pwndParams->m_ctlStart.IsWindowEnabled())
					m_pwndParams->m_ctlStart.SetFocus();
				else
					m_pwndParams->m_ctlName.SetFocus();
			}
			else if (pWndFocus->m_hWnd == m_pwndParams->m_ctlStart.m_hWnd && !m_pwndParams->m_ctlStart.IsWindowEnabled())
				m_pwndParams->m_ctlName.SetFocus();
		}
	}
}

bool CSearchResultsWnd::CanDeleteAllSearches() const
{
	return (searchselect.GetItemCount() > 0);
}

void CSearchResultsWnd::DeleteAllSearches()
{
	CancelEd2kSearch();

	for (int i = 0; i < searchselect.GetItemCount(); i++){
		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		if (searchselect.GetItem(i, &item) && item.lParam != -1 && item.lParam != NULL){
			Kademlia::CSearchManager::StopSearch(((const SSearchParams*)item.lParam)->dwSearchID, false);
			delete (SSearchParams*)item.lParam;
		}
	}
	theApp.searchlist->Clear();
	searchlistctrl.DeleteAllItems();
	ShowSearchSelector(false);
	searchselect.DeleteAllItems();
	searchlistctrl.NoTabs();

	CWnd* pWndFocus = GetFocus();
	m_pwndParams->m_ctlMore.EnableWindow(FALSE);
	m_pwndParams->m_ctlCancel.EnableWindow(FALSE);
	m_pwndParams->m_ctlStart.EnableWindow(m_pwndParams->m_ctlName.GetWindowTextLength() > 0);
	if (pWndFocus) {
		if (pWndFocus->m_hWnd == m_pwndParams->m_ctlMore.m_hWnd || pWndFocus->m_hWnd == m_pwndParams->m_ctlCancel.m_hWnd) {
			if (m_pwndParams->m_ctlStart.IsWindowEnabled())
				m_pwndParams->m_ctlStart.SetFocus();
			else
				m_pwndParams->m_ctlName.SetFocus();
		}
		else if (pWndFocus->m_hWnd == m_pwndParams->m_ctlStart.m_hWnd && !m_pwndParams->m_ctlStart.IsWindowEnabled())
			m_pwndParams->m_ctlName.SetFocus();
	}
}

void CSearchResultsWnd::ShowResults(const SSearchParams* pParams)
{
	// restoring the params works and is nice during development/testing but pretty annoying in practice.
	// TODO: maybe it should be done explicitly via a context menu function or such.
	if (GetAsyncKeyState(VK_CONTROL) < 0)
		m_pwndParams->SetParameters(pParams);

	if (pParams->eType == SearchTypeEd2kServer)
	{
		m_pwndParams->m_ctlCancel.EnableWindow(pParams->dwSearchID == m_nEd2kSearchID && IsLocalEd2kSearchRunning());
	}
	else if (pParams->eType == SearchTypeEd2kGlobal)
	{
		m_pwndParams->m_ctlCancel.EnableWindow(pParams->dwSearchID == m_nEd2kSearchID && (IsLocalEd2kSearchRunning() || IsGlobalEd2kSearchRunning()));
	}
	else if (pParams->eType == SearchTypeKademlia)
	{
		m_pwndParams->m_ctlCancel.EnableWindow(Kademlia::CSearchManager::IsSearching(pParams->dwSearchID));
	}
	searchlistctrl.ShowResults(pParams->dwSearchID);
}

void CSearchResultsWnd::OnSelChangeTab(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	CWaitCursor curWait; // this may take a while
	int cur_sel = searchselect.GetCurSel();
	if (cur_sel == -1)
		return;
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem(cur_sel, &item) && item.lParam != NULL)
	{
		searchselect.HighlightItem(cur_sel, FALSE);
		ShowResults((const SSearchParams*)item.lParam);
	}
	*pResult = 0;
}

LRESULT CSearchResultsWnd::OnCloseTab(WPARAM wParam, LPARAM /*lParam*/)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem((int)wParam, &item) && item.lParam != NULL)
	{
		int nSearchID = ((const SSearchParams*)item.lParam)->dwSearchID;
		if (!canceld && (UINT)nSearchID == m_nEd2kSearchID)
			CancelEd2kSearch();
		DeleteSearch(nSearchID);
	}
	return TRUE;
}

LRESULT CSearchResultsWnd::OnDblClickTab(WPARAM wParam, LPARAM /*lParam*/)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem((int)wParam, &item) && item.lParam != NULL)
	{
		m_pwndParams->SetParameters((const SSearchParams*)item.lParam);
	}
	return TRUE;
}

int CSearchResultsWnd::GetSelectedCat()
{
	return m_cattabs->GetCurSel();
}

void CSearchResultsWnd::UpdateCatTabs()
{
	int oldsel=m_cattabs->GetCurSel();
	m_cattabs->DeleteAllItems();
	for (int ix=0;ix<thePrefs.GetCatCount();ix++) {
		CString label=(ix==0)?GetResString(IDS_ALL):thePrefs.GetCategory(ix)->strTitle;
		label.Replace(_T("&"),_T("&&"));
		m_cattabs->InsertItem(ix,label);
	}
	if (oldsel>=m_cattabs->GetItemCount() || oldsel==-1)
		oldsel=0;

	m_cattabs->SetCurSel(oldsel);
	int flag;
	flag=(m_cattabs->GetItemCount()>1) ? SW_SHOW:SW_HIDE;
	m_cattabs->ShowWindow(flag);
	GetDlgItem(IDC_STATIC_DLTOof)->ShowWindow(flag);
}

void CSearchResultsWnd::ShowSearchSelector(bool visible)
{
	WINDOWPLACEMENT wpSearchWinPos;
	WINDOWPLACEMENT wpSelectWinPos;
	searchselect.GetWindowPlacement(&wpSelectWinPos);
	searchlistctrl.GetWindowPlacement(&wpSearchWinPos);
	if (visible)
		wpSearchWinPos.rcNormalPosition.top = wpSelectWinPos.rcNormalPosition.bottom;
	else
		wpSearchWinPos.rcNormalPosition.top = wpSelectWinPos.rcNormalPosition.top;
	searchselect.ShowWindow(visible ? SW_SHOW : SW_HIDE);
	RemoveAnchor(searchlistctrl);
	searchlistctrl.SetWindowPlacement(&wpSearchWinPos);
	AddAnchor(searchlistctrl, TOP_LEFT, BOTTOM_RIGHT);
	GetDlgItem(IDC_CLEARALL)->ShowWindow(visible ? SW_SHOW : SW_HIDE);
	m_ctlFilter.ShowWindow(visible ? SW_SHOW : SW_HIDE);
}

void CSearchResultsWnd::OnDestroy()
{
	int iTabItems = searchselect.GetItemCount();
	for (int i = 0; i < iTabItems; i++){
		TCITEM tci;
		tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL){
			delete (SSearchParams*)tci.lParam;
		}
	}

	CResizableFormView::OnDestroy();
}

void CSearchResultsWnd::OnClose()
{
	// Do not pass the WM_CLOSE to the base class. Since we have a rich edit control *and* an attached auto complete
	// control, the WM_CLOSE will get generated by the rich edit control when user presses ESC while the auto complete
	// is open.
	//__super::OnClose();
}

BOOL CSearchResultsWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	theApp.ShowHelp(eMule_FAQ_GUI_Search);
	return TRUE;
}

LRESULT CSearchResultsWnd::OnIdleUpdateCmdUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	BOOL bSearchParamsWndVisible = theApp.emuledlg->searchwnd->IsSearchParamsWndVisible();
	if (!bSearchParamsWndVisible) {
		m_ctlOpenParamsWnd.ShowWindow(SW_SHOW);
	}
	else {
		m_ctlOpenParamsWnd.ShowWindow(SW_HIDE);
	}
	return 0;
}

void CSearchResultsWnd::OnBnClickedOpenParamsWnd()
{
	theApp.emuledlg->searchwnd->OpenParametersWnd();
}

void CSearchResultsWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID == SC_KEYMENU)
	{
		if (lParam == EMULE_HOTMENU_ACCEL)
			theApp.emuledlg->SendMessage(WM_COMMAND, IDC_HOTMENU);
		else
			theApp.emuledlg->SendMessage(WM_SYSCOMMAND, nID, lParam);
		return;
	}
	__super::OnSysCommand(nID, lParam);
}

bool CSearchResultsWnd::CanSearchRelatedFiles() const
{
	return theApp.serverconnect->IsConnected() 
		&& theApp.serverconnect->GetCurrentServer() != NULL 
		&& theApp.serverconnect->GetCurrentServer()->GetRelatedSearchSupport();
}

void CSearchResultsWnd::SearchRelatedFiles(CPtrList& listFiles)
{
	SSearchParams* pParams = new SSearchParams;
	pParams->strExpression = _T("related");
	POSITION pos = listFiles.GetHeadPosition();
	if (pos == NULL){
		delete pParams;
		ASSERT( false );
		return;
	}

	CString strNames;
	while (pos != NULL){
		CAbstractFile* pFile = (CAbstractFile*)listFiles.GetNext(pos);
		if (pFile->IsKindOf(RUNTIME_CLASS(CAbstractFile))){
			pParams->strExpression += _T("::") + md4str(pFile->GetFileHash());
			if (!strNames.IsEmpty())
				strNames += _T(", ");
			strNames += pFile->GetFileName();
		}
		else
			ASSERT( false );
	}

	pParams->strSpecialTitle = GetResString(IDS_RELATED) + _T(": ") + strNames;
	if (pParams->strSpecialTitle.GetLength() > 50)
		pParams->strSpecialTitle = pParams->strSpecialTitle.Left(50) + _T("...");
	StartSearch(pParams);
}


///////////////////////////////////////////////////////////////////////////////
// CSearchResultsSelector

BEGIN_MESSAGE_MAP(CSearchResultsSelector, CClosableTabCtrl)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

BOOL CSearchResultsSelector::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case MP_RESTORESEARCHPARAMS:{
		int iTab = GetTabUnderContextMenu();
		if (iTab != -1) {
			GetParent()->SendMessage(UM_DBLCLICKTAB, (WPARAM)iTab);
			return TRUE;
		}
		break;
	  }
	}
	return CClosableTabCtrl::OnCommand(wParam, lParam);
}

void CSearchResultsSelector::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if (point.x == -1 || point.y == -1) {
		if (!SetDefaultContextMenuPos())
			return;
		point = m_ptCtxMenu;
		ClientToScreen(&point);
	}
	else {
		m_ptCtxMenu = point;
		ScreenToClient(&m_ptCtxMenu);
	}

	CTitleMenu menu;
	menu.CreatePopupMenu();
	menu.AddMenuTitle(GetResString(IDS_SW_RESULT));
	menu.AppendMenu(MF_STRING, MP_RESTORESEARCHPARAMS, GetResString(IDS_RESTORESEARCHPARAMS));
	menu.AppendMenu(MF_STRING, MP_REMOVE, GetResString(IDS_FD_CLOSE));
	menu.SetDefaultItem(MP_RESTORESEARCHPARAMS);
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

LRESULT CSearchResultsWnd::OnChangeFilter(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor curWait; // this may take a while

	bool bColumnDiff = (m_nFilterColumn != (uint32)wParam);
	m_nFilterColumn = (uint32)wParam;

	CStringArray astrFilter;
	CString strFullFilterExpr = (LPCTSTR)lParam;
	int iPos = 0;
	CString strFilter(strFullFilterExpr.Tokenize(_T(" "), iPos));
	while (!strFilter.IsEmpty()) {
		if (strFilter != _T("-"))
			astrFilter.Add(strFilter);
		strFilter = strFullFilterExpr.Tokenize(_T(" "), iPos);
	}

	bool bFilterDiff = (astrFilter.GetSize() != m_astrFilter.GetSize());
	if (!bFilterDiff) {
		for (int i = 0; i < astrFilter.GetSize(); i++) {
			if (astrFilter[i] != m_astrFilter[i]) {
				bFilterDiff = true;
				break;
			}
		}
	}

	if (!bColumnDiff && !bFilterDiff)
		return 0;
	m_astrFilter.RemoveAll();
	m_astrFilter.Append(astrFilter);

	int iCurSel = searchselect.GetCurSel();
	if (iCurSel == -1)
		return 0;
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem(iCurSel, &item) && item.lParam != NULL)
		ShowResults((const SSearchParams*)item.lParam);
	return 0;
}

void CSearchResultsWnd::OnSearchListMenuBtnDropDown(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CTitleMenu menu;
	menu.CreatePopupMenu();

	menu.AppendMenu(MF_STRING | (searchselect.GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED), MP_REMOVEALL, GetResString(IDS_REMOVEALLSEARCH));
	menu.AppendMenu(MF_SEPARATOR);
	CMenu menuFileSizeFormat;
	menuFileSizeFormat.CreateMenu();
	menuFileSizeFormat.AppendMenu(MF_STRING, MP_SHOW_FILESIZE_DFLT, GetResString(IDS_DEFAULT));
	menuFileSizeFormat.AppendMenu(MF_STRING, MP_SHOW_FILESIZE_KBYTE, GetResString(IDS_KBYTES));
	menuFileSizeFormat.AppendMenu(MF_STRING, MP_SHOW_FILESIZE_MBYTE, GetResString(IDS_MBYTES));
	menuFileSizeFormat.CheckMenuRadioItem(MP_SHOW_FILESIZE_DFLT, MP_SHOW_FILESIZE_MBYTE, MP_SHOW_FILESIZE_DFLT + searchlistctrl.GetFileSizeFormat(), 0);
	menu.AppendMenu(MF_POPUP, (UINT_PTR)menuFileSizeFormat.m_hMenu, GetResString(IDS_DL_SIZE));

	CRect rc;
	m_btnSearchListMenu->GetWindowRect(&rc);
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, rc.left, rc.bottom, this);
}

BOOL CSearchResultsWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case MP_REMOVEALL:
		DeleteAllSearches();
		return TRUE;
	case MP_SHOW_FILESIZE_DFLT:
		searchlistctrl.SetFileSizeFormat(fsizeDefault);
		return TRUE;
	case MP_SHOW_FILESIZE_KBYTE:
		searchlistctrl.SetFileSizeFormat(fsizeKByte);
		return TRUE;
	case MP_SHOW_FILESIZE_MBYTE:
		searchlistctrl.SetFileSizeFormat(fsizeMByte);
		return TRUE;
	}
	return CResizableFormView::OnCommand(wParam, lParam);
}

HBRUSH CSearchResultsWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = theApp.emuledlg->GetCtlColor(pDC, pWnd, nCtlColor);
	if (hbr)
		return hbr;
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}
