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
#include "ArchiveRecovery.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include <zlib/zlib.h>
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#pragma pack(push,1)
typedef struct {
	BYTE	type;
	WORD	flags;
	WORD	size;
} RARMAINHDR;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
	BYTE	type;
	WORD	flags;
	WORD	size;
	DWORD	packetSize;
	DWORD	unpacketSize;
	BYTE	hostOS;
	DWORD	fileCRC;
	DWORD	fileTime;
	BYTE	unpVer;
	BYTE	method;
	WORD	nameSize;
	DWORD	fileAttr;
} RARFILEHDR;
#pragma pack(pop)


void CArchiveRecovery::recover(CPartFile *partFile, bool preview, bool bCreatePartFileCopy)
{
	if (partFile->m_bPreviewing || partFile->m_bRecoveringArchive)
		return;
	partFile->m_bRecoveringArchive = true;

	AddLogLine(true, _T("%s \"%s\""), GetResString(IDS_ATTEMPTING_RECOVERY), partFile->GetFileName());

	// Get the current filled list for this file
	CTypedPtrList<CPtrList, Gap_Struct*> *filled = new CTypedPtrList<CPtrList, Gap_Struct*>;
	partFile->GetFilledList(filled);
#ifdef _DEBUG
	{
		int i = 0;
		TRACE("%s: filled\n", __FUNCTION__);
		POSITION pos = filled->GetHeadPosition();
		while (pos){
			Gap_Struct* gap = filled->GetNext(pos);
			TRACE("%3u: %10u  %10u  (%u)\n", i++, gap->start, gap->end, gap->end - gap->start + 1);
		}
	}
#endif

	// The rest of the work can be safely done in a new thread
	ThreadParam *tp = new ThreadParam;
	tp->partFile = partFile;
	tp->filled = filled;
	tp->preview = preview;
	tp->bCreatePartFileCopy = bCreatePartFileCopy;
	// - do NOT use Windows API 'CreateThread' to create a thread which uses MFC/CRT -> lot of mem leaks!
	if (!AfxBeginThread(run, (LPVOID)tp)){
		partFile->m_bRecoveringArchive = false;
		LogError(LOG_STATUSBAR, _T("%s \"%s\""), GetResString(IDS_RECOVERY_FAILED), partFile->GetFileName());
		// Need to delete the memory here as won't be done in thread
		DeleteMemory(tp);
	}
}

UINT AFX_CDECL CArchiveRecovery::run(LPVOID lpParam)
{
	ThreadParam *tp = (ThreadParam *)lpParam;
	DbgSetThreadName("ArchiveRecovery");
	InitThreadLocale();

	if (!performRecovery(tp->partFile, tp->filled, tp->preview, tp->bCreatePartFileCopy))
		theApp.QueueLogLine(true, _T("%s \"%s\""), GetResString(IDS_RECOVERY_FAILED), tp->partFile->GetFileName());

	tp->partFile->m_bRecoveringArchive = false;

	// Delete memory used by copied gap list
	DeleteMemory(tp);

	return 0;
}

bool CArchiveRecovery::performRecovery(CPartFile *partFile, CTypedPtrList<CPtrList, Gap_Struct*> *filled, 
									   bool preview, bool bCreatePartFileCopy)
{
	if (filled->GetCount()==0)
		return false;

	bool success = false;
	try
	{
		CFile temp;
		CString tempFileName;
		if (bCreatePartFileCopy)
		{
			// Copy the file
			tempFileName = partFile->GetTempPath() + partFile->GetFileName().Mid(0, 5) + _T("-rec.tmp");
			if (!CopyFile(partFile, filled, tempFileName))
				return false;

			// Open temp file for reading
			if (!temp.Open(tempFileName, CFile::modeRead|CFile::shareDenyWrite))
				return false;
		}
		else
		{
			if (!temp.Open(partFile->GetFilePath(), CFile::modeRead | CFile::shareDenyNone))
				return false;
		}

		// Open the output file
		EFileType myAtype=GetFileTypeEx(partFile);
		CString ext;
		if (myAtype==ARCHIVE_ZIP)
			ext=_T(".zip");
		else if (myAtype==ARCHIVE_RAR)
			ext=_T(".rar");
		else if (myAtype==ARCHIVE_ACE)
			ext=_T(".ace");

		CString outputFileName = partFile->GetTempPath()  + partFile->GetFileName().Mid(0, 5) + _T("-rec") + ext;
		CFile output;
		ULONGLONG ulTempFileSize = 0;
		if (output.Open(outputFileName, CFile::modeWrite | CFile::shareDenyWrite | CFile::modeCreate))
		{
			// Process the output file
			if ( myAtype==ARCHIVE_ZIP)
				success = recoverZip(&temp, &output, NULL, filled, (temp.GetLength() == partFile->GetFileSize()));
			else if(myAtype==ARCHIVE_RAR)
				success = recoverRar(&temp, &output, NULL, filled);
			else if(myAtype==ARCHIVE_ACE)
				success = recoverAce(&temp, &output, NULL, filled);

			ulTempFileSize = output.GetLength();

			// Close output
			output.Close();
		}
		// Close temp file
		temp.Close();

		// Remove temp file
		if (!tempFileName.IsEmpty() )
			CFile::Remove(tempFileName);

		// Report success
		if (success)
		{
			theApp.QueueLogLine(true, _T("%s \"%s\""), GetResString(IDS_RECOVERY_SUCCESSFUL), partFile->GetFileName());
			theApp.QueueDebugLogLine(false, _T("Archive recovery: Part file size: %s, temp. archive file size: %s (%.1f%%)"), CastItoXBytes(partFile->GetFileSize()), CastItoXBytes(ulTempFileSize), partFile->GetFileSize() > (uint64)0 ? (ulTempFileSize * 100.0 / (uint64)partFile->GetFileSize()) : 0.0);

			// Preview file if required
			if (preview)
			{
				SHELLEXECUTEINFO SE;
				memset(&SE,0,sizeof(SE));
				SE.fMask = SEE_MASK_NOCLOSEPROCESS ;
				SE.lpVerb = _T("open");
				SE.lpFile = outputFileName;
				SE.nShow = SW_SHOW;
				SE.cbSize = sizeof(SE);
				ShellExecuteEx(&SE);
				if (SE.hProcess)
				{
					WaitForSingleObject(SE.hProcess, INFINITE);
					CloseHandle(SE.hProcess);
				}
				CFile::Remove(outputFileName);
			}
		} else
			CFile::Remove(outputFileName);
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}
	return success;
}

