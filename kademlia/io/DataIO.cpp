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
#include <atlenc.h>
#include "./DataIO.h"
#include "./IOException.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Tag.h"
#include "../../resource.h"
#include "../../StringConversion.h"
#include "../../SafeFile.h"
#include "../../Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;

// This may look confusing that the normal methods use le() and the LE methods don't.
// The reason is the variables are stored in memory in little endian format already.

byte CDataIO::ReadByte()
{
	byte byRetVal;
	ReadArray(&byRetVal, 1);
	return byRetVal;
}

uint8 CDataIO::ReadUInt8()
{
	uint8 uRetVal;
	ReadArray(&uRetVal, sizeof(uint8));
	return uRetVal;
}

uint16 CDataIO::ReadUInt16()
{
	uint16 uRetVal;
	ReadArray(&uRetVal, sizeof(uint16));
	return uRetVal;
}

uint32 CDataIO::ReadUInt32()
{
	uint32 uRetVal;
	ReadArray(&uRetVal, sizeof(uint32));
	return uRetVal;
}

uint64 CDataIO::ReadUInt64()
{
	uint64 uRetVal;
	ReadArray(&uRetVal, sizeof(uint64));
	return uRetVal;
}

void CDataIO::ReadUInt128(CUInt128* puValue)
{
	ReadArray(puValue->GetDataPtr(), sizeof(uint32)*4);
}

float CDataIO::ReadFloat()
{
	float fRetVal;
	ReadArray(&fRetVal, sizeof(float));
	return fRetVal;
}

void CDataIO::ReadHash(BYTE* pbyValue)
{
	ReadArray(pbyValue, 16);
}

BYTE* CDataIO::ReadBsob(uint8* puSize)
{
	*puSize = ReadUInt8();
	if (GetAvailable() < *puSize)
		throw new CIOException(ERR_BUFFER_TOO_SMALL);
	BYTE* pbyBsob = new BYTE[*puSize];
	try
	{
		ReadArray(pbyBsob, *puSize);
	}
	catch(CException*)
	{
		delete[] pbyBsob;
		throw;
	}
	return pbyBsob;
}

CStringW CDataIO::ReadStringUTF8(bool bOptACP)
{
	UINT uRawSize = ReadUInt16();
	const UINT uMaxShortRawSize = SHORT_RAW_ED2K_UTF8_STR;
	if (uRawSize <= uMaxShortRawSize)
	{
		char acRaw[uMaxShortRawSize];
		ReadArray(acRaw, uRawSize);
		WCHAR awc[uMaxShortRawSize];
		int iChars = bOptACP
		             ? utf8towc(acRaw, uRawSize, awc, ARRSIZE(awc))
		             : ByteStreamToWideChar(acRaw, uRawSize, awc, ARRSIZE(awc));
		if (iChars >= 0)
			return CStringW(awc, iChars);
		return CStringW(acRaw, uRawSize); // use local codepage
	}
	else
	{
		Array<char> acRaw(uRawSize);
		ReadArray(acRaw, uRawSize);
		Array<WCHAR> awc(uRawSize);
		int iChars = bOptACP
		             ? utf8towc(acRaw, uRawSize, awc, uRawSize)
		             : ByteStreamToWideChar(acRaw, uRawSize, awc, uRawSize);
		if (iChars >= 0)
			return CStringW(awc, iChars);
		return CStringW(acRaw, uRawSize); // use local codepage
	}
}

