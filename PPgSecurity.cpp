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
#include <share.h>
#include "emule.h"
#include "PPgSecurity.h"
#include "OtherFunctions.h"
#include "IPFilter.h"
#include "Preferences.h"
#include "CustomAutoComplete.h"
#include "HttpDownloadDlg.h"
#include "emuledlg.h"
#include "HelpIDs.h"
#include "ZipFile.h"
#include "GZipFile.h"
#include "RarFile.h"
#include "Log.h"
#include "ServerWnd.h"
#include "ServerListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool GetMimeType(LPCTSTR pszFilePath, CString& rstrMimeType);

#define	IPFILTERUPDATEURL_STRINGS_PROFILE	_T("AC_IPFilterUpdateURLs.dat")

IMPLEMENT_DYNAMIC(CPPgSecurity, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgSecurity, CPropertyPage)
	ON_BN_CLICKED(IDC_FILTERSERVERBYIPFILTER , OnSettingsChange)
	ON_BN_CLICKED(IDC_RELOADFILTER, OnReloadIPFilter)
	ON_BN_CLICKED(IDC_EDITFILTER, OnEditIPFilter)
	ON_EN_CHANGE(IDC_FILTERLEVEL, OnSettingsChange)
	ON_BN_CLICKED(IDC_USESECIDENT, OnSettingsChange)
	ON_BN_CLICKED(IDC_LOADURL, OnLoadIPFFromURL)
	ON_EN_CHANGE(IDC_UPDATEURL, OnEnChangeUpdateUrl)
	ON_BN_CLICKED(IDC_DD,OnDDClicked)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_RUNASUSER, OnBnClickedRunAsUser)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SEESHARE1, OnSettingsChange)
	ON_BN_CLICKED(IDC_SEESHARE2, OnSettingsChange)
	ON_BN_CLICKED(IDC_SEESHARE3, OnSettingsChange)
	ON_BN_CLICKED(IDC_ENABLEOBFUSCATION, OnObfuscatedRequestedChange)
	ON_BN_CLICKED(IDC_ONLYOBFUSCATED, OnSettingsChange)
	ON_BN_CLICKED(IDC_DISABLEOBFUSCATION, OnObfuscatedDisabledChange)
	ON_BN_CLICKED(IDC_SEARCHSPAMFILTER, OnSettingsChange)
	ON_BN_CLICKED(IDC_CHECK_FILE_OPEN, OnSettingsChange)
END_MESSAGE_MAP()

CPPgSecurity::CPPgSecurity()
	: CPropertyPage(CPPgSecurity::IDD)
{
	m_pacIPFilterURL = NULL;
}

CPPgSecurity::~CPPgSecurity()
{
}

void CPPgSecurity::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

