/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "stdafx.h"
#include "./SearchManager.h"
#include "./Search.h"
#include "./Tag.h"
#include "./Defines.h"
#include "./Kademlia.h"
#include "./Indexed.h"
#include "./prefs.h"
#include "../io/IOException.h"
#include "../../resource.h"
#include "../../SafeFile.h"
#include "../../Log.h"
#include "../../emule.h"
#include "../../emuledlg.h"
#include "../../kademliawnd.h"
#include "../../KadSearchListCtrl.h"
#include "../../SearchDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPCSTR g_aszInvKadKeywordCharsA = INV_KAD_KEYWORD_CHARS;
LPCTSTR g_aszInvKadKeywordChars = _T(INV_KAD_KEYWORD_CHARS);
LPCWSTR g_awszInvKadKeywordChars = L" ()[]{}<>,._-!?:;\\/\"";

using namespace Kademlia;

uint32 CSearchManager::m_uNextID = 0;
SearchMap CSearchManager::m_mapSearches;

bool CSearchManager::IsSearching(uint32 uSearchID)
{
	// Check if this searchID is within the searches.
	for (SearchMap::const_iterator itSearchMap = m_mapSearches.begin(); itSearchMap != m_mapSearches.end(); ++itSearchMap)
	{
		if (itSearchMap->second->m_uSearchID == uSearchID)
			return true;
	}
	return false;
}

void CSearchManager::StopSearch(uint32 uSearchID, bool bDelayDelete)
{
	// Stop a specific searchID
	for (SearchMap::iterator itSearchMap = m_mapSearches.begin(); itSearchMap != m_mapSearches.end(); ++itSearchMap)
	{
		if (itSearchMap->second->m_uSearchID == uSearchID)
		{
			// Do not delete as we want to get a chance for late packets to be processed.
			if(bDelayDelete)
				itSearchMap->second->PrepareToStop();
			else
			{
				// Delete this search now.
				// If this method is changed to continue looping, take care of the iterator as we will already
				// be pointing to the next entry and the for-loop could cause you to iterate past the end.
				delete itSearchMap->second;
				m_mapSearches.erase(itSearchMap);
			}
			return;
		}
	}
}

void CSearchManager::StopAllSearches()
{
	// Stop and delete all searches.
	for (SearchMap::iterator itSearchMap = m_mapSearches.begin(); itSearchMap != m_mapSearches.end(); ++itSearchMap)
		delete itSearchMap->second;
	m_mapSearches.clear();
}

bool CSearchManager::StartSearch(CSearch* pSearch)
{
	// A search object was created, now try to start the search.
	if (AlreadySearchingFor(pSearch->m_uTarget))
	{
		// There was already a search in progress with this target.
		delete pSearch;
		return false;
	}
	// Add to the search map
	m_mapSearches[pSearch->m_uTarget] = pSearch;
	// Start the search.
	pSearch->Go();
	return true;
}