CKadTag *CDataIO::ReadTag(bool bOptACP)
{
	CKadTag *pRetVal = NULL;
	char *pcName = NULL;
	byte byType = 0;
	uint16 uLenName = 0;
	try
	{
		byType = ReadByte();
		uLenName = ReadUInt16();
		pcName = new char[uLenName+1];
		pcName[uLenName] = 0;
		ReadArray(pcName, uLenName);

		switch (byType)
		{
				// NOTE: This tag data type is accepted and stored only to give us the possibility to upgrade
				// the net in some months.
				//
				// And still.. it doesnt't work this way without breaking backward compatibility. To properly
				// do this without messing up the network the following would have to be done:
				//	 -	those tag types have to be ignored by any client, otherwise those tags would also be sent (and
				//		that's really the problem)
				//
				//	 -	ignoring means, each client has to read and right throw away those tags, so those tags get
				//		get never stored in any tag list which might be sent by that client to some other client.
				//
				//	 -	all calling functions have to be changed to deal with the 'nr. of tags' attribute (which was
				//		already parsed) correctly.. just ignoring those tags here is not enough, any taglists have to
				//		be built with the knowledge that the 'nr. of tags' attribute may get decreased during the tag
				//		reading..
				//
				// If those new tags would just be stored and sent to remote clients, any malicious or just bugged
				// client could let send a lot of nodes "corrupted" packets...
				//
			case TAGTYPE_HASH:
				{
					BYTE byValue[16];
					ReadHash(byValue);
					pRetVal = new CKadTagHash(pcName, byValue);
					break;
				}

			case TAGTYPE_STRING:
				pRetVal = new CKadTagStr(pcName, ReadStringUTF8(bOptACP));
				break;

			case TAGTYPE_UINT64:
				pRetVal = new CKadTagUInt64(pcName, ReadUInt64());
				break;

			case TAGTYPE_UINT32:
				pRetVal = new CKadTagUInt32(pcName, ReadUInt32());
				break;

			case TAGTYPE_UINT16:
				pRetVal = new CKadTagUInt16(pcName, ReadUInt16());
				break;

			case TAGTYPE_UINT8:
				pRetVal = new CKadTagUInt8(pcName, ReadUInt8());
				break;

			case TAGTYPE_FLOAT32:
				pRetVal = new CKadTagFloat(pcName, ReadFloat());
				break;

				// NOTE: This tag data type is accepted and stored only to give us the possibility to upgrade
				// the net in some months.
				//
				// And still.. it doesnt't work this way without breaking backward compatibility
			case TAGTYPE_BSOB:
				{
					uint8 uSize;
					BYTE* pValue = ReadBsob(&uSize);
					try
					{
						pRetVal = new CKadTagBsob(pcName, pValue, uSize);
					}
					catch(CException*)
					{
						delete[] pValue;
						throw;
					}
					delete[] pValue;
					break;
				}

			default:
				throw new CNotSupportedException;
		}
		delete [] pcName;
		pcName = NULL;
	}
	catch (...)
	{
		DebugLogError(_T("Invalid Kad tag; type=0x%02x  lenName=%u  name=0x%02x"), byType, uLenName, pcName!=NULL ? (BYTE)pcName[0] : 0);
		delete[] pcName;
		delete pRetVal;
		throw;
	}
	return pRetVal;
}

void CDataIO::ReadTagList(TagList* pTaglist, bool bOptACP)
{
	uint32 uCount = ReadByte();
	for (uint32 i=0; i<uCount; i++)
	{
		CKadTag* pTag = ReadTag(bOptACP);
		pTaglist->push_back(pTag);
	}
}

void CDataIO::WriteByte(byte byVal)
{
	WriteArray(&byVal, 1);
}

void CDataIO::WriteUInt8(uint8 uVal)
{
	WriteArray(&uVal, sizeof(uint8));
}

void CDataIO::WriteUInt16(uint16 uVal)
{
	WriteArray(&uVal, sizeof(uint16));
}

void CDataIO::WriteUInt32(uint32 uVal)
{
	WriteArray(&uVal, sizeof(uint32));
}

void CDataIO::WriteUInt64(uint64 uVal)
{
	WriteArray(&uVal, sizeof(uint64));
}

void CDataIO::WriteUInt128(const CUInt128& uVal)
{
	WriteArray(uVal.GetData(), sizeof(uint32)*4);
}

void CDataIO::WriteFloat(float fVal)
{
	WriteArray(&fVal, sizeof(float));
}

void CDataIO::WriteHash(const BYTE* pbyValue)
{
	WriteArray(pbyValue, 16);
}

void CDataIO::WriteBsob(const BYTE* pbyValue, uint8 uSize)
{
	WriteUInt8(uSize);
	WriteArray(pbyValue, uSize);
}

