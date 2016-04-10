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
#include "./Entry.h"
#include "../../Log.h"
#include "../../OtherFunctions.h"
#include "../../SafeFile.h"
#include "./Indexed.h"
#include "../io/DataIO.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;

CMap<uint32, uint32, uint32, uint32> CKeyEntry::s_mapGlobalPublishIPs;

CEntry::CEntry()
{
	m_uIP = 0;
	m_uTCPPort = 0;
	m_uUDPPort = 0;
	m_uSize = 0;
	m_tLifetime = 0;
	m_bSource = false;
}

CEntry::~CEntry()
{
	for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); itTagList++)
		delete *itTagList;
}

CEntry* CEntry::Copy()
{
	CEntry* pEntry = new CEntry();
	for (POSITION pos = m_listFileNames.GetHeadPosition(); pos != NULL;){
		pEntry->m_listFileNames.AddTail(m_listFileNames.GetNext(pos));
	}
	pEntry->m_uIP = m_uIP;
	pEntry->m_uKeyID.SetValue(m_uKeyID);
	pEntry->m_tLifetime = m_tLifetime;
	pEntry->m_uSize = m_uSize;
	pEntry->m_bSource = m_bSource;
	pEntry->m_uSourceID.SetValue(m_uSourceID);
	pEntry->m_uTCPPort = m_uTCPPort;
	pEntry->m_uUDPPort = m_uUDPPort;
	for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); itTagList++)
	{
		CKadTag* pTag = *itTagList;
		pEntry->m_listTag.push_back(pTag->Copy());
	}
	return pEntry;
}

uint64 CEntry::GetIntTagValue(CKadTagNameString strTagName, bool bIncludeVirtualTags) const
{
	uint64 uResult = 0;
	GetIntTagValue(strTagName, uResult, bIncludeVirtualTags);
	return uResult;
}

bool CEntry::GetIntTagValue(CKadTagNameString strTagName, uint64& rValue, bool bIncludeVirtualTags) const
{
	for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); itTagList++)
	{
		CKadTag* pTag = *itTagList;
		if (pTag->IsInt() && !pTag->m_name.Compare(strTagName)){
			rValue = pTag->GetInt();
			return true;
		}
	}

	if (bIncludeVirtualTags)
	{
		// SizeTag is not stored anymore, but queried in some places
		if (!strTagName.Compare(TAG_FILESIZE)){
			rValue = m_uSize;
			return true;
		}
	}
	rValue = 0;
	return false;
}

CKadTagValueString CEntry::GetStrTagValue(CKadTagNameString strTagName) const
{
	for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); itTagList++)
	{
		CKadTag* pTag = *itTagList;
		if (!pTag->m_name.Compare(strTagName) && pTag->IsStr())
			return pTag->GetStr();
	}
	return L"";
}

void CEntry::SetFileName(CKadTagValueString strName){
	if (!m_listFileNames.IsEmpty()){
		ASSERT( false );
		m_listFileNames.RemoveAll();
	}
	structFileNameEntry structFN = {strName, 1};
	m_listFileNames.AddHead(structFN);
}

CKadTagValueString CEntry::GetCommonFileName() const
{
	// return the filename on which most publishers seem to agree on
	// due to the counting, this doesn't has to be excact, we just want to make sure to not use a filename which just
	// a few bad publishers used and base or search matching and answering on this, instead of the most popular name
	// Note: The Index values are not the acutal numbers of publishers, but just a relativ number to compare to other entries
	POSITION posResult = NULL;
	uint32 nHighestPopularityIndex = 0;
	for (POSITION pos = m_listFileNames.GetHeadPosition(); pos != NULL;){
		POSITION posPrev = pos;
		const structFileNameEntry& rCur = m_listFileNames.GetNext(pos);
		if (rCur.m_uPopularityIndex > nHighestPopularityIndex){
			nHighestPopularityIndex = rCur.m_uPopularityIndex;
			posResult = posPrev;
		}
	}
	CKadTagValueString strResult(posResult != NULL 
		? m_listFileNames.GetAt(posResult).m_fileName : L"");
	ASSERT( !strResult.IsEmpty() || m_listFileNames.IsEmpty() );
	return strResult;
}

CKadTagValueString CEntry::GetCommonFileNameLowerCase() const
{
	CKadTagValueString strResult = GetCommonFileName();
	if (!strResult.IsEmpty())
		KadTagStrMakeLower(strResult);
	return strResult;
}

uint32 CEntry::GetTagCount() const // Adds filename and size to the count if not empty, even if they are not stored as tags
{
	return m_listTag.size() + ((m_uSize != 0) ? 1 : 0) + (GetCommonFileName().IsEmpty() ? 0 : 1);
}

