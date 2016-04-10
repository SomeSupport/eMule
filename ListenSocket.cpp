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
#include "DebugHelpers.h"
#include "emule.h"
#include "ListenSocket.h"
#include "PeerCacheSocket.h"
#include "opcodes.h"
#include "UpDownClient.h"
#include "ClientList.h"
#include "OtherFunctions.h"
#include "DownloadQueue.h"
#include "Statistics.h"
#include "IPFilter.h"
#include "SharedFileList.h"
#include "PartFile.h"
#include "SafeFile.h"
#include "Packets.h"
#include "UploadQueue.h"
#include "ServerList.h"
#include "Server.h"
#include "Sockets.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "ClientListCtrl.h"
#include "ChatWnd.h"
#include "PeerCacheFinder.h"
#include "Exceptions.h"
#include "Kademlia/Utils/uint128.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/Kademlia/prefs.h"
#include "ClientUDPSocket.h"
#include "SHAHashSet.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CClientReqSocket

IMPLEMENT_DYNCREATE(CClientReqSocket, CEMSocket)

CClientReqSocket::CClientReqSocket(CUpDownClient* in_client)
{
	SetClient(in_client);
	theApp.listensocket->AddSocket(this);
	ResetTimeOutTimer();
	deletethis = false;
	deltimer = 0;
	m_bPortTestCon=false;
	m_nOnConnect=SS_Other;
}

void CClientReqSocket::SetConState( SocketState val )
{
	//If no change, do nothing..
	if( (UINT)val == m_nOnConnect )
		return;
	//Decrease count of old state..
	switch( m_nOnConnect )
	{
		case SS_Half:
			theApp.listensocket->m_nHalfOpen--;
			break;
		case SS_Complete:
			theApp.listensocket->m_nComp--;
	}
	//Set state to new state..
	m_nOnConnect = val;
	//Increase count of new state..
	switch( m_nOnConnect )
	{
		case SS_Half:
			theApp.listensocket->m_nHalfOpen++;
			break;
		case SS_Complete:
			theApp.listensocket->m_nComp++;
	}
}

void CClientReqSocket::WaitForOnConnect()
{
	SetConState(SS_Half);
}
	
CClientReqSocket::~CClientReqSocket()
{
	//This will update our statistics.
	SetConState(SS_Other);
	if (client)
		client->socket = 0;
	client = 0;
	theApp.listensocket->RemoveSocket(this);

	DEBUG_ONLY (theApp.clientlist->Debug_SocketDeleted(this));
}

void CClientReqSocket::SetClient(CUpDownClient* pClient)
{
	client = pClient;
	if (client)
		client->socket = this;
}

void CClientReqSocket::ResetTimeOutTimer(){
	timeout_timer = ::GetTickCount();
}

UINT CClientReqSocket::GetTimeOut()
{
	// PC-TODO
	// the PC socket may even already be disconnected and deleted and we still need to keep the
	// ed2k socket open because remote client may still be downloading from cache.
	if (client && client->IsUploadingToPeerCache() && (client->m_pPCUpSocket == NULL || !client->m_pPCUpSocket->IsConnected()))
	{
		// we are uploading (or at least allow uploading) but currently no socket
		return max(CEMSocket::GetTimeOut(), GetPeerCacheSocketUploadTimeout());
	}
	else if (client && client->m_pPCUpSocket && client->m_pPCUpSocket->IsConnected())
	{
		// we have an uploading PC socket, but that socket is not used (nor can it be closed)
		return max(CEMSocket::GetTimeOut(), client->m_pPCUpSocket->GetTimeOut());
	}
	else if (client && client->m_pPCDownSocket && client->m_pPCDownSocket->IsConnected())
	{
		// we have a downloading PC socket
		return max(CEMSocket::GetTimeOut(), client->m_pPCDownSocket->GetTimeOut());
	}
	else
		return CEMSocket::GetTimeOut();
}

bool CClientReqSocket::CheckTimeOut()
{
	if(m_nOnConnect == SS_Half)
	{
		//This socket is still in a half connection state.. Because of SP2, we don't know
		//if this socket is actually failing, or if this socket is just queued in SP2's new
		//protection queue. Therefore we give the socket a chance to either finally report
		//the connection error, or finally make it through SP2's new queued socket system..
		if (::GetTickCount() - timeout_timer > CEMSocket::GetTimeOut()*4){
			timeout_timer = ::GetTickCount();
			CString str;
			str.Format(_T("Timeout: State:%u = SS_Half"), m_nOnConnect);
			Disconnect(str);
			return true;
		}
		return false;
	}
	UINT uTimeout = GetTimeOut();
	if(client)
	{
		if (client->GetKadState() == KS_CONNECTED_BUDDY)
		{
			uTimeout += MIN2MS(15);
		}
		if (client->GetChatState()!=MS_NONE)
		{
			//We extend the timeout time here to avoid people chatting from disconnecting to fast.
			uTimeout += CONNECTION_TIMEOUT;
		}
	}
	if (::GetTickCount() - timeout_timer > uTimeout){
		timeout_timer = ::GetTickCount();
		CString str;
		str.Format(_T("Timeout: State:%u (0 = SS_Other, 1 = SS_Half, 2 = SS_Complete"), m_nOnConnect);
		Disconnect(str);
		return true;
	}
	return false;
}

void CClientReqSocket::OnClose(int nErrorCode){
	ASSERT (theApp.listensocket->IsValidSocket(this));
	CEMSocket::OnClose(nErrorCode);

	LPCTSTR pszReason;
	CString* pstrReason = NULL;
	if (nErrorCode == 0)
		pszReason = _T("Close");
	else if (thePrefs.GetVerbose()){
		pstrReason = new CString;
		*pstrReason = GetErrorMessage(nErrorCode, 1);
		pszReason = *pstrReason;
	}
	else
		pszReason = NULL;
	Disconnect(pszReason);
	delete pstrReason;
}

void CClientReqSocket::Disconnect(LPCTSTR pszReason){
	AsyncSelect(0);
	byConnected = ES_DISCONNECTED;
	if (!client)
		Safe_Delete();
	else
        if(client->Disconnected(CString(_T("CClientReqSocket::Disconnect(): ")) + pszReason, true)){
			CUpDownClient* temp = client;
			client->socket = NULL;
			client = NULL;
			delete temp;
			Safe_Delete();
		}
		else{
			client = NULL;
			Safe_Delete();
		}
};

void CClientReqSocket::Delete_Timed(){
// it seems that MFC Sockets call socketfunctions after they are deleted, even if the socket is closed
// and select(0) is set. So we need to wait some time to make sure this doesn't happens
// we currently also trust on this for multithreading, rework snychronization if this ever changes
	if (::GetTickCount() - deltimer > 10000)
		delete this;
}

void CClientReqSocket::Safe_Delete()
{
	ASSERT (theApp.listensocket->IsValidSocket(this));
	AsyncSelect(0);
	deltimer = ::GetTickCount();
	if (m_SocketData.hSocket != INVALID_SOCKET) // deadlake PROXYSUPPORT - changed to AsyncSocketEx
		ShutDown(SD_BOTH);
	if (client)
		client->socket = 0;
	client = 0;
	byConnected = ES_DISCONNECTED;
	deletethis = true;
}

