//this file is part of eMule
//Copyright (C)2002-2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <io.h>
#include <share.h>
#include <iphlpapi.h>
#include "emule.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "OtherFunctions.h"
#include "Ini2.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "Statistics.h"
#include "MD5Sum.h"
#include "PartFile.h"
#include "Sockets.h"
#include "ListenSocket.h"
#include "ServerList.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "SafeFile.h"
#include "emuledlg.h"
#include "StatisticsDlg.h"
#include "Log.h"
#include "MuleToolbarCtrl.h"
#include "VistaDefines.h"

#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#include <crypto51/osrng.h>
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPreferences thePrefs;

CString CPreferences::m_astrDefaultDirs[13];
bool	CPreferences::m_abDefaultDirsCreated[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int		CPreferences::m_nCurrentUserDirMode = -1;
int		CPreferences::m_iDbgHeap;
CString	CPreferences::strNick;
uint16	CPreferences::minupload;
uint16	CPreferences::maxupload;
uint16	CPreferences::maxdownload;
LPCSTR	CPreferences::m_pszBindAddrA;
CStringA CPreferences::m_strBindAddrA;
LPCWSTR	CPreferences::m_pszBindAddrW;
CStringW CPreferences::m_strBindAddrW;
uint16	CPreferences::port;
uint16	CPreferences::udpport;
uint16	CPreferences::nServerUDPPort;
UINT	CPreferences::maxconnections;
UINT	CPreferences::maxhalfconnections;
bool	CPreferences::m_bOverlappedSockets;
bool	CPreferences::m_bConditionalTCPAccept;
bool	CPreferences::reconnect;
bool	CPreferences::m_bUseServerPriorities;
bool	CPreferences::m_bUseUserSortedServerList;
CString	CPreferences::m_strIncomingDir;
CStringArray CPreferences::tempdir;
bool	CPreferences::ICH;
bool	CPreferences::m_bAutoUpdateServerList;
bool	CPreferences::updatenotify;
bool	CPreferences::mintotray;
bool	CPreferences::autoconnect;
bool	CPreferences::m_bAutoConnectToStaticServersOnly;
bool	CPreferences::autotakeed2klinks;
bool	CPreferences::addnewfilespaused;
UINT	CPreferences::depth3D;
bool	CPreferences::m_bEnableMiniMule;
int		CPreferences::m_iStraightWindowStyles;
bool	CPreferences::m_bUseSystemFontForMainControls;
bool	CPreferences::m_bRTLWindowsLayout;
CString	CPreferences::m_strSkinProfile;
CString	CPreferences::m_strSkinProfileDir;
bool	CPreferences::m_bAddServersFromServer;
bool	CPreferences::m_bAddServersFromClients;
UINT	CPreferences::maxsourceperfile;
UINT	CPreferences::trafficOMeterInterval;
UINT	CPreferences::statsInterval;
bool	CPreferences::m_bFillGraphs;
uchar	CPreferences::userhash[16];
WINDOWPLACEMENT CPreferences::EmuleWindowPlacement;
int		CPreferences::maxGraphDownloadRate;
int		CPreferences::maxGraphUploadRate;
uint32	CPreferences::maxGraphUploadRateEstimated = 0;
bool	CPreferences::beepOnError;
bool	CPreferences::m_bIconflashOnNewMessage;
bool	CPreferences::confirmExit;
DWORD	CPreferences::m_adwStatsColors[15];
bool	CPreferences::bHasCustomTaskIconColor;
bool	CPreferences::splashscreen;
bool	CPreferences::filterLANIPs;
bool	CPreferences::m_bAllocLocalHostIP;
bool	CPreferences::onlineSig;
uint64	CPreferences::cumDownOverheadTotal;
uint64	CPreferences::cumDownOverheadFileReq;
uint64	CPreferences::cumDownOverheadSrcEx;
uint64	CPreferences::cumDownOverheadServer;
uint64	CPreferences::cumDownOverheadKad;
uint64	CPreferences::cumDownOverheadTotalPackets;
uint64	CPreferences::cumDownOverheadFileReqPackets;
uint64	CPreferences::cumDownOverheadSrcExPackets;
uint64	CPreferences::cumDownOverheadServerPackets;
uint64	CPreferences::cumDownOverheadKadPackets;
uint64	CPreferences::cumUpOverheadTotal;
uint64	CPreferences::cumUpOverheadFileReq;
uint64	CPreferences::cumUpOverheadSrcEx;
uint64	CPreferences::cumUpOverheadServer;
uint64	CPreferences::cumUpOverheadKad;
uint64	CPreferences::cumUpOverheadTotalPackets;
uint64	CPreferences::cumUpOverheadFileReqPackets;
uint64	CPreferences::cumUpOverheadSrcExPackets;
uint64	CPreferences::cumUpOverheadServerPackets;
uint64	CPreferences::cumUpOverheadKadPackets;
uint32	CPreferences::cumUpSuccessfulSessions;
uint32	CPreferences::cumUpFailedSessions;
uint32	CPreferences::cumUpAvgTime;
uint64	CPreferences::cumUpData_EDONKEY;
uint64	CPreferences::cumUpData_EDONKEYHYBRID;
uint64	CPreferences::cumUpData_EMULE;
uint64	CPreferences::cumUpData_MLDONKEY;
uint64	CPreferences::cumUpData_AMULE;
uint64	CPreferences::cumUpData_EMULECOMPAT;
uint64	CPreferences::cumUpData_SHAREAZA;
uint64	CPreferences::sesUpData_EDONKEY;
uint64	CPreferences::sesUpData_EDONKEYHYBRID;
uint64	CPreferences::sesUpData_EMULE;
uint64	CPreferences::sesUpData_MLDONKEY;
uint64	CPreferences::sesUpData_AMULE;
uint64	CPreferences::sesUpData_EMULECOMPAT;
uint64	CPreferences::sesUpData_SHAREAZA;
uint64	CPreferences::cumUpDataPort_4662;
uint64	CPreferences::cumUpDataPort_OTHER;
uint64	CPreferences::cumUpDataPort_PeerCache;
uint64	CPreferences::sesUpDataPort_4662;
uint64	CPreferences::sesUpDataPort_OTHER;
uint64	CPreferences::sesUpDataPort_PeerCache;
uint64	CPreferences::cumUpData_File;
uint64	CPreferences::cumUpData_Partfile;
uint64	CPreferences::sesUpData_File;
uint64	CPreferences::sesUpData_Partfile;
uint32	CPreferences::cumDownCompletedFiles;
uint32	CPreferences::cumDownSuccessfulSessions;
uint32	CPreferences::cumDownFailedSessions;
uint32	CPreferences::cumDownAvgTime;
uint64	CPreferences::cumLostFromCorruption;
uint64	CPreferences::cumSavedFromCompression;
uint32	CPreferences::cumPartsSavedByICH;
uint32	CPreferences::sesDownSuccessfulSessions;
uint32	CPreferences::sesDownFailedSessions;
uint32	CPreferences::sesDownAvgTime;
uint32	CPreferences::sesDownCompletedFiles;
uint64	CPreferences::sesLostFromCorruption;
uint64	CPreferences::sesSavedFromCompression;
uint32	CPreferences::sesPartsSavedByICH;
uint64	CPreferences::cumDownData_EDONKEY;
uint64	CPreferences::cumDownData_EDONKEYHYBRID;
uint64	CPreferences::cumDownData_EMULE;
uint64	CPreferences::cumDownData_MLDONKEY;
uint64	CPreferences::cumDownData_AMULE;
uint64	CPreferences::cumDownData_EMULECOMPAT;
uint64	CPreferences::cumDownData_SHAREAZA;
uint64	CPreferences::cumDownData_URL;
uint64	CPreferences::sesDownData_EDONKEY;
uint64	CPreferences::sesDownData_EDONKEYHYBRID;
uint64	CPreferences::sesDownData_EMULE;
uint64	CPreferences::sesDownData_MLDONKEY;
uint64	CPreferences::sesDownData_AMULE;
uint64	CPreferences::sesDownData_EMULECOMPAT;
uint64	CPreferences::sesDownData_SHAREAZA;
uint64	CPreferences::sesDownData_URL;
uint64	CPreferences::cumDownDataPort_4662;
uint64	CPreferences::cumDownDataPort_OTHER;
uint64	CPreferences::cumDownDataPort_PeerCache;
uint64	CPreferences::sesDownDataPort_4662;
uint64	CPreferences::sesDownDataPort_OTHER;
uint64	CPreferences::sesDownDataPort_PeerCache;
float	CPreferences::cumConnAvgDownRate;
float	CPreferences::cumConnMaxAvgDownRate;
float	CPreferences::cumConnMaxDownRate;
float	CPreferences::cumConnAvgUpRate;
float	CPreferences::cumConnMaxAvgUpRate;
float	CPreferences::cumConnMaxUpRate;
time_t	CPreferences::cumConnRunTime;
uint32	CPreferences::cumConnNumReconnects;
uint32	CPreferences::cumConnAvgConnections;
uint32	CPreferences::cumConnMaxConnLimitReached;
uint32	CPreferences::cumConnPeakConnections;
uint32	CPreferences::cumConnTransferTime;
uint32	CPreferences::cumConnDownloadTime;
uint32	CPreferences::cumConnUploadTime;
uint32	CPreferences::cumConnServerDuration;
uint32	CPreferences::cumSrvrsMostWorkingServers;
uint32	CPreferences::cumSrvrsMostUsersOnline;
uint32	CPreferences::cumSrvrsMostFilesAvail;
uint32	CPreferences::cumSharedMostFilesShared;
uint64	CPreferences::cumSharedLargestShareSize;
uint64	CPreferences::cumSharedLargestAvgFileSize;
uint64	CPreferences::cumSharedLargestFileSize;
time_t	CPreferences::stat_datetimeLastReset;
UINT	CPreferences::statsConnectionsGraphRatio;
UINT	CPreferences::statsSaveInterval;
CString	CPreferences::m_strStatsExpandedTreeItems;
bool	CPreferences::m_bShowVerticalHourMarkers;
uint64	CPreferences::totalDownloadedBytes;
uint64	CPreferences::totalUploadedBytes;
WORD	CPreferences::m_wLanguageID;
bool	CPreferences::transferDoubleclick;
EViewSharedFilesAccess CPreferences::m_iSeeShares;
UINT	CPreferences::m_iToolDelayTime;
bool	CPreferences::bringtoforeground;
UINT	CPreferences::splitterbarPosition;
UINT	CPreferences::splitterbarPositionSvr;
UINT	CPreferences::splitterbarPositionStat;
UINT	CPreferences::splitterbarPositionStat_HL;
UINT	CPreferences::splitterbarPositionStat_HR;
UINT	CPreferences::splitterbarPositionFriend;
UINT	CPreferences::splitterbarPositionIRC;
UINT	CPreferences::splitterbarPositionShared;
UINT	CPreferences::m_uTransferWnd1;
UINT	CPreferences::m_uTransferWnd2;
UINT	CPreferences::m_uDeadServerRetries;
DWORD	CPreferences::m_dwServerKeepAliveTimeout;
UINT	CPreferences::statsMax;
UINT	CPreferences::statsAverageMinutes;
CString	CPreferences::notifierConfiguration;
bool	CPreferences::notifierOnDownloadFinished;
bool	CPreferences::notifierOnNewDownload;
bool	CPreferences::notifierOnChat;
bool	CPreferences::notifierOnLog;
bool	CPreferences::notifierOnImportantError;
bool	CPreferences::notifierOnEveryChatMsg;
bool	CPreferences::notifierOnNewVersion;
ENotifierSoundType CPreferences::notifierSoundType = ntfstNoSound;
CString	CPreferences::notifierSoundFile;
CString CPreferences::m_strIRCServer;
CString	CPreferences::m_strIRCNick;
CString	CPreferences::m_strIRCChannelFilter;
bool	CPreferences::m_bIRCAddTimeStamp;
bool	CPreferences::m_bIRCUseChannelFilter;
UINT	CPreferences::m_uIRCChannelUserFilter;
CString	CPreferences::m_strIRCPerformString;
bool	CPreferences::m_bIRCUsePerform;
bool	CPreferences::m_bIRCGetChannelsOnConnect;
bool	CPreferences::m_bIRCAcceptLinks;
bool	CPreferences::m_bIRCAcceptLinksFriendsOnly;
bool	CPreferences::m_bIRCPlaySoundEvents;
bool	CPreferences::m_bIRCIgnoreMiscMessages;
bool	CPreferences::m_bIRCIgnoreJoinMessages;
bool	CPreferences::m_bIRCIgnorePartMessages;
bool	CPreferences::m_bIRCIgnoreQuitMessages;
bool	CPreferences::m_bIRCIgnoreEmuleAddFriendMsgs;
bool	CPreferences::m_bIRCAllowEmuleAddFriend;
bool	CPreferences::m_bIRCIgnoreEmuleSendLinkMsgs;
bool	CPreferences::m_bIRCJoinHelpChannel;
bool	CPreferences::m_bRemove2bin;
bool	CPreferences::m_bShowCopyEd2kLinkCmd;
bool	CPreferences::m_bpreviewprio;
bool	CPreferences::m_bSmartServerIdCheck;
uint8	CPreferences::smartidstate;
bool	CPreferences::m_bSafeServerConnect;
bool	CPreferences::startMinimized;
bool	CPreferences::m_bAutoStart;
bool	CPreferences::m_bRestoreLastMainWndDlg;
int		CPreferences::m_iLastMainWndDlgID;
bool	CPreferences::m_bRestoreLastLogPane;
int		CPreferences::m_iLastLogPaneID;
UINT	CPreferences::MaxConperFive;
bool	CPreferences::checkDiskspace;
UINT	CPreferences::m_uMinFreeDiskSpace;
bool	CPreferences::m_bSparsePartFiles;
CString	CPreferences::m_strYourHostname;
bool	CPreferences::m_bEnableVerboseOptions;
bool	CPreferences::m_bVerbose;
bool	CPreferences::m_bFullVerbose;
bool	CPreferences::m_bDebugSourceExchange;
bool	CPreferences::m_bLogBannedClients;
bool	CPreferences::m_bLogRatingDescReceived;
bool	CPreferences::m_bLogSecureIdent;
bool	CPreferences::m_bLogFilteredIPs;
bool	CPreferences::m_bLogFileSaving;
bool	CPreferences::m_bLogA4AF; // ZZ:DownloadManager
bool	CPreferences::m_bLogUlDlEvents;
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
bool	CPreferences::m_bUseDebugDevice = true;
#else
bool	CPreferences::m_bUseDebugDevice = false;
#endif
int		CPreferences::m_iDebugServerTCPLevel;
int		CPreferences::m_iDebugServerUDPLevel;
int		CPreferences::m_iDebugServerSourcesLevel;
int		CPreferences::m_iDebugServerSearchesLevel;
int		CPreferences::m_iDebugClientTCPLevel;
int		CPreferences::m_iDebugClientUDPLevel;
int		CPreferences::m_iDebugClientKadUDPLevel;
int		CPreferences::m_iDebugSearchResultDetailLevel;
bool	CPreferences::m_bupdatequeuelist;
bool	CPreferences::m_bManualAddedServersHighPriority;
bool	CPreferences::m_btransferfullchunks;
int		CPreferences::m_istartnextfile;
bool	CPreferences::m_bshowoverhead;
bool	CPreferences::m_bDAP;
bool	CPreferences::m_bUAP;
bool	CPreferences::m_bDisableKnownClientList;
bool	CPreferences::m_bDisableQueueList;
bool	CPreferences::m_bExtControls;
bool	CPreferences::m_bTransflstRemain;
UINT	CPreferences::versioncheckdays;
bool	CPreferences::showRatesInTitle;
CString	CPreferences::m_strTxtEditor;
CString	CPreferences::m_strVideoPlayer;
CString CPreferences::m_strVideoPlayerArgs;
bool	CPreferences::moviePreviewBackup;
int		CPreferences::m_iPreviewSmallBlocks;
bool	CPreferences::m_bPreviewCopiedArchives;
int		CPreferences::m_iInspectAllFileTypes;
bool	CPreferences::m_bPreviewOnIconDblClk;
bool	CPreferences::m_bCheckFileOpen;
bool	CPreferences::indicateratings;
bool	CPreferences::watchclipboard;
bool	CPreferences::filterserverbyip;
bool	CPreferences::m_bFirstStart;
bool	CPreferences::m_bBetaNaggingDone;
bool	CPreferences::m_bCreditSystem;
bool	CPreferences::log2disk;
bool	CPreferences::debug2disk;
int		CPreferences::iMaxLogBuff;
UINT	CPreferences::uMaxLogFileSize;
ELogFileFormat CPreferences::m_iLogFileFormat = Unicode;
bool	CPreferences::scheduler;
bool	CPreferences::dontcompressavi;
bool	CPreferences::msgonlyfriends;
bool	CPreferences::msgsecure;
bool	CPreferences::m_bUseChatCaptchas;
UINT	CPreferences::filterlevel;
UINT	CPreferences::m_iFileBufferSize;
UINT	CPreferences::m_uFileBufferTimeLimit;
UINT	CPreferences::m_iQueueSize;
int		CPreferences::m_iCommitFiles;
UINT	CPreferences::maxmsgsessions;
uint32	CPreferences::versioncheckLastAutomatic;
CString	CPreferences::messageFilter;
CString	CPreferences::commentFilter;
CString	CPreferences::filenameCleanups;
CString	CPreferences::m_strDateTimeFormat;
CString	CPreferences::m_strDateTimeFormat4Log;
CString	CPreferences::m_strDateTimeFormat4Lists;
LOGFONT CPreferences::m_lfHyperText;
LOGFONT CPreferences::m_lfLogText;
COLORREF CPreferences::m_crLogError = RGB(255, 0, 0);
COLORREF CPreferences::m_crLogWarning = RGB(128, 0, 128);
COLORREF CPreferences::m_crLogSuccess = RGB(0, 0, 255);
int		CPreferences::m_iExtractMetaData;
bool	CPreferences::m_bAdjustNTFSDaylightFileTime = true;
bool	CPreferences::m_bRearrangeKadSearchKeywords;
CString	CPreferences::m_strWebPassword;
CString	CPreferences::m_strWebLowPassword;
CUIntArray CPreferences::m_aAllowedRemoteAccessIPs;
uint16	CPreferences::m_nWebPort;
bool	CPreferences::m_bWebUseUPnP;
bool	CPreferences::m_bWebEnabled;
bool	CPreferences::m_bWebUseGzip;
int		CPreferences::m_nWebPageRefresh;
bool	CPreferences::m_bWebLowEnabled;
int		CPreferences::m_iWebTimeoutMins;
int		CPreferences::m_iWebFileUploadSizeLimitMB;
bool	CPreferences::m_bAllowAdminHiLevFunc;
CString	CPreferences::m_strTemplateFile;
ProxySettings CPreferences::proxy;
bool	CPreferences::showCatTabInfos;
bool	CPreferences::resumeSameCat;
bool	CPreferences::dontRecreateGraphs;
bool	CPreferences::autofilenamecleanup;
bool	CPreferences::m_bUseAutocompl;
bool	CPreferences::m_bShowDwlPercentage;
bool	CPreferences::m_bRemoveFinishedDownloads;
UINT	CPreferences::m_iMaxChatHistory;
bool	CPreferences::m_bShowActiveDownloadsBold;
int		CPreferences::m_iSearchMethod;
bool	CPreferences::m_bAdvancedSpamfilter;
bool	CPreferences::m_bUseSecureIdent;
CString	CPreferences::m_strMMPassword;
bool	CPreferences::m_bMMEnabled;
uint16	CPreferences::m_nMMPort;
bool	CPreferences::networkkademlia;
bool	CPreferences::networked2k;
EToolbarLabelType CPreferences::m_nToolbarLabels;
CString	CPreferences::m_sToolbarBitmap;
CString	CPreferences::m_sToolbarBitmapFolder;
CString	CPreferences::m_sToolbarSettings;
bool	CPreferences::m_bReBarToolbar;
CSize	CPreferences::m_sizToolbarIconSize;
bool	CPreferences::m_bPreviewEnabled;
bool	CPreferences::m_bAutomaticArcPreviewStart;
bool	CPreferences::m_bDynUpEnabled;
int		CPreferences::m_iDynUpPingTolerance;
int		CPreferences::m_iDynUpGoingUpDivider;
int		CPreferences::m_iDynUpGoingDownDivider;
int		CPreferences::m_iDynUpNumberOfPings;
int		CPreferences::m_iDynUpPingToleranceMilliseconds;
bool	CPreferences::m_bDynUpUseMillisecondPingTolerance;
bool    CPreferences::m_bAllocFull;
bool	CPreferences::m_bShowSharedFilesDetails;
bool	CPreferences::m_bShowUpDownIconInTaskbar;
bool	CPreferences::m_bShowWin7TaskbarGoodies;
bool	CPreferences::m_bForceSpeedsToKB;
bool	CPreferences::m_bAutoShowLookups;
bool	CPreferences::m_bExtraPreviewWithMenu;

// ZZ:DownloadManager -->
bool    CPreferences::m_bA4AFSaveCpu;
// ZZ:DownloadManager <--
bool    CPreferences::m_bHighresTimer;
bool	CPreferences::m_bResolveSharedShellLinks;
bool	CPreferences::m_bKeepUnavailableFixedSharedDirs;
CStringList CPreferences::shareddir_list;
CStringList CPreferences::addresses_list;
CString CPreferences::m_strFileCommentsFilePath;
Preferences_Ext_Struct* CPreferences::prefsExt;
WORD	CPreferences::m_wWinVer;
CArray<Category_Struct*,Category_Struct*> CPreferences::catMap;
UINT	CPreferences::m_nWebMirrorAlertLevel;
bool	CPreferences::m_bRunAsUser;
bool	CPreferences::m_bPreferRestrictedOverUser;
bool	CPreferences::m_bUseOldTimeRemaining;
uint32	CPreferences::m_uPeerCacheLastSearch;
bool	CPreferences::m_bPeerCacheWasFound;
bool	CPreferences::m_bPeerCacheEnabled;
uint16	CPreferences::m_nPeerCachePort;
bool	CPreferences::m_bPeerCacheShow;

bool	CPreferences::m_bOpenPortsOnStartUp;
int		CPreferences::m_byLogLevel;
bool	CPreferences::m_bTrustEveryHash;
bool	CPreferences::m_bRememberCancelledFiles;
bool	CPreferences::m_bRememberDownloadedFiles;
bool	CPreferences::m_bPartiallyPurgeOldKnownFiles;

bool	CPreferences::m_bNotifierSendMail;
CString	CPreferences::m_strNotifierMailServer;
CString	CPreferences::m_strNotifierMailSender;
CString	CPreferences::m_strNotifierMailReceiver;

bool	CPreferences::m_bWinaTransToolbar;
bool	CPreferences::m_bShowDownloadToolbar;

bool	CPreferences::m_bCryptLayerRequested;
bool	CPreferences::m_bCryptLayerSupported;
bool	CPreferences::m_bCryptLayerRequired;
uint32	CPreferences::m_dwKadUDPKey;
uint8	CPreferences::m_byCryptTCPPaddingLength;

bool	CPreferences::m_bSkipWANIPSetup;
bool	CPreferences::m_bSkipWANPPPSetup;
bool	CPreferences::m_bEnableUPnP;
bool	CPreferences::m_bCloseUPnPOnExit;
bool	CPreferences::m_bIsWinServImplDisabled;
bool	CPreferences::m_bIsMinilibImplDisabled;
int		CPreferences::m_nLastWorkingImpl;

bool	CPreferences::m_bEnableSearchResultFilter;

bool	CPreferences::m_bIRCEnableSmileys;
bool	CPreferences::m_bMessageEnableSmileys;

BOOL	CPreferences::m_bIsRunningAeroGlass;
bool	CPreferences::m_bPreventStandby;
bool	CPreferences::m_bStoreSearches;

CPreferences::CPreferences()
{
#ifdef _DEBUG
	m_iDbgHeap = 1;
#endif
}

CPreferences::~CPreferences()
{
	delete prefsExt;
}

LPCTSTR CPreferences::GetConfigFile()
{
	return theApp.m_pszProfileName;
}

void CPreferences::Init()
{
	srand((uint32)time(0)); // we need random numbers sometimes

	prefsExt = new Preferences_Ext_Struct;
	memset(prefsExt, 0, sizeof *prefsExt);

	m_strFileCommentsFilePath = GetMuleDirectory(EMULE_CONFIGDIR) + L"fileinfo.ini";

	///////////////////////////////////////////////////////////////////////////
	// Move *.log files from application directory into 'log' directory
	//
	CFileFind ff;
	BOOL bFoundFile = ff.FindFile(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"eMule*.log", 0);
	while (bFoundFile)
	{
		bFoundFile = ff.FindNextFile();
		if (ff.IsDots() || ff.IsSystem() || ff.IsDirectory() || ff.IsHidden())
			continue;
		MoveFile(ff.GetFilePath(), GetMuleDirectory(EMULE_LOGDIR) + ff.GetFileName());
	}
	ff.Close();

	///////////////////////////////////////////////////////////////////////////
	// Move 'downloads.txt/bak' files from application and/or data-base directory
	// into 'config' directory
	//
	if (PathFileExists(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.txt"))
		MoveFile(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.txt", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.txt");
	if (PathFileExists(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.bak"))
		MoveFile(GetMuleDirectory(EMULE_DATABASEDIR) + L"downloads.bak", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.bak");
	if (PathFileExists(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.txt"))
		MoveFile(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.txt", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.txt");
	if (PathFileExists(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.bak"))
		MoveFile(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"downloads.bak", GetMuleDirectory(EMULE_CONFIGDIR) + L"downloads.bak");

	CreateUserHash();

	// load preferences.dat or set standart values
	CString strFullPath;
	strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"preferences.dat";
	FILE* preffile = _tfsopen(strFullPath, L"rb", _SH_DENYWR);

	LoadPreferences();

	if (!preffile){
		SetStandartValues();
	}
	else{
		if (fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile) != 1 || ferror(preffile))
			SetStandartValues();

		md4cpy(userhash, prefsExt->userhash);
		EmuleWindowPlacement = prefsExt->EmuleWindowPlacement;

		fclose(preffile);
		smartidstate = 0;
	}

	// shared directories
	strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"shareddir.dat";
	CStdioFile* sdirfile = new CStdioFile();
	bool bIsUnicodeFile = IsUnicodeFile(strFullPath); // check for BOM
	// open the text file either in ANSI (text) or Unicode (binary), this way we can read old and new files
	// with nearly the same code..
	if (sdirfile->Open(strFullPath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
	{
		try {
			if (bIsUnicodeFile)
				sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

			CString toadd;
			while (sdirfile->ReadString(toadd))
			{
				toadd.Trim(L" \t\r\n"); // need to trim '\r' in binary mode
				if (toadd.IsEmpty())
					continue;

				TCHAR szFullPath[MAX_PATH];
				if (PathCanonicalize(szFullPath, toadd))
					toadd = szFullPath;

				if (!IsShareableDirectory(toadd))
					continue;

				// Skip non-existing directories from fixed disks only
				int iDrive = PathGetDriveNumber(toadd);
				if (iDrive >= 0 && iDrive <= 25) {
					WCHAR szRootPath[4] = L" :\\";
					szRootPath[0] = (WCHAR)(L'A' + iDrive);
					if (GetDriveType(szRootPath) == DRIVE_FIXED && !m_bKeepUnavailableFixedSharedDirs) {
						if (_taccess(toadd, 0) != 0)
							continue;
					}
				}

				if (toadd.Right(1) != L'\\')
					toadd.Append(L"\\");
				shareddir_list.AddHead(toadd);
			}
			sdirfile->Close();
		}
		catch (CFileException* ex) {
			ASSERT(0);
			ex->Delete();
		}
	}
	delete sdirfile;

	// serverlist addresses
	// filename update to reasonable name
	if (PathFileExists(GetMuleDirectory(EMULE_CONFIGDIR) + L"adresses.dat") ) {
		if (PathFileExists(GetMuleDirectory(EMULE_CONFIGDIR) + L"addresses.dat") )
			DeleteFile(GetMuleDirectory(EMULE_CONFIGDIR) + L"adresses.dat");
		else 
			MoveFile(GetMuleDirectory(EMULE_CONFIGDIR) + L"adresses.dat", GetMuleDirectory(EMULE_CONFIGDIR) + L"addresses.dat");
	}

	strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"addresses.dat";
	sdirfile = new CStdioFile();
	bIsUnicodeFile = IsUnicodeFile(strFullPath);
	if (sdirfile->Open(strFullPath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
	{
		try {
			if (bIsUnicodeFile)
				sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

			CString toadd;
			while (sdirfile->ReadString(toadd))
			{
				toadd.Trim(L" \t\r\n"); // need to trim '\r' in binary mode
				if (toadd.IsEmpty())
					continue;
				addresses_list.AddTail(toadd);
			}
		}
		catch (CFileException* ex) {
			ASSERT(0);
			ex->Delete();
		}
		sdirfile->Close();
	}
	delete sdirfile;

	userhash[5] = 14;
	userhash[14] = 111;

	// Explicitly inform the user about errors with incoming/temp folders!
	if (!PathFileExists(GetMuleDirectory(EMULE_INCOMINGDIR)) && !::CreateDirectory(GetMuleDirectory(EMULE_INCOMINGDIR),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetMuleDirectory(EMULE_INCOMINGDIR), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);

		m_strIncomingDir = GetDefaultDirectory(EMULE_INCOMINGDIR, true); // will also try to create it if needed
		if (!PathFileExists(GetMuleDirectory(EMULE_INCOMINGDIR))){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetMuleDirectory(EMULE_INCOMINGDIR), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);

		tempdir.SetAt(0, GetDefaultDirectory(EMULE_TEMPDIR, true)); // will also try to create it if needed);
		if (!PathFileExists(GetTempDir())){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}

	// Create 'skins' directory
	if (!PathFileExists(GetMuleDirectory(EMULE_SKINDIR)) && !CreateDirectory(GetMuleDirectory(EMULE_SKINDIR), 0)) {
		m_strSkinProfileDir = GetDefaultDirectory(EMULE_SKINDIR, true); // will also try to create it if needed
	}

	// Create 'toolbars' directory
	if (!PathFileExists(GetMuleDirectory(EMULE_TOOLBARDIR)) && !CreateDirectory(GetMuleDirectory(EMULE_TOOLBARDIR), 0)) {
		m_sToolbarBitmapFolder = GetDefaultDirectory(EMULE_TOOLBARDIR, true); // will also try to create it if needed;
	}


	if (isnulmd4(userhash))
		CreateUserHash();
}

void CPreferences::Uninit()
{
	while (!catMap.IsEmpty())
	{
		Category_Struct* delcat = catMap.GetAt(0); 
		catMap.RemoveAt(0); 
		delete delcat;
	}
}

void CPreferences::SetStandartValues()
{
	CreateUserHash();

	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=0;
	EmuleWindowPlacement=defaultWPM;
	versioncheckLastAutomatic=0;

//	Save();
}

bool CPreferences::IsTempFile(const CString& rstrDirectory, const CString& rstrName)
{
	bool bFound = false;
	for (int i=0;i<tempdir.GetCount() && !bFound;i++)
		if (CompareDirectories(rstrDirectory, GetTempDir(i))==0)
			bFound = true; //ok, found a directory
	
	if(!bFound) //found nowhere - not a tempfile...
		return false;

	// do not share a file from the temp directory, if it matches one of the following patterns
	CString strNameLower(rstrName);
	strNameLower.MakeLower();
	strNameLower += L"|"; // append an EOS character which we can query for
	static const LPCTSTR _apszNotSharedExts[] = {
		L"%u.part" L"%c", 
		L"%u.part.met" L"%c", 
		L"%u.part.met" PARTMET_BAK_EXT L"%c", 
		L"%u.part.met" PARTMET_TMP_EXT L"%c" 
	};
	for (int i = 0; i < _countof(_apszNotSharedExts); i++){
		UINT uNum;
		TCHAR iChar;
		// "misuse" the 'scanf' function for a very simple pattern scanning.
		if (_stscanf(strNameLower, _apszNotSharedExts[i], &uNum, &iChar) == 2 && iChar == L'|')
			return true;
	}

	return false;
}

uint16 CPreferences::GetMaxDownload(){
    return (uint16)(GetMaxDownloadInBytesPerSec()/1024);
}

uint64 CPreferences::GetMaxDownloadInBytesPerSec(bool dynamic){
	//dont be a Lam3r :)
	UINT maxup;
	if (dynamic && thePrefs.IsDynUpEnabled() && theApp.uploadqueue->GetWaitingUserCount() != 0 && theApp.uploadqueue->GetDatarate() != 0) {
		maxup = theApp.uploadqueue->GetDatarate();
	} else {
		maxup = GetMaxUpload()*1024;
	}

	if (maxup < 4*1024)
		return (((maxup < 10*1024) && ((uint64)maxup*3 < maxdownload*1024)) ? (uint64)maxup*3 : maxdownload*1024);
	return (((maxup < 10*1024) && ((uint64)maxup*4 < maxdownload*1024)) ? (uint64)maxup*4 : maxdownload*1024);
}

// -khaos--+++> A whole bunch of methods!  Keep going until you reach the end tag.
void CPreferences::SaveStats(int bBackUp){
	// This function saves all of the new statistics in my addon.  It is also used to
	// save backups for the Reset Stats function, and the Restore Stats function (Which is actually LoadStats)
	// bBackUp = 0: DEFAULT; save to statistics.ini
	// bBackUp = 1: Save to statbkup.ini, which is used to restore after a reset
	// bBackUp = 2: Save to statbkuptmp.ini, which is temporarily created during a restore and then renamed to statbkup.ini

	CString strFullPath(GetMuleDirectory(EMULE_CONFIGDIR));
	if (bBackUp == 1)
		strFullPath += L"statbkup.ini";
	else if (bBackUp == 2)
		strFullPath += L"statbkuptmp.ini";
	else
		strFullPath += L"statistics.ini";
	
	CIni ini(strFullPath, L"Statistics");

	// Save cumulative statistics to preferences.ini, going in order as they appear in CStatisticsDlg::ShowStatistics.
	// We do NOT SET the values in prefs struct here.

    // Save Cum Down Data
	ini.WriteUInt64(L"TotalDownloadedBytes", theStats.sessionReceivedBytes + GetTotalDownloaded());
	ini.WriteInt(L"DownSuccessfulSessions", cumDownSuccessfulSessions);
	ini.WriteInt(L"DownFailedSessions", cumDownFailedSessions);
	ini.WriteInt(L"DownAvgTime", (GetDownC_AvgTime() + GetDownS_AvgTime()) / 2);
	ini.WriteUInt64(L"LostFromCorruption", cumLostFromCorruption + sesLostFromCorruption);
	ini.WriteUInt64(L"SavedFromCompression", sesSavedFromCompression + cumSavedFromCompression);
	ini.WriteInt(L"PartsSavedByICH", cumPartsSavedByICH + sesPartsSavedByICH);

	ini.WriteUInt64(L"DownData_EDONKEY", GetCumDownData_EDONKEY());
	ini.WriteUInt64(L"DownData_EDONKEYHYBRID", GetCumDownData_EDONKEYHYBRID());
	ini.WriteUInt64(L"DownData_EMULE", GetCumDownData_EMULE());
	ini.WriteUInt64(L"DownData_MLDONKEY", GetCumDownData_MLDONKEY());
	ini.WriteUInt64(L"DownData_LMULE", GetCumDownData_EMULECOMPAT());
	ini.WriteUInt64(L"DownData_AMULE", GetCumDownData_AMULE());
	ini.WriteUInt64(L"DownData_SHAREAZA", GetCumDownData_SHAREAZA());
	ini.WriteUInt64(L"DownData_URL", GetCumDownData_URL());
	ini.WriteUInt64(L"DownDataPort_4662", GetCumDownDataPort_4662());
	ini.WriteUInt64(L"DownDataPort_OTHER", GetCumDownDataPort_OTHER());
	ini.WriteUInt64(L"DownDataPort_PeerCache", GetCumDownDataPort_PeerCache());

	ini.WriteUInt64(L"DownOverheadTotal",theStats.GetDownDataOverheadFileRequest() +
										theStats.GetDownDataOverheadSourceExchange() +
										theStats.GetDownDataOverheadServer() +
										theStats.GetDownDataOverheadKad() +
										theStats.GetDownDataOverheadOther() +
										GetDownOverheadTotal());
	ini.WriteUInt64(L"DownOverheadFileReq", theStats.GetDownDataOverheadFileRequest() + GetDownOverheadFileReq());
	ini.WriteUInt64(L"DownOverheadSrcEx", theStats.GetDownDataOverheadSourceExchange() + GetDownOverheadSrcEx());
	ini.WriteUInt64(L"DownOverheadServer", theStats.GetDownDataOverheadServer() + GetDownOverheadServer());
	ini.WriteUInt64(L"DownOverheadKad", theStats.GetDownDataOverheadKad() + GetDownOverheadKad());
	
	ini.WriteUInt64(L"DownOverheadTotalPackets", theStats.GetDownDataOverheadFileRequestPackets() + 
												theStats.GetDownDataOverheadSourceExchangePackets() + 
												theStats.GetDownDataOverheadServerPackets() + 
												theStats.GetDownDataOverheadKadPackets() + 
												theStats.GetDownDataOverheadOtherPackets() + 
												GetDownOverheadTotalPackets());
	ini.WriteUInt64(L"DownOverheadFileReqPackets", theStats.GetDownDataOverheadFileRequestPackets() + GetDownOverheadFileReqPackets());
	ini.WriteUInt64(L"DownOverheadSrcExPackets", theStats.GetDownDataOverheadSourceExchangePackets() + GetDownOverheadSrcExPackets());
	ini.WriteUInt64(L"DownOverheadServerPackets", theStats.GetDownDataOverheadServerPackets() + GetDownOverheadServerPackets());
	ini.WriteUInt64(L"DownOverheadKadPackets", theStats.GetDownDataOverheadKadPackets() + GetDownOverheadKadPackets());

	// Save Cumulative Upline Statistics
	ini.WriteUInt64(L"TotalUploadedBytes", theStats.sessionSentBytes + GetTotalUploaded());
	ini.WriteInt(L"UpSuccessfulSessions", theApp.uploadqueue->GetSuccessfullUpCount() + GetUpSuccessfulSessions());
	ini.WriteInt(L"UpFailedSessions", theApp.uploadqueue->GetFailedUpCount() + GetUpFailedSessions());
	ini.WriteInt(L"UpAvgTime", (theApp.uploadqueue->GetAverageUpTime() + GetUpAvgTime())/2);
	ini.WriteUInt64(L"UpData_EDONKEY", GetCumUpData_EDONKEY());
	ini.WriteUInt64(L"UpData_EDONKEYHYBRID", GetCumUpData_EDONKEYHYBRID());
	ini.WriteUInt64(L"UpData_EMULE", GetCumUpData_EMULE());
	ini.WriteUInt64(L"UpData_MLDONKEY", GetCumUpData_MLDONKEY());
	ini.WriteUInt64(L"UpData_LMULE", GetCumUpData_EMULECOMPAT());
	ini.WriteUInt64(L"UpData_AMULE", GetCumUpData_AMULE());
	ini.WriteUInt64(L"UpData_SHAREAZA", GetCumUpData_SHAREAZA());
	ini.WriteUInt64(L"UpDataPort_4662", GetCumUpDataPort_4662());
	ini.WriteUInt64(L"UpDataPort_OTHER", GetCumUpDataPort_OTHER());
	ini.WriteUInt64(L"UpDataPort_PeerCache", GetCumUpDataPort_PeerCache());
	ini.WriteUInt64(L"UpData_File", GetCumUpData_File());
	ini.WriteUInt64(L"UpData_Partfile", GetCumUpData_Partfile());

	ini.WriteUInt64(L"UpOverheadTotal", theStats.GetUpDataOverheadFileRequest() + 
										theStats.GetUpDataOverheadSourceExchange() + 
										theStats.GetUpDataOverheadServer() + 
										theStats.GetUpDataOverheadKad() + 
										theStats.GetUpDataOverheadOther() + 
										GetUpOverheadTotal());
	ini.WriteUInt64(L"UpOverheadFileReq", theStats.GetUpDataOverheadFileRequest() + GetUpOverheadFileReq());
	ini.WriteUInt64(L"UpOverheadSrcEx", theStats.GetUpDataOverheadSourceExchange() + GetUpOverheadSrcEx());
	ini.WriteUInt64(L"UpOverheadServer", theStats.GetUpDataOverheadServer() + GetUpOverheadServer());
	ini.WriteUInt64(L"UpOverheadKad", theStats.GetUpDataOverheadKad() + GetUpOverheadKad());

	ini.WriteUInt64(L"UpOverheadTotalPackets", theStats.GetUpDataOverheadFileRequestPackets() + 
										theStats.GetUpDataOverheadSourceExchangePackets() + 
										theStats.GetUpDataOverheadServerPackets() + 
										theStats.GetUpDataOverheadKadPackets() + 
										theStats.GetUpDataOverheadOtherPackets() + 
										GetUpOverheadTotalPackets());
	ini.WriteUInt64(L"UpOverheadFileReqPackets", theStats.GetUpDataOverheadFileRequestPackets() + GetUpOverheadFileReqPackets());
	ini.WriteUInt64(L"UpOverheadSrcExPackets", theStats.GetUpDataOverheadSourceExchangePackets() + GetUpOverheadSrcExPackets());
	ini.WriteUInt64(L"UpOverheadServerPackets", theStats.GetUpDataOverheadServerPackets() + GetUpOverheadServerPackets());
	ini.WriteUInt64(L"UpOverheadKadPackets", theStats.GetUpDataOverheadKadPackets() + GetUpOverheadKadPackets());

	// Save Cumulative Connection Statistics
	float tempRate = 0.0F;

	// Download Rate Average
	tempRate = theStats.GetAvgDownloadRate(AVG_TOTAL);
	ini.WriteFloat(L"ConnAvgDownRate", tempRate);
	
	// Max Download Rate Average
	if (tempRate > GetConnMaxAvgDownRate())
		SetConnMaxAvgDownRate(tempRate);
	ini.WriteFloat(L"ConnMaxAvgDownRate", GetConnMaxAvgDownRate());
	
	// Max Download Rate
	tempRate = (float)theApp.downloadqueue->GetDatarate() / 1024;
	if (tempRate > GetConnMaxDownRate())
		SetConnMaxDownRate(tempRate);
	ini.WriteFloat(L"ConnMaxDownRate", GetConnMaxDownRate());
	
	// Upload Rate Average
	tempRate = theStats.GetAvgUploadRate(AVG_TOTAL);
	ini.WriteFloat(L"ConnAvgUpRate", tempRate);
	
	// Max Upload Rate Average
	if (tempRate > GetConnMaxAvgUpRate())
		SetConnMaxAvgUpRate(tempRate);
	ini.WriteFloat(L"ConnMaxAvgUpRate", GetConnMaxAvgUpRate());
	
	// Max Upload Rate
	tempRate = (float)theApp.uploadqueue->GetDatarate() / 1024;
	if (tempRate > GetConnMaxUpRate())
		SetConnMaxUpRate(tempRate);
	ini.WriteFloat(L"ConnMaxUpRate", GetConnMaxUpRate());
	
	// Overall Run Time
	ini.WriteInt(L"ConnRunTime", (UINT)((GetTickCount() - theStats.starttime)/1000 + GetConnRunTime()));
	
	// Number of Reconnects
	ini.WriteInt(L"ConnNumReconnects", (theStats.reconnects>0) ? (theStats.reconnects - 1 + GetConnNumReconnects()) : GetConnNumReconnects());
	
	// Average Connections
	if (theApp.serverconnect->IsConnected())
		ini.WriteInt(L"ConnAvgConnections", (UINT)((theApp.listensocket->GetAverageConnections() + cumConnAvgConnections)/2));
	
	// Peak Connections
	if (theApp.listensocket->GetPeakConnections() > cumConnPeakConnections)
		cumConnPeakConnections = theApp.listensocket->GetPeakConnections();
	ini.WriteInt(L"ConnPeakConnections", cumConnPeakConnections);
	
	// Max Connection Limit Reached
	if (theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached > cumConnMaxConnLimitReached)
		ini.WriteInt(L"ConnMaxConnLimitReached", theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached);
	
	// Time Stuff...
	ini.WriteInt(L"ConnTransferTime", GetConnTransferTime() + theStats.GetTransferTime());
	ini.WriteInt(L"ConnUploadTime", GetConnUploadTime() + theStats.GetUploadTime());
	ini.WriteInt(L"ConnDownloadTime", GetConnDownloadTime() + theStats.GetDownloadTime());
	ini.WriteInt(L"ConnServerDuration", GetConnServerDuration() + theStats.GetServerDuration());
	
	// Compare and Save Server Records
	uint32 servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile;
	float servocc;
	theApp.serverlist->GetStatus(servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile, servocc);
	
	if (servtotal - servfail > cumSrvrsMostWorkingServers)
		cumSrvrsMostWorkingServers = servtotal - servfail;
	ini.WriteInt(L"SrvrsMostWorkingServers", cumSrvrsMostWorkingServers);

	if (servtuser > cumSrvrsMostUsersOnline)
		cumSrvrsMostUsersOnline = servtuser;
	ini.WriteInt(L"SrvrsMostUsersOnline", cumSrvrsMostUsersOnline);

	if (servtfile > cumSrvrsMostFilesAvail)
		cumSrvrsMostFilesAvail = servtfile;
	ini.WriteInt(L"SrvrsMostFilesAvail", cumSrvrsMostFilesAvail);

	// Compare and Save Shared File Records
	if ((UINT)theApp.sharedfiles->GetCount() > cumSharedMostFilesShared)
		cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	ini.WriteInt(L"SharedMostFilesShared", cumSharedMostFilesShared);

	uint64 bytesLargestFile = 0;
	uint64 allsize = theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize > cumSharedLargestShareSize)
		cumSharedLargestShareSize = allsize;
	ini.WriteUInt64(L"SharedLargestShareSize", cumSharedLargestShareSize);
	if (bytesLargestFile > cumSharedLargestFileSize)
		cumSharedLargestFileSize = bytesLargestFile;
	ini.WriteUInt64(L"SharedLargestFileSize", cumSharedLargestFileSize);

	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint > cumSharedLargestAvgFileSize)
			cumSharedLargestAvgFileSize = tempint;
	}

	ini.WriteUInt64(L"SharedLargestAvgFileSize", cumSharedLargestAvgFileSize);
	ini.WriteInt(L"statsDateTimeLastReset", stat_datetimeLastReset);

	// If we are saving a back-up or a temporary back-up, return now.
	if (bBackUp != 0)
		return;
}

void CPreferences::SetRecordStructMembers() {

	// The purpose of this function is to be called from CStatisticsDlg::ShowStatistics()
	// This was easier than making a bunch of functions to interface with the record
	// members of the prefs struct from ShowStatistics.

	// This function is going to compare current values with previously saved records, and if
	// the current values are greater, the corresponding member of prefs will be updated.
	// We will not write to INI here, because this code is going to be called a lot more often
	// than SaveStats()  - Khaos

	CString buffer;

	// Servers
	uint32 servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile;
	float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile, servocc );
	if ((servtotal-servfail)>cumSrvrsMostWorkingServers) cumSrvrsMostWorkingServers = (servtotal-servfail);
	if (servtuser>cumSrvrsMostUsersOnline) cumSrvrsMostUsersOnline = servtuser;
	if (servtfile>cumSrvrsMostFilesAvail) cumSrvrsMostFilesAvail = servtfile;

	// Shared Files
	if ((UINT)theApp.sharedfiles->GetCount() > cumSharedMostFilesShared)
		cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	uint64 bytesLargestFile = 0;
	uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize>cumSharedLargestShareSize) cumSharedLargestShareSize = allsize;
	if (bytesLargestFile>cumSharedLargestFileSize) cumSharedLargestFileSize = bytesLargestFile;
	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint>cumSharedLargestAvgFileSize) cumSharedLargestAvgFileSize = tempint;
	}
} // SetRecordStructMembers()

void CPreferences::SaveCompletedDownloadsStat(){

	// This function saves the values for the completed
	// download members to INI.  It is called from
	// CPartfile::PerformFileComplete ...   - Khaos

	CIni ini(GetMuleDirectory(EMULE_CONFIGDIR) + L"statistics.ini", L"Statistics" );

	ini.WriteInt(L"DownCompletedFiles",			GetDownCompletedFiles());
	ini.WriteInt(L"DownSessionCompletedFiles",	GetDownSessionCompletedFiles());
} // SaveCompletedDownloadsStat()

void CPreferences::Add2SessionTransferData(UINT uClientID, UINT uClientPort, BOOL bFromPF, 
										   BOOL bUpDown, uint32 bytes, bool sentToFriend)
{
	//	This function adds the transferred bytes to the appropriate variables,
	//	as well as to the totals for all clients. - Khaos
	//	PARAMETERS:
	//	uClientID - The identifier for which client software sent or received this data, eg SO_EMULE
	//	uClientPort - The remote port of the client that sent or received this data, eg 4662
	//	bFromPF - Applies only to uploads.  True is from partfile, False is from non-partfile.
	//	bUpDown - True is Up, False is Down
	//	bytes - Number of bytes sent by the client.  Subtract header before calling.

	switch (bUpDown){
		case true:
			//	Upline Data
			switch (uClientID){
				// Update session client breakdown stats for sent bytes...
				case SO_EMULE:
				case SO_OLDEMULE:		sesUpData_EMULE+=bytes;			break;
				case SO_EDONKEYHYBRID:	sesUpData_EDONKEYHYBRID+=bytes;	break;
				case SO_EDONKEY:		sesUpData_EDONKEY+=bytes;		break;
				case SO_MLDONKEY:		sesUpData_MLDONKEY+=bytes;		break;
				case SO_AMULE:			sesUpData_AMULE+=bytes;			break;
				case SO_SHAREAZA:		sesUpData_SHAREAZA+=bytes;		break;
				case SO_CDONKEY:
				case SO_LPHANT:
				case SO_XMULE:			sesUpData_EMULECOMPAT+=bytes;	break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for sent bytes...
				case 4662:				sesUpDataPort_4662+=bytes;		break;
				case (UINT)-1:			sesUpDataPort_PeerCache+=bytes;	break;
				//case (UINT)-2:		sesUpDataPort_URL+=bytes;		break;
				default:				sesUpDataPort_OTHER+=bytes;		break;
			}

			if (bFromPF)				sesUpData_Partfile+=bytes;
			else						sesUpData_File+=bytes;

			//	Add to our total for sent bytes...
			theApp.UpdateSentBytes(bytes, sentToFriend);

			break;

		case false:
			// Downline Data
			switch (uClientID){
                // Update session client breakdown stats for received bytes...
				case SO_EMULE:
				case SO_OLDEMULE:		sesDownData_EMULE+=bytes;		break;
				case SO_EDONKEYHYBRID:	sesDownData_EDONKEYHYBRID+=bytes;break;
				case SO_EDONKEY:		sesDownData_EDONKEY+=bytes;		break;
				case SO_MLDONKEY:		sesDownData_MLDONKEY+=bytes;	break;
				case SO_AMULE:			sesDownData_AMULE+=bytes;		break;
				case SO_SHAREAZA:		sesDownData_SHAREAZA+=bytes;	break;
				case SO_CDONKEY:
				case SO_LPHANT:
				case SO_XMULE:			sesDownData_EMULECOMPAT+=bytes;	break;
				case SO_URL:			sesDownData_URL+=bytes;			break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for received bytes...
				// For now we are only going to break it down by default and non-default.
				// A statistical analysis of all data sent from every single port/domain is
				// beyond the scope of this add-on.
				case 4662:				sesDownDataPort_4662+=bytes;	break;
				case (UINT)-1:			sesDownDataPort_PeerCache+=bytes;break;
				//case (UINT)-2:		sesDownDataPort_URL+=bytes;		break;
				default:				sesDownDataPort_OTHER+=bytes;	break;
			}

			//	Add to our total for received bytes...
			theApp.UpdateReceivedBytes(bytes);
	}
}

// Reset Statistics by Khaos

void CPreferences::ResetCumulativeStatistics(){

	// Save a backup so that we can undo this action
	SaveStats(1);

	// SET ALL CUMULATIVE STAT VALUES TO 0  :'-(

	totalDownloadedBytes=0;
	totalUploadedBytes=0;
	cumDownOverheadTotal=0;
	cumDownOverheadFileReq=0;
	cumDownOverheadSrcEx=0;
	cumDownOverheadServer=0;
	cumDownOverheadKad=0;
	cumDownOverheadTotalPackets=0;
	cumDownOverheadFileReqPackets=0;
	cumDownOverheadSrcExPackets=0;
	cumDownOverheadServerPackets=0;
	cumDownOverheadKadPackets=0;
	cumUpOverheadTotal=0;
	cumUpOverheadFileReq=0;
	cumUpOverheadSrcEx=0;
	cumUpOverheadServer=0;
	cumUpOverheadKad=0;
	cumUpOverheadTotalPackets=0;
	cumUpOverheadFileReqPackets=0;
	cumUpOverheadSrcExPackets=0;
	cumUpOverheadServerPackets=0;
	cumUpOverheadKadPackets=0;
	cumUpSuccessfulSessions=0;
	cumUpFailedSessions=0;
	cumUpAvgTime=0;
	cumUpData_EDONKEY=0;
	cumUpData_EDONKEYHYBRID=0;
	cumUpData_EMULE=0;
	cumUpData_MLDONKEY=0;
	cumUpData_AMULE=0;
	cumUpData_EMULECOMPAT=0;
	cumUpData_SHAREAZA=0;
	cumUpDataPort_4662=0;
	cumUpDataPort_OTHER=0;
	cumUpDataPort_PeerCache=0;
	cumDownCompletedFiles=0;
	cumDownSuccessfulSessions=0;
	cumDownFailedSessions=0;
	cumDownAvgTime=0;
	cumLostFromCorruption=0;
	cumSavedFromCompression=0;
	cumPartsSavedByICH=0;
	cumDownData_EDONKEY=0;
	cumDownData_EDONKEYHYBRID=0;
	cumDownData_EMULE=0;
	cumDownData_MLDONKEY=0;
	cumDownData_AMULE=0;
	cumDownData_EMULECOMPAT=0;
	cumDownData_SHAREAZA=0;
	cumDownData_URL=0;
	cumDownDataPort_4662=0;
	cumDownDataPort_OTHER=0;
	cumDownDataPort_PeerCache=0;
	cumConnAvgDownRate=0;
	cumConnMaxAvgDownRate=0;
	cumConnMaxDownRate=0;
	cumConnAvgUpRate=0;
	cumConnRunTime=0;
	cumConnNumReconnects=0;
	cumConnAvgConnections=0;
	cumConnMaxConnLimitReached=0;
	cumConnPeakConnections=0;
	cumConnDownloadTime=0;
	cumConnUploadTime=0;
	cumConnTransferTime=0;
	cumConnServerDuration=0;
	cumConnMaxAvgUpRate=0;
	cumConnMaxUpRate=0;
	cumSrvrsMostWorkingServers=0;
	cumSrvrsMostUsersOnline=0;
	cumSrvrsMostFilesAvail=0;
    cumSharedMostFilesShared=0;
	cumSharedLargestShareSize=0;
	cumSharedLargestAvgFileSize=0;

	// Set the time of last reset...
	time_t timeNow;
	time(&timeNow);
	stat_datetimeLastReset = timeNow;

	// Save the reset stats
	SaveStats();
	theApp.emuledlg->statisticswnd->ShowStatistics(true);
}


// Load Statistics
// This used to be integrated in LoadPreferences, but it has been altered
// so that it can be used to load the backup created when the stats are reset.
// Last Modified: 2-22-03 by Khaos
bool CPreferences::LoadStats(int loadBackUp)
{
	// loadBackUp is 0 by default
	// loadBackUp = 0: Load the stats normally like we used to do in LoadPreferences
	// loadBackUp = 1: Load the stats from statbkup.ini and create a backup of the current stats.  Also, do not initialize session variables.
	CString sINI;
	CFileFind findBackUp;

	switch (loadBackUp) {
		case 0:
			// for transition...
			if (PathFileExists(GetMuleDirectory(EMULE_CONFIGDIR) + L"statistics.ini"))
				sINI.Format(L"%sstatistics.ini", GetMuleDirectory(EMULE_CONFIGDIR));
			else
				sINI.Format(L"%spreferences.ini", GetMuleDirectory(EMULE_CONFIGDIR));
			break;
		case 1:
			sINI.Format(L"%sstatbkup.ini", GetMuleDirectory(EMULE_CONFIGDIR));
			if (!findBackUp.FindFile(sINI))
				return false;
			SaveStats(2); // Save our temp backup of current values to statbkuptmp.ini, we will be renaming it at the end of this function.
			break;
	}

	BOOL fileex = PathFileExists(sINI);
	CIni ini(sINI, L"Statistics");

	totalDownloadedBytes			= ini.GetUInt64(L"TotalDownloadedBytes");
	totalUploadedBytes				= ini.GetUInt64(L"TotalUploadedBytes");

	// Load stats for cumulative downline overhead
	cumDownOverheadTotal			= ini.GetUInt64(L"DownOverheadTotal");
	cumDownOverheadFileReq			= ini.GetUInt64(L"DownOverheadFileReq");
	cumDownOverheadSrcEx			= ini.GetUInt64(L"DownOverheadSrcEx");
	cumDownOverheadServer			= ini.GetUInt64(L"DownOverheadServer");
	cumDownOverheadKad				= ini.GetUInt64(L"DownOverheadKad");
	cumDownOverheadTotalPackets		= ini.GetUInt64(L"DownOverheadTotalPackets");
	cumDownOverheadFileReqPackets	= ini.GetUInt64(L"DownOverheadFileReqPackets");
	cumDownOverheadSrcExPackets		= ini.GetUInt64(L"DownOverheadSrcExPackets");
	cumDownOverheadServerPackets	= ini.GetUInt64(L"DownOverheadServerPackets");
	cumDownOverheadKadPackets		= ini.GetUInt64(L"DownOverheadKadPackets");

	// Load stats for cumulative upline overhead
	cumUpOverheadTotal				= ini.GetUInt64(L"UpOverHeadTotal");
	cumUpOverheadFileReq			= ini.GetUInt64(L"UpOverheadFileReq");
	cumUpOverheadSrcEx				= ini.GetUInt64(L"UpOverheadSrcEx");
	cumUpOverheadServer				= ini.GetUInt64(L"UpOverheadServer");
	cumUpOverheadKad				= ini.GetUInt64(L"UpOverheadKad");
	cumUpOverheadTotalPackets		= ini.GetUInt64(L"UpOverHeadTotalPackets");
	cumUpOverheadFileReqPackets		= ini.GetUInt64(L"UpOverheadFileReqPackets");
	cumUpOverheadSrcExPackets		= ini.GetUInt64(L"UpOverheadSrcExPackets");
	cumUpOverheadServerPackets		= ini.GetUInt64(L"UpOverheadServerPackets");
	cumUpOverheadKadPackets			= ini.GetUInt64(L"UpOverheadKadPackets");

	// Load stats for cumulative upline data
	cumUpSuccessfulSessions			= ini.GetInt(L"UpSuccessfulSessions");
	cumUpFailedSessions				= ini.GetInt(L"UpFailedSessions");
	cumUpAvgTime					= ini.GetInt(L"UpAvgTime");

	// Load cumulative client breakdown stats for sent bytes
	cumUpData_EDONKEY				= ini.GetUInt64(L"UpData_EDONKEY");
	cumUpData_EDONKEYHYBRID			= ini.GetUInt64(L"UpData_EDONKEYHYBRID");
	cumUpData_EMULE					= ini.GetUInt64(L"UpData_EMULE");
	cumUpData_MLDONKEY				= ini.GetUInt64(L"UpData_MLDONKEY");
	cumUpData_EMULECOMPAT			= ini.GetUInt64(L"UpData_LMULE");
	cumUpData_AMULE					= ini.GetUInt64(L"UpData_AMULE");
	cumUpData_SHAREAZA				= ini.GetUInt64(L"UpData_SHAREAZA");

	// Load cumulative port breakdown stats for sent bytes
	cumUpDataPort_4662				= ini.GetUInt64(L"UpDataPort_4662");
	cumUpDataPort_OTHER				= ini.GetUInt64(L"UpDataPort_OTHER");
	cumUpDataPort_PeerCache			= ini.GetUInt64(L"UpDataPort_PeerCache");

	// Load cumulative source breakdown stats for sent bytes
	cumUpData_File					= ini.GetUInt64(L"UpData_File");
	cumUpData_Partfile				= ini.GetUInt64(L"UpData_Partfile");

	// Load stats for cumulative downline data
	cumDownCompletedFiles			= ini.GetInt(L"DownCompletedFiles");
	cumDownSuccessfulSessions		= ini.GetInt(L"DownSuccessfulSessions");
	cumDownFailedSessions			= ini.GetInt(L"DownFailedSessions");
	cumDownAvgTime					= ini.GetInt(L"DownAvgTime");

	// Cumulative statistics for saved due to compression/lost due to corruption
	cumLostFromCorruption			= ini.GetUInt64(L"LostFromCorruption");
	cumSavedFromCompression			= ini.GetUInt64(L"SavedFromCompression");
	cumPartsSavedByICH				= ini.GetInt(L"PartsSavedByICH");

	// Load cumulative client breakdown stats for received bytes
	cumDownData_EDONKEY				= ini.GetUInt64(L"DownData_EDONKEY");
	cumDownData_EDONKEYHYBRID		= ini.GetUInt64(L"DownData_EDONKEYHYBRID");
	cumDownData_EMULE				= ini.GetUInt64(L"DownData_EMULE");
	cumDownData_MLDONKEY			= ini.GetUInt64(L"DownData_MLDONKEY");
	cumDownData_EMULECOMPAT			= ini.GetUInt64(L"DownData_LMULE");
	cumDownData_AMULE				= ini.GetUInt64(L"DownData_AMULE");
	cumDownData_SHAREAZA			= ini.GetUInt64(L"DownData_SHAREAZA");
	cumDownData_URL					= ini.GetUInt64(L"DownData_URL");

	// Load cumulative port breakdown stats for received bytes
	cumDownDataPort_4662			= ini.GetUInt64(L"DownDataPort_4662");
	cumDownDataPort_OTHER			= ini.GetUInt64(L"DownDataPort_OTHER");
	cumDownDataPort_PeerCache		= ini.GetUInt64(L"DownDataPort_PeerCache");

	// Load stats for cumulative connection data
	cumConnAvgDownRate				= ini.GetFloat(L"ConnAvgDownRate");
	cumConnMaxAvgDownRate			= ini.GetFloat(L"ConnMaxAvgDownRate");
	cumConnMaxDownRate				= ini.GetFloat(L"ConnMaxDownRate");
	cumConnAvgUpRate				= ini.GetFloat(L"ConnAvgUpRate");
	cumConnMaxAvgUpRate				= ini.GetFloat(L"ConnMaxAvgUpRate");
	cumConnMaxUpRate				= ini.GetFloat(L"ConnMaxUpRate");
	cumConnRunTime					= ini.GetInt(L"ConnRunTime");
	cumConnTransferTime				= ini.GetInt(L"ConnTransferTime");
	cumConnDownloadTime				= ini.GetInt(L"ConnDownloadTime");
	cumConnUploadTime				= ini.GetInt(L"ConnUploadTime");
	cumConnServerDuration			= ini.GetInt(L"ConnServerDuration");
	cumConnNumReconnects			= ini.GetInt(L"ConnNumReconnects");
	cumConnAvgConnections			= ini.GetInt(L"ConnAvgConnections");
	cumConnMaxConnLimitReached		= ini.GetInt(L"ConnMaxConnLimitReached");
	cumConnPeakConnections			= ini.GetInt(L"ConnPeakConnections");

	// Load date/time of last reset
	stat_datetimeLastReset			= ini.GetInt(L"statsDateTimeLastReset");

	// Smart Load For Restores - Don't overwrite records that are greater than the backed up ones
	if (loadBackUp == 1)
	{
		// Load records for servers / network
		if ((UINT)ini.GetInt(L"SrvrsMostWorkingServers") > cumSrvrsMostWorkingServers)
			cumSrvrsMostWorkingServers = ini.GetInt(L"SrvrsMostWorkingServers");

		if ((UINT)ini.GetInt(L"SrvrsMostUsersOnline") > cumSrvrsMostUsersOnline)
			cumSrvrsMostUsersOnline = ini.GetInt(L"SrvrsMostUsersOnline");

		if ((UINT)ini.GetInt(L"SrvrsMostFilesAvail") > cumSrvrsMostFilesAvail)
			cumSrvrsMostFilesAvail = ini.GetInt(L"SrvrsMostFilesAvail");

		// Load records for shared files
		if ((UINT)ini.GetInt(L"SharedMostFilesShared") > cumSharedMostFilesShared)
			cumSharedMostFilesShared =	ini.GetInt(L"SharedMostFilesShared");

		uint64 temp64 = ini.GetUInt64(L"SharedLargestShareSize");
		if (temp64 > cumSharedLargestShareSize)
			cumSharedLargestShareSize = temp64;

		temp64 = ini.GetUInt64(L"SharedLargestAvgFileSize");
		if (temp64 > cumSharedLargestAvgFileSize)
			cumSharedLargestAvgFileSize = temp64;

		temp64 = ini.GetUInt64(L"SharedLargestFileSize");
		if (temp64 > cumSharedLargestFileSize)
			cumSharedLargestFileSize = temp64;

		// Check to make sure the backup of the values we just overwrote exists.  If so, rename it to the backup file.
		// This allows us to undo a restore, so to speak, just in case we don't like the restored values...
		CString sINIBackUp;
		sINIBackUp.Format(L"%sstatbkuptmp.ini", GetMuleDirectory(EMULE_CONFIGDIR));
		if (findBackUp.FindFile(sINIBackUp)){
			::DeleteFile(sINI);				// Remove the backup that we just restored from
			::MoveFile(sINIBackUp, sINI);	// Rename our temporary backup to the normal statbkup.ini filename.
		}

		// Since we know this is a restore, now we should call ShowStatistics to update the data items to the new ones we just loaded.
		// Otherwise user is left waiting around for the tick counter to reach the next automatic update (Depending on setting in prefs)
		theApp.emuledlg->statisticswnd->ShowStatistics();
	}
	// Stupid Load -> Just load the values.
	else
	{
		// Load records for servers / network
		cumSrvrsMostWorkingServers	= ini.GetInt(L"SrvrsMostWorkingServers");
		cumSrvrsMostUsersOnline		= ini.GetInt(L"SrvrsMostUsersOnline");
		cumSrvrsMostFilesAvail		= ini.GetInt(L"SrvrsMostFilesAvail");

		// Load records for shared files
		cumSharedMostFilesShared	= ini.GetInt(L"SharedMostFilesShared");
		cumSharedLargestShareSize	= ini.GetUInt64(L"SharedLargestShareSize");
		cumSharedLargestAvgFileSize = ini.GetUInt64(L"SharedLargestAvgFileSize");
		cumSharedLargestFileSize	= ini.GetUInt64(L"SharedLargestFileSize");

		// Initialize new session statistic variables...
		sesDownCompletedFiles		= 0;
		
		sesUpData_EDONKEY			= 0;
		sesUpData_EDONKEYHYBRID		= 0;
		sesUpData_EMULE				= 0;
		sesUpData_MLDONKEY			= 0;
		sesUpData_AMULE				= 0;
		sesUpData_EMULECOMPAT		= 0;
		sesUpData_SHAREAZA			= 0;
		sesUpDataPort_4662			= 0;
		sesUpDataPort_OTHER			= 0;
		sesUpDataPort_PeerCache		= 0;

		sesDownData_EDONKEY			= 0;
		sesDownData_EDONKEYHYBRID	= 0;
		sesDownData_EMULE			= 0;
		sesDownData_MLDONKEY		= 0;
		sesDownData_AMULE			= 0;
		sesDownData_EMULECOMPAT		= 0;
		sesDownData_SHAREAZA		= 0;
		sesDownData_URL				= 0;
		sesDownDataPort_4662		= 0;
		sesDownDataPort_OTHER		= 0;
		sesDownDataPort_PeerCache	= 0;

		sesDownSuccessfulSessions	= 0;
		sesDownFailedSessions		= 0;
		sesPartsSavedByICH			= 0;
	}

	if (!fileex || (stat_datetimeLastReset==0 && totalDownloadedBytes==0 && totalUploadedBytes==0))
	{
		time_t timeNow;
		time(&timeNow);
		stat_datetimeLastReset = timeNow;
	}

	return true;
}

// This formats the UTC long value that is saved for stat_datetimeLastReset
// If this value is 0 (Never reset), then it returns Unknown.
CString CPreferences::GetStatsLastResetStr(bool formatLong)
{
	// formatLong dictates the format of the string returned.
	// For example...
	// true: DateTime format from the .ini
	// false: DateTime format from the .ini for the log
	CString	returnStr;
	if (GetStatsLastResetLng()) {
		tm *statsReset;
		TCHAR szDateReset[128];
		time_t lastResetDateTime = (time_t) GetStatsLastResetLng();
		statsReset = localtime(&lastResetDateTime);
		if (statsReset){
			_tcsftime(szDateReset, _countof(szDateReset), formatLong ? GetDateTimeFormat() : L"%c", statsReset);
			returnStr = szDateReset;
		}
	}
	if (returnStr.IsEmpty())
		returnStr = GetResString(IDS_UNKNOWN);
	return returnStr;
}

// <-----khaos-

bool CPreferences::Save(){

	bool error = false;
	CString strFullPath;
	strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"preferences.dat";

	FILE* preffile = _tfsopen(strFullPath, L"wb", _SH_DENYWR);
	prefsExt->version = PREFFILE_VERSION;
	if (preffile){
		prefsExt->version=PREFFILE_VERSION;
		prefsExt->EmuleWindowPlacement=EmuleWindowPlacement;
		md4cpy(prefsExt->userhash, userhash);

		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile)!=1;
		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			fflush(preffile); // flush file stream buffers to disk buffers
			(void)_commit(_fileno(preffile)); // commit disk buffers to disk
		}
		fclose(preffile);
	}
	else
		error = true;

	SavePreferences();
	SaveStats();

	strFullPath = GetMuleDirectory(EMULE_CONFIGDIR) + L"shareddir.dat";
	CStdioFile sdirfile;
	if (sdirfile.Open(strFullPath, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeBinary))
	{
		try{
			// write Unicode byte-order mark 0xFEFF
			WORD wBOM = 0xFEFF;
			sdirfile.Write(&wBOM, sizeof(wBOM));

			for (POSITION pos = shareddir_list.GetHeadPosition();pos != 0;){
				sdirfile.WriteString(shareddir_list.GetNext(pos));
				sdirfile.Write(L"\r\n", sizeof(TCHAR)*2);
			}
			if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
				sdirfile.Flush(); // flush file stream buffers to disk buffers
				if (_commit(_fileno(sdirfile.m_pStream)) != 0) // commit disk buffers to disk
					AfxThrowFileException(CFileException::hardIO, GetLastError(), sdirfile.GetFileName());
			}
			sdirfile.Close();
		}
		catch(CFileException* error){
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,_countof(buffer));
			if (thePrefs.GetVerbose())
				AddDebugLogLine(true, L"Failed to save %s - %s", strFullPath, buffer);
			error->Delete();
		}
	}
	else
		error = true;

	::CreateDirectory(GetMuleDirectory(EMULE_INCOMINGDIR), 0);
	::CreateDirectory(GetTempDir(), 0);
	return error;
}

void CPreferences::CreateUserHash()
{
	CryptoPP::AutoSeededRandomPool rng;
	rng.GenerateBlock(userhash, 16);
	// mark as emule client. that will be need in later version
	userhash[5] = 14;
	userhash[14] = 111;
}

int CPreferences::GetRecommendedMaxConnections() {
	int iRealMax = ::GetMaxWindowsTCPConnections();
	if(iRealMax == -1 || iRealMax > 520)
		return 500;

	if(iRealMax < 20)
		return iRealMax;

	if(iRealMax <= 256)
		return iRealMax - 10;

	return iRealMax - 20;
}

void CPreferences::SavePreferences()
{
	CString buffer;
	
	CIni ini(GetConfigFile(), L"eMule");
	//---
	ini.WriteString(L"AppVersion", theApp.m_strCurVersionLong);
	//---
#ifdef _BETA
	if (m_bBetaNaggingDone)
		ini.WriteString(L"BetaVersionNotified", theApp.m_strCurVersionLong);
#endif
#ifdef _DEBUG
	ini.WriteInt(L"DebugHeap", m_iDbgHeap);
#endif

	ini.WriteStringUTF8(L"Nick", strNick);
	ini.WriteString(L"IncomingDir", m_strIncomingDir);
	
	ini.WriteString(L"TempDir", tempdir.GetAt(0));

	CString tempdirs;
	for (int i=1;i<tempdir.GetCount();i++) {
		tempdirs.Append(tempdir.GetAt(i) );
		if (i+1<tempdir.GetCount())
			tempdirs.Append(L"|");
	}
	ini.WriteString(L"TempDirs", tempdirs);

    ini.WriteInt(L"MinUpload", minupload);
	ini.WriteInt(L"MaxUpload",maxupload);
	ini.WriteInt(L"MaxDownload",maxdownload);
	ini.WriteInt(L"MaxConnections",maxconnections);
	ini.WriteInt(L"MaxHalfConnections",maxhalfconnections);
	ini.WriteBool(L"ConditionalTCPAccept", m_bConditionalTCPAccept);
	ini.WriteInt(L"Port",port);
	ini.WriteInt(L"UDPPort",udpport);
	ini.WriteInt(L"ServerUDPPort", nServerUDPPort);
	ini.WriteInt(L"MaxSourcesPerFile",maxsourceperfile );
	ini.WriteWORD(L"Language",m_wLanguageID);
	ini.WriteInt(L"SeeShare",m_iSeeShares);
	ini.WriteInt(L"ToolTipDelay",m_iToolDelayTime);
	ini.WriteInt(L"StatGraphsInterval",trafficOMeterInterval);
	ini.WriteInt(L"StatsInterval",statsInterval);
	ini.WriteBool(L"StatsFillGraphs",m_bFillGraphs);
	ini.WriteInt(L"DownloadCapacity",maxGraphDownloadRate);
	ini.WriteInt(L"UploadCapacityNew",maxGraphUploadRate);
	ini.WriteInt(L"DeadServerRetry",m_uDeadServerRetries);
	ini.WriteInt(L"ServerKeepAliveTimeout",m_dwServerKeepAliveTimeout);
	ini.WriteInt(L"SplitterbarPosition",splitterbarPosition);
	ini.WriteInt(L"SplitterbarPositionServer",splitterbarPositionSvr);
	ini.WriteInt(L"SplitterbarPositionStat",splitterbarPositionStat+1);
	ini.WriteInt(L"SplitterbarPositionStat_HL",splitterbarPositionStat_HL+1);
	ini.WriteInt(L"SplitterbarPositionStat_HR",splitterbarPositionStat_HR+1);
	ini.WriteInt(L"SplitterbarPositionFriend",splitterbarPositionFriend);
	ini.WriteInt(L"SplitterbarPositionIRC",splitterbarPositionIRC);
	ini.WriteInt(L"SplitterbarPositionShared",splitterbarPositionShared);
	ini.WriteInt(L"TransferWnd1",m_uTransferWnd1);
	ini.WriteInt(L"TransferWnd2",m_uTransferWnd2);
	ini.WriteInt(L"VariousStatisticsMaxValue",statsMax);
	ini.WriteInt(L"StatsAverageMinutes",statsAverageMinutes);
	ini.WriteInt(L"MaxConnectionsPerFiveSeconds",MaxConperFive);
	ini.WriteInt(L"Check4NewVersionDelay",versioncheckdays);

	ini.WriteBool(L"Reconnect",reconnect);
	ini.WriteBool(L"Scoresystem",m_bUseServerPriorities);
	ini.WriteBool(L"Serverlist",m_bAutoUpdateServerList);
	ini.WriteBool(L"UpdateNotifyTestClient",updatenotify);
	if (IsRunningAeroGlassTheme())
		ini.WriteBool(L"MinToTray_Aero",mintotray);
	else
		ini.WriteBool(L"MinToTray",mintotray);
	ini.WriteBool(L"PreventStandby", m_bPreventStandby);
	ini.WriteBool(L"StoreSearches", m_bStoreSearches);
	ini.WriteBool(L"AddServersFromServer",m_bAddServersFromServer);
	ini.WriteBool(L"AddServersFromClient",m_bAddServersFromClients);
	ini.WriteBool(L"Splashscreen",splashscreen);
	ini.WriteBool(L"BringToFront",bringtoforeground);
	ini.WriteBool(L"TransferDoubleClick",transferDoubleclick);
	ini.WriteBool(L"ConfirmExit",confirmExit);
	ini.WriteBool(L"FilterBadIPs",filterLANIPs);
    ini.WriteBool(L"Autoconnect",autoconnect);
	ini.WriteBool(L"OnlineSignature",onlineSig);
	ini.WriteBool(L"StartupMinimized",startMinimized);
	ini.WriteBool(L"AutoStart",m_bAutoStart);
	ini.WriteInt(L"LastMainWndDlgID",m_iLastMainWndDlgID);
	ini.WriteInt(L"LastLogPaneID",m_iLastLogPaneID);
	ini.WriteBool(L"SafeServerConnect",m_bSafeServerConnect);
	ini.WriteBool(L"ShowRatesOnTitle",showRatesInTitle);
	ini.WriteBool(L"IndicateRatings",indicateratings);
	ini.WriteBool(L"WatchClipboard4ED2kFilelinks",watchclipboard);
	ini.WriteInt(L"SearchMethod",m_iSearchMethod);
	ini.WriteBool(L"CheckDiskspace",checkDiskspace);
	ini.WriteInt(L"MinFreeDiskSpace",m_uMinFreeDiskSpace);
	ini.WriteBool(L"SparsePartFiles",m_bSparsePartFiles);
	ini.WriteBool(L"ResolveSharedShellLinks",m_bResolveSharedShellLinks);
	ini.WriteString(L"YourHostname",m_strYourHostname);
	ini.WriteBool(L"CheckFileOpen",m_bCheckFileOpen);
	ini.WriteBool(L"ShowWin7TaskbarGoodies", m_bShowWin7TaskbarGoodies );

	// Barry - New properties...
    ini.WriteBool(L"AutoConnectStaticOnly", m_bAutoConnectToStaticServersOnly);
	ini.WriteBool(L"AutoTakeED2KLinks", autotakeed2klinks);
    ini.WriteBool(L"AddNewFilesPaused", addnewfilespaused);
    ini.WriteInt (L"3DDepth", depth3D);
	ini.WriteBool(L"MiniMule", m_bEnableMiniMule);

	ini.WriteString(L"NotifierConfiguration", notifierConfiguration);
	ini.WriteBool(L"NotifyOnDownload", notifierOnDownloadFinished);
	ini.WriteBool(L"NotifyOnNewDownload", notifierOnNewDownload);
	ini.WriteBool(L"NotifyOnChat", notifierOnChat);
	ini.WriteBool(L"NotifyOnLog", notifierOnLog);
	ini.WriteBool(L"NotifyOnImportantError", notifierOnImportantError);
	ini.WriteBool(L"NotifierPopEveryChatMessage", notifierOnEveryChatMsg);
	ini.WriteBool(L"NotifierPopNewVersion", notifierOnNewVersion);
	ini.WriteInt(L"NotifierUseSound", (int)notifierSoundType);
	ini.WriteString(L"NotifierSoundPath", notifierSoundFile);

	ini.WriteString(L"TxtEditor",m_strTxtEditor);
	ini.WriteString(L"VideoPlayer",m_strVideoPlayer);
	ini.WriteString(L"VideoPlayerArgs",m_strVideoPlayerArgs);
	ini.WriteString(L"MessageFilter",messageFilter);
	ini.WriteString(L"CommentFilter",commentFilter);
	ini.WriteString(L"DateTimeFormat",GetDateTimeFormat());
	ini.WriteString(L"DateTimeFormat4Log",GetDateTimeFormat4Log());
	ini.WriteString(L"WebTemplateFile",m_strTemplateFile);
	ini.WriteString(L"FilenameCleanups",filenameCleanups);
	ini.WriteInt(L"ExtractMetaData",m_iExtractMetaData);

	ini.WriteString(L"DefaultIRCServerNew", m_strIRCServer);
	ini.WriteString(L"IRCNick", m_strIRCNick);
	ini.WriteBool(L"IRCAddTimestamp", m_bIRCAddTimeStamp);
	ini.WriteString(L"IRCFilterName", m_strIRCChannelFilter);
	ini.WriteInt(L"IRCFilterUser", m_uIRCChannelUserFilter);
	ini.WriteBool(L"IRCUseFilter", m_bIRCUseChannelFilter);
	ini.WriteString(L"IRCPerformString", m_strIRCPerformString);
	ini.WriteBool(L"IRCUsePerform", m_bIRCUsePerform);
	ini.WriteBool(L"IRCListOnConnect", m_bIRCGetChannelsOnConnect);
	ini.WriteBool(L"IRCAcceptLink", m_bIRCAcceptLinks);
	ini.WriteBool(L"IRCAcceptLinkFriends", m_bIRCAcceptLinksFriendsOnly);
	ini.WriteBool(L"IRCSoundEvents", m_bIRCPlaySoundEvents);
	ini.WriteBool(L"IRCIgnoreMiscMessages", m_bIRCIgnoreMiscMessages);
	ini.WriteBool(L"IRCIgnoreJoinMessages", m_bIRCIgnoreJoinMessages);
	ini.WriteBool(L"IRCIgnorePartMessages", m_bIRCIgnorePartMessages);
	ini.WriteBool(L"IRCIgnoreQuitMessages", m_bIRCIgnoreQuitMessages);
	ini.WriteBool(L"IRCIgnoreEmuleAddFriendMsgs", m_bIRCIgnoreEmuleAddFriendMsgs);
	ini.WriteBool(L"IRCAllowEmuleAddFriend", m_bIRCAllowEmuleAddFriend);
	ini.WriteBool(L"IRCIgnoreEmuleSendLinkMsgs", m_bIRCIgnoreEmuleSendLinkMsgs);
	ini.WriteBool(L"IRCHelpChannel", m_bIRCJoinHelpChannel);
	ini.WriteBool(L"IRCEnableSmileys",m_bIRCEnableSmileys);
	ini.WriteBool(L"MessageEnableSmileys",m_bMessageEnableSmileys);

	ini.WriteBool(L"SmartIdCheck", m_bSmartServerIdCheck);
	ini.WriteBool(L"Verbose", m_bVerbose);
	ini.WriteBool(L"DebugSourceExchange", m_bDebugSourceExchange);	// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogBannedClients", m_bLogBannedClients);			// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogRatingDescReceived", m_bLogRatingDescReceived);// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogSecureIdent", m_bLogSecureIdent);				// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogFilteredIPs", m_bLogFilteredIPs);				// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogFileSaving", m_bLogFileSaving);				// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogA4AF", m_bLogA4AF);                           // do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogUlDlEvents", m_bLogUlDlEvents);
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	ini.WriteInt(L"DebugServerTCP",m_iDebugServerTCPLevel);
	ini.WriteInt(L"DebugServerUDP",m_iDebugServerUDPLevel);
	ini.WriteInt(L"DebugServerSources",m_iDebugServerSourcesLevel);
	ini.WriteInt(L"DebugServerSearches",m_iDebugServerSearchesLevel);
	ini.WriteInt(L"DebugClientTCP",m_iDebugClientTCPLevel);
	ini.WriteInt(L"DebugClientUDP",m_iDebugClientUDPLevel);
	ini.WriteInt(L"DebugClientKadUDP",m_iDebugClientKadUDPLevel);
#endif
	ini.WriteBool(L"PreviewPrio", m_bpreviewprio);
	ini.WriteBool(L"ManualHighPrio", m_bManualAddedServersHighPriority);
	ini.WriteBool(L"FullChunkTransfers", m_btransferfullchunks);
	ini.WriteBool(L"ShowOverhead", m_bshowoverhead);
	ini.WriteBool(L"VideoPreviewBackupped", moviePreviewBackup);
	ini.WriteInt(L"StartNextFile", m_istartnextfile);

	ini.DeleteKey(L"FileBufferSizePref"); // delete old 'file buff size' setting
	ini.WriteInt(L"FileBufferSize", m_iFileBufferSize);

	ini.DeleteKey(L"QueueSizePref"); // delete old 'queue size' setting
	ini.WriteInt(L"QueueSize", m_iQueueSize);

	ini.WriteInt(L"CommitFiles", m_iCommitFiles);
	ini.WriteBool(L"DAPPref", m_bDAP);
	ini.WriteBool(L"UAPPref", m_bUAP);
	ini.WriteBool(L"FilterServersByIP",filterserverbyip);
	ini.WriteBool(L"DisableKnownClientList",m_bDisableKnownClientList);
	ini.WriteBool(L"DisableQueueList",m_bDisableQueueList);
	ini.WriteBool(L"UseCreditSystem",m_bCreditSystem);
	ini.WriteBool(L"SaveLogToDisk",log2disk);
	ini.WriteBool(L"SaveDebugToDisk",debug2disk);
	ini.WriteBool(L"EnableScheduler",scheduler);
	ini.WriteBool(L"MessagesFromFriendsOnly",msgonlyfriends);
	ini.WriteBool(L"MessageUseCaptchas", m_bUseChatCaptchas);
	ini.WriteBool(L"ShowInfoOnCatTabs",showCatTabInfos);
	ini.WriteBool(L"AutoFilenameCleanup",autofilenamecleanup);
	ini.WriteBool(L"ShowExtControls",m_bExtControls);
	ini.WriteBool(L"UseAutocompletion",m_bUseAutocompl);
	ini.WriteBool(L"NetworkKademlia",networkkademlia);
	ini.WriteBool(L"NetworkED2K",networked2k);
	ini.WriteBool(L"AutoClearCompleted",m_bRemoveFinishedDownloads);
	ini.WriteBool(L"TransflstRemainOrder",m_bTransflstRemain);
	ini.WriteBool(L"UseSimpleTimeRemainingcomputation",m_bUseOldTimeRemaining);
	ini.WriteBool(L"AllocateFullFile",m_bAllocFull);
	ini.WriteBool(L"ShowSharedFilesDetails", m_bShowSharedFilesDetails);
	ini.WriteBool(L"AutoShowLookups", m_bAutoShowLookups);

	ini.WriteInt(L"VersionCheckLastAutomatic", versioncheckLastAutomatic);
	ini.WriteInt(L"FilterLevel",filterlevel);

	ini.WriteBool(L"SecureIdent", m_bUseSecureIdent);// change the name in future version to enable it by default
	ini.WriteBool(L"AdvancedSpamFilter",m_bAdvancedSpamfilter);
	ini.WriteBool(L"ShowDwlPercentage",m_bShowDwlPercentage);
	ini.WriteBool(L"RemoveFilesToBin",m_bRemove2bin);
	//ini.WriteBool(L"ShowCopyEd2kLinkCmd",m_bShowCopyEd2kLinkCmd);
	ini.WriteBool(L"AutoArchivePreviewStart", m_bAutomaticArcPreviewStart);

	// Toolbar
	ini.WriteString(L"ToolbarSetting", m_sToolbarSettings);
	ini.WriteString(L"ToolbarBitmap", m_sToolbarBitmap );
	ini.WriteString(L"ToolbarBitmapFolder", m_sToolbarBitmapFolder);
	ini.WriteInt(L"ToolbarLabels", m_nToolbarLabels);
	ini.WriteInt(L"ToolbarIconSize", m_sizToolbarIconSize.cx);
	ini.WriteString(L"SkinProfile", m_strSkinProfile);
	ini.WriteString(L"SkinProfileDir", m_strSkinProfileDir);

	ini.WriteBinary(L"HyperTextFont", (LPBYTE)&m_lfHyperText, sizeof m_lfHyperText);
	ini.WriteBinary(L"LogTextFont", (LPBYTE)&m_lfLogText, sizeof m_lfLogText);

	// ZZ:UploadSpeedSense -->
    ini.WriteBool(L"USSEnabled", m_bDynUpEnabled);
    ini.WriteBool(L"USSUseMillisecondPingTolerance", m_bDynUpUseMillisecondPingTolerance);
    ini.WriteInt(L"USSPingTolerance", m_iDynUpPingTolerance);
	ini.WriteInt(L"USSPingToleranceMilliseconds", m_iDynUpPingToleranceMilliseconds); // EastShare - Add by TAHO, USS limit
    ini.WriteInt(L"USSGoingUpDivider", m_iDynUpGoingUpDivider);
    ini.WriteInt(L"USSGoingDownDivider", m_iDynUpGoingDownDivider);
    ini.WriteInt(L"USSNumberOfPings", m_iDynUpNumberOfPings);
	// ZZ:UploadSpeedSense <--

    ini.WriteBool(L"A4AFSaveCpu", m_bA4AFSaveCpu); // ZZ:DownloadManager
    ini.WriteBool(L"HighresTimer", m_bHighresTimer);
	ini.WriteInt(L"WebMirrorAlertLevel", m_nWebMirrorAlertLevel);
	ini.WriteBool(L"RunAsUnprivilegedUser", m_bRunAsUser);
	ini.WriteBool(L"OpenPortsOnStartUp", m_bOpenPortsOnStartUp);
	ini.WriteInt(L"DebugLogLevel", m_byLogLevel);
	ini.WriteInt(L"WinXPSP2OrHigher", IsRunningXPSP2OrHigher());
	ini.WriteBool(L"RememberCancelledFiles", m_bRememberCancelledFiles);
	ini.WriteBool(L"RememberDownloadedFiles", m_bRememberDownloadedFiles);

	ini.WriteBool(L"NotifierSendMail", m_bNotifierSendMail);
	ini.WriteString(L"NotifierMailSender", m_strNotifierMailSender);
	ini.WriteString(L"NotifierMailServer", m_strNotifierMailServer);
	ini.WriteString(L"NotifierMailRecipient", m_strNotifierMailReceiver);

	ini.WriteBool(L"WinaTransToolbar", m_bWinaTransToolbar);
	ini.WriteBool(L"ShowDownloadToolbar", m_bShowDownloadToolbar);

	ini.WriteBool(L"CryptLayerRequested", m_bCryptLayerRequested);
	ini.WriteBool(L"CryptLayerRequired", m_bCryptLayerRequired);
	ini.WriteBool(L"CryptLayerSupported", m_bCryptLayerSupported);
	ini.WriteInt(L"KadUDPKey", m_dwKadUDPKey);

	ini.WriteBool(L"EnableSearchResultSpamFilter", m_bEnableSearchResultFilter);
	

	///////////////////////////////////////////////////////////////////////////
	// Section: "Proxy"
	//
	ini.WriteBool(L"ProxyEnablePassword",proxy.EnablePassword,L"Proxy");
	ini.WriteBool(L"ProxyEnableProxy",proxy.UseProxy,L"Proxy");
	ini.WriteString(L"ProxyName",CStringW(proxy.name),L"Proxy");
	ini.WriteString(L"ProxyPassword",CStringW(proxy.password),L"Proxy");
	ini.WriteString(L"ProxyUser",CStringW(proxy.user),L"Proxy");
	ini.WriteInt(L"ProxyPort",proxy.port,L"Proxy");
	ini.WriteInt(L"ProxyType",proxy.type,L"Proxy");


	///////////////////////////////////////////////////////////////////////////
	// Section: "Statistics"
	//
	ini.WriteInt(L"statsConnectionsGraphRatio", statsConnectionsGraphRatio,L"Statistics");
	ini.WriteString(L"statsExpandedTreeItems", m_strStatsExpandedTreeItems);
	CString buffer2;
	for (int i=0;i<15;i++) {
		buffer.Format(L"0x%06x",GetStatsColor(i));
		buffer2.Format(L"StatColor%i",i);
		ini.WriteString(buffer2,buffer,L"Statistics" );
	}
	ini.WriteBool(L"HasCustomTaskIconColor", bHasCustomTaskIconColor, L"Statistics");


	///////////////////////////////////////////////////////////////////////////
	// Section: "WebServer"
	//
	ini.WriteString(L"Password", GetWSPass(), L"WebServer");
	ini.WriteString(L"PasswordLow", GetWSLowPass());
	ini.WriteInt(L"Port", m_nWebPort);
	ini.WriteBool(L"WebUseUPnP", m_bWebUseUPnP);
	ini.WriteBool(L"Enabled", m_bWebEnabled);
	ini.WriteBool(L"UseGzip", m_bWebUseGzip);
	ini.WriteInt(L"PageRefreshTime", m_nWebPageRefresh);
	ini.WriteBool(L"UseLowRightsUser", m_bWebLowEnabled);
	ini.WriteBool(L"AllowAdminHiLevelFunc",m_bAllowAdminHiLevFunc);
	ini.WriteInt(L"WebTimeoutMins", m_iWebTimeoutMins);


	///////////////////////////////////////////////////////////////////////////
	// Section: "MobileMule"
	//
	ini.WriteString(L"Password", GetMMPass(), L"MobileMule");
	ini.WriteBool(L"Enabled", m_bMMEnabled);
	ini.WriteInt(L"Port", m_nMMPort);


	///////////////////////////////////////////////////////////////////////////
	// Section: "PeerCache"
	//
	ini.WriteInt(L"LastSearch", m_uPeerCacheLastSearch, L"PeerCache");
	ini.WriteBool(L"Found", m_bPeerCacheWasFound);
	ini.WriteInt(L"PCPort", m_nPeerCachePort);

	///////////////////////////////////////////////////////////////////////////
	// Section: "UPnP"
	//
	ini.WriteBool(L"EnableUPnP", m_bEnableUPnP, L"UPnP");
	ini.WriteBool(L"SkipWANIPSetup", m_bSkipWANIPSetup);
	ini.WriteBool(L"SkipWANPPPSetup", m_bSkipWANPPPSetup);
	ini.WriteBool(L"CloseUPnPOnExit", m_bCloseUPnPOnExit);
	ini.WriteInt(L"LastWorkingImplementation", m_nLastWorkingImpl);
	
}

void CPreferences::ResetStatsColor(int index)
{
	switch(index)
	{
		case  0: m_adwStatsColors[ 0]=RGB(  0,  0, 64);break;
		case  1: m_adwStatsColors[ 1]=RGB(192,192,255);break;
		case  2: m_adwStatsColors[ 2]=RGB(128,255,128);break;
		case  3: m_adwStatsColors[ 3]=RGB(  0,210,  0);break;
		case  4: m_adwStatsColors[ 4]=RGB(  0,128,  0);break;
		case  5: m_adwStatsColors[ 5]=RGB(255,128,128);break;
		case  6: m_adwStatsColors[ 6]=RGB(200,  0,  0);break;
		case  7: m_adwStatsColors[ 7]=RGB(140,  0,  0);break;
		case  8: m_adwStatsColors[ 8]=RGB(150,150,255);break;
		case  9: m_adwStatsColors[ 9]=RGB(192,  0,192);break;
		case 10: m_adwStatsColors[10]=RGB(255,255,128);break;
		case 11: m_adwStatsColors[11]=RGB(  0,  0,  0); bHasCustomTaskIconColor = false; break;
		case 12: m_adwStatsColors[12]=RGB(255,255,255);break;
		case 13: m_adwStatsColors[13]=RGB(255,255,255);break;
		case 14: m_adwStatsColors[14]=RGB(255,190,190);break;
	}
}

void CPreferences::GetAllStatsColors(int iCount, LPDWORD pdwColors)
{
	memset(pdwColors, 0, sizeof(*pdwColors) * iCount);
	memcpy(pdwColors, m_adwStatsColors, sizeof(*pdwColors) * min(_countof(m_adwStatsColors), iCount));
}

bool CPreferences::SetAllStatsColors(int iCount, const DWORD* pdwColors)
{
	bool bModified = false;
	int iMin = min(_countof(m_adwStatsColors), iCount);
	for (int i = 0; i < iMin; i++)
	{
		if (m_adwStatsColors[i] != pdwColors[i])
		{
			m_adwStatsColors[i] = pdwColors[i];
			bModified = true;
			if (i == 11)
				bHasCustomTaskIconColor = true;
		}
	}
	return bModified;
}

void CPreferences::IniCopy(CString si, CString di)
{
	CIni ini(GetConfigFile(), L"eMule");
	CString s = ini.GetString(si);
	// Do NOT write empty settings, this will mess up reading of default settings in case
	// there were no settings (fresh emule install) at all available!
	if (!s.IsEmpty())
	{
		ini.SetSection(L"ListControlSetup");
		ini.WriteString(di,s);
	}
}

void CPreferences::LoadPreferences()
{
	CIni ini(GetConfigFile(), L"eMule");
	ini.SetSection(L"eMule");

	CString strCurrVersion, strPrefsVersion;

	strCurrVersion = theApp.m_strCurVersionLong;
	strPrefsVersion = ini.GetString(L"AppVersion");

	m_bFirstStart = false;

	if (strPrefsVersion.IsEmpty()){
		m_bFirstStart = true;
	}
#ifdef _BETA
	CString strBetaNotified = ini.GetString(L"BetaVersionNotified", L"");
	m_bBetaNaggingDone = strBetaNotified.Compare(strCurrVersion) == 0;
#endif

#ifdef _DEBUG
	m_iDbgHeap = ini.GetInt(L"DebugHeap", 1);
#else
	m_iDbgHeap = 0;
#endif

	m_nWebMirrorAlertLevel = ini.GetInt(L"WebMirrorAlertLevel",0);
	updatenotify=ini.GetBool(L"UpdateNotifyTestClient",true);

	SetUserNick(ini.GetStringUTF8(L"Nick", DEFAULT_NICK));
	if (strNick.IsEmpty() || IsDefaultNick(strNick))
		SetUserNick(DEFAULT_NICK);

	m_strIncomingDir = ini.GetString(L"IncomingDir", _T(""));
	if (m_strIncomingDir.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
		m_strIncomingDir = GetDefaultDirectory(EMULE_INCOMINGDIR, true);
	MakeFoldername(m_strIncomingDir);

	// load tempdir(s) setting
	CString tempdirs;
	tempdirs = ini.GetString(L"TempDir", _T(""));
	if (tempdirs.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
		tempdirs = GetDefaultDirectory(EMULE_TEMPDIR, true);
	tempdirs += L"|" + ini.GetString(L"TempDirs");

	int curPos=0;
	bool doubled;
	CString atmp=tempdirs.Tokenize(L"|", curPos);
	while (!atmp.IsEmpty())
	{
		atmp.Trim();
		if (!atmp.IsEmpty()) {
			MakeFoldername(atmp);
			doubled=false;
			for (int i=0;i<tempdir.GetCount();i++)	// avoid double tempdirs
				if (atmp.CompareNoCase(GetTempDir(i))==0) {
					doubled=true;
					break;
				}
			if (!doubled) {
				if (PathFileExists(atmp)==FALSE) {
					CreateDirectory(atmp,NULL);
					if (PathFileExists(atmp)==TRUE || tempdir.GetCount()==0)
						tempdir.Add(atmp);
				}
				else
					tempdir.Add(atmp);
			}
		}
		atmp = tempdirs.Tokenize(L"|", curPos);
	}

	maxGraphDownloadRate=ini.GetInt(L"DownloadCapacity",96);
	if (maxGraphDownloadRate==0)
		maxGraphDownloadRate=96;
	
	maxGraphUploadRate = ini.GetInt(L"UploadCapacityNew",-1);
	if (maxGraphUploadRate == 0)
		maxGraphUploadRate = UNLIMITED;
	else if (maxGraphUploadRate == -1){
		// converting value from prior versions
		int nOldUploadCapacity = ini.GetInt(L"UploadCapacity", 16);
		if (nOldUploadCapacity == 16 && ini.GetInt(L"MaxUpload",12) == 12){
			// either this is a complete new install, or the prior version used the default value
			// in both cases, set the new default values to unlimited
			maxGraphUploadRate = UNLIMITED;
			ini.WriteInt(L"MaxUpload",UNLIMITED, L"eMule");
		}
		else
			maxGraphUploadRate = nOldUploadCapacity; // use old custoum value
	}

	minupload=(uint16)ini.GetInt(L"MinUpload", 1);
	maxupload=(uint16)ini.GetInt(L"MaxUpload",UNLIMITED);
	if (maxupload > maxGraphUploadRate && maxupload != UNLIMITED)
		maxupload = (uint16)(maxGraphUploadRate * .8);
	
	maxdownload=(uint16)ini.GetInt(L"MaxDownload", UNLIMITED);
	if (maxdownload > maxGraphDownloadRate && maxdownload != UNLIMITED)
		maxdownload = (uint16)(maxGraphDownloadRate * .8);
	maxconnections=ini.GetInt(L"MaxConnections",GetRecommendedMaxConnections());
	maxhalfconnections=ini.GetInt(L"MaxHalfConnections",9);
	m_bOverlappedSockets = ini.GetBool(L"OverlappedSockets", false); // Overlapped sockets are under investigations for buggyness (heap corruption), disable by default until fixed
	m_bConditionalTCPAccept = ini.GetBool(L"ConditionalTCPAccept", false);

	// reset max halfopen to a default if OS changed to SP2 (or higher) or away
	int dwSP2OrHigher = ini.GetInt(L"WinXPSP2OrHigher", -1);
	int dwCurSP2OrHigher = IsRunningXPSP2OrHigher();
	if (dwSP2OrHigher != dwCurSP2OrHigher){
		if (dwCurSP2OrHigher == 0)
			maxhalfconnections = 50;
		else if (dwCurSP2OrHigher == 1)
			maxhalfconnections = 9;
	}

	m_strBindAddrW = ini.GetString(L"BindAddr");
	m_strBindAddrW.Trim();
	m_pszBindAddrW = m_strBindAddrW.IsEmpty() ? NULL : (LPCWSTR)m_strBindAddrW;
	m_strBindAddrA = m_strBindAddrW;
	m_pszBindAddrA = m_strBindAddrA.IsEmpty() ? NULL : (LPCSTR)m_strBindAddrA;

	port = (uint16)ini.GetInt(L"Port", 0);
	if (port == 0)
		port = thePrefs.GetRandomTCPPort();

	// 0 is a valid value for the UDP port setting, as it is used for disabling it.
	int iPort = ini.GetInt(L"UDPPort", INT_MAX/*invalid port value*/);
	if (iPort == INT_MAX)
		udpport = thePrefs.GetRandomUDPPort();
	else
		udpport = (uint16)iPort;

	nServerUDPPort = (uint16)ini.GetInt(L"ServerUDPPort", -1); // 0 = Don't use UDP port for servers, -1 = use a random port (for backward compatibility)
	maxsourceperfile=ini.GetInt(L"MaxSourcesPerFile",400 );
	m_wLanguageID=ini.GetWORD(L"Language",0);
	m_iSeeShares=(EViewSharedFilesAccess)ini.GetInt(L"SeeShare",vsfaNobody);
	m_iToolDelayTime=ini.GetInt(L"ToolTipDelay",1);
	trafficOMeterInterval=ini.GetInt(L"StatGraphsInterval",3);
	statsInterval=ini.GetInt(L"statsInterval",5);
	m_bFillGraphs=ini.GetBool(L"StatsFillGraphs");
	dontcompressavi=ini.GetBool(L"DontCompressAvi",false);

	m_uDeadServerRetries=ini.GetInt(L"DeadServerRetry",1);
	if (m_uDeadServerRetries > MAX_SERVERFAILCOUNT)
		m_uDeadServerRetries = MAX_SERVERFAILCOUNT;
	m_dwServerKeepAliveTimeout=ini.GetInt(L"ServerKeepAliveTimeout",0);
	splitterbarPosition=ini.GetInt(L"SplitterbarPosition",75);
	if (splitterbarPosition < 9)
		splitterbarPosition = 9;
	else if (splitterbarPosition > 93)
		splitterbarPosition = 93;
	splitterbarPositionStat=ini.GetInt(L"SplitterbarPositionStat",30);
	splitterbarPositionStat_HL=ini.GetInt(L"SplitterbarPositionStat_HL",66);
	splitterbarPositionStat_HR=ini.GetInt(L"SplitterbarPositionStat_HR",33);
	if (splitterbarPositionStat_HR+1>=splitterbarPositionStat_HL){
		splitterbarPositionStat_HL = 66;
		splitterbarPositionStat_HR = 33;
	}
	splitterbarPositionFriend=ini.GetInt(L"SplitterbarPositionFriend",170);
	splitterbarPositionShared=ini.GetInt(L"SplitterbarPositionShared",179);
	splitterbarPositionIRC=ini.GetInt(L"SplitterbarPositionIRC",170);
	splitterbarPositionSvr=ini.GetInt(L"SplitterbarPositionServer",75);
	if (splitterbarPositionSvr>90 || splitterbarPositionSvr<10)
		splitterbarPositionSvr=75;

	m_uTransferWnd1 = ini.GetInt(L"TransferWnd1",0);
	m_uTransferWnd2 = ini.GetInt(L"TransferWnd2",1);

	statsMax=ini.GetInt(L"VariousStatisticsMaxValue",100);
	statsAverageMinutes=ini.GetInt(L"StatsAverageMinutes",5);
	MaxConperFive=ini.GetInt(L"MaxConnectionsPerFiveSeconds",GetDefaultMaxConperFive());

	reconnect = ini.GetBool(L"Reconnect", true);
	m_bUseServerPriorities = ini.GetBool(L"Scoresystem", true);
	m_bUseUserSortedServerList = ini.GetBool(L"UserSortedServerList", false);
	ICH = ini.GetBool(L"ICH", true);
	m_bAutoUpdateServerList = ini.GetBool(L"Serverlist", false);
	
	// since the minimize to tray button is not working under Aero (at least not at this point),
	// we enable map the minimize to tray on the minimize button by default if Aero is running
	if (IsRunningAeroGlassTheme())
		mintotray=ini.GetBool(L"MinToTray_Aero", true);
	else
		mintotray=ini.GetBool(L"MinToTray", false);

	m_bPreventStandby = ini.GetBool(L"PreventStandby", false);
	m_bStoreSearches = ini.GetBool(L"StoreSearches", true);
	m_bAddServersFromServer=ini.GetBool(L"AddServersFromServer",false);
	m_bAddServersFromClients=ini.GetBool(L"AddServersFromClient",false);
	splashscreen=ini.GetBool(L"Splashscreen",true);
	bringtoforeground=ini.GetBool(L"BringToFront",true);
	transferDoubleclick=ini.GetBool(L"TransferDoubleClick",true);
	beepOnError=ini.GetBool(L"BeepOnError",true);
	confirmExit=ini.GetBool(L"ConfirmExit",true);
	filterLANIPs=ini.GetBool(L"FilterBadIPs",true);
	m_bAllocLocalHostIP=ini.GetBool(L"AllowLocalHostIP",false);
	autoconnect=ini.GetBool(L"Autoconnect",false);
	showRatesInTitle=ini.GetBool(L"ShowRatesOnTitle",false);
	m_bIconflashOnNewMessage=ini.GetBool(L"IconflashOnNewMessage",false);

	onlineSig=ini.GetBool(L"OnlineSignature",false);
	startMinimized=ini.GetBool(L"StartupMinimized",false);
	m_bAutoStart=ini.GetBool(L"AutoStart",false);
	m_bRestoreLastMainWndDlg=ini.GetBool(L"RestoreLastMainWndDlg",false);
	m_iLastMainWndDlgID=ini.GetInt(L"LastMainWndDlgID",0);
	m_bRestoreLastLogPane=ini.GetBool(L"RestoreLastLogPane",false);
	m_iLastLogPaneID=ini.GetInt(L"LastLogPaneID",0);
	m_bSafeServerConnect =ini.GetBool(L"SafeServerConnect",false);

	m_bTransflstRemain =ini.GetBool(L"TransflstRemainOrder",false);
	filterserverbyip=ini.GetBool(L"FilterServersByIP",false);
	filterlevel=ini.GetInt(L"FilterLevel",127);
	checkDiskspace=ini.GetBool(L"CheckDiskspace",false);
	m_uMinFreeDiskSpace=ini.GetInt(L"MinFreeDiskSpace",20*1024*1024);
	m_bSparsePartFiles=ini.GetBool(L"SparsePartFiles",false);
	m_bResolveSharedShellLinks=ini.GetBool(L"ResolveSharedShellLinks",false);
	m_bKeepUnavailableFixedSharedDirs = ini.GetBool(L"KeepUnavailableFixedSharedDirs", false);
	m_strYourHostname=ini.GetString(L"YourHostname", L"");

	// Barry - New properties...
	m_bAutoConnectToStaticServersOnly = ini.GetBool(L"AutoConnectStaticOnly",false); 
	autotakeed2klinks = ini.GetBool(L"AutoTakeED2KLinks",true); 
	addnewfilespaused = ini.GetBool(L"AddNewFilesPaused",false); 
	depth3D = ini.GetInt(L"3DDepth", 5);
	m_bEnableMiniMule = ini.GetBool(L"MiniMule", true);

	// Notifier
	notifierConfiguration = ini.GetString(L"NotifierConfiguration", GetMuleDirectory(EMULE_CONFIGDIR) + L"Notifier.ini");
    notifierOnDownloadFinished = ini.GetBool(L"NotifyOnDownload");
	notifierOnNewDownload = ini.GetBool(L"NotifyOnNewDownload");
    notifierOnChat = ini.GetBool(L"NotifyOnChat");
    notifierOnLog = ini.GetBool(L"NotifyOnLog");
	notifierOnImportantError = ini.GetBool(L"NotifyOnImportantError");
	notifierOnEveryChatMsg = ini.GetBool(L"NotifierPopEveryChatMessage");
	notifierOnNewVersion = ini.GetBool(L"NotifierPopNewVersion");
    notifierSoundType = (ENotifierSoundType)ini.GetInt(L"NotifierUseSound", ntfstNoSound);
	notifierSoundFile = ini.GetString(L"NotifierSoundPath");

	m_strDateTimeFormat = ini.GetString(L"DateTimeFormat", L"%A, %c");
	m_strDateTimeFormat4Log = ini.GetString(L"DateTimeFormat4Log", L"%c");
	m_strDateTimeFormat4Lists = ini.GetString(L"DateTimeFormat4Lists", L"%c");

	m_strIRCServer = ini.GetString(L"DefaultIRCServerNew", L"ircchat.emule-project.net");
	m_strIRCNick = ini.GetString(L"IRCNick");
	m_bIRCAddTimeStamp = ini.GetBool(L"IRCAddTimestamp", true);
	m_bIRCUseChannelFilter = ini.GetBool(L"IRCUseFilter", true);
	m_strIRCChannelFilter = ini.GetString(L"IRCFilterName", L"");
	if (m_strIRCChannelFilter.IsEmpty())
		m_bIRCUseChannelFilter = false;
	m_uIRCChannelUserFilter = ini.GetInt(L"IRCFilterUser", 0);
	m_strIRCPerformString = ini.GetString(L"IRCPerformString");
	m_bIRCUsePerform = ini.GetBool(L"IRCUsePerform", false);
	m_bIRCGetChannelsOnConnect = ini.GetBool(L"IRCListOnConnect", true);
	m_bIRCAcceptLinks = ini.GetBool(L"IRCAcceptLink", true);
	m_bIRCAcceptLinksFriendsOnly = ini.GetBool(L"IRCAcceptLinkFriends", true);
	m_bIRCPlaySoundEvents = ini.GetBool(L"IRCSoundEvents", false);
	m_bIRCIgnoreMiscMessages = ini.GetBool(L"IRCIgnoreMiscMessages", false);
	m_bIRCIgnoreJoinMessages = ini.GetBool(L"IRCIgnoreJoinMessages", true);
	m_bIRCIgnorePartMessages = ini.GetBool(L"IRCIgnorePartMessages", true);
	m_bIRCIgnoreQuitMessages = ini.GetBool(L"IRCIgnoreQuitMessages", true);
	m_bIRCIgnoreEmuleAddFriendMsgs = ini.GetBool(L"IRCIgnoreEmuleAddFriendMsgs", false);
	m_bIRCAllowEmuleAddFriend = ini.GetBool(L"IRCAllowEmuleAddFriend", true);
	m_bIRCIgnoreEmuleSendLinkMsgs = ini.GetBool(L"IRCIgnoreEmuleSendLinkMsgs", false);
	m_bIRCJoinHelpChannel = ini.GetBool(L"IRCHelpChannel", true);
	m_bIRCEnableSmileys = ini.GetBool(L"IRCEnableSmileys", true);
	m_bMessageEnableSmileys = ini.GetBool(L"MessageEnableSmileys", true);

	m_bSmartServerIdCheck = ini.GetBool(L"SmartIdCheck",true);
	log2disk = ini.GetBool(L"SaveLogToDisk",false);
	uMaxLogFileSize = ini.GetInt(L"MaxLogFileSize", 1024*1024);
	iMaxLogBuff = ini.GetInt(L"MaxLogBuff",64) * 1024;
	m_iLogFileFormat = (ELogFileFormat)ini.GetInt(L"LogFileFormat", Unicode);
	m_bEnableVerboseOptions=ini.GetBool(L"VerboseOptions", true);
	if (m_bEnableVerboseOptions)
	{
		m_bVerbose=ini.GetBool(L"Verbose",false);
		m_bFullVerbose=ini.GetBool(L"FullVerbose",false);
		debug2disk=ini.GetBool(L"SaveDebugToDisk",false);
		m_bDebugSourceExchange=ini.GetBool(L"DebugSourceExchange",false);
		m_bLogBannedClients=ini.GetBool(L"LogBannedClients", true);
		m_bLogRatingDescReceived=ini.GetBool(L"LogRatingDescReceived",true);
		m_bLogSecureIdent=ini.GetBool(L"LogSecureIdent",true);
		m_bLogFilteredIPs=ini.GetBool(L"LogFilteredIPs",true);
		m_bLogFileSaving=ini.GetBool(L"LogFileSaving",false);
        m_bLogA4AF=ini.GetBool(L"LogA4AF",false); // ZZ:DownloadManager
		m_bLogUlDlEvents=ini.GetBool(L"LogUlDlEvents",true);
	}
	else
	{
		if (m_bRestoreLastLogPane && m_iLastLogPaneID>=2)
			m_iLastLogPaneID = 1;
	}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	m_iDebugServerTCPLevel = ini.GetInt(L"DebugServerTCP", 0);
	m_iDebugServerUDPLevel = ini.GetInt(L"DebugServerUDP", 0);
	m_iDebugServerSourcesLevel = ini.GetInt(L"DebugServerSources", 0);
	m_iDebugServerSearchesLevel = ini.GetInt(L"DebugServerSearches", 0);
	m_iDebugClientTCPLevel = ini.GetInt(L"DebugClientTCP", 0);
	m_iDebugClientUDPLevel = ini.GetInt(L"DebugClientUDP", 0);
	m_iDebugClientKadUDPLevel = ini.GetInt(L"DebugClientKadUDP", 0);
	m_iDebugSearchResultDetailLevel = ini.GetInt(L"DebugSearchResultDetailLevel", 0);
#else
	// for normal release builds ensure that those options are all turned off
	m_iDebugServerTCPLevel = 0;
	m_iDebugServerUDPLevel = 0;
	m_iDebugServerSourcesLevel = 0;
	m_iDebugServerSearchesLevel = 0;
	m_iDebugClientTCPLevel = 0;
	m_iDebugClientUDPLevel = 0;
	m_iDebugClientKadUDPLevel = 0;
	m_iDebugSearchResultDetailLevel = 0;
#endif

	m_bpreviewprio=ini.GetBool(L"PreviewPrio",false);
	m_bupdatequeuelist=ini.GetBool(L"UpdateQueueListPref",false);
	m_bManualAddedServersHighPriority=ini.GetBool(L"ManualHighPrio",false);
	m_btransferfullchunks=ini.GetBool(L"FullChunkTransfers",true);
	m_istartnextfile=ini.GetInt(L"StartNextFile",0);
	m_bshowoverhead=ini.GetBool(L"ShowOverhead",false);
	moviePreviewBackup=ini.GetBool(L"VideoPreviewBackupped",true);
	m_iPreviewSmallBlocks=ini.GetInt(L"PreviewSmallBlocks", 0);
	m_bPreviewCopiedArchives=ini.GetBool(L"PreviewCopiedArchives", true);
	m_iInspectAllFileTypes=ini.GetInt(L"InspectAllFileTypes", 0);
	m_bAllocFull=ini.GetBool(L"AllocateFullFile",0);
	m_bAutomaticArcPreviewStart=ini.GetBool(L"AutoArchivePreviewStart", true);
	m_bShowSharedFilesDetails = ini.GetBool(L"ShowSharedFilesDetails", true);
	m_bAutoShowLookups = ini.GetBool(L"AutoShowLookups", true);
	m_bShowUpDownIconInTaskbar = ini.GetBool(L"ShowUpDownIconInTaskbar", false );
	m_bShowWin7TaskbarGoodies  = ini.GetBool(L"ShowWin7TaskbarGoodies", true);
	m_bForceSpeedsToKB = ini.GetBool(L"ForceSpeedsToKB", false);
	m_bExtraPreviewWithMenu = ini.GetBool(L"ExtraPreviewWithMenu", false);

	// read file buffer size (with backward compatibility)
	m_iFileBufferSize=ini.GetInt(L"FileBufferSizePref",0); // old setting
	if (m_iFileBufferSize == 0)
		m_iFileBufferSize = 256*1024;
	else
		m_iFileBufferSize = ((m_iFileBufferSize*15000 + 512)/1024)*1024;
	m_iFileBufferSize=ini.GetInt(L"FileBufferSize",m_iFileBufferSize);
	m_uFileBufferTimeLimit = SEC2MS(ini.GetInt(L"FileBufferTimeLimit", 60));

	// read queue size (with backward compatibility)
	m_iQueueSize=ini.GetInt(L"QueueSizePref",0); // old setting
	if (m_iQueueSize == 0)
		m_iQueueSize = 50*100;
	else
		m_iQueueSize = m_iQueueSize*100;
	m_iQueueSize=ini.GetInt(L"QueueSize",m_iQueueSize);

	m_iCommitFiles=ini.GetInt(L"CommitFiles", 1); // 1 = "commit" on application shut down; 2 = "commit" on each file saveing
	versioncheckdays=ini.GetInt(L"Check4NewVersionDelay",5);
	m_bDAP=ini.GetBool(L"DAPPref",true);
	m_bUAP=ini.GetBool(L"UAPPref",true);
	m_bPreviewOnIconDblClk=ini.GetBool(L"PreviewOnIconDblClk",false);
	m_bCheckFileOpen=ini.GetBool(L"CheckFileOpen",true);
	indicateratings=ini.GetBool(L"IndicateRatings",true);
	watchclipboard=ini.GetBool(L"WatchClipboard4ED2kFilelinks",false);
	m_iSearchMethod=ini.GetInt(L"SearchMethod",0);

	showCatTabInfos=ini.GetBool(L"ShowInfoOnCatTabs",false);
//	resumeSameCat=ini.GetBool(L"ResumeNextFromSameCat",false);
	dontRecreateGraphs =ini.GetBool(L"DontRecreateStatGraphsOnResize",false);
	m_bExtControls =ini.GetBool(L"ShowExtControls",false);

	versioncheckLastAutomatic=ini.GetInt(L"VersionCheckLastAutomatic",0);
	m_bDisableKnownClientList=ini.GetBool(L"DisableKnownClientList",false);
	m_bDisableQueueList=ini.GetBool(L"DisableQueueList",false);
	m_bCreditSystem=ini.GetBool(L"UseCreditSystem",true);
	scheduler=ini.GetBool(L"EnableScheduler",false);
	msgonlyfriends=ini.GetBool(L"MessagesFromFriendsOnly",false);
	msgsecure=ini.GetBool(L"MessageFromValidSourcesOnly",true);
	m_bUseChatCaptchas = ini.GetBool(L"MessageUseCaptchas", true);
	autofilenamecleanup=ini.GetBool(L"AutoFilenameCleanup",false);
	m_bUseAutocompl=ini.GetBool(L"UseAutocompletion",true);
	m_bShowDwlPercentage=ini.GetBool(L"ShowDwlPercentage",false);
	networkkademlia=ini.GetBool(L"NetworkKademlia",true);
	networked2k=ini.GetBool(L"NetworkED2K",true);
	m_bRemove2bin=ini.GetBool(L"RemoveFilesToBin",true);
	m_bShowCopyEd2kLinkCmd=ini.GetBool(L"ShowCopyEd2kLinkCmd",false);

	m_iMaxChatHistory=ini.GetInt(L"MaxChatHistoryLines",100);
	if (m_iMaxChatHistory < 1)
		m_iMaxChatHistory = 100;
	maxmsgsessions=ini.GetInt(L"MaxMessageSessions",50);
	m_bShowActiveDownloadsBold = ini.GetBool(L"ShowActiveDownloadsBold", false);

	m_strTxtEditor = ini.GetString(L"TxtEditor", L"notepad.exe");
	m_strVideoPlayer = ini.GetString(L"VideoPlayer", L"");
	m_strVideoPlayerArgs = ini.GetString(L"VideoPlayerArgs",L"");
	
	m_strTemplateFile = ini.GetString(L"WebTemplateFile", GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"eMule.tmpl");
	// if emule is using the default, check if the file is in the config folder, as it used to be in prior version
	// and might be wanted by the user when switching to a personalized template
	if (m_strTemplateFile.Compare(GetMuleDirectory(EMULE_EXECUTEABLEDIR) + L"eMule.tmpl") == 0){
		CFileFind ff;
		if (ff.FindFile(GetMuleDirectory(EMULE_CONFIGDIR) + L"eMule.tmpl"))
			m_strTemplateFile = GetMuleDirectory(EMULE_CONFIGDIR) + L"eMule.tmpl";
		ff.Close();
	}

	messageFilter=ini.GetStringLong(L"MessageFilter",L"fastest download speed|fastest eMule");
	commentFilter = ini.GetStringLong(L"CommentFilter",L"http://|https://|ftp://|www.|ftp.");
	commentFilter.MakeLower();
	filenameCleanups=ini.GetStringLong(L"FilenameCleanups",L"http|www.|.com|.de|.org|.net|shared|powered|sponsored|sharelive|filedonkey|");
	m_iExtractMetaData = ini.GetInt(L"ExtractMetaData", 1); // 0=disable, 1=mp3, 2=MediaDet
	if (m_iExtractMetaData > 1)
		m_iExtractMetaData = 1;
	m_bAdjustNTFSDaylightFileTime=ini.GetBool(L"AdjustNTFSDaylightFileTime", true);
	m_bRearrangeKadSearchKeywords = ini.GetBool(L"RearrangeKadSearchKeywords", true);

	m_bUseSecureIdent=ini.GetBool(L"SecureIdent",true);
	m_bAdvancedSpamfilter=ini.GetBool(L"AdvancedSpamFilter",true);
	m_bRemoveFinishedDownloads=ini.GetBool(L"AutoClearCompleted",false);
	m_bUseOldTimeRemaining= ini.GetBool(L"UseSimpleTimeRemainingcomputation",false);

	// Toolbar
	m_sToolbarSettings = ini.GetString(L"ToolbarSetting", strDefaultToolbar);
	m_sToolbarBitmap = ini.GetString(L"ToolbarBitmap", L"");
	m_sToolbarBitmapFolder = ini.GetString(L"ToolbarBitmapFolder", _T(""));
	if (m_sToolbarBitmapFolder.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
		m_sToolbarBitmapFolder = GetDefaultDirectory(EMULE_TOOLBARDIR, true);
	m_nToolbarLabels = (EToolbarLabelType)ini.GetInt(L"ToolbarLabels", CMuleToolbarCtrl::GetDefaultLabelType());
	m_bReBarToolbar = ini.GetBool(L"ReBarToolbar", 1);
	m_sizToolbarIconSize.cx = m_sizToolbarIconSize.cy = ini.GetInt(L"ToolbarIconSize", 32);
	m_iStraightWindowStyles=ini.GetInt(L"StraightWindowStyles",0);
	m_bUseSystemFontForMainControls=ini.GetBool(L"UseSystemFontForMainControls",0);
	m_bRTLWindowsLayout = ini.GetBool(L"RTLWindowsLayout");
	m_strSkinProfile = ini.GetString(L"SkinProfile", L"");
	m_strSkinProfileDir = ini.GetString(L"SkinProfileDir", _T(""));
	if (m_strSkinProfileDir.IsEmpty()) // We want GetDefaultDirectory to also create the folder, so we have to know if we use the default or not
		m_strSkinProfileDir = GetDefaultDirectory(EMULE_SKINDIR, true);


	LPBYTE pData = NULL;
	UINT uSize = sizeof m_lfHyperText;
	if (ini.GetBinary(L"HyperTextFont", &pData, &uSize) && uSize == sizeof m_lfHyperText)
		memcpy(&m_lfHyperText, pData, sizeof m_lfHyperText);
	else
		memset(&m_lfHyperText, 0, sizeof m_lfHyperText);
	delete[] pData;

	pData = NULL;
	uSize = sizeof m_lfLogText;
	if (ini.GetBinary(L"LogTextFont", &pData, &uSize) && uSize == sizeof m_lfLogText)
		memcpy(&m_lfLogText, pData, sizeof m_lfLogText);
	else
		memset(&m_lfLogText, 0, sizeof m_lfLogText);
	delete[] pData;

	m_crLogError = ini.GetColRef(L"LogErrorColor", m_crLogError);
	m_crLogWarning = ini.GetColRef(L"LogWarningColor", m_crLogWarning);
	m_crLogSuccess = ini.GetColRef(L"LogSuccessColor", m_crLogSuccess);

	if (statsAverageMinutes < 1)
		statsAverageMinutes = 5;

	// ZZ:UploadSpeedSense -->
    m_bDynUpEnabled = ini.GetBool(L"USSEnabled", false);
    m_bDynUpUseMillisecondPingTolerance = ini.GetBool(L"USSUseMillisecondPingTolerance", false);
    m_iDynUpPingTolerance = ini.GetInt(L"USSPingTolerance", 500);
	m_iDynUpPingToleranceMilliseconds = ini.GetInt(L"USSPingToleranceMilliseconds", 200);
	if( minupload < 1 )
		minupload = 1;
	m_iDynUpGoingUpDivider = ini.GetInt(L"USSGoingUpDivider", 1000);
    m_iDynUpGoingDownDivider = ini.GetInt(L"USSGoingDownDivider", 1000);
    m_iDynUpNumberOfPings = ini.GetInt(L"USSNumberOfPings", 1);
	// ZZ:UploadSpeedSense <--

    m_bA4AFSaveCpu = ini.GetBool(L"A4AFSaveCpu", false); // ZZ:DownloadManager
    m_bHighresTimer = ini.GetBool(L"HighresTimer", false);
	m_bRunAsUser = ini.GetBool(L"RunAsUnprivilegedUser", false);
	m_bPreferRestrictedOverUser = ini.GetBool(L"PreferRestrictedOverUser", false);
	m_bOpenPortsOnStartUp = ini.GetBool(L"OpenPortsOnStartUp", false);
	m_byLogLevel = ini.GetInt(L"DebugLogLevel", DLP_VERYLOW);
	m_bTrustEveryHash = ini.GetBool(L"AICHTrustEveryHash", false);
	m_bRememberCancelledFiles = ini.GetBool(L"RememberCancelledFiles", true);
	m_bRememberDownloadedFiles = ini.GetBool(L"RememberDownloadedFiles", true);
	m_bPartiallyPurgeOldKnownFiles = ini.GetBool(L"PartiallyPurgeOldKnownFiles", true);

	m_bNotifierSendMail = ini.GetBool(L"NotifierSendMail", false);
#if _ATL_VER >= 0x0710
	if (!IsRunningXPSP2OrHigher())
		m_bNotifierSendMail = false;
#endif
	m_strNotifierMailSender = ini.GetString(L"NotifierMailSender", L"");
	m_strNotifierMailServer = ini.GetString(L"NotifierMailServer", L"");
	m_strNotifierMailReceiver = ini.GetString(L"NotifierMailRecipient", L"");

	m_bWinaTransToolbar = ini.GetBool(L"WinaTransToolbar", true);
	m_bShowDownloadToolbar = ini.GetBool(L"ShowDownloadToolbar", true);

	m_bCryptLayerRequested = ini.GetBool(L"CryptLayerRequested", false);
	m_bCryptLayerRequired = ini.GetBool(L"CryptLayerRequired", false);
	m_bCryptLayerSupported = ini.GetBool(L"CryptLayerSupported", true);
	m_dwKadUDPKey = ini.GetInt(L"KadUDPKey", GetRandomUInt32());

	uint32 nTmp = ini.GetInt(L"CryptTCPPaddingLength", 128);
	m_byCryptTCPPaddingLength = (uint8)min(nTmp, 254);

	m_bEnableSearchResultFilter = ini.GetBool(L"EnableSearchResultSpamFilter", true);

	///////////////////////////////////////////////////////////////////////////
	// Section: "Proxy"
	//
	proxy.EnablePassword = ini.GetBool(L"ProxyEnablePassword",false,L"Proxy");
	proxy.UseProxy = ini.GetBool(L"ProxyEnableProxy",false,L"Proxy");
	proxy.name = CStringA(ini.GetString(L"ProxyName", L"", L"Proxy"));
	proxy.user = CStringA(ini.GetString(L"ProxyUser", L"", L"Proxy"));
	proxy.password = CStringA(ini.GetString(L"ProxyPassword", L"", L"Proxy"));
	proxy.port = (uint16)ini.GetInt(L"ProxyPort",1080,L"Proxy");
	proxy.type = (uint16)ini.GetInt(L"ProxyType",PROXYTYPE_NOPROXY,L"Proxy");


	///////////////////////////////////////////////////////////////////////////
	// Section: "Statistics"
	//
	statsSaveInterval = ini.GetInt(L"SaveInterval", 60, L"Statistics");
	statsConnectionsGraphRatio = ini.GetInt(L"statsConnectionsGraphRatio", 3, L"Statistics");
	m_strStatsExpandedTreeItems = ini.GetString(L"statsExpandedTreeItems",L"111000000100000110000010000011110000010010",L"Statistics");
	CString buffer2;
	for (int i = 0; i < _countof(m_adwStatsColors); i++) {
		buffer2.Format(L"StatColor%i", i);
		m_adwStatsColors[i] = 0;
		if (_stscanf(ini.GetString(buffer2, L"", L"Statistics"), L"%i", &m_adwStatsColors[i]) != 1)
			ResetStatsColor(i);
	}
	bHasCustomTaskIconColor = ini.GetBool(L"HasCustomTaskIconColor",false, L"Statistics");
	m_bShowVerticalHourMarkers = ini.GetBool(L"ShowVerticalHourMarkers", true, L"Statistics");

	// -khaos--+++> Load Stats
	// I changed this to a seperate function because it is now also used
	// to load the stats backup and to load stats from preferences.ini.old.
	LoadStats();
	// <-----khaos-

	///////////////////////////////////////////////////////////////////////////
	// Section: "WebServer"
	//
	m_strWebPassword = ini.GetString(L"Password", L"", L"WebServer");
	m_strWebLowPassword = ini.GetString(L"PasswordLow", L"");
	m_nWebPort=(uint16)ini.GetInt(L"Port", 4711);
	m_bWebUseUPnP = ini.GetBool(L"WebUseUPnP", false);
	m_bWebEnabled=ini.GetBool(L"Enabled", false);
	m_bWebUseGzip=ini.GetBool(L"UseGzip", true);
	m_bWebLowEnabled=ini.GetBool(L"UseLowRightsUser", false);
	m_nWebPageRefresh=ini.GetInt(L"PageRefreshTime", 120);
	m_iWebTimeoutMins=ini.GetInt(L"WebTimeoutMins", 5 );
	m_iWebFileUploadSizeLimitMB=ini.GetInt(L"MaxFileUploadSizeMB", 5 );
	m_bAllowAdminHiLevFunc=ini.GetBool(L"AllowAdminHiLevelFunc", false);
	buffer2 = ini.GetString(L"AllowedIPs");
	int iPos = 0;
	CString strIP = buffer2.Tokenize(L";", iPos);
	while (!strIP.IsEmpty())
	{
		u_long nIP = inet_addr(CStringA(strIP));
		if (nIP != INADDR_ANY && nIP != INADDR_NONE)
			m_aAllowedRemoteAccessIPs.Add(nIP);
		strIP = buffer2.Tokenize(L";", iPos);
	}

	///////////////////////////////////////////////////////////////////////////
	// Section: "MobileMule"
	//
	m_strMMPassword = ini.GetString(L"Password", L"", L"MobileMule");
	m_bMMEnabled = ini.GetBool(L"Enabled", false);
	m_nMMPort = (uint16)ini.GetInt(L"Port", 80);

	///////////////////////////////////////////////////////////////////////////
	// Section: "PeerCache"
	//
	m_uPeerCacheLastSearch = ini.GetInt(L"LastSearch", 0, L"PeerCache");
	m_bPeerCacheWasFound = ini.GetBool(L"Found", false);
	m_bPeerCacheEnabled = ini.GetBool(L"EnabledDeprecated", false);
	m_nPeerCachePort = (uint16)ini.GetInt(L"PCPort", 0);
	m_bPeerCacheShow = ini.GetBool(L"Show", false);

	///////////////////////////////////////////////////////////////////////////
	// Section: "UPnP"
	//
	m_bEnableUPnP = ini.GetBool(L"EnableUPnP", false, L"UPnP");
	m_bSkipWANIPSetup = ini.GetBool(L"SkipWANIPSetup", false);
	m_bSkipWANPPPSetup = ini.GetBool(L"SkipWANPPPSetup", false);
	m_bCloseUPnPOnExit = ini.GetBool(L"CloseUPnPOnExit", true);
	m_nLastWorkingImpl = ini.GetInt(L"LastWorkingImplementation", 1 /*MiniUPnPLib*/);
	m_bIsMinilibImplDisabled = ini.GetBool(L"DisableMiniUPNPLibImpl", false);
	m_bIsWinServImplDisabled = ini.GetBool(L"DisableWinServImpl", false);

	LoadCats();
	SetLanguage();
}

WORD CPreferences::GetWindowsVersion(){
	static bool bWinVerAlreadyDetected = false;
	if(!bWinVerAlreadyDetected)
	{
		bWinVerAlreadyDetected = true;
		m_wWinVer = DetectWinVersion();
	}
	return m_wWinVer;
}

UINT CPreferences::GetDefaultMaxConperFive(){
	switch (GetWindowsVersion()){
		case _WINVER_98_:
			return 5;
		case _WINVER_95_:	
		case _WINVER_ME_:
			return MAXCON5WIN9X;
		case _WINVER_2K_:
		case _WINVER_XP_:
			return MAXCONPER5SEC;
		default:
			return MAXCONPER5SEC;
	}
}

//////////////////////////////////////////////////////////
// category implementations
//////////////////////////////////////////////////////////

void CPreferences::SaveCats()
{
	CString strCatIniFilePath;
	strCatIniFilePath.Format(L"%sCategory.ini", GetMuleDirectory(EMULE_CONFIGDIR));
	(void)_tremove(strCatIniFilePath);
	CIni ini(strCatIniFilePath);
	ini.WriteInt(L"Count", catMap.GetCount() - 1, L"General");
	for (int i = 0; i < catMap.GetCount(); i++)
	{
		CString strSection;
		strSection.Format(L"Cat#%i", i);
		ini.SetSection(strSection);

		ini.WriteStringUTF8(L"Title", catMap.GetAt(i)->strTitle);
		ini.WriteStringUTF8(L"Incoming", catMap.GetAt(i)->strIncomingPath);
		ini.WriteStringUTF8(L"Comment", catMap.GetAt(i)->strComment);
		ini.WriteStringUTF8(L"RegularExpression", catMap.GetAt(i)->regexp);
		ini.WriteInt(L"Color", catMap.GetAt(i)->color);
		ini.WriteInt(L"a4afPriority", catMap.GetAt(i)->prio); // ZZ:DownloadManager
		ini.WriteStringUTF8(L"AutoCat", catMap.GetAt(i)->autocat);
		ini.WriteInt(L"Filter", catMap.GetAt(i)->filter);
		ini.WriteBool(L"FilterNegator", catMap.GetAt(i)->filterNeg);
		ini.WriteBool(L"AutoCatAsRegularExpression", catMap.GetAt(i)->ac_regexpeval);
        ini.WriteBool(L"downloadInAlphabeticalOrder", catMap.GetAt(i)->downloadInAlphabeticalOrder!=FALSE);
		ini.WriteBool(L"Care4All", catMap.GetAt(i)->care4all);
	}
}

void CPreferences::LoadCats()
{
	CString strCatIniFilePath;
	strCatIniFilePath.Format(L"%sCategory.ini", GetMuleDirectory(EMULE_CONFIGDIR));
	CIni ini(strCatIniFilePath);
	int iNumCategories = ini.GetInt(L"Count", 0, L"General");
	for (int i = 0; i <= iNumCategories; i++)
	{
		CString strSection;
		strSection.Format(L"Cat#%i", i);
		ini.SetSection(strSection);

		Category_Struct* newcat = new Category_Struct;
		newcat->filter = 0;
		newcat->strTitle = ini.GetStringUTF8(L"Title");
		if (i != 0) // All category
		{
			newcat->strIncomingPath = ini.GetStringUTF8(L"Incoming");
			MakeFoldername(newcat->strIncomingPath);
			if (!IsShareableDirectory(newcat->strIncomingPath)
				|| (!PathFileExists(newcat->strIncomingPath) && !::CreateDirectory(newcat->strIncomingPath, 0)))
			{
				newcat->strIncomingPath = GetMuleDirectory(EMULE_INCOMINGDIR);
				MakeFoldername(newcat->strIncomingPath);
			}
		}
		else
			newcat->strIncomingPath.Empty();
		newcat->strComment = ini.GetStringUTF8(L"Comment");
		newcat->prio = ini.GetInt(L"a4afPriority", PR_NORMAL); // ZZ:DownloadManager
		newcat->filter = ini.GetInt(L"Filter", 0);
		newcat->filterNeg = ini.GetBool(L"FilterNegator", FALSE);
		newcat->ac_regexpeval = ini.GetBool(L"AutoCatAsRegularExpression", FALSE);
		newcat->care4all = ini.GetBool(L"Care4All", FALSE);
		newcat->regexp = ini.GetStringUTF8(L"RegularExpression");
		newcat->autocat = ini.GetStringUTF8(L"Autocat");
        newcat->downloadInAlphabeticalOrder = ini.GetBool(L"downloadInAlphabeticalOrder", FALSE); // ZZ:DownloadManager
		newcat->color = ini.GetInt(L"Color", (DWORD)-1 );
		AddCat(newcat);
	}
}

void CPreferences::RemoveCat(int index)
{
	if (index >= 0 && index < catMap.GetCount())
	{
		Category_Struct* delcat = catMap.GetAt(index); 
		catMap.RemoveAt(index);
		delete delcat;
	}
}

bool CPreferences::SetCatFilter(int index, int filter)
{
	if (index >= 0 && index < catMap.GetCount())
	{
		catMap.GetAt(index)->filter = filter;
		return true;
	}
	return false;
}

int CPreferences::GetCatFilter(int index)
{
	if (index >= 0 && index < catMap.GetCount())
		return catMap.GetAt(index)->filter;
    return 0;
}

bool CPreferences::GetCatFilterNeg(int index)
{
	if (index >= 0 && index < catMap.GetCount())
		return catMap.GetAt(index)->filterNeg;
    return false;
}

void CPreferences::SetCatFilterNeg(int index, bool val)
{
	if (index >= 0 && index < catMap.GetCount())
		catMap.GetAt(index)->filterNeg = val;
}

bool CPreferences::MoveCat(UINT from, UINT to)
{
	if (from >= (UINT)catMap.GetCount() || to >= (UINT)catMap.GetCount() + 1 || from == to)
		return false;

	Category_Struct* tomove = catMap.GetAt(from);
	if (from < to) {
		catMap.RemoveAt(from);
		catMap.InsertAt(to - 1, tomove);
	} else {
		catMap.InsertAt(to, tomove);
		catMap.RemoveAt(from + 1);
	}
	SaveCats();
	return true;
}


DWORD CPreferences::GetCatColor(int index, int nDefault) {
	if (index>=0 && index<catMap.GetCount()) {
		DWORD c=catMap.GetAt(index)->color;
		if (c!=(DWORD)-1)
			return catMap.GetAt(index)->color; 
	}

	return GetSysColor(nDefault);
}


///////////////////////////////////////////////////////

bool CPreferences::IsInstallationDirectory(const CString& rstrDir)
{
	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_EXECUTEABLEDIR)))
		return true;
	if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_CONFIGDIR)))
		return true;
	if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_WEBSERVERDIR)))
		return true;
	if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_INSTLANGDIR)))
		return true;
	if (!CompareDirectories(strFullPath, GetMuleDirectory(EMULE_LOGDIR)))
		return true;

	return false;
}

