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
#include <zlib/zlib.h>
#include "Packets.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "StringConversion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#pragma pack(1)
struct Header_Struct{
	uint8	eDonkeyID;
	uint32	packetlength;
	uint8	command;
};
#pragma pack()

#pragma pack(1)
struct UDP_Header_Struct{
	uint8	eDonkeyID;
	uint8	command;
};
#pragma pack()


Packet::Packet(uint8 protocol){
	m_bSplitted = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	size = 0;
	pBuffer = 0;
	completebuffer = 0;
	tempbuffer = 0;
	opcode = 0x00;
	prot = protocol;
	m_bPacked = false;
	uStatsPayLoad = 0;
}

Packet::Packet(char* header){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	tempbuffer = 0;
	pBuffer = 0;
	completebuffer = 0;
	Header_Struct* head = (Header_Struct*) header;
	size = head->packetlength-1;
	opcode = head->command;
	prot = head->eDonkeyID;
	uStatsPayLoad = 0;
}

Packet::Packet(char* pPacketPart, uint32 nSize, bool bLast, bool bFromPartFile){// only used for splitted packets!
	m_bFromPF = bFromPartFile;
	m_bSplitted = true;
	m_bPacked = false;
	m_bLastSplitted = bLast;
	tempbuffer = 0;
	pBuffer = 0;
	completebuffer = pPacketPart;
	size = nSize-6;
	opcode = 0x00;
	prot = 0x00;
	uStatsPayLoad = 0;
}

Packet::Packet(uint8 in_opcode, uint32 in_size, uint8 protocol, bool bFromPartFile){
	m_bFromPF = bFromPartFile;
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	tempbuffer = 0;
	if (in_size){
		completebuffer = new char[in_size+10];
		pBuffer = completebuffer+6;
		memset(completebuffer,0,in_size+10);
	}
	else{
		pBuffer = 0;
		completebuffer = 0;
	}
	opcode = in_opcode;
	size = in_size;
	prot = protocol;
	uStatsPayLoad = 0;
}

Packet::Packet(CMemFile* datafile, uint8 protocol, uint8 ucOpcode){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	size = (UINT)datafile->GetLength();
	completebuffer = new char[(UINT)datafile->GetLength() + 10];
	pBuffer = completebuffer+6;
	BYTE* tmp = datafile->Detach();
	memcpy(pBuffer,tmp,size);
	free(tmp);
	tempbuffer = 0;
	opcode = ucOpcode;
	prot = protocol;
	uStatsPayLoad = 0;
}

Packet::Packet(const CStringA& str, uint8 ucProtocol, uint8 ucOpcode){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	size = str.GetLength();
	completebuffer = new char[size+10];
	pBuffer = completebuffer+6;
	memcpy(pBuffer,(LPCSTR)str,size);
	tempbuffer = 0;
	opcode = ucOpcode;
	prot = ucProtocol;
	uStatsPayLoad = 0;
}

Packet::~Packet(){
	if (completebuffer)
		delete[] completebuffer;
	else
		delete[] pBuffer;
	delete[] tempbuffer;
}

char* Packet::GetPacket(){
	if (completebuffer){
		if (!m_bSplitted)
			memcpy(completebuffer,GetHeader(),6);
		return completebuffer;
	}
	else{
		delete[] tempbuffer;
		tempbuffer = NULL; // 'new' may throw an exception
		tempbuffer = new char[size+10];
		memcpy(tempbuffer,GetHeader(),6);
		memcpy(tempbuffer+6,pBuffer,size);
		return tempbuffer;
	}
}

char* Packet::DetachPacket(){
	if (completebuffer){
		if (!m_bSplitted)
			memcpy(completebuffer,GetHeader(),6);
		char* result = completebuffer;
		completebuffer = 0;
		pBuffer = 0;
		return result;
	}
	else{
		delete[] tempbuffer;
		tempbuffer = NULL; // 'new' may throw an exception
		tempbuffer = new char[size+10];
		memcpy(tempbuffer,GetHeader(),6);
		memcpy(tempbuffer+6,pBuffer,size);
		char* result = tempbuffer;
		tempbuffer = 0;
		return result;
	}
}

