/*
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
#include "./Indexed.h"
#include "./Kademlia.h"
#include "./Entry.h"
#include "./Prefs.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/MiscUtils.h"
#include "../io/BufferedFileIO.h"
#include "../io/IOException.h"
#include "../io/ByteIO.h"
#include "../../Preferences.h"
#include "../../Log.h"
#include "../utils/KadUDPKey.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;

void DebugSend(LPCTSTR pszMsg, uint32 uIP, uint16 uPort);

CString CIndexed::m_sKeyFileName;
CString CIndexed::m_sSourceFileName;
CString CIndexed::m_sLoadFileName;

CIndexed::CIndexed()
{
	m_mapKeyword.InitHashTable(1031);
	m_mapNotes.InitHashTable(1031);
	m_mapLoad.InitHashTable(1031);
	m_mapSources.InitHashTable(1031);
	m_sSourceFileName = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("src_index.dat");
	m_sKeyFileName = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("key_index.dat");
	m_sLoadFileName = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("load_index.dat");
	m_tLastClean = time(NULL) + (60*30);
	m_uTotalIndexSource = 0;
	m_uTotalIndexKeyword = 0;
	m_uTotalIndexNotes = 0;
	m_uTotalIndexLoad = 0;
	m_bAbortLoading = false;
	m_bDataLoaded = false;
	ReadFile();
}

void CIndexed::ReadFile(void)
{
	m_bAbortLoading = false;
	CLoadDataThread* pLoadDataThread = (CLoadDataThread*) AfxBeginThread(RUNTIME_CLASS(CLoadDataThread), THREAD_PRIORITY_BELOW_NORMAL,0, CREATE_SUSPENDED);
	pLoadDataThread->SetValues(this);
	pLoadDataThread->ResumeThread();
}

CIndexed::~CIndexed()
{ 
	if (!m_bDataLoaded){
		// the user clicked on disconnect/close just after he started kad (and probably just before posting in the forum that emule doesnt works :P )
		// while the loading thread is still busy. First tell the thread to abort its loading, afterwards wait for it to terminate
		// and then delete all loaded items without writing them to the files (as they are incomplete and unchanged)
		DebugLogWarning(_T("Kad stopping while still loading CIndexed data, waiting for abort"));
		m_bAbortLoading = true;
		CSingleLock sLock(&m_mutSync);
		sLock.Lock(); // wait
		ASSERT( m_bDataLoaded );

		// cleanup without storing
		POSITION pos1 = m_mapSources.GetStartPosition();
		while( pos1 != NULL )
		{
			CCKey key1;
			SrcHash* pCurrSrcHash;
			m_mapSources.GetNextAssoc( pos1, key1, pCurrSrcHash );
			CKadSourcePtrList* keyHashSrcMap = &pCurrSrcHash->ptrlistSource;
			POSITION pos2 = keyHashSrcMap->GetHeadPosition();
			while( pos2 != NULL )
			{
				Source* pCurrSource = keyHashSrcMap->GetNext(pos2);
				CKadEntryPtrList* srcEntryList = &pCurrSource->ptrlEntryList;
				for(POSITION pos3 = srcEntryList->GetHeadPosition(); pos3 != NULL; )
				{
					CEntry* pCurrName = srcEntryList->GetNext(pos3);
					delete pCurrName;
				}
				delete pCurrSource;
			}
			delete pCurrSrcHash;
		}

		pos1 = m_mapLoad.GetStartPosition();
		while (pos1 != NULL)
		{
			CCKey key1;
			Load* pLoad;
			m_mapLoad.GetNextAssoc(pos1, key1, pLoad);
			delete pLoad;
		}

		pos1 = m_mapKeyword.GetStartPosition();
		while( pos1 != NULL )
		{
			CCKey key1;
			KeyHash* pCurrKeyHash;
			m_mapKeyword.GetNextAssoc( pos1, key1, pCurrKeyHash );
			CSourceKeyMap* keySrcKeyMap = &pCurrKeyHash->mapSource;
			POSITION pos2 = keySrcKeyMap->GetStartPosition();
			while( pos2 != NULL )
			{
				Source* pCurrSource;
				CCKey key2;
				keySrcKeyMap->GetNextAssoc( pos2, key2, pCurrSource );
				CKadEntryPtrList* srcEntryList = &pCurrSource->ptrlEntryList;
				for(POSITION pos3 = srcEntryList->GetHeadPosition(); pos3 != NULL; )
				{
					CKeyEntry* pCurrName = (CKeyEntry*)srcEntryList->GetNext(pos3);
					ASSERT( pCurrName->IsKeyEntry() );
					pCurrName->DirtyDeletePublishData();
					delete pCurrName;
				}
				delete pCurrSource;
			}
			delete pCurrKeyHash;
		}
		CKeyEntry::ResetGlobalTrackingMap();
	}
	else {
		// standard cleanup with sotring
		try
		{
			uint32 uTotalSource = 0;
			uint32 uTotalKey = 0;
			uint32 uTotalLoad = 0;

			CBufferedFileIO fileLoad;
			if(fileLoad.Open(m_sLoadFileName, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyWrite))
			{
				setvbuf(fileLoad.m_pStream, NULL, _IOFBF, 32768);
				uint32 uVersion = 1;
				fileLoad.WriteUInt32(uVersion);
				fileLoad.WriteUInt32(time(NULL));
				fileLoad.WriteUInt32(m_mapLoad.GetCount());
				POSITION pos1 = m_mapLoad.GetStartPosition();
				while( pos1 != NULL )
				{
					Load* pLoad;
					CCKey key1;
					m_mapLoad.GetNextAssoc( pos1, key1, pLoad );
					fileLoad.WriteUInt128(pLoad->uKeyID);
					fileLoad.WriteUInt32(pLoad->uTime);
					uTotalLoad++;
					delete pLoad;
				}
				fileLoad.Close();
			}
			else
				DebugLogError(_T("Unable to store Kad file: %s"), m_sLoadFileName);

			CBufferedFileIO fileSource;
			if (fileSource.Open(m_sSourceFileName, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyWrite))
			{
				setvbuf(fileSource.m_pStream, NULL, _IOFBF, 32768);
				uint32 uVersion = 2;
				fileSource.WriteUInt32(uVersion);
				fileSource.WriteUInt32(time(NULL)+KADEMLIAREPUBLISHTIMES);
				fileSource.WriteUInt32(m_mapSources.GetCount());
				POSITION pos1 = m_mapSources.GetStartPosition();
				while( pos1 != NULL )
				{
					CCKey key1;
					SrcHash* pCurrSrcHash;
					m_mapSources.GetNextAssoc( pos1, key1, pCurrSrcHash );
					fileSource.WriteUInt128(pCurrSrcHash->uKeyID);
					CKadSourcePtrList* keyHashSrcMap = &pCurrSrcHash->ptrlistSource;
					fileSource.WriteUInt32(keyHashSrcMap->GetCount());
					POSITION pos2 = keyHashSrcMap->GetHeadPosition();
					while( pos2 != NULL )
					{
						Source* pCurrSource = keyHashSrcMap->GetNext(pos2);
						fileSource.WriteUInt128(pCurrSource->uSourceID);
						CKadEntryPtrList* srcEntryList = &pCurrSource->ptrlEntryList;
						fileSource.WriteUInt32(srcEntryList->GetCount());
						for(POSITION pos3 = srcEntryList->GetHeadPosition(); pos3 != NULL; )
						{
							CEntry* pCurrName = srcEntryList->GetNext(pos3);
							fileSource.WriteUInt32(pCurrName->m_tLifetime);
							pCurrName->WriteTagList(&fileSource);
							delete pCurrName;
							uTotalSource++;
						}
						delete pCurrSource;
					}
					delete pCurrSrcHash;
				}
				fileSource.Close();
			}
			else
				DebugLogError(_T("Unable to store Kad file: %s"), m_sSourceFileName);

			CBufferedFileIO fileKey;
			if (fileKey.Open(m_sKeyFileName, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyWrite))
			{
				setvbuf(fileKey.m_pStream, NULL, _IOFBF, 32768);
				uint32 uVersion = 4;
				fileKey.WriteUInt32(uVersion);
				fileKey.WriteUInt32(time(NULL)+KADEMLIAREPUBLISHTIMEK);
				fileKey.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());
				fileKey.WriteUInt32(m_mapKeyword.GetCount());
				POSITION pos1 = m_mapKeyword.GetStartPosition();
				while( pos1 != NULL )
				{
					CCKey key1;
					KeyHash* pCurrKeyHash;
					m_mapKeyword.GetNextAssoc( pos1, key1, pCurrKeyHash );
					fileKey.WriteUInt128(pCurrKeyHash->uKeyID);
					CSourceKeyMap* keySrcKeyMap = &pCurrKeyHash->mapSource;
					fileKey.WriteUInt32(keySrcKeyMap->GetCount());
					POSITION pos2 = keySrcKeyMap->GetStartPosition();
					while( pos2 != NULL )
					{
						Source* pCurrSource;
						CCKey key2;
						keySrcKeyMap->GetNextAssoc( pos2, key2, pCurrSource );
						fileKey.WriteUInt128(pCurrSource->uSourceID);
						CKadEntryPtrList* srcEntryList = &pCurrSource->ptrlEntryList;
						fileKey.WriteUInt32(srcEntryList->GetCount());
						for(POSITION pos3 = srcEntryList->GetHeadPosition(); pos3 != NULL; )
						{
							CKeyEntry* pCurrName = (CKeyEntry*)srcEntryList->GetNext(pos3);
							ASSERT( pCurrName->IsKeyEntry() );
							fileKey.WriteUInt32(pCurrName->m_tLifetime);
							pCurrName->WritePublishTrackingDataToFile(&fileKey);
							pCurrName->WriteTagList(&fileKey);
							pCurrName->DirtyDeletePublishData();
							delete pCurrName;
							uTotalKey++;
						}
						delete pCurrSource;
					}
					delete pCurrKeyHash;
				}
				CKeyEntry::ResetGlobalTrackingMap();
				fileKey.Close();
			}
			else
				DebugLogError(_T("Unable to store Kad file: %s"), m_sKeyFileName);

			AddDebugLogLine( false, _T("Wrote %u source, %u keyword, and %u load entries"), uTotalSource, uTotalKey, uTotalLoad);


		}
		catch ( CIOException *ioe )
		{
			AddDebugLogLine( false, _T("Exception in CIndexed::~CIndexed (IO error(%i))"), ioe->m_iCause);
			ioe->Delete();
		}
		catch (...)
		{
			AddDebugLogLine(false, _T("Exception in CIndexed::~CIndexed"));
		}
	}

	// leftover cleanup (same for both variants)
	POSITION pos1 = m_mapNotes.GetStartPosition();
	while( pos1 != NULL )
	{
		CCKey key1;
		SrcHash* pCurrNoteHash;
		m_mapNotes.GetNextAssoc( pos1, key1, pCurrNoteHash );
		CKadSourcePtrList* keyHashNoteMap = &pCurrNoteHash->ptrlistSource;
		POSITION pos2 = keyHashNoteMap->GetHeadPosition();
		while( pos2 != NULL )
		{
			Source* pCurrNote = keyHashNoteMap->GetNext(pos2);
			CKadEntryPtrList* noteEntryList = &pCurrNote->ptrlEntryList;
			for(POSITION pos3 = noteEntryList->GetHeadPosition(); pos3 != NULL; )
			{
				delete noteEntryList->GetNext(pos3);
			}
			delete pCurrNote;
		}
		delete pCurrNoteHash;
	}
}

void CIndexed::Clean(void)
{

	try
	{
		if( m_tLastClean > time(NULL) )
			return;

		uint32 uRemovedKey = 0;
		uint32 uRemovedSource = 0;
		uint32 uTotalSource = 0;
		uint32 uTotalKey = 0;
		time_t tNow = time(NULL);

		{
			POSITION pos1 = m_mapKeyword.GetStartPosition();
			while( pos1 != NULL )
			{
				CCKey key1;
				KeyHash* pCurrKeyHash;
				m_mapKeyword.GetNextAssoc( pos1, key1, pCurrKeyHash );
				POSITION pos2 = pCurrKeyHash->mapSource.GetStartPosition();
				while( pos2 != NULL )
				{
					CCKey key2;
					Source* pCurrSource;
					pCurrKeyHash->mapSource.GetNextAssoc( pos2, key2, pCurrSource );
					for(POSITION pos3 = pCurrSource->ptrlEntryList.GetHeadPosition(); pos3 != NULL; )
					{
						POSITION pos4 = pos3;
						CKeyEntry* pCurrName = (CKeyEntry*)pCurrSource->ptrlEntryList.GetNext(pos3);
						ASSERT( pCurrName->IsKeyEntry() );
						uTotalKey++;
						if( !pCurrName->m_bSource && pCurrName->m_tLifetime < tNow)
						{
							uRemovedKey++;
							pCurrSource->ptrlEntryList.RemoveAt(pos4);
							delete pCurrName;
						}
						else if (pCurrName->m_bSource)
							ASSERT( false );
						else
							pCurrName->CleanUpTrackedPublishers(); // intern cleanup
					}
					if( pCurrSource->ptrlEntryList.IsEmpty())
					{
						pCurrKeyHash->mapSource.RemoveKey(key2);
						delete pCurrSource;
					}
				}
				if( pCurrKeyHash->mapSource.IsEmpty())
				{
					m_mapKeyword.RemoveKey(key1);
					delete pCurrKeyHash;
				}
			}
		}
		{
			POSITION pos1 = m_mapSources.GetStartPosition();
			while( pos1 != NULL )
			{
				CCKey key1;
				SrcHash* pCurrSrcHash;
				m_mapSources.GetNextAssoc( pos1, key1, pCurrSrcHash );
				for(POSITION pos2 = pCurrSrcHash->ptrlistSource.GetHeadPosition(); pos2 != NULL; )
				{
					POSITION pos3 = pos2;
					Source* pCurrSource = pCurrSrcHash->ptrlistSource.GetNext(pos2);
					for(POSITION pos4 = pCurrSource->ptrlEntryList.GetHeadPosition(); pos4 != NULL; )
					{
						POSITION pos5 = pos4;
						CEntry* pCurrName = pCurrSource->ptrlEntryList.GetNext(pos4);
						uTotalSource++;
						if( pCurrName->m_tLifetime < tNow)
						{
							uRemovedSource++;
							pCurrSource->ptrlEntryList.RemoveAt(pos5);
							delete pCurrName;
						}
					}
					if( pCurrSource->ptrlEntryList.IsEmpty())
					{
						pCurrSrcHash->ptrlistSource.RemoveAt(pos3);
						delete pCurrSource;
					}
				}
				if( pCurrSrcHash->ptrlistSource.IsEmpty())
				{
					m_mapSources.RemoveKey(key1);
					delete pCurrSrcHash;
				}
			}
		}

		m_uTotalIndexSource = uTotalSource - uRemovedSource;;
		m_uTotalIndexKeyword = uTotalKey - uRemovedKey;;
		AddDebugLogLine( false, _T("Removed %u keyword out of %u and %u source out of %u"), uRemovedKey, uTotalKey, uRemovedSource, uTotalSource);
		m_tLastClean = time(NULL) + MIN2S(30);
	}
	catch(...)
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::clean"));
		ASSERT(0);
	}
}

bool CIndexed::AddKeyword(const CUInt128& uKeyID, const CUInt128& uSourceID, Kademlia::CKeyEntry* pEntry, uint8& uLoad, bool bIgnoreThreadLock)
{
	// do not access any data while the loading thread is busy;
	// bIgnoreThreadLock should be only used by CLoadDataThread itself
	if (!bIgnoreThreadLock && !m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return false;
	}

	if( !pEntry )
		return false;

	if (!pEntry->IsKeyEntry()){
		ASSERT( false );
		return false;
	}

	if( m_uTotalIndexKeyword > KADEMLIAMAXENTRIES )
	{
		uLoad = 100;
		return false;
	}

	if( pEntry->m_uSize == 0 || pEntry->GetCommonFileName().IsEmpty() || pEntry->GetTagCount() == 0 || pEntry->m_tLifetime < time(NULL))
		return false;

	KeyHash* pCurrKeyHash;
	if(!m_mapKeyword.Lookup(CCKey(uKeyID.GetData()), pCurrKeyHash))
	{
		Source* pCurrSource = new Source;
		pCurrSource->uSourceID.SetValue(uSourceID);
		pEntry->MergeIPsAndFilenames(NULL); //IpTracking init
		pCurrSource->ptrlEntryList.AddHead(pEntry);
		pCurrKeyHash = new KeyHash;
		pCurrKeyHash->uKeyID.SetValue(uKeyID);
		pCurrKeyHash->mapSource.SetAt(CCKey(pCurrSource->uSourceID.GetData()), pCurrSource);
		m_mapKeyword.SetAt(CCKey(pCurrKeyHash->uKeyID.GetData()), pCurrKeyHash);
		uLoad = 1;
		m_uTotalIndexKeyword++;
		return true;
	}
	else
	{
		uint32 uIndexTotal = pCurrKeyHash->mapSource.GetCount();
		if ( uIndexTotal > KADEMLIAMAXINDEX )
		{
			uLoad = 100;
			//Too many entries for this Keyword..
			return false;
		}
		Source* pCurrSource;
		if(pCurrKeyHash->mapSource.Lookup(CCKey(uSourceID.GetData()), pCurrSource))
		{
			if (pCurrSource->ptrlEntryList.GetCount() > 0)
			{
				if( uIndexTotal > KADEMLIAMAXINDEX - 5000 )
				{
					uLoad = 100;
					//We are in a hot node.. If we continued to update all the publishes
					//while this index is full, popular files will be the only thing you index.
					return false;
				}
				// also check for size match
				CKeyEntry* pOldEntry = NULL;
				for (POSITION pos = pCurrSource->ptrlEntryList.GetHeadPosition(); pos != NULL; pCurrSource->ptrlEntryList.GetNext(pos)){
					CKeyEntry* pCurEntry = (CKeyEntry*)pCurrSource->ptrlEntryList.GetAt(pos);
					ASSERT( pCurEntry->IsKeyEntry() );
					if (pCurEntry->m_uSize == pEntry->m_uSize){
						pOldEntry = pCurEntry;
						pCurrSource->ptrlEntryList.RemoveAt(pos);
						break;
					}
				}
				pEntry->MergeIPsAndFilenames(pOldEntry); // pOldEntry can be NULL, thats ok and we still need todo this call in this case
				if (pOldEntry == NULL){
					m_uTotalIndexKeyword++;
					DebugLogWarning(_T("Kad: Indexing: Keywords: Multiple sizes published for file %s"), pEntry->m_uSourceID.ToHexString());
				}
				DEBUG_ONLY( AddDebugLogLine(DLP_VERYLOW, false, _T("Indexed file %s"), pEntry->m_uSourceID.ToHexString()) );
				delete pOldEntry;
				pOldEntry = NULL;
			}
			else{
				m_uTotalIndexKeyword++;
				pEntry->MergeIPsAndFilenames(NULL); //IpTracking init
			}
			uLoad = (uint8)((uIndexTotal*100)/KADEMLIAMAXINDEX);
			pCurrSource->ptrlEntryList.AddHead(pEntry);
			return true;
		}
		else
		{
			pCurrSource = new Source;
			pCurrSource->uSourceID.SetValue(uSourceID);
			pEntry->MergeIPsAndFilenames(NULL); //IpTracking init
			pCurrSource->ptrlEntryList.AddHead(pEntry);
			pCurrKeyHash->mapSource.SetAt(CCKey(pCurrSource->uSourceID.GetData()), pCurrSource);
			m_uTotalIndexKeyword++;
			uLoad = (uint8)((uIndexTotal*100)/KADEMLIAMAXINDEX);
			return true;
		}
	}
}

bool CIndexed::AddSources(const CUInt128& uKeyID, const CUInt128& uSourceID, Kademlia::CEntry* pEntry, uint8& uLoad, bool bIgnoreThreadLock)
{
	// do not access any data while the loading thread is busy;
	// bIgnoreThreadLock should be only used by CLoadDataThread itself
	if (!bIgnoreThreadLock && !m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return false;
	}

	if( !pEntry )
		return false;
	if( pEntry->m_uIP == 0 || pEntry->m_uTCPPort == 0 || pEntry->m_uUDPPort == 0 || pEntry->GetTagCount() == 0 || pEntry->m_tLifetime < time(NULL))
		return false;

	SrcHash* pCurrSrcHash;
	if(!m_mapSources.Lookup(CCKey(uKeyID.GetData()), pCurrSrcHash))
	{
		Source* pCurrSource = new Source;
		pCurrSource->uSourceID.SetValue(uSourceID);
		pCurrSource->ptrlEntryList.AddHead(pEntry);
		pCurrSrcHash = new SrcHash;
		pCurrSrcHash->uKeyID.SetValue(uKeyID);
		pCurrSrcHash->ptrlistSource.AddHead(pCurrSource);
		m_mapSources.SetAt(CCKey(pCurrSrcHash->uKeyID.GetData()), pCurrSrcHash);
		m_uTotalIndexSource++;
		uLoad = 1;
		return true;
	}
	else
	{
		uint32 uSize = pCurrSrcHash->ptrlistSource.GetSize();
		for(POSITION pos1 = pCurrSrcHash->ptrlistSource.GetHeadPosition(); pos1 != NULL; )
		{
			Source* pCurrSource = pCurrSrcHash->ptrlistSource.GetNext(pos1);
			if( pCurrSource->ptrlEntryList.GetSize() )
			{
				CEntry* pCurrEntry = pCurrSource->ptrlEntryList.GetHead();
				ASSERT(pCurrEntry!=NULL);
				if( pCurrEntry->m_uIP == pEntry->m_uIP && ( pCurrEntry->m_uTCPPort == pEntry->m_uTCPPort || pCurrEntry->m_uUDPPort == pEntry->m_uUDPPort ))
				{
					delete pCurrSource->ptrlEntryList.RemoveHead();
					pCurrSource->ptrlEntryList.AddHead(pEntry);
					uLoad = (uint8)((uSize*100)/KADEMLIAMAXSOUCEPERFILE);
					return true;
				}
			}
			else
			{
				//This should never happen!
				pCurrSource->ptrlEntryList.AddHead(pEntry);
				ASSERT(0);
				uLoad = (uint8)((uSize*100)/KADEMLIAMAXSOUCEPERFILE);
				m_uTotalIndexSource++;
				return true;
			}
		}
		if( uSize > KADEMLIAMAXSOUCEPERFILE )
		{
			Source* pCurrSource = pCurrSrcHash->ptrlistSource.RemoveTail();
			delete pCurrSource->ptrlEntryList.RemoveTail();
			pCurrSource->uSourceID.SetValue(uSourceID);
			pCurrSource->ptrlEntryList.AddHead(pEntry);
			pCurrSrcHash->ptrlistSource.AddHead(pCurrSource);
			uLoad = 100;
			return true;
		}
		else
		{
			Source* pCurrSource = new Source;
			pCurrSource->uSourceID.SetValue(uSourceID);
			pCurrSource->ptrlEntryList.AddHead(pEntry);
			pCurrSrcHash->ptrlistSource.AddHead(pCurrSource);
			m_uTotalIndexSource++;
			uLoad = (uint8)((uSize*100)/KADEMLIAMAXSOUCEPERFILE);
			return true;
		}
	}
}

bool CIndexed::AddNotes(const CUInt128& uKeyID, const CUInt128& uSourceID, Kademlia::CEntry* pEntry, uint8& uLoad, bool bIgnoreThreadLock)
{
	// do not access any data while the loading thread is busy;
	// bIgnoreThreadLock should be only used by CLoadDataThread itself
	if (!bIgnoreThreadLock && !m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return false;
	}

	if( !pEntry )
		return false;
	if( pEntry->m_uIP == 0 || pEntry->GetTagCount() == 0 )
		return false;

	SrcHash* pCurrNoteHash;
	if(!m_mapNotes.Lookup(CCKey(uKeyID.GetData()), pCurrNoteHash))
	{
		Source* pCurrNote = new Source;
		pCurrNote->uSourceID.SetValue(uSourceID);
		pCurrNote->ptrlEntryList.AddHead(pEntry);
		SrcHash* pCurrNoteHash = new SrcHash;
		pCurrNoteHash->uKeyID.SetValue(uKeyID);
		pCurrNoteHash->ptrlistSource.AddHead(pCurrNote);
		m_mapNotes.SetAt(CCKey(pCurrNoteHash->uKeyID.GetData()), pCurrNoteHash);
		uLoad = 1;
		m_uTotalIndexNotes++;
		return true;
	}
	else
	{
		uint32 uSize = pCurrNoteHash->ptrlistSource.GetSize();
		for(POSITION pos1 = pCurrNoteHash->ptrlistSource.GetHeadPosition(); pos1 != NULL; )
		{
			Source* pCurrNote = pCurrNoteHash->ptrlistSource.GetNext(pos1);
			if( pCurrNote->ptrlEntryList.GetSize() )
			{
				CEntry* pCurrEntry = pCurrNote->ptrlEntryList.GetHead();
				if(pCurrEntry->m_uIP == pEntry->m_uIP || pCurrEntry->m_uSourceID == pEntry->m_uSourceID)
				{
					delete pCurrNote->ptrlEntryList.RemoveHead();
					pCurrNote->ptrlEntryList.AddHead(pEntry);
					uLoad = (uint8)((uSize*100)/KADEMLIAMAXNOTESPERFILE);
					return true;
				}
			}
			else
			{
				//This should never happen!
				pCurrNote->ptrlEntryList.AddHead(pEntry);
				ASSERT(0);
				uLoad = (uint8)((uSize*100)/KADEMLIAMAXNOTESPERFILE);
				m_uTotalIndexNotes++;
				return true;
			}
		}
		if( uSize > KADEMLIAMAXNOTESPERFILE )
		{
			Source* pCurrNote = pCurrNoteHash->ptrlistSource.RemoveTail();
			delete pCurrNote->ptrlEntryList.RemoveTail();
			pCurrNote->uSourceID.SetValue(uSourceID);
			pCurrNote->ptrlEntryList.AddHead(pEntry);
			pCurrNoteHash->ptrlistSource.AddHead(pCurrNote);
			uLoad = 100;
			return true;
		}
		else
		{
			Source* pCurrNote = new Source;
			pCurrNote->uSourceID.SetValue(uSourceID);
			pCurrNote->ptrlEntryList.AddHead(pEntry);
			pCurrNoteHash->ptrlistSource.AddHead(pCurrNote);
			uLoad = (uint8)((uSize*100)/KADEMLIAMAXNOTESPERFILE);
			m_uTotalIndexNotes++;
			return true;
		}
	}
}

bool CIndexed::AddLoad(const CUInt128& uKeyID, uint32 uTime, bool bIgnoreThreadLock)
{
	// do not access any data while the loading thread is busy;
	// bIgnoreThreadLock should be only used by CLoadDataThread itself
	if (!bIgnoreThreadLock && !m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return false;
	}

	//This is needed for when you restart the client.
	if((uint32)time(NULL)>uTime)
		return false;

	Load* pLoad;
	if(m_mapLoad.Lookup(CCKey(uKeyID.GetData()), pLoad))
		return false;

	pLoad = new Load();
	pLoad->uKeyID.SetValue(uKeyID);
	pLoad->uTime = uTime;
	m_mapLoad.SetAt(CCKey(pLoad->uKeyID.GetData()), pLoad);
	m_uTotalIndexLoad++;
	return true;
}

void CIndexed::SendValidKeywordResult(const CUInt128& uKeyID, const SSearchTerm* pSearchTerms, uint32 uIP, uint16 uPort, bool bOldClient, uint16 uStartPosition, CKadUDPKey senderUDPKey)
{
	// do not access any data while the loading thread is busy;
	if (!m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return;
	}

	KeyHash* pCurrKeyHash;
	if(m_mapKeyword.Lookup(CCKey(uKeyID.GetData()), pCurrKeyHash))
	{
		byte byPacket[1024*5];
		byte bySmallBuffer[2048];

		CByteIO byIO(byPacket,sizeof(byPacket));
		byIO.WriteByte(OP_KADEMLIAHEADER);
		byIO.WriteByte(KADEMLIA2_SEARCH_RES);
		byIO.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());
		byIO.WriteUInt128(uKeyID);
		
		byte* pbyCountPos = byPacket + byIO.GetUsed();
		ASSERT( byPacket+18+16 == pbyCountPos || byPacket+18 == pbyCountPos);
		byIO.WriteUInt16(0);

		const uint16 uMaxResults = 300;
		int iCount = 0-uStartPosition;
		int iUnsentCount = 0;
		CByteIO byIOTmp(bySmallBuffer, sizeof(bySmallBuffer));
		// we do 2 loops: In the first one we ignore all results which have a trustvalue below 1
		// in the second one we then also consider those. That way we make sure our 300 max results are not full
		// of spam entries. We could also sort by trustvalue, but we would risk to only send popular files this way
		// on very hot keywords
		bool bOnlyTrusted = true;
		uint32 dbgResultsTrusted = 0;
		uint32 dbgResultsUntrusted = 0;
		do{
			POSITION pos1 = pCurrKeyHash->mapSource.GetStartPosition();
			while( pos1 != NULL )
			{
				CCKey key1;
				Source* pCurrSource;
				pCurrKeyHash->mapSource.GetNextAssoc( pos1, key1, pCurrSource );
				for(POSITION pos2 = pCurrSource->ptrlEntryList.GetHeadPosition(); pos2 != NULL; )
				{
					CKeyEntry* pCurrName = (CKeyEntry*)pCurrSource->ptrlEntryList.GetNext(pos2);
					ASSERT( pCurrName->IsKeyEntry() );
					if ( (bOnlyTrusted ^ (pCurrName->GetTrustValue() < 1.0f)) && (!pSearchTerms || pCurrName->StartSearchTermsMatch(pSearchTerms)) )
					{
						if( iCount < 0 )
							iCount++;
						else if( (uint16)iCount < uMaxResults )
						{
							if((!bOldClient || pCurrName->m_uSize <= OLD_MAX_EMULE_FILE_SIZE))
							{
								iCount++;
								if (bOnlyTrusted)
									dbgResultsTrusted++;
								else
									dbgResultsUntrusted++;
								byIOTmp.WriteUInt128(pCurrName->m_uSourceID);
								pCurrName->WriteTagListWithPublishInfo(&byIOTmp);
								
								if( byIO.GetUsed() + byIOTmp.GetUsed() > UDP_KAD_MAXFRAGMENT && iUnsentCount > 0)
								{
									uint32 uLen = sizeof(byPacket)-byIO.GetAvailable();
									PokeUInt16(pbyCountPos, (uint16)iUnsentCount);
									CKademlia::GetUDPListener()->SendPacket(byPacket, uLen, uIP, uPort, senderUDPKey, NULL);
									byIO.Reset();
									byIO.WriteByte(OP_KADEMLIAHEADER);
									if (thePrefs.GetDebugClientKadUDPLevel() > 0)
										DebugSend("KADEMLIA2_SEARCH_RES", uIP, uPort);
									byIO.WriteByte(KADEMLIA2_SEARCH_RES);
									byIO.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());
									byIO.WriteUInt128(uKeyID);
									byIO.WriteUInt16(0);
									DEBUG_ONLY(DebugLog(_T("Sent %u keyword search results in one packet to avoid fragmentation"), iUnsentCount)); 
									iUnsentCount = 0;
								}
								ASSERT( byIO.GetUsed() + byIOTmp.GetUsed() <= UDP_KAD_MAXFRAGMENT );
								byIO.WriteArray(bySmallBuffer, byIOTmp.GetUsed());
								byIOTmp.Reset();
								iUnsentCount++;
							}
						}
						else
						{
							pos1 = NULL;
							break;
						}
					}
				}
			}
			if (bOnlyTrusted && iCount < (int)uMaxResults)
				bOnlyTrusted = false;
			else
				break;
		} while (!bOnlyTrusted);

		// LOGTODO: Remove Log
		//DebugLog(_T("Kad Keyword search Result Request: Send %u trusted and %u untrusted results"), dbgResultsTrusted, dbgResultsUntrusted);

		if(iUnsentCount > 0)
		{
			uint32 uLen = sizeof(byPacket)-byIO.GetAvailable();
			PokeUInt16(pbyCountPos, (uint16)iUnsentCount);
			if(thePrefs.GetDebugClientKadUDPLevel() > 0)
			{
				DebugSend("KADEMLIA2_SEARCH_RES", uIP, uPort);
			}
			CKademlia::GetUDPListener()->SendPacket(byPacket, uLen, uIP, uPort, senderUDPKey, NULL);
			DEBUG_ONLY(DebugLog(_T("Sent %u keyword search results in last packet to avoid fragmentation"), iUnsentCount));
		}
		else if (iCount > 0)
			ASSERT( false );
	}
	Clean();
}

void CIndexed::SendValidSourceResult(const CUInt128& uKeyID, uint32 uIP, uint16 uPort, uint16 uStartPosition, uint64 uFileSize, CKadUDPKey senderUDPKey)
{
	// do not access any data while the loading thread is busy;
	if (!m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return;
	}

	SrcHash* pCurrSrcHash;
	if(m_mapSources.Lookup(CCKey(uKeyID.GetData()), pCurrSrcHash))
	{
		byte byPacket[1024*5];
		byte bySmallBuffer[2048];
		CByteIO byIO(byPacket,sizeof(byPacket));
		byIO.WriteByte(OP_KADEMLIAHEADER);
		byIO.WriteByte(KADEMLIA2_SEARCH_RES);
		byIO.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());

		byIO.WriteUInt128(uKeyID);
		byte* pbyCountPos = byPacket + byIO.GetUsed();
		ASSERT( byPacket+18+16 == pbyCountPos || byPacket+18 == pbyCountPos);
		byIO.WriteUInt16(0);
		
		int iUnsentCount = 0;
		CByteIO byIOTmp(bySmallBuffer, sizeof(bySmallBuffer));

		uint16 uMaxResults = 300;
		int iCount = 0-uStartPosition;
		for(POSITION pos1 = pCurrSrcHash->ptrlistSource.GetHeadPosition(); pos1 != NULL; )
		{
			Source* pCurrSource = pCurrSrcHash->ptrlistSource.GetNext(pos1);
			if( pCurrSource->ptrlEntryList.GetSize() )
			{
				CEntry* pCurrName = pCurrSource->ptrlEntryList.GetHead();
				if( iCount < 0 )
					iCount++;
				else if( (uint16)iCount < uMaxResults )
				{
					if( !uFileSize || !pCurrName->m_uSize || pCurrName->m_uSize == uFileSize )
					{
						byIOTmp.WriteUInt128(pCurrName->m_uSourceID);
						pCurrName->WriteTagList(&byIOTmp);
						iCount++;
						if( byIO.GetUsed() + byIOTmp.GetUsed() > UDP_KAD_MAXFRAGMENT && iUnsentCount > 0)
						{
							uint32 uLen = sizeof(byPacket)-byIO.GetAvailable();
							PokeUInt16(pbyCountPos, (uint16)iUnsentCount);
							CKademlia::GetUDPListener()->SendPacket(byPacket, uLen, uIP, uPort, senderUDPKey, NULL);
							byIO.Reset();
							byIO.WriteByte(OP_KADEMLIAHEADER);
							if (thePrefs.GetDebugClientKadUDPLevel() > 0)
								DebugSend("KADEMLIA2_SEARCH_RES", uIP, uPort);
							byIO.WriteByte(KADEMLIA2_SEARCH_RES);
							byIO.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());
							byIO.WriteUInt128(uKeyID);
							byIO.WriteUInt16(0);
							//DEBUG_ONLY(DebugLog(_T("Sent %u source search results in one packet to avoid fragmentation"), iUnsentCount)); 
							iUnsentCount = 0;
						}
						ASSERT( byIO.GetUsed() + byIOTmp.GetUsed() <= UDP_KAD_MAXFRAGMENT );
						byIO.WriteArray(bySmallBuffer, byIOTmp.GetUsed());
						byIOTmp.Reset();
						iUnsentCount++;
					}
				}
				else
				{
					break;
				}
			}
		}

		if(iUnsentCount > 0)
		{
			uint32 uLen = sizeof(byPacket)-byIO.GetAvailable();
			PokeUInt16(pbyCountPos, (uint16)iUnsentCount);
			if(thePrefs.GetDebugClientKadUDPLevel() > 0)
			{
				DebugSend("KADEMLIA2_SEARCH_RES", uIP, uPort);
			}
			CKademlia::GetUDPListener()->SendPacket(byPacket, uLen, uIP, uPort, senderUDPKey, NULL);
			//DEBUG_ONLY(DebugLog(_T("Sent %u source search results in last packet to avoid fragmentation"), iUnsentCount));
		}
		else if (iCount > 0)
			ASSERT( false );
	}
	Clean();
}

void CIndexed::SendValidNoteResult(const CUInt128& uKeyID, uint32 uIP, uint16 uPort, uint64 uFileSize, CKadUDPKey senderUDPKey)
{
	// do not access any data while the loading thread is busy;
	if (!m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return;
	}

	try
	{
		SrcHash* pCurrNoteHash;
		if(m_mapNotes.Lookup(CCKey(uKeyID.GetData()), pCurrNoteHash))
		{
			byte byPacket[1024*5];
			byte bySmallBuffer[2048];
			CByteIO byIO(byPacket,sizeof(byPacket));
			byIO.WriteByte(OP_KADEMLIAHEADER);
			byIO.WriteByte(KADEMLIA2_SEARCH_RES);
			byIO.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());
			byIO.WriteUInt128(uKeyID);

			byte* pbyCountPos = byPacket + byIO.GetUsed();
			ASSERT( byPacket+18+16 == pbyCountPos || byPacket+18 == pbyCountPos);
			byIO.WriteUInt16(0);

			int iUnsentCount = 0;
			CByteIO byIOTmp(bySmallBuffer, sizeof(bySmallBuffer));
			uint16 uMaxResults = 150;
			uint16 uCount = 0;
			for(POSITION pos1 = pCurrNoteHash->ptrlistSource.GetHeadPosition(); pos1 != NULL; )
			{
				Source* pCurrNote = pCurrNoteHash->ptrlistSource.GetNext(pos1);
				if( pCurrNote->ptrlEntryList.GetSize() )
				{
					CEntry* pCurrName = pCurrNote->ptrlEntryList.GetHead();
					if( uCount < uMaxResults )
					{
						if( !uFileSize || !pCurrName->m_uSize || uFileSize == pCurrName->m_uSize )
						{
							byIOTmp.WriteUInt128(pCurrName->m_uSourceID);
							pCurrName->WriteTagList(&byIOTmp);
							uCount++;
							if( byIO.GetUsed() + byIOTmp.GetUsed() > UDP_KAD_MAXFRAGMENT && iUnsentCount > 0)
							{
								uint32 uLen = sizeof(byPacket)-byIO.GetAvailable();
								PokeUInt16(pbyCountPos, (uint16)iUnsentCount);
								CKademlia::GetUDPListener()->SendPacket(byPacket, uLen, uIP, uPort, senderUDPKey, NULL);
								byIO.Reset();
								byIO.WriteByte(OP_KADEMLIAHEADER);
								if (thePrefs.GetDebugClientKadUDPLevel() > 0)
									DebugSend("KADEMLIA2_SEARCH_RES", uIP, uPort);
								byIO.WriteByte(KADEMLIA2_SEARCH_RES);
								byIO.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());
								byIO.WriteUInt128(uKeyID);
								byIO.WriteUInt16(0);
								DEBUG_ONLY(DebugLog(_T("Sent %u keyword search results in one packet to avoid fragmentation"), iUnsentCount)); 
								iUnsentCount = 0;
							}
							ASSERT( byIO.GetUsed() + byIOTmp.GetUsed() <= UDP_KAD_MAXFRAGMENT );
							byIO.WriteArray(bySmallBuffer, byIOTmp.GetUsed());
							byIOTmp.Reset();
							iUnsentCount++;
						}
					}
					else
					{
						break;
					}
				}
			}
			if(iUnsentCount > 0)
			{
				uint32 uLen = sizeof(byPacket)-byIO.GetAvailable();
				PokeUInt16(pbyCountPos, (uint16)iUnsentCount);
				if(thePrefs.GetDebugClientKadUDPLevel() > 0)
				{
					DebugSend("KADEMLIA2_SEARCH_RES", uIP, uPort);
				}
				CKademlia::GetUDPListener()->SendPacket(byPacket, uLen, uIP, uPort, senderUDPKey, NULL);
				DEBUG_ONLY(DebugLog(_T("Sent %u note search results in last packet to avoid fragmentation"), iUnsentCount));
			}
			else if (uCount > 0)
				ASSERT( false );
		}
	}
	catch(...)
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::SendValidNoteResult"));
	}
}

bool CIndexed::SendStoreRequest(const CUInt128& uKeyID)
{
	// do not access any data while the loading thread is busy;
	if (!m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return true; // don't report overloaded with a false
	}

	Load* pLoad;
	if(m_mapLoad.Lookup(CCKey(uKeyID.GetData()), pLoad))
	{
		if(pLoad->uTime < (uint32)time(NULL))
		{
			m_mapLoad.RemoveKey(CCKey(uKeyID.GetData()));
			m_uTotalIndexLoad--;
			delete pLoad;
			return true;
		}
		return false;
	}
	return true;
}

uint32 CIndexed::GetFileKeyCount()
{
	// do not access any data while the loading thread is busy;
	if (!m_bDataLoaded) {
		DEBUG_ONLY( DebugLogWarning(_T("CIndexed Memberfunction call failed because the dataloading still in progress")) );
		return 0;
	}

	return m_mapKeyword.GetCount();
}

SSearchTerm::SSearchTerm()
{
	m_type = AND;
	m_pTag = NULL;
	m_pLeft = NULL;
	m_pRight = NULL;
}

SSearchTerm::~SSearchTerm()
{
	if (m_type == String)
		delete m_pastr;
	delete m_pTag;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CIndexed::CLoadDataThread Implementation
typedef CIndexed::CLoadDataThread CLoadDataThread;
IMPLEMENT_DYNCREATE(CLoadDataThread, CWinThread)

CIndexed::CLoadDataThread::CLoadDataThread()
{
	m_pOwner = NULL;
}

BOOL CIndexed::CLoadDataThread::InitInstance()
{
	InitThreadLocale();
	return TRUE;
}

int CIndexed::CLoadDataThread::Run()
{
	DbgSetThreadName("Kademlia Indexed Load Data");
	if ( !m_pOwner )
		return 0;

	ASSERT( m_pOwner->m_bDataLoaded == false );
	CSingleLock sLock(&m_pOwner->m_mutSync);
	sLock.Lock();

	try
	{
		uint32 uTotalLoad = 0;
		uint32 uTotalSource = 0;
		uint32 uTotalKeyword = 0;
		CUInt128 uKeyID, uID, uSourceID;
		
		if (!m_pOwner->m_bAbortLoading)
		{
			CBufferedFileIO fileLoad;
			if(fileLoad.Open(m_sLoadFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite))
			{
				setvbuf(fileLoad.m_pStream, NULL, _IOFBF, 32768);
				uint32 uVersion = fileLoad.ReadUInt32();
				if(uVersion<2)
				{
					/*time_t tSaveTime = */fileLoad.ReadUInt32();
					uint32 uNumLoad = fileLoad.ReadUInt32();
					while(uNumLoad && !m_pOwner->m_bAbortLoading)
					{
						fileLoad.ReadUInt128(&uKeyID);
						if(m_pOwner->AddLoad(uKeyID, fileLoad.ReadUInt32(), true))
							uTotalLoad++;
						uNumLoad--;
					}
				}
				fileLoad.Close();
			}
			else
				DebugLogWarning(_T("Unable to load Kad file: %s"), m_sLoadFileName);
		}

		if (!m_pOwner->m_bAbortLoading)
		{
			CBufferedFileIO fileKey;
			if (fileKey.Open(m_sKeyFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite))
			{
				setvbuf(fileKey.m_pStream, NULL, _IOFBF, 32768);

				uint32 uVersion = fileKey.ReadUInt32();
				if( uVersion < 5)
				{
					time_t tSaveTime = fileKey.ReadUInt32();
					if( tSaveTime > time(NULL) )
					{
						fileKey.ReadUInt128(&uID);
						if( Kademlia::CKademlia::GetPrefs()->GetKadID() == uID )
						{
							uint32 uNumKeys = fileKey.ReadUInt32();
							while( uNumKeys && !m_pOwner->m_bAbortLoading )
							{
								fileKey.ReadUInt128(&uKeyID);
								uint32 uNumSource = fileKey.ReadUInt32();
								while( uNumSource && !m_pOwner->m_bAbortLoading )
								{
									fileKey.ReadUInt128(&uSourceID);
									uint32 uNumName = fileKey.ReadUInt32();
									while( uNumName && !m_pOwner->m_bAbortLoading)
									{
										CKeyEntry* pToAdd = new Kademlia::CKeyEntry();
										pToAdd->m_uKeyID.SetValue(uKeyID);
										pToAdd->m_uSourceID.SetValue(uSourceID);									
										pToAdd->m_bSource = false;
										pToAdd->m_tLifetime = fileKey.ReadUInt32();
										if (uVersion >= 3)
											pToAdd->ReadPublishTrackingDataFromFile(&fileKey, uVersion >= 4);
										uint32 uTotalTags = fileKey.ReadByte();
										while( uTotalTags )
										{
											CKadTag* pTag = fileKey.ReadTag();
											if(pTag)
											{
												if (!pTag->m_name.Compare(TAG_FILENAME))
												{
													if (pToAdd->GetCommonFileName().IsEmpty())
														pToAdd->SetFileName(pTag->GetStr());
													delete pTag;
												}
												else if (!pTag->m_name.Compare(TAG_FILESIZE))
												{
													pToAdd->m_uSize = pTag->GetInt();
													delete pTag;
												}
												else if (!pTag->m_name.Compare(TAG_SOURCEIP))
												{
													pToAdd->m_uIP = (uint32)pTag->GetInt();
													pToAdd->AddTag(pTag);
												}
												else if (!pTag->m_name.Compare(TAG_SOURCEPORT))
												{
													pToAdd->m_uTCPPort = (uint16)pTag->GetInt();
													pToAdd->AddTag(pTag);
												}
												else if (!pTag->m_name.Compare(TAG_SOURCEUPORT))
												{
													pToAdd->m_uUDPPort = (uint16)pTag->GetInt();
													pToAdd->AddTag(pTag);
												}
												else
												{
													pToAdd->AddTag(pTag);
												}
											}
											uTotalTags--;
										}
										uint8 uLoad;
										if(m_pOwner->AddKeyword(uKeyID, uSourceID, pToAdd, uLoad, true))
											uTotalKeyword++;
										else
											delete pToAdd;
										uNumName--;
									}
									uNumSource--;
								}
								uNumKeys--;
							}
						}
					}
				}
				fileKey.Close();
			}
			else
				DebugLogWarning(_T("Unable to load Kad file: %s"), m_sKeyFileName);
		}

		if (!m_pOwner->m_bAbortLoading)
		{
			CBufferedFileIO fileSource;
			if (fileSource.Open(m_sSourceFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite))
			{
				setvbuf(fileSource.m_pStream, NULL, _IOFBF, 32768);

				uint32 uVersion = fileSource.ReadUInt32();
				if( uVersion < 3 )
				{
					time_t tSaveTime = fileSource.ReadUInt32();
					if( tSaveTime > time(NULL) )
					{
						uint32 uNumKeys = fileSource.ReadUInt32();
						while( uNumKeys && !m_pOwner->m_bAbortLoading )
						{
							fileSource.ReadUInt128(&uKeyID);
							uint32 uNumSource = fileSource.ReadUInt32();
							while( uNumSource && !m_pOwner->m_bAbortLoading )
							{
								fileSource.ReadUInt128(&uSourceID);
								uint32 uNumName = fileSource.ReadUInt32();
								while( uNumName && !m_pOwner->m_bAbortLoading )
								{
									CEntry* pToAdd = new Kademlia::CEntry();
									pToAdd->m_bSource = true;
									pToAdd->m_tLifetime = fileSource.ReadUInt32();
									uint32 uTotalTags = fileSource.ReadByte();
									while( uTotalTags )
									{
										CKadTag* pTag = fileSource.ReadTag();
										if(pTag)
										{
											if (!pTag->m_name.Compare(TAG_SOURCEIP))
											{
												pToAdd->m_uIP = (uint32)pTag->GetInt();
												pToAdd->AddTag(pTag);
											}
											else if (!pTag->m_name.Compare(TAG_SOURCEPORT))
											{
												pToAdd->m_uTCPPort = (uint16)pTag->GetInt();
												pToAdd->AddTag(pTag);
											}
											else if (!pTag->m_name.Compare(TAG_SOURCEUPORT))
											{
												pToAdd->m_uUDPPort = (uint16)pTag->GetInt();
												pToAdd->AddTag(pTag);
											}
											else
											{
												pToAdd->AddTag(pTag);
											}
										}
										uTotalTags--;
									}
									pToAdd->m_uKeyID.SetValue(uKeyID);
									pToAdd->m_uSourceID.SetValue(uSourceID);
									uint8 uLoad;
									if(m_pOwner->AddSources(uKeyID, uSourceID, pToAdd, uLoad, true))
										uTotalSource++;
									else
										delete pToAdd;
									uNumName--;
								}
								uNumSource--;
							}
							uNumKeys--;
						}
					}
				}
				fileSource.Close();

				m_pOwner->m_uTotalIndexSource = uTotalSource;
				m_pOwner->m_uTotalIndexKeyword = uTotalKeyword;
				m_pOwner->m_uTotalIndexLoad = uTotalLoad;
				AddDebugLogLine( false, _T("Read %u source, %u keyword, and %u load entries"), uTotalSource, uTotalKeyword, uTotalLoad);
			}
			else
				DebugLogWarning(_T("Unable to load Kad file: %s"), m_sSourceFileName);
		}
	}
	catch ( CIOException *ioe )
	{
		AddDebugLogLine( false, _T("CIndexed::CLoadDataThread::Run (IO error(%i))"), ioe->m_iCause);
		ioe->Delete();
	}
	catch (...)
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::CLoadDataThread::Run"));
		ASSERT( false );
	}
	if (m_pOwner->m_bAbortLoading)
		AddDebugLogLine(false, _T("Terminating CIndexed::CLoadDataThread - early abort requested"));
	else
		AddDebugLogLine(false, _T("Terminating CIndexed::CLoadDataThread - finished loading data"));

	m_pOwner->m_bDataLoaded = true;
	return 0;
}