bool CPreferences::IsShareableDirectory(const CString& rstrDir)
{
	if (IsInstallationDirectory(rstrDir))
		return false;

	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	for (int i=0;i<GetTempDirCount();i++)
		if (!CompareDirectories(strFullPath, GetTempDir(i)))			// ".\eMule\temp"
			return false;

	return true;
}

void CPreferences::UpdateLastVC()
{
	struct tm tmTemp;
	versioncheckLastAutomatic = safe_mktime(CTime::GetCurrentTime().GetLocalTm(&tmTemp));
}

void CPreferences::SetWSPass(CString strNewPass)
{
	m_strWebPassword = MD5Sum(strNewPass).GetHash();
}

void CPreferences::SetWSLowPass(CString strNewPass)
{
	m_strWebLowPassword = MD5Sum(strNewPass).GetHash();
}

void CPreferences::SetMMPass(CString strNewPass)
{
	m_strMMPassword = MD5Sum(strNewPass).GetHash();
}

void CPreferences::SetMaxUpload(UINT in)
{
	uint16 oldMaxUpload = (uint16)in;
	maxupload = (oldMaxUpload) ? oldMaxUpload : (uint16)UNLIMITED;
}

void CPreferences::SetMaxDownload(UINT in)
{
	uint16 oldMaxDownload = (uint16)in;
	maxdownload = (oldMaxDownload) ? oldMaxDownload : (uint16)UNLIMITED;
}

