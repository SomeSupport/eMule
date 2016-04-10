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
#include <wininet.h>
#include "UrlClient.h"
#include "emule.h"
#include "PartFile.h"
#include "Packets.h"
#include "ListenSocket.h"
#include "HttpClientReqSocket.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "Statistics.h"
#include "ClientCredits.h"
#include "Clientlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// CUrlClient

IMPLEMENT_DYNAMIC(CUrlClient, CUpDownClient)

CUrlClient::CUrlClient()
{
	m_iRedirected = 0;
	m_clientSoft = SO_URL;
}

void CUrlClient::SetRequestFile(CPartFile* pReqFile)
{
	CUpDownClient::SetRequestFile(pReqFile);
	if (reqfile)
	{
		m_nPartCount = reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		memset(m_abyPartStatus, 1, m_nPartCount);
		m_bCompleteSource = true;
	}
}

bool CUrlClient::SetUrl(LPCTSTR pszUrl, uint32 nIP)
{
	TCHAR szCanonUrl[INTERNET_MAX_URL_LENGTH];
	DWORD dwCanonUrlSize = ARRSIZE(szCanonUrl);
	if (!InternetCanonicalizeUrl(pszUrl, szCanonUrl, &dwCanonUrlSize, ICU_NO_ENCODE))
		return false;

	TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
	DWORD dwUrlSize = ARRSIZE(szUrl);
	if (!InternetCanonicalizeUrl(szCanonUrl, szUrl, &dwUrlSize, ICU_DECODE | ICU_NO_ENCODE | ICU_BROWSER_MODE))
		return false;

	TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
	TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
	TCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH];
	TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
	TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
	TCHAR szExtraInfo[INTERNET_MAX_URL_LENGTH];
	URL_COMPONENTS Url = {0};
	Url.dwStructSize = sizeof(Url);
	Url.lpszScheme = szScheme;
	Url.dwSchemeLength = ARRSIZE(szScheme);
	Url.lpszHostName = szHostName;
	Url.dwHostNameLength = ARRSIZE(szHostName);
	Url.lpszUserName = szUserName;
	Url.dwUserNameLength = ARRSIZE(szUserName);
	Url.lpszPassword = szPassword;
	Url.dwPasswordLength = ARRSIZE(szPassword);
	Url.lpszUrlPath = szUrlPath;
	Url.dwUrlPathLength = ARRSIZE(szUrlPath);
	Url.lpszExtraInfo = szExtraInfo;
	Url.dwExtraInfoLength = ARRSIZE(szExtraInfo);
	if (!InternetCrackUrl(szUrl, 0, 0, &Url))
		return false;

	if (Url.dwSchemeLength == 0 || Url.nScheme != INTERNET_SCHEME_HTTP)		// we only support "http://"
		return false;
	if (Url.dwHostNameLength == 0)			// we must know the hostname
		return false;
	if (Url.dwUserNameLength != 0)			// no support for user/password
		return false;
	if (Url.dwPasswordLength != 0)			// no support for user/password
		return false;
	if (Url.dwUrlPathLength == 0)			// we must know the URL path on that host
		return false;

	m_strHostA = szHostName;

	TCHAR szEncodedUrl[INTERNET_MAX_URL_LENGTH];
	DWORD dwEncodedUrl = ARRSIZE(szEncodedUrl);
	if (!InternetCanonicalizeUrl(szUrl, szEncodedUrl, &dwEncodedUrl, ICU_ENCODE_PERCENT))
		return false;
	m_strUrlPath = szEncodedUrl;
	m_nUrlStartPos = (uint64)-1;

	SetUserName(szUrl);

	//NOTE: be very careful with what is stored in the following IP/ID/Port members!
	if (nIP)
		m_nConnectIP = nIP;
	else
		m_nConnectIP = inet_addr(CT2A(szHostName));
//	if (m_nConnectIP == INADDR_NONE)
//		m_nConnectIP = 0;
	m_nUserIDHybrid = htonl(m_nConnectIP);
	ASSERT( m_nUserIDHybrid != 0 );
	m_nUserPort = Url.nPort;
	return true;
}

CUrlClient::~CUrlClient()
{
}

void CUrlClient::SendBlockRequests()
{
	ASSERT(0);
}

