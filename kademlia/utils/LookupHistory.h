//this file is part of eMule
//Copyright (C)2002-2010 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#pragma once
#include "../routing/Maps.h"
#include "../kademlia/Tag.h"

namespace Kademlia
{
	class CLookupHistory
	{
		public:
		struct SLookupHistoryEntry
		{
			CUInt128		m_uContactID;
			CUInt128		m_uDistance;
			CArray<int>		m_liReceivedFromIdx;
			uint32			m_dwAskedContactsTime;
			uint32			m_uRespondedContact;
			bool			m_bProvidedCloser;
			uint8			m_byContactVersion;
			uint32			m_dwAskedSearchItemTime;
			uint32			m_uRespondedSearchItem;
			uint32			m_uIP;
			uint16			m_uPort;
			bool			m_bForcedInteresting;
			bool			IsInteresting() const			{ return m_dwAskedContactsTime != 0 || m_dwAskedSearchItemTime != 0 || m_bForcedInteresting; }
		};

			CLookupHistory();
			~CLookupHistory();

			void			ContactReceived(CContact* pRecContact, CContact* pFromContact, CUInt128 uDistance, bool bCloser, bool bForceInteresting = false);
			void			ContactAskedKad(CContact* pContact);
			void			ContactAskedKeyword(CContact* pContact);
			void			ContactRespondedKeyword(uint32 uContactIP, uint16 uContactUDPPort, uint32 uResultCount);

			void			SetSearchStopped()							{ m_bSearchStopped = true; }
			void			SetSearchDeleted();
			void			SetGUIDeleted();
			void			SetUsedByGUI()								{ m_uRefCount++; }
			void			SetGUIName(const CKadTagValueString& sName)	{ m_sGUIName = sName; }
			void			SetSearchType(uint32 uVal)					{ m_uType = uVal; }
			
			bool			IsSearchStopped() const						{ return m_bSearchStopped; }
			bool			IsSearchDeleted() const						{ return m_bSearchDeleted; }
			
			CArray<SLookupHistoryEntry*>& GetHistoryEntries()			{ return m_aIntrestingHistoryEntries; }
			const CKadTagValueString&	GetGUIName() const				{ return m_sGUIName; }
			CString						GetTypeName() const;
			uint32						GetType() const					{ return m_uType; }

		protected:
			int				GetInterestingContactIdxByID(CUInt128 uContact) const;

		private:
			CArray<SLookupHistoryEntry*> m_aHistoryEntries;
			CArray<SLookupHistoryEntry*> m_aIntrestingHistoryEntries;
			bool				m_bSearchStopped;
			bool				m_bSearchDeleted;
			uint32				m_uRefCount;
			CKadTagValueString	m_sGUIName;
			uint32				m_uType;
	};
}