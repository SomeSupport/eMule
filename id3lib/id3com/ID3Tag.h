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
// ID3Tag.h : Declaration of the CID3Tag
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

#ifndef __ID3TAG_H_
#define __ID3TAG_H_

#include "resource.h"       // main symbols
#include <id3/tag.h>
#include "ID3COM.h"

/////////////////////////////////////////////////////////////////////////////
// CID3Tag
class ATL_NO_VTABLE CID3Tag : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CID3Tag, &CLSID_ID3ComTag>,
	public ISupportErrorInfo,
	public IDispatchImpl<IID3ComTag, &IID_IID3ComTag, &LIBID_ID3COM>
{
public:
	CID3Tag();
	~CID3Tag();

DECLARE_REGISTRY_RESOURCEID(IDR_ID3TAG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CID3Tag)
	COM_INTERFACE_ENTRY(IID3ComTag)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IID3ComTag
public:
	STDMETHOD(RemoveFrameByNum)(/*[in]*/ long FrameNum);
	STDMETHOD(RemoveFrame)(/*[in]*/ eID3FrameTypes FrameID);
	STDMETHOD(get_VersionString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_UnSync)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(put_Padding)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_PercentVolumeAdjust)(/*[out, retval]*/ double *pVal);
	STDMETHOD(put_PercentVolumeAdjust)(/*[in]*/ double newVal);
	STDMETHOD(get_TagCreated)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(put_TagCreated)(/*[in]*/ DATE newVal);
	STDMETHOD(get_Popularity)(/*[in]*/ BSTR EMailAddress, /*[out, retval]*/ short *pVal);
	STDMETHOD(put_Popularity)(/*[in]*/ BSTR EMailAddress, /*[in]*/ short newVal);
	STDMETHOD(get_PlayCount)(/*[in]*/ BSTR EMailAddress, /*[out, retval]*/ long *pVal);
	STDMETHOD(put_PlayCount)(/*[in]*/ BSTR EMailAddress, /*[in]*/ long newVal);
	STDMETHOD(FindFrameString)(/*[in]*/ eID3FrameTypes FrameID, /*[in]*/ eID3FieldTypes FieldType, /*[in]*/ BSTR FindString, /*[in]*/ VARIANT_BOOL CreateNewIfNotFound, /*[out, retval]*/ IID3ComFrame** pVal);
	STDMETHOD(get_HasLyrics)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_HasV2Tag)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_HasV1Tag)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_LastPlayed)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(put_LastPlayed)(/*[in]*/ DATE newVal);
	STDMETHOD(get_Track)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Track)(/*[in]*/ long newVal);
	STDMETHOD(get_Year)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Year)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Genre)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Genre)(/*[in]*/ long newVal);
	STDMETHOD(get_Comment)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Comment)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Title)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Title)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Album)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Album)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Artist)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Artist)(/*[in]*/ BSTR newVal);
	STDMETHOD(StripV2Tag)();
	STDMETHOD(SaveV2Tag)();
	STDMETHOD(StripV1Tag)();
	STDMETHOD(SaveV1Tag)();
	STDMETHOD(get_Item)(/*[in]*/ long FrameNum, /*[out, retval]*/ IID3ComFrame** pVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
	STDMETHOD(FindFrame)(/*[in]*/ eID3FrameTypes FrameID, /*[in]*/ VARIANT_BOOL CreateNewIfNotFound, /*[out, retval]*/ IID3ComFrame** pVal);
	STDMETHOD(get_HasChanged)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(Clear)();
	STDMETHOD(Link)(BSTR* FileName);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown** pRetVal);
protected:
	ID3_Tag* m_ID3Tag;
};

#endif //__ID3TAG_H_
