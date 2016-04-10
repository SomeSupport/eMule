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
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "UpDownClient.h"
#include "FriendList.h"
#include "Clientlist.h"
#include "OtherFunctions.h"
#include "PartFile.h"
#include "ListenSocket.h"
#include "PeerCacheSocket.h"
#include "Friend.h"
#include <zlib/zlib.h>
#include "Packets.h"
#include "Opcodes.h"
#include "SafeFile.h"
#include "Preferences.h"
#include "Server.h"
#include "ClientCredits.h"
#include "IPFilter.h"
#include "Statistics.h"
#include "Sockets.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "SearchFile.h"
#include "SearchList.h"
#include "SharedFileList.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Search.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "Kademlia/Kademlia/UDPFirewallTester.h"
#include "Kademlia/routing/RoutingZone.h"
#include "Kademlia/Utils/UInt128.h"
#include "Kademlia/Net/KademliaUDPListener.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "emuledlg.h"
#include "ServerWnd.h"
#include "TransferDlg.h"
#include "ChatWnd.h"
#include "CxImage/xImage.h"
#include "PreviewDlg.h"
#include "Exceptions.h"
#include "Peercachefinder.h"
#include "ClientUDPSocket.h"
#include "shahashset.h"
#include "Log.h"
#include "CaptchaGenerator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define URLINDICATOR	_T("http:|www.|.de |.net |.com |.org |.to |.tk |.cc |.fr |ftp:|ed2k:|https:|ftp.|.info|.biz|.uk|.eu|.es|.tv|.cn|.tw|.ws|.nu|.jp")

IMPLEMENT_DYNAMIC(CClientException, CException)
IMPLEMENT_DYNAMIC(CUpDownClient, CObject)

CUpDownClient::CUpDownClient(CClientReqSocket* sender)
{
	socket = sender;
	reqfile = NULL;
	Init();
}

CUpDownClient::CUpDownClient(CPartFile* in_reqfile, uint16 in_port, uint32 in_userid,uint32 in_serverip, uint16 in_serverport, bool ed2kID)
{
	//Converting to the HybridID system.. The ED2K system didn't take into account of IP address ending in 0..
	//All IP addresses ending in 0 were assumed to be a lowID because of the calculations.
	socket = NULL;
	reqfile = in_reqfile;
	Init();
	m_nUserPort = in_port;
	//If this is a ED2K source, check if it's a lowID.. If not, convert it to a HyrbidID.
	//Else, it's already in hybrid form.
	if(ed2kID && !IsLowID(in_userid))
		m_nUserIDHybrid = ntohl(in_userid);
	else
		m_nUserIDHybrid = in_userid;

	//If highID and ED2K source, incoming ID and IP are equal..
	//If highID and Kad source, incoming IP needs ntohl for the IP
	if (!HasLowID() && ed2kID)
		m_nConnectIP = in_userid;
	else if(!HasLowID())
		m_nConnectIP = ntohl(in_userid);
	m_dwServerIP = in_serverip;
	m_nServerPort = in_serverport;
}

void CUpDownClient::Init()
{
	m_nChatstate = MS_NONE;
	m_nKadState = KS_NONE;
	m_nChatCaptchaState = CA_NONE;
	m_nUploadState = US_NONE;
	m_nDownloadState = DS_NONE;
	m_SecureIdentState = IS_UNAVAILABLE;
	m_nConnectingState = CCS_NONE;
	m_ePeerCacheDownState = PCDS_NONE;
	m_ePeerCacheUpState = PCUS_NONE;

	credits = NULL;
	m_nSumForAvgUpDataRate = 0;
	m_bAddNextConnect = false;
	m_cShowDR = 0;
	m_nUDPPort = 0;
	m_nKadPort = 0;
	m_nTransferredUp = 0;
	m_cAsked = 0;
	m_cDownAsked = 0;
	m_nUpDatarate = 0;
	m_pszUsername = 0;
	m_nUserIDHybrid = 0;
	m_dwServerIP = 0;
	m_nServerPort = 0;
    m_iFileListRequested = 0;
	m_dwLastUpRequest = 0;
	m_bEmuleProtocol = false;
	m_bCompleteSource = false;
	m_bFriendSlot = false;
	m_bCommentDirty = false;
	m_bReaskPending = false;
	m_bUDPPending = false;
	m_byEmuleVersion = 0;
	m_nUserPort = 0;
	m_nPartCount = 0;
	m_nUpPartCount = 0;
	m_abyPartStatus = 0;
	m_abyUpPartStatus = 0;
	m_dwUploadTime = 0;
	m_nTransferredDown = 0;
	m_nDownDatarate = 0;
	m_nDownDataRateMS = 0;
	m_dwLastBlockReceived = 0;
	m_byDataCompVer = 0;
	m_byUDPVer = 0;
	m_bySourceExchange1Ver = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_nRemoteQueueRank = 0;
	m_dwLastSourceRequest = 0;
	m_dwLastSourceAnswer = 0;
	m_dwLastAskedForSources = 0;
	m_byCompatibleClient = 0;
	m_nSourceFrom = SF_SERVER;
	m_bIsHybrid = false;
	m_bIsML=false;
	m_Friend = NULL;
	m_uFileRating=0;
	(void)m_strFileComment;
	m_fMessageFiltered = 0;
	m_fIsSpammer = 0;
	m_cMessagesReceived = 0;
	m_cMessagesSent = 0;
	m_nCurSessionUp = 0;
	m_nCurSessionDown = 0;
    m_nCurSessionPayloadDown = 0;
	m_nSumForAvgDownDataRate = 0;
	m_clientSoft=SO_UNKNOWN;
	m_bRemoteQueueFull = false;
	md4clr(m_achUserHash);
	SetBuddyID(NULL);
	m_nBuddyIP = 0;
	m_nBuddyPort = 0;
	if (socket){
		SOCKADDR_IN sockAddr = {0};
		int nSockAddrLen = sizeof(sockAddr);
		socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
		SetIP(sockAddr.sin_addr.S_un.S_addr);
	}
	else{
		SetIP(0);
	}
	m_fHashsetRequestingAICH = 0;
	m_fHashsetRequestingMD4 = 0;
	m_fSharedDirectories = 0;
	m_fSentCancelTransfer = 0;
	m_nClientVersion = 0;
	m_lastRefreshedDLDisplay = 0;
	m_dwDownStartTime = 0;
	m_nLastBlockOffset = (uint64)-1;
	m_bUnicodeSupport = false;
	m_dwLastSignatureIP = 0;
	m_bySupportSecIdent = 0;
	m_byInfopacketsReceived = IP_NONE;
	m_lastPartAsked = (uint16)-1;
	m_nUpCompleteSourcesCount= 0;
	m_fSupportsPreview = 0;
	m_fPreviewReqPending = 0;
	m_fPreviewAnsPending = 0;
	m_bTransferredDownMini = false;
    m_addedPayloadQueueSession = 0;
    m_nCurQueueSessionPayloadUp = 0; // PENDING: Is this necessary? ResetSessionUp()...
    m_lastRefreshedULDisplay = ::GetTickCount();
	m_bGPLEvildoer = false;
	m_bHelloAnswerPending = false;
	m_fNoViewSharedFiles = 0;
	m_bMultiPacket = 0;
	md4clr(requpfileid);
	m_nTotalUDPPackets = 0;
	m_nFailedUDPPackets = 0;
	m_nUrlStartPos = (uint64)-1;
	m_iHttpSendState = 0;
	m_fPeerCache = 0;
	m_uPeerCacheDownloadPushId = 0;
	m_uPeerCacheUploadPushId = 0;
	m_pPCDownSocket = NULL;
	m_pPCUpSocket = NULL;
	m_uPeerCacheRemoteIP = 0;
	m_bPeerCacheDownHit = false;
	m_bPeerCacheUpHit = false;
	m_fNeedOurPublicIP = 0;
    m_random_update_wait = (uint32)(rand()/(RAND_MAX/1000));
    m_bSourceExchangeSwapped = false; // ZZ:DownloadManager
    m_dwLastTriedToConnect = ::GetTickCount()-20*60*1000; // ZZ:DownloadManager
	m_fQueueRankPending = 0;
	m_fUnaskQueueRankRecv = 0;
	m_fFailedFileIdReqs = 0;
    m_slotNumber = 0;
    lastSwapForSourceExchangeTick = 0;
	m_pReqFileAICHHash = NULL;
	m_fSupportsAICH = 0;
	m_fAICHRequested = 0;
	m_byKadVersion = 0;
	SetLastBuddyPingPongTime();
	m_fSentOutOfPartReqs = 0;
	m_bCollectionUploadSlot = false;
	m_fSupportsLargeFiles = 0;
	m_fExtMultiPacket = 0;
	m_fRequestsCryptLayer = 0;
	m_fSupportsCryptLayer = 0;
	m_fRequiresCryptLayer = 0;
	m_fSupportsSourceEx2 = 0;
	m_fSupportsCaptcha = 0;
	m_fDirectUDPCallback = 0;
	m_cCaptchasSent = 0;
	m_fSupportsFileIdent = 0;
}

CUpDownClient::~CUpDownClient(){
	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHRecoveryHashSet::ClientAICHRequestFailed(this);
	}

	if (GetFriend() != NULL)
	{
		if (GetFriend()->IsTryingToConnect())
			GetFriend()->UpdateFriendConnectionState(FCR_DELETED);
        m_Friend->SetLinkedClient(NULL);
	}
	ASSERT( m_nConnectingState == CCS_NONE || !theApp.emuledlg->IsRunning() );
	theApp.clientlist->RemoveClient(this, _T("Destructing client object"));

	if (socket){
		socket->client = 0;
		socket->Safe_Delete();
	}
	if (m_pPCDownSocket){
		m_pPCDownSocket->client = NULL;
		m_pPCDownSocket->Safe_Delete();
	}
	if (m_pPCUpSocket){
		m_pPCUpSocket->client = NULL;
		m_pPCUpSocket->Safe_Delete();
	}
	
	free(m_pszUsername);
	
	delete[] m_abyPartStatus;
	m_abyPartStatus = NULL;
	
	delete[] m_abyUpPartStatus;
	m_abyUpPartStatus = NULL;
	
	FlushSendBlocks();
	
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;)
		delete m_RequestedFiles_list.GetNext(pos);
	
	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;){
		Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
		delete pending->block;
		// Not always allocated
		if (pending->zStream){
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
		delete pending;
	}

	for (POSITION pos = m_WaitingPackets_list.GetHeadPosition();pos != 0;)
		delete m_WaitingPackets_list.GetNext(pos);
	
	DEBUG_ONLY (theApp.listensocket->Debug_ClientDeleted(this));
	SetUploadFileID(NULL);

    m_fileReaskTimes.RemoveAll(); // ZZ:DownloadManager (one resk timestamp for each file)

	delete m_pReqFileAICHHash;
}

void CUpDownClient::ClearHelloProperties()
{
	m_nUDPPort = 0;
	m_byUDPVer = 0;
	m_byDataCompVer = 0;
	m_byEmuleVersion = 0;
	m_bySourceExchange1Ver = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_byCompatibleClient = 0;
	m_nKadPort = 0;
	m_bySupportSecIdent = 0;
	m_fSupportsPreview = 0;
	m_nClientVersion = 0;
	m_fSharedDirectories = 0;
	m_bMultiPacket = 0;
	m_fPeerCache = 0;
	m_uPeerCacheDownloadPushId = 0;
	m_uPeerCacheUploadPushId = 0;
	m_byKadVersion = 0;
	m_fSupportsLargeFiles = 0;
	m_fExtMultiPacket = 0;
	m_fRequestsCryptLayer = 0;
	m_fSupportsCryptLayer = 0;
	m_fRequiresCryptLayer = 0;
	m_fSupportsSourceEx2 = 0;
	m_fSupportsCaptcha = 0;
	m_fDirectUDPCallback = 0;
	m_fSupportsFileIdent = 0;
}

bool CUpDownClient::ProcessHelloPacket(const uchar* pachPacket, uint32 nSize)
{
	CSafeMemFile data(pachPacket, nSize);
	data.ReadUInt8(); // read size of userhash
	// reset all client properties; a client may not send a particular emule tag any longer
	ClearHelloProperties();
	return ProcessHelloTypePacket(&data);
}

bool CUpDownClient::ProcessHelloAnswer(const uchar* pachPacket, uint32 nSize)
{
	CSafeMemFile data(pachPacket, nSize);
	bool bIsMule = ProcessHelloTypePacket(&data);
	m_bHelloAnswerPending = false;
	return bIsMule;
}

