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
#include "emuleDlg.h"
#include "TaskbarNotifier.h"
#include "OtherFunctions.h"
#include <atlsmtpconnection.h>
#include "Log.h"
#include "Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// CMimeRawAttachmentEx -- bug fixed version of CMimeRawAttachment

class CMimeRawAttachmentEx : public CMimeRawAttachment
{
protected:
	virtual BOOL MakeMimeHeader(CStringA& header, LPCSTR szBoundary) throw()
	{
		// WINBUG: Original code from 'CMimeRawAttachment' does not do what the comment below is announcing..
		// if no display name is specified, default to "rawdata"
		return MakeMimeHeader(header, szBoundary, m_szDisplayName[0] != _T('\0') ? m_szDisplayName : _T("rawdata"));
	}

	// Make the MIME header with the specified filename
	virtual BOOL MakeMimeHeader(CStringA& header, LPCSTR szBoundary, LPCTSTR szFileName) throw()
	{
		ATLASSERT(szBoundary != NULL);
		ATLASSERT(szFileName != NULL);
		ATLASSERT(m_pszEncodeString != NULL);

		char szBegin[256];
		if (*szBoundary)
		{
			// this is not the only body part
			memcpy(szBegin, "\r\n\r\n--", 6);
			memcpy(szBegin+6, szBoundary, ATL_MIME_BOUNDARYLEN);
			*(szBegin+(ATL_MIME_BOUNDARYLEN+6)) = '\0';
		}
		else
		{
			// this is the only body part, so output the MIME header
			memcpy(szBegin, "MIME-Version: 1.0", sizeof("MIME-Version: 1.0"));
		}

		// Get file name with the path stripped out
		TCHAR szFile[MAX_PATH+1];
		TCHAR szExt[_MAX_EXT+1];
		_tsplitpath(szFileName, NULL, NULL, szFile, szExt);
		_tcscat(szFile, szExt);

		_ATLTRY
		{
			CT2CAEX<MAX_PATH+1> szFileNameA(szFile);

			// WINBUG: Original code from 'CMimeRawAttachment' has 2 bugs:
			//	- there is no ';' character after the "charset" token
			//	- the "charset" token is not to be sent at all in case of a raw attachment
			header.Format("%s\r\nContent-Type: %s;\r\n\tname=\"%s\"\r\n"
						 "Content-Transfer-Encoding: %s\r\nContent-Disposition: attachment;\r\n\tfilename=\"%s\"\r\n\r\n",
				szBegin, (LPCSTR) m_ContentType, (LPCSTR) szFileNameA, m_pszEncodeString, (LPCSTR) szFileNameA);
			return TRUE;
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}
	}
};


///////////////////////////////////////////////////////////////////////////////
// CMimeMessageEx -- needed to access the bug fixed version of CMimeRawAttachmentEx

#pragma warning(disable:4701) // local variable 'pRawAttach' may be used without having been initialized
class CMimeMessageEx : public CMimeMessage
{
public:
	BOOL AttachRaw(void* pRawData, DWORD dwDataLength, int nEncodingScheme = ATLSMTP_BASE64_ENCODE, BOOL bCopyData = TRUE, 
		LPCTSTR szDisplayName = NULL, LPCTSTR szContentType = _T("application/octet-stream"), UINT uiCodepage = 0)
	{
		if (!pRawData)
			return FALSE;

		CAutoPtr<CMimeBodyPart> spRawAttach;
		CMimeRawAttachmentEx* pRawAttach;
		ATLTRY(spRawAttach.Attach(pRawAttach = new CMimeRawAttachmentEx()));
		if (!spRawAttach)
		{
			return FALSE;
		}

		BOOL bRet = pRawAttach->Initialize(pRawData, dwDataLength, bCopyData, szDisplayName, m_spMultiLanguage, uiCodepage);

		if (bRet)
			bRet = pRawAttach->SetEncodingScheme(nEncodingScheme);
		if (bRet)
			bRet = pRawAttach->SetContentType(szContentType);

		_ATLTRY
		{
			if (bRet)
				if(!m_BodyParts.AddTail(spRawAttach))
					bRet = FALSE;
		}
		_ATLCATCHALL()
		{
			bRet = FALSE;
		}

		return bRet;
	}
};
#pragma warning(default:4701)

