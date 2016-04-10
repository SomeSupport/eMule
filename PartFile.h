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
#include "KnownFile.h"
#include "DeadSourceList.h"
#include "CorruptionBlackBox.h"

enum EPartFileStatus{
	PS_READY			= 0,
	PS_EMPTY			= 1,
	PS_WAITINGFORHASH	= 2,
	PS_HASHING			= 3,
	PS_ERROR			= 4,
	PS_INSUFFICIENT		= 5,
	PS_UNKNOWN			= 6,
	PS_PAUSED			= 7,
	PS_COMPLETING		= 8,
	PS_COMPLETE			= 9
};

#define PR_VERYLOW			4 // I Had to change this because it didn't save negative number correctly.. Had to modify the sort function for this change..
#define PR_LOW				0 //*
#define PR_NORMAL			1 // Don't change this - needed for edonkey clients and server!
#define PR_HIGH				2 //*
#define PR_VERYHIGH			3
#define PR_AUTO				5 //UAP Hunter

//#define BUFFER_SIZE_LIMIT 500000 // Max bytes before forcing a flush

#define	PARTMET_BAK_EXT	_T(".bak")
#define	PARTMET_TMP_EXT	_T(".backup")

#define STATES_COUNT		17

enum EPartFileFormat{
	PMT_UNKNOWN			= 0,
	PMT_DEFAULTOLD,
	PMT_SPLITTED,
	PMT_NEWOLD,
	PMT_SHAREAZA,
	PMT_BADFORMAT	
};

enum EPartFileLoadResult{
	PLR_LOADSUCCESS = 1,
	PLR_CHECKSUCCESS = 2,
	PLR_FAILED_METFILE_CORRUPT = -1,
	PLR_FAILED_METFILE_NOACCESS = -2,
	PLR_FAILED_OTHER   = 0
};

#define	FILE_COMPLETION_THREAD_FAILED	0x0000
#define	FILE_COMPLETION_THREAD_SUCCESS	0x0001
#define	FILE_COMPLETION_THREAD_RENAMED	0x0002

enum EPartFileOp{
	PFOP_NONE = 0,
	PFOP_HASHING,
	PFOP_COPYING,
	PFOP_UNCOMPRESSING
};

class CSearchFile;
class CUpDownClient;
enum EDownloadState;
class CxImage;
class CSafeMemFile;

#pragma pack(1)
struct Requested_Block_Struct
{
	uint64	StartOffset;
	uint64	EndOffset;
	uchar	FileID[16];
	uint64  transferred; // Barry - This counts bytes completed
};
#pragma pack()

struct Gap_Struct
{
	uint64 start;
	uint64 end;
};

struct PartFileBufferedData
{
	BYTE *data;						// Barry - This is the data to be written
	uint64 start;					// Barry - This is the start offset of the data
	uint64 end;						// Barry - This is the end offset of the data
	Requested_Block_Struct *block;	// Barry - This is the requested block that this data relates to
};

typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CPartFile : public CKnownFile
{
	DECLARE_DYNAMIC(CPartFile)

	friend class CPartFileConvert;
public:
	CPartFile(UINT cat = 0);
	CPartFile(CSearchFile* searchresult, UINT cat = 0);
	CPartFile(CString edonkeylink, UINT cat = 0);
	CPartFile(class CED2KFileLink* fileLink, UINT cat = 0);
	virtual ~CPartFile();

	bool	IsPartFile() const { return !(status == PS_COMPLETE); }

	// eD2K filename
	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false, bool bRemoveControlChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	// part.met filename (without path!)
	const CString& GetPartMetFileName() const { return m_partmetfilename; }

	// full path to part.met file or completed file
	const CString& GetFullName() const { return m_fullname; }
	void	SetFullName(CString name) { m_fullname = name; }
	CString	GetTempPath() const;

	// local file system related properties
	bool	IsNormalFile() const { return (m_dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)) == 0; }
	const bool	IsAllocating() const { return m_AllocateThread != NULL; }
	EMFileSize	GetRealFileSize() const;
	void	GetLeftToTransferAndAdditionalNeededSpace(uint64 &ui64LeftToTransfer, uint64 &pui32AdditionalNeededSpace) const;
	uint64	GetNeededSpace() const;
	virtual void SetFileSize(EMFileSize nFileSize);

	// last file modification time (NT's version of UTC), to be used for stats only!
	CTime	GetCFileDate() const { return CTime(m_tLastModified); }
	uint32	GetFileDate() const { return m_tLastModified; }

	// file creation time (NT's version of UTC), to be used for stats only!
	CTime	GetCrCFileDate() const { return CTime(m_tCreated); }
	uint32	GetCrFileDate() const { return m_tCreated; }

	void	InitializeFromLink(CED2KFileLink* fileLink, UINT cat = 0);
	uint32	Process(uint32 reducedownload, UINT icounter);
	EPartFileLoadResult	LoadPartFile(LPCTSTR in_directory, LPCTSTR filename, EPartFileFormat* pOutCheckFileFormat = NULL); //filename = *.part.met
	EPartFileLoadResult	ImportShareazaTempfile(LPCTSTR in_directory,LPCTSTR in_filename, EPartFileFormat* pOutCheckFileFormat = NULL);

	bool	SavePartFile(bool bDontOverrideBak = false);
	void	PartFileHashFinished(CKnownFile* result);
	bool	HashSinglePart(UINT partnumber, bool* pbAICHReportedOK = NULL); // true = ok , false = corrupted

	void	AddGap(uint64 start, uint64 end);
	void	FillGap(uint64 start, uint64 end);
	void	DrawStatusBar(CDC* dc, LPCRECT rect, bool bFlat) /*const*/;
	virtual void	DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool	 bFlat) const;
	bool	IsComplete(uint64 start, uint64 end, bool bIgnoreBufferedData) const;
	bool	IsPureGap(uint64 start, uint64 end) const;
	bool	IsAlreadyRequested(uint64 start, uint64 end, bool bCheckBuffers = false) const;
    bool    ShrinkToAvoidAlreadyRequested(uint64& start, uint64& end) const;
	bool	IsCorruptedPart(UINT partnumber) const;
	uint64	GetTotalGapSizeInRange(uint64 uRangeStart, uint64 uRangeEnd) const;
	uint64	GetTotalGapSizeInPart(UINT uPart) const;
	void	UpdateCompletedInfos();
	void	UpdateCompletedInfos(uint64 uTotalGaps);
	virtual void	UpdatePartsInfo();

	bool	GetNextRequestedBlock(CUpDownClient* sender, Requested_Block_Struct** newblocks, uint16* count) /*const*/;
	void	WritePartStatus(CSafeMemFile* file) const;
	void	WriteCompleteSourcesCount(CSafeMemFile* file) const;
	void	AddSources(CSafeMemFile* sources,uint32 serverip, uint16 serverport, bool bWithObfuscationAndHash);
	void	AddSource(LPCTSTR pszURL, uint32 nIP);
	static bool CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, UINT* pdebug_lowiddropped = NULL, bool Ed2kID = true);
	
	EPartFileStatus	GetStatus(bool ignorepause = false) const;
	void	SetStatus(EPartFileStatus eStatus);		// set status and update GUI
	void	_SetStatus(EPartFileStatus eStatus);	// set status and do *not* update GUI
	void	NotifyStatusChange();
	bool	IsStopped() const												{ return stopped; }
	bool	GetCompletionError() const										{ return m_bCompletionError; }
	EMFileSize  GetCompletedSize() const									{ return completedsize; }
	CString getPartfileStatus() const;
	int		getPartfileStatusRang() const;
	void	SetActive(bool bActive);

	uint8	GetDownPriority() const											{ return m_iDownPriority; }
	void	SetDownPriority(uint8 iNewDownPriority, bool resort = true);
	bool	IsAutoDownPriority(void) const									{ return m_bAutoDownPriority; }
	void	SetAutoDownPriority(bool NewAutoDownPriority)					{ m_bAutoDownPriority = NewAutoDownPriority; }
	void	UpdateAutoDownPriority();

	UINT	GetSourceCount() const											{ return srclist.GetCount(); }
	UINT	GetSrcA4AFCount() const											{ return A4AFsrclist.GetCount(); }
	UINT	GetSrcStatisticsValue(EDownloadState nDLState) const;
	UINT	GetTransferringSrcCount() const;
	uint64	GetTransferred() const											{ return m_uTransferred; }
	uint32	GetDatarate() const												{ return datarate; }
	float	GetPercentCompleted() const										{ return percentcompleted; }
	UINT	GetNotCurrentSourcesCount() const;
	int		GetValidSourcesCount() const;
	bool	IsArchive(bool onlyPreviewable = false) const; // Barry - Also want to preview archives
    bool    IsPreviewableFileType() const;
	time_t	getTimeRemaining() const;
	time_t	getTimeRemainingSimple() const;
	uint32	GetDlActiveTime() const;

	// Barry - Added as replacement for BlockReceived to buffer data before writing to disk
	uint32	WriteToBuffer(uint64 transize, const BYTE *data, uint64 start, uint64 end, Requested_Block_Struct *block, const CUpDownClient* client);
	void	FlushBuffer(bool forcewait=false, bool bForceICH = false, bool bNoAICH = false);
	// Barry - This will invert the gap list, up to caller to delete gaps when done
	// 'Gaps' returned are really the filled areas, and guaranteed to be in order
	void	GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled) const;

	// Barry - Added to prevent list containing deleted blocks on shutdown
	void	RemoveAllRequestedBlocks(void);
	bool	RemoveBlockFromList(uint64 start, uint64 end);
	bool	IsInRequestedBlockList(const Requested_Block_Struct* block) const;
	void	RemoveAllSources(bool bTryToSwap);

	bool	CanOpenFile() const;
	bool	IsReadyForPreview() const;
	bool	CanStopFile() const;
	bool	CanPauseFile() const;
	bool	CanResumeFile() const;
	bool	IsPausingOnPreview() const										{ return m_bPauseOnPreview && IsPreviewableFileType() && CanPauseFile(); }

	void	OpenFile() const;
	void	PreviewFile();
	void	DeleteFile();
	void	StopFile(bool bCancel = false, bool resort = true);
	void	PauseFile(bool bInsufficient = false, bool resort = true);
	void	StopPausedFile();
	void	ResumeFile(bool resort = true);
	void	ResumeFileInsufficient();
	void	SetPauseOnPreview(bool bVal)									{ m_bPauseOnPreview = bVal; }

	virtual Packet* CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions) const;
	void	AddClientSources(CSafeMemFile* sources, uint8 sourceexchangeversion, bool bSourceExchange2, const CUpDownClient* pClient = NULL);

	UINT	GetAvailablePartCount() const									{ return availablePartsCount; }
	void	UpdateAvailablePartsCount();

	uint32	GetLastAnsweredTime() const										{ return m_ClientSrcAnswered; }
	void	SetLastAnsweredTime() { m_ClientSrcAnswered = ::GetTickCount(); }
	void	SetLastAnsweredTimeTimeout();

	uint64	GetCorruptionLoss() const { return m_uCorruptionLoss; }
	uint64	GetCompressionGain() const { return m_uCompressionGain; }
	uint32	GetRecoveredPartsByICH() const { return m_uPartsSavedDueICH; }

	virtual void	UpdateFileRatingCommentAvail(bool bForceUpdate = false);
	virtual void	RefilterFileComments();

	void	AddDownloadingSource(CUpDownClient* client);
	void	RemoveDownloadingSource(CUpDownClient* client);

	CString GetProgressString(uint16 size) const;
	CString GetInfoSummary(bool bNoFormatCommands = false) const;