void CDataIO::WriteTag(const CKadTag* pTag)
{
	try
	{
		uint8 uType;
		if (pTag->m_type == 0xFE)
		{
			if (pTag->GetInt() <= 0xFF)
				uType = TAGTYPE_UINT8;
			else if (pTag->GetInt() <= 0xFFFF)
				uType = TAGTYPE_UINT16;
			else if (pTag->GetInt() <= 0xFFFFFFFF)
				uType = TAGTYPE_UINT32;
			else
				uType = TAGTYPE_UINT64;
		}
		else
			uType = pTag->m_type;

		WriteByte(uType);

		const CKadTagNameString& name = pTag->m_name;
		WriteUInt16((uint16)name.GetLength());
		WriteArray((LPCSTR)name, name.GetLength());

		switch (uType)
		{
			case TAGTYPE_HASH:
				// Do NOT use this to transfer any tags for at least half a year!!
				WriteHash(pTag->GetHash());
				ASSERT(0);
				break;
			case TAGTYPE_STRING:
				{
					CUnicodeToUTF8 utf8(pTag->GetStr());
					WriteUInt16((uint16)utf8.GetLength());
					WriteArray(utf8, utf8.GetLength());
					break;
				}
			case TAGTYPE_UINT64:
				WriteUInt64(pTag->GetInt());
				break;
			case TAGTYPE_UINT32:
				WriteUInt32((uint32)pTag->GetInt());
				break;
			case TAGTYPE_UINT16:
				WriteUInt16((uint16)pTag->GetInt());
				break;
			case TAGTYPE_UINT8:
				WriteUInt8((uint8)pTag->GetInt());
				break;
			case TAGTYPE_FLOAT32:
				WriteFloat(pTag->GetFloat());
				break;
			case TAGTYPE_BSOB:
				WriteBsob(pTag->GetBsob(), pTag->GetBsobSize());
				break;
		}
	}
	catch (CIOException *ioe)
	{
		AddDebugLogLine( false, _T("Exception in CDataIO:writeTag (IO Error(%i))"), ioe->m_iCause);
		throw ioe;
	}
	catch (...)
	{
		AddDebugLogLine(false, _T("Exception in CDataIO:writeTag"));
		throw;
	}
}

void CDataIO::WriteTag(LPCSTR szName, uint64 uValue)
{
	CKadTagUInt64 tag(szName, uValue);
	WriteTag(&tag);
}

void CDataIO::WriteTag(LPCSTR szName, uint32 uValue)
{
	CKadTagUInt32 tag(szName, uValue);
	WriteTag(&tag);
}

void CDataIO::WriteTag(LPCSTR szName, uint16 uValue)
{
	CKadTagUInt16 tag(szName, uValue);
	WriteTag(&tag);
}

void CDataIO::WriteTag(LPCSTR szName, uint8 uValue)
{
	CKadTagUInt8 tag(szName, uValue);
	WriteTag(&tag);
}

void CDataIO::WriteTag(LPCSTR szName, float uValue)
{
	CKadTagFloat tag(szName, uValue);
	WriteTag(&tag);
}

void CDataIO::WriteTagList(const TagList& tagList)
{
	uint32 uCount = (uint32)tagList.size();
	ASSERT( uCount <= 0xFF );
	WriteByte((uint8)uCount);
	for (TagList::const_iterator itTagList = tagList.begin(); itTagList != tagList.end(); ++itTagList)
		WriteTag(*itTagList);
}

void CDataIO::WriteString(const CStringW& strVal){
	CUnicodeToUTF8 utf8(strVal);
	WriteUInt16((uint16)utf8.GetLength());
	WriteArray(utf8, utf8.GetLength());
}

namespace Kademlia
{
	void deleteTagListEntries(TagList* pTaglist)
	{
		for (TagList::const_iterator itTagList = pTaglist->begin(); itTagList != pTaglist->end(); ++itTagList)
			delete *itTagList;
		pTaglist->clear();
	}
}

static WCHAR s_awcLowerMap[0x10000];

bool CKademlia::InitUnicode(HMODULE hInst)
{
	bool bResult = false;
	HRSRC hResInfo = FindResource(hInst, MAKEINTRESOURCE(IDR_WIDECHARLOWERMAP), _T("WIDECHARMAP"));
	if (hResInfo)
	{
		HGLOBAL hRes = LoadResource(hInst, hResInfo);
		if (hRes)
		{
			LPBYTE pRes = (LPBYTE)LockResource(hRes);
			if (pRes)
			{
				if (SizeofResource(hInst, hResInfo) == sizeof s_awcLowerMap)
				{
					memcpy(s_awcLowerMap, pRes, sizeof s_awcLowerMap);
					if (s_awcLowerMap[L'A'] == L'a' && s_awcLowerMap[L'Z'] == L'z')
						bResult = true;
				}
				UnlockResource(hRes);
			}
			FreeResource(hRes);
		}
	}
	return bResult;
}

