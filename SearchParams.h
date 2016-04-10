#pragma once

#include "SafeFile.h"

///////////////////////////////////////////////////////////////////////////////
// ESearchType

enum ESearchType
{
	//NOTE: The numbers are *equal* to the entries in the comboxbox -> TODO: use item data
	SearchTypeAutomatic = 0,
	SearchTypeEd2kServer,
	SearchTypeEd2kGlobal,
	SearchTypeKademlia,
	SearchTypeContentDB
};


#define	MAX_SEARCH_EXPRESSION_LEN	512

///////////////////////////////////////////////////////////////////////////////
// SSearchParams

struct SSearchParams
{
	SSearchParams()
	{
		dwSearchID = (DWORD)-1;
		eType = SearchTypeEd2kServer;
		bClientSharedFiles = false;
		ullMinSize = 0;
		ullMaxSize = 0;
		uAvailability = 0;
		uComplete = 0;
		ulMinBitrate = 0;
		ulMinLength = 0;
		bMatchKeywords = false;
	}

	SSearchParams(CFileDataIO& rFile)
	{
		dwSearchID = rFile.ReadUInt32();
		eType = (ESearchType)rFile.ReadUInt8();
		bClientSharedFiles = rFile.ReadUInt8() > 0;
		strSpecialTitle = rFile.ReadString(true);
		strExpression = rFile.ReadString(true);
		strFileType = rFile.ReadString(true);
		ullMinSize = 0;
		ullMaxSize = 0;
		uAvailability = 0;
		uComplete = 0;
		ulMinBitrate = 0;
		ulMinLength = 0;
		bMatchKeywords = false;
	}
	DWORD dwSearchID;
	bool bClientSharedFiles;
	CString strSearchTitle;
	CString strExpression;
	CString strKeyword;
	CString strBooleanExpr;
	ESearchType eType;
	CStringA strFileType;
	CString strMinSize;
	uint64 ullMinSize;
	CString strMaxSize;
	uint64 ullMaxSize;
	UINT uAvailability;
	CString strExtension;
	UINT uComplete;
	CString strCodec;
	ULONG ulMinBitrate;
	ULONG ulMinLength;
	CString strTitle;
	CString strAlbum;
	CString strArtist;
	CString strSpecialTitle;
	bool bMatchKeywords;

	void StorePartially(CFileDataIO& rFile) const
	{
		rFile.WriteUInt32(dwSearchID);
		rFile.WriteUInt8((uint8)eType);
		rFile.WriteUInt8(bClientSharedFiles ? 1 : 0);
		rFile.WriteString(strSpecialTitle, utf8strRaw);
		rFile.WriteString(strExpression, utf8strRaw);
		rFile.WriteString(CString(strFileType), utf8strRaw);
	}
};

bool GetSearchPacket(CSafeMemFile* data, SSearchParams* pParams, bool bTargetSupports64Bit, bool* pbPacketUsing64Bit);
