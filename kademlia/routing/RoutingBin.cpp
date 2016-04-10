/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
 
This work is based on the java implementation of the Kademlia protocol.
Kademlia: Peer-to-peer routing based on the XOR metric
Copyright (C) 2002  Petar Maymounkov [petar@post.harvard.edu]
http://kademlia.scs.cs.nyu.edu
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "stdafx.h"
#include "./RoutingBin.h"
#include "./Contact.h"
#include "../kademlia/Defines.h"
#include "../../Log.h"
#include "../../preferences.h"
#include "../../OtherFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;

CMap<uint32, uint32, uint32, uint32> CRoutingBin::s_mapGlobalContactIPs;
CMap<uint32, uint32, uint32, uint32> CRoutingBin::s_mapGlobalContactSubnets;

#define MAX_CONTACTS_SUBNET			10
#define MAX_CONTACTS_IP				1

CRoutingBin::CRoutingBin()
{
	// Init delete contact flag.
	m_bDontDeleteContacts = false;
}

CRoutingBin::~CRoutingBin()
{
	try
	{

		// Delete all contacts
		for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
		{
			AdjustGlobalTracking((*itContactList)->GetIPAddress(), false);
			if (!m_bDontDeleteContacts)
			{
				delete *itContactList;
			}
		}
		// Remove all contact entries.
		m_listEntries.clear();
	}
	catch (...)
	{
		AddDebugLogLine(false, _T("Exception in ~CRoutingBin"));
	}
}

bool CRoutingBin::AddContact(CContact *pContact)
{
	ASSERT(pContact != NULL);
	uint32 cSameSubnets = 0;
	// Check if we already have a contact with this ID in the list.
	for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		if (pContact->GetClientID() == (*itContactList)->m_uClientID){
			return false;
		}
		if ((pContact->GetIPAddress() & 0xFFFFFF00) ==  ((*itContactList)->GetIPAddress() & 0xFFFFFF00))
			cSameSubnets++;
	}

	// Several checks to make sure that we don't store multiple contacts from the same IP or too many contacts from the same subnet
	// This is supposed to add a bit of protection against several attacks and raise the ressource needs (IPs) for a successful contact on the attacker side 
	// Such IPs are not banned from Kad, they still can index, search, etc so multiple KAD clients behind one IP still work
	if (!CheckGlobalIPLimits(pContact->GetIPAddress(), pContact->GetUDPPort(), true))
		return false;

	// no more than 2 IPs from the same /24 netmask in one bin, except if its a LANIP (if we don't accept LANIPs they already have been filtered before)
	if (cSameSubnets >= 2 && !::IsLANIP(ntohl(pContact->GetIPAddress()))){
		if (::thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("Ignored kad contact (IP=%s:%u) - too many contacts with the same subnet in RoutingBin") , ipstr(ntohl(pContact->GetIPAddress())), pContact->GetUDPPort());
		return false;	
	}		

	// If not full, add to end of list
	if ( m_listEntries.size() < K)
	{
		m_listEntries.push_back(pContact);
		AdjustGlobalTracking(pContact->GetIPAddress(), true);
		return true;
	}
	return false;
}

void CRoutingBin::SetAlive(CContact *pContact)
{
	ASSERT(pContact != NULL);
	// Check if we already have a contact with this ID in the list.
	CContact *pContactTest = GetContact(pContact->GetClientID());
	ASSERT(pContact == pContactTest);
	if (pContactTest)
	{
		// Mark contact as being alive.
		pContactTest->UpdateType();
		// Move to the end of the list
		PushToBottom(pContactTest);
	}
}

void CRoutingBin::SetTCPPort(uint32 uIP, uint16 uUDPPort, uint16 uTCPPort)
{
	// Find contact with IP/Port
	for (ContactList::iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		CContact* pContact = *itContactList;
		if ((uIP == pContact->GetIPAddress()) && (uUDPPort == pContact->GetUDPPort()))
		{
			// Set TCPPort and mark as alive.
			pContact->SetTCPPort(uTCPPort);
			pContact->UpdateType();
			// Move to the end of the list
			PushToBottom(pContact);
			break;
		}
	}
}