//	int		GetCommonFilePenalty() const;
	void	UpdateDisplayedInfo(bool force = false);

	UINT	GetCategory() /*const*/;
	void	SetCategory(UINT cat);
	bool	HasDefaultCategory() const;
	bool	CheckShowItemInGivenCat(int inCategory) /*const*/;

	uint8*	MMCreatePartStatus();

	//preview
	virtual bool GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth,void* pSender);
	virtual void GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender);

	void	FlushBuffersExceptionHandler(CFileException* error);
	void	FlushBuffersExceptionHandler();

	void	PerformFileCompleteEnd(DWORD dwResult);

	void	SetFileOp(EPartFileOp eFileOp);
	EPartFileOp GetFileOp() const											{ return m_eFileOp; }
	void	SetFileOpProgress(UINT uProgress);
	UINT	GetFileOpProgress() const										{ return m_uFileOpProgress; }

	CAICHRecoveryHashSet* GetAICHRecoveryHashSet()							{ return m_pAICHRecoveryHashSet; }
	void	RequestAICHRecovery(UINT nPart);
	void	AICHRecoveryDataAvailable(UINT nPart);
	bool	IsAICHPartHashSetNeeded() const									{ return m_FileIdentifier.HasAICHHash() && !m_FileIdentifier.HasExpectedAICHHashCount() && m_bAICHPartHashsetNeeded; }
	void	SetAICHHashSetNeeded(bool bVal)									{ m_bAICHPartHashsetNeeded = bVal; }

	uint32	m_LastSearchTime;
	uint32	m_LastSearchTimeKad;
	uint64	m_iAllocinfo;
	CUpDownClientPtrList srclist;
	CUpDownClientPtrList A4AFsrclist; //<<-- enkeyDEV(Ottavio84) -A4AF-
	CTime	lastseencomplete;
	CFile	m_hpartfile;				// permanent opened handle to avoid write conflicts
	CMutex 	m_FileCompleteMutex;		// Lord KiRon - Mutex for file completion
	uint16	src_stats[4];
	uint16  net_stats[3];
	volatile bool m_bPreviewing;
	volatile bool m_bRecoveringArchive; // Is archive recovery in progress
	bool	m_bLocalSrcReqQueued;
	bool	srcarevisible;				// used for downloadlistctrl
	bool	m_bMD4HashsetNeeded;
	uint8	m_TotalSearchesKad;
    bool    AllowSwapForSourceExchange()					{ return ::GetTickCount()-lastSwapForSourceExchangeTick > 30*1000; } // ZZ:DownloadManager
    void    SetSwapForSourceExchangeTick()					{ lastSwapForSourceExchangeTick = ::GetTickCount(); } // ZZ:DownloadManager
	
	UINT	SetPrivateMaxSources(uint32 in)					{ return m_uMaxSources = in; } 
	UINT	GetPrivateMaxSources() const					{ return m_uMaxSources; } 
	UINT	GetMaxSources() const;
	UINT	GetMaxSourcePerFileSoft() const;
	UINT	GetMaxSourcePerFileUDP() const;

    bool    GetPreviewPrio() const							{ return m_bpreviewprio; }
	void    SetPreviewPrio(bool in)							{ m_bpreviewprio=in; }

    static bool RightFileHasHigherPrio(CPartFile* left, CPartFile* right);

	CDeadSourceList	m_DeadSourceList;

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	bool	GetNextEmptyBlockInPart(UINT partnumber, Requested_Block_Struct* result) const;
	void	CompleteFile(bool hashingdone);
	void	CreatePartFile(UINT cat = 0);
	void	Init();

