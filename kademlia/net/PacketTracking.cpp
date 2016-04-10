//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

#include "stdafx.h"
#include "emule.h"
#include "./PacketTracking.h"
#include "../../Log.h"
#include "../../opcodes.h"
#include "../../OtherFunctions.h"
#include "../../clientlist.h"
#include "../Kademlia/Kademlia.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Kademlia;

CPacketTracking::CPacketTracking(){
	dwLastTrackInCleanup = 0;
}

CPacketTracking::~CPacketTracking(){
	m_mapTrackPacketsIn.RemoveAll();
	while (!m_liTrackPacketsIn.IsEmpty())
		delete m_liTrackPacketsIn.RemoveHead();
}

void CPacketTracking::AddTrackedOutPacket(uint32 dwIP, uint8 byOpcode){
	// this tracklist tacks _outgoing_ request packets, to make sure incoming answer packets were requested
	// only track packets which we actually check for later
	if (!IsTrackedOutListRequestPacket(byOpcode))
		return;
	TrackPackets_Struct sTrack = {dwIP, ::GetTickCount(), byOpcode};
	listTrackedRequests.AddHead(sTrack);
	while (!listTrackedRequests.IsEmpty()){
		if (::GetTickCount() - listTrackedRequests.GetTail().dwInserted > SEC2MS(180))
			listTrackedRequests.RemoveTail();
		else
			break;
	}
}

bool CPacketTracking::IsTrackedOutListRequestPacket(uint8 byOpcode) const
{
	switch(byOpcode){
		case KADEMLIA2_BOOTSTRAP_REQ:
		case KADEMLIA2_HELLO_REQ:
		case KADEMLIA2_HELLO_RES:
		case KADEMLIA2_REQ:
		case KADEMLIA_SEARCH_NOTES_REQ:
		case KADEMLIA2_SEARCH_NOTES_REQ:
		case KADEMLIA_PUBLISH_REQ:
		case KADEMLIA2_PUBLISH_KEY_REQ:
		case KADEMLIA2_PUBLISH_SOURCE_REQ:
		case KADEMLIA2_PUBLISH_NOTES_REQ:
		case KADEMLIA_FINDBUDDY_REQ:
		case KADEMLIA_CALLBACK_REQ:
		case KADEMLIA2_PING:
			return true;
			break;
		default:
			return false;
	}

}

bool CPacketTracking::IsOnOutTrackList(uint32 dwIP, uint8 byOpcode, bool bDontRemove){
#ifdef _DEBUG
		if (!IsTrackedOutListRequestPacket(byOpcode))
			ASSERT( false ); // code error / bug
#endif
	for (POSITION pos = listTrackedRequests.GetHeadPosition(); pos != NULL; listTrackedRequests.GetNext(pos)){
		if (listTrackedRequests.GetAt(pos).dwIP == dwIP && listTrackedRequests.GetAt(pos).byOpcode == byOpcode && ::GetTickCount() - listTrackedRequests.GetAt(pos).dwInserted < SEC2MS(180)){
			if (!bDontRemove)
				listTrackedRequests.RemoveAt(pos);
			return true;
		}
	}
	return false;
}

