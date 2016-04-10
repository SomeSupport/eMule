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
#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#include <crypto51/osrng.h>
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#include "./UInt128.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;
using namespace CryptoPP;

CUInt128::CUInt128()
{
	SetValue((ULONG)0);
}

CUInt128::CUInt128(bool bFill)
{
	if( bFill )
	{
		m_uData[0] = (ULONG)-1;
		m_uData[1] = (ULONG)-1;
		m_uData[2] = (ULONG)-1;
		m_uData[3] = (ULONG)-1;
	}
	else
		SetValue((ULONG)0);
}

CUInt128::CUInt128(ULONG uValue)
{
	SetValue(uValue);
}

CUInt128::CUInt128(const byte *pbyValueBE)
{
	SetValueBE(pbyValueBE);
}

CUInt128::CUInt128(const CUInt128 &uValue, UINT uNumBits)
{
	// Copy the whole ULONGs
	UINT uNumULONGs = uNumBits / 32;
	for (UINT iIndex=0; iIndex<uNumULONGs; iIndex++)
		m_uData[iIndex] = uValue.m_uData[iIndex];

	// Copy the remaining bits
	for (UINT iIndex=(32*uNumULONGs); iIndex<uNumBits; iIndex++)
		SetBitNumber(iIndex, uValue.GetBitNumber(iIndex));

	// Pad with random bytes (Not seeding based on time to allow multiple different ones to be created in quick succession)
	for (UINT iIndex=uNumBits; iIndex<128; iIndex++)
		SetBitNumber(iIndex, (rand()%2));
}

CUInt128& CUInt128::SetValue(const CUInt128 &uValue)
{
	m_uData[0] = uValue.m_uData[0];
	m_uData[1] = uValue.m_uData[1];
	m_uData[2] = uValue.m_uData[2];
	m_uData[3] = uValue.m_uData[3];
	return *this;
}

CUInt128& CUInt128::SetValue(ULONG uValue)
{
	m_uData[0] = 0;
	m_uData[1] = 0;
	m_uData[2] = 0;
	m_uData[3] = uValue;
	return *this;
}

CUInt128& CUInt128::SetValueBE(const byte *pbyValueBE)
{
	SetValue((ULONG)0);
	for (int iIndex=0; iIndex<16; iIndex++)
		m_uData[iIndex/4] |= ((ULONG)pbyValueBE[iIndex]) << (8*(3-(iIndex%4)));
	return *this;
}

CUInt128& CUInt128::SetValueRandom()
{
	AutoSeededRandomPool rng;
	byte byRandomBytes[16];
	rng.GenerateBlock(byRandomBytes, 16);
	SetValueBE( byRandomBytes );
	return *this;
}

CUInt128& CUInt128::SetValueGUID()
{
	SetValue((ULONG)0);
	GUID guid;
	if (CoCreateGuid(&guid) != S_OK)
		return *this;
	m_uData[0] = guid.Data1;
	m_uData[1] = ((ULONG)guid.Data2) << 16 | guid.Data3;
	m_uData[2] = ((ULONG)guid.Data4[0]) << 24 | ((ULONG)guid.Data4[1]) << 16 | ((ULONG)guid.Data4[2]) << 8 | ((ULONG)guid.Data4[3]);
	m_uData[3] = ((ULONG)guid.Data4[4]) << 24 | ((ULONG)guid.Data4[5]) << 16 | ((ULONG)guid.Data4[6]) << 8 | ((ULONG)guid.Data4[7]);
	return *this;
}

UINT CUInt128::GetBitNumber(UINT uBit) const
{
	if (uBit > 127)
		return 0;
	int iLongNum = uBit / 32;
	int iShift = 31 - (uBit % 32);
	return ((m_uData[iLongNum] >> iShift) & 1);
}

CUInt128& CUInt128::SetBitNumber(UINT uBit, UINT uValue)
{
	int iLongNum = uBit / 32;
	int iShift = 31 - (uBit % 32);
	m_uData[iLongNum] |= (1 << iShift);
	if (uValue == 0)
		m_uData[iLongNum] ^= (1 << iShift);
	return *this;
}