bool CArchiveRecovery::recoverZip(CFile *zipInput, CFile *zipOutput, archiveScannerThreadParams_s* aitp ,
								  CTypedPtrList<CPtrList, Gap_Struct*> *filled, bool fullSize)
{
	bool retVal = false;
	long fileCount = 0;
	try
	{
		CTypedPtrList<CPtrList, ZIP_CentralDirectory*>* centralDirectoryEntries;
		if (aitp==NULL) {
			centralDirectoryEntries= new CTypedPtrList<CPtrList, ZIP_CentralDirectory*>;
		} else {
			centralDirectoryEntries=aitp->ai->centralDirectoryEntries;
		}

		Gap_Struct *fill;

		// If the central directory is intact this is simple
		if (fullSize && readZipCentralDirectory(zipInput, centralDirectoryEntries, filled))
		{
			if (centralDirectoryEntries->GetCount() == 0)
			{
				if (aitp == NULL)
					delete centralDirectoryEntries;
				return false;
			}
			
			// if only read directory, return now
			if (zipOutput==NULL) {
				if (aitp == NULL) {
					ASSERT(0); // FIXME
					delete centralDirectoryEntries;
					return false;
				}
				aitp->ai->bZipCentralDir = true;
				return true;
			}

			ZIP_CentralDirectory *cdEntry;
			POSITION pos = centralDirectoryEntries->GetHeadPosition();
			bool deleteCD;
			for (int i=centralDirectoryEntries->GetCount(); i>0; i--)
			{
				deleteCD = false;
				cdEntry = centralDirectoryEntries->GetAt(pos);
				uint32 lenEntry = sizeof(ZIP_Entry) + cdEntry->lenFilename + cdEntry->lenExtraField + cdEntry->lenCompressed;
				if (IsFilled(cdEntry->relativeOffsetOfLocalHeader, cdEntry->relativeOffsetOfLocalHeader + lenEntry, filled))
				{
					zipInput->Seek(cdEntry->relativeOffsetOfLocalHeader, CFile::begin);
					// Update offset
					cdEntry->relativeOffsetOfLocalHeader = (UINT)zipOutput->GetPosition();
					if (!processZipEntry(zipInput, zipOutput, lenEntry, NULL))
						deleteCD = true;
				}
				else
					deleteCD = true;

				if (deleteCD)
				{
					delete [] cdEntry->filename;
					if (cdEntry->lenExtraField > 0)
						delete [] cdEntry->extraField;
					if (cdEntry->lenComment > 0)
						delete [] cdEntry->comment;
					delete cdEntry;
					POSITION del = pos;
					centralDirectoryEntries->GetNext(pos);
					centralDirectoryEntries->RemoveAt(del);
				}
				else
					centralDirectoryEntries->GetNext(pos);
			}
		}
		else // Have to scan the file the hard way
		{		
			// Loop through filled areas of the file looking for entries
			POSITION pos = filled->GetHeadPosition();
			while (pos != NULL)
			{
				fill = filled->GetNext(pos);
				uint32 filePos = (UINT)zipInput->GetPosition();
				// The file may have been positioned to the next entry in ScanForMarker() or processZipEntry()
				if (filePos > fill->end)
					continue;
				if (filePos < fill->start)
					zipInput->Seek(fill->start, CFile::begin);

				// If there is any problem, then don't bother checking the rest of this part
				for (;;)
				{
					if (aitp && !aitp->m_bIsValid)
						return 0;
					// Scan for entry marker within this filled area
					if (!scanForZipMarker(zipInput, aitp, (uint32)ZIP_LOCAL_HEADER_MAGIC, (UINT)(fill->end - zipInput->GetPosition() + 1)))
						break;
					if (zipInput->GetPosition() > fill->end)
						break;
					if (!processZipEntry(zipInput, zipOutput, (UINT)(fill->end - zipInput->GetPosition() + 1), centralDirectoryEntries))
						break;
				}
			}
			if (!zipOutput)
			{
				retVal = (centralDirectoryEntries->GetCount()>0 );
				if (aitp == NULL)
					delete centralDirectoryEntries;
				return retVal;
			}
		}

		// Remember offset before CD entries
		uint32 startOffset = (UINT)zipOutput->GetPosition();

		// Write all central directory entries
		fileCount = centralDirectoryEntries->GetCount();
		if (fileCount > 0)
		{
			ZIP_CentralDirectory *cdEntry;
			POSITION pos = centralDirectoryEntries->GetHeadPosition();
			while (pos != NULL)
			{
				cdEntry = centralDirectoryEntries->GetNext(pos);

				writeUInt32(zipOutput, ZIP_CD_MAGIC);
				writeUInt16(zipOutput, cdEntry->versionMadeBy);
				writeUInt16(zipOutput, cdEntry->versionToExtract);
				writeUInt16(zipOutput, cdEntry->generalPurposeFlag);
				writeUInt16(zipOutput, cdEntry->compressionMethod);
				writeUInt16(zipOutput, cdEntry->lastModFileTime);
				writeUInt16(zipOutput, cdEntry->lastModFileDate);
				writeUInt32(zipOutput, cdEntry->crc32);
				writeUInt32(zipOutput, cdEntry->lenCompressed);
				writeUInt32(zipOutput, cdEntry->lenUncompressed);
				writeUInt16(zipOutput, cdEntry->lenFilename);
				writeUInt16(zipOutput, cdEntry->lenExtraField);
				writeUInt16(zipOutput, cdEntry->lenComment);
				writeUInt16(zipOutput, 0); // Disk number start
				writeUInt16(zipOutput, cdEntry->internalFileAttributes);
				writeUInt32(zipOutput, cdEntry->externalFileAttributes);
				writeUInt32(zipOutput, cdEntry->relativeOffsetOfLocalHeader);
				zipOutput->Write(cdEntry->filename, cdEntry->lenFilename);
				if (cdEntry->lenExtraField > 0)
					zipOutput->Write(cdEntry->extraField, cdEntry->lenExtraField);
				if (cdEntry->lenComment > 0)
					zipOutput->Write(cdEntry->comment, cdEntry->lenComment);

				delete [] cdEntry->filename;
				if (cdEntry->lenExtraField > 0)
					delete [] cdEntry->extraField;
				if (cdEntry->lenComment > 0)
					delete [] cdEntry->comment;
				delete cdEntry;
			}

			// Remember offset before CD entries
			uint32 endOffset = (UINT)zipOutput->GetPosition();

			// Write end of central directory
			writeUInt32(zipOutput, ZIP_END_CD_MAGIC);
			writeUInt16(zipOutput, 0); // Number of this disk
			writeUInt16(zipOutput, 0); // Number of the disk with the start of the central directory
			writeUInt16(zipOutput, (uint16)fileCount);
			writeUInt16(zipOutput, (uint16)fileCount);
			writeUInt32(zipOutput, endOffset - startOffset);
			writeUInt32(zipOutput, startOffset);
			writeUInt16(zipOutput, (uint16)strlen(ZIP_COMMENT));
			zipOutput->Write(ZIP_COMMENT, strlen(ZIP_COMMENT));

			centralDirectoryEntries->RemoveAll();
		}
        if (aitp == NULL)
			delete centralDirectoryEntries;
		retVal = true;
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}

	// Tell the user how many files were recovered
	CString msg;	
	if (fileCount == 1)
		msg = GetResString(IDS_RECOVER_SINGLE);
	else
		msg.Format(GetResString(IDS_RECOVER_MULTIPLE), fileCount);
	theApp.QueueLogLine(true, _T("%s"), msg);

	return retVal;
}

bool CArchiveRecovery::readZipCentralDirectory(CFile *zipInput, CTypedPtrList<CPtrList, ZIP_CentralDirectory*> *centralDirectoryEntries, CTypedPtrList<CPtrList, Gap_Struct*> *filled)
{
	bool retVal = false;
	try
	{
		// Ideally this zip file will not have a comment and the End-CD will be easy to find
		zipInput->Seek(-22, CFile::end);
		if (!(readUInt32(zipInput) == ZIP_END_CD_MAGIC))
		{
			// Have to look for it, comment could be up to 65535 chars but only try with less than 1k
			zipInput->Seek(-1046, CFile::end);
			if (!scanForZipMarker(zipInput, NULL, (uint32)ZIP_END_CD_MAGIC, 1046))
				return false;
			// Skip it again
			readUInt32(zipInput);
		}

		// Found End-CD
		// Only interested in offset of first CD
		zipInput->Seek(12, CFile::current);
		uint32 startOffset = readUInt32(zipInput);
		if (!IsFilled(startOffset, (UINT)zipInput->GetLength(), filled))
			return false;

		// Goto first CD and start reading
		zipInput->Seek(startOffset, CFile::begin);
		ZIP_CentralDirectory *cdEntry;
		while (readUInt32(zipInput) == ZIP_CD_MAGIC)
		{
			cdEntry = new ZIP_CentralDirectory;
			cdEntry->versionMadeBy				= readUInt16(zipInput);
			cdEntry->versionToExtract			= readUInt16(zipInput);
			cdEntry->generalPurposeFlag			= readUInt16(zipInput);
			cdEntry->compressionMethod			= readUInt16(zipInput);
			cdEntry->lastModFileTime			= readUInt16(zipInput);
			cdEntry->lastModFileDate			= readUInt16(zipInput);
			cdEntry->crc32						= readUInt32(zipInput);
			cdEntry->lenCompressed				= readUInt32(zipInput);
			cdEntry->lenUncompressed			= readUInt32(zipInput);
			cdEntry->lenFilename				= readUInt16(zipInput);
			cdEntry->lenExtraField				= readUInt16(zipInput);
			cdEntry->lenComment					= readUInt16(zipInput);
			cdEntry->diskNumberStart			= readUInt16(zipInput);
			cdEntry->internalFileAttributes		= readUInt16(zipInput);
			cdEntry->externalFileAttributes		= readUInt32(zipInput);
			cdEntry->relativeOffsetOfLocalHeader= readUInt32(zipInput);

			if (cdEntry->lenFilename > 0)
			{
				cdEntry->filename = new BYTE[cdEntry->lenFilename];
				zipInput->Read(cdEntry->filename, cdEntry->lenFilename);
			}
			if (cdEntry->lenExtraField > 0)
			{
				cdEntry->extraField = new BYTE[cdEntry->lenExtraField];
				zipInput->Read(cdEntry->extraField, cdEntry->lenExtraField);
			}
			if (cdEntry->lenComment > 0)
			{
				cdEntry->comment = new BYTE[cdEntry->lenComment];
				zipInput->Read(cdEntry->comment, cdEntry->lenComment);
			}

			centralDirectoryEntries->AddTail(cdEntry);
		}

		retVal = true;
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}
	return retVal;
}

