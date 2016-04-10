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

#include "StdAfx.h"
#include "shahashset.h"
#include "opcodes.h"
#include "otherfunctions.h"
#include "emule.h"
#include "safefile.h"
#include "knownfile.h"
#include "preferences.h"
#include "sha.h"
#include "updownclient.h"
#include "DownloadQueue.h"
#include "partfile.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// for this version the limits are set very high, they might be lowered later
// to make a hash trustworthy, at least 10 unique Ips (255.255.128.0) must have send it
// and if we have received more than one hash  for the file, one hash has to be send by more than 95% of all unique IPs
#define MINUNIQUEIPS_TOTRUST		10	// how many unique IPs most have send us a hash to make it trustworthy
#define	MINPERCENTAGE_TOTRUST		92  // how many percentage of clients most have sent the same hash to make it trustworthy

CList<CAICHRequestedData> CAICHRecoveryHashSet::m_liRequestedData;
CMutex					  CAICHRecoveryHashSet::m_mutKnown2File;
CMap<CAICHHash, const CAICHHash&, ULONGLONG, ULONGLONG> CAICHRecoveryHashSet::m_mapAICHHashsStored;

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHash
CString CAICHHash::GetString() const{
	return EncodeBase32(m_abyBuffer, HASHSIZE);
}

void CAICHHash::Read(CFileDataIO* file)	{ 
	file->Read(m_abyBuffer,HASHSIZE);
}

void CAICHHash::Write(CFileDataIO* file) const{ 
	file->Write(m_abyBuffer,HASHSIZE);
}
/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashTree

CAICHHashTree::CAICHHashTree(uint64 nDataSize, bool bLeftBranch, uint64 nBaseSize){
	m_nDataSize = nDataSize;
	SetBaseSize(nBaseSize);
	m_bIsLeftBranch = bLeftBranch;
	m_pLeftTree = NULL;
	m_pRightTree = NULL;
	m_bHashValid = false;
}

CAICHHashTree::~CAICHHashTree(){
	delete m_pLeftTree;
	delete m_pRightTree;
}

void CAICHHashTree::SetBaseSize(uint64 uValue) {
	// BaseSize: to save ressources we use a bool to store the basesize as currently only two values are used
	// keep the original number based calculations and checks in the code through, so it can easily be adjusted in case we want to use a hashset with different basesizes
	if (uValue != PARTSIZE && uValue != EMBLOCKSIZE){
		ASSERT( false );
		theApp.QueueDebugLogLine(true, _T("CAICHHashTree::SetBaseSize() Bug!"));
	}
	m_bBaseSize = (uValue >= PARTSIZE) ? true : false;
}

uint64	CAICHHashTree::GetBaseSize() const
{ 
	return m_bBaseSize ? PARTSIZE : EMBLOCKSIZE; 
}

// recursive
CAICHHashTree* CAICHHashTree::FindHash(uint64 nStartPos, uint64 nSize, uint8* nLevel){
	(*nLevel)++;
	if (*nLevel > 22){ // sanity
		ASSERT( false );
		return false;
	}
	if (nStartPos + nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return NULL;
	}
	if (nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return NULL;
	}

	if (nStartPos == 0 && nSize == m_nDataSize){
		// this is the searched hash
		return this;
	}
	else if (m_nDataSize <= GetBaseSize()){ // sanity
		// this is already the last level, cant go deeper
		ASSERT( false );
		return NULL;
	}
	else{
		uint64 nBlocks = m_nDataSize / GetBaseSize() + ((m_nDataSize % GetBaseSize() != 0 )? 1:0); 
		uint64 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* GetBaseSize();
		uint64 nRight = m_nDataSize - nLeft;
		if (nStartPos < nLeft){
			if (nStartPos + nSize > nLeft){ // sanity
				ASSERT ( false );
				return NULL;
			}
			if (m_pLeftTree == NULL)
				m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pLeftTree->m_nDataSize == nLeft );
			}
			return m_pLeftTree->FindHash(nStartPos, nSize, nLevel);
		}
		else{
			nStartPos -= nLeft;
			if (nStartPos + nSize > nRight){ // sanity
				ASSERT ( false );
				return NULL;
			}
			if (m_pRightTree == NULL)
				m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pRightTree->m_nDataSize == nRight ); 
			}
			return m_pRightTree->FindHash(nStartPos, nSize, nLevel);
		}
	}
}

// find existing hash, which is part of a properly build tree/branch
// recursive
const CAICHHashTree* CAICHHashTree::FindExistingHash(uint64 nStartPos, uint64 nSize, uint8* nLevel) const
{
	(*nLevel)++;
	if (*nLevel > 22){ // sanity
		ASSERT( false );
		return false;
	}
	if (nStartPos + nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return NULL;
	}
	if (nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return NULL;
	}

	if (nStartPos == 0 && nSize == m_nDataSize){
		// this is the searched hash
		if (m_bHashValid)
			return this;
		else
			return NULL;
	}
	else if (m_nDataSize <= GetBaseSize()){ // sanity
		// this is already the last level, cant go deeper
		ASSERT( false );
		return NULL;
	}
	else{
		uint64 nBlocks = m_nDataSize / GetBaseSize() + ((m_nDataSize % GetBaseSize() != 0 )? 1:0); 
		uint64 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* GetBaseSize();
		uint64 nRight = m_nDataSize - nLeft;
		if (nStartPos < nLeft){
			if (nStartPos + nSize > nLeft){ // sanity
				ASSERT ( false );
				return NULL;
			}
			if (m_pLeftTree == NULL || !m_pLeftTree->m_bHashValid)
				return NULL;
			else{
				ASSERT( m_pLeftTree->m_nDataSize == nLeft );
				return m_pLeftTree->FindExistingHash(nStartPos, nSize, nLevel);
			}
		}
		else{
			nStartPos -= nLeft;
			if (nStartPos + nSize > nRight){ // sanity
				ASSERT ( false );
				return NULL;
			}
			if (m_pRightTree == NULL || !m_pRightTree->m_bHashValid)
				return NULL;
			else{
				ASSERT( m_pRightTree->m_nDataSize == nRight );
				return m_pRightTree->FindExistingHash(nStartPos, nSize, nLevel);
			}
		}
	}
}

