//this file is part of eMule
//Copyright (C)2003-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "mmserver.h"
#include "opcodes.h"
#include "md5sum.h"
#include "packets.h"
#include "searchFile.h"
#include "searchlist.h"
#include "Exceptions.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
#include "Statistics.h"
#include "MMSocket.h"
#include "Sockets.h"
#include "Server.h"
#include "PartFile.h"
#include "KnownFileList.h"
#include "CxImage/xImage.h"
#include "WebServer.h"
#include "otherfunctions.h"
#include "Preferences.h"
#include "SearchParams.h"
#include "kademlia/kademlia/kademlia.h"
#include "emuledlg.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CMMServer::CMMServer(void)
{
	m_SendSearchList.SetSize(0);
	h_timer = NULL;
	m_cPWFailed = 0;
	m_dwBlocked = 0;
	m_pSocket = NULL;
	m_nSessionID = 0;
	m_pPendingCommandSocket = NULL;
	m_nMaxDownloads = 0;
	m_nMaxBufDownloads = 0;
	m_bGrabListLogin =false;

}

CMMServer::~CMMServer(void)
{
	DeleteSearchFiles();
	delete m_pSocket;
	if (h_timer != NULL){
		KillTimer(0,h_timer);
	}
}

void CMMServer::Init(){
	if (thePrefs.IsMMServerEnabled() && !m_pSocket){
		m_pSocket = new CListenMMSocket(this);
		if (!m_pSocket->Create()){
			StopServer();
			AddLogLine(false, GetResString(IDS_MMFAILED) );
		}
		else{
			AddLogLine(false, GetResString(IDS_MMSTARTED), thePrefs.GetMMPort(), _T(MM_STRVERSION));
		}
	}
}

void CMMServer::StopServer(){
	delete m_pSocket;
	m_pSocket = NULL;
}

void CMMServer::DeleteSearchFiles(){
	for (int i = 0; i!= m_SendSearchList.GetSize(); i++){
		delete m_SendSearchList[i];
	}
	m_SendSearchList.SetSize(0);
}

bool CMMServer::PreProcessPacket(char* pPacket, uint32 nSize, CMMSocket* sender){
	if (nSize >= 3){
		uint16 nSessionID;
		memcpy(&nSessionID,pPacket+1,sizeof(nSessionID));
		if ( (m_nSessionID && nSessionID == m_nSessionID) || pPacket[0] == MMP_HELLO){
			return true;
		}
		else{
			CMMPacket* packet = new CMMPacket(MMP_INVALIDID);
			sender->SendPacket(packet);
			m_nSessionID = 0;
			return false;
		}
	}
	
	CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
	sender->SendPacket(packet);
	return false;
}

void CMMServer::ProcessHelloPacket(CMMData* data, CMMSocket* sender){
	CMMPacket* packet = new CMMPacket(MMP_HELLOANS);
	if (data->ReadByte() != MM_VERSION){
		packet->WriteByte(MMT_WRONGVERSION);
		sender->SendPacket(packet);
		return;
	}
	else{
		if(m_dwBlocked && m_dwBlocked > ::GetTickCount()){
			packet->WriteByte(MMT_WRONGPASSWORD);
			sender->SendPacket(packet);
			return;
		}
		CString plainPW = data->ReadString();
		CString testValue =MD5Sum(plainPW).GetHash();
		if (testValue != thePrefs.GetMMPass() || plainPW.GetLength() == 0 ){
			m_dwBlocked = 0;
			packet->WriteByte(MMT_WRONGPASSWORD);
			sender->SendPacket(packet);
			m_cPWFailed++;
			if (m_cPWFailed == 3){
				AddLogLine(false, GetResString(IDS_MM_BLOCK));
				m_cPWFailed = 0;
				m_dwBlocked = ::GetTickCount() + MMS_BLOCKTIME;
			}
			return;
		}
		else{
			m_bUseFakeContent = (data->ReadByte() != 0); 
			m_nMaxDownloads = data->ReadShort();
			m_nMaxBufDownloads = data->ReadShort();
			m_bGrabListLogin = (data->ReadByte() != 0);

			// everything ok, new sessionid
			AddLogLine(false, GetResString(IDS_MM_NEWUSER));
			packet->WriteByte(MMT_OK);
			m_nSessionID = (uint16)rand();
			packet->WriteShort(m_nSessionID);
			packet->WriteString(thePrefs.GetUserNick());
			packet->WriteShort((uint16)((thePrefs.GetMaxUpload() >= UNLIMITED) ? 0 : thePrefs.GetMaxUpload()));
			packet->WriteShort((uint16)((thePrefs.GetMaxDownload() >= UNLIMITED) ? 0 : (uint16)thePrefs.GetMaxDownload()));
			ProcessStatusRequest(sender,packet);
			//sender->SendPacket(packet);
		}

	}

}

