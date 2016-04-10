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
#include "emule.h"
#include "SearchFile.h"
#include "SearchList.h"
#include "SearchParams.h"
#include "Packets.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "UpDownClient.h"
#include "SafeFile.h"
#include "MMServer.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "DownloadQueue.h"
#include "PartFile.h"
#include "CxImage/xImage.h"
#include "kademlia/utils/uint128.h"
#include "Kademlia/Kademlia/Entry.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "emuledlg.h"
#include "SearchDlg.h"
#include "SearchListCtrl.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SPAMFILTER_FILENAME		_T("SearchSpam.met")
#define STOREDSEARCHES_FILENAME	_T("StoredSearches.met")
#define STOREDSEARCHES_VERSION	1
///////////////////////////////////////////////////////////////////////////////
// CSearchList

CSearchList::CSearchList()
{
	outputwnd = NULL;
	m_MobilMuleSearch = false;
	m_bSpamFilterLoaded = false;
}

CSearchList::~CSearchList()
{
	Clear();
	POSITION pos = m_aUDPServerRecords.GetStartPosition();
	while( pos != NULL )
	{
		uint32 dwIP;
		UDPServerRecord* pRecord;
		m_aUDPServerRecords.GetNextAssoc(pos, dwIP, pRecord);
		delete pRecord;
	}
	m_aUDPServerRecords.RemoveAll();
}

void CSearchList::Clear()
{
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL;)
	{
		POSITION posLast = pos;
		SearchListsStruct* listCur = m_listFileLists.GetNext(pos);
		for(POSITION pos2 = listCur->m_listSearchFiles.GetHeadPosition(); pos2 != NULL; )
			delete listCur->m_listSearchFiles.GetNext(pos2);
		listCur->m_listSearchFiles.RemoveAll();

		m_listFileLists.RemoveAt(posLast);
		delete listCur;
	}
}

void CSearchList::RemoveResults(uint32 nSearchID)
{
	// this will not delete the item from the window, make sure your code does it if you call this
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL;)
	{
		POSITION posLast = pos;
		SearchListsStruct* listCur =	m_listFileLists.GetNext(pos);
		if (listCur->m_nSearchID == nSearchID)
		{
			for(POSITION pos2 = listCur->m_listSearchFiles.GetHeadPosition(); pos2 != NULL; )
				delete listCur->m_listSearchFiles.GetNext(pos2);
			listCur->m_listSearchFiles.RemoveAll();

			m_listFileLists.RemoveAt(posLast);
			delete listCur;
			return;
		}
	}
}

void CSearchList::ShowResults(uint32 nSearchID)
{
	ASSERT( outputwnd );
	outputwnd->SetRedraw(FALSE);
	CMuleListCtrl::EUpdateMode bCurUpdateMode = outputwnd->SetUpdateMode(CMuleListCtrl::none/*direct*/);

	SearchList* list = GetSearchListForID(nSearchID);
	for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
	{
		const CSearchFile* cur_file = list->GetNext(pos);
		ASSERT( cur_file->GetSearchID() == nSearchID );
		if (cur_file->GetListParent() == NULL)
		{
			outputwnd->AddResult(cur_file);
			if (cur_file->IsListExpanded() && cur_file->GetListChildCount() > 0)
				outputwnd->UpdateSources(cur_file);
		}
	}

	outputwnd->SetUpdateMode(bCurUpdateMode);
	outputwnd->SetRedraw(TRUE);
}

void CSearchList::RemoveResult(CSearchFile* todel)
{
	SearchList* list = GetSearchListForID(todel->GetSearchID());
	for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
	{
		POSITION posLast = pos;
		CSearchFile* cur_file = list->GetNext(pos);
		if (cur_file == todel)
		{
			theApp.emuledlg->searchwnd->RemoveResult(todel);
			list->RemoveAt(posLast);
			delete todel;
			return;
		}
	}
}

void CSearchList::NewSearch(CSearchListCtrl* pWnd, CStringA strResultFileType, uint32 nSearchID, ESearchType eSearchType, CString strSearchExpression, bool bMobilMuleSearch)
{
	if (pWnd)
		outputwnd = pWnd;

	m_strResultFileType = strResultFileType;
	ASSERT( eSearchType != SearchTypeAutomatic );
	if (eSearchType == SearchTypeEd2kServer || eSearchType == SearchTypeEd2kGlobal){
		m_nCurED2KSearchID = nSearchID;
		m_aCurED2KSentRequestsIPs.RemoveAll();
		m_aCurED2KSentReceivedIPs.RemoveAll();
	}
	m_foundFilesCount.SetAt(nSearchID, 0);
	m_foundSourcesCount.SetAt(nSearchID, 0);
	m_ReceivedUDPAnswersCount.SetAt(nSearchID, 0);
	m_RequestedUDPAnswersCount.SetAt(nSearchID, 0);
	m_MobilMuleSearch = bMobilMuleSearch;

	// convert the expression into an array of searchkeywords which the user has typed in
	// this is used for the spamfilter later and not at all semantically equal with the actual search expression anymore
	m_astrSpamCheckCurSearchExp.RemoveAll();
	strSearchExpression.MakeLower();
	if (strSearchExpression.Find(_T("related:"), 0) != 0){ // ignore special searches
		int nPos, nPos2;
		while ((nPos = strSearchExpression.Find(_T('"'))) != -1 && (nPos2 = strSearchExpression.Find(_T('"'), nPos + 1)) != -1){
			CString strQuoted = strSearchExpression.Mid(nPos + 1, (nPos2 - nPos) - 1);
			m_astrSpamCheckCurSearchExp.Add(strQuoted);
			strSearchExpression.Delete(nPos, (nPos2 - nPos) + 1);
		}
		strSearchExpression.Replace(_T("AND"), _T(""));
		strSearchExpression.Replace(_T("OR"), _T(""));
		strSearchExpression.Replace(_T("NOT"), _T(""));
		nPos = 0;
		CString strToken = strSearchExpression.Tokenize(_T(".[]()!-'_ "), nPos);
		while (!strToken.IsEmpty()){
			m_astrSpamCheckCurSearchExp.Add(strToken);
			strToken = strSearchExpression.Tokenize(_T(".[]()!-'_ "), nPos);
		}
	}

}

UINT CSearchList::ProcessSearchAnswer(const uchar* in_packet, uint32 size,
									  CUpDownClient* Sender, bool* pbMoreResultsAvailable, LPCTSTR pszDirectory)
{
	ASSERT( Sender != NULL );
	uint32 nSearchID = (uint32)Sender;
	SSearchParams* pParams = new SSearchParams;
	pParams->strExpression = Sender->GetUserName();
	pParams->dwSearchID = nSearchID;
	pParams->bClientSharedFiles = true;
	if (theApp.emuledlg->searchwnd->CreateNewTab(pParams)){
		m_foundFilesCount.SetAt(nSearchID, 0);
		m_foundSourcesCount.SetAt(nSearchID, 0);
	}
	else{
		delete pParams;
		pParams = NULL;
	}

	CSafeMemFile packet(in_packet, size);
	UINT results = packet.ReadUInt32();
	for (UINT i = 0; i < results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, Sender ? Sender->GetUnicodeSupport()!=utf8strNone : false, nSearchID, 0, 0, pszDirectory);
		if (toadd->IsLargeFile() && (Sender == NULL || !Sender->SupportsLargeFiles())){
			DebugLogWarning(_T("Client offers large file (%s) but doesn't announced support for it - ignoring file"), toadd->GetFileName());
			delete toadd;
			continue;
		}
		if (Sender){
			toadd->SetClientID(Sender->GetIP());
			toadd->SetClientPort(Sender->GetUserPort());
			toadd->SetClientServerIP(Sender->GetServerIP());
			toadd->SetClientServerPort(Sender->GetServerPort());
			if (Sender->GetServerIP() && Sender->GetServerPort()){
				CSearchFile::SServer server(Sender->GetServerIP(), Sender->GetServerPort(), false);
				server.m_uAvail = 1;
				toadd->AddServer(server);
			}			
			toadd->SetPreviewPossible( Sender->GetPreviewSupport() && ED2KFT_VIDEO == GetED2KFileTypeID(toadd->GetFileName()) );
		}
		AddToList(toadd, true);
	}

	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1){
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = ucMore!=0;
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				Debug(_T("  Client search answer(%s): More=%u\n"), Sender->GetUserName(), ucMore);
		}
		else{
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				Debug(_T("*** NOTE: Client ProcessSearchAnswer(%s): ***AddData: 1 byte: 0x%02x\n"), Sender->GetUserName(), ucMore);
		}
	}
	else if (iAddData > 0){
		if (thePrefs.GetDebugClientTCPLevel() > 0){
			Debug(_T("*** NOTE: Client ProcessSearchAnswer(%s): ***AddData: %u bytes\n"), Sender->GetUserName(), iAddData);
			DebugHexDump(in_packet + packet.GetPosition(), iAddData);
		}
	}

	packet.Close();
	return GetResultCount(nSearchID);
}