CContact* CRoutingBin::GetContact(uint32 uIP, uint16 nPort, bool bTCPPort){
	// Find contact with IP/Port
	for (ContactList::iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		CContact* pContact = *itContactList;
		if ((uIP == pContact->GetIPAddress()) 
			&& ((!bTCPPort && nPort == pContact->GetUDPPort()) || (bTCPPort && nPort == pContact->GetTCPPort()) || nPort == 0))
		{
			return pContact;
		}
	}
	return NULL;
}

void CRoutingBin::RemoveContact(CContact *pContact,  bool bNoTrackingAdjust)
{
	if (!bNoTrackingAdjust)
		AdjustGlobalTracking(pContact->GetIPAddress(), false);
	m_listEntries.remove(pContact);
}

CContact *CRoutingBin::GetContact(const CUInt128 &uID)
{
	// Find contact by ID.
	for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		if (uID == (*itContactList)->m_uClientID)
			return *itContactList;
	}
	return NULL;
}

UINT CRoutingBin::GetSize() const
{
	return (UINT)m_listEntries.size();
}

void CRoutingBin::GetNumContacts(uint32& nInOutContacts, uint32& nInOutFilteredContacts, uint8 byMinVersion) const
{
	// Count all Nodes which meet the search criteria and also report those who don't
	for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		if ((*itContactList)->GetVersion() >= byMinVersion)
			nInOutContacts++;
		else
			nInOutFilteredContacts++;
	}
}

UINT CRoutingBin::GetRemaining() const
{
	return (UINT)K - m_listEntries.size();
}

void CRoutingBin::GetEntries(ContactList *plistResult, bool bEmptyFirst)
{
	// Clear results if requested first.
	if (bEmptyFirst)
		plistResult->clear();
	// Append all entries to the results.
	if (m_listEntries.size() > 0)
		plistResult->insert(plistResult->end(), m_listEntries.begin(), m_listEntries.end());
}

CContact *CRoutingBin::GetOldest()
{
	// All new/updated entries are appended to the back.
	if (m_listEntries.size() > 0)
		return m_listEntries.front();
	return NULL;
}

void CRoutingBin::GetClosestTo(uint32 uMaxType, const CUInt128 &uTarget, uint32 uMaxRequired, ContactMap *pmapResult, bool bEmptyFirst, bool bInUse)
{
	// Empty list if requested.
	if (bEmptyFirst)
		pmapResult->clear();

	// Return 0 since we have no entries.
	if (m_listEntries.size() == 0)
		return;

	// First put results in sort order for uTarget so we can insert them correctly.
	// We don't care about max results at this time.
	for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		if((*itContactList)->GetType() <= uMaxType && (*itContactList)->IsIpVerified())
		{
			CUInt128 uTargetDistance((*itContactList)->m_uClientID);
			uTargetDistance.Xor(uTarget);
			(*pmapResult)[uTargetDistance] = *itContactList;
			// This list will be used for an unknown time, Inc in use so it's not deleted.
			if( bInUse )
				(*itContactList)->IncUse();
		}
	}

	// Remove any extra results by least wanted first.
	while(pmapResult->size() > uMaxRequired)
	{
		// Dec in use count.
		if( bInUse )
			(--pmapResult->end())->second->DecUse();
		// remove from results
		pmapResult->erase(--pmapResult->end());
	}
	// Return result count to the caller.
	return;
}

