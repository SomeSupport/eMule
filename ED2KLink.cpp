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
#include <wininet.h>
#include "resource.h"
#include "ED2KLink.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "StringConversion.h"
#include "opcodes.h"
#include "preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace {
	struct autoFree {
		autoFree(TCHAR* p) : m_p(p) {}
		~autoFree() { free(m_p); }
	private:
		TCHAR * m_p;
	};
	inline unsigned int FromHexDigit(TCHAR digit) {
		switch (digit) {
		case _T('0'): return 0;
		case _T('1'): return 1;
		case _T('2'): return 2;
		case _T('3'): return 3;
		case _T('4'): return 4;
		case _T('5'): return 5;
		case _T('6'): return 6;
		case _T('7'): return 7;
		case _T('8'): return 8;
		case _T('9'): return 9;
		case _T('A'): return 10;
		case _T('B'): return 11;
		case _T('C'): return 12;
		case _T('D'): return 13;
		case _T('E'): return 14;
		case _T('F'): return 15;
		case _T('a'): return 10;
		case _T('b'): return 11;
		case _T('c'): return 12;
		case _T('d'): return 13;
		case _T('e'): return 14;
		case _T('f'): return 15;
		default: throw GetResString(IDS_ERR_ILLFORMEDHASH);
		}
	}
}

CED2KLink::~CED2KLink()
{
}

///////////////////////////////////////////// 
// CED2KServerListLink implementation 
///////////////////////////////////////////// 
CED2KServerListLink::CED2KServerListLink(const TCHAR* address)
{
	m_address = address;
}

CED2KServerListLink::~CED2KServerListLink()
{
} 

void CED2KServerListLink::GetLink(CString& lnk) const
{
	lnk.Format(_T("ed2k://|serverlist|%s|/"), m_address);
}

CED2KServerListLink* CED2KServerListLink::GetServerListLink()
{
	return this;
}

CED2KServerLink* CED2KServerListLink::GetServerLink()
{
	return NULL;
}

CED2KFileLink* CED2KServerListLink::GetFileLink()
{
	return NULL;
}

CED2KLink::LinkType CED2KServerListLink::GetKind() const
{
	return kServerList;
}

///////////////////////////////////////////// 
// CED2KNodesListLink implementation 
///////////////////////////////////////////// 
CED2KNodesListLink::CED2KNodesListLink(const TCHAR* address)
{
	m_address = address;
}

CED2KNodesListLink::~CED2KNodesListLink()
{
} 

void CED2KNodesListLink::GetLink(CString& lnk) const
{
	lnk.Format(_T("ed2k://|nodeslist|%s|/"), m_address);
}

CED2KServerListLink* CED2KNodesListLink::GetServerListLink()
{
	return NULL;
}

CED2KNodesListLink* CED2KNodesListLink::GetNodesListLink()
{
	return this;
}

CED2KServerLink* CED2KNodesListLink::GetServerLink()
{
	return NULL;
}

CED2KFileLink* CED2KNodesListLink::GetFileLink()
{
	return NULL;
}

CED2KLink::LinkType CED2KNodesListLink::GetKind() const
{
	return kNodesList;
}

/////////////////////////////////////////////
// CED2KServerLink implementation
/////////////////////////////////////////////
CED2KServerLink::CED2KServerLink(const TCHAR* ip, const TCHAR* port)
{
	m_strAddress = ip;
	unsigned long ul = _tcstoul(port, 0, 10);
	if (ul > 0xFFFF)
		throw GetResString(IDS_ERR_BADPORT);
	m_port = static_cast<uint16>(ul);
	m_defaultName = _T("Server ");
	m_defaultName += ip;
	m_defaultName += _T(":");
	m_defaultName += port;
}

CED2KServerLink::~CED2KServerLink()
{
}

void CED2KServerLink::GetLink(CString& lnk) const
{
	lnk.Format(_T("ed2k://|server|%s|%u|/"), GetAddress(), (UINT)GetPort());
}

