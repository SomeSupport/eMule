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
#include <io.h>
#include <share.h>
#include "emule.h"
#include "ServerList.h"
#include "SafeFile.h"
#include "Exceptions.h"
#include "OtherFunctions.h"
#include "IPFilter.h"
#include "LastCommonRouteFinder.h"
#include "Statistics.h"
#include "DownloadQueue.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "Server.h"
#include "Sockets.h"
#include "Packets.h"
#include "emuledlg.h"
#include "HttpDownloadDlg.h"
#include "ServerWnd.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	SERVER_MET_FILENAME	_T("server.met")

CServerList::CServerList()
{
	servercount = 0;
	version = 0;
	serverpos = 0;
	searchserverpos = 0;
	statserverpos = 0;
	delservercount = 0;
	m_nLastSaved = ::GetTickCount();
}

CServerList::~CServerList()
{
	SaveServermetToFile();
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; )
		delete list.GetNext(pos);
}

void CServerList::AutoUpdate()
{
	if (thePrefs.addresses_list.IsEmpty()){
		AfxMessageBox(GetResString(IDS_ERR_EMPTYADRESSESDAT), MB_ICONASTERISK);
		return;
	}

	bool bDownloaded = false;
	CString servermetdownload;
	CString servermetbackup;
	CString servermet;
	CString strURLToDownload; 
	servermetdownload.Format(_T("%sserver_met.download"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	servermetbackup.Format(_T("%sserver_met.old"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	servermet.Format(_T("%s") SERVER_MET_FILENAME, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	(void)_tremove(servermetbackup);
	(void)_tremove(servermetdownload);
	(void)_trename(servermet, servermetbackup);
	
	POSITION Pos = thePrefs.addresses_list.GetHeadPosition(); 
	while (!bDownloaded && Pos != NULL)
	{
		CHttpDownloadDlg dlgDownload;
		dlgDownload.m_strTitle = GetResString(IDS_HTTP_CAPTION);
		strURLToDownload = thePrefs.addresses_list.GetNext(Pos); 
		dlgDownload.m_sURLToDownload = strURLToDownload;
		dlgDownload.m_sFileToDownloadInto = servermetdownload;
		if (dlgDownload.DoModal() == IDOK)
			bDownloaded = true;
		else
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FAILEDDOWNLOADMET), strURLToDownload);
	}

	if (bDownloaded) {
		(void)_trename(servermet, servermetdownload);
		(void)_trename(servermetbackup, servermet);
	}
	else {
		(void)_tremove(servermet);
		(void)_trename(servermetbackup, servermet);
	}
}

bool CServerList::Init()
{
	// auto update the list by using an url
	if (thePrefs.GetAutoUpdateServerList())
		AutoUpdate();
	
	// Load Metfile
	CString strPath;
	strPath.Format(_T("%s") SERVER_MET_FILENAME, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	bool bRes = AddServerMetToList(strPath, false);
	if (thePrefs.GetAutoUpdateServerList())
	{
		strPath.Format(_T("%sserver_met.download"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
		bool bRes2 = AddServerMetToList(strPath, true);
		if (!bRes && bRes2)
			bRes = true;
	}

	// insert static servers from textfile
	strPath.Format(_T("%sstaticservers.dat"), thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	AddServersFromTextFile(strPath);

    theApp.serverlist->GiveServersForTraceRoute();
    
    return bRes;
}

bool CServerList::AddServerMetToList(const CString& strFile, bool bMerge) 
{
	if (!bMerge)
	{
		theApp.emuledlg->serverwnd->serverlistctrl.DeleteAllItems();
		RemoveAllServers();
	}

	CSafeBufferedFile servermet;
	CFileException fexp;
	if (!servermet.Open(strFile ,CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (!bMerge){
			CString strError(GetResString(IDS_ERR_LOADSERVERMET));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError,ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			LogError(LOG_STATUSBAR, _T("%s"), strError);
		}
		return false;
	}
	setvbuf(servermet.m_pStream, NULL, _IOFBF, 16384);
	try{
		version = servermet.ReadUInt8();
		if (version != 0xE0 && version != MET_HEADER){
			servermet.Close();
			LogError(LOG_STATUSBAR,GetResString(IDS_ERR_BADSERVERMETVERSION), version);
			return false;
		}
		theApp.emuledlg->serverwnd->serverlistctrl.Hide();
		theApp.emuledlg->serverwnd->serverlistctrl.SetRedraw(FALSE);
		UINT fservercount = servermet.ReadUInt32();
		
		ServerMet_Struct sbuffer;
		UINT iAddCount = 0;
		for (UINT j = 0; j < fservercount; j++)
		{
			// get server
			servermet.Read(&sbuffer, sizeof(ServerMet_Struct));
			CServer* newserver = new CServer(&sbuffer);

			// add tags
			for (UINT i = 0; i < sbuffer.tagcount; i++)
				newserver->AddTagFromFile(&servermet);

			if (bMerge) {
				// If we are merging a (downloaded) server list into our list, ignore the priority of the
				// server -- some server list providers are doing a poor job with this and offering lists
				// with dead servers set to 'High'..
				newserver->SetPreference(SRV_PR_NORMAL);
			}

			// set listname for server
			if (newserver->GetListName().IsEmpty())
				newserver->SetListName(newserver->GetAddress());

			if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(newserver, true, true))
			{
				CServer* update = theApp.serverlist->GetServerByAddress(newserver->GetAddress(), newserver->GetPort());
				if (update)
				{
					update->SetListName(newserver->GetListName());
					update->SetDescription(newserver->GetDescription());
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(update);
				}
				delete newserver;
			}
			else
				iAddCount++;
		}

		if (!bMerge)
			AddLogLine(true,GetResString(IDS_SERVERSFOUND), fservercount);
		else
			AddLogLine(true,GetResString(IDS_SERVERSADDED), iAddCount, fservercount - iAddCount);
		servermet.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile){
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_BADSERVERLIST));
		}
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FILEERROR_SERVERMET),buffer);
		}
		error->Delete();
	}
	theApp.emuledlg->serverwnd->serverlistctrl.SetRedraw(TRUE);
	theApp.emuledlg->serverwnd->serverlistctrl.Visable();
	return true;
}

bool CServerList::AddServer(const CServer* pServer, bool bAddTail)
{
	if (!IsGoodServerIP(pServer)){ // check for 0-IP, localhost and optionally for LAN addresses
		if (thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("IPFilter(AddServer): Filtered server \"%s\" (IP=%s) - Invalid IP or LAN address."), pServer->GetListName(), ipstr(pServer->GetIP()));
		return false;
	}

	if (thePrefs.GetFilterServerByIP()){
		// IP-Filter: We don't need to reject dynIP-servers here. After the DN was
		// resolved, the IP will get filtered and the server will get removed. This applies
		// for TCP-connections as well as for outgoing UDP-packets because for both protocols
		// we resolve the DN and filter the received IP.
		//if (pServer->HasDynIP())
		//	return false;
		if (theApp.ipfilter->IsFiltered(pServer->GetIP())){
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("IPFilter(AddServer): Filtered server \"%s\" (IP=%s) - IP filter (%s)"), pServer->GetListName(), ipstr(pServer->GetIP()), theApp.ipfilter->GetLastHit());
			return false;
		}
	}

	CServer* pFoundServer = GetServerByAddress(pServer->GetAddress(), pServer->GetPort());
	// Avoid duplicate (dynIP) servers: If the server which is to be added, is a dynIP-server
	// but we don't know yet it's DN, we need to search for an already available server with
	// that IP.
	if (pFoundServer == NULL && pServer->GetIP() != 0)
		pFoundServer = GetServerByIPTCP(pServer->GetIP(), pServer->GetPort());
	if (pFoundServer){
		pFoundServer->ResetFailedCount();
		theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(pFoundServer);
		return false;
	}
	if (bAddTail)
		list.AddTail(const_cast<CServer*>(pServer));
	else
		list.AddHead(const_cast<CServer*>(pServer));
	return true;
}