UINT CSearchList::ProcessSearchAnswer(const uchar* in_packet, uint32 size, bool bOptUTF8,
									  uint32 nServerIP, uint16 nServerPort, bool* pbMoreResultsAvailable)
{
	CSafeMemFile packet(in_packet, size);
	UINT results = packet.ReadUInt32();
	for (UINT i = 0; i < results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, bOptUTF8, m_nCurED2KSearchID);
		toadd->SetClientServerIP(nServerIP);
		toadd->SetClientServerPort(nServerPort);
		if (nServerIP && nServerPort){
			CSearchFile::SServer server(nServerIP, nServerPort, false);
			server.m_uAvail = toadd->GetIntTagValue(FT_SOURCES);
			toadd->AddServer(server);
		}
		AddToList(toadd, false);
	}
	if (m_MobilMuleSearch)
		theApp.mmserver->SearchFinished(false);
	m_MobilMuleSearch = false;

	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1){
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = ucMore!=0;
			if (thePrefs.GetDebugServerTCPLevel() > 0)
				Debug(_T("  Search answer(Server %s:%u): More=%u\n"), ipstr(nServerIP), nServerPort, ucMore);
		}
		else{
			if (thePrefs.GetDebugServerTCPLevel() > 0)
				Debug(_T("*** NOTE: ProcessSearchAnswer(Server %s:%u): ***AddData: 1 byte: 0x%02x\n"), ipstr(nServerIP), nServerPort, ucMore);
		}
	}
	else if (iAddData > 0){
		if (thePrefs.GetDebugServerTCPLevel() > 0){
			Debug(_T("*** NOTE: ProcessSearchAnswer(Server %s:%u): ***AddData: %u bytes\n"), ipstr(nServerIP), nServerPort, iAddData);
			DebugHexDump(in_packet + packet.GetPosition(), iAddData);
		}
	}

	packet.Close();
	return GetED2KResultCount();
}

UINT CSearchList::ProcessUDPSearchAnswer(CFileDataIO& packet, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort)
{
	CSearchFile* toadd = new CSearchFile(&packet, bOptUTF8, m_nCurED2KSearchID, nServerIP, nServerPort, NULL, false, true);
	
	bool bFound = false;
	for (int i = 0; i != m_aCurED2KSentRequestsIPs.GetCount(); i++){
		if (m_aCurED2KSentRequestsIPs[i] == nServerIP){
			bFound = true;
			break;
		}
	}
	if (!bFound){
		DebugLogError(_T("Unrequested or delayed Server UDP Searchresult received from IP %s, ignoring"), ipstr(nServerIP));
		delete toadd;
		return 0;
	}

	bool bNewResponse = true;
	for (int i = 0; i != m_aCurED2KSentReceivedIPs.GetCount(); i++){
		if (m_aCurED2KSentReceivedIPs[i] == nServerIP){
			bNewResponse = false;
			break;
		}
	}
	if (bNewResponse){
		uint32 nResponses = 0;
		VERIFY( m_ReceivedUDPAnswersCount.Lookup(m_nCurED2KSearchID, nResponses) );
		m_ReceivedUDPAnswersCount.SetAt(m_nCurED2KSearchID, nResponses + 1);
		m_aCurED2KSentReceivedIPs.Add(nServerIP);
	}

	UDPServerRecord* pRecord = NULL;
	m_aUDPServerRecords.Lookup(nServerIP, pRecord);
	if (pRecord == NULL){
		pRecord = new UDPServerRecord;
		pRecord->m_nResults = 1;
		pRecord->m_nSpamResults = 0;
		m_aUDPServerRecords.SetAt(nServerIP, pRecord);
	}
	else
		pRecord->m_nResults++;
			
	
	AddToList(toadd, false, nServerIP);
	return GetED2KResultCount();
}

UINT CSearchList::GetResultCount(uint32 nSearchID) const
{
	UINT nSources = 0;
	VERIFY( m_foundSourcesCount.Lookup(nSearchID, nSources) );
	return nSources;
}

UINT CSearchList::GetED2KResultCount() const
{
	return GetResultCount(m_nCurED2KSearchID);
}

void CSearchList::GetWebList(CQArray<SearchFileStruct, SearchFileStruct> *SearchFileArray, int iSortBy) const
{
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL;)
	{
		SearchListsStruct* listCur = m_listFileLists.GetNext(pos);
		for(POSITION pos2 = listCur->m_listSearchFiles.GetHeadPosition(); pos2 != NULL; )
		{
			const CSearchFile* pFile = listCur->m_listSearchFiles.GetNext(pos2);
			if (pFile == NULL || pFile->GetListParent() != NULL || pFile->GetFileSize() == (uint64)0 || pFile->GetFileName().IsEmpty())
				continue;

			SearchFileStruct structFile;
			structFile.m_strFileName = pFile->GetFileName();
			structFile.m_strFileType = pFile->GetFileTypeDisplayStr();
			structFile.m_strFileHash = md4str(pFile->GetFileHash());
			structFile.m_uSourceCount = pFile->GetSourceCount();
			structFile.m_dwCompleteSourceCount = pFile->GetCompleteSourceCount();
			structFile.m_uFileSize = pFile->GetFileSize();

			switch (iSortBy)
			{
				case 0:
					structFile.m_strIndex = structFile.m_strFileName;
					break;
				case 1:
					structFile.m_strIndex.Format(_T("%10u"), structFile.m_uFileSize);
					break;
				case 2:
					structFile.m_strIndex = structFile.m_strFileHash;
					break;
				case 3:
					structFile.m_strIndex.Format(_T("%09u"), structFile.m_uSourceCount);
					break;
				case 4:
					structFile.m_strIndex = structFile.m_strFileType;
					break;
				default:
					structFile.m_strIndex.Empty();
			}
			SearchFileArray->Add(structFile);
		}
	}
}

void CSearchList::AddFileToDownloadByHash(const uchar* hash, int cat)
{
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL;)
	{
		const SearchListsStruct* listCur = m_listFileLists.GetNext(pos);
		for(POSITION pos2 = listCur->m_listSearchFiles.GetHeadPosition(); pos2 != NULL; )
		{
			CSearchFile* sf = listCur->m_listSearchFiles.GetNext(pos2);
			if (!md4cmp(hash, sf->GetFileHash())){
				theApp.downloadqueue->AddSearchToDownload(sf, 2, cat);
				return;
			}
		}
	}
}

// mobilemule
CSearchFile* CSearchList::DetachNextFile(uint32 nSearchID)
{
	// the files are NOT deleted, make sure you do this if you call this function
	// find, removes and returns the searchresult with most Sources	
	uint32 nHighSource = 0;
	POSITION resultpos = 0;

	SearchList* list = GetSearchListForID(nSearchID);
	for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
	{
		POSITION cur_pos = pos;
		CSearchFile* cur_file = list->GetNext(pos);
		ASSERT( cur_file->GetSearchID() == nSearchID );
		if (cur_file->GetIntTagValue(FT_SOURCES) >= nHighSource)
		{
			nHighSource = cur_file->GetIntTagValue(FT_SOURCES);
			resultpos = cur_pos;
		}
	}
	if (resultpos == 0){
		ASSERT ( false );
		return NULL;
	}
	CSearchFile* result = list->GetAt(resultpos);
	list->RemoveAt(resultpos);
	return result;
}