bool CUpDownClient::ProcessHelloTypePacket(CSafeMemFile* data)
{
	bool bDbgInfo = thePrefs.GetUseDebugDevice();
	m_strHelloInfo.Empty();
	// clear hello properties which can be changed _only_ on receiving OP_Hello/OP_HelloAnswer
	m_bIsHybrid = false;
	m_bIsML = false;
	m_fNoViewSharedFiles = 0;
	m_bUnicodeSupport = false;

	data->ReadHash16(m_achUserHash);
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("Hash=%s (%s)"), md4str(m_achUserHash), DbgGetHashTypeString(m_achUserHash));
	m_nUserIDHybrid = data->ReadUInt32();
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("  UserID=%u (%s)"), m_nUserIDHybrid, ipstr(m_nUserIDHybrid));
	uint16 nUserPort = data->ReadUInt16(); // hmm clientport is sent twice - why?
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("  Port=%u"), nUserPort);
	
	DWORD dwEmuleTags = 0;
	bool bPrTag = false;
	uint32 tagcount = data->ReadUInt32();
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("  Tags=%u"), tagcount);
	for (uint32 i = 0; i < tagcount; i++)
	{
		CTag temptag(data, true);
		switch (temptag.GetNameID())
		{
			case CT_NAME:
				if (temptag.IsStr()) {
					free(m_pszUsername);
					m_pszUsername = _tcsdup(temptag.GetStr());
					if (bDbgInfo) {
						if (m_pszUsername) {//filter username for bad chars
							TCHAR* psz = m_pszUsername;
							while (*psz != _T('\0')) {
								if (*psz == _T('\n') || *psz == _T('\r'))
									*psz = _T(' ');
								psz++;
							}
						}
						m_strHelloInfo.AppendFormat(_T("\n  Name='%s'"), m_pszUsername);
					}
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_VERSION:
				if (temptag.IsInt()) {
					if (bDbgInfo)
						m_strHelloInfo.AppendFormat(_T("\n  Version=%u"), temptag.GetInt());
					m_nClientVersion = temptag.GetInt();
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_PORT:
				if (temptag.IsInt()) {
					if (bDbgInfo)
						m_strHelloInfo.AppendFormat(_T("\n  Port=%u"), temptag.GetInt());
					nUserPort = (uint16)temptag.GetInt();
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_MOD_VERSION:
				if (temptag.IsStr())
					m_strModVersion = temptag.GetStr();
				else if (temptag.IsInt())
					m_strModVersion.Format(_T("ModID=%u"), temptag.GetInt());
				else
					m_strModVersion = _T("ModID=<Unknown>");
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ModID=%s"), m_strModVersion);
				CheckForGPLEvilDoer();
				break;

			case CT_EMULE_UDPPORTS:
				// 16 KAD Port
				// 16 UDP Port
				if (temptag.IsInt()) {
					m_nKadPort = (uint16)(temptag.GetInt() >> 16);
					m_nUDPPort = (uint16)temptag.GetInt();
					if (bDbgInfo)
						m_strHelloInfo.AppendFormat(_T("\n  KadPort=%u  UDPPort=%u"), m_nKadPort, m_nUDPPort);
					dwEmuleTags |= 1;
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_EMULE_BUDDYUDP:
				// 16 --Reserved for future use--
				// 16 BUDDY Port
				if (temptag.IsInt()) {
					m_nBuddyPort = (uint16)temptag.GetInt();
					if (bDbgInfo)
						m_strHelloInfo.AppendFormat(_T("\n  BuddyPort=%u"), m_nBuddyPort);
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_EMULE_BUDDYIP:
				// 32 BUDDY IP
				if (temptag.IsInt()) {
					m_nBuddyIP = temptag.GetInt();
					if (bDbgInfo)
						m_strHelloInfo.AppendFormat(_T("\n  BuddyIP=%s"), ipstr(m_nBuddyIP));
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_EMULE_MISCOPTIONS1:
				//  3 AICH Version (0 = not supported)
				//  1 Unicode
				//  4 UDP version
				//  4 Data compression version
				//  4 Secure Ident
				//  4 Source Exchange - deprecated
				//  4 Ext. Requests
				//  4 Comments
				//	1 PeerChache supported
				//	1 No 'View Shared Files' supported
				//	1 MultiPacket - deprecated with FileIdentifiers/MultipacketExt2
				//  1 Preview
				if (temptag.IsInt()) {
					m_fSupportsAICH			= (temptag.GetInt() >> 29) & 0x07;
					m_bUnicodeSupport		= (temptag.GetInt() >> 28) & 0x01;
					m_byUDPVer				= (uint8)((temptag.GetInt() >> 24) & 0x0f);
					m_byDataCompVer			= (uint8)((temptag.GetInt() >> 20) & 0x0f);
					m_bySupportSecIdent		= (uint8)((temptag.GetInt() >> 16) & 0x0f);
					m_bySourceExchange1Ver	= (uint8)((temptag.GetInt() >> 12) & 0x0f);
					m_byExtendedRequestsVer	= (uint8)((temptag.GetInt() >>  8) & 0x0f);
					m_byAcceptCommentVer	= (uint8)((temptag.GetInt() >>  4) & 0x0f);
					m_fPeerCache			= (temptag.GetInt() >>  3) & 0x01;
					m_fNoViewSharedFiles	= (temptag.GetInt() >>  2) & 0x01;
					m_bMultiPacket			= (temptag.GetInt() >>  1) & 0x01;
					m_fSupportsPreview		= (temptag.GetInt() >>  0) & 0x01;
					dwEmuleTags |= 2;
					if (bDbgInfo) {
						m_strHelloInfo.AppendFormat(_T("\n  PeerCache=%u  UDPVer=%u  DataComp=%u  SecIdent=%u  SrcExchg=%u")
													_T("  ExtReq=%u  Commnt=%u  Preview=%u  NoViewFiles=%u  Unicode=%u"), 
													m_fPeerCache, m_byUDPVer, m_byDataCompVer, m_bySupportSecIdent, m_bySourceExchange1Ver, 
													m_byExtendedRequestsVer, m_byAcceptCommentVer, m_fSupportsPreview, m_fNoViewSharedFiles, m_bUnicodeSupport);
					}
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_EMULE_MISCOPTIONS2:
				//	18 Reserved
				//   1 Supports new FileIdentifiers/MultipacketExt2
				//   1 Direct UDP Callback supported and available
				//	 1 Supports ChatCaptchas
				//	 1 Supports SourceExachnge2 Packets, ignores SX1 Packet Version
				//	 1 Requires CryptLayer
				//	 1 Requests CryptLayer
				//	 1 Supports CryptLayer
				//	 1 Reserved (ModBit)
				//   1 Ext Multipacket (Hash+Size instead of Hash) - deprecated with FileIdentifiers/MultipacketExt2
				//   1 Large Files (includes support for 64bit tags)
				//   4 Kad Version - will go up to version 15 only (may need to add another field at some point in the future)
				if (temptag.IsInt()) {
					m_fSupportsFileIdent	= (temptag.GetInt() >>  13) & 0x01;
					m_fDirectUDPCallback	= (temptag.GetInt() >>  12) & 0x01;
					m_fSupportsCaptcha	    = (temptag.GetInt() >>  11) & 0x01;
					m_fSupportsSourceEx2	= (temptag.GetInt() >>  10) & 0x01;
					m_fRequiresCryptLayer	= (temptag.GetInt() >>  9) & 0x01;
					m_fRequestsCryptLayer	= (temptag.GetInt() >>  8) & 0x01;
					m_fSupportsCryptLayer	= (temptag.GetInt() >>  7) & 0x01;
					// reserved 1
					m_fExtMultiPacket		= (temptag.GetInt() >>  5) & 0x01;
					m_fSupportsLargeFiles   = (temptag.GetInt() >>  4) & 0x01;
					m_byKadVersion			= (uint8)((temptag.GetInt() >>  0) & 0x0f);
					dwEmuleTags |= 8;
					if (bDbgInfo)
						m_strHelloInfo.AppendFormat(_T("\n  KadVersion=%u, LargeFiles=%u ExtMultiPacket=%u CryptLayerSupport=%u CryptLayerRequest=%u CryptLayerRequires=%u SupportsSourceEx2=%u SupportsCaptcha=%u DirectUDPCallback=%u"), m_byKadVersion, m_fSupportsLargeFiles, m_fExtMultiPacket, m_fSupportsCryptLayer, m_fRequestsCryptLayer, m_fRequiresCryptLayer, m_fSupportsSourceEx2, m_fSupportsCaptcha, m_fDirectUDPCallback);
					m_fRequestsCryptLayer &= m_fSupportsCryptLayer;
					m_fRequiresCryptLayer &= m_fRequestsCryptLayer;

				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			case CT_EMULE_VERSION:
				//  8 Compatible Client ID
				//  7 Mjr Version (Doesn't really matter..)
				//  7 Min Version (Only need 0-99)
				//  3 Upd Version (Only need 0-5)
				//  7 Bld Version (Only need 0-99) -- currently not used
				if (temptag.IsInt()) {
					m_byCompatibleClient = (uint8)((temptag.GetInt() >> 24));
					m_nClientVersion = temptag.GetInt() & 0x00ffffff;
					m_byEmuleVersion = 0x99;
					m_fSharedDirectories = 1;
					dwEmuleTags |= 4;
					if (bDbgInfo)
						m_strHelloInfo.AppendFormat(_T("\n  ClientVer=%u.%u.%u.%u  Comptbl=%u"), (m_nClientVersion >> 17) & 0x7f, (m_nClientVersion >> 10) & 0x7f, (m_nClientVersion >> 7) & 0x07, m_nClientVersion & 0x7f, m_byCompatibleClient);
				}
				else if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;

			default:
				// Since eDonkeyHybrid 1.3 is no longer sending the additional Int32 at the end of the Hello packet,
				// we use the "pr=1" tag to determine them.
				if (temptag.GetName() && temptag.GetName()[0]=='p' && temptag.GetName()[1]=='r') {
					bPrTag = true;
				}
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkTag=%s"), temptag.GetFullInfo());
		}
	}
	m_nUserPort = nUserPort;
	m_dwServerIP = data->ReadUInt32();
	m_nServerPort = data->ReadUInt16();
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("\n  Server=%s:%u"), ipstr(m_dwServerIP), m_nServerPort);

	// Check for additional data in Hello packet to determine client's software version.
	//
	// *) eDonkeyHybrid 0.40 - 1.2 sends an additional Int32. (Since 1.3 they don't send it any longer.)
	// *) MLdonkey sends an additional Int32
	//
	if (data->GetLength() - data->GetPosition() == sizeof(uint32)){
		uint32 test = data->ReadUInt32();
		if (test == 'KDLM'){
			m_bIsML = true;
			if (bDbgInfo)
				m_strHelloInfo += _T("\n  ***AddData: \"MLDK\"");
		}
		else{
			m_bIsHybrid = true;
			if (bDbgInfo)
				m_strHelloInfo.AppendFormat(_T("\n  ***AddData: uint32=%u (0x%08x)"), test, test);
		}
	}
	else if (bDbgInfo && data->GetPosition() < data->GetLength()){
		UINT uAddHelloDataSize = (UINT)(data->GetLength() - data->GetPosition());
		if (uAddHelloDataSize == sizeof(uint32)){
			DWORD dwAddHelloInt32 = data->ReadUInt32();
			m_strHelloInfo.AppendFormat(_T("\n  ***AddData: uint32=%u (0x%08x)"), dwAddHelloInt32, dwAddHelloInt32);
		}
		else if (uAddHelloDataSize == sizeof(uint32)+sizeof(uint16)){
			DWORD dwAddHelloInt32 = data->ReadUInt32();
			WORD w = data->ReadUInt16();
			m_strHelloInfo.AppendFormat(_T("\n  ***AddData: uint32=%u (0x%08x),  uint16=%u (0x%04x)"), dwAddHelloInt32, dwAddHelloInt32, w, w);
		}
		else
			m_strHelloInfo.AppendFormat(_T("\n  ***AddData: %u bytes"), uAddHelloDataSize);
	}

	SOCKADDR_IN sockAddr = {0};
	int nSockAddrLen = sizeof(sockAddr);
	socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
	SetIP(sockAddr.sin_addr.S_un.S_addr);

	if (thePrefs.GetAddServersFromClients() && m_dwServerIP && m_nServerPort){
		CServer* addsrv = new CServer(m_nServerPort, ipstr(m_dwServerIP));
		addsrv->SetListName(addsrv->GetAddress());
		addsrv->SetPreference(SRV_PR_LOW);
		if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(addsrv, true))
			delete addsrv;
	}

	//(a)If this is a highID user, store the ID in the Hybrid format.
	//(b)Some older clients will not send a ID, these client are HighID users that are not connected to a server.
	//(c)Kad users with a *.*.*.0 IPs will look like a lowID user they are actually a highID user.. They can be detected easily
	//because they will send a ID that is the same as their IP..
	if(!HasLowID() || m_nUserIDHybrid == 0 || m_nUserIDHybrid == m_dwUserIP ) 
		m_nUserIDHybrid = ntohl(m_dwUserIP);

	CClientCredits* pFoundCredits = theApp.clientcredits->GetCredit(m_achUserHash);
	if (credits == NULL){
		credits = pFoundCredits;
		if (!theApp.clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits)){
			if (thePrefs.GetLogBannedClients())
				AddDebugLogLine(false, _T("Clients: %s (%s), Banreason: Userhash changed (Found in TrackedClientsList)"), GetUserName(), ipstr(GetConnectIP()));
			Ban();
		}	
	}
	else if (credits != pFoundCredits){
		// userhash change ok, however two hours "waittime" before it can be used
		credits = pFoundCredits;
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false, _T("Clients: %s (%s), Banreason: Userhash changed"), GetUserName(), ipstr(GetConnectIP()));
		Ban();
	}


	if (GetFriend() != NULL && GetFriend()->HasUserhash() && md4cmp(GetFriend()->m_abyUserhash, m_achUserHash) != 0)
	{
		// this isnt our friend anymore and it will be removed/replaced, tell our friendobject about it
		if (GetFriend()->IsTryingToConnect())
			GetFriend()->UpdateFriendConnectionState(FCR_USERHASHFAILED); // this will remove our linked friend
		else
			GetFriend()->SetLinkedClient(NULL);
	}
	// do not replace friendobjects which have no userhash, but the fitting ip with another friend object with the 
	// fitting userhash (both objects would fit to this instance), as this could lead to unwanted results
	if (GetFriend() == NULL || GetFriend()->HasUserhash() || GetFriend()->m_dwLastUsedIP != GetConnectIP()
		|| GetFriend()->m_nLastUsedPort != GetUserPort())
	{
		if ((m_Friend = theApp.friendlist->SearchFriend(m_achUserHash, m_dwUserIP, m_nUserPort)) != NULL){
			// Link the friend to that client
			m_Friend->SetLinkedClient(this);
		}
		else{
			// avoid that an unwanted client instance keeps a friend slot
			SetFriendSlot(false);
		}
	}
	else{
		// however, copy over our userhash in this case
		md4cpy(GetFriend()->m_abyUserhash, m_achUserHash);
	}


	// check for known major gpl breaker
	CString strBuffer = m_pszUsername;
	strBuffer.MakeUpper();
	strBuffer.Remove(_T(' '));
	if (strBuffer.Find(_T("EMULE-CLIENT")) != -1 || strBuffer.Find(_T("POWERMULE")) != -1 ){
		m_bGPLEvildoer = true;  
	}

	m_byInfopacketsReceived |= IP_EDONKEYPROTPACK;
	// check if at least CT_EMULEVERSION was received, all other tags are optional
	bool bIsMule = (dwEmuleTags & 0x04) == 0x04;
	if (bIsMule){
		m_bEmuleProtocol = true;
		m_byInfopacketsReceived |= IP_EMULEPROTPACK;
	}
	else if (bPrTag){
		m_bIsHybrid = true;
	}

	InitClientSoftwareVersion();

	if (m_bIsHybrid)
		m_fSharedDirectories = 1;

	if (thePrefs.GetVerbose() && GetServerIP() == INADDR_NONE)
		AddDebugLogLine(false, _T("Received invalid server IP %s from %s"), ipstr(GetServerIP()), DbgGetClientInfo());

	return bIsMule;
}

void CUpDownClient::SendHelloPacket(){
	if (socket == NULL){
		ASSERT(0);
		return;
	}

	CSafeMemFile data(128);
	data.WriteUInt8(16); // size of userhash
	SendHelloTypePacket(&data);
	Packet* packet = new Packet(&data);
	packet->opcode = OP_HELLO;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__Hello", this);
	theStats.AddUpDataOverheadOther(packet->size);
	SendPacket(packet, true);

	m_bHelloAnswerPending = true;
	return;
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer){
	if (socket == NULL){
		ASSERT(0);
		return;
	}

	CSafeMemFile data(128);
	data.WriteUInt8((uint8)theApp.m_uCurVersionShort);
	data.WriteUInt8(EMULE_PROTOCOL);
	data.WriteUInt32(7); // nr. of tags
	CTag tag(ET_COMPRESSION,1);
	tag.WriteTagToFile(&data);
	CTag tag2(ET_UDPVER,4);
	tag2.WriteTagToFile(&data);
	CTag tag3(ET_UDPPORT,thePrefs.GetUDPPort());
	tag3.WriteTagToFile(&data);
	CTag tag4(ET_SOURCEEXCHANGE,3);
	tag4.WriteTagToFile(&data);
	CTag tag5(ET_COMMENTS,1);
	tag5.WriteTagToFile(&data);
	CTag tag6(ET_EXTENDEDREQUEST,2);
	tag6.WriteTagToFile(&data);

	uint32 dwTagValue = (theApp.clientcredits->CryptoAvailable() ? 3 : 0);
	if (thePrefs.CanSeeShares() != vsfaNobody) // set 'Preview supported' only if 'View Shared Files' allowed
		dwTagValue |= 128;
	CTag tag7(ET_FEATURES, dwTagValue);
	tag7.WriteTagToFile(&data);

	Packet* packet = new Packet(&data,OP_EMULEPROT);
	if (!bAnswer)
		packet->opcode = OP_EMULEINFO;
	else
		packet->opcode = OP_EMULEINFOANSWER;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend(!bAnswer ? "OP__EmuleInfo" : "OP__EmuleInfoAnswer", this);
	theStats.AddUpDataOverheadOther(packet->size);
	SendPacket(packet, true);
}

void CUpDownClient::ProcessMuleInfoPacket(const uchar* pachPacket, uint32 nSize)
{
	bool bDbgInfo = thePrefs.GetUseDebugDevice();
	m_strMuleInfo.Empty();
	CSafeMemFile data(pachPacket, nSize);
	m_byCompatibleClient = 0;
	m_byEmuleVersion = data.ReadUInt8();
	if (bDbgInfo)
		m_strMuleInfo.AppendFormat(_T("EmuleVer=0x%x"), (UINT)m_byEmuleVersion);
	if (m_byEmuleVersion == 0x2B)
		m_byEmuleVersion = 0x22;
	uint8 protversion = data.ReadUInt8();
	if (bDbgInfo)
		m_strMuleInfo.AppendFormat(_T("  ProtVer=%u"), (UINT)protversion);

	//implicitly supported options by older clients
	if (protversion == EMULE_PROTOCOL) {
		//in the future do not use version to guess about new features

		if (m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x22)
			m_byUDPVer = 1;

		if (m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x21)
			m_bySourceExchange1Ver = 1;

		if (m_byEmuleVersion == 0x24)
			m_byAcceptCommentVer = 1;

		// Shared directories are requested from eMule 0.28+ because eMule 0.27 has a bug in 
		// the OP_ASKSHAREDFILESDIR handler, which does not return the shared files for a 
		// directory which has a trailing backslash.
		if (m_byEmuleVersion >= 0x28 && !m_bIsML) // MLdonkey currently does not support shared directories
			m_fSharedDirectories = 1;

	} else {
		return;
	}

	uint32 tagcount = data.ReadUInt32();
	if (bDbgInfo)
		m_strMuleInfo.AppendFormat(_T("  Tags=%u"), (UINT)tagcount);
	for (uint32 i = 0; i < tagcount; i++)
	{
		CTag temptag(&data, false);
		switch (temptag.GetNameID())
		{
			case ET_COMPRESSION:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: data compression version
				if (temptag.IsInt()) {
					m_byDataCompVer = (uint8)temptag.GetInt();
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  Compr=%u"), (UINT)temptag.GetInt());
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_UDPPORT:
				// Bits 31-16: 0 - reserved
				// Bits 15- 0: UDP port
				if (temptag.IsInt()) {
					m_nUDPPort = (uint16)temptag.GetInt();
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  UDPPort=%u"), (UINT)temptag.GetInt());
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_UDPVER:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: UDP protocol version
				if (temptag.IsInt()) {
					m_byUDPVer = (uint8)temptag.GetInt();
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  UDPVer=%u"), (UINT)temptag.GetInt());
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_SOURCEEXCHANGE:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: source exchange protocol version
				if (temptag.IsInt()) {
					m_bySourceExchange1Ver = (uint8)temptag.GetInt();
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  SrcExch=%u"), (UINT)temptag.GetInt());
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_COMMENTS:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: comments version
				if (temptag.IsInt()) {
					m_byAcceptCommentVer = (uint8)temptag.GetInt();
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  Commnts=%u"), (UINT)temptag.GetInt());
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_EXTENDEDREQUEST:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: extended requests version
				if (temptag.IsInt()) {
					m_byExtendedRequestsVer = (uint8)temptag.GetInt();
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  ExtReq=%u"), (UINT)temptag.GetInt());
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_COMPATIBLECLIENT:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: compatible client ID
				if (temptag.IsInt()) {
					m_byCompatibleClient = (uint8)temptag.GetInt();
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  Comptbl=%u"), (UINT)temptag.GetInt());
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_FEATURES:
				// Bits 31- 8: 0 - reserved
				// Bit	    7: Preview
				// Bit   6- 0: secure identification
				if (temptag.IsInt()) {
					m_bySupportSecIdent = (uint8)((temptag.GetInt()) & 3);
					m_fSupportsPreview  = (temptag.GetInt() >> 7) & 1;
					if (bDbgInfo)
						m_strMuleInfo.AppendFormat(_T("\n  SecIdent=%u  Preview=%u"), m_bySupportSecIdent, m_fSupportsPreview);
				}
				else if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkType=%s"), temptag.GetFullInfo());
				break;
			
			case ET_MOD_VERSION:
				if (temptag.IsStr())
					m_strModVersion = temptag.GetStr();
				else if (temptag.IsInt())
					m_strModVersion.Format(_T("ModID=%u"), temptag.GetInt());
				else
					m_strModVersion = _T("ModID=<Unknwon>");
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ModID=%s"), m_strModVersion);
				CheckForGPLEvilDoer();
				break;
			
			default:
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkTag=%s"), temptag.GetFullInfo());
		}
	}
	if (m_byDataCompVer == 0) {
		m_bySourceExchange1Ver = 0;
		m_byExtendedRequestsVer = 0;
		m_byAcceptCommentVer = 0;
		m_nUDPPort = 0;
	}
	if (bDbgInfo && data.GetPosition() < data.GetLength()) {
		m_strMuleInfo.AppendFormat(_T("\n  ***AddData: %u bytes"), data.GetLength() - data.GetPosition());
	}

	m_bEmuleProtocol = true;
	m_byInfopacketsReceived |= IP_EMULEPROTPACK;
	InitClientSoftwareVersion();

	if (thePrefs.GetVerbose() && GetServerIP() == INADDR_NONE)
		AddDebugLogLine(false, _T("Received invalid server IP %s from %s"), ipstr(GetServerIP()), DbgGetClientInfo());
}

void CUpDownClient::SendHelloAnswer(){
	if (socket == NULL){
		ASSERT(0);
		return;
	}

	CSafeMemFile data(128);
	SendHelloTypePacket(&data);
	Packet* packet = new Packet(&data);
	packet->opcode = OP_HELLOANSWER;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__HelloAnswer", this);
	theStats.AddUpDataOverheadOther(packet->size);

	// Servers send a FIN right in the data packet on check connection, so we need to force the response immediate
	bool bForceSend = theApp.serverconnect->AwaitingTestFromIP(GetConnectIP());
	socket->SendPacket(packet, true, true, 0, bForceSend);

	m_bHelloAnswerPending = false;
}

void CUpDownClient::SendHelloTypePacket(CSafeMemFile* data)
{
	data->WriteHash16(thePrefs.GetUserHash());
	uint32 clientid;
	clientid = theApp.GetID();

	data->WriteUInt32(clientid);
	data->WriteUInt16(thePrefs.GetPort());

	uint32 tagcount = 6;

	if( theApp.clientlist->GetBuddy() && theApp.IsFirewalled() )
		tagcount += 2;

	data->WriteUInt32(tagcount);

	// eD2K Name

	// TODO implement multi language website which informs users of the effects of bad mods
	CTag tagName(CT_NAME, (!m_bGPLEvildoer) ? thePrefs.GetUserNick() : _T("Please use a GPL-conform version of eMule") );
	tagName.WriteTagToFile(data, utf8strRaw);

	// eD2K Version
	CTag tagVersion(CT_VERSION,EDONKEYVERSION);
	tagVersion.WriteTagToFile(data);

	// eMule UDP Ports
	uint32 kadUDPPort = 0;
	if(Kademlia::CKademlia::IsConnected())
	{
		if (Kademlia::CKademlia::GetPrefs()->GetExternalKadPort() != 0 
			&& Kademlia::CKademlia::GetPrefs()->GetUseExternKadPort()
			&& Kademlia::CUDPFirewallTester::IsVerified())
		{
			kadUDPPort = Kademlia::CKademlia::GetPrefs()->GetExternalKadPort();
		}
		else
			kadUDPPort = Kademlia::CKademlia::GetPrefs()->GetInternKadPort();
	}
	CTag tagUdpPorts(CT_EMULE_UDPPORTS, 
				((uint32)kadUDPPort			   << 16) |
				((uint32)thePrefs.GetUDPPort() <<  0)
				); 
	tagUdpPorts.WriteTagToFile(data);
	
	if( theApp.clientlist->GetBuddy() && theApp.IsFirewalled() )
	{
		CTag tagBuddyIP(CT_EMULE_BUDDYIP, theApp.clientlist->GetBuddy()->GetIP() ); 
		tagBuddyIP.WriteTagToFile(data);
	
		CTag tagBuddyPort(CT_EMULE_BUDDYUDP, 
//					( RESERVED												)
					((uint32)theApp.clientlist->GetBuddy()->GetUDPPort()  ) 
					);
		tagBuddyPort.WriteTagToFile(data);
	}

	// eMule Misc. Options #1
	const UINT uUdpVer				= 4;
	const UINT uDataCompVer			= 1;
	const UINT uSupportSecIdent		= theApp.clientcredits->CryptoAvailable() ? 3 : 0;
	// ***
	// deprecated - will be set back to 3 with the next release (to allow the new version to spread first),
	// due to a bug in earlier eMule version. Use SupportsSourceEx2 and new opcodes instead
	const UINT uSourceExchange1Ver	= 4;
	// ***
	const UINT uExtendedRequestsVer	= 2;
	const UINT uAcceptCommentVer	= 1;
	const UINT uNoViewSharedFiles	= (thePrefs.CanSeeShares() == vsfaNobody) ? 1 : 0; // for backward compatibility this has to be a 'negative' flag
	const UINT uMultiPacket			= 1;
	const UINT uSupportPreview		= (thePrefs.CanSeeShares() != vsfaNobody) ? 1 : 0; // set 'Preview supported' only if 'View Shared Files' allowed
	const UINT uPeerCache			= 1;
	const UINT uUnicodeSupport		= 1;
	const UINT nAICHVer				= 1;
	CTag tagMisOptions1(CT_EMULE_MISCOPTIONS1, 
				(nAICHVer				<< 29) |
				(uUnicodeSupport		<< 28) |
				(uUdpVer				<< 24) |
				(uDataCompVer			<< 20) |
				(uSupportSecIdent		<< 16) |
				(uSourceExchange1Ver	<< 12) |
				(uExtendedRequestsVer	<<  8) |
				(uAcceptCommentVer		<<  4) |
				(uPeerCache				<<  3) |
				(uNoViewSharedFiles		<<  2) |
				(uMultiPacket			<<  1) |
				(uSupportPreview		<<  0)
				);
	tagMisOptions1.WriteTagToFile(data);

	// eMule Misc. Options #2
	const UINT uKadVersion			= KADEMLIA_VERSION;
	const UINT uSupportLargeFiles	= 1;
	const UINT uExtMultiPacket		= 1;
	const UINT uReserved			= 0; // mod bit
	const UINT uSupportsCryptLayer	= thePrefs.IsClientCryptLayerSupported() ? 1 : 0;
	const UINT uRequestsCryptLayer	= thePrefs.IsClientCryptLayerRequested() ? 1 : 0;
	const UINT uRequiresCryptLayer	= thePrefs.IsClientCryptLayerRequired() ? 1 : 0;
	const UINT uSupportsSourceEx2	= 1;
	const UINT uSupportsCaptcha		= 1;
	// direct callback is only possible if connected to kad, tcp firewalled and verified UDP open (for example on a full cone NAT)
	const UINT uDirectUDPCallback	= (Kademlia::CKademlia::IsRunning() && Kademlia::CKademlia::IsFirewalled()
		&& !Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) && Kademlia::CUDPFirewallTester::IsVerified()) ? 1 : 0;
	const UINT uFileIdentifiers		= 1;

	CTag tagMisOptions2(CT_EMULE_MISCOPTIONS2, 
//				(RESERVED				     )
				(uFileIdentifiers		<< 13) |
				(uDirectUDPCallback		<< 12) |
				(uSupportsCaptcha		<< 11) |
				(uSupportsSourceEx2		<< 10) |
				(uRequiresCryptLayer	<<  9) |
				(uRequestsCryptLayer	<<  8) |
				(uSupportsCryptLayer	<<  7) |
				(uReserved				<<  6) |
				(uExtMultiPacket		<<  5) |
				(uSupportLargeFiles		<<  4) |
				(uKadVersion			<<  0) 
				);
	tagMisOptions2.WriteTagToFile(data);

	// eMule Version
	CTag tagMuleVersion(CT_EMULE_VERSION, 
				//(uCompatibleClientID		<< 24) |
				(CemuleApp::m_nVersionMjr	<< 17) |
				(CemuleApp::m_nVersionMin	<< 10) |
				(CemuleApp::m_nVersionUpd	<<  7) 
//				(RESERVED			     ) 
				);
	tagMuleVersion.WriteTagToFile(data);

	uint32 dwIP;
	uint16 nPort;
	if (theApp.serverconnect->IsConnected()){
		dwIP = theApp.serverconnect->GetCurrentServer()->GetIP();
		nPort = theApp.serverconnect->GetCurrentServer()->GetPort();
#ifdef _DEBUG
		if (dwIP == theApp.serverconnect->GetLocalIP()){
			dwIP = 0;
			nPort = 0;
		}
#endif
	}
	else{
		nPort = 0;
		dwIP = 0;
	}
	data->WriteUInt32(dwIP);
	data->WriteUInt16(nPort);
//	data->WriteUInt32(dwIP); //The Hybrid added some bits here, what ARE THEY FOR?
}

void CUpDownClient::ProcessMuleCommentPacket(const uchar* pachPacket, uint32 nSize)
{
	if (reqfile && reqfile->IsPartFile())
	{
		CSafeMemFile data(pachPacket, nSize);
		uint8 uRating = data.ReadUInt8();
		if (thePrefs.GetLogRatingDescReceived() && uRating > 0)
			AddDebugLogLine(false, GetResString(IDS_RATINGRECV), m_strClientFilename, uRating);
		CString strComment;
		UINT uLength = data.ReadUInt32();
		if (uLength > 0)
		{
			// we have to increase the raw max. allowed file comment len because of possible UTF8 encoding.
			if (uLength > MAXFILECOMMENTLEN*4)
				uLength = MAXFILECOMMENTLEN*4;
			strComment = data.ReadString(GetUnicodeSupport()!=utf8strNone, uLength);

			if (strComment.GetLength() > MAXFILECOMMENTLEN) // enforce the max len on the comment
				strComment = strComment.Left(MAXFILECOMMENTLEN);

			if (thePrefs.GetLogRatingDescReceived() && !strComment.IsEmpty())
				AddDebugLogLine(false, GetResString(IDS_DESCRIPTIONRECV), m_strClientFilename, strComment);

			// test if comment is filtered
			if (!thePrefs.GetCommentFilter().IsEmpty())
			{
				CString strCommentLower(strComment);
				strCommentLower.MakeLower();

				int iPos = 0;
				CString strFilter(thePrefs.GetCommentFilter().Tokenize(_T("|"), iPos));
				while (!strFilter.IsEmpty())
				{
					// comment filters are already in lowercase, compare with temp. lowercased received comment
					if (strCommentLower.Find(strFilter) >= 0)
					{
						strComment.Empty();
						uRating = 0;
						SetSpammer(true);
						break;
					}
					strFilter = thePrefs.GetCommentFilter().Tokenize(_T("|"), iPos);
				}
			}
		}
		if (!strComment.IsEmpty() || uRating > 0)
		{
			m_strFileComment = strComment;
			m_uFileRating = uRating;
			reqfile->UpdateFileRatingCommentAvail();
		}
	}
}

bool CUpDownClient::Disconnected(LPCTSTR pszReason, bool bFromSocket)
{
	ASSERT( theApp.clientlist->IsValidClient(this) );
	
	// TODO LOGREMOVE
	if (m_nConnectingState == CCS_DIRECTCALLBACK)
		DebugLog(_T("Direct Callback failed - %s"), DbgGetClientInfo());
	
	if (GetKadState() == KS_QUEUED_FWCHECK_UDP || GetKadState() == KS_CONNECTING_FWCHECK_UDP)
		Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, true, ntohl(GetConnectIP()), 0); // inform the tester that this test was cancelled
	else if (GetKadState() == KS_FWCHECK_UDP)
		Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, false, ntohl(GetConnectIP()), 0); // inform the tester that this test has failed
	else if (GetKadState() == KS_CONNECTED_BUDDY)
		DebugLogWarning(_T("Buddy client disconnected - %s, %s"), pszReason, DbgGetClientInfo());
	//If this is a KAD client object, just delete it!
	SetKadState(KS_NONE);

    if (GetUploadState() == US_UPLOADING || GetUploadState() == US_CONNECTING)
	{
		// sets US_NONE
		theApp.uploadqueue->RemoveFromUploadQueue(this, CString(_T("CUpDownClient::Disconnected: ")) + pszReason);
	}

	if (GetDownloadState() == DS_DOWNLOADING){
		ASSERT( m_nConnectingState == CCS_NONE );
		if (m_ePeerCacheDownState == PCDS_WAIT_CACHE_REPLY || m_ePeerCacheDownState == PCDS_DOWNLOADING)
			theApp.m_pPeerCache->DownloadAttemptFailed();
		SetDownloadState(DS_ONQUEUE, CString(_T("Disconnected: ")) + pszReason);
	}
	else{
		// ensure that all possible block requests are removed from the partfile
		ClearDownloadBlockRequests();
		if (GetDownloadState() == DS_CONNECTED){ // successfully connected, but probably didn't responsed to our filerequest
			theApp.clientlist->m_globDeadSourceList.AddDeadSource(this);
			theApp.downloadqueue->RemoveSource(this);
	    }
	}

	// we had still an AICH request pending, handle it
	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHRecoveryHashSet::ClientAICHRequestFailed(this);
	}

	// The remote client does not have to answer with OP_HASHSETANSWER *immediatly* 
	// after we've sent OP_HASHSETREQUEST. It may occure that a (buggy) remote client 
	// is sending use another OP_FILESTATUS which would let us change to DL-state to DS_ONQUEUE.
	if (m_fHashsetRequestingMD4 && (reqfile != NULL))
        reqfile->m_bMD4HashsetNeeded = true;
	if (m_fHashsetRequestingAICH && (reqfile != NULL))
        reqfile->SetAICHHashSetNeeded(true);

    if (m_iFileListRequested){
		LogWarning(LOG_STATUSBAR, GetResString(IDS_SHAREDFILES_FAILED), GetUserName());
        m_iFileListRequested = 0;
	}

	if (m_Friend)
		theApp.friendlist->RefreshFriend(m_Friend);

	ASSERT( theApp.clientlist->IsValidClient(this) );

	//check if this client is needed in any way, if not delete it
	bool bDelete = true;
	switch(m_nUploadState){
		case US_ONUPLOADQUEUE:
			bDelete = false;
			break;
	}
	switch(m_nDownloadState){
		case DS_ONQUEUE:
		case DS_TOOMANYCONNS:
		case DS_NONEEDEDPARTS:
		case DS_LOWTOLOWIP:
			bDelete = false;
	}

	// Dead Soure Handling
	//
	// If we failed to connect to that client, it is supposed to be 'dead'. Add the IP
	// to the 'dead sources' lists so we don't waste resources and bandwidth to connect
	// to that client again within the next hour.
	//
	// But, if we were just connecting to a proxy and failed to do so, that client IP
	// is supposed to be valid until the proxy itself tells us that the IP can not be
	// connected to (e.g. 504 Bad Gateway)
	//
	if ( (m_nConnectingState != CCS_NONE && !(socket && socket->GetProxyConnectFailed()))
		|| m_nDownloadState == DS_ERROR)
	{
		if (m_nDownloadState != DS_NONE) // Unable to connect = Remove any downloadstate
			theApp.downloadqueue->RemoveSource(this);
		theApp.clientlist->m_globDeadSourceList.AddDeadSource(this);
		bDelete = true;
	}

	// We keep chat partners in any case
	if (GetChatState() != MS_NONE){
		bDelete = false;
		if (GetFriend() != NULL && GetFriend()->IsTryingToConnect())
			GetFriend()->UpdateFriendConnectionState(FCR_DISCONNECTED); // for friends any connectionupdate is handled in the friend class
		else
			theApp.emuledlg->chatwnd->chatselector.ConnectingResult(this,false); // other clients update directly
	}
	
	// Delete Socket
	if (!bFromSocket && socket){
		ASSERT( theApp.listensocket->IsValidSocket(socket) );
		socket->Safe_Delete();
	}
	socket = NULL;

	theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);

	// finally, remove the client from the timeouttimer and reset the connecting state
	m_nConnectingState = CCS_NONE;
	theApp.clientlist->RemoveConnectingClient(this);

	if (bDelete)
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			Debug(_T("--- Deleted client            %s; Reason=%s\n"), DbgGetClientInfo(true), pszReason);
		return true;
	}
	else
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			Debug(_T("--- Disconnected client       %s; Reason=%s\n"), DbgGetClientInfo(true), pszReason);
		m_fHashsetRequestingMD4 = 0;
		m_fHashsetRequestingAICH = 0;
		SetSentCancelTransfer(0);
		m_bHelloAnswerPending = false;
		m_fQueueRankPending = 0;
		m_fFailedFileIdReqs = 0;
		m_fUnaskQueueRankRecv = 0;
		m_uPeerCacheDownloadPushId = 0;
		m_uPeerCacheUploadPushId = 0;
		m_uPeerCacheRemoteIP = 0;
		SetPeerCacheDownState(PCDS_NONE);
		SetPeerCacheUpState(PCUS_NONE);
		if (m_pPCDownSocket){
			m_pPCDownSocket->client = NULL;
			m_pPCDownSocket->Safe_Delete();
		}
		if (m_pPCUpSocket){
			m_pPCUpSocket->client = NULL;
			m_pPCUpSocket->Safe_Delete();
		}
		m_fSentOutOfPartReqs = 0;
		return false;
	}
}

//Returned bool is not if the TryToConnect is successful or not..
//false means the client was deleted!
//true means the client was not deleted!
bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon, bool bNoCallbacks, CRuntimeClass* pClassSocket)
{
	// There are 7 possible ways how we are going to connect in this function, sorted by priority:
	// 1) Already Connected/Connecting
	//		We are already connected or try to connect right now. Abort, no additional Disconnect() call will be done
	// 2) Immediate Fail
	//		Some precheck or precondition failed, or no other way is available, so we do not try to connect at all
	//		but fail right away, possibly deleting the client as it becomes useless
	// 3) Normal Outgoing TCP Connection
	//		Applies to all HighIDs/Open clients: We do a straight forward connection try to the TCP port of the client
	// 4) Direct Callback Connections
	//		Applies to TCP firewalled - UDP open clients: We sent a UDP packet to the client, requesting him to connect
	//		to us. This is pretty easy too and ressourcewise nearly on the same level as 3)
	// (* 5) Waiting/Abort
	//		This check is done outside this function.
	//		We want to connect for some download related thing (for example reasking), but the client has a LowID and
	//		is on our uploadqueue. So we are smart and safing ressources by just waiting untill he reasks us, so we don't
	//		have to do the ressource intensive options 6 or 7. *)
	// 6) Server Callback
	//		This client is firewalled, but connected to our server. We sent the server a callback request to forward to
	//		the client and hope for the best
	// 7) Kad Callback
	//		This client is firewalled, but has a Kad buddy. We sent the buddy a callback request to forward to the client
	//		and hope for the best

	if( GetKadState() == KS_QUEUED_FWCHECK )
		SetKadState(KS_CONNECTING_FWCHECK);
	else if (GetKadState() == KS_QUEUED_FWCHECK_UDP)
		SetKadState(KS_CONNECTING_FWCHECK_UDP);

	////////////////////////////////////////////////////////////
	// Check for 1) Already Connected/Connecting
	if (m_nConnectingState != CCS_NONE) {
		DebugLog(_T("TryToConnect: Already Connecting (%s)"), DbgGetClientInfo());// TODO LogRemove
		return true;
	}
	else if (socket != NULL){
		if (socket->IsConnected())
		{
			if (CheckHandshakeFinished()){
				DEBUG_ONLY( DebugLog(_T("TryToConnect: Already Connected (%s)"), DbgGetClientInfo()) );// TODO LogRemove
				ConnectionEstablished();
			}
			else
				DebugLogWarning( _T("TryToConnect found connected socket, but without Handshake finished - %s"), DbgGetClientInfo());
			return true;
		}
		else
			socket->Safe_Delete();
	}
	m_nConnectingState = CCS_PRECONDITIONS; // We now officially try to connect :)

	////////////////////////////////////////////////////////////
	// Check for 2) Immediate Fail

	if (theApp.listensocket->TooManySockets() && !bIgnoreMaxCon)
	{
		// This is a sanitize check and counts as a "hard failure", so this check should be also done before calling
		// TryToConnect if a special handling, like waiting till there are enough connection avaiable should be fone
		DebugLogWarning(_T("TryToConnect: Too many connections sanitize check (%s)"), DbgGetClientInfo());
		if(Disconnected(_T("Too many connections")))
		{
			delete this;
			return false;
		}
		return true;
	}
	// do not try to connect to source which are incompatible with our encryption setting (one requires it, and the other one doesn't supports it)
	if ( (RequiresCryptLayer() && !thePrefs.IsClientCryptLayerSupported()) || (thePrefs.IsClientCryptLayerRequired() && !SupportsCryptLayer()) )
	{
		DEBUG_ONLY( AddDebugLogLine(DLP_DEFAULT, false, _T("Rejected outgoing connection because CryptLayer-Setting (Obfuscation) was incompatible %s"), DbgGetClientInfo()) );
		if(Disconnected(_T("CryptLayer-Settings (Obfuscation) incompatible"))){
			delete this;
			return false;
		}
		else
			return true;
	}

	uint32 uClientIP = (GetIP() != 0) ? GetIP() : GetConnectIP();
	if (uClientIP == 0 && !HasLowID())
		uClientIP = ntohl(m_nUserIDHybrid);
	if (uClientIP)
	{
		// although we filter all received IPs (server sources, source exchange) and all incomming connection attempts,
		// we do have to filter outgoing connection attempts here too, because we may have updated the ip filter list
		if (theApp.ipfilter->IsFiltered(uClientIP))
		{
			theStats.filteredclients++;
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(true, GetResString(IDS_IPFILTERED), ipstr(uClientIP), theApp.ipfilter->GetLastHit());
			if (Disconnected(_T("IPFilter")))
			{
				delete this;
				return false;
			}
			return true;
		}

		// for safety: check again whether that IP is banned
		if (theApp.clientlist->IsBannedClient(uClientIP))
		{
			if (thePrefs.GetLogBannedClients())
				AddDebugLogLine(false, _T("Refused to connect to banned client %s"), DbgGetClientInfo());
			if (Disconnected(_T("Banned IP")))
			{
				delete this;
				return false;
			}
			return true;
		}
	}

	if ( HasLowID() )
	{
		ASSERT( pClassSocket == NULL );
		if(!theApp.CanDoCallback(this)) // lowid2lowid check used for the whole function, don't remove
		{
			// We cannot reach this client, so we hard fail to connect, if this client should be kept,
			// for example because we might want to wait a bit and hope we get a highid, this check has
			// to be done before calling this function
			if(Disconnected(_T("LowID->LowID")))
			{
				delete this;
				return false;
			}
			return true;
		}

		// are callbacks disallowed?
		if (bNoCallbacks){
			DebugLogError(_T("TryToConnect: Would like to do callback on a no-callback client, %s"), DbgGetClientInfo());
			if(Disconnected(_T("LowID: No Callback Option allowed")))
			{
				delete this;
				return false;
			}
			return true;
		}

		// Is any callback available?
		if (!( (SupportsDirectUDPCallback() && thePrefs.GetUDPPort() != 0 && GetConnectIP() != 0) // Direct Callback
			|| (HasValidBuddyID() && Kademlia::CKademlia::IsConnected() && ((GetBuddyIP() && GetBuddyPort()) || reqfile != NULL)) // Kad Callback
			|| theApp.serverconnect->IsLocalServer(GetServerIP(), GetServerPort()) )) // Server Callback
		{
			// Nope
			if(Disconnected(_T("LowID: No Callback Option available")))
			{
				delete this;
				return false;
			}
			return true;
		}
	}

	// Prechecks finished, now for the real connecting
	////////////////////////////////////////////////////

	theApp.clientlist->AddConnectingClient(this); // Starts and checks for the timeout, ensures following Disconnect() or ConnectionEstablished() call

	////////////////////////////////////////////////////////////
	// 3) Normal Outgoing TCP Connection
	if (!HasLowID())
	{
		m_nConnectingState = CCS_DIRECTTCP;
		if (pClassSocket == NULL)
			pClassSocket = RUNTIME_CLASS(CClientReqSocket);
		socket = static_cast<CClientReqSocket*>(pClassSocket->CreateObject());
		socket->SetClient(this);
		if (!socket->Create())
		{
			socket->Safe_Delete();
			// we let the timeout handle the cleanup in this case
			DebugLogError(_T("TryToConnect: Failed to create socket for outgoing connection, %s"), DbgGetClientInfo());
		}
		else
			Connect();
		return true;
	}
	////////////////////////////////////////////////////////////
	// 4) Direct Callback Connections
	else if (SupportsDirectUDPCallback() && thePrefs.GetUDPPort() != 0 && GetConnectIP() != 0)
	{
		m_nConnectingState = CCS_DIRECTCALLBACK;
		// TODO LOGREMOVE
		DebugLog(_T("Direct Callback on port %u to client %s (%s) "), GetKadPort(), DbgGetClientInfo(), md4str(GetUserHash()));
		CSafeMemFile data;
		data.WriteUInt16(thePrefs.GetPort()); // needs to know our port
		data.WriteHash16(thePrefs.GetUserHash()); // and userhash
		// our connection settings
		data.WriteUInt8(GetMyConnectOptions(true, false));
		if (thePrefs.GetDebugClientUDPLevel() > 0)
			DebugSend("OP_DIRECTCALLBACKREQ", this);
		Packet* packet = new Packet(&data, OP_EMULEPROT);
		packet->opcode = OP_DIRECTCALLBACKREQ;
		theStats.AddUpDataOverheadOther(packet->size);
		theApp.clientudp->SendPacket(packet, GetConnectIP(), GetKadPort(), ShouldReceiveCryptUDPPackets(), GetUserHash(), false, 0);
		return true;
	}
	////////////////////////////////////////////////////////////
	// 6) Server Callback + 7) Kad Callback
	if (GetDownloadState() == DS_CONNECTING)
		SetDownloadState(DS_WAITCALLBACK);
	
	if (GetUploadState() == US_CONNECTING){
		ASSERT( false ); // we should never try to connect in this case, but wait for the LowID to connect to us
		DebugLogError( _T("LowID and US_CONNECTING (%s)"), DbgGetClientInfo());
	}

	if (theApp.serverconnect->IsLocalServer(m_dwServerIP, m_nServerPort))
	{
		m_nConnectingState = CCS_SERVERCALLBACK;
		Packet* packet = new Packet(OP_CALLBACKREQUEST,4);
		PokeUInt32(packet->pBuffer, m_nUserIDHybrid);
		if (thePrefs.GetDebugServerTCPLevel() > 0 || thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__CallbackRequest", this);
		theStats.AddUpDataOverheadServer(packet->size);
		theApp.serverconnect->SendPacket(packet);
		return true;
	}
	else if (HasValidBuddyID() && Kademlia::CKademlia::IsConnected() && ((GetBuddyIP() && GetBuddyPort()) || reqfile != NULL))
	{
		m_nConnectingState = CCS_KADCALLBACK;
		if( GetBuddyIP() && GetBuddyPort())
		{
			CSafeMemFile bio(34);
			bio.WriteUInt128(&Kademlia::CUInt128(GetBuddyID()));
			bio.WriteUInt128(&Kademlia::CUInt128(reqfile->GetFileHash()));
			bio.WriteUInt16(thePrefs.GetPort());
			if (thePrefs.GetDebugClientKadUDPLevel() > 0 || thePrefs.GetDebugClientUDPLevel() > 0)
				DebugSend("KadCallbackReq", this);
			Packet* packet = new Packet(&bio, OP_KADEMLIAHEADER);
			packet->opcode = KADEMLIA_CALLBACK_REQ;
			theStats.AddUpDataOverheadKad(packet->size);
			// FIXME: We dont know which kadversion the buddy has, so we need to send unencrypted
			theApp.clientudp->SendPacket(packet, GetBuddyIP(), GetBuddyPort(), false, NULL, true, 0);
			SetDownloadState(DS_WAITCALLBACKKAD);
		}
		else if (reqfile != NULL)
		{
			// I don't think we should ever have a buddy without its IP (anymore), but nevertheless let the functionality in
			//Create search to find buddy.
			Kademlia::CSearch *findSource = new Kademlia::CSearch;
			findSource->SetSearchTypes(Kademlia::CSearch::FINDSOURCE);
			findSource->SetTargetID(Kademlia::CUInt128(GetBuddyID()));
			findSource->AddFileID(Kademlia::CUInt128(reqfile->GetFileHash()));
			if( Kademlia::CKademlia::GetPrefs()->GetTotalSource() > 0 || Kademlia::CSearchManager::AlreadySearchingFor(Kademlia::CUInt128(GetBuddyID())))
			{
				//There are too many source lookups already or we are already searching this key.
				// bad luck, as lookups aren't supposed to hapen anyway, we just let it fail, if we want
				// to actually really use lookups (so buddies without known IPs), this should be reworked
				// for example by adding a queuesystem for queries
				DebugLogWarning(_T("TryToConnect: Buddy without knonw IP, Lookup crrently impossible"));
				return true;
			}
			if(Kademlia::CSearchManager::StartSearch(findSource))
			{
				//Started lookup..
				SetDownloadState(DS_WAITCALLBACKKAD);
			}
			else
			{
				//This should never happen..
				ASSERT(0);
			}
		}
		return true;
	}
	else {
		ASSERT( false );
		DebugLogError(_T("TryToConnect: Bug: No Callback available despite prechecks"));
		return true;
	}
}

void CUpDownClient::Connect()
{
	// enable or disable crypting based on our and the remote clients preference
	if (HasValidHash() && SupportsCryptLayer() && thePrefs.IsClientCryptLayerSupported() && (RequestsCryptLayer() || thePrefs.IsClientCryptLayerRequested())){
		//DebugLog(_T("Enabling CryptLayer on outgoing connection to client %s"), DbgGetClientInfo()); // to be removed later
		socket->SetConnectionEncryption(true, GetUserHash(), false);
	}
	else
		socket->SetConnectionEncryption(false, NULL, false);

	//Try to always tell the socket to WaitForOnConnect before you call Connect.
	socket->WaitForOnConnect();
	SOCKADDR_IN sockAddr = {0};
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(GetUserPort());
	sockAddr.sin_addr.S_un.S_addr = GetConnectIP();
	socket->Connect((SOCKADDR*)&sockAddr, sizeof sockAddr);
	SendHelloPacket();
}

void CUpDownClient::ConnectionEstablished()
{
	// ok we have a connection, lets see if we want anything from this client
	
	// was this a direct callback?
	if (m_nConnectingState == CCS_DIRECTCALLBACK) // TODO LOGREMOVE
		DebugLog(_T("Direct Callback succeeded, connection established - %s"), DbgGetClientInfo()); 

	// remove the connecting timer and state
	//if (m_nConnectingState == CCS_NONE) // TODO LOGREMOVE
	//	DEBUG_ONLY( DebugLog(_T("ConnectionEstablished with CCS_NONE (incoming, thats fine)")) );
	m_nConnectingState = CCS_NONE;
	theApp.clientlist->RemoveConnectingClient(this);

	// check if we should use this client to retrieve our public IP
	if (theApp.GetPublicIP() == 0 && theApp.IsConnected() && m_fPeerCache)
		SendPublicIPRequest();

	switch(GetKadState())
	{
		case KS_CONNECTING_FWCHECK:
            SetKadState(KS_CONNECTED_FWCHECK);
			break;
		case KS_CONNECTING_BUDDY:
		case KS_INCOMING_BUDDY:
			DEBUG_ONLY( DebugLog(_T("Set KS_CONNECTED_BUDDY for client %s"), DbgGetClientInfo()) );
			SetKadState(KS_CONNECTED_BUDDY);
			break;
		case KS_CONNECTING_FWCHECK_UDP:
			SetKadState(KS_FWCHECK_UDP);
			DEBUG_ONLY( DebugLog(_T("Set KS_FWCHECK_UDP for client %s"), DbgGetClientInfo()) );
			SendFirewallCheckUDPRequest();
			break;
	}

	if (GetChatState() == MS_CONNECTING || GetChatState() == MS_CHATTING)
	{
		if (GetFriend() != NULL && GetFriend()->IsTryingToConnect()){
			GetFriend()->UpdateFriendConnectionState(FCR_ESTABLISHED); // for friends any connectionupdate is handled in the friend class
			if (credits != NULL && credits->GetCurrentIdentState(GetConnectIP()) == IS_IDFAILED)
				GetFriend()->UpdateFriendConnectionState(FCR_SECUREIDENTFAILED);
		}
		else
			theApp.emuledlg->chatwnd->chatselector.ConnectingResult(this, true); // other clients update directly
	}

	switch(GetDownloadState())
	{
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
		case DS_WAITCALLBACKKAD:
			m_bReaskPending = false;
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
			break;
	}

	if (m_bReaskPending)
	{
		m_bReaskPending = false;
		if (GetDownloadState() != DS_NONE && GetDownloadState() != DS_DOWNLOADING)
		{
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
		}
	}

	switch(GetUploadState())
	{
		case US_CONNECTING:
			if (theApp.uploadqueue->IsDownloading(this))
			{
				SetUploadState(US_UPLOADING);
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugSend("OP__AcceptUploadReq", this);
				Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
				theStats.AddUpDataOverheadFileRequest(packet->size);
				SendPacket(packet,true);
			}
	}

	if (m_iFileListRequested == 1)
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend(m_fSharedDirectories ? "OP__AskSharedDirs" : "OP__AskSharedFiles", this);
        Packet* packet = new Packet(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES,0);
		theStats.AddUpDataOverheadOther(packet->size);
		SendPacket(packet,true);
	}

	while (!m_WaitingPackets_list.IsEmpty())
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("Buffered Packet", this);
		SendPacket(m_WaitingPackets_list.RemoveHead(), true);
	}

}

void CUpDownClient::InitClientSoftwareVersion()
{
	if (m_pszUsername == NULL){
		m_clientSoft = SO_UNKNOWN;
		return;
	}

	int iHashType = GetHashType();
	if (m_bEmuleProtocol || iHashType == SO_EMULE){
		LPCTSTR pszSoftware;
		switch(m_byCompatibleClient){
			case SO_CDONKEY:
				m_clientSoft = SO_CDONKEY;
				pszSoftware = _T("cDonkey");
				break;
			case SO_XMULE:
				m_clientSoft = SO_XMULE;
				pszSoftware = _T("xMule");
				break;
			case SO_AMULE:
				m_clientSoft = SO_AMULE;
				pszSoftware = _T("aMule");
				break;
			case SO_SHAREAZA:
			case 40:
				m_clientSoft = SO_SHAREAZA;
				pszSoftware = _T("Shareaza");
				break;
			case SO_LPHANT:
				m_clientSoft = SO_LPHANT;
				pszSoftware = _T("lphant");
				break;
			default:
				if (m_bIsML || m_byCompatibleClient == SO_MLDONKEY){
					m_clientSoft = SO_MLDONKEY;
					pszSoftware = _T("MLdonkey");
				}
				else if (m_bIsHybrid){
					m_clientSoft = SO_EDONKEYHYBRID;
					pszSoftware = _T("eDonkeyHybrid");
				}
				else if (m_byCompatibleClient != 0){
					m_clientSoft = SO_XMULE; // means: 'eMule Compatible'
					pszSoftware = _T("eMule Compat");
				}
				else{
					m_clientSoft = SO_EMULE;
					pszSoftware = _T("eMule");
				}
		}

		int iLen;
		TCHAR szSoftware[128];
		if (m_byEmuleVersion == 0){
			m_nClientVersion = MAKE_CLIENT_VERSION(0, 0, 0);
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s"), pszSoftware);
		}
		else if (m_byEmuleVersion != 0x99){
			UINT nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
			m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v0.%u"), pszSoftware, nClientMinVersion);
		}
		else{
			UINT nClientMajVersion = (m_nClientVersion >> 17) & 0x7f;
			UINT nClientMinVersion = (m_nClientVersion >> 10) & 0x7f;
			UINT nClientUpVersion  = (m_nClientVersion >>  7) & 0x07;
			m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);
			if (m_clientSoft == SO_EMULE)
				iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.%u%c"), pszSoftware, nClientMajVersion, nClientMinVersion, _T('a') + nClientUpVersion);
			else if (m_clientSoft == SO_AMULE || nClientUpVersion != 0)
				iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.%u.%u"), pszSoftware, nClientMajVersion, nClientMinVersion, nClientUpVersion);
			else if (m_clientSoft == SO_LPHANT)
			{
				if (nClientMinVersion < 10)
				    iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.0%u"), pszSoftware, (nClientMajVersion-1), nClientMinVersion);
				else
					iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.%u"), pszSoftware, (nClientMajVersion-1), nClientMinVersion);
			}
			else
				iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.%u"), pszSoftware, nClientMajVersion, nClientMinVersion);
		}
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
		}
		return;
	}

	if (m_bIsHybrid){
		m_clientSoft = SO_EDONKEYHYBRID;
		// seen:
		// 105010	0.50.10
		// 10501	0.50.1
		// 10300	1.3.0
		// 10201	1.2.1
		// 10103	1.1.3
		// 10102	1.1.2
		// 10100	1.1
		// 1051		0.51.0
		// 1002		1.0.2
		// 1000		1.0
		// 501		0.50.1

		UINT nClientMajVersion;
		UINT nClientMinVersion;
		UINT nClientUpVersion;
		if (m_nClientVersion > 100000){
			UINT uMaj = m_nClientVersion/100000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*100000) / 100;
			nClientUpVersion = m_nClientVersion % 100;
		}
		else if (m_nClientVersion >= 10100 && m_nClientVersion <= 10309){
			UINT uMaj = m_nClientVersion/10000;
			nClientMajVersion = uMaj;
			nClientMinVersion = (m_nClientVersion - uMaj*10000) / 100;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion > 10000){
			UINT uMaj = m_nClientVersion/10000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*10000) / 10;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion >= 1000 && m_nClientVersion < 1020){
			UINT uMaj = m_nClientVersion/1000;
			nClientMajVersion = uMaj;
			nClientMinVersion = (m_nClientVersion - uMaj*1000) / 10;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion > 1000){
			UINT uMaj = m_nClientVersion/1000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = m_nClientVersion - uMaj*1000;
			nClientUpVersion = 0;
		}
		else if (m_nClientVersion > 100){
			UINT uMin = m_nClientVersion/10;
			nClientMajVersion = 0;
			nClientMinVersion = uMin;
			nClientUpVersion = m_nClientVersion - uMin*10;
		}
		else{
			nClientMajVersion = 0;
			nClientMinVersion = m_nClientVersion;
			nClientUpVersion = 0;
		}
		m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);

		int iLen;
		TCHAR szSoftware[128];
		if (nClientUpVersion)
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("eDonkeyHybrid v%u.%u.%u"), nClientMajVersion, nClientMinVersion, nClientUpVersion);
		else
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("eDonkeyHybrid v%u.%u"), nClientMajVersion, nClientMinVersion);
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
		}
		return;
	}

	if (m_bIsML || iHashType == SO_MLDONKEY){
		m_clientSoft = SO_MLDONKEY;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		TCHAR szSoftware[128];
		int iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("MLdonkey v0.%u"), nClientMinVersion);
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
		}
		return;
	}

	if (iHashType == SO_OLDEMULE){
		m_clientSoft = SO_OLDEMULE;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		TCHAR szSoftware[128];
		int iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("Old eMule v0.%u"), nClientMinVersion);
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
		}
		return;
	}

	m_clientSoft = SO_EDONKEY;
	UINT nClientMinVersion = m_nClientVersion;
	m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
	TCHAR szSoftware[128];
	int iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("eDonkey v0.%u"), nClientMinVersion);
	if (iLen > 0){
		memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
		m_strClientSoftware.ReleaseBuffer(iLen);
	}
}