bool CPacketTracking::InTrackListIsAllowedPacket(uint32 uIP, uint8 byOpcode, bool /*bValidSenderkey*/){
	// this tracklist tacks _incoming_ request packets and acts as a general flood protection by dropping
	// too frequent requests from a single IP, avoiding response floods, processing time DOS attacks and slowing down
	// other possible attacks/behavior (scanning indexed files, fake publish floods, etc)

	// first figure out if this is a request packet to be tracked and its timelimits
	// timelimits are choosed by estimating the max. frequency of such packets on normal operation (+ buffer)
	// (those limits are not meant be fine to be used by normal usage, but only supposed to be a flood detection)
	uint32 iAllowedPacketsPerMinute;
	const byte byDbgOrgOpcode = byOpcode;
	switch (byOpcode){
		case KADEMLIA2_BOOTSTRAP_REQ:
			iAllowedPacketsPerMinute = 2;
			break;
		case KADEMLIA2_HELLO_REQ:
			iAllowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_REQ:
			iAllowedPacketsPerMinute = 10;
			break;
		case KADEMLIA2_SEARCH_NOTES_REQ:
			iAllowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_SEARCH_KEY_REQ:
			iAllowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_SEARCH_SOURCE_REQ:
			iAllowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_PUBLISH_KEY_REQ:
			iAllowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_PUBLISH_SOURCE_REQ:
			iAllowedPacketsPerMinute = 2;
			break;
		case KADEMLIA2_PUBLISH_NOTES_REQ:
			iAllowedPacketsPerMinute = 2;
			break;
		case KADEMLIA_FIREWALLED2_REQ:
			byOpcode = KADEMLIA_FIREWALLED_REQ;
		case KADEMLIA_FIREWALLED_REQ:
			iAllowedPacketsPerMinute = 2;
			break;
		case KADEMLIA_FINDBUDDY_REQ:
			iAllowedPacketsPerMinute = 2;
			break;
		case KADEMLIA_CALLBACK_REQ:
			iAllowedPacketsPerMinute = 1;
			break;
		case KADEMLIA2_PING:
			iAllowedPacketsPerMinute = 2;
			break;
		default:
			// not any request packets, so its a response packet - no further checks on this point
			return true;
	}
	const uint32 iSecondsPerPacket = 60 / iAllowedPacketsPerMinute;
	const uint32 dwCurrentTick = ::GetTickCount();
	// time for cleaning up?
	if (dwCurrentTick - dwLastTrackInCleanup > MIN2MS(12))
		InTrackListCleanup();

	// check for existing entries
	TrackPacketsIn_Struct* pTrackEntry = NULL;
	if (!m_mapTrackPacketsIn.Lookup(uIP, pTrackEntry)){
		pTrackEntry = new TrackPacketsIn_Struct();
		pTrackEntry->m_uIP = uIP;
		m_mapTrackPacketsIn.SetAt(uIP, pTrackEntry);
		m_liTrackPacketsIn.AddHead(pTrackEntry);
	}

	// search specific request tracks
	for (int i = 0; i < pTrackEntry->m_aTrackedRequests.GetCount(); i++){
		if (pTrackEntry->m_aTrackedRequests[i].m_byOpcode == byOpcode){
			// already tracked requests with theis opcode, remove already expired request counts
			TrackPacketsIn_Struct::TrackedRequestIn_Struct& rCurTrackedRequest = pTrackEntry->m_aTrackedRequests[i];
			if (rCurTrackedRequest.m_nCount > 0 
				&& dwCurrentTick - rCurTrackedRequest.m_dwFirstAdded > SEC2MS(iSecondsPerPacket))
			{
				uint32 nRemoveCount = (dwCurrentTick - rCurTrackedRequest.m_dwFirstAdded) / SEC2MS(iSecondsPerPacket);
				if (nRemoveCount > rCurTrackedRequest.m_nCount){
					rCurTrackedRequest.m_nCount = 0;
					rCurTrackedRequest.m_dwFirstAdded = dwCurrentTick; // for the packet we just process
				}
				else {
					rCurTrackedRequest.m_nCount -= nRemoveCount;
					rCurTrackedRequest.m_dwFirstAdded += SEC2MS(iSecondsPerPacket) * nRemoveCount;
				}
			}
			// we increase the counter in any case, even if we drop the packet later
			rCurTrackedRequest.m_nCount++;
			// remember only for easier cleanup
			pTrackEntry->m_dwLastExpire = max(pTrackEntry->m_dwLastExpire, rCurTrackedRequest.m_dwFirstAdded + SEC2MS(iSecondsPerPacket) * rCurTrackedRequest.m_nCount);
		
			if (CKademlia::IsRunningInLANMode() && ::IsLANIP(ntohl(uIP))) // no flood detection in LanMode
				return true;

			// now the actualy check if this request is allowed
			if (rCurTrackedRequest.m_nCount > iAllowedPacketsPerMinute * 5){
				// this is so far above the limit that it has to be an intentional flood / misuse in any case
				// so we take the next higher punishment and ban the IP
				DebugLogWarning(_T("Kad: Massive request flood detected for opcode 0x%X (0x%X) from IP %s - Banning IP"), byOpcode, byDbgOrgOpcode, ipstr(ntohl(uIP))); 
				theApp.clientlist->AddBannedClient(ntohl(uIP));
				return false; // drop packet
			}
			else if (rCurTrackedRequest.m_nCount > iAllowedPacketsPerMinute){
				// over the limit, drop the packet but do nothing else
				if (!rCurTrackedRequest.m_bDbgLogged){
					rCurTrackedRequest.m_bDbgLogged = true;
					DebugLog(_T("Kad: Request flood detected for opcode 0x%X (0x%X) from IP %s - Droping packets with this opcode"), byOpcode, byDbgOrgOpcode, ipstr(ntohl(uIP))); 
				}
				return false; // drop packet
			}
			else
				rCurTrackedRequest.m_bDbgLogged = false;
			return true;
		}	
	}

	// add a new entry for this request, no checks needed since 1 is always ok
	TrackPacketsIn_Struct::TrackedRequestIn_Struct curTrackedRequest;
	curTrackedRequest.m_byOpcode = byOpcode;
	curTrackedRequest.m_bDbgLogged = false;
	curTrackedRequest.m_dwFirstAdded = dwCurrentTick;
	curTrackedRequest.m_nCount = 1;
	// remember only for easier cleanup
	pTrackEntry->m_dwLastExpire = max(pTrackEntry->m_dwLastExpire, dwCurrentTick + SEC2MS(iSecondsPerPacket));
	pTrackEntry->m_aTrackedRequests.Add(curTrackedRequest);
	return true;
}