// recursive
// calculates missing hash fromt he existing ones
// overwrites existing hashs
// fails if no hash is found for any branch
bool CAICHHashTree::ReCalculateHash(CAICHHashAlgo* hashalg, bool bDontReplace){
	ASSERT ( !( (m_pLeftTree != NULL) ^ (m_pRightTree != NULL)) ); 
	if (m_pLeftTree && m_pRightTree){
		if ( !m_pLeftTree->ReCalculateHash(hashalg, bDontReplace) || !m_pRightTree->ReCalculateHash(hashalg, bDontReplace) )
			return false;
		if (bDontReplace && m_bHashValid)
			return true;
		if (m_pRightTree->m_bHashValid && m_pLeftTree->m_bHashValid){
			hashalg->Reset();
			hashalg->Add(m_pLeftTree->m_Hash.GetRawHash(), HASHSIZE);
			hashalg->Add(m_pRightTree->m_Hash.GetRawHash(), HASHSIZE);
			hashalg->Finish(m_Hash);
			m_bHashValid = true;
			return true;
		}
		else
			return m_bHashValid;
	}
	else
		return true;
}

bool CAICHHashTree::VerifyHashTree(CAICHHashAlgo* hashalg, bool bDeleteBadTrees){
	if (!m_bHashValid){
		ASSERT ( false );
		if (bDeleteBadTrees){
			delete m_pLeftTree;
			m_pLeftTree = NULL;
			delete m_pRightTree;
			m_pRightTree = NULL;
		}
		theApp.QueueDebugLogLine(/*DLP_HIGH,*/ false, _T("VerifyHashTree - No masterhash available"));
		return false;
	}

	// calculate missing hashs without overwriting anything
	if (m_pLeftTree && !m_pLeftTree->m_bHashValid)
		m_pLeftTree->ReCalculateHash(hashalg, true);
	if (m_pRightTree && !m_pRightTree->m_bHashValid)
		m_pRightTree->ReCalculateHash(hashalg, true);

	if ((m_pRightTree && m_pRightTree->m_bHashValid) ^ (m_pLeftTree && m_pLeftTree->m_bHashValid)){
		// one branch can never be verified
		if (bDeleteBadTrees){
			delete m_pLeftTree;
			m_pLeftTree = NULL;
			delete m_pRightTree;
			m_pRightTree = NULL;
		}
		theApp.QueueDebugLogLine(/*DLP_HIGH,*/ false, _T("VerifyHashSet failed - Hashtree incomplete"));
		return false;
	}
	if ((m_pRightTree && m_pRightTree->m_bHashValid) && (m_pLeftTree && m_pLeftTree->m_bHashValid)){			
		// check verify the hashs of both child nodes against my hash 

		CAICHHash CmpHash;
		hashalg->Reset();
		hashalg->Add(m_pLeftTree->m_Hash.GetRawHash(), HASHSIZE);
		hashalg->Add(m_pRightTree->m_Hash.GetRawHash(), HASHSIZE);
		hashalg->Finish(CmpHash);

		if (m_Hash != CmpHash){
			if (bDeleteBadTrees){
				delete m_pLeftTree;
				m_pLeftTree = NULL;
				delete m_pRightTree;
				m_pRightTree = NULL;
			}
			return false;
		}
		return m_pLeftTree->VerifyHashTree(hashalg, bDeleteBadTrees) && m_pRightTree->VerifyHashTree(hashalg, bDeleteBadTrees);
	}
	else
	{
		// last hash in branch - nothing below to verify

		// delete empty (without a hash) branches if they exist - they may actually have hashes in leafs below but they are not part of the tree
		// because of that, the tree itself can still succeed verification
		if (bDeleteBadTrees)
		{
			if (m_pLeftTree && !m_pLeftTree->m_bHashValid)
			{
				delete m_pLeftTree;
				m_pLeftTree = NULL;
			}
			if (m_pRightTree && !m_pRightTree->m_bHashValid)
			{
				delete m_pRightTree;
				m_pRightTree = NULL;
			}
		}
		return true;
	}

}

void CAICHHashTree::SetBlockHash(uint64 nSize, uint64 nStartPos, CAICHHashAlgo* pHashAlg){
	ASSERT ( nSize <= EMBLOCKSIZE );
	CAICHHashTree* pToInsert = FindHash(nStartPos, nSize);
	if (pToInsert == NULL){ // sanity
		ASSERT ( false );
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Critical Error: Failed to Insert SHA-HashBlock, FindHash() failed!"));
		return;
	}
	
	//sanity
	if (pToInsert->GetBaseSize() != EMBLOCKSIZE || pToInsert->m_nDataSize != nSize){
		ASSERT ( false );
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Critical Error: Logical error on values in SetBlockHashFromData"));
		return;
	}

	pHashAlg->Finish(pToInsert->m_Hash);
	pToInsert->m_bHashValid = true;
	//DEBUG_ONLY(theApp.QueueDebugLogLine(/*DLP_VERYLOW,*/ false, _T("Set ShaHash for block %u - %u (%u Bytes) to %s"), nStartPos, nStartPos + nSize, nSize, pToInsert->m_Hash.GetString()) );
	
}

bool CAICHHashTree::CreatePartRecoveryData(uint64 nStartPos, uint64 nSize, CFileDataIO* fileDataOut, uint32 wHashIdent, bool b32BitIdent){
	if (nStartPos + nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return false;
	}
	if (nSize > m_nDataSize){ // sanity
		ASSERT ( false );
		return false;
	}

	if (nStartPos == 0 && nSize == m_nDataSize){
		// this is the searched part, now write all blocks of this part
		// hashident for this level will be adjsuted by WriteLowestLevelHash
		return WriteLowestLevelHashs(fileDataOut, wHashIdent, false, b32BitIdent);
	}
	else if (m_nDataSize <= GetBaseSize()){ // sanity
		// this is already the last level, cant go deeper
		ASSERT( false );
		return false;
	}
	else{
		wHashIdent <<= 1;
		wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
		
		uint64 nBlocks = m_nDataSize / GetBaseSize() + ((m_nDataSize % GetBaseSize() != 0 )? 1:0); 
		uint64 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* GetBaseSize();
		uint64 nRight = m_nDataSize - nLeft;
		if (m_pLeftTree == NULL || m_pRightTree == NULL){
			ASSERT( false );
			return false;
		}
		if (nStartPos < nLeft){
			if (nStartPos + nSize > nLeft || !m_pRightTree->m_bHashValid){ // sanity
				ASSERT ( false );
				return false;
			}
			m_pRightTree->WriteHash(fileDataOut, wHashIdent, b32BitIdent);
			return m_pLeftTree->CreatePartRecoveryData(nStartPos, nSize, fileDataOut, wHashIdent, b32BitIdent);
		}
		else{
			nStartPos -= nLeft;
			if (nStartPos + nSize > nRight || !m_pLeftTree->m_bHashValid){ // sanity
				ASSERT ( false );
				return false;
			}
			m_pLeftTree->WriteHash(fileDataOut, wHashIdent, b32BitIdent);
			return m_pRightTree->CreatePartRecoveryData(nStartPos, nSize, fileDataOut, wHashIdent, b32BitIdent);

		}
	}
}

