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
#include "ServerWnd.h"
#include "PPgServer.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "HelpIDs.h"
#include "Opcodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgServer, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgServer, CPropertyPage)
	ON_EN_CHANGE(IDC_SERVERRETRIES, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOSERVER, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATESERVERCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATESERVERCLIENT, OnSettingsChange)
	ON_BN_CLICKED(IDC_SCORE, OnSettingsChange)
	ON_BN_CLICKED(IDC_SMARTIDCHECK, OnSettingsChange)
	ON_BN_CLICKED(IDC_SAFESERVERCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOCONNECTSTATICONLY, OnSettingsChange)
	ON_BN_CLICKED(IDC_MANUALSERVERHIGHPRIO, OnSettingsChange)
	ON_BN_CLICKED(IDC_EDITADR, OnBnClickedEditadr)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPPgServer::CPPgServer()
	: CPropertyPage(CPPgServer::IDD)
{
}

CPPgServer::~CPPgServer()
{
}

void CPPgServer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BOOL CPPgServer::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgServer::LoadSettings(void)
{
	SetDlgItemInt(IDC_SERVERRETRIES, thePrefs.m_uDeadServerRetries, FALSE);
	CheckDlgButton(IDC_AUTOSERVER, thePrefs.m_bAutoUpdateServerList);
	CheckDlgButton(IDC_UPDATESERVERCONNECT, thePrefs.m_bAddServersFromServer);
	CheckDlgButton(IDC_UPDATESERVERCLIENT, thePrefs.m_bAddServersFromClients);
	CheckDlgButton(IDC_SCORE, thePrefs.m_bUseServerPriorities);
	CheckDlgButton(IDC_SMARTIDCHECK, thePrefs.m_bSmartServerIdCheck);
	CheckDlgButton(IDC_SAFESERVERCONNECT, thePrefs.m_bSafeServerConnect);
	CheckDlgButton(IDC_AUTOCONNECTSTATICONLY, thePrefs.m_bAutoConnectToStaticServersOnly);
	CheckDlgButton(IDC_MANUALSERVERHIGHPRIO, thePrefs.m_bManualAddedServersHighPriority);
}

BOOL CPPgServer::OnApply()
{
	UINT uCurDeadServerRetries = thePrefs.m_uDeadServerRetries;
	thePrefs.m_uDeadServerRetries = GetDlgItemInt(IDC_SERVERRETRIES, NULL, FALSE);
	if (thePrefs.m_uDeadServerRetries < 1)
		thePrefs.m_uDeadServerRetries = 1;
	else if (thePrefs.m_uDeadServerRetries > MAX_SERVERFAILCOUNT)
		thePrefs.m_uDeadServerRetries = MAX_SERVERFAILCOUNT;
	if (uCurDeadServerRetries != thePrefs.m_uDeadServerRetries) {
		theApp.emuledlg->serverwnd->serverlistctrl.Invalidate();
		theApp.emuledlg->serverwnd->serverlistctrl.UpdateWindow();
	}
	thePrefs.m_bAutoUpdateServerList = IsDlgButtonChecked(IDC_AUTOSERVER)!=0;
	thePrefs.m_bAddServersFromServer = IsDlgButtonChecked(IDC_UPDATESERVERCONNECT)!=0;
	thePrefs.m_bAddServersFromClients = IsDlgButtonChecked(IDC_UPDATESERVERCLIENT)!=0;
	thePrefs.m_bUseServerPriorities = IsDlgButtonChecked(IDC_SCORE)!=0;
	thePrefs.m_bSmartServerIdCheck = IsDlgButtonChecked(IDC_SMARTIDCHECK)!=0;
	thePrefs.SetSafeServerConnectEnabled(IsDlgButtonChecked(IDC_SAFESERVERCONNECT)!=0);
	thePrefs.m_bAutoConnectToStaticServersOnly = IsDlgButtonChecked(IDC_AUTOCONNECTSTATICONLY)!=0;
	thePrefs.m_bManualAddedServersHighPriority = IsDlgButtonChecked(IDC_MANUALSERVERHIGHPRIO)!=0;

	LoadSettings();

	SetModified();
	return CPropertyPage::OnApply();
}

void CPPgServer::Localize(void)
{
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_SERVER));
		GetDlgItem(IDC_LBL_UPDATE_SERVERS)->SetWindowText(GetResString(IDS_SV_UPDATE));
		GetDlgItem(IDC_LBL_MISC)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_REMOVEDEAD)->SetWindowText(GetResString(IDS_PW_RDEAD));
		GetDlgItem(IDC_RETRIES_LBL)->SetWindowText(GetResString(IDS_PW_RETRIES));
		GetDlgItem(IDC_UPDATESERVERCONNECT)->SetWindowText(GetResString(IDS_PW_USC));
		GetDlgItem(IDC_UPDATESERVERCLIENT)->SetWindowText(GetResString(IDS_PW_UCC));
		GetDlgItem(IDC_AUTOSERVER)->SetWindowText(GetResString(IDS_PW_USS));
		GetDlgItem(IDC_SMARTIDCHECK)->SetWindowText(GetResString(IDS_SMARTLOWIDCHECK));
		GetDlgItem(IDC_SAFESERVERCONNECT)->SetWindowText(GetResString(IDS_PW_FASTSRVCON));
		GetDlgItem(IDC_SCORE)->SetWindowText(GetResString(IDS_PW_SCORE));
		GetDlgItem(IDC_MANUALSERVERHIGHPRIO)->SetWindowText(GetResString(IDS_MANUALSERVERHIGHPRIO));
		GetDlgItem(IDC_EDITADR)->SetWindowText(GetResString(IDS_EDITLIST));
		GetDlgItem(IDC_AUTOCONNECTSTATICONLY)->SetWindowText(GetResString(IDS_PW_AUTOCONNECTSTATICONLY));
	}
}

void CPPgServer::OnBnClickedEditadr()
{
	ShellExecute(NULL, _T("open"), thePrefs.GetTxtEditor(), _T("\"") + thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("addresses.dat\""), NULL, SW_SHOW); 
}

void CPPgServer::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Server);
}

BOOL CPPgServer::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgServer::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}