CSearch* CSearchManager::PrepareFindKeywords(LPCTSTR szKeyword, UINT uSearchTermsSize, LPBYTE pucSearchTermsData)
{
	// Create a keyword search object.
	CSearch *pSearch = new CSearch;
	try
	{
		// Set search to a keyword type.
		pSearch->SetSearchTypes(CSearch::KEYWORD);

		// Make sure we have a keyword list.
		GetWords(szKeyword, &pSearch->m_listWords);
		if (pSearch->m_listWords.size() == 0)
		{
			delete pSearch;
			throw GetResString(IDS_KAD_SEARCH_KEYWORD_TOO_SHORT);
		}

		// Get the targetID based on the primary keyword.
		CKadTagValueString wstrKeyword = pSearch->m_listWords.front();
		KadGetKeywordHash(wstrKeyword, &pSearch->m_uTarget);

		// Verify that we are not already searching for this target.
		if (AlreadySearchingFor(pSearch->m_uTarget))
		{
			delete pSearch;
			CString strError;
			strError.Format(GetResString(IDS_KAD_SEARCH_KEYWORD_ALREADY_SEARCHING), CString(wstrKeyword));
			throw strError;
		}

		pSearch->SetSearchTermData( uSearchTermsSize, pucSearchTermsData );
		pSearch->SetGUIName(szKeyword);
		// Inc our searchID
		pSearch->m_uSearchID = ++m_uNextID;
		// Insert search into map.
		m_mapSearches[pSearch->m_uTarget] = pSearch;
		// Start search.
		pSearch->Go();
	}
	catch (CIOException* ioe)
	{
		CString strError;
		strError.Format(_T("IO-Exception in %hs: Error %u"), __FUNCTION__, ioe->m_iCause);
		ioe->Delete();
		delete pSearch;
		throw strError;
	}
	catch (CFileException* e)
	{
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		e->m_strFileName = _T("search packet");
		e->GetErrorMessage(szError, ARRSIZE(szError));
		CString strError;
		strError.Format(_T("Exception in %hs: %s"), __FUNCTION__, szError);
		e->Delete();
		delete pSearch;
		throw strError;
	}
	catch (CString strException)
	{
		throw strException;
	}
	catch (...)
	{
		CString strError;
		strError.Format(_T("Unknown exception in %hs"), __FUNCTION__);
		delete pSearch;
		throw strError;
	}
	return pSearch;
}

CSearch* CSearchManager::PrepareLookup(uint32 uType, bool bStart, const CUInt128 &uID)
{
	// Prepare a kad lookup.
	// Make sure this target is not already in progress.
	if(AlreadySearchingFor(uID))
		return NULL;

	// Create a new search.
	CSearch *pSearch = new CSearch;

	// Set type and target.
	pSearch->SetSearchTypes(uType);
	pSearch->m_uTarget = uID;

	try
	{
		switch(pSearch->m_uType)
		{
			case CSearch::STOREKEYWORD:
				if(!Kademlia::CKademlia::GetIndexed()->SendStoreRequest(uID))
				{
					// Keyword Store was determined to be a overloaded node, abort store.
					delete pSearch;
					return NULL;
				}
				break;
		}

		// Inc search ID.
		pSearch->m_uSearchID = ++m_uNextID;
		if( bStart )
		{
			// Auto start this search.
			m_mapSearches[pSearch->m_uTarget] = pSearch;
			pSearch->Go();
		}
	}
	catch ( CIOException *ioe )
	{
		AddDebugLogLine( false, _T("Exception in CSearchManager::PrepareLookup (IO error(%i))"), ioe->m_iCause);
		ioe->Delete();
		delete pSearch;
		return NULL;
	}
	catch (...)
	{
		AddDebugLogLine(false, _T("Exception in CSearchManager::PrepareLookup"));
		delete pSearch;
		return NULL;
	}
	return pSearch;
}

void CSearchManager::FindNode(const CUInt128 &uID, bool bComplete)
{
	// Do a node lookup.
	CSearch *pSearch = new CSearch;
	if(bComplete)
		pSearch->SetSearchTypes(CSearch::NODECOMPLETE);
	else
		pSearch->SetSearchTypes(CSearch::NODE);
	pSearch->m_uTarget = uID;
	StartSearch(pSearch);
}

bool CSearchManager::AlreadySearchingFor(const CUInt128 &uTarget)
{
	// Check if this target is in the search map.
	return (m_mapSearches.count(uTarget) > 0);
}

bool CSearchManager::IsFWCheckUDPSearch(const CUInt128 &uTarget)
{
	// Check if this target is in the search map.
	SearchMap::const_iterator itSearchMap = m_mapSearches.find(uTarget);
	if (itSearchMap != m_mapSearches.end())
		return (itSearchMap->second->GetSearchTypes() == CSearch::NODEFWCHECKUDP);
	return false;
}