bool CServerList::GiveServersForTraceRoute()
{
    return theApp.lastCommonRouteFinder->AddHostsToCheck(list);
}

void CServerList::ServerStats()
{
	// Update the server list even if we are connected to Kademlia only. The idea is for both networks to keep 
	// each other up to date.. Kad network can get you back into the ED2K network.. And the ED2K network can get 
	// you back into the Kad network..
	if (theApp.IsConnected() && theApp.serverconnect->IsUDPSocketAvailable() && list.GetCount() > 0)
	{
		CServer* ping_server = GetNextStatServer();
		if (!ping_server)
			return;

		uint32 tNow = (uint32)time(NULL);
		const CServer* test = ping_server;
        while (ping_server->GetLastPingedTime() != 0 && (tNow - ping_server->GetLastPingedTime()) < UDPSERVSTATREASKTIME)
		{
			ping_server = GetNextStatServer();
			if (ping_server == test)
				return;
		}
		// IP-filter: We do not need to IP-filter any servers here, even dynIP-servers are not
		// needed to get filtered here. See also comments in 'CServerSocket::ConnectTo'.
		if (ping_server->GetFailedCount() >= thePrefs.GetDeadServerRetries()) {
			theApp.emuledlg->serverwnd->serverlistctrl.RemoveServer(ping_server);
			return;
		}
		srand(tNow);
		ping_server->SetRealLastPingedTime(tNow); // this is not used to calcualte the next ping, but only to ensure a minimum delay for premature pings
		if (!ping_server->GetCryptPingReplyPending() && (tNow - ping_server->GetLastPingedTime()) >= UDPSERVSTATREASKTIME && theApp.GetPublicIP() != 0 && thePrefs.IsServerCryptLayerUDPEnabled()){
			// we try a obfsucation ping first and wait 20 seconds for an answer
			// if it doesn'T get responsed, we don't count it as error but continue with a normal ping
			ping_server->SetCryptPingReplyPending(true);
			uint32 nPacketLen = 4 + (uint8)(rand() % 16); // max padding 16 bytes
			BYTE* pRawPacket = new BYTE[nPacketLen];
			uint32 dwChallenge = (rand() << 17) | (rand() << 2) | (rand() & 0x03);
			if (dwChallenge == 0)
				dwChallenge++;
			PokeUInt32(pRawPacket, dwChallenge);
			for (uint32 i = 4; i < nPacketLen; i++) // fillung up the remaining bytes with random data
				pRawPacket[i] = (uint8)rand();

			ping_server->SetChallenge(dwChallenge);
			ping_server->SetLastPinged(GetTickCount());
			ping_server->SetLastPingedTime((tNow - (uint32)UDPSERVSTATREASKTIME) + 20); // give it 20 seconds to respond
			
			if (thePrefs.GetDebugServerUDPLevel() > 0)
				Debug(_T(">>> Sending OP__GlobServStatReq (obfuscated) to server %s:%u\n"), ping_server->GetAddress(), ping_server->GetPort());

			theStats.AddUpDataOverheadServer(nPacketLen);
			theApp.serverconnect->SendUDPPacket(NULL, ping_server, true, ping_server->GetPort() + 12, pRawPacket, nPacketLen);
		}
		else if (ping_server->GetCryptPingReplyPending() || theApp.GetPublicIP() == 0 || !thePrefs.IsServerCryptLayerUDPEnabled()){
			// our obfsucation ping request was not answered, so probably the server doesn'T supports obfuscation
			// continue with a normal request
			if (ping_server->GetCryptPingReplyPending() && thePrefs.IsServerCryptLayerUDPEnabled())
				DEBUG_ONLY(DebugLog(_T("CryptPing failed for server %s"), ping_server->GetListName()));
			else if (thePrefs.IsServerCryptLayerUDPEnabled())
				DEBUG_ONLY(DebugLog(_T("CryptPing skipped because our public IP is unknown for server %s"), ping_server->GetListName()));
			
			ping_server->SetCryptPingReplyPending(false);			
			Packet* packet = new Packet(OP_GLOBSERVSTATREQ, 4);
			uint32 uChallenge = 0x55AA0000 + GetRandomUInt16();
			ping_server->SetChallenge(uChallenge);
			PokeUInt32(packet->pBuffer, uChallenge);
			ping_server->SetLastPinged(GetTickCount());
			ping_server->SetLastPingedTime(tNow - (rand() % HR2S(1)));
			ping_server->AddFailedCount();
			theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(ping_server);
			if (thePrefs.GetDebugServerUDPLevel() > 0)
				Debug(_T(">>> Sending OP__GlobServStatReq (not obfuscated) to server %s:%u\n"), ping_server->GetAddress(), ping_server->GetPort());
			theStats.AddUpDataOverheadServer(packet->size);
			theApp.serverconnect->SendUDPPacket(packet, ping_server, true);
		}
		else
			ASSERT( false );
	}
}