bool CSearchList::AddToList(CSearchFile* toadd, bool bClientResponse, uint32 dwFromUDPServerIP)
{
	if (!bClientResponse && !m_strResultFileType.IsEmpty() && _tcscmp(m_strResultFileType, toadd->GetFileType()) != 0)
	{
		delete toadd;
		return false;
	}
	SearchList* list = GetSearchListForID(toadd->GetSearchID());

	// Spamfilter: Calculate the filename without any used keywords (and seperators) for later use
	CString strNameWithoutKeyword;
	CString strName = toadd->GetFileName();
	strName.MakeLower();
	int nPos = 0;
	CString strToken = strName.Tokenize(_T(".[]()!-'_ "), nPos);
	bool bFound;
	while (!strToken.IsEmpty()){
		bFound = false;
		if (!bClientResponse && toadd->GetSearchID() == m_nCurED2KSearchID){
			for (int i = 0; i < m_astrSpamCheckCurSearchExp.GetCount(); i++){
				if (strToken.Compare(m_astrSpamCheckCurSearchExp[i]) == 0){
					bFound = true;
					break;
				}
			}
		}
		if (!bFound){
			if (!strNameWithoutKeyword.IsEmpty())
				strNameWithoutKeyword += _T(" ");
			strNameWithoutKeyword += strToken;
		}
		strToken = strName.Tokenize(_T(".[]()!-'_ "), nPos);
	}
	toadd->SetNameWithoutKeyword(strNameWithoutKeyword);

	// search for a 'parent' with same filehash and search-id as the new search result entry
	for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
	{
		CSearchFile* parent = list->GetNext(pos);
		if (   parent->GetListParent() == NULL
			&& md4cmp(parent->GetFileHash(), toadd->GetFileHash()) == 0)
		{

			// if this parent does not yet have any child entries, create one child entry 
			// which is equal to the current parent entry (needed for GUI when expanding the child list).
			if (parent->GetListChildCount() == 0)
			{
				CSearchFile* child = new CSearchFile(parent);
				child->SetListParent(parent);
				int iSources = parent->GetIntTagValue(FT_SOURCES);
				if (iSources == 0)
					iSources = 1;
				child->SetListChildCount(iSources);
				list->AddTail(child);
				parent->SetListChildCount(1);
			}

			// get the 'Availability' of the new search result entry
			UINT uAvail;
			if (bClientResponse) {
				// If this is a response from a client ("View Shared Files"), we set the "Availability" at least to 1.
				if (!toadd->GetIntTagValue(FT_SOURCES, uAvail) || uAvail==0)
					uAvail = 1;
			}
			else
				uAvail = toadd->GetIntTagValue(FT_SOURCES);


			// get 'Complete Sources' of the new search result entry
			uint32 uCompleteSources = (uint32)-1;
			bool bHasCompleteSources = toadd->GetIntTagValue(FT_COMPLETE_SOURCES, uCompleteSources);

			bool bFound = false;
			if (thePrefs.GetDebugSearchResultDetailLevel() >= 1)
			{
				; // for debugging: do not merge search results
			}
			else
			{
				// check if that parent already has a child with same filename as the new search result entry
				for (POSITION pos2 = list->GetHeadPosition(); pos2 != NULL && !bFound; )
				{
					CSearchFile* child = list->GetNext(pos2);
					if (    child != toadd													// not the same object
						&& child->GetListParent() == parent								// is a child of our result (one filehash)
						&& toadd->GetFileName().CompareNoCase(child->GetFileName()) == 0)	// same name
					{
						bFound = true;

						// add properties of new search result entry to the already available child entry (with same filename)
						// ed2k: use the sum of all values, kad: use the max. values
						if (toadd->IsKademlia()) {
							if (uAvail > child->GetListChildCount())
								child->SetListChildCount(uAvail);
						}
						else {
							child->AddListChildCount(uAvail);
						}
						child->AddSources(uAvail);
						if (bHasCompleteSources)
							child->AddCompleteSources(uCompleteSources);

						// Check AICH Hash - if they differ, clear it (see KademliaSearchKeyword)
						//					 if we didn't have a hash yet, take it over
						if (toadd->GetFileIdentifier().HasAICHHash())
						{
							if (child->GetFileIdentifier().HasAICHHash())
							{
								if (parent->GetFileIdentifier().GetAICHHash() != toadd->GetFileIdentifier().GetAICHHash())
								{
									DEBUG_ONLY(DebugLogWarning(_T("Kad: SearchList: AddToList: Received searchresult with different AICH hash than existing one, ignoring AICH for result %s"), child->GetFileName()) );
									child->SetFoundMultipleAICH();
									child->GetFileIdentifier().ClearAICHHash();
								}
							}
							else if (!child->DidFoundMultipleAICH())
							{
								DEBUG_ONLY(DebugLog(_T("Kad: SearchList: AddToList: Received searchresult with new AICH hash %s, taking over to existing result. Entry: %s"), toadd->GetFileIdentifier().GetAICHHash().GetString(), child->GetFileName()) );
								child->GetFileIdentifier().SetAICHHash(toadd->GetFileIdentifier().GetAICHHash());
							}
						}
						break;
					}
				}
			}
			if (!bFound)
			{
				// the parent which we had found does not yet have a child with that new search result's entry name,
				// add the new entry as a new child
				//
				toadd->SetListParent(parent);
				toadd->SetListChildCount(uAvail);
				parent->AddListChildCount(1);
				list->AddHead(toadd);
			}

			// copy possible available sources from new search result entry to parent
			if (toadd->GetClientID() && toadd->GetClientPort())
			{
				if (IsValidSearchResultClientIPPort(toadd->GetClientID(), toadd->GetClientPort()))
				{
					// pre-filter sources which would be dropped in CPartFile::AddSources
					if (CPartFile::CanAddSource(toadd->GetClientID(), toadd->GetClientPort(), toadd->GetClientServerIP(), toadd->GetClientServerPort()))
					{
						CSearchFile::SClient client(toadd->GetClientID(), toadd->GetClientPort(),
													toadd->GetClientServerIP(), toadd->GetClientServerPort());
						if (parent->GetClients().Find(client) == -1)
							parent->AddClient(client);
					}
				}
				else
				{
					if (thePrefs.GetDebugServerSearchesLevel() > 1)
					{
						uint32 nIP = toadd->GetClientID();
						Debug(_T("Filtered source from search result %s:%u\n"), DbgGetClientID(nIP), toadd->GetClientPort());
					}
				}
			}

			// copy possible available servers from new search result entry to parent
			// will be used in future
			if (toadd->GetClientServerIP() && toadd->GetClientServerPort())
			{
				CSearchFile::SServer server(toadd->GetClientServerIP(), toadd->GetClientServerPort(), toadd->IsServerUDPAnswer());
				int iFound = parent->GetServers().Find(server);
				if (iFound == -1) {
					server.m_uAvail = uAvail;
					parent->AddServer(server);
				}
				else
					parent->GetServerAt(iFound).m_uAvail += uAvail;
			}

			UINT uAllChildsSourceCount = 0;			// ed2k: sum of all sources, kad: the max. sources found
			UINT uAllChildsCompleteSourceCount = 0; // ed2k: sum of all sources, kad: the max. sources found
			UINT uDifferentNames = 0; // max known different names
			UINT uPublishersKnown = 0; // max publishers known (might be changed to median)
			UINT uTrustValue = 0; // average trust value (might be changed to median)
			uint32 nPublishInfoTags = 0;
			const CSearchFile* bestEntry = NULL;
			bool bHasMultipleAICHHashs = false;
			CAICHHash aichHash;
			bool bAICHHashValid = false;
			for (POSITION pos2 = list->GetHeadPosition(); pos2 != NULL; )
			{
				const CSearchFile* child = list->GetNext(pos2);
				if (child->GetListParent() == parent)
				{
					// figure out if the childs of different AICH hashs
					if (child->GetFileIdentifierC().HasAICHHash())
					{
						if (bAICHHashValid && aichHash != child->GetFileIdentifierC().GetAICHHash())
							 bHasMultipleAICHHashs = true;
						else if (!bAICHHashValid)
						{
							aichHash = child->GetFileIdentifierC().GetAICHHash();
							bAICHHashValid = true;
						}
					}
					else if (child->DidFoundMultipleAICH())
						 bHasMultipleAICHHashs = true;

					if (parent->IsKademlia())
					{
						if (child->GetListChildCount() > uAllChildsSourceCount)
							uAllChildsSourceCount = child->GetListChildCount();
						/*if (child->GetCompleteSourceCount() > uAllChildsCompleteSourceCount) // not yet supported
							uAllChildsCompleteSourceCount = child->GetCompleteSourceCount();*/
						if (child->GetKadPublishInfo() != 0){
							nPublishInfoTags++;
							uDifferentNames = max(uDifferentNames, ((child->GetKadPublishInfo() & 0xFF000000) >> 24));
							uPublishersKnown = max (uPublishersKnown, ((child->GetKadPublishInfo() & 0x00FF0000) >> 16));
							uTrustValue += child->GetKadPublishInfo() & 0x0000FFFF;
						}
					}
					else
					{
						uAllChildsSourceCount += child->GetListChildCount();
						uAllChildsCompleteSourceCount += child->GetCompleteSourceCount();
					}

					if (bestEntry == NULL)
						bestEntry = child;
					else if (child->GetListChildCount() > bestEntry->GetListChildCount())
						bestEntry = child;
				}
			}
			if (bestEntry)
			{
				parent->SetFileSize(bestEntry->GetFileSize());
				parent->SetFileName(bestEntry->GetFileName());
				parent->SetFileType(bestEntry->GetFileType());
				parent->ClearTags();
				parent->CopyTags(bestEntry->GetTags());
				parent->SetIntTagValue(FT_SOURCES, uAllChildsSourceCount);
				parent->SetIntTagValue(FT_COMPLETE_SOURCES, uAllChildsCompleteSourceCount);
				if (uTrustValue > 0 && nPublishInfoTags > 0)
					uTrustValue = uTrustValue / nPublishInfoTags;
				parent->SetKadPublishInfo(((uDifferentNames & 0xFF) << 24) | ((uPublishersKnown & 0xFF) << 16) | ((uTrustValue & 0xFFFF) << 0));
				// if all childs have the same AICH hash (or none), set the parent hash to it, otherwise clear it (see KademliaSearchKeyword)
				if (bHasMultipleAICHHashs || !bAICHHashValid)
					parent->GetFileIdentifier().ClearAICHHash();
				else if (bAICHHashValid)
					parent->GetFileIdentifier().SetAICHHash(aichHash);
			}
			// recalculate spamrating
			DoSpamRating(parent, bClientResponse, false, false, false, dwFromUDPServerIP);

			// add the 'Availability' of the new search result entry to the total search result count for this search
			AddResultCount(parent->GetSearchID(), parent->GetFileHash(), uAvail, parent->IsConsideredSpam());
			
			// update parent in GUI
			if (outputwnd && !m_MobilMuleSearch)
				outputwnd->UpdateSources(parent);

			if (bFound)
				delete toadd;
			return true;
		}
	}
	
	// no bounded result found yet -> add as parent to list
	toadd->SetListParent(NULL);
	UINT uAvail = 0;
	if (list->AddTail(toadd))
	{
		UINT tempValue = 0;
		VERIFY( m_foundFilesCount.Lookup(toadd->GetSearchID(), tempValue) );
		m_foundFilesCount.SetAt(toadd->GetSearchID(), tempValue + 1);

		// get the 'Availability' of this new search result entry
		
		if (bClientResponse) {
			// If this is a response from a client ("View Shared Files"), we set the "Availability" at least to 1.
			if (!toadd->GetIntTagValue(FT_SOURCES, uAvail) || uAvail==0)
				uAvail = 1;
			toadd->AddSources(uAvail);
		}
		else
			uAvail = toadd->GetIntTagValue(FT_SOURCES);
	}

	if (thePrefs.GetDebugSearchResultDetailLevel() >= 1)
		toadd->SetListExpanded(true);

	// calculate spamrating
	DoSpamRating(toadd, bClientResponse, false, false, false, dwFromUDPServerIP);

	// add the 'Availability' of this new search result entry to the total search result count for this search
	AddResultCount(toadd->GetSearchID(), toadd->GetFileHash(), uAvail, toadd->IsConsideredSpam());

	// add parent in GUI
	if (outputwnd && !m_MobilMuleSearch)
		outputwnd->AddResult(toadd);

	return true;
}

