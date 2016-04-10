// parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
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
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "KnownFile.h"
#include "KnownFileList.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "MMServer.h"
#include "ClientList.h"
#include "opcodes.h"
#include "ini2.h"
#include "FrameGrabThread.h"
#include "CxImage/xImage.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include "Packets.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "Kademlia/Kademlia/Entry.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "SafeFile.h"
#include "shahashset.h"
#include "Log.h"
#include "MD4.h"
#include "Collection.h"
#include "emuledlg.h"
#include "SharedFilesWnd.h"
#include "MediaInfo.h"
#pragma warning(disable:4100) // unreferenced formal parameter
#include <id3/tag.h>
#include <id3/misc_support.h>
#pragma warning(default:4100) // unreferenced formal parameter
extern wchar_t *ID3_GetStringW(const ID3_Frame *frame, ID3_FieldID fldName);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Meta data version
// -----------------
//	0	untrusted meta data which was received via search results
//	1	trusted meta data, Unicode (strings where not stored correctly)
//	2	0.49c: trusted meta data, Unicode
#define	META_DATA_VER	2

IMPLEMENT_DYNAMIC(CKnownFile, CShareableFile)

CKnownFile::CKnownFile()
{
	m_iPartCount = 0;
	m_iED2KPartCount = 0;
	m_tUtcLastModified = (UINT)-1;
	if(thePrefs.GetNewAutoUp()){
		m_iUpPriority = PR_HIGH;
		m_bAutoUpPriority = true;
	}
	else{
		m_iUpPriority = PR_NORMAL;
		m_bAutoUpPriority = false;
	}
	statistic.fileParent = this;
	(void)m_strComment;
	m_PublishedED2K = false;
	kadFileSearchID = 0;
	SetLastPublishTimeKadSrc(0,0);
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 1;
	m_nCompleteSourcesCountLo = 1;
	m_nCompleteSourcesCountHi = 1;
	m_uMetaDataVer = 0;
	m_lastPublishTimeKadSrc = 0;
	m_lastPublishTimeKadNotes = 0;
	m_lastBuddyIP = 0;
	m_pCollection = NULL;
	m_timeLastSeen = 0;
	m_bAICHRecoverHashSetAvailable = false;
}

CKnownFile::~CKnownFile()
{
	delete m_pCollection;
}

#ifdef _DEBUG
void CKnownFile::AssertValid() const
{
	CAbstractFile::AssertValid();

	(void)m_tUtcLastModified;
	(void)statistic;
	(void)m_nCompleteSourcesTime;
	(void)m_nCompleteSourcesCount;
	(void)m_nCompleteSourcesCountLo;
	(void)m_nCompleteSourcesCountHi;
	m_ClientUploadList.AssertValid();
	m_AvailPartFrequency.AssertValid();
	(void)m_strDirectory;
	(void)m_strFilePath;
	(void)m_iPartCount;
	(void)m_iED2KPartCount;
	ASSERT( m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH );
	CHECK_BOOL(m_bAutoUpPriority);
	(void)s_ShareStatusBar;
	CHECK_BOOL(m_PublishedED2K);
	(void)kadFileSearchID;
	(void)m_lastPublishTimeKadSrc;
	(void)m_lastPublishTimeKadNotes;
	(void)m_lastBuddyIP;
	(void)wordlist;
}

void CKnownFile::Dump(CDumpContext& dc) const
{
	CAbstractFile::Dump(dc);
}
#endif

CBarShader CKnownFile::s_ShareStatusBar(16);

void CKnownFile::DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool  bFlat) const
{
	s_ShareStatusBar.SetFileSize(GetFileSize());
	s_ShareStatusBar.SetHeight(rect->bottom - rect->top);
	s_ShareStatusBar.SetWidth(rect->right - rect->left);

    if(m_ClientUploadList.GetSize() > 0 || m_nCompleteSourcesCountHi > 1) {
        // We have info about chunk frequency in the net, so we will color the chunks we have after perceived availability.
    	const COLORREF crMissing = RGB(255, 0, 0);
	    s_ShareStatusBar.Fill(crMissing);

	    if (!onlygreyrect) {
		    COLORREF crProgress;
		    COLORREF crHave;
		    COLORREF crPending;
		    if(bFlat) { 
			    crProgress = RGB(0, 150, 0);
			    crHave = RGB(0, 0, 0);
			    crPending = RGB(255,208,0);
		    } else { 
			    crProgress = RGB(0, 224, 0);
			    crHave = RGB(104, 104, 104);
			    crPending = RGB(255, 208, 0);
	        }

            uint32 tempCompleteSources = m_nCompleteSourcesCountLo;
            if(tempCompleteSources > 0) {
                tempCompleteSources--;
            }

		    for (UINT i = 0; i < GetPartCount(); i++){
                uint32 frequency = tempCompleteSources;
                if(!m_AvailPartFrequency.IsEmpty()) {
                    frequency = max(m_AvailPartFrequency[i], tempCompleteSources);
                }

			    if(frequency > 0 ){
				    COLORREF color = RGB(0, (22*(frequency-1) >= 210)? 0:210-(22*(frequency-1)), 255);
				    s_ShareStatusBar.FillRange(PARTSIZE*(uint64)(i),PARTSIZE*(uint64)(i+1),color);
			    }
	        }
	    }
    } else {
        // We have no info about chunk frequency in the net, so just color the chunk we have as black.
        COLORREF crNooneAsked;
		if(bFlat) { 
		    crNooneAsked = RGB(0, 0, 0);
		} else { 
		    crNooneAsked = RGB(104, 104, 104);
	    }
		s_ShareStatusBar.Fill(crNooneAsked);
    }

   	s_ShareStatusBar.Draw(dc, rect->left, rect->top, bFlat); 
} 

// SLUGFILLER: heapsortCompletesrc
static void HeapSort(CArray<uint16,uint16> &count, UINT first, UINT last){
	UINT r;
	for ( r = first; !(r & (UINT)INT_MIN) && (r<<1) < last; ){
		UINT r2 = (r<<1)+1;
		if (r2 != last)
			if (count[r2] < count[r2+1])
				r2++;
		if (count[r] < count[r2]){
			uint16 t = count[r2];
			count[r2] = count[r];
			count[r] = t;
			r = r2;
		}
		else
			break;
	}
}
// SLUGFILLER: heapsortCompletesrc