bool CUrlClient::SendHttpBlockRequests()
{
	m_dwLastBlockReceived = ::GetTickCount();
	if (reqfile == NULL)
		throw CString(_T("Failed to send block requests - No 'reqfile' attached"));

	CreateBlockRequests(PARTSIZE / EMBLOCKSIZE, PARTSIZE / EMBLOCKSIZE);
	if (m_PendingBlocks_list.IsEmpty()){
		SetDownloadState(DS_NONEEDEDPARTS);
        SwapToAnotherFile(_T("A4AF for NNP file. UrlClient::SendHttpBlockRequests()"), true, false, false, NULL, true, true);
		return false;
	}
	
	POSITION pos = m_PendingBlocks_list.GetHeadPosition();
	Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
	m_uReqStart = pending->block->StartOffset;
	m_uReqEnd = pending->block->EndOffset;
	bool bMergeBlocks = true;
	while (pos)
	{
		POSITION posLast = pos;
		pending = m_PendingBlocks_list.GetNext(pos);
		if (bMergeBlocks && pending->block->StartOffset == m_uReqEnd + 1)
			m_uReqEnd = pending->block->EndOffset;
		else
		{
			bMergeBlocks = false;
			reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
			delete pending->block;
			delete pending;
			m_PendingBlocks_list.RemoveAt(posLast);
		}
	}

	m_nUrlStartPos = m_uReqStart;

	CStringA strHttpRequest;
	strHttpRequest.AppendFormat("GET %s HTTP/1.0\r\n", m_strUrlPath);
	strHttpRequest.AppendFormat("Accept: */*\r\n");
	strHttpRequest.AppendFormat("Range: bytes=%I64u-%I64u\r\n", m_uReqStart, m_uReqEnd);
	strHttpRequest.AppendFormat("Connection: Keep-Alive\r\n");
	strHttpRequest.AppendFormat("Host: %s\r\n", m_strHostA);
	strHttpRequest.AppendFormat("\r\n");

	if (thePrefs.GetDebugClientTCPLevel() > 0)
		Debug(_T("Sending HTTP request:\n%hs"), strHttpRequest);
	CRawPacket* pHttpPacket = new CRawPacket(strHttpRequest);
	theStats.AddUpDataOverheadFileRequest(pHttpPacket->size);
	socket->SendPacket(pHttpPacket);
	STATIC_DOWNCAST(CHttpClientDownSocket, socket)->SetHttpState(HttpStateRecvExpected);
	return true;
}

bool CUrlClient::TryToConnect(bool bIgnoreMaxCon, bool bNoCallbacks, CRuntimeClass* /*pClassSocket*/)
{
	return CUpDownClient::TryToConnect(bIgnoreMaxCon, bNoCallbacks, RUNTIME_CLASS(CHttpClientDownSocket));
}

void CUrlClient::Connect()
{
	if (GetConnectIP() != 0 && GetConnectIP() != INADDR_NONE){
		CUpDownClient::Connect();
		return; 
	}
	//Try to always tell the socket to WaitForOnConnect before you call Connect.
	socket->WaitForOnConnect();
	socket->Connect(m_strHostA, m_nUserPort);
	return;
}

void CUrlClient::OnSocketConnected(int nErrorCode)
{
	if (nErrorCode == 0)
		ConnectionEstablished();
}

void CUrlClient::ConnectionEstablished()
{
	m_nConnectingState = CCS_NONE;
	theApp.clientlist->RemoveConnectingClient(this);
	SendHttpBlockRequests();
	SetDownStartTime();
}

void CUrlClient::SendHelloPacket()
{
	//SendHttpBlockRequests();
	return;
}

void CUrlClient::SendFileRequest()
{
	// This may be called in some rare situations depending on socket states.
	; // just ignore it
}

bool CUrlClient::Disconnected(LPCTSTR pszReason, bool bFromSocket)
{
	CHttpClientDownSocket* s = STATIC_DOWNCAST(CHttpClientDownSocket, socket);

	TRACE(_T("%hs: HttpState=%u, Reason=%s\n"), __FUNCTION__, s==NULL ? -1 : s->GetHttpState(), pszReason);
	// TODO: This is a mess..
	if (s && (s->GetHttpState() == HttpStateRecvExpected || s->GetHttpState() == HttpStateRecvBody))
        m_fileReaskTimes.RemoveKey(reqfile); // ZZ:DownloadManager (one resk timestamp for each file)
	return CUpDownClient::Disconnected(CString(_T("CUrlClient::Disconnected")) + pszReason, bFromSocket);
}