CSearchFile* CSearchList::GetSearchFileByHash(const uchar* hash) const
{
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL;)
	{
		const SearchListsStruct* listCur = m_listFileLists.GetNext(pos);
		for(POSITION pos2 = listCur->m_listSearchFiles.GetHeadPosition(); pos2 != NULL; )
		{
			CSearchFile* sf = listCur->m_listSearchFiles.GetNext(pos2);
			if (!md4cmp(hash, sf->GetFileHash()))
				return sf;
		}
	}
	return NULL;
}

bool CSearchList::AddNotes(Kademlia::CEntry* entry, const uchar *hash)
{
	bool flag = false;
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL;)
	{
		const SearchListsStruct* listCur = m_listFileLists.GetNext(pos);
		for(POSITION pos2 = listCur->m_listSearchFiles.GetHeadPosition(); pos2 != NULL; )
		{
			CSearchFile* sf = listCur->m_listSearchFiles.GetNext(pos2);
			if (!md4cmp(hash, sf->GetFileHash())){
				Kademlia::CEntry* entryClone = entry->Copy();
				if(sf->AddNote(entryClone))
					flag = true;
				else 
					delete entryClone;
			}
		}
	}
	return flag;
}

void CSearchList::SetNotesSearchStatus(const uchar* pFileHash, bool bSearchRunning){
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL;)
	{
		const SearchListsStruct* listCur = m_listFileLists.GetNext(pos);
		for(POSITION pos2 = listCur->m_listSearchFiles.GetHeadPosition(); pos2 != NULL; )
		{
			CSearchFile* sf = listCur->m_listSearchFiles.GetNext(pos2);
			if (!md4cmp(pFileHash, sf->GetFileHash()))
				sf->SetKadCommentSearchRunning(bSearchRunning);
		}
	}
}

void CSearchList::AddResultCount(uint32 nSearchID, const uchar* hash, UINT nCount, bool bSpam)
{
	// do not count already available or downloading files for the search result limit
	if (theApp.sharedfiles->GetFileByID(hash) || theApp.downloadqueue->GetFileByID(hash))
		return;

	UINT tempValue = 0;
	VERIFY( m_foundSourcesCount.Lookup(nSearchID, tempValue) );
	
	// spam files count as max 5 availability
	m_foundSourcesCount.SetAt(nSearchID, tempValue 
		+ ( (bSpam && thePrefs.IsSearchSpamFilterEnabled()) ? min(nCount, 5) : nCount) );
}
// FIXME LARGE FILES
void CSearchList::KademliaSearchKeyword(uint32 searchID, const Kademlia::CUInt128* fileID, LPCTSTR name,
										uint64 size, LPCTSTR type, UINT uKadPublishInfo
										, CArray<CAICHHash>& raAICHHashs, CArray<uint8>& raAICHHashPopularity
										, SSearchTerm* pQueriedSearchTerm,  UINT numProperties, ...)
{
	va_list args;
	va_start(args, numProperties);

	EUtf8Str eStrEncode = utf8strRaw;
	CSafeMemFile* temp = new CSafeMemFile(250);
	Kademlia::CKeyEntry verifierEntry;

	verifierEntry.m_uKeyID.SetValue(*fileID);
	uchar fileid[16];
	fileID->ToByteArray(fileid);
	temp->WriteHash16(fileid);
	
	
	temp->WriteUInt32(0);	// client IP
	temp->WriteUInt16(0);	// client port
	
	// write tag list
	UINT uFilePosTagCount = (UINT)temp->GetPosition();
	uint32 tagcount = 0;
	temp->WriteUInt32(tagcount); // dummy tag count, will be filled later

	// standard tags
	CTag tagName(FT_FILENAME, name);
	tagName.WriteTagToFile(temp, eStrEncode);
	tagcount++;
	verifierEntry.SetFileName(name);

	CTag tagSize(FT_FILESIZE, size, true); 
	tagSize.WriteTagToFile(temp, eStrEncode);
	tagcount++;
	verifierEntry.m_uSize = size;

	if (type != NULL && type[0] != _T('\0'))
	{
		CTag tagType(FT_FILETYPE, type);
		tagType.WriteTagToFile(temp, eStrEncode);
		tagcount++;
		verifierEntry.AddTag(new Kademlia::CKadTagStr(TAG_FILETYPE, type));
	}

	// additional tags
	while (numProperties-- > 0)
	{
		UINT uPropType = va_arg(args, UINT);
		LPCSTR pszPropName = va_arg(args, LPCSTR);
		LPVOID pvPropValue = va_arg(args, LPVOID);
		if (uPropType == 2 /*TAGTYPE_STRING*/)
		{
			if ((LPCTSTR)pvPropValue != NULL && ((LPCTSTR)pvPropValue)[0] != _T('\0'))
			{
				if (strlen(pszPropName) == 1)
				{
					CTag tagProp((uint8)*pszPropName, (LPCTSTR)pvPropValue);
					tagProp.WriteTagToFile(temp, eStrEncode);
				}
				else
				{
					CTag tagProp(pszPropName, (LPCTSTR)pvPropValue);
					tagProp.WriteTagToFile(temp, eStrEncode);
				}
				verifierEntry.AddTag(new Kademlia::CKadTagStr(pszPropName, (LPCTSTR)pvPropValue));
				tagcount++;
			}
		}
		else if (uPropType == 3 /*TAGTYPE_UINT32*/)
		{
			if ((uint32)pvPropValue != 0)
			{	
				CTag tagProp(pszPropName, (uint32)pvPropValue);
				tagProp.WriteTagToFile(temp, eStrEncode);
				tagcount++;
				verifierEntry.AddTag(new Kademlia::CKadTagUInt(pszPropName, (uint32)pvPropValue));
			}
		}
		else
		{
			ASSERT(0);
		}
	}
	va_end(args);
	temp->Seek(uFilePosTagCount, SEEK_SET);
	temp->WriteUInt32(tagcount);
	
	temp->SeekToBegin();
	if (pQueriedSearchTerm == NULL || verifierEntry.StartSearchTermsMatch(pQueriedSearchTerm))
	{
		CSearchFile* tempFile = new CSearchFile(temp, eStrEncode == utf8strRaw, searchID, 0, 0, 0, true);
		tempFile->SetKadPublishInfo(uKadPublishInfo);
		// About the AICH hash: We received a list of possible AICH Hashs for this file and now have to deceide what to do
		// If it wasn't for backwards compability, the choice would be easy: Each different md4+aich+size is its own result,
		// but we can'T do this alone for the fact that for the next years we will always have publishers which don'T report
		// the AICH hash at all (which would mean ahving a different entry, which leads to double files in searchresults). So here is what we do for now:
		// If we have excactly 1 AICH hash and more than 1/3 of the publishers reported it, we set it as verified AICH hash for
		// the file (which is as good as using a ed2k link with an AICH hash attached). If less publishers reported it or if we
		// have multiple AICH hashes, we ignore them and use the MD4 only.
		// This isn't a perfect solution, but it makes sure not to open any new attack vectors (a wrong AICH hash means we cannot
		// download the file sucessfully) nor to confuse users by requiering them to select an entry out of several equal looking results.
		// Once the majority of nodes in the network publishes AICH hashes, this might get reworked to make the AICH hash more sticky
		if (raAICHHashs.GetCount() == 1 && raAICHHashPopularity.GetCount() == 1)
		{
			uint8 byPublishers = (uint8)((uKadPublishInfo & 0x00FF0000) >> 16);
			if ( byPublishers > 0 && raAICHHashPopularity[0] > 0 && byPublishers / raAICHHashPopularity[0] <= 3)
			{
				DEBUG_ONLY( DebugLog(_T("Received accepted AICH Hash for search result %s, %u out of %u Publishers, Hash: %s")
								, tempFile->GetFileName(), raAICHHashPopularity[0], byPublishers, raAICHHashs[0].GetString()) );
				tempFile->GetFileIdentifier().SetAICHHash(raAICHHashs[0]);
			}
			else
				DEBUG_ONLY( DebugLog(_T("Received unaccepted AICH Hash for search result %s, %u out of %u Publishers, Hash: %s")
								, tempFile->GetFileName(), raAICHHashPopularity[0], byPublishers, raAICHHashs[0].GetString()) );
		}
		else if (raAICHHashs.GetCount() > 1)
			DEBUG_ONLY( DebugLog(_T("Received multiple (%u) AICH Hashs for search result %s, ignoring AICH"), raAICHHashs.GetCount(), tempFile->GetFileName()) );
		AddToList(tempFile);
	}
	else
	{
		DebugLogWarning(_T("Kad Searchresult failed sanitize check against search query, ignoring. (%s)"), name);
	}
	
	delete temp;
}