void CMMServer::ProcessStatusRequest(CMMSocket* sender, CMMPacket* packet){
	if (packet == NULL)
		packet = new CMMPacket(MMP_STATUSANSWER);
	else
		packet->WriteByte(MMP_STATUSANSWER);

	packet->WriteShort((uint16)theApp.uploadqueue->GetDatarate()/100);
	packet->WriteShort((uint16)((thePrefs.GetMaxGraphUploadRate(true)*1024)/100));
	packet->WriteShort((uint16)theApp.downloadqueue->GetDatarate()/100);
	packet->WriteShort((uint16)((thePrefs.GetMaxGraphDownloadRate()*1024)/100));
	packet->WriteByte((uint8)theApp.downloadqueue->GetDownloadingFileCount());
	packet->WriteByte((uint8)theApp.downloadqueue->GetPausedFileCount());
	packet->WriteInt((uint32)(theStats.sessionReceivedBytes/1048576));
	packet->WriteShort((uint16)((theStats.GetAvgDownloadRate(0)*1024)/100));
	if (theApp.serverconnect->IsConnected()){
		if(theApp.serverconnect->IsLowID())
			packet->WriteByte(1);
		else
			packet->WriteByte(2);
		if (theApp.serverconnect->GetCurrentServer() != NULL){
			packet->WriteInt(theApp.serverconnect->GetCurrentServer()->GetUsers());
			packet->WriteString(theApp.serverconnect->GetCurrentServer()->GetListName());
		}
		else{
			packet->WriteInt(0);
			packet->WriteString("");
		}
	}
	else{
		packet->WriteByte(0);
		packet->WriteInt(0);
		packet->WriteString("");
	}

	if(thePrefs.GetNetworkKademlia()){
		if ( Kademlia::CKademlia::IsConnected() ){
			packet->WriteByte(2);
			packet->WriteInt(Kademlia::CKademlia::GetKademliaUsers());
		}
		else{
			packet->WriteByte(1);
			packet->WriteInt(0);
		}
	}
	else{
			packet->WriteByte(0);
			packet->WriteInt(0);
	}



	sender->SendPacket(packet);
}

void CMMServer::ProcessFileListRequest(CMMSocket* sender, CMMPacket* packet){
	
	if (packet == NULL)
		packet = new CMMPacket(MMP_FILELISTANS);
	else
		packet->WriteByte(MMP_FILELISTANS);
	
	int nCount = thePrefs.GetCatCount();
	packet->WriteByte((uint8)nCount);
	for (int i = 0; i != nCount; i++){
		packet->WriteString(thePrefs.GetCategory(i)->strTitle);
	}

	nCount = (theApp.downloadqueue->GetFileCount() > m_nMaxDownloads)? m_nMaxDownloads : theApp.downloadqueue->GetFileCount();
	m_SentFileList.SetSize(nCount);
	packet->WriteByte((uint8)nCount);
	for (int i = 0; i != nCount; i++){
		// while this is not the fastest method the trace this list, it's not timecritical here
		CPartFile* cur_file = theApp.downloadqueue->GetFileByIndex(i);
		if (cur_file == NULL){
			delete packet;
			packet = new CMMPacket(MMP_GENERALERROR);
			sender->SendPacket(packet);
			ASSERT ( false );
			return;
		}
		m_SentFileList[i] = cur_file;
		if (cur_file->GetStatus(false) == PS_PAUSED)
			packet->WriteByte(MMT_PAUSED);
		else{
			if (cur_file->GetTransferringSrcCount() > 0)
				packet->WriteByte(MMT_DOWNLOADING);
			else
				packet->WriteByte(MMT_WAITING);
		}
		packet->WriteString(cur_file->GetFileName());
		packet->WriteByte((uint8)cur_file->GetCategory());

		if (i < m_nMaxBufDownloads){
			packet->WriteByte(1);
			WriteFileInfo(cur_file,packet);
		}
		else{
			packet->WriteByte(0);
		}
	}
	sender->SendPacket(packet);
}

