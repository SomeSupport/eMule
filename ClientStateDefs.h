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

enum EUploadState{
	US_UPLOADING,
	US_ONUPLOADQUEUE,
	US_CONNECTING,
	US_BANNED,
	US_NONE
};

enum EDownloadState{
	DS_DOWNLOADING,
	DS_ONQUEUE,
	DS_CONNECTED,
	DS_CONNECTING,
	DS_WAITCALLBACK,
	DS_WAITCALLBACKKAD,
	DS_REQHASHSET,
	DS_NONEEDEDPARTS,
	DS_TOOMANYCONNS,
	DS_TOOMANYCONNSKAD,
	DS_LOWTOLOWIP,
	DS_BANNED,
	DS_ERROR,
	DS_NONE,
	DS_REMOTEQUEUEFULL  // not used yet, except in statistics
};

enum EPeerCacheDownState{
	PCDS_NONE = 0,
	PCDS_WAIT_CLIENT_REPLY,
	PCDS_WAIT_CACHE_REPLY,
	PCDS_DOWNLOADING
};

enum EPeerCacheUpState{
	PCUS_NONE = 0,
	PCUS_WAIT_CACHE_REPLY,
	PCUS_UPLOADING
};

enum EChatState{
	MS_NONE,
	MS_CHATTING,
	MS_CONNECTING,
	MS_UNABLETOCONNECT
};

enum EKadState{
	KS_NONE,
	KS_QUEUED_FWCHECK,
	KS_CONNECTING_FWCHECK,
	KS_CONNECTED_FWCHECK,
	KS_QUEUED_BUDDY,
	KS_INCOMING_BUDDY,
	KS_CONNECTING_BUDDY,
	KS_CONNECTED_BUDDY,
	KS_QUEUED_FWCHECK_UDP,
	KS_FWCHECK_UDP,
	KS_CONNECTING_FWCHECK_UDP
};

enum EClientSoftware{
	SO_EMULE			= 0,	// default
	SO_CDONKEY			= 1,	// ET_COMPATIBLECLIENT
	SO_XMULE			= 2,	// ET_COMPATIBLECLIENT
	SO_AMULE			= 3,	// ET_COMPATIBLECLIENT
	SO_SHAREAZA			= 4,	// ET_COMPATIBLECLIENT
	SO_MLDONKEY			= 10,	// ET_COMPATIBLECLIENT
	SO_LPHANT			= 20,	// ET_COMPATIBLECLIENT
	// other client types which are not identified with ET_COMPATIBLECLIENT
	SO_EDONKEYHYBRID	= 50,
	SO_EDONKEY,
	SO_OLDEMULE,
	SO_URL,
	SO_UNKNOWN
};

enum ESecureIdentState{
	IS_UNAVAILABLE		= 0,
	IS_ALLREQUESTSSEND  = 0,
	IS_SIGNATURENEEDED	= 1,
	IS_KEYANDSIGNEEDED	= 2,
};

enum EInfoPacketState{
	IP_NONE				= 0,
	IP_EDONKEYPROTPACK  = 1,
	IP_EMULEPROTPACK	= 2,
	IP_BOTH				= 3,
};

enum ESourceFrom{
	SF_SERVER			= 0,
	SF_KADEMLIA			= 1,
	SF_SOURCE_EXCHANGE	= 2,
	SF_PASSIVE			= 3,
	SF_LINK				= 4
};

enum EChatCaptchaState{
	CA_NONE				= 0,
	CA_CHALLENGESENT,
	CA_CAPTCHASOLVED,
	CA_ACCEPTING,
	CA_CAPTCHARECV,
	CA_SOLUTIONSENT
};

enum EConnectingState{
	CCS_NONE				= 0,
	CCS_DIRECTTCP,
	CCS_DIRECTCALLBACK,
	CCS_KADCALLBACK,
	CCS_SERVERCALLBACK,
	CCS_PRECONDITIONS
};

#ifdef _DEBUG
	// use the 'Enums' only for debug builds, each enum costs 4 bytes (3 unused)
#define _EClientSoftware	EClientSoftware
#define _EChatState			EChatState
#define _EKadState			EKadState
#define _ESecureIdentState	ESecureIdentState
#define _EUploadState		EUploadState
#define _EDownloadState		EDownloadState
#define _ESourceFrom		ESourceFrom
#define _EChatCaptchaState  EChatCaptchaState
#define _EConnectingState	EConnectingState
#else
#define _EClientSoftware	uint8
#define _EChatState			uint8
#define _EKadState			uint8
#define _ESecureIdentState	uint8
#define _EUploadState		uint8
#define _EDownloadState		uint8
#define _ESourceFrom		uint8
#define _EChatCaptchaState	uint8
#define _EConnectingState	uint8
#endif