bool CArchiveRecovery::processZipEntry(CFile *zipInput, CFile *zipOutput, uint32 available, CTypedPtrList<CPtrList, ZIP_CentralDirectory*> *centralDirectoryEntries)
{
	if (available < 26)
		return false;

	bool retVal = false;
	long startOffset=0;

	try
	{
		// Need to know where it started
		if (zipOutput)
			startOffset = (long)zipOutput->GetPosition();
		else 
			startOffset = (long)zipInput->GetPosition();

		// Entry format :
		//  4      2 bytes  Version needed to extract
		//  6      2 bytes  General purpose bit flag
		//  8      2 bytes  Compression method
		// 10      2 bytes  Last mod file time
		// 12      2 bytes  Last mod file date
		// 14      4 bytes  CRC-32
		// 18      4 bytes  Compressed size (n)
		// 22      4 bytes  Uncompressed size
		// 26      2 bytes  Filename length (f)
		// 28      2 bytes  Extra field length (e)
		//        (f)bytes  Filename
		//        (e)bytes  Extra field
		//        (n)bytes  Compressed data

		// Read header
		if (readUInt32(zipInput) != ZIP_LOCAL_HEADER_MAGIC)
			return false;

		ZIP_Entry entry = {0};
		entry.versionToExtract		= readUInt16(zipInput);
		entry.generalPurposeFlag	= readUInt16(zipInput);
		entry.compressionMethod		= readUInt16(zipInput);
		entry.lastModFileTime		= readUInt16(zipInput);
		entry.lastModFileDate		= readUInt16(zipInput);
		entry.crc32					= readUInt32(zipInput);
		entry.lenCompressed			= readUInt32(zipInput);
		entry.lenUncompressed		= readUInt32(zipInput);
		entry.lenFilename			= readUInt16(zipInput);
		entry.lenExtraField			= readUInt16(zipInput);
		
		// Do some quick checks at this stage that data is looking ok
		if ((entry.crc32 == 0) && (entry.lenCompressed == 0) && (entry.lenUncompressed == 0) && (entry.lenFilename != 0))
			; // this is a directory entry
		else if ((entry.crc32 == 0) || (entry.lenCompressed == 0) || (entry.lenUncompressed == 0) || (entry.lenFilename == 0))
			return false;

		// Is this entry complete
		if ((entry.lenFilename + entry.lenExtraField + 
			((zipOutput!=NULL)?entry.lenCompressed:0)
			) > (available - 26))
		{
			// Move the file pointer to the start of the next entry
			zipInput->Seek((entry.lenFilename + entry.lenExtraField + entry.lenCompressed), CFile::current);
			return false;
		}

		// Filename
		if (entry.lenFilename > MAX_PATH)
			return false; // Possibly corrupt, don't allocate lots of memory
		entry.filename = new BYTE[entry.lenFilename];
		if (zipInput->Read(entry.filename, entry.lenFilename) != entry.lenFilename)
		{
			delete [] entry.filename;
			return false;
		}

		// Extra data
		if (entry.lenExtraField > 0)
		{
			entry.extraField = new BYTE[entry.lenExtraField];
			zipInput->Read(entry.extraField, entry.lenExtraField);
		}

		if (zipOutput) {
			// Output
			writeUInt32(zipOutput, ZIP_LOCAL_HEADER_MAGIC);
			writeUInt16(zipOutput, entry.versionToExtract);
			writeUInt16(zipOutput, entry.generalPurposeFlag);
			writeUInt16(zipOutput, entry.compressionMethod);
			writeUInt16(zipOutput, entry.lastModFileTime);
			writeUInt16(zipOutput, entry.lastModFileDate);
			writeUInt32(zipOutput, entry.crc32);
			writeUInt32(zipOutput, entry.lenCompressed);
			writeUInt32(zipOutput, entry.lenUncompressed);
			writeUInt16(zipOutput, entry.lenFilename);
			writeUInt16(zipOutput, entry.lenExtraField);
			if (entry.lenFilename > 0)
				zipOutput->Write(entry.filename, entry.lenFilename);
			if (entry.lenExtraField > 0)
				zipOutput->Write(entry.extraField, entry.lenExtraField);
			
			// Read and write compressed data to avoid reading all into memory
			uint32 written = 0;
			BYTE buf[4096];
			uint32 lenChunk = 4096;
			while (written < entry.lenCompressed)
			{
				lenChunk  = (entry.lenCompressed - written);
				if (lenChunk > 4096)
					lenChunk = 4096;
				lenChunk = zipInput->Read(buf, lenChunk);
				if (lenChunk == 0)
					break;
				written += lenChunk;
				zipOutput->Write(buf, lenChunk);
			}
			zipOutput->Flush();
		}

		//Central directory:
		if (centralDirectoryEntries != NULL)
		{
			ZIP_CentralDirectory *cdEntry = new ZIP_CentralDirectory;
			cdEntry->header = ZIP_CD_MAGIC;
			cdEntry->versionMadeBy = entry.versionToExtract;
			cdEntry->versionToExtract = entry.versionToExtract;
			cdEntry->generalPurposeFlag = entry.generalPurposeFlag;
			cdEntry->compressionMethod = entry.compressionMethod;
			cdEntry->lastModFileTime = entry.lastModFileTime;
			cdEntry->lastModFileDate = entry.lastModFileDate;
			cdEntry->crc32 = entry.crc32;
			cdEntry->lenCompressed = entry.lenCompressed;
			cdEntry->lenUncompressed = entry.lenUncompressed;
			cdEntry->lenFilename = entry.lenFilename;
			cdEntry->lenExtraField = entry.lenExtraField;
			cdEntry->diskNumberStart = 0;
			cdEntry->internalFileAttributes = 1;
			cdEntry->externalFileAttributes = 0x81B60020;
			cdEntry->relativeOffsetOfLocalHeader = startOffset;
			cdEntry->filename = entry.filename;

			if (entry.lenExtraField > 0)
				cdEntry->extraField = entry.extraField;

			cdEntry->lenComment = 0;
			if (zipOutput!=NULL) {
				cdEntry->lenComment = (uint16)strlen(ZIP_COMMENT);
				cdEntry->comment = new BYTE[cdEntry->lenComment];
				memcpy(cdEntry->comment, ZIP_COMMENT, cdEntry->lenComment);
			}

			centralDirectoryEntries->AddTail(cdEntry);
		}
		else
		{
			delete [] entry.filename;
			if (entry.lenExtraField > 0)
				delete [] entry.extraField;
		}
		retVal = true;

		// skip data when reading directory only
		if (zipOutput==NULL) {

			// out of available data?
			if ((entry.lenFilename + entry.lenExtraField + entry.lenCompressed ) > (available - 26))
				return false;

			zipInput->Seek( entry.lenCompressed, CFile::current);
		}
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}
	return retVal;
}

void CArchiveRecovery::DeleteMemory(ThreadParam *tp)
{
	POSITION pos = tp->filled->GetHeadPosition();
	while (pos != NULL)
		delete tp->filled->GetNext(pos);
	tp->filled->RemoveAll();
	delete tp->filled;
	delete tp;
}

bool CArchiveRecovery::CopyFile(CPartFile *partFile, CTypedPtrList<CPtrList, Gap_Struct*> *filled, CString tempFileName)
{
	bool retVal = false;
	try
	{
		CFile srcFile;
		if (!srcFile.Open(partFile->GetFilePath(), CFile::modeRead | CFile::shareDenyNone))
			return false;

		// Open destination file and set length to last filled end position
		CFile destFile;
		destFile.Open(tempFileName, CFile::modeWrite | CFile::shareDenyWrite | CFile::modeCreate);
		Gap_Struct *fill = filled->GetTail();
		destFile.SetLength(fill->end);

		BYTE buffer[4096];
		uint32 read;
		uint32 copied;

		// Loop through filled areas and copy data
		partFile->m_bPreviewing = true;
		POSITION pos = filled->GetHeadPosition();
		while (pos != NULL)
		{
			fill = filled->GetNext(pos);
			copied = 0;
			srcFile.Seek(fill->start, CFile::begin);
			destFile.Seek(fill->start, CFile::begin);
			while ((read = srcFile.Read(buffer, 4096)) > 0)
			{
				destFile.Write(buffer, read);
				copied += read;
				// Stop when finished fill (don't worry about extra)
				if (fill->start + copied >= fill->end)
					break;
			}
		}
		destFile.Close();
		srcFile.Close();
		partFile->m_bPreviewing = false;

		retVal = true;
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}

	return retVal;
}