char* Packet::GetHeader(){
	ASSERT ( !m_bSplitted );
	Header_Struct* header = (Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;
	header->packetlength = size+1;
	return head;
}

char* Packet::GetUDPHeader(){
	ASSERT ( !m_bSplitted );
	UDP_Header_Struct* header = (UDP_Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;
	return head;
}

void Packet::PackPacket(){
	ASSERT (!m_bSplitted);
	uLongf newsize = size+300;
	BYTE* output = new BYTE[newsize];
	UINT result = compress2(output,&newsize,(BYTE*)pBuffer,size,Z_BEST_COMPRESSION);
	if (result != Z_OK || size <= newsize){
		delete[] output;
		return;
	}
	if( prot == OP_KADEMLIAHEADER )
		prot = OP_KADEMLIAPACKEDPROT;
	else
		prot = OP_PACKEDPROT;
	memcpy(pBuffer,output,newsize);
	size = newsize;
	delete[] output;
	m_bPacked = true;
}

bool Packet::UnPackPacket(UINT uMaxDecompressedSize){
	ASSERT ( prot == OP_PACKEDPROT || prot == OP_KADEMLIAPACKEDPROT); 
	uint32 nNewSize = size*10+300;
	if (nNewSize > uMaxDecompressedSize){
		//ASSERT(0);
		nNewSize = uMaxDecompressedSize;
	}
	BYTE* unpack = NULL;
	uLongf unpackedsize = 0;
	UINT result = 0;
	do {
		delete[] unpack;
		unpack = new BYTE[nNewSize];
		unpackedsize = nNewSize;
		result = uncompress(unpack,&unpackedsize,(BYTE*)pBuffer,size);
		nNewSize *= 2; // size for the next try if needed
	} while (result == Z_BUF_ERROR && nNewSize < uMaxDecompressedSize);
	
	if (result == Z_OK){
		ASSERT ( completebuffer == NULL );
		ASSERT ( pBuffer != NULL );
		size = unpackedsize;
		delete[] pBuffer;
		pBuffer = (char*)unpack;
		if( prot == OP_KADEMLIAPACKEDPROT )
			prot = OP_KADEMLIAHEADER;
		else
			prot =  OP_EMULEPROT;
		return true;
	}
	delete[] unpack;
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// CRawPacket

CRawPacket::CRawPacket(const CStringA& rstr)
{
	ASSERT( opcode == 0 );
	ASSERT( !m_bSplitted );
	ASSERT( !m_bLastSplitted );
	ASSERT( !m_bPacked );
	ASSERT( !m_bFromPF );
	ASSERT( completebuffer == NULL );
	ASSERT( tempbuffer == NULL );

	prot = 0x00;
	size = rstr.GetLength();
	pBuffer = new char[size];
	memcpy(pBuffer, (LPCSTR)rstr, size);
}

CRawPacket::CRawPacket(const char* pcData, UINT uSize, bool bFromPartFile)
{
	ASSERT( opcode == 0 );
	ASSERT( !m_bSplitted );
	ASSERT( !m_bLastSplitted );
	ASSERT( !m_bPacked );
	ASSERT( !m_bFromPF );
	ASSERT( completebuffer == NULL );
	ASSERT( tempbuffer == NULL );

	prot = 0x00;
	size = uSize;
	pBuffer = new char[size];
	memcpy(pBuffer, pcData, size);
	m_bFromPF = bFromPartFile;
}

CRawPacket::~CRawPacket()
{
	ASSERT( completebuffer == NULL );
}

char* CRawPacket::GetHeader()
{
	ASSERT(0);
	return NULL;
}

char* CRawPacket::GetUDPHeader()
{
	ASSERT(0);
	return NULL;
}

void CRawPacket::AttachPacket(char* pPacketData, UINT uPacketSize, bool bFromPartFile)
{
	ASSERT( pBuffer == NULL );
	pBuffer = pPacketData;
	size = uPacketSize;
	m_bFromPF = bFromPartFile;
}

char* CRawPacket::DetachPacket()
{
	char* pResult = pBuffer;
	pBuffer = NULL;
	return pResult;
}


///////////////////////////////////////////////////////////////////////////////
// CTag

CTag::CTag(LPCSTR pszName, uint64 uVal, bool bInt64)
{
	ASSERT( uVal <= 0xFFFFFFFF || bInt64 );
	if (bInt64){
		m_uType = TAGTYPE_UINT64;
	}
	else{
		m_uType = TAGTYPE_UINT32;
	}
	m_uVal = uVal;
	m_uName = 0;
	m_pszName = nstrdup(pszName);
	m_nBlobSize = 0;
	ASSERT_VALID(this);
}

CTag::CTag(uint8 uName, uint64 uVal, bool bInt64)
{
	ASSERT( uVal <= 0xFFFFFFFF || bInt64 );
	if (bInt64){
		m_uType = TAGTYPE_UINT64;
	}
	else{
		m_uType = TAGTYPE_UINT32;
	}
	m_uVal = uVal;
	m_uName = uName;
	m_pszName = NULL;
	m_nBlobSize = 0;
	ASSERT_VALID(this);
}

CTag::CTag(LPCSTR pszName, LPCTSTR pszVal)
{
	m_uType = TAGTYPE_STRING;
	m_uName = 0;
	m_pszName = nstrdup(pszName);
	m_pstrVal = new CString(pszVal);
	m_nBlobSize = 0;
	ASSERT_VALID(this);
}

CTag::CTag(uint8 uName, LPCTSTR pszVal)
{
	m_uType = TAGTYPE_STRING;
	m_uName = uName;
	m_pszName = NULL;
	m_pstrVal = new CString(pszVal);
	m_nBlobSize = 0;
	ASSERT_VALID(this);
}

CTag::CTag(LPCSTR pszName, const CString& rstrVal)
{
	m_uType = TAGTYPE_STRING;
	m_uName = 0;
	m_pszName = nstrdup(pszName);
	m_pstrVal = new CString(rstrVal);
	m_nBlobSize = 0;
	ASSERT_VALID(this);
}

CTag::CTag(uint8 uName, const CString& rstrVal)
{
	m_uType = TAGTYPE_STRING;
	m_uName = uName;
	m_pszName = NULL;
	m_pstrVal = new CString(rstrVal);
	m_nBlobSize = 0;
	ASSERT_VALID(this);
}

CTag::CTag(uint8 uName, const BYTE* pucHash)
{
	m_uType = TAGTYPE_HASH;
	m_uName = uName;
	m_pszName = NULL;
	m_pData = new BYTE[16];
	md4cpy(m_pData, pucHash);
	m_nBlobSize = 0;
	ASSERT_VALID(this);
}

CTag::CTag(uint8 uName, uint32 nSize, const BYTE* pucData){
	m_uType = TAGTYPE_BLOB;
	m_uName = uName;
	m_pszName = NULL;
	m_pData = new BYTE[nSize];
	memcpy(m_pData, pucData, nSize);
	m_nBlobSize = nSize;
	ASSERT_VALID(this);
}

CTag::CTag(uint8 uName, BYTE* pucAttachData, uint32 nSize)
{
	m_uType = TAGTYPE_BLOB;
	m_uName = uName;
	m_pszName = NULL;
	m_pData = pucAttachData;
	m_nBlobSize = nSize;
	ASSERT_VALID(this);
}

CTag::CTag(const CTag& rTag)
{
	m_uType = rTag.m_uType;
	m_uName = rTag.m_uName;
	m_pszName = rTag.m_pszName!=NULL ? nstrdup(rTag.m_pszName) : NULL;
	m_nBlobSize = 0;
	if (rTag.IsStr())
		m_pstrVal = new CString(rTag.GetStr());
	else if (rTag.IsInt())
		m_uVal = rTag.GetInt();
	else if (rTag.IsInt64(false))
		m_uVal = rTag.GetInt64();
	else if (rTag.IsFloat())
		m_fVal = rTag.GetFloat();
	else if (rTag.IsHash()){
		m_pData = new BYTE[16];
		md4cpy(m_pData, rTag.GetHash());
	}
	else if (rTag.IsBlob()){
		m_nBlobSize = rTag.GetBlobSize();
		m_pData = new BYTE[rTag.GetBlobSize()];
		memcpy(m_pData, rTag.GetBlob(), rTag.GetBlobSize());
	}
	else{
		ASSERT(0);
		m_uVal = 0;
	}
	ASSERT_VALID(this);
}

CTag::CTag(CFileDataIO* data, bool bOptUTF8)
{
	m_uType = data->ReadUInt8();
	if (m_uType & 0x80)
	{
		m_uType &= 0x7F;
		m_uName = data->ReadUInt8();
		m_pszName = NULL;
	}
	else
	{
		UINT length = data->ReadUInt16();
		if (length == 1)
		{
			m_uName = data->ReadUInt8();
			m_pszName = NULL;
		}
		else
		{
			m_uName = 0;
			m_pszName = new char[length+1];
			try{
				data->Read(m_pszName, length);
			}
			catch(CFileException* ex){
				delete[] m_pszName;
				throw ex;
			}
			m_pszName[length] = '\0';
		}
	}
	
	m_nBlobSize = 0;

	// NOTE: It's very important that we read the *entire* packet data, even if we do
	// not use each tag. Otherwise we will get troubles when the packets are returned in 
	// a list - like the search results from a server.
	if (m_uType == TAGTYPE_STRING)
	{
		m_pstrVal = new CString(data->ReadString(bOptUTF8));
	}
	else if (m_uType == TAGTYPE_UINT32)
	{
		m_uVal = data->ReadUInt32();
	}
	else if (m_uType == TAGTYPE_UINT64)
	{
		m_uVal = data->ReadUInt64();
	}
	else if (m_uType == TAGTYPE_UINT16)
	{
		m_uVal = data->ReadUInt16();
		m_uType = TAGTYPE_UINT32;
	}
	else if (m_uType == TAGTYPE_UINT8)
	{
		m_uVal = data->ReadUInt8();
		m_uType = TAGTYPE_UINT32;
	}
	else if (m_uType == TAGTYPE_FLOAT32)
	{
		data->Read(&m_fVal, 4);
	}
	else if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16)
	{
		UINT length = m_uType - TAGTYPE_STR1 + 1;
		m_pstrVal = new CString(data->ReadString(bOptUTF8, length));
		m_uType = TAGTYPE_STRING;
	}
	else if (m_uType == TAGTYPE_HASH)
	{
		m_pData = new BYTE[16];
		try{
			data->Read(m_pData, 16);
		}
		catch(CFileException* ex){
			delete[] m_pData;
			throw ex;
		}
	}
	else if (m_uType == TAGTYPE_BOOL)
	{
		TRACE("***NOTE: %s; Reading BOOL tag\n", __FUNCTION__);
		data->Seek(1, CFile::current);
	}
	else if (m_uType == TAGTYPE_BOOLARRAY)
	{
		TRACE("***NOTE: %s; Reading BOOL Array tag\n", __FUNCTION__);
		uint16 len;
		data->Read(&len, 2);
		// 07-Apr-2004: eMule versions prior to 0.42e.29 used the formula "(len+7)/8"!
		data->Seek((len/8)+1, CFile::current);
	}
	else if (m_uType == TAGTYPE_BLOB)
	{
		// 07-Apr-2004: eMule versions prior to 0.42e.29 handled the "len" as int16!
		m_nBlobSize = data->ReadUInt32();
		if (m_nBlobSize <= data->GetLength() - data->GetPosition()){
			m_pData = new BYTE[m_nBlobSize];
			data->Read(m_pData, m_nBlobSize);
		}
		else{
			ASSERT( false );
			m_nBlobSize = 0;
			m_pData = NULL;
		}
	}
	else
	{
		if (m_uName != 0)
			TRACE("%s; Unknown tag: type=0x%02X  specialtag=%u\n", __FUNCTION__, m_uType, m_uName);
		else
			TRACE("%s; Unknown tag: type=0x%02X  name=\"%s\"\n", __FUNCTION__, m_uType, m_pszName);
		m_uVal = 0;
	}
	ASSERT_VALID(this);
}

CTag::~CTag()
{
	delete[] m_pszName;
	if (IsStr())
		delete m_pstrVal;
	else if (IsHash())
		delete[] m_pData;
	else if (IsBlob())
		delete[] m_pData;
}

bool CTag::WriteNewEd2kTag(CFileDataIO* data, EUtf8Str eStrEncode) const
{
	ASSERT_VALID(this);

	// Write tag type
	uint8 uType;
	UINT uStrValLen = 0;
	LPCSTR pszValA = NULL;
	CStringA* pstrValA = NULL;
	if (IsInt())
	{
		if (m_uVal <= 0xFF)
			uType = TAGTYPE_UINT8;
		else if (m_uVal <= 0xFFFF)
			uType = TAGTYPE_UINT16;
		else
			uType = TAGTYPE_UINT32;
	}
	else if (IsInt64(false))
	{
		if (m_uVal <= 0xFF)
			uType = TAGTYPE_UINT8;
		else if (m_uVal <= 0xFFFF)
			uType = TAGTYPE_UINT16;
		else if (m_uVal <= 0xFFFFFFFF)
			uType = TAGTYPE_UINT32;
		else
			uType = TAGTYPE_UINT64;
	}
	else if (IsStr())
	{
		if (eStrEncode == utf8strRaw)
		{
			CUnicodeToUTF8 utf8(*m_pstrVal);
			pstrValA = new CStringA((LPCSTR)utf8, utf8.GetLength());
		}
		else if (eStrEncode == utf8strOptBOM)
		{
			if (NeedUTF8String(*m_pstrVal))
			{
				CUnicodeToBOMUTF8 bomutf8(*m_pstrVal);
				pstrValA = new CStringA((LPCSTR)bomutf8, bomutf8.GetLength());
			}
			else
			{
				CUnicodeToMultiByte mb(*m_pstrVal);
				pstrValA = new CStringA((LPCSTR)mb, mb.GetLength());
			}
		}
		else
		{
			CUnicodeToMultiByte mb(*m_pstrVal);
			pstrValA = new CStringA((LPCSTR)mb, mb.GetLength());
		}
		uStrValLen = pstrValA->GetLength();
		pszValA = *pstrValA;
		if (uStrValLen >= 1 && uStrValLen <= 16)
			uType = (uint8)(TAGTYPE_STR1 + uStrValLen - 1);
		else
			uType = TAGTYPE_STRING;
	}
	else
		uType = m_uType;

	// Write tag name
	if (m_pszName)
	{
		data->WriteUInt8(uType);
		UINT uTagNameLen = strlen(m_pszName);
		data->WriteUInt16((uint16)uTagNameLen);
		data->Write(m_pszName, uTagNameLen);
	}
	else
	{
		ASSERT( m_uName != 0 );
		data->WriteUInt8(uType | 0x80);
		data->WriteUInt8(m_uName);
	}

	// Write tag data
	if (uType == TAGTYPE_STRING)
	{
		data->WriteUInt16((uint16)uStrValLen);
		data->Write(pszValA, uStrValLen);
	}
	else if (uType >= TAGTYPE_STR1 && uType <= TAGTYPE_STR16)
	{
		data->Write(pszValA, uStrValLen);
	}
	else if (uType == TAGTYPE_UINT64)
	{
		data->WriteUInt64(m_uVal);
	}
	else if (uType == TAGTYPE_UINT32)
	{
		data->WriteUInt32((uint32)m_uVal);
	}
	else if (uType == TAGTYPE_UINT16)
	{
		data->WriteUInt16((uint16)m_uVal);
	}
	else if (uType == TAGTYPE_UINT8)
	{
		data->WriteUInt8((uint8)m_uVal);
	}
	else if (uType == TAGTYPE_FLOAT32)
	{
		data->Write(&m_fVal, 4);
	}
	else if (uType == TAGTYPE_HASH)
	{
		data->WriteHash16(m_pData);
	}
	else if (uType == TAGTYPE_BLOB)
	{
		data->WriteUInt32(m_nBlobSize);
		data->Write(m_pData, m_nBlobSize);
	}
	else
	{
		TRACE("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, uType);
		ASSERT(0);
		return false;
	}

	delete pstrValA;
	return true;
}

bool CTag::WriteTagToFile(CFileDataIO* file, EUtf8Str eStrEncode) const
{
	ASSERT_VALID(this);

	// don't write tags of unknown types, we wouldn't be able to read them in again 
	// and the met file would be corrupted
	if (IsStr() || IsInt() || IsFloat() || IsBlob() || IsInt64())
	{
		file->WriteUInt8(m_uType);
		
		if (m_pszName)
		{
			UINT taglen = strlen(m_pszName);
			file->WriteUInt16((uint16)taglen);
			file->Write(m_pszName, taglen);
		}
		else
		{
			file->WriteUInt16(1);
			file->WriteUInt8(m_uName);
		}

		if (IsStr())
		{
			file->WriteString(GetStr(), eStrEncode);
		}
		else if (IsInt())
		{
			file->WriteUInt32((uint32)m_uVal);
		}
		else if (IsInt64(false))
		{
			file->WriteUInt64(m_uVal);
		}
		else if (IsFloat())
		{
			file->Write(&m_fVal, 4);
		}
		else if (IsBlob())
		{
			// NOTE: This will break backward compatibility with met files for eMule versions prior to 0.44a
			file->WriteUInt32(m_nBlobSize);
			file->Write(m_pData, m_nBlobSize);
		}
		//TODO: Support more tag types
		else
		{
			TRACE("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, m_uType);
			ASSERT(0);
			return false;
		}
		return true;
	}
	else
	{
		TRACE("%s; Ignored tag with unknown type=0x%02X\n", __FUNCTION__, m_uType);
		ASSERT(0);
		return false;
	}
}

void CTag::SetInt(uint32 uVal)
{
	ASSERT( IsInt() );
	if (IsInt())
		m_uVal = uVal;
}

void CTag::SetInt64(uint64 uVal)
{
	ASSERT( IsInt64(true) );
	if (IsInt64(true)){
		m_uVal = uVal;
		m_uType = TAGTYPE_UINT64;
	}
}

void CTag::SetStr(LPCTSTR pszVal)
{
	ASSERT( IsStr() );
	if (IsStr())
	{
		delete m_pstrVal;
		m_pstrVal = NULL;
		m_pstrVal = new CString(pszVal);
	}
}

CString CTag::GetFullInfo(CString (*pfnDbgGetFileMetaTagName)(UINT uMetaTagID)) const
{
	CString strTag;
	if (m_pszName)
	{
		strTag = _T('\"');
		strTag += m_pszName;
		strTag += _T('\"');
	}
	else
	{
		if (pfnDbgGetFileMetaTagName)
			strTag.Format(_T("\"%s\""), (*pfnDbgGetFileMetaTagName)(m_uName));
		else
			strTag.Format(_T("Tag0x%02X"), m_uName);
	}
	strTag += _T("=");
	if (m_uType == TAGTYPE_STRING)
	{
		strTag += _T("\"");
		strTag += *m_pstrVal;
		strTag += _T("\"");
	}
	else if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16)
	{
		strTag.AppendFormat(_T("(Str%u)\"%s\""), m_uType - TAGTYPE_STR1 + 1, *m_pstrVal);
	}
	else if (m_uType == TAGTYPE_UINT32)
	{
		strTag.AppendFormat(_T("(Int32)%u"), (uint32)m_uVal);
	}
	else if (m_uType == TAGTYPE_UINT64)
	{
		strTag.AppendFormat(_T("(Int64)%I64u"), m_uVal);
	}
	else if (m_uType == TAGTYPE_UINT16)
	{
		strTag.AppendFormat(_T("(Int16)%u"), m_uVal);
	}
	else if (m_uType == TAGTYPE_UINT8)
	{
		strTag.AppendFormat(_T("(Int8)%u"), m_uVal);
	}
	else if (m_uType == TAGTYPE_FLOAT32)
	{
		strTag.AppendFormat(_T("(Float32)%f"), m_fVal);
	}
	else if (m_uType == TAGTYPE_BLOB)
	{
		strTag.AppendFormat(_T("(Blob)%u"), m_nBlobSize);
	}
	else
	{
		strTag.AppendFormat(_T("Type=%u"), m_uType);
	}
	return strTag;
}

#ifdef _DEBUG
void CTag::AssertValid() const
{
	CObject::AssertValid();

	ASSERT( m_uType != 0 );
	ASSERT( m_uName != 0 && m_pszName == NULL || m_uName == 0 && m_pszName != NULL );
	ASSERT( m_pszName == NULL || AfxIsValidString(m_pszName) );
	if (IsStr())
		ASSERT( m_pstrVal != NULL && AfxIsValidString(*m_pstrVal) );
	else if (IsHash())
		ASSERT( m_pData != NULL && AfxIsValidAddress(m_pData, 16) );
	else if (IsBlob())
		ASSERT( m_pData != NULL && AfxIsValidAddress(m_pData, m_nBlobSize) );
}

void CTag::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
}
#endif