CED2KServerListLink* CED2KServerLink::GetServerListLink() 
{ 
	return NULL;
}

CED2KServerLink* CED2KServerLink::GetServerLink() 
{ 
	return this;
}

CED2KFileLink* CED2KServerLink::GetFileLink() 
{ 
	return NULL;
}

CED2KLink::LinkType CED2KServerLink::GetKind() const
{
	return kServer;
}

///////////////////////////////////////////// 
// CED2KSearchLink implementation 
///////////////////////////////////////////// 
CED2KSearchLink::CED2KSearchLink(const TCHAR* pszSearchTerm)
{
	m_strSearchTerm = OptUtf8ToStr(URLDecode(pszSearchTerm));
}

CED2KSearchLink::~CED2KSearchLink()
{
} 

void CED2KSearchLink::GetLink(CString& lnk) const
{
	lnk.Format(_T("ed2k://|search|%s|/"), EncodeUrlUtf8(m_strSearchTerm));
}

CED2KServerListLink* CED2KSearchLink::GetServerListLink()
{
	return NULL;
}

CED2KSearchLink* CED2KSearchLink::GetSearchLink()
{
	return this;
}

CED2KServerLink* CED2KSearchLink::GetServerLink()
{
	return NULL;
}

CED2KFileLink* CED2KSearchLink::GetFileLink()
{
	return NULL;
}

CED2KNodesListLink* CED2KSearchLink::GetNodesListLink()
{
	return NULL;
}

CED2KLink::LinkType CED2KSearchLink::GetKind() const
{
	return kSearch;
}