bool CClientReqSocket::ProcessPacket(const BYTE* packet, uint32 size, UINT opcode)
{
	try
	{
		try
		{
			if (!client && opcode != OP_HELLO)
			{
				theStats.AddDownDataOverheadOther(size);
				throw GetResString(IDS_ERR_NOHELLO);
			}
			else if (client && opcode != OP_HELLO && opcode != OP_HELLOANSWER)
				client->CheckHandshakeFinished();
			switch(opcode)
			{
				case OP_HELLOANSWER:
				{
					theStats.AddDownDataOverheadOther(size);
					client->ProcessHelloAnswer(packet,size);
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_HelloAnswer", client);
						Debug(_T("  %s\n"), client->DbgGetHelloInfo());
					}

					// start secure identification, if
					//  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
					//	- we have received eMule-OP_HELLOANSWER (new eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();

					if (client)
					{
						client->ConnectionEstablished();
						theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(client);
					}
					break;
				}
				case OP_HELLO:
				{
					theStats.AddDownDataOverheadOther(size);

					bool bNewClient = !client;
					if (bNewClient)
					{
						// create new client to save standart informations
						client = new CUpDownClient(this);
					}

					bool bIsMuleHello = false;
					try
					{
						bIsMuleHello = client->ProcessHelloPacket(packet,size);
					}
					catch(...)
					{
						if (bNewClient)
						{
							// Don't let CUpDownClient::Disconnected be processed for a client which is not in the list of clients.
							delete client;
							client = NULL;
						}
						throw;
					}

					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_Hello", client);
						Debug(_T("  %s\n"), client->DbgGetHelloInfo());
					}

					// now we check if we know this client already. if yes this socket will
					// be attached to the known client, the new client will be deleted
					// and the var. "client" will point to the known client.
					// if not we keep our new-constructed client ;)
					if (theApp.clientlist->AttachToAlreadyKnown(&client,this))
					{
						// update the old client informations
						bIsMuleHello = client->ProcessHelloPacket(packet,size);
					}
					else 
					{
						theApp.clientlist->AddClient(client);
						client->SetCommentDirty();
					}

					theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(client);

					// send a response packet with standart informations
					if (client->GetHashType() == SO_EMULE && !bIsMuleHello)
						client->SendMuleInfoPacket(false);

					client->SendHelloAnswer();

					if (client)
						client->ConnectionEstablished();

					ASSERT( client );
					if(client)
					{
						// start secure identification, if
						//	- we have received eMule-OP_HELLO (new eMule)
						if (client->GetInfoPacketsReceived() == IP_BOTH)
							client->InfoPacketsReceived();

						if( client->GetKadPort() && client->GetKadVersion() > 1)
							Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort());
					}
					break;
				}
				case OP_REQUESTFILENAME:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileRequest", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					if (size >= 16)
					{
						if (!client->GetWaitStartTime())
							client->SetWaitStartTime();

						CSafeMemFile data_in(packet, size);
						uchar reqfilehash[16];
						data_in.ReadHash16(reqfilehash);

						CKnownFile* reqfile;
						if ( (reqfile = theApp.sharedfiles->GetFileByID(reqfilehash)) == NULL ){
							if ( !((reqfile = theApp.downloadqueue->GetFileByID(reqfilehash)) != NULL
								   && reqfile->GetFileSize() > (uint64)PARTSIZE) )
							{
								client->CheckFailedFileIdReqs(reqfilehash);
								break;
							}
						}

						if (reqfile->IsLargeFile() && !client->SupportsLargeFiles()){
							DebugLogWarning(_T("Client without 64bit file support requested large file; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());
							break;
						}

						// check to see if this is a new file they are asking for
						if (md4cmp(client->GetUploadFileID(), reqfilehash) != 0)
							client->SetCommentDirty();
						client->SetUploadFileID(reqfile);

						if (!client->ProcessExtendedInfo(&data_in, reqfile)){
							if (thePrefs.GetDebugClientTCPLevel() > 0)
								DebugSend("OP__FileReqAnsNoFil", client, packet);
							Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
							md4cpy(replypacket->pBuffer, reqfile->GetFileHash());
							theStats.AddUpDataOverheadFileRequest(replypacket->size);
							SendPacket(replypacket, true);
							DebugLogWarning(_T("Partcount mismatch on requested file, sending FNF; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());
							break;
						}

						// if we are downloading this file, this could be a new source
						// no passive adding of files with only one part
						if (reqfile->IsPartFile() && reqfile->GetFileSize() > (uint64)PARTSIZE)
						{
							if (((CPartFile*)reqfile)->GetMaxSources() > ((CPartFile*)reqfile)->GetSourceCount())
								theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, client, true);
						}

						// send filename etc
						CSafeMemFile data_out(128);
						data_out.WriteHash16(reqfile->GetFileHash());
						data_out.WriteString(reqfile->GetFileName(), client->GetUnicodeSupport());
						Packet* packet = new Packet(&data_out);
						packet->opcode = OP_REQFILENAMEANSWER;
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__FileReqAnswer", client, reqfile->GetFileHash());
						theStats.AddUpDataOverheadFileRequest(packet->size);
						SendPacket(packet, true);
						
						client->SendCommentInfo(reqfile);
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
				}
				case OP_SETREQFILEID:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_SetReqFileID", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					if (size == 16)
					{
						if (!client->GetWaitStartTime())
							client->SetWaitStartTime();

						CKnownFile* reqfile;
						if ( (reqfile = theApp.sharedfiles->GetFileByID(packet)) == NULL ){
							if ( !((reqfile = theApp.downloadqueue->GetFileByID(packet)) != NULL
								   && reqfile->GetFileSize() > (uint64)PARTSIZE) )
							{
								// send file request no such file packet (0x48)
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugSend("OP__FileReqAnsNoFil", client, packet);
								Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
								md4cpy(replypacket->pBuffer, packet);
								theStats.AddUpDataOverheadFileRequest(replypacket->size);
								SendPacket(replypacket, true);
								client->CheckFailedFileIdReqs(packet);
								break;
							}
						}
						if (reqfile->IsLargeFile() && !client->SupportsLargeFiles()){
							if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugSend("OP__FileReqAnsNoFil", client, packet);
							Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
							md4cpy(replypacket->pBuffer, packet);
							theStats.AddUpDataOverheadFileRequest(replypacket->size);
							SendPacket(replypacket, true);
							DebugLogWarning(_T("Client without 64bit file support requested large file; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());
							break;
						}

						// check to see if this is a new file they are asking for
						if (md4cmp(client->GetUploadFileID(), packet) != 0)
							client->SetCommentDirty();

						client->SetUploadFileID(reqfile);

						// send filestatus
						CSafeMemFile data(16+16);
						data.WriteHash16(reqfile->GetFileHash());
						if (reqfile->IsPartFile())
							((CPartFile*)reqfile)->WritePartStatus(&data);
						else
							data.WriteUInt16(0);
						Packet* packet = new Packet(&data);
						packet->opcode = OP_FILESTATUS;
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__FileStatus", client, reqfile->GetFileHash());
						theStats.AddUpDataOverheadFileRequest(packet->size);
						SendPacket(packet, true);
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
				}
				case OP_FILEREQANSNOFIL:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileReqAnsNoFil", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);
					if (size == 16)
					{
						CPartFile* reqfile = theApp.downloadqueue->GetFileByID(packet);
						if (!reqfile){
							client->CheckFailedFileIdReqs(packet);
							break;
						}
						else
							reqfile->m_DeadSourceList.AddDeadSource(client);
						// if that client does not have my file maybe has another different
						// we try to swap to another file ignoring no needed parts files
						switch (client->GetDownloadState())
						{
							case DS_CONNECTED:
							case DS_ONQUEUE:
							case DS_NONEEDEDPARTS:
                                client->DontSwapTo(client->GetRequestFile()); // ZZ:DownloadManager
                                if (!client->SwapToAnotherFile(_T("Source says it doesn't have the file. CClientReqSocket::ProcessPacket()"), true, true, true, NULL, false, false)) { // ZZ:DownloadManager
    								theApp.downloadqueue->RemoveSource(client);
                                }
							break;
						}
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
				}
				case OP_REQFILENAMEANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileReqAnswer", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					CSafeMemFile data(packet, size);
					uchar cfilehash[16];
					data.ReadHash16(cfilehash);
					CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
					if (file == NULL)
						client->CheckFailedFileIdReqs(cfilehash);
					client->ProcessFileInfo(&data, file);
					break;
				}
				case OP_FILESTATUS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileStatus", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					CSafeMemFile data(packet, size);
					uchar cfilehash[16];
					data.ReadHash16(cfilehash);
					CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
					if (file == NULL)
						client->CheckFailedFileIdReqs(cfilehash);
					client->ProcessFileStatus(false, &data, file);
					break;
				}
				case OP_STARTUPLOADREQ:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_StartUpLoadReq", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);
				
					if (!client->CheckHandshakeFinished())
						break;
					if (size == 16)
					{
						CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(packet);
						if (reqfile)
						{
							if (md4cmp(client->GetUploadFileID(), packet) != 0)
								client->SetCommentDirty();
							client->SetUploadFileID(reqfile);
							client->SendCommentInfo(reqfile);
							theApp.uploadqueue->AddClientToQueue(client);
						}
						else
							client->CheckFailedFileIdReqs(packet);
					}
					break;
				}
				case OP_QUEUERANK:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_QueueRank", client);
					theStats.AddDownDataOverheadFileRequest(size);
					client->ProcessEdonkeyQueueRank(packet, size);
					break;
				}
				case OP_ACCEPTUPLOADREQ:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_AcceptUploadReq", client, (size >= 16) ? packet : NULL);
						if (size > 0)
							Debug(_T("  ***NOTE: Packet contains %u additional bytes\n"), size);
						Debug(_T("  QR=%d\n"), client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)client->GetRemoteQueueRank());
					}
					theStats.AddDownDataOverheadFileRequest(size);
					client->ProcessAcceptUpload();
					break;
				}
				case OP_REQUESTPARTS:
				{
					// see also OP_REQUESTPARTS_I64
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_RequestParts", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					CSafeMemFile data(packet, size);
					uchar reqfilehash[16];
					data.ReadHash16(reqfilehash);

					uint32 auStartOffsets[3];
					auStartOffsets[0] = data.ReadUInt32();
					auStartOffsets[1] = data.ReadUInt32();
					auStartOffsets[2] = data.ReadUInt32();

					uint32 auEndOffsets[3];
					auEndOffsets[0] = data.ReadUInt32();
					auEndOffsets[1] = data.ReadUInt32();
					auEndOffsets[2] = data.ReadUInt32();

					if (thePrefs.GetDebugClientTCPLevel() > 0){
						Debug(_T("  Start1=%u  End1=%u  Size=%u\n"), auStartOffsets[0], auEndOffsets[0], auEndOffsets[0] - auStartOffsets[0]);
						Debug(_T("  Start2=%u  End2=%u  Size=%u\n"), auStartOffsets[1], auEndOffsets[1], auEndOffsets[1] - auStartOffsets[1]);
						Debug(_T("  Start3=%u  End3=%u  Size=%u\n"), auStartOffsets[2], auEndOffsets[2], auEndOffsets[2] - auStartOffsets[2]);
					}

					for (int i = 0; i < ARRSIZE(auStartOffsets); i++)
					{
						if (auEndOffsets[i] > auStartOffsets[i])
						{
							Requested_Block_Struct* reqblock = new Requested_Block_Struct;
							reqblock->StartOffset = auStartOffsets[i];
							reqblock->EndOffset = auEndOffsets[i];
							md4cpy(reqblock->FileID, reqfilehash);
							reqblock->transferred = 0;
							client->AddReqBlock(reqblock, false);
						}
						else
						{
							if (thePrefs.GetVerbose())
							{
								if (auEndOffsets[i] != 0 || auStartOffsets[i] != 0)
									DebugLogWarning(_T("Client requests invalid %u. file block %u-%u (%d bytes): %s"), i, auStartOffsets[i], auEndOffsets[i], auEndOffsets[i] - auStartOffsets[i], client->DbgGetClientInfo());
							}
						}
					}
					client->AddReqBlock(NULL, true);
					break;
				}
				case OP_CANCELTRANSFER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_CancelTransfer", client);
					theStats.AddDownDataOverheadFileRequest(size);
					theApp.uploadqueue->RemoveFromUploadQueue(client, _T("Remote client cancelled transfer."));
					break;
				}
				case OP_END_OF_DOWNLOAD:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_EndOfDownload", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);
					if (size>=16 && !md4cmp(client->GetUploadFileID(),packet))
						theApp.uploadqueue->RemoveFromUploadQueue(client, _T("Remote client ended transfer."));
					else
						client->CheckFailedFileIdReqs(packet);
					break;
				}
				case OP_HASHSETREQUEST:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_HashSetReq", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					if (size != 16)
						throw GetResString(IDS_ERR_WRONGHPACKAGESIZE);
					client->SendHashsetPacket(packet, 16, false);
					break;
				}
				case OP_HASHSETANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_HashSetAnswer", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);
					client->ProcessHashSet(packet, size, false);
					break;
				}
				case OP_SENDINGPART:
				{
					// see also OP_SENDINGPART_I64
					if (thePrefs.GetDebugClientTCPLevel() > 1)
						DebugRecv("OP_SendingPart", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(24);
					if (client->GetRequestFile() && !client->GetRequestFile()->IsStopped() && (client->GetRequestFile()->GetStatus()==PS_READY || client->GetRequestFile()->GetStatus()==PS_EMPTY))
					{
						client->ProcessBlockPacket(packet, size, false, false);
						if (client->GetRequestFile())
						{
							if (client->GetRequestFile()->IsStopped() || client->GetRequestFile()->GetStatus()==PS_PAUSED || client->GetRequestFile()->GetStatus()==PS_ERROR)
							{
								client->SendCancelTransfer();
								client->SetDownloadState(client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);
							}
						}
						else
							ASSERT( false );
					}
					else
					{
						client->SendCancelTransfer();
						client->SetDownloadState((client->GetRequestFile()==NULL || client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
					}
					break;
				}
				case OP_OUTOFPARTREQS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_OutOfPartReqs", client);
					theStats.AddDownDataOverheadFileRequest(size);
					if (client->GetDownloadState() == DS_DOWNLOADING)
					{
						client->SetDownloadState(DS_ONQUEUE, _T("The remote client decided to stop/complete the transfer (got OP_OutOfPartReqs)."));
					}
					break;
				}
				case OP_CHANGE_CLIENT_ID:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_ChangedClientID", client);
					theStats.AddDownDataOverheadOther(size);

					CSafeMemFile data(packet, size);
					uint32 nNewUserID = data.ReadUInt32();
					uint32 nNewServerIP = data.ReadUInt32();
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						Debug(_T("  NewUserID=%u (%08x, %s)  NewServerIP=%u (%08x, %s)\n"), nNewUserID, nNewUserID, ipstr(nNewUserID), nNewServerIP, nNewServerIP, ipstr(nNewServerIP));
					if (IsLowID(nNewUserID))
					{	// client changed server and has a LowID
						CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
						if (pNewServer != NULL)
						{
							client->SetUserIDHybrid(nNewUserID); // update UserID only if we know the server
							client->SetServerIP(nNewServerIP);
							client->SetServerPort(pNewServer->GetPort());
						}
					}
					else if (nNewUserID == client->GetIP())
					{	// client changed server and has a HighID(IP)
						client->SetUserIDHybrid(ntohl(nNewUserID));
						CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
						if (pNewServer != NULL)
						{
							client->SetServerIP(nNewServerIP);
							client->SetServerPort(pNewServer->GetPort());
						}
					}
					else{
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							Debug(_T("***NOTE: OP_ChangedClientID unknown contents\n"));
					}
					UINT uAddData = (UINT)(data.GetLength() - data.GetPosition());
					if (uAddData > 0){
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							Debug(_T("***NOTE: OP_ChangedClientID contains add. data %s\n"), DbgGetHexDump(packet + data.GetPosition(), uAddData));
					}
					break;
				}
				case OP_CHANGE_SLOT:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_ChangeSlot", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					// sometimes sent by Hybrid
					break;
				}
				case OP_MESSAGE:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_Message", client);
					theStats.AddDownDataOverheadOther(size);
					
					if (size < 2)
						throw CString(_T("invalid message packet"));
					CSafeMemFile data(packet, size);
					UINT length = data.ReadUInt16();
					if (length+2 != size)
						throw CString(_T("invalid message packet"));

					if (length > MAX_CLIENT_MSG_LEN){
						if (thePrefs.GetVerbose())
							AddDebugLogLine(false, _T("Message from '%s' (IP:%s) exceeds limit by %u chars, truncated."), client->GetUserName(), ipstr(client->GetConnectIP()), length - MAX_CLIENT_MSG_LEN);
						length = MAX_CLIENT_MSG_LEN;
					}					
					
					client->ProcessChatMessage(&data, length);
					break;
				}
				case OP_ASKSHAREDFILES:
				{	
					// client wants to know what we have in share, let's see if we allow him to know that
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFiles", client);
					theStats.AddDownDataOverheadOther(size);


					CPtrList list;
					if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))
					{
						CCKey bufKey;
						CKnownFile* cur_file;
						for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition();pos != 0;)
						{
							theApp.sharedfiles->m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
							if (!cur_file->IsLargeFile() || client->SupportsLargeFiles())
								list.AddTail((void*&)cur_file);
						}
						AddLogLine(true, GetResString(IDS_REQ_SHAREDFILES), client->GetUserName(), client->GetUserIDHybrid(), GetResString(IDS_ACCEPTED));
					}
					else
					{
						DebugLog(GetResString(IDS_REQ_SHAREDFILES), client->GetUserName(), client->GetUserIDHybrid(), GetResString(IDS_DENIED));
					}

					// now create the memfile for the packet
					uint32 iTotalCount = list.GetCount();
					CSafeMemFile tempfile(80);
					tempfile.WriteUInt32(iTotalCount);
					while (list.GetCount())
					{
						theApp.sharedfiles->CreateOfferedFilePacket((CKnownFile*)list.GetHead(), &tempfile, NULL, client);
						list.RemoveHead();
					}

					// create a packet and send it
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugSend("OP__AskSharedFilesAnswer", client);
					Packet* replypacket = new Packet(&tempfile);
					replypacket->opcode = OP_ASKSHAREDFILESANSWER;
					theStats.AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket, true, true);
					break;
				}
				case OP_ASKSHAREDFILESANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFilesAnswer", client);
					theStats.AddDownDataOverheadOther(size);
					client->ProcessSharedFileList(packet,size);
					break;
				}
                case OP_ASKSHAREDDIRS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedDirectories", client);
                    theStats.AddDownDataOverheadOther(size);

                    if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))
					{
						AddLogLine(true, GetResString(IDS_SHAREDREQ1), client->GetUserName(), client->GetUserIDHybrid(), GetResString(IDS_ACCEPTED));
						client->SendSharedDirectories();
					}
					else
					{
						DebugLog(GetResString(IDS_SHAREDREQ1), client->GetUserName(), client->GetUserIDHybrid(), GetResString(IDS_DENIED));
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AskSharedDeniedAnswer", client);
                        Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
						theStats.AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
                    }
                    break;
                }
                case OP_ASKSHAREDFILESDIR:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFilesInDirectory", client);
                    theStats.AddDownDataOverheadOther(size);

					CSafeMemFile data(packet, size);
                    CString strReqDir = data.ReadString(client->GetUnicodeSupport()!=utf8strNone);
					CString strOrgReqDir = strReqDir;
                    if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))
					{
						AddLogLine(true, GetResString(IDS_SHAREDREQ2), client->GetUserName(), client->GetUserIDHybrid(), strReqDir, GetResString(IDS_ACCEPTED));
                        ASSERT( data.GetPosition() == data.GetLength() );
                        CTypedPtrList<CPtrList, CKnownFile*> list;
						if (strReqDir == OP_INCOMPLETE_SHARED_FILES)
						{
							// get all shared files from download queue
							int iQueuedFiles = theApp.downloadqueue->GetFileCount();
							for (int i = 0; i < iQueuedFiles; i++)
							{
								CPartFile* pFile = theApp.downloadqueue->GetFileByIndex(i);
								if (pFile == NULL || pFile->GetStatus(true) != PS_READY || (pFile->IsLargeFile() && !client->SupportsLargeFiles()))
									continue;
								list.AddTail(pFile);
							}
						}
						else
						{
							bool bSingleSharedFiles = strReqDir == OP_OTHER_SHARED_FILES;
							if (!bSingleSharedFiles)
								strReqDir = theApp.sharedfiles->GetDirNameByPseudo(strReqDir);
							if (!strReqDir.IsEmpty())
							{
								// get all shared files from requested directory
								for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition();pos != 0;)
								{
									CCKey bufKey;
									CKnownFile* cur_file;
									theApp.sharedfiles->m_Files_map.GetNextAssoc(pos, bufKey, cur_file);

									// all files which are not within a shared directory have to be single shared files
									if (((!bSingleSharedFiles && CompareDirectories(strReqDir, cur_file->GetSharedDirectory()) == 0) || (bSingleSharedFiles && !theApp.sharedfiles->ShouldBeShared(cur_file->GetSharedDirectory(), _T(""), false)))
										&& (!cur_file->IsLargeFile() || client->SupportsLargeFiles()))
									{
										list.AddTail(cur_file);
									}

								}
							}
							else
								DebugLogError(_T("View shared files: Pseudonym for requested Directory (%s) was not found - sending empty result"), strOrgReqDir);	
						}

						// Currently we are sending each shared directory, even if it does not contain any files.
						// Because of this we also have to send an empty shared files list..
						CSafeMemFile tempfile(80);
						tempfile.WriteString(strOrgReqDir, client->GetUnicodeSupport());
						tempfile.WriteUInt32(list.GetCount());
						while (list.GetCount())
						{
							theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile, NULL, client);
							list.RemoveHead();
						}

						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AskSharedFilesInDirectoryAnswer", client);
						Packet* replypacket = new Packet(&tempfile);
						replypacket->opcode = OP_ASKSHAREDFILESDIRANS;
						theStats.AddUpDataOverheadOther(replypacket->size);
						SendPacket(replypacket, true, true);
					}
                    else
					{
						DebugLog(GetResString(IDS_SHAREDREQ2), client->GetUserName(), client->GetUserIDHybrid(), strReqDir, GetResString(IDS_DENIED));
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AskSharedDeniedAnswer", client);
                        Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
                        theStats.AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
                    }
                    break;
                }
				case OP_ASKSHAREDDIRSANS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedDirectoriesAnswer", client);
                    theStats.AddDownDataOverheadOther(size);
                    if (client->GetFileListRequested() == 1)
					{
						CSafeMemFile data(packet, size);
						UINT uDirs = data.ReadUInt32();
                        for (UINT i = 0; i < uDirs; i++)
						{
                            CString strDir = data.ReadString(client->GetUnicodeSupport()!=utf8strNone);
							// Better send the received and untouched directory string back to that client
							//PathRemoveBackslash(strDir.GetBuffer());
							//strDir.ReleaseBuffer();
							AddLogLine(true, GetResString(IDS_SHAREDANSW), client->GetUserName(), client->GetUserIDHybrid(), strDir);

							if (thePrefs.GetDebugClientTCPLevel() > 0)
								DebugSend("OP__AskSharedFilesInDirectory", client);
                            CSafeMemFile tempfile(80);
							tempfile.WriteString(strDir, client->GetUnicodeSupport());
                            Packet* replypacket = new Packet(&tempfile);
                            replypacket->opcode = OP_ASKSHAREDFILESDIR;
                            theStats.AddUpDataOverheadOther(replypacket->size);
                            SendPacket(replypacket, true, true);
                        }
                        ASSERT( data.GetPosition() == data.GetLength() );
                        client->SetFileListRequested(uDirs);
                    }
					else
						AddLogLine(true, GetResString(IDS_SHAREDANSW2), client->GetUserName(), client->GetUserIDHybrid());
                    break;
                }
                case OP_ASKSHAREDFILESDIRANS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFilesInDirectoryAnswer", client);
                    theStats.AddDownDataOverheadOther(size);

					CSafeMemFile data(packet, size);
                    CString strDir = data.ReadString(client->GetUnicodeSupport()!=utf8strNone);
					PathRemoveBackslash(strDir.GetBuffer());
					strDir.ReleaseBuffer();
                    if (client->GetFileListRequested() > 0)
					{
						AddLogLine(true, GetResString(IDS_SHAREDINFO1), client->GetUserName(), client->GetUserIDHybrid(), strDir);
						client->ProcessSharedFileList(packet + (UINT)data.GetPosition(), (UINT)(size - data.GetPosition()), strDir);
						if (client->GetFileListRequested() == 0)
							AddLogLine(true, GetResString(IDS_SHAREDINFO2), client->GetUserName(), client->GetUserIDHybrid());
                    }
					else
						AddLogLine(true, GetResString(IDS_SHAREDANSW3), client->GetUserName(), client->GetUserIDHybrid(), strDir);
                    break;
                }
                case OP_ASKSHAREDDENIEDANS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedDeniedAnswer", client);
                    theStats.AddDownDataOverheadOther(size);

					AddLogLine(true, GetResString(IDS_SHAREDREQDENIED), client->GetUserName(), client->GetUserIDHybrid());
					client->SetFileListRequested(0);
                    break;
                }
				default:
					theStats.AddDownDataOverheadOther(size);
					PacketToDebugLogLine(_T("eDonkey"), packet, size, opcode);
					break;
			}
		}
		catch(CFileException* error)
		{
			error->Delete();
			throw GetResString(IDS_ERR_INVALIDPACKAGE);
		}
		catch(CMemoryException* error)
		{
			error->Delete();
			throw CString(_T("Memory exception"));
		}
	}
	catch(CClientException* ex) // nearly same as the 'CString' exception but with optional deleting of the client
	{
		if (thePrefs.GetVerbose() && !ex->m_strMsg.IsEmpty())
			DebugLogWarning(_T("Error: %s - while processing eDonkey packet: opcode=%s  size=%u; %s"), ex->m_strMsg, DbgGetDonkeyClientTCPOpcode(opcode), size, DbgGetClientInfo());
		if (client && ex->m_bDelete)
			client->SetDownloadState(DS_ERROR, _T("Error while processing eDonkey packet (CClientException): ") + ex->m_strMsg);
		Disconnect(ex->m_strMsg);
		ex->Delete();
		return false;
	}
	catch(CString error)
	{
		if (thePrefs.GetVerbose() && !error.IsEmpty())
			DebugLogWarning(_T("Error: %s - while processing eDonkey packet: opcode=%s  size=%u; %s"), error, DbgGetDonkeyClientTCPOpcode(opcode), size, DbgGetClientInfo());
		if (client)
			client->SetDownloadState(DS_ERROR, _T("Error while processing eDonkey packet (CString exception): ") + error);	
		Disconnect(_T("Error when processing packet.") + error);
		return false;
	}
	return true;
}

