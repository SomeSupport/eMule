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
#include <afxinet.h>
#define MMNODRV			// mmsystem: Installable driver support
//#define MMNOSOUND		// mmsystem: Sound support
#define MMNOWAVE		// mmsystem: Waveform support
#define MMNOMIDI		// mmsystem: MIDI support
#define MMNOAUX			// mmsystem: Auxiliary audio support
#define MMNOMIXER		// mmsystem: Mixer support
#define MMNOTIMER		// mmsystem: Timer support
#define MMNOJOY			// mmsystem: Joystick support
#define MMNOMCI			// mmsystem: MCI support
#define MMNOMMIO		// mmsystem: Multimedia file I/O support
#define MMNOMMSYSTEM	// mmsystem: General MMSYSTEM functions
#include <Mmsystem.h>
#include <HtmlHelp.h>
#include <share.h>
#include "emule.h"
#include "emuleDlg.h"
#include "ServerWnd.h"
#include "KademliaWnd.h"
#include "TransferWnd.h"
#include "TransferDlg.h"
#include "SearchResultsWnd.h"
#include "SearchDlg.h"
#include "SharedFilesWnd.h"
#include "ChatWnd.h"
#include "IrcWnd.h"
#include "StatisticsDlg.h"
#include "CreditsDlg.h"
#include "PreferencesDlg.h"
#include "Sockets.h"
#include "KnownFileList.h"
#include "ServerList.h"
#include "Opcodes.h"
#include "SharedFileList.h"
#include "ED2KLink.h"
#include "Splashscreen.h"
#include "PartFileConvert.h"
#include "EnBitmap.h"
#include "Exceptions.h"
#include "SearchList.h"
#include "HTRichEditCtrl.h"
#include "FrameGrabThread.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/routing/RoutingZone.h"
#include "kademlia/routing/contact.h"
#include "kademlia/kademlia/prefs.h"
#include "KadSearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "PerfLog.h"
#include "DropTarget.h"
#include "LastCommonRouteFinder.h"
#include "WebServer.h"
#include "MMServer.h"
#include "DownloadQueue.h"
#include "ClientUDPSocket.h"
#include "UploadQueue.h"
#include "ClientList.h"
#include "UploadBandwidthThrottler.h"
#include "FriendList.h"
#include "IPFilter.h"
#include "Statistics.h"
#include "MuleToolbarCtrl.h"
#include "TaskbarNotifier.h"
#include "MuleStatusbarCtrl.h"
#include "ListenSocket.h"
#include "Server.h"
#include "PartFile.h"
#include "Scheduler.h"
#include "ClientCredits.h"
#include "MenuCmds.h"
#include "MuleSystrayDlg.h"
#include "IPFilterDlg.h"
#include "WebServices.h"
#include "DirectDownloadDlg.h"
#include "PeerCacheFinder.h"
#include "Statistics.h"
#include "FirewallOpener.h"
#include "StringConversion.h"
#include "aichsyncthread.h"
#include "Log.h"
#include "MiniMule.h"
#include "UserMsgs.h"
#include "TextToSpeech.h"
#include "Collection.h"
#include "CollectionViewDialog.h"
#include "VisualStylesXP.h"
#include "UPnPImpl.h"
#include "UPnPImplWrapper.h"
#include <dbt.h>
#include "XMessageBox.h"
#include "UploadDiskIOThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL FirstTimeWizard();

#define	SYS_TRAY_ICON_COOKIE_FORCE_UPDATE	(UINT)-1

UINT g_uMainThreadId = 0;
const static UINT UWM_ARE_YOU_EMULE = RegisterWindowMessage(EMULE_GUID);

#ifdef HAVE_WIN7_SDK_H
const static UINT UWM_TASK_BUTTON_CREATED = RegisterWindowMessage(_T("TaskbarButtonCreated"));
#endif



///////////////////////////////////////////////////////////////////////////
// CemuleDlg Dialog

IMPLEMENT_DYNAMIC(CMsgBoxException, CException)

BEGIN_MESSAGE_MAP(CemuleDlg, CTrayDialog)
	///////////////////////////////////////////////////////////////////////////
	// Windows messages
	//
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_ENDSESSION()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_MENUCHAR()
	ON_WM_QUERYENDSESSION()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_COPYDATA, OnWMData)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_MESSAGE(WM_USERCHANGED, OnUserChanged)
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_SETTINGCHANGE()
	ON_WM_DEVICECHANGE()
	ON_MESSAGE(WM_POWERBROADCAST, OnPowerBroadcast)

	///////////////////////////////////////////////////////////////////////////
	// WM_COMMAND messages
	//
	ON_COMMAND(MP_CONNECT, StartConnection)
	ON_COMMAND(MP_DISCONNECT, CloseConnection)
	ON_COMMAND(MP_EXIT, OnClose)
	ON_COMMAND(MP_RESTORE, RestoreWindow)
	// quick-speed changer -- 
	ON_COMMAND_RANGE(MP_QS_U10, MP_QS_UP10, QuickSpeedUpload)
	ON_COMMAND_RANGE(MP_QS_D10, MP_QS_DC, QuickSpeedDownload)
	//--- quickspeed - paralize all ---
	ON_COMMAND_RANGE(MP_QS_PA, MP_QS_UA, QuickSpeedOther)
	// quick-speed changer -- based on xrmb	
	ON_NOTIFY_EX_RANGE(RBN_CHEVRONPUSHED, 0, 0xFFFF, OnChevronPushed)

	ON_REGISTERED_MESSAGE(UWM_ARE_YOU_EMULE, OnAreYouEmule)
	ON_BN_CLICKED(IDC_HOTMENU, OnBnClickedHotmenu)

	///////////////////////////////////////////////////////////////////////////
	// WM_USER messages
	//
	ON_MESSAGE(UM_TASKBARNOTIFIERCLICKED, OnTaskbarNotifierClicked)
	ON_MESSAGE(UM_CLOSE_MINIMULE, OnCloseMiniMule)
	
	// Webserver messages
	ON_MESSAGE(WEB_GUI_INTERACTION, OnWebGUIInteraction)
	ON_MESSAGE(WEB_CLEAR_COMPLETED, OnWebServerClearCompleted)
	ON_MESSAGE(WEB_FILE_RENAME, OnWebServerFileRename)
	ON_MESSAGE(WEB_ADDDOWNLOADS, OnWebAddDownloads)
	ON_MESSAGE(WEB_CATPRIO, OnWebSetCatPrio)
	ON_MESSAGE(WEB_ADDREMOVEFRIEND, OnAddRemoveFriend)

	// Version Check DNS
	ON_MESSAGE(UM_VERSIONCHECK_RESPONSE, OnVersionCheckResponse)

	// PeerCache DNS
	ON_MESSAGE(UM_PEERCHACHE_RESPONSE, OnPeerCacheResponse)

	// UPnP
	ON_MESSAGE(UM_UPNP_RESULT, OnUPnPResult)

	///////////////////////////////////////////////////////////////////////////
	// WM_APP messages
	//
	ON_MESSAGE(TM_FINISHEDHASHING, OnFileHashed)
	ON_MESSAGE(TM_FILEOPPROGRESS, OnFileOpProgress)
	ON_MESSAGE(TM_HASHFAILED, OnHashFailed)
	ON_MESSAGE(TM_FRAMEGRABFINISHED, OnFrameGrabFinished)
	ON_MESSAGE(TM_FILEALLOCEXC, OnFileAllocExc)
	ON_MESSAGE(TM_FILECOMPLETED, OnFileCompleted)
	ON_MESSAGE(TM_CONSOLETHREADEVENT, OnConsoleThreadEvent)

#ifdef HAVE_WIN7_SDK_H
	ON_REGISTERED_MESSAGE(UWM_TASK_BUTTON_CREATED, OnTaskbarBtnCreated)
#endif

END_MESSAGE_MAP()

CemuleDlg::CemuleDlg(CWnd* pParent /*=NULL*/)
	: CTrayDialog(CemuleDlg::IDD, pParent)
{
	g_uMainThreadId = GetCurrentThreadId();
	preferenceswnd = new CPreferencesDlg;
	serverwnd = new CServerWnd;
	kademliawnd = new CKademliaWnd;
	transferwnd = new CTransferDlg;
	sharedfileswnd = new CSharedFilesWnd;
	searchwnd = new CSearchDlg;
	chatwnd = new CChatWnd;
	ircwnd = new CIrcWnd;
	statisticswnd = new CStatisticsDlg;
	toolbar = new CMuleToolbarCtrl;
	statusbar = new CMuleStatusBarCtrl;
	m_wndTaskbarNotifier = new CTaskbarNotifier;

	m_hIcon = NULL;
	theApp.m_app_state = APP_STATE_RUNNING;
	ready = false; 
	m_bStartMinimizedChecked = false;
	m_bStartMinimized = false;
	memset(&m_wpFirstRestore, 0, sizeof m_wpFirstRestore);
	m_uUpDatarate = 0;
	m_uDownDatarate = 0;
	status = 0;
	activewnd = NULL;
	for (int i = 0; i < _countof(connicons); i++)
		connicons[i] = NULL;
	transicons[0] = NULL;
	transicons[1] = NULL;
	transicons[2] = NULL;
	transicons[3] = NULL;
	imicons[0] = NULL;
	imicons[1] = NULL;
	imicons[2] = NULL;
	m_iMsgIcon = 0;
	m_iMsgBlinkState = false;
	m_icoSysTrayConnected = NULL;
	m_icoSysTrayDisconnected = NULL;
	m_icoSysTrayLowID = NULL;
	usericon = NULL;
	m_icoSysTrayCurrent = NULL;
	m_hTimer = 0;
	notifierenabled = false;
	m_pDropTarget = new CMainFrameDropTarget;
	m_pSplashWnd = NULL;
	m_dwSplashTime = (DWORD)-1;
	m_pSystrayDlg = NULL;
	m_pMiniMule = NULL;
	m_uLastSysTrayIconCookie = SYS_TRAY_ICON_COOKIE_FORCE_UPDATE;
	m_hUPnPTimeOutTimer = 0;
	m_bConnectRequestDelayedForUPnP = false;
	m_bEd2kSuspendDisconnect = false;
	m_bKadSuspendDisconnect = false;
	m_bInitedCOM = false;
}

CemuleDlg::~CemuleDlg()
{
	CloseTTS();
	DestroyMiniMule();
	if (m_icoSysTrayCurrent) VERIFY( DestroyIcon(m_icoSysTrayCurrent) );
	if (m_hIcon) VERIFY( ::DestroyIcon(m_hIcon) );
	for (int i = 0; i < _countof(connicons); i++){
		if (connicons[i]) VERIFY( ::DestroyIcon(connicons[i]) );
	}
	if (transicons[0]) VERIFY( ::DestroyIcon(transicons[0]) );
	if (transicons[1]) VERIFY( ::DestroyIcon(transicons[1]) );
	if (transicons[2]) VERIFY( ::DestroyIcon(transicons[2]) );
	if (transicons[3]) VERIFY( ::DestroyIcon(transicons[3]) );
	if (imicons[0]) VERIFY( ::DestroyIcon(imicons[0]) );
	if (imicons[1]) VERIFY( ::DestroyIcon(imicons[1]) );
	if (imicons[2]) VERIFY( ::DestroyIcon(imicons[2]) );
	if (m_icoSysTrayConnected) VERIFY( ::DestroyIcon(m_icoSysTrayConnected) );
	if (m_icoSysTrayDisconnected) VERIFY( ::DestroyIcon(m_icoSysTrayDisconnected) );
	if (m_icoSysTrayLowID) VERIFY( ::DestroyIcon(m_icoSysTrayLowID) );
	if (usericon) VERIFY( ::DestroyIcon(usericon) );

#ifdef HAVE_WIN7_SDK_H
	if (m_pTaskbarList != NULL)
	{
		m_pTaskbarList.Release();
		ASSERT( m_bInitedCOM );
	}
	if (m_bInitedCOM)
		CoUninitialize();
#endif

	// already destroyed by windows?
	//VERIFY( m_menuUploadCtrl.DestroyMenu() );
	//VERIFY( m_menuDownloadCtrl.DestroyMenu() );
	//VERIFY( m_SysMenuOptions.DestroyMenu() );

	delete preferenceswnd;
	delete serverwnd;
	delete kademliawnd;
	delete sharedfileswnd;
	delete chatwnd;
	delete ircwnd;
	delete statisticswnd;
	delete toolbar;
	delete statusbar;
	delete m_wndTaskbarNotifier;
	delete m_pDropTarget;
}

void CemuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CTrayDialog::DoDataExchange(pDX);
}

LRESULT CemuleDlg::OnAreYouEmule(WPARAM, LPARAM)
{
	return UWM_ARE_YOU_EMULE;
}

void DialogCreateIndirect(CDialog *pWnd, UINT uID)
{
#if 0
	// This could be a nice way to change the font size of the main windows without needing
	// to re-design the dialog resources. However, that technique does not work for the
	// SearchWnd and it also introduces new glitches (which would need to get resolved)
	// in almost all of the main windows.
	CDialogTemplate dlgTempl;
	dlgTempl.Load(MAKEINTRESOURCE(uID));
	dlgTempl.SetFont(_T("MS Shell Dlg"), 8);
	pWnd->CreateIndirect(dlgTempl.m_hTemplate);
	FreeResource(dlgTempl.Detach());
#else
	pWnd->Create(uID);
#endif
}


