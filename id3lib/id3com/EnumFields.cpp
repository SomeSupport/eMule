/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000 John Adcock.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/
//
/////////////////////////////////////////////////////////////////////////////
// EnumFields.cpp : Implementation of DLL Exports.
/////////////////////////////////////////////////////////////////////////////
// Change Log
//
// Date          Developer             Changes
//
// 05 Jan 2000   John Adcock           Original Release    
// 26 Apr 2000   John Adcock           Got working with id3lib 3.7.3
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ID3COM.h"
#include "EnumFields.h"
#include <comdef.h>

/////////////////////////////////////////////////////////////////////////////
// CEnumFields
CEnumFields::CEnumFields()
{
	m_Tag = NULL;
	m_CurrentNum = 0;
}

CEnumFields::~CEnumFields()
{
	if(m_Tag != NULL)
	{
		m_Tag->Release();
	}
}

IEnumVARIANT* CEnumFields::CreateObject(IID3ComTag* Tag)
{
	CComObject<CEnumFields>* pRetVal = new CComObject<CEnumFields>;
	pRetVal->m_Tag = Tag;
	pRetVal->m_Tag->AddRef();
	return pRetVal;
}

STDMETHODIMP CEnumFields::Next(ULONG celt, VARIANT* rgelt, ULONG * pceltFetched)
{
	long NumItems;
	m_Tag->get_Count(&NumItems);
	for(long i(0); i < (long)((pceltFetched==NULL)?1:(*pceltFetched)); i++)
	{
		if(m_CurrentNum + i >= NumItems)
		{
			return S_FALSE;
		}
		else
		{
			IID3ComFrame* Frame;
			m_Tag->get_Item(m_CurrentNum + i, &Frame);
			*(_variant_t*)rgelt = Frame;
			Frame->Release();
			rgelt++;
		}
	}
	m_CurrentNum += ((pceltFetched==NULL)?1:(*pceltFetched));
	return S_OK;
}

STDMETHODIMP CEnumFields::Skip(ULONG celt)
{
	m_CurrentNum += celt;
	return S_OK;
}

STDMETHODIMP CEnumFields::Reset(void)
{
	m_CurrentNum = 0;
	return S_OK;
}

STDMETHODIMP CEnumFields::Clone(IEnumVARIANT ** ppenum)
{
	CComObject<CEnumFields>* pRetVal = new CComObject<CEnumFields>;
	pRetVal->m_Tag = m_Tag;
	pRetVal->m_CurrentNum = m_CurrentNum;
	pRetVal->m_Tag->AddRef();
	*ppenum = pRetVal;
	return S_OK;
}