bool CClientReqSocket::ProcessExtPacket(const BYTE* packet, uint32 size, UINT opcode, UINT uRawSize)
{
	try
	{
		try
		{
			if (!client && opcode!=OP_PORTTEST)
			{
				theStats.AddDownDataOverheadOther(uRawSize);
				throw GetResString(IDS_ERR_UNKNOWNCLIENTACTION);
			}
			if (thePrefs.m_iDbgHeap >= 2 && opcode!=OP_PORTTEST)
				ASSERT_VALID(client);
			switch(opcode)
			{
                case OP_MULTIPACKET: // deprecated
				case OP_MULTIPACKET_EXT: // deprecated
				case OP_MULTIPACKET_EXT2:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						if (opcode == OP_MULTIPACKET_EXT)
							DebugRecv("OP_MultiPacket_Ext", client, (size >= 24) ? packet : NULL);
						else
							DebugRecv("OP_MultiPacket", client, (size >= 16) ? packet : NULL);
					}
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->CheckHandshakeFinished();

					if( client->GetKadPort() && client->GetKadVersion() > 1)
						Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort());

					CSafeMemFile data_in(packet, size);
					uint64 nSize = 0;
					CKnownFile* reqfile;
					bool bNotFound = false;
					uchar reqfilehash[16];
					if (opcode == OP_MULTIPACKET_EXT2) // fileidentifier support
					{
						CFileIdentifierSA fileIdent;
						if (!fileIdent.ReadIdentifier(&data_in))
						{
							DebugLogWarning(_T("Error while reading file identifier from MultiPacket_Ext2 - %s"), client->DbgGetClientInfo());
							break;
						}
						md4cpy(reqfilehash, fileIdent.GetMD4Hash()); // need this in case we want to sent a FNF
						if ( (reqfile = theApp.sharedfiles->GetFileByID(fileIdent.GetMD4Hash())) == NULL ){
							if ( !((reqfile = theApp.downloadqueue->GetFileByID(fileIdent.GetMD4Hash())) != NULL
									&& reqfile->GetFileSize() > (uint64)PARTSIZE) )
							{
								bNotFound = true;
								client->CheckFailedFileIdReqs(fileIdent.GetMD4Hash());
							}
						}
						if (!bNotFound && !reqfile->GetFileIdentifier().CompareRelaxed(fileIdent)){
							bNotFound = true;
							DebugLogWarning(_T("FileIdentifier Mismatch on requested file, sending FNF; %s, File=\"%s\", Local Ident: %s, Received Ident: %s"), client->DbgGetClientInfo()
								, reqfile->GetFileName() , reqfile->GetFileIdentifier().DbgInfo(), fileIdent.DbgInfo());
						}
					}
					else // no fileidentifier
					{
						data_in.ReadHash16(reqfilehash);
						if (opcode == OP_MULTIPACKET_EXT){
							nSize = data_in.ReadUInt64();
						}
						if ( (reqfile = theApp.sharedfiles->GetFileByID(reqfilehash)) == NULL ){
							if ( !((reqfile = theApp.downloadqueue->GetFileByID(reqfilehash)) != NULL
									&& reqfile->GetFileSize() > (uint64)PARTSIZE) )
							{
								bNotFound = true;
								client->CheckFailedFileIdReqs(reqfilehash);
							}
						}
						if (!bNotFound && nSize != 0 && nSize != reqfile->GetFileSize()){
							bNotFound = true;
							DebugLogWarning(_T("Size Mismatch on requested file, sending FNF; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());
						}
					}

					if (!bNotFound && reqfile->IsLargeFile() && !client->SupportsLargeFiles()){
						bNotFound = true;
						DebugLogWarning(_T("Client without 64bit file support requested large file; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());
					}
					if (bNotFound)
					{
						// send file request no such file packet (0x48)
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__FileReqAnsNoFil", client, packet);
						Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
						md4cpy(replypacket->pBuffer, reqfilehash);
						theStats.AddUpDataOverheadFileRequest(replypacket->size);
						SendPacket(replypacket, true);
						break;
					}

					if (!client->GetWaitStartTime())
						client->SetWaitStartTime();

					// if we are downloading this file, this could be a new source
					// no passive adding of files with only one part
					if (reqfile->IsPartFile() && reqfile->GetFileSize() > (uint64)PARTSIZE)
					{
						if (((CPartFile*)reqfile)->GetMaxSources() > ((CPartFile*)reqfile)->GetSourceCount())
							theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, client, true);
					}

					// check to see if this is a new file they are asking for
					if (md4cmp(client->GetUploadFileID(), reqfile->GetFileHash()) != 0)
						client->SetCommentDirty();

					client->SetUploadFileID(reqfile);

					uint8 opcode_in;
					CSafeMemFile data_out(128);
					if (opcode == OP_MULTIPACKET_EXT2) // fileidentifier support
						reqfile->GetFileIdentifierC().WriteIdentifier(&data_out);
					else
						data_out.WriteHash16(reqfile->GetFileHash());
					bool bAnswerFNF = false;
					while (data_in.GetLength()-data_in.GetPosition() && !bAnswerFNF)
					{
						opcode_in = data_in.ReadUInt8();
						switch (opcode_in)
						{
							case OP_REQUESTFILENAME:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPReqFileName", client, packet);

								if (!client->ProcessExtendedInfo(&data_in, reqfile)){
									if (thePrefs.GetDebugClientTCPLevel() > 0)
												DebugSend("OP__FileReqAnsNoFil", client, packet);
										Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
										md4cpy(replypacket->pBuffer, reqfile->GetFileHash());
										theStats.AddUpDataOverheadFileRequest(replypacket->size);
										SendPacket(replypacket, true);
										DebugLogWarning(_T("Partcount mismatch on requested file, sending FNF; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());
										bAnswerFNF = true;
										break;
								}
								data_out.WriteUInt8(OP_REQFILENAMEANSWER);
								data_out.WriteString(reqfile->GetFileName(), client->GetUnicodeSupport());
								break;
							}
							case OP_AICHFILEHASHREQ:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPAichFileHashReq", client, packet);

								if (client->SupportsFileIdentifiers() || opcode == OP_MULTIPACKET_EXT2)
								{// not allowed anymore with fileidents supported
									DebugLogWarning(_T("Client requested AICH Hash packet, but supports FileIdentifiers, ignored - %s"), client->DbgGetClientInfo());
									break;
								}
								if (client->IsSupportingAICH() && reqfile->GetFileIdentifier().HasAICHHash())
								{
									data_out.WriteUInt8(OP_AICHFILEHASHANS);
									reqfile->GetFileIdentifier().GetAICHHash().Write(&data_out);
								}
								break;
							}
							case OP_SETREQFILEID:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPSetReqFileID", client, packet);

								data_out.WriteUInt8(OP_FILESTATUS);
								if (reqfile->IsPartFile())
									((CPartFile*)reqfile)->WritePartStatus(&data_out);
								else
									data_out.WriteUInt16(0);
								break;
							}
							//We still send the source packet seperately.. 
							case OP_REQUESTSOURCES2:
							case OP_REQUESTSOURCES:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv(opcode_in == OP_REQUESTSOURCES2 ? "OP_MPReqSources2" : "OP_MPReqSources", client, packet);

								if (thePrefs.GetDebugSourceExchange())
									AddDebugLogLine(false, _T("SXRecv: Client source request; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());
								
								uint8 byRequestedVersion = 0;
								uint16 byRequestedOptions = 0;
								if (opcode_in == OP_REQUESTSOURCES2){ // SX2 requests contains additional data
									byRequestedVersion = data_in.ReadUInt8();
									byRequestedOptions = data_in.ReadUInt16();
								}
								//Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.
								if (byRequestedVersion > 0 || client->GetSourceExchange1Version() > 1)
								{
									DWORD dwTimePassed = ::GetTickCount() - client->GetLastSrcReqTime() + CONNECTION_LATENCY;
									bool bNeverAskedBefore = client->GetLastSrcReqTime() == 0;
									if (
										//if not complete and file is rare
										(    reqfile->IsPartFile()
										  && (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
										  && ((CPartFile*)reqfile)->GetSourceCount() <= RARE_FILE
										) ||
										//OR if file is not rare or if file is complete
										(bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY)
									   )
									{
										client->SetLastSrcReqTime();
										Packet* tosend = reqfile->CreateSrcInfoPacket(client, byRequestedVersion, byRequestedOptions);
										if (tosend)
										{
											if (thePrefs.GetDebugClientTCPLevel() > 0)
												DebugSend("OP__AnswerSources", client, reqfile->GetFileHash());
											theStats.AddUpDataOverheadSourceExchange(tosend->size);
											SendPacket(tosend, true);
										}
									}
									/*else
									{
										if (thePrefs.GetVerbose())
											AddDebugLogLine(false, _T("RCV: Source Request to fast. (This is testing the new timers to see how much older client will not receive this)"));
									}*/
								}
								break;
							}
							default:
							{
								CString strError;
								strError.Format(_T("Invalid sub opcode 0x%02x received"), opcode_in);
								throw strError;
							}
						}
					}
					if (data_out.GetLength() > 16 && !bAnswerFNF)
					{
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__MultiPacketAns", client, reqfile->GetFileHash());
						Packet* reply = new Packet(&data_out, OP_EMULEPROT);
						reply->opcode = (opcode == OP_MULTIPACKET_EXT2) ?  OP_MULTIPACKETANSWER_EXT2 : OP_MULTIPACKETANSWER;
						theStats.AddUpDataOverheadFileRequest(reply->size);
						SendPacket(reply, true);
					}
					break;
				}
				case OP_MULTIPACKETANSWER:
				case OP_MULTIPACKETANSWER_EXT2:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_MultiPacketAns", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->CheckHandshakeFinished();

					if( client->GetKadPort() && client->GetKadVersion() > 1)
						Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort());

					CSafeMemFile data_in(packet, size);
					
					CPartFile* reqfile = NULL;
					if (opcode == OP_MULTIPACKETANSWER_EXT2)
					{
						CFileIdentifierSA fileIdent;
						if (!fileIdent.ReadIdentifier(&data_in))
							throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER_EXT2; ReadIdentifier() failed)");
						reqfile = theApp.downloadqueue->GetFileByID(fileIdent.GetMD4Hash());
						if (reqfile==NULL){
							client->CheckFailedFileIdReqs(fileIdent.GetMD4Hash());
							throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER_EXT2; reqfile==NULL)");
						}
						if (!reqfile->GetFileIdentifier().CompareRelaxed(fileIdent))
							throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER_EXT2; FileIdentifier mistmatch)");
						if (fileIdent.HasAICHHash())
							client->ProcessAICHFileHash(NULL, reqfile, &fileIdent.GetAICHHash());
					}
					else{
						uchar reqfilehash[16];
						data_in.ReadHash16(reqfilehash);
						reqfile = theApp.downloadqueue->GetFileByID(reqfilehash);
						//Make sure we are downloading this file.
						if (reqfile==NULL){
							client->CheckFailedFileIdReqs(reqfilehash);
							throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER; reqfile==NULL)");
						}
					}
					if (client->GetRequestFile()==NULL)
						throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER; client->GetRequestFile()==NULL)");
					if (reqfile != client->GetRequestFile())
						throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER; reqfile!=client->GetRequestFile())");
					uint8 opcode_in;
					while(data_in.GetLength()-data_in.GetPosition())
					{
						opcode_in = data_in.ReadUInt8();
						switch(opcode_in)
						{
							case OP_REQFILENAMEANSWER:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPReqFileNameAns", client, packet);

								client->ProcessFileInfo(&data_in, reqfile);
								break;
							}
							case OP_FILESTATUS:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPFileStatus", client, packet);

								client->ProcessFileStatus(false, &data_in, reqfile);
								break;
							}
							case OP_AICHFILEHASHANS:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPAichFileHashAns", client);
								
								client->ProcessAICHFileHash(&data_in, reqfile, NULL);
								break;
							}
							default:
							{
								CString strError;
								strError.Format(_T("Invalid sub opcode 0x%02x received"), opcode_in);
								throw strError;
							}
						}
					}
					break;
				}
				case OP_EMULEINFO:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					client->ProcessMuleInfoPacket(packet,size);
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_EmuleInfo", client);
						Debug(_T("  %s\n"), client->DbgGetMuleInfo());
					}

					// start secure identification, if
					//  - we have received eD2K and eMule info (old eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();

					client->SendMuleInfoPacket(true);
					break;
				}
				case OP_EMULEINFOANSWER:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					client->ProcessMuleInfoPacket(packet,size);
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_EmuleInfoAnswer", client);
						Debug(_T("  %s\n"), client->DbgGetMuleInfo());
					}

					// start secure identification, if
					//  - we have received eD2K and eMule info (old eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();
					break;
				}
				case OP_SECIDENTSTATE:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_SecIdentState", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					client->ProcessSecIdentStatePacket(packet, size);
					if (client->GetSecureIdentState() == IS_SIGNATURENEEDED)
						client->SendSignaturePacket();
					else if (client->GetSecureIdentState() == IS_KEYANDSIGNEEDED)
					{
						client->SendPublicKeyPacket();
						client->SendSignaturePacket();
					}
					break;
				}
				case OP_PUBLICKEY:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PublicKey", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					client->ProcessPublicKeyPacket(packet, size);
					break;
				}
  				case OP_SIGNATURE:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_Signature", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					client->ProcessSignaturePacket(packet, size);
					break;
				}
				case OP_QUEUERANKING:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_QueueRanking", client);
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->CheckHandshakeFinished();

					client->ProcessEmuleQueueRank(packet, size);
					break;
				}
				case OP_REQUESTSOURCES:
				case OP_REQUESTSOURCES2:
				{
					CSafeMemFile data(packet, size);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv(opcode == OP_REQUESTSOURCES2 ? "OP_MPReqSources2" : "OP_MPReqSources", client, (size >= 16) ? packet : NULL);
					
					theStats.AddDownDataOverheadSourceExchange(uRawSize);
					client->CheckHandshakeFinished();

					uint8 byRequestedVersion = 0;
					uint16 byRequestedOptions = 0;
					if (opcode == OP_REQUESTSOURCES2){ // SX2 requests contains additional data
						byRequestedVersion = data.ReadUInt8();
						byRequestedOptions = data.ReadUInt16();
					}
					//Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.
					if (byRequestedVersion > 0 || client->GetSourceExchange1Version() > 1)
					{
						if (size < 16)
							throw GetResString(IDS_ERR_BADSIZE);

						if (thePrefs.GetDebugSourceExchange())
							AddDebugLogLine(false, _T("SXRecv: Client source request; %s, %s"), client->DbgGetClientInfo(), DbgGetFileInfo(packet));

						//first check shared file list, then download list
						uchar ucHash[16];
						data.ReadHash16(ucHash);
						CKnownFile* reqfile;
						if ((reqfile = theApp.sharedfiles->GetFileByID(ucHash)) != NULL ||
							(reqfile = theApp.downloadqueue->GetFileByID(ucHash)) != NULL)
						{
							// There are some clients which do not follow the correct protocol procedure of sending
							// the sequence OP_REQUESTFILENAME, OP_SETREQFILEID, OP_REQUESTSOURCES. If those clients
							// are doing this, they will not get the optimal set of sources which we could offer if
							// the would follow the above noted protocol sequence. They better to it the right way
							// or they will get just a random set of sources because we do not know their download
							// part status which may get cleared with the call of 'SetUploadFileID'.
							client->SetUploadFileID(reqfile);

							DWORD dwTimePassed = ::GetTickCount() - client->GetLastSrcReqTime() + CONNECTION_LATENCY;
							bool bNeverAskedBefore = client->GetLastSrcReqTime() == 0;
							if (
								//if not complete and file is rare
								(    reqfile->IsPartFile()
								  && (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
								  && ((CPartFile*)reqfile)->GetSourceCount() <= RARE_FILE
								) ||
								//OR if file is not rare or if file is complete
								(bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY)
							   )
							{
								client->SetLastSrcReqTime();
								Packet* tosend = reqfile->CreateSrcInfoPacket(client, byRequestedVersion, byRequestedOptions);
								if (tosend)
								{
									if (thePrefs.GetDebugClientTCPLevel() > 0)
										DebugSend("OP__AnswerSources", client, reqfile->GetFileHash());
									theStats.AddUpDataOverheadSourceExchange(tosend->size);
									SendPacket(tosend, true, true);
								}
							}
						}
						else
							client->CheckFailedFileIdReqs(ucHash);
					}
					break;
				}
 				case OP_ANSWERSOURCES:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AnswerSources", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadSourceExchange(uRawSize);
					client->CheckHandshakeFinished();

					CSafeMemFile data(packet, size);
					uchar hash[16];
					data.ReadHash16(hash);
					CKnownFile* file = theApp.downloadqueue->GetFileByID(hash);
					if (file){
						if (file->IsPartFile()){
							//set the client's answer time
							client->SetLastSrcAnswerTime();
							//and set the file's last answer time
							((CPartFile*)file)->SetLastAnsweredTime();
							((CPartFile*)file)->AddClientSources(&data, client->GetSourceExchange1Version(), false, client);
						}
					}
					else
						client->CheckFailedFileIdReqs(hash);
					break;
				}
 				case OP_ANSWERSOURCES2:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AnswerSources2", client, (size >= 17) ? packet : NULL);
					theStats.AddDownDataOverheadSourceExchange(uRawSize);
					client->CheckHandshakeFinished();

					CSafeMemFile data(packet, size);
					uint8 byVersion = data.ReadUInt8();
					uchar hash[16];
					data.ReadHash16(hash);
					CKnownFile* file = theApp.downloadqueue->GetFileByID(hash);
					if (file){
						if (file->IsPartFile()){
							//set the client's answer time
							client->SetLastSrcAnswerTime();
							//and set the file's last answer time
							((CPartFile*)file)->SetLastAnsweredTime();
							((CPartFile*)file)->AddClientSources(&data, byVersion, true, client);
						}
					}
					else
						client->CheckFailedFileIdReqs(hash);
					break;
				}
				case OP_FILEDESC:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileDesc", client);
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->CheckHandshakeFinished();

					client->ProcessMuleCommentPacket(packet,size);
					break;
				}
				case OP_REQUESTPREVIEW:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_RequestPreView", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadOther(uRawSize);
					client->CheckHandshakeFinished();
	
					if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))	
					{
						if (thePrefs.GetVerbose())
							AddDebugLogLine(true,_T("Client '%s' (%s) requested Preview - accepted"), client->GetUserName(), ipstr(client->GetConnectIP()));
						client->ProcessPreviewReq(packet,size);	
					}
					else
					{
						// we don't send any answer here, because the client should know that he was not allowed to ask
						if (thePrefs.GetVerbose())
							AddDebugLogLine(true,_T("Client '%s' (%s) requested Preview - denied"), client->GetUserName(), ipstr(client->GetConnectIP()));
					}
					break;
				}
				case OP_PREVIEWANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PreviewAnswer", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadOther(uRawSize);
					client->CheckHandshakeFinished();

					client->ProcessPreviewAnswer(packet, size);
					break;
				}
				case OP_PEERCACHE_QUERY:
				{
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					if (!client->ProcessPeerCacheQuery(packet, size))
					{
						CSafeMemFile dataSend(128);
						dataSend.WriteUInt8(PCPCK_VERSION);
						dataSend.WriteUInt8(PCOP_NONE);
						if (thePrefs.GetDebugClientTCPLevel() > 0){
							DebugSend("OP__PeerCacheAnswer", client);
							Debug(_T("  %s\n"), _T("Not supported"));
						}
						Packet* pEd2kPacket = new Packet(&dataSend, OP_EMULEPROT, OP_PEERCACHE_ANSWER);
						theStats.AddUpDataOverheadFileRequest(pEd2kPacket->size);
						SendPacket(pEd2kPacket);
					}
					break;
				}
				case OP_PEERCACHE_ANSWER:
				{
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					if ( (!client->ProcessPeerCacheAnswer(packet, size)) && client->GetDownloadState() != DS_NONEEDEDPARTS)
					{
						// We have sent a PeerCache Query to the remote client, for any reason the remote client
						// can not process it -> fall back to ed2k download.
						client->SetPeerCacheDownState(PCDS_NONE);
						ASSERT( client->m_pPCDownSocket == NULL );

						// PC-TODO: Check client state.
						ASSERT( client->GetDownloadState() == DS_DOWNLOADING );
						client->SetDownloadState(DS_ONQUEUE, _T("Peer cache query trouble")); // clear block requests
						if (client)
							client->StartDownload();
					}
					break;
				}
				case OP_PEERCACHE_ACK:
				{
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->ProcessPeerCacheAcknowledge(packet, size);
					break;
				}
				case OP_PUBLICIP_ANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PublicIPAns", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					client->ProcessPublicIPAnswer(packet, size);
					break;
				}
				case OP_PUBLICIP_REQ:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PublicIPReq", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugSend("OP__PublicIPAns", client);
					Packet* pPacket = new Packet(OP_PUBLICIP_ANSWER, 4, OP_EMULEPROT);
					PokeUInt32(pPacket->pBuffer, client->GetIP());
					theStats.AddUpDataOverheadOther(pPacket->size);
					SendPacket(pPacket);
					break;
				}
				case OP_PORTTEST:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PortTest", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					m_bPortTestCon=true;
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugSend("OP__PortTest", client);
					Packet* replypacket = new Packet(OP_PORTTEST, 1);
					replypacket->pBuffer[0]=0x12;
					theStats.AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket);
					break;
				}
				case OP_CALLBACK:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_Callback", client);
					theStats.AddDownDataOverheadFileRequest(uRawSize);

					if(!Kademlia::CKademlia::IsRunning())
						break;
					CSafeMemFile data(packet, size);
					Kademlia::CUInt128 check;
					data.ReadUInt128(&check);
					check.Xor(Kademlia::CUInt128(true));
					if(check == Kademlia::CKademlia::GetPrefs()->GetKadID())
					{
						Kademlia::CUInt128 fileid;
						data.ReadUInt128(&fileid);
						uchar fileid2[16];
						fileid.ToByteArray(fileid2);
						CKnownFile* reqfile;
						if ( (reqfile = theApp.sharedfiles->GetFileByID(fileid2)) == NULL )
						{
							if ( (reqfile = theApp.downloadqueue->GetFileByID(fileid2)) == NULL)
							{
								client->CheckFailedFileIdReqs(fileid2);
								break;
							}
						}

						uint32 ip = data.ReadUInt32();
						uint16 tcp = data.ReadUInt16();
						CUpDownClient* callback;
						callback = theApp.clientlist->FindClientByIP(ntohl(ip), tcp);
						if( callback == NULL )
						{
							callback = new CUpDownClient(NULL,tcp,ip,0,0);
							theApp.clientlist->AddClient(callback);
						}
						callback->TryToConnect(true);
					}
					break;
				}
				case OP_BUDDYPING:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_BuddyPing", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					CUpDownClient* buddy = theApp.clientlist->GetBuddy();
					if( buddy != client || client->GetKadVersion() == 0 || !client->AllowIncomeingBuddyPingPong() )
						//This ping was not from our buddy or wrong version or packet sent to fast. Ignore
						break;
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugSend("OP__BuddyPong", client);
					Packet* replypacket = new Packet(OP_BUDDYPONG, 0, OP_EMULEPROT);
					theStats.AddDownDataOverheadOther(replypacket->size);
					SendPacket(replypacket);
					client->SetLastBuddyPingPongTime();
					break;
				}
				case OP_BUDDYPONG:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_BuddyPong", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					CUpDownClient* buddy = theApp.clientlist->GetBuddy();
					if( buddy != client || client->GetKadVersion() == 0 )
						//This pong was not from our buddy or wrong version. Ignore
						break;
					client->SetLastBuddyPingPongTime();
					//All this is for is to reset our socket timeout.
					break;
				}
				case OP_REASKCALLBACKTCP:
				{
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					CUpDownClient* buddy = theApp.clientlist->GetBuddy();
					if (buddy != client) {
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugRecv("OP_ReaskCallbackTCP", client, NULL);
						//This callback was not from our buddy.. Ignore.
						break;
					}
					CSafeMemFile data_in(packet, size);
					uint32 destip = data_in.ReadUInt32();
					uint16 destport = data_in.ReadUInt16();
					uchar reqfilehash[16];
					data_in.ReadHash16(reqfilehash);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_ReaskCallbackTCP", client, reqfilehash);
					CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);

					bool bSenderMultipleIpUnknown = false;
					CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(destip, destport, true, &bSenderMultipleIpUnknown);					
					if (!reqfile)
					{
						if (thePrefs.GetDebugClientUDPLevel() > 0)
							DebugSend("OP__FileNotFound", NULL);
						Packet* response = new Packet(OP_FILENOTFOUND,0,OP_EMULEPROT);
						theStats.AddUpDataOverheadFileRequest(response->size);
						if (sender != NULL)
							theApp.clientudp->SendPacket(response, destip, destport, sender->ShouldReceiveCryptUDPPackets(), sender->GetUserHash(), false, 0);
						else
							theApp.clientudp->SendPacket(response, destip, destport, false, NULL, false, 0);
						break;
					}

					if (sender)
					{
						//Make sure we are still thinking about the same file
						if (md4cmp(reqfilehash, sender->GetUploadFileID()) == 0)
						{
							sender->AddAskedCount();
							sender->SetLastUpRequest();
							//I messed up when I first added extended info to UDP
							//I should have originally used the entire ProcessExtenedInfo the first time.
							//So now I am forced to check UDPVersion to see if we are sending all the extended info.
							//For now on, we should not have to change anything here if we change
							//anything to the extended info data as this will be taken care of in ProcessExtendedInfo()
							//Update extended info. 
							if (sender->GetUDPVersion() > 3)
							{
								sender->ProcessExtendedInfo(&data_in, reqfile);
							}
							//Update our complete source counts.
							else if (sender->GetUDPVersion() > 2)
							{
								uint16 nCompleteCountLast= sender->GetUpCompleteSourcesCount();
								uint16 nCompleteCountNew = data_in.ReadUInt16();
								sender->SetUpCompleteSourcesCount(nCompleteCountNew);
								if (nCompleteCountLast != nCompleteCountNew)
								{
									reqfile->UpdatePartsInfo();
								}
							}
							CSafeMemFile data_out(128);
							if(sender->GetUDPVersion() > 3)
							{
								if (reqfile->IsPartFile())
									((CPartFile*)reqfile)->WritePartStatus(&data_out);
								else
									data_out.WriteUInt16(0);
							}
							data_out.WriteUInt16((uint16)theApp.uploadqueue->GetWaitingPosition(sender));
							if (thePrefs.GetDebugClientUDPLevel() > 0)
								DebugSend("OP__ReaskAck", sender);
							Packet* response = new Packet(&data_out, OP_EMULEPROT);
							response->opcode = OP_REASKACK;
							theStats.AddUpDataOverheadFileRequest(response->size);
							theApp.clientudp->SendPacket(response, destip, destport, sender->ShouldReceiveCryptUDPPackets(), sender->GetUserHash(), false, 0);
						}
						else
						{
							DebugLogWarning(_T("Client UDP socket; OP_REASKCALLBACKTCP; reqfile does not match"));
							TRACE(_T("reqfile:         %s\n"), DbgGetFileInfo(reqfile->GetFileHash()));
							TRACE(_T("sender->GetRequestFile(): %s\n"), sender->GetRequestFile() ? DbgGetFileInfo(sender->GetRequestFile()->GetFileHash()) : _T("(null)"));
						}
					}
					else
					{
						if (!bSenderMultipleIpUnknown){
							if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > thePrefs.GetQueueSize())
							{
								if (thePrefs.GetDebugClientUDPLevel() > 0)
									DebugSend("OP__QueueFull", NULL);
								Packet* response = new Packet(OP_QUEUEFULL,0,OP_EMULEPROT);
								theStats.AddUpDataOverheadFileRequest(response->size);
								theApp.clientudp->SendPacket(response, destip, destport, false, NULL, false, 0);
							}
						}
						else{
							DebugLogWarning(_T("OP_REASKCALLBACKTCP Packet received - multiple clients with the same IP but different UDP port found. Possible UDP Portmapping problem, enforcing TCP connection. IP: %s, Port: %u"), ipstr(destip), destport); 
						}
					}
					break;
				}
				case OP_AICHANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AichAnswer", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(uRawSize);

					client->ProcessAICHAnswer(packet,size);
					break;
				}
				case OP_AICHREQUEST:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AichRequest", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(uRawSize);

					client->ProcessAICHRequest(packet,size);
					break;
				}
				case OP_AICHFILEHASHANS:
				{
					// those should not be received normally, since we should only get those in MULTIPACKET
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AichFileHashAns", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(uRawSize);

					CSafeMemFile data(packet, size);
					client->ProcessAICHFileHash(&data, NULL, NULL);
					break;
				}
				case OP_AICHFILEHASHREQ:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AichFileHashReq", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(uRawSize);

					// those should not be received normally, since we should only get those in MULTIPACKET
					CSafeMemFile data(packet, size);
					uchar abyHash[16];
					data.ReadHash16(abyHash);
					CKnownFile* pPartFile = theApp.sharedfiles->GetFileByID(abyHash);
					if (pPartFile == NULL){
						client->CheckFailedFileIdReqs(abyHash);
						break;
					}
					if (client->IsSupportingAICH() && pPartFile->GetFileIdentifier().HasAICHHash())
					{
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AichFileHashAns", client, abyHash);
						CSafeMemFile data_out;
						data_out.WriteHash16(abyHash);
						pPartFile->GetFileIdentifier().GetAICHHash().Write(&data_out);
						Packet* response = new Packet(&data_out, OP_EMULEPROT, OP_AICHFILEHASHANS);
						theStats.AddUpDataOverheadFileRequest(response->size);
						SendPacket(response);
					}
					break;
				}
				case OP_REQUESTPARTS_I64:
				{
					// see also OP_REQUESTPARTS
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_RequestParts_I64", client, (size >= 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					CSafeMemFile data(packet, size);
					uchar reqfilehash[16];
					data.ReadHash16(reqfilehash);

					uint64 auStartOffsets[3];
					auStartOffsets[0] = data.ReadUInt64();
					auStartOffsets[1] = data.ReadUInt64();
					auStartOffsets[2] = data.ReadUInt64();

					uint64 auEndOffsets[3];
					auEndOffsets[0] = data.ReadUInt64();
					auEndOffsets[1] = data.ReadUInt64();
					auEndOffsets[2] = data.ReadUInt64();

					if (thePrefs.GetDebugClientTCPLevel() > 0){
						Debug(_T("  Start1=%I64u  End1=%I64u  Size=%I64u\n"), auStartOffsets[0], auEndOffsets[0], auEndOffsets[0] - auStartOffsets[0]);
						Debug(_T("  Start2=%I64u  End2=%I64u  Size=%I64u\n"), auStartOffsets[1], auEndOffsets[1], auEndOffsets[1] - auStartOffsets[1]);
						Debug(_T("  Start3=%I64u  End3=%I64u  Size=%I64u\n"), auStartOffsets[2], auEndOffsets[2], auEndOffsets[2] - auStartOffsets[2]);
					}

					for (int i = 0; i < ARRSIZE(auStartOffsets); i++)
					{
						if (auEndOffsets[i] > auStartOffsets[i])
						{
							Requested_Block_Struct* reqblock = new Requested_Block_Struct;
							reqblock->StartOffset = auStartOffsets[i];
							reqblock->EndOffset = auEndOffsets[i];
							md4cpy(reqblock->FileID, reqfilehash);
							reqblock->transferred = 0;
							client->AddReqBlock(reqblock, false);
						}
						else
						{
							if (thePrefs.GetVerbose())
							{
								if (auEndOffsets[i] != 0 || auStartOffsets[i] != 0)
									DebugLogWarning(_T("Client requests invalid %u. file block %I64u-%I64u (%I64d bytes): %s"), i, auStartOffsets[i], auEndOffsets[i], auEndOffsets[i] - auStartOffsets[i], client->DbgGetClientInfo());
							}
						}
					}
					client->AddReqBlock(NULL, true);
					break;
				}
				case OP_COMPRESSEDPART:
				case OP_SENDINGPART_I64:
				case OP_COMPRESSEDPART_I64:
				{
					// see also OP_SENDINGPART
					if (thePrefs.GetDebugClientTCPLevel() > 1){
						if (opcode == OP_COMPRESSEDPART)
							DebugRecv("OP_CompressedPart", client, (size >= 16) ? packet : NULL);
						else if (opcode == OP_SENDINGPART_I64)
							DebugRecv("OP_SendingPart_I64", client, (size >= 16) ? packet : NULL);
						else
							DebugRecv("OP_CompressedPart_I64", client, (size >= 16) ? packet : NULL);
					}
					
					theStats.AddDownDataOverheadFileRequest(16 + 2*(opcode == OP_COMPRESSEDPART ? 4 : 8));
					client->CheckHandshakeFinished();

					if (client->GetRequestFile() && !client->GetRequestFile()->IsStopped() && (client->GetRequestFile()->GetStatus()==PS_READY || client->GetRequestFile()->GetStatus()==PS_EMPTY))
					{
						client->ProcessBlockPacket(packet, size, (opcode == OP_COMPRESSEDPART || opcode == OP_COMPRESSEDPART_I64), (opcode == OP_SENDINGPART_I64 || opcode == OP_COMPRESSEDPART_I64) );
						if (client->GetRequestFile())
						{
							if (client->GetRequestFile()->IsStopped() || client->GetRequestFile()->GetStatus()==PS_PAUSED || client->GetRequestFile()->GetStatus()==PS_ERROR)
							{
								client->SendCancelTransfer();
								client->SetDownloadState(client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);
							}
						}
						else
							ASSERT( false );
					}
					else
					{
						client->SendCancelTransfer();
						client->SetDownloadState((client->GetRequestFile()==NULL || client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
					}
					break;
				}
				case OP_CHATCAPTCHAREQ:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_CHATCAPTCHAREQ", client);
					theStats.AddDownDataOverheadOther(uRawSize);
					CSafeMemFile data(packet, size);
					client->ProcessCaptchaRequest(&data);
					break;
				}
				case OP_CHATCAPTCHARES:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_CHATCAPTCHARES", client);
					theStats.AddDownDataOverheadOther(uRawSize);
					if (size < 1)
						throw GetResString(IDS_ERR_BADSIZE);
					client->ProcessCaptchaReqRes(packet[0]);
					break;
				}
				case OP_FWCHECKUDPREQ: //*Support required for Kadversion >= 6
				{
					// Kad related packet
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FWCHECKUDPREQ", client);
					theStats.AddDownDataOverheadOther(uRawSize);
					CSafeMemFile data(packet, size);
					client->ProcessFirewallCheckUDPRequest(&data);
					break;
				}
				case OP_KAD_FWTCPCHECK_ACK: //*Support required for Kadversion >= 7
				{
					// Kad related packet, replaces KADEMLIA_FIREWALLED_ACK_RES
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_KAD_FWTCPCHECK_ACK", client);
					if (theApp.clientlist->IsKadFirewallCheckIP(client->GetIP())){
						if (Kademlia::CKademlia::IsRunning())
							Kademlia::CKademlia::GetPrefs()->IncFirewalled();
					}
					else
						DebugLogWarning(_T("Unrequested OP_KAD_FWTCPCHECK_ACK packet from client %s"), client->DbgGetClientInfo());
					break;
				}
				case OP_HASHSETANSWER2:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_HashSetAnswer2", client);
					theStats.AddDownDataOverheadFileRequest(size);
					client->ProcessHashSet(packet, size, true);
					break;
				}
				case OP_HASHSETREQUEST2:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_HashSetReq2", client);
					theStats.AddDownDataOverheadFileRequest(size);
					client->SendHashsetPacket(packet, size, true);
					break;
				}
				default:
					theStats.AddDownDataOverheadOther(uRawSize);
					PacketToDebugLogLine(_T("eMule"), packet, size, opcode);
					break;
			}
		}
		catch(CFileException* error)
		{
			error->Delete();
			throw GetResString(IDS_ERR_INVALIDPACKAGE);
		}
		catch(CMemoryException* error)
		{
			error->Delete();
			throw CString(_T("Memory exception"));
		}
	}
	catch(CClientException* ex) // nearly same as the 'CString' exception but with optional deleting of the client
	{
		if (thePrefs.GetVerbose() && !ex->m_strMsg.IsEmpty())
			DebugLogWarning(_T("Error: %s - while processing eMule packet: opcode=%s  size=%u; %s"), ex->m_strMsg, DbgGetMuleClientTCPOpcode(opcode), size, DbgGetClientInfo());
		if (client && ex->m_bDelete)
			client->SetDownloadState(DS_ERROR, _T("Error while processing eMule packet: ") + ex->m_strMsg);
		Disconnect(ex->m_strMsg);
		ex->Delete();
		return false;
	}
	catch(CString error)
	{
		if (thePrefs.GetVerbose() && !error.IsEmpty())
			DebugLogWarning(_T("Error: %s - while processing eMule packet: opcode=%s  size=%u; %s"), error, DbgGetMuleClientTCPOpcode(opcode), size, DbgGetClientInfo());
		if (client)
			client->SetDownloadState(DS_ERROR, _T("ProcessExtPacket error. ") + error);
		Disconnect(_T("ProcessExtPacket error. ") + error);
		return false;
	}
	return true;
}