void CKnownFile::UpdateFileRatingCommentAvail(bool bForceUpdate)
{
	bool bOldHasComment = m_bHasComment;
	UINT uOldUserRatings = m_uUserRating;

	m_bHasComment = false;
	UINT uRatings = 0;
	UINT uUserRatings = 0;

	for(POSITION pos = m_kadNotes.GetHeadPosition(); pos != NULL; )
	{
		Kademlia::CEntry* entry = m_kadNotes.GetNext(pos);
		if (!m_bHasComment && !entry->GetStrTagValue(TAG_DESCRIPTION).IsEmpty())
			m_bHasComment = true;
		UINT rating = (UINT)entry->GetIntTagValue(TAG_FILERATING);
		if (rating != 0)
		{
			uRatings++;
			uUserRatings += rating;
		}
	}

	if (uRatings)
		m_uUserRating = (uint32)ROUND((float)uUserRatings / uRatings);
	else
		m_uUserRating = 0;

	if (bOldHasComment != m_bHasComment || uOldUserRatings != m_uUserRating || bForceUpdate)
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

void CKnownFile::UpdatePartsInfo()
{
	// Cache part count
	UINT partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 
	
	// Reset part counters
	if ((UINT)m_AvailPartFrequency.GetSize() < partcount)
		m_AvailPartFrequency.SetSize(partcount);
	for (UINT i = 0; i < partcount; i++)
		m_AvailPartFrequency[i] = 0;

	CArray<uint16, uint16> count;
	if (flag)
		count.SetSize(0, m_ClientUploadList.GetSize());
	for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0; )
	{
		CUpDownClient* cur_src = m_ClientUploadList.GetNext(pos);
		//This could be a partfile that just completed.. Many of these clients will not have this information.
		if (cur_src->m_abyUpPartStatus && cur_src->GetUpPartCount() == partcount)
		{
			for (UINT i = 0; i < partcount; i++)
			{
				if (cur_src->IsUpPartAvailable(i))
					m_AvailPartFrequency[i] += 1;
			}
			if (flag)
				count.Add(cur_src->GetUpCompleteSourcesCount());
		}
	}

	if (flag)
	{
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

		if (partcount > 0)
			m_nCompleteSourcesCount = m_AvailPartFrequency[0];
		for (UINT i = 1; i < partcount; i++)
		{
			if (m_nCompleteSourcesCount > m_AvailPartFrequency[i])
				m_nCompleteSourcesCount = m_AvailPartFrequency[i];
		}
	
		count.Add(m_nCompleteSourcesCount+1); // plus 1 since we have the file complete too
	
		int n = count.GetSize();
		if (n > 0)
		{
			// SLUGFILLER: heapsortCompletesrc
			int r;
			for (r = n/2; r--; )
				HeapSort(count, r, n-1);
			for (r = n; --r; ){
				uint16 t = count[r];
				count[r] = count[0];
				count[0] = t;
				HeapSort(count, 0, r-1);
			}
			// SLUGFILLER: heapsortCompletesrc
			
			// calculate range
			int i = n >> 1;			// (n / 2)
			int j = (n * 3) >> 2;	// (n * 3) / 4
			int k = (n * 7) >> 3;	// (n * 7) / 8

			//For complete files, trust the people your uploading to more...

			//For low guess and normal guess count
			//	If we see more sources then the guessed low and normal, use what we see.
			//	If we see less sources then the guessed low, adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the normal.
			//For high guess
			//  Adjust 100% network and 0% what we see.
			if (n < 20)
			{
				if ( count.GetAt(i) < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				else
					m_nCompleteSourcesCountLo = count.GetAt(i);
				m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= count.GetAt(j);
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
			else
			//Many sources..
			//For low guess
			//	Use what we see.
			//For normal guess
			//	Adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the low.
			//For high guess
			//  Adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the normal.
			{
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCount= count.GetAt(j);
				if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo )
					m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= count.GetAt(k);
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	if (theApp.emuledlg->sharedfileswnd->m_hWnd)
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

void CKnownFile::AddUploadingClient(CUpDownClient* client){
	POSITION pos = m_ClientUploadList.Find(client); // to be sure
	if(pos == NULL){
		m_ClientUploadList.AddTail(client);
		UpdateAutoUpPriority();
	}
}

void CKnownFile::RemoveUploadingClient(CUpDownClient* client){
	POSITION pos = m_ClientUploadList.Find(client); // to be sure
	if(pos != NULL){
		m_ClientUploadList.RemoveAt(pos);
		UpdateAutoUpPriority();
	}
}

#ifdef _DEBUG
void Dump(const Kademlia::WordList& wordlist)
{
	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); it++)
	{
		const CStringW& rstrKeyword = *it;
		TRACE("  %ls\n", rstrKeyword);
	}
}
#endif

void CKnownFile::SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars, bool bRemoveControlChars)
{ 
	CKnownFile* pFile = NULL;

	// If this is called within the sharedfiles object during startup,
	// we cannot reference it yet..

	if(theApp.sharedfiles)
		pFile = theApp.sharedfiles->GetFileByID(GetFileHash());

	if (pFile && pFile == this)
		theApp.sharedfiles->RemoveKeywords(this);

	CAbstractFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars, true, bRemoveControlChars);
	m_verifiedFileType = FILETYPE_UNKNOWN;

	wordlist.clear();
	if(m_pCollection)
	{
		CString sKeyWords;
		sKeyWords.Format(_T("%s %s"), m_pCollection->GetCollectionAuthorKeyString(), GetFileName());
		Kademlia::CSearchManager::GetWords(sKeyWords, &wordlist);
	}
	else
		Kademlia::CSearchManager::GetWords(GetFileName(), &wordlist);

	if (pFile && pFile == this)
		theApp.sharedfiles->AddKeywords(this);
} 

bool CKnownFile::CreateFromFile(LPCTSTR in_directory, LPCTSTR in_filename, LPVOID pvProgressParam)
{
	SetPath(in_directory);
	SetFileName(in_filename);

	// open file
	CString strFilePath;
	if (!_tmakepathlimit(strFilePath.GetBuffer(MAX_PATH), NULL, in_directory, in_filename, NULL)){
		LogError(GetResString(IDS_ERR_FILEOPEN), in_filename, _T(""));
		return false;
	}
	strFilePath.ReleaseBuffer();
	SetFilePath(strFilePath);
	FILE* file = _tfsopen(strFilePath, _T("rbS"), _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
	if (!file){
		LogError(GetResString(IDS_ERR_FILEOPEN) + _T(" - %s"), strFilePath, _T(""), _tcserror(errno));
		return false;
	}

	// set filesize
	__int64 llFileSize = _filelengthi64(_fileno(file));
	if ((uint64)llFileSize > MAX_EMULE_FILE_SIZE){
		if (llFileSize == -1i64)
			LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
		else
			LogError(_T("Skipped hashing of file \"%s\" - File size exceeds limit."), strFilePath);
		fclose(file);
		return false; // not supported by network
	}
	SetFileSize((uint64)llFileSize);

	// we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
	setvbuf(file, NULL, _IOFBF, 1024*8*2);

	m_AvailPartFrequency.SetSize(GetPartCount());
	for (UINT i = 0; i < GetPartCount();i++)
		m_AvailPartFrequency[i] = 0;
	
	// create hashset
	CAICHRecoveryHashSet cAICHHashSet(this, m_nFileSize);
	uint64 togo = m_nFileSize;
	UINT hashcount;
	for (hashcount = 0; togo >= PARTSIZE; )
	{
		CAICHHashTree* pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, PARTSIZE);
		ASSERT( pBlockAICHHashTree != NULL );

		uchar* newhash = new uchar[16];
		if (!CreateHash(file, PARTSIZE, newhash, pBlockAICHHashTree)) {
			LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
			fclose(file);
			delete[] newhash;
			return false;
		}

		if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){ // in case of shutdown while still hashing
			fclose(file);
			delete[] newhash;
			return false;
		}

		m_FileIdentifier.GetRawMD4HashSet().Add(newhash);
		togo -= PARTSIZE;
		hashcount++;

		if (pvProgressParam && theApp.emuledlg && theApp.emuledlg->IsRunning()){
			ASSERT( ((CKnownFile*)pvProgressParam)->IsKindOf(RUNTIME_CLASS(CKnownFile)) );
			ASSERT( ((CKnownFile*)pvProgressParam)->GetFileSize() == GetFileSize() );
			UINT uProgress = (UINT)(uint64)(((uint64)(GetFileSize() - togo) * 100) / GetFileSize());
			ASSERT( uProgress <= 100 );
			VERIFY( PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)pvProgressParam) );
		}
	}

	CAICHHashTree* pBlockAICHHashTree;
	if (togo == 0)
		pBlockAICHHashTree = NULL; // sha hashtree doesnt takes hash of 0-sized data
	else{
		pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, togo);
		ASSERT( pBlockAICHHashTree != NULL );
	}
	
	uchar* lasthash = new uchar[16];
	md4clr(lasthash);
	if (!CreateHash(file, togo, lasthash, pBlockAICHHashTree)) {
		LogError(_T("Failed to hash file \"%s\" - %s"), strFilePath, _tcserror(errno));
		fclose(file);
		delete[] lasthash;
		return false;
	}
	
	cAICHHashSet.ReCalculateHash(false);
	if (cAICHHashSet.VerifyHashTree(true))
	{
		cAICHHashSet.SetStatus(AICH_HASHSETCOMPLETE);
		m_FileIdentifier.SetAICHHash(cAICHHashSet.GetMasterHash());
		if (!m_FileIdentifier.SetAICHHashSet(cAICHHashSet))
		{
			ASSERT( false );
			DebugLogError(_T("CreateFromFile() - failed to create AICH PartHashSet out of RecoveryHashSet - %s"), GetFileName());
		}
		if (!cAICHHashSet.SaveHashSet())
			LogError(LOG_STATUSBAR, GetResString(IDS_SAVEACFAILED));
		else
			SetAICHRecoverHashSetAvailable(true);
	}
	else{
		// now something went pretty wrong
		DebugLogError(LOG_STATUSBAR, _T("Failed to calculate AICH Hashset from file %s"), GetFileName());
	}

	if (!hashcount){
		m_FileIdentifier.SetMD4Hash(lasthash);
		delete[] lasthash;
	} 
	else {
		m_FileIdentifier.GetRawMD4HashSet().Add(lasthash);
		m_FileIdentifier.CalculateMD4HashByHashSet(false);
	}

	if (pvProgressParam && theApp.emuledlg && theApp.emuledlg->IsRunning()){
		ASSERT( ((CKnownFile*)pvProgressParam)->IsKindOf(RUNTIME_CLASS(CKnownFile)) );
		ASSERT( ((CKnownFile*)pvProgressParam)->GetFileSize() == GetFileSize() );
		UINT uProgress = 100;
		ASSERT( uProgress <= 100 );
		VERIFY( PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)pvProgressParam) );
	}

	// set lastwrite date
	struct _stat32i64 fileinfo;
	if (_fstat32i64(file->_file, &fileinfo) == 0){
		m_tUtcLastModified = fileinfo.st_mtime;
		AdjustNTFSDaylightFileTime(m_tUtcLastModified, strFilePath);
	}

	fclose(file);
	file = NULL;

	// Add filetags
	UpdateMetaDataTags();

	UpdatePartsInfo();

	return true;	
}

