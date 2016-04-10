//this file is part of eMule
//Copyright (C)2003 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "mmsocket.h"
#include "MMServer.h"
#include "Preferences.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CMMSocket::CMMSocket(CMMServer* pOwner)
{
	m_pOwner = pOwner;
	m_pBuf = NULL;
	m_dwRecv = 0;
	m_dwBufSize = 0;
	m_dwHttpHeaderLen = 0;
	m_dwHttpContentLen = 0;
	m_bClosed = false;
	m_pSendBuffer = NULL;
	m_nSendLen = 0;
	m_nSent = 0;
	m_dwTimedShutdown = 0;
}

CMMSocket::~CMMSocket(void)
{
	delete[] m_pBuf;
	while (!m_PacketQueue.IsEmpty()){
		delete m_PacketQueue.RemoveHead();
	}
	delete[] m_pSendBuffer;
}

void CMMSocket::Close(){
	if (m_hSocket != INVALID_SOCKET && m_hSocket != NULL){
		this->AsyncSelect(0);
		CAsyncSocket::Close();
	}
	m_bClosed = true;
}

void CMMSocket::OnClose(int /*nErrorCode*/)
{
	m_bClosed = true;
	if (m_pOwner->m_pPendingCommandSocket == this){
		m_pOwner->m_pPendingCommandSocket = NULL;
	}
}

void CMMSocket::OnReceive(int nErrorCode){
	static char GlobalReadBuffer[10240];
	if(nErrorCode != 0){
		return;
	}
	const UINT SIZE_PRESERVE = 0x1000;
	uint32 readMax = sizeof(GlobalReadBuffer); 
	uint32 dwSize = Receive(GlobalReadBuffer, readMax);
	if(dwSize == SOCKET_ERROR || dwSize == 0){
		return;
	}
	if (m_dwBufSize < dwSize + m_dwRecv)
	{
		// reallocate
		char* pNewBuf = new char[m_dwBufSize = dwSize + m_dwRecv + SIZE_PRESERVE];
		if (!pNewBuf)
		{
			shutdown(m_hSocket, SD_BOTH);
			Close();
			return;
		}

		if (m_pBuf)
		{
			CopyMemory(pNewBuf, m_pBuf, m_dwRecv);
			delete[] m_pBuf;
		}

		m_pBuf = pNewBuf;
	}
	CopyMemory(m_pBuf + m_dwRecv, GlobalReadBuffer, dwSize);
	m_dwRecv += dwSize;

	// check if we have all that we want
	if (!m_dwHttpHeaderLen)
	{
		// try to find it
		bool bPrevEndl = false;
		for (DWORD dwPos = 0; dwPos < m_dwRecv; dwPos++)
			if ('\n' == m_pBuf[dwPos])
				if (bPrevEndl)
				{
					// We just found the end of the http header
					// Now write the message's position into two first DWORDs of the buffer
					m_dwHttpHeaderLen = dwPos + 1;

					for (dwPos = 0; dwPos < m_dwHttpHeaderLen; )
					{
						char* pPtr = (char*)memchr(m_pBuf + dwPos, '\n', m_dwHttpHeaderLen - dwPos);
						if (!pPtr)
							break;
						DWORD dwNextPos = pPtr - m_pBuf;

						// check this header
						char szMatch[] = "content-length";
						if (!_strnicmp(m_pBuf + dwPos, szMatch, sizeof(szMatch) - 1))
						{
							dwPos += sizeof(szMatch) - 1;
							pPtr = (char*)memchr(m_pBuf + dwPos, ':', m_dwHttpHeaderLen - dwPos);
							if (pPtr)
								m_dwHttpContentLen = atol((pPtr) + 1);

							break;
						}
						dwPos = dwNextPos + 1;
					}

					break;
				}
				else
				{
					bPrevEndl = true;
				}
			else
				if ('\r' != m_pBuf[dwPos])
					bPrevEndl = false;
	}
	if (m_dwHttpHeaderLen && !m_dwHttpContentLen)
		m_dwHttpContentLen = m_dwRecv - m_dwHttpHeaderLen;
	if (m_dwHttpHeaderLen && m_dwHttpContentLen < m_dwRecv && (!m_dwHttpContentLen || (m_dwHttpHeaderLen + m_dwHttpContentLen <= m_dwRecv)))
	{
		OnRequestReceived(m_pBuf, m_dwHttpHeaderLen, m_pBuf + m_dwHttpHeaderLen, m_dwHttpContentLen);

		if (m_dwRecv > m_dwHttpHeaderLen + m_dwHttpContentLen)
		{
			// move our data
			m_dwRecv -= m_dwHttpHeaderLen + m_dwHttpContentLen;
			MoveMemory(m_pBuf, m_pBuf + m_dwHttpHeaderLen + m_dwHttpContentLen, m_dwRecv);
		} else
			m_dwRecv = 0;

		m_dwHttpHeaderLen = 0;
		m_dwHttpContentLen = 0;
	}
}