void CAICHHashTree::WriteHash(CFileDataIO* fileDataOut, uint32 wHashIdent, bool b32BitIdent) const{
	ASSERT( m_bHashValid );
	wHashIdent <<= 1;
	wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
	if (!b32BitIdent){
		ASSERT( wHashIdent <= 0xFFFF );
		fileDataOut->WriteUInt16((uint16)wHashIdent);
	}
	else
		fileDataOut->WriteUInt32(wHashIdent);
	m_Hash.Write(fileDataOut);
}

// write lowest level hashs into file, ordered from left to right optional without identifier
bool CAICHHashTree::WriteLowestLevelHashs(CFileDataIO* fileDataOut, uint32 wHashIdent, bool bNoIdent, bool b32BitIdent) const{
	wHashIdent <<= 1;
	wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
	if (m_pLeftTree == NULL && m_pRightTree == NULL){
		if (m_nDataSize <= GetBaseSize() && m_bHashValid ){
			if (!bNoIdent && !b32BitIdent){
				ASSERT( wHashIdent <= 0xFFFF );
				fileDataOut->WriteUInt16((uint16)wHashIdent);
			}
			else if (!bNoIdent && b32BitIdent)
				fileDataOut->WriteUInt32(wHashIdent);
			m_Hash.Write(fileDataOut);
			//theApp.AddDebugLogLine(false,_T("%s"),m_Hash.GetString(), wHashIdent, this);
			return true;
		}
		else{
			ASSERT( false );
			return false;
		}
	}
	else if (m_pLeftTree == NULL || m_pRightTree == NULL){
		ASSERT( false );
		return false;
	}
	else{
		return m_pLeftTree->WriteLowestLevelHashs(fileDataOut, wHashIdent, bNoIdent, b32BitIdent)
				&& m_pRightTree->WriteLowestLevelHashs(fileDataOut, wHashIdent, bNoIdent, b32BitIdent);
	}
}

// recover all low level hashs from given data. hashs are assumed to be ordered in left to right - no identifier used
bool CAICHHashTree::LoadLowestLevelHashs(CFileDataIO* fileInput){
	if (m_nDataSize <= GetBaseSize()){ // sanity
		// lowest level, read hash
		m_Hash.Read(fileInput);
		//theApp.AddDebugLogLine(false,m_Hash.GetString());
		m_bHashValid = true; 
		return true;
	}
	else{
		uint64 nBlocks = m_nDataSize / GetBaseSize() + ((m_nDataSize % GetBaseSize() != 0 )? 1:0); 
		uint64 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* GetBaseSize();
		uint64 nRight = m_nDataSize - nLeft;
		if (m_pLeftTree == NULL)
			m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
		else{
			ASSERT( m_pLeftTree->m_nDataSize == nLeft );
		}
		if (m_pRightTree == NULL)
			m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
		else{
			ASSERT( m_pRightTree->m_nDataSize == nRight ); 
		}
		return m_pLeftTree->LoadLowestLevelHashs(fileInput)
				&& m_pRightTree->LoadLowestLevelHashs(fileInput);
	}
}


// write the hash, specified by wHashIdent, with Data from fileInput.
bool CAICHHashTree::SetHash(CFileDataIO* fileInput, uint32 wHashIdent, sint8 nLevel, bool bAllowOverwrite){
	if (nLevel == (-1)){
		// first call, check how many level we need to go
		uint8 i;
		for (i = 0; i != 32 && (wHashIdent & 0x80000000) == 0; i++){
			wHashIdent <<= 1;
		}
		if (i > 31){
			theApp.QueueDebugLogLine(/*DLP_HIGH,*/ false, _T("CAICHHashTree::SetHash - found invalid HashIdent (0)"));
			return false;
		}
		else{
			nLevel = 31 - i;
		}
	}
	if (nLevel == 0){
		// this is the searched hash
		if (m_bHashValid && !bAllowOverwrite){
			// not allowed to overwrite this hash, however move the filepointer by reading a hash
			CAICHHash(fileInput);
			return true;
		}
		m_Hash.Read(fileInput);
		m_bHashValid = true; 
		return true;
	}
	else if (m_nDataSize <= GetBaseSize()){ // sanity
		// this is already the last level, cant go deeper
		ASSERT( false );
		return false;
	}
	else{
		// adjust ident to point the path to the next node
		wHashIdent <<= 1;
		nLevel--;
		uint64 nBlocks = m_nDataSize / GetBaseSize() + ((m_nDataSize % GetBaseSize() != 0 )? 1:0); 
		uint64 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* GetBaseSize();
		uint64 nRight = m_nDataSize - nLeft;
		if ((wHashIdent & 0x80000000) > 0){
			if (m_pLeftTree == NULL)
				m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pLeftTree->m_nDataSize == nLeft );
			}
			return m_pLeftTree->SetHash(fileInput, wHashIdent, nLevel);
		}
		else{
			if (m_pRightTree == NULL)
				m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				ASSERT( m_pRightTree->m_nDataSize == nRight ); 
			}
			return m_pRightTree->SetHash(fileInput, wHashIdent, nLevel);
		}
	}
}

// removes all hashes from the hashset which have a smaller BaseSize (not DataSize) than the given one
bool CAICHHashTree::ReduceToBaseSize(uint64 nBaseSize) {
	bool bDeleted = false;
	if (m_pLeftTree != NULL){
		if (m_pLeftTree->GetBaseSize() < nBaseSize){
			delete m_pLeftTree;
			m_pLeftTree = NULL;
			bDeleted = true;
		}
		else
			bDeleted |= m_pLeftTree->ReduceToBaseSize(nBaseSize);
	}
	if (m_pRightTree != NULL){
		if (m_pRightTree->GetBaseSize() < nBaseSize){
			delete m_pRightTree;
			m_pRightTree = NULL;
			bDeleted = true;
		}
		else
			bDeleted |= m_pRightTree->ReduceToBaseSize(nBaseSize);
	}
	return bDeleted;
}