void CPreferences::SetNetworkKademlia(bool val)
{
	networkkademlia = val; 
}

CString CPreferences::GetHomepageBaseURLForLevel(int nLevel){
	CString tmp;
	if (nLevel == 0)
		tmp = L"http://emule-project.net";
	else if (nLevel == 1)
		tmp = L"http://www.emule-project.org";
	else if (nLevel == 2)
		tmp = L"http://www.emule-project.com";
	else if (nLevel < 100)
		tmp.Format(L"http://www%i.emule-project.net",nLevel-2);
	else if (nLevel < 150)
		tmp.Format(L"http://www%i.emule-project.org",nLevel);
	else if (nLevel < 200)
		tmp.Format(L"http://www%i.emule-project.com",nLevel);
	else if (nLevel == 200)
		tmp = L"http://emule.sf.net";
	else if (nLevel == 201)
		tmp = L"http://www.emuleproject.net";
	else if (nLevel == 202)
		tmp = L"http://sourceforge.net/projects/emule/";
	else
		tmp = L"http://www.emule-project.net";
	return tmp;
}

CString CPreferences::GetVersionCheckBaseURL(){
	CString tmp;
	UINT nWebMirrorAlertLevel = GetWebMirrorAlertLevel();
	if (nWebMirrorAlertLevel < 100)
		tmp = L"http://vcheck.emule-project.net";
	else if (nWebMirrorAlertLevel < 150)
		tmp.Format(L"http://vcheck%i.emule-project.org",nWebMirrorAlertLevel);
	else if (nWebMirrorAlertLevel < 200)
		tmp.Format(L"http://vcheck%i.emule-project.com",nWebMirrorAlertLevel);
	else if (nWebMirrorAlertLevel == 200)
		tmp = L"http://emule.sf.net";
	else if (nWebMirrorAlertLevel == 201)
		tmp = L"http://www.emuleproject.net";
	else
		tmp = L"http://vcheck.emule-project.net";
	return tmp;
}