bool CMMSocket::SendPacket(CMMPacket* packet, bool bQueueFirst){
	if (m_pSendBuffer != NULL){
		m_PacketQueue.AddTail(packet);
		return false;
	}
	else{
		char szBuf[0x1000];
		int nLen;
		if (!packet->m_bSpecialHeader)
			nLen = _snprintf(szBuf, _countof(szBuf), "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",m_pOwner->GetContentType(), (uint32)packet->m_pBuffer->GetLength());
		else
			nLen = _snprintf(szBuf, _countof(szBuf), "Content-Length: %ld\r\n\r\n", (uint32)packet->m_pBuffer->GetLength());
		m_nSendLen = nLen + (UINT)packet->m_pBuffer->GetLength();
		m_pSendBuffer =	new char[m_nSendLen];
		memcpy(m_pSendBuffer,szBuf,nLen);
		packet->m_pBuffer->SeekToBegin();
		packet->m_pBuffer->Read(m_pSendBuffer+nLen, (UINT)packet->m_pBuffer->GetLength()); 
		
		m_nSent = Send(m_pSendBuffer,m_nSendLen);
		if (m_nSent == SOCKET_ERROR){
			delete[] m_pSendBuffer;
			m_pSendBuffer = NULL;
			m_nSendLen = 0;
			if (GetLastError() == WSAEWOULDBLOCK){
				if (bQueueFirst)
					m_PacketQueue.AddHead(packet);
				else
					m_PacketQueue.AddTail(packet);
			}
			else{
				delete packet;
				Close();
			}
			return false;
		}
		else{
			if (m_nSent == m_nSendLen){
				delete[] m_pSendBuffer;
				m_pSendBuffer = NULL;
				m_nSendLen = 0;
				delete packet;
				CheckForClosing();
				return true;
			}
			else{
				delete packet;
				return false;
			}
			
		}
	}
}

void CMMSocket::CheckForClosing()
{
	if (m_nSendLen == 0 && m_PacketQueue.IsEmpty() && !m_bClosed){
		m_dwTimedShutdown = ::GetTickCount() + 1000;
	}
}

void CMMSocket::OnSend(int /*nErrorCode*/)
{
	if(m_pSendBuffer != NULL){
		uint32 res = Send(m_pSendBuffer+m_nSent,m_nSendLen-m_nSent);
		if (res == SOCKET_ERROR){
			if (GetLastError() != WSAEWOULDBLOCK)
				Close();
			return;
		}
		else{
			m_nSent += res;
			if (m_nSent >= m_nSendLen){
				delete[] m_pSendBuffer;
				m_pSendBuffer = NULL;
				m_nSendLen = 0;
				CheckForClosing();
			}
			else
				return;
		}
	}
	while (!m_PacketQueue.IsEmpty()){
		CMMPacket* packet = m_PacketQueue.RemoveHead();
		if (!SendPacket(packet,true))
			return;
	}
}