//#include <winuser.h>
BOOL CemuleDlg::OnInitDialog()
{
#ifdef HAVE_WIN7_SDK_H
	// allow the TaskbarButtonCreated- & (tbb-)WM_COMMAND message to be sent to our window if our app is running elevated
	if (thePrefs.GetWindowsVersion() >= _WINVER_7_)
	{
		int res = CoInitialize(NULL);
		if (res == S_OK || res == S_FALSE)
		{
			m_bInitedCOM = true;
			typedef BOOL (WINAPI* PChangeWindowMessageFilter)(UINT message, DWORD dwFlag);
			PChangeWindowMessageFilter ChangeWindowMessageFilter = 
			(PChangeWindowMessageFilter )(GetProcAddress(
				GetModuleHandle(TEXT("user32.dll")), "ChangeWindowMessageFilter"));
			if (ChangeWindowMessageFilter) {
				ChangeWindowMessageFilter(UWM_TASK_BUTTON_CREATED,1);
				ChangeWindowMessageFilter(WM_COMMAND, 1);
			}
		}
		else
			ASSERT( false );
	}
#endif

	m_bStartMinimized = thePrefs.GetStartMinimized();
	if (!m_bStartMinimized)
		m_bStartMinimized = theApp.DidWeAutoStart();

	// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
	if (thePrefs.IsFirstStart())
		m_bStartMinimized = false;

	// show splashscreen as early as possible to "entertain" user while starting emule up
	if (thePrefs.UseSplashScreen() && !m_bStartMinimized)
		ShowSplash();

	// Create global GUI objects
	theApp.CreateAllFonts();
	theApp.CreateBackwardDiagonalBrush();
	m_wndTaskbarNotifier->SetTextDefaultFont();
	CTrayDialog::OnInitDialog();
	InitWindowStyles(this);
	CreateToolbarCmdIconMap();

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL){
		pSysMenu->AppendMenu(MF_SEPARATOR);

		ASSERT( (MP_ABOUTBOX & 0xFFF0) == MP_ABOUTBOX && MP_ABOUTBOX < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_ABOUTBOX, GetResString(IDS_ABOUTBOX));

		ASSERT( (MP_VERSIONCHECK & 0xFFF0) == MP_VERSIONCHECK && MP_VERSIONCHECK < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK));

		// remaining system menu entries are created later...
	}

	CWnd* pwndToolbarX = toolbar;
	if (toolbar->Create(WS_CHILD | WS_VISIBLE, CRect(0,0,0,0), this, IDC_TOOLBAR))
	{
		toolbar->Init();
		if (thePrefs.GetUseReBarToolbar())
		{
		    if (m_ctlMainTopReBar.Create(WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
									     RBS_BANDBORDERS | RBS_AUTOSIZE | CCS_NODIVIDER, 
									     CRect(0, 0, 0, 0), this, AFX_IDW_REBAR))
		    {
			    CSize sizeBar;
			    VERIFY( toolbar->GetMaxSize(&sizeBar) );
			    REBARBANDINFO rbbi = {0};
			    rbbi.cbSize = sizeof(rbbi);
				rbbi.fMask = RBBIM_STYLE | RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_ID;
			    rbbi.fStyle = RBBS_NOGRIPPER | RBBS_BREAK | RBBS_USECHEVRON;
			    rbbi.hwndChild = toolbar->m_hWnd;
			    rbbi.cxMinChild = sizeBar.cy;
			    rbbi.cyMinChild = sizeBar.cy;
			    rbbi.cxIdeal = sizeBar.cx;
			    rbbi.cx = rbbi.cxIdeal;
				rbbi.wID = 0;
			    VERIFY( m_ctlMainTopReBar.InsertBand((UINT)-1, &rbbi) );
				toolbar->SaveCurHeight();
		    	toolbar->UpdateBackground();
    
			    pwndToolbarX = &m_ctlMainTopReBar;
		    }
		}
	}

	// set title
	SetWindowText(_T("eMule v") + theApp.m_strCurVersionLong);

	// Init taskbar notifier
	m_wndTaskbarNotifier->Create(this);
	LoadNotifier(thePrefs.GetNotifierConfiguration());

	// set statusbar
	// the statusbar control is created as a custom control in the dialog resource,
	// this solves font and sizing problems when using large system fonts
	statusbar->SubclassWindow(GetDlgItem(IDC_STATUSBAR)->m_hWnd);
	statusbar->EnableToolTips(true);
	SetStatusBarPartsSize();

	// create main window dialog pages
	DialogCreateIndirect(serverwnd, IDD_SERVER);
	DialogCreateIndirect(sharedfileswnd, IDD_FILES);
	searchwnd->Create(this); // can not use 'DialogCreateIndirect' for the SearchWnd, grrr..
	DialogCreateIndirect(chatwnd, IDD_CHAT);
	transferwnd->Create(this);
	DialogCreateIndirect(statisticswnd, IDD_STATISTICS);
	DialogCreateIndirect(kademliawnd, IDD_KADEMLIAWND);
	DialogCreateIndirect(ircwnd, IDD_IRC);

	// with the top rebar control, some XP themes look better with some additional lite borders.. some not..
	//serverwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//sharedfileswnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//searchwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//chatwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//transferwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//statisticswnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//kademliawnd->ModifyStyleEx(0, WS_EX_STATICEDGE);
	//ircwnd->ModifyStyleEx(0, WS_EX_STATICEDGE);

	// optional: restore last used main window dialog
	if (thePrefs.GetRestoreLastMainWndDlg()){
		switch (thePrefs.GetLastMainWndDlgID()){
		case IDD_SERVER:
			SetActiveDialog(serverwnd);
			break;
		case IDD_FILES:
			SetActiveDialog(sharedfileswnd);
			break;
		case IDD_SEARCH:
			SetActiveDialog(searchwnd);
			break;
		case IDD_CHAT:
			SetActiveDialog(chatwnd);
			break;
		case IDD_TRANSFER:
			SetActiveDialog(transferwnd);
			break;
		case IDD_STATISTICS:
			SetActiveDialog(statisticswnd);
			break;
		case IDD_KADEMLIAWND:
			SetActiveDialog(kademliawnd);
			break;
		case IDD_IRC:
			SetActiveDialog(ircwnd);
			break;
		}
	}

	// if still no active window, activate server window
	if (activewnd == NULL)
		SetActiveDialog(serverwnd);

	SetAllIcons();
	Localize();

	// set updateintervall of graphic rate display (in seconds)
	//ShowConnectionState(false);

	// adjust all main window sizes for toolbar height and maximize the child windows
	CRect rcClient, rcToolbar, rcStatusbar;
	GetClientRect(&rcClient);
	pwndToolbarX->GetWindowRect(&rcToolbar);
	statusbar->GetWindowRect(&rcStatusbar);
	rcClient.top += rcToolbar.Height();
	rcClient.bottom -= rcStatusbar.Height();

	CWnd* apWnds[] =
	{
		serverwnd,
		kademliawnd,
		transferwnd,
		sharedfileswnd,
		searchwnd,
		chatwnd,
		ircwnd,
		statisticswnd
	};
	for (int i = 0; i < _countof(apWnds); i++)
		apWnds[i]->SetWindowPos(NULL, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), SWP_NOZORDER);

	// anchors
	AddAnchor(*serverwnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*kademliawnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*transferwnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*sharedfileswnd,	TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(*searchwnd,		TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(*chatwnd,			TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*ircwnd,			TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*statisticswnd,	TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*pwndToolbarX,	TOP_LEFT, TOP_RIGHT);
	AddAnchor(*statusbar,		BOTTOM_LEFT, BOTTOM_RIGHT);

	statisticswnd->ShowInterval();

	// tray icon
	TraySetMinimizeToTray(thePrefs.GetMinTrayPTR());
	TrayMinimizeToTrayChange();

	ShowTransferRate(true);
    ShowPing();
	searchwnd->UpdateCatTabs();

	///////////////////////////////////////////////////////////////////////////
	// Restore saved window placement
	//
	WINDOWPLACEMENT wp = {0};
	wp.length = sizeof(wp);
	wp = thePrefs.GetEmuleWindowPlacement();
	if (m_bStartMinimized)
	{
		// To avoid the window flickering during startup we try to set the proper window show state right here.
		if (*thePrefs.GetMinTrayPTR())
		{
			// Minimize to System Tray
			//
			// Unfortunately this does not work. The eMule main window is a modal dialog which is invoked
			// by CDialog::DoModal which eventually calls CWnd::RunModalLoop. Look at 'MLF_SHOWONIDLE' and
			// 'bShowIdle' in the above noted functions to see why it's not possible to create the window
			// right in hidden state.

			//--- attempt #1
			//wp.showCmd = SW_HIDE;
			//TrayShow();
			//--- doesn't work at all

			//--- attempt #2
			//if (wp.showCmd == SW_SHOWMAXIMIZED)
			//	wp.flags = WPF_RESTORETOMAXIMIZED;
			//m_bStartMinimizedChecked = false; // post-hide the window..
			//--- creates window flickering

			//--- attempt #3
			// Minimize the window into the task bar and later move it into the tray bar
			if (wp.showCmd == SW_SHOWMAXIMIZED)
				wp.flags = WPF_RESTORETOMAXIMIZED;
			wp.showCmd = SW_MINIMIZE;
			m_bStartMinimizedChecked = false;

			// to get properly restored from tray bar (after attempt #3) we have to use a patched 'restore' window cmd..
			m_wpFirstRestore = thePrefs.GetEmuleWindowPlacement();
			m_wpFirstRestore.length = sizeof(m_wpFirstRestore);
			if (m_wpFirstRestore.showCmd != SW_SHOWMAXIMIZED)
				m_wpFirstRestore.showCmd = SW_SHOWNORMAL;
		}
		else {
			// Minimize to System Taskbar
			//
			if (wp.showCmd == SW_SHOWMAXIMIZED)
				wp.flags = WPF_RESTORETOMAXIMIZED;
			wp.showCmd = SW_MINIMIZE; // Minimize window but do not activate it.
			m_bStartMinimizedChecked = true;
		}
	}
	else
	{
		// Allow only SW_SHOWNORMAL and SW_SHOWMAXIMIZED. Ignore SW_SHOWMINIMIZED to make sure the window
		// becomes visible. If user wants SW_SHOWMINIMIZED, we already have an explicit option for this (see above).
		if (wp.showCmd != SW_SHOWMAXIMIZED)
			wp.showCmd = SW_SHOWNORMAL;
		m_bStartMinimizedChecked = true;
	}
	SetWindowPlacement(&wp);

	if (thePrefs.GetWSIsEnabled())
		theApp.webserver->StartServer();
	theApp.mmserver->Init();

	VERIFY( (m_hTimer = ::SetTimer(NULL, NULL, 300, StartupTimer)) != NULL );
	if (thePrefs.GetVerbose() && !m_hTimer)
		AddDebugLogLine(true,_T("Failed to create 'startup' timer - %s"),GetErrorMessage(GetLastError()));

	theStats.starttime = GetTickCount();

	// Start UPnP prot forwarding
	if (thePrefs.IsUPnPEnabled())
		StartUPnP();

	if (thePrefs.IsFirstStart())
	{
		// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
		m_bStartMinimized = false;
		DestroySplash();
		FirstTimeWizard();
	}

	VERIFY( m_pDropTarget->Register(this) );

	// initalize PeerCache
	theApp.m_pPeerCache->Init(thePrefs.GetPeerCacheLastSearch(), thePrefs.WasPeerCacheFound(), thePrefs.IsPeerCacheDownloadEnabled(), thePrefs.GetPeerCachePort());
	
	// start aichsyncthread
	AfxBeginThread(RUNTIME_CLASS(CAICHSyncThread), THREAD_PRIORITY_BELOW_NORMAL,0);

	// debug info
	DebugLog(_T("Using '%s' as config directory"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	if (thePrefs.GetUseOverlappedSockets())
		DebugLog(_T("Using overlapped sockets"));

	if (!thePrefs.HasCustomTaskIconColor())
		SetTaskbarIconColor();

	return TRUE;
}

// modders: dont remove or change the original versioncheck! (additionals are ok)
void CemuleDlg::DoVersioncheck(bool manual) {
#ifndef _DEVBUILD
	if (!manual && thePrefs.GetLastVC()!=0) {
		CTime last(thePrefs.GetLastVC());
		struct tm tmTemp;
		time_t tLast = safe_mktime(last.GetLocalTm(&tmTemp));
		time_t tNow = safe_mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp));
#ifndef _BETA
		if ( (difftime(tNow,tLast) / 86400) < thePrefs.GetUpdateDays() ){
#else
		if ( (difftime(tNow,tLast) / 86400) < 3 ){
#endif
			return;
		}
	}
#ifndef _BETA
	if (WSAAsyncGetHostByName(m_hWnd, UM_VERSIONCHECK_RESPONSE, "vcdns2.emule-project.org", m_acVCDNSBuffer, sizeof(m_acVCDNSBuffer)) == 0){
#else
	if (WSAAsyncGetHostByName(m_hWnd, UM_VERSIONCHECK_RESPONSE, "vcdns1.emule-project.org", m_acVCDNSBuffer, sizeof(m_acVCDNSBuffer)) == 0){
#endif
		AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
	}
#endif
}

void CALLBACK CemuleDlg::StartupTimer(HWND /*hwnd*/, UINT /*uiMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		switch(theApp.emuledlg->status){
			case 0:
				theApp.emuledlg->status++;
				theApp.emuledlg->ready = true;
				theApp.sharedfiles->SetOutputCtrl(&theApp.emuledlg->sharedfileswnd->sharedfilesctrl);
				theApp.emuledlg->status++;
				break;
			case 1:
				break;
			case 2:
				theApp.emuledlg->status++;
				try{
					theApp.serverlist->Init();
				}
				catch(...){
					ASSERT(0);
					LogError(LOG_STATUSBAR,_T("Failed to initialize server list - Unknown exception"));
				}
				theApp.emuledlg->status++;
				break;
			case 3:
				break;
			case 4:{
				bool bError = false;
				theApp.emuledlg->status++;

				// NOTE: If we have an unhandled exception in CDownloadQueue::Init, MFC will silently catch it
				// and the creation of the TCP and the UDP socket will not be done -> client will get a LowID!
				try{
					theApp.downloadqueue->Init();
				}
				catch(...){
					ASSERT(0);
					LogError(LOG_STATUSBAR,_T("Failed to initialize download queue - Unknown exception"));
					bError = true;
				}
				if (!theApp.listensocket->StartListening()) {
					CString strError;
					strError.Format(GetResString(IDS_MAIN_SOCKETERROR), thePrefs.GetPort());
					LogError(LOG_STATUSBAR, _T("%s"), strError);
					if (thePrefs.GetNotifierOnImportantError())
						theApp.emuledlg->ShowNotifier(strError, TBN_IMPORTANTEVENT);
					bError = true;
				}
				if (!theApp.clientudp->Create()) {
					CString strError;
					strError.Format(GetResString(IDS_MAIN_SOCKETERROR), thePrefs.GetUDPPort());
					LogError(LOG_STATUSBAR, _T("%s"), strError);
					if (thePrefs.GetNotifierOnImportantError())
						theApp.emuledlg->ShowNotifier(strError, TBN_IMPORTANTEVENT);
				}
				
				if (!bError) // show the success msg, only if we had no serious error
					AddLogLine(true, GetResString(IDS_MAIN_READY), theApp.m_strCurVersionLong);

				if (thePrefs.DoAutoConnect())
					theApp.emuledlg->OnBnClickedConnect();

#ifdef HAVE_WIN7_SDK_H
				theApp.emuledlg->UpdateStatusBarProgress();
#endif
				break;
			}
			case 5:
				if (thePrefs.IsStoringSearchesEnabled())
					theApp.searchlist->LoadSearches();
				theApp.emuledlg->status++;
				break;
			default:
				theApp.emuledlg->StopTimer();
		}
	}
	CATCH_DFLT_EXCEPTIONS(_T("CemuleDlg::StartupTimer"))
}

void CemuleDlg::StopTimer()
{
	if (m_hTimer){
		VERIFY( ::KillTimer(NULL, m_hTimer) );
		m_hTimer = 0;
	}
	if (thePrefs.UpdateNotify())
		DoVersioncheck(false);
	
	if (theApp.pstrPendingLink != NULL){
		OnWMData(NULL, (LPARAM)&theApp.sendstruct);
		delete theApp.pstrPendingLink;
	}

}

void CemuleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// Systemmenu-Speedselector
	if (nID >= MP_QS_U10 && nID <= MP_QS_UP10) {
		QuickSpeedUpload(nID);
		return;
	}
	if (nID >= MP_QS_D10 && nID <= MP_QS_DC) {
		QuickSpeedDownload(nID);
		return;
	}
	if (nID == MP_QS_PA || nID == MP_QS_UA) {
		QuickSpeedOther(nID);
		return;
	}
	
	switch (nID /*& 0xFFF0*/)
	{
		case MP_ABOUTBOX: {
			CCreditsDlg dlgAbout;
			dlgAbout.DoModal();
			break;
		}
		case MP_VERSIONCHECK:
			DoVersioncheck(true);
			break;
		case MP_CONNECT:
			StartConnection();
			break;
		case MP_DISCONNECT:
			CloseConnection();
			break;
		default:
			CTrayDialog::OnSysCommand(nID, lParam);
	}

	if ((nID & 0xFFF0) == SC_MINIMIZE		||
		(nID & 0xFFF0) == MP_MINIMIZETOTRAY	||
		(nID & 0xFFF0) == SC_RESTORE		||
		(nID & 0xFFF0) == SC_MAXIMIZE)
	{
		ShowTransferRate(true);
		ShowPing();
		transferwnd->UpdateCatTabTitles();
	}
}

void CemuleDlg::PostStartupMinimized()
{
	if (!m_bStartMinimizedChecked)
	{
		//TODO: Use full initialized 'WINDOWPLACEMENT' and remove the 'OnCancel' call...
		// Isn't that easy.. Read comments in OnInitDialog..
		m_bStartMinimizedChecked = true;
		if (m_bStartMinimized)
		{
			if (theApp.DidWeAutoStart())
			{
				if (!thePrefs.mintotray) {
					thePrefs.mintotray = true;
					MinimizeWindow();
					thePrefs.mintotray = false;
				}
				else
					MinimizeWindow();
			}
			else
				MinimizeWindow();
		}
	}
}

void CemuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
		CTrayDialog::OnPaint();
}

HCURSOR CemuleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CemuleDlg::OnBnClickedConnect(){
	if (!theApp.IsConnected())
		//connect if not currently connected
		if (!theApp.serverconnect->IsConnecting() && !Kademlia::CKademlia::IsRunning() ){
			StartConnection();
		}
		else {
			CloseConnection();
		}
	else{
		//disconnect if currently connected
		CloseConnection();
	}
}

void CemuleDlg::ResetServerInfo(){
	serverwnd->servermsgbox->Reset();
}
void CemuleDlg::ResetLog(){
	serverwnd->logbox->Reset();
}

void CemuleDlg::ResetDebugLog(){
	serverwnd->debuglog->Reset();
}

void CemuleDlg::AddLogText(UINT uFlags, LPCTSTR pszText)
{
	if (GetCurrentThreadId() != g_uMainThreadId)
	{
		theApp.QueueLogLineEx(uFlags, _T("%s"), pszText);
		return;
	}

	if (uFlags & LOG_STATUSBAR)
	{
        if (statusbar->m_hWnd /*&& ready*/)
		{
			if (theApp.m_app_state != APP_STATE_SHUTTINGDOWN)
				statusbar->SetText(pszText, SBarLog, 0);
		}
		else
			AfxMessageBox(pszText);
	}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	Debug(_T("%s\n"), pszText);
#endif

	if ((uFlags & LOG_DEBUG) && !thePrefs.GetVerbose())
		return;

	TCHAR temp[1060];
	int iLen = _sntprintf(temp, _countof(temp), _T("%s: %s\r\n"), CTime::GetCurrentTime().Format(thePrefs.GetDateTimeFormat4Log()), pszText);
	if (iLen >= 0)
	{
		if (!(uFlags & LOG_DEBUG))
		{
			serverwnd->logbox->AddTyped(temp, iLen, uFlags & LOGMSGTYPEMASK);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneLog, TRUE);
			if (!(uFlags & LOG_DONTNOTIFY) && ready)
				ShowNotifier(pszText, TBN_LOG);
			if (thePrefs.GetLog2Disk())
				theLog.Log(temp, iLen);
		}

		if (thePrefs.GetVerbose() && ((uFlags & LOG_DEBUG) || thePrefs.GetFullVerbose()))
		{
			serverwnd->debuglog->AddTyped(temp, iLen, uFlags & LOGMSGTYPEMASK);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneVerboseLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneVerboseLog, TRUE);

			if (thePrefs.GetDebug2Disk())
				theVerboseLog.Log(temp, iLen);
		}
	}
}

CString CemuleDlg::GetLastLogEntry()
{
	return serverwnd->logbox->GetLastLogEntry();
}

CString CemuleDlg::GetAllLogEntries()
{
	return serverwnd->logbox->GetAllLogEntries();
}

CString CemuleDlg::GetLastDebugLogEntry()
{
	return serverwnd->debuglog->GetLastLogEntry();
}

CString CemuleDlg::GetAllDebugLogEntries()
{
	return serverwnd->debuglog->GetAllLogEntries();
}
CString CemuleDlg::GetServerInfoText()
{
	return serverwnd->servermsgbox->GetText();
}

void CemuleDlg::AddServerMessageLine(UINT uFlags, LPCTSTR pszLine)
{
	CString strMsgLine(pszLine);
	strMsgLine += _T('\n');
	if ((uFlags & LOGMSGTYPEMASK) == LOG_INFO)
		serverwnd->servermsgbox->AppendText(strMsgLine);
	else
		serverwnd->servermsgbox->AddTyped(strMsgLine, strMsgLine.GetLength(), uFlags & LOGMSGTYPEMASK);
	if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneServerInfo)
		serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneServerInfo, TRUE);
}

UINT CemuleDlg::GetConnectionStateIconIndex() const
{
	if (theApp.serverconnect->IsConnected() && !Kademlia::CKademlia::IsConnected())
	{
		if (theApp.serverconnect->IsLowID())
			return 3; // LowNot
		else
			return 6; // HighNot
	}
	else if (!theApp.serverconnect->IsConnected() && Kademlia::CKademlia::IsConnected())
	{
		if (Kademlia::CKademlia::IsFirewalled())
			return 1; // NotLow
		else
			return 2; // NotHigh
	}
	else if (theApp.serverconnect->IsConnected() && Kademlia::CKademlia::IsConnected())
	{
		if (theApp.serverconnect->IsLowID() && Kademlia::CKademlia::IsFirewalled())
			return 4; // LowLow
		else if (theApp.serverconnect->IsLowID())
			return 5; // LowHigh
		else if (Kademlia::CKademlia::IsFirewalled())
			return 7; // HighLow
		else
			return 8; // HighHigh
	}
	else
	{
		return 0; // NotNot
	}
}

void CemuleDlg::ShowConnectionStateIcon()
{
	UINT uIconIdx = GetConnectionStateIconIndex();
	if (uIconIdx >= _countof(connicons)){
		ASSERT(0);
		uIconIdx = 0;
	}
	statusbar->SetIcon(SBarConnected, connicons[uIconIdx]);
}

CString CemuleDlg::GetConnectionStateString()
{
	CString status;
	if (theApp.serverconnect->IsConnected())
		status = _T("eD2K:") + GetResString(IDS_CONNECTED);
	else if (theApp.serverconnect->IsConnecting())
		status = _T("eD2K:") + GetResString(IDS_CONNECTING);
	else
		status = _T("eD2K:") + GetResString(IDS_NOTCONNECTED);

	if (Kademlia::CKademlia::IsConnected())
		status += _T("|Kad:") + GetResString(IDS_CONNECTED);
	else if (Kademlia::CKademlia::IsRunning())
		status += _T("|Kad:") + GetResString(IDS_CONNECTING);
	else
		status += _T("|Kad:") + GetResString(IDS_NOTCONNECTED);
	return status;
}

void CemuleDlg::ShowConnectionState()
{
	theApp.downloadqueue->OnConnectionState(theApp.IsConnected());
	serverwnd->UpdateMyInfo();
	serverwnd->UpdateControlsState();
	kademliawnd->UpdateControlsState();

	ShowConnectionStateIcon();
	statusbar->SetText(GetConnectionStateString(), SBarConnected, 0);

	if (theApp.IsConnected())
	{
		CString strPane(GetResString(IDS_MAIN_BTN_DISCONNECT));
		TBBUTTONINFO tbi;
		tbi.cbSize = sizeof(TBBUTTONINFO);
		tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
		tbi.iImage = 1;
		tbi.pszText = const_cast<LPTSTR>((LPCTSTR)strPane);
		toolbar->SetButtonInfo(TBBTN_CONNECT, &tbi);
	}
	else
	{
		if (theApp.serverconnect->IsConnecting() || Kademlia::CKademlia::IsRunning()) 
		{
			CString strPane(GetResString(IDS_MAIN_BTN_CANCEL));
			TBBUTTONINFO tbi;
			tbi.cbSize = sizeof(TBBUTTONINFO);
			tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
			tbi.iImage = 2;
			tbi.pszText = const_cast<LPTSTR>((LPCTSTR)strPane);
			toolbar->SetButtonInfo(TBBTN_CONNECT, &tbi);
		} 
		else 
		{
			CString strPane(GetResString(IDS_MAIN_BTN_CONNECT));
			TBBUTTONINFO tbi;
			tbi.cbSize = sizeof(TBBUTTONINFO);
			tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
			tbi.iImage = 0;
			tbi.pszText = const_cast<LPTSTR>((LPCTSTR)strPane);
			toolbar->SetButtonInfo(TBBTN_CONNECT, &tbi);
		}
	}
	ShowUserCount();
#ifdef HAVE_WIN7_SDK_H
	UpdateThumbBarButtons();
#endif
}

void CemuleDlg::ShowUserCount()
{
	uint32 totaluser, totalfile;
	totaluser = totalfile = 0;
	theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	CString buffer;
	if (theApp.serverconnect->IsConnected() && Kademlia::CKademlia::IsRunning() && Kademlia::CKademlia::IsConnected())
		buffer.Format(_T("%s:%s(%s)|%s:%s(%s)"), GetResString(IDS_UUSERS), CastItoIShort(totaluser, false, 1), CastItoIShort(Kademlia::CKademlia::GetKademliaUsers(), false, 1), GetResString(IDS_FILES), CastItoIShort(totalfile, false, 1), CastItoIShort(Kademlia::CKademlia::GetKademliaFiles(), false, 1));
	else if (theApp.serverconnect->IsConnected())
		buffer.Format(_T("%s:%s|%s:%s"), GetResString(IDS_UUSERS), CastItoIShort(totaluser, false, 1), GetResString(IDS_FILES), CastItoIShort(totalfile, false, 1));
	else if (Kademlia::CKademlia::IsRunning() && Kademlia::CKademlia::IsConnected())
		buffer.Format(_T("%s:%s|%s:%s"), GetResString(IDS_UUSERS), CastItoIShort(Kademlia::CKademlia::GetKademliaUsers(), false, 1), GetResString(IDS_FILES), CastItoIShort(Kademlia::CKademlia::GetKademliaFiles(), false, 1));
	else
		buffer.Format(_T("%s:0|%s:0"), GetResString(IDS_UUSERS), GetResString(IDS_FILES));
	statusbar->SetText(buffer, SBarUsers, 0);
}