int CUpDownClient::GetHashType() const
{
	if (m_achUserHash[5] == 13 && m_achUserHash[14] == 110)
		return SO_OLDEMULE;
	else if (m_achUserHash[5] == 14 && m_achUserHash[14] == 111)
		return SO_EMULE;
 	else if (m_achUserHash[5] == 'M' && m_achUserHash[14] == 'L')
		return SO_MLDONKEY;
	else
		return SO_UNKNOWN;
}

void CUpDownClient::SetUserName(LPCTSTR pszNewName)
{
	free(m_pszUsername);
	if (pszNewName)
		m_pszUsername = _tcsdup(pszNewName);
	else
		m_pszUsername = NULL;
}

void CUpDownClient::RequestSharedFileList()
{
	if (m_iFileListRequested == 0){
		AddLogLine(true, GetResString(IDS_SHAREDFILES_REQUEST), GetUserName());
    	m_iFileListRequested = 1;
		TryToConnect(true);
	}
	else{
		LogWarning(LOG_STATUSBAR, _T("Requesting shared files from user %s (%u) is already in progress"), GetUserName(), GetUserIDHybrid());
	}
}

void CUpDownClient::ProcessSharedFileList(const uchar* pachPacket, uint32 nSize, LPCTSTR pszDirectory)
{
	if (m_iFileListRequested > 0)
	{
        m_iFileListRequested--;
		theApp.searchlist->ProcessSearchAnswer(pachPacket,nSize,this,NULL,pszDirectory);
	}
}