void CMMSocket::OnRequestReceived(char* pHeader, DWORD dwHeaderLen, char* pData, DWORD dwDataLen)
{
	CString sHeader(pHeader, dwHeaderLen);
	if(sHeader.Left(4) != _T("POST"))
		return;
	if (!m_pOwner->PreProcessPacket(pData, dwDataLen, this))
		return;
	try{
		if (dwDataLen > 3){

			try{
				CMMData data(pData+3,dwDataLen-3);
				switch(pData[0]){
					case MMP_HELLO:
						m_pOwner->ProcessHelloPacket(&data,this);
						break;
					case MMP_FILECOMMANDREQ:
						m_pOwner->ProcessFileCommand(&data,this);
						break;
					case MMP_FILEDETAILREQ:
						m_pOwner->ProcessDetailRequest(&data,this);
						break;
					case MMP_COMMANDREQ:
						m_pOwner->ProcessCommandRequest(&data,this);
						break;
					case MMP_SEARCHREQ:
						m_pOwner->ProcessSearchRequest(&data,this);
						break;
					case MMP_DOWNLOADREQ:
						m_pOwner->ProcessDownloadRequest(&data,this);
						break;
					case MMP_PREVIEWREQ:
						m_pOwner->ProcessPreviewRequest(&data,this);
						break;
					case MMP_CHANGELIMIT:
						m_pOwner->ProcessChangeLimitRequest(&data,this);
						break;
					case MMP_STATISTICSREQ:
						m_pOwner->ProcessStatisticsRequest(&data,this);
						break;
				}
			}
			catch(CFileException* error){
				ASSERT ( false ); // remove later
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Corrupt MobileMule Packet received"));
				error->Delete();
			}
		}
		else{
			switch(pData[0]){
				case MMP_STATUSREQ:
					m_pOwner->ProcessStatusRequest(this);
					break;
				case MMP_FILELISTREQ:
					m_pOwner->ProcessFileListRequest(this);
					break;
				case MMP_FINISHEDREQ:
					m_pOwner->ProcessFinishedListRequest(this);
					break;
			}
		}
	}
	catch(...){
		ASSERT ( false ); // remove later
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Unexpected Error while processing MobileMule Packet"));
	}

}

// *************** Listener ****************************

CListenMMSocket::CListenMMSocket(CMMServer* pOwner)
{
	m_pOwner = pOwner;
}

CListenMMSocket::~CListenMMSocket(void)
{
	while(!m_socket_list.IsEmpty())
		delete m_socket_list.RemoveHead();
}

bool  CListenMMSocket::Create(){
	return CAsyncSocket::Create(thePrefs.GetMMPort(),SOCK_STREAM,FD_ACCEPT) && Listen();;
}


void CListenMMSocket::OnAccept(int nErrorCode){
	if (!nErrorCode){
		CMMSocket* newclient = new CMMSocket(m_pOwner);
			if (!Accept(*newclient))
				delete newclient;
			else{
				newclient->AsyncSelect(FD_WRITE|FD_READ|FD_CLOSE);
				m_socket_list.AddTail(newclient);
				/*LINGER linger = { 1, 7 };
				VERIFY(newclient->SetSockOpt(SO_LINGER,&linger, sizeof(linger), SOL_SOCKET));
				DeleteClosedSockets();*/
			}
	}
}

void CListenMMSocket::DeleteClosedSockets(){
	POSITION pos2,pos1;
	for(pos1 = m_socket_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
       m_socket_list.GetNext(pos1);
	   CMMSocket* cur_sock = m_socket_list.GetAt(pos2);
	   if ( cur_sock->m_bClosed){
			m_socket_list.RemoveAt(pos2);
			delete cur_sock;
	   }
	   else if (cur_sock->m_dwTimedShutdown && cur_sock->m_dwTimedShutdown < ::GetTickCount()){
		   cur_sock->ShutDown(SD_SEND);
		   cur_sock->m_dwTimedShutdown = 0;
	   }
   }
}

void CListenMMSocket::Process(){
	DeleteClosedSockets();
}