void CMMServer::ProcessFinishedListRequest(CMMSocket* sender){
	CMMPacket* packet = new CMMPacket(MMP_FINISHEDANS);
	int nCount = thePrefs.GetCatCount();
	packet->WriteByte((uint8)nCount);
	for (int i = 0; i != nCount; i++){
		packet->WriteString(thePrefs.GetCategory(i)->strTitle);
	}

	nCount = (m_SentFinishedList.GetCount() > 30)? 30 : m_SentFinishedList.GetCount();
	packet->WriteByte((uint8)nCount);
	for (int i = 0; i != nCount; i++){
		CKnownFile* cur_file = m_SentFinishedList[i];
		packet->WriteByte(0xFF);
		packet->WriteString(cur_file->GetFileName());
		if (cur_file->IsPartFile())
			packet->WriteByte((uint8)(((CPartFile*)cur_file)->GetCategory()));
		else
			packet->WriteByte(0);
	}
	sender->SendPacket(packet);
}

void CMMServer::ProcessFileCommand(CMMData* data, CMMSocket* sender){
	uint8 byCommand = data->ReadByte();
	uint8 byFileIndex = data->ReadByte();
	if (byFileIndex >= m_SentFileList.GetSize()
		|| !theApp.downloadqueue->IsPartFile(m_SentFileList[byFileIndex]))
	{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;		
	}
	CPartFile* selFile = m_SentFileList[byFileIndex];
	switch (byCommand){
		case MMT_PAUSE:
			selFile->PauseFile();
			break;
		case MMT_RESUME:
			selFile->ResumeFile();
			break;
		case MMT_CANCEL:{
			switch(selFile->GetStatus()) { 
				case PS_WAITINGFORHASH: 
				case PS_HASHING: 
				case PS_COMPLETING: 
				case PS_COMPLETE:  
					break;
				case PS_PAUSED:
					selFile->DeleteFile(); 
					break;
				default:
                    theApp.downloadqueue->StartNextFileIfPrefs(selFile->GetCategory());
					selFile->DeleteFile(); 
			}
			break;
		}
		default:
			CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
			sender->SendPacket(packet);
			return;
	}
	CMMPacket* packet = new CMMPacket(MMP_FILECOMMANDANS);
	ProcessFileListRequest(sender,packet); 

}

void  CMMServer::ProcessDetailRequest(CMMData* data, CMMSocket* sender){
	uint8 byFileIndex = data->ReadByte();
	if (byFileIndex >= m_SentFileList.GetSize()
		|| !theApp.downloadqueue->IsPartFile(m_SentFileList[byFileIndex]))
	{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;		
	}
	CPartFile* selFile = m_SentFileList[byFileIndex];
	CMMPacket* packet = new CMMPacket(MMP_FILEDETAILANS);
	WriteFileInfo(selFile, packet);
	sender->SendPacket(packet);
}


void  CMMServer::ProcessCommandRequest(CMMData* data, CMMSocket* sender){
	uint8 byCommand = data->ReadByte();
	bool bSuccess = false;
	bool bQueueCommand = false;
	switch(byCommand){
		case MMT_SDEMULE:
		case MMT_SDPC:
			h_timer = SetTimer(0,0,5000,CommandTimer);
			if (h_timer)
				bSuccess = true;
			bQueueCommand = true;
			break;
		case MMT_SERVERCONNECT:
			theApp.serverconnect->ConnectToAnyServer();
			bSuccess = true;
			break;
	}
	if (bSuccess){
		CMMPacket* packet = new CMMPacket(MMP_COMMANDANS);
		sender->SendPacket(packet);
		if (bQueueCommand)
			m_byPendingCommand = byCommand;
	}
	else{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;
	}
}