void CPPgSecurity::LoadSettings(void)
{
	CString strBuffer;
	
	strBuffer.Format(_T("%i"), thePrefs.filterlevel);
	GetDlgItem(IDC_FILTERLEVEL)->SetWindowText(strBuffer);
	CheckDlgButton(IDC_FILTERSERVERBYIPFILTER, thePrefs.filterserverbyip);

	CheckDlgButton(IDC_USESECIDENT, thePrefs.m_bUseSecureIdent);

	if ((thePrefs.GetWindowsVersion() == _WINVER_XP_ || thePrefs.GetWindowsVersion() == _WINVER_2K_ || thePrefs.GetWindowsVersion() == _WINVER_2003_)
		&& thePrefs.m_nCurrentUserDirMode == 2)
		GetDlgItem(IDC_RUNASUSER)->EnableWindow(TRUE);
	else
		GetDlgItem(IDC_RUNASUSER)->EnableWindow(FALSE);
	CheckDlgButton(IDC_RUNASUSER, thePrefs.IsRunAsUserEnabled());

	if (!thePrefs.IsClientCryptLayerSupported()){
		CheckDlgButton(IDC_DISABLEOBFUSCATION,1);
		GetDlgItem(IDC_ENABLEOBFUSCATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(FALSE);
	}
	else{
		CheckDlgButton(IDC_DISABLEOBFUSCATION,0);
		GetDlgItem(IDC_ENABLEOBFUSCATION)->EnableWindow(TRUE);
		GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(TRUE);
	}

	if (thePrefs.IsClientCryptLayerRequested()){
		CheckDlgButton(IDC_ENABLEOBFUSCATION,1);
		GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(TRUE);
	}
	else{
		CheckDlgButton(IDC_ENABLEOBFUSCATION,0);
		GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(FALSE);
	}

	CheckDlgButton(IDC_ONLYOBFUSCATED, thePrefs.IsClientCryptLayerRequired());
	CheckDlgButton(IDC_SEARCHSPAMFILTER, thePrefs.IsSearchSpamFilterEnabled());
	CheckDlgButton(IDC_CHECK_FILE_OPEN, thePrefs.GetCheckFileOpen());

	ASSERT( vsfaEverybody == 0 );
	ASSERT( vsfaFriends == 1 );
	ASSERT( vsfaNobody == 2 );
	CheckRadioButton(IDC_SEESHARE1, IDC_SEESHARE3, IDC_SEESHARE1 + thePrefs.m_iSeeShares);
}

BOOL CPPgSecurity::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	if (thePrefs.GetUseAutocompletion()) {
		if (!m_pacIPFilterURL) {
			m_pacIPFilterURL = new CCustomAutoComplete();
			m_pacIPFilterURL->AddRef();
			if (m_pacIPFilterURL->Bind(::GetDlgItem(m_hWnd, IDC_UPDATEURL), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES ))
				m_pacIPFilterURL->LoadList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + IPFILTERUPDATEURL_STRINGS_PROFILE);
		}
		SetDlgItemText(IDC_UPDATEURL,m_pacIPFilterURL->GetItem(0));
		if (theApp.m_fontSymbol.m_hObject) {
			GetDlgItem(IDC_DD)->SetFont(&theApp.m_fontSymbol);
			GetDlgItem(IDC_DD)->SetWindowText(_T("6")); // show a down-arrow
		}
	}
	else
		GetDlgItem(IDC_DD)->ShowWindow(SW_HIDE);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgSecurity::OnApply()
{
	bool bIPFilterSettingsChanged = false;

	TCHAR buffer[510];
	if (GetDlgItem(IDC_FILTERLEVEL)->GetWindowTextLength()) {
		GetDlgItem(IDC_FILTERLEVEL)->GetWindowText(buffer, 4);
		int iNewFilterLevel = _tstoi(buffer);
		if (iNewFilterLevel >= 0 && (UINT)iNewFilterLevel != thePrefs.filterlevel) {
			thePrefs.filterlevel = iNewFilterLevel;
			bIPFilterSettingsChanged = IsDlgButtonChecked(IDC_FILTERSERVERBYIPFILTER)!=0;
		}
	}
	if (!thePrefs.filterserverbyip && IsDlgButtonChecked(IDC_FILTERSERVERBYIPFILTER)!=0)
		bIPFilterSettingsChanged = true;
	thePrefs.filterserverbyip = IsDlgButtonChecked(IDC_FILTERSERVERBYIPFILTER)!=0;
	if (bIPFilterSettingsChanged)
		theApp.emuledlg->serverwnd->serverlistctrl.RemoveAllFilteredServers();

	thePrefs.m_bUseSecureIdent = IsDlgButtonChecked(IDC_USESECIDENT)!=0;
	thePrefs.m_bRunAsUser = IsDlgButtonChecked(IDC_RUNASUSER)!=0;

	thePrefs.m_bCryptLayerRequested = IsDlgButtonChecked(IDC_ENABLEOBFUSCATION) != 0;
	thePrefs.m_bCryptLayerRequired = IsDlgButtonChecked(IDC_ONLYOBFUSCATED) != 0;
	thePrefs.m_bCryptLayerSupported = IsDlgButtonChecked(IDC_DISABLEOBFUSCATION) == 0;
	thePrefs.m_bCheckFileOpen = IsDlgButtonChecked(IDC_CHECK_FILE_OPEN) != 0;
	thePrefs.m_bEnableSearchResultFilter = IsDlgButtonChecked(IDC_SEARCHSPAMFILTER) != 0;


	if (IsDlgButtonChecked(IDC_SEESHARE1))
		thePrefs.m_iSeeShares = vsfaEverybody;
	else if (IsDlgButtonChecked(IDC_SEESHARE2))
		thePrefs.m_iSeeShares = vsfaFriends;
	else
		thePrefs.m_iSeeShares = vsfaNobody;

	LoadSettings();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgSecurity::Localize(void)
{
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_SECURITY));
		GetDlgItem(IDC_STATIC_IPFILTER)->SetWindowText(GetResString(IDS_IPFILTER));
		GetDlgItem(IDC_RELOADFILTER)->SetWindowText(GetResString(IDS_SF_RELOAD));
		GetDlgItem(IDC_EDITFILTER)->SetWindowText(GetResString(IDS_EDIT));
		GetDlgItem(IDC_STATIC_FILTERLEVEL)->SetWindowText(GetResString(IDS_FILTERLEVEL)+_T(":"));
		GetDlgItem(IDC_FILTERSERVERBYIPFILTER)->SetWindowText(GetResString(IDS_FILTERSERVERBYIPFILTER));

		GetDlgItem(IDC_SEC_MISC)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_USESECIDENT)->SetWindowText(GetResString(IDS_USESECIDENT));
		SetDlgItemText(IDC_RUNASUSER,GetResString(IDS_RUNASUSER));

		SetDlgItemText(IDC_STATIC_UPDATEFROM,GetResString(IDS_UPDATEFROM));
		SetDlgItemText(IDC_LOADURL,GetResString(IDS_LOADURL));

		GetDlgItem(IDC_SEEMYSHARE_FRM)->SetWindowText(GetResString(IDS_PW_SHARE));
		GetDlgItem(IDC_SEESHARE1)->SetWindowText(GetResString(IDS_PW_EVER));
		GetDlgItem(IDC_SEESHARE2)->SetWindowText(GetResString(IDS_FSTATUS_FRIENDSONLY));
		GetDlgItem(IDC_SEESHARE3)->SetWindowText(GetResString(IDS_PW_NOONE));

		GetDlgItem(IDC_DISABLEOBFUSCATION)->SetWindowText(GetResString(IDS_DISABLEOBFUSCATION));
		GetDlgItem(IDC_ONLYOBFUSCATED)->SetWindowText(GetResString(IDS_ONLYOBFUSCATED));
		GetDlgItem(IDC_ENABLEOBFUSCATION)->SetWindowText(GetResString(IDS_ENABLEOBFUSCATION));
		GetDlgItem(IDC_SEC_OBFUSCATIONBOX)->SetWindowText(GetResString(IDS_PROTOCOLOBFUSCATION));
		GetDlgItem(IDC_SEARCHSPAMFILTER)->SetWindowText(GetResString(IDS_SEARCHSPAMFILTER));
		GetDlgItem(IDC_CHECK_FILE_OPEN)->SetWindowText(GetResString(IDS_CHECK_FILE_OPEN));
	}
}