bool CServerList::IsGoodServerIP(const CServer* pServer) const
{
	if (pServer->HasDynIP())
		return true;
	return IsGoodIPPort(pServer->GetIP(), pServer->GetPort());
}

void CServerList::RemoveServer(const CServer* pServer)
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; )
	{
		POSITION pos2 = pos;
		const CServer* test_server = list.GetNext(pos);
		if (test_server == pServer){
			if (theApp.downloadqueue->cur_udpserver == pServer)
				theApp.downloadqueue->cur_udpserver = NULL;
			list.RemoveAt(pos2);
			delservercount++;
			delete test_server;
			return;
		}
	}
}

void CServerList::RemoveAllServers()
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; ){
		delete list.GetNext(pos);
		delservercount++;
	}
	list.RemoveAll();
}

void CServerList::GetStatus(uint32& total, uint32& failed, 
							uint32& user, uint32& file, uint32& lowiduser,
							uint32& totaluser, uint32& totalfile, 
							float& occ) const
{
	total = list.GetCount();
	failed = 0;
	user = 0;
	file = 0;
	totaluser = 0;
	totalfile = 0;
	occ = 0;
	lowiduser = 0;

	uint32 maxuserknownmax = 0;
	uint32 totaluserknownmax = 0;
	for (POSITION pos = list.GetHeadPosition(); pos != 0; ){
		const CServer* curr = list.GetNext(pos);
		if (curr->GetFailedCount()){
			failed++;
		}
		else{
			user += curr->GetUsers();
			file += curr->GetFiles();
			lowiduser += curr->GetLowIDUsers();
		}
		totaluser += curr->GetUsers();
		totalfile += curr->GetFiles();
		
		if (curr->GetMaxUsers()){
			totaluserknownmax += curr->GetUsers(); // total users on servers with known maximum
			maxuserknownmax += curr->GetMaxUsers();
		}
	}
	
	if (maxuserknownmax > 0)
		occ = (float)(totaluserknownmax * 100) / maxuserknownmax;
}