void CClientReqSocket::PacketToDebugLogLine(LPCTSTR protocol, const uchar* packet, uint32 size, UINT opcode)
{
	if (thePrefs.GetVerbose())
	{
		CString buffer; 
	    buffer.Format(_T("Unknown %s Protocol Opcode: 0x%02x, Size=%u, Data=["), protocol, opcode, size);
		UINT i;
		for (i = 0; i < size && i < 50; i++){
			if (i > 0)
				buffer += _T(' ');
			TCHAR temp[3];
		    _stprintf(temp, _T("%02x"), packet[i]);
			buffer += temp;
		}
		buffer += (i == size) ? _T("]") : _T("..]");
		DbgAppendClientInfo(buffer);
		DebugLogWarning(_T("%s"), buffer);
	}
}

CString CClientReqSocket::DbgGetClientInfo()
{
	CString str;
	SOCKADDR_IN sockAddr = {0};
	int nSockAddrLen = sizeof(sockAddr);
	GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
	if (sockAddr.sin_addr.S_un.S_addr != 0 && (client == NULL || sockAddr.sin_addr.S_un.S_addr != client->GetIP()))
		str.AppendFormat(_T("IP=%s"), ipstr(sockAddr.sin_addr));
	if (client){
		if (!str.IsEmpty())
			str += _T("; ");
		str += _T("Client=") + client->DbgGetClientInfo();
	}
	return str;
}