void  CMMServer::ProcessSearchRequest(CMMData* data, CMMSocket* sender){
	DeleteSearchFiles();
	SSearchParams Params;
	Params.strExpression = data->ReadString();
	uint8 byType = data->ReadByte();
	switch(byType){
		case 0:
			Params.strFileType.Empty();
			break;
		case 1:
			Params.strFileType = ED2KFTSTR_ARCHIVE;
			break;
		case 2:
			Params.strFileType = ED2KFTSTR_AUDIO;
			break;
		case 3:
			Params.strFileType = ED2KFTSTR_CDIMAGE;
			break;
		case 4:
			Params.strFileType = ED2KFTSTR_PROGRAM;
			break;
		case 5:
			Params.strFileType = ED2KFTSTR_VIDEO;
			break;
		case 6:
			Params.strFileType = ED2KFTSTR_EMULECOLLECTION;
			break;
		default:
			ASSERT ( false );
			Params.strFileType.Empty();
	}

	bool bServerError = false;

	if (!theApp.serverconnect->IsConnected()){
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->WriteByte(MMT_NOTCONNECTED);
		sender->SendPacket(packet);
		return;
	}

	CSafeMemFile searchdata(100);
	bool bResSearchPacket = false;
	try
	{
		bool bServerSupports64Bit = theApp.serverconnect->GetCurrentServer() != NULL
									&& (theApp.serverconnect->GetCurrentServer()->GetTCPFlags() & SRV_TCPFLG_LARGEFILES);
		bResSearchPacket = GetSearchPacket(&searchdata, &Params, bServerSupports64Bit, NULL);
	}
	catch (CMsgBoxException* ex)
	{
		ex->Delete();
	}

	if (!bResSearchPacket || searchdata.GetLength() == 0){
		bServerError = true;
	}
	else{
		h_timer = SetTimer(0,0,20000,CommandTimer);
		if (!h_timer){
			bServerError = true;
		}
	}
	if (bServerError){
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;
	}

	m_byPendingCommand = MMT_SEARCH;
	m_pPendingCommandSocket = sender;

	theApp.searchlist->NewSearch(NULL, Params.strFileType, MMS_SEARCHID, Params.eType, Params.strExpression, true);
	Packet* searchpacket = new Packet(&searchdata);
	searchpacket->opcode = OP_SEARCHREQUEST;
	theStats.AddUpDataOverheadServer(searchpacket->size);
	theApp.serverconnect->SendPacket(searchpacket,true);
	char buffer[500];
	int iBuffLen = _snprintf(buffer, _countof(buffer), "HTTP/1.1 200 OK\r\nConnection: Close\r\nContent-Type: %s\r\n", GetContentType());
	if (iBuffLen > 0)
		sender->Send(buffer,iBuffLen);
}

void  CMMServer::ProcessChangeLimitRequest(CMMData* data, CMMSocket* sender){
	uint16 nNewUpload = data->ReadShort();
	uint16 nNewDownload = data->ReadShort();
	thePrefs.SetMaxUpload(nNewUpload);
	thePrefs.SetMaxDownload(nNewDownload);

	CMMPacket* packet = new CMMPacket(MMP_CHANGELIMITANS);
	packet->WriteShort((uint16)((thePrefs.GetMaxUpload() >= UNLIMITED) ? 0 : thePrefs.GetMaxUpload()));
	packet->WriteShort((uint16)((thePrefs.GetMaxDownload() >= UNLIMITED) ? 0 : thePrefs.GetMaxDownload()));
	sender->SendPacket(packet);
}