// default spam threshold = 60
#define SPAM_FILEHASH_HIT					100

#define SPAM_FULLNAME_HIT					80
#define	SPAM_SMALLFULLNAME_HIT				50
#define SPAM_SIMILARNAME_HIT				60
#define SPAM_SMALLSIMILARNAME_HIT			40
#define SPAM_SIMILARNAME_NEARHIT			50
#define SPAM_SIMILARNAME_FARHIT				40

#define SPAM_SIMILARSIZE_HIT				10

#define SPAM_UDPSERVERRES_HIT				21
#define SPAM_UDPSERVERRES_NEARHIT			15
#define SPAM_UDPSERVERRES_FARHIT			10

#define SPAM_ONLYUDPSPAMSERVERS_HIT			30

#define SPAM_SOURCE_HIT						39

#define SPAM_HEURISTIC_BASEHIT				39
#define SPAM_HEURISTIC_MAXHIT				60


#define UDP_SPAMRATIO_THRESHOLD				50


void CSearchList::DoSpamRating(CSearchFile* pSearchFile, bool bIsClientFile, bool bMarkAsNoSpam, bool bRecalculateAll, bool bUpdate, uint32 dwFromUDPServerIP){
	/* This spam filter uses two simple approaches to try to identify spam search results:
	1 - detect general characteristics of fake results - not very reliable
		which are (each hit increases the score)
		* high availability from one udp server, but none from others
		* archive or programm + size between 0,1 and 10 MB
		* 100% complete sources together with high availability
		Appearently, those characteristics target for current spyware fake results, other fake results like videos
		and so on will not be detectable, because only the first point is more or less common for server fake results,
		which would produce too many false positives

	2 - learn characteristics of files a user has marked as spam
		remembered data is:
		* FileHash (of course, a hit will always lead to a full score rating)
		* Equal filename
		* Equal or similar name after removing the search keywords and seperators 
			(if search for "emule", "blubby!! emule foo.rar" is remembered as "blubby foo rar")
		* Similar size (+- 5% but max 5MB) as other spam files
		* Equal search source server (UDP only)
		* Equal initial source clients
		* Ratio (Spam / NotSpam) of UDP Servers 
	Both detection methods add to the same score rating.

	bMarkAsNoSpam = true: Will remove all stored characteristics which would add to an postive spam score for this file
	*/
	ASSERT( (bRecalculateAll && bMarkAsNoSpam) || !bRecalculateAll );

	if (!thePrefs.IsSearchSpamFilterEnabled())
		return;
	
	if (!m_bSpamFilterLoaded)
		LoadSpamFilter();
	
	int nSpamScore = 0;
	CString strDebug;
	bool bSureNegative = false;
	int nDbgFileHash, nDbgStrings, nDbgSize, nDbgServer, nDbgSources, nDbgHeuristic, nDbgOnlySpamServer;
	nDbgFileHash = nDbgStrings = nDbgSize = nDbgServer = nDbgSources = nDbgHeuristic = nDbgOnlySpamServer = 0;

	// 1- filehash
	bool bSpam = false;
	if (m_mapKnownSpamHashs.Lookup(CSKey(pSearchFile->GetFileHash()), bSpam)){
		if (!bMarkAsNoSpam && bSpam){
			nSpamScore += SPAM_FILEHASH_HIT;
			nDbgFileHash = SPAM_FILEHASH_HIT;
		}
		else if (bSpam)
			m_mapKnownSpamHashs.RemoveKey(CSKey(pSearchFile->GetFileHash()));
		else
			bSureNegative = true;
	}
	CSearchFile* pParent = NULL;
	if (pSearchFile->GetListParent() != NULL)
		pParent = pSearchFile->GetListParent();
	else if (pSearchFile->GetListChildCount() > 0)
		pParent = pSearchFile;
	CSearchFile* pTempFile = (pSearchFile->GetListParent() != NULL) ? pSearchFile->GetListParent() : pSearchFile;

	if (!bSureNegative && bMarkAsNoSpam)
		m_mapKnownSpamHashs.SetAt(CSKey(pSearchFile->GetFileHash()), false);
#ifndef _DEBUG
	else if (bSureNegative && !bMarkAsNoSpam)
	{
#endif
		// 2-3 FileNames
		// consider also filenames of childs / parents / silblings and take the highest rating


		uint32 nHighestRating;	
		if (pParent != NULL){
			nHighestRating = GetSpamFilenameRatings(pParent, bMarkAsNoSpam);
			SearchList* list = GetSearchListForID(pParent->GetSearchID());
			for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
			{
				const CSearchFile* pCurFile = list->GetNext(pos);
				if (pCurFile->GetListParent() == pParent){
					uint32 nRating = GetSpamFilenameRatings(pCurFile, bMarkAsNoSpam);
					nHighestRating = max(nHighestRating, nRating);
				}
			}
		}
		else
			nHighestRating = GetSpamFilenameRatings(pSearchFile, bMarkAsNoSpam);
		nSpamScore += nHighestRating;
		nDbgStrings = nHighestRating;

		//4 - Sizes
		for (int i = 0; i < m_aui64KnownSpamSizes.GetCount(); i++){
			if ((uint64)pSearchFile->GetFileSize() != 0 && _abs64((uint64)pSearchFile->GetFileSize() - m_aui64KnownSpamSizes[i]) < 5242880 &&
				((_abs64((uint64)pSearchFile->GetFileSize() - m_aui64KnownSpamSizes[i]) * 100) / (uint64)pSearchFile->GetFileSize()) < 5)
			{
				if (!bMarkAsNoSpam){
					nSpamScore += SPAM_SIMILARSIZE_HIT;
					nDbgSize = SPAM_SIMILARSIZE_HIT;
					break;
				}
				else{
					m_aui64KnownSpamSizes.RemoveAt(i);
					i--;
				}
			}
		}

		if (!bIsClientFile){ // only to skip some useless calculations
			//5 Servers
			for (int i = 0; i != pTempFile->GetServers().GetSize(); i++){
				bool bFound = false;
				if (pTempFile->GetServers()[i].m_nIP != 0 && pTempFile->GetServers()[i].m_bUDPAnswer 
					&& m_mapKnownSpamServerIPs.Lookup(pTempFile->GetServers()[i].m_nIP, bFound))
				{
					if (!bMarkAsNoSpam){
						strDebug.AppendFormat(_T(" (Serverhit: %s)"), ipstr(pTempFile->GetServers()[i].m_nIP));
						if (pSearchFile->GetServers().GetSize() == 1 && m_mapKnownSpamServerIPs.GetCount() <= 10){
							// source only from one server
							nSpamScore += SPAM_UDPSERVERRES_HIT;
							nDbgServer = SPAM_UDPSERVERRES_HIT;
						}
						else if (pSearchFile->GetServers().GetSize() == 1){
							// source only from one server but the users seems to be a bit careless with the mark as spam option
							// and has already added a lot UDP servers. To avoid false positives, we give a lower rating
							nSpamScore += SPAM_UDPSERVERRES_NEARHIT;
							nDbgServer = SPAM_UDPSERVERRES_NEARHIT;
						}
						else{
							// file was given by more than one server, lowest spam rating for server hits
							nSpamScore += SPAM_UDPSERVERRES_FARHIT;
							nDbgServer = SPAM_UDPSERVERRES_FARHIT;
						}
						break;
					}
					else
						m_mapKnownSpamServerIPs.RemoveKey(pTempFile->GetServers()[i].m_nIP);
				}
			}

			// partial heuristic - only udp spamservers
			// has this file at least one server as origin which is not rated for spam or UDP
			// or not a result from a server at all
			bool bNormalServerWithoutCurrentPresent = (pTempFile->GetServers().GetSize() == 0);
			bool bNormalServerPresent = (pTempFile->GetServers().GetSize() == 0); 
			for (int i = 0; i != pTempFile->GetServers().GetSize(); i++){
				UDPServerRecord* pRecord = NULL;
				if (!bMarkAsNoSpam && pTempFile->GetServers()[i].m_bUDPAnswer && m_aUDPServerRecords.Lookup(pTempFile->GetServers()[i].m_nIP, pRecord) && pRecord != NULL){
					ASSERT( pRecord->m_nResults >= pRecord->m_nSpamResults );
					int nRatio;
					if (pRecord->m_nResults >= pRecord->m_nSpamResults && pRecord->m_nResults != 0){
						nRatio = (pRecord->m_nSpamResults * 100) / pRecord->m_nResults;
					}
					else
						nRatio = 100;
					if (nRatio < 50){
						bNormalServerWithoutCurrentPresent |= (dwFromUDPServerIP != pTempFile->GetServers()[i].m_nIP);
						bNormalServerPresent = true; 
					}
				}
				else if (!pTempFile->GetServers()[i].m_bUDPAnswer){
					bNormalServerWithoutCurrentPresent = true;
					bNormalServerPresent = true;
					break;
				}
				else if (!bMarkAsNoSpam)
					ASSERT( pRecord != NULL );
			}
			if (!bNormalServerPresent && !bMarkAsNoSpam){
				nDbgOnlySpamServer = SPAM_ONLYUDPSPAMSERVERS_HIT;
				nSpamScore += SPAM_ONLYUDPSPAMSERVERS_HIT;
				strDebug += _T(" (AllSpamServers)");
			}
			else if(!bNormalServerWithoutCurrentPresent && !bMarkAsNoSpam)
				strDebug += _T(" (AllSpamServersWoCurrent)");


		// 7 Heuristic (UDP Results)
			uint32 nResponses = 0;
			VERIFY( m_ReceivedUDPAnswersCount.Lookup(pTempFile->GetSearchID(), nResponses) );
			uint32 nRequests = 0;
			VERIFY( m_RequestedUDPAnswersCount.Lookup(pTempFile->GetSearchID(), nRequests) );
			if (!bNormalServerWithoutCurrentPresent 
				&& (nResponses >= 3 || nRequests >= 5) && pTempFile->GetSourceCount() > 100)		
			{
				// check if the one of the files sources are in the same ip subnet as a udp server
				// which indicates that the server is advertising its own files
				bool bSourceServer = false;
				for (int i = 0; i != pTempFile->GetServers().GetSize(); i++){
					if (pTempFile->GetServers()[i].m_nIP != 0){
						if ( (pTempFile->GetServers()[i].m_nIP & 0x00FFFFFF) == (pTempFile->GetClientID() & 0x00FFFFFF)){
							bSourceServer = true;
							strDebug.AppendFormat(_T(" (Server: %s - Source: %s Hit)"), ipstr(pTempFile->GetServers()[i].m_nIP), ipstr(pTempFile->GetClientID()));
							break;
						}
						for (int j = 0; j != pTempFile->GetClients().GetSize(); j++){
							if ( (pTempFile->GetServers()[i].m_nIP & 0x00FFFFFF) == (pTempFile->GetClients()[j].m_nIP & 0x00FFFFFF)){
								bSourceServer = true;
								strDebug.AppendFormat(_T(" (Server: %s - Source: %s Hit)"), ipstr(pTempFile->GetServers()[i].m_nIP), ipstr(pTempFile->GetClients()[j].m_nIP));
								break;
							}
						}
					}
				}

				if (((GetED2KFileTypeID(pTempFile->GetFileName()) == ED2KFT_PROGRAM || GetED2KFileTypeID(pTempFile->GetFileName()) ==  ED2KFT_ARCHIVE)
					&& (uint64)pTempFile->GetFileSize() > 102400 && (uint64)pTempFile->GetFileSize() < 10485760
					&& !bMarkAsNoSpam)
					|| bSourceServer)
				{
					nSpamScore += SPAM_HEURISTIC_MAXHIT;
					nDbgHeuristic = SPAM_HEURISTIC_MAXHIT;
				}
				else if (!bMarkAsNoSpam){
					nSpamScore += SPAM_HEURISTIC_BASEHIT;
					nDbgHeuristic = SPAM_HEURISTIC_BASEHIT;
				}
			}
		}		
		// 6 Sources
		bool bFound = false;
		if (IsValidSearchResultClientIPPort(pTempFile->GetClientID(), pTempFile->GetClientPort())
			&& !::IsLowID(pTempFile->GetClientID())
			&& m_mapKnownSpamSourcesIPs.Lookup(pTempFile->GetClientID(), bFound))
		{
			if (!bMarkAsNoSpam){
				strDebug.AppendFormat(_T(" (Sourceshit: %s)"), ipstr(pTempFile->GetClientID()));
				nSpamScore += SPAM_SOURCE_HIT;
				nDbgSources = SPAM_SOURCE_HIT;
			}
			else
				m_mapKnownSpamSourcesIPs.RemoveKey(pTempFile->GetClientID());
		}
		else{
			for (int i = 0; i != pTempFile->GetClients().GetSize(); i++){
				if (pTempFile->GetClients()[i].m_nIP != 0 
					&& m_mapKnownSpamSourcesIPs.Lookup(pTempFile->GetClients()[i].m_nIP, bFound))
				{
					if (!bMarkAsNoSpam){
						strDebug.AppendFormat(_T(" (Sources: %s)"), ipstr(pTempFile->GetClients()[i].m_nIP));
						nSpamScore += SPAM_SOURCE_HIT;
						nDbgSources = SPAM_SOURCE_HIT;
						break;
					}
					else
						m_mapKnownSpamSourcesIPs.RemoveKey(pTempFile->GetClients()[i].m_nIP);
				}		
			}
		}
#ifndef _DEBUG
	}
#endif

	if (!bMarkAsNoSpam){
		if (nSpamScore > 0)
			DebugLog(_T("Spamrating Result: %u. Details: Hash: %u, Name: %u, Size: %u, Server: %u, Sources: %u, Heuristic: %u, OnlySpamServers: %u. %s Filename: %s")
				,bSureNegative ? 0 : nSpamScore, nDbgFileHash, nDbgStrings, nDbgSize, nDbgServer, nDbgSources, nDbgHeuristic, nDbgOnlySpamServer, strDebug, pSearchFile->GetFileName());
	}
	else{
		DebugLog(_T("Marked file as No Spam, Old Rating: %u."), pSearchFile->GetSpamRating());
	}
	
	bool bOldSpamStatus = pSearchFile->IsConsideredSpam();

	pParent = NULL;
	if (pSearchFile->GetListParent() != NULL)
		pParent = pSearchFile->GetListParent();
	else if (pSearchFile->GetListChildCount() > 0)
		pParent = pSearchFile;

	if (pParent != NULL){
		pParent->SetSpamRating(bMarkAsNoSpam ? 0 : nSpamScore);
		SearchList* list = GetSearchListForID(pParent->GetSearchID());
		for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
		{
			CSearchFile* pCurFile = list->GetNext(pos);
			if (pCurFile->GetListParent() == pParent)
				pCurFile->SetSpamRating( (bMarkAsNoSpam || bSureNegative) ? 0 : nSpamScore);
		}
	}
	else
		pSearchFile->SetSpamRating(bMarkAsNoSpam ? 0 : nSpamScore);

	// keep record about ratio of spam in UDP server results
	if (bOldSpamStatus != pSearchFile->IsConsideredSpam()){
		for (int i = 0; i != pTempFile->GetServers().GetSize(); i++){
			UDPServerRecord* pRecord = NULL;
			if (pTempFile->GetServers()[i].m_bUDPAnswer
				&& m_aUDPServerRecords.Lookup(pTempFile->GetServers()[i].m_nIP, pRecord) && pRecord != NULL)
			{
				if (pSearchFile->IsConsideredSpam())
					pRecord->m_nSpamResults++;
				else {
					ASSERT( pRecord->m_nSpamResults > 0 );
					pRecord->m_nSpamResults--;
				}
			}
		}
	}
	else if (dwFromUDPServerIP != 0 && pSearchFile->IsConsideredSpam()){
		// files was already spam, but a new server also gave it as result, add it to his spam stats
		UDPServerRecord* pRecord = NULL;
		if (m_aUDPServerRecords.Lookup(dwFromUDPServerIP, pRecord) && pRecord != NULL)
			pRecord->m_nSpamResults++;
	}
	
	if (bUpdate && outputwnd != NULL)
		outputwnd->UpdateSources((pParent != NULL) ? pParent : pSearchFile);
	if (bRecalculateAll)
		RecalculateSpamRatings(pSearchFile->GetSearchID(), false, true, bUpdate);
}