/////////////////////////////////////////////////////////////////////////////////////////
///CAICHUntrustedHash
bool CAICHUntrustedHash::AddSigningIP(uint32 dwIP, bool bTestOnly){
	dwIP &= 0x00F0FFFF; // we use only the 20 most significant bytes for unique IPs
	for (int i=0; i < m_adwIpsSigning.GetCount(); i++){
		if (m_adwIpsSigning[i] == dwIP)
			return false;
	}
	if (!bTestOnly)
		m_adwIpsSigning.Add(dwIP);
	return true;
}



/////////////////////////////////////////////////////////////////////////////////////////
///CAICHRecoveryHashSet
CAICHRecoveryHashSet::CAICHRecoveryHashSet(CKnownFile* pOwner, EMFileSize nSize)
	: m_pHashTree(0, true, PARTSIZE)
{
	m_eStatus = AICH_EMPTY;
	m_pOwner = pOwner;
	if (nSize != (uint64)0)
		SetFileSize(nSize);
}

CAICHRecoveryHashSet::~CAICHRecoveryHashSet(void)
{
	FreeHashSet();
}

bool CAICHRecoveryHashSet::GetPartHashs(CArray<CAICHHash>& rResult) const
{
	ASSERT( m_pOwner );
	ASSERT( rResult.IsEmpty() );
	rResult.RemoveAll();
	if (m_pOwner->IsPartFile() || m_eStatus != AICH_HASHSETCOMPLETE){
		ASSERT( false );
		return false;
	}

	uint32 uPartCount = (uint16)(((uint64)m_pOwner->GetFileSize() + (PARTSIZE - 1)) / PARTSIZE);
	if (uPartCount <= 1)
		return true; // No AICH Part Hashs
	for (uint32 nPart = 0; nPart < uPartCount; nPart++)
	{
		uint64 nPartStartPos = (uint64)nPart*PARTSIZE;
		uint32 nPartSize = (uint32)min(PARTSIZE, (uint64)m_pOwner->GetFileSize()-nPartStartPos);
		const CAICHHashTree* pPartHashTree = m_pHashTree.FindExistingHash(nPartStartPos, nPartSize);
		if (pPartHashTree != NULL && pPartHashTree->m_bHashValid)
			rResult.Add(pPartHashTree->m_Hash);
		else
		{
			rResult.RemoveAll();
			ASSERT( false );
			return false;
		}
	}
	return true;
}

const CAICHHashTree* CAICHRecoveryHashSet::FindPartHash(uint16 nPart)
{
	ASSERT( m_pOwner );
	ASSERT( m_pOwner->IsPartFile() );
	if ( m_pOwner->GetFileSize() <= PARTSIZE )
		return &m_pHashTree;
	uint64 nPartStartPos = (uint64)nPart*PARTSIZE;
	uint32 nPartSize = (uint32)(min(PARTSIZE, (uint64)m_pOwner->GetFileSize()-nPartStartPos));
	const CAICHHashTree* phtResult = m_pHashTree.FindHash(nPartStartPos, nPartSize);
	ASSERT( phtResult != NULL );
	return phtResult;
}

bool CAICHRecoveryHashSet::CreatePartRecoveryData(uint64 nPartStartPos, CFileDataIO* fileDataOut, bool bDbgDontLoad){
	ASSERT( m_pOwner );
	if (m_pOwner->IsPartFile() || m_eStatus != AICH_HASHSETCOMPLETE){
		ASSERT( false );
		return false;
	}
	if (m_pHashTree.m_nDataSize <= EMBLOCKSIZE){
		ASSERT( false );
		return false;
	}
	if (!bDbgDontLoad){
		if (!LoadHashSet()){
			theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Created RecoveryData error: failed to load hashset (file: %s)"), m_pOwner->GetFileName() );
			SetStatus(AICH_ERROR);
			return false;
		}
	}
	bool bResult;
	uint8 nLevel = 0;
	uint32 nPartSize = (uint32)min(PARTSIZE, (uint64)m_pOwner->GetFileSize()-nPartStartPos);
	m_pHashTree.FindHash(nPartStartPos, nPartSize,&nLevel);
	uint16 nHashsToWrite = (uint16)((nLevel-1) + nPartSize/EMBLOCKSIZE + ((nPartSize % EMBLOCKSIZE != 0 )? 1:0));
	const bool bUse32BitIdentifier = m_pOwner->IsLargeFile();

	if (bUse32BitIdentifier)
		fileDataOut->WriteUInt16(0); // no 16bit hashs to write
	fileDataOut->WriteUInt16(nHashsToWrite);
	uint32 nCheckFilePos = (UINT)fileDataOut->GetPosition();
	if (m_pHashTree.CreatePartRecoveryData(nPartStartPos, nPartSize, fileDataOut, 0, bUse32BitIdentifier)){
		if (nHashsToWrite*(HASHSIZE+(bUse32BitIdentifier? 4:2)) != fileDataOut->GetPosition() - nCheckFilePos){
			ASSERT( false );
			theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Created RecoveryData has wrong length (file: %s)"), m_pOwner->GetFileName() );
			bResult = false;
			SetStatus(AICH_ERROR);
		}
		else
			bResult = true;
	}
	else{
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to create RecoveryData for %s"), m_pOwner->GetFileName() );
		bResult = false;
		SetStatus(AICH_ERROR);
	}
	if (!bUse32BitIdentifier)
		fileDataOut->WriteUInt16(0); // no 32bit hashs to write

	if (!bDbgDontLoad){
		FreeHashSet();
	}
	return bResult;
}