/////////////////////////////////////////////
// CED2KFileLink implementation
/////////////////////////////////////////////
CED2KFileLink::CED2KFileLink(const TCHAR* pszName, const TCHAR* pszSize, const TCHAR* pszHash, 
							 const CStringArray& astrParams, const TCHAR* pszSources)
	: m_size(pszSize)
{
	// Here we have a little problem.. Actually the proper solution would be to decode from UTF8,
	// only if the string does contain escape sequences. But if user pastes a raw UTF8 encoded
	// string (for whatever reason), we would miss to decode that string. On the other side, 
	// always decoding UTF8 can give flaws in case the string is valid for Unicode and UTF8
	// at the same time. However, to avoid the pasting of raw UTF8 strings (which would lead
	// to a greater mess in the network) we always try to decode from UTF8, even if the string
	// did not contain escape sequences.
	m_name = OptUtf8ToStr(URLDecode(pszName));
	m_name.Trim();
	if (m_name.IsEmpty())
		throw GetResString(IDS_ERR_NOTAFILELINK);

	SourcesList = NULL;
	m_hashset = NULL;
	m_bAICHHashValid = false;

	if (_tcslen(pszHash) != 32)
		throw GetResString(IDS_ERR_ILLFORMEDHASH);

	if (_tstoi64(pszSize) >= MAX_EMULE_FILE_SIZE)
		throw GetResString(IDS_ERR_TOOLARGEFILE);
	if (_tstoi64(pszSize)<=0)
		throw GetResString(IDS_ERR_NOTAFILELINK);
	if (_tstoi64(pszSize) > OLD_MAX_EMULE_FILE_SIZE && !thePrefs.CanFSHandleLargeFiles(0))
		throw GetResString(IDS_ERR_FSCANTHANDLEFILE);
	
	for (int idx = 0; idx < 16; ++idx) {
		m_hash[idx] = (BYTE)(FromHexDigit(*pszHash++)*16);
		m_hash[idx] = (BYTE)(m_hash[idx] + FromHexDigit(*pszHash++));
	}

	bool bError = false;
	for (int i = 0; !bError && i < astrParams.GetCount(); i++)
	{
		const CString& strParam = astrParams.GetAt(i);
		ASSERT( !strParam.IsEmpty() );

		CString strTok;
		int iPos = strParam.Find(_T('='));
		if (iPos != -1)
			strTok = strParam.Left(iPos);
		if (strTok == _T("s"))
		{
			CString strURL = strParam.Mid(iPos + 1);
			if (!strURL.IsEmpty())
			{
				TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
				TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
				TCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH];
				TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
				TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
				TCHAR szExtraInfo[INTERNET_MAX_URL_LENGTH];
				URL_COMPONENTS Url = {0};
				Url.dwStructSize = sizeof(Url);
				Url.lpszScheme = szScheme;
				Url.dwSchemeLength = ARRSIZE(szScheme);
				Url.lpszHostName = szHostName;
				Url.dwHostNameLength = ARRSIZE(szHostName);
				Url.lpszUserName = szUserName;
				Url.dwUserNameLength = ARRSIZE(szUserName);
				Url.lpszPassword = szPassword;
				Url.dwPasswordLength = ARRSIZE(szPassword);
				Url.lpszUrlPath = szUrlPath;
				Url.dwUrlPathLength = ARRSIZE(szUrlPath);
				Url.lpszExtraInfo = szExtraInfo;
				Url.dwExtraInfoLength = ARRSIZE(szExtraInfo);
				if (InternetCrackUrl(strURL, 0, 0, &Url) && Url.dwHostNameLength > 0 && Url.dwHostNameLength < INTERNET_MAX_HOST_NAME_LENGTH)
				{
					SUnresolvedHostname* hostname = new SUnresolvedHostname;
					hostname->strURL = strURL;
					hostname->strHostname = szHostName;
					m_HostnameSourcesList.AddTail(hostname);
				}
			}
			else
				ASSERT(0);
		}
		else if (strTok == _T("p"))
		{
			CString strPartHashs = strParam.Tokenize(_T("="), iPos);

			if (m_hashset != NULL){
				ASSERT(0);
				bError = true;
				break;
			}

			m_hashset = new CSafeMemFile(256);
			m_hashset->WriteHash16(m_hash);
			m_hashset->WriteUInt16(0);

			int iPartHashs = 0;
			int iPosPH = 0;
			CString strHash = strPartHashs.Tokenize(_T(":"), iPosPH);
			while (!strHash.IsEmpty())
			{
				uchar aucPartHash[16];
				if (!strmd4(strHash, aucPartHash)){
					bError = true;
					break;
				}
				m_hashset->WriteHash16(aucPartHash);
				iPartHashs++;
				strHash = strPartHashs.Tokenize(_T(":"), iPosPH);
			}
			if (bError)
				break;

			m_hashset->Seek(16, CFile::begin);
			m_hashset->WriteUInt16((uint16)iPartHashs);
			m_hashset->Seek(0, CFile::begin);
		}
		else if (strTok == _T("h"))
		{
			CString strHash = strParam.Mid(iPos + 1);
			if (!strHash.IsEmpty())
			{
				if (DecodeBase32(strHash, m_AICHHash.GetRawHash(), CAICHHash::GetHashSize()) == (UINT)CAICHHash::GetHashSize()){
					m_bAICHHashValid = true;
					ASSERT( m_AICHHash.GetString().CompareNoCase(strHash) == 0 );
				}
				else
					ASSERT( false );
			}
			else
				ASSERT( false );
		}
		else
			ASSERT(0);
	}

	if (bError)
	{
		delete m_hashset;
		m_hashset = NULL;
	}

	if (pszSources)
	{
		TCHAR* pNewString = _tcsdup(pszSources);
		autoFree liberator(pNewString);
		TCHAR* pCh = pNewString;
		TCHAR* pEnd;
		TCHAR* pIP;
		TCHAR* pPort;

		bool bAllowSources;
		TCHAR date[3];
		COleDateTime expirationDate;
		int nYear,nMonth,nDay;

		uint16 nCount = 0;
		uint32 dwID;
		uint16 nPort;
		uint32 dwServerIP = 0; 
		uint16 nServerPort = 0;
		unsigned long ul;

		int nInvalid = 0;

		pCh = _tcsstr( pCh, _T("sources") );
		if( pCh != NULL ) {
			pCh = pCh + 7; // point to char after "sources"
			pEnd = pCh;
			while( *pEnd ) pEnd++; // make pEnd point to the terminating NULL
			bAllowSources=true;
			// if there's an expiration date...
			if( *pCh == _T('@') && (pEnd-pCh) > 7 )
			{
				pCh++; // after '@'
				date[2] = 0; // terminate the two character string
				date[0] = *(pCh++); date[1] = *(pCh++);
				nYear = _tcstol( date, 0, 10 ) + 2000;
				date[0] = *(pCh++); date[1] = *(pCh++);
				nMonth = _tcstol( date, 0, 10 );
				date[0] = *(pCh++); date[1] = *(pCh++);
				nDay = _tcstol( date, 0, 10 );
				bAllowSources = ( expirationDate.SetDate(nYear,nMonth,nDay) == 0 );
				if (bAllowSources) bAllowSources=(COleDateTime::GetCurrentTime() < expirationDate);
			}

			// increment pCh to point to the first "ip:port" and check for sources
			if ( bAllowSources && ++pCh < pEnd ) {
				SourcesList = new CSafeMemFile(256);
				SourcesList->WriteUInt16(nCount); // init to 0, we'll fix this at the end.
				// for each "ip:port" source string until the end
				// limit to prevent overflow (uint16 due to CPartFile::AddClientSources)
				while( *pCh != 0 && nCount < MAXSHORT ) {
					pIP = pCh;
					// find the end of this ip:port string & start of next ip:port string.
					if( (pCh = _tcschr(pCh, _T(','))) != NULL ) {
						*pCh = 0; // terminate current "ip:port"
						pCh++; // point to next "ip:port"
					}
					else
						pCh = pEnd;

					// if port is not present for this ip, go to the next ip.
					if( (pPort = _tcschr(pIP, _T(':'))) == NULL )
					{	nInvalid++;	continue;	}

					*pPort = 0;	// terminate ip string
					pPort++;	// point pPort to port string.

					dwID = inet_addr(CStringA(pIP));
					ul = _tcstoul( pPort, 0, 10 );
					nPort = static_cast<uint16>(ul);

					// skip bad ips / ports
					if (ul > 0xFFFF || ul == 0 )	// port
					{	nInvalid++;	continue;	}
					if( dwID == INADDR_NONE) {	// hostname?
						if (_tcslen(pIP) > 512)
						{	nInvalid++;	continue;	}
						SUnresolvedHostname* hostname = new SUnresolvedHostname;
						hostname->nPort = nPort;
						hostname->strHostname = pIP;
						m_HostnameSourcesList.AddTail(hostname);
						continue;
					}
					//TODO: This will filter out *.*.*.0 clients. Is there a nice way to fix?
					if( IsLowID(dwID) )	// ip
					{	nInvalid++;	continue;	}

					SourcesList->WriteUInt32(dwID);
					SourcesList->WriteUInt16(nPort);
					SourcesList->WriteUInt32(dwServerIP);
					SourcesList->WriteUInt16(nServerPort);
					nCount++;
				}
				SourcesList->SeekToBegin();
				SourcesList->WriteUInt16(nCount);
				SourcesList->SeekToBegin();
				if (nCount==0) {
					delete SourcesList;
					SourcesList=NULL;
				}
			}
		}
	}
}