private:
	BOOL 		PerformFileComplete(); // Lord KiRon
	static UINT CompleteThreadProc(LPVOID pvParams); // Lord KiRon - Used as separate thread to complete file
	static UINT AFX_CDECL AllocateSpaceThread(LPVOID lpParam);
	void		CharFillRange(CString* buffer,uint32 start, uint32 end, char color) const;

	CCorruptionBlackBox	m_CorruptionBlackBox;
	static CBarShader s_LoadBar;
	static CBarShader s_ChunkBar;
	uint32	m_iLastPausePurge;
	uint16	count;
	UINT	m_anStates[STATES_COUNT];
	EMFileSize	completedsize;
	uint64	m_uCorruptionLoss;
	uint64	m_uCompressionGain;
	uint32	m_uPartsSavedDueICH;
	uint32	datarate;
	CString m_fullname;
	CString m_partmetfilename;
	uint64	m_uTransferred;
	UINT	m_uMaxSources;
	bool	paused;
	bool	m_bPauseOnPreview;
	bool	stopped;
	bool	insufficient;
	bool	m_bCompletionError;
	bool	m_bAICHPartHashsetNeeded;
	uint8	m_iDownPriority;
	bool	m_bAutoDownPriority;
	EPartFileStatus	status;
	bool	newdate;	// indicates if there was a writeaccess to the .part file
	uint32	lastpurgetime;
	uint32	m_LastNoNeededCheck;
	CTypedPtrList<CPtrList, Gap_Struct*> gaplist;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> requestedblocks_list;
	CArray<uint16,uint16> m_SrcpartFrequency;
	float	percentcompleted;
	CList<uint16, uint16> corrupted_list;
	uint32	m_ClientSrcAnswered;
	UINT	availablePartsCount;
	CWinThread* m_AllocateThread;
	DWORD	m_lastRefreshedDLDisplay;
	CUpDownClientPtrList m_downloadingSourceList;
	bool	m_bDeleteAfterAlloc;
    bool	m_bpreviewprio;
	// Barry - Buffered data to be written
	CTypedPtrList<CPtrList, PartFileBufferedData*> m_BufferedData_list;
	uint64	m_nTotalBufferData;
	uint32	m_nLastBufferFlushTime;
	UINT	m_category;
	DWORD	m_dwFileAttributes;
	time_t	m_tActivated;
	uint32	m_nDlActiveTime;
	uint32	m_tLastModified;	// last file modification time (NT's version of UTC), to be used for stats only!
	uint32	m_tCreated;			// file creation time (NT's version of UTC), to be used for stats only!
    uint32	m_random_update_wait;	
	volatile EPartFileOp m_eFileOp;
	volatile UINT m_uFileOpProgress;
	CAICHRecoveryHashSet*	m_pAICHRecoveryHashSet;

    DWORD   lastSwapForSourceExchangeTick; // ZZ:DownloadManaager


};