void CUpDownClient::SetUserHash(const uchar* pucUserHash)
{
	if( pucUserHash == NULL ){
		md4clr(m_achUserHash);
		return;
	}
	md4cpy(m_achUserHash, pucUserHash);
}

void CUpDownClient::SetBuddyID(const uchar* pucBuddyID)
{
	if( pucBuddyID == NULL ){
		md4clr(m_achBuddyID);
		m_bBuddyIDValid = false;
		return;
	}
	m_bBuddyIDValid = true;
	md4cpy(m_achBuddyID, pucBuddyID);
}

void CUpDownClient::SendPublicKeyPacket()
{
	// send our public key to the client who requested it
	if (socket == NULL || credits == NULL || m_SecureIdentState != IS_KEYANDSIGNEEDED){
		ASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;

    Packet* packet = new Packet(OP_PUBLICKEY,theApp.clientcredits->GetPubKeyLen() + 1,OP_EMULEPROT);
	theStats.AddUpDataOverheadOther(packet->size);
	memcpy(packet->pBuffer+1,theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
	packet->pBuffer[0] = theApp.clientcredits->GetPubKeyLen();
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__PublicKey", this);
	SendPacket(packet, true);
	m_SecureIdentState = IS_SIGNATURENEEDED;
}

void CUpDownClient::SendSignaturePacket()
{
	// signate the public key of this client and send it
	if (socket == NULL || credits == NULL || m_SecureIdentState == 0){
		ASSERT ( false );
		return;
	}

	if (!theApp.clientcredits->CryptoAvailable())
		return;
	if (credits->GetSecIDKeyLen() == 0)
		return; // We don't have his public key yet, will be back here later
	// do we have a challenge value received (actually we should if we are in this function)
	if (credits->m_dwCryptRndChallengeFrom == 0){
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("Want to send signature but challenge value is invalid ('%s')"), GetUserName());
		return;
	}
	// v2
	// we will use v1 as default, except if only v2 is supported
	bool bUseV2;
	if ( (m_bySupportSecIdent&1) == 1 )
		bUseV2 = false;
	else
		bUseV2 = true;

	uint8 byChaIPKind = 0;
	uint32 ChallengeIP = 0;
	if (bUseV2){
		if (theApp.serverconnect->GetClientID() == 0 || theApp.serverconnect->IsLowID()){
			// we cannot do not know for sure our public ip, so use the remote clients one
			ChallengeIP = GetIP();
			byChaIPKind = CRYPT_CIP_REMOTECLIENT;
		}
		else{
			ChallengeIP = theApp.serverconnect->GetClientID();
			byChaIPKind  = CRYPT_CIP_LOCALCLIENT;
		}
	}
	//end v2
	uchar achBuffer[250];
	uint8 siglen = theApp.clientcredits->CreateSignature(credits, achBuffer,  250, ChallengeIP, byChaIPKind );
	if (siglen == 0){
		ASSERT ( false );
		return;
	}
	Packet* packet = new Packet(OP_SIGNATURE,siglen + 1+ ( (bUseV2)? 1:0 ),OP_EMULEPROT);
	theStats.AddUpDataOverheadOther(packet->size);
	memcpy(packet->pBuffer+1,achBuffer, siglen);
	packet->pBuffer[0] = siglen;
	if (bUseV2)
		packet->pBuffer[1+siglen] = byChaIPKind;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__Signature", this);
	SendPacket(packet, true);
	m_SecureIdentState = IS_ALLREQUESTSSEND;
}

void CUpDownClient::ProcessPublicKeyPacket(const uchar* pachPacket, uint32 nSize)
{
	theApp.clientlist->AddTrackClient(this);

	if (socket == NULL || credits == NULL || pachPacket[0] != nSize-1
		|| nSize < 10 || nSize > 250){
		ASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;
	// the function will handle everything (mulitple key etc)
	if (credits->SetSecureIdent(pachPacket+1, pachPacket[0])){
		// if this client wants a signature, now we can send him one
		if (m_SecureIdentState == IS_SIGNATURENEEDED){
			SendSignaturePacket();
		}
		else if(m_SecureIdentState == IS_KEYANDSIGNEEDED)
		{
			// something is wrong
			if (thePrefs.GetLogSecureIdent())
				AddDebugLogLine(false, _T("Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket"));
		}
	}
	else
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("Failed to use new received public key"));
	}
}