void CemuleDlg::ShowMessageState(UINT iconnr)
{
	m_iMsgIcon = iconnr;
	statusbar->SetIcon(SBarChatMsg, imicons[m_iMsgIcon]);
}

void CemuleDlg::ShowTransferStateIcon()
{
	if (m_uUpDatarate && m_uDownDatarate)
		statusbar->SetIcon(SBarUpDown, transicons[3]);
	else if (m_uUpDatarate)
		statusbar->SetIcon(SBarUpDown, transicons[2]);
	else if (m_uDownDatarate)
		statusbar->SetIcon(SBarUpDown, transicons[1]);
	else
		statusbar->SetIcon(SBarUpDown, transicons[0]);
}

CString CemuleDlg::GetUpDatarateString(UINT uUpDatarate)
{
	m_uUpDatarate = uUpDatarate != (UINT)-1 ? uUpDatarate : theApp.uploadqueue->GetDatarate();
	TCHAR szBuff[128];
	if (thePrefs.ShowOverhead()) {
		_sntprintf(szBuff, _countof(szBuff), _T("%.1f (%.1f)"), (float)m_uUpDatarate/1024, (float)theStats.GetUpDatarateOverhead()/1024);
		szBuff[_countof(szBuff) - 1] = _T('\0');
	}
	else {
		_sntprintf(szBuff, _countof(szBuff), _T("%.1f"), (float)m_uUpDatarate/1024);
		szBuff[_countof(szBuff) - 1] = _T('\0');
	}
	return szBuff;
}

CString CemuleDlg::GetDownDatarateString(UINT uDownDatarate)
{
	m_uDownDatarate = uDownDatarate != (UINT)-1 ? uDownDatarate : theApp.downloadqueue->GetDatarate();
	TCHAR szBuff[128];
	if (thePrefs.ShowOverhead()) {
		_sntprintf(szBuff, _countof(szBuff), _T("%.1f (%.1f)"), (float)m_uDownDatarate/1024, (float)theStats.GetDownDatarateOverhead()/1024);
		szBuff[_countof(szBuff) - 1] = _T('\0');
	}
	else {
		_sntprintf(szBuff, _countof(szBuff), _T("%.1f"), (float)m_uDownDatarate/1024);
		szBuff[_countof(szBuff) - 1] = _T('\0');
	}
	return szBuff;
}

CString CemuleDlg::GetTransferRateString()
{
	TCHAR szBuff[128];
	if (thePrefs.ShowOverhead()) {
		_sntprintf(szBuff, _countof(szBuff), GetResString(IDS_UPDOWN),
			       (float)m_uUpDatarate/1024, (float)theStats.GetUpDatarateOverhead()/1024,
			       (float)m_uDownDatarate/1024, (float)theStats.GetDownDatarateOverhead()/1024);
		szBuff[_countof(szBuff) - 1] = _T('\0');
	}
	else {
		_sntprintf(szBuff, _countof(szBuff), GetResString(IDS_UPDOWNSMALL), (float)m_uUpDatarate/1024, (float)m_uDownDatarate/1024);
		szBuff[_countof(szBuff) - 1] = _T('\0');
	}
	return szBuff;
}

void CemuleDlg::ShowTransferRate(bool bForceAll)
{
	if (bForceAll)
		m_uLastSysTrayIconCookie = SYS_TRAY_ICON_COOKIE_FORCE_UPDATE;

	m_uDownDatarate = theApp.downloadqueue->GetDatarate();
	m_uUpDatarate = theApp.uploadqueue->GetDatarate();

	CString strTransferRate = GetTransferRateString();
	if (TrayIsVisible() || bForceAll)
	{
		TCHAR buffer2[64];
		// set trayicon-icon
		int iDownRateProcent = (int)ceil((m_uDownDatarate/10.24) / thePrefs.GetMaxGraphDownloadRate());
		if (iDownRateProcent > 100)
			iDownRateProcent = 100;
		UpdateTrayIcon(iDownRateProcent);

		if (theApp.IsConnected()) {
			_sntprintf(buffer2, _countof(buffer2), _T("eMule v%s (%s)\r\n%s"), theApp.m_strCurVersionLong, GetResString(IDS_CONNECTED), strTransferRate);
			buffer2[_countof(buffer2) - 1] = _T('\0');
		}
		else {
			_sntprintf(buffer2, _countof(buffer2), _T("eMule v%s (%s)\r\n%s"), theApp.m_strCurVersionLong, GetResString(IDS_DISCONNECTED), strTransferRate);
			buffer2[_countof(buffer2) - 1] = _T('\0');
		}

		// Win98: '\r\n' is not displayed correctly in tooltip
		if (afxIsWin95()) {
			LPTSTR psz = buffer2;
			while (*psz) {
				if (*psz == _T('\r') || *psz == _T('\n'))
					*psz = _T(' ');
				psz++;
			}
		}
		TraySetToolTip(buffer2);
	}

	if (IsWindowVisible() || bForceAll)
	{
		statusbar->SetText(strTransferRate, SBarUpDown, 0);
		ShowTransferStateIcon();
	}
	if (IsWindowVisible() && thePrefs.ShowRatesOnTitle())
	{
		TCHAR szBuff[128];
		_sntprintf(szBuff, _countof(szBuff), _T("(U:%.1f D:%.1f) eMule v%s"), (float)m_uUpDatarate/1024, (float)m_uDownDatarate/1024, theApp.m_strCurVersionLong);
		szBuff[_countof(szBuff) - 1] = _T('\0');
		SetWindowText(szBuff);
	}
	if (m_pMiniMule && m_pMiniMule->m_hWnd && m_pMiniMule->IsWindowVisible() && !m_pMiniMule->GetAutoClose() && !m_pMiniMule->IsInInitDialog())
	{
		m_pMiniMule->UpdateContent(m_uUpDatarate, m_uDownDatarate);
	}
}

void CemuleDlg::ShowPing()
{
    if (IsWindowVisible())
	{
        CString buffer;
        if (thePrefs.IsDynUpEnabled())
		{
			CurrentPingStruct lastPing = theApp.lastCommonRouteFinder->GetCurrentPing();
            if (lastPing.state.GetLength() == 0)
			{
                if (lastPing.lowest > 0 && !thePrefs.IsDynUpUseMillisecondPingTolerance())
                    buffer.Format(_T("%.1f | %ims | %i%%"),lastPing.currentLimit/1024.0f, lastPing.latency, lastPing.latency*100/lastPing.lowest);
                else
                    buffer.Format(_T("%.1f | %ims"),lastPing.currentLimit/1024.0f, lastPing.latency);
            }
			else
                buffer.SetString(lastPing.state);
			statusbar->SetText(buffer, SBarUSS, 0);
        };
    }
}

void CemuleDlg::OnOK()
{
}

void CemuleDlg::OnCancel()
{
	if (!thePrefs.GetStraightWindowStyles())
		MinimizeWindow();
}

void CemuleDlg::MinimizeWindow()
{
	if (*thePrefs.GetMinTrayPTR())
	{
		TrayShow();
		ShowWindow(SW_HIDE);
	}
	else
	{
		ShowWindow(SW_MINIMIZE);
	}
	ShowTransferRate();
	ShowPing();
}

void CemuleDlg::SetActiveDialog(CWnd* dlg)
{
	if (dlg == activewnd)
		return;
	if (activewnd)
		activewnd->ShowWindow(SW_HIDE);
	dlg->ShowWindow(SW_SHOW);
	dlg->SetFocus();
	activewnd = dlg;
	int iToolbarButtonID = MapWindowToToolbarButton(dlg);
	if (iToolbarButtonID != -1)
		toolbar->PressMuleButton(iToolbarButtonID);
	if (dlg == transferwnd){
		if (thePrefs.ShowCatTabInfos())
			transferwnd->UpdateCatTabTitles();
	}
	else if (dlg == chatwnd){
		chatwnd->chatselector.ShowChat();
	}
	else if (dlg == statisticswnd){
		statisticswnd->ShowStatistics();
	}
}

void CemuleDlg::SetStatusBarPartsSize()
{
	CRect rect;
	statusbar->GetClientRect(&rect);
	int ussShift = 0;
	if(thePrefs.IsDynUpEnabled())
	{
        if (thePrefs.IsDynUpUseMillisecondPingTolerance())
            ussShift = 65;
        else
            ussShift = 110;
	}
		int aiWidths[6] =
		{ 
			rect.right - 675 - ussShift,
			rect.right - 440 - ussShift,
			rect.right - 250 - ussShift,
			rect.right - ussShift - 25,
			rect.right -  25, 
			-1
		};
		statusbar->SetParts(_countof(aiWidths), aiWidths);
}

void CemuleDlg::OnSize(UINT nType, int cx, int cy)
{
	CTrayDialog::OnSize(nType, cx, cy);
	SetStatusBarPartsSize();
	// we might receive this message during shutdown -> bad
	if (transferwnd != NULL && IsRunning())
		transferwnd->VerifyCatTabSize();
}

void CemuleDlg::ProcessED2KLink(LPCTSTR pszData)
{
	try {
		CString link2;
		CString link;
		link2 = pszData;
		link2.Replace(_T("%7c"),_T("|"));
		link2.Replace(_T("%7C"),_T("|"));
		link = OptUtf8ToStr(URLDecode(link2));
		CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(link);
		_ASSERT( pLink !=0 );
		switch (pLink->GetKind()) {
		case CED2KLink::kFile:
			{
				CED2KFileLink* pFileLink = pLink->GetFileLink();
				_ASSERT(pFileLink !=0);
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink,searchwnd->GetSelectedCat());
			}
			break;
		case CED2KLink::kServerList:
			{
				CED2KServerListLink* pListLink = pLink->GetServerListLink(); 
				_ASSERT( pListLink !=0 ); 
				CString strAddress = pListLink->GetAddress(); 
				if(strAddress.GetLength() != 0)
					serverwnd->UpdateServerMetFromURL(strAddress);
			}
			break;
		case CED2KLink::kNodesList:
			{
				CED2KNodesListLink* pListLink = pLink->GetNodesListLink(); 
				_ASSERT( pListLink !=0 ); 
				CString strAddress = pListLink->GetAddress();
				// Becasue the nodes.dat is vital for kad and its routing and doesn't needs to be updated in general
				// we request a confirm to avoid accidental / malicious updating of this file. This is a bit inconsitent
				// as the same kinda applies to the server.met, but those require more updates and are easier to understand
				CString strConfirm;
				strConfirm.Format(GetResString(IDS_CONFIRMNODESDOWNLOAD), strAddress);
				if(strAddress.GetLength() != 0 && AfxMessageBox(strConfirm, MB_YESNO | MB_ICONQUESTION, 0) == IDYES)
					kademliawnd->UpdateNodesDatFromURL(strAddress);
			}
			break;
		case CED2KLink::kServer:
			{
				CString defName;
				CED2KServerLink* pSrvLink = pLink->GetServerLink();
				_ASSERT( pSrvLink !=0 );
				CServer* pSrv = new CServer(pSrvLink->GetPort(), pSrvLink->GetAddress());
				_ASSERT( pSrv !=0 );
				pSrvLink->GetDefaultName(defName);
				pSrv->SetListName(defName);

				// Barry - Default all new servers to high priority
				if (thePrefs.GetManualAddedServersHighPriority())
					pSrv->SetPreference(SRV_PR_HIGH);

				if (!serverwnd->serverlistctrl.AddServer(pSrv,true)) 
					delete pSrv; 
				else
					AddLogLine(true,GetResString(IDS_SERVERADDED), pSrv->GetListName());
			}
			break;
		case CED2KLink::kSearch:
			{
				CED2KSearchLink* pListLink = pLink->GetSearchLink();
				_ASSERT( pListLink !=0 ); 
				SetActiveDialog(searchwnd);
				searchwnd->ProcessEd2kSearchLinkRequest(pListLink->GetSearchTerm());
			}
			break;
		default:
			break;
		}
		delete pLink;
	}
	catch(CString strError){
		LogWarning(LOG_STATUSBAR, GetResString(IDS_LINKNOTADDED) + _T(" - ") + strError);
	}
	catch(...){
		LogWarning(LOG_STATUSBAR, GetResString(IDS_LINKNOTADDED));
	}
}