bool CKnownFile::CreateAICHHashSetOnly()
{
	ASSERT( !IsPartFile() );
	
	FILE* file = _tfsopen(GetFilePath(), _T("rbS"), _SH_DENYNO); // can not use _SH_DENYWR because we may access a completing part file
	if (!file){
		LogError(GetResString(IDS_ERR_FILEOPEN) + _T(" - %s"), GetFilePath(), _T(""), _tcserror(errno));
		return false;
	}
	// we are reading the file data later in 8K blocks, adjust the internal file stream buffer accordingly
	setvbuf(file, NULL, _IOFBF, 1024*8*2);

	// create aichhashset
	CAICHRecoveryHashSet cAICHHashSet(this, m_nFileSize);
	uint64 togo = m_nFileSize;
	UINT hashcount;
	for (hashcount = 0; togo >= PARTSIZE; )
	{
		CAICHHashTree* pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, PARTSIZE);
		ASSERT( pBlockAICHHashTree != NULL );
		if (!CreateHash(file, PARTSIZE, NULL, pBlockAICHHashTree)) {
			LogError(_T("Failed to hash file \"%s\" - %s"), GetFilePath(), _tcserror(errno));
			fclose(file);
			return false;
		}

		if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){ // in case of shutdown while still hashing
			fclose(file);
			return false;
		}

		togo -= PARTSIZE;
		hashcount++;
	}

	if (togo != 0)
	{
		CAICHHashTree* pBlockAICHHashTree = cAICHHashSet.m_pHashTree.FindHash((uint64)hashcount*PARTSIZE, togo);
		ASSERT( pBlockAICHHashTree != NULL );
		if (!CreateHash(file, togo, NULL, pBlockAICHHashTree)) {
			LogError(_T("Failed to hash file \"%s\" - %s"), GetFilePath(), _tcserror(errno));
			fclose(file);
			return false;
		}
	}

	cAICHHashSet.ReCalculateHash(false);
	if (cAICHHashSet.VerifyHashTree(true))
	{
		cAICHHashSet.SetStatus(AICH_HASHSETCOMPLETE);
		if (m_FileIdentifier.HasAICHHash() && m_FileIdentifier.GetAICHHash() != cAICHHashSet.GetMasterHash())
			theApp.knownfiles->AICHHashChanged(&m_FileIdentifier.GetAICHHash(), cAICHHashSet.GetMasterHash(), this);
		else
			theApp.knownfiles->AICHHashChanged(NULL, cAICHHashSet.GetMasterHash(), this);
		m_FileIdentifier.SetAICHHash(cAICHHashSet.GetMasterHash());
		if (!m_FileIdentifier.SetAICHHashSet(cAICHHashSet))
		{
			ASSERT( false );
			DebugLogError(_T("CreateAICHHashSetOnly() - failed to create AICH PartHashSet out of RecoveryHashSet - %s"), GetFileName());
		}
		if (!cAICHHashSet.SaveHashSet())
			LogError(LOG_STATUSBAR, GetResString(IDS_SAVEACFAILED));
		else
			SetAICHRecoverHashSetAvailable(true);
	}
	else{
		// now something went pretty wrong
		DebugLogError(LOG_STATUSBAR, _T("Failed to calculate AICH Hashset from file %s"), GetFileName());
	}

	fclose(file);
	file = NULL;
	
	return true;	
}

void CKnownFile::SetFileSize(EMFileSize nFileSize)
{
	CAbstractFile::SetFileSize(nFileSize);

	// Examples of parthashs, hashsets and filehashs for different filesizes
	// according the ed2k protocol
	//----------------------------------------------------------------------
	//
	//File size: 3 bytes
	//File hash: 2D55E87D0E21F49B9AD25F98531F3724
	//Nr. hashs: 0
	//
	//
	//File size: 1*PARTSIZE
	//File hash: A72CA8DF7F07154E217C236C89C17619
	//Nr. hashs: 2
	//Hash[  0]: 4891ED2E5C9C49F442145A3A5F608299
	//Hash[  1]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 1*PARTSIZE + 1 byte
	//File hash: 2F620AE9D462CBB6A59FE8401D2B3D23
	//Nr. hashs: 2
	//Hash[  0]: 121795F0BEDE02DDC7C5426D0995F53F
	//Hash[  1]: C329E527945B8FE75B3C5E8826755747
	//
	//
	//File size: 2*PARTSIZE
	//File hash: A54C5E562D5E03CA7D77961EB9A745A4
	//Nr. hashs: 3
	//Hash[  0]: B3F5CE2A06BF403BFB9BFFF68BDDC4D9
	//Hash[  1]: 509AA30C9EA8FC136B1159DF2F35B8A9
	//Hash[  2]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 3*PARTSIZE
	//File hash: 5E249B96F9A46A18FC2489B005BF2667
	//Nr. hashs: 4
	//Hash[  0]: 5319896A2ECAD43BF17E2E3575278E72
	//Hash[  1]: D86EF157D5E49C5ED502EDC15BB5F82B
	//Hash[  2]: 10F2D5B1FCB95C0840519C58D708480F
	//Hash[  3]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 3*PARTSIZE + 1 byte
	//File hash: 797ED552F34380CAFF8C958207E40355
	//Nr. hashs: 4
	//Hash[  0]: FC7FD02CCD6987DCF1421F4C0AF94FB8
	//Hash[  1]: 2FE466AF8A7C06DA3365317B75A5ACFE
	//Hash[  2]: 873D3BF52629F7C1527C6E8E473C1C30
	//Hash[  3]: BCE50BEE7877BB07BB6FDA56BFE142FB
	//

	// File size       Data parts      ED2K parts      ED2K part hashs		AICH part hashs
	// -------------------------------------------------------------------------------------------
	// 1..PARTSIZE-1   1               1               0(!)					0 (!)
	// PARTSIZE        1               2(!)            2(!)					0 (!)
	// PARTSIZE+1      2               2               2					2
	// PARTSIZE*2      2               3(!)            3(!)					2
	// PARTSIZE*2+1    3               3               3					3

	if (nFileSize == (uint64)0){
		ASSERT(0);
		m_iPartCount = 0;
		m_iED2KPartCount = 0;
		return;
	}

	// nr. of data parts
	ASSERT( (uint64)(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE) <= (UINT)USHRT_MAX );
	m_iPartCount = (uint16)(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE);

	// nr. of parts to be used with OP_FILESTATUS
	m_iED2KPartCount = (uint16)((uint64)nFileSize / PARTSIZE + 1);
}
 
