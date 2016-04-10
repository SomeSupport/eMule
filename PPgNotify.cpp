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
#include "PPgNotify.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "HelpIDs.h"
#include "TextToSpeech.h"
#include "TaskbarNotifier.h"
#include ".\ppgnotify.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgNotify, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgNotify, CPropertyPage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_CB_TBN_NOSOUND, OnBnClickedNoSound)
	ON_BN_CLICKED(IDC_CB_TBN_USESOUND, OnBnClickedUseSound)
	ON_BN_CLICKED(IDC_CB_TBN_USESPEECH, OnBnClickedUseSpeech)
	ON_EN_CHANGE(IDC_EDIT_TBN_WAVFILE, OnSettingsChange)
	ON_BN_CLICKED(IDC_BTN_BROWSE_WAV, OnBnClickedBrowseAudioFile)
	ON_BN_CLICKED(IDC_TEST_NOTIFICATION, OnBnClickedTestNotification)
	ON_BN_CLICKED(IDC_CB_TBN_ONNEWDOWNLOAD, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_ONDOWNLOAD, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_ONLOG, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_ONCHAT, OnBnClickedOnChat)
	ON_BN_CLICKED(IDC_CB_TBN_IMPORTATNT , OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_POP_ALWAYS, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_ONNEWVERSION, OnSettingsChange)
	ON_EN_CHANGE(IDC_EDIT_SMTPSERVER, OnSettingsChange)
	ON_EN_CHANGE(IDC_EDIT_SENDER, OnSettingsChange)
	ON_EN_CHANGE(IDC_EDIT_RECEIVER, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_ENABLENOTIFICATIONS, OnBnClickedCbEnablenotifications)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CPPgNotify::CPPgNotify()
	: CPropertyPage(CPPgNotify::IDD)
{
	m_bEnableEMail = true;
	m_icoBrowse = NULL;
}

CPPgNotify::~CPPgNotify()
{
}

void CPPgNotify::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BOOL CPPgNotify::OnInitDialog()
{
#if _ATL_VER >= 0x0710
	m_bEnableEMail = (IsRunningXPSP2OrHigher() > 0);
#endif

	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	AddBuddyButton(GetDlgItem(IDC_EDIT_TBN_WAVFILE)->m_hWnd, ::GetDlgItem(m_hWnd, IDC_BTN_BROWSE_WAV));
	InitAttachedBrowseButton(::GetDlgItem(m_hWnd, IDC_BTN_BROWSE_WAV), m_icoBrowse);

	int iBtnID;
	if (thePrefs.notifierSoundType == ntfstSoundFile)
		iBtnID = IDC_CB_TBN_USESOUND;
	else if (thePrefs.notifierSoundType == ntfstSpeech)
		iBtnID = IDC_CB_TBN_USESPEECH;
	else {
		ASSERT( thePrefs.notifierSoundType == ntfstNoSound );
		iBtnID = IDC_CB_TBN_NOSOUND;
	}
	ASSERT( IDC_CB_TBN_NOSOUND < IDC_CB_TBN_USESOUND && IDC_CB_TBN_USESOUND < IDC_CB_TBN_USESPEECH );
	CheckRadioButton(IDC_CB_TBN_NOSOUND, IDC_CB_TBN_USESPEECH, iBtnID);

	CheckDlgButton(IDC_CB_TBN_ONDOWNLOAD, thePrefs.notifierOnDownloadFinished ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CB_TBN_ONNEWDOWNLOAD, thePrefs.notifierOnNewDownload ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CB_TBN_ONCHAT, thePrefs.notifierOnChat ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CB_TBN_ONLOG, thePrefs.notifierOnLog ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CB_TBN_IMPORTATNT, thePrefs.notifierOnImportantError ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CB_TBN_POP_ALWAYS, thePrefs.notifierOnEveryChatMsg ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CB_TBN_ONNEWVERSION, thePrefs.notifierOnNewVersion ? BST_CHECKED : BST_UNCHECKED);

	CButton* btnPTR = (CButton*)GetDlgItem(IDC_CB_TBN_POP_ALWAYS);
	btnPTR->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_ONCHAT));

	SetDlgItemText(IDC_EDIT_TBN_WAVFILE, thePrefs.notifierSoundFile);

	if (!m_bEnableEMail){
		CheckDlgButton(IDC_CB_ENABLENOTIFICATIONS, BST_UNCHECKED);
		GetDlgItem(IDC_EMAILNOT_GROUP)->EnableWindow(FALSE);
		GetDlgItem(IDC_CB_ENABLENOTIFICATIONS)->EnableWindow(FALSE);
		GetDlgItem(IDC_TXT_SMTPSERVER)->EnableWindow(FALSE);
		GetDlgItem(IDC_TXT_RECEIVER)->EnableWindow(FALSE);
		GetDlgItem(IDC_TXT_SENDER)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_SMTPSERVER)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_RECEIVER)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_SENDER)->EnableWindow(FALSE);
	}
	else{
		SetDlgItemText(IDC_EDIT_SMTPSERVER, thePrefs.GetNotifierMailServer());
		SetDlgItemText(IDC_EDIT_RECEIVER, thePrefs.GetNotifierMailReceiver());
		SetDlgItemText(IDC_EDIT_SENDER, thePrefs.GetNotifierMailSender());
		if (thePrefs.IsNotifierSendMailEnabled()){
			CheckDlgButton(IDC_CB_ENABLENOTIFICATIONS, BST_CHECKED);
			GetDlgItem(IDC_EDIT_SMTPSERVER)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_RECEIVER)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_SENDER)->EnableWindow(TRUE);
		}
		else{
			CheckDlgButton(IDC_CB_ENABLENOTIFICATIONS, BST_UNCHECKED);
			GetDlgItem(IDC_EDIT_SMTPSERVER)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_RECEIVER)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_SENDER)->EnableWindow(FALSE);
		}
	}

	UpdateControls();
	Localize();

	GetDlgItem(IDC_CB_TBN_USESPEECH)->EnableWindow(IsSpeechEngineAvailable());

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgNotify::UpdateControls()
{
	GetDlgItem(IDC_EDIT_TBN_WAVFILE)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_USESOUND));
	GetDlgItem(IDC_BTN_BROWSE_WAV)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_USESOUND));
	GetDlgItem(IDC_EDIT_SMTPSERVER)->EnableWindow(IsDlgButtonChecked(IDC_CB_ENABLENOTIFICATIONS));
	GetDlgItem(IDC_EDIT_RECEIVER)->EnableWindow(IsDlgButtonChecked(IDC_CB_ENABLENOTIFICATIONS));
	GetDlgItem(IDC_EDIT_SENDER)->EnableWindow(IsDlgButtonChecked(IDC_CB_ENABLENOTIFICATIONS));
}