uint32 CSearchList::GetSpamFilenameRatings(const CSearchFile* pSearchFile, bool bMarkAsNoSpam){
	for (int i = 0; i < m_astrKnownSpamNames.GetCount(); i++){
		if (pSearchFile->GetFileName().CompareNoCase(m_astrKnownSpamNames[i]) == 0){
			if (!bMarkAsNoSpam){
				if (pSearchFile->GetFileName().GetLength() <= 10)
					return SPAM_SMALLFULLNAME_HIT;
				else
					return SPAM_FULLNAME_HIT;
			}
			else{
				m_astrKnownSpamNames.RemoveAt(i);
				i--;
			}
		}
	}
	uint32 nResult = 0;
	if (!m_astrKnownSimilarSpamNames.IsEmpty() && !pSearchFile->GetNameWithoutKeyword().IsEmpty()){
		
		for (int i = 0; i < m_astrKnownSimilarSpamNames.GetCount(); i++){
			bool bRemove = false;
			if (pSearchFile->GetNameWithoutKeyword().Compare(m_astrKnownSimilarSpamNames[i]) == 0){
				if (!bMarkAsNoSpam){
					if (pSearchFile->GetNameWithoutKeyword().GetLength() <= 10)
						return SPAM_SMALLSIMILARNAME_HIT;
					else
						return SPAM_SIMILARNAME_HIT;
				}
				else
					bRemove = true;
			}
			else if (pSearchFile->GetNameWithoutKeyword().GetLength() > 10 
				&& (abs(pSearchFile->GetNameWithoutKeyword().GetLength() - m_astrKnownSimilarSpamNames[i].GetLength()) == 0
				|| pSearchFile->GetNameWithoutKeyword().GetLength() / abs(pSearchFile->GetNameWithoutKeyword().GetLength() - m_astrKnownSimilarSpamNames[i].GetLength()) >= 3))
			{
				uint32 nStringComp = LevenshteinDistance(pSearchFile->GetNameWithoutKeyword(), m_astrKnownSimilarSpamNames[i]);
				if (nStringComp != 0)
					nStringComp = pSearchFile->GetNameWithoutKeyword().GetLength() / nStringComp;
				if (nStringComp >= 3){
					if (!bMarkAsNoSpam){
						if (nStringComp >= 6)
							nResult = SPAM_SIMILARNAME_NEARHIT;
						else
							nResult = max(nResult, SPAM_SIMILARNAME_FARHIT); 
					}
					else
						bRemove = true;
				}
			}
			if (bRemove){
				m_astrKnownSimilarSpamNames.RemoveAt(i);
				i--;
			}
		}
	}
	return nResult;
}