bool CKnownFile::LoadTagsFromFile(CFileDataIO* file)
{
	UINT tagcount = file->ReadUInt32();
	bool bHadAICHHashSetTag = false;
	for (UINT j = 0; j < tagcount; j++){
		CTag* newtag = new CTag(file, false);
		switch (newtag->GetNameID()){
			case FT_FILENAME:{
				ASSERT( newtag->IsStr() );
				if (newtag->IsStr()){
					if (GetFileName().IsEmpty())
						SetFileName(newtag->GetStr());
				}
				delete newtag;
				break;
			}
			case FT_FILESIZE:{
				ASSERT( newtag->IsInt64(true) );
				if (newtag->IsInt64(true))
				{
					SetFileSize(newtag->GetInt64());
					m_AvailPartFrequency.SetSize(GetPartCount());
					for (UINT i = 0; i < GetPartCount();i++)
						m_AvailPartFrequency[i] = 0;
				}
				delete newtag;
				break;
			}
			case FT_ATTRANSFERRED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.SetAllTimeTransferred(newtag->GetInt());
				delete newtag;
				break;
			}
			case FT_ATTRANSFERREDHI:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.SetAllTimeTransferred(((uint64)newtag->GetInt() << 32) | (UINT)statistic.GetAllTimeTransferred());
				delete newtag;
				break;
			}
			case FT_ATREQUESTED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.SetAllTimeRequests(newtag->GetInt());
				delete newtag;
				break;
			}
 			case FT_ATACCEPTED:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					statistic.SetAllTimeAccepts(newtag->GetInt());
				delete newtag;
				break;
			}
			case FT_ULPRIORITY:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
				{
					m_iUpPriority = (uint8)newtag->GetInt();
					if( m_iUpPriority == PR_AUTO ){
						m_iUpPriority = PR_HIGH;
						m_bAutoUpPriority = true;
					}
					else{
						if (m_iUpPriority != PR_VERYLOW && m_iUpPriority != PR_LOW && m_iUpPriority != PR_NORMAL && m_iUpPriority != PR_HIGH && m_iUpPriority != PR_VERYHIGH)
							m_iUpPriority = PR_NORMAL;
						m_bAutoUpPriority = false;
					}
				}
				delete newtag;
				break;
			}
			case FT_KADLASTPUBLISHSRC:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					SetLastPublishTimeKadSrc( newtag->GetInt(), 0 );
				if(GetLastPublishTimeKadSrc() > (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES)
				{
					//There may be a posibility of an older client that saved a random number here.. This will check for that..
					SetLastPublishTimeKadSrc(0,0);
				}
				delete newtag;
				break;
			}
			case FT_KADLASTPUBLISHNOTES:{
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					SetLastPublishTimeKadNotes( newtag->GetInt() );
				delete newtag;
				break;
			}
			case FT_FLAGS:
				// Misc. Flags
				// ------------------------------------------------------------------------------
				// Bits  3-0: Meta data version
				//				0	untrusted meta data which was received via search results
				//				1	trusted meta data, Unicode (strings where not stored correctly)
				//				2	0.49c: trusted meta data, Unicode
				// Bits 31-4: Reserved
				ASSERT( newtag->IsInt() );
				if (newtag->IsInt())
					m_uMetaDataVer = newtag->GetInt() & 0x0F;
				delete newtag;
				break;
			// old tags: as long as they are not needed, take the chance to purge them
			case FT_PERMISSIONS:
				ASSERT( newtag->IsInt() );
				delete newtag;
				break;
			case FT_KADLASTPUBLISHKEY:
				ASSERT( newtag->IsInt() );
				delete newtag;
				break;
			case FT_AICH_HASH:{
				if(!newtag->IsStr()){
					//ASSERT( false ); uncomment later
					break;
				}
				CAICHHash hash;
				if (DecodeBase32(newtag->GetStr(),hash) == (UINT)CAICHHash::GetHashSize())
					m_FileIdentifier.SetAICHHash(hash);
				else
					ASSERT( false );
				delete newtag;
				break;
			}
			case FT_LASTSHARED:
				if (newtag->IsInt())
					m_timeLastSeen = newtag->GetInt();
				else
					ASSERT( false );
				delete newtag;
				break;
			case FT_AICHHASHSET:
				if (newtag->IsBlob())
				{
					CSafeMemFile aichHashSetFile(newtag->GetBlob(), newtag->GetBlobSize());
					m_FileIdentifier.LoadAICHHashsetFromFile(&aichHashSetFile, false);
					aichHashSetFile.Detach();
					bHadAICHHashSetTag = true;
				}
				else
					ASSERT( false );
				delete newtag;
				break;
			default:
				ConvertED2KTag(newtag);
				if (newtag)
					taglist.Add(newtag);
		}
	}
	if (bHadAICHHashSetTag)
	{
		if (!m_FileIdentifier.VerifyAICHHashSet())
			DebugLogError(_T("Failed to load AICH Part HashSet for file %s"), GetFileName());
		//else
		//	DebugLog(_T("Succeeded to load AICH Part HashSet for file %s"), GetFileName());
	}

	// 05-Jän-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
	// the chance to clean any available meta data tags and provide only tags which were determined by us.
	// It's a brute force method, but that wrong meta data is driving me crazy because wrong meta data is even worse than
	// missing meta data.
	if (m_uMetaDataVer == 0)
		RemoveMetaDataTags();
	else if (m_uMetaDataVer == 1)
	{
		// Meta data tags v1 did not store Unicode strings correctly.
		// Remove broken Unicode string meta data tags from v1, but keep the integer tags.
		RemoveBrokenUnicodeMetaDataTags();
		m_uMetaDataVer = META_DATA_VER;
	}

	return true;
}

bool CKnownFile::LoadDateFromFile(CFileDataIO* file){
	m_tUtcLastModified = file->ReadUInt32();
	return true;
}

bool CKnownFile::LoadFromFile(CFileDataIO* file){
	// SLUGFILLER: SafeHash - load first, verify later
	bool ret1 = LoadDateFromFile(file);
	bool ret2 = m_FileIdentifier.LoadMD4HashsetFromFile(file, false);
	bool ret3 = LoadTagsFromFile(file);
	UpdatePartsInfo();
	return ret1 && ret2 && ret3 && m_FileIdentifier.HasExpectedMD4HashCount();// Final hash-count verification, needs to be done after the tags are loaded.
	// SLUGFILLER: SafeHash
}

bool CKnownFile::WriteToFile(CFileDataIO* file)
{
	// date
	file->WriteUInt32(m_tUtcLastModified);

	// hashset
	m_FileIdentifier.WriteMD4HashsetToFile(file);

	uint32 uTagCount = 0;
	ULONG uTagCountFilePos = (ULONG)file->GetPosition();
	file->WriteUInt32(uTagCount);

	CTag nametag(FT_FILENAME, GetFileName());
	nametag.WriteTagToFile(file, utf8strOptBOM);
	uTagCount++;
	
	CTag sizetag(FT_FILESIZE, m_nFileSize, IsLargeFile());
	sizetag.WriteTagToFile(file);
	uTagCount++;

	//AICH Filehash
	if (m_FileIdentifier.HasAICHHash())
	{
		CTag aichtag(FT_AICH_HASH, m_FileIdentifier.GetAICHHash().GetString());
		aichtag.WriteTagToFile(file);
		uTagCount++;
	}

	// last shared
	static bool sDbgWarnedOnZero = false;
	if (!sDbgWarnedOnZero && m_timeLastSeen == 0)
	{
		DebugLog(_T("Unknown last seen date on stored file(s), upgrading from old version?"));
		sDbgWarnedOnZero = true;
	}
	ASSERT( m_timeLastSeen <= time(NULL) );
	time_t timeLastShared = (m_timeLastSeen > 0 && m_timeLastSeen <= time(NULL)) ? m_timeLastSeen : time(NULL);
	CTag lastSharedTag(FT_LASTSHARED, (uint32)timeLastShared);
	lastSharedTag.WriteTagToFile(file);
	uTagCount++;

	if (!ShouldPartiallyPurgeFile())
	{
		// those tags are no longer stored for long time not seen (shared) known files to tidy up known.met and known2.met
		
		// AICH Part HashSet
		// no point in permanently storing the AICH part hashset if we need to rehash the file anyway to fetch the full recovery hashset
		// the tag will make the known.met incompatible with emule version prior 0.44a - but that one is nearly 6 years old 
		if (m_FileIdentifier.HasAICHHash() && m_FileIdentifier.HasExpectedAICHHashCount())
		{
			uint32 nAICHHashSetSize = (CAICHHash::GetHashSize() * (m_FileIdentifier.GetAvailableAICHPartHashCount() + 1)) + 2;
			BYTE* pHashBuffer = new BYTE[nAICHHashSetSize];
			CSafeMemFile hashSetFile(pHashBuffer, nAICHHashSetSize);
			bool bWriteHashSet = true;
			try
			{
				m_FileIdentifier.WriteAICHHashsetToFile(&hashSetFile);
			}
			catch (CFileException* pError)
			{
				ASSERT( false );
				DebugLogError(_T("Memfile Error while storing AICH Part HashSet"));
				bWriteHashSet = false;
				delete[] hashSetFile.Detach();
				pError->Delete();
			}
			if (bWriteHashSet)
			{
				CTag tagAICHHashSet(FT_AICHHASHSET, hashSetFile.Detach(), nAICHHashSetSize);
				tagAICHHashSet.WriteTagToFile(file);
				uTagCount++;
			}
		}

		// statistic
		if (statistic.GetAllTimeTransferred()){
			CTag attag1(FT_ATTRANSFERRED, (uint32)statistic.GetAllTimeTransferred());
			attag1.WriteTagToFile(file);
			uTagCount++;
			
			CTag attag4(FT_ATTRANSFERREDHI, (uint32)(statistic.GetAllTimeTransferred() >> 32));
			attag4.WriteTagToFile(file);
			uTagCount++;
		}

		if (statistic.GetAllTimeRequests()){
			CTag attag2(FT_ATREQUESTED, statistic.GetAllTimeRequests());
			attag2.WriteTagToFile(file);
			uTagCount++;
		}
		
		if (statistic.GetAllTimeAccepts()){
			CTag attag3(FT_ATACCEPTED, statistic.GetAllTimeAccepts());
			attag3.WriteTagToFile(file);
			uTagCount++;
		}

		// priority N permission
		CTag priotag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : m_iUpPriority);
		priotag.WriteTagToFile(file);
		uTagCount++;
		

		if (m_lastPublishTimeKadSrc){
			CTag kadLastPubSrc(FT_KADLASTPUBLISHSRC, m_lastPublishTimeKadSrc);
			kadLastPubSrc.WriteTagToFile(file);
			uTagCount++;
		}

		if (m_lastPublishTimeKadNotes){
			CTag kadLastPubNotes(FT_KADLASTPUBLISHNOTES, m_lastPublishTimeKadNotes);
			kadLastPubNotes.WriteTagToFile(file);
			uTagCount++;
		}

		if (m_uMetaDataVer > 0)
		{
			// Misc. Flags
			// ------------------------------------------------------------------------------
			// Bits  3-0: Meta data version
			//				0	untrusted meta data which was received via search results
			//				1	trusted meta data, Unicode (strings where not stored correctly)
			//				2	0.49c: trusted meta data, Unicode
			// Bits 31-4: Reserved
			ASSERT( m_uMetaDataVer <= 0x0F );
			uint32 uFlags = m_uMetaDataVer & 0x0F;
			CTag tagFlags(FT_FLAGS, uFlags);
			tagFlags.WriteTagToFile(file);
			uTagCount++;
		}

		// other tags
		for (int j = 0; j < taglist.GetCount(); j++){
			if (taglist[j]->IsStr() || taglist[j]->IsInt()){
				taglist[j]->WriteTagToFile(file, utf8strOptBOM);
				uTagCount++;
			}
		}
	}

	file->Seek(uTagCountFilePos, CFile::begin);
	file->WriteUInt32(uTagCount);
	file->Seek(0, CFile::end);

	return true;
}