void CPPgNotify::Localize(void)
{
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_EKDEV_OPTIONS));
		GetDlgItem(IDC_CB_TBN_USESOUND)->SetWindowText(GetResString(IDS_PW_TBN_USESOUND));
		GetDlgItem(IDC_CB_TBN_NOSOUND)->SetWindowText(GetResString(IDS_NOSOUND));
		GetDlgItem(IDC_CB_TBN_ONLOG)->SetWindowText(GetResString(IDS_PW_TBN_ONLOG));
		GetDlgItem(IDC_CB_TBN_ONCHAT)->SetWindowText(GetResString(IDS_PW_TBN_ONCHAT));
		GetDlgItem(IDC_CB_TBN_POP_ALWAYS)->SetWindowText(GetResString(IDS_PW_TBN_POP_ALWAYS));
		GetDlgItem(IDC_CB_TBN_ONDOWNLOAD)->SetWindowText(GetResString(IDS_PW_TBN_ONDOWNLOAD) + _T(" (*)") );
		GetDlgItem(IDC_CB_TBN_ONNEWDOWNLOAD)->SetWindowText(GetResString(IDS_TBN_ONNEWDOWNLOAD));
		GetDlgItem(IDC_TASKBARNOTIFIER)->SetWindowText(GetResString(IDS_PW_TASKBARNOTIFIER));
		GetDlgItem(IDC_CB_TBN_IMPORTATNT)->SetWindowText(GetResString(IDS_PS_TBN_IMPORTANT) + _T(" (*)"));
		GetDlgItem(IDC_CB_TBN_ONNEWVERSION)->SetWindowText(GetResString(IDS_CB_TBN_ONNEWVERSION));
		GetDlgItem(IDC_TBN_OPTIONS)->SetWindowText(GetResString(IDS_PW_TBN_OPTIONS));
		GetDlgItem(IDC_CB_TBN_USESPEECH)->SetWindowText(GetResString(IDS_USESPEECH));
		GetDlgItem(IDC_EMAILNOT_GROUP)->SetWindowText(GetResString(IDS_PW_EMAILNOTIFICATIONS) + _T(" (*)"));
		GetDlgItem(IDC_TXT_SMTPSERVER)->SetWindowText(GetResString(IDS_PW_SMTPSERVER));
		GetDlgItem(IDC_TXT_RECEIVER)->SetWindowText(GetResString(IDS_PW_RECEIVERADDRESS));
		GetDlgItem(IDC_TXT_SENDER)->SetWindowText(GetResString(IDS_PW_SENDERADDRESS));
		GetDlgItem(IDC_CB_ENABLENOTIFICATIONS)->SetWindowText(GetResString(IDS_PW_ENABLEEMAIL));
		SetDlgItemText(IDC_TEST_NOTIFICATION, GetResString(IDS_TEST) );
	}
}

