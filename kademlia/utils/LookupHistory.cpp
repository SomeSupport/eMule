//this file is part of eMule
//Copyright (C)2010 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "StdAfx.h"
#include "LookupHistory.h"
#include "emule.h"
#include "../routing/contact.h"
#include "../kademlia/search.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;

CLookupHistory::CLookupHistory()
{
	m_bSearchStopped = false;
	m_bSearchDeleted = false;
	m_uRefCount = 1;
	(void)m_sGUIName;
}

CLookupHistory::~CLookupHistory()
{
	for (int i = 0; i < m_aHistoryEntries.GetCount(); i++)
		delete m_aHistoryEntries[i];
	 m_aHistoryEntries.RemoveAll();
}

void CLookupHistory::SetSearchDeleted()
{
	ASSERT( m_uRefCount );
	m_bSearchDeleted = true;
	m_uRefCount--;
	if (m_uRefCount == 0)
		delete this;
}

void CLookupHistory::SetGUIDeleted()
{
	ASSERT( m_uRefCount );
	m_uRefCount--;
	if (m_uRefCount == 0)
		delete this;
}

void CLookupHistory::ContactReceived(CContact* pRecContact, CContact* pFromContact, CUInt128 uDistance, bool bCloser, bool bForceInteresting)
{
	// Do we know this contact already? If pRecContact is NULL we only set the responded flag to the pFromContact
	for (int i = 0; i < m_aHistoryEntries.GetCount(); i++)
	{
		if (uDistance == m_aHistoryEntries[i]->m_uDistance || pRecContact == NULL)
		{
			if (pFromContact != NULL)
			{
				int iIdx = GetInterestingContactIdxByID(pFromContact->GetClientID());
				if (iIdx >= 0)
				{
					if (pRecContact != NULL)
						m_aHistoryEntries[i]->m_liReceivedFromIdx.Add(iIdx);
					m_aIntrestingHistoryEntries[iIdx]->m_uRespondedContact++;
					if (bCloser)
						m_aIntrestingHistoryEntries[iIdx]->m_bProvidedCloser = true;
				}
			}
			return;
		}
	}
	SLookupHistoryEntry* pstructNewEntry = new SLookupHistoryEntry;
	pstructNewEntry->m_uRespondedContact = 0;
	pstructNewEntry->m_uRespondedSearchItem = 0;
	pstructNewEntry->m_bProvidedCloser = false;
	pstructNewEntry->m_dwAskedContactsTime = 0;
	pstructNewEntry->m_dwAskedSearchItemTime = 0;
	if (pFromContact != NULL)
	{
		int iIdx = GetInterestingContactIdxByID(pFromContact->GetClientID());
		if (iIdx >= 0)
		{
			pstructNewEntry->m_liReceivedFromIdx.Add(iIdx);
			m_aIntrestingHistoryEntries[iIdx]->m_uRespondedContact++;
			if (bCloser)
				m_aIntrestingHistoryEntries[iIdx]->m_bProvidedCloser = true;
		}
	}
	pstructNewEntry->m_uContactID = pRecContact->GetClientID();
	pstructNewEntry->m_byContactVersion = pRecContact->GetVersion();
	pstructNewEntry->m_uDistance = uDistance;
	pstructNewEntry->m_uIP = pRecContact->GetIPAddress();
	pstructNewEntry->m_uPort = pRecContact->GetUDPPort();
	pstructNewEntry->m_bForcedInteresting = bForceInteresting;
	m_aHistoryEntries.Add(pstructNewEntry);
	if (bForceInteresting)
		m_aIntrestingHistoryEntries.Add(pstructNewEntry);
}

void CLookupHistory::ContactAskedKad(CContact* pContact)
{
	// Find contact
	for (int i = 0; i < m_aHistoryEntries.GetCount(); i++)
	{
		if (pContact->GetClientID() == m_aHistoryEntries[i]->m_uContactID)
		{
			if (!m_aHistoryEntries[i]->IsInteresting())
				m_aIntrestingHistoryEntries.Add(m_aHistoryEntries[i]);
			m_aHistoryEntries[i]->m_dwAskedContactsTime = ::GetTickCount();
			return;
		}
	}
	ASSERT( false );
}

int	CLookupHistory::GetInterestingContactIdxByID(CUInt128 uContact) const
{
	for (int i = 0; i < m_aIntrestingHistoryEntries.GetCount(); i++)
	{
		if (uContact == m_aIntrestingHistoryEntries[i]->m_uContactID)
			return i;
	}
	ASSERT( false );
	return (-1);
}

void CLookupHistory::ContactAskedKeyword(CContact* pContact)
{
	// Find contact
	for (int i = 0; i < m_aHistoryEntries.GetCount(); i++)
	{
		if (pContact->GetClientID() == m_aHistoryEntries[i]->m_uContactID)
		{
			if (!m_aHistoryEntries[i]->IsInteresting())
				m_aIntrestingHistoryEntries.Add(m_aHistoryEntries[i]);
			m_aHistoryEntries[i]->m_dwAskedSearchItemTime = ::GetTickCount();
			ASSERT( m_aHistoryEntries[i]->m_uRespondedSearchItem == 0 );
			return;
		}
	}
	ASSERT( false );
}

void CLookupHistory::ContactRespondedKeyword(uint32 uContactIP, uint16 uContactUDPPort, uint32 uResultCount)
{
	for (int i = 0; i < m_aIntrestingHistoryEntries.GetCount(); i++)
	{
		if ((m_aIntrestingHistoryEntries[i]->m_uIP == uContactIP) && (m_aIntrestingHistoryEntries[i]->m_uPort == uContactUDPPort))
		{
			ASSERT( m_aIntrestingHistoryEntries[i]->m_dwAskedSearchItemTime > 0 || m_uType == CSearch::NODE || m_uType == CSearch::NODEFWCHECKUDP);
			m_aIntrestingHistoryEntries[i]->m_uRespondedSearchItem += uResultCount;
			return;
		}
	}
	//ASSERT( false );
}

CString	CLookupHistory::GetTypeName() const
{
	switch (m_uType)
	{
		case CSearch::FILE:
			return GetResString(IDS_KAD_SEARCHSRC);
		case CSearch::KEYWORD:
			return GetResString(IDS_KAD_SEARCHKW);
		case CSearch::NODE:
		case CSearch::NODECOMPLETE:
		case CSearch::NODESPECIAL:
		case CSearch::NODEFWCHECKUDP:
			return GetResString(IDS_KAD_NODE);
		case CSearch::STOREFILE:
			return GetResString(IDS_KAD_STOREFILE);
		case CSearch::STOREKEYWORD:
			return GetResString(IDS_KAD_STOREKW);
		case CSearch::FINDBUDDY:
			return GetResString(IDS_FINDBUDDY);
		case CSearch::STORENOTES:
			return GetResString(IDS_STORENOTES);
		case CSearch::NOTES:
			return GetResString(IDS_NOTES);
		default:
			return GetResString(IDS_KAD_UNKNOWN);
	}
}