bool CUrlClient::ProcessHttpDownResponse(const CStringAArray& astrHeaders)
{
	if (reqfile == NULL)
		throw CString(_T("Failed to process received HTTP data block - No 'reqfile' attached"));
	if (astrHeaders.GetCount() == 0)
		throw CString(_T("Unexpected HTTP response - No headers available"));

	const CStringA& rstrHdr = astrHeaders.GetAt(0);
	UINT uHttpMajVer, uHttpMinVer, uHttpStatusCode;
	if (sscanf(rstrHdr, "HTTP/%u.%u %u", &uHttpMajVer, &uHttpMinVer, &uHttpStatusCode) != 3){
		CString strError;
		strError.Format(_T("Unexpected HTTP response: \"%hs\""), rstrHdr);
		throw strError;
	}
	if (uHttpMajVer != 1 || (uHttpMinVer != 0 && uHttpMinVer != 1)){
		CString strError;
		strError.Format(_T("Unexpected HTTP version: \"%hs\""), rstrHdr);
		throw strError;
	}
	bool bExpectData = uHttpStatusCode == HTTP_STATUS_OK || uHttpStatusCode == HTTP_STATUS_PARTIAL_CONTENT;
	bool bRedirection = uHttpStatusCode == HTTP_STATUS_MOVED || uHttpStatusCode == HTTP_STATUS_REDIRECT;
	if (!bExpectData && !bRedirection){
		CString strError;
		strError.Format(_T("Unexpected HTTP status code \"%u\""), uHttpStatusCode);
		throw strError;
	}

	bool bNewLocation = false;
	bool bValidContentRange = false;
	for (int i = 1; i < astrHeaders.GetCount(); i++)
	{
		const CStringA& rstrHdr = astrHeaders.GetAt(i);
		if (bExpectData && _strnicmp(rstrHdr, "Content-Length:", 15) == 0)
		{
			uint64 uContentLength = _atoi64((LPCSTR)rstrHdr + 15);
			if (uContentLength != m_uReqEnd - m_uReqStart + 1){
				if (uContentLength != reqfile->GetFileSize()){ // tolerate this case only
					CString strError;
					strError.Format(_T("Unexpected HTTP header field \"%hs\""), rstrHdr);
					throw strError;
				}
				TRACE("+++ Unexpected HTTP header field \"%s\"\n", rstrHdr);
			}
		}
		else if (bExpectData && _strnicmp(rstrHdr, "Content-Range:", 14) == 0)
		{
			uint64 ui64Start = 0, ui64End = 0, ui64Len = 0;
			if (sscanf((LPCSTR)rstrHdr + 14," bytes %I64u - %I64u / %I64u", &ui64Start, &ui64End, &ui64Len) != 3){
				CString strError;
				strError.Format(_T("Unexpected HTTP header field \"%hs\""), rstrHdr);
				throw strError;
			}

			if (ui64Start != m_uReqStart || ui64End != m_uReqEnd || ui64Len != reqfile->GetFileSize()){
				CString strError;
				strError.Format(_T("Unexpected HTTP header field \"%hs\""), rstrHdr);
				throw strError;
			}
			bValidContentRange = true;
		}
		else if (_strnicmp(rstrHdr, "Server:", 7) == 0)
		{
			if (m_strClientSoftware.IsEmpty())
				m_strClientSoftware = rstrHdr.Mid(7).Trim();
		}
		else if (bRedirection && _strnicmp(rstrHdr, "Location:", 9) == 0)
		{
			CString strLocation(rstrHdr.Mid(9).Trim());
			if (!SetUrl(strLocation)){
				CString strError;
				strError.Format(_T("Failed to process HTTP redirection URL \"%s\""), strLocation);
				throw strError;
			}
			bNewLocation = true;
		}
	}

	if (bNewLocation)
	{
		m_iRedirected++;
		if (m_iRedirected >= 3)
			throw CString(_T("Max. HTTP redirection count exceeded"));

		// the tricky part
		socket->Safe_Delete();		// mark our parent object for getting deleted!
		if (!TryToConnect(true))	// replace our parent object with a new one
			throw CString(_T("Failed to connect to redirected URL"));
		return false;				// tell our old parent object (which was marked as to get deleted 
									// and which is no longer attached to us) to disconnect.
	}

	if (!bValidContentRange){
		if (thePrefs.GetDebugClientTCPLevel() <= 0)
			DebugHttpHeaders(astrHeaders);
		CString strError;
		strError.Format(_T("Unexpected HTTP response - No valid HTTP content range found"));
		throw strError;
	}

	SetDownloadState(DS_DOWNLOADING);
	return true;
}