CED2KFileLink::~CED2KFileLink()
{
	delete SourcesList;
	while (!m_HostnameSourcesList.IsEmpty())
		delete m_HostnameSourcesList.RemoveHead();
	delete m_hashset;
}

void CED2KFileLink::GetLink(CString& lnk) const
{
	lnk = _T("ed2k://|file|");
	lnk += EncodeUrlUtf8(m_name);
	lnk += _T("|");
	lnk += m_size;
	lnk += _T("|");
	for (int idx=0; idx != 16 ; ++idx ) {
		unsigned int ui1 = m_hash[idx] / 16;
		unsigned int ui2 = m_hash[idx] % 16;
		lnk+= static_cast<TCHAR>( ui1 > 9 ? (_T('0')+ui1) : (_T('A')+(ui1-10)) );
		lnk+= static_cast<TCHAR>( ui2 > 9 ? (_T('0')+ui2) : (_T('A')+(ui2-10)) );
	}
	lnk += _T("|/");
}

CED2KServerListLink* CED2KFileLink::GetServerListLink()
{
	return NULL;
}

CED2KServerLink* CED2KFileLink::GetServerLink()
{
	return NULL;
}

CED2KFileLink* CED2KFileLink::GetFileLink()
{
	return this;
}

CED2KLink::LinkType CED2KFileLink::GetKind() const
{
	return kFile;
}