void CClientReqSocket::DbgAppendClientInfo(CString& str)
{
	CString strClientInfo(DbgGetClientInfo());
	if (!strClientInfo.IsEmpty()){
		if (!str.IsEmpty())
			str += _T("; ");
		str += strClientInfo;
	}
}

void CClientReqSocket::OnConnect(int nErrorCode)
{
	SetConState(SS_Complete);
	CEMSocket::OnConnect(nErrorCode);
	if (nErrorCode)
	{
	    CString strTCPError;
		if (thePrefs.GetVerbose())
		{
			strTCPError = GetFullErrorMessage(nErrorCode);
		    if ((nErrorCode != WSAECONNREFUSED && nErrorCode != WSAETIMEDOUT) || !GetLastProxyError().IsEmpty())
			    DebugLogError(_T("Client TCP socket (OnConnect): %s; %s"), strTCPError, DbgGetClientInfo());
		}
		Disconnect(strTCPError);
	}
	else
	{
		//This socket may have been delayed by SP2 protection, lets make sure it doesn't time out instantly.
		ResetTimeOutTimer();
	}
}

void CClientReqSocket::OnSend(int nErrorCode)
{
	ResetTimeOutTimer();
	CEMSocket::OnSend(nErrorCode);
}

void CClientReqSocket::OnError(int nErrorCode)
{
	CString strTCPError;
	if (thePrefs.GetVerbose())
	{
		if (nErrorCode == ERR_WRONGHEADER)
			strTCPError = _T("Error: Wrong header");
		else if (nErrorCode == ERR_TOOBIG)
			strTCPError = _T("Error: Too much data sent");
		else if (nErrorCode == ERR_ENCRYPTION)
			strTCPError = _T("Error: Encryption layer error");
		else if (nErrorCode == ERR_ENCRYPTION_NOTALLOWED)
			strTCPError = _T("Error: Unencrypted Connection when Encryption was required");
		else
			strTCPError = GetErrorMessage(nErrorCode);
		DebugLogWarning(_T("Client TCP socket: %s; %s"), strTCPError, DbgGetClientInfo());
	}

	Disconnect(strTCPError);
}