void CServerList::GetAvgFile(uint32& average) const
{
	//Since there is no real way to know how many files are in the kad network,
	//I figure to try to use the ED2K network stats to find how many files the
	//average user shares..
	uint32 totaluser = 0;
	uint32 totalfile = 0;
	for (POSITION pos = list.GetHeadPosition(); pos != 0; ){
		const CServer* curr = list.GetNext(pos);
		//If this server has reported Users/Files and doesn't limit it's files too much
		//use this in the calculation..
		if( curr->GetUsers() && curr->GetFiles() && curr->GetSoftFiles() > 1000 )
		{
			totaluser += curr->GetUsers();
			totalfile += curr->GetFiles();
		}
	}
	//If the user count is a little low, don't send back a average..
	//I added 50 to the count as many servers do not allow a large amount of files to be shared..
	//Therefore the extimate here will be lower then the actual.
	//I would love to add a way for the client to send some statistics back so we could see the real
	//values here..
	if ( totaluser > 500000 )
		average = (totalfile/totaluser)+50;
	else
		average = 0;
}

void CServerList::GetUserFileStatus(uint32& user, uint32& file) const
{
	user = 0;
	file = 0;
	for (POSITION pos = list.GetHeadPosition(); pos != 0; ){
		const CServer* curr = list.GetNext(pos);
		if( !curr->GetFailedCount() ){
			user += curr->GetUsers();
			file += curr->GetFiles();
		}
	}
}

void CServerList::MoveServerDown(const CServer* pServer)
{
	POSITION pos1, pos2;
	int i = 0;
	for (pos1 = list.GetHeadPosition(); (pos2 = pos1) != NULL; ) {
		list.GetNext(pos1);
		CServer* cur_server = list.GetAt(pos2);
		if (cur_server == pServer) {
			list.AddTail(cur_server);
			list.RemoveAt(pos2);
			return;
		}
		i++;
		if (i == list.GetCount())
			break;
	}
}

