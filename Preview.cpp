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
#include <sys/stat.h>
#include <share.h>
#include "emule.h"
#include "Preview.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "PartFile.h"
#include "MenuCmds.h"
#include "opcodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CPreviewApps thePreviewApps;

///////////////////////////////////////////////////////////////////////////////
// CPreviewThread

IMPLEMENT_DYNCREATE(CPreviewThread, CWinThread)

BEGIN_MESSAGE_MAP(CPreviewThread, CWinThread)
END_MESSAGE_MAP()

CPreviewThread::CPreviewThread()
{
}

CPreviewThread::~CPreviewThread()
{
}

BOOL CPreviewThread::InitInstance()
{
	DbgSetThreadName("PartFilePreview");
	InitThreadLocale();
	return TRUE;
}

BOOL CPreviewThread::Run()
{
	ASSERT (m_pPartfile) ;
	CFile destFile;
	CFile srcFile;
	if (!srcFile.Open(m_pPartfile->GetFilePath(), CFile::modeRead | CFile::shareDenyNone))
		return FALSE;
	try{
		uint64 nSize = m_pPartfile->GetFileSize();
		CString strExtension = CString(_tcsrchr(m_pPartfile->GetFileName(), _T('.')));
		CString strPreviewName = m_pPartfile->GetTempPath() + m_pPartfile->GetFileName().Mid(0, 5) + _T("_preview") + strExtension;
		bool bFullSized = true;
		if (!strExtension.CompareNoCase(_T(".mpg")) || !strExtension.CompareNoCase(_T(".mpeg")))
			bFullSized = false;
		if (!destFile.Open(strPreviewName, CFile::modeWrite | CFile::shareDenyWrite | CFile::modeCreate))
			return FALSE;
		srcFile.SeekToBegin();
		if (bFullSized)
			destFile.SetLength(nSize);
		destFile.SeekToBegin();
		BYTE abyBuffer[4096];
		uint32 nRead;
		while (destFile.GetPosition()+4096 < PARTSIZE*2){
			nRead = srcFile.Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		srcFile.Seek(-(LONGLONG)(PARTSIZE*2), CFile::end);
		uint32 nToGo = PARTSIZE*2;
		if (bFullSized)
			destFile.Seek(-(LONGLONG)(PARTSIZE*2), CFile::end);
		do{
			nRead = (nToGo - 4096 < 1)? nToGo:4096;
			nToGo -= nRead;
			nRead = srcFile.Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		while (nToGo);
		destFile.Close();
		srcFile.Close();
		m_pPartfile->m_bPreviewing = false;

		SHELLEXECUTEINFO SE = {0};
		SE.fMask = SEE_MASK_NOCLOSEPROCESS;

		CString strCommand;
		CString strArgs;
		CString strCommandDir;
		if (!m_strCommand.IsEmpty())
		{
			SE.lpVerb = _T("open");	// "open" the specified video player

			// get directory of video player application
			strCommandDir = m_strCommand;
			int iPos = strCommandDir.ReverseFind(_T('\\'));
			if (iPos == -1)
				strCommandDir.Empty();
			else
				strCommandDir = strCommandDir.Left(iPos + 1);
			PathRemoveBackslash(strCommandDir.GetBuffer());
			strCommandDir.ReleaseBuffer();

			strArgs = m_strCommandArgs;
			if (!strArgs.IsEmpty())
				strArgs += _T(' ');
			if (strPreviewName.Find(_T(' ')) != -1)
				strArgs += _T('\"') + strPreviewName + _T('\"');
			else
				strArgs += strPreviewName;

			strCommand = m_strCommand;
			ExpandEnvironmentStrings(strCommand);
			ExpandEnvironmentStrings(strArgs);
			ExpandEnvironmentStrings(strCommandDir);
			SE.lpFile = strCommand;
			SE.lpParameters = strArgs;
			SE.lpDirectory = strCommandDir;
		}
		else
		{
			SE.lpVerb = NULL;	// use the default verb or the open verb for the document
			SE.lpFile = strPreviewName;
		}
		SE.nShow = SW_SHOW;
		SE.cbSize = sizeof(SE);
		ShellExecuteEx(&SE);
		if (SE.hProcess){
			WaitForSingleObject(SE.hProcess, INFINITE);
			CloseHandle(SE.hProcess);
		}
		CFile::Remove(strPreviewName);
	}
	catch(CFileException* error){
		m_pPartfile->m_bPreviewing = false;
		error->Delete();
	}
	return TRUE;
}

void CPreviewThread::SetValues(CPartFile* pPartFile, LPCTSTR pszCommand, LPCTSTR pszCommandArgs)
{
	m_pPartfile = pPartFile;
	m_strCommand = pszCommand;
	m_strCommandArgs = pszCommandArgs;
}


///////////////////////////////////////////////////////////////////////////////
// CPreviewApps

CPreviewApps::CPreviewApps()
{
	m_tDefAppsFileLastModified = 0;
}

CString CPreviewApps::GetDefaultAppsFile() const
{
	return thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("PreviewApps.dat");
}

void CPreviewApps::RemoveAllApps()
{
	m_aApps.RemoveAll();
	m_tDefAppsFileLastModified = 0;
}

int CPreviewApps::ReadAllApps()
{
	RemoveAllApps();

	CString strFilePath = GetDefaultAppsFile();
	FILE* readFile = _tfsopen(strFilePath, _T("r"), _SH_DENYWR);
	if (readFile != NULL)
	{
		CString name, url, sbuffer;
		while (!feof(readFile))
		{
			TCHAR buffer[1024];
			if (_fgetts(buffer, ARRSIZE(buffer), readFile) == NULL)
				break;
			sbuffer = buffer;
			sbuffer.TrimRight(_T("\r\n\t"));

			// ignore comments & too short lines
			if (sbuffer.GetAt(0) == _T('#') || sbuffer.GetAt(0) == _T('/') || sbuffer.GetLength() < 5)
				continue;

			int iPos = 0;
			CString strTitle = sbuffer.Tokenize(_T("="), iPos);
			strTitle.Trim();
			if (!strTitle.IsEmpty())
			{
				CString strCommandLine = sbuffer.Tokenize(_T(";"), iPos);
				strCommandLine.Trim();
				if (!strCommandLine.IsEmpty())
				{
					LPCTSTR pszCommandLine = strCommandLine;
					LPTSTR pszCommandArgs = PathGetArgs(pszCommandLine);
					CString strCommand, strCommandArgs;
					if (pszCommandArgs)
						strCommand = strCommandLine.Left(pszCommandArgs - pszCommandLine);
					else
						strCommand = strCommandLine;
					strCommand.Trim(_T(" \t\""));
					if (!strCommand.IsEmpty())
					{
						uint64 ullMinCompletedSize = 0;
						uint64 ullMinStartOfFile = 0;
						CStringArray astrExtensions;
						CString strParams = sbuffer.Tokenize(_T(";"), iPos);
						while (!strParams.IsEmpty())
						{
							int iPosParam = 0;
							CString strId = strParams.Tokenize(_T("="), iPosParam);
							if (!strId.IsEmpty())
							{
								CString strValue = strParams.Tokenize(_T("="), iPosParam);
								if (strId.CompareNoCase(_T("Ext")) == 0)
								{
									if (!strValue.IsEmpty())
									{
										if (strValue[0] != _T('.'))
											strValue = _T('.') + strValue;
										astrExtensions.Add(strValue);
									}
								}
								else if (strId.CompareNoCase(_T("MinSize")) == 0)
								{
									if (!strValue.IsEmpty())
										(void)_stscanf(strValue, _T("%I64u"), &ullMinCompletedSize);
								}
								else if (strId.CompareNoCase(_T("MinStart")) == 0)
								{
									if (!strValue.IsEmpty())
										(void)_stscanf(strValue, _T("%I64u"), &ullMinStartOfFile);
								}
							}
							strParams = sbuffer.Tokenize(_T(";"), iPos);
						}

						SPreviewApp svc;
						svc.strTitle = strTitle;
						svc.strCommand = strCommand;
						svc.strCommandArgs = pszCommandArgs;
						svc.strCommandArgs.Trim();
						svc.astrExtensions.Append(astrExtensions);
						svc.ullMinCompletedSize = ullMinCompletedSize;
						svc.ullMinStartOfFile = ullMinStartOfFile;
						m_aApps.Add(svc);
					}
				}
			}
		}
		fclose(readFile);

		struct _stat st;
		if (_tstat(strFilePath, &st) == 0)
			m_tDefAppsFileLastModified = st.st_mtime;
	}

	return m_aApps.GetCount();
}

void CPreviewApps::UpdateApps()
{
	if (m_aApps.GetCount() == 0)
	{
		ReadAllApps();
	}
	else
	{
		struct _stat st;
		if (_tstat(GetDefaultAppsFile(), &st) == 0 && st.st_mtime > m_tDefAppsFileLastModified)
			ReadAllApps();
	}
}

int CPreviewApps::GetAllMenuEntries(CMenu& rMenu, const CPartFile* file)
{
	UpdateApps();

	for (int i = 0; i < m_aApps.GetCount(); i++)
	{
		const SPreviewApp& rSvc = m_aApps.GetAt(i);
		if (MP_PREVIEW_APP_MIN + i > MP_PREVIEW_APP_MAX)
			break;
		bool bEnabled = false;
		if (file)
		{
			if (file->GetCompletedSize() >= (uint64)16*1024)
				bEnabled = true;
		}
		rMenu.AppendMenu(MF_STRING | (bEnabled ? MF_ENABLED : MF_GRAYED), MP_PREVIEW_APP_MIN + i, rSvc.strTitle);
	}
	return m_aApps.GetCount();
}

void CPreviewApps::RunApp(CPartFile* file, UINT uMenuID)
{
	const SPreviewApp& svc = m_aApps.GetAt(uMenuID - MP_PREVIEW_APP_MIN);
	::ExecutePartFile(file, svc.strCommand, svc.strCommandArgs);
}

void ExecutePartFile(CPartFile* file, LPCTSTR pszCommand, LPCTSTR pszCommandArgs)
{
	if (!CheckFileOpen(file->GetFilePath(), file->GetFileName()))
		return;

	CString strQuotedPartFilePath(file->GetFilePath());

	// if the path contains spaces, quote the entire path
	if (strQuotedPartFilePath.Find(_T(' ')) != -1)
		strQuotedPartFilePath = _T('\"') + strQuotedPartFilePath + _T('\"');

	// get directory of video player application
	CString strCommandDir = pszCommand;
	int iPos = strCommandDir.ReverseFind(_T('\\'));
	if (iPos == -1)
		strCommandDir.Empty();
	else
		strCommandDir = strCommandDir.Left(iPos + 1);
	PathRemoveBackslash(strCommandDir.GetBuffer());
	strCommandDir.ReleaseBuffer();

	CString strArgs = pszCommandArgs;
	if (!strArgs.IsEmpty())
		strArgs += _T(' ');
	strArgs += strQuotedPartFilePath;

	file->FlushBuffer(true);

	CString strCommand = pszCommand;
	ExpandEnvironmentStrings(strCommand);
	ExpandEnvironmentStrings(strArgs);
	ExpandEnvironmentStrings(strCommandDir);

	LPCTSTR pszVerb = _T("open");

	// Backward compatibility with old 'preview' command (when no preview application is specified):
	//	"ShellExecute(NULL, NULL, strPartFilePath, NULL, NULL, SW_SHOWNORMAL);"
	if (strCommand.IsEmpty())
	{
		strCommand = strArgs;
		strArgs.Empty();
		strCommandDir.Empty();
		pszVerb = NULL;
	}

	TRACE(_T("Starting preview application:\n"));
	TRACE(_T("  Command =%s\n"), strCommand);
	TRACE(_T("  Args    =%s\n"), strArgs);
	TRACE(_T("  Dir     =%s\n"), strCommandDir);
	DWORD_PTR dwError = (DWORD_PTR)ShellExecute(NULL, pszVerb, strCommand, strArgs.IsEmpty() ? NULL : strArgs, strCommandDir.IsEmpty() ? NULL : strCommandDir, SW_SHOWNORMAL);
	if (dwError <= 32)
	{
		//
		// Unfortunately, Windows may already have shown an error dialog which tells
		// the user about the failed 'ShellExecute' call. *BUT* that error dialog is not
		// shown in each case!
		//
		// Examples:
		//	 -	Specifying an executeable which does not exist (e.g. APP.EXE)
		//		-> (Error 2) -> No error is shown.
		//
		//   -	Executing a document (verb "open") which has an unregistered extension
		//		-> (Error 31) -> Error is shown.
		//
		// I'm not sure whether this behaviour (showing an error dialog in cases of some
		// specific errors) is handled the same way in all Windows version -> therefore I
		// decide to always show an application specific error dialog!
		//
		CString strMsg;
		strMsg.Format(_T("Failed to execute: %s %s"), strCommand, strArgs);

		CString strSysErrMsg(GetShellExecuteErrMsg(dwError));
		if (!strSysErrMsg.IsEmpty())
			strMsg += _T("\r\n\r\n") + strSysErrMsg;

		AfxMessageBox(strMsg, MB_ICONSTOP);
	}
}

int CPreviewApps::GetPreviewApp(const CPartFile* file)
{
	LPCTSTR pszExt = PathFindExtension(file->GetFileName());
	if (pszExt == NULL)
		return -1;

	UpdateApps();

	int iApp = -1;
	for (int i = 0; iApp == -1 && i < m_aApps.GetCount(); i++)
	{
		const SPreviewApp& rApp = m_aApps.GetAt(i);
		for (int j = 0; j < rApp.astrExtensions.GetCount(); j++)
		{
			if (rApp.astrExtensions.GetAt(j).CompareNoCase(pszExt) == 0) {
				iApp = i;
				break;
			}
		}
	}

	return iApp;
}

CPreviewApps::ECanPreviewRes CPreviewApps::CanPreview(const CPartFile* file)
{
	int iApp = GetPreviewApp(file);
	if (iApp == -1)
		return NotHandled;

	const SPreviewApp* pApp = &m_aApps.GetAt(iApp);
	if (pApp->ullMinCompletedSize != 0)
	{
		if (file->GetCompletedSize() < pApp->ullMinCompletedSize)
			return No;
	}

	if (pApp->ullMinStartOfFile != 0)
	{
		if (!file->IsComplete(0, pApp->ullMinStartOfFile, false))
			return No;
	}

	return Yes;
}

bool CPreviewApps::Preview(CPartFile* file)
{
	int iApp = GetPreviewApp(file);
	if (iApp == -1)
		return false;
	RunApp(file, MP_PREVIEW_APP_MIN + iApp);
	return true;
}