void CEntry::WriteTagListInc(CDataIO* pData, uint32 nIncreaseTagNumber){
	// write taglist and add name + size tag
	if (pData == NULL){
		ASSERT( false );
		return;
	}

	uint32 uCount = GetTagCount() + nIncreaseTagNumber; // will include name and size tag in the count if needed
	ASSERT( uCount <= 0xFF );
	pData->WriteByte((uint8)uCount);

	CKadTagValueString strCommonFileName(GetCommonFileName());
	if (!strCommonFileName.IsEmpty()){
		ASSERT( uCount > m_listTag.size() );
		CKadTagStr tag(TAG_FILENAME, strCommonFileName);
		pData->WriteTag(&tag);
	}
	if (m_uSize != 0){
		ASSERT( uCount > m_listTag.size() );
		CKadTagUInt tag(TAG_FILESIZE, m_uSize);
		pData->WriteTag(&tag);
	}

	for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
		pData->WriteTag(*itTagList);
}

void CEntry::AddTag(CKadTag* pTag, uint32 uDbgSourceIP)
{
	// Filter tags which are for sending query results only and should never be stored (or even worse sent within the taglist)
	if (!pTag->m_name.Compare(TAG_KADAICHHASHRESULT))
	{
		DebugLogWarning(_T("Received result tag TAG_KADAICHHASHRESULT on publishing, filtered, source %s"), ipstr(ntohl(uDbgSourceIP)));
		delete pTag;
	}
	else if (!pTag->m_name.Compare(TAG_PUBLISHINFO))
	{
		DebugLogWarning(_T("Received result tag TAG_PUBLISHINFO on publishing, filtered, source %s"), ipstr(ntohl(uDbgSourceIP)));
		delete pTag;
	}
	else
		m_listTag.push_back(pTag);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// CKeyEntry
CKeyEntry::CKeyEntry()
{
	m_pliPublishingIPs = NULL;
	m_fTrustValue = 0;
	dwLastTrustValueCalc = 0;
}

CKeyEntry::~CKeyEntry()
{
	if (m_pliPublishingIPs != NULL){
		while(m_pliPublishingIPs->GetHeadPosition() != NULL){
			structPublishingIP curEntry = m_pliPublishingIPs->RemoveHead();
			AdjustGlobalPublishTracking(curEntry.m_uIP, false, _T("instance delete"));
		}
		delete m_pliPublishingIPs;
		m_pliPublishingIPs = NULL;
	}
}

bool CKeyEntry::StartSearchTermsMatch(const SSearchTerm* pSearchTerm)
{
	m_strSearchTermCacheCommonFileNameLowerCase = GetCommonFileNameLowerCase();
	bool bResult = SearchTermsMatch(pSearchTerm);
	m_strSearchTermCacheCommonFileNameLowerCase.Empty();
	return bResult;
}

bool CKeyEntry::SearchTermsMatch(const SSearchTerm* pSearchTerm) const
{
	// boolean operators
	if (pSearchTerm->m_type == SSearchTerm::AND)
		return SearchTermsMatch(pSearchTerm->m_pLeft) && SearchTermsMatch(pSearchTerm->m_pRight);

	if (pSearchTerm->m_type == SSearchTerm::OR)
		return SearchTermsMatch(pSearchTerm->m_pLeft) || SearchTermsMatch(pSearchTerm->m_pRight);

	if (pSearchTerm->m_type == SSearchTerm::NOT)
		return SearchTermsMatch(pSearchTerm->m_pLeft) && !SearchTermsMatch(pSearchTerm->m_pRight);

	// word which is to be searched in the file name (and in additional meta data as done by some ed2k servers???)
	if (pSearchTerm->m_type == SSearchTerm::String)
	{
		int iStrSearchTerms = pSearchTerm->m_pastr->GetCount();
		if (iStrSearchTerms == 0)
			return false;
		// if there are more than one search strings specified (e.g. "aaa bbb ccc") the entire string is handled
		// like "aaa AND bbb AND ccc". search all strings from the string search term in the tokenized list of
		// the file name. all strings of string search term have to be found (AND)
		for (int iSearchTerm = 0; iSearchTerm < iStrSearchTerms; iSearchTerm++)
		{
			// this will not give the same results as when tokenizing the filename string, but it is 20 times faster.
			if (wcsstr(m_strSearchTermCacheCommonFileNameLowerCase, pSearchTerm->m_pastr->GetAt(iSearchTerm)) == NULL)
				return false;
		}
		return true;
	}

	if (pSearchTerm->m_type == SSearchTerm::MetaTag)
	{
		if (pSearchTerm->m_pTag->m_type == 2) // meta tags with string values
		{
			if (pSearchTerm->m_pTag->m_name.Compare(TAG_FILEFORMAT) == 0)
			{
				// 21-Sep-2006 []: Special handling for TAG_FILEFORMAT which is already part
				// of the filename and thus does not need to get published nor stored explicitly,
				int iExt = m_strSearchTermCacheCommonFileNameLowerCase.ReverseFind(_T('.'));
				if (iExt != -1)
				{
					if (KadTagStrCompareNoCase((LPCWSTR)m_strSearchTermCacheCommonFileNameLowerCase + iExt + 1, pSearchTerm->m_pTag->GetStr()) == 0)
						return true;
				}
			}
			else
			{
				for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
				{
					const CKadTag* pTag = *itTagList;
					if (pTag->IsStr() && pSearchTerm->m_pTag->m_name.Compare(pTag->m_name) == 0)
						return KadTagStrCompareNoCase(pTag->GetStr(), pSearchTerm->m_pTag->GetStr()) == 0;
				}
			}
		}
	}
	else if (pSearchTerm->m_type == SSearchTerm::OpGreaterEqual)
	{
		if (pSearchTerm->m_pTag->IsInt()) // meta tags with integer values
		{
			uint64 uValue;
			if (GetIntTagValue(pSearchTerm->m_pTag->m_name, uValue, true)){
				return uValue >= pSearchTerm->m_pTag->GetInt();
			}
		}
		else if (pSearchTerm->m_pTag->IsFloat()) // meta tags with float values
		{
			for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
			{
				const Kademlia::CKadTag* pTag = *itTagList;
				if (pTag->IsFloat() && pSearchTerm->m_pTag->m_name.Compare(pTag->m_name) == 0)
					return pTag->GetFloat() >= pSearchTerm->m_pTag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->m_type == SSearchTerm::OpLessEqual)
	{
		if (pSearchTerm->m_pTag->IsInt()) // meta tags with integer values
		{
			uint64 uValue;
			if (GetIntTagValue(pSearchTerm->m_pTag->m_name, uValue, true)){
				return uValue <= pSearchTerm->m_pTag->GetInt();
			}
		}
		else if (pSearchTerm->m_pTag->IsFloat()) // meta tags with float values
		{
			for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
			{
				const Kademlia::CKadTag* pTag = *itTagList;
				if (pTag->IsFloat() && pSearchTerm->m_pTag->m_name.Compare(pTag->m_name) == 0)
					return pTag->GetFloat() <= pSearchTerm->m_pTag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->m_type == SSearchTerm::OpGreater)
	{
		if (pSearchTerm->m_pTag->IsInt()) // meta tags with integer values
		{
			uint64 uValue;
			if (GetIntTagValue(pSearchTerm->m_pTag->m_name, uValue, true)){
				return uValue > pSearchTerm->m_pTag->GetInt();
			}
		}
		else if (pSearchTerm->m_pTag->IsFloat()) // meta tags with float values
		{
			for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
			{
				const Kademlia::CKadTag* pTag = *itTagList;
				if (pTag->IsFloat() && pSearchTerm->m_pTag->m_name.Compare(pTag->m_name) == 0)
					return pTag->GetFloat() > pSearchTerm->m_pTag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->m_type == SSearchTerm::OpLess)
	{
		if (pSearchTerm->m_pTag->IsInt()) // meta tags with integer values
		{
			uint64 uValue;
			if (GetIntTagValue(pSearchTerm->m_pTag->m_name, uValue, true)){
				return uValue < pSearchTerm->m_pTag->GetInt();
			}
		}
		else if (pSearchTerm->m_pTag->IsFloat()) // meta tags with float values
		{
			for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
			{
				const Kademlia::CKadTag* pTag = *itTagList;
				if (pTag->IsFloat() && pSearchTerm->m_pTag->m_name.Compare(pTag->m_name) == 0)
					return pTag->GetFloat() < pSearchTerm->m_pTag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->m_type == SSearchTerm::OpEqual)
	{
		if (pSearchTerm->m_pTag->IsInt()) // meta tags with integer values
		{
			uint64 uValue;
			if (GetIntTagValue(pSearchTerm->m_pTag->m_name, uValue, true)){
				return uValue == pSearchTerm->m_pTag->GetInt();
			}
		}
		else if (pSearchTerm->m_pTag->IsFloat()) // meta tags with float values
		{
			for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
			{
				const Kademlia::CKadTag* pTag = *itTagList;
				if (pTag->IsFloat() && pSearchTerm->m_pTag->m_name.Compare(pTag->m_name) == 0)
					return pTag->GetFloat() == pSearchTerm->m_pTag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->m_type == SSearchTerm::OpNotEqual)
	{
		if (pSearchTerm->m_pTag->IsInt()) // meta tags with integer values
		{
			uint64 uValue;
			if (GetIntTagValue(pSearchTerm->m_pTag->m_name, uValue, true)){
				return uValue != pSearchTerm->m_pTag->GetInt();
			}
		}
		else if (pSearchTerm->m_pTag->IsFloat()) // meta tags with float values
		{
			for (TagList::const_iterator itTagList = m_listTag.begin(); itTagList != m_listTag.end(); ++itTagList)
			{
				const Kademlia::CKadTag* pTag = *itTagList;
				if (pTag->IsFloat() && pSearchTerm->m_pTag->m_name.Compare(pTag->m_name) == 0)
					return pTag->GetFloat() != pSearchTerm->m_pTag->GetFloat();
			}
		}
	}

	return false;
}

void CKeyEntry::AdjustGlobalPublishTracking(uint32 uIP, bool bIncrease, CString strDbgReason){
	uint32 nCount = 0;
	BOOL bFound = s_mapGlobalPublishIPs.Lookup(uIP & 0xFFFFFF00 /* /24 netmask, take care of endian if needed*/, nCount);
	if (bIncrease)
		nCount++;
	else
		nCount--;
	if (bFound || bIncrease)
		s_mapGlobalPublishIPs.SetAt(uIP & 0xFFFFFF00, nCount);
	else
		ASSERT( false );
	//LOGTODO
	//if (!strDbgReason.IsEmpty())
	//	DebugLog(_T("KadEntryTack: %s %s (%s) - (%s), new count %u"), (bIncrease ? _T("Adding") : _T("Removing")), ipstr(ntohl(uIP & 0xFFFFFF00)), ipstr(ntohl(uIP)), strDbgReason, nCount);
}

void CKeyEntry::MergeIPsAndFilenames(CKeyEntry* pFromEntry){
	// this is called when replaceing a stored entry with a refreshed one. 
	// we want to take over the tracked IPs, AICHHash and the different filesnames from the old entry, the rest is still
	// "overwritten" with the refreshed values. This might be not perfect for the taglist in some cases, but we cant afford
	// to store hundrets of taglists to figure out the best one like we do for the filenames now
	if (m_pliPublishingIPs != NULL){ // This instance needs to be a new entry, otherwise we don't want/need to merge
		ASSERT( pFromEntry == NULL );
		ASSERT( !m_pliPublishingIPs->IsEmpty() );
		ASSERT( !m_listFileNames.IsEmpty() );
		return;
	}
	ASSERT( m_aAICHHashs.GetCount() <= 1 );
	//fetch the "new" AICH hash if any
	CAICHHash* pNewAICHHash = NULL;
	if ( !m_aAICHHashs.IsEmpty() )
	{
		pNewAICHHash = new CAICHHash(m_aAICHHashs[0]);
		m_aAICHHashs.RemoveAll();
		m_anAICHHashPopularity.RemoveAll();
	}
	bool bRefresh = false;
	if (pFromEntry == NULL || pFromEntry->m_pliPublishingIPs == NULL){
		ASSERT( pFromEntry == NULL );
		// if called with NULL, this is a complete new entry and we need to initalize our lists
		if (m_pliPublishingIPs == NULL)
			m_pliPublishingIPs = new CList<structPublishingIP>();
		// update the global track map below
	}
	else{
		delete m_pliPublishingIPs; // should be always NULL, already ASSERTed above if not

		//  copy over the existing ones.
		m_aAICHHashs.Copy(pFromEntry->m_aAICHHashs);
		m_anAICHHashPopularity.Copy(pFromEntry->m_anAICHHashPopularity);

		// merge the tracked IPs, add this one if not already on the list
		m_pliPublishingIPs = pFromEntry->m_pliPublishingIPs;
		pFromEntry->m_pliPublishingIPs = NULL;	
		bool bFastRefresh = false;
		for (POSITION pos = m_pliPublishingIPs->GetHeadPosition(); pos != NULL; m_pliPublishingIPs->GetNext(pos)){
			structPublishingIP Cur = m_pliPublishingIPs->GetAt(pos);
			if (Cur.m_uIP == m_uIP)
			{
				bRefresh = true;
				if ((time(NULL) - Cur.m_tLastPublish) < (KADEMLIAREPUBLISHTIMES - HR2S(1))){
					DEBUG_ONLY( DebugLog(_T("KadEntryTracking: FastRefresh publish, ip: %s"), ipstr(ntohl(m_uIP))) );
					bFastRefresh = true; // refreshed faster than expected, will not count into filenamepopularity index
				}
				Cur.m_tLastPublish = time(NULL);
				m_pliPublishingIPs->RemoveAt(pos);
				m_pliPublishingIPs->AddTail(Cur);
				// Has the AICH Hash this publisher reported changed?
				if (pNewAICHHash != NULL)
				{
					if (Cur.m_byAICHHashIdx != _UI16_MAX && m_aAICHHashs[Cur.m_byAICHHashIdx] != *pNewAICHHash)
					{
						DebugLogWarning(_T("KadEntryTracking: AICH Hash changed, publisher ip: %s"), ipstr(ntohl(m_uIP)));
						AddRemoveAICHHash(m_aAICHHashs[Cur.m_byAICHHashIdx], false);
						Cur.m_byAICHHashIdx = AddRemoveAICHHash(*pNewAICHHash, true);
					}
					else if (Cur.m_byAICHHashIdx == _UI16_MAX)
					{
						DEBUG_ONLY( DebugLog(_T("KadEntryTracking: New AICH Hash during publishing (publisher reported none before), publisher ip: %s"), ipstr(ntohl(m_uIP))) );
						Cur.m_byAICHHashIdx = AddRemoveAICHHash(*pNewAICHHash, true);
					}
				}
				else if (Cur.m_byAICHHashIdx != _UI16_MAX)
				{
					DebugLogWarning(_T("KadEntryTracking: AICH Hash removed, publisher ip: %s"), ipstr(ntohl(m_uIP)));
					AddRemoveAICHHash(m_aAICHHashs[Cur.m_byAICHHashIdx], false);
					Cur.m_byAICHHashIdx = _UI16_MAX;
				}
				break;
			}
		}
		// copy over trust value, in case we dont want to recalculate
		m_fTrustValue = pFromEntry->m_fTrustValue;
		dwLastTrustValueCalc = pFromEntry->dwLastTrustValueCalc;

		// copy over the different names, if they are different the one we have right now
		ASSERT( m_listFileNames.GetCount() == 1 ); // we should have only one name here, since its the entry from one sinlge source
		structFileNameEntry structCurrentName = {_T(""), 0};;
		if (m_listFileNames.GetHeadPosition() != NULL)
			structCurrentName = m_listFileNames.RemoveHead();
		
		bool bDuplicate = false;
		for (POSITION pos = pFromEntry->m_listFileNames.GetHeadPosition(); pos != NULL; pFromEntry->m_listFileNames.GetNext(pos)){
			structFileNameEntry structNameToCopy = pFromEntry->m_listFileNames.GetAt(pos);
			if (KadTagStrCompareNoCase(structCurrentName.m_fileName, structNameToCopy.m_fileName) == 0){
				// the filename of our new entry matches with our old, increase the popularity index for the old one
				bDuplicate = true;
				if (!bFastRefresh)
					structNameToCopy.m_uPopularityIndex++;
			}
			m_listFileNames.AddTail(structNameToCopy);
		}
		if (!bDuplicate)
			m_listFileNames.AddTail(structCurrentName);
	}
	// if this was a refresh done, otherwise update the global track map
	if (!bRefresh){
		ASSERT( m_uIP != 0 );
		uint16 nAICHHashIdx;
		if (pNewAICHHash != NULL)
			nAICHHashIdx = AddRemoveAICHHash(*pNewAICHHash, true);
		else
			nAICHHashIdx = _UI16_MAX;
		structPublishingIP add = { m_uIP, time(NULL), nAICHHashIdx };
		m_pliPublishingIPs->AddTail(add);

		// add the publisher to the tacking list
		AdjustGlobalPublishTracking(m_uIP, true, _T("new publisher"));

		// we keep track of max 100 IPs, in order to avoid too much time for calculation/storing/loading.
		if (m_pliPublishingIPs->GetCount() > 100){
			structPublishingIP curEntry = m_pliPublishingIPs->RemoveHead();
			if (curEntry.m_byAICHHashIdx != _UI16_MAX)
				VERIFY( AddRemoveAICHHash(m_aAICHHashs[curEntry.m_byAICHHashIdx], false) == curEntry.m_byAICHHashIdx );
			AdjustGlobalPublishTracking(curEntry.m_uIP, false, _T("more than 100 publishers purge"));
		}
		// since we added a new publisher, we want to (re)calcualte the trust value for this entry		
		RecalcualteTrustValue();
	}
	delete pNewAICHHash;
	/*//DEBUG_ONLY( 
		DebugLog(_T("Kad: EntryTrack: Indexed Keyword, Refresh: %s, Current Publisher: %s, Total Publishers: %u, Total different Names: %u,TrustValue: %.2f, file: %s"),
			(bRefresh ? _T("Yes") : _T("No")), ipstr(ntohl(m_uIP)), m_pliPublishingIPs->GetCount(), m_listFileNames.GetCount(), m_fTrustValue, m_uSourceID.ToHexString());
		//);*/
	/*if (m_aAICHHashs.GetCount() == 1)
	{
			DebugLog(_T("Kad: EntryTrack: Indexed Keyword, Refresh: %s, Current Publisher: %s, Total Publishers: %u, Total different Names: %u,TrustValue: %.2f, file: %s, AICH Hash: %s, Popularity: %u"),
			(bRefresh ? _T("Yes") : _T("No")), ipstr(ntohl(m_uIP)), m_pliPublishingIPs->GetCount(), m_listFileNames.GetCount(), m_fTrustValue, m_uSourceID.ToHexString(), m_aAICHHashs[0].GetString(), m_anAICHHashPopularity[0]);
	}
	else if (m_aAICHHashs.GetCount() > 1)
	{
			DebugLog(_T("Kad: EntryTrack: Indexed Keyword, Refresh: %s, Current Publisher: %s, Total Publishers: %u, Total different Names: %u,TrustValue: %.2f, file: %s, AICH Hash: %u - dumping"),
			(bRefresh ? _T("Yes") : _T("No")), ipstr(ntohl(m_uIP)), m_pliPublishingIPs->GetCount(), m_listFileNames.GetCount(), m_fTrustValue, m_uSourceID.ToHexString(), m_aAICHHashs.GetCount());
			for (int i = 0; i < m_aAICHHashs.GetCount(); i++)
			{
				DebugLog(_T("Hash: %s, Populalrity: %u"),  m_aAICHHashs[i].GetString(), m_anAICHHashPopularity[i]);
			}
	}*/
}

void CKeyEntry::RecalcualteTrustValue(){
#define		PUBLISHPOINTSSPERSUBNET			10.0f
	// The trustvalue is supposed to be an indicator how trustworthy/important (or spamy) this entry is and lies between 0 and ~10000,
	// but mostly we say everything below 1 is bad, everything above 1 is good. It is calculated by looking at how many differnt
	// IPs/24 have published this entry and how many entries each of those IPs have.
	// Each IP/24 has x (say 3) points. This means if one IP publishs 3 differnt entries without any other IP publishing those entries,
	// each of those entries will have 3 / 3 = 1 Trustvalue. Thats fine. If it publishes 6 alone, each entry has 3 / 6 = 0.5 trustvalue - not so good
	// However if there is another publisher for entry 5, which only publishes this entry then we have 3/6 + 3/1 = 3.5 trustvalue for this entry
	//
	// Whats the point? With this rating we try to avoid getting spammed with entries for a given keyword by a small IP range, which blends out
	// all other entries for this keyword do to its amount as well as giving an indicator for the searcher. So if we are the node to index "Knoppix", and someone
	// from 1 IP publishes 500 times "knoppix casino 500% bonus.txt", all those entries will have a trsutvalue of 0.006 and we make sure that
	// on search requests for knoppix, those entries are only returned after all entries with a trustvalue > 1 were sent (if there is still space).
	//
	// Its important to note that entry with < 1 do NOT get ignored or singled out, this only comes into play if we have 300 more results for
	// a search request rating > 1
	if (m_pliPublishingIPs == NULL){
		ASSERT( false );
		return;
	}
	dwLastTrustValueCalc = ::GetTickCount();
	m_fTrustValue = 0;
	ASSERT( !m_pliPublishingIPs->IsEmpty() );
	for (POSITION pos = m_pliPublishingIPs->GetHeadPosition(); pos != NULL; m_pliPublishingIPs->GetNext(pos)){
		structPublishingIP curEntry = m_pliPublishingIPs->GetAt(pos);
		uint32 nCount = 0;
		s_mapGlobalPublishIPs.Lookup(curEntry.m_uIP & 0xFFFFFF00 /* /24 netmask, take care of endian if needed*/, nCount);
		if (nCount > 0){
			m_fTrustValue += PUBLISHPOINTSSPERSUBNET / nCount;
		}
		else {
			DebugLogError(_T("Kad: EntryTrack: Inconsistency RecalcualteTrustValue()"));
			ASSERT( false );
		}
	}
}

float CKeyEntry::GetTrustValue(){
	// update if last calcualtion is too old, will assert if this entry is not supposed to have a trustvalue
	if (::GetTickCount() - dwLastTrustValueCalc > MIN2MS(10))
		RecalcualteTrustValue();
	return m_fTrustValue;
}

void CKeyEntry::CleanUpTrackedPublishers(){
	if (m_pliPublishingIPs == NULL)
		return;
	time_t now = time(NULL);
	while (m_pliPublishingIPs->GetHeadPosition() != NULL){
		// entries are ordered, older ones first
		structPublishingIP curEntry = m_pliPublishingIPs->GetHead();
		if (now - curEntry.m_tLastPublish > KADEMLIAREPUBLISHTIMEK){
			AdjustGlobalPublishTracking(curEntry.m_uIP, false, _T("cleanup"));
			m_pliPublishingIPs->RemoveHead();
		}
		else
			break;
	}
}

CEntry*	CKeyEntry::Copy(){
	return CEntry::Copy();
}

void CKeyEntry::WritePublishTrackingDataToFile(CDataIO* pData){

	// format: <AICH HashCount 2><{AICH Hash Indexed} HashCount> <Names_Count 4><{<Name string><PopularityIndex 4>} Names_Count>
	//		   <PublisherCount 4><{<IP 4><Time 4><AICH Idx 2>} PublisherCount>

	// Write AICH Hashes and map them to a new cleaned up index without unreferenced hashes
	uint16 nNewIdxPos = 0;
	CArray<uint16> aNewIndexes;
	for (int i = 0; i < m_aAICHHashs.GetCount(); i++)
	{
		if (m_anAICHHashPopularity[i] > 0)
		{
			aNewIndexes.Add(nNewIdxPos);
			nNewIdxPos++;
		}
		else
			aNewIndexes.Add(_UI16_MAX);
	}
	pData->WriteUInt16(nNewIdxPos);
	for (int i = 0; i < m_aAICHHashs.GetCount(); i++)
	{
		if (m_anAICHHashPopularity[i] > 0)
			pData->WriteArray(m_aAICHHashs[i].GetRawHashC(), CAICHHash::GetHashSize());
	}

	pData->WriteUInt32((uint32)m_listFileNames.GetCount());
	for (POSITION pos = m_listFileNames.GetHeadPosition(); pos != NULL;){
		const structFileNameEntry& rCur = m_listFileNames.GetNext(pos);
		pData->WriteString(rCur.m_fileName);
		pData->WriteUInt32(rCur.m_uPopularityIndex);
	}
	if (m_pliPublishingIPs != NULL){
		pData->WriteUInt32((uint32)m_pliPublishingIPs->GetCount());
		for (POSITION pos = m_pliPublishingIPs->GetHeadPosition(); pos != NULL;){
			const structPublishingIP& rCur = m_pliPublishingIPs->GetNext(pos);
			ASSERT( rCur.m_uIP != 0 );
			pData->WriteUInt32(rCur.m_uIP);
			pData->WriteUInt32((uint32)rCur.m_tLastPublish);
			uint16 nIdx = _UI16_MAX;
			if (rCur.m_byAICHHashIdx != _UI16_MAX)
			{
				nIdx = aNewIndexes[rCur.m_byAICHHashIdx];
				ASSERT( nIdx != _UI16_MAX );
			}
			pData->WriteUInt16(nIdx);
		}
	}
	else{
		ASSERT( false );
		pData->WriteUInt32(0);
	}
}

void CKeyEntry::ReadPublishTrackingDataFromFile(CDataIO* pData, bool bIncludesAICH){
	// format: <AICH HashCount 2><{AICH Hash Indexed} HashCount> <Names_Count 4><{<Name string><PopularityIndex 4>} Names_Count>
	//		   <PublisherCount 4><{<IP 4><Time 4><AICH Idx 2>} PublisherCount>	    
	ASSERT( m_aAICHHashs.IsEmpty() );
	ASSERT( m_anAICHHashPopularity.IsEmpty() );
	if (bIncludesAICH)
	{
		uint16 nAICHHashCount = pData->ReadUInt16();
		for (uint16 i = 0; i < nAICHHashCount; i++)
		{
			CAICHHash hash;
			pData->ReadArray(hash.GetRawHash(), CAICHHash::GetHashSize());
			m_aAICHHashs.Add(hash);
			m_anAICHHashPopularity.Add(0);
		}
	}


	ASSERT( m_listFileNames.IsEmpty() );
	uint32 nNameCount = pData->ReadUInt32();
	for (uint32 i = 0; i < nNameCount; i++){
		structFileNameEntry sToAdd;
		sToAdd.m_fileName = pData->ReadStringUTF8();
		sToAdd.m_uPopularityIndex = pData->ReadUInt32();
		m_listFileNames.AddTail(sToAdd);
	}

	ASSERT( m_pliPublishingIPs == NULL );
	m_pliPublishingIPs = new CList<structPublishingIP>();
	uint32 nIPCount = pData->ReadUInt32();
	uint32 nDbgLastTime = 0;
	for (uint32 i = 0; i < nIPCount; i++){
		structPublishingIP sToAdd;
		sToAdd.m_uIP = pData->ReadUInt32();
		ASSERT( sToAdd.m_uIP != 0 );
		sToAdd.m_tLastPublish = pData->ReadUInt32();
		ASSERT( nDbgLastTime <= (uint32)sToAdd.m_tLastPublish ); // shoudl always be sorted oldest first
		nDbgLastTime = sToAdd.m_tLastPublish;
		// read hash index and update popularity index
		if (bIncludesAICH)
		{
			sToAdd.m_byAICHHashIdx = pData->ReadUInt16();
			if (sToAdd.m_byAICHHashIdx != _UI16_MAX)
			{
				if (sToAdd.m_byAICHHashIdx >= m_aAICHHashs.GetCount())
				{
					// should never happen
					ASSERT( false );
					DebugLogError(_T("CKeyEntry::ReadPublishTrackingDataFromFile - Out of Index AICH Hash index value while loading keywords"));
					sToAdd.m_byAICHHashIdx = _UI16_MAX;
				}
				else
					m_anAICHHashPopularity[sToAdd.m_byAICHHashIdx]++;
			}
		}
		else
			sToAdd.m_byAICHHashIdx = _UI16_MAX;

		AdjustGlobalPublishTracking(sToAdd.m_uIP, true, _T(""));

		m_pliPublishingIPs->AddTail(sToAdd);
	}
	RecalcualteTrustValue();
#ifdef _DEBUG
	if (m_aAICHHashs.GetCount() == 1)
		DebugLog(_T("Loaded 1 AICH Hash (%s, publishers %u of %u) for file %s"), m_aAICHHashs[0].GetString(), m_anAICHHashPopularity[0], m_pliPublishingIPs->GetCount(), m_uSourceID.ToHexString());
	else if (m_aAICHHashs.GetCount() > 1)
	{
		DebugLogWarning(_T("Loaded multiple (%u) AICH Hashs for file %s, dumping..."), m_aAICHHashs.GetCount(), m_uSourceID.ToHexString());
		for (int i = 0; i < m_aAICHHashs.GetCount(); i++)
			DebugLog(_T("%s - %u out of %u publishers"), m_aAICHHashs[i].GetString(), m_anAICHHashPopularity[i], m_pliPublishingIPs->GetCount());
	}
	//if (GetTrustValue() < 1.0f)
		//DEBUG_ONLY( DebugLog(_T("Loaded %u different names, %u different publishIPs (trustvalue = %.2f) for file %s"), nNameCount, nIPCount, GetTrustValue(), m_uSourceID.ToHexString()) );
#endif
}

void CKeyEntry::DirtyDeletePublishData(){
	// instead of deleting our publishers properly in the destructor with decreasing the count in the global map 
	// we just remove them, and trust that the caller in the end also resets the global map, so the
	// kad shutdown is speed up a bit
	delete m_pliPublishingIPs;
	m_pliPublishingIPs = NULL;
}

void CKeyEntry::WriteTagListWithPublishInfo(CDataIO* pData){

	if (m_pliPublishingIPs == NULL || m_pliPublishingIPs->GetCount() == 0){
		ASSERT( false );
		WriteTagList(pData);
		return;
	}

	
	uint32 nAdditionalTags = 1;
	if (!m_aAICHHashs.IsEmpty())
		nAdditionalTags++;
	WriteTagListInc(pData, nAdditionalTags); // write the standard taglist but increase the tagcount by the count we wan to add

	// here we add a tag including how many publishers this entry has, the trustvalue and how many different names are known
	// this is supposed to get used in later versions as an indicator for the user how valid this result is (of course this tag
	// alone cannt be trusted 100%, because we could be a bad node, but its a part of the puzzle)
	uint32 uTrust = (uint16)(GetTrustValue() * 100);
	uint32 uPublishers = m_pliPublishingIPs->GetCount() % 256;
	uint32 uNames = m_listFileNames.GetCount() % 256;
	// 32 bit tag: <namecount uint8><publishers uint8><trustvalue*100 uint16>
	uint32 uTagValue = (uNames << 24) | (uPublishers << 16) | (uTrust << 0);
	CKadTagUInt tag(TAG_PUBLISHINFO, uTagValue);
	pData->WriteTag(&tag);

	// Last but not least the AICH Hash tag, containing all reported (hopefulley exactly 1) AICH hashes for this file together
	// with the count of publishers who reported it
	if (!m_aAICHHashs.IsEmpty())
	{
		CSafeMemFile fileAICHTag(100);
		uint8 byCount = 0;
		// get count of AICH tags with popularity > 0
		for (int i = 0; i < m_aAICHHashs.GetCount(); i++)
		{
			if (m_anAICHHashPopularity[i] > 0)
				byCount++;
			// bobs tags in kad are limited to 255 bytes, so no more than 12 AICH hashes can be written
			// that shouldn't be an issue however, as the normal AICH hash count is 1, if we have more than
			// 10 for some reason we can't use it most likely anyway
			if (1 + (CAICHHash::GetHashSize() * (byCount + 1)) + (1 * (byCount + 1)) > 250)
			{
				DebugLogWarning(_T("More than 12(!) AICH Hashs to send for search answer, have to truncate, entry: %s"), m_uSourceID.ToHexString());
				break;
			}
						
		}
		// write tag even on 0 count now
		fileAICHTag.WriteUInt8(byCount);
		uint8 nWritten = 0;
		uint8 j;
		for (j = 0; nWritten < byCount && j < m_aAICHHashs.GetCount(); j++)
		{
			if (m_anAICHHashPopularity[j] > 0)
			{
				fileAICHTag.WriteUInt8(m_anAICHHashPopularity[j]);
				m_aAICHHashs[j].Write(&fileAICHTag);
				nWritten++;
			}
		}
		ASSERT( nWritten == byCount && nWritten <= j );
		ASSERT(fileAICHTag.GetLength() <= 255 );
		uint8 nSize = (uint8)fileAICHTag.GetLength();
		BYTE* byBuffer = fileAICHTag.Detach();
		CKadTagBsob tag(TAG_KADAICHHASHRESULT, byBuffer, nSize);
		pData->WriteTag(&tag);
		free(byBuffer);
	}
}

uint16 CKeyEntry::AddRemoveAICHHash(const CAICHHash& hash, bool bAdd)
{
	ASSERT( m_aAICHHashs.GetCount() == m_anAICHHashPopularity.GetCount() );
	for (int i = 0; i < m_aAICHHashs.GetCount(); i++)
	{
		if (m_aAICHHashs[i] == hash)
		{
			if (bAdd)
			{
				m_anAICHHashPopularity[i] += 1;
				return (uint16)i;
			}
			else
			{
				if (m_anAICHHashPopularity[i] >= 1)
					m_anAICHHashPopularity[i] -= 1;
				else
					ASSERT( false );
				return (uint16)i;
			}
		}
	}
	if (bAdd)
	{
		m_aAICHHashs.Add(hash);
		m_anAICHHashPopularity.Add(1);
		return (uint16)m_aAICHHashs.GetCount() - 1;
	}
	else
	{
		ASSERT( false );
		return _UI16_MAX;
	}
}