void CPPgSecurity::OnReloadIPFilter()
{
	CWaitCursor curHourglass;
	theApp.ipfilter->LoadFromDefaultFile();
	if (thePrefs.GetFilterServerByIP())
		theApp.emuledlg->serverwnd->serverlistctrl.RemoveAllFilteredServers();
}

void CPPgSecurity::OnEditIPFilter()
{
	ShellExecute(NULL, _T("open"), thePrefs.GetTxtEditor(),
		_T("\"") + thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + DFLT_IPFILTER_FILENAME _T("\""), NULL, SW_SHOW);
}

void CPPgSecurity::OnLoadIPFFromURL()
{
	bool bHaveNewFilterFile = false;
	CString url;
	GetDlgItemText(IDC_UPDATEURL,url);
	if (!url.IsEmpty())
	{
		// add entered URL to LRU list even if it's not yet known whether we can download from this URL (it's just more convenient this way)
		if (m_pacIPFilterURL && m_pacIPFilterURL->IsBound())
			m_pacIPFilterURL->AddItem(url, 0);

		CString strTempFilePath;
		_tmakepathlimit(strTempFilePath.GetBuffer(MAX_PATH), NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), DFLT_IPFILTER_FILENAME, _T("tmp"));
		strTempFilePath.ReleaseBuffer();

		CHttpDownloadDlg dlgDownload;
		dlgDownload.m_strTitle = GetResString(IDS_DWL_IPFILTERFILE);
		dlgDownload.m_sURLToDownload = url;
		dlgDownload.m_sFileToDownloadInto = strTempFilePath;
		if (dlgDownload.DoModal() != IDOK)
		{
			(void)_tremove(strTempFilePath);
			CString strError = GetResString(IDS_DWLIPFILTERFAILED);
			if (!dlgDownload.GetError().IsEmpty())
				strError += _T("\r\n\r\n") + dlgDownload.GetError();
			AfxMessageBox(strError, MB_ICONERROR);
			return;
		}

		CString strMimeType;
		GetMimeType(strTempFilePath, strMimeType);

		bool bIsArchiveFile = false;
		bool bUncompressed = false;
		CZIPFile zip;
		if (zip.Open(strTempFilePath))
		{
			bIsArchiveFile = true;

			CZIPFile::File* zfile = zip.GetFile(_T("ipfilter.dat"));
			if (zfile == NULL)
				zfile = zip.GetFile(_T("guarding.p2p"));
			if (zfile == NULL)
				zfile = zip.GetFile(_T("guardian.p2p"));
			if (zfile)
			{
				CString strTempUnzipFilePath;
				_tmakepathlimit(strTempUnzipFilePath.GetBuffer(_MAX_PATH), NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), DFLT_IPFILTER_FILENAME, _T(".unzip.tmp"));
				strTempUnzipFilePath.ReleaseBuffer();

				if (zfile->Extract(strTempUnzipFilePath))
				{
					zip.Close();
					zfile = NULL;

					if (_tremove(theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE(_T("*** Error: Failed to remove default IP filter file \"%s\" - %s\n"), theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
					if (_trename(strTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE(_T("*** Error: Failed to rename uncompressed IP filter file \"%s\" to default IP filter file \"%s\" - %s\n"), strTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
					if (_tremove(strTempFilePath) != 0)
						TRACE(_T("*** Error: Failed to remove temporary IP filter file \"%s\" - %s\n"), strTempFilePath, _tcserror(errno));
					bUncompressed = true;
					bHaveNewFilterFile = true;
				}
				else {
					CString strError;
					strError.Format(GetResString(IDS_ERR_IPFILTERZIPEXTR), strTempFilePath);
					AfxMessageBox(strError, MB_ICONERROR);
				}
			}
			else {
				CString strError;
				strError.Format(GetResString(IDS_ERR_IPFILTERCONTENTERR), strTempFilePath);
				AfxMessageBox(strError, MB_ICONERROR);
			}

			zip.Close();
		}
		else if (strMimeType.CompareNoCase(_T("application/x-rar-compressed")) == 0)
		{
			bIsArchiveFile = true;

			CRARFile rar;
			if (rar.Open(strTempFilePath))
			{
				CString strFile;
				if (rar.GetNextFile(strFile)
					&& (   strFile.CompareNoCase(_T("ipfilter.dat")) == 0 
					    || strFile.CompareNoCase(_T("guarding.p2p")) == 0
						|| strFile.CompareNoCase(_T("guardian.p2p")) == 0))
				{
					CString strTempUnzipFilePath;
					_tmakepathlimit(strTempUnzipFilePath.GetBuffer(MAX_PATH), NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), DFLT_IPFILTER_FILENAME, _T(".unzip.tmp"));
					strTempUnzipFilePath.ReleaseBuffer();
					if (rar.Extract(strTempUnzipFilePath))
					{
						rar.Close();

						if (_tremove(theApp.ipfilter->GetDefaultFilePath()) != 0)
							TRACE(_T("*** Error: Failed to remove default IP filter file \"%s\" - %s\n"), theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
						if (_trename(strTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath()) != 0)
							TRACE(_T("*** Error: Failed to rename uncompressed IP filter file \"%s\" to default IP filter file \"%s\" - %s\n"), strTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
						if (_tremove(strTempFilePath) != 0)
							TRACE(_T("*** Error: Failed to remove temporary IP filter file \"%s\" - %s\n"), strTempFilePath, _tcserror(errno));
						bUncompressed = true;
						bHaveNewFilterFile = true;
					}
					else
					{
						CString strError;
						strError.Format(_T("Failed to extract IP filter file from RAR file \"%s\"."), strTempFilePath);
						AfxMessageBox(strError, MB_ICONERROR);
					}
				}
				else
				{
					CString strError;
					strError.Format(_T("Failed to find IP filter file \"guarding.p2p\" or \"ipfilter.dat\" in RAR file \"%s\"."), strTempFilePath);
					AfxMessageBox(strError, MB_ICONERROR);
				}
				rar.Close();
			}
			else
			{
				CString strError;
				strError.Format(_T("Failed to open file \"%s\".\r\n\r\nInvalid file format?\r\n\r\nDownload latest version of UNRAR.DLL from http://www.rarlab.com and copy UNRAR.DLL into eMule installation folder."), url);
				AfxMessageBox(strError, MB_ICONERROR);
			}
		}
		else
		{
			CGZIPFile gz;
			if (gz.Open(strTempFilePath))
			{
				bIsArchiveFile = true;

				CString strTempUnzipFilePath;
				_tmakepathlimit(strTempUnzipFilePath.GetBuffer(_MAX_PATH), NULL, thePrefs.GetMuleDirectory(EMULE_CONFIGDIR), DFLT_IPFILTER_FILENAME, _T(".unzip.tmp"));
				strTempUnzipFilePath.ReleaseBuffer();

				// add filename and extension of uncompressed file to temporary file
				CString strUncompressedFileName = gz.GetUncompressedFileName();
				if (!strUncompressedFileName.IsEmpty())
				{
					strTempUnzipFilePath += _T('.');
					strTempUnzipFilePath += strUncompressedFileName;
				}

				if (gz.Extract(strTempUnzipFilePath))
				{
					gz.Close();

					if (_tremove(theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE(_T("*** Error: Failed to remove default IP filter file \"%s\" - %s\n"), theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
					if (_trename(strTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath()) != 0)
						TRACE(_T("*** Error: Failed to rename uncompressed IP filter file \"%s\" to default IP filter file \"%s\" - %s\n"), strTempUnzipFilePath, theApp.ipfilter->GetDefaultFilePath(), _tcserror(errno));
					if (_tremove(strTempFilePath) != 0)
						TRACE(_T("*** Error: Failed to remove temporary IP filter file \"%s\" - %s\n"), strTempFilePath, _tcserror(errno));
					bUncompressed = true;
					bHaveNewFilterFile = true;
				}
				else {
					CString strError;
					strError.Format(GetResString(IDS_ERR_IPFILTERZIPEXTR), strTempFilePath);
					AfxMessageBox(strError, MB_ICONERROR);
				}
			}
			gz.Close();
		}

		if (!bIsArchiveFile && !bUncompressed)
		{
			// Check first lines of downloaded file for potential HTML content (e.g. 404 error pages)
			bool bValidIPFilterFile = true;
			FILE* fp = _tfsopen(strTempFilePath, _T("rb"), _SH_DENYWR);
			if (fp)
			{
				char szBuff[16384];
				int iRead = fread(szBuff, 1, _countof(szBuff)-1, fp);
				if (iRead <= 0)
					bValidIPFilterFile = false;
				else
				{
					szBuff[iRead-1] = '\0';

					const char* pc = szBuff;
					while (*pc == ' ' || *pc == '\t' || *pc == '\r' || *pc == '\n')
						pc++;
					if (_strnicmp(pc, "<html", 5) == 0
						|| _strnicmp(pc, "<xml", 4) == 0
						|| _strnicmp(pc, "<!doc", 5) == 0)
					{
						bValidIPFilterFile = false;
					}
				}
				fclose(fp);
			}

			if (bValidIPFilterFile)
			{
				(void)_tremove(theApp.ipfilter->GetDefaultFilePath());
				VERIFY( _trename(strTempFilePath, theApp.ipfilter->GetDefaultFilePath()) == 0 );
				bHaveNewFilterFile = true;
			}
			else
			{
				AfxMessageBox(GetResString(IDS_DWLIPFILTERFAILED), MB_ICONERROR);
			}
		}
	}

	if (url.IsEmpty() || bHaveNewFilterFile)
		OnReloadIPFilter();

	// In case we received an invalid IP-filter file (e.g. an 404 HTML page with HTTP status "OK"),
	// warn the user that there are no IP-filters available any longer.
	if (bHaveNewFilterFile && theApp.ipfilter->GetIPFilter().GetCount() == 0)
	{
		CString strLoaded;
		strLoaded.Format(GetResString(IDS_IPFILTERLOADED), theApp.ipfilter->GetIPFilter().GetCount());
		CString strError;
		strError.Format(_T("%s\r\n\r\n%s"), GetResString(IDS_DWLIPFILTERFAILED), strLoaded);
		AfxMessageBox(strError, MB_ICONERROR);
	}
}

void CPPgSecurity::OnDestroy()
{
	DeleteDDB();
	CPropertyPage::OnDestroy();
}

void CPPgSecurity::DeleteDDB()
{
	if (m_pacIPFilterURL)
	{
		m_pacIPFilterURL->SaveList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + IPFILTERUPDATEURL_STRINGS_PROFILE);
		m_pacIPFilterURL->Unbind();
		m_pacIPFilterURL->Release();
		m_pacIPFilterURL = NULL;
	}
}

BOOL CPPgSecurity::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN){

		if (pMsg->wParam == VK_ESCAPE)
			return FALSE;

		if (pMsg->wParam == VK_DELETE && m_pacIPFilterURL && m_pacIPFilterURL->IsBound() && pMsg->hwnd == GetDlgItem(IDC_UPDATEURL)->m_hWnd)
		{
			if (GetAsyncKeyState(VK_MENU)<0 || GetAsyncKeyState(VK_CONTROL)<0)
				m_pacIPFilterURL->Clear();
			else
				m_pacIPFilterURL->RemoveSelectedItem();
		}

		if (pMsg->wParam == VK_RETURN){
			if (pMsg->hwnd == GetDlgItem(IDC_UPDATEURL)->m_hWnd){
				if (m_pacIPFilterURL && m_pacIPFilterURL->IsBound() ){
					CString strText;
					GetDlgItem(IDC_UPDATEURL)->GetWindowText(strText);
					if (!strText.IsEmpty()){
						GetDlgItem(IDC_UPDATEURL)->SetWindowText(_T("")); // this seems to be the only chance to let the dropdown list to disapear
						GetDlgItem(IDC_UPDATEURL)->SetWindowText(strText);
						((CEdit*)GetDlgItem(IDC_UPDATEURL))->SetSel(strText.GetLength(), strText.GetLength());
					}
				}
				return TRUE;
			}
		}
	}
   
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CPPgSecurity::OnEnChangeUpdateUrl()
{
	CString strUrl;
	GetDlgItemText(IDC_UPDATEURL, strUrl);
	GetDlgItem(IDC_LOADURL)->EnableWindow(!strUrl.IsEmpty());
}

void CPPgSecurity::OnDDClicked()
{
	CWnd* box=GetDlgItem(IDC_UPDATEURL);
	box->SetFocus();
	box->SetWindowText(_T(""));
	box->SendMessage(WM_KEYDOWN,VK_DOWN,0x00510001);
}

void CPPgSecurity::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Security);
}

BOOL CPPgSecurity::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgSecurity::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPPgSecurity::OnBnClickedRunAsUser()
{
	if ( ((CButton*)GetDlgItem(IDC_RUNASUSER))->GetCheck() == BST_CHECKED){
		if (AfxMessageBox(GetResString(IDS_RAU_WARNING),MB_OKCANCEL | MB_ICONINFORMATION,0) == IDCANCEL)
			((CButton*)GetDlgItem(IDC_RUNASUSER))->SetCheck(BST_UNCHECKED);
	}
	OnSettingsChange();
}

void CPPgSecurity::OnObfuscatedDisabledChange(){
	if (IsDlgButtonChecked(IDC_DISABLEOBFUSCATION) != 0){
		GetDlgItem(IDC_ENABLEOBFUSCATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(FALSE);
		CheckDlgButton(IDC_ENABLEOBFUSCATION, 0);
		CheckDlgButton(IDC_ONLYOBFUSCATED, 0);
	}
	else{
		GetDlgItem(IDC_ENABLEOBFUSCATION)->EnableWindow(TRUE);
	}
	OnSettingsChange();
}

void CPPgSecurity::OnObfuscatedRequestedChange(){
	if (IsDlgButtonChecked(IDC_ENABLEOBFUSCATION) == 0){
		GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(FALSE);
		CheckDlgButton(IDC_ONLYOBFUSCATED, 0);
	}
	else{
		GetDlgItem(IDC_ENABLEOBFUSCATION)->EnableWindow(TRUE);
		GetDlgItem(IDC_ONLYOBFUSCATED)->EnableWindow(TRUE);
	}
	OnSettingsChange();
}