void CServerList::Sort()
{
	POSITION pos1, pos2;
	int i = 0;
	for (pos1 = list.GetHeadPosition(); (pos2 = pos1) != NULL; ) {
		list.GetNext(pos1);
		CServer* cur_server = list.GetAt(pos2);
		if (cur_server->GetPreference() == SRV_PR_HIGH) {
			list.AddHead(cur_server);
			list.RemoveAt(pos2);
		}
		else if (cur_server->GetPreference() == SRV_PR_LOW) {
			list.AddTail(cur_server);
			list.RemoveAt(pos2);
		}
		i++;
		if (i == list.GetCount())
			break;
	}
}

void CServerList::GetUserSortedServers()
{
	CServerListCtrl& serverListCtrl = theApp.emuledlg->serverwnd->serverlistctrl;
	ASSERT( serverListCtrl.GetItemCount() == list.GetCount() );
	list.RemoveAll();
	int iServers = serverListCtrl.GetItemCount();
	for (int i = 0; i < iServers; i++)
		list.AddTail((CServer*)serverListCtrl.GetItemData(i));
}

#ifdef _DEBUG
void CServerList::Dump()
{
	int i = 1;
	POSITION pos = list.GetHeadPosition();
	while (pos)
	{
		const CServer* pServer = list.GetNext(pos);
		TRACE(_T("%3u: Pri=%s \"%s\"\n"), i, pServer->GetPreference() == SRV_PR_HIGH ? _T("Hi") : (pServer->GetPreference() == SRV_PR_LOW ? _T("Lo") : _T("No")), pServer->GetListName());
		i++;
	}
}
#endif

void CServerList::SetServerPosition(UINT newPosition)
{
	if (newPosition < (UINT)list.GetCount())
		serverpos = newPosition;
	else
		serverpos = 0;
}

CServer* CServerList::GetNextServer(bool bOnlyObfuscated)
{
	if (serverpos >= (UINT)list.GetCount())
		return NULL;

	CServer* nextserver = NULL;
	int i = 0;
	while (!nextserver && i < list.GetCount()){
		POSITION posIndex = list.FindIndex(serverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			serverpos = 0;
		}

		serverpos++;
		i++;
		if (!bOnlyObfuscated || ((CServer*)list.GetAt(posIndex))->SupportsObfuscationTCP())
			nextserver = list.GetAt(posIndex);
		else if (serverpos >= (UINT)list.GetCount())
			return NULL;
	}
	return nextserver;
}

CServer* CServerList::GetNextSearchServer()
{
	CServer* nextserver = NULL;
	int i = 0;
	while (!nextserver && i < list.GetCount()){
		POSITION posIndex = list.FindIndex(searchserverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			searchserverpos = 0;
		}
		nextserver = list.GetAt(posIndex);
		searchserverpos++;
		i++;
		if (searchserverpos == (UINT)list.GetCount())
			searchserverpos = 0;
	}
	return nextserver;
}

CServer* CServerList::GetNextStatServer()
{
	CServer* nextserver = NULL;
	int i = 0;
	while (!nextserver && i < list.GetCount()){
		POSITION posIndex = list.FindIndex(statserverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			statserverpos = 0;
		}
		nextserver = list.GetAt(posIndex);
		statserverpos++;
		i++;
		if (statserverpos == (UINT)list.GetCount())
			statserverpos = 0;
	}
	return nextserver;
}

CServer* CServerList::GetSuccServer(const CServer* lastserver) const
{
	if (list.IsEmpty())
		return NULL;
	if (!lastserver)
		return list.GetHead();

	POSITION pos = list.Find(const_cast<CServer*>(lastserver));
	if (!pos)
		return list.GetHead();
	list.GetNext(pos);
	if (!pos)
		return NULL;
	return list.GetAt(pos);
}

CServer* CServerList::GetServerByAddress(LPCTSTR address, uint16 port) const
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
        if ((port == s->GetPort() || port == 0) && !_tcscmp(s->GetAddress(), address))
			return s; 
	}
	return NULL;
}

CServer* CServerList::GetServerByIP(uint32 nIP) const
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP)
			return s;
	}
	return NULL;
}

CServer* CServerList::GetServerByIPTCP(uint32 nIP, uint16 nTCPPort) const
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP && s->GetPort() == nTCPPort)
			return s;
	}
	return NULL;
}