// This is the function which is used to pre-create the "WIDECHARMAP" resource for mapping
// Unicode characters to lowercase characters in a way which is *IDENTICAL* on each single
// node within the network. It is *MANDATORY* that each single Kad node is using the
// exactly same character mapping. Thus, this array is pre-created at compile time and *MUST*
// get used by each node in the network.
//
// However, the Unicode standard gets developed further during the years and new characters
// where added which also do have a corresponding lowercase version. So, that "WIDECHARMAP"
// should get updated at least with each new Windows version. Windows Vista indeed added
// around 200 new characters.
//
//void gen_wclwrtab()
//{
//	FILE* fpt = fopen("wclwrtab_gen.txt", "wb");
//	fputwc(0xFEFF, fpt);
//
//	int iDiffs = 0;
//	FILE* fp = fopen("wclwrtab.bin", "wb");
//	for (UINT ch = 0; ch < 0x10000; ch++)
//	{
//		WCHAR wch = (WCHAR)ch;
//		int iRes = LCMapString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), LCMAP_LOWERCASE, &wch, 1, &wch, 1);
//		ASSERT( iRes == 1);
//		if (wch != ch)
//		{
//			fwprintf(fpt, L"%04x != %04x: %c %c\r\n", ch, wch, ch, wch);
//			iDiffs++;
//		}
//		fwrite(&wch, sizeof(wch), 1, fp);
//	}
//	fclose(fp);
//
//	fclose(fpt);
//	printf("Diffs=%u\n", iDiffs);
//}
//
//void use_wclwrtab(const char *fname)
//{
//	size_t uMapSize = sizeof(wchar_t) * 0x10000;
//	wchar_t *pLowerMap = (wchar_t *)malloc(uMapSize);
//	FILE* fp = fopen(fname, "rb");
//	if (!fp) {
//		perror(fname);
//		exit(1);
//	}
//	fread(pLowerMap, uMapSize, 1, fp);
//	fclose(fp);
//	fp = NULL;
//
//	FILE* fpt = fopen("wclwrtab_use.txt", "wb");
//	fputwc(0xFEFF, fpt);
//	int iDiffs = 0;
//	for (UINT ch = 0; ch < 0x10000; ch++)
//	{
//		WCHAR wch = ch;
//		wch = pLowerMap[wch];
//		if (wch != ch)
//		{
//			fwprintf(fpt, L"%04x != %04x: %c %c\r\n", ch, wch, ch, wch);
//			iDiffs++;
//		}
//	}
//	fclose(fpt);
//	free(pLowerMap);
//	printf("Diffs=%u\n", iDiffs);
//}

void KadTagStrMakeLower(CKadTagValueString& rwstr)
{
	// NOTE: We can *not* use any locale dependant string functions here. All clients in the network have to
	// use the same character mapping whereby it actually does not matter if they 'understand' the strings
	// or not -- they just have to use the same mapping. That's why we hardcode to 'LANG_ENGLISH' here!
	// Note also, using 'LANG_ENGLISH' is not the same as using the "C" locale. The "C" locale would only
	// handle ASCII-7 characters while the 'LANG_ENGLISH' locale also handles chars from 0x80-0xFF and more.
	//rwstr.MakeLower();

#if 0
	//PROBLEM: LCMapStringW does not work on Win9x (the string is not changed and LCMapStringW returns 0!)
	// Possible solution: use a pre-computed static character map..
	int iLen = rwstr.GetLength();
	LPWSTR pwsz = rwstr.GetBuffer(iLen);
	int iSize = LCMapStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
	                         LCMAP_LOWERCASE, pwsz, -1, pwsz, iLen + 1);
	ASSERT( iSize - 1 == iLen );
	rwstr.ReleaseBuffer(iLen);
#else
	// NOTE: It's very important that the Unicode->LowerCase map already was initialized!
	if (s_awcLowerMap[L'A'] != L'a')
	{
		AfxMessageBox(_T("Kad Unicode lowercase character map not initialized!"));
		exit(1);
	}

	int iLen = rwstr.GetLength();
	LPWSTR pwsz = rwstr.GetBuffer(iLen);
	while ((*pwsz = s_awcLowerMap[*pwsz]) != L'\0')
		pwsz++;
	rwstr.ReleaseBuffer(iLen);
#endif
}

int KadTagStrCompareNoCase(LPCWSTR dst, LPCWSTR src)
{
	// NOTE: It's very important that the Unicode->LowerCase map already was initialized!
	if (s_awcLowerMap[L'A'] != L'a')
	{
		AfxMessageBox(_T("Kad Unicode lowercase character map not initialized!"));
		exit(1);
	}

    WCHAR d, s;
	do {
        d = s_awcLowerMap[*dst++];
        s = s_awcLowerMap[*src++];
    }
	while (d != L'\0' && (d == s));
    return (int)(d - s);
}