bool CArchiveRecovery::recoverRar(CFile *rarInput, CFile *rarOutput, archiveScannerThreadParams_s* aitp, 
								  CTypedPtrList<CPtrList, Gap_Struct*> *filled)
{
	bool retVal = false;
	long fileCount = 0;
	try
	{
		// Try to get file header and main header
		//
		bool bValidFileHeader = false;
		bool bOldFormat = false;
		bool bValidMainHeader = false;
		BYTE fileHeader[7] = {0};
		RARMAINHDR mainHeader = {0};
		if (rarInput->Read(fileHeader, sizeof fileHeader) == sizeof fileHeader)
		{
			if (fileHeader[0] == 0x52) {
				if (fileHeader[1] == 0x45 && fileHeader[2] ==0x7e && fileHeader[3] == 0x5e) {
					bOldFormat = true;
					bValidFileHeader = true;
				}
				else if (fileHeader[1] == 0x61 && fileHeader[2] == 0x72 && fileHeader[3] == 0x21 && fileHeader[4] == 0x1a && fileHeader[5] == 0x07 && fileHeader[6] == 0x00) {
					bValidFileHeader = true;
				}
			}

			if (bValidFileHeader && !bOldFormat)
			{
				WORD checkCRC;
				if (rarInput->Read(&checkCRC, sizeof checkCRC) == sizeof checkCRC)
				{
					if (rarInput->Read(&mainHeader, sizeof mainHeader) == sizeof mainHeader)
					{
						if (mainHeader.type == 0x73)
						{
							DWORD crc = crc32(0, (Bytef*)&mainHeader, sizeof mainHeader);
							for (UINT i = 0; i < sizeof(WORD) + sizeof(DWORD); i++)
							{
								BYTE ch;
								if (rarInput->Read(&ch, sizeof ch) != sizeof ch)
									break;
								crc = crc32(crc, &ch, 1);
							}
							if (checkCRC == (WORD)crc)
								bValidMainHeader = true;
						}
					}
				}
			}
			rarInput->SeekToBegin();
		}

		static const BYTE start[] = {
			// RAR file header
			0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00,

			// main header
			0x08, 0x1A, 		// CRC
			0x73, 				// type
			0x02, 0x00,			// flags
			0x3B, 0x00,			// size
			0x00, 0x00,			// AV
			0x00, 0x00,			// AV
            0x00, 0x00,			// AV

			// main comment
			0xCA, 0x44,			// CRC
			0x75,				// type
			0x00, 0x00,			// flags
			0x2E, 0x00,			// size

			0x12, 0x00, 0x14, 0x34, 0x2B,
			0x4A, 0x08, 0x15, 0x48, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x2B, 0xF9, 0x0E, 0xE2, 0xC1,
			0x32, 0xFB, 0x9E, 0x04, 0x10, 0x50, 0xD7, 0xFE, 0xCD, 0x75, 0x87, 0x9C, 0x28, 0x85, 0xDF, 0xA3,
			0x97, 0xE0 };

		// If this is a 'solid' archive the chance to successfully decompress any entries gets higher,
		// when we pass the 'solid' main header bit to the temp. archive.
		BYTE start1[sizeof start];
		memcpy(start1, start, sizeof start);
		if (bValidFileHeader && bValidMainHeader && (mainHeader.flags & 0x0008/*MHD_SOLID*/)) {
			start1[10] |= 8; /*MHD_SOLID*/
			*((short*)&start1[7]) = (short)crc32(0, &start1[9], 11);
		}

		if (aitp)
			aitp->ai->rarFlags=mainHeader.flags;

		if (rarOutput)
			rarOutput->Write(start1, sizeof(start1));

		RAR_BlockFile *block;

		// loop filled blocks
		POSITION pos = filled->GetHeadPosition();
		Gap_Struct *fill;
		while (pos != NULL)
		{
			fill = filled->GetNext(pos);
			
			rarInput->Seek(fill->start , CFile::begin);

			while ((block = scanForRarFileHeader(rarInput, aitp, (fill->end - rarInput->GetPosition() ))) != NULL)
			{
				if (aitp){
					aitp->ai->RARdir->AddTail(block);
					if (!aitp->m_bIsValid)
						return false;
				}

				if (rarOutput!=NULL && IsFilled((UINT)block->offsetData, (UINT)(block->offsetData + block->dataLength), filled))
				{
					// Don't include directories in file count
					if ((block->HEAD_FLAGS & 0xE0) != 0xE0) 
						fileCount++;
					if (rarOutput)
						writeRarBlock(rarInput, rarOutput, block);
				}
				else
				{
					rarInput->Seek(block->offsetData + block->dataLength, CFile::begin);
				}
				if (aitp==NULL) {
					delete [] block->FILE_NAME;
					delete block;
				}
				if (rarInput->GetPosition() >=fill->end)
					break;
			}
		}
		retVal = true;
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}

	// Tell the user how many files were recovered
	if (aitp==NULL) {
		CString msg;	
		if (fileCount == 1)
			msg = GetResString(IDS_RECOVER_SINGLE);
		else
			msg.Format(GetResString(IDS_RECOVER_MULTIPLE), fileCount);
		theApp.QueueLogLine(true, _T("%s"), msg);
	}

	return retVal;
}

bool CArchiveRecovery::IsFilled(uint32 start, uint32 end, CTypedPtrList<CPtrList, Gap_Struct*> *filled)
{
	POSITION pos = filled->GetHeadPosition();
	Gap_Struct *fill;
	while (pos != NULL)
	{
		fill = filled->GetNext(pos);
		if (fill->start > start)
			return false;
		if (fill->end >= end)
			return true;
	}
	return false;
}

// This will find the marker in the file and leave it positioned at the position to read the marker again
bool CArchiveRecovery::scanForZipMarker(CFile *input, archiveScannerThreadParams_s* aitp, uint32 marker, uint32 available)
{
	try
	{
		//uint32 originalOffset = input->GetPosition();
		int lenChunk = 51200; // 50k buffer 
		BYTE chunk[51200];
		BYTE *foundPos = NULL;
		int pos = 0;

		while ((available > 0) && ((lenChunk = input->Read(chunk, lenChunk)) > 0))
		{
			available -= lenChunk;
			foundPos = &chunk[0];
			// Move back one, will be incremented in loop
			foundPos--;
			while (foundPos != NULL)
			{
				if (aitp && !aitp->m_bIsValid)
					return false;

				// Find first matching byte
				foundPos = (BYTE*)memchr( foundPos+1, (marker & 0xFF), (lenChunk - (foundPos+1 - (&chunk[0]))) );
				if (foundPos == NULL)
					break;

				// Test for end of buffer
				pos = foundPos - (&chunk[0]);
				if ((pos + 3) > lenChunk)
				{
					// Re-read buffer starting from found first byte position
					input->Seek(pos - lenChunk, CFile::current);
					break;
				}

				if (aitp) {
					ProcessProgress(aitp, input->GetPosition() );
				}


				// Check for other bytes
				if (chunk[pos + 1] == ((marker >> 8) & 0xFF))
				{
					if (chunk[pos + 2] == ((marker >> 16) & 0xFF))
					{
						if (chunk[pos + 3] == ((marker >> 24) & 0xFF))
						{
							// Found it
							input->Seek(pos - lenChunk, CFile::current);
							return true;
						}
					}
				}
			}
		}
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}
	return false;
}