void CPacketTracking::InTrackListCleanup(){
	const uint32 dwCurrentTick = ::GetTickCount();
	const uint32 dbgOldSize = m_liTrackPacketsIn.GetCount();
	dwLastTrackInCleanup = dwCurrentTick;
	POSITION pos1, pos2;
	for (pos1 = m_liTrackPacketsIn.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		m_liTrackPacketsIn.GetNext(pos1);
		TrackPacketsIn_Struct* curEntry = m_liTrackPacketsIn.GetAt(pos2);
		if (curEntry->m_dwLastExpire < dwCurrentTick){
			VERIFY(m_mapTrackPacketsIn.RemoveKey(curEntry->m_uIP));
			m_liTrackPacketsIn.RemoveAt(pos2);
			delete curEntry;
		}
	}
	DebugLog(_T("Cleaned up Kad Incoming Requests Tracklist, entries before: %u, after %u"), dbgOldSize, m_liTrackPacketsIn.GetCount());
}

void CPacketTracking::AddLegacyChallenge(CUInt128 uContactID, CUInt128 uChallengeID, uint32 uIP, uint8 byOpcode){
	TrackChallenge_Struct sTrack = {uIP, ::GetTickCount(), byOpcode, uContactID, uChallengeID};
	listChallengeRequests.AddHead(sTrack);
	while (!listChallengeRequests.IsEmpty()){
		if (::GetTickCount() - listChallengeRequests.GetTail().dwInserted > SEC2MS(180)){
			DEBUG_ONLY( DebugLog(_T("Challenge timed out, client not verified - %s"), ipstr(ntohl(listChallengeRequests.GetTail().uIP))) ); 
			listChallengeRequests.RemoveTail();
		}
		else
			break;
	}
}

bool CPacketTracking::IsLegacyChallenge(CUInt128 uChallengeID, uint32 uIP, uint8 byOpcode, CUInt128& ruContactID){
	bool bDbgWarning = false;
	for (POSITION pos = listChallengeRequests.GetHeadPosition(); pos != NULL; listChallengeRequests.GetNext(pos)){
		if (listChallengeRequests.GetAt(pos).uIP == uIP && listChallengeRequests.GetAt(pos).byOpcode == byOpcode 
			&& ::GetTickCount() - listChallengeRequests.GetAt(pos).dwInserted < SEC2MS(180))
		{
			ASSERT( listChallengeRequests.GetAt(pos).uChallenge != 0 || byOpcode == KADEMLIA2_PING );
			if (listChallengeRequests.GetAt(pos).uChallenge == 0 || listChallengeRequests.GetAt(pos).uChallenge == uChallengeID) {
				ruContactID = listChallengeRequests.GetAt(pos).uContactID;
				listChallengeRequests.RemoveAt(pos);
				return true;
			}
			else
				bDbgWarning = true;
		}
	}
	if (bDbgWarning)
		DebugLogWarning(_T("Kad: IsLegacyChallenge: Wrong challenge answer received, client not verified (%s)"), ipstr(ntohl(uIP)));
	return false;
}

bool CPacketTracking::HasActiveLegacyChallenge(uint32 uIP) const
{
	for (POSITION pos = listChallengeRequests.GetHeadPosition(); pos != NULL; listChallengeRequests.GetNext(pos)){
		if (listChallengeRequests.GetAt(pos).uIP == uIP && ::GetTickCount() - listChallengeRequests.GetAt(pos).dwInserted < SEC2MS(180))
			return true;
	}
	return false;
}