void CUpDownClient::ProcessSignaturePacket(const uchar* pachPacket, uint32 nSize)
{
	// here we spread the good guys from the bad ones ;)

	if (socket == NULL || credits == NULL || nSize > 250 || nSize < 10){
		ASSERT ( false );
		return;
	}

	uint8 byChaIPKind;
	if (pachPacket[0] == nSize-1)
		byChaIPKind = 0;
	else if (pachPacket[0] == nSize-2 && (m_bySupportSecIdent & 2) > 0) //v2
		byChaIPKind = pachPacket[nSize-1];
	else{
		ASSERT ( false );
		return;
	}

	if (!theApp.clientcredits->CryptoAvailable())
		return;
	
	// we accept only one signature per IP, to avoid floods which need a lot cpu time for cryptfunctions
	if (m_dwLastSignatureIP == GetIP())
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("received multiple signatures from one client"));
		return;
	}
	
	// also make sure this client has a public key
	if (credits->GetSecIDKeyLen() == 0)
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("received signature for client without public key"));
		return;
	}
	
	// and one more check: did we ask for a signature and sent a challange packet?
	if (credits->m_dwCryptRndChallengeFor == 0)
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("received signature for client with invalid challenge value ('%s')"), GetUserName());
		return;
	}

	if (theApp.clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], GetIP(), byChaIPKind ) ){
		// result is saved in function abouve
		//if (thePrefs.GetLogSecureIdent())
		//	AddDebugLogLine(false, _T("'%s' has passed the secure identification, V2 State: %i"), GetUserName(), byChaIPKind);

		// inform our friendobject if needed
		if (GetFriend() != NULL && GetFriend()->IsTryingToConnect())
			GetFriend()->UpdateFriendConnectionState(FCR_USERHASHVERIFIED);
	}
	else
	{
		if (GetFriend() != NULL && GetFriend()->IsTryingToConnect())
			GetFriend()->UpdateFriendConnectionState(FCR_SECUREIDENTFAILED);
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("'%s' has failed the secure identification, V2 State: %i"), GetUserName(), byChaIPKind);
	}
	m_dwLastSignatureIP = GetIP(); 
}