// Print the hash in a format which is similar to CertMgr's..
CString GetCertHash(const BYTE* pucHash, int iBytes)
{
	const static LPCTSTR pszHex = _T("0123456789abcdef");
	CString strHash;
	LPTSTR pszHash = strHash.GetBuffer(iBytes * 3);
	for (int i = 0; i < iBytes; i++, pucHash++)
	{
		*pszHash++ = pszHex[*pucHash >> 4];
		*pszHash++ = pszHex[*pucHash & 0xf];
		*pszHash++ = _T(' ');
	}
	*pszHash = _T('\0');
	strHash.ReleaseBuffer();
	return strHash;
}

// Print the 'integer' in a format which is similar to CertMgr's..
CString GetCertInteger(const BYTE* pucBlob, int cbBlob)
{
	CString strInteger;
	for (int i = cbBlob - 1; i >= 0; i--)
		strInteger.AppendFormat(_T("%02x "), pucBlob[i]);
	return strInteger;
}

PCCERT_CONTEXT GetCertificate(HCERTSTORE hCertStore, DWORD dwFindType, LPCWSTR pszCertName)
{
	PCCERT_CONTEXT pCertContext = NULL;
	pCertContext = CertFindCertificateInStore(hCertStore, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 0,
											  dwFindType, pszCertName, NULL);
	if (thePrefs.GetVerbose() && pCertContext)
	{
		DebugLog(LOG_DONTNOTIFY, _T("E-Mail Encryption: Found certificate"));

		TCHAR szString[512] = {0};
		PCERT_INFO pCertInfo = pCertContext->pCertInfo;
		if (pCertInfo)
		{
			if (CertNameToStr(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &pCertInfo->Subject, CERT_X500_NAME_STR, szString, ARRSIZE(szString)))
				DebugLog(LOG_DONTNOTIFY, _T("E-Mail Encryption: Subject: %s"), szString);

			if (pCertInfo->SerialNumber.cbData && pCertInfo->SerialNumber.pbData)
				DebugLog(LOG_DONTNOTIFY, _T("E-Mail Encryption: Serial nr.: %s"), GetCertInteger(pCertInfo->SerialNumber.pbData, pCertInfo->SerialNumber.cbData));

			if (CertNameToStr(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &pCertInfo->Issuer, CERT_X500_NAME_STR, szString, ARRSIZE(szString)))
				DebugLog(LOG_DONTNOTIFY, _T("E-Mail Encryption: Issuer: %s"), szString);
		}
		else
		{
			if (CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_DISABLE_IE4_UTF8_FLAG, szOID_COMMON_NAME, szString, ARRSIZE(szString)))
				DebugLog(LOG_DONTNOTIFY, _T("E-Mail Encryption: Name: %s"), szString);

			BYTE md5[16] = {0};
			DWORD cb = sizeof md5;
			if (CertGetCertificateContextProperty(pCertContext, CERT_MD5_HASH_PROP_ID, md5, &cb) && cb == sizeof md5)
				DebugLog(LOG_DONTNOTIFY, _T("E-Mail Encryption: MD5 hash: %s"), GetCertHash(md5, cb));

			BYTE sha1[20] = {0};
			cb = sizeof sha1;
			if (CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, sha1, &cb) && cb == sizeof sha1)
				DebugLog(LOG_DONTNOTIFY, _T("E-Mail Encryption: SHA1 hash: %s"), GetCertHash(sha1, cb));
		}
	}

	return pCertContext;
}

