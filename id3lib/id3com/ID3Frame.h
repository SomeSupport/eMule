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
// ID3Frame.h : Declaration of the CID3Frame
/////////////////////////////////////////////////////////////////////////////
// Change Log
//
// Date          Developer             Changes
//
// 05 Jan 2000   John Adcock           Original Release    
// 18 Aug 2000   Philip Oldaker        Added Picture Functionality
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ID3FRAME_H_
#define __ID3FRAME_H_

#include "resource.h"       // main symbols
#include <id3/tag.h>
#include <id3.h>

/////////////////////////////////////////////////////////////////////////////
// CID3Frame
class ATL_NO_VTABLE CID3Frame : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CID3Frame, &CLSID_ID3ComFrame>,
	public ISupportErrorInfo,
	public IDispatchImpl<IID3ComFrame, &IID_IID3ComFrame, &LIBID_ID3COM>
{
public:
	CID3Frame();
	~CID3Frame();
	ID3_Frame *GetID3Frame()
	{
		return m_Frame;
	}
	static IID3ComFrame* CreateObject(IID3ComTag* TagParent, ID3_Frame* Frame);

DECLARE_REGISTRY_RESOURCEID(IDR_ID3FRAME)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CID3Frame)
	COM_INTERFACE_ENTRY(IID3ComFrame)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IID3ComFrame
public:
	STDMETHOD(get_Compressed)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Compressed)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_FrameName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ID)(/*[out, retval]*/ eID3FrameTypes *pVal);
	STDMETHOD(put_ID)(/*[in]*/ eID3FrameTypes newVal);
	STDMETHOD(Clear)();
	STDMETHOD(get_Field)(/*[in]*/ eID3FieldTypes FieldType, /*[out, retval]*/ IID3ComField** pVal);

protected:
	ID3_Frame* m_Frame;
	IID3ComTag* m_TagParent;
};

#endif //__ID3FRAME_H_