void CUpDownClient::SendSecIdentStatePacket()
{
	// check if we need public key and signature
	uint8 nValue = 0;
	if (credits){
		if (theApp.clientcredits->CryptoAvailable()){
			if (credits->GetSecIDKeyLen() == 0)
				nValue = IS_KEYANDSIGNEEDED;
			else if (m_dwLastSignatureIP != GetIP())
				nValue = IS_SIGNATURENEEDED;
		}
		if (nValue == 0){
			//if (thePrefs.GetLogSecureIdent())
			//	AddDebugLogLine(false, _T("Not sending SecIdentState Packet, because State is Zero"));
			return;
		}
		// crypt: send random data to sign
		uint32 dwRandom = rand()+1;
		credits->m_dwCryptRndChallengeFor = dwRandom;
		Packet* packet = new Packet(OP_SECIDENTSTATE,5,OP_EMULEPROT);
		theStats.AddUpDataOverheadOther(packet->size);
		packet->pBuffer[0] = nValue;
		PokeUInt32(packet->pBuffer+1, dwRandom);
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__SecIdentState", this);
		SendPacket(packet, true);
	}
	else
		ASSERT ( false );
}

void CUpDownClient::ProcessSecIdentStatePacket(const uchar* pachPacket, uint32 nSize)
{
	if (nSize != 5)
		return;
	if (!credits){
		ASSERT ( false );
		return;
	}
	switch(pachPacket[0]){
		case 0:
			m_SecureIdentState = IS_UNAVAILABLE;
			break;
		case 1:
			m_SecureIdentState = IS_SIGNATURENEEDED;
			break;
		case 2:
			m_SecureIdentState = IS_KEYANDSIGNEEDED;
			break;
	}
	credits->m_dwCryptRndChallengeFrom = PeekUInt32(pachPacket+1);
}

void CUpDownClient::InfoPacketsReceived()
{
	// indicates that both Information Packets has been received
	// needed for actions, which process data from both packets
	ASSERT ( m_byInfopacketsReceived == IP_BOTH );
	m_byInfopacketsReceived = IP_NONE;
	
	if (m_bySupportSecIdent){
		SendSecIdentStatePacket();
	}
}

void CUpDownClient::ResetFileStatusInfo()
{
	delete[] m_abyPartStatus;
	m_abyPartStatus = NULL;
	m_nRemoteQueueRank = 0;
	m_nPartCount = 0;
	m_strClientFilename.Empty();
	m_bCompleteSource = false;
	m_uFileRating = 0;
	m_strFileComment.Empty();
	delete m_pReqFileAICHHash;
	m_pReqFileAICHHash = NULL;
}

bool CUpDownClient::IsBanned() const
{
	return theApp.clientlist->IsBannedClient(GetIP());
}

void CUpDownClient::SendPreviewRequest(const CAbstractFile* pForFile)
{
	if (m_fPreviewReqPending == 0){
		m_fPreviewReqPending = 1;
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__RequestPreview", this, pForFile->GetFileHash());
		Packet* packet = new Packet(OP_REQUESTPREVIEW,16,OP_EMULEPROT);
		md4cpy(packet->pBuffer,pForFile->GetFileHash());
		theStats.AddUpDataOverheadOther(packet->size);
		SafeConnectAndSendPacket(packet);
	}
	else{
		LogWarning(LOG_STATUSBAR, GetResString(IDS_ERR_PREVIEWALREADY));
	}
}

void CUpDownClient::SendPreviewAnswer(const CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount)
{
	m_fPreviewAnsPending = 0;
	CSafeMemFile data(1024);
	if (pForFile){
		data.WriteHash16(pForFile->GetFileHash());
	}
	else{
		static const uchar _aucZeroHash[16] = {0};
		data.WriteHash16(_aucZeroHash);
	}
	data.WriteUInt8(nCount);
	for (int i = 0; i != nCount; i++){
		if (imgFrames == NULL){
			ASSERT ( false );
			return;
		}
		CxImage* cur_frame = imgFrames[i];
		if (cur_frame == NULL){
			ASSERT ( false );
			return;
		}
		BYTE* abyResultBuffer = NULL;
		long nResultSize = 0;
		if (!cur_frame->Encode(abyResultBuffer, nResultSize, CXIMAGE_FORMAT_PNG)){
			ASSERT ( false );			
			return;
		}
		data.WriteUInt32(nResultSize);
		data.Write(abyResultBuffer, nResultSize);
		free(abyResultBuffer);
	}
	Packet* packet = new Packet(&data, OP_EMULEPROT);
	packet->opcode = OP_PREVIEWANSWER;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__PreviewAnswer", this, (uchar*)packet->pBuffer);
	theStats.AddUpDataOverheadOther(packet->size);
	SafeConnectAndSendPacket(packet);
}

void CUpDownClient::ProcessPreviewReq(const uchar* pachPacket, uint32 nSize)
{
	if (nSize < 16)
		throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
	
	if (m_fPreviewAnsPending || thePrefs.CanSeeShares()==vsfaNobody || (thePrefs.CanSeeShares()==vsfaFriends && !IsFriend()))
		return;
	
	m_fPreviewAnsPending = 1;
	CKnownFile* previewFile = theApp.sharedfiles->GetFileByID(pachPacket);
	if (previewFile == NULL){
		SendPreviewAnswer(NULL, NULL, 0);
	}
	else{
		previewFile->GrabImage(4,0,true,450,this);
	}
}

void CUpDownClient::ProcessPreviewAnswer(const uchar* pachPacket, uint32 nSize)
{
	if (m_fPreviewReqPending == 0)
		return;
	m_fPreviewReqPending = 0;
	CSafeMemFile data(pachPacket, nSize);
	uchar Hash[16];
	data.ReadHash16(Hash);
	uint8 nCount = data.ReadUInt8();
	if (nCount == 0){
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_PREVIEWFAILED), GetUserName());
		return;
	}
	CSearchFile* sfile = theApp.searchlist->GetSearchFileByHash(Hash);
	if (sfile == NULL){
		//already deleted
		return;
	}

	BYTE* pBuffer = NULL;
	try{
		for (int i = 0; i != nCount; i++){
			uint32 nImgSize = data.ReadUInt32();
			if (nImgSize > nSize)
				throw CString(_T("CUpDownClient::ProcessPreviewAnswer - Provided image size exceeds limit"));
			pBuffer = new BYTE[nImgSize];
			data.Read(pBuffer, nImgSize);
			CxImage* image = new CxImage(pBuffer, nImgSize, CXIMAGE_FORMAT_PNG);
			delete[] pBuffer;
			pBuffer = NULL;
			if (image->IsValid())
				sfile->AddPreviewImg(image);
			else
				delete image;
		}
	}
	catch(...){
		delete[] pBuffer;
		throw;
	}
	(new PreviewDlg())->SetFile(sfile);
}

// sends a packet, if needed it will establish a connection before
// options used: ignore max connections, control packet, delete packet
// !if the functions returns false that client object was deleted because the connection try failed and the object wasn't needed anymore.
bool CUpDownClient::SafeConnectAndSendPacket(Packet* packet)
{
	if (socket != NULL && socket->IsConnected())
	{
		socket->SendPacket(packet, true, true);
		return true;
	}
	else
	{
		m_WaitingPackets_list.AddTail(packet);
		return TryToConnect(true);
	}
}

bool CUpDownClient::SendPacket(Packet* packet, bool bDeletePacket, bool bVerifyConnection)
{
	if (socket != NULL && (!bVerifyConnection || socket->IsConnected()))
	{
		socket->SendPacket(packet, bDeletePacket, true);
		return true;
	}
	else
	{
		DebugLogError(_T("Outgoing packet (0x%X) discarded because expected socket or connection does not exists %s"), packet->opcode, DbgGetClientInfo());
		if (bDeletePacket)
			delete packet;
		return false;
	}
}

#ifdef _DEBUG
void CUpDownClient::AssertValid() const
{
	CObject::AssertValid();

	CHECK_OBJ(socket);
	CHECK_PTR(credits);
	CHECK_PTR(m_Friend);
	CHECK_OBJ(reqfile);
	(void)m_abyUpPartStatus;
	m_OtherRequests_list.AssertValid();
	m_OtherNoNeeded_list.AssertValid();
	(void)m_lastPartAsked;
	(void)m_cMessagesReceived;
	(void)m_cMessagesSent;
	(void)m_dwUserIP;
	(void)m_dwServerIP;
	(void)m_nUserIDHybrid;
	(void)m_nUserPort;
	(void)m_nServerPort;
	(void)m_nClientVersion;
	(void)m_nUpDatarate;
	(void)m_byEmuleVersion;
	(void)m_byDataCompVer;
	CHECK_BOOL(m_bEmuleProtocol);
	CHECK_BOOL(m_bIsHybrid);
	(void)m_pszUsername;
	(void)m_achUserHash;
	(void)m_achBuddyID;
	(void)m_nBuddyIP;
	(void)m_nBuddyPort;
	(void)m_nUDPPort;
	(void)m_nKadPort;
	(void)m_byUDPVer;
	(void)m_bySourceExchange1Ver;
	(void)m_byAcceptCommentVer;
	(void)m_byExtendedRequestsVer;
	CHECK_BOOL(m_bFriendSlot);
	CHECK_BOOL(m_bCommentDirty);
	CHECK_BOOL(m_bIsML);
	//ASSERT( m_clientSoft >= SO_EMULE && m_clientSoft <= SO_SHAREAZA || m_clientSoft == SO_MLDONKEY || m_clientSoft >= SO_EDONKEYHYBRID && m_clientSoft <= SO_UNKNOWN );
	(void)m_strClientSoftware;
	(void)m_dwLastSourceRequest;
	(void)m_dwLastSourceAnswer;
	(void)m_dwLastAskedForSources;
    (void)m_iFileListRequested;
	(void)m_byCompatibleClient;
	m_WaitingPackets_list.AssertValid();
	m_DontSwap_list.AssertValid();
	(void)m_lastRefreshedDLDisplay;
	ASSERT( m_SecureIdentState >= IS_UNAVAILABLE && m_SecureIdentState <= IS_KEYANDSIGNEEDED );
	(void)m_dwLastSignatureIP;
	ASSERT( (m_byInfopacketsReceived & ~IP_BOTH) == 0 );
	(void)m_bySupportSecIdent;
	(void)m_nTransferredUp;
	ASSERT( m_nUploadState >= US_UPLOADING && m_nUploadState <= US_NONE );
	(void)m_dwUploadTime;
	(void)m_cAsked;
	(void)m_dwLastUpRequest;
	(void)m_nCurSessionUp;
    (void)m_nCurQueueSessionPayloadUp;
    (void)m_addedPayloadQueueSession;
	(void)m_nUpPartCount;
	(void)m_nUpCompleteSourcesCount;
	(void)s_UpStatusBar;
	(void)requpfileid;
    (void)m_lastRefreshedULDisplay;
	m_AvarageUDR_list.AssertValid();
	m_RequestedFiles_list.AssertValid();
	ASSERT( m_nDownloadState >= DS_DOWNLOADING && m_nDownloadState <= DS_NONE );
	(void)m_cDownAsked;
	(void)m_abyPartStatus;
	(void)m_strClientFilename;
	(void)m_nTransferredDown;
    (void)m_nCurSessionPayloadDown;
	(void)m_dwDownStartTime;
	(void)m_nLastBlockOffset;
	(void)m_nDownDatarate;
	(void)m_nDownDataRateMS;
	(void)m_nSumForAvgDownDataRate;
	(void)m_cShowDR;
	(void)m_nRemoteQueueRank;
	(void)m_dwLastBlockReceived;
	(void)m_nPartCount;
	ASSERT( m_nSourceFrom >= SF_SERVER && m_nSourceFrom <= SF_LINK );
	CHECK_BOOL(m_bRemoteQueueFull);
	CHECK_BOOL(m_bCompleteSource);
	CHECK_BOOL(m_bReaskPending);
	CHECK_BOOL(m_bUDPPending);
	CHECK_BOOL(m_bTransferredDownMini);
	CHECK_BOOL(m_bUnicodeSupport);
	ASSERT( m_nKadState >= KS_NONE && m_nKadState <= KS_CONNECTING_FWCHECK_UDP);
	m_AvarageDDR_list.AssertValid();
	(void)m_nSumForAvgUpDataRate;
	m_PendingBlocks_list.AssertValid();
	(void)s_StatusBar;
	ASSERT( m_nChatstate >= MS_NONE && m_nChatstate <= MS_UNABLETOCONNECT );
	(void)m_strFileComment;
	(void)m_uFileRating;
	CHECK_BOOL(m_bCollectionUploadSlot);
#undef CHECK_PTR
#undef CHECK_BOOL
}
#endif

