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
#include "PPgIRC.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "IrcWnd.h"
#include "HelpIDs.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgIRC, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgIRC, CPropertyPage)
	ON_BN_CLICKED(IDC_IRC_USECHANFILTER, OnBtnClickPerform)
	ON_BN_CLICKED(IDC_IRC_USEPERFORM, OnBtnClickPerform)
	ON_EN_CHANGE(IDC_IRC_NICK_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_PERFORM_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_SERVER_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_NAME_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_MINUSER_BOX, OnSettingsChange)
	ON_WM_DESTROY()
	ON_MESSAGE(UM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPPgIRC::CPPgIRC()
	: CPropertyPage(CPPgIRC::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	m_bTimeStamp = false;
	m_bSoundEvents = false;
	m_bMiscMessage = false;
	m_bJoinMessage = false;
	m_bPartMessage = false;
	m_bQuitMessage = false;
	m_bEmuleAllowAddFriend = false;
	m_bEmuleAddFriend = false;
	m_bEmuleSendLink = false;
	m_bAcceptLinks = false;
	m_bIRCAcceptLinksFriendsOnly = false;
	m_bHelpChannel = false;
	m_bChannelsOnConnect = false;
	m_bInitializedTreeOpts = false;
	m_htiTimeStamp = NULL;
	m_htiSoundEvents = NULL;
	m_htiInfoMessage = NULL;
	m_htiMiscMessage = NULL;
	m_htiJoinMessage = NULL;
	m_htiPartMessage = NULL;
	m_htiQuitMessage = NULL;
	m_htiEmuleProto = NULL;
	m_htiEmuleAddFriend = NULL;
	m_htiEmuleAllowAddFriend = NULL;
	m_htiEmuleSendLink = NULL;
	m_htiAcceptLinks = NULL;
	m_htiAcceptLinksFriends = NULL;
	m_htiHelpChannel = NULL;
	m_htiChannelsOnConnect = NULL;
	m_htiSmileys = NULL;
}

CPPgIRC::~CPPgIRC()
{
}

void CPPgIRC::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MISC_IRC, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		m_htiHelpChannel = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_HELPCHANNEL), TVI_ROOT, m_bHelpChannel);
		m_htiChannelsOnConnect = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_LOADCHANNELLISTONCON), TVI_ROOT, m_bChannelsOnConnect);
		m_htiTimeStamp = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ADDTIMESTAMP), TVI_ROOT, m_bTimeStamp);
		m_htiInfoMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREINFOMESSAGE), TVI_ROOT, FALSE);
		m_htiMiscMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREMISCMESSAGE), m_htiInfoMessage, m_bMiscMessage);
		m_htiJoinMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREJOINMESSAGE), m_htiInfoMessage, m_bJoinMessage);
		m_htiPartMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREPARTMESSAGE), m_htiInfoMessage, m_bPartMessage);
		m_htiQuitMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREQUITMESSAGE), m_htiInfoMessage, m_bQuitMessage);
		m_htiEmuleProto = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_IGNOREINFOMESSAGE), TVI_ROOT, FALSE);
		m_htiEmuleAddFriend = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_IGNOREADDFRIEND), m_htiEmuleProto, m_bEmuleAddFriend);
		m_htiEmuleSendLink = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_IGNORESENDLINK), m_htiEmuleProto, m_bEmuleSendLink);
		m_htiEmuleAllowAddFriend = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_ALLOWADDFRIEND), TVI_ROOT, m_bEmuleAllowAddFriend);
		m_htiAcceptLinks = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ACCEPTLINKS), TVI_ROOT, m_bAcceptLinks);
		m_htiAcceptLinksFriends = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ACCEPTLINKSFRIENDS), TVI_ROOT, m_bIRCAcceptLinksFriendsOnly);
		m_htiSmileys = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SHOWSMILEYS), TVI_ROOT, m_bIRCEnableSmileys);
		m_htiSoundEvents = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_SOUNDEVENTS), TVI_ROOT, m_bSoundEvents);

		m_ctrlTreeOptions.Expand(m_htiInfoMessage, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiEmuleProto, TVE_EXPAND);

		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);

        m_bInitializedTreeOpts = true;
	}
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiTimeStamp, m_bTimeStamp);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiSoundEvents, m_bSoundEvents);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiMiscMessage, m_bMiscMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiJoinMessage, m_bJoinMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiPartMessage, m_bPartMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiQuitMessage, m_bQuitMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiEmuleAddFriend, m_bEmuleAddFriend);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiEmuleAllowAddFriend, m_bEmuleAllowAddFriend);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiEmuleSendLink, m_bEmuleSendLink);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiAcceptLinks, m_bAcceptLinks);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiAcceptLinksFriends, m_bIRCAcceptLinksFriendsOnly);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiHelpChannel, m_bHelpChannel);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiChannelsOnConnect, m_bChannelsOnConnect);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiSmileys, m_bIRCEnableSmileys);

	m_ctrlTreeOptions.UpdateCheckBoxGroup(m_htiEmuleProto);
	m_ctrlTreeOptions.UpdateCheckBoxGroup(m_htiInfoMessage);
	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiAcceptLinksFriends, m_bAcceptLinks);
}

