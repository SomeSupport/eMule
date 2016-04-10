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
#include "PPgFiles.h"
#include "Inputbox.h"
#include "OtherFunctions.h"
#include "TransferDlg.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "HelpIDs.h"
#include ".\ppgfiles.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgFiles, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgFiles, CPropertyPage)
	ON_BN_CLICKED(IDC_PF_TIMECALC, OnSettingsChange)
	ON_BN_CLICKED(IDC_UAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_DAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_PREVIEWPRIO, OnSettingsChange)
	ON_BN_CLICKED(IDC_ADDNEWFILESPAUSED, OnSettingsChange)
	ON_BN_CLICKED(IDC_FULLCHUNKTRANS, OnSettingsChange)
	ON_BN_CLICKED(IDC_STARTNEXTFILE, OnSettingsChange)
	ON_BN_CLICKED(IDC_WATCHCB, OnSettingsChange)
	ON_BN_CLICKED(IDC_STARTNEXTFILECAT, OnSettingsChangeCat1)
	ON_BN_CLICKED(IDC_STARTNEXTFILECAT2, OnSettingsChangeCat2)
	ON_BN_CLICKED(IDC_FNCLEANUP, OnSettingsChange)
	ON_BN_CLICKED(IDC_FNC, OnSetCleanupFilter)
	ON_EN_CHANGE(IDC_VIDEOPLAYER, OnSettingsChange)
	ON_EN_CHANGE(IDC_VIDEOPLAYER_ARGS, OnSettingsChange)
	ON_BN_CLICKED(IDC_VIDEOBACKUP, OnSettingsChange)
	ON_BN_CLICKED(IDC_REMEMBERDOWNLOADED, OnSettingsChange) 
	ON_BN_CLICKED(IDC_REMEMBERCANCELLED, OnSettingsChange) 
	ON_BN_CLICKED(IDC_BROWSEV, BrowseVideoplayer)
	ON_WM_HELPINFO()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CPPgFiles::CPPgFiles()
	: CPropertyPage(CPPgFiles::IDD)
{
	m_icoBrowse = NULL;
}

CPPgFiles::~CPPgFiles()
{
}

void CPPgFiles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BOOL CPPgFiles::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	AddBuddyButton(GetDlgItem(IDC_VIDEOPLAYER)->m_hWnd, ::GetDlgItem(m_hWnd, IDC_BROWSEV));
	InitAttachedBrowseButton(::GetDlgItem(m_hWnd, IDC_BROWSEV), m_icoBrowse);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgFiles::LoadSettings(void)
{
	if (thePrefs.addnewfilespaused)
		CheckDlgButton(IDC_ADDNEWFILESPAUSED,1);
	else
		CheckDlgButton(IDC_ADDNEWFILESPAUSED,0);

	if (thePrefs.m_bUseOldTimeRemaining)
		CheckDlgButton(IDC_PF_TIMECALC,0);
	else
		CheckDlgButton(IDC_PF_TIMECALC,1);

	if (thePrefs.m_bpreviewprio)
		CheckDlgButton(IDC_PREVIEWPRIO,1);
	else
		CheckDlgButton(IDC_PREVIEWPRIO,0);

	if (thePrefs.m_bDAP)
		CheckDlgButton(IDC_DAP,1);
	else
		CheckDlgButton(IDC_DAP,0);

	if (thePrefs.m_bUAP)
		CheckDlgButton(IDC_UAP,1);
	else
		CheckDlgButton(IDC_UAP,0);

	if (thePrefs.m_btransferfullchunks)
		CheckDlgButton(IDC_FULLCHUNKTRANS,1);
	else
		CheckDlgButton(IDC_FULLCHUNKTRANS,0);

	CheckDlgButton( IDC_STARTNEXTFILECAT, FALSE);
	CheckDlgButton( IDC_STARTNEXTFILECAT2, FALSE);
	if (thePrefs.m_istartnextfile)
	{
		CheckDlgButton(IDC_STARTNEXTFILE,1);
		if (thePrefs.m_istartnextfile == 2)
			CheckDlgButton( IDC_STARTNEXTFILECAT, TRUE);
		else if (thePrefs.m_istartnextfile == 3)
			CheckDlgButton( IDC_STARTNEXTFILECAT2, TRUE);
	}
	else
		CheckDlgButton(IDC_STARTNEXTFILE,0);

	GetDlgItem(IDC_VIDEOPLAYER)->SetWindowText(thePrefs.m_strVideoPlayer);
	GetDlgItem(IDC_VIDEOPLAYER_ARGS)->SetWindowText(thePrefs.m_strVideoPlayerArgs);
	if (thePrefs.moviePreviewBackup)
		CheckDlgButton(IDC_VIDEOBACKUP,1);
	else
		CheckDlgButton(IDC_VIDEOBACKUP,0);

	CheckDlgButton(IDC_FNCLEANUP, (uint8)thePrefs.AutoFilenameCleanup());

	if (thePrefs.watchclipboard)
		CheckDlgButton(IDC_WATCHCB,1);
	else
		CheckDlgButton(IDC_WATCHCB,0);

	if (thePrefs.IsRememberingDownloadedFiles())
		CheckDlgButton(IDC_REMEMBERDOWNLOADED,1);
	else
		CheckDlgButton(IDC_REMEMBERDOWNLOADED,0);

	if (thePrefs.IsRememberingCancelledFiles())
		CheckDlgButton(IDC_REMEMBERCANCELLED,1);
	else
		CheckDlgButton(IDC_REMEMBERCANCELLED,0);
	
	GetDlgItem(IDC_STARTNEXTFILECAT)->EnableWindow(IsDlgButtonChecked(IDC_STARTNEXTFILE));
}