CServer* CServerList::GetServerByIPUDP(uint32 nIP, uint16 nUDPPort, bool bObfuscationPorts) const
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP && (s->GetPort() == nUDPPort-4 ||
			(bObfuscationPorts && (s->GetObfuscationPortUDP() == nUDPPort) || (s->GetPort() == nUDPPort - 12))))
			return s;
	}
	return NULL;
}

bool CServerList::SaveServermetToFile()
{
	if (thePrefs.GetLogFileSaving())
		AddDebugLogLine(false, _T("Saving servers list file \"%s\""), SERVER_MET_FILENAME);
	m_nLastSaved = ::GetTickCount(); 
	CString newservermet(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
	newservermet += SERVER_MET_FILENAME _T(".new");
	CSafeBufferedFile servermet;
	CFileException fexp;
	if (!servermet.Open(newservermet, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		CString strError(GetResString(IDS_ERR_SAVESERVERMET));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		return false;
	}
	setvbuf(servermet.m_pStream, NULL, _IOFBF, 16384);
	
	try{
		servermet.WriteUInt8(0xE0);
		
		UINT fservercount = list.GetCount();
		servermet.WriteUInt32(fservercount);
		
		for (UINT j = 0; j < fservercount; j++)
		{
			const CServer* nextserver = GetServerAt(j);

			// don't write potential out-dated IPs of dynIP-servers
			servermet.WriteUInt32(nextserver->HasDynIP() ? 0 : nextserver->GetIP());
			servermet.WriteUInt16(nextserver->GetPort());
			
			UINT uTagCount = 0;
			ULONG uTagCountFilePos = (ULONG)servermet.GetPosition();
			servermet.WriteUInt32(uTagCount);

			if (!nextserver->GetListName().IsEmpty()){
				CTag servername(ST_SERVERNAME, nextserver->GetListName());
				servername.WriteTagToFile(&servermet, utf8strOptBOM);
				uTagCount++;
			}
			
			if (!nextserver->GetDynIP().IsEmpty()){
				CTag serverdynip(ST_DYNIP, nextserver->GetDynIP());
				serverdynip.WriteTagToFile(&servermet, utf8strOptBOM);
				uTagCount++;
			}
			
			if (!nextserver->GetDescription().IsEmpty()){
				CTag serverdesc(ST_DESCRIPTION, nextserver->GetDescription());
				serverdesc.WriteTagToFile(&servermet, utf8strOptBOM);
				uTagCount++;
			}
			
			if (nextserver->GetFailedCount()){
				CTag serverfail(ST_FAIL, nextserver->GetFailedCount());
				serverfail.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetPreference() != SRV_PR_NORMAL){
				CTag serverpref(ST_PREFERENCE, nextserver->GetPreference());
				serverpref.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetUsers()){
				CTag serveruser("users", nextserver->GetUsers());
				serveruser.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetFiles()){
				CTag serverfiles("files", nextserver->GetFiles());
				serverfiles.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetPing()){
				CTag serverping(ST_PING, nextserver->GetPing());
				serverping.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetLastPingedTime()){
				CTag serverlastp(ST_LASTPING, nextserver->GetLastPingedTime());
				serverlastp.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetMaxUsers()){
				CTag servermaxusers(ST_MAXUSERS, nextserver->GetMaxUsers());
				servermaxusers.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetSoftFiles()){
				CTag softfiles(ST_SOFTFILES, nextserver->GetSoftFiles());
				softfiles.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetHardFiles()){
				CTag hardfiles(ST_HARDFILES, nextserver->GetHardFiles());
				hardfiles.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (!nextserver->GetVersion().IsEmpty()){
				// as long as we don't receive an integer version tag from the local server (TCP) we store it as string
				CTag version(ST_VERSION, nextserver->GetVersion());
				version.WriteTagToFile(&servermet, utf8strOptBOM);
				uTagCount++;
			}
			
			if (nextserver->GetUDPFlags()){
				CTag tagUDPFlags(ST_UDPFLAGS, nextserver->GetUDPFlags());
				tagUDPFlags.WriteTagToFile(&servermet);
				uTagCount++;
			}
			
			if (nextserver->GetLowIDUsers()){
				CTag tagLowIDUsers(ST_LOWIDUSERS, nextserver->GetLowIDUsers());
				tagLowIDUsers.WriteTagToFile(&servermet);
				uTagCount++;
			}

			if (nextserver->GetServerKeyUDP(true)){
				CTag tagServerKeyUDP(ST_UDPKEY, nextserver->GetServerKeyUDP(true));
				tagServerKeyUDP.WriteTagToFile(&servermet);
				uTagCount++;
			}

			if (nextserver->GetServerKeyUDPIP()){
				CTag tagServerKeyUDPIP(ST_UDPKEYIP, nextserver->GetServerKeyUDPIP());
				tagServerKeyUDPIP.WriteTagToFile(&servermet);
				uTagCount++;
			}

			if (nextserver->GetObfuscationPortTCP()){
				CTag tagObfuscationPortTCP(ST_TCPPORTOBFUSCATION, nextserver->GetObfuscationPortTCP());
				tagObfuscationPortTCP.WriteTagToFile(&servermet);
				uTagCount++;
			}

			if (nextserver->GetObfuscationPortUDP()){
				CTag tagObfuscationPortUDP(ST_UDPPORTOBFUSCATION, nextserver->GetObfuscationPortUDP());
				tagObfuscationPortUDP.WriteTagToFile(&servermet);
				uTagCount++;
			}

			servermet.Seek(uTagCountFilePos, CFile::begin);
			servermet.WriteUInt32(uTagCount);
			servermet.SeekToEnd();
		}

		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			servermet.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(servermet.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), servermet.GetFileName());
		}
		servermet.Close();

		CString curservermet(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
		CString oldservermet(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR));
		curservermet += SERVER_MET_FILENAME;
		oldservermet += _T("server_met.old");
		
		if (_taccess(oldservermet, 0) == 0)
			CFile::Remove(oldservermet);
		if (_taccess(curservermet, 0) == 0)
			CFile::Rename(curservermet,oldservermet);
		CFile::Rename(newservermet,curservermet);
	}
	catch(CFileException* error) {
		CString strError(GetResString(IDS_ERR_SAVESERVERMET2));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		error->Delete();
		return false;
	}
	return true;
}

void CServerList::AddServersFromTextFile(const CString& strFilename)
{
	CStdioFile f;
	// open the text file either in ANSI (text) or Unicode (binary), this way we can read old and new files
	// with nearly the same code..
	bool bIsUnicodeFile = IsUnicodeFile(strFilename); // check for BOM
	if (!f.Open(strFilename, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
		return;

	try	{
		if (bIsUnicodeFile)
			f.Seek(sizeof(WORD), CFile::current); // skip BOM

		CString strLine;
		while (f.ReadString(strLine))
		{
			// format is host:port,priority,Name
			if (strLine.GetLength() < 5)
				continue;
			if (strLine.GetAt(0) == _T('#') || strLine.GetAt(0) == _T('/'))
				continue;

			// fetch host
			int pos = strLine.Find(_T(':'));
			if (pos == -1){
				pos = strLine.Find(_T(','));
				if (pos == -1) 
					continue;
			}
			CString strHost = strLine.Left(pos);
			strLine = strLine.Mid(pos+1);
			// fetch  port
			pos = strLine.Find(_T(','));
			if (pos == -1)
				continue;
			CString strPort = strLine.Left(pos);
			strLine = strLine.Mid(pos+1);

			// Barry - fetch priority
			pos = strLine.Find(_T(','));
			int priority = SRV_PR_HIGH;
			if (pos == 1)
			{
				CString strPriority = strLine.Left(pos);
				priority = _tstoi(strPriority);
				if (priority < 0 || priority > 2)
					priority = SRV_PR_HIGH;
				strLine = strLine.Mid(pos+1);
			}
		
			// fetch name
			CString strName = strLine;
			strName.Remove(_T('\r'));
			strName.Remove(_T('\n'));

			// create server object and add it to the list
			CServer* nsrv = new CServer((uint16)_tstoi(strPort), strHost);
			nsrv->SetListName(strName);
			nsrv->SetIsStaticMember(true);
			nsrv->SetPreference(priority); 
			if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(nsrv, true))
			{
				delete nsrv;
				CServer* srvexisting = GetServerByAddress(strHost, (uint16)_tstoi(strPort));
				if (srvexisting) {
					srvexisting->SetListName(strName);
					srvexisting->SetIsStaticMember(true);
					srvexisting->SetPreference(priority); 
					if (theApp.emuledlg->serverwnd)
						theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(srvexisting);
				}
			}
		}
		f.Close();
	}
	catch (CFileException* ex) {
		ASSERT(0);
		ex->Delete();
	}
}

bool CServerList::SaveStaticServers()
{
	bool bResult = false;

	FILE* fpStaticServers = _tfsopen(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("staticservers.dat"), _T("wb"), _SH_DENYWR);
	if (fpStaticServers == NULL) {
		LogError(LOG_STATUSBAR, GetResString(IDS_ERROR_SSF));
		return bResult;
	}

	// write Unicode byte-order mark 0xFEFF
	if (fputwc(0xFEFF, fpStaticServers) != _TEOF)
	{
		bResult = true;
		POSITION pos = list.GetHeadPosition();
		while (pos)
		{
			const CServer* pServer = list.GetNext(pos);
			if (pServer->IsStaticMember())
			{
				if (_ftprintf(fpStaticServers, _T("%s:%i,%i,%s\r\n"), pServer->GetAddress(), pServer->GetPort(), pServer->GetPreference(), pServer->GetListName()) == EOF) {
					bResult = false;
					break;
				}
			}
		}
	}

	fclose(fpStaticServers);
	return bResult;
}

void CServerList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(17))
		SaveServermetToFile();
}