#define TESTCHUNKSIZE 51200	 // 50k buffer 
// This will find a file block in the file and leave it positioned at the end of the filename
RAR_BlockFile *CArchiveRecovery::scanForRarFileHeader(CFile *input, archiveScannerThreadParams_s* aitp, UINT64 available)
{
	RAR_BlockFile *retVal = NULL;
	try
	{
		UINT lenChunk;
		BYTE chunk[TESTCHUNKSIZE];
		BYTE *foundPos = NULL;
		ULONGLONG foundOffset;
		ULONGLONG chunkOffset;
		UINT64 chunkstart;

		uint16 headCRC;
		BYTE checkCRC[sizeof(RARFILEHDR) + 8 + sizeof(DWORD)*2 + 512];
		unsigned checkCRCsize = 0;
		uint16 lenFileName;
		BYTE *fileName;
		uint32 crc;

		while (available > 0)
		{
			chunkstart=input->GetPosition();
			lenChunk = input->Read(chunk, (UINT)(min(available, TESTCHUNKSIZE)) );
			if (lenChunk==0)
				break;

			available-=lenChunk;
			foundPos = &chunk[0];

			// Move back one, will be incremented in loop
			foundPos--;
			while (foundPos != NULL)
			{
				if (aitp && !aitp->m_bIsValid)
					return false;

				// Find rar head block marker
				foundPos = (BYTE*)memchr( foundPos+1, RAR_HEAD_FILE, (lenChunk - (foundPos+1 - (&chunk[0]))   ) );
				if (foundPos == NULL)
					break;

				chunkOffset = foundPos - (&chunk[0]);
				foundOffset = chunkstart + chunkOffset;

				if (aitp) {
					ProcessProgress(aitp,foundOffset);
				}

				// Move back 2 bytes to get crc and read block
				input->Seek(foundOffset-2 , CFile::begin);

				// CRC of fields from HEAD_TYPE to ATTR + filename + ext. stuff
				headCRC = readUInt16(input);

				RARFILEHDR* hdr = (RARFILEHDR*)checkCRC;
				input->Read(checkCRC, sizeof(*hdr));

				// this bit always is set
				if (!(hdr->flags & 0x8000))
					continue;

				checkCRCsize = sizeof(*hdr);

				// get high parts of 64-bit file size fields
				if (hdr->flags & 0x0100/*LHD_LARGE*/) {
					input->Read(&checkCRC[checkCRCsize], sizeof(DWORD) * 2);
					checkCRCsize += sizeof(DWORD) * 2;
				}

				// get filename
				lenFileName = hdr->nameSize;
				fileName = new BYTE[lenFileName];
				input->Read(fileName, lenFileName);

				// get encryption params
				unsigned saltPos = 0;
				if (hdr->flags & 0x0400/*LHD_SALT*/) {
					saltPos = checkCRCsize;
					input->Read(&checkCRC[checkCRCsize], 8);
					checkCRCsize += 8;
				}

				// get ext. file date/time
				unsigned extTimePos = 0;
				unsigned extTimeSize = 0;
				if (hdr->flags & 0x1000/*LHD_EXTTIME*/)
				{
					try {
						extTimePos = checkCRCsize;
						if (checkCRCsize + sizeof(WORD) > sizeof(checkCRC))
							throw -1;
						input->Read(&checkCRC[checkCRCsize], sizeof(WORD));
						unsigned short Flags = *((WORD*)&checkCRC[checkCRCsize]);
						checkCRCsize += sizeof(WORD);
						for (int i = 0; i < 4; i++)
						{
							unsigned int rmode = Flags >> (3 - i) * 4;
							if ((rmode & 8) == 0)
								continue;
							if (i != 0) {
								if (checkCRCsize + sizeof(DWORD) > sizeof(checkCRC))
									throw -1;
								input->Read(&checkCRC[checkCRCsize], sizeof(DWORD));
								checkCRCsize += sizeof(DWORD);
							}
							int count = rmode & 3;
							for (int j = 0; j < count; j++) {
								if (checkCRCsize + sizeof(BYTE) > sizeof(checkCRC))
									throw -1;
								input->Read(&checkCRC[checkCRCsize], sizeof(BYTE));
								checkCRCsize += sizeof(BYTE);
							}
						}
						extTimeSize = checkCRCsize - extTimePos;
					}
					catch (int ex) {
						(void)ex;
						extTimePos = 0;
						extTimeSize = 0;
					}
				}

				crc = crc32(0, checkCRC, sizeof(*hdr));
				crc = crc32(crc, fileName, lenFileName);
				if (checkCRCsize > sizeof(*hdr))
					crc = crc32(crc, &checkCRC[sizeof(*hdr)], checkCRCsize - sizeof(*hdr));
				if ((crc & 0xFFFF) == headCRC)
				{
					// Found valid crc, build block and return
					// Note that it may still be invalid data, so more checks should be performed
					retVal = new RAR_BlockFile;
					retVal->HEAD_CRC		= headCRC;
					retVal->HEAD_TYPE		= 0x74;
					retVal->HEAD_FLAGS		= calcUInt16(&checkCRC[ 1]);
					retVal->HEAD_SIZE		= calcUInt16(&checkCRC[ 3]);
					retVal->PACK_SIZE		= calcUInt32(&checkCRC[ 5]);
					retVal->UNP_SIZE		= calcUInt32(&checkCRC[ 9]);
					retVal->HOST_OS			= checkCRC[13];
					retVal->FILE_CRC		= calcUInt32(&checkCRC[14]);
					retVal->FTIME			= calcUInt32(&checkCRC[18]);
					retVal->UNP_VER			= checkCRC[22];
					retVal->METHOD			= checkCRC[23];
					retVal->NAME_SIZE		= lenFileName;
					retVal->ATTR			= calcUInt32(&checkCRC[26]);
					// Optional values, present only if bit 0x100 in HEAD_FLAGS is set.
					if ((retVal->HEAD_FLAGS & 0x100) == 0x100) {
						retVal->HIGH_PACK_SIZE	= calcUInt32(&checkCRC[30]);
						retVal->HIGH_UNP_SIZE	= calcUInt32(&checkCRC[34]);
					}else {
						retVal->HIGH_PACK_SIZE=0;
						retVal->HIGH_UNP_SIZE=0;
					}
					retVal->FILE_NAME		= fileName;
					if (saltPos != 0)
						memcpy(retVal->SALT, &checkCRC[saltPos], sizeof retVal->SALT);
					if (extTimePos != 0 && extTimeSize != 0) {
						retVal->EXT_DATE = new BYTE[extTimeSize];
						memcpy(retVal->EXT_DATE, &checkCRC[extTimePos], extTimeSize);
						retVal->EXT_DATE_SIZE = extTimeSize;
					}

					// Run some quick checks
					if (validateRarFileBlock(retVal))
					{
						// Set some useful markers in the block
						retVal->offsetData = input->GetPosition();
						uint32 dataLength = retVal->PACK_SIZE;
						// If comment present find length
						if ((retVal->HEAD_FLAGS & 0x08) == 0x08)
						{
							// Skip start of comment block
							input->Seek(5, CFile::current);
							// Read comment length
							dataLength += readUInt16(input);
						}
						retVal->dataLength = dataLength;

						return retVal;
					}
				}
				// If not valid, continue searching where we left of
				delete [] fileName;
				delete retVal;
				retVal = NULL;
			}
		}
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}
	return false;
}

// This assumes that head crc has already been checked
bool CArchiveRecovery::validateRarFileBlock(RAR_BlockFile *block)
{
	if (block->HEAD_TYPE != 0x74)
		return false;

	// The following condition basically makes sense, but it though triggers false errors
	// in some cases (e.g. when there are very small files compressed it's not that unlikely
	// that the compressed size is a little larger than the uncompressed size due to RAR
	// overhead.
	//if ((block->HEAD_FLAGS & 0x0400/*LHD_SALT*/) == 0 && block->UNP_SIZE < block->PACK_SIZE)
	//	return false;

	if (block->HOST_OS > 5)
		return false;
	
	switch (block->METHOD)
	{
		case 0x30: // storing
		case 0x31: // fastest compression
		case 0x32: // fast compression
		case 0x33: // normal compression
		case 0x34: // good compression
		case 0x35: // best compression
			break;
		default:
			return false;
	}
	
	if (block->HEAD_FLAGS & 0x0200) {
		// ANSI+Unicode name
		if (block->NAME_SIZE > MAX_PATH + MAX_PATH*2)
			return false;
	}
	else {
		// ANSI+Unicode name
		if (block->NAME_SIZE > MAX_PATH)
			return false;
	}

	// Check directory entry has no size
	if (((block->HEAD_FLAGS & 0xE0) == 0xE0) && ((block->PACK_SIZE + block->UNP_SIZE + block->FILE_CRC) > 0))
		return false;

	return true;
}