void CMMServer::SearchFinished(bool bTimeOut){
#define MAXRESULTS	20
	if (h_timer != 0){
		KillTimer(0,h_timer);
		h_timer = 0;
	}
	if (m_pPendingCommandSocket == NULL)
		return;
	if (bTimeOut){
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->WriteByte(MMT_TIMEDOUT);
		packet->m_bSpecialHeader = true;
		m_pPendingCommandSocket->SendPacket(packet);
	}
	else if (theApp.searchlist->GetFoundFiles(MMS_SEARCHID) == 0){
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->WriteByte(MMT_NORESULTS);
		packet->m_bSpecialHeader = true;
		m_pPendingCommandSocket->SendPacket(packet);
	}
	else{
		UINT results = theApp.searchlist->GetFoundFiles(MMS_SEARCHID);
		if (results > MAXRESULTS)
			results = MAXRESULTS;
		m_SendSearchList.SetSize(results);
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->m_bSpecialHeader = true;
		packet->WriteByte(MMT_OK);
		packet->WriteByte((uint8)results);
		for (UINT i = 0; i < results; i++){
			CSearchFile* cur_file = theApp.searchlist->DetachNextFile(MMS_SEARCHID);
			m_SendSearchList[i] = cur_file;
			packet->WriteString(cur_file->GetFileName());
			packet->WriteShort((uint16)cur_file->GetSourceCount());
			packet->WriteInt64(cur_file->GetFileSize());
		}
		m_pPendingCommandSocket->SendPacket(packet);
		theApp.searchlist->RemoveResults(MMS_SEARCHID);
	}
	m_pPendingCommandSocket = NULL;
}

void  CMMServer::ProcessDownloadRequest(CMMData* data, CMMSocket* sender){
	uint8 byFileIndex = data->ReadByte();
	if (byFileIndex >= m_SendSearchList.GetSize() )
	{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;		
	}
	CSearchFile* todownload = m_SendSearchList[byFileIndex];
	theApp.downloadqueue->AddSearchToDownload(todownload, 2, 0);
	CMMPacket* packet = new CMMPacket(MMP_DOWNLOADANS);
	if (theApp.downloadqueue->GetFileByID(todownload->GetFileHash()) != NULL){
		packet->WriteByte(MMT_OK);
	}
	else{
		packet->WriteByte(MMT_FAILED);
	}
	sender->SendPacket(packet);
}

void  CMMServer::ProcessPreviewRequest(CMMData* data, CMMSocket* sender){
	uint8 byFileType = data->ReadByte();
	uint8 byFileIndex = data->ReadByte();
	uint16 nDisplayWidth = data->ReadShort();
	uint8 nNumber = data->ReadByte();
	CKnownFile* knownfile = NULL;
	bool bError = false;

	if (byFileType == MMT_PARTFILFE){
		if (byFileIndex >= m_SentFileList.GetSize()
		|| !theApp.downloadqueue->IsPartFile(m_SentFileList[byFileIndex]))
		{
			bError = true;	
		}
		else
			knownfile = m_SentFileList[byFileIndex];
	}
	else if (byFileType == MMT_FINISHEDFILE){
		if (byFileIndex >= m_SentFinishedList.GetSize()
		|| !theApp.knownfiles->IsKnownFile(m_SentFinishedList[byFileIndex]))
		{
			bError = true;	
		}
		else
			knownfile = m_SentFinishedList[byFileIndex];
	}

	if (!bError){
		if (h_timer != 0)
			bError = true;
		else{
			h_timer = SetTimer(0,0,20000,CommandTimer);
			if (!h_timer){
				bError = true;
			}
			else{
				if (nDisplayWidth > 140)
					nDisplayWidth = 140;
				m_byPendingCommand = MMT_PREVIEW;
				m_pPendingCommandSocket = sender;
				if (!knownfile->GrabImage(1,(nNumber+1)*50.0,true,nDisplayWidth,this))
					PreviewFinished(NULL,0);
			}
		}
	}

	if (bError){
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;
	}
}

void CMMServer::PreviewFinished(CxImage** imgFrames, uint8 nCount){
	if (h_timer != 0){
		KillTimer(0,h_timer);
		h_timer = 0;
	}
	if (m_byPendingCommand != MMT_PREVIEW)
		return;
	m_byPendingCommand = 0;
	if (m_pPendingCommandSocket == NULL)
		return;

	CMMPacket* packet = new CMMPacket(MMP_PREVIEWANS);
	if (imgFrames != NULL && nCount != 0){
		packet->WriteByte(MMT_OK);
		CxImage* cur_frame = imgFrames[0];
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
		packet->WriteInt(nResultSize);
		packet->m_pBuffer->Write(abyResultBuffer, nResultSize);
		free(abyResultBuffer);
	}
	else{
		packet->WriteByte(MMT_FAILED);
	}

	m_pPendingCommandSocket->SendPacket(packet);
	m_pPendingCommandSocket = NULL;
}