int CServerList::GetPositionOfServer(const CServer* pServer) const
{
	int iPos = -1;
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; ){
		iPos++;
		if (pServer == list.GetNext(pos))
			break;
	}
	return iPos;
}

void CServerList::RemoveDuplicatesByAddress(const CServer* pExceptThis)
{
	POSITION pos = list.GetHeadPosition();
	while (pos)
	{
		CServer* pServer = list.GetNext(pos);
		if (pServer == pExceptThis)
			continue;
		if (   _tcsicmp(pServer->GetAddress(), pExceptThis->GetAddress()) == 0
			&& pServer->GetPort() == pExceptThis->GetPort())
		{
			theApp.emuledlg->serverwnd->serverlistctrl.RemoveServer(pServer);
			return;
		}
	}
}

void CServerList::RemoveDuplicatesByIP(const CServer* pExceptThis)
{
	POSITION pos = list.GetHeadPosition();
	while (pos)
	{
		CServer* pServer = list.GetNext(pos);
		if (pServer == pExceptThis)
			continue;
		if (pServer->GetIP() == pExceptThis->GetIP() && pServer->GetPort() == pExceptThis->GetPort())
		{
			theApp.emuledlg->serverwnd->serverlistctrl.RemoveServer(pServer);
			return;
		}
	}
}