void CSearchManager::GetWords(LPCTSTR sz, WordList *plistWords)
{
	LPCTSTR szS = sz;
	size_t uChars = 0;
	size_t uBytes = 0;
	CKadTagValueString sWord;
	while (_tcslen(szS) > 0)
	{
		uChars = _tcscspn(szS, g_aszInvKadKeywordChars);
		sWord = szS;
		sWord.Truncate(uChars);
		// TODO: We'd need a safe way to determine if a sequence which contains only 3 chars is a real word.
		// Currently we do this by evaluating the UTF-8 byte count. This will work well for Western locales,
		// AS LONG AS the min. byte count is 3(!). If the byte count is once changed to 2, this will not
		// work properly any longer because there are a lot of Western characters which need 2 bytes in UTF-8.
		// Maybe we need to evaluate the Unicode character values itself whether the characters are located
		// in code ranges where single characters are known to represent words.
		uBytes = KadGetKeywordBytes(sWord).GetLength();
		if (uBytes >= 3)
		{
			KadTagStrMakeLower(sWord);
			plistWords->remove(sWord);
			plistWords->push_back(sWord);
		}
		szS += uChars;
		if (uChars < _tcslen(szS))
			szS++;
	}

	// if the last word we have added, contains 3 chars (and 3 bytes), it's in almost all cases a file's extension.
	if (plistWords->size() > 1 && (uChars == 3 && uBytes == 3))
		plistWords->pop_back();
}