SearchList* CSearchList::GetSearchListForID(uint32 nSearchID){
	for (POSITION pos = m_listFileLists.GetHeadPosition(); pos != NULL; )
	{
		SearchListsStruct* list = m_listFileLists.GetNext(pos);
		if (list->m_nSearchID == nSearchID)
			return &list->m_listSearchFiles;
	}
	SearchListsStruct* list = new SearchListsStruct;
	list->m_nSearchID = nSearchID;
	m_listFileLists.AddHead(list);
	return &list->m_listSearchFiles;
}

void CSearchList::SentUDPRequestNotification(uint32 nSearchID, uint32 dwServerIP){
	if (nSearchID == m_nCurED2KSearchID){
		m_RequestedUDPAnswersCount.SetAt(nSearchID, m_aCurED2KSentRequestsIPs.Add(dwServerIP) + 1);
	}
	else
		ASSERT( false );

}

void CSearchList::MarkFileAsSpam(CSearchFile* pSpamFile, bool bRecalculateAll, bool bUpdate){
	if (!m_bSpamFilterLoaded)
		LoadSpamFilter();

	m_astrKnownSpamNames.Add(pSpamFile->GetFileName());
	m_astrKnownSimilarSpamNames.Add(pSpamFile->GetNameWithoutKeyword());
	m_mapKnownSpamHashs.SetAt(CSKey(pSpamFile->GetFileHash()), true);
	m_aui64KnownSpamSizes.Add((uint64)pSpamFile->GetFileSize());
	
	if (IsValidSearchResultClientIPPort(pSpamFile->GetClientID(), pSpamFile->GetClientPort())
		&& !::IsLowID(pSpamFile->GetClientID()))
	{
		m_mapKnownSpamSourcesIPs.SetAt(pSpamFile->GetClientID(), true);
	}
	for (int i = 0; i != pSpamFile->GetClients().GetSize(); i++){;
		if (pSpamFile->GetClients()[i].m_nIP != 0){
			m_mapKnownSpamSourcesIPs.SetAt(pSpamFile->GetClients()[i].m_nIP, true);
		}		
	}
	for (int i = 0; i != pSpamFile->GetServers().GetSize(); i++){
		if (pSpamFile->GetServers()[i].m_nIP != 0 && pSpamFile->GetServers()[i].m_bUDPAnswer){
			m_mapKnownSpamServerIPs.SetAt(pSpamFile->GetServers()[i].m_nIP, true);
		}		
	}


	if (bRecalculateAll)
		RecalculateSpamRatings(pSpamFile->GetSearchID(), true, false, bUpdate);
	else
		DoSpamRating(pSpamFile);
	
	if (bUpdate && outputwnd != NULL)
		outputwnd->UpdateSources(pSpamFile);
}

void CSearchList::RecalculateSpamRatings(uint32 nSearchID, bool bExpectHigher, bool bExpectLower, bool bUpdate){
	ASSERT( !(bExpectHigher && bExpectLower) );
	ASSERT( m_bSpamFilterLoaded );

	SearchList* list = GetSearchListForID(nSearchID);
	for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
	{
		CSearchFile* pCurFile = list->GetNext(pos);
		// check only parents and only if we expect a status change
		if (pCurFile->GetListParent() == NULL && !(pCurFile->IsConsideredSpam() && bExpectHigher)
			&& !(!pCurFile->IsConsideredSpam() && bExpectLower))
		{
			DoSpamRating(pCurFile, false, false, false, false);
			if (bUpdate && outputwnd != NULL)
				outputwnd->UpdateSources(pCurFile);
		}
	}
}

void CSearchList::LoadSpamFilter(){
	m_astrKnownSpamNames.RemoveAll();
	m_astrKnownSimilarSpamNames.RemoveAll();
	m_mapKnownSpamServerIPs.RemoveAll();
	m_mapKnownSpamSourcesIPs.RemoveAll();
	m_mapKnownSpamHashs.RemoveAll();
	m_aui64KnownSpamSizes.RemoveAll();
	int nDbgFileHashPos = 0;

	m_bSpamFilterLoaded = true;

	CString fullpath = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR);
	fullpath.Append(SPAMFILTER_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") SPAMFILTER_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			DebugLogError(_T("%s"), strError);
		}
		return;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	try {
		uint8 header = file.ReadUInt8();
		if (header != MET_HEADER_I64TAGS){
			file.Close();
			DebugLogError(_T("Failed to load searchspam.met, invalid first byte"));
			return;
		}

		UINT RecordsNumber = file.ReadUInt32();
		for (UINT i = 0; i < RecordsNumber; i++) {
			CTag tag(&file, false);
			switch (tag.GetNameID()){
				case SP_FILEHASHSPAM:
					ASSERT( tag.IsHash() );
					if (tag.IsHash())
						m_mapKnownSpamHashs.SetAt(CSKey(tag.GetHash()), true);
					break;
				case SP_FILEHASHNOSPAM:
					ASSERT( tag.IsHash() );
					if (tag.IsHash()){
						m_mapKnownSpamHashs.SetAt(CSKey(tag.GetHash()), false);
						nDbgFileHashPos++;
					}
					break;
				case SP_FILEFULLNAME:
					ASSERT( tag.IsStr() );
					if (tag.IsStr())
						m_astrKnownSpamNames.Add(tag.GetStr());
					break;
				case SP_FILESIMILARNAME:
					ASSERT( tag.IsStr() );
					if (tag.IsStr())
						m_astrKnownSimilarSpamNames.Add(tag.GetStr());
					break;
				case SP_FILESOURCEIP:
					ASSERT( tag.IsInt() );
					if (tag.IsInt())
						m_mapKnownSpamSourcesIPs.SetAt(tag.GetInt(), true);
					break;
				case SP_FILESERVERIP:
					ASSERT( tag.IsInt() );
					if (tag.IsInt())
						m_mapKnownSpamServerIPs.SetAt(tag.GetInt(), true);
					break;
				case SP_FILESIZE:
					ASSERT( tag.IsInt64() );
					if (tag.IsInt64())
						m_aui64KnownSpamSizes.Add(tag.GetInt64());
					break;
				case SP_UDPSERVERSPAMRATIO:
					ASSERT( tag.IsBlob() && tag.GetBlobSize() == 12);
					if (tag.IsBlob() && tag.GetBlobSize() == 12){
						const BYTE* pBuffer = tag.GetBlob();
						UDPServerRecord* pRecord = new UDPServerRecord;
						pRecord->m_nResults = PeekUInt32(&pBuffer[4]);
						pRecord->m_nSpamResults = PeekUInt32(&pBuffer[8]);
						m_aUDPServerRecords.SetAt(PeekUInt32(&pBuffer[0]), pRecord);
						int nRatio;
						if (pRecord->m_nResults >= pRecord->m_nSpamResults && pRecord->m_nResults != 0){
							nRatio = (pRecord->m_nSpamResults * 100) / pRecord->m_nResults;
						}
						else
							nRatio = 100;
						DEBUG_ONLY(DebugLog(_T("UDP Server Spam Record: IP: %s, Results: %u, SpamResults: %u, Ratio: %u")
							, ipstr(PeekUInt32(&pBuffer[0])), pRecord->m_nResults, pRecord->m_nSpamResults, nRatio));
					}
					break;
				default:
					ASSERT( false );
			}
		}
		file.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			DebugLogError(_T("Failed to load searchspam.met, corrupt"));
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			DebugLogError(_T("Failed to load searchspam.met, %s"),buffer);
		}
		error->Delete();
		return;
	}

	DebugLog(_T("Loaded search Spam Filter. Entries - ServerIPs: %u, SourceIPs, %u, Hashs: %u, PositiveHashs: %u, FileSizes: %u, FullNames: %u, SimilarNames: %u")
		, m_mapKnownSpamSourcesIPs.GetCount(), m_mapKnownSpamServerIPs.GetCount(), m_mapKnownSpamHashs.GetCount() - nDbgFileHashPos, nDbgFileHashPos
		, m_aui64KnownSpamSizes.GetCount(), m_astrKnownSpamNames.GetCount(), m_astrKnownSimilarSpamNames.GetCount());
}