bool CAICHRecoveryHashSet::ReadRecoveryData(uint64 nPartStartPos, CSafeMemFile* fileDataIn){
	if (/*TODO !m_pOwner->IsPartFile() ||*/ !(m_eStatus == AICH_VERIFIED || m_eStatus == AICH_TRUSTED) ){
		ASSERT( false );
		return false;
	}
	/* V2 AICH Hash Packet:
		<count1 uint16>											16bit-hashs-to-read
		(<identifier uint16><hash HASHSIZE>)[count1]			AICH hashs
		<count2 uint16>											32bit-hashs-to-read
		(<identifier uint32><hash HASHSIZE>)[count2]			AICH hashs
	*/

	// at this time we check the recoverydata for the correct ammounts of hashs only
	// all hash are then taken into the tree, depending on there hashidentifier (except the masterhash)

	uint8 nLevel = 0;
	uint32 nPartSize = (uint32)min(PARTSIZE, (uint64)m_pOwner->GetFileSize()-nPartStartPos);
	m_pHashTree.FindHash(nPartStartPos, nPartSize,&nLevel);
	uint16 nHashsToRead = (uint16)((nLevel-1) + nPartSize/EMBLOCKSIZE + ((nPartSize % EMBLOCKSIZE != 0 )? 1:0));
	
	// read hashs with 16 bit identifier
	uint16 nHashsAvailable = fileDataIn->ReadUInt16();
	if (fileDataIn->GetLength()-fileDataIn->GetPosition() < nHashsToRead*(HASHSIZE+2) || (nHashsToRead != nHashsAvailable && nHashsAvailable != 0)){
		// this check is redunant, CSafememfile would catch such an error too
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Received datasize/amounts of hashs was invalid (1)"), m_pOwner->GetFileName() );
		return false;
	}
	DEBUG_ONLY( theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("read RecoveryData for %s - Received packet with  %u 16bit hash identifiers)"), m_pOwner->GetFileName(), nHashsAvailable ) );
	for (uint32 i = 0; i != nHashsAvailable; i++){
		uint16 wHashIdent = fileDataIn->ReadUInt16();
		if (wHashIdent == 1 /*never allow masterhash to be overwritten*/
			|| !m_pHashTree.SetHash(fileDataIn, wHashIdent,(-1), false))
		{
			theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Error when trying to read hash into tree (1)"), m_pOwner->GetFileName() );
			VerifyHashTree(true); // remove invalid hashs which we have already written
			return false;
		}
	}

	// read hashs with 32bit identifier
	if (nHashsAvailable == 0 && fileDataIn->GetLength() - fileDataIn->GetPosition() >= 2){
		nHashsAvailable = fileDataIn->ReadUInt16();
		if (fileDataIn->GetLength()-fileDataIn->GetPosition() < nHashsToRead*(HASHSIZE+4) || (nHashsToRead != nHashsAvailable && nHashsAvailable != 0)){
			// this check is redunant, CSafememfile would catch such an error too
			theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Received datasize/amounts of hashs was invalid (2)"), m_pOwner->GetFileName() );
			return false;
		}
		DEBUG_ONLY( theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("read RecoveryData for %s - Received packet with  %u 32bit hash identifiers)"), m_pOwner->GetFileName(), nHashsAvailable ) );
		for (uint32 i = 0; i != nHashsToRead; i++){
			uint32 wHashIdent = fileDataIn->ReadUInt32();
			if (wHashIdent == 1 /*never allow masterhash to be overwritten*/
				|| wHashIdent > 0x400000
				|| !m_pHashTree.SetHash(fileDataIn, wHashIdent,(-1), false))
			{
				theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Error when trying to read hash into tree (2)"), m_pOwner->GetFileName() );
				VerifyHashTree(true); // remove invalid hashs which we have already written
				return false;
			}
		}
	}
	else if (fileDataIn->GetLength() - fileDataIn->GetPosition() >= 2)
    {
        fileDataIn->ReadUInt16();
    }

	
	if (nHashsAvailable == 0){
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Packet didn't contained any hashs"), m_pOwner->GetFileName() );
		return false;
	}


	if (VerifyHashTree(true)){
		// some final check if all hashs we wanted are there
		for (uint32 nPartPos = 0; nPartPos < nPartSize; nPartPos += EMBLOCKSIZE){
			const CAICHHashTree* phtToCheck = m_pHashTree.FindExistingHash(nPartStartPos+nPartPos, min(EMBLOCKSIZE, nPartSize-nPartPos));
			if (phtToCheck == NULL || !phtToCheck->m_bHashValid){
				theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Error while verifying presence of all lowest level hashs"), m_pOwner->GetFileName() );
				return false;
			}
		}
		// all done
		return true;
	}
	else{
		theApp.QueueDebugLogLine(/*DLP_VERYHIGH,*/ false, _T("Failed to read RecoveryData for %s - Verifying received hashtree failed"), m_pOwner->GetFileName() );
		return false;
	}
}

// this function is only allowed to be called right after successfully calculating the hashset (!)
// will delete the hashset, after saving to free the memory
bool CAICHRecoveryHashSet::SaveHashSet(){
	if (m_eStatus != AICH_HASHSETCOMPLETE){
		ASSERT( false );
		return false;
	}
	if ( !m_pHashTree.m_bHashValid || m_pHashTree.m_nDataSize != m_pOwner->GetFileSize()){
		ASSERT( false );
		return false;
	}
	CSingleLock lockKnown2Met(&m_mutKnown2File, false);
	if (!lockKnown2Met.Lock(5000)){
		return false;
	}

	CString fullpath = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR);
	fullpath.Append(KNOWN2_MET_FILENAME);
	CSafeFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeCreate|CFile::modeReadWrite|CFile::modeNoTruncate|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyNone, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") KNOWN2_MET_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			theApp.QueueLogLine(true, _T("%s"), strError);
		}
		return false;
	}
	try {
		//setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
		uint8 header = file.ReadUInt8();
		if (header != KNOWN2_MET_VERSION){
			AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
		}
		// first we check if the hashset we want to write is already stored
		ULONGLONG nTmp;
		if (m_mapAICHHashsStored.Lookup(m_pHashTree.m_Hash, nTmp) == TRUE)
		{
			theApp.QueueDebugLogLine(false, _T("AICH Hashset to write should be already present in known2.met - %s"), m_pHashTree.m_Hash.GetString());
			// this hashset if already available, no need to save it again
			return true;
		}		

		// write hashset
		uint32 nExistingSize = (UINT)file.GetLength();
		file.SeekToEnd();
		ULONGLONG nHashSetWritePosition = file.GetPosition();
		m_pHashTree.m_Hash.Write(&file);
		uint32 nHashCount = (uint32)((PARTSIZE/EMBLOCKSIZE + ((PARTSIZE % EMBLOCKSIZE != 0)? 1 : 0)) * (m_pHashTree.m_nDataSize/PARTSIZE));
		if (m_pHashTree.m_nDataSize % PARTSIZE != 0)
			nHashCount += (uint32)((m_pHashTree.m_nDataSize % PARTSIZE)/EMBLOCKSIZE + (((m_pHashTree.m_nDataSize % PARTSIZE) % EMBLOCKSIZE != 0)? 1 : 0));
		file.WriteUInt32(nHashCount);
		if (!m_pHashTree.WriteLowestLevelHashs(&file, 0, true, true)){
			// thats bad... really
			file.SetLength(nExistingSize);
			theApp.QueueDebugLogLine(true, _T("Failed to save HashSet: WriteLowestLevelHashs() failed!"));
			return false;
		}
		if (file.GetLength() != nExistingSize + (nHashCount+1)*HASHSIZE + 4){
			// thats even worse
			file.SetLength(nExistingSize);
			theApp.QueueDebugLogLine(true, _T("Failed to save HashSet: Calculated and real size of hashset differ!"));
			return false;
		}
		CAICHRecoveryHashSet::AddStoredAICHHash(m_pHashTree.m_Hash, nHashSetWritePosition);
		theApp.QueueDebugLogLine(false, _T("Successfully saved eMuleAC Hashset, %u Hashs + 1 Masterhash written"), nHashCount);
	    file.Flush();
	    file.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			theApp.QueueLogLine(true, GetResString(IDS_ERR_MET_BAD), KNOWN2_MET_FILENAME);
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			theApp.QueueLogLine(true,GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
		}
		error->Delete();
		return false;
	}
	FreeHashSet();
	return true;
}


