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
#pragma once
#include "SafeFile.h"

class CMMServer;

//****** Outgoing Packets Class
class CMMPacket
{
public:
	CMMPacket(uint8 byOpcode)	{m_pBuffer = new CMemFile; m_pBuffer->Write(&byOpcode,1); m_bSpecialHeader = false;}
	~CMMPacket()				{delete m_pBuffer;}
	void	WriteByte(uint8 write)		{m_pBuffer->Write(&write,1);}
	void	WriteShort(uint16 write)	{m_pBuffer->Write(&write,2);}
	void	WriteInt(uint32 write)		{m_pBuffer->Write(&write,4);}
	void	WriteInt64(uint64 write)	{m_pBuffer->Write(&write,8);}
	void	WriteString(CStringA write){
		uint8 len = (write.GetLength() > 255) ? (uint8)255 : (uint8)write.GetLength();
		WriteByte(len);
		m_pBuffer->Write(const_cast<LPSTR>((LPCSTR)write), len);
	}
	void	WriteString(CString write){
		CStringA strA(write);
		WriteString(strA);
	}
	CMemFile* m_pBuffer;
	bool	  m_bSpecialHeader;
};

//****** Incoming Packets Class
class CMMData: public CSafeMemFile
{
public:
	CMMData(char* pData,uint32 nSize):CSafeMemFile((BYTE*)pData,nSize)	{}

	uint8	ReadByte(){
		uint8 buf;
		Read(&buf,1);
		return buf;
	}
	uint16	ReadShort(){
		uint16 buf;
		Read(&buf,2);
		return buf;
	}
	uint32	ReadInt(){
		uint32 buf;
		Read(&buf,4);
		return buf;
	}
	uint64	ReadInt64(){
		uint64 buf;
		Read(&buf,8);
		return buf;
	}
	CString ReadString(){
		uint8 buf;
		char str[256];
		buf = ReadByte();
		Read(str,buf);
		return CString(str,buf);
	}
};

//****** Socket
class CMMSocket: public CAsyncSocket
{
public:
	CMMSocket(CMMServer* pOwner);
	~CMMSocket(void);
	bool	SendPacket(CMMPacket* packet, bool bQueueFirst = false);
	bool	m_bClosed;
	uint32	m_dwTimedShutdown;
protected:
	void	OnReceive(int nErrorCode);
	void	OnClose(int nErrorCode);
	void	Close();
	void	OnRequestReceived(char* pHeader, DWORD dwHeaderLen, char* pData, DWORD dwDataLen);
	void	OnSend(int nErrorCode);
	void	CheckForClosing();
private:
	char* m_pBuf;
	DWORD m_dwRecv;
	DWORD m_dwBufSize;
	DWORD m_dwHttpHeaderLen;
	DWORD m_dwHttpContentLen;
	CMMServer* m_pOwner;
	char*	  m_pSendBuffer;
	uint32	  m_nSendLen;
	uint32	  m_nSent;

	CTypedPtrList<CPtrList, CMMPacket*> m_PacketQueue;
};

//****** Listening Socket
class CListenMMSocket: public CAsyncSocket
{
public:
	CListenMMSocket(CMMServer* pOwner);
	~CListenMMSocket(void);
	bool	Create();
	void	Process();
	virtual void OnAccept(int nErrorCode);
protected:
	void	DeleteClosedSockets();
private:
	CMMServer* m_pOwner;
	CTypedPtrList<CPtrList, CMMSocket*> m_socket_list;
};




//opcodes
#define MMP_HELLO			0x01
#define MMP_HELLOANS		0x02
#define	MMP_INVALIDID		0x03
#define	MMP_GENERALERROR	0x04
#define	MMP_STATUSREQ		0x05
#define	MMP_STATUSANSWER	0x06
#define	MMP_FILELISTREQ		0x07
#define	MMP_FILELISTANS		0x08
#define	MMP_FILECOMMANDREQ	0x09
#define	MMP_FILECOMMANDANS	0x10
#define	MMP_FILEDETAILREQ	0x11
#define	MMP_FILEDETAILANS	0x12
#define	MMP_COMMANDREQ		0x13
#define	MMP_COMMANDANS		0x14
#define	MMP_SEARCHREQ		0x15
#define	MMP_SEARCHANS		0x16
#define	MMP_DOWNLOADREQ		0x17
#define	MMP_DOWNLOADANS		0x18
#define MMP_PREVIEWREQ		0x19
#define MMP_PREVIEWANS		0x20
#define MMP_FINISHEDREQ		0x21
#define MMP_FINISHEDANS		0x22
#define MMP_CHANGELIMIT		0x23
#define MMP_CHANGELIMITANS	0x24
#define MMP_STATISTICSREQ	0x25
#define MMP_STATISTICSANS	0x26

// tags
#define	MMT_OK				0x01
#define	MMT_WRONGVERSION	0x02
#define	MMT_WRONGPASSWORD	0x03
#define	MMT_FAILED			0x00
#define MMT_PAUSED			0x00
#define MMT_WAITING			0x01
#define MMT_DOWNLOADING		0x02

#define	MMT_PAUSE			0x03
#define MMT_RESUME			0x02
#define MMT_CANCEL			0x01

#define MMT_SDEMULE			0x01
#define MMT_SDPC			0x02
#define MMT_SERVERCONNECT	0x03
#define MMT_SEARCH			0x30
#define MMT_PREVIEW			0x40

// OK = 0x01
#define MMT_NOTCONNECTED	0x02
#define MMT_TIMEDOUT		0x03
#define MMT_NORESULTS		0x04

// failed = 0x00
// OK = 0x01
#define MMT_NOTAVAILABLE	0x02
#define MMT_NOTSUPPORTED	0x03

#define MMT_PARTFILFE		0x01
#define MMT_FINISHEDFILE	0x02

#define MM_VERSION			0x9B
#define MM_STRVERSION		"0.9x"