CUInt128& CUInt128::Xor(const CUInt128 &uValue)
{
	for (int iIndex=0; iIndex<4; iIndex++)
		m_uData[iIndex] ^= uValue.m_uData[iIndex];
	return *this;
}

CUInt128& CUInt128::XorBE(const byte *pbyValueBE)
{
	return Xor(CUInt128(pbyValueBE));
}

void CUInt128::ToHexString(CString *pstr) const
{
	pstr->SetString(_T(""));
	CString sElement;
	for (int iIndex=0; iIndex<4; iIndex++)
	{
		sElement.Format(_T("%08X"), m_uData[iIndex]);
		pstr->Append(sElement);
	}
}

CString CUInt128::ToHexString() const
{
	CString pstr;
	CString sElement;
	for (int iIndex=0; iIndex<4; iIndex++)
	{
		sElement.Format(_T("%08X"), m_uData[iIndex]);
		pstr.Append(sElement);
	}
	return pstr;
}

void CUInt128::ToBinaryString(CString *pstr, bool bTrim) const
{
	pstr->SetString(_T(""));
	CString sElement;
	int iBit;
	for (int iIndex=0; iIndex<128; iIndex++)
	{
		iBit = GetBitNumber(iIndex);
		if ((!bTrim) || (iBit != 0))
		{
			sElement.Format(_T("%d"), iBit);
			pstr->Append(sElement);
			bTrim = false;
		}
	}
	if (pstr->GetLength() == 0)
		pstr->SetString(_T("0"));
}

#if defined(_M_IX86) && (_MSC_FULL_VER > 13009037)
#pragma intrinsic(_byteswap_ulong)
#endif
void CUInt128::ToByteArray(byte *pby) const
{
#if defined(_M_IX86) && (_MSC_FULL_VER > 13009037)
	((uint32*)pby)[0] = _byteswap_ulong(m_uData[0]);
	((uint32*)pby)[1] = _byteswap_ulong(m_uData[1]);
	((uint32*)pby)[2] = _byteswap_ulong(m_uData[2]);
	((uint32*)pby)[3] = _byteswap_ulong(m_uData[3]);
#else

	for (int iIndex=0; iIndex<16; iIndex++)
		pby[iIndex] = (byte)(m_uData[iIndex/4] >> (8*(3-(iIndex%4))));
#endif
}

int CUInt128::CompareTo(const CUInt128 &uOther) const
{
	for (int iIndex=0; iIndex<4; iIndex++)
	{
		if (m_uData[iIndex] < uOther.m_uData[iIndex])
			return -1;
		if (m_uData[iIndex] > uOther.m_uData[iIndex])
			return 1;
	}
	return 0;
}

int CUInt128::CompareTo(ULONG uValue) const
{
	if ((m_uData[0] > 0) || (m_uData[1] > 0) || (m_uData[2] > 0) || (m_uData[3] > uValue))
		return 1;
	if (m_uData[3] < uValue)
		return -1;
	return 0;
}

CUInt128& CUInt128::Add(const CUInt128 &uValue)
{
	if (uValue == 0)
		return *this;
	__int64 iSum = 0;
	for (int iIndex=3; iIndex>=0; iIndex--)
	{
		iSum += m_uData[iIndex];
		iSum += uValue.m_uData[iIndex];
		m_uData[iIndex] = (ULONG)iSum;
		iSum = iSum >> 32;
	}
	return *this;
}

CUInt128& CUInt128::Add(ULONG uValue)
{
	if (uValue == 0)
		return *this;
	Add(CUInt128(uValue));
	return *this;
}

CUInt128& CUInt128::Subtract(const CUInt128 &uValue)
{
	if (uValue == 0)
		return *this;
	__int64 iSum = 0;
	for (int iIndex=3; iIndex>=0; iIndex--)
	{
		iSum += m_uData[iIndex];
		iSum -= uValue.m_uData[iIndex];
		m_uData[iIndex] = (ULONG)iSum;
		iSum = iSum >> 32;
	}
	return *this;
}