#ifdef _DEBUG
void CUpDownClient::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
}
#endif

LPCTSTR CUpDownClient::DbgGetDownloadState() const
{
	const static LPCTSTR apszState[] =
	{
		_T("Downloading"),
		_T("OnQueue"),
		_T("Connected"),
		_T("Connecting"),
		_T("WaitCallback"),
		_T("WaitCallbackKad"),
		_T("ReqHashSet"),
		_T("NoNeededParts"),
		_T("TooManyConns"),
		_T("TooManyConnsKad"),
		_T("LowToLowIp"),
		_T("Banned"),
		_T("Error"),
		_T("None"),
		_T("RemoteQueueFull")
	};
	if (GetDownloadState() >= ARRSIZE(apszState))
		return _T("*Unknown*");
	return apszState[GetDownloadState()];
}

LPCTSTR CUpDownClient::DbgGetUploadState() const
{
	const static LPCTSTR apszState[] =
	{
		_T("Uploading"),
		_T("OnUploadQueue"),
		_T("Connecting"),
		_T("Banned"),
		_T("None")
	};
	if (GetUploadState() >= ARRSIZE(apszState))
		return _T("*Unknown*");
	return apszState[GetUploadState()];
}

LPCTSTR CUpDownClient::DbgGetKadState() const
{
	const static LPCTSTR apszState[] =
	{
		_T("None"),
		_T("FwCheckQueued"),
		_T("FwCheckConnecting"),
		_T("FwCheckConnected"),
		_T("BuddyQueued"),
		_T("BuddyIncoming"),
		_T("BuddyConnecting"),
		_T("BuddyConnected"),
		_T("QueuedFWCheckUDP"),
		_T("FWCheckUDP"),
		_T("FwCheckConnectingUDP")
	};
	if (GetKadState() >= ARRSIZE(apszState))
		return _T("*Unknown*");
	return apszState[GetKadState()];
}

CString CUpDownClient::DbgGetFullClientSoftVer() const
{
	if (GetClientModVer().IsEmpty())
		return GetClientSoftVer();
	return GetClientSoftVer() + _T(" [") + GetClientModVer() + _T(']');
}

CString CUpDownClient::DbgGetClientInfo(bool bFormatIP) const
{
	CString str;
	if (this != NULL)
	{
		try{
			if (HasLowID())
			{
				if (GetConnectIP())
				{
					str.Format(_T("%u@%s (%s) '%s' (%s,%s/%s/%s)"),
						GetUserIDHybrid(), ipstr(GetServerIP()),
						ipstr(GetConnectIP()),
						GetUserName(),
						DbgGetFullClientSoftVer(),
						DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
				}
				else
				{
					str.Format(_T("%u@%s '%s' (%s,%s/%s/%s)"),
						GetUserIDHybrid(), ipstr(GetServerIP()),
						GetUserName(),
						DbgGetFullClientSoftVer(),
						DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
				}
			}
			else
			{
				str.Format(bFormatIP ? _T("%-15s '%s' (%s,%s/%s/%s)") : _T("%s '%s' (%s,%s/%s/%s)"),
					ipstr(GetConnectIP()),
					GetUserName(),
					DbgGetFullClientSoftVer(),
					DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
			}
		}
		catch(...){
			str.Format(_T("%p - Invalid client instance"), this);
		}
	}
	return str;
}

bool CUpDownClient::CheckHandshakeFinished() const
{
	if (m_bHelloAnswerPending)
	{
		// 24-Nov-2004 [bc]: The reason for this is that 2 clients are connecting to each other at the same..
		//if (thePrefs.GetVerbose())
		//	AddDebugLogLine(DLP_VERYLOW, false, _T("Handshake not finished - while processing packet: %s; %s"), DbgGetClientTCPOpcode(protocol, opcode), DbgGetClientInfo());
		return false;
	}

	return true;
}

void CUpDownClient::CheckForGPLEvilDoer()
{
	if (!m_strModVersion.IsEmpty()){
		LPCTSTR pszModVersion = (LPCTSTR)m_strModVersion;

		// skip leading spaces
		while (*pszModVersion == _T(' '))
			pszModVersion++;

		// check for known major gpl breaker
		if (_tcsnicmp(pszModVersion, _T("LH"), 2)==0 || _tcsnicmp(pszModVersion, _T("LIO"), 3)==0 || _tcsnicmp(pszModVersion, _T("PLUS PLUS"), 9)==0)
			m_bGPLEvildoer = true;
	}
}

void CUpDownClient::OnSocketConnected(int /*nErrorCode*/)
{
}

CString CUpDownClient::GetDownloadStateDisplayString() const
{
	CString strState;
	switch (GetDownloadState())
	{
		case DS_CONNECTING:
			strState = GetResString(IDS_CONNECTING);
			break;
		case DS_CONNECTED:
			strState = GetResString(IDS_ASKING);
			break;
		case DS_WAITCALLBACK:
			strState = GetResString(IDS_CONNVIASERVER);
			break;
		case DS_ONQUEUE:
			if (IsRemoteQueueFull())
				strState = GetResString(IDS_QUEUEFULL);
			else
				strState = GetResString(IDS_ONQUEUE);
			break;
		case DS_DOWNLOADING:
			strState = GetResString(IDS_TRANSFERRING);
			break;
		case DS_REQHASHSET:
			strState = GetResString(IDS_RECHASHSET);
			break;
		case DS_NONEEDEDPARTS:
			strState = GetResString(IDS_NONEEDEDPARTS);
			break;
		case DS_LOWTOLOWIP:
			strState = GetResString(IDS_NOCONNECTLOW2LOW);
			break;
		case DS_TOOMANYCONNS:
			strState = GetResString(IDS_TOOMANYCONNS);
			break;
		case DS_ERROR:
			strState = GetResString(IDS_ERROR);
			break;
		case DS_WAITCALLBACKKAD:
			strState = GetResString(IDS_KAD_WAITCBK);
			break;
		case DS_TOOMANYCONNSKAD:
			strState = GetResString(IDS_KAD_TOOMANDYKADLKPS);
			break;
	}

	if (thePrefs.GetPeerCacheShow())
	{
		switch (m_ePeerCacheDownState)
		{
		case PCDS_WAIT_CLIENT_REPLY:
			strState += _T(" ")+GetResString(IDS_PCDS_CLIENTWAIT);
			break;
		case PCDS_WAIT_CACHE_REPLY:
			strState += _T(" ")+GetResString(IDS_PCDS_CACHEWAIT);
			break;
		case PCDS_DOWNLOADING:
			strState += _T(" ")+GetResString(IDS_CACHE);
			break;
		}
		if (m_ePeerCacheDownState != PCDS_NONE && m_bPeerCacheDownHit)
			strState += _T(" Hit");
	}

	return strState;
}

CString CUpDownClient::GetUploadStateDisplayString() const
{
	CString strState;
	switch (GetUploadState()){
		case US_ONUPLOADQUEUE:
			strState = GetResString(IDS_ONQUEUE);
			break;
		case US_BANNED:
			strState = GetResString(IDS_BANNED);
			break;
		case US_CONNECTING:
			strState = GetResString(IDS_CONNECTING);
			break;
		case US_UPLOADING:
			// GetNumberOfRequestedBlocksInQueue is no longer available and retrieving it would cause quite some extra load
			// (either due to thread syncing or due to adding redunant extra varts just for this function), so given that
			// "stalled, waiting for disk" should happen like never, it is removed for now
            if(GetPayloadInBuffer() == 0 && /*GetNumberOfRequestedBlocksInQueue() == 0 &&*/ thePrefs.IsExtControlsEnabled()) {
				strState = GetResString(IDS_US_STALLEDW4BR);
            /*} else if(GetPayloadInBuffer() == 0 && thePrefs.IsExtControlsEnabled()) {
				strState = GetResString(IDS_US_STALLEDREADINGFDISK);*/
            } else if(GetSlotNumber() <= theApp.uploadqueue->GetActiveUploadsCount()) {
				strState = GetResString(IDS_TRANSFERRING);
            } else {
                strState = GetResString(IDS_TRICKLING);
            }
			break;
	}

	if (thePrefs.GetPeerCacheShow())
	{
		switch (m_ePeerCacheUpState)
		{
		case PCUS_WAIT_CACHE_REPLY:
			strState += _T(" CacheWait");
			break;
		case PCUS_UPLOADING:
			strState += _T(" Cache");
			break;
		}
		if (m_ePeerCacheUpState != PCUS_NONE && m_bPeerCacheUpHit)
			strState += _T(" Hit");
	}

	return strState;
}

void CUpDownClient::SendPublicIPRequest(){
	if (socket && socket->IsConnected()){
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__PublicIPReq", this);
		Packet* packet = new Packet(OP_PUBLICIP_REQ,0,OP_EMULEPROT);
		theStats.AddUpDataOverheadOther(packet->size);
		SendPacket(packet, true);
		m_fNeedOurPublicIP = 1;
	}
}

void CUpDownClient::ProcessPublicIPAnswer(const BYTE* pbyData, UINT uSize){
	if (uSize != 4)
		throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
	uint32 dwIP = PeekUInt32(pbyData);
	if (m_fNeedOurPublicIP == 1){ // did we?
		m_fNeedOurPublicIP = 0;
		if (theApp.GetPublicIP() == 0 && !::IsLowID(dwIP) )
			theApp.SetPublicIP(dwIP);
	}	
}

void CUpDownClient::CheckFailedFileIdReqs(const uchar* aucFileHash)
{
	if ( aucFileHash != NULL && (theApp.sharedfiles->IsUnsharedFile(aucFileHash) || theApp.downloadqueue->GetFileByID(aucFileHash)) )
		return;
	//if (GetDownloadState() != DS_DOWNLOADING) // filereq floods are never allowed!
	{
		if (m_fFailedFileIdReqs < 6)// NOTE: Do not increase this nr. without increasing the bits for 'm_fFailedFileIdReqs'
			m_fFailedFileIdReqs++;
		if (m_fFailedFileIdReqs == 6)
		{
			if (theApp.clientlist->GetBadRequests(this) < 2)
				theApp.clientlist->TrackBadRequest(this, 1);
			if (theApp.clientlist->GetBadRequests(this) == 2){
				theApp.clientlist->TrackBadRequest(this, -2); // reset so the client will not be rebanned right after the ban is lifted
				Ban(_T("FileReq flood"));
			}
			throw CString(thePrefs.GetLogBannedClients() ? _T("FileReq flood") : _T(""));
		}
	}
}

EUtf8Str CUpDownClient::GetUnicodeSupport() const
{
	if (m_bUnicodeSupport)
		return utf8strRaw;
	return utf8strNone;
}

void CUpDownClient::SetSpammer(bool bVal){ 
	if (bVal)
		Ban(_T("Identified as Spammer"));
	else if (IsBanned() && m_fIsSpammer)
		UnBan();
	m_fIsSpammer = bVal ? 1 : 0;
}

void  CUpDownClient::SetMessageFiltered(bool bVal)	{
	m_fMessageFiltered = bVal ? 1 : 0;
}

bool  CUpDownClient::IsObfuscatedConnectionEstablished() const {
	if (socket != NULL && socket->IsConnected())
		return socket->IsObfusicating();
	else
		return false;
}

bool CUpDownClient::ShouldReceiveCryptUDPPackets() const {
	return (thePrefs.IsClientCryptLayerSupported() && SupportsCryptLayer() && theApp.GetPublicIP() != 0
		&& HasValidHash() && (thePrefs.IsClientCryptLayerRequested() || RequestsCryptLayer()) );
}

void CUpDownClient::ProcessChatMessage(CSafeMemFile* data, uint32 nLength)
{
	//filter me?
	if ( (thePrefs.MsgOnlyFriends() && !IsFriend()) || (thePrefs.MsgOnlySecure() && GetUserName()==NULL) )
	{
		if (!GetMessageFiltered()){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false,_T("Filtered Message from '%s' (IP:%s)"), GetUserName(), ipstr(GetConnectIP()));
		}
		SetMessageFiltered(true);
		return;
	}

	CString strMessage(data->ReadString(GetUnicodeSupport()!=utf8strNone, nLength));
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		Debug(_T("  %s\n"), strMessage);
	
	// default filtering
	CString strMessageCheck(strMessage);
	strMessageCheck.MakeLower();
	CString resToken;
	int curPos = 0;
	resToken = thePrefs.GetMessageFilter().Tokenize(_T("|"), curPos);
	while (!resToken.IsEmpty())
	{
		resToken.Trim();
		if (strMessageCheck.Find(resToken.MakeLower()) > -1){
			if ( thePrefs.IsAdvSpamfilterEnabled() && !IsFriend() && GetMessagesSent() == 0 ){
				SetSpammer(true);
				theApp.emuledlg->chatwnd->chatselector.EndSession(this);
			}
			return;
		}
		resToken = thePrefs.GetMessageFilter().Tokenize(_T("|"), curPos);
	}

	// advanced spamfilter check
	if (thePrefs.IsChatCaptchaEnabled() && !IsFriend()) {
		// captcha checks outrank any further checks - if the captcha has been solved, we assume its no spam
		// first check if we need to sent a captcha request to this client
		if (GetMessagesSent() == 0 && GetMessagesReceived() == 0 && GetChatCaptchaState() != CA_CAPTCHASOLVED)
		{
			// we have never sent a message to this client, and no message from him has ever passed our filters
			if (GetChatCaptchaState() != CA_CHALLENGESENT)
			{
				// we also aren't currently expecting a cpatcha response
				if (m_fSupportsCaptcha != NULL)
				{
					// and he supports captcha, so send him on and store the message (without showing for now)
					if (m_cCaptchasSent < 3) // no more than 3 tries
					{
						m_strCaptchaPendingMsg = strMessage;
						CSafeMemFile fileAnswer(1024);
						fileAnswer.WriteUInt8(0); // no tags, for future use
						CCaptchaGenerator captcha(4);
						if (captcha.WriteCaptchaImage(fileAnswer)){
							m_strCaptchaChallenge = captcha.GetCaptchaText();
							m_nChatCaptchaState = CA_CHALLENGESENT;
							m_cCaptchasSent++;
							Packet* packet = new Packet(&fileAnswer, OP_EMULEPROT, OP_CHATCAPTCHAREQ);
							theStats.AddUpDataOverheadOther(packet->size);
							if (!SafeConnectAndSendPacket(packet))
								return; // deleted client while connecting
						}
						else{
							ASSERT( false );
							DebugLogError(_T("Failed to create Captcha for client %s"), DbgGetClientInfo());
						}
					}
				}
				else
				{
					// client doesn't supports captchas, but we require them, tell him that its not going to work out
					// with an answer message (will not be shown and doesn't counts as sent message)
					if (m_cCaptchasSent < 1) // dont sent this notifier more than once
					{
						m_cCaptchasSent++;
						// always sent in english
						CString rstrMessage = _T("In order to avoid spam messages, this user requires you to solve a captcha before you can send a message to him. However your client does not support captchas, so you will not be able to chat with this user.");
						DebugLog(_T("Received message from client not supporting captchs, filtered and sent notifier (%s)"), DbgGetClientInfo());
						SendChatMessage(rstrMessage); // could delete client
					}
					else
						DebugLog(_T("Received message from client not supporting captchs, filtered, didn't sent notifier (%s)"), DbgGetClientInfo());
				}
				return;
			}
			else //(GetChatCaptchaState() == CA_CHALLENGESENT)
			{
				// this message must be the answer to the captcha request we send him, lets verify
				ASSERT( !m_strCaptchaChallenge.IsEmpty() );
				if (m_strCaptchaChallenge.CompareNoCase(strMessage.Trim().Right(min(strMessage.GetLength(), m_strCaptchaChallenge.GetLength()))) == 0){
					// allright
					DebugLog(_T("Captcha solved, showing withheld message (%s)"), DbgGetClientInfo());
					m_nChatCaptchaState = CA_CAPTCHASOLVED; // this state isn't persitent, but the messagecounter will be used to determine later if the captcha has been solved
					// replace captchaanswer with withheld message and show it
					strMessage = m_strCaptchaPendingMsg;
					m_cCaptchasSent = 0;
					m_strCaptchaChallenge = _T("");
					Packet* packet = new Packet(OP_CHATCAPTCHARES, 1, OP_EMULEPROT, false);
					packet->pBuffer[0] = 0; // status response
					theStats.AddUpDataOverheadOther(packet->size);
					if (!SafeConnectAndSendPacket(packet)) {
						ASSERT( false ); // deleted client while connecting
						return;
					}
				}
				else{ // wrong, cleanup and ignore
					DebugLogWarning(_T("Captcha answer failed (%s)"), DbgGetClientInfo());
					m_nChatCaptchaState = CA_NONE;
					m_strCaptchaChallenge = _T("");
					m_strCaptchaPendingMsg = _T("");
					Packet* packet = new Packet(OP_CHATCAPTCHARES, 1, OP_EMULEPROT, false);
					packet->pBuffer[0] = (m_cCaptchasSent < 3)? 1 : 2; // status response
					theStats.AddUpDataOverheadOther(packet->size);
					SafeConnectAndSendPacket(packet);
					return; // nothing more todo
				}
			}	
		}
		else
			DEBUG_ONLY( DebugLog(_T("Message passed CaptchaFilter - already solved or not needed (%s)"), DbgGetClientInfo()) );

	}
	if (thePrefs.IsAdvSpamfilterEnabled() && !IsFriend()) // friends are never spammer... (but what if two spammers are friends :P )
	{	
		bool bIsSpam = false;
		if (IsSpammer())
			bIsSpam = true;
		else
		{

			// first fixed criteria: If a client  sends me an URL in his first message before I response to him
			// there is a 99,9% chance that it is some poor guy advising his leech mod, or selling you .. well you know :P
			if (GetMessagesSent() == 0){
				int curPos=0;
				CString resToken = CString(URLINDICATOR).Tokenize(_T("|"), curPos);
				while (resToken != _T("")){
					if (strMessage.Find(resToken) > (-1) )
						bIsSpam = true;
					resToken= CString(URLINDICATOR).Tokenize(_T("|"),curPos);
				}
				// second fixed criteria: he sent me 4  or more messages and I didn't answered him once
				if (GetMessagesReceived() > 3)
					bIsSpam = true;
			}
		}
		if (bIsSpam)
		{
			if (IsSpammer()){
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("'%s' has been marked as spammer"), GetUserName());
			}
			SetSpammer(true);
			theApp.emuledlg->chatwnd->chatselector.EndSession(this);
			return;

		}
	}

	theApp.emuledlg->chatwnd->chatselector.ProcessMessage(this, strMessage);
}

void CUpDownClient::ProcessCaptchaRequest(CSafeMemFile* data){
	// received a captcha request, check if we actually accept it (only after sending a message ourself to this client)
	if (GetChatCaptchaState() == CA_ACCEPTING && GetChatState() != MS_NONE
		&& theApp.emuledlg->chatwnd->chatselector.GetItemByClient(this) != NULL)
	{
		// read tags (for future use)
		uint8 nTagCount = data->ReadUInt8();
		for (uint32 i = 0; i < nTagCount; i++)
			CTag tag(data, true);
		// sanitize checks - we want a small captcha not a wallpaper
		uint32 nSize = (uint32)(data->GetLength() - data->GetPosition());
		if ( nSize > 128 && nSize < 4096)
		{
			ULONGLONG pos = data->GetPosition();
			BYTE* byBuffer = data->Detach();
			CxImage imgCaptcha(&byBuffer[pos], nSize, CXIMAGE_FORMAT_BMP);
			//free(byBuffer);
			if (imgCaptcha.IsValid() && imgCaptcha.GetHeight() > 10 && imgCaptcha.GetHeight() < 50
				&& imgCaptcha.GetWidth() > 10 && imgCaptcha.GetWidth() < 150 )
			{
				HBITMAP hbmp = imgCaptcha.MakeBitmap();
				if (hbmp != NULL){
					m_nChatCaptchaState = CA_CAPTCHARECV;
					theApp.emuledlg->chatwnd->chatselector.ShowCaptchaRequest(this, hbmp);
					DeleteObject(hbmp);
				}
				else
					DebugLogWarning(_T("Received captcha request from client, Creating bitmap failed (%s)"), DbgGetClientInfo());
			}
			else
				DebugLogWarning(_T("Received captcha request from client, processing image failed or invalid pixel size (%s)"), DbgGetClientInfo());
		}
		else
			DebugLogWarning(_T("Received captcha request from client, size sanitize check failed (%u) (%s)"), nSize, DbgGetClientInfo());
	}
	else
		DebugLogWarning(_T("Received captcha request from client, but don't accepting it at this time (%s)"), DbgGetClientInfo());
}

void CUpDownClient::ProcessCaptchaReqRes(uint8 nStatus)
{
	if (GetChatCaptchaState() == CA_SOLUTIONSENT && GetChatState() != MS_NONE
		&& theApp.emuledlg->chatwnd->chatselector.GetItemByClient(this) != NULL)
	{
		ASSERT( nStatus < 3 );
		m_nChatCaptchaState = CA_NONE;
		theApp.emuledlg->chatwnd->chatselector.ShowCaptchaResult(this, GetResString((nStatus == 0) ? IDS_CAPTCHASOLVED : IDS_CAPTCHAFAILED));
	}
	else {
		m_nChatCaptchaState = CA_NONE;
		DebugLogWarning(_T("Received captcha result from client, but don't accepting it at this time (%s)"), DbgGetClientInfo());
	}
}

CFriend* CUpDownClient::GetFriend() const
{
	if (m_Friend != NULL && theApp.friendlist->IsValid(m_Friend))
		return m_Friend;
	else if (m_Friend != NULL)
		ASSERT( FALSE );
	return NULL;
}

void CUpDownClient::SendChatMessage(CString strMessage)
{
	CSafeMemFile data;
	data.WriteString(strMessage, GetUnicodeSupport());
	Packet* packet = new Packet(&data, OP_EDONKEYPROT, OP_MESSAGE);
	theStats.AddUpDataOverheadOther(packet->size);
	SafeConnectAndSendPacket(packet);
}

bool CUpDownClient::HasPassedSecureIdent(bool bPassIfUnavailable) const
{
	if (credits != NULL)
	{
		if (credits->GetCurrentIdentState(GetConnectIP()) == IS_IDENTIFIED 
			|| (credits->GetCurrentIdentState(GetConnectIP()) == IS_NOTAVAILABLE && bPassIfUnavailable))
		{
			return true;
		}
	}
	return false;
}

void CUpDownClient::SendFirewallCheckUDPRequest()
{
	ASSERT( GetKadState() == KS_FWCHECK_UDP );
	if (!Kademlia::CKademlia::IsRunning()){
		SetKadState(KS_NONE);
		return;
	}
	else if (GetUploadState() != US_NONE || GetDownloadState() != DS_NONE || GetChatState() != MS_NONE
		|| GetKadVersion() <= KADEMLIA_VERSION5_48a || GetKadPort() == 0)
	{
		Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, true, ntohl(GetIP()), 0); // inform the tester that this test was cancelled
		SetKadState(KS_NONE);
		return;
	}
	CSafeMemFile data;
	data.WriteUInt16(Kademlia::CKademlia::GetPrefs()->GetInternKadPort());
	data.WriteUInt16(Kademlia::CKademlia::GetPrefs()->GetExternalKadPort());
	data.WriteUInt32(Kademlia::CKademlia::GetPrefs()->GetUDPVerifyKey(GetConnectIP()));
	Packet* packet = new Packet(&data, OP_EMULEPROT, OP_FWCHECKUDPREQ);
	theStats.AddUpDataOverheadKad(packet->size);
	SafeConnectAndSendPacket(packet);
}