bool CClientReqSocket::PacketReceivedCppEH(Packet* packet)
{
	bool bResult;
	UINT uRawSize = packet->size;
	switch (packet->prot){
		case OP_EDONKEYPROT:
			bResult = ProcessPacket((const BYTE*)packet->pBuffer, packet->size, packet->opcode);
			break;
		case OP_PACKEDPROT:
			if (!packet->UnPackPacket()){
				if (thePrefs.GetVerbose())
					DebugLogError(_T("Failed to decompress client TCP packet; %s; %s"), DbgGetClientTCPPacket(packet->prot, packet->opcode, packet->size), DbgGetClientInfo());
				bResult = false;
				break;
			}
		case OP_EMULEPROT:
			bResult = ProcessExtPacket((const BYTE*)packet->pBuffer, packet->size, packet->opcode, uRawSize);
			break;
		default:{
			theStats.AddDownDataOverheadOther(uRawSize);
			if (thePrefs.GetVerbose())
				DebugLogWarning(_T("Received unknown client TCP packet; %s; %s"), DbgGetClientTCPPacket(packet->prot, packet->opcode, packet->size), DbgGetClientInfo());

			if (client)
				client->SetDownloadState(DS_ERROR, _T("Unknown protocol"));
			Disconnect(_T("Unknown protocol"));
			bResult = false;
		}
	}
	return bResult;
}