bool CUpDownClient::ProcessHttpDownResponse(const CStringAArray& )
{
	ASSERT(0);
	return false;
}

bool CUpDownClient::ProcessHttpDownResponseBody(const BYTE* pucData, UINT uSize)
{
	ProcessHttpBlockPacket(pucData, uSize);
	return true;
}

void CUpDownClient::ProcessHttpBlockPacket(const BYTE* pucData, UINT uSize)
{
	if (reqfile == NULL)
		throw CString(_T("Failed to process HTTP data block - No 'reqfile' attached"));

	if (reqfile->IsStopped() || (reqfile->GetStatus() != PS_READY && reqfile->GetStatus() != PS_EMPTY))
		throw CString(_T("Failed to process HTTP data block - File not ready for receiving data"));

	if (m_nUrlStartPos == (uint64)-1)
		throw CString(_T("Failed to process HTTP data block - Unexpected file data"));

	uint64 nStartPos = m_nUrlStartPos;
	uint64 nEndPos = m_nUrlStartPos + uSize;

	m_nUrlStartPos += uSize;

//	if (thePrefs.GetDebugClientTCPLevel() > 0)
//		Debug("  Start=%I64u  End=%I64u  Size=%u  %s\n", nStartPos, nEndPos, size, DbgGetFileInfo(reqfile->GetFileHash()));

	if (!(GetDownloadState() == DS_DOWNLOADING || GetDownloadState() == DS_NONEEDEDPARTS))
		throw CString(_T("Failed to process HTTP data block - Invalid download state"));

	m_dwLastBlockReceived = ::GetTickCount();

	if (nEndPos == nStartPos || uSize != nEndPos - nStartPos)
		throw CString(_T("Failed to process HTTP data block - Invalid block start/end offsets"));

	thePrefs.Add2SessionTransferData(GetClientSoft(), (GetClientSoft()==SO_URL) ? (UINT)-2 : (UINT)-1, false, false, uSize);
	m_nDownDataRateMS += uSize;
	if (credits)
		credits->AddDownloaded(uSize, GetIP());
	nEndPos--;

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != NULL; )
	{
		POSITION posLast = pos;
		Pending_Block_Struct *cur_block = m_PendingBlocks_list.GetNext(pos);
		if (cur_block->block->StartOffset <= nStartPos && nStartPos <= cur_block->block->EndOffset)
		{
			if (thePrefs.GetDebugClientTCPLevel() > 0){
				// NOTE: 'Left' is only accurate in case we have one(!) request block!
				void* p = m_pPCDownSocket ? (void*)m_pPCDownSocket : (void*)socket;
				Debug(_T("%08x  Start=%I64u  End=%I64u  Size=%u  Left=%I64u  %s\n"), p, nStartPos, nEndPos, uSize, cur_block->block->EndOffset - (nStartPos + uSize) + 1, DbgGetFileInfo(reqfile->GetFileHash()));
			}

			m_nLastBlockOffset = nStartPos;
			uint32 lenWritten = reqfile->WriteToBuffer(uSize, pucData, nStartPos, nEndPos, cur_block->block, this);
			if (lenWritten > 0)
			{
				m_nTransferredDown += uSize;
                m_nCurSessionPayloadDown += lenWritten;
				SetTransferredDownMini();

				if (nEndPos >= cur_block->block->EndOffset)
				{
					reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
					delete cur_block->block;
					delete cur_block;
					m_PendingBlocks_list.RemoveAt(posLast);

					if (m_PendingBlocks_list.IsEmpty())
					{
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("More block requests", this);
						m_nUrlStartPos = (uint64)-1;
						SendHttpBlockRequests();
					}
				}
//				else
//					TRACE("%hs - %d bytes missing\n", __FUNCTION__, cur_block->block->EndOffset - nEndPos);
			}

			return;
		}
	}

	TRACE("%s - Dropping packet\n", __FUNCTION__);
}

void CUrlClient::SendCancelTransfer(Packet* /*packet*/)
{
	if (socket)
	{
		STATIC_DOWNCAST(CHttpClientDownSocket, socket)->SetHttpState(HttpStateUnknown);
		socket->Safe_Delete();
	}
}
