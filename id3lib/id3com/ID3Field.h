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
// ID3Field.h : Declaration of the CID3Field
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

#ifndef __ID3FIELD_H_
#define __ID3FIELD_H_

#include "resource.h"       // main symbols
#include <id3/tag.h>
#include <id3.h>

/////////////////////////////////////////////////////////////////////////////
// CID3Field
class ATL_NO_VTABLE CID3Field : 
	public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CID3Field, &CLSID_ID3ComField>,
	public ISupportErrorInfo,
	public IDispatchImpl<IID3ComField, &IID_IID3ComField, &LIBID_ID3COM>
{
public:
	CID3Field();
	~CID3Field();
	ID3_Field *GetID3Field()
	{
		return m_Field;
	}

	static IID3ComField* CreateObject(IID3ComFrame* FrameParent, ID3_Field* Field);

DECLARE_REGISTRY_RESOURCEID(IDR_ID3FIELD)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CID3Field)
	COM_INTERFACE_ENTRY(IID3ComField)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IID3ComField
public:
	STDMETHOD(get_Binary)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Binary)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_NumTextItems)(/*[out, retval]*/ long *pVal);
	STDMETHOD(CopyDataFromFile)(BSTR FileName);
	STDMETHOD(CopyDataToFile)(BSTR FileName);
	STDMETHOD(Clear)();
	STDMETHOD(get_Long)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Long)(/*[in]*/ long newVal);
	STDMETHOD(get_Text)(/*[in]*/ long ItemNum, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Text)(/*[in]*/ long ItemNum, /*[in]*/ BSTR newVal);
protected:
	ID3_Field* m_Field;
	IID3ComFrame* m_FrameParent;
};

#endif //__ID3FIELD_H_