void CSearchManager::JumpStart()
{
	// Find any searches that has stalled and jumpstart them.
	// This will also prune all searches.
	time_t tNow = time(NULL);
	SearchMap::iterator itSearchMap = m_mapSearches.begin();
	while(itSearchMap != m_mapSearches.end())
	{
		// Each type has it's own criteria for being deleted or jumpstarted.
		switch(itSearchMap->second->GetSearchTypes())
		{
			case CSearch::FILE:
				{
					if (itSearchMap->second->m_tCreated + SEARCHFILE_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHFILE_TOTAL || itSearchMap->second->m_tCreated + SEARCHFILE_LIFETIME - SEC(20) < tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::KEYWORD:
				{
					if (itSearchMap->second->m_tCreated + SEARCHKEYWORD_LIFETIME < tNow)
					{
						// Tell GUI that search ended
						if (theApp.emuledlg && theApp.emuledlg->searchwnd)
							theApp.emuledlg->searchwnd->CancelKadSearch(itSearchMap->second->GetSearchID());
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHKEYWORD_TOTAL || itSearchMap->second->m_tCreated + SEARCHKEYWORD_LIFETIME - SEC(20) < tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::NOTES:
				{
					if (itSearchMap->second->m_tCreated + SEARCHNOTES_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHNOTES_TOTAL || itSearchMap->second->m_tCreated + SEARCHNOTES_LIFETIME - SEC(20) < tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::FINDBUDDY:
				{
					if (itSearchMap->second->m_tCreated + SEARCHFINDBUDDY_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHFINDBUDDY_TOTAL || itSearchMap->second->m_tCreated + SEARCHFINDBUDDY_LIFETIME - SEC(20) < tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::FINDSOURCE:
				{
					if (itSearchMap->second->m_tCreated + SEARCHFINDSOURCE_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHFINDSOURCE_TOTAL || itSearchMap->second->m_tCreated + SEARCHFINDSOURCE_LIFETIME - SEC(20) < tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::NODE:
			case CSearch::NODESPECIAL:
			case CSearch::NODEFWCHECKUDP:
				{
					if (itSearchMap->second->m_tCreated + SEARCHNODE_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::NODECOMPLETE:
				{
					if (itSearchMap->second->m_tCreated + SEARCHNODE_LIFETIME < tNow)
					{
						// Tell Kad that it can start publishing.
						CKademlia::GetPrefs()->SetPublish(true);
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if ((itSearchMap->second->m_tCreated + SEARCHNODECOMP_LIFETIME < tNow) && (itSearchMap->second->GetAnswers() >= SEARCHNODECOMP_TOTAL))
					{
						// Tell Kad that it can start publishing.
						CKademlia::GetPrefs()->SetPublish(true);
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::STOREFILE:
				{
					if (itSearchMap->second->m_tCreated + SEARCHSTOREFILE_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHSTOREFILE_TOTAL || itSearchMap->second->m_tCreated + SEARCHSTOREFILE_LIFETIME - SEC(20) < tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::STOREKEYWORD:
				{
					if (itSearchMap->second->m_tCreated + SEARCHSTOREKEYWORD_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHSTOREKEYWORD_TOTAL || itSearchMap->second->m_tCreated + SEARCHSTOREKEYWORD_LIFETIME - SEC(20)< tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			case CSearch::STORENOTES:
				{
					if (itSearchMap->second->m_tCreated + SEARCHSTORENOTES_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else if (itSearchMap->second->GetAnswers() >= SEARCHSTORENOTES_TOTAL || itSearchMap->second->m_tCreated + SEARCHSTORENOTES_LIFETIME - SEC(20)< tNow)
						itSearchMap->second->PrepareToStop();
					else
						itSearchMap->second->JumpStart();
					break;
				}
			default:
				{
					if (itSearchMap->second->m_tCreated + SEARCH_LIFETIME < tNow)
					{
						delete itSearchMap->second;
						itSearchMap = m_mapSearches.erase(itSearchMap);
						//Don't do anything after this.. We are already at the next entry.
						continue;
					}
					else
						itSearchMap->second->JumpStart();
					break;
				}
		}
		++itSearchMap;
	}
}

void CSearchManager::UpdateStats()
{
	// Update stats on the searches, this info can be used to determine if we need can start new searches.
	uint8 uTotalFile = 0;
	uint8 uTotalStoreSrc = 0;
	uint8 uTotalStoreKey = 0;
	uint8 uTotalSource = 0;
	uint8 uTotalNotes = 0;
	uint8 uTotalStoreNotes = 0;
	for (SearchMap::const_iterator itSearchMap = m_mapSearches.begin(); itSearchMap != m_mapSearches.end(); ++itSearchMap)
	{
		switch(itSearchMap->second->GetSearchTypes())
		{
			case CSearch::FILE:
				uTotalFile++;
				break;
			case CSearch::STOREFILE:
				uTotalStoreSrc++;
				break;
			case CSearch::STOREKEYWORD:
				uTotalStoreKey++;
				break;
			case CSearch::FINDSOURCE:
				uTotalSource++;
				break;
			case CSearch::STORENOTES:
				uTotalStoreNotes++;
				break;
			case CSearch::NOTES:
				uTotalNotes++;
				break;
		}
	}
	CPrefs *pPrefs = CKademlia::GetPrefs();
	if(pPrefs)
	{
		pPrefs->SetTotalFile(uTotalFile);
		pPrefs->SetTotalStoreSrc(uTotalStoreSrc);
		pPrefs->SetTotalStoreKey(uTotalStoreKey);
		pPrefs->SetTotalSource(uTotalSource);
		pPrefs->SetTotalNotes(uTotalNotes);
		pPrefs->SetTotalStoreNotes(uTotalStoreNotes);
	}
}

void CSearchManager::ProcessPublishResult(const CUInt128 &uTarget, const uint8 uLoad, const bool bLoadResponse)
{
	// We tried to publish some info and got a result.
	CSearch *pSearch = NULL;
	SearchMap::const_iterator itSearchMap = m_mapSearches.find(uTarget);
	if (itSearchMap != m_mapSearches.end())
		pSearch = itSearchMap->second;

	// Result could be very late and store deleted, abort.
	if (pSearch == NULL)
		return;

	switch(pSearch->GetSearchTypes())
	{
		case CSearch::STOREKEYWORD:
			if( bLoadResponse )
				pSearch->UpdateNodeLoad( uLoad );
			break;
		case CSearch::STOREFILE:
		case CSearch::STORENOTES:
			break;
	}

	// Inc the number of answers.
	pSearch->m_uAnswers++;
	// Update the search for the GUI
	theApp.emuledlg->kademliawnd->searchList->SearchRef(pSearch);
}


void CSearchManager::ProcessResponse(const CUInt128 &uTarget, uint32 uFromIP, uint16 uFromPort, ContactList *plistResults)
{
	// We got a response to a kad lookup.
	CSearch *pSearch = NULL;
	SearchMap::const_iterator itSearchMap= m_mapSearches.find(uTarget);
	if (itSearchMap != m_mapSearches.end())
		pSearch = itSearchMap->second;

	// If this search was deleted before this response, delete contacts and abort, otherwise process them.
	if (pSearch == NULL)
	{
		for (ContactList::const_iterator itContactList = plistResults->begin(); itContactList != plistResults->end(); ++itContactList)
			delete (*itContactList);
		delete plistResults;
		return;
	}
	else
		pSearch->ProcessResponse(uFromIP, uFromPort, plistResults);
}

uint8 CSearchManager::GetExpectedResponseContactCount(const CUInt128 &uTarget)
{
	CSearch *pSearch = NULL;
	SearchMap::const_iterator itSearchMap= m_mapSearches.find(uTarget);
	if (itSearchMap != m_mapSearches.end())
		pSearch = itSearchMap->second;

	// If this search was deleted before this response, delete contacts and abort, otherwise process them.
	if (pSearch == NULL)
	{
		return 0;
	}
	else
		return pSearch->GetRequestContactCount();
}

void CSearchManager::ProcessResult(const CUInt128 &uTarget, const CUInt128 &uAnswer, TagList *plistInfo, uint32 uFromIP, uint16 uFromPort)
{
	// We have results for a request for info.
	CSearch *pSearch = NULL;
	SearchMap::const_iterator itSearchMap = m_mapSearches.find(uTarget);
	if (itSearchMap != m_mapSearches.end())
		pSearch = itSearchMap->second;

	// If this search was deleted before these results, delete contacts and abort, otherwise process them.
	if (pSearch == NULL)
	{
		for (TagList::const_iterator itTagList = plistInfo->begin(); itTagList != plistInfo->end(); ++itTagList)
			delete *itTagList;
		delete plistInfo;
	}
	else
		pSearch->ProcessResult(uAnswer, plistInfo, uFromIP, uFromPort);
}

bool CSearchManager::FindNodeSpecial(const CUInt128 &uID, CKadClientSearcher* pRequester){
	// Do a node lookup.
	CString strDbgID;
	uID.ToHexString(&strDbgID);
	DebugLog(_T("Starting NODESPECIAL Kad Search for %s"), strDbgID);
	CSearch *pSearch = new CSearch;
	pSearch->SetSearchTypes(CSearch::NODESPECIAL);
	pSearch->m_uTarget = uID;
	pSearch->SetNodeSpecialSearchRequester(pRequester);
	return StartSearch(pSearch);
}

bool CSearchManager::FindNodeFWCheckUDP(){
	CancelNodeFWCheckUDPSearch();
	CUInt128 uID;
	uID.SetValueRandom();
	DebugLog(_T("Starting NODEFWCHECKUDP Kad Search"));
	CSearch *pSearch = new CSearch;
	pSearch->SetSearchTypes(CSearch::NODEFWCHECKUDP);
	pSearch->m_uTarget = uID;
	return StartSearch(pSearch);
}

void CSearchManager::CancelNodeSpecial(CKadClientSearcher* pRequester){
	// Stop a specific nodespecialsearch
	for (SearchMap::iterator itSearchMap = m_mapSearches.begin(); itSearchMap != m_mapSearches.end(); ++itSearchMap)
	{
		if (itSearchMap->second->GetNodeSpecialSearchRequester() == pRequester)
		{
			itSearchMap->second->SetNodeSpecialSearchRequester(NULL);
			itSearchMap->second->PrepareToStop();
			return;
		}
	}
}

void CSearchManager::CancelNodeFWCheckUDPSearch(){
	// Stop node searches done for udp firewallcheck
	for (SearchMap::iterator itSearchMap = m_mapSearches.begin(); itSearchMap != m_mapSearches.end(); ++itSearchMap)
	{
		if (itSearchMap->second->GetSearchTypes() == CSearch::NODEFWCHECKUDP)
		{
			itSearchMap->second->PrepareToStop();
		}
	}
}
