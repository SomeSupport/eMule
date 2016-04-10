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
#pragma once

class Packet;

typedef enum EHttpSocketState
{
	HttpStateUnknown = 0,
	HttpStateRecvExpected,
	HttpStateRecvHeaders,
	HttpStateRecvBody
};

///////////////////////////////////////////////////////////////////////////////
// CHttpClientReqSocket

class CHttpClientReqSocket : public CClientReqSocket
{
	DECLARE_DYNCREATE(CHttpClientReqSocket)

public:
	virtual CUpDownClient* GetClient() const { return client; }

	virtual void SendPacket(Packet* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0, bool bForceImmediateSend = false);
	virtual bool IsRawDataMode() const { return true; }

	EHttpSocketState GetHttpState() const { return m_eHttpState; }
	void SetHttpState(EHttpSocketState eState);
	void ClearHttpHeaders();

protected:
	CHttpClientReqSocket(CUpDownClient* client = NULL);
	virtual ~CHttpClientReqSocket();

	virtual void DataReceived(const BYTE* pucData, UINT uSize);
	virtual void OnConnect(int nErrorCode);

	EHttpSocketState	m_eHttpState;
	CStringA			m_strHttpCurHdrLine;
	CStringAArray		m_astrHttpHeaders;
	int					m_iHttpHeadersSize;

	bool ProcessHttpPacket(const BYTE* packet, UINT size);
	void ProcessHttpHeaderPacket(const char* packet, UINT size, LPBYTE& pBody, int& iSizeBody);

	virtual bool ProcessHttpResponse();
	virtual bool ProcessHttpResponseBody(const BYTE* pucData, UINT size);
	virtual bool ProcessHttpRequest();
};


///////////////////////////////////////////////////////////////////////////////
// CHttpClientDownSocket

class CHttpClientDownSocket : public CHttpClientReqSocket
{
	DECLARE_DYNCREATE(CHttpClientDownSocket)

public:
	CHttpClientDownSocket(CUpDownClient* client = NULL);

protected:
	virtual ~CHttpClientDownSocket();

	virtual bool ProcessHttpResponse();
	virtual bool ProcessHttpResponseBody(const BYTE* pucData, UINT size);
	virtual bool ProcessHttpRequest();
};
