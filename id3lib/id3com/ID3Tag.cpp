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
// ID3Tag.cpp : Implementation of CID3Tag
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
#include "ID3Tag.h"
#include "config.h"
#include "ID3Frame.h"
#include "ID3Field.h"
#include "EnumFields.h"
#include "id3/misc_support.h"
#include "math.h"

/////////////////////////////////////////////////////////////////////////////
// CID3Tag

CID3Tag::CID3Tag()
{
	m_ID3Tag = new ID3_Tag();
}

CID3Tag::~CID3Tag()
{
	if(m_ID3Tag != NULL)
	{
		delete m_ID3Tag;
	}
}

STDMETHODIMP CID3Tag::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
		&IID_IID3ComTag
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CID3Tag::Link(BSTR *FileName)
{
	USES_CONVERSION;
	
	if(FileName == NULL)
	{
		return E_INVALIDARG;
	}

	try
	{
		m_ID3Tag->Link(OLE2A(*FileName));
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
	return S_OK;
}

STDMETHODIMP CID3Tag::Clear()
{
	try
	{
		m_ID3Tag->Clear();
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
	return S_OK;
}

STDMETHODIMP CID3Tag::get_HasChanged(VARIANT_BOOL *pVal)
{
	try
	{
		*pVal = VARIANT_FALSE;
		if(m_ID3Tag->HasChanged())
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
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::FindFrame(eID3FrameTypes FrameID, VARIANT_BOOL CreateNewIfNotFound, IID3ComFrame** pVal)
{
	try
	{
		*pVal = NULL;
		ID3_Frame* pFrame = m_ID3Tag->Find((enum ID3_FrameID)FrameID);
		if(CreateNewIfNotFound == VARIANT_TRUE && pFrame == NULL)
		{
			pFrame = new ID3_Frame((enum ID3_FrameID)FrameID);
			m_ID3Tag->AttachFrame(pFrame);
		}
		if(pFrame != NULL)
		{
			*pVal = CID3Frame::CreateObject(this, pFrame);
			(*pVal)->AddRef();
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get__NewEnum(IUnknown** pRetVal)
{
	*pRetVal = CEnumFields::CreateObject(this);
	(*pRetVal)->AddRef();
	return S_OK;
}

STDMETHODIMP CID3Tag::get_Count(long *pVal)
{
	try
	{
		*pVal = 0;
		*pVal = m_ID3Tag->NumFrames();
		return S_OK;
	}
	catch (...)
	{
		*pVal = 0;
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Item(long FrameNum, IID3ComFrame** pVal)
{
	try
	{
		*pVal = NULL;
        ID3_Frame* pFrame = NULL;
        ID3_Tag::Iterator* it = m_ID3Tag->CreateIterator();
        pFrame = it->GetNext();
        while(FrameNum)
        {
            pFrame = it->GetNext();
            --FrameNum;
        }
        delete it;
        if(pFrame != NULL)
        {
		    *pVal = CID3Frame::CreateObject(this, pFrame);
		    (*pVal)->AddRef();
        }
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::SaveV1Tag()
{
	try
	{
		m_ID3Tag->Update(ID3TT_ID3V1);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::StripV1Tag()
{
	try
	{
		m_ID3Tag->Strip(ID3TT_ID3V1);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::SaveV2Tag()
{
	try
	{
		m_ID3Tag->Update(ID3TT_ID3V2);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::StripV2Tag()
{
	try
	{
		m_ID3Tag->Strip(ID3TT_ID3V2);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Artist(BSTR *pVal)
{
	USES_CONVERSION;
	try
	{
		*pVal = NULL;
		char* RetStr = NULL;
		RetStr = ID3_GetArtist(m_ID3Tag);
		*pVal = SysAllocString(A2W(RetStr));
		if(RetStr != NULL)
		{
			delete [] RetStr;
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Artist(BSTR newVal)
{
	USES_CONVERSION;
	try
	{
		ID3_Frame *pFrame;
		if((pFrame = m_ID3Tag->Find(ID3FID_LEADARTIST)) == NULL &&
			(pFrame = m_ID3Tag->Find(ID3FID_BAND)) == NULL &&
			(pFrame = m_ID3Tag->Find(ID3FID_CONDUCTOR)) == NULL &&
			(pFrame = m_ID3Tag->Find(ID3FID_COMPOSER)) == NULL)
		{
			pFrame = new ID3_Frame(ID3FID_LEADARTIST);
			m_ID3Tag->AttachFrame(pFrame);
		}
		pFrame->Field(ID3FN_TEXT) = OLE2A(newVal);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Album(BSTR *pVal)
{
	USES_CONVERSION;
	char* RetStr = NULL;
	try
	{
		*pVal = NULL;
		RetStr = ID3_GetAlbum(m_ID3Tag);
		*pVal = SysAllocString(A2W(RetStr));
		if(RetStr != NULL)
		{
			delete [] RetStr;
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Album(BSTR newVal)
{
	USES_CONVERSION;
	try
	{
		ID3_AddAlbum(m_ID3Tag, OLE2A(newVal), true);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Title(BSTR *pVal)
{
	USES_CONVERSION;
	char* RetStr = NULL;
	try
	{
		*pVal = NULL;
		RetStr = ID3_GetTitle(m_ID3Tag);
		*pVal = SysAllocString(A2W(RetStr));
		if(RetStr != NULL)
		{
			delete [] RetStr;
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Title(BSTR newVal)
{
	USES_CONVERSION;
	try
	{
		ID3_AddTitle(m_ID3Tag, OLE2A(newVal), true);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Comment(BSTR *pVal)
{
	USES_CONVERSION;
	try
	{
		*pVal = NULL;
		char* RetStr = NULL;
		RetStr = ID3_GetComment(m_ID3Tag);
		*pVal = SysAllocString(A2W(RetStr));
		if(RetStr != NULL)
		{
			delete [] RetStr;
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Comment(BSTR newVal)
{
	USES_CONVERSION;
	try
	{
		ID3_AddComment(m_ID3Tag, OLE2A(newVal), STR_V1_COMMENT_DESC, true);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Genre(long *pVal)
{
	try
	{
		*pVal = 255;
		*pVal = ID3_GetGenreNum(m_ID3Tag);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Genre(long newVal)
{
	try
	{
		ID3_AddGenre(m_ID3Tag, newVal, true);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Year(BSTR *pVal)
{
	USES_CONVERSION;
	try
	{
		*pVal = 0;
		char* RetStr = NULL;
		RetStr = ID3_GetYear(m_ID3Tag);
		*pVal = SysAllocString(A2W(RetStr));
		if(RetStr != NULL)
		{
			delete [] RetStr;
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Year(BSTR newVal)
{
	USES_CONVERSION;
	try
	{
		ID3_AddYear(m_ID3Tag, OLE2A(newVal), true);
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
	return S_OK;
}

STDMETHODIMP CID3Tag::get_Track(long *pVal)
{
	try
	{
		*pVal = -1;
		*pVal = ID3_GetTrackNum(m_ID3Tag);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Track(long newVal)
{
	try
	{
		ID3_AddTrack(m_ID3Tag, (uchar)newVal, 0, true);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_LastPlayed(DATE *pVal)
{
	try
	{
		*pVal = 0;
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_USERTEXT, ID3FN_DESCRIPTION, "LastPlayed");
		if (pFrame != NULL)
		{
			char* szLastPlayed = ID3_GetString(pFrame, ID3FN_TEXT);
			if(szLastPlayed != NULL)
			{
				*pVal = atof(szLastPlayed);
				delete [] szLastPlayed;
			}
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_LastPlayed(DATE newVal)
{
	try
	{
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_USERTEXT, ID3FN_DESCRIPTION, "LastPlayed");
		if(pFrame == NULL)
		{
			pFrame = new ID3_Frame(ID3FID_USERTEXT);
			pFrame->Field(ID3FN_DESCRIPTION) = "LastPlayed";
			pFrame->Field(ID3FN_TEXTENC) = ID3TE_ASCII;
			m_ID3Tag->AttachFrame(pFrame);
		}
		char Buffer[40];
		sprintf(Buffer, "%6.7f", newVal);
		pFrame->Field(ID3FN_TEXT) = Buffer;
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_HasV1Tag(VARIANT_BOOL *pVal)
{
	try
	{
		*pVal = VARIANT_FALSE;
		if (m_ID3Tag->HasTagType(ID3TT_ID3V1))
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
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_HasV2Tag(VARIANT_BOOL *pVal)
{
	try
	{
		*pVal = VARIANT_FALSE;
		if (m_ID3Tag->HasTagType(ID3TT_ID3V2))
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
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_HasLyrics(VARIANT_BOOL *pVal)
{
	try
	{
		if (m_ID3Tag->HasTagType(ID3TT_LYRICS))
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
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::FindFrameString(eID3FrameTypes FrameID, eID3FieldTypes FieldType, BSTR FindString, VARIANT_BOOL CreateNewIfNotFound, IID3ComFrame** pVal)
{
	try
	{
		*pVal = NULL;
		ID3_Frame* pFrame = m_ID3Tag->Find((enum ID3_FrameID)FrameID, (enum ID3_FieldID)FieldType, FindString);
		if(CreateNewIfNotFound == VARIANT_TRUE && pFrame == NULL)
		{
			pFrame = new ID3_Frame((enum ID3_FrameID)FrameID);
			pFrame->Field((enum ID3_FieldID)FieldType) = FindString;
                        m_ID3Tag->AttachFrame(pFrame);
		}
		if(pFrame != NULL)
		{
			*pVal = CID3Frame::CreateObject(this, pFrame);
			(*pVal)->AddRef();
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}


STDMETHODIMP CID3Tag::get_PlayCount(BSTR EMailAddress, long *pVal)
{
	try
	{
		*pVal = 0;
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_POPULARIMETER, ID3FN_EMAIL, EMailAddress);
		if (pFrame != NULL)
		{
			*pVal = pFrame->Field(ID3FN_COUNTER).Get();
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_PlayCount(BSTR EMailAddress, long newVal)
{
	try
	{
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_POPULARIMETER, ID3FN_EMAIL, EMailAddress);
		if(pFrame == NULL)
		{
			pFrame = new ID3_Frame(ID3FID_POPULARIMETER);
			pFrame->Field(ID3FN_EMAIL) = EMailAddress;
			m_ID3Tag->AttachFrame(pFrame);
		}
		pFrame->Field(ID3FN_COUNTER) = newVal;
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_Popularity(BSTR EMailAddress, short *pVal)
{
	try
	{
		*pVal = 0;
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_POPULARIMETER, ID3FN_EMAIL, EMailAddress);
		if (pFrame != NULL)
		{
			*pVal = (short)pFrame->Field(ID3FN_RATING).Get();
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Popularity(BSTR EMailAddress, short newVal)
{
	try
	{
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_POPULARIMETER, ID3FN_EMAIL, EMailAddress);
		if(pFrame == NULL)
		{
			pFrame = new ID3_Frame(ID3FID_POPULARIMETER);
			pFrame->Field(ID3FN_EMAIL) = EMailAddress;
			m_ID3Tag->AttachFrame(pFrame);
		}
		pFrame->Field(ID3FN_RATING) = newVal;
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_TagCreated(DATE *pVal)
{
	try
	{
		*pVal = 0;
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_USERTEXT, ID3FN_DESCRIPTION, "TagCreated");
		if (pFrame != NULL)
		{
			char* szTagCreated = ID3_GetString(pFrame, ID3FN_TEXT);
			if(szTagCreated != NULL)
			{
				*pVal = atof(szTagCreated);
				delete [] szTagCreated;
			}
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_TagCreated(DATE newVal)
{
	try
	{
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_USERTEXT, ID3FN_DESCRIPTION, "TagCreated");
		if(pFrame == NULL)
		{
			pFrame = new ID3_Frame(ID3FID_USERTEXT);
			pFrame->Field(ID3FN_DESCRIPTION) = "TagCreated";
			pFrame->Field(ID3FN_TEXTENC) = ID3TE_ASCII;
			m_ID3Tag->AttachFrame(pFrame);
		}
		char Buffer[40];
		sprintf(Buffer, "%6.7f", newVal);
		pFrame->Field(ID3FN_TEXT) = Buffer;
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_PercentVolumeAdjust(double *pVal)
{
	try
	{
		*pVal = 100;
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_USERTEXT, ID3FN_DESCRIPTION, "PercentVolAdj");
		if (pFrame != NULL)
		{
			char* szVolAdjust = ID3_GetString(pFrame, ID3FN_TEXT);
			if(szVolAdjust != NULL)
			{
				*pVal = atof(szVolAdjust);
				delete [] szVolAdjust;
			}
		}
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_PercentVolumeAdjust(double newVal)
{
	try
	{
		ID3_Frame *pFrame = m_ID3Tag->Find(ID3FID_USERTEXT, ID3FN_DESCRIPTION, "PercentVolAdj");
		if(pFrame == NULL)
		{
			pFrame = new ID3_Frame(ID3FID_USERTEXT);
			pFrame->Field(ID3FN_DESCRIPTION) = "PercentVolAdj";
			pFrame->Field(ID3FN_TEXTENC) = ID3TE_ASCII;
			m_ID3Tag->AttachFrame(pFrame);
		}
		char Buffer[40];
		sprintf(Buffer, "%6.7f", newVal);
		pFrame->Field(ID3FN_TEXT) = Buffer;
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_Padding(VARIANT_BOOL newVal)
{
	try
	{
		m_ID3Tag->SetPadding(newVal == VARIANT_TRUE);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::put_UnSync(VARIANT_BOOL newVal)
{
	try
	{
		m_ID3Tag->SetUnsync(newVal == VARIANT_TRUE);
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::get_VersionString(BSTR *pVal)
{
	USES_CONVERSION;
	try
	{
		// this buffer needs to be overly large
		// for some unexplained reason
		// if the size is small you get what?
		char Buffer[1024];
		sprintf(Buffer, "id3com %d.%d.%d Compiled %s %s", _ID3LIB_MAJOR_VERSION,
														_ID3LIB_MINOR_VERSION,
														_ID3LIB_PATCH_VERSION,
														__DATE__,
														__TIME__);
		*pVal = SysAllocString(A2W(Buffer));
		return S_OK;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
}

STDMETHODIMP CID3Tag::RemoveFrame(eID3FrameTypes FrameID)
{
	// TODO: Add your implementation code here
	HRESULT hr=S_OK;
	try
	{
		ID3_Frame* pFrame = m_ID3Tag->Find((enum ID3_FrameID)FrameID);
		if (pFrame)
		{
			/* pTag is an ID3_Tag */
			delete m_ID3Tag->RemoveFrame(pFrame);
		}
		else
			hr = E_POINTER;
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}
	return hr;
}

STDMETHODIMP CID3Tag::RemoveFrameByNum(long FrameNum)
{
	HRESULT hr=S_OK;
	try
	{
        ID3_Frame* pFrame = NULL;
        ID3_Tag::Iterator* it = m_ID3Tag->CreateIterator();
        pFrame = it->GetNext();
        while(FrameNum)
        {
            pFrame = it->GetNext();
            --FrameNum;
        }
        delete it;

		if(pFrame)
		{
			/* pTag is an ID3_Tag */
			delete m_ID3Tag->RemoveFrame(pFrame);			
		}
		else
        {
			hr = E_POINTER;
        }
	}
	catch (...)
	{
		return AtlReportError(CLSID_ID3ComTag, "An unexpected error has occurred", IID_IID3ComTag, E_UNEXPECTED);
	}

	return hr;
}
///////////////////////////////////////////////////////
// End Added Philip Oldaker 12-Aug 2000
///////////////////////////////////////////////////////