void CRoutingBin::AdjustGlobalTracking(uint32 uIP, bool bIncrease){
	// IP
	uint32 nSameIPCount = 0;
	s_mapGlobalContactIPs.Lookup(uIP, nSameIPCount);
	if (bIncrease){
		if (nSameIPCount >= MAX_CONTACTS_IP){
			ASSERT( false );
			DebugLogError(_T("RoutingBin Global IP Tracking inconsitency on increase (%s)"), ipstr(ntohl(uIP)));
		}
		nSameIPCount++;
	}
	else if (!bIncrease){
		if (nSameIPCount == 0){
			ASSERT( false );
			DebugLogError(_T("RoutingBin Global IP Tracking inconsitency on decrease (%s)"), ipstr(ntohl(uIP)));
		}
		else
			nSameIPCount--;
	}
	if (nSameIPCount != 0)
		s_mapGlobalContactIPs.SetAt(uIP, nSameIPCount);
	else
		s_mapGlobalContactIPs.RemoveKey(uIP);

	// Subnet
	uint32 nSameSubnetCount = 0;
	s_mapGlobalContactSubnets.Lookup(uIP & 0xFFFFFF00, nSameSubnetCount);
	if (bIncrease){
		if (nSameSubnetCount >= MAX_CONTACTS_SUBNET && !::IsLANIP(ntohl(uIP))){
			ASSERT( false );
			DebugLogError(_T("RoutingBin Global Subnet Tracking inconsitency on increase (%s)"), ipstr(ntohl(uIP)));
		}
		nSameSubnetCount++;
	}
	else if (!bIncrease){
		if (nSameSubnetCount == 0){
			ASSERT( false );
			DebugLogError(_T("RoutingBin Global IP Subnet inconsitency on decrease (%s)"), ipstr(ntohl(uIP)));
		}
		else
			nSameSubnetCount--;
	}
	if (nSameSubnetCount != 0)
		s_mapGlobalContactSubnets.SetAt(uIP & 0xFFFFFF00, nSameSubnetCount);
	else
		s_mapGlobalContactSubnets.RemoveKey(uIP & 0xFFFFFF00);	
}

bool CRoutingBin::ChangeContactIPAddress(CContact* pContact, uint32 uNewIP)
{
	// Called if we want to update a indexed contact with a new IP. We have to check if we actually allow such a change
	// and if adjust our tracking. Rejecting a change will in the worst case lead a node contact to become invalid and purged later, 
	// but it also protects against a flood of malicous update requests from on IP which would be able to "reroute" all
	// contacts to itself and by that making them useless
	if (pContact->GetIPAddress() == uNewIP)
		return true;

	ASSERT( GetContact(pContact->GetClientID()) == pContact );

	// no more than 1 KadID per IP
	uint32 nSameIPCount = 0;
	s_mapGlobalContactIPs.Lookup(uNewIP, nSameIPCount);
	if (nSameIPCount >= MAX_CONTACTS_IP){
		if (::thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("Rejected kad contact ip change on update (old IP=%s, requested IP=%s) - too many contacts with the same IP (global)") , ipstr(ntohl(pContact->GetIPAddress())), ipstr(ntohl(uNewIP)));
		return false;
	}

	if ((pContact->GetIPAddress() & 0xFFFFFF00) != (uNewIP & 0xFFFFFF00)){
		//  no more than 10 IPs from the same /24 netmask global, except if its a LANIP (if we don't accept LANIPs they already have been filtered before)
		uint32 nSameSubnetGlobalCount = 0;
		s_mapGlobalContactSubnets.Lookup(uNewIP & 0xFFFFFF00, nSameSubnetGlobalCount);
		if (nSameSubnetGlobalCount >= MAX_CONTACTS_SUBNET && !::IsLANIP(ntohl(uNewIP))){
			if (::thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("Rejected kad contact ip change on update (old IP=%s, requested IP=%s) - too many contacts with the same Subnet (global)") , ipstr(ntohl(pContact->GetIPAddress())), ipstr(ntohl(uNewIP)));
			return false;	
		}

		// no more than 2 IPs from the same /24 netmask in one bin, except if its a LANIP (if we don't accept LANIPs they already have been filtered before)
		uint32 cSameSubnets = 0;
		// Check if we already have a contact with this ID in the list.
		for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
		{
			if ((uNewIP & 0xFFFFFF00) ==  ((*itContactList)->GetIPAddress() & 0xFFFFFF00))
				cSameSubnets++;
		}
		if (cSameSubnets >= 2 && !::IsLANIP(ntohl(uNewIP))){
			if (::thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("Rejected kad contact ip change on update (old IP=%s, requested IP=%s) - too many contacts with the same Subnet (local)") , ipstr(ntohl(pContact->GetIPAddress())), ipstr(ntohl(uNewIP)));
			return false;	
		}
	}

	// everything fine
	// LOGTODO REMOVE
	DEBUG_ONLY( DebugLog(_T("Index contact IP change allowed %s -> %s"), ipstr(ntohl(pContact->GetIPAddress())), ipstr(ntohl(uNewIP))) );
	AdjustGlobalTracking(pContact->GetIPAddress(), false);
	pContact->SetIPAddress(uNewIP);
	AdjustGlobalTracking(pContact->GetIPAddress(), true);
	return true;
}