BOOL CPPgFiles::OnApply()
{
	CString buffer;

    bool bOldPreviewPrio = thePrefs.m_bpreviewprio;
	if (IsDlgButtonChecked(IDC_PREVIEWPRIO))
		thePrefs.m_bpreviewprio = true;
	else
		thePrefs.m_bpreviewprio = false;

    if (bOldPreviewPrio != thePrefs.m_bpreviewprio)
		theApp.emuledlg->transferwnd->GetDownloadList()->CreateMenues();

	if (IsDlgButtonChecked(IDC_DAP))
		thePrefs.m_bDAP = true;
	else
		thePrefs.m_bDAP = false;

	if (IsDlgButtonChecked(IDC_UAP))
		thePrefs.m_bUAP = true;
	else
		thePrefs.m_bUAP = false;

	if (IsDlgButtonChecked(IDC_STARTNEXTFILE))
	{
		thePrefs.m_istartnextfile = 1;
		if (IsDlgButtonChecked(IDC_STARTNEXTFILECAT))
			thePrefs.m_istartnextfile = 2;
		else if (IsDlgButtonChecked(IDC_STARTNEXTFILECAT2))
			thePrefs.m_istartnextfile = 3;
	}
	else
		thePrefs.m_istartnextfile = 0;

	if (IsDlgButtonChecked(IDC_FULLCHUNKTRANS))
		thePrefs.m_btransferfullchunks = true;
	else
		thePrefs.m_btransferfullchunks = false;


	if (IsDlgButtonChecked(IDC_WATCHCB))
		thePrefs.watchclipboard = true;
	else
		thePrefs.watchclipboard = false;

	if (IsDlgButtonChecked(IDC_REMEMBERDOWNLOADED))
		thePrefs.SetRememberDownloadedFiles(true);
	else
		thePrefs.SetRememberDownloadedFiles(false);

	if (IsDlgButtonChecked(IDC_REMEMBERCANCELLED))
		thePrefs.SetRememberCancelledFiles(true);
	else
		thePrefs.SetRememberCancelledFiles(false);

	thePrefs.addnewfilespaused = IsDlgButtonChecked(IDC_ADDNEWFILESPAUSED)!=0;
	thePrefs.autofilenamecleanup = IsDlgButtonChecked(IDC_FNCLEANUP)!=0;
	thePrefs.m_bUseOldTimeRemaining = IsDlgButtonChecked(IDC_PF_TIMECALC)==0;

	GetDlgItem(IDC_VIDEOPLAYER)->GetWindowText(thePrefs.m_strVideoPlayer);
	thePrefs.m_strVideoPlayer.Trim();
	GetDlgItem(IDC_VIDEOPLAYER_ARGS)->GetWindowText(thePrefs.m_strVideoPlayerArgs);
	thePrefs.m_strVideoPlayerArgs.Trim();
	thePrefs.moviePreviewBackup = IsDlgButtonChecked(IDC_VIDEOBACKUP)!=0;

	LoadSettings();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgFiles::Localize(void)
{
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_FILES));
		GetDlgItem(IDC_LBL_MISC)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_PF_TIMECALC)->SetWindowText(GetResString(IDS_PF_ADVANCEDCALC));
		GetDlgItem(IDC_UAP)->SetWindowText(GetResString(IDS_PW_UAP));
		GetDlgItem(IDC_DAP)->SetWindowText(GetResString(IDS_PW_DAP));
		GetDlgItem(IDC_PREVIEWPRIO)->SetWindowText(GetResString(IDS_DOWNLOADMOVIECHUNKS));
		GetDlgItem(IDC_ADDNEWFILESPAUSED)->SetWindowText(GetResString(IDS_ADDNEWFILESPAUSED));
		GetDlgItem(IDC_WATCHCB)->SetWindowText(GetResString(IDS_PF_WATCHCB));
		GetDlgItem(IDC_FULLCHUNKTRANS)->SetWindowText(GetResString(IDS_FULLCHUNKTRANS));
		GetDlgItem(IDC_STARTNEXTFILE)->SetWindowText(GetResString(IDS_STARTNEXTFILE));
		GetDlgItem(IDC_STARTNEXTFILECAT)->SetWindowText(GetResString(IDS_PREF_STARTNEXTFILECAT));
		GetDlgItem(IDC_STARTNEXTFILECAT2)->SetWindowText(GetResString(IDS_PREF_STARTNEXTFILECATONLY));
		GetDlgItem(IDC_FNC)->SetWindowText(GetResString(IDS_EDIT));
		GetDlgItem(IDC_ONND)->SetWindowText(GetResString(IDS_ONNEWDOWNLOAD));
		GetDlgItem(IDC_FNCLEANUP)->SetWindowText(GetResString(IDS_AUTOCLEANUPFN));
		GetDlgItem(IDC_STATICVIDEOPLAYER)->SetWindowText(GetResString(IDS_PW_VIDEOPLAYER));
		GetDlgItem(IDC_VIDEOPLAYER_CMD_LBL)->SetWindowText(GetResString(IDS_COMMAND));
		GetDlgItem(IDC_VIDEOPLAYER_ARGS_LBL)->SetWindowText(GetResString(IDS_ARGUMENTS));
		GetDlgItem(IDC_VIDEOBACKUP)->SetWindowText(GetResString(IDS_VIDEOBACKUP));		
		GetDlgItem(IDC_REMEMBERDOWNLOADED)->SetWindowText(GetResString(IDS_PW_REMEMBERDOWNLOADED));
		GetDlgItem(IDC_REMEMBERCANCELLED)->SetWindowText(GetResString(IDS_PW_REMEMBERCANCELLED));		
	}
}