void CKnownFile::CreateHash(CFile* pFile, uint64 Length, uchar* pMd4HashOut, CAICHHashTree* pShaHashOut)
{
	ASSERT( pFile != NULL );
	ASSERT( pMd4HashOut != NULL || pShaHashOut != NULL );

	uint64  Required = Length;
	uchar   X[64*128];
	uint64	posCurrentEMBlock = 0;
	uint64	nIACHPos = 0;
	CMD4 md4;
	CAICHHashAlgo* pHashAlg = NULL;
	if (pShaHashOut != NULL)
		pHashAlg = CAICHRecoveryHashSet::GetNewHashAlgo();

	while (Required >= 64){
        uint32 len; 
        if ((Required / 64) > sizeof(X)/(64 * sizeof(X[0]))) 
			len = sizeof(X)/(64 * sizeof(X[0]));
		else
			len = (uint32)Required / 64;
		pFile->Read(X, len*64);

		// SHA hash needs 180KB blocks
		if (pShaHashOut != NULL && pHashAlg != NULL){
			if (nIACHPos + len*64 >= EMBLOCKSIZE){
				uint32 nToComplete = (uint32)(EMBLOCKSIZE - nIACHPos);
				pHashAlg->Add(X, nToComplete);
				ASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
				posCurrentEMBlock += EMBLOCKSIZE;
				pHashAlg->Reset();
				pHashAlg->Add(X+nToComplete,(len*64) - nToComplete);
				nIACHPos = (len*64) - nToComplete;
			}
			else{
				pHashAlg->Add(X, len*64);
				nIACHPos += len*64;
			}
		}

		if (pMd4HashOut != NULL){
			md4.Add(X, len*64);
		}
		Required -= len*64;
	}

	Required = Length % 64;
	if (Required != 0){
		pFile->Read(X, (uint32)Required);

		if (pShaHashOut != NULL){
			if (nIACHPos + Required >= EMBLOCKSIZE){
				uint32 nToComplete = (uint32)(EMBLOCKSIZE - nIACHPos);
				pHashAlg->Add(X, nToComplete);
				ASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
				posCurrentEMBlock += EMBLOCKSIZE;
				pHashAlg->Reset();
				pHashAlg->Add(X+nToComplete, (uint32)(Required - nToComplete));
				nIACHPos = Required - nToComplete;
			}
			else{
				pHashAlg->Add(X, (uint32)Required);
				nIACHPos += Required;
			}
		}
	}
	if (pShaHashOut != NULL){
		if(nIACHPos > 0){
			pShaHashOut->SetBlockHash(nIACHPos, posCurrentEMBlock, pHashAlg);
			posCurrentEMBlock += nIACHPos;
		}
		ASSERT( posCurrentEMBlock == Length );
		VERIFY( pShaHashOut->ReCalculateHash(pHashAlg, false) );
	}

	if (pMd4HashOut != NULL){
		md4.Add(X, (uint32)Required);
		md4.Finish();
		md4cpy(pMd4HashOut, md4.GetHash());
	}

	delete pHashAlg;
}

bool CKnownFile::CreateHash(FILE* fp, uint64 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut)
{
	bool bResult = false;
	CStdioFile file(fp);
	try
	{
		CreateHash(&file, uSize, pucHash, pShaHashOut);
		bResult = true;
	}
	catch(CFileException* ex)
	{
		ex->Delete();
	}
	return bResult;
}

bool CKnownFile::CreateHash(const uchar* pucData, uint32 uSize, uchar* pucHash, CAICHHashTree* pShaHashOut)
{
	bool bResult = false;
	CMemFile file(const_cast<uchar*>(pucData), uSize);
	try
	{
		CreateHash(&file, uSize, pucHash, pShaHashOut);
		bResult = true;
	}
	catch(CFileException* ex)
	{
		ex->Delete();
	}
	return bResult;
}