bool CPreferences::IsDefaultNick(const CString strCheck){
	// not fast, but this function is called often
	for (int i = 0; i != 255; i++){
		if (GetHomepageBaseURLForLevel(i) == strCheck)
			return true;
	}
	return ( strCheck == L"http://emule-project.net" );
}

void CPreferences::SetUserNick(LPCTSTR pszNick)
{
	strNick = pszNick;
}

UINT CPreferences::GetWebMirrorAlertLevel(){
	// Known upcoming DDoS Attacks
	if (m_nWebMirrorAlertLevel == 0){
		// no threats known at this time
	}
	// end
	if (UpdateNotify())
		return m_nWebMirrorAlertLevel;
	else
		return 0;
}

bool CPreferences::IsRunAsUserEnabled(){
	return (GetWindowsVersion() == _WINVER_XP_ || GetWindowsVersion() == _WINVER_2K_ || GetWindowsVersion() == _WINVER_2003_) 
		&& m_bRunAsUser
		&& m_nCurrentUserDirMode == 2;
}

bool CPreferences::GetUseReBarToolbar()
{
	return GetReBarToolbar() && theApp.m_ullComCtrlVer >= MAKEDLLVERULL(5,8,0,0);
}

int	CPreferences::GetMaxGraphUploadRate(bool bEstimateIfUnlimited){
	if (maxGraphUploadRate != UNLIMITED || !bEstimateIfUnlimited){
		return maxGraphUploadRate;
	}
	else{
		if (maxGraphUploadRateEstimated != 0){
			return maxGraphUploadRateEstimated +4;
		}
		else
			return 16;
	}
}