void CMMServer::Process(){
	if (m_pSocket){ 
		m_pSocket->Process(); 
	} 
}

CStringA CMMServer::GetContentType(){
	if (m_bUseFakeContent)
		return CStringA("image/vnd.wap.wbmp");
	else
		return CStringA("application/octet-stream");
}

VOID CALLBACK CMMServer::CommandTimer(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		KillTimer(0,theApp.mmserver->h_timer);
		theApp.mmserver->h_timer = 0;
		switch(theApp.mmserver->m_byPendingCommand){
			case MMT_SDPC:{
				HANDLE hToken; 
				TOKEN_PRIVILEGES tkp; 
				try{
					if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
						throw 1; 
					LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
					tkp.PrivilegeCount = 1;  // one privilege to set    
					tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
					AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
				}
				catch(...){
				}
				if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCEIFHUNG, 0)) 
					break;
			}
			case MMT_SDEMULE:
				theApp.m_app_state = APP_STATE_SHUTTINGDOWN;
				SendMessage(theApp.emuledlg->m_hWnd,WM_CLOSE,0,0);
				break;
			case MMT_SEARCH:
				theApp.mmserver->SearchFinished(true);
				break;
			case MMT_PREVIEW:
				theApp.mmserver->PreviewFinished(NULL,0);
				break;
		}
	}
	CATCH_DFLT_EXCEPTIONS(_T("CMMServer::CommandTimer"))
}

void  CMMServer::ProcessStatisticsRequest(CMMData* data, CMMSocket* sender){
	uint16 nWidth = data->ReadShort();
	CArray<UpDown>* rawData = theApp.webserver->GetPointsForWeb();
	int nRawDataSize = rawData->GetSize();
	int nCompressEvery = (nRawDataSize > nWidth) ? nRawDataSize / nWidth : 1;
	int nPos = (nRawDataSize > nWidth) ? (nRawDataSize % nWidth) : 0;
	int nAddUp, nAddDown, nAddCon, i;
	ASSERT (nPos + nCompressEvery * nWidth == nRawDataSize || (nPos == 0 && nRawDataSize < nWidth));
	
	CMMPacket* packet = new CMMPacket(MMP_STATISTICSANS);
	packet->WriteShort((uint16)((nRawDataSize-nPos)*thePrefs.GetTrafficOMeterInterval()));
	packet->WriteShort((uint16)(min(nWidth, nRawDataSize)));
	while (nPos < nRawDataSize){
		nAddUp = nAddDown = nAddCon = 0;
		for (i = 0; i != nCompressEvery; i++){
			if (nPos >= nRawDataSize){
				ASSERT ( false );
				break;
			}
			else{
				nAddUp += (int) (rawData->ElementAt(nPos).upload * 1024);
				nAddDown += (int) (rawData->ElementAt(nPos).download * 1024);
				nAddCon += rawData->ElementAt(nPos).connections;
			}
			nPos++;
		}
		packet->WriteInt((uint32)ROUND(nAddUp/i));
		packet->WriteInt((uint32)ROUND(nAddDown/i));
		packet->WriteShort((uint16)ROUND(nAddCon/i));
	}
	ASSERT ( nPos == nRawDataSize );
	sender->SendPacket(packet);
}

void CMMServer::WriteFileInfo(CPartFile* selFile, CMMPacket* packet){
	packet->WriteInt64( selFile->GetFileSize() );
	packet->WriteInt64( selFile->GetTransferred() );
	packet->WriteInt64( selFile->GetCompletedSize() );
	packet->WriteShort((uint16)(selFile->GetDatarate()/100));
	packet->WriteShort((uint16)(selFile->GetSourceCount()));
	packet->WriteShort((uint16)(selFile->GetTransferringSrcCount()));
	if (selFile->IsAutoDownPriority()){
		packet->WriteByte(4);
	}
	else{
		packet->WriteByte(selFile->GetDownPriority());
	}
	uint8* parts = selFile->MMCreatePartStatus();
	packet->WriteShort((uint16)selFile->GetPartCount());
	for (UINT i = 0; i < selFile->GetPartCount(); i++){
		packet->WriteByte(parts[i]);
	}
	delete[] parts;
}