void CRoutingBin::PushToBottom(CContact* pContact) // puts an existing contact from X to the end of the list
{
	ASSERT( GetContact(pContact->GetClientID()) == pContact );
	RemoveContact(pContact, true);
	m_listEntries.push_back(pContact);
}

CContact* CRoutingBin::GetRandomContact(uint32 nMaxType, uint32 nMinKadVersion)
{
	if (m_listEntries.empty())
		return NULL;
	// Find contact with IP/Port
	CContact* pLastFit = NULL;
	uint32 nRandomStartPos = GetRandomUInt16() % m_listEntries.size();
	uint32 nIndex = 0;
	for (ContactList::iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		CContact* pContact = *itContactList;
		if (pContact->GetType() <= nMaxType && pContact->GetVersion() >= nMinKadVersion)
		{
			if (nIndex >= nRandomStartPos)
				return pContact;
			else
				pLastFit = pContact;
		}
		nIndex++;
	}
	return pLastFit;
}

void CRoutingBin::SetAllContactsVerified()
{
	// Find contact by ID.
	for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		(*itContactList)->SetIpVerified(true);
	}
}

bool CRoutingBin::CheckGlobalIPLimits(uint32 uIP, uint16 uPort, bool bLog)
{
	// no more than 1 KadID per IP
	uint32 nSameIPCount = 0;
	s_mapGlobalContactIPs.Lookup(uIP, nSameIPCount);
	if (nSameIPCount >= MAX_CONTACTS_IP){
		if (bLog && ::thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("Ignored kad contact (IP=%s:%u) - too many contacts with the same IP (global)") , ipstr(ntohl(uIP)), uPort);
		return false;	
	}	
	//  no more than 10 IPs from the same /24 netmask global, except if its a LANIP (if we don't accept LANIPs they already have been filtered before)
	uint32 nSameSubnetGlobalCount = 0;
	s_mapGlobalContactSubnets.Lookup(uIP & 0xFFFFFF00, nSameSubnetGlobalCount);
	if (nSameSubnetGlobalCount >= MAX_CONTACTS_SUBNET && !::IsLANIP(ntohl(uIP))){
		if (bLog && ::thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("Ignored kad contact (IP=%s:%u) - too many contacts with the same Subnet (global)"), ipstr(ntohl(uIP)), uPort);
		return false;	
	}
	return true;
}

bool CRoutingBin::HasOnlyLANNodes() const
{
	for (ContactList::const_iterator itContactList = m_listEntries.begin(); itContactList != m_listEntries.end(); ++itContactList)
	{
		if (!::IsLANIP(ntohl((*itContactList)->GetIPAddress())))
			return false;
	}
	return true;
}