void CServerList::CheckForExpiredUDPKeys() {
	
	if (!thePrefs.IsServerCryptLayerUDPEnabled())
		return;

	uint32 cKeysTotal = 0;
	uint32 cKeysExpired = 0;
	uint32 cPingDelayed = 0;
	const uint32 dwIP = theApp.GetPublicIP();
	const uint32 tNow = (uint32)time(NULL);
	ASSERT( dwIP != 0 );

	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* pServer = list.GetNext(pos);
		if (pServer->SupportsObfuscationUDP() && pServer->GetServerKeyUDP(true) != 0 && pServer->GetServerKeyUDPIP() != dwIP){
			cKeysTotal++;
			cKeysExpired++;
			if (tNow - pServer->GetRealLastPingedTime() < UDPSERVSTATMINREASKTIME){
				cPingDelayed++;
				// next ping: Now + (MinimumDelay - already elapsed time)
				pServer->SetLastPingedTime((tNow - (uint32)UDPSERVSTATREASKTIME) + (UDPSERVSTATMINREASKTIME - (tNow - pServer->GetRealLastPingedTime())));
			}
			else
				pServer->SetLastPingedTime(0);
		}
		else if (pServer->SupportsObfuscationUDP() && pServer->GetServerKeyUDP(false) != 0)
			cKeysTotal++;
	}

	DebugLog(_T("Possible IP Change - Checking for expired Server UDP-Keys: %u UDP Keys total, %u UDP Keys expired, %u immediate UDP Pings forced, %u delayed UDP Pings forced"),
		cKeysTotal, cKeysExpired, cKeysExpired - cPingDelayed, cPingDelayed); 
}