LRESULT CemuleDlg::OnWMData(WPARAM /*wParam*/, LPARAM lParam)
{
	PCOPYDATASTRUCT data = (PCOPYDATASTRUCT)lParam;
	if (data->dwData == OP_ED2KLINK)
	{
		if (thePrefs.IsBringToFront())
		{
			FlashWindow(TRUE);
			if (IsIconic())
				ShowWindow(SW_SHOWNORMAL);
			else if (TrayHide())
				RestoreWindow();
			else
				SetForegroundWindow();
		}
		ProcessED2KLink((LPCTSTR)data->lpData);
	}
	else if(data->dwData == OP_COLLECTION){
		FlashWindow(TRUE);
		if (IsIconic())
			ShowWindow(SW_SHOWNORMAL);
		else if (TrayHide())
			RestoreWindow();
		else
			SetForegroundWindow();

		CCollection* pCollection = new CCollection();
		CString strPath = CString((LPCTSTR)data->lpData);
		if (pCollection->InitCollectionFromFile(strPath, strPath.Right((strPath.GetLength()-1)-strPath.ReverseFind('\\')))){
			CCollectionViewDialog dialog;
			dialog.SetCollection(pCollection);
			dialog.DoModal();
		}
		delete pCollection;
	}
	else if (data->dwData == OP_CLCOMMAND){
		// command line command received
		CString clcommand((LPCTSTR)data->lpData);
		clcommand.MakeLower();
		AddLogLine(true,_T("CLI: %s"),clcommand);

		if (clcommand==_T("connect")) {StartConnection(); return true;}
		if (clcommand==_T("disconnect")) {theApp.serverconnect->Disconnect(); return true;}
		if (clcommand==_T("resume")) {theApp.downloadqueue->StartNextFile(); return true;}
		if (clcommand==_T("exit"))
		{
			theApp.m_app_state = APP_STATE_SHUTTINGDOWN; // do no ask to close
			OnClose(); 
			return true;
		}
		if (clcommand==_T("restore")) {RestoreWindow();return true;}
		if (clcommand==_T("reloadipf")) {theApp.ipfilter->LoadFromDefaultFile(); return true;}
		if (clcommand.Left(7).MakeLower()==_T("limits=") && clcommand.GetLength()>8) {
			CString down;
			CString up=clcommand.Mid(7);
			int pos=up.Find(_T(','));
			if (pos>0) {
				down=up.Mid(pos+1);
				up=up.Left(pos);
			}
			if (down.GetLength()>0) thePrefs.SetMaxDownload(_tstoi(down));
			if (up.GetLength()>0) thePrefs.SetMaxUpload(_tstoi(up));

			return true;
		}

		if (clcommand==_T("help") || clcommand==_T("/?")) {
			// show usage
			return true;
		}

		if (clcommand==_T("status")) {
			CString strBuff;
			strBuff.Format(_T("%sstatus.log"), thePrefs.GetMuleDirectory(EMULE_CONFIGBASEDIR));
			FILE* file = _tfsopen(strBuff, _T("wt"), _SH_DENYWR);
			if (file){
				if (theApp.serverconnect->IsConnected())
					strBuff = GetResString(IDS_CONNECTED);
				else if (theApp.serverconnect->IsConnecting())
					strBuff = GetResString(IDS_CONNECTING);
				else
					strBuff = GetResString(IDS_DISCONNECTED);
				_ftprintf(file, _T("%s\n"), strBuff);

				strBuff.Format(GetResString(IDS_UPDOWNSMALL), (float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
				_ftprintf(file, _T("%s"), strBuff); // next string (getTextList) is already prefixed with '\n'!
				_ftprintf(file, _T("%s\n"), transferwnd->GetDownloadList()->getTextList());
				
				fclose(file);
			}
			return true;
		}
		// show "unknown command";
	}
	return true;
}

LRESULT CemuleDlg::OnFileHashed(WPARAM wParam, LPARAM lParam)
{
	if (theApp.m_app_state == APP_STATE_SHUTTINGDOWN)
		return FALSE;

	CKnownFile* result = (CKnownFile*)lParam;
	ASSERT( result->IsKindOf(RUNTIME_CLASS(CKnownFile)) );

	if (wParam)
	{
		// File hashing finished for a part file when:
		// - part file just completed
		// - part file was rehashed at startup because the file date of part.met did not match the part file date

		CPartFile* requester = (CPartFile*)wParam;
		ASSERT( requester->IsKindOf(RUNTIME_CLASS(CPartFile)) );

		// SLUGFILLER: SafeHash - could have been canceled
		if (theApp.downloadqueue->IsPartFile(requester))
			requester->PartFileHashFinished(result);
		else
			delete result;
		// SLUGFILLER: SafeHash
	}
	else
	{
		ASSERT( !result->IsKindOf(RUNTIME_CLASS(CPartFile)) );

		// File hashing finished for a shared file (none partfile)
		//	- reading shared directories at startup and hashing files which were not found in known.met
		//	- reading shared directories during runtime (user hit Reload button, added a shared directory, ...)
		theApp.sharedfiles->FileHashingFinished(result);
	}
	return TRUE;
}

LRESULT CemuleDlg::OnFileOpProgress(WPARAM wParam, LPARAM lParam)
{
	if (theApp.m_app_state == APP_STATE_SHUTTINGDOWN)
		return FALSE;

	CKnownFile* pKnownFile = (CKnownFile*)lParam;
	ASSERT( pKnownFile->IsKindOf(RUNTIME_CLASS(CKnownFile)) );

	if (pKnownFile->IsKindOf(RUNTIME_CLASS(CPartFile)))
	{
		CPartFile* pPartFile = static_cast<CPartFile*>(pKnownFile);
		pPartFile->SetFileOpProgress(wParam);
		pPartFile->UpdateDisplayedInfo(true);
	}

	return 0;
}

// SLUGFILLER: SafeHash
LRESULT CemuleDlg::OnHashFailed(WPARAM /*wParam*/, LPARAM lParam)
{
	theApp.sharedfiles->HashFailed((UnknownFile_Struct*)lParam);
	return 0;
}
// SLUGFILLER: SafeHash

LRESULT CemuleDlg::OnFileAllocExc(WPARAM wParam,LPARAM lParam)
{
	if (lParam == 0)
		((CPartFile*)wParam)->FlushBuffersExceptionHandler();
	else
		((CPartFile*)wParam)->FlushBuffersExceptionHandler((CFileException*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnFileCompleted(WPARAM wParam, LPARAM lParam)
{
	CPartFile* partfile = (CPartFile*)lParam;
	ASSERT( partfile != NULL );
	if (partfile)
		partfile->PerformFileCompleteEnd(wParam);
	return 0;
}

#ifdef _DEBUG
void BeBusy(UINT uSeconds, LPCSTR pszCaller)
{
	UINT s = 0;
	while (uSeconds--) {
		theVerboseLog.Logf(_T("%hs: called=%hs, waited %u sec."), __FUNCTION__, pszCaller, s++);
		Sleep(1000);
	}
}
#endif

BOOL CemuleDlg::OnQueryEndSession()
{
	AddDebugLogLine(DLP_VERYLOW, _T("%hs"), __FUNCTION__);
	if (!CTrayDialog::OnQueryEndSession())
		return FALSE;

	AddDebugLogLine(DLP_VERYLOW, _T("%hs: returning TRUE"), __FUNCTION__);
	return TRUE;
}

void CemuleDlg::OnEndSession(BOOL bEnding)
{
	AddDebugLogLine(DLP_VERYLOW, _T("%hs: bEnding=%d"), __FUNCTION__, bEnding);
	if (bEnding && theApp.m_app_state == APP_STATE_RUNNING)
	{
		// If eMule was *not* started with "RUNAS":
		// When user is logging of (or reboots or shutdown system), Windows sends the
		// WM_QUERYENDSESSION/WM_ENDSESSION to all top level windows.
		// Here we can consume as much time as we need to perform our shutdown. Even if we
		// take longer than 20 seconds, Windows will just show a dialog box that 'emule'
		// is not terminating in time and gives the user a chance to cancel that. If the user
		// does not cancel the Windows dialog, Windows will though wait until eMule has 
		// terminated by itself - no data loss, no file corruption, everything is fine.
		theApp.m_app_state = APP_STATE_SHUTTINGDOWN;
		OnClose();
	}

	CTrayDialog::OnEndSession(bEnding);
	AddDebugLogLine(DLP_VERYLOW, _T("%hs: returning"), __FUNCTION__);
}

LRESULT CemuleDlg::OnUserChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	AddDebugLogLine(DLP_VERYLOW, _T("%hs"), __FUNCTION__);
	// Just want to know if we ever get this message. Maybe it helps us to handle the
	// logoff/reboot/shutdown problem when eMule was started with "RUNAS".
	return Default();
}

LRESULT CemuleDlg::OnConsoleThreadEvent(WPARAM wParam, LPARAM lParam)
{
	AddDebugLogLine(DLP_VERYLOW, _T("%hs: nEvent=%u, nThreadID=%u"), __FUNCTION__, wParam, lParam);

	// If eMule was started with "RUNAS":
	// This message handler receives a 'console event' from the concurrently and thus
	// asynchronously running console control handler thread which was spawned by Windows
	// in case the user logs off/reboots/shutdown. Even if the console control handler thread
	// is waiting on the result from this message handler (is waiting until the main thread
	// has finished processing this inter-application message), the application will get
	// forcefully terminated by Windows after 20 seconds! There is no known way to prevent
	// that. This means, that if we would invoke our standard shutdown code ('OnClose') here
	// and the shutdown takes longer than 20 sec, we will get forcefully terminated by 
	// Windows, regardless of what we are doing. This means, MET-file and PART-file corruption
	// may occure. Because the shutdown code in 'OnClose' does also shutdown Kad (which takes
	// a noticeable amount of time) it is not that unlikely that we run into problems with
	// not being finished with our shutdown in 20 seconds.
	// 
	if (theApp.m_app_state == APP_STATE_RUNNING)
	{
#if 1
		// And it really should be OK to expect that emule can shutdown in 20 sec on almost
		// all computers. So, use the proper shutdown.
		theApp.m_app_state = APP_STATE_SHUTTINGDOWN;
		OnClose();	// do not invoke if shutdown takes longer than 20 sec, read above
#else
		// As a minimum action we at least set the 'shutting down' flag, this will help e.g.
		// the CUploadQueue::UploadTimer to not start any file save actions which could get
		// interrupted by windows and which would then lead to corrupted MET-files.
		// Setting this flag also helps any possible running threads to stop their work.
		theApp.m_app_state = APP_STATE_SHUTTINGDOWN;

#ifdef _DEBUG
		// Simulate some work.
		//
		// NOTE: If the console thread has already exited, Windows may terminate the process
		// even before the 20 sec. timeout!
		//BeBusy(70, __FUNCTION__);
#endif

		// Actually, just calling 'ExitProcess' should be the most safe thing which we can
		// do here. Because we received this message via the main message queue we are 
		// totally in-sync with the application and therefore we know that we are currently
		// not within a file save action and thus we simply can not cause any file corruption
		// when we exit right now.
		//
		// Of course, there may be some data loss. But it's the same amount of data loss which
		// could occure if we keep running. But if we keep running and wait until Windows
		// terminates us after 20 sec, there is also the chance for file corruption.
		if (thePrefs.GetDebug2Disk()) {
			theVerboseLog.Logf(_T("%hs: ExitProcess"), __FUNCTION__);
			theVerboseLog.Close();
		}
		ExitProcess(0);
#endif
	}

	AddDebugLogLine(DLP_VERYLOW, _T("%hs: returning"), __FUNCTION__);
	return 1;
}

void CemuleDlg::OnDestroy()
{
	AddDebugLogLine(DLP_VERYLOW, _T("%hs"), __FUNCTION__);

	// If eMule was started with "RUNAS":
	// When user is logging of (or reboots or shutdown system), Windows may or may not send 
	// a WM_DESTROY (depends on how long the application needed to process the 
	// CTRL_LOGOFF_EVENT). But, regardless of what happened and regardless of how long any
	// application specific shutdown took, Windows fill forcefully terminate the process 
	// after 1-2 seconds after WM_DESTROY! So, we can not use WM_DESTROY for any lengthy
	// shutdown actions in that case.
	CTrayDialog::OnDestroy();
}

bool CemuleDlg::CanClose()
{
	if (theApp.m_app_state == APP_STATE_RUNNING && thePrefs.IsConfirmExitEnabled())
	{
		if (IsIconic())
			ShowWindow(SW_SHOWNORMAL); // XMessageBox has a bug when displaying in minimized windows, but it makes sense to restore it for this prompt anyway
		int nResult = XMessageBox(GetSafeHwnd(), GetResString(IDS_MAIN_EXIT), GetResString(IDS_CLOSEEMULE), MB_YESNO | MB_DEFBUTTON2 | MB_DONOTASKAGAIN | MB_ICONQUESTION);
		if ((nResult & MB_DONOTASKAGAIN) > 0)
			thePrefs.SetConfirmExit(false);
		if ((nResult & 0xFF) == IDNO)
			return false;
	}
	return true;
}

void CemuleDlg::OnClose()
{
	if (!CanClose())
		return;

	Log(_T("Closing eMule"));
	CloseTTS();
	m_pDropTarget->Revoke();
	theApp.m_app_state = APP_STATE_SHUTTINGDOWN;
	theApp.serverconnect->Disconnect();
	theApp.OnlineSig(); // Added By Bouc7 

	// get main window placement
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	ASSERT( wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED || wp.showCmd == SW_SHOWNORMAL );
	if (wp.showCmd == SW_SHOWMINIMIZED && (wp.flags & WPF_RESTORETOMAXIMIZED))
		wp.showCmd = SW_SHOWMAXIMIZED;
	wp.flags = 0;
	thePrefs.SetWindowLayout(wp);

	// get active main window dialog
	if (activewnd){
		if (activewnd->IsKindOf(RUNTIME_CLASS(CServerWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_SERVER);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CSharedFilesWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_FILES);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CSearchDlg)))
			thePrefs.SetLastMainWndDlgID(IDD_SEARCH);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CChatWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_CHAT);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CTransferDlg)))
			thePrefs.SetLastMainWndDlgID(IDD_TRANSFER);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CStatisticsDlg)))
			thePrefs.SetLastMainWndDlgID(IDD_STATISTICS);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CKademliaWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_KADEMLIAWND);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CIrcWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_IRC);
		else{
			ASSERT(0);
			thePrefs.SetLastMainWndDlgID(0);
		}
	}

	Kademlia::CKademlia::Stop();	// couple of data files are written

	// try to wait untill the hashing thread notices that we are shutting down
	CSingleLock sLock1(&theApp.hashing_mut); // only one filehash at a time
	sLock1.Lock(2000);

	theApp.m_pUploadDiskIOThread->EndThread();

	// saving data & stuff
	theApp.emuledlg->preferenceswnd->m_wndSecurity.DeleteDDB();

	theApp.knownfiles->Save();										// CKnownFileList::Save
	theApp.sharedfiles->Save();
	//transferwnd->downloadlistctrl.SaveSettings();
	//transferwnd->downloadclientsctrl.SaveSettings();
	//transferwnd->uploadlistctrl.SaveSettings();
	//transferwnd->queuelistctrl.SaveSettings();
	//transferwnd->clientlistctrl.SaveSettings();
	//sharedfileswnd->sharedfilesctrl.SaveSettings();
	//chatwnd->m_FriendListCtrl.SaveSettings();
	searchwnd->SaveAllSettings();
	serverwnd->SaveAllSettings();
	kademliawnd->SaveAllSettings();

	theApp.m_pPeerCache->Save();
	theApp.scheduler->RestoreOriginals();
	theApp.searchlist->SaveSpamFilter();
	if (thePrefs.IsStoringSearchesEnabled())
		theApp.searchlist->StoreSearches();
	
	// close uPnP Ports
	theApp.m_pUPnPFinder->GetImplementation()->StopAsyncFind();
	if (thePrefs.CloseUPnPOnExit())
		theApp.m_pUPnPFinder->GetImplementation()->DeletePorts();

	thePrefs.Save();
	thePerfLog.Shutdown();

	// explicitly delete all listview items which may hold ptrs to objects which will get deleted
	// by the dtors (some lines below) to avoid potential problems during application shutdown.
	transferwnd->GetDownloadList()->DeleteAllItems();
	chatwnd->chatselector.DeleteAllItems();
	chatwnd->m_FriendListCtrl.DeleteAllItems();
	theApp.clientlist->DeleteAll();
	searchwnd->DeleteAllSearchListCtrlItems();
	sharedfileswnd->sharedfilesctrl.DeleteAllItems();
    transferwnd->GetQueueList()->DeleteAllItems();
	transferwnd->GetClientList()->DeleteAllItems();
	transferwnd->GetUploadList()->DeleteAllItems();
	transferwnd->GetDownloadClientsList()->DeleteAllItems();
	serverwnd->serverlistctrl.DeleteAllItems();

	CPartFileConvert::CloseGUI();
	CPartFileConvert::RemoveAllJobs();

    theApp.uploadBandwidthThrottler->EndThread();
    theApp.lastCommonRouteFinder->EndThread();

	theApp.sharedfiles->DeletePartFileInstances();

	searchwnd->SendMessage(WM_CLOSE);
	transferwnd->SendMessage(WM_CLOSE);

    // NOTE: Do not move those dtors into 'CemuleApp::InitInstance' (althought they should be there). The
	// dtors are indirectly calling functions which access several windows which would not be available 
	// after we have closed the main window -> crash!
	delete theApp.mmserver;			theApp.mmserver = NULL;
	delete theApp.listensocket;		theApp.listensocket = NULL;
	delete theApp.clientudp;		theApp.clientudp = NULL;
	delete theApp.sharedfiles;		theApp.sharedfiles = NULL;
	delete theApp.serverconnect;	theApp.serverconnect = NULL;
	delete theApp.serverlist;		theApp.serverlist = NULL;		// CServerList::SaveServermetToFile
	delete theApp.knownfiles;		theApp.knownfiles = NULL;
	delete theApp.searchlist;		theApp.searchlist = NULL;
	delete theApp.clientcredits;	theApp.clientcredits = NULL;	// CClientCreditsList::SaveList
	delete theApp.downloadqueue;	theApp.downloadqueue = NULL;	// N * (CPartFile::FlushBuffer + CPartFile::SavePartFile)
	delete theApp.uploadqueue;		theApp.uploadqueue = NULL;
	delete theApp.clientlist;		theApp.clientlist = NULL;
	delete theApp.friendlist;		theApp.friendlist = NULL;		// CFriendList::SaveList
	delete theApp.scheduler;		theApp.scheduler = NULL;
	delete theApp.ipfilter;			theApp.ipfilter = NULL;			// CIPFilter::SaveToDefaultFile
	delete theApp.webserver;		theApp.webserver = NULL;
	delete theApp.m_pPeerCache;		theApp.m_pPeerCache = NULL;
	delete theApp.m_pFirewallOpener;theApp.m_pFirewallOpener = NULL;
	delete theApp.uploadBandwidthThrottler;	theApp.uploadBandwidthThrottler = NULL;
	delete theApp.lastCommonRouteFinder;	theApp.lastCommonRouteFinder = NULL;
	delete theApp.m_pUPnPFinder;			theApp.m_pUPnPFinder = NULL;
	delete theApp.m_pUploadDiskIOThread;	theApp.m_pUploadDiskIOThread = NULL;

	thePrefs.Uninit();
	theApp.m_app_state = APP_STATE_DONE;
	CTrayDialog::OnCancel();
	AddDebugLogLine(DLP_VERYLOW, _T("Closed eMule"));
}

void CemuleDlg::DestroyMiniMule()
{
	if (m_pMiniMule)
	{
		if (m_pMiniMule->IsInInitDialog())
		{
			TRACE("%s - *** Cannot destroy Minimule, it's still in 'OnInitDialog'\n", __FUNCTION__);
			m_pMiniMule->SetDestroyAfterInitDialog();
		}
		else if (!m_pMiniMule->IsInCallback()) // for safety
		{
			TRACE("%s - m_pMiniMule->DestroyWindow();\n", __FUNCTION__);
			m_pMiniMule->DestroyWindow();
			ASSERT( m_pMiniMule == NULL );
			m_pMiniMule = NULL;
		}
		else
			ASSERT(0);
	}
}

LRESULT CemuleDlg::OnCloseMiniMule(WPARAM wParam, LPARAM /*lParam*/)
{
	TRACE("%s -> DestroyMiniMule();\n", __FUNCTION__);
	DestroyMiniMule();
	if (wParam)
		RestoreWindow();
	return 0;
}

void CemuleDlg::OnTrayLButtonUp(CPoint /*pt*/)
{
	if(!IsRunning())
		return;

	// Avoid reentrancy problems with main window, options dialog and mini mule window
	if (IsPreferencesDlgOpen()) {
		MessageBeep(MB_OK);
		preferenceswnd->SetForegroundWindow();
		preferenceswnd->BringWindowToTop();
		return;
	}

	if (m_pMiniMule) {
		if (!m_pMiniMule->IsInInitDialog()) {
			TRACE("%s - m_pMiniMule->ShowWindow(SW_SHOW);\n", __FUNCTION__);
			m_pMiniMule->ShowWindow(SW_SHOW);
			m_pMiniMule->SetForegroundWindow();
			m_pMiniMule->BringWindowToTop();
		}
		return;
	}

	if (thePrefs.GetEnableMiniMule())
	{
		try
		{
			TRACE("%s - m_pMiniMule = new CMiniMule(this);\n", __FUNCTION__);
			ASSERT( m_pMiniMule == NULL );
			m_pMiniMule = new CMiniMule(this);
			m_pMiniMule->Create(CMiniMule::IDD, this);
			if (m_pMiniMule->GetDestroyAfterInitDialog())
			{
				DestroyMiniMule();
			}
			else
			{
				//m_pMiniMule->ShowWindow(SW_SHOW);	// do not explicitly show the window, it will do that for itself when it's ready..
				m_pMiniMule->SetForegroundWindow();
				m_pMiniMule->BringWindowToTop();
			}
		}
		catch(...)
		{
			ASSERT(0);
			m_pMiniMule = NULL;
		}
	}
}

void CemuleDlg::OnTrayRButtonUp(CPoint pt)
{
	if (!IsRunning())
		return;

	// Avoid reentrancy problems with main window, options dialog and mini mule window
	if (IsPreferencesDlgOpen()) {
		MessageBeep(MB_OK);
		preferenceswnd->SetForegroundWindow();
		preferenceswnd->BringWindowToTop();
		return;
	}

	if (m_pMiniMule)
	{
		if (m_pMiniMule->GetAutoClose())
		{
			TRACE("%s - m_pMiniMule->GetAutoClose() -> DestroyMiniMule();\n", __FUNCTION__);
			DestroyMiniMule();
		}
		else
		{
			// Avoid reentrancy problems with main window, options dialog and mini mule window
			if (m_pMiniMule->m_hWnd && !m_pMiniMule->IsWindowEnabled()) {
				MessageBeep(MB_OK);
				return;
			}
		}
	}

	if (m_pSystrayDlg) {
		m_pSystrayDlg->BringWindowToTop();
		return;
	}

	m_pSystrayDlg = new CMuleSystrayDlg(this, pt,
										thePrefs.GetMaxGraphUploadRate(true), thePrefs.GetMaxGraphDownloadRate(),
										thePrefs.GetMaxUpload(), thePrefs.GetMaxDownload());
	if (m_pSystrayDlg)
	{
		UINT nResult = m_pSystrayDlg->DoModal();
		delete m_pSystrayDlg;
		m_pSystrayDlg = NULL;
		switch (nResult)
		{
			case IDC_TOMAX:
				QuickSpeedOther(MP_QS_UA);
				break;
			case IDC_TOMIN:
				QuickSpeedOther(MP_QS_PA);
				break;
			case IDC_RESTORE:
				RestoreWindow();
				break;
			case IDC_CONNECT:
				StartConnection();
				break;
			case IDC_DISCONNECT:
				CloseConnection();
				break;
			case IDC_EXIT:
				OnClose();
				break;
			case IDC_PREFERENCES:
				ShowPreferences();
				break;
		}
	}
}