bool CAICHRecoveryHashSet::LoadHashSet(){
	if (m_eStatus != AICH_HASHSETCOMPLETE){
		ASSERT( false );
		return false;
	}
	if ( !m_pHashTree.m_bHashValid || m_pHashTree.m_nDataSize != m_pOwner->GetFileSize() || m_pHashTree.m_nDataSize == 0){
		ASSERT( false );
		return false;
	}
	CString fullpath = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR);
	fullpath.Append(KNOWN2_MET_FILENAME);
	CSafeFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeCreate|CFile::modeRead|CFile::modeNoTruncate|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyNone, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") KNOWN2_MET_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			theApp.QueueLogLine(true, _T("%s"), strError);
		}
		return false;
	}
	try {
		//setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
		uint8 header = file.ReadUInt8();
		if (header != KNOWN2_MET_VERSION){
			AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
		}
		CAICHHash CurrentHash;
		uint32 nExistingSize = (UINT)file.GetLength();
		uint32 nHashCount;
		
		bool bUseExpectedPos = true;
		ULONGLONG nExpectedHashSetPos = 0;
		if (m_mapAICHHashsStored.Lookup(m_pHashTree.m_Hash, nExpectedHashSetPos) == FALSE || nExpectedHashSetPos >= nExistingSize)
		{
			bUseExpectedPos = false;
			theApp.QueueDebugLogLine(false, _T("AICH Hashset to read not present in AICH hash index - %s"), m_pHashTree.m_Hash.GetString());
		}	
		
		while (file.GetPosition() < nExistingSize)
		{
			if (bUseExpectedPos)
			{
				ULONGLONG nFallbackPos = file.GetPosition();
				file.Seek(nExpectedHashSetPos, CFile::begin);
				CurrentHash.Read(&file);
				if (m_pHashTree.m_Hash != CurrentHash)
				{
					ASSERT( false );
					theApp.QueueDebugLogLine(false, _T("AICH Hashset to read not present at expected position according to AICH hash index - %s"), m_pHashTree.m_Hash.GetString());
					// fallback and do a full search of the file for the hashset
					file.Seek(nFallbackPos, CFile::begin);
					CurrentHash.Read(&file);	
				}
				bUseExpectedPos = false;
			}
			else
				CurrentHash.Read(&file);
			if (m_pHashTree.m_Hash == CurrentHash){
				// found Hashset
				uint32 nExpectedCount =	(uint32)((PARTSIZE/EMBLOCKSIZE + ((PARTSIZE % EMBLOCKSIZE != 0)? 1 : 0)) * (m_pHashTree.m_nDataSize/PARTSIZE));
				if (m_pHashTree.m_nDataSize % PARTSIZE != 0)
					nExpectedCount += (uint32)((m_pHashTree.m_nDataSize % PARTSIZE)/EMBLOCKSIZE + (((m_pHashTree.m_nDataSize % PARTSIZE) % EMBLOCKSIZE != 0)? 1 : 0));
				nHashCount = file.ReadUInt32();
				if (nHashCount != nExpectedCount){
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: Available Hashs and expected hashcount differ!"));
					return false;
				}
				//uint32 dbgPos = file.GetPosition();
				if (!m_pHashTree.LoadLowestLevelHashs(&file)){
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: LoadLowestLevelHashs failed!"));
					return false;
				}
				//uint32 dbgHashRead = (file.GetPosition()-dbgPos)/HASHSIZE;
				if (!ReCalculateHash(false)){
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: Calculating loaded hashs failed!"));
					return false;
				}
				if (CurrentHash != m_pHashTree.m_Hash){
					theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: Calculated Masterhash differs from given Masterhash - hashset corrupt!"));
					return false;
				}
				return true;
			}
			nHashCount = file.ReadUInt32();
			if (file.GetPosition() + nHashCount*HASHSIZE > nExistingSize){
				AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*HASHSIZE, CFile::current);
		}
		theApp.QueueDebugLogLine(true, _T("Failed to load HashSet: HashSet not found!"));
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			theApp.QueueLogLine(true, GetResString(IDS_ERR_MET_BAD), KNOWN2_MET_FILENAME);
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			theApp.QueueLogLine(true,GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
		}
		error->Delete();
	}
	return false;
}

// delete the hashset except the masterhash
void CAICHRecoveryHashSet::FreeHashSet(){
	delete m_pHashTree.m_pLeftTree;
	m_pHashTree.m_pLeftTree = NULL;
	delete m_pHashTree.m_pRightTree;
	m_pHashTree.m_pRightTree = NULL;
}

void CAICHRecoveryHashSet::SetMasterHash(const CAICHHash& Hash, EAICHStatus eNewStatus){
	m_pHashTree.m_Hash = Hash;
	m_pHashTree.m_bHashValid = true;
	SetStatus(eNewStatus);
}

