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
#pragma once
#include "AbstractFile.h"

class CFileDataIO;
class CxImage;

class CSearchFile : public CAbstractFile
{
	DECLARE_DYNAMIC(CSearchFile)

	friend class CPartFile;
	friend class CSearchListCtrl;
public:
	CSearchFile(CFileDataIO* in_data, bool bOptUTF8, uint32 nSearchID,
				uint32 nServerIP=0, uint16 nServerPort=0,
				LPCTSTR pszDirectory = NULL, 
				bool nKademlia = false, bool bServerUDPAnswer = false);
	CSearchFile(const CSearchFile* copyfrom);
	virtual ~CSearchFile();

	bool	IsKademlia() const				{ return m_bKademlia; }
	bool	IsServerUDPAnswer() const		{ return m_bServerUDPAnswer; }
	uint32	AddSources(uint32 count);
	uint32	GetSourceCount() const;
	uint32	AddCompleteSources(uint32 count);
	uint32	GetCompleteSourceCount() const;
	int		IsComplete() const;
	int		IsComplete(UINT uSources, UINT uCompleteSources) const;
	time_t	GetLastSeenComplete() const;
	uint32	GetSearchID() const { return m_nSearchID; }
	LPCTSTR GetDirectory() const { return m_pszDirectory; }

	uint32	GetClientID() const				{ return m_nClientID; }
	void	SetClientID(uint32 nClientID)	{ m_nClientID = nClientID; }
	uint16	GetClientPort() const			{ return m_nClientPort; }
	void	SetClientPort(uint16 nPort)		{ m_nClientPort = nPort; }
	uint32	GetClientServerIP() const		{ return m_nClientServerIP; }
	void	SetClientServerIP(uint32 uIP)   { m_nClientServerIP = uIP; }
	uint16	GetClientServerPort() const		{ return m_nClientServerPort; }
	void	SetClientServerPort(uint16 nPort) { m_nClientServerPort = nPort; }
	int		GetClientsCount() const			{ return ((GetClientID() && GetClientPort()) ? 1 : 0) + m_aClients.GetSize(); }
	void	SetKadPublishInfo(uint32 dwVal)	{ m_nKadPublishInfo = dwVal; }
	uint32	GetKadPublishInfo() const		{ return m_nKadPublishInfo; } // == TAG_PUBLISHINFO
	bool	DidFoundMultipleAICH() const	{ return m_bMultipleAICHFound; }
	void	SetFoundMultipleAICH()			{ m_bMultipleAICHFound = true; }

	// Spamfilter
	void	SetNameWithoutKeyword(CString strName)	{ m_strNameWithoutKeywords = strName; }
	CString	GetNameWithoutKeyword()	const			{ return m_strNameWithoutKeywords; }
	void	SetSpamRating(uint32 nRating)			{ m_nSpamRating = nRating; }
	uint32	GetSpamRating() const					{ return m_nSpamRating; }
	bool	IsConsideredSpam() const;

	virtual void	UpdateFileRatingCommentAvail(bool bForceUpdate = false);

	// GUI helpers
	CSearchFile* GetListParent() const		{ return m_list_parent; }
	void		 SetListParent(CSearchFile* parent) { m_list_parent = parent; }
	UINT		 GetListChildCount() const	{ return m_list_childcount; }
	void		 SetListChildCount(int cnt)	{ m_list_childcount = cnt; }
	void		 AddListChildCount(int cnt) { m_list_childcount += cnt; }
	bool		 IsListExpanded() const		{ return m_list_bExpanded; }
	void		 SetListExpanded(bool val)	{ m_list_bExpanded = val; }

	void		 StoreToFile(CFileDataIO& rFile) const;

	struct SClient {
		SClient() {
			m_nIP = 0;
			m_nPort = 0;
			m_nServerIP = 0;
			m_nServerPort = 0;
		}
		SClient(uint32 nIP, uint16 nPort, uint32 nServerIP, uint16 nServerPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_nServerIP = nServerIP;
			m_nServerPort = nServerPort;
		}
		friend __inline bool __stdcall operator==(const CSearchFile::SClient& c1, const CSearchFile::SClient& c2) {
			return c1.m_nIP==c2.m_nIP && c1.m_nPort==c2.m_nPort &&
				   c1.m_nServerIP==c2.m_nServerIP && c1.m_nServerPort==c2.m_nServerPort;
		}
		uint32 m_nIP;
		uint32 m_nServerIP;
		uint16 m_nPort;
		uint16 m_nServerPort;
	};
	void AddClient(const SClient& client) { m_aClients.Add(client); }
	const CSimpleArray<SClient>& GetClients() const { return m_aClients; }

	struct SServer {
		SServer() {
			m_nIP = 0;
			m_nPort = 0;
			m_uAvail = 0;
			m_bUDPAnswer = false;
		}
		SServer(uint32 nIP, uint16 nPort, bool bUDPAnswer) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_uAvail = 0;
			m_bUDPAnswer = bUDPAnswer;
		}
		friend __inline bool __stdcall operator==(const CSearchFile::SServer& s1, const CSearchFile::SServer& s2) {
			return s1.m_nIP==s2.m_nIP && s1.m_nPort==s2.m_nPort;
		}
		uint32 m_nIP;
		uint16 m_nPort;
		UINT   m_uAvail;
		bool   m_bUDPAnswer;
	};
	void AddServer(const SServer& server) { m_aServers.Add(server); }
	const CSimpleArray<SServer>& GetServers() const { return m_aServers; }
	SServer& GetServerAt(int iServer) { return m_aServers[iServer]; }
	
	void	AddPreviewImg(CxImage* img)	{	m_listImages.Add(img); }
	const CSimpleArray<CxImage*>& GetPreviews() const { return m_listImages; }
	bool	IsPreviewPossible() const { return m_bPreviewPossible;}
	void	SetPreviewPossible(bool in)	{ m_bPreviewPossible = in; }

	enum EKnownType
	{
		NotDetermined,
		Shared,
		Downloading,
		Downloaded,
		Cancelled,
		Unknown
	};

	EKnownType GetKnownType() const { return m_eKnown; }
	void SetKnownType(EKnownType eType) { m_eKnown = eType; }

private:
	bool	m_bMultipleAICHFound;
	bool	m_bKademlia;
	bool	m_bServerUDPAnswer;
	uint32	m_nClientID;
	uint16	m_nClientPort;
	uint32	m_nSearchID;
	uint32	m_nClientServerIP;
	uint16	m_nClientServerPort;
	uint32	m_nKadPublishInfo;
	CSimpleArray<SClient> m_aClients;
	CSimpleArray<SServer> m_aServers;
	CSimpleArray<CxImage*> m_listImages;
	LPTSTR m_pszDirectory;
	// spamfilter
	CString	m_strNameWithoutKeywords;
	uint32	m_nSpamRating;

	// GUI helpers
	bool		m_bPreviewPossible;
	bool		m_list_bExpanded;
	UINT		m_list_childcount;
	CSearchFile*m_list_parent;
	EKnownType	m_eKnown;
};

bool IsValidSearchResultClientIPPort(uint32 nIP, uint16 nPort);