void CemuleDlg::AddSpeedSelectorMenus(CMenu* addToMenu)
{
	CString text;

	// Create UploadPopup Menu
	ASSERT( m_menuUploadCtrl.m_hMenu == NULL );
	if (m_menuUploadCtrl.CreateMenu())
	{
		text.Format(_T("20%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate(true)*0.2),GetResString(IDS_KBYTESPERSEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U20,  text);
		text.Format(_T("40%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate(true)*0.4),GetResString(IDS_KBYTESPERSEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U40,  text);
		text.Format(_T("60%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate(true)*0.6),GetResString(IDS_KBYTESPERSEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U60,  text);
		text.Format(_T("80%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate(true)*0.8),GetResString(IDS_KBYTESPERSEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U80,  text);
		text.Format(_T("100%%\t%i %s"), (uint16)(thePrefs.GetMaxGraphUploadRate(true)),GetResString(IDS_KBYTESPERSEC));		m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U100, text);
		m_menuUploadCtrl.AppendMenu(MF_SEPARATOR);
		
		if (GetRecMaxUpload() > 0) {
			text.Format(GetResString(IDS_PW_MINREC) + GetResString(IDS_KBYTESPERSEC), GetRecMaxUpload());
			m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_UP10, text);
		}

		text.Format(_T("%s:"), GetResString(IDS_PW_UPL));
		addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_menuUploadCtrl.m_hMenu, text);
	}

	// Create DownloadPopup Menu
	ASSERT( m_menuDownloadCtrl.m_hMenu == NULL );
	if (m_menuDownloadCtrl.CreateMenu())
	{
		text.Format(_T("20%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.2),GetResString(IDS_KBYTESPERSEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D20,  text);
		text.Format(_T("40%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.4),GetResString(IDS_KBYTESPERSEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D40,  text);
		text.Format(_T("60%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.6),GetResString(IDS_KBYTESPERSEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D60,  text);
		text.Format(_T("80%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.8),GetResString(IDS_KBYTESPERSEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D80,  text);
		text.Format(_T("100%%\t%i %s"), (uint16)(thePrefs.GetMaxGraphDownloadRate()),GetResString(IDS_KBYTESPERSEC));		m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D100, text);

		text.Format(_T("%s:"), GetResString(IDS_PW_DOWNL));
		addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_menuDownloadCtrl.m_hMenu, text);
	}

	addToMenu->AppendMenu(MF_SEPARATOR);
	addToMenu->AppendMenu(MF_STRING, MP_CONNECT, GetResString(IDS_MAIN_BTN_CONNECT));
	addToMenu->AppendMenu(MF_STRING, MP_DISCONNECT, GetResString(IDS_MAIN_BTN_DISCONNECT)); 
}

void CemuleDlg::StartConnection()
{
	if (   (!theApp.serverconnect->IsConnecting() && !theApp.serverconnect->IsConnected())
		|| !Kademlia::CKademlia::IsRunning())
	{
		// UPnP is still trying to open the ports. In order to not get a LowID by connecting to the servers / kad before
		// the ports are opened we delay the connection untill UPnP gets a result or the timeout is reached
		// If the user clicks two times on the button, let him have his will and connect regardless
		if (m_hUPnPTimeOutTimer != 0 && !m_bConnectRequestDelayedForUPnP){
			AddLogLine(false, GetResString(IDS_DELAYEDBYUPNP));
			AddLogLine(true, GetResString(IDS_DELAYEDBYUPNP2));
			m_bConnectRequestDelayedForUPnP = true;
			return;
		}
		else{
			m_bConnectRequestDelayedForUPnP = false;
			if (m_hUPnPTimeOutTimer != 0){
				VERIFY( ::KillTimer(NULL, m_hUPnPTimeOutTimer) );
				m_hUPnPTimeOutTimer = 0;
			}
			AddLogLine(true, GetResString(IDS_CONNECTING));

			// ed2k
			if ((thePrefs.GetNetworkED2K() || m_bEd2kSuspendDisconnect) && !theApp.serverconnect->IsConnecting() && !theApp.serverconnect->IsConnected()) {
				theApp.serverconnect->ConnectToAnyServer();
			}

			// kad
			if ((thePrefs.GetNetworkKademlia() || m_bKadSuspendDisconnect) && !Kademlia::CKademlia::IsRunning()) {
				Kademlia::CKademlia::Start();
			}
		}

		ShowConnectionState();
	}
	m_bEd2kSuspendDisconnect = false;
	m_bKadSuspendDisconnect = false;
}

void CemuleDlg::CloseConnection()
{
	if (theApp.serverconnect->IsConnected()){
		theApp.serverconnect->Disconnect();
	}

	if (theApp.serverconnect->IsConnecting()){
		theApp.serverconnect->StopConnectionTry();
	}
	Kademlia::CKademlia::Stop();
	theApp.OnlineSig(); // Added By Bouc7 
	ShowConnectionState();
}

void CemuleDlg::RestoreWindow()
{
	if (IsPreferencesDlgOpen()) {
		MessageBeep(MB_OK);
		preferenceswnd->SetForegroundWindow();
		preferenceswnd->BringWindowToTop();
		return;
	}
	if (TrayIsVisible())
		TrayHide();

	DestroyMiniMule();

	if (m_wpFirstRestore.length)
	{
		SetWindowPlacement(&m_wpFirstRestore);
		memset(&m_wpFirstRestore, 0, sizeof m_wpFirstRestore);
		SetForegroundWindow();
		BringWindowToTop();
	}
	else
		CTrayDialog::RestoreWindow();
}

void CemuleDlg::UpdateTrayIcon(int iPercent)
{
	// compute an id of the icon to be generated
	UINT uSysTrayIconCookie = (iPercent > 0) ? (16 - ((iPercent*15/100) + 1)) : 0;
	if (theApp.IsConnected()) {
		if (!theApp.IsFirewalled())
			uSysTrayIconCookie += 50;
	}
	else
		uSysTrayIconCookie += 100;
	
	// dont update if the same icon as displayed would be generated
	if (m_uLastSysTrayIconCookie == uSysTrayIconCookie)
		return;
	m_uLastSysTrayIconCookie = uSysTrayIconCookie;

	// prepare it up
	if (m_iMsgIcon!=0 && thePrefs.DoFlashOnNewMessage()==true ) {
		m_iMsgBlinkState=!m_iMsgBlinkState;

		if (m_iMsgBlinkState)
			m_TrayIcon.Init(imicons[1], 100, 1, 1, 16, 16, thePrefs.GetStatsColor(11));
	} else m_iMsgBlinkState=false;

	if (!m_iMsgBlinkState) {
		if (theApp.IsConnected()) {
			if (theApp.IsFirewalled())
				m_TrayIcon.Init(m_icoSysTrayLowID, 100, 1, 1, 16, 16, thePrefs.GetStatsColor(11));
			else
				m_TrayIcon.Init(m_icoSysTrayConnected, 100, 1, 1, 16, 16, thePrefs.GetStatsColor(11));
		}
		else
			m_TrayIcon.Init(m_icoSysTrayDisconnected, 100, 1, 1, 16, 16, thePrefs.GetStatsColor(11));
	}

	// load our limit and color info
	static const int aiLimits[1] = { 100 }; // set the limits of where the bar color changes (low-high)
	COLORREF aColors[1] = { thePrefs.GetStatsColor(11) }; // set the corresponding color for each level
	m_TrayIcon.SetColorLevels(aiLimits, aColors, _countof(aiLimits));

	// generate the icon (do *not* destroy that icon using DestroyIcon(), that's done in 'TrayUpdate')
	int aiVals[1] = { iPercent };
	m_icoSysTrayCurrent = m_TrayIcon.Create(aiVals);
	ASSERT( m_icoSysTrayCurrent != NULL );
	if (m_icoSysTrayCurrent)
		TraySetIcon(m_icoSysTrayCurrent, true);
	TrayUpdate();
}

int CemuleDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CTrayDialog::OnCreate(lpCreateStruct);
}

void CemuleDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (IsRunning()){
		ShowTransferRate(true);

		if (bShow == TRUE && activewnd == chatwnd)
			chatwnd->chatselector.ShowChat();

	}
	CTrayDialog::OnShowWindow(bShow, nStatus);
}

void CemuleDlg::ShowNotifier(LPCTSTR pszText, int iMsgType, LPCTSTR pszLink, bool bForceSoundOFF)
{
	if (!notifierenabled)
		return;

	LPCTSTR pszSoundEvent = NULL;
	int iSoundPrio = 0;
	bool bShowIt = false;
	switch (iMsgType)
	{
		case TBN_CHAT:
            if (thePrefs.GetNotifierOnChat())
			{
				m_wndTaskbarNotifier->Show(pszText, iMsgType, pszLink);
				bShowIt = true;
				pszSoundEvent = _T("eMule_Chat");
				iSoundPrio = 1;
			}
			break;
		case TBN_DOWNLOADFINISHED:
            if (thePrefs.GetNotifierOnDownloadFinished())
			{
				m_wndTaskbarNotifier->Show(pszText, iMsgType, pszLink);
				bShowIt = true;
				pszSoundEvent = _T("eMule_DownloadFinished");
				iSoundPrio = 1;
				SendNotificationMail(iMsgType, pszText);
			}
			break;
		case TBN_DOWNLOADADDED:
            if (thePrefs.GetNotifierOnNewDownload())
			{
				m_wndTaskbarNotifier->Show(pszText, iMsgType, pszLink);
				bShowIt = true;
				pszSoundEvent = _T("eMule_DownloadAdded");
				iSoundPrio = 1;
			}
			break;
		case TBN_LOG:
            if (thePrefs.GetNotifierOnLog())
			{
				m_wndTaskbarNotifier->Show(pszText, iMsgType, pszLink);
				bShowIt = true;
				pszSoundEvent = _T("eMule_LogEntryAdded");
			}
			break;
		case TBN_IMPORTANTEVENT:
			if (thePrefs.GetNotifierOnImportantError())
			{
				m_wndTaskbarNotifier->Show(pszText, iMsgType, pszLink);
				bShowIt = true;
				pszSoundEvent = _T("eMule_Urgent");
				iSoundPrio = 1;
				SendNotificationMail(iMsgType, pszText);
			}
			break;

		case TBN_NEWVERSION:
			if (thePrefs.GetNotifierOnNewVersion())
			{
				m_wndTaskbarNotifier->Show(pszText, iMsgType, pszLink);
				bShowIt = true;
				pszSoundEvent = _T("eMule_NewVersion");
				iSoundPrio = 1;
			}
			break;
		case TBN_NULL:
            m_wndTaskbarNotifier->Show(pszText, iMsgType, pszLink);
			bShowIt = true;
			break;
	}
	
	if (bShowIt && !bForceSoundOFF && thePrefs.GetNotifierSoundType() != ntfstNoSound)
	{
		bool bNotifiedWithAudio = false;
		if (thePrefs.GetNotifierSoundType() == ntfstSpeech)
			bNotifiedWithAudio = Speak(pszText);

		if (!bNotifiedWithAudio)
		{
			if (!thePrefs.GetNotifierSoundFile().IsEmpty())
			{
				PlaySound(thePrefs.GetNotifierSoundFile(), NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
			}
			else if (pszSoundEvent)
			{
				// use 'SND_NOSTOP' only for low priority events, otherwise the 'Log message' event may overrule a more important
				// event which is fired nearly at the same time.
				PlaySound(pszSoundEvent, NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | ((iSoundPrio > 0) ? 0 : SND_NOSTOP));
			}
		}
	}
}

void CemuleDlg::LoadNotifier(CString configuration)
{
	notifierenabled = m_wndTaskbarNotifier->LoadConfiguration(configuration)!=FALSE;
}

LRESULT CemuleDlg::OnTaskbarNotifierClicked(WPARAM /*wParam*/, LPARAM lParam)
{
	if (lParam)
	{
		LPTSTR pszLink = (LPTSTR)lParam;
		ShellOpenFile(pszLink, NULL);
		free(pszLink);
		pszLink = NULL;
	}

	switch (m_wndTaskbarNotifier->GetMessageType())
	{
		case TBN_CHAT:
			RestoreWindow();
			SetActiveDialog(chatwnd);
			break;

		case TBN_DOWNLOADFINISHED:
			// if we had a link and opened the downloaded file, dont restore the app window
			if (lParam==0)
			{
				RestoreWindow();
				SetActiveDialog(transferwnd);
			}
			break;

		case TBN_DOWNLOADADDED:
			RestoreWindow();
			SetActiveDialog(transferwnd);
			break;

		case TBN_IMPORTANTEVENT:
			RestoreWindow();
			SetActiveDialog(serverwnd);	
			break;

		case TBN_LOG:
			RestoreWindow();
			SetActiveDialog(serverwnd);	
			break;

		case TBN_NEWVERSION:
		{
			CString theUrl;
			theUrl.Format(_T("/en/version_check.php?version=%i&language=%i"),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
			theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
			ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
			break;
		}
	}
    return 0;
}

void CemuleDlg::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	TRACE(_T("CemuleDlg::OnSettingChange: uFlags=0x%08x  lpszSection=\"%s\"\n"), lpszSection);
	// Do not update the Shell's large icon size, because we still have an image list
	// from the shell which contains the old large icon size.
	//theApp.UpdateLargeIconSize();
	theApp.UpdateDesktopColorDepth();
	CTrayDialog::OnSettingChange(uFlags, lpszSection);
}

void CemuleDlg::OnSysColorChange()
{
	theApp.UpdateDesktopColorDepth();
	CTrayDialog::OnSysColorChange();
	SetAllIcons();
}

HBRUSH CemuleDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = GetCtlColor(pDC, pWnd, nCtlColor);
	if (hbr)
		return hbr;
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

HBRUSH CemuleDlg::GetCtlColor(CDC* /*pDC*/, CWnd* /*pWnd*/, UINT nCtlColor)
{
	UNREFERENCED_PARAMETER(nCtlColor);
	// This function could be used to give the entire eMule (at least all of the main windows)
	// a somewhat more Vista like look by giving them all a bright background color.
	// However, again, the ownerdrawn tab controls are noticeable disturbing that attempt. They
	// do not change their background color accordingly. They don't use NMCUSTOMDRAW nor to they
	// use WM_CTLCOLOR...
	//
	//if (theApp.m_ullComCtrlVer >= MAKEDLLVERULL(6,16,0,0) && g_xpStyle.IsThemeActive() && g_xpStyle.IsAppThemed()) {
	//	if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC)
	//		return GetSysColorBrush(COLOR_WINDOW);
	//}
	return NULL;
}

void CemuleDlg::SetAllIcons()
{
	// application icon (although it's not customizable, we may need to load a different color resolution)
	if (m_hIcon)
		VERIFY( ::DestroyIcon(m_hIcon) );
	// NOTE: the application icon name is prefixed with "AAA" to make sure it's alphabetically sorted by the
	// resource compiler as the 1st icon in the resource table!
	m_hIcon = AfxGetApp()->LoadIcon(_T("AAAEMULEAPP"));
	SetIcon(m_hIcon, TRUE);
	// this scales the 32x32 icon down to 16x16, does not look nice at least under WinXP
	//SetIcon(m_hIcon, FALSE);

	// connection state
	for (int i = 0; i < _countof(connicons); i++){
		if (connicons[i]) VERIFY( ::DestroyIcon(connicons[i]) );
	}
	connicons[0] = theApp.LoadIcon(_T("ConnectedNotNot"), 16, 16);
	connicons[1] = theApp.LoadIcon(_T("ConnectedNotLow"), 16, 16);
	connicons[2] = theApp.LoadIcon(_T("ConnectedNotHigh"), 16, 16);
	connicons[3] = theApp.LoadIcon(_T("ConnectedLowNot"), 16, 16);
	connicons[4] = theApp.LoadIcon(_T("ConnectedLowLow"), 16, 16);
	connicons[5] = theApp.LoadIcon(_T("ConnectedLowHigh"), 16, 16);
	connicons[6] = theApp.LoadIcon(_T("ConnectedHighNot"), 16, 16);
	connicons[7] = theApp.LoadIcon(_T("ConnectedHighLow"), 16, 16);
	connicons[8] = theApp.LoadIcon(_T("ConnectedHighHigh"), 16, 16);
	ShowConnectionStateIcon();

	// transfer state
	if (transicons[0]) VERIFY( ::DestroyIcon(transicons[0]) );
	if (transicons[1]) VERIFY( ::DestroyIcon(transicons[1]) );
	if (transicons[2]) VERIFY( ::DestroyIcon(transicons[2]) );
	if (transicons[3]) VERIFY( ::DestroyIcon(transicons[3]) );
	transicons[0] = theApp.LoadIcon(_T("UP0DOWN0"), 16, 16);
	transicons[1] = theApp.LoadIcon(_T("UP0DOWN1"), 16, 16);
	transicons[2] = theApp.LoadIcon(_T("UP1DOWN0"), 16, 16);
	transicons[3] = theApp.LoadIcon(_T("UP1DOWN1"), 16, 16);
	ShowTransferStateIcon();

	// users state
	if (usericon) VERIFY( ::DestroyIcon(usericon) );
	usericon = theApp.LoadIcon(_T("StatsClients"), 16, 16);
	ShowUserStateIcon();

	// traybar icons
	if (m_icoSysTrayConnected) VERIFY( ::DestroyIcon(m_icoSysTrayConnected) );
	if (m_icoSysTrayDisconnected) VERIFY( ::DestroyIcon(m_icoSysTrayDisconnected) );
	if (m_icoSysTrayLowID) VERIFY( ::DestroyIcon(m_icoSysTrayLowID) );
	m_icoSysTrayConnected = theApp.LoadIcon(_T("TrayConnected"), 16, 16);
	m_icoSysTrayDisconnected = theApp.LoadIcon(_T("TrayNotConnected"), 16, 16);
	m_icoSysTrayLowID = theApp.LoadIcon(_T("TrayLowID"), 16, 16);
	ShowTransferRate(true);

	if (imicons[0]) VERIFY( ::DestroyIcon(imicons[0]) );
	if (imicons[1]) VERIFY( ::DestroyIcon(imicons[1]) );
	if (imicons[2]) VERIFY( ::DestroyIcon(imicons[2]) );
	imicons[0] = NULL;
	imicons[1] = theApp.LoadIcon(_T("Message"), 16, 16);
	imicons[2] = theApp.LoadIcon(_T("MessagePending"), 16, 16);
	ShowMessageState(m_iMsgIcon);
}

void CemuleDlg::Localize()
{
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu)
	{
		VERIFY( pSysMenu->ModifyMenu(MP_ABOUTBOX, MF_BYCOMMAND | MF_STRING, MP_ABOUTBOX, GetResString(IDS_ABOUTBOX)) );
		VERIFY( pSysMenu->ModifyMenu(MP_VERSIONCHECK, MF_BYCOMMAND | MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK)) );

		switch (thePrefs.GetWindowsVersion())
		{
			case _WINVER_98_:
			case _WINVER_95_:
			case _WINVER_ME_:
				// NOTE: I think the reason why the old version of the following code crashed under Win9X was because
				// of the menus were destroyed right after they were added to the system menu. New code should work
				// under Win9X too but I can't test it.
				break;
			default:{
				// localize the 'speed control' sub menus by deleting the current menus and creating a new ones.

				// remove any already available 'speed control' menus from system menu
				UINT uOptMenuPos = pSysMenu->GetMenuItemCount() - 1;
				CMenu* pAccelMenu = pSysMenu->GetSubMenu(uOptMenuPos);
				if (pAccelMenu)
				{
					ASSERT( pAccelMenu->m_hMenu == m_SysMenuOptions.m_hMenu );
					VERIFY( pSysMenu->RemoveMenu(uOptMenuPos, MF_BYPOSITION) );
					pAccelMenu = NULL;
				}

				// destroy all 'speed control' menus
				if (m_menuUploadCtrl)
					VERIFY( m_menuUploadCtrl.DestroyMenu() );
				if (m_menuDownloadCtrl)
					VERIFY( m_menuDownloadCtrl.DestroyMenu() );
				if (m_SysMenuOptions)
					VERIFY( m_SysMenuOptions.DestroyMenu() );

				// create new 'speed control' menus
				if (m_SysMenuOptions.CreateMenu())
				{
					AddSpeedSelectorMenus(&m_SysMenuOptions);
					pSysMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_SysMenuOptions.m_hMenu, GetResString(IDS_EM_PREFS));
				}
			}
		}
	}

	ShowUserStateIcon();
	toolbar->Localize();
	ShowConnectionState();
	ShowTransferRate(true);
	ShowUserCount();
	CPartFileConvert::Localize();
	if (m_pMiniMule && !m_pMiniMule->IsInInitDialog())
		m_pMiniMule->Localize();
}

void CemuleDlg::ShowUserStateIcon()
{
	statusbar->SetIcon(SBarUsers, usericon);
}

void CemuleDlg::QuickSpeedOther(UINT nID)
{
	switch (nID) {
		case MP_QS_PA: 
			thePrefs.SetMaxUpload(1);
			thePrefs.SetMaxDownload(1);
			break ;
		case MP_QS_UA: 
			thePrefs.SetMaxUpload(thePrefs.GetMaxGraphUploadRate(true));
			thePrefs.SetMaxDownload(thePrefs.GetMaxGraphDownloadRate());
			break ;
	}
}


void CemuleDlg::QuickSpeedUpload(UINT nID)
{
	switch (nID) {
		case MP_QS_U10: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.1)); break ;
		case MP_QS_U20: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.2)); break ;
		case MP_QS_U30: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.3)); break ;
		case MP_QS_U40: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.4)); break ;
		case MP_QS_U50: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.5)); break ;
		case MP_QS_U60: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.6)); break ;
		case MP_QS_U70: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.7)); break ;
		case MP_QS_U80: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.8)); break ;
		case MP_QS_U90: thePrefs.SetMaxUpload((UINT)(thePrefs.GetMaxGraphUploadRate(true)*0.9)); break ;
		case MP_QS_U100: thePrefs.SetMaxUpload((UINT)thePrefs.GetMaxGraphUploadRate(true)); break ;
//		case MP_QS_UPC: thePrefs.SetMaxUpload(UNLIMITED); break ;
		case MP_QS_UP10: thePrefs.SetMaxUpload(GetRecMaxUpload()); break ;
	}
}

void CemuleDlg::QuickSpeedDownload(UINT nID)
{
	switch (nID) {
		case MP_QS_D10: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.1)); break ;
		case MP_QS_D20: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.2)); break ;
		case MP_QS_D30: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.3)); break ;
		case MP_QS_D40: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.4)); break ;
		case MP_QS_D50: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.5)); break ;
		case MP_QS_D60: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.6)); break ;
		case MP_QS_D70: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.7)); break ;
		case MP_QS_D80: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.8)); break ;
		case MP_QS_D90: thePrefs.SetMaxDownload((UINT)(thePrefs.GetMaxGraphDownloadRate()*0.9)); break ;
		case MP_QS_D100: thePrefs.SetMaxDownload((UINT)thePrefs.GetMaxGraphDownloadRate()); break ;
//		case MP_QS_DC: thePrefs.SetMaxDownload(UNLIMITED); break ;
	}
}
// quick-speed changer -- based on xrmb

int CemuleDlg::GetRecMaxUpload() {
	
	if (thePrefs.GetMaxGraphUploadRate(true)<7) return 0;
	if (thePrefs.GetMaxGraphUploadRate(true)<15) return thePrefs.GetMaxGraphUploadRate(true)-3;
	return (thePrefs.GetMaxGraphUploadRate(true)-4);

}

