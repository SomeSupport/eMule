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
#pragma once

class CServer;
class CUpDownClient;
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

struct CurrentPingStruct 
{
	//uint32	datalen;
    CString state;
	uint32	latency;
	uint32	lowest;
    uint32  currentLimit;
};

class LastCommonRouteFinder :
    public CWinThread 
{
public:
	LastCommonRouteFinder();
	~LastCommonRouteFinder();

	void EndThread();

    bool AddHostToCheck(uint32 ip);
	bool AddHostsToCheck(CTypedPtrList<CPtrList, CServer*> &list);
	bool AddHostsToCheck(CUpDownClientPtrList &list);

	//uint32 GetPingedHost();
	CurrentPingStruct GetCurrentPing();
	bool AcceptNewClient();

	void SetPrefs(bool pEnabled, uint32 pCurUpload, uint32 pMinUpload, uint32 pMaxUpload, bool pUseMillisecondPingTolerance, double pPingTolerance, uint32 pPingToleranceMilliseconds, uint32 pGoingUpDivider, uint32 pGoingDownDivider, uint32 pNumberOfPingsForAverage, uint64 pLowestInitialPingAllowed);
    void InitiateFastReactionPeriod();

	uint32 GetUpload();
private:
	static UINT RunProc(LPVOID pParam);
	UINT RunInternal();

	void SetUpload(uint32 newValue);
    bool AddHostToCheckNoLock(uint32 ip);

	typedef CList<uint32,uint32> UInt32Clist;        
    uint32 Median(UInt32Clist& list);

	bool doRun;
	bool acceptNewClient;
	bool m_enabled;
	bool needMoreHosts;

	CCriticalSection addHostLocker;
	CCriticalSection prefsLocker;
	CCriticalSection uploadLocker;
	CCriticalSection pingLocker;

	CEvent* threadEndedEvent;
	CEvent* newTraceRouteHostEvent;
	CEvent* prefsEvent;

	CMap<uint32,uint32,uint32,uint32> hostsToTraceRoute;

    UInt32Clist pingDelays;

	uint64 pingDelaysTotal;

	uint32 minUpload;
	uint32 maxUpload;
	uint32 m_CurUpload;
	uint32 m_upload;

	double m_pingTolerance;
	uint32 m_iPingToleranceMilliseconds;
	bool m_bUseMillisecondPingTolerance;
	uint32 m_goingUpDivider;
	uint32 m_goingDownDivider;
	uint32 m_iNumberOfPingsForAverage;

	uint32 m_pingAverage;
	uint32 m_lowestPing;
	uint64 m_LowestInitialPingAllowed;

    bool m_initiateFastReactionPeriod;

	CString m_state;
};