bool Encrypt(const CStringA& rstrContentA, CByteArray& raEncrypted, LPCWSTR pwszCertSubject)
{
	LPCTSTR pszContainer = AfxGetAppName();
	HCRYPTPROV hCryptProv;
	if (!CryptAcquireContext(&hCryptProv, pszContainer, NULL, PROV_RSA_FULL, NULL))
	{
		DWORD dwError = GetLastError();
		if (dwError != NTE_BAD_KEYSET) {
			DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Encryption: Failed to acquire certificate context container '%s' - %s"), pszContainer, GetErrorMessage(dwError, 1));
			return false;
		}
		if (!CryptAcquireContext(&hCryptProv, pszContainer, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
			DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Encryption: Failed to create certificate context container '%s' - %s"), pszContainer, GetErrorMessage(GetLastError(), 1));
			return false;
		}
	}

	LPCTSTR pszCertStore = _T("addressbook");
	HCERTSTORE hStoreHandle = CertOpenSystemStore(hCryptProv, pszCertStore);
	if (hStoreHandle)
	{
		PCCERT_CONTEXT pRecipientCert = GetCertificate(hStoreHandle, CERT_FIND_SUBJECT_STR, pwszCertSubject);
		if (pRecipientCert)
		{
			PCCERT_CONTEXT RecipientCertArray[1] = {0};
			RecipientCertArray[0] = pRecipientCert;

			CRYPT_ALGORITHM_IDENTIFIER EncryptAlgorithm = {0};
			EncryptAlgorithm.pszObjId = szOID_RSA_DES_EDE3_CBC;

			CRYPT_ENCRYPT_MESSAGE_PARA EncryptParams = {0};
			EncryptParams.cbSize = sizeof(EncryptParams);
			EncryptParams.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
			EncryptParams.hCryptProv = hCryptProv;
			EncryptParams.ContentEncryptionAlgorithm = EncryptAlgorithm;

			DWORD cbEncryptedBlob = 0;
			if (CryptEncryptMessage(&EncryptParams, 1, RecipientCertArray, (const BYTE*)(LPCSTR)rstrContentA, rstrContentA.GetLength(), NULL, &cbEncryptedBlob))
			{
				try {
					raEncrypted.SetSize(cbEncryptedBlob);
					if (!CryptEncryptMessage(&EncryptParams, 1, RecipientCertArray, (const BYTE*)(LPCSTR)rstrContentA, rstrContentA.GetLength(), raEncrypted.GetData(), &cbEncryptedBlob)) {
						DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Encryption: Failed to encrypt message - %s"), GetErrorMessage(GetLastError(), 1));
						raEncrypted.SetSize(0);
					}
				}
				catch (CMemoryException* ex){
					ex->Delete();
					DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Encryption: Failed to encrypt message - %s"), _tcserror(ENOMEM));
				}
			}
			else {
				DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Encryption: Failed to get length of encrypted message - %s"), GetErrorMessage(GetLastError(), 1));
			}
			VERIFY( CertFreeCertificateContext(pRecipientCert) );
			pRecipientCert = NULL;
		}
		else {
			DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Encryption: Failed to find certificate with subject '%ls' - %s"), pwszCertSubject, GetErrorMessage(GetLastError(), 1));
		}
		VERIFY( CertCloseStore(hStoreHandle, 0) );
		hStoreHandle = NULL;
	}
	else {
		DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Encryption: Failed to open certificate store '%s' - %s"), pszCertStore, GetErrorMessage(GetLastError(), 1));
	}
	VERIFY( CryptReleaseContext(hCryptProv, 0) );
	hCryptProv = NULL;

	return raEncrypted.GetSize() > 0;
}


///////////////////////////////////////////////////////////////////////////////
// CNotifierMailThread

class CNotifierMailThread : public CWinThread
{
	DECLARE_DYNCREATE(CNotifierMailThread)

protected:
	CNotifierMailThread();           // protected constructor used by dynamic creation
	virtual ~CNotifierMailThread();
	static CCriticalSection sm_critSect;

public:
	CString m_strHostName;
	CString m_strRecipient;
	CString m_strSender;
	CString m_strSubject;
	CString m_strBody;
	CString m_strEncryptCertName;

	virtual	BOOL InitInstance();
};

CCriticalSection CNotifierMailThread::sm_critSect;

IMPLEMENT_DYNCREATE(CNotifierMailThread, CWinThread)

CNotifierMailThread::CNotifierMailThread()
{
}

CNotifierMailThread::~CNotifierMailThread()
{
}

BOOL CNotifierMailThread::InitInstance()
{
	DbgSetThreadName("NotifierMailThread");
	if (theApp.emuledlg != NULL && theApp.emuledlg->IsRunning())
	{
		sm_critSect.Lock();
		InitThreadLocale();
		CoInitialize(NULL);

		bool bHaveValidMsg = false;
		UINT uCodePage = CP_UTF8;
		CByteArray aEncryptedBody;
		if (!m_strEncryptCertName.IsEmpty())
		{
			CComPtr<IMultiLanguage> spMultiLanguage;
			HRESULT hr = spMultiLanguage.CoCreateInstance(__uuidof(CMultiLanguage));
			if (hr == S_OK)
			{
				CStringA strMimeMessageA;
				strMimeMessageA = "Content-Type: text/plain;\r\n\tformat=flowed";
				char szCharset[ATL_MAX_ENC_CHARSET_LENGTH];
#if _ATL_VER==0x0700
				if (AtlMimeCharsetFromCodePage(szCharset, uCodePage, spMultiLanguage))
#else
				if (AtlMimeCharsetFromCodePage(szCharset, uCodePage, spMultiLanguage, ARRSIZE(szCharset)))
#endif
					strMimeMessageA.AppendFormat(";\r\n\tcharset=\"%s\"", szCharset);
				strMimeMessageA += "\r\n";
				strMimeMessageA += "Content-Transfer-Encoding: 8bit\r\n";
				strMimeMessageA += "\r\n";

				UINT nMimeTextLen = 0;
				LPSTR pszMimeText = NULL;
				if (AtlMimeConvertString(spMultiLanguage, uCodePage, m_strBody, &pszMimeText, &nMimeTextLen))
				{
					strMimeMessageA.Append(pszMimeText, nMimeTextLen);
					if (Encrypt(strMimeMessageA, aEncryptedBody, m_strEncryptCertName))
						bHaveValidMsg = aEncryptedBody.GetSize() > 0;
				}

				CHeapPtr<char> pMimeText;
				pMimeText.Attach(pszMimeText);
			}
			else {
				DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Notification: Failed to instantiate 'IMultiLanguage' - %s"), GetErrorMessage(hr, 1));
			}
		}
		else
		{
			bHaveValidMsg = !m_strBody.IsEmpty();
		}

		if (bHaveValidMsg)
		{
			CSMTPConnection smtp;
			if (smtp.Connect(m_strHostName))
			{
				if (theApp.emuledlg != NULL && theApp.emuledlg->IsRunning())
				{
					CMimeMessageEx msg;
					msg.SetSenderName(_T("eMule"));
					msg.SetSender(m_strSender);
					msg.AddRecipient(m_strRecipient);
					msg.SetSubject(m_strSubject);
					if (aEncryptedBody.GetSize() > 0) {
						msg.AttachRaw((void*)aEncryptedBody.GetData(), aEncryptedBody.GetSize(), ATLSMTP_BASE64_ENCODE, TRUE, 
									_T("smime.p7m"), _T("application/x-pkcs7-mime;\r\n\tformat=flowed;\r\n\tsmime-type=enveloped-data"));
					}
					else {
						msg.AddText(m_strBody, -1, 1, uCodePage);
					}
					if (!smtp.SendMessage(msg)) {
						// can't use 'GetLastError' here!
						DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Notification: Failed to send e-mail to '%s'"), m_strRecipient);
					}
				}
			}
			else {
				// can't use 'GetLastError' here!
				DebugLogWarning(LOG_DONTNOTIFY, _T("E-Mail Notification: Failed to connect to SMTP server '%s'"), m_strHostName);
			}
		}

		CoUninitialize();
		sm_critSect.Unlock();
	}
	return FALSE;
}

void CemuleDlg::SendNotificationMail(int iMsgType, LPCTSTR pszText)
{
	if (!thePrefs.IsNotifierSendMailEnabled())
		return;

	CString strHostName = thePrefs.GetNotifierMailServer();
	strHostName.Trim();
	if (strHostName.IsEmpty())
		return;

	CString strRecipient = thePrefs.GetNotifierMailReceiver();
	strRecipient.Trim();
	if (strRecipient.IsEmpty())
		return;

	CString strSender = thePrefs.GetNotifierMailSender();
	strSender.Trim();
	if (strSender.IsEmpty())
		return;

	CNotifierMailThread* pThread = (CNotifierMailThread*)AfxBeginThread(RUNTIME_CLASS(CNotifierMailThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	if (pThread)
	{
		pThread->m_strHostName = strHostName;
		pThread->m_strRecipient = strRecipient;
		pThread->m_strSender = strSender;
		pThread->m_strEncryptCertName = theApp.GetProfileString(_T("eMule"), _T("NotifierMailEncryptCertName")).Trim();
		pThread->m_strSubject = GetResString(IDS_EMULENOTIFICATION);
		switch (iMsgType)
		{
			case TBN_CHAT:
				pThread->m_strSubject +=  _T(": ") + GetResString(IDS_PW_TBN_POP_ALWAYS);
				break;
			case TBN_DOWNLOADFINISHED:
				pThread->m_strSubject +=  _T(": ") + GetResString(IDS_PW_TBN_ONDOWNLOAD);
				break;
			case TBN_DOWNLOADADDED:
				pThread->m_strSubject +=  _T(": ") + GetResString(IDS_TBN_ONNEWDOWNLOAD);
				break;
			case TBN_LOG:
				pThread->m_strSubject += _T(": ") + GetResString(IDS_PW_TBN_ONLOG);
				break;
			case TBN_IMPORTANTEVENT:
				pThread->m_strSubject +=_T(": ") + GetResString(IDS_ERROR);
				break;
			case TBN_NEWVERSION:
				pThread->m_strSubject += _T(": ") + GetResString(IDS_CB_TBN_ONNEWVERSION);
				break;
			default:
				ASSERT(0);
		}
		pThread->m_strBody = pszText;
		pThread->ResumeThread();
	}
}
