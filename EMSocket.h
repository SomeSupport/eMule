//this file is part of eMule
//Copyright (C)2002-2010 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include "EncryptedStreamSocket.h"
#include "OtherFunctions.h"
#include "ThrottledSocket.h" // ZZ:UploadBandWithThrottler (UDP)

class CAsyncProxySocketLayer;
class Packet;

#define ES_DISCONNECTED		0xFF
#define ES_NOTCONNECTED		0x00
#define ES_CONNECTED		0x01

#define PACKET_HEADER_SIZE	6

struct StandardPacketQueueEntry {
	uint32 actualPayloadSize;
	Packet* packet;
};

class CEMSocket : public CEncryptedStreamSocket, public ThrottledFileSocket // ZZ:UploadBandWithThrottler
{
	DECLARE_DYNAMIC(CEMSocket)
public:
	CEMSocket();
	virtual ~CEMSocket();

	virtual void 	SendPacket(Packet* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0, bool bForceImmediateSend = false);
	bool	IsConnected() const {return byConnected == ES_CONNECTED;}
	uint8	GetConState() const {return byConnected;}
	virtual bool IsRawDataMode() const { return false; }
	void	SetDownloadLimit(uint32 limit);
	void	DisableDownloadLimit();
	BOOL	AsyncSelect(long lEvent);
	virtual bool IsBusyExtensiveCheck();
	virtual bool IsBusyQuickCheck() const;
    virtual bool HasQueues(bool bOnlyStandardPackets = false) const;
	virtual bool IsEnoughFileDataQueued(uint32 nMinFilePayloadBytes) const;
	virtual bool UseBigSendBuffer();
	int			 DbgGetStdQueueCount() const	{return standartpacket_queue.GetCount();}

	virtual UINT GetTimeOut() const;
	virtual void SetTimeOut(UINT uTimeOut);

	virtual BOOL Connect(LPCSTR lpszHostAddress, UINT nHostPort);
	virtual BOOL Connect(SOCKADDR* pSockAddr, int iSockAddrLen);

	void InitProxySupport();
	virtual void RemoveAllLayers();
	const CString GetLastProxyError() const { return m_strLastProxyError; }
	bool GetProxyConnectFailed() const { return m_bProxyConnectFailed; }

	CString GetFullErrorMessage(DWORD dwError);

	DWORD GetLastCalledSend() { return lastCalledSend; }
	uint64 GetSentBytesCompleteFileSinceLastCallAndReset();
	uint64 GetSentBytesPartFileSinceLastCallAndReset();
	uint64 GetSentBytesControlPacketSinceLastCallAndReset();
	uint64 GetSentPayloadSinceLastCall(bool bReset);
	void TruncateQueues();

    virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) { return Send(maxNumberOfBytesToSend, minFragSize, true); };
    virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) { return Send(maxNumberOfBytesToSend, minFragSize, false); };

    uint32	GetNeededBytes();
#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual int	OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nCode, WPARAM wParam, LPARAM lParam);
	
	virtual void	DataReceived(const BYTE* pcData, UINT uSize);
	virtual bool	PacketReceived(Packet* packet) = 0;
	virtual void	OnError(int nErrorCode) = 0;
	virtual void	OnClose(int nErrorCode);
	virtual void	OnSend(int nErrorCode);
	virtual void	OnReceive(int nErrorCode);
	uint8	byConnected;
	UINT	m_uTimeOut;
	bool	m_bProxyConnectFailed;
	CAsyncProxySocketLayer* m_pProxyLayer;
	CString m_strLastProxyError;

private:
    virtual SocketSentBytes Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket);
	SocketSentBytes SendStd(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket);
	SocketSentBytes SendOv(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket);
	void	ClearQueues();
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
	void	CleanUpOverlappedSendOperation(bool bCancelRequestFirst);

    uint32 GetNextFragSize(uint32 current, uint32 minFragSize);
    bool    HasSent() { return m_hasSent; }

	// Download (pseudo) rate control
	uint32	downloadLimit;
	bool	downloadLimitEnable;
	bool	pendingOnReceive;

	// Download partial header
	char	pendingHeader[PACKET_HEADER_SIZE];	// actually, this holds only 'PACKET_HEADER_SIZE-1' bytes.
	uint32	pendingHeaderSize;

	// Download partial packet
	Packet* pendingPacket;
	uint32	pendingPacketSize;

	// Upload control
	char*	sendbuffer;
	uint32	sendblen;
	uint32	sent;
	LPWSAOVERLAPPED m_pPendingSendOperation;
	CArray<WSABUF> m_aBufferSend;

	CTypedPtrList<CPtrList, Packet*> controlpacket_queue;
	CList<StandardPacketQueueEntry> standartpacket_queue;
	bool m_currentPacket_is_controlpacket;
	CCriticalSection sendLocker;
	uint64 m_numberOfSentBytesCompleteFile;
	uint64 m_numberOfSentBytesPartFile;
	uint64 m_numberOfSentBytesControlPacket;
	bool m_currentPackageIsFromPartFile;
	bool m_bAccelerateUpload;
	DWORD lastCalledSend;
    DWORD lastSent;
	uint32 lastFinishedStandard;
	uint32 m_actualPayloadSize;			// Payloadsize of the data currently in sendbuffer
	uint32 m_actualPayloadSizeSent;
    bool m_bBusy;
    bool m_hasSent;
	bool m_bUsesBigSendBuffers;
	bool m_bOverlappedSending;
};