BOOL CemuleDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{	
		case TBBTN_CONNECT:
			OnBnClickedConnect();
			break;
		case MP_HM_KAD:
		case TBBTN_KAD:
			SetActiveDialog(kademliawnd);
			break;
		case TBBTN_SERVER:
		case MP_HM_SRVR:
			SetActiveDialog(serverwnd);
			break;
		case TBBTN_TRANSFERS:
		case MP_HM_TRANSFER:
			SetActiveDialog(transferwnd);
			break;
		case TBBTN_SEARCH:
		case MP_HM_SEARCH:
			SetActiveDialog(searchwnd);
			break;
		case TBBTN_SHARED:
		case MP_HM_FILES:
			SetActiveDialog(sharedfileswnd);
			break;
		case TBBTN_MESSAGES:
		case MP_HM_MSGS:
			SetActiveDialog(chatwnd);
			break;
		case TBBTN_IRC:
		case MP_HM_IRC:
			SetActiveDialog(ircwnd);
			break;
		case TBBTN_STATS:
		case MP_HM_STATS:
			SetActiveDialog(statisticswnd);
			break;
		case TBBTN_OPTIONS:
		case MP_HM_PREFS:
			toolbar->CheckButton(TBBTN_OPTIONS, TRUE);
			ShowPreferences();
			toolbar->CheckButton(TBBTN_OPTIONS, FALSE);
			break;
		case TBBTN_TOOLS:
			ShowToolPopup(true);
			break;
		case MP_HM_OPENINC:
			ShellExecute(NULL, _T("open"), thePrefs.GetMuleDirectory(EMULE_INCOMINGDIR),NULL, NULL, SW_SHOW); 
			break;
		case MP_HM_HELP:
		case TBBTN_HELP:
			if (activewnd != NULL) {
				HELPINFO hi;
				ZeroMemory(&hi, sizeof(HELPINFO));
				hi.cbSize = sizeof(HELPINFO);
				activewnd->SendMessage(WM_HELP, 0, (LPARAM)&hi);
			}
			else
				wParam = ID_HELP;
			break;
		case MP_HM_CON:
			OnBnClickedConnect();
			break;
		case MP_HM_EXIT:
			OnClose();
			break;
		case MP_HM_LINK1: // MOD: dont remove!
			ShellExecute(NULL, NULL, thePrefs.GetHomepageBaseURL(), NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK2:
			ShellExecute(NULL, NULL, thePrefs.GetHomepageBaseURL()+ CString(_T("/faq/")), NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK3: {
			CString theUrl;
			theUrl.Format( thePrefs.GetVersionCheckBaseURL() + CString(_T("/en/version_check.php?version=%i&language=%i")),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
			ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
			break;
		}
		case MP_WEBSVC_EDIT:
			theWebServices.Edit();
			break;
		case MP_HM_CONVERTPF:
			CPartFileConvert::ShowGUI();
			break;
		case MP_HM_SCHEDONOFF:
			thePrefs.SetSchedulerEnabled(!thePrefs.IsSchedulerEnabled());
			theApp.scheduler->Check(true);
			break;
		case MP_HM_1STSWIZARD:
			FirstTimeWizard();
			break;
		case MP_HM_IPFILTER:{
			CIPFilterDlg dlg;
			dlg.DoModal();
			break;
		}
		case MP_HM_DIRECT_DOWNLOAD:{
			CDirectDownloadDlg dlg;
			dlg.DoModal();
			break;
		}
	}	
	if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+99) {
		theWebServices.RunURL(NULL, wParam);
	}
	else if (wParam>=MP_SCHACTIONS && wParam<=MP_SCHACTIONS+99) {
		theApp.scheduler->ActivateSchedule(wParam-MP_SCHACTIONS);
		theApp.scheduler->SaveOriginals(); // use the new settings as original
	}
#ifdef HAVE_WIN7_SDK_H
	else if (HIWORD(wParam) == THBN_CLICKED) {
		OnTBBPressed(LOWORD(wParam));
		return TRUE;
	}
#endif

	return CTrayDialog::OnCommand(wParam, lParam);
}

LRESULT CemuleDlg::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
	UINT nCmdID;
	if (toolbar->MapAccelerator((TCHAR)nChar, &nCmdID)){
		OnCommand(nCmdID, 0);
		return MAKELONG(0,MNC_CLOSE);
	}
	return CTrayDialog::OnMenuChar(nChar, nFlags, pMenu);
}

// Barry - To find out if app is running or shutting/shut down
bool CemuleDlg::IsRunning()
{
	return (theApp.m_app_state == APP_STATE_RUNNING);
}


void CemuleDlg::OnBnClickedHotmenu()
{
	ShowToolPopup(false);
}

void CemuleDlg::ShowToolPopup(bool toolsonly)
{
	POINT point;
	::GetCursorPos(&point);

	CTitleMenu menu;
	menu.CreatePopupMenu();
	if (!toolsonly)
		menu.AddMenuTitle(GetResString(IDS_HOTMENU), true);
	else
		menu.AddMenuTitle(GetResString(IDS_TOOLS), true);

	CTitleMenu Links;
	Links.CreateMenu();
	Links.AddMenuTitle(NULL, true);
	Links.AppendMenu(MF_STRING, MP_HM_LINK1, GetResString(IDS_HM_LINKHP), _T("WEB"));
	Links.AppendMenu(MF_STRING, MP_HM_LINK2, GetResString(IDS_HM_LINKFAQ), _T("WEB"));
	Links.AppendMenu(MF_STRING, MP_HM_LINK3, GetResString(IDS_HM_LINKVC), _T("WEB"));
	theWebServices.GetGeneralMenuEntries(&Links);
	Links.InsertMenu(3, MF_BYPOSITION | MF_SEPARATOR);
	Links.AppendMenu(MF_STRING, MP_WEBSVC_EDIT, GetResString(IDS_WEBSVEDIT));

	CMenu scheduler;
	scheduler.CreateMenu();
	CString schedonoff= (!thePrefs.IsSchedulerEnabled())?GetResString(IDS_HM_SCHED_ON):GetResString(IDS_HM_SCHED_OFF);

	scheduler.AppendMenu(MF_STRING,MP_HM_SCHEDONOFF, schedonoff);
	if (theApp.scheduler->GetCount()>0) {
		scheduler.AppendMenu(MF_SEPARATOR);
		for (UINT i=0; i<theApp.scheduler->GetCount();i++)
			scheduler.AppendMenu(MF_STRING,MP_SCHACTIONS+i, theApp.scheduler->GetSchedule(i)->title);
	}

	if (!toolsonly) {
		if (theApp.serverconnect->IsConnected())
			menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_DISCONNECT), _T("DISCONNECT"));
		else if (theApp.serverconnect->IsConnecting())
			menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_CANCEL), _T("STOPCONNECTING"));
		else
			menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_CONNECT), _T("CONNECT"));

		menu.AppendMenu(MF_STRING,MP_HM_KAD, GetResString(IDS_EM_KADEMLIA), _T("KADEMLIA") );
		menu.AppendMenu(MF_STRING,MP_HM_SRVR, GetResString(IDS_EM_SERVER), _T("SERVER") );
		menu.AppendMenu(MF_STRING,MP_HM_TRANSFER, GetResString(IDS_EM_TRANS),_T("TRANSFER") );
		menu.AppendMenu(MF_STRING,MP_HM_SEARCH, GetResString(IDS_EM_SEARCH), _T("SEARCH"));
		menu.AppendMenu(MF_STRING,MP_HM_FILES, GetResString(IDS_EM_FILES), _T("SharedFiles"));
		menu.AppendMenu(MF_STRING,MP_HM_MSGS, GetResString(IDS_EM_MESSAGES), _T("MESSAGES"));
		menu.AppendMenu(MF_STRING,MP_HM_IRC, GetResString(IDS_IRC), _T("IRC"));
		menu.AppendMenu(MF_STRING,MP_HM_STATS, GetResString(IDS_EM_STATISTIC), _T("STATISTICS"));
		menu.AppendMenu(MF_STRING,MP_HM_PREFS, GetResString(IDS_EM_PREFS), _T("PREFERENCES"));
		menu.AppendMenu(MF_STRING,MP_HM_HELP, GetResString(IDS_EM_HELP), _T("HELP"));
		menu.AppendMenu(MF_SEPARATOR);
	}

	menu.AppendMenu(MF_STRING,MP_HM_OPENINC, GetResString(IDS_OPENINC) + _T("..."), _T("INCOMING"));
	menu.AppendMenu(MF_STRING,MP_HM_CONVERTPF, GetResString(IDS_IMPORTSPLPF) + _T("..."), _T("CONVERT"));
	menu.AppendMenu(MF_STRING,MP_HM_1STSWIZARD, GetResString(IDS_WIZ1) + _T("..."), _T("WIZARD"));
	menu.AppendMenu(MF_STRING,MP_HM_IPFILTER, GetResString(IDS_IPFILTER) + _T("..."), _T("IPFILTER"));
	menu.AppendMenu(MF_STRING,MP_HM_DIRECT_DOWNLOAD, GetResString(IDS_SW_DIRECTDOWNLOAD) + _T("..."), _T("PASTELINK"));

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)Links.m_hMenu, GetResString(IDS_LINKS), _T("WEB") );
	menu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)scheduler.m_hMenu, GetResString(IDS_SCHEDULER), _T("SCHEDULER") );

	if (!toolsonly) {
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING,MP_HM_EXIT, GetResString(IDS_EXIT), _T("EXIT"));
	}
	menu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( Links.DestroyMenu() );
	VERIFY( scheduler.DestroyMenu() );
	VERIFY( menu.DestroyMenu() );
}


void CemuleDlg::ApplyHyperTextFont(LPLOGFONT plf)
{
	theApp.m_fontHyperText.DeleteObject();
	if (theApp.m_fontHyperText.CreateFontIndirect(plf))
	{
		thePrefs.SetHyperTextFont(plf);
		serverwnd->servermsgbox->SetFont(&theApp.m_fontHyperText);
		chatwnd->chatselector.UpdateFonts(&theApp.m_fontHyperText);
		ircwnd->UpdateFonts(&theApp.m_fontHyperText);
	}
}

void CemuleDlg::ApplyLogFont(LPLOGFONT plf)
{
	theApp.m_fontLog.DeleteObject();
	if (theApp.m_fontLog.CreateFontIndirect(plf))
	{
		thePrefs.SetLogFont(plf);
		serverwnd->logbox->SetFont(&theApp.m_fontLog);
		serverwnd->debuglog->SetFont(&theApp.m_fontLog);
	}
}

LRESULT CemuleDlg::OnFrameGrabFinished(WPARAM wParam,LPARAM lParam){
	CKnownFile* pOwner = (CKnownFile*)wParam;
	FrameGrabResult_Struct* result = (FrameGrabResult_Struct*)lParam;
	
	if (theApp.knownfiles->IsKnownFile(pOwner) || theApp.downloadqueue->IsPartFile(pOwner) ){
		pOwner->GrabbingFinished(result->imgResults,result->nImagesGrabbed, result->pSender);
	}
	else{
		ASSERT ( false );
	}

	delete result;
	return 0;
}

void StraightWindowStyles(CWnd* pWnd)
{
	CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		StraightWindowStyles(pWndChild);
		pWndChild = pWndChild->GetNextWindow();
	}

	CHAR szClassName[MAX_PATH];
	if (::GetClassNameA(*pWnd, szClassName, _countof(szClassName)))
	{
		if (__ascii_stricmp(szClassName, "Button") == 0)
			pWnd->ModifyStyle(BS_FLAT, 0);
		else if (   (__ascii_stricmp(szClassName, "EDIT") == 0 && (pWnd->GetExStyle() & WS_EX_STATICEDGE))
			     || __ascii_stricmp(szClassName, "SysListView32") == 0
			     || __ascii_stricmp(szClassName, "msctls_trackbar32") == 0
			    )
		{
			pWnd->ModifyStyleEx(WS_EX_STATICEDGE, WS_EX_CLIENTEDGE);
		}
		//else if (__ascii_stricmp(szClassName, "SysTreeView32") == 0)
		//{
		//	pWnd->ModifyStyleEx(WS_EX_STATICEDGE, WS_EX_CLIENTEDGE);
		//}
	}
}

void ApplySystemFont(CWnd* pWnd)
{
	CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		ApplySystemFont(pWndChild);
		pWndChild = pWndChild->GetNextWindow();
	}

	CHAR szClassName[MAX_PATH];
	if (::GetClassNameA(*pWnd, szClassName, _countof(szClassName)))
	{
		if (   __ascii_stricmp(szClassName, "SysListView32") == 0
			|| __ascii_stricmp(szClassName, "SysTreeView32") == 0)
		{
			pWnd->SendMessage(WM_SETFONT, NULL, FALSE);
		}
	}
}

static bool s_bIsXPStyle;

void FlatWindowStyles(CWnd* pWnd)
{
	CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		FlatWindowStyles(pWndChild);
		pWndChild = pWndChild->GetNextWindow();
	}

	CHAR szClassName[MAX_PATH];
	if (::GetClassNameA(*pWnd, szClassName, _countof(szClassName)))
	{
		if (__ascii_stricmp(szClassName, "Button") == 0)
		{
			if (!s_bIsXPStyle || (pWnd->GetStyle() & BS_ICON) == 0)
				pWnd->ModifyStyle(0, BS_FLAT);
		}
		else if (__ascii_stricmp(szClassName, "SysListView32") == 0)
		{
			pWnd->ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE);
		}
		else if (__ascii_stricmp(szClassName, "SysTreeView32") == 0)
		{
			pWnd->ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE);
		}
	}
}

void InitWindowStyles(CWnd* pWnd)
{
	//ApplySystemFont(pWnd);
	if (thePrefs.GetStraightWindowStyles() < 0)
		return;
	else if (thePrefs.GetStraightWindowStyles() > 0)
		/*StraightWindowStyles(pWnd)*/;	// no longer needed
	else
	{
		s_bIsXPStyle = g_xpStyle.IsAppThemed() && g_xpStyle.IsThemeActive();
		if (!s_bIsXPStyle)
			FlatWindowStyles(pWnd);
	}
}

LRESULT CemuleDlg::OnVersionCheckResponse(WPARAM /*wParam*/, LPARAM lParam)
{
	if (WSAGETASYNCERROR(lParam) == 0)
	{
		int iBufLen = WSAGETASYNCBUFLEN(lParam);
		if (iBufLen >= sizeof(HOSTENT))
		{
			LPHOSTENT pHost = (LPHOSTENT)m_acVCDNSBuffer;
			if (pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0])
			{
				uint32 dwResult = ((LPIN_ADDR)(pHost->h_addr_list[0]))->s_addr;		
				// last byte contains informations about mirror urls, to avoid effects of future DDoS Attacks against eMules Homepage
				thePrefs.SetWebMirrorAlertLevel((uint8)(dwResult >> 24));
				uint8 abyCurVer[4] = { (uint8)(CemuleApp::m_nVersionBld + 1), (uint8)(CemuleApp::m_nVersionUpd), (uint8)(CemuleApp::m_nVersionMin), (uint8)0};
				dwResult &= 0x00FFFFFF;
				if (dwResult > *(uint32*)abyCurVer){
					SetActiveWindow();
#ifndef _BETA
					Log(LOG_SUCCESS|LOG_STATUSBAR,GetResString(IDS_NEWVERSIONAVL));
					ShowNotifier(GetResString(IDS_NEWVERSIONAVLPOPUP), TBN_NEWVERSION);
					thePrefs.UpdateLastVC();
					if (!thePrefs.GetNotifierOnNewVersion()){
						if (AfxMessageBox(GetResString(IDS_NEWVERSIONAVL)+GetResString(IDS_VISITVERSIONCHECK),MB_YESNO)==IDYES) {
							CString theUrl;
							theUrl.Format(_T("/en/version_check.php?version=%i&language=%i"),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
							theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
							ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
						}
					}
#else
					Log(LOG_SUCCESS|LOG_STATUSBAR,GetResString(IDS_NEWVERSIONAVLBETA));
					if (AfxMessageBox(GetResString(IDS_NEWVERSIONAVLBETA)+GetResString(IDS_VISITVERSIONCHECK),MB_OK)==IDOK) {
						CString theUrl;
						theUrl = _T("/beta");
						theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
						ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
					}
#endif
				}
				else{
					thePrefs.UpdateLastVC();
					AddLogLine(true,GetResString(IDS_NONEWERVERSION));
				}
				return 0;
			}
		}
	}
	LogWarning(LOG_STATUSBAR,GetResString(IDS_NEWVERSIONFAILED));
	return 0;
}

void CemuleDlg::ShowSplash()
{
	ASSERT( m_pSplashWnd == NULL );
	if (m_pSplashWnd == NULL)
	{
		m_pSplashWnd = new CSplashScreen;
		if (m_pSplashWnd != NULL)
		{
			ASSERT(m_hWnd);
			if (m_pSplashWnd->Create(CSplashScreen::IDD, this))
			{
				m_pSplashWnd->ShowWindow(SW_SHOW);
				m_pSplashWnd->UpdateWindow();
				m_dwSplashTime = ::GetCurrentTime();
			}
			else
			{
				delete m_pSplashWnd;
				m_pSplashWnd = NULL;
			}
		}
	}
}

void CemuleDlg::DestroySplash()
{
	if (m_pSplashWnd != NULL)
	{
		m_pSplashWnd->DestroyWindow();
		delete m_pSplashWnd;
		m_pSplashWnd = NULL;
	}
#ifdef _BETA
	// only do it once to not be annyoing given that the beta phases are expected to last longer these days
	if (!thePrefs.IsFirstStart() && thePrefs.ShouldBetaNag())
	{
		thePrefs.SetDidBetaNagging();
		AfxMessageBox(GetResString(IDS_BETANAG), MB_ICONINFORMATION | MB_OK, 0);
	}
#endif
}

BOOL CemuleApp::IsIdleMessage(MSG *pMsg)
{
	// This function is closely related to 'CemuleDlg::OnKickIdle'.
	//
	// * See MFC source code for 'CWnd::RunModalLoop' to see how those functions are related 
	//	 to each other.
	//
	// * See MFC documentation for 'CWnd::IsIdleMessage' to see why WM_TIMER messages are
	//	 filtered here.
	//
	// Generally we want to filter WM_TIMER messages because they are triggering idle
	// processing (e.g. cleaning up temp. MFC maps) and because they are occuring very often
	// in eMule (we have a rather high frequency timer in upload queue). To save CPU load but
	// do not miss the chance to cleanup MFC temp. maps and other stuff, we do not use each
	// occuring WM_TIMER message -- that would just be overkill! However, we can not simply
	// filter all WM_TIMER messages. If eMule is running in taskbar the only messages which
	// are received by main window are those WM_TIMER messages, thus those messages are the
	// only chance to trigger some idle processing. So, we must use at last some of those
	// messages because otherwise we would not do any idle processing at all in some cases.
	//

	static DWORD s_dwLastIdleMessage;
	if (pMsg->message == WM_TIMER)
	{
		// Allow this WM_TIMER message to trigger idle processing only if we did not do so
		// since some seconds.
		DWORD dwNow = GetTickCount();
		if (dwNow - s_dwLastIdleMessage >= SEC2MS(5))
		{
			s_dwLastIdleMessage = dwNow;
			return TRUE;// Request idle processing (will send a WM_KICKIDLE)
		}
		return FALSE;	// No idle processing
	}

	if (!CWinApp::IsIdleMessage(pMsg))
		return FALSE;	// No idle processing

	s_dwLastIdleMessage = GetTickCount();
	return TRUE;		// Request idle processing (will send a WM_KICKIDLE)
}

