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
// EnumFields,h
/////////////////////////////////////////////////////////////////////////////
// Change Log
//
// Date          Developer             Changes
//
// 05 Jan 2000   John Adcock           Original Release    
// 26 Apr 2000   John Adcock           Got working with id3lib 3.7.3
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ENUMFIELDS_H_
#define __ENUMFIELDS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEnumFields
class ATL_NO_VTABLE CEnumFields : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IEnumVARIANT
{
public:
	CEnumFields();
	~CEnumFields();

	static IEnumVARIANT* CreateObject(IID3ComTag* Tag);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnumFields)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

	STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG * pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumVARIANT ** ppenum);
protected:
	IID3ComTag* m_Tag;
	long m_CurrentNum;
};

#endif //__ENUMVBWORKITEMS_H_
