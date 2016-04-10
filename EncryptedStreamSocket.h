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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

/* This class supports obfuscation and encryption for a eMule tcp connection.
   Right now only basic obfuscation is supported, but this can be expanded, as their is a
   dedicated handshake to negotiate the encryption method used.

   Please note, even if obfuscation uses encryption methods, it does not fulfill cryptographic standards since it
   doesn't use secret (and for rc4 important: unique) keys
*/

#pragma once
#include "AsyncSocketEx.h"

// cryptoPP used for DH integer calculations
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#include <crypto51/integer.h>
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data



#define ERR_WRONGHEADER				0x01
#define ERR_TOOBIG					0x02
#define ERR_ENCRYPTION				0x03
#define ERR_ENCRYPTION_NOTALLOWED	0x04

enum EStreamCryptState {
	ECS_NONE = 0,			// Disabled or not available
	ECS_UNKNOWN,			// Incoming connection, will test the first incoming data for encrypted protocol
	ECS_PENDING,			// Outgoing connection, will start sending encryption protocol
	ECS_PENDING_SERVER,		// Outgoing serverconnection, will start sending encryption protocol
	ECS_NEGOTIATING,		// Encryption supported, handshake still uncompleted
	ECS_ENCRYPTING			// Encryption enabled
};

enum ENegotiatingState {
	ONS_NONE,
	
	ONS_BASIC_CLIENTA_RANDOMPART,
	ONS_BASIC_CLIENTA_MAGICVALUE,
	ONS_BASIC_CLIENTA_METHODTAGSPADLEN,
	ONS_BASIC_CLIENTA_PADDING,
	
	ONS_BASIC_CLIENTB_MAGICVALUE,
	ONS_BASIC_CLIENTB_METHODTAGSPADLEN,
	ONS_BASIC_CLIENTB_PADDING,

	ONS_BASIC_SERVER_DHANSWER,
	ONS_BASIC_SERVER_MAGICVALUE,
	ONS_BASIC_SERVER_METHODTAGSPADLEN,
	ONS_BASIC_SERVER_PADDING,
	ONS_BASIC_SERVER_DELAYEDSENDING,

	ONS_COMPLETE
};

enum EEncryptionMethods {
	ENM_OBFUSCATION = 0x00
};

class CSafeMemFile;
struct RC4_Key_Struct;

class CEncryptedStreamSocket : public CAsyncSocketEx
{
	DECLARE_DYNAMIC(CEncryptedStreamSocket)
public:
	CEncryptedStreamSocket();
	virtual ~CEncryptedStreamSocket();

	void	SetConnectionEncryption(bool bEnabled, const uchar* pTargetClientHash, bool bServerConnection);
	uint32	GetRealReceivedBytes() const		{ return m_nObfuscationBytesReceived; } // indicates how many bytes were received including obfusication so that the parent knows if the receive limit was reached
	bool	IsObfusicating() const				{ return m_StreamCryptState == ECS_ENCRYPTING && m_EncryptionMethod == ENM_OBFUSCATION; }
	
	bool	IsServerCryptEnabledConnection() const { return m_bServerCrypt; }	

	uint8	m_dbgbyEncryptionSupported;
	uint8	m_dbgbyEncryptionRequested;
	uint8	m_dbgbyEncryptionMethodSet;

protected:
	int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	int SendOv(CArray<WSABUF>& raBuffer, DWORD& dwBytesSent, LPWSAOVERLAPPED lpOverlapped);
	int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
	virtual void	OnError(int nErrorCode) = 0;
	virtual void	OnSend(int nErrorCode);
	CString			DbgGetIPString();
	void			CryptPrepareSendData(uchar* pBuffer, uint32 nLen);
	bool			IsEncryptionLayerReady();
	uint8			GetSemiRandomNotProtocolMarker() const;
	
	uint32	m_nObfuscationBytesReceived;
	EStreamCryptState	m_StreamCryptState;
	EEncryptionMethods  m_EncryptionMethod;
	bool	m_bFullReceive;
	bool	m_bServerCrypt;

private:
	int		Negotiate(const uchar* pBuffer, uint32 nLen);
	void	StartNegotiation(bool bOutgoing);
	int		SendNegotiatingData(const void* lpBuf, uint32 nBufLen, uint32 nStartCryptFromByte = 0, bool bDelaySend = false);

	RC4_Key_Struct*		m_pRC4SendKey;
	RC4_Key_Struct*		m_pRC4ReceiveKey;
	ENegotiatingState	m_NegotiatingState;
	CSafeMemFile*		m_pfiReceiveBuffer;
	uint32				m_nReceiveBytesWanted;
	CSafeMemFile*		m_pfiSendBuffer;
	uint32				m_nRandomKeyPart;
	CryptoPP::Integer	m_cryptDHA;

};