void CSearchList::SaveSpamFilter(){
	if (!m_bSpamFilterLoaded)
		return;

	CString fullpath = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR);
	fullpath.Append(SPAMFILTER_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") SPAMFILTER_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			DebugLogError(_T("%s"), strError);
		}
		return;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
	uint32 nCount = 0;
	try{
		file.WriteUInt8(MET_HEADER_I64TAGS);
		
		file.WriteUInt32(nCount);
		
		for (int i = 0; i < m_astrKnownSpamNames.GetCount(); i++){
			CTag tag(SP_FILEFULLNAME, m_astrKnownSpamNames[i]);
			tag.WriteNewEd2kTag(&file, utf8strOptBOM);
			nCount++;
		}

		for (int i = 0; i < m_astrKnownSimilarSpamNames.GetCount(); i++){
			CTag tag(SP_FILESIMILARNAME, m_astrKnownSimilarSpamNames[i]);
			tag.WriteNewEd2kTag(&file, utf8strOptBOM);
			nCount++;
		}

		for (int i = 0; i < m_aui64KnownSpamSizes.GetCount(); i++){
			CTag tag(SP_FILESIZE, m_aui64KnownSpamSizes[i], true);
			tag.WriteNewEd2kTag(&file);
			nCount++;
		}

		POSITION pos = m_mapKnownSpamHashs.GetStartPosition();
		while( pos != NULL )
		{
			bool bSpam;
			CSKey key;
			m_mapKnownSpamHashs.GetNextAssoc(pos, key, bSpam);
			if (bSpam){
				CTag tag(SP_FILEHASHSPAM, (BYTE*)key.m_key);
				tag.WriteNewEd2kTag(&file);
			}
			else{
				CTag tag(SP_FILEHASHNOSPAM, (BYTE*)key.m_key);
				tag.WriteNewEd2kTag(&file);
			}
			nCount++;
		}

		pos = m_mapKnownSpamServerIPs.GetStartPosition();
		while( pos != NULL )
		{
			bool bTmp;
			uint32 dwIP;
			m_mapKnownSpamServerIPs.GetNextAssoc(pos, dwIP, bTmp);
			CTag tag(SP_FILESERVERIP, dwIP);
			tag.WriteNewEd2kTag(&file);
			nCount++;
		}

		pos = m_mapKnownSpamSourcesIPs.GetStartPosition();
		while( pos != NULL )
		{
			bool bTmp;
			uint32 dwIP;
			m_mapKnownSpamSourcesIPs.GetNextAssoc(pos, dwIP, bTmp);
			CTag tag(SP_FILESOURCEIP, dwIP);
			tag.WriteNewEd2kTag(&file);
			nCount++;
		}

		pos = m_aUDPServerRecords.GetStartPosition();
		while( pos != NULL )
		{
			UDPServerRecord* pRecord;
			uint32 dwIP;
			m_aUDPServerRecords.GetNextAssoc(pos, dwIP, pRecord);
			BYTE abyBuffer[12];
			PokeUInt32(&abyBuffer[0], dwIP);
			PokeUInt32(&abyBuffer[4], pRecord->m_nResults);
			PokeUInt32(&abyBuffer[8], pRecord->m_nSpamResults);
			CTag tag(SP_UDPSERVERSPAMRATIO, sizeof(abyBuffer), abyBuffer);
			tag.WriteNewEd2kTag(&file);
			nCount++;
		}

		file.Seek(1, CFile::begin);
		file.WriteUInt32(nCount);
		file.Close();
	}
	catch(CFileException* error){
		TCHAR buffer[MAX_CFEXP_ERRORMSG];
		error->GetErrorMessage(buffer, ARRSIZE(buffer));
		DebugLogError(_T("Failed to save searchspam.met, %s"),buffer);
		error->Delete();
		return;
	}
	DebugLog(_T("Stored searchspam.met, wrote %u records"), nCount);
}

void CSearchList::StoreSearches(){
	// store open searches on shutdown to restore them on the next startup
	CString fullpath = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR);
	fullpath.Append(STOREDSEARCHES_FILENAME);

	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") STOREDSEARCHES_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			DebugLogError(_T("%s"), strError);
		}
		return;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
	uint16 nCount = 0;
	try{
		file.WriteUInt8(MET_HEADER_I64TAGS);
		uint8 byVersion = STOREDSEARCHES_VERSION;
		file.WriteUInt8(byVersion);
		// count how many (if any) open searches we have which are GUI related	
		POSITION pos;
		for (pos = m_listFileLists.GetHeadPosition(); pos != NULL; ){
			SearchListsStruct* pSl = m_listFileLists.GetNext(pos);
			if (theApp.emuledlg->searchwnd->GetSearchParamsBySearchID(pSl->m_nSearchID) != NULL)
				nCount++;
		}
		file.WriteUInt16(nCount);
		if (nCount > 0){
			POSITION pos;
			for (pos = m_listFileLists.GetTailPosition(); pos != NULL; ){
				SearchListsStruct* pSl = m_listFileLists.GetPrev(pos);
				SSearchParams* pParams = theApp.emuledlg->searchwnd->GetSearchParamsBySearchID(pSl->m_nSearchID);
				if (pParams != NULL){
					pParams->StorePartially(file);
					file.WriteUInt32(pSl->m_listSearchFiles.GetCount());
					POSITION pos2;
					for (pos2 = pSl->m_listSearchFiles.GetHeadPosition(); pos2 != NULL;)
						pSl->m_listSearchFiles.GetNext(pos2)->StoreToFile(file);
				}
			}
		}
		file.Close();
	}
	catch(CFileException* error){
		TCHAR buffer[MAX_CFEXP_ERRORMSG];
		error->GetErrorMessage(buffer, ARRSIZE(buffer));
		DebugLogError(_T("Failed to save %s, %s"), STOREDSEARCHES_FILENAME, buffer);
		error->Delete();
		return;
	}
	DebugLog(_T("Stored %u open search for restoring on next start"), nCount);
}

void CSearchList::LoadSearches(){
	ASSERT( m_listFileLists.GetCount() == 0 );
	CString fullpath = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR);
	fullpath.Append(STOREDSEARCHES_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") STOREDSEARCHES_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			DebugLogError(_T("%s"), strError);
		}
		return;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	try {
		uint8 header = file.ReadUInt8();
		if (header != MET_HEADER_I64TAGS){
			file.Close();
			DebugLogError(_T("Failed to load %s, invalid first byte"), STOREDSEARCHES_FILENAME);
			return;
		}
		uint8 byVersion = file.ReadUInt8();
		if (byVersion != STOREDSEARCHES_VERSION){
			file.Close();
			return;
		}

		uint32 nHighestKadSearchID = 0xFFFFFFFF;
		uint32 nHighestEd2kSearchID = 0xFFFFFFFF;
		uint16 nCount = file.ReadUInt16();
		for (int i = 0; i < nCount; i++){
			SSearchParams* pParams = new SSearchParams(file);
			uint32 nFileCount = file.ReadUInt32();

			// backward compability fix for new automatic option
			if (pParams->eType == SearchTypeAutomatic)
				pParams->eType = SearchTypeEd2kServer;
			else if (pParams->eType == SearchTypeEd2kGlobal && pParams->dwSearchID < 0x80000000)
				pParams->eType = SearchTypeKademlia;
			
			if (pParams->eType == SearchTypeKademlia && (nHighestKadSearchID == 0xFFFFFFFF || nHighestKadSearchID < pParams->dwSearchID)){
				ASSERT( pParams->dwSearchID < 0x80000000 );
				nHighestKadSearchID = pParams->dwSearchID;
			}
			else if (pParams->eType != SearchTypeKademlia && (nHighestEd2kSearchID == 0xFFFFFFFF || nHighestEd2kSearchID < pParams->dwSearchID)){
				ASSERT( pParams->dwSearchID >= 0x80000000 );
				nHighestEd2kSearchID = pParams->dwSearchID;
			}

			// create the new tab
			CStringA strResultType = pParams->strFileType;
			if (strResultType == ED2KFTSTR_PROGRAM)
				strResultType.Empty();
			NewSearch(NULL, strResultType, pParams->dwSearchID, pParams->eType, pParams->strExpression, false);
			
			bool bDeleteParams = false;
			if (theApp.emuledlg->searchwnd->CreateNewTab(pParams, false)){
				m_foundFilesCount.SetAt(pParams->dwSearchID, 0);
				m_foundSourcesCount.SetAt(pParams->dwSearchID, 0);
			}
			else{
				bDeleteParams = true;
				ASSERT( false );
			}

			// fill the list with stored results
			for (uint32 j = 0; j < nFileCount; j++) {
				CSearchFile* toadd = new CSearchFile(&file, true, pParams->dwSearchID, 0, 0, NULL, pParams->eType == SearchTypeKademlia);
				AddToList(toadd, pParams->bClientSharedFiles);
			}
			if (bDeleteParams){
				delete pParams;
				pParams = NULL;
			}
		}
		file.Close();
		// adjust the start values for searchids in order to not reuse IDs of our loaded searches
		if (nHighestKadSearchID != 0xFFFFFFFF)
			Kademlia::CSearchManager::SetNextSearchID(nHighestKadSearchID + 1);
		if (nHighestEd2kSearchID != 0xFFFFFFFF)
			theApp.emuledlg->searchwnd->SetNextSearchID(max(nHighestEd2kSearchID + 1, 0x80000000));
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			DebugLogError(_T("Failed to load %s, corrupt"), STOREDSEARCHES_FILENAME);
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			DebugLogError(_T("Failed to load %s, %s"), STOREDSEARCHES_FILENAME, buffer);
		}
		error->Delete();
		return;
	}
}