void CArchiveRecovery::writeRarBlock(CFile *input, CFile *output, RAR_BlockFile *block)
{
	ULONGLONG offsetStart = output->GetPosition();
	try
	{
		writeUInt16(output, block->HEAD_CRC);
		output->Write(&block->HEAD_TYPE, 1);
		writeUInt16(output, block->HEAD_FLAGS);
		writeUInt16(output, block->HEAD_SIZE);
		writeUInt32(output, block->PACK_SIZE);
		writeUInt32(output, block->UNP_SIZE);
		output->Write(&block->HOST_OS, 1);
		writeUInt32(output, block->FILE_CRC);
		writeUInt32(output, block->FTIME);
		output->Write(&block->UNP_VER, 1);
		output->Write(&block->METHOD, 1);
		writeUInt16(output, block->NAME_SIZE);
		writeUInt32(output, block->ATTR);
		// Optional values, present only if bit 0x100 in HEAD_FLAGS is set.
		if ((block->HEAD_FLAGS & 0x100) == 0x100) {
			writeUInt32(output, block->HIGH_PACK_SIZE);
			writeUInt32(output, block->HIGH_UNP_SIZE);
		}
		output->Write(block->FILE_NAME, block->NAME_SIZE);
		if (block->HEAD_FLAGS & 0x0400/*LHD_SALT*/)
			output->Write(block->SALT, sizeof block->SALT);
		output->Write(block->EXT_DATE, block->EXT_DATE_SIZE);

		// Now copy compressed data from input file
		uint32 lenToCopy = block->dataLength;
		if (lenToCopy > 0)
		{
			input->Seek(block->offsetData, CFile::begin);
			uint32 written = 0;
			BYTE chunk[4096];
			uint32 lenChunk = 4096;
			while (written < lenToCopy)
			{
				lenChunk  = (lenToCopy - written);
				if (lenChunk > 4096)
					lenChunk = 4096;
				lenChunk = input->Read(chunk, lenChunk);
				if (lenChunk == 0)
					break;
				written += lenChunk;
				output->Write(chunk, lenChunk);
			}
		}
		output->Flush();
	}
	catch (CFileException* error){
		error->Delete();
		try { output->SetLength(offsetStart); } catch (...) {ASSERT(0);}
	}
	catch (...){
		ASSERT(0);
		try { output->SetLength(offsetStart); } catch (...) {ASSERT(0);}
	}
}


//################################   ACE   ####################################
#define CRC_MASK 0xFFFFFFFFL
#define CRCPOLY  0xEDB88320L
static ULONG crctable[256];
void make_crctable(void)   // initializes CRC table
{
   ULONG r,i,j;

   for (i = 0; i <= 255; i++)
   {
      for (r = i, j = 8; j; j--)
         r = (r & 1) ? (r >> 1) ^ CRCPOLY : (r >> 1);
      crctable[i] = r;
   }
}

// Updates crc from addr till addr+len-1
//
ULONG getcrc(ULONG crc, UCHAR * addr, INT len)
{
   while (len--)
      crc = crctable[(unsigned char) crc ^ (*addr++)] ^ (crc >> 8);
   return (crc);
}

bool CArchiveRecovery::recoverAce(CFile *aceInput, CFile *aceOutput, archiveScannerThreadParams_s* aitp, 
								  CTypedPtrList<CPtrList, Gap_Struct*> *filled)
{
	UINT64 filesearchstart=0;
	bool retVal=false;
	ACE_ARCHIVEHEADER* acehdr=NULL;

	make_crctable();

	try
	{
		// Try to get file header and main header
		if (IsFilled(0,32,filled)) {
			static const char ACE_ID[]	= { 0x2A, 0x2A, 0x41, 0x43, 0x45, 0x2A, 0x2A };
			acehdr=new ACE_ARCHIVEHEADER;

			LONG headcrc;
			UINT hdrread=0;

			hdrread+=aceInput->Read((void*)acehdr, sizeof(ACE_ARCHIVEHEADER) - (3*sizeof(char*)) - sizeof(uint16) );

			if (memcmp(acehdr->HEAD_SIGN , ACE_ID, sizeof(ACE_ID) )!=0 || acehdr->HEAD_TYPE!=0 ||
				IsFilled(0,acehdr->HEAD_SIZE,filled)==false ) {
				delete acehdr;
				acehdr=NULL;
				if (aceOutput)
					return false;

			}else {

				hdrread-= 2*sizeof(uint16);		// care for the size that is specified with HEADER_SIZE
				headcrc = getcrc(CRC_MASK, (BYTE*)&acehdr->HEAD_TYPE, hdrread );
				
				if (acehdr->AVSIZE) {
					acehdr->AV=(char*)calloc(acehdr->AVSIZE+1,1);
					hdrread+=aceInput->Read(acehdr->AV, acehdr->AVSIZE);
					headcrc = getcrc(headcrc, (UCHAR*)acehdr->AV, acehdr->AVSIZE);
				}

				if (hdrread<acehdr->HEAD_SIZE) {

					hdrread+=aceInput->Read(&acehdr->COMMENT_SIZE, sizeof(acehdr->COMMENT_SIZE));
					headcrc = getcrc(headcrc, (UCHAR*)(&acehdr->COMMENT_SIZE), sizeof(acehdr->COMMENT_SIZE) );
					if (acehdr->COMMENT_SIZE){
						acehdr->COMMENT=(char*)calloc(acehdr->COMMENT_SIZE+1,1);
						hdrread+=aceInput->Read(acehdr->COMMENT, acehdr->COMMENT_SIZE);
						headcrc = getcrc(headcrc, (UCHAR*)acehdr->COMMENT, acehdr->COMMENT_SIZE );
					}
				}

				// some unknown data left to read
				if (hdrread<acehdr->HEAD_SIZE) {
					UINT dumpsize=acehdr->HEAD_SIZE-hdrread;
					acehdr->DUMP=(char*)malloc(dumpsize);
					hdrread+=aceInput->Read(acehdr->DUMP , dumpsize);
					headcrc = getcrc(headcrc, (UCHAR*)acehdr->DUMP, dumpsize );
				}

				if (acehdr->HEAD_CRC == (headcrc & 0xffff)) {
					if (aitp)
						aitp->ai->ACEhdr=acehdr;
					filesearchstart=acehdr->HEAD_SIZE + 2*sizeof(uint16);
					if (aceOutput)
						writeAceHeader(aceOutput,acehdr);

				} else {
					aceInput->SeekToBegin();
					filesearchstart=0;
					delete acehdr;
					acehdr=NULL;
				}
			}
		}

		if (aceOutput && !acehdr)
			return false;
		if(aitp == NULL && acehdr != NULL)
		{
			delete acehdr;
			acehdr = NULL;
		}


		ACE_BlockFile *block;

		// loop filled blocks
		POSITION pos = filled->GetHeadPosition();
		Gap_Struct *fill;
		while (pos != NULL)
		{
			fill = filled->GetNext(pos);

			filesearchstart = (filesearchstart<fill->start)?fill->start:filesearchstart;
			aceInput->Seek( filesearchstart, CFile::begin);

			while ((block = scanForAceFileHeader(aceInput, aitp, (fill->end - filesearchstart ))) != NULL)
			{
				if (aitp){
					if (!aitp->m_bIsValid)
						return false;

					aitp->ai->ACEdir->AddTail(block);
				}


				if (aceOutput!=NULL && IsFilled((UINT)block->data_offset, (UINT)(block->data_offset + block->PACK_SIZE), filled))
				{
					// Don't include directories in file count
					//if ((block->HEAD_FLAGS & 0xE0) != 0xE0)						fileCount++;
					writeAceBlock(aceInput, aceOutput, block);
				}
				else
				{
					aceInput->Seek(block->PACK_SIZE, CFile::current);
				}
				if (aitp==NULL) {
					delete block;
				}
				if (aceInput->GetPosition() >=fill->end)
					break;
			}
		}
		retVal = true;

	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}

	return retVal;
}

