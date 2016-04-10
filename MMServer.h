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

class CMMSocket;
class CMMData;
class CMMPacket;
class CListenMMSocket;
class CSearchFile;
class CxImage;
class CKnownFile;
class CPartFile;

#define  MMS_BLOCKTIME	600000	
#define  MMS_SEARCHID	500

class CMMServer
{
public:
	CMMServer(void);
	~CMMServer(void);
	void	Init();
	void	StopServer();
	// packet processing
	bool	PreProcessPacket(char* pPacket, uint32 nSize, CMMSocket* sender);
	void	ProcessHelloPacket(CMMData* data, CMMSocket* sender);
	void	ProcessStatusRequest(CMMSocket* sender, CMMPacket* usePacket = NULL);
	void	ProcessFileListRequest(CMMSocket* sender, CMMPacket* usePacket = NULL);
	void	ProcessFileCommand(CMMData* data, CMMSocket* sender);
	void	ProcessDetailRequest(CMMData* data, CMMSocket* socket);
	void	ProcessCommandRequest(CMMData* data, CMMSocket* sender);
	void	ProcessSearchRequest(CMMData* data, CMMSocket* sender);
	void	ProcessPreviewRequest(CMMData* data, CMMSocket* sender);
	void	ProcessDownloadRequest(CMMData* data, CMMSocket* sender);
	void	ProcessChangeLimitRequest(CMMData* data, CMMSocket* sender);
	void	ProcessFinishedListRequest(CMMSocket* sender);
	void	ProcessStatisticsRequest(CMMData* data, CMMSocket* sender);
	// other
	void	SearchFinished(bool bTimeOut);
	void	PreviewFinished(CxImage** imgFrames, uint8 nCount);
	void	Process();
	void	AddFinishedFile(CKnownFile* file)	{m_SentFinishedList.Add(file);}
	CStringA GetContentType();

	UINT_PTR h_timer;
	uint8	m_byPendingCommand;
	CMMSocket*	m_pPendingCommandSocket;

protected:
	static VOID CALLBACK CommandTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime);
	void	DeleteSearchFiles();
	void	WriteFileInfo(CPartFile* selFile, CMMPacket* packet);

private:
	CListenMMSocket*	m_pSocket;
	uint16				m_nSessionID;
	CArray<CPartFile*,CPartFile*>		m_SentFileList;
	CArray<CSearchFile*, CSearchFile*>	m_SendSearchList;
	CArray<CKnownFile*,CKnownFile*>		m_SentFinishedList;
	uint8				m_cPWFailed;
	uint32				m_dwBlocked;
	bool				m_bUseFakeContent;
	uint16				m_nMaxDownloads;
	uint16				m_nMaxBufDownloads;
	bool				m_bGrabListLogin;
};