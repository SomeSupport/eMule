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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "StdAfx.h"
#include "UPnPImpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CUPnPImpl::CUPnPImpl()
:	m_bUPnPPortsForwarded( TRIS_FALSE ),
	m_hResultMessageWindow(0),
	m_nResultMessageID(0)
{
	m_nUDPPort = 0;
	m_nTCPPort = 0;
	m_nTCPWebPort = 0;
	m_bCheckAndRefresh = false;
}

CUPnPImpl::~CUPnPImpl()
{
}

void CUPnPImpl::SetMessageOnResult(HWND hWindow, UINT nMessageID){
	m_hResultMessageWindow = hWindow;
	m_nResultMessageID = nMessageID;
}

void CUPnPImpl::SendResultMessage(){
	if (m_hResultMessageWindow != 0 && m_nResultMessageID != 0){
		PostMessage(m_hResultMessageWindow, m_nResultMessageID, (WPARAM)(m_bUPnPPortsForwarded == TRIS_TRUE ? UPNP_OK : UPNP_FAILED), m_bCheckAndRefresh ? 1 : 0);
		m_nResultMessageID = 0;
		m_hResultMessageWindow = 0;
	}
}

void CUPnPImpl::LateEnableWebServerPort(uint16 nPort)
{
	if (m_nTCPWebPort == 0 && ArePortsForwarded() == TRIS_TRUE && IsReady())
	{
		m_nTCPWebPort = nPort;
		CheckAndRefresh();
	}
}