void CUpDownClient::ProcessFirewallCheckUDPRequest(CSafeMemFile* data){
	if (!Kademlia::CKademlia::IsRunning() || Kademlia::CKademlia::GetUDPListener() == NULL){
		DebugLogWarning(_T("Ignored Kad Firewallrequest UDP because Kad is not running (%s)"), DbgGetClientInfo());
		return;
	}
	// first search if we know this IP already, if so the result might be biased and we need tell the requester 
	bool bErrorAlreadyKnown = false;
	if (GetUploadState() != US_NONE || GetDownloadState() != DS_NONE || GetChatState() != MS_NONE)
		bErrorAlreadyKnown = true;
	else if (Kademlia::CKademlia::GetRoutingZone()->GetContact(ntohl(GetConnectIP()), 0, false) != NULL)
		bErrorAlreadyKnown = true;

	uint16 nRemoteInternPort = data->ReadUInt16();
	uint16 nRemoteExternPort = data->ReadUInt16();
	uint32 dwSenderKey = data->ReadUInt32();
	if (nRemoteInternPort == 0){
		DebugLogError(_T("UDP Firewallcheck requested with Intern Port == 0 (%s)"), DbgGetClientInfo());
		return;
	}
	if (dwSenderKey == 0)
		DebugLogWarning(_T("UDP Firewallcheck requested with SenderKey == 0 (%s)"), DbgGetClientInfo());
	
	CSafeMemFile fileTestPacket1;
	fileTestPacket1.WriteUInt8(bErrorAlreadyKnown ? 1 : 0);
	fileTestPacket1.WriteUInt16(nRemoteInternPort);
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KADEMLIA2_FIREWALLUDP", ntohl(GetConnectIP()), nRemoteInternPort);
	Kademlia::CKademlia::GetUDPListener()->SendPacket(&fileTestPacket1, KADEMLIA2_FIREWALLUDP, ntohl(GetConnectIP())
		, nRemoteInternPort, Kademlia::CKadUDPKey(dwSenderKey, theApp.GetPublicIP(false)), NULL);
	
	// if the client has a router with PAT (and therefore a different extern port than intern), test this port too
	if (nRemoteExternPort != 0 && nRemoteExternPort != nRemoteInternPort){
		CSafeMemFile fileTestPacket2;
		fileTestPacket2.WriteUInt8(bErrorAlreadyKnown ? 1 : 0);
		fileTestPacket2.WriteUInt16(nRemoteExternPort);
		if (thePrefs.GetDebugClientKadUDPLevel() > 0)
			DebugSend("KADEMLIA2_FIREWALLUDP", ntohl(GetConnectIP()), nRemoteExternPort);
		Kademlia::CKademlia::GetUDPListener()->SendPacket(&fileTestPacket2, KADEMLIA2_FIREWALLUDP, ntohl(GetConnectIP())
			, nRemoteExternPort, Kademlia::CKadUDPKey(dwSenderKey, theApp.GetPublicIP(false)), NULL);
	}
	DebugLog(_T("Answered UDP Firewallcheck request (%s)"), DbgGetClientInfo());
}

void CUpDownClient::SetConnectOptions(uint8 byOptions, bool bEncryption, bool bCallback)
{
	SetCryptLayerSupport((byOptions & 0x01) != 0 && bEncryption);
	SetCryptLayerRequest((byOptions & 0x02) != 0 && bEncryption);
	SetCryptLayerRequires((byOptions & 0x04) != 0 && bEncryption);
	SetDirectUDPCallbackSupport((byOptions & 0x08) != 0 && bCallback);
}

void CUpDownClient::SendSharedDirectories()
{
	//TODO: Don't send shared directories which do not contain any files
	// add shared directories
	CString strDir;
	CStringArray arFolders;
	POSITION pos = thePrefs.shareddir_list.GetHeadPosition();
	while (pos)
	{
		strDir = theApp.sharedfiles->GetPseudoDirName(thePrefs.shareddir_list.GetNext(pos));
		if (!strDir.IsEmpty())
			arFolders.Add(strDir);
	}

	// add incoming folders
	for (int iCat = 0; iCat < thePrefs.GetCatCount(); iCat++)
	{
		strDir = theApp.sharedfiles->GetPseudoDirName(thePrefs.GetCategory(iCat)->strIncomingPath);
		if (!strDir.IsEmpty())
			arFolders.Add(strDir);
	}

	// add temporary folder if there are any temp files
	if (theApp.downloadqueue->GetFileCount() > 0)
		arFolders.Add(CString(OP_INCOMPLETE_SHARED_FILES));
	// add "Other" folder (for single shared files) if there are any single shared files
	if (theApp.sharedfiles->ProbablyHaveSingleSharedFiles())
		arFolders.Add(CString(OP_OTHER_SHARED_FILES));

	// build packet
	CSafeMemFile tempfile(80);
	tempfile.WriteUInt32(arFolders.GetCount());
	for (int i = 0; i < arFolders.GetCount(); i++)
		tempfile.WriteString(arFolders.GetAt(i), GetUnicodeSupport());

	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__AskSharedDirsAnswer", this);
	Packet* replypacket = new Packet(&tempfile);
	replypacket->opcode = OP_ASKSHAREDDIRSANS;
	theStats.AddUpDataOverheadOther(replypacket->size);
	VERIFY( SendPacket(replypacket, true, true) );
}
