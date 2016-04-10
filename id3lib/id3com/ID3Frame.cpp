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
// ID3Frame.cpp : Implementation of CID3Frame
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
#include "ID3Frame.h"
#include "ID3Field.h"

/////////////////////////////////////////////////////////////////////////////
// CID3Frame

CID3Frame::CID3Frame()
{
	m_Frame = NULL;
	m_TagParent = NULL;
}

CID3Frame::~CID3Frame()
{
	if(m_TagParent != NULL)
	{
		m_TagParent->Release();
	}
	m_Frame = NULL;
}

IID3ComFrame* CID3Frame::CreateObject(IID3ComTag* TagParent, ID3_Frame* Frame)
{
	CComObject<CID3Frame>* pRetVal = new CComObject<CID3Frame>;
	pRetVal->m_Frame = Frame;
	pRetVal->m_TagParent = TagParent;
	TagParent->AddRef();
	return (IID3ComFrame*)pRetVal;
}

STDMETHODIMP CID3Frame::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IID3ComFrame
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CID3Frame::get_Field(eID3FieldTypes FieldType, IID3ComField** pVal)
{
	try
	{
		*pVal = NULL;
		ID3_Field* Field = &(m_Frame->Field((enum ID3_FieldID)FieldType));
        if(Field != NULL)
        {
		    *pVal = CID3Field::CreateObject(this, Field);
		    (*pVal)->AddRef();
        }
        else
        {
            *pVal = NULL;
        }
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComFrame, "An unexpected error has occurred", IID_IID3ComFrame, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Frame::Clear()
{
	try
	{
		m_Frame->Clear();
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComFrame, "An unexpected error has occurred", IID_IID3ComFrame, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Frame::get_ID(eID3FrameTypes *pVal)
{
	try
	{
		*pVal = ID3_NOFRAME;
		*pVal = (eID3FrameTypes)m_Frame->GetID();
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComFrame, "An unexpected error has occurred", IID_IID3ComFrame, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Frame::put_ID(eID3FrameTypes newVal)
{
	try
	{
		m_Frame->SetID((ID3_FrameID)newVal);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComFrame, "An unexpected error has occurred", IID_IID3ComFrame, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Frame::get_FrameName(BSTR *pVal)
{
	USES_CONVERSION;
	try
	{
		const char* sDesc = m_Frame->GetDescription();
		if(sDesc != NULL)
		{
			*pVal = SysAllocString(A2W(sDesc));
		}
		else
		{
			*pVal = SysAllocString(L"Unknown Frame");
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComFrame, "An unexpected error has occurred", IID_IID3ComFrame, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Frame::put_Compressed(VARIANT_BOOL newVal)
{
	try
	{
		m_Frame->SetCompression(newVal == VARIANT_TRUE);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComFrame, "An unexpected error has occurred", IID_IID3ComFrame, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Frame::get_Compressed(VARIANT_BOOL *pVal)
{
	USES_CONVERSION;
	try
	{
		if(m_Frame->GetCompression())
		{
			*pVal = VARIANT_TRUE;
		}
		else
		{
			*pVal = VARIANT_FALSE;
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComFrame, "An unexpected error has occurred", IID_IID3ComFrame, E_UNEXPECTED);
	}
}