LRESULT CemuleDlg::OnKickIdle(UINT /*nWhy*/, long lIdleCount)
{
	LRESULT lResult = 0;

	if (m_pSplashWnd)
	{
		if (::GetCurrentTime() - m_dwSplashTime > 2500)
		{
			// timeout expired, destroy the splash window
			DestroySplash();
			UpdateWindow();
		}
		else
		{
			// check again later...
			lResult = 1;
		}
	}

	if (m_bStartMinimized)
		PostStartupMinimized();

	if (searchwnd && searchwnd->m_hWnd)
	{
		if (theApp.m_app_state != APP_STATE_SHUTTINGDOWN)
		{
//#ifdef _DEBUG
//			TCHAR szDbg[80];
//			wsprintf(szDbg, L"%10u: lIdleCount=%d, %s", GetTickCount(), lIdleCount, (lIdleCount > 0) ? L"FreeTempMaps" : L"");
//			SetWindowText(szDbg);
//			TRACE(_T("%s\n"), szDbg);
//#endif
			// NOTE: See also 'CemuleApp::IsIdleMessage'. If 'CemuleApp::IsIdleMessage'
			// would not filter most of the WM_TIMER messages we might get a performance
			// problem here because the idle processing would be performed very, very often.
			//
			// The default MFC implementation of 'CWinApp::OnIdle' is sufficient for us. We
			// will get called with 'lIdleCount=0' and with 'lIdleCount=1'.
			//
			// CWinApp::OnIdle(0)	takes care about pending MFC GUI stuff and returns 'TRUE'
			//						to request another invocation to perform more idle processing
			// CWinApp::OnIdle(>=1)	frees temporary internally MFC maps and returns 'FALSE'
			//						because no more idle processing is needed.
			lResult = theApp.OnIdle(lIdleCount);
		}
	}

	return lResult;
}

int CemuleDlg::MapWindowToToolbarButton(CWnd* pWnd) const
{
	int iButtonID = -1;
	if (pWnd == transferwnd)        iButtonID = TBBTN_TRANSFERS;
	else if (pWnd == serverwnd)     iButtonID = TBBTN_SERVER;
	else if (pWnd == chatwnd)       iButtonID = TBBTN_MESSAGES;
	else if (pWnd == ircwnd)        iButtonID = TBBTN_IRC;
	else if (pWnd == sharedfileswnd)iButtonID = TBBTN_SHARED;
	else if (pWnd == searchwnd)     iButtonID = TBBTN_SEARCH;
	else if (pWnd == statisticswnd)	iButtonID = TBBTN_STATS;
	else if	(pWnd == kademliawnd)	iButtonID = TBBTN_KAD;
	else ASSERT(0);
	return iButtonID;
}

CWnd* CemuleDlg::MapToolbarButtonToWindow(int iButtonID) const
{
	CWnd* pWnd;
	switch (iButtonID)
	{
		case TBBTN_TRANSFERS:	pWnd = transferwnd;		break;
		case TBBTN_SERVER:		pWnd = serverwnd;		break;
		case TBBTN_MESSAGES:	pWnd = chatwnd;			break;
		case TBBTN_IRC:			pWnd = ircwnd;			break;
		case TBBTN_SHARED:		pWnd = sharedfileswnd;	break;
		case TBBTN_SEARCH:		pWnd = searchwnd;		break;
		case TBBTN_STATS:		pWnd = statisticswnd;	break;
		case TBBTN_KAD:			pWnd = kademliawnd;		break;
		default:				pWnd = NULL; ASSERT(0);
	}
	return pWnd;
}

bool CemuleDlg::IsWindowToolbarButton(int iButtonID) const
{
	switch (iButtonID)
	{
		case TBBTN_TRANSFERS:	return true;
		case TBBTN_SERVER:		return true;
		case TBBTN_MESSAGES:	return true;
		case TBBTN_IRC:			return true;
		case TBBTN_SHARED:		return true;
		case TBBTN_SEARCH:		return true;
		case TBBTN_STATS:		return true;
		case TBBTN_KAD:			return true;
	}
	return false;
}

int CemuleDlg::GetNextWindowToolbarButton(int iButtonID, int iDirection) const
{
	ASSERT( iDirection == 1 || iDirection == -1 );
	int iButtonCount = toolbar->GetButtonCount();
	if (iButtonCount > 0)
	{
		int iButtonIdx = toolbar->CommandToIndex(iButtonID);
		if (iButtonIdx >= 0 && iButtonIdx < iButtonCount)
		{
			int iEvaluatedButtons = 0;
			while (iEvaluatedButtons < iButtonCount)
			{
				iButtonIdx = iButtonIdx + iDirection;
				if (iButtonIdx < 0)
					iButtonIdx = iButtonCount - 1;
				else if (iButtonIdx >= iButtonCount)
					iButtonIdx = 0;

				TBBUTTON tbbt = {0};
				if (toolbar->GetButton(iButtonIdx, &tbbt))
				{
					if (IsWindowToolbarButton(tbbt.idCommand))
						return tbbt.idCommand;
				}
				iEvaluatedButtons++;
			}
		}
	}
	return -1;
}

BOOL CemuleDlg::PreTranslateMessage(MSG* pMsg)
{
	BOOL bResult = CTrayDialog::PreTranslateMessage(pMsg);

	if (m_pSplashWnd && m_pSplashWnd->m_hWnd != NULL &&
		(pMsg->message == WM_KEYDOWN	   ||
		 pMsg->message == WM_SYSKEYDOWN	   ||
		 pMsg->message == WM_LBUTTONDOWN   ||
		 pMsg->message == WM_RBUTTONDOWN   ||
		 pMsg->message == WM_MBUTTONDOWN   ||
		 pMsg->message == WM_NCLBUTTONDOWN ||
		 pMsg->message == WM_NCRBUTTONDOWN ||
		 pMsg->message == WM_NCMBUTTONDOWN))
	{
		DestroySplash();
		UpdateWindow();
	}
	else
	{
		if (pMsg->message == WM_KEYDOWN)
		{
			// Handle Ctrl+Tab and Ctrl+Shift+Tab
			if (pMsg->wParam == VK_TAB && GetAsyncKeyState(VK_CONTROL) < 0)
			{
				int iButtonID = MapWindowToToolbarButton(activewnd);
				if (iButtonID != -1)
				{
					int iNextButtonID = GetNextWindowToolbarButton(iButtonID, GetAsyncKeyState(VK_SHIFT) < 0 ? -1 : 1);
					if (iNextButtonID != -1)
					{
						CWnd* pWndNext = MapToolbarButtonToWindow(iNextButtonID);
						if (pWndNext)
							SetActiveDialog(pWndNext);
					}
				}
			}
		}
	}

	return bResult;
}

void CemuleDlg::HtmlHelp(DWORD_PTR dwData, UINT nCmd)
{
	CWinApp* pApp = AfxGetApp();
	ASSERT_VALID(pApp);
	ASSERT(pApp->m_pszHelpFilePath != NULL);
	// to call HtmlHelp the m_fUseHtmlHelp must be set in
	// the application's constructor
	ASSERT(pApp->m_eHelpType == afxHTMLHelp);

	CWaitCursor wait;

	PrepareForHelp();

	// need to use top level parent (for the case where m_hWnd is in DLL)
	CWnd* pWnd = GetTopLevelParent();

	TRACE(traceAppMsg, 0, _T("HtmlHelp: pszHelpFile = '%s', dwData: $%lx, fuCommand: %d.\n"), pApp->m_pszHelpFilePath, dwData, nCmd);

	bool bHelpError = false;
	CString strHelpError;
	int iTry = 0;
	while (iTry++ < 2)
	{
		if (!AfxHtmlHelp(pWnd->m_hWnd, pApp->m_pszHelpFilePath, nCmd, dwData))
		{
			bHelpError = true;
			strHelpError.LoadString(AFX_IDP_FAILED_TO_LAUNCH_HELP);

			typedef struct tagHH_LAST_ERROR
			{
				int      cbStruct;
				HRESULT  hr;
				BSTR     description;
			} HH_LAST_ERROR;
			HH_LAST_ERROR hhLastError = {0};
			hhLastError.cbStruct = sizeof hhLastError;
			HWND hwndResult = AfxHtmlHelp(pWnd->m_hWnd, NULL, HH_GET_LAST_ERROR, reinterpret_cast<DWORD>(&hhLastError));
			if (hwndResult != 0)
			{
				if (FAILED(hhLastError.hr))
				{
					if (hhLastError.description)
					{
						strHelpError = hhLastError.description;
						::SysFreeString(hhLastError.description);
					}
					if (   hhLastError.hr == 0x8004020A  /*no topics IDs available in Help file*/
						|| hhLastError.hr == 0x8004020B) /*requested Help topic ID not found*/
					{
						// try opening once again without help topic ID
						if (nCmd != HH_DISPLAY_TOC)
						{
							nCmd = HH_DISPLAY_TOC;
							dwData = 0;
							continue;
						}
					}
				}
			}
			break;
		}
		else
		{
			bHelpError = false;
			strHelpError.Empty();
			break;
		}
	}

	if (bHelpError)
	{
		if (AfxMessageBox(CString(pApp->m_pszHelpFilePath) + _T("\n\n") + strHelpError + _T("\n\n") + GetResString(IDS_ERR_NOHELP), MB_YESNO | MB_ICONERROR) == IDYES)
		{
			CString strUrl = thePrefs.GetHomepageBaseURL() + _T("/home/perl/help.cgi");
			ShellExecute(NULL, NULL, strUrl, NULL, thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), SW_SHOWDEFAULT);
		}
	}
}

LRESULT CemuleDlg::OnPeerCacheResponse(WPARAM wParam, LPARAM lParam)
{
	return theApp.m_pPeerCache->OnPeerCacheCheckResponse(wParam,lParam);
}