#if !NO_USE_CLIENT_TCP_CATCH_ALL_HANDLER
int FilterSE(DWORD dwExCode, LPEXCEPTION_POINTERS pExPtrs, CClientReqSocket* reqsock, Packet* packet)
{
	if (thePrefs.GetVerbose())
	{
		CString strExError;
		if (pExPtrs){
			const EXCEPTION_RECORD* er = pExPtrs->ExceptionRecord;
			strExError.Format(_T("Error: Unknown exception %08x in CClientReqSocket::PacketReceived at %p"), er->ExceptionCode, er->ExceptionAddress);
		}
		else
			strExError.Format(_T("Error: Unknown exception %08x in CClientReqSocket::PacketReceived"), dwExCode);

		// we already had an unknown exception, better be prepared for dealing with invalid data -> use another exception handler
		try{
			CString strError = strExError;
			strError.AppendFormat(_T("; %s"), DbgGetClientTCPPacket(packet?packet->prot:0, packet?packet->opcode:0, packet?packet->size:0));
			reqsock->DbgAppendClientInfo(strError);
			DebugLogError(_T("%s"), strError);
		}
		catch(...){
			ASSERT(0);
			DebugLogError(_T("%s"), strExError);
		}
	}
	
	// this searches the next exception handler -> catch(...) in 'CAsyncSocketExHelperWindow::WindowProc'
	// as long as I do not know where and why we are crashing, I prefere to have it handled that way which
	// worked fine in 28a/b.
	//
	// 03-Jän-2004 [bc]: Returning the execution to the catch-all handler in 'CAsyncSocketExHelperWindow::WindowProc'
	// can make things even worse, because in some situations, the socket will continue fireing received events. And
	// because the processed packet (which has thrown the exception) was not removed from the EMSocket buffers, it would
	// be processed again and again.
	//return EXCEPTION_CONTINUE_SEARCH;

	// this would continue the program "as usual" -> return execution to the '__except' handler
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif//!NO_USE_CLIENT_TCP_CATCH_ALL_HANDLER

#if !NO_USE_CLIENT_TCP_CATCH_ALL_HANDLER
int CClientReqSocket::PacketReceivedSEH(Packet* packet)
{
	int iResult;
	// this function is only here to get a chance of determining the crash address via SEH
	__try{
		iResult = PacketReceivedCppEH(packet);
	}
	__except(FilterSE(GetExceptionCode(), GetExceptionInformation(), this, packet)){
		iResult = -1;
	}
	return iResult;
}
#endif//!NO_USE_CLIENT_TCP_CATCH_ALL_HANDLER

bool CClientReqSocket::PacketReceived(Packet* packet)
{
	bool bResult;
#if !NO_USE_CLIENT_TCP_CATCH_ALL_HANDLER
	int iResult = PacketReceivedSEH(packet);
	if (iResult < 0)
	{
		if (client)
			client->SetDownloadState(DS_ERROR, _T("Unknown Exception"));
		Disconnect(_T("Unknown Exception"));
		bResult = false;
	}
	else
		bResult = iResult!=0;
#else//!NO_USE_CLIENT_TCP_CATCH_ALL_HANDLER
	bResult = PacketReceivedCppEH(packet);
#endif//!NO_USE_CLIENT_TCP_CATCH_ALL_HANDLER
	return bResult;
}

void CClientReqSocket::OnReceive(int nErrorCode)
{
	ResetTimeOutTimer();
	CEMSocket::OnReceive(nErrorCode);
}

bool CClientReqSocket::Create()
{
	theApp.listensocket->AddConnection();
	return (CAsyncSocketEx::Create(0, SOCK_STREAM, FD_WRITE | FD_READ | FD_CLOSE | FD_CONNECT, thePrefs.GetBindAddrA()) != FALSE);
}

SocketSentBytes CClientReqSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend)
{
    SocketSentBytes returnStatus = CEMSocket::SendControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);
    if (returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0))
        ResetTimeOutTimer();
    return returnStatus;
}

SocketSentBytes CClientReqSocket::SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend)
{
    SocketSentBytes returnStatus = CEMSocket::SendFileAndControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);
    if (returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0))
        ResetTimeOutTimer();
    return returnStatus;
}

void CClientReqSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize, bool bForceImmediateSend)
{
	ResetTimeOutTimer();
	CEMSocket::SendPacket(packet, delpacket, controlpacket, actualPayloadSize, bForceImmediateSend);
}

bool CListenSocket::SendPortTestReply(char result, bool disconnect)
{
	POSITION pos2;
	for(POSITION pos1 = socket_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; )
	{
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		if (cur_sock->m_bPortTestCon)
		{
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				DebugSend("OP__PortTest", cur_sock->client);
			Packet* replypacket = new Packet(OP_PORTTEST, 1);
			replypacket->pBuffer[0]=result;
			theStats.AddUpDataOverheadOther(replypacket->size);
			cur_sock->SendPacket(replypacket);
			if (disconnect)
				cur_sock->m_bPortTestCon = false;
			return true;
		}
	}
	return false;
}

CListenSocket::CListenSocket()
{
	bListening = false;
	maxconnectionreached = 0;
	m_OpenSocketsInterval = 0;
	m_nPendingConnections = 0;
	memset(m_ConnectionStates, 0, sizeof m_ConnectionStates);
	peakconnections = 0;
	totalconnectionchecks = 0;
	averageconnections = 0.0;
	activeconnections = 0;
	m_port=0;
	m_nHalfOpen = 0;
	m_nComp = 0;
}

CListenSocket::~CListenSocket()
{
	Close();
	KillAllSockets();
}

bool CListenSocket::Rebind()
{
	if (thePrefs.GetPort() == m_port)
		return false;

	Close();
	KillAllSockets();

	return StartListening();
}

bool CListenSocket::StartListening()
{
	bListening = true;

	// Creating the socket with SO_REUSEADDR may solve LowID issues if emule was restarted
	// quickly or started after a crash, but(!) it will also create another problem. If the
	// socket is already used by some other application (e.g. a 2nd emule), we though bind
	// to that socket leading to the situation that 2 applications are listening at the same
	// port!
	if (!Create(thePrefs.GetPort(), SOCK_STREAM, FD_ACCEPT, thePrefs.GetBindAddrA(), FALSE/*bReuseAddr*/))
		return false;

	// Rejecting a connection with conditional WSAAccept and not using SO_CONDITIONAL_ACCEPT
	// -------------------------------------------------------------------------------------
	// recv: SYN
	// send: SYN ACK (!)
	// recv: ACK
	// send: ACK RST
	// recv: PSH ACK + OP_HELLO packet
	// send: RST
	// --- 455 total bytes (depending on OP_HELLO packet)
	// In case SO_CONDITIONAL_ACCEPT is not used, the TCP/IP stack establishes the connection
	// before WSAAccept has a chance to reject it. That's why the remote peer starts to send
	// it's first data packet.
	// ---
	// Not using SO_CONDITIONAL_ACCEPT gives us 6 TCP packets and the OP_HELLO data. We
	// have to lookup the IP only 1 time. This is still way less traffic than rejecting the
	// connection by closing it after the 'Accept'.

	// Rejecting a connection with conditional WSAAccept and using SO_CONDITIONAL_ACCEPT
	// ---------------------------------------------------------------------------------
	// recv: SYN
	// send: ACK RST
	// recv: SYN
	// send: ACK RST
	// recv: SYN
	// send: ACK RST
	// --- 348 total bytes
	// The TCP/IP stack tries to establish the connection 3 times until it gives up. 
	// Furthermore the remote peer experiences a total timeout of ~ 1 minute which is
	// supposed to be the default TCP/IP connection timeout (as noted in MSDN).
	// ---
	// Although we get a total of 6 TCP packets in case of using SO_CONDITIONAL_ACCEPT,
	// it's still less than not using SO_CONDITIONAL_ACCEPT. But, we have to lookup
	// the IP 3 times instead of 1 time.

	//if (thePrefs.GetConditionalTCPAccept() && !thePrefs.GetProxySettings().UseProxy) {
	//	int iOptVal = 1;
	//	VERIFY( SetSockOpt(SO_CONDITIONAL_ACCEPT, &iOptVal, sizeof iOptVal) );
	//}

	if (!Listen())
		return false;

	m_port = thePrefs.GetPort();
	return true;
}

void CListenSocket::ReStartListening()
{
	bListening = true;

	ASSERT( m_nPendingConnections >= 0 );
	if (m_nPendingConnections > 0)
	{
		m_nPendingConnections--;
		OnAccept(0);
	}
}

void CListenSocket::StopListening()
{
	bListening = false;
	maxconnectionreached++;
}

static int s_iAcceptConnectionCondRejected;