void CPPgFiles::OnSetCleanupFilter()
{
	CString prompt = GetResString(IDS_FILTERFILENAMEWORD);
	InputBox inputbox;
	inputbox.SetLabels(GetResString(IDS_FNFILTERTITLE), prompt, thePrefs.GetFilenameCleanups());
	inputbox.DoModal();
	if (!inputbox.WasCancelled())
		thePrefs.SetFilenameCleanups(inputbox.GetInput());
}

void CPPgFiles::BrowseVideoplayer()
{
	CString strPlayerPath;
	GetDlgItemText(IDC_VIDEOPLAYER, strPlayerPath);
	CFileDialog dlgFile(TRUE, _T("exe"), strPlayerPath, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, _T("Executable (*.exe)|*.exe||"), NULL, 0);
	if (dlgFile.DoModal() == IDOK)
	{
		GetDlgItem(IDC_VIDEOPLAYER)->SetWindowText(dlgFile.GetPathName());
		SetModified();
	}
}

void CPPgFiles::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Files);
}

BOOL CPPgFiles::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgFiles::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPPgFiles::OnSettingsChange()
{
	SetModified();
	GetDlgItem(IDC_STARTNEXTFILECAT)->EnableWindow(IsDlgButtonChecked(IDC_STARTNEXTFILE));
	GetDlgItem(IDC_STARTNEXTFILECAT2)->EnableWindow(IsDlgButtonChecked(IDC_STARTNEXTFILE));
}

void CPPgFiles::OnSettingsChangeCat(uint8 index)
{
	bool on = IsDlgButtonChecked(index == 1 ? IDC_STARTNEXTFILECAT : IDC_STARTNEXTFILECAT2)!=0;
	if (on)
		CheckDlgButton(index == 1 ? IDC_STARTNEXTFILECAT2 : IDC_STARTNEXTFILECAT, FALSE);
	OnSettingsChange();
}

void CPPgFiles::OnDestroy()
{
	CPropertyPage::OnDestroy();
	if (m_icoBrowse)
	{
		VERIFY( DestroyIcon(m_icoBrowse) );
		m_icoBrowse = NULL;
	}
}