BOOL CPPgIRC::OnInitDialog()
{
	m_bTimeStamp = thePrefs.GetIRCAddTimeStamp();
	m_bSoundEvents = thePrefs.GetIRCPlaySoundEvents();
	m_bMiscMessage = thePrefs.GetIRCIgnoreMiscMessages();
	m_bJoinMessage = thePrefs.GetIRCIgnoreJoinMessages();
	m_bPartMessage = thePrefs.GetIRCIgnorePartMessages();
	m_bQuitMessage = thePrefs.GetIRCIgnoreQuitMessages();
	m_bEmuleAddFriend = thePrefs.GetIRCIgnoreEmuleAddFriendMsgs();
	m_bEmuleAllowAddFriend = thePrefs.GetIRCAllowEmuleAddFriend();
	m_bEmuleSendLink = thePrefs.GetIRCIgnoreEmuleSendLinkMsgs();
	m_bAcceptLinks = thePrefs.GetIRCAcceptLinks();
	m_bIRCAcceptLinksFriendsOnly = thePrefs.GetIRCAcceptLinksFriendsOnly();
	m_bHelpChannel = thePrefs.GetIRCJoinHelpChannel();
	m_bChannelsOnConnect = thePrefs.GetIRCGetChannelsOnConnect();
	m_bIRCEnableSmileys = thePrefs.GetIRCEnableSmileys();

	m_ctrlTreeOptions.SetImageListColorFlags(theApp.m_iDfltImageListColorFlags);
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	((CEdit*)GetDlgItem(IDC_IRC_NICK_BOX))->SetLimitText(20);
	((CEdit*)GetDlgItem(IDC_IRC_MINUSER_BOX))->SetLimitText(6);
	((CEdit*)GetDlgItem(IDC_IRC_SERVER_BOX))->SetLimitText(40);
	((CEdit*)GetDlgItem(IDC_IRC_NAME_BOX))->SetLimitText(40);
	((CEdit*)GetDlgItem(IDC_IRC_PERFORM_BOX))->SetLimitText(250);
	LoadSettings();
	Localize();

	UpdateControls();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgIRC::OnKillActive()
{
	// if prop page is closed by pressing ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

void CPPgIRC::LoadSettings(void)
{	
	if(thePrefs.m_bIRCUseChannelFilter)
		CheckDlgButton(IDC_IRC_USECHANFILTER,1);
	else
		CheckDlgButton(IDC_IRC_USECHANFILTER,0);
	
	if(thePrefs.m_bIRCUsePerform)
		CheckDlgButton(IDC_IRC_USEPERFORM,1);
	else
		CheckDlgButton(IDC_IRC_USEPERFORM,0);
	
	GetDlgItem(IDC_IRC_SERVER_BOX)->SetWindowText(thePrefs.m_strIRCServer);
	GetDlgItem(IDC_IRC_NICK_BOX)->SetWindowText(thePrefs.m_strIRCNick);
	GetDlgItem(IDC_IRC_NAME_BOX)->SetWindowText(thePrefs.m_strIRCChannelFilter);
	GetDlgItem(IDC_IRC_PERFORM_BOX)->SetWindowText(thePrefs.m_strIRCPerformString);
	CString strBuffer;
	strBuffer.Format(_T("%d"), thePrefs.m_uIRCChannelUserFilter);
	GetDlgItem(IDC_IRC_MINUSER_BOX)->SetWindowText(strBuffer);
}

BOOL CPPgIRC::OnApply()
{
	// if prop page is closed by pressing ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();

	if (!UpdateData())
		return FALSE;

	thePrefs.m_bIRCAddTimeStamp = m_bTimeStamp;
	thePrefs.m_bIRCPlaySoundEvents = m_bSoundEvents;
	thePrefs.m_bIRCIgnoreMiscMessages = m_bMiscMessage;
	thePrefs.m_bIRCIgnoreJoinMessages = m_bJoinMessage;
	thePrefs.m_bIRCIgnorePartMessages = m_bPartMessage;
	thePrefs.m_bIRCIgnoreQuitMessages = m_bQuitMessage;
	thePrefs.m_bIRCIgnoreEmuleAddFriendMsgs = m_bEmuleAddFriend;
	thePrefs.m_bIRCAllowEmuleAddFriend = m_bEmuleAllowAddFriend;
	thePrefs.m_bIRCIgnoreEmuleSendLinkMsgs = m_bEmuleSendLink;
	thePrefs.m_bIRCAcceptLinks = m_bAcceptLinks;
	thePrefs.m_bIRCAcceptLinksFriendsOnly = m_bIRCAcceptLinksFriendsOnly;
	thePrefs.m_bIRCJoinHelpChannel = m_bHelpChannel;
	thePrefs.m_bIRCGetChannelsOnConnect = m_bChannelsOnConnect;
	bool bOldSmileys = thePrefs.GetIRCEnableSmileys();
	thePrefs.m_bIRCEnableSmileys = m_bIRCEnableSmileys;
	if (bOldSmileys != thePrefs.GetIRCEnableSmileys())
		theApp.emuledlg->ircwnd->EnableSmileys(thePrefs.GetIRCEnableSmileys());

	if(IsDlgButtonChecked(IDC_IRC_USECHANFILTER))
		thePrefs.m_bIRCUseChannelFilter = true;
	else
		thePrefs.m_bIRCUseChannelFilter = false;

	if(IsDlgButtonChecked(IDC_IRC_USEPERFORM))
		thePrefs.m_bIRCUsePerform = true;
	else
		thePrefs.m_bIRCUsePerform = false;

	TCHAR buffer[510];
	if (GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowText(buffer, 20);
		if (_tcscmp(buffer, thePrefs.m_strIRCNick) != 0)
		{
			CString input;
			input.Format(_T("%s"), buffer);
			input.Trim();
			input = input.SpanExcluding(_T(" !@#$%^&*():;<>,.?{}~`+=-"));
			if (input != _T(""))
			{
				theApp.emuledlg->ircwnd->SendString(_T("NICK ") + input);
				thePrefs.m_strIRCNick = input;
			}
		}
	}

	if (GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowTextLength())
		GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowText(thePrefs.m_strIRCServer);
	GetDlgItem(IDC_IRC_NAME_BOX)->GetWindowText(thePrefs.m_strIRCChannelFilter);
	GetDlgItem(IDC_IRC_PERFORM_BOX)->GetWindowText(thePrefs.m_strIRCPerformString);

	if(GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowText(buffer,6);
		thePrefs.m_uIRCChannelUserFilter = _tstoi(buffer);
	}
	else
		thePrefs.m_uIRCChannelUserFilter = 0;

	LoadSettings();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgIRC::Localize(void)
{
	if(m_hWnd)
	{
		GetDlgItem(IDC_IRC_SERVER_FRM)->SetWindowText(GetResString(IDS_PW_SERVER));
		GetDlgItem(IDC_IRC_MISC_FRM)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_IRC_NICK_FRM)->SetWindowText(GetResString(IDS_PW_NICK));
		GetDlgItem(IDC_IRC_NAME_TEXT)->SetWindowText(GetResString(IDS_IRC_NAME));
		GetDlgItem(IDC_IRC_MINUSER_TEXT)->SetWindowText(GetResString(IDS_UUSERS));
		GetDlgItem(IDC_IRC_FILTER_FRM)->SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
		GetDlgItem(IDC_IRC_USECHANFILTER)->SetWindowText(GetResString(IDS_IRC_USEFILTER));
		GetDlgItem(IDC_IRC_PERFORM_FRM)->SetWindowText(GetResString(IDS_IRC_PERFORM));
		GetDlgItem(IDC_IRC_USEPERFORM)->SetWindowText(GetResString(IDS_IRC_USEPERFORM));

		if (m_htiSoundEvents) m_ctrlTreeOptions.SetItemText(m_htiSoundEvents, GetResString(IDS_IRC_SOUNDEVENTS));
		if (m_htiTimeStamp) m_ctrlTreeOptions.SetItemText(m_htiTimeStamp, GetResString(IDS_IRC_ADDTIMESTAMP));
		if (m_htiInfoMessage) m_ctrlTreeOptions.SetItemText(m_htiInfoMessage, GetResString(IDS_IRC_IGNOREINFOMESSAGE));
		if (m_htiMiscMessage) m_ctrlTreeOptions.SetItemText(m_htiMiscMessage, GetResString(IDS_IRC_IGNOREMISCMESSAGE));
		if (m_htiJoinMessage) m_ctrlTreeOptions.SetItemText(m_htiJoinMessage, GetResString(IDS_IRC_IGNOREJOINMESSAGE));
		if (m_htiPartMessage) m_ctrlTreeOptions.SetItemText(m_htiPartMessage, GetResString(IDS_IRC_IGNOREPARTMESSAGE));
		if (m_htiQuitMessage) m_ctrlTreeOptions.SetItemText(m_htiQuitMessage, GetResString(IDS_IRC_IGNOREQUITMESSAGE));
		if (m_htiEmuleProto) m_ctrlTreeOptions.SetItemText(m_htiEmuleProto, GetResString(IDS_IRC_EMULEPROTO_IGNOREINFOMESSAGE));
		if (m_htiEmuleAddFriend) m_ctrlTreeOptions.SetItemText(m_htiEmuleAddFriend, GetResString(IDS_IRC_EMULEPROTO_IGNOREADDFRIEND));
		if (m_htiEmuleAllowAddFriend) m_ctrlTreeOptions.SetItemText(m_htiEmuleAllowAddFriend, GetResString(IDS_IRC_EMULEPROTO_ALLOWADDFRIEND));
		if (m_htiEmuleSendLink) m_ctrlTreeOptions.SetItemText(m_htiEmuleSendLink, GetResString(IDS_IRC_EMULEPROTO_IGNORESENDLINK));
		if (m_htiAcceptLinks) m_ctrlTreeOptions.SetItemText(m_htiAcceptLinks, GetResString(IDS_IRC_ACCEPTLINKS));
		if (m_htiAcceptLinksFriends) m_ctrlTreeOptions.SetItemText(m_htiAcceptLinksFriends, GetResString(IDS_IRC_ACCEPTLINKSFRIENDS));
		if (m_htiHelpChannel) m_ctrlTreeOptions.SetItemText(m_htiHelpChannel, GetResString(IDS_IRC_HELPCHANNEL));
		if (m_htiChannelsOnConnect) m_ctrlTreeOptions.SetItemText(m_htiChannelsOnConnect, GetResString(IDS_IRC_LOADCHANNELLISTONCON));
		if (m_htiSmileys) m_ctrlTreeOptions.SetItemText(m_htiSmileys, GetResString(IDS_SHOWSMILEYS));
	}
}

void CPPgIRC::OnBtnClickPerform()
{
	SetModified();
	UpdateControls();
}

void CPPgIRC::UpdateControls()
{
	GetDlgItem(IDC_IRC_PERFORM_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USEPERFORM) );
	GetDlgItem(IDC_IRC_NAME_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );
	GetDlgItem(IDC_IRC_MINUSER_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );
}

void CPPgIRC::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;
	m_htiAcceptLinks = NULL;
	m_htiAcceptLinksFriends = NULL;
	m_htiEmuleProto = NULL;
	m_htiEmuleAddFriend = NULL;
	m_htiEmuleAllowAddFriend = NULL;
	m_htiEmuleSendLink = NULL;
	m_htiHelpChannel = NULL;
	m_htiChannelsOnConnect = NULL;
	m_htiSoundEvents = NULL;
	m_htiInfoMessage = NULL;
	m_htiMiscMessage = NULL;
	m_htiJoinMessage = NULL;
	m_htiPartMessage = NULL;
	m_htiQuitMessage = NULL;
	m_htiSmileys = NULL;
	m_htiTimeStamp = NULL;
    CPropertyPage::OnDestroy();
}

LRESULT CPPgIRC::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_MISC_IRC){
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		if (pton->hItem == m_htiAcceptLinks){
			BOOL bCheck;
			if (m_ctrlTreeOptions.GetCheckBox(m_htiAcceptLinks, bCheck))
				m_ctrlTreeOptions.SetCheckBoxEnable(m_htiAcceptLinksFriends, bCheck);
		}
		SetModified();
	}
	return 0;
}

void CPPgIRC::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_IRC);
}

BOOL CPPgIRC::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgIRC::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}
