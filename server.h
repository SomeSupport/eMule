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

class CTag;
class CFileDataIO;

#pragma pack(1)
struct ServerMet_Struct {
	uint32	ip;
	uint16	port;
	uint32	tagcount;
};
#pragma pack()

#define SRV_PR_LOW			2
#define SRV_PR_NORMAL		0
#define SRV_PR_HIGH			1


// Server TCP flags
#define	SRV_TCPFLG_COMPRESSION		0x00000001
#define	SRV_TCPFLG_NEWTAGS			0x00000008
#define	SRV_TCPFLG_UNICODE			0x00000010
#define SRV_TCPFLG_RELATEDSEARCH	0x00000040
#define SRV_TCPFLG_TYPETAGINTEGER	0x00000080
#define SRV_TCPFLG_LARGEFILES		0x00000100
#define SRV_TCPFLG_TCPOBFUSCATION	0x00000400

// Server UDP flags
#define	SRV_UDPFLG_EXT_GETSOURCES	0x00000001
#define	SRV_UDPFLG_EXT_GETFILES		0x00000002
#define	SRV_UDPFLG_NEWTAGS			0x00000008
#define	SRV_UDPFLG_UNICODE			0x00000010
#define	SRV_UDPFLG_EXT_GETSOURCES2	0x00000020
#define SRV_UDPFLG_LARGEFILES		0x00000100
#define SRV_UDPFLG_UDPOBFUSCATION	0x00000200
#define SRV_UDPFLG_TCPOBFUSCATION	0x00000400

class CServer{
public:
	CServer(const ServerMet_Struct* in_data);
	CServer(uint16 in_port, LPCTSTR i_addr);
	CServer(const CServer* pOld);
	~CServer();

	bool	AddTagFromFile(CFileDataIO* servermet);

	const CString& GetListName() const				{return m_strName;}
	void	SetListName(LPCTSTR newname);

	const CString& GetDescription() const			{return m_strDescription;}
	void	SetDescription(LPCTSTR newdescription);

	uint32	GetIP() const							{return ip;}
	void	SetIP(uint32 newip);

	const CString& GetDynIP() const					{return m_strDynIP;}
	bool	HasDynIP() const						{return !m_strDynIP.IsEmpty();}
	void	SetDynIP(LPCTSTR newdynip);

	LPCTSTR	GetFullIP() const						{return ipfull;}
	LPCTSTR	GetAddress() const;
	uint16	GetPort() const							{return port;}

	uint32	GetFiles() const						{return files;}
	void	SetFileCount(uint32 in_files)			{files = in_files;}

	uint32	GetUsers() const						{return users;}
	void	SetUserCount(uint32 in_users)			{users = in_users;}

	UINT	GetPreference() const					{return m_uPreference;}
	void	SetPreference(UINT uPreference)			{m_uPreference = uPreference;}

	uint32	GetPing() const							{return ping;}
	void	SetPing(uint32 in_ping)					{ping = in_ping;}

	uint32	GetMaxUsers() const						{return maxusers;}
	void	SetMaxUsers(uint32 in_maxusers) 		{maxusers = in_maxusers;}

	uint32	GetFailedCount() const					{return failedcount;}
	void	SetFailedCount(uint32 nCount)			{failedcount = nCount;}
	void	AddFailedCount()						{failedcount++;} 
	void	ResetFailedCount()						{failedcount = 0;}

	uint32	GetLastPingedTime() const				{return lastpingedtime;}
	void	SetLastPingedTime(uint32 in_lastpingedtime)	{lastpingedtime = in_lastpingedtime;}

	uint32	GetRealLastPingedTime() const					{return m_dwRealLastPingedTime;} // last pinged time without any random modificator
	void	SetRealLastPingedTime(uint32 in_lastpingedtime)	{m_dwRealLastPingedTime = in_lastpingedtime;}

	uint32	GetLastPinged() const					{return lastpinged;}
	void	SetLastPinged(uint32 in_lastpinged)		{lastpinged = in_lastpinged;}

	UINT	GetLastDescPingedCount() const			{return lastdescpingedcout;}
	void	SetLastDescPingedCount(bool reset);

	bool	IsStaticMember() const					{return staticservermember;}
	void	SetIsStaticMember(bool in)				{staticservermember = in;}

	uint32	GetChallenge() const					{return challenge;}
	void	SetChallenge(uint32 in_challenge)		{challenge = in_challenge;}