int CALLBACK AcceptConnectionCond(LPWSABUF lpCallerId, LPWSABUF /*lpCallerData*/, LPQOS /*lpSQOS*/, LPQOS /*lpGQOS*/,
								  LPWSABUF /*lpCalleeId*/, LPWSABUF /*lpCalleeData*/, GROUP FAR* /*g*/, DWORD /*dwCallbackData*/)
{
	if (lpCallerId && lpCallerId->buf && lpCallerId->len >= sizeof SOCKADDR_IN)
	{
		LPSOCKADDR_IN pSockAddr = (LPSOCKADDR_IN)lpCallerId->buf;
		ASSERT( pSockAddr->sin_addr.S_un.S_addr != 0 && pSockAddr->sin_addr.S_un.S_addr != INADDR_NONE );

		if (theApp.ipfilter->IsFiltered(pSockAddr->sin_addr.S_un.S_addr)){
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("Rejecting connection attempt (IP=%s) - IP filter (%s)"), ipstr(pSockAddr->sin_addr.S_un.S_addr), theApp.ipfilter->GetLastHit());
			s_iAcceptConnectionCondRejected = 1;
			return CF_REJECT;
		}

		if (theApp.clientlist->IsBannedClient(pSockAddr->sin_addr.S_un.S_addr)){
			if (thePrefs.GetLogBannedClients()){
				CUpDownClient* pClient = theApp.clientlist->FindClientByIP(pSockAddr->sin_addr.S_un.S_addr);
				AddDebugLogLine(false, _T("Rejecting connection attempt of banned client %s %s"), ipstr(pSockAddr->sin_addr.S_un.S_addr), pClient->DbgGetClientInfo());
			}
			s_iAcceptConnectionCondRejected = 2;
			return CF_REJECT;
		}
	}
	else {
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Client TCP socket: AcceptConnectionCond unexpected lpCallerId"));
	}

	return CF_ACCEPT;
}

void CListenSocket::OnAccept(int nErrorCode)
{
	if (!nErrorCode)
	{
		m_nPendingConnections++;
		if (m_nPendingConnections < 1){
			ASSERT(0);
			m_nPendingConnections = 1;
		}

		if (TooManySockets(true) && !theApp.serverconnect->IsConnecting()){
			StopListening();
			return;
		}
		else if (!bListening)
			ReStartListening(); //If the client is still at maxconnections, this will allow it to go above it.. But if you don't, you will get a lowID on all servers.
	
		uint32 nFataErrors = 0;
		while (m_nPendingConnections > 0)
		{
			m_nPendingConnections--;

			CClientReqSocket* newclient;
			SOCKADDR_IN SockAddr = {0};
			int iSockAddrLen = sizeof SockAddr;
			if (thePrefs.GetConditionalTCPAccept() && !thePrefs.GetProxySettings().UseProxy)
			{
				s_iAcceptConnectionCondRejected = 0;
				SOCKET sNew = WSAAccept(m_SocketData.hSocket, (SOCKADDR*)&SockAddr, &iSockAddrLen, AcceptConnectionCond, 0);
				if (sNew == INVALID_SOCKET){
				    DWORD nError = GetLastError();
				    if (nError == WSAEWOULDBLOCK){
					    DebugLogError(LOG_STATUSBAR, _T("%hs: Backlogcounter says %u connections waiting, Accept() says WSAEWOULDBLOCK - setting counter to zero!"), __FUNCTION__, m_nPendingConnections);
					    m_nPendingConnections = 0;
					    break;
				    }
				    else{
						if (nError != WSAECONNREFUSED || s_iAcceptConnectionCondRejected == 0){
						    DebugLogError(LOG_STATUSBAR, _T("%hs: Backlogcounter says %u connections waiting, Accept() says %s - setting counter to zero!"), __FUNCTION__, m_nPendingConnections, GetErrorMessage(nError, 1));
							nFataErrors++;
						}
						else if (s_iAcceptConnectionCondRejected == 1)
							theStats.filteredclients++;
				    }
				    if (nFataErrors > 10){
					    // the question is what todo on a error. We cant just ignore it because then the backlog will fill up
					    // and lock everything. We can also just endlos try to repeat it because this will lock up eMule
					    // this should basically never happen anyway
					    // however if we are in such a position, try to reinitalize the socket.
					    DebugLogError(LOG_STATUSBAR, _T("%hs: Accept() Error Loop, recreating socket"), __FUNCTION__);
					    Close();
					    StartListening();
					    m_nPendingConnections = 0;
					    break;
				    }
					continue;
				}
				newclient = new CClientReqSocket;
				VERIFY( newclient->InitAsyncSocketExInstance() );
				newclient->m_SocketData.hSocket = sNew;
				newclient->AttachHandle(sNew);

				AddConnection();
			}
			else
			{
				newclient = new CClientReqSocket;
			    if (!Accept(*newclient, (SOCKADDR*)&SockAddr, &iSockAddrLen)){
				    newclient->Safe_Delete();
				    DWORD nError = GetLastError();
				    if (nError == WSAEWOULDBLOCK){
					    DebugLogError(LOG_STATUSBAR, _T("%hs: Backlogcounter says %u connections waiting, Accept() says WSAEWOULDBLOCK - setting counter to zero!"), __FUNCTION__, m_nPendingConnections);
					    m_nPendingConnections = 0;
					    break;
				    }
				    else{
					    DebugLogError(LOG_STATUSBAR, _T("%hs: Backlogcounter says %u connections waiting, Accept() says %s - setting counter to zero!"), __FUNCTION__, m_nPendingConnections, GetErrorMessage(nError, 1));
					    nFataErrors++;
				    }
				    if (nFataErrors > 10){
					    // the question is what todo on a error. We cant just ignore it because then the backlog will fill up
					    // and lock everything. We can also just endlos try to repeat it because this will lock up eMule
					    // this should basically never happen anyway
					    // however if we are in such a position, try to reinitalize the socket.
					    DebugLogError(LOG_STATUSBAR, _T("%hs: Accept() Error Loop, recreating socket"), __FUNCTION__);
					    Close();
					    StartListening();
					    m_nPendingConnections = 0;
					    break;
				    }
				    continue;
			    }
	    
			    AddConnection();
    
			    if (SockAddr.sin_addr.S_un.S_addr == 0) // for safety..
			    {
				    iSockAddrLen = sizeof SockAddr;
				    newclient->GetPeerName((SOCKADDR*)&SockAddr, &iSockAddrLen);
				    DebugLogWarning(_T("SockAddr.sin_addr.S_un.S_addr == 0;  GetPeerName returned %s"), ipstr(SockAddr.sin_addr.S_un.S_addr));
			    }
    
			    ASSERT( SockAddr.sin_addr.S_un.S_addr != 0 && SockAddr.sin_addr.S_un.S_addr != INADDR_NONE );
    
			    if (theApp.ipfilter->IsFiltered(SockAddr.sin_addr.S_un.S_addr)){
				    if (thePrefs.GetLogFilteredIPs())
					    AddDebugLogLine(false, _T("Rejecting connection attempt (IP=%s) - IP filter (%s)"), ipstr(SockAddr.sin_addr.S_un.S_addr), theApp.ipfilter->GetLastHit());
				    newclient->Safe_Delete();
				    theStats.filteredclients++;
				    continue;
			    }
    
			    if (theApp.clientlist->IsBannedClient(SockAddr.sin_addr.S_un.S_addr)){
				    if (thePrefs.GetLogBannedClients()){
					    CUpDownClient* pClient = theApp.clientlist->FindClientByIP(SockAddr.sin_addr.S_un.S_addr);
					    AddDebugLogLine(false, _T("Rejecting connection attempt of banned client %s %s"), ipstr(SockAddr.sin_addr.S_un.S_addr), pClient->DbgGetClientInfo());
				    }
				    newclient->Safe_Delete();
				    continue;
			    }
			}
			newclient->AsyncSelect(FD_WRITE | FD_READ | FD_CLOSE);
		}

		ASSERT( m_nPendingConnections >= 0 );
	}
}

void CListenSocket::Process()
{
	m_OpenSocketsInterval = 0;
	POSITION pos2;
	for (POSITION pos1 = socket_list.GetHeadPosition(); (pos2 = pos1) != NULL; )
	{
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		if (cur_sock->deletethis)
		{
			if (cur_sock->m_SocketData.hSocket != INVALID_SOCKET){
				cur_sock->Close();			// calls 'closesocket'
			}
			else{
				cur_sock->Delete_Timed();	// may delete 'cur_sock'
			}
		}
		else{
			cur_sock->CheckTimeOut();		// may call 'shutdown'
		}
	}

	if ((GetOpenSockets() + 5 < thePrefs.GetMaxConnections() || theApp.serverconnect->IsConnecting()) && !bListening)
		ReStartListening();
}

void CListenSocket::RecalculateStats()
{
	memset(m_ConnectionStates, 0, sizeof m_ConnectionStates);
	for (POSITION pos = socket_list.GetHeadPosition(); pos != NULL; )
	{
		switch (socket_list.GetNext(pos)->GetConState())
		{
			case ES_DISCONNECTED:
				m_ConnectionStates[0]++;
				break;
			case ES_NOTCONNECTED:
				m_ConnectionStates[1]++;
				break;
			case ES_CONNECTED:
				m_ConnectionStates[2]++;
				break;
		}
   }
}

void CListenSocket::AddSocket(CClientReqSocket* toadd)
{
	socket_list.AddTail(toadd);
}

void CListenSocket::RemoveSocket(CClientReqSocket* todel)
{
	for (POSITION pos = socket_list.GetHeadPosition(); pos != NULL; )
	{
		POSITION posLast = pos;
		if (socket_list.GetNext(pos) == todel)
			socket_list.RemoveAt(posLast);
	}
}

void CListenSocket::KillAllSockets()
{
	for (POSITION pos = socket_list.GetHeadPosition(); pos != 0; pos = socket_list.GetHeadPosition())
	{
		CClientReqSocket* cur_socket = socket_list.GetAt(pos);
		if (cur_socket->client)
			delete cur_socket->client;
		else
			delete cur_socket;
	}
}

void CListenSocket::AddConnection()
{
	m_OpenSocketsInterval++;
}

bool CListenSocket::TooManySockets(bool bIgnoreInterval)
{
	if (   GetOpenSockets() > thePrefs.GetMaxConnections()
		|| (m_OpenSocketsInterval > (thePrefs.GetMaxConperFive() * GetMaxConperFiveModifier()) && !bIgnoreInterval)
		|| (m_nHalfOpen >= thePrefs.GetMaxHalfConnections() && !bIgnoreInterval))
		return true;
	return false;
}

bool CListenSocket::IsValidSocket(CClientReqSocket* totest)
{
	return socket_list.Find(totest) != NULL;
}

#ifdef _DEBUG
void CListenSocket::Debug_ClientDeleted(CUpDownClient* deleted)
{
	for (POSITION pos = socket_list.GetHeadPosition(); pos != NULL;)
	{
		CClientReqSocket* cur_sock = socket_list.GetNext(pos);
		if (!AfxIsValidAddress(cur_sock, sizeof(CClientReqSocket)))
			AfxDebugBreak();
		if (thePrefs.m_iDbgHeap >= 2)
			ASSERT_VALID(cur_sock);
		if (cur_sock->client == deleted)
			AfxDebugBreak();
	}
}
#endif

void CListenSocket::UpdateConnectionsStatus()
{
	activeconnections = GetOpenSockets();

	// Update statistics for 'peak connections'
	if (peakconnections < activeconnections)
		peakconnections = activeconnections;
	if (peakconnections > thePrefs.GetConnPeakConnections())
		thePrefs.SetConnPeakConnections(peakconnections);

	if (theApp.IsConnected())
	{
		totalconnectionchecks++;
		if (totalconnectionchecks == 0) {
			 // wrap around occured, avoid division by zero
			 totalconnectionchecks = 100;
		}

		// Get a weight for the 'avg. connections' value. The longer we run the higher 
		// gets the weight (the percent of 'avg. connections' we use).
		float fPercent = (float)(totalconnectionchecks - 1) / (float)totalconnectionchecks;
		if (fPercent > 0.99F)
			fPercent = 0.99F;

		// The longer we run the more we use the 'avg. connections' value and the less we
		// use the 'active connections' value. However, if we are running quite some time
		// without any connections (except the server connection) we will eventually create 
		// a floating point underflow exception.
		averageconnections = averageconnections * fPercent + activeconnections * (1.0F - fPercent);
		if (averageconnections < 0.001F)
			averageconnections = 0.001F;	// avoid floating point underflow
	}
}

float CListenSocket::GetMaxConperFiveModifier()
{
	float SpikeSize = GetOpenSockets() - averageconnections;
	if (SpikeSize < 1.0F)
		return 1.0F;

	float SpikeTolerance = 25.0F * (float)thePrefs.GetMaxConperFive() / 10.0F;
	if (SpikeSize > SpikeTolerance)
		return 0;

	float Modifier = 1.0F - SpikeSize / SpikeTolerance;
	return Modifier;
}