CUInt128& CUInt128::Subtract(ULONG uValue)
{
	if (uValue == 0)
		return *this;
	Subtract(CUInt128(uValue));
	return *this;
}

/* Untested
CUInt128& CUInt128::Div(ULONG uValue)
{
	ULONG uBit, uRemain = 0;
	for (i = 0; i < 128; i++)
	{
		uBit = GetBitNumber(0);
		uRemain <<= 1;
		if (uBit)
			uRemain |= 1;
		ShiftLeft(1);
		if (uRemain >= uValue)
		{
			uRemain -= uValue;
			SetBitNumber(127, 1);
		}
	}
	return *this;
}
*/

CUInt128& CUInt128::ShiftLeft(UINT uBits)
{
	if ((uBits == 0) || (CompareTo(0) == 0))
		return *this;
	if (uBits > 127)
	{
		SetValue((ULONG)0);
		return *this;
	}

	ULONG uResult[] = {0,0,0,0};
	int iIndexShift = (int)uBits / 32;
	__int64 iShifted = 0;
	for (int iIndex=3; iIndex>=iIndexShift; iIndex--)
	{
		iShifted += ((__int64)m_uData[iIndex]) << (uBits % 32);
		uResult[iIndex-iIndexShift] = (ULONG)iShifted;
		iShifted = iShifted >> 32;
	}
	for (int iIndex=0; iIndex<4; iIndex++)
		m_uData[iIndex] = uResult[iIndex];
	return *this;
}

const byte* CUInt128::GetData() const
{
	return (byte*)m_uData;
}

byte* CUInt128::GetDataPtr() const
{
	return (byte*)m_uData;
}

ULONG CUInt128::Get32BitChunk(int iVal) const
{
	return m_uData[iVal];
}

void CUInt128::operator+  (const CUInt128 &uValue)
{
	Add(uValue);
}
void CUInt128::operator-  (const CUInt128 &uValue)
{
	Subtract(uValue);
}
void CUInt128::operator=  (const CUInt128 &uValue)
{
	SetValue(uValue);
}
bool CUInt128::operator<  (const CUInt128 &uValue) const
{
	return (CompareTo(uValue) <  0);
}
bool CUInt128::operator>  (const CUInt128 &uValue) const
{
	return (CompareTo(uValue) >  0);
}
bool CUInt128::operator<= (const CUInt128 &uValue) const
{
	return (CompareTo(uValue) <= 0);
}
bool CUInt128::operator>= (const CUInt128 &uValue) const
{
	return (CompareTo(uValue) >= 0);
}
bool CUInt128::operator== (const CUInt128 &uValue) const
{
	return (CompareTo(uValue) == 0);
}
bool CUInt128::operator!= (const CUInt128 &uValue) const
{
	return (CompareTo(uValue) != 0);
}

void CUInt128::operator+  (ULONG uValue)
{
	Add(uValue);
}
void CUInt128::operator-  (ULONG uValue)
{
	Subtract(uValue);
}
void CUInt128::operator=  (ULONG uValue)
{
	SetValue(uValue);
}
bool CUInt128::operator<  (ULONG uValue) const
{
	return (CompareTo(uValue) <  0);
}
bool CUInt128::operator>  (ULONG uValue) const
{
	return (CompareTo(uValue) >  0);
}
bool CUInt128::operator<= (ULONG uValue) const
{
	return (CompareTo(uValue) <= 0);
}
bool CUInt128::operator>= (ULONG uValue) const
{
	return (CompareTo(uValue) >= 0);
}
bool CUInt128::operator== (ULONG uValue) const
{
	return (CompareTo(uValue) == 0);
}
bool CUInt128::operator!= (ULONG uValue) const
{
	return (CompareTo(uValue) != 0);
}