Packet*	CKnownFile::CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const
{
	if (m_ClientUploadList.IsEmpty())
		return NULL;

	if (md4cmp(forClient->GetUploadFileID(), GetFileHash())!=0) {
		// should never happen
		DEBUG_ONLY( DebugLogError(_T("*** %hs - client (%s) upload file \"%s\" does not match file \"%s\""), __FUNCTION__, forClient->DbgGetClientInfo(), DbgGetFileInfo(forClient->GetUploadFileID()), GetFileName()) );
		ASSERT(0);
		return NULL;
	}

	// check whether client has either no download status at all or a download status which is valid for this file
	if (   !(forClient->GetUpPartCount()==0 && forClient->GetUpPartStatus()==NULL)
		&& !(forClient->GetUpPartCount()==GetPartCount() && forClient->GetUpPartStatus()!=NULL)) {
		// should never happen
		DEBUG_ONLY( DebugLogError(_T("*** %hs - part count (%u) of client (%s) does not match part count (%u) of file \"%s\""), __FUNCTION__, forClient->GetUpPartCount(), forClient->DbgGetClientInfo(), GetPartCount(), GetFileName()) );
		ASSERT(0);
		return NULL;
	}

	CSafeMemFile data(1024);
	
	uint8 byUsedVersion;
	bool bIsSX2Packet;
	if (forClient->SupportsSourceExchange2() && byRequestedVersion > 0){
		// the client uses SourceExchange2 and requested the highest version he knows
		// and we send the highest version we know, but of course not higher than his request
		byUsedVersion = min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
		bIsSX2Packet = true;
		data.WriteUInt8(byUsedVersion);

		// we don't support any special SX2 options yet, reserved for later use
		if (nRequestedOptions != 0)
			DebugLogWarning(_T("Client requested unknown options for SourceExchange2: %u (%s)"), nRequestedOptions, forClient->DbgGetClientInfo());
	}
	else{
		byUsedVersion = forClient->GetSourceExchange1Version();
		bIsSX2Packet = false;
		if (forClient->SupportsSourceExchange2())
			DebugLogWarning(_T("Client which announced to support SX2 sent SX1 packet instead (%s)"), forClient->DbgGetClientInfo());
	}

	uint16 nCount = 0;
	data.WriteHash16(forClient->GetUploadFileID());
	data.WriteUInt16(nCount);
	uint32 cDbgNoSrc = 0;
	for (POSITION pos = m_ClientUploadList.GetHeadPosition(); pos != 0; )
	{
		/*const*/ CUpDownClient* cur_src = m_ClientUploadList.GetNext(pos);
		
		// some rare issue seen in a crashdumps, hopefully fixed already, but to be sure we double check here
		// TODO: remove check next version, as it uses ressources and shouldn't be necessary
		if (!theApp.clientlist->IsValidClient(cur_src))
		{
#if defined(_BETA) || defined(_DEVBUILD)
			throw new CUserException();
#endif
			ASSERT( false );
			DebugLogError(_T("Invalid client in uploading list for file %s"), GetFileName());
			return NULL;
		}

		if (cur_src->HasLowID() || cur_src == forClient || !(cur_src->GetUploadState() == US_UPLOADING || cur_src->GetUploadState() == US_ONUPLOADQUEUE))
			continue;
		if (!cur_src->IsEd2kClient())
			continue;

		bool bNeeded = false;
		const uint8* rcvstatus = forClient->GetUpPartStatus();
		if (rcvstatus)
		{
			ASSERT( forClient->GetUpPartCount() == GetPartCount() );
			const uint8* srcstatus = cur_src->GetUpPartStatus();
			if (srcstatus)
			{
				ASSERT( cur_src->GetUpPartCount() == GetPartCount() );
				if (cur_src->GetUpPartCount() == forClient->GetUpPartCount())
				{
					for (UINT x = 0; x < GetPartCount(); x++)
					{
						if (srcstatus[x] && !rcvstatus[x])
						{
							// We know the recieving client needs a chunk from this client.
							bNeeded = true;
							break;
						}
					}
				}
				else
				{
					// should never happen
					//if (thePrefs.GetVerbose())
						DEBUG_ONLY( DebugLogError(_T("*** %hs - found source (%s) with wrong part count (%u) attached to file \"%s\" (partcount=%u)"), __FUNCTION__, cur_src->DbgGetClientInfo(), cur_src->GetUpPartCount(), GetFileName(), GetPartCount()));
				}
			}
			else
			{
				cDbgNoSrc++;
				// This client doesn't support upload chunk status. So just send it and hope for the best.
				bNeeded = true;
			}
		}
		else
		{
			ASSERT( forClient->GetUpPartCount() == 0 );
			TRACE(_T("%hs, requesting client has no chunk status - %s"), __FUNCTION__, forClient->DbgGetClientInfo());
			// remote client does not support upload chunk status, search sources which have at least one complete part
			// we could even sort the list of sources by available chunks to return as much sources as possible which
			// have the most available chunks. but this could be a noticeable performance problem.
			const uint8* srcstatus = cur_src->GetUpPartStatus();
			if (srcstatus)
			{
				ASSERT( cur_src->GetUpPartCount() == GetPartCount() );
				for (UINT x = 0; x < GetPartCount(); x++ )
				{
					if (srcstatus[x])
					{
						// this client has at least one chunk
						bNeeded = true;
						break;
					}
				}
			}
			else
			{
				// This client doesn't support upload chunk status. So just send it and hope for the best.
				bNeeded = true;
			}
		}

		if (bNeeded)
		{
			nCount++;
			uint32 dwID;
			if (byUsedVersion >= 3)
				dwID = cur_src->GetUserIDHybrid();
			else
				dwID = cur_src->GetIP();
		    data.WriteUInt32(dwID);
		    data.WriteUInt16(cur_src->GetUserPort());
		    data.WriteUInt32(cur_src->GetServerIP());
		    data.WriteUInt16(cur_src->GetServerPort());
			if (byUsedVersion >= 2)
			    data.WriteHash16(cur_src->GetUserHash());
			if (byUsedVersion >= 4){
				// ConnectSettings - SourceExchange V4
				// 4 Reserved (!)
				// 1 DirectCallback Supported/Available 
				// 1 CryptLayer Required
				// 1 CryptLayer Requested
				// 1 CryptLayer Supported
				const uint8 uSupportsCryptLayer	= cur_src->SupportsCryptLayer() ? 1 : 0;
				const uint8 uRequestsCryptLayer	= cur_src->RequestsCryptLayer() ? 1 : 0;
				const uint8 uRequiresCryptLayer	= cur_src->RequiresCryptLayer() ? 1 : 0;
				//const uint8 uDirectUDPCallback	= cur_src->SupportsDirectUDPCallback() ? 1 : 0;
				const uint8 byCryptOptions = /*(uDirectUDPCallback << 3) |*/ (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
				data.WriteUInt8(byCryptOptions);
			}
			if (nCount > 500)
				break;
		}
	}
	TRACE(_T("%hs: Out of %u clients, %u had no valid chunk status\n"), __FUNCTION__, m_ClientUploadList.GetCount(), cDbgNoSrc);
	if (!nCount)
		return 0;
	data.Seek(bIsSX2Packet ? 17 : 16, SEEK_SET);
	data.WriteUInt16((uint16)nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = bIsSX2Packet ? OP_ANSWERSOURCES2 : OP_ANSWERSOURCES;
	// (1+)16+2+501*(4+2+4+2+16+1) = 14547 (14548) bytes max.
	if (result->size > 354)
		result->PackPacket();
	if (thePrefs.GetDebugSourceExchange())
		AddDebugLogLine(false, _T("SXSend: Client source response SX2=%s, Version=%u; Count=%u, %s, File=\"%s\""), bIsSX2Packet ? _T("Yes") : _T("No"), byUsedVersion, nCount, forClient->DbgGetClientInfo(), GetFileName());
	return result;
}

void CKnownFile::SetFileComment(LPCTSTR pszComment)
{
	if (m_strComment.Compare(pszComment) != 0)
	{
		SetLastPublishTimeKadNotes(0);
		CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
		ini.WriteStringUTF8(_T("Comment"), pszComment);
		m_strComment = pszComment;

		for (POSITION pos = m_ClientUploadList.GetHeadPosition();pos != 0;)
			m_ClientUploadList.GetNext(pos)->SetCommentDirty();
	}
}

void CKnownFile::SetFileRating(UINT uRating)
{
	if (m_uRating != uRating)
	{
		SetLastPublishTimeKadNotes(0);
		CIni ini(thePrefs.GetFileCommentsFilePath(), md4str(GetFileHash()));
		ini.WriteInt(_T("Rate"), uRating);
		m_uRating = uRating;

		for (POSITION pos = m_ClientUploadList.GetHeadPosition();pos != 0;)
			m_ClientUploadList.GetNext(pos)->SetCommentDirty();
	}
}

void CKnownFile::UpdateAutoUpPriority(){
	if( !IsAutoUpPriority() )
		return;
	if ( GetQueuedCount() > 20 ){
		if( GetUpPriority() != PR_LOW ){
			SetUpPriority( PR_LOW );
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if ( GetQueuedCount() > 1 ){
		if( GetUpPriority() != PR_NORMAL ){
			SetUpPriority( PR_NORMAL );
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
		}
		return;
	}
	if( GetUpPriority() != PR_HIGH){
		SetUpPriority( PR_HIGH );
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
	}
}

void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool bSave)
{
	m_iUpPriority = iNewUpPriority;
	ASSERT( m_iUpPriority == PR_VERYLOW || m_iUpPriority == PR_LOW || m_iUpPriority == PR_NORMAL || m_iUpPriority == PR_HIGH || m_iUpPriority == PR_VERYHIGH );

	if( IsPartFile() && bSave )
		((CPartFile*)this)->SavePartFile();
}

void SecToTimeLength(unsigned long ulSec, CStringA& rstrTimeLength)
{
	// this function creates the content for the "length" ed2k meta tag which was introduced by eDonkeyHybrid 
	// with the data type 'string' :/  to save some bytes we do not format the duration with leading zeros
	if (ulSec >= 3600){
		UINT uHours = ulSec/3600;
		UINT uMin = (ulSec - uHours*3600)/60;
		UINT uSec = ulSec - uHours*3600 - uMin*60;
		rstrTimeLength.Format("%u:%02u:%02u", uHours, uMin, uSec);
	}
	else{
		UINT uMin = ulSec/60;
		UINT uSec = ulSec - uMin*60;
		rstrTimeLength.Format("%u:%02u", uMin, uSec);
	}
}

void SecToTimeLength(unsigned long ulSec, CStringW& rstrTimeLength)
{
	// this function creates the content for the "length" ed2k meta tag which was introduced by eDonkeyHybrid 
	// with the data type 'string' :/  to save some bytes we do not format the duration with leading zeros
	if (ulSec >= 3600){
		UINT uHours = ulSec/3600;
		UINT uMin = (ulSec - uHours*3600)/60;
		UINT uSec = ulSec - uHours*3600 - uMin*60;
		rstrTimeLength.Format(L"%u:%02u:%02u", uHours, uMin, uSec);
	}
	else{
		UINT uMin = ulSec/60;
		UINT uSec = ulSec - uMin*60;
		rstrTimeLength.Format(L"%u:%02u", uMin, uSec);
	}
}

void CKnownFile::RemoveMetaDataTags(UINT uTagType)
{
	static const struct
	{
		uint8	nID;
		uint8	nType;
	} _aEmuleMetaTags[] = 
	{
		{ FT_MEDIA_ARTIST,  TAGTYPE_STRING },
		{ FT_MEDIA_ALBUM,   TAGTYPE_STRING },
		{ FT_MEDIA_TITLE,   TAGTYPE_STRING },
		{ FT_MEDIA_LENGTH,  TAGTYPE_UINT32 },
		{ FT_MEDIA_BITRATE, TAGTYPE_UINT32 },
		{ FT_MEDIA_CODEC,   TAGTYPE_STRING }
	};

	// 05-Jän-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
	// the chance to clean any available meta data tags and provide only tags which were determined by us.
	// Remove all meta tags. Never ever trust the meta tags received from other clients or servers.
	for (int j = 0; j < _countof(_aEmuleMetaTags); j++)
	{
		if (uTagType == 0 || (uTagType == _aEmuleMetaTags[j].nType))
		{
			int i = 0;
			while (i < taglist.GetSize())
			{
				const CTag* pTag = taglist[i];
				if (pTag->GetNameID() == _aEmuleMetaTags[j].nID)
				{
					delete pTag;
					taglist.RemoveAt(i);
				}
				else
					i++;
			}
		}
	}

	m_uMetaDataVer = 0;
}

void CKnownFile::RemoveBrokenUnicodeMetaDataTags()
{
	static const struct
	{
		uint8	nID;
		uint8	nType;
	} _aEmuleMetaTags[] = 
	{
		{ FT_MEDIA_ARTIST,  TAGTYPE_STRING },
		{ FT_MEDIA_ALBUM,   TAGTYPE_STRING },
		{ FT_MEDIA_TITLE,   TAGTYPE_STRING },
		{ FT_MEDIA_CODEC,   TAGTYPE_STRING }	// This one actually contains only ASCII
	};

	for (int j = 0; j < _countof(_aEmuleMetaTags); j++)
	{
		int i = 0;
		while (i < taglist.GetSize())
		{
			// Meta data strings of older eMule versions did store Unicode strings as MBCS strings,
			// which means that - depending on the Unicode string content - particular characters
			// got lost. Unicode characters which cannot get converted into the local codepage
			// will get replaced by Windows with a '?' character. So, to estimate if we have a
			// broken Unicode string (due to the conversion between Unicode/MBCS), we search the
			// strings for '?' characters. This is not 100% perfect, as it would also give
			// false results for strings which do contain the '?' character by intention. It also
			// would give wrong results for particular characters which got mapped to ASCII chars
			// due to the conversion from Unicode->MBCS. But at least it prevents us from deleting
			// all the existing meta data strings.
			const CTag* pTag = taglist[i];
			if (   pTag->GetNameID() == _aEmuleMetaTags[j].nID
				&& pTag->IsStr()
				&& _tcschr(pTag->GetStr(), _T('?')) != NULL)
			{
				delete pTag;
				taglist.RemoveAt(i);
			}
			else
				i++;
		}
	}
}

CStringA GetED2KAudioCodec(WORD wFormatTag)
{
	CStringA strCodec(GetAudioFormatCodecId(wFormatTag));
	strCodec.Trim();
	strCodec.MakeLower();
	return strCodec;
}

CStringA GetED2KVideoCodec(DWORD biCompression)
{
	if (biCompression == BI_RGB)
		return "rgb";
	else if (biCompression == BI_RLE8)
		return "rle8";
	else if (biCompression == BI_RLE4)
		return "rle4";
	else if (biCompression == BI_BITFIELDS)
		return "bitfields";
	else if (biCompression == BI_JPEG)
		return "jpeg";
	else if (biCompression == BI_PNG)
		return "png";

	LPCSTR pszCompression = (LPCSTR)&biCompression;
	for (int i = 0; i < 4; i++)
	{
		if (   !__iscsym((unsigned char)pszCompression[i])
			&& pszCompression[i] != '.' 
			&& pszCompression[i] != ' ')
			return "";
	}

	CStringA strCodec;
	memcpy(strCodec.GetBuffer(4), &biCompression, 4);
	strCodec.ReleaseBuffer(4);
	strCodec.Trim();
	if (strCodec.GetLength() < 2)
		return "";
	strCodec.MakeLower();
	return strCodec;
}

SMediaInfo *GetRIFFMediaInfo(LPCTSTR pszFullPath)
{
	bool bIsAVI;
	SMediaInfo *mi = new SMediaInfo;
	if (!GetRIFFHeaders(pszFullPath, mi, bIsAVI)) {
		delete mi;
		return NULL;
	}
	return mi;
}

SMediaInfo *GetRMMediaInfo(LPCTSTR pszFullPath)
{
	bool bIsRM;
	SMediaInfo *mi = new SMediaInfo;
	if (!GetRMHeaders(pszFullPath, mi, bIsRM)) {
		delete mi;
		return NULL;
	}
	return mi;
}

SMediaInfo *GetWMMediaInfo(LPCTSTR pszFullPath)
{
#ifdef HAVE_WMSDK_H
	bool bIsWM;
	SMediaInfo *mi = new SMediaInfo;
	if (!GetWMHeaders(pszFullPath, mi, bIsWM)) {
		delete mi;
		return NULL;
	}
	return mi;
#else//HAVE_WMSDK_H
	UNREFERENCED_PARAMETER(pszFullPath);
	return NULL;
#endif//HAVE_WMSDK_H
}

// Max. string length which is used for string meta tags like TAG_MEDIA_TITLE, TAG_MEDIA_ARTIST, ...
#define	MAX_METADATA_STR_LEN	80

void TruncateED2KMetaData(CString& rstrData)
{
	rstrData.Trim();
	if (rstrData.GetLength() > MAX_METADATA_STR_LEN)
	{
		rstrData.Truncate(MAX_METADATA_STR_LEN);
		rstrData.Trim();
	}
}

void CKnownFile::UpdateMetaDataTags()
{
	// 05-Jän-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
	// the chance to clean any available meta data tags and provide only tags which were determined by us.
	RemoveMetaDataTags();

	if (thePrefs.GetExtractMetaData() == 0)
		return;

	TCHAR szExt[_MAX_EXT];
	_tsplitpath(GetFileName(), NULL, NULL, NULL, szExt);
	_tcslwr(szExt);
	if (_tcscmp(szExt, _T(".mp3"))==0 || _tcscmp(szExt, _T(".mp2"))==0 || _tcscmp(szExt, _T(".mp1"))==0 || _tcscmp(szExt, _T(".mpa"))==0)
	{
		TCHAR szFullPath[MAX_PATH];
		if (_tmakepathlimit(szFullPath, NULL, GetPath(), GetFileName(), NULL)){
			try{
				// ID3LIB BUG: If there are ID3v2 _and_ ID3v1 tags available, id3lib
				// destroys (actually corrupts) the Unicode strings from ID3v2 tags due to
				// converting Unicode to ASCII and then convertion back from ASCII to Unicode.
				// To prevent this, we force the reading of ID3v2 tags only, in case there are 
				// also ID3v1 tags available.
				ID3_Tag myTag;
				CStringA strFilePathA(szFullPath);
				size_t id3Size = myTag.Link(strFilePathA, ID3TT_ID3V2);
				if (id3Size == 0) {
					myTag.Clear();
					myTag.Link(strFilePathA, ID3TT_ID3V1);
				}

				const Mp3_Headerinfo* mp3info;
				mp3info = myTag.GetMp3HeaderInfo();
				if (mp3info)
				{
					// length
					if (mp3info->time){
						CTag* pTag = new CTag(FT_MEDIA_LENGTH, (uint32)mp3info->time);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					// here we could also create a "codec" ed2k meta tag.. though it would probable not be worth the
					// extra bytes which would have to be sent to the servers..

					// bitrate
					UINT uBitrate = (mp3info->vbr_bitrate ? mp3info->vbr_bitrate : mp3info->bitrate) / 1000;
					if (uBitrate){
						CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}
				}

				ID3_Tag::Iterator* iter = myTag.CreateIterator();
				const ID3_Frame* frame;
				while ((frame = iter->GetNext()) != NULL)
				{
					ID3_FrameID eFrameID = frame->GetID();
					switch (eFrameID)
					{
						case ID3FID_LEADARTIST:{
							wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
							CString strText(pszText);
							TruncateED2KMetaData(strText);
							if (!strText.IsEmpty()){
								CTag* pTag = new CTag(FT_MEDIA_ARTIST, strText);
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}
							delete[] pszText;
							break;
						}
						case ID3FID_ALBUM:{
							wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
							CString strText(pszText);
							TruncateED2KMetaData(strText);
							if (!strText.IsEmpty()){
								CTag* pTag = new CTag(FT_MEDIA_ALBUM, strText);
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}
							delete[] pszText;
							break;
						}
						case ID3FID_TITLE:{
							wchar_t* pszText = ID3_GetStringW(frame, ID3FN_TEXT);
							CString strText(pszText);
							TruncateED2KMetaData(strText);
							if (!strText.IsEmpty()){
								CTag* pTag = new CTag(FT_MEDIA_TITLE, strText);
								AddTagUnique(pTag);
								m_uMetaDataVer = META_DATA_VER;
							}
							delete[] pszText;
							break;
						}
					}
				}
				delete iter;
			}
			catch(...){
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (MP3) data from \"%s\""), szFullPath);
				ASSERT(0);
			}
		}
	}
	else
	{
		TCHAR szFullPath[MAX_PATH];
		if (_tmakepathlimit(szFullPath, NULL, GetPath(), GetFileName(), NULL))
		{
			SMediaInfo* mi = NULL;
			try
			{
				mi = GetRIFFMediaInfo(szFullPath);
				if (mi == NULL)
					mi = GetRMMediaInfo(szFullPath);
				if (mi == NULL)
					mi = GetWMMediaInfo(szFullPath);
				if (mi)
				{
					mi->InitFileLength();
					UINT uLengthSec = (UINT)mi->fFileLengthSec;

					CStringA strCodec;
					uint32 uBitrate = 0;
					if (mi->iVideoStreams) {
						strCodec = GetED2KVideoCodec(mi->video.bmiHeader.biCompression);
						uBitrate = (mi->video.dwBitRate + 500) / 1000;
					}
					else if (mi->iAudioStreams) {
						strCodec = GetED2KAudioCodec(mi->audio.wFormatTag);
						uBitrate = (DWORD)(((mi->audio.nAvgBytesPerSec * 8.0) + 500.0) / 1000.0);
					}

					if (uLengthSec) {
						CTag* pTag = new CTag(FT_MEDIA_LENGTH, (uint32)uLengthSec);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					if (!strCodec.IsEmpty()) {
						CTag* pTag = new CTag(FT_MEDIA_CODEC, CString(strCodec));
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					if (uBitrate) {
						CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					TruncateED2KMetaData(mi->strTitle);
					if (!mi->strTitle.IsEmpty()){
						CTag* pTag = new CTag(FT_MEDIA_TITLE, mi->strTitle);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					TruncateED2KMetaData(mi->strAuthor);
					if (!mi->strAuthor.IsEmpty()){
						CTag* pTag = new CTag(FT_MEDIA_ARTIST, mi->strAuthor);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					TruncateED2KMetaData(mi->strAlbum);
					if (!mi->strAlbum.IsEmpty()){
						CTag* pTag = new CTag(FT_MEDIA_ALBUM, mi->strAlbum);
						AddTagUnique(pTag);
						m_uMetaDataVer = META_DATA_VER;
					}

					delete mi;
					return;
				}
			}
			catch(...){
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (AVI) data from \"%s\""), szFullPath);
				ASSERT(0);
			}
			delete mi;
		}
	}
}

void CKnownFile::SetPublishedED2K(bool val){
	m_PublishedED2K = val;
	theApp.emuledlg->sharedfileswnd->sharedfilesctrl.UpdateFile(this);
}

bool CKnownFile::PublishNotes()
{
	if(m_lastPublishTimeKadNotes > (uint32)time(NULL))
	{
		return false;
	}
	if(GetFileComment() != _T(""))
	{
		m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
		return true;
	}
	if(GetFileRating() != 0)
	{
		m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
		return true;
	}

	return false;
}

bool CKnownFile::PublishSrc()
{
	uint32 lastBuddyIP = 0;
	if( theApp.IsFirewalled() && 
		(Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) || !Kademlia::CUDPFirewallTester::IsVerified()))
	{
		CUpDownClient* buddy = theApp.clientlist->GetBuddy();
		if( buddy )
		{
			lastBuddyIP = theApp.clientlist->GetBuddy()->GetIP();
			if( lastBuddyIP != m_lastBuddyIP )
			{
				SetLastPublishTimeKadSrc( (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES, lastBuddyIP );
				return true;
			}
		}
		else
			return false;
	}

	if(m_lastPublishTimeKadSrc > (uint32)time(NULL))
		return false;

	SetLastPublishTimeKadSrc((uint32)time(NULL)+KADEMLIAREPUBLISHTIMES,lastBuddyIP);
	return true;
}

bool CKnownFile::IsMovie() const
{
	return (ED2KFT_VIDEO == GetED2KFileTypeID(GetFileName()) );
}

// function assumes that this file is shared and that any needed permission to preview exists. checks have to be done before calling! 
bool CKnownFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
	return GrabImage(GetPath() + CString(_T("\\")) + GetFileName(), nFramesToGrab,  dStartTime, bReduceColor, nMaxWidth, pSender);
}

bool CKnownFile::GrabImage(CString strFileName,uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth, void* pSender)
{
	if (!IsMovie())
		return false;
	CFrameGrabThread* framegrabthread = (CFrameGrabThread*) AfxBeginThread(RUNTIME_CLASS(CFrameGrabThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
	framegrabthread->SetValues(this, strFileName, nFramesToGrab, dStartTime, bReduceColor, nMaxWidth, pSender);
	framegrabthread->ResumeThread();
	return true;
}

// imgResults[i] can be NULL
void CKnownFile::GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender)
{
	// continue processing
	if (pSender == theApp.mmserver){
		theApp.mmserver->PreviewFinished(imgResults, nFramesGrabbed);
	}
	else if (theApp.clientlist->IsValidClient((CUpDownClient*)pSender)){
		((CUpDownClient*)pSender)->SendPreviewAnswer(this, imgResults, nFramesGrabbed);
	}
	else{
		//probably a client which got deleted while grabbing the frames for some reason
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Couldn't find Sender of FrameGrabbing Request"));
	}
	//cleanup
	for (int i = 0; i != nFramesGrabbed; i++)
		delete imgResults[i];
	delete[] imgResults;
}

CString CKnownFile::GetInfoSummary(bool bNoFormatCommands) const
{
	CString strFolder = GetPath();
	PathRemoveBackslash(strFolder.GetBuffer());
	strFolder.ReleaseBuffer();

	CString strAccepts, strRequests, strTransferred;
    strRequests.Format(_T("%u (%u)"), statistic.GetRequests(), statistic.GetAllTimeRequests());
	strAccepts.Format(_T("%u (%u)"), statistic.GetAccepts(), statistic.GetAllTimeAccepts());
	strTransferred.Format(_T("%s (%s)"), CastItoXBytes(statistic.GetTransferred(), false, false), CastItoXBytes(statistic.GetAllTimeTransferred(), false, false));
	CString strType = GetFileTypeDisplayStr();
	if (strType.IsEmpty())
		strType = _T("-");
	CString dbgInfo;
#ifdef _DEBUG
	dbgInfo.Format(_T("\nAICH Part HashSet: %s\nAICH Rec HashSet: %s"), m_FileIdentifier.HasExpectedAICHHashCount() ? _T("Yes") : _T("No")
		, IsAICHRecoverHashSetAvailable() ? _T("Yes") : _T("No"));
#endif

	CString strHeadFormatCommand = bNoFormatCommands ? _T("") : _T("<br_head>");
	CString info;
	info.Format(_T("%s\n")
		+ CString(_T("eD2K ")) + GetResString(IDS_FD_HASH) + _T(" %s\n")
		+ GetResString(IDS_AICHHASH) + _T(": %s\n")
		+ GetResString(IDS_FD_SIZE) + _T(" %s\n") + strHeadFormatCommand + _T("\n")
		+ GetResString(IDS_TYPE) + _T(": %s\n")
		+ GetResString(IDS_FOLDER) + _T(": %s\n\n")
		+ GetResString(IDS_PRIORITY) + _T(": %s\n")
		+ GetResString(IDS_SF_REQUESTS) + _T(": %s\n")
		+ GetResString(IDS_SF_ACCEPTS) + _T(": %s\n")
		+ GetResString(IDS_SF_TRANSFERRED) + _T(": %s%s"),
		GetFileName(),
		md4str(GetFileHash()),
		m_FileIdentifier.GetAICHHash().GetString(),
		CastItoXBytes(GetFileSize(), false, false),
		strType,
		strFolder,
		GetUpPriorityDisplayString(),
		strRequests,
		strAccepts,
		strTransferred,
		dbgInfo);
	return info;
}

CString CKnownFile::GetUpPriorityDisplayString() const {
	switch (GetUpPriority()) {
		case PR_VERYLOW :
			return GetResString(IDS_PRIOVERYLOW);
		case PR_LOW :
			if (IsAutoUpPriority())
				return GetResString(IDS_PRIOAUTOLOW);
			else
				return GetResString(IDS_PRIOLOW);
		case PR_NORMAL :
			if (IsAutoUpPriority())
				return GetResString(IDS_PRIOAUTONORMAL);
			else
				return GetResString(IDS_PRIONORMAL);
		case PR_HIGH :
			if (IsAutoUpPriority())
				return GetResString(IDS_PRIOAUTOHIGH);
			else
				return GetResString(IDS_PRIOHIGH);
		case PR_VERYHIGH :
			return GetResString(IDS_PRIORELEASE);
		default:
			return _T("");
	}
}

bool CKnownFile::ShouldPartiallyPurgeFile() const
{
	return thePrefs.DoPartiallyPurgeOldKnownFiles() && m_timeLastSeen > 0
		&& time(NULL) > m_timeLastSeen && time(NULL) - m_timeLastSeen > OLDFILES_PARTIALLYPURGE;
}