CAICHHashAlgo*	CAICHRecoveryHashSet::GetNewHashAlgo(){
	return new CSHA();
}

bool CAICHRecoveryHashSet::ReCalculateHash(bool bDontReplace){
	CAICHHashAlgo* hashalg = GetNewHashAlgo();
	bool bResult = m_pHashTree.ReCalculateHash(hashalg, bDontReplace);
	delete hashalg;
	return bResult;
}

bool CAICHRecoveryHashSet::VerifyHashTree(bool bDeleteBadTrees){
	CAICHHashAlgo* hashalg = GetNewHashAlgo();
	bool bResult = m_pHashTree.VerifyHashTree(hashalg, bDeleteBadTrees);
	delete hashalg;
	return bResult;
}

void CAICHRecoveryHashSet::SetFileSize(EMFileSize nSize){
	m_pHashTree.m_nDataSize = nSize;
	m_pHashTree.SetBaseSize((nSize <= (uint64)PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);	
}

void CAICHRecoveryHashSet::UntrustedHashReceived(const CAICHHash& Hash, uint32 dwFromIP){
	switch(GetStatus()){
		case AICH_EMPTY:
		case AICH_UNTRUSTED:
		case AICH_TRUSTED:
			break;
		default:
			return;
	}
	bool bFound = false;
	bool bAdded = false;
	for (int i = 0; i < m_aUntrustedHashs.GetCount(); i++){
		if (m_aUntrustedHashs[i].m_Hash == Hash){
			bAdded = m_aUntrustedHashs[i].AddSigningIP(dwFromIP, false);
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		for (int i = 0; i < m_aUntrustedHashs.GetCount(); i++)
		{
			if (!m_aUntrustedHashs[i].AddSigningIP(dwFromIP, true))
			{
				AddDebugLogLine(DLP_LOW, false, _T("Received different AICH hashs for file %s from IP/20 %s, ignored"), (m_pOwner != NULL) ? m_pOwner->GetFileName() : _T(""), ipstr(dwFromIP)); 
				// nothing changed, so we can return early without any rechecks
				return;
			}
		}
		bAdded = true;
		CAICHUntrustedHash uhToAdd;
		uhToAdd.m_Hash = Hash;
		uhToAdd.AddSigningIP(dwFromIP, false);
		m_aUntrustedHashs.Add(uhToAdd);
	}

	uint32 nSigningIPsTotal = 0;	// unique clients who send us a hash
	int nMostTrustedPos = (-1);  // the hash which most clients send us
	uint32 nMostTrustedIPs = 0;
	for (uint32 i = 0; i < (uint32)m_aUntrustedHashs.GetCount(); i++){
		nSigningIPsTotal += m_aUntrustedHashs[i].m_adwIpsSigning.GetCount();
		if ((uint32)m_aUntrustedHashs[i].m_adwIpsSigning.GetCount() > nMostTrustedIPs){
			nMostTrustedIPs = m_aUntrustedHashs[i].m_adwIpsSigning.GetCount();
			nMostTrustedPos = i;
		}
	}
	if (nMostTrustedPos == (-1) || nSigningIPsTotal == 0){
		ASSERT( false );
		return;
	}
	// the check if we trust any hash
	if ( thePrefs.IsTrustingEveryHash() ||
		(nMostTrustedIPs >= MINUNIQUEIPS_TOTRUST && (100 * nMostTrustedIPs)/nSigningIPsTotal >= MINPERCENTAGE_TOTRUST)){
		//trusted
			//theApp.QueueDebugLogLine(false, _T("AICH Hash received: %s (%sadded), We have now %u hash from %u unique IPs. We trust the Hash %s from %u clients (%u%%). Added IP:%s, file: %s")
			//, Hash.GetString(), bAdded? _T(""):_T("not "), m_aUntrustedHashs.GetCount(), nSigningIPsTotal, m_aUntrustedHashs[nMostTrustedPos].m_Hash.GetString()
			//, nMostTrustedIPs, (100 * nMostTrustedIPs)/nSigningIPsTotal, ipstr(dwFromIP & 0x00F0FFFF), m_pOwner->GetFileName());
		
		SetStatus(AICH_TRUSTED);
		if (!HasValidMasterHash() || GetMasterHash() != m_aUntrustedHashs[nMostTrustedPos].m_Hash){
			SetMasterHash(m_aUntrustedHashs[nMostTrustedPos].m_Hash, AICH_TRUSTED);
			FreeHashSet();
		}
	}
	else{
		// untrusted
		//theApp.QueueDebugLogLine(false, _T("AICH Hash received: %s (%sadded), We have now %u hash from %u unique IPs. Best Hash (%s) is from %u clients (%u%%) - but we dont trust it yet. Added IP:%s, file: %s")
		//	, Hash.GetString(), bAdded? _T(""):_T("not "), m_aUntrustedHashs.GetCount(), nSigningIPsTotal, m_aUntrustedHashs[nMostTrustedPos].m_Hash.GetString()
		//	, nMostTrustedIPs, (100 * nMostTrustedIPs)/nSigningIPsTotal, ipstr(dwFromIP & 0x00F0FFFF), m_pOwner->GetFileName());
		
		SetStatus(AICH_UNTRUSTED);
		if (!HasValidMasterHash() || GetMasterHash() != m_aUntrustedHashs[nMostTrustedPos].m_Hash){
			SetMasterHash(m_aUntrustedHashs[nMostTrustedPos].m_Hash, AICH_UNTRUSTED);
			FreeHashSet();
		}
	}
}

void CAICHRecoveryHashSet::ClientAICHRequestFailed(CUpDownClient* pClient){
	pClient->SetReqFileAICHHash(NULL);
	CAICHRequestedData data = GetAICHReqDetails(pClient);
	RemoveClientAICHRequest(pClient);
	if (data.m_pClient != pClient)
		return;
	if(theApp.downloadqueue->IsPartFile(data.m_pPartFile)){
		theApp.QueueDebugLogLine(false, _T("AICH Request failed, Trying to ask another client (file %s, Part: %u,  Client%s)"), data.m_pPartFile->GetFileName(), data.m_nPart, pClient->DbgGetClientInfo());
		data.m_pPartFile->RequestAICHRecovery(data.m_nPart);
	}
}

void CAICHRecoveryHashSet::RemoveClientAICHRequest(const CUpDownClient* pClient){
	for (POSITION pos = m_liRequestedData.GetHeadPosition();pos != 0; m_liRequestedData.GetNext(pos))
	{
		if (m_liRequestedData.GetAt(pos).m_pClient == pClient){
			m_liRequestedData.RemoveAt(pos);
			return;
		}
	}
	ASSERT( false );
}

bool CAICHRecoveryHashSet::IsClientRequestPending(const CPartFile* pForFile, uint16 nPart){
	for (POSITION pos = m_liRequestedData.GetHeadPosition();pos != 0; m_liRequestedData.GetNext(pos))
	{
		if (m_liRequestedData.GetAt(pos).m_pPartFile == pForFile && m_liRequestedData.GetAt(pos).m_nPart == nPart){
			return true;
		}
	}
	return false;
}

CAICHRequestedData CAICHRecoveryHashSet::GetAICHReqDetails(const  CUpDownClient* pClient){
	for (POSITION pos = m_liRequestedData.GetHeadPosition();pos != 0; m_liRequestedData.GetNext(pos))
	{
		if (m_liRequestedData.GetAt(pos).m_pClient == pClient){
			return m_liRequestedData.GetAt(pos);
		}
	}	
	ASSERT( false );
	CAICHRequestedData empty;
	return empty;
}

void CAICHRecoveryHashSet::AddStoredAICHHash(CAICHHash Hash, ULONGLONG nFilePos)
{
#ifdef _DEBUG
	ULONGLONG nTmp;
	if (m_mapAICHHashsStored.Lookup(Hash, nTmp) == TRUE)
	{
		theApp.QueueDebugLogLine(false, _T("AICH hash storing is not unique - %s"), Hash.GetString());
		ASSERT( false );
	}
#endif
	m_mapAICHHashsStored.SetAt(Hash, nFilePos);
}

bool CAICHRecoveryHashSet::IsPartDataAvailable(uint64 nPartStartPos){
	if (!(m_eStatus == AICH_VERIFIED || m_eStatus == AICH_TRUSTED || m_eStatus == AICH_HASHSETCOMPLETE) ){
		ASSERT( false );
		return false;
	}
	uint32 nPartSize = (uint32)(min(PARTSIZE, (uint64)m_pOwner->GetFileSize()-nPartStartPos));
	for (uint64 nPartPos = 0; nPartPos < nPartSize; nPartPos += EMBLOCKSIZE){
		const CAICHHashTree* phtToCheck = m_pHashTree.FindExistingHash(nPartStartPos+nPartPos, min(EMBLOCKSIZE, nPartSize-nPartPos));
		if (phtToCheck == NULL || !phtToCheck->m_bHashValid){
			return false;
		}
	}
	return true;
}

void CAICHRecoveryHashSet::DbgTest(){
#ifdef _DEBUG
	//define TESTSIZE 4294567295
	uint8 maxLevel = 0;
	uint32 cHash = 1;
	uint8 curLevel = 0;
	//uint32 cParts = 0;
	maxLevel = 0;
/*	CAICHHashTree* pTest = new CAICHHashTree(TESTSIZE, true, 9728000);
	for (uint64 i = 0; i+9728000 < TESTSIZE; i += 9728000){
		CAICHHashTree* pTest2 = new CAICHHashTree(9728000, true, EMBLOCKSIZE);
		pTest->ReplaceHashTree(i, 9728000, &pTest2);
		cParts++;
	}
	CAICHHashTree* pTest2 = new CAICHHashTree(TESTSIZE-i, true, EMBLOCKSIZE);
	pTest->ReplaceHashTree(i, (TESTSIZE-i), &pTest2);
	cParts++;
*/
#define TESTSIZE m_pHashTree.m_nDataSize
	if (m_pHashTree.m_nDataSize <= EMBLOCKSIZE)
		return;
	CAICHRecoveryHashSet TestHashSet(m_pOwner);
	TestHashSet.SetFileSize(m_pOwner->GetFileSize());
	TestHashSet.SetMasterHash(GetMasterHash(), AICH_VERIFIED);
	CSafeMemFile file;
	uint64 i;
	for (i = 0; i+9728000 < TESTSIZE; i += 9728000){
		VERIFY( CreatePartRecoveryData(i, &file) );
		
		/*uint32 nRandomCorruption = (rand() * rand()) % (file.GetLength()-4);
		file.Seek(nRandomCorruption, CFile::begin);
		file.Write(&nRandomCorruption, 4);*/

		file.SeekToBegin();
		VERIFY( TestHashSet.ReadRecoveryData(i, &file) );
		file.SeekToBegin();
		TestHashSet.FreeHashSet();
		uint32 j;
		for (j = 0; j+EMBLOCKSIZE < 9728000; j += EMBLOCKSIZE){
			VERIFY( m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel) );
			//TRACE(_T("%u - %s\r\n"), cHash, m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel)->m_Hash.GetString());
			maxLevel = max(curLevel, maxLevel);
			curLevel = 0;
			cHash++;
		}
		VERIFY( m_pHashTree.FindHash(i+j, 9728000-j, &curLevel) );
		//TRACE(_T("%u - %s\r\n"), cHash, m_pHashTree.FindHash(i+j, 9728000-j, &curLevel)->m_Hash.GetString());
		maxLevel = max(curLevel, maxLevel);
		curLevel = 0;
		cHash++;

	}
	VERIFY( CreatePartRecoveryData(i, &file) );
	file.SeekToBegin();
	VERIFY( TestHashSet.ReadRecoveryData(i, &file) );
	file.SeekToBegin();
	TestHashSet.FreeHashSet();
	uint64 j;
	for (j = 0; j+EMBLOCKSIZE < TESTSIZE-i; j += EMBLOCKSIZE){
		VERIFY( m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel) );
		//TRACE(_T("%u - %s\r\n"), cHash,m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel)->m_Hash.GetString());
		maxLevel = max(curLevel, maxLevel);
		curLevel = 0;
		cHash++;
	}
	//VERIFY( m_pHashTree.FindHash(i+j, (TESTSIZE-i)-j, &curLevel) );
	TRACE(_T("%u - %s\r\n"), cHash,m_pHashTree.FindHash(i+j, (TESTSIZE-i)-j, &curLevel)->m_Hash.GetString());
	maxLevel = max(curLevel, maxLevel);
#endif
}