#define MAXACEHEADERSIZE 10240
ACE_BlockFile *CArchiveRecovery::scanForAceFileHeader(CFile *input, archiveScannerThreadParams_s* aitp, UINT64 available)
{
	static char blockmem[MAXACEHEADERSIZE];
	
	BYTE chunk[TESTCHUNKSIZE];
	UINT lenChunk;
	BYTE *foundPos = NULL;
	ULONGLONG foundOffset;
	ULONGLONG chunkOffset;
	UINT64 chunkstart;
	uint16 headCRC,headSize;
	uint32 crc;
	

	try
	{
		while (available > 0)
		{
			chunkstart=input->GetPosition();
			lenChunk = input->Read(chunk, (UINT)(min(available, TESTCHUNKSIZE)) );
			if (lenChunk==0)
				break;

			available-=lenChunk;
			foundPos = &chunk[0];

			// Move back one, will be incremented in loop
			foundPos--;
			while (foundPos != NULL)
			{
				if (aitp && !aitp->m_bIsValid)
					return false;

				// Find rar head block marker
				foundPos = (BYTE*)memchr( foundPos+1, 0x01, (lenChunk - (foundPos+1 - (&chunk[0]))   ) );
				if (foundPos == NULL)
					break;

				chunkOffset = foundPos - (&chunk[0]);
				foundOffset = chunkstart + chunkOffset;

				if (chunkOffset<4)
					continue;

				if (aitp) {
					ProcessProgress(aitp,foundOffset);
				}


				// Move back 4 bytes to get crc,size and read block
				input->Seek(foundOffset-4 , CFile::begin);

				input->Read(&headCRC, 2);
				input->Read(&headSize,2);
				if (headSize>MAXACEHEADERSIZE || // header limit
					(lenChunk-chunkOffset)+available < headSize	// header too big for my filled part
				)		
					continue;

				input->Read(&blockmem[0],headSize);
				crc = getcrc(CRC_MASK, (BYTE*)blockmem,headSize);

				if ( (crc & 0xFFFF) != headCRC  )
					continue;

				char* mempos= &blockmem[0];

				ACE_BlockFile *newblock = new ACE_BlockFile;

				newblock->HEAD_CRC=headCRC;
				newblock->HEAD_SIZE=headSize;

				memcpy(&newblock->HEAD_TYPE,
					mempos, 
					sizeof(ACE_BlockFile)- 4- (2*sizeof(newblock->COMMENT)) - sizeof(newblock->COMM_SIZE) - sizeof(newblock->data_offset) );

				mempos+=sizeof(ACE_BlockFile)-sizeof(newblock->HEAD_CRC)-sizeof(newblock->HEAD_SIZE) - (2*sizeof(newblock->COMMENT)) - sizeof(newblock->COMM_SIZE) - sizeof(newblock->data_offset);

				// basic checks
				if (newblock->HEAD_SIZE < newblock->FNAME_SIZE) {
					delete newblock;
					continue;
				}

				if (newblock->FNAME_SIZE>0) {
					newblock->FNAME = (char*)calloc(newblock->FNAME_SIZE+1,1);
					memcpy(newblock->FNAME, mempos,newblock->FNAME_SIZE);
					mempos+=newblock->FNAME_SIZE;
				}

//				int blockleft=newblock->HEAD_SIZE - newblock->FNAME_SIZE -					(sizeof(ACE_BlockFile)- (3*sizeof(uint16)) - sizeof(newblock->data_offset) - 2* sizeof(char*) ) ;

				if (mempos < &blockmem[0] + newblock->HEAD_SIZE) {
					memcpy(&newblock->COMM_SIZE, mempos, sizeof(newblock->COMM_SIZE));
					mempos+=sizeof(newblock->COMM_SIZE);
					
					if (newblock->COMM_SIZE) {
						newblock->COMMENT = (char*)calloc(newblock->COMM_SIZE,1);
						memcpy(newblock->COMMENT, mempos,newblock->COMM_SIZE);
						mempos+=newblock->COMM_SIZE;
					}
				} 
				
				//if (mempos-blockmem[4] > 0) input->Seek(blockleft, CFile::current) ;


				newblock->data_offset= input->GetPosition();
				return newblock;

			} // while foundpos
		} // while available>0
	}
	catch (CFileException* error){
		error->Delete();
	}
	catch (...){
		ASSERT(0);
	}

	return NULL;
}

void CArchiveRecovery::writeAceHeader(CFile *output, ACE_ARCHIVEHEADER* hdr) {

	writeUInt16(output, hdr->HEAD_CRC);
	writeUInt16(output, hdr->HEAD_SIZE);
	output->Write(&hdr->HEAD_TYPE, 1);
	writeUInt16(output, hdr->HEAD_FLAGS);
	output->Write(&hdr->HEAD_SIGN, sizeof(hdr->HEAD_SIGN));
	output->Write(&hdr->VER_EXTRACT, 1);
	output->Write(&hdr->VER_CREATED, 1);
	output->Write(&hdr->HOST_CREATED, 1);
	output->Write(&hdr->VOLUME_NUM, 1);
	writeUInt32(output, hdr->FTIME);
	output->Write(&hdr->RESERVED, sizeof(hdr->RESERVED));
	output->Write(&hdr->AVSIZE, 1);

	int rest= hdr->HEAD_SIZE - (sizeof(ACE_ARCHIVEHEADER)-	(3*sizeof(uint16)) -  (3*sizeof(char*)));

	if (hdr->AVSIZE>0){
		output->Write(hdr->AV, hdr->AVSIZE);
		rest-=hdr->AVSIZE;
	}

	if (hdr->COMMENT_SIZE) {
		writeUInt16(output, hdr->COMMENT_SIZE);
		rest-=sizeof(hdr->COMMENT_SIZE);

		output->Write(hdr->COMMENT, hdr->COMMENT_SIZE);
		rest-=hdr->COMMENT_SIZE;
	}

	if ( rest && hdr->DUMP ) {
		output->Write(hdr->DUMP,rest);
		rest=0;
	}
	ASSERT(rest==0);
}

void CArchiveRecovery::writeAceBlock(CFile *input, CFile *output, ACE_BlockFile *block)
{
	ULONGLONG offsetStart = output->GetPosition();
	try
	{
//		block->data_offset = offsetStart;


		writeUInt16(output, block->HEAD_CRC);
		writeUInt16(output, block->HEAD_SIZE);
		output->Write(&block->HEAD_TYPE, 1);
		writeUInt16(output, block->HEAD_FLAGS);
		writeUInt32(output, block->PACK_SIZE);
		writeUInt32(output, block->ORIG_SIZE);
		writeUInt32(output, block->FTIME);
		writeUInt32(output, block->FILE_ATTRIBS);
		writeUInt32(output, block->CRC32);
		writeUInt32(output, block->TECHINFO);
		writeUInt16(output, block->RESERVED);
		writeUInt16(output, block->FNAME_SIZE);
		output->Write(block->FNAME, block->FNAME_SIZE);
		
		if (block->COMM_SIZE) {
			writeUInt16(output, block->COMM_SIZE);
			output->Write(block->COMMENT, block->COMM_SIZE);
		}

		// skip unknown data between header and compressed data - if ever existed...

		// Now copy compressed data from input file
		uint32 lenToCopy = block->PACK_SIZE;
		if (lenToCopy > 0)
		{
			input->Seek(block->data_offset, CFile::begin);
			uint32 written = 0;
			BYTE chunk[4096];
			uint32 lenChunk = 4096;
			while (written < lenToCopy)
			{
				lenChunk  = (lenToCopy - written);
				if (lenChunk > 4096)
					lenChunk = 4096;
				lenChunk = input->Read(chunk, lenChunk);
				if (lenChunk == 0)
					break;
				written += lenChunk;
				output->Write(chunk, lenChunk);
			}
		}

		output->Flush();
	}
	catch (CFileException* error){
		error->Delete();
		try { output->SetLength(offsetStart); } catch (...) {ASSERT(0);}
	}
	catch (...){
		ASSERT(0);
		try { output->SetLength(offsetStart); } catch (...) {ASSERT(0);}
	}
}

// ############### ISO handling #############
// ISO, reads a directory entries of a directory at the given sector (startSec)

void CArchiveRecovery::ISOReadDirectory(archiveScannerThreadParams_s* aitp, UINT32 startSec, CFile* isoInput, CString currentDirName)
{
	// read directory entries
	int iSecsOfDirectoy = -1;
	UINT32 blocksize;

	if (!IsFilled( startSec * aitp->ai->isoInfos.secSize, (startSec * aitp->ai->isoInfos.secSize) + aitp->ai->isoInfos.secSize,
		aitp->filled))
		return;

	isoInput->Seek(startSec * aitp->ai->isoInfos.secSize, FILE_BEGIN);

	while (aitp->m_bIsValid)
	{
		ISO_FileFolderEntry *file = new ISO_FileFolderEntry;
		
		blocksize = isoInput->Read(file, sizeof(ISO_FileFolderEntry)-sizeof(file->name) );

		if (file->lenRecord==0) {
			delete file;

			// do we continue at next sector?
			if (iSecsOfDirectoy-- > 1) {
				startSec++;
				if (!IsFilled( startSec * aitp->ai->isoInfos.secSize, (startSec * aitp->ai->isoInfos.secSize) + aitp->ai->isoInfos.secSize,
					aitp->filled))
						break;

				isoInput->Seek(startSec * aitp->ai->isoInfos.secSize, FILE_BEGIN);
				continue;
			} else
				break; // folder end
		}

		file->name = (TCHAR*)calloc(	file->nameLen+2, sizeof(TCHAR*));

		blocksize += isoInput->Read(file->name, file->nameLen );
		
		if (file->nameLen % 2 ==0 )
			blocksize++;

		UINT32 skip=LODWORD(file->lenRecord) - blocksize;
		UINT64 pos2 = isoInput->Seek(skip, FILE_CURRENT);		// skip padding 
		if (pos2 % 2 ){
			isoInput->Seek(1, FILE_CURRENT);					// skip padding 
			pos2++;
		}

		// set progressbar
		if (aitp)
			ProcessProgress(aitp, pos2);
		
		// selfdir, parentdir ( "." && ".." ) handling
		if ((file->fileFlags & ISO_DIRECTORY) && file->nameLen==1 && (file->name[0]==0x00 || file->name[0]==0x01 ) )
		{
			// get size of directory and calculate how many sectors are spanned
			if (file->name[0]==0x00)
				iSecsOfDirectoy = (int)(file->dataSize / aitp->ai->isoInfos.secSize);
			delete file;
			continue;
		}

		if (aitp->ai->isoInfos.iJolietUnicode){
			for (unsigned int i = 0; i < file->nameLen/sizeof(uint16); i++)
				file->name[i] = _byteswap_ushort(file->name[i]);
		}

		// make filename Cstring from ascii or unicode
		CString filename;
		if (aitp->ai->isoInfos.iJolietUnicode==0)
			filename = CString((char*)file->name);
		else
			filename = CString(file->name);

		if (file->fileFlags & ISO_DIRECTORY)
		{
			// store dir entry
			CString pathNew;
			pathNew = currentDirName + filename + _T("\\");
			free(file->name);
			file->name = _tcsdup(pathNew);
			aitp->ai->ISOdir->AddTail(file);

			// read subdirectory recursively
			LONGLONG curpos = isoInput->GetPosition();
			ISOReadDirectory(aitp, LODWORD(file->sector1OfExtension), isoInput, pathNew);
			isoInput->Seek(curpos,FILE_BEGIN);
		}
			else 
		{
			// store file entry
			CString fullpath;
			fullpath = currentDirName + filename;
			free(file->name);
			file->name = _tcsdup(fullpath);
			aitp->ai->ISOdir->AddTail(file);
		}
	}
}