BOOL CPPgNotify::OnApply()
{
    thePrefs.notifierOnDownloadFinished = IsDlgButtonChecked(IDC_CB_TBN_ONDOWNLOAD)!=0;
    thePrefs.notifierOnNewDownload = IsDlgButtonChecked(IDC_CB_TBN_ONNEWDOWNLOAD)!=0;
    thePrefs.notifierOnChat = IsDlgButtonChecked(IDC_CB_TBN_ONCHAT)!=0;
    thePrefs.notifierOnLog = IsDlgButtonChecked(IDC_CB_TBN_ONLOG)!=0;
	thePrefs.notifierOnImportantError = IsDlgButtonChecked(IDC_CB_TBN_IMPORTATNT)!=0;
    thePrefs.notifierOnEveryChatMsg = IsDlgButtonChecked(IDC_CB_TBN_POP_ALWAYS)!=0;
	thePrefs.notifierOnNewVersion = IsDlgButtonChecked(IDC_CB_TBN_ONNEWVERSION)!=0;

	if (m_bEnableEMail){
		GetDlgItemText(IDC_EDIT_SMTPSERVER, thePrefs.m_strNotifierMailServer);
		GetDlgItemText(IDC_EDIT_SENDER, thePrefs.m_strNotifierMailSender);
		GetDlgItemText(IDC_EDIT_RECEIVER, thePrefs.m_strNotifierMailReceiver);
		thePrefs.SetNotifierSendMail(IsDlgButtonChecked(IDC_CB_ENABLENOTIFICATIONS) != 0);
	}

	ApplyNotifierSoundType();
	if (thePrefs.notifierSoundType != ntfstSpeech)
		ReleaseTTS();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgNotify::ApplyNotifierSoundType()
{
	GetDlgItemText(IDC_EDIT_TBN_WAVFILE, thePrefs.notifierSoundFile);
	if (IsDlgButtonChecked(IDC_CB_TBN_USESOUND))
		thePrefs.notifierSoundType = ntfstSoundFile;
	else if (IsDlgButtonChecked(IDC_CB_TBN_USESPEECH))
		thePrefs.notifierSoundType = IsSpeechEngineAvailable() ? ntfstSpeech : ntfstNoSound;
	else {
		ASSERT( IsDlgButtonChecked(IDC_CB_TBN_NOSOUND) );
		thePrefs.notifierSoundType = ntfstNoSound;
	}
}

void CPPgNotify::OnBnClickedOnChat()
{
    GetDlgItem(IDC_CB_TBN_POP_ALWAYS)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_ONCHAT));
	SetModified();
}

void CPPgNotify::OnBnClickedBrowseAudioFile()
{
	CString strWavPath;
	GetDlgItemText(IDC_EDIT_TBN_WAVFILE, strWavPath);
	CString buffer;
    if (DialogBrowseFile(buffer, _T("Audio-Files (*.wav)|*.wav||"), strWavPath)){
		SetDlgItemText(IDC_EDIT_TBN_WAVFILE, buffer);
		SetModified();
	}
}

void CPPgNotify::OnBnClickedNoSound()
{
	UpdateControls();
	SetModified();
}

void CPPgNotify::OnBnClickedUseSound()
{
	UpdateControls();
	SetModified();
}

void CPPgNotify::OnBnClickedUseSpeech()
{
	UpdateControls();
	SetModified();
}

void CPPgNotify::OnBnClickedTestNotification()
{
	// save current pref settings
	bool bCurNotifyOnImportantError = thePrefs.notifierOnImportantError;
	ENotifierSoundType iCurSoundType = thePrefs.notifierSoundType;
	CString strSoundFile = thePrefs.notifierSoundFile;

	// temporary apply current settings from dialog
	thePrefs.notifierOnImportantError = true;
	ApplyNotifierSoundType();

	// play test notification
	CString strTest;
	strTest.Format(GetResString(IDS_MAIN_READY), theApp.m_strCurVersionLong);
	theApp.emuledlg->ShowNotifier(strTest, TBN_IMPORTANTEVENT);

	// restore pref settings
	thePrefs.notifierSoundFile = strSoundFile;
	thePrefs.notifierSoundType = iCurSoundType;
	thePrefs.notifierOnImportantError = bCurNotifyOnImportantError;
}

void CPPgNotify::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Notifications);
}

BOOL CPPgNotify::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgNotify::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPPgNotify::OnBnClickedCbEnablenotifications()
{
	UpdateControls();
	SetModified();
}

void CPPgNotify::OnDestroy()
{
	CPropertyPage::OnDestroy();
	if (m_icoBrowse)
	{
		VERIFY( DestroyIcon(m_icoBrowse) );
		m_icoBrowse = NULL;
	}
}