	uint32	GetDescReqChallenge() const				{return m_uDescReqChallenge;}
	void	SetDescReqChallenge(uint32 uDescReqChallenge) {m_uDescReqChallenge = uDescReqChallenge;}

	uint32	GetSoftFiles() const					{return softfiles;}
	void	SetSoftFiles(uint32 in_softfiles)		{softfiles = in_softfiles;}

	uint32	GetHardFiles() const					{return hardfiles;}
	void	SetHardFiles(uint32 in_hardfiles)		{hardfiles = in_hardfiles;}

	const CString& GetVersion() const				{return m_strVersion;}
	void	SetVersion(LPCTSTR pszVersion)			{m_strVersion = pszVersion;}

	uint32	GetTCPFlags() const						{return m_uTCPFlags;}
	void	SetTCPFlags(uint32 uFlags)				{m_uTCPFlags = uFlags;}

	uint32	GetUDPFlags() const						{return m_uUDPFlags;}
	void	SetUDPFlags(uint32 uFlags)				{m_uUDPFlags = uFlags;}

	uint32	GetLowIDUsers() const					{return m_uLowIDUsers;}
	void	SetLowIDUsers(uint32 uLowIDUsers)		{m_uLowIDUsers = uLowIDUsers;}

	uint16	GetObfuscationPortTCP() const			{return m_nObfuscationPortTCP;}
	void	SetObfuscationPortTCP(uint16 nPort)		{m_nObfuscationPortTCP = nPort;}

	uint16	GetObfuscationPortUDP() const			{return m_nObfuscationPortUDP;}
	void	SetObfuscationPortUDP(uint16 nPort)		{m_nObfuscationPortUDP = nPort;}

	uint32	GetServerKeyUDP(bool bForce = false) const;
	void	SetServerKeyUDP(uint32 dwServerKeyUDP);

	bool	GetCryptPingReplyPending() const		{return m_bCryptPingReplyPending;}
	void	SetCryptPingReplyPending(bool bVal)		{m_bCryptPingReplyPending = bVal;}

	uint32	GetServerKeyUDPIP() const				{return m_dwIPServerKeyUDP;}

	bool	GetUnicodeSupport() const				{return (GetTCPFlags() & SRV_TCPFLG_UNICODE)!=0;}
	bool	GetRelatedSearchSupport() const			{return (GetTCPFlags() & SRV_TCPFLG_RELATEDSEARCH)!=0;}
	bool	SupportsLargeFilesTCP() const			{return (GetTCPFlags() & SRV_TCPFLG_LARGEFILES)!=0;}
	bool	SupportsLargeFilesUDP() const			{return (GetUDPFlags() & SRV_UDPFLG_LARGEFILES)!=0;}
	bool	SupportsObfuscationUDP() const			{return (GetUDPFlags() & SRV_UDPFLG_UDPOBFUSCATION)!=0;}
	bool	SupportsObfuscationTCP() const			{return GetObfuscationPortTCP() != 0 && ((GetUDPFlags() & SRV_UDPFLG_TCPOBFUSCATION)!=0 || (GetTCPFlags() & SRV_TCPFLG_TCPOBFUSCATION)!=0);}
	bool	SupportsGetSourcesObfuscation() const	{return (GetTCPFlags() & SRV_TCPFLG_TCPOBFUSCATION)!=0;} // mapped to TCPFLAG_TCPOBFU

	bool	IsEqual(const CServer* pServer) const;

private:
	uint32		challenge;
	uint32		m_uDescReqChallenge;
	uint32		lastpinged; //This is to get the ping delay.
	uint32		lastpingedtime; //This is to decided when we retry the ping.
	uint32		lastdescpingedcout;
	uint32		files;
	uint32		users;
	uint32		maxusers;
	uint32		softfiles;
	uint32		hardfiles;
	UINT		m_uPreference;
	uint32		ping;
	CString		m_strDescription;
	CString		m_strName;
	CString		m_strDynIP;
	TCHAR		ipfull[3+1+3+1+3+1+3+1]; // 16
	uint32		ip;
	uint16		port;
	bool		staticservermember;
	bool		m_bCryptPingReplyPending;
	uint32		failedcount; 
	CString		m_strVersion;
	uint32		m_uTCPFlags;
	uint32		m_uUDPFlags;
	uint32		m_uLowIDUsers;
	uint32		m_dwServerKeyUDP;
	uint32		m_dwIPServerKeyUDP;
	uint16		m_nObfuscationPortTCP;
	uint16		m_nObfuscationPortUDP;
	uint32		m_dwRealLastPingedTime;
};