void CemuleDlg::CreateToolbarCmdIconMap()
{
	m_mapTbarCmdToIcon.SetAt(TBBTN_CONNECT, _T("Connect"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_KAD, _T("Kademlia"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_SERVER, _T("Server"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_TRANSFERS, _T("Transfer"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_SEARCH, _T("Search"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_SHARED, _T("SharedFiles"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_MESSAGES, _T("Messages"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_IRC, _T("IRC"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_STATS, _T("Statistics"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_OPTIONS, _T("Preferences"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_TOOLS, _T("Tools"));
	m_mapTbarCmdToIcon.SetAt(TBBTN_HELP, _T("Help"));
}

LPCTSTR CemuleDlg::GetIconFromCmdId(UINT uId)
{
	LPCTSTR pszIconId = NULL;
	if (m_mapTbarCmdToIcon.Lookup(uId, pszIconId))
		return pszIconId;
	return NULL;
}

BOOL CemuleDlg::OnChevronPushed(UINT id, NMHDR* pNMHDR, LRESULT* plResult)
{
	UNREFERENCED_PARAMETER(id);
	if (!thePrefs.GetUseReBarToolbar())
		return FALSE;

	NMREBARCHEVRON* pnmrc = (NMREBARCHEVRON*)pNMHDR;

	ASSERT( id == AFX_IDW_REBAR );
	ASSERT( pnmrc->uBand == 0 );
	ASSERT( pnmrc->wID == 0 );
	ASSERT( m_mapTbarCmdToIcon.GetSize() != 0 );

	// get visible area of rebar/toolbar
	CRect rcVisibleButtons;
	toolbar->GetClientRect(&rcVisibleButtons);

	// search the first toolbar button which is not fully visible
	int iButtons = toolbar->GetButtonCount();
	int i;
	for (i = 0; i < iButtons; i++)
	{
		CRect rcButton;
		toolbar->GetItemRect(i, &rcButton);

		CRect rcVisible;
		if (!rcVisible.IntersectRect(&rcVisibleButtons, &rcButton) || !EqualRect(rcButton, rcVisible))
			break;
	}

	// create menu for all toolbar buttons which are not (fully) visible
	BOOL bLastMenuItemIsSep = TRUE;
	CTitleMenu menu;
	menu.CreatePopupMenu();
	menu.AddMenuTitle(_T("eMule"), true);
	while (i < iButtons)
	{
		TCHAR szString[256];
		szString[0] = _T('\0');
		TBBUTTONINFO tbbi = {0};
		tbbi.cbSize = sizeof tbbi;
		tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE | TBIF_STATE | TBIF_TEXT;
		tbbi.cchText = _countof(szString);
		tbbi.pszText = szString;
		if (toolbar->GetButtonInfo(i, &tbbi) != -1)
		{
			szString[_countof(szString) - 1] = _T('\0');
			if (tbbi.fsStyle & TBSTYLE_SEP)
			{
				if (!bLastMenuItemIsSep)
					bLastMenuItemIsSep = menu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
			}
			else
			{
				if (szString[0] != _T('\0') && menu.AppendMenu(MF_STRING, tbbi.idCommand, szString, GetIconFromCmdId(tbbi.idCommand)))
				{
					bLastMenuItemIsSep = FALSE;
					if (tbbi.fsState & TBSTATE_CHECKED)
						menu.CheckMenuItem(tbbi.idCommand, MF_BYCOMMAND | MF_CHECKED);
					if ((tbbi.fsState & TBSTATE_ENABLED) == 0)
						menu.EnableMenuItem(tbbi.idCommand, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				}
			}
		}

		i++;
	}

	CPoint ptMenu(pnmrc->rc.left, pnmrc->rc.top);
	ClientToScreen(&ptMenu);
	ptMenu.y += rcVisibleButtons.Height();
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, ptMenu.x, ptMenu.y, this);
	*plResult = 1;
	return FALSE;
}

bool CemuleDlg::IsPreferencesDlgOpen() const
{
	return (preferenceswnd->m_hWnd != NULL);
}

int CemuleDlg::ShowPreferences(UINT uStartPageID)
{
	if (IsPreferencesDlgOpen())
	{
		preferenceswnd->SetForegroundWindow();
		preferenceswnd->BringWindowToTop();
		return -1;
	}
	else
	{
		if (uStartPageID != (UINT)-1)
			preferenceswnd->SetStartPage(uStartPageID);
		return preferenceswnd->DoModal();
	}
}



//////////////////////////////////////////////////////////////////
// Webserver related

LRESULT CemuleDlg::OnWebAddDownloads(WPARAM wParam, LPARAM lParam)
{
	CString link=CString((TCHAR*)wParam);
	if (link.GetLength()==32 && link.Left(4).CompareNoCase(_T("ed2k"))!=0) {
		uchar fileid[16];
		DecodeBase16(link, link.GetLength(), fileid, _countof(fileid));
		theApp.searchlist->AddFileToDownloadByHash(fileid,(uint8)lParam);

	} else
		theApp.AddEd2kLinksToDownload((TCHAR*)wParam, (int)lParam);

	return 0;
}

LRESULT CemuleDlg::OnAddRemoveFriend(WPARAM wParam, LPARAM lParam)
{
	if (lParam==0) { // remove
		theApp.friendlist->RemoveFriend((CFriend*)wParam);
	} else {		// add
		theApp.friendlist->AddFriend((CUpDownClient*)wParam);
	}

	return 0;
}

LRESULT CemuleDlg::OnWebSetCatPrio(WPARAM wParam, LPARAM lParam)
{
	theApp.downloadqueue->SetCatPrio(wParam,(uint8)lParam);
	return 0;
}
LRESULT CemuleDlg::OnWebServerClearCompleted(WPARAM wParam, LPARAM lParam)
{
	if(!wParam)
	{
		int cat=(int)lParam;
		transferwnd->GetDownloadList()->ClearCompleted(cat);
	}
	else
	{
		uchar* pFileHash = reinterpret_cast<uchar*>(lParam);
		CKnownFile* file=theApp.knownfiles->FindKnownFileByID(pFileHash);
		if (file)
			transferwnd->GetDownloadList()->RemoveFile((CPartFile*)file);
		delete[] pFileHash;
	}

	return 0;
}

LRESULT CemuleDlg::OnWebServerFileRename(WPARAM wParam, LPARAM lParam)
{
	CString sNewName = ((LPCTSTR)(lParam));

	((CPartFile*)wParam)->SetFileName(sNewName);
	((CPartFile*)wParam)->SavePartFile();
	((CPartFile*)wParam)->UpdateDisplayedInfo();
	sharedfileswnd->sharedfilesctrl.UpdateFile( (CKnownFile*)((CPartFile*)wParam));

	return 0;
}

LRESULT CemuleDlg::OnWebGUIInteraction(WPARAM wParam, LPARAM lParam) {

	switch (wParam) {
		case WEBGUIIA_UPDATEMYINFO:
			serverwnd->UpdateMyInfo();
			break;
		case WEBGUIIA_WINFUNC:{
			if (thePrefs.GetWebAdminAllowedHiLevFunc())
			{
				try {
					HANDLE hToken;
					TOKEN_PRIVILEGES tkp;	// Get a token for this process.
					
					if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
						throw;
					LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
					tkp.PrivilegeCount = 1;  // one privilege to set
					tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;	// Get the shutdown privilege for this process.
					AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

					if (lParam==1) {	// shutdown
						ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
					} else 
					if (lParam==2) {
						ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
					}

				} catch(...)
					{
						AddLogLine(true, GetResString(IDS_WEB_REBOOT) + _T(' ') + GetResString(IDS_FAILED));
				}
			}
			else 
				AddLogLine(true, GetResString(IDS_WEB_REBOOT) + _T(' ') + GetResString(IDS_ACCESSDENIED));
			break;
		}
		case WEBGUIIA_UPD_CATTABS:
			theApp.emuledlg->transferwnd->UpdateCatTabTitles();
			break;
		case WEBGUIIA_UPD_SFUPDATE: {
			CKnownFile* kf=(CKnownFile*)lParam;
			if (kf)
				theApp.sharedfiles->UpdateFile(kf);
			}
			break;
		case WEBGUIIA_UPDATESERVER:
			serverwnd->serverlistctrl.RefreshServer((CServer*)lParam);
			break;
		case WEBGUIIA_STOPCONNECTING:
			theApp.serverconnect->StopConnectionTry();
			break;
		case WEBGUIIA_CONNECTTOSERVER: {
			CServer* server=(CServer*)lParam;
			if (server==NULL) 
				theApp.serverconnect->ConnectToAnyServer();
			else 
				theApp.serverconnect->ConnectToServer(server);
			break;
			}
		case WEBGUIIA_DISCONNECT:
			if (lParam!=2)	// !KAD
				theApp.serverconnect->Disconnect();
			if (lParam!=1)	// !ED2K
				Kademlia::CKademlia::Stop();
			break;

		case WEBGUIIA_SERVER_REMOVE: {
			serverwnd->serverlistctrl.RemoveServer((CServer*)lParam);
			break;
		}
		case WEBGUIIA_SHARED_FILES_RELOAD: {
			theApp.sharedfiles->Reload();
			break;
		}
		case WEBGUIIA_ADD_TO_STATIC: {
			serverwnd->serverlistctrl.StaticServerFileAppend((CServer*)lParam);
			break;
		}
		case WEBGUIIA_REMOVE_FROM_STATIC: {
			serverwnd->serverlistctrl.StaticServerFileRemove((CServer*)lParam);
			break;
		}
		case WEBGUIIA_UPDATESERVERMETFROMURL:
			theApp.emuledlg->serverwnd->UpdateServerMetFromURL((TCHAR*)lParam);
			break;
		case WEBGUIIA_SHOWSTATISTICS:
			theApp.emuledlg->statisticswnd->ShowStatistics(lParam!=0);
			break;
		case WEBGUIIA_DELETEALLSEARCHES:
			theApp.emuledlg->searchwnd->DeleteAllSearches();
			break;

		case WEBGUIIA_KAD_BOOTSTRAP:{
			CString dest=CString((TCHAR*)lParam);
			int pos=dest.Find(_T(':'));
			if (pos!=-1) {
				uint16 port = (uint16)_tstoi(dest.Right(dest.GetLength() - pos - 1));
				CString ip = dest.Left(pos);
				Kademlia::CKademlia::Bootstrap(ip, port);
			}
			break;
		}
		case WEBGUIIA_KAD_START:
			Kademlia::CKademlia::Start();
			break;
		case WEBGUIIA_KAD_STOP:
			Kademlia::CKademlia::Stop();
			break;
		case WEBGUIIA_KAD_RCFW:
			Kademlia::CKademlia::RecheckFirewalled();
			break;
	}

	return 0;
}

void CemuleDlg::TrayMinimizeToTrayChange()
{
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		if (!thePrefs.GetMinToTray())
		{
			// just for safety, ensure that we are not adding duplicate menu entries..
			if (pSysMenu->EnableMenuItem(MP_MINIMIZETOTRAY, MF_BYCOMMAND | MF_ENABLED) == -1)
			{
				ASSERT( (MP_MINIMIZETOTRAY & 0xFFF0) == MP_MINIMIZETOTRAY && MP_MINIMIZETOTRAY < 0xF000);
				VERIFY( pSysMenu->InsertMenu(SC_MINIMIZE, MF_BYCOMMAND, MP_MINIMIZETOTRAY, GetResString(IDS_PW_TRAY)) );
			}
			else
				ASSERT(0);
		}
		else
		{
			(void)pSysMenu->RemoveMenu(MP_MINIMIZETOTRAY, MF_BYCOMMAND);
		}
	}
	CTrayDialog::TrayMinimizeToTrayChange();
}

void CemuleDlg::SetToolTipsDelay(UINT uMilliseconds)
{
	//searchwnd->SetToolTipsDelay(uMilliseconds);
	transferwnd->SetToolTipsDelay(uMilliseconds);
	sharedfileswnd->SetToolTipsDelay(uMilliseconds);
}

void CemuleDlg::UPnPTimeOutTimer(HWND /*hwnd*/, UINT /*uiMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/){
	::PostMessage(theApp.emuledlg->GetSafeHwnd(), UM_UPNP_RESULT, (WPARAM)CUPnPImpl::UPNP_TIMEOUT, 0);
}

LRESULT CemuleDlg::OnUPnPResult(WPARAM wParam, LPARAM lParam){
	bool bWasRefresh = lParam != 0;
	if (!bWasRefresh && wParam == CUPnPImpl::UPNP_FAILED){		
		// UPnP failed, check if we can retry it with another implementation
		if (theApp.m_pUPnPFinder->SwitchImplentation()){
			StartUPnP(false);
			return 0;
		}
		else
			DebugLog(_T("No more available UPnP implementations left"));

	}
	if (m_hUPnPTimeOutTimer != 0){
		VERIFY( ::KillTimer(NULL, m_hUPnPTimeOutTimer) );
		m_hUPnPTimeOutTimer = 0;
	}
	if(IsRunning() && m_bConnectRequestDelayedForUPnP){
		StartConnection();
	}
	if (!bWasRefresh && wParam == CUPnPImpl::UPNP_OK){
		// remember the last working implementation
		thePrefs.SetLastWorkingUPnPImpl(theApp.m_pUPnPFinder->GetImplementation()->GetImplementationID());
		Log(GetResString(IDS_UPNPSUCCESS), theApp.m_pUPnPFinder->GetImplementation()->GetUsedTCPPort()
			, theApp.m_pUPnPFinder->GetImplementation()->GetUsedUDPPort());
	}
	else if (!bWasRefresh)
		LogWarning(GetResString(IDS_UPNPFAILED));

	return 0;
}

LRESULT  CemuleDlg::OnPowerBroadcast(WPARAM wParam, LPARAM lParam)
{
	//DebugLog(_T("DEBUG:Power state change. wParam=%d lPararm=%ld"),wParam,lParam);
	switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC:
		{
			if (m_bEd2kSuspendDisconnect || m_bKadSuspendDisconnect)
			{
				DebugLog(_T("Reconnect after Power state change. wParam=%d lPararm=%ld"),wParam,lParam);
				RefreshUPnP(true);
				PostMessage(WM_SYSCOMMAND , MP_CONNECT, 0); // tell to connect.. a sec later...
			}
			return TRUE; // message processed.
			break;
		}
		case PBT_APMSUSPEND:
		{		
			DebugLog(_T("System is going is suspending operation, disconnecting. wParam=%d lPararm=%ld"),wParam,lParam);
			m_bEd2kSuspendDisconnect = theApp.serverconnect->IsConnected();
			m_bKadSuspendDisconnect = Kademlia::CKademlia::IsConnected();
			CloseConnection();
			return TRUE; // message processed.
			break;
		}
		default:
			return FALSE; // we do not process this message
	}

}

void CemuleDlg::StartUPnP(bool bReset, uint16 nForceTCPPort, uint16 nForceUDPPort) {
	if (theApp.m_pUPnPFinder != NULL && (m_hUPnPTimeOutTimer == 0 || !bReset)){
		if (bReset){
			theApp.m_pUPnPFinder->Reset();
			Log(GetResString(IDS_UPNPSETUP));
		}
		try
		{
			if (theApp.m_pUPnPFinder->GetImplementation()->IsReady()){
				theApp.m_pUPnPFinder->GetImplementation()->SetMessageOnResult(GetSafeHwnd(), UM_UPNP_RESULT);
				if (bReset)
					VERIFY( (m_hUPnPTimeOutTimer = ::SetTimer(NULL, NULL, SEC2MS(40), UPnPTimeOutTimer)) != NULL );
				theApp.m_pUPnPFinder->GetImplementation()->StartDiscovery(((nForceTCPPort != 0) ? nForceTCPPort : thePrefs.GetPort())
					, ((nForceUDPPort != 0) ? nForceUDPPort :thePrefs.GetUDPPort())
					, thePrefs.GetWSUseUPnP() ? thePrefs.GetWSPort() : 0);
			}
			else
				::PostMessage(theApp.emuledlg->GetSafeHwnd(), UM_UPNP_RESULT, (WPARAM)CUPnPImpl::UPNP_FAILED, 0);
		}
		catch ( CUPnPImpl::UPnPError& ) {}
		catch ( CException* e ) { e->Delete(); }
	}
	else
		ASSERT( false );
}

void CemuleDlg::RefreshUPnP(bool bRequestAnswer)
{
	if (!thePrefs.IsUPnPEnabled())
		return;
	if (theApp.m_pUPnPFinder != NULL && m_hUPnPTimeOutTimer == 0){
		try
		{
			if (theApp.m_pUPnPFinder->GetImplementation()->IsReady())
			{
				if (bRequestAnswer)
					theApp.m_pUPnPFinder->GetImplementation()->SetMessageOnResult(GetSafeHwnd(), UM_UPNP_RESULT);
				if (theApp.m_pUPnPFinder->GetImplementation()->CheckAndRefresh() && bRequestAnswer)
				{
					VERIFY( (m_hUPnPTimeOutTimer = ::SetTimer(NULL, NULL, SEC2MS(10), UPnPTimeOutTimer)) != NULL );
				}
				else
					theApp.m_pUPnPFinder->GetImplementation()->SetMessageOnResult(0, 0);
			}
			else
				DebugLogWarning(_T("RefreshUPnP, implementation not ready"));
		}
		catch ( CUPnPImpl::UPnPError& ) {}
		catch ( CException* e ) { e->Delete(); }
	}
	else
		ASSERT( false );
}

BOOL CemuleDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	// WM_DEVICECHANGE is sent for:
	//	Drives which where created/deleted with "SUBST" command (handled like network drives)
	//	Drives which where created/deleted as regular network drives.
	//
	// WM_DEVICECHANGE is *NOT* sent for:
	//	Floppy disk drives
	//	ZIP disk drives (although Windows Explorer recognizes a changed media, we do not get a message)
	//	CD-ROM drives (although MSDN says different...)
	//
	if ((nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEREMOVECOMPLETE) && !IsBadReadPtr((void *)dwData, sizeof(DEV_BROADCAST_HDR)))
	{
#ifdef _DEBUG
		CString strMsg;
		if (nEventType == DBT_DEVICEARRIVAL)
			strMsg += _T("DBT_DEVICEARRIVAL");
		else if (nEventType == DBT_DEVICEREMOVECOMPLETE)
			strMsg += _T("DBT_DEVICEREMOVECOMPLETE");
#endif

		const DEV_BROADCAST_HDR *pHdr = (DEV_BROADCAST_HDR *)dwData;
		if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME && !IsBadReadPtr((void *)dwData, sizeof(DEV_BROADCAST_VOLUME)))
		{
			const DEV_BROADCAST_VOLUME *pVol = (DEV_BROADCAST_VOLUME *)pHdr;
#ifdef _DEBUG
			strMsg += _T(" Volume");
			if (pVol->dbcv_flags & DBTF_MEDIA)
				strMsg += _T(" Media");
			if (pVol->dbcv_flags & DBTF_NET)
				strMsg += _T(" Net");
			if ((pVol->dbcv_flags & ~(DBTF_NET | DBTF_MEDIA)) != 0)
				strMsg.AppendFormat(_T(" flags=0x%08x"), pVol->dbcv_flags);
#endif

			bool bVolumesChanged = false;
			for (UINT uDrive = 0; uDrive <= 25; uDrive++)
			{
				UINT uMask = 1 << uDrive;
				if (pVol->dbcv_unitmask & uMask)
				{
					DEBUG_ONLY( strMsg.AppendFormat(_T(" %c:"), _T('A') + uDrive) );
					if (pVol->dbcv_flags & (DBTF_MEDIA | DBTF_NET))
						ClearVolumeInfoCache(uDrive);
					bVolumesChanged = true;
				}
			}
			if (bVolumesChanged && sharedfileswnd)
				sharedfileswnd->OnVolumesChanged();
		}
		else
		{
			DEBUG_ONLY( strMsg.AppendFormat(_T(" devicetype=0x%08x"), pHdr->dbch_devicetype) );
		}
#ifdef _DEBUG
		TRACE(_T("CemuleDlg::OnDeviceChange: %s\n"), strMsg);
#endif
	}
	else
		TRACE(_T("CemuleDlg::OnDeviceChange: nEventType=0x%08x  dwData=0x%08x\n"), nEventType, dwData);
	return __super::OnDeviceChange(nEventType, dwData);
}


//////////////////////////////////////////////////////////////////
// Windows 7 GUI goodies

#ifdef HAVE_WIN7_SDK_H
// update thumbbarbutton structs and add/update the GUI thumbbar
void CemuleDlg::UpdateThumbBarButtons(bool initialAddToDlg) {
	
	if (!m_pTaskbarList)
		return;
	
	THUMBBUTTONMASK dwMask = THB_ICON | THB_FLAGS;
	for (int i=TBB_FIRST; i<=TBB_LAST;i++) {
		m_thbButtons[i].dwMask = dwMask;
		m_thbButtons[i].iId = i;
		m_thbButtons[i].iBitmap = 0;
		m_thbButtons[i].dwFlags = THBF_DISMISSONCLICK;
		CString tooltip;

		switch(i) {
			case TBB_CONNECT:
				{
					m_thbButtons[i].hIcon   =  theApp.LoadIcon(_T("CONNECT"), 16, 16);
					tooltip = GetResString(IDS_MAIN_BTN_CONNECT);
					if (theApp.IsConnected()==true)
						m_thbButtons[i].dwFlags |= THBF_DISABLED;
					break;
				}
			case TBB_DISCONNECT:
				{
					m_thbButtons[i].hIcon   = theApp.LoadIcon(_T("DISCONNECT"), 16, 16);
					tooltip = GetResString(IDS_MAIN_BTN_DISCONNECT);
					if (theApp.IsConnected()==false)
						m_thbButtons[i].dwFlags |= THBF_DISABLED;
					break;
				}
			case TBB_THROTTLE:
				{
					m_thbButtons[i].hIcon   = theApp.LoadIcon(_T("SPEEDMIN"), 16, 16);
					tooltip = GetResString(IDS_PW_PA);
					break;
				}
			case TBB_UNTHROTTLE:
				{
					m_thbButtons[i].hIcon   =  theApp.LoadIcon(_T("SPEEDMAX"), 16, 16);
					tooltip = GetResString(IDS_PW_UA);
					break;
				}
			case TBB_PREFERENCES:
				m_thbButtons[i].hIcon   =  theApp.LoadIcon(_T("PREFERENCES"), 16, 16);
				tooltip = GetResString(IDS_EM_PREFS);
				break;
		}
		// set tooltips in widechar
		if (!tooltip.IsEmpty()) {
			tooltip.Remove('&');
			wcscpy(m_thbButtons[i].szTip,tooltip);
			m_thbButtons[i].dwMask |= THB_TOOLTIP;
		}
	}

	if (initialAddToDlg)
		m_pTaskbarList->ThumbBarAddButtons(m_hWnd, ARRAYSIZE(m_thbButtons), m_thbButtons);
	else
		m_pTaskbarList->ThumbBarUpdateButtons(m_hWnd, ARRAYSIZE(m_thbButtons), m_thbButtons);

	// clean up icons, they were copied in the previous call
	for (int i=TBB_FIRST; i<=TBB_LAST;i++) {
		DestroyIcon(m_thbButtons[i].hIcon);
	}
}

// Handle pressed thumbbar button
void CemuleDlg::OnTBBPressed(UINT id)
{
	switch (id) {
		case TBB_CONNECT:
			OnBnClickedConnect();
			break;
		case TBB_DISCONNECT:
			CloseConnection();
			break;
		case TBB_THROTTLE:
			QuickSpeedOther(MP_QS_PA);
			break;
		case TBB_UNTHROTTLE:
			QuickSpeedOther(MP_QS_UA);
			break;
		case TBB_PREFERENCES:
			ShowPreferences();
			break;
	}
}

// When Windows tells us, the taskbarbutton was created, it is safe to initialize our taskbar stuff
LRESULT CemuleDlg::OnTaskbarBtnCreated ( WPARAM , LPARAM  )
{
	// Sanity check that the OS is Win 7 or later
	if (thePrefs.GetWindowsVersion() >= _WINVER_7_ && IsRunning())
	{
		if (m_pTaskbarList)
			m_pTaskbarList.Release();
		
		if (m_pTaskbarList.CoCreateInstance ( CLSID_TaskbarList ) == S_OK)
		{
			m_pTaskbarList->SetProgressState ( m_hWnd, TBPF_NOPROGRESS );
			
			m_currentTBP_state = TBPF_NOPROGRESS;
			m_prevProgress=0;
			m_ovlIcon = NULL;
			
			UpdateThumbBarButtons(true);
			UpdateStatusBarProgress();
		}
		else
			ASSERT( false );
	}
	return 0;
}

// Updates global progress and /down state overlayicon
// Overlayicon looks rather annoying than useful, so its disabled by default for the common user and can be enabled by ini setting only (Ornis)
void CemuleDlg::EnableTaskbarGoodies(bool enable)
{
	if (m_pTaskbarList) {
		m_pTaskbarList->SetOverlayIcon ( m_hWnd, NULL, _T("") );
		if (!enable) {
			m_pTaskbarList->SetProgressState ( m_hWnd, TBPF_NOPROGRESS );
			m_currentTBP_state=TBPF_NOPROGRESS;
			m_prevProgress=0;
			m_ovlIcon = NULL;
		}
		else
			UpdateStatusBarProgress();
	}
}
void CemuleDlg::UpdateStatusBarProgress()
{
	if (m_pTaskbarList && thePrefs.IsWin7TaskbarGoodiesEnabled()) 
	{
		// calc global progress & status
		float globalDone = theStats.m_fGlobalDone;
		float globalSize = theStats.m_fGlobalSize;
		float finishedsize = theApp.emuledlg->transferwnd->GetDownloadList()->GetFinishedSize();
		globalDone += finishedsize;
		globalSize += finishedsize;
		float overallProgress = globalSize?(globalDone/globalSize):0;

		TBPFLAG			new_state=m_currentTBP_state;

		if (globalSize==0) {
			// if there is no download, disable progress
			if (m_currentTBP_state!=TBPF_NOPROGRESS) {
				m_currentTBP_state=TBPF_NOPROGRESS;
				m_pTaskbarList->SetProgressState ( m_hWnd, TBPF_NOPROGRESS );
			}
		} else {

			new_state=TBPF_PAUSED;

			if (theStats.m_dwOverallStatus & STATE_DOWNLOADING) // smth downloading
				new_state=TBPF_NORMAL;
			
			if (theStats.m_dwOverallStatus & STATE_ERROROUS) // smth error
				new_state=TBPF_ERROR;

			if (new_state!=m_currentTBP_state) {
				m_pTaskbarList->SetProgressState ( m_hWnd, new_state );
				m_currentTBP_state=new_state;
			}

			if (overallProgress != m_prevProgress) {
				m_pTaskbarList->SetProgressValue(m_hWnd,(ULONGLONG)(overallProgress*100) ,100);
				m_prevProgress=overallProgress;
			}

		}
		// overlay up/down-speed
		if (thePrefs.IsShowUpDownIconInTaskbar()) 
		{
			bool bUp   = theApp.emuledlg->transferwnd->GetUploadList()->GetItemCount() >0;
			bool bDown = theStats.m_dwOverallStatus & STATE_DOWNLOADING;

			HICON newicon = NULL;
			if (bUp && bDown)
				newicon=transicons[3];
			else if (bUp)
				newicon=transicons[2];
			else if (bDown)
				newicon=transicons[1];
			else
				newicon = NULL;

			if (m_ovlIcon!=newicon) {
				m_ovlIcon=newicon;
				m_pTaskbarList->SetOverlayIcon ( m_hWnd, m_ovlIcon, _T("eMule Up/Down Indicator") );
			}
		}
	}
}
#endif

void CemuleDlg::SetTaskbarIconColor()
{
	bool bBrightTaskbarIconSpeed = false;
	bool bTransparent = false;
	COLORREF cr = RGB(0, 0, 0);
	if (thePrefs.IsRunningAeroGlassTheme())
	{
		HMODULE hDWMAPI = LoadLibrary(_T("dwmapi.dll"));
		if (hDWMAPI){
			HRESULT (WINAPI *pfnDwmGetColorizationColor)(DWORD*, BOOL*);
			(FARPROC&)pfnDwmGetColorizationColor = GetProcAddress(hDWMAPI, "DwmGetColorizationColor");
			DWORD dwGlassColor = 0;
			BOOL bOpaque;
			if (pfnDwmGetColorizationColor != NULL)
			{
				if (pfnDwmGetColorizationColor(&dwGlassColor, &bOpaque) == S_OK)
				{
					uint8 byAlpha = (uint8)(dwGlassColor >> 24);
					cr = 0xFFFFFF & dwGlassColor;
					if (byAlpha < 200 && bOpaque == FALSE)
					{
						// on transparent themes we can never figure out which excact color is shown (may we could in real time?)
						// but given that a color is blended against the background, it is a good guess that a bright speedbar will
						// be the best solution in most cases
						bTransparent = true;
					}							
				}
			}
			FreeLibrary(hDWMAPI);
		}
	}
	else
	{
		if (g_xpStyle.IsThemeActive() && g_xpStyle.IsAppThemed())
		{
			CWnd* ptmpWnd = new CWnd();
			VERIFY( ptmpWnd->Create(_T("STATIC"), _T("Tmp"), 0, CRect(0, 0, 10, 10), this, 1235) );
			VERIFY( g_xpStyle.SetWindowTheme(ptmpWnd->GetSafeHwnd(), L"TrayNotifyHoriz", NULL) == S_OK );
			HTHEME hTheme = g_xpStyle.OpenThemeData(ptmpWnd->GetSafeHwnd(), L"TrayNotify");
			if (hTheme != NULL)
			{

				if (g_xpStyle.GetThemeColor(hTheme, TNP_BACKGROUND, 0, TMT_FILLCOLORHINT, &cr) != S_OK)
					ASSERT( false );
				g_xpStyle.CloseThemeData(hTheme);
			}
			else
				ASSERT( false );
			ptmpWnd->DestroyWindow();
			delete ptmpWnd;
		}
		else
		{
			DEBUG_ONLY(DebugLog(_T("Taskbar Notifier Color: GetSysColor() used")));
			cr = GetSysColor(COLOR_3DFACE);
		}
	}
	uint8 iRed = GetRValue(cr);
	uint8 iBlue = GetBValue(cr);
	uint8 iGreen = GetGValue(cr);
	uint16 iBrightness = (uint16)sqrt(((iRed * iRed * 0.241f) + (iGreen * iGreen * 0.691f) + (iBlue * iBlue * 0.068f)));
	ASSERT( iBrightness <= 255 );
	bBrightTaskbarIconSpeed = iBrightness < 132;
	DebugLog(_T("Taskbar Notifier Color: R:%u G:%u B:%u, Brightness: %u, Transparent: %s"), iRed, iGreen, iBlue, iBrightness, bTransparent ? _T("Yes") : _T("No"));
	if (bBrightTaskbarIconSpeed || bTransparent)
		thePrefs.SetStatsColor(11, RGB(255, 255, 255));
	else
		thePrefs.SetStatsColor(11, RGB(0, 0, 0));
}
