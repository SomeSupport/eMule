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
// ID3Field.cpp : Implementation of CID3Field
/////////////////////////////////////////////////////////////////////////////
// Change Log
//
// Date          Developer             Changes
//
// 05 Jan 2000   John Adcock           Original Release    
// 26 Apr 2000   John Adcock           Got working with id3lib 3.7.3
// 18 Aug 2000   Philip Oldaker        Added Picture Functionality
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ID3COM.h"
#include "ID3Field.h"
#include "ID3Frame.h"

/////////////////////////////////////////////////////////////////////////////
// CID3Field

CID3Field::CID3Field()
{
	m_Field = NULL;
	m_FrameParent = NULL;
}

CID3Field::~CID3Field()
{
	if(m_FrameParent != NULL)
	{
		m_FrameParent->Release();
	}
	m_Field = NULL;
}

IID3ComField* CID3Field::CreateObject(IID3ComFrame* FrameParent, ID3_Field* Field)
{
	CComObject<CID3Field>* pRetVal = new CComObject<CID3Field>;
	pRetVal->m_Field = Field;
	pRetVal->m_FrameParent = FrameParent;
	FrameParent->AddRef();
	return (IID3ComField*)pRetVal;
}

STDMETHODIMP CID3Field::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IID3ComField
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CID3Field::get_Text(long ItemNum, BSTR *pVal)
{
    USES_CONVERSION;
	luint TryChars = 0;
	luint GotChars = 0;
	try
	{
		*pVal = NULL;
		size_t nText = m_Field->Size();
        if(m_Field->GetEncoding() == ID3TE_UNICODE)
        {
		    unicode_t* sText = new unicode_t[nText + 1];
		    try 
		    {
			    m_Field->Get(sText, nText, ItemNum - 1);
			    sText[nText] = '\0';
			    *pVal = SysAllocString(sText);
		    }
		    catch (...)
		    {
			    *pVal = NULL;
		    }
		    delete [] sText;
        }
        else
        {
		    char* sText = new char[nText + 1];
		    try 
		    {
			    m_Field->Get(sText, nText, ItemNum - 1);
			    sText[nText] = '\0';
			    *pVal = SysAllocString(A2W(sText));
		    }
		    catch (...)
		    {
			    *pVal = NULL;
		    }
		    delete [] sText;
        }
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::put_Text(long ItemNum, BSTR newVal)
{
	try
	{
		m_Field->Set(newVal);
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::get_Long(long *pVal)
{
	try
	{
		*pVal = m_Field->Get();
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::put_Long(long newVal)
{
	try
	{
		m_Field->Set(newVal);
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::Clear()
{
	try
	{
		m_Field->Clear();
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::CopyDataToFile(BSTR FileName)
{
	USES_CONVERSION;
	try
	{
		m_Field->ToFile(OLE2A(FileName));
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::CopyDataFromFile(BSTR FileName)
{
	USES_CONVERSION;
	try
	{
		m_Field->FromFile(OLE2A(FileName));
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::get_NumTextItems(long *pVal)
{
	try
	{
		*pVal = m_Field->GetNumTextItems();
		return S_OK;
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Field::get_Binary(BSTR *pVal)
{
	// TODO: Add your implementation code here
	if (pVal == NULL)
		return E_POINTER;
	try
	{
		*pVal = NULL;
		const uchar *pData = m_Field->GetRawBinary();
		if (pData == NULL)
			return E_FAIL;
		ID3_Frame *pFrame = ((CID3Frame*)m_FrameParent)->GetID3Frame();		
		if (pFrame->GetID() == ID3FID_PICTURE)
		{
			*pVal = _bstr_t((LPCTSTR)pData).copy();			
		}
		else
		{
			*pVal = SysAllocStringByteLen((const char *)pData,m_Field->Size());
		}
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
	return S_OK;
}

STDMETHODIMP CID3Field::put_Binary(BSTR newVal)
{
	// TODO: Add your implementation code here
	try
	{
		size_t Size = SysStringByteLen(newVal);
		m_Field->Set((const uchar *)newVal,Size);
	}
	catch (...)
	{
		return Error(IDS_UNEXPECTED_ERROR, IID_IID3ComField, E_UNEXPECTED);
	}
	return S_OK;
}
///////////////////////////////////////////////////////
// End Added Philip Oldaker 12-Aug 2000
///////////////////////////////////////////////////////