bool CArchiveRecovery::recoverISO(CFile *isoInput, CFile *isoOutput, archiveScannerThreadParams_s* aitp, 
								  CTypedPtrList<CPtrList, Gap_Struct*> *filled)
{
	if (isoOutput)
		return false;

	aitp->ai->isoInfos.secSize = sizeof(ISO_PVD_s);

	// do we have the primary volume descriptor ?
	if (!IsFilled(16*aitp->ai->isoInfos.secSize, 17*aitp->ai->isoInfos.secSize, filled))
		return false;

	ISO_PVD_s pvd, svd, tempSec;

	// skip to PVD
	UINT32 nextstart = 16 * aitp->ai->isoInfos.secSize;
	isoInput->Seek(nextstart, FILE_BEGIN);

	pvd.descr_type=0xff;
	svd.descr_type=0xff;
	aitp->ai->isoInfos.type = ISOtype_unknown;

	int iUdfDetectState=0;
	// read PVD
	do 
	{
		isoInput->Read(&tempSec, aitp->ai->isoInfos.secSize);
		nextstart+=aitp->ai->isoInfos.secSize;

		if (tempSec.descr_type==0xff || (tempSec.descr_type==0 && tempSec.descr_ver==0)) // Volume Descriptor Set Terminator (VDST) 
			break;

		if (tempSec.descr_type==0x01 && pvd.descr_type==0xff) {
			memcpy(&pvd, &tempSec, aitp->ai->isoInfos.secSize);
			aitp->ai->isoInfos.type |= ISOtype_9660;
		}
		
		if (tempSec.descr_type==0x02) {
			memcpy(&svd, &tempSec, aitp->ai->isoInfos.secSize);
			if (svd.escSeq[0]==0x25 && svd.escSeq[1]==0x2f)
			{
				aitp->ai->isoInfos.type |= ISOtype_joliet;

				if (svd.escSeq[2]==0x40)
					aitp->ai->isoInfos.iJolietUnicode = 1;
				else if (svd.escSeq[2]==0x43)
					aitp->ai->isoInfos.iJolietUnicode = 2;
				else if (svd.escSeq[2]==0x45)
					aitp->ai->isoInfos.iJolietUnicode = 3;
			}
		}

		if (tempSec.descr_type==0x00) {
			BootDescr* bDesc = (BootDescr*)&tempSec;
			if ( memcmp((const char*)bDesc->sysid, sElToritoID, strlen(sElToritoID))==0)
				aitp->ai->isoInfos.bBootable = true;	// anything else?
		}

		// check for udf 
		if (tempSec.descr_type==0x00 && tempSec.descr_ver==0x01 ) {

			if (memcmp(&tempSec.magic, sig_udf_bea, 5)==0 && iUdfDetectState==0)
				iUdfDetectState=1;// detected Beginning Extended Area Descriptor (BEA)
			
			else if (memcmp(&tempSec.magic, sig_udf_nsr2, 5)==0 && iUdfDetectState==1) // Volume Sequence Descriptor (VSD) 2
				iUdfDetectState=2;
			
			else if (memcmp(&tempSec.magic, sig_udf_nsr3, 5)==0 && iUdfDetectState==1) // Volume Sequence Descriptor (VSD) 3
				iUdfDetectState=3;
			
			else if (memcmp(&tempSec.magic, sig_tea, 5)==0 && (iUdfDetectState==2 || iUdfDetectState==3))
				iUdfDetectState+=2;	// remember Terminating Extended Area Descriptor (TEA) received
		}

	} while ( IsFilled(nextstart, nextstart + aitp->ai->isoInfos.secSize, filled ));

	if (iUdfDetectState==4)
		aitp->ai->isoInfos.type |= ISOtype_UDF_nsr02;
	else if (iUdfDetectState==5)
		aitp->ai->isoInfos.type |= ISOtype_UDF_nsr03;

	if (aitp->ai->isoInfos.type == 0)
		return false;

	if (iUdfDetectState==4 || iUdfDetectState==5)
	{
		// TODO: UDF handling  (http://www.osta.org/specs/)
		return false;	// no known udf version
	} 
		else 
	{
		// ISO9660/Joliet handling

		// read root directory of iso and recursive
		ISO_FileFolderEntry rootdir;
		memcpy(&rootdir, svd.descr_type!=0xff?svd.rootdir:pvd.rootdir, 33);

		ISOReadDirectory(aitp, LODWORD(rootdir.sector1OfExtension), isoInput, _T("") );
	}

	return true;
}

// ########## end of ISO handling #############


uint16 CArchiveRecovery::readUInt16(CFile *input)
{
	uint16 retVal = 0;
	BYTE b[2];
	if (input->Read(b, 2) > 0)
		retVal = (b[1] << 8) + b[0];
	return retVal;
}

uint32 CArchiveRecovery::readUInt32(CFile *input)
{
	uint32 retVal = 0;
	BYTE b[4];
	if (input->Read(b, 4) > 0)
		retVal = (b[3] << 24) + (b[2] << 16) + (b[1] << 8) + b[0];
	return retVal;
}

uint16 CArchiveRecovery::calcUInt16(BYTE *input)
{
	return (((uint16)input[1]) << 8) + ((uint16)input[0]);
}

uint32 CArchiveRecovery::calcUInt32(BYTE *input)
{
	return (((uint32)input[3]) << 24) + (((uint32)input[2]) << 16) + (((uint32)input[1]) << 8) + ((uint32)input[0]);
}

void CArchiveRecovery::writeUInt16(CFile *output, uint16 val)
{
	BYTE b[2];
	b[0] = (BYTE)(val & 0x000000ff);
	b[1] = (BYTE)((val & 0x0000ff00) >>  8);
	output->Write(b, 2);
}

void CArchiveRecovery::writeUInt32(CFile *output, uint32 val)
{
	BYTE b[4];
	b[0] = (BYTE)(val & 0x000000ff);
	b[1] = (BYTE)((val & 0x0000ff00) >>  8);
	b[2] = (BYTE)((val & 0x00ff0000) >> 16);
	b[3] = (BYTE)((val & 0xff000000) >> 24);
	output->Write(b, 4);
}

void CArchiveRecovery::ProcessProgress(archiveScannerThreadParams_s* aitp, UINT64 pos) {

	if (!aitp->m_bIsValid)
		return;

/*
	if (aitp->maxProgress==0) {
		
		if (aitp->file->IsPartFile())
			aitp->maxProgress=((CPartFile*)aitp->file)->GetCompletedSize();
		else
			aitp->maxProgress=aitp->file->GetFileSize();
		aitp->maxProgress=aitp->file->GetFileSize();
	}
*/

	int nNewProgress = 0;
	if ((uint64)aitp->file->GetFileSize() > 0)
		nNewProgress = (int)(uint64)((pos*1000) / aitp->file->GetFileSize());

	if (nNewProgress <= aitp->curProgress+1)
		return;

	aitp->curProgress=nNewProgress;
	
	SendMessage(aitp->progressHwnd, PBM_SETPOS, nNewProgress, 0);
}

