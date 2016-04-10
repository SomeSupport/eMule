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

#define CS_FATALERROR	-5
#define CS_DISCONNECTED	-4
#define CS_SERVERDEAD	-3
#define	CS_ERROR		-2
#define CS_SERVERFULL	-1
#define	CS_NOTCONNECTED	0
#define	CS_CONNECTING	1
#define	CS_CONNECTED	2
#define	CS_WAITFORLOGIN	3
#define CS_WAITFORPROXYLISTENING 4 // deadlake PROXYSUPPORT

#define CS_RETRYCONNECTTIME  30 // seconds

class CServerList;
class CUDPSocket;
class CServerSocket;
class CServer;
class Packet;

class CServerConnect
{
public:
	CServerConnect();
	~CServerConnect();

	void	ConnectionFailed(CServerSocket* sender);
	void	ConnectionEstablished(CServerSocket* sender);
	
	void	ConnectToAnyServer() { ConnectToAnyServer(0, true, true); }
	void	ConnectToAnyServer(UINT startAt, bool prioSort = false, bool isAuto = true, bool bNoCrypt = false);
	void	ConnectToServer(CServer* toconnect, bool multiconnect = false, bool bNoCrypt = false);
	void	StopConnectionTry();
	static  VOID CALLBACK RetryConnectTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime);

	void	CheckForTimeout();
	void	DestroySocket(CServerSocket* pSck);	// safe socket closure and destruction
	bool	SendPacket(Packet* packet,bool delpacket = true, CServerSocket* to = 0);
	bool	IsUDPSocketAvailable() const { return udpsocket != NULL; }
	bool	SendUDPPacket(Packet* packet,CServer* host, bool delpacket = false, uint16 nSpecialPort = 0, BYTE* pRawPacket = NULL, uint32 nLen = 0);
	void	KeepConnectionAlive();
	bool	Disconnect();
	bool	IsConnecting()		{ return connecting; }
	bool	IsConnected()		{ return connected; }
	uint32	GetClientID()		{ return clientid; }
	CServer* GetCurrentServer();

	uint32	clientid;
	uint8	pendingConnects;
	uint32	m_curuser;

	bool	IsLowID();
	void	SetClientID(uint32 newid);
	bool	IsLocalServer(uint32 dwIP, uint16 nPort);
	void	TryAnotherConnectionRequest();
	bool	IsSingleConnect()	{ return singleconnecting; }
	void	InitLocalIP();
	uint32	GetLocalIP()		{ return m_nLocalIP; }

	bool	AwaitingTestFromIP(uint32 dwIP) const;
	bool	IsConnectedObfuscated() const;

private:
	bool	connecting;
	bool	singleconnecting;
	bool	connected;
	bool	m_bTryObfuscated;
	uint8	max_simcons;
	UINT	m_uStartAutoConnectPos;
	CServerSocket* connectedsocket;
	CUDPSocket* udpsocket;
	CPtrList m_lstOpenSockets;	// list of currently opened sockets
	UINT	m_idRetryTimer;
	uint32	m_nLocalIP;
	CMap<ULONG, ULONG, CServerSocket*, CServerSocket*> connectionattemps;
};