void CPreferences::EstimateMaxUploadCap(uint32 nCurrentUpload){
	if (maxGraphUploadRateEstimated+1 < nCurrentUpload){
		maxGraphUploadRateEstimated = nCurrentUpload;
		if (maxGraphUploadRate == UNLIMITED && theApp.emuledlg && theApp.emuledlg->statisticswnd)
			theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.GetMaxGraphUploadRate(true));
	}
}

void CPreferences::SetMaxGraphUploadRate(int in){
	maxGraphUploadRate	=(in) ? in : UNLIMITED;
}

bool CPreferences::IsDynUpEnabled()	{
	return m_bDynUpEnabled || maxGraphUploadRate == UNLIMITED;
}

bool CPreferences::CanFSHandleLargeFiles(int nForCat)	{
	bool bResult = false;
	for (int i = 0; i != tempdir.GetCount(); i++){
		if (!IsFileOnFATVolume(tempdir.GetAt(i))){
			bResult = true;
			break;
		}
	}
	return bResult && !IsFileOnFATVolume((nForCat > 0) ? GetCatPath(nForCat) : GetMuleDirectory(EMULE_INCOMINGDIR));
}

uint16 CPreferences::GetRandomTCPPort()
{
	// Get table of currently used TCP ports.
	PMIB_TCPTABLE pTCPTab = NULL;
	// Couple of crash dmp files are showing that we may crash somewhere in 'iphlpapi.dll' when doing the 2nd call
	__try {
		HMODULE hIpHlpDll = LoadLibrary(_T("iphlpapi.dll"));
		if (hIpHlpDll)
		{
			DWORD (WINAPI *pfnGetTcpTable)(PMIB_TCPTABLE, PDWORD, BOOL);
			(FARPROC&)pfnGetTcpTable = GetProcAddress(hIpHlpDll, "GetTcpTable");
			if (pfnGetTcpTable)
			{
				DWORD dwSize = 0;
				if ((*pfnGetTcpTable)(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
				{
					// The nr. of TCP entries could change (increase) between
					// the two function calls, allocate some more memory.
					dwSize += sizeof(pTCPTab->table[0]) * 50;
					pTCPTab = (PMIB_TCPTABLE)malloc(dwSize);
					if (pTCPTab)
					{
						if ((*pfnGetTcpTable)(pTCPTab, &dwSize, TRUE) != ERROR_SUCCESS)
						{
							free(pTCPTab);
							pTCPTab = NULL;
						}
					}
				}
			}
			FreeLibrary(hIpHlpDll);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		free(pTCPTab);
		pTCPTab = NULL;
	}

	const UINT uValidPortRange = 61000;
	int iMaxTests = uValidPortRange; // just in case, avoid endless loop
	uint16 nPort;
	bool bPortIsFree;
	do {
		// Get random port
		nPort = 4096 + (GetRandomUInt16() % uValidPortRange);

		// The port is by default assumed to be available. If we got a table of currently
		// used TCP ports, we verify that this port is currently not used in any way.
		bPortIsFree = true;
		if (pTCPTab)
		{
			uint16 nPortBE = htons(nPort);
			for (UINT e = 0; e < pTCPTab->dwNumEntries; e++)
			{
				// If there is a TCP entry in the table (regardless of its state), the port
				// is treated as not available.
				if (pTCPTab->table[e].dwLocalPort == nPortBE)
				{
					bPortIsFree = false;
					break;
				}
			}
		}
	}
	while (!bPortIsFree && --iMaxTests > 0);
	free(pTCPTab);
	return nPort;
}

uint16 CPreferences::GetRandomUDPPort()
{
	// Get table of currently used UDP ports.
	PMIB_UDPTABLE pUDPTab = NULL;
	// Couple of crash dmp files are showing that we may crash somewhere in 'iphlpapi.dll' when doing the 2nd call
	__try {
		HMODULE hIpHlpDll = LoadLibrary(_T("iphlpapi.dll"));
		if (hIpHlpDll)
		{
			DWORD (WINAPI *pfnGetUdpTable)(PMIB_UDPTABLE, PDWORD, BOOL);
			(FARPROC&)pfnGetUdpTable = GetProcAddress(hIpHlpDll, "GetUdpTable");
			if (pfnGetUdpTable)
			{
				DWORD dwSize = 0;
				if ((*pfnGetUdpTable)(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
				{
					// The nr. of UDP entries could change (increase) between
					// the two function calls, allocate some more memory.
					dwSize += sizeof(pUDPTab->table[0]) * 50;
					pUDPTab = (PMIB_UDPTABLE)malloc(dwSize);
					if (pUDPTab)
					{
						if ((*pfnGetUdpTable)(pUDPTab, &dwSize, TRUE) != ERROR_SUCCESS)
						{
							free(pUDPTab);
							pUDPTab = NULL;
						}
					}
				}
			}
			FreeLibrary(hIpHlpDll);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		free(pUDPTab);
		pUDPTab = NULL;
	}

	const UINT uValidPortRange = 61000;
	int iMaxTests = uValidPortRange; // just in case, avoid endless loop
	uint16 nPort;
	bool bPortIsFree;
	do {
		// Get random port
		nPort = 4096 + (GetRandomUInt16() % uValidPortRange);

		// The port is by default assumed to be available. If we got a table of currently
		// used UDP ports, we verify that this port is currently not used in any way.
		bPortIsFree = true;
		if (pUDPTab)
		{
			uint16 nPortBE = htons(nPort);
			for (UINT e = 0; e < pUDPTab->dwNumEntries; e++)
			{
				if (pUDPTab->table[e].dwLocalPort == nPortBE)
				{
					bPortIsFree = false;
					break;
				}
			}
		}
	}
	while (!bPortIsFree && --iMaxTests > 0);
	free(pUDPTab);
	return nPort;
}

// General behavior:
//
// WinVer < Vista
// Default: ApplicationDir if preference.ini exists there. If not: user specific dirs if preferences.ini exits there. If not: again ApplicationDir
// Default overwritten by Registry value (see below)
// Fallback: ApplicationDir
//
// WinVer >= Vista:
// Default: User specific Dir if preferences.ini exists there. If not: All users dir, if preferences.ini exists there. If not user specific dirs again
// Default overwritten by Registry value (see below)
// Fallback: ApplicationDir
CString CPreferences::GetDefaultDirectory(EDefaultDirectory eDirectory, bool bCreate){
	
	if (m_astrDefaultDirs[0].IsEmpty()){ // already have all directories fetched and stored?	
		
		// Get out exectuable starting directory which was our default till Vista
		TCHAR tchBuffer[MAX_PATH];
		::GetModuleFileName(NULL, tchBuffer, _countof(tchBuffer));
		tchBuffer[_countof(tchBuffer) - 1] = _T('\0');
		LPTSTR pszFileName = _tcsrchr(tchBuffer, L'\\') + 1;
		*pszFileName = L'\0';
		m_astrDefaultDirs[EMULE_EXECUTEABLEDIR] = tchBuffer;

		// set our results to old default / fallback values
		// those 3 dirs are the base for all others
		CString strSelectedDataBaseDirectory = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR];
		CString strSelectedConfigBaseDirectory = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR];
		CString strSelectedExpansionBaseDirectory = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR];
		m_nCurrentUserDirMode = 2; // To let us know which "mode" we are using in case we want to switch per options

		// check if preferences.ini exists already in our default / fallback dir
		CFileFind ff;
		bool bConfigAvailableExecuteable = ff.FindFile(strSelectedConfigBaseDirectory + CONFIGFOLDER + _T("preferences.ini"), 0) != 0;
		ff.Close();
		
		// check if our registry setting is present which forces the single or multiuser directories
		// and lets us ignore other defaults
		// 0 = Multiuser, 1 = Publicuser, 2 = ExecuteableDir. (on Winver < Vista 1 has the same effect as 2)
		DWORD nRegistrySetting = (DWORD)-1;
		CRegKey rkEMuleRegKey;
		if (rkEMuleRegKey.Open(HKEY_CURRENT_USER, _T("Software\\eMule"), KEY_READ) == ERROR_SUCCESS){
			rkEMuleRegKey.QueryDWORDValue(_T("UsePublicUserDirectories"), nRegistrySetting);
			rkEMuleRegKey.Close();
		}
		if (nRegistrySetting != -1 && nRegistrySetting != 0 && nRegistrySetting != 1 && nRegistrySetting != 2)
			nRegistrySetting = (DWORD)-1;

		// Do we need to get SystemFolders or do we use our old Default anyway? (Executable Dir)
		if (   nRegistrySetting == 0
			|| (nRegistrySetting == 1 && GetWindowsVersion() >= _WINVER_VISTA_)
			|| (nRegistrySetting == -1 && (!bConfigAvailableExecuteable || GetWindowsVersion() >= _WINVER_VISTA_)))
		{
			HMODULE hShell32 = LoadLibrary(_T("shell32.dll"));
			if (hShell32){
				if (GetWindowsVersion() >= _WINVER_VISTA_){
					
					PWSTR pszLocalAppData = NULL;
					PWSTR pszPersonalDownloads = NULL;
					PWSTR pszPublicDownloads = NULL;
					PWSTR pszProgrammData = NULL;

					// function not available on < WinVista
					HRESULT (WINAPI *pfnSHGetKnownFolderPath)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);
					(FARPROC&)pfnSHGetKnownFolderPath = GetProcAddress(hShell32, "SHGetKnownFolderPath");
					
					if (pfnSHGetKnownFolderPath != NULL
						&& (*pfnSHGetKnownFolderPath)(FOLDERID_LocalAppData, 0, NULL, &pszLocalAppData) == S_OK
						&& (*pfnSHGetKnownFolderPath)(FOLDERID_Downloads, 0, NULL, &pszPersonalDownloads) == S_OK
						&& (*pfnSHGetKnownFolderPath)(FOLDERID_PublicDownloads, 0, NULL, &pszPublicDownloads) == S_OK
						&& (*pfnSHGetKnownFolderPath)(FOLDERID_ProgramData, 0, NULL, &pszProgrammData) == S_OK)
					{
						if (_tcsclen(pszLocalAppData) < MAX_PATH - 30 && _tcsclen(pszPersonalDownloads) < MAX_PATH - 40
							&& _tcsclen(pszProgrammData) < MAX_PATH - 30 && _tcsclen(pszPublicDownloads) < MAX_PATH - 40)
						{
							CString strLocalAppData  = pszLocalAppData;
							CString strPersonalDownloads = pszPersonalDownloads;
							CString strPublicDownloads = pszPublicDownloads;
							CString strProgrammData = pszProgrammData;
							if (strLocalAppData.Right(1) != _T("\\"))
								strLocalAppData += _T("\\");
							if (strPersonalDownloads.Right(1) != _T("\\"))
								strPersonalDownloads += _T("\\");
							if (strPublicDownloads.Right(1) != _T("\\"))
								strPublicDownloads += _T("\\");
							if (strProgrammData.Right(1) != _T("\\"))
								strProgrammData += _T("\\");

							if (nRegistrySetting == -1){
								// no registry default, check if we find a preferences.ini to use
								bool bRes =  ff.FindFile(strLocalAppData + _T("eMule\\") + CONFIGFOLDER + _T("preferences.ini"), 0) != 0;
								ff.Close();
								if (bRes)
									m_nCurrentUserDirMode = 0;
								else{
									bRes =  ff.FindFile(strProgrammData + _T("eMule\\") + CONFIGFOLDER + _T("preferences.ini"), 0) != 0;
									ff.Close();
									if (bRes)
										m_nCurrentUserDirMode = 1;
									else if (bConfigAvailableExecuteable)
										m_nCurrentUserDirMode = 2;
									else
										m_nCurrentUserDirMode = 0; // no preferences.ini found, use the default
								}
							}
							else
								m_nCurrentUserDirMode = nRegistrySetting;
							
							if (m_nCurrentUserDirMode == 0){
								// multiuser
								strSelectedDataBaseDirectory = strPersonalDownloads + _T("eMule\\");
								strSelectedConfigBaseDirectory = strLocalAppData + _T("eMule\\");
								strSelectedExpansionBaseDirectory = strProgrammData + _T("eMule\\");
							}
							else if (m_nCurrentUserDirMode == 1){
								// public user
								strSelectedDataBaseDirectory = strPublicDownloads + _T("eMule\\");
								strSelectedConfigBaseDirectory = strProgrammData + _T("eMule\\");
								strSelectedExpansionBaseDirectory = strProgrammData + _T("eMule\\");
							}
							else if (m_nCurrentUserDirMode == 2){
								// programm directory
							}
							else
								ASSERT( false );
						}
						else
							ASSERT( false );
						}

						CoTaskMemFree(pszLocalAppData);
						CoTaskMemFree(pszPersonalDownloads);
						CoTaskMemFree(pszPublicDownloads);
						CoTaskMemFree(pszProgrammData);
				}
				else { // GetWindowsVersion() >= _WINVER_VISTA_

					CString strAppData = ShellGetFolderPath(CSIDL_APPDATA);
					CString strPersonal = ShellGetFolderPath(CSIDL_PERSONAL);
					if (!strAppData.IsEmpty() && !strPersonal.IsEmpty())
					{
						if (strAppData.GetLength() < MAX_PATH - 30 && strPersonal.GetLength() < MAX_PATH - 40){
							if (strPersonal.Right(1) != _T("\\"))
								strPersonal += _T("\\");
							if (strAppData.Right(1) != _T("\\"))
								strAppData += _T("\\");
							if (nRegistrySetting == 0){
								// registry setting overwrites, use these folders
								strSelectedDataBaseDirectory = strPersonal + _T("eMule Downloads\\");
								strSelectedConfigBaseDirectory = strAppData + _T("eMule\\");
								m_nCurrentUserDirMode = 0;
								// strSelectedExpansionBaseDirectory stays default
							}
							else if (nRegistrySetting == -1 && !bConfigAvailableExecuteable){
								if (ff.FindFile(strAppData + _T("eMule\\") + CONFIGFOLDER + _T("preferences.ini"), 0)){
									// preferences.ini found, so we use this as default
									strSelectedDataBaseDirectory = strPersonal + _T("eMule Downloads\\");
									strSelectedConfigBaseDirectory = strAppData + _T("eMule\\");
									m_nCurrentUserDirMode = 0;
								}
								ff.Close();
							}
							else
								ASSERT( false );
						}
						else
							ASSERT( false );
					}
				}
				FreeLibrary(hShell32);
			}
			else{
				DebugLogError(_T("Unable to load shell32.dll to retrieve the systemfolder locations, using fallbacks"));
				ASSERT( false );
			}
		}

		// the use of ending backslashes is inconsitent, would need a rework throughout the code to fix this
		m_astrDefaultDirs[EMULE_CONFIGDIR] = strSelectedConfigBaseDirectory + CONFIGFOLDER;
		m_astrDefaultDirs[EMULE_TEMPDIR] = strSelectedDataBaseDirectory + _T("Temp");
		m_astrDefaultDirs[EMULE_INCOMINGDIR] = strSelectedDataBaseDirectory + _T("Incoming");
		m_astrDefaultDirs[EMULE_LOGDIR] = strSelectedConfigBaseDirectory + _T("logs\\");
		m_astrDefaultDirs[EMULE_ADDLANGDIR] = strSelectedExpansionBaseDirectory + _T("lang\\");
		m_astrDefaultDirs[EMULE_INSTLANGDIR] = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR] + _T("lang\\");
		m_astrDefaultDirs[EMULE_WEBSERVERDIR] = m_astrDefaultDirs[EMULE_EXECUTEABLEDIR] + _T("webserver\\");
		m_astrDefaultDirs[EMULE_SKINDIR] = strSelectedExpansionBaseDirectory + _T("skins");
		m_astrDefaultDirs[EMULE_DATABASEDIR] = strSelectedDataBaseDirectory; // has ending backslashes
		m_astrDefaultDirs[EMULE_CONFIGBASEDIR] = strSelectedConfigBaseDirectory; // has ending backslashes
		//                EMULE_EXECUTEABLEDIR
		m_astrDefaultDirs[EMULE_TOOLBARDIR] = strSelectedExpansionBaseDirectory + _T("skins");
		m_astrDefaultDirs[EMULE_EXPANSIONDIR] = strSelectedExpansionBaseDirectory; // has ending backslashes

		/*CString strDebug;
		for (int i = 0; i < 12; i++)
			strDebug += m_astrDefaultDirs[i] + _T("\n");
		AfxMessageBox(strDebug, MB_ICONINFORMATION);*/
	}
	if (bCreate && !m_abDefaultDirsCreated[eDirectory]){
		switch (eDirectory){ // create the underlying directory first - be sure to adjust this if changing default directories
			case EMULE_CONFIGDIR:
			case EMULE_LOGDIR:
				::CreateDirectory(m_astrDefaultDirs[EMULE_CONFIGBASEDIR], NULL);
				break;
			case EMULE_TEMPDIR:
			case EMULE_INCOMINGDIR:
				::CreateDirectory(m_astrDefaultDirs[EMULE_DATABASEDIR], NULL);
				break;
			case EMULE_ADDLANGDIR:
			case EMULE_SKINDIR:
			case EMULE_TOOLBARDIR:
				::CreateDirectory(m_astrDefaultDirs[EMULE_EXPANSIONDIR], NULL);
				break;
		}
		::CreateDirectory(m_astrDefaultDirs[eDirectory], NULL);
		m_abDefaultDirsCreated[eDirectory] = true;
	}
	return m_astrDefaultDirs[eDirectory];
}

CString	CPreferences::GetMuleDirectory(EDefaultDirectory eDirectory, bool bCreate){
	switch (eDirectory){
		case EMULE_INCOMINGDIR:
			return m_strIncomingDir;
		case EMULE_TEMPDIR:
			ASSERT( false ); // use GetTempDir() instead! This function can only return the first tempdirectory
			return GetTempDir(0);
		case EMULE_SKINDIR:
			return m_strSkinProfileDir;
		case EMULE_TOOLBARDIR:
			return m_sToolbarBitmapFolder;
		default:
			return GetDefaultDirectory(eDirectory, bCreate);
	}
}

void CPreferences::SetMuleDirectory(EDefaultDirectory eDirectory, CString strNewDir){
	switch (eDirectory){
		case EMULE_INCOMINGDIR:
			m_strIncomingDir = strNewDir;
			break;
		case EMULE_SKINDIR:
			m_strSkinProfileDir = strNewDir;
			break;
		case EMULE_TOOLBARDIR:
			m_sToolbarBitmapFolder = strNewDir;
			break;
		default:
			ASSERT( false );
	}
}

void CPreferences::ChangeUserDirMode(int nNewMode){
	if (m_nCurrentUserDirMode == nNewMode)
		return;
	if (nNewMode == 1 && GetWindowsVersion() < _WINVER_VISTA_)
	{
		ASSERT( false );
		return;
	}
	// check if our registry setting is present which forces the single or multiuser directories
	// and lets us ignore other defaults
	// 0 = Multiuser, 1 = Publicuser, 2 = ExecuteableDir.
	CRegKey rkEMuleRegKey;
	if (rkEMuleRegKey.Create(HKEY_CURRENT_USER, _T("Software\\eMule")) == ERROR_SUCCESS){
		if (rkEMuleRegKey.SetDWORDValue(_T("UsePublicUserDirectories"), nNewMode) != ERROR_SUCCESS)
			DebugLogError(_T("Failed to write registry key to switch UserDirMode"));
		else
			m_nCurrentUserDirMode = nNewMode;
		rkEMuleRegKey.Close();
	}
}

bool CPreferences::GetSparsePartFiles()	{
	// Vistas Sparse File implemenation seems to be buggy as far as i can see
	// If a sparsefile exceeds a given limit of write io operations in a certain order (or i.e. end to beginning)
	// in its lifetime, it will at some point throw out a FILE_SYSTEM_LIMITATION error and deny any writing
	// to this file.
	// It was suggested that Vista might limits the dataruns, which would lead to such a behavior, but wouldn't
	// make much sense for a sparse file implementation nevertheless.
	// Due to the fact that eMule wirtes a lot small blocks into sparse files and flushs them every 6 seconds,
	// this problem pops up sooner or later for all big files. I don't see any way to walk arround this for now
	// Update: This problem seems to be fixed on Win7, possibly on earlier Vista ServicePacks too
	//		   In any case, we allow sparse files for vesions earlier and later than Vista
	return m_bSparsePartFiles && (GetWindowsVersion() != _WINVER_VISTA_);
}

bool CPreferences::IsRunningAeroGlassTheme(){
	// This is important for all functions which need to draw in the NC-Area (glass style)
	// Aero by default does not allow this, any drawing will not be visible. This can be turned off,
	// but Vista will not deliver the Glass style then as background when calling the default draw function
	// in other words, its draw all or nothing yourself - eMule chooses currently nothing
	static bool bAeroAlreadyDetected = false;
	if (!bAeroAlreadyDetected){
		bAeroAlreadyDetected = true;
		m_bIsRunningAeroGlass = FALSE;
		if (GetWindowsVersion() >= _WINVER_VISTA_){
			HMODULE hDWMAPI = LoadLibrary(_T("dwmapi.dll"));
			if (hDWMAPI){
				HRESULT (WINAPI *pfnDwmIsCompositionEnabled)(BOOL*);
				(FARPROC&)pfnDwmIsCompositionEnabled = GetProcAddress(hDWMAPI, "DwmIsCompositionEnabled");
				if (pfnDwmIsCompositionEnabled != NULL)
					pfnDwmIsCompositionEnabled(&m_bIsRunningAeroGlass);
				FreeLibrary(hDWMAPI);
			}
		}
	}
	return m_bIsRunningAeroGlass == TRUE ? true : false;
}