CED2KLink* CED2KLink::CreateLinkFromUrl(const TCHAR* uri)
{
	CString strURI(uri);
	strURI.Trim(); // This function is used for various sources, trim the string again.
	int iPos = 0;
	CString strTok = GetNextString(strURI, _T("|"), iPos);
	if (strTok.CompareNoCase(_T("ed2k://")) == 0)
	{
		strTok = GetNextString(strURI, _T("|"), iPos);
		if (strTok == _T("file"))
		{
			CString strName = GetNextString(strURI, _T("|"), iPos);
			if (!strName.IsEmpty())
			{
				CString strSize = GetNextString(strURI, _T("|"), iPos);
				if (!strSize.IsEmpty())
				{
					CString strHash = GetNextString(strURI, _T("|"), iPos);
					if (!strHash.IsEmpty())
					{
						CStringArray astrEd2kParams;
						bool bEmuleExt = false;
						CString strEmuleExt;

						CString strLastTok;
						strTok = GetNextString(strURI, _T("|"), iPos);
						while (!strTok.IsEmpty())
						{
							strLastTok = strTok;
							if (strTok == _T("/"))
							{
								if (bEmuleExt)
									break;
								bEmuleExt = true;
							}
							else
							{
								if (bEmuleExt)
								{
									if (!strEmuleExt.IsEmpty())
										strEmuleExt += _T('|');
									strEmuleExt += strTok;
								}
								else
									astrEd2kParams.Add(strTok);
							}
							strTok = GetNextString(strURI, _T("|"), iPos);
						}

						if (strLastTok == _T("/"))
							return new CED2KFileLink(strName, strSize, strHash, astrEd2kParams, strEmuleExt.IsEmpty() ? (LPCTSTR)NULL : (LPCTSTR)strEmuleExt);
					}
				}
			}
		}
		else if (strTok == _T("serverlist"))
		{
			CString strURL = GetNextString(strURI, _T("|"), iPos);
			if (!strURL.IsEmpty() && GetNextString(strURI, _T("|"), iPos) == _T("/"))
				return new CED2KServerListLink(strURL);
		}
		else if (strTok == _T("server"))
		{
			CString strServer = GetNextString(strURI, _T("|"), iPos);
			if (!strServer.IsEmpty())
			{
				CString strPort = GetNextString(strURI, _T("|"), iPos);
				if (!strPort.IsEmpty() && GetNextString(strURI, _T("|"), iPos) == _T("/"))
					return new CED2KServerLink(strServer, strPort);
			}
		}
		else if (strTok == _T("nodeslist"))
		{
			CString strURL = GetNextString(strURI, _T("|"), iPos);
			if (!strURL.IsEmpty() && GetNextString(strURI, _T("|"), iPos) == _T("/"))
				return new CED2KNodesListLink(strURL);
		}
		else if (strTok == _T("search"))
		{
			CString strSearchTerm = GetNextString(strURI, _T("|"), iPos);
			// might be extended with more parameters in future versions
			if (!strSearchTerm.IsEmpty())
				return new CED2KSearchLink(strSearchTerm);
		}
	}

	throw GetResString(IDS_ERR_NOSLLINK);
}
